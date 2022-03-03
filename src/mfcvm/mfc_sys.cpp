//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 ＰＩ．(ytanaka@ipc-tokai.or.jp)
//	[ MFC サブウィンドウ(システム) ]
//
//---------------------------------------------------------------------------

#if defined(_WIN32)

#include "os.h"
#include "xm6.h"
#include "log.h"
#include "device.h"
#include "vm.h"
#include "schedule.h"
#include "event.h"
#include "render.h"
#include "mfc_sub.h"
#include "mfc_frm.h"
#include "mfc_draw.h"
#include "mfc_com.h"
#include "mfc_sch.h"
#include "mfc_res.h"
#include "mfc_cpu.h"
#include "mfc_sys.h"

//===========================================================================
//
//	ログウィンドウ
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	コンストラクタ
//
//---------------------------------------------------------------------------
CLogWnd::CLogWnd() : CSubListWnd()
{
	// ウィンドウパラメータ定義
	m_dwID = MAKEID('L', 'O', 'G', 'L');
	::GetMsg(IDS_SWND_LOG, m_strCaption);
	m_nWidth = 64;
	m_nHeight = 8;

	// フォーカスなし
	m_nFocus = -1;

	// その他
	m_pLog = NULL;
}

//---------------------------------------------------------------------------
//
//	メッセージ マップ
//
//---------------------------------------------------------------------------
BEGIN_MESSAGE_MAP(CLogWnd, CSubListWnd)
	ON_WM_CREATE()
	ON_WM_DRAWITEM()
	ON_NOTIFY(NM_DBLCLK, 0, OnDblClick)
END_MESSAGE_MAP()

//---------------------------------------------------------------------------
//
//	ウィンドウ作成
//
//---------------------------------------------------------------------------
int CLogWnd::OnCreate(LPCREATESTRUCT lpcs)
{
	ASSERT(this);

	// 基本クラス
	if (CSubListWnd::OnCreate(lpcs) != 0) {
		return -1;
	}

	// 文字列を読み込む
	::GetMsg(IDS_SWND_LOG_DETAIL, m_strDetail);
	::GetMsg(IDS_SWND_LOG_NORMAL, m_strNormal);
	::GetMsg(IDS_SWND_LOG_WARNING, m_strWarning);

	// ログを取得
	ASSERT(!m_pLog);
	m_pLog = &(::GetVM()->log);
	ASSERT(m_pLog);

	return 0;
}

//---------------------------------------------------------------------------
//
//	ダブルクリック
//
//---------------------------------------------------------------------------
void CLogWnd::OnDblClick(NMHDR* /*pNotifyStruct*/, LRESULT* /*result*/)
{
	Log::logdata_t logdata;
	BOOL bSuccess;
	CDrawView *pView;
	CDisasmWnd *pDisWnd;

	ASSERT(this);
	ASSERT_VALID(this);

	// フォーカスがある場合に限る
	if (m_nFocus < 0) {
		return;
	}

	// ログデータ取得、文字列バッファ解放
	bSuccess = m_pLog->GetData(m_nFocus, &logdata);
	if (!bSuccess) {
		// バッファの解放必要性なし
		return;
	}

	// 逆アセンブルウィンドウへアドレス指定
	pView = (CDrawView*)GetParent();
	pDisWnd = (CDisasmWnd*)pView->SearchSWnd(MAKEID('D', 'I', 'S', 'A'));
	if (pDisWnd) {
		pDisWnd->SetAddr(logdata.pc);
	}

	// GetData()で確保したバッファを解放
	delete[] logdata.string;
}

