//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2005 ‚o‚hD(ytanaka@ipc-tokai.or.jp)
//	[ MFC ƒTƒuƒEƒBƒ“ƒhƒE ]
//
//---------------------------------------------------------------------------

#if defined(_WIN32)

#include "os.h"
#include "xm6.h"
#include "render.h"
#include "mfc_frm.h"
#include "mfc_draw.h"
#include "mfc_res.h"
#include "mfc_com.h"
#include "mfc_sch.h"
#include "mfc_sub.h"

//===========================================================================
//
//	ƒTƒuƒEƒBƒ“ƒhƒE
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	ƒRƒ“ƒXƒgƒ‰ƒNƒ^
//
//---------------------------------------------------------------------------
CSubWnd::CSubWnd()
{
	// ƒIƒuƒWƒFƒNƒg
	m_pSch = NULL;
	m_pDrawView = NULL;
	m_pNextWnd = NULL;

	// ƒvƒƒpƒeƒB
	m_strCaption.Empty();
	m_bEnable = TRUE;
	m_dwID = 0;
	m_bPopup = FALSE;

	// ƒEƒBƒ“ƒhƒEƒTƒCƒY
	m_nWidth = -1;
	m_nHeight = -1;

	// ƒeƒLƒXƒgƒtƒHƒ“ƒg
	m_pTextFont = NULL;
	m_tmWidth = -1;
	m_tmHeight = -1;
}

//---------------------------------------------------------------------------
//
//	ƒƒbƒZ[ƒW ƒ}ƒbƒv
//
//---------------------------------------------------------------------------
BEGIN_MESSAGE_MAP(CSubWnd, CWnd)
	ON_WM_CREATE()
	ON_WM_DESTROY()
	ON_WM_ERASEBKGND()
	ON_WM_ACTIVATE()
END_MESSAGE_MAP()

//---------------------------------------------------------------------------
//
//	‰Šú‰»
//
//---------------------------------------------------------------------------
BOOL FASTCALL CSubWnd::Init(CDrawView *pDrawView)
{
	BOOL bRet;
	CFrmWnd *pFrmWnd;

	ASSERT(this);
	ASSERT(pDrawView);
	ASSERT(m_dwID != 0);

	// Drawƒrƒ…[‹L‰¯
	ASSERT(!m_pDrawView);
	m_pDrawView = pDrawView;
	ASSERT(m_pDrawView);

	// ƒtƒŒ[ƒ€ƒEƒBƒ“ƒhƒEæ“¾
	pFrmWnd = (CFrmWnd*)AfxGetApp()->m_pMainWnd;
	ASSERT(pFrmWnd);

	// ƒXƒPƒWƒ…[ƒ‰æ“¾
	ASSERT(!m_pSch);
	m_pSch = pFrmWnd->GetScheduler();
	ASSERT(m_pSch);

	// ƒEƒBƒ“ƒhƒEì¬
	if (pFrmWnd->IsPopupSWnd()) {
		// ƒ|ƒbƒvƒAƒbƒv
		m_bPopup = TRUE;
		bRet = CreateEx(0,
					pDrawView->GetWndClassName(),
					m_strCaption,
					WS_POPUP | WS_OVERLAPPED | WS_SYSMENU | WS_CAPTION |
					WS_VISIBLE | WS_MINIMIZEBOX | WS_BORDER,
					0, 0,
					100, 100,
					pDrawView->m_hWnd,
					(HMENU)0,
					0);
	}
	else {
		// ƒ`ƒƒƒCƒ‹ƒh
		m_bPopup = FALSE;
		bRet = Create(NULL,
					m_strCaption,
					WS_CHILD | WS_OVERLAPPED | WS_SYSMENU | WS_CAPTION |
					WS_VISIBLE | WS_MINIMIZEBOX | WS_CLIPSIBLINGS,
					CRect(0, 0, 100, 100),
					pDrawView,
					(UINT)m_dwID);
	}

	// ¬Œ÷‚·‚ê‚Î
	if (bRet) {
		// eƒEƒBƒ“ƒhƒE‚Ö“o˜^
		m_pDrawView->AddSWnd(this);
	}

	return bRet;
}

//---------------------------------------------------------------------------
//
//	ƒEƒBƒ“ƒhƒEì¬
//
//---------------------------------------------------------------------------
int CSubWnd::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	int nSWnd;
	CRect rectWnd;
	CRect rectParent;
	CPoint point;

	ASSERT(this);
	ASSERT(lpCreateStruct);
	ASSERT(m_nWidth > 0);
	ASSERT(m_nHeight > 0);

	// Šî–{ƒNƒ‰ƒX
	if (CWnd::OnCreate(lpCreateStruct) != 0) {
		return -1;
	}

	// ƒAƒCƒRƒ“İ’è
	SetIcon(AfxGetApp()->LoadIcon(IDI_XICON), TRUE);

	// IMEƒIƒt
	::ImmAssociateContext(m_hWnd, (HIMC)NULL);

	// ƒeƒLƒXƒgƒtƒHƒ“ƒgƒZƒbƒgƒAƒbƒv
	m_pTextFont = m_pDrawView->GetTextFont();
	SetupTextFont();

	// ƒEƒBƒ“ƒhƒE‚Ì“K‡ƒTƒCƒY‚ğŒvZ
	rectWnd.left = 0;
	rectWnd.top = 0;
	rectWnd.right = m_nWidth * m_tmWidth;
	rectWnd.bottom = m_nHeight * m_tmHeight;
	CalcWindowRect(&rectWnd);

	// ƒCƒ“ƒfƒbƒNƒX(—\’è)‚ğ“¾‚é
	nSWnd = m_pDrawView->GetNewSWnd();

	// ƒCƒ“ƒfƒbƒNƒX‚©‚çƒEƒBƒ“ƒhƒEˆÊ’u‚ğŒˆ’è
	m_pDrawView->GetWindowRect(&rectParent);
	point.x = (nSWnd * 24) % (rectParent.Width() - 24);
	point.y = (nSWnd * 24) % (rectParent.Height() - 24);

	if (m_bPopup) {
		// ƒ|ƒbƒvƒAƒbƒvƒ^ƒCƒv‚Ìê‡‚ÍAƒXƒNƒŠ[ƒ“À•W‚ªŠî€‚É‚È‚é
		point.x += rectParent.left;
		point.y += rectParent.top;
	}

	// ƒEƒBƒ“ƒhƒEˆÊ’uA‘å‚«‚³‚ğİ’è
	SetWindowPos(&wndTop, point.x, point.y,
				rectWnd.Width(), rectWnd.Height(), 0);

	return 0;
}

//---------------------------------------------------------------------------
//
//	ƒEƒBƒ“ƒhƒEíœ
//
//---------------------------------------------------------------------------
void CSubWnd::OnDestroy()
{
	ASSERT(this);

	// “®ì’â~
	Enable(FALSE);

	// eƒEƒBƒ“ƒhƒE‚Ö’Ê’m
	m_pDrawView->DelSWnd(this);

	// Šî–{ƒNƒ‰ƒX‚Ö
	CWnd::OnDestroy();
}

//---------------------------------------------------------------------------
//
//	ƒEƒBƒ“ƒhƒEíœŠ®—¹
//
//---------------------------------------------------------------------------
void CSubWnd::PostNcDestroy()
{
	// ƒCƒ“ƒ^ƒtƒF[ƒX—v‘f‚ğíœ
	ASSERT(this);
	delete this;
}

//---------------------------------------------------------------------------
//
//	”wŒi•`‰æ
//
//---------------------------------------------------------------------------
BOOL CSubWnd::OnEraseBkgnd(CDC *pDC)
{
	ASSERT(this);
	ASSERT_VALID(this);

	// —LŒø‚È‚çA‘S—ÌˆæÓ”C‚ğ‚Â
	if (m_bEnable) {
		return TRUE;
	}

	// Šî–{ƒNƒ‰ƒX‚Ö
	return CWnd::OnEraseBkgnd(pDC);
}

//---------------------------------------------------------------------------
//
//	ƒAƒNƒeƒBƒx[ƒg
//
//---------------------------------------------------------------------------
void CSubWnd::OnActivate(UINT nState, CWnd *pWnd, BOOL bMinimized)
{
	ASSERT(this);
	ASSERT_VALID(this);

	// ƒ|ƒbƒvƒAƒbƒvƒEƒBƒ“ƒhƒE‚Ì‚İ
	if (m_bPopup) {
		if (nState == WA_INACTIVE) {
			// ƒ|ƒbƒvƒAƒbƒvƒEƒBƒ“ƒhƒE‚ªƒCƒ“ƒAƒNƒeƒBƒu‚È‚çA’á‘¬Às
			m_pSch->Activate(FALSE);
		}
		else {
			// ƒ|ƒbƒvƒAƒbƒvƒEƒBƒ“ƒhƒE‚ªƒAƒNƒeƒBƒu‚È‚çA’ÊíÀs
			m_pSch->Activate(TRUE);
		}
	}

	// Šî–{ƒNƒ‰ƒX
	CWnd::OnActivate(nState, pWnd, bMinimized);
}

//---------------------------------------------------------------------------
//
//	“®ì§Œä
//
//---------------------------------------------------------------------------
void FASTCALL CSubWnd::Enable(BOOL bEnable)
{
	ASSERT(this);

	m_bEnable = bEnable;
}

