//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 ＰＩ．(ytanaka@ipc-tokai.or.jp)
//	[ SCSI(MB89352) ]
//
//---------------------------------------------------------------------------

#include "os.h"
#include "xm6.h"
#include "vm.h"
#include "log.h"
#include "schedule.h"
#include "memory.h"
#include "sram.h"
#include "config.h"
#include "disk.h"
#include "fileio.h"
#include "filepath.h"
#include "scsi.h"

//===========================================================================
//
//	SCSI
//
//===========================================================================
//#define SCSI_LOG

//---------------------------------------------------------------------------
//
//	コンストラクタ
//
//---------------------------------------------------------------------------
SCSI::SCSI(VM *p) : MemDevice(p)
{
	// デバイスIDを初期化
	dev.id = MAKEID('S', 'C', 'S', 'I');
	dev.desc = "SCSI (MB89352)";

	// 開始アドレス、終了アドレス
	memdev.first = 0xea0000;
	memdev.last = 0xea1fff;

	// ワーク初期化
	memset(&scsi, 0, sizeof(scsi));

	// デバイス
	memory = NULL;
	sram = NULL;
}

//---------------------------------------------------------------------------
//
//	初期化
//
//---------------------------------------------------------------------------
BOOL FASTCALL SCSI::Init()
{
	int i;

	ASSERT(this);

	// 基本クラス
	if (!MemDevice::Init()) {
		return FALSE;
	}

	// メモリ取得
	ASSERT(!memory);
	memory = (Memory*)vm->SearchDevice(MAKEID('M', 'E', 'M', ' '));
	ASSERT(memory);

	// SRAM取得
	ASSERT(!sram);
	sram = (SRAM*)vm->SearchDevice(MAKEID('S', 'R', 'A', 'M'));
	ASSERT(sram);

	// ワーク初期化
	scsi.bdid = 0x80;
	scsi.ilevel = 2;
	scsi.vector = -1;
	scsi.id = -1;
	for (i=0; i<sizeof(scsi.cmd); i++) {
		scsi.cmd[i] = 0;
	}

	// コンフィグ初期化
	scsi.memsw = TRUE;
	scsi.mo_first = FALSE;

	// イベント初期化
	event.SetDevice(this);
	event.SetDesc("Selection");
	event.SetUser(0);
	event.SetTime(0);
	cdda.SetDevice(this);
	cdda.SetDesc("CD-DA 75fps");
	cdda.SetUser(1);
	cdda.SetTime(0);

	// ディスク初期化
	for (i=0; i<DeviceMax; i++) {
		scsi.disk[i] = NULL;
	}
	for (i=0; i<HDMax; i++) {
		scsi.hd[i] = NULL;
	}
	scsi.mo = NULL;
	scsi.cdrom = NULL;

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	クリーンアップ
//
//---------------------------------------------------------------------------
void FASTCALL SCSI::Cleanup()
{
	int i;

	ASSERT(this);

	// HD削除
	for (i=0; i<HDMax; i++) {
		if (scsi.hd[i]) {
			delete scsi.hd[i];
			scsi.hd[i] = NULL;
		}
	}

	// MO削除
	if (scsi.mo) {
		delete scsi.mo;
		scsi.mo = NULL;
	}

	// CD削除
	if (scsi.cdrom) {
		delete scsi.cdrom;
		scsi.cdrom = NULL;
	}

	// 基本クラスへ
	MemDevice::Cleanup();
}

//---------------------------------------------------------------------------
//
//	リセット
//
//---------------------------------------------------------------------------
void FASTCALL SCSI::Reset()
{
	Memory::memtype type;
	int i;

	ASSERT(this);
	LOG0(Log::Normal, "リセット");

	// SCSIタイプ(メモリに問い合わせる)
	type = memory->GetMemType();
	switch (type) {
		// SASIのみ
		case Memory::None:
		case Memory::SASI:
			scsi.type = 0;
			break;

		// 外付
		case Memory::SCSIExt:
			scsi.type = 1;
			break;

		// その他(内蔵)
		default:
			scsi.type = 2;
			break;
	}

	// イベントを動的に追加・削除
	if (scsi.type == 0) {
		// イベント削除
		if (scheduler->HasEvent(&event)) {
			scheduler->DelEvent(&event);
			scheduler->DelEvent(&cdda);
		}
	}
	else {
		// イベント追加
		if (!scheduler->HasEvent(&event)) {
			scheduler->AddEvent(&event);
			scheduler->AddEvent(&cdda);
		}
	}

	// メモリスイッチ書き込み
	if (scsi.memsw) {
		// SCSIが存在する場合のみ
		if (scsi.type != 0) {
			// $ED006F:SCSIフラグ('V'でSCSI有効)
			if (sram->GetMemSw(0x6f) != 'V') {
				sram->SetMemSw(0x6f, 'V');

				// 本体IDを7に初期化
				sram->SetMemSw(0x70, 0x07);
			}

			// $ED0070:ボード種別+本体ID
			if (scsi.type == 1) {
				// 外付
				sram->SetMemSw(0x70, sram->GetMemSw(0x70) | 0x08);
			}
			else {
				// 内蔵
				sram->SetMemSw(0x70, sram->GetMemSw(0x70) & ~0x08);
			}

			// $ED0071:SASIエミュレーションフラグ
			sram->SetMemSw(0x71, 0x00);
		}
	}

	// ディスク再構築
	Construct();

	// ディスクリセット
	for (i=0; i<HDMax; i++) {
		if (scsi.hd[i]) {
			scsi.hd[i]->Reset();
		}
	}
	if (scsi.mo) {
		scsi.mo->Reset();
	}
	if (scsi.cdrom) {
		scsi.cdrom->Reset();
	}

	// コマンドクリア
	for (i=0; i<sizeof(scsi.cmd); i++) {
		scsi.cmd[i] = 0;
	}

	// レジスタリセット
	ResetReg();

	// バスフリーフェーズ
	BusFree();
}

//---------------------------------------------------------------------------
//
//	セーブ
//
//---------------------------------------------------------------------------
BOOL FASTCALL SCSI::Save(Fileio *fio, int ver)
{
	size_t sz;
	int i;
	Disk *disk;

	ASSERT(this);
	ASSERT(fio);

	LOG0(Log::Normal, "セーブ");

	// ディスクをフラッシュ
	for (i=0; i<HDMax; i++) {
		if (scsi.hd[i]) {
			if (!scsi.hd[i]->Flush()) {
				return FALSE;
			}
		}
	}
	if (scsi.mo) {
		if (!scsi.mo->Flush()) {
			return FALSE;
		}
	}
	if (scsi.cdrom) {
		if (!scsi.cdrom->Flush()) {
			return FALSE;
		}
	}

	// サイズをセーブ
	sz = sizeof(scsi_t);
	if (!fio->Write(&sz, sizeof(sz))) {
		return FALSE;
	}

	// 実体をセーブ
	if (!fio->Write(&scsi, (int)sz)) {
		return FALSE;
	}

	// イベントをセーブ
	if (!event.Save(fio, ver)) {
		return FALSE;
	}
	if (!cdda.Save(fio, ver)) {
		return FALSE;
	}

	// パスをセーブ
	for (i=0; i<HDMax; i++) {
		if (!scsihd[i].Save(fio, ver)) {
			return FALSE;
		}
	}

	// ディスクをセーブ
	for (i=0; i<HDMax; i++) {
		if (scsi.hd[i]) {
			if (!scsi.hd[i]->Save(fio, ver)) {
				return FALSE;
			}
		}
		else {
			// 数あわせのため、ダミーでもよいから保存する(version2.04)
			disk = new SCSIHD(this);
			if (!disk->Save(fio, ver)) {
				delete disk;
				return FALSE;
			}
			delete disk;
		}
	}
	if (scsi.mo) {
		if (!scsi.mo->Save(fio, ver)) {
			return FALSE;
		}
	}
	if (scsi.cdrom) {
		if (!scsi.cdrom->Save(fio, ver)) {
			return FALSE;
		}
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	ロード
//
//---------------------------------------------------------------------------
BOOL FASTCALL SCSI::Load(Fileio *fio, int ver)
{
	size_t sz;
	int i;
	Disk *disk;

	ASSERT(this);
	ASSERT(fio);
	ASSERT(ver >= 0x0200);

	LOG0(Log::Normal, "ロード");

	// version2.01以前は、SCSIをサポートしていない
	if (ver <= 0x201) {
		// SCSIなし、ドライブ数0
		scsi.type = 0;
		scsi.scsi_drives = 0;

		// イベント削除
		if (scheduler->HasEvent(&event)) {
			scheduler->DelEvent(&event);
			scheduler->DelEvent(&cdda);
		}
		return TRUE;
	}

	// ディスクを削除し、一旦初期化する
	for (i=0; i<HDMax; i++) {
		if (scsi.hd[i]) {
			delete scsi.hd[i];
			scsi.hd[i] = NULL;
		}
	}
	if (scsi.mo) {
		delete scsi.mo;
		scsi.mo = NULL;
	}
	if (scsi.cdrom) {
		delete scsi.cdrom;
		scsi.cdrom = NULL;
	}
	scsi.type = 0;
	scsi.scsi_drives = 0;

	// サイズをロード、照合
	if (!fio->Read(&sz, sizeof(sz))) {
		return FALSE;
	}
	if (sz != sizeof(scsi_t)) {
		return FALSE;
	}

	// 実体をロード
	if (!fio->Read(&scsi, (int)sz)) {
		return FALSE;
	}

	// ディスクをクリア(ロード時に上書きされるため)
	for (i=0; i<HDMax; i++) {
		scsi.hd[i] = NULL;
	}
	scsi.mo = NULL;
	scsi.cdrom = NULL;

	// イベントをロード
	if (!event.Load(fio, ver)) {
		return FALSE;
	}
	if (ver >= 0x0203) {
		// version2.03で、CD-DAイベントが追加された
		if (!cdda.Load(fio, ver)) {
			return FALSE;
		}
	}

	// パスをロード
	for (i=0; i<HDMax; i++) {
		if (!scsihd[i].Load(fio, ver)) {
			return FALSE;
		}
	}

	// ディスク再構築
	Construct();

	// ディスクをロード(version2.03で追加)
	if (ver >= 0x0203) {
		for (i=0; i<HDMax; i++) {
			if (scsi.hd[i]) {
				if (!scsi.hd[i]->Load(fio, ver)) {
					return FALSE;
				}
			}
			else {
				// 数あわせのため、ダミーでもよいのでロード(version2.04)
				if (ver >= 0x0204) {
					disk = new SCSIHD(this);
					if (!disk->Load(fio, ver)) {
						delete disk;
						return FALSE;
					}
					delete disk;
				}
			}
		}
		if (scsi.mo) {
			if (!scsi.mo->Load(fio, ver)) {
				return FALSE;
			}
		}
		if (scsi.cdrom) {
			if (!scsi.cdrom->Load(fio, ver)) {
				return FALSE;
			}
		}
	}

	// イベントを動的に追加・削除
	if (scsi.type == 0) {
		// イベント削除
		if (scheduler->HasEvent(&event)) {
			scheduler->DelEvent(&event);
			scheduler->DelEvent(&cdda);
		}
	}
	else {
		// イベント追加
		if (!scheduler->HasEvent(&event)) {
			scheduler->AddEvent(&event);
			scheduler->AddEvent(&cdda);
		}
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	設定適用
//
//---------------------------------------------------------------------------
void FASTCALL SCSI::ApplyCfg(const Config *config)
{
	int i;

	ASSERT(this);
	ASSERT(config);
	LOG0(Log::Normal, "設定適用");

	// SCSIドライブ数
	scsi.scsi_drives = config->scsi_drives;

	// メモリスイッチ自動更新
	scsi.memsw = config->scsi_sramsync;

	// MO優先フラグ
	scsi.mo_first = config->scsi_mofirst;

	// SCSIファイル名
	for (i=0; i<HDMax; i++) {
		scsihd[i].SetPath(config->scsi_file[i]);
	}

	// ディスク再構築
	Construct();
}

#if !defined(NDEBUG)
//---------------------------------------------------------------------------
//
//	診断
//
//---------------------------------------------------------------------------
void FASTCALL SCSI::AssertDiag() const
{
	// 基本クラス
	MemDevice::AssertDiag();

	ASSERT(this);
	ASSERT(GetID() == MAKEID('S', 'C', 'S', 'I'));
	ASSERT(memory);
	ASSERT(memory->GetID() == MAKEID('M', 'E', 'M', ' '));
	ASSERT(sram);
	ASSERT(sram->GetID() == MAKEID('S', 'R', 'A', 'M'));
	ASSERT((scsi.type == 0) || (scsi.type == 1) || (scsi.type == 2));
	ASSERT((scsi.vector == -1) || (scsi.vector == 0x6c) || (scsi.vector == 0xf6));
	ASSERT((scsi.id >= -1) && (scsi.id <= 7));
	ASSERT(scsi.bdid != 0);
	ASSERT(scsi.bdid < 0x100);
	ASSERT(scsi.ints < 0x100);
	ASSERT(scsi.mbc < 0x10);
}
#endif	// NDEBUG

//---------------------------------------------------------------------------
//
//	バイト読み込み
//
//---------------------------------------------------------------------------
DWORD FASTCALL SCSI::ReadByte(DWORD addr)
{
	const BYTE *rom;

	ASSERT(this);
	ASSERT(addr <= 0xffffff);

	// 通常の読み込みの場合
	if (addr >= memdev.first) {
		// SASIのみ or 内蔵の場合、この空間は見えない
		if (scsi.type != 1) {
			// バスエラー
			cpu->BusErr(addr, TRUE);
			return 0xff;
		}
	}

	// アドレスマスク
	addr &= 0x1fff;

	// ROMデータ
	if (addr >= 0x20) {
		// ウェイト
		scheduler->Wait(1);

		// ROMを返す
		rom = memory->GetSCSI();
		return rom[addr];
	}

	// レジスタ
	if ((addr & 1) == 0) {
		// 偶数アドレスはデコードされていない
		return 0xff;
	}
	addr &= 0x1f;
	addr >>= 1;

	// ウェイト
	scheduler->Wait(1);

	// レジスタ別
	switch (addr) {
		// BDID
		case 0:
			return scsi.bdid;

		// SCTL
		case 1:
			return scsi.sctl;

		// SCMD
		case 2:
			return scsi.scmd;

		// リザーブ
		case 3:
			return 0xff;

		// INTS
		case 4:
			return scsi.ints;

		// PSNS
		case 5:
			return GetPSNS();

		// SSTS
		case 6:
			return GetSSTS();

		// SERR
		case 7:
			return GetSERR();

		// PCTL
		case 8:
			return scsi.pctl;

		// MBC
		case 9:
			return scsi.mbc;

		// DREG
		case 10:
			return GetDREG();

		// TEMP
		case 11:
			return scsi.temp;

		// TCH
		case 12:
			return (BYTE)(scsi.tc >> 16);

		// TCM
		case 13:
			return (BYTE)(scsi.tc >> 8);

		// TCL
		case 14:
			return (BYTE)scsi.tc;

		// リザーブ
		case 15:
			return 0xff;
	}

	// 通常、ここには来ない
	LOG1(Log::Warning, "未定義レジスタ読み込み R%d", addr);
	return 0xff;
}

//---------------------------------------------------------------------------
//
//	ワード読み込み
//
//---------------------------------------------------------------------------
DWORD FASTCALL SCSI::ReadWord(DWORD addr)
{
	ASSERT(this);
	ASSERT((addr & 1) == 0);

	return (0xff00 | ReadByte(addr + 1));
}

//---------------------------------------------------------------------------
//
//	バイト書き込み
//
//---------------------------------------------------------------------------
void FASTCALL SCSI::WriteByte(DWORD addr, DWORD data)
{
	ASSERT(this);
	ASSERT(addr <= 0xffffff);
	ASSERT(data < 0x100);

	// 通常の読み込みの場合
	if (addr >= memdev.first) {
		// SASIのみ or 内蔵の場合、この空間は見えない
		if (scsi.type != 1) {
			// バスエラー
			cpu->BusErr(addr, FALSE);
			return;
		}
	}

	// アドレスマスク
	addr &= 0x1fff;

	// ROMデータ
	if (addr >= 0x20) {
		// ROMへの書き込み
		return;
	}

	// レジスタ
	if ((addr & 1) == 0) {
		// 偶数アドレスはデコードされていない
		return;
	}
	addr &= 0x1f;
	addr >>= 1;

	// ウェイト
	scheduler->Wait(1);

	// レジスタ別
	switch (addr) {
		// BDID
		case 0:
			SetBDID(data);
			return;

		// SCTL
		case 1:
			SetSCTL(data);
			return;

		// SCMD
		case 2:
			SetSCMD(data);
			return;

		// リザーブ
		case 3:
			break;

		// INTS
		case 4:
			SetINTS(data);
			return;

		// SDGC
		case 5:
			SetSDGC(data);
			return;

		// SSTS(Read Only)
		case 6:
			// SCSIドライバv1.04の修正パッチで、時間稼ぎのために書き込んでいる様子
			return;

		// SERR(Read Only)
		case 7:
			break;

		// PCTL
		case 8:
			SetPCTL(data);
			return;

		// MBC(Read Only)
		case 9:
			break;

		// DREG
		case 10:
			SetDREG(data);
			return;

		// TEMP
		case 11:
			SetTEMP(data);
			return;

		// TCH
		case 12:
			SetTCH(data);
			return;

		// TCM
		case 13:
			SetTCM(data);
			return;

		// TCL
		case 14:
			SetTCL(data);
			return;

		// リザーブ
		case 15:
			break;
	}

	// 通常、ここには来ない
	LOG2(Log::Warning, "未定義レジスタ書き込み R%d <- $%02X", addr, data);
}

//---------------------------------------------------------------------------
//
//	ワード書き込み
//
//---------------------------------------------------------------------------
void FASTCALL SCSI::WriteWord(DWORD addr, DWORD data)
{
	ASSERT(this);
	ASSERT((addr & 1) == 0);
	ASSERT(data < 0x10000);

	WriteByte(addr + 1, (BYTE)data);
}

//---------------------------------------------------------------------------
//
//	読み込みのみ
//
//---------------------------------------------------------------------------
DWORD FASTCALL SCSI::ReadOnly(DWORD addr) const
{
	const BYTE *rom;

	ASSERT(this);

	// 通常の読み込みの場合
	if (addr >= memdev.first) {
		// SASIのみ or 内蔵の場合、この空間は見えない
		if (scsi.type != 1) {
			return 0xff;
		}
	}

	// アドレスマスク
	addr &= 0x1fff;

	// ROMデータ
	if (addr >= 0x20) {
		// ROMを返す
		rom = memory->GetSCSI();
		return rom[addr];
	}

	// レジスタ
	if ((addr & 1) == 0) {
		// 偶数アドレスはデコードされていない
		return 0xff;
	}
	addr &= 0x1f;
	addr >>= 1;

	// レジスタ別
	switch (addr) {
		// BDID
		case 0:
			return scsi.bdid;

		// SCTL
		case 1:
			return scsi.sctl;

		// SCMD
		case 2:
			return scsi.scmd;

		// リザーブ
		case 3:
			break;

		// INTS
		case 4:
			return scsi.ints;

		// PSNS
		case 5:
			return GetPSNS();

		// SSTS
		case 6:
			return GetSSTS();

		// SERR
		case 7:
			return GetSERR();

		// PCTL
		case 8:
			return scsi.pctl;

		// MBC
		case 9:
			return scsi.mbc;

		// DREG
		case 10:
			return scsi.buffer[scsi.offset];

		// TEMP
		case 11:
			return scsi.temp;

		// TCH
		case 12:
			return (BYTE)(scsi.tc >> 16);

		// TCM
		case 13:
			return (BYTE)(scsi.tc >> 8);

		// TCL
		case 14:
			return (BYTE)scsi.tc;

		// リザーブ
		case 15:
			break;
	}

	return 0xff;
}

//---------------------------------------------------------------------------
//
//	内部データ取得
//
//---------------------------------------------------------------------------
void FASTCALL SCSI::GetSCSI(scsi_t *buffer) const
{
	ASSERT(this);
	ASSERT(buffer);
	ASSERT_DIAG();

	// 内部ワークをコピー
	*buffer = scsi;
}

//---------------------------------------------------------------------------
//
//	イベントコールバック
//
//---------------------------------------------------------------------------
BOOL FASTCALL SCSI::Callback(Event* ev)
{
	BOOL success;

	ASSERT(this);
	ASSERT(ev);
	ASSERT_DIAG();

	// CD-DAイベント専用処理
	if (ev->GetUser() == 1) {
		if (scsi.cdrom) {
			// CD-ROMへ通知
			if (scsi.cdrom->NextFrame()) {
				// 通常速度
				ev->SetTime(26666);
			}
			else {
				// 75回に1回だけ、補正する
				ev->SetTime(26716);
			}
		}

		// 継続
		return TRUE;
	}

	// セレクションフェーズのみ有効
	ASSERT(ev->GetUser() == 0);
	if (scsi.phase != selection) {
		// 単発
		return FALSE;
	}

	// 成功フラグ作成
	success = FALSE;
	if (scsi.id != -1) {
		// IDがイニシエータとターゲット、両方指定されていて
		if (scsi.disk[scsi.id]) {
			// 有効なディスクがマウントされている
			success = TRUE;
		}
	}

	// 成功フラグにより分ける
	if (success) {
#if defined(SCSI_LOG)
		LOG1(Log::Normal, "セレクション成功 ID=%d", scsi.id);
#endif	// SCSI_LOG

		// TCをデクリメント
		scsi.tc &= 0x00ffff00;
		scsi.tc -= 0x2800;

		// コンプリート(セレクション成功)
		Interrupt(4, TRUE);

		// ここでBSY=1
		scsi.bsy = TRUE;

		// ATN=1ならメッセージアウトフェーズ、そうでなければコマンドフェーズ
		if (scsi.atn) {
			MsgOut();
		}
		else {
			Command();
		}
	}
	else {
#if defined(SCSI_LOG)
		LOG1(Log::Normal, "セレクション失敗 TEMP=$%02X", scsi.temp);
#endif	// SCSI_LOG

		// TC=0とする(タイムアウト)
		scsi.tc = 0;

		// タイムアウト(セレクション失敗)
		Interrupt(2, TRUE);

		// BSY=FALSEならバスフリー
		if (!scsi.bsy) {
			BusFree();
		}
	}

	// 単発
	return FALSE;
}

//---------------------------------------------------------------------------
//
//	レジスタリセット
//
//---------------------------------------------------------------------------
void FASTCALL SCSI::ResetReg()
{
	ASSERT(this);

#if defined(SCSI_LOG)
	LOG0(Log::Normal, "レジスタリセット");
#endif	// SCSI_LOG

	// カレントID
	scsi.id = -1;

	// 割り込み
	scsi.vector = -1;

	// レジスタ
	scsi.bdid = 0x80;
	scsi.sctl = 0x80;
	scsi.scmd = 0;
	scsi.ints = 0;
	scsi.sdgc = 0;
	scsi.pctl = 0;
	scsi.mbc = 0;
	scsi.temp = 0;
	scsi.tc = 0;

	// 転送コマンド停止
	scsi.trans = FALSE;

	// フェーズ(SPCはバスフリーのつもり)
	scsi.phase = busfree;

	// 割り込みチェック
	IntCheck();
}

//---------------------------------------------------------------------------
//
//	転送リセット
//
//---------------------------------------------------------------------------
void FASTCALL SCSI::ResetCtrl()
{
	ASSERT(this);

#if defined(SCSI_LOG)
	LOG0(Log::Normal, "転送リセット");
#endif	// SCSI_LOG

	// 転送コマンド停止
	scsi.trans = FALSE;
}

//---------------------------------------------------------------------------
//
//	バスリセット
//
//---------------------------------------------------------------------------
void FASTCALL SCSI::ResetBus(BOOL reset)
{
	ASSERT(this);
	ASSERT_DIAG();

#if defined(SCSI_LOG)
	if (reset) {
		LOG0(Log::Normal, "SCSIバス リセット");
	}
	else {
		LOG0(Log::Normal, "SCSIバス リセット解除");
	}
#endif	// SCSI_LOG

	// 記憶
	scsi.rst = reset;

	// 信号線(バスフリーと同じ)
	scsi.bsy = FALSE;
	scsi.sel = FALSE;
	scsi.atn = FALSE;
	scsi.msg = FALSE;
	scsi.cd = FALSE;
	scsi.io = FALSE;
	scsi.req = FALSE;
	scsi.ack = FALSE;

	// 転送コマンド停止
	scsi.trans = FALSE;

	// フェーズ(SPCはバスフリーのつもり)
	scsi.phase = busfree;

	// バスリセット割り込み発生・解除
	Interrupt(0, reset);
}

//---------------------------------------------------------------------------
//
//	BDID設定
//
//---------------------------------------------------------------------------
void FASTCALL SCSI::SetBDID(DWORD data)
{
	DWORD bdid;

	ASSERT(this);
	ASSERT(data < 0x100);

	// 3bitのみ有効
	data &= 0x07;

#if defined(SCSI_LOG)
	LOG1(Log::Normal, "ボードID設定 ID%d", data);
#endif	// SCSI_LOG

	// 数値からビットへ変換
	bdid = (DWORD)(1 << data);

	// 異なるときは、ディスク再構築が必要
	if (scsi.bdid != bdid) {
		scsi.bdid = bdid;
		Construct();
	}
}

//---------------------------------------------------------------------------
//
//	SCTL設定
//
//---------------------------------------------------------------------------
void FASTCALL SCSI::SetSCTL(DWORD data)
{
	ASSERT(this);
	ASSERT(data < 0x100);

#if defined(SCSI_LOG)
	LOG1(Log::Normal, "SCTL設定 $%02X", data);
#endif	// SCSI_LOG

	scsi.sctl = data;

	// bit7:Reset & Disable
	if (scsi.sctl & 0x80) {
		// レジスタリセット
		ResetReg();
	}

	// bit6:Reset Control
	if (scsi.sctl & 0x40) {
		// 転送リセット
		ResetCtrl();
	}

	// 割り込みチェック
	IntCheck();
}

//---------------------------------------------------------------------------
//
//	SCMD設定
//
//---------------------------------------------------------------------------
void FASTCALL SCSI::SetSCMD(DWORD data)
{
	ASSERT(this);
	ASSERT(data < 0x100);
	ASSERT_DIAG();

	// bit4: RST Out
	if (data & 0x10) {
		if ((scsi.scmd & 0x10) == 0) {
			// RSTが0→1
			if ((scsi.sctl & 0x80) == 0) {
				// SPCがリセット中でない
				ResetBus(TRUE);
			}
		}
	}
	else {
		if (scsi.scmd & 0x10) {
			// RSTが1→0
			if ((scsi.sctl & 0x80) == 0) {
				// SPCがリセット中でない
				ResetBus(FALSE);
			}
		}
	}

	// コマンド記憶
	scsi.scmd = data;

	// bit7-5:Command
	switch (scsi.scmd >> 5) {
		// Bus Release
		case 0:
			BusRelease();
			break;

		// Select
		case 1:
			Select();
			break;

		// Reset ATN
		case 2:
			ResetATN();
			break;

		// Set ATN
		case 3:
			SetATN();
			break;

		// Transfer
		case 4:
			Transfer();
			break;

		// Transfer Pause
		case 5:
			TransPause();
			break;

		// Reset ACK/REQ
		case 6:
			ResetACKREQ();
			break;

		// Set ACK/REQ
		case 7:
			SetACKREQ();
			break;

		// その他(ありえない)
		default:
			ASSERT(FALSE);
			break;
	}
}

//---------------------------------------------------------------------------
//
//	INTS設定
//
//---------------------------------------------------------------------------
void FASTCALL SCSI::SetINTS(DWORD data)
{
	ASSERT(this);
	ASSERT(data < 0x100);
	ASSERT_DIAG();

#if defined(SCSI_LOG)
	LOG1(Log::Normal, "割り込みクリア $%02X", data);
#endif	// SCSI_LOG

	// ビットを立てた割り込みを取り下げる
	scsi.ints &= ~data;

	// 割り込みチェック
	IntCheck();
}

//---------------------------------------------------------------------------
//
//	PSNS取得
//
//---------------------------------------------------------------------------
DWORD FASTCALL SCSI::GetPSNS() const
{
	DWORD data;

	ASSERT(this);
	ASSERT_DIAG();

	// データ初期化
	data = 0;

	// bit7:REQ
	if (scsi.req) {
		data |= 0x80;
	}

	// bit6:ACK
	if (scsi.ack) {
		data |= 0x40;
	}

	// bit5:ATN
	if (scsi.atn) {
		data |= 0x20;
	}

	// bit4:SEL
	if (scsi.sel) {
		data |= 0x10;
	}

	// bit3:BSY
	if (scsi.bsy) {
		data |= 0x08;
	}

	// bit2:MSG
	if (scsi.msg) {
		data |= 0x04;
	}

	// bit1:C/D
	if (scsi.cd) {
		data |= 0x02;
	}

	// bit0:I/O
	if (scsi.io) {
		data |= 0x01;
	}

	return data;
}

//---------------------------------------------------------------------------
//
//	SDGC設定
//
//---------------------------------------------------------------------------
void FASTCALL SCSI::SetSDGC(DWORD data)
{
	ASSERT(this);
	ASSERT(data < 0x100);

#if defined(SCSI_LOG)
	LOG1(Log::Normal, "SDGC設定 $%02X", data);
#endif	// SCSI_LOG

	scsi.sdgc = data;
}

//---------------------------------------------------------------------------
//
//	SSTS取得
//
//---------------------------------------------------------------------------
DWORD FASTCALL SCSI::GetSSTS() const
{
	DWORD data;

	ASSERT(this);
	ASSERT_DIAG();

	// データ初期化
	data = 0;

	// bit6-7:Connected
	if (scsi.phase != busfree) {
		// バスフリー以外は、イニシエータとして結合中
		data = 0x80;
	}

	// bit5:SPC BUSY
	if (scsi.bsy) {
		data |= 0x20;
	}

	// bit4:Transfer Progress
	if (scsi.trans) {
		data |= 0x10;
	}

	// bit3:Reset
	if (scsi.rst) {
		data |= 0x08;
	}

	// bit2:TC=0
	if (scsi.tc == 0) {
		data |= 0x04;
	}

	// bit0-1:FIFO数 (00:1～7バイト, 01:Empty, 10:8バイト, 11:未使用)
	if (scsi.trans) {
		// 転送中で
		if (scsi.io) {
			// Read時に限り、FIFOにデータが溜まる可能性がある
			if ((scsi.length > 0) && (scsi.tc > 0)) {
				if ((scsi.length >= 8) && (scsi.tc >= 8)) {
					// 8バイト以上(TS-6BS1mkIII)
					data |= 0x02;
					return data;
				}
				else {
					// 7バイト以下
					return data;
				}
			}
		}
	}

	// FIFOにデータはない
	data |= 0x01;
	return data;
}

//---------------------------------------------------------------------------
//
//	SERR取得
//
//---------------------------------------------------------------------------
DWORD FASTCALL SCSI::GetSERR() const
{
	ASSERT(this);
	ASSERT_DIAG();

	// プログラム転送時、リクエスト信号を出すなら
	if (scsi.sdgc & 0x20) {
		if (scsi.trans) {
			if (scsi.tc != 0) {
				// Xfer Outを示す
				return 0x20;
			}
		}
	}

	// それ以外は0
	return 0x00;
}

//---------------------------------------------------------------------------
//
//	PCTL設定
//
//---------------------------------------------------------------------------
void FASTCALL SCSI::SetPCTL(DWORD data)
{
	ASSERT(this);
	ASSERT(data < 0x100);

	scsi.pctl = data;

}

//---------------------------------------------------------------------------
//
//	DREG取得
//
//---------------------------------------------------------------------------
DWORD FASTCALL SCSI::GetDREG()
{
	DWORD data;

	ASSERT(this);
	ASSERT_DIAG();

	// 転送中でなければ、FFを返す
	if (!scsi.trans) {
		return 0xff;
	}

	// リクエストが上がっていなければ、FFを返す
	if (!scsi.req) {
		return 0xff;
	}

	// TCが0なら、FFを返す
	if (scsi.tc == 0) {
		return 0xff;
	}

	// REQ-ACKハンドシェークを自動で行う
	Xfer(&data);
	XferNext();

	// MBC
	scsi.mbc = (scsi.mbc - 1) & 0x0f;

	// TC
	scsi.tc--;
	if (scsi.tc == 0) {
		// 転送完了(scsi.length != 0なら、SCSIバスはロック)
		TransComplete();
		return data;
	}

	// ステータスフェーズになっていれば、転送完了orエラー
	if (scsi.phase == status) {
		// 転送完了
		TransComplete();
		return data;
	}

	// 読み込み継続
	return data;
}

//---------------------------------------------------------------------------
//
//	DREG設定
//
//---------------------------------------------------------------------------
void FASTCALL SCSI::SetDREG(DWORD data)
{
	ASSERT(this);
	ASSERT(data < 0x100);
	ASSERT_DIAG();

	// 転送中でなければ、何もしない
	if (!scsi.trans) {
		return;
	}

	// リクエストが上がっていなければ、何もしない
	if (!scsi.req) {
		return;
	}

	// TCが0なら、何もしない
	if (scsi.tc == 0) {
		return;
	}

	// REQ-ACKハンドシェークを自動で行う
	Xfer(&data);
	XferNext();

	// MBC
	scsi.mbc = (scsi.mbc - 1) & 0x0f;

	// TC
	scsi.tc--;
	if (scsi.tc == 0) {
		// 転送完了(scsi.length != 0なら、SCSIバスはロック)
		TransComplete();
		return;
	}

	// ステータスフェーズになっていれば、転送完了orエラー
	if (scsi.phase == status) {
		// 転送完了
		TransComplete();
		return;
	}
}

//---------------------------------------------------------------------------
//
//	TEMP設定
//
//---------------------------------------------------------------------------
void FASTCALL SCSI::SetTEMP(DWORD data)
{
	ASSERT(this);
	ASSERT(data < 0x100);

	scsi.temp = data;

	// セレクションフェーズの場合、0x00のセットによりSEL=0となる
	if (scsi.phase == selection) {
		if (data == 0x00) {
			// SEL=0, BSY=0
			scsi.sel = FALSE;
			scsi.bsy = FALSE;
		}
	}
}

//---------------------------------------------------------------------------
//
//	TCH設定
//
//---------------------------------------------------------------------------
void FASTCALL SCSI::SetTCH(DWORD data)
{
	ASSERT(this);
	ASSERT(data < 0x100);
	ASSERT_DIAG();

	// b23-b16に設定
	scsi.tc &= 0x0000ffff;
	data <<= 16;
	scsi.tc |= data;

#if defined(SCSI_LOG)
	LOG1(Log::Normal, "TCH設定 TC=$%06X", scsi.tc);
#endif	// SCSI_LOG
}

//---------------------------------------------------------------------------
//
//	TCM設定
//
//---------------------------------------------------------------------------
void FASTCALL SCSI::SetTCM(DWORD data)
{
	ASSERT(this);
	ASSERT(data < 0x100);

	// b15-b8に設定
	scsi.tc &= 0x00ff00ff;
	data <<= 8;
	scsi.tc |= data;

#if defined(SCSI_LOG)
	LOG1(Log::Normal, "TCM設定 TC=$%06X", scsi.tc);
#endif	// SCSI_LOG
}

//---------------------------------------------------------------------------
//
//	TCL設定
//
//---------------------------------------------------------------------------
void FASTCALL SCSI::SetTCL(DWORD data)
{
	ASSERT(this);
	ASSERT(data < 0x100);

	// b7-b0に設定
	scsi.tc &= 0x00ffff00;
	scsi.tc |= data;

#if defined(SCSI_LOG)
	LOG1(Log::Normal, "TCL設定 TC=$%06X", scsi.tc);
#endif	// SCSI_LOG

	// SCMDが$20なら、セレクション継続
	if ((scsi.scmd & 0xe0) == 0x20) {
		SelectTime();
	}
}

//---------------------------------------------------------------------------
//
//	バスリリース
//
//---------------------------------------------------------------------------
void FASTCALL SCSI::BusRelease()
{
	ASSERT(this);
	ASSERT_DIAG();

#if defined(SCSI_LOG)
	LOG0(Log::Normal, "Bus Releaseコマンド");
#endif	// SCSI_LOG

	// バスフリー
	BusFree();
}

//---------------------------------------------------------------------------
//
//	セレクト
//
//---------------------------------------------------------------------------
void FASTCALL SCSI::Select()
{
	ASSERT(this);
	ASSERT_DIAG();

#if defined(SCSI_LOG)
	LOG0(Log::Normal, "Selectコマンド");
#endif	// SCSI_LOG

	// SPCリセット中、SCSIバスリセット中は、処理されない
	if (scsi.sctl & 0x80) {
		return;
	}
	if (scsi.rst) {
		return;
	}

	// SCTLのbit4が1なら、アービトレーションフェーズが先に実行される
	if (scsi.sctl & 0x10) {
		Arbitration();
	}

	// PCTLのbit0が1なら、リセレクション(ターゲットのみ許される)
	if (scsi.pctl & 1) {
		LOG0(Log::Warning, "リセレクションフェーズ");
		BusFree();
		return;
	}

	// セレクションフェーズ
	Selection();
}

//---------------------------------------------------------------------------
//
//	ATNリセット
//
//---------------------------------------------------------------------------
void FASTCALL SCSI::ResetATN()
{
	ASSERT(this);
	ASSERT_DIAG();

#if defined(SCSI_LOG)
	LOG0(Log::Normal, "Reset ATNコマンド");
#endif	// SCSI_LOG

	// イニシエータが操作する信号線
	scsi.atn = FALSE;
}

//---------------------------------------------------------------------------
//
//	ATNセット
//
//---------------------------------------------------------------------------
void FASTCALL SCSI::SetATN()
{
	ASSERT(this);
	ASSERT_DIAG();

#if defined(SCSI_LOG)
	LOG0(Log::Normal, "Set ATNコマンド");
#endif	// SCSI_LOG

	// イニシエータが操作する信号線
	scsi.atn = TRUE;
}

//---------------------------------------------------------------------------
//
//	転送
//
//---------------------------------------------------------------------------
void FASTCALL SCSI::Transfer()
{
	ASSERT(this);
	ASSERT_DIAG();
	ASSERT(scsi.req);

#if defined(SCSI_LOG)
	LOG0(Log::Normal, "Transferコマンド");
#endif	// SCSI_LOG

	// デバイス側の準備ができているか
	if (scsi.length <= 0) {
		// Service Required(フェーズが合わない)
		Interrupt(3, TRUE);
		return;
	}

	// フェーズが一致しているか

	// コントローラ側の準備ができているか
	if (scsi.tc == 0) {
		// コンプリート(転送終了)
		Interrupt(4, TRUE);
	}

	// 転送コマンド開始
	scsi.trans = TRUE;
}

//---------------------------------------------------------------------------
//
//	転送中断
//
//---------------------------------------------------------------------------
void FASTCALL SCSI::TransPause()
{
	ASSERT(this);
	ASSERT_DIAG();

	// ターゲットのみ発行できる
	LOG0(Log::Warning, "Transfer Pauseコマンド");
}

//---------------------------------------------------------------------------
//
//	転送完了
//
//---------------------------------------------------------------------------
void FASTCALL SCSI::TransComplete()
{
	ASSERT(this);
	ASSERT(!scsi.ack);
	ASSERT(scsi.trans);
	ASSERT_DIAG();

	// 転送完了
	scsi.trans = FALSE;

	// 転送完了割り込み
	Interrupt(4, TRUE);
}

//---------------------------------------------------------------------------
//
//	ACKREQリセット
//
//---------------------------------------------------------------------------
void FASTCALL SCSI::ResetACKREQ()
{
	ASSERT(this);
	ASSERT_DIAG();

	// ACKが上がっている場合のみ意味を持つ
	if (!scsi.ack) {
		return;
	}

	// ACKが上がるフェーズは限られる
	ASSERT((scsi.phase >= command) && (scsi.phase != execute));

	// データ転送を次に進める
	XferNext();
}

//---------------------------------------------------------------------------
//
//	ACK/REQセット
//
//---------------------------------------------------------------------------
void FASTCALL SCSI::SetACKREQ()
{
	ASSERT(this);
	ASSERT_DIAG();

	// REQが上がっている場合のみ意味を持つ
	if (!scsi.req) {
		return;
	}

	// REQが上がるフェーズは限られる
	ASSERT((scsi.phase >= command) && (scsi.phase != execute));

	// TEMPレジスタ-SCSIデータバス間のデータ転送を行う
	Xfer(&scsi.temp);
}

//---------------------------------------------------------------------------
//
//	データ転送
//
//---------------------------------------------------------------------------
void FASTCALL SCSI::Xfer(DWORD *reg)
{
	ASSERT(this);
	ASSERT(reg);
	ASSERT_DIAG();

	// REQが上がっていること
	ASSERT(scsi.req);
	ASSERT(!scsi.ack);
	ASSERT((scsi.phase >= command) && (scsi.phase != execute));

	// イニシエータが操作する信号線
	scsi.ack = TRUE;

	// SCSIデータバスにデータを乗せる
	switch (scsi.phase) {
		// コマンドフェーズ
		case command:
#if defined(SCSI_LOG)
			LOG1(Log::Normal, "コマンドフェーズ $%02X", *reg);
#endif	// SCSI_LOG
			// 最初のデータ(オフセット0)によりレングスを再設定
			scsi.cmd[scsi.offset] = *reg;
			if (scsi.offset == 0) {
				if (scsi.temp >= 0x20) {
					// 10バイトCDB
					scsi.length = 10;
				}
			}
			break;

		// メッセージインフェーズ
		case msgin:
			*reg = scsi.message;
#if defined(SCSI_LOG)
			LOG1(Log::Normal, "メッセージインフェーズ $%02X", *reg);
#endif	// SCSI_LOG
			break;

		// メッセージアウトフェーズ
		case msgout:
			scsi.message = *reg;
#if defined(SCSI_LOG)
			LOG1(Log::Normal, "メッセージアウトフェーズ $%02X", *reg);
#endif	// SCSI_LOG
			break;

		// データインフェーズ
		case datain:
			*reg = (DWORD)scsi.buffer[scsi.offset];
			break;

		// データアウトフェーズ
		case dataout:
			scsi.buffer[scsi.offset] = (BYTE)*reg;
			break;

		// ステータスフェーズ
		case status:
			*reg = scsi.status;
#if defined(SCSI_LOG)
			LOG1(Log::Normal, "ステータスフェーズ $%02X", *reg);
#endif	// SCSI_LOG
			break;

		// その他(ありえない)
		default:
			ASSERT(FALSE);
			break;
	}

	// ターゲットが操作する信号線
	scsi.req = FALSE;
}

//---------------------------------------------------------------------------
//
//	データ転送継続
//
//---------------------------------------------------------------------------
void FASTCALL SCSI::XferNext()
{
	BOOL result;

	ASSERT(this);
	ASSERT_DIAG();

	// ACKが上がっていること
	ASSERT(!scsi.req);
	ASSERT(scsi.ack);
	ASSERT((scsi.phase >= command) && (scsi.phase != execute));

	// オフセットとレングス
	ASSERT(scsi.length >= 1);
	scsi.offset++;
	scsi.length--;

	// イニシエータが操作する信号線
	scsi.ack = FALSE;

	// レングス!=0なら、再びreqをセット
	if (scsi.length != 0) {
		scsi.req = TRUE;
		return;
	}

	// ブロック減算、リザルト初期化
	scsi.blocks--;
	result = TRUE;

	// データ受理後の処理(フェーズ別)
	switch (scsi.phase) {
		// データインフェーズ
		case datain:
			if (scsi.blocks != 0) {
				// 次のバッファを設定(offset, lengthをセットすること)
				result = XferIn();
			}
			break;

		// データアウトフェーズ
		case dataout:
			if (scsi.blocks == 0) {
				// このバッファで終了
				result = XferOut(FALSE);
			}
			else {
				// 次のバッファに続く(offset, lengthをセットすること)
				result = XferOut(TRUE);
			}
			break;

		// メッセージアウトフェーズ
		case msgout:
			if (!XferMsg(scsi.message)) {
				// メッセージアウトに失敗したら、即座にバスフリー
				BusFree();
				return;
			}
			// メッセージインに備え、メッセージデータをクリアしておく
			scsi.message = 0x00;
			break;
	}

	// リザルトFALSEなら、ステータスフェーズへ移行
	if (!result) {
		Error();
		return;
	}

	// ブロック!=0なら、再びreqをセット
	if (scsi.blocks != 0){
		ASSERT(scsi.length > 0);
		ASSERT(scsi.offset == 0);
		scsi.req = TRUE;
		return;
	}

	// 次フェーズに移動
	switch (scsi.phase) {
		// コマンドフェーズ
		case command:
			// 実行フェーズ
			Execute();
			break;

		// メッセージインフェーズ
		case msgin:
			// バスフリーフェーズ
			BusFree();
			break;

		// メッセージアウトフェーズ
		case msgout:
			// コマンドフェーズ
			Command();
			break;

		// データインフェーズ
		case datain:
			// ステータスフェーズ
			Status();
			break;

		// データアウトフェーズ
		case dataout:
			// ステータスフェーズ
			Status();
			break;

		// ステータスフェーズ
		case status:
			// メッセージインフェーズ
			MsgIn();
			break;

		// その他(ありえない)
		default:
			ASSERT(FALSE);
			break;
	}
}

//---------------------------------------------------------------------------
//
//	データ転送IN
//	※offset, lengthを再設定すること
//
//---------------------------------------------------------------------------
BOOL FASTCALL SCSI::XferIn()
{
	ASSERT(this);
	ASSERT(scsi.phase == datain);
	ASSERT(scsi.disk[scsi.id]);
	ASSERT_DIAG();

	// ステートロードによりディスクが無くなった場合
	if (!scsi.disk[scsi.id]) {
		// データイン中止
		return FALSE;
	}

	// READ系コマンドに限る
	switch (scsi.cmd[0]) {
		// READ(6)
		case 0x08:
		// READ(10)
		case 0x28:
			// ディスクから読み取りを行う
			scsi.length = scsi.disk[scsi.id]->Read(scsi.buffer, scsi.next);
			scsi.next++;

			// エラーなら、ステータスフェーズへ
			if (scsi.length <= 0) {
				// データイン中止
				return FALSE;
			}

			// 正常なら、ワーク設定
			scsi.offset = 0;
			break;

		// その他(ありえない)
		default:
			ASSERT(FALSE);
			return FALSE;
	}

	// バッファ設定に成功した
	return TRUE;
}

//---------------------------------------------------------------------------
//
//	データ転送OUT
//	※cont=TRUEの場合、offset, lengthを再設定すること
//
//---------------------------------------------------------------------------
BOOL FASTCALL SCSI::XferOut(BOOL cont)
{
	ASSERT(this);
	ASSERT(scsi.phase == dataout);
	ASSERT(scsi.disk[scsi.id]);
	ASSERT_DIAG();

	// ステートロードによりディスクが無くなった場合
	if (!scsi.disk[scsi.id]) {
		// データアウト中止
		return FALSE;
	}

	// MODE SELECTまたは、WRITE系
	switch (scsi.cmd[0]) {
		// MODE SELECT
		case 0x15:
			if (!scsi.disk[scsi.id]->ModeSelect(scsi.buffer, scsi.offset)) {
				// MODE SELECTに失敗
				return FALSE;
			}
			break;

		// WRITE(6)
		case 0x0a:
		// WRITE(10)
		case 0x2a:
			// 書き込みを行う
			if (!scsi.disk[scsi.id]->Write(scsi.buffer, scsi.next - 1)) {
				// 書き込み失敗
				return FALSE;
			}

			// 次のブロックが必要ないならここまで
			scsi.next++;
			if (!cont) {
				break;
			}

			// 次のブロックをチェック
			scsi.length = scsi.disk[scsi.id]->WriteCheck(scsi.next);
			if (scsi.length <= 0) {
				// 書き込みできない
				return FALSE;
			}

			// 正常なら、ワーク設定
			scsi.offset = 0;
			break;
	}

	// バッファ保存に成功した
	return TRUE;
}

//---------------------------------------------------------------------------
//
//	データ転送MSG
//
//---------------------------------------------------------------------------
BOOL FASTCALL SCSI::XferMsg(DWORD /*msg*/)
{
	ASSERT(this);
	ASSERT(scsi.phase == msgout);
	ASSERT_DIAG();

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	バスフリーフェーズ
//
//---------------------------------------------------------------------------
void FASTCALL SCSI::BusFree()
{
	ASSERT(this);
	ASSERT_DIAG();

#if defined(SCSI_LOG)
	LOG0(Log::Normal, "バスフリーフェーズ");
#endif	// SCSI_LOG

	// フェーズ設定
	scsi.phase = busfree;

	// 信号線
	scsi.bsy = FALSE;
	scsi.sel = FALSE;
	scsi.atn = FALSE;
	scsi.msg = FALSE;
	scsi.cd = FALSE;
	scsi.io = FALSE;
	scsi.req = FALSE;
	scsi.ack = FALSE;

	// ステータスとメッセージを初期化(GOOD)
	scsi.status = 0x00;
	scsi.message = 0x00;

	// イベント停止
	event.SetTime(0);

	// PCTLのBusfree INT Enableをチェック
	if (scsi.pctl & 0x8000) {
		// Disconnect割り込み
		Interrupt(5, TRUE);
	}
	else {
		// 割り込みオフ
		Interrupt(5, FALSE);
	}
}

//---------------------------------------------------------------------------
//
//	アービトレーションフェーズ
//
//---------------------------------------------------------------------------
void FASTCALL SCSI::Arbitration()
{
	ASSERT(this);
	ASSERT_DIAG();

#if defined(SCSI_LOG)
	LOG0(Log::Normal, "アービトレーションフェーズ");
#endif	// SCSI_LOG

	// フェーズ設定
	scsi.phase = arbitration;

	// 信号線
	scsi.bsy = TRUE;
	scsi.sel = TRUE;
}

//---------------------------------------------------------------------------
//
//	セレクションフェーズ
//
//---------------------------------------------------------------------------
void FASTCALL SCSI::Selection()
{
	int i;
	DWORD temp;

	ASSERT(this);
	ASSERT_DIAG();

	// フェーズ設定
	scsi.phase = selection;

	// ターゲットID初期化
	scsi.id = -1;

	// TEMPレジスタにはイニシエータとターゲット、両方のビットが必要
	temp = scsi.temp;
	if ((temp & scsi.bdid) == scsi.bdid) {
		// イニシエータのビットを削除
		temp &= ~(scsi.bdid);

		// ターゲットを探す
		for (i=0; i<8; i++) {
			if (temp == (DWORD)(1 << i)) {
				scsi.id = i;
				break;
			}
		}
	}

#if defined(SCSI_LOG)
	if (scsi.id != -1) {
		if (scsi.disk[scsi.id]) {
			LOG1(Log::Normal, "セレクションフェーズ ID=%d (デバイスあり)", scsi.id);
		}
		else {
			LOG1(Log::Normal, "セレクションフェーズ ID=%d (未接続)", scsi.id);
		}
	}
	else {
		LOG0(Log::Normal, "セレクションフェーズ (無効)");
	}
#endif	// SCSI_LOG

	// セレクションタイム決定
	SelectTime();
}

//---------------------------------------------------------------------------
//
//	セレクションフェーズ 時間設定
//
//---------------------------------------------------------------------------
void FASTCALL SCSI::SelectTime()
{
	DWORD tc;

	ASSERT(this);
	ASSERT((scsi.scmd & 0xe0) == 0x20);
	ASSERT_DIAG();

	tc = scsi.tc;
	if (tc == 0) {
		LOG0(Log::Warning, "セレクションタイムアウトが無限大");
		tc = 0x1000000;
	}
	tc &= 0x00ffff00;
	tc += 15;

	// さらにTCLを考慮する
	tc += (((scsi.tc & 0xff) + 7) >> 1);

	// 400ns単位から、500ns単位(hus)へ変換
	tc = (tc * 4 / 5);

	// フェーズ
	scsi.phase = selection;

	// 信号線
	scsi.sel = TRUE;

	// イベントタイム設定
	event.SetDesc("Selection");
	if (scsi.id != -1) {
		if (scsi.disk[scsi.id]) {
			// 指定IDが正常の場合、16usでセレクションを成功させる
			event.SetTime(32);
			return;
		}
	}

	// タイムアウトの場合、TC通りに設定
	event.SetTime(tc);
}

//---------------------------------------------------------------------------
//
//	コマンドフェーズ
//
//---------------------------------------------------------------------------
void FASTCALL SCSI::Command()
{
	ASSERT(this);
	ASSERT((scsi.id >= 0) && (scsi.id <= 7));
	ASSERT(scsi.disk[scsi.id]);
	ASSERT_DIAG();

#if defined(SCSI_LOG)
	LOG0(Log::Normal, "コマンドフェーズ");
#endif	// SCSI_LOG

	// ステートロードによりディスクが無くなった場合
	if (!scsi.disk[scsi.id]) {
		// コマンドフェーズ中止
		BusFree();
		return;
	}

	// フェーズ設定
	scsi.phase = command;

	// イニシエータが操作する信号線
	scsi.atn = FALSE;

	// ターゲットが操作する信号線
	scsi.msg = FALSE;
	scsi.cd = TRUE;
	scsi.io = FALSE;

	// データ転送は6バイトx1ブロック
	scsi.offset = 0;
	scsi.length = 6;
	scsi.blocks = 1;

	// コマンドを要求
	scsi.req = TRUE;
}

//---------------------------------------------------------------------------
//
//	実行フェーズ
//
//---------------------------------------------------------------------------
void FASTCALL SCSI::Execute()
{
	ASSERT(this);
	ASSERT((scsi.id >= 0) && (scsi.id <= 7));
	ASSERT(scsi.disk[scsi.id]);
	ASSERT_DIAG();

#if defined(SCSI_LOG)
	LOG1(Log::Normal, "実行フェーズ コマンド$%02X", scsi.cmd[0]);
#endif	// SCSI_LOG

	// ステートロードによりディスクが無くなった場合
	if (!scsi.disk[scsi.id]) {
		// コマンドフェーズ中止
		Error();
		return;
	}

	// フェーズ設定
	scsi.phase = execute;

	// データ転送のための初期化
	scsi.offset = 0;
	scsi.blocks = 1;

	// コマンド別処理
	switch (scsi.cmd[0]) {
		// TEST UNIT READY
		case 0x00:
			TestUnitReady();
			return;

		// REZERO
		case 0x01:
			Rezero();
			return;

		// REQUEST SENSE
		case 0x03:
			RequestSense();
			return;

		// FORMAT UNIT
		case 0x04:
			Format();
			return;

		// REASSIGN BLOCKS
		case 0x07:
			Reassign();
			return;

		// READ(6)
		case 0x08:
			Read6();
			return;

		// WRITE(6)
		case 0x0a:
			Write6();
			return;

		// SEEK(6)
		case 0x0b:
			Seek6();
			return;

		// INQUIRY
		case 0x12:
			Inquiry();
			return;

		// MODE SELECT
		case 0x15:
			ModeSelect();
			return;

		// MDOE SENSE
		case 0x1a:
			ModeSense();
			return;

		// START STOP UNIT
		case 0x1b:
			StartStop();
			return;

		// SEND DIAGNOSTIC
		case 0x1d:
			SendDiag();
			return;

		// PREVENT/ALLOW MEDIUM REMOVAL
		case 0x1e:
			Removal();
			return;

		// READ CAPACITY
		case 0x25:
			ReadCapacity();
			return;

		// READ(10)
		case 0x28:
			Read10();
			return;

		// WRITE(10)
		case 0x2a:
			Write10();
			return;

		// SEEK(10)
		case 0x2b:
			Seek10();
			return;

		// WRITE AND VERIFY
		case 0x2e:
			Write10();
			return;

		// VERIFY
		case 0x2f:
			Verify();
			return;

		// READ TOC
		case 0x43:
			ReadToc();
			return;

		// PLAY AUDIO(10)
		case 0x45:
			PlayAudio10();
			return;

		// PLAY AUDIO MSF
		case 0x47:
			PlayAudioMSF();
			return;

		// PLAY AUDIO TRACK
		case 0x48:
			PlayAudioTrack();
			return;
	}

	// それ以外は対応していない
	LOG1(Log::Warning, "未対応コマンド $%02X", scsi.cmd[0]);
	scsi.disk[scsi.id]->InvalidCmd();

	// エラー
	Error();
}

//---------------------------------------------------------------------------
//
//	メッセージインフェーズ
//
//---------------------------------------------------------------------------
void FASTCALL SCSI::MsgIn()
{
	ASSERT(this);
	ASSERT_DIAG();

#if defined(SCSI_LOG)
	LOG0(Log::Normal, "メッセージインフェーズ");
#endif	// SCSI_LOG

	// フェーズ設定
	scsi.phase = msgin;

	// ターゲットが操作する信号線
	scsi.msg = TRUE;
	scsi.cd = TRUE;
	scsi.io = TRUE;

	// データ転送は1バイトx1ブロック
	scsi.offset = 0;
	scsi.length = 1;
	scsi.blocks = 1;

	// メッセージを要求
	scsi.req = TRUE;
}

//---------------------------------------------------------------------------
//
//	メッセージアウトフェーズ
//
//---------------------------------------------------------------------------
void FASTCALL SCSI::MsgOut()
{
	ASSERT(this);
	ASSERT_DIAG();

#if defined(SCSI_LOG)
	LOG0(Log::Normal, "メッセージアウトフェーズ");
#endif	// SCSI_LOG

	// フェーズ設定
	scsi.phase = msgout;

	// ターゲットが操作する信号線
	scsi.msg = TRUE;
	scsi.cd = TRUE;
	scsi.io = FALSE;

	// データ転送は1バイトx1ブロック
	scsi.offset = 0;
	scsi.length = 1;
	scsi.blocks = 1;

	// メッセージを要求
	scsi.req = TRUE;
}

//---------------------------------------------------------------------------
//
//	データインフェーズ
//
//---------------------------------------------------------------------------
void FASTCALL SCSI::DataIn()
{
	ASSERT(this);
	ASSERT(scsi.length >= 0);
	ASSERT_DIAG();

#if defined(SCSI_LOG)
	LOG0(Log::Normal, "データインフェーズ");
#endif	// SCSI_LOG

	// レングス0なら、ステータスフェーズへ
	if (scsi.length == 0) {
		Status();
		return;
	}

	// フェーズ設定
	scsi.phase = datain;

	// ターゲットが操作する信号線
	scsi.msg = FALSE;
	scsi.cd = FALSE;
	scsi.io = TRUE;

	// length, blocksは設定済み
	ASSERT(scsi.length > 0);
	ASSERT(scsi.blocks > 0);
	scsi.offset = 0;

	// データを要求
	scsi.req = TRUE;
}

//---------------------------------------------------------------------------
//
//	データアウトフェーズ
//
//---------------------------------------------------------------------------
void FASTCALL SCSI::DataOut()
{
	ASSERT(this);
	ASSERT(scsi.length >= 0);
	ASSERT_DIAG();

#if defined(SCSI_LOG)
	LOG0(Log::Normal, "データアウトフェーズ");
#endif	// SCSI_LOG

	// レングス0なら、ステータスフェーズへ
	if (scsi.length == 0) {
		Status();
		return;
	}

	// フェーズ設定
	scsi.phase = dataout;

	// ターゲットが操作する信号線
	scsi.msg = FALSE;
	scsi.cd = FALSE;
	scsi.io = FALSE;

	// length, blocksは設定済み
	ASSERT(scsi.length > 0);
	ASSERT(scsi.blocks > 0);
	scsi.offset = 0;

	// データを要求
	scsi.req = TRUE;
}

//---------------------------------------------------------------------------
//
//	ステータスフェーズ
//
//---------------------------------------------------------------------------
void FASTCALL SCSI::Status()
{
	ASSERT(this);
	ASSERT_DIAG();

#if defined(SCSI_LOG)
	LOG0(Log::Normal, "ステータスフェーズ");
#endif	// SCSI_LOG

	// フェーズ設定
	scsi.phase = status;

	// ターゲットが操作する信号線
	scsi.msg = FALSE;
	scsi.cd = TRUE;
	scsi.io = TRUE;

	// データ転送は1バイトx1ブロック
	scsi.offset = 0;
	scsi.length = 1;
	scsi.blocks = 1;

	// ステータスを要求
	scsi.req = TRUE;
}

//---------------------------------------------------------------------------
//
//	割り込み要求
//
//---------------------------------------------------------------------------
void FASTCALL SCSI::Interrupt(int type, BOOL flag)
{
	DWORD ints;

	ASSERT(this);
	ASSERT((type >= 0) && (type <= 7));
	ASSERT_DIAG();

	// INTSのバックアップを取る
	ints = scsi.ints;

	// INTSレジスタ設定
	if (flag) {
		// 割り込み要求
		scsi.ints |= (1 << type);
	}
	else {
		// 割り込みキャンセル
		scsi.ints &= ~(1 << type);
	}

	// 変化していれば、割り込みチェック
	if (ints != scsi.ints) {
		IntCheck();
	}
}

//---------------------------------------------------------------------------
//
//	割り込みチェック
//
//---------------------------------------------------------------------------
void FASTCALL SCSI::IntCheck()
{
	int v;
	int lev;

	ASSERT(this);
	ASSERT_DIAG();

	// 割り込みレベル設定
	if (scsi.type == 1) {
		// 外付(レベルは2または4から選択)
		lev = scsi.ilevel;
	}
	else {
		// 内蔵(レベルは1固定)
		lev = 1;
	}

	// 要求ベクタ作成
	v = -1;
	if (scsi.sctl & 0x01) {
		// 割り込み許可
		if (scsi.ints != 0) {
			// 割り込み要求あり。ベクタを作成
			if (scsi.type == 1) {
				// 外付(ベクタは$F6)
				v = 0xf6;
			}
			else {
				// 内蔵(ベクタは$6C)
				v = 0x6c;
			}
		}
	}

	// 一致していればOK
	if (scsi.vector == v) {
		return;
	}

	// 既に割り込み要求されていれば、キャンセル
	if (scsi.vector >= 0) {
		cpu->IntCancel(lev);
	}

	// キャンセル要求なら、ここまで
	if (v < 0) {
#if defined(SCSI_LOG)
		LOG2(Log::Normal, "割り込みキャンセル レベル%d ベクタ$%02X",
							lev, scsi.vector);
#endif	// SCSI_LOG
		scsi.vector = -1;
		return;
	}

	// 割り込み要求
#if defined(SCSI_LOG)
	LOG3(Log::Normal, "割り込み要求 レベル%d ベクタ$%02X INTS=%02X",
						lev, v, scsi.ints);
#endif	// SCSI_LOG
	cpu->Interrupt(lev, v);
	scsi.vector = v;
}

//---------------------------------------------------------------------------
//
//	割り込みACK
//
//---------------------------------------------------------------------------
void FASTCALL SCSI::IntAck(int level)
{
	ASSERT(this);
	ASSERT((level == 1) || (level == 2) || (level == 4));
	ASSERT_DIAG();

	// リセット直後に、CPUから割り込みが間違って入る場合か、または他のデバイス
	if (scsi.vector < 0) {
		return;
	}

	// 割り込みレベルが異なっていれば、他のデバイス
	if (scsi.type == 1) {
		// 外付(レベルは2または4から選択)
		if (level != scsi.ilevel) {
			return;
		}
	}
	else {
		// 内蔵(レベルは1固定)
		if (level != 1) {
			return;
		}
	}

	// 割り込みなし
	scsi.vector = -1;

	// 割り込みチェック
	IntCheck();
}

//---------------------------------------------------------------------------
//
//	SCSI-ID取得
//
//---------------------------------------------------------------------------
int FASTCALL SCSI::GetSCSIID() const
{
	int id;
	DWORD bdid;

	ASSERT(this);
	ASSERT_DIAG();

	// 存在しなければ7固定
	if (scsi.type == 0) {
		return 7;
	}

	// BDIDから作成
	ASSERT(scsi.bdid != 0);
	ASSERT(scsi.bdid < 0x100);

	// 初期化
	id = 0;
	bdid = scsi.bdid;

	// ビットから数値へ変換
	while ((bdid & 1) == 0) {
		bdid >>= 1;
		id++;
	}

	ASSERT((id >= 0) && (id <= 7));
	return id;
}

//---------------------------------------------------------------------------
//
//	BUSYチェック
//
//---------------------------------------------------------------------------
BOOL FASTCALL SCSI::IsBusy() const
{
	ASSERT(this);
	ASSERT_DIAG();

	// BSYが立っていなければFALSE
	if (!scsi.bsy) {
		return FALSE;
	}

	// セレクションフェーズならFALSE
	if (scsi.phase == selection) {
		return FALSE;
	}

	// BUSY
	return TRUE;
}

//---------------------------------------------------------------------------
//
//	BUSYチェック
//
//---------------------------------------------------------------------------
DWORD FASTCALL SCSI::GetBusyDevice() const
{
	ASSERT(this);
	ASSERT_DIAG();

	// BSYが立っていなければ0
	if (!scsi.bsy) {
		return 0;
	}

	// セレクションフェーズなら0
	if (scsi.phase == selection) {
		return 0;
	}

	// セレクションが終わっているので、通常はデバイスが存在する
	ASSERT((scsi.id >= 0) && (scsi.id <= 7));
	ASSERT(scsi.disk[scsi.id]);

	// デバイスがなければ0
	if (!scsi.disk[scsi.id]) {
		return 0;
	}

	return scsi.disk[scsi.id]->GetID();
}

//---------------------------------------------------------------------------
//
//	共通エラー
//
//---------------------------------------------------------------------------
void FASTCALL SCSI::Error()
{
	ASSERT(this);
	ASSERT_DIAG();

#if defined(SCSI_LOG)
	LOG0(Log::Normal, "エラー(ステータスフェーズへ)");
#endif	// SCSI_LOG

	// ステータスとメッセージを設定(CHECK CONDITION)
	scsi.status = 0x02;
	scsi.message = 0x00;

	// イベント停止
	event.SetTime(0);

	// ステータスフェーズ
	Status();
}

//---------------------------------------------------------------------------
//
//	TEST UNIT READY
//
//---------------------------------------------------------------------------
void FASTCALL SCSI::TestUnitReady()
{
	BOOL status;

	ASSERT(this);
	ASSERT(scsi.disk[scsi.id]);
	ASSERT_DIAG();

#if defined(SCSI_LOG)
	LOG0(Log::Normal, "TEST UNIT READYコマンド");
#endif	// SCSI_LOG

	// ドライブでコマンド処理
	status = scsi.disk[scsi.id]->TestUnitReady(scsi.cmd);
	if (!status) {
		// 失敗(エラー)
		Error();
		return;
	}

	// ステータスフェーズ
	Status();
}

//---------------------------------------------------------------------------
//
//	REZERO UNIT
//
//---------------------------------------------------------------------------
void FASTCALL SCSI::Rezero()
{
	BOOL status;

	ASSERT(this);
	ASSERT(scsi.disk[scsi.id]);
	ASSERT_DIAG();

#if defined(SCSI_LOG)
	LOG0(Log::Normal, "REZERO UNITコマンド");
#endif	// SCSI_LOG

	// ドライブでコマンド処理
	status = scsi.disk[scsi.id]->Rezero(scsi.cmd);
	if (!status) {
		// 失敗(エラー)
		Error();
		return;
	}

	// ステータスフェーズ
	Status();
}

//---------------------------------------------------------------------------
//
//	REQUEST SENSE
//
//---------------------------------------------------------------------------
void FASTCALL SCSI::RequestSense()
{
	ASSERT(this);
	ASSERT(scsi.disk[scsi.id]);
	ASSERT_DIAG();

#if defined(SCSI_LOG)
	LOG0(Log::Normal, "REQUEST SENSEコマンド");
#endif	// SCSI_LOG

	// ドライブでコマンド処理
	scsi.length = scsi.disk[scsi.id]->RequestSense(scsi.cmd, scsi.buffer);
	ASSERT(scsi.length > 0);

#if defined(SCSI_LOG)
	LOG1(Log::Normal, "センスキー $%02X", scsi.buffer[2]);
#endif

	// データインフェーズ
	DataIn();
}

//---------------------------------------------------------------------------
//
//	FORMAT UNIT
//
//---------------------------------------------------------------------------
void FASTCALL SCSI::Format()
{
	BOOL status;

	ASSERT(this);
	ASSERT(scsi.disk[scsi.id]);
	ASSERT_DIAG();

#if defined(SCSI_LOG)
	LOG0(Log::Normal, "FORMAT UNITコマンド");
#endif	// SCSI_LOG

	// ドライブでコマンド処理
	status = scsi.disk[scsi.id]->Format(scsi.cmd);
	if (!status) {
		// 失敗(エラー)
		Error();
		return;
	}

	// ステータスフェーズ
	Status();
}

//---------------------------------------------------------------------------
//
//	REASSIGN BLOCKS
//
//---------------------------------------------------------------------------
void FASTCALL SCSI::Reassign()
{
	BOOL status;

	ASSERT(this);
	ASSERT(scsi.disk[scsi.id]);
	ASSERT_DIAG();

#if defined(SCSI_LOG)
	LOG0(Log::Normal, "REASSIGN BLOCKSコマンド");
#endif	// SCSI_LOG

	// ドライブでコマンド処理
	status = scsi.disk[scsi.id]->Reassign(scsi.cmd);
	if (!status) {
		// 失敗(エラー)
		Error();
		return;
	}

	// ステータスフェーズ
	Status();
}

//---------------------------------------------------------------------------
//
//	READ(6)
//
//---------------------------------------------------------------------------
void FASTCALL SCSI::Read6()
{
	DWORD record;

	ASSERT(this);
	ASSERT(scsi.disk[scsi.id]);
	ASSERT_DIAG();

	// レコード番号とブロック数を取得
	record = scsi.cmd[1] & 0x1f;
	record <<= 8;
	record |= scsi.cmd[2];
	record <<= 8;
	record |= scsi.cmd[3];
	scsi.blocks = scsi.cmd[4];
	if (scsi.blocks == 0) {
		scsi.blocks = 0x100;
	}

#if defined(SCSI_LOG)
	LOG2(Log::Normal, "READ(6)コマンド レコード=%06X ブロック=%d", record, scsi.blocks);
#endif	// SCSI_LOG

	// ドライブでコマンド処理
	scsi.length = scsi.disk[scsi.id]->Read(scsi.buffer, record);
	if (scsi.length <= 0) {
		// 失敗(エラー)
		Error();
		return;
	}

	// 次のブロックを設定
	scsi.next = record + 1;

	// データインフェーズ
	DataIn();
}

//---------------------------------------------------------------------------
//
//	WRITE(6)
//
//---------------------------------------------------------------------------
void FASTCALL SCSI::Write6()
{
	DWORD record;

	ASSERT(this);
	ASSERT(scsi.disk[scsi.id]);
	ASSERT_DIAG();

	// レコード番号とブロック数を取得
	record = scsi.cmd[1] & 0x1f;
	record <<= 8;
	record |= scsi.cmd[2];
	record <<= 8;
	record |= scsi.cmd[3];
	scsi.blocks = scsi.cmd[4];
	if (scsi.blocks == 0) {
		scsi.blocks = 0x100;
	}

#if defined(SCSI_LOG)
	LOG2(Log::Normal, "WRITE(6)コマンド レコード=%06X ブロック=%d", record, scsi.blocks);
#endif	// SCSI_LOG

	// ドライブでコマンド処理
	scsi.length = scsi.disk[scsi.id]->WriteCheck(record);
	if (scsi.length <= 0) {
		// 失敗(エラー)
		Error();
		return;
	}

	// 次のブロックを設定
	scsi.next = record + 1;

	// データアウトフェーズ
	DataOut();
}

//---------------------------------------------------------------------------
//
//	SEEK(6)
//
//---------------------------------------------------------------------------
void FASTCALL SCSI::Seek6()
{
	BOOL status;

	ASSERT(this);
	ASSERT(scsi.disk[scsi.id]);
	ASSERT_DIAG();

#if defined(SCSI_LOG)
	LOG0(Log::Normal, "SEEK(6)コマンド");
#endif	// SCSI_LOG

	// ドライブでコマンド処理
	status = scsi.disk[scsi.id]->Seek(scsi.cmd);
	if (!status) {
		// 失敗(エラー)
		Error();
		return;
	}

	// ステータスフェーズ
	Status();
}

//---------------------------------------------------------------------------
//
//	INQUIRY
//
//---------------------------------------------------------------------------
void FASTCALL SCSI::Inquiry()
{
	ASSERT(this);
	ASSERT(scsi.disk[scsi.id]);
	ASSERT_DIAG();

#if defined(SCSI_LOG)
	LOG0(Log::Normal, "INQUIRYコマンド");
#endif	// SCSI_LOG

	// ドライブでコマンド処理
	scsi.length = scsi.disk[scsi.id]->Inquiry(scsi.cmd, scsi.buffer);
	if (scsi.length <= 0) {
		// 失敗(エラー)
		Error();
		return;
	}

	// データインフェーズ
	DataIn();
}

//---------------------------------------------------------------------------
//
//	MODE SELECT
//
//---------------------------------------------------------------------------
void FASTCALL SCSI::ModeSelect()
{
	ASSERT(this);
	ASSERT(scsi.disk[scsi.id]);
	ASSERT_DIAG();

#if defined(SCSI_LOG)
	LOG0(Log::Normal, "MODE SELECTコマンド");
#endif	// SCSI_LOG

	// ドライブでコマンド処理
	scsi.length = scsi.disk[scsi.id]->SelectCheck(scsi.cmd);
	if (scsi.length <= 0) {
		// 失敗(エラー)
		Error();
		return;
	}

	// データアウトフェーズ
	DataOut();
}

//---------------------------------------------------------------------------
//
//	MODE SENSE
//
//---------------------------------------------------------------------------
void FASTCALL SCSI::ModeSense()
{
	ASSERT(this);
	ASSERT(scsi.disk[scsi.id]);
	ASSERT_DIAG();

#if defined(SCSI_LOG)
	LOG0(Log::Normal, "MODE SENSEコマンド");
#endif	// SCSI_LOG

	// ドライブでコマンド処理
	scsi.length = scsi.disk[scsi.id]->ModeSense(scsi.cmd, scsi.buffer);
	ASSERT(scsi.length >= 0);
	if (scsi.length == 0) {
		LOG1(Log::Warning, "サポートしていないMODE SENSEページ $%02X", scsi.cmd[2]);

		// 失敗(エラー)
		Error();
		return;
	}

	// データインフェーズ
	DataIn();
}

//---------------------------------------------------------------------------
//
//	START STOP UNIT
//
//---------------------------------------------------------------------------
void FASTCALL SCSI::StartStop()
{
	BOOL status;

	ASSERT(this);
	ASSERT(scsi.disk[scsi.id]);
	ASSERT_DIAG();

#if defined(SCSI_LOG)
	LOG0(Log::Normal, "START STOP UNITコマンド");
#endif	// SCSI_LOG

	// ドライブでコマンド処理
	status = scsi.disk[scsi.id]->StartStop(scsi.cmd);
	if (!status) {
		// 失敗(エラー)
		Error();
		return;
	}

	// ステータスフェーズ
	Status();
}

//---------------------------------------------------------------------------
//
//	SEND DIAGNOSTIC
//
//---------------------------------------------------------------------------
void FASTCALL SCSI::SendDiag()
{
	BOOL status;

	ASSERT(this);
	ASSERT(scsi.disk[scsi.id]);
	ASSERT_DIAG();

#if defined(SCSI_LOG)
	LOG0(Log::Normal, "SEND DIAGNOSTICコマンド");
#endif	// SCSI_LOG

	// ドライブでコマンド処理
	status = scsi.disk[scsi.id]->SendDiag(scsi.cmd);
	if (!status) {
		// 失敗(エラー)
		Error();
		return;
	}

	// ステータスフェーズ
	Status();
}

//---------------------------------------------------------------------------
//
//	PREVENT/ALLOW MEDIUM REMOVAL
//
//---------------------------------------------------------------------------
void FASTCALL SCSI::Removal()
{
	BOOL status;

	ASSERT(this);
	ASSERT(scsi.disk[scsi.id]);
	ASSERT_DIAG();

#if defined(SCSI_LOG)
	LOG0(Log::Normal, "PREVENT/ALLOW MEDIUM REMOVALコマンド");
#endif	// SCSI_LOG

	// ドライブでコマンド処理
	status = scsi.disk[scsi.id]->Removal(scsi.cmd);
	if (!status) {
		// 失敗(エラー)
		Error();
		return;
	}

	// ステータスフェーズ
	Status();
}

//---------------------------------------------------------------------------
//
//	READ CAPACITY
//
//---------------------------------------------------------------------------
void FASTCALL SCSI::ReadCapacity()
{
	int length;

	ASSERT(this);
	ASSERT(scsi.disk[scsi.id]);
	ASSERT_DIAG();

#if defined(SCSI_LOG)
	LOG0(Log::Normal, "READ CAPACITYコマンド");
#endif	// SCSI_LOG

	// ドライブでコマンド処理
	length = scsi.disk[scsi.id]->ReadCapacity(scsi.cmd, scsi.buffer);
	ASSERT(length >= 0);
	if (length <= 0) {
		Error();
		return;
	}

	// レングス設定
	scsi.length = length;

	// データインフェーズ
	DataIn();
}

//---------------------------------------------------------------------------
//
//	READ(10)
//
//---------------------------------------------------------------------------
void FASTCALL SCSI::Read10()
{
	DWORD record;

	ASSERT(this);
	ASSERT(scsi.disk[scsi.id]);
	ASSERT_DIAG();

	// レコード番号とブロック数を取得
	record = scsi.cmd[2];
	record <<= 8;
	record |= scsi.cmd[3];
	record <<= 8;
	record |= scsi.cmd[4];
	record <<= 8;
	record |= scsi.cmd[5];
	scsi.blocks = scsi.cmd[7];
	scsi.blocks <<= 8;
	scsi.blocks |= scsi.cmd[8];

#if defined(SCSI_LOG)
	LOG2(Log::Normal, "READ(10)コマンド レコード=%08X ブロック=%d", record, scsi.blocks);
#endif	// SCSI_LOG

	// ブロック数0は処理しない
	if (scsi.blocks == 0) {
		Status();
		return;
	}

	// ドライブでコマンド処理
	scsi.length = scsi.disk[scsi.id]->Read(scsi.buffer, record);
	if (scsi.length <= 0) {
		// 失敗(エラー)
		Error();
		return;
	}

	// 次のブロックを設定
	scsi.next = record + 1;

	// データインフェーズ
	DataIn();
}

//---------------------------------------------------------------------------
//
//	WRITE(10)
//
//---------------------------------------------------------------------------
void FASTCALL SCSI::Write10()
{
	DWORD record;

	ASSERT(this);
	ASSERT(scsi.disk[scsi.id]);
	ASSERT_DIAG();

	// レコード番号とブロック数を取得
	record = scsi.cmd[2];
	record <<= 8;
	record |= scsi.cmd[3];
	record <<= 8;
	record |= scsi.cmd[4];
	record <<= 8;
	record |= scsi.cmd[5];
	scsi.blocks = scsi.cmd[7];
	scsi.blocks <<= 8;
	scsi.blocks |= scsi.cmd[8];

#if defined(SCSI_LOG)
	LOG2(Log::Normal, "WRTIE(10)コマンド レコード=%08X ブロック=%d", record, scsi.blocks);
#endif	// SCSI_LOG

	// ブロック数0は処理しない
	if (scsi.blocks == 0) {
		Status();
		return;
	}

	// ドライブでコマンド処理
	scsi.length = scsi.disk[scsi.id]->WriteCheck(record);
	if (scsi.length <= 0) {
		// 失敗(エラー)
		Error();
		return;
	}

	// 次のブロックを設定
	scsi.next = record + 1;

	// データアウトフェーズ
	DataOut();
}

//---------------------------------------------------------------------------
//
//	SEEK(10)
//
//---------------------------------------------------------------------------
void FASTCALL SCSI::Seek10()
{
	BOOL status;

	ASSERT(this);
	ASSERT(scsi.disk[scsi.id]);
	ASSERT_DIAG();

#if defined(SCSI_LOG)
	LOG0(Log::Normal, "SEEK(10)コマンド");
#endif	// SCSI_LOG

	// ドライブでコマンド処理
	status = scsi.disk[scsi.id]->Seek(scsi.cmd);
	if (!status) {
		// 失敗(エラー)
		Error();
		return;
	}

	// ステータスフェーズ
	Status();
}

//---------------------------------------------------------------------------
//
//	VERIFY
//
//---------------------------------------------------------------------------
void FASTCALL SCSI::Verify()
{
	BOOL status;
	DWORD record;

	ASSERT(this);
	ASSERT(scsi.disk[scsi.id]);
	ASSERT_DIAG();

	// レコード番号とブロック数を取得
	record = scsi.cmd[2];
	record <<= 8;
	record |= scsi.cmd[3];
	record <<= 8;
	record |= scsi.cmd[4];
	record <<= 8;
	record |= scsi.cmd[5];
	scsi.blocks = scsi.cmd[7];
	scsi.blocks <<= 8;
	scsi.blocks |= scsi.cmd[8];

#if defined(SCSI_LOG)
	LOG2(Log::Normal, "VERIFYコマンド レコード=%08X ブロック=%d", record, scsi.blocks);
#endif	// SCSI_LOG

	// ブロック数0は処理しない
	if (scsi.blocks == 0) {
		Status();
		return;
	}

	// BytChk=0なら
	if ((scsi.cmd[1] & 0x02) == 0) {
		// ドライブでコマンド処理
		status = scsi.disk[scsi.id]->Seek(scsi.cmd);
		if (!status) {
			// 失敗(エラー)
			Error();
			return;
		}

		// ステータスフェーズ
		Status();
		return;
	}

	// テスト読み込み
	scsi.length = scsi.disk[scsi.id]->Read(scsi.buffer, record);
	if (scsi.length <= 0) {
		// 失敗(エラー)
		Error();
		return;
	}

	// 次のブロックを設定
	scsi.next = record + 1;

	// データアウトフェーズ
	DataOut();
}

//---------------------------------------------------------------------------
//
//	READ TOC
//
//---------------------------------------------------------------------------
void FASTCALL SCSI::ReadToc()
{
	ASSERT(this);
	ASSERT(scsi.disk[scsi.id]);
	ASSERT_DIAG();

	// ドライブでコマンド処理
	scsi.length = scsi.disk[scsi.id]->ReadToc(scsi.cmd, scsi.buffer);
	if (scsi.length <= 0) {
		// 失敗(エラー)
		Error();
		return;
	}

	// データインフェーズ
	DataIn();
}

//---------------------------------------------------------------------------
//
//	PLAY AUDIO(10)
//
//---------------------------------------------------------------------------
void FASTCALL SCSI::PlayAudio10()
{
	BOOL status;

	ASSERT(this);
	ASSERT(scsi.disk[scsi.id]);
	ASSERT_DIAG();

	// ドライブでコマンド処理
	status = scsi.disk[scsi.id]->PlayAudio(scsi.cmd);
	if (!status) {
		// 失敗(エラー)
		Error();
		return;
	}

	// CD-DAイベントスタート(26666×74+26716で、合計1sec)
	if (cdda.GetTime() == 0) {
		cdda.SetTime(26666);
	}

	// ステータスフェーズ
	Status();
}

//---------------------------------------------------------------------------
//
//	PLAY AUDIO MSF
//
//---------------------------------------------------------------------------
void FASTCALL SCSI::PlayAudioMSF()
{
	BOOL status;

	ASSERT(this);
	ASSERT(scsi.disk[scsi.id]);
	ASSERT_DIAG();

	// ドライブでコマンド処理
	status = scsi.disk[scsi.id]->PlayAudioMSF(scsi.cmd);
	if (!status) {
		// 失敗(エラー)
		Error();
		return;
	}

	// CD-DAイベントスタート(26666×74+26716で、合計1sec)
	if (cdda.GetTime() == 0) {
		cdda.SetTime(26666);
	}

	// ステータスフェーズ
	Status();
}

//---------------------------------------------------------------------------
//
//	PLAY AUDIO TRACK
//
//---------------------------------------------------------------------------
void FASTCALL SCSI::PlayAudioTrack()
{
	BOOL status;

	ASSERT(this);
	ASSERT(scsi.disk[scsi.id]);
	ASSERT_DIAG();

	// ドライブでコマンド処理
	status = scsi.disk[scsi.id]->PlayAudioTrack(scsi.cmd);
	if (!status) {
		// 失敗(エラー)
		Error();
		return;
	}

	// CD-DAイベントスタート(26666×74+26716で、合計1sec)
	if (cdda.GetTime() == 0) {
		cdda.SetTime(26666);
	}

	// ステータスフェーズ
	Status();
}

//---------------------------------------------------------------------------
//
//	ディスク再構築
//
//---------------------------------------------------------------------------
void FASTCALL SCSI::Construct()
{
	int hd;
	BOOL mo;
	BOOL cd;
	int i;
	int init;
	int index;
	Filepath path;

	ASSERT(this);
	ASSERT_DIAG();

	// 一旦クリア
	hd = 0;
	mo = FALSE;
	cd = FALSE;
	for (i=0; i<DeviceMax; i++) {
		scsi.disk[i] = NULL;
	}

	// SCSIが存在しないなら、ディスク削除して終了
	if (scsi.type == 0) {
		for (i=0; i<HDMax; i++) {
			if (scsi.hd[i]) {
				delete scsi.hd[i];
				scsi.hd[i] = NULL;
			}
			if (scsi.mo) {
				delete scsi.mo;
				scsi.mo = NULL;
			}
			if (scsi.cdrom) {
				delete scsi.cdrom;
				scsi.cdrom = NULL;
			}
		}
		return;
	}

	// ディスク数を決定
	switch (scsi.scsi_drives) {
		// 0台
		case 0:
			break;

		// 1台
		case 1:
			// MO優先か、HD優先かで分ける
			if (scsi.mo_first) {
				mo = TRUE;
			}
			else {
				hd = 1;
			}
			break;

		// 2台
		case 2:
			// HD,MOとも1台
			hd = 1;
			mo = TRUE;
			break;

		// 3台
		case 3:
			// HD,MO,CDとも1台
			hd = 1;
			mo = TRUE;
			cd = TRUE;

		// 4台以上
		default:
			ASSERT(scsi.scsi_drives <= 7);
			hd = scsi.scsi_drives - 2;
			mo = TRUE;
			cd = TRUE;
			break;
	}

	// HD作成
	for (i=0; i<HDMax; i++) {
		// 個数を超えていれば、削除
		if (i >= hd) {
			if (scsi.hd[i]) {
				delete scsi.hd[i];
				scsi.hd[i] = NULL;
			}
			continue;
		}

		// ディスク種別を確認
		if (scsi.hd[i]) {
			if (scsi.hd[i]->GetID() != MAKEID('S', 'C', 'H', 'D')) {
				delete scsi.hd[i];
				scsi.hd[i] = NULL;
			}
		}

		// ディスクがあって
		if (scsi.hd[i]) {
			// パスを取得し、一致すればok
			scsi.hd[i]->GetPath(path);
			if (path.CmpPath(scsihd[i])) {
				// パスが一致している
				continue;
			}

			// パスが違うので、ディスクをいったん削除
			delete scsi.hd[i];
			scsi.hd[i] = NULL;
		}

		// SCSIハードディスクを作成してオープンを試みる
		scsi.hd[i] = new SCSIHD(this);
		if (!scsi.hd[i]->Open(scsihd[i])) {
			// オープン失敗。この番号のscsi.diskはNULLとする
			delete scsi.hd[i];
			scsi.hd[i] = NULL;
		}
	}

	// MO作成
	if (mo) {
		// MOあり
		if (scsi.mo) {
			if (scsi.mo->GetID() != MAKEID('S', 'C', 'M', 'O')) {
				delete scsi.mo;
				scsi.mo = NULL;
			}
		}
		if (!scsi.mo) {
			scsi.mo = new SCSIMO(this);
		}
	}
	else {
		// MOなし
		if (scsi.mo) {
			delete scsi.mo;
			scsi.mo = NULL;
		}
	}

	// CD-ROM作成
	if (cd) {
		// CD-ROMあり
		if (scsi.cdrom) {
			if (scsi.cdrom->GetID() != MAKEID('S', 'C', 'C', 'D')) {
				delete scsi.cdrom;
				scsi.cdrom = NULL;
			}
		}
		if (!scsi.cdrom) {
			scsi.cdrom = new SCSICD(this);
		}
	}
	else {
		// CD-ROMなし
		if (scsi.cdrom) {
			delete scsi.cdrom;
			scsi.cdrom = NULL;
		}
	}

	// ホスト・MO優先を考慮して、ディスクを並べる
	init = GetSCSIID();
	index = 0;
	for (i=0; i<DeviceMax; i++) {
		// ホスト(イニシエータ)はスキップ
		if (i == init) {
			continue;
		}

		// MO優先の場合、MOを並べる
		if (scsi.mo_first) {
			if (mo) {
				ASSERT(scsi.mo);
				scsi.disk[i] = scsi.mo;
				mo = FALSE;
				continue;
			}
		}

		// HDを並べる
		if (index < hd) {
			// NULLでもよい
			scsi.disk[i] = scsi.hd[index];
			index++;
			continue;
		}

		// MOを並べる
		if (mo) {
			ASSERT(scsi.mo);
			scsi.disk[i] = scsi.mo;
			mo = FALSE;
			continue;
		}
	}

	// CDはID6固定。ID6が一杯ならID7
	if (cd) {
		ASSERT(scsi.cdrom);
		if (!scsi.disk[6]) {
			scsi.disk[6] = scsi.cdrom;
		}
		else {
			ASSERT(!scsi.disk[7]);
			scsi.disk[7] = scsi.cdrom;
		}
	}

#if defined(SCSI_LOG)
	for (i=0; i<DeviceMax; i++) {
		if (!scsi.disk[i]) {
			LOG1(Log::Detail, "ID=%d : 未接続またはイニシエータ", i);
			continue;
		}
		switch (scsi.disk[i]->GetID()) {
			case MAKEID('S', 'C', 'H', 'D'):
				LOG1(Log::Detail, "ID=%d : SCSI ハードディスク", i);
				break;
			case MAKEID('S', 'C', 'M', 'O'):
				LOG1(Log::Detail, "ID=%d : SCSI MOディスク", i);
				break;
			case MAKEID('S', 'C', 'C', 'D'):
				LOG1(Log::Detail, "ID=%d : SCSI CD-ROMドライブ", i);
				break;
			default:
				LOG2(Log::Detail, "ID=%d : 未定義(%08X)", i, scsi.disk[i]->GetID());
				break;
		}
	}
#endif	//	SCSI_LOG
}

//---------------------------------------------------------------------------
//
//	MO/CD オープン
//
//---------------------------------------------------------------------------
BOOL FASTCALL SCSI::Open(const Filepath& path, BOOL mo)
{
	ASSERT(this);
	ASSERT_DIAG();

	// 有効でなければエラー
	if (!IsValid(mo)) {
		return FALSE;
	}

	// イジェクト
	Eject(FALSE, mo);

	// ノットレディでなければエラー
	if (IsReady(mo)) {
		return FALSE;
	}

	// ドライブに任せる
	if (mo) {
		ASSERT(scsi.mo);
		if (scsi.mo->Open(path, TRUE)) {
			// 成功(MO)
			return TRUE;
		}
	}
	else {
		ASSERT(scsi.cdrom);
		if (scsi.cdrom->Open(path, TRUE)) {
			// 成功(CD)
			return TRUE;
		}
	}

	// 失敗
	return FALSE;
}

//---------------------------------------------------------------------------
//
//	MO/CD イジェクト
//
//---------------------------------------------------------------------------
void FASTCALL SCSI::Eject(BOOL force, BOOL mo)
{
	ASSERT(this);
	ASSERT_DIAG();

	// レディでなければ何もしない
	if (!IsReady(mo)) {
		return;
	}

	// ドライブに通知
	if (mo) {
		ASSERT(scsi.mo);
		scsi.mo->Eject(force);
	}
	else {
		ASSERT(scsi.cdrom);
		scsi.cdrom->Eject(force);
	}
}

//---------------------------------------------------------------------------
//
//	MO 書き込み禁止
//
//---------------------------------------------------------------------------
void FASTCALL SCSI::WriteP(BOOL flag)
{
	ASSERT(this);
	ASSERT_DIAG();

	// レディでなければ何もしない
	if (!IsReady()) {
		return;
	}

	// ライトプロテクト設定を試みる
	ASSERT(scsi.mo);
	scsi.mo->WriteP(flag);
}

//---------------------------------------------------------------------------
//
//	MO 書き込み禁止取得
//
//---------------------------------------------------------------------------
BOOL FASTCALL SCSI::IsWriteP() const
{
	ASSERT(this);
	ASSERT_DIAG();

	// レディでなければライトプロテクトでない
	if (!IsReady()) {
		return FALSE;
	}

	// ドライブに聞く
	ASSERT(scsi.mo);
	return scsi.mo->IsWriteP();
}

//---------------------------------------------------------------------------
//
//	MO ReadOnlyチェック
//
//---------------------------------------------------------------------------
BOOL FASTCALL SCSI::IsReadOnly() const
{
	ASSERT(this);
	ASSERT_DIAG();

	// レディでなければリードオンリーでない
	if (!IsReady()) {
		return FALSE;
	}

	// ドライブに聞く
	ASSERT(scsi.mo);
	return scsi.mo->IsReadOnly();
}

//---------------------------------------------------------------------------
//
//	MO/CD Lockedチェック
//
//---------------------------------------------------------------------------
BOOL FASTCALL SCSI::IsLocked(BOOL mo) const
{
	ASSERT(this);
	ASSERT_DIAG();

	if (mo) {
		// ドライブが存在するか(MO)
		if (!scsi.mo) {
			return FALSE;
		}

		// ドライブに聞く(MO)
		return scsi.mo->IsLocked();
	}
	else {
		// ドライブが存在するか(CD)
		if (!scsi.cdrom) {
			return FALSE;
		}

		// ドライブに聞く(CD)
		return scsi.cdrom->IsLocked();
	}
}

//---------------------------------------------------------------------------
//
//	MO/CD Readyチェック
//
//---------------------------------------------------------------------------
BOOL FASTCALL SCSI::IsReady(BOOL mo) const
{
	ASSERT(this);
	ASSERT_DIAG();

	if (mo) {
		// ドライブが存在するか(MO)
		if (!scsi.mo) {
			return FALSE;
		}

		// ドライブに聞く(MO)
		return scsi.mo->IsReady();
	}
	else {
		// ドライブが存在するか(CD)
		if (!scsi.cdrom) {
			return FALSE;
		}

		// ドライブに聞く(CD)
		return scsi.cdrom->IsReady();
	}
}

//---------------------------------------------------------------------------
//
//	MO/CD 有効チェック
//
//---------------------------------------------------------------------------
BOOL FASTCALL SCSI::IsValid(BOOL mo) const
{
	ASSERT(this);
	ASSERT_DIAG();

	if (mo) {
		// ポインタで判別(MO)
		if (scsi.mo) {
			return TRUE;
		}
	}
	else {
		// ポインタで判別(CD)
		if (scsi.cdrom) {
			return TRUE;
		}
	}

	// ドライブなし
	return FALSE;
}

//---------------------------------------------------------------------------
//
//	MO/CD パス取得
//
//---------------------------------------------------------------------------
void FASTCALL SCSI::GetPath(Filepath& path, BOOL mo) const
{
	ASSERT(this);
	ASSERT_DIAG();

	if (mo) {
		// MO
		if (scsi.mo) {
			// レディか
			if (scsi.mo->IsReady()) {
				// パス取得
				scsi.mo->GetPath(path);
				return;
			}
		}
	}
	else {
		// CD
		if (scsi.cdrom) {
			// レディか
			if (scsi.cdrom->IsReady()) {
				// パス取得
				scsi.cdrom->GetPath(path);
				return;
			}
		}
	}

	// 有効なパスでないので、クリア
	path.Clear();
}

//---------------------------------------------------------------------------
//
//	CD-DAバッファ取得
//
//---------------------------------------------------------------------------
void FASTCALL SCSI::GetBuf(DWORD *buffer, int samples, DWORD rate)
{
	ASSERT(this);
	ASSERT(buffer);
	ASSERT(samples >= 0);
	ASSERT(rate != 0);
	ASSERT((rate % 100) == 0);
	ASSERT_DIAG();

	// インタフェースが有効で
	if (scsi.type != 0) {
		// CD-ROMが存在する場合に限る
		if (scsi.cdrom) {
			// CD-ROMから要求
			scsi.cdrom->GetBuf(buffer, samples, rate);
		}
	}
}
