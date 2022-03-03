//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 �o�h�D(ytanaka@ipc-tokai.or.jp)
//	[ MFC �R�}���h���� ]
//
//---------------------------------------------------------------------------

#if defined(_WIN32)

#include "os.h"
#include "xm6.h"
#include "vm.h"
#include "fdd.h"
#include "fdi.h"
#include "rtc.h"
#include "keyboard.h"
#include "sasi.h"
#include "sram.h"
#include "memory.h"
#include "render.h"
#include "fileio.h"
#include "mfc_frm.h"
#include "mfc_res.h"
#include "mfc_draw.h"
#include "mfc_cpu.h"
#include "mfc_sys.h"
#include "mfc_dev.h"
#include "mfc_w32.h"
#include "mfc_ver.h"
#include "mfc_com.h"
#include "mfc_sch.h"
#include "mfc_snd.h"
#include "mfc_inp.h"
#include "mfc_cfg.h"
#include "mfc_vid.h"
#include "mfc_rend.h"
#include "mfc_stat.h"
#include "mfc_tool.h"

//---------------------------------------------------------------------------
//
//	Abrir
//
//---------------------------------------------------------------------------
void CFrmWnd::OnOpen()
{
	Filepath path;
	TCHAR szPath[_MAX_PATH];

	// �R�����_�C�A���O���s
	::GetVM()->GetPath(path);
	_tcscpy(szPath, path.GetPath());
	if (!::FileOpenDlg(this, szPath, IDS_XM6OPEN)) {
		ResetCaption();
		return;
	}
	path.SetPath(szPath);
			

	// �I�[�v���O����
	if (!OnOpenPrep(path)) {
		return;
	}

	// �I�[�v���T�u
	OnOpenSub(path);
}



void CFrmWnd::OnFastOpen()
{
	Filepath path;
	TCHAR szPath[_MAX_PATH];		
	TCHAR rutaCargaRapida[_MAX_PATH];		 
	 sprintf(rutaCargaRapida, "%s\\%s.xm6", RutaSaveStates, NombreArchivoXM6);


	/* int msgboxID = MessageBox(
		 rutaCargaRapida,"ruta",
        2 );	*/
	 
	_tcscpy(szPath, rutaCargaRapida);
	path.SetPath(szPath);
			

	// �I�[�v���O����
	if (!OnOpenPrep(path)) {
		return;
	}

	// �I�[�v���T�u
	OnOpenSub(path);
}

//---------------------------------------------------------------------------
//
//	�J�� UI
//
//---------------------------------------------------------------------------
void CFrmWnd::OnOpenUI(CCmdUI *pCmdUI)
{
	BOOL bPower;
	BOOL bSW;
	Filepath path;
	CMenu *pMenu;
	CMenu *pSubMenu;
	CString strExit;
	TCHAR szMRU[_MAX_PATH];
	TCHAR szDrive[_MAX_DRIVE];
	TCHAR szDir[_MAX_DIR];
	TCHAR szFile[_MAX_FNAME];
	TCHAR szExt[_MAX_EXT];
	int nEnable;
	int i;

	// �d����Ԃ��擾�A�t�@�C���p�X���擾(VM���b�N���čs��)
	::LockVM();
	bPower = ::GetVM()->IsPower();
	bSW = ::GetVM()->IsPowerSW();
	::GetVM()->GetPath(path);
	::UnlockVM();

	// �I�[�v��
	pCmdUI->Enable(bPower);

	// �T�u���j���[�擾
	if (m_bPopupMenu) {
		pMenu = m_PopupMenu.GetSubMenu(0);
	}
	else {
		pMenu = &m_Menu;
	}
	ASSERT(pMenu);
	// �t�@�C�����j���[�͍ŏ�
	pSubMenu = pMenu->GetSubMenu(0);
	ASSERT(pSubMenu);

	// �㏑���ۑ�UI(�ȉ��AON_UPDATE_COMMAND_UI�̃^�C�~���O�΍�)
	if (bPower && (_tcslen(path.GetPath()) > 0)) {
		pSubMenu->EnableMenuItem(1, MF_BYPOSITION | MF_ENABLED);
	}
	else {
		pSubMenu->EnableMenuItem(1, MF_BYPOSITION | MF_GRAYED);
	}

	// ���O��t���ĕۑ�UI
	if (bPower) {
		pSubMenu->EnableMenuItem(2, MF_BYPOSITION | MF_GRAYED);
	}
	else {
		pSubMenu->EnableMenuItem(2, MF_BYPOSITION | MF_ENABLED);
	}

	// ���Z�b�gUI
	if (bPower) {
		pSubMenu->EnableMenuItem(4, MF_BYPOSITION | MF_ENABLED);
	}
	else {
		pSubMenu->EnableMenuItem(4, MF_BYPOSITION | MF_GRAYED);
	}

	// �C���^���v�gUI
	if (bPower) {
		pSubMenu->EnableMenuItem(6, MF_BYPOSITION | MF_ENABLED);
	}
	else {
		pSubMenu->EnableMenuItem(6, MF_BYPOSITION | MF_GRAYED);
	}

	// �d���X�C�b�`UI
	if (bSW) {
		pSubMenu->EnableMenuItem(7, MF_BYPOSITION | MF_CHECKED);
	}
	else {
		pSubMenu->EnableMenuItem(7, MF_BYPOSITION | MF_UNCHECKED);
	}

	// �Z�p���[�^�������āA����ȍ~�̃��j���[�͂��ׂč폜
	while (pSubMenu->GetMenuItemCount() > 9) {
		pSubMenu->RemoveMenu(9, MF_BYPOSITION);
	}

	// MRU���Ȃ���΁A�I�����j���[��ǉ����ďI���
	if (GetConfig()->GetMRUNum(4) == 0) {
		::GetMsg(IDS_EXIT, strExit);
		pSubMenu->AppendMenu(MF_STRING, IDM_EXIT, strExit);
		return;
	}

	// �L���E�����萔�ݒ�
	if (bPower) {
		nEnable = MF_BYCOMMAND | MF_GRAYED;
	}
	else {
		nEnable = MF_BYCOMMAND | MF_ENABLED;
	}

	// MRU���� - �ǉ�
	for (i=0; i<9; i++) {
		// �擾���Ă݂�
		GetConfig()->GetMRUFile(4, i, szMRU);
		if (szMRU[0] == _T('\0')) {
			break;
		}

		// ����΃��j���[�ɒǉ�
		_tsplitpath(szMRU, szDrive, szDir, szFile, szExt);
		if (_tcslen(szDir) > 1) {
			_tcscpy(szDir, _T("\\...\\"));
		}
		_stprintf(szMRU, _T("&%d "), i + 1);
		_tcscat(szMRU, szDrive);
		_tcscat(szMRU, szDir);
		_tcscat(szMRU, szFile);
		_tcscat(szMRU, szExt);

		pSubMenu->AppendMenu(MF_STRING, IDM_XM6_MRU0 + i, szMRU);
		pSubMenu->EnableMenuItem(IDM_XM6_MRU0 + i, nEnable);
	}

	// �Z�p���[�^��ǉ�
	pSubMenu->AppendMenu(MF_SEPARATOR, 0, (LPCTSTR)NULL);

	// �I�����j���[��ǉ�
	::GetMsg(IDS_EXIT, strExit);
	pSubMenu->AppendMenu(MF_STRING, IDM_EXIT, strExit);
}

//---------------------------------------------------------------------------
//
//	�I�[�v���O�`�F�b�N
//
//---------------------------------------------------------------------------
BOOL FASTCALL CFrmWnd::OnOpenPrep(const Filepath& path, BOOL bWarning)
{
	Fileio fio;
	CString strMsg;
	CString strFmt;
	char cHeader[0x10];
	int nRecVer;
	int nNowVer;
	DWORD dwMajor;
	DWORD dwMinor;

	ASSERT(this);

	// �t�@�C�����݃`�F�b�N
	if (!fio.Open(path, Fileio::ReadOnly)) {
		if (bWarning) {
			::GetMsg(IDS_XM6LOADFILE, strMsg);
			MessageBox(strMsg, NULL, MB_ICONSTOP | MB_OK);
		}
		return FALSE;
	}

	// �w�b�_�ǂݍ���
	memset(cHeader, 0, sizeof(cHeader));
	fio.Read(cHeader, sizeof(cHeader));
	fio.Close();

	// �L�^�o�[�W�����擾
	cHeader[0x0a] = '\0';
	nRecVer = ::strtoul(&cHeader[0x09], NULL, 16);
	nRecVer <<= 8;
	cHeader[0x0d] = '\0';
	nRecVer |= ::strtoul(&cHeader[0x0b], NULL, 16);

	// ���s�o�[�W�����擾
	::GetVM()->GetVersion(dwMajor, dwMinor);
	nNowVer = (int)((dwMajor << 8) | dwMinor);

	// �w�b�_�`�F�b�N
	cHeader[0x09] = '\0';
	if (strcmp(cHeader, "XM6 DATA ") != 0) {
		if (bWarning) {
			::GetMsg(IDS_XM6LOADHDR, strMsg);
			MessageBox(strMsg, NULL, MB_ICONSTOP | MB_OK);
		}
		return FALSE;
	}

	// �o�[�W�����`�F�b�N
	if (nNowVer < nRecVer) {
		// �L�^����Ă���o�[�W�����̂ق����V����(�m��Ȃ��`��)
		::GetMsg(IDS_XM6LOADVER, strMsg);
		strFmt.Format(strMsg,
						nNowVer >> 8, nNowVer & 0xff,
						nRecVer >> 8, nRecVer & 0xff);
		MessageBox(strFmt, NULL, MB_ICONSTOP | MB_OK);
		return FALSE;
	}

	// �p��
	return TRUE;
}

