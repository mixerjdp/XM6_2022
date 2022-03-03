//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 �o�h�D(ytanaka@ipc-tokai.or.jp)
//	[ MFC �T�u�E�B���h�E(�f�o�C�X) ]
//
//---------------------------------------------------------------------------

#if defined(_WIN32)

#include "os.h"
#include "xm6.h"
#include "vm.h"
#include "mfp.h"
#include "dmac.h"
#include "crtc.h"
#include "vc.h"
#include "rtc.h"
#include "opmif.h"
#include "keyboard.h"
#include "fdd.h"
#include "fdc.h"
#include "fdi.h"
#include "scc.h"
#include "render.h"
#include "sprite.h"
#include "disk.h"
#include "sasi.h"
#include "midi.h"
#include "mfc_sub.h"
#include "mfc_draw.h"
#include "mfc_res.h"
#include "mfc_dev.h"

//===========================================================================
//
//	MFP�E�B���h�E
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	�R���X�g���N�^
//
//---------------------------------------------------------------------------
CMFPWnd::CMFPWnd()
{
	// �E�B���h�E�p�����[�^��`
	m_dwID = MAKEID('M', 'F', 'P', ' ');
	::GetMsg(IDS_SWND_MFP, m_strCaption);
	m_nWidth = 55;
	m_nHeight = 34;

	// MFP�擾
	m_pMFP = (MFP*)::GetVM()->SearchDevice(MAKEID('M', 'F', 'P', ' '));
	ASSERT(m_pMFP);
}

//---------------------------------------------------------------------------
//
//	�Z�b�g�A�b�v
//
//---------------------------------------------------------------------------
void FASTCALL CMFPWnd::Setup()
{
	ASSERT(this);

	// �N���A
	Clear();

	// �f�[�^�擾
	ASSERT(m_pMFP);
	m_pMFP->GetMFP(&m_mfp);

	// �Z�b�g�A�b�v
	SetupInt(0, 0);
	SetupGPIP(0, 19);
	SetupTimer(0, 29);
}

//---------------------------------------------------------------------------
//
//	�Z�b�g�A�b�v(���荞��)
//
//---------------------------------------------------------------------------
void FASTCALL CMFPWnd::SetupInt(int x, int y)
{
	ASSERT(this);
	ASSERT(x >= 0);
	ASSERT(y >= 0);

	int i;

	// �K�C�h
	SetString(x, y, _T("(High)"));
	SetString(x + 24, y, _T("IER"));
	SetString(x + 33, y, _T("IPR"));
	SetString(x + 42, y, _T("ISR"));
	SetString(x + 50, y, _T("IMR"));
	y++;

	// ���[�v
	for (i=0; i<0x10; i++) {
		// ���̕�����
		if (m_mfp.iidx == i) {
			Reverse(TRUE);
			SetString(x, y, DescInt[i]);
			Reverse(FALSE);
		}
		else {
			SetString(x, y, DescInt[i]);
		}

		// ���N�G�X�g
		if (m_mfp.ireq[i]) {
			SetString(x + 15, y, _T("Request"));
		}

		// IER
		if (m_mfp.ier[i]) {
			SetString(x + 23, y, _T("Enable"));
		}

		// IPR
		if (m_mfp.ipr[i]) {
			SetString(x + 31, y, _T("Pending"));
		}

		// ISR
		if (m_mfp.isr[i]) {
			SetString(x + 39, y, _T("InService"));
		}

		// IMR
		if (!m_mfp.imr[i]) {
			SetString(x + 50, y, _T("Mask"));
		}

		// ����
		y++;
	}

	SetString(x, y, _T("(Low)"));
}

//---------------------------------------------------------------------------
//
//	���荞�݃e�[�u��
//
//---------------------------------------------------------------------------
LPCTSTR CMFPWnd::DescInt[] = {
	_T("CRTC H-SYNC"),
	_T("CRTC Raster"),
	_T("MFP Timer-A"),
	_T("MFP RxBufFull"),
	_T("MFP RxError"),
	_T("MFP TxBufEmpty"),
	_T("MFP TxError"),
	_T("MFP Timer-B"),
	_T("(No Connect)"),
	_T("CRTC V-DISP"),
	_T("MFP Timer-C"),
	_T("MFP Timer-D"),
	_T("OPM Timer IRQ"),
	_T("Power Switch"),
	_T("EXP Power On"),
	_T("RTC ALARM")
};

//---------------------------------------------------------------------------
//
//	�Z�b�g�A�b�v(GPIP)
//
//---------------------------------------------------------------------------
void FASTCALL CMFPWnd::SetupGPIP(int x, int y)
{
	int i;
	CString strText;
	DWORD dwGPDR;
	DWORD dwAER;
	DWORD dwDDR;
	DWORD dwBER;

	ASSERT(this);
	ASSERT(x >= 0);
	ASSERT(y >= 0);

	// �f�[�^�擾
	dwGPDR = m_mfp.gpdr;
	dwAER = m_mfp.aer;
	dwDDR = m_mfp.ddr;
	dwBER = m_mfp.ber;

	// �K�C�h
	SetString(x, y, _T("No."));
	SetString(x + 6, y, _T("Description"));
	SetString(x + 23, y, _T("GPIP"));
	SetString(x + 32, y, _T("AER"));
	SetString(x + 42, y, _T("DDR"));
	SetString(x + 51, y, _T("BER"));
	y++;

	// ���[�v
	for (i=0; i<8; i++) {
		// �i���o�\��
		strText.Format(_T("GPIP%1d"), 7 - i);
		SetString(x, y, strText);

		// �f�B�X�N���v�V����
		SetString(x + 6, y, DescGPIP[i]);

		// GPIP
		if (dwGPDR & 0x80) {
			SetString(x + 24, y, _T("1"));
		}
		else {
			SetString(x + 24, y, _T("0"));
		}

		// AER
		if (dwAER & 0x80) {
			SetString(x + 32, y, _T("0->1"));
		}
		else {
			SetString(x + 32, y, _T("1->0"));
		}

		// DDR
		if (dwDDR & 0x80) {
			SetString(x + 41, y, _T("Output"));
		}
		else {
			SetString(x + 41, y, _T("Input"));
		}

		// BER
		if (dwBER & 0x80) {
			SetString(x + 52, y, _T("1"));
		}
		else {
			SetString(x + 52, y, _T("0"));
		}

		dwGPDR <<= 1;
		dwAER <<= 1;
		dwDDR <<= 1;
		dwBER <<= 1;
		y++;
	}
}

//---------------------------------------------------------------------------
//
//	GPIP�e�[�u��
//
//---------------------------------------------------------------------------
LPCTSTR CMFPWnd::DescGPIP[] = {
	_T("CRTC H-SYNC"),
	_T("CRTC Raster"),
	_T("(No Connect)"),
	_T("CRTC V-DISP"),
	_T("OPM Timer IRQ"),
	_T("Power Switch"),
	_T("EXP Power On"),
	_T("RTC ALARM")
};

//---------------------------------------------------------------------------
//
//	�Z�b�g�A�b�v(�^�C�})
//
//---------------------------------------------------------------------------
void FASTCALL CMFPWnd::SetupTimer(int x, int y)
{
	int i;
	CString strText;

	ASSERT(this);
	ASSERT(x >= 0);
	ASSERT(y >= 0);

	SetString(x + 1, y, _T("Timer"));
	SetString(x + 14, y, _T("Mode"));
	SetString(x + 31, y, _T("Counter"));
	SetString(x + 45, y, _T("Reload"));

	y++;

	for (i=0; i<4; i++) {
		// �^�C�}�[����
		strText.Format(_T("Timer-%c"), _T('A') + i);
		SetString(0, y, strText);

		// ���[�h
		SetString(9, y, DescTimer[ m_mfp.tcr[i] & 0x0f ]);

		// �J�E���g
		strText.Format(_T("%3d"), m_mfp.tir[i]);
		SetString(33, y, strText);

		// �����[�h
		strText.Format(_T("%3d"), m_mfp.tdr[i]);
		SetString(46, y, strText);

		y++;
	}
}

//---------------------------------------------------------------------------
//
//	�^�C�}�e�[�u��
//
//---------------------------------------------------------------------------
LPCTSTR CMFPWnd::DescTimer[] = {
	_T("Stop"),
	_T("Delay (1us)"),
	_T("Delay (2.5us)"),
	_T("Delay (4us)"),
	_T("Delay (12.5us)"),
	_T("Delay (16us)"),
	_T("Delay (25us)"),
	_T("Delay (50us)"),
	_T("Event Count"),
	_T("Pulse-Width (1us)"),
	_T("Pulse-Width (2.5us)"),
	_T("Pulse-Width (4us)"),
	_T("Pulse-Width (12.5us)"),
	_T("Pulse-Width (16us)"),
	_T("Pulse-Width (25us)"),
	_T("Pulse-Width (50us)")
};

//===========================================================================
//
//	DMAC�E�B���h�E
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	�R���X�g���N�^
//
//---------------------------------------------------------------------------
CDMACWnd::CDMACWnd()
{
	// �E�B���h�E�p�����[�^��`
	m_dwID = MAKEID('D', 'M', 'A', 'C');
	::GetMsg(IDS_SWND_DMAC, m_strCaption);
	m_nWidth = 90;
	m_nHeight = 34;

	// DMAC�擾
	m_pDMAC = (DMAC*)::GetVM()->SearchDevice(MAKEID('D', 'M', 'A', 'C'));
	ASSERT(m_pDMAC);
}

//---------------------------------------------------------------------------
//
//	�Z�b�g�A�b�v
//
//---------------------------------------------------------------------------
void FASTCALL CDMACWnd::Setup()
{
	int i;
	DMAC::dma_t dma;
	static LPCTSTR lpszGuide[] = {
		_T("Request Mode"),
		_T("Device Type"),
		_T("Port Size"),
		_T("PCL Line"),
		_T("Direction"),
		_T("DONE Signal"),
		_T("Operand Size"),
		_T("Chain Mode"),
		_T("Request Gen."),
		_T("Active"),
		_T("Continue"),
		_T("Halt"),
		_T("Channel Complete"),
		_T("Block Complete"),
		_T("Interrupt"),
		_T("Normal Interrupt"),
		_T("Error  Interrupt"),
		_T("Error"),
		_T("Error Code"),
		_T("Transfer Type"),
		_T("Memory Transfer"),
		_T("Memory Address"),
		_T("Memory Inc/Dec"),
		_T("Device Address"),
		_T("Device Inc/Dec"),
		_T("Base   Transfer"),
		_T("Base   Address"),
		_T("Burst Time"),
		_T("Band Ratio"),
		_T("Priority"),
		_T("Start Count"),
		_T("Error Count"),
		NULL
	};

	// �N���A
	Clear();

	// �K�C�h�\��
	for (i=0; ; i++) {
		if (!lpszGuide[i]) {
			break;
		}
		SetString(0, i + 2, lpszGuide[i]);
	}

	ASSERT(m_pDMAC);

	// �`���l��0 (FDC)
	m_pDMAC->GetDMA(0, &dma);
	SetupCh(0, &dma, _T("#0 (FDC)"));

	// �`���l��1 (HDC)
	m_pDMAC->GetDMA(1, &dma);
	SetupCh(1, &dma, _T("#1 (HDC)"));

	// �`���l��2 (USER)
	m_pDMAC->GetDMA(2, &dma);
	SetupCh(2, &dma, _T("#2 (USER)"));

	// �`���l��3 (ADPCM)
	m_pDMAC->GetDMA(3, &dma);
	SetupCh(3, &dma, _T("#3 (ADPCM)"));
}

