//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2005 �o�h�D(ytanaka@ipc-tokai.or.jp)
//	[ MFC �X�e�[�^�X�r���[ ]
//
//---------------------------------------------------------------------------

#if defined(_WIN32)

#include "os.h"
#include "xm6.h"
#include "vm.h"
#include "render.h"
#include "mfc_frm.h"
#include "mfc_res.h"
#include "mfc_draw.h"
#include "mfc_info.h"
#include "mfc_stat.h"

//===========================================================================
//
//	�X�e�[�^�X�r���[
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	�R���X�g���N�^
//
//---------------------------------------------------------------------------
CStatusView::CStatusView()
{
	int i;

	// �t���[���E�B���h�E
	m_pFrmWnd = NULL;

	// ���b�Z�[�W
	m_strCaption.Empty();
	::GetMsg(AFX_IDS_IDLEMESSAGE, m_strIdle);
	m_strMenu = m_strIdle;
	m_strInfo.Empty();

	// FDD��`
	for (i=0; i<2; i++) {
		m_rectFDD[i].left = 0;
		m_rectFDD[i].top = 0;
		m_rectFDD[i].right = 0;
		m_rectFDD[i].bottom = 0;
	}

	// LED��`
	for (i=0; i<3; i++) {
		m_rectLED[i].left = 0;
		m_rectLED[i].top = 0;
		m_rectLED[i].right = 0;
		m_rectLED[i].bottom = 0;
	}
}

//---------------------------------------------------------------------------
//
//	���b�Z�[�W �}�b�v
//
//---------------------------------------------------------------------------
BEGIN_MESSAGE_MAP(CStatusView, CWnd)
	ON_WM_PAINT()
	ON_WM_ERASEBKGND()
	ON_WM_SIZE()
	ON_WM_MOVE()
END_MESSAGE_MAP()

