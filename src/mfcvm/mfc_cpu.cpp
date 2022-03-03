//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 �o�h�D(ytanaka@ipc-tokai.or.jp)
//	[ MFC �T�u�E�B���h�E(CPU) ]
//
//---------------------------------------------------------------------------

#if defined(_WIN32)

#include "os.h"
#include "xm6.h"
#include "vm.h"
#include "cpu.h"
#include "memory.h"
#include "schedule.h"
#include "mfp.h"
#include "scc.h"
#include "dmac.h"
#include "iosc.h"
#include "render.h"
#include "mfc_sub.h"
#include "mfc_draw.h"
#include "mfc_res.h"
#include "mfc_cpu.h"

//---------------------------------------------------------------------------
//
//	cpudebug.c�Ƃ̘A��
//
//---------------------------------------------------------------------------
#if defined(__cplusplus)
extern "C" {
#endif	// __cplusplus

void cpudebug_disassemble(int n);
										// 1�s�t�A�Z���u��
extern void (*cpudebug_put)(const char*);
										// 1�s�o��
extern DWORD debugpc;
										// �t�A�Z���u��PC

#if defined(__cplusplus)
}
#endif	// __cplusplus

static char debugbuf[0x200];
										// �o�̓o�b�t�@

//===========================================================================
//
//	�q�X�g���t���_�C�A���O
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	�R���X�g���N�^
//
//---------------------------------------------------------------------------
CHistoryDlg::CHistoryDlg(UINT nID, CWnd *pParentWnd) : CDialog(nID, pParentWnd)
{
	// ������
	m_dwValue = 0;
	m_nBit = 32;
}

