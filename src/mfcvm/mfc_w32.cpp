//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2005 �o�h�D(ytanaka@ipc-tokai.or.jp)
//	[ MFC �T�u�E�B���h�E(Win32) ]
//
//---------------------------------------------------------------------------

#if defined(_WIN32)

#include "os.h"
#include "xm6.h"
#include "vm.h"
#include "schedule.h"
#include "opmif.h"
#include "adpcm.h"
#include "render.h"
#include "memory.h"
#include "keyboard.h"
#include "midi.h"
#include "mfc_sub.h"
#include "mfc_res.h"
#include "mfc_frm.h"
#include "mfc_com.h"
#include "mfc_snd.h"
#include "mfc_inp.h"
#include "mfc_draw.h"
#include "mfc_port.h"
#include "mfc_que.h"
#include "mfc_midi.h"
#include "mfc_sch.h"
#include "mfc_w32.h"

//===========================================================================
//
//	�R���|�[�l���g�E�B���h�E
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	�R���X�g���N�^
//
//---------------------------------------------------------------------------
CComponentWnd::CComponentWnd()
{
	CFrmWnd *pFrmWnd;

	// �E�B���h�E�p�����[�^��`
	m_dwID = MAKEID('C', 'O', 'M', 'P');
	::GetMsg(IDS_SWND_COMPONENT, m_strCaption);
	m_nWidth = 58;
	m_nHeight = 10;

	// �ŏ��̃R���|�[�l���g���擾
	pFrmWnd = (CFrmWnd*)AfxGetApp()->m_pMainWnd;
	ASSERT(pFrmWnd);
	m_pComponent = pFrmWnd->GetFirstComponent();
	ASSERT(m_pComponent);
}

//---------------------------------------------------------------------------
//
//	�Z�b�g�A�b�v
//
//---------------------------------------------------------------------------
void FASTCALL CComponentWnd::Setup()
{
	DWORD dwID;
	int nComponent;
	CString strText;
	CComponent *pComponent;

	ASSERT(this);

	// �N���A
	Clear();

	// �K�C�h
	SetString(0, 0, _T("No."));
	SetString(5, 0, _T("ID"));
	SetString(10, 0, _T("Flag"));
	SetString(17, 0, _T("Object"));
	SetString(27, 0, _T("Prev"));
	SetString(36, 0, _T("Next"));
	SetString(45, 0, _T("Description"));

	// ������
	pComponent = m_pComponent;
	nComponent = 1;

	// ���[�v
	while (pComponent) {
		// No.
		strText.Format(_T("%2d"), nComponent);
		SetString(0, nComponent, strText);

		// ID
		dwID = pComponent->GetID();
		strText.Format(_T("%c%c%c%c"),
			(dwID >> 24) & 0xff,
			(dwID >> 16) & 0xff,
			(dwID >> 8) & 0xff,
			dwID & 0xff);
		SetString(4, nComponent, strText);

		// �C�l�[�u��
		if (pComponent->IsEnable()) {
			SetString(9, nComponent,_T("Enable"));
		}

		// �|�C���^
		strText.Format(_T("%08X"), pComponent);
		SetString(16, nComponent, strText);

		// �O�̃|�C���^
		strText.Format(_T("%08X"), pComponent->GetPrevComponent());
		SetString(25, nComponent, strText);

		// ���̃|�C���^
		strText.Format(_T("%08X"), pComponent->GetNextComponent());
		SetString(34, nComponent, strText);

		// ����
		pComponent->GetDesc(strText);
		SetString(43, nComponent, strText);

		// ����
		pComponent = pComponent->GetNextComponent();
		nComponent++;
	}
}

//===========================================================================
//
//	OS���E�B���h�E
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	�R���X�g���N�^
//
//---------------------------------------------------------------------------
COSInfoWnd::COSInfoWnd()
{
	// �E�B���h�E�p�����[�^��`
	m_dwID = MAKEID('O', 'S', 'I', 'N');
	::GetMsg(IDS_SWND_OSINFO, m_strCaption);
	m_nWidth = 30;
	m_nHeight = 13;
}

//---------------------------------------------------------------------------
//
//	�Z�b�g�A�b�v
//
//---------------------------------------------------------------------------
void FASTCALL COSInfoWnd::Setup()
{
	int x;
	int y;
	CString strText;
	OSVERSIONINFO ovi;
	SYSTEM_INFO si;

	ASSERT(this);

	// �N���A
	Clear();
	x = 25;
	y = 0;

	// OS�o�[�W�����擾
	memset(&ovi, 0, sizeof(ovi));
	ovi.dwOSVersionInfoSize = sizeof(ovi);
	VERIFY(GetVersionEx(&ovi));

	// �����Z�b�g
	SetString(0, y, _T("MajorVersion"));
	strText.Format(_T("%5u"), ovi.dwMajorVersion);
	SetString(x, y, strText);
	y++;

	SetString(0, y, _T("MinorVersion"));
	strText.Format(_T("%5u"), ovi.dwMinorVersion);
	SetString(x, y, strText);
	y++;

	SetString(0, y, _T("BuildNumber"));
	strText.Format(_T("%5u"), LOWORD(ovi.dwBuildNumber));
	SetString(x, y, strText);
	y++;

	SetString(0, y, _T("Platform"));
	switch (ovi.dwPlatformId) {
		case VER_PLATFORM_WIN32_WINDOWS:
			strText = _T("Windows 9x");
			break;
		case VER_PLATFORM_WIN32_NT:
			strText = _T("Windows NT");
			break;
		default:
			strText = _T(" (Unknown)");
	}
	SetString(x - 5, y, strText);
	y++;

	SetString(0, y, _T("CSDVersion"));
	strText = ovi.szCSDVersion;
	SetString(x + 5 - strText.GetLength(), y, strText);
	y++;

	// ������
	SetString(0, y, _T("System LangID"));
	strText.Format(_T("0x%04X"), ::GetSystemDefaultLangID());
	SetString(x - 1, y, strText);
	y++;

	// ������
	SetString(0, y, _T("User LangID"));
	strText.Format(_T("0x%04X"), ::GetUserDefaultLangID());
	SetString(x - 1, y, strText);
	y++;

	// �V�X�e�����擾
	memset(&si, 0, sizeof(si));
	::GetSystemInfo(&si);

	// �����Z�b�g
	SetString(0, y, _T("Number of Processor"));
	strText.Format(_T("%5u"), si.dwNumberOfProcessors);
	SetString(x, y, strText);
	y++;

	SetString(0, y, _T("Processor Family"));
	strText.Format(_T("%5u"), si.wProcessorLevel);
	SetString(x, y, strText);
	y++;

	SetString(0, y, _T("Processor Model"));
	strText.Format(_T("%5u"), HIBYTE(si.wProcessorRevision));
	SetString(x, y, strText);
	y++;

	SetString(0, y, _T("Processor Revision"));
	strText.Format(_T("%5u"), LOBYTE(si.wProcessorRevision));
	SetString(x, y, strText);
	y++;

	// MMX
	SetString(0, y, _T("MMX  Support"));
	if (::IsMMX()) {
		SetString(x + 2, y, _T("Yes"));
	}
	else {
		SetString(x + 3, y, _T("No"));
	}
	y++;

	// CMOV
	SetString(0, y, _T("CMOV Support"));
	if (::IsCMOV()) {
		SetString(x + 2, y, _T("Yes"));
	}
	else {
		SetString(x + 3, y, _T("No"));
	}
}

//===========================================================================
//
//	�T�E���h�E�B���h�E
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	�R���X�g���N�^
//
//---------------------------------------------------------------------------
CSoundWnd::CSoundWnd()
{
	CFrmWnd *pFrmWnd;

	// �E�B���h�E�p�����[�^��`
	m_dwID = MAKEID('S', 'N', 'D', ' ');
	::GetMsg(IDS_SWND_SOUND, m_strCaption);
	m_nWidth = 25;
	m_nHeight = 14;

	// �X�P�W���[�����擾
	m_pScheduler = (Scheduler*)::GetVM()->SearchDevice(MAKEID('S', 'C', 'H', 'E'));
	ASSERT(m_pScheduler);

	// OPM���擾
	m_pOPMIF = (OPMIF*)::GetVM()->SearchDevice(MAKEID('O', 'P', 'M', ' '));
	ASSERT(m_pOPMIF);

	// ADPCM���擾
	m_pADPCM = (ADPCM*)::GetVM()->SearchDevice(MAKEID('A', 'P', 'C', 'M'));
	ASSERT(m_pADPCM);

	// �T�E���h�R���|�[�l���g���擾
	pFrmWnd = (CFrmWnd*)AfxGetApp()->m_pMainWnd;
	ASSERT(pFrmWnd);
	m_pSound = pFrmWnd->GetSound();
	ASSERT(m_pSound);
}

//---------------------------------------------------------------------------
//
//	�Z�b�g�A�b�v
//
//---------------------------------------------------------------------------
void FASTCALL CSoundWnd::Setup()
{
	int x;
	int y;
	CString strText;
	OPMIF::opmbuf_t opmbuf;
	ADPCM::adpcm_t adpcm;
	DWORD dwTime;

	ASSERT(this);
	ASSERT(m_pScheduler);
	ASSERT(m_pOPMIF);
	ASSERT(m_pSound);

	// �N���A
	Clear();
	x = 20;
	y = 0;

	// �T�E���h����
	dwTime = m_pScheduler->GetSoundTime();
	SetString(0, y, _T("Sound Time"));
	strText.Format(_T("%3d.%03dms"), dwTime / 2000, (dwTime % 2000) / 2);
	SetString(x - 4, y, strText);
	y++;

	// �o�b�t�@�����܂Ƃ߂ē���
	m_pOPMIF->GetBufInfo(&opmbuf);

	// �o�b�t�@����\��
	SetString(0, y, _T("Buffer Max"));
	strText.Format(_T("%5u"), opmbuf.max);
	SetString(x, y, strText);
	y++;
	SetString(0, y, _T("Buffer Num"));
	strText.Format(_T("%5u"), opmbuf.num);
	SetString(x, y, strText);
	y++;
	SetString(0, y, _T("Buffer Read"));
	strText.Format(_T("%5u"), opmbuf.read);
	SetString(x, y, strText);
	y++;
	SetString(0, y, _T("Buffer Write"));
	strText.Format(_T("%5u"), opmbuf.write);
	SetString(x, y, strText);
	y++;
	SetString(0, y, _T("Buffer Samples"));
	strText.Format(_T("%5u"), opmbuf.samples);
	SetString(x, y, strText);
	y++;
	SetString(0, y, _T("Buffer Rate"));
	strText.Format(_T("%5u"), opmbuf.rate);
	SetString(x, y, strText);
	y++;
	SetString(0, y, _T("Buffer Underrun"));
	strText.Format(_T("%5u"), opmbuf.under);
	SetString(x, y, strText);
	y++;
	SetString(0, y, _T("Buffer Overrun"));
	strText.Format(_T("%5u"), opmbuf.over);
	SetString(x, y, strText);
	y++;

	// ADPCM�̏��𓾂�
	m_pADPCM->GetADPCM(&adpcm);

	SetString(0, y, _T("ADPCM Sample"));
	strText.Format(_T("%06X"), adpcm.sample & 0xffffff);
	SetString(x - 1, y, strText);
	y++;

	SetString(0, y, _T("ADPCM Output"));
	strText.Format(_T("%06X"), adpcm.out & 0xffffff);
	SetString(x - 1, y, strText);
	y++;

	SetString(0, y, _T("ADPCM Num"));
	strText.Format(_T("%5u"), adpcm.number);
	SetString(x, y, strText);
	y++;

	SetString(0, y, _T("ADPCM Read"));
	strText.Format(_T("%5u"), adpcm.readpoint);
	SetString(x, y, strText);
	y++;

	SetString(0, y, _T("ADPCM Write"));
	strText.Format(_T("%5u"), adpcm.writepoint);
	SetString(x, y, strText);
}