//---------------------------------------------------------------------------
//
//	�I�[�v���T�u
//
//---------------------------------------------------------------------------
BOOL FASTCALL CFrmWnd::OnOpenSub(const Filepath& path)
{
	BOOL bRun;
	BOOL bSound;
	CString strMsg;
	DWORD dwPos;
	Filepath diskpath;
	int nDrive;

	// �X�P�W���[����~�A�T�E���h��~
	bRun = GetScheduler()->IsEnable();
	GetScheduler()->Enable(FALSE);
	::LockVM();
	::UnlockVM();
	bSound = GetSound()->IsEnable();
	GetSound()->Enable(FALSE);

	// ���[�h
	AfxGetApp()->BeginWaitCursor();

	// VM
	dwPos = ::GetVM()->Load(path);
	if (dwPos == 0) {
		AfxGetApp()->EndWaitCursor();

		// ���s�͓r�����f�Ŋ댯�Ȃ��߁A�K�����Z�b�g����
		::GetVM()->Reset();
		GetSound()->Enable(bSound);
		GetScheduler()->Reset();
		GetScheduler()->Enable(bRun);
		ResetCaption();

		// ���[�h�G���[
		::GetMsg(IDS_XM6LOADERR, strMsg);
		MessageBox(strMsg, NULL, MB_ICONSTOP | MB_OK);
		return FALSE;
	}

	// MFC
	if (!LoadComponent(path, dwPos)) {
		AfxGetApp()->EndWaitCursor();

		// ���s�͓r�����f�Ŋ댯�Ȃ��߁A�K�����Z�b�g����
		::GetVM()->Reset();
		GetSound()->Enable(bSound);
		GetScheduler()->Reset();
		GetScheduler()->Enable(bRun);
		ResetCaption();

		// ���[�h�G���[
		::GetMsg(IDS_XM6LOADERR, strMsg);
		MessageBox(strMsg, NULL, MB_ICONSTOP | MB_OK);
		return FALSE;
	}

	// ���[�h�I��
	AfxGetApp()->EndWaitCursor();

	// FD, MO, CD��MRU�֒ǉ�(version2.04�ȍ~�̃��W���[���΍�)
	for (nDrive=0; nDrive<2; nDrive++) {
		if (m_pFDD->IsReady(nDrive, FALSE)) {
			m_pFDD->GetPath(nDrive, diskpath);
			GetConfig()->SetMRUFile(nDrive, diskpath.GetPath());
		}
	}
	if (m_pSASI->IsReady()) {
		m_pSASI->GetPath(diskpath);
		GetConfig()->SetMRUFile(2, diskpath.GetPath());
	}
	if (m_pSCSI->IsReady(FALSE)) {
		m_pSCSI->GetPath(diskpath, FALSE);
		GetConfig()->SetMRUFile(3, diskpath.GetPath());
	}

	// �X�P�W���[������~�̏�ԂŃZ�[�u����Ă���΁A��~�̂܂�(version2.04)
	if (GetScheduler()->HasSavedEnable()) {
		bRun = GetScheduler()->GetSavedEnable();
	}

	// ���s�J�E���^���N���A
	m_dwExec = 0;

	// ����
	GetSound()->Enable(bSound);
	GetScheduler()->Reset();
	GetScheduler()->Enable(bRun);
	ResetCaption();

	// MRU�ɒǉ�
	GetConfig()->SetMRUFile(4, path.GetPath());

	// ��񃁃b�Z�[�W��\��
	::GetMsg(IDS_XM6LOADOK, strMsg);
	SetInfo(strMsg);

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	�㏑���ۑ�
//
//---------------------------------------------------------------------------
void CFrmWnd::OnSave()
{
	Filepath path;

	// VM����J�����g�p�X���󂯎��
	::GetVM()->GetPath(path);

	// �N���A����Ă���ΏI��
	if (path.IsClear()) {
		return;
	}

	// �ۑ��T�u
	OnSaveSub(path);
}

//---------------------------------------------------------------------------
//
//	�㏑���ۑ� UI
//
//---------------------------------------------------------------------------
void CFrmWnd::OnSaveUI(CCmdUI *pCmdUI)
{
	Filepath path;

	// �d��OFF�ł���΋֎~
	if (!::GetVM()->IsPower()) {
		pCmdUI->Enable(FALSE);
		return;
	}

	// VM����J�����g�p�X���󂯎��
	::GetVM()->GetPath(path);

	// �N���A����Ă���Ύg�p�֎~
	if (path.IsClear()) {
		pCmdUI->Enable(FALSE);
		return;
	}

	// �g�p����
	pCmdUI->Enable(TRUE);
}

//---------------------------------------------------------------------------
//
//	���O��t���ĕۑ�
//
//---------------------------------------------------------------------------
void CFrmWnd::OnSaveAs()
{
	Filepath path;
	TCHAR szPath[_MAX_PATH];

	// �R�����_�C�A���O���s
	::GetVM()->GetPath(path);
	_tcscpy(szPath, path.GetPath());
	/*if (!::FileSaveDlg(this, szPath, _T("xm6"), IDS_XM6OPEN)) {
		ResetCaption();		 

		return;
	}*/

	 TCHAR lpFile[_MAX_PATH];
	 strcpy(lpFile,RutaSaveStates);
	 TCHAR cadenaArchivo[_MAX_PATH];
	 sprintf(cadenaArchivo, "%s\\%s.xm6", lpFile, NombreArchivoXM6);

	 /* ACA SE GUARDA UN ESTADO RAPIDO */
	

	/* int msgboxID = MessageBox(
		 cadenaArchivo, "GUARDADO",
        4     );*/



	 path.SetPath(cadenaArchivo);
			

	// �ۑ��T�u
	OnSaveSub(path);
}

//---------------------------------------------------------------------------
//
//	���O��t���ĕۑ� UI
//
//---------------------------------------------------------------------------
void CFrmWnd::OnSaveAsUI(CCmdUI *pCmdUI)
{
	// �d��ON�̏ꍇ�̂�
	pCmdUI->Enable(::GetVM()->IsPower());
}

//---------------------------------------------------------------------------
//
//	�ۑ��T�u
//
//---------------------------------------------------------------------------
void FASTCALL CFrmWnd::OnSaveSub(const Filepath& path)
{
	BOOL bRun;
	BOOL bSound;
	CString strMsg;
	DWORD dwPos;

	// �X�P�W���[����~�A�T�E���h��~
	bRun = GetScheduler()->IsEnable();
	GetScheduler()->Enable(FALSE);
	::LockVM();
	::UnlockVM();
	bSound = GetSound()->IsEnable();
	GetSound()->Enable(FALSE);

	AfxGetApp()->BeginWaitCursor();

	// �X�P�W���[���ɑ΂��āA�Z�[�u���̏�Ԃ�ʒm(version2.04)
	GetScheduler()->SetSavedEnable(bRun);

	// VM
	dwPos = ::GetVM()->Save(path);
	if (dwPos== 0) {
		AfxGetApp()->EndWaitCursor();

		// �Z�[�u���s
		GetSound()->Enable(bSound);
		GetScheduler()->Reset();
		GetScheduler()->Enable(bRun);
		ResetCaption();

		// �Z�[�u�G���[
		::GetMsg(IDS_XM6SAVEERR, strMsg);
		MessageBox(strMsg, NULL, MB_ICONSTOP | MB_OK);
		return;
	}

	// MFC
	if (!SaveComponent(path, dwPos)) {
		AfxGetApp()->EndWaitCursor();

		// �Z�[�u���s
		GetSound()->Enable(bSound);
		GetScheduler()->Reset();
		GetScheduler()->Enable(bRun);
		ResetCaption();

		// �Z�[�u�G���[
		::GetMsg(IDS_XM6SAVEERR, strMsg);
		MessageBox(strMsg, NULL, MB_ICONSTOP | MB_OK);
		return;
	}

	// ���s�J�E���^���N���A
	m_dwExec = 0;

	AfxGetApp()->EndWaitCursor();

	// ����
	GetSound()->Enable(bSound);
	GetScheduler()->Reset();
	GetScheduler()->Enable(bRun);
	ResetCaption();

	// MRU�ɒǉ�
	GetConfig()->SetMRUFile(4, path.GetPath());

	// ��񃁃b�Z�[�W��\��
	::GetMsg(IDS_XM6SAVEOK, strMsg);
	SetInfo(strMsg);
}

//---------------------------------------------------------------------------
//
//	MRU
//
//---------------------------------------------------------------------------
void CFrmWnd::OnMRU(UINT uID)
{
	TCHAR szMRU[_MAX_PATH];
	Filepath path;

	ASSERT(uID >= IDM_XM6_MRU0);

	// uID�ϊ�
	uID -= IDM_XM6_MRU0;
	ASSERT(uID <= 8);

	// MRU�擾�A�p�X�쐬
	GetConfig()->GetMRUFile(4, (int)uID, szMRU);
	if (szMRU[0] == _T('\0')) {
		return;
	}
	path.SetPath(szMRU);

	// �I�[�v���O����
	if (!OnOpenPrep(path)) {
		return;
	}

	// �I�[�v������
	if (OnOpenSub(path)) {
		// �f�t�H���g�f�B���N�g���X�V
		Filepath::SetDefaultDir(szMRU);
	}
}

//---------------------------------------------------------------------------
//
//	MRU UI
//
//---------------------------------------------------------------------------
void CFrmWnd::OnMRUUI(CCmdUI *pCmdUI)
{
	// �d��ON�̏ꍇ�̂�
	pCmdUI->Enable(::GetVM()->IsPower());
}

//---------------------------------------------------------------------------
//
//	���Z�b�g
//
//---------------------------------------------------------------------------
void CFrmWnd::OnReset()
{
	SRAM *pSRAM;
	DWORD Sw[0x100];
	DWORD dwDevice;
	DWORD dwAddr;
	CString strReset;
	CString strSub;
	BOOL bFlag;
	int i;

	// �d��OFF�Ȃ瑀��s��
	if (!::GetVM()->IsPower()) {
		return;
	}

	::LockVM();

	// ���Z�b�g���ĕ`��
	::GetVM()->Reset();
	GetView()->Refresh();
	ResetCaption();

	// �������X�C�b�`�擾���s��
	pSRAM = (SRAM*)::GetVM()->SearchDevice(MAKEID('S', 'R', 'A', 'M'));
	ASSERT(pSRAM);
	for (i=0; i<0x100; i++) {
		Sw[i] = pSRAM->ReadOnly(0xed0000 + i);
	}

	::UnlockVM();

	// ���Z�b�g���b�Z�[�W�����[�h
	::GetMsg(IDS_RESET, strReset);

	// �������X�C�b�`�̐擪���r
	if (memcmp(Sw, SigTable, sizeof(DWORD) * 7) != 0) {
		SetInfo(strReset);
		return;
	}

	// �u�[�g�f�o�C�X���擾
	dwDevice = Sw[0x18];
	dwDevice <<= 8;
	dwDevice |= Sw[0x19];

	// �u�[�g�f�o�C�X����
	bFlag = FALSE;
	if (dwDevice == 0x0000) {
		// STD
		strSub = _T("STD)");
		bFlag = TRUE;
	}
	if (dwDevice == 0xa000) {
		// ROM
		dwAddr = Sw[0x0c];
		dwAddr = (dwAddr << 8) | Sw[0x0d];
		dwAddr = (dwAddr << 8) | Sw[0x0e];
		dwAddr = (dwAddr << 8) | Sw[0x0f];

		// FC0000�`FC001C�ƁAEA0020�`EA003C��SCSI#
		strSub.Format(_T("ROM $%06X)"), dwAddr);
		if ((dwAddr >= 0xfc0000) && (dwAddr < 0xfc0020)) {
			strSub.Format(_T("SCSI%1d)"), (dwAddr & 0x001f) >> 2);
		}
		if ((dwAddr >= 0xea0020) && (dwAddr < 0xea0040)) {
			strSub.Format(_T("SCSI%1d)"), (dwAddr & 0x001f) >> 2);
		}
		bFlag = TRUE;
	}
	if (dwDevice == 0xb000) {
		// RAM
		dwAddr = Sw[0x10];
		dwAddr = (dwAddr << 8) | Sw[0x11];
		dwAddr = (dwAddr << 8) | Sw[0x12];
		dwAddr = (dwAddr << 8) | Sw[0x13];
		strSub.Format(_T("RAM $%06X)"), dwAddr);
		bFlag = TRUE;
	}
	if ((dwDevice & 0xf0ff) == 0x9070) {
		strSub.Format(_T("2HD%1d)"), (dwDevice & 0xf00) >> 8);
		bFlag = TRUE;
	}
	if ((dwDevice & 0xf0ff) == 0x8000) {
		strSub.Format(_T("HD%1d)"), (dwDevice & 0xf00) >> 8);
		bFlag = TRUE;
	}
	if (!bFlag) {
		strSub = _T("Unknown)");
	}

	// �\��
	strReset += _T(" (");
	strReset += strSub;
	SetInfo(strReset);
}

//---------------------------------------------------------------------------
//
//	���Z�b�g UI
//
//---------------------------------------------------------------------------
void CFrmWnd::OnResetUI(CCmdUI *pCmdUI)
{
	// �d��ON�Ȃ瑀��ł���
	pCmdUI->Enable(::GetVM()->IsPower());
}

//---------------------------------------------------------------------------
//
//	SRAM�V�O�l�`���e�[�u��
//
//---------------------------------------------------------------------------
const DWORD CFrmWnd::SigTable[] = {
	0x82, 0x77, 0x36, 0x38, 0x30, 0x30, 0x30
};

//---------------------------------------------------------------------------
//
//	�C���^���v�g
//
//---------------------------------------------------------------------------
void CFrmWnd::OnInterrupt()
{
	CString strIntr;

	// �d��ON�Ȃ瑀��ł���
	if (::GetVM()->IsPower()) {
		// NMI���荞��
		::LockVM();
		::GetVM()->Interrupt();
		::UnlockVM();

		// ���b�Z�[�W
		::GetMsg(IDS_INTERRUPT, strIntr);
		SetInfo(strIntr);
	}
}

//---------------------------------------------------------------------------
//
//	�C���^���v�g UI
//
//---------------------------------------------------------------------------
void CFrmWnd::OnInterruptUI(CCmdUI *pCmdUI)
{
	// �d��ON�Ȃ瑀��ł���
	pCmdUI->Enable(::GetVM()->IsPower());
}

//---------------------------------------------------------------------------
//
//	�d���X�C�b�`
//
//---------------------------------------------------------------------------
void CFrmWnd::OnPower()
{
	BOOL bPower;

	::LockVM();

	if (::GetVM()->IsPowerSW()) {
		// �I���Ȃ�I�t
		::GetVM()->PowerSW(FALSE);
		::UnlockVM();
		return;
	}

	// ���݂̓d���̏�Ԃ�ۑ����āA�d��ON
	bPower = ::GetVM()->IsPower();
	::GetVM()->PowerSW(TRUE);

	// �d�����؂�Ă��ăX�P�W���[�����~�܂��Ă���΁A������
	if (!bPower && !GetScheduler()->IsEnable()) {
		GetScheduler()->Enable(TRUE);
	}

	::UnlockVM();

	// ���Z�b�g(�X�e�[�^�X�o�[�\���̂���)
	OnReset();
}

//---------------------------------------------------------------------------
//
//	�d���X�C�b�` UI
//
//---------------------------------------------------------------------------
void CFrmWnd::OnPowerUI(CCmdUI *pCmdUI)
{
	// �Ƃ肠�����A�I���Ȃ�`�F�b�N���Ă���
	pCmdUI->SetCheck(::GetVM()->IsPowerSW());
}

//---------------------------------------------------------------------------
//
//	�I��
//
//---------------------------------------------------------------------------
void CFrmWnd::OnExit()
{
	PostMessage(WM_CLOSE, 0, 0);
}

//---------------------------------------------------------------------------
//
//	�t���b�s�[�f�B�X�N����
//
//---------------------------------------------------------------------------
void CFrmWnd::OnFD(UINT uID)
{
	int nDrive;

	// �h���C�u����
	nDrive = 0;
	if (uID >= IDM_D1OPEN) {
		nDrive = 1;
		uID -= (IDM_D1OPEN - IDM_D0OPEN);
	}

	switch (uID) {
		// �I�[�v��
		case IDM_D0OPEN:
			OnFDOpen(nDrive);
			break;

		// �C�W�F�N�g
		case IDM_D0EJECT:
			OnFDEject(nDrive);
			break;

		// �������ݕی�
		case IDM_D0WRITEP:
			OnFDWriteP(nDrive);
			break;

		// �����C�W�F�N�g
		case IDM_D0FORCE:
			OnFDForce(nDrive);
			break;

		// ��}��
		case IDM_D0INVALID:
			OnFDInvalid(nDrive);
			break;

		// ����ȊO
		default:
			if (uID >= IDM_D0_MRU0) {
				// MRU
				uID -= IDM_D0_MRU0;
				ASSERT(uID <= 8);
				OnFDMRU(nDrive, (int)uID);
			}
			else {
				// Media
				uID -= IDM_D0_MEDIA0;
				ASSERT(uID <= 15);
				OnFDMedia(nDrive, (int)uID);
			}
			break;
	}
}

//---------------------------------------------------------------------------
//
//	�t���b�s�[�I�[�v��
//
//---------------------------------------------------------------------------
void FASTCALL CFrmWnd::OnFDOpen(int nDrive)
{
	Filepath path;
	CString strMsg;
	TCHAR szPath[_MAX_PATH];
	FDI *pFDI;

	ASSERT((nDrive == 0) || (nDrive == 1));
	ASSERT(m_pFDD);

	// �R�����_�C�A���O���s
	memset(szPath, 0, sizeof(szPath));
	if (!::FileOpenDlg(this, szPath, IDS_FDOPEN)) {
		ResetCaption();
		return;
	}
	path.SetPath(szPath);

	/* ACA SE ABREN ARCHIVOS DE FLOPPY (ROMS) */


	CString sz;	
	sz.Format(_T("%s"),  szPath);	
    CString fileName= sz.Mid(sz.ReverseFind('\\')+1);

	RutaCompletaArchivoXM6 = szPath;
	NombreArchivoXM6 = fileName;

			

	// VM���b�N
	::LockVM();

	// �f�B�X�N���蓖��
	if (!m_pFDD->Open(nDrive, path)) {
		GetScheduler()->Reset();
		::UnlockVM();

		// �I�[�v���G���[
		::GetMsg(IDS_FDERR, strMsg);
		MessageBox(strMsg, NULL, MB_ICONSTOP | MB_OK);
		ResetCaption();
		return;
	}

	// VM�����X�^�[�g������O�ɁAFDI���擾���Ă���
	pFDI = m_pFDD->GetFDI(nDrive);

	// ����
	GetScheduler()->Reset();
	ResetCaption();
	::UnlockVM();

	// MRU�ɒǉ�
	GetConfig()->SetMRUFile(nDrive, szPath);

	// �����Ȃ�ABAD�C���[�W�x��
	if (pFDI->GetID() == MAKEID('B', 'A', 'D', ' ')) {
		::GetMsg(IDS_BADFDI_WARNING, strMsg);
		MessageBox(strMsg, NULL, MB_ICONSTOP | MB_OK);
	}
}

//---------------------------------------------------------------------------
//
//	�t���b�s�[�C�W�F�N�g
//
//---------------------------------------------------------------------------
void FASTCALL CFrmWnd::OnFDEject(int nDrive)
{
	ASSERT(m_pFDD);
	ASSERT((nDrive == 0) || (nDrive == 1));

	// VM�����b�N���čs��
	::LockVM();
	m_pFDD->Eject(nDrive, FALSE);
	::UnlockVM();
}

//---------------------------------------------------------------------------
//
//	�t���b�s�[�������ݕی�
//
//---------------------------------------------------------------------------
void FASTCALL CFrmWnd::OnFDWriteP(int nDrive)
{
	ASSERT(m_pFDD);
	ASSERT((nDrive == 0) || (nDrive == 1));

	// �C���[�W�𑀍�
	::LockVM();
	m_pFDD->WriteP(nDrive, !m_pFDD->IsWriteP(nDrive));
	::UnlockVM();
}

//---------------------------------------------------------------------------
//
//	�t���b�s�[�����C�W�F�N�g
//
//---------------------------------------------------------------------------
void FASTCALL CFrmWnd::OnFDForce(int nDrive)
{
	ASSERT(m_pFDD);
	ASSERT((nDrive == 0) || (nDrive == 1));

	// VM�����b�N���čs��
	::LockVM();
	m_pFDD->Eject(nDrive, TRUE);
	::UnlockVM();
}

//---------------------------------------------------------------------------
//
//	�t���b�s�[��}��
//
//---------------------------------------------------------------------------
void FASTCALL CFrmWnd::OnFDInvalid(int nDrive)
{
	ASSERT(m_pFDD);
	ASSERT((nDrive == 0) || (nDrive == 1));

	// VM�����b�N���čs��
	::LockVM();
	m_pFDD->Invalid(nDrive);
	::UnlockVM();
}

//---------------------------------------------------------------------------
//
//	�t���b�s�[���f�B�A
//
//---------------------------------------------------------------------------
void FASTCALL CFrmWnd::OnFDMedia(int nDrive, int nMedia)
{
	Filepath path;

	ASSERT((nDrive == 0) || (nDrive == 1));
	ASSERT((nMedia >= 0) && (nMedia <= 15));

	// VM���b�N
	::LockVM();

	// �O�̂��ߊm�F
	if (nMedia < m_pFDD->GetDisks(nDrive)) {
		m_pFDD->GetPath(nDrive, path);

		// �ăI�[�v��
		m_pFDD->Open(nDrive, path, nMedia);
	}

	// VM�A�����b�N
	::UnlockVM();
}

//---------------------------------------------------------------------------
//
//	�t���b�s�[MRU
//
//---------------------------------------------------------------------------
void FASTCALL CFrmWnd::OnFDMRU(int nDrive, int nMRU)
{
	TCHAR szMRU[_MAX_PATH];
	Filepath path;
	BOOL bResult;
	FDI *pFDI;
	CString strMsg;

	ASSERT((nDrive == 0) || (nDrive == 1));
	ASSERT((nMRU >= 0) && (nMRU <= 8));

	// MRU�擾�A�p�X�쐬
	GetConfig()->GetMRUFile(nDrive, nMRU, szMRU);
	if (szMRU[0] == _T('\0')) {
		return;
	}
	path.SetPath(szMRU);

	CString sz;	
	sz.Format(_T("%s"),  szMRU);	
    CString fileName= sz.Mid(sz.ReverseFind('\\')+1);


	/* Cuando se carga rom desde lista MRU */
	RutaCompletaArchivoXM6 = szMRU;
	NombreArchivoXM6 = fileName;
	

	// VM���b�N
	::LockVM();

	// �f�B�X�N���蓖�Ă����݂�
	bResult = m_pFDD->Open(nDrive, path);
	pFDI = m_pFDD->GetFDI(nDrive);
	GetScheduler()->Reset();
	ResetCaption();

	// VM�A�����b�N
	::UnlockVM();

	// ��������΁A�f�B���N�g���X�V��MRU�ǉ�
	if (bResult) {
		// �f�t�H���g�f�B���N�g���X�V
		Filepath::SetDefaultDir(szMRU);

		// MRU�ɒǉ�
		GetConfig()->SetMRUFile(nDrive, szMRU);

		// BAD�C���[�W�x��
		if (pFDI->GetID() == MAKEID('B', 'A', 'D', ' ')) {
			::GetMsg(IDS_BADFDI_WARNING, strMsg);
			MessageBox(strMsg, NULL, MB_ICONSTOP | MB_OK);
		}
	}
}

//---------------------------------------------------------------------------
//
//	�t���b�s�[�I�[�v�� UI
//
//---------------------------------------------------------------------------
void CFrmWnd::OnFDOpenUI(CCmdUI *pCmdUI)
{
	CMenu *pMenu;
	CMenu *pSubMenu;
	UINT nEnable;
	int nDrive;
	int nStat;
	int nDisks;
	int nMedia;
	char szShort[_MAX_PATH];
	LPTSTR lpszShort;
	int i;
	TCHAR szMRU[_MAX_PATH];
	TCHAR szDrive[_MAX_DRIVE];
	TCHAR szDir[_MAX_DIR];
	TCHAR szFile[_MAX_FNAME];
	TCHAR szExt[_MAX_EXT];

	ASSERT(this);
	ASSERT(m_pFDD);

	// �h���C�u����
	nDrive = 0;
	if (pCmdUI->m_nID >= IDM_D1OPEN) {
		nDrive = 1;
	}

	// �C�W�F�N�g�֎~�ŁA�f�B�X�N����ȊO�̓I�[�v���ł���
	::LockVM();
	nStat = m_pFDD->GetStatus(nDrive);
	m_nFDDStatus[nDrive] = nStat;
	nDisks = m_pFDD->GetDisks(nDrive);
	nMedia = m_pFDD->GetMedia(nDrive);
	::UnlockVM();
	if (!(nStat & FDST_EJECT) && (nStat & FDST_INSERT)) {
		pCmdUI->Enable(FALSE);
	}
	else {
		pCmdUI->Enable(TRUE);
	}

	// �T�u���j���[�擾
	if (m_bPopupMenu) {
		pMenu = m_PopupMenu.GetSubMenu(0);
	}
	else {
		pMenu = &m_Menu;
	}
	// �t�@�C��(F)�̎��Ƀt���b�s�[0�A�t���b�s�[1�ƕ���
	pSubMenu = pMenu->GetSubMenu(nDrive + 1);

	// �C�W�F�N�gUI(�ȉ��AON_UPDATE_COMMAND_UI�̃^�C�~���O�΍�)
	if ((nStat & FDST_INSERT) && (nStat & FDST_EJECT)) {
		pSubMenu->EnableMenuItem(1, MF_BYPOSITION | MF_ENABLED);
	}
	else {
		pSubMenu->EnableMenuItem(1, MF_BYPOSITION | MF_GRAYED);
	}

	// �������ݕی�UI
	if (m_pFDD->IsReadOnly(nDrive) || !(nStat & FDST_INSERT)) {
		pSubMenu->EnableMenuItem(2, MF_BYPOSITION | MF_GRAYED);
	}
	else {
		pSubMenu->EnableMenuItem(2, MF_BYPOSITION | MF_ENABLED);
	}

	// �����C�W�F�N�gUI
	if (!(nStat & FDST_EJECT) && (nStat & FDST_INSERT)) {
		pSubMenu->EnableMenuItem(4, MF_BYPOSITION | MF_ENABLED);
	}
	else {
		pSubMenu->EnableMenuItem(4, MF_BYPOSITION | MF_GRAYED);
	}

	// ��}��UI
	if (!(nStat & FDST_INSERT) && !(nStat & FDST_INVALID)) {
		pSubMenu->EnableMenuItem(5, MF_BYPOSITION | MF_ENABLED);
	}
	else {
		pSubMenu->EnableMenuItem(5, MF_BYPOSITION | MF_GRAYED);
	}

	// �ȍ~�̃��j���[�͂��ׂč폜
	while (pSubMenu->GetMenuItemCount() > 6) {
		pSubMenu->RemoveMenu(6, MF_BYPOSITION);
	}

	// �}���`�f�B�X�N����
	if (nDisks > 1) {
		// �L���E�����萔�ݒ�
		if (!(nStat & FDST_EJECT) && (nStat & FDST_INSERT)) {
			nEnable = MF_BYCOMMAND | MF_ENABLED;
		}
		else {
			nEnable = MF_BYCOMMAND | MF_GRAYED;
		}

		// �Z�p���[�^��}��
		pSubMenu->AppendMenu(MF_SEPARATOR, 0, (LPCTSTR)NULL);

		// ���f�B�A���[�v
		ASSERT(nDisks <= 16);
		for (i=0; i<nDisks; i++) {
			// �f�B�X�N����char*�Ŋi�[����Ă���ׁATCHAR�֕ϊ�
			m_pFDD->GetName(nDrive, szShort, i);
			lpszShort = A2T(szShort);

			// �ǉ�
			if (nDrive == 0) {
				pSubMenu->AppendMenu(MF_STRING, IDM_D0_MEDIA0 + i, lpszShort);
				pSubMenu->EnableMenuItem(IDM_D0_MEDIA0 + i, nEnable);
			}
			else {
				pSubMenu->AppendMenu(MF_STRING, IDM_D1_MEDIA0 + i, lpszShort);
				pSubMenu->EnableMenuItem(IDM_D1_MEDIA0 + i, nEnable);
			}
		}

		// ���W�I�{�^���ݒ�
		if (nDrive == 0) {
			pSubMenu->CheckMenuRadioItem(IDM_D0_MEDIA0, IDM_D0_MEDIAF,
										IDM_D0_MEDIA0 + nMedia, MF_BYCOMMAND);
		}
		else {
			pSubMenu->CheckMenuRadioItem(IDM_D1_MEDIA0, IDM_D1_MEDIAF,
										IDM_D1_MEDIA0 + nMedia, MF_BYCOMMAND);
		}
	}

	// MRU���� - �Z�p���[�^
	if (GetConfig()->GetMRUNum(nDrive) == 0) {
		return;
	}
	pSubMenu->AppendMenu(MF_SEPARATOR, 0, (LPCTSTR)NULL);

	// �L���E�����萔�ݒ�
	if (!(nStat & FDST_EJECT) && (nStat & FDST_INSERT)) {
		nEnable = MF_BYCOMMAND | MF_GRAYED;
	}
	else {
		nEnable = MF_BYCOMMAND | MF_ENABLED;
	}


	

	// MRU���� - �ǉ�
	for (i=0; i<9; i++) {
		// �擾���Ă݂�
		GetConfig()->GetMRUFile(nDrive, i, szMRU);
		if (szMRU[0] == _T('\0')) {
			break;
		}

		// ����΃��j���[�ɒǉ�
		_tsplitpath(szMRU, szDrive, szDir, szFile, szExt);

		


		if (_tcslen(szDir) > 1) {
			_tcscpy(szDir, _T("\\...\\"));
		}
		_stprintf(szMRU, _T("&%d "), i + 1);
		_tcscat(szMRU, szDrive);
		_tcscat(szMRU, szDir);
		_tcscat(szMRU, szFile);
		_tcscat(szMRU, szExt);
		if (nDrive == 0) {
			pSubMenu->AppendMenu(MF_STRING, IDM_D0_MRU0 + i, szMRU);
			pSubMenu->EnableMenuItem(IDM_D0_MRU0 + i, nEnable);
		}
		else {
			pSubMenu->AppendMenu(MF_STRING, IDM_D1_MRU0 + i, szMRU);
			pSubMenu->EnableMenuItem(IDM_D1_MRU0 + i, nEnable);
		}
	}
}

//---------------------------------------------------------------------------
//
//	�t���b�s�[�C�W�F�N�g UI
//
//---------------------------------------------------------------------------
void CFrmWnd::OnFDEjectUI(CCmdUI *pCmdUI)
{
	int nDrive;
	int nStat;

	ASSERT(m_pFDD);

	// �h���C�u����
	nDrive = 0;
	if (pCmdUI->m_nID >= IDM_D1OPEN) {
		nDrive = 1;
	}

	// �X�e�[�^�X�擾
	nStat = m_nFDDStatus[nDrive];

	// �C���T�[�g���ŁA�C�W�F�N�g�֎~�łȂ���΃C�W�F�N�g�ł���
	if ((nStat & FDST_INSERT) && (nStat & FDST_EJECT)) {
		pCmdUI->Enable(TRUE);
		return;
	}
	pCmdUI->Enable(FALSE);
}

//---------------------------------------------------------------------------
//
//	�t���b�s�[�������ݕی� UI
//
//---------------------------------------------------------------------------
void CFrmWnd::OnFDWritePUI(CCmdUI *pCmdUI)
{
	int nDrive;
	int nStat;

	ASSERT(m_pFDD);

	// �h���C�u����
	nDrive = 0;
	if (pCmdUI->m_nID >= IDM_D1OPEN) {
		nDrive = 1;
	}

	// �X�e�[�^�X�擾
	nStat = m_nFDDStatus[nDrive];

	// �������ݕی�ɏ]���`�F�b�N
	pCmdUI->SetCheck(m_pFDD->IsWriteP(nDrive));

	// ���[�h�I�����[���A�C���T�[�g����Ă��Ȃ���Ζ���
	if (m_pFDD->IsReadOnly(nDrive) || !(nStat & FDST_INSERT)) {
		pCmdUI->Enable(FALSE);
		return;
	}
	pCmdUI->Enable(TRUE);
}

//---------------------------------------------------------------------------
//
//	�t���b�s�[�����C�W�F�N�g UI
//
//---------------------------------------------------------------------------
void CFrmWnd::OnFDForceUI(CCmdUI *pCmdUI)
{
	int nDrive;
	int nStat;

	ASSERT(m_pFDD);

	// �h���C�u����
	nDrive = 0;
	if (pCmdUI->m_nID >= IDM_D1OPEN) {
		nDrive = 1;
	}

	// �X�e�[�^�X�擾
	nStat = m_nFDDStatus[nDrive];

	// �C�W�F�N�g�֎~�̎��̂ݗL��
	if (!(nStat & FDST_EJECT) && (nStat & FDST_INSERT)) {
		pCmdUI->Enable(TRUE);
		return;
	}
	pCmdUI->Enable(FALSE);
}

//---------------------------------------------------------------------------
//
//	�t���b�s�[��}�� UI
//
//---------------------------------------------------------------------------
void CFrmWnd::OnFDInvalidUI(CCmdUI *pCmdUI)
{
	int nDrive;
	int nStat;

	ASSERT(m_pFDD);

	// �h���C�u����
	nDrive = 0;
	if (pCmdUI->m_nID >= IDM_D1OPEN) {
		nDrive = 1;
	}

	// �X�e�[�^�X�擾
	nStat = m_nFDDStatus[nDrive];

	// �}������Ă��Ȃ����̂ݗL��
	if (!(nStat & FDST_INSERT) && !(nStat & FDST_INVALID)) {
		pCmdUI->Enable(TRUE);
		return;
	}
	pCmdUI->Enable(FALSE);
}

//---------------------------------------------------------------------------
//
//	�t���b�s�[���f�B�A UI
//
//---------------------------------------------------------------------------
void CFrmWnd::OnFDMediaUI(CCmdUI *pCmdUI)
{
	int nDrive;
	int nStat;

	ASSERT(m_pFDD);

	// �h���C�u����
	nDrive = 0;
	if (pCmdUI->m_nID >= IDM_D1OPEN) {
		nDrive = 1;
	}

	// �X�e�[�^�X�擾
	nStat = m_nFDDStatus[nDrive];

	// �C�W�F�N�g�֎~�ŁA�f�B�X�N����ȊO�̓I�[�v���ł���
	if (!(nStat & FDST_EJECT) && (nStat & FDST_INSERT)) {
		pCmdUI->Enable(FALSE);
	}
	else {
		pCmdUI->Enable(TRUE);
	}
}

//---------------------------------------------------------------------------
//
//	�t���b�s�[MRU UI
//
//---------------------------------------------------------------------------
void CFrmWnd::OnFDMRUUI(CCmdUI *pCmdUI)
{
	int nDrive;
	int nStat;

	ASSERT(m_pFDD);

	// �h���C�u����
	nDrive = 0;
	if (pCmdUI->m_nID >= IDM_D1OPEN) {
		nDrive = 1;
	}

	// �X�e�[�^�X�擾
	nStat = m_nFDDStatus[nDrive];

	// �C�W�F�N�g�֎~�ŁA�f�B�X�N����ȊO�̓I�[�v���ł���
	if (!(nStat & FDST_EJECT) && (nStat & FDST_INSERT)) {
		pCmdUI->Enable(FALSE);
	}
	else {
		pCmdUI->Enable(TRUE);
	}
}

//---------------------------------------------------------------------------
//
//	MO�f�B�X�N�I�[�v��
//
//---------------------------------------------------------------------------
void CFrmWnd::OnMOOpen()
{
	Filepath path;
	TCHAR szPath[_MAX_PATH];
	CString strMsg;

	ASSERT(this);
	ASSERT(m_pSASI);

	// �R�����_�C�A���O���s
	memset(szPath, 0, sizeof(szPath));
	if (!::FileOpenDlg(this, szPath, IDS_MOOPEN)) {
		ResetCaption();
		return;
	}
	path.SetPath(szPath);

	// VM���b�N
	::LockVM();

	// MO�f�B�X�N���蓖��
	if (!m_pSASI->Open(path)) {
		GetScheduler()->Reset();
		::UnlockVM();

		// �I�[�v���G���[
		::GetMsg(IDS_MOERR, strMsg);
		MessageBox(strMsg, NULL, MB_ICONSTOP | MB_OK);
		ResetCaption();
		return;
	}

	// ����
	GetScheduler()->Reset();
	ResetCaption();
	::UnlockVM();

	// MRU�ɒǉ�
	GetConfig()->SetMRUFile(2, szPath);
}

//---------------------------------------------------------------------------
//
//	MO�f�B�X�N�I�[�v�� UI
//
//---------------------------------------------------------------------------
void CFrmWnd::OnMOOpenUI(CCmdUI *pCmdUI)
{
	int i;
	BOOL bValid;
	BOOL bReady;
	BOOL bWriteP;
	BOOL bReadOnly;
	BOOL bLocked;
	TCHAR szMRU[_MAX_PATH];
	TCHAR szDrive[_MAX_DRIVE];
	TCHAR szDir[_MAX_DIR];
	TCHAR szFile[_MAX_FNAME];
	TCHAR szExt[_MAX_EXT];
	CMenu *pMenu;
	CMenu *pSubMenu;
	UINT nEnable;

	ASSERT(this);
	ASSERT(m_pSASI);

	// �h���C�u�̏�Ԃ��擾(���b�N���čs��)
	::LockVM();
	bValid = m_pSASI->IsValid();
	bReady = m_pSASI->IsReady();
	bWriteP = m_pSASI->IsWriteP();
	bReadOnly = m_pSASI->IsReadOnly();
	bLocked = m_pSASI->IsLocked();
	::UnlockVM();

	// �I�[�v��
	if (bValid) {
		if (bReady && bLocked) {
			pCmdUI->Enable(FALSE);
		}
		else {
			pCmdUI->Enable(TRUE);
		}
	}
	else {
		pCmdUI->Enable(FALSE);
	}

	// �T�u���j���[�擾
	if (m_bPopupMenu) {
		pMenu = m_PopupMenu.GetSubMenu(0);
	}
	else {
		pMenu = &m_Menu;
	}
	ASSERT(pMenu);
	// MO���j���[�̓t�@�C���A�t���b�s�[0�A�t���b�s�[1�̎�
	pSubMenu = pMenu->GetSubMenu(3);
	ASSERT(pSubMenu);

	// �C�W�F�N�gUI(�ȉ��AON_UPDATE_COMMAND_UI�̃^�C�~���O�΍�)
	if (bReady && !bLocked) {
		pSubMenu->EnableMenuItem(1, MF_BYPOSITION | MF_ENABLED);
	}
	else {
		pSubMenu->EnableMenuItem(1, MF_BYPOSITION | MF_GRAYED);
	}

	// �������ݕی�UI
	if (bReady && !bReadOnly) {
		pSubMenu->EnableMenuItem(2, MF_BYPOSITION | MF_GRAYED);
	}
	else {
		pSubMenu->EnableMenuItem(2, MF_BYPOSITION | MF_ENABLED);
	}

	// �����C�W�F�N�gUI
	if (bReady && bLocked) {
		pSubMenu->EnableMenuItem(4, MF_BYPOSITION | MF_ENABLED);
	}
	else {
		pSubMenu->EnableMenuItem(4, MF_BYPOSITION | MF_GRAYED);
	}

	// �ȍ~�̃��j���[�͂��ׂč폜
	while (pSubMenu->GetMenuItemCount() > 5) {
		pSubMenu->RemoveMenu(5, MF_BYPOSITION);
	}

	// MRU���� - �Z�p���[�^
	if (GetConfig()->GetMRUNum(2) == 0) {
		return;
	}
	pSubMenu->AppendMenu(MF_SEPARATOR, 0, (LPCTSTR)NULL);

	// �L���E�����萔�ݒ�
	nEnable = MF_BYCOMMAND | MF_GRAYED;
	if (bValid) {
		// �h���C�u�L����
		if (!bReady || !bLocked) {
			// ���f�B�A�����݂��Ȃ� or ���b�N����Ă��Ȃ� �Ȃ�C���T�[�g�ł���
			nEnable = MF_BYCOMMAND | MF_ENABLED;
		}
	}

	// MRU���� - �ǉ�
	for (i=0; i<9; i++) {
		// �擾���Ă݂�
		GetConfig()->GetMRUFile(2, i, szMRU);
		if (szMRU[0] == _T('\0')) {
			break;
		}

		// ����΃��j���[�ɒǉ�
		_tsplitpath(szMRU, szDrive, szDir, szFile, szExt);
		if (_tcslen(szDir) > 1) {
			_tcscpy(szDir, _T("\\...\\"));
		}
		_stprintf(szMRU, _T("&%d "), i + 1);
		_tcscat(szMRU, szDrive);
		_tcscat(szMRU, szDir);
		_tcscat(szMRU, szFile);
		_tcscat(szMRU, szExt);

		pSubMenu->AppendMenu(MF_STRING, IDM_MO_MRU0 + i, szMRU);
		pSubMenu->EnableMenuItem(IDM_MO_MRU0 + i, nEnable);
	}
}

//---------------------------------------------------------------------------
//
//	MO�f�B�X�N�C�W�F�N�g
//
//---------------------------------------------------------------------------
void CFrmWnd::OnMOEject()
{
	ASSERT(this);
	ASSERT(m_pSASI);

	// VM�����b�N���čs��
	::LockVM();
	m_pSASI->Eject(FALSE);
	::UnlockVM();
}

//---------------------------------------------------------------------------
//
//	MO�f�B�X�N�C�W�F�N�g UI
//
//---------------------------------------------------------------------------
void CFrmWnd::OnMOEjectUI(CCmdUI *pCmdUI)
{
	ASSERT(this);
	ASSERT(m_pSASI);

	// ���f�B�łȂ���΋֎~
	if (!m_pSASI->IsReady()) {
		pCmdUI->Enable(FALSE);
		return;
	}

	// ���b�N����Ă���΋֎~
	if (m_pSASI->IsLocked()) {
		pCmdUI->Enable(FALSE);
		return;
	}

	// ����
	pCmdUI->Enable(TRUE);
}

//---------------------------------------------------------------------------
//
//	MO�f�B�X�N�������ݕی�
//
//---------------------------------------------------------------------------
void CFrmWnd::OnMOWriteP()
{
	ASSERT(this);
	ASSERT(m_pSASI);

	// VM�����b�N���čs��
	::LockVM();
	m_pSASI->WriteP(!m_pSASI->IsWriteP());
	::UnlockVM();
}

//---------------------------------------------------------------------------
//
//	MO�f�B�X�N�������ݕی� UI
//
//---------------------------------------------------------------------------
void CFrmWnd::OnMOWritePUI(CCmdUI *pCmdUI)
{
	ASSERT(this);
	ASSERT(m_pSASI);

	// ���f�B�łȂ���΃`�F�b�N�Ȃ��A�֎~
	if (!m_pSASI->IsReady()) {
		pCmdUI->SetCheck(0);
		pCmdUI->Enable(FALSE);
		return;
	}

	// Read Only�ł���΃`�F�b�N����A�֎~
	if (m_pSASI->IsReadOnly()) {
		pCmdUI->SetCheck(1);
		pCmdUI->Enable(FALSE);
		return;
	}

	// ����ɉ����ă`�F�b�N���āA����
	pCmdUI->SetCheck(m_pSASI->IsWriteP());
	pCmdUI->Enable(TRUE);
}

//---------------------------------------------------------------------------
//
//	MO�f�B�X�N�����C�W�F�N�g
//
//---------------------------------------------------------------------------
void CFrmWnd::OnMOForce()
{
	ASSERT(this);
	ASSERT(m_pSASI);

	// VM�����b�N���čs��
	::LockVM();
	m_pSASI->Eject(TRUE);
	::UnlockVM();
}

//---------------------------------------------------------------------------
//
//	MO�f�B�X�N�����C�W�F�N�g UI
//
//---------------------------------------------------------------------------
void CFrmWnd::OnMOForceUI(CCmdUI *pCmdUI)
{
	ASSERT(this);
	ASSERT(m_pSASI);

	// ���f�B�łȂ���΋֎~
	if (!m_pSASI->IsReady()) {
		pCmdUI->Enable(FALSE);
		return;
	}

	// ���b�N����Ă��Ȃ���΋֎~
	if (!m_pSASI->IsLocked()) {
		pCmdUI->Enable(FALSE);
		return;
	}

	// ����
	pCmdUI->Enable(TRUE);
}

//---------------------------------------------------------------------------
//
//	MO�f�B�X�NMRU
//
//---------------------------------------------------------------------------
void CFrmWnd::OnMOMRU(UINT uID)
{
	int nMRU;
	TCHAR szMRU[_MAX_PATH];
	Filepath path;
	BOOL bResult;

	ASSERT(this);
	ASSERT(m_pSASI);
	ASSERT((uID >= IDM_MO_MRU0) && (uID <= IDM_MO_MRU8));

	// �C���f�b�N�X�쐬
	nMRU = uID - IDM_MO_MRU0;
	ASSERT((nMRU >= 0) && (nMRU <= 8));

	// MRU�擾
	GetConfig()->GetMRUFile(2, nMRU, szMRU);
	if (szMRU[0] == _T('\0')) {
		return;
	}
	path.SetPath(szMRU);

	// VM���b�N
	::LockVM();

	// �I�[�v��
	bResult = m_pSASI->Open(path);
	GetScheduler()->Reset();
	ResetCaption();
	::UnlockVM();

	// ��������΁A�f�B���N�g���X�V��MRU�ǉ�
	if (bResult) {
		// �C�j�V�����f�B���N�g���X�V
		Filepath::SetDefaultDir(szMRU);

		// MRU�ɒǉ�
		GetConfig()->SetMRUFile(2, szMRU);
	}
}

//---------------------------------------------------------------------------
//
//	MO�f�B�X�NMRU UI
//
//---------------------------------------------------------------------------
void CFrmWnd::OnMOMRUUI(CCmdUI *pCmdUI)
{
	ASSERT(this);
	ASSERT(m_pSASI);

	// �h���C�u���L���łȂ��Ζ���
	if (!m_pSASI->IsValid()) {
		pCmdUI->Enable(FALSE);
		return;
	}

	// ���b�N����Ă���΋֎~
	if (m_pSASI->IsLocked()) {
		pCmdUI->Enable(FALSE);
		return;
	}

	// ����
	pCmdUI->Enable(TRUE);
}

//---------------------------------------------------------------------------
//
//	CD-ROM�I�[�v��
//
//---------------------------------------------------------------------------
void CFrmWnd::OnCDOpen()
{
	Filepath path;
	TCHAR szPath[_MAX_PATH];
	CString strMsg;

	ASSERT(this);
	ASSERT(m_pSCSI);

	// �R�����_�C�A���O���s
	memset(szPath, 0, sizeof(szPath));
	if (!::FileOpenDlg(this, szPath, IDS_CDOPEN)) {
		ResetCaption();
		return;
	}
	path.SetPath(szPath);

	// VM���b�N
	::LockVM();

	// CD�f�B�X�N���蓖��
	if (!m_pSCSI->Open(path, FALSE)) {
		GetScheduler()->Reset();
		::UnlockVM();

		// �I�[�v���G���[
		::GetMsg(IDS_CDERR, strMsg);
		MessageBox(strMsg, NULL, MB_ICONSTOP | MB_OK);
		ResetCaption();
		return;
	}

	// ����
	GetScheduler()->Reset();
	ResetCaption();
	::UnlockVM();

	// MRU�ɒǉ�
	GetConfig()->SetMRUFile(3, szPath);
}

//---------------------------------------------------------------------------
//
//	CD-ROM�I�[�v�� UI
//
//---------------------------------------------------------------------------
void CFrmWnd::OnCDOpenUI(CCmdUI *pCmdUI)
{
	int i;
	BOOL bValid;
	BOOL bReady;
	BOOL bLocked;
	TCHAR szMRU[_MAX_PATH];
	TCHAR szDrive[_MAX_DRIVE];
	TCHAR szDir[_MAX_DIR];
	TCHAR szFile[_MAX_FNAME];
	TCHAR szExt[_MAX_EXT];
	CMenu *pMenu;
	CMenu *pSubMenu;
	UINT nEnable;

	ASSERT(this);
	ASSERT(m_pSCSI);

	// �h���C�u�̏�Ԃ��擾(���b�N���čs��)
	::LockVM();
	bValid = m_pSCSI->IsValid(FALSE);
	bReady = m_pSCSI->IsReady(FALSE);
	bLocked = m_pSCSI->IsLocked(FALSE);
	::UnlockVM();

	// �I�[�v��
	if (bValid) {
		if (bReady && bLocked) {
			pCmdUI->Enable(FALSE);
		}
		else {
			pCmdUI->Enable(TRUE);
		}
	}
	else {
		pCmdUI->Enable(FALSE);
	}

	// �T�u���j���[�擾
	if (m_bPopupMenu) {
		pMenu = m_PopupMenu.GetSubMenu(0);
	}
	else {
		pMenu = &m_Menu;
	}
	ASSERT(pMenu);

	// CD���j���[�̓t�@�C���A�t���b�s�[0�A�t���b�s�[1�AMO�̎�
	pSubMenu = pMenu->GetSubMenu(4);
	ASSERT(pSubMenu);

	// �C�W�F�N�gUI(�ȉ��AON_UPDATE_COMMAND_UI�̃^�C�~���O�΍�)
	if (bReady && !bLocked) {
		pSubMenu->EnableMenuItem(1, MF_BYPOSITION | MF_ENABLED);
	}
	else {
		pSubMenu->EnableMenuItem(1, MF_BYPOSITION | MF_GRAYED);
	}

	// �����C�W�F�N�gUI
	if (bReady && bLocked) {
		pSubMenu->EnableMenuItem(3, MF_BYPOSITION | MF_ENABLED);
	}
	else {
		pSubMenu->EnableMenuItem(3, MF_BYPOSITION | MF_GRAYED);
	}

	// �ȍ~�̃��j���[�͂��ׂč폜
	while (pSubMenu->GetMenuItemCount() > 4) {
		pSubMenu->RemoveMenu(4, MF_BYPOSITION);
	}

	// MRU���� - �Z�p���[�^
	if (GetConfig()->GetMRUNum(3) == 0) {
		return;
	}
	pSubMenu->AppendMenu(MF_SEPARATOR, 0, (LPCTSTR)NULL);

	// �L���E�����萔�ݒ�
	nEnable = MF_BYCOMMAND | MF_GRAYED;
	if (bValid) {
		// �h���C�u�L����
		if (!bReady || !bLocked) {
			// ���f�B�A�����݂��Ȃ� or ���b�N����Ă��Ȃ� �Ȃ�C���T�[�g�ł���
			nEnable = MF_BYCOMMAND | MF_ENABLED;
		}
	}

	// MRU���� - �ǉ�
	for (i=0; i<9; i++) {
		// �擾���Ă݂�
		GetConfig()->GetMRUFile(3, i, szMRU);
		if (szMRU[0] == _T('\0')) {
			break;
		}

		// ����΃��j���[�ɒǉ�
		_tsplitpath(szMRU, szDrive, szDir, szFile, szExt);
		if (_tcslen(szDir) > 1) {
			_tcscpy(szDir, _T("\\...\\"));
		}
		_stprintf(szMRU, _T("&%d "), i + 1);
		_tcscat(szMRU, szDrive);
		_tcscat(szMRU, szDir);
		_tcscat(szMRU, szFile);
		_tcscat(szMRU, szExt);

		pSubMenu->AppendMenu(MF_STRING, IDM_CD_MRU0 + i, szMRU);
		pSubMenu->EnableMenuItem(IDM_CD_MRU0 + i, nEnable);
	}
}

//---------------------------------------------------------------------------
//
//	CD-ROM�C�W�F�N�g
//
//---------------------------------------------------------------------------
void CFrmWnd::OnCDEject()
{
	ASSERT(this);
	ASSERT(m_pSCSI);

	// VM�����b�N���čs��
	::LockVM();
	m_pSCSI->Eject(FALSE, FALSE);
	::UnlockVM();
}

//---------------------------------------------------------------------------
//
//	CD-ROM�C�W�F�N�g UI
//
//---------------------------------------------------------------------------
void CFrmWnd::OnCDEjectUI(CCmdUI *pCmdUI)
{
	ASSERT(this);
	ASSERT(m_pSCSI);

	// ���f�B�łȂ���΋֎~
	if (!m_pSCSI->IsReady(FALSE)) {
		pCmdUI->Enable(FALSE);
		return;
	}

	// ���b�N����Ă���΋֎~
	if (m_pSCSI->IsLocked(FALSE)) {
		pCmdUI->Enable(FALSE);
		return;
	}

	// ����
	pCmdUI->Enable(TRUE);
}

//---------------------------------------------------------------------------
//
//	CD-ROM�����C�W�F�N�g
//
//---------------------------------------------------------------------------
void CFrmWnd::OnCDForce()
{
	ASSERT(this);
	ASSERT(m_pSCSI);

	// VM�����b�N���čs��
	::LockVM();
	m_pSCSI->Eject(TRUE, FALSE);
	::UnlockVM();
}

//---------------------------------------------------------------------------
//
//	CD-ROM�����C�W�F�N�g UI
//
//---------------------------------------------------------------------------
void CFrmWnd::OnCDForceUI(CCmdUI *pCmdUI)
{
	ASSERT(this);
	ASSERT(m_pSCSI);

	// ���f�B�łȂ���΋֎~
	if (!m_pSCSI->IsReady(FALSE)) {
		pCmdUI->Enable(FALSE);
		return;
	}

	// ���b�N����Ă��Ȃ���΋֎~
	if (!m_pSCSI->IsLocked(FALSE)) {
		pCmdUI->Enable(FALSE);
		return;
	}

	// ����
	pCmdUI->Enable(TRUE);
}

//---------------------------------------------------------------------------
//
//	CD-ROM MRU
//
//---------------------------------------------------------------------------
void CFrmWnd::OnCDMRU(UINT uID)
{
	int nMRU;
	TCHAR szMRU[_MAX_PATH];
	Filepath path;
	BOOL bResult;

	ASSERT(this);
	ASSERT(m_pSCSI);
	ASSERT((uID >= IDM_CD_MRU0) && (uID <= IDM_CD_MRU8));

	// �C���f�b�N�X�쐬
	nMRU = uID - IDM_CD_MRU0;
	ASSERT((nMRU >= 0) && (nMRU <= 8));

	// MRU�擾
	GetConfig()->GetMRUFile(3, nMRU, szMRU);
	if (szMRU[0] == _T('\0')) {
		return;
	}
	path.SetPath(szMRU);

	// VM���b�N
	::LockVM();

	// �I�[�v��
	bResult = m_pSCSI->Open(path, FALSE);
	GetScheduler()->Reset();
	ResetCaption();
	::UnlockVM();

	// ��������΁A�f�B���N�g���X�V��MRU�ǉ�
	if (bResult) {
		// �C�j�V�����f�B���N�g���X�V
		Filepath::SetDefaultDir(szMRU);

		// MRU�ɒǉ�
		GetConfig()->SetMRUFile(3, szMRU);
	}
}

//---------------------------------------------------------------------------
//
//	CD-ROM MRU UI
//
//---------------------------------------------------------------------------
void CFrmWnd::OnCDMRUUI(CCmdUI *pCmdUI)
{
	ASSERT(this);
	ASSERT(m_pSCSI);

	// �h���C�u���L���łȂ��Ζ���
	if (!m_pSCSI->IsValid(FALSE)) {
		pCmdUI->Enable(FALSE);
		return;
	}

	// ���b�N����Ă���΋֎~
	if (m_pSCSI->IsLocked(FALSE)) {
		pCmdUI->Enable(FALSE);
		return;
	}

	// ����
	pCmdUI->Enable(TRUE);
}

//---------------------------------------------------------------------------
//
//	�T�u�E�B���h�E �R�}���h�EUI�}�N��
//
//---------------------------------------------------------------------------
#define ON_SUB_WINDOW(id, wndcls)	do { \
									CSubWnd *pWnd = GetView()->SearchSWnd(id); \
									if (pWnd) { pWnd->DestroyWindow(); return; } \
									wndcls *pNewWnd = new wndcls; \
									pNewWnd->Init(GetView()); \
									} while (0)
