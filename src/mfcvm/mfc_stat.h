//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2005 ＰＩ．(ytanaka@ipc-tokai.or.jp)
//	[ MFC ステータスビュー ]
//
//---------------------------------------------------------------------------

#if defined(_WIN32)

#if !defined (mfc_stat_h)
#define mfc_stat_h

//===========================================================================
//
//	ステータスビュー
//
//===========================================================================
class CStatusView : public CWnd
{
public:
	CStatusView();
										// コンストラクタ
	BOOL FASTCALL Init(CFrmWnd *pFrmWnd);
										// 初期化
	void FASTCALL SetCaptionString(CString& strCap);
										// キャプション文字列指定
	void FASTCALL SetMenuString(CString& strMenu);
										// メニュー文字列指定
	void FASTCALL SetInfoString(CString& strInfo);
										// 情報文字列指定
	void FASTCALL DrawStatus(int nPane);
										// ステータス描画

protected:
	void PostNcDestroy();
										// ウィンドウ削除完了
	afx_msg void OnPaint();
										// ウィンドウ再描画
	afx_msg BOOL OnEraseBkgnd(CDC *pDC);
										// ウィンドウ背景描画
	afx_msg void OnSize(UINT nType, int cx, int cy);
										// ウィンドウサイズ変更
	afx_msg void OnMove(int x, int y);
										// ウィンドウ位置変更

private:
	// 全体
	CFrmWnd *m_pFrmWnd;
										// フレームウィンドウ
	CRect m_rectClient;
										// クライアント矩形
	LONG m_tmWidth;
										// キャラクタ横幅
	LONG m_tmHeight;
										// キャラクタ高さ

	// メッセージ
	void FASTCALL DrawMessage(CDC *pDC) const;
										// メッセージ描画
	CString m_strIdle;
										// アイドル文字列
	CString m_strCaption;
										// キャプション文字列
	CString m_strMenu;
										// メニュー文字列
	CString m_strInfo;
										// 情報表示文字列

	// ステータス
	CRect m_rectFDD[2];
										// FDD矩形
	CRect m_rectLED[3];
										// LED矩形

	DECLARE_MESSAGE_MAP()
										// メッセージ マップあり
};

#endif	// mfc_stat_h
#endif	// _WIN32