//---------------------------------------------------------------------------
//
//	�Z�b�g�A�b�v(�`���l��)
//
//---------------------------------------------------------------------------
void FASTCALL CDMACWnd::SetupCh(int nCh, DMAC::dma_t *pDMA, LPCTSTR lpszTitle)
{
	int x;
	int y;
	CString string;

	ASSERT((nCh >= 0) && (nCh <= 3));
	ASSERT(pDMA);
	ASSERT(lpszTitle);

	// x,y�Z�o
	x = (nCh * 18) + 20;
	y = 0;

	// �^�C�g���\��
	SetString(x, y, lpszTitle);
	y += 2;

	// XRM
	switch (pDMA->xrm) {
		case 0:
			string = _T("Burst");
			break;
		case 1:
			string = _T("Error");
			break;
		case 2:
			string = _T("Cycle Steal");
			break;
		case 3:
			string = _T("Cycle(Hold)");
			break;
		default:
			ASSERT(FALSE);
			break;
	}
	SetString(x, y, string);
	y++;

	// �f�o�C�X�^�C�v
	switch (pDMA->dtyp) {
		case 0:
			string = _T("68000");
			break;
		case 1:
			string = _T("6800");
			break;
		case 2:
			string = _T("ACK");
			break;
		case 3:
			string = _T("ACK & READY");
			break;
		default:
			ASSERT(FALSE);
			break;
	}
	SetString(x, y, string);
	y++;

	// �|�[�g�T�C�Y
	if (pDMA->dps) {
		string = _T("16bit");
	}
	else {
		string = _T("8bit");
	}
	SetString(x, y, string);
	y++;

	// PCL
	switch (pDMA->pcl) {
		case 0:
			string = _T("Status");
			break;
		case 1:
			string = _T("Status w/Int");
			break;
		case 2:
			string = _T("1/8 Pulse");
			break;
		case 3:
			string = _T("Abort Signal");
			break;
		default:
			ASSERT(FALSE);
			break;
	}
	SetString(x, y, string);
	y++;

	// �f�B���N�V����
	if (pDMA->dir) {
		string = _T("Device->Memory");
	}
	else {
		string = _T("Memory->Device");
	}
	SetString(x, y, string);
	y++;

	// DONE����
	if (pDMA->btd) {
		string = _T("Next Block");
	}
	else {
		string = _T("Normal");
	}
	SetString(x, y, string);
	y++;

	// �I�y�����h�T�C�Y
	switch (pDMA->size) {
		case 0:
			string = _T("Pack (8bit)");
			break;
		case 1:
			string = _T("Pack (16bit)");
			break;
		case 2:
			string = _T("Pack (32bit)");
			break;
		case 3:
			string = _T("Unpack (8bit)");
			break;
		default:
			ASSERT(FALSE);
			break;
	}
	SetString(x, y, string);
	y++;

	// �`�F�C�����[�h
	switch (pDMA->chain) {
		case 0:
			string = _T("No Chain");
			break;
		case 1:
			string = _T("Error");
			break;
		case 2:
			string = _T("Array Chain");
			break;
		case 3:
			string = _T("Link-Array Chain");
			break;
		default:
			ASSERT(FALSE);
			break;
	}
	SetString(x, y, string);
	y++;

	// ���N�G�X�g�W�F�l���[�V����
	switch (pDMA->reqg) {
		case 0:
			string = _T("Auto Req.(Limit)");
			break;
		case 1:
			string = _T("Auto Req.(Full)");
			break;
		case 2:
			string = _T("Ext Req.");
			break;
		case 3:
			string = _T("Auto + Ext");
			break;
		default:
			ASSERT(FALSE);
			break;
	}
	SetString(x, y, string);
	y++;

	// �`���l���A�N�e�B�u
	if (pDMA->act) {
		string = _T("Active");
	}
	else {
		string = _T("No");
	}
	SetString(x, y, string);
	y++;

	// �R���e�B�j���[
	if (pDMA->cnt) {
		string = _T("Continue");
	}
	else {
		string = _T("No");
	}
	SetString(x, y, string);
	y++;

	// �z�[���g
	if (pDMA->hlt) {
		string = _T("Halt");
	}
	else {
		string = _T("No");
	}
	SetString(x, y, string);
	y++;

	// �`���l���g�����X�t�@�R���v���[�g
	if (pDMA->coc) {
		string = _T("Complete");
	}
	else {
		string = _T("No");
	}
	SetString(x, y, string);
	y++;

	// �u���b�N�g�����X�t�@�R���v���[�g
	if (pDMA->boc) {
		string = _T("Complete");
	}
	else {
		string = _T("No");
	}
	SetString(x, y, string);
	y++;

	// ���荞��
	if (pDMA->intr) {
		string = _T("Enable");
	}
	else {
		string = _T("Disable");
	}
	SetString(x, y, string);
	y++;

	// �m�[�}�����荞��
	string.Format(_T("Vector $%02X"), pDMA->niv);
	SetString(x, y, string);
	y++;

	// �G���[���荞��
	string.Format(_T("Vector $%02X"), pDMA->eiv);
	SetString(x, y, string);
	y++;

	// �G���[�r�b�g
	if (pDMA->err) {
		string = _T("Error");
	}
	else {
		string = _T("No");
	}
	SetString(x, y, string);
	y++;

	// �G���[�R�[�h
	string.Format(_T("$%02X"), pDMA->ecode);
	SetString(x, y, string);
	y++;

	// �f�[�^�]���^�C�v
	string.Format(_T("%d"), pDMA->type);
	SetString(x, y, string);
	y++;

	// �������g�����X�t�@�J�E���^
	string.Format(_T("$%04X"), pDMA->mtc);
	SetString(x, y, string);
	y++;

	// �������A�h���X
	string.Format(_T("$%06X"), pDMA->mar);
	SetString(x, y, string);
	y++;

	// �������A�h���X���W�X�^�J�E���g
	switch (pDMA->mac) {
		case 0:
			string = _T("HOLD");
			break;
		case 1:
			string = _T("INC");
			break;
		case 2:
			string = _T("DEC");
			break;
		case 3:
			string = _T("ERROR");
			break;
		default:
			ASSERT(FALSE);
			break;
	}
	SetString(x, y, string);
	y++;

	// �f�o�C�X�A�h���X
	string.Format(_T("$%06X"), pDMA->dar);
	SetString(x, y, string);
	y++;

	// �f�o�C�X�A�h���X���W�X�^�J�E���g
	switch (pDMA->dac) {
		case 0:
			string = _T("HOLD");
			break;
		case 1:
			string = _T("INC");
			break;
		case 2:
			string = _T("DEC");
			break;
		case 3:
			string = _T("ERROR");
			break;
		default:
			ASSERT(FALSE);
			break;
	}
	SetString(x, y, string);
	y++;

	// �x�[�X�g�����X�t�@�J�E���^
	string.Format(_T("$%04X"), pDMA->btc);
	SetString(x, y, string);
	y++;

	// �x�[�X�A�h���X
	string.Format(_T("$%06X"), pDMA->bar);
	SetString(x, y, string);
	y++;

	// �o�[�X�g����
	string.Format(_T("%d Clock"), 1 << (pDMA->bt + 4));
	SetString(x, y, string);
	y++;

	// �o�X�䗦
	switch (pDMA->br) {
		case 0:
			string = _T("50%");
			break;
		case 1:
			string = _T("25%");
			break;
		case 2:
			string = _T("12.5%");
			break;
		case 3:
			string = _T("6.25%");
			break;
	}
	SetString(x, y, string);
	y++;

	// �v���C�I���e�B
	string.Format(_T("$%02X"), pDMA->cp);
	SetString(x, y, string);
	y++;

	// �X�^�[�g�J�E���g
	string.Format(_T("%d"), pDMA->startcnt);
	SetString(x, y, string);
	y++;

	// �G���[�J�E���g
	string.Format(_T("%d"), pDMA->errorcnt);
	SetString(x, y, string);
}

//===========================================================================
//
//	CRTC�E�B���h�E
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	�R���X�g���N�^
//
//---------------------------------------------------------------------------
CCRTCWnd::CCRTCWnd()
{
	// �E�B���h�E�p�����[�^��`
	m_dwID = MAKEID('C', 'R', 'T', 'C');
	::GetMsg(IDS_SWND_CRTC, m_strCaption);
	m_nWidth = 54;
	m_nHeight = 17;

	// CRTC�擾
	m_pCRTC = (CRTC*)::GetVM()->SearchDevice(MAKEID('C', 'R', 'T', 'C'));
	ASSERT(m_pCRTC);
}

//---------------------------------------------------------------------------
//
//	�Z�b�g�A�b�v
//
//---------------------------------------------------------------------------
void FASTCALL CCRTCWnd::Setup()
{
	CString string;
	DWORD h, v;
	int x, y;
	CRTC::crtc_t crtc;

	ASSERT(this);

	// �N���A
	Clear();
	y = 0;

	// �����f�[�^�y�ѓ������g�����擾
	ASSERT(m_pCRTC);
	m_pCRTC->GetCRTC(&crtc);
	m_pCRTC->GetHVHz(&h, &v);

	// �����{��x2�̏ꍇ�́A�����Ɍ��炷
	if (crtc.v_mul == 2 && !crtc.lowres) {
		crtc.v_dots >>= 1;
	}

	// ������
	SetString(0, y, "Horizontal");
	string.Format("%2d.%02d kHz", h / 100, h % 100);
	SetString(16, y, string);
	y++;
	SetString(0, y, "Width");
	string.Format("%4d dot", crtc.h_dots);
	SetString(0 + 17, y, string);
	y++;
	SetString(0, y, "Multiple");
	string.Format("%1d", crtc.h_mul);
	SetString(0 + 24, y, string);
	y++;

	// ����(���萔)
	SetString(0, y, "Sync");
	string.Format("%10d ns", crtc.h_sync);
	SetString(0 + 12, y, string);
	y++;
	SetString(0, y, "Pulse Width");
	string.Format("%10d ns", crtc.h_pulse);
	SetString(0 + 12, y, string);
	y++;
	SetString(0, y, "Back  Poach");
	string.Format("%10d ns", crtc.h_back);
	SetString(0 + 12, y, string);
	y++;
	SetString(0, y, "Front Poach");
	string.Format("%10d ns", crtc.h_front);
	SetString(0 + 12, y, string);
	y++;

	// ���X�^
	SetString(0, y, "Raster (All)");
	string.Format("%4d", crtc.raster_count);
	SetString(0 + 21, y, string);
	y++;
	SetString(0, y, "Raster (Disp)");
	string.Format("%4d", crtc.v_scan);
	SetString(0 + 21, y, string);
	y++;
	SetString(0, y, "Raster (Int)");
	string.Format("%4d", crtc.raster_int);
	SetString(0 + 21, y, string);
	y++;

	// �e�L�X�g�������A�O���t�B�b�N�����
	SetString(0, y, "Text VRAM");
	if (crtc.tmem) {
		SetString(0 + 19, y, "Buffer");
	}
	else {
		SetString(0 + 20, y, "Video");
	}
	y++;
	SetString(0, y, "Graphic Size");
	if (crtc.siz != 0) {
		SetString(0 + 16, y, "1024x1024");
	}
	else {
		SetString(0 + 18, y, "512x512");
	}
	y++;

	// �X�N���[��
	SetString(0, y, "Text Scroll X");
	string.Format("%4d", crtc.text_scrlx);
	SetString(0 + 21, y, string);
	y++;
	SetString(0, y, "GrpA Scroll X");
	string.Format("%4d", crtc.grp_scrlx[0]);
	SetString(0 + 21, y, string);
	y++;
	SetString(0, y, "GrpB Scroll X");
	string.Format("%4d", crtc.grp_scrlx[1]);
	SetString(0 + 21, y, string);
	y++;
	SetString(0, y, "GrpC Scroll X");
	string.Format("%4d", crtc.grp_scrlx[2]);
	SetString(0 + 21, y, string);
	y++;
	SetString(0, y, "GrpD Scroll X");
	string.Format("%4d", crtc.grp_scrlx[3]);
	SetString(0 + 21, y, string);

	// ������
	x = 29;
	y = 0;
	SetString(x, y, "Vertical");
	string.Format("%2d.%02d Hz", v / 100, v % 100);
	SetString(x + 17, y, string);
	y++;
	SetString(x, y, "Height");
	string.Format("%4d dot", crtc.v_dots);
	SetString(x + 17, y, string);
	y++;
	SetString(x, y, "Multiple");
	string.Format("%1d", crtc.v_mul);
	SetString(x + 24, y, string);
	y++;

	// ����(���萔)
	SetString(x, y, "Sync");
	string.Format("%10d ns", crtc.v_sync * crtc.h_sync);
	SetString(x + 12, y, string);
	y++;
	SetString(x, y, "Pulse Width");
	string.Format("%10d ns", crtc.v_pulse * crtc.h_sync);
	SetString(x + 12, y, string);
	y++;
	SetString(x, y, "Back Poach");
	string.Format("%10d ns", crtc.v_back * crtc.h_sync);
	SetString(x + 12, y, string);
	y++;
	SetString(x, y, "Front Poach");
	string.Format("%10d ns", crtc.v_front * crtc.h_sync);
	SetString(x + 12, y, string);
	y++;

	// �t���O
	SetString(x, y, "Sync  Flag");
	if (crtc.v_disp) {
		SetString(x + 22, y, "Off");
	}
	else {
		SetString(x + 21, y, "Sync");
	}
	y++;
	SetString(x, y, "Blank Flag");
	if (crtc.v_blank) {
		SetString(x + 20, y, "Blank");
	}
	else {
		SetString(x + 22, y, "Off");
	}
	y++;
	SetString(x, y, "Sync Count");
	string.Format("%10d", crtc.v_count);
	SetString(x + 15, y, string);
	y++;

	// �O���t�B�b�N�������A�O���t�B�b�N�F
	SetString(x, y, "Graphic VRAM");
	if (crtc.gmem) {
		SetString(x + 19, y, "Buffer");
	}
	else {
		SetString(x + 20, y, "Video");
	}
	y++;
	SetString(x, y, "Graphic Color");
	switch (crtc.col) {
		case 0:
			SetString(x + 23, y, "16");
			break;
		case 1:
			SetString(x + 22, y, "256");
			break;
		case 2:
			SetString(x + 20, y, "Undef");
			break;
		case 3:
			SetString(x + 20, y, "65536");
			break;
		default:
			ASSERT(FALSE);
	}
	y++;

	// �X�N���[��
	SetString(x, y, "Text Scroll Y");
	string.Format("%4d", crtc.text_scrly);
	SetString(x + 21, y, string);
	y++;
	SetString(x, y, "GrpA Scroll Y");
	string.Format("%4d", crtc.grp_scrly[0]);
	SetString(x + 21, y, string);
	y++;
	SetString(x, y, "GrpB Scroll Y");
	string.Format("%4d", crtc.grp_scrly[1]);
	SetString(x + 21, y, string);
	y++;
	SetString(x, y, "GrpC Scroll Y");
	string.Format("%4d", crtc.grp_scrly[2]);
	SetString(x + 21, y, string);
	y++;
	SetString(x, y, "GrpD Scroll Y");
	string.Format("%4d", crtc.grp_scrly[3]);
	SetString(x + 21, y, string);
}

//===========================================================================
//
//	VC�E�B���h�E
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	�R���X�g���N�^
//
//---------------------------------------------------------------------------
CVCWnd::CVCWnd()
{
	// �E�B���h�E�p�����[�^��`
	m_dwID = MAKEID('V', 'C', ' ', ' ');
	::GetMsg(IDS_SWND_VC, m_strCaption);
	m_nWidth = 54;
	m_nHeight = 12;

	// VC�擾
	m_pVC = (VC*)::GetVM()->SearchDevice(MAKEID('V', 'C', ' ', ' '));
	ASSERT(m_pVC);
}

//---------------------------------------------------------------------------
//
//	�Z�b�g�A�b�v
//
//---------------------------------------------------------------------------
void FASTCALL CVCWnd::Setup()
{
	VC::vc_t vc;
	CString string;
	int x;
	int y;
	int i;

	// �N���A
	Clear();

	// �r�f�I�R���g���[������f�[�^�擾
	m_pVC->GetVC(&vc);

	// ������
	x = 0;
	y = 0;

	// �T�C�Y
	SetString(x, y, "Graphic Size");
	if (vc.siz) {
		SetString(x + 16, y, "1024x1024");
	}
	else {
		SetString(x + 18, y, "512x512");
	}
	y++;

	// Ys
	SetString(x, y, "Ys");
	if (vc.ys) {
		SetString(x + 23, y, "On");
	}
	else {
		SetString(x + 22, y, "Off");
	}
	y++;

	// AH
	SetString(x, y, "AH");
	if (vc.ah) {
		SetString(x + 23, y, "On");
	}
	else {
		SetString(x + 22, y, "Off");
	}
	y++;

	// VHT
	SetString(x, y, "VHT");
	if (vc.vht) {
		SetString(x + 23, y, "On");
	}
	else {
		SetString(x + 22, y, "Off");
	}
	y++;

	// EXON
	SetString(x, y, "EXON");
	if (vc.exon) {
		SetString(x + 23, y, "On");
	}
	else {
		SetString(x + 22, y, "Off");
	}
	y++;

	// SON
	SetString(x, y, "BG/Sprite");
	if (vc.son) {
		SetString(x + 18, y, "Display");
	}
	else {
		SetString(x + 19, y, "Hidden");
	}
	y++;

	// TON
	SetString(x, y, "Text");
	if (vc.ton) {
		SetString(x + 18, y, "Display");
	}
	else {
		SetString(x + 19, y, "Hidden");
	}
	y++;

	// GON
	SetString(x, y, "Graphic(1024)");
	if (vc.gon) {
		SetString(x + 18, y, "Display");
	}
	else {
		SetString(x + 19, y, "Hidden");
	}
	y++;

	// GS
	for (i=0; i<4; i++) {
		string.Format("Graphic(512-Pri%1d)", i);
		SetString(x, y, string);
		if (vc.gs[i]) {
			SetString(x + 18, y, "Display");
		}
		else {
			SetString(x + 19, y, "Hidden");
		}
		y++;
	}

	// �E����
	x = 29;
	y = 0;

	// �F
	SetString(x, y, "Graphic Color");
	switch (vc.col) {
		case 0:
			if (vc.siz) {
				SetString(x + 19, y, "16(x1)");
			}
			else {
				SetString(x + 19, y, "16(x4)");
			}
			break;
		case 1:
			SetString(x + 18, y, "256(x2)");
			break;
		case 2:
		case 3:
			SetString(x + 16, y, "65536(x1)");
			break;
	}
	y++;

	// H/P
	SetString(x, y, "H/P");
	if (vc.hp) {
		SetString(x + 23, y, "On");
	}
	else {
		SetString(x + 22, y, "Off");
	}
	y++;

	// B/P
	SetString(x, y, "B/P");
	if (vc.bp) {
		SetString(x + 23, y, "On");
	}
	else {
		SetString(x + 22, y, "Off");
	}
	y++;

	// G/G
	SetString(x, y, "G/G");
	if (vc.gg) {
		SetString(x + 23, y, "On");
	}
	else {
		SetString(x + 22, y, "Off");
	}
	y++;

	// G/T
	SetString(x, y, "G/T");
	if (vc.gt) {
		SetString(x + 23, y, "On");
	}
	else {
		SetString(x + 22, y, "Off");
	}
	y++;

	// SP
	SetString(x, y, "BG/Sprite");
	string.Format("Pri%1d", vc.sp);
	SetString(x + 21, y, string);
	y++;

	// TX
	SetString(x, y, "Text");
	string.Format("Pri%1d", vc.tx);
	SetString(x + 21, y, string);
	y++;

	// GR
	SetString(x, y, "Graphic(All)");
	string.Format("Pri%1d", vc.gr);
	SetString(x + 21, y, string);
	y++;

	// GP
	for (i=0; i<4; i++) {
		string.Format("Graphic(512-Pri%1d)", i);
		SetString(x, y, string);
		string.Format("Blk%1d", vc.gp[i]);
		SetString(x + 21, y, string);
		y++;
	}
}