//---------------------------------------------------------------------------
//
//	�_�C�A���O������
//
//---------------------------------------------------------------------------
BOOL CHistoryDlg::OnInitDialog()
{
	int i;
	int nNum;
	DWORD *pData;
	CString strText;
	CComboBox *pComboBox;

	// ��{�N���X
	if (!CDialog::OnInitDialog()) {
		return FALSE;
	}

	// �}�X�N����
	m_dwMask = 0;
	for (i=0; i<(int)m_nBit; i++) {
		m_dwMask <<= 1;
		m_dwMask |= 0x01;
	}

	// �R���{�{�b�N�X�N���A
	pComboBox = (CComboBox*)GetDlgItem(IDC_ADDR_ADDRE);
	ASSERT(pComboBox);
	pComboBox->ResetContent();

	// �R���{�{�b�N�X�ǉ�
	nNum = *(int *)GetNumPtr();
	pData = GetDataPtr();
	for (i=0; i<nNum; i++) {
		if (pData[i] > m_dwMask) {
			// �}�X�N���傫���̂ŁA�ꗥ32bit��OK
			strText.Format(_T("%08X"), pData[i]);
		}
		else {
			// �}�X�N�ȉ�
			switch (m_nBit) {
				case 8:
					strText.Format(_T("%02X"), pData[i]);
					break;
				case 16:
					strText.Format(_T("%04X"), pData[i]);
					break;
				case 24:
					strText.Format(_T("%06X"), pData[i]);
					break;
				default:
					strText.Format(_T("%08X"), pData[i]);
					break;
			}
		}
		// �ǉ�
		pComboBox->AddString(strText);
	}

	// dwValue�͕K���}�X�N
	m_dwValue &= m_dwMask;
	switch (m_nBit) {
		case 8:
			strText.Format(_T("%02X"), m_dwValue);
			break;
		case 16:
			strText.Format(_T("%04X"), m_dwValue);
			break;
		case 24:
			strText.Format(_T("%06X"), m_dwValue);
			break;
		default:
			strText.Format(_T("%08X"), m_dwValue);
			break;
	}
	pComboBox->SetWindowText(strText);

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	�_�C�A���OOK
//
//---------------------------------------------------------------------------
void CHistoryDlg::OnOK()
{
	CComboBox *pComboBox;
	CString strText;
	int i;
	int nHit;
	int nNum;
	DWORD *pData;

	// ���͐��l���擾
	pComboBox = (CComboBox*)GetDlgItem(IDC_ADDR_ADDRE);
	ASSERT(pComboBox);
	pComboBox->GetWindowText(strText);
	m_dwValue = _tcstoul((LPCTSTR)strText, NULL, 16);

	// ���ɓ��͂��ꂽ���̂Ɠ������`�F�b�N
	nNum = *(int *)GetNumPtr();
	pData = GetDataPtr();
	nHit = -1;
	for (i=0; i<nNum; i++) {
		if (pData[i] == m_dwValue) {
			nHit = i;
			break;
		}
	}

	// �V�K���A�̗p��
	if (nHit >= 0) {
		// ���ɂ�����̂Ɠ����B�ꏊ����ւ�
		for (i=(nHit - 1); i>=0; i--) {
			pData[i + 1] = pData[i];
		}
		pData[0] = m_dwValue;
	}
	else {
		// �V�K�B�������̂��ЂƂ��֊i����
		for (i=9; i>=1; i--) {
			pData[i] = pData[i - 1];
		}

		// �ŐV��[0]��
		pData[0] = m_dwValue;

		// 10�܂ł͒ǉ��ł���
		if (nNum < 10) {
			*(int *)GetNumPtr() = (nNum + 1);
		}
	}

	// dwValue���}�X�N����OK
	m_dwValue &= m_dwMask;

	// ��{�N���X
	CDialog::OnOK();
}

//===========================================================================
//
//	�A�h���X���̓_�C�A���O
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	�R���X�g���N�^
//
//---------------------------------------------------------------------------
CAddrDlg::CAddrDlg(CWnd *pParent) : CHistoryDlg(IDD_ADDRDLG, pParent)
{
	// �p����ւ̑Ή�
	if (!::IsJapanese()) {
		m_lpszTemplateName = MAKEINTRESOURCE(IDD_US_ADDRDLG);
		m_nIDHelp = IDD_US_ADDRDLG;
	}

	m_nBit = 24;
}

//---------------------------------------------------------------------------
//
//	���j���[�Z�b�g�A�b�v
//
//---------------------------------------------------------------------------
void CAddrDlg::SetupHisMenu(CMenu *pMenu)
{
	int i;
	CString string;

	ASSERT(pMenu);

	// ���j���[������
	for (i=0; i<(int)m_Num; i++) {
		string.Format("$%06X", m_Data[i]);
		pMenu->ModifyMenu(IDM_HISTORY_0 + i, MF_BYCOMMAND | MF_STRING,
							IDM_HISTORY_0 + i, (LPCTSTR)string);
	}

	// ���j���[�폜
	for (i=m_Num; i<10; i++) {
		pMenu->DeleteMenu(IDM_HISTORY_0 + i, MF_BYCOMMAND);
	}
}

//---------------------------------------------------------------------------
//
//	���j���[���ʎ擾
//
//---------------------------------------------------------------------------
DWORD CAddrDlg::GetAddr(UINT nID)
{
	DWORD dwAddr;
    int i;

	ASSERT((nID >= IDM_HISTORY_0) && (nID <= IDM_HISTORY_9));

	// �A�h���X�擾
	nID -= IDM_HISTORY_0;
	ASSERT(nID < 10);
	dwAddr = m_Data[nID];

	// �ꏊ����ւ�
	for (i=(int)(nID - 1); i>=0; i--) {
		m_Data[i + 1] = m_Data[i];
	}
	m_Data[0] = dwAddr;

	return dwAddr;
}

//---------------------------------------------------------------------------
//
//	�q�X�g�����|�C���^�擾
//
//---------------------------------------------------------------------------
UINT* CAddrDlg::GetNumPtr()
{
	return &m_Num;
}

//---------------------------------------------------------------------------
//
//	�q�X�g���f�[�^�|�C���^�擾
//
//---------------------------------------------------------------------------
DWORD* CAddrDlg::GetDataPtr()
{
	return m_Data;
}

//---------------------------------------------------------------------------
//
//	�q�X�g����
//
//---------------------------------------------------------------------------
UINT CAddrDlg::m_Num = 0;

//---------------------------------------------------------------------------
//
//	�q�X�g���f�[�^
//
//---------------------------------------------------------------------------
DWORD CAddrDlg::m_Data[10] = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

//===========================================================================
//
//	���W�X�^���̓_�C�A���O
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	�R���X�g���N�^
//
//---------------------------------------------------------------------------
CRegDlg::CRegDlg(CWnd *pParent) : CHistoryDlg(IDD_REGDLG, pParent)
{
	// �p����ւ̑Ή�
	if (!::IsJapanese()) {
		m_lpszTemplateName = MAKEINTRESOURCE(IDD_US_REGDLG);
		m_nIDHelp = IDD_US_REGDLG;
	}

	m_nIndex = 0;
	m_nBit = 32;
}

//---------------------------------------------------------------------------
//
//	�_�C�A���O������
//
//---------------------------------------------------------------------------
BOOL CRegDlg::OnInitDialog()
{
	CWnd *pWnd;
	CString strRegister;
	CPU *pCPU;
	CPU::cpu_t reg;

	ASSERT(this);
	ASSERT(m_nIndex < 20);

	// CPU���W�X�^�擾
	::LockVM();
	pCPU = (CPU*)::GetVM()->SearchDevice(MAKEID('C', 'P', 'U', ' '));
	ASSERT(pCPU);
	pCPU->GetCPU(&reg);
	::UnlockVM();

	// ������쐬
	if (m_nIndex <= 7) {
		strRegister.Format("D%d", m_nIndex);
		m_dwValue = reg.dreg[m_nIndex];
	}
	if ((m_nIndex >= 8) && (m_nIndex <= 15)) {
		strRegister.Format("A%d", m_nIndex & 7);
		m_dwValue = reg.areg[m_nIndex & 7];
	}
	switch (m_nIndex) {
		// USP
		case 16:
			strRegister = "USP";
			if (reg.sr & 0x2000) {
				m_dwValue = reg.sp;
			}
			else {
				m_dwValue = reg.areg[7];
			}
			break;
		// SSP
		case 17:
			strRegister = "SSP";
			if (reg.sr & 0x2000) {
				m_dwValue = reg.areg[7];
			}
			else {
				m_dwValue = reg.sp;
			}
			break;
		// PC
		case 18:
			strRegister = "PC";
			m_dwValue = reg.pc;
			break;
		// SR
		case 19:
			strRegister = "SR";
			m_dwValue = reg.sr;
			break;
	}

	// ��{�N���X�������ŌĂ�
	if (!CHistoryDlg::OnInitDialog()) {
		return FALSE;
	}

	// �l��ݒ�
	pWnd = GetDlgItem(IDC_ADDR_ADDRL);
	ASSERT(pWnd);
	pWnd->SetWindowText(strRegister);

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	OK
//
//---------------------------------------------------------------------------
void CRegDlg::OnOK()
{
	CComboBox *pComboBox;
	CString string;
	DWORD dwValue;
	CPU *pCPU;
	CPU::cpu_t reg;

	ASSERT(this);

	// Value���擾
	pComboBox = (CComboBox*)GetDlgItem(IDC_ADDR_ADDRE);
	ASSERT(pComboBox);
	pComboBox->GetWindowText(string);
	dwValue = ::strtoul((LPCTSTR)string, NULL, 16);

	// VM���b�N�A���W�X�^�擾
	::LockVM();
	pCPU = (CPU*)::GetVM()->SearchDevice(MAKEID('C', 'P', 'U', ' '));
	ASSERT(pCPU);
	pCPU->GetCPU(&reg);

	// �C���f�b�N�X��
	if (m_nIndex <= 7) {
		reg.dreg[m_nIndex] = dwValue;
	}
	if ((m_nIndex >= 8) && (m_nIndex <= 15)) {
		reg.areg[m_nIndex & 7] = dwValue;
	}
	switch (m_nIndex) {
		// USP
		case 16:
			if (reg.sr & 0x2000) {
				reg.sp = dwValue;
			}
			else {
				reg.areg[7] = dwValue;
			}
			break;
		// SSP
		case 17:
			if (reg.sr & 0x2000) {
				reg.areg[7] = dwValue;
			}
			else {
				reg.sp = dwValue;
			}
			break;
		// PC
		case 18:
			reg.pc = dwValue & 0xfffffe;
			break;
		// SR
		case 19:
			reg.sr = (WORD)dwValue;
			break;
	}

	// ���W�X�^�ݒ�AVM�A�����b�N
	pCPU->SetCPU(&reg);
	::UnlockVM();

	// ��{�N���X
	CHistoryDlg::OnOK();
}

//---------------------------------------------------------------------------
//
//	�q�X�g�����|�C���^�擾
//
//---------------------------------------------------------------------------
UINT* CRegDlg::GetNumPtr()
{
	return &m_Num;
}

//---------------------------------------------------------------------------
//
//	�q�X�g���f�[�^�|�C���^�擾
//
//---------------------------------------------------------------------------
DWORD* CRegDlg::GetDataPtr()
{
	return m_Data;
}

//---------------------------------------------------------------------------
//
//	�q�X�g����
//
//---------------------------------------------------------------------------
UINT CRegDlg::m_Num = 0;

//---------------------------------------------------------------------------
//
//	�q�X�g���f�[�^
//
//---------------------------------------------------------------------------
DWORD CRegDlg::m_Data[10] = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

//===========================================================================
//
//	�f�[�^���̓_�C�A���O
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	�R���X�g���N�^
//
//---------------------------------------------------------------------------
CDataDlg::CDataDlg(CWnd *pParent) : CHistoryDlg(IDD_DATADLG, pParent)
{
	// �p����ւ̑Ή�
	if (!::IsJapanese()) {
		m_lpszTemplateName = MAKEINTRESOURCE(IDD_US_DATADLG);
		m_nIDHelp = IDD_US_DATADLG;
	}

	// �ꉞ�A������
	m_dwAddr = 0;
	m_nSize = 0;
}

//---------------------------------------------------------------------------
//
//	�_�C�A���O������
//
//---------------------------------------------------------------------------
BOOL CDataDlg::OnInitDialog()
{
	CWnd *pWnd;
	CString string;

	ASSERT(this);
	ASSERT(m_dwAddr < 0x1000000);

	// �A�h���X�ƃr�b�g������
	switch (m_nSize) {
		// Byte
		case 0:
			string.Format("$%06X (B)", m_dwAddr);
			m_nBit = 8;
			break;
		// Word
		case 1:
			string.Format("$%06X (W)", m_dwAddr);
			m_nBit = 16;
			break;
		// Long
		case 2:
			string.Format("$%06X (L)", m_dwAddr);
			m_nBit = 32;
			break;
		// ���̑�
		default:
			ASSERT(FALSE);
			break;
	}

	// ��{�N���X
	if (!CHistoryDlg::OnInitDialog()) {
		return FALSE;
	}

	// �������m���̌�Őݒ�
	pWnd = GetDlgItem(IDC_ADDR_ADDRL);
	ASSERT(pWnd);
	pWnd->SetWindowText(string);

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	�q�X�g�����|�C���^�擾
//
//---------------------------------------------------------------------------
UINT* CDataDlg::GetNumPtr()
{
	return &m_Num;
}

//---------------------------------------------------------------------------
//
//	�q�X�g���f�[�^�|�C���^�擾
//
//---------------------------------------------------------------------------
DWORD* CDataDlg::GetDataPtr()
{
	return m_Data;
}

//---------------------------------------------------------------------------
//
//	�q�X�g����
//
//---------------------------------------------------------------------------
UINT CDataDlg::m_Num = 0;

//---------------------------------------------------------------------------
//
//	�q�X�g���f�[�^
//
//---------------------------------------------------------------------------
DWORD CDataDlg::m_Data[10] = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

//===========================================================================
//
//	CPU���W�X�^�E�B���h�E
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	�R���X�g���N�^
//
//---------------------------------------------------------------------------
CCPURegWnd::CCPURegWnd()
{
	// �E�B���h�E�p�����[�^��`
	m_dwID = MAKEID('M', 'P', 'U', 'R');
	::GetMsg(IDS_SWND_CPUREG, m_strCaption);
	m_nWidth = 27;
	m_nHeight = 10;

	// CPU�擾
	m_pCPU = (CPU*)::GetVM()->SearchDevice(MAKEID('C', 'P', 'U', ' '));
	ASSERT(m_pCPU);
}

//---------------------------------------------------------------------------
//
//	���b�Z�[�W �}�b�v
//
//---------------------------------------------------------------------------
BEGIN_MESSAGE_MAP(CCPURegWnd, CSubTextWnd)
	ON_WM_LBUTTONDBLCLK()
	ON_WM_CONTEXTMENU()
	ON_COMMAND_RANGE(IDM_REG_D0, IDM_REG_SR, OnReg)
END_MESSAGE_MAP()

//---------------------------------------------------------------------------
//
//	�Z�b�g�A�b�v
//
//---------------------------------------------------------------------------
void FASTCALL CCPURegWnd::Setup()
{
	CPU::cpu_t buf;
	int i;
	CString string;

	ASSERT(this);

	// �N���A
	Clear();

	// CPU���W�X�^�擾
	m_pCPU->GetCPU(&buf);

	// �Z�b�g(D, A)
	for (i=0; i<8; i++) {
		string.Format("D%1d  %08X", i, buf.dreg[i]);
		SetString(0, i, string);

		string.Format("A%1d  %08X", i, buf.areg[i]);
		SetString(15, i, string);
	}

	// �Z�b�g(�X�^�b�N)
	if (buf.sr & 0x2000) {
		string.Format("USP %08X", buf.sp);
		SetString(0, 8, string);
		string.Format("SSP %08X", buf.areg[7]);
		SetString(15, 8, string);
	}
	else {
		string.Format("USP %08X", buf.areg[7]);
		SetString(0, 8, string);
		string.Format("SSP %08X", buf.sp);
		SetString(15, 8, string);
	}

	// �Z�b�g(���̑�)
	string.Format("PC    %06X", buf.pc);
	SetString(0, 9, string);
	string.Format("SR      %04X", buf.sr);
	SetString(15, 9, string);
}

//---------------------------------------------------------------------------
//
//	���{�^���_�u���N���b�N
//
//---------------------------------------------------------------------------
void CCPURegWnd::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	int x;
	int y;
	int index;
	CRegDlg dlg(this);

	// x,y�Z�o
	x = point.x / m_tmWidth;
	y = point.y / m_tmHeight;

	// �C���f�b�N�X���o��
	if (y < 8) {
		if (x < 15) {
			// D0-D7
			index = y;
		}
		else {
			// A0-A7
			index = y + 8;
		}
	}
	else {
		index = y - 8;
		index <<= 1;
		if (x >= 15) {
			index++;
		}
		index += 16;
	}

	// �_�C�A���O���s
	dlg.m_nIndex = index;
	dlg.DoModal();
}

//---------------------------------------------------------------------------
//
//	�R���e�L�X�g���j���[
//
//---------------------------------------------------------------------------
void CCPURegWnd::OnContextMenu(CWnd *pWnd, CPoint point)
{
	CRect rect;
	CMenu menu;
	CMenu *pMenu;

	// �N���C�A���g�̈���ŉ����ꂽ�����肷��
	GetClientRect(&rect);
	ClientToScreen(&rect);
	if (!rect.PtInRect(point)) {
		CSubTextWnd::OnContextMenu(pWnd, point);
		return;
	}

	// ���j���[���[�h
	menu.LoadMenu(IDR_REGMENU);
	pMenu = menu.GetSubMenu(0);

	// ���j���[�Z�b�g�A�b�v
	SetupRegMenu(pMenu, m_pCPU, TRUE);

	// ���j���[���s
	pMenu->TrackPopupMenu(TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_RIGHTBUTTON,
							point.x, point.y, this);
}

//---------------------------------------------------------------------------
//
//	���j���[�Z�b�g�A�b�v
//
//---------------------------------------------------------------------------
void CCPURegWnd::SetupRegMenu(CMenu *pMenu, CPU *pCPU, BOOL bSR)
{
	int i;
	CString string;
	CPU::cpu_t reg;

	ASSERT(pMenu);
	ASSERT(pCPU);

	// CPU���W�X�^�擾
	::LockVM();
	pCPU->GetCPU(&reg);
	::UnlockVM();

	// �Z�b�g�A�b�v(D)
	for (i=0; i<8; i++) {
		string.Format("D%1d ($%08X)", i, reg.dreg[i]);
		pMenu->ModifyMenu(IDM_REG_D0 + i, MF_BYCOMMAND | MF_STRING,
							IDM_REG_D0 + i, (LPCTSTR)string);
	}
	// �Z�b�g�A�b�v(A)
	for (i=0; i<8; i++) {
		string.Format("A%1d ($%08X)", i, reg.areg[i]);
		pMenu->ModifyMenu(IDM_REG_A0 + i, MF_BYCOMMAND | MF_STRING,
							IDM_REG_A0 + i, (LPCTSTR)string);
	}
	// �Z�b�g�A�b�v(USP,SSP)
	if (reg.sr & 0x2000) {
		string.Format("USP ($%08X)", reg.sp);
		pMenu->ModifyMenu(IDM_REG_USP, MF_BYCOMMAND | MF_STRING, IDM_REG_USP, (LPCTSTR)string);
		string.Format("SSP ($%08X)", reg.areg[7]);
		pMenu->ModifyMenu(IDM_REG_SSP, MF_BYCOMMAND | MF_STRING, IDM_REG_SSP, (LPCTSTR)string);
	}
	else {
		string.Format("USP ($%08X)", reg.areg[7]);
		pMenu->ModifyMenu(IDM_REG_USP, MF_BYCOMMAND | MF_STRING, IDM_REG_USP, (LPCTSTR)string);
		string.Format("SSP ($%08X)", reg.sp);
		pMenu->ModifyMenu(IDM_REG_SSP, MF_BYCOMMAND | MF_STRING, IDM_REG_SSP, (LPCTSTR)string);
	}

	// �Z�b�g�A�b�v(PC)
	string.Format("PC ($%06X)", reg.pc);
	pMenu->ModifyMenu(IDM_REG_PC, MF_BYCOMMAND | MF_STRING, IDM_REG_PC, (LPCTSTR)string);

	// �Z�b�g�A�b�v(SR)
	if (bSR) {
		string.Format("SR ($%04X)", reg.sr);
		pMenu->ModifyMenu(IDM_REG_SR, MF_BYCOMMAND | MF_STRING, IDM_REG_SR, (LPCTSTR)string);
	}
}

//---------------------------------------------------------------------------
//
//	���W�X�^�l�擾
//
//---------------------------------------------------------------------------
DWORD CCPURegWnd::GetRegValue(CPU *pCPU, UINT uID)
{
	CPU::cpu_t reg;

	ASSERT(pCPU);
	ASSERT((uID >= IDM_REG_D0) && (uID <= IDM_REG_SR));

	// CPU���W�X�^�擾
	::LockVM();
	pCPU->GetCPU(&reg);
	::UnlockVM();

	// D0�`D7
	if (uID <= IDM_REG_D7) {
		return reg.dreg[uID - IDM_REG_D0];
	}

	// A0-A7
	if (uID <= IDM_REG_A7) {
		return reg.areg[uID - IDM_REG_A0];
	}

	// USP
	if (uID == IDM_REG_USP) {
		if (reg.sr & 0x2000) {
			return reg.sp;
		}
		else {
			return reg.areg[7];
		}
	}

	// SSP
	if (uID == IDM_REG_SSP) {
		if (reg.sr & 0x2000) {
			return reg.areg[7];
		}
		else {
			return reg.sp;
		}
	}

	// PC
	if (uID == IDM_REG_PC) {
		return reg.pc;
	}

	// SR
	ASSERT(uID == IDM_REG_SR);
	return reg.sr;
}

//---------------------------------------------------------------------------
//
//	���W�X�^�R�}���h
//
//---------------------------------------------------------------------------
void CCPURegWnd::OnReg(UINT nID)
{
	CRegDlg dlg(this);

	ASSERT((nID >= IDM_REG_D0) && (nID <= IDM_REG_SR));

	// ���Z����
	nID -= IDM_REG_D0;

	// �_�C�A���O�ɂ܂�����
	dlg.m_nIndex = nID;
	dlg.DoModal();
}

//===========================================================================
//
//	���荞�݃E�B���h�E
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	�R���X�g���N�^
//
//---------------------------------------------------------------------------
CIntWnd::CIntWnd()
{
	// �E�B���h�E�p�����[�^��`
	m_dwID = MAKEID('I', 'N', 'T', ' ');
	::GetMsg(IDS_SWND_INT, m_strCaption);
	m_nWidth = 47;
	m_nHeight = 9;

	// CPU�擾
	m_pCPU = (CPU*)::GetVM()->SearchDevice(MAKEID('C', 'P', 'U', ' '));
	ASSERT(m_pCPU);
}

//---------------------------------------------------------------------------
//
//	���b�Z�[�W �}�b�v
//
//---------------------------------------------------------------------------
BEGIN_MESSAGE_MAP(CIntWnd, CSubTextWnd)
	ON_WM_LBUTTONDBLCLK()
END_MESSAGE_MAP()

//---------------------------------------------------------------------------
//
//	�Z�b�g�A�b�v
//
//---------------------------------------------------------------------------
void FASTCALL CIntWnd::Setup()
{
	static const char *desc_table[] = {
		"INTSW",
		"MFP",
		"SCC",
		"(Ext.)",
		"DMAC",
		"(Ext.)",
		"IOSC"
	};
	int y;
	int i;
	int level;
	CString string;
	CPU::cpu_t cpu;

	// CPU�f�[�^�擾
	m_pCPU->GetCPU(&cpu);
	level = (cpu.sr >> 8);
	level &= 0x07;

	// �N���A
	Clear();
	y = 0;

	// �K�C�h
	SetString(0, y, "(High)  Device  Mask  Vector     Req        Ack");
	y++;

	// 7���x������
	for (i=7; i>=1; i--) {
		// ���荞�ݖ��̃Z�b�g
		string.Format("Level%1d  ", i);
		string += desc_table[7 - i];
		SetString(0, y, string);

		// �}�X�N
		if (i < 7) {
			if (i <= level) {
				SetString(16, y, "Mask");
			}
		}

		// ���N�G�X�g����
		if (cpu.intr[0] & 0x80) {
			// ���N�G�X�g����B�x�N�^�\��
			string.Format("$%02X", cpu.intr[i]);
			SetString(22, y, string);
		}

		// ���N�G�X�g�J�E���^
		string.Format("%10d", cpu.intreq[i]);
		SetString(26, y, string);
		
		// �����J�E���^
		string.Format("%10d", cpu.intack[i]);
		SetString(37, y, string);

		// ����
		y++;
		cpu.intr[0] <<= 1;
	}

	// �K�C�h
	SetString(0, y, "(Low)");
}

//---------------------------------------------------------------------------
//
//	���{�^���_�u���N���b�N
//
//---------------------------------------------------------------------------
void CIntWnd::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	int y;
	int level;
	CPU::cpu_t cpu;

	// y�Z�o
	y = point.y / m_tmHeight;

	// y=1,2,3,4,5,6,7�����ꂼ��int7,6,5,4,3,2,1�ɑΉ�
	level = 8 - y;
	if ((level < 1) || (level > 7)) {
		return;
	}

	// ���b�N�A�f�[�^�擾
	::LockVM();
	m_pCPU->GetCPU(&cpu);

	// �N���A
	ASSERT((level >= 1) && (level <= 7));
	cpu.intreq[level] = 0;
	cpu.intack[level] = 0;

	// �f�[�^�ݒ�A�A�����b�N
	m_pCPU->SetCPU(&cpu);
	::UnlockVM();
}

//===========================================================================
//
//	�t�A�Z���u���E�B���h�E
//
//===========================================================================

#if defined(__cplusplus)
extern "C" {
#endif
//---------------------------------------------------------------------------
//
//	cpudebug.c ������o��
//
//---------------------------------------------------------------------------
void disasm_put(const char *s)
{
	strcpy(debugbuf, s);
}

//---------------------------------------------------------------------------
//
//	�������f�o�C�X
//
//---------------------------------------------------------------------------
static Memory* cpudebug_memory;

//---------------------------------------------------------------------------
//
//	cpudebug.c ���[�h�ǂݏo��
//
//---------------------------------------------------------------------------
WORD cpudebug_fetch(DWORD addr)
{
	WORD w;

	ASSERT(cpudebug_memory);

	addr &= 0xfffffe;
	w = cpudebug_memory->ReadOnly(addr);
	w <<= 8;
	w |= cpudebug_memory->ReadOnly(addr + 1);

	return w;
}
#if defined(__cplusplus)
};
#endif

//---------------------------------------------------------------------------
//
//	�R���X�g���N�^
//
//---------------------------------------------------------------------------
CDisasmWnd::CDisasmWnd(int index)
{
	// �t�A�Z���u���E�B���h�E��8��ނ܂�
	ASSERT((index >= 0) && (index <= 0x07));

	// �E�B���h�E�p�����[�^��`
	m_dwID = MAKEID('D', 'I', 'S', (index + 'A'));
	::GetMsg(IDS_SWND_DISASM, m_strCaption);
	m_nWidth = 70;
	m_nHeight = 16;

	// �E�B���h�E�p�����[�^��`(�X�N���[��)
	m_ScrlWidth = 70;
	m_ScrlHeight = 0x8000;

	// �ŏ��̃E�B���h�E��PC��������A����ȊO�͖���
	if (index == 0) {
		m_bSync = TRUE;
	}
	else {
		m_bSync = FALSE;
	}

	// ���̑�
	m_pAddrBuf = NULL;
	m_Caption = m_strCaption;
	m_CaptionSet = "";

	// �f�o�C�X���擾
	m_pCPU = (CPU*)::GetVM()->SearchDevice(MAKEID('C', 'P', 'U', ' '));
	ASSERT(m_pCPU);
	cpudebug_memory = (Memory*)::GetVM()->SearchDevice(MAKEID('M', 'E', 'M', ' '));
	ASSERT(cpudebug_memory);
	m_pScheduler = (Scheduler*)::GetVM()->SearchDevice(MAKEID('S', 'C', 'H', 'E'));
	ASSERT(m_pScheduler);
	m_pMFP = (MFP*)::GetVM()->SearchDevice(MAKEID('M', 'F', 'P', ' '));
	ASSERT(m_pMFP);
	m_pMemory = (Memory*)::GetVM()->SearchDevice(MAKEID('M', 'E', 'M', ' '));
	ASSERT(m_pMemory);
	m_pSCC = (SCC*)::GetVM()->SearchDevice(MAKEID('S', 'C', 'C', ' '));
	ASSERT(m_pSCC);
	m_pDMAC = (DMAC*)::GetVM()->SearchDevice(MAKEID('D', 'M', 'A', 'C'));
	ASSERT(m_pDMAC);
	m_pIOSC = (IOSC*)::GetVM()->SearchDevice(MAKEID('I', 'O', 'S', 'C'));
	ASSERT(m_pIOSC);

	// �A�h���X��PC�ɏ�����
	m_dwSetAddr = m_pCPU->GetPC();
	m_dwAddr = m_dwSetAddr;
	m_dwAddr = m_dwAddr & 0xff0000;
	m_dwPC = 0xffffffff;

	// �t�A�Z���u���o�b�t�@�ڑ�
	::cpudebug_put = ::disasm_put;
}

//---------------------------------------------------------------------------
//
//	���b�Z�[�W �}�b�v
//
//---------------------------------------------------------------------------
BEGIN_MESSAGE_MAP(CDisasmWnd, CSubTextScrlWnd)
	ON_WM_CREATE()
	ON_WM_DESTROY()
	ON_WM_SIZE()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_VSCROLL()
	ON_WM_CONTEXTMENU()
	ON_COMMAND(IDM_DIS_NEWWIN, OnNewWin)
	ON_COMMAND(IDM_DIS_PC, OnPC)
	ON_COMMAND(IDM_DIS_SYNC, OnSync)
	ON_COMMAND(IDM_DIS_ADDR, OnAddr)
	ON_COMMAND_RANGE(IDM_REG_D0, IDM_REG_PC, OnReg)
	ON_COMMAND_RANGE(IDM_STACK_0, IDM_STACK_F, OnStack)
	ON_COMMAND_RANGE(IDM_DIS_BREAKP0, IDM_DIS_BREAKP7, OnBreak)
	ON_COMMAND_RANGE(IDM_HISTORY_0, IDM_HISTORY_9, OnHistory)
	ON_COMMAND_RANGE(IDM_DIS_RESET, IDM_DIS_FLINE, OnCPUExcept)
	ON_COMMAND_RANGE(IDM_DIS_TRAP0, IDM_DIS_TRAPF, OnTrap)
	ON_COMMAND_RANGE(IDM_DIS_MFP0, IDM_DIS_MFPF, OnMFP)
	ON_COMMAND_RANGE(IDM_DIS_SCC0, IDM_DIS_SCC7, OnSCC)
	ON_COMMAND_RANGE(IDM_DIS_DMAC0, IDM_DIS_DMAC7, OnDMAC)
	ON_COMMAND_RANGE(IDM_DIS_IOSC0, IDM_DIS_IOSC3, OnIOSC)
END_MESSAGE_MAP()

//---------------------------------------------------------------------------
//
//	�E�B���h�E�쐬
//
//---------------------------------------------------------------------------
int CDisasmWnd::OnCreate(LPCREATESTRUCT lpcs)
{
	int i;

	// �A�h���X�o�b�t�@���Ɋm��
	m_pAddrBuf = new DWORD[ m_nHeight ];
	for (i=0; i<m_nHeight; i++) {
		m_pAddrBuf[i] = 0xffffffff;
	}

	// ��{�N���X
	if (CSubTextScrlWnd::OnCreate(lpcs) != 0) {
		return -1;
	}

	// �A�h���X������
	SetAddr(m_dwSetAddr);

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	�E�B���h�E�폜
//
//---------------------------------------------------------------------------
void CDisasmWnd::OnDestroy()
{
	m_bEnable = FALSE;

	// �A�h���X�o�b�t�@���
	if (m_pAddrBuf) {
		delete[] m_pAddrBuf;
		m_pAddrBuf = NULL;
	}

	// ��{�N���X��
	CSubTextScrlWnd::OnDestroy();
}

//---------------------------------------------------------------------------
//
//	�T�C�Y�ύX
//
//---------------------------------------------------------------------------
void CDisasmWnd::OnSize(UINT nType, int cx, int cy)
{
	CRect rect;
	int i;

	ASSERT(this);
	ASSERT(cx >= 0);
	ASSERT(cy >= 0);

	// �A�h���X�o�b�t�@������Έ�x���
	::LockVM();
	if (m_pAddrBuf) {
		delete[] m_pAddrBuf;
		m_pAddrBuf = NULL;
	}
	::UnlockVM();

	// ��{�N���X(���̒��ŁACDisasmWnd::OnSize���ēx�Ă΂��ꍇ����)
	CSubTextScrlWnd::OnSize(nType, cx, cy);

	// �A�h���X�o�b�t�@���ēx�m�ہB����`�F�b�N���s��
	::LockVM();
	if (m_pAddrBuf) {
		delete[] m_pAddrBuf;
		m_pAddrBuf = NULL;
	}
	m_pAddrBuf = new DWORD[ m_nHeight ];
	for (i=0; i<m_nHeight; i++) {
		m_pAddrBuf[i] = 0xffffffff;
	}
	::UnlockVM();

	// �ăA�h���X�Z�b�g
	SetAddr(m_dwSetAddr);
}

//---------------------------------------------------------------------------
//
//	���N���b�N
//
//---------------------------------------------------------------------------
void CDisasmWnd::OnLButtonDown(UINT nFlags, CPoint point)
{
	OnLButtonDblClk(nFlags, point);
}

//---------------------------------------------------------------------------
//
//	���{�^���_�u���N���b�N
//
//---------------------------------------------------------------------------
void CDisasmWnd::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	int y;
	DWORD dwAddr;

	// �A�h���X�o�b�t�@��������ΏI��
	if (!m_pAddrBuf) {
		return;
	}

	// �A�h���X�擾�A�`�F�b�N
	y = point.y / m_tmHeight;
	dwAddr = m_pAddrBuf[y];
	if (dwAddr >= 0x01000000) {
		return;
	}

	// VM���b�N
	::LockVM();

	// �A�h���X������΍폜�A�Ȃ���ΐݒ�
	if (m_pScheduler->IsBreak(dwAddr) >= 0) {
		m_pScheduler->DelBreak(dwAddr);
	}
	else {
		m_pScheduler->SetBreak(dwAddr);
	}

	// VM�A�����b�N
	::UnlockVM();
}

//---------------------------------------------------------------------------
//
//	�X�N���[��(����)
//
//---------------------------------------------------------------------------
void CDisasmWnd::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar *pBar)
{
	SCROLLINFO si;
	DWORD dwDiff;
	DWORD dwAddr;
	int i;

	// �X�N���[�������擾
	memset(&si, 0, sizeof(si));
	si.cbSize = sizeof(si);
	GetScrollInfo(SB_VERT, &si, SIF_ALL);

	// �X�N���[���o�[�R�[�h��
	switch (nSBCode) {
		// ���
		case SB_TOP:
			m_ScrlY = si.nMin;
			break;

		// ����
		case SB_BOTTOM:
			m_ScrlY = si.nMax;
			break;

		// 1���C�����
		case SB_LINEUP:
			// �A�h���X�o�b�t�@���`�F�b�N
			if (m_pAddrBuf) {
				// ��O�̃A�h���X���擾���A�����v�Z
				dwDiff = GetPrevAddr(m_pAddrBuf[0]);
				dwDiff = m_pAddrBuf[0] - dwDiff;

				// ������Ό����A�����Ȃ����-1
				if (dwDiff > 0) {
					dwDiff >>= 1;
					m_ScrlY -= dwDiff;
				}
				else {
					m_ScrlY--;
				}
			}
			break;

		// 1���C������
		case SB_LINEDOWN:
			// �A�h���X�o�b�t�@�����āA1���ߕ��i�߂�
			if (m_nHeight >= 2) {
				if (m_pAddrBuf) {
					dwDiff = m_pAddrBuf[1] - m_pAddrBuf[0];
					dwDiff >>= 1;
					m_ScrlY += dwDiff;
				}
			}
			break;

		// 1�y�[�W���
		case SB_PAGEUP:
			// �A�h���X�o�b�t�@���`�F�b�N
			if (m_pAddrBuf) {
				dwAddr = m_pAddrBuf[0];
				for (i=0; i<m_nHeight-1; i++) {
					// ��O�̃A�h���X���擾���A�����v�Z
					dwDiff = GetPrevAddr(dwAddr);
					dwDiff = dwAddr - dwDiff;
					dwAddr -= dwDiff;

					// ������Ό����A�����Ȃ����-1
					if (dwDiff > 0) {
						dwDiff >>= 1;
						m_ScrlY -= dwDiff;
					}
					else {
						m_ScrlY--;
					}

					// �I�[�o�[�`�F�b�N
					if (m_ScrlY < 0) {
						m_ScrlY = 0;
					}
				}

				// �S���߂�Ȃ������ꍇ���l��
				if (dwAddr == m_pAddrBuf[0]) {
					m_ScrlY -= si.nPage;
				}
			}
			break;

		// 1�y�[�W����
		case SB_PAGEDOWN:
			// �A�h���X�o�b�t�@�����āAm_nHeight���ߕ��i�߂�
			if (m_pAddrBuf) {
				dwDiff = m_pAddrBuf[m_nHeight - 1] - m_pAddrBuf[0];
				dwDiff >>= 1;
				m_ScrlY += dwDiff;
			}
			break;

		// �T���ړ�
		case SB_THUMBPOSITION:
			m_ScrlY = nPos;
			break;
		case SB_THUMBTRACK:
			m_ScrlY = nPos;
			break;
	}

	// �I�[�o�[�`�F�b�N
	if (m_ScrlY < 0) {
		m_ScrlY = 0;
	}
	if (m_ScrlY > si.nMax) {
		m_ScrlY = si.nMax;
	}

	// �Z�b�g
	SetupScrlV();
}