#define ON_UPDATE_SUB_WINDOW(id)	if (GetView()->SearchSWnd(id)) pCmdUI->SetCheck(1); \
									else pCmdUI->SetCheck(0);

//---------------------------------------------------------------------------
//
//	���O
//
//---------------------------------------------------------------------------
void CFrmWnd::OnLog()
{
	ON_SUB_WINDOW(MAKEID('L', 'O', 'G', 'L'), CLogWnd);
}

//---------------------------------------------------------------------------
//
//	���O UI
//
//---------------------------------------------------------------------------
void CFrmWnd::OnLogUI(CCmdUI *pCmdUI)
{
	ON_UPDATE_SUB_WINDOW(MAKEID('L', 'O', 'G', 'L'));
}

//---------------------------------------------------------------------------
//
//	�X�P�W���[��
//
//---------------------------------------------------------------------------
void CFrmWnd::OnScheduler()
{
	ON_SUB_WINDOW(MAKEID('S', 'C', 'H', 'E'), CSchedulerWnd);
}

//---------------------------------------------------------------------------
//
//	�X�P�W���[�� UI
//
//---------------------------------------------------------------------------
void CFrmWnd::OnSchedulerUI(CCmdUI *pCmdUI)
{
	ON_UPDATE_SUB_WINDOW(MAKEID('S', 'C', 'H', 'E'));
}