//===========================================================================
//
//	RTC�E�B���h�E
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	�R���X�g���N�^
//
//---------------------------------------------------------------------------
CRTCWnd::CRTCWnd()
{
	// �E�B���h�E�p�����[�^��`
	m_dwID = MAKEID('R', 'T', 'C', ' ');
	::GetMsg(IDS_SWND_RTC, m_strCaption);
	m_nWidth = 25;
	m_nHeight = 17;

	// RTC�擾
	m_pRTC = (RTC*)::GetVM()->SearchDevice(MAKEID('R', 'T', 'C', ' '));
	ASSERT(m_pRTC);
}

//---------------------------------------------------------------------------
//
//	�Z�b�g�A�b�v
//
//---------------------------------------------------------------------------
void FASTCALL CRTCWnd::Setup()
{
	RTC::rtc_t rtc;
	CString string;
	int x;
	int y;
	static const char* ClkoutTable[] = {
		"Always H",
		"16384Hz",
		"1024Hz",
		"128Hz",
		"16Hz",
		"1Hz",
		"1/60Hz",
		"Always L"
	};

	// �N���A
	Clear();
	x = 17;
	y = 0;

	// �f�[�^�擾
	ASSERT(m_pRTC);
	m_pRTC->GetRTC(&rtc);

	// ����
	SetString(0, y, "Time");
	string.Format("%02d:%02d:%02d", rtc.hour, rtc.min, rtc.sec);
	SetString(x, y, string);
	y++;

	// ���t
	SetString(0, y, "Date");
	string.Format("%02d/%02d/%02d", rtc.year, rtc.month, rtc.day);
	SetString(x, y, string);
	y++;

	// �j��
	SetString(0, y, "Day of Week");
	string.Format("%d", rtc.week);
	SetString(x + 7, y, string);
	y++;

	// �^�C�}�[�C�l�[�u��
	SetString(0, y, "Timer Enable");
	if (rtc.timer_en) {
		SetString(x + 2, y, "Enable");
	}
	else {
		SetString(x + 1, y, "Disable");
	}
	y++;

	// �A���[���C�l�[�u��
	SetString(0, y, "Alarm Enable");
	if (rtc.alarm_en) {
		SetString(x + 2, y, "Enable");
	}
	else {
		SetString(x + 1, y, "Disable");
	}
	y++;

	// 1Hz�C�l�[�u��
	SetString(0, y, "1Hz   Enable");
	if (rtc.alarm_1hz) {
		SetString(x + 2, y, "Enable");
	}
	else {
		SetString(x + 1, y, "Disable");
	}
	y++;

	// 16Hz�C�l�[�u��
	SetString(0, y, "16Hz  Enable");
	if (rtc.alarm_16hz) {
		SetString(x + 2, y, "Enable");
	}
	else {
		SetString(x + 1, y, "Disable");
	}
	y++;

	// CLKOUT�Z���N�g
	SetString(0, y, "CLKOUT");
	ASSERT(rtc.clkout <= 7);
	SetString(x, y, ClkoutTable[rtc.clkout]);
	y++;

	// �A���[������
	SetString(0, y, "Alarm Time");
	string.Format("%02d:%02d", rtc.alarm_hour, rtc.alarm_min);
	SetString(x + 3, y, string);
	y++;

	// �A���[������
	SetString(0, y, "Alarm Day");
	string.Format("%02d(%1d)", rtc.alarm_day, rtc.alarm_week);
	SetString(x + 3, y, string);
	y++;

	// �A���[��
	SetString(0, y, "Alarm Cmp");
	if (rtc.alarm) {
		SetString(x + 7, y, "Z");
	}
	else {
		SetString(x + 6, y, "NZ");
	}
	y++;

	// �A���[��OUT
	SetString(0, y, "ALARM OUT");
	if (rtc.alarmout) {
		SetString(x + 7, y, "H");
	}
	else {
		SetString(x + 7, y, "L");
	}
	y++;

	// 12h,24h�t���O
	SetString(0, y, "12h/24h");
	if (rtc.fullhour) {
		SetString(x + 5, y, "24h");
	}
	else {
		SetString(x + 5, y, "12h");
	}
	y++;

	// �[�N�J�E���^
	SetString(0, y, "Leap Count");
	string.Format("%1d", rtc.leap);
	SetString(x + 7, y, string);
	y++;

	// 1Hz�M��
	SetString(0, y, "1Hz  Signal");
	if (rtc.signal_1hz) {
		SetString(x + 7, y, "H");
	}
	else {
		SetString(x + 7, y, "L");
	}
	y++;

	// 16Hz�M��
	SetString(0, y, "16Hz Signal");
	if (rtc.signal_16hz) {
		SetString(x + 7, y, "H");
	}
	else {
		SetString(x + 7, y, "L");
	}
	y++;

	// �^�C�}�[LED
	SetString(0, y, "Timer LED");
	if (m_pRTC->GetTimerLED()) {
		SetString(x + 6, y, "On");
	}
	else {
		SetString(x + 5, y, "Off");
	}
}

//===========================================================================
//
//	OPM�E�B���h�E
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	�R���X�g���N�^
//
//---------------------------------------------------------------------------
COPMWnd::COPMWnd()
{
	// �E�B���h�E�p�����[�^��`
	m_dwID = MAKEID('O', 'P', 'M', ' ');
	::GetMsg(IDS_SWND_OPM, m_strCaption);
	m_nWidth = 20;
	m_nHeight = 33;

	// OPM�擾
	m_pOPM = (OPMIF*)::GetVM()->SearchDevice(MAKEID('O', 'P', 'M', ' '));
	ASSERT(m_pOPM);
}

//---------------------------------------------------------------------------
//
//	�Z�b�g�A�b�v
//
//---------------------------------------------------------------------------
void FASTCALL COPMWnd::Setup()
{
	int y;
	int i;
	int j;
	OPMIF::opm_t opm;
	DWORD *p;
	CString string;

	// �N���A
	Clear();

	// �f�[�^�擾
	ASSERT(m_pOPM);
	m_pOPM->GetOPM(&opm);

	// �w�b�_
	y = 0;
	SetString(0, y, "REG:+0+1+2+3+4+5+6+7");
	y++;

	// ���W�X�^���[�v
	p = opm.reg;
	for (i=0; i<32; i++) {
		// �A�h���X�Z�b�g
		string.Format("+%02X:", i << 3);
		SetString(0, y, string);

		// ���[�v
		for (j=0; j<8; j++) {
			string.Format("%02X", *p++);
			SetString(j * 2 + 4, y, string);
		}

		// ���̍s��
		y++;
	}
}

//===========================================================================
//
//	�L�[�{�[�h�E�B���h�E
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	�R���X�g���N�^
//
//---------------------------------------------------------------------------
CKeyboardWnd::CKeyboardWnd()
{
	// �E�B���h�E�p�����[�^��`
	m_dwID = MAKEID('K', 'E', 'Y', 'B');
	::GetMsg(IDS_SWND_KEYBOARD, m_strCaption);
	m_nWidth = 37;
	m_nHeight = 16;

	// �L�[�{�[�h�擾
	m_pKeyboard = (Keyboard*)::GetVM()->SearchDevice(MAKEID('K', 'E', 'Y', 'B'));
	ASSERT(m_pKeyboard);
}

//---------------------------------------------------------------------------
//
//	�Z�b�g�A�b�v
//
//---------------------------------------------------------------------------
void FASTCALL CKeyboardWnd::Setup()
{
	Keyboard::keyboard_t keyboard;
	CString strText;
	int x;
	int y;
	int nLED;
	DWORD dwLED;
	int nKey;

	// �N���A
	Clear();
	y = 0;

	// �f�[�^�擾
	ASSERT(m_pKeyboard);
	m_pKeyboard->GetKeyboard(&keyboard);

	// ���s�[�g�R�[�h
	SetString(0, y, _T("Repeat Code"));
	strText.Format(_T("%02X"), keyboard.rep_code);
	SetString(19, y, strText);
	y++;

	// ���s�[�g�J�E���^
	SetString(0, y, _T("Repeat Count"));
	strText.Format(_T("%6u"), keyboard.rep_count);
	SetString(15, y, strText);
	y++;

	// ���s�[�g�^�C��
	SetString(0, y, _T("Repeat Time1"));
	strText.Format(_T("%5dms"), keyboard.rep_start / 2000);
	SetString(14, y, strText);
	y++;
	SetString(0, y, _T("Repeat Time2"));
	strText.Format(_T("%5dms"), keyboard.rep_next / 2000);
	SetString(14, y, strText);
	y++;

	// ���M�C�l�[�u��
	SetString(0, y, _T("Send Enable"));
	if (keyboard.send_en) {
		SetString(15, y, _T("Enable"));
	}
	else {
		SetString(14, y, _T("Disable"));
	}
	y++;

	// ���M�E�F�C�g
	SetString(0, y, _T("Send Wait"));
	if (keyboard.send_wait) {
		SetString(17, y, _T("Wait"));
	}
	else {
		SetString(15, y, _T("Normal"));
	}
	y++;

	// MSCTRL
	SetString(0, y, _T("MSCTRL"));
	strText.Format(_T("%01X"), keyboard.msctrl);
	SetString(20, y, strText);
	y++;

	// OPT2�R���g���[��
	SetString(0, y, _T("OPT2 Ctrl"));
	if (keyboard.opt2_ctrl) {
		SetString(15, y, _T("Enable"));
	}
	else {
		SetString(14, y, _T("Disable"));
	}
	y++;

	// ���邳
	SetString(0, y, _T("LED Bright"));
	strText.Format(_T("%01X"), keyboard.bright);
	SetString(20, y, strText);
	y++;

	// LED
	dwLED = keyboard.led;
	for (nLED=0;; nLED++) {
		// ����
		if (!DescLED[nLED]) {
			break;
		}
		SetString(0, y, DescLED[nLED]);

		// ON,OFF
		SetString(9, y, _T("LED"));
		if (dwLED & 1) {
			SetString(19, y, _T("On"));
		}
		else {
			SetString(18, y, _T("Off"));
		}
		dwLED >>= 1;
		y++;
	}

	// �ď�����
	x = 25;
	y = 0;

	// �L�[�}�b�v
	SetString(x, y, _T("KEY:01234567"));
	for (nKey=0; nKey<120; nKey++) {
		// 8�L�[�ɂ�1��
		if ((nKey & 7) == 0) {
			// ���s&LF
			x = 25;
			y++;

			// ���C�����Ƃ̃K�C�h
			strText.Format(_T("+%02X:"), nKey);
			SetString(x, y, strText);
			x += 4;
		}

		// �L�[�`�F�b�N
		if (keyboard.status[nKey]) {
			// Make���
			SetString(x, y, _T("X"));
		}
		else {
			// Break���
			SetString(x, y, _T("."));
		}
		x++;
	}
}

//---------------------------------------------------------------------------
//
//	�L�[LED�e�[�u��
//
//---------------------------------------------------------------------------
LPCTSTR CKeyboardWnd::DescLED[] = {
	_T("KANA"),
	_T("RO-MAJI"),
	_T("CODE"),
	_T("CAPS"),
	_T("INS"),
	_T("HIRAGANA"),
	_T("ZENKAKU"),
	NULL
};

//===========================================================================
//
//	FDD�E�B���h�E
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	�R���X�g���N�^
//
//---------------------------------------------------------------------------
CFDDWnd::CFDDWnd()
{
	int i;
	const FDC::fdc_t *pFDC;

	// �E�B���h�E�p�����[�^��`
	m_dwID = MAKEID('F', 'D', 'D', ' ');
	::GetMsg(IDS_SWND_FDD, m_strCaption);
	m_nWidth = 63;
	m_nHeight = 30;

	// FDD�擾
	m_pFDD = (FDD*)::GetVM()->SearchDevice(MAKEID('F', 'D', 'D', ' '));
	ASSERT(m_pFDD);

	// FDC�擾
	m_pFDC = (FDC*)::GetVM()->SearchDevice(MAKEID('F', 'D', 'C', ' '));
	ASSERT(m_pFDC);

	// �A�N�Z�X�h���C�u�E�w�b�h�ECHRN��������
	pFDC = m_pFDC->GetWork();
	m_dwDrive = pFDC->dsr;
	m_dwHD = pFDC->hd;
	if (m_dwDrive >= 2) {
		m_dwDrive = 0;
	}
	for (i=0; i<4; i++) {
		m_CHRN[i] = 0;
	}
}

//---------------------------------------------------------------------------
//
//	�Z�b�g�A�b�v
//
//---------------------------------------------------------------------------
void FASTCALL CFDDWnd::Setup()
{
	int i;
	int y;
	const FDC::fdc_t *pFDC;

	// �N���A
	Clear();
	y = 2;

	// ������\��
	for (i=0;; i++) {
		if (!DescTable[i]) {
			break;
		}
		SetString(0, y, DescTable[i]);
		y++;
	}

	// ���Ԃɏ���
	for (i=0; i<=3; i++) {
		SetupFDD(i, (i * 12) + 18);
	}

	// FDC���[�N���擾�A�R�}���h���`�F�b�N
	pFDC = m_pFDC->GetWork();
	switch (pFDC->cmd) {
		case FDC::read_id:
		case FDC::read_data:
		case FDC::read_del_data:
		case FDC::write_data:
		case FDC::write_del_data:
		case FDC::read_diag:
		case FDC::scan_eq:
		case FDC::scan_lo_eq:
		case FDC::scan_hi_eq:
			// �h���C�u��CHRN���擾
			m_dwDrive = pFDC->dsr;
			if (m_dwDrive > 1) {
				m_dwDrive = 0;
			}
			m_dwHD = pFDC->hd;
			for (i=0; i<4; i++) {
				m_CHRN[i] = pFDC->chrn[i];
			}
			break;

		default:
			break;
	}

	// �Z�b�g�A�b�v(�g���b�N)
	SetupTrack();
	return;
}

//---------------------------------------------------------------------------
//
//	�����e�[�u��
//
//---------------------------------------------------------------------------
LPCTSTR CFDDWnd::DescTable[] = {
	_T("Image Type"),
	_T("Multi Image"),
	_T("Read Only"),
	_T("Write Protect"),
	_T("Selected Drive"),
	_T("Density"),
	_T("Motor"),
	_T("Settle"),
	_T("Force Ready"),
	_T("Insert"),
	_T("Invalid Insert"),
	_T("Eject Button"),
	_T("Blink  LED"),
	_T("Access LED"),
	_T("Seek"),
	_T("Cylinder"),
	_T(""),
	_T("Track Status"),
	_T("C"),
	_T("H"),
	_T("R"),
	_T("N"),
	_T("MFM/FM Flag"),
	_T("GAP3 Length"),
	_T("Error Status1"),
	_T("Error Status2"),
	_T("Access Time1"),
	_T("Access Time2"),
	NULL
};

