//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 �o�h�D(ytanaka@ipc-tokai.or.jp)
//	[ MFC �C���v�b�g ]
//
//---------------------------------------------------------------------------

#if defined(_WIN32)

#if !defined(mfc_inp_h)
#define mfc_inp_h

//===========================================================================
//
//	�C���v�b�g
//
//===========================================================================
class CInput : public CComponent
{
public:
	// �W���C�X�e�B�b�N�萔
	enum {
		JoyDeviceMax = 16,				// �T�|�[�g�f�o�C�X�ő吔
		JoyDevices = 2,					// �g�p�f�o�C�X�ő吔
		JoyAxes = 9,					// ���ő吔
		JoyButtons = 12,				// �{�^���ő吔
		JoyRapids = 10,					// �A�˃��x����
	};
	// �W���C�X�e�B�b�N�ݒ�
	typedef struct _JOYCFG {
		int nDevice;					// �f�o�C�X�ԍ�(1�`)�A���g�p=0
		DWORD dwAxis[JoyAxes];			// ���ϊ� (1�`)�A���g�p=0
		BOOL bAxis[JoyAxes];			// �����]
		DWORD dwButton[JoyButtons];		// �{�^���ϊ� (1�`)�A���g�p=0
		DWORD dwRapid[JoyButtons];		// �A�ˊԊu �A�˂Ȃ�=0
		DWORD dwCount[JoyButtons];		// �A�˃J�E���^
	} JOYCFG, *LPJOYCFG;

public:
	// ��{�t�@���N�V����
	CInput(CFrmWnd *pWnd);
										// �R���X�g���N�^
	BOOL FASTCALL Init();
										// ������
	void FASTCALL Cleanup();
										// �N���[���A�b�v
	void FASTCALL ApplyCfg(const Config *pConfig);
										// �ݒ�K�p
#if defined(_DEBUG)
	void AssertValid() const;
										// �f�f
#endif	// _DEBUG

	// �Z�[�u�E���[�h
	BOOL FASTCALL Save(Fileio *pFio, int nVer);
										// �Z�[�u
	BOOL FASTCALL Load(Fileio *pFio, int nVer);
										// ���[�h

	// �O��API
	void FASTCALL Process(BOOL bRun);
										// �i�s
	void FASTCALL Activate(BOOL bActivate);
										// �A�N�e�B�u�ʒm
	BOOL FASTCALL IsActive() const		{ return m_bActive; }
										// �A�N�e�B�u�󋵎擾
	void FASTCALL Menu(BOOL bMenu);
										// ���j���[�ʒm
	BOOL FASTCALL IsMenu() const		{ return m_bMenu; }
										// ���j���[�󋵎擾
	DWORD FASTCALL GetProcessCount() const	{ return m_dwProcessCount; }
										// �i�s�J�E���^�擾
	DWORD FASTCALL GetAcquireCount(int nType) const;
										// �l���J�E���^�擾

	// �L�[�{�[�h
	void FASTCALL GetKeyBuf(BOOL *pKeyBuf) const;
										// �L�[���͏��擾
	void FASTCALL EnableKey(BOOL bEnable);
										// �L�[�L�����E������
	void FASTCALL SetDefaultKeyMap(DWORD *pKeyMap);
										// �f�t�H���g�}�b�v�ݒ�
	int FASTCALL Key2DirectX(int nKey);
										// �L�[�ϊ�
	int FASTCALL Key2X68k(int nDXKey);
										// �L�[�ϊ�
	static LPCTSTR FASTCALL GetKeyName(int nKey);
										// �L�[���̎擾
	static LPCTSTR FASTCALL GetKeyID(int nID);
										// �L�[ID�擾
	void FASTCALL GetKeyMap(DWORD *pKeyMap);
										// �L�[�}�b�v�擾
	void FASTCALL SetKeyMap(const DWORD *pKeyMap);
										// �L�[�}�b�v�ݒ�
	BOOL FASTCALL IsKeyMapped(int nID) const;
										// �L�[�}�b�v�L���`�F�b�N

	// �}�E�X
	void FASTCALL SetMouseMode(BOOL bMode);
										// �}�E�X���[�h�ݒ�
	BOOL FASTCALL GetMouseMode() const	{ return m_bMouseMode; }
										// �}�E�X���[�h�擾
	void FASTCALL GetMouseInfo(int *pPos, BOOL *pBtn) const;
										// �}�E�X���擾

	// �W���C�X�e�B�b�N
	static BOOL CALLBACK EnumCb(LPDIDEVICEINSTANCE pDevInst, LPVOID pvRef);
										// �W���C�X�e�B�b�N�R�[���o�b�N
	void FASTCALL EnableJoy(BOOL bEnable);
										// �W���C�X�e�B�b�N�L�����E������
	int FASTCALL GetJoyDevice(int nJoy) const;
										// �W���C�X�e�B�b�N�f�o�C�X�擾
	LONG FASTCALL GetJoyAxis(int nJoy, int nAxis) const;
										// �W���C�X�e�B�b�N���擾
	DWORD FASTCALL GetJoyButton(int nJoy, int nButton) const;
										// �W���C�X�e�B�b�N�{�^���擾
	BOOL FASTCALL GetJoyCaps(int nDevice, CString& strDesc, DIDEVCAPS *pCaps) const;
										// �W���C�X�e�B�b�NCaps�擾
	void FASTCALL GetJoyCfg(int nJoy, LPJOYCFG lpJoyCfg) const;
										// �W���C�X�e�B�b�N�ݒ�擾
	void FASTCALL SetJoyCfg(int nJoy, const LPJOYCFG lpJoyCfg);
										// �W���C�X�e�B�b�N�ݒ�Z�b�g

private:
	// ����
	LPDIRECTINPUT m_lpDI;
										// DirectInput
	BOOL m_bActive;
										// �A�N�e�B�u�t���O
	BOOL m_bMenu;
										// ���j���[�t���O
	CRTC *m_pCRTC;
										// CRTC
	DWORD m_dwDispCount;
										// CRTC�\���J�E���g
	DWORD m_dwProcessCount;
										// �i�s�J�E���g