//===========================================================================
//
//	�C���v�b�g�E�B���h�E
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	�R���X�g���N�^
//
//---------------------------------------------------------------------------
CInputWnd::CInputWnd()
{
	CFrmWnd *pFrmWnd;

	// �E�B���h�E�p�����[�^��`
	m_dwID = MAKEID('I', 'N', 'P', ' ');
	::GetMsg(IDS_SWND_INPUT, m_strCaption);
	m_nWidth = 46;
	m_nHeight = 33;

	// �C���v�b�g�R���|�[�l���g���擾
	pFrmWnd = (CFrmWnd*)AfxGetApp()->m_pMainWnd;
	ASSERT(pFrmWnd);
	m_pInput = pFrmWnd->GetInput();
	ASSERT(m_pInput);
}

//---------------------------------------------------------------------------
//
//	�Z�b�g�A�b�v
//
//---------------------------------------------------------------------------
void FASTCALL CInputWnd::Setup()
{
	int x;
	int y;

	ASSERT(this);

	// �N���A
	Clear();

	// ���͌n�S��
	x = 0;
	y = 0;
	SetupInput(x, y);

	// �}�E�X
	x = 0;
	y = 4;
	SetupMouse(x, y);

	// �L�[�{�[�h
	x = 26;
	y = 0;
	SetupKey(x, y);

	// �W���C�X�e�B�b�NA
	x = 0;
	y = 11;
	SetupJoy(x, y, 0);

	// �W���C�X�e�B�b�NB
	x = 25;
	y = 11;
	SetupJoy(x, y, 1);
}

//---------------------------------------------------------------------------
//
//	�Z�b�g�A�b�v(���)
//
//---------------------------------------------------------------------------
void FASTCALL CInputWnd::SetupInput(int x, int y)
{
	CString strText;

	ASSERT(this);
	ASSERT(m_pInput);
	ASSERT((x >= 0) && (x < m_nWidth));
	ASSERT((y >= 0) && (y < m_nHeight));

	// ���s�J�E���^
	SetString(x, y, _T("Input Counter"));
	strText.Format(_T("%6u"), m_pInput->GetProcessCount());
	SetString(x + 16, y, strText);
	y++;

	// �A�N�e�B�u
	SetString(x, y, _T("Active"));
	if (m_pInput->IsActive()) {
		SetString(x + 16, y, _T("Active"));
	}
	else {
		SetString(x + 20, y, _T("No"));
	}
	y++;

	// ���j���[
	SetString(x, y, _T("Menu"));
	if (m_pInput->IsMenu()) {
		SetString(x + 18, y, _T("Menu"));
	}
	else {
		SetString(x + 20, y, _T("No"));
	}
}

//---------------------------------------------------------------------------
//
//	�Z�b�g�A�b�v(�}�E�X)
//
//---------------------------------------------------------------------------
void FASTCALL CInputWnd::SetupMouse(int x, int y)
{
	CString strText;
	int nPos[3];
	BOOL bBtn[3];

	ASSERT(this);
	ASSERT(m_pInput);
	ASSERT((x >= 0) && (x < m_nWidth));
	ASSERT((y >= 0) && (y < m_nHeight));

	// Acquire
	SetString(x, y, _T("Mouse Acquire"));
	strText.Format(_T("%6u"), m_pInput->GetAcquireCount(1));
	SetString(x + 16, y, strText);
	y++;

	// Mode
	SetString(x, y, _T("Mouse Mode"));
	if (m_pInput->GetMouseMode()) {
		SetString(x + 16, y, _T("Enable"));
	}
	else {
		SetString(x + 15, y, _T("Disable"));
	}
	y++;

	// ���擾
	m_pInput->GetMouseInfo(nPos, bBtn);

	// X
	SetString(x, y, _T("Mouse X"));
	strText.Format(_T("%6d"), nPos[0]);
	SetString(x + 16, y, strText);
	y++;

	// Y
	SetString(x, y, _T("Mouse Y"));
	strText.Format(_T("%6d"), nPos[1]);
	SetString(x + 16, y, strText);
	y++;

	// �{�^����
	SetString(x, y, _T("Mouse Button(L)"));
	if (bBtn[0]) {
		SetString(x + 18, y, _T("Down"));
	}
	else {
		SetString(x + 20, y, _T("Up"));
	}
	y++;

	// �{�^���E
	SetString(x, y, _T("Mouse Button(R)"));
	if (bBtn[1]) {
		SetString(x + 18, y, _T("Down"));
	}
	else {
		SetString(x + 20, y, _T("Up"));
	}
	y++;
}

//---------------------------------------------------------------------------
//
//	�Z�b�g�A�b�v(�L�[�{�[�h)
//
//---------------------------------------------------------------------------
void FASTCALL CInputWnd::SetupKey(int x, int y)
{
	CString strText;
	LPCTSTR lpszID;
	int i;
	int nCount;
	BOOL buf[0x100];

	ASSERT(this);
	ASSERT(m_pInput);
	ASSERT((x >= 0) && (x < m_nWidth));
	ASSERT((y >= 0) && (y < m_nHeight));

	// Acquire
	SetString(x, y, _T("Key Acquire"));
	strText.Format(_T("%6u"), m_pInput->GetAcquireCount(0));
	SetString(x + 14, y, strText);
	y++;

	// �L�[�o�b�t�@�擾
	m_pInput->GetKeyBuf(buf);

	// �J�E���g�ݒ�
	nCount = 0;

	// �L�[���[�v
	for (i=0; i<0x100; i++) {
		// �I�[�o�[�`�F�b�N
		if (nCount >= 9) {
			break;
		}

		// �����`�F�b�N
		if (buf[i]) {
			// ���̎擾
			lpszID = m_pInput->GetKeyID(i);
			ASSERT(lpszID);

			// �\��
			SetString(x, y, lpszID);
			nCount++;
			y++;
		}
	}
}

//---------------------------------------------------------------------------
//
//	�Z�b�g�A�b�v(�W���C�X�e�B�b�N)
//
//---------------------------------------------------------------------------
void FASTCALL CInputWnd::SetupJoy(int x, int y, int nJoy)
{
	int i;
	int nDevice;
	LONG lAxis;
	DWORD dwButton;
	CString strJoy;
	CString strText;

	ASSERT(this);
	ASSERT(m_pInput);
	ASSERT((x >= 0) && (x < m_nWidth));
	ASSERT((y >= 0) && (y < m_nHeight));
	ASSERT((nJoy >= 0) && (nJoy < 2));

	// Joy-A or Joy-B �쐬
	if (nJoy == 0) {
		strJoy = _T("Joy-A");
	}
	else {
		strJoy = _T("Joy-B");
	}

	// �f�o�C�X
	SetString(x, y, strJoy);
	SetString(x + 6, y, _T("Device"));
	nDevice = m_pInput->GetJoyDevice(nJoy);
	if (nDevice > 0) {
		strText.Format(_T("%2u"), nDevice - 1);
		SetString(x + 19, y, strText);
	}
	if (nDevice == 0) {
		SetString(x + 15, y, _T("(None)"));
	}
	if (nDevice < 0) {
		SetString(x + 14, y, _T("(Error)"));
	}
	y++;

	// Acquire
	SetString(x, y, strJoy);
	SetString(x + 6, y, _T("Acquire"));
	strText.Format(_T("%6u"), m_pInput->GetAcquireCount(nJoy + 2));
	SetString(x + 15, y, strText);
	y++;

	// ��
	for (i=0; i<8; i++) {
		SetString(x, y, strJoy);
		strText.Format(_T("Axis%d"), i + 1);
		SetString(x + 6, y, strText);

		// �f�[�^�擾
		lAxis = m_pInput->GetJoyAxis(nJoy, i);

		if (lAxis >= 0x10000) {
			// �������݂��Ȃ�
			SetString(x + 16, y, _T("-----"));
		}
		else {
			// ���f�[�^��\��
			strText.Format(_T("%5d"), lAxis);
			SetString(x + 16, y, strText);
		}
		y++;
	}

	// �{�^��
	for (i=0; i<12; i++) {
		SetString(x, y, strJoy);
		strText.Format(_T("Button%d"), i + 1);
		SetString(x + 6, y, strText);

		// �f�[�^�擾
		dwButton = m_pInput->GetJoyButton(nJoy, i);

		if (dwButton >= 0x10000) {
			// �{�^�������݂��Ȃ�
			SetString(x + 18, y, _T("---"));
		}
		else {
			if (dwButton & 0x80) {
				// ������Ă���
				SetString(x + 19, y, _T("On"));
			}
			else {
				// ������Ă��Ȃ�
				SetString(x + 18, y, _T("Off"));
			}
		}
		y++;
	}
}

//===========================================================================
//
//	�|�[�g�E�B���h�E
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	�R���X�g���N�^
//
//---------------------------------------------------------------------------
CPortWnd::CPortWnd()
{
	CFrmWnd *pFrmWnd;

	// �E�B���h�E�p�����[�^��`
	m_dwID = MAKEID('P', 'O', 'R', 'T');
	::GetMsg(IDS_SWND_PORT, m_strCaption);
	m_nWidth = 23;
	m_nHeight = 18;

	// �|�[�g�R���|�[�l���g���擾
	pFrmWnd = (CFrmWnd*)AfxGetApp()->m_pMainWnd;
	ASSERT(pFrmWnd);
	m_pPort = pFrmWnd->GetPort();
	ASSERT(m_pPort);
}

