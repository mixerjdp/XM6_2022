//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 ＰＩ．(ytanaka@ipc-tokai.or.jp)
//	[ システムポート ]
//
//---------------------------------------------------------------------------

#include "os.h"
#include "xm6.h"
#include "vm.h"
#include "log.h"
#include "sram.h"
#include "keyboard.h"
#include "cpu.h"
#include "crtc.h"
#include "render.h"
#include "memory.h"
#include "schedule.h"
#include "fileio.h"
#include "sysport.h"

//===========================================================================
//
//	システムポート
//
//===========================================================================
//#define SYSPORT_LOG

//---------------------------------------------------------------------------
//
//	コンストラクタ
//
//---------------------------------------------------------------------------
SysPort::SysPort(VM *p) : MemDevice(p)
{
	// デバイスIDを初期化
	dev.id = MAKEID('S', 'Y', 'S', 'P');
	dev.desc = "System (MESSIAH)";

	// 開始アドレス、終了アドレス
	memdev.first = 0xe8e000;
	memdev.last = 0xe8ffff;

	// すべてクリア
	memset(&sysport, 0, sizeof(sysport));

	// オブジェクト
	memory = NULL;
	sram = NULL;
	keyboard = NULL;
	crtc = NULL;
	render = NULL;
}

//---------------------------------------------------------------------------
//
//	初期化
//
//---------------------------------------------------------------------------
BOOL FASTCALL SysPort::Init()
{
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

	// キーボード取得
	ASSERT(!keyboard);
	keyboard = (Keyboard*)vm->SearchDevice(MAKEID('K', 'E', 'Y', 'B'));
	ASSERT(keyboard);

	// CRTC取得
	ASSERT(!crtc);
	crtc = (CRTC*)vm->SearchDevice(MAKEID('C', 'R', 'T', 'C'));
	ASSERT(crtc);

	// レンダラ取得
	ASSERT(!render);
	render = (Render*)vm->SearchDevice(MAKEID('R', 'E', 'N', 'D'));
	ASSERT(render);

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	クリーンアップ
//
//---------------------------------------------------------------------------
void FASTCALL SysPort::Cleanup()
{
	ASSERT(this);

	// 基本クラスへ
	MemDevice::Cleanup();
}

//---------------------------------------------------------------------------
//
//	リセット
//
//---------------------------------------------------------------------------
void FASTCALL SysPort::Reset()
{
	ASSERT(this);
	ASSERT_DIAG();

	LOG0(Log::Normal, "リセット");

	// 設定値をリセット
	sysport.contrast = 0;
	sysport.scope_3d = 0;
	sysport.image_unit = 0;
	sysport.power_count = 0;
	sysport.ver_count = 0;
}

//---------------------------------------------------------------------------
//
//	セーブ
//
//---------------------------------------------------------------------------
BOOL FASTCALL SysPort::Save(Fileio *fio, int /*ver*/)
{
	size_t sz;

	ASSERT(this);
	ASSERT(fio);
	ASSERT_DIAG();

	LOG0(Log::Normal, "セーブ");

	// サイズをセーブ
	sz = sizeof(sysport_t);
	if (!fio->Write(&sz, sizeof(sz))) {
		return FALSE;
	}

	// 実体をセーブ
	if (!fio->Write(&sysport, (int)sz)) {
		return FALSE;
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	ロード
//
//---------------------------------------------------------------------------
BOOL FASTCALL SysPort::Load(Fileio *fio, int /*ver*/)
{
	size_t sz;

	ASSERT(this);
	ASSERT(fio);
	ASSERT_DIAG();

	LOG0(Log::Normal, "ロード");

	// サイズをロード、照合
	if (!fio->Read(&sz, sizeof(sz))) {
		return FALSE;
	}
	if (sz != sizeof(sysport_t)) {
		return FALSE;
	}

	// 実体をロード
	if (!fio->Read(&sysport, (int)sz)) {
		return FALSE;
	}

	// レンダラへ通知
	render->SetContrast(sysport.contrast);

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	設定適用
//
//---------------------------------------------------------------------------
void FASTCALL SysPort::ApplyCfg(const Config* /*config*/)
{
	ASSERT(this);
	ASSERT_DIAG();

	LOG0(Log::Normal, "設定適用");
}

#if !defined(NDEBUG)
//---------------------------------------------------------------------------
//
//	診断
//
//---------------------------------------------------------------------------
void FASTCALL SysPort::AssertDiag() const
{
	// 基本クラス
	MemDevice::AssertDiag();

	ASSERT(this);
	ASSERT(GetID() == MAKEID('S', 'Y', 'S', 'P'));
	ASSERT(memdev.first == 0xe8e000);
	ASSERT(memdev.last == 0xe8ffff);
	ASSERT(memory);
	ASSERT(memory->GetID() == MAKEID('M', 'E', 'M', ' '));
	ASSERT(sram);
	ASSERT(sram->GetID() == MAKEID('S', 'R', 'A', 'M'));
	ASSERT(keyboard);
	ASSERT(keyboard->GetID() == MAKEID('K', 'E', 'Y', 'B'));
	ASSERT(crtc);
	ASSERT(crtc->GetID() == MAKEID('C', 'R', 'T', 'C'));
	ASSERT(sysport.contrast <= 0x0f);
	ASSERT(sysport.scope_3d <= 0x03);
	ASSERT(sysport.image_unit <= 0x1f);
	ASSERT(sysport.power_count <= 0x03);
	ASSERT(sysport.ver_count <= 0x03);
}
#endif	// NDEBUG

//---------------------------------------------------------------------------
//
//	バイト読み込み
//
//---------------------------------------------------------------------------
DWORD FASTCALL SysPort::ReadByte(DWORD addr)
{
	DWORD data;

	ASSERT(this);
	ASSERT((addr >= memdev.first) && (addr <= memdev.last));
	ASSERT_DIAG();

	// 4bitのみデコード
	addr &= 0x0f;

	// 奇数バイトのみデコード
	if ((addr & 1) == 0) {
		return 0xff;
	}
	addr >>= 1;

	// ウェイト
	scheduler->Wait(1);

	switch (addr) {
		// コントラスト
		case 0:
			data = 0xf0;
			data |= sysport.contrast;
			return data;

		// ディスプレイ制御信号・3Dスコープ
		case 1:
			data = 0xf8;
			data |= sysport.scope_3d;
			return data;

		// イメージユニット制御
		case 2:
			return 0xff;

		// キー制御・NMIリセット・HRL
		case 3:
			data = 0xf0;
			if (keyboard->IsConnect()) {
				data |= 0x08;
			}
			if (crtc->GetHRL()) {
				data |= 0x02;
			}
			return data;

		// ROM/DRAMウェイト制御(X68030)
		case 4:
			return 0xff;

		// MPU種別・クロック
		case 5:
			switch (memory->GetMemType()) {
				// 初代/ACE/EXPERT/PRO/SUPER
				case Memory::SASI:
				case Memory::SCSIInt:
				case Memory::SCSIExt:
					// 未定義ポートであったので、0xffが読み出せる
					return 0xff;

				// XVI/Compact
				case Memory::XVI:
				case Memory::Compact:
					// 逆転の発想で、下位4bitを動作クロックに割り当てた
					// 1111:10MHz
					// 1110:16MHz
					// 1101:20MHz
					// 1100:25MHz
					// 1011:33MHz
					// 1010:40MHz
					// 1001:50MHz
					return 0xfe;

				// X68030
				case Memory::X68030:
					// さらに、上位4bitをMPU種別に割り当てた
					// 1111:68000
					// 1110:68020
					// 1101:68030
					// 1100:68040
					return 0xdc;

				// その他(ありえない)
				default:
					ASSERT(FALSE);
					break;
			}
			return 0xff;

		// SRAM書き込み許可
		case 6:
			// バージョン情報(XM6拡張)
			if (sysport.ver_count != 0) {
				return GetVR();
			}
			return 0xff;

		// POWER制御
		case 7:
			return 0xff;
	}

	// 通常、ここにはこない
	ASSERT(FALSE);
	return 0xff;
}

//---------------------------------------------------------------------------
//
//	ワード読み込み
//
//---------------------------------------------------------------------------
DWORD FASTCALL SysPort::ReadWord(DWORD addr)
{
	ASSERT(this);
	ASSERT((addr >= memdev.first) && (addr <= memdev.last));
	ASSERT((addr & 1) == 0);
	ASSERT_DIAG();

	return (0xff00 | ReadByte(addr + 1));
}

//---------------------------------------------------------------------------
//
//	バイト書き込み
//
//---------------------------------------------------------------------------
void FASTCALL SysPort::WriteByte(DWORD addr, DWORD data)
{
	ASSERT(this);
	ASSERT((addr >= memdev.first) && (addr <= memdev.last));
	ASSERT(data < 0x100);
	ASSERT_DIAG();

	// 4bitのみデコード
	addr &= 0x0f;

	// 奇数バイトのみデコード
	if ((addr & 1) == 0) {
		return;
	}
	addr >>= 1;

	// ウェイト
	scheduler->Wait(1);

	switch (addr) {
		// コントラスト
		case 0:
			data &= 0x0f;
			if (sysport.contrast != data) {
#if defined(SYSPORT_LOG)
				LOG1(Log::Normal, "コントラスト設定 %d", data);
#endif	// SYSPORT_LOG

				// 違っていたら設定
				sysport.contrast = data;
				render->SetContrast(data);
			}
			return;

		// ディスプレイ制御信号・3Dスコープ
		case 1:
			data &= 3;
			if (sysport.scope_3d != data) {
#if defined(SYSPORT_LOG)
				LOG1(Log::Normal, "3Dスコープ設定 %d", data);
#endif	// SYSPORT_LOG

				// 違っていたら設定
				sysport.scope_3d = data;
			}
			return;

		// キー制御・NMIリセット・HRL
		case 3:
			if (data & 0x08) {
#if defined(SYSPORT_LOG)
				LOG0(Log::Normal, "キーボード許可");
#endif	// SYSPORT_LOG
				keyboard->SendWait(FALSE);
			}
			else {
#if defined(SYSPORT_LOG)
				LOG0(Log::Normal, "キーボード禁止");
#endif	// SYSPORT_LOG
				keyboard->SendWait(TRUE);
			}
			if (data & 0x02) {
#if defined(SYSPORT_LOG)
				LOG0(Log::Normal, "HRL=1");
#endif	// SYSPORT_LOG
				crtc->SetHRL(TRUE);
			}
			else {
#if defined(SYSPORT_LOG)
				LOG0(Log::Normal, "HRL=0");
#endif	// SYSPORT_LOG
				crtc->SetHRL(FALSE);
			}
			return;

		// イメージユニット制御
		case 4:
#if defined(SYSPORT_LOG)
			LOG1(Log::Normal, "イメージユニット制御 $%02X", data & 0x1f);
#endif	// SYSPORT_LOG
			sysport.image_unit = data & 0x1f;
			return;

		// ROM/DRAMウェイト制御
		case 5:
			return;

		// SRAM書き込み許可
		case 6:
			// SRAM書き込み
			if (data == 0x31) {
				sram->WriteEnable(TRUE);
			}
			else {
				sram->WriteEnable(FALSE);
			}
			// バージョン判別(XM6拡張)
			if (data == 'X') {
#if defined(SYSPORT_LOG)
				LOG0(Log::Normal, "XM6バージョン取得開始");
#endif	// SYSPORT_LOG
				sysport.ver_count = 1;
			}
			else {
				sysport.ver_count = 0;
			}
			return;

		// 電源制御(00, 0F, 0Fの順で落ちる)
		case 7:
			data &= 0x0f;
			switch (sysport.power_count) {
				// アクセスなし
				case 0:
					if (data == 0x00) {
						sysport.power_count++;
					}
					else {
						sysport.power_count = 0;
					}
					break;

				// 00あり
				case 1:
					if (data == 0x0f) {
						sysport.power_count++;
					}
					else {
						sysport.power_count = 0;
					}
					break;

				// 00,0Fあり
				case 2:
					if (data == 0x0f) {
						sysport.power_count++;
						LOG0(Log::Normal, "電源OFF");
						vm->SetPower(FALSE);
					}
					else {
						sysport.power_count = 0;
					}
					break;

				// 00,0F,0F
				default:
					break;
			}
			return;

		default:
			break;
	}

	// 通常、ここにはこない
	ASSERT(FALSE);
	return;
}

//---------------------------------------------------------------------------
//
//	ワード書き込み
//
//---------------------------------------------------------------------------
void FASTCALL SysPort::WriteWord(DWORD addr, DWORD data)
{
	ASSERT(this);
	ASSERT((addr >= memdev.first) && (addr <= memdev.last));
	ASSERT((addr & 1) == 0);
	ASSERT(data < 0x10000);
	ASSERT_DIAG();

	WriteByte(addr + 1, (BYTE)data);
}

//---------------------------------------------------------------------------
//
//	読み込みのみ
//
//---------------------------------------------------------------------------
DWORD FASTCALL SysPort::ReadOnly(DWORD addr) const
{
	DWORD data;

	ASSERT(this);
	ASSERT((addr >= memdev.first) && (addr <= memdev.last));
	ASSERT_DIAG();

	// 4bitのみデコード
	addr &= 0x0f;

	// 奇数バイトのみデコード
	if ((addr & 1) == 0) {
		return 0xff;
	}

	switch (addr) {
		// コントラスト
		case 0:
			data = 0xf0;
			data |= sysport.contrast;
			return data;

		// ディスプレイ制御信号・3Dスコープ
		case 1:
			data = 0xf8;
			data |= sysport.scope_3d;
			return data;

		// イメージユニット制御
		case 2:
			return 0xff;

		// キー制御・NMIリセット・HRL
		case 3:
			data = 0xf0;
			if (keyboard->IsConnect()) {
				data |= 0x08;
			}
			if (crtc->GetHRL()) {
				data |= 0x02;
			}
			return data;

		// ROM/DRAMウェイト制御(X68030)
		case 4:
			return 0xff;

		// MPU種別・クロック
		case 5:
			switch (memory->GetMemType()) {
				// 初代/ACE/EXPERT/PRO/SUPER
				case Memory::SASI:
				case Memory::SCSIInt:
				case Memory::SCSIExt:
					// 未定義ポートであったので、0xffが読み出せる
					return 0xff;

				// XVI/Compact
				case Memory::XVI:
				case Memory::Compact:
					// 逆転の発想で、下位4bitを動作クロックに割り当てた
					// 1111:10MHz
					// 1110:16MHz
					// 1101:20MHz
					// 1100:25MHz
					// 1011:33MHz
					// 1010:40MHz
					// 1001:50MHz
					return 0xfe;

				// X68030
				case Memory::X68030:
					// さらに、上位4bitをMPU種別に割り当てた
					// 1111:68000
					// 1110:68020
					// 1101:68030
					// 1100:68040
					return 0xdc;

				// その他(ありえない)
				default:
					ASSERT(FALSE);
					break;
			}
			return 0xff;

		// SRAM書き込み許可
		case 6:
			return 0xff;

		// POWER制御
		case 7:
			return 0xff;
	}

	return 0xff;
}

//---------------------------------------------------------------------------
//
//	バージョン情報読み出し
//
//---------------------------------------------------------------------------
DWORD FASTCALL SysPort::GetVR()
{
	DWORD major;
	DWORD minor;

	ASSERT(this);
	ASSERT_DIAG();

	switch (sysport.ver_count) {
		// 'X'書き込み直後
		case 1:
			sysport.ver_count++;
			return '6';

		// メジャーバージョン
		case 2:
			vm->GetVersion(major, minor);
			sysport.ver_count++;
			return major;

		// マイナーバージョン
		case 3:
			vm->GetVersion(major, minor);
			sysport.ver_count = 0;
			return minor;

		// その他(ありえない)
		default:
			ASSERT(FALSE);
			break;
	}

	return 0xff;
}
