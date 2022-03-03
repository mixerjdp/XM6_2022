//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2005 �o�h�D(ytanaka@ipc-tokai.or.jp)
//	[ MFC �T�u�E�B���h�E(�r�f�I) ]
//
//---------------------------------------------------------------------------

#if defined(_WIN32)

#include "os.h"
#include "xm6.h"
#include "vm.h"
#include "vc.h"
#include "tvram.h"
#include "gvram.h"
#include "sprite.h"
#include "render.h"
#include "mfc_sub.h"
#include "mfc_vid.h"
#include "mfc_asm.h"
#include "mfc_res.h"

//===========================================================================
//
//	�T�u�r�f�I�E�B���h�E
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	�R���X�g���N�^
//
//---------------------------------------------------------------------------
CSubVideoWnd::CSubVideoWnd()
{
	// �����T�C�Y�ݒ�
	m_nWidth = 48;
	m_nHeight = 16;

	// �`���C���h
	m_pBMPWnd = NULL;
	m_nPane = 1;

	// �ő�T�C�Y
	m_nScrlWidth = -1;
	m_nScrlHeight = -1;

	// �I�u�W�F�N�g
	m_pCRTC = NULL;
	m_pVC = NULL;

	// �t���O
	m_bScroll = FALSE;
	m_bPalette = FALSE;
	m_bContrast = FALSE;
}

//---------------------------------------------------------------------------
//
//	���b�Z�[�W �}�b�v
//
//---------------------------------------------------------------------------
BEGIN_MESSAGE_MAP(CSubVideoWnd, CSubWnd)
	ON_WM_CREATE()
	ON_WM_SIZING()
	ON_WM_SIZE()
END_MESSAGE_MAP()

//---------------------------------------------------------------------------
//
//	�E�B���h�E�쐬����
//
//---------------------------------------------------------------------------
BOOL CSubVideoWnd::PreCreateWindow(CREATESTRUCT& cs)
{
	ASSERT(this);

	// ��{�N���X
	if (!CSubWnd::PreCreateWindow(cs)) {
		return FALSE;
	}

	// �T�C�Y��
	cs.style |= WS_THICKFRAME;

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	�E�B���h�E�쐬
//
//---------------------------------------------------------------------------
int CSubVideoWnd::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	UINT uID[16];
	CSize size;
	CRect rect;
	int i;

	ASSERT(this);
	ASSERT(lpCreateStruct);
	ASSERT(m_nScrlWidth > 0);
	ASSERT(m_nScrlHeight > 0);

	// ��{�N���X
	if (CSubWnd::OnCreate(lpCreateStruct) != 0) {
		return -1;
	}

	// CRTC�擾
	ASSERT(!m_pCRTC);
	m_pCRTC = (CRTC*)::GetVM()->SearchDevice(MAKEID('C', 'R', 'T', 'C'));
	ASSERT(m_pCRTC);

	// VC�擾
	ASSERT(!m_pVC);
	m_pVC = (VC*)::GetVM()->SearchDevice(MAKEID('V', 'C', ' ', ' '));
	ASSERT(m_pVC);

	// �X�e�[�^�X�o�[�쐬
	m_StatusBar.Create(this);
	size = m_StatusBar.CalcFixedLayout(TRUE, TRUE);
	GetClientRect(&rect);
	m_StatusBar.MoveWindow(0, rect.bottom - size.cy, rect.Width(), size.cy);

	// �X�e�[�^�X�o�[�ݒ�
	ASSERT((m_nPane >= 1) && (m_nPane <= 16));
	for (i=0; i<m_nPane; i++) {
		if (i == 0) {
			uID[0] = ID_SEPARATOR;
			continue;
		}
		uID[i] = i;
	}
	m_StatusBar.SetIndicators(uID, m_nPane);
	m_StatusBar.SetPaneInfo(0, 0, SBPS_NOBORDERS | SBPS_STRETCH, 0);

	// BMP�E�B���h�E
	rect.bottom -= size.cy;
	m_pBMPWnd = new CSubBMPWnd;
	m_pBMPWnd->m_nScrlWidth = m_nScrlWidth;
	m_pBMPWnd->m_nScrlHeight = m_nScrlHeight;
	m_pBMPWnd->Create(NULL, NULL, WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL,
					rect, this, 0, NULL);

	return 0;
}