//---------------------------------------------------------------------------
//
//	�Z�b�g�A�b�v�T�u
//
//---------------------------------------------------------------------------
void FASTCALL CFDDWnd::SetupFDD(int nDrive, int x)
{
	CString strText;
	FDD::fdd_t fdd;
	FDD::drv_t drv;
	int y;
	DWORD dwID;

	ASSERT((nDrive >= 0) && (nDrive <= 3));
	ASSERT(x >= 0);

	// �h���C�u��
	strText.Format(_T("FDD#%1d"), nDrive);
	SetString(x, 0, strText);

	// �h���C�u���擾
	ASSERT(m_pFDD);
	m_pFDD->GetFDD(&fdd);
	m_pFDD->GetDrive(nDrive, &drv);
	y = 2;

	// �C���[�W�^�C�v
	dwID = drv.fdi->GetID();
	strText.Format(_T("%c%c%c%c"),
				(dwID >> 24) & 0xff,
				(dwID >> 16) & 0xff,
				(dwID >> 8) & 0xff,
				dwID & 0xff);
	SetString(x, y, strText);
	y++;

	// �}���`�C���[�W
	if (drv.fdi->IsMulti()) {
		SetString(x, y, _T("Multi"));
	}
	else {
		SetString(x, y, _T("No"));
	}
	y++;

	// ���[�h�I�����[
	if (drv.fdi->IsReadOnly()) {
		SetString(x, y, _T("RO"));
	}
	else {
		SetString(x, y, _T("RW"));
	}
	y++;

	// ���C�g�v���e�N�g
	if (drv.fdi->IsWriteP()) {
		SetString(x, y, _T("Protect"));
	}
	else {
		SetString(x, y, _T("No"));
	}
	y++;

	// �I���h���C�u
	if (fdd.selected == nDrive) {
		SetString(x, y, _T("Selected"));
	}
	else {
		SetString(x, y, _T("No"));
	}
	y++;

	// ���x
	if (fdd.hd) {
		SetString(x, y, _T("HD"));
	}
	else {
		SetString(x, y, _T("DD"));
	}
	y++;

	// ���[�^
	if (fdd.motor) {
		SetString(x, y, _T("On"));
	}
	else {
		SetString(x, y, _T("Off"));
	}
	y++;

	// �Z�g�����O
	if (fdd.settle || fdd.motor) {
		SetString(x, y, _T("Settle"));
	}
	else {
		SetString(x, y, _T("No"));
	}
	y++;

	// �������f�B
	if (fdd.force) {
		SetString(x, y, _T("Ready"));
	}
	else {
		SetString(x, y, _T("No"));
	}
	y++;

	// �C���T�[�g
	if (drv.insert) {
		SetString(x, y, _T("Insert"));
	}
	else {
		SetString(x, y, _T("No"));
	}
	y++;

	// ��}��
	if (drv.invalid) {
		SetString(x, y, _T("Invalid"));
	}
	else {
		SetString(x, y, _T("No"));
	}
	y++;

	// �C�W�F�N�g�L��
	if (drv.eject) {
		SetString(x, y, _T("Enable"));
	}
	else {
		SetString(x, y, _T("Disable"));
	}
	y++;

	// �u�����N
	if (drv.blink) {
		SetString(x, y, _T("Blink"));
	}
	else {
		SetString(x, y, _T("Off"));
	}
	y++;

	// �A�N�Z�X
	if (drv.access) {
		SetString(x, y, _T("Access"));
	}
	else {
		SetString(x, y, _T("No"));
	}
	y++;

	// �V�[�N
	if (drv.seeking) {
		SetString(x, y, _T("Seek"));
	}
	else {
		SetString(x, y, _T("No"));
	}
	y++;

	// �V�����_
	strText.Format(_T("%d"), drv.cylinder);
	SetString(x, y, strText);
}

//---------------------------------------------------------------------------
//
//	�Z�b�g�A�b�v�g���b�N
//
//---------------------------------------------------------------------------
BOOL FASTCALL CFDDWnd::SetupTrack()
{
	FDD::drv_t drv;
	FDI *pFDI;
	FDIDisk *pDisk;
	FDITrack *pTrack;
	FDISector *pSector;
	CString strText;
	int i;
	int x;
	int y;
	int nSecs;
	int nTrack;
	DWORD chrn[4];

	ASSERT(this);

	// FDD�擾
	m_pFDD->GetDrive((int)m_dwDrive, &drv);

	// x,y�ݒ�
	x = 18;
	y = 19;

	// �擾
	pFDI = m_pFDD->GetFDI((int)m_dwDrive);
	ASSERT(pFDI);
	pDisk = pFDI->GetDisk();
	if (!pDisk) {
		return FALSE;
	}
	if (m_dwHD == 0) {
		nTrack = drv.cylinder << 1;
	}
	else {
		nTrack = (drv.cylinder << 1) + 1;
	}
	pTrack = pDisk->Search(nTrack);
	if (!pTrack) {
		return FALSE;
	}

	// �Z�N�^���𐔂���
	nSecs = 0;
	pSector = pTrack->GetFirst();
	while (pSector) {
		nSecs++;
		pSector = pSector->GetNext();
	}

	// �ŏ��̕\��
	if (nSecs == 0) {
		if (pTrack->IsHD()) {
			strText.Format(_T("Track %d, 2HD, unformat"), nTrack);
		}
		else {
			strText.Format(_T("Track %d, 2DD, unformat"), nTrack);
		}
	}
	else {
		if (pTrack->IsHD()) {
			strText.Format(_T("Track %d, 2HD, %d sectors"), nTrack, nSecs);
		}
		else {
			strText.Format(_T("Track %d, 2DD, %d sectors"), nTrack, nSecs);
		}
	}
	SetString(x, y, strText);
	y++;

	// �Z�N�^�������[�v
	pSector = pTrack->GetFirst();
	while (pSector) {
		// �擾
		pSector->GetCHRN(chrn);

		// ��v���Ă���΃��o�[�X(C,H,R�̂݌���)
		chrn[3] = m_CHRN[3];
		if (memcmp(chrn, m_CHRN, sizeof(chrn)) == 0) {
			Reverse(TRUE);
		}
		else {
			Reverse(FALSE);
		}

		// CHRN
		pSector->GetCHRN(chrn);
		for (i=0; i<4; i++) {
			strText.Format(_T("%02X"), chrn[i]);
			SetString(x, y + i, strText);
		}

		// MFM
		if (pSector->IsMFM()) {
			SetString(x, y + 4, _T("MF"));
		}
		else {
			SetString(x, y + 4, _T("FM"));
		}

		// GAP3
		strText.Format(_T("%02X"), pSector->GetGAP3());
		SetString(x, y + 5, strText);

		// STAT
		strText.Format(_T("%02X"), pSector->GetError() >> 8);
		SetString(x, y + 6, strText);
		strText.Format(_T("%02X"), pSector->GetError() & 0xff);
		SetString(x, y + 7, strText);

		// TIME
		strText.Format(_T("%02X"), (pSector->GetPos() >> 4) >> 8);
		SetString(x, y + 8, strText);
		strText.Format(_T("%02X"), (pSector->GetPos() >> 4) & 0xff);
		SetString(x, y + 9, strText);

		// �����擾
		x += 3;
		pSector = pSector->GetNext();
	}

	return TRUE;
}


//===========================================================================
//
//	FDC�E�B���h�E
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	�R���X�g���N�^
//
//---------------------------------------------------------------------------
CFDCWnd::CFDCWnd()
{
	// �E�B���h�E�p�����[�^��`
	m_dwID = MAKEID('F', 'D', 'C', ' ');
	::GetMsg(IDS_SWND_FDC, m_strCaption);
	m_nWidth = 71;
	m_nHeight = 19;

	// FDC�擾
	m_pFDC = (FDC*)::GetVM()->SearchDevice(MAKEID('F', 'D', 'C', ' '));
	ASSERT(m_pFDC);
}

//---------------------------------------------------------------------------
//
//	�Z�b�g�A�b�v
//
//---------------------------------------------------------------------------
void FASTCALL CFDCWnd::Setup()
{
	ASSERT(this);

	// ���[�N�A�h���X�擾
	ASSERT(m_pFDC);
	m_pWork = m_pFDC->GetWork();

	// �N���A
	Clear();

	// ���ɃZ�b�g
	SetupGeneral(0, 0);
	SetupParam(24, 0);
	SetupSR(48, 0);
	SetupST0(0, 10);
	SetupST1(24, 10);
	SetupST2(48, 10);
}

//---------------------------------------------------------------------------
//
//	�Z�b�g�A�b�v(���)
//
//---------------------------------------------------------------------------
void FASTCALL CFDCWnd::SetupGeneral(int x, int y)
{
	CString strText;

	ASSERT(this);
	ASSERT(x >= 0);
	ASSERT(y >= 0);

	// �R�}���h
	switch (m_pWork->cmd) {
		case FDC::read_data:
			strText = _T("READ DATA");
			break;
		case FDC::read_del_data:
			strText = _T("READ DELETED DATA");
			break;
		case FDC::read_id:
			strText = _T("READ ID");
			break;
		case FDC::write_id:
			strText = _T("WRITE ID");
			break;
		case FDC::write_data:
			strText = _T("WRITE DATA");
			break;
		case FDC::write_del_data:
			strText = _T("WRITE DELETED DATA");
			break;
		case FDC::read_diag:
			strText = _T("READ DIAGNOSTIC");
			break;
		case FDC::scan_eq:
			strText = _T("SCAN EQUAL");
			break;
		case FDC::scan_lo_eq:
			strText = _T("SCAN LOW OR EQUAL");
			break;
		case FDC::scan_hi_eq:
			strText = _T("SCAN HIGH OR EQUAL");
			break;
		case FDC::seek:
			strText = _T("SEEK");
			break;
		case FDC::recalibrate:
			strText = _T("RECALIBRATE");
			break;
		case FDC::sense_int_stat:
			strText = _T("SENSE INTERRUPT STATUS");
			break;
		case FDC::sense_dev_stat:
			strText = _T("SENSE DEVICE STATUS");
			break;
		case FDC::specify:
			strText = _T("SPECIFY");
			break;
		case FDC::set_stdby:
			strText = _T("SET STANDBY");
			break;
		case FDC::reset_stdby:
			strText = _T("RESET STANDBY");
			break;
		case FDC::fdc_reset:
			strText = _T("SOFTWARE RESET");
			break;
		case FDC::invalid:
			strText = _T("INVALID");
			break;
		case FDC::no_cmd:
			strText = _T("(NO COMMAND)");
			break;
		default:
			strText.Empty();
			ASSERT(FALSE);
			break;
	}
	SetString(x, y, strText);
	y++;

	// �t�F�[�Y
	switch (m_pWork->phase) {
		case FDC::idle:
			strText = _T("(Idle)");
			break;
		case FDC::command:
			strText = _T("C-Phase");
			break;
		case FDC::execute:
			strText = _T("E-Phase");
			break;
		case FDC::read:
			strText = _T("ER-Phase");
			break;
		case FDC::write:
			strText = _T("EW-Phase");
			break;
		case FDC::result:
			strText = _T("R-Phase");
			break;
		default:
			strText.Empty();
			ASSERT(FALSE);
			break;
	}
	SetString(x, y, strText);
	y++;

	// ��s������
	y++;

	// �h���C�u�Z���N�g���W�X�^
	SetString(x, y, _T("Drive Select"));
	strText.Format(_T("%1d"), m_pWork->dsr);
	SetString(x + 19, y, strText);
	y++;

	// �R���g���[�����W�X�^
	SetString(x, y, _T("Drive Control"));
	strText.Format(_T("%02X"), m_pWork->dcr);
	SetString(x + 18, y, strText);
	y++;

	// FIFO(����)
	SetString(x, y, _T("FIFO(IN)  Count"));
	strText.Format(_T("%2d"), m_pWork->in_cnt);
	SetString(x + 18, y, strText);
	y++;
	SetString(x, y, _T("FIFO(IN)  Length"));
	strText.Format(_T("%2d"), m_pWork->in_len);
	SetString(x + 18, y, strText);
	y++;

	// FIFO(�o��)
	SetString(x, y, _T("FIFO(OUT) Count"));
	strText.Format(_T("%2d"), m_pWork->out_cnt);
	SetString(x + 18, y, strText);
	y++;
	SetString(x, y, _T("FIFO(OUT) Length"));
	strText.Format(_T("%2d"), m_pWork->out_len);
	SetString(x + 18, y, strText);
}

//---------------------------------------------------------------------------
//
//	�Z�b�g�A�b�v(�p�����[�^)
//
//---------------------------------------------------------------------------
void FASTCALL CFDCWnd::SetupParam(int x, int y)
{
	CString strText;

	ASSERT(this);
	ASSERT(x >= 0);
	ASSERT(y >= 0);

	// CHRN
	SetString(x, y, _T("C"));
	strText.Format(_T("%02X"), m_pWork->chrn[0]);
	SetString(x + 6, y, strText);
	y++;
	SetString(x, y, _T("H"));
	strText.Format(_T("%02X"), m_pWork->chrn[1]);
	SetString(x + 6, y, strText);
	y++;
	SetString(x, y, _T("R"));
	strText.Format(_T("%02X"), m_pWork->chrn[2]);
	SetString(x + 6, y, strText);
	y++;
	SetString(x, y, _T("N"));
	strText.Format(_T("%02X"), m_pWork->chrn[3]);
	SetString(x + 6, y, strText);
	y++;

	// HD,US
	SetString(x, y, _T("HD"));
	strText.Format(_T("%02X"), m_pWork->hd);
	SetString(x + 6, y, strText);
	y++;
	SetString(x, y, _T("US"));
	strText.Format(_T("%02X"), m_pWork->us);
	SetString(x + 6, y, strText);
	y++;

	// MFM,MT,SK
	SetString(x, y, _T("MFM"));
	strText.Format(_T("%2d"), m_pWork->mfm);
	SetString(x + 6, y, strText);
	y++;
	SetString(x, y, _T("MT"));
	strText.Format(_T("%2d"), m_pWork->mt);
	SetString(x + 6, y, strText);
	y++;
	SetString(x, y, _T("SK"));
	strText.Format(_T("%2d"), m_pWork->sk);
	SetString(x + 6, y, strText);

	// ���̃J�����ֈړ�
	x += 12;
	y -= 8;

	// EOT,GAP3
	SetString(x, y, _T("EOT"));
	strText.Format(_T("%02X"), m_pWork->eot);
	SetString(x + 8, y, strText);
	y++;
	SetString(x, y, _T("GAP3"));
	if (m_pWork->cmd == FDC::write_id) {
		strText.Format(_T("%02X"), m_pWork->gpl);
	}
	else {
		strText.Format(_T("%02X"), m_pWork->gsl);
	}
	SetString(x + 8, y, strText);
	y++;

	// Head
	SetString(x, y, _T("HEAD"));
	if (m_pWork->load) {
		SetString(x + 6, y, _T("LOAD"));
	}
	else {
		SetString(x + 7, y, _T("UNL"));
	}
	y++;

	// CYL
	SetString(x, y, _T("PCN0"));
	strText.Format(_T("%02X"), m_pWork->cyl[0]);
	SetString(x + 8, y, strText);
	y++;
	SetString(x, y, _T("PCN1"));
	strText.Format(_T("%02X"), m_pWork->cyl[1]);
	SetString(x + 8, y, strText);
	y++;

	// NDM
	SetString(x, y, _T("NDM"));
	if (m_pWork->ndm) {
		SetString(x + 7, y, _T("CPU"));
	}
	else {
		SetString(x + 7, y, _T("DMA"));
	}
	y++;

	// err
	SetString(x, y, _T("ERR"));
	strText.Format(_T("%04X"), m_pWork->err);
	SetString(x + 6, y, strText);
	y++;

	// offset
	SetString(x, y, _T("OFF"));
	strText.Format(_T("%4X"), m_pWork->offset);
	SetString(x + 6, y, strText);
	y++;

	// length
	SetString(x, y, _T("LEN"));
	strText.Format(_T("%7X"), m_pWork->len);
	SetString(x + 3, y, strText);
}

