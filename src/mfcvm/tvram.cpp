//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 ＰＩ．(ytanaka@ipc-tokai.or.jp)
//	[ テキストVRAM ]
//
//---------------------------------------------------------------------------

#include "os.h"
#include "xm6.h"
#include "vm.h"
#include "log.h"
#include "fileio.h"
#include "schedule.h"
#include "render.h"
#include "renderin.h"
#include "tvram.h"

//===========================================================================
//
//	テキストVRAMハンドラ
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	コンストラクタ
//
//---------------------------------------------------------------------------
TVRAMHandler::TVRAMHandler(Render *rend, BYTE *mem)
{
	ASSERT(rend);
	ASSERT(mem);

	// 記憶
	render = rend;
	tvram = mem;

	// ワークを初期化
	multi = 0;
	mask = 0;
	rev = 0;
	maskh = 0;
	revh = 0;
}

//===========================================================================
//
//	テキストVRAMハンドラ(ノーマル)
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	コンストラクタ
//
//---------------------------------------------------------------------------
TVRAMNormal::TVRAMNormal(Render *rend, BYTE *mem) : TVRAMHandler(rend, mem)
{
}

//---------------------------------------------------------------------------
//
//	バイト書き込み
//
//---------------------------------------------------------------------------
void FASTCALL TVRAMNormal::WriteByte(DWORD addr, DWORD data)
{
	ASSERT(this);
	ASSERT(addr < 0x80000);
	ASSERT(data < 0x100);

	if (tvram[addr] != data) {
		tvram[addr] = (BYTE)data;
		render->TextMem(addr);
	}
}

//---------------------------------------------------------------------------
//
//	ワード書き込み
//
//---------------------------------------------------------------------------
void FASTCALL TVRAMNormal::WriteWord(DWORD addr, DWORD data)
{
	ASSERT(this);
	ASSERT(addr < 0x80000);
	ASSERT(data < 0x10000);

	if ((DWORD)*(WORD*)(&tvram[addr]) != data) {
		*(WORD*)(&tvram[addr]) = (WORD)data;
		render->TextMem(addr);
	}
}

//===========================================================================
//
//	テキストVRAMハンドラ(マスク)
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	コンストラクタ
//
//---------------------------------------------------------------------------
TVRAMMask::TVRAMMask(Render *rend, BYTE *mem) : TVRAMHandler(rend, mem)
{
}

//---------------------------------------------------------------------------
//
//	バイト書き込み
//
//---------------------------------------------------------------------------
void FASTCALL TVRAMMask::WriteByte(DWORD addr, DWORD data)
{
	DWORD mem;

	ASSERT(this);
	ASSERT(addr < 0x80000);
	ASSERT(data < 0x100);

	// maskで1は変更しない、0は変更する
	mem = (DWORD)tvram[addr];
	if (addr & 1) {
		// 68000では偶数アドレス→b15-b8を使う
		mem &= maskh;
		data &= revh;
	}
	else {
		// 68000では奇数アドレス→b7-b0を使う
		mem &= mask;
		data &= rev;
	}

	// 合成
	data |= mem;

	// 書き込み
	if (tvram[addr] != data) {
		tvram[addr] = (BYTE)data;
		render->TextMem(addr);
	}
}

//---------------------------------------------------------------------------
//
//	ワード書き込み
//
//---------------------------------------------------------------------------
void FASTCALL TVRAMMask::WriteWord(DWORD addr, DWORD data)
{
	DWORD mem;

	ASSERT(this);
	ASSERT(addr < 0x80000);
	ASSERT(data < 0x10000);

	// maskで1は変更しない、0は変更する
	mem = (DWORD)*(WORD*)(&tvram[addr]);
	mem &= mask;
	data &= rev;

	// 合成
	data |= mem;

	if ((DWORD)*(WORD*)(&tvram[addr]) != data) {
		*(WORD*)(&tvram[addr]) = (WORD)data;
		render->TextMem(addr);
	}
}

//===========================================================================
//
//	テキストVRAMハンドラ(マルチ)
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	コンストラクタ
//
//---------------------------------------------------------------------------
TVRAMMulti::TVRAMMulti(Render *rend, BYTE *mem) : TVRAMHandler(rend, mem)
{
}

