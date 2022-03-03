//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2004 ＰＩ．(ytanaka@ipc-tokai.or.jp)
//	[ MFC TrueKey ]
//
//---------------------------------------------------------------------------

#if defined(_WIN32)

#if !defined(mfc_tkey_h)
#define mfc_tkey_h

#include "fileio.h"
#include "mfc_que.h"

//===========================================================================
//
//	TrueKey
//
//===========================================================================
class CTKey : public CComponent
{
public:
	// 基本ファンクション
	CTKey(CFrmWnd *pWnd);
										// コンストラクタ
	BOOL FASTCALL Init();
										// 初期化
	void FASTCALL Cleanup();
										// クリーンアップ
	void FASTCALL ApplyCfg(const Config *pConfig);
										// 設定適用

	// 外部API
	void FASTCALL Run();
										// スレッド実行
	void FASTCALL GetTxQueue(CQueue::LPQUEUEINFO lpqi) const;
										// 送信キュー情報取得
	void FASTCALL GetRxQueue(CQueue::LPQUEUEINFO lpqi) const;
										// 受信キュー情報取得

	// キーマップ
	void FASTCALL GetKeyMap(int *pMap);
										// キーマップ取得
	void FASTCALL SetKeyMap(const int *pMap);
										// キーマップ設定
	LPCTSTR FASTCALL GetKeyID(int nVK);
										// VKキーID取得

private:
	typedef struct _VKEYID {
		int nVK;						// 仮想キーコード
		LPCSTR lpszID;					// 仮想キー名
	} VKEYID, *PVKEYID;
	enum {
		KeyMax = 0x73					// キー総数(1<=nKey<=KeyMax)
	};

	BOOL FASTCALL Open();
										// ポートオープン
	void FASTCALL Close();
										// ポートクローズ
	void FASTCALL Ctrl(BOOL bForce);
										// RTS制御
	void FASTCALL Rx();
										// 受信
	void FASTCALL Tx();
										// 送信
	void FASTCALL BufSync();
										// バッファ同期

	// パラメータ
	int m_nMode;
										// モード(bit0:VM bit1:Win32)
	int m_nCOM;
										// ポート番号(0で使用しない)
	BOOL m_bRTS;
										// RTS論理(送信)
	BOOL m_bLine;
										// RTSライン状態

	// ハンドル・スレッド
	HANDLE m_hCOM;
										// ファイルハンドル
	CWinThread *m_pCOM;
										// スレッド
	BOOL m_bReq;
										// スレッド終了要求

	// 送信制御
	CQueue m_TxQueue;
										// 送信キュー
	OVERLAPPED m_TxOver;
										// オーバーラップ
	BOOL m_bTxValid;
										// 送信有効フラグ
	BYTE m_TxBuf[0x1000];
										// 送信オーバーラップバッファ

	// 受信制御
	CQueue m_RxQueue;
										// 受信キュー
	OVERLAPPED m_RxOver;
										// オーバーラップ
	BYTE m_RxBuf[0x1000];
										// 受信オーバーラップバッファ

	// キーマップ
	BOOL m_bKey[KeyMax];
										// キー押下フラグ
	BOOL m_bWin[KeyMax];
										// Windowsキー押下フラグ
	int m_nKey[KeyMax];
										// キー変換テーブル
	static const int KeyTable[KeyMax];
										// キー変換テーブル(デフォルト)
	static const VKEYID KeyIDTable[];
										// VKキーIDテーブル

	// オブジェクト
	MFP *m_pMFP;
										// MFP
	Keyboard *m_pKeyboard;
										// キーボード
	CScheduler *m_pSch;
										// スケジューラ
};

#endif	// mfc_tkey_h
#endif	// _WIN32