//---------------------------------------------------------------------------
//
//	�R���e�L�X�g���j���[
//
//---------------------------------------------------------------------------
void CDisasmWnd::OnContextMenu(CWnd *pWnd, CPoint point)
{
	CRect rect;
	CMenu menu;
	CMenu *pMenu;

	// �N���C�A���g�̈���ŉ����ꂽ�����肷��
	GetClientRect(&rect);
	ClientToScreen(&rect);
	if (!rect.PtInRect(point)) {
		CSubTextScrlWnd::OnContextMenu(pWnd, point);
		return;
	}

	// ���j���[���[�h
	if (::IsJapanese()) {
		menu.LoadMenu(IDR_DISMENU);
	}
	else {
		menu.LoadMenu(IDR_US_DISMENU);
	}
	pMenu = menu.GetSubMenu(0);

	// �Z�b�g�A�b�v
	SetupContext(pMenu);

	// ���j���[���s
	pMenu->TrackPopupMenu(TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_RIGHTBUTTON,
							point.x, point.y, this);
}

//---------------------------------------------------------------------------
//
//	�R���e�L�X�g���j���[ �Z�b�g�A�b�v
//
//---------------------------------------------------------------------------
void FASTCALL CDisasmWnd::SetupContext(CMenu *pMenu)
{
	CMenu *pSubMenu;
	int i;

	ASSERT(pMenu);

	// PC�ɓ���
	if (m_bSync) {
		pMenu->CheckMenuItem(IDM_DIS_SYNC, MF_BYCOMMAND | MF_CHECKED);
	}

	// �V�����E�B���h�E
	if (!m_pDrawView->IsNewWindow(TRUE)) {
		pMenu->EnableMenuItem(IDM_DIS_NEWWIN, MF_BYCOMMAND | MF_GRAYED);
	}

	// MPU���W�X�^�E�X�^�b�N�E�A�h���X�q�X�g��
	CCPURegWnd::SetupRegMenu(pMenu, m_pCPU, FALSE);
	CMemoryWnd::SetupStackMenu(pMenu, m_pMemory, m_pCPU);
	CAddrDlg::SetupHisMenu(pMenu);

	// �u���[�N�|�C���g
	SetupBreakMenu(pMenu, m_pScheduler);

	// VM�����b�N
	::LockVM();

	// ���荞�݃x�N�^����
	pSubMenu = pMenu->GetSubMenu(9);

	// MPU�W��
	SetupVector(pSubMenu, 0, 1, 11);

	// trap #x
	SetupVector(pSubMenu, 12, 0x20, 16);

	// MFP
	SetupVector(pSubMenu, 29, (m_pMFP->GetVR() & 0xf0), 16);

	// SCC
	for (i=0; i<8; i++) {
		SetupVector(pSubMenu, 46 + i, m_pSCC->GetVector(i), 1);
	}

	// DMAC
	for (i=0; i<8; i++) {
		SetupVector(pSubMenu, 55 + i, m_pDMAC->GetVector(i), 1);
	}

	// IOSC
	SetupVector(pSubMenu, 64, m_pIOSC->GetVector(), 4);

	// VM���A�����b�N
	::UnlockVM();
}