//---------------------------------------------------------------------------
//
//	�f�o�C�X
//
//---------------------------------------------------------------------------
void CFrmWnd::OnDevice()
{
	ON_SUB_WINDOW(MAKEID('D', 'E', 'V', 'I'), CDeviceWnd);
}

//---------------------------------------------------------------------------
//
//	�f�o�C�X UI
//
//---------------------------------------------------------------------------
void CFrmWnd::OnDeviceUI(CCmdUI *pCmdUI)
{
	ON_UPDATE_SUB_WINDOW(MAKEID('D', 'E', 'V', 'I'));
}

//---------------------------------------------------------------------------
//
//	CPU���W�X�^
//
//---------------------------------------------------------------------------
void CFrmWnd::OnCPUReg()
{
	ON_SUB_WINDOW(MAKEID('M', 'P', 'U', 'R'), CCPURegWnd);
}

//---------------------------------------------------------------------------
//
//	CPU���W�X�^ UI
//
//---------------------------------------------------------------------------
void CFrmWnd::OnCPURegUI(CCmdUI *pCmdUI)
{
	ON_UPDATE_SUB_WINDOW(MAKEID('M', 'P', 'U', 'R'));
}

//---------------------------------------------------------------------------
//
//	���荞��
//
//---------------------------------------------------------------------------
void CFrmWnd::OnInt()
{
	ON_SUB_WINDOW(MAKEID('I', 'N', 'T', ' '), CIntWnd);
}

