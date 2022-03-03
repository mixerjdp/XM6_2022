//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 �o�h�D(ytanaka@ipc-tokai.or.jp)
//	[ MFC Info ]
//
//---------------------------------------------------------------------------

#if defined(_WIN32)

#include "os.h"
#include "xm6.h"
#include "vm.h"
#include "schedule.h"
#include "sasi.h"
#include "scsi.h"
#include "fdd.h"
#include "fdi.h"
#include "rtc.h"
#include "memory.h"
#include "fileio.h"
#include "config.h"
#include "mfc_frm.h"
#include "mfc_com.h"
#include "mfc_stat.h"
#include "mfc_sch.h"
#include "mfc_cfg.h"
#include "mfc_res.h"
#include "mfc_info.h"

//===========================================================================
//
//	Info
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	�R���X�g���N�^
//
//---------------------------------------------------------------------------
CInfo::CInfo(CFrmWnd *pWnd, CStatusBar *pBar) : CComponent(pWnd)
{
	int nPane;

	// �R���|�[�l���g�p�����[�^
	m_dwID = MAKEID('I', 'N', 'F', 'O');
	m_strDesc = _T("Info Center");

	// �L���v�V����
	m_strRun.Empty();
	m_strStop.Empty();
	m_szVM[0] = _T('\0');
	m_szVMFull[0] = _T('\0');
	m_szMO[0] = _T('\0');
	m_szMOFull[0] = _T('\0');
	m_szCD[0] = _T('\0');
	m_szCDFull[0] = _T('\0');
	m_bRun = FALSE;
	m_nParcent = -1;
	m_bCount = FALSE;
	m_dwTime = 0;
	m_dwTick = 0;
	m_pSch = NULL;
	m_pScheduler = NULL;
	m_pSASI = NULL;
	m_pSCSI = NULL;

	// ���
	m_bInfo = FALSE;
	m_bPower = FALSE;
	m_dwInfo = 0;
	m_strInfo.Empty();
	m_szInfo[0] = _T('\0');

	// �X�e�[�^�X
	for (nPane=0; nPane<PaneMax; nPane++) {
		m_colStatus[nPane] = (COLORREF)-1;
		m_szStatus[nPane][0] = _T('\0');
	}
	m_bFloppyLED = FALSE;
	m_bPowerLED = FALSE;
	m_bCaptionInfo = FALSE;
	m_pFDD = NULL;
	m_pRTC = NULL;
	m_dwNumber = 0;
	m_dwDiskID = 0;
	m_dwDiskTime = DiskTypeTime;

	// �X�e�[�^�X�o�[
	ASSERT(pBar);
	m_pStatusBar = pBar;

	// �X�e�[�^�X�r���[
	m_pStatusView = NULL;
}