//---------------------------------------------------------------------------
//
//	ƒEƒBƒ“ƒhƒEID‚ğæ“¾
//
//---------------------------------------------------------------------------
DWORD FASTCALL CSubWnd::GetID() const
{
	ASSERT(this);
	ASSERT_VALID(this);

	return m_dwID;
}

//---------------------------------------------------------------------------
//
//	ƒeƒLƒXƒgƒtƒHƒ“ƒgƒZƒbƒgƒAƒbƒv
//
//---------------------------------------------------------------------------
void FASTCALL CSubWnd::SetupTextFont()
{
	CClientDC dc(this);
	CFont *pFont;
	TEXTMETRIC tm;

	ASSERT(this);

	// ƒtƒHƒ“ƒgƒZƒŒƒNƒg
	ASSERT(m_pTextFont);
	pFont = dc.SelectObject(m_pTextFont);
	ASSERT(pFont);

	// ƒeƒLƒXƒgƒƒgƒŠƒbƒN‚ğæ“¾
	VERIFY(dc.GetTextMetrics(&tm));

	// ƒtƒHƒ“ƒg‚ğ–ß‚·
	VERIFY(dc.SelectObject(pFont));

	// ƒeƒLƒXƒg‰¡•Ac•‚ğŠi”[
	m_tmWidth = tm.tmAveCharWidth;
	m_tmHeight = tm.tmHeight + tm.tmExternalLeading;
}

//---------------------------------------------------------------------------
//
//	ƒƒbƒZ[ƒWƒXƒŒƒbƒh‚©‚çXV
//
//---------------------------------------------------------------------------
void FASTCALL CSubWnd::Update()
{
	ASSERT(this);
	ASSERT_VALID(this);
}