//---------------------------------------------------------------------------
//
//	�Z�b�g�A�b�v
//
//---------------------------------------------------------------------------
void FASTCALL CPortWnd::Setup()
{
	int x;
	int y;
	CString strText;
	TCHAR szDevice[_MAX_PATH];
	DWORD dwHandle;
	BOOL bRun;
	CQueue::QUEUEINFO qi;

	ASSERT(this);

	// ������
	Clear();
	x = 15;
	y = 0;

	// COM���擾
	bRun = m_pPort->GetCOMInfo(szDevice, &dwHandle);

	// COM�X���b�h
	SetString(0, y, _T("COM Thread"));
	if (bRun) {
		SetString(x + 5, y, _T("Run"));
	}
	else {
		SetString(x + 4, y, _T("Stop"));
	}
	y++;

	// COM�t�@�C��
	SetString(0, y, _T("COM Device"));
	SetString(x, y, szDevice);
	y++;

	// COM���O
	SetString(0, y, _T("COM Log"));
	strText.Format(_T("%08X"), dwHandle);
	SetString(x, y, strText);
	y++;

	// Tx�L���[���擾
	m_pPort->GetTxQueue(&qi);

	// Tx Read
	SetString(0, y, _T("Tx  Read"));
	strText.Format(_T("%4d"), qi.dwRead);
	SetString(x + 4, y, strText);
	y++;

	// Tx Write
	SetString(0, y, _T("Tx  Write"));
	strText.Format(_T("%4d"), qi.dwWrite);
	SetString(x + 4, y, strText);
	y++;

	// Tx Num
	SetString(0, y, _T("Tx  Num"));
	strText.Format(_T("%4d"), qi.dwNum);
	SetString(x + 4, y, strText);
	y++;

	// Tx Total
	SetString(0, y, _T("Tx  Total"));
	strText.Format(_T("%7d"), qi.dwTotalRead);
	SetString(x + 1, y, strText);
	y++;

	// Rx�L���[���擾
	m_pPort->GetRxQueue(&qi);

	// Rx Read
	SetString(0, y, _T("Rx  Read"));
	strText.Format(_T("%4d"), qi.dwRead);
	SetString(x + 4, y, strText);
	y++;

	// Rx Write
	SetString(0, y, _T("Rx  Write"));
	strText.Format(_T("%4d"), qi.dwWrite);
	SetString(x + 4, y, strText);
	y++;

	// Rx Num
	SetString(0, y, _T("Rx  Num"));
	strText.Format(_T("%4d"), qi.dwNum);
	SetString(x + 4, y, strText);
	y++;

	// Rx Total
	SetString(0, y, _T("Rx  Total"));
	strText.Format(_T("%7d"), qi.dwTotalRead);
	SetString(x + 1, y, strText);
	y++;

	// LPT���擾
	bRun = m_pPort->GetLPTInfo(szDevice, &dwHandle);

	// LPT�X���b�h
	SetString(0, y, _T("LPT Thread"));
	if (bRun) {
		SetString(x + 5, y, _T("Run"));
	}
	else {
		SetString(x + 4, y, _T("Stop"));
	}
	y++;

	// LPT�t�@�C��
	SetString(0, y, _T("LPT Device"));
	SetString(x, y, szDevice);
	y++;

	// LPT���O
	SetString(0, y, _T("LPT Log"));
	strText.Format(_T("%08X"), dwHandle);
	SetString(x, y, strText);
	y++;

	// LPT�L���[���擾
	m_pPort->GetLPTQueue(&qi);

	// LPT Read
	SetString(0, y, _T("LPT Read"));
	strText.Format(_T("%4d"), qi.dwRead);
	SetString(x + 4, y, strText);
	y++;

	// LPT Write
	SetString(0, y, _T("LPT Write"));
	strText.Format(_T("%4d"), qi.dwWrite);
	SetString(x + 4, y, strText);
	y++;

	// LPT Num
	SetString(0, y, _T("LPT Num"));
	strText.Format(_T("%4d"), qi.dwNum);
	SetString(x + 4, y, strText);
	y++;

	// LPT Total
	SetString(0, y, _T("LPT Total"));
	strText.Format(_T("%7d"), qi.dwTotalRead);
	SetString(x + 1, y, strText);
	y++;
}

//===========================================================================
//
//	�r�b�g�}�b�v�E�B���h�E
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	�R���X�g���N�^
//
//---------------------------------------------------------------------------
CBitmapWnd::CBitmapWnd()
{
	CFrmWnd *pFrmWnd;

	// �E�B���h�E�p�����[�^��`
	m_dwID = MAKEID('B', 'M', 'A', 'P');
	::GetMsg(IDS_SWND_BITMAP, m_strCaption);
	m_nWidth = 46;
	m_nHeight = 8;

	// �`��E�B���h�E���擾
	pFrmWnd = (CFrmWnd*)AfxGetApp()->m_pMainWnd;
	ASSERT(pFrmWnd);
	m_pView = pFrmWnd->GetView();
	ASSERT(m_pView);
}

//---------------------------------------------------------------------------
//
//	�Z�b�g�A�b�v
//
//---------------------------------------------------------------------------
void FASTCALL CBitmapWnd::Setup()
{
	int x;
	int y;
	CString string;
	CDrawView::DRAWINFO info;

	// �f�[�^���擾
	m_pView->GetDrawInfo(&info);

	// �N���A
	Clear();
	x = 0;
	y = 0;

	// �r�b�g�}�b�v
	SetString(x, y, "Bitmap Width");
	string.Format("%4d", info.nBMPWidth);
	SetString(x + 17, y, string);
	y++;
	SetString(x, y, "Render Width");
	string.Format("%4d", info.nRendWidth);
	SetString(x + 17, y, string);
	y++;
	SetString(x, y, "Render H Mul.");
	string.Format("%4d", info.nRendHMul);
	SetString(x + 17, y, string);
	y++;

	// BitBlt
	SetString(x, y, "BitBlt Width");
	string.Format("%4d", info.nWidth);
	SetString(x + 17, y, string);
	y++;
	SetString(x, y, "BitBlt X");
	string.Format("%4d", info.nLeft);
	SetString(x + 17, y, string);
	y++;
	SetString(x, y, "BitBlt Top");
	string.Format("%4d", info.nBltTop);
	SetString(x + 17, y, string);
	y++;
	SetString(x, y, "BitBlt Left");
	string.Format("%4d", info.nBltLeft);
	SetString(x + 17, y, string);
	y++;

	// �t���O
	SetString(x, y, "BitBlt Stretch");
	if (info.bBltStretch) {
		SetString(x + 18, y, "Yes");
	}
	else {
		SetString(x + 19, y, "No");
	}

	// �E����
	x = 25;
	y = 0;

	// �r�b�g�}�b�v
	SetString(x, y, "Bitmap Height");
	string.Format("%4d", info.nBMPHeight);
	SetString(x + 17, y, string);
	y++;
	SetString(x, y, "Render Height");
	string.Format("%4d", info.nRendHeight);
	SetString(x + 17, y, string);
	y++;
	SetString(x, y, "Render V Mul.");
	string.Format("%4d", info.nRendVMul);
	SetString(x + 17, y, string);
	y++;

	// BitBlt
	SetString(x, y, "BitBlt Height");
	string.Format("%4d", info.nHeight);
	SetString(x + 17, y, string);
	y++;
	SetString(x, y, "BitBlt Y");
	string.Format("%4d", info.nTop);
	SetString(x + 17, y, string);
	y++;
	SetString(x, y, "BitBlt Bottom");
	string.Format("%4d", info.nBltBottom + 1);
	SetString(x + 17, y, string);
	y++;
	SetString(x, y, "BitBlt Right");
	string.Format("%4d", info.nBltRight + 1);
	SetString(x + 17, y, string);
	y++;

	// ��
	SetString(x, y, "BitBlt Count");
	string.Format("%7d", info.dwDrawCount);
	SetString(x + 14, y, string);
}

//===========================================================================
//
//	MIDI�h���C�o�E�B���h�E
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	�R���X�g���N�^
//
//---------------------------------------------------------------------------
CMIDIDrvWnd::CMIDIDrvWnd()
{
	CFrmWnd *pFrmWnd;

	// �E�B���h�E�p�����[�^��`
	m_dwID = MAKEID('M', 'D', 'R', 'V');
	::GetMsg(IDS_SWND_MIDIDRV, m_strCaption);
	m_nWidth = 38;
	m_nHeight = 11;

	// MIDI�f�o�C�X���擾
	m_pMIDI = (MIDI*)::GetVM()->SearchDevice(MAKEID('M', 'I', 'D', 'I'));
	ASSERT(m_pMIDI);

	// MIDI�R���|�[�l���g���擾
	pFrmWnd = (CFrmWnd*)AfxGetApp()->m_pMainWnd;
	ASSERT(pFrmWnd);
	m_pMIDIDrv = pFrmWnd->GetMIDI();
	ASSERT(m_pMIDIDrv);
}

//---------------------------------------------------------------------------
//
//	�Z�b�g�A�b�v
//
//---------------------------------------------------------------------------
void FASTCALL CMIDIDrvWnd::Setup()
{
	int i;
	int y;
	MIDI::midi_t midi;
	CMIDI::MIDIINFO mi;
	DWORD dwStart;
	DWORD dwEnd;

	ASSERT(this);
	ASSERT(m_pMIDI);
	ASSERT(m_pMIDIDrv);

	// �f�o�C�X���[�N�擾
	m_pMIDI->GetMIDI(&midi);

	// �N���A
	Clear();

	// ������\��
	y = 1;
	for (i=0;; i++) {
		if (!DescTable[i]) {
			break;
		}

		SetString(0, y, DescTable[i]);
		y++;
	}

	// IN
	m_pMIDIDrv->GetInInfo(&mi);
	SetString(20, 0, _T("MIDI IN"));
	mi.dwBufNum = midi.recvnum;
	mi.dwBufRead = midi.recvread;
	mi.dwBufWrite = midi.recvwrite;
	SetupInfo(19, 1, &mi);

	// IN(�G�N�X�N���[�V�u�J�E���^)
	dwStart = m_pMIDI->GetExCount(2);
	dwEnd = m_pMIDI->GetExCount(3);
	SetupExCnt(19, 1, dwStart, dwEnd);

	// OUT
	m_pMIDIDrv->GetOutInfo(&mi);
	SetString(30, 0, _T("MIDI OUT"));
	mi.dwBufNum = midi.transnum;
	mi.dwBufRead = midi.transread;
	mi.dwBufWrite = midi.transwrite;
	SetupInfo(30, 1, &mi);

	// OUT(�G�N�X�N���[�V�u�J�E���^)
	dwStart = m_pMIDI->GetExCount(0);
	dwEnd = m_pMIDI->GetExCount(1);
	SetupExCnt(30, 1, dwStart, dwEnd);
}

//---------------------------------------------------------------------------
//
//	�Z�b�g�A�b�v(�T�u)
//
//---------------------------------------------------------------------------
void FASTCALL CMIDIDrvWnd::SetupInfo(int x, int y, CMIDI::LPMIDIINFO pInfo)
{
	CString strText;

	ASSERT(this);
	ASSERT((x >= 0) && (x < m_nWidth));
	ASSERT((y >= 0) && (y < m_nHeight));
	ASSERT(pInfo);

	// �f�o�C�X����
	strText.Format(_T("%8d"), pInfo->dwDevices);
	SetString(x, y, strText);
	y++;

	// �J�����g�f�o�C�X
	strText.Format(_T("%8d"), pInfo->dwDevice);
	SetString(x, y, strText);
	y++;

	// �V���[�g���b�Z�[�W
	strText.Format(_T("%8d"), pInfo->dwShort);
	SetString(x, y, strText);
	y++;

	// �����O���b�Z�[�W
	strText.Format(_T("%8d"), pInfo->dwLong);
	SetString(x, y, strText);
	y++;

	// �w�b�_���
	strText.Format(_T("%8d"), pInfo->dwUnprepare);
	SetString(x, y, strText);
	y++;

	// �o�b�t�@�L����
	strText.Format(_T("%8d"), pInfo->dwBufNum);
	SetString(x, y, strText);
	y++;

	// �o�b�t�@Read
	strText.Format(_T("%8d"), pInfo->dwBufRead);
	SetString(x, y, strText);
	y++;

	// �o�b�t�@Write
	strText.Format(_T("%8d"), pInfo->dwBufWrite);
	SetString(x, y, strText);
}