//---------------------------------------------------------------------------
//
//	���荞�݃x�N�^�Z�b�g�A�b�v
//
//---------------------------------------------------------------------------
void FASTCALL CDisasmWnd::SetupVector(CMenu *pMenu, UINT index, DWORD vector, int num)
{
	int i;
	DWORD handler;

	ASSERT(pMenu);
	ASSERT(num > 0);

	// ���荞�݃x�N�^�A�h���X������
	vector <<= 2;

	// ���[�v
	for (i=0; i<num; i++) {
		// ���荞�݃n���h���A�h���X�擾
		handler = (DWORD)m_pMemory->ReadOnly(vector + 1);
		handler <<= 8;
		handler |= (DWORD)m_pMemory->ReadOnly(vector + 2);
		handler <<= 8;
		handler |= (DWORD)m_pMemory->ReadOnly(vector + 3);
		vector += 4;

		// �A�h���X�Z�b�g
		SetupAddress(pMenu, index, handler);
		index++;
	}
}

//---------------------------------------------------------------------------
//
//	�A�h���X�Z�b�g�A�b�v
//
//---------------------------------------------------------------------------
void FASTCALL CDisasmWnd::SetupAddress(CMenu *pMenu, UINT index, DWORD addr)
{
	CString string;
	CString menustr;
	int ext;
	UINT id;

	ASSERT(pMenu);
	ASSERT(addr <= 0xffffff);

	// ���݂̕�������擾
	pMenu->GetMenuString(index, string, MF_BYPOSITION);

	// �J�b�R�̐擪��T���āA����΂���ȍ~�̂�
	ext = string.Find(" : ");
	if (ext >= 0) {
		menustr = string.Mid(ext + 3);
	}
	else {
		menustr = string;
	}

	// ($)�̕�������쐬
	string.Format("$%06X : ", addr);
	string += menustr;

	// ������Z�b�g
	id = pMenu->GetMenuItemID(index);
	pMenu->ModifyMenu(index, MF_BYPOSITION | MF_STRING, id, string);
}

//---------------------------------------------------------------------------
//
//	�V�����E�B���h�E
//
//---------------------------------------------------------------------------
void CDisasmWnd::OnNewWin()
{
	CDisasmWnd *pDisasmWnd;
	DWORD dwAddr;

	// �e�E�B���h�E�ɑ΂��A�V�����E�B���h�E�̍쐬��v��
	pDisasmWnd = (CDisasmWnd*)m_pDrawView->NewWindow(TRUE);

	// ����������A�����Ɠ����A�h���X��n��
	if (pDisasmWnd) {
		dwAddr = m_ScrlY * 2;
		dwAddr += m_dwAddr;
		pDisasmWnd->SetAddr(dwAddr);
	}
}

//---------------------------------------------------------------------------
//
//	PC�ֈړ�
//
//---------------------------------------------------------------------------
void CDisasmWnd::OnPC()
{
	// ���݂�PC�ɃA�h���X�Z�b�g(������Refresh���s��)
	SetAddr(m_pCPU->GetPC());
}

//---------------------------------------------------------------------------
//
//	PC�Ɠ���
//
//---------------------------------------------------------------------------
void CDisasmWnd::OnSync()
{
	m_bSync = (!m_bSync);
}