//---------------------------------------------------------------------------
//
//	ƒZ[ƒu
//
//---------------------------------------------------------------------------
BOOL FASTCALL CSubWnd::Save(Fileio* /*pFio*/, int /*nVer*/)
{
	ASSERT(this);
	ASSERT_VALID(this);

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	ƒ[ƒh
//
//---------------------------------------------------------------------------
BOOL FASTCALL CSubWnd::Load(Fileio* /*pFio*/, int /*nVer*/)
{
	ASSERT(this);
	ASSERT_VALID(this);

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	İ’è“K—p
//
//---------------------------------------------------------------------------
void FASTCALL CSubWnd::ApplyCfg(const Config* /*pConfig*/)
{
	ASSERT(this);
	ASSERT_VALID(this);
}

#if !defined(NDEBUG)
//---------------------------------------------------------------------------
//
//	f’f
//
//---------------------------------------------------------------------------
void CSubWnd::AssertValid() const
{
	ASSERT(this);

	// ‘¼‚ÌƒIƒuƒWƒFƒNƒg
	ASSERT(m_pSch);
	ASSERT(m_pSch->GetID() == MAKEID('S', 'C', 'H', 'E'));
	ASSERT(m_pDrawView);
	ASSERT(m_pDrawView->m_hWnd);
	ASSERT(::IsWindow(m_pDrawView->m_hWnd));

	// ƒvƒƒpƒeƒB
	ASSERT(m_strCaption.GetLength() > 0);
	ASSERT(m_dwID != 0);
	ASSERT(m_nWidth > 0);
	ASSERT(m_nHeight > 0);
	ASSERT(m_pTextFont);
	ASSERT(m_pTextFont->m_hObject);
	ASSERT(m_tmWidth > 0);
	ASSERT(m_tmHeight > 0);
}
#endif	// NDEBUG

//===========================================================================
//
//	ƒTƒuƒeƒLƒXƒgƒEƒBƒ“ƒhƒE
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	ƒRƒ“ƒXƒgƒ‰ƒNƒ^
//
//---------------------------------------------------------------------------
CSubTextWnd::CSubTextWnd()
{
	// ƒƒ“ƒo•Ï”‰Šú‰»
	m_bReverse = FALSE;
	m_pTextBuf = NULL;
	m_pDrawBuf = NULL;
}

//---------------------------------------------------------------------------
//
//	ƒƒbƒZ[ƒW ƒ}ƒbƒv
//
//---------------------------------------------------------------------------
BEGIN_MESSAGE_MAP(CSubTextWnd, CSubWnd)
	ON_WM_CREATE()
	ON_WM_DESTROY()
	ON_WM_PAINT()
	ON_WM_SIZE()
END_MESSAGE_MAP()

//---------------------------------------------------------------------------
//
//	ƒEƒBƒ“ƒhƒEì¬
//
//---------------------------------------------------------------------------
int CSubTextWnd::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	ASSERT(this);
	ASSERT(!m_pTextBuf);
	ASSERT(m_nWidth > 0);
	ASSERT(m_nHeight > 0);

	// ƒeƒLƒXƒgƒoƒbƒtƒ@Šm•Û
	m_pTextBuf = new BYTE[m_nWidth * m_nHeight];
	ASSERT(m_pTextBuf);

	// •`‰æƒoƒbƒtƒ@Šm•ÛA‰Šú‰»
	m_pDrawBuf = new BYTE[m_nWidth * m_nHeight];
	ASSERT(m_pDrawBuf);
	memset(m_pDrawBuf, 0xff, m_nWidth * m_nHeight);

	// Šî–{ƒNƒ‰ƒX(‚±‚±‚ÅDrawViewƒw’Ê’m)
	if (CSubWnd::OnCreate(lpCreateStruct) != 0) {
		return -1;
	}

	return 0;
}

//---------------------------------------------------------------------------
//
//	ƒEƒBƒ“ƒhƒEíœ
//
//---------------------------------------------------------------------------
void CSubTextWnd::OnDestroy()
{
	ASSERT(this);

	// Šî–{ƒNƒ‰ƒX‚ğæ‚ÉÀs‚·‚é(DrawView‚Ö‚Ì’Ê’m‚ğ‹}‚®‚½‚ß)
	CSubWnd::OnDestroy();

	// ƒeƒLƒXƒgƒoƒbƒtƒ@‰ğ•ú
	ASSERT(m_pTextBuf);
	if (m_pTextBuf) {
		delete[] m_pTextBuf;
		m_pTextBuf = NULL;
	}

	// •`‰æƒoƒbƒtƒ@‰ğ•ú
	ASSERT(m_pDrawBuf);
	if (m_pDrawBuf) {
		delete[] m_pDrawBuf;
		m_pDrawBuf = NULL;
	}
}

//---------------------------------------------------------------------------
//
//	•`‰æ
//
//---------------------------------------------------------------------------
void CSubTextWnd::OnPaint()
{
	PAINTSTRUCT ps;

	// VM‚ğƒƒbƒN
	::LockVM();

    BeginPaint(&ps);

	// ƒeƒLƒXƒgƒoƒbƒtƒ@A—LŒøƒtƒ‰ƒOƒ`ƒFƒbƒN
	if (m_pTextBuf && m_bEnable) {
		// •`‰æƒoƒbƒtƒ@‚ğFF‚Å–„‚ß‚é
		memset(m_pDrawBuf, 0xff, m_nWidth * m_nHeight);

		// •`‰æ‚Ís‚í‚È‚¢(VMƒXƒŒƒbƒh‚©‚ç‚ÌRefresh‚É”C‚¹‚é)
	}

	EndPaint(&ps);

	// VMƒAƒ“ƒƒbƒN
	::UnlockVM();
}

//---------------------------------------------------------------------------
//
//	ƒTƒCƒY•ÏX
//
//---------------------------------------------------------------------------
void CSubTextWnd::OnSize(UINT nType, int cx, int cy)
{
	int nWidth;
	int nHeight;
	CRect rectClient;

	ASSERT(this);
	ASSERT(m_tmWidth > 0);
	ASSERT(m_tmHeight > 0);

	// ƒLƒƒƒ‰ƒNƒ^“–‚½‚è‚ÌƒNƒ‰ƒCƒAƒ“ƒgƒTƒCƒY‚ğŒvZ
	GetClientRect(&rectClient);
	nWidth = rectClient.Width() / m_tmWidth;
	nHeight = rectClient.Height() / m_tmHeight;

	// ˆê’v‚µ‚Ä‚¢‚È‚­‚Ä
	if ((nWidth != m_nWidth) || (nHeight != m_nHeight)) {
		// Å¬‰»‚Å‚È‚­‚Ä
		if (nType != SIZE_MINIMIZED) {
			// ƒeƒLƒXƒgƒoƒbƒtƒ@‚ª‚ ‚ê‚Î
			if (m_pTextBuf && m_pDrawBuf) {
				// —]—T‚ª‚ ‚ê‚ÎA+1
				if ((nWidth * m_tmWidth) < rectClient.Width()) {
					nWidth++;
				}
				if ((nHeight * m_tmWidth) < rectClient.Height()) {
					nHeight++;
				}

				// ƒoƒbƒtƒ@•ÏX(VMƒƒbƒN‚ğ‹²‚Ş)
				::LockVM();
				ASSERT(m_pTextBuf);
				if (m_pTextBuf) {
					delete[] m_pTextBuf;
					m_pTextBuf = NULL;
				}
				ASSERT(m_pDrawBuf);
				if (m_pDrawBuf) {
					delete[] m_pDrawBuf;
					m_pDrawBuf = NULL;
				}
				m_pTextBuf = new BYTE[nWidth * nHeight];
				m_pDrawBuf = new BYTE[nWidth * nHeight];
				m_nWidth = nWidth;
				m_nHeight = nHeight;
				::UnlockVM();
			}
		}
	}

	// Ä•`‰æ‚ğ}‚é
	Invalidate(FALSE);

	// Šî–{ƒNƒ‰ƒX‚Ö
	CSubWnd::OnSize(nType, cx, cy);
}

//---------------------------------------------------------------------------
//
//	ƒŠƒtƒŒƒbƒVƒ…•`‰æ
//
//---------------------------------------------------------------------------
void FASTCALL CSubTextWnd::Refresh()
{
	CClientDC dc(this);

	ASSERT(this);

	// ƒeƒLƒXƒgƒoƒbƒtƒ@A—LŒøƒtƒ‰ƒOƒ`ƒFƒbƒN
	if (m_pTextBuf && m_bEnable) {
		// ƒZƒbƒgƒAƒbƒv
		Setup();

		// •`‰æ
		OnDraw(&dc);
	}
}

//---------------------------------------------------------------------------
//
//	•`‰æƒƒCƒ“
//
//---------------------------------------------------------------------------
void FASTCALL CSubTextWnd::OnDraw(CDC *pDC)
{
	CFont *pFont;
	BOOL bReverse;
	BYTE *pText;
	BYTE *pDraw;
	CPoint point;
	int x;
	int y;
	BYTE ch;

	ASSERT(this);
	ASSERT(pDC);
	ASSERT(m_pTextBuf);
	ASSERT(m_pDrawBuf);
	ASSERT(m_pTextFont);
	ASSERT(m_nWidth >= 0);
	ASSERT(m_nHeight >= 0);

	// ƒtƒHƒ“ƒgw’è
	pFont = pDC->SelectObject(m_pTextFont);
	ASSERT(pFont);

	// Fw’è
	pDC->SetTextColor(::GetSysColor(COLOR_WINDOWTEXT));
	pDC->SetBkColor(::GetSysColor(COLOR_WINDOW));
	bReverse = FALSE;

	// ƒ|ƒCƒ“ƒ^AÀ•W‰Šú‰»
	pText = m_pTextBuf;
	pDraw = m_pDrawBuf;
	point.y = 0;

	// yƒ‹[ƒv
	for (y=0; y<m_nHeight; y++) {
		point.x = 0;
		for (x=0; x<m_nWidth; x++) {
			// ˆê’vƒ`ƒFƒbƒN
			if (*pText == *pDraw) {
				pText++;
				pDraw++;
				point.x += m_tmWidth;
				continue;
			}

			// ƒRƒs[
			ch = *pText++;
			*pDraw++ = ch;

			// ”½“]ƒ`ƒFƒbƒN
			if (ch >= 0x80) {
				ch &= 0x7f;
				if (!bReverse) {
					pDC->SetTextColor(::GetSysColor(COLOR_WINDOW));
					pDC->SetBkColor(::GetSysColor(COLOR_WINDOWTEXT));
					bReverse = TRUE;
				}
			}
			else {
				if (bReverse) {
					pDC->SetTextColor(::GetSysColor(COLOR_WINDOWTEXT));
					pDC->SetBkColor(::GetSysColor(COLOR_WINDOW));
					bReverse = FALSE;
				}
			}

			// ƒeƒLƒXƒgƒAƒEƒg
			pDC->TextOut(point.x, point.y, (LPCTSTR)&ch ,1);
			point.x += m_tmWidth;
		}
		point.y += m_tmHeight;
	}

	// ƒtƒHƒ“ƒgw’è‰ğœ
	VERIFY(pDC->SelectObject(pFont));
}

//---------------------------------------------------------------------------
//
//	ƒNƒŠƒA
//
//---------------------------------------------------------------------------
void FASTCALL CSubTextWnd::Clear()
{
	ASSERT(this);
	ASSERT(m_pTextBuf);
	ASSERT(m_nWidth >= 0);
	ASSERT(m_nHeight >= 0);

	// ƒNƒŠƒA
	if (m_pTextBuf) {
		memset(m_pTextBuf, 0x20, m_nWidth * m_nHeight);
	}

	// ”½“]ƒIƒt
	Reverse(FALSE);
}

//---------------------------------------------------------------------------
//
//	•¶šƒZƒbƒg
//
//---------------------------------------------------------------------------
void FASTCALL CSubTextWnd::SetChr(int x, int y, TCHAR ch)
{
	ASSERT(this);
	ASSERT(m_pTextBuf);
	ASSERT(x >= 0);
	ASSERT(y >= 0);
	ASSERT(_istprint(ch));

	// —LŒø”ÍˆÍ‚©ƒ`ƒFƒbƒN
	if ((x < m_nWidth) && (y < m_nHeight)) {
		// ‘‚«‚İ
		if (m_bReverse) {
			m_pTextBuf[(y * m_nWidth) + x] = (BYTE)(ch | 0x80);
		}
		else {
			m_pTextBuf[(y * m_nWidth) + x] = ch;
		}
	}
}

//---------------------------------------------------------------------------
//
//	•¶š—ñƒZƒbƒg
//
//---------------------------------------------------------------------------
void FASTCALL CSubTextWnd::SetString(int x, int y, LPCTSTR lpszText)
{
	ASSERT(this);
	ASSERT(m_pTextBuf);
	ASSERT(x >= 0);
	ASSERT(y >= 0);
	ASSERT(lpszText);

	// '\0'‚Ü‚Åƒ‹[ƒv
	while (_istprint(*lpszText)) {
		SetChr(x, y, *lpszText);

		lpszText++;
		x++;
	}
}

//---------------------------------------------------------------------------
//
//	”½“]ƒZƒbƒg
//
//---------------------------------------------------------------------------
void FASTCALL CSubTextWnd::Reverse(BOOL bReverse)
{
	ASSERT(this);
	ASSERT(m_pTextBuf);

	m_bReverse = bReverse;
}

//---------------------------------------------------------------------------
//
//	ƒŠƒTƒCƒY
//
//---------------------------------------------------------------------------
void FASTCALL CSubTextWnd::ReSize(int nWidth, int nHeight)
{
	CRect rectWnd;
	CRect rectNow;

	ASSERT(this);
	ASSERT(nWidth > 0);
	ASSERT(nHeight > 0);

	// ˆê’vƒ`ƒFƒbƒN
	if ((nWidth == m_nWidth) && (nHeight == m_nHeight)) {
		return;
	}

	// ƒoƒbƒtƒ@‚ª‚È‚¯‚ê‚Î‰½‚à‚µ‚È‚¢
	if (!m_pTextBuf || !m_pDrawBuf) {
		return;
	}

	// ƒTƒCƒY‚ğZo
	rectWnd.left = 0;
	rectWnd.top = 0;
	rectWnd.right = nWidth * m_tmWidth;
	rectWnd.bottom = nHeight * m_tmHeight;
	CalcWindowRect(&rectWnd);

	// Œ»İ‚Æ“¯‚¶‚È‚ç‰½‚à‚µ‚È‚¢
	GetWindowRect(&rectNow);
	if ((rectNow.Width() == rectWnd.Width()) && (rectNow.Height() == rectWnd.Height())) {
		return;
	}

	// ƒTƒCƒY•ÏX
	SetWindowPos(&wndTop, 0, 0, rectWnd.Width(), rectWnd.Height(), SWP_NOMOVE);
}

//===========================================================================
//
//	ƒTƒuƒeƒLƒXƒg‰Â•ÏƒEƒBƒ“ƒhƒE
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	ƒRƒ“ƒXƒgƒ‰ƒNƒ^
//
//---------------------------------------------------------------------------
CSubTextSizeWnd::CSubTextSizeWnd()
{
}

//---------------------------------------------------------------------------
//
//	ƒEƒBƒ“ƒhƒEì¬€”õ
//
//---------------------------------------------------------------------------
BOOL CSubTextSizeWnd::PreCreateWindow(CREATESTRUCT& cs)
{
	ASSERT(this);

	// Šî–{ƒNƒ‰ƒX
	if (!CSubTextWnd::PreCreateWindow(cs)) {
		return FALSE;
	}

	// ƒTƒCƒY‰Â•ÏAÅ‘å‰
	cs.style |= WS_THICKFRAME;
	cs.style |= WS_MAXIMIZEBOX;

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	•`‰æƒƒCƒ“
//
//---------------------------------------------------------------------------
void FASTCALL CSubTextSizeWnd::OnDraw(CDC *pDC)
{
	CRect rect;

	ASSERT(this);
	ASSERT(pDC);

	// ƒNƒ‰ƒCƒAƒ“ƒg‹éŒ`‚ğæ“¾
	GetClientRect(&rect);

	// ‰E‘¤‚ğƒNƒŠƒA
	rect.left = m_nWidth * m_tmWidth;
	pDC->FillSolidRect(&rect, GetSysColor(COLOR_WINDOW));

	// ‰º‘¤‚ğƒNƒŠƒA
	rect.left = 0;
	rect.top = m_nHeight * m_tmHeight;
	pDC->FillSolidRect(&rect, GetSysColor(COLOR_WINDOW));

	// Šî–{ƒNƒ‰ƒX‚Ö
	CSubTextWnd::OnDraw(pDC);
}

//===========================================================================
//
//	ƒTƒuƒeƒLƒXƒgƒXƒNƒ[ƒ‹ƒEƒBƒ“ƒhƒE
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	ƒRƒ“ƒXƒgƒ‰ƒNƒ^
//
//---------------------------------------------------------------------------
CSubTextScrlWnd::CSubTextScrlWnd()
{
	// ƒƒ“ƒo•Ï”‰Šú‰»
	m_ScrlWidth = -1;
	m_ScrlHeight = -1;
	m_bScrlH = FALSE;
	m_bScrlV = FALSE;
	m_ScrlX = 0;
	m_ScrlY = 0;
}

//---------------------------------------------------------------------------
//
//	ƒƒbƒZ[ƒW ƒ}ƒbƒv
//
//---------------------------------------------------------------------------
BEGIN_MESSAGE_MAP(CSubTextScrlWnd, CSubTextSizeWnd)
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_WM_HSCROLL()
	ON_WM_VSCROLL()
END_MESSAGE_MAP()

//---------------------------------------------------------------------------
//
//	ƒEƒBƒ“ƒhƒEì¬€”õ
//
//---------------------------------------------------------------------------
BOOL CSubTextScrlWnd::PreCreateWindow(CREATESTRUCT& cs)
{
	ASSERT(this);

	// Šî–{ƒNƒ‰ƒX
	if (!CSubTextSizeWnd::PreCreateWindow(cs)) {
		return FALSE;
	}

	// ƒXƒNƒ[ƒ‹ƒo[‚ğ’Ç‰Á
	cs.style |= WS_HSCROLL;
	cs.style |= WS_VSCROLL;

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	ƒEƒBƒ“ƒhƒEì¬
//
//---------------------------------------------------------------------------
int CSubTextScrlWnd::OnCreate(LPCREATESTRUCT lpcs)
{
	CRect wrect;
	CRect crect;

	ASSERT(this);

	// Šî–{ƒNƒ‰ƒX
	if (CSubTextSizeWnd::OnCreate(lpcs) != 0) {
		return -1;
	}

	// ƒXƒNƒ[ƒ‹€”õ
	ShowScrollBar(SB_BOTH, FALSE);
	SetupScrl();

	// ƒTƒCƒY’²®
	GetWindowRect(&wrect);
	GetClientRect(&crect);
	wrect.right -= wrect.left;
	wrect.right -= crect.right;
	wrect.right += m_ScrlWidth * m_tmWidth;
	SetWindowPos(&wndTop, 0, 0, wrect.right, wrect.Height(),
						SWP_NOMOVE | SWP_NOZORDER);

	return 0;
}

//---------------------------------------------------------------------------
//
//	ƒTƒCƒY•ÏX
//
//---------------------------------------------------------------------------
void CSubTextScrlWnd::OnSize(UINT nType, int cx, int cy)
{
	ASSERT(this);

	// Šî–{ƒNƒ‰ƒX‚ğæ‚ÉÀs
	CSubTextSizeWnd::OnSize(nType, cx, cy);

	// ƒXƒNƒ[ƒ‹€”õ
	SetupScrl();
}

//---------------------------------------------------------------------------
//
//	ƒXƒNƒ[ƒ‹€”õ
//
//---------------------------------------------------------------------------
void FASTCALL CSubTextScrlWnd::SetupScrl()
{
	CRect rect;
	int width;
	int height;

	ASSERT(this);
	ASSERT(m_ScrlWidth >= 0);
	ASSERT(m_ScrlHeight >= 0);
	ASSERT(m_tmWidth > 0);
	ASSERT(m_tmHeight > 0);

	// ƒNƒ‰ƒCƒAƒ“ƒgƒTƒCƒY‚ğæ“¾
	GetClientRect(&rect);

	// Å¬‰»‚È‚ç‰½‚à‚µ‚È‚¢
	if ((rect.right == 0) || (rect.bottom == 0)) {
		return;
	}

	// HƒXƒNƒ[ƒ‹‚ğ”»’è
	width = rect.right / m_tmWidth;
	if (width < m_ScrlWidth) {
		if (!m_bScrlH) {
			m_bScrlH = TRUE;
			m_ScrlX = 0;
			ShowScrollBar(SB_HORZ, TRUE);
		}
		// ƒXƒNƒ[ƒ‹ƒo[‚ª•K—v‚È‚Ì‚ÅAÚ×İ’è
		SetupScrlH();
	}
	else {
		if (m_bScrlH) {
			m_bScrlH = FALSE;
			m_ScrlX = 0;
			ShowScrollBar(SB_HORZ, FALSE);
		}
	}

	// VƒXƒNƒ[ƒ‹‚ğ”»’è
	height = rect.bottom / m_tmHeight;
	if (height < m_ScrlHeight) {
		if (!m_bScrlV) {
			m_bScrlV = TRUE;
			m_ScrlY = 0;
			ShowScrollBar(SB_VERT, TRUE);
		}
		// ƒXƒNƒ[ƒ‹ƒo[‚ª•K—v‚È‚Ì‚ÅAÚ×İ’è
		SetupScrlV();
	}
	else {
		if (m_bScrlV) {
			m_bScrlV = FALSE;
			m_ScrlY = 0;
			ShowScrollBar(SB_VERT, FALSE);
		}
	}
}

//---------------------------------------------------------------------------
//
//	ƒXƒNƒ[ƒ‹€”õ(…•½)
//
//---------------------------------------------------------------------------
void FASTCALL CSubTextScrlWnd::SetupScrlH()
{
	SCROLLINFO si;
	CRect rect;
	int width;

	ASSERT(this);

	// …•½•\¦‰Â”\ƒLƒƒƒ‰ƒNƒ^‚ğæ“¾
	GetClientRect(&rect);
	width = rect.right / m_tmWidth;

	// ƒXƒNƒ[ƒ‹î•ñ‚ğƒZƒbƒg
	memset(&si, 0, sizeof(si));
	si.cbSize = sizeof(si);
	si.fMask = SIF_ALL;
	si.nMin = 0;
	si.nMax = m_ScrlWidth - 1;
	si.nPage = width;

	// ˆÊ’u‚ÍA•K—v‚È‚ç•â³‚·‚é
	si.nPos = m_ScrlX;
	if (si.nPos + width > m_ScrlWidth) {
		si.nPos = m_ScrlWidth - width;
		if (si.nPos < 0) {
			si.nPos = 0;
		}
		m_ScrlX = si.nPos;
	}

	SetScrollInfo(SB_HORZ, &si, TRUE);
}

//---------------------------------------------------------------------------
//
//	ƒXƒNƒ[ƒ‹(…•½)
//
//---------------------------------------------------------------------------
void CSubTextScrlWnd::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar * /*pBar*/)
{
	SCROLLINFO si;

	ASSERT(this);

	// ƒXƒNƒ[ƒ‹î•ñ‚ğæ“¾
	memset(&si, 0, sizeof(si));
	si.cbSize = sizeof(si);
	GetScrollInfo(SB_HORZ, &si, SIF_ALL);

	// ƒXƒNƒ[ƒ‹ƒo[ƒR[ƒh•Ê
	switch (nSBCode) {
		// ¶‚Ö
		case SB_LEFT:
			m_ScrlX = si.nMin;
			break;

		// ‰E‚Ö
		case SB_RIGHT:
			m_ScrlX = si.nMax;
			break;

		// 1ƒ‰ƒCƒ“¶‚Ö
		case SB_LINELEFT:
			if (m_ScrlX > 0) {
				m_ScrlX--;
			}
			break;

		// 1ƒ‰ƒCƒ“‰E‚Ö
		case SB_LINERIGHT:
			if (m_ScrlX < si.nMax) {
				m_ScrlX++;
			}
			break;

		// 1ƒy[ƒW¶‚Ö
		case SB_PAGELEFT:
			if (m_ScrlX >= (int)si.nPage) {
				m_ScrlX -= (int)si.nPage;
			}
			else {
				m_ScrlX = 0;
			}
			break;

		// 1ƒy[ƒW‰E‚Ö
		case SB_PAGERIGHT:
			if ((m_ScrlX + (int)si.nPage) <= (int)si.nMax) {
				m_ScrlX += si.nPage;
			}
			else {
				m_ScrlX = si.nMax;
			}
			break;

		// ƒTƒ€ˆÚ“®
		case SB_THUMBPOSITION:
		case SB_THUMBTRACK:
			m_ScrlX = nPos;
			break;
	}

	// ³‚É•â³
	if (m_ScrlX < 0) {
		m_ScrlX = 0;
	}

	// ƒZƒbƒg
	SetupScrlH();
}

//---------------------------------------------------------------------------
//
//	ƒXƒNƒ[ƒ‹€”õ(‚’¼)
//
//---------------------------------------------------------------------------
void FASTCALL CSubTextScrlWnd::SetupScrlV()
{
	SCROLLINFO si;
	CRect rect;
	int height;

	ASSERT(this);

	// ‚’¼•\¦‰Â”\ƒLƒƒƒ‰ƒNƒ^‚ğæ“¾
	GetClientRect(&rect);
	height = rect.bottom / m_tmHeight;

	// ƒXƒNƒ[ƒ‹î•ñ‚ğƒZƒbƒg
	memset(&si, 0, sizeof(si));
	si.cbSize = sizeof(si);
	si.fMask = SIF_ALL;
	si.nMin = 0;
	si.nMax = m_ScrlHeight - 1;
	si.nPage = height;

	// ˆÊ’u‚ÍA•K—v‚È‚ç•â³‚·‚é
	si.nPos = m_ScrlY;
	if (si.nPos + height > m_ScrlHeight) {
		si.nPos = m_ScrlHeight - height;
		if (si.nPos < 0) {
			si.nPos = 0;
		}
		m_ScrlY = si.nPos;
	}

	SetScrollInfo(SB_VERT, &si, TRUE);
}

//---------------------------------------------------------------------------
//
//	ƒXƒNƒ[ƒ‹(‚’¼)
//
//---------------------------------------------------------------------------
void CSubTextScrlWnd::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar * /*pBar*/)
{
	SCROLLINFO si;

	ASSERT(this);

	// ƒXƒNƒ[ƒ‹î•ñ‚ğæ“¾
	memset(&si, 0, sizeof(si));
	si.cbSize = sizeof(si);
	GetScrollInfo(SB_VERT, &si, SIF_ALL);

	// ƒXƒNƒ[ƒ‹ƒo[ƒR[ƒh•Ê
	switch (nSBCode) {
		// ã‚Ö
		case SB_TOP:
			m_ScrlY = si.nMin;
			break;

		// ‰º‚Ö
		case SB_BOTTOM:
			m_ScrlY = si.nMax;
			break;

		// 1ƒ‰ƒCƒ“ã‚Ö
		case SB_LINEUP:
			if (m_ScrlY > 0) {
				m_ScrlY--;
			}
			break;

		// 1ƒ‰ƒCƒ“‰º‚Ö
		case SB_LINEDOWN:
			if (m_ScrlY < si.nMax) {
				m_ScrlY++;
			}
			break;

		// 1ƒy[ƒWã‚Ö
		case SB_PAGEUP:
			if (m_ScrlY >= (int)si.nPage) {
				m_ScrlY -= si.nPage;
			}
			else {
				m_ScrlY = 0;
			}
			break;

		// 1ƒy[ƒW‰º‚Ö
		case SB_PAGEDOWN:
			if ((m_ScrlY + (int)si.nPage) <= (int)si.nMax) {
				m_ScrlY += si.nPage;
			}
			else {
				m_ScrlY = si.nMax;
			}
			break;

		// ƒTƒ€ˆÚ“®
		case SB_THUMBPOSITION:
			m_ScrlY = nPos;
			break;
		case SB_THUMBTRACK:
			m_ScrlY = nPos;
			break;
	}

	// ³‚É•â³
	if (m_ScrlY < 0) {
		m_ScrlY = 0;
	}

	// ƒZƒbƒgA•`‰æ
	SetupScrlV();
}

//===========================================================================
//
//	ƒTƒuƒŠƒXƒgƒEƒBƒ“ƒhƒE
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	ƒRƒ“ƒXƒgƒ‰ƒNƒ^
//
//---------------------------------------------------------------------------
CSubListWnd::CSubListWnd()
{
	// ƒŠƒXƒgƒRƒ“ƒgƒ[ƒ‹‚Ì€”õ‚ªI‚í‚é‚Ü‚ÅDisable‚µ‚Ä‚¨‚­
	m_bEnable = FALSE;
}

//---------------------------------------------------------------------------
//
//	ƒƒbƒZ[ƒW ƒ}ƒbƒv
//
//---------------------------------------------------------------------------
BEGIN_MESSAGE_MAP(CSubListWnd, CSubWnd)
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_WM_DRAWITEM()
END_MESSAGE_MAP()

//---------------------------------------------------------------------------
//
//	ƒEƒBƒ“ƒhƒEì¬€”õ
//
//---------------------------------------------------------------------------
BOOL CSubListWnd::PreCreateWindow(CREATESTRUCT& cs)
{
	ASSERT(this);

	// Šî–{ƒNƒ‰ƒX
	if (!CSubWnd::PreCreateWindow(cs)) {
		return FALSE;
	}

	// ƒEƒBƒ“ƒhƒEƒTƒCƒY‚ğ‰Â•Ï‚Æ‚·‚é
	cs.style |= WS_THICKFRAME;
	cs.style |= WS_MAXIMIZEBOX;

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	ƒEƒBƒ“ƒhƒEì¬
//
//---------------------------------------------------------------------------
int CSubListWnd::OnCreate(LPCREATESTRUCT lpcs)
{
	CRect rect;
	CDC *pDC;
	TEXTMETRIC tm;

	// Šî–{ƒNƒ‰ƒX
	if (CSubWnd::OnCreate(lpcs) != 0) {
		return -1;
	}

	// ƒŠƒXƒgƒRƒ“ƒgƒ[ƒ‹‚ğì¬
	VERIFY(m_ListCtrl.Create(WS_CHILD | WS_VISIBLE |
							LVS_REPORT | LVS_NOSORTHEADER| LVS_SINGLESEL |
						 	LVS_OWNERDRAWFIXED | LVS_OWNERDATA,
							CRect(0, 0, 0, 0), this, 0));

	// ƒtƒHƒ“ƒgƒTƒCƒY‚ğæ‚è’¼‚·
	pDC = m_ListCtrl.GetDC();
	pDC->GetTextMetrics(&tm);
	m_ListCtrl.ReleaseDC(pDC);
	m_tmWidth = tm.tmAveCharWidth;
	m_tmHeight = tm.tmHeight + tm.tmExternalLeading;

	// ‰Šú‰»
	InitList();

	// —LŒø‚É‚µ‚ÄAƒŠƒtƒŒƒbƒVƒ…
	m_bEnable = TRUE;
	Refresh();

	return 0;
}

//---------------------------------------------------------------------------
//
//	ƒTƒCƒY•ÏX
//
//---------------------------------------------------------------------------
void CSubListWnd::OnSize(UINT nType, int cx, int cy)
{
	CRect rect;

	ASSERT(this);

	// Šî–{ƒNƒ‰ƒX
	CSubWnd::OnSize(nType, cx, cy);

	// ƒNƒ‰ƒCƒAƒ“ƒgƒGƒŠƒAˆê”t‚É‚È‚é‚æ‚¤‚ÉAƒŠƒXƒgƒRƒ“ƒgƒ[ƒ‹‚ğ’²®
	if (m_ListCtrl.GetSafeHwnd()) {
		GetClientRect(&rect);
		m_ListCtrl.SetWindowPos(&wndTop, 0, 0, rect.right, rect.bottom, SWP_NOZORDER);
	}
}

//---------------------------------------------------------------------------
//
//	ƒI[ƒi[ƒhƒ[
//
//---------------------------------------------------------------------------
void CSubListWnd::OnDrawItem(int /*nID*/, LPDRAWITEMSTRUCT /*lpDIS*/)
{
	// •K‚¸”h¶ƒNƒ‰ƒX‚Å’è‹`‚·‚é‚±‚Æ
	ASSERT(FALSE);
}

//===========================================================================
//
//	ƒTƒuBMPƒEƒBƒ“ƒhƒE
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	ƒRƒ“ƒXƒgƒ‰ƒNƒ^
//
//---------------------------------------------------------------------------
CSubBMPWnd::CSubBMPWnd()
{
	// ƒrƒbƒgƒ}ƒbƒv‚È‚µ
	memset(&m_bmi, 0, sizeof(m_bmi));
	m_bmi.biSize = sizeof(BITMAPINFOHEADER);
	m_pBits = NULL;
	m_hBitmap = NULL;

	// ”{—¦100%
	m_nMul = 2;

	// ‰¼‘z‰æ–ÊƒTƒCƒY
	m_nScrlWidth = -1;
	m_nScrlHeight = -1;

	// ƒXƒNƒ[ƒ‹
	m_nScrlX = 0;
	m_nScrlY = 0;

	// ƒ}ƒEƒXƒJ[ƒ\ƒ‹
	m_nCursorX = -1;
	m_nCursorY = -1;
}

//---------------------------------------------------------------------------
//
//	ƒƒbƒZ[ƒW ƒ}ƒbƒv
//
//---------------------------------------------------------------------------
BEGIN_MESSAGE_MAP(CSubBMPWnd, CWnd)
	ON_WM_CREATE()
	ON_WM_DESTROY()
	ON_WM_SIZE()
	ON_WM_ERASEBKGND()
	ON_WM_PAINT()
	ON_WM_HSCROLL()
	ON_WM_VSCROLL()
	ON_WM_MOUSEMOVE()
END_MESSAGE_MAP()

//---------------------------------------------------------------------------
//
//	ƒEƒBƒ“ƒhƒEì¬
//
//---------------------------------------------------------------------------
int CSubBMPWnd::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	ASSERT(this);

	// Šî–{ƒNƒ‰ƒX
	if (CWnd::OnCreate(lpCreateStruct) != 0) {
		return -1;
	}

	// IMEƒIƒt
	::ImmAssociateContext(m_hWnd, (HIMC)NULL);

	// ƒXƒNƒ[ƒ‹ƒo[‚ ‚è
	ShowScrollBar(SB_HORZ, TRUE);
	ShowScrollBar(SB_VERT, TRUE);

	return 0;
}

//---------------------------------------------------------------------------
//
//	ƒEƒBƒ“ƒhƒEíœ
//
//---------------------------------------------------------------------------
void CSubBMPWnd::OnDestroy()
{
	ASSERT(this);
	ASSERT_VALID(this);

	// ƒrƒbƒgƒ}ƒbƒvíœ
	if (m_hBitmap) {
		::DeleteObject(m_hBitmap);
		m_hBitmap = NULL;
		ASSERT(m_pBits);
		m_pBits = NULL;
	}

	// Šî–{ƒNƒ‰ƒX
	CWnd::OnDestroy();
}

//---------------------------------------------------------------------------
//
//	ƒEƒBƒ“ƒhƒEíœŠ®—¹
//
//---------------------------------------------------------------------------
void CSubBMPWnd::PostNcDestroy()
{
	ASSERT(this);
	ASSERT(!m_hBitmap);
	ASSERT(!m_pBits);

	// ƒCƒ“ƒ^ƒtƒF[ƒX—v‘f‚ğíœ
	delete this;
}

//---------------------------------------------------------------------------
//
//	ƒEƒBƒ“ƒhƒEƒTƒCƒY•ÏX
//
//---------------------------------------------------------------------------
void CSubBMPWnd::OnSize(UINT nType, int cx, int cy)
{
	CClientDC dc(this);

	ASSERT(this);

	// Šî–{ƒNƒ‰ƒX
	CWnd::OnSize(nType, cx, cy);

	// ƒ}ƒEƒX‰Šú‰»
	m_nCursorX = -1;
	m_nCursorY = -1;

	// ƒƒbƒN
	::LockVM();

	// ƒrƒbƒgƒ}ƒbƒv‚ğ‚Á‚Ä‚¢‚ê‚ÎAˆê’U‰ğ•ú
	if (m_hBitmap) {
		::DeleteObject(m_hBitmap);
		m_hBitmap = NULL;
		ASSERT(m_pBits);
		m_pBits = NULL;
	}

	// Å¬‰»‚È‚çƒŠƒ^[ƒ“
	if (nType == SIZE_MINIMIZED) {
		::UnlockVM();
		return;
	}

	// cx,cy‚ğƒXƒNƒ[ƒ‹—Ìˆæ‚Ü‚Å‚É‰Ÿ‚³‚¦‚é
	cx = (cx * 2) / m_nMul;
	if (cx >= m_nScrlWidth) {
		cx = m_nScrlWidth;
	}
	cy = (cy * 2) / m_nMul;
	if (cy >= m_nScrlHeight) {
		cy = m_nScrlHeight;
	}

	// ƒrƒbƒgƒ}ƒbƒvì¬(32bitA“™”{)
	m_bmi.biWidth = cx;
	m_bmi.biHeight = -cy;
	m_bmi.biPlanes = 1;
	m_bmi.biBitCount = 32;
	m_bmi.biSizeImage = cx * cy * 4;
	m_hBitmap = ::CreateDIBSection(dc.m_hDC, (BITMAPINFO*)&m_bmi,
						DIB_RGB_COLORS, (void**)&m_pBits, NULL, 0);

	// ‰Šú‰»(•‚Å“h‚è‚Â‚Ô‚·)
	if (m_hBitmap) {
		memset(m_pBits, 0, m_bmi.biSizeImage);
	}

	// ƒXƒNƒ[ƒ‹İ’è
	SetupScrlH();
	SetupScrlV();

	// ƒAƒ“ƒƒbƒN
	::UnlockVM();
}

//---------------------------------------------------------------------------
//
//	”wŒi•`‰æ
//
//---------------------------------------------------------------------------
BOOL CSubBMPWnd::OnEraseBkgnd(CDC* /*pDC*/)
{
	// ‰½‚à‚µ‚È‚¢
	return TRUE;
}

//---------------------------------------------------------------------------
//
//	Ä•`‰æ
//
//---------------------------------------------------------------------------
void CSubBMPWnd::OnPaint()
{
	PAINTSTRUCT ps;

	// Windows‚É‘Î‚·‚éƒ|[ƒY‚¾‚¯
	BeginPaint(&ps);
	EndPaint(&ps);
}

#if !defined(NDEBUG)
//---------------------------------------------------------------------------
//
//	f’f
//
//---------------------------------------------------------------------------
void CSubBMPWnd::AssertValid() const
{
	ASSERT(this);
	ASSERT(m_hWnd);
	ASSERT(::IsWindow(m_hWnd));
	ASSERT(m_nMul >= 1);
	ASSERT(m_nScrlWidth > 0);
	ASSERT(m_nScrlHeight > 0);
	ASSERT(m_nScrlX >= 0);
	ASSERT(m_nScrlX < m_nScrlWidth);
	ASSERT(m_nScrlY >= 0);
	ASSERT(m_nScrlY < m_nScrlHeight);
}
#endif	// NDEBUG

//---------------------------------------------------------------------------
//
//	ƒXƒNƒ[ƒ‹€”õ(…•½)
//
//---------------------------------------------------------------------------
void FASTCALL CSubBMPWnd::SetupScrlH()
{
	SCROLLINFO si;
	CRect rect;

	ASSERT(this);
	ASSERT_VALID(this);

	// ƒrƒbƒgƒ}ƒbƒv‚ª‚È‚¯‚ê‚ÎA‚â‚ç‚È‚¢
	if (!m_hBitmap) {
		return;
	}

	// ƒXƒNƒ[ƒ‹î•ñ‚ğƒZƒbƒg
	memset(&si, 0, sizeof(si));
	si.cbSize = sizeof(si);
	si.fMask = SIF_ALL;
	si.nMin = 0;
	si.nMax = m_nScrlWidth - 1;
	si.nPage = m_bmi.biWidth;

	// ˆÊ’u‚ÍA•K—v‚È‚ç•â³‚·‚é
	si.nPos = m_nScrlX;
	if (si.nPos + (int)si.nPage >= m_nScrlWidth) {
		si.nPos = m_nScrlWidth - (int)si.nPage;
	}
	if (si.nPos < 0) {
		si.nPos = 0;
	}
	m_nScrlX = si.nPos;
	ASSERT((m_nScrlX >= 0) && (m_nScrlX < m_nScrlWidth));

	// İ’è
	SetScrollInfo(SB_HORZ, &si, TRUE);
}

//---------------------------------------------------------------------------
//
//	ƒXƒNƒ[ƒ‹(…•½)
//
//---------------------------------------------------------------------------
void CSubBMPWnd::OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* /*pBar*/)
{
	SCROLLINFO si;

	ASSERT(this);
	ASSERT_VALID(this);

	// ƒXƒNƒ[ƒ‹î•ñ‚ğæ“¾
	memset(&si, 0, sizeof(si));
	si.cbSize = sizeof(si);
	GetScrollInfo(SB_HORZ, &si, SIF_ALL);

	// ƒXƒNƒ[ƒ‹ƒo[ƒR[ƒh•Ê
	switch (nSBCode) {
		// ¶‚Ö
		case SB_LEFT:
			m_nScrlX = si.nMin;
			break;

		// ‰E‚Ö
		case SB_RIGHT:
			m_nScrlX = si.nMax;
			break;

		// 1ƒ‰ƒCƒ“¶‚Ö
		case SB_LINELEFT:
			if (m_nScrlX > 0) {
				m_nScrlX--;
			}
			break;

		// 1ƒ‰ƒCƒ“‰E‚Ö
		case SB_LINERIGHT:
			if (m_nScrlX < si.nMax) {
				m_nScrlX++;
			}
			break;

		// 1ƒy[ƒW¶‚Ö
		case SB_PAGELEFT:
			if (m_nScrlX >= (int)si.nPage) {
				m_nScrlX -= (int)si.nPage;
			}
			else {
				m_nScrlX = 0;
			}
			break;

		// 1ƒy[ƒW‰E‚Ö
		case SB_PAGERIGHT:
			if ((m_nScrlX + (int)si.nPage) <= si.nMax) {
				m_nScrlX += (int)si.nPage;
			}
			else {
				m_nScrlX = si.nMax;
			}
			break;

		// ƒTƒ€ˆÚ“®
		case SB_THUMBPOSITION:
		case SB_THUMBTRACK:
			m_nScrlX = nPos;
			break;
	}
	ASSERT((m_nScrlX >= 0) && (m_nScrlX < m_nScrlWidth));

	// ƒZƒbƒg
	SetupScrlH();
}

