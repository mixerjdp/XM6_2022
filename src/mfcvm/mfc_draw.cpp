//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 ＰＩ．(ytanaka@ipc-tokai.or.jp)
//	[ Vista de dibujo del MFC ]
//
//---------------------------------------------------------------------------

#if defined(_WIN32)

#include "os.h"
#include "xm6.h"
#include "vm.h"
#include "render.h"
#include "crtc.h"
#include "config.h"
#include "mfc_sub.h" 
#include "mfc_frm.h"
#include "mfc_com.h"
#include "mfc_sch.h"
#include "mfc_cpu.h"
#include "mfc_cfg.h"
#include "mfc_res.h"
#include "mfc_sch.h"
#include "mfc_inp.h"
#include "mfc_draw.h"

//===========================================================================
//
//	Vista de dibujo
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	Constructor
//
//---------------------------------------------------------------------------
CDrawView::CDrawView()
{
	// Inicializacion de la pieza (basica)	
	m_bEnable = FALSE;
	m_pSubWnd = NULL;
	m_pFrmWnd = NULL;

	// Componentes
	m_pScheduler = NULL;
	m_pInput = NULL;

	// Inicializacion de la pieza (dibujo general)
	m_Info.bPower = FALSE;
	m_Info.pRender = NULL;
	m_Info.pWork = NULL;
	m_Info.dwDrawCount = 0;

	// Inicializacion de la pieza (seccion DIB)
	m_Info.hBitmap = NULL;
	m_Info.pBits = NULL;
	m_Info.nBMPWidth = 0;
	m_Info.nBMPHeight = 0;

	// Inicializacion de la pieza (ajuste de tamano))
	m_Info.nRendWidth = 0;
	m_Info.nRendHeight = 0;
	m_Info.nRendHMul = 0;
	m_Info.nRendVMul = 0;
	m_Info.nLeft = 0;
	m_Info.nTop = 0;
	m_Info.nWidth = 0;
	m_Info.nHeight = 0;

	// Inicializacion de la pieza (Blt)
	m_Info.nBltTop = 0;
	m_Info.nBltBottom = 0;
	m_Info.nBltLeft = 0;
	m_Info.nBltRight = 0;
	m_Info.bBltAll = TRUE;
	m_Info.bBltStretch = FALSE;
}

//---------------------------------------------------------------------------
//
//	Mapa de mensajes
//
//---------------------------------------------------------------------------
BEGIN_MESSAGE_MAP(CDrawView, CView)
	ON_WM_CREATE()
	ON_WM_DESTROY()
	ON_WM_SIZE()
	ON_WM_PAINT()
	ON_WM_ERASEBKGND()
	ON_MESSAGE(WM_DISPLAYCHANGE, OnDisplayChange)
	ON_WM_DROPFILES()
#if _MFC_VER >= 0x600
	ON_WM_MOUSEWHEEL()
#endif	// _MFC_VER
	ON_WM_KEYDOWN()
	ON_WM_SYSKEYDOWN()
	ON_WM_KEYUP()
	ON_WM_SYSKEYUP()
	ON_WM_MOVE()
END_MESSAGE_MAP()

