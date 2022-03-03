//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2005 ＰＩ．(ytanaka@ipc-tokai.or.jp)
//	[ MFC ポート ]
//
//---------------------------------------------------------------------------

#if defined(_WIN32)

#include "os.h"
#include "xm6.h"
#include "vm.h"
#include "scc.h"
#include "printer.h"
#include "config.h"
#include "filepath.h"
#include "fileio.h"
#include "mfc_com.h"
#include "mfc_cfg.h"
#include "mfc_que.h"
#include "mfc_port.h"

//===========================================================================
//
//	ポート
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	COMスレッド関数
//
//---------------------------------------------------------------------------
static UINT COMThread(LPVOID pParam)
{
	CPort *pPort;

	// パラメータを受け取る
	pPort = (CPort*)pParam;
	ASSERT(pPort);

	// 実行
	pPort->RunCOM();

	// 終了コードを持ってスレッドを終了
	return 0;
}

//---------------------------------------------------------------------------
//
//	LPTスレッド関数
//
//---------------------------------------------------------------------------
static UINT LPTThread(LPVOID pParam)
{
	CPort *pPort;

	// パラメータを受け取る
	pPort = (CPort*)pParam;
	ASSERT(pPort);

	// 実行
	pPort->RunLPT();

	// 終了コードを持ってスレッドを終了
	return 0;
}

//---------------------------------------------------------------------------
//
//	コンストラクタ
//
//---------------------------------------------------------------------------
CPort::CPort(CFrmWnd *pWnd) : CComponent(pWnd)
{
	// コンポーネントパラメータ
	m_dwID = MAKEID('P', 'O', 'R', 'T');
	m_strDesc = _T("Port Handler");

	// シリアルポート
	m_nCOM = 0;
	m_hCOM = INVALID_HANDLE_VALUE;
	m_pCOM = NULL;
	m_bCOMReq = FALSE;
	m_bBreak = FALSE;
	m_bRTS = FALSE;
	m_bDTR = FALSE;
	m_bCTS = FALSE;
	m_bDSR = FALSE;
	m_bTxValid = FALSE;
	memset(&m_TxOver, 0, sizeof(m_TxOver));
	m_dwErr = 0;
	memset(&m_RxOver, 0, sizeof(m_RxOver));
	m_RecvLog[0] = _T('\0');
	m_bForce = FALSE;
	m_pSCC = NULL;

	// パラレルポート
	m_nLPT = 0;
	m_hLPT = INVALID_HANDLE_VALUE;
	m_pLPT = NULL;
	m_bLPTValid = FALSE;
	m_bLPTReq = FALSE;
	memset(&m_LPTOver, 0, sizeof(m_LPTOver));
	m_SendLog[0] = _T('\0');
	m_pPrinter = NULL;
}

