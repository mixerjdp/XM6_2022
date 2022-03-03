//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 �o�h�D(ytanaka@ipc-tokai.or.jp)
//	[ MFC �T�u�E�B���h�E(�f�o�C�X) ]
//
//---------------------------------------------------------------------------

#if defined(_WIN32)

#if !defined(mfc_dev_h)
#define mfc_dev_h

#include "mfp.h"
#include "dmac.h"
#include "scc.h"
#include "fdc.h"
#include "midi.h"
#include "sasi.h"
#include "scsi.h"
#include "mfc_sub.h"

//===========================================================================
//
//	MFP�E�B���h�E
//
//===========================================================================
class CMFPWnd : public CSubTextWnd
{
public:
	CMFPWnd();
										// �R���X�g���N�^
	void FASTCALL Setup();
										// �Z�b�g�A�b�v

private:
	void FASTCALL SetupInt(int x, int y);
										// �Z�b�g�A�b�v(���荞��)
	void FASTCALL SetupGPIP(int x, int y);
										// �Z�b�g�A�b�v(GPIP)
	void FASTCALL SetupTimer(int x, int y);
										// �Z�b�g�A�b�v(�^�C�})
	static LPCTSTR DescInt[];
										// ���荞�݃e�[�u��
	static LPCTSTR DescGPIP[];
										// GPIP�e�[�u��
	static LPCTSTR DescTimer[];
										// �^�C�}�e�[�u��
	MFP *m_pMFP;
										// MFP
	MFP::mfp_t m_mfp;
										// MFP�����f�[�^
};

//===========================================================================
//
//	DMAC�E�B���h�E
//
//===========================================================================
class CDMACWnd : public CSubTextWnd
{
public:
	CDMACWnd();
										// �R���X�g���N�^
	void FASTCALL Setup();
										// �Z�b�g�A�b�v

private:
	void FASTCALL SetupCh(int nCh, DMAC::dma_t *pDMA, LPCTSTR lpszTitle);
										// �Z�b�g�A�b�v(�`���l��)
	DMAC *m_pDMAC;
										// DMAC
};

//===========================================================================
//
//	CRTC�E�B���h�E
//
//===========================================================================
class CCRTCWnd : public CSubTextWnd
{
public:
	CCRTCWnd();
										// �R���X�g���N�^
	void FASTCALL Setup();
										// �Z�b�g�A�b�v

private:
	CRTC *m_pCRTC;
										// CRTC
};

//===========================================================================
//
//	VC�E�B���h�E
//
//===========================================================================
class CVCWnd : public CSubTextWnd
{
public:
	CVCWnd();
										// �R���X�g���N�^
	void FASTCALL Setup();
										// �Z�b�g�A�b�v

private:
	VC *m_pVC;
										// VC
};

//===========================================================================
//
//	RTC�E�B���h�E
//
//===========================================================================
class CRTCWnd : public CSubTextWnd
{
public:
	CRTCWnd();
										// �R���X�g���N�^
	void FASTCALL Setup();
										// �Z�b�g�A�b�v

private:
	RTC* m_pRTC;
										// FDC
};

//===========================================================================
//
//	OPM�E�B���h�E
//
//===========================================================================
class COPMWnd : public CSubTextWnd
{
public:
	COPMWnd();
										// �R���X�g���N�^
	void FASTCALL Setup();
										// �Z�b�g�A�b�v

private:
	OPMIF *m_pOPM;
										// OPM
};

//===========================================================================
//
//	�L�[�{�[�h�E�B���h�E
//
//===========================================================================
class CKeyboardWnd : public CSubTextWnd
{
public:
	CKeyboardWnd();
										// �R���X�g���N�^
	void FASTCALL Setup();
										// �Z�b�g�A�b�v

private:
	static LPCTSTR DescLED[];
										// LED�e�[�u��
	Keyboard *m_pKeyboard;
										// �L�[�{�[�h
};

