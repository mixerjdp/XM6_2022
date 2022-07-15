//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 �o�h�D(ytanaka@ipc-tokai.or.jp)
//	[ MFC �t���[���E�B���h�E ]
//
//---------------------------------------------------------------------------
#include <Windows.h>
#include <tchar.h>
#include <shlobj.h>
#include <assert.h>

#if defined(_WIN32)

#if !defined(mfc_frm_h)
#define mfc_frm_h

//---------------------------------------------------------------------------
//
//	�E�B���h�E���b�Z�[�W
//
//---------------------------------------------------------------------------
#define WM_KICK			WM_APP				// �G�~�����[�^�X�^�[�g
#define WM_SHELLNOTIFY	(WM_USER + 5)		// �t�@�C���V�X�e����ԕω�


//===========================================================================
//
//	�t���[���E�B���h�E
//
//===========================================================================
class CFrmWnd : public CFrameWnd
{
public:
	// ������
	CFrmWnd();
										// �R���X�g���N�^
	BOOL Init();
										// ������
	BOOL m_bFullScreen;
	// �擾
	CDrawView* FASTCALL GetView() const;
										// �`��r���[�擾
	CComponent* FASTCALL GetFirstComponent() const;
										// �ŏ��̃R���|�[�l���g���擾
	CScheduler* FASTCALL GetScheduler() const;
										// �X�P�W���[���擾
	CSound* FASTCALL GetSound() const;
										// �T�E���h�擾
	CInput* FASTCALL GetInput() const;
										// �C���v�b�g�擾
	CPort* FASTCALL GetPort() const;
										// �|�[�g�擾
	CMIDI* FASTCALL GetMIDI() const;
										// MIDI�擾
	CTKey* FASTCALL GetTKey() const;
										// TrueKey�擾
	CHost* FASTCALL GetHost() const;
										// Host�擾
	CInfo* FASTCALL GetInfo() const;
										// Info�擾
	CConfig* FASTCALL GetConfig() const;
										// �R���t�B�O�擾

	// �X�e�[�^�X�r���[�T�|�[�g
	void FASTCALL RecalcStatusView();
										// �X�e�[�^�X�r���[�Ĕz�u

	// �T�u�E�B���h�E�T�|�[�g
	LPCTSTR FASTCALL GetWndClassName() const;
										// �E�B���h�E�N���X���擾
	BOOL FASTCALL IsPopupSWnd() const;


	/* Nombre de Archivo XM6  */
	CString NombreArchivoXM6;
	CString RutaCompletaArchivoXM6;
	CString RutaSaveStates;



	// �h���b�O���h���b�v�T�|�[�g
	BOOL FASTCALL InitCmdSub(int nDrive, LPCTSTR lpszPath);
										// �R�}���h���C������ �T�u

protected:
	// �I�[�o�[���C�h
	BOOL PreCreateWindow(CREATESTRUCT& cs);
										// �E�B���h�E�쐬����
	void GetMessageString(UINT nID, CString& rMessage) const;
										// ���b�Z�[�W�������

	// WM���b�Z�[�W
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
										// �E�B���h�E�쐬
	afx_msg void OnClose();
										// �E�B���h�E�N���[�Y
	afx_msg void OnDestroy();
										// �E�B���h�E�폜
	afx_msg void OnMove(int x, int y);
										// �E�B���h�E�ړ�
	afx_msg LRESULT OnDisplayChange(UINT uParam, LONG lParam);
										// �f�B�X�v���C�ύX
	afx_msg BOOL OnEraseBkgnd(CDC *pDC);
										// �E�B���h�E�w�i�`��
	afx_msg void OnPaint();
										// �E�B���h�E�`��
	afx_msg void OnActivate(UINT nState, CWnd *pWnd, BOOL bMinimized);
										// �A�N�e�B�x�[�g
#if _MFC_VER >= 0x700
	afx_msg void OnActivateApp(BOOL bActive, DWORD dwThreadID);
#else
	afx_msg void OnActivateApp(BOOL bActive, HTASK hTask);
#endif
										// �^�X�N�؂�ւ�
	afx_msg void OnEnterMenuLoop(BOOL bTrackPopup);
										// ���j���[���[�v�J�n
	afx_msg void OnExitMenuLoop(BOOL bTrackPopup);
										// ���j���[���[�v�I��
	afx_msg void OnParentNotify(UINT message, LPARAM lParam);
										// �e�E�B���h�E�ʒm
	afx_msg LONG OnKick(UINT uParam, LONG lParam);
										// �L�b�N
	afx_msg void OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDIS);
										// �I�[�i�[�h���[
	afx_msg void OnContextMenu(CWnd *pWnd, CPoint pos);
										// �R���e�L�X�g���j���[
	afx_msg LONG OnPowerBroadCast(UINT uParam, LONG lParam);
										// �d���ύX�ʒm
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
										// �V�X�e���R�}���h