//---------------------------------------------------------------------------
//
//	�T�C�Y�ύX��
//
//---------------------------------------------------------------------------
void CSubVideoWnd::OnSizing(UINT nSide, LPRECT lpRect)
{
	CRect rect;
	CSize sizeBar;
	CRect rectSizing;

	// ��{�N���X
	CSubWnd::OnSizing(nSide, lpRect);

	// �X�e�[�^�X�o�[���Ȃ���΁A���^�[��
	if (!::IsWindow(m_StatusBar.m_hWnd)) {
		return;
	}

	// BMP�E�B���h�E�̍ő�T�C�Y�𓾂�(�X�N���[���o�[����)
	m_pBMPWnd->GetMaximumRect(&rect, TRUE);

	// �X�e�[�^�X�o�[�̃T�C�Y�𓾂āA���v
	sizeBar = m_StatusBar.CalcFixedLayout(TRUE, TRUE);
	rect.bottom += sizeBar.cy;

	// ���̃E�B���h�E�̍ő厞�̃T�C�Y�𓾂�
	CalcWindowRect(&rect);

	// �I�[�o�[�`�F�b�N
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
//	�T�C�Y�ύX
//
//---------------------------------------------------------------------------
void CSubVideoWnd::OnSize(UINT nType, int cx, int cy)
{
	CSize sizeBar;
	CRect rectClient;
	CRect rectBmp;
	CRect rectMax;
	CRect rectWnd;
	CSize sizeWnd;
	BOOL bReSize;

	ASSERT(this);
	ASSERT(cx >= 0);
	ASSERT(cy >= 0);

	// ��{�N���X
	CSubWnd::OnSize(nType, cx, cy);

	// �X�e�[�^�X�o�[�Ĕz�u(�E�B���h�E�L���̏ꍇ�Ɍ���)
	if (::IsWindow(m_StatusBar.m_hWnd)) {
		// �X�e�[�^�X�o�[�̍����A�N���C�A���g�̈�̍L���𓾂�
		sizeBar = m_StatusBar.CalcFixedLayout(TRUE, TRUE);
		GetClientRect(&rectClient);

		// �N���C�A���g�̈悪�A�X�e�[�^�X�o�[�����߂邽�߂ɏ\���ł���Έʒu�ύX
		if (rectClient.Height() > sizeBar.cy) {
			m_StatusBar.MoveWindow(0,
								rectClient.Height() - sizeBar.cy,
								rectClient.Width(),
								sizeBar.cy);

			// BMP�E�B���h�E�����킹�čĔz�u
			rectClient.bottom -= sizeBar.cy;
			m_pBMPWnd->MoveWindow(0, 0, rectClient.Width(), rectClient.Height());
		}

		// BMP�E�B���h�E�̃E�B���h�E�T�C�Y�A���e�ő�T�C�Y�𓾂�
		m_pBMPWnd->GetWindowRect(&rectBmp);
		m_pBMPWnd->GetMaximumRect(&rectMax, FALSE);

		// ���e�ő�T�C�Y�𒴂��Ă���΁A���ꂾ���k��(�X�N���[���o�[�̎���ON/OFF�ɑΏ�)
		GetWindowRect(&rectWnd);
		sizeWnd.cx = rectWnd.Width();
		sizeWnd.cy = rectWnd.Height();
		bReSize = FALSE;
		if (rectBmp.Width() > rectMax.Width()) {
			sizeWnd.cx -= (rectBmp.Width() - rectMax.Width());
			bReSize = TRUE;
		}
		if (rectBmp.Height() > rectMax.Height()) {
			sizeWnd.cy -= (rectBmp.Height() - rectMax.Height());
			bReSize = TRUE;
		}
		if (bReSize) {
			SetWindowPos(&wndTop, 0, 0, sizeWnd.cx, sizeWnd.cy, SWP_NOMOVE);
			return;
		}

		// �ǂ��炩�̃T�C�Y�𒴂��Ă���΁A�k��(�^�C���E�J�X�P�[�h�Ȃǂւ̑΍�)
		m_pBMPWnd->GetMaximumRect(&rectMax, TRUE);
		rectMax.bottom += sizeBar.cy;
		CalcWindowRect(&rectMax);
		GetWindowRect(&rectWnd);
		bReSize = FALSE;
		if (rectWnd.Width() >= rectMax.Width()) {
			rectWnd.right = rectWnd.left + rectMax.Width();
			bReSize = TRUE;
		}
		if (rectWnd.Height() >= rectMax.Height()) {
			rectWnd.bottom = rectWnd.top + rectMax.Height();
			bReSize = TRUE;
		}
		if (bReSize) {
			SetWindowPos(&wndTop, 0, 0, rectWnd.Width(), rectWnd.Height(), SWP_NOMOVE);
		}
	}
}

//---------------------------------------------------------------------------
//
//	�X�V
//
//---------------------------------------------------------------------------
void FASTCALL CSubVideoWnd::Refresh()
{
	CRect rect;

	ASSERT(this);
	ASSERT_VALID(this);

	// �L���t���O�`�F�b�N
	if (!m_bEnable || !m_pBMPWnd) {
		return;
	}

	// �`���`�擾
	m_pBMPWnd->GetDrawRect(&rect);
	if ((rect.Width() == 0) || (rect.Height() == 0)) {
		return;
	}

	// �Z�b�g�A�b�v
	Setup(rect, m_pBMPWnd->GetBits());

	// �\��
	m_pBMPWnd->Refresh();
}

//---------------------------------------------------------------------------
//
//	���b�Z�[�W�X���b�h����̍X�V
//
//---------------------------------------------------------------------------
void FASTCALL CSubVideoWnd::Update()
{
	ASSERT(this);
	ASSERT_VALID(this);
}

#if !defined(NDEBUG)
//---------------------------------------------------------------------------
//
//	�f�f
//
//---------------------------------------------------------------------------
void CSubVideoWnd::AssertValid() const
{
	ASSERT(this);

	// ��{�N���X
	CSubWnd::AssertValid();

	// ���z��ʃT�C�Y
	ASSERT(m_nScrlWidth > 0);
	ASSERT(m_nScrlHeight > 0);
}
#endif	// NDEBUG

//===========================================================================
//
//	�e�L�X�g��ʃE�B���h�E
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	�R���X�g���N�^
//
//---------------------------------------------------------------------------
CTVRAMWnd::CTVRAMWnd()
{
	// �E�B���h�E�p�����[�^��`
	m_dwID = MAKEID('T', 'V', 'R', 'M');
	::GetMsg(IDS_SWND_TVRAM, m_strCaption);

	// ���z��ʃT�C�Y�ݒ�
	m_nScrlWidth = 0x400;
	m_nScrlHeight = 0x400;

	// TVRAM�擾
	m_pTVRAM = (TVRAM*)::GetVM()->SearchDevice(MAKEID('T', 'V', 'R', 'M'));
	ASSERT(m_pTVRAM);
}

//---------------------------------------------------------------------------
//
//	�Z�b�g�A�b�v
//
//---------------------------------------------------------------------------
void FASTCALL CTVRAMWnd::Setup(CRect& rect, BYTE *pBits)
{
//	int x;
//	int y;
//	DWORD bufBit[0x400];


#if 0
	int i;
	const BYTE *p;
	DWORD buf[1024];
	int len;
	int below;
	int delta;

	ASSERT(x >= 0);
	ASSERT(y >= 0);
	ASSERT(width > 0);
	ASSERT(height > 0);
	ASSERT(ptr);

	// �p���b�g�`�F�b�N
	for (i=0; i<16; i++) {
		if (m_WndPalette[i] != m_pPalette[i]) {
			m_WndPalette[i] = m_pPalette[i];
			m_WndColor[i] = ConvPalette(m_pPalette[i]);
		}
	}

	// x, y�`�F�b�N
	if (x >= 1024) {
		// �\���̈�Ȃ��B���ׂč�
		for (i=0; i<height; i++) {
			memset(ptr, 0, (width << 2));
			ptr += (width << 2);
		}
		return;
	}
	if (y >= 1024) {
		// �\���̈�Ȃ��B���ׂč�
		for (i=0; i<height; i++) {
			memset(ptr, 0, (width << 2));
			ptr += (width << 2);
		}
		return;
	}

	// �|�C���^������
	p = m_pTVRAM;
	i = (x >> 5);
	p += (y << 7);
	p += (i << 2);

	// x�����I�t�Z�b�g�A�����_�����O��
	x &= 0x1f;
	len = x + width + 31;
	len >>= 5;

	// �I�[�o�[�΍�
	below = 0;
	if ((y + height) > 1024) {
		below = height - 1024 + y;
		height = 1024 - y;
	}
	delta = 0;
	if ((len + i) > 32) {
		len = 32 - i;
		delta = width - (len << 5);
		width = (len << 5);
	}

	// �����_�����O���[�v
	for (i=0; i<height; i++) {
		::VideoText(p, buf, len, m_WndColor);
		p += 128;

		// x, width�����Ă��ăR�s�[
		memcpy(ptr, &buf[x], (width << 2));
		ptr += (width << 2);
		memset(ptr, 0, (delta << 2));
		ptr += (delta << 2);
	}

	// �]�v�ȉ�����������
	for (i=0; i<below; i++) {
		memset(ptr, 0, (width << 2));
		ptr += (width << 2);
		memset(ptr, 0, (delta << 2));
		ptr += (delta << 2);
	}
#endif
}

//---------------------------------------------------------------------------
//
//	���b�Z�[�W�X���b�h����̍X�V
//
//---------------------------------------------------------------------------
void FASTCALL CTVRAMWnd::Update()
{
	ASSERT(this);
	ASSERT_VALID(this);

	m_StatusBar.SetPaneText(0, "ABC", TRUE);
#if 0
	int x;
	int y;
	DWORD addr;
	CString string;
	BYTE p[4];
	int value;
	int bit;
	int r;
	int g;
	int b;

	// BMP�E�B���h�E�`�F�b�N
	if (!m_pBMPWnd) {
		return;
	}

	// �}�E�X�J�[�\���`�F�b�N
	if ((m_pBMPWnd->m_nCursorX < 0) || (m_pBMPWnd->m_nCursorY < 0)) {
		m_StatusBar.SetPaneText(0, "");
		return;
	}

	// ���W�v�Z�A�I�[�o�[�`�F�b�N
	x = m_pBMPWnd->m_nCursorX + m_pBMPWnd->m_nScrlX;
	y = m_pBMPWnd->m_nCursorY + m_pBMPWnd->m_nScrlY;
	if (x >= m_nScrlWidth) {
		return;
	}
	if (y >= m_nScrlHeight) {
		return;
	}

	// �f�[�^�擾
	addr = ((y << 7) + (x >> 3)) ^ 1;
	p[0] = m_pTVRAM[addr + 0x00000];
	p[1] = m_pTVRAM[addr + 0x20000];
	p[2] = m_pTVRAM[addr + 0x40000];
	p[3] = m_pTVRAM[addr + 0x60000];
	addr = (addr ^ 1) + 0xe00000;

	// RGB�l�擾
	value = (x & 0x07);
	bit = (0x80 >> value);
	value = 0;
	if (p[0] & bit) {
		value |= 0x01;
	}
	if (p[1] & bit) {
		value |= 0x02;
	}
	if (p[2] & bit) {
		value |= 0x04;
	}
	if (p[3] & bit) {
		value |= 0x08;
	}
	r = m_WndColor[value] >> 16;
	g = (m_WndColor[value] >> 8) & 0xff;
	b = m_WndColor[value] & 0xff;

	// �t�H�[�}�b�g�A�Z�b�g
	string.Format("( %d, %d) $%06X [%02X %02X %02X %02X] Color%d R%d G%d B%d",
				x, y, addr,
				p[0], p[1], p[2], p[3],
				value, r, g, b);
	m_StatusBar.SetPaneText(0, string);
#endif
}

//===========================================================================
//
//	�O���t�B�b�N���(1024�~1024)�E�B���h�E
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	�R���X�g���N�^
//
//---------------------------------------------------------------------------
CG1024Wnd::CG1024Wnd()
{
	VC *pVC;
	GVRAM *pGVRAM;
	int i;

	// �E�B���h�E�p�����[�^��`
	m_dwID = MAKEID('G', '1', '0', '2');
	::GetMsg(IDS_SWND_G1024, m_strCaption);

	// ���z��ʃT�C�Y
	m_nScrlWidth = 1024;
	m_nScrlHeight = 1024;

	// �p���b�g�������擾
	pVC = (VC*)::GetVM()->SearchDevice(MAKEID('V', 'C', ' ', ' '));
	ASSERT(pVC);
	m_pPalette = (WORD*)pVC->GetPalette();

	// �O���t�B�b�NVRAM�������擾
	pGVRAM = (GVRAM*)::GetVM()->SearchDevice(MAKEID('G', 'V', 'R', 'M'));
	ASSERT(pGVRAM);
	m_pGVRAM = pGVRAM->GetGVRAM();

	// �p���b�g�f�[�^������
	for (i=0; i<16; i++) {
		m_WndPalette[i] = 0;
		m_WndColor[i] = 0;
	}
}

//---------------------------------------------------------------------------
//
//	�Z�b�g�A�b�v
//
//---------------------------------------------------------------------------
void FASTCALL CG1024Wnd::Setup(int x, int y, int width, int height, BYTE *ptr)
{
	int i;
	DWORD buf[1024];

	ASSERT(x >= 0);
	ASSERT(y >= 0);
	ASSERT(width > 0);
	ASSERT(height > 0);
	ASSERT(ptr);

	// �p���b�g�`�F�b�N
	for (i=0; i<16; i++) {
		if (m_WndPalette[i] != m_pPalette[i]) {
			m_WndPalette[i] = m_pPalette[i];
			m_WndColor[i] = ConvPalette(m_pPalette[i]);
		}
	}

	for (i=0; i<height; i++) {
		if (y < 512) {
			::VideoG1024A(&m_pGVRAM[y << 10], buf, m_WndColor);
		}
		else {
			::VideoG1024B(&m_pGVRAM[(y & 0x1ff) << 10], buf, m_WndColor);
		}
		memcpy(ptr, &buf[x], (width << 2));
		ptr += (width << 2);
		y++;
	}
}

//---------------------------------------------------------------------------
//
//	���b�Z�[�W�X���b�h����̍X�V
//
//---------------------------------------------------------------------------
void FASTCALL CG1024Wnd::Update()
{
	int x;
	int y;
	DWORD addr;
	CString string;
	int value;
	int r;
	int g;
	int b;

	// BMP�E�B���h�E�`�F�b�N
	if (!m_pBMPWnd) {
		return;
	}

	// �}�E�X�J�[�\���`�F�b�N
	if ((m_pBMPWnd->m_nCursorX < 0) || (m_pBMPWnd->m_nCursorY < 0)) {
		m_StatusBar.SetPaneText(0, "");
		return;
	}

	// ���W�v�Z�A�I�[�o�[�`�F�b�N
	x = m_pBMPWnd->m_nCursorX + m_pBMPWnd->m_nScrlX;
	y = m_pBMPWnd->m_nCursorY + m_pBMPWnd->m_nScrlY;
	if (x >= m_nScrlWidth) {
		return;
	}
	if (y >= m_nScrlHeight) {
		return;
	}

	// �f�[�^�擾
	addr = (y & 0x1ff);
	addr <<= 10;
	addr += ((x & 0x1ff) << 1);
	if (y < 512) {
		if (x < 512) {
			value = m_pGVRAM[addr] & 0x0f;
			addr += 0xc00000;
		}
		else {
			value = m_pGVRAM[addr] >> 4;
			addr += 0xc80000;
		}
	}
	else {
		if (x < 512) {
			value = m_pGVRAM[addr ^ 1] & 0x0f;
			addr += 0xd00000;
		}
		else {
			value = m_pGVRAM[addr ^ 1] >> 4;
			addr += 0xd80000;
		}
	}

	// RGB�l�擾
	r = m_WndColor[value] >> 16;
	g = (m_WndColor[value] >> 8) & 0xff;
	b = m_WndColor[value] & 0xff;

	// �t�H�[�}�b�g�A�Z�b�g
	string.Format("( %d, %d) $%06X [%02X] Color%d R%d G%d B%d",
				x, y, addr,
				value, value, r, g, b);
	m_StatusBar.SetPaneText(0, string);
}

//===========================================================================
//
//	�O���t�B�b�N���(16�F)�E�B���h�E
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	�R���X�g���N�^
//
//---------------------------------------------------------------------------
CG16Wnd::CG16Wnd(int nPage)
{
	VC *pVC;
	GVRAM *pGVRAM;
	int i;

	// �y�[�W�L��
	ASSERT((nPage >= 0) || (nPage <= 3));
	m_nPage = nPage;

	// �E�B���h�E�p�����[�^��`
	m_dwID = MAKEID('G', '1', '6', ('A' + nPage));
	::GetMsg((IDS_SWND_G16P0 + nPage), m_strCaption);

	// ���z��ʃT�C�Y
	m_nScrlWidth = 512;
	m_nScrlHeight = 512;

	// �p���b�g�������擾
	pVC = (VC*)::GetVM()->SearchDevice(MAKEID('V', 'C', ' ', ' '));
	ASSERT(pVC);
	m_pPalette = (WORD*)pVC->GetPalette();

	// �O���t�B�b�NVRAM�������擾
	pGVRAM = (GVRAM*)::GetVM()->SearchDevice(MAKEID('G', 'V', 'R', 'M'));
	ASSERT(pGVRAM);
	m_pGVRAM = pGVRAM->GetGVRAM();

	// �p���b�g�f�[�^������
	for (i=0; i<16; i++) {
		m_WndPalette[i] = 0;
		m_WndColor[i] = 0;
	}
}

//---------------------------------------------------------------------------
//
//	�Z�b�g�A�b�v
//
//---------------------------------------------------------------------------
void FASTCALL CG16Wnd::Setup(int x, int y, int width, int height, BYTE *ptr)
{
	int i;
	const BYTE *p;
	DWORD buf[1024];
	int delta;

	ASSERT(x >= 0);
	ASSERT(y >= 0);
	ASSERT(width > 0);
	ASSERT(height > 0);
	ASSERT(ptr);

	// �p���b�g�`�F�b�N
	for (i=0; i<16; i++) {
		if (m_WndPalette[i] != m_pPalette[i]) {
			m_WndPalette[i] = m_pPalette[i];
			m_WndColor[i] = ConvPalette(m_pPalette[i]);
		}
	}

	// �|�C���^������
	p = m_pGVRAM;
	p += (x << 1);
	p += (y << 10);

	// 512�~512�Ȃ̂ŁA�I�[�o�[����\��������
	if ((y + height) > m_nScrlHeight) {
		height = m_nScrlHeight - y;
	}
	delta = 0;
	if ((x + width) > m_nScrlWidth) {
		delta = width - m_nScrlWidth + x;
		width = m_nScrlWidth - x;
	}

	// �����_�����O���[�v
	switch (m_nPage) {
		// �y�[�W0
		case 0:
			for (i=0; i<height; i++) {
				::VideoG16A(p, buf, width, m_WndColor);
				p += 1024;
				// x, width�����Ă��ăR�s�[
				memcpy(ptr, buf, (width << 2));
				ptr += (width << 2);
				ptr += (delta << 2);
			}
			break;
		// �y�[�W1
		case 1:
			for (i=0; i<height; i++) {
				::VideoG16B(p, buf, width, m_WndColor);
				p += 1024;
				// x, width�����Ă��ăR�s�[
				memcpy(ptr, buf, (width << 2));
				ptr += (width << 2);
				ptr += (delta << 2);
			}
			break;
		// �y�[�W2
		case 2:
			for (i=0; i<height; i++) {
				::VideoG16C(p, buf, width, m_WndColor);
				p += 1024;
				// x, width�����Ă��ăR�s�[
				memcpy(ptr, buf, (width << 2));
				ptr += (width << 2);
				ptr += (delta << 2);
			}
			break;
		// �y�[�W3
		case 3:
			for (i=0; i<height; i++) {
				::VideoG16D(p, buf, width, m_WndColor);
				p += 1024;
				// x, width�����Ă��ăR�s�[
				memcpy(ptr, buf, (width << 2));
				ptr += (width << 2);
				ptr += (delta << 2);
			}
			break;
	}
}

//---------------------------------------------------------------------------
//
//	���b�Z�[�W�X���b�h����̍X�V
//
//---------------------------------------------------------------------------
void FASTCALL CG16Wnd::Update()
{
	int x;
	int y;
	DWORD addr;
	CString string;
	int value;
	int r;
	int g;
	int b;

	// BMP�E�B���h�E�`�F�b�N
	if (!m_pBMPWnd) {
		return;
	}

	// �}�E�X�J�[�\���`�F�b�N
	if ((m_pBMPWnd->m_nCursorX < 0) || (m_pBMPWnd->m_nCursorY < 0)) {
		m_StatusBar.SetPaneText(0, "");
		return;
	}

	// ���W�v�Z�A�I�[�o�[�`�F�b�N
	x = m_pBMPWnd->m_nCursorX + m_pBMPWnd->m_nScrlX;
	y = m_pBMPWnd->m_nCursorY + m_pBMPWnd->m_nScrlY;
	if (x >= m_nScrlWidth) {
		return;
	}
	if (y >= m_nScrlHeight) {
		return;
	}

	// �f�[�^�擾
	addr = ((y << 10) + (x << 1));
	switch (m_nPage) {
		case 0:
			value = m_pGVRAM[addr] & 0x0f;
			addr += 0xc00000;
			break;
		case 1:
			value = m_pGVRAM[addr] >> 4;
			addr += 0xc80000;
			break;
		case 2:
			value = m_pGVRAM[addr ^ 1] & 0x0f;
			addr += 0xd00000;
			break;
		case 3:
			value = m_pGVRAM[addr ^ 1] >> 4;
			addr += 0xd80000;
			break;
	}

	// RGB�l�擾
	r = m_WndColor[value] >> 16;
	g = (m_WndColor[value] >> 8) & 0xff;
	b = m_WndColor[value] & 0xff;

	// �t�H�[�}�b�g�A�Z�b�g
	string.Format("( %d, %d) $%06X [%02X] Color%d R%d G%d B%d",
				x, y, addr,
				value, value, r, g, b);
	m_StatusBar.SetPaneText(0, string);
}

//===========================================================================
//
//	�O���t�B�b�N���(256�F)�E�B���h�E
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	�R���X�g���N�^
//
//---------------------------------------------------------------------------
CG256Wnd::CG256Wnd(int nPage)
{
	VC *pVC;
	GVRAM *pGVRAM;
	int i;

	// �y�[�W�L��
	ASSERT((nPage == 0) || (nPage == 1));
	m_nPage = nPage;

	// �E�B���h�E�p�����[�^��`
	m_dwID = MAKEID('G', '2', '5', ('A' + nPage));
	::GetMsg((IDS_SWND_G256P0 + nPage), m_strCaption);

	// ���z��ʃT�C�Y
	m_nScrlWidth = 512;
	m_nScrlHeight = 512;

	// �p���b�g�������擾
	pVC = (VC*)::GetVM()->SearchDevice(MAKEID('V', 'C', ' ', ' '));
	ASSERT(pVC);
	m_pPalette = (WORD*)pVC->GetPalette();

	// �O���t�B�b�NVRAM�������擾
	pGVRAM = (GVRAM*)::GetVM()->SearchDevice(MAKEID('G', 'V', 'R', 'M'));
	ASSERT(pGVRAM);
	m_pGVRAM = pGVRAM->GetGVRAM();

	// �p���b�g�f�[�^������
	for (i=0; i<256; i++) {
		m_WndPalette[i] = 0;
		m_WndColor[i] = 0;
	}
}

//---------------------------------------------------------------------------
//
//	�Z�b�g�A�b�v
//
//---------------------------------------------------------------------------
void FASTCALL CG256Wnd::Setup(int x, int y, int width, int height, BYTE *ptr)
{
	int i;
	const BYTE *p;
	DWORD buf[512];
	int delta;

	// assert
	ASSERT(x >= 0);
	ASSERT(y >= 0);
	ASSERT(width > 0);
	ASSERT(height > 0);
	ASSERT(ptr);

	// �p���b�g�`�F�b�N
	for (i=0; i<256; i++) {
		if (m_WndPalette[i] != m_pPalette[i]) {
			m_WndPalette[i] = m_pPalette[i];
			m_WndColor[i] = ConvPalette(m_pPalette[i]);
		}
	}

	// �|�C���^������
	p = m_pGVRAM;
	p += (y << 10);
	p += (x << 1);

	// 512�~512�Ȃ̂ŁA�I�[�o�[����\��������
	if ((y + height) > 512) {
		height = 512 - y;
	}
	delta = 0;
	if ((x + width) > 512) {
		delta = width - 512 + x;
		width = 512 - x;
	}

	// �����_�����O���[�v
	if (m_nPage == 0) {
		for (i=0; i<height; i++) {
			::VideoG256A(p, buf, width, m_WndColor);
			p += 1024;

			// x, width�����Ă��ăR�s�[
			memcpy(ptr, buf, (width << 2));
			ptr += (width << 2);
			ptr += (delta << 2);
		}
	}
	else {
		for (i=0; i<height; i++) {
			::VideoG256B(p, buf, width, m_WndColor);
			p += 1024;

			// x, width�����Ă��ăR�s�[
			memcpy(ptr, buf, (width << 2));
			ptr += (width << 2);
			ptr += (delta << 2);
		}
	}
}

//---------------------------------------------------------------------------
//
//	���b�Z�[�W�X���b�h����̍X�V
//
//---------------------------------------------------------------------------
void FASTCALL CG256Wnd::Update()
{
	int x;
	int y;
	DWORD addr;
	CString string;
	int value;
	int r;
	int g;
	int b;

	// BMP�E�B���h�E�`�F�b�N
	if (!m_pBMPWnd) {
		return;
	}

	// �}�E�X�J�[�\���`�F�b�N
	if ((m_pBMPWnd->m_nCursorX < 0) || (m_pBMPWnd->m_nCursorY < 0)) {
		m_StatusBar.SetPaneText(0, "");
		return;
	}

	// ���W�v�Z�A�I�[�o�[�`�F�b�N
	x = m_pBMPWnd->m_nCursorX + m_pBMPWnd->m_nScrlX;
	y = m_pBMPWnd->m_nCursorY + m_pBMPWnd->m_nScrlY;
	if (x >= m_nScrlWidth) {
		return;
	}
	if (y >= m_nScrlHeight) {
		return;
	}

	// �f�[�^�擾(256�F��GRAM�z�u���O��)
	addr = (y << 9) + x;
	addr <<= 1;
	if (m_nPage > 0) {
		value = m_pGVRAM[addr ^ 1];
		addr += 0xc80000;
	}
	else {
		value = m_pGVRAM[addr];
		addr += 0xc00000;
	}

	// RGB�l�擾
	r = m_WndColor[value] >> 16;
	g = (m_WndColor[value] >> 8) & 0xff;
	b = m_WndColor[value] & 0xff;

	// �t�H�[�}�b�g�A�Z�b�g
	string.Format("( %d, %d) $%06X [%02X] Color%d R%d G%d B%d",
				x, y, addr,
				value, value, r, g, b);
	m_StatusBar.SetPaneText(0, string);
}

//===========================================================================
//
//	�O���t�B�b�N���(65536�F)�E�B���h�E
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	�R���X�g���N�^
//
//---------------------------------------------------------------------------
CG64KWnd::CG64KWnd()
{
	VC *pVC;
	GVRAM *pGVRAM;
	int i;

	// �E�B���h�E�p�����[�^��`
	m_dwID = MAKEID('G', '6', '4', 'K');
	::GetMsg(IDS_SWND_G64K, m_strCaption);

	// ���z��ʃT�C�Y
	m_nScrlWidth = 512;
	m_nScrlHeight = 512;

	// �p���b�g�������擾
	pVC = (VC*)::GetVM()->SearchDevice(MAKEID('V', 'C', ' ', ' '));
	ASSERT(pVC);
	m_pPalette = (WORD*)pVC->GetPalette();

	// �O���t�B�b�NVRAM�������擾
	pGVRAM = (GVRAM*)::GetVM()->SearchDevice(MAKEID('G', 'V', 'R', 'M'));
	ASSERT(pGVRAM);
	m_pGVRAM = pGVRAM->GetGVRAM();

	// �p���b�g�f�[�^������
	for (i=0; i<256; i++) {
		m_WndPalette[i] = 0;
		m_WndColor[i] = 0;
	}
}

//---------------------------------------------------------------------------
//
//	�Z�b�g�A�b�v
//
//---------------------------------------------------------------------------
void FASTCALL CG64KWnd::Setup(int x, int y, int width, int height, BYTE *ptr)
{
	BOOL flag;
	int i;
	const BYTE *p;
	DWORD buf[512];
	int delta;

	// assert
	ASSERT(x >= 0);
	ASSERT(y >= 0);
	ASSERT(width > 0);
	ASSERT(height > 0);
	ASSERT(ptr);

	// �p���b�g�`�F�b�N
	flag = FALSE;
	for (i=0; i<256; i++) {
		if (m_WndPalette[i] != m_pPalette[i]) {
			m_WndPalette[i] = m_pPalette[i];
			flag = TRUE;
		}
	}

	// �p���b�g�č\��
	if (flag) {
		Palette();
	}

	// �|�C���^������
	p = m_pGVRAM;
	p += (y << 10);
	p += (x << 1);

	// 512�~512�Ȃ̂ŁA�I�[�o�[����\��������
	if ((y + height) > 512) {
		height = 512 - y;
	}
	delta = 0;
	if ((x + width) > 512) {
		delta = width - 512 + x;
		width = 512 - x;
	}

	// �����_�����O���[�v
	for (i=0; i<height; i++) {
		// �����_�����O
		::VideoG64K(p, buf, width, m_WndColor);
		p += 1024;

		// x, width�����Ă��ăR�s�[
		memcpy(ptr, buf, (width << 2));
		ptr += (width << 2);
		ptr += (delta << 2);
	}
}

//---------------------------------------------------------------------------
//
//	�p���b�g�č\��
//
//---------------------------------------------------------------------------
void FASTCALL CG64KWnd::Palette()
{
	const WORD *p;
	const WORD *q;
	DWORD *r;
	WORD hi;
	WORD lo;
	int i;
	int j;

	// �X�g�A���������
	r = m_WndColor;

	// ���[�v1
	p = m_pPalette + 1;
	for (i=0; i<256; i++) {
		// hi�f�[�^�擾
		hi = *p;
		if (i & 1) {
			hi <<= 8;
			p += 2;
		}
		else {
			hi &= 0xff00;
		}

		// ���[�v2
		q = m_pPalette;
		for (j=0; j<256; j++) {
			// lo�f�[�^�擾
			lo = *q;
			if (j & 1) {
				lo &= 0x00ff;
				q += 2;
			}
			else {
				lo >>= 8;
			}

			// �ϊ����X�g�A
			lo |= hi;
			*r++ = ConvPalette(lo);
		}
	}
}

//---------------------------------------------------------------------------
//
//	���b�Z�[�W�X���b�h����̍X�V
//
//---------------------------------------------------------------------------
void FASTCALL CG64KWnd::Update()
{
	int x;
	int y;
	DWORD addr;
	CString string;
	int value;
	int r;
	int g;
	int b;

	// BMP�E�B���h�E�`�F�b�N
	if (!m_pBMPWnd) {
		return;
	}

	// �}�E�X�J�[�\���`�F�b�N
	if ((m_pBMPWnd->m_nCursorX < 0) || (m_pBMPWnd->m_nCursorY < 0)) {
		m_StatusBar.SetPaneText(0, "");
		return;
	}

	// ���W�v�Z�A�I�[�o�[�`�F�b�N
	x = m_pBMPWnd->m_nCursorX + m_pBMPWnd->m_nScrlX;
	y = m_pBMPWnd->m_nCursorY + m_pBMPWnd->m_nScrlY;
	if (x >= m_nScrlWidth) {
		return;
	}
	if (y >= m_nScrlHeight) {
		return;
	}

	// �f�[�^�擾(64K�F��GRAM�z�u���O��)
	addr = ((y << 10) + (x << 1));
	value = m_pGVRAM[addr + 1];
	value <<= 8;
	value |= m_pGVRAM[addr];
	addr += 0xc00000;

	// RGB�l�擾
	r = m_WndColor[value] >> 16;
	g = (m_WndColor[value] >> 8) & 0xff;
	b = m_WndColor[value] & 0xff;

	// �t�H�[�}�b�g�A�Z�b�g
	string.Format("( %d, %d) $%06X [%04X] Color%d R%d G%d B%d",
				x, y, addr,
				value, value, r, g, b);
	m_StatusBar.SetPaneText(0, string);
}

//===========================================================================
//
//	PCG�E�B���h�E
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	�R���X�g���N�^
//
//---------------------------------------------------------------------------
CPCGWnd::CPCGWnd()
{
	VC *pVC;
	Sprite *pSprite;
    int i;

	// �E�B���h�E�p�����[�^��`
	m_dwID = MAKEID('P', 'C', 'G', ' ');
	::GetMsg(IDS_SWND_PCG, m_strCaption);

	// �E�B���h�E�����T�C�Y
	m_nWidth = 28;
	m_nHeight = 16;

	// ���z��ʃT�C�Y
	m_nScrlWidth = 256;
	m_nScrlHeight = 256;

	// �p���b�g�������擾
	pVC = (VC*)::GetVM()->SearchDevice(MAKEID('V', 'C', ' ', ' '));
	ASSERT(pVC);
	m_pPalette = (WORD*)pVC->GetPalette();
	m_pPalette += 256;

	// PCG�������擾
	pSprite = (Sprite*)::GetVM()->SearchDevice(MAKEID('S', 'P', 'R', ' '));
	ASSERT(pSprite);
	m_pPCG = pSprite->GetPCG();

	// �p���b�g�f�[�^������
	for (i=0; i<256; i++) {
		m_WndPalette[i] = 0;
		m_WndColor[i] = 0;
	}

	// �J���[�u���b�N1
	m_nColor = 1;
}

//---------------------------------------------------------------------------
//
//	���b�Z�[�W �}�b�v
//
//---------------------------------------------------------------------------
BEGIN_MESSAGE_MAP(CPCGWnd, CSubBitmapWnd)
	ON_WM_PARENTNOTIFY()
	ON_WM_CONTEXTMENU()
	ON_COMMAND_RANGE(IDM_PCG_0, IDM_PCG_F, OnPalette)
END_MESSAGE_MAP()

//---------------------------------------------------------------------------
//
//	�Z�b�g�A�b�v
//
//---------------------------------------------------------------------------
void FASTCALL CPCGWnd::Setup(int x, int y, int width, int height, BYTE *ptr)
{
	int i;
	int delta;
	int addr;
	DWORD buf[256];

	// assert
	ASSERT(x >= 0);
	ASSERT(y >= 0);
	ASSERT(width > 0);
	ASSERT(height > 0);
	ASSERT(ptr);

	// �p���b�g�`�F�b�N
	for (i=0; i<256; i++) {
		if (m_WndPalette[i] != m_pPalette[i]) {
			m_WndPalette[i] = m_pPalette[i];
			m_WndColor[i] = ConvPalette(m_pPalette[i]);
		}
	}

	// 256�~256�Ȃ̂ŁA�I�[�o�[����\��������
	if ((y + height) > 256) {
		height = 256 - y;
	}
	delta = 0;
	if ((x + width) > 256) {
		delta = width - 256 + x;
		width = 256 - x;
	}

	// �����_�����O���[�v
	for (i=0; i<height; i++) {
		// y����A�A�h���X�����߂�
		addr = y >> 4;
		addr <<= 11;
		if (y & 8) {
			addr += 0x0020;
		}
		addr += ((y & 7) << 2);

		// �����_�����O
		::VideoPCG((BYTE*)&m_pPCG[addr], buf, &m_WndColor[m_nColor << 4]);

		// x, width�����Ă��ăR�s�[
		memcpy(ptr, &buf[x], (width << 2));
		ptr += (width << 2);
		ptr += (delta << 2);

		// ����y��
		y++;
	}
}

//---------------------------------------------------------------------------
//
//	���b�Z�[�W�X���b�h����̍X�V
//
//---------------------------------------------------------------------------
void FASTCALL CPCGWnd::Update()
{
	int x;
	int y;
	CString string;
	int addr;
	int base;
	int color;
	int value;
	int r;
	int g;
	int b;

	// BMP�E�B���h�E�`�F�b�N
	if (!m_pBMPWnd) {
		return;
	}

	// �}�E�X�J�[�\���`�F�b�N
	if ((m_pBMPWnd->m_nCursorX < 0) || (m_pBMPWnd->m_nCursorY < 0)) {
		m_StatusBar.SetPaneText(0, "");
		return;
	}

	// ���W�v�Z�A�I�[�o�[�`�F�b�N
	x = m_pBMPWnd->m_nCursorX + m_pBMPWnd->m_nScrlX;
	y = m_pBMPWnd->m_nCursorY + m_pBMPWnd->m_nScrlY;
	if (x >= m_nScrlWidth) {
		return;
	}
	if (y >= m_nScrlHeight) {
		return;
	}

	// �A�h���X�v�Z
	addr = y >> 4;
	addr <<= 11;
	if (y & 8) {
		addr += 0x0020;
	}
	addr += ((y & 7) << 2);
	addr += ((x >> 3) << 6);
	addr += ((x & 7) >> 1);
	base = addr;

	// �A�h���X�v�Z
	addr += 0xeb8000;

	// �J���[�擾
	color = m_pPCG[base ^ 1];
	if ((x & 1) == 0) {
		color >>= 4;
	}
	color &= 0x0f;

	// RGB�擾
	value = (m_nColor << 4) + color;
	r = m_WndColor[value] >> 16;
	g = (m_WndColor[value] >> 8) & 0xff;
	b = m_WndColor[value] & 0xff;

	// �t�H�[�}�b�g�A�Z�b�g
	string.Format("( %d, %d) $%06X Pal%1X [$%02X +%d +%d] Color%d R%d G%d B%d",
				x, y, addr, m_nColor,
				(y & 0xf0) + (x >> 4), (x & 0x0f), (y & 0x0f),
				color, r, g, b);
	m_StatusBar.SetPaneText(0, string);
}

//---------------------------------------------------------------------------
//
//	�e�E�B���h�E�֒ʒm
//
//---------------------------------------------------------------------------
void CPCGWnd::OnParentNotify(UINT message, LPARAM lParam)
{
	POINT point;
	CRect rect;

	if (message == WM_LBUTTONDOWN) {
		// �N���C�A���g���Ȃ�A�p���b�g�ύX
		::GetCursorPos(&point);
		m_pBMPWnd->GetClientRect(&rect);
		m_pBMPWnd->ClientToScreen(&rect);
		if (rect.PtInRect(point)) {
			m_nColor = (m_nColor + 1) & 0x0f;
		}
	}

	// ��{�N���X
	CSubBitmapWnd::OnParentNotify(message, lParam);
}

//---------------------------------------------------------------------------
//
//	�R���e�L�X�g���j���[
//
//---------------------------------------------------------------------------
void CPCGWnd::OnContextMenu(CWnd *pWnd, CPoint point)
{
	CRect rect;
	CMenu menu;
	CMenu *pMenu;

	// �N���C�A���g�̈���ŉ����ꂽ�����肷��
	GetClientRect(&rect);
	ClientToScreen(&rect);
	if (!rect.PtInRect(point)) {
		CSubBitmapWnd::OnContextMenu(pWnd, point);
		return;
	}

	// ���j���[���s
	menu.LoadMenu(IDR_PCGMENU);
	pMenu = menu.GetSubMenu(0);
	pMenu->CheckMenuItem(m_nColor, MF_CHECKED | MF_BYPOSITION);
	pMenu->TrackPopupMenu(TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_RIGHTBUTTON,
							point.x, point.y, this);
}

//---------------------------------------------------------------------------
//
//	�p���b�g�ύX
//
//---------------------------------------------------------------------------
void CPCGWnd::OnPalette(UINT uID)
{
	uID -= IDM_PCG_0;
	ASSERT(uID < 0x10);

	m_nColor = (int)uID;
}

//===========================================================================
//
//	BG�E�B���h�E
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	�R���X�g���N�^
//
//---------------------------------------------------------------------------
CBGWnd::CBGWnd(int nPage)
{
	VC *pVC;
	int i;

	ASSERT((nPage == 0) || (nPage == 1));

	// �y�[�W�L��
	m_nPage = nPage;

	// �E�B���h�E�p�����[�^��`
	m_dwID = MAKEID('B', 'G', ('0' + nPage), ' ');
	::GetMsg(IDS_SWND_BG0 + nPage, m_strCaption);

	// ���z��ʃT�C�Y
	if (nPage == 0) {
		m_nScrlWidth = 1024;
		m_nScrlHeight = 1024;
	}
	else {
		m_nScrlWidth = 512;
		m_nScrlHeight = 512;
	}

	// �p���b�g�������擾
	pVC = (VC*)::GetVM()->SearchDevice(MAKEID('V', 'C', ' ', ' '));
	ASSERT(pVC);
	m_pPalette = (WORD*)pVC->GetPalette();
	m_pPalette += 256;

	// �p���b�g�f�[�^������
	for (i=0; i<256; i++) {
		m_WndPalette[i] = 0;
		m_WndColor[i] = 0;
	}

	// �X�v���C�g�擾
	m_pSprite = (Sprite*)::GetVM()->SearchDevice(MAKEID('S', 'P', 'R', ' '));
	ASSERT(m_pSprite);
}

//---------------------------------------------------------------------------
//
//	�Z�b�g�A�b�v
//
//---------------------------------------------------------------------------
void FASTCALL CBGWnd::Setup(int x, int y, int width, int height, BYTE *ptr)
{
	DWORD buf[1024];
	DWORD *q;
	BYTE *pcg;
	Sprite::sprite_t spr;
	int delta;
	int size;
	int offset;
	int i;
	int j;

	// assert
	ASSERT(x >= 0);
	ASSERT(y >= 0);
	ASSERT(width > 0);
	ASSERT(height > 0);
	ASSERT(ptr);

	// �X�v���C�g�f�[�^�擾�APCG�v�Z
	m_pSprite->GetSprite(&spr);
	pcg = &spr.mem[0x8000];

	// BG1�̏ꍇ
	if (m_nPage != 0) {
		// BG�T�C�Y��16�~16�Ȃ�\�����Ȃ�
		if (spr.bg_size) {
			memset(ptr, 0, (width * height) << 2);
			return;
		}
	}

	// �ŏ��ɃN���A
	memset(buf, 0, sizeof(buf));
	memset(ptr, 0, (width * height) << 2);

	// �T�C�Y�m��
	if (spr.bg_size) {
		ASSERT(m_nPage == 0);
		size = 1024;
	}
	else {
		size = 512;
	}

	// �p���b�g�`�F�b�N
	for (i=0; i<256; i++) {
		if (m_WndPalette[i] != m_pPalette[i]) {
			m_WndPalette[i] = m_pPalette[i];
			m_WndColor[i] = ConvPalette(m_pPalette[i]);
		}
	}

	// ��s�I�[�o�[�`�F�b�N
	if (y > size) {
		return;
	}
	if (x > size) {
		return;
	}

	// �I�[�o�[�`�F�b�N
	if ((y + height) > size) {
		height = size - y;
	}
	delta = 0;
	if ((x + width) > size) {
		delta = width - size + x;
		width = size - x;
	}

	// �����_�����O���[�v
	for (i=0; i<height; i++) {
		// y����BG�f�[�^�̃I�t�Z�b�g�𓾂�
		if (spr.bg_size) {
			// 16x16
			offset = (y >> 4);
		}
		else {
			// 8x8
			offset = (y >> 3);
		}
		// ��������64BG�A�܂�128�o�C�g
		offset <<= 7;
		// BG�f�[�^�G���A0��+C000�ABG�f�[�^�G���A1��+E000
		offset += ((spr.bg_area[m_nPage] & 0x01) << 13);
		offset += 0xc000;

		// X�������[�v
		q = buf;
		for (j=0; j<64; j++) {
			// �A�Z���u���T�u
			if (spr.bg_size) {
				::VideoBG16(pcg, q, *(WORD*)&spr.mem[offset],
											y & 0x0f, m_WndColor);
				q += 16;
			}
			else {
				::VideoBG8(pcg, q, *(WORD*)&spr.mem[offset],
											y & 0x07, m_WndColor);
				q += 8;
			}
			offset += 2;
		}

		// x, width�����Ă��ăR�s�[
		memcpy(ptr, &buf[x], (width << 2));
		ptr += (width << 2);
		ptr += (delta << 2);

		// ����y��
		y++;
	}
}

//---------------------------------------------------------------------------
//
//	���b�Z�[�W�X���b�h����̍X�V
//
//---------------------------------------------------------------------------
void FASTCALL CBGWnd::Update()
{
	CString string;
	Sprite::sprite_t spr;
	DWORD addr;
	WORD data;
	int x;
	int y;

	// �X�v���C�g�f�[�^�擾�APCG�v�Z
	m_pSprite->GetSprite(&spr);

	// BMP�E�B���h�E�`�F�b�N
	if (!m_pBMPWnd) {
		return;
	}

	// �}�E�X�J�[�\���`�F�b�N
	if ((m_pBMPWnd->m_nCursorX < 0) || (m_pBMPWnd->m_nCursorY < 0)) {
		m_StatusBar.SetPaneText(0, "");
		return;
	}

	// ���W�v�Z�A�I�[�o�[�`�F�b�N
	x = m_pBMPWnd->m_nCursorX + m_pBMPWnd->m_nScrlX;
	y = m_pBMPWnd->m_nCursorY + m_pBMPWnd->m_nScrlY;
	if (x >= m_nScrlWidth) {
		return;
	}
	if (y >= m_nScrlHeight) {
		return;
	}

	// �y�[�W0��1024�~1024�E�B���h�E�ŁA512�~512�����g��Ȃ��ꍇ����
	if ((m_nPage == 0) && (!spr.bg_size)) {
		if (x >= 512) {
			m_StatusBar.SetPaneText(0, "");
			return;
		}
		if (y >= 512) {
			m_StatusBar.SetPaneText(0, "");
			return;
		}
	}

	// �A�h���X�Z�o
	if (spr.bg_size) {
		// 16�~16
		addr = y >> 4;
		addr <<= 7;
		addr += ((x >> 4) << 1);
	}
	else {
		addr = y >> 3;
		addr <<= 7;
		addr += ((x >> 3) << 1);
	}
	// BG�f�[�^�G���A0��+C000�ABG�f�[�^�G���A1��+E000
	addr += ((spr.bg_area[m_nPage] & 0x01) << 13);
	addr += 0xc000;

	// �f�[�^�擾
	data = *(WORD*)&spr.mem[addr];
	addr += 0xeb0000;

	// �o��
	string.Format("( %d, %d) $%06X [%04X] PCG%d Pal%1X",
				x, y, addr, data, data & 0xff, ((data >> 8) & 0x0f));
	if (data & 0x4000) {
		string += " X-Reverse";
	}
	if (data & 0x8000) {
		string += " Y-Reverse";
	}
	m_StatusBar.SetPaneText(0, string);
}

//===========================================================================
//
//	�p���b�g�E�B���h�E
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	�R���X�g���N�^
//
//---------------------------------------------------------------------------
CPaletteWnd::CPaletteWnd(BOOL bRend)
{
	VC *vc;

	// �E�B���h�E�T�C�Y�ݒ�(32x16�ŁA16x32)
	m_nScrlWidth = 512;
	m_nScrlHeight = 512;

	// �^�C�v�ݒ�
	m_bRend = bRend;

	// �^�C�v�ɉ����Đݒ�
	if (m_bRend) {
		m_dwID = MAKEID('P', 'A', 'L', 'B');
		::GetMsg(IDS_SWND_REND_PALET, m_strCaption);
		m_pRender = (Render*)::GetVM()->SearchDevice(MAKEID('R', 'E', 'N', 'D'));
		ASSERT(m_pRender);
	}
	else {
		m_dwID = MAKEID('P', 'A', 'L', ' ');
		::GetMsg(IDS_SWND_PALET, m_strCaption);
		vc = (VC*)::GetVM()->SearchDevice(MAKEID('V', 'C', ' ', ' '));
		ASSERT(vc);
		m_pVCPal = (WORD*)vc->GetPalette();
		ASSERT(m_pVCPal);
	}
}

//---------------------------------------------------------------------------
//
//	�Z�b�g�A�b�v
//
//---------------------------------------------------------------------------
void FASTCALL CPaletteWnd::Setup(int x, int y, int width, int height, BYTE *ptr)
{
	int i;
	int n;
	int m;
	int delta;
	DWORD buf[512];

	// �f�[�^��������
	m = -2;

	// 512�~512�Ȃ̂ŁA�I�[�o�[����\��������
	if ((y + height) > 512) {
		height = 512 - y;
	}
	delta = 0;
	if ((x + width) > 512) {
		delta = width - 512 + x;
		width = 512 - x;
	}

	// ���[�v
	for (i=0; i<height; i++) {
		// �\�����ׂ��f�[�^�����߂�
		n = y + i;
		if ((n & 0x0f) == 0x0f) {
			// y��+15:�Ԃ̋�
			n = -1;
		}
		else {
			// y��+0�`+14:�ʏ�f�[�^
			n >>= 4;
		}

		// �\���f�[�^���쐬
		if (m != n) {
			m = n;
			if (n < 0) {
				// �Ԃ̋�
				memset(buf, 0, sizeof(buf));
			}
			else {
				// �ʏ�f�[�^
				if (m_bRend) {
					SetupRend(buf, n);
				}
				else {
					SetupVC(buf, n);
				}
			}
		}

		// �R�s�[
		memcpy(ptr, &buf[x], width << 2);
		ptr += (width << 2);
		ptr += (delta << 2);
	}
}

//---------------------------------------------------------------------------
//
//	�Z�b�g�A�b�v(�����_��)
//
//---------------------------------------------------------------------------
void FASTCALL CPaletteWnd::SetupRend(DWORD *buf, int n)
{
	const DWORD *p;
	DWORD rgb;
	int i;
	int j;

	ASSERT(buf);
	ASSERT((n >= 0) && (n <= 0x1f));

	// �|�C���^����
	p = m_pRender->GetPalette();
	p += (n << 4);

	// 16�F���[�v
	for (i=0; i<16; i++) {
		// �J���[�擾
		rgb =*p++;

		// 31�h�b�g�����ĕ`��
		for (j=0; j<31; j++) {
			*buf++ = rgb;
		}

		// ���͋�
		*buf++ = 0;
	}
}

//---------------------------------------------------------------------------
//
//	�Z�b�g�A�b�v(VC)
//
//---------------------------------------------------------------------------
void FASTCALL CPaletteWnd::SetupVC(DWORD *buf, int n)
{
	const WORD *p;
	DWORD rgb;
	int i;
	int j;

	ASSERT(buf);
	ASSERT((n >= 0) && (n <= 0x1f));

	// �|�C���^����
	ASSERT(m_pVCPal);
	p = m_pVCPal;
	p += (n << 4);

	// 16�F���[�v
	for (i=0; i<16; i++) {
		// �J���[�擾
		rgb = ConvPalette(*p++);

		// 31�h�b�g�����ĕ`��
		for (j=0; j<31; j++) {
			*buf++ = rgb;
		}

		// ���͋�
		*buf++ = 0;
	}
}

//---------------------------------------------------------------------------
//
//	���b�Z�[�W�X���b�h����̍X�V
//
//---------------------------------------------------------------------------
void FASTCALL CPaletteWnd::Update()
{
	int x;
	int y;
	int index;
	DWORD rgb;
	const DWORD *p;
	CString string;

	// BMP�E�B���h�E�`�F�b�N
	if (!m_pBMPWnd) {
		return;
	}

	// �}�E�X�J�[�\���`�F�b�N
	if ((m_pBMPWnd->m_nCursorX < 0) || (m_pBMPWnd->m_nCursorY < 0)) {
		m_StatusBar.SetPaneText(0, "");
		return;
	}

	// ���W�v�Z�A�I�[�o�[�`�F�b�N
	x = m_pBMPWnd->m_nCursorX + m_pBMPWnd->m_nScrlX;
	y = m_pBMPWnd->m_nCursorY + m_pBMPWnd->m_nScrlY;
	if (x >= m_nScrlWidth) {
		return;
	}
	if (y >= m_nScrlHeight) {
		return;
	}

	// �\���f�[�^�쐬
	index = y & ~0xf;
	index += (x >> 5);

	if (m_bRend) {
		p = m_pRender->GetPalette();
		rgb = p[index];
		string.Format("($%03X) Contrast%d R%d G%d B%d",
						index,
						m_pRender->GetContrast(),
						(rgb >> 16) & 0xff,
						(rgb >> 8) & 0xff,
						(rgb & 0xff));
	}
	else {
		rgb = ConvPalette(m_pVCPal[index]);
		string.Format("($%03X) $%06X [$%04X] R%d G%d B%d",
						index,
						0xe82000 + (index << 1),
						m_pVCPal[index],
						(rgb >> 16) & 0xff,
						(rgb >> 8) & 0xff,
						(rgb & 0xff));
	}
	m_StatusBar.SetPaneText(0, string);
}

#endif	// _WIN32