//---------------------------------------------------------------------------
//
//	�Z�b�g�A�b�v(�X�e�[�^�X���W�X�^)
//
//---------------------------------------------------------------------------
void FASTCALL CFDCWnd::SetupSR(int x, int y)
{
	ASSERT(this);
	ASSERT(x >= 0);
	ASSERT(y >= 0);

	// �T�u�֐��ɔC����
	SetupSub(x, y, _T("[Status]"), SRDesc, m_pWork->sr);
}

//---------------------------------------------------------------------------
//
//	������(�X�e�[�^�X���W�X�^)
//
//---------------------------------------------------------------------------
LPCTSTR CFDCWnd::SRDesc[8] = {
	_T("Request for Master"),
	_T("Data Input/Output"),
	_T("Non-DMA Mode"),
	_T("FDC Busy"),
	_T("FDD#3 Busy"),
	_T("FDD#2 Busy"),
	_T("FDD#1 Busy"),
	_T("FDD#0 Busy")
};

//---------------------------------------------------------------------------
//
//	�Z�b�g�A�b�v(ST0)
//
//---------------------------------------------------------------------------
void FASTCALL CFDCWnd::SetupST0(int x, int y)
{
	ASSERT(this);
	ASSERT(x >= 0);
	ASSERT(y >= 0);

	// �T�u�֐��ɔC����
	SetupSub(x, y, _T("[ST0]"), ST0Desc, m_pWork->st[0]);
}

//---------------------------------------------------------------------------
//
//	������(ST0)
//
//---------------------------------------------------------------------------
LPCTSTR CFDCWnd::ST0Desc[8] = {
	_T("Interrupt Code 1"),
	_T("Interrupt Code 0"),
	_T("Seek End"),
	_T("Equipment Check"),
	_T("Not Ready"),
	_T("Head Address"),
	_T("Unit Select 1"),
	_T("Unit Select 0")
};

//---------------------------------------------------------------------------
//
//	�Z�b�g�A�b�v(ST1)
//
//---------------------------------------------------------------------------
void FASTCALL CFDCWnd::SetupST1(int x, int y)
{
	ASSERT(this);
	ASSERT(x >= 0);
	ASSERT(y >= 0);

	// �T�u�֐��ɔC����
	SetupSub(x, y, _T("[ST1]"), ST1Desc, m_pWork->st[1]);
}

//---------------------------------------------------------------------------
//
//	������(ST1)
//
//---------------------------------------------------------------------------
LPCTSTR CFDCWnd::ST1Desc[8] = {
	_T("End of Cylinder"),
	NULL,
	_T("ID/Data Error"),
	_T("Over Run"),
	NULL,
	_T("No Data"),
	_T("Not Writable"),
	_T("Missing Address Mark")
};

//---------------------------------------------------------------------------
//
//	�Z�b�g�A�b�v(ST2)
//
//---------------------------------------------------------------------------
void FASTCALL CFDCWnd::SetupST2(int x, int y)
{
	ASSERT(this);
	ASSERT(x >= 0);
	ASSERT(y >= 0);

	// �T�u�֐��ɔC����
	SetupSub(x, y, _T("[ST2]"), ST2Desc, m_pWork->st[2]);
}

//---------------------------------------------------------------------------
//
//	������(ST2)
//
//---------------------------------------------------------------------------
LPCTSTR CFDCWnd::ST2Desc[8] = {
	NULL,
	_T("Control Mark"),
	_T("Data Error"),
	_T("No Cylinder"),
	_T("Scan Equal Hit"),
	_T("Scan Not Satisfied"),
	_T("Bad Cylinder"),
	_T("Missing Data Mark")
};

//---------------------------------------------------------------------------
//
//	�Z�b�g�A�b�v(�T�u)
//
//---------------------------------------------------------------------------
void FASTCALL CFDCWnd::SetupSub(int x, int y, LPCTSTR lpszTitle, LPCTSTR *lpszDesc, DWORD data)
{
	int i;
	int j;
	TCHAR strBuf[0x40];
	CString strText;

	ASSERT(this);
	ASSERT(x >= 0);
	ASSERT(y >= 0);
	ASSERT(lpszTitle);
	ASSERT(lpszDesc);

	// �^�C�g���\��
	SetString(x, y, lpszTitle);
	y++;

	// ���[�v
	for (i=0; i<8; i++) {
		// b7-b0
		strText.Format(_T("b%1d:"), 7 - i);
		SetString(x, y, strText);

		// ������쐬
		if (lpszDesc[i]) {
			// �X�y�[�X�Ŗ��߂�
			for (j=0; j<0x40; j++) {
				strBuf[j] = _T(' ');
			}

			// �����񒷂����R�s�[
			ASSERT(_tcslen(lpszDesc[i]) < 0x40);
			memcpy(strBuf, lpszDesc[i], _tcslen(lpszDesc[i]) * sizeof(TCHAR));
		}
		else {
			// -�Ŗ��߂�
			for (j=0; j<0x40; j++) {
				strBuf[j] = _T('-');
			}
		}

		// �����͂����Ō��܂�
		strBuf[20] = _T('\0');

		// �Y������r�b�g�͔��]�ɂĕ\��
		if (data & 0x80) {
			Reverse(TRUE);
			SetString(x + 3, y, (LPCTSTR)strBuf);
			Reverse(FALSE);
		}
		else {
			SetString(x + 3, y, (LPCTSTR)strBuf);
		}

		// ����
		data <<= 1;
		y++;
	}
}

//===========================================================================
//
//	SCC�E�B���h�E
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	�R���X�g���N�^
//
//---------------------------------------------------------------------------
CSCCWnd::CSCCWnd()
{
	// �E�B���h�E�p�����[�^��`
	m_dwID = MAKEID('S', 'C', 'C', ' ');
	::GetMsg(IDS_SWND_SCC, m_strCaption);
	m_nWidth = 48;
	m_nHeight = 35;

	// SCC�擾
	m_pSCC = (SCC*)::GetVM()->SearchDevice(MAKEID('S', 'C', 'C', ' '));
	ASSERT(m_pSCC);
}

//---------------------------------------------------------------------------
//
//	�Z�b�g�A�b�v
//
//---------------------------------------------------------------------------
void FASTCALL CSCCWnd::Setup()
{
	int i;
	CString strText;
	SCC::scc_t scc;

	// �N���A
	Clear();

	// �f�[�^�擾
	ASSERT(m_pSCC);
	m_pSCC->GetSCC(&scc);

	// ���ڕ\��
	for (i=0;; i++) {
		if (!DescTable[i]) {
			break;
		}
		SetString(0, i, DescTable[i]);
	}

	// �`���l������
	SetString(19, 0, _T("A(RS-232C)"));
	SetupSCC(&scc.ch[0], 19, 1);
	SetString(35, 0, _T("B(MOUSE)"));
	SetupSCC(&scc.ch[1], 35, 1);
}

//---------------------------------------------------------------------------
//
//	������e�[�u��
//
//---------------------------------------------------------------------------
LPCTSTR CSCCWnd::DescTable[] = {
	_T("Channel"),
	_T("BaudRate"),
	_T("StopBit"),
	_T("Parity"),
	_T("Tx Enable"),
	_T("Tx Bit"),
	_T("Tx Break"),
	_T("Tx Busy"),
	_T("Tx Sent"),
	_T("Tx Buf Num"),
	_T("Tx Buf Read"),
	_T("Tx Buf Write"),
	_T("Rx Enable"),
	_T("Rx Bit"),
	_T("Rx Break"),
	_T("Rx FIFO"),
	_T("Rx Received"),
	_T("Rx Buf Num"),
	_T("Rx Buf Read"),
	_T("Rx Buf Write"),
	_T("Framing Err."),
	_T("Parity  Err."),
	_T("Overrun Err."),
	_T("Auto Mode"),
	_T("~CTS Line"),
	_T("~DCD Line"),
	_T("~RTS Line"),
	_T("~DTR Line"),
	_T("Rx Int Enable"),
	_T("Tx Int Enable"),
	_T("ExtInt Enable"),
	_T("Rx Int Pending"),
	_T("Rs Int Pending"),
	_T("Tx Int Pending"),
	_T("ExtInt Pending"),
	NULL
};

//---------------------------------------------------------------------------
//
//	�Z�b�g�A�b�v(�`���l��)
//
//---------------------------------------------------------------------------
void FASTCALL CSCCWnd::SetupSCC(SCC::ch_t *pCh, int x, int y)
{
	CString strText;

	ASSERT(this);
	ASSERT(pCh);
	ASSERT(x >= 0);
	ASSERT(y >= 0);

	// �{�[���[�g
	strText.Format(_T("%u"), pCh->baudrate);
	SetString(x, y, strText);
	y++;

	// �X�g�b�v�r�b�g
	switch (pCh->stopbit) {
		case 0:
			SetString(x, y, _T("NOT Async"));
			break;
		case 1:
			SetString(x, y, _T("1bit"));
			break;
		case 2:
			SetString(x, y, _T("1.5bit"));
			break;
		case 3:
			SetString(x, y, _T("2bit"));
			break;
		default:
			ASSERT(FALSE);
			break;
	}
	y++;

	// �p���e�B
	switch (pCh->parity) {
		case 0:
			SetString(x, y, _T("No"));
			break;
		case 1:
			SetString(x, y, _T("Odd"));
			break;
		case 2:
			SetString(x, y, _T("Even"));
			break;
		default:
			ASSERT(FALSE);
			break;
	}
	y++;

	// Tx�C�l�[�u��
	if (pCh->txen) {
		SetString(x, y, _T("Enable"));
	}
	else {
		SetString(x, y, _T("Disable"));
	}
	y++;

	// Tx�r�b�g
	strText.Format(_T("%d"), pCh->txbit);
	SetString(x, y, strText);
	y++;

	// Tx�u���[�N
	if (pCh->brk) {
		SetString(x, y, _T("Break"));
	}
	else {
		SetString(x, y, _T("Normal"));
	}
	y++;

	// Tx Busy
	if (pCh->tdf) {
		SetString(x, y, _T("Busy"));
	}
	else {
		SetString(x, y, _T("Ready"));
	}
	y++;

	// Tx Sent
	strText.Format(_T("%u"), pCh->txtotal);
	SetString(x, y, strText);
	y++;

	// Tx�o�b�t�@ Num
	strText.Format(_T("0x%03X"), pCh->txnum);
	SetString(x, y, strText);
	y++;

	// Tx�o�b�t�@ Read
	strText.Format(_T("0x%03X"), pCh->txread);
	SetString(x, y, strText);
	y++;

	// Tx�o�b�t�@ Write
	strText.Format(_T("0x%03X"), pCh->txwrite);
	SetString(x, y, strText);
	y++;

	// Rx�C�l�[�u��
	if (pCh->rxen) {
		SetString(x, y, _T("Enable"));
	}
	else {
		SetString(x, y, _T("Disable"));
	}
	y++;

	// Rx�r�b�g
	strText.Format(_T("%d"), pCh->rxbit);
	SetString(x, y, strText);
	y++;

	// Rx�u���[�N
	if (pCh->ba) {
		SetString(x, y, _T("Break"));
	}
	else {
		SetString(x, y, _T("Normal"));
	}
	y++;

	// Rx FIFO
	strText.Format(_T("%u"), pCh->rxfifo);
	SetString(x, y, strText);
	y++;

	// Rx Received
	strText.Format(_T("%u"), pCh->rxtotal);
	SetString(x, y, strText);
	y++;

	// Rx�o�b�t�@ Num
	strText.Format(_T("0x%03X"), pCh->rxnum);
	SetString(x, y, strText);
	y++;

	// Rx�o�b�t�@ Read
	strText.Format(_T("0x%03X"), pCh->rxread);
	SetString(x, y, strText);
	y++;

	// Rx�o�b�t�@ Write
	strText.Format(_T("0x%03X"), pCh->rxwrite);
	SetString(x, y, strText);
	y++;

	// �t���[�~���O�G���[
	if (pCh->framing) {
		SetString(x, y, _T("Error"));
	}
	else {
		SetString(x, y, _T("No"));
	}
	y++;

	// �p���e�B�G���[
	if (pCh->parerr) {
		SetString(x, y, _T("Error"));
	}
	else {
		SetString(x, y, _T("No"));
	}
	y++;

	// �I�[�o�[�����G���[
	if (pCh->overrun) {
		SetString(x, y, _T("Error"));
	}
	else {
		SetString(x, y, _T("No"));
	}
	y++;

	// �I�[�g���[�h
	if (pCh->aen) {
		SetString(x, y, _T("Auto"));
	}
	else {
		SetString(x, y, _T("No"));
	}
	y++;

	// CTS
	if (pCh->cts) {
		SetString(x, y, _T("L"));
	}
	else {
		SetString(x, y, _T("H"));
	}
	y++;

	// DCD
	if (pCh->dcd) {
		SetString(x, y, _T("L"));
	}
	else {
		SetString(x, y, _T("H"));
	}
	y++;

	// RTS
	if (pCh->rts) {
		SetString(x, y, _T("L"));
	}
	else {
		SetString(x, y, _T("H"));
	}
	y++;

	// DTR
	if (!pCh->dtrreq) {
		if (pCh->dtr) {
			SetString(x, y, _T("L"));
		}
		else {
			SetString(x, y, _T("H"));
		}
	}
	else {
		if (!(pCh->tdf)) {
			SetString(x, y, _T("L"));
		}
		else {
			SetString(x, y, _T("H"));
		}
	}
	y++;

	// RxInt En
	if (pCh->rxim >= 1) {
		SetString(x, y, _T("Enable"));
	}
	else {
		SetString(x, y, _T("Disable"));
	}
	y++;

	// TxInt En
	if (pCh->txie) {
		SetString(x, y, _T("Enable"));
	}
	else {
		SetString(x, y, _T("Disable"));
	}
	y++;

	// ExtInt En
	if (pCh->extie) {
		SetString(x, y, _T("Enable"));
	}
	else {
		SetString(x, y, _T("Disable"));
	}
	y++;

	// RxInt Pending
	if (pCh->rxip) {
		SetString(x, y, _T("Pending"));
	}
	else {
		SetString(x, y, _T("No"));
	}
	y++;

	// RsInt Pending
	if (pCh->rsip) {
		SetString(x, y, _T("Pending"));
	}
	else {
		SetString(x, y, _T("No"));
	}
	y++;

	// TxInt Pending
	if (pCh->txip) {
		SetString(x, y, _T("Pending"));
	}
	else {
		SetString(x, y, _T("No"));
	}
	y++;

	// ExtInt Pending
	if (pCh->extip) {
		SetString(x, y, _T("Pending"));
	}
	else {
		SetString(x, y, _T("No"));
	}
}

