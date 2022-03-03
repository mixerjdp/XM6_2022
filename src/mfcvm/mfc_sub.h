//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2005 ＰＩ．(ytanaka@ipc-tokai.or.jp)
//	[ MFC サブウィンドウ ]
//
//---------------------------------------------------------------------------

#if defined(_WIN32)

#if !defined (mfc_sub_h)
#define mfc_sub_h

//===========================================================================
//
//	サブウィンドウ
//
//===========================================================================
class CSubWnd : public CWnd
{
public:
	CSubWnd();
										// コンストラクタ
	BOOL FASTCALL Init(CDrawView *pDrawView);
										// 初期化
	void FASTCALL Enable(BOOL m_bEnable);
										// 動作制御
	DWORD FASTCALL GetID() const;
										// ウィンドウIDを返す
	virtual void FASTCALL Refresh() = 0;
										// リフレッシュ描画
	virtual void FASTCALL Update();
										// メッセージスレッドから更新
	virtual BOOL FASTCALL Save(Fileio *pFio, int nVer);
										// セーブ
	virtual BOOL FASTCALL Load(Fileio *pFio, int nVer);
										// ロード
	virtual void FASTCALL ApplyCfg(const Config *pConfig);
										// 設定適用
#if !defined(NDEBUG)
	void AssertValid() const;
										// 診断
#endif	// NDEBUG

	CSubWnd *m_pNextWnd;
										// 次のウィンドウ

protected:
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
										// ウィンドウ作成
	afx_msg void OnDestroy();
										// ウィンドウ削除
	afx_msg BOOL OnEraseBkgnd(CDC *pDC);
										// 背景描画
	afx_msg void OnActivate(UINT nState, CWnd *pWnd, BOOL bMinimized);
										// アクティベート
	void PostNcDestroy();
										// ウィンドウ削除完了
	CString m_strCaption;
										// ウィンドウキャプション
	BOOL m_bEnable;
										// 有効フラグ
	DWORD m_dwID;
										// ウィンドウID
	CScheduler *m_pSch;
										// スケジューラ(Win32)
	CDrawView *m_pDrawView;
										// Drawビュー
	CFont *m_pTextFont;
										// テキストフォント
	int m_tmWidth;
										// テキスト幅
	int m_tmHeight;
										// テキスト高さ
	int m_nWidth;
										// ウィンドウ幅(キャラクタ単位)
	int m_nHeight;
										// ウィンドウ高さ(キャラクタ単位)
	BOOL m_bPopup;
										// ポップアップタイプ

private:
	void FASTCALL SetupTextFont();
										// テキストフォントセットアップ

	DECLARE_MESSAGE_MAP()
										// メッセージ マップあり
};

//===========================================================================
//
//	サブテキストウィンドウ
//
//===========================================================================
class CSubTextWnd : public CSubWnd
{
public:
	CSubTextWnd();
										// コンストラクタ
	void FASTCALL Refresh();
										// リフレッシュ描画

protected:
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
										// ウィンドウ作成
	afx_msg void OnDestroy();
										// ウィンドウ削除
	afx_msg void OnPaint();
										// 描画
	afx_msg void OnSize(UINT nType, int cx, int cy);
										// サイズ変更
	virtual void FASTCALL OnDraw(CDC *pDC);
										// 描画メイン
	virtual void FASTCALL Setup() = 0;
										// セットアップ
	void FASTCALL Clear();
										// テキストバッファ消去
	void FASTCALL SetChr(int x, int y, TCHAR ch);
										// 文字セット
	void FASTCALL SetString(int x, int y, LPCTSTR lpszText);
										// 文字列セット(x, y指定あり)
	void FASTCALL Reverse(BOOL bReverse);
										// 反転属性セット
	void FASTCALL ReSize(int nWidth, int nHeight);
										// リサイズ
	BOOL m_bReverse;
										// 反転フラグ
	BYTE *m_pTextBuf;
										// テキストバッファ
	BYTE *m_pDrawBuf;
										// 描画バッファ

	DECLARE_MESSAGE_MAP()
										// メッセージ マップあり
};

//===========================================================================
//
//	サブテキスト可変ウィンドウ
//
//===========================================================================
class CSubTextSizeWnd : public CSubTextWnd
{
public:
	CSubTextSizeWnd();
										// コンストラクタ
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
										// ウィンドウ作成準備

protected:
	virtual void FASTCALL OnDraw(CDC *pDC);
										// 描画メイン
};