//---------------------------------------------------------------------------
//
//	�A�h���X����
//
//---------------------------------------------------------------------------
void CDisasmWnd::OnAddr()
{
	CAddrDlg dlg(this);

	// �_�C�A���O���s
	dlg.m_dwValue = m_dwSetAddr;
	if (dlg.DoModal() != IDOK) {
		return;
	}

	// �A�h���X�Z�b�g
	SetAddr(dlg.m_dwValue);
}

//---------------------------------------------------------------------------
//
//	MPU���W�X�^
//
//---------------------------------------------------------------------------
void CDisasmWnd::OnReg(UINT nID)
{
	DWORD dwAddr;

	ASSERT((nID >= IDM_REG_D0) && (nID <= IDM_REG_PC));

	dwAddr = CCPURegWnd::GetRegValue(m_pCPU, nID);
	SetAddr(dwAddr);
}

//---------------------------------------------------------------------------
//
//	�X�^�b�N
//
//---------------------------------------------------------------------------
void CDisasmWnd::OnStack(UINT nID)
{
	DWORD dwAddr;

	ASSERT((nID >= IDM_STACK_0) && (nID <= IDM_STACK_F));

	dwAddr = CMemoryWnd::GetStackAddr(nID, m_pMemory, m_pCPU);
	SetAddr(dwAddr);
}

//---------------------------------------------------------------------------
//
//	�u���[�N�|�C���g
//
//---------------------------------------------------------------------------
void CDisasmWnd::OnBreak(UINT nID)
{
	DWORD dwAddr;

	ASSERT((nID >= IDM_DIS_BREAKP0) && (nID <= IDM_DIS_BREAKP7));

	dwAddr = GetBreak(nID, m_pScheduler);
	SetAddr(dwAddr);
}

//---------------------------------------------------------------------------
//
//	�A�h���X�q�X�g��
//
//---------------------------------------------------------------------------
void CDisasmWnd::OnHistory(UINT nID)
{
	DWORD dwAddr;

	ASSERT((nID >= IDM_HISTORY_0) && (nID <= IDM_HISTORY_9));

	dwAddr = CAddrDlg::GetAddr(nID);
	SetAddr(dwAddr);
}

//---------------------------------------------------------------------------
//
//	CPU��O�x�N�^
//
//---------------------------------------------------------------------------
void CDisasmWnd::OnCPUExcept(UINT nID)
{
	nID -= IDM_DIS_RESET;

	OnVector(nID + 1);
}

//---------------------------------------------------------------------------
//
//	trap�x�N�^
//
//---------------------------------------------------------------------------
void CDisasmWnd::OnTrap(UINT nID)
{
	nID -= IDM_DIS_TRAP0;

	OnVector(nID + 0x20);
}

//---------------------------------------------------------------------------
//
//	MFP�x�N�^
//
//---------------------------------------------------------------------------
void CDisasmWnd::OnMFP(UINT nID)
{
	nID -= IDM_DIS_MFP0;

	OnVector(nID + (m_pMFP->GetVR() & 0xf0));
}

//---------------------------------------------------------------------------
//
//	SCC�x�N�^
//
//---------------------------------------------------------------------------
void CDisasmWnd::OnSCC(UINT nID)
{
	DWORD vector;

	nID -= IDM_DIS_SCC0;
	ASSERT(nID <= 7);

	// �x�N�^�ԍ��𓾂�
	::LockVM();
	vector = m_pSCC->GetVector(nID);
	::UnlockVM();

	OnVector(vector);
}

//---------------------------------------------------------------------------
//
//	DMAC�x�N�^
//
//---------------------------------------------------------------------------
void CDisasmWnd::OnDMAC(UINT nID)
{
	DWORD vector;

	nID -= IDM_DIS_DMAC0;
	ASSERT(nID <= 7);

	// �x�N�^�ԍ��𓾂�
	::LockVM();
	vector = m_pDMAC->GetVector(nID);
	::UnlockVM();

	OnVector(vector);
}

//---------------------------------------------------------------------------
//
//	IOSC�x�N�^
//
//---------------------------------------------------------------------------
void CDisasmWnd::OnIOSC(UINT nID)
{
	DWORD vector;

	nID -= IDM_DIS_IOSC0;
	ASSERT(nID <= 3);

	// �x�N�^�ԍ��𓾂�
	::LockVM();
	vector = m_pIOSC->GetVector() + nID;
	::UnlockVM();

	OnVector(vector);
}

//---------------------------------------------------------------------------
//
//	�x�N�^�w��
//
//---------------------------------------------------------------------------
void FASTCALL CDisasmWnd::OnVector(UINT vector)
{
	DWORD addr;

	// �x�N�^�ǂݏo��
	::LockVM();
	vector <<= 2;
	addr = (DWORD)m_pMemory->ReadOnly(vector + 1);
	addr <<= 8;
	addr |= (DWORD)m_pMemory->ReadOnly(vector + 2);
	addr <<= 8;
	addr |= (DWORD)m_pMemory->ReadOnly(vector + 3);
	::UnlockVM();

	// �A�h���X�w��
	SetAddr(addr);
}

//---------------------------------------------------------------------------
//
//	�A�h���X�w��
//
//---------------------------------------------------------------------------
void FASTCALL CDisasmWnd::SetAddr(DWORD dwAddr)
{
	int offset;
	CString string;

	::LockVM();

	// �A�h���X�L��
	dwAddr &= 0xffffff;
	m_dwSetAddr = dwAddr;
	m_dwAddr = dwAddr & 0xff0000;

	// ���ʂ̂ݎ��o��
	offset = dwAddr & 0x00ffff;
	offset >>= 1;

	// �X�N���[��
	m_ScrlY = offset;
	::UnlockVM();
	SetScrollPos(SB_VERT, offset, TRUE);

	// �L���v�V������������X�V
	string.Format(" [%d] ($%06X - $%06X)", (m_dwID & 0xff) - 'A' + 1, m_dwAddr, m_dwAddr + 0xffff);
	string = m_strCaption + string;
	if (m_Caption != string) {
		m_Caption = string;
		SetWindowText(string);
	}
}

//---------------------------------------------------------------------------
//
//	PC�w��
//
//---------------------------------------------------------------------------
void FASTCALL CDisasmWnd::SetPC(DWORD pc)
{
	ASSERT(pc <= 0xffffff);

	// �����t���O�������Ă���΁A�A�h���X�Z�b�g
	if (m_bSync) {
		m_dwPC = pc;
	}
	else {
		m_dwPC = 0xffffffff;
	}
}

//---------------------------------------------------------------------------
//
//	���b�Z�[�W�X���b�h����̍X�V
//
//---------------------------------------------------------------------------
void FASTCALL CDisasmWnd::Update()
{
	// PC�w��
	if (m_dwPC < 0x1000000) {
		SetAddr(m_dwPC);
		m_dwPC = 0xffffffff;
	}
}

//---------------------------------------------------------------------------
//
//	�Z�b�g�A�b�v
//
//---------------------------------------------------------------------------
void FASTCALL CDisasmWnd::Setup()
{
	DWORD dwAddr;
	DWORD dwPC;
	int i;
	int j;
	int k;

	// �A�h���X�w��
	dwAddr = (m_dwAddr & 0xff0000);
	dwAddr |= (DWORD)(m_ScrlY * 2);
	::debugpc = dwAddr;

	// PC�擾
	dwPC = m_pCPU->GetPC();

	// ���[�v
	for (i=0; i<m_nHeight; i++) {
		dwAddr = ::debugpc;

		// �A�h���X�i�[
		if (m_pAddrBuf) {
			m_pAddrBuf[i] = dwAddr;
		}

		// �A�h���X���[�v�`�F�b�N(FFFFFFF���[�v���l��)
		if (dwAddr > 0xffffff) {
			// ���[�v�����B�A�h���X�̓z�[���h
			ASSERT(i > 0);
			if (m_pAddrBuf) {
				m_pAddrBuf[i] = m_pAddrBuf[i - 1];
			}

			// ����
			Reverse(FALSE);
			for (j=0; j<m_nWidth; j++) {
				SetChr(j, i, ' ');
			}
			continue;
		}

		// ��������
		k = m_pScheduler->IsBreak(dwAddr);
		if (k >= 0) {
			Reverse(TRUE);
		}
		else {
			Reverse(FALSE);
		}
		// �h��Ԃ�
		for (j=0; j<m_nWidth; j++) {
			SetChr(j, i, ' ');
		}

		// �t�A�Z���u��
		::cpudebug_disassemble(1);

		// PC�}�[�N�A�u���[�N�}�[�N
		if (k >= 0) {
			::debugbuf[0] = (char)(k + '1');
		}
		else {
			::debugbuf[0] = ' ';
		}
		if (dwAddr == dwPC) {
			::debugbuf[1] = '>';
		}
		else {
			::debugbuf[1] = ' ';
		}

		// �\��
		if (m_ScrlX < (int)strlen(::debugbuf)) {
			SetString(0, i, &debugbuf[m_ScrlX]);
		}

		// �u���[�N�|�C���g�Ŗ����̏ꍇ�ɑΉ�
		k = m_pScheduler->IsBreak(dwAddr, TRUE);
		if (k >= 0) {
			Reverse(TRUE);
			SetChr(0, i, (char)(k + '1'));
		}
	}
}

//---------------------------------------------------------------------------
//
//	��O�̃A�h���X���擾
//
//---------------------------------------------------------------------------
DWORD FASTCALL CDisasmWnd::GetPrevAddr(DWORD dwAddr)
{
	int i;
	DWORD dwTest;

	ASSERT(dwAddr <= 0xffffff);

	// �A�h���X������
	dwTest = dwAddr;

	for (i=0; i<16; i++) {
		// dwTest�������A�I�[�o�[�`�F�b�N
		dwTest -= 2;
		if (dwTest >= 0x01000000) {
			return dwAddr;
		}

		// ��������t�A�Z���u�����āA�A�h���X�������݂�
		::debugpc = dwTest;
		::cpudebug_disassemble(1);

		// ��v���Ă����"UNRECOG"�`�F�b�N
		if (::debugpc == dwAddr) {
			if ((::debugbuf[35] == 'U') || (::debugbuf[36] == 'N') || (::debugbuf[37] == 'R')) {
				continue;
			}
			// ok�A���^�[��
			return dwTest;
		}
	}

	// �s��v�BdwAddr��Ԃ�
	return dwAddr;
}

//---------------------------------------------------------------------------
//
//	�u���[�N�|�C���g���j���[ �Z�b�g�A�b�v
//
//---------------------------------------------------------------------------
void FASTCALL CDisasmWnd::SetupBreakMenu(CMenu *pMenu, Scheduler *pScheduler)
{
	int num;
	int i;
	Scheduler::breakpoint_t buf;
	CString string;

	ASSERT(pMenu);
	ASSERT(pScheduler);

	// ���N���A
	num = 0;

	// �ݒ�
	::LockVM();
	for (i=0; i<Scheduler::BreakMax; i++) {
		pScheduler->GetBreak(i, &buf);
		if (buf.use) {
			// �g�p���Ȃ̂ŁA�ݒ�
			string.Format("%1d : $%06X", num + 1, buf.addr);
			pMenu->ModifyMenu(IDM_DIS_BREAKP0 + num, MF_BYCOMMAND | MF_STRING,
				IDM_DIS_BREAKP0 + num, (LPCTSTR)string);

			// +1
			num++;
		}
	}
	::UnlockVM();

	// ����ȊO�̂Ƃ��̓N���A
	for (i=num; i<Scheduler::BreakMax; i++) {
		pMenu->DeleteMenu(IDM_DIS_BREAKP0 + i, MF_BYCOMMAND);
	}
}