//===========================================================================
//
//	Cynthia�E�B���h�E
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	�R���X�g���N�^
//
//---------------------------------------------------------------------------
CCynthiaWnd::CCynthiaWnd()
{
	// �E�B���h�E�p�����[�^��`
	m_dwID = MAKEID('C', 'Y', 'N', 'T');
	::GetMsg(IDS_SWND_CYNTHIA, m_strCaption);
	m_nWidth = 46;
	m_nHeight = 5;

	// �X�v���C�g�R���g���[���擾
	m_pSprite = (Sprite*)::GetVM()->SearchDevice(MAKEID('S', 'P', 'R', ' '));
	ASSERT(m_pSprite);
}

//---------------------------------------------------------------------------
//
//	�Z�b�g�A�b�v
//
//---------------------------------------------------------------------------
void FASTCALL CCynthiaWnd::Setup()
{
	int i;
	int x;
	int y;
	CString string;
	Sprite::sprite_t spr;

	// �N���A�A�f�[�^�擾
	Clear();
	y = 0;
	x = 25;
	m_pSprite->GetSprite(&spr);

	// �\��
	SetString(0, y, "Access");
	if (spr.disp) {
		SetString(16, y, "Video");
	}
	else {
		SetString(18, y, "CPU");
	}

	// BG�T�C�Y
	SetString(x, y, "BG Size");
	if (spr.bg_size) {
		SetString(x + 16, y, "16x16");
	}
	else {
		SetString(x + 18, y, "8x8");
	}
	y++;

	// BG����
	for (i=0; i<2; i++) {
		// x����
		if (i == 0) {
			x = 0;
		}
		else {
			x = 25;
		}

		// �\��
		string.Format("BG%d Area", i);
		SetString(x, y + 0, string);
		string.Format("%1d", spr.bg_area[i]);
		SetString(x + 20, y + 0, string);
		string.Format("BG%d", i);
		SetString(x, y + 1, string);
		if (spr.bg_on[i]) {
			SetString(x + 14, y + 1, "Display");
		}
		else {
			SetString(x + 15, y + 1, "Hidden");
		}
		string.Format("BG%d Scroll X", i);
		SetString(x, y + 2, string);
		string.Format("%4d", spr.bg_scrlx[i]);
		SetString(x + 17, y + 2, string);
		string.Format("BG%d Scroll Y", i);
		SetString(x, y + 3, string);
		string.Format("%4d", spr.bg_scrly[i]);
		SetString(x + 17, y + 3, string);
	}
}

//===========================================================================
//
//	SASI�E�B���h�E
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	�R���X�g���N�^
//
//---------------------------------------------------------------------------
CSASIWnd::CSASIWnd()
{
	// �E�B���h�E�p�����[�^��`
	m_dwID = MAKEID('S', 'A', 'S', 'I');
	::GetMsg(IDS_SWND_SASI, m_strCaption);
	m_nWidth = 59;
	m_nHeight = 16;

	// SASI�擾
	m_pSASI = (SASI*)::GetVM()->SearchDevice(MAKEID('S', 'A', 'S', 'I'));
	ASSERT(m_pSASI);
}

//---------------------------------------------------------------------------
//
//	�Z�b�g�A�b�v
//
//---------------------------------------------------------------------------
void FASTCALL CSASIWnd::Setup()
{
	ASSERT(this);
	ASSERT(m_pSASI);

	// �N���A
	Clear();

	// SASI�f�[�^�擾
	m_pSASI->GetSASI(&m_sasi);

	// �R�}���h
	SetupCmd(0, 0);

	// �R���g���[��
	SetupCtrl(0, 1);

	// �h���C�u
	SetupDrive(0, 11);

	// �L���b�V��
	SetupCache(23, 0);
}

//---------------------------------------------------------------------------
//
//	�Z�b�g�A�b�v(�R�}���h)
//
//---------------------------------------------------------------------------
void FASTCALL CSASIWnd::SetupCmd(int x, int y)
{
	CString strCmd;

	ASSERT(this);
	ASSERT(x >= 0);
	ASSERT(y >= 0);

	// ������
	strCmd = _T("(UNKNOWN)");

	// �R�}���h��
	switch (m_sasi.cmd[0]) {
		case 0x00:
			strCmd = _T("TEST UNIT READY");
			break;
		case 0x01:
			strCmd = _T("REZERO UNIT");
			break;
		case 0x03:
			strCmd = _T("REQUEST SENSE");
			break;
		case 0x04:
		case 0x06:
			strCmd = _T("FORMAT UNIT");
			break;
		case 0x07:
			strCmd = _T("REASSIGN BLOCKS");
			break;
		case 0x08:
			strCmd = _T("READ(6)");
			break;
		case 0x0a:
			strCmd = _T("WRITE(10)");
			break;
		case 0x0b:
			strCmd = _T("SEEK(6)");
			break;
		case 0x0e:
			strCmd = _T("ASSIGN");
			break;
		case 0x12:
			strCmd = _T("INQUIRY");
			break;
		case 0x15:
			strCmd = _T("MODE SELECT");
			break;
		case 0x1a:
			strCmd = _T("MODE SENSE");
			break;
		case 0x1b:
			strCmd = _T("START STOP UNIT");
			break;
		case 0x1e:
			strCmd = _T("PREVENT/ALLOW REMOVAL");
			break;
		case 0x25:
			strCmd = _T("READ CAPACITY");
			break;
		case 0x28:
			strCmd = _T("READ(10)");
			break;
		case 0x2a:
			strCmd = _T("WRITE(10)");
			break;
		case 0x2b:
			strCmd = _T("SEEK(10)");
			break;
		case 0x2e:
			strCmd = _T("WRITE and VERIFY");
			break;
		case 0x2f:
			strCmd = _T("VERIFY");
			break;
		case 0xc2:
			strCmd = _T("SPECIFY");
			break;
	}

	// �A�C�h���t�F�[�Y�̃`�F�b�N
	if (!m_sasi.bsy) {
		strCmd = _T("(IDLE)");
	}

	SetString(x, y, strCmd);
}

//---------------------------------------------------------------------------
//
//	�Z�b�g�A�b�v(�R���g���[��)
//
//---------------------------------------------------------------------------
void FASTCALL CSASIWnd::SetupCtrl(int x, int y)
{
	CString strPhase;
	CString strText;

	ASSERT(this);
	ASSERT(x >= 0);
	ASSERT(y >= 0);

	// �t�F�[�Y
	switch (m_sasi.phase) {
		case SASI::busfree:
			strPhase = _T("Busfree");
			break;
		case SASI::selection:
			strPhase = _T("Selection");
			break;
		case SASI::command:
			strPhase = _T("Command");
			break;
		case SASI::execute:
			strPhase = _T("Execute");
			break;
		case SASI::read:
			strPhase = _T("Read");
			break;
		case SASI::write:
			strPhase = _T("Write");
			break;
		case SASI::status:
			strPhase = _T("Status");
			break;
		case SASI::message:
			strPhase = _T("Message");
			break;
		default:
			ASSERT(FALSE);
			break;
	}
	SetString(x, y, _T("Phase"));
	SetString(x + 11, y, strPhase);
	y++;

	// �Z���N�g
	SetString(x, y, _T("SEL"));
	if (m_sasi.sel) {
		SetString(x + 11, y, _T("Select"));
	}
	else {
		SetString(x + 11, y, _T("No"));
	}
	y++;

	// ���b�Z�[�W
	SetString(x, y, _T("MSG"));
	if (m_sasi.msg) {
		SetString(x + 11, y, _T("Message"));
	}
	else {
		SetString(x + 11, y, _T("No"));
	}
	y++;

	// �R�}���h/�f�[�^
	SetString(x, y, _T("C/D"));
	if (m_sasi.cd) {
		SetString(x + 11, y, _T("Command"));
	}
	else {
		SetString(x + 11, y, _T("Data"));
	}
	y++;

	// Input/Output
	SetString(x, y, _T("I/O"));
	if (m_sasi.io) {
		SetString(x + 11, y, _T("Input"));
	}
	else {
		SetString(x + 11, y, _T("Output"));
	}
	y++;

	// Busy
	SetString(x, y, _T("BSY"));
	if (m_sasi.bsy) {
		SetString(x + 11, y, _T("Busy"));
	}
	else {
		SetString(x + 11, y, _T("No"));
	}
	y++;

	// REQ
	SetString(x, y, _T("REQ"));
	if (m_sasi.req) {
		SetString(x + 11, y, _T("Request"));
	}
	else {
		SetString(x + 11, y, _T("No"));
	}
	y++;

	// Message
	SetString(x, y, _T("Message"));
	strText.Format(_T("%02X"), m_sasi.message);
	SetString(x + 11, y, strText);
	y++;

	// Status
	SetString(x, y, _T("Status"));
	strText.Format(_T("%02X"), m_sasi.status);
	SetString(x + 11, y, strText);
}

//---------------------------------------------------------------------------
//
//	�Z�b�g�A�b�v(�h���C�u)
//
//---------------------------------------------------------------------------
void FASTCALL CSASIWnd::SetupDrive(int x, int y)
{
	CString strText;
	Disk *pDisk;
	Disk::disk_t disk;
	DWORD dwSize;

	ASSERT(this);
	ASSERT(x >= 0);
	ASSERT(y >= 0);

	// ������
	pDisk = NULL;

	// ID
	SetString(x, y, _T("ID"));
	if (m_sasi.cmd[1] & 0x20) {
		// LUN����
		strText.Format(_T("%d (LUN:1)"), m_sasi.ctrl);
		pDisk = m_sasi.disk[(m_sasi.ctrl << 1) + 1];
	}
	else {
		// LUN�Ȃ�
		strText.Format(_T("%d"), m_sasi.ctrl);
		pDisk = m_sasi.disk[m_sasi.ctrl << 1];
	}
	SetString(x + 11, y, strText);
	y++;

	// �f�[�^�擾
	ASSERT(pDisk);
	pDisk->GetDisk(&disk);

	// ���
	SetString(x, y, _T("Type"));
	switch (disk.id) {
		case MAKEID('S', 'A', 'H', 'D'):
			strText = _T("SASI-HD");
			break;
		case MAKEID('S', 'C', 'H', 'D'):
			strText = _T("SCSI-HD");
			break;
		case MAKEID('S', 'C', 'M', 'O'):
			strText = _T("SCSI-MO");
			break;
		case MAKEID('N', 'U', 'L', 'L'):
			strText = _T("(No Unit)");
			break;
		default:
			ASSERT(FALSE);
			break;
	}
	SetString(x + 11, y, strText);
	y++;

	// �e��
	SetString(x, y, _T("Capacity"));
	dwSize = disk.blocks;
	if (!disk.ready) {
		dwSize = 0;
	}
	ASSERT((disk.size == 0) || (disk.size == 8) || (disk.size == 9) || (disk.size == 11));
	if (disk.size == 8) {
		dwSize >>= 1;
	}
	if (disk.size == 11) {
		dwSize <<= 2;
	}
	strText.Format(_T("%d.%1dMB"), (dwSize >> 11), (dwSize & 0x7ff) / 205);
	SetString(x + 11, y, strText);
	y++;

	// �A�N�Z�X�^�C�v
	SetString(x, y, _T("Access"));
	if (disk.writep) {
		SetString(x + 11, y, _T("RO"));
	}
	else {
		SetString(x + 11, y, _T("R/W"));
	}
	y++;

	// ���f�B�A����
	SetString(x, y, _T("Media"));
	if (disk.removable) {
		SetString(x + 11, y, _T("Removable"));
	}
	else {
		SetString(x + 11, y, _T("Fixed"));
	}
}

//---------------------------------------------------------------------------
//
//	�Z�b�g�A�b�v(�L���b�V��)
//
//---------------------------------------------------------------------------
void FASTCALL CSASIWnd::SetupCache(int x, int y)
{
	int nCache;
	int nLast;
	int nTrack;
	DWORD dwSerial;
	DWORD dwMax;
	CString strText;
	Disk::disk_t disk;

	ASSERT(this);
	ASSERT(x >= 0);
	ASSERT(y >= 0);

	// ���݊��蓖�Ă��Ă���h���C�u�����邩
	if (!m_sasi.current) {
		return;
	}
	// ���f�B��
	if (!m_sasi.current->IsReady()) {
		return;
	}

	// �������[�N�擾
	m_sasi.current->GetDisk(&disk);

	// �ł��Ō�Ɏg��ꂽ�C���f�b�N�X��T��
	dwMax = 0;
	nLast = -1;
	for (nCache=0; nCache<DiskCache::CacheMax; nCache++) {
		// �g�p����
		if (disk.dcache->GetCache(nCache, nTrack, dwSerial)) {
			// �V���A�������傫����
			if (dwSerial > dwMax) {
				nLast = nCache;
				dwMax = dwSerial;
			}
		}
	}

	// ���[�v
	for (nCache=0; nCache<DiskCache::CacheMax; nCache++) {
		// �g�p����
		if (disk.dcache->GetCache(nCache, nTrack, dwSerial)) {
			// ���]
			if (nCache == nLast) {
				Reverse(TRUE);
			}

			// ������쐬
			strText.Format(_T("Cache%1X: Track%08X Serial%08X"),
								nCache, nTrack, dwSerial);

			// �Z�b�g
			SetString(x, y, strText);

			// ���]
			if (nCache == nLast) {
				Reverse(FALSE);
			}
		}

		// ����
		y++;
	}
}

//===========================================================================
//
//	MIDI�E�B���h�E
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	�R���X�g���N�^
//
//---------------------------------------------------------------------------
CMIDIWnd::CMIDIWnd()
{
	// �E�B���h�E�p�����[�^��`
	m_dwID = MAKEID('M', 'I', 'D', 'I');
	::GetMsg(IDS_SWND_MIDI, m_strCaption);
	m_nWidth = 42;
	m_nHeight = 24;

	// MIDI�擾
	m_pMIDI = (MIDI*)::GetVM()->SearchDevice(MAKEID('M', 'I', 'D', 'I'));
	ASSERT(m_pMIDI);
}

//---------------------------------------------------------------------------
//
//	�Z�b�g�A�b�v
//
//---------------------------------------------------------------------------
void FASTCALL CMIDIWnd::Setup()
{
	int x;
	int y;
	CString strText;

	ASSERT(this);
	ASSERT(m_pMIDI);

	// MIDI�����f�[�^�擾
	m_pMIDI->GetMIDI(&m_midi);

	// �N���A
	Clear();

	// �R���g���[��
	x = 0;
	y = 0;
	SetString(x, y, _T("<Ctrl>"));
	y++;
	SetupCtrl(x, y);

	// ���荞��
	x = 15;
	y = 0;
	SetString(x, y, _T("<Int>"));
	y++;
	SetupInt(x, y);

	// ���M
	x = 0;
	y = 7;
	SetString(x, y, _T("<Trans>"));
	y++;
	SetupTrans(x, y);

	// ��M
	x = 15;
	y = 7;
	SetString(x, y, _T("<Recv>"));
	y++;
	SetupRecv(x, y);

	// ���A���^�C�����M
	x = 0;
	y = 18;
	SetString(x, y, _T("<R-Trans>"));
	y++;
	SetupRT(x, y);

	// ���A���^�C����M
	x = 15;
	y = 18;
	SetString(x, y, _T("<R-Recv>"));
	y++;
	SetupRR(x, y);

	// �J�E���^
	x = 30;
	y = 0;
	SetString(x, y, _T("<Count>"));
	y++;
	SetupCount(x, y);

	// �A�h���X�n���^
	x = 30;
	y = 12;
	SetString(x, y, _T("<A-Hunter>"));
	y++;
	SetupHunter(x, y);

	// FSK
	x = 30;
	y = 16;
	SetString(x, y, _T("<FSK>"));
	y++;
	SetupFSK(x, y);

	// GPIO
	x = 30;
	y = 20;
	SetString(x, y, _T("<GPIO>"));
	y++;
	SetupGPIO(x, y);
}

