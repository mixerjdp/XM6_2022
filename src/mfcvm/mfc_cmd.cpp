//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 ‚o‚hD(ytanaka@ipc-tokai.or.jp)
//	[ MFC ƒRƒ}ƒ“ƒhˆ— ]
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

	// ƒRƒ‚ƒ“ƒ_ƒCƒAƒƒOÀs
	::GetVM()->GetPath(path);
	_tcscpy(szPath, path.GetPath());
	if (!::FileOpenDlg(this, szPath, IDS_XM6OPEN)) {
		ResetCaption();
		return;
	}
	path.SetPath(szPath);
			

	// ƒI[ƒvƒ“‘Oˆ—
	if (!OnOpenPrep(path)) {
		return;
	}

	// ƒI[ƒvƒ“ƒTƒu
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
			

	// ƒI[ƒvƒ“‘Oˆ—
	if (!OnOpenPrep(path)) {
		return;
	}

	// ƒI[ƒvƒ“ƒTƒu
	OnOpenSub(path);
}

//---------------------------------------------------------------------------
//
//	ŠJ‚­ UI
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

	// “dŒ¹ó‘Ô‚ğæ“¾Aƒtƒ@ƒCƒ‹ƒpƒX‚ğæ“¾(VMƒƒbƒN‚µ‚Äs‚¤)
	::LockVM();
	bPower = ::GetVM()->IsPower();
	bSW = ::GetVM()->IsPowerSW();
	::GetVM()->GetPath(path);
	::UnlockVM();

	// ƒI[ƒvƒ“
	pCmdUI->Enable(bPower);

	// ƒTƒuƒƒjƒ…[æ“¾
	if (m_bPopupMenu) {
		pMenu = m_PopupMenu.GetSubMenu(0);
	}
	else {
		pMenu = &m_Menu;
	}
	ASSERT(pMenu);
	// ƒtƒ@ƒCƒ‹ƒƒjƒ…[‚ÍÅ‰
	pSubMenu = pMenu->GetSubMenu(0);
	ASSERT(pSubMenu);

	// ã‘‚«•Û‘¶UI(ˆÈ‰ºAON_UPDATE_COMMAND_UI‚Ìƒ^ƒCƒ~ƒ“ƒO‘Îô)
	if (bPower && (_tcslen(path.GetPath()) > 0)) {
		pSubMenu->EnableMenuItem(1, MF_BYPOSITION | MF_ENABLED);
	}
	else {
		pSubMenu->EnableMenuItem(1, MF_BYPOSITION | MF_GRAYED);
	}

	// –¼‘O‚ğ•t‚¯‚Ä•Û‘¶UI
	if (bPower) {
		pSubMenu->EnableMenuItem(2, MF_BYPOSITION | MF_GRAYED);
	}
	else {
		pSubMenu->EnableMenuItem(2, MF_BYPOSITION | MF_ENABLED);
	}

	// ƒŠƒZƒbƒgUI
	if (bPower) {
		pSubMenu->EnableMenuItem(4, MF_BYPOSITION | MF_ENABLED);
	}
	else {
		pSubMenu->EnableMenuItem(4, MF_BYPOSITION | MF_GRAYED);
	}

	// ƒCƒ“ƒ^ƒ‰ƒvƒgUI
	if (bPower) {
		pSubMenu->EnableMenuItem(6, MF_BYPOSITION | MF_ENABLED);
	}
	else {
		pSubMenu->EnableMenuItem(6, MF_BYPOSITION | MF_GRAYED);
	}

	// “dŒ¹ƒXƒCƒbƒ`UI
	if (bSW) {
		pSubMenu->EnableMenuItem(7, MF_BYPOSITION | MF_CHECKED);
	}
	else {
		pSubMenu->EnableMenuItem(7, MF_BYPOSITION | MF_UNCHECKED);
	}

	// ƒZƒpƒŒ[ƒ^‚ğ‚ ‚¯‚ÄA‚»‚êˆÈ~‚Ìƒƒjƒ…[‚Í‚·‚×‚Äíœ
	while (pSubMenu->GetMenuItemCount() > 9) {
		pSubMenu->RemoveMenu(9, MF_BYPOSITION);
	}

	// MRU‚ª‚È‚¯‚ê‚ÎAI—¹ƒƒjƒ…[‚ğ’Ç‰Á‚µ‚ÄI‚í‚é
	if (GetConfig()->GetMRUNum(4) == 0) {
		::GetMsg(IDS_EXIT, strExit);
		pSubMenu->AppendMenu(MF_STRING, IDM_EXIT, strExit);
		return;
	}

	// —LŒøE–³Œø’è”İ’è
	if (bPower) {
		nEnable = MF_BYCOMMAND | MF_GRAYED;
	}
	else {
		nEnable = MF_BYCOMMAND | MF_ENABLED;
	}

	// MRUˆ— - ’Ç‰Á
	for (i=0; i<9; i++) {
		// æ“¾‚µ‚Ä‚İ‚Ä
		GetConfig()->GetMRUFile(4, i, szMRU);
		if (szMRU[0] == _T('\0')) {
			break;
		}

		// ‚ ‚ê‚Îƒƒjƒ…[‚É’Ç‰Á
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

	// ƒZƒpƒŒ[ƒ^‚ğ’Ç‰Á
	pSubMenu->AppendMenu(MF_SEPARATOR, 0, (LPCTSTR)NULL);

	// I—¹ƒƒjƒ…[‚ğ’Ç‰Á
	::GetMsg(IDS_EXIT, strExit);
	pSubMenu->AppendMenu(MF_STRING, IDM_EXIT, strExit);
}

//---------------------------------------------------------------------------
//
//	ƒI[ƒvƒ“‘Oƒ`ƒFƒbƒN
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

	// ƒtƒ@ƒCƒ‹‘¶İƒ`ƒFƒbƒN
	if (!fio.Open(path, Fileio::ReadOnly)) {
		if (bWarning) {
			::GetMsg(IDS_XM6LOADFILE, strMsg);
			MessageBox(strMsg, NULL, MB_ICONSTOP | MB_OK);
		}
		return FALSE;
	}

	// ƒwƒbƒ_“Ç‚İ‚İ
	memset(cHeader, 0, sizeof(cHeader));
	fio.Read(cHeader, sizeof(cHeader));
	fio.Close();

	// ‹L˜^ƒo[ƒWƒ‡ƒ“æ“¾
	cHeader[0x0a] = '\0';
	nRecVer = ::strtoul(&cHeader[0x09], NULL, 16);
	nRecVer <<= 8;
	cHeader[0x0d] = '\0';
	nRecVer |= ::strtoul(&cHeader[0x0b], NULL, 16);

	// Œ»sƒo[ƒWƒ‡ƒ“æ“¾
	::GetVM()->GetVersion(dwMajor, dwMinor);
	nNowVer = (int)((dwMajor << 8) | dwMinor);

	// ƒwƒbƒ_ƒ`ƒFƒbƒN
	cHeader[0x09] = '\0';
	if (strcmp(cHeader, "XM6 DATA ") != 0) {
		if (bWarning) {
			::GetMsg(IDS_XM6LOADHDR, strMsg);
			MessageBox(strMsg, NULL, MB_ICONSTOP | MB_OK);
		}
		return FALSE;
	}

	// ƒo[ƒWƒ‡ƒ“ƒ`ƒFƒbƒN
	if (nNowVer < nRecVer) {
		// ‹L˜^‚³‚ê‚Ä‚¢‚éƒo[ƒWƒ‡ƒ“‚Ì‚Ù‚¤‚ªV‚µ‚¢(’m‚ç‚È‚¢Œ`®)
		::GetMsg(IDS_XM6LOADVER, strMsg);
		strFmt.Format(strMsg,
						nNowVer >> 8, nNowVer & 0xff,
						nRecVer >> 8, nRecVer & 0xff);
		MessageBox(strFmt, NULL, MB_ICONSTOP | MB_OK);
		return FALSE;
	}

	// Œp‘±
	return TRUE;
}

//---------------------------------------------------------------------------
//
//	ƒI[ƒvƒ“ƒTƒu
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

	// ƒXƒPƒWƒ…[ƒ‰’â~AƒTƒEƒ“ƒh’â~
	bRun = GetScheduler()->IsEnable();
	GetScheduler()->Enable(FALSE);
	::LockVM();
	::UnlockVM();
	bSound = GetSound()->IsEnable();
	GetSound()->Enable(FALSE);

	// ƒ[ƒh
	AfxGetApp()->BeginWaitCursor();

	// VM
	dwPos = ::GetVM()->Load(path);
	if (dwPos == 0) {
		AfxGetApp()->EndWaitCursor();

		// ¸”s‚Í“r’†’†’f‚ÅŠëŒ¯‚È‚½‚ßA•K‚¸ƒŠƒZƒbƒg‚·‚é
		::GetVM()->Reset();
		GetSound()->Enable(bSound);
		GetScheduler()->Reset();
		GetScheduler()->Enable(bRun);
		ResetCaption();

		// ƒ[ƒhƒGƒ‰[
		::GetMsg(IDS_XM6LOADERR, strMsg);
		MessageBox(strMsg, NULL, MB_ICONSTOP | MB_OK);
		return FALSE;
	}

	// MFC
	if (!LoadComponent(path, dwPos)) {
		AfxGetApp()->EndWaitCursor();

		// ¸”s‚Í“r’†’†’f‚ÅŠëŒ¯‚È‚½‚ßA•K‚¸ƒŠƒZƒbƒg‚·‚é
		::GetVM()->Reset();
		GetSound()->Enable(bSound);
		GetScheduler()->Reset();
		GetScheduler()->Enable(bRun);
		ResetCaption();

		// ƒ[ƒhƒGƒ‰[
		::GetMsg(IDS_XM6LOADERR, strMsg);
		MessageBox(strMsg, NULL, MB_ICONSTOP | MB_OK);
		return FALSE;
	}

	// ƒ[ƒhI—¹
	AfxGetApp()->EndWaitCursor();

	// FD, MO, CD‚ğMRU‚Ö’Ç‰Á(version2.04ˆÈ~‚ÌƒŒƒWƒ…[ƒ€‘Îô)
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

	// ƒXƒPƒWƒ…[ƒ‰‚ª’â~‚Ìó‘Ô‚ÅƒZ[ƒu‚³‚ê‚Ä‚¢‚ê‚ÎA’â~‚Ì‚Ü‚Ü(version2.04)
	if (GetScheduler()->HasSavedEnable()) {
		bRun = GetScheduler()->GetSavedEnable();
	}

	// ÀsƒJƒEƒ“ƒ^‚ğƒNƒŠƒA
	m_dwExec = 0;

	// ¬Œ÷
	GetSound()->Enable(bSound);
	GetScheduler()->Reset();
	GetScheduler()->Enable(bRun);
	ResetCaption();

	// MRU‚É’Ç‰Á
	GetConfig()->SetMRUFile(4, path.GetPath());

	// î•ñƒƒbƒZ[ƒW‚ğ•\¦
	::GetMsg(IDS_XM6LOADOK, strMsg);
	SetInfo(strMsg);

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	ã‘‚«•Û‘¶
//
//---------------------------------------------------------------------------
void CFrmWnd::OnSave()
{
	Filepath path;

	// VM‚©‚çƒJƒŒƒ“ƒgƒpƒX‚ğó‚¯æ‚é
	::GetVM()->GetPath(path);

	// ƒNƒŠƒA‚³‚ê‚Ä‚¢‚ê‚ÎI—¹
	if (path.IsClear()) {
		return;
	}

	// •Û‘¶ƒTƒu
	OnSaveSub(path);
}

//---------------------------------------------------------------------------
//
//	ã‘‚«•Û‘¶ UI
//
//---------------------------------------------------------------------------
void CFrmWnd::OnSaveUI(CCmdUI *pCmdUI)
{
	Filepath path;

	// “dŒ¹OFF‚Å‚ ‚ê‚Î‹Ö~
	if (!::GetVM()->IsPower()) {
		pCmdUI->Enable(FALSE);
		return;
	}

	// VM‚©‚çƒJƒŒƒ“ƒgƒpƒX‚ğó‚¯æ‚é
	::GetVM()->GetPath(path);

	// ƒNƒŠƒA‚³‚ê‚Ä‚¢‚ê‚Îg—p‹Ö~
	if (path.IsClear()) {
		pCmdUI->Enable(FALSE);
		return;
	}

	// g—p‹–‰Â
	pCmdUI->Enable(TRUE);
}

//---------------------------------------------------------------------------
//
//	–¼‘O‚ğ•t‚¯‚Ä•Û‘¶
//
//---------------------------------------------------------------------------
void CFrmWnd::OnSaveAs()
{
	Filepath path;
	TCHAR szPath[_MAX_PATH];

	// ƒRƒ‚ƒ“ƒ_ƒCƒAƒƒOÀs
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
			

	// •Û‘¶ƒTƒu
	OnSaveSub(path);
}

//---------------------------------------------------------------------------
//
//	–¼‘O‚ğ•t‚¯‚Ä•Û‘¶ UI
//
//---------------------------------------------------------------------------
void CFrmWnd::OnSaveAsUI(CCmdUI *pCmdUI)
{
	// “dŒ¹ON‚Ìê‡‚Ì‚İ
	pCmdUI->Enable(::GetVM()->IsPower());
}