//---------------------------------------------------------------------------
//
//	オーナー描画
//
//---------------------------------------------------------------------------
void CLogWnd::OnDrawItem(int nID, LPDRAWITEMSTRUCT lpDIS)
{
	CDC dc;
	CDC mDC;
	CBitmap Bitmap;
	CBitmap *pBitmap;
	CFont *pFont;
	CRect rectItem(&lpDIS->rcItem);
	CRect rectClient;
	CString strText;
	Log::logdata_t logdata;
	CSize size;
	int nItem;
	int nWidth;
#if !defined(UNICODE)
	wchar_t szWide[100];
#endif	// UNICODE

	ASSERT(this);
	ASSERT(nID == 0);
	ASSERT(lpDIS);
	ASSERT_VALID(this);

	// DCをアタッチ
	VERIFY(dc.Attach(lpDIS->hDC));

	// 無効なら空白を描画
	if (!m_bEnable) {
		dc.FillSolidRect(&rectItem, ::GetSysColor(COLOR_WINDOW));
		dc.Detach();
		return;
	}

	// ログデータ取得
	if (!m_pLog->GetData(lpDIS->itemID, &logdata)) {
		// なければ空白を表示
		dc.FillSolidRect(&rectItem, ::GetSysColor(COLOR_WINDOW));
		dc.Detach();
		return;
	}

	// 表示準備
	VERIFY(mDC.CreateCompatibleDC(&dc));
	VERIFY(Bitmap.CreateCompatibleBitmap(&dc, rectItem.Width(), rectItem.Height()));
	pBitmap = mDC.SelectObject(&Bitmap);
	ASSERT(pBitmap);

	// フォント移転
	pFont = mDC.SelectObject(m_ListCtrl.GetFont());
	ASSERT(pFont);

	// 地色を決めてベタ塗りする
	rectClient.left = 0;
	rectClient.right = rectItem.Width();
	rectClient.top = 0;
	rectClient.bottom = rectItem.Height();
	if (lpDIS->itemState & ODS_FOCUS) {
		// フォーカス番号を記録するのを忘れずに
		m_nFocus = lpDIS->itemID;
		mDC.FillSolidRect(&rectClient, GetSysColor(COLOR_HIGHLIGHT));
		mDC.SetTextColor(::GetSysColor(COLOR_HIGHLIGHTTEXT));
		mDC.SetBkColor(::GetSysColor(COLOR_HIGHLIGHT));
	}
	else {
		// フォーカスなし
		mDC.FillSolidRect(&rectClient, ::GetSysColor(COLOR_WINDOW));
		mDC.SetBkColor(::GetSysColor(COLOR_WINDOW));
	}

	// サブアイテムを順番に表示
	for (nItem=0; nItem<6; nItem++) {
		// 座標作成
		nWidth = m_ListCtrl.GetColumnWidth(nItem);
		rectClient.right = rectClient.left + nWidth;

		// テキスト取得
		TextSub(nItem, &logdata, strText);

		// 色設定
		if (!(lpDIS->itemState & ODS_FOCUS)) {
			mDC.SetTextColor(RGB(0, 0, 0));
			if (nItem == 3) {
				switch (logdata.level) {
					// 詳細(灰色)
					case Log::Detail:
						mDC.SetTextColor(RGB(192, 192, 192));
						break;
					// 警告(赤色)
					case Log::Warning:
						mDC.SetTextColor(RGB(255, 0, 0));
						break;
				}
			}
		}

		// 表示
		switch (nItem) {
			// No.(右詰め)
			case 0:
			// 仮想時間(右詰め)
			case 1:
				mDC.DrawText(strText, &rectClient, DT_SINGLELINE | DT_RIGHT | DT_VCENTER);
				break;

			// PC(中央揃え)
			case 2:
			// レベル(中央揃え)
			case 3:
			// デバイス(中央揃え)
			case 4:
				mDC.DrawText(strText, &rectClient, DT_SINGLELINE | DT_CENTER | DT_VCENTER);
				break;

			// メッセージ(左詰め)
			case 5:
#if !defined(UNICODE)
				// 日本語なら、VMからのシフトJIS文字列をそのまま表示できる
				if (::IsJapanese()) {
					// 日本語
					mDC.DrawText(strText, &rectClient, DT_SINGLELINE | DT_LEFT | DT_VCENTER);
					break;
				}

				// CP932サポートか
				if (::Support932()) {
					// CP932サポート(UNICODEもサポートされている)
					memset(&szWide[0], 0, sizeof(szWide));
					if (_tcslen(strText) < 0x100) {
						// シフトJISから、UNICODEへ変換する
						::MultiByteToWideChar(932, MB_PRECOMPOSED, (LPCSTR)(LPCTSTR)strText, -1, szWide, 100);

						// UNICODEとして表示
						::DrawTextWide(mDC.m_hDC, szWide, -1, rectClient, DT_SINGLELINE | DT_LEFT | DT_VCENTER);
					}
					break;
				}

				// 表示できない
				if (::IsWinNT()) {
					strText = _T("CP932(Japanese Shift-JIS) is required");
				}
				else {
					strText = _T("(Japanese Text)");
				}
#endif	// !UNICODE
				mDC.DrawText(strText, &rectClient, DT_SINGLELINE | DT_LEFT | DT_VCENTER);
				break;

			// その他(ありえない)
			default:
				ASSERT(FALSE);
		}

		// 矩形更新
		rectClient.left += nWidth;
	}

	// 表示終了
	VERIFY(dc.BitBlt(rectItem.left, rectItem.top, rectItem.Width(), rectItem.Height(),
						&mDC, 0, 0, SRCCOPY));
	VERIFY(mDC.SelectObject(pFont));
	VERIFY(mDC.SelectObject(pBitmap));
	VERIFY(Bitmap.DeleteObject());
	VERIFY(mDC.DeleteDC());
	dc.Detach();

	// 文字列バッファ解放
	delete[] logdata.string;
}

