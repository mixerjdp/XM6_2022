//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 �o�h�D(ytanaka@ipc-tokai.or.jp)
//	[ MFC �R���t�B�O ]
//
//---------------------------------------------------------------------------

#if defined(_WIN32)

#if !defined(mfc_cfg_h)
#define mfc_cfg_h

#include "config.h"
#include "ppi.h"

//===========================================================================
//
//	�R���t�B�O
//
//===========================================================================
class CConfig : public CComponent
{
public:
	// ��{�t�@���N�V����
	CConfig(CFrmWnd *pWnd);
										// �R���X�g���N�^
	BOOL FASTCALL Init();
										// ������
	void FASTCALL Cleanup();
										// �N���[���A�b�v

	// �ݒ�f�[�^(�S��)
	void FASTCALL GetConfig(Config *pConfigBuf) const;
										// �ݒ�f�[�^�擾
	void FASTCALL SetConfig(Config *pConfigBuf);
										// �ݒ�f�[�^�ݒ�

	// �ݒ�f�[�^(��)
	void FASTCALL SetStretch(BOOL bStretch);
										// ��ʊg��ݒ�
	void FASTCALL SetMIDIDevice(int nDevice, BOOL bIn);
										// MIDI�f�o�C�X�ݒ�

	// MRU
	void FASTCALL SetMRUFile(int nType, LPCTSTR pszFile);
										// MRU�t�@�C���ݒ�(�ł��V����)
	void FASTCALL GetMRUFile(int nType, int nIndex, LPTSTR pszFile) const;
										// MRU�t�@�C���擾
	int FASTCALL GetMRUNum(int nType) const;
										// MRU�t�@�C�����擾

	// �Z�[�u�E���[�h
	BOOL FASTCALL Save(Fileio *pFio, int nVer);
										// �Z�[�u
	BOOL FASTCALL Load(Fileio *pFio, int nVer);
										// ���[�h
	BOOL FASTCALL IsApply();
										// �K�p���邩

private:
	// �ݒ�f�[�^
	typedef struct _INIKEY {
		void *pBuf;						// �|�C���^
		LPCTSTR pszSection;				// �Z�N�V������
		LPCTSTR pszKey;					// �L�[��
		int nType;						// �^
		int nDef;						// �f�t�H���g�l
		int nMin;						// �ŏ��l(�ꕔ�^�C�v�̂�)
		int nMax;						// �ő�l(�ꕔ�̂�)
	} INIKEY, *PINIKEY;

	// INI�t�@�C��
	TCHAR m_IniFile[FILEPATH_MAX];
										// INI�t�@�C����

	// �ݒ�f�[�^
	void FASTCALL LoadConfig();
										// �ݒ�f�[�^���[�h
	void FASTCALL SaveConfig() const;
										// �ݒ�f�[�^�Z�[�u
	static const INIKEY IniTable[];
										// �ݒ�f�[�^INI�e�[�u��
	static Config m_Config;
										// �ݒ�f�[�^

	// �o�[�W�����݊�
	void FASTCALL ResetSASI();
										// SASI�Đݒ�
	void FASTCALL ResetCDROM();
										// CD-ROM�Đݒ�
	static BOOL m_bCDROM;
										// CD-ROM�L��

	// MRU
	enum {
		MruTypes = 5					// MRU�^�C�v��
	};
	void FASTCALL ClearMRU(int nType);
										// MRU�N���A
	void FASTCALL LoadMRU(int nType);
										// MRU���[�h
	void FASTCALL SaveMRU(int nType) const;
										// MRU�Z�[�u
	TCHAR m_MRUFile[MruTypes][9][FILEPATH_MAX];
										// MRU�t�@�C��
	int m_MRUNum[MruTypes];
										// MRU��

	// �L�[
	void FASTCALL LoadKey() const;
										// �L�[���[�h
	void FASTCALL SaveKey() const;
										// �L�[�Z�[�u

	// TrueKey
	void FASTCALL LoadTKey() const;
										// TrueKey���[�h
	void FASTCALL SaveTKey() const;
										// TrueKey�Z�[�u

