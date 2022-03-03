//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 ＰＩ．(ytanaka@ipc-tokai.or.jp)
//	[ I/Oコントローラ(IOSC-2) ]
//
//---------------------------------------------------------------------------

#include "os.h"
#include "xm6.h"
#include "cpu.h"
#include "vm.h"
#include "log.h"
#include "fileio.h"
#include "schedule.h"
#include "printer.h"
#include "iosc.h"

//===========================================================================
//
//	IOSC
//
//===========================================================================
//#define IOSC_LOG

//---------------------------------------------------------------------------
//
//	コンストラクタ
//
//---------------------------------------------------------------------------
IOSC::IOSC(VM *p) : MemDevice(p)
{
	// デバイスIDを初期化
	dev.id = MAKEID('I', 'O', 'S', 'C');
	dev.desc = "I/O Ctrl (IOSC-2)";

	// 開始アドレス、終了アドレス
	memdev.first = 0xe9c000;
	memdev.last = 0xe9dfff;

	// その他
	printer = NULL;
}

//---------------------------------------------------------------------------
//
//	初期化
//
//---------------------------------------------------------------------------
BOOL FASTCALL IOSC::Init()
{
	ASSERT(this);

	// 基本クラス
	if (!MemDevice::Init()) {
		return FALSE;
	}

	// プリンタを取得
	printer = (Printer*)vm->SearchDevice(MAKEID('P', 'R', 'N', ' '));
	ASSERT(printer);

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	クリーンアップ
//
//---------------------------------------------------------------------------
void FASTCALL IOSC::Cleanup()
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
void FASTCALL IOSC::Reset()
{
	LOG0(Log::Normal, "リセット");

	// ワーク初期化
	iosc.prt_int = FALSE;
	iosc.prt_en = FALSE;
	iosc.fdd_int = FALSE;
	iosc.fdd_en = FALSE;
	iosc.fdc_int = FALSE;
	iosc.fdc_en = FALSE;
	iosc.hdc_int = FALSE;
	iosc.hdc_en = FALSE;
	iosc.vbase = 0;

	// 要求中のベクタなし
	iosc.vector = -1;
}

//---------------------------------------------------------------------------
//
//	セーブ
//
//---------------------------------------------------------------------------
BOOL FASTCALL IOSC::Save(Fileio *fio, int /*ver*/)
{
	size_t sz;

	ASSERT(this);
	ASSERT(fio);
	LOG0(Log::Normal, "セーブ");

	// サイズをセーブ
	sz = sizeof(iosc_t);
	if (!fio->Write(&sz, sizeof(sz))) {
		return FALSE;
	}

	// 実体をセーブ
	if (!fio->Write(&iosc, sizeof(iosc))) {
		return FALSE;
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	ロード
//
//---------------------------------------------------------------------------
BOOL FASTCALL IOSC::Load(Fileio *fio, int /*ver*/)
{
	size_t sz;

	ASSERT(this);
	ASSERT(fio);
	LOG0(Log::Normal, "ロード");

	// サイズをロード、照合
	if (!fio->Read(&sz, sizeof(sz))) {
		return FALSE;
	}
	if (sz != sizeof(iosc_t)) {
		return FALSE;
	}

	// 実体をロード
	if (!fio->Read(&iosc, sizeof(iosc))) {
		return FALSE;
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	設定適用
//
//---------------------------------------------------------------------------
void FASTCALL IOSC::ApplyCfg(const Config* /*config*/)
{
	ASSERT(this);
	LOG0(Log::Normal, "設定適用");
}

//---------------------------------------------------------------------------
//
//	バイト読み込み
//
//---------------------------------------------------------------------------
DWORD FASTCALL IOSC::ReadByte(DWORD addr)
{
	DWORD data;

	ASSERT(this);
	ASSERT((addr >= memdev.first) && (addr <= memdev.last));

	// 16バイト単位でループ
	addr &= 0x0f;

	// 奇数アドレスのみデコードされている
	if ((addr & 1) != 0) {

		// ウェイト
		scheduler->Wait(2);

		// $E9C001 割り込みステータス
		if (addr == 1) {
			data = 0;
			if (iosc.fdc_int) {
				data |= 0x80;
			}
			if (iosc.fdd_int) {
				data |= 0x40;
			}
			if (iosc.hdc_int) {
				data |= 0x10;
			}
			if (iosc.hdc_en) {
				data |= 0x08;
			}
			if (iosc.fdc_en) {
				data |= 0x04;
			}
			if (iosc.fdd_en) {
				data |= 0x02;
			}
			if (iosc.prt_en) {
				data |= 0x01;
			}

			// プリンタはREADYの表示
			if (printer->IsReady()) {
				data |= 0x20;
			}

			// プリンタ割り込みはこの時点で降ろす
			if (iosc.prt_int) {
				iosc.prt_int = FALSE;
				IntChk();
			}
			return data;
		}

		// $E9C003 割り込みベクタ
		if (addr == 3) {
			return 0xff;
		}

		LOG1(Log::Warning, "未実装レジスタ読み込み $%06X", memdev.first + addr);
		return 0xff;
	}

	// バスエラーは発生しない
	return 0xff;
}

//---------------------------------------------------------------------------
//
//	ワード読み込み
//
//---------------------------------------------------------------------------
DWORD FASTCALL IOSC::ReadWord(DWORD addr)
{
	ASSERT(this);
	ASSERT((addr >= memdev.first) && (addr <= memdev.last));
	ASSERT((addr & 1) == 0);

	return (0xff00 | ReadByte(addr + 1));
}

//---------------------------------------------------------------------------
//
//	バイト書き込み
//
//---------------------------------------------------------------------------
void FASTCALL IOSC::WriteByte(DWORD addr, DWORD data)
{
	ASSERT(this);
	ASSERT((addr >= memdev.first) && (addr <= memdev.last));
	ASSERT(data < 0x100);

	// 16バイト単位でループ
	addr &= 0x0f;

	// 奇数アドレスのみデコードされている
	if ((addr & 1) != 0) {

		// ウェイト
		scheduler->Wait(2);

		// $E9C001 割り込みマスク
		if (addr == 1) {
			if (data & 0x01) {
				iosc.prt_en = TRUE;
			}
			else {
				iosc.prt_en = FALSE;
			}
			if (data & 0x02) {
				iosc.fdd_en = TRUE;
			}
			else {
				iosc.fdd_en = FALSE;
			}
			if (data & 0x04) {
				iosc.fdc_en = TRUE;
			}
			else {
				iosc.fdc_en = FALSE;
			}
			if (data & 0x08) {
				iosc.hdc_en = TRUE;
			}
			else {
				iosc.hdc_en = FALSE;
			}

			// 割り込みチェック(FORMULA 68K)
			IntChk();
			return;
		}

		// $E9C003 割り込みベクタ
		if (addr == 3) {
			data &= 0xfc;
			iosc.vbase &= 0x03;
			iosc.vbase |= data;

			LOG1(Log::Detail, "割り込みベクタベース $%02X", iosc.vbase);
			return;
		}

		// 未定義
		LOG2(Log::Warning, "未実装レジスタ書き込み $%06X <- $%02X",
										memdev.first + addr, data);
		return;
	}
}

//---------------------------------------------------------------------------
//
//	ワード書き込み
//
//---------------------------------------------------------------------------
void FASTCALL IOSC::WriteWord(DWORD addr, DWORD data)
{
	ASSERT(this);
	ASSERT((addr >= memdev.first) && (addr <= memdev.last));
	ASSERT((addr & 1) == 0);
	ASSERT(data < 0x10000);

	WriteByte(addr + 1, (BYTE)data);
}

//---------------------------------------------------------------------------
//
//	読み込みのみ
//
//---------------------------------------------------------------------------
DWORD FASTCALL IOSC::ReadOnly(DWORD addr) const
{
	DWORD data;

	ASSERT(this);
	ASSERT((addr >= memdev.first) && (addr <= memdev.last));

	// 16バイト単位でループ
	addr &= 0x0f;

	// 奇数アドレスのみデコードされている
	if ((addr & 1) != 0) {

		// $E9C001 割り込みステータス
		if (addr == 1) {
			data = 0;
			if (iosc.fdc_int) {
				data |= 0x80;
			}
			if (iosc.fdd_int) {
				data |= 0x40;
			}
			if (iosc.prt_int) {
				data |= 0x20;
			}
			if (iosc.hdc_int) {
				data |= 0x10;
			}
			if (iosc.hdc_en) {
				data |= 0x08;
			}
			if (iosc.fdc_en) {
				data |= 0x04;
			}
			if (iosc.fdd_en) {
				data |= 0x02;
			}
			if (iosc.prt_en) {
				data |= 0x01;
			}
			return data;
		}

		// $E9C003 割り込みベクタ
		if (addr == 3) {
			return 0xff;
		}

		// 未定義アドレス
		return 0xff;
	}

	// 偶数アドレス
	return 0xff;
}

//---------------------------------------------------------------------------
//
//	内部データ取得
//
//---------------------------------------------------------------------------
void FASTCALL IOSC::GetIOSC(iosc_t *buffer) const
{
	ASSERT(this);
	ASSERT(buffer);

	// 内部ワークをコピー
	*buffer = iosc;
}

//---------------------------------------------------------------------------
//
//	割り込みチェック
//
//---------------------------------------------------------------------------
void FASTCALL IOSC::IntChk()
{
	int v;

	ASSERT(this);

	// 割り込み無し
	v = -1;

	// プリンタ割り込み
	if (iosc.prt_int && iosc.prt_en) {
		v = iosc.vbase + 3;
	}

	// HDC割り込み
	if (iosc.hdc_int && iosc.hdc_en) {
		v = iosc.vbase + 2;
	}

	// FDD割り込み
	if (iosc.fdd_int && iosc.fdd_en) {
		v = iosc.vbase + 1;
	}

	// FDC割り込み
	if (iosc.fdc_int && iosc.fdc_en) {
		v = iosc.vbase;
	}

	// 要求したい割り込みがない場合
	if (v < 0) {
		// 割り込み要求中でなければ、OK
		if (iosc.vector < 0) {
			return;
		}

		// 何らかの割り込みを要求しているので、割り込みキャンセル
#if defined(IOSC_LOG)
		LOG1(Log::Normal, "割り込みキャンセル ベクタ$%02X", iosc.vector);
#endif	// IOSC_LOG
		cpu->IntCancel(1);
		iosc.vector = -1;
		return;
	}

	// 既に要求しているベクタと同じであれば、OK
	if (iosc.vector == v) {
		return;
	}

	// 違うので、要求中なら一度キャンセル
	if (iosc.vector >= 0) {
#if defined(IOSC_LOG)
		LOG1(Log::Normal, "割り込みキャンセル ベクタ$%02X", iosc.vector);
#endif	// IOSC_LOG
		cpu->IntCancel(1);
		iosc.vector = -1;
	}

	// 割り込み要求
#if defined(IOSC_LOG)
	LOG1(Log::Normal, "割り込み要求 ベクタ$%02X", v);
#endif	// IOSC_LOG
	cpu->Interrupt(1, (BYTE)v);

	// 記憶
	iosc.vector = v;
}

//---------------------------------------------------------------------------
//
//	割り込み応答
//
//---------------------------------------------------------------------------
void FASTCALL IOSC::IntAck()
{
	ASSERT(this);

	// リセット直後に、CPUから割り込みが間違って入る場合がある
	// または他のデバイス
	if (iosc.vector < 0) {
#if defined(IOSC_LOG)
		LOG0(Log::Warning, "要求していない割り込み");
#endif	// IOSC_LOG
		return;
	}

#if defined(IOSC_LOG)
	LOG1(Log::Normal, "割り込みACK ベクタ$%02X", iosc.vector);
#endif	// IOSC_LOG

	// 割り込みなし
	iosc.vector = -1;
}

//---------------------------------------------------------------------------
//
//	FDC割り込み
//
//---------------------------------------------------------------------------
void FASTCALL IOSC::IntFDC(BOOL flag)
{
	ASSERT(this);

	iosc.fdc_int = flag;

	// 割り込みチェック
	IntChk();
}

//---------------------------------------------------------------------------
//
//	FDD割り込み
//
//---------------------------------------------------------------------------
void FASTCALL IOSC::IntFDD(BOOL flag)
{
	ASSERT(this);

	iosc.fdd_int = flag;

	// 割り込みチェック
	IntChk();
}

//---------------------------------------------------------------------------
//
//	HDC割り込み
//
//---------------------------------------------------------------------------
void FASTCALL IOSC::IntHDC(BOOL flag)
{
	ASSERT(this);

	iosc.hdc_int = flag;

	// 割り込みチェック
	IntChk();
}

//---------------------------------------------------------------------------
//
//	プリンタ割り込み
//
//---------------------------------------------------------------------------
void FASTCALL IOSC::IntPRT(BOOL flag)
{
	ASSERT(this);

	iosc.prt_int = flag;

	// 割り込みチェック
	IntChk();
}
