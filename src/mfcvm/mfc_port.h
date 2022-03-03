//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2004 ＰＩ．(ytanaka@ipc-tokai.or.jp)
//	[ MFC ポート ]
//
//---------------------------------------------------------------------------

#if defined(_WIN32)

#if !defined(mfc_port_h)
#define mfc_port_h

#include "fileio.h"
#include "mfc_que.h"

//===========================================================================
//
//	ポート
//
//===========================================================================
class CPort : public CComponent
{
public:
	// 基本ファンクション
	CPort(CFrmWnd *pWnd);
										// コンストラクタ
	BOOL FASTCALL Init();
										// 初期化
	void FASTCALL Cleanup();
										// クリーンアップ
	void FASTCALL ApplyCfg(const Config *pConfig);
										// 設定適用

	// 外部API
	void FASTCALL RunCOM();
										// COM実行
	BOOL FASTCALL GetCOMInfo(LPTSTR lpszDevFile, DWORD *dwLogFile) const;
										// COM情報取得
	void FASTCALL GetTxQueue(CQueue::LPQUEUEINFO lpqi) const;
										// 送信キュー情報取得
	void FASTCALL GetRxQueue(CQueue::LPQUEUEINFO lpqi) const;
										// 受信キュー情報取得
	void FASTCALL RunLPT();
										// LPT実行
	BOOL FASTCALL GetLPTInfo(LPTSTR lpszDevFile, DWORD *dwLogFile) const;
										// LPT情報取得
	void FASTCALL GetLPTQueue(CQueue::LPQUEUEINFO lpqi) const;
										// LPTキュー取得

private:
	// シリアルポート
	BOOL FASTCALL OpenCOM();
										// COMオープン
	void FASTCALL AppendCOM();
										// COMファイル
	void FASTCALL CloseCOM();
										// COMクローズ
	void FASTCALL AdjustCOM(DCB *pDCB);
										// パラメータあわせ
	BOOL FASTCALL MatchCOM(DWORD dwSCC, DWORD dwBase);
										// ボーレートマッチ
	void FASTCALL SignalCOM();
										// 信号線あわせ
	void FASTCALL BufCOM();
										// バッファあわせ
	void FASTCALL CtrlCOM();
										// 信号線制御
	void FASTCALL OnCTSDSR();
										// CTS, DSR変化
	void FASTCALL OnErr();
										// 受信エラー
	void FASTCALL OnRx();
										// 受信サブ
	void FASTCALL OnTx();
										// 送信サブ
	UINT m_nCOM;
										// COMポート(0で使用しない)
	HANDLE m_hCOM;
										// ファイルハンドル
	CWinThread *m_pCOM;
										// スレッド
	BOOL m_bCOMReq;
										// スレッド終了要求
	BOOL m_bBreak;
										// X68000送出ブレーク
	BOOL m_bRTS;
										// RTS(送信)
	BOOL m_bDTR;
										// DTR(送信)
	BOOL m_bCTS;
										// CTS(受信)
	BOOL m_bDSR;
										// DSR(受信)
	OVERLAPPED m_TxOver;
										// オーバーラップ
	CQueue m_TxQueue;
										// 送信キュー
	BYTE m_TxBuf[0x1000];
										// 送信オーバーラップバッファ
	BOOL m_bTxValid;
										// 送信有効フラグ
	OVERLAPPED m_RxOver;
										// オーバーラップ
	CQueue m_RxQueue;
										// 受信キュー
	BYTE m_RxBuf[0x1000];
										// 受信オーバーラップバッファ
	DWORD m_dwErr;
										// 受信エラー
	TCHAR m_RecvLog[_MAX_PATH];
										// 受信ログ
	Fileio m_RecvFile;
										// 受信ログ
	BOOL m_bForce;
										// 強制38400bpsモード
	SCC *m_pSCC;
										// SCC

	// パラレルポート
	BOOL FASTCALL OpenLPT();
										// LPTオープン
	void FASTCALL AppendLPT();
										// LPTファイル
	void FASTCALL CloseLPT();
										// LPTクローズ
	void FASTCALL RecvLPT();
										// LPT受信
	void FASTCALL SendLPT();
										// LPT送信

	UINT m_nLPT;
										// LPTポート(0で使用しない)
	HANDLE m_hLPT;
										// ファイルハンドル
	CWinThread *m_pLPT;
										// スレッド
	BOOL m_bLPTReq;
										// スレッド終了要求
	BOOL m_bLPTValid;
										// 送信有効フラグ
	OVERLAPPED m_LPTOver;
										// オーバーラップ
	CQueue m_LPTQueue;
										// キュー
	TCHAR m_SendLog[_MAX_PATH];
										// 送信ログ
	Fileio m_SendFile;
										// 送信ログ
	Printer *m_pPrinter;
										// プリンタ
};

#endif	// mfc_port_h
#endif	// _WIN32