#if _MFC_VER >= 0x700
	afx_msg BOOL OnCopyData(CWnd* pWnd, COPYDATASTRUCT* pCopyDataStruct);
#else
	afx_msg LONG OnCopyData(UINT uParam, LONG lParam);
										// �f�[�^�]��
#endif
	afx_msg void OnEndSession(BOOL bEnding);
										// �Z�b�V�����I��
	afx_msg LONG OnShellNotify(UINT uParam, LONG lParam);
										// �t�@�C���V�X�e����ԕω�

	// �R�}���h����
	afx_msg void OnOpen();

	afx_msg void OnFastOpen();
										// �J��
	afx_msg void OnOpenUI(CCmdUI *pCmdUI);
										// �J�� UI
	afx_msg void OnSave();
										// �㏑���ۑ�
	afx_msg void OnSaveUI(CCmdUI *pCmdUI);
										// �㏑���ۑ� UI
	afx_msg void OnSaveAs();
										// ���O��t���ĕۑ�
	afx_msg void OnSaveAsUI(CCmdUI *pCmdUI);
										// ���O��t���ĕۑ� UI
	afx_msg void OnMRU(UINT uID);
										// MRU
	afx_msg void OnMRUUI(CCmdUI *pCmdUI);
										// MRU UI
	afx_msg void OnReset();
										// ���Z�b�g
	afx_msg void OnResetUI(CCmdUI *pCmdUI);

	afx_msg void OnScc();
	// ���Z�b�g
	afx_msg void OnSccUI(CCmdUI* pCmdUI);
										// ���Z�b�g UI
	afx_msg void OnInterrupt();
										// �C���^���v�g
	afx_msg void OnInterruptUI(CCmdUI *pCmdUI);
										// �C���^���v�g UI
	afx_msg void OnPower();
										// �d���X�C�b�`
	afx_msg void OnPowerUI(CCmdUI *pCmdUI);
										// �d���X�C�b�` UI
	afx_msg void OnExit();
										// �I��

	afx_msg void OnFD(UINT uID);
										// �t���b�s�[�f�B�X�N�R�}���h
	afx_msg void OnFDOpenUI(CCmdUI *pCmdUI);
										// �t���b�s�[�I�[�v�� UI
	afx_msg void OnFDEjectUI(CCmdUI *pCmdUI);
										// �t���b�s�[�C�W�F�N�g UI
	afx_msg void OnFDWritePUI(CCmdUI *pCmdUI);
										// �t���b�s�[�������ݕی� UI
	afx_msg void OnFDForceUI(CCmdUI *pCmdUI);
										// �t���b�s�[�����C�W�F�N�g UI
	afx_msg void OnFDInvalidUI(CCmdUI *pCmdUI);
										// �t���b�s�[��}�� UI
	afx_msg void OnFDMediaUI(CCmdUI *pCmdUI);
										// �t���b�s�[���f�B�A UI
	afx_msg void OnFDMRUUI(CCmdUI *pCmdUI);
										// �t���b�s�[MRU UI

	afx_msg void OnMOOpen();
										// MO�I�[�v��
	afx_msg void OnMOOpenUI(CCmdUI *pCmdUI);
										// MO�I�[�v�� UI
	afx_msg void OnMOEject();
										// MO�C�W�F�N�g
	afx_msg void OnMOEjectUI(CCmdUI *pCmdUI);
										// MO�C�W�F�N�g UI
	afx_msg void OnMOWriteP();
										// MO�������ݕی�
	afx_msg void OnMOWritePUI(CCmdUI *pCmdUI);
										// MO�������ݕی� UI
	afx_msg void OnMOForce();
										// MO�����C�W�F�N�g
	afx_msg void OnMOForceUI(CCmdUI *pCmdUI);
										// MO�����C�W�F�N�g UI
	afx_msg void OnMOMRU(UINT uID);
										// MOMRU
	afx_msg void OnMOMRUUI(CCmdUI *pCmdUI);
										// MOMRU UI

	afx_msg void OnCDOpen();
										// CD�I�[�v��
	afx_msg void OnCDOpenUI(CCmdUI *pCmdUI);
										// CD�I�[�v�� UI
	afx_msg void OnCDEject();
										// CD�C�W�F�N�g
	afx_msg void OnCDEjectUI(CCmdUI *pCmdUI);
										// CD�C�W�F�N�g UI
	afx_msg void OnCDForce();
										// CD�����C�W�F�N�g
	afx_msg void OnCDForceUI(CCmdUI *pCmdUI);
										// CD�����C�W�F�N�g UI
	afx_msg void OnCDMRU(UINT nID);
										// CDMRU
	afx_msg void OnCDMRUUI(CCmdUI *pCmdUI);
										// CDMRU UI

	afx_msg void OnLog();
										// ���O
	afx_msg void OnLogUI(CCmdUI *pCmdUI);
										// ���O UI
	afx_msg void OnScheduler();
										// �X�P�W���[��
	afx_msg void OnSchedulerUI(CCmdUI *pCmdUI);
										// �X�P�W���[�� UI
	afx_msg void OnDevice();
										// �f�o�C�X
	afx_msg void OnDeviceUI(CCmdUI *pCmdUI);
										// �f�o�C�X UI
	afx_msg void OnCPUReg();
										// CPU���W�X�^
	afx_msg void OnCPURegUI(CCmdUI *pCmdUI);
										// CPU���W�X�^ UI
	afx_msg void OnInt();
										// ���荞��
	afx_msg void OnIntUI(CCmdUI *pCmdUI);
										// ���荞�� UI
	afx_msg void OnDisasm();
										// �t�A�Z���u��
	afx_msg void OnDisasmUI(CCmdUI *pCmdUI);
										// �t�A�Z���u�� UI
	afx_msg void OnMemory();
										// ������
	afx_msg void OnMemoryUI(CCmdUI *pCmdUI);
										// ������ UI
	afx_msg void OnBreakP();
										// �u���[�N�|�C���g
	afx_msg void OnBreakPUI(CCmdUI *pCmdUI);
										// �u���[�N�|�C���g UI
	afx_msg void OnMFP();
										// MFP
	afx_msg void OnMFPUI(CCmdUI *pCmdUI);
										// MFP UI
	afx_msg void OnDMAC();
										// DMAC
	afx_msg void OnDMACUI(CCmdUI *pCmdUI);
										// DMAC UI
	afx_msg void OnCRTC();
										// CRTC
	afx_msg void OnCRTCUI(CCmdUI *pCmdUI);
										// CRTC UI
	afx_msg void OnVC();
										// VC
	afx_msg void OnVCUI(CCmdUI *pCmdUI);
										// VC UI
	afx_msg void OnRTC();
										// RTC
	afx_msg void OnRTCUI(CCmdUI *pCmdUI);
										// RTC UI
	afx_msg void OnOPM();
										// OPM
	afx_msg void OnOPMUI(CCmdUI *pCmdUI);
										// OPM UI
	afx_msg void OnKeyboard();
										// �L�[�{�[�h
	afx_msg void OnKeyboardUI(CCmdUI *pCmdUI);
										// �L�[�{�[�h UI
	afx_msg void OnFDD();
										// FDD
	afx_msg void OnFDDUI(CCmdUI *pCmdUI);
										// FDD UI
	afx_msg void OnFDC();
										// FDC
	afx_msg void OnFDCUI(CCmdUI *pCmdUI);
										// FDC UI
	afx_msg void OnSCC();
										// SCC
	afx_msg void OnSCCUI(CCmdUI *pCmdUI);
										// SCC UI
	afx_msg void OnCynthia();
										// CYNTHIA
	afx_msg void OnCynthiaUI(CCmdUI *pCmdUI);
										// CYNTHIA UI
	afx_msg void OnSASI();
										// SASI
	afx_msg void OnSASIUI(CCmdUI *pCmdUI);
										// SASI UI
	afx_msg void OnMIDI();
										// MIDI
	afx_msg void OnMIDIUI(CCmdUI *pCmdUI);
										// MIDI UI
	afx_msg void OnSCSI();
										// SCSI
	afx_msg void OnSCSIUI(CCmdUI *pCmdUI);
										// SCSI UI
	afx_msg void OnTVRAM();
										// �e�L�X�g���
	afx_msg void OnTVRAMUI(CCmdUI *pCmdUI);
										// �e�L�X�g��� UI
	afx_msg void OnG1024();
										// �O���t�B�b�N���1024�~1024
	afx_msg void OnG1024UI(CCmdUI *pCmdUI);
										// �O���t�B�b�N���1024�~1024 UI
	afx_msg void OnG16(UINT uID);
										// �O���t�B�b�N���16�F
	afx_msg void OnG16UI(CCmdUI *pCmdUI);
										// �O���t�B�b�N���16�F UI
	afx_msg void OnG256(UINT uID);
										// �O���t�B�b�N���256�F
	afx_msg void OnG256UI(CCmdUI *pCmdUI);
										// �O���t�B�b�N���256�F UI
	afx_msg void OnG64K();
										// �O���t�B�b�N���65536�F
	afx_msg void OnG64KUI(CCmdUI *pCmdUI);
										// �O���t�B�b�N���65536�F UI
	afx_msg void OnPCG();
										// PCG
	afx_msg void OnPCGUI(CCmdUI *pCmdUI);
										// PCG UI
	afx_msg void OnBG(UINT uID);
										// BG���
	afx_msg void OnBGUI(CCmdUI *pCmdUI);
										// BG��� UI
	afx_msg void OnPalet();
										// �p���b�g
	afx_msg void OnPaletUI(CCmdUI *pCmdUI);
										// �p���b�g UI
	afx_msg void OnTextBuf();
										// �e�L�X�g�o�b�t�@
	afx_msg void OnTextBufUI(CCmdUI *pCmdUI);
										// �e�L�X�g�o�b�t�@ UI
	afx_msg void OnGrpBuf(UINT uID);
										// �O���t�B�b�N�o�b�t�@
	afx_msg void OnGrpBufUI(CCmdUI *pCmdUI);
										// �O���t�B�b�N�o�b�t�@ UI
	afx_msg void OnPCGBuf();
										// PCG�o�b�t�@
	afx_msg void OnPCGBufUI(CCmdUI *pCmdUI);
										// PCG�o�b�t�@ UI
	afx_msg void OnBGSpBuf();
										// BG/�X�v���C�g�o�b�t�@
	afx_msg void OnBGSpBufUI(CCmdUI *pCmdUI);
										// BG/�X�v���C�g�o�b�t�@ UI
	afx_msg void OnPaletBuf();
										// �p���b�g�o�b�t�@
	afx_msg void OnPaletBufUI(CCmdUI *pCmdUI);
										// �p���b�g�o�b�t�@ UI
	afx_msg void OnMixBuf();
										// �����o�b�t�@
	afx_msg void OnMixBufUI(CCmdUI *pCmdUI);
										// �����o�b�t�@ UI
	afx_msg void OnComponent();
										// �R���|�[�l���g
	afx_msg void OnComponentUI(CCmdUI *pCmdUI);
										// �R���|�[�l���g UI
	afx_msg void OnOSInfo();
										// OS���
	afx_msg void OnOSInfoUI(CCmdUI *pCmdUI);
										// OS��� UI
	afx_msg void OnSound();
										// �T�E���h
	afx_msg void OnSoundUI(CCmdUI *pCmdUI);
										// �T�E���h UI
	afx_msg void OnInput();
										// �C���v�b�g
	afx_msg void OnInputUI(CCmdUI *pCmdUI);
										// �C���v�b�g UI
	afx_msg void OnPort();
										// �|�[�g
	afx_msg void OnPortUI(CCmdUI *pCmdUI);
										// �|�[�g UI
	afx_msg void OnBitmap();
										// �r�b�g�}�b�v
	afx_msg void OnBitmapUI(CCmdUI *pCmdUI);
										// �r�b�g�}�b�v UI
	afx_msg void OnMIDIDrv();
										// MIDI�h���C�o
	afx_msg void OnMIDIDrvUI(CCmdUI *pCmdUI);
										// MIDI�h���C�o UI
	afx_msg void OnCaption();
										// �L���v�V����
	afx_msg void OnCaptionUI(CCmdUI *pCmdUI);
										// �L���v�V���� UI
	afx_msg void OnMenu();
										// ���j���[�o�[
	afx_msg void OnMenuUI(CCmdUI *pCmdUI);
										// ���j���[�o�[ UI
	afx_msg void OnStatus();
										// �X�e�[�^�X�o�[
	afx_msg void OnStatusUI(CCmdUI *pCmdUI);
										// �X�e�[�^�X�o�[ UI
	afx_msg void OnRefresh();
										// ���t���b�V��
	afx_msg void OnStretch();
										// �g��
	afx_msg void OnStretchUI(CCmdUI *pCmdUI);
										// �g�� UI
	afx_msg void OnFullScreen();
										// �t���X�N���[��
	afx_msg void OnFullScreenUI(CCmdUI *pCmdUI);
										// �t���X�N���[��UI

	afx_msg void GetDesktopResolution(int& horizontal, int& vertical);

	afx_msg void OnExec();
										// ���s
	afx_msg void OnExecUI(CCmdUI *pCmdUI);
										// ���s UI
	afx_msg void OnBreak();
										// ��~
	afx_msg void OnBreakUI(CCmdUI *pCmdUI);
										// ��~ UI
	afx_msg void OnTrace();
										// �g���[�X
	afx_msg void OnTraceUI(CCmdUI *pCmdUI);
										// �g���[�X UI

	afx_msg void OnMouseMode();
										// �}�E�X���[�h
	afx_msg void OnSoftKey();
										// �\�t�g�E�F�A�L�[�{�[�h
	afx_msg void OnSoftKeyUI(CCmdUI *pCmdUI);
										// �\�t�g�E�F�A�L�[�{�[�h UI
	afx_msg void OnTimeAdj();
										// �������킹
	afx_msg void OnTrap();
										// trap#0
	afx_msg void OnTrapUI(CCmdUI *pCmdUI);
										// trap#0 UI
	afx_msg void OnSaveWav();
										// WAV�L���v�`��
	afx_msg void OnSaveWavUI(CCmdUI *pCmdUI);
										// WAV�L���v�`�� UI
	afx_msg void OnNewFD();
										// �V�����t���b�s�[�f�B�X�N
	afx_msg void OnNewDisk(UINT uID);
										// �V������e�ʃf�B�X�N
	afx_msg void OnOptions();
										// �I�v�V����

	afx_msg void OnCascade();
										// �d�˂ĕ\��
	afx_msg void OnCascadeUI(CCmdUI *pCmdUI);
										// �d�˂ĕ\�� UI
	afx_msg void OnTile();
										// ���ׂĕ\��
	afx_msg void OnTileUI(CCmdUI *pCmdUI);
										// ���ׂĕ\�� UI
	afx_msg void OnIconic();
										// �S�ăA�C�R����
	afx_msg void OnIconicUI(CCmdUI *pCmdUI);
										// �S�ăA�C�R���� UI
	afx_msg void OnArrangeIcon();
										// �A�C�R���̐���
	afx_msg void OnArrangeIconUI(CCmdUI *pCmdUI);
										// �A�C�R���̐��� UI
	afx_msg void OnHide();
										// �S�ĉB��
	afx_msg void OnHideUI(CCmdUI *pCmdUI);
										// �S�ĉB�� UI
	afx_msg void OnRestore();
										// �S�ĕ���
	afx_msg void OnRestoreUI(CCmdUI *pCmdUI);
										// �S�ĕ��� UI
	afx_msg void OnWindow(UINT uID);
										// �E�B���h�E�I��
	afx_msg void OnAbout();
										// �o�[�W�������

private:
	// ������
	BOOL FASTCALL InitChild();
										// �`���C���h�E�B���h�E������
	void FASTCALL InitPos(BOOL bStart = TRUE);
										// �ʒu�E��`������
	void FASTCALL InitShell();
										// �V�F���A�g������
	BOOL FASTCALL InitVM();
										// VM������
	BOOL FASTCALL InitComponent();
										// �R���|�[�l���g������
	void FASTCALL InitVer();
										// �o�[�W����������
	void FASTCALL InitCmd(LPCTSTR lpszCmd);

	void FASTCALL ReadFile(LPCTSTR pszFileName, CString& str);
	CString FASTCALL CFrmWnd::ProcesarM3u(CString str);

										// �R�}���h���C������
	void FASTCALL ApplyCfg();
										// �ݒ�K�p
	void FASTCALL SizeStatus();
										// �X�e�[�^�X�o�[�T�C�Y�ύX
	void FASTCALL HideTaskBar(BOOL bHide, BOOL bFore);
										// �^�X�N�o�[�B��
	BOOL RestoreFrameWnd(BOOL bFullScreen);
										// �E�B���h�E����
	void RestoreDiskState();
										// �f�B�X�N�E�X�e�[�g����
	int m_nStatus;
										// �X�e�[�^�X�R�[�h
	static const DWORD SigTable[];
										// SRAM�V�O�l�`���e�[�u��

	// �I��
	void SaveFrameWnd();
										// �E�B���h�E�ۑ�
	void SaveDiskState();
										// �f�B�X�N�E�X�e�[�g�ۑ�
	void FASTCALL CleanSub();
										// �N���[���A�b�v
	BOOL m_bExit;
										// �I���t���O
	BOOL m_bSaved;
										// �t���[���E�f�B�X�N�E�X�e�[�g�ۑ��t���O

	// �Z�[�u�E���[�h
	BOOL FASTCALL SaveComponent(const Filepath& path, DWORD dwPos);
										// �Z�[�u
	BOOL FASTCALL LoadComponent(const Filepath& path, DWORD dwPos);
										// ���[�h

	// �R�}���h�n���h���T�u
	BOOL FASTCALL OnOpenSub(const Filepath& path);
										// �I�[�v���T�u
	BOOL FASTCALL OnOpenPrep(const Filepath& path, BOOL bWarning = TRUE);
										// �I�[�v���`�F�b�N
	void FASTCALL OnSaveSub(const Filepath& path);
										// �ۑ��T�u
	void FASTCALL OnFDOpen(int nDrive);
										// �t���b�s�[�I�[�v��
	void FASTCALL OnFDEject(int nDrive);
										// �t���b�s�[�C�W�F�N�g
	void FASTCALL OnFDWriteP(int nDrive);
										// �t���b�s�[�������ݕی�
	void FASTCALL OnFDForce(int nDrive);
										// �t���b�s�[�����C�W�F�N�g
	void FASTCALL OnFDInvalid(int nDrive);
										// �t���b�s�[��}��
	void FASTCALL OnFDMedia(int nDrive, int nMedia);
										// �t���b�s�[���f�B�A
	void FASTCALL OnFDMRU(int nDrive, int nMRU);
										// �t���b�s�[MRU
	int m_nFDDStatus[2];
										// �t���b�s�[�X�e�[�^�X

	// �f�o�C�X�E�r���[�E�R���|�[�l���g
	FDD *m_pFDD;
										// FDD
	SASI *m_pSASI;
										// SASI
	SCSI *m_pSCSI;
										// SCSI
	Scheduler *m_pScheduler;
										// Scheduler
	Keyboard *m_pKeyboard;
										// Keyboard
	Mouse *m_pMouse;
										// Mouse
	CDrawView *m_pDrawView;
										// �`��r���[
	CComponent *m_pFirstComponent;
										// �ŏ��̃R���|�[�l���g
	CScheduler *m_pSch;
										// �X�P�W���[��
	CSound *m_pSound;
										// �T�E���h
	CInput *m_pInput;
										// �C���v�b�g
	CPort *m_pPort;
										// �|�[�g
	CMIDI *m_pMIDI;
										// MIDI
	CTKey *m_pTKey;
										// TrueKey
	CHost *m_pHost;
										// Host
	CInfo *m_pInfo;
										// Info
	CConfig *m_pConfig;
										// �R���t�B�O

	// �t���X�N���[��
	
										// �t���X�N���[���t���O
	DEVMODE m_DevMode;
										// �X�N���[���p�����[�^�L��
	HWND m_hTaskBar;
										// �^�X�N�o�[
	int m_nWndLeft;
										// �E�B���h�E���[�h��x
	int m_nWndTop;
										// �E�B���h�E���[�h��y

	// �T�u�E�B���h�E
	CString m_strWndClsName;
										// �E�B���h�E�N���X��

	// �X�e�[�^�X�r���[
	void FASTCALL CreateStatusView();
										// �X�e�[�^�X�r���[�쐬
	void FASTCALL DestroyStatusView();
										// �X�e�[�^�X�r���[�폜
	CStatusView *m_pStatusView;
										// �X�e�[�^�X�r���[

	// �X�e�[�^�X�o�[
	void FASTCALL ShowStatus();
										// �X�e�[�^�X�o�[�\��
	void FASTCALL ResetStatus();
										// �X�e�[�^�X�o�[���Z�b�g
	CStatusBar m_StatusBar;
										// �X�e�[�^�X�o�[
	BOOL m_bStatusBar;
										// �X�e�[�^�X�o�[�\���t���O

	// ���j���[
	void FASTCALL ShowMenu();
										// ���j���[�o�[�\��
	CMenu m_Menu;
										// ���C�����j���[
	BOOL m_bMenuBar;
										// ���j���[�o�[�\���t���O
	CMenu m_PopupMenu;
										// �|�b�v�A�b�v���j���[
	BOOL m_bPopupMenu;
										// �|�b�v�A�b�v���j���[���s��

	// �L���v�V����
	void FASTCALL ShowCaption();
										// �L���v�V�����\��
	void FASTCALL ResetCaption();
										// �L���v�V�������Z�b�g
	BOOL m_bCaption;
										// �L���v�V�����\���t���O

	// ���
	void FASTCALL SetInfo(CString& strInfo);
										// ��񕶎���Z�b�g	
	ULONG m_uNotifyId;
	
	
	SHChangeNotifyEntry m_fsne[1];							

	// �X�e�[�g�t�@�C��
	void FASTCALL UpdateExec();
										// �X�V(���s)
	DWORD m_dwExec;
										// �Z�[�u����s�J�E���^

	// �R���t�B�M�����[�V����
	BOOL m_bMouseMid;
										// �}�E�X���{�^���L��
	BOOL m_bPopup;
										// �|�b�v�A�b�v���[�h
	BOOL m_bAutoMouse;
										// �����}�E�X���[�h

	DECLARE_MESSAGE_MAP()
										// ���b�Z�[�W �}�b�v����
};

#endif	// mfc_frm_h
#endif	// _WIN32