//---------------------------------------------------------------------------
//
//	�Z�b�g�A�b�v(�G�N�X�N���[�V�u�J�E���^)
//
//---------------------------------------------------------------------------
void FASTCALL CMIDIDrvWnd::SetupExCnt(int x, int y, DWORD dwStart, DWORD dwEnd)
{
	CString strText;

	ASSERT(this);
	ASSERT((x >= 0) && (x < m_nWidth));
	ASSERT((y >= 0) && (y < m_nHeight));

	// y�␳
	y += 8;

	// �G�N�X�N���[�V�u�J�n(F0)��
	strText.Format(_T("%8d"), dwStart);
	SetString(x, y, strText);
	y++;

	// �G�N�X�N���[�V�u�I��(F7)��
	strText.Format(_T("%8d"), dwEnd);
	SetString(x, y, strText);
	y++;
}

//---------------------------------------------------------------------------
//
//	������e�[�u��
//
//---------------------------------------------------------------------------
LPCTSTR CMIDIDrvWnd::DescTable[] = {
	_T("Num of Devices"),
	_T("Current Device"),
	_T("Short     Message"),
	_T("Exclusive Message"),
	_T("Unprepere Header"),
	_T("Buffer Num"),
	_T("Buffer Read"),
	_T("Buffer Write"),
	_T("Exclusive Start"),
	_T("Exclusive End"),
	NULL
};

//===========================================================================
//
//	�L�[�{�[�h�f�B�X�v���C�E�B���h�E
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	�R���X�g���N�^
//
//---------------------------------------------------------------------------
CKeyDispWnd::CKeyDispWnd()
{
	// �_�����[�N
	m_nMode = 0;
	memset(m_nKey, 0, sizeof(m_nKey));
	memset(m_bKey, 0, sizeof(m_bKey));
	m_nPoint = 0;

	// �\�����[�N
	m_pCG = NULL;
	m_pBits = NULL;
	m_hBitmap = NULL;
	m_nBMPWidth = 0;
	m_nBMPHeight = 0;
	m_nBMPMul = 0;
}

//---------------------------------------------------------------------------
//
//	���b�Z�[�W �}�b�v
//
//---------------------------------------------------------------------------
BEGIN_MESSAGE_MAP(CKeyDispWnd, CWnd)
	ON_WM_CREATE()
	ON_WM_DESTROY()
	ON_WM_SIZE()
	ON_WM_ERASEBKGND()
	ON_WM_PAINT()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_RBUTTONDOWN()
	ON_WM_RBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_WM_GETDLGCODE()
END_MESSAGE_MAP()

//---------------------------------------------------------------------------
//
//	�E�B���h�E�쐬
//
//---------------------------------------------------------------------------
int CKeyDispWnd::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	Memory *pMemory;
	CRect rectWnd;

	// ��{�N���X
	if (CWnd::OnCreate(lpCreateStruct) != 0) {
		return -1;
	}

	// IME�I�t
	::ImmAssociateContext(m_hWnd, (HIMC)NULL);

	// CGROM�擾
	pMemory = (Memory*)::GetVM()->SearchDevice(MAKEID('M', 'E', 'M', ' '));
	ASSERT(pMemory);
	m_pCG = pMemory->GetCG();
	ASSERT(m_pCG);

	// ���T�C�Y
	rectWnd.left = 0;
	rectWnd.top = 0;
	rectWnd.right = 616;
	rectWnd.bottom = 140;
	CalcWindowRect(&rectWnd);
	SetWindowPos(&wndTop, 0, 0, rectWnd.Width(), rectWnd.Height(), SWP_NOMOVE);

	// �r�b�g�}�b�v�쐬
	SetupBitmap();

	return 0;
}

//---------------------------------------------------------------------------
//
//	�E�B���h�E�폜
//
//---------------------------------------------------------------------------
void CKeyDispWnd::OnDestroy()
{
	// �r�b�g�}�b�v���
	if (m_hBitmap) {
		::DeleteObject(m_hBitmap);
		m_hBitmap = NULL;
		m_pBits = NULL;
	}

	// ��{�N���X
	CWnd::OnDestroy();
}

//---------------------------------------------------------------------------
//
//	�E�B���h�E�폜����
//
//---------------------------------------------------------------------------
void CKeyDispWnd::PostNcDestroy()
{
	ASSERT(this);
	ASSERT(!m_hBitmap);
	ASSERT(!m_pBits);

	// �C���^�t�F�[�X�v�f���폜
	delete this;
}

//---------------------------------------------------------------------------
//
//	�E�B���h�E�T�C�Y�ύX
//
//---------------------------------------------------------------------------
void CKeyDispWnd::OnSize(UINT nType, int cx, int cy)
{
	// ��{�N���X���Ɏ��s
	CWnd::OnSize(nType, cx, cy);

	// �r�b�g�}�b�v�쐬
	SetupBitmap();
}

//---------------------------------------------------------------------------
//
//	�r�b�g�}�b�v�쐬
//
//---------------------------------------------------------------------------
void FASTCALL CKeyDispWnd::SetupBitmap()
{
	int i;
	BITMAPINFOHEADER bmi;
	CRect rectClient;
	CClientDC dc(this);

	ASSERT(this);

	// �ŏ����Ȃ�r�b�g�}�b�v�쐬���Ȃ�
	if (IsIconic()) {
		return;
	}

	// �N���C�A���g��`�擾
	GetClientRect(&rectClient);
	ASSERT((rectClient.Width() != 0) || (rectClient.Height() != 0));

	// �r�b�g�}�b�v�����݂���
	if (m_hBitmap) {
		// �f�B�����W�����������Ȃ�
		if (m_nBMPWidth == (UINT)rectClient.Width()) {
			if (m_nBMPHeight == (UINT)rectClient.Height()) {
				// �������Ȃ�
				return;
			}
		}
	}

	// VM���b�N
	::LockVM();

	// �r�b�g�}�b�v���
	if (m_hBitmap) {
		::DeleteObject(m_hBitmap);
		m_hBitmap = NULL;
		m_pBits = NULL;
	}

	// �r�b�g�}�b�v���L��
	m_nBMPWidth = rectClient.Width();
	m_nBMPHeight = rectClient.Height();
	m_nBMPMul = (m_nBMPWidth + 3) >> 2;
	m_nBMPMul <<= 2;

	// �r�b�g�}�b�v���쐬(256�F�p���b�g���r�b�g�}�b�v)
	memset(&bmi, 0, sizeof(bmi));
	bmi.biSize = sizeof(BITMAPINFOHEADER);
	bmi.biWidth = m_nBMPWidth;
	bmi.biHeight = m_nBMPHeight;
	bmi.biHeight = -(bmi.biHeight);
	bmi.biPlanes = 1;
	bmi.biBitCount = 8;
	bmi.biCompression = BI_RGB;
	bmi.biSizeImage = m_nBMPMul * m_nBMPHeight;

	// DC�擾�ADIB�Z�N�V�����쐬
	m_hBitmap = ::CreateDIBSection(dc.m_hDC, (BITMAPINFO*)&bmi, DIB_RGB_COLORS,
									(void**)&m_pBits, NULL, 0);
	ASSERT(m_hBitmap);
	ASSERT(m_pBits);

	// �r�b�g�}�b�v�N���A
	if (m_hBitmap) {
		memset(m_pBits, 0, bmi.biSizeImage);
	}

	// �\�����e�͕s��
	for (i=0; i<0x80; i++) {
		m_nKey[i] = 0;
	}

	// VM�A�����b�N
	::UnlockVM();

	// �ĕ`����N����
	Invalidate(FALSE);
}

//---------------------------------------------------------------------------
//
//	�E�B���h�E�w�i�`��
//
//---------------------------------------------------------------------------
BOOL CKeyDispWnd::OnEraseBkgnd(CDC* /*pDC*/)
{
	ASSERT(this);

	// ���������ATRUE��Ԃ�
	return TRUE;
}

//---------------------------------------------------------------------------
//
//	�E�B���h�E�ĕ`��
//
//---------------------------------------------------------------------------
void CKeyDispWnd::OnPaint()
{
	CPaintDC dc(this);
	int i;

	ASSERT(this);

	// �S�ẴL�[�𖢒��Ԃ�
	for (i=0; i<0x80; i++) {
		m_nKey[i] = 0;
	}

	// �`��
	OnDraw(&dc);
}

//---------------------------------------------------------------------------
//
//	���{�^������
//
//---------------------------------------------------------------------------
void CKeyDispWnd::OnLButtonDown(UINT /*nFlags*/, CPoint point)
{
	CWnd *pWnd;
	int nKey;

	// Point����L�[�C���f�b�N�X�擾
	nKey = PtInKey(point);

	// �����Ă��Ȃ���Ή������Ȃ�
	if (nKey == 0) {
		return;
	}

	// �e�E�B���h�E�Ƀ��b�Z�[�W�𑗐M
	pWnd = GetParent();
	if (pWnd) {
		pWnd->PostMessage(WM_APP, (WPARAM)nKey, (LPARAM)WM_LBUTTONDOWN);
	}
}

//---------------------------------------------------------------------------
//
//	���{�^��������
//
//---------------------------------------------------------------------------
void CKeyDispWnd::OnLButtonUp(UINT /*nFlags*/, CPoint point)
{
	CWnd *pWnd;
	int nKey;

	// Point����L�[�C���f�b�N�X�擾
	nKey = PtInKey(point);

	// �e�E�B���h�E�Ƀ��b�Z�[�W�𑗐M
	pWnd = GetParent();
	if (pWnd) {
		pWnd->PostMessage(WM_APP, (WPARAM)nKey, (LPARAM)WM_LBUTTONUP);
	}
}

//---------------------------------------------------------------------------
//
//	�E�{�^������
//
//---------------------------------------------------------------------------
void CKeyDispWnd::OnRButtonDown(UINT /*nFlags*/, CPoint point)
{
	CWnd *pWnd;
	int nKey;

	// Point����L�[�C���f�b�N�X�擾
	nKey = PtInKey(point);

	// �����Ă��Ȃ���Ή������Ȃ�
	if (nKey == 0) {
		return;
	}

	// �e�E�B���h�E�Ƀ��b�Z�[�W�𑗐M
	pWnd = GetParent();
	if (pWnd) {
		pWnd->PostMessage(WM_APP, (WPARAM)nKey, (LPARAM)WM_RBUTTONDOWN);
	}
}

//---------------------------------------------------------------------------
//
//	�E�{�^��������
//
//---------------------------------------------------------------------------
void CKeyDispWnd::OnRButtonUp(UINT /*nFlags*/, CPoint point)
{
	CWnd *pWnd;
	int nKey;

	// Point����L�[�C���f�b�N�X�擾
	nKey = PtInKey(point);

	// �e�E�B���h�E�Ƀ��b�Z�[�W�𑗐M
	pWnd = GetParent();
	if (pWnd) {
		pWnd->PostMessage(WM_APP, (WPARAM)nKey, (LPARAM)WM_RBUTTONUP);
	}
}

