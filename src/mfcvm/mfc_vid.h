//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2005 ＰＩ．(ytanaka@ipc-tokai.or.jp)
//	[ MFC サブウィンドウ(ビデオ) ]
//
//---------------------------------------------------------------------------

#if defined(_WIN32)

#if !defined(mfc_vid_h)
#define mfc_vid_h

#include "mfc_sub.h"

//===========================================================================
//
//	サブビデオウィンドウ
//
//===========================================================================
class CSubVideoWnd : public CSubWnd
{
public:
	CSubVideoWnd();
										// コンストラクタ
	BOOL PreCreateWindow(CREATESTRUCT& cs);
										// ウィンドウ作成準備
	void FASTCALL Refresh();
										// リフレッシュ
	void FASTCALL Update();
										// メッセージスレッドからの更新
#if !defined(NDEBUG)
	void AssertValid() const;
										// 診断
#endif	// NDEBUG

protected:
	// メッセージ ハンドラ
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
										// ウィンドウ作成
	afx_msg void OnSizing(UINT nSide, LPRECT lpRect);
										// サイズ変更中
	afx_msg void OnSize(UINT nType, int cx, int cy);
										// サイズ変更

	// 更新
	virtual void FASTCALL Setup(CRect& rect, BYTE *pBits) = 0;
										// セットアップ

	// 子ウィンドウ
	CSubBMPWnd *m_pBMPWnd;
										// ビットマップウィンドウ
	CStatusBar m_StatusBar;
										// ステータスバー
	int m_nScrlWidth;
										// 仮想画面幅(ドット単位)
	int m_nScrlHeight;
										// 仮想画面高さ(ドット単位)
	int m_nPane;
										// ステータスバーペイン数

	// 共通
	DWORD FASTCALL GetColor(DWORD dwColor) const;
										// カラー作成 (16bitから)
	DWORD FASTCALL GetPalette(DWORD dwPalette) const;
										// カラー作成 (パレットインデックスから)
	CRTC *m_pCRTC;
										// CRTC
	VC *m_pVC;
										// VC
	BOOL m_bScroll;
										// スクロール考慮する
	BOOL m_bPalette;
										// パレット考慮する
	BOOL m_bContrast;
										// コントラスト考慮する

	DECLARE_MESSAGE_MAP()
										// メッセージ マップあり
};


//===========================================================================
//
//	テキスト画面ウィンドウ
//
//===========================================================================
class CTVRAMWnd : public CSubVideoWnd
{
public:
	CTVRAMWnd();
										// コンストラクタ
	void FASTCALL Setup(CRect& rect, BYTE *pBits);
										// セットアップ
	void FASTCALL Update();
										// メッセージスレッドからの更新

private:
	TVRAM *m_pTVRAM;
										// TVRAM
};

//===========================================================================
//
//	グラフィック画面(1024×1024)ウィンドウ
//
//===========================================================================
class CG1024Wnd : public CSubBitmapWnd
{
public:
	CG1024Wnd();
										// コンストラクタ
	void FASTCALL Setup(int x, int y, int width, int height, BYTE *ptr);
										// セットアップ
	void FASTCALL Update();
										// メッセージスレッドからの更新

private:
	const WORD *m_pPalette;
										// VCパレットアドレス
	WORD m_WndPalette[16];
										// ウィンドウ保持パレット
	DWORD m_WndColor[16];
										// ウィンドウカラーテーブル
	const BYTE *m_pGVRAM;
										// GVRAMアドレス
};

//===========================================================================
//
//	グラフィック画面(16色)ウィンドウ
//
//===========================================================================
class CG16Wnd : public CSubBitmapWnd
{
public:
	CG16Wnd(int nPage);
										// コンストラクタ
	void FASTCALL Setup(int x, int y, int width, int height, BYTE *ptr);
										// セットアップ
	void FASTCALL Update();
										// メッセージスレッドからの更新

private:
	int m_nPage;
										// ページ番号
	const WORD *m_pPalette;
										// VCパレットアドレス
	WORD m_WndPalette[16];
										// ウィンドウ保持パレット
	DWORD m_WndColor[16];
										// ウィンドウカラーテーブル
	const BYTE *m_pGVRAM;
										// GVRAMアドレス
};

