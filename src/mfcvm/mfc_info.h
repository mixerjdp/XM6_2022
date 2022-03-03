//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 �o�h�D(ytanaka@ipc-tokai.or.jp)
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
	// �萔�l
	enum {
		InfoBufMax = 100				// ���o�b�t�@�ő�L�����N�^��
	};

public:
	// ��{�t�@���N�V����
	CInfo(CFrmWnd *pWnd, CStatusBar *pBar);
										// �R���X�g���N�^
	BOOL FASTCALL Init();
										// ������
	void FASTCALL Cleanup();
										// �N���[���A�b�v
	void FASTCALL ApplyCfg(const Config *pConfig);
										// �ݒ�K�p
	void FASTCALL Enable(BOOL bEnable);
										// ���쐧��

	// �L���v�V����
	void FASTCALL ResetCaption();
										// �L���v�V�������Z�b�g
	void FASTCALL UpdateCaption();
										// �L���v�V�����X�V

	// ���
	void FASTCALL SetInfo(CString& strInfo);
										// ���ݒ�
	void FASTCALL SetMessageString(const CString& strMessage) const;
										// �ʏ탁�b�Z�[�W�ݒ�
	void FASTCALL UpdateInfo();
										// ���X�V

	// �X�e�[�^�X
	void FASTCALL ResetStatus();
										// �X�e�[�^�X���Z�b�g
	void FASTCALL UpdateStatus();
										// �X�e�[�^�X�X�V
	void FASTCALL DrawStatus(int nPane, HDC hDC, CRect& rectDraw);
										// �X�e�[�^�X�`��

	// �X�e�[�^�X�r���[
	void FASTCALL SetStatusView(CStatusView *pView);
										// �X�e�[�^�X�r���[�ݒ�

private:
	// �萔�l
	enum {
		CapTimeLong = 1500,				// �L���v�V�����X�V����(�ʏ�)
		CapTimeShort = 300,				// �L���v�V�����X�V����(����)
		InfoTime = 2000,				// ���\������
		PaneMax = 5,					// �X�e�[�^�X�ő�y�C����
		DiskTypeTime = 12				// �f�B�X�N��ʕێ�����
	};

	// �L���v�V����
	BOOL FASTCALL CheckParcent();
										// ���`�F�b�N
	BOOL FASTCALL CheckVM();
										// VM�t�@�C���`�F�b�N
	BOOL FASTCALL CheckMO();
										// MO�t�@�C���`�F�b�N
	BOOL FASTCALL CheckCD();
										// CD�t�@�C���`�F�b�N
	void FASTCALL SetCaption();
										// �L���v�V�����ݒ�
	CString m_strRun;
										// ���쒆���b�Z�[�W
	CString m_strStop;
										// ��~�����b�Z�[�W
	BOOL m_bRun;
										// �X�P�W���[�����쒆
	BOOL m_bCount;
										// ���쁓�v����
	int m_nParcent;
										// ���쁓(-1�͔�\��)
	DWORD m_dwTick;
										// ���v��(GetTickCount)
	DWORD m_dwTime;
										// ���v��(GetTotalTime)
	TCHAR m_szVM[_MAX_PATH];
										// VM�t�@�C���p�X
	TCHAR m_szVMFull[_MAX_PATH];
										// VM�t�@�C���p�X(�t��)
	TCHAR m_szMO[_MAX_PATH];
										// MO�t�@�C���p�X
	TCHAR m_szMOFull[_MAX_PATH];
										// MO�t�@�C���p�X(�t��)
	TCHAR m_szCD[_MAX_PATH];
										// CD�t�@�C���p�X
	TCHAR m_szCDFull[_MAX_PATH];
										// CD�t�@�C���p�X(�t��)
	CScheduler *m_pSch;
										// �X�P�W���[��(Win)
	Scheduler *m_pScheduler;
										// �X�P�W���[��
	SASI *m_pSASI;
										// SASI
	SCSI *m_pSCSI;
										// SCSI

	// ���
	BOOL m_bInfo;
										// ���̗L��
	BOOL m_bPower;
										// �d����ON/OFF
	CString m_strInfo;
										// ��񕶎���
	CString m_strPower;
										// �d��OFF������
	TCHAR m_szInfo[InfoBufMax];
										// �ʏ핶����
	DWORD m_dwInfo;
										// ���\������

	// �X�e�[�^�X
	COLORREF FASTCALL StatusFloppy(LPTSTR szText, int nDrive) const;
										// �X�e�[�^�XFD
	COLORREF FASTCALL StatusHardDisk(LPTSTR szText);
										// �X�e�[�^�XHD
	COLORREF FASTCALL StatusTimer(LPTSTR szText) const;
										// �X�e�[�^�XTIMER
	COLORREF FASTCALL StatusPower(LPTSTR szText) const;
										// �X�e�[�^�XPOWER
	COLORREF m_colStatus[PaneMax];
										// �X�e�[�^�X�\���F
	TCHAR m_szStatus[PaneMax][_MAX_PATH];
										// �X�e�[�^�X������
	BYTE m_bmpDrive[2][0x100];
										// �X�e�[�^�X�h���C�uBMP
	FDD *m_pFDD;
										// FDD
	RTC *m_pRTC;
										// RTC
	DWORD m_dwNumber;
										// �X�V�i���o
	DWORD m_dwDiskID;
										// �f�B�X�NBUSY���
	DWORD m_dwDiskTime;
										// �f�B�X�N��ʕێ�����

	// �X�e�[�^�X�o�[
	CStatusBar *m_pStatusBar;
										// �X�e�[�^�X�o�[

	// �X�e�[�^�X�r���[
	CStatusView *m_pStatusView;
										// �X�e�[�^�X�r���[

	// �R���t�B�M�����[�V����
	BOOL m_bFloppyLED;
										// ���[�^ON��LED�_��
	BOOL m_bPowerLED;
										// �F�d��LED
	BOOL m_bCaptionInfo;
										// �L���v�V�����ւ̏��\��
};

#endif	// mfc_info_h
#endif	// _WIN32