//---------------------------------------------------------------------------
//
//	������e�[�u��(�R���g���[��)
//
//---------------------------------------------------------------------------
LPCTSTR CMIDIWnd::DescCtrl[] = {
	_T("Reset"),
	_T("Access"),
	_T("BID"),
	_T("WDR"),
	_T("RGR"),
	NULL
};

//---------------------------------------------------------------------------
//
//	�Z�b�g�A�b�v(�R���g���[��)
//
//---------------------------------------------------------------------------
void FASTCALL CMIDIWnd::SetupCtrl(int x, int y)
{
	int i;
	CString strText;

	ASSERT(this);
	ASSERT((x >= 0) && (x < m_nWidth));
	ASSERT((y >= 0) && (y < m_nHeight));

	// ������e�[�u��
	for (i=0;; i++) {
		if (!DescCtrl[i]) {
			break;
		}
		SetString(x, y + i, DescCtrl[i]);
	}

	// �I�t�Z�b�g
	x += 8;

	// ���Z�b�g
	if (m_midi.reset) {
		SetString(x, y, _T("Reset"));
	}
	else {
		SetString(x, y, _T("No"));
	}
	y++;

	// �A�N�Z�X
	if (m_midi.access) {
		SetString(x, y, _T("Access"));
	}
	else {
		SetString(x, y, _T("No"));
	}
	y++;

	// �{�[�hID
	strText.Format(_T("%d"), m_midi.bid);
	SetString(x, y, strText);
	y++;

	// WDR
	strText.Format(_T("%02X"), m_midi.wdr);
	SetString(x, y, strText);
	y++;

	// RGR
	strText.Format(_T("%02X"), m_midi.rgr);
	SetString(x, y, strText);
}

//---------------------------------------------------------------------------
//
//	�Z�b�g�A�b�v(���荞��)
//
//---------------------------------------------------------------------------
void FASTCALL CMIDIWnd::SetupInt(int x, int y)
{
	int i;
	CString strText;

	ASSERT(this);
	ASSERT((x >= 0) && (x < m_nWidth));
	ASSERT((y >= 0) && (y < m_nHeight));

	// ������e�[�u��
	for (i=0;; i++) {
		if (!DescInt[i]) {
			break;
		}
		SetString(x, y + i, DescInt[i]);
	}

	// �I�t�Z�b�g
	x += 8;

	// ���x��
	strText.Format(_T("%d"), m_midi.ilevel);
	SetString(x, y, strText);
	y++;

	// IVR
	strText.Format(_T("%02X"), m_midi.ivr);
	SetString(x, y, strText);
	y++;

	// ISR
	strText.Format(_T("%02X"), m_midi.isr);
	SetString(x, y, strText);
	y++;

	// IMR
	strText.Format(_T("%02X"), m_midi.imr);
	SetString(x, y, strText);
	y++;

	// IER
	strText.Format(_T("%02X"), m_midi.ier);
	SetString(x, y, strText);
}

//---------------------------------------------------------------------------
//
//	������e�[�u��(���荞��)
//
//---------------------------------------------------------------------------
LPCTSTR CMIDIWnd::DescInt[] = {
	_T("Level"),
	_T("IVR"),
	_T("ISR"),
	_T("IMR"),
	_T("IER"),
	NULL
};

//---------------------------------------------------------------------------
//
//	�Z�b�g�A�b�v(���M)
//
//---------------------------------------------------------------------------
void FASTCALL CMIDIWnd::SetupTrans(int x, int y)
{
	int i;
	CString strText;

	ASSERT(this);
	ASSERT((x >= 0) && (x < m_nWidth));
	ASSERT((y >= 0) && (y < m_nHeight));

	// ������e�[�u��
	for (i=0;; i++) {
		if (!DescTrans[i]) {
			break;
		}
		SetString(x, y + i, DescTrans[i]);
	}

	// �I�t�Z�b�g
	x += 8;

	// TRR
	strText.Format(_T("%02X"), m_midi.trr);
	SetString(x, y, strText);
	y++;

	// TMR
	strText.Format(_T("%02X"), m_midi.tmr);
	SetString(x, y, strText);
	y++;

	// TBS
	strText.Format(_T("%02X"), m_midi.tbs);
	SetString(x, y, strText);
	y++;

	// TCR
	strText.Format(_T("%02X"), m_midi.tcr);
	SetString(x, y, strText);
	y++;

	// TCN
	strText.Format(_T("%03X"), m_midi.tcn);
	SetString(x, y, strText);
	y++;

	// Num
	strText.Format(_T("%d"), m_midi.normnum);
	SetString(x, y, strText);
	y++;

	// Read
	strText.Format(_T("%d"), m_midi.normread);
	SetString(x, y, strText);
	y++;

	// Write
	strText.Format(_T("%d"), m_midi.normwrite);
	SetString(x, y, strText);
	y++;

	// Total
	strText.Format(_T("%-6d"), m_midi.normtotal);
	SetString(x, y, strText);
}

//---------------------------------------------------------------------------
//
//	������e�[�u��(���M)
//
//---------------------------------------------------------------------------
LPCTSTR CMIDIWnd::DescTrans[] = {
	_T("TRR"),
	_T("TMR"),
	_T("TBS"),
	_T("TCR"),
	_T("TCN"),
	_T("Num"),
	_T("Read"),
	_T("Write"),
	_T("Total"),
	NULL
};

//---------------------------------------------------------------------------
//
//	�Z�b�g�A�b�v(��M)
//
//---------------------------------------------------------------------------
void FASTCALL CMIDIWnd::SetupRecv(int x, int y)
{
	int i;
	CString strText;

	ASSERT(this);
	ASSERT((x >= 0) && (x < m_nWidth));
	ASSERT((y >= 0) && (y < m_nHeight));

	// ������e�[�u��
	for (i=0;; i++) {
		if (!DescRecv[i]) {
			break;
		}
		SetString(x, y + i, DescRecv[i]);
	}

	// �I�t�Z�b�g
	x += 8;

	// RRR
	strText.Format(_T("%02X"), m_midi.rrr);
	SetString(x, y, strText);
	y++;

	// RMR
	strText.Format(_T("%02X"), m_midi.rmr);
	SetString(x, y, strText);
	y++;

	// RSR
	strText.Format(_T("%02X"), m_midi.rsr);
	SetString(x, y, strText);
	y++;

	// RCR
	strText.Format(_T("%02X"), m_midi.rcr);
	SetString(x, y, strText);
	y++;

	// RCN
	strText.Format(_T("%03X"), m_midi.rcn);
	SetString(x, y, strText);
	y++;

	// Num
	strText.Format(_T("%d"), m_midi.stdnum);
	SetString(x, y, strText);
	y++;

	// Read
	strText.Format(_T("%d"), m_midi.stdread);
	SetString(x, y, strText);
	y++;

	// Write
	strText.Format(_T("%d"), m_midi.stdwrite);
	SetString(x, y, strText);
	y++;

	// Total
	strText.Format(_T("%-6d"), m_midi.stdtotal);
	SetString(x, y, strText);
}

//---------------------------------------------------------------------------
//
//	������e�[�u��(��M)
//
//---------------------------------------------------------------------------
LPCTSTR CMIDIWnd::DescRecv[] = {
	_T("RRR"),
	_T("RMR"),
	_T("RSR"),
	_T("RCR"),
	_T("RCN"),
	_T("Num"),
	_T("Read"),
	_T("Write"),
	_T("Total"),
	NULL
};

//---------------------------------------------------------------------------
//
//	�Z�b�g�A�b�v(���A���^�C�����M)
//
//---------------------------------------------------------------------------
void FASTCALL CMIDIWnd::SetupRT(int x, int y)
{
	int i;
	CString strText;

	ASSERT(this);
	ASSERT((x >= 0) && (x < m_nWidth));
	ASSERT((y >= 0) && (y < m_nHeight));

	// ������e�[�u��
	for (i=0;; i++) {
		if (!DescRT[i]) {
			break;
		}
		SetString(x, y + i, DescRT[i]);
	}

	// �I�t�Z�b�g
	x += 8;

	// DMR
	strText.Format(_T("%02X"), m_midi.dmr);
	SetString(x, y, strText);
	y++;

	// Num
	strText.Format(_T("%d"), m_midi.rtnum);
	SetString(x, y, strText);
	y++;

	// Read
	strText.Format(_T("%d"), m_midi.rtread);
	SetString(x, y, strText);
	y++;

	// Write
	strText.Format(_T("%d"), m_midi.rtwrite);
	SetString(x, y, strText);
	y++;

	// Total
	strText.Format(_T("%-6d"), m_midi.rttotal);
	SetString(x, y, strText);
}

//---------------------------------------------------------------------------
//
//	������e�[�u��(���A���^�C�����M)
//
//---------------------------------------------------------------------------
LPCTSTR CMIDIWnd::DescRT[] = {
	_T("DMR"),
	_T("Num"),
	_T("Read"),
	_T("Write"),
	_T("Total"),
	NULL
};

//---------------------------------------------------------------------------
//
//	�Z�b�g�A�b�v(���A���^�C����M)
//
//---------------------------------------------------------------------------
void FASTCALL CMIDIWnd::SetupRR(int x, int y)
{
	int i;
	CString strText;

	ASSERT(this);
	ASSERT((x >= 0) && (x < m_nWidth));
	ASSERT((y >= 0) && (y < m_nHeight));

	// ������e�[�u��
	for (i=0;; i++) {
		if (!DescRR[i]) {
			break;
		}
		SetString(x, y + i, DescRR[i]);
	}

	// �I�t�Z�b�g
	x += 8;

	// DCR
	strText.Format(_T("%02X"), m_midi.dcr);
	SetString(x, y, strText);
	y++;

	// Num
	strText.Format(_T("%d"), m_midi.rrnum);
	SetString(x, y, strText);
	y++;

	// Read
	strText.Format(_T("%d"), m_midi.rrread);
	SetString(x, y, strText);
	y++;

	// Write
	strText.Format(_T("%d"), m_midi.rrwrite);
	SetString(x, y, strText);
	y++;

	// Total
	strText.Format(_T("%-6d"), m_midi.rrtotal);
	SetString(x, y, strText);
}

//---------------------------------------------------------------------------
//
//	������e�[�u��(���A���^�C����M)
//
//---------------------------------------------------------------------------
LPCTSTR CMIDIWnd::DescRR[] = {
	_T("DCR"),
	_T("Num"),
	_T("Read"),
	_T("Write"),
	_T("Total"),
	NULL
};

//---------------------------------------------------------------------------
//
//	�Z�b�g�A�b�v(�J�E���^)
//
//---------------------------------------------------------------------------
void FASTCALL CMIDIWnd::SetupCount(int x, int y)
{
	int i;
	CString strText;

	ASSERT(this);
	ASSERT((x >= 0) && (x < m_nWidth));
	ASSERT((y >= 0) && (y < m_nHeight));

	// ������e�[�u��
	for (i=0;; i++) {
		if (!DescCount[i]) {
			break;
		}
		SetString(x, y + i, DescCount[i]);
	}

	// �I�t�Z�b�g
	x += 8;

	// CCR
	strText.Format(_T("%02X"), m_midi.ccr);
	SetString(x, y, strText);
	y++;

	// CDR
	strText.Format(_T("%02X"), m_midi.cdr);
	SetString(x, y, strText);
	y++;

	// CTR
	strText.Format(_T("%02X"), m_midi.ctr);
	SetString(x, y, strText);
	y++;

	// SRR
	strText.Format(_T("%02X"), m_midi.srr);
	SetString(x, y, strText);
	y++;

	// SCR
	strText.Format(_T("%02X"), m_midi.scr);
	SetString(x, y, strText);
	y++;

	// SCT
	strText.Format(_T("%02X"), m_midi.sct);
	SetString(x, y, strText);
	y++;

	// SPR
	strText.Format(_T("%04X"), m_midi.spr);
	SetString(x, y, strText);
	y++;

	// STR
	strText.Format(_T("%04X"), m_midi.str & 0xffff);
	SetString(x, y, strText);
	y++;

	// GTR
	strText.Format(_T("%04X"), m_midi.gtr);
	SetString(x, y, strText);
	y++;

	// MTR
	strText.Format(_T("%04X"), m_midi.mtr);
	SetString(x, y, strText);
}

//---------------------------------------------------------------------------
//
//	������e�[�u��(�J�E���^)
//
//---------------------------------------------------------------------------
LPCTSTR CMIDIWnd::DescCount[] = {
	_T("CCR"),
	_T("CDR"),
	_T("CTR"),
	_T("SRR"),
	_T("SCR"),
	_T("SCT"),
	_T("SPR"),
	_T("STR"),
	_T("GTR"),
	_T("MTR"),
	NULL
};

//---------------------------------------------------------------------------
//
//	�Z�b�g�A�b�v(�A�h���X�n���^)
//
//---------------------------------------------------------------------------
void FASTCALL CMIDIWnd::SetupHunter(int x, int y)
{
	int i;
	CString strText;

	ASSERT(this);
	ASSERT((x >= 0) && (x < m_nWidth));
	ASSERT((y >= 0) && (y < m_nHeight));

	// ������e�[�u��
	for (i=0;; i++) {
		if (!DescHunter[i]) {
			break;
		}
		SetString(x, y + i, DescHunter[i]);
	}

	// �I�t�Z�b�g
	x += 8;

	// AMR
	strText.Format(_T("%02X"), m_midi.amr);
	SetString(x, y, strText);
	y++;

	// ADR
	strText.Format(_T("%02X"), m_midi.adr);
	SetString(x, y, strText);
}

//---------------------------------------------------------------------------
//
//	������e�[�u��(�A�h���X�n���^)
//
//---------------------------------------------------------------------------
LPCTSTR CMIDIWnd::DescHunter[] = {
	_T("AMR"),
	_T("ADR"),
	NULL
};

//---------------------------------------------------------------------------
//
//	�Z�b�g�A�b�v(FSK)
//
//---------------------------------------------------------------------------
void FASTCALL CMIDIWnd::SetupFSK(int x, int y)
{
	int i;
	CString strText;

	ASSERT(this);
	ASSERT((x >= 0) && (x < m_nWidth));
	ASSERT((y >= 0) && (y < m_nHeight));

	// ������e�[�u��
	for (i=0;; i++) {
		if (!DescFSK[i]) {
			break;
		}
		SetString(x, y + i, DescFSK[i]);
	}

	// �I�t�Z�b�g
	x += 8;

	// FSR
	strText.Format(_T("%02X"), m_midi.fsr);
	SetString(x, y, strText);
	y++;

	// FCR
	strText.Format(_T("%02X"), m_midi.fcr);
	SetString(x, y, strText);
}

//---------------------------------------------------------------------------
//
//	������e�[�u��(FSK)
//
//---------------------------------------------------------------------------
LPCTSTR CMIDIWnd::DescFSK[] = {
	_T("FSR"),
	_T("FCR"),
	NULL
};