	// �o�[�W�����݊�
	BOOL FASTCALL Load200(Fileio *pFio);
										// version2.00 or version2.01
	BOOL FASTCALL Load202(Fileio *pFio);
										// version2.02 or version2.03

	// ���[�h�E�Z�[�u
	BOOL m_bApply;
										// ���[�h��ApplyCfg��v��
};

//---------------------------------------------------------------------------
//
//	�N���X��s��`
//
//---------------------------------------------------------------------------
class CConfigSheet;

//===========================================================================
//
//	�R���t�B�O�v���p�e�B�y�[�W
//
//===========================================================================
class CConfigPage : public CPropertyPage
{
public:
	CConfigPage();
										// �R���X�g���N�^
	void FASTCALL Init(CConfigSheet *pSheet);
										// ������
	virtual BOOL OnInitDialog();
										// �_�C�A���O������
	virtual BOOL OnSetActive();
										// �y�[�W�A�N�e�B�u
	DWORD FASTCALL GetID() const		{ return m_dwID; }
										// ID�擾

protected:
	afx_msg BOOL OnSetCursor(CWnd *pWnd, UINT nHitTest, UINT nMsg);
										// �}�E�X�J�[�\���ݒ�
	Config *m_pConfig;
										// �ݒ�f�[�^
	DWORD m_dwID;
										// �y�[�WID
	int m_nTemplate;
										// �e���v���[�gID
	UINT m_uHelpID;
										// �w���vID
	UINT m_uMsgID;
										// �w���v���b�Z�[�WID
	CConfigSheet *m_pSheet;
										// �v���p�e�B�V�[�g

	DECLARE_MESSAGE_MAP()
										// ���b�Z�[�W �}�b�v����
};

//===========================================================================
//
//	��{�y�[�W
//
//===========================================================================
class CBasicPage : public CConfigPage
{
public:
	CBasicPage();
										// �R���X�g���N�^
	BOOL OnInitDialog();
										// ������
	void OnOK();
										// ����

protected:
	afx_msg void OnMPUFull();
										// MPU�t���X�s�[�h

	DECLARE_MESSAGE_MAP()
										// ���b�Z�[�W �}�b�v����
};

//===========================================================================
//
//	�T�E���h�y�[�W
//
//===========================================================================
class CSoundPage : public CConfigPage
{
public:
	CSoundPage();
										// �R���X�g���N�^
	BOOL OnInitDialog();
										// ������
	void OnOK();
										// ����

protected:
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pBar);
										// �c�X�N���[��
	afx_msg void OnSelChange();
										// �R���{�{�b�N�X�ύX

private:
	void FASTCALL EnableControls(BOOL bEnable);
										// �R���g���[����ԕύX
	BOOL m_bEnableCtrl;
										// �R���g���[�����
	static const UINT ControlTable[];
										// �R���g���[���e�[�u��

	DECLARE_MESSAGE_MAP()
										// ���b�Z�[�W �}�b�v����
};

//===========================================================================
//
//	���ʃy�[�W
//
//===========================================================================
class CVolPage : public CConfigPage
{
public:
	CVolPage();
										// �R���X�g���N�^
	BOOL OnInitDialog();
										// ������
	void OnOK();
										// ����
	void OnCancel();
										// �L�����Z��

protected:
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar *pBar);
										// ���X�N���[��
	afx_msg void OnFMCheck();
										// FM�����`�F�b�N
	afx_msg void OnADPCMCheck();
										// ADPCM�����`�F�b�N
#if _MFC_VER >= 0x700
	afx_msg void OnTimer(UINT_PTR nTimerID);
#else
	afx_msg void OnTimer(UINT nTimerID);
#endif
										// �^�C�}

private:
	CSound *m_pSound;
										// �T�E���h
	OPMIF *m_pOPMIF;
										// OPM�C���^�t�F�[�X
	ADPCM *m_pADPCM;
										// ADPCM
	CMIDI *m_pMIDI;
										// MIDI
#if _MFC_VER >= 0x700
	UINT_PTR m_nTimerID;
#else
	UINT m_nTimerID;
