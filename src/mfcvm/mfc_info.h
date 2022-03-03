//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 ＰＩ．(ytanaka@ipc-tokai.or.jp)
//	[ MFC Info ]
//
//---------------------------------------------------------------------------

#if defined(_WIN32)

#if !defined(mfc_info_h)
#define mfc_info_h

#include "mfc_com.h"

//===========================================================================
//
//	Info
//
//===========================================================================
class CInfo : public CComponent
{
public:
	// 定数値
	enum {
		InfoBufMax = 100				// 情報バッファ最大キャラクタ数
	};

public:
	// 基本ファンクション
	CInfo(CFrmWnd *pWnd, CStatusBar *pBar);
										// コンストラクタ
	BOOL FASTCALL Init();
										// 初期化
	void FASTCALL Cleanup();
										// クリーンアップ
	void FASTCALL ApplyCfg(const Config *pConfig);
										// 設定適用
	void FASTCALL Enable(BOOL bEnable);
										// 動作制御

	// キャプション
	void FASTCALL ResetCaption();
										// キャプションリセット
	void FASTCALL UpdateCaption();
										// キャプション更新

	// 情報
	void FASTCALL SetInfo(CString& strInfo);
										// 情報設定
	void FASTCALL SetMessageString(const CString& strMessage) const;
										// 通常メッセージ設定
	void FASTCALL UpdateInfo();
										// 情報更新

	// ステータス
	void FASTCALL ResetStatus();
										// ステータスリセット
	void FASTCALL UpdateStatus();
										// ステータス更新
	void FASTCALL DrawStatus(int nPane, HDC hDC, CRect& rectDraw);
										// ステータス描画

	// ステータスビュー
	void FASTCALL SetStatusView(CStatusView *pView);
										// ステータスビュー設定

private:
	// 定数値
	enum {
		CapTimeLong = 1500,				// キャプション更新時間(通常)
		CapTimeShort = 300,				// キャプション更新時間(初回)
		InfoTime = 2000,				// 情報表示時間
		PaneMax = 5,					// ステータス最大ペイン数
		DiskTypeTime = 12				// ディスク種別保持時間
	};

	// キャプション
	BOOL FASTCALL CheckParcent();
										// ％チェック
	BOOL FASTCALL CheckVM();
										// VMファイルチェック
	BOOL FASTCALL CheckMO();
										// MOファイルチェック
	BOOL FASTCALL CheckCD();
										// CDファイルチェック
	void FASTCALL SetCaption();
										// キャプション設定
	CString m_strRun;
										// 動作中メッセージ
	CString m_strStop;
										// 停止中メッセージ
	BOOL m_bRun;
										// スケジューラ動作中
	BOOL m_bCount;
										// 動作％計測中
	int m_nParcent;
										// 動作％(-1は非表示)
	DWORD m_dwTick;
										// ％計測(GetTickCount)
	DWORD m_dwTime;
										// ％計測(GetTotalTime)
	TCHAR m_szVM[_MAX_PATH];
										// VMファイルパス
	TCHAR m_szVMFull[_MAX_PATH];
										// VMファイルパス(フル)
	TCHAR m_szMO[_MAX_PATH];
										// MOファイルパス
	TCHAR m_szMOFull[_MAX_PATH];
										// MOファイルパス(フル)
	TCHAR m_szCD[_MAX_PATH];
										// CDファイルパス
	TCHAR m_szCDFull[_MAX_PATH];
										// CDファイルパス(フル)
	CScheduler *m_pSch;
										// スケジューラ(Win)
	Scheduler *m_pScheduler;
										// スケジューラ
	SASI *m_pSASI;
										// SASI
	SCSI *m_pSCSI;
										// SCSI

	// 情報
	BOOL m_bInfo;
										// 情報の有無
	BOOL m_bPower;
										// 電源のON/OFF
	CString m_strInfo;
										// 情報文字列
	CString m_strPower;
										// 電源OFF文字列
	TCHAR m_szInfo[InfoBufMax];
										// 通常文字列
	DWORD m_dwInfo;
										// 情報表示時間

	// ステータス
	COLORREF FASTCALL StatusFloppy(LPTSTR szText, int nDrive) const;
										// ステータスFD
	COLORREF FASTCALL StatusHardDisk(LPTSTR szText);
										// ステータスHD
	COLORREF FASTCALL StatusTimer(LPTSTR szText) const;
										// ステータスTIMER
	COLORREF FASTCALL StatusPower(LPTSTR szText) const;
										// ステータスPOWER
	COLORREF m_colStatus[PaneMax];
										// ステータス表示色
	TCHAR m_szStatus[PaneMax][_MAX_PATH];
										// ステータス文字列
	BYTE m_bmpDrive[2][0x100];
										// ステータスドライブBMP
	FDD *m_pFDD;
										// FDD
	RTC *m_pRTC;
										// RTC
	DWORD m_dwNumber;
										// 更新ナンバ
	DWORD m_dwDiskID;
										// ディスクBUSY種別
	DWORD m_dwDiskTime;
										// ディスク種別保持時間

	// ステータスバー
	CStatusBar *m_pStatusBar;
										// ステータスバー

	// ステータスビュー
	CStatusView *m_pStatusView;
										// ステータスビュー

	// コンフィギュレーション
	BOOL m_bFloppyLED;
										// モータONでLED点灯
	BOOL m_bPowerLED;
										// 青色電源LED
	BOOL m_bCaptionInfo;
										// キャプションへの情報表示
};

#endif	// mfc_info_h
#endif	// _WIN32
