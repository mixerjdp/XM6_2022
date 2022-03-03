//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 ＰＩ．(ytanaka@ipc-tokai.or.jp)
//	[ MFC MIDI ]
//
//---------------------------------------------------------------------------

#if defined(_WIN32)

#include "os.h"
#include "xm6.h"
#include "vm.h"
#include "midi.h"
#include "config.h"
#include "mfc_com.h"
#include "mfc_sch.h"
#include "mfc_cfg.h"
#include "mfc_midi.h"

//===========================================================================
//
//	MIDI
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	MIDIメッセージ定義
//
//---------------------------------------------------------------------------
#define MIDI_NOTEOFF		0x80		// ノートオフ
#define MIDI_NOTEON			0x90		// ノートオン(一部ノートオフ)
#define MIDI_PKEYPRS		0xa0		// ポリフォニックキープレッシャー
#define MIDI_CTRLCHG		0xb0		// コントロールチェンジ
#define MIDI_PROGCHG		0xc0		// プログラムチェンジ
#define MIDI_CHPRS			0xd0		// チャネルプレッシャー
#define MIDI_PITCHCHG		0xe0		// ピッチベンドチェンジ
#define MIDI_EXSTART		0xf0		// エクスクルーシブ開始
#define MIDI_QFRAME			0xf1		// タイムコードクォーターフレーム
#define MIDI_SONGPTR		0xf2		// ソングポジションポインタ
#define MIDI_SONGSEL		0xf3		// ソングセレクト
#define MIDI_TUNEREQ		0xf6		// チューンセレクト
#define MIDI_EXEND			0xf7		// エクスクルーシブ終了
#define MIDI_TIMINGCLK		0xf8		// タイミングクロック
#define MIDI_START			0xfa		// スタート
#define MIDI_CONTINUE		0xfb		// コンティニュー
#define MIDI_STOP			0xfc		// ストップ
#define MIDI_ACTSENSE		0xfe		// アクティブセンシング
#define MIDI_SYSRESET		0xff		// システムリセット

//---------------------------------------------------------------------------
//
//	コンストラクタ
//
//---------------------------------------------------------------------------
CMIDI::CMIDI(CFrmWnd *pWnd) : CComponent(pWnd)
{
	// コンポーネントパラメータ
	m_dwID = MAKEID('M', 'I', 'D', 'I');
	m_strDesc = _T("MIDI Driver");

	// オブジェクト
	m_pMIDI = NULL;
	m_pScheduler = NULL;
	m_pSch = NULL;

	// Out
	m_dwOutDevice = 0;
	m_hOut = NULL;
	m_pOutThread = NULL;
	m_bOutThread = FALSE;
	m_dwOutDevs = 0;
	m_pOutCaps = NULL;
	m_dwOutDelay = (84 * 2000);
	m_nOutReset = 0;
	m_dwShortSend = 0;
	m_dwExSend = 0;
	m_dwUnSend = 0;

	// In
	m_dwInDevice = 0;
	m_hIn = NULL;
	m_pInThread = NULL;
	m_bInThread = FALSE;
	m_dwInDevs = 0;
	m_pInCaps = NULL;
	m_InState[0] = InNotUsed;
	m_InState[1] = InNotUsed;
	m_dwShortRecv = 0;
	m_dwExRecv = 0;
	m_dwUnRecv = 0;
}