//---------------------------------------------------------------------------
//
//	•Û‘¶ƒTƒu
//
//---------------------------------------------------------------------------
void FASTCALL CFrmWnd::OnSaveSub(const Filepath& path)
{
	BOOL bRun;
	BOOL bSound;
	CString strMsg;
	DWORD dwPos;

	// ƒXƒPƒWƒ…[ƒ‰’â~AƒTƒEƒ“ƒh’â~
	bRun = GetScheduler()->IsEnable();
	GetScheduler()->Enable(FALSE);
	::LockVM();
	::UnlockVM();
	bSound = GetSound()->IsEnable();
	GetSound()->Enable(FALSE);

	AfxGetApp()->BeginWaitCursor();

	// ƒXƒPƒWƒ…[ƒ‰‚É‘Î‚µ‚ÄAƒZ[ƒu‚Ìó‘Ô‚ğ’Ê’m(version2.04)
	GetScheduler()->SetSavedEnable(bRun);

	// VM
	dwPos = ::GetVM()->Save(path);
	if (dwPos== 0) {
		AfxGetApp()->EndWaitCursor();

		// ƒZ[ƒu¸”s
		GetSound()->Enable(bSound);
		GetScheduler()->Reset();
		GetScheduler()->Enable(bRun);
		ResetCaption();

		// ƒZ[ƒuƒGƒ‰[
		::GetMsg(IDS_XM6SAVEERR, strMsg);
		MessageBox(strMsg, NULL, MB_ICONSTOP | MB_OK);
		return;
	}

	// MFC
	if (!SaveComponent(path, dwPos)) {
		AfxGetApp()->EndWaitCursor();

		// ƒZ[ƒu¸”s
		GetSound()->Enable(bSound);
		GetScheduler()->Reset();
		GetScheduler()->Enable(bRun);
		ResetCaption();

		// ƒZ[ƒuƒGƒ‰[
		::GetMsg(IDS_XM6SAVEERR, strMsg);
		MessageBox(strMsg, NULL, MB_ICONSTOP | MB_OK);
		return;
	}

	// ÀsƒJƒEƒ“ƒ^‚ğƒNƒŠƒA
	m_dwExec = 0;

	AfxGetApp()->EndWaitCursor();

	// ¬Œ÷
	GetSound()->Enable(bSound);
	GetScheduler()->Reset();
	GetScheduler()->Enable(bRun);
	ResetCaption();

	// MRU‚É’Ç‰Á
	GetConfig()->SetMRUFile(4, path.GetPath());

	// î•ñƒƒbƒZ[ƒW‚ğ•\¦
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

	// uID•ÏŠ·
	uID -= IDM_XM6_MRU0;
	ASSERT(uID <= 8);

	// MRUæ“¾AƒpƒXì¬
	GetConfig()->GetMRUFile(4, (int)uID, szMRU);
	if (szMRU[0] == _T('\0')) {
		return;
	}
	path.SetPath(szMRU);

	// ƒI[ƒvƒ“‘Oˆ—
	if (!OnOpenPrep(path)) {
		return;
	}

	// ƒI[ƒvƒ“‹¤’Ê
	if (OnOpenSub(path)) {
		// ƒfƒtƒHƒ‹ƒgƒfƒBƒŒƒNƒgƒŠXV
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
	// “dŒ¹ON‚Ìê‡‚Ì‚İ
	pCmdUI->Enable(::GetVM()->IsPower());
}

//---------------------------------------------------------------------------
//
//	ƒŠƒZƒbƒg
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

	// “dŒ¹OFF‚È‚ç‘€ì•s‰Â
	if (!::GetVM()->IsPower()) {
		return;
	}

	::LockVM();

	// ƒŠƒZƒbƒg•Ä•`‰æ
	::GetVM()->Reset();
	//OutputDebugString("\n\nSe ejecuto GetVM->Reset\n\n");
	GetView()->Refresh();
	ResetCaption();

	// ƒƒ‚ƒŠƒXƒCƒbƒ`æ“¾‚ğs‚¤
	pSRAM = (SRAM*)::GetVM()->SearchDevice(MAKEID('S', 'R', 'A', 'M'));
	ASSERT(pSRAM);
	for (i=0; i<0x100; i++) {
		Sw[i] = pSRAM->ReadOnly(0xed0000 + i);
	}

	::UnlockVM();
	//OutputDebugString("\n\nSe ejecuto UnlockVm\n\n");

	// ƒŠƒZƒbƒgƒƒbƒZ[ƒW‚ğƒ[ƒh
	::GetMsg(IDS_RESET, strReset);

	// ƒƒ‚ƒŠƒXƒCƒbƒ`‚Ìæ“ª‚ğ”äŠr
	if (memcmp(Sw, SigTable, sizeof(DWORD) * 7) != 0) {
		SetInfo(strReset);
		return;
	}

	// ƒu[ƒgƒfƒoƒCƒX‚ğæ“¾
	dwDevice = Sw[0x18];
	dwDevice <<= 8;
	dwDevice |= Sw[0x19];

	// ƒu[ƒgƒfƒoƒCƒX”»•Ê
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

		// FC0000`FC001C‚ÆAEA0020`EA003C‚ÍSCSI#
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

	// •\¦
	strReset += _T(" (");
	strReset += strSub;
	SetInfo(strReset);
	OutputDebugString("\n\nSe ejecuto OnReset...\n\n");
}

//---------------------------------------------------------------------------
//
//	ƒŠƒZƒbƒg UI
//
//---------------------------------------------------------------------------
void CFrmWnd::OnResetUI(CCmdUI *pCmdUI)
{
	// “dŒ¹ON‚È‚ç‘€ì‚Å‚«‚é
	pCmdUI->Enable(::GetVM()->IsPower());
}


void CFrmWnd::OnScc()
{
	if (NombreArchivoXM6.GetLength() > 0)
	{
		m_pConfig->CustomInit();
		m_pConfig->Cleanup();

		CString sz;
		sz.Format(_T("\n\nSe ha guardado la configuración para %s\n\n"), NombreArchivoXM6);
		OutputDebugStringW(CT2W(sz));

		MessageBox(sz, "Configuración", MB_OK);
	}
	else
	{
		MessageBox("No se ha guardado configuración ya que no se cargó juego", "Configuración", MB_OK);
	}
}



void CFrmWnd::OnSccUI(CCmdUI* pCmdUI)
{
	// “dŒ¹ON‚Ìê‡‚Ì‚İ
	pCmdUI->Enable(::GetVM()->IsPower());
}

//---------------------------------------------------------------------------
//
//	SRAMƒVƒOƒlƒ`ƒƒƒe[ƒuƒ‹
//
//---------------------------------------------------------------------------
const DWORD CFrmWnd::SigTable[] = {
	0x82, 0x77, 0x36, 0x38, 0x30, 0x30, 0x30
};

//---------------------------------------------------------------------------
//
//	ƒCƒ“ƒ^ƒ‰ƒvƒg
//
//---------------------------------------------------------------------------
void CFrmWnd::OnInterrupt()
{
	CString strIntr;

	// “dŒ¹ON‚È‚ç‘€ì‚Å‚«‚é
	if (::GetVM()->IsPower()) {
		// NMIŠ„‚è‚İ
		::LockVM();
		::GetVM()->Interrupt();
		::UnlockVM();

		// ƒƒbƒZ[ƒW
		::GetMsg(IDS_INTERRUPT, strIntr);
		SetInfo(strIntr);
	}
}

//---------------------------------------------------------------------------
//
//	ƒCƒ“ƒ^ƒ‰ƒvƒg UI
//
//---------------------------------------------------------------------------
void CFrmWnd::OnInterruptUI(CCmdUI *pCmdUI)
{
	// “dŒ¹ON‚È‚ç‘€ì‚Å‚«‚é
	pCmdUI->Enable(::GetVM()->IsPower());
}

//---------------------------------------------------------------------------
//
//	“dŒ¹ƒXƒCƒbƒ`
//
//---------------------------------------------------------------------------
void CFrmWnd::OnPower()
{
	BOOL bPower;

	::LockVM();

	if (::GetVM()->IsPowerSW()) {
		// ƒIƒ“‚È‚çƒIƒt
		::GetVM()->PowerSW(FALSE);
		::UnlockVM();
		return;
	}

	// Œ»İ‚Ì“dŒ¹‚Ìó‘Ô‚ğ•Û‘¶‚µ‚ÄA“dŒ¹ON
	bPower = ::GetVM()->IsPower();
	::GetVM()->PowerSW(TRUE);

	// “dŒ¹‚ªØ‚ê‚Ä‚¢‚ÄƒXƒPƒWƒ…[ƒ‰‚ª~‚Ü‚Á‚Ä‚¢‚ê‚ÎA“®‚©‚·
	if (!bPower && !GetScheduler()->IsEnable()) {
		GetScheduler()->Enable(TRUE);
	}

	::UnlockVM();

	// ƒŠƒZƒbƒg(ƒXƒe[ƒ^ƒXƒo[•\¦‚Ì‚½‚ß)
	OnReset();
}

//---------------------------------------------------------------------------
//
//	“dŒ¹ƒXƒCƒbƒ` UI
//
//---------------------------------------------------------------------------
void CFrmWnd::OnPowerUI(CCmdUI *pCmdUI)
{
	// ‚Æ‚è‚ ‚¦‚¸AƒIƒ“‚È‚çƒ`ƒFƒbƒN‚µ‚Ä‚¨‚­
	pCmdUI->SetCheck(::GetVM()->IsPowerSW());
}

//---------------------------------------------------------------------------
//
//	I—¹
//
//---------------------------------------------------------------------------
void CFrmWnd::OnExit()
{
	PostMessage(WM_CLOSE, 0, 0);
}

//---------------------------------------------------------------------------
//
//	ƒtƒƒbƒs[ƒfƒBƒXƒNˆ—
//
//---------------------------------------------------------------------------
void CFrmWnd::OnFD(UINT uID)
{
	int nDrive;

	// ƒhƒ‰ƒCƒuŒˆ’è
	nDrive = 0;
	if (uID >= IDM_D1OPEN) {
		nDrive = 1;
		uID -= (IDM_D1OPEN - IDM_D0OPEN);
	}

	switch (uID) {
		// ƒI[ƒvƒ“
		case IDM_D0OPEN:
			OnFDOpen(nDrive);
			break;

		// ƒCƒWƒFƒNƒg
		case IDM_D0EJECT:
			OnFDEject(nDrive);
			break;

		// ‘‚«‚İ•ÛŒì
		case IDM_D0WRITEP:
			OnFDWriteP(nDrive);
			break;

		// ‹­§ƒCƒWƒFƒNƒg
		case IDM_D0FORCE:
			OnFDForce(nDrive);
			break;

		// Œë‘}“ü
		case IDM_D0INVALID:
			OnFDInvalid(nDrive);
			break;

		// ‚»‚êˆÈŠO
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
//	ƒtƒƒbƒs[ƒI[ƒvƒ“
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

	// ƒRƒ‚ƒ“ƒ_ƒCƒAƒƒOÀs
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

			

	// VMƒƒbƒN
	::LockVM();

	// ƒfƒBƒXƒNŠ„‚è“–‚Ä
	if (!m_pFDD->Open(nDrive, path)) {
		GetScheduler()->Reset();
		::UnlockVM();

		// ƒI[ƒvƒ“ƒGƒ‰[
		::GetMsg(IDS_FDERR, strMsg);
		MessageBox(strMsg, NULL, MB_ICONSTOP | MB_OK);
		ResetCaption();
		return;
	}

	// VM‚ğƒŠƒXƒ^[ƒg‚³‚¹‚é‘O‚ÉAFDI‚ğæ“¾‚µ‚Ä‚¨‚­
	pFDI = m_pFDD->GetFDI(nDrive);

	// ¬Œ÷
	GetScheduler()->Reset();
	ResetCaption();
	::UnlockVM();

	// MRU‚É’Ç‰Á
	GetConfig()->SetMRUFile(nDrive, szPath);

	// ¬Œ÷‚È‚çABADƒCƒ[ƒWŒx
	if (pFDI->GetID() == MAKEID('B', 'A', 'D', ' ')) {
		::GetMsg(IDS_BADFDI_WARNING, strMsg);
		MessageBox(strMsg, NULL, MB_ICONSTOP | MB_OK);
	}
}

//---------------------------------------------------------------------------
//
//	ƒtƒƒbƒs[ƒCƒWƒFƒNƒg
//
//---------------------------------------------------------------------------
void FASTCALL CFrmWnd::OnFDEject(int nDrive)
{
	ASSERT(m_pFDD);
	ASSERT((nDrive == 0) || (nDrive == 1));

	// VM‚ğƒƒbƒN‚µ‚Äs‚¤
	::LockVM();
	m_pFDD->Eject(nDrive, FALSE);
	::UnlockVM();
}

//---------------------------------------------------------------------------
//
//	ƒtƒƒbƒs[‘‚«‚İ•ÛŒì
//
//---------------------------------------------------------------------------
void FASTCALL CFrmWnd::OnFDWriteP(int nDrive)
{
	ASSERT(m_pFDD);
	ASSERT((nDrive == 0) || (nDrive == 1));

	// ƒCƒ[ƒW‚ğ‘€ì
	::LockVM();
	m_pFDD->WriteP(nDrive, !m_pFDD->IsWriteP(nDrive));
	::UnlockVM();
}

//---------------------------------------------------------------------------
//
//	ƒtƒƒbƒs[‹­§ƒCƒWƒFƒNƒg
//
//---------------------------------------------------------------------------
void FASTCALL CFrmWnd::OnFDForce(int nDrive)
{
	ASSERT(m_pFDD);
	ASSERT((nDrive == 0) || (nDrive == 1));

	// VM‚ğƒƒbƒN‚µ‚Äs‚¤
	::LockVM();
	m_pFDD->Eject(nDrive, TRUE);
	::UnlockVM();
}

//---------------------------------------------------------------------------
//
//	ƒtƒƒbƒs[Œë‘}“ü
//
//---------------------------------------------------------------------------
void FASTCALL CFrmWnd::OnFDInvalid(int nDrive)
{
	ASSERT(m_pFDD);
	ASSERT((nDrive == 0) || (nDrive == 1));

	// VM‚ğƒƒbƒN‚µ‚Äs‚¤
	::LockVM();
	m_pFDD->Invalid(nDrive);
	::UnlockVM();
}

//---------------------------------------------------------------------------
//
//	ƒtƒƒbƒs[ƒƒfƒBƒA
//
//---------------------------------------------------------------------------
void FASTCALL CFrmWnd::OnFDMedia(int nDrive, int nMedia)
{
	Filepath path;

	ASSERT((nDrive == 0) || (nDrive == 1));
	ASSERT((nMedia >= 0) && (nMedia <= 15));

	// VMƒƒbƒN
	::LockVM();

	// ”O‚Ì‚½‚ßŠm”F
	if (nMedia < m_pFDD->GetDisks(nDrive)) {
		m_pFDD->GetPath(nDrive, path);

		// ÄƒI[ƒvƒ“
		m_pFDD->Open(nDrive, path, nMedia);
	}

	// VMƒAƒ“ƒƒbƒN
	::UnlockVM();
}

//---------------------------------------------------------------------------
//
//	ƒtƒƒbƒs[MRU
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

	// MRUæ“¾AƒpƒXì¬
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
	

	// VMƒƒbƒN
	::LockVM();

	// ƒfƒBƒXƒNŠ„‚è“–‚Ä‚ğ‚İ‚é
	bResult = m_pFDD->Open(nDrive, path);
	pFDI = m_pFDD->GetFDI(nDrive);
	GetScheduler()->Reset();
	ResetCaption();

	// VMƒAƒ“ƒƒbƒN
	::UnlockVM();

	// ¬Œ÷‚·‚ê‚ÎAƒfƒBƒŒƒNƒgƒŠXV‚ÆMRU’Ç‰Á
	if (bResult) {
		// ƒfƒtƒHƒ‹ƒgƒfƒBƒŒƒNƒgƒŠXV
		Filepath::SetDefaultDir(szMRU);

		// MRU‚É’Ç‰Á
		GetConfig()->SetMRUFile(nDrive, szMRU);

		// BADƒCƒ[ƒWŒx
		if (pFDI->GetID() == MAKEID('B', 'A', 'D', ' ')) {
			::GetMsg(IDS_BADFDI_WARNING, strMsg);
			MessageBox(strMsg, NULL, MB_ICONSTOP | MB_OK);
		}
	}
}