//---------------------------------------------------------------------------
//
//	���荞�� UI
//
//---------------------------------------------------------------------------
void CFrmWnd::OnIntUI(CCmdUI *pCmdUI)
{
	ON_UPDATE_SUB_WINDOW(MAKEID('I', 'N', 'T', ' '));
}

//---------------------------------------------------------------------------
//
//	�t�A�Z���u��
//
//---------------------------------------------------------------------------
void CFrmWnd::OnDisasm()
{
	CDisasmWnd *pWnd;
	int i;
	BOOL flag;

	// 8��ނ��ׂă`�F�b�N����
	flag = FALSE;
	for (i=0; i<8; i++) {
		pWnd = (CDisasmWnd*)GetView()->SearchSWnd(MAKEID('D', 'I', 'S', 'A' + i));
		if (pWnd) {
			pWnd->PostMessage(WM_CLOSE, 0, 0);
			flag = TRUE;
		}
	}

	// �V�K�쐬
	if (!flag) {
		pWnd = new CDisasmWnd(0);
		VERIFY(pWnd->Init(GetView()));
	}
}

//---------------------------------------------------------------------------
//
//	�t�A�Z���u�� UI
//
//---------------------------------------------------------------------------
void CFrmWnd::OnDisasmUI(CCmdUI *pCmdUI)
{
	CSubWnd *pSubWnd;
	int i;

	// 8��ނ��ׂă`�F�b�N����
	for (i=0; i<8; i++) {
		pSubWnd = GetView()->SearchSWnd(MAKEID('D', 'I', 'S', 'A' + i));
		if (pSubWnd) {
			pCmdUI->SetCheck(1);
			return;
		}
	}

	pCmdUI->SetCheck(0);
}

//---------------------------------------------------------------------------
//
//	������
//
//---------------------------------------------------------------------------
void CFrmWnd::OnMemory()
{
	CMemoryWnd *pWnd;
	int i;
	BOOL flag;

	// 8��ނ��ׂă`�F�b�N����
	flag = FALSE;
	for (i=0; i<8; i++) {
		pWnd = (CMemoryWnd*)GetView()->SearchSWnd(MAKEID('M', 'E', 'M', 'A' + i));
		if (pWnd) {
			pWnd->PostMessage(WM_CLOSE, 0, 0);
			flag = TRUE;
		}
	}

	// �V�K�쐬
	if (!flag) {
		pWnd = new CMemoryWnd(0);
		VERIFY(pWnd->Init(GetView()));
	}
}

//---------------------------------------------------------------------------
//
//	������ UI
//
//---------------------------------------------------------------------------
void CFrmWnd::OnMemoryUI(CCmdUI *pCmdUI)
{
	CSubWnd *pSubWnd;
	int i;

	// 8��ނ��ׂă`�F�b�N����
	for (i=0; i<8; i++) {
		pSubWnd = GetView()->SearchSWnd(MAKEID('M', 'E', 'M', 'A' + i));
		if (pSubWnd) {
			pCmdUI->SetCheck(1);
			return;
		}
	}

	pCmdUI->SetCheck(0);
}

//---------------------------------------------------------------------------
//
//	�u���[�N�|�C���g
//
//---------------------------------------------------------------------------
void CFrmWnd::OnBreakP()
{
	ON_SUB_WINDOW(MAKEID('B', 'R', 'K', 'P'), CBreakPWnd);
}

//---------------------------------------------------------------------------
//
//	�u���[�N�|�C���g UI
//
//---------------------------------------------------------------------------
void CFrmWnd::OnBreakPUI(CCmdUI *pCmdUI)
{
	ON_UPDATE_SUB_WINDOW(MAKEID('B', 'R', 'K', 'P'));
}

//---------------------------------------------------------------------------
//
//	MFP
//
//---------------------------------------------------------------------------
void CFrmWnd::OnMFP()
{
	ON_SUB_WINDOW(MAKEID('M', 'F', 'P', ' '), CMFPWnd);
}

//---------------------------------------------------------------------------
//
//	MFP UI
//
//---------------------------------------------------------------------------
void CFrmWnd::OnMFPUI(CCmdUI *pCmdUI)
{
	ON_UPDATE_SUB_WINDOW(MAKEID('M', 'F', 'P', ' '));
}

//---------------------------------------------------------------------------
//
//	DMAC
//
//---------------------------------------------------------------------------
void CFrmWnd::OnDMAC()
{
	ON_SUB_WINDOW(MAKEID('D', 'M', 'A', 'C'), CDMACWnd);
}

//---------------------------------------------------------------------------
//
//	DMAC UI
//
//---------------------------------------------------------------------------
void CFrmWnd::OnDMACUI(CCmdUI *pCmdUI)
{
	ON_UPDATE_SUB_WINDOW(MAKEID('D', 'M', 'A', 'C'));
}

//---------------------------------------------------------------------------
//
//	CRTC
//
//---------------------------------------------------------------------------
void CFrmWnd::OnCRTC()
{
	ON_SUB_WINDOW(MAKEID('C', 'R', 'T', 'C'), CCRTCWnd);
}

//---------------------------------------------------------------------------
//
//	CRTC UI
//
//---------------------------------------------------------------------------
void CFrmWnd::OnCRTCUI(CCmdUI *pCmdUI)
{
	ON_UPDATE_SUB_WINDOW(MAKEID('C', 'R', 'T', 'C'));
}

//---------------------------------------------------------------------------
//
//	VC
//
//---------------------------------------------------------------------------
void CFrmWnd::OnVC()
{
	ON_SUB_WINDOW(MAKEID('V', 'C', ' ', ' '), CVCWnd);
}

//---------------------------------------------------------------------------
//
//	VC UI
//
//---------------------------------------------------------------------------
void CFrmWnd::OnVCUI(CCmdUI *pCmdUI)
{
	ON_UPDATE_SUB_WINDOW(MAKEID('V', 'C', ' ', ' '));
}

//---------------------------------------------------------------------------
//
//	RTC
//
//---------------------------------------------------------------------------
void CFrmWnd::OnRTC()
{
	ON_SUB_WINDOW(MAKEID('R', 'T', 'C', ' '), CRTCWnd);
}

//---------------------------------------------------------------------------
//
//	RTC UI
//
//---------------------------------------------------------------------------
void CFrmWnd::OnRTCUI(CCmdUI *pCmdUI)
{
	ON_UPDATE_SUB_WINDOW(MAKEID('R', 'T', 'C', ' '));
}

//---------------------------------------------------------------------------
//
//	OPM
//
//---------------------------------------------------------------------------
void CFrmWnd::OnOPM()
{
	ON_SUB_WINDOW(MAKEID('O', 'P', 'M', ' '), COPMWnd);
}

//---------------------------------------------------------------------------
//
//	OPM UI
//
//---------------------------------------------------------------------------
void CFrmWnd::OnOPMUI(CCmdUI *pCmdUI)
{
	ON_UPDATE_SUB_WINDOW(MAKEID('O', 'P', 'M', ' '));
}

//---------------------------------------------------------------------------
//
//	�L�[�{�[�h
//
//---------------------------------------------------------------------------
void CFrmWnd::OnKeyboard()
{
	ON_SUB_WINDOW(MAKEID('K', 'E', 'Y', 'B'), CKeyboardWnd);
}

//---------------------------------------------------------------------------
//
//	�L�[�{�[�h UI
//
//---------------------------------------------------------------------------
void CFrmWnd::OnKeyboardUI(CCmdUI *pCmdUI)
{
	ON_UPDATE_SUB_WINDOW(MAKEID('K', 'E', 'Y', 'B'));
}

//---------------------------------------------------------------------------
//
//	FDD
//
//---------------------------------------------------------------------------
void CFrmWnd::OnFDD()
{
	ON_SUB_WINDOW(MAKEID('F', 'D', 'D', ' '), CFDDWnd);
}

//---------------------------------------------------------------------------
//
//	FDD UI
//
//---------------------------------------------------------------------------
void CFrmWnd::OnFDDUI(CCmdUI *pCmdUI)
{
	ON_UPDATE_SUB_WINDOW(MAKEID('F', 'D', 'D', ' '));
}

//---------------------------------------------------------------------------
//
//	FDC
//
//---------------------------------------------------------------------------
void CFrmWnd::OnFDC()
{
	ON_SUB_WINDOW(MAKEID('F', 'D', 'C', ' '), CFDCWnd);
}

//---------------------------------------------------------------------------
//
//	FDC UI
//
//---------------------------------------------------------------------------
void CFrmWnd::OnFDCUI(CCmdUI *pCmdUI)
{
	ON_UPDATE_SUB_WINDOW(MAKEID('F', 'D', 'C', ' '));
}

//---------------------------------------------------------------------------
//
//	SCC
//
//---------------------------------------------------------------------------
void CFrmWnd::OnSCC()
{
	ON_SUB_WINDOW(MAKEID('S', 'C', 'C', ' '), CSCCWnd);
}

//---------------------------------------------------------------------------
//
//	SCC UI
//
//---------------------------------------------------------------------------
void CFrmWnd::OnSCCUI(CCmdUI *pCmdUI)
{
	ON_UPDATE_SUB_WINDOW(MAKEID('S', 'C', 'C', ' '));
}

//---------------------------------------------------------------------------
//
//	CYNTHIA
//
//---------------------------------------------------------------------------
void CFrmWnd::OnCynthia()
{
	ON_SUB_WINDOW(MAKEID('C', 'Y', 'N', 'T'), CCynthiaWnd);
}

//---------------------------------------------------------------------------
//
//	CYNTHIA UI
//
//---------------------------------------------------------------------------
void CFrmWnd::OnCynthiaUI(CCmdUI *pCmdUI)
{
	ON_UPDATE_SUB_WINDOW(MAKEID('C', 'Y', 'N', 'T'));
}

//---------------------------------------------------------------------------
//
//	SASI
//
//---------------------------------------------------------------------------
void CFrmWnd::OnSASI()
{
	ON_SUB_WINDOW(MAKEID('S', 'A', 'S', 'I'), CSASIWnd);
}

//---------------------------------------------------------------------------
//
//	SASI UI
//
//---------------------------------------------------------------------------
void CFrmWnd::OnSASIUI(CCmdUI *pCmdUI)
{
	ON_UPDATE_SUB_WINDOW(MAKEID('S', 'A', 'S', 'I'));
}

//---------------------------------------------------------------------------
//
//	MIDI
//
//---------------------------------------------------------------------------
void CFrmWnd::OnMIDI()
{
	ON_SUB_WINDOW(MAKEID('M', 'I', 'D', 'I'), CMIDIWnd);
}

//---------------------------------------------------------------------------
//
//	MIDI UI
//
//---------------------------------------------------------------------------
void CFrmWnd::OnMIDIUI(CCmdUI *pCmdUI)
{
	ON_UPDATE_SUB_WINDOW(MAKEID('M', 'I', 'D', 'I'));
}

//---------------------------------------------------------------------------
//
//	SCSI
//
//---------------------------------------------------------------------------
void CFrmWnd::OnSCSI()
{
	ON_SUB_WINDOW(MAKEID('S', 'C', 'S', 'I'), CSCSIWnd);
}

//---------------------------------------------------------------------------
//
//	SCSI UI
//
//---------------------------------------------------------------------------
void CFrmWnd::OnSCSIUI(CCmdUI *pCmdUI)
{
	ON_UPDATE_SUB_WINDOW(MAKEID('S', 'C', 'S', 'I'));
}

//---------------------------------------------------------------------------
//
//	�e�L�X�g���
//
//---------------------------------------------------------------------------
void CFrmWnd::OnTVRAM()
{
	ON_SUB_WINDOW(MAKEID('T', 'V', 'R', 'M'), CTVRAMWnd);
}

//---------------------------------------------------------------------------
//
//	�e�L�X�g��� UI
//
//---------------------------------------------------------------------------
void CFrmWnd::OnTVRAMUI(CCmdUI *pCmdUI)
{
	ON_UPDATE_SUB_WINDOW(MAKEID('T', 'V', 'R', 'M'));
}

//---------------------------------------------------------------------------
//
//	�O���t�B�b�N���1024�~1024
//
//---------------------------------------------------------------------------
void CFrmWnd::OnG1024()
{
	ON_SUB_WINDOW(MAKEID('G', '1', '0', '2'), CG1024Wnd);
}

//---------------------------------------------------------------------------
//
//	�O���t�B�b�N���1024�~1024 UI
//
//---------------------------------------------------------------------------
void CFrmWnd::OnG1024UI(CCmdUI *pCmdUI)
{
	ON_UPDATE_SUB_WINDOW(MAKEID('G', '1', '0', '2'));
}

//---------------------------------------------------------------------------
//
//	�O���t�B�b�N���16�F
//
//---------------------------------------------------------------------------
void CFrmWnd::OnG16(UINT uID)
{
	CG16Wnd *pWnd;
	int index;

	// �C���f�b�N�X����
	index = (int)(uID - IDM_G16P0);
	ASSERT((index >= 0) || (index <= 3));

	// ���݂���Ώ���
	pWnd = (CG16Wnd*)GetView()->SearchSWnd(MAKEID('G', '1', '6', ('A' + index)));
	if (pWnd) {
		pWnd->PostMessage(WM_CLOSE, 0, 0);
		return;
	}

	// �T�u�E�B���h�E�쐬
	pWnd = new CG16Wnd(index);
	VERIFY(pWnd->Init(GetView()));
}

//---------------------------------------------------------------------------
//
//	�O���t�B�b�N���16�F UI
//
//---------------------------------------------------------------------------
void CFrmWnd::OnG16UI(CCmdUI *pCmdUI)
{
	int index;
	CSubWnd *pSubWnd;

	// �C���f�b�N�X����
	index = (int)(pCmdUI->m_nID - IDM_G16P0);
	ASSERT((index >= 0) || (index <= 3));

	pSubWnd = GetView()->SearchSWnd(MAKEID('G', '1', '6', ('A' + index)));
	if (pSubWnd) {
		pCmdUI->SetCheck(1);
	}
	else {
		pCmdUI->SetCheck(0);
	}
}

//---------------------------------------------------------------------------
//
//	�O���t�B�b�N���256�F
//
//---------------------------------------------------------------------------
void CFrmWnd::OnG256(UINT uID)
{
	CG256Wnd *pWnd;
	int index;

	// �C���f�b�N�X����
	index = (int)(uID - IDM_G256P0);
	ASSERT((index == 0) || (index == 1));

	// ���݂���Ώ���
	pWnd = (CG256Wnd*)GetView()->SearchSWnd(MAKEID('G', '2', '5', ('A' + index)));
	if (pWnd) {
		pWnd->PostMessage(WM_CLOSE, 0, 0);
		return;
	}

	// �T�u�E�B���h�E�쐬
	pWnd = new CG256Wnd(index);
	VERIFY(pWnd->Init(GetView()));
}

//---------------------------------------------------------------------------
//
//	�O���t�B�b�N���256�F UI
//
//---------------------------------------------------------------------------
void CFrmWnd::OnG256UI(CCmdUI *pCmdUI)
{
	int index;
	CSubWnd *pSubWnd;

	// �C���f�b�N�X����
	index = (int)(pCmdUI->m_nID - IDM_G256P0);
	ASSERT((index == 0) || (index == 1));

	pSubWnd = GetView()->SearchSWnd(MAKEID('G', '2', '5', ('A' + index)));
	if (pSubWnd) {
		pCmdUI->SetCheck(1);
	}
	else {
		pCmdUI->SetCheck(0);
	}
}

//---------------------------------------------------------------------------
//
//	�O���t�B�b�N���65536�F
//
//---------------------------------------------------------------------------
void CFrmWnd::OnG64K()
{
	CG64KWnd *pWnd;

	// ���݂���Ώ���
	pWnd = (CG64KWnd*)GetView()->SearchSWnd(MAKEID('G', '6', '4', 'K'));
	if (pWnd) {
		pWnd->PostMessage(WM_CLOSE, 0, 0);
		return;
	}

	// �T�u�E�B���h�E�쐬
	pWnd = new CG64KWnd;
	VERIFY(pWnd->Init(GetView()));
}

//---------------------------------------------------------------------------
//
//	�O���t�B�b�N���65536�F UI
//
//---------------------------------------------------------------------------
void CFrmWnd::OnG64KUI(CCmdUI *pCmdUI)
{
	ON_UPDATE_SUB_WINDOW(MAKEID('G', '6', '4', 'K'));
}

//---------------------------------------------------------------------------
//
//	PCG
//
//---------------------------------------------------------------------------
void CFrmWnd::OnPCG()
{
	CPCGWnd *pWnd;

	// ���݂���Ώ���
	pWnd = (CPCGWnd*)GetView()->SearchSWnd(MAKEID('P', 'C', 'G', ' '));
	if (pWnd) {
		pWnd->PostMessage(WM_CLOSE, 0, 0);
		return;
	}

	// �T�u�E�B���h�E�쐬
	pWnd = new CPCGWnd;
	VERIFY(pWnd->Init(GetView()));
}