//---------------------------------------------------------------------------
//
//	初期化
//
//---------------------------------------------------------------------------
BOOL FASTCALL CPort::Init()
{
	ASSERT(this);

	// 基本クラス
	if (!CComponent::Init()) {
		return FALSE;
	}

	// SCC取得
	ASSERT(!m_pSCC);
	m_pSCC = (SCC*)::GetVM()->SearchDevice(MAKEID('S', 'C', 'C', ' '));
	ASSERT(m_pSCC);

	// シリアルキュー初期化
	if (!m_TxQueue.Init(0x1000)) {
		return FALSE;
	}
	if (!m_RxQueue.Init(0x1000)) {
		return FALSE;
	}

	// シリアルポート
	OpenCOM();

	// プリンタ取得
	ASSERT(!m_pPrinter);
	m_pPrinter = (Printer*)::GetVM()->SearchDevice(MAKEID('P', 'R', 'N', ' '));
	ASSERT(m_pPrinter);

	// パラレルキュー初期化
	if (!m_LPTQueue.Init(0x1000)) {
		return FALSE;
	}

	// パラレルポート
	OpenLPT();

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	クリーンアップ
//
//---------------------------------------------------------------------------
void FASTCALL CPort::Cleanup()
{
	ASSERT(this);

	// パラレルポート
	CloseLPT();

	// シリアルポート
	CloseCOM();

	// 基本クラス
	CComponent::Cleanup();
}

//---------------------------------------------------------------------------
//
//	設定適用
//
//---------------------------------------------------------------------------
void FASTCALL CPort::ApplyCfg(const Config* pConfig)
{
	BOOL bCOM;
	BOOL bLPT;

	ASSERT(this);
	ASSERT(pConfig);

	// COMフラグ初期化
	bCOM = FALSE;

	// COMポート番号が違っているか
	if (pConfig->port_com != (int)m_nCOM) {
		m_nCOM = pConfig->port_com;
		bCOM = TRUE;

		// TrueKeyとポート番号が重なっていたら、TrueKeyを優先
		if (pConfig->port_com == pConfig->tkey_com) {
			m_nCOM = 0;
		}
	}

	// 受信ファイル名が違っているか
	if (_tcscmp(pConfig->port_recvlog, m_RecvLog) != 0) {
		ASSERT(_tcslen(pConfig->port_recvlog) < _MAX_PATH);
		_tcscpy(m_RecvLog, pConfig->port_recvlog);
		bCOM = TRUE;
	}

	// 強制フラグが違っているか
	if (pConfig->port_384 != m_bForce) {
		m_bForce = pConfig->port_384;
		bCOM = TRUE;
	}

	// 変更点があれば
	if (bCOM) {
		// クローズ成功させるため、一度ロックしているVMを離す
		::UnlockVM();
		CloseCOM();
		OpenCOM();
		::LockVM();
	}

	// LPTフラグ初期化
	bLPT = FALSE;

	// LPTポート番号が違っているか
	if (pConfig->port_lpt != (int)m_nLPT) {
		m_nLPT = pConfig->port_lpt;
		bLPT = TRUE;
	}

	// 送信ファイル名が違っているか
	if (strcmp(pConfig->port_sendlog, m_SendLog) != 0) {
		ASSERT(strlen(pConfig->port_sendlog) < sizeof(m_SendLog));
		strcpy(m_SendLog, pConfig->port_sendlog);
		bLPT = TRUE;
	}

	// 変更点があれば
	if (bLPT) {
		// クローズ成功させるため、一度ロックしているVMを離す
		::UnlockVM();
		CloseLPT();
		OpenLPT();
		::LockVM();
	}
}

//===========================================================================
//
//	シリアルポート
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	COMオープン
//	※VMロック状態
//
//---------------------------------------------------------------------------
BOOL FASTCALL CPort::OpenCOM()
{
	CString strFile;
	DCB dcb;

	ASSERT(this);

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

	// 信号線の初期設定
	OnErr();
	OnCTSDSR();
	::GetCommState(m_hCOM, &dcb);
	::LockVM();
	AdjustCOM(&dcb);
	SignalCOM();
	::UnlockVM();
	CtrlCOM();

	// ログファイルの作成(アペンド)
	AppendCOM();

	// スレッドを立てる
	m_bCOMReq = FALSE;
	m_pCOM = AfxBeginThread(COMThread, this);
	if (!m_pCOM) {
		CloseCOM();
		return FALSE;
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	COMログファイル
//
//---------------------------------------------------------------------------
void FASTCALL CPort::AppendCOM()
{
	TCHAR szDrive[_MAX_DRIVE];
	TCHAR szDir[_MAX_DIR];
	TCHAR szFile[_MAX_FNAME];
	TCHAR szExt[_MAX_EXT];
	Filepath path;

	ASSERT(this);

	// ログファイル指定が空であれば、必要ない
	if (_tcslen(m_RecvLog) == 0) {
		return;
	}

	// 分割
	_tsplitpath(m_RecvLog, szDrive, szDir, szFile, szExt);

	// フルパス指定でなければ相対パスを採用
	path.SetPath(m_RecvLog);
	if ((_tcslen(szDrive) == 0) && (szDir[0] != _T('\\'))) {
		path.SetBaseDir();
	}

	// アペンドオープン
	m_RecvFile.Open(path, Fileio::Append);
}

//---------------------------------------------------------------------------
//
//	COMクローズ
//	※VMロック状態
//
//---------------------------------------------------------------------------
void FASTCALL CPort::CloseCOM()
{
	DWORD dwCompleted;

	ASSERT(this);

	// スレッドを閉じる
	if (m_pCOM) {
		// 終了要求を上げ
		m_bCOMReq = TRUE;

		// 終了待ち(無期限)
		::WaitForSingleObject(m_pCOM->m_hThread, INFINITE);

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

	// ログファイルクローズ
	m_RecvFile.Close();

	// SCCに対し、送信ウェイト解除など
	m_pSCC->WaitTx(0, FALSE);
	m_pSCC->SetCTS(0, FALSE);
	m_pSCC->SetDCD(0, FALSE);
}

//---------------------------------------------------------------------------
//
//	COM実行
//
//---------------------------------------------------------------------------
void FASTCALL CPort::RunCOM()
{
	COMMTIMEOUTS cto;
	DCB dcb;

	ASSERT(this);
	ASSERT(m_hCOM);

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

	// 受信待ち
	memset(&m_RxOver, 0, sizeof(m_RxOver));
	::ReadFile(m_hCOM, m_RxBuf, sizeof(m_RxBuf), NULL, &m_RxOver);

	// 送信なし
	m_bTxValid = FALSE;
	memset(&m_TxOver, 0, sizeof(m_TxOver));

	// 終了フラグが上がるまで
	while (!m_bCOMReq) {
		// スリープ
		::Sleep(5);

		// CTS, DSR
		OnCTSDSR();

		// エラー
		OnErr();

		// 受信
		OnRx();

		// Win32側情報を取得
		::GetCommState(m_hCOM, &dcb);

		// VMロック
		::LockVM();

		// 同期処理
		AdjustCOM(&dcb);
		SignalCOM();
		BufCOM();

		// VMアンロック
		::UnlockVM();

		// 信号線制御
		CtrlCOM();

		// 送信
		OnTx();
	}
}

//---------------------------------------------------------------------------
//
//	パラメータあわせ
//
//---------------------------------------------------------------------------
void FASTCALL CPort::AdjustCOM(DCB *pDCB)
{
	DCB dcb;
	const SCC::scc_t *scc;
	const SCC::ch_t *p;
	int i;
	BOOL bFlag;
	DWORD dwValue;
	static DWORD dwTable[] = {
		CBR_110,
		CBR_300,
		CBR_600,
		CBR_1200,
		CBR_2400,
		CBR_4800,
		CBR_9600,
		CBR_14400,
		CBR_19200,
		31250,
		CBR_38400,
		CBR_57600,
		CBR_115200,
		0
	};

	ASSERT(this);
	ASSERT(m_pSCC);
	ASSERT(m_hCOM);
	ASSERT(pDCB);

	// 変更有無をオフ
	bFlag = FALSE;

	// SCC側情報を取得
	scc = m_pSCC->GetWork();
	p = &scc->ch[0];

	// Win32側情報を取得
	memcpy(&dcb, pDCB, sizeof(dcb));

	// ボーレート
	dwValue = CBR_300;
	for (i=0;; i++) {
		// テーブル終端であれば300bps
		if (dwTable[i] == 0) {
			break;
		}
		// 上下5%の幅でマッチさせる
		if (MatchCOM(p->baudrate, dwTable[i])) {
			dwValue = dwTable[i];
			break;
		}
	}

	// 強制38400bps
	if (m_bForce) {
		dwValue = CBR_38400;
	}

	// 比較
	if (dcb.BaudRate != dwValue) {
		dcb.BaudRate = dwValue;
		bFlag = TRUE;
	}

	// バイナリモード
	dcb.fBinary = 0;

	// パリティ有無
	if (p->parity == 0) {
		dwValue = 0;
	}
	else {
		dwValue = 1;
	}
	if (dcb.fParity != dwValue) {
		dcb.fParity = dwValue;
	}

	// CTSフロー
	if (dcb.fOutxCtsFlow != 0) {
		dcb.fOutxCtsFlow = 0;
		bFlag = TRUE;
	}

	// DSRフロー
	if (dcb.fOutxDsrFlow != 0) {
		dcb.fOutxDsrFlow = 0;
		bFlag = TRUE;
	}

	// DTR制御
	if (dcb.fDtrControl != DTR_CONTROL_ENABLE) {
		dcb.fDtrControl = DTR_CONTROL_ENABLE;
		bFlag = TRUE;
	}

	// DSRセンス
	if (dcb.fDsrSensitivity != 0) {
		dcb.fDsrSensitivity = 0;
		bFlag = TRUE;
	}

	// XON,XOFF
	if (dcb.fTXContinueOnXoff != 0) {
		dcb.fTXContinueOnXoff = 0;
		bFlag = TRUE;
	}

	// OutX
	if (dcb.fOutX != 0) {
		dcb.fOutX = 0;
		bFlag = TRUE;
	}

	// InX
	if (dcb.fInX != 0) {
		dcb.fInX = 0;
		bFlag = TRUE;
	}

	// ErrorChar
	if (dcb.fErrorChar != 0) {
		dcb.fErrorChar = 0;
		bFlag = TRUE;
	}

	// fRTSControl
	if (dcb.fRtsControl != RTS_CONTROL_ENABLE) {
		dcb.fRtsControl = RTS_CONTROL_ENABLE;
		bFlag = TRUE;
	}

	// fAbortOnError
	if (dcb.fAbortOnError != TRUE) {
		dcb.fAbortOnError = TRUE;
		bFlag = TRUE;
	}

	// データビット長
	if (dcb.ByteSize != (BYTE)p->rxbit) {
		if (p->rxbit == p->txbit) {
			if (p->rxbit >= 5) {
				dcb.ByteSize = (BYTE)p->rxbit;
				bFlag = TRUE;
			}
		}
	}

	// パリティ
	switch (p->parity) {
		case 0:
			dwValue = NOPARITY;
			break;
		case 1:
			dwValue = ODDPARITY;
			break;
		case 2:
			dwValue = EVENPARITY;
			break;
		default:
			ASSERT(FALSE);
			break;
	}
	if (dcb.Parity != (BYTE)dwValue) {
		dcb.Parity = (BYTE)dwValue;
		bFlag = TRUE;
	}

	// ストップビット
	switch (p->stopbit) {
		case 1:
			dwValue = ONESTOPBIT;
			break;
		case 2:
			dwValue = ONE5STOPBITS;
			break;
		case 3:
			dwValue = TWOSTOPBITS;
			break;
		default:
			dwValue = ONESTOPBIT;
			break;
	}
	if (dcb.StopBits != (BYTE)dwValue) {
		dcb.StopBits = (BYTE)dwValue;
		bFlag = TRUE;
	}

	// 異なっていれば
	if (bFlag) {
		// 設定
		::SetCommState(m_hCOM, &dcb);

		// バッファクリア
		m_TxQueue.Clear();
		m_RxQueue.Clear();

		// Win32側パージ
		::PurgeComm(m_hCOM, PURGE_TXCLEAR);
		::PurgeComm(m_hCOM, PURGE_RXCLEAR);
	}
}

//---------------------------------------------------------------------------
//
//	ボーレートマッチ
//
//---------------------------------------------------------------------------
BOOL FASTCALL CPort::MatchCOM(DWORD dwSCC, DWORD dwBase)
{
	DWORD dwOffset;

	ASSERT(this);

	// オリジナルに対して5%のオフセット
	dwOffset = (dwBase * 5);
	dwOffset /= 100;

	// 範囲に収まっていればOK
	if ((dwBase - dwOffset) <= dwSCC) {
		if (dwSCC <= (dwBase + dwOffset)) {
			return TRUE;
		}
	}

	// 違う
	return FALSE;
}

//---------------------------------------------------------------------------
//
//	信号線あわせ
//
//---------------------------------------------------------------------------
void FASTCALL CPort::SignalCOM()
{
	ASSERT(this);
	ASSERT(m_pSCC);

	// VMへ通知(エラー)
	if (m_dwErr & CE_BREAK) {
		m_pSCC->SetBreak(0, TRUE);
	}
	else {
		m_pSCC->SetBreak(0, FALSE);
	}
	if (m_dwErr & CE_FRAME) {
		m_pSCC->FramingErr(0);
	}
	if (m_dwErr & CE_RXPARITY) {
		m_pSCC->ParityErr(0);
	}

	// VMへ通知(信号線)
	m_pSCC->SetCTS(0, m_bCTS);
	m_pSCC->SetDCD(0, m_bDSR);

	// VMから通知(信号線)
	m_bBreak = m_pSCC->GetBreak(0);
	m_bRTS = m_pSCC->GetRTS(0);
	m_bDTR = m_pSCC->GetDTR(0);
}

//---------------------------------------------------------------------------
//
//	バッファあわせ
//
//---------------------------------------------------------------------------
void FASTCALL CPort::BufCOM()
{
	DWORD i;
	BYTE RxBuf[0x1000];
	DWORD dwRx;
	BYTE TxData;

	ASSERT(this);
	ASSERT(m_pSCC);

	// 受信データを一括取得
	dwRx = m_RxQueue.Get(RxBuf);

	// 有効なデータはすべてSCCへ送る
	for (i=0; i<dwRx; i++) {
		m_pSCC->Send(0, RxBuf[i]);
	}

	// SCC側バッファの状態を見て、送信ウェイト制御
	if (m_pSCC->IsTxFull(0)) {
		// バッファが一杯なので、これ以上送信させない
		m_pSCC->WaitTx(0, TRUE);
	}
	else {
		// 送信可
		m_pSCC->WaitTx(0, FALSE);
	}

	// 送信データがあれば、受け取る
	while (!m_pSCC->IsTxEmpty(0)) {
		// バッファを超えない範囲に押さえる
		if (m_TxQueue.GetFree() == 0) {
			break;
		}

		TxData = (BYTE)m_pSCC->Receive(0);
		m_TxQueue.Insert(&TxData, 1);
	}
}

//---------------------------------------------------------------------------
//
//	信号線制御
//
//---------------------------------------------------------------------------
void FASTCALL CPort::CtrlCOM()
{
	ASSERT(this);
	ASSERT(m_hCOM);

	// ブレーク
	if (m_bBreak) {
		::EscapeCommFunction(m_hCOM, SETBREAK);
	}
	else {
		::EscapeCommFunction(m_hCOM, CLRBREAK);
	}

	// RTS
	if (m_bRTS) {
		::EscapeCommFunction(m_hCOM, SETRTS);
	}
	else {
		::EscapeCommFunction(m_hCOM, CLRRTS);
	}

	// DTR
	if (m_bDTR) {
		::EscapeCommFunction(m_hCOM, SETDTR);
	}
	else {
		::EscapeCommFunction(m_hCOM, CLRDTR);
	}
}

//---------------------------------------------------------------------------
//
//	CTS,DSR
//
//---------------------------------------------------------------------------
void FASTCALL CPort::OnCTSDSR()
{
	DWORD dwStat;

	ASSERT(this);
	ASSERT(m_hCOM);

	// 状態取得
	::GetCommModemStatus(m_hCOM, &dwStat);

	// CTS
	if (dwStat & MS_CTS_ON) {
		m_bCTS = TRUE;
	}
	else {
		m_bCTS = FALSE;
	}

	// DSR
	if (dwStat & MS_DSR_ON) {
		m_bDSR = TRUE;
	}
	else {
		m_bDSR = FALSE;
	}
}

//---------------------------------------------------------------------------
//
//	受信エラー
//
//---------------------------------------------------------------------------
void FASTCALL CPort::OnErr()
{
	ASSERT(this);
	ASSERT(m_hCOM);

	// エラーをクリアし、同時に情報を記録
	::ClearCommError(m_hCOM, &m_dwErr, NULL);
}

//---------------------------------------------------------------------------
//
//	受信サブ
//
//---------------------------------------------------------------------------
void FASTCALL CPort::OnRx()
{
	DWORD dwReceived;

	ASSERT(this);
	ASSERT(m_hCOM);

	// 受信完了していなければ何もしない
	dwReceived = 0;
	if (!GetOverlappedResult(m_hCOM, &m_RxOver, &dwReceived, FALSE)) {
		return;
	}

	// 受信が有効であれば
	if (dwReceived > 0) {
		// 受信ログファイルが有効であれば、書き込む
		if (m_RecvFile.IsValid()) {
			m_RecvFile.Write(m_RxBuf, dwReceived);
		}

		// キューに挿入
		m_RxQueue.Insert(m_RxBuf, dwReceived);
	}

	// 次の受信
	memset(&m_RxOver, 0, sizeof(m_RxOver));
	::ReadFile(m_hCOM, m_RxBuf, sizeof(m_RxBuf), NULL, &m_RxOver);
}

//---------------------------------------------------------------------------
//
//	送信サブ
//
//---------------------------------------------------------------------------
void FASTCALL CPort::OnTx()
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
//	COM情報取得
//
//---------------------------------------------------------------------------
BOOL FASTCALL CPort::GetCOMInfo(LPTSTR lpszDevFile, DWORD *dwLogFile) const
{
	ASSERT(this);
	ASSERT(lpszDevFile);
	ASSERT(dwLogFile);

	// ファイルはヌル、ハンドルは無効
	_tcscpy(lpszDevFile, _T("  (None)"));
	*dwLogFile = (DWORD)INVALID_HANDLE_VALUE;

	// スレッドが立っていなければ、この時点でFALSEで帰る
	if (!m_pCOM) {
		return FALSE;
	}

	// デバイスファイルを設定
	if (m_nCOM > 0) {
		_stprintf(lpszDevFile, _T("\\\\.\\COM%d"), m_nCOM);
	}

	// ログファイルを設定
	if (m_RecvFile.IsValid()) {
		*dwLogFile = (DWORD)m_RecvFile.GetHandle();
	}

	// スレッドは動いている
	return TRUE;
}

//---------------------------------------------------------------------------
//
//	送信キュー情報取得
//
//---------------------------------------------------------------------------
void FASTCALL CPort::GetTxQueue(CQueue::LPQUEUEINFO lpqi) const
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
void FASTCALL CPort::GetRxQueue(CQueue::LPQUEUEINFO lpqi) const
{
	ASSERT(this);
	ASSERT(lpqi);

	m_RxQueue.GetQueue(lpqi);
}

//===========================================================================
//
//	パラレルポート
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	LPTオープン
//	※VMロック状態
//
//---------------------------------------------------------------------------
BOOL FASTCALL CPort::OpenLPT()
{
	COMMTIMEOUTS cto;
	CString strFile;

	ASSERT(this);

	// ハンドル初期化
	m_hLPT = INVALID_HANDLE_VALUE;
	m_pLPT = NULL;
	m_bLPTValid = FALSE;

	// ログファイルの作成(アペンド)
	AppendLPT();

	// ポートが0でない場合、LPTデバイスへ出力
	if (m_nLPT != 0) {
		// ファイル名を作成
		strFile.Format(_T("\\\\.\\LPT%d"), m_nLPT);

		// オープン
		m_hLPT = ::CreateFile(  strFile,
								GENERIC_WRITE,
								0,
								NULL,
								OPEN_EXISTING,
								FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
								0);

		// オープンできていたら設定
		if (m_hLPT != INVALID_HANDLE_VALUE) {
			// バッファサイズ
			::SetupComm(m_hLPT, 0x400, 0x1000);

			// イベントマスク
			::SetCommMask(m_hLPT, 0);

			// タイムアウト
			::GetCommTimeouts(m_hLPT, &cto);
			cto.WriteTotalTimeoutMultiplier = 1;
			cto.WriteTotalTimeoutConstant = 10;
			::SetCommTimeouts(m_hLPT, &cto);
		}
	}

	// バッファ初期化
	m_LPTQueue.Clear();

	// ログファイル又はLPTデバイスがあれば
	if (m_SendFile.IsValid() || (m_hLPT != INVALID_HANDLE_VALUE)) {
		// スレッドを立てる
		m_bLPTReq = FALSE;
		m_pLPT = AfxBeginThread(LPTThread, this);

		// スレッド成功なら、プリンタへ通知
		if (m_pLPT) {
			m_pPrinter->Connect(TRUE);
			return TRUE;
		}
	}

	// ポートを閉じる(内部でm_pPrinter->Connect(FALSE)を呼ぶ)
	CloseLPT();
	return TRUE;
}

//---------------------------------------------------------------------------
//
//	LPTログファイル
//
//---------------------------------------------------------------------------
void FASTCALL CPort::AppendLPT()
{
	TCHAR szDrive[_MAX_DRIVE];
	TCHAR szDir[_MAX_DIR];
	TCHAR szFile[_MAX_FNAME];
	TCHAR szExt[_MAX_EXT];
	Filepath path;

	ASSERT(this);

	// ログファイル指定が空であれば、必要ない
	if (_tcslen(m_SendLog) == 0) {
		return;
	}

	// 分割
	_tsplitpath(m_SendLog, szDrive, szDir, szFile, szExt);

	// フルパス指定でなければ相対パスを採用
	path.SetPath(m_SendLog);
	if ((_tcslen(szDrive) == 0) && (szDir[0] != _T('\\'))) {
		path.SetBaseDir();
	}

	// アペンドオープン
	m_SendFile.Open(path, Fileio::Append);
}

//---------------------------------------------------------------------------
//
//	LPTクローズ
//	※VMロック状態
//
//---------------------------------------------------------------------------
void FASTCALL CPort::CloseLPT()
{
	DWORD dwCompleted;

	ASSERT(this);

	// スレッドを閉じる
	if (m_pLPT) {
		// 終了要求を上げ
		m_bLPTReq = TRUE;

		// 終了待ち(無期限)
		::WaitForSingleObject(m_pLPT->m_hThread, INFINITE);

		m_pLPT = NULL;
	}

	// ハンドルを閉じる
	if (m_hLPT != INVALID_HANDLE_VALUE) {
		// 送信中か
		if (m_bLPTValid) {
			if (!GetOverlappedResult(m_hLPT, &m_LPTOver, &dwCompleted, FALSE)) {
				// まだ保留中
				CancelIo(m_hLPT);
			}
			// 送信していない
			m_bLPTValid = FALSE;
		}

		// クローズ
		::CloseHandle(m_hLPT);

		// ワーク
		m_hLPT = INVALID_HANDLE_VALUE;
		m_bLPTValid = FALSE;
		memset(&m_LPTOver, 0, sizeof(m_LPTOver));
	}

	// ログファイルクローズ
	if (m_SendFile.IsValid()) {
		m_SendFile.Close();
	}

	// プリンタに対し、切断を通知
	m_pPrinter->Connect(FALSE);
}

//---------------------------------------------------------------------------
//
//	LPT実行
//
//---------------------------------------------------------------------------
void FASTCALL CPort::RunLPT()
{
	ASSERT(this);

	// 終了フラグが上がるまで
	while (!m_bLPTReq) {
		// スリープ
		::Sleep(10);

		// バッファの内容を送信またはログ出力
		SendLPT();

		// プリンタからデータ取得
		RecvLPT();
	}
}

//---------------------------------------------------------------------------
//
//	プリンタデータ送信
//
//---------------------------------------------------------------------------
void FASTCALL CPort::SendLPT()
{
	BYTE Buffer[0x1000];
	DWORD dwNum;
	DWORD dwWritten;

	ASSERT(this);

	// 送信中か
	if (!m_bLPTValid) {
		// 手持ちのバッファが空なら、何もしない
		if (m_LPTQueue.IsEmpty()) {
			return;
		}

		// データを一括して受け取る
		dwNum = m_LPTQueue.Copy(Buffer);
		ASSERT(dwNum > 0);
		ASSERT(dwNum <= sizeof(Buffer));

		// バッファは有効。LPTデバイスがなければログ記録のみ
		if (m_hLPT == INVALID_HANDLE_VALUE) {
			// 送信ログファイルは必ず存在
			ASSERT(m_SendFile.IsValid());

			// ログファイルを書き込む
			m_SendFile.Write(Buffer, dwNum);

			// ここで進める
			m_LPTQueue.Discard(dwNum);
			return;
		}

		// バッファ有効かつLPTデバイスあり。同期書き込みを行う
		memset(&m_LPTOver, 0, sizeof(m_LPTOver));
		::WriteFile(m_hLPT, Buffer, dwNum, &dwWritten, &m_LPTOver);
		m_bLPTValid = TRUE;
		return;
	}

	// デバイスがなければここで完了
	if (m_hLPT == INVALID_HANDLE_VALUE) {
		return;
	}

	// 送信完了したか
	if (!GetOverlappedResult(m_hLPT, &m_LPTOver, &dwWritten, FALSE)) {
		return;
	}

	// 送信完了した。出来た部分だけ改めて受け取る(バッファ整合)
	m_LPTQueue.Copy(Buffer);
	m_LPTQueue.Discard(dwWritten);

	// 書けた分だけログファイルへ
	if (m_SendFile.IsValid()) {
		m_SendFile.Write(Buffer, dwWritten);
	}

	// 送信フラグ落とす
	m_bLPTValid = FALSE;
}

//---------------------------------------------------------------------------
//
//	プリンタデータ取得
//	※プリンタデバイス内で排他制御がかかる
//
//---------------------------------------------------------------------------
void FASTCALL CPort::RecvLPT()
{
	BYTE Buffer[0x1000];
	DWORD dwNum;
	DWORD dwFree;

	ASSERT(this);

	// バッファ余裕個数を取得
	dwFree = m_LPTQueue.GetFree();
	dwNum = 0;

	// バッファに余裕があるだけループ
	while (dwFree > 0) {
		// データ取得を試みる
		if (!m_pPrinter->GetData(&Buffer[dwNum])) {
			break;
		}

		// データ個数を増やす
		dwFree--;
		dwNum++;
	}

	// 追加したものがあればキューへ
	if (dwNum > 0) {
		m_LPTQueue.Insert(Buffer, dwNum);
	}
}

//---------------------------------------------------------------------------
//
//	LPT情報取得
//
//---------------------------------------------------------------------------
BOOL FASTCALL CPort::GetLPTInfo(LPTSTR lpszDevFile, DWORD *dwLogFile) const
{
	ASSERT(this);
	ASSERT(lpszDevFile);
	ASSERT(dwLogFile);

	// ファイルはヌル、ハンドルは無効
	_tcscpy(lpszDevFile, _T("  (None)"));
	*dwLogFile = (DWORD)INVALID_HANDLE_VALUE;

	// スレッドが立っていなければ、この時点でFALSEで帰る
	if (!m_pLPT) {
		return FALSE;
	}

	// デバイスファイルを設定
	if (m_nLPT > 0) {
		_stprintf(lpszDevFile, _T("\\\\.\\LPT%d"), m_nLPT);
	}

	// ログファイルを設定
	if (m_SendFile.IsValid()) {
		*dwLogFile = (DWORD)m_SendFile.GetHandle();
	}

	// スレッドは動いている
	return TRUE;
}

//---------------------------------------------------------------------------
//
//	LPTキュー情報取得
//
//---------------------------------------------------------------------------
void FASTCALL CPort::GetLPTQueue(CQueue::LPQUEUEINFO lpqi) const
{
	ASSERT(this);
	ASSERT(lpqi);

	m_LPTQueue.GetQueue(lpqi);
}

#endif	// _WIN32