#endif
										// �^�C�}ID
	int m_nMasterVol;
										// �}�X�^�[����
	int m_nMasterOrg;
										// �}�X�^�[���ʃI���W�i��
	int m_nMIDIVol;
										// MIDI����
	int m_nMIDIOrg;
										// MIDI���ʃI���W�i��

	DECLARE_MESSAGE_MAP()
										// ���b�Z�[�W �}�b�v����
};

//===========================================================================
//
//	�L�[�{�[�h�y�[�W
//
//===========================================================================
class CKbdPage : public CConfigPage
{
public:
	CKbdPage();
										// �R���X�g���N�^
	BOOL OnInitDialog();
										// ������
	void OnOK();
										// ����
	void OnCancel();
										// �L�����Z��

protected:
	afx_msg void OnEdit();
										// �ҏW
	afx_msg void OnDefault();
										// �f�t�H���g
	afx_msg void OnClick(NMHDR *pNMHDR, LRESULT *pResult);
										// �J�����N���b�N
	afx_msg void OnRClick(NMHDR *pNMHDR, LRESULT *pResult);
										// �J�����E�N���b�N
	afx_msg void OnConnect();
										// �ڑ�

private:
	void FASTCALL UpdateReport();
										// ���|�[�g�X�V
	void FASTCALL EnableControls(BOOL bEnable);
										// �R���g���[����ԕύX
	DWORD m_dwEdit[0x100];
										// �G�f�B�b�g
	DWORD m_dwBackup[0x100];
										// �o�b�N�A�b�v
	CInput *m_pInput;
										// CInput
	BOOL m_bEnableCtrl;
										// �R���g���[�����
	static const UINT ControlTable[];
										// �R���g���[���e�[�u��

	DECLARE_MESSAGE_MAP()
										// ���b�Z�[�W �}�b�v����
};

//===========================================================================
//
//	�L�[�{�[�h�}�b�v�ҏW�_�C�A���O
//
//===========================================================================
class CKbdMapDlg : public CDialog
{
public:
	CKbdMapDlg(CWnd *pParent, DWORD *pMap);
										// �R���X�g���N�^
	BOOL OnInitDialog();
										// ������
	void OnOK();
										// OK
	void OnCancel();
										// �L�����Z��

protected:
	afx_msg void OnPaint();
										// �_�C�A���O�`��
	afx_msg LONG OnKickIdle(UINT uParam, LONG lParam);
										// �A�C�h������
	afx_msg LONG OnApp(UINT uParam, LONG lParam);
										// ���[�U(���ʃE�B���h�E����̒ʒm)

private:
	void FASTCALL OnDraw(CDC *pDC);
										// �`��T�u
	CRect m_rectStat;
										// �X�e�[�^�X��`
	CString m_strStat;
										// �X�e�[�^�X���b�Z�[�W
	CString m_strGuide;
										// �K�C�h���b�Z�[�W
	CWnd *m_pDispWnd;
										// CKeyDispWnd
	CInput *m_pInput;
										// CInput
	DWORD *m_pEditMap;
										// �ҏW���̃}�b�v

	DECLARE_MESSAGE_MAP()
										// ���b�Z�[�W �}�b�v����
};

//===========================================================================
//
//	�L�[���̓_�C�A���O
//
//===========================================================================
class CKeyinDlg : public CDialog
{
public:
	CKeyinDlg(CWnd *pParent);
										// �R���X�g���N�^
	BOOL OnInitDialog();
										// ������
	void OnOK();
										// OK
	UINT m_nTarget;
										// �^�[�Q�b�g�L�[
	UINT m_nKey;
										// ���蓖�ăL�[

protected:
	afx_msg void OnPaint();
										// �`��
	afx_msg UINT OnGetDlgCode();
										// �_�C�A���O�R�[�h�擾
	afx_msg LONG OnKickIdle(UINT uParam, LONG lParam);
										// �A�C�h������
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
										// �E�N���b�N

private:
	void FASTCALL OnDraw(CDC *pDC);
										// �`��T�u
	CInput *m_pInput;
										// CInput
	BOOL m_bKey[0x100];
										// �L�[�L���p
	CRect m_GuideRect;
										// �K�C�h��`
	CString m_GuideString;
										// �K�C�h������
	CRect m_AssignRect;
										// �L�[���蓖�ċ�`
	CString m_AssignString;
										// �L�[���蓖�ĕ�����
	CRect m_KeyRect;
										// �L�[��`
	CString m_KeyString;
										// �L�[������

