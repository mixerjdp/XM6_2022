//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001,2002 ＰＩ．(ytanaka@ipc-tokai.or.jp)
//	[ レンダラ(インライン) ]
//
//---------------------------------------------------------------------------

#if !defined(renderin_h)
#define renderin_h

#include "render.h"

//---------------------------------------------------------------------------
//
//	パレット設定
//
//---------------------------------------------------------------------------
inline void FASTCALL Render::SetPalette(int index)
{
	// VCの時点で比較チェックを行う
	render.palmod[index] = TRUE;
	render.palette = TRUE;
}

//---------------------------------------------------------------------------
//
//	テキストVRAM変更
//
//---------------------------------------------------------------------------
inline void FASTCALL Render::TextMem(DWORD addr)
{
	// テキストVRAMの時点で比較チェックを行う
	addr &= 0x1ffff;
	addr >>= 2;
	render.textflag[addr] = TRUE;
	addr >>= 5;
	render.textmod[addr] = TRUE;
}

//---------------------------------------------------------------------------
//
//	グラフィックVRAM変更
//
//---------------------------------------------------------------------------
inline void FASTCALL Render::GrpMem(DWORD addr, DWORD block)
{
	// グラフィックVRAMの時点で比較チェックを行う
	ASSERT(addr <= 0x7ffff);
	ASSERT(block <= 3);

	// 16dotフラグ(16dot単位、(512/16)ｘ512ｘ4 = 0x10000)
	addr >>= 5;
	block <<= 14;
	render.grpflag[addr | block] = TRUE;
	// ラインフラグ(ライン単位、512x4 = 2048)
	addr >>= 5;
	block >>= 5;
	render.grpmod[addr | block] = TRUE;
}

//---------------------------------------------------------------------------
//
//	グラフィックVRAM変更(全て)
//
//---------------------------------------------------------------------------
inline void FASTCALL Render::GrpAll(DWORD line, DWORD block)
{
	ASSERT(line <= 0x1ff);
	ASSERT(block <= 3);

	render.grppal[(block << 9) | line] = TRUE;
}

#endif	// renderin_h
