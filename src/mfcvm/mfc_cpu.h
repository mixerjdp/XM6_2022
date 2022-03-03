//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2005 �o�h�D(ytanaka@ipc-tokai.or.jp)
//	[ MFC �T�u�E�B���h�E(CPU) ]
//
//---------------------------------------------------------------------------

#if defined(_WIN32)

#if !defined(mfc_cpu_h)
#define mfc_cpu_h

#include "mfc_sub.h"

//===========================================================================
//
//	�q�X�g���t���_�C�A���O
//
//===========================================================================
class CHistoryDlg : public CDialog
{
public:
	CHistoryDlg(UINT nID, CWnd *pParentWnd);
										// �R���X�g���N�^
	BOOL OnInitDialog();
										// �_�C�A���O������
	void OnOK();
										// OK
	DWORD m_dwValue;
										// �G�f�B�b�g�l

protected:
	virtual UINT* GetNumPtr() = 0;
										// �q�X�g�����|�C���^�擾
	virtual DWORD* GetDataPtr() = 0;
										// �q�X�g���f�[�^�|�C���^�擾
	UINT m_nBit;
										// �L���r�b�g
	DWORD m_dwMask;
										// �}�X�N(�r�b�g�����������)
};

//===========================================================================
//
//	�A�h���X���̓_�C�A���O
//
//===========================================================================
class CAddrDlg : public CHistoryDlg
{
public:
	CAddrDlg(CWnd *pParent = NULL);
										// �R���X�g���N�^
	static void SetupHisMenu(CMenu *pMenu);
										// ���j���[�Z�b�g�A�b�v
	static DWORD GetAddr(UINT nID);
										// ���j���[���ʎ擾

protected:
	UINT* GetNumPtr();
										// �q�X�g�����|�C���^�擾
	DWORD* GetDataPtr();
										// �q�X�g���f�[�^�|�C���^�擾
	static UINT m_Num;
										// �q�X�g����
	static DWORD m_Data[10];
										// �q�X�g���f�[�^
};

//===========================================================================
//
//	���W�X�^���̓_�C�A���O
//
//===========================================================================
class CRegDlg : public CHistoryDlg
{
public:
	CRegDlg(CWnd *pParent = NULL);
										// �R���X�g���N�^
	BOOL OnInitDialog();
										// �_�C�A���O������
	void OnOK();
										// OK
	UINT m_nIndex;
										// ���W�X�^�C���f�b�N�X

protected:
	UINT* GetNumPtr();
										// �q�X�g�����|�C���^�擾
	DWORD* GetDataPtr();
										// �q�X�g���f�[�^�|�C���^�擾
	static UINT m_Num;
										// �q�X�g����
	static DWORD m_Data[10];
										// �q�X�g���f�[�^
};

//===========================================================================
//
//	�f�[�^���̓_�C�A���O
//
//===========================================================================
class CDataDlg : public CHistoryDlg
{
public:
	CDataDlg(CWnd *pParent = NULL);
										// �R���X�g���N�^
	BOOL OnInitDialog();
										// �_�C�A���O������
	UINT m_nSize;
										// �T�C�Y
	DWORD m_dwAddr;
										// �A�h���X

protected:
	UINT* GetNumPtr();
										// �q�X�g�����|�C���^�擾
	DWORD* GetDataPtr();
										// �q�X�g���f�[�^�|�C���^�擾
	static UINT m_Num;
										// �q�X�g����
	static DWORD m_Data[10];
										// �q�X�g���f�[�^
};

//===========================================================================
//
//	MPU���W�X�^�E�B���h�E
//
//===========================================================================
class CCPURegWnd : public CSubTextWnd
{
public:
	CCPURegWnd();
										// �R���X�g���N�^
	void FASTCALL Setup();
										// �Z�b�g�A�b�v
	static void SetupRegMenu(CMenu *pMenu, CPU *pCPU, BOOL bSR);
										// ���j���[�Z�b�g�A�b�v
	static DWORD GetRegValue(CPU *pCPU, UINT uID);
										// ���W�X�^�l�擾

protected:
	afx_msg void OnContextMenu(CWnd *pWnd, CPoint point);
										// �R���e�L�X�g���j���[
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
										// ���_�u���N���b�N
	afx_msg void OnReg(UINT nID);
										// ���W�X�^�I��

private:
	CPU *m_pCPU;
										// CPU

	DECLARE_MESSAGE_MAP()
										// ���b�Z�[�W �}�b�v����
};

