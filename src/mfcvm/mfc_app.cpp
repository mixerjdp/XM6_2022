//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 ÇoÇhÅD(ytanaka@ipc-tokai.or.jp)
//	[  Aplicaciones MFC ]
//
//---------------------------------------------------------------------------

#if defined(_WIN32)
 
#include "os.h"
#include "xm6.h"
#include "filepath.h"
#include "mfc_frm.h"
#include "mfc_asm.h"
#include "mfc_com.h"
#include "mfc_info.h"
#include "mfc_res.h"
#include "mfc_app.h"

//---------------------------------------------------------------------------
//
//	Instancia de la aplicacion
//
//---------------------------------------------------------------------------
CApp theApp;

//---------------------------------------------------------------------------
//
//	Definicion de puntero de funcion
//
//---------------------------------------------------------------------------
extern "C" {
typedef int (WINAPI *DRAWTEXTWIDE)(HDC, LPCWSTR, int, LPRECT, UINT);
}

//---------------------------------------------------------------------------
//
//	Trabajo Worker global
//
//---------------------------------------------------------------------------
VM *pVM;								// Virtual Machine

//---------------------------------------------------------------------------
//
//	Trabajo Worker estatico
//
//---------------------------------------------------------------------------
static CCriticalSection csect;			// Seccion critica para el bloqueo de maquinas virtuales
static BOOL bJapanese;					// Bandera de discriminacion japonesa/inglesa
static BOOL bWinNT;						// Bandera de identificacion de Windows NT y Windows 9x
static BOOL bSupport932;				// Bandera de apoyo al CP932 (SHIFT-JIS)
static BOOL bMMX;						// Bandera de identificacion MMX
static BOOL bCMOV;						// Bandera de identificacion CMOV
static LPSTR lpszInfoMsg;				// Buffer de mensajes de informacion
static DRAWTEXTWIDE pDrawTextW;			// DrawTextW

//---------------------------------------------------------------------------
//
//	Determinacion del entorno linguistico japones
//
//---------------------------------------------------------------------------
BOOL FASTCALL IsJapanese(void)
{
	return bJapanese;
}

//---------------------------------------------------------------------------
//
//	Determinacion de Windows NT
//
//---------------------------------------------------------------------------
BOOL FASTCALL IsWinNT(void)
{
	return bWinNT;
}

//---------------------------------------------------------------------------
//
//	Determinacion de la ayuda al CP932
//
//---------------------------------------------------------------------------
BOOL FASTCALL Support932(void)
{
	return bSupport932;
}

//---------------------------------------------------------------------------
//
//	Determinacion de MMX
//
//---------------------------------------------------------------------------
BOOL FASTCALL IsMMX(void)
{
	return bMMX;
}

//---------------------------------------------------------------------------
//
//	Determinacion del CMOV
//
//---------------------------------------------------------------------------
BOOL FASTCALL IsCMOV(void)
{
	return bCMOV;
}

//---------------------------------------------------------------------------
//
//	Recuperacion de mensajes
//
//---------------------------------------------------------------------------
void FASTCALL GetMsg(UINT uID, CString& string)
{
	// Puede volar con uID=0
	if (uID == 0) {
		string.Empty();
		return;
	}

	// Japones.
	if (IsJapanese()) {
		if (!string.LoadString(uID)) {
#if defined(_DEBUG)
			TRACE(_T("GetMsg:ï∂éöóÒÉçÅ[ÉhÇ…é∏îs ID:%d\n"), uID);
#endif	// _DEBUG
			string.Empty();
		}
		return;
	}

	// Ingles. Prueba con +5000
	if (string.LoadString(uID + 5000)) {
		return;
	}

	// +0 de nuevo
	if (!string.LoadString(uID)) {
#if defined(_DEBUG)
		TRACE(_T("GetMsg:ï∂éöóÒÉçÅ[ÉhÇ…é∏îs ID:%d\n"), uID);
#endif	// _DEBUG
		string.Empty();
	}
}

//---------------------------------------------------------------------------
//
//	Obtener maquina virtual
//
//---------------------------------------------------------------------------
VM* FASTCALL GetVM(void)
{
	ASSERT(pVM);
	return pVM;
}

//---------------------------------------------------------------------------
//
//	Bloquear la maquina virtual
//
//---------------------------------------------------------------------------
void FASTCALL LockVM(void)
{
	csect.Lock();
}

