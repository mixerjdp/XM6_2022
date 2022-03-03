//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2004 �o�h�D(ytanaka@ipc-tokai.or.jp)
//	[ MFC �⏕�c�[�� ]
//
//---------------------------------------------------------------------------

#if defined(_WIN32)

#if !defined (mfc_tool_h)
#define mfc_tool_h

//===========================================================================
//
//	�t���b�s�[�f�B�X�N�C���[�W�쐬�_�C�A���O
//
//===========================================================================
class CFDIDlg : public CDialog
{
public:
	CFDIDlg(CWnd *pParent);
										// �R���X�g���N�^
	BOOL OnInitDialog();
										// �_�C�A���O������
	void OnOK();
										// �_�C�A���OOK
	void OnCancel();
										// �_�C�A���O�L�����Z��

	TCHAR m_szFileName[_MAX_PATH];
										// �t�@�C����
	TCHAR m_szDiskName[60];
										// �f�B�X�N��
	DWORD m_dwType;
										// �t�@�C���`��(2HD,DIM,D68)
	DWORD m_dwPhysical;
										// �����t�H�[�}�b�g
	BOOL m_bLogical;
										// �_���t�H�[�}�b�g
	int m_nDrive;
										// �}�E���g�h���C�u(-1�Ń}�E���g���Ȃ�)

protected:
	afx_msg void OnBrowse();
										// �t�@�C���I��
	afx_msg void OnPhysical();
										// �����t�H�[�}�b�g�N���b�N

private:
	void FASTCALL MaskName();
										// �f�B�X�N���̃}�X�N
	void FASTCALL SetPhysical();
										// �����t�H�[�}�b�g�ݒ�
	void FASTCALL GetPhysical();
										// �����t�H�[�}�b�g�擾
	void FASTCALL MaskPhysical();
										// �����t�H�[�}�b�g�}�X�N
	void FASTCALL SetLogical();
										// �_���t�H�[�}�b�g�ݒ�
	void FASTCALL GetLogical();
										// �_���t�H�[�}�b�g�擾
	void FASTCALL MaskLogical();
										// �_���t�H�[�}�b�g�}�X�N
	static const DWORD IDTable[16];
										// �����t�H�[�}�b�gID�e�[�u��
	BOOL FASTCALL GetFile();
	 									// �t�@�C�����擾

	DECLARE_MESSAGE_MAP()
										// ���b�Z�[�W �}�b�v����
};

//===========================================================================
//
//	��e�ʃf�B�X�N�C���[�W�쐬�_�C�A���O
//
//===========================================================================
class CDiskDlg : public CDialog
{
public:
	CDiskDlg(CWnd *pParent);
										// �R���X�g���N�^
	BOOL OnInitDialog();
										// �_�C�A���O������
	void OnOK();
										// �_�C�A���OOK
	BOOL FASTCALL IsSucceeded() const	{ return m_bSucceed; }
										// �쐬�ɐ���������
	BOOL FASTCALL IsCanceled() const	{ return m_bCancel; }
										// �L�����Z��������
	LPCTSTR FASTCALL GetPath() const	{ return m_szPath; }
										// �p�X���擾
	int m_nType;
										// ���(0:SASI-HD 1:SCSI-HD 2:SCSI-MO)
	BOOL m_bMount;
										// �}�E���g�t���O(SCSI-MO�̂�)

protected:
	afx_msg void OnBrowse();
										// �t�@�C���I��
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pBar);
										// �c�X�N���[��
	afx_msg void OnMOSize();
										// MO�T�C�Y�ύX

private:
	void FASTCALL CtrlSASI();
										// SASI-HD �R���g���[��������
	void FASTCALL CtrlSCSI();
										// SCSI-HD �R���g���[��������
	void FASTCALL CtrlMO();
										// SCSI-MO �R���g���[��������
	BOOL FASTCALL GetFile();
										// �t�@�C���I��
	void FASTCALL CreateSASI();
										// SASI-HD �f�B�X�N�쐬
	void FASTCALL CreateSCSI();
										// SCSI-HD �f�B�X�N�쐬
	void FASTCALL CreateMO();
										// SCSI-MO �f�B�X�N�쐬

	TCHAR m_szPath[_MAX_PATH];
										// �p�X��
	BOOL m_bSucceed;
										// �쐬�ɐ���������
	BOOL m_bCancel;
										// �L�����Z��������
	UINT m_nSize;
										// �f�B�X�N�T�C�Y(MB)
	UINT m_nFormat;
										// �_���t�H�[�}�b�g
	static const UINT SASITable[];
										// SASI-HD ID�e�[�u��
	static const UINT SCSITable[];
										// SCSI-HD ID�e�[�u��
	static const UINT MOTable[];
										// SCSI-MO ID�e�[�u��

	DECLARE_MESSAGE_MAP()
										// ���b�Z�[�W �}�b�v����
};

//===========================================================================
//
//	�f�B�X�N�C���[�W�쐬�_�C�A���O
//
//===========================================================================
class CDiskMakeDlg : public CDialog
{
public:
	// ��{�t�@���N�V����
	CDiskMakeDlg(CWnd *pParent);
										// �R���X�g���N�^
	BOOL OnInitDialog();
										// �_�C�A���O������
	void OnOK();
										// �_�C�A���OOK
	void OnCancel();
										// �_�C�A���O�L�����Z��

	// API
	BOOL FASTCALL IsSucceeded() const	{ return m_bSucceed; }
										// �쐬�ɐ���������
	BOOL FASTCALL IsCanceled() const	{ return m_bCancel; }
										// �L�����Z��������
	static UINT ThreadFunc(LPVOID lpParam);
										// �X���b�h�֐�
	void FASTCALL Run();
										// �X���b�h���C��

	// �p�����[�^
	DWORD m_dwSize;
										// �f�B�X�N�T�C�Y
	TCHAR m_szPath[_MAX_PATH];
										// �p�X��
	int m_nFormat;
										// �_���t�H�[�}�b�g���

protected:
#if _MFC_VER >= 0x700
	afx_msg void OnTimer(UINT_PTR nTimerID);
#else
	afx_msg void OnTimer(UINT nTimerID);
#endif
										// �^�C�}
	afx_msg void OnDestroy();
										// �_�C�A���O�폜
	virtual BOOL FASTCALL Format();
										// �_���t�H�[�}�b�g
	DWORD m_dwCurrent;
										// �쐬�Ǘ�

private:
	CWinThread *m_pThread;
										// �쐬�X���b�h
	BOOL m_bThread;
										// �X���b�h�I���t���O
	CCriticalSection m_CSection;
										// �N���e�B�J���Z�N�V����
	BYTE *m_pBuffer;
										// �o�b�t�@
#if _MFC_VER >= 0x700
	UINT_PTR m_nTimerID;
#else
	UINT m_nTimerID;
#endif
										// �^�C�}ID
	DWORD m_dwParcent;
										// �i�s�p�[�Z���g
	BOOL m_bSucceed;
										// �����t���O
	BOOL m_bCancel;
										// �L�����Z���t���O

	DECLARE_MESSAGE_MAP()
										// ���b�Z�[�W �}�b�v����
};

//===========================================================================
//
//	SASI�f�B�X�N�C���[�W�쐬�_�C�A���O
//
//===========================================================================
class CSASIMakeDlg : public CDiskMakeDlg
{
public:
	CSASIMakeDlg(CWnd *pParent);
										// �R���X�g���N�^

protected:
	BOOL FASTCALL Format();
										// �t�H�[�}�b�g

private:
	static const BYTE MENU[];
										// ���j���[
	static const BYTE IPL[];
										// IPL
};

//===========================================================================
//
//	SCSI�f�B�X�N�C���[�W�쐬�_�C�A���O
//
//===========================================================================
class CSCSIMakeDlg : public CDiskMakeDlg
{
public:
	CSCSIMakeDlg(CWnd *pParent);
										// �R���X�g���N�^
};

//===========================================================================
//
//	MO�f�B�X�N�C���[�W�쐬�_�C�A���O
//
//===========================================================================
class CMOMakeDlg : public CDiskMakeDlg
{
public:
	CMOMakeDlg(CWnd *pParent);
										// �R���X�g���N�^

protected:
	BOOL FASTCALL Format();
										// �t�H�[�}�b�g

private:
	int m_nMedia;
										// ���f�B�A�^�C�v(0:128MB...3:640MB)
	BOOL FASTCALL FormatIBM();
										// IBM�t�H�[�}�b�g
	void FASTCALL MakeBPB(BYTE *pBPB);
										// IBM�t�H�[�}�b�gBPB�쐬
	void FASTCALL MakeSerial(BYTE *pSerial);
										// �{�����[���V���A���쐬
	BOOL FASTCALL FormatSHARP();
										// SHARP�t�H�[�}�b�g
	static const BYTE PartTable128[];
										// �p�[�e�B�V�����e�[�u��(128MB)
	static const BYTE PartTable230[];
										// �p�[�e�B�V�����e�[�u��(230MB)
	static const BYTE PartTable540[];
										// �p�[�e�B�V�����e�[�u��(540MB)
	static const BYTE PartTop128[];
										// ��1�p�[�e�B�V����(128MB)
	static const BYTE PartTop230[];
										// ��1�p�[�e�B�V����(230MB)
	static const BYTE PartTop540[];
										// ��1�p�[�e�B�V����(540MB)
	static const BYTE SCSIMENU[];
										// SCSI�p�[�e�B�V�����I�����j���[
	static const BYTE SCHDISK[];
										// SCSI�f�B�X�N�h���C�o
	static const BYTE SCSIIOCS[];
										// SCSIIOCS�⏕�h���C�o
	static const BYTE IPL[];
										// Human68k IPL
};

//===========================================================================
//
//	trap #0�_�C�A���O
//
//===========================================================================
class CTrapDlg : public CDialog
{
public:
	CTrapDlg(CWnd *pParent);
										// �R���X�g���N�^
	BOOL OnInitDialog();
										// �_�C�A���O������
	void OnOK();
										// �_�C�A���OOK
	DWORD m_dwCode;
										// �R�[�h�l

protected:
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar *pBar);
										// �c�X�N���[��

	DECLARE_MESSAGE_MAP()
										// ���b�Z�[�W �}�b�v����
};

#endif	// mfc_tool_h
#endif	// _WIN32