//---------------------------------------------------------------------------
//
//	�u���[�N�|�C���g���j���[�擾
//
//---------------------------------------------------------------------------
DWORD FASTCALL CDisasmWnd::GetBreak(UINT nID, Scheduler *pScheduler)
{
	int i;
	Scheduler::breakpoint_t buf;

	ASSERT((nID >= IDM_DIS_BREAKP0) && (nID <= IDM_DIS_BREAKP7));
	ASSERT(pScheduler);
	nID -= IDM_DIS_BREAKP0;

	// �������[�v
	::LockVM();
	for (i=0; i<Scheduler::BreakMax; i++) {
		pScheduler->GetBreak(i, &buf);
		if (buf.use) {
			// �g�p���Ȃ̂ŁA���ꂩ�H
			if (nID == 0) {
				::UnlockVM();
				return buf.addr;
			}
			nID--;
		}
	}
	::UnlockVM();

	return 0;
}

//===========================================================================
//
//	�������E�B���h�E
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	�R���X�g���N�^
//
//---------------------------------------------------------------------------
CMemoryWnd::CMemoryWnd(int nWnd)
{
	// �������E�B���h�E��8��ނ܂�
	ASSERT((nWnd >= 0) && (nWnd <= 7));

	// �E�B���h�E�p�����[�^��`
	m_dwID = MAKEID('M', 'E', 'M', (nWnd + 'A'));
	::GetMsg(IDS_SWND_MEMORY, m_strCaption);
	m_nWidth = 73;
	m_nHeight = 16;

	// �E�B���h�E�p�����[�^��`(�X�N���[��)
	m_ScrlWidth = 73;
	m_ScrlHeight = 0x8000;

	// CPU�擾
	m_pCPU = (CPU*)::GetVM()->SearchDevice(MAKEID('C', 'P', 'U', ' '));
	ASSERT(m_pCPU);

	// �������擾
	m_pMemory = (Memory*)::GetVM()->SearchDevice(MAKEID('M', 'E', 'M', ' '));
	ASSERT(m_pMemory);

	// ���̑�
	m_dwAddr = 0;
	m_nUnit = 0;
	m_strCaptionReq.Empty();
	m_strCaptionSet.Empty();
}

//---------------------------------------------------------------------------
//
//	���b�Z�[�W �}�b�v
//
//---------------------------------------------------------------------------
BEGIN_MESSAGE_MAP(CMemoryWnd, CSubTextScrlWnd)
	ON_WM_CREATE()
	ON_WM_PAINT()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_CONTEXTMENU()
	ON_COMMAND(IDM_MEMORY_ADDR, OnAddr)
	ON_COMMAND(IDM_MEMORY_NEWWIN, OnNewWin)
	ON_COMMAND_RANGE(IDM_MEMORY_BYTE, IDM_MEMORY_LONG, OnUnit)
	ON_COMMAND_RANGE(IDM_MEMORY_0, IDM_MEMORY_F, OnRange)
	ON_COMMAND_RANGE(IDM_REG_D0, IDM_REG_PC, OnReg)
	ON_COMMAND_RANGE(IDM_AREA_MPU, IDM_AREA_IPLROM, OnArea)
	ON_COMMAND_RANGE(IDM_HISTORY_0, IDM_HISTORY_9, OnHistory)
	ON_COMMAND_RANGE(IDM_STACK_0, IDM_STACK_F, OnStack)
END_MESSAGE_MAP()

//---------------------------------------------------------------------------
//
//	�E�B���h�E�쐬
//
//---------------------------------------------------------------------------
int CMemoryWnd::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	// ��{�N���X
	if (CSubTextScrlWnd::OnCreate(lpCreateStruct) != 0) {
		return -1;
	}

	// �A�h���X������
	SetAddr(0);
	return TRUE;
}

//---------------------------------------------------------------------------
//
//	�`��
//
//---------------------------------------------------------------------------
void CMemoryWnd::OnPaint()
{
	// ��{�N���X
	CSubTextScrlWnd::OnPaint();

	// �L���v�V�����ݒ�
	SetWindowText(m_strCaption);
}

//---------------------------------------------------------------------------
//
//	���{�^���_�u���N���b�N
//
//---------------------------------------------------------------------------
void CMemoryWnd::OnLButtonDblClk(UINT /*nFlags*/, CPoint point)
{
	int x;
	int y;
	DWORD dwAddr;
	DWORD dwData;
	CDataDlg dlg(this);

	// x, y�Z�o
	x = point.x / m_tmWidth;
	y = point.y / m_tmHeight;

	// x�`�F�b�N
	if (x < 8) {
		return;
	}
	x -= 8;

	// y�̃A�h���X�𓾂�
	dwAddr = m_dwAddr | (m_ScrlY << 5);
	dwAddr += (y << 4);
	if ((dwAddr - m_dwAddr) >= 0x100000) {
		return;
	}

	// x����A�h���X�𓾂�
	switch (m_nUnit) {
		// Byte
		case 0:
			x /= 3;
			break;
		// Word
		case 1:
			x /= 5;
			x <<= 1;
			break;
		// Long
		case 2:
			x /= 9;
			x <<= 2;
			break;
		// ���̑�
		default:
			ASSERT(FALSE);
			break;
	}
	if (x >= 16) {
		return;
	}
	dwAddr += x;

	// �f�[�^�ǂݍ���
	::LockVM();
	switch (m_nUnit) {
		// Byte
		case 0:
			dwData = m_pMemory->ReadOnly(dwAddr);
			break;
		// Word
		case 1:
			dwData = m_pMemory->ReadOnly(dwAddr);
			dwData <<= 8;
			dwData |= m_pMemory->ReadOnly(dwAddr + 1);
			break;
		// Long
		case 2:
			dwData = m_pMemory->ReadOnly(dwAddr);
			dwData <<= 8;
			dwData |= m_pMemory->ReadOnly(dwAddr + 1);
			dwData <<= 8;
			dwData |= m_pMemory->ReadOnly(dwAddr + 2);
			dwData <<= 8;
			dwData |= m_pMemory->ReadOnly(dwAddr + 3);
			break;
		// ���̑�
		default:
			dwData = 0;
			ASSERT(FALSE);
			break;
	}
	::UnlockVM();

	// �_�C�A���O���s
	dlg.m_dwAddr = dwAddr;
	dlg.m_dwValue = dwData;
	dlg.m_nSize = m_nUnit;
	if (dlg.DoModal() != IDOK) {
		return;
	}

	// ��������
	dwData = dlg.m_dwValue;
	::LockVM();
	switch (m_nUnit) {
		// Byte
		case 0:
			m_pMemory->WriteByte(dwAddr, dwData);
			break;
		// Word
		case 1:
			m_pMemory->WriteWord(dwAddr, dwData);
			break;
		// Long
		case 2:
			m_pMemory->WriteWord(dwAddr, (WORD)(dwData >> 16));
			m_pMemory->WriteWord(dwAddr + 2, (WORD)dwData);
			break;
		// ���̑�
		default:
			ASSERT(FALSE);
			break;
	}
	::UnlockVM();
}

//---------------------------------------------------------------------------
//
//	�R���e�L�X�g���j���[
//
//---------------------------------------------------------------------------
void CMemoryWnd::OnContextMenu(CWnd *pWnd, CPoint point)
{
	CRect rect;
	CMenu menu;
	CMenu *pMenu;

	// �N���C�A���g�̈���ŉ����ꂽ�����肷��
	GetClientRect(&rect);
	ClientToScreen(&rect);
	if (!rect.PtInRect(point)) {
		CSubTextScrlWnd::OnContextMenu(pWnd, point);
		return;
	}

	// ���j���[���s
	if (::IsJapanese()) {
		menu.LoadMenu(IDR_MEMORYMENU);
	}
	else {
		menu.LoadMenu(IDR_US_MEMORYMENU);
	}
	pMenu = menu.GetSubMenu(0);
	SetupContext(pMenu);
	pMenu->TrackPopupMenu(TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_RIGHTBUTTON,
							point.x, point.y, this);
}

//---------------------------------------------------------------------------
//
//	�R���e�L�X�g���j���[ �Z�b�g�A�b�v
//
//---------------------------------------------------------------------------
void FASTCALL CMemoryWnd::SetupContext(CMenu *pMenu)
{
	ASSERT(pMenu);

	// �V�����E�B���h�E
	if (!m_pDrawView->IsNewWindow(FALSE)) {
		pMenu->EnableMenuItem(IDM_MEMORY_NEWWIN, MF_BYCOMMAND | MF_GRAYED);
	}

	// �T�C�Y�`�F�b�N
	pMenu->CheckMenuRadioItem(IDM_MEMORY_BYTE, IDM_MEMORY_LONG,
			IDM_MEMORY_BYTE + m_nUnit, MF_BYCOMMAND);

	// �A�h���X�`�F�b�N
	pMenu->CheckMenuRadioItem(IDM_MEMORY_0, IDM_MEMORY_F,
			IDM_MEMORY_0 + (m_dwAddr >> 20), MF_BYCOMMAND);

	// MPU���W�X�^
	CCPURegWnd::SetupRegMenu(pMenu, m_pCPU, FALSE);

	// �A�h���X�q�X�g��
	CAddrDlg::SetupHisMenu(pMenu);

	// �X�^�b�N
	SetupStackMenu(pMenu, m_pMemory, m_pCPU);
}

//---------------------------------------------------------------------------
//
//	�A�h���X����
//
//---------------------------------------------------------------------------
void CMemoryWnd::OnAddr()
{
	CAddrDlg dlg(this);

	// �_�C�A���O���s
	dlg.m_dwValue = m_dwAddr | (m_ScrlY * 0x20);
	if (dlg.DoModal() != IDOK) {
		return;
	}

	// �A�h���X�Z�b�g
	SetAddr(dlg.m_dwValue);
}

//---------------------------------------------------------------------------
//
//	�V�����E�B���h�E
//
//---------------------------------------------------------------------------
void CMemoryWnd::OnNewWin()
{
	CMemoryWnd *pWnd;

	// �e�E�B���h�E�ɑ΂��A�V�����E�B���h�E�̍쐬��v��
	pWnd = (CMemoryWnd*)m_pDrawView->NewWindow(FALSE);

	// �쐬�ł�����A�����Ɠ����A�h���X�E�T�C�Y���w��
	if (pWnd) {
		pWnd->SetAddr(m_dwAddr | (m_ScrlY * 0x20));
		pWnd->SetUnit(m_nUnit);
	}
}

//---------------------------------------------------------------------------
//
//	�\���P�ʎw��
//
//---------------------------------------------------------------------------
void CMemoryWnd::OnUnit(UINT uID)
{
	int unit;

	unit = (int)(uID - IDM_MEMORY_BYTE);
	ASSERT((unit >= 0) && (unit <= 2));

	SetUnit(unit);
}

//---------------------------------------------------------------------------
//
//	�\���P�ʃZ�b�g
//
//---------------------------------------------------------------------------
void FASTCALL CMemoryWnd::SetUnit(int nUnit)
{
	ASSERT(this);
	ASSERT((nUnit >= 0) && (nUnit <= 2));

	// ���b�N�A�ύX
	::LockVM();
	m_nUnit = nUnit;
	::UnlockVM();
}

//---------------------------------------------------------------------------
//
//	�A�h���X�͈͎w��
//
//---------------------------------------------------------------------------
void CMemoryWnd::OnRange(UINT uID)
{
	DWORD dwAddr;

	ASSERT((uID >= IDM_MEMORY_0) && (uID <= IDM_MEMORY_F));

	dwAddr = (DWORD)(uID - IDM_MEMORY_0);
	dwAddr *= 0x100000;
	dwAddr |= (DWORD)(m_ScrlY * 0x20);
	SetAddr(dwAddr);
}

