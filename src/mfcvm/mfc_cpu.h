//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2005 ＰＩ．(ytanaka@ipc-tokai.or.jp)
//	[ MFC サブウィンドウ(CPU) ]
//
//---------------------------------------------------------------------------

#if defined(_WIN32)

#if !defined(mfc_cpu_h)
#define mfc_cpu_h

#include "mfc_sub.h"

//===========================================================================
//
//	ヒストリ付きダイアログ
//
//===========================================================================
class CHistoryDlg : public CDialog
{
public:
	CHistoryDlg(UINT nID, CWnd *pParentWnd);
										// コンストラクタ
	BOOL OnInitDialog();
										// ダイアログ初期化
	void OnOK();
										// OK
	DWORD m_dwValue;
										// エディット値

protected:
	virtual UINT* GetNumPtr() = 0;
										// ヒストリ個数ポインタ取得
	virtual DWORD* GetDataPtr() = 0;
										// ヒストリデータポインタ取得
	UINT m_nBit;
										// 有効ビット
	DWORD m_dwMask;
										// マスク(ビットから内部生成)
};

//===========================================================================
//
//	アドレス入力ダイアログ
//
//===========================================================================
class CAddrDlg : public CHistoryDlg
{
public:
	CAddrDlg(CWnd *pParent = NULL);
										// コンストラクタ
	static void SetupHisMenu(CMenu *pMenu);
										// メニューセットアップ
	static DWORD GetAddr(UINT nID);
										// メニュー結果取得

protected:
	UINT* GetNumPtr();
										// ヒストリ個数ポインタ取得
	DWORD* GetDataPtr();
										// ヒストリデータポインタ取得
	static UINT m_Num;
										// ヒストリ個数
	static DWORD m_Data[10];
										// ヒストリデータ
};

//===========================================================================
//
//	レジスタ入力ダイアログ
//
//===========================================================================
class CRegDlg : public CHistoryDlg
{
public:
	CRegDlg(CWnd *pParent = NULL);
										// コンストラクタ
	BOOL OnInitDialog();
										// ダイアログ初期化
	void OnOK();
										// OK
	UINT m_nIndex;
										// レジスタインデックス

protected:
	UINT* GetNumPtr();
										// ヒストリ個数ポインタ取得
	DWORD* GetDataPtr();
										// ヒストリデータポインタ取得
	static UINT m_Num;
										// ヒストリ個数
	static DWORD m_Data[10];
										// ヒストリデータ
};

//===========================================================================
//
//	データ入力ダイアログ
//
//===========================================================================
class CDataDlg : public CHistoryDlg
{
public:
	CDataDlg(CWnd *pParent = NULL);
										// コンストラクタ
	BOOL OnInitDialog();
										// ダイアログ初期化
	UINT m_nSize;
										// サイズ
	DWORD m_dwAddr;
										// アドレス

protected:
	UINT* GetNumPtr();
										// ヒストリ個数ポインタ取得
	DWORD* GetDataPtr();
										// ヒストリデータポインタ取得
	static UINT m_Num;
										// ヒストリ個数
	static DWORD m_Data[10];
										// ヒストリデータ
};

//===========================================================================
//
//	MPUレジスタウィンドウ
//
//===========================================================================
class CCPURegWnd : public CSubTextWnd
{
public:
	CCPURegWnd();
										// コンストラクタ
	void FASTCALL Setup();
										// セットアップ
	static void SetupRegMenu(CMenu *pMenu, CPU *pCPU, BOOL bSR);
										// メニューセットアップ
	static DWORD GetRegValue(CPU *pCPU, UINT uID);
										// レジスタ値取得

protected:
	afx_msg void OnContextMenu(CWnd *pWnd, CPoint point);
										// コンテキストメニュー
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
										// 左ダブルクリック
	afx_msg void OnReg(UINT nID);
										// レジスタ選択

private:
	CPU *m_pCPU;
										// CPU

