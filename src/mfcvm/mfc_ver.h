//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 ＰＩ．(ytanaka@ipc-tokai.or.jp)
//	[ MFC バージョン情報ダイアログ ]
//
//---------------------------------------------------------------------------

#if defined(_WIN32)

#if !defined(mfc_ver_h)
#define mfc_ver_h

//===========================================================================
//
//	バージョン情報ダイアログ
//
//===========================================================================
class CAboutDlg : public CDialog
{
public:
	CAboutDlg(CWnd *pParent = NULL);
										// コンストラクタ
	BOOL OnInitDialog();
										// ダイアログ初期化
	void OnOK();
										// OK
	void OnCancel();
										// キャンセル

protected:
	afx_msg void OnPaint();
										// 描画
#if _MFC_VER >= 0x800
	afx_msg LRESULT OnNcHitTest(CPoint point);
										// ヒットテスト
#else
	afx_msg UINT OnNcHitTest(CPoint point);
										// ヒットテスト
#endif	// _MFC_VER
	afx_msg BOOL OnSetCursor(CWnd *pWnd, UINT nHitTest, UINT message);
										// カーソルセット
#if _MFC_VER >= 0x700
	afx_msg void OnTimer(UINT_PTR nTimerID);
										// タイマ
#else
	afx_msg void OnTimer(UINT nTimerID);
										// タイマ
#endif	// _MFC_VER

private:
	void FASTCALL DrawURL(CDC *pDC);
										// URL描画
	void FASTCALL DrawCRT(CDC *pDC);
										// CRT描画
	void FASTCALL DrawX68k(CDC *pDC);
										// X68k描画
	void FASTCALL DrawLED(int x, int y, CDC *pDC);
										// LED描画
	void FASTCALL DrawView(int x, int y, CDC *pDC);
										// ビュー描画
	CString m_URLString;
										// URL文字列
	CRect m_URLRect;
										// URL矩形
	BOOL m_bURLHit;
										// URLヒットフラグ
	CRect m_IconRect;
										// アイコン矩形
#if _MFC_VER >= 0x700
	UINT_PTR m_nTimerID;
										// タイマID
#else
	UINT m_nTimerID;
										// タイマID
#endif
	RTC *m_pRTC;
										// RTC
	SASI *m_pSASI;
										// SASI
	FDD *m_pFDD;
										// FDD
	CDrawView *m_pDrawView;
										// 描画ビュー
	BOOL m_bFloppyLED;
										// フロッピーLEDモード
	BOOL m_bPowerLED;
										// 電源LEDモード

	DECLARE_MESSAGE_MAP()
										// メッセージ マップあり
};

#endif	// mfc_ver_h
#endif	// _WIN32