	DECLARE_MESSAGE_MAP()
										// ���b�Z�[�W �}�b�v����
};

//===========================================================================
//
//	�}�E�X�y�[�W
//
//===========================================================================
class CMousePage : public CConfigPage
{
public:
	CMousePage();
										// �R���X�g���N�^
	BOOL OnInitDialog();
										// ������
	void OnOK();
										// ����

protected:
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar *pBar);
										// ���X�N���[��
	afx_msg void OnPort();
										// �|�[�g�I��

private:
	void FASTCALL EnableControls(BOOL bEnable);
										// �R���g���[����ԕύX
	BOOL m_bEnableCtrl;
										// �R���g���[�����
	static const UINT ControlTable[];
										// �R���g���[���e�[�u��

	DECLARE_MESSAGE_MAP()
										// ���b�Z�[�W �}�b�v����
};

//===========================================================================
//
//	�W���C�X�e�B�b�N�y�[�W
//
//===========================================================================
class CJoyPage : public CConfigPage
{
public:
	CJoyPage();
										// �R���X�g���N�^
	BOOL OnInitDialog();
										// ������
	void OnOK();
										// ����
	void OnCancel();
										// �L�����Z��

protected:
	BOOL OnCommand(WPARAM wParam, LPARAM lParam);
										// �R�}���h�ʒm

private:
	void FASTCALL OnSelChg(CComboBox* pComboBox);
										// �R���{�{�b�N�X�ύX
	void FASTCALL OnDetail(UINT nButton);
										// �ڍ�
	void FASTCALL OnSetting(UINT nButton);
										// �ݒ�
	CButton* GetCorButton(UINT nComboBox);
										// �Ή��{�^���擾
	CComboBox* GetCorCombo(UINT nButton);
										// �Ή��R���{�{�b�N�X�擾
	CInput *m_pInput;
										// CInput
	static UINT ControlTable[];
										// �R���g���[���e�[�u��
};

//===========================================================================
//
//	�W���C�X�e�B�b�N�ڍ׃_�C�A���O
//
//===========================================================================
class CJoyDetDlg : public CDialog
{
public:
	CJoyDetDlg(CWnd *pParent);
										// �R���X�g���N�^
	BOOL OnInitDialog();
										// ������

	CString m_strDesc;
										// �f�o�C�X����
	int m_nPort;
										// �|�[�g�ԍ�(0 or 1)
	int m_nType;
										// �^�C�v(0�`12)
};

//===========================================================================
//
//	�{�^���ݒ�y�[�W
//
//===========================================================================
class CBtnSetPage : public CPropertyPage
{
public:
	CBtnSetPage();
										// �R���X�g���N�^
	void FASTCALL Init(CPropertySheet *pSheet);
										// �쐬
	BOOL OnInitDialog();
										// ������
	void OnOK();
										// ����
	void OnCancel();
										// �L�����Z��
	int m_nJoy;
										// �W���C�X�e�B�b�N�ԍ�(0 or 1)
	int m_nType[PPI::PortMax];
										// �W���C�X�e�B�b�N�^�C�v(0�`12)

protected:
	afx_msg void OnPaint();
										// �_�C�A���O�`��
#if _MFC_VER >= 0x700
	afx_msg void OnTimer(UINT_PTR nTimerID);
#else
	afx_msg void OnTimer(UINT nTimerID);
#endif
										// �^�C�}
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar *pBar);
										// �����X�N���[��
	BOOL OnCommand(WPARAM wParam, LPARAM lParam);
										// �R�}���h�ʒm