//---------------------------------------------------------------------------
//
//	�}�E�X�ړ�
//
//---------------------------------------------------------------------------
void CKeyDispWnd::OnMouseMove(UINT /*nFlags*/, CPoint point)
{
	CWnd *pWnd;
	int nKey;

	// Point����L�[�C���f�b�N�X�擾
	nKey = PtInKey(point);

	// ����ƈ�v���Ă����OK
	if (m_nPoint == nKey) {
		return;
	}

	// �X�V����
	m_nPoint = nKey;

	// �e�E�B���h�E�Ƀ��b�Z�[�W�𑗐M
	pWnd = GetParent();
	if (pWnd) {
		pWnd->PostMessage(WM_APP, (WPARAM)nKey, (LPARAM)WM_MOUSEMOVE);
	}
}

//---------------------------------------------------------------------------
//
//	�_�C�A���O�R�[�h�擾
//	���_�C�A���O���̃`���C���h�E�B���h�E�Ƃ��Ďg��ꂽ�ꍇ�̂�
//
//---------------------------------------------------------------------------
UINT CKeyDispWnd::OnGetDlgCode()
{
	return DLGC_WANTMESSAGE;
}

//---------------------------------------------------------------------------
//
//	��`���L�[�擾
//
//---------------------------------------------------------------------------
int FASTCALL CKeyDispWnd::PtInKey(CPoint& point)
{
	int i;
	int nKey;
	CRect rect;

	ASSERT(this);

	// ������
	nKey = -1;

	// ��`���ɓ����Ă��邩����
	for (i=0; i<=0x74; i++) {
		rect = RectTable[i];
		if (rect.top != 0) {
			// �����Ɋ񂹂�
			rect.top++;
			rect.left++;
			rect.right--;
			rect.bottom--;

			// ����
			if (rect.PtInRect(point)) {
				nKey = i;
				break;
			}
		}
	}

	// �����Ă��Ȃ����
	if (nKey < 0) {
		// CR�̔���
		rect.top = RectTable[0x1c].top + 1;
		rect.bottom = RectTable[0x29].bottom - 1;
		rect.left = RectTable[0x1c].right + 1;
		rect.right = RectTable[0x0f].right - 1;
		if (rect.PtInRect(point)) {
			nKey = 0x1d;
		}
		else {
			// ���̑���0
			nKey = 0;
		}
	}

	// SHIFT�L�[���ꏈ��
	if (nKey == 0x74) {
		nKey = 0x70;
	}

	return nKey;
}

//---------------------------------------------------------------------------
//
//	�V�t�g���[�h�w��
//
//	b7	SHIFT
//	b6	CTRL
//	b5	(���g�p)
//	b4	(���g�p)
//	b3	CAPS
//	b2	�R�[�h����
//	b1	���[�}��
//	b0	����
//
//---------------------------------------------------------------------------
void FASTCALL CKeyDispWnd::SetShiftMode(UINT nMode)
{
	int i;

	ASSERT(this);
	ASSERT(nMode < 0x100);

	// �Q�ƕ������}�X�N
	nMode &= 0x89;

	// ���[�h���ω����Ă��Ȃ���΁A�������Ȃ�
	if (m_nMode == nMode) {
		return;
	}

	// ���[�h�X�V
	m_nMode = nMode;

	// �S�ẴL�[�𖢒��Ԃ�(�`��͂��Ȃ�)
	for (i=0; i<0x80; i++) {
		m_nKey[i] = 0;
	}
}

//---------------------------------------------------------------------------
//
//	���t���b�V��
//
//---------------------------------------------------------------------------
void FASTCALL CKeyDispWnd::Refresh(const BOOL *pKey)
{
	int i;
	UINT nCode;
	BOOL bFlag;
	CClientDC dc(this);

	ASSERT(this);
	ASSERT(pKey);

	// �ύX�t���O������
	bFlag = FALSE;

	// ���[�v
	for (i=0; i<0x80; i++) {
		// �܂��R�s�[
		m_bKey[i] = pKey[i];

		// ���l�����Ĕ�r
		if (m_bKey[i]) {
			nCode = 2;
		}
		else {
			nCode = 1;
		}

		// �قȂ��Ă���΃t���O���āA������
		if (m_nKey[i] != nCode) {
			bFlag = TRUE;
			break;
		}
	}

	// �t���O�~��Ă���΂����܂�
	if (!bFlag) {
		return;
	}

	// �`��
	OnDraw(&dc);
}

//---------------------------------------------------------------------------
//
//  �L�[�ꊇ�ݒ�
//
//---------------------------------------------------------------------------
void FASTCALL CKeyDispWnd::SetKey(const BOOL* pKey)
{
	ASSERT(this);
	ASSERT(pKey);

	// �L�[�����R�s�[
	memcpy(m_bKey, pKey, sizeof(m_bKey));
}

//---------------------------------------------------------------------------
//
//	�E�B���h�E�`��
//
//---------------------------------------------------------------------------
void FASTCALL CKeyDispWnd::OnDraw(CDC *pDC)
{
	HBITMAP hBitmap;
	CRect rect;
	CDC mDC;
	int i;
	UINT nCode;

	ASSERT(this);

	// �r�b�g�}�b�v���ł��Ă��Ȃ���΁A�������Ȃ�
	if (!m_hBitmap) {
		return;
	}

	// �����L�[���[�v
	for (i=0; i<0x80; i++) {
		// ���l������
		if (m_bKey[i]) {
			nCode = 2;
		}
		else {
			nCode = 1;
		}

		// ��r
		if (m_nKey[i] != nCode) {
			// �قȂ��Ă���̂ŕۑ�����
			m_nKey[i] = nCode;

			// �`��
			if (nCode == 2) {
				DrawKey(i, TRUE);
			}
			else {
				DrawKey(i, FALSE);
			}
		}
	}

	// ������DC�쐬
	ASSERT(m_pBits);
	VERIFY(mDC.CreateCompatibleDC(pDC));
	GetClientRect(&rect);

	// �r�b�g�}�b�v�Z���N�g
	hBitmap = (HBITMAP)::SelectObject(mDC.m_hDC, m_hBitmap);
	if (hBitmap) {
		// DIB�J���[�e�[�u���ݒ�
		::SetDIBColorTable(mDC.m_hDC, 0, 16, PalTable);

		// BitBlt
		::BitBlt(pDC->m_hDC, 0, 0, rect.Width(), rect.Height(),
					mDC.m_hDC, 0, 0, SRCCOPY);

		// ����
		::SelectObject(mDC.m_hDC, hBitmap);
	}

	// ������DC�폜
	mDC.DeleteDC();
}

//---------------------------------------------------------------------------
//
//	�L�[������擾
//
//---------------------------------------------------------------------------
LPCTSTR FASTCALL CKeyDispWnd::GetKeyString(int nKey)
{
	LPCTSTR lpszString;
	UINT nModeXor;

	ASSERT(this);
	ASSERT((nKey >= 0) && (nKey < 0x80));

	// �I�[�o�[�`�F�b�N
	if (nKey > 0x74) {
		return NULL;
	}

	// �܂���{�l����
	lpszString = NormalTable[nKey];

	// ���ȃ`�F�b�N
	if (m_nMode & 1) {
		// ���ȃL�[�͈�
		if ((nKey >= 2) && (nKey <= 0x34)) {
			// SHIFT�ɂ�蕪����
			if (m_nMode & 0x80) {
				return KanaShiftTable[nKey - 2];
			}
			else {
				return KanaTable[nKey - 2];
			}
		}
	}

	// SHIFT�`�F�b�N
	if (m_nMode & 0x80) {
		if ((nKey >= 2) && (nKey <= 0x0e)) {
			return MarkTable[nKey - 2];
		}
	}

	// �p�����[�h�쐬
	nModeXor = m_nMode >> 4;
	nModeXor ^= m_nMode;
	if ((nModeXor & 0x08) == 0) {
		// �p���������̂��߁A����ւ�
		if ((nKey >= 0x11) && (nKey <= 0x1a)) {
			lpszString = AnotherTable[nKey - 0x11];
		}
		if ((nKey >= 0x1e) && (nKey <= 0x26)) {
			lpszString = AnotherTable[nKey - 0x11];
		}
		if ((nKey >= 0x2a) && (nKey <= 0x30)) {
			lpszString = AnotherTable[nKey - 0x11];
		}
	}

	// �L����SHIFT�œ���ւ��
	if (m_nMode & 0x80) {
		if ((nKey >= 0x1b) && (nKey <= 0x1d)) {
			lpszString = AnotherTable[nKey - 0x11];
		}
		if ((nKey >= 0x27) && (nKey <= 0x29)) {
			lpszString = AnotherTable[nKey - 0x11];
		}
		if ((nKey >= 0x31) && (nKey <= 0x34)) {
			lpszString = AnotherTable[nKey - 0x11];
		}
	}

	return lpszString;
}

//---------------------------------------------------------------------------
//
//	�L�[�`��
//
//---------------------------------------------------------------------------
void FASTCALL CKeyDispWnd::DrawKey(int nKey, BOOL bDown)
{
	int x;
	int y;
	int i;
	DWORD dwChar;
	CRect rectKey;
	const char *lpszKey;
	int nColor;
	int nWidth;

	ASSERT(this);
	ASSERT((nKey >= 0) && (nKey < 0x80));

	// ������擾
	lpszKey = GetKeyString(nKey);
	if (!lpszKey) {
		// CR�̓��������
		if (nKey == 0x1d) {
			// CR
			rectKey.left = RectTable[0x1c].right - 1;
			rectKey.top = RectTable[0x1c].top;
			rectKey.right = RectTable[0x0f].right;
			rectKey.bottom = RectTable[0x29].bottom;

			// CR�`��
			if (bDown) {
				DrawCRBox(10, 11, rectKey);
				DrawCRChar(rectKey.left, rectKey.top, 12);
			}
			else {
				DrawCRBox(10, 8, rectKey);
				DrawCRChar(rectKey.left, rectKey.top, 9);
			}
		}
		return;
	}

	// ��`�擾
	ASSERT(nKey <= 0x74);
	rectKey = RectTable[nKey];

	// ��`�`��
	if (bDown) {
		DrawBox(10, 11, rectKey);
		nColor = 12;
	}
	else {
		DrawBox(10, 8, rectKey);
		nColor = 9;
	}

	// �������𐔂��A�h�b�g��
	nWidth = (int)strlen(lpszKey);
	nWidth <<= 3;

	// �Z���^�����O
	x = 0;
	if (nWidth < rectKey.Width()) {
		x = (rectKey.Width() >> 1);
		x -= (nWidth >> 1);
	}
	y = 0;
	if (rectKey.Height() > 16) {
		y = (rectKey.Height() >> 1);
		y -= (15 >> 1);
	}

	// �����`��
	for (i=0;; i++) {
		// �I���`�F�b�N
		if (lpszKey[i] == '\0') {
			break;
		}

		// 1�����擾
		dwChar = (DWORD)(lpszKey[i] & 0xff);

		// ���p����
		if (dwChar < 0x80) {
			DrawChar(rectKey.left + x, rectKey.top + y + 1, nColor, dwChar);
			x += 8;
			continue;
		}
		if ((dwChar >= 0xa0) && (dwChar < 0xe0)) {
			DrawChar(rectKey.left + x, rectKey.top + y + 1, nColor, dwChar);
			x += 8;
			continue;
		}

		// �S�p
		dwChar <<= 8;
		dwChar |= (DWORD)(lpszKey[i + 1] & 0xff);
		i++;

		DrawChar(rectKey.left + x, rectKey.top + y + 1, nColor, dwChar);
		x += 16;
	}
}

