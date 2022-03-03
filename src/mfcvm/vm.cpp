//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 ＰＩ．(ytanaka@ipc-tokai.or.jp)
//	[ 仮想マシン ]
//
//---------------------------------------------------------------------------

#include "os.h"
#include "xm6.h"
#include "device.h"
#include "schedule.h"
#include "cpu.h"
#include "memory.h"
#include "sram.h"
#include "sysport.h"
#include "tvram.h"
#include "vc.h"
#include "crtc.h"
#include "rtc.h"
#include "ppi.h"
#include "dmac.h"
#include "mfp.h"
#include "fdc.h"
#include "iosc.h"
#include "sasi.h"
#include "opmif.h"
#include "keyboard.h"
#include "adpcm.h"
#include "gvram.h"
#include "sprite.h"
#include "fdd.h"
#include "scc.h"
#include "mouse.h"
#include "printer.h"
#include "areaset.h"
#include "windrv.h"
#include "render.h"
#include "midi.h"
#include "scsi.h"
#include "mercury.h"
#include "neptune.h"
#include "filepath.h"
#include "fileio.h"
#include "vm.h"

//===========================================================================
//
//	仮想マシン
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	コンストラクタ
//
//---------------------------------------------------------------------------
VM::VM()
{
	// ワーク初期化
	status = FALSE;
	first_device = NULL;
	scheduler = NULL;

	// デバイスNULL
	scheduler = NULL;
	cpu = NULL;
	mfp = NULL;
	rtc = NULL;
	sram = NULL;

	// バージョン(実際はプラットフォームから再設定される)
	major_ver = 0x01;
	minor_ver = 0x00;

	// カレントパスをクリア
	Clear();
}

//---------------------------------------------------------------------------
//
//	初期化
//
//---------------------------------------------------------------------------
BOOL FASTCALL VM::Init()
{
	Device *device;

	ASSERT(this);
	ASSERT(!first_device);
	ASSERT(!status);

	// 電源、電源スイッチon
	power = TRUE;
	power_sw = TRUE;

	// デバイスを作成(順序に注意)
	scheduler = new Scheduler(this);
	cpu = new CPU(this);
	new Keyboard(this);
	new Mouse(this);
	new FDD(this);
	new Render(this);
	new Memory(this);
	new GVRAM(this);
	new TVRAM(this);
	new CRTC(this);
	new VC(this);
	new DMAC(this);
	new AreaSet(this);
	mfp = new MFP(this);
	rtc = new RTC(this);
	new Printer(this);
	new SysPort(this);
	new OPMIF(this);
	new ADPCM(this);
	new FDC(this);
	new SASI(this);
	new SCC(this);
	new PPI(this);
	new IOSC(this);
	new Windrv(this);
	new SCSI(this);
	new MIDI(this);
	new Sprite(this);
	new Mercury(this);
	new Neptune(this);
	sram = new SRAM(this);

	// ログを初期化
	if (!log.Init(this)) {
		return FALSE;
	}

	// デバイスポインタ初期化
	device = first_device;

	// 初期化(順番に回る)
	status = TRUE;
	while (device) {
		if (!device->Init()) {
			status = FALSE;
		}
		device = device->GetNextDevice();
	}

	return status;
}

//---------------------------------------------------------------------------
//
//	クリーンアップ
//
//---------------------------------------------------------------------------
void FASTCALL VM::Cleanup()
{
	ASSERT(this);

	// 電源がONの状態で強制終了した場合、SRAMの起動カウンタを更新する
	if (status) {
		if (power) {
			// SRAM更新
			ASSERT(sram);
			sram->UpdateBoot();
		}
	}

	// ポインタは変更されるので、先頭だけ見る
	while (first_device) {
		first_device->Cleanup();
	}

	// ログをクリーンアップ
	log.Cleanup();
}

//---------------------------------------------------------------------------
//
//	リセット
//
//---------------------------------------------------------------------------
void FASTCALL VM::Reset()
{
	Device *device;

	ASSERT(this);

	// ログをリセット
	log.Reset();

	// デバイスポインタ初期化
	device = first_device;

	// リセット(順番に回る)
	while (device) {
		device->Reset();
		device = device->GetNextDevice();
	}

	// カレントパスをクリア
	Clear();
}