//---------------------------------------------------------------------------
//
//	リストコントロール初期化
//
//---------------------------------------------------------------------------
void FASTCALL CLogWnd::InitList()
{
	CString strHeader;

	ASSERT(this);
	ASSERT_VALID(this);

	// カラム0(通し番号)
	::GetMsg(IDS_SWND_LOG_NUMBER, strHeader);
	m_ListCtrl.InsertColumn(0, strHeader, LVCFMT_RIGHT, m_tmWidth * 7, 0);

	// カラム1(時間)
	::GetMsg(IDS_SWND_LOG_TIME, strHeader);
	m_ListCtrl.InsertColumn(1, strHeader, LVCFMT_RIGHT, m_tmWidth * 10, 1);

	// カラム2(PC)
	::GetMsg(IDS_SWND_LOG_PC, strHeader);
	m_ListCtrl.InsertColumn(2, strHeader, LVCFMT_CENTER, m_tmWidth * 8, 2);

	// カラム3(レベル)
	::GetMsg(IDS_SWND_LOG_LEVEL, strHeader);
	m_ListCtrl.InsertColumn(3, strHeader, LVCFMT_CENTER, m_tmWidth * 6, 3);

	// カラム4(デバイス)
	::GetMsg(IDS_SWND_LOG_DEVICE, strHeader);
	m_ListCtrl.InsertColumn(4, strHeader, LVCFMT_CENTER, m_tmWidth * 8, 4);

	// カラム5(メッセージ)
	::GetMsg(IDS_SWND_LOG_MSG, strHeader);
	m_ListCtrl.InsertColumn(5, strHeader, LVCFMT_LEFT, m_tmWidth * 32, 5);

	// 全て削除
	m_ListCtrl.DeleteAllItems();
}

//---------------------------------------------------------------------------
//
//	リフレッシュ
//
//---------------------------------------------------------------------------
void FASTCALL CLogWnd::Refresh()
{
	// Updateでメッセージスレッドから処理するため、何もしない
}

//---------------------------------------------------------------------------
//
//	メッセージスレッドからの更新
//
//---------------------------------------------------------------------------
void FASTCALL CLogWnd::Update()
{
	int nItem;
	int nLog;
	BOOL bRun;
	CString strEmpty;

	ASSERT(this);
	ASSERT_VALID(this);

	// 無効なら何もしない
	if (!m_bEnable) {
		return;
	}

	// 現在の数を得て、アイテム数を調整
	nItem = m_ListCtrl.GetItemCount();
	nLog = m_pLog->GetNum();

	// 縮小
	if (nItem > nLog) {
		m_ListCtrl.DeleteAllItems();
		nItem = 0;
	}

	// 拡大
	if (nItem < nLog) {
		// ダミー文字列を用意
		strEmpty.Empty();

		// 1024アイテム以上の場合、一旦VMを止めて、アイテムを増やす
		if ((nLog - nItem) >= 0x400) {
			// この拡大は時間がかかるため、VMを止める
			bRun = m_pSch->IsEnable();
			m_pSch->Enable(FALSE);

			// 拡大
			while (nItem < nLog) {
				m_ListCtrl.InsertItem(nItem, strEmpty);
				nItem++;
			}

			// VMを復元
			m_pSch->Enable(bRun);
			m_pSch->Reset();
		}
		else {
			// 1023アイテム以下なので、VMを実行しながら拡大
			while (nItem < nLog) {
				m_ListCtrl.InsertItem(nItem, strEmpty);
				nItem++;
			}
		}
	}

	// 特に描画指示はしない方がよい結果をもたらす
}