//===========================================================================
//
//	グラフィック画面(256色)ウィンドウ
//
//===========================================================================
class CG256Wnd : public CSubBitmapWnd
{
public:
	CG256Wnd(int nPage);
										// コンストラクタ
	void FASTCALL Setup(int x, int y, int width, int height, BYTE *ptr);
										// セットアップ
	void FASTCALL Update();
										// メッセージスレッドからの更新

private:
	int m_nPage;
										// ページ番号
	const WORD *m_pPalette;
										// VCパレットアドレス
	WORD m_WndPalette[256];
										// ウィンドウ保持パレット
	DWORD m_WndColor[256];
										// ウィンドウカラーテーブル
	const BYTE *m_pGVRAM;
										// GVRAMアドレス
};

//===========================================================================
//
//	グラフィック画面(65536色)ウィンドウ
//
//===========================================================================
class CG64KWnd : public CSubBitmapWnd
{
public:
	CG64KWnd();
										// コンストラクタ
	void FASTCALL Setup(int x, int y, int width, int height, BYTE *ptr);
										// セットアップ
	void FASTCALL Update();
										// メッセージスレッドからの更新

private:
	void FASTCALL Palette();
										// パレット再設定
	const WORD *m_pPalette;
										// VCパレットアドレス
	WORD m_WndPalette[256];
										// ウィンドウ保持パレット
	DWORD m_WndColor[0x10000];
										// ウィンドウカラーテーブル
	const BYTE *m_pGVRAM;
										// GVRAMアドレス
};

//===========================================================================
//
//	PCGウィンドウ
//
//===========================================================================
class CPCGWnd : public CSubBitmapWnd
{
public:
	CPCGWnd();
										// コンストラクタ
	void FASTCALL Setup(int x, int y, int width, int height, BYTE *ptr);
										// セットアップ
	void FASTCALL Update();
										// メッセージスレッドからの更新
	afx_msg void OnParentNotify(UINT message, LPARAM lParam);
										// 親ウインドウへ通知
	afx_msg void OnContextMenu(CWnd *pWnd, CPoint point);
										// コンテキストメニュー
	afx_msg void OnPalette(UINT uID);
										// パレット変更

private:
	const WORD *m_pPalette;
										// VCパレットアドレス
	WORD m_WndPalette[256];
										// ウィンドウ保持パレット
	DWORD m_WndColor[256];
										// ウィンドウカラーテーブル
	const BYTE *m_pPCG;
										// PCGアドレス
	int m_nColor;
										// カラーブロック

	DECLARE_MESSAGE_MAP()
										// メッセージ マップあり
};

//===========================================================================
//
//	BGウィンドウ
//
//===========================================================================
class CBGWnd : public CSubBitmapWnd
{
public:
	CBGWnd(int nPage);
										// コンストラクタ
	void FASTCALL Setup(int x, int y, int width, int height, BYTE *ptr);
										// セットアップ
	void FASTCALL Update();
										// メッセージスレッドからの更新

private:
	const WORD *m_pPalette;
										// VCパレットアドレス
	WORD m_WndPalette[256];
										// ウィンドウ保持パレット
	DWORD m_WndColor[256];
										// ウィンドウカラーテーブル
	int m_nPage;
										// ページ
	Sprite *m_pSprite;
										// スプライトコントローラ
};

//===========================================================================
//
//	パレットウィンドウ
//
//===========================================================================
class CPaletteWnd : public CSubBitmapWnd
{
public:
	CPaletteWnd(BOOL bRend);
										// コンストラクタ
	void FASTCALL Setup(int x, int y, int width, int height, BYTE *ptr);
										// セットアップ
	void FASTCALL Update();
										// メッセージスレッドからの更新

private:
	void FASTCALL SetupRend(DWORD *buf, int no);
										// セットアップ(レンダラ)
	void FASTCALL SetupVC(DWORD *buf, int no);
										// セットアップ(VC)
	BOOL m_bRend;
										// レンダラフラグ
	Render *m_pRender;
										// レンダラ
	const WORD *m_pVCPal;
										// VCパレット
};

#endif	// mfc_vid_h
#endif	// _WIN32