//---------------------------------------------------------------------------
//
//	バイト書き込み
//
//---------------------------------------------------------------------------
void FASTCALL TVRAMMulti::WriteByte(DWORD addr, DWORD data)
{
	BOOL flag;

	ASSERT(this);
	ASSERT(addr < 0x80000);
	ASSERT(data < 0x100);

	// 初期化
	addr &= 0x1ffff;
	flag = FALSE;

	// プレーンB
	if (multi & 1) {
		if (tvram[addr] != data) {
			tvram[addr] = (BYTE)data;
			flag = TRUE;
		}
	}
	addr += 0x20000;

	// プレーンG
	if (multi & 2) {
		if (tvram[addr] != data) {
			tvram[addr] = (BYTE)data;
			flag = TRUE;
		}
	}
	addr += 0x20000;

	// プレーンR
	if (multi & 4) {
		if (tvram[addr] != data) {
			tvram[addr] = (BYTE)data;
			flag = TRUE;
		}
	}
	addr += 0x20000;

	// プレーンI
	if (multi & 8) {
		if (tvram[addr] != data) {
			tvram[addr] = (BYTE)data;
			flag = TRUE;
		}
	}

	// レンダラへ通知
	if (flag) {
		render->TextMem(addr);
	}
}

//---------------------------------------------------------------------------
//
//	ワード書き込み
//
//---------------------------------------------------------------------------
void FASTCALL TVRAMMulti::WriteWord(DWORD addr, DWORD data)
{
	BOOL flag;

	ASSERT(this);
	ASSERT(addr < 0x80000);
	ASSERT(data < 0x10000);

	// 初期化
	addr &= 0x1fffe;
	flag = FALSE;

	// プレーンB
	if (multi & 1) {
		if ((DWORD)*(WORD*)(&tvram[addr]) != data) {
			*(WORD*)(&tvram[addr]) = (WORD)data;
			flag = TRUE;
		}
	}
	addr += 0x20000;

	// プレーンG
	if (multi & 2) {
		if ((DWORD)*(WORD*)(&tvram[addr]) != data) {
			*(WORD*)(&tvram[addr]) = (WORD)data;
			flag = TRUE;
		}
	}
	addr += 0x20000;

	// プレーンR
	if (multi & 4) {
		if ((DWORD)*(WORD*)(&tvram[addr]) != data) {
			*(WORD*)(&tvram[addr]) = (WORD)data;
			flag = TRUE;
		}
	}
	addr += 0x20000;

	// プレーンI
	if (multi & 8) {
		if ((DWORD)*(WORD*)(&tvram[addr]) != data) {
			*(WORD*)(&tvram[addr]) = (WORD)data;
			flag = TRUE;
		}
	}

	// レンダラへ通知
	if (flag) {
		render->TextMem(addr);
	}
}

//===========================================================================
//
//	テキストVRAMハンドラ(両方)
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	コンストラクタ
//
//---------------------------------------------------------------------------
TVRAMBoth::TVRAMBoth(Render *rend, BYTE *mem) : TVRAMHandler(rend, mem)
{
}

//---------------------------------------------------------------------------
//
//	バイト書き込み
//
//---------------------------------------------------------------------------
void FASTCALL TVRAMBoth::WriteByte(DWORD addr, DWORD data)
{
	DWORD mem;
	DWORD maskhl;
	BOOL flag;

	ASSERT(this);
	ASSERT(addr < 0x80000);
	ASSERT(data < 0x100);

	// 偶数・奇数の判定は先に済ませる
	if (addr & 1) {
		maskhl = maskh;
		data &= revh;
	}
	else {
		maskhl = mask;
		data &= rev;
	}

	// 初期化
	addr &= 0x1ffff;
	flag = FALSE;

	// プレーンB
	if (multi & 1) {
		mem = (DWORD)tvram[addr];
		mem &= maskhl;
		mem |= data;

		if (tvram[addr] != mem) {
			tvram[addr] = (BYTE)mem;
			flag = TRUE;
		}
	}
	addr += 0x20000;

	// プレーンG
	if (multi & 2) {
		mem = (DWORD)tvram[addr];
		mem &= maskhl;
		mem |= data;

		if (tvram[addr] != mem) {
			tvram[addr] = (BYTE)mem;
			flag = TRUE;
		}
	}
	addr += 0x20000;

	// プレーンR
	if (multi & 4) {
		mem = (DWORD)tvram[addr];
		mem &= maskhl;
		mem |= data;

		if (tvram[addr] != mem) {
			tvram[addr] = (BYTE)mem;
			flag = TRUE;
		}
	}
	addr += 0x20000;

	// プレーンI
	if (multi & 8) {
		mem = (DWORD)tvram[addr];
		mem &= maskhl;
		mem |= data;

		if (tvram[addr] != mem) {
			tvram[addr] = (BYTE)mem;
			flag = TRUE;
		}
	}

	// レンダラへ通知
	if (flag) {
		render->TextMem(addr);
	}
}

