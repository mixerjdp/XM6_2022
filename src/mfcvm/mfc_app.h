//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 ＰＩ．(ytanaka@ipc-tokai.or.jp)
//	[ MFC Aplicacion ]
//
//---------------------------------------------------------------------------

#if defined(_WIN32)

#if !defined(mfc_app_h)
#define mfc_app_h

//===========================================================================
//
//	Aplicaciones
//
//===========================================================================
class CApp : public CWinApp
{
public:
	CApp();
										// コンストラクタ
	BOOL InitInstance();
										// インスタンス初期化
	BOOL ExitInstance();
										// インスタンス終了

private:
	BOOL FASTCALL CheckMutex();
										// Comprobacion de mutex
	BOOL FASTCALL CheckEnvironment();
										// Control ambiental
	void FASTCALL SendCmd();
										// Enviar comando
	HWND FASTCALL SearchXM6Wnd();
										// Busqueda de ventanas XM6
	static BOOL CALLBACK EnumXM6Proc(HWND hWnd, LPARAM lParam);
										// Llamadas de enumeracion de ventanas
	HANDLE m_hMutex;
										// Manipulacion de Mutex
	HMODULE m_hUser32;
										// Manipulacion de USER32.DLL
};

#endif	// mfc_app_h
#endif	// _WIN32