private:
	enum CtrlType {
		BtnLabel,						// ���x��(�{�^��n)
		BtnCombo,						// �R���{�{�b�N�X
		BtnRapid,						// �A�˃X���C�_
		BtnValue						// �A�˃��x��
	};
	void FASTCALL OnDraw(CDC *pDC, BOOL *pButton, BOOL bForce);
										// �`�惁�C��
	void FASTCALL OnSlider(int nButton);
										// �X���C�_�ύX
	void FASTCALL OnSelChg(int nButton);
										// �R���{�{�b�N�X�ύX
	void FASTCALL GetButtonDesc(const char *pszDesc, CString &strDesc);
										// �{�^���\���擾
	UINT FASTCALL GetControl(int nButton, CtrlType ctlType) const;
										// �R���g���[��ID�擾
	CPropertySheet *m_pSheet;
										// �e�V�[�g
	CInput *m_pInput;
										// CInput
	CRect m_rectLabel[12];
										// ���x���ʒu
	BOOL m_bButton[12];
										// �{�^�������L��
#if _MFC_VER >= 0x700
	UINT_PTR m_nTimerID;
#else
	UINT m_nTimerID;
#endif
										// �^�C�}ID
	static const UINT ControlTable[];
										// �R���g���[���e�[�u��
	static const int RapidTable[];
										// �A�˃e�[�u��

	DECLARE_MESSAGE_MAP()
										// ���b�Z�[�W �}�b�v����
};

//===========================================================================
//
//	�W���C�X�e�B�b�N�v���p�e�B�V�[�g
//
//===========================================================================
class CJoySheet : public CPropertySheet
{
public:
	CJoySheet(CWnd *pParent);
										// �R���X�g���N�^
	void FASTCALL SetParam(int nJoy, int nCombo, int nType[]);
										// �p�����[�^�ݒ�
	void FASTCALL InitSheet();
										// �V�[�g������
	int FASTCALL GetAxes() const;
										// �����擾
	int FASTCALL GetButtons() const;
										// �{�^�����擾

private:
	CBtnSetPage m_BtnSet;
										// �{�^���ݒ�y�[�W
	CInput *m_pInput;
										// CInput
	int m_nJoy;
										// �W���C�X�e�B�b�N�ԍ�(0 or 1)
	int m_nCombo;
										// �R���{�{�b�N�X�I��
	int m_nType[PPI::PortMax];
										// VM���^�C�v�I��
	DIDEVCAPS m_DevCaps;
										// �f�o�C�XCaps
};

//===========================================================================
//
//	SASI�y�[�W
//
//===========================================================================
class CSASIPage : public CConfigPage
{
public:
	CSASIPage();
										// �R���X�g���N�^
	BOOL OnInitDialog();
										// ������
	void OnOK();
										// ����
	BOOL OnSetActive();
										// �y�[�W�A�N�e�B�u
	int FASTCALL GetDrives(const Config *pConfig) const;
										// SASI�h���C�u���擾

protected:
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pBar);
										// �c�X�N���[��
	afx_msg void OnClick(NMHDR *pNMHDR, LRESULT *pResult);
										// �J�����N���b�N

private:
	void FASTCALL UpdateList();
										// ���X�g�R���g���[���X�V
	void FASTCALL CheckSASI(DWORD *pDisk);
										// SASI�t�@�C���`�F�b�N
	void FASTCALL EnableControls(BOOL bEnable, BOOL bDrive = TRUE);
										// �R���g���[����ԕύX
	SASI *m_pSASI;
										// SASI
	BOOL m_bInit;
										// �������t���O
	int m_nDrives;
										// �h���C�u��
	TCHAR m_szFile[16][FILEPATH_MAX];
										// SASI�n�[�h�f�B�X�N�t�@�C����
	CString m_strError;
										// �G���[������

	DECLARE_MESSAGE_MAP()
										// ���b�Z�[�W �}�b�v����
};

//===========================================================================
//
//	SxSI�y�[�W
//
//===========================================================================
class CSxSIPage : public CConfigPage
{
public:
	CSxSIPage();
										// �R���X�g���N�^
	BOOL OnInitDialog();
										// ������
	BOOL OnSetActive();
										// �y�[�W�A�N�e�B�u
	void OnOK();
										// ����
	int FASTCALL GetDrives(const Config *pConfig) const;
										// SxSI�h���C�u���擾

protected:
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pBar);
										// �c�X�N���[��
	afx_msg void OnClick(NMHDR *pNMHDR, LRESULT *pResult);
										// �J�����N���b�N
	afx_msg void OnCheck();
										// �`�F�b�N�{�b�N�X�N���b�N

