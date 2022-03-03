//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 ＰＩ．(ytanaka@ipc-tokai.or.jp)
//	[ ビデオコントローラ(CATHY & VIPS) ]
//
//---------------------------------------------------------------------------

#include "os.h"
#include "xm6.h"
#include "vm.h"
#include "log.h"
#include "schedule.h"
#include "fileio.h"
#include "render.h"
#include "renderin.h"
#include "vc.h"

//===========================================================================
//
//	ビデオコントローラ
//
//===========================================================================
//#define VC_LOG

//---------------------------------------------------------------------------
//
//	コンストラクタ
//
//---------------------------------------------------------------------------
VC::VC(VM *p) : MemDevice(p)
{
	// デバイスIDを初期化
	dev.id = MAKEID('V', 'C', ' ', ' ');
	dev.desc = "VC (CATHY & VIPS)";

	// 開始アドレス、終了アドレス
	memdev.first = 0xe82000;
	memdev.last = 0xe83fff;

	// その他
	render = NULL;
}

//---------------------------------------------------------------------------
//
//	初期化
//
//---------------------------------------------------------------------------
BOOL FASTCALL VC::Init()
{
	ASSERT(this);

	// 基本クラス
	if (!MemDevice::Init()) {
		return FALSE;
	}

	// レンダラ取得
	render = (Render*)vm->SearchDevice(MAKEID('R', 'E', 'N', 'D'));
	ASSERT(render);

	// パレットワークをクリア
	memset(palette, 0, sizeof(palette));

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	クリーンアップ
//
//---------------------------------------------------------------------------
void FASTCALL VC::Cleanup()
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
void FASTCALL VC::Reset()
{
	ASSERT(this);
	LOG0(Log::Normal, "リセット");

	// ビデオワークをクリア
	memset(&vc, 0, sizeof(vc));

	// 記憶用ワークを確実に反転させる
	vc.vr1h = 0xff;
	vc.vr1l = 0xff;
	vc.vr2h = 0xff;
	vc.vr2l = 0xff;
}

//---------------------------------------------------------------------------
//
//	セーブ
//
//---------------------------------------------------------------------------
BOOL FASTCALL VC::Save(Fileio *fio, int /*ver*/)
{
	size_t sz;

	ASSERT(this);
	ASSERT(fio);

	LOG0(Log::Normal, "セーブ");

	// サイズをセーブ
	sz = sizeof(vc_t);
	if (!fio->Write(&sz, (int)sizeof(sz))) {
		return FALSE;
	}

	// 実体をセーブ
	if (!fio->Write(&vc, (int)sz)) {
		return FALSE;
	}

	// パレットをセーブ
	if (!fio->Write(palette, sizeof(palette))) {
		return FALSE;
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	ロード
//
//---------------------------------------------------------------------------
BOOL FASTCALL VC::Load(Fileio *fio, int /*ver*/)
{
	size_t sz;
	DWORD addr;

	ASSERT(this);
	ASSERT(fio);

	LOG0(Log::Normal, "ロード");

	// サイズをロード、照合
	if (!fio->Read(&sz, (int)sizeof(sz))) {
		return FALSE;
	}
	if (sz != sizeof(vc_t)) {
		return FALSE;
	}

	// 実体をロード
	if (!fio->Read(&vc, (int)sz)) {
		return FALSE;
	}

	// パレットをロード
	if (!fio->Read(palette, sizeof(palette))) {
		return FALSE;
	}

	// レンダラへ通知
	render->SetVC();
	for (addr=0; addr<0x200; addr++) {
		render->SetPalette(addr);
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	設定適用
//
//---------------------------------------------------------------------------
void FASTCALL VC::ApplyCfg(const Config *config)
{
	ASSERT(config);
	LOG0(Log::Normal, "設定適用");
}

//---------------------------------------------------------------------------
//
//	バイト読み込み
//
//---------------------------------------------------------------------------
DWORD FASTCALL VC::ReadByte(DWORD addr)
{
	ASSERT(this);
	ASSERT((addr >= memdev.first) && (addr <= memdev.last));

	// $1000単位でループ
	addr &= 0xfff;

	// デコード
	if (addr < 0x400) {
		// パレットエリア
		scheduler->Wait(1);
		addr ^= 1;
		return palette[addr];
	}

	// ビデオコントローラレジスタ
	if (addr < 0x500) {
		if (addr & 1) {
			return (BYTE)GetVR0();
		}
		else {
			return (GetVR0() >> 8);
		}
	}
	if (addr < 0x600) {
		if (addr & 1) {
			return (BYTE)GetVR1();
		}
		else {
			return (GetVR1() >> 8);
		}
	}
	if (addr < 0x700) {
		if (addr & 1) {
			return (BYTE)GetVR2();
		}
		else {
			return (GetVR2() >> 8);
		}
	}

	// デコードされていないエリアは0
	return 0;
}

//---------------------------------------------------------------------------
//
//	ワード読み込み
//
//---------------------------------------------------------------------------
DWORD FASTCALL VC::ReadWord(DWORD addr)
{
	ASSERT(this);
	ASSERT((addr >= memdev.first) && (addr <= memdev.last));
	ASSERT((addr & 1) == 0);

	// $1000単位でループ
	addr &= 0xfff;

	// デコード
	if (addr < 0x400) {
		// パレット
		scheduler->Wait(1);
		return *(WORD *)(&palette[addr]);
	}

	// ビデオコントローラレジスタ
	if (addr < 0x500) {
		return GetVR0();
	}
	if (addr < 0x600) {
		return GetVR1();
	}
	if (addr < 0x700) {
		return GetVR2();
	}

	// デコードされていないエリアは0
	return 0;
}

//---------------------------------------------------------------------------
//
//	バイト書き込み
//
//---------------------------------------------------------------------------
void FASTCALL VC::WriteByte(DWORD addr, DWORD data)
{
	ASSERT(this);
	ASSERT((addr >= memdev.first) && (addr <= memdev.last));
	ASSERT(data < 0x100);

#if defined(VC_LOG)
	if ((addr & 0xfff) >= 0x400) {
		LOG2(Log::Normal, "VC書き込み %08X <- %02X", addr, data);
	}
#endif	// VC_LOG

	// $1000単位でループ
	addr &= 0xfff;

	// デコード
	if (addr < 0x400) {
		// パレットエリア
		scheduler->Wait(1);
		addr ^= 1;

		// 比較
		if (palette[addr] != data) {
			palette[addr] = (BYTE)data;

			// レンダラへ通知
			render->SetPalette(addr >> 1);
		}
		return;
	}

	// ビデオコントローラレジスタ
	if (addr < 0x500) {
		if (addr & 1) {
			SetVR0L(data);
		}
		return;
	}
	if (addr < 0x600) {
		if (addr & 1) {
			SetVR1L(data);
		}
		else {
			SetVR1H(data);
		}
		return;
	}
	if (addr < 0x700) {
		if (addr & 1) {
			SetVR2L(data);
		}
		else {
			SetVR2H(data);
		}
		return;
	}

	// それ以外はデコードされていない
}

//---------------------------------------------------------------------------
//
//	ワード書き込み
//
//---------------------------------------------------------------------------
void FASTCALL VC::WriteWord(DWORD addr, DWORD data)
{
	ASSERT(this);
	ASSERT((addr >= memdev.first) && (addr <= memdev.last));
	ASSERT((addr & 1) == 0);
	ASSERT(data < 0x10000);

#if defined(VC_LOG)
	if ((addr & 0xfff) >= 0x400) {
		LOG2(Log::Normal, "VC書き込み %08X <- %04X", addr, data);
	}
#endif	// VC_LOG

	// $1000単位でループ
	addr &= 0xfff;

	// デコード
	if (addr < 0x400) {
		// パレットエリア
		scheduler->Wait(1);

		// 比較
		if (data != *(WORD*)(&palette[addr])) {
			*(WORD *)(&palette[addr]) = (WORD)data;

			// レンダラへ通知
			render->SetPalette(addr >> 1);
		}
		return;
	}

	// ビデオコントローラレジスタ
	if (addr < 0x500) {
		SetVR0L((BYTE)data);
		return;
	}
	if (addr < 0x600) {
		SetVR1L((BYTE)data);
		SetVR1H(data >> 8);
		return;
	}
	if (addr < 0x700) {
		SetVR2L((BYTE)data);
		SetVR2H(data >> 8);
		return;
	}

	// それ以外はデコードされていない
}

//---------------------------------------------------------------------------
//
//	読み込みのみ
//
//---------------------------------------------------------------------------
DWORD FASTCALL VC::ReadOnly(DWORD addr) const
{
	ASSERT(this);
	ASSERT((addr >= memdev.first) && (addr <= memdev.last));

	// $1000単位でループ
	addr &= 0xfff;

	// デコード
	if (addr < 0x400) {
		// パレットエリア
		addr ^= 1;
		return palette[addr];
	}

	// ビデオコントローラレジスタ
	if (addr < 0x500) {
		if (addr & 1) {
			return (BYTE)GetVR0();
		}
		else {
			return (GetVR0() >> 8);
		}
	}
	if (addr < 0x600) {
		if (addr & 1) {
			return (BYTE)GetVR1();
		}
		else {
			return (GetVR1() >> 8);
		}
	}
	if (addr < 0x700) {
		if (addr & 1) {
			return (BYTE)GetVR2();
		}
		else {
			return (GetVR2() >> 8);
		}
	}

	// デコードされていないエリアは0
	return 0;
}

//---------------------------------------------------------------------------
//
//	内部データ取得
//
//---------------------------------------------------------------------------
void FASTCALL VC::GetVC(vc_t *buffer)
{
	ASSERT(this);
	ASSERT(buffer);

	// 内部ワークをコピー
	*buffer = vc;
}

//---------------------------------------------------------------------------
//
//	ビデオレジスタ0(L)設定
//
//---------------------------------------------------------------------------
void FASTCALL VC::SetVR0L(DWORD data)
{
	BOOL siz;
	DWORD col;

	ASSERT(this);
	ASSERT(data < 0x100);

	// 記憶
	siz = vc.siz;
	col = vc.col;

	// 設定
	if (data & 4) {
		vc.siz = TRUE;
	}
	else {
		vc.siz = FALSE;
	}
	vc.col = (data & 3);

	// 比較
	if ((vc.siz != siz) || (vc.col != col)) {
		render->SetVC();
	}
}

//---------------------------------------------------------------------------
//
//	ビデオレジスタ0取得
//
//---------------------------------------------------------------------------
DWORD FASTCALL VC::GetVR0() const
{
	DWORD data;

	ASSERT(this);

	data = 0;
	if (vc.siz) {
		data |= 0x04;
	}
	data |= vc.col;

	return data;
}

//---------------------------------------------------------------------------
//
//	ビデオレジスタ1(H)設定
//
//---------------------------------------------------------------------------
void FASTCALL VC::SetVR1H(DWORD data)
{
	ASSERT(this);
	ASSERT(data < 0x100);

	data &= 0x3f;

	// 比較
	if (vc.vr1h == data) {
		return;
	}
	vc.vr1h = data;

	vc.gr = (data & 3);
	data >>= 2;
	vc.tx = (data & 3);
	data >>= 2;
	vc.sp = data;

	// 通知
	render->SetVC();
}

//---------------------------------------------------------------------------
//
//	ビデオレジスタ1(L)設定
//
//---------------------------------------------------------------------------
void FASTCALL VC::SetVR1L(DWORD data)
{
	ASSERT(this);
	ASSERT(data < 0x100);

	// 比較
	if (vc.vr1l == data) {
		return;
	}
	vc.vr1l = data;

	vc.gp[0] = (data & 3);
	data >>= 2;
	vc.gp[1] = (data & 3);
	data >>= 2;
	vc.gp[2] = (data & 3);
	data >>= 2;
	vc.gp[3] = (data & 3);

	// 通知
	render->SetVC();
}

//---------------------------------------------------------------------------
//
//	ビデオレジスタ1取得
//
//---------------------------------------------------------------------------
DWORD FASTCALL VC::GetVR1() const
{
	DWORD data;

	ASSERT(this);

	data = vc.sp;
	data <<= 2;
	data |= vc.tx;
	data <<= 2;
	data |= vc.gr;
	data <<= 2;
	data |= vc.gp[3];
	data <<= 2;
	data |= vc.gp[2];
	data <<= 2;
	data |= vc.gp[1];
	data <<= 2;
	data |= vc.gp[0];

	return data;
}

//---------------------------------------------------------------------------
//
//	ビデオレジスタ2(H)設定
//
//---------------------------------------------------------------------------
void FASTCALL VC::SetVR2H(DWORD data)
{
	ASSERT(this);
	ASSERT(data < 0x100);

	// データ比較
	if (vc.vr2h == data) {
		return;
	}
	vc.vr2h = data;

	// YS
	if (data & 0x80) {
		vc.ys = TRUE;
	}
	else {
		vc.ys = FALSE;
	}

	// AH
	if (data & 0x40) {
		vc.ah = TRUE;
	}
	else {
		vc.ah = FALSE;
	}

	// VHT
	if (data & 0x20) {
		vc.vht = TRUE;
	}
	else {
		vc.vht = FALSE;
	}

	// EXON
	if (data & 0x10) {
		vc.exon = TRUE;
	}
	else {
		vc.exon = FALSE;
	}

	// H/P
	if (data & 0x08) {
		vc.hp = TRUE;
	}
	else {
		vc.hp = FALSE;
	}

	// B/P
	if (data & 0x04) {
		vc.bp = TRUE;
	}
	else {
		vc.bp = FALSE;
	}

	// G/G
	if (data & 0x02) {
		vc.gg = TRUE;
	}
	else {
		vc.gg = FALSE;
	}

	// G/T
	if (data & 0x01) {
		vc.gt = TRUE;
	}
	else {
		vc.gt = FALSE;
	}

	// 通知
	render->SetVC();
}

//---------------------------------------------------------------------------
//
//	ビデオレジスタ2(L)設定
//
//---------------------------------------------------------------------------
void FASTCALL VC::SetVR2L(DWORD data)
{
	ASSERT(this);
	ASSERT(data < 0x100);

	// 比較
	if (vc.vr2l == data) {
		return;
	}
	vc.vr2l = data;

	// BCON
	if (data & 0x80) {
		vc.bcon = TRUE;
	}
	else {
		vc.bcon = FALSE;
	}

	// SON
	if (data & 0x40) {
		vc.son = TRUE;
	}
	else {
		vc.son = FALSE;
	}

	// TON
	if (data & 0x20) {
		vc.ton = TRUE;
	}
	else {
		vc.ton = FALSE;
	}

	// GON
	if (data & 0x10) {
		vc.gon = TRUE;
	}
	else {
		vc.gon = FALSE;
	}

	// GS[3]
	if (data & 0x08) {
		vc.gs[3] = TRUE;
	}
	else {
		vc.gs[3] = FALSE;
	}

	// GS[2]
	if (data & 0x04) {
		vc.gs[2] = TRUE;
	}
	else {
		vc.gs[2] = FALSE;
	}

	// GS[1]
	if (data & 0x02) {
		vc.gs[1] = TRUE;
	}
	else {
		vc.gs[1] = FALSE;
	}

	// GS[0]
	if (data & 0x01) {
		vc.gs[0] = TRUE;
	}
	else {
		vc.gs[0] = FALSE;
	}

	// 通知
	render->SetVC();
}

//---------------------------------------------------------------------------
//
//	ビデオレジスタ2取得
//
//---------------------------------------------------------------------------
DWORD FASTCALL VC::GetVR2() const
{
	DWORD data;

	ASSERT(this);

	data = 0;
	if (vc.ys) {
		data |= 0x8000;
	}
	if (vc.ah) {
		data |= 0x4000;
	}
	if (vc.vht) {
		data |= 0x2000;
	}
	if (vc.exon) {
		data |= 0x1000;
	}
	if (vc.hp) {
		data |= 0x0800;
	}
	if (vc.bp) {
		data |= 0x0400;
	}
	if (vc.gg) {
		data |= 0x0200;
	}
	if (vc.gt) {
		data |= 0x0100;
	}
	if (vc.bcon) {
		data |= 0x0080;
	}
	if (vc.son) {
		data |= 0x0040;
	}
	if (vc.ton) {
		data |= 0x0020;
	}
	if (vc.gon) {
		data |= 0x0010;
	}
	if (vc.gs[3]) {
		data |= 0x0008;
	}
	if (vc.gs[2]) {
		data |= 0x0004;
	}
	if (vc.gs[1]) {
		data |= 0x0002;
	}
	if (vc.gs[0]) {
		data |= 0x0001;
	}

	return data;
}