//---------------------------------------------------------------------------
//
//	�{�b�N�X�`��
//
//---------------------------------------------------------------------------
void FASTCALL CKeyDispWnd::DrawBox(int nColorOut, int nColorIn, RECT& rect)
{
	int x;
	int y;
	BYTE *p;

	ASSERT(this);
	ASSERT(m_hBitmap);
	ASSERT(m_pBits);
	ASSERT((nColorOut >= 0) && (nColorOut < 0x100));
	ASSERT((nColorIn >= 0) && (nColorIn < 0x100));
	ASSERT(rect.top >= 0);
	ASSERT(rect.top <= rect.bottom);
	ASSERT(rect.left >= 0);
	ASSERT(rect.left <= rect.right);

	// �����ʒu�v�Z
	p = m_pBits;
	p += (m_nBMPMul * rect.top);
	p += rect.left;

	// ��(���E1dot�P�ʂł�����)
	memset(p + 1, nColorOut, (rect.right - rect.left - 2));
	p += m_nBMPMul;

	// ����
	x = rect.right - rect.left;
	x -= 2;
	rect.bottom -= 2;

	// y���[�v
	for (y=rect.top; y<rect.bottom; y++) {
		// ���C������
		p[0] = (BYTE)nColorOut;
		memset(p + 1, nColorIn, x);
		p[x + 1] = (BYTE)nColorOut;

		p += m_nBMPMul;
	}

	// �Ō�(���E1dot�P�ʂł�����)
	memset(p + 1, nColorOut, (rect.right - rect.left - 2));
}

//---------------------------------------------------------------------------
//
//	CR�{�b�N�X�`��
//
//---------------------------------------------------------------------------
void FASTCALL CKeyDispWnd::DrawCRBox(int nColorOut, int nColorIn, RECT& rect)
{
	int x;
	int y;
	BYTE *p;

	ASSERT(this);
	ASSERT(m_pBits);
	ASSERT(m_hBitmap);
	ASSERT(m_pBits);
	ASSERT((nColorOut >= 0) && (nColorOut < 0x100));
	ASSERT((nColorIn >= 0) && (nColorIn < 0x100));
	ASSERT(rect.top >= 0);
	ASSERT(rect.top <= rect.bottom);
	ASSERT(rect.left >= 0);
	ASSERT(rect.left <= rect.right);

	// �����ʒu�v�Z
	p = m_pBits;
	p += (m_nBMPMul * rect.top);
	p += rect.left;

	// ��(���E1dot�P�ʂł�����)
	memset(p + 1, nColorOut, (rect.right - rect.left - 2));
	p += m_nBMPMul;

	// ����
	x = rect.right - rect.left;
	x -= 2;
	rect.bottom -= 2;

	// y���[�v
	for (y=rect.top; y<rect.bottom; y++) {
		if ((y - rect.top) < 19) {
			// �ʏ�
			p[0] = (BYTE)nColorOut;
			memset(p + 1, nColorIn, x);
			p[x + 1] = (BYTE)nColorOut;
		}
		else {
			// ����
			p[10] = (BYTE)nColorOut;
			memset(p + 11, nColorIn, 41);
			p[x + 1] = (BYTE)nColorOut;
		}

		p += m_nBMPMul;
	}

	// ��ԉ��͏ȗ�(0x74:SHIFT�ɔC����)
}

//---------------------------------------------------------------------------
//
//	�L�����N�^�`��
//
//---------------------------------------------------------------------------
void FASTCALL CKeyDispWnd::DrawChar(int x, int y, int nColor, DWORD dwChar)
{
	const BYTE *pCG;
	BYTE *pBits;
	int i;
	int j;
	int nWidth;
	int nOffset;
	DWORD dwPattern;

	ASSERT(this);
	ASSERT(m_hBitmap);
	ASSERT(m_pBits);
	ASSERT((x >= 0) && (x < (int)m_nBMPWidth));
	ASSERT((y >= 0) && (y < (int)m_nBMPHeight));
	ASSERT((nColor >= 0) && (nColor < 0x100));
	ASSERT(dwChar <= 0x10000);
	ASSERT(m_pCG);

	// �t�H���g�A�h���X�Z�o
	if (dwChar < 0x100) {
		// ���p
		nWidth = 1;
		pCG = m_pCG + 0x3a800;
		pCG += (dwChar << 4);
	}
	else {
		// �S�p
		nWidth = 2;
		pCG = m_pCG;
		pCG += CalcCGAddr(dwChar);
	}

	// �r�b�g�ʒu�Z�o
	pBits = m_pBits;
	pBits += (m_nBMPMul * y);
	pBits += x;

	// �I�t�Z�b�g�Z�o
	nOffset = m_nBMPMul - (nWidth << 3);

	if (nWidth == 1) {
		// ���p
		for (i=0; i<8; i++) {
			// +1
			dwPattern = (DWORD)pCG[1];
			for (j=0; j<8; j++) {
				if (dwPattern & 0x80) {
					*pBits = (BYTE)nColor;
				}
				pBits++;
				dwPattern <<= 1;
			}
			pBits += nOffset;

			// -1
			dwPattern = (DWORD)pCG[0];
			for (j=0; j<8; j++) {
				if (dwPattern & 0x80) {
					*pBits = (BYTE)nColor;
				}
				pBits++;
				dwPattern <<= 1;
			}
			pBits += nOffset;

			// +2
			pCG += 2;
		}
	}
	else {
		// �S�p
		for (i=0; i<16; i++) {
			// +1
			dwPattern = (DWORD)pCG[1];
			for (j=0; j<8; j++) {
				if (dwPattern & 0x80) {
					*pBits = (BYTE)nColor;
				}
				pBits++;
				dwPattern <<= 1;
			}

			// -1
			dwPattern = (DWORD)pCG[0];
			for (j=0; j<8; j++) {
				if (dwPattern & 0x80) {
					*pBits = (BYTE)nColor;
				}
				pBits++;
				dwPattern <<= 1;
			}

			// +2
			pCG += 2;
			pBits += nOffset;
		}
	}
}

//---------------------------------------------------------------------------
//
//	CR�L�����N�^�`��
//
//---------------------------------------------------------------------------
void FASTCALL CKeyDispWnd::DrawCRChar(int x, int y, int nColor)
{
	BYTE *pBits;
	int i;

	ASSERT(this);
	ASSERT(m_pBits);
	ASSERT((x >= 0) && (x < (int)m_nBMPWidth));
	ASSERT((y >= 0) && (y < (int)m_nBMPHeight));
	ASSERT((nColor >= 0) && (nColor < 0x100));

	// �c��
	pBits = m_pBits;
	pBits += ((y + 7) * m_nBMPMul);
	pBits += (x + 34);
	for (i=0; i<24; i++) {
		if (i == 0) {
			memset(pBits, nColor, 5);
		}
		else {
			pBits[4] = (BYTE)nColor;
			if (i < 21) {
				pBits[0] = (BYTE)nColor;
			}
		}
		pBits += m_nBMPMul;
	}

	// ����
	pBits = m_pBits;
	pBits += ((y + 27) * m_nBMPMul);
	pBits += (x + 27);
	memset(pBits, (BYTE)nColor, 7);
	pBits += (m_nBMPMul * 3);
	memset(pBits, (BYTE)nColor, 11);

	// ���
	pBits = m_pBits;
	pBits += ((y + 28) * m_nBMPMul);
	pBits += (x + 20);
	pBits[m_nBMPMul * 0 + 0] = (BYTE)nColor;
	pBits[m_nBMPMul * 1 + 0] = (BYTE)nColor;
	pBits[m_nBMPMul * -1 + 1] = (BYTE)nColor;
	pBits[m_nBMPMul * -1 + 2] = (BYTE)nColor;
	pBits[m_nBMPMul * 2 + 1] = (BYTE)nColor;
	pBits[m_nBMPMul * 2 + 2] = (BYTE)nColor;
	pBits[m_nBMPMul * -2 + 3] = (BYTE)nColor;
	pBits[m_nBMPMul * -2 + 4] = (BYTE)nColor;
	pBits[m_nBMPMul * 3 + 3] = (BYTE)nColor;
	pBits[m_nBMPMul * 3 + 4] = (BYTE)nColor;
	pBits[m_nBMPMul * -3 + 5] = (BYTE)nColor;
	pBits[m_nBMPMul * -3 + 6] = (BYTE)nColor;
	pBits[m_nBMPMul * -2 + 6] = (BYTE)nColor;
	pBits[m_nBMPMul * -1 + 6] = (BYTE)nColor;
	pBits[m_nBMPMul * 4 + 5] = (BYTE)nColor;
	pBits[m_nBMPMul * 4 + 6] = (BYTE)nColor;
	pBits[m_nBMPMul * 3 + 6] = (BYTE)nColor;
	pBits[m_nBMPMul * 2 + 6] = (BYTE)nColor;
}

//---------------------------------------------------------------------------
//
//	�S�pCGROM�A�h���X�Z�o
//
//---------------------------------------------------------------------------
int FASTCALL CKeyDispWnd::CalcCGAddr(DWORD dwChar)
{
	DWORD dwLow;

	ASSERT(this);
	ASSERT(m_pCG);
	ASSERT((dwChar >= 0x8000) && (dwChar < 0x10000));

	// �V�t�gJIS��JIS�ϊ�(1)
	dwLow = (dwChar & 0xff);
	dwChar &= 0x3f00;
	dwChar <<= 1;
	dwChar += 0x1f00;

	// �V�t�gJIS��JIS�ϊ�(2)
	if (dwLow < 0x7f) {
		dwLow -= 0x1f;
	}
	if (dwLow >= 0x9f) {
		dwChar += 0x0100;
		dwLow -= 0x7e;
	}
	else {
		if ((dwLow >= 0x7f) && (dwLow < 0x9f)) {
			dwLow -= 0x20;
		}
	}

	// �A�h���X�Z�o(���)
	dwChar >>= 8;
	if (dwChar >= 0x30) {
		dwChar -= 7;
	}
	ASSERT(dwChar >= 0x21);
	dwChar -= 0x21;
	dwChar *= 0x5e;

	// �A�h���X�Z�o(����)
	ASSERT(dwLow >= 0x21);
	dwLow -= 0x21;

	// �A�h���X�Z�o(����)
	dwChar += dwLow;
	dwChar <<= 5;

	return dwChar;
}