//---------------------------------------------------------------------------
//
//	�Z�b�g�A�b�v(GPIO)
//
//---------------------------------------------------------------------------
void FASTCALL CMIDIWnd::SetupGPIO(int x, int y)
{
	int i;
	CString strText;

	ASSERT(this);
	ASSERT((x >= 0) && (x < m_nWidth));
	ASSERT((y >= 0) && (y < m_nHeight));

	// ������e�[�u��
	for (i=0;; i++) {
		if (!DescGPIO[i]) {
			break;
		}
		SetString(x, y + i, DescGPIO[i]);
	}

	// �I�t�Z�b�g
	x += 8;

	// EDR
	strText.Format(_T("%02X"), m_midi.edr);
	SetString(x, y, strText);
	y++;

	// EOR
	strText.Format(_T("%02X"), m_midi.eor);
	SetString(x, y, strText);
	y++;

	// EIR
	strText.Format(_T("%02X"), m_midi.eir);
	SetString(x, y, strText);
}

//---------------------------------------------------------------------------
//
//	������e�[�u��(GPIO)
//
//---------------------------------------------------------------------------
LPCTSTR CMIDIWnd::DescGPIO[] = {
	_T("EDR"),
	_T("EOR"),
	_T("EIR"),
	NULL
};

//===========================================================================
//
//	SCSI�E�B���h�E
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	�R���X�g���N�^
//
//---------------------------------------------------------------------------
CSCSIWnd::CSCSIWnd()
{
	// �E�B���h�E�p�����[�^��`
	m_dwID = MAKEID('S', 'C', 'S', 'I');
	::GetMsg(IDS_SWND_SCSI, m_strCaption);
	m_nWidth = 39;
	m_nHeight = 22;

	// SCSI�擾
	m_pSCSI = (SCSI*)::GetVM()->SearchDevice(MAKEID('S', 'C', 'S', 'I'));
	ASSERT(m_pSCSI);
}

//---------------------------------------------------------------------------
//
//	�Z�b�g�A�b�v
//
//---------------------------------------------------------------------------
void FASTCALL CSCSIWnd::Setup()
{
	ASSERT(this);
	ASSERT(m_pSCSI);

	// �N���A
	Clear();

	// SCSI�f�[�^�擾
	m_pSCSI->GetSCSI(&m_scsi);

	// �R�}���h
	SetupCmd(0, 0);

	// �R���g���[��
	SetupCtrl(0, 1);

	// �h���C�u
	SetupDrive(0, 14);

	// ���W�X�^
	SetupReg(26, 0);

	// CDB
	SetupCDB(26, 12);
}

//---------------------------------------------------------------------------
//
//	�Z�b�g�A�b�v(�R�}���h)
//
//---------------------------------------------------------------------------
void FASTCALL CSCSIWnd::SetupCmd(int x, int y)
{
	CString strCmd;

	ASSERT(this);
	ASSERT(x >= 0);
	ASSERT(y >= 0);

	// ������
	strCmd = _T("(UNKNOWN)");

	// �R�}���h��
	switch (m_scsi.cmd[0]) {
		case 0x00:
			strCmd = _T("TEST UNIT READY");
			break;
		case 0x01:
			strCmd = _T("REZERO UNIT");
			break;
		case 0x03:
			strCmd = _T("REQUEST SENSE");
			break;
		case 0x04:
			strCmd = _T("FORMAT UNIT");
			break;
		case 0x07:
			strCmd = _T("REASSIGN BLOCKS");
			break;
		case 0x08:
			strCmd = _T("READ(6)");
			break;
		case 0x0a:
			strCmd = _T("WRITE(10)");
			break;
		case 0x0b:
			strCmd = _T("SEEK(6)");
			break;
		case 0x0e:
			strCmd = _T("ASSIGN");
			break;
		case 0x12:
			strCmd = _T("INQUIRY");
			break;
		case 0x15:
			strCmd = _T("MODE SELECT");
			break;
		case 0x1a:
			strCmd = _T("MODE SENSE");
			break;
		case 0x1b:
			strCmd = _T("START STOP UNIT");
			break;
		case 0x1e:
			strCmd = _T("PREVENT/ALLOW REMOVAL");
			break;
		case 0x25:
			strCmd = _T("READ CAPACITY");
			break;
		case 0x28:
			strCmd = _T("READ(10)");
			break;
		case 0x2a:
			strCmd = _T("WRITE(10)");
			break;
		case 0x2b:
			strCmd = _T("SEEK(10)");
			break;
		case 0x2e:
			strCmd = _T("WRITE and VERIFY");
			break;
		case 0x2f:
			strCmd = _T("VERIFY");
			break;
		case 0x43:
			strCmd = _T("READ TOC");
			break;
	}

	// �A�C�h���t�F�[�Y�̃`�F�b�N
	if (m_scsi.phase == SCSI::busfree) {
		strCmd = _T("(IDLE)");
	}

	SetString(x, y, strCmd);
}

//---------------------------------------------------------------------------
//
//	�Z�b�g�A�b�v(�R���g���[��)
//
//---------------------------------------------------------------------------
void FASTCALL CSCSIWnd::SetupCtrl(int x, int y)
{
	CString strPhase;
	CString strText;

	ASSERT(this);
	ASSERT(x >= 0);
	ASSERT(y >= 0);

	// �t�F�[�Y
	switch (m_scsi.phase) {
		case SCSI::busfree:
			strPhase = _T("Busfree");
			break;
		case SCSI::arbitration:
			strPhase = _T("Arbitration");
			break;
		case SCSI::selection:
			strPhase = _T("Selection");
			break;
		case SCSI::reselection:
			strPhase = _T("Re-selection");
			break;
		case SCSI::command:
			strPhase = _T("Command");
			break;
		case SCSI::execute:
			strPhase = _T("Execute");
			break;
		case SCSI::msgin:
			strPhase = _T("Message In");
			break;
		case SCSI::msgout:
			strPhase = _T("Message Out");
			break;
		case SCSI::datain:
			strPhase = _T("Data In");
			break;
		case SCSI::dataout:
			strPhase = _T("Data Out");
			break;
		case SCSI::status:
			strPhase = _T("Status");
			break;
		default:
			ASSERT(FALSE);
			break;
	}
	SetString(x, y, _T("Phase"));
	SetString(x + 11, y, strPhase);
	y++;

	// �Z���N�g
	SetString(x, y, _T("SEL"));
	if (m_scsi.sel) {
		SetString(x + 11, y, _T("Select"));
	}
	else {
		SetString(x + 11, y, _T("No"));
	}
	y++;

	// �A�e���V����
	SetString(x, y, _T("ATN"));
	if (m_scsi.atn) {
		SetString(x + 11, y, _T("Attention"));
	}
	else {
		SetString(x + 11, y, _T("No"));
	}
	y++;

	// ���b�Z�[�W
	SetString(x, y, _T("MSG"));
	if (m_scsi.msg) {
		SetString(x + 11, y, _T("Message"));
	}
	else {
		SetString(x + 11, y, _T("No"));
	}
	y++;

	// �R�}���h/�f�[�^
	SetString(x, y, _T("C/D"));
	if (m_scsi.cd) {
		SetString(x + 11, y, _T("Command"));
	}
	else {
		SetString(x + 11, y, _T("Data"));
	}
	y++;

	// Input/Output
	SetString(x, y, _T("I/O"));
	if (m_scsi.io) {
		SetString(x + 11, y, _T("Input"));
	}
	else {
		SetString(x + 11, y, _T("Output"));
	}
	y++;

	// Busy
	SetString(x, y, _T("BSY"));
	if (m_scsi.bsy) {
		SetString(x + 11, y, _T("Busy"));
	}
	else {
		SetString(x + 11, y, _T("No"));
	}
	y++;

	// REQ
	SetString(x, y, _T("REQ"));
	if (m_scsi.req) {
		SetString(x + 11, y, _T("Request"));
	}
	else {
		SetString(x + 11, y, _T("No"));
	}
	y++;

	// ACK
	SetString(x, y, _T("ACK"));
	if (m_scsi.ack) {
		SetString(x + 11, y, _T("ACK"));
	}
	else {
		SetString(x + 11, y, _T("No"));
	}
	y++;

	// ���Z�b�g
	SetString(x, y, _T("RST"));
	if (m_scsi.rst) {
		SetString(x + 11, y, _T("Reset"));
	}
	else {
		SetString(x + 11, y, _T("No"));
	}
	y++;

	// Message
	SetString(x, y, _T("Message"));
	strText.Format(_T("%02X"), m_scsi.message);
	SetString(x + 11, y, strText);
	y++;

	// Status
	SetString(x, y, _T("Status"));
	strText.Format(_T("%02X"), m_scsi.status);
	SetString(x + 11, y, strText);
}

//---------------------------------------------------------------------------
//
//	�Z�b�g�A�b�v(�h���C�u)
//
//---------------------------------------------------------------------------
void FASTCALL CSCSIWnd::SetupDrive(int x, int y)
{
	CString strText;
	Disk *pDisk;
	Disk::disk_t disk;
	DWORD dwSize;

	ASSERT(this);
	ASSERT(x >= 0);
	ASSERT(y >= 0);

	// ������
	pDisk = NULL;

	// ID
	SetString(x, y, _T("ID"));
	if ((m_scsi.id >= 0) && (m_scsi.id <= 7)) {
		strText.Format(_T("%d"), m_scsi.id);
		pDisk = m_scsi.disk[m_scsi.id];
	}
	else {
		strText = _T("(Invalid)");
	}
	SetString(x + 11, y, strText);
	y++;

	// �f�[�^�擾
	if (pDisk) {
		pDisk->GetDisk(&disk);
	}
	else {
		memset(&disk, 0, sizeof(disk));
	}

	// ���
	SetString(x, y, _T("Type"));
	strText = _T("(No Unit)");
	if (pDisk) {
		switch (disk.id) {
			case MAKEID('S', 'C', 'H', 'D'):
				strText = _T("SCSI-HD");
				break;
			case MAKEID('S', 'C', 'M', 'O'):
				strText = _T("SCSI-MO");
				break;
			case MAKEID('S', 'C', 'C', 'D'):
				strText = _T("CD-ROM");
				break;
		}
	}
	SetString(x + 11, y, strText);
	y++;

	// �e��
	SetString(x, y, _T("Capacity"));
	dwSize = 0;
	if (pDisk) {
		if (disk.ready) {
			dwSize = disk.blocks;
			ASSERT((disk.size == 0) || (disk.size == 9) || (disk.size == 11));
			if (disk.size == 11) {
				dwSize <<= 2;
			}
		}
	}
	strText.Format(_T("%d.%1dMB"), (dwSize >> 11), (dwSize & 0x7ff) / 205);
	SetString(x + 11, y, strText);
	y++;

	// �A�N�Z�X�^�C�v
	SetString(x, y, _T("Access"));
	if (pDisk) {
		if (disk.writep) {
			SetString(x + 11, y, _T("RO"));
		}
		else {
			SetString(x + 11, y, _T("R/W"));
		}
	}
	else {
		SetString(x + 11, y, _T("N/A"));
	}
	y++;

	// ���f�B�A����
	SetString(x, y, _T("Media"));
	if (pDisk) {
		if (disk.removable) {
			SetString(x + 11, y, _T("Removable"));
		}
		else {
			SetString(x + 11, y, _T("Fixed"));
		}
	}
	else {
		SetString(x + 11, y, _T("N/A"));
	}
	y++;

	// ������
	strText = _T("N/A");

	// �Z���X�L�[
	SetString(x, y, _T("Sense"));
	if (pDisk) {
		strText.Format(_T("%02X"), (BYTE)(disk.code >> 16));
	}
	SetString(x + 11, y, strText);
	y++;

	// �g���Z���X�R�[�h
	SetString(x, y, _T("ASC"));
	if (pDisk) {
		strText.Format(_T("%02X"), (BYTE)(disk.code >> 8));
	}
	SetString(x + 11, y, strText);
	y++;

	// �g���Z���X�R�[�h
	SetString(x, y, _T("ASCQ"));
	if (pDisk) {
		strText.Format(_T("%02X"), (BYTE)disk.code);
	}
	SetString(x + 11, y, strText);
}

//---------------------------------------------------------------------------
//
//	�Z�b�g�A�b�v(���W�X�^)
//
//---------------------------------------------------------------------------
void FASTCALL CSCSIWnd::SetupReg(int x, int y)
{
	CString strText;

	ASSERT(this);
	ASSERT(x >= 0);
	ASSERT(y >= 0);

	// BDID(�r�b�g�\��)
	SetString(x, y, _T("BDID"));
	strText.Format(_T("%02X"), m_scsi.bdid);
	SetString(x + 11, y, strText);
	y++;

	// SCTL
	SetString(x, y, _T("SCTL"));
	strText.Format(_T("%02X"), m_scsi.sctl);
	SetString(x + 11, y, strText);
	y++;

	// SCMD
	SetString(x, y, _T("SCMD"));
	strText.Format(_T("%02X"), m_scsi.scmd);
	SetString(x + 11, y, strText);
	y++;

	// INTS
	SetString(x, y, _T("INTS"));
	strText.Format(_T("%02X"), m_scsi.ints);
	SetString(x + 11, y, strText);
	y++;

	// SDGC
	SetString(x, y, _T("SDGC"));
	strText.Format(_T("%02X"), m_scsi.sdgc);
	SetString(x + 11, y, strText);
	y++;

	// PCTL
	SetString(x, y, _T("PCTL"));
	strText.Format(_T("%02X"), m_scsi.pctl);
	SetString(x + 11, y, strText);
	y++;

	// MBC
	SetString(x, y, _T("MBC"));
	strText.Format(_T("%02X"), m_scsi.mbc);
	SetString(x + 11, y, strText);
	y++;

	// TEMP
	SetString(x, y, _T("TEMP"));
	strText.Format(_T("%02X"), m_scsi.temp);
	SetString(x + 11, y, strText);
	y++;

	// TC
	SetString(x, y, _T("TC"));
	strText.Format(_T("%06X"), m_scsi.tc);
	SetString(x + 11 - 4, y, strText);
	y++;

	// Transfer
	SetString(x, y, _T("XFER"));
	if (m_scsi.trans) {
		SetString(x + 9, y, _T("Auto"));
	}
	else {
		SetString(x + 7, y, _T("Manual"));
	}
	y++;

	// Length
	SetString(x, y, _T("LEN"));
	strText.Format(_T("%04X"), m_scsi.length);
	SetString(x + 9, y, strText);
}

//---------------------------------------------------------------------------
//
//	�Z�b�g�A�b�v(CDB)
//
//---------------------------------------------------------------------------
void FASTCALL CSCSIWnd::SetupCDB(int x, int y)
{
	int nCDB;
	CString strText;

	ASSERT(this);
	ASSERT(x >= 0);
	ASSERT(y >= 0);

	// CDB���[�v
	for (nCDB=0; nCDB<10; nCDB++) {
		// 6�o�C�g�ȍ~�͕\�����Ȃ��ꍇ����
		if (nCDB == 6) {
			if (m_scsi.cmd[0] < 0x20) {
				break;
			}
		}

		// �\��
		strText.Format(_T("CDB[%d]"), nCDB);
		SetString(x, y, strText);
		strText.Format(_T("%02X"), m_scsi.cmd[nCDB]);
		SetString(x + 11, y, strText);

		// ����
		y++;
	}
}

#endif	// _WIN32