//===========================================================================
//
//	FDD�E�B���h�E
//
//===========================================================================
class CFDDWnd : public CSubTextWnd
{
public:
	CFDDWnd();
										// �R���X�g���N�^
	void FASTCALL Setup();
										// �Z�b�g�A�b�v

private:
	void FASTCALL SetupFDD(int nDrive, int x);
										// �Z�b�g�A�b�v�T�u
	BOOL FASTCALL SetupTrack();
										// �Z�b�g�A�b�v�g���b�N
	static LPCTSTR DescTable[];
										// �����e�[�u��
	FDD *m_pFDD;
										// FDD
	FDC *m_pFDC;
										// FDC
	DWORD m_dwDrive;
										// �A�N�Z�X�h���C�u
	DWORD m_dwHD;
										// �A�N�Z�X�w�b�h
	DWORD m_CHRN[4];
										// �A�N�Z�XCHRN
};

//===========================================================================
//
//	FDC�E�B���h�E
//
//===========================================================================
class CFDCWnd : public CSubTextWnd
{
public:
	CFDCWnd();
										// �R���X�g���N�^
	void FASTCALL Setup();
										// �Z�b�g�A�b�v

private:
	void FASTCALL SetupGeneral(int x, int y);
										// �Z�b�g�A�b�v(���)
	void FASTCALL SetupParam(int x, int y);
										// �Z�b�g�A�b�v(�p�����[�^)
	void FASTCALL SetupSR(int x, int y);
										// �Z�b�g�A�b�v(�X�e�[�^�X���W�X�^)
	static LPCTSTR SRDesc[8];
										// ������(�X�e�[�^�X���W�X�^)
	void FASTCALL SetupST0(int x, int y);
										// �Z�b�g�A�b�v(ST0)
	static LPCTSTR ST0Desc[8];
										// ������(ST0)
	void FASTCALL SetupST1(int x, int y);
										// �Z�b�g�A�b�v(ST1)
	static LPCTSTR ST1Desc[8];
										// ������(ST1)
	void FASTCALL SetupST2(int x, int y);
										// �Z�b�g�A�b�v(ST2)
	static LPCTSTR ST2Desc[8];
										// ������(ST2)
	void FASTCALL SetupSub(int x, int y, LPCTSTR lpszTitle, LPCTSTR *lpszDesc,
					DWORD data);		// �Z�b�g�A�b�v(�T�u)
	FDC *m_pFDC;
										// FDC
	const FDC::fdc_t *m_pWork;
										// FDC�����f�[�^
};

//===========================================================================
//
//	SCC�E�B���h�E
//
//===========================================================================
class CSCCWnd : public CSubTextWnd
{
public:
	CSCCWnd();
										// �R���X�g���N�^
	void FASTCALL Setup();
										// �Z�b�g�A�b�v

private:
	void FASTCALL SetupSCC(SCC::ch_t *pCh, int x, int y);
										// �Z�b�g�A�b�v(�`���l��)
	SCC *m_pSCC;
										// SCC
	static LPCTSTR DescTable[];
										// ������e�[�u��
};

//===========================================================================
//
//	Cynthia�E�B���h�E
//
//===========================================================================
class CCynthiaWnd : public CSubTextWnd
{
public:
	CCynthiaWnd();
										// �R���X�g���N�^
	void FASTCALL Setup();
										// �Z�b�g�A�b�v

private:
	Sprite *m_pSprite;
										// CYNTHIA
};

//===========================================================================
//
//	SASI�E�B���h�E
//
//===========================================================================
class CSASIWnd : public CSubTextWnd
{
public:
	CSASIWnd();
										// �R���X�g���N�^
	void FASTCALL Setup();
										// �Z�b�g�A�b�v

private:
	void FASTCALL SetupCmd(int x, int y);
										// �Z�b�g�A�b�v(�R�}���h)
	void FASTCALL SetupCtrl(int x, int y);
										// �Z�b�g�A�b�v(�R���g���[��)
	void FASTCALL SetupDrive(int x, int y);
										// �Z�b�g�A�b�v(�h���C�u)
	void FASTCALL SetupCache(int x, int y);
										// �Z�b�g�A�b�v(�L���b�V��)
	SASI *m_pSASI;
										// SASI
	SASI::sasi_t m_sasi;
										// SASI�����f�[�^
};