//---------------------------------------------------------------------------
//
//	文字列セット
//
//---------------------------------------------------------------------------
void FASTCALL CLogWnd::TextSub(int nType, Log::logdata_t *pLogData, CString& string)
{
	DWORD dwTime;
	CString strTemp;
#if defined(UNICODE)
	wchar_t szWide[100];
#endif	// UNICODE

	ASSERT(this);
	ASSERT((nType >= 0) && (nType <= 5));
	ASSERT(pLogData);

	switch (nType) {
		// 通し番号
		case 0:
			string.Format("%d", pLogData->number);
			return;

		// 時間 (s.ms.us)
		case 1:
			dwTime = pLogData->time;
			dwTime /= 2;
			strTemp.Format("%d.", dwTime / (1000 * 1000));
			string = strTemp;
			dwTime %= (1000 * 1000);
			strTemp.Format("%03d.", dwTime / 1000);
			string += strTemp;
			dwTime %= 1000;
			strTemp.Format("%03d", dwTime);
			string += strTemp;
			return;

		// PC
		case 2:
			string.Format("$%06X", pLogData->pc);
			return;

		// レベル
		case 3:
			switch (pLogData->level) {
				// 詳細
				case Log::Detail:
					string = m_strDetail;
					break;
				// 通常
				case Log::Normal:
					string = m_strNormal;
					break;
				// 警告
				case Log::Warning:
					string = m_strWarning;
					break;
				// その他(ありえない)
				default:
					ASSERT(FALSE);
					string.Empty();
					break;
			}
			return;

		// デバイス
		case 4:
			string.Format(_T("%c%c%c%c"),
						(BYTE)(pLogData->id >> 24),
						(BYTE)(pLogData->id >> 16),
						(BYTE)(pLogData->id >> 8),
						(BYTE)(pLogData->id));
			return;

		// メッセージ
		case 5:
#if defined(UNICODE)
			// ここでCP932から変換しておく
			if (::Support932()) {
				// シフトJISから、UNICODEへ変換する
				::MultiByteToWideChar(932, MB_PRECOMPOSED, (LPCSTR)pLogData->string, -1, szWide, 100);

				// セット
				string = szWide;
			}
			else {
				if (::IsWinNT()) {
					// UNICODEサポートされているが、CP932が無い
					string = _T("CP932(Japanese Shift-JIS) is required");
				}
				else {
					// UNICODE指定なのに、Win9xで動かしている
					string = _T("(Japanese Text)");
				}
			}
#else
			// UNICODEでないので、stringもマルチバイト
			string = pLogData->string;
#endif	// UNICODE
			return;

		// その他(ありえない)
		default:
			break;
	}

	// 通常、ここにはこない
	ASSERT(FALSE);
}

//===========================================================================
//
//	スケジューラウィンドウ
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	コンストラクタ
//
//---------------------------------------------------------------------------
CSchedulerWnd::CSchedulerWnd()
{
	// ウィンドウパラメータ定義
	m_dwID = MAKEID('S', 'C', 'H', 'E');
	::GetMsg(IDS_SWND_SCHEDULER, m_strCaption);
	m_nWidth = 72;
	m_nHeight = 23;

	// スケジューラ取得
	m_pScheduler = (Scheduler*)::GetVM()->SearchDevice(MAKEID('S', 'C', 'H', 'E'));
	ASSERT(m_pScheduler);
}

//---------------------------------------------------------------------------
//
//	セットアップ
//
//---------------------------------------------------------------------------
void FASTCALL CSchedulerWnd::Setup()
{
	int x;
	int y;
	int nIndex;
	DWORD dwRate;
	Event *pEvent;
	Device *pDevice;
	DWORD dwID;
	CString strText;
	Scheduler::scheduler_t sch;

	ASSERT(this);
	ASSERT(m_pSch);
	ASSERT(m_pScheduler);

	// スケジューラ内部ワーク取得
	m_pScheduler->GetScheduler(&sch);

	// クリア
	Clear();
	x = 44;

	// 左半分
	y = 0;

	// トータル実行時間
	SetString(0, y, _T("Passed Time"));
	strText.Format(_T("%12dms"), sch.total / 2000);
	SetString(15, y, strText);
	y++;

	// 微少実行時間
	SetString(0, y, _T("Execution Time"));
	strText.Format(_T("%6d.%1dus"), sch.one / 2, (sch.one & 1) * 5);
	SetString(19, y, strText);
	y++;

	// フレームレート
	dwRate = m_pSch->GetFrameRate();
	SetString(0, y, _T("Frame Rate"));
	strText.Format(_T("%2d.%1dfps"), dwRate / 10, (dwRate % 10));
	SetString(22, y, strText);

	// 右半分
	y = 0;

	// CPU情報(実効速度)
	SetString(x, y, _T("MPU Speed"));
	strText.Format(_T("%d.%02dMHz"), sch.speed / 100, sch.speed % 100);
	if (sch.speed >= 1000) {
		SetString(x + 20, y, strText);
	}
	else {
		SetString(x + 21, y, strText);
	}
	y++;

	// CPU情報(微小時間サイクル)
	SetString(x, y, _T("MPU Over Cycle"));
	strText.Format(_T("%5u"), sch.cycle);
	SetString(x + 23, y, strText);
	y++;

	// CPU情報(微小時間)
	SetString(x, y, _T("MPU Over Time"));
	strText.Format(_T("%3d.%1dus"), sch.time / 2, (sch.time & 1) * 5);
	SetString(x + 21, y, strText);
	y++;

	// イベントガイド
	y++;
	SetString(0, y, _T("No."));
	SetString(6, y, _T("Remain"));
	SetString(20, y, _T("Time"));
	SetString(34, y, _T("User"));
	SetString(42, y, _T("Flag"));
	SetString(48, y, _T("Device"));
	SetString(56, y, _T("Description"));
	y++;

	// イベントループ
	pEvent = m_pScheduler->GetFirstEvent();
	nIndex = 0;
	while (pEvent) {
		// 基本パラメータ
		strText.Format(_T("%2d %5d.%04dms (%5d.%04dms) $%08X "),
			nIndex + 1,
			pEvent->GetRemain() / 2000,
			(pEvent->GetRemain() % 2000) * 5,
			pEvent->GetTime() / 2000,
			(pEvent->GetTime() % 2000) * 5,
			pEvent->GetUser());
		SetString(0, y, strText);

		// 有効フラグ
		if (pEvent->GetTime() != 0) {
			SetString(42, y, _T("Enable"));
		}

		// デバイス名
		pDevice = pEvent->GetDevice();
		ASSERT(pDevice);
		dwID = pDevice->GetID();
		strText.Format(_T("%c%c%c%c"),
						(BYTE)(dwID >> 24),
						(BYTE)(dwID >> 16),
						(BYTE)(dwID >> 8),
						(BYTE)dwID);
		SetString(49, y, strText);

		// 名称
		SetString(54, y, pEvent->GetDesc());

		// 次のイベントへ
		pEvent = pEvent->GetNextEvent();
		nIndex++;
		y++;
	}
}

