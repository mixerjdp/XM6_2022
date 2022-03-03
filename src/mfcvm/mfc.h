//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 ÇoÇhÅD(ytanaka@ipc-tokai.or.jp)
//	[ MFC ]
//
//---------------------------------------------------------------------------

#if defined(_WIN32)

#if !defined(mfc_h)
#define mfc_h

//---------------------------------------------------------------------------
//
//	#include
//
//---------------------------------------------------------------------------
#define WINVER					0x600	// Windows98,Me,2000,XPà»ç~
#define _WIN32_WINNT			0x600	// Windows98,Me,2000,XPà»ç~
#define VC_EXTRALEAN

// MFC
#include <afxwin.h>
#include <afxext.h>
#include <afxdlgs.h>
#include <afxcmn.h>
#include <afxmt.h>
#include <afxconv.h>
#include <windows.h>

// Win32API
#include <imm.h>
#include <mmsystem.h>
#include <shlobj.h>

// DirectX 
#define DIRECTSOUND_VERSION		0x700	// DirectX5ÇéwíË
#include <dsound.h>
#define DIRECTINPUT_VERSION		0x700	// DirectX5ÇéwíË
#include <dinput.h>

// C Runtime
#include <math.h>
#include <io.h>
#include <fcntl.h>
#include <sys/stat.h>

//---------------------------------------------------------------------------
//
//	#define
//
//---------------------------------------------------------------------------
#if defined(_DEBUG)
#define new		DEBUG_NEW
#endif	// _DEBUG

#if defined(_MSC_VER) && defined(_M_IX86)
#define FASTCALL	__fastcall
#else
#define FASTCALL
#endif	// _MSC_VER

//---------------------------------------------------------------------------
//
//	Declaracion de clases
//
//---------------------------------------------------------------------------
class VM;
										// Maquina Virtual

class CApp;
										// Aplicaciones
class CFrmWnd;
										// Ventana del marco
class CDrawView;
										// Vista de dibujo
class CStatusView;
										// Vista de estado

class CSubWnd;
										// Subventana
class CSubTextWnd;
										// Subventana (texto)
class CSubListWnd;
										// Subventana (control de listas)

class CComponent;
										// Componentes comunes
class CConfig;
										// Componentes de configuracion
class CScheduler;
										// Componente del programa
class CSound;
										// Componentes de sonido
class CInput;
										// Componentes de entrada
class CPort;
										// Componentes de los puertos
class CMIDI;
										// Componentes MIDI
class CTKey;
										// Componentes TrueKey
class CHost;
										// Componentes de la sede
class CInfo;
										// Componentes de informacion

//---------------------------------------------------------------------------
//
//	Global
//
//---------------------------------------------------------------------------
extern VM *pVM;
										// Maquinas virtuales
BOOL FASTCALL IsJapanese(void);
										// Determinacion del entorno linguistico japones
BOOL FASTCALL IsWinNT(void);
										// Determinacion del entorno de Windows NT
BOOL FASTCALL Support932(void);
										// Determine si el soporte del CP932 esta disponible
BOOL FASTCALL IsMMX(void);
										// Determinar el entorno MMX
BOOL FASTCALL IsCMOV(void);
										// Determinacion del apoyo de la CMOV
void FASTCALL GetMsg(UINT uID, CString& string);
										// Recuperacion de mensajes
VM* FASTCALL GetVM(void);
										// Obtener la maquina virtual
void FASTCALL LockVM(void);
										// Bloquear la maquina virtual
void FASTCALL UnlockVM(void);
										// Desbloquear la maquina virtual
BOOL FASTCALL FileOpenDlg(CWnd *pParent, LPTSTR lpszPath, UINT nFilterID);
										// Dialogo de apertura de archivos
BOOL FASTCALL FileSaveDlg(CWnd *pParent, LPTSTR lpszPath, LPCTSTR lpszExt, UINT nFilterID);
										// Dialogo para guardar el archivo
void FASTCALL SetInfoMsg(LPCTSTR lpszBuf, BOOL bRec);
										// Mensaje de informacion que contiene sub
int FASTCALL DrawTextWide(HDC hDC, LPCWSTR lpString, int nCount, LPRECT lpRect, UINT uFormat);
										// DrawTextWrapper

#endif	// mfc_h
#endif	// _WIN32