//---------------------------------------------------------------------------
//
//	セーブ
//
//---------------------------------------------------------------------------
DWORD FASTCALL VM::Save(const Filepath& path)
{
	Fileio fio;
	char header[0x10];
	int ver;
	Device *device;
	DWORD id;
	DWORD pos;

	ASSERT(this);

	// デバイスポインタ初期化
	device = first_device;

	// バージョン作成
	ver = (int)((major_ver << 8) | minor_ver);

	// ヘッダ作成
	sprintf(header, "XM6 DATA %1X.%02X", major_ver, minor_ver);
	header[0x0d] = 0x0d;
	header[0x0e] = 0x0a;
	header[0x0f] = 0x1a;

	// ファイル作成、ヘッダ書き込み
	if (!fio.Open(path, Fileio::WriteOnly)) {
		return 0;
	}
	if (!fio.Write(header, 0x10)) {
		fio.Close();
		return 0;
	}

	// 順番に回る(バージョンはBCDが渡される)
	while (device) {
		// ID書き込み
		id = device->GetID();
		if (!fio.Write(&id, sizeof(id))) {
			fio.Close();
			return 0;
		}

		// デバイス別
		if (!device->Save(&fio, ver)) {
			// デバイスが失敗した
			fio.Close();
			return 0;
		}

		// 次のデバイスへ
		device = device->GetNextDevice();
	}

	// 識別用として、デバイス名ENDを与える
	id = MAKEID('E', 'N', 'D', ' ');
	if (!fio.Write(&id, sizeof(id))) {
		fio.Close();
		return 0;
	}

	// 位置を保存
	pos = fio.GetFilePos();

	// ファイルクローズ
	fio.Close();

	// カレントに設定
	current = path;

	// 成功
	return pos;
}

//---------------------------------------------------------------------------
//
//	ロード
//
//---------------------------------------------------------------------------
DWORD FASTCALL VM::Load(const Filepath& path)
{
	Fileio fio;
	char buf[0x10];
	int rec;
	int ver;
	Device *device;
	DWORD id;
	DWORD pos;

	ASSERT(this);

	// カレントパスをクリア
	current.Clear();

	// ファイルオープン、ヘッダ読み込み
	if (!fio.Open(path, Fileio::ReadOnly)) {
		return 0;
	}
	if (!fio.Read(buf, 0x10)) {
		fio.Close();
		return 0;
	}

	// 記録バージョン取得
	buf[0x0a] = '\0';
	rec = ::strtoul(&buf[0x09], NULL, 16);
	rec <<= 8;
	buf[0x0d] = '\0';
	rec |= ::strtoul(&buf[0x0b], NULL, 16);

	// 現行バージョン作成
	ver = (int)((major_ver << 8) | minor_ver);

	// ヘッダチェック
	buf[0x09] = '\0';
	if (strcmp(buf, "XM6 DATA ") != 0) {
		fio.Close();
		return 0;
	}

	// バージョンチェック
	if (ver < rec) {
		// 記録されているバージョンのほうが新しい(知らない形式)
		fio.Close();
		return 0;
	}

	// デバイス名を検索しながら回る(バージョンはBCDが渡される)
	for (;;) {
		// ID読み込み
		if (!fio.Read(&id, sizeof(id))) {
			fio.Close();
			return 0;
		}

		// 終端チェック
		if (id == MAKEID('E', 'N', 'D', ' ')) {
			break;
		}

		// デバイスサーチ
		device = SearchDevice(id);
		if (!device) {
			// セーブ時に存在したデバイスが、今はない。ロードできない
			fio.Close();
			return 0;
		}

		// デバイス別
		if (!device->Load(&fio, rec)) {
			// デバイスが失敗した
			fio.Close();
			return 0;
		}
	}

	// 位置を保存
	pos = fio.GetFilePos();

	// ファイルクローズ
	fio.Close();

	// カレントに設定
	current = path;

	// 成功
	return pos;
}


//---------------------------------------------------------------------------
//
//	パス取得
//
//---------------------------------------------------------------------------
void FASTCALL VM::GetPath(Filepath& path) const
{
	ASSERT(this);

	path = current;
}

//---------------------------------------------------------------------------
//
//	パスクリア
//
//---------------------------------------------------------------------------
void FASTCALL VM::Clear()
{
	ASSERT(this);

	current.Clear();
}

//---------------------------------------------------------------------------
//
//	設定適用
//
//---------------------------------------------------------------------------
void FASTCALL VM::ApplyCfg(const Config *config)
{
	Device *device;

	ASSERT(this);
	ASSERT(config);

	// デバイスポインタ初期化
	device = first_device;

	// 適用(順番に回る)
	while (device) {
		device->ApplyCfg(config);
		device = device->GetNextDevice();
	}
}

