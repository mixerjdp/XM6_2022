//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001,2002 ＰＩ．(ytanaka@ipc-tokai.or.jp)
//	[ MFC サブウィンドウ(レンダラ) ]
//
//---------------------------------------------------------------------------

#if defined(_WIN32)

#if !defined(mfc_rend_h)
#define mfc_rend_h

#include "mfc_sub.h"

//===========================================================================
//
//	汎用バッファウィンドウ
//
//===========================================================================
class CRendBufWnd : public CSubBitmapWnd
{
public:
	CRendBufWnd(int nType);
										// コンストラクタ
	void FASTCALL Setup(int x, int y, int width, int height, BYTE *ptr);
										// セットアップ
	void FASTCALL Update();
										// メッセージスレッドからの更新

private:
	int m_nType;
										// タイプ
	const DWORD *m_pRendBuf;
										// テキストバッファ
};

//===========================================================================
//
//	合成バッファウィンドウ
//
//===========================================================================
class CMixBufWnd : public CSubBitmapWnd
{
public:
	CMixBufWnd();
										// コンストラクタ
	void FASTCALL Setup(int x, int y, int width, int height, BYTE *ptr);
										// セットアップ
	void FASTCALL Update();
										// メッセージスレッドからの更新

private:
	const Render::render_t *m_pRendWork;
										// レンダラワーク
};

//===========================================================================
//
//	PCGバッファウィンドウ
//
//===========================================================================
class CPCGBufWnd : public CSubBitmapWnd
{
public:
	CPCGBufWnd();
										// コンストラクタ
	void FASTCALL Setup(int x, int y, int width, int height, BYTE *ptr);
										// セットアップ
	void FASTCALL Update();
										// メッセージスレッドからの更新

private:
	const DWORD *m_pPCGBuf;
										// PCGバッファ実体
	const DWORD *m_dwPCGBuf;
										// PCGバッファ使用カウンタ
};

#endif	// mfc_rend_h
#endif	// _WIN32