//===========================================================================
//
//	MIDI�E�B���h�E
//
//===========================================================================
class CMIDIWnd : public CSubTextWnd
{
public:
	CMIDIWnd();
										// �R���X�g���N�^
	void FASTCALL Setup();
										// �Z�b�g�A�b�v

private:
	void FASTCALL SetupCtrl(int x, int y);
										// �Z�b�g�A�b�v(�R���g���[��)
	static LPCTSTR DescCtrl[];
										// ������e�[�u��(�R���g���[��)
	void FASTCALL SetupInt(int x, int y);
										// �Z�b�g�A�b�v(���荞��)
	static LPCTSTR DescInt[];
										// ������e�[�u��(���荞��)
	void FASTCALL SetupTrans(int x, int y);
										// �Z�b�g�A�b�v(���M)
	static LPCTSTR DescTrans[];
										// ������e�[�u��(���M)
	void FASTCALL SetupRecv(int x, int y);
										// �Z�b�g�A�b�v(��M)
	static LPCTSTR DescRecv[];
										// ������e�[�u��(��M)
	void FASTCALL SetupRT(int x, int y);
										// �Z�b�g�A�b�v(���A���^�C�����M)
	static LPCTSTR DescRT[];
										// ������e�[�u��(���A���^�C�����M)
	void FASTCALL SetupRR(int x, int y);
										// �Z�b�g�A�b�v(���A���^�C����M)
	static LPCTSTR DescRR[];
										// ������e�[�u��(���A���^�C����M)
	void FASTCALL SetupCount(int x, int y);
										// �Z�b�g�A�b�v(�J�E���^)
	static LPCTSTR DescCount[];
										// ������e�[�u��(�J�E���^)
	void FASTCALL SetupHunter(int x, int y);
										// �Z�b�g�A�b�v(�A�h���X�n���^)
	static LPCTSTR DescHunter[];
										// ������e�[�u��(�A�h���X�n���^)
	void FASTCALL SetupFSK(int x, int y);
										// �Z�b�g�A�b�v(FSK)
	static LPCTSTR DescFSK[];
										// ������e�[�u��(FSK)
	void FASTCALL SetupGPIO(int x, int y);
										// �Z�b�g�A�b�v(GPIO)
	static LPCTSTR DescGPIO[];
										// ������e�[�u��(GPIO)

	MIDI *m_pMIDI;
										// MIDI
	MIDI::midi_t m_midi;
										// MIDI�����f�[�^
};

//===========================================================================
//
//	SCSI�E�B���h�E
//
//===========================================================================
class CSCSIWnd : public CSubTextWnd
{
public:
	CSCSIWnd();
										// �R���X�g���N�^
	void FASTCALL Setup();
										// �Z�b�g�A�b�v

private:
	void FASTCALL SetupCmd(int x, int y);
										// �Z�b�g�A�b�v(�R�}���h)
	void FASTCALL SetupCtrl(int x, int y);
										// �Z�b�g�A�b�v(�R���g���[��)
	void FASTCALL SetupDrive(int x, int y);
										// �Z�b�g�A�b�v(�h���C�u)
	void FASTCALL SetupReg(int x, int y);
										// �Z�b�g�A�b�v(���W�X�^)
	void FASTCALL SetupCDB(int x, int y);
										// �Z�b�g�A�b�v(CDB)
	SCSI *m_pSCSI;
										// SCSI
	SCSI::scsi_t m_scsi;
										// SCSI�����f�[�^
};

#endif	// mfc_dev_h
#endif	// _WIN32