//---------------------------------------------------------------------------
//
//	���W�X�^�w��
//
//---------------------------------------------------------------------------
void CMemoryWnd::OnReg(UINT uID)
{
	DWORD dwAddr;

	ASSERT((uID >= IDM_REG_D0) && (uID <= IDM_REG_PC));

	dwAddr = CCPURegWnd::GetRegValue(m_pCPU, uID);
	SetAddr(dwAddr);
}

//---------------------------------------------------------------------------
//
//	�G���A�w��
//
//---------------------------------------------------------------------------
void CMemoryWnd::OnArea(UINT uID)
{
	CMenu menu;
	TCHAR buf[0x100];
	DWORD dwAddr;

	ASSERT((uID >= IDM_AREA_MPU) && (uID <= IDM_AREA_IPLROM));

	// ���j���[�����[�h
	if (::IsJapanese()) {
		menu.LoadMenu(IDR_MEMORYMENU);
	}
	else {
		menu.LoadMenu(IDR_US_MEMORYMENU);
	}

	// �w��ID�̃��j���[��������擾
	menu.GetMenuString(uID, buf, 0x100, MF_BYCOMMAND);

	// "$000000 : "�̌`�������肷��
	buf[0] = _T('0');
	buf[7] = _T('\0');
	dwAddr = ::_tcstoul(buf, NULL, 16);
	SetAddr(dwAddr);
}

//---------------------------------------------------------------------------
//
//	�q�X�g���w��
//
//---------------------------------------------------------------------------
void CMemoryWnd::OnHistory(UINT uID)
{
	DWORD dwAddr;

	ASSERT((uID >= IDM_HISTORY_0) && (uID <= IDM_HISTORY_9));
	dwAddr = CAddrDlg::GetAddr(uID);
	SetAddr(dwAddr);
}

//---------------------------------------------------------------------------
//
//	�X�^�b�N�w��
//
//---------------------------------------------------------------------------
void CMemoryWnd::OnStack(UINT uID)
{
	DWORD dwAddr;

	ASSERT((uID >= IDM_STACK_0) && (uID <= IDM_STACK_F));
	dwAddr = GetStackAddr(uID, m_pMemory, m_pCPU);
	SetAddr(dwAddr);
}

//---------------------------------------------------------------------------
//
//	�A�h���X�w��
//
//---------------------------------------------------------------------------
void FASTCALL CMemoryWnd::SetAddr(DWORD dwAddr)
{
	int offset;
	CString strCap;

	ASSERT(this);
	ASSERT(dwAddr <= 0x1000000);

	// ��ʂ̂ݎ��o��
	m_dwAddr = dwAddr & 0xf00000;

	// ���ʂ̂ݎ��o��
	offset = dwAddr & 0x0fffff;
	offset /= 0x20;

	// �X�N���[��
	m_ScrlY = offset;
	SetScrollPos(SB_VERT, offset, TRUE);

	// �L���v�V����������쐬
	strCap.Format(_T(" [%d] ($%06X - $%06X)"), (m_dwID & 0xff) - 'A' + 1,
									m_dwAddr, m_dwAddr + 0x0fffff);
	m_CSection.Lock();
	m_strCaptionReq = m_strCaption + strCap;
	m_CSection.Unlock();
}

//---------------------------------------------------------------------------
//
//	���b�Z�[�W�X���b�h����̍X�V
//
//---------------------------------------------------------------------------
void FASTCALL CMemoryWnd::Update()
{
	CString strCap;

	// �L���v�V������������擾
	m_CSection.Lock();
	strCap = m_strCaptionReq;
	m_CSection.Unlock();

	// ��r
	if (m_strCaptionSet != strCap) {
		m_strCaptionSet = strCap;
		SetWindowText(m_strCaptionSet);
	}
}

//---------------------------------------------------------------------------
//
//	�Z�b�g�A�b�v
//
//---------------------------------------------------------------------------
void FASTCALL CMemoryWnd::Setup()
{
	int x;
	int y;
	CString strText;
	CString strHex;
	DWORD dwAddr;
	DWORD dwOffset;
	TCHAR szAscii[2];

	// �N���A�A�A�h���X������
	Clear();
	dwAddr = (m_dwAddr & 0xf00000);
	dwOffset = (DWORD)(m_ScrlY << 5);
	dwAddr |= dwOffset;

	// �����񏉊���
	szAscii[1] = _T('\0');

	// y���[�v
	for (y=0; y<m_nHeight; y++) {
		// �I�[�o�[�`�F�b�N
		if (dwOffset >= 0x100000) {
			break;
		}

		// �A�h���X�\��
		strText.Format(_T("%06X:"), dwAddr);

		// x���[�v
		switch (m_nUnit) {
			// Byte
			case 0:
				for (x=0; x<16; x++) {
					strHex.Format(_T(" %02X"), m_pMemory->ReadOnly(dwAddr));
					strText += strHex;
					dwAddr++;
				}
				break;
			// Word
			case 1:
				for (x=0; x<8; x++) {
					strHex.Format(_T(" %02X%02X"),  m_pMemory->ReadOnly(dwAddr),
													m_pMemory->ReadOnly(dwAddr + 1));
					strText += strHex;
					dwAddr += 2;
				}
				break;
			// Long
			case 2:
				for (x=0; x<4; x++) {
					strHex.Format(" %02X%02X%02X%02X",  m_pMemory->ReadOnly(dwAddr),
														m_pMemory->ReadOnly(dwAddr + 1),
														m_pMemory->ReadOnly(dwAddr + 2),
														m_pMemory->ReadOnly(dwAddr + 3));
					strText += strHex;
					dwAddr += 4;
				}
				break;
			// ���̑�(���肦�Ȃ�)
			default:
				ASSERT(FALSE);
				break;
		}

		// ��x�߂�
		dwAddr -= 0x10;
		dwAddr &= 0xffffff;

		// ASCII�L�����N�^�ǉ�
		strText += _T("  ");
		for (x=0; x<16; x++) {
			szAscii[0] = (TCHAR)m_pMemory->ReadOnly(dwAddr + x);
			if ((szAscii[0] >= 0) && (szAscii[0] < 0x20)) {
				szAscii[0] = TCHAR('.');
			}
			if ((szAscii[0] < 0) || (szAscii[0] >= 0x7f)) {
				szAscii[0] = TCHAR('.');
			}
			strText += szAscii;
		}
		dwAddr += 0x10;
		dwAddr &= 0xffffff;

		// �I�t�Z�b�g��i�߂�
		dwOffset += 0x10;

		// �\��
		if (m_ScrlX < strText.GetLength()) {
			SetString(0, y, (LPCTSTR)(strText) + m_ScrlX * sizeof(TCHAR));
		}
	}
}

//---------------------------------------------------------------------------
//
//	�X�N���[������(����)
//
//---------------------------------------------------------------------------
void FASTCALL CMemoryWnd::SetupScrlV()
{
	SCROLLINFO si;
	CRect rect;
	int height;

	// �����\���L�����N�^�����擾
	GetClientRect(&rect);
	height = rect.bottom / m_tmHeight;

	// �␳(�X�N���[���P�ʂɂ�2�s�̂���)
	height >>= 1;

	// �X�N���[�������Z�b�g
	memset(&si, 0, sizeof(si));
	si.cbSize = sizeof(si);
	si.fMask = SIF_ALL;
	si.nMin = 0;
	si.nMax = m_ScrlHeight - 1;
	si.nPage = height;

	// �ʒu�́A�K�v�Ȃ�␳����
	si.nPos = m_ScrlY;
	if (si.nPos + height > m_ScrlHeight) {
		si.nPos = m_ScrlHeight - height;
		if (si.nPos < 0) {
			si.nPos = 0;
		}
		m_ScrlY = si.nPos;
	}

	SetScrollInfo(SB_VERT, &si, TRUE);
}

//---------------------------------------------------------------------------
//
//	�X�^�b�N���j���[�Z�b�g�A�b�v
//
//---------------------------------------------------------------------------
void CMemoryWnd::SetupStackMenu(CMenu *pMenu, Memory *pMemory, CPU *pCPU)
{
	int i;
	CPU::cpu_t reg;
	DWORD dwAddr;
	DWORD dwValue;
	CString strMenu;

	ASSERT(pMenu);
	ASSERT(pMemory);
	ASSERT(pCPU);

	// VM���b�N�A���W�X�^�擾
	::LockVM();
	pCPU->GetCPU(&reg);

	// 16���x��
	for (i=0; i<16; i++) {
		// �A�h���X�Z�o
		dwAddr = reg.areg[7];
		dwAddr += (i << 1);
		dwAddr &= 0xfffffe;

		// �f�[�^�擾
		dwValue = pMemory->ReadOnly(dwAddr + 1);
		dwAddr = (dwAddr + 2) & 0xfffffe;
		dwValue <<= 8;
		dwValue |= pMemory->ReadOnly(dwAddr);
		dwValue <<= 8;
		dwValue |= pMemory->ReadOnly(dwAddr + 1);

		// ���j���[�X�V
		strMenu.Format(_T("(A7+%1X) : $%06X"), (i << 1), dwValue);
		pMenu->ModifyMenu(IDM_STACK_0 + i, MF_BYCOMMAND | MF_STRING,
							IDM_STACK_0 + i, (LPCTSTR)strMenu);
	}

	// VM�A�����b�N
	::UnlockVM();
}

//---------------------------------------------------------------------------
//
//	�X�^�b�N�A�h���X�擾
//
//---------------------------------------------------------------------------
DWORD CMemoryWnd::GetStackAddr(UINT nID, Memory *pMemory, CPU *pCPU)
{
	CPU::cpu_t reg;
	DWORD dwAddr;
	DWORD dwValue;

	ASSERT((nID >= IDM_STACK_0) && (nID <= IDM_STACK_F));
	ASSERT(pMemory);
	ASSERT(pCPU);

	// �I�t�Z�b�g�Z�o
	nID -= IDM_STACK_0;
	ASSERT(nID <= 15);
	nID <<= 1;

	// CPU���W�X�^����A�h���X�A�������Z�o
	::LockVM();
	pCPU->GetCPU(&reg);
	dwAddr = reg.areg[7];
	dwAddr += nID;
	dwAddr &= 0xfffffe;

	// �f�[�^�擾
	dwValue = pMemory->ReadOnly(dwAddr + 1);
	dwAddr = (dwAddr + 2) & 0xfffffe;
	dwValue <<= 8;
	dwValue |= pMemory->ReadOnly(dwAddr);
	dwValue <<= 8;
	dwValue |= pMemory->ReadOnly(dwAddr + 1);
	::UnlockVM();

	return dwValue;
}

//===========================================================================
//
//	�u���[�N�|�C���g�E�B���h�E
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	�R���X�g���N�^
//
//---------------------------------------------------------------------------
CBreakPWnd::CBreakPWnd()
{
	// �E�B���h�E�p�����[�^��`
	m_dwID = MAKEID('B', 'R', 'K', 'P');
	::GetMsg(IDS_SWND_BREAKP, m_strCaption);
	m_nWidth = 43;
	m_nHeight = Scheduler::BreakMax + 1;

	// �X�P�W���[���擾
	m_pScheduler = (Scheduler*)::GetVM()->SearchDevice(MAKEID('S', 'C', 'H', 'E'));
	ASSERT(m_pScheduler);
}

//---------------------------------------------------------------------------
//
//	���b�Z�[�W �}�b�v
//
//---------------------------------------------------------------------------
BEGIN_MESSAGE_MAP(CBreakPWnd, CSubTextWnd)
	ON_WM_LBUTTONDBLCLK()
	ON_WM_CONTEXTMENU()
	ON_COMMAND(IDM_BREAKP_ENABLE, OnEnable)
	ON_COMMAND(IDM_BREAKP_CLEAR, OnClear)
	ON_COMMAND(IDM_BREAKP_DEL, OnDel)
	ON_COMMAND(IDM_BREAKP_ADDR, OnAddr)
	ON_COMMAND_RANGE(IDM_HISTORY_0, IDM_HISTORY_9, OnHistory)
	ON_COMMAND(IDM_BREAKP_ALL, OnAll)