//---------------------------------------------------------------------------
//
//	ƒXƒNƒ[ƒ‹€”õ(‚’¼)
//
//---------------------------------------------------------------------------
void FASTCALL CSubBMPWnd::SetupScrlV()
{
	SCROLLINFO si;

	ASSERT(this);
	ASSERT_VALID(this);

	// ƒrƒbƒgƒ}ƒbƒv‚ª‚È‚¯‚ê‚ÎA‚â‚ç‚È‚¢
	if (!m_hBitmap) {
		return;
	}

	// ƒXƒNƒ[ƒ‹î•ñ‚ğƒZƒbƒg
	memset(&si, 0, sizeof(si));
	si.cbSize = sizeof(si);
	si.fMask = SIF_ALL;
	si.nMin = 0;
	si.nMax = m_nScrlHeight - 1;
	si.nPage = -m_bmi.biHeight;

	// ˆÊ’u‚ÍA•K—v‚È‚ç•â³‚·‚é
	si.nPos = m_nScrlY;
	if (si.nPos + (int)si.nPage >= m_nScrlHeight) {
		si.nPos = m_nScrlHeight - (int)si.nPage;
	}
	if (si.nPos < 0) {
		si.nPos = 0;
	}
	m_nScrlY = si.nPos;
	ASSERT((m_nScrlY >= 0) && (m_nScrlY <= m_nScrlHeight));

	SetScrollInfo(SB_VERT, &si, TRUE);
}

