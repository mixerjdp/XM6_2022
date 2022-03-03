//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 ＰＩ．(ytanaka@ipc-tokai.or.jp)
//	[ FDD(FD55GFR) ]
//
//---------------------------------------------------------------------------

#include "os.h"
#include "xm6.h"
#include "vm.h"
#include "log.h"
#include "schedule.h"
#include "filepath.h"
#include "fileio.h"
#include "rtc.h"
#include "iosc.h"
#include "config.h"
#include "fdc.h"
#include "fdi.h"
#include "fdd.h"

//===========================================================================
//
//	FDD
//
//===========================================================================
//#define FDD_LOG

//---------------------------------------------------------------------------
//
//	コンストラクタ
//
//---------------------------------------------------------------------------
FDD::FDD(VM *p) : Device(p)
{
	// デバイスIDを初期化
	dev.id = MAKEID('F', 'D', 'D', ' ');
	dev.desc = "Floppy Drive";

	// オブジェクト
	fdc = NULL;
	iosc = NULL;
	rtc = NULL;
}

//---------------------------------------------------------------------------
//
//	初期化
//
//---------------------------------------------------------------------------
BOOL FASTCALL FDD::Init()
{
	int i;
	Scheduler *scheduler;

	ASSERT(this);

	// 基本クラス
	if (!Device::Init()) {
		return FALSE;
	}

	// FDC取得
	fdc = (FDC*)vm->SearchDevice(MAKEID('F', 'D', 'C', ' '));
	ASSERT(fdc);

	// IOSC取得
	iosc = (IOSC*)vm->SearchDevice(MAKEID('I', 'O', 'S', 'C'));
	ASSERT(iosc);

	// スケジューラ取得
	scheduler = (Scheduler*)vm->SearchDevice(MAKEID('S', 'C', 'H', 'E'));
	ASSERT(scheduler);

	// RTC取得
	rtc = (RTC*)vm->SearchDevice(MAKEID('R', 'T', 'C', ' '));
	ASSERT(rtc);

	// ドライブ別の初期化
	for (i=0; i<4; i++) {
		drv[i].fdi = new FDI(this);
		drv[i].next = NULL;
		drv[i].seeking = FALSE;
		drv[i].cylinder = 0;
		drv[i].insert = FALSE;
		drv[i].invalid = FALSE;
		drv[i].eject = TRUE;
		drv[i].blink = FALSE;
		drv[i].access = FALSE;
	}

	// ApplyCfg部分
	fdd.fast = FALSE;

	// 共通部分の初期化
	fdd.motor = FALSE;
	fdd.settle = FALSE;
	fdd.force = FALSE;
	fdd.first = TRUE;
	fdd.selected = 0;
	fdd.hd = TRUE;

	// シークイベント初期化
	seek.SetDevice(this);
	seek.SetDesc("Seek");
	seek.SetUser(0);
	seek.SetTime(0);
	scheduler->AddEvent(&seek);

	// 回転数イベント初期化(セトリング兼用)
	rotation.SetDevice(this);
	rotation.SetDesc("Rotation Stopped");
	rotation.SetUser(1);
	rotation.SetTime(0);
	scheduler->AddEvent(&rotation);

	// イジェクトイベント初期化(誤挿入兼用)
	eject.SetDevice(this);
	eject.SetDesc("Eject");
	eject.SetUser(2);
	eject.SetTime(0);
	scheduler->AddEvent(&eject);

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	クリーンアップ
//
//---------------------------------------------------------------------------
void FASTCALL FDD::Cleanup()
{
	int i;

	ASSERT(this);

	// イメージファイルを解放
	for (i=0; i<4; i++) {
		if (drv[i].fdi) {
			delete drv[i].fdi;
			drv[i].fdi = NULL;
		}
		if (drv[i].next) {
			delete drv[i].next;
			drv[i].next = NULL;
		}
	}

	// 基本クラスへ
	Device::Cleanup();
}

//---------------------------------------------------------------------------
//
//	リセット
//
//---------------------------------------------------------------------------
void FASTCALL FDD::Reset()
{
	int i;

	ASSERT(this);
	LOG0(Log::Normal, "リセット");

	// ドライブ別のリセット
	for (i=0; i<4; i++) {
		drv[i].seeking = FALSE;
		drv[i].eject = TRUE;
		drv[i].blink = FALSE;
		drv[i].access = FALSE;

		// nextがいれば、格上げ
		if (drv[i].next) {
			delete drv[i].fdi;
			drv[i].fdi = drv[i].next;
			drv[0].fdi->Adjust();
			drv[1].fdi->Adjust();
			drv[i].next = NULL;
			drv[i].insert = TRUE;
			drv[i].invalid = FALSE;
		}

		// invalidなら、イジェクト状態
		if (drv[i].invalid) {
			delete drv[i].fdi;
			drv[i].fdi = new FDI(this);
			drv[i].insert = FALSE;
			drv[i].invalid = FALSE;
		}

		// シリンダ0へシーク
		drv[i].cylinder = 0;
		drv[i].fdi->Seek(0);
	}

	// 共通部分のリセット(selectedはFDCのDSRと合わせる事)
	fdd.motor = FALSE;
	fdd.settle = FALSE;
	fdd.force = FALSE;
	fdd.first = TRUE;
	fdd.selected = 0;
	fdd.hd = TRUE;

	// シークイベントなし(seeking=FALSE)
	seek.SetTime(0);

	// 回転数・セトリングイベントなし(motor=FALSE, settle=FALSE)
	rotation.SetDesc("Rotation Stopped");
	rotation.SetTime(0);

	// イジェクトイベントなし(格上げ＆invalid)
	eject.SetTime(0);
}

//---------------------------------------------------------------------------
//
//	セーブ
//
//---------------------------------------------------------------------------
BOOL FASTCALL FDD::Save(Fileio *fio, int ver)
{
	int i;
	size_t sz;

	ASSERT(this);
	ASSERT(fio);

	LOG0(Log::Normal, "セーブ");

	// ドライブ個別部分をセーブ
	for (i=0; i<4; i++) {
		// サイズをセーブ
		sz = sizeof(drv_t);
		if (!fio->Write(&sz, sizeof(sz))) {
			return FALSE;
		}

		// 実体をセーブ
		if (!fio->Write(&drv[i], (int)sz)) {
			return FALSE;
		}

		// イメージ部分は任せる
		if (drv[i].fdi) {
			if (!drv[i].fdi->Save(fio, ver)) {
				return FALSE;
			}
		}
		if (drv[i].next) {
			if (!drv[i].next->Save(fio, ver)) {
				return FALSE;
			}
		}
	}

	// 共通部分をセーブ
	sz = sizeof(fdd);
	if (!fio->Write(&sz, sizeof(sz))) {
		return FALSE;
	}
	if (!fio->Write(&fdd, (int)sz)) {
		return FALSE;
	}

	// イベントをセーブ
	if (!seek.Save(fio, ver)) {
		return FALSE;
	}
	if (!rotation.Save(fio, ver)) {
		return FALSE;
	}
	if (!eject.Save(fio, ver)) {
		return FALSE;
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	ロード
//
//---------------------------------------------------------------------------
BOOL FASTCALL FDD::Load(Fileio *fio, int ver)
{
	int i;
	size_t sz;
	drv_t work;
	BOOL ready;
	Filepath path;
	int media;
	BOOL success;
	BOOL failed;

	ASSERT(this);
	ASSERT(fio);

	LOG0(Log::Normal, "ロード");

	// ドライブ個別部分をセーブ
	for (i=0; i<4; i++) {
		// サイズをロード
		if (!fio->Read(&sz, sizeof(sz))) {
			return FALSE;
		}
		if (sz != sizeof(drv_t)) {
			return FALSE;
		}

		// 実体をロード
		if (!fio->Read(&work, (int)sz)) {
			return FALSE;
		}

		// 現在のイメージをすべて削除
		if (drv[i].fdi) {
			delete drv[i].fdi;
			drv[i].fdi = NULL;
		}
		if (drv[i].next) {
			delete drv[i].next;
			drv[i].next = NULL;
		}

		// 転送
		drv[i] = work;

		// イメージ部分を再構築
		failed = FALSE;
		if (drv[i].fdi) {
			// 構築(現在のdrv[i].fdiは意味をもたないため)
			drv[i].fdi = new FDI(this);

			// ロードを試みる
			success = FALSE;
			if (drv[i].fdi->Load(fio, ver, &ready, &media, path)) {
				// レディの場合
				if (ready) {
					// オープンを試みる
					if (drv[i].fdi->Open(path, media)) {
						// 成功
						drv[i].fdi->Seek(drv[i].cylinder);
						success = TRUE;
					}
					else {
						// 失敗(セーブする時点はレディだったので、イジェクト)
						failed = TRUE;
					}
				}
				else {
					// レディでなければ、ロードOK
					success = TRUE;
				}
			}

			// 失敗した場合、念のため再作成
			if (!success) {
				delete drv[i].fdi;
				drv[i].fdi = new FDI(this);
			}
		}
		if (drv[i].next) {
			// 構築(現在のdrv[i].nextは意味をもたないため)
			drv[i].next = new FDI(this);

			// ロードを試みる
			success = FALSE;
			if (drv[i].next->Load(fio, ver, &ready, &media, path)) {
				// レディの場合
				if (ready) {
					// オープンを試みる
					if (drv[i].next->Open(path, media)) {
						// 成功
						drv[i].next->Seek(drv[i].cylinder);
						success = TRUE;
					}
				}
			}

			// 失敗した場合は、deleteしてnextを取り外す
			if (!success) {
				delete drv[i].next;
				drv[i].next = NULL;
			}
		}

		// 本体FDIの再オープンに失敗した場合、強制イジェクトを起こす
		if (failed) {
			Eject(i, TRUE);
		}
	}

	// 共通部分をロード
	if (!fio->Read(&sz, sizeof(sz))) {
		return FALSE;
	}
	if (sz != sizeof(fdd)) {
		return FALSE;
	}
	if (!fio->Read(&fdd, (int)sz)) {
		return FALSE;
	}

	// イベントをロード
	if (!seek.Load(fio, ver)) {
		return FALSE;
	}
	if (!rotation.Load(fio, ver)) {
		return FALSE;
	}
	if (!eject.Load(fio, ver)) {
		return FALSE;
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	設定適用
//
//---------------------------------------------------------------------------
void FASTCALL FDD::ApplyCfg(const Config *config)
{
	ASSERT(this);
	ASSERT(config);
	LOG0(Log::Normal, "設定適用");

	// 高速モード
	fdd.fast = config->floppy_speed;
#if defined(FDD_LOG)
	if (fdd.fast) {
		LOG0(Log::Normal, "高速モード ON");
	}
	else {
		LOG0(Log::Normal, "高速モード OFF");
	}
#endif	// FDD_LOG
}

//---------------------------------------------------------------------------
//
//	ドライブワーク取得
//
//---------------------------------------------------------------------------
void FASTCALL FDD::GetDrive(int drive, drv_t *buffer) const
{
	ASSERT(this);
	ASSERT(buffer);
	ASSERT((drive >= 0) && (drive <= 3));

	// ドライブワークをコピー
	*buffer = drv[drive];
}

//---------------------------------------------------------------------------
//
//	内部ワーク取得
//
//---------------------------------------------------------------------------
void FASTCALL FDD::GetFDD(fdd_t *buffer) const
{
	ASSERT(this);
	ASSERT(buffer);

	// 内部ワークをコピー
	*buffer = fdd;
}

//---------------------------------------------------------------------------
//
//	FDI取得
//
//---------------------------------------------------------------------------
FDI* FASTCALL FDD::GetFDI(int drive)
{
	ASSERT(this);
	ASSERT((drive == 0) || (drive == 1));

	// nextがあれば、nextを優先
	if (drv[drive].next) {
		return drv[drive].next;
	}

	// fdiは必ず存在
	ASSERT(drv[drive].fdi);
	return drv[drive].fdi;
}

//---------------------------------------------------------------------------
//
//	イベントコールバック
//
//---------------------------------------------------------------------------
BOOL FASTCALL FDD::Callback(Event *ev)
{
	DWORD user;
	int i;

	ASSERT(this);
	ASSERT(ev);

	// ユーザデータを受け取る
	user = ev->GetUser();

	switch (user) {
		// 0:シーク
		case 0:
			for (i=0; i<4; i++) {
				// シーク中か
				if (drv[i].seeking) {
					// シーク完了
					drv[i].seeking = FALSE;
#if defined(FDD_LOG)
					LOG1(Log::Normal, "シーク完了 ドライブ%d", i);
#endif	// FDD_LOG

					// レディ状態によって分ける
					fdc->CompleteSeek(i, IsReady(i));
				}
			}
			// 単発なのでbreak
			break;

		// 1:回転・セトリング
		case 1:
			// スタンバイ中なら単発
			if (!fdd.settle && !fdd.motor) {
				return FALSE;
			}

			// セトリング完了か
			if (fdd.settle) {
				fdd.settle = FALSE;
				fdd.motor = TRUE;
				fdd.first = TRUE;

				// 回転
				Rotation();
			}
			// 継続
			return TRUE;

		// 2:イジェクト・誤挿入
		case 2:
			for (i=0; i<4; i++) {
				// Nextがあれば、入れ替え
				if (drv[i].next) {
					delete drv[i].fdi;
					drv[i].fdi = drv[i].next;
					drv[0].fdi->Adjust();
					drv[1].fdi->Adjust();
					drv[i].next = NULL;
					Insert(i);
				}

				// invalidでイジェクト
				if (drv[i].invalid) {
					Eject(i, TRUE);
				}
			}
			// 単発なのでbreak
			break;

		// その他(ありえない)
		default:
			ASSERT(FALSE);
			break;
	}

	// 単発処理
	return FALSE;
}

//---------------------------------------------------------------------------
//
//	オープン
//
//---------------------------------------------------------------------------
BOOL FASTCALL FDD::Open(int drive, const Filepath& path, int media)
{
	FDI *fdi;

	ASSERT(this);
	ASSERT((drive >= 0) && (drive <= 3));
	ASSERT((media >= 0) && (media < 0x10));

#if defined(FDD_LOG)
	LOG2(Log::Normal, "ディスクオープン ドライブ%d メディア%d", drive, media);
#endif	// FDD_LOG

	// FDI作成+オープン
	fdi = new FDI(this);
	if (!fdi->Open(path, media)) {
		delete fdi;
		return FALSE;
	}

	// イメージだけ先行してシーク
	fdi->Seek(drv[drive].cylinder);

	// 既に次イメージが予約されている場合
	if (drv[drive].next) {
		// 次イメージを削除して再度イベント(300ms)
		delete drv[drive].next;
		drv[drive].next = fdi;
		eject.SetTime(300 * 1000 * 2);
		return TRUE;
	}

	// メディアがある場合
	if (drv[drive].insert && !drv[drive].invalid) {
		// 次イメージにセットしてイジェクト、イベント発行(300ms)
		Eject(drive, FALSE);
		drv[drive].next = fdi;
		eject.SetTime(300 * 1000 * 2);
		return TRUE;
	}

	// 通常はそのままインサート
	delete drv[drive].fdi;
	drv[drive].fdi = fdi;
	drv[0].fdi->Adjust();
	drv[1].fdi->Adjust();
	Insert(drive);
	return TRUE;
}

//---------------------------------------------------------------------------
//
//	ディスク挿入
//
//---------------------------------------------------------------------------
void FASTCALL FDD::Insert(int drive)
{
	ASSERT(this);
	ASSERT((drive >= 0) && (drive <= 3));

#if defined(FDD_LOG)
	LOG1(Log::Normal, "ディスク挿入 ドライブ%d", drive);
#endif	// FDD_LOG

	// シークは一旦解除
	if (drv[drive].seeking) {
		drv[drive].seeking = FALSE;
		fdc->CompleteSeek(drive, FALSE);
	}

	// ディスク挿入あり、誤挿入なし
	drv[drive].insert = TRUE;
	drv[drive].invalid = FALSE;

	// 現在のシリンダへシーク
	ASSERT(drv[drive].fdi);
	drv[drive].fdi->Seek(drv[drive].cylinder);

	// 割り込みを起こす
	iosc->IntFDD(TRUE);
}

//---------------------------------------------------------------------------
//
//	ディスクイジェクト
//
//---------------------------------------------------------------------------
void FASTCALL FDD::Eject(int drive, BOOL force)
{
	ASSERT(this);
	ASSERT((drive >= 0) && (drive <= 3));

	// 強制フラグが立っていなければ、イジェクト禁止を考慮
	if (!force && !drv[drive].eject) {
		return;
	}

	// 既にイジェクトされていれば不要
	if (!drv[drive].insert && !drv[drive].invalid) {
		return;
	}

#if defined(FDD_LOG)
	LOG1(Log::Normal, "ディスクイジェクト ドライブ%d", drive);
#endif	// FDD_LOG

	// シークは一旦解除
	if (drv[drive].seeking) {
		drv[drive].seeking = FALSE;
		fdc->CompleteSeek(drive, FALSE);
	}

	// イメージファイルをFDIに入れ替える
	ASSERT(drv[drive].fdi);
	delete drv[drive].fdi;
	drv[drive].fdi = new FDI(this);

	// ディスク挿入なし、誤挿入なし
	drv[drive].insert = FALSE;
	drv[drive].invalid = FALSE;

	// アクセスなし(LED上の都合)
	drv[drive].access = FALSE;

	// nextは一旦解除
	if (drv[drive].next) {
		delete drv[drive].next;
		drv[drive].next = NULL;
	}

	// 割り込みを起こす
	iosc->IntFDD(TRUE);
}

//---------------------------------------------------------------------------
//
//	ディスク誤挿入
//
//---------------------------------------------------------------------------
void FASTCALL FDD::Invalid(int drive)
{
	ASSERT(this);
	ASSERT((drive >= 0) && (drive <= 3));

#if defined(FDD_LOG)
	LOG1(Log::Normal, "ディスク誤挿入 ドライブ%d", drive);
#endif	// FDD_LOG

	// 既に誤挿入であれば不要
	if (drv[drive].insert && drv[drive].invalid) {
		return;
	}

	// シークは一旦解除
	if (drv[drive].seeking) {
		drv[drive].seeking = FALSE;
		fdc->CompleteSeek(drive, FALSE);
	}

	// ディスク挿入あり、誤挿入あり
	drv[drive].insert = TRUE;
	drv[drive].invalid = TRUE;

	// nextは一旦解除
	if (drv[drive].next) {
		delete drv[drive].next;
		drv[drive].next = NULL;
	}

	// 割り込みを起こす
	iosc->IntFDD(TRUE);

	// イベントを設定(300ms)
	eject.SetTime(300 * 1000 * 2);
}

//---------------------------------------------------------------------------
//
//	ドライブコントロール
//
//---------------------------------------------------------------------------
void FASTCALL FDD::Control(int drive, DWORD func)
{
	ASSERT(this);
	ASSERT((drive >= 0) && (drive <= 3));

	// bit7 - LED制御
	if (func & 0x80) {
		if (!drv[drive].blink) {
			// 点滅なし→点滅あり
#if defined(FDD_LOG)
			LOG1(Log::Normal, "LED点滅あり ドライブ%d", drive);
#endif	// FDD_LOG
			drv[drive].blink = TRUE;
		}
	}
	else {
		drv[drive].blink = FALSE;
	}

	// bit6 - イジェクト有効
	if (func & 0x40) {
		if (drv[drive].eject) {
#if defined(FDD_LOG)
			LOG1(Log::Normal, "イジェクト禁止 ドライブ%d", drive);
#endif	// FDD_LOG
			drv[drive].eject = FALSE;
		}
	}
	else {
		drv[drive].eject = TRUE;
	}

	// bit5 - イジェクト
	if (func & 0x20) {
		Eject(drive, TRUE);
	}
}

//---------------------------------------------------------------------------
//
//	強制レディ
//
//---------------------------------------------------------------------------
void FASTCALL FDD::ForceReady(BOOL flag)
{
	ASSERT(this);

#if defined(FDD_LOG)
	if (flag) {
		LOG0(Log::Normal, "強制レディ ON");
	}
	else {
		LOG0(Log::Normal, "強制レディ OFF");
	}
#endif	// FDD_LOG

	// セットのみ
	fdd.force = flag;
}

//---------------------------------------------------------------------------
//
//	回転位置取得
//
//---------------------------------------------------------------------------
DWORD FASTCALL FDD::GetRotationPos() const
{
	DWORD remain;
	DWORD hus;

	ASSERT(this);

	// 停止していれば0
	if (rotation.GetTime() == 0) {
		return 0;
	}

	// 回転数を得る
	hus = GetRotationTime();

	// ダウンカウンタを得る
	remain = rotation.GetRemain();
	if (remain > hus) {
		remain = 0;
	}

	// 逆順に
	return (DWORD)(hus - remain);
}

//---------------------------------------------------------------------------
//
//	回転時間取得
//
//---------------------------------------------------------------------------
DWORD FASTCALL FDD::GetRotationTime() const
{
	ASSERT(this);

	// 2HDは360rpm、2DDは300rpm
	if (fdd.hd) {
		return 333333;
	}
	else {
		return 400000;
	}
}

//---------------------------------------------------------------------------
//
//	回転
//
//---------------------------------------------------------------------------
void FASTCALL FDD::Rotation()
{
	DWORD rpm;
	DWORD hus;
	char desc[0x20];

	ASSERT(this);
	ASSERT(!fdd.settle);
	ASSERT(fdd.motor);

	rpm = 2000 * 1000 * 60;
	hus = GetRotationTime();
	rpm /= hus;
	sprintf(desc, "Rotation %drpm", rpm);
	rotation.SetDesc(desc);
	rotation.SetTime(hus);
}

//---------------------------------------------------------------------------
//
//	検索時間取得
//
//---------------------------------------------------------------------------
DWORD FASTCALL FDD::GetSearch()
{
	DWORD schtime;

	ASSERT(this);

	// 高速モード時は64us固定
	if (fdd.fast) {
		return 128;
	}

	// イメージファイルに聞く
	schtime = drv[ fdd.selected ].fdi->GetSearch();

	return schtime;
}

//---------------------------------------------------------------------------
//
//	HD設定
//
//---------------------------------------------------------------------------
void FASTCALL FDD::SetHD(BOOL hd)
{

	ASSERT(this);

	if (hd) {
		if (fdd.hd) {
			return;
		}
#if defined(FDD_LOG)
		LOG0(Log::Normal, "ドライブ密度変更 2HD");
#endif	// FDD_LOG
		fdd.hd = TRUE;
	}
	else {
		if (!fdd.hd) {
			return;
		}
#if defined(FDD_LOG)
		LOG0(Log::Normal, "ドライブ密度変更 2DD");
#endif	// FDD_LOG
		fdd.hd = FALSE;
	}

	// モータ動作中なら、速度変更
	if (!fdd.motor || fdd.settle) {
		return;
	}

	// 回転を再設定
	Rotation();
}

//---------------------------------------------------------------------------
//
//	HD取得
//
//---------------------------------------------------------------------------
BOOL FASTCALL FDD::IsHD() const
{
	ASSERT(this);

	return fdd.hd;
}

//---------------------------------------------------------------------------
//
//	アクセスLED設定
//
//---------------------------------------------------------------------------
void FASTCALL FDD::Access(BOOL flag)
{
	int i;

	ASSERT(this);

	// すべて下ろす
	for (i=0; i<4; i++) {
		drv[i].access = FALSE;
	}

	// flagが上がっていれば、セレクトドライブが有効
	if (flag) {
		drv[ fdd.selected ].access = TRUE;
	}
}

//---------------------------------------------------------------------------
//
//	レディチェック
//
//---------------------------------------------------------------------------
BOOL FASTCALL FDD::IsReady(int drive, BOOL fdc) const
{
	ASSERT(this);
	ASSERT((drive >= 0) && (drive <= 3));

	// FDCからのアクセスなら
	if (fdc) {
		// 強制レディならレディ
		if (fdd.force) {
			return TRUE;
		}

		// モータオフならノットレディ
		if (!fdd.motor) {
			return FALSE;
		}
	}

	// nextがあれば、nextを優先
	if (drv[drive].next) {
		return drv[drive].next->IsReady();
	}

	// イメージファイルに聞く
	return drv[drive].fdi->IsReady();
}

//---------------------------------------------------------------------------
//
//	書き込み禁止チェック
//
//---------------------------------------------------------------------------
BOOL FASTCALL FDD::IsWriteP(int drive) const
{
	ASSERT(this);
	ASSERT((drive >= 0) && (drive <= 3));

	// nextがあれば、nextを優先
	if (drv[drive].next) {
		return drv[drive].next->IsWriteP();
	}

	// イメージファイルに聞く
	return drv[drive].fdi->IsWriteP();
}

//---------------------------------------------------------------------------
//
//	Read Onlyチェック
//
//---------------------------------------------------------------------------
BOOL FASTCALL FDD::IsReadOnly(int drive) const
{
	ASSERT(this);
	ASSERT((drive >= 0) && (drive <= 3));

	// nextがあれば、nextを優先
	if (drv[drive].next) {
		return drv[drive].next->IsReadOnly();
	}

	// イメージファイルに聞く
	return drv[drive].fdi->IsReadOnly();
}

//---------------------------------------------------------------------------
//
//	書き込み禁止
//
//---------------------------------------------------------------------------
void FASTCALL FDD::WriteP(int drive, BOOL flag)
{
	ASSERT(this);
	ASSERT((drive >= 0) && (drive <= 3));

	// イメージファイルで処理
	drv[drive].fdi->WriteP(flag);
}

//---------------------------------------------------------------------------
//
//	ドライブステータス取得
//
//	b7 : Insert
//	b6 : Invalid Insert
//	b5 : Inhibit Eject
//	b4 : Blink (Global)
//	b3 : Blink (Current)
//	b2 : Motor
//	b1 : Select
//	b0 : Access
//---------------------------------------------------------------------------
int FASTCALL FDD::GetStatus(int drive) const
{
	int st;

	ASSERT(this);
	ASSERT((drive >= 0) && (drive <= 3));

	// クリア
	st = 0;

	// bit7 - 挿入
	if (drv[drive].insert) {
		st |= FDST_INSERT;
	}

	// bit6 - 誤挿入(挿入を含む)
	if (drv[drive].invalid) {
		st |= FDST_INVALID;
	}

	// bit5 - イジェクトできる
	if (drv[drive].eject) {
		st |= FDST_EJECT;
	}

	// bit4 - 点滅(グローバル、挿入を含まない)
	if (drv[drive].blink && !drv[drive].insert) {
		st |= FDST_BLINK;

		// bit3 - 点滅(現在)
		if (rtc->GetBlink(drive)) {
			st |= FDST_CURRENT;
		}
	}

	// bit2 - モータ
	if (fdd.motor) {
		st |= FDST_MOTOR;
	}

	// bit1 - セレクト
	if (drive == fdd.selected) {
		st |= FDST_SELECT;
	}

	// bit0 - アクセス
	if (drv[drive].access) {
		st |= FDST_ACCESS;
	}

	return st;
}

//---------------------------------------------------------------------------
//
//	モータ設定＋ドライブセレクト
//
//---------------------------------------------------------------------------
void FASTCALL FDD::SetMotor(int drive, BOOL flag)
{
	ASSERT(this);
	ASSERT((drive >= 0) && (drive <= 3));

	// flag=FALSEなら簡単
	if (!flag) {
		// すでにOFFなら必要ない
		if (!fdd.motor) {
			return;
		}
#if defined(FDD_LOG)
		LOG0(Log::Normal, "モータOFF");
#endif	// FDD_LOG

		// モータ停止
		fdd.motor = FALSE;
		fdd.settle = FALSE;

		// セレクトドライブ設定
		fdd.selected = drive;

		// スタンバイイベントを設定
		rotation.SetDesc("Standby 54000ms");
		rotation.SetTime(54 * 1000 * 2 * 1000);
		return;
	}

	// セレクトドライブ設定
	fdd.selected = drive;

	// モータONまたはセトリング中なら問題なし
	if (fdd.motor || fdd.settle) {
		return;
	}

#if defined(FDD_LOG)
	LOG1(Log::Normal, "モータON ドライブ%dセレクト", drive);
#endif	// FDD_LOG

	// ここでイベントが動いていれば、スタンバイ中
	if (rotation.GetTime() != 0) {
		// 即モータON
		fdd.settle = FALSE;
		fdd.motor = TRUE;
		fdd.first = TRUE;

		// 回転
		Rotation();
		return;
	}

	// モータOFF、セトリング中
	fdd.motor = FALSE;
	fdd.settle = TRUE;

	// セトリングイベントを設定(高速モード時64us、通常モード時384ms)
	rotation.SetDesc("Settle 384ms");
	if (fdd.fast) {
		rotation.SetTime(128);
	}
	else {
		rotation.SetTime(384 * 1000 * 2);
	}
}

//---------------------------------------------------------------------------
//
//	シリンダ取得
//
//---------------------------------------------------------------------------
int FASTCALL FDD::GetCylinder(int drive) const
{
	ASSERT(this);
	ASSERT((drive >= 0) && (drive <= 3));

	// ワークを返す
	return drv[drive].cylinder;
}

//---------------------------------------------------------------------------
//
//	ディスク名取得
//
//---------------------------------------------------------------------------
void FASTCALL FDD::GetName(int drive, char *buf, int media) const
{
	ASSERT(this);
	ASSERT((drive >= 0) && (drive <= 3));
	ASSERT(buf);
	ASSERT((media >= -1) && (media < 0x10));

	// nextがあれば、nextを優先
	if (drv[drive].next) {
		drv[drive].next->GetName(buf, media);
		return;
	}

	// イメージに聞く
	drv[drive].fdi->GetName(buf, media);
}

//---------------------------------------------------------------------------
//
//	パス取得
//
//---------------------------------------------------------------------------
void FASTCALL FDD::GetPath(int drive, Filepath& path) const
{
	ASSERT(this);
	ASSERT((drive >= 0) && (drive <= 3));

	// nextがあれば、nextを優先
	if (drv[drive].next) {
		drv[drive].next->GetPath(path);
		return;
	}

	// イメージに聞く
	drv[drive].fdi->GetPath(path);
}

//---------------------------------------------------------------------------
//
//	ディスク枚数取得
//
//---------------------------------------------------------------------------
int FASTCALL FDD::GetDisks(int drive) const
{
	ASSERT(this);
	ASSERT((drive >= 0) && (drive <= 3));

	// nextがあれば、nextを優先
	if (drv[drive].next) {
		return drv[drive].next->GetDisks();
	}

	// イメージに聞く
	return drv[drive].fdi->GetDisks();
}

//---------------------------------------------------------------------------
//
//	カレントメディア番号取得
//
//---------------------------------------------------------------------------
int FASTCALL FDD::GetMedia(int drive) const
{
	ASSERT(this);
	ASSERT((drive >= 0) && (drive <= 3));

	// nextがあれば、nextを優先
	if (drv[drive].next) {
		return drv[drive].next->GetMedia();
	}

	// イメージに聞く
	return drv[drive].fdi->GetMedia();
}

//---------------------------------------------------------------------------
//
//	リキャリブレート
//
//---------------------------------------------------------------------------
void FASTCALL FDD::Recalibrate(DWORD srt)
{
	ASSERT(this);

	// 一致チェック
	if (drv[ fdd.selected ].cylinder == 0) {
		fdc->CompleteSeek(fdd.selected, IsReady(fdd.selected));
		return;
	}

#if defined(FDD_LOG)
	LOG1(Log::Normal, "リキャリブレート ドライブ%d", fdd.selected);
#endif	// FDD_LOG

	// fdd[drive].cylinderだけステップアウト
	StepOut(drv[ fdd.selected ].cylinder, srt);
}

//---------------------------------------------------------------------------
//
//	ステップイン
//
//---------------------------------------------------------------------------
void FASTCALL FDD::StepIn(int step, DWORD srt)
{
	int cylinder;

	ASSERT(this);
	ASSERT((step >= 0) && (step < 256));

	// ステップイン
	cylinder = drv[ fdd.selected ].cylinder;
	cylinder += step;
	if (cylinder >= 82) {
		cylinder = 81;
	}

	// 一致チェック
	if (drv[ fdd.selected ].cylinder == cylinder) {
		// すぐ割り込みを出す
		fdc->CompleteSeek(fdd.selected, IsReady(fdd.selected));
		return;
	}

#if defined(FDD_LOG)
	LOG2(Log::Normal, "ステップイン ドライブ%d ステップ%d", fdd.selected, step);
#endif	// FDD_LOG

	// シーク共通へ
	SeekInOut(cylinder, srt);
}

//---------------------------------------------------------------------------
//
//	ステップアウト
//
//---------------------------------------------------------------------------
void FASTCALL FDD::StepOut(int step, DWORD srt)
{
	int cylinder;

	ASSERT(this);
	ASSERT((step >= 0) && (step < 256));

	// ステップアウト
	cylinder = drv[ fdd.selected ].cylinder;
	cylinder -= step;
	if (cylinder < 0) {
		cylinder = 0;
	}

	// 一致チェック
	if (drv[ fdd.selected ].cylinder == cylinder) {
		fdc->CompleteSeek(fdd.selected, IsReady(fdd.selected));
		return;
	}

#if defined(FDD_LOG)
	LOG2(Log::Normal, "ステップアウト ドライブ%d ステップ%d", fdd.selected, step);
#endif	// FDD_LOG

	// シーク共通へ
	SeekInOut(cylinder, srt);
}

//---------------------------------------------------------------------------
//
//	シーク共通
//
//---------------------------------------------------------------------------
void FASTCALL FDD::SeekInOut(int cylinder, DWORD srt)
{
	int step;

	ASSERT(this);
	ASSERT((cylinder >= 0) && (cylinder < 82));

	// ステップ数を計算
	ASSERT(drv[ fdd.selected ].cylinder != cylinder);
	if (drv[ fdd.selected ].cylinder < cylinder) {
		step = cylinder - drv[ fdd.selected ].cylinder;
	}
	else {
		step = drv[ fdd.selected ].cylinder - cylinder;
	}

	// 先にシーク動作を行ってしまう(実行中フラグは立てる)
	drv[ fdd.selected ].cylinder = cylinder;
	drv[ fdd.selected ].fdi->Seek(cylinder);
	drv[ fdd.selected ].seeking = TRUE;

	// イベント設定(SRT * ステップ数 + 0.64ms)
	if (fdd.fast) {
		// 高速モード時は64us固定
		seek.SetTime(128);
	}
	else {
		srt *= step;
		srt += 1280;

		// モータON後の初回は128msを加える
		if (fdd.first) {
			srt += (128 * 0x800);
			fdd.first = FALSE;
		}
		seek.SetTime(srt);
	}
}

//---------------------------------------------------------------------------
//
//	リードID
//	※次のステータスを返す(エラーはORされる)
//		FDD_NOERROR		エラーなし
//		FDD_NOTREADY	ノットレディ
//		FDD_MAM			指定密度ではアンフォーマット
//		FDD_NODATA		指定密度ではアンフォーマットか、
//						または有効なセクタすべてID CRC
//
//---------------------------------------------------------------------------
int FASTCALL FDD::ReadID(DWORD *buf, BOOL mfm, int hd)
{
	ASSERT(this);
	ASSERT(buf);
	ASSERT((hd == 0) || (hd == 4));

#if defined(FDD_LOG)
	LOG2(Log::Normal, "リードID ドライブ%d ヘッド%d", fdd.selected, hd);
#endif	// FDD_LOG

	// イメージに任せる
	return drv[ fdd.selected ].fdi->ReadID(buf, mfm, hd);
}

//---------------------------------------------------------------------------
//
//	リードセクタ
//	※次のステータスを返す(エラーはORされる)
//		FDD_NOERROR		エラーなし
//		FDD_NOTREADY	ノットレディ
//		FDD_MAM			指定密度ではアンフォーマット
//		FDD_NODATA		指定密度ではアンフォーマットか、
//						または有効なセクタを全検索してRが一致しない
//		FDD_NOCYL		検索中にIDのCが一致でず、FFでないセクタを見つけた
//		FDD_BADCYL		検索中にIDのCが一致せず、FFとなっているセクタを見つけた
//		FDD_IDCRC		IDフィールドにCRCエラーがある
//		FDD_DATACRC		DATAフィールドにCRCエラーがある
//		FDD_DDAM		デリーテッドセクタである
//
//---------------------------------------------------------------------------
int FASTCALL FDD::ReadSector(BYTE *buf, int *len, BOOL mfm, DWORD *chrn, int hd)
{
	ASSERT(this);
	ASSERT(buf);
	ASSERT(len);
	ASSERT(chrn);
	ASSERT((hd == 0) || (hd == 4));

#if defined(FDD_LOG)
	LOG4(Log::Normal, "リードセクタ C:%02X H:%02X R:%02X N:%02X",
						chrn[0], chrn[1], chrn[2], chrn[3]);
#endif	// FDD_LOG

	// イメージに任せる
	return drv[ fdd.selected ].fdi->ReadSector(buf, len, mfm, chrn, hd);
}

//---------------------------------------------------------------------------
//
//	ライトセクタ
//	※次のステータスを返す(エラーはORされる)
//		FDD_NOERROR		エラーなし
//		FDD_NOTREADY	ノットレディ
//		FDD_NOTWRITE	メディアは書き込み禁止
//		FDD_MAM			指定密度ではアンフォーマット
//		FDD_NODATA		指定密度ではアンフォーマットか、
//						または有効なセクタを全検索してRが一致しない
//		FDD_NOCYL		検索中にIDのCが一致でず、FFでないセクタを見つけた
//		FDD_BADCYL		検索中にIDのCが一致せず、FFとなっているセクタを見つけた
//		FDD_IDCRC		IDフィールドにCRCエラーがある
//		FDD_DDAM		デリーテッドセクタである
//
//---------------------------------------------------------------------------
int FASTCALL FDD::WriteSector(const BYTE *buf, int *len, BOOL mfm, DWORD *chrn, int hd, BOOL deleted)
{
	ASSERT(this);
	ASSERT(len);
	ASSERT(chrn);
	ASSERT((hd == 0) || (hd == 4));

#if defined(FDD_LOG)
	LOG4(Log::Normal, "ライトセクタ C:%02X H:%02X R:%02X N:%02X",
						chrn[0], chrn[1], chrn[2], chrn[3]);
#endif	// FDD_LOG

	// イメージに任せる
	return drv[ fdd.selected ].fdi->WriteSector(buf, len, mfm, chrn, hd, deleted);
}

//---------------------------------------------------------------------------
//
//	リードダイアグ
//
//---------------------------------------------------------------------------
int FASTCALL FDD::ReadDiag(BYTE *buf, int *len, BOOL mfm, DWORD *chrn, int hd)
{
	ASSERT(this);
	ASSERT(len);
	ASSERT(chrn);
	ASSERT((hd == 0) || (hd == 4));

#if defined(FDD_LOG)
	LOG4(Log::Normal, "リードダイアグ C:%02X H:%02X R:%02X N:%02X",
						chrn[0], chrn[1], chrn[2], chrn[3]);
#endif	// FDD_LOG

	// イメージに任せる
	return drv[ fdd.selected ].fdi->ReadDiag(buf, len, mfm, chrn, hd);
}

//---------------------------------------------------------------------------
//
//	ライトID
//
//---------------------------------------------------------------------------
int FASTCALL FDD::WriteID(const BYTE *buf, DWORD d, int sc, BOOL mfm, int hd, int gpl)
{
	ASSERT(this);
	ASSERT(sc > 0);
	ASSERT((hd == 0) || (hd == 4));

#if defined(FDD_LOG)
	LOG3(Log::Normal, "ライトID ドライブ%d シリンダ%d ヘッド%d",
					fdd.selected, drv[ fdd.selected ].cylinder, hd);
#endif	// FDD_LOG

	// イメージに任せる
	return drv[ fdd.selected ].fdi->WriteID(buf, d, sc, mfm, hd, gpl);
}