	DECLARE_MESSAGE_MAP()
										// メッセージ マップあり
};

//===========================================================================
//
//	割り込みウィンドウ
//
//===========================================================================
class CIntWnd : public CSubTextWnd
{
public:
	CIntWnd();
										// コンストラクタ
	void FASTCALL Setup();
										// セットアップ

protected:
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
										// 左ダブルクリック
	afx_msg void OnContextMenu(CWnd *pWnd, CPoint point);
										// コンテキストメニュー

private:
	CPU* m_pCPU;
										// CPU

	DECLARE_MESSAGE_MAP()
										// メッセージ マップあり
};

//===========================================================================
//
//	逆アセンブルウィンドウ
//
//===========================================================================
class CDisasmWnd : public CSubTextScrlWnd
{
public:
	CDisasmWnd(int index);
										// コンストラクタ
	void FASTCALL Setup();
										// セットアップ
	void FASTCALL SetAddr(DWORD dwAddr);
										// アドレス指定
	void FASTCALL SetPC(DWORD pc);
										// PC指定
	void FASTCALL Update();
										// メッセージスレッドからの更新
	static void FASTCALL SetupBreakMenu(CMenu *pMenu, Scheduler *pScheduler);
										// ブレークポイントメニュー設定
	static DWORD FASTCALL GetBreak(UINT nID, Scheduler *pScheduler);
										// ブレークポイント取得

protected:
	afx_msg int OnCreate(LPCREATESTRUCT lpcs);
										// ウィンドウ作成
	afx_msg void OnDestroy();
										// ウィンドウ削除
	afx_msg void OnSize(UINT nType, int cx, int cy);
										// サイズ変更
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
										// 左クリック
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
										// 左ダブルクリック
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar *pBar);
										// 垂直スクロール
	afx_msg void OnContextMenu(CWnd *pWnd, CPoint point);
										// コンテキストメニュー
	afx_msg void OnNewWin();
										// 新しいウィンドウ
	afx_msg void OnPC();
										// PCへ移動
	afx_msg void OnSync();
										// PCに同期
	afx_msg void OnAddr();
										// アドレス入力
	afx_msg void OnReg(UINT nID);
										// レジスタ
	afx_msg void OnStack(UINT nID);
										// スタック
	afx_msg void OnBreak(UINT nID);
										// ブレークポイント
	afx_msg void OnHistory(UINT nID);
										// アドレスヒストリ
	afx_msg void OnCPUExcept(UINT nID);
										// CPU例外ベクタ
	afx_msg void OnTrap(UINT nID);
										// trapベクタ
	afx_msg void OnMFP(UINT nID);
										// MFPベクタ
	afx_msg void OnSCC(UINT nID);
										// SCCベクタ
	afx_msg void OnDMAC(UINT uID);
										// DMACベクタ
	afx_msg void OnIOSC(UINT uID);
										// IOSCベクタ

private:
	DWORD FASTCALL GetPrevAddr(DWORD dwAddr);
										// 手前のアドレスを取得
	void FASTCALL SetupContext(CMenu *pMenu);
										// コンテキストメニューセットアップ
	void FASTCALL SetupVector(CMenu *pMenu, UINT index, DWORD vector, int num);
										// 割り込みベクタセットアップ
	void FASTCALL SetupAddress(CMenu *pMenu, UINT index, DWORD addr);
										// アドレスセットアップ
	void FASTCALL OnVector(UINT vector);
										// ベクタ指定
	CPU *m_pCPU;
										// CPU
	Scheduler *m_pScheduler;
										// スケジューラ
	MFP *m_pMFP;
										// MFP
	Memory *m_pMemory;
										// メモリ
	SCC * m_pSCC;
										// SCC
	DMAC *m_pDMAC;
										// DMAC
	IOSC *m_pIOSC;
										// IOSC
	BOOL m_bSync;
										// PC同期フラグ
	DWORD m_dwPC;
										// PC
	DWORD m_dwAddr;
										// 表示開始アドレス
	DWORD m_dwSetAddr;
										// セットされたアドレス
	DWORD *m_pAddrBuf;
										// アドレスバッファ
	CString m_Caption;
										// キャプション文字列
	CString m_CaptionSet;
										// キャプション設定文字列

	DECLARE_MESSAGE_MAP()
										// メッセージ マップあり
};

