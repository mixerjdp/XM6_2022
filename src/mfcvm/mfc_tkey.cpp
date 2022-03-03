//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2005 ＰＩ．(ytanaka@ipc-tokai.or.jp)
//	[ MFC TrueKey ]
//
//---------------------------------------------------------------------------

#if defined(_WIN32)

#include "os.h"
#include "xm6.h"
#include "vm.h"
#include "mfp.h"
#include "keyboard.h"
#include "mfc_com.h"
#include "mfc_cfg.h"
#include "mfc_sch.h"
#include "mfc_que.h"
#include "mfc_port.h"
#include "mfc_tkey.h"

//===========================================================================
//
//	TrueKey
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	スレッド関数
//
//---------------------------------------------------------------------------
static UINT ThreadFunc(LPVOID pParam)
{
	CTKey *pTKey;

	// パラメータを受け取る
	pTKey = (CTKey*)pParam;
	ASSERT(pTKey);

	// 実行
	pTKey->Run();

	// 終了コードを持ってスレッドを終了
	return 0;
}

//---------------------------------------------------------------------------
//
//	コンストラクタ
//
//---------------------------------------------------------------------------
CTKey::CTKey(CFrmWnd *pWnd) : CComponent(pWnd)
{
	int i;

	// コンポーネントパラメータ
	m_dwID = MAKEID('T', 'K', 'E', 'Y');
	m_strDesc = _T("TrueKey Module");

	// パラメータ
	m_nMode = 0;
	m_nCOM = 0;
	m_bRTS = FALSE;
	m_bLine = FALSE;

	// ハンドル・スレッド
	m_hCOM = INVALID_HANDLE_VALUE;
	m_pCOM = NULL;
	m_bReq = FALSE;

	// 送信制御
	m_bTxValid = FALSE;
	memset(&m_TxOver, 0, sizeof(m_TxOver));

	// 受信制御
	memset(&m_RxOver, 0, sizeof(m_RxOver));

	// キーフラグ、テーブル
	for (i=0; i<KeyMax; i++) {
		m_bKey[i] = FALSE;
		m_bWin[i] = FALSE;
		m_nKey[i] = KeyTable[i];
	}

	// オブジェクト
	m_pMFP = NULL;
	m_pKeyboard = NULL;
	m_pSch = NULL;
}