//===========================================================================
//
//	サブテキストスクロールウィンドウ
//
//===========================================================================
class CSubTextScrlWnd : public CSubTextSizeWnd
{
public:
	CSubTextScrlWnd();
										// コンストラクタ
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
										// ウィンドウ作成準備

protected:
	afx_msg int OnCreate(LPCREATESTRUCT lpcs);
										// ウィンドウ作成
	afx_msg void OnSize(UINT nType, int cx, int cy);
										// サイズ変更
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar *pBar);
										// 水平スクロール
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar *pBar);
										// 垂直スクロール
	void FASTCALL SetupScrl();
										// スクロール準備
	virtual void FASTCALL SetupScrlH();
										// スクロール準備(水平)
	virtual void FASTCALL SetupScrlV();
										// スクロール準備(垂直)
	int m_ScrlWidth;
										// 仮想画面幅(キャラクタ単位)
	int m_ScrlHeight;
										// 仮想画面高さ(キャラクタ単位)
	BOOL m_bScrlH;
										// 水平スクロールフラグ
	BOOL m_bScrlV;
										// 垂直スクロールフラグ
	int m_ScrlX;
										// 水平オフセット(キャラクタ単位)
	int m_ScrlY;
										// 垂直オフセット(キャラクタ単位)

	DECLARE_MESSAGE_MAP()
										// メッセージ マップあり
};

//===========================================================================
//
//	サブリストウィンドウ
//
//===========================================================================
class CSubListWnd : public CSubWnd
{
public:
	CSubListWnd();
										// コンストラクタ
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
										// ウィンドウ作成準備
	virtual void FASTCALL Refresh() = 0;
										// リフレッシュ

protected:
	afx_msg int OnCreate(LPCREATESTRUCT lpcs);
										// ウィンドウ作成
	afx_msg void OnSize(UINT nType, int cx, int cy);
										// サイズ変更
	afx_msg void OnDrawItem(int nID, LPDRAWITEMSTRUCT lpDIS);
										// オーナードロー
	virtual void FASTCALL InitList() = 0;
										// リストコントロール初期化
	CListCtrl m_ListCtrl;
										// リストコントロール

	DECLARE_MESSAGE_MAP()
										// メッセージ マップあり
};

//===========================================================================
//
//	サブBMPウィンドウ
//
//===========================================================================
class CSubBMPWnd : public CWnd
{
public:
	CSubBMPWnd();
										// コンストラクタ
	void FASTCALL Refresh();
										// 描画
#if !defined(NDEBUG)
	void AssertValid() const;
										// 診断
#endif	// NDEBUG

	// 上位向け
	void FASTCALL GetMaximumRect(LPRECT lpRect, BOOL bScroll);
										// 最大ウィンドウ矩形取得
	void FASTCALL GetFitRect(LPRECT lpRect);
										// フィット矩形取得
	void FASTCALL GetDrawRect(LPRECT lpRect);
										// 描画矩形取得
	BYTE* FASTCALL GetBits() const;
										// ビットポインタ取得
	void FASTCALL DrawBits(CDC *pDC, CPoint point);
										// 描画

	int m_nScrlWidth;
										// 仮想画面幅
	int m_nScrlHeight;
										// 仮想画面高さ
	int m_nScrlX;
										// 水平オフセット
	int m_nScrlY;
										// 垂直オフセット
	int m_nCursorX;
										// マウスカーソルX
	int m_nCursorY;
										// マウスカーソルY

protected:
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
										// ウィンドウ作成
	afx_msg void OnDestroy();
										// ウィンドウ削除
	afx_msg void OnSize(UINT nType, int cx, int cy);
										// サイズ変更
	afx_msg BOOL OnEraseBkgnd(CDC *pDC);
										// 背景描画
	afx_msg void OnPaint();
										// 再描画
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar *pBar);
										// 水平スクロール
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar *pBar);
										// 垂直スクロール
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
										// マウス移動
	void PostNcDestroy();
										// ウィンドウ削除完了

	// ビットマップ
	BITMAPINFOHEADER m_bmi;
										// ビットマップヘッダ
	HANDLE m_hBitmap;
										// ビットマップハンドル
	BYTE *m_pBits;
										// ビットマップビット
	int m_nMul;
										// 倍率(50%単位)

	// スクロール
	void FASTCALL SetupScrlH();
										// スクロール準備(水平)
	void FASTCALL SetupScrlV();
										// スクロール準備(垂直)

	DECLARE_MESSAGE_MAP()
										// メッセージ マップあり
};

//===========================================================================
//
//	サブビットマップウィンドウ
//
//===========================================================================
class CSubBitmapWnd : public CSubWnd
{
public:
	CSubBitmapWnd();
										// コンストラクタ
	BOOL PreCreateWindow(CREATESTRUCT& cs);
										// ウィンドウ作成準備
	void FASTCALL Refresh();
										// リフレッシュ
	virtual void FASTCALL Setup(int x, int y, int width, int height, BYTE *ptr) = 0;
										// セットアップ

protected:
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
										// ウィンドウ作成
	afx_msg void OnSizing(UINT nSide, LPRECT lpRect);
										// サイズ変更中
	afx_msg void OnSize(UINT nType, int cx, int cy);
										// サイズ変更
	DWORD FASTCALL ConvPalette(WORD value);
										// パレット変換
	CSubBMPWnd *m_pBMPWnd;
										// ビットマップウィンドウ
	CStatusBar m_StatusBar;
										// ステータスバー
	int m_nScrlWidth;
										// 仮想画面幅(ドット単位)
	int m_nScrlHeight;
										// 仮想画面高さ(ドット単位)

	DECLARE_MESSAGE_MAP()
										// メッセージ マップあり
};

#endif	// mfc_sub_h
#endif	// _WIN32