//---------------------------------------------------------------------------
//
//	ワード書き込み
//
//---------------------------------------------------------------------------
void FASTCALL TVRAMBoth::WriteWord(DWORD addr, DWORD data)
{
	DWORD mem;
	BOOL flag;

	ASSERT(this);
	ASSERT(addr < 0x80000);
	ASSERT(data < 0x10000);

	// データは先にマスクしておく
	data &= rev;

	// 初期化
	addr &= 0x1fffe;
	flag = FALSE;

	// プレーンB
	if (multi & 1) {
		mem = (DWORD)*(WORD*)(&tvram[addr]);
		mem &= mask;
		mem |= data;

		if ((DWORD)*(WORD*)(&tvram[addr]) != mem) {
			*(WORD*)(&tvram[addr]) = (WORD)mem;
			flag = TRUE;
		}
	}
	addr += 0x20000;

	// プレーンG
	if (multi & 2) {
		mem = (DWORD)*(WORD*)(&tvram[addr]);
		mem &= mask;
		mem |= data;

		if ((DWORD)*(WORD*)(&tvram[addr]) != mem) {
			*(WORD*)(&tvram[addr]) = (WORD)mem;
			flag = TRUE;
		}
	}
	addr += 0x20000;

	// プレーンR
	if (multi & 4) {
		mem = (DWORD)*(WORD*)(&tvram[addr]);
		mem &= mask;
		mem |= data;

		if ((DWORD)*(WORD*)(&tvram[addr]) != mem) {
			*(WORD*)(&tvram[addr]) = (WORD)mem;
			flag = TRUE;
		}
	}
	addr += 0x20000;

	// プレーンI
	if (multi & 8) {
		mem = (DWORD)*(WORD*)(&tvram[addr]);
		mem &= mask;
		mem |= data;

		if ((DWORD)*(WORD*)(&tvram[addr]) != mem) {
			*(WORD*)(&tvram[addr]) = (WORD)mem;
			flag = TRUE;
		}
	}

	// レンダラへ通知
	if (flag) {
		render->TextMem(addr);
	}
}

//===========================================================================
//
//	テキストVRAM
//
//===========================================================================
//#define TVRAM_LOG

//---------------------------------------------------------------------------
//
//	コンストラクタ
//
//---------------------------------------------------------------------------
TVRAM::TVRAM(VM *p) : MemDevice(p)
{
	// デバイスIDを初期化
	dev.id = MAKEID('T', 'V', 'R', 'M');
	dev.desc = "Text VRAM";

	// 開始アドレス、終了アドレス
	memdev.first = 0xe00000;
	memdev.last = 0xe7ffff;

	// ハンドラ
	normal = NULL;
	mask = NULL;
	multi = NULL;
	both = NULL;

	// その他
	render = NULL;
	tvram = NULL;
}