private:
	enum DevType {
		DevSASI,						// SASI �n�[�h�f�B�X�N�h���C�u
		DevSCSI,						// SCSI �n�[�h�f�B�X�N�h���C�u
		DevMO,							// SCSI MO�h���C�u
		DevInit,						// SCSI �C�j�V�G�[�^(�z�X�g)
		DevNone							// �f�o�C�X�Ȃ�
	};
	void FASTCALL UpdateList();
										// ���X�g�R���g���[���X�V
	void FASTCALL BuildMap();
										// �f�o�C�X�}�b�v�쐬
	int FASTCALL CheckSCSI(int nDrive);
										// SCSI�f�o�C�X�`�F�b�N
	void FASTCALL EnableControls(BOOL bEnable, BOOL bDrive = TRUE);
										// �R���g���[����ԕύX
	BOOL m_bInit;
										// �������t���O
	int m_nSASIDrives;
										// SASI�h���C�u��
	DevType m_DevMap[8];
										// �f�o�C�X�}�b�v
	TCHAR m_szFile[6][FILEPATH_MAX];
										// SCSI�n�[�h�f�B�X�N�t�@�C����
	CString m_strSASI;
										// SASI HD������
	CString m_strMO;
										// SCSI MO������
	CString m_strInit;
										// �C�j�V�G�[�^������
	CString m_strNone;
										// n/a������������
	CString m_strError;
										// �f�o�C�X�G���[������
	static const UINT ControlTable[];
										// �R���g���[���e�[�u��

	DECLARE_MESSAGE_MAP()
										// ���b�Z�[�W �}�b�v����
};

//===========================================================================
//
//	SCSI�y�[�W
//
//===========================================================================
class CSCSIPage : public CConfigPage
{
public:
	CSCSIPage();
										// �R���X�g���N�^
	BOOL OnInitDialog();
										// ������
	void OnOK();
										// ����
	int FASTCALL GetInterface(const Config *pConfig) const;
										// �C���^�t�F�[�X��ʎ擾

protected:
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pBar);
										// �c�X�N���[��
	afx_msg void OnClick(NMHDR *pNMHDR, LRESULT *pResult);
										// �J�����N���b�N
	afx_msg void OnButton();
										// ���W�I�{�^���I��
	afx_msg void OnCheck();
										// �`�F�b�N�{�b�N�X�N���b�N

private:
	enum DevType {
		DevSCSI,						// SCSI �n�[�h�f�B�X�N�h���C�u
		DevMO,							// SCSI MO�h���C�u
		DevCD,							// SCSI CD-ROM�h���C�u
		DevInit,						// SCSI �C�j�V�G�[�^(�z�X�g)
		DevNone							// �f�o�C�X�Ȃ�
	};
	int FASTCALL GetIfCtrl() const;
										// �C���^�t�F�[�X��ʎ擾(�R���g���[�����)
	BOOL FASTCALL CheckROM(int nType) const;
										// ROM�`�F�b�N
	void FASTCALL UpdateList();
										// ���X�g�R���g���[���X�V
	void FASTCALL BuildMap();
										// �f�o�C�X�}�b�v�쐬
	int FASTCALL CheckSCSI(int nDrive);
										// SCSI�f�o�C�X�`�F�b�N
	void FASTCALL EnableControls(BOOL bEnable, BOOL bDrive = TRUE);
										// �R���g���[����ԕύX
	SCSI *m_pSCSI;
										// SCSI�f�o�C�X
	BOOL m_bInit;
										// �������t���O
	int m_nDrives;
										// �h���C�u��
	BOOL m_bMOFirst;
										// MO�擪�t���O
	DevType m_DevMap[8];
										// �f�o�C�X�}�b�v
	TCHAR m_szFile[5][FILEPATH_MAX];
										// SCSI�n�[�h�f�B�X�N�t�@�C����
	CString m_strMO;
										// SCSI MO������
	CString m_strCD;
										// SCSI CD������
	CString m_strInit;
										// �C�j�V�G�[�^������
	CString m_strNone;
										// n/a������������
	CString m_strError;
										// �f�o�C�X�G���[������
	static const UINT ControlTable[];
										// �R���g���[���e�[�u��

	DECLARE_MESSAGE_MAP()
										// ���b�Z�[�W �}�b�v����
};

