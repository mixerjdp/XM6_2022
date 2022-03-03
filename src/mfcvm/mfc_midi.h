//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 ＰＩ．(ytanaka@ipc-tokai.or.jp)
//	[ MFC MIDI ]
//
//---------------------------------------------------------------------------

#if defined(_WIN32)

#if !defined(mfc_midi_h)
#define mfc_midi_h

#include "mfc_com.h"
#include "mfc_que.h"

//===========================================================================
//
//	MIDI
//
//===========================================================================
class CMIDI : public CComponent
{
public:
	// 情報送信型定義
	typedef struct _MIDIINFO {
		DWORD dwDevices;				// デバイス数
		DWORD dwDevice;					// デバイス番号
		DWORD dwHandle;					// ハンドル(HMIDI*)
		CWinThread *pThread;			// スレッド
		DWORD dwShort;					// ショートメッセージカウンタ
		DWORD dwLong;					// エクスクルーシブカウンタ
		DWORD dwUnprepare;				// ヘッダ解放カウンタ
		DWORD dwBufNum;					// バッファ有効数
		DWORD dwBufRead;				// バッファ読み取り位置
		DWORD dwBufWrite;				// バッファ書き込み位置
	} MIDIINFO, *LPMIDIINFO;

public:
	// 基本ファンクション
	CMIDI(CFrmWnd *pWnd);
										// コンストラクタ
	BOOL FASTCALL Init();
										// 初期化
	void FASTCALL Cleanup();
										// クリーンアップ
	void FASTCALL ApplyCfg(const Config *pConfig);
										// 設定適用
#if !defined(NDEBUG)
	void AssertValid() const;
										// 診断
#endif	// NDEBUG

	// コールバック向け
	HMIDIOUT GetOutHandle() const		{ return m_hOut; }
										// Outデバイスハンドル取得
	HMIDIIN GetInHandle() const			{ return m_hIn; }
										// Inデバイスハンドル取得

	// デバイス
	DWORD FASTCALL GetOutDevs() const;
										// Outデバイス数取得
	BOOL FASTCALL GetOutDevDesc(int nDevice, CString& strDesc) const;
										// Outデバイス名称取得
	DWORD FASTCALL GetInDevs() const;
										// INデバイス数取得
	BOOL FASTCALL GetInDevDesc(int nDevice, CString& strDesc) const;
										// INデバイス名称取得

	// ディレイ
	void FASTCALL SetOutDelay(int nDelay);
										// Outディレイ設定
	void FASTCALL SetInDelay(int nDelay);
										// Inディレイ設定

	// ミキサ
	int FASTCALL GetOutVolume();
										// Out音量取得
	void FASTCALL SetOutVolume(int nVolume);
										// Out音量設定

	// 情報取得
	void FASTCALL GetOutInfo(LPMIDIINFO pInfo) const;
										// Out情報取得
	void FASTCALL GetInInfo(LPMIDIINFO pInfo) const;
										// In情報取得

private:
	MIDI *m_pMIDI;
										// MIDI
	Scheduler *m_pScheduler;
										// スケジューラ
	CScheduler *m_pSch;
										// スケジューラ

