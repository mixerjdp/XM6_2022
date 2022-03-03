//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2005 �o�h�D(ytanaka@ipc-tokai.or.jp)
//	[ MFC �|�[�g ]
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
//	�|�[�g
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	COM�X���b�h�֐�
//
//---------------------------------------------------------------------------
static UINT COMThread(LPVOID pParam)
{
	CPort *pPort;

	// �p�����[�^���󂯎��
	pPort = (CPort*)pParam;
	ASSERT(pPort);

	// ���s
	pPort->RunCOM();

	// �I���R�[�h�������ăX���b�h���I��
	return 0;
}

//---------------------------------------------------------------------------
//
//	LPT�X���b�h�֐�
//
//---------------------------------------------------------------------------
static UINT LPTThread(LPVOID pParam)
{
	CPort *pPort;

	// �p�����[�^���󂯎��
	pPort = (CPort*)pParam;
	ASSERT(pPort);

	// ���s
	pPort->RunLPT();

	// �I���R�[�h�������ăX���b�h���I��
	return 0;
}

//---------------------------------------------------------------------------
//
//	�R���X�g���N�^
//
//---------------------------------------------------------------------------
CPort::CPort(CFrmWnd *pWnd) : CComponent(pWnd)
{
	// �R���|�[�l���g�p�����[�^
	m_dwID = MAKEID('P', 'O', 'R', 'T');
	m_strDesc = _T("Port Handler");

	// �V���A���|�[�g
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

	// �p�������|�[�g
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
//	������
//
//---------------------------------------------------------------------------
BOOL FASTCALL CPort::Init()
{
	ASSERT(this);

	// ��{�N���X
	if (!CComponent::Init()) {
		return FALSE;
	}

	// SCC�擾
	ASSERT(!m_pSCC);
	m_pSCC = (SCC*)::GetVM()->SearchDevice(MAKEID('S', 'C', 'C', ' '));
	ASSERT(m_pSCC);

	// �V���A���L���[������
	if (!m_TxQueue.Init(0x1000)) {
		return FALSE;
	}
	if (!m_RxQueue.Init(0x1000)) {
		return FALSE;
	}

	// �V���A���|�[�g
	OpenCOM();

	// �v�����^�擾
	ASSERT(!m_pPrinter);
	m_pPrinter = (Printer*)::GetVM()->SearchDevice(MAKEID('P', 'R', 'N', ' '));
	ASSERT(m_pPrinter);

	// �p�������L���[������
	if (!m_LPTQueue.Init(0x1000)) {
		return FALSE;
	}

	// �p�������|�[�g
	OpenLPT();

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	�N���[���A�b�v
//
//---------------------------------------------------------------------------
void FASTCALL CPort::Cleanup()
{
	ASSERT(this);

	// �p�������|�[�g
	CloseLPT();

	// �V���A���|�[�g
	CloseCOM();

	// ��{�N���X
	CComponent::Cleanup();
}

//---------------------------------------------------------------------------
//
//	�ݒ�K�p
//
//---------------------------------------------------------------------------
void FASTCALL CPort::ApplyCfg(const Config* pConfig)
{
	BOOL bCOM;
	BOOL bLPT;

	ASSERT(this);
	ASSERT(pConfig);

	// COM�t���O������
	bCOM = FALSE;

	// COM�|�[�g�ԍ�������Ă��邩
	if (pConfig->port_com != (int)m_nCOM) {
		m_nCOM = pConfig->port_com;
		bCOM = TRUE;

		// TrueKey�ƃ|�[�g�ԍ����d�Ȃ��Ă�����ATrueKey��D��
		if (pConfig->port_com == pConfig->tkey_com) {
			m_nCOM = 0;
		}
	}

	// ��M�t�@�C����������Ă��邩
	if (_tcscmp(pConfig->port_recvlog, m_RecvLog) != 0) {
		ASSERT(_tcslen(pConfig->port_recvlog) < _MAX_PATH);
		_tcscpy(m_RecvLog, pConfig->port_recvlog);
		bCOM = TRUE;
	}

	// �����t���O������Ă��邩
	if (pConfig->port_384 != m_bForce) {
		m_bForce = pConfig->port_384;
		bCOM = TRUE;
	}

	// �ύX�_�������
	if (bCOM) {
		// �N���[�Y���������邽�߁A��x���b�N���Ă���VM�𗣂�
		::UnlockVM();
		CloseCOM();
		OpenCOM();
		::LockVM();
	}

	// LPT�t���O������
	bLPT = FALSE;

	// LPT�|�[�g�ԍ�������Ă��邩
	if (pConfig->port_lpt != (int)m_nLPT) {
		m_nLPT = pConfig->port_lpt;
		bLPT = TRUE;
	}

	// ���M�t�@�C����������Ă��邩
	if (strcmp(pConfig->port_sendlog, m_SendLog) != 0) {
		ASSERT(strlen(pConfig->port_sendlog) < sizeof(m_SendLog));
		strcpy(m_SendLog, pConfig->port_sendlog);
		bLPT = TRUE;
	}

	// �ύX�_�������
	if (bLPT) {
		// �N���[�Y���������邽�߁A��x���b�N���Ă���VM�𗣂�
		::UnlockVM();
		CloseLPT();
		OpenLPT();
		::LockVM();
	}
}

//===========================================================================
//
//	�V���A���|�[�g
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	COM�I�[�v��
//	��VM���b�N���
//
//---------------------------------------------------------------------------
BOOL FASTCALL CPort::OpenCOM()
{
	CString strFile;
	DCB dcb;

	ASSERT(this);

	// �|�[�g��0�Ȃ�A���蓖�ĂȂ�
	if (m_nCOM == 0) {
		m_hCOM = INVALID_HANDLE_VALUE;
		return TRUE;
	}

	// �t�@�C�������쐬
	strFile.Format(_T("\\\\.\\COM%d"), m_nCOM);

	// �I�[�v��
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

	// �o�b�t�@�T�C�Y
	::SetupComm(m_hCOM, 0x1000, 0x1000);

	// ����M���N���A
	m_TxQueue.Clear();
	m_RxQueue.Clear();
	::PurgeComm(m_hCOM, PURGE_TXCLEAR);
	::PurgeComm(m_hCOM, PURGE_RXCLEAR);

	// �M�����̏����ݒ�
	OnErr();
	OnCTSDSR();
	::GetCommState(m_hCOM, &dcb);
	::LockVM();
	AdjustCOM(&dcb);
	SignalCOM();
	::UnlockVM();
	CtrlCOM();

	// ���O�t�@�C���̍쐬(�A�y���h)
	AppendCOM();

	// �X���b�h�𗧂Ă�
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
//	COM���O�t�@�C��
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

	// ���O�t�@�C���w�肪��ł���΁A�K�v�Ȃ�
	if (_tcslen(m_RecvLog) == 0) {
		return;
	}

	// ����
	_tsplitpath(m_RecvLog, szDrive, szDir, szFile, szExt);

	// �t���p�X�w��łȂ���Α��΃p�X���̗p
	path.SetPath(m_RecvLog);
	if ((_tcslen(szDrive) == 0) && (szDir[0] != _T('\\'))) {
		path.SetBaseDir();
	}

	// �A�y���h�I�[�v��
	m_RecvFile.Open(path, Fileio::Append);
}

//---------------------------------------------------------------------------
//
//	COM�N���[�Y
//	��VM���b�N���
//
//---------------------------------------------------------------------------
void FASTCALL CPort::CloseCOM()
{
	DWORD dwCompleted;

	ASSERT(this);

	// �X���b�h�����
	if (m_pCOM) {
		// �I���v�����グ
		m_bCOMReq = TRUE;

		// �I���҂�(������)
		::WaitForSingleObject(m_pCOM->m_hThread, INFINITE);

		m_pCOM = NULL;
	}

	// �n���h�������
	if (m_hCOM != INVALID_HANDLE_VALUE) {
		// �o�b�t�@�p�[�W
		m_TxQueue.Clear();
		m_RxQueue.Clear();
		::PurgeComm(m_hCOM, PURGE_TXCLEAR);
		::PurgeComm(m_hCOM, PURGE_RXCLEAR);

		// ���M����
		if (m_bTxValid) {
			// ���MI/O���������Ă��邩
			if (!GetOverlappedResult(m_hCOM, &m_TxOver, &dwCompleted, FALSE)) {
				// �܂��ۗ����Ȃ̂ŁA�L�����Z��
				CancelIo(m_hCOM);
			}
			m_bTxValid = FALSE;
		}

		// ��MI/O���������Ă��邩
		if (!GetOverlappedResult(m_hCOM, &m_RxOver, &dwCompleted, FALSE)) {
			// �܂��ۗ����Ȃ̂ŁA�L�����Z��
			CancelIo(m_hCOM);
		}

		// �N���[�Y
		::CloseHandle(m_hCOM);
		m_hCOM = INVALID_HANDLE_VALUE;
	}

	// ���O�t�@�C���N���[�Y
	m_RecvFile.Close();

	// SCC�ɑ΂��A���M�E�F�C�g�����Ȃ�
	m_pSCC->WaitTx(0, FALSE);
	m_pSCC->SetCTS(0, FALSE);
	m_pSCC->SetDCD(0, FALSE);
}

//---------------------------------------------------------------------------
//
//	COM���s
//
//---------------------------------------------------------------------------
void FASTCALL CPort::RunCOM()
{
	COMMTIMEOUTS cto;
	DCB dcb;

	ASSERT(this);
	ASSERT(m_hCOM);

	// �C�x���g�}�X�N
	::SetCommMask(m_hCOM, 0);

	// �^�C���A�E�g
	::GetCommTimeouts(m_hCOM, &cto);
	cto.ReadIntervalTimeout = 1;
	cto.ReadTotalTimeoutMultiplier = 0;
	cto.ReadTotalTimeoutConstant = 10000;
	cto.WriteTotalTimeoutMultiplier = 0;
	cto.WriteTotalTimeoutConstant = 10000;
	::SetCommTimeouts(m_hCOM, &cto);

	// ��M�҂�
	memset(&m_RxOver, 0, sizeof(m_RxOver));
	::ReadFile(m_hCOM, m_RxBuf, sizeof(m_RxBuf), NULL, &m_RxOver);

	// ���M�Ȃ�
	m_bTxValid = FALSE;
	memset(&m_TxOver, 0, sizeof(m_TxOver));

	// �I���t���O���オ��܂�
	while (!m_bCOMReq) {
		// �X���[�v
		::Sleep(5);

		// CTS, DSR
		OnCTSDSR();

		// �G���[
		OnErr();

		// ��M
		OnRx();

		// Win32�������擾
		::GetCommState(m_hCOM, &dcb);

		// VM���b�N
		::LockVM();

		// ��������
		AdjustCOM(&dcb);
		SignalCOM();
		BufCOM();

		// VM�A�����b�N
		::UnlockVM();

		// �M��������
		CtrlCOM();

		// ���M
		OnTx();
	}
}

//---------------------------------------------------------------------------
//
//	�p�����[�^���킹
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

	// �ύX�L�����I�t
	bFlag = FALSE;

	// SCC�������擾
	scc = m_pSCC->GetWork();
	p = &scc->ch[0];

	// Win32�������擾
	memcpy(&dcb, pDCB, sizeof(dcb));

	// �{�[���[�g
	dwValue = CBR_300;
	for (i=0;; i++) {
		// �e�[�u���I�[�ł����300bps
		if (dwTable[i] == 0) {
			break;
		}
		// �㉺5%�̕��Ń}�b�`������
		if (MatchCOM(p->baudrate, dwTable[i])) {
			dwValue = dwTable[i];
			break;
		}
	}

	// ����38400bps
	if (m_bForce) {
		dwValue = CBR_38400;
	}

	// ��r
	if (dcb.BaudRate != dwValue) {
		dcb.BaudRate = dwValue;
		bFlag = TRUE;
	}

	// �o�C�i�����[�h
	dcb.fBinary = 0;

	// �p���e�B�L��
	if (p->parity == 0) {
		dwValue = 0;
	}
	else {
		dwValue = 1;
	}
	if (dcb.fParity != dwValue) {
		dcb.fParity = dwValue;
	}

	// CTS�t���[
	if (dcb.fOutxCtsFlow != 0) {
		dcb.fOutxCtsFlow = 0;
		bFlag = TRUE;
	}

	// DSR�t���[
	if (dcb.fOutxDsrFlow != 0) {
		dcb.fOutxDsrFlow = 0;
		bFlag = TRUE;
	}

	// DTR����
	if (dcb.fDtrControl != DTR_CONTROL_ENABLE) {
		dcb.fDtrControl = DTR_CONTROL_ENABLE;
		bFlag = TRUE;
	}

	// DSR�Z���X
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

	// �f�[�^�r�b�g��
	if (dcb.ByteSize != (BYTE)p->rxbit) {
		if (p->rxbit == p->txbit) {
			if (p->rxbit >= 5) {
				dcb.ByteSize = (BYTE)p->rxbit;
				bFlag = TRUE;
			}
		}
	}

	// �p���e�B
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

	// �X�g�b�v�r�b�g
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

	// �قȂ��Ă����
	if (bFlag) {
		// �ݒ�
		::SetCommState(m_hCOM, &dcb);

		// �o�b�t�@�N���A
		m_TxQueue.Clear();
		m_RxQueue.Clear();

		// Win32���p�[�W
		::PurgeComm(m_hCOM, PURGE_TXCLEAR);
		::PurgeComm(m_hCOM, PURGE_RXCLEAR);
	}
}

//---------------------------------------------------------------------------
//
//	�{�[���[�g�}�b�`
//
//---------------------------------------------------------------------------
BOOL FASTCALL CPort::MatchCOM(DWORD dwSCC, DWORD dwBase)
{
	DWORD dwOffset;

	ASSERT(this);

	// �I���W�i���ɑ΂���5%�̃I�t�Z�b�g
	dwOffset = (dwBase * 5);
	dwOffset /= 100;

	// �͈͂Ɏ��܂��Ă����OK
	if ((dwBase - dwOffset) <= dwSCC) {
		if (dwSCC <= (dwBase + dwOffset)) {
			return TRUE;
		}
	}

	// �Ⴄ
	return FALSE;
}

//---------------------------------------------------------------------------
//
//	�M�������킹
//
//---------------------------------------------------------------------------
void FASTCALL CPort::SignalCOM()
{
	ASSERT(this);
	ASSERT(m_pSCC);

	// VM�֒ʒm(�G���[)
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

	// VM�֒ʒm(�M����)
	m_pSCC->SetCTS(0, m_bCTS);
	m_pSCC->SetDCD(0, m_bDSR);

	// VM����ʒm(�M����)
	m_bBreak = m_pSCC->GetBreak(0);
	m_bRTS = m_pSCC->GetRTS(0);
	m_bDTR = m_pSCC->GetDTR(0);
}

//---------------------------------------------------------------------------
//
//	�o�b�t�@���킹
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

	// ��M�f�[�^���ꊇ�擾
	dwRx = m_RxQueue.Get(RxBuf);

	// �L���ȃf�[�^�͂��ׂ�SCC�֑���
	for (i=0; i<dwRx; i++) {
		m_pSCC->Send(0, RxBuf[i]);
	}

	// SCC���o�b�t�@�̏�Ԃ����āA���M�E�F�C�g����
	if (m_pSCC->IsTxFull(0)) {
		// �o�b�t�@����t�Ȃ̂ŁA����ȏ㑗�M�����Ȃ�
		m_pSCC->WaitTx(0, TRUE);
	}
	else {
		// ���M��
		m_pSCC->WaitTx(0, FALSE);
	}

	// ���M�f�[�^������΁A�󂯎��
	while (!m_pSCC->IsTxEmpty(0)) {
		// �o�b�t�@�𒴂��Ȃ��͈͂ɉ�������
		if (m_TxQueue.GetFree() == 0) {
			break;
		}

		TxData = (BYTE)m_pSCC->Receive(0);
		m_TxQueue.Insert(&TxData, 1);
	}
}

//---------------------------------------------------------------------------
//
//	�M��������
//
//---------------------------------------------------------------------------
void FASTCALL CPort::CtrlCOM()
{
	ASSERT(this);
	ASSERT(m_hCOM);

	// �u���[�N
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

	// ��Ԏ擾
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
//	��M�G���[
//
//---------------------------------------------------------------------------
void FASTCALL CPort::OnErr()
{
	ASSERT(this);
	ASSERT(m_hCOM);

	// �G���[���N���A���A�����ɏ����L�^
	::ClearCommError(m_hCOM, &m_dwErr, NULL);
}

//---------------------------------------------------------------------------
//
//	��M�T�u
//
//---------------------------------------------------------------------------
void FASTCALL CPort::OnRx()
{
	DWORD dwReceived;

	ASSERT(this);
	ASSERT(m_hCOM);

	// ��M�������Ă��Ȃ���Ή������Ȃ�
	dwReceived = 0;
	if (!GetOverlappedResult(m_hCOM, &m_RxOver, &dwReceived, FALSE)) {
		return;
	}

	// ��M���L���ł����
	if (dwReceived > 0) {
		// ��M���O�t�@�C�����L���ł���΁A��������
		if (m_RecvFile.IsValid()) {
			m_RecvFile.Write(m_RxBuf, dwReceived);
		}

		// �L���[�ɑ}��
		m_RxQueue.Insert(m_RxBuf, dwReceived);
	}

	// ���̎�M
	memset(&m_RxOver, 0, sizeof(m_RxOver));
	::ReadFile(m_hCOM, m_RxBuf, sizeof(m_RxBuf), NULL, &m_RxOver);
}

//---------------------------------------------------------------------------
//
//	���M�T�u
//
//---------------------------------------------------------------------------
void FASTCALL CPort::OnTx()
{
	DWORD dwSize;
	DWORD dwSent;

	ASSERT(this);
	ASSERT(m_hCOM);

	// �O��̑��M���I����Ă��Ȃ���΁A���^�[��
	if (m_bTxValid) {
		if (!GetOverlappedResult(m_hCOM, &m_TxOver, &dwSent, FALSE)) {
			return;
		}

		// ���M�ł����������A���M�o�b�t�@����i�߂�
		if (dwSent > 0) {
			m_TxQueue.Discard(dwSent);
		}

		// ���󑗐M�Ȃ�
		m_bTxValid = FALSE;
	}

	// ���M������̂��Ȃ���΁A���^�[��
	if (m_TxQueue.IsEmpty()) {
		return;
	}

	// ���M�L���[����擾(�|�C���^�͐i�߂Ȃ�)
	dwSize = m_TxQueue.Copy(m_TxBuf);

	// ���M�J�n
	memset(&m_TxOver, 0, sizeof(m_TxOver));
	m_bTxValid = TRUE;
	::WriteFile(m_hCOM, m_TxBuf, dwSize, NULL, &m_TxOver);
}

//---------------------------------------------------------------------------
//
//	COM���擾
//
//---------------------------------------------------------------------------
BOOL FASTCALL CPort::GetCOMInfo(LPTSTR lpszDevFile, DWORD *dwLogFile) const
{
	ASSERT(this);
	ASSERT(lpszDevFile);
	ASSERT(dwLogFile);

	// �t�@�C���̓k���A�n���h���͖���
	_tcscpy(lpszDevFile, _T("  (None)"));
	*dwLogFile = (DWORD)INVALID_HANDLE_VALUE;

	// �X���b�h�������Ă��Ȃ���΁A���̎��_��FALSE�ŋA��
	if (!m_pCOM) {
		return FALSE;
	}

	// �f�o�C�X�t�@�C����ݒ�
	if (m_nCOM > 0) {
		_stprintf(lpszDevFile, _T("\\\\.\\COM%d"), m_nCOM);
	}

	// ���O�t�@�C����ݒ�
	if (m_RecvFile.IsValid()) {
		*dwLogFile = (DWORD)m_RecvFile.GetHandle();
	}

	// �X���b�h�͓����Ă���
	return TRUE;
}

//---------------------------------------------------------------------------
//
//	���M�L���[���擾
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
//	��M�L���[���擾
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
//	�p�������|�[�g
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	LPT�I�[�v��
//	��VM���b�N���
//
//---------------------------------------------------------------------------
BOOL FASTCALL CPort::OpenLPT()
{
	COMMTIMEOUTS cto;
	CString strFile;

	ASSERT(this);

	// �n���h��������
	m_hLPT = INVALID_HANDLE_VALUE;
	m_pLPT = NULL;
	m_bLPTValid = FALSE;

	// ���O�t�@�C���̍쐬(�A�y���h)
	AppendLPT();

	// �|�[�g��0�łȂ��ꍇ�ALPT�f�o�C�X�֏o��
	if (m_nLPT != 0) {
		// �t�@�C�������쐬
		strFile.Format(_T("\\\\.\\LPT%d"), m_nLPT);

		// �I�[�v��
		m_hLPT = ::CreateFile(  strFile,
								GENERIC_WRITE,
								0,
								NULL,
								OPEN_EXISTING,
								FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
								0);

		// �I�[�v���ł��Ă�����ݒ�
		if (m_hLPT != INVALID_HANDLE_VALUE) {
			// �o�b�t�@�T�C�Y
			::SetupComm(m_hLPT, 0x400, 0x1000);

			// �C�x���g�}�X�N
			::SetCommMask(m_hLPT, 0);

			// �^�C���A�E�g
			::GetCommTimeouts(m_hLPT, &cto);
			cto.WriteTotalTimeoutMultiplier = 1;
			cto.WriteTotalTimeoutConstant = 10;
			::SetCommTimeouts(m_hLPT, &cto);
		}
	}

	// �o�b�t�@������
	m_LPTQueue.Clear();

	// ���O�t�@�C������LPT�f�o�C�X�������
	if (m_SendFile.IsValid() || (m_hLPT != INVALID_HANDLE_VALUE)) {
		// �X���b�h�𗧂Ă�
		m_bLPTReq = FALSE;
		m_pLPT = AfxBeginThread(LPTThread, this);

		// �X���b�h�����Ȃ�A�v�����^�֒ʒm
		if (m_pLPT) {
			m_pPrinter->Connect(TRUE);
			return TRUE;
		}
	}

	// �|�[�g�����(������m_pPrinter->Connect(FALSE)���Ă�)
	CloseLPT();
	return TRUE;
}

//---------------------------------------------------------------------------
//
//	LPT���O�t�@�C��
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

	// ���O�t�@�C���w�肪��ł���΁A�K�v�Ȃ�
	if (_tcslen(m_SendLog) == 0) {
		return;
	}

	// ����
	_tsplitpath(m_SendLog, szDrive, szDir, szFile, szExt);

	// �t���p�X�w��łȂ���Α��΃p�X���̗p
	path.SetPath(m_SendLog);
	if ((_tcslen(szDrive) == 0) && (szDir[0] != _T('\\'))) {
		path.SetBaseDir();
	}

	// �A�y���h�I�[�v��
	m_SendFile.Open(path, Fileio::Append);
}

//---------------------------------------------------------------------------
//
//	LPT�N���[�Y
//	��VM���b�N���
//
//---------------------------------------------------------------------------
void FASTCALL CPort::CloseLPT()
{
	DWORD dwCompleted;

	ASSERT(this);

	// �X���b�h�����
	if (m_pLPT) {
		// �I���v�����グ
		m_bLPTReq = TRUE;

		// �I���҂�(������)
		::WaitForSingleObject(m_pLPT->m_hThread, INFINITE);

		m_pLPT = NULL;
	}

	// �n���h�������
	if (m_hLPT != INVALID_HANDLE_VALUE) {
		// ���M����
		if (m_bLPTValid) {
			if (!GetOverlappedResult(m_hLPT, &m_LPTOver, &dwCompleted, FALSE)) {
				// �܂��ۗ���
				CancelIo(m_hLPT);
			}
			// ���M���Ă��Ȃ�
			m_bLPTValid = FALSE;
		}

		// �N���[�Y
		::CloseHandle(m_hLPT);

		// ���[�N
		m_hLPT = INVALID_HANDLE_VALUE;
		m_bLPTValid = FALSE;
		memset(&m_LPTOver, 0, sizeof(m_LPTOver));
	}

	// ���O�t�@�C���N���[�Y
	if (m_SendFile.IsValid()) {
		m_SendFile.Close();
	}

	// �v�����^�ɑ΂��A�ؒf��ʒm
	m_pPrinter->Connect(FALSE);
}

//---------------------------------------------------------------------------
//
//	LPT���s
//
//---------------------------------------------------------------------------
void FASTCALL CPort::RunLPT()
{
	ASSERT(this);

	// �I���t���O���オ��܂�
	while (!m_bLPTReq) {
		// �X���[�v
		::Sleep(10);

		// �o�b�t�@�̓��e�𑗐M�܂��̓��O�o��
		SendLPT();

		// �v�����^����f�[�^�擾
		RecvLPT();
	}
}

//---------------------------------------------------------------------------
//
//	�v�����^�f�[�^���M
//
//---------------------------------------------------------------------------
void FASTCALL CPort::SendLPT()
{
	BYTE Buffer[0x1000];
	DWORD dwNum;
	DWORD dwWritten;

	ASSERT(this);

	// ���M����
	if (!m_bLPTValid) {
		// �莝���̃o�b�t�@����Ȃ�A�������Ȃ�
		if (m_LPTQueue.IsEmpty()) {
			return;
		}

		// �f�[�^���ꊇ���Ď󂯎��
		dwNum = m_LPTQueue.Copy(Buffer);
		ASSERT(dwNum > 0);
		ASSERT(dwNum <= sizeof(Buffer));

		// �o�b�t�@�͗L���BLPT�f�o�C�X���Ȃ���΃��O�L�^�̂�
		if (m_hLPT == INVALID_HANDLE_VALUE) {
			// ���M���O�t�@�C���͕K������
			ASSERT(m_SendFile.IsValid());

			// ���O�t�@�C������������
			m_SendFile.Write(Buffer, dwNum);

			// �����Ői�߂�
			m_LPTQueue.Discard(dwNum);
			return;
		}

		// �o�b�t�@�L������LPT�f�o�C�X����B�����������݂��s��
		memset(&m_LPTOver, 0, sizeof(m_LPTOver));
		::WriteFile(m_hLPT, Buffer, dwNum, &dwWritten, &m_LPTOver);
		m_bLPTValid = TRUE;
		return;
	}

	// �f�o�C�X���Ȃ���΂����Ŋ���
	if (m_hLPT == INVALID_HANDLE_VALUE) {
		return;
	}

	// ���M����������
	if (!GetOverlappedResult(m_hLPT, &m_LPTOver, &dwWritten, FALSE)) {
		return;
	}

	// ���M���������B�o���������������߂Ď󂯎��(�o�b�t�@����)
	m_LPTQueue.Copy(Buffer);
	m_LPTQueue.Discard(dwWritten);

	// ���������������O�t�@�C����
	if (m_SendFile.IsValid()) {
		m_SendFile.Write(Buffer, dwWritten);
	}

	// ���M�t���O���Ƃ�
	m_bLPTValid = FALSE;
}

//---------------------------------------------------------------------------
//
//	�v�����^�f�[�^�擾
//	���v�����^�f�o�C�X���Ŕr�����䂪������
//
//---------------------------------------------------------------------------
void FASTCALL CPort::RecvLPT()
{
	BYTE Buffer[0x1000];
	DWORD dwNum;
	DWORD dwFree;

	ASSERT(this);

	// �o�b�t�@�]�T�����擾
	dwFree = m_LPTQueue.GetFree();
	dwNum = 0;

	// �o�b�t�@�ɗ]�T�����邾�����[�v
	while (dwFree > 0) {
		// �f�[�^�擾�����݂�
		if (!m_pPrinter->GetData(&Buffer[dwNum])) {
			break;
		}

		// �f�[�^���𑝂₷
		dwFree--;
		dwNum++;
	}

	// �ǉ��������̂�����΃L���[��
	if (dwNum > 0) {
		m_LPTQueue.Insert(Buffer, dwNum);
	}
}

//---------------------------------------------------------------------------
//
//	LPT���擾
//
//---------------------------------------------------------------------------
BOOL FASTCALL CPort::GetLPTInfo(LPTSTR lpszDevFile, DWORD *dwLogFile) const
{
	ASSERT(this);
	ASSERT(lpszDevFile);
	ASSERT(dwLogFile);

	// �t�@�C���̓k���A�n���h���͖���
	_tcscpy(lpszDevFile, _T("  (None)"));
	*dwLogFile = (DWORD)INVALID_HANDLE_VALUE;

	// �X���b�h�������Ă��Ȃ���΁A���̎��_��FALSE�ŋA��
	if (!m_pLPT) {
		return FALSE;
	}

	// �f�o�C�X�t�@�C����ݒ�
	if (m_nLPT > 0) {
		_stprintf(lpszDevFile, _T("\\\\.\\LPT%d"), m_nLPT);
	}

	// ���O�t�@�C����ݒ�
	if (m_SendFile.IsValid()) {
		*dwLogFile = (DWORD)m_SendFile.GetHandle();
	}

	// �X���b�h�͓����Ă���
	return TRUE;
}

//---------------------------------------------------------------------------
//
//	LPT�L���[���擾
//
//---------------------------------------------------------------------------
void FASTCALL CPort::GetLPTQueue(CQueue::LPQUEUEINFO lpqi) const
{
	ASSERT(this);
	ASSERT(lpqi);

	m_LPTQueue.GetQueue(lpqi);
}

#endif	// _WIN32