//===========================================================================
//
//	�|�[�g�y�[�W
//
//===========================================================================
class CPortPage : public CConfigPage
{
public:
	CPortPage();
										// �R���X�g���N�^
	BOOL OnInitDialog();
										// ������
	void OnOK();
										// ����
};

//===========================================================================
//
//	MIDI�y�[�W
//
//===========================================================================
class CMIDIPage : public CConfigPage
{
public:
	CMIDIPage();
										// �R���X�g���N�^
	BOOL OnInitDialog();
										// ������
	void OnOK();
										// ����
	void OnCancel();
										// �L�����Z��

protected:
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar *pBar);
										// �c�X�N���[��
	afx_msg void OnBIDClick();
										// �{�[�hID�N���b�N

private:
	void FASTCALL EnableControls(BOOL bEnable);
										// �R���g���[����ԕύX
	CMIDI *m_pMIDI;
										// MIDI�R���|�[�l���g
	BOOL m_bEnableCtrl;
										// �R���g���[�����
	int m_nInDelay;
										// In�f�B���C(ms)
	int m_nOutDelay;
										// Out�f�B���C(ms)
	static const UINT ControlTable[];
										// �R���g���[���e�[�u��

	DECLARE_MESSAGE_MAP()
										// ���b�Z�[�W �}�b�v����
};

//===========================================================================
//
//	�����y�[�W
//
//===========================================================================
class CAlterPage : public CConfigPage
{
public:
	CAlterPage();
										// �R���X�g���N�^
	BOOL OnInitDialog();
										// ������
	BOOL OnKillActive();
										// �y�[�W�ړ�
	BOOL FASTCALL HasParity(const Config *pConfig) const;
										// SASI�p���e�B�@�\�`�F�b�N

protected:
	void DoDataExchange(CDataExchange *pDX);
										// �f�[�^����

private:
	BOOL m_bInit;
										// ������
	BOOL m_bParity;
										// �p���e�B����
};

//===========================================================================
//
//	���W���[���y�[�W
//
//===========================================================================
class CResumePage : public CConfigPage
{
public:
	CResumePage();
										// �R���X�g���N�^
	BOOL OnInitDialog();
										// ������

protected:
	void DoDataExchange(CDataExchange *pDX);
										// �f�[�^����
};

//===========================================================================
//
//	TrueKey���̓_�C�A���O
//
//===========================================================================
class CTKeyDlg : public CDialog
{
public:
	CTKeyDlg(CWnd *pParent);
										// �R���X�g���N�^
	BOOL OnInitDialog();
										// ������
	void OnOK();
										// OK
	void OnCancel();
										// �L�����Z��
	int m_nTarget;
										// �^�[�Q�b�g�L�[
	int m_nKey;
										// ���蓖�ăL�[

protected:
	afx_msg void OnPaint();
										// �`��
	afx_msg UINT OnGetDlgCode();
										// �_�C�A���O�R�[�h�擾
#if _MFC_VER >= 0x700
	afx_msg void OnTimer(UINT_PTR nTimerID);
										// �^�C�}
#else
	afx_msg void OnTimer(UINT nTimerID);
										// �^�C�}
#endif
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
										// �E�N���b�N

private:
	void FASTCALL OnDraw(CDC *pDC);
										// �`�惁�C��
#if _MFC_VER >= 0x700
	UINT_PTR m_nTimerID;
										// �^�C�}ID
#else
	UINT m_nTimerID;
										// �^�C�}ID
#endif
	BYTE m_KeyState[0x100];
										// VK�L�[���
	CTKey *m_pTKey;
										// TrueKey
	CRect m_rectGuide;
										// �K�C�h��`
	CString m_strGuide;
										// �K�C�h������
	CRect m_rectAssign;
										// �L�[���蓖�ċ�`
	CString m_strAssign;
										// �L�[���蓖�ĕ�����
	CRect m_rectKey;
										// �L�[��`
	CString m_strKey;
										// �L�[������

	DECLARE_MESSAGE_MAP()
										// ���b�Z�[�W �}�b�v����
};

//===========================================================================
//
//	TrueKey�y�[�W
//
//===========================================================================
class CTKeyPage : public CConfigPage
{
public:
	CTKeyPage();
										// �R���X�g���N�^
	BOOL OnInitDialog();
										// ������
	void OnOK();
										// ����

protected:
	afx_msg void OnSelChange();
										// �R���{�{�b�N�X�ύX
	afx_msg void OnClick(NMHDR *pNMHDR, LRESULT *pResult);
										// �J�����N���b�N
	afx_msg void OnRClick(NMHDR *pNMHDR, LRESULT *pResult);
										// �J�����E�N���b�N

private:
	void FASTCALL EnableControls(BOOL bEnable);
										// �R���g���[����ԕύX
	void FASTCALL UpdateReport();
										// ���|�[�g�X�V
	BOOL m_bEnableCtrl;
										// �R���g���[���L���t���O
	CInput *m_pInput;
										// CInput
	CTKey *m_pTKey;
										// TrueKey
	int m_nKey[0x73];
										// �ҏW���̕ϊ��e�[�u��
	static const UINT ControlTable[];
										// �R���g���[���e�[�u��

	DECLARE_MESSAGE_MAP()
										// ���b�Z�[�W �}�b�v����
};

//===========================================================================
//
//	���̑��y�[�W
//
//===========================================================================
class CMiscPage : public CConfigPage
{
public:
	CMiscPage();
	BOOL OnInitDialog();
	void OnBuscarFolder();
	void OnOK();
										// �R���X�g���N�^


	void DoDataExchange(CDataExchange *pDX);
										// �f�[�^����
										
DECLARE_MESSAGE_MAP()
};

//===========================================================================
//
//	�R���t�B�O�v���p�e�B�V�[�g
//
//===========================================================================
class CConfigSheet : public CPropertySheet
{
public:
	CConfigSheet(CWnd *pParent);
										// �R���X�g���N�^
	Config *m_pConfig;
										// �ݒ�f�[�^
	CConfigPage* FASTCALL SearchPage(DWORD dwID) const;
										// �y�[�W����

protected:
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
										// �E�B���h�E�쐬
	afx_msg void OnDestroy();
										// �E�B���h�E�폜
#if _MFC_VER >= 0x700
	afx_msg void OnTimer(UINT_PTR nTimerID);
#else
	afx_msg void OnTimer(UINT nTimerID);
#endif
										// �^�C�}

private:
	CFrmWnd *m_pFrmWnd;
										// �t���[���E�B���h�E
#if _MFC_VER >= 0x700
	UINT_PTR m_nTimerID;
#else
	UINT m_nTimerID;
#endif
										// �^�C�}ID

	CBasicPage m_Basic;
										// ��{
	CSoundPage m_Sound;
										// �T�E���h
	CVolPage m_Vol;
										// ����
	CKbdPage m_Kbd;
										// �L�[�{�[�h
	CMousePage m_Mouse;
										// �}�E�X
	CJoyPage m_Joy;
										// �W���C�X�e�B�b�N
	CSASIPage m_SASI;
										// SASI
	CSxSIPage m_SxSI;
										// SxSI
	CSCSIPage m_SCSI;
										// SCSI
	CPortPage m_Port;
										// �|�[�g
	CMIDIPage m_MIDI;
										// MIDI
	CAlterPage m_Alter;
										// ����
	CResumePage m_Resume;
										// ���W���[��
	CTKeyPage m_TKey;
										// TrueKey
	CMiscPage m_Misc;
										// ���̑�

	DECLARE_MESSAGE_MAP()
										// ���b�Z�[�W �}�b�v����
};

#endif	// mfc_cfg_h
#endif	// _WIN32