//---------------------------------------------------------------------------
//
//	初期化
//
//---------------------------------------------------------------------------
BOOL FASTCALL CTKey::Init()
{
	ASSERT(this);

	// 基本クラス
	if (!CComponent::Init()) {
		return FALSE;
	}

	// MFP取得
	ASSERT(!m_pMFP);
	m_pMFP = (MFP*)::GetVM()->SearchDevice(MAKEID('M', 'F', 'P', ' '));
	ASSERT(m_pMFP);

	// キーボード取得
	ASSERT(!m_pKeyboard);
	m_pKeyboard = (Keyboard*)::GetVM()->SearchDevice(MAKEID('K', 'E', 'Y', 'B'));
	ASSERT(m_pKeyboard);

	// スケジューラ取得
	ASSERT(!m_pSch);
	m_pSch = (CScheduler*)SearchComponent(MAKEID('S', 'C', 'H', 'E'));
	ASSERT(m_pSch);

	// キュー初期化
	if (!m_TxQueue.Init(0x1000)) {
		return FALSE;
	}
	if (!m_RxQueue.Init(0x1000)) {
		return FALSE;
	}

	// オープン
	Open();

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	クリーンアップ
//
//---------------------------------------------------------------------------
void FASTCALL CTKey::Cleanup()
{
	ASSERT(this);

	// クローズ
	Close();

	// 基本クラス
	CComponent::Cleanup();
}

//---------------------------------------------------------------------------
//
//	設定適用
//
//---------------------------------------------------------------------------
void FASTCALL CTKey::ApplyCfg(const Config* pConfig)
{
	BOOL bAction;
	CPort *pPort;

	ASSERT(this);
	ASSERT(pConfig);

	// 再設定フラグ初期化
	bAction = FALSE;

	// モードが違っているか
	if (pConfig->tkey_mode != m_nMode) {
		m_nMode = pConfig->tkey_mode;
		bAction = TRUE;
	}

	// ポート番号が違っているか
	if (pConfig->tkey_com != m_nCOM) {
		m_nCOM = pConfig->tkey_com;
		bAction = TRUE;
	}

	// RTS論理が違っているか
	if (pConfig->tkey_rts != m_bRTS) {
		m_bRTS = pConfig->tkey_rts;
		bAction = TRUE;
	}

	// 変更点があれば
	if (bAction) {
		// クローズとオープンを行う(ここではVMロックされているが、問題ない)
		Close();

		// オープンを試みる
		if (!Open()) {
			// オープン失敗の場合、ポートハンドラにリソースを解放してもらう
			pPort = (CPort*)SearchComponent(MAKEID('P', 'O', 'R', 'T'));
			ASSERT(pPort);
			pPort->ApplyCfg(pConfig);

			// もう一度オープン
			Open();
		}
	}

}

//---------------------------------------------------------------------------
//
//	オープン
//	※VMロック状態
//
//---------------------------------------------------------------------------
BOOL FASTCALL CTKey::Open()
{
	CString strFile;
	COMMTIMEOUTS cto;
	DCB dcb;

	ASSERT(this);

	// モードが0なら、割り当てない
	if (m_nMode == 0) {
		m_hCOM = INVALID_HANDLE_VALUE;
		return TRUE;
	}

	// ポートが0なら、割り当てない
	if (m_nCOM == 0) {
		m_hCOM = INVALID_HANDLE_VALUE;
		return TRUE;
	}

	// ファイル名を作成
	strFile.Format(_T("\\\\.\\COM%d"), m_nCOM);

	// オープン
	m_hCOM = ::CreateFile(  strFile,
							GENERIC_READ | GENERIC_WRITE,
							0,
							NULL,
							OPEN_EXISTING,
							FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
							0);
	if (m_hCOM == INVALID_HANDLE_VALUE) {
		return FALSE;
	}

	// バッファサイズ
	::SetupComm(m_hCOM, 0x1000, 0x1000);

	// 送受信をクリア
	m_TxQueue.Clear();
	m_RxQueue.Clear();
	::PurgeComm(m_hCOM, PURGE_TXCLEAR);
	::PurgeComm(m_hCOM, PURGE_RXCLEAR);

	// イベントマスク
	::SetCommMask(m_hCOM, 0);

	// タイムアウト
	::GetCommTimeouts(m_hCOM, &cto);
	cto.ReadIntervalTimeout = 1;
	cto.ReadTotalTimeoutMultiplier = 0;
	cto.ReadTotalTimeoutConstant = 10000;
	cto.WriteTotalTimeoutMultiplier = 0;
	cto.WriteTotalTimeoutConstant = 10000;
	::SetCommTimeouts(m_hCOM, &cto);

	// パラメータ
	::GetCommState(m_hCOM, &dcb);
	dcb.BaudRate = CBR_2400;
	dcb.fBinary = 0;
	dcb.fParity = 0;
	dcb.fOutxCtsFlow = 0;
	dcb.fOutxDsrFlow = 0;
	dcb.fDtrControl = DTR_CONTROL_ENABLE;
	dcb.fDsrSensitivity = 0;
	dcb.fTXContinueOnXoff = 0;
	dcb.fOutX = 0;
	dcb.fInX = 0;
	dcb.fErrorChar = 0;
	dcb.fNull = 0;
	dcb.fRtsControl = RTS_CONTROL_ENABLE;
	dcb.fAbortOnError = 0;
	dcb.ByteSize = 8;
	dcb.Parity = NOPARITY;
	dcb.StopBits = ONESTOPBIT;
	::SetCommState(m_hCOM, &dcb);
	::EscapeCommFunction(m_hCOM, SETDTR);

	// スレッドを立てる
	m_bReq = FALSE;
	m_pCOM = AfxBeginThread(ThreadFunc, this);
	if (!m_pCOM) {
		Close();
		return FALSE;
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	クローズ
//	※VMロック状態
//
//---------------------------------------------------------------------------
void FASTCALL CTKey::Close()
{
	int i;
	DWORD dwCompleted;
	INPUT input;

	ASSERT(this);

	// スレッドを閉じる
	if (m_pCOM) {
		// 終了要求を上げ
		m_bReq = TRUE;

		// 終了待ち(無期限)
		::WaitForSingleObject(m_pCOM->m_hThread, INFINITE);

		// スレッドはm_bAutoDeleteで自動削除される
		m_pCOM = NULL;
	}

	// ハンドルを閉じる
	if (m_hCOM != INVALID_HANDLE_VALUE) {
		// バッファパージ
		m_TxQueue.Clear();
		m_RxQueue.Clear();
		::PurgeComm(m_hCOM, PURGE_TXCLEAR);
		::PurgeComm(m_hCOM, PURGE_RXCLEAR);

		// 送信中か
		if (m_bTxValid) {
			// 送信I/Oが完了しているか
			if (!GetOverlappedResult(m_hCOM, &m_TxOver, &dwCompleted, FALSE)) {
				// まだ保留中なので、キャンセル
				CancelIo(m_hCOM);
			}
			m_bTxValid = FALSE;
		}

		// 受信I/Oが完了しているか
		if (!GetOverlappedResult(m_hCOM, &m_RxOver, &dwCompleted, FALSE)) {
			// まだ保留中なので、キャンセル
			CancelIo(m_hCOM);
		}

		// クローズ
		::CloseHandle(m_hCOM);
		m_hCOM = INVALID_HANDLE_VALUE;
	}

	// Windowsへの入力があれば、すべて解除
	for (i=0; i<KeyMax; i++) {
		if (m_bWin[i]) {
			// 構造体を作成
			memset(&input, 0, sizeof(input));
			input.ki.wVk = (WORD)m_nKey[i];
			input.ki.wScan = 0;
			input.ki.dwFlags = KEYEVENTF_KEYUP;
			input.type = INPUT_KEYBOARD;

			// 入力(キーBreak)
			::SendInput(1, &input, sizeof(INPUT));

			// フラグOFF
			m_bWin[i] = FALSE;
		}
	}
}

//---------------------------------------------------------------------------
//
//	実行
//
//---------------------------------------------------------------------------
void FASTCALL CTKey::Run()
{
	ASSERT(this);
	ASSERT(m_hCOM);
	ASSERT(m_pKeyboard);

	// キーボード内の送信バッファをクリア
	m_pKeyboard->ClrCommand();

	// 初回の受信待ち
	memset(&m_RxOver, 0, sizeof(m_RxOver));
	::ReadFile(m_hCOM, m_RxBuf, sizeof(m_RxBuf), NULL, &m_RxOver);

	// 送信なし
	m_bTxValid = FALSE;
	memset(&m_TxOver, 0, sizeof(m_TxOver));

	// RTSライン初期化
	Ctrl(TRUE);

	// 終了フラグが上がるまで
	while (!m_bReq) {
		// スリープ
		::Sleep(10);

		// イネーブルに限り
		if (m_bEnable) {
			// RTS制御
			Ctrl(FALSE);

			// 受信
			Rx();

			// 送信
			Tx();

			// バッファ同期
			BufSync();
		}
	}
}

//---------------------------------------------------------------------------
//
//	RTS制御
//
//---------------------------------------------------------------------------
void FASTCALL CTKey::Ctrl(BOOL bForce)
{
	BOOL bSet;

	ASSERT(this);
	ASSERT(m_hCOM);
	ASSERT(m_pKeyboard);

	// 初期化(送信許可)
	bSet = TRUE;

	// Win32制御のみなら、常時送信可とする。それ以外はVMの状態に従う
	if (m_nMode != 2) {
		// キーボードから状態を取得
		if (m_pKeyboard->IsSendWait()) {
			// 送信不可
			bSet = FALSE;
		}
	}

	// 反転フラグを考慮
	if (m_bRTS) {
		bSet ^= TRUE;
	}

	// 強制でなければ一致チェック
	if (!bForce) {
		if (bSet == m_bLine) {
			return;
		}
	}

	// 設定
	if (bSet) {
		::EscapeCommFunction(m_hCOM, CLRRTS);
		m_bLine = TRUE;
	}
	else {
		::EscapeCommFunction(m_hCOM, SETRTS);
		m_bLine = FALSE;
	}
}

//---------------------------------------------------------------------------
//
//	受信
//
//---------------------------------------------------------------------------
void FASTCALL CTKey::Rx()
{
	DWORD dwReceived;
	DWORD dwCount;
	int nKey;
	BOOL bWin;
	INPUT input;

	ASSERT(this);
	ASSERT(m_hCOM);

	// 受信完了していなければ何もしない
	dwReceived = 0;
	if (!GetOverlappedResult(m_hCOM, &m_RxOver, &dwReceived, FALSE)) {
		return;
	}

	// Windows送信可能フラグを作成
	bWin = FALSE;
	if (!m_pSch->IsEnable()) {
		// スケジューラ停止中
		if (m_nMode & 2) {
			// Windowsビットが立っている
			bWin = TRUE;
		}
	}

	// 受信が有効であれば
	if (dwReceived > 0) {
		// キーを解釈し、フラグをON/OFFする
		for (dwCount=0; dwCount<dwReceived; dwCount++) {
			nKey = m_RxBuf[dwCount] & 0x7f;
			nKey--;
			if ((nKey >= 0x00) && (nKey < KeyMax)) {
				if (m_RxBuf[dwCount] & 0x80) {
					// キー離された
					m_bKey[nKey] = FALSE;

					// Windows処理
					if (m_bWin[nKey]) {
						m_bWin[nKey] = FALSE;
						if (bWin && m_nKey[nKey]) {
							// Windowsへ送信
							memset(&input, 0, sizeof(input));
							input.ki.wVk = (WORD)m_nKey[nKey];
							input.ki.wScan = 0;
							input.ki.dwFlags = KEYEVENTF_KEYUP;
							input.type = INPUT_KEYBOARD;
							::SendInput(1, &input, sizeof(INPUT));
						}
					}
				}
				else {
					// キー押された
					m_bKey[nKey] = TRUE;

					// Windows処理
					if (!m_bWin[nKey]) {
						m_bWin[nKey] = TRUE;
						if (bWin && m_nKey[nKey]) {
							// Windowsへ送信
							memset(&input, 0, sizeof(input));
							input.ki.wVk = (WORD)m_nKey[nKey];
							input.ki.wScan = 0;
							input.type = INPUT_KEYBOARD;
							::SendInput(1, &input, sizeof(INPUT));
						}
					}
					else {
						if (bWin && m_nKey[nKey]) {
							// キーリピートエミュレーション
							memset(&input, 0, sizeof(input));
							input.ki.wVk = (WORD)m_nKey[nKey];
							input.ki.wScan = 0;
							input.ki.dwFlags = KEYEVENTF_KEYUP;
							input.type = INPUT_KEYBOARD;
							::SendInput(1, &input, sizeof(INPUT));
							memset(&input, 0, sizeof(input));
							input.ki.wVk = (WORD)m_nKey[nKey];
							input.ki.wScan = 0;
							input.type = INPUT_KEYBOARD;
							::SendInput(1, &input, sizeof(INPUT));
						}
					}
				}
			}
		}

		// キューに挿入
		m_RxQueue.Insert(m_RxBuf, dwReceived);
	}

	// 次の受信を開始
	memset(&m_RxOver, 0, sizeof(m_RxOver));
	::ReadFile(m_hCOM, m_RxBuf, sizeof(m_RxBuf), NULL, &m_RxOver);
}

//---------------------------------------------------------------------------
//
//	送信
//
//---------------------------------------------------------------------------
void FASTCALL CTKey::Tx()
{
	DWORD dwSize;
	DWORD dwSent;

	ASSERT(this);
	ASSERT(m_hCOM);

	// 前回の送信が終わっていなければ、リターン
	if (m_bTxValid) {
		if (!GetOverlappedResult(m_hCOM, &m_TxOver, &dwSent, FALSE)) {
			return;
		}

		// 送信できた分だけ、送信バッファから進める
		if (dwSent > 0) {
			m_TxQueue.Discard(dwSent);
		}

		// 現状送信なし
		m_bTxValid = FALSE;
	}

	// 送信するものがなければ、リターン
	if (m_TxQueue.IsEmpty()) {
		return;
	}

	// 送信キューから取得(ポインタは進めない)
	dwSize = m_TxQueue.Copy(m_TxBuf);

	// 送信開始
	memset(&m_TxOver, 0, sizeof(m_TxOver));
	m_bTxValid = TRUE;
	::WriteFile(m_hCOM, m_TxBuf, dwSize, NULL, &m_TxOver);
}

//---------------------------------------------------------------------------
//
//	バッファ同期
//
//---------------------------------------------------------------------------
void FASTCALL CTKey::BufSync()
{
	BYTE RxBuf[0x1000];
	DWORD dwRx;
	DWORD i;
	DWORD dwCommand;
	BYTE byCommand;

	ASSERT(this);

	// 受信データを一括取得
	dwRx = m_RxQueue.Get(RxBuf);

	// モードのbit0が有効で、かつスケジューラ動作中ならMFPへ
	if (m_nMode & 1) {
		if (m_pSch->IsEnable()) {
			// 有効なデータはすべてMFPへ送る
			for (i=0; i<dwRx; i++) {
				m_pMFP->KeyData((DWORD)RxBuf[i]);
			}
		}
	}

	// 送信データがあれば、受け取る
	for (;;) {
		// バッファがいっぱいなら、これ以上送信しない
		if (m_TxQueue.GetFree() == 0) {
			break;
		}

		// データ取得
		if (!m_pKeyboard->GetCommand(dwCommand)) {
			break;
		}

		// 送信キューへ挿入
		byCommand = (BYTE)dwCommand;
		m_TxQueue.Insert(&byCommand, 1);
	}
}

//---------------------------------------------------------------------------
//
//	送信キュー情報取得
//
//---------------------------------------------------------------------------
void FASTCALL CTKey::GetTxQueue(CQueue::LPQUEUEINFO lpqi) const
{
	ASSERT(this);
	ASSERT(lpqi);

	m_TxQueue.GetQueue(lpqi);
}

//---------------------------------------------------------------------------
//
//	受信キュー情報取得
//
//---------------------------------------------------------------------------
void FASTCALL CTKey::GetRxQueue(CQueue::LPQUEUEINFO lpqi) const
{
	ASSERT(this);
	ASSERT(lpqi);

	m_RxQueue.GetQueue(lpqi);
}

//---------------------------------------------------------------------------
//
//	キーマップ取得
//
//---------------------------------------------------------------------------
void FASTCALL CTKey::GetKeyMap(int *pMap)
{
	ASSERT(this);
	ASSERT(pMap);

	memcpy(pMap, m_nKey, sizeof(m_nKey));
}

//---------------------------------------------------------------------------
//
//	キーマップ設定
//
//---------------------------------------------------------------------------
void FASTCALL CTKey::SetKeyMap(const int *pMap)
{
	ASSERT(this);
	ASSERT(pMap);

	memcpy(m_nKey, pMap, sizeof(m_nKey));
}

//---------------------------------------------------------------------------
//
//	VK_OEMコード定義
//	※VK_OEM_CLEAR以外はPlatform SDKでの拡張。Win32SDKでは定義されていない
//
//---------------------------------------------------------------------------
#if !defined(VK_OEM_NEC_EQUAL)
#define VK_OEM_NEC_EQUAL	0x92		// '=' key on numpad
#define VK_OEM_1			0xBA		// ';:' for US
#define VK_OEM_PLUS			0xBB		// '+' any country
#define VK_OEM_COMMA		0xBC		// ',' any country
#define VK_OEM_MINUS		0xBD		// '-' any country
#define VK_OEM_PERIOD		0xBE		// '.' any country
#define VK_OEM_2			0xBF		// '/?' for US
#define VK_OEM_3			0xC0		// '`~' for US
#define VK_OEM_4			0xDB		//  '[{' for US
#define VK_OEM_5			0xDC		//  '\|' for US
#define VK_OEM_6			0xDD		//  ']}' for US
#define VK_OEM_7			0xDE		//  ''"' for US
#define VK_OEM_102			0xE2		//  "<>" or "\|" on RT 102-key kbd.
#endif	// VK_OEM_NEC_EQUAL

//---------------------------------------------------------------------------
//
//	キー変換テーブル
//
//---------------------------------------------------------------------------
const int CTKey::KeyTable[KeyMax] = {
	VK_ESCAPE,							// 01 [ESC]
	'1',								// 02 [1]
	'2',								// 03 [2]
	'3',								// 04 [3]
	'4',								// 05 [4]
	'5',								// 06 [5]
	'6',								// 07 [6]
	'7',								// 08 [7]
	'8',								// 09 [8]
	'9',								// 0A [9]
	'0',								// 0B [0]
	VK_OEM_MINUS,						// 0C [-=]
	VK_OEM_7,							// 0D [^~]
	VK_OEM_5,							// 0E [Yen|]
	VK_BACK,							// 0F [BS]

	VK_TAB,								// 10 [TAB]
	'Q',								// 11 [Q]
	'W',								// 12 [W]
	'E',								// 13 [E]
	'R',								// 14 [R]
	'T',								// 15 [T]
	'Y',								// 16 [Y]
	'U',								// 17 [U]
	'I',								// 18 [I]
	'O',								// 19 [O]
	'P',								// 1A [P]
	VK_OEM_3,							// 1B @`
	VK_OEM_4,							// 1C [{
	VK_RETURN,							// 1D [RETURN]

	'A',								// 1E [A]
	'S',								// 1F [S]
	'D',								// 20 [D]
	'F',								// 21 [F]
	'G',								// 22 [G]
	'H',								// 23 [H]
	'J',								// 24 [J]
	'K',								// 25 [K]
	'L',								// 26 [L]
	VK_OEM_PLUS,						// 27 [;+]
	VK_OEM_1,							// 28 [:*]
	VK_OEM_6,							// 29 []}]

	'Z',								// 2A [Z]
	'X',								// 2B [X]
	'C',								// 2C [C]
	'V',								// 2D [V]
	'B',								// 2E [B]
	'N',								// 2F [N]
	'M',								// 30 [M]
	VK_OEM_COMMA,						// 31 [,<]
	VK_OEM_PERIOD,						// 32 [.>]
	VK_OEM_2,							// 33 [/?]
	VK_OEM_102,							// 34 [\_]

	VK_SPACE,							// 35 [SPACE]
	VK_HOME,							// 36 [HOME]
	VK_DELETE,							// 37 [DEL]
	VK_NEXT,							// 38 [ROLL UP]
	VK_PRIOR,							// 39 [ROLL DOWN]
	VK_END,								// 3A [UNDO]
	VK_LEFT,							// 3B [←]
	VK_UP,								// 3C [↑]
	VK_RIGHT,							// 3D [→]
	VK_DOWN,							// 3E [↓]

	VK_CLEAR,							// 3F [CLR]
	VK_DIVIDE,							// 40 [/]
	VK_MULTIPLY,						// 41 [*]
	VK_SUBTRACT,						// 42 [-]
	VK_NUMPAD7,							// 43 [7]
	VK_NUMPAD8,							// 44 [8]
	VK_NUMPAD9,							// 45 [9]
	VK_ADD,								// 46 [+]
	VK_NUMPAD4,							// 47 [4]
	VK_NUMPAD5,							// 48 [5]
	VK_NUMPAD6,							// 49 [6]
	VK_OEM_NEC_EQUAL,					// 4A [=]
	VK_NUMPAD1,							// 4B [1]
	VK_NUMPAD2,							// 4C [2]
	VK_NUMPAD3,							// 4D [3]
	VK_RETURN,							// 4E [ENTER]
	VK_NUMPAD0,							// 4F [0]
	VK_OEM_COMMA,						// 50 [.>]
	VK_DECIMAL,							// 51 [.]

	VK_PRINT,							// 52 [記号入力]
	VK_SCROLL,							// 53 [登録]
	VK_PAUSE,							// 54 [HELP]

	VK_MENU,							// 55 [XF1]
	VK_KANJI,							// 56 [XF2]
	VK_CONVERT,							// 57 [XF3]
	VK_NONCONVERT,						// 58 [XF4]
	VK_KANA,							// 59 [XF5]

	0,									// 5A [かな]
	0,									// 5B [ローマ字]
	0,									// 5C [コード入力]
	VK_CAPITAL,							// 5D [CAPS]

	VK_INSERT,							// 5E [INS]

	0,									// 5F [ひらがな]
	0,									// 60 [全角]

	0,									// 61 [BREAK]
	0,									// 62 [COPY]

	VK_F1,								// 63 [F1]
	VK_F2,								// 64 [F2]
	VK_F3,								// 65 [F3]
	VK_F4,								// 66 [F4]
	VK_F5,								// 67 [F5]
	VK_F6,								// 68 [F6]
	VK_F7,								// 69 [F7]
	VK_F8,								// 6A [F8]
	VK_F9,								// 6B [F9]
	VK_F10,								// 6C [F10]

	0,									// 6D [空き]
	0,									// 6E [空き]
	0,									// 6F [空き]

	VK_SHIFT,							// 70 [SHIFT]
	VK_CONTROL,							// 71 [CTRL]
	VK_LWIN,							// 72 [OPT.1]
	VK_APPS								// 73 [OPT.2]
};

//---------------------------------------------------------------------------
//
//	VKキーID取得
//
//---------------------------------------------------------------------------
LPCTSTR CTKey::GetKeyID(int nVK)
{
	int i;

	ASSERT(this);
	ASSERT((nVK >= 0) && (nVK < 0x100));

	// 0はNULL
	if (nVK == 0) {
		return NULL;
	}

	// 検索
	for (i=0; ; i++) {
		// nVKに0が出れば終端
		if (KeyIDTable[i].nVK == 0) {
			break;
		}

		// 一致すればOK
		if (nVK == KeyIDTable[i].nVK) {
			// キーID名を返す
			return KeyIDTable[i].lpszID;
		}
	}

	// 見つからなかった
	return NULL;
}

//---------------------------------------------------------------------------
//
//	VKキーIDテーブル
//
//---------------------------------------------------------------------------
const CTKey::VKEYID CTKey::KeyIDTable[] = {
	{ 0x01, _T("VK_LBUTTON") },
	{ 0x02, _T("VK_RBUTTON") },
	{ 0x03, _T("VK_CANCEL") },
	{ 0x04, _T("VK_MBUTTON") },
	{ 0x05, _T("VK_XBUTTON1") },
	{ 0x06, _T("VK_XBUTTON2") },
	{ 0x08, _T("VK_BACK") },
	{ 0x09, _T("VK_TAB") },
	{ 0x0C, _T("VK_CLEAR") },
	{ 0x0D, _T("VK_RETURN") },
	{ 0x10, _T("VK_SHIFT") },
	{ 0x11, _T("VK_CONTROL") },
	{ 0x12, _T("VK_MENU") },
	{ 0x13, _T("VK_PAUSE") },
	{ 0x14, _T("VK_CAPITAL") },
	{ 0x15, _T("VK_KANA") },
	{ 0x17, _T("VK_JUNJA") },
	{ 0x18, _T("VK_FINAL") },
	{ 0x19, _T("VK_KANJI") },
	{ 0x1B, _T("VK_ESCAPE") },
	{ 0x1C, _T("VK_CONVERT") },
	{ 0x1D, _T("VK_NONCONVERT") },
	{ 0x1E, _T("VK_ACCEPT") },
	{ 0x1F, _T("VK_MODECHANGE") },
	{ 0x20, _T("VK_SPACE") },
	{ 0x21, _T("VK_PRIOR") },
	{ 0x22, _T("VK_NEXT") },
	{ 0x23, _T("VK_END") },
	{ 0x24, _T("VK_HOME") },
	{ 0x25, _T("VK_LEFT") },
	{ 0x26, _T("VK_UP") },
	{ 0x27, _T("VK_RIGHT") },
	{ 0x28, _T("VK_DOWN") },
	{ 0x29, _T("VK_SELECT") },
	{ 0x2A, _T("VK_PRINT") },
	{ 0x2B, _T("VK_EXECUTE") },
	{ 0x2C, _T("VK_SNAPSHOT") },
	{ 0x2D, _T("VK_INSERT") },
	{ 0x2E, _T("VK_DELETE") },
	{ 0x2F, _T("VK_HELP") },
	{ 0x30, _T("VK_0") },
	{ 0x31, _T("VK_1") },
	{ 0x32, _T("VK_2") },
	{ 0x33, _T("VK_3") },
	{ 0x34, _T("VK_4") },
	{ 0x35, _T("VK_5") },
	{ 0x36, _T("VK_6") },
	{ 0x37, _T("VK_7") },
	{ 0x38, _T("VK_8") },
	{ 0x39, _T("VK_9") },
	{ 0x41, _T("VK_A") },
	{ 0x42, _T("VK_B") },
	{ 0x43, _T("VK_C") },
	{ 0x44, _T("VK_D") },
	{ 0x45, _T("VK_E") },
	{ 0x46, _T("VK_F") },
	{ 0x47, _T("VK_G") },
	{ 0x48, _T("VK_H") },
	{ 0x49, _T("VK_I") },
	{ 0x4A, _T("VK_J") },
	{ 0x4B, _T("VK_K") },
	{ 0x4C, _T("VK_L") },
	{ 0x4D, _T("VK_M") },
	{ 0x4E, _T("VK_N") },
	{ 0x4F, _T("VK_O") },
	{ 0x50, _T("VK_P") },
	{ 0x51, _T("VK_Q") },
	{ 0x52, _T("VK_R") },
	{ 0x53, _T("VK_S") },
	{ 0x54, _T("VK_T") },
	{ 0x55, _T("VK_U") },
	{ 0x56, _T("VK_V") },
	{ 0x57, _T("VK_W") },
	{ 0x58, _T("VK_X") },
	{ 0x59, _T("VK_Y") },
	{ 0x5A, _T("VK_Z") },
	{ 0x5B, _T("VK_LWIN") },
	{ 0x5C, _T("VK_RWIN") },
	{ 0x5D, _T("VK_APPS") },
	{ 0x5F, _T("VK_SLEEP") },
	{ 0x60, _T("VK_NUMPAD0") },
	{ 0x61, _T("VK_NUMPAD1") },
	{ 0x62, _T("VK_NUMPAD2") },
	{ 0x63, _T("VK_NUMPAD3") },
	{ 0x64, _T("VK_NUMPAD4") },
	{ 0x65, _T("VK_NUMPAD5") },
	{ 0x66, _T("VK_NUMPAD6") },
	{ 0x67, _T("VK_NUMPAD7") },
	{ 0x68, _T("VK_NUMPAD8") },
	{ 0x69, _T("VK_NUMPAD9") },
	{ 0x6A, _T("VK_MULTIPLY") },
	{ 0x6B, _T("VK_ADD") },
	{ 0x6C, _T("VK_SEPARATOR") },
	{ 0x6D, _T("VK_SUBTRACT") },
	{ 0x6E, _T("VK_DECIMAL") },
	{ 0x6F, _T("VK_DIVIDE") },
	{ 0x70, _T("VK_F1") },
	{ 0x71, _T("VK_F2") },
	{ 0x72, _T("VK_F3") },
	{ 0x73, _T("VK_F4") },
	{ 0x74, _T("VK_F5") },
	{ 0x75, _T("VK_F6") },
	{ 0x76, _T("VK_F7") },
	{ 0x77, _T("VK_F8") },
	{ 0x78, _T("VK_F9") },
	{ 0x79, _T("VK_F10") },
	{ 0x7A, _T("VK_F11") },
	{ 0x7B, _T("VK_F12") },
	{ 0x7C, _T("VK_F13") },
	{ 0x7D, _T("VK_F14") },
	{ 0x7E, _T("VK_F15") },
	{ 0x7F, _T("VK_F16") },
	{ 0x80, _T("VK_F17") },
	{ 0x81, _T("VK_F18") },
	{ 0x82, _T("VK_F19") },
	{ 0x83, _T("VK_F20") },
	{ 0x84, _T("VK_F21") },
	{ 0x85, _T("VK_F22") },
	{ 0x86, _T("VK_F23") },
	{ 0x87, _T("VK_F24") },
	{ 0x90, _T("VK_NUMLOCK") },
	{ 0x91, _T("VK_SCROLL") },
	{ 0x92, _T("VK_OEM_NEC_EQUAL") },
	{ 0x93, _T("VK_OEM_FJ_MASSHOU") },
	{ 0x94, _T("VK_OEM_FJ_TOUROKU") },
	{ 0x95, _T("VK_OEM_FJ_LOYA") },
	{ 0x96, _T("VK_OEM_FJ_ROYA") },
	{ 0xA0, _T("VK_LSHIFT") },
	{ 0xA1, _T("VK_RSHIFT") },
	{ 0xA2, _T("VK_LCONTROL") },
	{ 0xA3, _T("VK_RCONTROL") },
	{ 0xA4, _T("VK_LMENU") },
	{ 0xA5, _T("VK_RMENU") },
	{ 0xA6, _T("VK_BROWSER_BACK") },
	{ 0xA7, _T("VK_BROWSER_FORWARD") },
	{ 0xA8, _T("VK_BROWSER_REFRESH") },
	{ 0xA9, _T("VK_BROWSER_STOP") },
	{ 0xAA, _T("VK_BROWSER_SEARCH") },
	{ 0xAB, _T("VK_BROWSER_FAVORITES") },
	{ 0xAC, _T("VK_BROWSER_HOME") },
	{ 0xAD, _T("VK_VOLUME_MUTE") },
	{ 0xAE, _T("VK_VOLUME_DOWN") },
	{ 0xAF, _T("VK_VOLUME_UP") },
	{ 0xB0, _T("VK_MEDIA_NEXT_TRACK") },
	{ 0xB1, _T("VK_MEDIA_PREV_TRACK") },
	{ 0xB2, _T("VK_MEDIA_STOP") },
	{ 0xB3, _T("VK_MEDIA_PLAY_PAUSE") },
	{ 0xB4, _T("VK_LAUNCH_MAIL") },
	{ 0xB5, _T("VK_LAUNCH_MEDIA_SELECT") },
	{ 0xB6, _T("VK_LAUNCH_APP1") },
	{ 0xB7, _T("VK_LAUNCH_APP2") },
	{ 0xBA, _T("VK_OEM_1") },
	{ 0xBB, _T("VK_OEM_PLUS") },
	{ 0xBC, _T("VK_OEM_COMMA") },
	{ 0xBD, _T("VK_OEM_MINUS") },
	{ 0xBE, _T("VK_OEM_PERIOD") },
	{ 0xBF, _T("VK_OEM_2") },
	{ 0xC0, _T("VK_OEM_3") },
	{ 0xDB, _T("VK_OEM_4") },
	{ 0xDC, _T("VK_OEM_5") },
	{ 0xDD, _T("VK_OEM_6") },
	{ 0xDE, _T("VK_OEM_7") },
	{ 0xDF, _T("VK_OEM_8") },
	{ 0xE1, _T("VK_OEM_AX") },
	{ 0xE2, _T("VK_OEM_102") },
	{ 0xE3, _T("VK_ICO_HELP") },
	{ 0xE4, _T("VK_ICO_00") },
	{ 0xE5, _T("VK_PROCESSKEY") },
	{ 0xE6, _T("VK_ICO_CLEAR") },
	{ 0xE7, _T("VK_PACKET") },
	{ 0xE9, _T("VK_OEM_RESET") },
	{ 0xEA, _T("VK_OEM_JUMP") },
	{ 0xEB, _T("VK_OEM_PA1") },
	{ 0xEC, _T("VK_OEM_PA2") },
	{ 0xED, _T("VK_OEM_PA3") },
	{ 0xEE, _T("VK_OEM_WSCTRL") },
	{ 0xEF, _T("VK_OEM_CUSEL") },
	{ 0xF0, _T("VK_OEM_ATTN") },
	{ 0xF1, _T("VK_OEM_FINISH") },
	{ 0xF2, _T("VK_OEM_COPY") },
	{ 0xF3, _T("VK_OEM_AUTO") },
	{ 0xF4, _T("VK_OEM_ENLW") },
	{ 0xF5, _T("VK_OEM_BACKTAB") },
	{ 0xF6, _T("VK_ATTN") },
	{ 0xF7, _T("VK_CRSEL") },
	{ 0xF8, _T("VK_EXSEL") },
	{ 0xF9, _T("VK_EREOF") },
	{ 0xFA, _T("VK_PLAY") },
	{ 0xFB, _T("VK_ZOOM") },
	{ 0xFC, _T("VK_NONAME") },
	{ 0xFD, _T("VK_PA1") },
	{ 0xFE, _T("VK_OEM_CLEAR") },
	{ 0x00, NULL }
};

#endif	// _WIN32