//---------------------------------------------------------------------------
//
//	メッセージスレッドからの更新
//
//---------------------------------------------------------------------------
void FASTCALL CSchedulerWnd::Update()
{
	int nNum;

	ASSERT(this);

	// スケジューラのもつイベント数を取得し、リサイズ
	nNum = m_pScheduler->GetEventNum();
	ReSize(m_nWidth, nNum + 5);
}

//===========================================================================
//
//	デバイスウィンドウ
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	コンストラクタ
//
//---------------------------------------------------------------------------
CDeviceWnd::CDeviceWnd()
{
	// ウィンドウパラメータ定義
	m_dwID = MAKEID('D', 'E', 'V', 'I');
	::GetMsg(IDS_SWND_DEVICE, m_strCaption);

	m_nWidth = 57;
	m_nHeight = 32;
}

//---------------------------------------------------------------------------
//
//	セットアップ
//
//---------------------------------------------------------------------------
void FASTCALL CDeviceWnd::Setup()
{
	int nDevice;
	const Device *pDevice;
	const MemDevice *pMemDev;
	CString strText;
	CString strTemp;
	DWORD dwID;
	DWORD dwMemID;
	BOOL bMem;

	ASSERT(this);

	// クリア
	Clear();

	// 最初のデバイスを取得、メモリID作成
	pDevice = ::GetVM()->GetFirstDevice();
	ASSERT(pDevice);
	dwMemID = MAKEID('M', 'E', 'M', ' ');
	bMem = FALSE;

	SetString(0, 0, _T("No. Device Object"));
	SetString(23, 0, _T("Address Range"));
	SetString(41, 0, _T("Description"));

	// ループ
	nDevice = 0;
	while (pDevice) {
		// 番号
		strText.Format(_T("%2d  "), nDevice + 1);

		// ID
		dwID = pDevice->GetID();
		strTemp.Format(_T("%c%c%c%c"),
						(BYTE)(dwID >> 24),
						(BYTE)(dwID >> 16),
						(BYTE)(dwID >> 8),
						(BYTE)(dwID));
		strText += strTemp;

		// ポインタ
		strTemp.Format(_T("  %08x  "), pDevice);
		strText += strTemp;

		// メモリフラグ
		if (dwID == dwMemID) {
			bMem = TRUE;
		}

		// メモリ範囲
		if (bMem) {
			pMemDev = (MemDevice *)pDevice;
			strTemp.Format(_T("$%06X - $%06X"),
						pMemDev->GetFirstAddr(), pMemDev->GetLastAddr());
		}
		else {
			strTemp = _T("(NO MEMORY)");
		}
		strText += strTemp;

		// セット
		SetString(0, nDevice + 1, strText);

		// 名称
		strText = pDevice->GetDesc();
		SetString(39, nDevice + 1, strText);

		// 次へ
		nDevice++;
		pDevice = pDevice->GetNextDevice();
	}
}

#endif	// _WIN32