//---------------------------------------------------------------------------
//
//	ƒtƒƒbƒs[ƒI[ƒvƒ“ UI
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

	// ƒhƒ‰ƒCƒuŒˆ’è
	nDrive = 0;
	if (pCmdUI->m_nID >= IDM_D1OPEN) {
		nDrive = 1;
	}

	// ƒCƒWƒFƒNƒg‹Ö~‚ÅAƒfƒBƒXƒN‚ ‚èˆÈŠO‚ÍƒI[ƒvƒ“‚Å‚«‚é
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

	// ƒTƒuƒƒjƒ…[æ“¾
	if (m_bPopupMenu) {
		pMenu = m_PopupMenu.GetSubMenu(0);
	}
	else {
		pMenu = &m_Menu;
	}
	// ƒtƒ@ƒCƒ‹(F)‚ÌŸ‚Éƒtƒƒbƒs[0Aƒtƒƒbƒs[1‚Æ•À‚Ô
	pSubMenu = pMenu->GetSubMenu(nDrive + 1);

	// ƒCƒWƒFƒNƒgUI(ˆÈ‰ºAON_UPDATE_COMMAND_UI‚Ìƒ^ƒCƒ~ƒ“ƒO‘Îô)
	if ((nStat & FDST_INSERT) && (nStat & FDST_EJECT)) {
		pSubMenu->EnableMenuItem(1, MF_BYPOSITION | MF_ENABLED);
	}
	else {
		pSubMenu->EnableMenuItem(1, MF_BYPOSITION | MF_GRAYED);
	}

	// ‘‚«‚İ•ÛŒìUI
	if (m_pFDD->IsReadOnly(nDrive) || !(nStat & FDST_INSERT)) {
		pSubMenu->EnableMenuItem(2, MF_BYPOSITION | MF_GRAYED);
	}
	else {
		pSubMenu->EnableMenuItem(2, MF_BYPOSITION | MF_ENABLED);
	}

	// ‹­§ƒCƒWƒFƒNƒgUI
	if (!(nStat & FDST_EJECT) && (nStat & FDST_INSERT)) {
		pSubMenu->EnableMenuItem(4, MF_BYPOSITION | MF_ENABLED);
	}
	else {
		pSubMenu->EnableMenuItem(4, MF_BYPOSITION | MF_GRAYED);
	}

	// Œë‘}“üUI
	if (!(nStat & FDST_INSERT) && !(nStat & FDST_INVALID)) {
		pSubMenu->EnableMenuItem(5, MF_BYPOSITION | MF_ENABLED);
	}
	else {
		pSubMenu->EnableMenuItem(5, MF_BYPOSITION | MF_GRAYED);
	}

	// ˆÈ~‚Ìƒƒjƒ…[‚Í‚·‚×‚Äíœ
	while (pSubMenu->GetMenuItemCount() > 6) {
		pSubMenu->RemoveMenu(6, MF_BYPOSITION);
	}

	// ƒ}ƒ‹ƒ`ƒfƒBƒXƒNˆ—
	if (nDisks > 1) {
		// —LŒøE–³Œø’è”İ’è
		if (!(nStat & FDST_EJECT) && (nStat & FDST_INSERT)) {
			nEnable = MF_BYCOMMAND | MF_ENABLED;
		}
		else {
			nEnable = MF_BYCOMMAND | MF_GRAYED;
		}

		// ƒZƒpƒŒ[ƒ^‚ğ‘}“ü
		pSubMenu->AppendMenu(MF_SEPARATOR, 0, (LPCTSTR)NULL);

		// ƒƒfƒBƒAƒ‹[ƒv
		ASSERT(nDisks <= 16);
		for (i=0; i<nDisks; i++) {
			// ƒfƒBƒXƒN–¼‚Íchar*‚ÅŠi”[‚³‚ê‚Ä‚¢‚éˆ×ATCHAR‚Ö•ÏŠ·
			m_pFDD->GetName(nDrive, szShort, i);
			lpszShort = A2T(szShort);

			// ’Ç‰Á
			if (nDrive == 0) {
				pSubMenu->AppendMenu(MF_STRING, IDM_D0_MEDIA0 + i, lpszShort);
				pSubMenu->EnableMenuItem(IDM_D0_MEDIA0 + i, nEnable);
			}
			else {
				pSubMenu->AppendMenu(MF_STRING, IDM_D1_MEDIA0 + i, lpszShort);
				pSubMenu->EnableMenuItem(IDM_D1_MEDIA0 + i, nEnable);
			}
		}

		// ƒ‰ƒWƒIƒ{ƒ^ƒ“İ’è
		if (nDrive == 0) {
			pSubMenu->CheckMenuRadioItem(IDM_D0_MEDIA0, IDM_D0_MEDIAF,
										IDM_D0_MEDIA0 + nMedia, MF_BYCOMMAND);
		}
		else {
			pSubMenu->CheckMenuRadioItem(IDM_D1_MEDIA0, IDM_D1_MEDIAF,
										IDM_D1_MEDIA0 + nMedia, MF_BYCOMMAND);
		}
	}

	// MRUˆ— - ƒZƒpƒŒ[ƒ^
	if (GetConfig()->GetMRUNum(nDrive) == 0) {
		return;
	}
	pSubMenu->AppendMenu(MF_SEPARATOR, 0, (LPCTSTR)NULL);

	// —LŒøE–³Œø’è”İ’è
	if (!(nStat & FDST_EJECT) && (nStat & FDST_INSERT)) {
		nEnable = MF_BYCOMMAND | MF_GRAYED;
	}
	else {
		nEnable = MF_BYCOMMAND | MF_ENABLED;
	}


	

	// MRUˆ— - ’Ç‰Á
	for (i=0; i<9; i++) {
		// æ“¾‚µ‚Ä‚İ‚Ä
		GetConfig()->GetMRUFile(nDrive, i, szMRU);
		if (szMRU[0] == _T('\0')) {
			break;
		}

		// ‚ ‚ê‚Îƒƒjƒ…[‚É’Ç‰Á
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
//	ƒtƒƒbƒs[ƒCƒWƒFƒNƒg UI
//
//---------------------------------------------------------------------------
void CFrmWnd::OnFDEjectUI(CCmdUI *pCmdUI)
{
	int nDrive;
	int nStat;

	ASSERT(m_pFDD);

	// ƒhƒ‰ƒCƒuŒˆ’è
	nDrive = 0;
	if (pCmdUI->m_nID >= IDM_D1OPEN) {
		nDrive = 1;
	}

	// ƒXƒe[ƒ^ƒXæ“¾
	nStat = m_nFDDStatus[nDrive];

	// ƒCƒ“ƒT[ƒg’†‚ÅAƒCƒWƒFƒNƒg‹Ö~‚Å‚È‚¯‚ê‚ÎƒCƒWƒFƒNƒg‚Å‚«‚é
	if ((nStat & FDST_INSERT) && (nStat & FDST_EJECT)) {
		pCmdUI->Enable(TRUE);
		return;
	}
	pCmdUI->Enable(FALSE);
}

//---------------------------------------------------------------------------
//
//	ƒtƒƒbƒs[‘‚«‚İ•ÛŒì UI
//
//---------------------------------------------------------------------------
void CFrmWnd::OnFDWritePUI(CCmdUI *pCmdUI)
{
	int nDrive;
	int nStat;

	ASSERT(m_pFDD);

	// ƒhƒ‰ƒCƒuŒˆ’è
	nDrive = 0;
	if (pCmdUI->m_nID >= IDM_D1OPEN) {
		nDrive = 1;
	}

	// ƒXƒe[ƒ^ƒXæ“¾
	nStat = m_nFDDStatus[nDrive];

	// ‘‚«‚İ•ÛŒì‚É]‚¢ƒ`ƒFƒbƒN
	pCmdUI->SetCheck(m_pFDD->IsWriteP(nDrive));

	// ƒŠ[ƒhƒIƒ“ƒŠ[‚©AƒCƒ“ƒT[ƒg‚³‚ê‚Ä‚¢‚È‚¯‚ê‚Î–³Œø
	if (m_pFDD->IsReadOnly(nDrive) || !(nStat & FDST_INSERT)) {
		pCmdUI->Enable(FALSE);
		return;
	}
	pCmdUI->Enable(TRUE);
}

//---------------------------------------------------------------------------
//
//	ƒtƒƒbƒs[‹­§ƒCƒWƒFƒNƒg UI
//
//---------------------------------------------------------------------------
void CFrmWnd::OnFDForceUI(CCmdUI *pCmdUI)
{
	int nDrive;
	int nStat;

	ASSERT(m_pFDD);

	// ƒhƒ‰ƒCƒuŒˆ’è
	nDrive = 0;
	if (pCmdUI->m_nID >= IDM_D1OPEN) {
		nDrive = 1;
	}

	// ƒXƒe[ƒ^ƒXæ“¾
	nStat = m_nFDDStatus[nDrive];

	// ƒCƒWƒFƒNƒg‹Ö~‚Ì‚Ì‚İ—LŒø
	if (!(nStat & FDST_EJECT) && (nStat & FDST_INSERT)) {
		pCmdUI->Enable(TRUE);
		return;
	}
	pCmdUI->Enable(FALSE);
}

//---------------------------------------------------------------------------
//
//	ƒtƒƒbƒs[Œë‘}“ü UI
//
//---------------------------------------------------------------------------
void CFrmWnd::OnFDInvalidUI(CCmdUI *pCmdUI)
{
	int nDrive;
	int nStat;

	ASSERT(m_pFDD);

	// ƒhƒ‰ƒCƒuŒˆ’è
	nDrive = 0;
	if (pCmdUI->m_nID >= IDM_D1OPEN) {
		nDrive = 1;
	}

	// ƒXƒe[ƒ^ƒXæ“¾
	nStat = m_nFDDStatus[nDrive];

	// ‘}“ü‚³‚ê‚Ä‚¢‚È‚¢‚Ì‚İ—LŒø
	if (!(nStat & FDST_INSERT) && !(nStat & FDST_INVALID)) {
		pCmdUI->Enable(TRUE);
		return;
	}
	pCmdUI->Enable(FALSE);
}

//---------------------------------------------------------------------------
//
//	ƒtƒƒbƒs[ƒƒfƒBƒA UI
//
//---------------------------------------------------------------------------
void CFrmWnd::OnFDMediaUI(CCmdUI *pCmdUI)
{
	int nDrive;
	int nStat;

	ASSERT(m_pFDD);

	// ƒhƒ‰ƒCƒuŒˆ’è
	nDrive = 0;
	if (pCmdUI->m_nID >= IDM_D1OPEN) {
		nDrive = 1;
	}

	// ƒXƒe[ƒ^ƒXæ“¾
	nStat = m_nFDDStatus[nDrive];

	// ƒCƒWƒFƒNƒg‹Ö~‚ÅAƒfƒBƒXƒN‚ ‚èˆÈŠO‚ÍƒI[ƒvƒ“‚Å‚«‚é
	if (!(nStat & FDST_EJECT) && (nStat & FDST_INSERT)) {
		pCmdUI->Enable(FALSE);
	}
	else {
		pCmdUI->Enable(TRUE);
	}
}

//---------------------------------------------------------------------------
//
//	ƒtƒƒbƒs[MRU UI
//
//---------------------------------------------------------------------------
void CFrmWnd::OnFDMRUUI(CCmdUI *pCmdUI)
{
	int nDrive;
	int nStat;

	ASSERT(m_pFDD);

	// ƒhƒ‰ƒCƒuŒˆ’è
	nDrive = 0;
	if (pCmdUI->m_nID >= IDM_D1OPEN) {
		nDrive = 1;
	}

	// ƒXƒe[ƒ^ƒXæ“¾
	nStat = m_nFDDStatus[nDrive];

	// ƒCƒWƒFƒNƒg‹Ö~‚ÅAƒfƒBƒXƒN‚ ‚èˆÈŠO‚ÍƒI[ƒvƒ“‚Å‚«‚é
	if (!(nStat & FDST_EJECT) && (nStat & FDST_INSERT)) {
		pCmdUI->Enable(FALSE);
	}
	else {
		pCmdUI->Enable(TRUE);
	}
}

//---------------------------------------------------------------------------
//
//	MOƒfƒBƒXƒNƒI[ƒvƒ“
//
//---------------------------------------------------------------------------
void CFrmWnd::OnMOOpen()
{
	Filepath path;
	TCHAR szPath[_MAX_PATH];
	CString strMsg;

	ASSERT(this);
	ASSERT(m_pSASI);

	// ƒRƒ‚ƒ“ƒ_ƒCƒAƒƒOÀs
	memset(szPath, 0, sizeof(szPath));
	if (!::FileOpenDlg(this, szPath, IDS_MOOPEN)) {
		ResetCaption();
		return;
	}
	path.SetPath(szPath);

	// VMƒƒbƒN
	::LockVM();

	// MOƒfƒBƒXƒNŠ„‚è“–‚Ä
	if (!m_pSASI->Open(path)) {
		GetScheduler()->Reset();
		::UnlockVM();

		// ƒI[ƒvƒ“ƒGƒ‰[
		::GetMsg(IDS_MOERR, strMsg);
		MessageBox(strMsg, NULL, MB_ICONSTOP | MB_OK);
		ResetCaption();
		return;
	}

	// ¬Œ÷
	GetScheduler()->Reset();
	ResetCaption();
	::UnlockVM();

	// MRU‚É’Ç‰Á
	GetConfig()->SetMRUFile(2, szPath);
}

//---------------------------------------------------------------------------
//
//	MOƒfƒBƒXƒNƒI[ƒvƒ“ UI
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

	// ƒhƒ‰ƒCƒu‚Ìó‘Ô‚ğæ“¾(ƒƒbƒN‚µ‚Äs‚¤)
	::LockVM();
	bValid = m_pSASI->IsValid();
	bReady = m_pSASI->IsReady();
	bWriteP = m_pSASI->IsWriteP();
	bReadOnly = m_pSASI->IsReadOnly();
	bLocked = m_pSASI->IsLocked();
	::UnlockVM();

	// ƒI[ƒvƒ“
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

	// ƒTƒuƒƒjƒ…[æ“¾
	if (m_bPopupMenu) {
		pMenu = m_PopupMenu.GetSubMenu(0);
	}
	else {
		pMenu = &m_Menu;
	}
	ASSERT(pMenu);
	// MOƒƒjƒ…[‚Íƒtƒ@ƒCƒ‹Aƒtƒƒbƒs[0Aƒtƒƒbƒs[1‚ÌŸ
	pSubMenu = pMenu->GetSubMenu(3);
	ASSERT(pSubMenu);

	// ƒCƒWƒFƒNƒgUI(ˆÈ‰ºAON_UPDATE_COMMAND_UI‚Ìƒ^ƒCƒ~ƒ“ƒO‘Îô)
	if (bReady && !bLocked) {
		pSubMenu->EnableMenuItem(1, MF_BYPOSITION | MF_ENABLED);
	}
	else {
		pSubMenu->EnableMenuItem(1, MF_BYPOSITION | MF_GRAYED);
	}

	// ‘‚«‚İ•ÛŒìUI
	if (bReady && !bReadOnly) {
		pSubMenu->EnableMenuItem(2, MF_BYPOSITION | MF_GRAYED);
	}
	else {
		pSubMenu->EnableMenuItem(2, MF_BYPOSITION | MF_ENABLED);
	}

	// ‹­§ƒCƒWƒFƒNƒgUI
	if (bReady && bLocked) {
		pSubMenu->EnableMenuItem(4, MF_BYPOSITION | MF_ENABLED);
	}
	else {
		pSubMenu->EnableMenuItem(4, MF_BYPOSITION | MF_GRAYED);
	}

	// ˆÈ~‚Ìƒƒjƒ…[‚Í‚·‚×‚Äíœ
	while (pSubMenu->GetMenuItemCount() > 5) {
		pSubMenu->RemoveMenu(5, MF_BYPOSITION);
	}

	// MRUˆ— - ƒZƒpƒŒ[ƒ^
	if (GetConfig()->GetMRUNum(2) == 0) {
		return;
	}
	pSubMenu->AppendMenu(MF_SEPARATOR, 0, (LPCTSTR)NULL);

	// —LŒøE–³Œø’è”İ’è
	nEnable = MF_BYCOMMAND | MF_GRAYED;
	if (bValid) {
		// ƒhƒ‰ƒCƒu—LŒø‚Å
		if (!bReady || !bLocked) {
			// ƒƒfƒBƒA‚ª‘¶İ‚µ‚È‚¢ or ƒƒbƒN‚³‚ê‚Ä‚¢‚È‚¢ ‚È‚çƒCƒ“ƒT[ƒg‚Å‚«‚é
			nEnable = MF_BYCOMMAND | MF_ENABLED;
		}
	}

	// MRUˆ— - ’Ç‰Á
	for (i=0; i<9; i++) {
		// æ“¾‚µ‚Ä‚İ‚Ä
		GetConfig()->GetMRUFile(2, i, szMRU);
		if (szMRU[0] == _T('\0')) {
			break;
		}

		// ‚ ‚ê‚Îƒƒjƒ…[‚É’Ç‰Á
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
//	MOƒfƒBƒXƒNƒCƒWƒFƒNƒg
//
//---------------------------------------------------------------------------
void CFrmWnd::OnMOEject()
{
	ASSERT(this);
	ASSERT(m_pSASI);

	// VM‚ğƒƒbƒN‚µ‚Äs‚¤
	::LockVM();
	m_pSASI->Eject(FALSE);
	::UnlockVM();
}

//---------------------------------------------------------------------------
//
//	MOƒfƒBƒXƒNƒCƒWƒFƒNƒg UI
//
//---------------------------------------------------------------------------
void CFrmWnd::OnMOEjectUI(CCmdUI *pCmdUI)
{
	ASSERT(this);
	ASSERT(m_pSASI);

	// ƒŒƒfƒB‚Å‚È‚¯‚ê‚Î‹Ö~
	if (!m_pSASI->IsReady()) {
		pCmdUI->Enable(FALSE);
		return;
	}

	// ƒƒbƒN‚³‚ê‚Ä‚¢‚ê‚Î‹Ö~
	if (m_pSASI->IsLocked()) {
		pCmdUI->Enable(FALSE);
		return;
	}

	// ‹–‰Â
	pCmdUI->Enable(TRUE);
}

//---------------------------------------------------------------------------
//
//	MOƒfƒBƒXƒN‘‚«‚İ•ÛŒì
//
//---------------------------------------------------------------------------
void CFrmWnd::OnMOWriteP()
{
	ASSERT(this);
	ASSERT(m_pSASI);

	// VM‚ğƒƒbƒN‚µ‚Äs‚¤
	::LockVM();
	m_pSASI->WriteP(!m_pSASI->IsWriteP());
	::UnlockVM();
}

//---------------------------------------------------------------------------
//
//	MOƒfƒBƒXƒN‘‚«‚İ•ÛŒì UI
//
//---------------------------------------------------------------------------
void CFrmWnd::OnMOWritePUI(CCmdUI *pCmdUI)
{
	ASSERT(this);
	ASSERT(m_pSASI);

	// ƒŒƒfƒB‚Å‚È‚¯‚ê‚Îƒ`ƒFƒbƒN‚È‚µA‹Ö~
	if (!m_pSASI->IsReady()) {
		pCmdUI->SetCheck(0);
		pCmdUI->Enable(FALSE);
		return;
	}

	// Read Only‚Å‚ ‚ê‚Îƒ`ƒFƒbƒN‚ ‚èA‹Ö~
	if (m_pSASI->IsReadOnly()) {
		pCmdUI->SetCheck(1);
		pCmdUI->Enable(FALSE);
		return;
	}

	// Œ»ó‚É‰‚¶‚Äƒ`ƒFƒbƒN‚µ‚ÄA‹–‰Â
	pCmdUI->SetCheck(m_pSASI->IsWriteP());
	pCmdUI->Enable(TRUE);
}

//---------------------------------------------------------------------------
//
//	MOƒfƒBƒXƒN‹­§ƒCƒWƒFƒNƒg
//
//---------------------------------------------------------------------------
void CFrmWnd::OnMOForce()
{
	ASSERT(this);
	ASSERT(m_pSASI);

	// VM‚ğƒƒbƒN‚µ‚Äs‚¤
	::LockVM();
	m_pSASI->Eject(TRUE);
	::UnlockVM();
}

//---------------------------------------------------------------------------
//
//	MOƒfƒBƒXƒN‹­§ƒCƒWƒFƒNƒg UI
//
//---------------------------------------------------------------------------
void CFrmWnd::OnMOForceUI(CCmdUI *pCmdUI)
{
	ASSERT(this);
	ASSERT(m_pSASI);

	// ƒŒƒfƒB‚Å‚È‚¯‚ê‚Î‹Ö~
	if (!m_pSASI->IsReady()) {
		pCmdUI->Enable(FALSE);
		return;
	}

	// ƒƒbƒN‚³‚ê‚Ä‚¢‚È‚¯‚ê‚Î‹Ö~
	if (!m_pSASI->IsLocked()) {
		pCmdUI->Enable(FALSE);
		return;
	}

	// ‹–‰Â
	pCmdUI->Enable(TRUE);
}

//---------------------------------------------------------------------------
//
//	MOƒfƒBƒXƒNMRU
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

	// ƒCƒ“ƒfƒbƒNƒXì¬
	nMRU = uID - IDM_MO_MRU0;
	ASSERT((nMRU >= 0) && (nMRU <= 8));

	// MRUæ“¾
	GetConfig()->GetMRUFile(2, nMRU, szMRU);
	if (szMRU[0] == _T('\0')) {
		return;
	}
	path.SetPath(szMRU);

	// VMƒƒbƒN
	::LockVM();

	// ƒI[ƒvƒ“
	bResult = m_pSASI->Open(path);
	GetScheduler()->Reset();
	ResetCaption();
	::UnlockVM();

	// ¬Œ÷‚·‚ê‚ÎAƒfƒBƒŒƒNƒgƒŠXV‚ÆMRU’Ç‰Á
	if (bResult) {
		// ƒCƒjƒVƒƒƒ‹ƒfƒBƒŒƒNƒgƒŠXV
		Filepath::SetDefaultDir(szMRU);

		// MRU‚É’Ç‰Á
		GetConfig()->SetMRUFile(2, szMRU);
	}
}

//---------------------------------------------------------------------------
//
//	MOƒfƒBƒXƒNMRU UI
//
//---------------------------------------------------------------------------
void CFrmWnd::OnMOMRUUI(CCmdUI *pCmdUI)
{
	ASSERT(this);
	ASSERT(m_pSASI);

	// ƒhƒ‰ƒCƒu‚ª—LŒø‚Å‚È‚¯‚Î–³Œø
	if (!m_pSASI->IsValid()) {
		pCmdUI->Enable(FALSE);
		return;
	}

	// ƒƒbƒN‚³‚ê‚Ä‚¢‚ê‚Î‹Ö~
	if (m_pSASI->IsLocked()) {
		pCmdUI->Enable(FALSE);
		return;
	}

	// ‹–‰Â
	pCmdUI->Enable(TRUE);
}

//---------------------------------------------------------------------------
//
//	CD-ROMƒI[ƒvƒ“
//
//---------------------------------------------------------------------------
void CFrmWnd::OnCDOpen()
{
	Filepath path;
	TCHAR szPath[_MAX_PATH];
	CString strMsg;

	ASSERT(this);
	ASSERT(m_pSCSI);

	// ƒRƒ‚ƒ“ƒ_ƒCƒAƒƒOÀs
	memset(szPath, 0, sizeof(szPath));
	if (!::FileOpenDlg(this, szPath, IDS_CDOPEN)) {
		ResetCaption();
		return;
	}
	path.SetPath(szPath);

	// VMƒƒbƒN
	::LockVM();

	// CDƒfƒBƒXƒNŠ„‚è“–‚Ä
	if (!m_pSCSI->Open(path, FALSE)) {
		GetScheduler()->Reset();
		::UnlockVM();

		// ƒI[ƒvƒ“ƒGƒ‰[
		::GetMsg(IDS_CDERR, strMsg);
		MessageBox(strMsg, NULL, MB_ICONSTOP | MB_OK);
		ResetCaption();
		return;
	}

	// ¬Œ÷
	GetScheduler()->Reset();
	ResetCaption();
	::UnlockVM();

	// MRU‚É’Ç‰Á
	GetConfig()->SetMRUFile(3, szPath);
}

//---------------------------------------------------------------------------
//
//	CD-ROMƒI[ƒvƒ“ UI
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

	// ƒhƒ‰ƒCƒu‚Ìó‘Ô‚ğæ“¾(ƒƒbƒN‚µ‚Äs‚¤)
	::LockVM();
	bValid = m_pSCSI->IsValid(FALSE);
	bReady = m_pSCSI->IsReady(FALSE);
	bLocked = m_pSCSI->IsLocked(FALSE);
	::UnlockVM();

	// ƒI[ƒvƒ“
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

	// ƒTƒuƒƒjƒ…[æ“¾
	if (m_bPopupMenu) {
		pMenu = m_PopupMenu.GetSubMenu(0);
	}
	else {
		pMenu = &m_Menu;
	}
	ASSERT(pMenu);

	// CDƒƒjƒ…[‚Íƒtƒ@ƒCƒ‹Aƒtƒƒbƒs[0Aƒtƒƒbƒs[1AMO‚ÌŸ
	pSubMenu = pMenu->GetSubMenu(4);
	ASSERT(pSubMenu);

	// ƒCƒWƒFƒNƒgUI(ˆÈ‰ºAON_UPDATE_COMMAND_UI‚Ìƒ^ƒCƒ~ƒ“ƒO‘Îô)
	if (bReady && !bLocked) {
		pSubMenu->EnableMenuItem(1, MF_BYPOSITION | MF_ENABLED);
	}
	else {
		pSubMenu->EnableMenuItem(1, MF_BYPOSITION | MF_GRAYED);
	}

	// ‹­§ƒCƒWƒFƒNƒgUI
	if (bReady && bLocked) {
		pSubMenu->EnableMenuItem(3, MF_BYPOSITION | MF_ENABLED);
	}
	else {
		pSubMenu->EnableMenuItem(3, MF_BYPOSITION | MF_GRAYED);
	}

	// ˆÈ~‚Ìƒƒjƒ…[‚Í‚·‚×‚Äíœ
	while (pSubMenu->GetMenuItemCount() > 4) {
		pSubMenu->RemoveMenu(4, MF_BYPOSITION);
	}

	// MRUˆ— - ƒZƒpƒŒ[ƒ^
	if (GetConfig()->GetMRUNum(3) == 0) {
		return;
	}
	pSubMenu->AppendMenu(MF_SEPARATOR, 0, (LPCTSTR)NULL);

	// —LŒøE–³Œø’è”İ’è
	nEnable = MF_BYCOMMAND | MF_GRAYED;
	if (bValid) {
		// ƒhƒ‰ƒCƒu—LŒø‚Å
		if (!bReady || !bLocked) {
			// ƒƒfƒBƒA‚ª‘¶İ‚µ‚È‚¢ or ƒƒbƒN‚³‚ê‚Ä‚¢‚È‚¢ ‚È‚çƒCƒ“ƒT[ƒg‚Å‚«‚é
			nEnable = MF_BYCOMMAND | MF_ENABLED;
		}
	}

	// MRUˆ— - ’Ç‰Á
	for (i=0; i<9; i++) {
		// æ“¾‚µ‚Ä‚İ‚Ä
		GetConfig()->GetMRUFile(3, i, szMRU);
		if (szMRU[0] == _T('\0')) {
			break;
		}

		// ‚ ‚ê‚Îƒƒjƒ…[‚É’Ç‰Á
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
//	CD-ROMƒCƒWƒFƒNƒg
//
//---------------------------------------------------------------------------
void CFrmWnd::OnCDEject()
{
	ASSERT(this);
	ASSERT(m_pSCSI);

	// VM‚ğƒƒbƒN‚µ‚Äs‚¤
	::LockVM();
	m_pSCSI->Eject(FALSE, FALSE);
	::UnlockVM();
}

//---------------------------------------------------------------------------
//
//	CD-ROMƒCƒWƒFƒNƒg UI
//
//---------------------------------------------------------------------------
void CFrmWnd::OnCDEjectUI(CCmdUI *pCmdUI)
{
	ASSERT(this);
	ASSERT(m_pSCSI);

	// ƒŒƒfƒB‚Å‚È‚¯‚ê‚Î‹Ö~
	if (!m_pSCSI->IsReady(FALSE)) {
		pCmdUI->Enable(FALSE);
		return;
	}

	// ƒƒbƒN‚³‚ê‚Ä‚¢‚ê‚Î‹Ö~
	if (m_pSCSI->IsLocked(FALSE)) {
		pCmdUI->Enable(FALSE);
		return;
	}

	// ‹–‰Â
	pCmdUI->Enable(TRUE);
}

//---------------------------------------------------------------------------
//
//	CD-ROM‹­§ƒCƒWƒFƒNƒg
//
//---------------------------------------------------------------------------
void CFrmWnd::OnCDForce()
{
	ASSERT(this);
	ASSERT(m_pSCSI);

	// VM‚ğƒƒbƒN‚µ‚Äs‚¤
	::LockVM();
	m_pSCSI->Eject(TRUE, FALSE);
	::UnlockVM();
}

//---------------------------------------------------------------------------
//
//	CD-ROM‹­§ƒCƒWƒFƒNƒg UI
//
//---------------------------------------------------------------------------
void CFrmWnd::OnCDForceUI(CCmdUI *pCmdUI)
{
	ASSERT(this);
	ASSERT(m_pSCSI);

	// ƒŒƒfƒB‚Å‚È‚¯‚ê‚Î‹Ö~
	if (!m_pSCSI->IsReady(FALSE)) {
		pCmdUI->Enable(FALSE);
		return;
	}

	// ƒƒbƒN‚³‚ê‚Ä‚¢‚È‚¯‚ê‚Î‹Ö~
	if (!m_pSCSI->IsLocked(FALSE)) {
		pCmdUI->Enable(FALSE);
		return;
	}

	// ‹–‰Â
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

	// ƒCƒ“ƒfƒbƒNƒXì¬
	nMRU = uID - IDM_CD_MRU0;
	ASSERT((nMRU >= 0) && (nMRU <= 8));

	// MRUæ“¾
	GetConfig()->GetMRUFile(3, nMRU, szMRU);
	if (szMRU[0] == _T('\0')) {
		return;
	}
	path.SetPath(szMRU);

	// VMƒƒbƒN
	::LockVM();

	// ƒI[ƒvƒ“
	bResult = m_pSCSI->Open(path, FALSE);
	GetScheduler()->Reset();
	ResetCaption();
	::UnlockVM();

	// ¬Œ÷‚·‚ê‚ÎAƒfƒBƒŒƒNƒgƒŠXV‚ÆMRU’Ç‰Á
	if (bResult) {
		// ƒCƒjƒVƒƒƒ‹ƒfƒBƒŒƒNƒgƒŠXV
		Filepath::SetDefaultDir(szMRU);

		// MRU‚É’Ç‰Á
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

	// ƒhƒ‰ƒCƒu‚ª—LŒø‚Å‚È‚¯‚Î–³Œø
	if (!m_pSCSI->IsValid(FALSE)) {
		pCmdUI->Enable(FALSE);
		return;
	}

	// ƒƒbƒN‚³‚ê‚Ä‚¢‚ê‚Î‹Ö~
	if (m_pSCSI->IsLocked(FALSE)) {
		pCmdUI->Enable(FALSE);
		return;
	}

	// ‹–‰Â
	pCmdUI->Enable(TRUE);
}

//---------------------------------------------------------------------------
//
//	ƒTƒuƒEƒBƒ“ƒhƒE ƒRƒ}ƒ“ƒhEUIƒ}ƒNƒ
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
//	ƒƒO
//
//---------------------------------------------------------------------------
void CFrmWnd::OnLog()
{
	ON_SUB_WINDOW(MAKEID('L', 'O', 'G', 'L'), CLogWnd);
}

//---------------------------------------------------------------------------
//
//	ƒƒO UI
//
//---------------------------------------------------------------------------
void CFrmWnd::OnLogUI(CCmdUI *pCmdUI)
{
	ON_UPDATE_SUB_WINDOW(MAKEID('L', 'O', 'G', 'L'));
}

//---------------------------------------------------------------------------
//
//	ƒXƒPƒWƒ…[ƒ‰
//
//---------------------------------------------------------------------------
void CFrmWnd::OnScheduler()
{
	ON_SUB_WINDOW(MAKEID('S', 'C', 'H', 'E'), CSchedulerWnd);
}

//---------------------------------------------------------------------------
//
//	ƒXƒPƒWƒ…[ƒ‰ UI
//
//---------------------------------------------------------------------------
void CFrmWnd::OnSchedulerUI(CCmdUI *pCmdUI)
{
	ON_UPDATE_SUB_WINDOW(MAKEID('S', 'C', 'H', 'E'));
}

//---------------------------------------------------------------------------
//
//	ƒfƒoƒCƒX
//
//---------------------------------------------------------------------------
void CFrmWnd::OnDevice()
{
	ON_SUB_WINDOW(MAKEID('D', 'E', 'V', 'I'), CDeviceWnd);
}

//---------------------------------------------------------------------------
//
//	ƒfƒoƒCƒX UI
//
//---------------------------------------------------------------------------
void CFrmWnd::OnDeviceUI(CCmdUI *pCmdUI)
{
	ON_UPDATE_SUB_WINDOW(MAKEID('D', 'E', 'V', 'I'));
}

//---------------------------------------------------------------------------
//
//	CPUƒŒƒWƒXƒ^
//
//---------------------------------------------------------------------------
void CFrmWnd::OnCPUReg()
{
	ON_SUB_WINDOW(MAKEID('M', 'P', 'U', 'R'), CCPURegWnd);
}

//---------------------------------------------------------------------------
//
//	CPUƒŒƒWƒXƒ^ UI
//
//---------------------------------------------------------------------------
void CFrmWnd::OnCPURegUI(CCmdUI *pCmdUI)
{
	ON_UPDATE_SUB_WINDOW(MAKEID('M', 'P', 'U', 'R'));
}

//---------------------------------------------------------------------------
//
//	Š„‚è‚İ
//
//---------------------------------------------------------------------------
void CFrmWnd::OnInt()
{
	ON_SUB_WINDOW(MAKEID('I', 'N', 'T', ' '), CIntWnd);
}

//---------------------------------------------------------------------------
//
//	Š„‚è‚İ UI
//
//---------------------------------------------------------------------------
void CFrmWnd::OnIntUI(CCmdUI *pCmdUI)
{
	ON_UPDATE_SUB_WINDOW(MAKEID('I', 'N', 'T', ' '));
}

//---------------------------------------------------------------------------
//
//	‹tƒAƒZƒ“ƒuƒ‹
//
//---------------------------------------------------------------------------
void CFrmWnd::OnDisasm()
{
	CDisasmWnd *pWnd;
	int i;
	BOOL flag;

	// 8í—Ş‚·‚×‚Äƒ`ƒFƒbƒN‚·‚é
	flag = FALSE;
	for (i=0; i<8; i++) {
		pWnd = (CDisasmWnd*)GetView()->SearchSWnd(MAKEID('D', 'I', 'S', 'A' + i));
		if (pWnd) {
			pWnd->PostMessage(WM_CLOSE, 0, 0);
			flag = TRUE;
		}
	}

	// V‹Kì¬
	if (!flag) {
		pWnd = new CDisasmWnd(0);
		VERIFY(pWnd->Init(GetView()));
	}
}

//---------------------------------------------------------------------------
//
//	‹tƒAƒZƒ“ƒuƒ‹ UI
//
//---------------------------------------------------------------------------
void CFrmWnd::OnDisasmUI(CCmdUI *pCmdUI)
{
	CSubWnd *pSubWnd;
	int i;

	// 8í—Ş‚·‚×‚Äƒ`ƒFƒbƒN‚·‚é
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
//	ƒƒ‚ƒŠ
//
//---------------------------------------------------------------------------
void CFrmWnd::OnMemory()
{
	CMemoryWnd *pWnd;
	int i;
	BOOL flag;

	// 8í—Ş‚·‚×‚Äƒ`ƒFƒbƒN‚·‚é
	flag = FALSE;
	for (i=0; i<8; i++) {
		pWnd = (CMemoryWnd*)GetView()->SearchSWnd(MAKEID('M', 'E', 'M', 'A' + i));
		if (pWnd) {
			pWnd->PostMessage(WM_CLOSE, 0, 0);
			flag = TRUE;
		}
	}

	// V‹Kì¬
	if (!flag) {
		pWnd = new CMemoryWnd(0);
		VERIFY(pWnd->Init(GetView()));
	}
}

//---------------------------------------------------------------------------
//
//	ƒƒ‚ƒŠ UI
//
//---------------------------------------------------------------------------
void CFrmWnd::OnMemoryUI(CCmdUI *pCmdUI)
{
	CSubWnd *pSubWnd;
	int i;

	// 8í—Ş‚·‚×‚Äƒ`ƒFƒbƒN‚·‚é
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
//	ƒuƒŒ[ƒNƒ|ƒCƒ“ƒg
//
//---------------------------------------------------------------------------
void CFrmWnd::OnBreakP()
{
	ON_SUB_WINDOW(MAKEID('B', 'R', 'K', 'P'), CBreakPWnd);
}

//---------------------------------------------------------------------------
//
//	ƒuƒŒ[ƒNƒ|ƒCƒ“ƒg UI
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
//	ƒL[ƒ{[ƒh
//
//---------------------------------------------------------------------------
void CFrmWnd::OnKeyboard()
{
	ON_SUB_WINDOW(MAKEID('K', 'E', 'Y', 'B'), CKeyboardWnd);
}

//---------------------------------------------------------------------------
//
//	ƒL[ƒ{[ƒh UI
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
//	ƒeƒLƒXƒg‰æ–Ê
//
//---------------------------------------------------------------------------
void CFrmWnd::OnTVRAM()
{
	ON_SUB_WINDOW(MAKEID('T', 'V', 'R', 'M'), CTVRAMWnd);
}

//---------------------------------------------------------------------------
//
//	ƒeƒLƒXƒg‰æ–Ê UI
//
//---------------------------------------------------------------------------
void CFrmWnd::OnTVRAMUI(CCmdUI *pCmdUI)
{
	ON_UPDATE_SUB_WINDOW(MAKEID('T', 'V', 'R', 'M'));
}

//---------------------------------------------------------------------------
//
//	ƒOƒ‰ƒtƒBƒbƒN‰æ–Ê1024~1024
//
//---------------------------------------------------------------------------
void CFrmWnd::OnG1024()
{
	ON_SUB_WINDOW(MAKEID('G', '1', '0', '2'), CG1024Wnd);
}

//---------------------------------------------------------------------------
//
//	ƒOƒ‰ƒtƒBƒbƒN‰æ–Ê1024~1024 UI
//
//---------------------------------------------------------------------------
void CFrmWnd::OnG1024UI(CCmdUI *pCmdUI)
{
	ON_UPDATE_SUB_WINDOW(MAKEID('G', '1', '0', '2'));
}

//---------------------------------------------------------------------------
//
//	ƒOƒ‰ƒtƒBƒbƒN‰æ–Ê16F
//
//---------------------------------------------------------------------------
void CFrmWnd::OnG16(UINT uID)
{
	CG16Wnd *pWnd;
	int index;

	// ƒCƒ“ƒfƒbƒNƒXŒˆ’è
	index = (int)(uID - IDM_G16P0);
	ASSERT((index >= 0) || (index <= 3));

	// ‘¶İ‚·‚ê‚ÎÁ‹
	pWnd = (CG16Wnd*)GetView()->SearchSWnd(MAKEID('G', '1', '6', ('A' + index)));
	if (pWnd) {
		pWnd->PostMessage(WM_CLOSE, 0, 0);
		return;
	}

	// ƒTƒuƒEƒBƒ“ƒhƒEì¬
	pWnd = new CG16Wnd(index);
	VERIFY(pWnd->Init(GetView()));
}

//---------------------------------------------------------------------------
//
//	ƒOƒ‰ƒtƒBƒbƒN‰æ–Ê16F UI
//
//---------------------------------------------------------------------------
void CFrmWnd::OnG16UI(CCmdUI *pCmdUI)
{
	int index;
	CSubWnd *pSubWnd;

	// ƒCƒ“ƒfƒbƒNƒXŒˆ’è
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
//	ƒOƒ‰ƒtƒBƒbƒN‰æ–Ê256F
//
//---------------------------------------------------------------------------
void CFrmWnd::OnG256(UINT uID)
{
	CG256Wnd *pWnd;
	int index;

	// ƒCƒ“ƒfƒbƒNƒXŒˆ’è
	index = (int)(uID - IDM_G256P0);
	ASSERT((index == 0) || (index == 1));

	// ‘¶İ‚·‚ê‚ÎÁ‹
	pWnd = (CG256Wnd*)GetView()->SearchSWnd(MAKEID('G', '2', '5', ('A' + index)));
	if (pWnd) {
		pWnd->PostMessage(WM_CLOSE, 0, 0);
		return;
	}

	// ƒTƒuƒEƒBƒ“ƒhƒEì¬
	pWnd = new CG256Wnd(index);
	VERIFY(pWnd->Init(GetView()));
}

//---------------------------------------------------------------------------
//
//	ƒOƒ‰ƒtƒBƒbƒN‰æ–Ê256F UI
//
//---------------------------------------------------------------------------
void CFrmWnd::OnG256UI(CCmdUI *pCmdUI)
{
	int index;
	CSubWnd *pSubWnd;

	// ƒCƒ“ƒfƒbƒNƒXŒˆ’è
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
//	ƒOƒ‰ƒtƒBƒbƒN‰æ–Ê65536F
//
//---------------------------------------------------------------------------
void CFrmWnd::OnG64K()
{
	CG64KWnd *pWnd;

	// ‘¶İ‚·‚ê‚ÎÁ‹
	pWnd = (CG64KWnd*)GetView()->SearchSWnd(MAKEID('G', '6', '4', 'K'));
	if (pWnd) {
		pWnd->PostMessage(WM_CLOSE, 0, 0);
		return;
	}

	// ƒTƒuƒEƒBƒ“ƒhƒEì¬
	pWnd = new CG64KWnd;
	VERIFY(pWnd->Init(GetView()));
}

//---------------------------------------------------------------------------
//
//	ƒOƒ‰ƒtƒBƒbƒN‰æ–Ê65536F UI
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

	// ‘¶İ‚·‚ê‚ÎÁ‹
	pWnd = (CPCGWnd*)GetView()->SearchSWnd(MAKEID('P', 'C', 'G', ' '));
	if (pWnd) {
		pWnd->PostMessage(WM_CLOSE, 0, 0);
		return;
	}

	// ƒTƒuƒEƒBƒ“ƒhƒEì¬
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
//	BG‰æ–Ê
//
//---------------------------------------------------------------------------
void CFrmWnd::OnBG(UINT uID)
{
	CBGWnd *pWnd;
	int index;

	// ƒCƒ“ƒfƒbƒNƒXŒˆ’è
	index = (int)(uID - IDM_BG0);
	ASSERT((index == 0) || (index == 1));

	// ‘¶İ‚·‚ê‚ÎÁ‹
	pWnd = (CBGWnd*)GetView()->SearchSWnd(MAKEID('B', 'G', ('0' + index), ' '));
	if (pWnd) {
		pWnd->PostMessage(WM_CLOSE, 0, 0);
		return;
	}

	// ƒTƒuƒEƒBƒ“ƒhƒEì¬
	pWnd = new CBGWnd(index);
	VERIFY(pWnd->Init(GetView()));
}

//---------------------------------------------------------------------------
//
//	BG‰æ–Ê UI
//
//---------------------------------------------------------------------------
void CFrmWnd::OnBGUI(CCmdUI *pCmdUI)
{
	int index;
	CSubWnd *pSubWnd;

	// ƒCƒ“ƒfƒbƒNƒXŒˆ’è
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
//	ƒpƒŒƒbƒg
//
//---------------------------------------------------------------------------
void CFrmWnd::OnPalet()
{
	CPaletteWnd *pWnd;

	// ‘¶İ‚·‚ê‚ÎÁ‹
	pWnd = (CPaletteWnd*)GetView()->SearchSWnd(MAKEID('P', 'A', 'L', ' '));
	if (pWnd) {
		pWnd->PostMessage(WM_CLOSE, 0, 0);
		return;
	}

	// ƒTƒuƒEƒBƒ“ƒhƒEì¬
	pWnd = new CPaletteWnd(FALSE);
	VERIFY(pWnd->Init(GetView()));
}

//---------------------------------------------------------------------------
//
//	ƒpƒŒƒbƒg UI
//
//---------------------------------------------------------------------------
void CFrmWnd::OnPaletUI(CCmdUI *pCmdUI)
{
	ON_UPDATE_SUB_WINDOW(MAKEID('P', 'A', 'L', ' '));
}

//---------------------------------------------------------------------------
//
//	ƒeƒLƒXƒgƒoƒbƒtƒ@
//
//---------------------------------------------------------------------------
void CFrmWnd::OnTextBuf()
{
	CRendBufWnd *pWnd;

	// ‘¶İ‚·‚ê‚ÎÁ‹
	pWnd = (CRendBufWnd*)GetView()->SearchSWnd(MAKEID('T', 'E', 'X', 'B'));
	if (pWnd) {
		pWnd->PostMessage(WM_CLOSE, 0, 0);
		return;
	}

	// ƒTƒuƒEƒBƒ“ƒhƒEì¬
	pWnd = new CRendBufWnd(0);
	VERIFY(pWnd->Init(GetView()));
}

//---------------------------------------------------------------------------
//
//	ƒeƒLƒXƒgƒoƒbƒtƒ@ UI
//
//---------------------------------------------------------------------------
void CFrmWnd::OnTextBufUI(CCmdUI *pCmdUI)
{
	ON_UPDATE_SUB_WINDOW(MAKEID('T', 'E', 'X', 'B'));
}

//---------------------------------------------------------------------------
//
//	ƒOƒ‰ƒtƒBƒbƒNƒoƒbƒtƒ@
//
//---------------------------------------------------------------------------
void CFrmWnd::OnGrpBuf(UINT uID)
{
	CRendBufWnd *pWnd;
	int index;

	// ƒCƒ“ƒfƒbƒNƒXŒˆ’è
	index = (int)(uID - IDM_REND_GP0);
	ASSERT((index >= 0) || (index <= 4));

	// ‘¶İ‚·‚ê‚ÎÁ‹
	pWnd = (CRendBufWnd*)GetView()->SearchSWnd(MAKEID('G', 'P', ('0' + index), 'B'));
	if (pWnd) {
		pWnd->PostMessage(WM_CLOSE, 0, 0);
		return;
	}

	// ƒTƒuƒEƒBƒ“ƒhƒEì¬
	pWnd = new CRendBufWnd(index + 1);
	VERIFY(pWnd->Init(GetView()));
}

//---------------------------------------------------------------------------
//
//	ƒOƒ‰ƒtƒBƒbƒNƒoƒbƒtƒ@ UI
//
//---------------------------------------------------------------------------
void CFrmWnd::OnGrpBufUI(CCmdUI *pCmdUI)
{
	int index;
	CSubWnd *pSubWnd;

	// ƒCƒ“ƒfƒbƒNƒXŒˆ’è
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
//	PCGƒoƒbƒtƒ@
//
//---------------------------------------------------------------------------
void CFrmWnd::OnPCGBuf()
{
	ON_SUB_WINDOW(MAKEID('P', 'C', 'G', 'B'), CPCGBufWnd);
}

//---------------------------------------------------------------------------
//
//	PCGƒoƒbƒtƒ@ UI
//
//---------------------------------------------------------------------------
void CFrmWnd::OnPCGBufUI(CCmdUI *pCmdUI)
{
	ON_UPDATE_SUB_WINDOW(MAKEID('P', 'C', 'G', 'B'));
}

//---------------------------------------------------------------------------
//
//	BG/ƒXƒvƒ‰ƒCƒgƒoƒbƒtƒ@
//
//---------------------------------------------------------------------------
void CFrmWnd::OnBGSpBuf()
{
	CRendBufWnd *pWnd;

	// ‘¶İ‚·‚ê‚ÎÁ‹
	pWnd = (CRendBufWnd*)GetView()->SearchSWnd(MAKEID('B', 'G', 'S', 'P'));
	if (pWnd) {
		pWnd->PostMessage(WM_CLOSE, 0, 0);
		return;
	}

	// ƒTƒuƒEƒBƒ“ƒhƒEì¬
	pWnd = new CRendBufWnd(5);
	VERIFY(pWnd->Init(GetView()));
}

//---------------------------------------------------------------------------
//
//	BG/ƒXƒvƒ‰ƒCƒgƒoƒbƒtƒ@ UI
//
//---------------------------------------------------------------------------
void CFrmWnd::OnBGSpBufUI(CCmdUI *pCmdUI)
{
	ON_UPDATE_SUB_WINDOW(MAKEID('B', 'G', 'S', 'P'));
}

//---------------------------------------------------------------------------
//
//	ƒpƒŒƒbƒgƒoƒbƒtƒ@
//
//---------------------------------------------------------------------------
void CFrmWnd::OnPaletBuf()
{
	CPaletteWnd *pWnd;

	// ‘¶İ‚·‚ê‚ÎÁ‹
	pWnd = (CPaletteWnd*)GetView()->SearchSWnd(MAKEID('P', 'A', 'L', 'B'));
	if (pWnd) {
		pWnd->PostMessage(WM_CLOSE, 0, 0);
		return;
	}

	// ƒTƒuƒEƒBƒ“ƒhƒEì¬
	pWnd = new CPaletteWnd(TRUE);
	VERIFY(pWnd->Init(GetView()));
}

//---------------------------------------------------------------------------
//
//	ƒpƒŒƒbƒgƒoƒbƒtƒ@ UI
//
//---------------------------------------------------------------------------
void CFrmWnd::OnPaletBufUI(CCmdUI *pCmdUI)
{
	ON_UPDATE_SUB_WINDOW(MAKEID('P', 'A', 'L', 'B'));
}

//---------------------------------------------------------------------------
//
//	‡¬ƒoƒbƒtƒ@
//
//---------------------------------------------------------------------------
void CFrmWnd::OnMixBuf()
{
	ON_SUB_WINDOW(MAKEID('M', 'I', 'X', 'B'), CMixBufWnd);
}

//---------------------------------------------------------------------------
//
//	‡¬ƒoƒbƒtƒ@ UI
//
//---------------------------------------------------------------------------
void CFrmWnd::OnMixBufUI(CCmdUI *pCmdUI)
{
	ON_UPDATE_SUB_WINDOW(MAKEID('M', 'I', 'X', 'B'));
}

//---------------------------------------------------------------------------
//
//	ƒRƒ“ƒ|[ƒlƒ“ƒg
//
//---------------------------------------------------------------------------
void CFrmWnd::OnComponent()
{
	ON_SUB_WINDOW(MAKEID('C', 'O', 'M', 'P'), CComponentWnd);
}

//---------------------------------------------------------------------------
//
//	ƒRƒ“ƒ|[ƒlƒ“ƒg UI
//
//---------------------------------------------------------------------------
void CFrmWnd::OnComponentUI(CCmdUI *pCmdUI)
{
	ON_UPDATE_SUB_WINDOW(MAKEID('C', 'O', 'M', 'P'));
}

//---------------------------------------------------------------------------
//
//	OSî•ñ
//
//---------------------------------------------------------------------------
void CFrmWnd::OnOSInfo()
{
	ON_SUB_WINDOW(MAKEID('O', 'S', 'I', 'N'), COSInfoWnd);
}

//---------------------------------------------------------------------------
//
//	OSî•ñ UI
//
//---------------------------------------------------------------------------
void CFrmWnd::OnOSInfoUI(CCmdUI *pCmdUI)
{
	ON_UPDATE_SUB_WINDOW(MAKEID('O', 'S', 'I', 'N'));
}

//---------------------------------------------------------------------------
//
//	ƒTƒEƒ“ƒh
//
//---------------------------------------------------------------------------
void CFrmWnd::OnSound()
{
	ON_SUB_WINDOW(MAKEID('S', 'N', 'D', ' '), CSoundWnd);
}

//---------------------------------------------------------------------------
//
//	ƒTƒEƒ“ƒh UI
//
//---------------------------------------------------------------------------
void CFrmWnd::OnSoundUI(CCmdUI *pCmdUI)
{
	ON_UPDATE_SUB_WINDOW(MAKEID('S', 'N', 'D', ' '));
}

//---------------------------------------------------------------------------
//
//	ƒCƒ“ƒvƒbƒg
//
//---------------------------------------------------------------------------
void CFrmWnd::OnInput()
{
	ON_SUB_WINDOW(MAKEID('I', 'N', 'P', ' '), CInputWnd);
}

//---------------------------------------------------------------------------
//
//	ƒCƒ“ƒvƒbƒg UI
//
//---------------------------------------------------------------------------
void CFrmWnd::OnInputUI(CCmdUI *pCmdUI)
{
	ON_UPDATE_SUB_WINDOW(MAKEID('I', 'N', 'P', ' '));
}

//---------------------------------------------------------------------------
//
//	ƒ|[ƒg
//
//---------------------------------------------------------------------------
void CFrmWnd::OnPort()
{
	ON_SUB_WINDOW(MAKEID('P', 'O', 'R', 'T'), CPortWnd);
}

//---------------------------------------------------------------------------
//
//	ƒ|[ƒg UI
//
//---------------------------------------------------------------------------
void CFrmWnd::OnPortUI(CCmdUI *pCmdUI)
{
	ON_UPDATE_SUB_WINDOW(MAKEID('P', 'O', 'R', 'T'));
}

//---------------------------------------------------------------------------
//
//	ƒrƒbƒgƒ}ƒbƒv
//
//---------------------------------------------------------------------------
void CFrmWnd::OnBitmap()
{
	ON_SUB_WINDOW(MAKEID('B', 'M', 'A', 'P'), CBitmapWnd);
}

//---------------------------------------------------------------------------
//
//	ƒrƒbƒgƒ}ƒbƒv UI
//
//---------------------------------------------------------------------------
void CFrmWnd::OnBitmapUI(CCmdUI *pCmdUI)
{
	ON_UPDATE_SUB_WINDOW(MAKEID('B', 'M', 'A', 'P'));
}

//---------------------------------------------------------------------------
//
//	MIDIƒhƒ‰ƒCƒo
//
//---------------------------------------------------------------------------
void CFrmWnd::OnMIDIDrv()
{
	ON_SUB_WINDOW(MAKEID('M', 'D', 'R', 'V'), CMIDIDrvWnd);
}

//---------------------------------------------------------------------------
//
//	MIDIƒhƒ‰ƒCƒo UI
//
//---------------------------------------------------------------------------
void CFrmWnd::OnMIDIDrvUI(CCmdUI *pCmdUI)
{
	ON_UPDATE_SUB_WINDOW(MAKEID('M', 'D', 'R', 'V'));
}

//---------------------------------------------------------------------------
//
//	ƒLƒƒƒvƒVƒ‡ƒ“
//
//---------------------------------------------------------------------------
void CFrmWnd::OnCaption()
{
	// ƒtƒ‰ƒO‚ğ”½“]
	m_bCaption = !m_bCaption;

	// ˆ—‚ÍShowCaption‚É”C‚¹‚é
	ShowCaption();
}

//---------------------------------------------------------------------------
//
//	ƒLƒƒƒvƒVƒ‡ƒ“ UI
//
//---------------------------------------------------------------------------
void CFrmWnd::OnCaptionUI(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(m_bCaption);
}

//---------------------------------------------------------------------------
//
//	ƒƒjƒ…[ƒo[
//
//---------------------------------------------------------------------------
void CFrmWnd::OnMenu()
{
	// ƒtƒ‰ƒO‚ğ”½“]
	m_bMenuBar = !m_bMenuBar;

	// ˆ—‚ÍShowMenu‚É”C‚¹‚é
	ShowMenu();
}

//---------------------------------------------------------------------------
//
//	ƒƒjƒ…[ƒo[ UI
//
//---------------------------------------------------------------------------
void CFrmWnd::OnMenuUI(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(m_bMenuBar);
}

//---------------------------------------------------------------------------
//
//	ƒXƒe[ƒ^ƒXƒo[
//
//---------------------------------------------------------------------------
void CFrmWnd::OnStatus()
{
	// ƒtƒ‰ƒO‚ğ”½“]
	m_bStatusBar = !m_bStatusBar;

	// ˆ—‚ÍShowStatus‚É”C‚¹‚é
	ShowStatus();
}

//---------------------------------------------------------------------------
//
//	ƒXƒe[ƒ^ƒXƒo[ UI
//
//---------------------------------------------------------------------------
void CFrmWnd::OnStatusUI(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(m_bStatusBar);
}

//---------------------------------------------------------------------------
//
//	ƒŠƒtƒŒƒbƒVƒ…
//
//---------------------------------------------------------------------------
void CFrmWnd::OnRefresh()
{
	// ƒƒbƒN
	::LockVM();

	// Ä•`‰æ
	GetView()->Refresh();

	// ƒAƒ“ƒƒbƒN
	::UnlockVM();
}

//---------------------------------------------------------------------------
//
//	Šg‘å
//
//---------------------------------------------------------------------------
void CFrmWnd::OnStretch()
{
	BOOL bFlag;

	// VM‚ğƒƒbƒN
	::LockVM();

	// ”½“]
	bFlag = GetView()->IsStretch();
	GetView()->Stretch(!bFlag);

	// ƒRƒ“ƒtƒBƒO‚ğ•Ï‚¦‚é
	GetConfig()->SetStretch(!bFlag);

	// VMƒAƒ“ƒƒbƒN
	::UnlockVM();
}

//---------------------------------------------------------------------------
//
//	Šg‘å UI
//
//---------------------------------------------------------------------------
void CFrmWnd::OnStretchUI(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(GetView()->IsStretch());
}

//---------------------------------------------------------------------------
//
//	ƒtƒ‹ƒXƒNƒŠ[ƒ“
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

	// •ÏX‚ğ¸”s‚É‰Šú‰»
	bChanged = FALSE;

	// ƒ}ƒEƒXOFFAƒXƒPƒWƒ…[ƒ‰’â~AƒTƒEƒ“ƒhOFF
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
		// ’Êíƒ‚[ƒh‚Ö

		// ƒEƒBƒ“ƒhƒE—v‘f
		ModifyStyle(0, WS_CAPTION | WS_SYSMENU | WS_BORDER, 0);
		ModifyStyleEx(0, WS_EX_WINDOWEDGE, 0);
		GetView()->ModifyStyleEx(0, WS_EX_CLIENTEDGE, 0);

		// ‰æ–Êƒ‚[ƒhØ‚è‘Ö‚¦(1)
		m_DevMode.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT | DM_DISPLAYFREQUENCY;
		if (::ChangeDisplaySettings(&m_DevMode, 0) == DISP_CHANGE_SUCCESSFUL) {
			bChanged = TRUE;
		}

		// ¸”s‚µ‚½‚çAFREQw’è‚ğŠO‚µ‚ÄAÄ“x‚İ‚é
		if (!bChanged) {
			// ‰æ–Êƒ‚[ƒhØ‚è‘Ö‚¦(2)
			m_DevMode.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT;
			if (::ChangeDisplaySettings(&m_DevMode, 0) == DISP_CHANGE_SUCCESSFUL) {
				bChanged = TRUE;
			}
		}

		// ¬Œ÷‚µ‚½ê‡
		if (bChanged) {
			// Å‘å‰»ó‘Ô‚Å‘S‰æ–Ê‚É‚µ‚½‚Ì‚Å‚ ‚ê‚ÎAŒ³‚É–ß‚µ‚Ä‚â‚é
			if (IsZoomed()) {
				ShowWindow(SW_RESTORE);
			}

			// ƒLƒƒƒvƒVƒ‡ƒ“Aƒƒjƒ…[AƒXƒe[ƒ^ƒXƒo[
			m_bFullScreen = FALSE;
			ShowCaption();
			ShowMenu();
			ShowStatus();

			// ÄŠJˆ—
			HideTaskBar(FALSE, TRUE);

			InitPos(FALSE);
			RecalcLayout();
			GetScheduler()->Enable(bEnable);
			GetSound()->Enable(bSound);
			GetInput()->SetMouseMode(bMouse);
			ResetCaption();
			ResetStatus();

			// ©“®ƒ}ƒEƒXƒ‚[ƒh
			if (m_bAutoMouse && bMouse) {
				OnMouseMode();
			}
			return;
		}

		// ’Êíƒ‚[ƒh‚É‚µ‚½‚©‚Á‚½‚ªA‚Å‚«‚È‚©‚Á‚½
		ModifyStyle(WS_CAPTION | WS_SYSMENU | WS_BORDER, 0, 0);
		ModifyStyleEx(WS_EX_WINDOWEDGE, 0, 0);
		GetView()->ModifyStyleEx(WS_EX_CLIENTEDGE, 0, 0);

		// ÄŠJˆ—
		GetScheduler()->Enable(bEnable);
		GetSound()->Enable(bSound);
		GetInput()->SetMouseMode(bMouse);
		ResetCaption();
		ResetStatus();
		return;
	}

	// ƒtƒ‹ƒXƒNƒŠ[ƒ“‚ÖˆÚs‚·‚éê‡AŒ»İ‚ÌƒEƒBƒ“ƒhƒEˆÊ’u‚ğ•Û‘¶
	GetWindowRect(&rectWnd);
	m_nWndLeft = rectWnd.left;
	m_nWndTop = rectWnd.top;

	


	// ƒEƒBƒ“ƒhƒE—v‘f
	ModifyStyle(WS_CAPTION | WS_SYSMENU | WS_BORDER, 0, 0);
	ModifyStyleEx(WS_EX_WINDOWEDGE, 0, 0);
	GetView()->ModifyStyleEx(WS_EX_CLIENTEDGE, 0, 0);

	// Œ»İ‚Ì‰æ–Êƒ‚[ƒh‚ğæ“¾
	hDC = ::GetDC(NULL);
	ASSERT(hDC);
	memset(&m_DevMode, 0, sizeof(m_DevMode));
	m_DevMode.dmSize = sizeof(DEVMODE);
	m_DevMode.dmPelsWidth = ::GetDeviceCaps(hDC, HORZRES);
	m_DevMode.dmPelsHeight = ::GetDeviceCaps(hDC, VERTRES);
	m_DevMode.dmBitsPerPel = ::GetDeviceCaps(hDC, BITSPIXEL);
	m_DevMode.dmDisplayFrequency = ::GetDeviceCaps(hDC, VREFRESH);
	::ReleaseDC(NULL, hDC);

	// ‰æ–Êƒ‚[ƒhØ‚è‘Ö‚¦
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
		// ƒLƒƒƒvƒVƒ‡ƒ“Aƒƒjƒ…[AƒXƒe[ƒ^ƒXƒo[
		m_bFullScreen = TRUE;
		ShowCaption();
		ShowMenu();


		/* No mostrar barra de estado en fullScreen */
		//ShowStatus();

		// ÄŠJˆ—
		HideTaskBar(TRUE, TRUE);

		InitPos(FALSE);
		RecalcStatusView();
		GetScheduler()->Enable(bEnable);
		GetSound()->Enable(bSound);
		GetInput()->SetMouseMode(bMouse);
		ResetCaption();
		ResetStatus();

		// ©“®ƒ}ƒEƒXƒ‚[ƒh
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

	// ƒtƒ‹ƒXƒNƒŠ[ƒ“‚É‚µ‚æ‚¤‚Æ‚µ‚½‚ªA‚Å‚«‚È‚©‚Á‚½
	ModifyStyle(0, WS_CAPTION | WS_SYSMENU | WS_BORDER, 0);
	ModifyStyleEx(0, WS_EX_WINDOWEDGE, 0);
	
	GetView()->ModifyStyleEx(0, WS_EX_CLIENTEDGE, 0);
	//GetView()->SetupBitmap();
	// ÄŠJˆ—
	GetScheduler()->Enable(bEnable);
	GetSound()->Enable(bSound);
	GetInput()->SetMouseMode(bMouse);
	ResetCaption();
	ResetStatus();	


}


//---------------------------------------------------------------------------
//
//	ƒtƒ‹ƒXƒNƒŠ[ƒ“ UI
//
//---------------------------------------------------------------------------
void CFrmWnd::OnFullScreenUI(CCmdUI *pCmdUI)
{
	pCmdUI->SetCheck(m_bFullScreen);
}

//---------------------------------------------------------------------------
//
//	Às
//
//---------------------------------------------------------------------------
void CFrmWnd::OnExec()
{
	// ƒƒbƒN
	::LockVM();

	// ƒXƒPƒWƒ…[ƒ‰‚ğ—LŒø‰»
	GetScheduler()->Reset();
	GetScheduler()->Enable(TRUE);
	ResetCaption();

	// ƒAƒ“ƒƒbƒN
	::UnlockVM();
}

//---------------------------------------------------------------------------
//
//	Às UI
//
//---------------------------------------------------------------------------
void CFrmWnd::OnExecUI(CCmdUI *pCmdUI)
{
	// ƒXƒPƒWƒ…[ƒ‰‚ª’â~’†‚È‚ç—LŒø
	if ((!GetScheduler()->IsEnable()) && ::GetVM()->IsPower()) {
		pCmdUI->Enable(TRUE);
	}
	else {
		pCmdUI->Enable(FALSE);
	}
}

//---------------------------------------------------------------------------
//
//	’â~
//
//---------------------------------------------------------------------------
void CFrmWnd::OnBreak()
{
	// ƒƒbƒN
	::LockVM();

	// ƒXƒPƒWƒ…[ƒ‰‚ğ–³Œø‰»
	GetScheduler()->Enable(FALSE);

	// ƒAƒ“ƒƒbƒN
	::UnlockVM();
}

//---------------------------------------------------------------------------
//
//	’â~ UI
//
//---------------------------------------------------------------------------
void CFrmWnd::OnBreakUI(CCmdUI *pCmdUI)
{
	// ƒXƒPƒWƒ…[ƒ‰‚ª“®ì’†‚È‚ç—LŒø
	if (GetScheduler()->IsEnable()) {
		pCmdUI->Enable(TRUE);
	}
	else {
		pCmdUI->Enable(FALSE);
	}
}

//---------------------------------------------------------------------------
//
//	ƒgƒŒ[ƒX
//
//---------------------------------------------------------------------------
void CFrmWnd::OnTrace()
{
	// ƒƒbƒN
	::LockVM();

	// ƒgƒŒ[ƒX
	::GetVM()->Trace();
	GetScheduler()->SyncDisasm();

	// ƒAƒ“ƒƒbƒN
	::UnlockVM();
}

//---------------------------------------------------------------------------
//
//	ƒgƒŒ[ƒX UI
//
//---------------------------------------------------------------------------
void CFrmWnd::OnTraceUI(CCmdUI *pCmdUI)
{
	// ƒXƒPƒWƒ…[ƒ‰‚ª’â~’†‚È‚ç—LŒø
	if ((!GetScheduler()->IsEnable()) && ::GetVM()->IsPower()) {
		pCmdUI->Enable(TRUE);
	}
	else {
		pCmdUI->Enable(FALSE);
	}
}

//---------------------------------------------------------------------------
//
//	ƒ}ƒEƒXƒ‚[ƒh
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

	// Œ»İ‚Ìƒ‚[ƒh‚ğŒ©‚é
	b = GetInput()->GetMouseMode();

	// Å¬‰»A‚¨‚æ‚ÑƒXƒPƒWƒ…[ƒ‰OFF‚ÍOFF“®ì‚Ì‚İ
	if (!b) {
		// Å¬‰»‚È‚çON‚É‚³‚¹‚È‚¢
		if (IsIconic()) {
			return;
		}
		// ƒXƒPƒWƒ…[ƒ‰OFF‚È‚çON‚É‚³‚¹‚È‚¢
		if (!GetScheduler()->IsEnable()) {
			return;
		}
		// ”ñƒAƒNƒeƒBƒu‚È‚çON‚É‚³‚¹‚È‚¢
		if (!GetInput()->IsActive()) {
			return;
		}
	}

	if (b) {
		// ƒ}ƒEƒXƒ‚[ƒhOFF‚Ö
		GetInput()->SetMouseMode(FALSE);

		// ƒNƒŠƒbƒv‰ğœ
		ClipCursor(NULL);

		// ƒ}ƒEƒXƒJ[ƒ\ƒ‹‚ğƒXƒNƒŠ[ƒ“’†‰›‚Ö
		cx = ::GetSystemMetrics(SM_CXSCREEN);
		cy = ::GetSystemMetrics(SM_CYSCREEN);
		SetCursorPos(cx >> 1, cy >> 1);

		// ƒ}ƒEƒXƒJ[ƒ\ƒ‹ON
		cnt = ::ShowCursor(TRUE);
		while (cnt < 0) {
			cnt = ::ShowCursor(TRUE);
		}

		// ƒƒbƒZ[ƒW
		::GetMsg(IDS_MOUSE_WIN, string);
	}
	else {
		// ƒ}ƒEƒXƒJ[ƒ\ƒ‹‚ğƒNƒŠƒbƒvBViewƒEƒBƒ“ƒhƒE‚Ì+16‚Ì‚İ
		GetView()->GetWindowRect(&rect);
		rect.right = rect.left + 16;
		rect.bottom = rect.top + 16;
		ClipCursor(&rect);

		// ƒ}ƒEƒXƒJ[ƒ\ƒ‹OFF
		cnt = ::ShowCursor(FALSE);
		while (cnt >= 0) {
			cnt = ::ShowCursor(FALSE);
		}

		// ƒ}ƒEƒXƒ‚[ƒhON‚Ö
		GetInput()->SetMouseMode(TRUE);

		// ƒƒbƒZ[ƒW
		::GetMsg(IDS_MOUSE_X68K, string);
	}

	// ƒƒbƒZ[ƒWƒZƒbƒg
	SetInfo(string);
}

//---------------------------------------------------------------------------
//
//	ƒ\ƒtƒgƒEƒFƒAƒL[ƒ{[ƒh
//
//---------------------------------------------------------------------------
void CFrmWnd::OnSoftKey()
{
	ON_SUB_WINDOW(MAKEID('S', 'K', 'E', 'Y'), CSoftKeyWnd);
}

//---------------------------------------------------------------------------
//
//	ƒ\ƒtƒgƒEƒFƒAƒL[ƒ{[ƒh UI
//
//---------------------------------------------------------------------------
void CFrmWnd::OnSoftKeyUI(CCmdUI *pCmdUI)
{
	ON_UPDATE_SUB_WINDOW(MAKEID('S', 'K', 'E', 'Y'));

	// ƒL[ƒ{[ƒhƒfƒoƒCƒX‚ª‘¶İ‚µ‚È‚¯‚ê‚ÎA‰½‚à‚µ‚È‚¢
	if (!m_pKeyboard) {
		return;
	}

	// ƒTƒuƒEƒBƒ“ƒhƒE‘¶İ‚¹‚¸
	if (GetView()->SearchSWnd(MAKEID('S', 'K', 'E', 'Y')) == NULL) {
		// ƒL[ƒ{[ƒh‚ªÚ‘±‚³‚ê‚Ä‚¢‚È‚¯‚ê‚Î
		if (!m_pKeyboard->IsConnect()) {
			// ‹Ö~
			pCmdUI->Enable(FALSE);
			return;
		}
	}

	// —LŒø
	pCmdUI->Enable(TRUE);
}

//---------------------------------------------------------------------------
//
//	‡‚í‚¹
//
//---------------------------------------------------------------------------
void CFrmWnd::OnTimeAdj()
{
	RTC *pRTC;

	// pRTC->Adjust()‚Å‡‚í‚¹‚é
	::LockVM();
	pRTC = (RTC*)::GetVM()->SearchDevice(MAKEID('R', 'T', 'C', ' '));
	ASSERT(pRTC);
	pRTC->Adjust(FALSE);
	::UnlockVM();
}

//---------------------------------------------------------------------------
//
//	WAVƒLƒƒƒvƒ`ƒƒ
//
//---------------------------------------------------------------------------
void CFrmWnd::OnSaveWav()
{
	TCHAR szPath[_MAX_PATH];
	CString strMsg;
	BOOL bResult;

	// WAVƒZ[ƒu’†‚È‚ç~‚ß‚é
	if (GetSound()->IsSaveWav()) {
		GetSound()->EndSaveWav();
		::GetMsg(IDS_WAVSTOP, strMsg);
		SetInfo(strMsg);
		return;
	}

	// ƒtƒ@ƒCƒ‹‘I‘ğ
	szPath[0] = _T('\0');
	if (!FileSaveDlg(this, szPath, _T("wav"), IDS_WAVOPEN)) {
		ResetCaption();
		return;
	}

	// WAVƒZ[ƒu‚Éƒgƒ‰ƒC
	::LockVM();
	bResult = GetSound()->StartSaveWav(szPath);
	::UnlockVM();

	// Œ‹‰Ê•]‰¿
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
//	WAVƒLƒƒƒvƒ`ƒƒ UI
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

	// ƒfƒoƒCƒXæ“¾
	pMFP = (MFP*)::GetVM()->SearchDevice(MAKEID('M', 'F', 'P', ' '));
	ASSERT(pMFP);
	pMemory = (Memory*)::GetVM()->SearchDevice(MAKEID('M', 'E', 'M', ' '));
	ASSERT(pMemory);

	// ƒƒbƒN
	::LockVM();

	// MFP‚æ‚èAƒxƒNƒ^‚ğæ“¾
	dwVector = (pMFP->GetVR() & 0xf0) + 5;
	dwVector <<= 2;

	// Œ»İ‚ÌTimer-CƒxƒNƒ^ƒAƒhƒŒƒX‚ğ“¾‚é
	dwAddr = pMemory->ReadOnly(dwVector);
	dwAddr <<= 8;
	dwAddr |= pMemory->ReadOnly(dwVector + 1);
	dwAddr <<= 8;
	dwAddr |= pMemory->ReadOnly(dwVector + 2);
	dwAddr <<= 8;
	dwAddr |= pMemory->ReadOnly(dwVector + 3);

	// –¢‰Šú‰»(24bitˆÈã)‚È‚ç–³Œø
	if (dwAddr > 0xffffff) {
		::UnlockVM();
		return;
	}

	// Šù‚É0x6800‚È‚ç–³Œø
	if (dwAddr == 0x6800) {
		::UnlockVM();
		return;
	}

	// ƒR[ƒh‘I‘ğ
	dlg.m_dwCode = pMemory->ReadOnly(0x6809);
	if (pMemory->ReadOnly(0x6808) == 0xff) {
		// –¢‰Šú‰»‚Æ‚İ‚È‚µA0‚©‚çƒXƒ^[ƒg
		dlg.m_dwCode = 1;
	}
	::UnlockVM();
	if (dlg.DoModal() != IDOK) {
		return;
	}
	dwCode = dlg.m_dwCode;

	// ƒR[ƒh‘‚«‚İ
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

	// Timer-CƒxƒNƒ^•ÏX
	pMemory->WriteWord(dwVector, 0x0000);
	pMemory->WriteWord(dwVector + 2, 0x6800);

	// ƒAƒ“ƒƒbƒN
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

	// ƒfƒoƒCƒXæ“¾
	pMFP = (MFP*)::GetVM()->SearchDevice(MAKEID('M', 'F', 'P', ' '));
	ASSERT(pMFP);
	pMemory = (Memory*)::GetVM()->SearchDevice(MAKEID('M', 'E', 'M', ' '));
	ASSERT(pMemory);

	// MFP‚æ‚èAƒxƒNƒ^‚ğæ“¾
	dwVector = (pMFP->GetVR() & 0xf0) + 5;
	dwVector <<= 2;

	// Œ»İ‚ÌTimer-CƒxƒNƒ^ƒAƒhƒŒƒX‚ğ“¾‚é
	dwAddr = pMemory->ReadOnly(dwVector);
	dwAddr <<= 8;
	dwAddr |= pMemory->ReadOnly(dwVector + 1);
	dwAddr <<= 8;
	dwAddr |= pMemory->ReadOnly(dwVector + 2);
	dwAddr <<= 8;
	dwAddr |= pMemory->ReadOnly(dwVector + 3);

	// –¢‰Šú‰»(24bitˆÈã)‚È‚ç‹Ö~
	if (dwAddr > 0xffffff) {
		pCmdUI->Enable(FALSE);
		return;
	}

	// Šù‚É0x6800‚È‚ç‹Ö~
	if (dwAddr == 0x6800) {
		pCmdUI->Enable(FALSE);
		return;
	}

	// trap #0ƒxƒNƒ^‚ğæ“¾
	dwAddr = pMemory->ReadOnly(0x0080);
	dwAddr <<= 8;
	dwAddr |= pMemory->ReadOnly(0x0081);
	dwAddr <<= 8;
	dwAddr |= pMemory->ReadOnly(0x0082);
	dwAddr <<= 8;
	dwAddr |= pMemory->ReadOnly(0x0083);

	// –¢‰Šú‰»(24bitˆÈã)‚È‚ç‹Ö~
	if (dwAddr > 0xffffff) {
		pCmdUI->Enable(FALSE);
		return;
	}

	// ‹–‰Â
	pCmdUI->Enable(TRUE);
}

//---------------------------------------------------------------------------
//
//	V‚µ‚¢ƒtƒƒbƒs[ƒfƒBƒXƒN
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

	// ƒ_ƒCƒAƒƒOÀs
	if (dlg.DoModal() != IDOK) {
		return;
	}

	// ƒpƒXì¬
	path.SetPath(dlg.m_szFileName);

	// ƒIƒvƒVƒ‡ƒ“\‘¢‘Ìì¬
	opt.phyfmt = dlg.m_dwPhysical;
	opt.logfmt = dlg.m_bLogical;
	ASSERT(_tcslen(dlg.m_szDiskName) < 60);
	_tcscpy(opt.name, dlg.m_szDiskName);

	// ƒ^ƒCƒv‚É‰‚¶‚ÄƒCƒ[ƒW‚ğì‚é
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

	// ƒtƒH[ƒ}ƒbƒg(•¨—E˜_—EƒZ[ƒu‚ğ‚·‚×‚ÄŠÜ‚Ş)
	// ƒtƒH[ƒ}ƒbƒg’†‚ÍƒXƒPƒWƒ…[ƒ‰‚ğ~‚ß‚é
	AfxGetApp()->BeginWaitCursor();
	bRun = GetScheduler()->IsEnable();
	GetScheduler()->Enable(FALSE);
	::LockVM();
	::UnlockVM();
	if (!pDisk->Create(path, &opt)) {
		AfxGetApp()->EndWaitCursor();
		// ˆê“xíœ
		delete pDisk;

		// ƒƒbƒZ[ƒWƒ{ƒbƒNƒX
		::GetMsg(IDS_CREATEERR, strMsg);
		MessageBox(strMsg, NULL, MB_ICONSTOP | MB_OK);
		GetScheduler()->Reset();
		GetScheduler()->Enable(bRun);
		ResetCaption();
		return;
	}
	AfxGetApp()->EndWaitCursor();

	// ˆê“xíœ
	delete pDisk;

	// ƒI[ƒgƒ}ƒEƒ“ƒg(ƒIƒvƒVƒ‡ƒiƒ‹)
	if (dlg.m_nDrive >= 0) {
		InitCmdSub(dlg.m_nDrive, dlg.m_szFileName);
	}

	// ƒXƒe[ƒ^ƒXƒƒbƒZ[ƒWAÄŠJ
	::GetMsg(IDS_NEWFD, strMsg);
	SetInfo(strMsg);
	GetScheduler()->Reset();
	GetScheduler()->Enable(bRun);
	ResetCaption();
}

//---------------------------------------------------------------------------
//
//	V‚µ‚¢‘å—e—ÊƒfƒBƒXƒN
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

	// ƒpƒ‰ƒ[ƒ^ó‚¯æ‚è
	ASSERT(this);
	ASSERT((uID >= IDM_NEWSASI) && (uID <= IDM_NEWMO));
	uID -= IDM_NEWSASI;
	ASSERT(uID <= 2);

	// í•Ê‚ğ“n‚·
	dlg.m_nType = (int)uID;

	// ƒXƒPƒWƒ…[ƒ‰‚Ìó‘Ô‚ğ“¾‚Ä‚¨‚­(’â~‚Íƒ_ƒCƒAƒƒO“à•”‚Ås‚È‚¤)
	bRun = GetScheduler()->IsEnable();

	// ƒ_ƒCƒAƒƒOÀs
	if (dlg.DoModal() != IDOK) {
		// ƒLƒƒƒ“ƒZƒ‹‚µ‚½ê‡
		return;
	}

	// Às‚µ‚½ê‡‚ÍAŒ‹‰Ê•]‰¿
	if (!dlg.IsSucceeded()) {
		if (dlg.IsCanceled()) {
			// ƒLƒƒƒ“ƒZƒ‹‚µ‚½
			::GetMsg(IDS_CANCEL, strMsg);
		}
		else {
			// ì‚ë‚¤‚Æ‚µ‚½‚ª¸”s‚µ‚½
			::GetMsg(IDS_CREATEERR, strMsg);
		}
		MessageBox(strMsg, NULL, MB_ICONSTOP | MB_OK);
		GetScheduler()->Reset();
		GetScheduler()->Enable(bRun);
		ResetCaption();
		return;
	}

	// î•ñƒƒbƒZ[ƒW‚ğ“¾‚é
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

		// ‚»‚Ì‘¼(‚ ‚è“¾‚È‚¢)
		default:
			ASSERT(FALSE);
			nMsg = 0;
			break;
	}

	// ƒƒbƒZ[ƒW‚ğƒ[ƒh
	::GetMsg(nMsg, strMsg);

	// MO‚Ìƒ}ƒEƒ“ƒg
	if (uID == 2) {
		// ƒtƒ‰ƒOƒ`ƒFƒbƒN
		if (dlg.m_bMount) {
			// MO—LŒø‚©
			if (m_pSASI->IsValid()) {
				// ƒtƒ@ƒCƒ‹–¼æ“¾
				lpszPath = dlg.GetPath();

				// ƒI[ƒvƒ“‚ÆMRU
				path.SetPath(lpszPath);
				if (m_pSASI->Open(path)) {
					GetConfig()->SetMRUFile(2, lpszPath);
				}
			}
		}
	}

	// ƒXƒe[ƒ^ƒXƒƒbƒZ[ƒWAÄŠJ
	SetInfo(strMsg);
	GetScheduler()->Reset();
	GetScheduler()->Enable(bRun);
	ResetCaption();
}

//---------------------------------------------------------------------------
//
//	ƒIƒvƒVƒ‡ƒ“
//
//---------------------------------------------------------------------------
void CFrmWnd::OnOptions()
{
	Config config;
	CConfigSheet sheet(this);

	// İ’èƒf[ƒ^‚ğæ“¾
	GetConfig()->GetConfig(&config);

	// ƒvƒƒpƒeƒBƒV[ƒg‚ğÀs
	sheet.m_pConfig = &config;
	if (sheet.DoModal() != IDOK) {
		return;
	}

	// ƒf[ƒ^“]‘—
	GetConfig()->SetConfig(&config);

	// “K—p(VMƒƒbƒN‚µ‚Äs‚¤)
	::LockVM();
	ApplyCfg();
	GetScheduler()->Reset();
	ResetCaption();
	::UnlockVM();
}

//---------------------------------------------------------------------------
//
//	d‚Ë‚Ä•\¦
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
//	d‚Ë‚Ä•\¦ UI
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

	// ‚±‚Ìƒƒjƒ…[©‘Ì‚Ìˆ—
	if (IsPopupSWnd()) {
		// ƒ|ƒbƒvƒAƒbƒv‚Ìê‡‚ÍˆÓ–¡‚ª‚È‚¢
		pCmdUI->Enable(FALSE);
	}
	else {
		// ƒ`ƒƒƒCƒ‹ƒh‚Ìê‡AƒTƒuƒEƒBƒ“ƒhƒE‚ª1‚ÂˆÈã‚ ‚ê‚ÎTRUE
		if (GetView()->GetSubWndNum() >= 1) {
			pCmdUI->Enable(TRUE);
		}
		else {
			pCmdUI->Enable(FALSE);
		}
	}

	// Windowƒƒjƒ…[‚ğ’T‚·
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

	// 6‚Â‚ğc‚µ‚Äíœ
	while (pSubMenu->GetMenuItemCount() > 6) {
		pSubMenu->DeleteMenu(6, MF_BYPOSITION);
	}

	// —LŒø‚ÈƒTƒuƒEƒBƒ“ƒhƒE‚ª‚ ‚é‚©
	pWnd = GetView()->GetFirstSWnd();
	if (!pWnd) {
		return;
	}

	// ƒZƒpƒŒ[ƒ^‚ğ’Ç‰Á
	pSubMenu->AppendMenu(MF_SEPARATOR, 0);

	// ƒƒjƒ…[‚ğ‡Ÿ’Ç‰Á
	uID = IDM_SWND_START;
	while (pWnd) {
		// ƒEƒBƒ“ƒhƒEƒ^ƒCƒgƒ‹,ID‚ğæ“¾
		pWnd->GetWindowText(string);
		dwID = pWnd->GetID();
		temp.Format("%c%c%c%c - ",
			(dwID >> 24) & 0xff,
			(dwID >> 16) & 0xff,
			(dwID >> 8) & 0xff,
			dwID & 0xff);
		string = temp + string;

		// ƒƒjƒ…[’Ç‰Á
		pSubMenu->AppendMenu(MF_STRING, uID, string);

		// Ÿ‚Ö
		pWnd = pWnd->m_pNextWnd;
		uID++;
	}
}

//---------------------------------------------------------------------------
//
//	•À‚×‚Ä•\¦
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
//	•À‚×‚Ä•\¦ UI
//
//---------------------------------------------------------------------------
void CFrmWnd::OnTileUI(CCmdUI *pCmdUI)
{
	if (IsPopupSWnd()) {
		// ƒ|ƒbƒvƒAƒbƒv‚Ìê‡‚ÍˆÓ–¡‚ª‚È‚¢
		pCmdUI->Enable(FALSE);
	}
	else {
		// ƒ`ƒƒƒCƒ‹ƒh‚Ìê‡AƒTƒuƒEƒBƒ“ƒhƒE‚ª1‚ÂˆÈã‚ ‚ê‚ÎTRUE
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
//	‘S‚ÄƒAƒCƒRƒ“‰»
//
//---------------------------------------------------------------------------
void CFrmWnd::OnIconic()
{
	CSubWnd *pSubWnd;

	ASSERT(this);
	ASSERT(GetView());
	ASSERT(GetView()->m_hWnd);

	// Å‰‚ÌƒTƒuƒEƒBƒ“ƒhƒE‚ğæ“¾
	pSubWnd = GetView()->GetFirstSWnd();

	// ƒ‹[ƒv
	while (pSubWnd) {
		pSubWnd->ShowWindow(SW_MINIMIZE);
		pSubWnd = pSubWnd->m_pNextWnd;
	}
}

//---------------------------------------------------------------------------
//
//	‘S‚ÄƒAƒCƒRƒ“‰» UI
//
//---------------------------------------------------------------------------
void CFrmWnd::OnIconicUI(CCmdUI *pCmdUI)
{
	// ƒTƒuƒEƒBƒ“ƒhƒE‚ª‘¶İ‚·‚ê‚ÎTRUE
	if (GetView()->GetSubWndNum() > 0) {
		pCmdUI->Enable(TRUE);
	}
	else {
		pCmdUI->Enable(FALSE);
	}
}

//---------------------------------------------------------------------------
//
//	ƒAƒCƒRƒ“‚Ì®—ñ
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
//	ƒAƒCƒRƒ“‚Ì®—ñ UI
//
//---------------------------------------------------------------------------
void CFrmWnd::OnArrangeIconUI(CCmdUI *pCmdUI)
{
	CSubWnd *pSubWnd;

	if (IsPopupSWnd()) {
		// ƒ|ƒbƒvƒAƒbƒv‚Ìê‡‚ÍˆÓ–¡‚ª‚È‚¢
		pCmdUI->Enable(FALSE);
		return;
	}

	// ƒTƒuƒEƒBƒ“ƒhƒE‚Ì’†‚ÅAƒAƒCƒRƒ“ó‘Ô‚Ì‚à‚Ì‚ª‚ ‚ê‚Î—LŒø
	pSubWnd = GetView()->GetFirstSWnd();
	while (pSubWnd) {
		// Å¬‰»‚³‚ê‚Ä‚¢‚é‚©
		if (pSubWnd->IsIconic()) {
			pCmdUI->Enable(TRUE);
			return;
		}

		// Ÿ‚ÌƒTƒuƒEƒBƒ“ƒhƒE
		pSubWnd = pSubWnd->m_pNextWnd;
	}

	// Å¬‰»‚³‚ê‚Ä‚¢‚é‚à‚Ì‚ª‚È‚¢‚Ì‚ÅA–³Œø
	pCmdUI->Enable(FALSE);
}

//---------------------------------------------------------------------------
//
//	‘S‚Ä‰B‚·
//
//---------------------------------------------------------------------------
void CFrmWnd::OnHide()
{
	CSubWnd *pSubWnd;

	ASSERT(this);
	ASSERT(GetView());
	ASSERT(GetView()->m_hWnd);

	// Å‰‚ÌƒTƒuƒEƒBƒ“ƒhƒE‚ğæ“¾
	pSubWnd = GetView()->GetFirstSWnd();

	// ƒ‹[ƒv
	while (pSubWnd) {
		pSubWnd->ShowWindow(SW_HIDE);
		pSubWnd = pSubWnd->m_pNextWnd;
	}
}

//---------------------------------------------------------------------------
//
//	‘S‚Ä‰B‚· UI
//
//---------------------------------------------------------------------------
void CFrmWnd::OnHideUI(CCmdUI *pCmdUI)
{
	// ƒTƒuƒEƒBƒ“ƒhƒE‚ª‘¶İ‚·‚ê‚ÎTRUE
	if (GetView()->GetSubWndNum() > 0) {
		pCmdUI->Enable(TRUE);
	}
	else {
		pCmdUI->Enable(FALSE);
	}
}

//---------------------------------------------------------------------------
//
//	‘S‚Ä•œŒ³
//
//---------------------------------------------------------------------------
void CFrmWnd::OnRestore()
{
	CSubWnd *pSubWnd;

	ASSERT(this);
	ASSERT(GetView());
	ASSERT(GetView()->m_hWnd);

	// Å‰‚ÌƒTƒuƒEƒBƒ“ƒhƒE‚ğæ“¾
	pSubWnd = GetView()->GetFirstSWnd();

	// ƒ‹[ƒv
	while (pSubWnd) {
		pSubWnd->ShowWindow(SW_RESTORE);
		pSubWnd = pSubWnd->m_pNextWnd;
	}
}

//---------------------------------------------------------------------------
//
//	‘S‚Ä•œŒ³ UI
//
//---------------------------------------------------------------------------
void CFrmWnd::OnRestoreUI(CCmdUI *pCmdUI)
{
	// ƒTƒuƒEƒBƒ“ƒhƒE‚ª‘¶İ‚·‚ê‚ÎTRUE
	if (GetView()->GetSubWndNum() > 0) {
		pCmdUI->Enable(TRUE);
	}
	else {
		pCmdUI->Enable(FALSE);
	}
}

//---------------------------------------------------------------------------
//
//	ƒEƒBƒ“ƒhƒEw’è
//
//---------------------------------------------------------------------------
void CFrmWnd::OnWindow(UINT uID)
{
	CSubWnd *pSubWnd;
	int n;

	ASSERT(this);
	ASSERT(GetView());
	ASSERT(GetView()->m_hWnd);

	// ƒTƒuƒEƒBƒ“ƒhƒE‚ğ“Á’è‚·‚é
	n = (int)(uID - IDM_SWND_START);
	pSubWnd = GetView()->GetFirstSWnd();
	if (!pSubWnd) {
		return;
	}

	// ŒŸõƒ‹[ƒv
	while (n > 0) {
		if (!pSubWnd) {
			return;
		}

		n--;
		pSubWnd = pSubWnd->m_pNextWnd;
	}

	// pSubWnd‚ğƒZƒŒƒNƒg
	pSubWnd->ShowWindow(SW_RESTORE);
	pSubWnd->SetWindowPos(&wndTop, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
}

//---------------------------------------------------------------------------
//
//	ƒo[ƒWƒ‡ƒ“î•ñ
//
//---------------------------------------------------------------------------
void CFrmWnd::OnAbout()
{
	CAboutDlg dlg(this);

	// ƒ‚[ƒ_ƒ‹ƒ_ƒCƒAƒƒO‚ğÀs
	dlg.DoModal();
}

#endif	// _WIN32