//---------------------------------------------------------------------------
//
//	������
//
//---------------------------------------------------------------------------
BOOL FASTCALL CStatusView::Init(CFrmWnd *pFrmWnd)
{
	CRect rectParent;

	ASSERT(this);
	ASSERT(pFrmWnd);

	// �t���[���E�B���h�E�L��
	m_pFrmWnd = pFrmWnd;

	// �t���[���E�B���h�E�̋�`���擾
	pFrmWnd->GetClientRect(&rectParent);

	// �t���[���E�B���h�E�̕��̂܂܁A������20�̋�`������
	m_rectClient.left = 0;
	m_rectClient.top = 0;
	m_rectClient.right = rectParent.Width();
	m_rectClient.bottom = 20;

	// �쐬
	if (!Create(NULL, NULL, WS_CHILD | WS_VISIBLE,
				m_rectClient, pFrmWnd, IDM_FULLSCREEN, NULL)) {
		return FALSE;
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	�E�B���h�E�ĕ`��
//
//---------------------------------------------------------------------------
void CStatusView::OnPaint()
{
	CPaintDC dc(this);
	CInfo *pInfo;
	int i;

	// �ŏ����Ȃ牽�����Ȃ�
	if (IsIconic()) {
		return;
	}

	// ���b�Z�[�W
	DrawMessage(&dc);

	// �X�e�[�^�X
	pInfo = m_pFrmWnd->GetInfo();
	if (pInfo) {
		// Info�ɔC����
		for (i=0; i<2; i++) {
			// FDD
			if (m_rectFDD[i].Width() > 0) {
				pInfo->DrawStatus(i, dc.m_hDC, m_rectFDD[i]);
			}
		}
		for (i=0; i<3; i++) {
			// LED
			if (m_rectLED[i].Width() > 0) {
				pInfo->DrawStatus(i + 2, dc.m_hDC, m_rectLED[i]);
			}
		}
	}
}

//---------------------------------------------------------------------------
//
//	�E�B���h�E�w�i�`��
//
//---------------------------------------------------------------------------
BOOL CStatusView::OnEraseBkgnd(CDC *pDC)
{
	// ����ۏ؂���
	pDC->FillSolidRect(&m_rectClient, RGB(0, 0, 0));

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	�E�B���h�E�T�C�Y�ύX
//
//---------------------------------------------------------------------------
void CStatusView::OnSize(UINT nType, int cx, int cy)
{
	CClientDC *pDC;
	TEXTMETRIC tm;
	HFONT hFont;
	HFONT hDefFont;
	int i;
	int nRest;

	// �ŏ����Ȃ�N���C�A���g��`���ŏ���
	if ((cx == 0) || (cy == 0)) {
		m_rectClient.right = 0;
		m_rectClient.bottom = 0;
		return;
	}

	// �N���C�A���g��`���L��
	m_rectClient.right = cx;
	m_rectClient.bottom = cy;

	// �e�L�X�g���g���b�N���擾
	pDC = new CClientDC(this);
	hFont = (HFONT)::GetStockObject(DEFAULT_GUI_FONT);
	hDefFont = (HFONT)::SelectObject(pDC->m_hDC, hFont);
	ASSERT(hDefFont);
	pDC->GetTextMetrics(&tm);
	::SelectObject(pDC->m_hDC, hDefFont);
	delete pDC;

	// �e�L�X�g���g���b�N���L��
	m_tmWidth = tm.tmAveCharWidth;
	m_tmHeight = tm.tmHeight + tm.tmExternalLeading;

	// �e�L�X�g�ɑ΂��A�N���C�A���g��`�̏㉺�̗]��ʂ����߂�
	nRest = cy - m_tmHeight;

	// LED��`�����߂�
	for (i=0; i<3; i++) {
		m_rectLED[i].top = nRest >> 2;
		m_rectLED[i].bottom = cy - (nRest >> 2);

		// �X�̃T�C�Y��ݒ�
		switch (i) {
			// HD BUSY
			case 0:
				m_rectLED[i].left = cx - m_tmWidth * 31;
				m_rectLED[i].right = m_rectLED[i].left;
				m_rectLED[i].right += m_tmWidth * 11;
				break;
			// TIMER
			case 1:
				m_rectLED[i].left = cx - m_tmWidth * 19;
				m_rectLED[i].right = m_rectLED[i].left;
				m_rectLED[i].right += m_tmWidth * 9;
				break;
			// POWER
			case 2:
				m_rectLED[i].left = cx - m_tmWidth * 9;
				m_rectLED[i].right = m_rectLED[i].left;
				m_rectLED[i].right += m_tmWidth * 9;
				break;
			// ���̑�(���肦�Ȃ�)
			default:
				ASSERT(FALSE);
				break;
		}
	}

	// FDD��`�����߂�
	m_rectFDD[0].left = cx * 4 / 10;
	m_rectFDD[0].right = m_rectLED[0].left - m_tmWidth;
	m_rectFDD[1].left = m_rectFDD[0].left + m_rectFDD[0].Width() / 2;
	m_rectFDD[1].right = m_rectFDD[0].right;
	m_rectFDD[0].right = m_rectFDD[1].left - m_tmWidth / 2;
	m_rectFDD[1].left += m_tmWidth / 2;
	for (i=0; i<2; i++) {
		m_rectFDD[i].top = nRest >> 2;
		m_rectFDD[i].bottom = cy - (nRest >> 2);
	}

	// �ĕ`��𑣂�
	Invalidate(FALSE);

	// ��{�N���X
	CWnd::OnSize(nType, cx, cy);
}

//---------------------------------------------------------------------------
//
//	�E�B���h�E�ړ�
//
//---------------------------------------------------------------------------
void CStatusView::OnMove(int x, int y)
{
	ASSERT(m_pFrmWnd);

	// ��{�N���X
	CWnd::OnMove(x, y);

	// �t���[���E�B���h�E�̍Ĕz�u����������
	m_pFrmWnd->RecalcStatusView();
}

//---------------------------------------------------------------------------
//
//	�E�B���h�E�폜����
//
//---------------------------------------------------------------------------
void CStatusView::PostNcDestroy()
{
	// �C���^�t�F�[�X�v�f���폜
	delete this;
}

//---------------------------------------------------------------------------
//
//	�L���v�V����������
//
//---------------------------------------------------------------------------
void FASTCALL CStatusView::SetCaptionString(CString& strCap)
{
	CClientDC dc(this);

	ASSERT(this);

	// �L�����ĕ\��
	m_strCaption = strCap;
	DrawMessage(&dc);
}

//---------------------------------------------------------------------------
//
//	���j���[������
//
//---------------------------------------------------------------------------
void FASTCALL CStatusView::SetMenuString(CString& strMenu)
{
	CClientDC dc(this);

	ASSERT(this);

	// �L�����ĕ\��
	m_strMenu = strMenu;
	DrawMessage(&dc);
}

//---------------------------------------------------------------------------
//
//	��񕶎���
//
//---------------------------------------------------------------------------
void FASTCALL CStatusView::SetInfoString(CString& strInfo)
{
	CClientDC dc(this);

	ASSERT(this);

	// �L�����ĕ\��
	m_strInfo = strInfo;
	DrawMessage(&dc);
}

//---------------------------------------------------------------------------
//
//	���b�Z�[�W�`��
//
//---------------------------------------------------------------------------
void FASTCALL CStatusView::DrawMessage(CDC *pDC) const
{
	CString strMsg;
	CBitmap Bitmap;
	CBitmap *pBitmap;
	CDC mDC;
	HFONT hFont;
	HFONT hDefFont;
	CRect rectDraw;

	ASSERT(pDC);

	// �ŏ����Ȃ牽�����Ȃ�
	if ((m_rectClient.Width() == 0) || (m_rectClient.Height() == 0)) {
		return;
	}


	strMsg = m_strMenu;
	if (strMsg == m_strIdle) {
		if (m_strInfo.GetLength() > 0) {
			strMsg = m_strInfo;
		}
		else {
			strMsg = m_strCaption;
		}
	}

	// ��`�����(��������40%)
	rectDraw.left = 0;
	rectDraw.top = 0;
	rectDraw.right = m_rectClient.right * 4 / 10;
	rectDraw.bottom = m_rectClient.bottom;

	// ������DC�ƁA�������̃r�b�g�}�b�v���쐬
	VERIFY(mDC.CreateCompatibleDC(pDC));
	VERIFY(Bitmap.CreateCompatibleBitmap(pDC, rectDraw.right, rectDraw.bottom));
	pBitmap = mDC.SelectObject(&Bitmap);
	ASSERT(pBitmap);

	// �t�H���g���Z���N�g
	hFont = (HFONT)::GetStockObject(DEFAULT_GUI_FONT);
	hDefFont = (HFONT)::SelectObject(mDC.m_hDC, hFont);
	ASSERT(hDefFont);

	// ���ŃN���A�A�F��ݒ�
	mDC.SetTextColor(RGB(255, 255, 255));
	mDC.FillSolidRect(&rectDraw, RGB(0, 0, 0));

	// DrawText�ŕ`��
	rectDraw.left = 16;
	mDC.DrawText(strMsg, &rectDraw, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);

	// BitBlt
	pDC->BitBlt(0, 0, rectDraw.right, rectDraw.bottom, &mDC, 0, 0, SRCCOPY);

	// �I������
	::SelectObject(mDC.m_hDC, hDefFont);
	mDC.SelectObject(pBitmap);
	VERIFY(Bitmap.DeleteObject());
	VERIFY(mDC.DeleteDC());
}

//---------------------------------------------------------------------------
//
//	�X�e�[�^�X�`��
//
//---------------------------------------------------------------------------
void FASTCALL CStatusView::DrawStatus(int nPane)
{
	CClientDC dc(this);
	CRect rect;
	CInfo *pInfo;

	ASSERT(this);
	ASSERT(nPane >= 0);
	ASSERT(m_pFrmWnd);

	// Info�擾
	pInfo = m_pFrmWnd->GetInfo();
	if (!pInfo) {
		return;
	}

	// ��`�擾
	if (nPane > 2) {
		rect = m_rectLED[nPane - 2];
	}
	else {
		rect = m_rectFDD[nPane];
	}

	// Info�ɔC����
	pInfo->DrawStatus(nPane, dc.m_hDC, rect);
}

#endif	// _WIN32