//---------------------------------------------------------------------------
//
//	������
//
//---------------------------------------------------------------------------
BOOL FASTCALL CInfo::Init()
{
	CString strIdle;

	ASSERT(this);

	// ��{�N���X
	if (!CComponent::Init()) {
		return FALSE;
	}

	// �L���v�V����
	::GetMsg(IDS_CAPTION_RUN, m_strRun);
	::GetMsg(IDS_CAPTION_STOP, m_strStop);
	m_pSch = (CScheduler*)SearchComponent(MAKEID('S', 'C', 'H', 'E'));
	ASSERT(m_pSch);
	m_pScheduler = (Scheduler*)::GetVM()->SearchDevice(MAKEID('S', 'C', 'H', 'E'));
	ASSERT(m_pScheduler);
	m_pSASI = (SASI*)::GetVM()->SearchDevice(MAKEID('S', 'A', 'S', 'I'));
	ASSERT(m_pSASI);
	m_pSCSI = (SCSI*)::GetVM()->SearchDevice(MAKEID('S', 'C', 'S', 'I'));
	ASSERT(m_pSCSI);

	// ���
	::SetInfoMsg(m_szInfo, TRUE);
	::GetMsg(AFX_IDS_IDLEMESSAGE, strIdle);
	if (strIdle.GetLength() < 0x100) {
		_tcscpy(m_szInfo, (LPCTSTR)strIdle);
	}
	::GetMsg(IDS_POWEROFF, m_strPower);
	m_bPower = ::GetVM()->IsPower();

	// �X�e�[�^�X
	m_pFDD = (FDD*)::GetVM()->SearchDevice(MAKEID('F', 'D', 'D', ' '));
	ASSERT(m_pFDD);
	m_pRTC = (RTC*)::GetVM()->SearchDevice(MAKEID('R', 'T', 'C', ' '));
	ASSERT(m_pRTC);

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	�N���[���A�b�v
//
//---------------------------------------------------------------------------
void FASTCALL CInfo::Cleanup()
{
	ASSERT(this);


	// ��{�N���X
	CComponent::Cleanup();
}

//---------------------------------------------------------------------------
//
//	�ݒ�K�p
//
//---------------------------------------------------------------------------
void FASTCALL CInfo::ApplyCfg(const Config* pConfig)
{
	ASSERT(this);
	ASSERT(pConfig);

	// �X�e�[�^�X
	m_bFloppyLED = pConfig->floppy_led;
	m_bPowerLED = pConfig->power_led;
	m_bCaptionInfo = pConfig->caption_info;
}

//---------------------------------------------------------------------------
//
//	���쐧��
//
//---------------------------------------------------------------------------
void FASTCALL CInfo::Enable(BOOL bEnable)
{
	CConfig *pConfig;
	Config cfg;
	int i;
	int x;
	int y;
	BITMAPINFOHEADER *bmi;
	Memory *pMemory;
	const BYTE *pCG;
	const BYTE *pChr;
	BYTE *pBits;
	BYTE byteFont;

	// �L���ɂ���ꍇ(�N��������Ӗ�����)
	if (bEnable) {
		// �R���t�B�O�}�l�[�W�����擾
		pConfig = (CConfig*)SearchComponent(MAKEID('C', 'F', 'G', ' '));
		ASSERT(pConfig);

		// �R���t�B�O�f�[�^���擾
		pConfig->GetConfig(&cfg);

		// �d��ON�N���AOFF�N���ŕ�����
		if (cfg.power_off) {
			// �d��OFF����
			m_bPower = FALSE;
			m_bRun = FALSE;
			m_bCount = TRUE;
		}
		else {
			// �d��ON����
			m_bPower = TRUE;
			m_bRun = TRUE;
			m_bCount = FALSE;
		}
		m_dwTick = ::GetTickCount();

		// �h���C�u�p�r�b�g�}�b�v�쐬
		pMemory = (Memory*)::GetVM()->SearchDevice(MAKEID('M', 'E', 'M', ' '));
		ASSERT(pMemory);
		pCG = pMemory->GetCG();
		ASSERT(pCG);

		// �h���C�u���[�v
		for (i=0; i<2; i++) {
			// �T�C�Y�`�F�b�N
			ASSERT((sizeof(BITMAPINFOHEADER) + 8 * 8 * 3) <= sizeof(m_bmpDrive[i]));

			// �N���A
			memset(m_bmpDrive[i], 0, sizeof(m_bmpDrive[i]));

			// �r�b�g�}�b�v�w�b�_�ݒ�
			bmi = (BITMAPINFOHEADER*)&m_bmpDrive[i][0];
			bmi->biSize = sizeof(BITMAPINFOHEADER);
			bmi->biWidth = 8;
			bmi->biHeight = -8;
			bmi->biPlanes = 1;
			bmi->biBitCount = 24;
			bmi->biCompression = BI_RGB;
			bmi->biSizeImage = 8 * 8 * 3;

			// �A�h���X�ݒ�
			pChr = pCG + 0x3a000 + (('0' + i) << 3);
			pBits = &m_bmpDrive[i][sizeof(BITMAPINFOHEADER)];

			// x, y���[�v
			for (y=0; y<8; y++) {
				// CGROM����f�[�^�擾
				byteFont = pChr[y ^ 1];

				// 1�r�b�g���ƂɌ���
				for (x=0; x<8; x++) {
					if (byteFont & 0x80) {
						pBits[0] = 208;
						pBits[1] = 208;
						pBits[2] = 208;
					}
					byteFont <<= 1;
					pBits += 3;
				}
			}
		}
	}

	// ��{�N���X
	CComponent::Enable(bEnable);
}

//===========================================================================
//
//	�L���v�V����
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	�L���v�V�������Z�b�g
//
//---------------------------------------------------------------------------
void FASTCALL CInfo::ResetCaption()
{
	CString strCap;

	ASSERT(this);
	ASSERT(m_pSch);

	// ���݂̏�ԂƔ��]
	m_bRun = !m_pSch->IsEnable();

	// �X�V�A���S���Y���̋t��ݒ�
	if (m_bRun) {
		m_bCount = FALSE;
		m_nParcent = -1;
	}
	else {
		m_bCount = TRUE;
		m_nParcent = -1;
	}

	// ���݂̏�Ԃ��烁�b�Z�[�W�����
	if (m_pSch->IsEnable()) {
		strCap = m_strRun;
	}
	else {
		strCap = m_strStop;
	}

	// �ݒ�
	m_pFrmWnd->SetWindowText(strCap);
	if (m_pStatusView) {
		m_pStatusView->SetCaptionString(strCap);
	}
}

//---------------------------------------------------------------------------
//
//	�L���v�V�����X�V
//
//---------------------------------------------------------------------------
void FASTCALL CInfo::UpdateCaption()
{
	BOOL bParcent;
	BOOL bVM;
	BOOL bMO;
	BOOL bCD;

	ASSERT(this);

	// ���`�F�b�N
	bParcent = CheckParcent();

	// VM�`�F�b�N
	bVM = CheckVM();

	// MO�`�F�b�N
	bMO = CheckMO();

	// CD�`�F�b�N
	bCD = CheckCD();

	// �ǂꂩ�قȂ��Ă�����A�L���v�V�����ݒ�
	if (bParcent || bVM || bMO || bCD) {
		SetCaption();
	}
}

//---------------------------------------------------------------------------
//
//	���`�F�b�N
//
//---------------------------------------------------------------------------
BOOL FASTCALL CInfo::CheckParcent()
{
	BOOL bRun;
	DWORD dwDiff;
	DWORD dwTick;
	int nParcent;

	ASSERT(this);
	ASSERT(m_pSch);
	ASSERT(m_pScheduler);

	// �O��̏�Ԃ��o�b�N�A�b�v
	bRun = m_bRun;

	// ����̓����Ԃ��擾
	m_bRun = m_pSch->IsEnable();

	// ��~�ɂȂ��Ă���Γ��ʏ���
	if (!m_bRun) {
		// �O�����~�Ȃ�
		if (!bRun) {
			// �v������ɂȂ��Ă���Ή��낵�āA�X�V����
			if (m_bCount) {
				m_bCount = FALSE;
				return TRUE;
			}
			return FALSE;
		}

		// �v���Ȃ�
		m_bCount = FALSE;

		// ���\���Ȃ�
		m_nParcent = -1;

		// �ύX����
		return TRUE;
	}

	// �v���J�n���Ă��Ȃ���Γ��ʏ���(STOP��RUN�͂����ɓ���)
	if (!m_bCount || !bRun) {
		// �v���J�n
		m_bCount = TRUE;
		m_dwTick = ::GetTickCount();
		m_dwTime = m_pScheduler->GetTotalTime();

		// ���\���Ȃ�
		m_nParcent = -1;

		// �ύX����
		return TRUE;
	}

	// RUN��RUN�̏ꍇ�̂�
	ASSERT(m_bCount);
	ASSERT(bRun);
	ASSERT(m_bRun);

	// ���ԍ���������
	dwDiff = ::GetTickCount();
	dwDiff -= m_dwTick;
	if (m_nParcent >= 0) {
		// ���Ɂ��\�����̏ꍇ
		if (dwDiff < CapTimeLong) {
			// �ύX�Ȃ�
			return FALSE;
		}
	}
	else {
		// �܂����\�����Ă��Ȃ��ꍇ
		if (dwDiff < CapTimeShort) {
			// �ύX�Ȃ�
			return FALSE;
		}
	}

	// ���ԍ��A���s�^�C�����𓾂�
	dwTick = ::GetTickCount();
	m_dwTime = m_pScheduler->GetTotalTime() - m_dwTime;
	m_dwTick = dwTick - m_dwTick;

	// ���Z�ɂ�聓���Z�o
	if ((m_dwTime == 0) || (m_dwTick == 0)) {
		nParcent = 0;
	}
	else {
		// VM����0.5us�P��
		nParcent = (int)m_dwTime;
		nParcent /= (int)m_dwTick;
		if (nParcent > 0) {
			nParcent /= 2;
		}
	}

	// �ăZ�b�g
	m_dwTime = m_pScheduler->GetTotalTime();
	m_dwTick = dwTick;

	// �����H(4��5��)
	if ((nParcent % 10) >= 5) {
		nParcent /= 10;
		nParcent++;
	}
	else {
		nParcent /= 10;
	}

	// �قȂ��Ă���΍X�V���āATRUE
	if (m_nParcent != nParcent) {
		m_nParcent = nParcent;
		return TRUE;
	}

	// �O��Ɠ����BFALSE
	return FALSE;
}

//---------------------------------------------------------------------------
//
//	VM�t�@�C���`�F�b�N
//
//---------------------------------------------------------------------------
BOOL FASTCALL CInfo::CheckVM()
{
	Filepath path;
	LPCTSTR lpszPath;
	LPCTSTR lpszFileExt;

	ASSERT(this);
	ASSERT(m_pSASI);

	// VM�p�X�擾
	::GetVM()->GetPath(path);
	lpszPath = path.GetPath();

	// �t���ƃp�X�����񂪈�v���Ă��邩�ǂ������ׂ�
	if (_tcscmp(lpszPath, m_szVMFull) == 0) {
		// �ύX�Ȃ�
		return FALSE;
	}

	// �t���ɑS�ăR�s�[
	_tcscpy(m_szVMFull, lpszPath);

	// �t�@�C�����{�g���q�̂ݎ��o��
	lpszFileExt = path.GetFileExt();

	// �X�V
	_tcscpy(m_szVM, lpszFileExt);

	// �ύX����
	return TRUE;
}

//---------------------------------------------------------------------------
//
//	MO�t�@�C���`�F�b�N
//
//---------------------------------------------------------------------------
BOOL FASTCALL CInfo::CheckMO()
{
	Filepath path;
	LPCTSTR lpszPath;
	LPCTSTR lpszFileExt;

	ASSERT(this);
	ASSERT(m_pSASI);

	// MO�p�X�擾
	m_pSASI->GetPath(path);
	lpszPath = path.GetPath();

	// �t���ƃp�X�����񂪈�v���Ă��邩�ǂ������ׂ�
	if (_tcscmp(lpszPath, m_szMOFull) == 0) {
		// �ύX�Ȃ�
		return FALSE;
	}

	// �t���ɑS�ăR�s�[
	_tcscpy(m_szMOFull, lpszPath);

	// �t�@�C�����{�g���q�̂ݎ��o��
	lpszFileExt = path.GetFileExt();

	// �X�V
	_tcscpy(m_szMO, lpszFileExt);

	// �ύX����
	return TRUE;
}

//---------------------------------------------------------------------------
//
//	CD�t�@�C���`�F�b�N
//
//---------------------------------------------------------------------------
BOOL FASTCALL CInfo::CheckCD()
{
	Filepath path;
	LPCTSTR lpszPath;
	LPCTSTR lpszFileExt;

	ASSERT(this);
	ASSERT(m_pSCSI);

	// CD�p�X�擾
	m_pSCSI->GetPath(path, FALSE);
	lpszPath = path.GetPath();

	// �t���ƃp�X�����񂪈�v���Ă��邩�ǂ������ׂ�
	if (_tcscmp(lpszPath, m_szCDFull) == 0) {
		// �ύX�Ȃ�
		return FALSE;
	}

	// �t���ɑS�ăR�s�[
	_tcscpy(m_szCDFull, lpszPath);

	// �t�@�C�����{�g���q�̂ݎ��o��
	lpszFileExt = path.GetFileExt();

	// �X�V
	_tcscpy(m_szCD, lpszFileExt);

	// �ύX����
	return TRUE;
}

//---------------------------------------------------------------------------
//
//	�L���v�V�����ݒ�
//
//---------------------------------------------------------------------------
void FASTCALL CInfo::SetCaption()
{
	CString strCap;
	CString strSub;

	ASSERT(this);
	ASSERT(m_pFrmWnd);

	// ���쒆 or ��~
	if (m_bRun) {
		// ���쒆
		strCap = m_strRun;
	}
	else {
		// ��~��
		strCap = m_strStop;
	}

	// �ݒ�(�L���v�V�����֏��\�����Ȃ��ꍇ)
	if (m_bCaptionInfo == FALSE) {
		strCap = _T("XM6");

		// �L���v�V����
		m_pFrmWnd->SetWindowText(strCap);

		// �X�e�[�^�X�r���[
		if (m_pStatusView) {
			m_pStatusView->SetCaptionString(strCap);
		}
	}

	// %�\��
	if (m_nParcent >= 0) {
		strSub.Format(_T(" - %3d%%"), m_nParcent);
		strCap += strSub;
	}

	// VM
	if (_tcslen(m_szVM) > 0) {
		strSub.Format(_T(" [%s] "), m_szVM);
		strCap += strSub;
	}

	// MO
	if (_tcslen(m_szMO) > 0) {
		strSub.Format(_T(" (%s) "), m_szMO);
		strCap += strSub;
	}

	// CD
	if (_tcslen(m_szCD) > 0) {
		strSub.Format(_T(" {%s} "), m_szCD);
		strCap += strSub;
	}

	//  �ݒ�(�L���v�V�����֏��\������ꍇ)
	if (m_bCaptionInfo) {
		// �L���v�V����
		m_pFrmWnd->SetWindowText(strCap);

		// �X�e�[�^�X�r���[
		if (m_pStatusView) {
			m_pStatusView->SetCaptionString(strCap);
		}
	}
}

//===========================================================================
//
//	���
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	���ݒ�
//
//---------------------------------------------------------------------------
void FASTCALL CInfo::SetInfo(CString& strInfo)
{
	ASSERT(this);

	// ���b�Z�[�W�L��
	m_strInfo = strInfo;

	// �t���O�A�b�v�A���ԋL��
	m_bInfo = TRUE;
	m_dwInfo = ::GetTickCount();

	// �X�e�[�^�X�o�[�֐ݒ�
	ASSERT(m_pStatusBar);
	m_pStatusBar->SetPaneText(0, m_strInfo, TRUE);

	// �X�e�[�^�X�r���[�֐ݒ�
	if (m_pStatusView) {
		m_pStatusView->SetInfoString(m_strInfo);
	}
}

//---------------------------------------------------------------------------
//
//	�ʏ탁�b�Z�[�W�ݒ�
//
//---------------------------------------------------------------------------
void FASTCALL CInfo::SetMessageString(const CString& strMessage) const
{
	LPCTSTR lpszMessage;

	// LPCTSTR�֕ϊ�
	lpszMessage = (LPCTSTR)strMessage;

	// const����𔲂��邽�߁AC�`���̊֐����o�R
	::SetInfoMsg(lpszMessage, FALSE);
}

//---------------------------------------------------------------------------
//
//	���X�V
//
//---------------------------------------------------------------------------
void FASTCALL CInfo::UpdateInfo()
{
	BOOL bPower;
	DWORD dwDiff;
	CString strText;

	ASSERT(this);

	// �d����Ԃ��`�F�b�N
	bPower = ::GetVM()->IsPower();
	if (m_bPower && !bPower) {
		// �d����ON��OFF�ɑJ�ڂ����B�������b�Z�[�W
		SetInfo(m_strPower);
	}
	// �d����Ԃ���ɍX�V
	m_bPower = bPower;

	// ���L���t���OOFF�Ȃ�
	if (!m_bInfo) {
		// �������Ȃ�
		return;
	}

	// ���Ԍv���B2000ms�ŏ���
	dwDiff = ::GetTickCount();
	dwDiff -= m_dwInfo;
	if (dwDiff < InfoTime) {
		// �\����
		return;
	}

	// ���L���t���OOFF
	m_bInfo = FALSE;

	// �ʏ탁�b�Z�[�W�𕜌�
	strText = m_szInfo;

	// �X�e�[�^�X�o�[�֐ݒ�
	ASSERT(m_pStatusBar);
	m_pStatusBar->SetPaneText(0, strText, TRUE);

	// �X�e�[�^�X�r���[�֐ݒ�
	if (m_pStatusView) {
		strText.Empty();
		m_pStatusView->SetInfoString(strText);
	}
}

//===========================================================================
//
//	�X�e�[�^�X
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	�X�e�[�^�X���Z�b�g
//
//---------------------------------------------------------------------------
void FASTCALL CInfo::ResetStatus()
{
	int nPane;

	ASSERT(this);

	// �y�C�������ׂăN���A
	for (nPane=0; nPane<PaneMax; nPane++) {
		// �s��F
		m_colStatus[nPane] = (COLORREF)-1;

		// �\���e�L�X�g�Ȃ�
		m_szStatus[nPane][0] = _T('\0');
	}
}

//---------------------------------------------------------------------------
//
//	�X�e�[�^�X�X�V
//
//---------------------------------------------------------------------------
void FASTCALL CInfo::UpdateStatus()
{
	COLORREF colStatus[PaneMax];
	TCHAR szStatus[PaneMax][_MAX_PATH];
	int nPane;
	BOOL bDraw;
	BOOL bAll;
	CString strNumber;

	ASSERT(this);
	ASSERT(m_pStatusBar);

	// VM���b�N
	::LockVM();

	// �e�L�X�g�ƃJ���[���쐬
	ASSERT(PaneMax == 5);
	colStatus[0] = StatusFloppy(szStatus[0], 0);
	colStatus[1] = StatusFloppy(szStatus[1], 1);
	colStatus[2] = StatusHardDisk(szStatus[2]);
	colStatus[3] = StatusTimer(szStatus[3]);
	colStatus[4] = StatusPower(szStatus[4]);

	// VM�A�����b�N
	::UnlockVM();

	// �i���o������쐬(����̍X�V�ŁA�قȂ镶������쐬)
	strNumber.Format(_T("%08X"), m_dwNumber);

	// ��r���[�v�ƕ`��
	bAll = FALSE;
	for (nPane=0; nPane<5; nPane++) {
		// ��r���A�`�悷�邩�ǂ��������߂�
		bDraw = FALSE;
		if (m_colStatus[nPane] != colStatus[nPane]) {
			bDraw = TRUE;
		}
		if (_tcscmp(m_szStatus[nPane], szStatus[nPane]) != 0) {
			bDraw = TRUE;
		}

		// �`�悷��K�v���Ȃ���Ύ���
		if (!bDraw) {
			continue;
		}

		// �R�s�[
		m_colStatus[nPane] = colStatus[nPane];
		_tcscpy(m_szStatus[nPane], szStatus[nPane]);

		// �`��
		m_pStatusBar->SetPaneText(nPane + 1, strNumber, TRUE);

		// �X�e�[�^�X�r���[
		if (m_pStatusView) {
			m_pStatusView->DrawStatus(nPane);
		}

		// �t���OUp
		bAll = TRUE;
	}

	// �i���o�X�V
	if (bAll) {
		m_dwNumber++;
	}
}

//---------------------------------------------------------------------------
//
//	�X�e�[�^�XFD
//
//---------------------------------------------------------------------------
COLORREF FASTCALL CInfo::StatusFloppy(LPTSTR lpszText, int nDrive) const
{
	COLORREF colStatus;
	int nStatus;
	BOOL bPower;
	FDD::drv_t drv;
	char name[60];
	LPCTSTR lpszName;

	ASSERT(this);
	ASSERT(lpszText);
	ASSERT((nDrive == 0) || (nDrive == 1));
	ASSERT(m_pFDD);
	ASSERT(m_pSch);

	// �������AFDD�f�[�^�𓾂�
	bPower = ::GetVM()->IsPower();
	lpszText[0] = _T('\0');
	colStatus = RGB(1, 1, 1);
	nStatus = m_pFDD->GetStatus(nDrive);

	// �_�Œ���
	if (nStatus & FDST_BLINK) {
		// �_�Œ��B�_�Ɩł̂ǂ��炩
		if ((nStatus & FDST_CURRENT) && bPower) {
			// ��
			colStatus = RGB(15, 159, 15);
		}
		else {
			// ��
			colStatus = RGB(0, 0, 0);
		}
	}

	// �}���܂��͌�}����
	if (nStatus & FDST_INSERT) {
		// �D
		colStatus = RGB(95, 95, 95);

		// �f�B�X�N���擾
		m_pFDD->GetName(nDrive, name);
		lpszName = A2CT(name);
		_tcscpy(lpszText, lpszName);

		// �A�N�Z�X��
		if (m_bFloppyLED) {
			// ���[�^ON&�Z���N�g�Ȃ��
			if ((nStatus & FDST_MOTOR) && (nStatus & FDST_SELECT)) {
				colStatus = RGB(208, 31, 31);
			}
		}
		else {
			// �A�N�Z�X���Ȃ��
			if (nStatus & FDST_ACCESS) {
				colStatus = RGB(208, 31, 31);
			}
		}

		// �d��OFF�Ȃ�K���D�F
		if (!bPower) {
			colStatus = RGB(95, 95, 95);
		}
	}

	// �X�P�W���[����~���̏ꍇ�̓���
	if (!m_pSch->IsEnable()) {
		// �h���C�u�ڍׂ��擾
		m_pFDD->GetDrive(nDrive, &drv);
		if (!(nStatus & FDST_INSERT)) {
			// �f�B�X�N�͑}������Ă��Ȃ�
			if (drv.next) {
				// ���f�B�X�N������
				colStatus = RGB(95, 95, 95);

				drv.next->GetName(name, 0);
				lpszName = A2CT(name);
				_tcscpy(lpszText, lpszName);
			}
		}
	}

	// �C�W�F�N�g�֎~�Ȃ�ŏ�ʂ𗧂Ă�
	if (!(nStatus & FDST_EJECT)) {
		colStatus |= (COLORREF)0x80000000;
	}

	return colStatus;
}

//---------------------------------------------------------------------------
//
//	�X�e�[�^�XHD
//
//---------------------------------------------------------------------------
COLORREF FASTCALL CInfo::StatusHardDisk(LPTSTR lpszText)
{
	DWORD dwID;
	COLORREF color;

	ASSERT(this);
	ASSERT(lpszText);
	ASSERT(m_pSASI);

	// �d���`�F�b�N
	if (!::GetVM()->IsPower()) {
		// �d���������Ă��Ȃ��B��
		_tcscpy(lpszText, _T("HD BUSY"));
		m_dwDiskID = 0;
		m_dwDiskTime = DiskTypeTime;
		return RGB(0, 0, 0);
	}

	// �f�o�C�X�擾
	dwID = m_pSASI->GetBusyDevice();
	switch (dwID) {
		// SASI-HD
		case MAKEID('S', 'A', 'H', 'D'):
			color = RGB(208, 31, 31);
			break;

		// SCSI-HD
		case MAKEID('S', 'C', 'H', 'D'):
			color = RGB(208, 31, 31);
			break;

		// SCSI-MO
		case MAKEID('S', 'C', 'M', 'O'):
			color = RGB(208, 32, 31);
			break;

		// SCSI-CD
		case MAKEID('S', 'C', 'C', 'D'):
			color = RGB(208, 31, 32);
			break;

		// BUSY�łȂ�
		case 0:
			color = RGB(0, 0, 0);
			break;

		// ���̑�(���肦�Ȃ�)
		default:
			ASSERT(FALSE);
			_tcscpy(lpszText, _T("HD BUSY"));
			return RGB(0, 0, 0);
	}

	// �L���ȃf�o�C�X�̏ꍇ
	if (color != RGB(0, 0, 0)) {
		// �O��̃f�o�C�X�Ɠ�����
		if (dwID == m_dwDiskID) {
			// ���Ԃ�+1(�ő��DiskTypeTime)
			if (m_dwDiskTime < DiskTypeTime) {
				m_dwDiskTime++;
			}
		}
		else {
			// �f�o�C�X���؂�ւ�����̂ŁA�L�����Ď��Ԃ�������
			m_dwDiskID = dwID;
			m_dwDiskTime = 0;
		}
	}
	else {
		// BUSY�łȂ���΁A���ԃ`�F�b�N
		if (m_dwDiskTime >= DiskTypeTime) {
			// HD BUSY�ɖ߂�
			ASSERT(m_dwDiskTime == DiskTypeTime);
			_tcscpy(lpszText, _T("HD BUSY"));
			m_dwDiskID = 0;

			// ��
			return RGB(0, 0, 0);
		}

		// ���΂炭�A���ɋ߂��F�ŁA�O��̃f�o�C�X�ŕ\��
		m_dwDiskTime++;
		dwID = m_dwDiskID;
		color = RGB(0, 0, 1);
	}

	// ������쐬
	switch (dwID) {
		// SASI-HD
		case MAKEID('S', 'A', 'H', 'D'):
			_tcscpy(lpszText, _T("HD BUSY"));
			break;

		// SCSI-HD
		case MAKEID('S', 'C', 'H', 'D'):
			_tcscpy(lpszText, _T("HD BUSY"));
			break;

		// SCSI-MO
		case MAKEID('S', 'C', 'M', 'O'):
			_tcscpy(lpszText, _T("MO BUSY"));
			break;

		// SCSI-CD
		case MAKEID('S', 'C', 'C', 'D'):
			_tcscpy(lpszText, _T("CD BUSY"));
			break;

		// ���̑�(���肦�Ȃ�)
		default:
			ASSERT(FALSE);
	}

	return color;
}

//---------------------------------------------------------------------------
//
//	�X�e�[�^�XTIMER
//
//---------------------------------------------------------------------------
COLORREF FASTCALL CInfo::StatusTimer(LPTSTR lpszText) const
{
	ASSERT(this);
	ASSERT(lpszText);
	ASSERT(m_pRTC);

	// �e�L�X�g
	_tcscpy(lpszText, _T("TIMER"));

	// �^�C�}�[ON�Ȃ��(�d���͊֌W�Ȃ�)
	if (m_pRTC->GetTimerLED()) {
		return RGB(208, 31, 31);
	}

	// ��
	return RGB(0, 0, 0);
}

//---------------------------------------------------------------------------
//
//	�X�e�[�^�XPOWER
//
//---------------------------------------------------------------------------
COLORREF FASTCALL CInfo::StatusPower(LPTSTR lpszText) const
{
	VM *pVM;

	ASSERT(this);
	ASSERT(lpszText);
	ASSERT(m_pRTC);

	// VM�擾
	pVM = m_pRTC->GetVM();
	ASSERT(pVM);

	// �e�L�X�g
	_tcscpy(lpszText, _T("POWER"));

	// �d��OFF��
	if (!pVM->IsPower()) {
		if (m_bPowerLED) {
			// �Ð�
			return RGB(12, 23, 129);
		}
		else {
			// ��
			return RGB(208, 31, 31);
		}
	}

	// �d���X�C�b�`��ON��
	if (pVM->IsPowerSW()) {
		// �΂܂��͐�
		if (m_bPowerLED) {
			return RGB(50, 50, 255);
		}
		else {
			return RGB(31, 208, 31);
		}
	}

	// �d���X�C�b�`OFF�̎��́AALARM�o�͂ɂ��g�O������
	if (m_pRTC->GetAlarmOut()) {
		// �΂܂��͐�
		if (m_bPowerLED) {
			return RGB(50, 50, 255);
		}
		else {
			return RGB(31, 208, 31);
		}
	}
	if (m_bPowerLED) {
		// �Ð�
		return RGB(12, 23, 129);
	}
	else {
		// ��
		return RGB(0, 0, 0);
	}
}

//---------------------------------------------------------------------------
//
//	�X�e�[�^�X�`��
//
//---------------------------------------------------------------------------
void FASTCALL CInfo::DrawStatus(int nPane, HDC hDC, CRect& rectDraw)
{
	BOOL bHalf;
	COLORREF colStatus;
	HDC hMemDC;
	HBITMAP hBitmap;
	HBITMAP hDefBitmap;
	HFONT hFont;
	HFONT hDefFont;
	CRect rectMem;
	CRect rectLine;
	int nLine;

	ASSERT(this);
	ASSERT((nPane >= 0) && (nPane < PaneMax));
	ASSERT(hDC);

	// �J���[�ƃn�[�t�g�[��(���C�������`��)�̐ݒ�
	colStatus = m_colStatus[nPane];
	bHalf = FALSE;
	if (colStatus & 0x80000000) {
		bHalf = TRUE;
		colStatus &= (COLORREF)(0x7fffffff);
	}

	// ��������`�̐ݒ�
	rectMem.left = 0;
	rectMem.top = 0;
	rectMem.right = rectDraw.Width();
	rectMem.bottom = rectDraw.Height();

	// ������DC�쐬
	hMemDC = ::CreateCompatibleDC(hDC);

	// �r�b�g�}�b�v�쐬�A�Z���N�g
	hBitmap = ::CreateCompatibleBitmap(hDC, rectDraw.Width(), rectDraw.Height());
	hDefBitmap = (HBITMAP)::SelectObject(hMemDC, hBitmap);
	ASSERT(hDefBitmap);

	// �t�H���g�쐬�A�Z���N�g
	hFont = (HFONT)::GetStockObject(DEFAULT_GUI_FONT);
	hDefFont = (HFONT)::SelectObject(hMemDC, hFont);
	ASSERT(hDefFont);

	// �h��Ԃ�
	::SetBkColor(hMemDC, colStatus);
	::ExtTextOut(hMemDC, 0, 0, ETO_OPAQUE, &rectMem, NULL, 0, NULL);

	// �n�[�t����
	if (bHalf) {
		// ���C�������ɍ���������
		rectLine = rectMem;
		::SetBkColor(hMemDC, RGB(0, 0, 0));
		for (nLine=0; nLine<rectMem.bottom; nLine+=2) {
			rectLine.top = nLine;
			rectLine.bottom = nLine + 1;
			::ExtTextOut(hMemDC, 0, 0, ETO_OPAQUE, &rectLine, NULL, 0, NULL);
		}
		::SetBkColor(hMemDC, colStatus);
	}

	// �e�L�X�g�`��(�������A��O����)
	if (!m_pStatusView || (nPane < 2) || (colStatus != 0)) {
		// �e��t���邽�߁A1�h�b�g�����E���փV�t�g
		rectMem.left++;
		rectMem.top++;
		rectMem.right++;
		rectMem.bottom++;

		// �e��\��(��)
		::SetTextColor(hMemDC, RGB(0, 0, 0));
		::SetBkMode(hMemDC, TRANSPARENT);
		::DrawText(hMemDC, m_szStatus[nPane], (int)_tcslen(m_szStatus[nPane]),
						 &rectMem, DT_NOPREFIX | DT_CENTER | DT_VCENTER | DT_SINGLELINE);

		// �߂����߁A1�h�b�g��������փV�t�g
		rectMem.left--;
		rectMem.top--;
		rectMem.right--;
		rectMem.bottom--;

		// �{�̂�\��(��)
		::SetTextColor(hMemDC, RGB(255, 255, 255));
		::SetBkMode(hMemDC, TRANSPARENT);
		::DrawText(hMemDC, m_szStatus[nPane], (int)_tcslen(m_szStatus[nPane]),
						 &rectMem, DT_NOPREFIX | DT_CENTER | DT_VCENTER | DT_SINGLELINE);
	}

	// �t���b�s�[�f�B�X�N�̓h���C�u�ԍ���`��
	if ((nPane < 2) && (colStatus != 0)) {
		// ��O����
		if (!m_pStatusView || (colStatus != RGB(1, 1, 1))) {
			::SetDIBitsToDevice(hMemDC, 0, 0, 8, 8, 0, 0, 0, 8,
								&m_bmpDrive[nPane][sizeof(BITMAPINFOHEADER)],
								(BITMAPINFO*)&m_bmpDrive[nPane][0],
								DIB_RGB_COLORS);
		}
	}

	// BitBlt
	::BitBlt(hDC, rectDraw.left, rectDraw.top, rectDraw.Width(), rectDraw.Height(),
						hMemDC, 0, 0, SRCCOPY);

	// �t�H���g�߂�
	::SelectObject(hMemDC, hDefFont);

	// �r�b�g�}�b�v�߂�
	::SelectObject(hMemDC, hDefBitmap);
	::DeleteObject(hBitmap);

	// ������DC�߂�
	::DeleteDC(hMemDC);
}

//===========================================================================
//
//	���̑�
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	�X�e�[�^�X�r���[�ʒm
//
//---------------------------------------------------------------------------
void FASTCALL CInfo::SetStatusView(CStatusView *pView)
{
	ASSERT(this);

	// NULL�ɂ�����炸�A�L��
	m_pStatusView = pView;
}

#endif	// _WIN32