//---------------------------------------------------------------------------
//
//	初期化
//
//---------------------------------------------------------------------------
BOOL FASTCALL CMIDI::Init()
{
	ASSERT(this);

	// 基本クラス
	if (!CComponent::Init()) {
		return FALSE;
	}

	// MIDI取得
	ASSERT(!m_pMIDI);
	m_pMIDI = (MIDI*)::GetVM()->SearchDevice(MAKEID('M', 'I', 'D', 'I'));
	ASSERT(m_pMIDI);

	// スケジューラ取得
	ASSERT(!m_pScheduler);
	m_pScheduler = (Scheduler*)::GetVM()->SearchDevice(MAKEID('S', 'C', 'H', 'E'));
	ASSERT(m_pScheduler);

	// スケジューラ(Win32)取得
	ASSERT(!m_pSch);
	m_pSch = (CScheduler*)SearchComponent(MAKEID('S', 'C', 'H', 'E'));
	ASSERT(m_pSch);

	// Outデバイス情報
	m_dwOutDevs = ::midiOutGetNumDevs();
	if (m_dwOutDevs > 0) {
		
		try {
			m_pOutCaps = new MIDIOUTCAPS[m_dwOutDevs];
		}
		catch (...) {
			return FALSE;
		}
		if (!m_pOutCaps) {
			return FALSE;
		}

		// CAPSを取得
		GetOutCaps();
	}

	// Inデバイス情報
	m_dwInDevs = ::midiInGetNumDevs();
	if (m_dwInDevs > 0) {
		
		try {
			m_pInCaps = new MIDIINCAPS[m_dwInDevs];
		}
		catch (...) {
			return FALSE;
		}
		if (!m_pInCaps) {
			return FALSE;
		}

		// CAPSを取得
		GetInCaps();
	}

	// Inキュー初期化
	if (!m_InQueue.Init(InBufMax)) {
		return FALSE;
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	クリーンアップ
//
//---------------------------------------------------------------------------
void FASTCALL CMIDI::Cleanup()
{
	ASSERT(this);

	// クローズ
	CloseOut();
	CloseIn();

	// Caps削除
	if (m_pOutCaps) {
		delete[] m_pOutCaps;
		m_pOutCaps = NULL;
	}
	if (m_pInCaps) {
		delete[] m_pInCaps;
		m_pInCaps = NULL;
	}

	// 基本クラス
	CComponent::Cleanup();
}

//---------------------------------------------------------------------------
//
//	設定適用
//
//---------------------------------------------------------------------------
void FASTCALL CMIDI::ApplyCfg(const Config* pConfig)
{
	CConfig *pComponent;

	ASSERT(this);
	ASSERT(pConfig);
	ASSERT_VALID(this);

	// コンフィグコンポーネントを取得
	pComponent = (CConfig*)SearchComponent(MAKEID('C', 'F', 'G', ' '));
	ASSERT(pComponent);

	// リセットコマンド
	m_nOutReset = pConfig->midi_reset;

	// Outディレイ
	m_dwOutDelay = (DWORD)(pConfig->midiout_delay * 2000);

	// Outデバイス
	if (pConfig->midiout_device != (int)m_dwOutDevice) {
		// 一旦閉じる
		CloseOut();

		// 指定デバイスでトライ
		if (pConfig->midiout_device > 0) {
			OpenOut(pConfig->midiout_device);
		}

		// 成功すれば、コンフィグに反映させる
		if (m_dwOutDevice != 0) {
			pComponent->SetMIDIDevice((int)m_dwOutDevice, FALSE);
		}
	}

	// Inディレイ
	SetInDelay(pConfig->midiin_delay * 2000);

	// Inデバイス
	if (pConfig->midiin_device != (int)m_dwInDevice) {
		// 一旦閉じる
		CloseIn();

		// 指定デバイスでトライ
		if (pConfig->midiin_device > 0) {
			OpenIn(pConfig->midiin_device);
		}

		// 成功すれば、コンフィグに反映させる
		if (m_dwInDevice > 0) {
			pComponent->SetMIDIDevice((int)m_dwInDevice, TRUE);
		}
	}
}

#if !defined(NDEBUG)
//---------------------------------------------------------------------------
//
//	診断
//
//---------------------------------------------------------------------------
void CMIDI::AssertValid() const
{
	// 基本クラス
	CComponent::AssertValid();

	ASSERT(this);
	ASSERT(m_pMIDI);
	ASSERT(m_pMIDI->GetID() == MAKEID('M', 'I', 'D', 'I'));
	ASSERT(m_pScheduler);
	ASSERT(m_pScheduler->GetID() == MAKEID('S', 'C', 'H', 'E'));
	ASSERT(m_pSch);
	ASSERT(m_pSch->GetID() == MAKEID('S', 'C', 'H', 'E'));
	ASSERT((m_dwOutDevs == 0) || m_pOutCaps);
	ASSERT((m_dwInDevs == 0) || m_pInCaps);
	ASSERT((m_nOutReset >= 0) && (m_nOutReset <= 3));
}
#endif	// NDEBUG

//===========================================================================
//
//	OUT部分
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	Outオープン
//	※dwDevice=0は割り当てなし、dwDevice=1はMIDIマッパー
//
//---------------------------------------------------------------------------
void FASTCALL CMIDI::OpenOut(DWORD dwDevice)
{
	MMRESULT mmResult;
	UINT uDeviceID;

	ASSERT(this);
	ASSERT(m_dwOutDevice == 0);
	ASSERT(!m_pOutThread);
	ASSERT_VALID(this);

	// 割り当てしないのであればリターン
	if (dwDevice == 0) {
		return;
	}

	// デバイスがなければリターン
	if (m_dwOutDevs == 0) {
		return;
	}

	// デバイス数を超えていればMIDIマッパー
	if (dwDevice > (m_dwOutDevs + 1)) {
		dwDevice = 1;
	}

	// デバイス決定
	if (dwDevice == 1) {
		uDeviceID = MIDI_MAPPER;
	}
	else {
		uDeviceID = (UINT)(dwDevice - 2);
	}

	// オープン
#if _MFC_VER >= 0x700
	mmResult = ::midiOutOpen(&m_hOut, uDeviceID,
							(DWORD_PTR)OutProc, (DWORD_PTR)this, CALLBACK_FUNCTION);
#else
	mmResult = ::midiOutOpen(&m_hOut, uDeviceID,
							(DWORD)OutProc, (DWORD)this, CALLBACK_FUNCTION);
#endif

	// 失敗なら終了
	if (mmResult != MMSYSERR_NOERROR) {
		return;
	}

	// デバイス番号を更新
	m_dwOutDevice = dwDevice;

	// 転送バッファをクリア
	m_pMIDI->ClrTransData();

	// スレッドを立てる
	m_bOutThread = FALSE;
	m_pOutThread = AfxBeginThread(OutThread, this);
}

//---------------------------------------------------------------------------
//
//	Outクローズ
//
//---------------------------------------------------------------------------
void FASTCALL CMIDI::CloseOut()
{
	ASSERT(this);
	ASSERT_VALID(this);

	// スレッドを止める
	if (m_pOutThread) {
		// 終了フラグを上げ
		m_bOutThread = TRUE;

		// 終了待ち(無期限)
		::WaitForSingleObject(m_pOutThread->m_hThread, INFINITE);

		// スレッドなし
		m_pOutThread = NULL;
	}

	// デバイスを閉じる
	if (m_dwOutDevice != 0) {
		// 全てのノートをキーオフして、クローズ
		SendAllNoteOff();
		::midiOutReset(m_hOut);
		::midiOutClose(m_hOut);

		// オープンデバイスなし
		m_dwOutDevice = 0;
	}
}

//---------------------------------------------------------------------------
//
//	Outスレッド
//
//---------------------------------------------------------------------------
void FASTCALL CMIDI::RunOut()
{
	BOOL bEnable;
	BOOL bActive;
	OutState outState;
	DWORD dwTime;
	const MIDI::mididata_t *pData;
	DWORD dwLen;
	DWORD dwCnt;
	DWORD dwCmd;
	DWORD dwShortMsg;

	ASSERT(this);
	ASSERT(m_hOut);
	ASSERT_VALID(this);

	// スケジューラの状態を記憶
	bEnable = m_pSch->IsEnable();

	// エクスクルーシブ送信なし、ヘッダ使用なし
	m_bSendEx = FALSE;
	m_bSendExHdr = FALSE;

	// カウンタ初期化
	m_dwShortSend = 0;
	m_dwExSend = 0;
	m_dwUnSend = 0;

	// データなし、コマンド初期化、ステートレディ
	pData = NULL;
	dwCmd = 0;
	dwLen = 0;
	dwCnt = 0;
	dwShortMsg = 0;
	outState = OutReady;

	// リセット
	SendReset();

	// アクティブ(有効なデータをMIDI OUTへ送ったか)
	bActive = FALSE;

	// ループ
	while (!m_bOutThread) {
		// エクスクルーシブ送信中なら待つ
		if (m_bSendEx) {
			::Sleep(1);
			continue;
		}

		// エクスクルーシブ送信ヘッダを使った後なら、後片付け
		if (m_bSendExHdr) {
			// ロックして行う
			m_OutSection.Lock();
			::midiOutUnprepareHeader(m_hOut, &m_ExHdr, sizeof(MIDIHDR));
			m_OutSection.Unlock();

			// エクスクルーシブヘッダ使用フラグを下ろす
			m_bSendExHdr = FALSE;
			pData = NULL;
			outState = OutReady;

			// カウントアップ
			m_dwUnSend++;
		}

		// 無効なら
		if (!m_bEnable) {
			// アクティブなら、全ノートオフとリセット
			if (bActive) {
				SendAllNoteOff();
				SendReset();
				bActive = FALSE;

				// 有効データなし
				outState = OutReady;
				pData = NULL;
			}

			::Sleep(10);
			continue;
		}

		// スケジューラが無効なら
		if (!m_pSch->IsEnable()) {
			// それまで有効であったなら
			if (bEnable) {
				// 全ノートーオフ
				SendAllNoteOff();

				// それ以外の状態はキープ
				bEnable = FALSE;
			}

			// これまでも無効
			::Sleep(10);
			continue;
		}

		// スケジューラは有効
		bEnable = TRUE;

		// MIDIがリセットされたら
		if (m_pMIDI->IsReset()) {
			// フラグ落として
			m_pMIDI->ClrReset();

			// 全ノートオフとリセット
			SendAllNoteOff();
			SendReset();
			pData = NULL;
			outState = OutReady;
		}

		// MIDIがアクティブでなければ何もしない
		if (!m_pMIDI->IsActive()) {
			::Sleep(10);
			continue;
		}

		// MIDI送信バッファの個数を取得。0ならSleep
		if (!pData) {
			if (m_pMIDI->GetTransNum() == 0) {
				// 15ms以上ディレイがあるので、10msウェイト
				if (m_dwOutDelay > 30000) {
					::Sleep(10);
					continue;
				}
				// ディレイは14ms以下。1msウェイト
				::Sleep(1);
				continue;
			}

			// 送信データの先頭を取得
			pData = m_pMIDI->GetTransData(0);
		}

		// 1バイト以上データがあるので、アクティブとする
		bActive = TRUE;

		// レディで、エクスクルーシブコマンドでなければ、時間待ち
		if ((outState == OutReady) && (pData->data != MIDI_EXSTART)) {
			dwTime = m_pScheduler->GetTotalTime() - pData->vtime;
			if (dwTime < m_dwOutDelay) {
				if (dwTime > 30000) {
					// 15ms以上空いていれば、1.5ms待つ
					::Sleep(10);
					continue;
				}
				if (dwTime > 3000) {
					// 1.5ms以上空いていれば、1.5ms待つ
					::Sleep(1);
					continue;
				}
				// 1.5ms未満の待ちはSleep(0)で吸収
				::Sleep(0);
				continue;
			}
		}

		// データ解釈(初回)
		if (outState == OutReady) {
			// エクスクルーシブ送信中なら、ここで出力(闇の血族)
			if ((pData->data & 0x80) && (pData->data != MIDI_EXEND)) {
				if (outState == OutEx) {
					m_ExBuf[dwCnt] = MIDI_EXEND;
					dwCnt++;
					SendEx(m_ExBuf);
					continue;
				}
			}

			// 初回データ
			if ((pData->data >= MIDI_NOTEOFF) && (pData->data < MIDI_EXSTART)) {
				// 80-EF
				outState = OutShort;
				dwCmd = pData->data;
				dwCnt = 1;
				dwShortMsg = dwCmd;
				dwLen = 3;

				if ((pData->data >= MIDI_PROGCHG) && (pData->data < MIDI_PITCHCHG)) {
					// C0-DF
					dwLen = 2;
				}
			}

			// コモンメッセージ、リアルタイムメッセージ
			switch (pData->data) {
				// エクスクルーシブ開始
				case MIDI_EXSTART:
					outState = OutEx;
					m_ExBuf[0] = (BYTE)pData->data;
					dwCnt = 1;
					break;
				// タイムコードクォーターフレーム
				case MIDI_QFRAME:
				// ソングセレクト
				case MIDI_SONGSEL:
					outState = OutShort;
					dwCnt = 1;
					dwShortMsg = pData->data;
					dwLen = 2;
					break;
				// ソングポジションポインタ
				case MIDI_SONGPTR:
					outState = OutShort;
					dwCnt = 1;
					dwShortMsg = pData->data;
					dwLen = 3;
					break;
				// チューンリクエスト、リアルタイムメッセージ
				case MIDI_TUNEREQ:
				case MIDI_TIMINGCLK:
				case MIDI_START:
				case MIDI_CONTINUE:
				case MIDI_STOP:
				case MIDI_ACTSENSE:
				case MIDI_SYSRESET:
					// リアルタイムメッセージの類は出力しない(大魔界村)
					m_pMIDI->DelTransData(1);
					pData = NULL;
					continue;
			}

			// ランニングステータス
			if (pData->data < MIDI_NOTEOFF) {
				dwCnt = 2;
				dwShortMsg = (pData->data << 8) | dwCmd;
				outState = OutShort;
			}

#if defined(_DEBUG)
			// データチェック(DEBUGのみ)
			if (outState == OutReady) {
				TRACE("MIDI異常データ検出 $%02X\n", pData->data);
			}
#endif	// _DEBUG

			// このデータを削除
			m_pMIDI->DelTransData(1);
			pData = NULL;

			// ここで送信できるものに限り、出力
			if ((outState == OutShort) && (dwCnt == dwLen)) {
				m_OutSection.Lock();
				::midiOutShortMsg(m_hOut, dwShortMsg);
				m_OutSection.Unlock();

				// カウントアップ
				m_dwShortSend++;
				outState = OutReady;
			}
			continue;
		}

		// データ解釈(2バイト目以降)
		if (pData->data & 0x80) {
			// エクスクルーシブ送信中なら、ここで出力(闇の血族)
			if (pData->data != MIDI_EXEND) {
				if (outState == OutEx) {
					m_ExBuf[dwCnt] = MIDI_EXEND;
					dwCnt++;
					SendEx(m_ExBuf);
					continue;
				}
			}

			if ((pData->data >= MIDI_NOTEOFF) && (pData->data < MIDI_EXSTART)) {
				// 80-EF
				outState = OutShort;
				dwCmd = pData->data;
				dwCnt = 1;
				dwShortMsg = dwCmd;
				dwLen = 3;

				if ((pData->data >= MIDI_PROGCHG) && (pData->data < MIDI_PITCHCHG)) {
					// C0-DF
					dwLen = 2;
				}
			}

			// コモンメッセージ、リアルタイムメッセージ
			switch (pData->data) {
				// エクスクルーシブ開始
				case MIDI_EXSTART:
					outState = OutEx;
					m_ExBuf[0] = (BYTE)pData->data;
					dwCnt = 1;
					break;
				// タイムコードクォーターフレーム
				case MIDI_QFRAME:
				// ソングセレクト
				case MIDI_SONGSEL:
					outState = OutShort;
					dwCnt = 1;
					dwShortMsg = pData->data;
					dwLen = 2;
					break;
				// ソングポジションポインタ
				case MIDI_SONGPTR:
					outState = OutShort;
					dwCnt = 1;
					dwShortMsg = pData->data;
					dwLen = 3;
					break;
				// チューンリクエスト、リアルタイムメッセージ
				case MIDI_TUNEREQ:
				case MIDI_TIMINGCLK:
				case MIDI_START:
				case MIDI_CONTINUE:
				case MIDI_STOP:
				case MIDI_ACTSENSE:
				case MIDI_SYSRESET:
					// リアルタイムメッセージの類は出力しない(大魔界村)
					break;
				// エクスクルーシブ終了
				case MIDI_EXEND:
					if (outState == OutEx) {
						// エクスクルーシブ送信中なら、ここで出力
						m_ExBuf[dwCnt] = (BYTE)pData->data;
						dwCnt++;
						SendEx(m_ExBuf);
						outState = OutReady;

						// このデータを削除
						m_pMIDI->DelTransData(1);
						pData = NULL;
						continue;
					}
			}
		}
		else {
			// 00-7F
			if (outState == OutEx) {
				// エクスクルーシブ
				m_ExBuf[dwCnt] = (BYTE)pData->data;
				dwCnt++;
				if (dwCnt >= sizeof(m_ExBuf)) {
					outState = OutReady;
				}
			}
			if (outState == OutShort) {
				// ショートメッセージ
				dwShortMsg |= (pData->data << (dwCnt << 3));
				dwCnt++;
			}
		}

		// このデータを削除
		m_pMIDI->DelTransData(1);
		pData = NULL;

		// ここで送信できるものに限り、出力
		if ((outState == OutShort) && (dwCnt == dwLen)) {
			m_OutSection.Lock();
			::midiOutShortMsg(m_hOut, dwShortMsg);
			m_OutSection.Unlock();

			// カウントアップ
			m_dwShortSend++;
			outState = OutReady;
		}
	}

	// エクスクルーシブ送信後のウェイト
	SendExWait();

	// 全ノートオフ
	SendAllNoteOff();
}

//---------------------------------------------------------------------------
//
//	Outコールバック
//
//---------------------------------------------------------------------------
void FASTCALL CMIDI::CallbackOut(UINT wMsg, DWORD /*dwParam1*/, DWORD /*dwParam2*/)
{
	ASSERT(this);
	ASSERT_VALID(this);

	// エクスクルーシブ送信完了のみハンドリング
	if (wMsg == MM_MOM_DONE) {

		// エクスクルーシブ送信フラグOFF(BOOLデータのため、同期は取らない)
		m_bSendEx = FALSE;
	}
}

//---------------------------------------------------------------------------
//
//	OutデバイスCaps取得
//
//---------------------------------------------------------------------------
void FASTCALL CMIDI::GetOutCaps()
{
	MMRESULT mmResult;
#if _MFC_VER >= 0x700
	UINT_PTR uDeviceID;
#else
	UINT uDeviceID;
#endif	// MFC_VER

	// デバイスID初期化
	uDeviceID = 0;

	// ループ
	while ((DWORD)uDeviceID < m_dwOutDevs) {
		// midiOutGetDevCapsで取得
		mmResult = ::midiOutGetDevCaps( uDeviceID,
										&m_pOutCaps[uDeviceID],
										sizeof(MIDIOUTCAPS));

		// 結果を記録
		if (mmResult == MMSYSERR_NOERROR) {
			m_pOutCaps[uDeviceID].dwSupport = 0;
		}
		else {
			m_pOutCaps[uDeviceID].dwSupport = 1;
		}

		// デバイスIDを次へ
		uDeviceID++;
	}
}

//---------------------------------------------------------------------------
//
//	Outスレッド
//
//---------------------------------------------------------------------------
UINT CMIDI::OutThread(LPVOID pParam)
{
	CMIDI *pMIDI;

	// パラメータ受け取り
	pMIDI = (CMIDI*)pParam;

	// 実行
	pMIDI->RunOut();

	// 終了
	return 0;
}

//---------------------------------------------------------------------------
//
//	Outコールバック
//
//---------------------------------------------------------------------------
#if _MFC_VER >= 0x700
void CALLBACK CMIDI::OutProc(HMIDIOUT hOut, UINT wMsg, DWORD_PTR dwInstance,
							DWORD dwParam1, DWORD dwParam2)
#else
void CALLBACK CMIDI::OutProc(HMIDIOUT hOut, UINT wMsg, DWORD dwInstance,
							DWORD dwParam1, DWORD dwParam2)
#endif
{
	CMIDI *pMIDI;

	// パラメータ受け取り
	pMIDI = (CMIDI*)dwInstance;

	// 呼び出し
	if (hOut == pMIDI->GetOutHandle()) {
		pMIDI->CallbackOut(wMsg, dwParam1, dwParam2);
	}
}

//---------------------------------------------------------------------------
//
//	エクスクルーシブ送信
//	※前回の送信が完了し、ヘッダの後片付けが終わってから呼び出すこと
//
//---------------------------------------------------------------------------
BOOL FASTCALL CMIDI::SendEx(const BYTE *pExData)
{
	MMRESULT mmResult;
	DWORD dwLength;

	ASSERT(this);
	ASSERT(pExData);
	ASSERT(!m_bSendEx);
	ASSERT(!m_bSendExHdr);
	ASSERT_VALID(this);

	// 個数を数える
	dwLength = 1;
	while (pExData[dwLength - 1] != 0xf7) {
		ASSERT(dwLength <= sizeof(m_ExBuf));
		dwLength++;
	}

	// 改めてデータを検査
	ASSERT(pExData[0] == 0xf0);
	ASSERT(pExData[dwLength - 1] == 0xf7);

	// ヘッダ準備
	memset(&m_ExHdr, 0, sizeof(m_ExHdr));
	m_ExHdr.lpData = (LPSTR)pExData;
	m_ExHdr.dwBufferLength = dwLength;
	m_ExHdr.dwBytesRecorded = dwLength;
	m_OutSection.Lock();
	mmResult = ::midiOutPrepareHeader(m_hOut, &m_ExHdr, sizeof(MIDIHDR));
	m_OutSection.Unlock();
	if (mmResult != MMSYSERR_NOERROR) {
		return FALSE;
	}

	// エクスクルーシブヘッダ使用フラグON
	m_bSendExHdr = TRUE;

	// エクスクルーシブ送信フラグを立てる
	m_bSendEx = TRUE;

	// LongMsg送信
	m_OutSection.Lock();
	mmResult = ::midiOutLongMsg(m_hOut, &m_ExHdr, sizeof(MIDIHDR));
	m_OutSection.Unlock();
	if (mmResult != MMSYSERR_NOERROR) {
		// 失敗したらフラグ落とす
		m_bSendEx = FALSE;
		return FALSE;
	}

	// 成功
	m_dwExSend++;
	return TRUE;
}

//---------------------------------------------------------------------------
//
//	エクスクルーシブ送信完了待ち
//
//---------------------------------------------------------------------------
BOOL FASTCALL CMIDI::SendExWait()
{
	DWORD dwTime;
	DWORD dwDiff;
	BOOL bResult;

	ASSERT(this);
	ASSERT_VALID(this);

	// フラグは成功
	bResult = TRUE;

	// エクスクルーシブ送信中なら
	if (m_bSendEx) {
		// 現在時間を取得
		dwTime = ::timeGetTime();

		// 送信完了までウェイト(タイムアウト2000ms)
		while (m_bSendEx) {
			// 経過時間を算出
			dwDiff = ::timeGetTime();
			dwDiff -= dwTime;

			// タイムアウトなら失敗として抜ける
			if (dwDiff >= 2000) {
				bResult = FALSE;
				break;
			}

			// スリープ
			::Sleep(1);
		}
	}

	// エクスクルーシブ送信ヘッダを使った後なら、後片付け
	if (m_bSendExHdr) {
		m_OutSection.Lock();
		::midiOutUnprepareHeader(m_hOut, &m_ExHdr, sizeof(MIDIHDR));
		m_OutSection.Unlock();
		m_bSendExHdr = FALSE;
	}

	return bResult;
}

//---------------------------------------------------------------------------
//
//	全ノートオフ
//
//---------------------------------------------------------------------------
void FASTCALL CMIDI::SendAllNoteOff()
{
	int nCh;
	DWORD dwMsg;

	ASSERT(this);
	ASSERT_VALID(this);

	// モードメッセージAll Sound Offを送信
	m_OutSection.Lock();
	for (nCh=0; nCh<16; nCh++) {
		dwMsg = (DWORD)(0x78b0 + nCh);
		::midiOutShortMsg(m_hOut, dwMsg);
	}
	m_OutSection.Unlock();

	// ノートオフ(MT-32対策)
	m_OutSection.Lock();
	for (nCh=0; nCh<16; nCh++) {
		::midiOutShortMsg(m_hOut, (DWORD)(0x7bb0 + nCh));
		::midiOutShortMsg(m_hOut, (DWORD)(0x7db0 + nCh));
		::midiOutShortMsg(m_hOut, (DWORD)(0x7fb0 + nCh));
	}
	m_OutSection.Unlock();
}

//---------------------------------------------------------------------------
//
//	音源リセット
//
//---------------------------------------------------------------------------
BOOL FASTCALL CMIDI::SendReset()
{
	int nCh;

	ASSERT(this);
	ASSERT_VALID(this);

	// Reset All Controllerコマンド
	m_OutSection.Lock();
	for (nCh=0; nCh<16; nCh++) {
		::midiOutShortMsg(m_hOut, (DWORD)(0x79b0 + nCh));
	}
	m_OutSection.Unlock();

	// リセットタイプ別
	switch (m_nOutReset) {
		// GM
		case 0:
			SendEx(ResetGM);
			break;

		// GS
		case 1:
			// GM->GSの順で送出
			SendEx(ResetGM);
			SendExWait();
			SendEx(ResetGS);
			break;

		// XG
		case 2:
			// GM->XGの順で送出
			SendEx(ResetGM);
			SendExWait();
			SendEx(ResetXG);
			break;

		// LA
		case 3:
			SendEx(ResetLA);
			break;

		// その他(あり得ない)
		default:
			ASSERT(FALSE);
			break;
	};

	// エクスクルーシブ送信完了を待つ
	SendExWait();

	// カウントクリア
	m_dwShortSend = 0;
	m_dwExSend = 0;
	m_dwUnSend = 0;

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	Outデバイス数を取得
//
//---------------------------------------------------------------------------
DWORD FASTCALL CMIDI::GetOutDevs() const
{
	ASSERT(this);
	ASSERT_VALID(this);

	return m_dwOutDevs;
}

//---------------------------------------------------------------------------
//
//	Outデバイス名称を取得
//
//---------------------------------------------------------------------------
BOOL FASTCALL CMIDI::GetOutDevDesc(int nDevice, CString& strDesc) const
{
	LPCTSTR lpstrDesc;

	ASSERT(this);
	ASSERT(nDevice >= 0);
	ASSERT_VALID(this);

	// インデックスチェック
	if (nDevice >= (int)m_dwOutDevs) {
		strDesc.Empty();
		return FALSE;
	}

	// LPCTSTRへ変換
	lpstrDesc = A2T(m_pOutCaps[nDevice].szPname);
	strDesc = lpstrDesc;

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	Outディレイ設定
//
//---------------------------------------------------------------------------
void FASTCALL CMIDI::SetOutDelay(int nDelay)
{
	ASSERT(this);
	ASSERT(nDelay >= 0);
	ASSERT_VALID(this);

	m_dwOutDelay = (DWORD)(nDelay * 2000);
}

//---------------------------------------------------------------------------
//
//	Out音量取得
//
//---------------------------------------------------------------------------
int FASTCALL CMIDI::GetOutVolume()
{
	MMRESULT mmResult;
	DWORD dwVolume;
	int nVolume;

	ASSERT(this);
	ASSERT_VALID(this);

	// ハンドルが有効で、スレッドが生きていること
	if ((m_dwOutDevice > 0) && m_pOutThread) {
		// クリティカルセクションを挟んで、音量を取得
		m_OutSection.Lock();
		mmResult = ::midiOutGetVolume(m_hOut, &dwVolume);
		m_OutSection.Unlock();

		// 成功した場合、LOWORD(左)を優先する
		if (mmResult == MMSYSERR_NOERROR) {
			// 得られる値は16bit
			nVolume = LOWORD(dwVolume);

			// 成功
			return nVolume;
		}
	}

	// MIDIミキサは無効
	return -1;
}

//---------------------------------------------------------------------------
//
//	Out音量設定
//
//---------------------------------------------------------------------------
void FASTCALL CMIDI::SetOutVolume(int nVolume)
{
	DWORD dwVolume;

	ASSERT(this);
	ASSERT((nVolume >= 0) && (nVolume < 0x10000));
	ASSERT_VALID(this);

	// ハンドルが有効で、スレッドが生きていること
	if ((m_dwOutDevice > 0) && m_pOutThread) {
		// 値を作成
		dwVolume = (DWORD)nVolume;
		dwVolume <<= 16;
		dwVolume |= (DWORD)nVolume;

		// クリティカルセクションを挟んで、音量を設定
		m_OutSection.Lock();
		::midiOutSetVolume(m_hOut, dwVolume);
		m_OutSection.Unlock();
	}
}

//---------------------------------------------------------------------------
//
//	Out情報取得
//
//---------------------------------------------------------------------------
void FASTCALL CMIDI::GetOutInfo(LPMIDIINFO pInfo) const
{
	ASSERT(this);
	ASSERT(pInfo);
	ASSERT_VALID(this);

	// デバイス
	pInfo->dwDevices = m_dwOutDevs;
	pInfo->dwDevice = m_dwOutDevice;

	// カウンタ
	pInfo->dwShort = m_dwShortSend;
	pInfo->dwLong = m_dwExSend;
	pInfo->dwUnprepare = m_dwUnSend;

	// バッファ情報はVMデバイス側が持つので、ここでは設定しない
}

//---------------------------------------------------------------------------
//
//	GM音源リセットコマンド
//
//---------------------------------------------------------------------------
const BYTE CMIDI::ResetGM[] = {
	0xf0, 0x7e, 0x7f, 0x09, 0x01, 0xf7
};

//---------------------------------------------------------------------------
//
//	GS音源リセットコマンド
//
//---------------------------------------------------------------------------
const BYTE CMIDI::ResetGS[] = {
	0xf0, 0x41, 0x10, 0x42, 0x12, 0x40, 0x00, 0x7f,
	0x00, 0x41, 0xf7
};

//---------------------------------------------------------------------------
//
//	XG音源リセットコマンド
//
//---------------------------------------------------------------------------
const BYTE CMIDI::ResetXG[] = {
	0xf0, 0x43, 0x10, 0x4c, 0x00, 0x00, 0x7e, 0x00,
	0xf7
};

//---------------------------------------------------------------------------
//
//	LA音源リセットコマンド
//
//---------------------------------------------------------------------------
const BYTE CMIDI::ResetLA[] = {
	0xf0, 0x41, 0x10, 0x16, 0x12, 0x7f, 0x00, 0x00,
	0x00, 0x01, 0xf7
};

//===========================================================================
//
//	IN部分
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	Inオープン
//	※dwDevice=0は割り当てなし
//
//---------------------------------------------------------------------------
void FASTCALL CMIDI::OpenIn(DWORD dwDevice)
{
	MMRESULT mmResult;
	UINT uDeviceID;

	ASSERT(this);
	ASSERT(m_dwInDevice == 0);
	ASSERT(!m_pInThread);
	ASSERT_VALID(this);

	// 割り当てしないのであればリターン
	if (dwDevice == 0) {
		return;
	}

	// デバイスがなければリターン
	if (m_dwInDevs == 0) {
		return;
	}

	// デバイス数を超えていればデバイス0
	if (dwDevice > m_dwInDevs) {
		dwDevice = 1;
	}

	// デバイス決定
	ASSERT(dwDevice >= 1);
	uDeviceID = (UINT)(dwDevice - 1);

	// オープン

#if _MFC_VER >= 0x700
	mmResult = ::midiInOpen(&m_hIn, uDeviceID,
							(DWORD_PTR)InProc, (DWORD_PTR)this, CALLBACK_FUNCTION);
#else
	mmResult = ::midiInOpen(&m_hIn, uDeviceID,
							(DWORD)InProc, (DWORD)this, CALLBACK_FUNCTION);
#endif

	// 失敗なら終了
	if (mmResult != MMSYSERR_NOERROR) {
		return;
	}

	// デバイス番号を更新
	m_dwInDevice = dwDevice;

	// スレッドを立てる
	m_bInThread = FALSE;
	m_pInThread = AfxBeginThread(InThread, this);
}

//---------------------------------------------------------------------------
//
//	Inクローズ
//
//---------------------------------------------------------------------------
void FASTCALL CMIDI::CloseIn()
{
	ASSERT(this);
	ASSERT_VALID(this);

	// スレッドを止める
	if (m_pInThread) {
		// 終了フラグを上げ
		m_bInThread = TRUE;

		// 終了待ち(無期限)
		::WaitForSingleObject(m_pInThread->m_hThread, INFINITE);

		// スレッドなし
		m_pInThread = NULL;
	}

	// デバイスを閉じる
	if (m_dwInDevice != 0) {
		// リセットして、クローズ
		::midiInReset(m_hIn);
		::midiInClose(m_hIn);

		// オープンデバイスなし
		m_dwInDevice = 0;
	}
}

//---------------------------------------------------------------------------
//
//	Inスレッド
//
//---------------------------------------------------------------------------
void FASTCALL CMIDI::RunIn()
{
	int i;
	BOOL bActive;
	BOOL bValid;
	BOOL bMsg;

	ASSERT(this);
	ASSERT(m_hIn);
	ASSERT_VALID(this);

	// 非アクティブ
	bActive = FALSE;

	// エクスクルーシブバッファ未使用
	m_InState[0] = InNotUsed;
	m_InState[1] = InNotUsed;

	// カウンタリセット
	m_dwShortRecv = 0;
	m_dwExRecv = 0;
	m_dwUnRecv = 0;

	// リセット
	::midiInReset(m_hIn);

	// ループ
	while (!m_bInThread) {
		// 有効フラグ設定
		bValid = FALSE;
		if (m_pSch->IsEnable()) {
			// スケジューラ動作中で
			if (m_pMIDI->IsActive()) {
				// MIDIアクティブなら
				bValid = TRUE;
			}
		}

		// 無効な場合
		if (!bValid) {
			// Sleep
			::Sleep(10);
			continue;
		}

		// 有効なので、まだスタートしていなければスタート
		if (!bActive) {
			StartIn();
			bActive = TRUE;
		}

		// メッセージ処理なし
		bMsg = FALSE;

		// ロングメッセージの検査
		for (i=0; i<2; i++) {
			if (m_InState[i] == InDone) {
				// ロングメッセージ受信済み
				LongIn(i);
				bMsg = TRUE;
			}
		}

		// ショートメッセージの検査
		m_InSection.Lock();
		if (!m_InQueue.IsEmpty()) {
			// ショートメッセージ受信済み
			ShortIn();
			bMsg = TRUE;
		}
		m_InSection.Unlock();

		// メッセージ処理なければSleep
		if (!bMsg) {
			::Sleep(1);
		}
	}

	// 停止
	if (bActive) {
		bActive = FALSE;
		StopIn();
	}
}

//---------------------------------------------------------------------------
//
//	In開始
//
//---------------------------------------------------------------------------
BOOL FASTCALL CMIDI::StartIn()
{
	int i;
	MMRESULT mmResult;

	ASSERT(this);
	ASSERT(m_hIn);
	ASSERT_VALID(this);

	// エクスクルーシブバッファを登録
	for (i=0; i<2; i++) {
		if (m_InState[i] == InNotUsed) {
			// MIDIヘッダ作成
			memset(&m_InHdr[i], 0, sizeof(MIDIHDR));
			m_InHdr[i].lpData = (LPSTR)&m_InBuf[i][0];
			m_InHdr[i].dwBufferLength = InBufMax;
			m_InHdr[i].dwUser = (DWORD)i;

			// ヘッダ準備
			mmResult = ::midiInPrepareHeader(m_hIn, &m_InHdr[i], sizeof(MIDIHDR));
			if (mmResult != MMSYSERR_NOERROR) {
				// ヘッダ準備失敗
				continue;
			}

			// ヘッダ追加
			mmResult = ::midiInAddBuffer(m_hIn, &m_InHdr[i], sizeof(MIDIHDR));
			if (mmResult != MMSYSERR_NOERROR) {
				// ヘッダ追加失敗
				::midiInUnprepareHeader(m_hIn, &m_InHdr[i], sizeof(MIDIHDR));
				continue;
			}

			// ステート設定
			m_InState[i] = InReady;
		}
	}

	// Inキューをクリア
	m_InQueue.Clear();

	// MIDI入力開始
	mmResult = ::midiInStart(m_hIn);
	if (mmResult != MMSYSERR_NOERROR) {
		return FALSE;
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	In停止(バッファのみ)
//
//---------------------------------------------------------------------------
void FASTCALL CMIDI::StopIn()
{
	int i;

	ASSERT(this);
	ASSERT(m_hIn);
	ASSERT_VALID(this);

	// エクスクルーシブバッファを削除
	for (i=0; i<2; i++) {
		// ヘッダ削除
		if (m_InState[i] != InNotUsed) {
			::midiInUnprepareHeader(m_hIn, &m_InHdr[i], sizeof(MIDIHDR));
			m_InState[i] = InNotUsed;
		}
	}
}

//---------------------------------------------------------------------------
//
//	Inコールバック
//
//---------------------------------------------------------------------------
void FASTCALL CMIDI::CallbackIn(UINT wMsg, DWORD dwParam1, DWORD /*dwParam2*/)
{
	MIDIHDR *lpMidiHdr;
	DWORD dwLength;
	BYTE msg[3];

	ASSERT(this);
	ASSERT_VALID(this);

	switch (wMsg) {
		// ショートメッセージ
		case MIM_DATA:
		case MIM_MOREDATA:
			// メッセージを作成
			msg[0] = (BYTE)dwParam1;
			msg[1] = (BYTE)(dwParam1 >> 8);
			msg[2] = (BYTE)(dwParam1 >> 16);

			// ステータスバイトに応じて長さを決める
			if (msg[0] < MIDI_NOTEOFF) {
				// ランニングステータスは無いので、異常データ
				break;
			}
			dwLength = 3;
			if ((msg[0] >= MIDI_PROGCHG) && (msg[0] < MIDI_PITCHCHG)){
				dwLength = 2;
			}
			if (msg[0] >= MIDI_EXSTART) {
				// F0以上のメッセージ
				switch (msg[0]) {
					// ソングポジションポインタ
					case MIDI_SONGPTR:
						dwLength = 3;
						break;

					// タイムコードクォーターフレーム
					case MIDI_QFRAME:
					// ソングセレクト
					case MIDI_SONGSEL:
						dwLength = 2;
						break;

					// チューンセレクト
					case MIDI_TUNEREQ:
					// タイミングクロック
					case MIDI_TIMINGCLK:
					// スタート
					case MIDI_START:
					// コンティニュー
					case MIDI_CONTINUE:
					// ストップ
					case MIDI_STOP:
					// アクティブセンシング
					case MIDI_ACTSENSE:
					// システムリセット
					case MIDI_SYSRESET:
						dwLength = 1;

					// その他は受け取らない
					default:
						dwLength = 0;
				}
			}

			// データ追加
			if (dwLength > 0) {
				// ロック
				m_InSection.Lock();

				// 挿入
				m_InQueue.Insert(msg, dwLength);

				// アンロック
				m_InSection.Unlock();

				// カウントアップ
				m_dwShortRecv++;
			}
			break;

		// ロングメッセージ
		case MIM_LONGDATA:
		case MIM_LONGERROR:
			// フラグ変更
			lpMidiHdr = (MIDIHDR*)dwParam1;
			if (lpMidiHdr->dwUser < 2) {
				m_InState[lpMidiHdr->dwUser] = InDone;

				// カウントアップ
				m_dwExRecv++;
			}
			break;

		// その他は処理しない
		default:
			break;
	}
}

//---------------------------------------------------------------------------
//
//	ショートメッセージ受信
//	※クリティカルセクションはロック状態
//
//---------------------------------------------------------------------------
void FASTCALL CMIDI::ShortIn()
{
	BYTE buf[InBufMax];
	DWORD dwReceived;

	ASSERT(this);
	ASSERT(m_pMIDI);
	ASSERT_VALID(this);

	// スタック上にすべて取得
	dwReceived = m_InQueue.Get(buf);
	ASSERT(dwReceived > 0);

	// MIDIデバイスへ送信
	m_pMIDI->SetRecvData(buf, (int)dwReceived);
}

//---------------------------------------------------------------------------
//
//	ロングメッセージ受信
//
//---------------------------------------------------------------------------
void FASTCALL CMIDI::LongIn(int nHdr)
{
	int i;
	MMRESULT mmResult;

	ASSERT(this);
	ASSERT((nHdr == 0) || (nHdr == 1));
	ASSERT(m_hIn);
	ASSERT_VALID(this);

	// 正常に受信できたことを確認
	ASSERT(m_InHdr[nHdr].dwFlags & MHDR_DONE);

	// MIDIデバイスへ送信
	m_pMIDI->SetRecvData((const BYTE*)m_InHdr[nHdr].lpData,
						 (int)m_InHdr[nHdr].dwBytesRecorded);

	// ヘッダを解放
	::midiInUnprepareHeader(m_hIn, &m_InHdr[nHdr], sizeof(MIDIHDR));
	m_InState[nHdr] = InNotUsed;
	m_dwUnRecv++;

	// 再スタート
	for (i=0; i<2; i++) {
		if (m_InState[i] == InNotUsed) {
			// MIDIヘッダ作成
			memset(&m_InHdr[i], 0, sizeof(MIDIHDR));
			m_InHdr[i].lpData = (LPSTR)&m_InBuf[i][0];
			m_InHdr[i].dwBufferLength = InBufMax;
			m_InHdr[i].dwUser = (DWORD)i;

			// ヘッダ準備
			mmResult = ::midiInPrepareHeader(m_hIn, &m_InHdr[i], sizeof(MIDIHDR));
			if (mmResult != MMSYSERR_NOERROR) {
				// ヘッダ準備失敗
				continue;
			}

			// ヘッダ追加
			mmResult = ::midiInAddBuffer(m_hIn, &m_InHdr[i], sizeof(MIDIHDR));
			if (mmResult != MMSYSERR_NOERROR) {
				// ヘッダ追加失敗
				::midiInUnprepareHeader(m_hIn, &m_InHdr[i], sizeof(MIDIHDR));
				continue;
			}

			// ステート設定
			m_InState[i] = InReady;
		}
	}
}

//---------------------------------------------------------------------------
//
//	InデバイスCaps取得
//
//---------------------------------------------------------------------------
void FASTCALL CMIDI::GetInCaps()
{
	MMRESULT mmResult;
#if _MFC_VER >= 0x700
	UINT_PTR uDeviceID;
#else
	UINT uDeviceID;
#endif	// MFC_VER

	// デバイスID初期化
	uDeviceID = 0;

	// ループ
	while ((DWORD)uDeviceID < m_dwInDevs) {
		// midiInGetDevCapsで取得
		mmResult = ::midiInGetDevCaps(uDeviceID,
									  &m_pInCaps[uDeviceID],
									  sizeof(MIDIINCAPS));

		// 結果を記録
		if (mmResult == MMSYSERR_NOERROR) {
			m_pInCaps[uDeviceID].dwSupport = 0;
		}
		else {
			m_pInCaps[uDeviceID].dwSupport = 1;
		}

		// デバイスIDを次へ
		uDeviceID++;
	}
}

//---------------------------------------------------------------------------
//
//	Inスレッド
//
//---------------------------------------------------------------------------
UINT CMIDI::InThread(LPVOID pParam)
{
	CMIDI *pMIDI;

	// パラメータ受け取り
	pMIDI = (CMIDI*)pParam;

	// 実行
	pMIDI->RunIn();

	// 終了
	return 0;
}

//---------------------------------------------------------------------------
//
//	Inコールバック
//
//---------------------------------------------------------------------------
#if _MFC_VER >= 0x700
void CALLBACK CMIDI::InProc(HMIDIIN hIn, UINT wMsg, DWORD_PTR dwInstance,
							DWORD dwParam1, DWORD dwParam2)
#else
void CALLBACK CMIDI::InProc(HMIDIIN hIn, UINT wMsg, DWORD dwInstance,
							DWORD dwParam1, DWORD dwParam2)
#endif
{
	CMIDI *pMIDI;

	// パラメータ受け取り
	pMIDI = (CMIDI*)dwInstance;

	// 呼び出し
	if (hIn == pMIDI->GetInHandle()) {
		pMIDI->CallbackIn(wMsg, dwParam1, dwParam2);
	}
}

//---------------------------------------------------------------------------
//
//	Inデバイス数を取得
//
//---------------------------------------------------------------------------
DWORD FASTCALL CMIDI::GetInDevs() const
{
	ASSERT(this);
	ASSERT_VALID(this);

	return m_dwInDevs;
}

//---------------------------------------------------------------------------
//
//	Inデバイス名称を取得
//
//---------------------------------------------------------------------------
BOOL FASTCALL CMIDI::GetInDevDesc(int nDevice, CString& strDesc) const
{
	LPCTSTR lpstrDesc;

	ASSERT(this);
	ASSERT(nDevice >= 0);
	ASSERT_VALID(this);

	// インデックスチェック
	if (nDevice >= (int)m_dwInDevs) {
		strDesc.Empty();
		return FALSE;
	}

	// LPCTSTRへ変換
	lpstrDesc = A2T(m_pInCaps[nDevice].szPname);
	strDesc = lpstrDesc;

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	Inディレイ設定
//
//---------------------------------------------------------------------------
void FASTCALL CMIDI::SetInDelay(int nDelay)
{
	ASSERT(this);
	ASSERT(nDelay >= 0);
	ASSERT_VALID(this);

	// デバイスへ通知
	m_pMIDI->SetRecvDelay(nDelay);
}

//---------------------------------------------------------------------------
//
//	In情報取得
//
//---------------------------------------------------------------------------
void FASTCALL CMIDI::GetInInfo(LPMIDIINFO pInfo) const
{
	ASSERT(this);
	ASSERT(pInfo);
	ASSERT_VALID(this);

	// デバイス
	pInfo->dwDevices = m_dwInDevs;
	pInfo->dwDevice = m_dwInDevice;

	// カウンタ
	pInfo->dwShort = m_dwShortRecv;
	pInfo->dwLong = m_dwExRecv;
	pInfo->dwUnprepare = m_dwUnRecv;

	// バッファ情報はVMデバイス側が持つので、ここでは設定しない
}

#endif	// _WIN32