//---------------------------------------------------------------------------
//
//	�p���b�g�e�[�u��
//
//---------------------------------------------------------------------------
RGBQUAD CKeyDispWnd::PalTable[0x10] = {
	{ 0x50, 0x50, 0x50, 0x00 },			//  0:�w�i�F
	{ 0xff, 0x00, 0x00, 0x00 },			//  1:��
	{ 0x00, 0x00, 0xff, 0x00 },			//  2:��
	{ 0xff, 0x00, 0xff, 0x00 },			//  3:��
	{ 0x00, 0xff, 0x00, 0x00 },			//  4:��
	{ 0xff, 0xff, 0x00, 0x00 },			//  5:���F
	{ 0x00, 0xff, 0xff, 0x00 },			//  6:��
	{ 0xff, 0xff, 0xff, 0x00 },			//  7:��

	{ 0xd0, 0xd0, 0xd0, 0x00 },			//  8:�L�[�����F
	{ 0x08, 0x08, 0x08, 0x00 },			//  9:�L�[�����F
	{ 0x00, 0x00, 0x00, 0x00 },			// 10:�L�[�O���F
	{ 0x1f, 0x1f, 0xd0, 0x00 },			// 11:�L�[�����F(����)
	{ 0xf8, 0xf8, 0xf8, 0x00 },			// 12:�L�[�����F(����)
	{ 0x00, 0x00, 0x00, 0x00 },			// 13:���g�p
	{ 0x00, 0x00, 0x00, 0x00 },			// 14:���g�p
	{ 0x00, 0x00, 0x00, 0x00 }			// 15:���g�p
};

//---------------------------------------------------------------------------
//
//	�ʒu�e�[�u��
//
//---------------------------------------------------------------------------
const RECT CKeyDispWnd::RectTable[0x75] = {
	{   0,   0,   0,   0 },
	{   3,  30,  33,  51 },
	{  32,  30,  55,  51 },
	{  54,  30,  77,  51 },
	{  76,  30,  99,  51 },
	{  98,  30, 121,  51 },
	{ 120,  30, 143,  51 },
	{ 142,  30, 165,  51 },
	{ 164,  30, 187,  51 },
	{ 186,  30, 209,  51 },
	{ 208,  30, 231,  51 },
	{ 230,  30, 253,  51 },
	{ 252,  30, 275,  51 },
	{ 274,  30, 297,  51 },
	{ 296,  30, 319,  51 },
	{ 318,  30, 356,  51 },
	{   3,  50,  41,  71 },
	{  40,  50,  63,  71 },
	{  62,  50,  85,  71 },
	{  84,  50, 107,  71 },
	{ 106,  50, 129,  71 },
	{ 128,  50, 151,  71 },
	{ 150,  50, 173,  71 },
	{ 172,  50, 195,  71 },
	{ 194,  50, 217,  71 },
	{ 216,  50, 239,  71 },
	{ 238,  50, 261,  71 },
	{ 260,  50, 283,  71 },
	{ 282,  50, 305,  71 },
	{   0,   0,   0,   0 },
	{  50,  70,  73,  91 },
	{  72,  70,  95,  91 },
	{  94,  70, 117,  91 },
	{ 116,  70, 139,  91 },
	{ 138,  70, 161,  91 },
	{ 160,  70, 183,  91 },
	{ 182,  70, 205,  91 },
	{ 204,  70, 227,  91 },
	{ 226,  70, 249,  91 },
	{ 248,  70, 271,  91 },
	{ 270,  70, 293,  91 },
	{ 292,  70, 315,  91 },
	{  58,  90,  81, 111 },
	{  80,  90, 103, 111 },
	{ 102,  90, 125, 111 },
	{ 124,  90, 147, 111 },
	{ 146,  90, 169, 111 },
	{ 168,  90, 191, 111 },
	{ 190,  90, 213, 111 },
	{ 212,  90, 235, 111 },
	{ 234,  90, 257, 111 },
	{ 256,  90, 279, 111 },
	{ 278,  90, 301, 111 },
	{ 125, 110, 189, 131 },
	{ 361,  30, 397,  51 },
	{ 431,  30, 467,  51 },
	{ 361,  50, 397,  71 },
	{ 396,  50, 432,  71 },
	{ 431,  50, 467,  71 },
	{ 361,  70, 397, 111 },
	{ 396,  70, 432,  91 },
	{ 431,  70, 467, 111 },
	{ 396,  90, 432, 111 },
	{ 472,  30, 508,  51 },
	{ 507,  30, 543,  51 },
	{ 542,  30, 578,  51 },
	{ 577,  30, 613,  51 },
	{ 472,  50, 508,  71 },
	{ 507,  50, 543,  71 },
	{ 542,  50, 578,  71 },
	{ 577,  50, 613,  71 },
	{ 472,  70, 508,  91 },
	{ 507,  70, 543,  91 },
	{ 542,  70, 578,  91 },
	{ 577,  70, 613,  91 },
	{ 472,  90, 508, 111 },
	{ 507,  90, 543, 111 },
	{ 542,  90, 578, 111 },
	{ 577,  90, 613, 131 },
	{ 472, 110, 508, 131 },
	{ 507, 110, 543, 131 },
	{ 542, 110, 578, 131 },
	{ 507,   6, 543,  27 },
	{ 542,   6, 578,  27 },
	{ 577,   6, 613,  27 },
	{  68, 110,  98, 131 },
	{  97, 110, 127, 131 },
	{ 188, 110, 218, 131 },
	{ 217, 110, 247, 131 },
	{ 246, 110, 276, 131 },
	{ 361,   6, 397,  27 },
	{ 396,   6, 432,  27 },
	{ 431,   6, 467,  27 },
	{ 472,   6, 508,  27 },
	{ 396,  30, 432,  51 },
	{  33, 110,  69, 131 },
	{ 274, 110, 310, 131 },
	{   3,   6,  33,  27 },
	{  33,   6,  63,  27 },
	{  65,   6,  95,  27 },
	{  94,   6, 124,  27 },
	{ 123,   6, 153,  27 },
	{ 152,   6, 182,  27 },
	{ 181,   6, 211,  27 },
	{ 210,   6, 240,  27 },
	{ 239,   6, 269,  27 },
	{ 268,   6, 298,  27 },
	{ 297,   6, 327,  27 },
	{ 326,   6, 356,  27 },
	{   0,   0,   0,   0 },
	{   0,   0,   0,   0 },
	{   0,   0,   0,   0 },
	{   3,  90,  59, 111 },
	{   3,  70,  51,  91 },
	{ 361, 110, 414, 131 },
	{ 413, 110, 467, 131 },
	{ 300,  90, 356, 111 }
};

//---------------------------------------------------------------------------
//
//	������e�[�u��(�ʏ�)
//
//---------------------------------------------------------------------------
LPCTSTR CKeyDispWnd::NormalTable[] = {
	NULL,
	_T("ESC"),
	_T("1"),
	_T("2"),
	_T("3"),
	_T("4"),
	_T("5"),
	_T("6"),
	_T("7"),
	_T("8"),
	_T("9"),
	_T("0"),
	_T("-"),
	_T("^"),
	_T("\\"),
	_T("BS"),

	_T("TAB"),
	_T("Q"),
	_T("W"),
	_T("E"),
	_T("R"),
	_T("T"),
	_T("Y"),
	_T("U"),
	_T("I"),
	_T("O"),
	_T("P"),
	_T("@"),
	_T("["),
	NULL,

	_T("A"),
	_T("S"),
	_T("D"),
	_T("F"),
	_T("G"),
	_T("H"),
	_T("J"),
	_T("K"),
	_T("L"),
	_T(";"),
	_T(":"),
	_T("]"),

	_T("Z"),
	_T("X"),
	_T("C"),
	_T("V"),
	_T("B"),
	_T("N"),
	_T("M"),
	_T(","),
	_T("."),
	_T("/"),
	_T(" "),
	_T(" "),

	_T("HOME"),
	_T("DEL"),
	_T("RLUP"),
	_T("RLDN"),
	_T("UNDO"),
	_T("��"),
	_T("��"),
	_T("��"),
	_T("��"),

	_T("CLR"),
	_T("/"),
	_T("*"),
	_T("-"),
	_T("7"),
	_T("8"),
	_T("9"),
	_T("+"),
	_T("4"),
	_T("5"),
	_T("6"),
	_T("="),
	_T("1"),
	_T("2"),
	_T("3"),
	_T("ENT"),
	_T("0"),
	_T(","),
	_T("."),

	_T("�L��"),
	_T("�o�^"),
	_T("HELP"),

	_T("XF1"),
	_T("XF2"),
	_T("XF3"),
	_T("XF4"),
	_T("XF5"),

	_T("����"),
	_T("۰�"),
	_T("����"),
	_T("CAPS"),
	_T("INS"),
	_T("�Ђ�"),
	_T("�S�p"),

	_T("BRK"),
	_T("CPY"),
	_T("F1"),
	_T("F2"),
	_T("F3"),
	_T("F4"),
	_T("F5"),
	_T("F6"),
	_T("F7"),
	_T("F8"),
	_T("F9"),
	_T("F10"),
	NULL,
	NULL,
	NULL,

	_T("SHIFT"),
	_T("CTRL"),
	_T("OPT.1"),
	_T("OPT.2"),

	_T("SHIFT")
};

//---------------------------------------------------------------------------
//
//	������e�[�u��(���ȁA�ʏ�)
//
//---------------------------------------------------------------------------
LPCTSTR CKeyDispWnd::KanaTable[] = {
	_T("�"),
	_T("�"),
	_T("�"),
	_T("�"),
	_T("�"),
	_T("�"),
	_T("�"),
	_T("�"),
	_T("�"),
	_T("�"),
	_T("�"),
	_T("�"),
	_T("�"),
	_T("BS"),

	_T("TAB"),
	_T("�"),
	_T("�"),
	_T("�"),
	_T("�"),
	_T("�"),
	_T("�"),
	_T("�"),
	_T("�"),
	_T("�"),
	_T("�"),
	_T("�"),
	_T("�"),
	NULL,

	_T("�"),
	_T("�"),
	_T("�"),
	_T("�"),
	_T("�"),
	_T("�"),
	_T("�"),
	_T("�"),
	_T("�"),
	_T("�"),
	_T("�"),
	_T("�"),

	_T("�"),
	_T("�"),
	_T("�"),
	_T("�"),
	_T("�"),
	_T("�"),
	_T("�"),
	_T("�"),
	_T("�"),
	_T("�"),
	_T("�")
};

//---------------------------------------------------------------------------
//
//	������e�[�u��(���ȁASHIFT)
//
//---------------------------------------------------------------------------
LPCTSTR CKeyDispWnd::KanaShiftTable[] = {
	_T("�"),
	_T("�"),
	_T("�"),
	_T("�"),
	_T("�"),
	_T("�"),
	_T("�"),
	_T("�"),
	_T("�"),
	_T("�"),
	_T("�"),
	_T("�"),
	_T("�"),
	_T("BS"),

	_T("TAB"),
	_T("�"),
	_T("�"),
	_T("�"),
	_T("�"),
	_T("�"),
	_T("�"),
	_T("�"),
	_T("�"),
	_T("�"),
	_T("�"),
	_T("�"),
	_T("�"),
	NULL,

	_T("�"),
	_T("�"),
	_T("�"),
	_T("�"),
	_T("�"),
	_T("�"),
	_T("�"),
	_T("�"),
	_T("�"),
	_T("�"),
	_T("�"),
	_T("�"),

	_T("�"),
	_T("�"),
	_T("�"),
	_T("�"),
	_T("�"),
	_T("�"),
	_T("�"),
	_T("�"),
	_T("�"),
	_T("�"),
	_T(" ")
};

