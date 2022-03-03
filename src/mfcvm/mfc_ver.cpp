//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 �o�h�D(ytanaka@ipc-tokai.or.jp)
//	[ MFC �o�[�W�������_�C�A���O ]
//
//---------------------------------------------------------------------------

#if defined(_WIN32)

#include "os.h"
#include "xm6.h"
#include "vm.h"
#include "rtc.h"
#include "sasi.h"
#include "fdd.h"
#include "render.h"
#include "config.h"
#include "mfc_frm.h"
#include "mfc_com.h"
#include "mfc_draw.h"
#include "mfc_cfg.h"
#include "mfc_info.h"
#include "mfc_res.h"
#include "mfc_ver.h"

//---------------------------------------------------------------------------
//
//	�R���X�g���N�^
//
//---------------------------------------------------------------------------
CAboutDlg::CAboutDlg(CWnd *pParent) : CDialog(IDD_ABOUTDLG, pParent)
{
	// �p����ւ̑Ή�
	if (!::IsJapanese()) {
		m_lpszTemplateName = MAKEINTRESOURCE(IDD_US_ABOUTDLG);
		m_nIDHelp = IDD_US_ABOUTDLG;
	}

	// �f�o�C�X
	m_pRTC = NULL;
	m_pSASI = NULL;
	m_pFDD = NULL;

	// ���̑�
	m_nTimerID = NULL;
	m_pDrawView = NULL;
	m_bFloppyLED = FALSE;
}

//---------------------------------------------------------------------------
//
//	���b�Z�[�W �}�b�v
//
//---------------------------------------------------------------------------
BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
	ON_WM_PAINT()
	ON_WM_NCHITTEST()
	ON_WM_SETCURSOR()
	ON_WM_TIMER()
END_MESSAGE_MAP()