//---------------------------------------------------------------------------
//
//	PCG UI
//
//---------------------------------------------------------------------------
void CFrmWnd::OnPCGUI(CCmdUI *pCmdUI)
{
	ON_UPDATE_SUB_WINDOW(MAKEID('P', 'C', 'G', ' '));
}

//---------------------------------------------------------------------------
//
//	BG���
//
//---------------------------------------------------------------------------
void CFrmWnd::OnBG(UINT uID)
{
	CBGWnd *pWnd;
	int index;

	// �C���f�b�N�X����
	index = (int)(uID - IDM_BG0);
	ASSERT((index == 0) || (index == 1));

	// ���݂���Ώ���
	pWnd = (CBGWnd*)GetView()->SearchSWnd(MAKEID('B', 'G', ('0' + index), ' '));
	if (pWnd) {
		pWnd->PostMessage(WM_CLOSE, 0, 0);
		return;
	}

	// �T�u�E�B���h�E�쐬
	pWnd = new CBGWnd(index);
	VERIFY(pWnd->Init(GetView()));
}

//---------------------------------------------------------------------------
//
//	BG��� UI
//
//---------------------------------------------------------------------------
void CFrmWnd::OnBGUI(CCmdUI *pCmdUI)
{
	int index;
	CSubWnd *pSubWnd;

	// �C���f�b�N�X����
	index = (int)(pCmdUI->m_nID - IDM_BG0);
	ASSERT((index == 0) || (index == 1));

	pSubWnd = GetView()->SearchSWnd(MAKEID('B', 'G', ('0' + index), ' '));
	if (pSubWnd) {
		pCmdUI->SetCheck(1);
	}
	else {
		pCmdUI->SetCheck(0);
	}
}

//---------------------------------------------------------------------------
//
//	�p���b�g
//
//---------------------------------------------------------------------------
void CFrmWnd::OnPalet()
{
	CPaletteWnd *pWnd;

	// ���݂���Ώ���
	pWnd = (CPaletteWnd*)GetView()->SearchSWnd(MAKEID('P', 'A', 'L', ' '));
	if (pWnd) {
		pWnd->PostMessage(WM_CLOSE, 0, 0);
		return;
	}

	// �T�u�E�B���h�E�쐬
	pWnd = new CPaletteWnd(FALSE);
	VERIFY(pWnd->Init(GetView()));
}

//---------------------------------------------------------------------------
//
//	�p���b�g UI
//
//---------------------------------------------------------------------------
void CFrmWnd::OnPaletUI(CCmdUI *pCmdUI)
{
	ON_UPDATE_SUB_WINDOW(MAKEID('P', 'A', 'L', ' '));
}

//---------------------------------------------------------------------------
//
//	�e�L�X�g�o�b�t�@
//
//---------------------------------------------------------------------------
void CFrmWnd::OnTextBuf()
{
	CRendBufWnd *pWnd;

	// ���݂���Ώ���
	pWnd = (CRendBufWnd*)GetView()->SearchSWnd(MAKEID('T', 'E', 'X', 'B'));
	if (pWnd) {
		pWnd->PostMessage(WM_CLOSE, 0, 0);
		return;
	}

	// �T�u�E�B���h�E�쐬
	pWnd = new CRendBufWnd(0);
	VERIFY(pWnd->Init(GetView()));
}

//---------------------------------------------------------------------------
//
//	�e�L�X�g�o�b�t�@ UI
//
//---------------------------------------------------------------------------
void CFrmWnd::OnTextBufUI(CCmdUI *pCmdUI)
{
	ON_UPDATE_SUB_WINDOW(MAKEID('T', 'E', 'X', 'B'));
}

//---------------------------------------------------------------------------
//
//	�O���t�B�b�N�o�b�t�@
//
//---------------------------------------------------------------------------
void CFrmWnd::OnGrpBuf(UINT uID)
{
	CRendBufWnd *pWnd;
	int index;

	// �C���f�b�N�X����
	index = (int)(uID - IDM_REND_GP0);
	ASSERT((index >= 0) || (index <= 4));

	// ���݂���Ώ���
	pWnd = (CRendBufWnd*)GetView()->SearchSWnd(MAKEID('G', 'P', ('0' + index), 'B'));
	if (pWnd) {
		pWnd->PostMessage(WM_CLOSE, 0, 0);
		return;
	}

	// �T�u�E�B���h�E�쐬
	pWnd = new CRendBufWnd(index + 1);
	VERIFY(pWnd->Init(GetView()));
}

//---------------------------------------------------------------------------
//
//	�O���t�B�b�N�o�b�t�@ UI
//
//---------------------------------------------------------------------------
void CFrmWnd::OnGrpBufUI(CCmdUI *pCmdUI)
{
	int index;
	CSubWnd *pSubWnd;

	// �C���f�b�N�X����
	index = (int)(pCmdUI->m_nID - IDM_REND_GP0);
	ASSERT((index >= 0) || (index <= 4));

	pSubWnd = GetView()->SearchSWnd(MAKEID('G', 'P', ('0' + index), 'B'));
	if (pSubWnd) {
		pCmdUI->SetCheck(1);
	}
	else {
		pCmdUI->SetCheck(0);
	}
}

//---------------------------------------------------------------------------
//
//	PCG�o�b�t�@
//
//---------------------------------------------------------------------------
void CFrmWnd::OnPCGBuf()
{
	ON_SUB_WINDOW(MAKEID('P', 'C', 'G', 'B'), CPCGBufWnd);
}

//---------------------------------------------------------------------------
//
//	PCG�o�b�t�@ UI
//
//---------------------------------------------------------------------------
void CFrmWnd::OnPCGBufUI(CCmdUI *pCmdUI)
{
	ON_UPDATE_SUB_WINDOW(MAKEID('P', 'C', 'G', 'B'));
}

//---------------------------------------------------------------------------
//
//	BG/�X�v���C�g�o�b�t�@
//
//---------------------------------------------------------------------------
void CFrmWnd::OnBGSpBuf()
{
	CRendBufWnd *pWnd;

	// ���݂���Ώ���
	pWnd = (CRendBufWnd*)GetView()->SearchSWnd(MAKEID('B', 'G', 'S', 'P'));
	if (pWnd) {
		pWnd->PostMessage(WM_CLOSE, 0, 0);
		return;
	}

	// �T�u�E�B���h�E�쐬
	pWnd = new CRendBufWnd(5);
	VERIFY(pWnd->Init(GetView()));
}

//---------------------------------------------------------------------------
//
//	BG/�X�v���C�g�o�b�t�@ UI
//
//---------------------------------------------------------------------------
void CFrmWnd::OnBGSpBufUI(CCmdUI *pCmdUI)
{
	ON_UPDATE_SUB_WINDOW(MAKEID('B', 'G', 'S', 'P'));
}

//---------------------------------------------------------------------------
//
//	�p���b�g�o�b�t�@
//
//---------------------------------------------------------------------------
void CFrmWnd::OnPaletBuf()
{
	CPaletteWnd *pWnd;

	// ���݂���Ώ���
	pWnd = (CPaletteWnd*)GetView()->SearchSWnd(MAKEID('P', 'A', 'L', 'B'));
	if (pWnd) {
		pWnd->PostMessage(WM_CLOSE, 0, 0);
		return;
	}

	// �T�u�E�B���h�E�쐬
	pWnd = new CPaletteWnd(TRUE);
	VERIFY(pWnd->Init(GetView()));
}

//---------------------------------------------------------------------------
//
//	�p���b�g�o�b�t�@ UI
//
//---------------------------------------------------------------------------
void CFrmWnd::OnPaletBufUI(CCmdUI *pCmdUI)
{
	ON_UPDATE_SUB_WINDOW(MAKEID('P', 'A', 'L', 'B'));
}

//---------------------------------------------------------------------------
//
//	�����o�b�t�@
//
//---------------------------------------------------------------------------
void CFrmWnd::OnMixBuf()
{
	ON_SUB_WINDOW(MAKEID('M', 'I', 'X', 'B'), CMixBufWnd);
}

//---------------------------------------------------------------------------
//
//	�����o�b�t�@ UI
//
//---------------------------------------------------------------------------
void CFrmWnd::OnMixBufUI(CCmdUI *pCmdUI)
{
	ON_UPDATE_SUB_WINDOW(MAKEID('M', 'I', 'X', 'B'));
}

//---------------------------------------------------------------------------
//
//	�R���|�[�l���g
//
//---------------------------------------------------------------------------
void CFrmWnd::OnComponent()
{
	ON_SUB_WINDOW(MAKEID('C', 'O', 'M', 'P'), CComponentWnd);
}

//---------------------------------------------------------------------------
//
//	�R���|�[�l���g UI
//
//---------------------------------------------------------------------------
void CFrmWnd::OnComponentUI(CCmdUI *pCmdUI)
{
	ON_UPDATE_SUB_WINDOW(MAKEID('C', 'O', 'M', 'P'));
}

//---------------------------------------------------------------------------
//
//	OS���
//
//---------------------------------------------------------------------------
void CFrmWnd::OnOSInfo()
{
	ON_SUB_WINDOW(MAKEID('O', 'S', 'I', 'N'), COSInfoWnd);
}

//---------------------------------------------------------------------------
//
//	OS��� UI
//
//---------------------------------------------------------------------------
void CFrmWnd::OnOSInfoUI(CCmdUI *pCmdUI)
{
	ON_UPDATE_SUB_WINDOW(MAKEID('O', 'S', 'I', 'N'));
}

//---------------------------------------------------------------------------
//
//	�T�E���h
//
//---------------------------------------------------------------------------
void CFrmWnd::OnSound()
{
	ON_SUB_WINDOW(MAKEID('S', 'N', 'D', ' '), CSoundWnd);
}

//---------------------------------------------------------------------------
//
//	�T�E���h UI
//
//---------------------------------------------------------------------------
void CFrmWnd::OnSoundUI(CCmdUI *pCmdUI)
{
	ON_UPDATE_SUB_WINDOW(MAKEID('S', 'N', 'D', ' '));
}

//---------------------------------------------------------------------------
//
//	�C���v�b�g
//
//---------------------------------------------------------------------------
void CFrmWnd::OnInput()
{
	ON_SUB_WINDOW(MAKEID('I', 'N', 'P', ' '), CInputWnd);
}

//---------------------------------------------------------------------------
//
//	�C���v�b�g UI
//
//---------------------------------------------------------------------------
void CFrmWnd::OnInputUI(CCmdUI *pCmdUI)
{
	ON_UPDATE_SUB_WINDOW(MAKEID('I', 'N', 'P', ' '));
}

//---------------------------------------------------------------------------
//
//	�|�[�g
//
//---------------------------------------------------------------------------
void CFrmWnd::OnPort()
{
	ON_SUB_WINDOW(MAKEID('P', 'O', 'R', 'T'), CPortWnd);
}

//---------------------------------------------------------------------------
//
//	�|�[�g UI
//
//---------------------------------------------------------------------------
void CFrmWnd::OnPortUI(CCmdUI *pCmdUI)
{
	ON_UPDATE_SUB_WINDOW(MAKEID('P', 'O', 'R', 'T'));
}

//---------------------------------------------------------------------------
//
//	�r�b�g�}�b�v
//
//---------------------------------------------------------------------------
void CFrmWnd::OnBitmap()
{
	ON_SUB_WINDOW(MAKEID('B', 'M', 'A', 'P'), CBitmapWnd);
}

//---------------------------------------------------------------------------
//
//	�r�b�g�}�b�v UI
//
//---------------------------------------------------------------------------
void CFrmWnd::OnBitmapUI(CCmdUI *pCmdUI)
{
	ON_UPDATE_SUB_WINDOW(MAKEID('B', 'M', 'A', 'P'));
}

//---------------------------------------------------------------------------
//
//	MIDI�h���C�o
//
//---------------------------------------------------------------------------
void CFrmWnd::OnMIDIDrv()
{
	ON_SUB_WINDOW(MAKEID('M', 'D', 'R', 'V'), CMIDIDrvWnd);
}

//---------------------------------------------------------------------------
//
//	MIDI�h���C�o UI
//
//---------------------------------------------------------------------------
void CFrmWnd::OnMIDIDrvUI(CCmdUI *pCmdUI)
{
	ON_UPDATE_SUB_WINDOW(MAKEID('M', 'D', 'R', 'V'));
}

//---------------------------------------------------------------------------
//
//	�L���v�V����
//
//---------------------------------------------------------------------------
void CFrmWnd::OnCaption()
{
	// �t���O�𔽓]
	m_bCaption = !m_bCaption;

	// ������ShowCaption�ɔC����
	ShowCaption();
}

//---------------------------------------------------------------------------
//
//	�L���v�V���� UI
//
//---------------------------------------------------------------------------
void CFrmWnd::OnCaptionUI(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(m_bCaption);
}

//---------------------------------------------------------------------------
//
//	���j���[�o�[
//
//---------------------------------------------------------------------------
void CFrmWnd::OnMenu()
{
	// �t���O�𔽓]
	m_bMenuBar = !m_bMenuBar;

	// ������ShowMenu�ɔC����
	ShowMenu();
}

//---------------------------------------------------------------------------
//
//	���j���[�o�[ UI
//
//---------------------------------------------------------------------------
void CFrmWnd::OnMenuUI(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(m_bMenuBar);
}

//---------------------------------------------------------------------------
//
//	�X�e�[�^�X�o�[
//
//---------------------------------------------------------------------------
void CFrmWnd::OnStatus()
{
	// �t���O�𔽓]
	m_bStatusBar = !m_bStatusBar;

	// ������ShowStatus�ɔC����
	ShowStatus();
}

//---------------------------------------------------------------------------
//
//	�X�e�[�^�X�o�[ UI
//
//---------------------------------------------------------------------------
void CFrmWnd::OnStatusUI(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(m_bStatusBar);
}

//---------------------------------------------------------------------------
//
//	���t���b�V��
//
//---------------------------------------------------------------------------
void CFrmWnd::OnRefresh()
{
	// ���b�N
	::LockVM();

	// �ĕ`��
	GetView()->Refresh();

	// �A�����b�N
	::UnlockVM();
}

//---------------------------------------------------------------------------
//
//	�g��
//
//---------------------------------------------------------------------------
void CFrmWnd::OnStretch()
{
	BOOL bFlag;

	// VM�����b�N
	::LockVM();

	// ���]
	bFlag = GetView()->IsStretch();
	GetView()->Stretch(!bFlag);

	// �R���t�B�O��ς���
	GetConfig()->SetStretch(!bFlag);

	// VM�A�����b�N
	::UnlockVM();
}

//---------------------------------------------------------------------------
//
//	�g�� UI
//
//---------------------------------------------------------------------------
void CFrmWnd::OnStretchUI(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(GetView()->IsStretch());
}