//===========================================================================
//
//	���荞�݃E�B���h�E
//
//===========================================================================
class CIntWnd : public CSubTextWnd
{
public:
	CIntWnd();
										// �R���X�g���N�^
	void FASTCALL Setup();
										// �Z�b�g�A�b�v

protected:
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
										// ���_�u���N���b�N
	afx_msg void OnContextMenu(CWnd *pWnd, CPoint point);
										// �R���e�L�X�g���j���[

private:
	CPU* m_pCPU;
										// CPU

	DECLARE_MESSAGE_MAP()
										// ���b�Z�[�W �}�b�v����
};

//===========================================================================
//
//	�t�A�Z���u���E�B���h�E
//
//===========================================================================
class CDisasmWnd : public CSubTextScrlWnd
{
public:
	CDisasmWnd(int index);
										// �R���X�g���N�^
	void FASTCALL Setup();
										// �Z�b�g�A�b�v
	void FASTCALL SetAddr(DWORD dwAddr);
										// �A�h���X�w��
	void FASTCALL SetPC(DWORD pc);
										// PC�w��
	void FASTCALL Update();
										// ���b�Z�[�W�X���b�h����̍X�V
	static void FASTCALL SetupBreakMenu(CMenu *pMenu, Scheduler *pScheduler);
										// �u���[�N�|�C���g���j���[�ݒ�
	static DWORD FASTCALL GetBreak(UINT nID, Scheduler *pScheduler);
										// �u���[�N�|�C���g�擾

protected:
	afx_msg int OnCreate(LPCREATESTRUCT lpcs);
										// �E�B���h�E�쐬
	afx_msg void OnDestroy();
										// �E�B���h�E�폜
	afx_msg void OnSize(UINT nType, int cx, int cy);
										// �T�C�Y�ύX
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
										// ���N���b�N
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
										// ���_�u���N���b�N
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar *pBar);
										// �����X�N���[��
	afx_msg void OnContextMenu(CWnd *pWnd, CPoint point);
										// �R���e�L�X�g���j���[
	afx_msg void OnNewWin();
										// �V�����E�B���h�E
	afx_msg void OnPC();
										// PC�ֈړ�
	afx_msg void OnSync();
										// PC�ɓ���
	afx_msg void OnAddr();
										// �A�h���X����
	afx_msg void OnReg(UINT nID);
										// ���W�X�^
	afx_msg void OnStack(UINT nID);
										// �X�^�b�N
	afx_msg void OnBreak(UINT nID);
										// �u���[�N�|�C���g
	afx_msg void OnHistory(UINT nID);
										// �A�h���X�q�X�g��
	afx_msg void OnCPUExcept(UINT nID);
										// CPU��O�x�N�^
	afx_msg void OnTrap(UINT nID);
										// trap�x�N�^
	afx_msg void OnMFP(UINT nID);
										// MFP�x�N�^
	afx_msg void OnSCC(UINT nID);
										// SCC�x�N�^
	afx_msg void OnDMAC(UINT uID);
										// DMAC�x�N�^
	afx_msg void OnIOSC(UINT uID);
										// IOSC�x�N�^

private:
	DWORD FASTCALL GetPrevAddr(DWORD dwAddr);
										// ��O�̃A�h���X���擾
	void FASTCALL SetupContext(CMenu *pMenu);
										// �R���e�L�X�g���j���[�Z�b�g�A�b�v
	void FASTCALL SetupVector(CMenu *pMenu, UINT index, DWORD vector, int num);
										// ���荞�݃x�N�^�Z�b�g�A�b�v
	void FASTCALL SetupAddress(CMenu *pMenu, UINT index, DWORD addr);
										// �A�h���X�Z�b�g�A�b�v
	void FASTCALL OnVector(UINT vector);
										// �x�N�^�w��
	CPU *m_pCPU;
										// CPU
	Scheduler *m_pScheduler;
										// �X�P�W���[��
	MFP *m_pMFP;
										// MFP
	Memory *m_pMemory;
										// ������
	SCC * m_pSCC;
										// SCC
	DMAC *m_pDMAC;
										// DMAC
	IOSC *m_pIOSC;
										// IOSC
	BOOL m_bSync;
										// PC�����t���O
	DWORD m_dwPC;
										// PC
	DWORD m_dwAddr;
										// �\���J�n�A�h���X
	DWORD m_dwSetAddr;
										// �Z�b�g���ꂽ�A�h���X
	DWORD *m_pAddrBuf;
										// �A�h���X�o�b�t�@
	CString m_Caption;
										// �L���v�V����������
	CString m_CaptionSet;
										// �L���v�V�����ݒ蕶����

	DECLARE_MESSAGE_MAP()
										// ���b�Z�[�W �}�b�v����
};