	// �Z�[�u�E���[�h
	BOOL FASTCALL SaveMain(Fileio *pFio);
										// �Z�[�u�{��
	BOOL FASTCALL Load200(Fileio *pFio);
										// ���[�h�{�� (version2.00)
	BOOL FASTCALL Load201(Fileio *pFio);
										// ���[�h�{�� (version2.01)

	// �L�[�{�[�h
	BOOL FASTCALL InitKey();
										// �L�[�{�[�h������
	void FASTCALL InputKey(BOOL bEnable);
										// �L�[�{�[�h����
	Keyboard *m_pKeyboard;
										// �L�[�{�[�h
	LPDIRECTINPUTDEVICE m_lpDIKey;
										// �L�[�{�[�h�f�o�C�X
	DWORD m_dwKeyAcquire;
										// �L�[�{�[�h�l���J�E���^
	BOOL m_bKeyEnable;
										// �L�[�{�[�h�L���t���O
	BOOL m_KeyBuf[0x100];
										// �L�[�{�[�h�o�b�t�@
	DWORD m_KeyMap[0x100];
										// �L�[�ϊ��}�b�v
	static const DWORD m_KeyMap106[];
										// �f�t�H���g�}�b�v(106)
	static LPCTSTR KeyNameTable[];
										// �L�[���̃e�[�u��
	static LPCSTR KeyIDTable[];
										// DirectX�L�[ID�e�[�u��

	// �}�E�X
	BOOL FASTCALL InitMouse();
										// �}�E�X������
	void FASTCALL InputMouse(BOOL bEnable);
										// �}�E�X����
	Mouse *m_pMouse;
										// �}�E�X
	LPDIRECTINPUTDEVICE m_lpDIMouse;
										// �}�E�X�f�o�C�X
	DWORD m_dwMouseAcquire;
										// �}�E�X�l���J�E���^
	BOOL m_bMouseMode;
										// �}�E�X���[�h�t���O
	int m_nMouseX;
										// �}�E�Xx���W
	int m_nMouseY;
										// �}�E�Xy���W
	BOOL m_bMouseB[2];
										// �}�E�X���E�{�^��
	DWORD m_dwMouseMid;
										// �}�E�X�����{�^���J�E���g
	BOOL m_bMouseMid;
										// �}�E�X�����{�^���g�p�t���O

	// �W���C�X�e�B�b�N
	void FASTCALL EnumJoy();
										// �W���C�X�e�B�b�N��
	BOOL FASTCALL EnumDev(LPDIDEVICEINSTANCE pDevInst);
										// �W���C�X�e�B�b�N�ǉ�
	void FASTCALL InitJoy();
										// �W���C�X�e�B�b�N������
	void FASTCALL InputJoy(BOOL bEnable);
										// �W���C�X�e�B�b�N����
	void FASTCALL MakeJoy(BOOL bEnable);
										// �W���C�X�e�B�b�N����
	PPI *m_pPPI;
										// PPI
	BOOL m_bJoyEnable;
										// �W���C�X�e�B�b�N�L���E����
	LPDIRECTINPUTDEVICE m_lpDIJoy[JoyDevices];
										// �W���C�X�e�B�b�N�f�o�C�X
	LPDIRECTINPUTDEVICE2 m_lpDIDev2[JoyDevices];
										// �t�H�[�X�t�B�[�h�o�b�N�f�o�C�X
	JOYCFG m_JoyCfg[JoyDevices];
										// �W���C�X�e�B�b�N�R���t�B�O
	LONG m_lJoyAxisMin[JoyDevices][JoyAxes];
										// �W���C�X�e�B�b�N���ŏ��l
	LONG m_lJoyAxisMax[JoyDevices][JoyAxes];
										// �W���C�X�e�B�b�N���ő�l
	DWORD m_dwJoyAcquire[JoyDevices];
										// �W���C�X�e�B�b�N�l���J�E���^
	DIJOYSTATE m_JoyState[JoyDevices];
										// �W���C�X�e�B�b�N���
	DWORD m_dwJoyDevs;
										// �W���C�X�e�B�b�N�f�o�C�X��
	DIDEVCAPS m_JoyDevCaps[JoyDeviceMax];
										// �W���C�X�e�B�b�NCaps
	DIDEVICEINSTANCE m_JoyDevInst[JoyDeviceMax];
										// �W���C�X�e�B�b�N�C���X�^���X
	static const DWORD JoyAxisOffsetTable[JoyAxes];
										// �W���C�X�e�B�b�N���I�t�Z�b�g�e�[�u��
	static const DWORD JoyRapidTable[JoyRapids + 1];
										// �W���C�X�e�B�b�N�A�˃e�[�u��
};

#endif	// mfc_inp_h
#endif	// _WIN32