//---------------------------------------------------------------------------
//
//	ƒXƒNƒ[ƒ‹(‚’¼)
//
//---------------------------------------------------------------------------
void CSubBMPWnd::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* /*pBar*/)
{
	SCROLLINFO si;

	ASSERT(this);
	ASSERT_VALID(this);

	// ƒXƒNƒ[ƒ‹î•ñ‚ğæ“¾
	memset(&si, 0, sizeof(si));
	si.cbSize = sizeof(si);
	GetScrollInfo(SB_VERT, &si, SIF_ALL);

	// ƒXƒNƒ[ƒ‹ƒo[ƒR[ƒh•Ê
	switch (nSBCode) {
		// ã‚Ö
		case SB_TOP:
			m_nScrlY = si.nMin;
			break;

		// ‰º‚Ö
		case SB_BOTTOM:
			m_nScrlY = si.nMax;
			break;

		// 1ƒ‰ƒCƒ“ã‚Ö
		case SB_LINEUP:
			if (m_nScrlY > 0) {
				m_nScrlY--;
			}
			break;

		// 1ƒ‰ƒCƒ“‰º‚Ö
		case SB_LINEDOWN:
			if (m_nScrlY < si.nMax) {
				m_nScrlY++;
			}
			break;

		// 1ƒy[ƒWã‚Ö
		case SB_PAGEUP:
			if (m_nScrlY >= (int)si.nPage) {
				m_nScrlY -= (int)si.nPage;
			}
			else {
				m_nScrlY = 0;
			}
			break;

		// 1ƒy[ƒW‰º‚Ö
		case SB_PAGEDOWN:
			if ((m_nScrlY + (int)si.nPage) <= si.nMax) {
				m_nScrlY += (int)si.nPage;
			}
			else {
				m_nScrlY = si.nMax;
			}
			break;

		// ƒTƒ€ˆÚ“®
		case SB_THUMBPOSITION:
			m_nScrlY = nPos;
			break;
		case SB_THUMBTRACK:
			m_nScrlY = nPos;
			break;
	}

	ASSERT((m_nScrlY >= 0) && (m_nScrlY <= m_nScrlHeight));

	// ƒZƒbƒg
	SetupScrlV();
}

