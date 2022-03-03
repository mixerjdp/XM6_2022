//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 �o�h�D(ytanaka@ipc-tokai.or.jp)
//	[ MFC �X�P�W���[�� ]
//
//---------------------------------------------------------------------------

#if defined(_WIN32)

#if !defined(mfc_sch_h)
#define mfc_sch_h

//===========================================================================
//
//	�X�P�W���[��
//
//===========================================================================
class CScheduler : public CComponent
{
public:
	// ��{�t�@���N�V����
	CScheduler(CFrmWnd *pFrmWnd);
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

	// ���s����
	void FASTCALL Reset();
										// ���Ԃ����Z�b�g
	void FASTCALL Run();
										// ���s
	void FASTCALL Stop();
										// �X�P�W���[����~

	// �Z�[�u�E���[�h
	BOOL FASTCALL Save(Fileio *pFio, int nVer);
										// �Z�[�u
	BOOL FASTCALL Load(Fileio *pFio, int nVer);
										// ���[�h
	BOOL FASTCALL HasSavedEnable() const { return m_bSavedValid; }
										// �Z�[�u����Enable��Ԃ�ۑ����Ă��邩
	BOOL FASTCALL GetSavedEnable() const { return m_bSavedEnable; }
										// �Z�[�u����Enable��Ԃ�������
	void FASTCALL SetSavedEnable(BOOL bEnable) { m_bSavedEnable = bEnable; }
										// �Z�[�u���̏�Ԃ�ݒ�

	// ���̑�
	void FASTCALL Menu(BOOL bMenu)		{ m_bMenu = bMenu; }
										// ���j���[�ʒm
	void FASTCALL Activate(BOOL bAct)	{ m_bActivate = bAct; }
										// �A�N�e�B�u�ʒm
	void FASTCALL SyncDisasm();
										// �t�A�Z���u������
	int FASTCALL GetFrameRate();
										// �t���[�����[�g�擾

private:
	static UINT ThreadFunc(LPVOID pParam);
										// �X���b�h�֐�
	DWORD FASTCALL GetTime()			{ return timeGetTime(); }
										// ���Ԏ擾
	void FASTCALL Lock()				{ ::LockVM(); }
										// VM���b�N
	void FASTCALL Unlock()				{ ::UnlockVM(); }
										// VM�A�����b�N
	void FASTCALL Refresh();
										// ���t���b�V��
	CPU *m_pCPU;
										// CPU
	Render *m_pRender;
										// �����_��
	CWinThread *m_pThread;
										// �X���b�h�|�C���^
	CSound *m_pSound;
										// �T�E���h
	CInput *m_pInput;
										// �C���v�b�g
	BOOL m_bExitReq;
										// �X���b�h�I���v��
	DWORD m_dwExecTime;
										// �^�C�}�[�J�E���g(���s)
	int m_nSubWndNum;
										// �T�u�E�B���h�E�̌�
	int m_nSubWndDisp;
										// �T�u�E�B���h�E�̕\��(-1:���C�����)
	BOOL m_bMPUFull;
										// MPU�����t���O
	BOOL m_bVMFull;
										// VM�����t���O
	DWORD m_dwDrawCount;
										// ���C���E�B���h�E�\����
	DWORD m_dwDrawPrev;
										// ���C���E�B���h�E�\����(�O)
	DWORD m_dwDrawTime;
										// ���C���E�B���h�E�\������
	DWORD m_dwDrawBackup;
										// ���C���E�B���h�E�\����(�O)
	BOOL m_bMenu;
										// ���j���[�t���O
	BOOL m_bActivate;
										// �A�N�e�B�u�t���O
	BOOL m_bBackup;
										// Enable�t���O�o�b�N�A�b�v
	BOOL m_bSavedValid;
										// �Z�[�u����Enable��Ԃ�ۑ����Ă��邩
	BOOL m_bSavedEnable;
										// �Z�[�u����Enable��������
};

#endif	// mfc_sch_h
#endif	// _WIN32