//---------------------------------------------------------------------------
//
//	Inicializacion
//
//---------------------------------------------------------------------------
BOOL FASTCALL CDrawView::Init(CWnd *pParent)
{
	ASSERT(pParent);

	// Memoria de la ventana del marco
	m_pFrmWnd = (CFrmWnd*)pParent;

	// Crear como primera vista
	if (!Create(NULL, NULL, WS_CHILD | WS_VISIBLE | WS_CLIPCHILDREN,
				CRect(0, 0, 0, 0), pParent, AFX_IDW_PANE_FIRST, NULL)) {
		return FALSE;
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	Preparando la creacion de una ventana
//
//---------------------------------------------------------------------------
BOOL CDrawView::PreCreateWindow(CREATESTRUCT& cs)
{
	// Clase basica
	if (!CView::PreCreateWindow(cs)) {
		return FALSE;
	}

	// Anadir WS_CLIPCHILDREN
	cs.style |= WS_CLIPCHILDREN;

	// Anadir un borde de cliente
	cs.dwExStyle |= WS_EX_CLIENTEDGE;

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	Creacion de ventanas
//
//---------------------------------------------------------------------------
int CDrawView::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	// Clase basica
	if (CView::OnCreate(lpCreateStruct) != 0) {
		return -1;
	}

	// IME off
	::ImmAssociateContext(m_hWnd, (HIMC)NULL);

	// Creacion de fuentes de texto
	if (IsJapanese()) {
		// Entorno linguistico japones
		m_TextFont.CreateFont(14, 0, 0, 0,
							FW_NORMAL, 0, 0, 0,
							SHIFTJIS_CHARSET, OUT_DEFAULT_PRECIS,
							CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
							FIXED_PITCH, NULL);
	}
	else {
		// Entorno linguistico ingles
		m_TextFont.CreateFont(14, 0, 0, 0,
							FW_NORMAL, 0, 0, 0,
							DEFAULT_CHARSET, OUT_DEFAULT_PRECIS,
							CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY,
							FIXED_PITCH, NULL);
	}

	// Permiso para arrastrar y soltar
	DragAcceptFiles(TRUE);

	return 0;
}

//---------------------------------------------------------------------------
//
//	Eliminacion de ventanas
//
//---------------------------------------------------------------------------
void CDrawView::OnDestroy()
{
	// Detener la operacion
	Enable(FALSE);

	// Borrar mapa de bits
	if (m_Info.hBitmap) {
		::DeleteObject(m_Info.hBitmap);
		m_Info.hBitmap = NULL;
		m_Info.pBits = NULL;
	}

	// Borrar la fuente del texto
	m_TextFont.DeleteObject();

	// A la clase basica
	CView::OnDestroy();
}

//---------------------------------------------------------------------------
//
//	Redimensionar
//
//---------------------------------------------------------------------------
void CDrawView::OnSize(UINT nType, int cx, int cy)
{
	// Actualizacion del mapa de bits
	SetupBitmap();

	// Clase basica
	CView::OnSize(nType, cx, cy);
}

//---------------------------------------------------------------------------
//
//	Dibujo
//
//---------------------------------------------------------------------------
void CDrawView::OnPaint()
{
	Render *pRender;
	CRTC *pCRTC;
	const CRTC::crtc_t *p;
	CFrmWnd *pFrmWnd;
	PAINTSTRUCT ps;

	// Bloqueo VM
	::LockVM();

	// Todas las banderas de dibujo ON
	m_Info.bBltAll = TRUE;

	// Si esta activado y el programador esta desactivado, crea en el buffer Mix (bastante forzado)
	if (m_bEnable) {
		pFrmWnd = (CFrmWnd*)GetParent();
		ASSERT(pFrmWnd);
		if (!pFrmWnd->GetScheduler()->IsEnable()) {
			// El renderizado y el CRTC estan siempre presentes si estan activados
			pRender = (Render*)::GetVM()->SearchDevice(MAKEID('R', 'E', 'N', 'D'));
			ASSERT(pRender);
			pCRTC = (CRTC*)::GetVM()->SearchDevice(MAKEID('C', 'R', 'T', 'C'));
			ASSERT(pCRTC);
			p = pCRTC->GetWorkAddr();

			// Crear
			m_Info.bPower = ::GetVM()->IsPower();
			if (m_Info.bPower) {
				pRender->Complete();
				pRender->EnableAct(TRUE);
				pRender->StartFrame();
				pRender->HSync(p->v_dots);
				pRender->EndFrame();
			}
			else {
				// Borrar todos los mapas de bits
				memset(m_Info.pBits, 0, m_Info.nBMPWidth * m_Info.nBMPHeight * 4);
			}

			// Dibujo (conectar con CDrawView::OnDraw)
			CView::OnPaint();

			// Desbloqueo VM
			::UnlockVM();
			return;
		}
	}

	// Solo consigue DCs (tontos)
	BeginPaint(&ps);
	EndPaint(&ps);

	// Desbloqueo VM
	::UnlockVM();
}

//---------------------------------------------------------------------------
//
//	Dibujo de fondo
//
//---------------------------------------------------------------------------
BOOL CDrawView::OnEraseBkgnd(CDC *pDC)
{
	CRect rect;

	// Si no esta activado, rellenar con negro
	if (!m_bEnable) {
		GetClientRect(&rect);
		pDC->FillSolidRect(&rect, RGB(0, 0, 0));
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	Cambiar el entorno de visualizacion
//
//---------------------------------------------------------------------------
LRESULT CDrawView::OnDisplayChange(WPARAM /* wParam */, LPARAM /* lParam */)
{
	// Preparacion del mapa de bits
	SetupBitmap();

	return 0;
}

//---------------------------------------------------------------------------
//
//	Caida de archivos
//
//---------------------------------------------------------------------------
void CDrawView::OnDropFiles(HDROP hDropInfo)
{
	TCHAR szPath[_MAX_PATH];
	POINT point;
	CRect rect;
	int nFiles;
	int nDrive;

	// Obtenga el numero de archivos que se han eliminado
	nFiles = ::DragQueryFile(hDropInfo, 0xffffffff, szPath, _MAX_PATH);
	ASSERT(nFiles > 0);

	// Determinar el accionamiento desde la posicion de caida
	::DragQueryPoint(hDropInfo, &point);
	GetClientRect(rect);
	if (point.x < (rect.right >> 1)) {
		// Mitad izquierda (unidad 0)
		nDrive = 0;
	}
	else {
		// Mitad derecha (unidad 1)
		nDrive = 1;
	}

	// Dividir por numero de archivos
	if (nFiles == 1) {
		// Un solo archivo es la mitad izquierda y la mitad derecha de la ventana
		::DragQueryFile(hDropInfo, 0, szPath, _MAX_PATH);
		m_pFrmWnd->InitCmdSub(nDrive, szPath);
	}
	else {
		// Los archivos dobles son 0,1 respectivamente
		::DragQueryFile(hDropInfo, 0, szPath, _MAX_PATH);
		m_pFrmWnd->InitCmdSub(0, szPath);
		::DragQueryFile(hDropInfo, 1, szPath, _MAX_PATH);
		m_pFrmWnd->InitCmdSub(1, szPath);
	}

	// 処理終了
	::DragFinish(hDropInfo);

	// Los archivos dobles se restablecen
	if (nFiles > 1) {
		m_pFrmWnd->PostMessage(WM_COMMAND, IDM_RESET, 0);
	}
}

//---------------------------------------------------------------------------
//
//	Rueda del raton
//
//---------------------------------------------------------------------------
BOOL CDrawView::OnMouseWheel(UINT /*nFlags*/, short zDelta, CPoint /*pt*/)
{
	CConfig *pConfig;

	// Obtenga la configuracion
	pConfig = m_pFrmWnd->GetConfig();

	// Bloquear la maquina virtual
	::LockVM();

	// Cambia segun la orientacion del eje Z
	if (zDelta > 0) {
		// Orientacion hacia atras - ampliar
		Stretch(TRUE);
		pConfig->SetStretch(TRUE);
	}
	else {
		// De frente - no ampliada
		Stretch(FALSE);
		pConfig->SetStretch(FALSE);
	}

	// Desbloqueo VM
	::UnlockVM();

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	Pulsar la tecla
//
//---------------------------------------------------------------------------
void CDrawView::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	// Determine que claves quiere excluir
	if (!KeyUpDown(nChar, nFlags, TRUE)) {
		return;
	}

	// Flujo a la clase basica
	CView::OnKeyDown(nChar, nRepCnt, nFlags);
}

//---------------------------------------------------------------------------
//
//	Tecla del sistema pulsada
//
//---------------------------------------------------------------------------
void CDrawView::OnSysKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	// Determine que claves quiere excluir
	if (!KeyUpDown(nChar, nFlags, TRUE)) {
		return;
	}

	// Flujo a la clase basica
	CView::OnSysKeyDown(nChar, nRepCnt, nFlags);
}

//---------------------------------------------------------------------------
//
//	Tecla liberada.
//
//---------------------------------------------------------------------------
void CDrawView::OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	// 除外したいキーを判別
	if (!KeyUpDown(nChar, nFlags, FALSE)) {
		return;
	}

	// 基本クラスへ流す
	CView::OnKeyUp(nChar, nRepCnt, nFlags);
}

//---------------------------------------------------------------------------
//
//	Tecla del sistema liberada.
//
//---------------------------------------------------------------------------
void CDrawView::OnSysKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	// Determine que claves quiere excluir
	if (!KeyUpDown(nChar, nFlags, FALSE)) {
		return;
	}

	// Flujo a la clase basica
	CView::OnSysKeyUp(nChar, nRepCnt, nFlags);
}

//---------------------------------------------------------------------------
//
//	Identificacion de teclas
//
//---------------------------------------------------------------------------
BOOL FASTCALL CDrawView::KeyUpDown(UINT nChar, UINT nFlags, BOOL bDown)
{
#if defined(INPUT_MOUSE) && defined(INPUT_KEYBOARD) && defined(INPUT_HARDWARE)
	INPUT input;

	ASSERT(this);
	ASSERT(nChar < 0x100);

	// スケジューラを取得
	if (!m_pScheduler) {
		m_pScheduler = m_pFrmWnd->GetScheduler();
		if (!m_pScheduler) {
			// スケジューラが存在しないので、除外しない
			return TRUE;
		}
	}

	// インプットを取得
	if (!m_pInput) {
		m_pInput = m_pFrmWnd->GetInput();
		if (!m_pScheduler) {
			// インプットが存在しないので、除外しない
			return TRUE;
		}
	}

	// スケジューラが停止していれば、除外しない
	if (!m_pScheduler->IsEnable()) {
		return TRUE;
	}

	// インプットが非アクティブ又はメニュー中なら、除外しない
	if (!m_pInput->IsActive()) {
		return TRUE;
	}
	if (m_pInput->IsMenu()) {
		return TRUE;
	}

	// キーの判別
	switch (nChar) {
		// F10
		case VK_F10:
			if (m_pInput->IsKeyMapped(DIK_F10)) {
				// マップされている
				return FALSE;
			}
			// マップされていない
			return TRUE;

		// 左ALT
		case VK_LMENU:
			if (m_pInput->IsKeyMapped(DIK_LMENU)) {
				if (bDown) {
					// 他のキーを割り込ませる
					memset(&input, 0, sizeof(input));
					input.type = INPUT_KEYBOARD;
					input.ki.wVk = VK_SHIFT;
					::SendInput(1, &input, sizeof(INPUT));
					input.type = INPUT_KEYBOARD;
					input.ki.wVk = VK_SHIFT;
					input.ki.dwFlags = KEYEVENTF_KEYUP;
					::SendInput(1, &input, sizeof(INPUT));
				}

				// マップされている
				return FALSE;
			}
			// マップされていない
			return TRUE;

		// 右ALT
		case VK_RMENU:
			if (m_pInput->IsKeyMapped(DIK_RMENU)) {
				// マップされている
				return FALSE;
			}
			// マップされていない
			return TRUE;

		// 共通ALT
		case VK_MENU:
			if (m_pInput->IsKeyMapped(DIK_LMENU) || m_pInput->IsKeyMapped(DIK_RMENU)) {
				// マップされている
				return FALSE;
			}
			// マップされていない
			return TRUE;

		// 左Windows
		case VK_LWIN:
			if (m_pInput->IsKeyMapped(DIK_LWIN)) {
				// マップされている
				if (bDown) {
					memset(&input, 0, sizeof(input));
					input.type = INPUT_KEYBOARD;
					input.ki.wVk = VK_SHIFT;
					::SendInput(1, &input, sizeof(INPUT));
					input.type = INPUT_KEYBOARD;
					input.ki.wVk = VK_SHIFT;
					input.ki.dwFlags = KEYEVENTF_KEYUP;
					::SendInput(1, &input, sizeof(INPUT));
				}
				return FALSE;
			}
			// マップされていない
			return TRUE;

		// 右Windows
		case VK_RWIN:
			if (m_pInput->IsKeyMapped(DIK_RWIN)) {
				// マップされている
				if (bDown) {
					// Windowsキーをブロックするのではなく、他のキーを割り込ませる
					memset(&input, 0, sizeof(input));
					input.type = INPUT_KEYBOARD;
					input.ki.wVk = VK_SHIFT;
					::SendInput(1, &input, sizeof(INPUT));
					input.type = INPUT_KEYBOARD;
					input.ki.wVk = VK_SHIFT;
					input.ki.dwFlags = KEYEVENTF_KEYUP;
					::SendInput(1, &input, sizeof(INPUT));
				}
				return FALSE;
			}
			// マップされていない
			return TRUE;

		// その他
		default:
			// ALTつきのキーか
			if (nFlags & 0x2000) {
				// 左ALTまたは右ALTがマップされているか
				if (m_pInput->IsKeyMapped(DIK_LMENU) || m_pInput->IsKeyMapped(DIK_RMENU)) {
					// どちらかのALTがマップされていれば、ALT+キーを無効化する
					return FALSE;
				}
			}
			break;
	}
#endif	// INPUT_MOUSE && INPUT_KEYBOARD && INPUT_HARDWARE

	// それ以外は、許可
	return TRUE;
}

//---------------------------------------------------------------------------
//
//	Movimiento de las ventanas
//
//---------------------------------------------------------------------------
void CDrawView::OnMove(int x, int y)
{
	ASSERT(m_pFrmWnd);

	// Clase basica
	CView::OnMove(x, y);

	// Pide que se cambie la posicion de la ventana del marco
	m_pFrmWnd->RecalcStatusView();
}

//---------------------------------------------------------------------------
//
//	Preparacion del mapa de bits
//
//---------------------------------------------------------------------------
void FASTCALL CDrawView::SetupBitmap()
{
	CClientDC *pDC;
	BITMAPINFOHEADER *p;
	CRect rect;

	// Si hay un mapa de bits, liberalo una vez
	if (m_Info.hBitmap) {
		if (m_Info.pRender) {
			m_Info.pRender->SetMixBuf(NULL, 0, 0);
		}
		::DeleteObject(m_Info.hBitmap);
		m_Info.hBitmap = NULL;
		m_Info.pBits = NULL;
	}

	// Tratamiento especial para la minimizacion
	GetClientRect(&rect);
	if ((rect.Width() == 0) || (rect.Height() == 0)) {
		return;
	}
	

	// Asignacion de memoria para las cabeceras de los mapas de bits
	p = (BITMAPINFOHEADER*) new BYTE[sizeof(BITMAPINFOHEADER) + sizeof(RGBQUAD)];
	memset(p, 0, sizeof(BITMAPINFOHEADER) + sizeof(RGBQUAD));

	// Creacion de informacion de mapa de bits
	m_Info.nBMPWidth = rect.Width();

	/* ACA SE ESTABLECE LA ALTURA DEL BITMAP A LEER */
	m_Info.nBMPHeight = (rect.Height() < 512) ? 512 : rect.Height();
	p->biSize = sizeof(BITMAPINFOHEADER);
	p->biWidth = m_Info.nBMPWidth;
	p->biHeight = -m_Info.nBMPHeight;
	p->biPlanes = 1;
	p->biBitCount = 32;
	p->biCompression = BI_RGB;
	p->biSizeImage = m_Info.nBMPWidth * m_Info.nBMPHeight * (32 >> 3);

	// Adquisicion de DC, creacion de la seccion DIB
	pDC = new CClientDC(this);
	m_Info.hBitmap = ::CreateDIBSection(pDC->m_hDC, (BITMAPINFO*)p, DIB_RGB_COLORS,
								(void**)&(m_Info.pBits), NULL, 0);
	// Si tiene exito, indique al renderizador
	if (m_Info.hBitmap && m_Info.pRender) {
		m_Info.pRender->SetMixBuf(m_Info.pBits, m_Info.nBMPWidth, m_Info.nBMPHeight);
	}
	delete pDC;
	delete[] p;

	// 再計算
	m_Info.nRendHMul = -1;
	m_Info.nRendVMul = -1;
	ReCalc(rect);
}

//---------------------------------------------------------------------------
//
//	Control de funcionamiento
//
//---------------------------------------------------------------------------
void FASTCALL CDrawView::Enable(BOOL bEnable)
{
	CSubWnd *pWnd;

	// Memoria de la bandera
	m_bEnable = bEnable;

	// Memoria del renderizador si esta activada
	if (m_bEnable) {
		if (!m_Info.pRender) {
			m_Info.pRender = (Render*)::GetVM()->SearchDevice(MAKEID('R', 'E', 'N', 'D'));
			ASSERT(m_Info.pRender);
			m_Info.pWork = m_Info.pRender->GetWorkAddr();
			ASSERT(m_Info.pWork);
			if (m_Info.pBits) {
				m_Info.pRender->SetMixBuf(m_Info.pBits, m_Info.nBMPWidth, m_Info.nBMPHeight);
			}
		}
	}

	// Instrucciones para las subventanas
	pWnd = m_pSubWnd;
	while (pWnd) {
		pWnd->Enable(bEnable);
		pWnd = pWnd->m_pNextWnd;
	}
}

//---------------------------------------------------------------------------
//
//	Adquisicion de banderas de explotacion
//
//---------------------------------------------------------------------------
BOOL FASTCALL CDrawView::IsEnable() const
{
	return m_bEnable;
}

//---------------------------------------------------------------------------
//
//	Refresco de dibujo
//
//---------------------------------------------------------------------------
void FASTCALL CDrawView::Refresh()
{
	CSubWnd *pWnd;
	CClientDC dc(this);

	// Redibujar la vista de dibujo
	OnDraw(&dc);

	// Volver a dibujar la subventana
	pWnd = m_pSubWnd;
	while (pWnd) {
		pWnd->Refresh();

		// Siguiente subventana
		pWnd = pWnd->m_pNextWnd;
	}
}

//---------------------------------------------------------------------------
//
//	Dibujo (del programador)
//
//---------------------------------------------------------------------------
void FASTCALL CDrawView::Draw(int nChildWnd)
{
	CSubWnd *pSubWnd;
	CClientDC *pDC;

	ASSERT(nChildWnd >= -1);

	// -1 es la vista Draw
	if (nChildWnd < 0) {
		pDC = new CClientDC(this);
		OnDraw(pDC);
		delete pDC;
		return;
	}

	// Subventana despues de 0
	pSubWnd = m_pSubWnd;

	while (nChildWnd > 0) {
		// Siguiente subventana
		pSubWnd = pSubWnd->m_pNextWnd;
		ASSERT(pSubWnd);
		nChildWnd--;
	}

	// Refresco
	pSubWnd->Refresh();
}

//---------------------------------------------------------------------------
//
//	Actualizacion del hilo de mensajes
//
//---------------------------------------------------------------------------
void FASTCALL CDrawView::Update()
{
	CSubWnd *pWnd;

	// Instrucciones para las subventanas
	pWnd = m_pSubWnd;
	while (pWnd) {
		pWnd->Update();
		pWnd = pWnd->m_pNextWnd;
	}
}

//---------------------------------------------------------------------------
//
//	Aplicar ajustes
//
//---------------------------------------------------------------------------
void FASTCALL CDrawView::ApplyCfg(const Config *pConfig)
{
	CSubWnd *pWnd;

	ASSERT(pConfig);

	// Estiramiento
	Stretch(pConfig->aspect_stretch);

	// Instrucciones para las subventanas
	pWnd = m_pSubWnd;
	while (pWnd) {
		pWnd->ApplyCfg(pConfig);
		pWnd = pWnd->m_pNextWnd;
	}
}

//---------------------------------------------------------------------------
//
//	Adquisicion de informacion sobre el dibujo
//
//---------------------------------------------------------------------------
void FASTCALL CDrawView::GetDrawInfo(LPDRAWINFO pDrawInfo) const
{
	ASSERT(this);
	ASSERT(pDrawInfo);

	// Copiar el trabajo interno
	*pDrawInfo = m_Info;
}

//---------------------------------------------------------------------------
//
//	Dibujo
//
//---------------------------------------------------------------------------
void CDrawView::OnDraw(CDC *pDC)
{
	CRect rect;
	HDC hMemDC;
	HBITMAP hDefBitmap;
	int i;
	int vmul;
	int hmul;

	// Rellenar si el mapa de bits no esta listo
	GetClientRect(&rect);
	if (!m_Info.hBitmap || !m_bEnable || !m_Info.pWork) {
		pDC->FillSolidRect(&rect, RGB(0, 0, 0));
		return;
	}

	// Recalculo
	ReCalc(rect);

	// Medidas de desconexion
	if (::GetVM()->IsPower() != m_Info.bPower) {
		m_Info.bPower = ::GetVM()->IsPower();
		if (!m_Info.bPower) {
			// Borrar todos los mapas de bits
			memset(m_Info.pBits, 0, m_Info.nBMPWidth * m_Info.nBMPHeight * 4);
			m_Info.bBltAll = TRUE;
		}
	}

	// Dibujar una esquina
	if (m_Info.bBltAll) {
		DrawRect(pDC);
	}

	// Fijar la ampliacion de la pantalla final
	hmul = 1;
	if (m_Info.nRendHMul == 2) {
		// Res 256, etc.
		hmul = 2;
	}


	/* ACA SE ESTABLECE STRETCH HORIZONTAL */
	if ( m_Info.bBltStretch) {	// si se requiere una mejora de la declaracion
		// Otros que no sean 768 x 512, el mismo modo de aspecto especificado
		int numeroDeMultiplicador = 4;


		/* Mi  Codigo de prueba para calcular stretch  maximo posible con respecto al anfitrion */
		int mihmul = hmul;				
		int anchoCalculado = 0;
		while (anchoCalculado <= rect.Width())
		{						
			numeroDeMultiplicador++;
			mihmul = hmul * numeroDeMultiplicador;			
			anchoCalculado = (m_Info.nRendWidth * mihmul) >> 2;						
		}
		numeroDeMultiplicador--;
		

		/*CString sz;	
		sz.Format(_T("numeroDeMultiplicador: %d   \r\n"),  numeroDeMultiplicador);	
		OutputDebugStringW(CT2W(sz));		*/
		
		hmul *= numeroDeMultiplicador;

	}
	else {
		// La relacion de aspecto no es identica. Igualdad de aumentos
		hmul <<= 2;
	}

	/* ACA SE ESTABLECE STRETCH VERTICAL   */
	vmul = 4;
	

	/* Mi  Codigo de prueba para calcular stretch vertical m痊imo posible con respecto al anfitrion */
		if (m_Info.bBltStretch) 
		{
			vmul = 1;							
			int mivmul = vmul;
			int altoCalculado = 0;
			while (altoCalculado < rect.Height()) // Ir aumentando multiplicadores hasta que alcance resolucion de pantalla
			{																	
				mivmul++;
				altoCalculado = (m_Info.nRendHeight * mivmul) >> 2;		
			}
			if (altoCalculado - rect.Height() > 16 )  // aunque falten 16 pix calzar 256 para 240p
				mivmul--;
			vmul = mivmul;	

		/*	CString sv;	
		    sv.Format(_T("altoCalculado: %d  rendheight:%d  mivmul: %d   m_Info.nRendVMul: %d  rect.Height(): %d \r\n"),  altoCalculado, m_Info.nRendHeight, mivmul, m_Info.nRendVMul, rect.Height());	
			OutputDebugStringW(CT2W(sv));   */
		}

	
        



	// En el caso de bBltAll, se determina toda el area
	if (m_Info.bBltAll) {
		// Creacion y seleccion de la memoria DC
		hMemDC = CreateCompatibleDC(pDC->m_hDC);
		if (!hMemDC) {
			return;
		}
		hDefBitmap = (HBITMAP)SelectObject(hMemDC, m_Info.hBitmap);
		if (!hDefBitmap) {
			DeleteDC(hMemDC);
			return;
		}

		// Blt
		if ((hmul == 4) && (vmul == 4)) {
			::BitBlt(pDC->m_hDC,
				m_Info.nLeft, m_Info.nTop,
				m_Info.nWidth, m_Info.nHeight,
				hMemDC, 0, 0,
				SRCCOPY);
		}
		else {
			::StretchBlt(pDC->m_hDC,
				m_Info.nLeft, m_Info.nTop,
				(m_Info.nWidth * hmul) >> 2,
				(m_Info.nHeight * vmul) >> 2,
				hMemDC, 0, 0,
				m_Info.nWidth, m_Info.nHeight,
				SRCCOPY);
		}
		::GdiFlush();
		m_Info.bBltAll = FALSE;

		// Volver al mapa de bits
		SelectObject(hMemDC, hDefBitmap);
		DeleteDC(hMemDC);

		// !No olvides bajar la bandera del dibujo!
		for (i=0; i<m_Info.nHeight * 64; i++) {
			m_Info.pWork->drawflag[i] = FALSE;
		}
		m_Info.dwDrawCount++;
		m_Info.nBltLeft = 0;
		m_Info.nBltTop = 0;
		m_Info.nBltRight = m_Info.nWidth - 1;
		m_Info.nBltBottom = m_Info.nHeight - 1;
		return;
	}

	// Comprobacion del area de dibujo
	if (!CalcRect()) {
		return;
	}
	ASSERT(m_Info.nBltTop <= m_Info.nBltBottom);
	ASSERT(m_Info.nBltLeft <= m_Info.nBltRight);

	// Creacion y seleccion de la memoria DC
	hMemDC = CreateCompatibleDC(pDC->m_hDC);
	if (!hMemDC) {
		m_Info.bBltAll = TRUE;
		return;
	}

	hDefBitmap = (HBITMAP)SelectObject(hMemDC, m_Info.hBitmap);
	if (!hDefBitmap) {
		DeleteDC(hMemDC);
		m_Info.bBltAll = TRUE;
		return;
	}

	// Solo se dibujan algunas zonas
	if ((hmul == 4) && (vmul == 4)) {
		::BitBlt(pDC->m_hDC,
			m_Info.nLeft + m_Info.nBltLeft,
			m_Info.nTop + m_Info.nBltTop,
			m_Info.nBltRight - m_Info.nBltLeft + 1,
			m_Info.nBltBottom - m_Info.nBltTop + 1,
			hMemDC,
			m_Info.nBltLeft,
			m_Info.nBltTop,
			SRCCOPY);
	}
	else {
		::StretchBlt(pDC->m_hDC,
			m_Info.nLeft + ((m_Info.nBltLeft * hmul) >> 2),
			m_Info.nTop + ((m_Info.nBltTop * vmul) >> 2),
			((m_Info.nBltRight - m_Info.nBltLeft + 1) * hmul) >> 2,
			((m_Info.nBltBottom - m_Info.nBltTop + 1) * vmul) >> 2,
			hMemDC,
			m_Info.nBltLeft,
			m_Info.nBltTop,
			m_Info.nBltRight - m_Info.nBltLeft + 1,
			m_Info.nBltBottom - m_Info.nBltTop + 1,
			SRCCOPY);
	}
	::GdiFlush();

	// Volver al mapa de bits
	SelectObject(hMemDC, hDefBitmap);
	DeleteDC(hMemDC);

	// La bandera es bajada por CalcRect.
	m_Info.dwDrawCount++;
}

//---------------------------------------------------------------------------
//
//	Recalculo
//
//---------------------------------------------------------------------------
void FASTCALL CDrawView::ReCalc(CRect& rect)
{
	int width;
	int height;
	BOOL flag;

	// Trabajo del renderizador, devuelve si no hay mapa de bits disponible
	if (!m_Info.pWork || !m_Info.hBitmap) {
		return;
	}

	// Comparacion
	flag = FALSE;
	if (m_Info.nRendWidth != m_Info.pWork->width) {
		m_Info.nRendWidth = m_Info.pWork->width;
		flag = TRUE;
	}
	if (m_Info.nRendHeight != m_Info.pWork->height) {
		m_Info.nRendHeight = m_Info.pWork->height;
		flag = TRUE;
	}
	if (m_Info.nRendHMul != m_Info.pWork->h_mul) {
		m_Info.nRendHMul = m_Info.pWork->h_mul;
		flag = TRUE;
	}
	if (m_Info.nRendVMul != m_Info.pWork->v_mul) {
		m_Info.nRendVMul = m_Info.pWork->v_mul;
		flag = TRUE;
	}
	if (!flag) {
		return;
	}

	// Renderizador, toma el menor de los dos mapas de bits
	m_Info.nWidth = m_Info.nRendWidth;
	if (m_Info.nBMPWidth < m_Info.nWidth) {
		m_Info.nWidth = m_Info.nBMPWidth;
	}
	m_Info.nHeight = m_Info.nRendHeight;
	if (m_Info.nRendVMul == 0) {
		// Procesamiento para el entrelazado de 15k
		m_Info.nHeight <<= 1;
	}
	if (m_Info.nBMPHeight < m_Info.nRendHeight) {
		m_Info.nHeight = m_Info.nBMPHeight;
	}

	// Centrado y margenes calculados teniendo en cuenta el aumento
	width = m_Info.nWidth * m_Info.nRendHMul;
	if ((m_Info.nRendWidth < 600) && m_Info.bBltStretch) {	// si necesita estirar
		width = (width * 5) >> 2;  // Proporcion 5:4
	}
	height = m_Info.nHeight;
	if (m_Info.nRendVMul == 2) {
		height <<= 1;
	}

	int cx = 0, cy = 0;
	int bordeAncho = 0, bordeAlto = 0;

	if (m_pFrmWnd->m_bFullScreen)
	{
		cx = ::GetSystemMetrics(SM_CXSCREEN);
		cy = ::GetSystemMetrics(SM_CYSCREEN);
	}
	
	if (m_Info.nRendWidth > 256)
	{
		if (cx > m_Info.nRendWidth)
			bordeAncho = cx % m_Info.nRendWidth;
	}
	if (m_Info.nRendHeight < 240)
	{
		if (cy > m_Info.nRendHeight)
			bordeAlto = cy % m_Info.nRendHeight;
	}


	/*CString sz, sz2, sz3, sz4;		
	sz.Format(_T("nRendHmul: %d   nRendVMul: %d \r\n"),  m_Info.nRendHMul,  m_Info.nRendVMul);	
	sz2.Format(_T("width: %d   height: %d   bordeAncho: %d  bordeAlto: %d  \r\n "),  width,  height, bordeAncho, bordeAlto);	
	sz3.Format(_T("nWidth: %d   nHeight: %d   nRendWidth: %d   nRendHeight: %d \r\n"),  m_Info.nWidth,  m_Info.nHeight, m_Info.nRendWidth,  m_Info.nRendHeight);	
	sz4.Format(_T("rect.Width(): %d   rect.Height(): %d  cx:%d cy:%d\r\n\r\n\r\n"),  rect.Width(),  rect.Height(),cx,cy );		
	OutputDebugStringW(CT2W(sz));	
	OutputDebugStringW(CT2W(sz2));
	OutputDebugStringW(CT2W(sz3));
	OutputDebugStringW(CT2W(sz4));*/
	    
	
	/* ACA SE DETERMINAN LAS ESQUINAS SUPERIORES IZQ Y TOP DEL FRAME PRINCIPAL */ 
	
		if (m_Info.bBltStretch) 		
			m_Info.nLeft = (bordeAncho > 0) ? (bordeAncho >> 2) : bordeAncho;
		else 
		    m_Info.nLeft = (rect.Width() - width) >> 1;
	
	
		if (m_Info.bBltStretch) 	
			m_Info.nTop = (bordeAlto > 0) ? (bordeAlto >> 2) : bordeAlto; 
		else
			m_Info.nTop = (rect.Height() - height) >> 1;
	


	// Especifique el dibujo de la zona
	m_Info.bBltAll = TRUE;
}

//---------------------------------------------------------------------------
//
//	Modo de aumento
//
//---------------------------------------------------------------------------
void FASTCALL CDrawView::Stretch(BOOL bStretch)
{
	CRect rect;

	ASSERT(this);

	/*char cadena[20];	  
    sprintf(cadena, "%d", bStretch);
	 int msgboxID = MessageBox(
       cadena,"Stretch",
        2 );*/

	// Si es una coincidencia, no se hara nada.
	if (bStretch == m_Info.bBltStretch) {
		return;
	}
	m_Info.bBltStretch = bStretch;

	// Recalcular si no es 768 x 512
	if ((m_Info.nRendWidth > 0) && (m_Info.nRendWidth < 600)) {		// si se requiere una mejora de la declaracion
		m_Info.nRendWidth = m_Info.pWork->width + 1;
		GetClientRect(&rect);
		ReCalc(rect);
	}
}

//---------------------------------------------------------------------------
//
//	Dibujar rect疣gulos de bordes en las esquinas
//
//---------------------------------------------------------------------------
void FASTCALL CDrawView::DrawRect(CDC *pDC)
{
	CRect crect;
	CRect brect;

	ASSERT(m_Info.bBltAll);
	ASSERT(pDC);

	// Si los usas todos, no los necesitas.
	if ((m_Info.nLeft == 0) && (m_Info.nTop == 0)) {
		return;
	}

	// Obtener el rectangulo del cliente
	GetClientRect(&crect);


	/* ACA SE ESTABLECEN LOS BORDES EXTERIORES DE VENTANA DE JUEGO Y SUS COLORES */
	if (m_Info.nLeft > 0) {
		// Mitad izquierda
		brect.left = 0;
		brect.top = 0;
		brect.right = m_Info.nLeft;
		brect.bottom = crect.bottom;
		pDC->FillSolidRect(&brect, RGB(0, 0, 0));

		// La mitad derecha
		brect.right = crect.right;
		brect.left = brect.right - m_Info.nLeft - 1;
		pDC->FillSolidRect(&brect, RGB(0, 0, 0));
	}

	if (m_Info.nTop > 0) {
		// La mitad superior
		brect.left = 0;
		brect.top = 0;
		brect.right = crect.right;
		brect.bottom = m_Info.nTop;
		pDC->FillSolidRect(&brect, RGB(0, 0, 0));

		// La mitad derecha
		brect.bottom = crect.bottom;
		brect.top = brect.bottom - m_Info.nTop - 1;
		pDC->FillSolidRect(&brect, RGB(0, 0, 0));
	}
}

//---------------------------------------------------------------------------
//
//	Comprobacion del rango de dibujo
//
//---------------------------------------------------------------------------
BOOL FASTCALL CDrawView::CalcRect()
{
	int i;
	int j;
	int left;
	int top;
	int right;
	int bottom;
	BOOL *p;
	BOOL flag;

	// Inicializacion
	left = 64;
	top = 2048;
	right = -1;
	bottom = -1;
	p = m_Info.pWork->drawflag;

	// y bucle
	for (i=0; i<m_Info.nHeight; i++) {
		flag = FALSE;

		// x bucle
		for(j=0; j<64; j++) {
			if (*p) {
				// Borrar
				*p = FALSE;

				// Este 16dot necesita ser dibujado
				if (left > j) {
					left = j;
				}
				if (right < j) {
					right = j;
				}
				flag = TRUE;
			}
			p++;
		}

		if (flag) {
			// Es necesario trazar esta linea
			if (top > i) {
				top = i;
			}
			if (bottom < i) {
				bottom = i;
			}
		}
	}

	// No es necesario si y no cambia
	if (bottom < top) {
		return FALSE;
	}

	// Correccion (x16)
	left <<= 4;
	right = ((right + 1) << 4) - 1;
	if (right >= m_Info.nWidth) {
		right = m_Info.nWidth - 1;
	}

	// Copiar
	m_Info.nBltLeft = left;
	m_Info.nBltTop = top;
	m_Info.nBltRight = right;
	m_Info.nBltBottom = bottom;

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	Obtener el nuevo indice de la subventana
//
//---------------------------------------------------------------------------
int FASTCALL CDrawView::GetNewSWnd() const
{
	CSubWnd *pWnd;
	int nSubWnd;

	ASSERT(this);
	ASSERT_VALID(this);

	// 最初のサブウィンドウか
	if (!m_pSubWnd) {
		return 0;
	}

	// 初期化
	nSubWnd = 1;
	pWnd = m_pSubWnd;

	// ループ
	while (pWnd->m_pNextWnd) {
		pWnd = pWnd->m_pNextWnd;
		nSubWnd++;
	}

	// インデックスを返す
	return nSubWnd;
}

//---------------------------------------------------------------------------
//
//	Anadir subventana
//	Llamada desde el CSubWnd que se quiere anadir
//
//---------------------------------------------------------------------------
void FASTCALL CDrawView::AddSWnd(CSubWnd *pSubWnd)
{
	CSubWnd *pWnd;

	ASSERT(this);
	ASSERT(pSubWnd);
	ASSERT_VALID(this);

	// 最初のサブウィンドウか
	if (!m_pSubWnd) {
		// このウィンドウが最初。登録する
		m_pSubWnd = pSubWnd;
		ASSERT(!pSubWnd->m_pNextWnd);
		return;
	}

	// 終端を探す
	pWnd = m_pSubWnd;
	while (pWnd->m_pNextWnd) {
		pWnd = pWnd->m_pNextWnd;
	}

	// pWndの後ろに追加
	pWnd->m_pNextWnd = pSubWnd;
	ASSERT(!pSubWnd->m_pNextWnd);
}

//---------------------------------------------------------------------------
//
//	Eliminar la subventana
//	Llama desde el CSubWnd que quieres eliminar
//
//---------------------------------------------------------------------------
void FASTCALL CDrawView::DelSWnd(CSubWnd *pSubWnd)
{
	CSubWnd *pWnd;

	// assert
	ASSERT(pSubWnd);

	// VMをロック
	::LockVM();

	// 最初のサブウィンドウか
	if (m_pSubWnd == pSubWnd) {
		// 次があるなら、次を登録。なければNULL
		if (pSubWnd->m_pNextWnd) {
			m_pSubWnd = pSubWnd->m_pNextWnd;
		}
		else {
			m_pSubWnd = NULL;
		}
		::UnlockVM();
		return;
	}

	// pSubWndを記憶しているサブウィンドウを探す
	pWnd = m_pSubWnd;
	while (pWnd->m_pNextWnd != pSubWnd) {
		ASSERT(pWnd->m_pNextWnd);
		pWnd = pWnd->m_pNextWnd;
	}

	// pSubWnd->m_pNextWndを、pWndに結びつけスキップさせる
	pWnd->m_pNextWnd = pSubWnd->m_pNextWnd;

	// VMをアンロック
	::UnlockVM();
}

//---------------------------------------------------------------------------
//
//	Eliminar todas las subventanas
//
//---------------------------------------------------------------------------
void FASTCALL CDrawView::ClrSWnd()
{
	CSubWnd *pWnd;
    CSubWnd *pNext;

	ASSERT(this);

	// 最初のサブウィンドウを取得
	pWnd = GetFirstSWnd();

	// ループ
	while (pWnd) {
		// 次を取得
		pNext = pWnd->m_pNextWnd;

		// このウィンドウを削除
		pWnd->DestroyWindow();

		// 移動
		pWnd = pNext;
	}
}

//---------------------------------------------------------------------------
//
//	Obtener la primera subventana
//	Si no, devuelve NULL
//
//---------------------------------------------------------------------------
CSubWnd* FASTCALL CDrawView::GetFirstSWnd() const
{
	return m_pSubWnd;
}

//---------------------------------------------------------------------------
//
//	サブウィンドウを検索
//	※見つからなければNULLを返す
//
//---------------------------------------------------------------------------
CSubWnd* FASTCALL CDrawView::SearchSWnd(DWORD dwID) const
{
	CSubWnd *pWnd;

	// ウィンドウを初期化
	pWnd = m_pSubWnd;

	// 検索ループ
	while (pWnd) {
		// IDが一致するかチェック
		if (pWnd->GetID() == dwID) {
			return pWnd;
		}

		// 次へ
		pWnd = pWnd->m_pNextWnd;
	}

	// 見つからなかった
	return NULL;
}

//---------------------------------------------------------------------------
//
//	Obtener la fuente del texto
//
//---------------------------------------------------------------------------
CFont* FASTCALL CDrawView::GetTextFont()
{
	ASSERT(m_TextFont.m_hObject);

	return &m_TextFont;
}

//---------------------------------------------------------------------------
//
//	Crear una nueva ventana
//
//---------------------------------------------------------------------------
CSubWnd* FASTCALL CDrawView::NewWindow(BOOL bDis)
{
	DWORD dwID;
	int i;
	CSubWnd *pWnd;
	CDisasmWnd *pDisWnd;
	CMemoryWnd *pMemWnd;

	// 基準ID作成
	if (bDis) {
		dwID = MAKEID('D', 'I', 'S', 'A');
	}
	else {
		dwID = MAKEID('M', 'E', 'M', 'A');
	}

	// 8回ループ
	for (i=0; i<8; i++) {
	
		pWnd = SearchSWnd(dwID);
		if (!pWnd) {
			if (bDis) {
				pDisWnd = new CDisasmWnd(i);
				VERIFY(pDisWnd->Init(this));
				return pDisWnd;
			}
			else {
				pMemWnd = new CMemoryWnd(i);
				VERIFY(pMemWnd->Init(this));
				return pMemWnd;
			}
		}

		// 次のウィンドウIDをつくる
		dwID++;
	}

	// 作成できなかった
	return NULL;
}

//---------------------------------------------------------------------------
//
//	Comprobar si se puede crear una nueva ventana
//
//---------------------------------------------------------------------------
BOOL FASTCALL CDrawView::IsNewWindow(BOOL bDis)
{
	DWORD dwID;
	int i;
	CSubWnd *pWnd;

	// 基準ID作成
	if (bDis) {
		dwID = MAKEID('D', 'I', 'S', 'A');
	}
	else {
		dwID = MAKEID('M', 'E', 'M', 'A');
	}

	// 8回ループ
	for (i=0; i<8; i++) {
		// ウィンドウが見つからなければ、新規作成可
		pWnd = SearchSWnd(dwID);
		if (!pWnd) {
			return TRUE;
		}

		// 次のウィンドウIDをつくる
		dwID++;
	}

	// すべてのウィンドウがある→新規作成できない
	return FALSE;
}

//---------------------------------------------------------------------------
//
//	Obtener el numero de subventanas
//	Devuelve NULL si no se encuentra
//
//---------------------------------------------------------------------------
int FASTCALL CDrawView::GetSubWndNum() const
{
	CSubWnd *pWnd;
	int num;

	// 初期化
	pWnd = m_pSubWnd;
	num = 0;

	// ループ
	while (pWnd) {
		// 個数++
		num++;

		// 次へ
		pWnd = pWnd->m_pNextWnd;
	}

	return num;
}

//---------------------------------------------------------------------------
//
//	Obtener el nombre de la clase de la ventana
//
//---------------------------------------------------------------------------
LPCTSTR FASTCALL CDrawView::GetWndClassName() const
{
	ASSERT(this);
	ASSERT(m_pFrmWnd);
	return m_pFrmWnd->GetWndClassName();
}

#endif	// _WIN32