//---------------------------------------------------------------------------
//
//	ƒ}ƒEƒXˆÚ“®
//
//---------------------------------------------------------------------------
void CSubBMPWnd::OnMouseMove(UINT nFlags, CPoint point)
{
	ASSERT(this);
	ASSERT_VALID(this);

	// ƒ}ƒEƒXˆÚ“®ˆÊ’u‚ğ‹L‰¯
	m_nCursorX = point.x;
	m_nCursorY = point.y;

	// ”{—¦‚ğl—¶
	m_nCursorX = (m_nCursorX * 2) / m_nMul;
	m_nCursorY = (m_nCursorY * 2) / m_nMul;

	// Šî–{ƒNƒ‰ƒX
	CWnd::OnMouseMove(nFlags, point);
}

//---------------------------------------------------------------------------
//
//	•`‰æ
//
//---------------------------------------------------------------------------
void FASTCALL CSubBMPWnd::Refresh()
{
	CClientDC dc(this);
	CDC mDC;
	HBITMAP hBitmap;

	ASSERT(this);
	ASSERT_VALID(this);

	// ƒrƒbƒgƒ}ƒbƒv‚ª‚ ‚éê‡‚ÉŒÀ‚è
	if (m_hBitmap) {
		// ƒƒ‚ƒŠDCì¬
		mDC.CreateCompatibleDC(&dc);

		// ƒIƒuƒWƒFƒNƒg‘I‘ğ
		hBitmap = (HBITMAP)::SelectObject(mDC.m_hDC, m_hBitmap);

		// BitBlt or StretchBlt
		if (hBitmap) {
			if (m_nMul == 2) {
				// “™”{
				dc.BitBlt(0, 0, m_bmi.biWidth, -m_bmi.biHeight,
									&mDC, 0, 0, SRCCOPY);
			}
			else {
				// n”{
				dc.StretchBlt(  0, 0,
								(m_bmi.biWidth * m_nMul) >> 1,
								-((m_bmi.biHeight * m_nMul) >> 1),
								&mDC,
								0, 0,
								m_bmi.biWidth, -m_bmi.biHeight,
								SRCCOPY);
			}

			// ƒIƒuƒWƒFƒNƒg‘I‘ğI—¹
			::SelectObject(mDC.m_hDC, hBitmap);
		}

		// ƒƒ‚ƒŠDCI—¹
		mDC.DeleteDC();
	}
}