	// Out
	enum OutState {
		OutReady,						// データ待ち状態なし
		OutEx,							// エクスクルーシブメッセージ出力中
		OutShort,						// ショートメッセージ出力中
	};
	void FASTCALL OpenOut(DWORD dwDevice);
										// Outオープン
	void FASTCALL CloseOut();
										// Outクローズ
	void FASTCALL RunOut();
										// Outスレッド
	void FASTCALL CallbackOut(UINT wMsg, DWORD dwParam1, DWORD dwParam2);
										// Outコールバック
	void FASTCALL GetOutCaps();
										// OutデバイスCaps取得
	BOOL FASTCALL SendEx(const BYTE *pExData);
										// エクスクルーシブ送信
	BOOL FASTCALL SendExWait();
										// エクスクルーシブ完了待ち
	void FASTCALL SendAllNoteOff();
										// 全ノートオフ
	BOOL FASTCALL SendReset();
										// リセット送信
	static UINT OutThread(LPVOID pParam);
										// Outスレッド
#if _MFC_VER >= 0x700
	static void CALLBACK OutProc(HMIDIOUT hOut, UINT wMsg, DWORD_PTR dwInstance,
		DWORD dwParam1, DWORD dwParam2);// Outコールバック
#else
	static void CALLBACK OutProc(HMIDIOUT hOut, UINT wMsg, DWORD dwInstance,
		DWORD dwParam1, DWORD dwParam2);// Outコールバック
#endif
	DWORD m_dwOutDevice;
										// OutデバイスiD
	HMIDIOUT m_hOut;
										// Outデバイスハンドル
	CWinThread *m_pOutThread;
										// Outスレッド
	BOOL m_bOutThread;
										// Outスレッド終了フラグ
	DWORD m_dwOutDevs;
										// Outデバイス数
	LPMIDIOUTCAPS m_pOutCaps;
										// OutデバイスCaps
	DWORD m_dwOutDelay;
										// Outディレイ(ms)
	BOOL m_bSendEx;
										// エクスクルーシブ送信フラグ
	BOOL m_bSendExHdr;
										// エクスクルーシブヘッダ使用フラグ
	BYTE m_ExBuf[0x2000];
										// エクスクルーシブバッファ
	MIDIHDR m_ExHdr;
										// エクスクルーシブヘッダ
	int m_nOutReset;
										// リセットコマンド種別
	DWORD m_dwShortSend;
										// ショートコマンド送信カウンタ
	DWORD m_dwExSend;
										// エクスクルーシブ送信試行カウンタ
	DWORD m_dwUnSend;
										// ヘッダ解放カウンタ
	CCriticalSection m_OutSection;
										// クリティカルセクション
	static const BYTE ResetGM[];
										// GMリセットコマンド
	static const BYTE ResetGS[];
										// GSリセットコマンド
	static const BYTE ResetXG[];
										// XGリセットコマンド
	static const BYTE ResetLA[];
										// LAリセットコマンド

	// In
	enum InState {
		InNotUsed,						// 未使用
		InReady,						// データ待ち
		InDone							// データあり
	};
	enum {
		InBufMax = 0x800				// バッファサイズ
	};
	void FASTCALL OpenIn(DWORD dwDevice);
										// Inオープン
	void FASTCALL CloseIn();
										// Inクローズ
	void FASTCALL RunIn();
										// Inスレッド
	void FASTCALL CallbackIn(UINT wMsg, DWORD dwParam1, DWORD dwParam2);
										// Inコールバック
	void FASTCALL GetInCaps();
										// InデバイスCaps取得
	BOOL FASTCALL StartIn();
										// In開始
	void FASTCALL StopIn();
										// In停止
	void FASTCALL ShortIn();
										// Inショートメッセージ
	void FASTCALL LongIn(int nHdr);
										// Inロングメッセージ
	static UINT InThread(LPVOID pParam);
										// Inスレッド
#if _MFC_VER >= 0x700
	static void CALLBACK InProc(HMIDIIN hIn, UINT wMsg, DWORD_PTR dwInstance,
		DWORD dwParam1, DWORD dwParam2);// Inコールバック
#else
	static void CALLBACK InProc(HMIDIIN hIn, UINT wMsg, DWORD dwInstance,
		DWORD dwParam1, DWORD dwParam2);// Inコールバック
#endif
	DWORD m_dwInDevice;
										// InデバイスiD
	HMIDIIN m_hIn;
										// Inデバイスハンドル
	CWinThread *m_pInThread;
										// InTスレッド
	BOOL m_bInThread;
										// Inスレッド終了フラグ
	DWORD m_dwInDevs;
										// Inデバイス数
	LPMIDIINCAPS m_pInCaps;
										// InデバイスCaps
	MIDIHDR m_InHdr[2];
										// エクスクルーシブヘッダ
	BYTE m_InBuf[2][InBufMax];
										// エクスクルーシブバッファ
	InState m_InState[2];
										// エクスクルーシブバッファ状態
	CQueue m_InQueue;
										// ショートメッセージ入力キュー
	DWORD m_dwShortRecv;
										// ショートコマンド受信カウンタ
	DWORD m_dwExRecv;
										// エクスクルーシブ受信カウンタ
	DWORD m_dwUnRecv;
										// ヘッダ解放カウンタ
	CCriticalSection m_InSection;
										// クリティカルセクション
};

#endif	// mfc_midi_h
#endif	// _WIN32
