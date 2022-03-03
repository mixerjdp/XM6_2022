//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2005 ＰＩ．(ytanaka@ipc-tokai.or.jp)
//	[ MFC Drawビュー ]
//
//---------------------------------------------------------------------------

#if defined(_WIN32)

#if !defined(mfc_draw_h)
#define mfc_draw_h

//===========================================================================
//
//	Drawビュー
//
//===========================================================================
class CDrawView : public CView
{
public:
	// 内部ワーク定義
	typedef struct _DRAWINFO {
		BOOL bPower;					// 電源
		Render *pRender;				// レンダラ
		Render::render_t *pWork;		// レンダラワーク
        DWORD dwDrawCount;				// 描画回数

		// DIBセクション
		HBITMAP hBitmap;				// DIBセクション
		DWORD *pBits;					// ビットデータ
		int nBMPWidth;					// BMP幅
		int nBMPHeight;					// BMP高さ

		// レンダラ連携
		int nRendWidth;					// レンダラ幅
		int nRendHeight;				// レンダラ高さ
		int nRendHMul;					// レンダラ横方向倍率
		int nRendVMul;					// レンダラ縦方向倍率
		int nLeft;						// 横マージン
		int nTop;						// 縦マージン
		int nWidth;						// BitBlt幅
		int nHeight;					// BitBlt高さ

		// Blt系
		int nBltTop;					// 描画開始Y
		int nBltBottom;					// 描画終了Y
		int nBltLeft;					// 描画開始X
		int nBltRight;					// 描画終了X
		BOOL bBltAll;					// 全表示フラグ
		BOOL bBltStretch;				// アスペクト比にあわせ拡大
	} DRAWINFO, *LPDRAWINFO;

public:
	// 基本ファンクション
	CDrawView();
										// コンストラクタ
	void FASTCALL Enable(BOOL bEnable);
										// 動作制御
	BOOL FASTCALL IsEnable() const;
										// 動作フラグ取得
	BOOL FASTCALL Init(CWnd *pParent);
										// 初期化
	BOOL PreCreateWindow(CREATESTRUCT& cs);
										// ウィンドウ作成準備
	void FASTCALL Refresh();
										// リフレッシュ描画
	void FASTCALL Draw(int index);
										// 描画(あるウィンドウのみ)
	void FASTCALL Update();
										// メッセージスレッドからの更新
	void FASTCALL ApplyCfg(const Config *pConfig);
										// 設定適用
	void FASTCALL GetDrawInfo(LPDRAWINFO pDrawInfo) const;
										// 描画情報取得

	// レンダリング描画
	void OnDraw(CDC *pDC);
										// 描画
	void FASTCALL Stretch(BOOL bStretch);
										// 拡大モード
	BOOL IsStretch() const				{ return m_Info.bBltStretch; }
										// 拡大モード取得

	// サブウィンドウ管理
	int FASTCALL GetNewSWnd() const;
										// サブウィンドウ新規インデックス取得
	void FASTCALL AddSWnd(CSubWnd *pSubWnd);
										// サブウィンドウ追加(子から呼ばれる)
	void FASTCALL DelSWnd(CSubWnd *pSubWnd);
										// サブウィンドウ削除(子から呼ばれる)
	void FASTCALL ClrSWnd();
										// サブウィンドウをすべて削除
	CSubWnd* FASTCALL GetFirstSWnd() const;
										// 最初のサブウィンドウを取得
	CSubWnd* FASTCALL SearchSWnd(DWORD dwID) const;
										// 任意IDのサブウィンドウを取得
	CFont* FASTCALL GetTextFont();
										// テキストフォント取得
	CSubWnd* FASTCALL NewWindow(BOOL bDis);
										// 新規ウィンドウ作成(Dis, Mem)
	BOOL FASTCALL IsNewWindow(BOOL bDis);
										// 新規ウィンドウ作成可能か問い合わせ
	int FASTCALL GetSubWndNum() const;
										// サブウィンドウの個数を取得
	LPCTSTR FASTCALL GetWndClassName() const;
										// ウィンドウクラス名取得
	void FASTCALL SetupBitmap();

protected:
	// WMメッセージ
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
										// ウィンドウ作成
	afx_msg void OnDestroy();
										// ウィンドウ削除
	afx_msg void OnSize(UINT nType, int cx, int cy);
										// ウィンドウサイズ変更
	afx_msg void OnPaint();
										// 描画
	afx_msg BOOL OnEraseBkgnd(CDC *pDC);
										// 背景描画
	afx_msg LRESULT OnDisplayChange(WPARAM wParam, LPARAM lParam);
										// ディスプレイ変更
	afx_msg void OnDropFiles(HDROP hDropInfo);
										// ファイルドロップ
	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
										// マウスホイール
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
										// キー押下
	afx_msg void OnSysKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
										// システムキー押下
	afx_msg void OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags);
										// キー離した
	afx_msg void OnSysKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags);
										// システムキー離した
	afx_msg void OnMove(int x, int y);
										// ウィンドウ移動

	BOOL m_bEnable;
										// 有効フラグ
	CFont m_TextFont;
										// テキストフォント
	
private:
	
										// ビットマップ準備
	inline void FASTCALL ReCalc(CRect& rect);
										// 再計算
	inline void FASTCALL DrawRect(CDC *pDC);
										// 周囲の余白を描画
	inline BOOL FASTCALL CalcRect();
										// 描画必要領域を調べる
	int FASTCALL MakeBits();
										// ビット作成
	BOOL FASTCALL KeyUpDown(UINT nChar, UINT nFlags, BOOL bDown);
										// キー判別
	CSubWnd *m_pSubWnd;
										// 最初のサブウィンドウ
	CFrmWnd *m_pFrmWnd;
										// フレームウィンドウ
	CScheduler *m_pScheduler;
										// スケジューラ
	CInput *m_pInput;
										// インプット
	DRAWINFO m_Info;
										// 内部ワーク

	DECLARE_MESSAGE_MAP()
										// メッセージ マップあり
};

#endif	// mfc_draw_h
#endif	// _WIN32