//---------------------------------------------------------------------------
//
//	Desbloquear la maquina virtual
//
//---------------------------------------------------------------------------
void FASTCALL UnlockVM(void)
{
	csect.Unlock();
}

//---------------------------------------------------------------------------
//
//	Dialogo de apertura de archivos
//	LPSZPath debe ser inicializado y llamado.
//
//---------------------------------------------------------------------------
BOOL FASTCALL FileOpenDlg(CWnd *pParent, LPSTR lpszPath, UINT nFilterID)
{
	OPENFILENAME ofn;
	TCHAR szFilter[0x200];
	TCHAR szDrive[_MAX_DRIVE];
	TCHAR szDir[_MAX_DIR];
	CString strFilter;
	int i;
	int nLen;
	WIN32_FIND_DATA wfd;
	HANDLE hFind;

	ASSERT(pParent);
	ASSERT(lpszPath);
	ASSERT(nFilterID);

	// Estructura del conjunto
	memset(&ofn, 0, sizeof(OPENFILENAME));
	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = pParent->m_hWnd;
	ofn.lpstrFilter = szFilter;
	ofn.lpstrFile = lpszPath;
	ofn.nMaxFile = _MAX_PATH;
	ofn.lpstrInitialDir = Filepath::GetDefaultDir();
	ofn.Flags = OFN_EXPLORER | OFN_HIDEREADONLY | OFN_FILEMUSTEXIST;

	// Fijar el filtro
	GetMsg(nFilterID, strFilter);
	_tcscpy(szFilter, (LPCTSTR)strFilter);
	nLen = (int)_tcslen(szFilter);
	for (i=0; i<nLen; i++) {
		if (szFilter[i] == _T('|')) {
			szFilter[i] = _T('\0');
		}
	}

	// Ejecucion de dialogos comunes
	if (!GetOpenFileName(&ofn)) {
		return FALSE;
	}

	// Obtener el nombre de archivo adecuado (FindFirstFile solo da el nombre del archivo + la extension)
	hFind = FindFirstFile(lpszPath, &wfd);
	FindClose(hFind);
	_tsplitpath(lpszPath, szDrive, szDir, NULL, NULL);
	_tcscpy(lpszPath, szDrive);
	_tcscat(lpszPath, szDir);
	_tcscat(lpszPath, wfd.cFileName);

	// Guardar el directorio por defecto
	Filepath::SetDefaultDir(lpszPath);

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	Dialogo para guardar el archivo
//	Å¶lpszPath debe ser inicializado antes de llamar. lpszExt es valido solo para los tres primeros caracteres.
//
//---------------------------------------------------------------------------
BOOL FASTCALL FileSaveDlg(CWnd *pParent, LPSTR lpszPath, LPCTSTR lpszExt, UINT nFilterID)
{
	OPENFILENAME ofn;
	TCHAR szFilter[0x200];
	CString strFilter;
	int i;
	int nLen;

	ASSERT(pParent);
	ASSERT(lpszPath);
	ASSERT(nFilterID);

	// Estructura del conjunto
	memset(&ofn, 0, sizeof(OPENFILENAME));
	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = pParent->m_hWnd;
	ofn.lpstrFilter = szFilter;
	ofn.lpstrFile = lpszPath;
	ofn.nMaxFile = _MAX_PATH;
	ofn.lpstrInitialDir = Filepath::GetDefaultDir();
	ofn.Flags = OFN_EXPLORER | OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT;
	ofn.lpstrDefExt = lpszExt;

	// Fijar el filtro
	GetMsg(nFilterID, strFilter);
	_tcscpy(szFilter, (LPCTSTR)strFilter);
	nLen = (int)_tcslen(szFilter);
	for (i=0; i<nLen; i++) {
		if (szFilter[i] == _T('|')) {
			szFilter[i] = _T('\0');
		}
	}

	// Ejecucion de dialogos comunes
	if (!GetSaveFileName(&ofn)) {
		return FALSE;
	}

	// Guardar el directorio por defecto
	Filepath::SetDefaultDir(lpszPath);

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	Configuracion de los mensajes de informacion
//
//---------------------------------------------------------------------------
void FASTCALL SetInfoMsg(LPCTSTR lpszMsg, BOOL bRec)
{
	// Ver los indicadores de memoria
	if (bRec) {
		// Memorizar la direccion del buffer
		lpszInfoMsg = (LPSTR)lpszMsg;
		return;
	}

	// Si no se indica la direccion del bufer, se ignora
	if (!lpszInfoMsg) {
		return;
	}

	// Comprobacion del tamano del buffer
	if (_tcslen(lpszInfoMsg) < CInfo::InfoBufMax) {
		// Copiar la cadena dada
		_tcscpy(lpszInfoMsg, lpszMsg);
	}
}

//---------------------------------------------------------------------------
//
//	DrawTextW
//	No hacer nada en sistemas operativos no compatibles
//
//---------------------------------------------------------------------------
int FASTCALL DrawTextWide(HDC hDC, LPCWSTR lpString, int nCount, LPRECT lpRect, UINT uFormat)
{
	// ?Es compatible?
	if (!pDrawTextW) {
		// Nada que hacer
		return 1;
	}

	// Dibujar en caracteres anchos
	return pDrawTextW(hDC, lpString, nCount, lpRect, uFormat);
}

//===========================================================================
//
//	Aplicaciones
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	Constructor
//
//---------------------------------------------------------------------------
CApp::CApp() : CWinApp(_T("XM6"))
{
	m_hMutex = NULL;
	m_hUser32 = NULL;
}

//---------------------------------------------------------------------------
//
//	Inicializacion de instancias
//
//---------------------------------------------------------------------------
BOOL CApp::InitInstance()
{
	CFrmWnd *pFrmWnd;

	// Borrar el directorio por defecto
	Filepath::ClearDefaultDir();

	// Evaluacion de ambiente
	if (!CheckEnvironment()) {
		return FALSE;
	}

	// Comprobacion de la doble activacion
	if (!CheckMutex()) {
		// Si hay una linea de comandos, pase
		if (m_lpCmdLine[0] != _T('\0')) {
			SendCmd();
		}
		return FALSE;
	}

	// Crear ventana principal (asignar a m_pMainWnd inmediatamente)
	pFrmWnd = new CFrmWnd();
	m_pMainWnd = (CWnd*)pFrmWnd;

	// Inicializacion
	if (!pFrmWnd->Init()) {
		return FALSE;
	}

	// Mostrar
	pFrmWnd->ShowWindow(m_nCmdShow);
	pFrmWnd->UpdateWindow();

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	Fin de la instancia
//
//---------------------------------------------------------------------------
BOOL CApp::ExitInstance()
{
	// Borrar Mutex
	if (m_hMutex) {
		::CloseHandle(m_hMutex);
		m_hMutex = NULL;
	}

	// Liberar USER32.DLL
	if (m_hUser32) {
		::FreeLibrary(m_hUser32);
		m_hUser32 = NULL;
	}

	// Clase basica
	return TRUE;
}

//---------------------------------------------------------------------------
//
//	Comprobacion de mutex
//
//---------------------------------------------------------------------------
BOOL FASTCALL CApp::CheckMutex()
{
	HANDLE hMutex;

	ASSERT(this);

	// Creada con o sin
	hMutex = ::CreateMutex(NULL, TRUE, _T("XM6"));
	if (hMutex) {
		// ?Ya esta activado?
		if (::GetLastError() == ERROR_ALREADY_EXISTS) {
			return FALSE;
		}

		// OK
		m_hMutex = hMutex;
		return TRUE;
	}

	// Por que ha fallado
	return FALSE;
}

//---------------------------------------------------------------------------
//
//	Juzgando el entorno
//
//---------------------------------------------------------------------------
BOOL FASTCALL CApp::CheckEnvironment()
{
	OSVERSIONINFO ovi;
	CString strError;

	ASSERT(this);

	//
	//	Determinacion del sistema operativo
	//

	// Determinacion del entorno linguistico japones
	::bJapanese = FALSE;
	if (PRIMARYLANGID(GetSystemDefaultLangID()) == LANG_JAPANESE) {
		if (PRIMARYLANGID(GetUserDefaultLangID()) == LANG_JAPANESE) {
			// Determinacion por defecto del sistema y del usuario
			::bJapanese = TRUE;
		}
	}

	// Determinacion de Windows NT
	memset(&ovi, 0, sizeof(ovi));
	ovi.dwOSVersionInfoSize = sizeof(ovi);
	VERIFY(::GetVersionEx(&ovi));
	if (ovi.dwPlatformId == VER_PLATFORM_WIN32_NT) {
		::bWinNT = TRUE;
	}
	else {
		::bWinNT = FALSE;
	}

	// Determinacion de la compatibilidad con la pagina de codigos 932 (asumiendo la compatibilidad con UNICODE)
	::bSupport932 = FALSE;
	::pDrawTextW = NULL;
	if (::bWinNT) {
		// Sistemas operativos compatibles con UNICODE
		if (::IsValidCodePage(932)) {
			// Cargar USER32.DLL
			m_hUser32 = ::LoadLibrary(_T("USER32.DLL"));
			if (m_hUser32) {
				// Obtener la direccion de DrawTextW
				pDrawTextW = (DRAWTEXTWIDE)::GetProcAddress(m_hUser32, _T("DrawTextW"));
				if (pDrawTextW) {
					// Conversion a y desde CP932
					::bSupport932 = TRUE;
				}
			}
		}
	}

	//
	//	Determinacion del procesador
	//

	// Determinacion del CMOV
	::bCMOV = FALSE;
	if (::IsCMOVSupport()) {
		::bCMOV = TRUE;
	}

	// Determinacion de MMX (solo Windows 98 o posterior)
	::bMMX = FALSE;
	if (ovi.dwMajorVersion >= 4) {
		// Windows95 or WindowsNT4 à»ç~
		if ((ovi.dwMajorVersion == 4) && (ovi.dwMinorVersion == 0)) {
			// Windows95 or WindowsNT4
			::bMMX = FALSE;
		}
		else {
			// Por el procesador
			::bMMX = ::IsMMXSupport();
		}
	}

	// Necesario para CMOV y MMX desde la version 2.05
	if (!::bCMOV || !::bMMX) {
		::GetMsg(IDS_PROCESSOR ,strError);
		AfxMessageBox(strError, MB_ICONSTOP | MB_OK);
		return FALSE;
	}

	// Todo bien
	return TRUE;
}

//---------------------------------------------------------------------------
//
//	Transmision de la linea de comandos
//
//---------------------------------------------------------------------------
void FASTCALL CApp::SendCmd()
{
	HWND hWnd;
	COPYDATASTRUCT cds;

	ASSERT(this);

	// Busqueda en la ventana
	hWnd = SearchXM6Wnd();
	if (!hWnd) {
		return;
	}

	// WM_COPYDATA
	memset(&cds, 0, sizeof(cds));
	cds.dwData = WM_COPYDATA;
	cds.cbData = ((DWORD)_tcslen(m_lpCmdLine) + 1) * sizeof(TCHAR);
	cds.lpData = m_lpCmdLine;
	::SendMessage(hWnd, WM_COPYDATA, NULL, (LPARAM)&cds);
}

//---------------------------------------------------------------------------
//
//	Busqueda de ventanas XM6
//
//---------------------------------------------------------------------------
HWND FASTCALL CApp::SearchXM6Wnd()
{
	HWND hWnd;

	//La ventana es NULL
	hWnd = NULL;

	// Busque en
	::EnumWindows(EnumXM6Proc, (LPARAM)&hWnd);

	// Poner el resultado en una funcion callback
	return hWnd;
}

//---------------------------------------------------------------------------
//
//	Llamada a la busqueda de ventanas en XM6
//
//---------------------------------------------------------------------------
BOOL CALLBACK CApp::EnumXM6Proc(HWND hWnd, LPARAM lParam)
{
	HWND *phWnd;
	LONG lUser;

	// Recepcion de parametros
	phWnd = (HWND*)lParam;
	ASSERT(phWnd);
	ASSERT(*phWnd == NULL);

	// Obtener los datos del usuario para la ventana correspondiente
	lUser = ::GetWindowLong(hWnd, GWL_USERDATA);

	// Realizar una comprobacion XM6
	if (lUser == (LONG)MAKEID('X', 'M', '6', ' ')) {
		// Se determina que es una ventana de marco XM6 y se termina
		*phWnd = hWnd;
		return FALSE;
	}

	// Continua porque es diferente
	return TRUE;
}

#endif	// _WIN32