//---------------------------------------------------------------------------
//
//	�t���X�N���[��
//
//---------------------------------------------------------------------------
void CFrmWnd::OnFullScreen()
{
	HDC hDC;
	DEVMODE devmode;
	BOOL bEnable;
	BOOL bSound;
	BOOL bMouse;
	BOOL bChanged;
	CRect rectWnd;

	// �ύX�����s�ɏ�����
	bChanged = FALSE;

	// �}�E�XOFF�A�X�P�W���[����~�A�T�E���hOFF
	bEnable = GetScheduler()->IsEnable();
	bMouse = GetInput()->GetMouseMode();
	if (bMouse) {
		GetInput()->SetMouseMode(FALSE);
	}
	GetScheduler()->Enable(FALSE);
	::LockVM();
	::UnlockVM();
	::LockVM();
	bSound = GetSound()->IsEnable();
	GetSound()->Enable(FALSE);
	::UnlockVM();

	if (m_bFullScreen) {
		// �ʏ탂�[�h��

		// �E�B���h�E�v�f
		ModifyStyle(0, WS_CAPTION | WS_SYSMENU | WS_BORDER, 0);
		ModifyStyleEx(0, WS_EX_WINDOWEDGE, 0);
		GetView()->ModifyStyleEx(0, WS_EX_CLIENTEDGE, 0);

		// ��ʃ��[�h�؂�ւ�(1)
		m_DevMode.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT | DM_DISPLAYFREQUENCY;
		if (::ChangeDisplaySettings(&m_DevMode, 0) == DISP_CHANGE_SUCCESSFUL) {
			bChanged = TRUE;
		}

		// ���s������AFREQ�w����O���āA�ēx���݂�
		if (!bChanged) {
			// ��ʃ��[�h�؂�ւ�(2)
			m_DevMode.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;
			if (::ChangeDisplaySettings(&m_DevMode, 0) == DISP_CHANGE_SUCCESSFUL) {
				bChanged = TRUE;
			}
		}

		// ���������ꍇ
		if (bChanged) {
			// �ő剻��ԂőS��ʂɂ����̂ł���΁A���ɖ߂��Ă��
			if (IsZoomed()) {
				ShowWindow(SW_RESTORE);
			}

			// �L���v�V�����A���j���[�A�X�e�[�^�X�o�[
			m_bFullScreen = FALSE;
			ShowCaption();
			ShowMenu();
			ShowStatus();

			// �ĊJ����
			HideTaskBar(FALSE, TRUE);

			InitPos(FALSE);
			RecalcLayout();
			GetScheduler()->Enable(bEnable);
			GetSound()->Enable(bSound);
			GetInput()->SetMouseMode(bMouse);
			ResetCaption();
			ResetStatus();

			// �����}�E�X���[�h
			if (m_bAutoMouse && bMouse) {
				OnMouseMode();
			}
			return;
		}

		// �ʏ탂�[�h�ɂ������������A�ł��Ȃ�����
		ModifyStyle(WS_CAPTION | WS_SYSMENU | WS_BORDER, 0, 0);
		ModifyStyleEx(WS_EX_WINDOWEDGE, 0, 0);
		GetView()->ModifyStyleEx(WS_EX_CLIENTEDGE, 0, 0);

		// �ĊJ����
		GetScheduler()->Enable(bEnable);
		GetSound()->Enable(bSound);
		GetInput()->SetMouseMode(bMouse);
		ResetCaption();
		ResetStatus();
		return;
	}

	// �t���X�N���[���ֈڍs����ꍇ�A���݂̃E�B���h�E�ʒu��ۑ�
	GetWindowRect(&rectWnd);
	m_nWndLeft = rectWnd.left;
	m_nWndTop = rectWnd.top;

	


	// �E�B���h�E�v�f
	ModifyStyle(WS_CAPTION | WS_SYSMENU | WS_BORDER, 0, 0);
	ModifyStyleEx(WS_EX_WINDOWEDGE, 0, 0);
	GetView()->ModifyStyleEx(WS_EX_CLIENTEDGE, 0, 0);

	// ���݂̉�ʃ��[�h���擾
	hDC = ::GetDC(NULL);
	ASSERT(hDC);
	memset(&m_DevMode, 0, sizeof(m_DevMode));
	m_DevMode.dmSize = sizeof(DEVMODE);
	m_DevMode.dmPelsWidth = ::GetDeviceCaps(hDC, HORZRES);
	m_DevMode.dmPelsHeight = ::GetDeviceCaps(hDC, VERTRES);
	m_DevMode.dmBitsPerPel = ::GetDeviceCaps(hDC, BITSPIXEL);
	m_DevMode.dmDisplayFrequency = ::GetDeviceCaps(hDC, VREFRESH);
	::ReleaseDC(NULL, hDC);

	// ��ʃ��[�h�؂�ւ�
	memset(&devmode, 0, sizeof(devmode));
	devmode.dmSize = sizeof(DEVMODE);

	/* ACA SE MODIFICA LA RESOLUCION DE PANTALLA COMPLETA*/
/*	int rcx, rcy;
	GetDesktopResolution(rcx, rcy);*/

	//::SetProcessDPIAware();
	int	rcx = ::GetSystemMetrics(SM_CXSCREEN);
    int rcy = ::GetSystemMetrics(SM_CYSCREEN);


	/*char cadena[20],cadena2[20];	  
    sprintf(cadena, "%d", rcx);
	sprintf(cadena2, "%d", rcy);
	 int msgboxID = MessageBox(
       cadena,"Width",
        2 );
	 int msgboxID2 = MessageBox(
       cadena2,"Height",
        2 );*/
		

	devmode.dmBitsPerPel = 32;
	devmode.dmPelsWidth = rcx;
	devmode.dmPelsHeight = rcy;
	devmode.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;
	if (::ChangeDisplaySettings(&devmode, CDS_FULLSCREEN) == DISP_CHANGE_SUCCESSFUL) {
		// �L���v�V�����A���j���[�A�X�e�[�^�X�o�[
		m_bFullScreen = TRUE;
		ShowCaption();
		ShowMenu();


		/* No mostrar barra de estado en fullScreen */
		//ShowStatus();

		// �ĊJ����
		HideTaskBar(TRUE, TRUE);

		InitPos(FALSE);
		RecalcStatusView();
		GetScheduler()->Enable(bEnable);
		GetSound()->Enable(bSound);
		GetInput()->SetMouseMode(bMouse);
		ResetCaption();
		ResetStatus();

		// �����}�E�X���[�h
		if (m_bAutoMouse && bEnable && !bMouse) {
			OnMouseMode();
		}
		//RestoreFrameWnd(FALSE);


/*
		// Mensajes de top y bottom
	char cadena[20],cadena2[20];	  
    sprintf(cadena, "%d", rectWnd.top);
	sprintf(cadena2, "%d", rectWnd.bottom);
	 int msgboxID = MessageBox(
       cadena,"Height",
        2 );
	 int msgboxID2 = MessageBox(
       cadena2,"Width",
        2 );
		*/

		return;
	}

	// �t���X�N���[���ɂ��悤�Ƃ������A�ł��Ȃ�����
	ModifyStyle(0, WS_CAPTION | WS_SYSMENU | WS_BORDER, 0);
	ModifyStyleEx(0, WS_EX_WINDOWEDGE, 0);
	
	GetView()->ModifyStyleEx(0, WS_EX_CLIENTEDGE, 0);
	//GetView()->SetupBitmap();
	// �ĊJ����
	GetScheduler()->Enable(bEnable);
	GetSound()->Enable(bSound);
	GetInput()->SetMouseMode(bMouse);
	ResetCaption();
	ResetStatus();	


}


//---------------------------------------------------------------------------
//
//	�t���X�N���[�� UI
//
//---------------------------------------------------------------------------
void CFrmWnd::OnFullScreenUI(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(m_bFullScreen);
}

//---------------------------------------------------------------------------
//
//	���s
//
//---------------------------------------------------------------------------
void CFrmWnd::OnExec()
{
	// ���b�N
	::LockVM();

	// �X�P�W���[����L����
	GetScheduler()->Reset();
	GetScheduler()->Enable(TRUE);
	ResetCaption();

	// �A�����b�N
	::UnlockVM();
}

//---------------------------------------------------------------------------
//
//	���s UI
//
//---------------------------------------------------------------------------
void CFrmWnd::OnExecUI(CCmdUI *pCmdUI)
{
	// �X�P�W���[������~���Ȃ�L��
	if ((!GetScheduler()->IsEnable()) && ::GetVM()->IsPower()) {
		pCmdUI->Enable(TRUE);
	}
	else {
		pCmdUI->Enable(FALSE);
	}
}

//---------------------------------------------------------------------------
//
//	��~
//
//---------------------------------------------------------------------------
void CFrmWnd::OnBreak()
{
	// ���b�N
	::LockVM();

	// �X�P�W���[���𖳌���
	GetScheduler()->Enable(FALSE);

	// �A�����b�N
	::UnlockVM();
}

//---------------------------------------------------------------------------
//
//	��~ UI
//
//---------------------------------------------------------------------------
void CFrmWnd::OnBreakUI(CCmdUI *pCmdUI)
{
	// �X�P�W���[�������쒆�Ȃ�L��
	if (GetScheduler()->IsEnable()) {
		pCmdUI->Enable(TRUE);
	}
	else {
		pCmdUI->Enable(FALSE);
	}
}

//---------------------------------------------------------------------------
//
//	�g���[�X
//
//---------------------------------------------------------------------------
void CFrmWnd::OnTrace()
{
	// ���b�N
	::LockVM();

	// �g���[�X
	::GetVM()->Trace();
	GetScheduler()->SyncDisasm();

	// �A�����b�N
	::UnlockVM();
}

//---------------------------------------------------------------------------
//
//	�g���[�X UI
//
//---------------------------------------------------------------------------
void CFrmWnd::OnTraceUI(CCmdUI *pCmdUI)
{
	// �X�P�W���[������~���Ȃ�L��
	if ((!GetScheduler()->IsEnable()) && ::GetVM()->IsPower()) {
		pCmdUI->Enable(TRUE);
	}
	else {
		pCmdUI->Enable(FALSE);
	}
}

//---------------------------------------------------------------------------
//
//	�}�E�X���[�h
//
//---------------------------------------------------------------------------
void CFrmWnd::OnMouseMode()
{
	CRect rect;
	BOOL b;
	int cnt;
	CString string;
	LONG cx;
	LONG cy;

	// ���݂̃��[�h������
	b = GetInput()->GetMouseMode();

	// �ŏ������A����уX�P�W���[��OFF����OFF����̂�
	if (!b) {
		// �ŏ����Ȃ�ON�ɂ����Ȃ�
		if (IsIconic()) {
			return;
		}
		// �X�P�W���[��OFF�Ȃ�ON�ɂ����Ȃ�
		if (!GetScheduler()->IsEnable()) {
			return;
		}
		// ��A�N�e�B�u�Ȃ�ON�ɂ����Ȃ�
		if (!GetInput()->IsActive()) {
			return;
		}
	}

	if (b) {
		// �}�E�X���[�hOFF��
		GetInput()->SetMouseMode(FALSE);

		// �N���b�v����
		ClipCursor(NULL);

		// �}�E�X�J�[�\�����X�N���[��������
		cx = ::GetSystemMetrics(SM_CXSCREEN);
		cy = ::GetSystemMetrics(SM_CYSCREEN);
		SetCursorPos(cx >> 1, cy >> 1);

		// �}�E�X�J�[�\��ON
		cnt = ::ShowCursor(TRUE);
		while (cnt < 0) {
			cnt = ::ShowCursor(TRUE);
		}

		// ���b�Z�[�W
		::GetMsg(IDS_MOUSE_WIN, string);
	}
	else {
		// �}�E�X�J�[�\�����N���b�v�BView�E�B���h�E��+16�̂�
		GetView()->GetWindowRect(&rect);
		rect.right = rect.left + 16;
		rect.bottom = rect.top + 16;
		ClipCursor(&rect);

		// �}�E�X�J�[�\��OFF
		cnt = ::ShowCursor(FALSE);
		while (cnt >= 0) {
			cnt = ::ShowCursor(FALSE);
		}

		// �}�E�X���[�hON��
		GetInput()->SetMouseMode(TRUE);

		// ���b�Z�[�W
		::GetMsg(IDS_MOUSE_X68K, string);
	}

	// ���b�Z�[�W�Z�b�g
	SetInfo(string);
}

//---------------------------------------------------------------------------
//
//	�\�t�g�E�F�A�L�[�{�[�h
//
//---------------------------------------------------------------------------
void CFrmWnd::OnSoftKey()
{
	ON_SUB_WINDOW(MAKEID('S', 'K', 'E', 'Y'), CSoftKeyWnd);
}

//---------------------------------------------------------------------------
//
//	�\�t�g�E�F�A�L�[�{�[�h UI
//
//---------------------------------------------------------------------------
void CFrmWnd::OnSoftKeyUI(CCmdUI *pCmdUI)
{
	ON_UPDATE_SUB_WINDOW(MAKEID('S', 'K', 'E', 'Y'));

	// �L�[�{�[�h�f�o�C�X�����݂��Ȃ���΁A�������Ȃ�
	if (!m_pKeyboard) {
		return;
	}

	// �T�u�E�B���h�E���݂���
	if (GetView()->SearchSWnd(MAKEID('S', 'K', 'E', 'Y')) == NULL) {
		// �L�[�{�[�h���ڑ�����Ă��Ȃ����
		if (!m_pKeyboard->IsConnect()) {
			// �֎~
			pCmdUI->Enable(FALSE);
			return;
		}
	}

	// �L��
	pCmdUI->Enable(TRUE);
}

//---------------------------------------------------------------------------
//
//	�������킹
//
//---------------------------------------------------------------------------
void CFrmWnd::OnTimeAdj()
{
	RTC *pRTC;

	// pRTC->Adjust()�ō��킹��
	::LockVM();
	pRTC = (RTC*)::GetVM()->SearchDevice(MAKEID('R', 'T', 'C', ' '));
	ASSERT(pRTC);
	pRTC->Adjust(FALSE);
	::UnlockVM();
}

//---------------------------------------------------------------------------
//
//	WAV�L���v�`��
//
//---------------------------------------------------------------------------
void CFrmWnd::OnSaveWav()
{
	TCHAR szPath[_MAX_PATH];
	CString strMsg;
	BOOL bResult;

	// WAV�Z�[�u���Ȃ�~�߂�
	if (GetSound()->IsSaveWav()) {
		GetSound()->EndSaveWav();
		::GetMsg(IDS_WAVSTOP, strMsg);
		SetInfo(strMsg);
		return;
	}

	// �t�@�C���I��
	szPath[0] = _T('\0');
	if (!FileSaveDlg(this, szPath, _T("wav"), IDS_WAVOPEN)) {
		ResetCaption();
		return;
	}

	// WAV�Z�[�u�Ƀg���C
	::LockVM();
	bResult = GetSound()->StartSaveWav(szPath);
	::UnlockVM();

	// ���ʕ]��
	if (bResult) {
		::GetMsg(IDS_WAVSTART, strMsg);
		SetInfo(strMsg);
	}
	else {
		::GetMsg(IDS_CREATEERR, strMsg);
		MessageBox(strMsg, NULL, MB_ICONSTOP | MB_OK);
	}
}

//---------------------------------------------------------------------------
//
//	WAV�L���v�`�� UI
//
//---------------------------------------------------------------------------
void CFrmWnd::OnSaveWavUI(CCmdUI *pCmdUI)
{
	pCmdUI->Enable(GetSound()->IsPlay());
	pCmdUI->SetCheck(GetSound()->IsSaveWav());
}

//---------------------------------------------------------------------------
//
//	trap#0
//
//---------------------------------------------------------------------------
void CFrmWnd::OnTrap()
{
	CTrapDlg dlg(this);
	DWORD dwVector;
	MFP *pMFP;
	Memory *pMemory;
	DWORD dwAddr;
	DWORD dwCode;

	// �f�o�C�X�擾
	pMFP = (MFP*)::GetVM()->SearchDevice(MAKEID('M', 'F', 'P', ' '));
	ASSERT(pMFP);
	pMemory = (Memory*)::GetVM()->SearchDevice(MAKEID('M', 'E', 'M', ' '));
	ASSERT(pMemory);

	// ���b�N
	::LockVM();

	// MFP���A�x�N�^���擾
	dwVector = (pMFP->GetVR() & 0xf0) + 5;
	dwVector <<= 2;

	// ���݂�Timer-C�x�N�^�A�h���X�𓾂�
	dwAddr = pMemory->ReadOnly(dwVector);
	dwAddr <<= 8;
	dwAddr |= pMemory->ReadOnly(dwVector + 1);
	dwAddr <<= 8;
	dwAddr |= pMemory->ReadOnly(dwVector + 2);
	dwAddr <<= 8;
	dwAddr |= pMemory->ReadOnly(dwVector + 3);

	// ��������(24bit�ȏ�)�Ȃ疳��
	if (dwAddr > 0xffffff) {
		::UnlockVM();
		return;
	}

	// ����0x6800�Ȃ疳��
	if (dwAddr == 0x6800) {
		::UnlockVM();
		return;
	}

	// �R�[�h�I��
	dlg.m_dwCode = pMemory->ReadOnly(0x6809);
	if (pMemory->ReadOnly(0x6808) == 0xff) {
		// ���������Ƃ݂Ȃ��A0����X�^�[�g
		dlg.m_dwCode = 1;
	}
	::UnlockVM();
	if (dlg.DoModal() != IDOK) {
		return;
	}
	dwCode = dlg.m_dwCode;

	// �R�[�h��������
	::LockVM();
	pMemory->WriteWord(0x6800, 0x2f00);
	pMemory->WriteWord(0x6802, 0x2f00);
	pMemory->WriteWord(0x6804, 0x203c);
	pMemory->WriteWord(0x6806, dwCode >> 16);
	pMemory->WriteWord(0x6808, (WORD)dwCode);
	pMemory->WriteWord(0x680a, 0x4e40);
	pMemory->WriteWord(0x680c, 0x203c);
	pMemory->WriteWord(0x680e, dwAddr >> 16);
	pMemory->WriteWord(0x6810, (WORD)dwAddr);
	pMemory->WriteWord(0x6812, 0x21c0);
	pMemory->WriteWord(0x6814, dwVector);
	pMemory->WriteWord(0x6816, 0x2f40);
	pMemory->WriteWord(0x6818, 0x0004);
	pMemory->WriteWord(0x681a, 0x201f);
	pMemory->WriteWord(0x681c, 0x4e75);

	// Timer-C�x�N�^�ύX
	pMemory->WriteWord(dwVector, 0x0000);
	pMemory->WriteWord(dwVector + 2, 0x6800);

	// �A�����b�N
	::UnlockVM();
}