//---------------------------------------------------------------------------
//
//	Å‘åƒEƒBƒ“ƒhƒE‹éŒ`æ“¾
//
//---------------------------------------------------------------------------
void FASTCALL CSubBMPWnd::GetMaximumRect(LPRECT lpRect, BOOL bScroll)
{
	ASSERT(this);
	ASSERT(lpRect);
	ASSERT_VALID(this);

	// BMPƒEƒBƒ“ƒhƒE‚ÌÅ‘å‚ÌƒTƒCƒY‚ğ“¾‚é
	lpRect->left = 0;
	lpRect->top = 0;
	lpRect->right = (m_nScrlWidth * m_nMul) >> 1;
	lpRect->bottom = (m_nScrlHeight * m_nMul) >> 1;

	// ƒXƒNƒ[ƒ‹ƒo[‚ğ‰ÁZ
	if (bScroll) {
		lpRect->right += ::GetSystemMetrics(SM_CXVSCROLL);
		lpRect->bottom += ::GetSystemMetrics(SM_CYHSCROLL);
	}

	// ƒEƒBƒ“ƒhƒE‹éŒ`‚É•ÏŠ·
	CalcWindowRect(lpRect);
}

//---------------------------------------------------------------------------
//
//	ƒtƒBƒbƒg‹éŒ`æ“¾
//
//---------------------------------------------------------------------------
void FASTCALL CSubBMPWnd::GetFitRect(LPRECT lpRect)
{
	ASSERT(this);
	ASSERT(lpRect);
	ASSERT_VALID(this);

	// Œ»İ‚ÌƒNƒ‰ƒCƒAƒ“ƒg‹éŒ`‚ğ“¾‚é
	GetClientRect(lpRect);

	// ƒEƒBƒ“ƒhƒE‹éŒ`‚É•ÏŠ·
	CalcWindowRect(lpRect);
}

//---------------------------------------------------------------------------
//
//	•`‰æ‹éŒ`æ“¾
//
//---------------------------------------------------------------------------
void FASTCALL CSubBMPWnd::GetDrawRect(LPRECT lpRect)
{
	ASSERT(this);
	ASSERT(lpRect);
	ASSERT_VALID(this);

	// ƒrƒbƒgƒ}ƒbƒv‚ª‚È‚¯‚ê‚ÎAƒGƒ‰[
	if (!m_hBitmap) {
		ASSERT(!m_pBits);
		lpRect->top = 0;
		lpRect->left = 0;
		lpRect->right = 0;
		lpRect->bottom = 0;
		return;
	}

	// ƒrƒbƒgƒ}ƒbƒv‚ ‚è
	ASSERT(m_pBits);

	// ƒXƒNƒ[ƒ‹İ’è
	lpRect->left = m_nScrlX;
	lpRect->top = m_nScrlY;

	// ”ÍˆÍİ’è(m_bmi.biHeight‚Íí‚É•‰)
	lpRect->right = lpRect->left + m_bmi.biWidth;
	lpRect->bottom = lpRect->top - m_bmi.biHeight;

	// ŒŸ¸
	ASSERT(lpRect->left <= lpRect->right);
	ASSERT(lpRect->top <= lpRect->bottom);
	ASSERT(lpRect->right <= m_nScrlWidth);
	ASSERT(lpRect->bottom <= m_nScrlHeight);
}

//---------------------------------------------------------------------------
//
//	ƒrƒbƒgæ“¾
//
//---------------------------------------------------------------------------
BYTE* FASTCALL CSubBMPWnd::GetBits() const
{
	ASSERT(this);
	ASSERT_VALID(this);

	// ƒrƒbƒgƒ}ƒbƒv‚ ‚è
	if (m_pBits) {
		ASSERT(m_hBitmap);
		return m_pBits;
	}

	// ƒrƒbƒgƒ}ƒbƒv‚È‚µ
	ASSERT(!m_hBitmap);
	return NULL;
}

//===========================================================================
//
//	ƒTƒuƒrƒbƒgƒ}ƒbƒvƒEƒBƒ“ƒhƒE
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	ƒRƒ“ƒXƒgƒ‰ƒNƒ^
//
//---------------------------------------------------------------------------
CSubBitmapWnd::CSubBitmapWnd()
{
	// ƒƒ“ƒo•Ï”‰Šú‰»
	m_nWidth = 48;
	m_nHeight = 16;
	m_pBMPWnd = NULL;

	// ‰¼‘z‰æ–ÊƒTƒCƒY(”h¶ƒNƒ‰ƒX‚Å•K‚¸Ä’è‹`‚·‚é‚±‚Æ)
	m_nScrlWidth = -1;
	m_nScrlHeight = -1;
}

//---------------------------------------------------------------------------
//
//	ƒƒbƒZ[ƒW ƒ}ƒbƒv
//
//---------------------------------------------------------------------------
BEGIN_MESSAGE_MAP(CSubBitmapWnd, CSubWnd)
	ON_WM_CREATE()
	ON_WM_SIZING()
	ON_WM_SIZE()
END_MESSAGE_MAP()

