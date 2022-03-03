//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2005 ＰＩ．(ytanaka@ipc-tokai.or.jp)
//	[ MFC ステータスビュー ]
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
//	ステータスビュー
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	コンストラクタ
//
//---------------------------------------------------------------------------
CStatusView::CStatusView()
{
	int i;

	// フレームウィンドウ
	m_pFrmWnd = NULL;

	// メッセージ
	m_strCaption.Empty();
	::GetMsg(AFX_IDS_IDLEMESSAGE, m_strIdle);
	m_strMenu = m_strIdle;
	m_strInfo.Empty();

	// FDD矩形
	for (i=0; i<2; i++) {
		m_rectFDD[i].left = 0;
		m_rectFDD[i].top = 0;
		m_rectFDD[i].right = 0;
		m_rectFDD[i].bottom = 0;
	}

	// LED矩形
	for (i=0; i<3; i++) {
		m_rectLED[i].left = 0;
		m_rectLED[i].top = 0;
		m_rectLED[i].right = 0;
		m_rectLED[i].bottom = 0;
	}
}

//---------------------------------------------------------------------------
//
//	メッセージ マップ
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
//	初期化
//
//---------------------------------------------------------------------------
BOOL FASTCALL CStatusView::Init(CFrmWnd *pFrmWnd)
{
	CRect rectParent;

	ASSERT(this);
	ASSERT(pFrmWnd);

	// フレームウィンドウ記憶
	m_pFrmWnd = pFrmWnd;

	// フレームウィンドウの矩形を取得
	pFrmWnd->GetClientRect(&rectParent);

	// フレームウィンドウの幅のまま、高さを20の矩形をつくる
	m_rectClient.left = 0;
	m_rectClient.top = 0;
	m_rectClient.right = rectParent.Width();
	m_rectClient.bottom = 20;

	// 作成
	if (!Create(NULL, NULL, WS_CHILD | WS_VISIBLE,
				m_rectClient, pFrmWnd, IDM_FULLSCREEN, NULL)) {
		return FALSE;
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	ウィンドウ再描画
//
//---------------------------------------------------------------------------
void CStatusView::OnPaint()
{
	CPaintDC dc(this);
	CInfo *pInfo;
	int i;

	// 最小化なら何もしない
	if (IsIconic()) {
		return;
	}

	// メッセージ
	DrawMessage(&dc);

	// ステータス
	pInfo = m_pFrmWnd->GetInfo();
	if (pInfo) {
		// Infoに任せる
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
//	ウィンドウ背景描画
//
//---------------------------------------------------------------------------
BOOL CStatusView::OnEraseBkgnd(CDC *pDC)
{
	// 黒を保証する
	pDC->FillSolidRect(&m_rectClient, RGB(0, 0, 0));

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	ウィンドウサイズ変更
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

	// 最小化ならクライアント矩形も最小に
	if ((cx == 0) || (cy == 0)) {
		m_rectClient.right = 0;
		m_rectClient.bottom = 0;
		return;
	}

	// クライアント矩形を記憶
	m_rectClient.right = cx;
	m_rectClient.bottom = cy;

	// テキストメトリックを取得
	pDC = new CClientDC(this);
	hFont = (HFONT)::GetStockObject(DEFAULT_GUI_FONT);
	hDefFont = (HFONT)::SelectObject(pDC->m_hDC, hFont);
	ASSERT(hDefFont);
	pDC->GetTextMetrics(&tm);
	::SelectObject(pDC->m_hDC, hDefFont);
	delete pDC;

	// テキストメトリックを記憶
	m_tmWidth = tm.tmAveCharWidth;
	m_tmHeight = tm.tmHeight + tm.tmExternalLeading;

	// テキストに対し、クライアント矩形の上下の余り量を求める
	nRest = cy - m_tmHeight;

	// LED矩形を決める
	for (i=0; i<3; i++) {
		m_rectLED[i].top = nRest >> 2;
		m_rectLED[i].bottom = cy - (nRest >> 2);

		// 個々のサイズを設定
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
			// その他(ありえない)
			default:
				ASSERT(FALSE);
				break;
		}
	}

	// FDD矩形を求める
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

	// 再描画を促す
	Invalidate(FALSE);

	// 基本クラス
	CWnd::OnSize(nType, cx, cy);
}

//---------------------------------------------------------------------------
//
//	ウィンドウ移動
//
//---------------------------------------------------------------------------
void CStatusView::OnMove(int x, int y)
{
	ASSERT(m_pFrmWnd);

	// 基本クラス
	CWnd::OnMove(x, y);

	// フレームウィンドウの再配置を処理する
	m_pFrmWnd->RecalcStatusView();
}

//---------------------------------------------------------------------------
//
//	ウィンドウ削除完了
//
//---------------------------------------------------------------------------
void CStatusView::PostNcDestroy()
{
	// インタフェース要素を削除
	delete this;
}

//---------------------------------------------------------------------------
//
//	キャプション文字列
//
//---------------------------------------------------------------------------
void FASTCALL CStatusView::SetCaptionString(CString& strCap)
{
	CClientDC dc(this);

	ASSERT(this);

	// 記憶して表示
	m_strCaption = strCap;
	DrawMessage(&dc);
}

//---------------------------------------------------------------------------
//
//	メニュー文字列
//
//---------------------------------------------------------------------------
void FASTCALL CStatusView::SetMenuString(CString& strMenu)
{
	CClientDC dc(this);

	ASSERT(this);

	// 記憶して表示
	m_strMenu = strMenu;
	DrawMessage(&dc);
}

//---------------------------------------------------------------------------
//
//	情報文字列
//
//---------------------------------------------------------------------------
void FASTCALL CStatusView::SetInfoString(CString& strInfo)
{
	CClientDC dc(this);

	ASSERT(this);

	// 記憶して表示
	m_strInfo = strInfo;
	DrawMessage(&dc);
}

//---------------------------------------------------------------------------
//
//	メッセージ描画
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

	// 最小化なら何もしない
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

	// 矩形を作る(左半分の40%)
	rectDraw.left = 0;
	rectDraw.top = 0;
	rectDraw.right = m_rectClient.right * 4 / 10;
	rectDraw.bottom = m_rectClient.bottom;

	// メモリDCと、左半分のビットマップを作成
	VERIFY(mDC.CreateCompatibleDC(pDC));
	VERIFY(Bitmap.CreateCompatibleBitmap(pDC, rectDraw.right, rectDraw.bottom));
	pBitmap = mDC.SelectObject(&Bitmap);
	ASSERT(pBitmap);

	// フォントをセレクト
	hFont = (HFONT)::GetStockObject(DEFAULT_GUI_FONT);
	hDefFont = (HFONT)::SelectObject(mDC.m_hDC, hFont);
	ASSERT(hDefFont);

	// 黒でクリア、色を設定
	mDC.SetTextColor(RGB(255, 255, 255));
	mDC.FillSolidRect(&rectDraw, RGB(0, 0, 0));

	// DrawTextで描画
	rectDraw.left = 16;
	mDC.DrawText(strMsg, &rectDraw, DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_NOPREFIX);

	// BitBlt
	pDC->BitBlt(0, 0, rectDraw.right, rectDraw.bottom, &mDC, 0, 0, SRCCOPY);

	// 終了処理
	::SelectObject(mDC.m_hDC, hDefFont);
	mDC.SelectObject(pBitmap);
	VERIFY(Bitmap.DeleteObject());
	VERIFY(mDC.DeleteDC());
}

//---------------------------------------------------------------------------
//
//	ステータス描画
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

	// Info取得
	pInfo = m_pFrmWnd->GetInfo();
	if (!pInfo) {
		return;
	}

	// 矩形取得
	if (nPane > 2) {
		rect = m_rectLED[nPane - 2];
	}
	else {
		rect = m_rectFDD[nPane];
	}

	// Infoに任せる
	pInfo->DrawStatus(nPane, dc.m_hDC, rect);
}

#endif	// _WIN32