//---------------------------------------------------------------------------
//
//	trap#0 UI
//
//---------------------------------------------------------------------------
void CFrmWnd::OnTrapUI(CCmdUI *pCmdUI)
{
	DWORD dwVector;
	MFP *pMFP;
	Memory *pMemory;
	DWORD dwAddr;

	// �f�o�C�X�擾
	pMFP = (MFP*)::GetVM()->SearchDevice(MAKEID('M', 'F', 'P', ' '));
	ASSERT(pMFP);
	pMemory = (Memory*)::GetVM()->SearchDevice(MAKEID('M', 'E', 'M', ' '));
	ASSERT(pMemory);

	// MFP���A�x�N�^���擾
	dwVector = (pMFP->GetVR() & 0xf0) + 5;
	dwVector <<= 2;

	// ���݂�Timer-C�x�N�^�A�h���X�𓾂�
	dwAddr = pMemory->ReadOnly(dwVector);
	dwAddr <<= 8;
	dwAddr |= pMemory->ReadOnly(dwVector + 1);
	dwAddr <<= 8;
	dwAddr |= pMemory->ReadOnly(dwVector + 2);
	dwAddr <<= 8;
	dwAddr |= pMemory->ReadOnly(dwVector + 3);

	// ��������(24bit�ȏ�)�Ȃ�֎~
	if (dwAddr > 0xffffff) {
		pCmdUI->Enable(FALSE);
		return;
	}

	// ����0x6800�Ȃ�֎~
	if (dwAddr == 0x6800) {
		pCmdUI->Enable(FALSE);
		return;
	}

	// trap #0�x�N�^���擾
	dwAddr = pMemory->ReadOnly(0x0080);
	dwAddr <<= 8;
	dwAddr |= pMemory->ReadOnly(0x0081);
	dwAddr <<= 8;
	dwAddr |= pMemory->ReadOnly(0x0082);
	dwAddr <<= 8;
	dwAddr |= pMemory->ReadOnly(0x0083);

	// ��������(24bit�ȏ�)�Ȃ�֎~
	if (dwAddr > 0xffffff) {
		pCmdUI->Enable(FALSE);
		return;
	}

	// ����
	pCmdUI->Enable(TRUE);
}

//---------------------------------------------------------------------------
//
//	�V�����t���b�s�[�f�B�X�N
//
//---------------------------------------------------------------------------
void CFrmWnd::OnNewFD()
{
	CFDIDlg dlg(this);
	FDIDisk *pDisk;
	FDIDisk::option_t opt;
	CString strMsg;
	BOOL bRun;
	Filepath path;

	// �_�C�A���O���s
	if (dlg.DoModal() != IDOK) {
		return;
	}

	// �p�X�쐬
	path.SetPath(dlg.m_szFileName);

	// �I�v�V�����\���̍쐬
	opt.phyfmt = dlg.m_dwPhysical;
	opt.logfmt = dlg.m_bLogical;
	ASSERT(_tcslen(dlg.m_szDiskName) < 60);
	_tcscpy(opt.name, dlg.m_szDiskName);

	// �^�C�v�ɉ����ăC���[�W�����
	switch (dlg.m_dwType) {
		case 0:
			pDisk = (FDIDisk*)new FDIDisk2HD(0, NULL);
			break;
		case 1:
			pDisk = (FDIDisk*)new FDIDiskDIM(0, NULL);
			break;
		case 2:
			pDisk = (FDIDisk*)new FDIDiskD68(0, NULL);
			break;
		case 3:
			pDisk = (FDIDisk*)new FDIDisk2DD(0, NULL);
			break;
		case 4:
			pDisk = (FDIDisk*)new FDIDisk2HQ(0, NULL);
			break;
		default:
			ASSERT(FALSE);
			return;
	}

	// �t�H�[�}�b�g(�����E�_���E�Z�[�u�����ׂĊ܂�)
	// �t�H�[�}�b�g���̓X�P�W���[�����~�߂�
	AfxGetApp()->BeginWaitCursor();
	bRun = GetScheduler()->IsEnable();
	GetScheduler()->Enable(FALSE);
	::LockVM();
	::UnlockVM();
	if (!pDisk->Create(path, &opt)) {
		AfxGetApp()->EndWaitCursor();
		// ��x�폜
		delete pDisk;

		// ���b�Z�[�W�{�b�N�X
		::GetMsg(IDS_CREATEERR, strMsg);
		MessageBox(strMsg, NULL, MB_ICONSTOP | MB_OK);
		GetScheduler()->Reset();
		GetScheduler()->Enable(bRun);
		ResetCaption();
		return;
	}
	AfxGetApp()->EndWaitCursor();

	// ��x�폜
	delete pDisk;

	// �I�[�g�}�E���g(�I�v�V���i��)
	if (dlg.m_nDrive >= 0) {
		InitCmdSub(dlg.m_nDrive, dlg.m_szFileName);
	}

	// �X�e�[�^�X���b�Z�[�W�A�ĊJ
	::GetMsg(IDS_NEWFD, strMsg);
	SetInfo(strMsg);
	GetScheduler()->Reset();
	GetScheduler()->Enable(bRun);
	ResetCaption();
}

//---------------------------------------------------------------------------
//
//	�V������e�ʃf�B�X�N
//
//---------------------------------------------------------------------------
void CFrmWnd::OnNewDisk(UINT uID)
{
	CDiskDlg dlg(this);
	CString strMsg;
	BOOL bRun;
	UINT nMsg;
	LPCTSTR lpszPath;
	Filepath path;

	// �p�����[�^�󂯎��
	ASSERT(this);
	ASSERT((uID >= IDM_NEWSASI) && (uID <= IDM_NEWMO));
	uID -= IDM_NEWSASI;
	ASSERT(uID <= 2);

	// ��ʂ�n��
	dlg.m_nType = (int)uID;

	// �X�P�W���[���̏�Ԃ𓾂Ă���(��~�̓_�C�A���O�����ōs�Ȃ�)
	bRun = GetScheduler()->IsEnable();

	// �_�C�A���O���s
	if (dlg.DoModal() != IDOK) {
		// �L�����Z�������ꍇ
		return;
	}

	// ���s�����ꍇ�́A���ʕ]��
	if (!dlg.IsSucceeded()) {
		if (dlg.IsCanceled()) {
			// �L�����Z������
			::GetMsg(IDS_CANCEL, strMsg);
		}
		else {
			// ��낤�Ƃ��������s����
			::GetMsg(IDS_CREATEERR, strMsg);
		}
		MessageBox(strMsg, NULL, MB_ICONSTOP | MB_OK);
		GetScheduler()->Reset();
		GetScheduler()->Enable(bRun);
		ResetCaption();
		return;
	}

	// ��񃁃b�Z�[�W�𓾂�
	switch (uID) {
		// SASI-HD
		case 0:
			nMsg = IDS_NEWSASI;
			break;

		// SCSI-HD
		case 1:
			nMsg = IDS_NEWSCSI;
			break;

		// SCSI-MO
		case 2:
			nMsg = IDS_NEWMO;
			break;

		// ���̑�(���蓾�Ȃ�)
		default:
			ASSERT(FALSE);
			nMsg = 0;
			break;
	}

	// ���b�Z�[�W�����[�h
	::GetMsg(nMsg, strMsg);

	// MO�̃}�E���g
	if (uID == 2) {
		// �t���O�`�F�b�N
		if (dlg.m_bMount) {
			// MO�L����
			if (m_pSASI->IsValid()) {
				// �t�@�C�����擾
				lpszPath = dlg.GetPath();

				// �I�[�v����MRU
				path.SetPath(lpszPath);
				if (m_pSASI->Open(path)) {
					GetConfig()->SetMRUFile(2, lpszPath);
				}
			}
		}
	}

	// �X�e�[�^�X���b�Z�[�W�A�ĊJ
	SetInfo(strMsg);
	GetScheduler()->Reset();
	GetScheduler()->Enable(bRun);
	ResetCaption();
}

//---------------------------------------------------------------------------
//
//	�I�v�V����
//
//---------------------------------------------------------------------------
void CFrmWnd::OnOptions()
{
	Config config;
	CConfigSheet sheet(this);

	// �ݒ�f�[�^���擾
	GetConfig()->GetConfig(&config);

	// �v���p�e�B�V�[�g�����s
	sheet.m_pConfig = &config;
	if (sheet.DoModal() != IDOK) {
		return;
	}

	// �f�[�^�]��
	GetConfig()->SetConfig(&config);

	// �K�p(VM���b�N���čs��)
	::LockVM();
	ApplyCfg();
	GetScheduler()->Reset();
	ResetCaption();
	::UnlockVM();
}

//---------------------------------------------------------------------------
//
//	�d�˂ĕ\��
//
//---------------------------------------------------------------------------
void CFrmWnd::OnCascade()
{
	ASSERT(GetView());
	ASSERT(GetView()->m_hWnd);
	::CascadeWindows(GetView()->m_hWnd, 0, NULL, 0, NULL);
}

//---------------------------------------------------------------------------
//
//	�d�˂ĕ\�� UI
//
//---------------------------------------------------------------------------
void CFrmWnd::OnCascadeUI(CCmdUI *pCmdUI)
{
	CMenu *pMenu;
	CMenu *pSubMenu;
	CSubWnd *pWnd;
	CString string;
	CString temp;
	int i;
	int n;
	UINT uID;
	DWORD dwID;

	ASSERT(GetView());
	ASSERT(GetView()->m_hWnd);

	// ���̃��j���[���̂̏���
	if (IsPopupSWnd()) {
		// �|�b�v�A�b�v�̏ꍇ�͈Ӗ����Ȃ�
		pCmdUI->Enable(FALSE);
	}
	else {
		// �`���C���h�̏ꍇ�A�T�u�E�B���h�E��1�ȏ゠���TRUE
		if (GetView()->GetSubWndNum() >= 1) {
			pCmdUI->Enable(TRUE);
		}
		else {
			pCmdUI->Enable(FALSE);
		}
	}

	// Window���j���[��T��
	if (m_bPopupMenu) {
		pMenu = m_PopupMenu.GetSubMenu(0);
	}
	else {
		pMenu = &m_Menu;
	}
	ASSERT(pMenu);
	n = pMenu->GetMenuItemCount();
	pSubMenu = NULL;
	for (i=0; i<n; i++) {
		pSubMenu = pMenu->GetSubMenu(i);
		ASSERT(pSubMenu);
		if (pSubMenu->GetMenuItemID(0) == IDM_CASCADE) {
			break;
		}
	}
	ASSERT(pSubMenu);
	ASSERT(i < n);

	// 6���c���č폜
	while (pSubMenu->GetMenuItemCount() > 6) {
		pSubMenu->DeleteMenu(6, MF_BYPOSITION);
	}

	// �L���ȃT�u�E�B���h�E�����邩
	pWnd = GetView()->GetFirstSWnd();
	if (!pWnd) {
		return;
	}

	// �Z�p���[�^��ǉ�
	pSubMenu->AppendMenu(MF_SEPARATOR, 0);

	// ���j���[�������ǉ�
	uID = IDM_SWND_START;
	while (pWnd) {
		// �E�B���h�E�^�C�g��,ID���擾
		pWnd->GetWindowText(string);
		dwID = pWnd->GetID();
		temp.Format("%c%c%c%c - ",
			(dwID >> 24) & 0xff,
			(dwID >> 16) & 0xff,
			(dwID >> 8) & 0xff,
			dwID & 0xff);
		string = temp + string;

		// ���j���[�ǉ�
		pSubMenu->AppendMenu(MF_STRING, uID, string);

		// ����
		pWnd = pWnd->m_pNextWnd;
		uID++;
	}
}

//---------------------------------------------------------------------------
//
//	���ׂĕ\��
//
//---------------------------------------------------------------------------
void CFrmWnd::OnTile()
{
	ASSERT(GetView());
	ASSERT(GetView()->m_hWnd);
	::TileWindows(GetView()->m_hWnd, MDITILE_VERTICAL, NULL, 0, NULL);
}

//---------------------------------------------------------------------------
//
//	���ׂĕ\�� UI
//
//---------------------------------------------------------------------------
void CFrmWnd::OnTileUI(CCmdUI *pCmdUI)
{
	if (IsPopupSWnd()) {
		// �|�b�v�A�b�v�̏ꍇ�͈Ӗ����Ȃ�
		pCmdUI->Enable(FALSE);
	}
	else {
		// �`���C���h�̏ꍇ�A�T�u�E�B���h�E��1�ȏ゠���TRUE
		if (GetView()->GetSubWndNum() >= 1) {
			pCmdUI->Enable(TRUE);
		}
		else {
			pCmdUI->Enable(FALSE);
		}
	}
}

//---------------------------------------------------------------------------
//
//	�S�ăA�C�R����
//
//---------------------------------------------------------------------------
void CFrmWnd::OnIconic()
{
	CSubWnd *pSubWnd;

	ASSERT(this);
	ASSERT(GetView());
	ASSERT(GetView()->m_hWnd);

	// �ŏ��̃T�u�E�B���h�E���擾
	pSubWnd = GetView()->GetFirstSWnd();

	// ���[�v
	while (pSubWnd) {
		pSubWnd->ShowWindow(SW_MINIMIZE);
		pSubWnd = pSubWnd->m_pNextWnd;
	}
}

//---------------------------------------------------------------------------
//
//	�S�ăA�C�R���� UI
//
//---------------------------------------------------------------------------
void CFrmWnd::OnIconicUI(CCmdUI *pCmdUI)
{
	// �T�u�E�B���h�E�����݂����TRUE
	if (GetView()->GetSubWndNum() > 0) {
		pCmdUI->Enable(TRUE);
	}
	else {
		pCmdUI->Enable(FALSE);
	}
}

//---------------------------------------------------------------------------
//
//	�A�C�R���̐���
//
//---------------------------------------------------------------------------
void CFrmWnd::OnArrangeIcon()
{
	ASSERT(GetView());
	ASSERT(GetView()->m_hWnd);
	::ArrangeIconicWindows(GetView()->m_hWnd);
}

//---------------------------------------------------------------------------
//
//	�A�C�R���̐��� UI
//
//---------------------------------------------------------------------------
void CFrmWnd::OnArrangeIconUI(CCmdUI *pCmdUI)
{
	CSubWnd *pSubWnd;

	if (IsPopupSWnd()) {
		// �|�b�v�A�b�v�̏ꍇ�͈Ӗ����Ȃ�
		pCmdUI->Enable(FALSE);
		return;
	}

	// �T�u�E�B���h�E�̒��ŁA�A�C�R����Ԃ̂��̂�����ΗL��
	pSubWnd = GetView()->GetFirstSWnd();
	while (pSubWnd) {
		// �ŏ�������Ă��邩
		if (pSubWnd->IsIconic()) {
			pCmdUI->Enable(TRUE);
			return;
		}

		// ���̃T�u�E�B���h�E
		pSubWnd = pSubWnd->m_pNextWnd;
	}

	// �ŏ�������Ă�����̂��Ȃ��̂ŁA����
	pCmdUI->Enable(FALSE);
}

//---------------------------------------------------------------------------
//
//	�S�ĉB��
//
//---------------------------------------------------------------------------
void CFrmWnd::OnHide()
{
	CSubWnd *pSubWnd;

	ASSERT(this);
	ASSERT(GetView());
	ASSERT(GetView()->m_hWnd);

	// �ŏ��̃T�u�E�B���h�E���擾
	pSubWnd = GetView()->GetFirstSWnd();

	// ���[�v
	while (pSubWnd) {
		pSubWnd->ShowWindow(SW_HIDE);
		pSubWnd = pSubWnd->m_pNextWnd;
	}
}

//---------------------------------------------------------------------------
//
//	�S�ĉB�� UI
//
//---------------------------------------------------------------------------
void CFrmWnd::OnHideUI(CCmdUI *pCmdUI)
{
	// �T�u�E�B���h�E�����݂����TRUE
	if (GetView()->GetSubWndNum() > 0) {
		pCmdUI->Enable(TRUE);
	}
	else {
		pCmdUI->Enable(FALSE);
	}
}

//---------------------------------------------------------------------------
//
//	�S�ĕ���
//
//---------------------------------------------------------------------------
void CFrmWnd::OnRestore()
{
	CSubWnd *pSubWnd;

	ASSERT(this);
	ASSERT(GetView());
	ASSERT(GetView()->m_hWnd);

	// �ŏ��̃T�u�E�B���h�E���擾
	pSubWnd = GetView()->GetFirstSWnd();

	// ���[�v
	while (pSubWnd) {
		pSubWnd->ShowWindow(SW_RESTORE);
		pSubWnd = pSubWnd->m_pNextWnd;
	}
}

//---------------------------------------------------------------------------
//
//	�S�ĕ��� UI
//
//---------------------------------------------------------------------------
void CFrmWnd::OnRestoreUI(CCmdUI *pCmdUI)
{
	// �T�u�E�B���h�E�����݂����TRUE
	if (GetView()->GetSubWndNum() > 0) {
		pCmdUI->Enable(TRUE);
	}
	else {
		pCmdUI->Enable(FALSE);
	}
}

//---------------------------------------------------------------------------
//
//	�E�B���h�E�w��
//
//---------------------------------------------------------------------------
void CFrmWnd::OnWindow(UINT uID)
{
	CSubWnd *pSubWnd;
	int n;

	ASSERT(this);
	ASSERT(GetView());
	ASSERT(GetView()->m_hWnd);

	// �T�u�E�B���h�E����肷��
	n = (int)(uID - IDM_SWND_START);
	pSubWnd = GetView()->GetFirstSWnd();
	if (!pSubWnd) {
		return;
	}

	// �������[�v
	while (n > 0) {
		if (!pSubWnd) {
			return;
		}

		n--;
		pSubWnd = pSubWnd->m_pNextWnd;
	}

	// pSubWnd���Z���N�g
	pSubWnd->ShowWindow(SW_RESTORE);
	pSubWnd->SetWindowPos(&wndTop, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
}

//---------------------------------------------------------------------------
//
//	�o�[�W�������
//
//---------------------------------------------------------------------------
void CFrmWnd::OnAbout()
{
	CAboutDlg dlg(this);

	// ���[�_���_�C�A���O�����s
	dlg.DoModal();
}

#endif	// _WIN32