//---------------------------------------------------------------------------
//
//	デバイス追加
//	※追加したいDeviceから呼び出す
//
//---------------------------------------------------------------------------
void FASTCALL VM::AddDevice(Device *device)
{
	Device *dev;

	ASSERT(this);
	ASSERT(device);

	// 最初のデバイスか
	if (!first_device) {
		// このデバイスが最初。登録する
		first_device = device;
		ASSERT(!device->GetNextDevice());
		return;
	}

	// 終端を探す
	dev = first_device;
	while (dev->GetNextDevice()) {
		dev = dev->GetNextDevice();
	}

	// devの後ろに追加
	dev->SetNextDevice(device);
	ASSERT(!device->GetNextDevice());
}

//---------------------------------------------------------------------------
//
//	デバイス削除
//	※削除したいDeviceから呼び出す
//
//---------------------------------------------------------------------------
void FASTCALL VM::DelDevice(const Device *device)
{
	Device *dev;

	ASSERT(this);
	ASSERT(device);

	// 最初のデバイスか
	if (first_device == device) {
		// 次があるなら、次を登録。なければNULL
		if (device->GetNextDevice()) {
			first_device = device->GetNextDevice();
		}
		else {
			first_device = NULL;
		}
		return;
	}

	// deviceを記憶しているサブウィンドウを探す
	dev = first_device;
	while (dev->GetNextDevice() != device) {
		ASSERT(dev->GetNextDevice());
		dev = dev->GetNextDevice();
	}

	// device->next_deviceを、devに結びつけスキップさせる
	dev->SetNextDevice(device->GetNextDevice());
}

//---------------------------------------------------------------------------
//
//	デバイス検索
//	※見つからなければNULLを返す
//
//---------------------------------------------------------------------------
Device* FASTCALL VM::SearchDevice(DWORD id) const
{
	Device *dev;

	ASSERT(this);

	// デバイスを初期化
	dev = first_device;

	// 検索ループ
	while (dev) {
		// IDが一致するかチェック
		if (dev->GetID() == id) {
			return dev;
		}

		// 次へ
		dev = dev->GetNextDevice();
	}

	// 見つからなかった
	return NULL;
}

//---------------------------------------------------------------------------
//
//	実行
//
//---------------------------------------------------------------------------
BOOL FASTCALL VM::Exec(DWORD hus)
{
	DWORD ret;

	ASSERT(this);
	ASSERT(scheduler);

	// 電源チェック
	if (power) {
		// 実行ループ
		while (hus > 0) {
			ret = scheduler->Exec(hus);

			// 正常なら、残りタイムを減らす
			if (ret < 0x80000000) {
				hus -= ret;
				continue;
			}

			// ブレークしたら、FALSEで終了
			return FALSE;
		}
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	トレース
//
//---------------------------------------------------------------------------
void FASTCALL VM::Trace()
{
	ASSERT(this);
	ASSERT(scheduler);

	// 電源チェック
	if (!power) {
		return;
	}

	// 0以外が出るまで実行
	for (;;) {
		if (scheduler->Trace(100) != 0) {
			return;
		}
	}
}

//---------------------------------------------------------------------------
//
//	電源スイッチ制御
//
//---------------------------------------------------------------------------
void FASTCALL VM::PowerSW(BOOL sw)
{
	ASSERT(this);

	// 現在の状態と同じなら何もしない
	if (power_sw == sw) {
		return;
	}

	// 記憶して
	power_sw = sw;

	// 電源オフなら、電源オンでリセット
	if (sw) {
		SetPower(TRUE);
	}

	// MFPに対し、電源情報を伝える
	ASSERT(mfp);
	if (sw) {
		mfp->SetGPIP(2, 0);
	}
	else {
		mfp->SetGPIP(2, 1);
	}
}

//---------------------------------------------------------------------------
//
//	電源の状態を設定
//
//---------------------------------------------------------------------------
void FASTCALL VM::SetPower(BOOL flag)
{
	ASSERT(this);

	// 一致していれば何もしない
	if (flag == power) {
		return;
	}

	// 一致
	power = flag;

	if (flag) {
		// 電源ON(時刻アジャストを行う)
		Reset();
		ASSERT(rtc);
		rtc->Adjust(FALSE);
	}
}

//---------------------------------------------------------------------------
//
//	バージョン設定
//
//---------------------------------------------------------------------------
void FASTCALL VM::SetVersion(DWORD major, DWORD minor)
{
	ASSERT(this);
	ASSERT(major < 0x100);
	ASSERT(minor < 0x100);

	major_ver = major;
	minor_ver = minor;
}

//---------------------------------------------------------------------------
//
//	バージョン取得
//
//---------------------------------------------------------------------------
void FASTCALL VM::GetVersion(DWORD& major, DWORD& minor)
{
	ASSERT(this);
	ASSERT(major_ver < 0x100);
	ASSERT(minor_ver < 0x100);

	major = major_ver;
	minor = minor_ver;
}
