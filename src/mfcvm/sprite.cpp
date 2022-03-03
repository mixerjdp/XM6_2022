//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 ＰＩ．(ytanaka@ipc-tokai.or.jp)
//	[ スプライト(CYNTHIA) ]
//
//---------------------------------------------------------------------------

#include "os.h"
#include "xm6.h"
#include "vm.h"
#include "log.h"
#include "schedule.h"
#include "fileio.h"
#include "render.h"
#include "sprite.h"

//===========================================================================
//
//	スプライト
//
//===========================================================================
//#define SPRITE_LOG

//---------------------------------------------------------------------------
//
//	コンストラクタ
//
//---------------------------------------------------------------------------
Sprite::Sprite(VM *p) : MemDevice(p)
{
	// デバイスIDを初期化
	dev.id = MAKEID('S', 'P', 'R', ' ');
	dev.desc = "Sprite (CYNTHIA)";

	// 開始アドレス、終了アドレス
	memdev.first = 0xeb0000;
	memdev.last = 0xebffff;

	// その他
	sprite = NULL;
	render = NULL;
	spr.mem = NULL;
	spr.pcg = NULL;
}

//---------------------------------------------------------------------------
//
//	初期化
//
//---------------------------------------------------------------------------
BOOL FASTCALL Sprite::Init()
{
	ASSERT(this);

	// 基本クラス
	if (!MemDevice::Init()) {
		return FALSE;
	}

	// メモリ確保、クリア
	try {
		sprite = new BYTE[ 0x10000 ];
	}
	catch (...) {
		return FALSE;
	}
	if (!sprite) {
		return FALSE;
	}

	// EB0400-EB07FF, EB0812-EB7FFFはReserved(FF)
	memset(sprite, 0, 0x10000);
	memset(&sprite[0x400], 0xff, 0x400);
	memset(&sprite[0x812], 0xff, 0x77ee);

	// ワーク初期化
	memset(&spr, 0, sizeof(spr));
	spr.mem = &sprite[0x0000];
	spr.pcg = &sprite[0x8000];

	// レンダラ取得
	render = (Render*)vm->SearchDevice(MAKEID('R', 'E', 'N', 'D'));
	ASSERT(render);

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	クリーンアップ
//
//---------------------------------------------------------------------------
void FASTCALL Sprite::Cleanup()
{
	// メモリ解放
	if (sprite) {
		delete[] sprite;
		sprite = NULL;
		spr.mem = NULL;
		spr.pcg = NULL;
	}

	// 基本クラスへ
	MemDevice::Cleanup();
}

//---------------------------------------------------------------------------
//
//	リセット
//
//---------------------------------------------------------------------------
void FASTCALL Sprite::Reset()
{
	int i;

	ASSERT(this);
	LOG0(Log::Normal, "リセット");

	// レジスタ設定
	spr.connect = FALSE;
	spr.disp = FALSE;

	// BGページ初期化
	for (i=0; i<2; i++) {
		spr.bg_on[i] = FALSE;
		spr.bg_area[i] = 0;
		spr.bg_scrlx[i] = 0;
		spr.bg_scrly[i] = 0;
	}

	// BGサイズ初期化
	spr.bg_size = FALSE;

	// タイミング初期化
	spr.h_total = 0;
	spr.h_disp = 0;
	spr.v_disp = 0;
	spr.lowres = FALSE;
	spr.v_res = 0;
	spr.h_res = 0;
}

//---------------------------------------------------------------------------
//
//	セーブ
//
//---------------------------------------------------------------------------
BOOL FASTCALL Sprite::Save(Fileio *fio, int /*ver*/)
{
	size_t sz;

	ASSERT(this);
	ASSERT(fio);
	ASSERT(spr.mem);

	LOG0(Log::Normal, "セーブ");

	// サイズをセーブ
	sz = sizeof(sprite_t);
	if (!fio->Write(&sz, sizeof(sz))) {
		return FALSE;
	}

	// 実体をセーブ
	if (!fio->Write(&spr, (int)sz)) {
		return FALSE;
	}

	// メモリをセーブ
	if (!fio->Write(sprite, 0x10000)) {
		return FALSE;
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	ロード
//
//---------------------------------------------------------------------------
BOOL FASTCALL Sprite::Load(Fileio *fio, int /*ver*/)
{
	size_t sz;
	int i;
	DWORD addr;
	DWORD data;

	ASSERT(this);
	ASSERT(fio);
	ASSERT(spr.mem);

	LOG0(Log::Normal, "ロード");

	// サイズをロード、照合
	if (!fio->Read(&sz, sizeof(sz))) {
		return FALSE;
	}
	if (sz != sizeof(sprite_t)) {
		return FALSE;
	}

	// 実体をロード
	if (!fio->Read(&spr, (int)sz)) {
		return FALSE;
	}

	// メモリをロード
	if (!fio->Read(sprite, 0x10000)) {
		return FALSE;
	}

	// ポインタを上書き
	spr.mem = &sprite[0x0000];
	spr.pcg = &sprite[0x8000];

	// レンダラへ通知(レジスタ)
	render->BGCtrl(4, spr.bg_size);
	for (i=0; i<2; i++) {
		// BGデータエリア
		if (spr.bg_area[i] & 1) {
			render->BGCtrl(i + 2, TRUE);
		}
		else {
			render->BGCtrl(i + 2, FALSE);
		}

		// BG表示ON/OFF
		render->BGCtrl(i, spr.bg_on[i]);

		// BGスクロール
		render->BGScrl(i, spr.bg_scrlx[i], spr.bg_scrly[i]);
	}

	// レンダラへ通知(メモリ:偶数アドレスのみ)
	for (addr=0; addr<0x10000; addr+=2) {
		if (addr < 0x400) {
			data = *(WORD*)(&sprite[addr]);
			render->SpriteReg(addr, data);
			continue;
		}
		if (addr < 0x8000) {
			continue;
		}
		if (addr >= 0xc000) {
			data = *(WORD*)(&sprite[addr]);
			render->BGMem(addr, data);
		}
		render->PCGMem(addr);
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	設定適用
//
//---------------------------------------------------------------------------
void FASTCALL Sprite::ApplyCfg(const Config *config)
{
	ASSERT(config);
	LOG0(Log::Normal, "設定適用");
}

//---------------------------------------------------------------------------
//
//	バイト読み込み
//
//---------------------------------------------------------------------------
DWORD FASTCALL Sprite::ReadByte(DWORD addr)
{
	ASSERT(this);
	ASSERT((addr >= memdev.first) && (addr <= memdev.last));

	// オフセット算出
	addr &= 0xffff;

	// 0800～7FFFはバスエラーの影響を受けない
	if ((addr >= 0x800) && (addr < 0x8000)) {
		return sprite[addr ^ 1];
	}

	// 接続チェック
	if (!IsConnect()) {
		cpu->BusErr(memdev.first + addr, TRUE);
		return 0xff;
	}

	// ウェイト(エトワールプリンセス)
	if (addr & 1) {
		if (spr.disp) {
			scheduler->Wait(4);
		}
		else {
			scheduler->Wait(2);
		}
	}

	// エンディアンを反転させて読み込み
	return sprite[addr ^ 1];
}

//---------------------------------------------------------------------------
//
//	ワード読み込み
//
//---------------------------------------------------------------------------
DWORD FASTCALL Sprite::ReadWord(DWORD addr)
{
	ASSERT(this);
	ASSERT((addr >= memdev.first) && (addr <= memdev.last));
	ASSERT((addr & 1) == 0);

	// オフセット算出
	addr &= 0xffff;

	// 0800～7FFFはバスエラーの影響を受けない
	if ((addr >= 0x800) && (addr < 0x8000)) {
		return *(WORD *)(&sprite[addr]);
	}

	// 接続チェック
	if (!IsConnect()) {
		cpu->BusErr(memdev.first + addr, TRUE);
		return 0xff;
	}

	// ウェイト(エトワールプリンセス)
	if (spr.disp) {
		scheduler->Wait(4);
	}
	else {
		scheduler->Wait(2);
	}

	// 読み込み
	return *(WORD *)(&sprite[addr]);
}

//---------------------------------------------------------------------------
//
//	バイト書き込み
//
//---------------------------------------------------------------------------
void FASTCALL Sprite::WriteByte(DWORD addr, DWORD data)
{
	DWORD ctrl;

	ASSERT(this);
	ASSERT((addr >= memdev.first) && (addr <= memdev.last));
	ASSERT(data < 0x100);

	// オフセット算出
	addr &= 0xffff;

	// 一致チェック
	if (sprite[addr ^ 1] == data) {
		return;
	}

	// 800～811はコントロールレジスタ
	if ((addr >= 0x800) && (addr < 0x812)) {
		// データ書き込み
		sprite[addr ^ 1] = (BYTE)data;

		if (addr & 1) {
			// 下位書き込み。上位とあわせてコントロール
			ctrl = (DWORD)sprite[addr];
			ctrl <<= 8;
			ctrl |= data;
			Control((DWORD)(addr & 0xfffe), ctrl);
		}
		else {
			// 上位書き込み。下位とあわせてコントロール
			ctrl = data;
			ctrl <<= 8;
			ctrl |= (DWORD)sprite[addr];
			Control(addr, ctrl);
		}
		return;
	}

	// 0812-7FFFはリザーブ(バスエラーの影響を受けない)
	if ((addr >= 0x812) && (addr < 0x8000)) {
		return;
	}

	// 接続チェック
	if (!IsConnect()) {
		cpu->BusErr(memdev.first + addr, FALSE);
		return;
	}

	// ウェイト(エトワールプリンセス)
	if (addr & 1) {
		if (spr.disp) {
			scheduler->Wait(4);
		}
		else {
			scheduler->Wait(2);
		}
	}

	// 0400-07FFはリザーブ(バスエラーの影響を受ける)
	if ((addr >= 0x400) && (addr < 0x800)) {
		return;
	}

	// 書き込み
	sprite[addr ^ 1] = (BYTE)data;

	// レンダラ処理
	addr &= 0xfffe;
	if (addr < 0x400) {
		ctrl = *(WORD*)(&sprite[addr]);
		render->SpriteReg(addr, ctrl);
		return;
	}
	if (addr >= 0x8000) {
		render->PCGMem(addr);
	}
	if (addr >= 0xc000) {
		ctrl = *(WORD*)(&sprite[addr]);
		render->BGMem(addr, ctrl);
	}
}

//---------------------------------------------------------------------------
//
//	ワード書き込み
//
//---------------------------------------------------------------------------
void FASTCALL Sprite::WriteWord(DWORD addr, DWORD data)
{
	ASSERT(this);
	ASSERT((addr >= memdev.first) && (addr <= memdev.last));
	ASSERT((addr & 1) == 0);
	ASSERT(data < 0x10000);

	// オフセット算出
	addr &= 0xfffe;

	// 一致チェック
	if (*(WORD *)(&sprite[addr]) == data) {
		return;
	}

	// 800～811はコントロールレジスタ
	if ((addr >= 0x800) && (addr < 0x812)) {
		*(WORD *)(&sprite[addr]) = data;
		Control(addr, data);
		return;
	}
	// 0812-7FFFはリザーブ(バスエラーの影響を受けない)
	if ((addr >= 0x812) && (addr < 0x8000)) {
		return;
	}

	// ウェイト(エトワールプリンセス)
	if (spr.disp) {
		scheduler->Wait(4);
	}
	else {
		scheduler->Wait(2);
	}

	// 0400-07FFはリザーブ(バスエラーの影響を受ける)
	if ((addr >= 0x400) && (addr < 0x800)) {
		return;
	}

	// 書き込み
	*(WORD *)(&sprite[addr]) = (WORD)data;

	// レンダラ
	if (addr < 0x400) {
		render->SpriteReg(addr, data);
		return;
	}
	if (addr >= 0x8000) {
		render->PCGMem(addr);
	}
	if (addr >= 0xc000) {
		render->BGMem(addr, data);
	}
}

//---------------------------------------------------------------------------
//
//	読み込みのみ
//
//---------------------------------------------------------------------------
DWORD FASTCALL Sprite::ReadOnly(DWORD addr) const
{
	ASSERT(this);
	ASSERT((addr >= memdev.first) && (addr <= memdev.last));

	// オフセット算出
	addr &= 0xffff;

	// エンディアンを反転させて読み込み
	return sprite[addr ^ 1];
}

//---------------------------------------------------------------------------
//
//	コントロール
//
//---------------------------------------------------------------------------
void FASTCALL Sprite::Control(DWORD addr, DWORD data)
{
	ASSERT((addr >= 0x800) && (addr < 0x812));
	ASSERT((addr & 1) == 0);
	ASSERT(data < 0x10000);

	// アドレスを整理
	addr -= 0x800;
	addr >>= 1;

	switch (addr) {
		// BG0スクロールX
		case 0:
			spr.bg_scrlx[0] = data & 0x3ff;
			render->BGScrl(0, spr.bg_scrlx[0], spr.bg_scrly[0]);
			break;

		// BG0スクロールY
		case 1:
			spr.bg_scrly[0] = data & 0x3ff;
			render->BGScrl(0, spr.bg_scrlx[0], spr.bg_scrly[0]);
			break;

		// BG1スクロールX
		case 2:
			spr.bg_scrlx[1] = data & 0x3ff;
			render->BGScrl(1, spr.bg_scrlx[1], spr.bg_scrly[1]);
			break;

		// BG1スクロールY
		case 3:
			spr.bg_scrly[1] = data & 0x3ff;
			render->BGScrl(1, spr.bg_scrlx[1], spr.bg_scrly[1]);
			break;

		// BGコントロール
		case 4:
#if defined(SPRITE_LOG)
			LOG1(Log::Normal, "BGコントロール $%04X", data);
#endif	// SPRITE_LOG
			// bit17 : DISP
			if (data & 0x0200) {
				spr.disp = TRUE;
			}
			else {
				spr.disp = FALSE;
			}

			// BG1
			spr.bg_area[1] = (data >> 4) & 0x03;
			if (spr.bg_area[1] & 2) {
				LOG1(Log::Warning, "BG1データエリア未定義 $%02X", spr.bg_area[1]);
			}
			if (spr.bg_area[1] & 1) {
				render->BGCtrl(3, TRUE);
			}
			else {
				render->BGCtrl(3, FALSE);
			}
			if (data & 0x08) {
				spr.bg_on[1] = TRUE;
			}
			else {
				spr.bg_on[1] = FALSE;
			}
			render->BGCtrl(1, spr.bg_on[1]);

			// BG0
			spr.bg_area[0] = (data >> 1) & 0x03;
			if (spr.bg_area[0] & 2) {
				LOG1(Log::Warning, "BG0データエリア未定義 $%02X", spr.bg_area[0]);
			}
			if (spr.bg_area[0] & 1) {
				render->BGCtrl(2, TRUE);
			}
			else {
				render->BGCtrl(2, FALSE);
			}
			if (data & 0x01) {
				spr.bg_on[0] = TRUE;
			}
			else {
				spr.bg_on[0] = FALSE;
			}
			render->BGCtrl(0, spr.bg_on[0]);
			break;

		// 水平トータル
		case 5:
			spr.h_total = data & 0xff;
			break;

		// 水平表示
		case 6:
			spr.h_disp = data & 0x3f;
			break;

		// 垂直表示
		case 7:
			spr.v_disp = data & 0xff;
			break;

		// 画面モード
		case 8:
			spr.h_res = data & 0x03;
			spr.v_res = (data >> 2) & 0x03;

			// 15kHz
			if (data & 0x10) {
				spr.lowres = FALSE;
			}
			else {
				spr.lowres = TRUE;
			}

			// BGサイズ
			if (spr.h_res == 0) {
				// 8x8
				spr.bg_size = FALSE;
			}
			else {
				// 16x16
				spr.bg_size = TRUE;
			}
			render->BGCtrl(4, spr.bg_size);
			if (spr.h_res & 2) {
				LOG1(Log::Warning, "BG/スプライト H-Res未定義 %d", spr.h_res);
			}
			break;

		// その他
		default:
			ASSERT(FALSE);
			break;
	}
}

//---------------------------------------------------------------------------
//
//	内部データ取得
//
//---------------------------------------------------------------------------
void FASTCALL Sprite::GetSprite(sprite_t *buffer) const
{
	ASSERT(this);
	ASSERT(buffer);

	// 内部ワークをコピー
	*buffer = spr;
}

//---------------------------------------------------------------------------
//
//	メモリエリア取得
//
//---------------------------------------------------------------------------
const BYTE* FASTCALL Sprite::GetMem() const
{
	ASSERT(this);
	ASSERT(spr.mem);

	return spr.mem;
}

//---------------------------------------------------------------------------
//
//	PCGエリア取得
//
//---------------------------------------------------------------------------
const BYTE* FASTCALL Sprite::GetPCG() const
{
	ASSERT(this);
	ASSERT(spr.pcg);

	return spr.pcg;
}