END_MESSAGE_MAP()

//---------------------------------------------------------------------------
//
//	���{�^���_�u���N���b�N
//
//---------------------------------------------------------------------------
void CBreakPWnd::OnLButtonDblClk(UINT /*nFlags*/, CPoint point)
{
	int y;
	Scheduler::breakpoint_t buf;

	// y�擾�A�␳(-1)�A�`�F�b�N
	y = point.y / m_tmHeight;
	y--;
	if ((y < 0) || (y >= Scheduler::BreakMax)) {
		return;
	}

	// ���b�N�A�u���[�N�|�C���g�擾
	::LockVM();
	m_pScheduler->GetBreak(y, &buf);

	// �g�p���Ȃ甽�]
	if (buf.use) {
		m_pScheduler->EnableBreak(y, !(buf.enable));
	}

	// �A�����b�N
	::UnlockVM();
}

//---------------------------------------------------------------------------
//
//	�R���e�L�X�g���j���[
//
//---------------------------------------------------------------------------
void CBreakPWnd::OnContextMenu(CWnd *pWnd, CPoint point)
{
	CRect rect;
	CMenu menu;
	CMenu *pMenu;

	// �N���C�A���g�̈���ŉ����ꂽ�����肷��
	GetClientRect(&rect);
	ClientToScreen(&rect);
	if (!rect.PtInRect(point)) {
		CSubTextWnd::OnContextMenu(pWnd, point);
		return;
	}

	// �ʒu�L��
	m_Point = point;

	// ���j���[���s
	if (::IsJapanese()) {
		menu.LoadMenu(IDR_BREAKPMENU);
	}
	else {
		menu.LoadMenu(IDR_US_BREAKPMENU);
	}
	pMenu = menu.GetSubMenu(0);
	SetupContext(pMenu);
	pMenu->TrackPopupMenu(TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_RIGHTBUTTON,
							point.x, point.y, this);
}

//---------------------------------------------------------------------------
//
//	�R���e�L�X�g���j���[�Z�b�g�A�b�v
//
//---------------------------------------------------------------------------
void FASTCALL CBreakPWnd::SetupContext(CMenu *pMenu)
{
	int y;
	CPoint point;
	Scheduler::breakpoint_t buf;
	int nCount;
	int nBreak;

	ASSERT(pMenu);

	// ������
	buf.enable = FALSE;
	buf.use = FALSE;

	// y�擾
	point = m_Point;
	ScreenToClient(&point);
	y = point.y / m_tmHeight;
	y--;

	// �u���[�N�|�C���g�̎g�p���擾�ƁA�J�����g�̎擾
	nCount = 0;
	::LockVM();
	for (nBreak=0; nBreak<Scheduler::BreakMax; nBreak++) {
		m_pScheduler->GetBreak(nBreak, &buf);
		if (buf.use) {
			nCount++;
		}
	}
	buf.use = FALSE;
	if ((y >= 0) && (y < Scheduler::BreakMax)) {
		m_pScheduler->GetBreak(y, &buf);
	}
	::UnlockVM();

	// �S�č폜
	if (nCount > 0) {
		pMenu->EnableMenuItem(IDM_BREAKP_ALL, MF_BYCOMMAND | MF_ENABLED);
	}
	else {
		pMenu->EnableMenuItem(IDM_BREAKP_ALL, MF_BYCOMMAND | MF_GRAYED);
	}

	// �A�h���X�q�X�g��
	CAddrDlg::SetupHisMenu(pMenu);

	// �J�����g�����g�p�Ȃ�A�ύX�n�͖���
	if (!buf.use) {
		pMenu->EnableMenuItem(IDM_BREAKP_ENABLE, MF_BYCOMMAND | MF_GRAYED);
		pMenu->EnableMenuItem(IDM_BREAKP_CLEAR, MF_BYCOMMAND | MF_GRAYED);
		pMenu->EnableMenuItem(IDM_BREAKP_DEL, MF_BYCOMMAND | MF_GRAYED);
		return;
	}

	// �g�p���Ȃ�AEnable�`�F�b�N
	if (buf.enable) {
		pMenu->CheckMenuItem(IDM_BREAKP_ENABLE, MF_BYCOMMAND | MF_CHECKED);
	}
	else {
		pMenu->CheckMenuItem(IDM_BREAKP_ENABLE, MF_BYCOMMAND | MF_UNCHECKED);
	}
}

//---------------------------------------------------------------------------
//
//	�L���E����
//
//---------------------------------------------------------------------------
void CBreakPWnd::OnEnable()
{
	int y;
	CPoint point;
	Scheduler::breakpoint_t buf;

	// y�擾
	point = m_Point;
	ScreenToClient(&point);
	y = point.y / m_tmHeight;
	y--;
	ASSERT((y >= 0) && (y < Scheduler::BreakMax));

	// �u���[�N�|�C���g���]
	::LockVM();
	m_pScheduler->GetBreak(y, &buf);
	ASSERT(buf.use);
	m_pScheduler->EnableBreak(y, !(buf.enable));
	::UnlockVM();
}

//---------------------------------------------------------------------------
//
//	�񐔃N���A
//
//---------------------------------------------------------------------------
void CBreakPWnd::OnClear()
{
	int y;
	CPoint point;

	// y�擾
	point = m_Point;
	ScreenToClient(&point);
	y = point.y / m_tmHeight;
	y--;
	ASSERT((y >= 0) && (y < Scheduler::BreakMax));

	// �񐔃N���A
	::LockVM();
	m_pScheduler->ClearBreak(y);
	::UnlockVM();
}

//---------------------------------------------------------------------------
//
//	�u���[�N�|�C���g�폜
//
//---------------------------------------------------------------------------
void CBreakPWnd::OnDel()
{
	int y;
	CPoint point;
	Scheduler::breakpoint_t buf;

	// y�擾
	point = m_Point;
	ScreenToClient(&point);
	y = point.y / m_tmHeight;
	y--;
	ASSERT((y >= 0) && (y < Scheduler::BreakMax));

	// �񐔃N���A
	::LockVM();
	m_pScheduler->GetBreak(y, &buf);
	ASSERT(buf.use);
	m_pScheduler->DelBreak(buf.addr);
	::UnlockVM();
}

//---------------------------------------------------------------------------
//
//	�A�h���X�w��
//
//---------------------------------------------------------------------------
void CBreakPWnd::OnAddr()
{
	int y;
	CPoint point;
	Scheduler::breakpoint_t buf;
	CPU *pCPU;
	CPU::cpu_t reg;
	DWORD dwAddr;
	CAddrDlg dlg(this);

	// y�擾
	point = m_Point;
	ScreenToClient(&point);
	y = point.y / m_tmHeight;
	y--;

	::LockVM();
	// �L���ȃu���[�N�|�C���g���w���Ă���΁A���̃A�h���X
	dwAddr = 0xffffffff;
	if ((y >= 0) && (y < Scheduler::BreakMax)) {
		m_pScheduler->GetBreak(y, &buf);
		if (buf.use) {
			dwAddr = buf.addr & 0xffffff;
		}
	}
	// �����łȂ���΁APC
	if (dwAddr == 0xffffffff) {
		pCPU = (CPU*)::GetVM()->SearchDevice(MAKEID('C', 'P', 'U', ' '));
		ASSERT(pCPU);
		pCPU->GetCPU(&reg);
		dwAddr = reg.pc & 0xffffff;
	}
	::UnlockVM();
	ASSERT(dwAddr <= 0xffffff);

	// ���̓_�C�A���O
	dlg.m_dwValue = dwAddr;
	if (dlg.DoModal() != IDOK) {
		return;
	}

	// ���ʃ��[�`���ɂ܂�����
	SetAddr(dlg.m_dwValue);
}

//---------------------------------------------------------------------------
//
//	�A�h���X�q�X�g��
//
//---------------------------------------------------------------------------
void CBreakPWnd::OnHistory(UINT nID)
{
	DWORD dwAddr;

	ASSERT((nID >= IDM_HISTORY_0) && (nID <= IDM_HISTORY_9));

	// ���ʃ��[�`���ɂ܂�����
	dwAddr = CAddrDlg::GetAddr(nID);
	SetAddr(dwAddr);
}

//---------------------------------------------------------------------------
//
//	�S�č폜
//
//---------------------------------------------------------------------------
void CBreakPWnd::OnAll()
{
	Scheduler::breakpoint_t buf;
	int i;

	// �S�ăN���A
	::LockVM();
	for (i=0; i<Scheduler::BreakMax; i++) {
		m_pScheduler->GetBreak(i, &buf);
		if (buf.use) {
			m_pScheduler->DelBreak(buf.addr);
		}
	}
	::UnlockVM();
}

//---------------------------------------------------------------------------
//
//	�A�h���X�ݒ�
//
//---------------------------------------------------------------------------
void FASTCALL CBreakPWnd::SetAddr(DWORD dwAddr)
{
	int y;
	CPoint point;
	Scheduler::breakpoint_t buf;

	ASSERT(dwAddr <= 0xffffff);

	// ���ɓo�^����Ă���A�h���X�Ȃ�A����
	::LockVM();
	for (y=0; y<Scheduler::BreakMax; y++) {
		m_pScheduler->GetBreak(y, &buf);
		if (buf.use) {
			if (buf.addr == dwAddr) {
				::UnlockVM();
				return;
			}
		}
	}
	::UnlockVM();

	// y�擾
	point = m_Point;
	ScreenToClient(&point);
	y = point.y / m_tmHeight;
	y--;

	// �g�p���̃u���[�N�|�C���g���w���Ă���΁A�����������ւ�
	::LockVM();
	if ((y >= 0) && (y < Scheduler::BreakMax)) {
		m_pScheduler->GetBreak(y, &buf);
		if (buf.use) {
			m_pScheduler->AddrBreak(y, dwAddr);
			::UnlockVM();
			return;
		}
	}

	// �����łȂ���΁A�V�K�ݒ�
	m_pScheduler->SetBreak(dwAddr);
	::UnlockVM();
}

//---------------------------------------------------------------------------
//
//	�Z�b�g�A�b�v
//
//---------------------------------------------------------------------------
void FASTCALL CBreakPWnd::Setup()
{
	int i;
	CString strText;
	CString strFmt;
	Scheduler::breakpoint_t buf;

	// �N���A
	Clear();

	// �K�C�h�\��
	SetString(0, 0, _T("No."));
	SetString(5, 0, _T("Address"));
	SetString(14, 0, _T("Flag"));
	SetString(28, 0, _T("Time"));
	SetString(38, 0, _T("Count"));

	// ���[�v
	for (i=0; i<Scheduler::BreakMax; i++) {
		// �ԍ�
		strText.Format(_T("%2d "), i + 1);

		// �擾�A�L���`�F�b�N
		m_pScheduler->GetBreak(i, &buf);
		if (buf.use) {
			// �A�h���X
			strFmt.Format(_T("  $%06X "), buf.addr);
			strText += strFmt;

			// �t���O
			if (buf.enable) {
				strText += _T(" Enable");
			}
			else {
				strText += _T("Disable");
			}

			// ����
			if (buf.count > 0) {
				strFmt.Format(_T(" %7d.%05dms"), (buf.time / 2000), (buf.time % 2000) * 5);
				strText += strFmt;

				// �J�E���g
				strFmt.Format(_T("   %4d"), buf.count);
				strText += strFmt;
			}
		}

		// ������Z�b�g
		SetString(0, i + 1, strText);
	}
}

#endif	// _WIN32