//===========================================================================
//
//	�������E�B���h�E
//
//===========================================================================
class CMemoryWnd : public CSubTextScrlWnd
{
public:
	CMemoryWnd(int nWnd);
										// �R���X�g���N�^
	void FASTCALL Setup();
										// �Z�b�g�A�b�v
	void FASTCALL SetAddr(DWORD dwAddr);
										// �A�h���X�w��
	void FASTCALL SetUnit(int nUnit);
										// �\���P�ʎw��
	void FASTCALL Update();
										// ���b�Z�[�W�X���b�h����̍X�V
	static void SetupStackMenu(CMenu *pMenu, Memory *pMemory, CPU *pCPU);
										// �X�^�b�N���j���[�Z�b�g�A�b�v
	static DWORD GetStackAddr(UINT nID, Memory *pMemory, CPU *pCPU);
										// �X�^�b�N�擾

protected:
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
										// �E�B���h�E�쐬
	afx_msg void OnPaint();
										// �`��
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
										// ���_�u���N���b�N
	afx_msg void OnContextMenu(CWnd *pWnd, CPoint point);
										// �R���e�L�X�g���j���[
	afx_msg void OnAddr();
										// �A�h���X����
	afx_msg void OnNewWin();
										// �V�����E�B���h�E
	afx_msg void OnUnit(UINT uID);
										// �\���P�ʎw��
	afx_msg void OnRange(UINT uID);
										// �A�h���X�͈͎w��
	afx_msg void OnReg(UINT uID);
										// ���W�X�^�l���w��
	afx_msg void OnArea(UINT uID);
										// �G���A�w��
	afx_msg void OnHistory(UINT uID);
										// �A�h���X�q�X�g��
	afx_msg void OnStack(UINT uID);
										// �X�^�b�N
	void FASTCALL SetupScrlV();
										// �X�N���[������(����)

private:
	void FASTCALL SetupContext(CMenu *pMenu);
										// �R���e�L�X�g���j���[ �Z�b�g�A�b�v
	Memory *m_pMemory;
										// ������
	CPU *m_pCPU;
										// CPU
	DWORD m_dwAddr;
										// �\���J�n�A�h���X
	CString m_strCaptionReq;
										// �L���v�V����������(�v��)
	CString m_strCaptionSet;
										// �L���v�V����������(�ݒ�)
	CCriticalSection m_CSection;
										// �N���e�B�J���Z�N�V����
	UINT m_nUnit;
										// �\���T�C�Y0/1/2=Byte/Word/Long

	DECLARE_MESSAGE_MAP()
										// ���b�Z�[�W �}�b�v����
};

//===========================================================================
//
//	�u���[�N�|�C���g�E�B���h�E
//
//===========================================================================
class CBreakPWnd : public CSubTextWnd
{
public:
	CBreakPWnd();
										// �R���X�g���N�^
	void FASTCALL Setup();
										// �Z�b�g�A�b�v

protected:
	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
										// ���_�u���N���b�N
	afx_msg void OnContextMenu(CWnd *pWnd, CPoint point);
										// �R���e�L�X�g���j���[
	afx_msg void OnEnable();
										// �L���E����
	afx_msg void OnClear();
										// �񐔃N���A
	afx_msg void OnDel();
										// �폜
	afx_msg void OnAddr();
										// �A�h���X�w��
	afx_msg void OnAll();
										// �S�č폜
	afx_msg void OnHistory(UINT nID);
										// �A�h���X�q�X�g��

private:
	void FASTCALL SetupContext(CMenu *pMenu);
										// �R���e�L�X�g���j���[ �Z�b�g�A�b�v
	void FASTCALL SetAddr(DWORD dwAddr);
										// �A�h���X�ݒ�
	Scheduler* m_pScheduler;
										// �X�P�W���[��
	CPoint m_Point;
										// �R���e�L�X�g���j���[�|�C���g

	DECLARE_MESSAGE_MAP()
										// ���b�Z�[�W �}�b�v����
};

#endif	// mfc_cpu_h
#endif	// _WIN32