//===========================================================================
//
//	メモリウィンドウ
//
//===========================================================================
class CMemoryWnd : public CSubTextScrlWnd
{
public:
	CMemoryWnd(int nWnd);
										// コンストラクタ
	void FASTCALL Setup();
										// セットアップ
	void FASTCALL SetAddr(DWORD dwAddr);
										// アドレス指定
	void FASTCALL SetUnit(int nUnit);
										// 表示単位指定
	void FASTCALL Update();
										// メッセージスレッドからの更新
	static void SetupStackMenu(CMenu *pMenu, Memory *pMemory, CPU *pCPU);
										// スタックメニューセットアップ
	static DWORD GetStackAddr(UINT nID, Memory *pMemory, CPU *pCPU);
										// スタック取得

protected:
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
										// ウィンドウ作成
	afx_msg void OnPaint();
										// 描画
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
										// 左ダブルクリック
	afx_msg void OnContextMenu(CWnd *pWnd, CPoint point);
										// コンテキストメニュー
	afx_msg void OnAddr();
										// アドレス入力
	afx_msg void OnNewWin();
										// 新しいウィンドウ
	afx_msg void OnUnit(UINT uID);
										// 表示単位指定
	afx_msg void OnRange(UINT uID);
										// アドレス範囲指定
	afx_msg void OnReg(UINT uID);
										// レジスタ値を指定
	afx_msg void OnArea(UINT uID);
										// エリア指定
	afx_msg void OnHistory(UINT uID);
										// アドレスヒストリ
	afx_msg void OnStack(UINT uID);
										// スタック
	void FASTCALL SetupScrlV();
										// スクロール準備(垂直)

private:
	void FASTCALL SetupContext(CMenu *pMenu);
										// コンテキストメニュー セットアップ
	Memory *m_pMemory;
										// メモリ
	CPU *m_pCPU;
										// CPU
	DWORD m_dwAddr;
										// 表示開始アドレス
	CString m_strCaptionReq;
										// キャプション文字列(要求)
	CString m_strCaptionSet;
										// キャプション文字列(設定)
	CCriticalSection m_CSection;
										// クリティカルセクション
	UINT m_nUnit;
										// 表示サイズ0/1/2=Byte/Word/Long

	DECLARE_MESSAGE_MAP()
										// メッセージ マップあり
};

//===========================================================================
//
//	ブレークポイントウィンドウ
//
//===========================================================================
class CBreakPWnd : public CSubTextWnd
{
public:
	CBreakPWnd();
										// コンストラクタ
	void FASTCALL Setup();
										// セットアップ

protected:
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
										// 左ダブルクリック
	afx_msg void OnContextMenu(CWnd *pWnd, CPoint point);
										// コンテキストメニュー
	afx_msg void OnEnable();
										// 有効・無効
	afx_msg void OnClear();
										// 回数クリア
	afx_msg void OnDel();
										// 削除
	afx_msg void OnAddr();
										// アドレス指定
	afx_msg void OnAll();
										// 全て削除
	afx_msg void OnHistory(UINT nID);
										// アドレスヒストリ

private:
	void FASTCALL SetupContext(CMenu *pMenu);
										// コンテキストメニュー セットアップ
	void FASTCALL SetAddr(DWORD dwAddr);
										// アドレス設定
	Scheduler* m_pScheduler;
										// スケジューラ
	CPoint m_Point;
										// コンテキストメニューポイント

	DECLARE_MESSAGE_MAP()
										// メッセージ マップあり
};

#endif	// mfc_cpu_h
#endif	// _WIN32