//---------------------------------------------------------------------------
//
//	�_�C�A���O������
//
//---------------------------------------------------------------------------
BOOL CAboutDlg::OnInitDialog()
{
	CFrmWnd *pFrmWnd;
	CStatic *pStatic;
	CString strFormat;
	CString strText;
	DWORD dwMajor;
	DWORD dwMinor;
	Config config;

	// ��{�N���X
	if (!CDialog::OnInitDialog()) {
		return FALSE;
	}

	// URL������E��`���擾���A����
	pStatic = (CStatic*)GetDlgItem(IDC_ABOUT_URL);
	ASSERT(pStatic);
	pStatic->GetWindowText(m_URLString);
	pStatic->GetWindowRect(&m_URLRect);
	ScreenToClient(&m_URLRect);
	pStatic->DestroyWindow();
	m_bURLHit = FALSE;

	// �A�C�R����`���擾���A����
	pStatic = (CStatic*)GetDlgItem(IDC_ABOUT_ICON);
	ASSERT(pStatic);
	pStatic->GetWindowRect(&m_IconRect);
	ScreenToClient(&m_IconRect);
	pStatic->DestroyWindow();

	// �o�[�W�����̏���
	pStatic = (CStatic*)GetDlgItem(IDC_ABOUT_VER);
	ASSERT(pStatic);
	pStatic->GetWindowText(strFormat);
	::GetVM()->GetVersion(dwMajor, dwMinor);
	strText.Format(strFormat, dwMajor, dwMinor);
	pStatic->SetWindowText(strText);

	// �^�C�}�X�^�[�g(80ms)
	m_nTimerID = SetTimer(IDD_ABOUTDLG, 100, NULL);
	ASSERT(m_nTimerID);

	// RTC�擾
	ASSERT(!m_pRTC);
	m_pRTC = (RTC*)::GetVM()->SearchDevice(MAKEID('R', 'T', 'C', ' '));
	ASSERT(m_pRTC);

	// SASI�擾
	ASSERT(!m_pSASI);
	m_pSASI = (SASI*)::GetVM()->SearchDevice(MAKEID('S', 'A', 'S', 'I'));
	ASSERT(m_pSASI);

	// FDD�擾
	ASSERT(!m_pFDD);
	m_pFDD = (FDD*)::GetVM()->SearchDevice(MAKEID('F', 'D', 'D', ' '));
	ASSERT(m_pFDD);

	// Draw�r���[�擾
	pFrmWnd = (CFrmWnd*)GetParent();
	ASSERT(pFrmWnd);
	m_pDrawView = pFrmWnd->GetView();
	ASSERT(m_pDrawView);

	// Config�I�v�V�������擾
	pFrmWnd->GetConfig()->GetConfig(&config);
	m_bFloppyLED = config.floppy_led;
	m_bPowerLED = config.power_led;

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	OK
//
//---------------------------------------------------------------------------
void CAboutDlg::OnOK()
{
	// �^�C�}�폜
	if (m_nTimerID) {
		KillTimer(m_nTimerID);
		m_nTimerID = NULL;
	}

	// �_�C�A���O�I��
	CDialog::OnOK();
}

//---------------------------------------------------------------------------
//
//	�L�����Z��
//
//---------------------------------------------------------------------------
void CAboutDlg::OnCancel()
{
	// �^�C�}�폜
	if (m_nTimerID) {
		KillTimer(m_nTimerID);
		m_nTimerID = NULL;
	}

	// �_�C�A���O�I��
	CDialog::OnCancel();
}

//---------------------------------------------------------------------------
//
//	�`��
//
//---------------------------------------------------------------------------
void CAboutDlg::OnPaint()
{
	CPaintDC dc(this);
	CDC mDC;
	CDC cpDC;
	CBitmap Bitmap;
	CBitmap *pBitmap;

	// URL�`��
	DrawURL(&dc);

	// ������DC�A�݊��r�b�g�}�b�v�A�R�s�[�r�b�g�}�b�v���쐬
	VERIFY(mDC.CreateCompatibleDC(&dc));
	VERIFY(Bitmap.CreateCompatibleBitmap(&dc,
						m_IconRect.Width(), m_IconRect.Height()));

	// �r�b�g�}�b�v��I��
	pBitmap = mDC.SelectObject(&Bitmap);
	ASSERT(pBitmap);

	// �N���A�A�R�s�[DC����
	mDC.FillSolidRect(0, 0, m_IconRect.Width(), m_IconRect.Height(), ::GetSysColor(COLOR_3DFACE));

	// �r�b�g�}�b�v�`�惁�C��
	DrawCRT(&mDC);
	DrawX68k(&mDC);
	DrawLED(0, 0, &mDC);
	DrawView(0, 0, &mDC);

	// BitBlt
	VERIFY(dc.BitBlt(m_IconRect.left, m_IconRect.top,
					m_IconRect.Width(), m_IconRect.Height(), &mDC, 0, 0, SRCCOPY));

	// �`��I��
	VERIFY(mDC.SelectObject(pBitmap));
	VERIFY(Bitmap.DeleteObject());
	VERIFY(mDC.DeleteDC());
}

//---------------------------------------------------------------------------
//
//	�`��(URL)
//
//---------------------------------------------------------------------------
void FASTCALL CAboutDlg::DrawURL(CDC *pDC)
{
	HFONT hFont;
	HFONT hDefFont;
	TEXTMETRIC tm;
	LOGFONT lf;

	ASSERT(pDC);

	// GUI�t�H���g�̃��g���b�N�𓾂�
	hFont = (HFONT)::GetStockObject(DEFAULT_GUI_FONT);
	ASSERT(hFont);
	hDefFont = (HFONT)::SelectObject(pDC->m_hDC, hFont);
	::GetTextMetrics(pDC->m_hDC, &tm);
	memset(&lf, 0, sizeof(lf));
	::GetTextFace(pDC->m_hDC, LF_FACESIZE, lf.lfFaceName);
	::SelectObject(pDC->m_hDC, hDefFont);

	// �A���_�[���C����t�������t�H���g���쐬
	lf.lfHeight = tm.tmHeight;
	lf.lfWidth = 0;
	lf.lfEscapement = 0;
	lf.lfOrientation = 0;
	lf.lfWeight = FW_DONTCARE;
	lf.lfItalic = tm.tmItalic;
	lf.lfUnderline = TRUE;
	lf.lfStrikeOut = tm.tmStruckOut;
	lf.lfCharSet = tm.tmCharSet;
	lf.lfOutPrecision = OUT_DEFAULT_PRECIS;
	lf.lfClipPrecision = CLIP_DEFAULT_PRECIS;
	lf.lfQuality = DEFAULT_QUALITY;
	lf.lfPitchAndFamily = tm.tmPitchAndFamily;
	hFont = ::CreateFontIndirect(&lf);
	ASSERT(hFont);

	// �Z���N�g�A�`��
	hDefFont = (HFONT)::SelectObject(pDC->m_hDC, hFont);
	ASSERT(hDefFont);
	if (m_bURLHit) {
		::SetTextColor(pDC->m_hDC, RGB(255, 0, 0));
	}
	else {
		::SetTextColor(pDC->m_hDC, RGB(0, 0, 255));
	}
	::SetBkColor(pDC->m_hDC, ::GetSysColor(COLOR_3DFACE));
	::DrawText(pDC->m_hDC, (LPCTSTR)m_URLString, m_URLString.GetLength(),
				&m_URLRect, DT_CENTER | DT_SINGLELINE);

	// �t�H���g��߂�
	::SelectObject(pDC->m_hDC, hDefFont);
	::DeleteObject(hFont);
}

//---------------------------------------------------------------------------
//
//	�`��(CRT)
//
//---------------------------------------------------------------------------
void FASTCALL CAboutDlg::DrawCRT(CDC *pDC)
{
	BYTE *pbmp;
	BYTE Info[0x800];
	HRSRC hResource;
	HGLOBAL hGlobal;
	BITMAPINFOHEADER *pbmi;
	RGBQUAD *prgb;
	DWORD color3d;

	// ���\�[�XIDB_CRT���蓮���[�h
	hResource = ::FindResource(AfxGetApp()->m_hInstance,
								MAKEINTRESOURCE(IDB_CRT), RT_BITMAP);
	ASSERT(hResource);
	hGlobal = ::LoadResource(AfxGetApp()->m_hInstance, hResource);
	ASSERT(hGlobal);

	// ���[�h�r�b�g�}�b�v����w�b�_�����R�s�[
	pbmp = (BYTE*)::LockResource(hGlobal);
	ASSERT(pbmp);
	memcpy(Info, pbmp, sizeof(Info));

	// �p���b�g��ύX
	pbmi = (BITMAPINFOHEADER*)Info;
	prgb = (RGBQUAD*)&Info[pbmi->biSize];
	color3d = ::GetSysColor(COLOR_3DFACE);
	prgb->rgbBlue = GetBValue(color3d);
	prgb->rgbGreen = GetGValue(color3d);
	prgb->rgbRed = GetRValue(color3d);

	// SetDIBitsToDevice�ŕ`��
	::SetDIBitsToDevice(pDC->m_hDC, 2, 2, 62, 64, 0, 0, 0, 64,
						&pbmp[pbmi->biSize + sizeof(RGBQUAD) * 256],
						(BITMAPINFO*)pbmi, DIB_RGB_COLORS);

	// ���\�[�X�����(Win9x�n�̂ݕK�v)
	::FreeResource(hGlobal);
}

//---------------------------------------------------------------------------
//
//	�`��(X68k)
//
//---------------------------------------------------------------------------
void FASTCALL CAboutDlg::DrawX68k(CDC *pDC)
{
	BYTE *pbmp;
	BYTE Info[0x800];
	HRSRC hResource;
	HGLOBAL hGlobal;
	BITMAPINFOHEADER *pbmi;
	RGBQUAD *prgb;
	DWORD color3d;

	// ���\�[�XIDB_CRT���蓮���[�h
	hResource = ::FindResource(AfxGetApp()->m_hInstance,
								MAKEINTRESOURCE(IDB_X68K), RT_BITMAP);
	ASSERT(hResource);
	hGlobal = ::LoadResource(AfxGetApp()->m_hInstance, hResource);
	ASSERT(hGlobal);

	// ���[�h�r�b�g�}�b�v����w�b�_�����R�s�[
	pbmp = (BYTE*)::LockResource(hGlobal);
	ASSERT(pbmp);
	memcpy(Info, pbmp, sizeof(Info));

	// �p���b�g��ύX
	pbmi = (BITMAPINFOHEADER*)Info;
	prgb = (RGBQUAD*)&Info[pbmi->biSize];
	color3d = ::GetSysColor(COLOR_3DFACE);
	prgb[207].rgbBlue = GetBValue(color3d);
	prgb[207].rgbGreen = GetGValue(color3d);
	prgb[207].rgbRed = GetRValue(color3d);

	// SetDIBitsToDevice�ŕ`��
	::SetDIBitsToDevice(pDC->m_hDC, 67, 2, 30, 64, 0, 0, 0, 64,
						&pbmp[pbmi->biSize + sizeof(RGBQUAD) * 256],
						(BITMAPINFO*)pbmi, DIB_RGB_COLORS);

	// ���\�[�X�����(Win9x�n�̂ݕK�v)
	::FreeResource(hGlobal);
}

//---------------------------------------------------------------------------
//
//	�q�b�g�e�X�g
//
//---------------------------------------------------------------------------
#if _MFC_VER >= 0x800
LRESULT CAboutDlg::OnNcHitTest(CPoint point)
#else
UINT CAboutDlg::OnNcHitTest(CPoint point)
#endif	// _MFC_VER
{
	CPoint pt;

	// �N���C�A���g���W�ɕϊ����āA��`���`�F�b�N
	pt = point;
	ScreenToClient(&pt);
	if (m_URLRect.PtInRect(pt)) {
		if (!m_bURLHit) {
			m_bURLHit = TRUE;
			InvalidateRect(&m_URLRect, FALSE);
		}
	}
	else {
		if (m_bURLHit) {
			m_bURLHit = FALSE;
			InvalidateRect(&m_URLRect, FALSE);
		}
	}

	return CDialog::OnNcHitTest(point);
}

//---------------------------------------------------------------------------
//
//	�J�[�\���Z�b�g
//
//---------------------------------------------------------------------------
BOOL CAboutDlg::OnSetCursor(CWnd *pWnd, UINT nHitTest, UINT message)
{
	HCURSOR hCursor;

	// �q�b�g���Ă��Ȃ���Βʏ폈��
	if (!m_bURLHit) {
		return CDialog::OnSetCursor(pWnd, nHitTest, message);
	}

	// �A�C�R��IDC_HAND�����[�h����BWINVER >= 0x500�ȏオ�K�v
	hCursor = AfxGetApp()->LoadStandardCursor(MAKEINTRESOURCE(32649));
	if (!hCursor) {
		// ���s�����ꍇ�͖���ȃA�C�R���Ń��g���C
		hCursor = AfxGetApp()->LoadStandardCursor(IDC_IBEAM);
	}
	::SetCursor(hCursor);

	// �}�E�X��������Ă����URL���s
	if ((message == WM_LBUTTONDOWN) || (message == WM_LBUTTONDBLCLK)) {
		::ShellExecute(m_hWnd, NULL, (LPCTSTR)m_URLString, NULL, NULL, SW_SHOWNORMAL);
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	�^�C�}
//
//---------------------------------------------------------------------------
#if _MFC_VER >= 0x700
void CAboutDlg::OnTimer(UINT_PTR /*nID */)
#else
void CAboutDlg::OnTimer(UINT /*nID */)
#endif	// _MFC_VER
{
	CFrmWnd *pFrmWnd;
	CClientDC *pDC;
	CInfo *pInfo;

	// �^�C�}�폜
	KillTimer(m_nTimerID);
	m_nTimerID = NULL;

	// LED�ƃr���[���X�V
	pDC = new CClientDC(this);
	DrawLED(m_IconRect.left, m_IconRect.top, pDC);
	DrawView(m_IconRect.left, m_IconRect.top, pDC);
	delete pDC;

	// ���\��
	pFrmWnd = (CFrmWnd*)GetParent();
	ASSERT(pFrmWnd);
	pInfo = pFrmWnd->GetInfo();
	if (pInfo) {
		if (pInfo->IsEnable()) {
			pInfo->UpdateStatus();
			pInfo->UpdateInfo();
			pInfo->UpdateCaption();
		}
	}

	// �^�C�}�ݒ�(�`��I���ォ��A�Œ�100ms������)
	m_nTimerID = SetTimer(IDD_ABOUTDLG, 100, NULL);
}

//---------------------------------------------------------------------------
//
//	LED�`��
//
//---------------------------------------------------------------------------
void FASTCALL CAboutDlg::DrawLED(int x, int y, CDC *pDC)
{
	int nDrive;
	int nLength;
	int nStatus;
	COLORREF color;
	BOOL bPower;

	ASSERT(x >= 0);
	ASSERT(y >= 0);
	ASSERT(pDC);

	ASSERT(m_pRTC);
	ASSERT(m_pSASI);
	ASSERT(m_pFDD);

	// �d��
	bPower = ::GetVM()->IsPower();

	// HD BUSY
	color = RGB(0, 0, 0);
	if (m_pSASI->IsBusy() && bPower) {
		color = RGB(208, 31, 31);
	}
	pDC->SetPixelV(x + 67 + 19, y + 2 + 12, color);

	// �^�C�}
	color = RGB(0, 0, 0);
	if (m_pRTC->GetTimerLED()) {
		color = RGB(208, 31, 31);
	}
	pDC->SetPixelV(x + 67 + 23, y + 2 + 12, color);

	// �d��LED
	if (m_bPowerLED) {
		// �Ð�
		color = RGB(12, 23, 129);
	}
	else {
		// ��
		color = RGB(208, 31, 31);
	}
	if (bPower) {
		if (::GetVM()->IsPowerSW()) {
			if (m_bPowerLED) {
				// ��
				color = RGB(50, 50, 255);
			}
			else {
				// ��
				color = RGB(31, 208, 31);
			}
		}
		else {
			if (m_pRTC->GetAlarmOut()) {
				if (m_bPowerLED) {
					// ��
					color = RGB(50, 50, 255);
				}
				else {
					// ��
					color = RGB(31, 208, 31);
				}
			}
			else {
				if (m_bPowerLED) {
					color = RGB(12, 23, 129);
				}
				else {
					// ��
					color = RGB(0, 0, 0);
				}
			}
		}
	}
	pDC->SetPixelV(x + 67 + 27, y + 2 + 12, color);

	// FDD
	for (nDrive=0; nDrive<2; nDrive++) {
		// X���W����
		if (nDrive == 0) {
			x += 4;
		}
		else {
			x += 5;
		}

		// �擾
		nStatus = m_pFDD->GetStatus(nDrive);

		// ���f�B�A����(�c��)
		if (nStatus & FDST_INSERT) {
			color = RGB(64, 63, 63);
		}
		else {
			color = RGB(7, 6, 6);
		}
		for (nLength=30; nLength<=50; nLength++) {
			pDC->SetPixelV(x + 67, y + nLength + 2, color);
		}

		// ��LED�͍��E�_�ŁE�}���E�A�N�Z�X(��ɂȂ�قǗD�捂��)
		color = 0;
		if (nStatus & FDST_CURRENT) {
			color = RGB(15, 159, 15);
		}
		if (nStatus & FDST_INSERT) {
			color = RGB(38, 37, 37);
		}
		if (m_bFloppyLED) {
			if ((nStatus & FDST_MOTOR) && (nStatus & FDST_SELECT)) {
				color = RGB(208, 31, 31);
			}
		}
		else {
			if (nStatus & FDST_ACCESS) {
				color = RGB(208, 31, 31);
			}
		}
		if (!bPower) {
			color = 0;
		}
		pDC->SetPixelV(x + 67, y + 27 + 2, color);

		// ����LED�͖��}���E�ʏ�E�C�W�F�N�g�֎~(��ɂȂ�قǗD��x����)
		color = RGB(38, 37, 37);
		if (nStatus & FDST_INSERT) {
			if (nStatus & FDST_EJECT) {
				color = RGB(31, 208, 31);
			}
			else {
				color = RGB(119, 119, 119);
			}
		}
		if (!bPower) {
			color = 0;
		}
		pDC->SetPixelV(x + 67, y + 53 + 2, color);
	}
}

//---------------------------------------------------------------------------
//
//	�r���[�`��
//
//---------------------------------------------------------------------------
void FASTCALL CAboutDlg::DrawView(int x, int y, CDC *pDC)
{
	CDrawView::DRAWINFO info;
	CRect rect;
	BITMAPINFOHEADER bmi;

	ASSERT(x >= 0);
	ASSERT(y >= 0);
	ASSERT(pDC);

	// ��`�ݒ�
	rect.left = x + 10;
	rect.top = y + 8;
	rect.right = x + 55;
	rect.bottom  = y + 43;

	// �d���I�t�Ȃ�A��
	if (!::GetVM()->IsPower()) {
		pDC->FillSolidRect(&rect, RGB(0, 0, 0));
		return;
	}

	// �r���[�̃��[�N�𓾂�
	ASSERT(m_pDrawView);
	m_pDrawView->GetDrawInfo(&info);

	// BITMAPINFO����
	memset(&bmi, 0, sizeof(bmi));
	bmi.biSize = sizeof(bmi);
	bmi.biWidth = info.nBMPWidth;
	bmi.biHeight = info.nBMPHeight;
	bmi.biPlanes = 1;
	bmi.biBitCount = 32;
	bmi.biCompression = BI_RGB;
	bmi.biSizeImage = info.nBMPWidth * info.nBMPHeight * (32 >> 3);

	// �k����Blt
	::StretchDIBits(pDC->m_hDC, rect.left, rect.top + rect.Height() - 1,
					rect.Width(), -rect.Height(),
					0, 0, info.nWidth, info.nHeight,
					info.pBits, (BITMAPINFO*)&bmi,
					DIB_RGB_COLORS, SRCCOPY);
}

#endif	// _WIN32
