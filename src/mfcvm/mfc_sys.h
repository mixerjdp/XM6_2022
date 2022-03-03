//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 ＰＩ．(ytanaka@ipc-tokai.or.jp)
//	[ MFC サブウィンドウ(システム) ]
//
//---------------------------------------------------------------------------

#if defined(_WIN32)

#if !defined(mfc_sys_h)
#define mfc_sys_h

#include "mfc_sub.h"
#include "log.h"

//===========================================================================
//
//	ログウィンドウ
//
//===========================================================================
class CLogWnd : public CSubListWnd
{
public:
	CLogWnd();
										// コンストラクタ
	void FASTCALL Refresh();
										// リフレッシュ描画
	void FASTCALL Update();
										// メッセージスレッドからの更新

protected:
	afx_msg int OnCreate(LPCREATESTRUCT lpcs);
										// ウィンドウ作成
	afx_msg void OnDblClick(NMHDR *pNotifyStruct, LRESULT *result);
										// ダブルクリック
	afx_msg void OnDrawItem(int nID, LPDRAWITEMSTRUCT lpdis);
										// オーナー描画
	void FASTCALL InitList();
										// リストコントロール初期化

private:
	void FASTCALL TextSub(int nType, Log::logdata_t *pLogData, CString& string);
										// 文字列セット
	Log *m_pLog;
										// ログ
	CString m_strDetail;
										// 詳細
	CString m_strNormal;
										// 通常
	CString m_strWarning;
										// 警告
	int m_nFocus;
										// フォーカス番号

	DECLARE_MESSAGE_MAP()
										// メッセージ マップあり
};

//===========================================================================
//
//	スケジューラウィンドウ
//
//===========================================================================
class CSchedulerWnd : public CSubTextWnd
{
public:
	CSchedulerWnd();
										// コンストラクタ
	void FASTCALL Setup();
										// セットアップ
	void FASTCALL Update();
										// メッセージスレッドから更新

private:
	Scheduler *m_pScheduler;
										// スケジューラ
};

//===========================================================================
//
//	デバイスウィンドウ
//
//===========================================================================
class CDeviceWnd : public CSubTextWnd
{
public:
	CDeviceWnd();
										// コンストラクタ
	void FASTCALL Setup();
										// セットアップ
};

#endif	// mfc_sys_h
#endif	// _WIN32