//---------------------------------------------------------------------------
//
//	ƒEƒBƒ“ƒhƒEì¬€”õ
//
//---------------------------------------------------------------------------
BOOL CSubBitmapWnd::PreCreateWindow(CREATESTRUCT& cs)
{
	ASSERT(this);

	// Šî–{ƒNƒ‰ƒX
	if (!CSubWnd::PreCreateWindow(cs)) {
		return FALSE;
	}

	// ƒTƒCƒY‰Â•Ï
	cs.style |= WS_THICKFRAME;

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	ƒEƒBƒ“ƒhƒEì¬
//
//---------------------------------------------------------------------------
int CSubBitmapWnd::OnCreate(LPCREATESTRUCT lpcs)
{
	UINT id;
	CSize size;
	CRect rect;

	ASSERT(this);

	// Šî–{ƒNƒ‰ƒX
	if (CSubWnd::OnCreate(lpcs) != 0) {
		return -1;
	}

	// ƒXƒe[ƒ^ƒXƒo[
	id = 0;
	m_StatusBar.Create(this);
	size = m_StatusBar.CalcFixedLayout(TRUE, TRUE);
	GetClientRect(&rect);
	m_StatusBar.MoveWindow(0, rect.bottom - size.cy, rect.Width(), size.cy);
	m_StatusBar.SetIndicators(&id, 1);
	m_StatusBar.SetPaneInfo(0, 0, SBPS_STRETCH, 0);

	// BMPƒEƒBƒ“ƒhƒE
	rect.bottom -= size.cy;
	m_pBMPWnd = new CSubBMPWnd;
	ASSERT(m_nScrlWidth > 0);
	m_pBMPWnd->m_nScrlWidth = m_nScrlWidth;
	ASSERT(m_nScrlHeight > 0);
	m_pBMPWnd->m_nScrlHeight = m_nScrlHeight;
	m_pBMPWnd->Create(NULL, NULL, WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL,
					rect, this, 0, NULL);

	return 0;
}

//---------------------------------------------------------------------------
//
//	ƒTƒCƒY•ÏX’†
//
//---------------------------------------------------------------------------
void CSubBitmapWnd::OnSizing(UINT nSide, LPRECT lpRect)
{
	CRect rect;
	CSize sizeBar;
	CRect rectSizing;

	// Šî–{ƒNƒ‰ƒX
	CSubWnd::OnSizing(nSide, lpRect);

	// ƒXƒe[ƒ^ƒXƒo[‚ª‚È‚¯‚ê‚ÎAƒŠƒ^[ƒ“
	if (!::IsWindow(m_StatusBar.m_hWnd)) {
		return;
	}

	// BMPƒEƒBƒ“ƒhƒE‚ÌÅ‘åƒTƒCƒY‚ğ“¾‚é(ƒXƒNƒ[ƒ‹ƒo[‚İ)
	m_pBMPWnd->GetMaximumRect(&rect, TRUE);

	// ƒXƒe[ƒ^ƒXƒo[‚ÌƒTƒCƒY‚ğ“¾‚ÄA‡Œv
	sizeBar = m_StatusBar.CalcFixedLayout(TRUE, TRUE);
	rect.bottom += sizeBar.cy;

	// ‚±‚ÌƒEƒBƒ“ƒhƒE‚ÌÅ‘å‚ÌƒTƒCƒY‚ğ“¾‚é
	::AdjustWindowRectEx(&rect, GetStyle(), FALSE, GetExStyle());

	// ƒI[ƒo[ƒ`ƒFƒbƒN
	rectSizing = *lpRect;
	if (rectSizing.Width() >= rect.Width()) {
		lpRect->right = lpRect->left + rect.Width();
	}
	if (rectSizing.Height() >= rect.Height()) {
		lpRect->bottom = lpRect->top + rect.Height();
	}
}

//---------------------------------------------------------------------------
//
//	ƒTƒCƒY•ÏX
//
//---------------------------------------------------------------------------
void CSubBitmapWnd::OnSize(UINT nType, int cx, int cy)
{
	CSize sizeBar;
	CRect rectClient;
	CRect rectWnd;
	CRect rectMax;
	CRect rectFit;

	ASSERT(this);
	ASSERT(cx >= 0);
	ASSERT(cy >= 0);

	// Šî–{ƒNƒ‰ƒX
	CSubWnd::OnSize(nType, cx, cy);

	// ƒXƒe[ƒ^ƒXƒo[Ä”z’u(ƒEƒBƒ“ƒhƒE—LŒø‚Ìê‡‚ÉŒÀ’è)
	if (::IsWindow(m_StatusBar.m_hWnd)) {
		// ƒXƒe[ƒ^ƒXƒo[‚Ì‚‚³AƒNƒ‰ƒCƒAƒ“ƒg—Ìˆæ‚ÌL‚³‚ğ“¾‚é
		sizeBar = m_StatusBar.CalcFixedLayout(TRUE, TRUE);
		GetClientRect(&rectClient);

		// ƒNƒ‰ƒCƒAƒ“ƒg—Ìˆæ‚ªAƒXƒe[ƒ^ƒXƒo[‚ğû‚ß‚é‚½‚ß‚É\•ª‚Å‚ ‚ê‚ÎˆÊ’u•ÏX
		if (rectClient.Height() > sizeBar.cy) {
			m_StatusBar.MoveWindow(0,
								rectClient.Height() - sizeBar.cy,
								rectClient.Width(),
								sizeBar.cy);

			// BMPƒEƒBƒ“ƒhƒE‚ğ‚ ‚í‚¹‚ÄÄ”z’u
			rectClient.bottom -= sizeBar.cy;
			m_pBMPWnd->MoveWindow(0, 0, rectClient.Width(), rectClient.Height());
		}

		// BMPƒEƒBƒ“ƒhƒE‚ÌƒTƒCƒY‚ğ“¾‚ÄAƒI[ƒo[•ªæ“¾
		m_pBMPWnd->GetWindowRect(&rectWnd);
		m_pBMPWnd->GetMaximumRect(&rectMax, FALSE);
		cx = rectWnd.Width() - rectMax.Width();
		cy = rectWnd.Height() - rectMax.Height();

		// ‚à‚µ‚ ‚ê‚ÎA‚±‚ÌƒEƒBƒ“ƒhƒE‚ğ‚»‚ê‚¾‚¯k¬(ƒXƒNƒ[ƒ‹ƒo[‚Ì©“®ON/OFF‚É‘Îˆ)
		if ((cx > 0) || (cy > 0)) {
			GetWindowRect(&rectWnd);
			SetWindowPos(&wndTop, 0, 0, rectWnd.Width() - cx, rectWnd.Height() - cy,
						SWP_NOMOVE);
			return;
		}

#if 0
		// BMPƒEƒBƒ“ƒhƒE‚ÌƒtƒBƒbƒgƒTƒCƒY‚ğ“¾‚ÄAƒI[ƒo[•ªæ“¾
		m_pBMPWnd->GetFitRect(&rectFit);
		cx = rectWnd.Width() - rectFit.Width();
		cy = rectWnd.Height() - rectFit.Height();

		// ‚à‚µ‚ ‚ê‚ÎA‚±‚ÌƒEƒBƒ“ƒhƒE‚ğ‚»‚ê‚¾‚¯k¬(200%ˆÈã‚Ìê‡‚Ì—]•ª‚É‘Îˆ)
		if ((cx > 0) || (cy > 0)) {
			GetWindowRect(&rectWnd);
			SetWindowPos(&wndTop, 0, 0, rectWnd.Width() - cx, rectWnd.Height() - cy,
						SWP_NOMOVE);
		}
#endif
	}
}

//---------------------------------------------------------------------------
//
//	XV
//
//---------------------------------------------------------------------------
void FASTCALL CSubBitmapWnd::Refresh()
{
	CRect rect;

	// —LŒøƒtƒ‰ƒOƒ`ƒFƒbƒN
	if (!m_bEnable || !m_pBMPWnd) {
		return;
	}

	// •`‰æ‹éŒ`æ“¾
	m_pBMPWnd->GetDrawRect(&rect);
	if ((rect.Width() == 0) && (rect.Height() == 0)) {
		return;
	}

	// ƒZƒbƒgƒAƒbƒv
	Setup(rect.left, rect.top, rect.Width(), rect.Height(), m_pBMPWnd->GetBits());

	// •\¦
	m_pBMPWnd->Refresh();
}

//---------------------------------------------------------------------------
//
//	ƒpƒŒƒbƒg•ÏŠ·
//
//---------------------------------------------------------------------------
DWORD FASTCALL CSubBitmapWnd::ConvPalette(WORD value)
{
	DWORD r;
	DWORD g;
	DWORD b;

	// ‘S‚ÄƒRƒs[
	r = (DWORD)value;
	g = (DWORD)value;
	b = (DWORD)value;

	// MSB‚©‚çG:5AR:5AB:5AI:1‚Ì‡‚É‚È‚Á‚Ä‚¢‚é
	// ‚±‚ê‚ğ R:8 G:8 B:8‚ÌDWORD‚É•ÏŠ·Bb31-b24‚Íg‚í‚È‚¢
	r <<= 13;
	r &= 0xf80000;
	g &= 0x00f800;
	b <<= 2;
	b &= 0x0000f8;

	// ‹P“xƒrƒbƒg‚Íˆê—¥Up
	if (value & 1) {
		r |= 0x070000;
		g |= 0x000700;
		b |= 0x000007;
	}

	return (DWORD)(r | g | b);
}

#endif	// _WIN32