//---------------------------------------------------------------------------
//
//	初期化
//
//---------------------------------------------------------------------------
BOOL FASTCALL TVRAM::Init()
{
	ASSERT(this);

	// 基本クラス
	if (!MemDevice::Init()) {
		return FALSE;
	}

	// レンダラ取得
	render = (Render*)vm->SearchDevice(MAKEID('R', 'E', 'N', 'D'));
	ASSERT(render);

	// メモリ確保、クリア
	try {
		tvram = new BYTE[ 0x80000 ];
	}
	catch (...) {
		return FALSE;
	}
	if (!tvram) {
		return FALSE;
	}
	memset(tvram, 0, 0x80000);

	// ハンドラ作成
	normal = new TVRAMNormal(render, tvram);
	mask = new TVRAMMask(render, tvram);
	multi = new TVRAMMulti(render, tvram);
	both = new TVRAMBoth(render, tvram);
	handler = normal;

	// ワークエリア初期化
	tvdata.multi = 0;
	tvdata.mask = 0;
	tvdata.rev = 0xffffffff;
	tvdata.maskh = 0;
	tvdata.revh = 0xffffffff;
	tvdata.src = 0;
	tvdata.dst = 0;
	tvdata.plane = 0;
	tvcount = 0;

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	クリーンアップ
//
//---------------------------------------------------------------------------
void FASTCALL TVRAM::Cleanup()
{
	ASSERT(this);

	// ハンドラ削除
	if (both) {
		delete both;
		both = NULL;
	}
	if (multi) {
		delete multi;
		multi = NULL;
	}
	if (mask) {
		delete mask;
		mask = NULL;
	}
	if (normal) {
		delete normal;
		normal = NULL;
	}
	handler = NULL;

	// メモリ解放
	delete[] tvram;
	tvram = NULL;

	// 基本クラスへ
	MemDevice::Cleanup();
}

//---------------------------------------------------------------------------
//
//	リセット
//
//---------------------------------------------------------------------------
void FASTCALL TVRAM::Reset()
{
	ASSERT(this);
	ASSERT_DIAG();

	LOG0(Log::Normal, "リセット");

	// ワークエリア初期化
	tvdata.multi = 0;
	tvdata.mask = 0;
	tvdata.rev = 0xffffffff;
	tvdata.maskh = 0;
	tvdata.revh = 0xffffffff;
	tvdata.src = 0;
	tvdata.dst = 0;
	tvdata.plane = 0;

	// アクセスカウント0
	tvcount = 0;

	// ハンドラはノーマル
	handler = normal;
	ASSERT(handler);
}

//---------------------------------------------------------------------------
//
//	セーブ
//
//---------------------------------------------------------------------------
BOOL FASTCALL TVRAM::Save(Fileio *fio, int /*ver*/)
{
	size_t sz;

	ASSERT(this);
	ASSERT(fio);
	ASSERT_DIAG();

	LOG0(Log::Normal, "セーブ");

	// メモリをセーブ
	if (!fio->Write(tvram, 0x80000)) {
		return FALSE;
	}

	// サイズをセーブ
	sz = sizeof(tvram_t);
	if (!fio->Write(&sz, sizeof(sz))) {
		return FALSE;
	}

	// 実体をセーブ
	if (!fio->Write(&tvdata, (int)sz)) {
		return FALSE;
	}

	// tvcount(version2.04で追加)
	if (!fio->Write(&tvcount, sizeof(tvcount))) {
		return FALSE;
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	ロード
//
//---------------------------------------------------------------------------
BOOL FASTCALL TVRAM::Load(Fileio *fio, int ver)
{
	size_t sz;
	DWORD addr;

	ASSERT(this);
	ASSERT(fio);
	ASSERT(ver >= 0x0200);
	ASSERT_DIAG();

	LOG0(Log::Normal, "ロード");

	// メモリをロード
	if (!fio->Read(tvram, 0x80000)) {
		return FALSE;
	}

	// サイズをロード、照合
	if (!fio->Read(&sz, sizeof(sz))) {
		return FALSE;
	}
	if (sz != sizeof(tvram_t)) {
		return FALSE;
	}

	// 実体をロード
	if (!fio->Read(&tvdata, (int)sz)) {
		return FALSE;
	}

	// tvcount(version2.04で追加)
	tvcount = 0;
	if (ver >= 0x0204) {
		if (!fio->Read(&tvcount, sizeof(tvcount))) {
			return FALSE;
		}
	}

	// レンダラへ通知
	for(addr=0; addr<0x20000; addr++) {
		render->TextMem(addr);
	}

	// ハンドラ設定
	SelectHandler();

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	設定適用
//
//---------------------------------------------------------------------------
void FASTCALL TVRAM::ApplyCfg(const Config* /*config*/)
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
void FASTCALL TVRAM::AssertDiag() const
{
	// 基本クラス
	MemDevice::AssertDiag();

	ASSERT(this);
	ASSERT(GetID() == MAKEID('T', 'V', 'R', 'M'));
	ASSERT(memdev.first == 0xe00000);
	ASSERT(memdev.last == 0xe7ffff);
	ASSERT(tvram);
	ASSERT(normal);
	ASSERT(mask);
	ASSERT(multi);
	ASSERT(both);
	ASSERT(handler);
	ASSERT(tvdata.multi <= 0x1f);
	ASSERT(tvdata.mask < 0x10000);
	ASSERT(tvdata.rev >= 0xffff0000);
	ASSERT(tvdata.maskh < 0x100);
	ASSERT(tvdata.revh >= 0xffffff00);
	ASSERT(tvdata.src < 0x100);
	ASSERT(tvdata.dst < 0x100);
	ASSERT(tvdata.plane <= 0x0f);
}
#endif	// NDEBUG

//---------------------------------------------------------------------------
//
//	バイト読み込み
//
//---------------------------------------------------------------------------
DWORD FASTCALL TVRAM::ReadByte(DWORD addr)
{
	ASSERT(this);
	ASSERT((addr >= memdev.first) && (addr <= memdev.last));
	ASSERT_DIAG();

	// ウェイト(0.75ウェイト)
	tvcount++;
	if (tvcount & 3) {
		scheduler->Wait(1);
	}

	// エンディアンを反転させて読み込み
	return (DWORD)tvram[(addr & 0x7ffff) ^ 1];
}

//---------------------------------------------------------------------------
//
//	ワード読み込み
//
//---------------------------------------------------------------------------
DWORD FASTCALL TVRAM::ReadWord(DWORD addr)
{
	ASSERT(this);
	ASSERT((addr >= memdev.first) && (addr <= memdev.last));
	ASSERT((addr & 1) == 0);
	ASSERT_DIAG();

	// ウェイト(0.75ウェイト)
	tvcount++;
	if (tvcount & 3) {
		scheduler->Wait(1);
	}

	// 読み込み
	return (DWORD)*(WORD *)(&tvram[addr & 0x7ffff]);
}

//---------------------------------------------------------------------------
//
//	バイト書き込み
//
//---------------------------------------------------------------------------
void FASTCALL TVRAM::WriteByte(DWORD addr, DWORD data)
{
	ASSERT(this);
	ASSERT((addr >= memdev.first) && (addr <= memdev.last));
	ASSERT(data < 0x100);
	ASSERT_DIAG();

	// ウェイト(0.75ウェイト)
	tvcount++;
	if (tvcount & 3) {
		scheduler->Wait(1);
	}

	// 書き込み
	handler->WriteByte((addr & 0x7ffff) ^ 1, data);
}

//---------------------------------------------------------------------------
//
//	ワード書き込み
//
//---------------------------------------------------------------------------
void FASTCALL TVRAM::WriteWord(DWORD addr, DWORD data)
{
	ASSERT(this);
	ASSERT((addr >= memdev.first) && (addr <= memdev.last));
	ASSERT((addr & 1) == 0);
	ASSERT(data < 0x10000);
	ASSERT_DIAG();

	// ウェイト(0.75ウェイト)
	tvcount++;
	if (tvcount & 3) {
		scheduler->Wait(1);
	}

	// 書き込み
	handler->WriteWord(addr & 0x7ffff, data);
}

//---------------------------------------------------------------------------
//
//	読み込みのみ
//
//---------------------------------------------------------------------------
DWORD FASTCALL TVRAM::ReadOnly(DWORD addr) const
{
	ASSERT(this);
	ASSERT((addr >= memdev.first) && (addr <= memdev.last));
	ASSERT_DIAG();

	// エンディアンを反転させて読み込み
	return tvram[(addr & 0x7ffff) ^ 1];
}

//---------------------------------------------------------------------------
//
//	TVRAM取得
//
//---------------------------------------------------------------------------
const BYTE* FASTCALL TVRAM::GetTVRAM() const
{
	ASSERT(this);
	ASSERT_DIAG();

	return tvram;
}

//---------------------------------------------------------------------------
//
//	同時書き込み設定
//
//---------------------------------------------------------------------------
void FASTCALL TVRAM::SetMulti(DWORD data)
{
	ASSERT(this);
	ASSERT(data <= 0x1f);
	ASSERT_DIAG();

	// 一致チェック
	if (tvdata.multi == data) {
		return;
	}

	// データをコピー
	tvdata.multi = data;

	// ハンドラ選択
	SelectHandler();
}

//---------------------------------------------------------------------------
//
//	アクセスマスク設定
//
//---------------------------------------------------------------------------
void FASTCALL TVRAM::SetMask(DWORD data)
{
	ASSERT(this);
	ASSERT(data < 0x10000);
	ASSERT_DIAG();

	// 一致チェック
	if (tvdata.mask == data) {
		return;
	}

	// データをコピー
	tvdata.mask = data;
	tvdata.rev = ~tvdata.mask;
	tvdata.maskh = tvdata.mask >> 8;
	tvdata.revh = ~tvdata.maskh;

	// ハンドラ選択
	SelectHandler();
}

//---------------------------------------------------------------------------
//
//	ハンドラ選択
//
//---------------------------------------------------------------------------
void FASTCALL TVRAM::SelectHandler()
{
	ASSERT(this);
	ASSERT_DIAG();

	// 通常
	handler = normal;

	// マルチチェック
	if (tvdata.multi != 0) {
		// マスクと併用か
		if (tvdata.mask != 0) {
			// 両方
			handler = both;

			// マルチデータを設定
			handler->multi = tvdata.multi;

			// マスクデータを設定
			handler->mask = tvdata.mask;
			handler->rev = tvdata.rev;
			handler->maskh = tvdata.maskh;
			handler->revh = tvdata.revh;
		}
		else {
			// マルチ
			handler = multi;

			// マルチデータを設定
			handler->multi = tvdata.multi;
		}
		return;
	}

	// マスクチェック
	if (tvdata.mask != 0) {
		// マスク
		handler = mask;

		// マスクデータを設定
		handler->mask = tvdata.mask;
		handler->rev = tvdata.rev;
		handler->maskh = tvdata.maskh;
		handler->revh = tvdata.revh;
	}
}

//---------------------------------------------------------------------------
//
//	ラスタコピー設定
//
//---------------------------------------------------------------------------
void FASTCALL TVRAM::SetCopyRaster(DWORD src, DWORD dst, DWORD plane)
{
	ASSERT(this);
	ASSERT(src < 0x100);
	ASSERT(dst < 0x100);
	ASSERT(plane <= 0x0f);
	ASSERT_DIAG();

	tvdata.src = src;
	tvdata.dst = dst;
	tvdata.plane = plane;
}

//---------------------------------------------------------------------------
//
//	ラスタコピー実行
//
//---------------------------------------------------------------------------
void FASTCALL TVRAM::RasterCopy()
{
	ASSERT(this);
	ASSERT_DIAG();
#if 0
	DWORD *p;
	DWORD *q;
	int i;
	int j;
	DWORD plane;

	// ポインタ、プレーンを初期化
	p = (DWORD*)&tvram[tvdata.src << 9];
	q = (DWORD*)&tvram[tvdata.dst << 9];
	plane = tvdata.plane;

	// プレーン別に行う
	for (i=0; i<4; i++) {
		if (plane & 1) {
			for (j=7; j>=0; j--) {
				q[0] = p[0];
				q[1] = p[1];
				q[2] = p[2];
				q[3] = p[3];
				q[4] = p[4];
				q[5] = p[5];
				q[6] = p[6];
				q[7] = p[7];
				q[8] = p[8];
				q[9] = p[9];
				q[10] = p[10];
				q[11] = p[11];
				q[12] = p[12];
				q[13] = p[13];
				q[14] = p[14];
				q[15] = p[15];
				p += 16;
				q += 16;
			}
			p -= 128;
			q -= 128;
		}
		p += 0x8000;
		q += 0x8000;
		plane >>= 1;
	}

	// レンダラに、コピー先のエリアが置き換わったことを通知
	plane = tvdata.dst;
	plane <<= 9;
	for (i=0; i<0x200; i++) {
		render->TextMem(plane + i);
	}
#endif
	// レンダラを呼ぶ
	if (tvdata.plane != 0) {
		render->TextCopy(tvdata.src, tvdata.dst, tvdata.plane);
	}
}