//---------------------------------------------------------------------------
//
//	������e�[�u��(�L��)
//
//---------------------------------------------------------------------------
LPCTSTR CKeyDispWnd::MarkTable[] = {
	_T("!"),
	_T("\x22"),
	_T("#"),
	_T("$"),
	_T("%"),
	_T("&"),
	_T("'"),
	_T("("),
	_T(")"),
	_T(" "),
	_T("="),
	_T("~"),
	_T("|")
};

//---------------------------------------------------------------------------
//
//	������e�[�u��(���̑�)
//
//---------------------------------------------------------------------------
LPCTSTR CKeyDispWnd::AnotherTable[] = {
	_T("q"),
	_T("w"),
	_T("e"),
	_T("r"),
	_T("t"),
	_T("y"),
	_T("u"),
	_T("i"),
	_T("o"),
	_T("p"),
	_T("`"),
	_T("{"),
	NULL,

	_T("a"),
	_T("s"),
	_T("d"),
	_T("f"),
	_T("g"),
	_T("h"),
	_T("j"),
	_T("k"),
	_T("l"),
	_T("+"),
	_T("*"),
	_T("}"),

	_T("z"),
	_T("x"),
	_T("c"),
	_T("v"),
	_T("b"),
	_T("n"),
	_T("m"),
	_T("<"),
	_T(">"),
	_T("?"),
	_T("_")
};

//===========================================================================
//
//	�\�t�g�E�F�A�L�[�{�[�h�E�B���h�E
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	�R���X�g���N�^
//
//---------------------------------------------------------------------------
CSoftKeyWnd::CSoftKeyWnd()
{
	CFrmWnd *pWnd;

	// �E�B���h�E�p�����[�^��`
	m_dwID = MAKEID('S', 'K', 'E', 'Y');
	::GetMsg(IDS_SWND_SOFTKEY, m_strCaption);

	// ��ʃT�C�Y
	m_nWidth = 88;
	m_nHeight = 10;

	// �L�[�{�[�h�擾
	m_pKeyboard = (Keyboard*)::GetVM()->SearchDevice(MAKEID('K', 'E', 'Y', 'B'));
	ASSERT(m_pKeyboard);

	// �C���v�b�g�擾
	pWnd = (CFrmWnd*)AfxGetApp()->m_pMainWnd;
	ASSERT(pWnd);
	m_pInput = pWnd->GetInput();

	// ���[�N������
	m_nSoftKey = 0;
	m_pDispWnd = NULL;
}

//---------------------------------------------------------------------------
//
//	���b�Z�[�W �}�b�v
//
//---------------------------------------------------------------------------
BEGIN_MESSAGE_MAP(CSoftKeyWnd, CSubWnd)
	ON_WM_CREATE()
	ON_WM_DESTROY()
	ON_WM_ACTIVATE()
	ON_MESSAGE(WM_APP, OnApp)
END_MESSAGE_MAP()

//---------------------------------------------------------------------------
//
//	�E�B���h�E�쐬
//
//---------------------------------------------------------------------------
int CSoftKeyWnd::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	CSize size;
	CRect rect;
	UINT uID;
	Keyboard::keyboard_t kbd;

	ASSERT(this);

	// ��{�N���X
	if (CSubWnd::OnCreate(lpCreateStruct) != 0) {
		return -1;
	}

	// �X�e�[�^�X�o�[�쐬
	uID = 0;
	m_StatusBar.Create(this);
	m_StatusBar.SetIndicators(&uID, 1);
	m_StatusBar.SetPaneInfo(0, 0, SBPS_STRETCH, 0);

	// ���A�����v�Z(�T�C�Y�̓t�H���g�ɂ��Ȃ��Œ�l)
	size = m_StatusBar.CalcFixedLayout(TRUE, TRUE);
	rect.left = 0;
	rect.top = 0;
	rect.right = 616;
	rect.bottom = 140 + size.cy;

	// �œK�ȃE�B���h�E�T�C�Y���Z�o���A�T�C�Y�ύX
	CalcWindowRect(&rect);
	SetWindowPos(&wndTop, 0, 0, rect.Width(), rect.Height(), SWP_NOMOVE);

	// �X�e�[�^�X�o�[�ړ�
	GetClientRect(&rect);
	rect.bottom -= size.cy;
	m_StatusBar.MoveWindow(0, rect.bottom, rect.Width(), size.cy);

	// BMP�E�B���h�E
	m_pDispWnd = new CKeyDispWnd;
	m_pDispWnd->Create(NULL, NULL, WS_CHILD | WS_VISIBLE,
						rect, this, 0, NULL);

	// �����ݒ�(����\������A���݂̏�Ԃ����̂܂ܕ\������)
	m_pKeyboard->GetKeyboard(&kbd);
	Analyze(&kbd);
	m_pDispWnd->SetKey(kbd.status);

	// ����
	return 0;
}

//---------------------------------------------------------------------------
//
//	�E�B���h�E�폜
//
//---------------------------------------------------------------------------
void CSoftKeyWnd::OnDestroy()
{
	// �G�~�����[�V�������̃L�[������΁ABreak
	// ������VM�������Ă���ꍇ�̂�
	if (m_nSoftKey != 0) {
		if (::GetVM()) {
			ASSERT(m_pKeyboard);
			m_pKeyboard->BreakKey(m_nSoftKey);
			m_nSoftKey = 0;
		}
	}

	// �q�E�B���h�E�Ƃ̃����N��؂�
	m_pDispWnd = NULL;

	// ��{�N���X
	CSubWnd::OnDestroy();
}

//---------------------------------------------------------------------------
//
//	�A�N�e�B�x�[�g
//
//---------------------------------------------------------------------------
void CSoftKeyWnd::OnActivate(UINT nState, CWnd *pWnd, BOOL bMinimized)
{
	// ��{�N���X
	CSubWnd::OnActivate(nState, pWnd, bMinimized);

	// �|�b�v�A�b�v�E�B���h�E�Ȃ�
	if (m_bPopup) {
		ASSERT(m_pInput);

		// �t�H�[�J�X���A���͂����B��t�H�[�J�X���͓��͋����Ȃ�
		if (nState == WA_INACTIVE) {
			// ���͋����Ȃ�
			m_pInput->Activate(FALSE);
		}
		else {
			// ���͋�����
			m_pInput->Activate(TRUE);
		}
	}
}

//---------------------------------------------------------------------------
//
//	���[�U
//	���q�E�B���h�E����̒ʒm
//
//---------------------------------------------------------------------------
LONG CSoftKeyWnd::OnApp(UINT uParam, LONG lParam)
{
	CString strStatus;
	LPCTSTR lpszKey;
	Keyboard::keyboard_t kbd;

	ASSERT(this);
	ASSERT(uParam <= 0x73);

	// �L�[�{�[�h�擾
	ASSERT(m_pKeyboard);
	m_pKeyboard->GetKeyboard(&kbd);

	// �U�蕪��
	switch (lParam) {
		// ���{�^��������
		case WM_LBUTTONDOWN:
			// ���ɃG�~�����[�V�������̃L�[������΁A����
			if (m_nSoftKey != 0) {
				m_pKeyboard->BreakKey(m_nSoftKey);
				m_nSoftKey = 0;
			}

			// ���̃L�[��������Ă���΁A����
			if (!kbd.status[uParam]) {
				m_pKeyboard->MakeKey(uParam);

				// SHIFT,CTRL�ȊO�ł���΋L�^
				if ((uParam != 0x70) && (uParam != 0x71)) {
					m_nSoftKey = uParam;
				}
			}
			else {
				// ������Ă���BSHIFT,CTRL�ł���Η����Ă��
				if ((uParam == 0x70) || (uParam == 0x71)) {
					m_pKeyboard->BreakKey(uParam);
				}
				else {
					// ������������
					m_pKeyboard->MakeKey(uParam);

					// SHIFT,CTRL�ȊO�ł���΋L�^
					if ((uParam != 0x70) && (uParam != 0x71)) {
						m_nSoftKey = uParam;
					}
				}
			}
			break;

		// ���{�^��������
		case WM_LBUTTONUP:
			// ���ɃG�~�����[�V�������̃L�[������΁A����
			if (m_nSoftKey != 0) {
				m_pKeyboard->BreakKey(m_nSoftKey);
				m_nSoftKey = 0;
			}
			break;

		// �E�{�^��������
		case WM_RBUTTONDOWN:
			// ���ɃG�~�����[�V�������̃L�[������΁A����
			if (m_nSoftKey != 0) {
				m_pKeyboard->BreakKey(m_nSoftKey);
				m_nSoftKey = 0;
			}

			// ���̃L�[��������Ă���΁A����
			if (!kbd.status[uParam]) {
				m_pKeyboard->MakeKey(uParam);
			}
			else {
				// ������Ă���B�����Ă��
				m_pKeyboard->BreakKey(uParam);
				if (m_nSoftKey == uParam) {
					m_nSoftKey = 0;
				}
			}
			break;

		// �E�{�^��������
		case WM_RBUTTONUP:
			break;

		// �}�E�X�ړ�
		case WM_MOUSEMOVE:
			strStatus.Empty();
			if (uParam != 0) {
				strStatus.Format(_T("Key%02X  "), uParam);
				lpszKey = m_pInput->GetKeyName(uParam);
				ASSERT(lpszKey);
				strStatus += lpszKey;
			}
			m_StatusBar.SetPaneText(0, strStatus);
			break;

		// ���̑�(���肦�Ȃ�)
		default:
			ASSERT(FALSE);
			break;
	}

	return 0;
}

//---------------------------------------------------------------------------
//
//	���t���b�V��
//
//---------------------------------------------------------------------------
void FASTCALL CSoftKeyWnd::Refresh()
{
	Keyboard::keyboard_t kbd;

	ASSERT(this);

	// ��ԃ`�F�b�N
	if (!m_pDispWnd || !m_pKeyboard) {
		return;
	}

	// �L�[�{�[�h�f�[�^�擾
	m_pKeyboard->GetKeyboard(&kbd);

	// �f�[�^��́A�ݒ�
	Analyze(&kbd);

	// �\��
	m_pDispWnd->Refresh(kbd.status);
}

//---------------------------------------------------------------------------
//
//	�L�[�{�[�h�f�[�^���
//
//---------------------------------------------------------------------------
void FASTCALL CSoftKeyWnd::Analyze(Keyboard::keyboard_t *pKbd)
{
	int i;
	DWORD dwLed;
	UINT nMode;

	ASSERT(this);
	ASSERT(pKbd);

	// �V�t�g���[�h�l�쐬�A�ʒm
	nMode = pKbd->led & 0x0f;
	if (pKbd->status[0x70]) {
		nMode |= 0x80;
	}
	if (pKbd->status[0x71]) {
		nMode |= 0x40;
	}
	m_pDispWnd->SetShiftMode(nMode);

	// SHIFT�L�[�ڐA
	pKbd->status[0x74] = pKbd->status[0x70];

	// LED�L�[�����Ԃ𓾂�ꍇ
	dwLed = pKbd->led;
	for (i=0; i<7; i++) {
		// LED���_�����Ă����ON�A�����łȂ����OFF
		if (dwLed & 1) {
			pKbd->status[i + 0x5a] = TRUE;
		}
		else {
			pKbd->status[i + 0x5a] = FALSE;
		}
		// ����
		dwLed >>= 1;
	}
}

#endif	// _WIN32
