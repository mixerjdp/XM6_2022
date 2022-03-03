//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 ＰＩ．(ytanaka@ipc-tokai.or.jp)
//	[ MFC Info ]
//
//---------------------------------------------------------------------------

#if defined(_WIN32)

#include "os.h"
#include "xm6.h"
#include "vm.h"
#include "schedule.h"
#include "sasi.h"
#include "scsi.h"
#include "fdd.h"
#include "fdi.h"
#include "rtc.h"
#include "memory.h"
#include "fileio.h"
#include "config.h"
#include "mfc_frm.h"
#include "mfc_com.h"
#include "mfc_stat.h"
#include "mfc_sch.h"
#include "mfc_cfg.h"
#include "mfc_res.h"
#include "mfc_info.h"

//===========================================================================
//
//	Info
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	コンストラクタ
//
//---------------------------------------------------------------------------
CInfo::CInfo(CFrmWnd *pWnd, CStatusBar *pBar) : CComponent(pWnd)
{
	int nPane;

	// コンポーネントパラメータ
	m_dwID = MAKEID('I', 'N', 'F', 'O');
	m_strDesc = _T("Info Center");

	// キャプション
	m_strRun.Empty();
	m_strStop.Empty();
	m_szVM[0] = _T('\0');
	m_szVMFull[0] = _T('\0');
	m_szMO[0] = _T('\0');
	m_szMOFull[0] = _T('\0');
	m_szCD[0] = _T('\0');
	m_szCDFull[0] = _T('\0');
	m_bRun = FALSE;
	m_nParcent = -1;
	m_bCount = FALSE;
	m_dwTime = 0;
	m_dwTick = 0;
	m_pSch = NULL;
	m_pScheduler = NULL;
	m_pSASI = NULL;
	m_pSCSI = NULL;

	// 情報
	m_bInfo = FALSE;
	m_bPower = FALSE;
	m_dwInfo = 0;
	m_strInfo.Empty();
	m_szInfo[0] = _T('\0');

	// ステータス
	for (nPane=0; nPane<PaneMax; nPane++) {
		m_colStatus[nPane] = (COLORREF)-1;
		m_szStatus[nPane][0] = _T('\0');
	}
	m_bFloppyLED = FALSE;
	m_bPowerLED = FALSE;
	m_bCaptionInfo = FALSE;
	m_pFDD = NULL;
	m_pRTC = NULL;
	m_dwNumber = 0;
	m_dwDiskID = 0;
	m_dwDiskTime = DiskTypeTime;

	// ステータスバー
	ASSERT(pBar);
	m_pStatusBar = pBar;

	// ステータスビュー
	m_pStatusView = NULL;
}

//---------------------------------------------------------------------------
//
//	初期化
//
//---------------------------------------------------------------------------
BOOL FASTCALL CInfo::Init()
{
	CString strIdle;

	ASSERT(this);

	// 基本クラス
	if (!CComponent::Init()) {
		return FALSE;
	}

	// キャプション
	::GetMsg(IDS_CAPTION_RUN, m_strRun);
	::GetMsg(IDS_CAPTION_STOP, m_strStop);
	m_pSch = (CScheduler*)SearchComponent(MAKEID('S', 'C', 'H', 'E'));
	ASSERT(m_pSch);
	m_pScheduler = (Scheduler*)::GetVM()->SearchDevice(MAKEID('S', 'C', 'H', 'E'));
	ASSERT(m_pScheduler);
	m_pSASI = (SASI*)::GetVM()->SearchDevice(MAKEID('S', 'A', 'S', 'I'));
	ASSERT(m_pSASI);
	m_pSCSI = (SCSI*)::GetVM()->SearchDevice(MAKEID('S', 'C', 'S', 'I'));
	ASSERT(m_pSCSI);

	// 情報
	::SetInfoMsg(m_szInfo, TRUE);
	::GetMsg(AFX_IDS_IDLEMESSAGE, strIdle);
	if (strIdle.GetLength() < 0x100) {
		_tcscpy(m_szInfo, (LPCTSTR)strIdle);
	}
	::GetMsg(IDS_POWEROFF, m_strPower);
	m_bPower = ::GetVM()->IsPower();

	// ステータス
	m_pFDD = (FDD*)::GetVM()->SearchDevice(MAKEID('F', 'D', 'D', ' '));
	ASSERT(m_pFDD);
	m_pRTC = (RTC*)::GetVM()->SearchDevice(MAKEID('R', 'T', 'C', ' '));
	ASSERT(m_pRTC);

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	クリーンアップ
//
//---------------------------------------------------------------------------
void FASTCALL CInfo::Cleanup()
{
	ASSERT(this);


	// 基本クラス
	CComponent::Cleanup();
}

//---------------------------------------------------------------------------
//
//	設定適用
//
//---------------------------------------------------------------------------
void FASTCALL CInfo::ApplyCfg(const Config* pConfig)
{
	ASSERT(this);
	ASSERT(pConfig);

	// ステータス
	m_bFloppyLED = pConfig->floppy_led;
	m_bPowerLED = pConfig->power_led;
	m_bCaptionInfo = pConfig->caption_info;
}

//---------------------------------------------------------------------------
//
//	動作制御
//
//---------------------------------------------------------------------------
void FASTCALL CInfo::Enable(BOOL bEnable)
{
	CConfig *pConfig;
	Config cfg;
	int i;
	int x;
	int y;
	BITMAPINFOHEADER *bmi;
	Memory *pMemory;
	const BYTE *pCG;
	const BYTE *pChr;
	BYTE *pBits;
	BYTE byteFont;

	// 有効にする場合(起動初回を意味する)
	if (bEnable) {
		// コンフィグマネージャを取得
		pConfig = (CConfig*)SearchComponent(MAKEID('C', 'F', 'G', ' '));
		ASSERT(pConfig);

		// コンフィグデータを取得
		pConfig->GetConfig(&cfg);

		// 電源ON起動、OFF起動で分ける
		if (cfg.power_off) {
			// 電源OFFから
			m_bPower = FALSE;
			m_bRun = FALSE;
			m_bCount = TRUE;
		}
		else {
			// 電源ONから
			m_bPower = TRUE;
			m_bRun = TRUE;
			m_bCount = FALSE;
		}
		m_dwTick = ::GetTickCount();

		// ドライブ用ビットマップ作成
		pMemory = (Memory*)::GetVM()->SearchDevice(MAKEID('M', 'E', 'M', ' '));
		ASSERT(pMemory);
		pCG = pMemory->GetCG();
		ASSERT(pCG);

		// ドライブループ
		for (i=0; i<2; i++) {
			// サイズチェック
			ASSERT((sizeof(BITMAPINFOHEADER) + 8 * 8 * 3) <= sizeof(m_bmpDrive[i]));

			// クリア
			memset(m_bmpDrive[i], 0, sizeof(m_bmpDrive[i]));

			// ビットマップヘッダ設定
			bmi = (BITMAPINFOHEADER*)&m_bmpDrive[i][0];
			bmi->biSize = sizeof(BITMAPINFOHEADER);
			bmi->biWidth = 8;
			bmi->biHeight = -8;
			bmi->biPlanes = 1;
			bmi->biBitCount = 24;
			bmi->biCompression = BI_RGB;
			bmi->biSizeImage = 8 * 8 * 3;

			// アドレス設定
			pChr = pCG + 0x3a000 + (('0' + i) << 3);
			pBits = &m_bmpDrive[i][sizeof(BITMAPINFOHEADER)];

			// x, yループ
			for (y=0; y<8; y++) {
				// CGROMからデータ取得
				byteFont = pChr[y ^ 1];

				// 1ビットごとに検査
				for (x=0; x<8; x++) {
					if (byteFont & 0x80) {
						pBits[0] = 208;
						pBits[1] = 208;
						pBits[2] = 208;
					}
					byteFont <<= 1;
					pBits += 3;
				}
			}
		}
	}

	// 基本クラス
	CComponent::Enable(bEnable);
}

//===========================================================================
//
//	キャプション
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	キャプションリセット
//
//---------------------------------------------------------------------------
void FASTCALL CInfo::ResetCaption()
{
	CString strCap;

	ASSERT(this);
	ASSERT(m_pSch);

	// 現在の状態と反転
	m_bRun = !m_pSch->IsEnable();

	// 更新アルゴリズムの逆を設定
	if (m_bRun) {
		m_bCount = FALSE;
		m_nParcent = -1;
	}
	else {
		m_bCount = TRUE;
		m_nParcent = -1;
	}

	// 現在の状態からメッセージを作る
	if (m_pSch->IsEnable()) {
		strCap = m_strRun;
	}
	else {
		strCap = m_strStop;
	}

	// 設定
	m_pFrmWnd->SetWindowText(strCap);
	if (m_pStatusView) {
		m_pStatusView->SetCaptionString(strCap);
	}
}

//---------------------------------------------------------------------------
//
//	キャプション更新
//
//---------------------------------------------------------------------------
void FASTCALL CInfo::UpdateCaption()
{
	BOOL bParcent;
	BOOL bVM;
	BOOL bMO;
	BOOL bCD;

	ASSERT(this);

	// ％チェック
	bParcent = CheckParcent();

	// VMチェック
	bVM = CheckVM();

	// MOチェック
	bMO = CheckMO();

	// CDチェック
	bCD = CheckCD();

	// どれか異なっていたら、キャプション設定
	if (bParcent || bVM || bMO || bCD) {
		SetCaption();
	}
}

//---------------------------------------------------------------------------
//
//	％チェック
//
//---------------------------------------------------------------------------
BOOL FASTCALL CInfo::CheckParcent()
{
	BOOL bRun;
	DWORD dwDiff;
	DWORD dwTick;
	int nParcent;

	ASSERT(this);
	ASSERT(m_pSch);
	ASSERT(m_pScheduler);

	// 前回の状態をバックアップ
	bRun = m_bRun;

	// 今回の動作状態を取得
	m_bRun = m_pSch->IsEnable();

	// 停止になっていれば特別処理
	if (!m_bRun) {
		// 前回も停止なら
		if (!bRun) {
			// 計測ありになっていれば下ろして、更新あり
			if (m_bCount) {
				m_bCount = FALSE;
				return TRUE;
			}
			return FALSE;
		}

		// 計測なし
		m_bCount = FALSE;

		// ％表示なし
		m_nParcent = -1;

		// 変更あり
		return TRUE;
	}

	// 計測開始していなければ特別処理(STOP→RUNはここに入る)
	if (!m_bCount || !bRun) {
		// 計測開始
		m_bCount = TRUE;
		m_dwTick = ::GetTickCount();
		m_dwTime = m_pScheduler->GetTotalTime();

		// ％表示なし
		m_nParcent = -1;

		// 変更あり
		return TRUE;
	}

	// RUN→RUNの場合のみ
	ASSERT(m_bCount);
	ASSERT(bRun);
	ASSERT(m_bRun);

	// 時間差分を見る
	dwDiff = ::GetTickCount();
	dwDiff -= m_dwTick;
	if (m_nParcent >= 0) {
		// 既に％表示中の場合
		if (dwDiff < CapTimeLong) {
			// 変更なし
			return FALSE;
		}
	}
	else {
		// まだ％表示していない場合
		if (dwDiff < CapTimeShort) {
			// 変更なし
			return FALSE;
		}
	}

	// 時間差、実行タイム差を得る
	dwTick = ::GetTickCount();
	m_dwTime = m_pScheduler->GetTotalTime() - m_dwTime;
	m_dwTick = dwTick - m_dwTick;

	// 除算により％を算出
	if ((m_dwTime == 0) || (m_dwTick == 0)) {
		nParcent = 0;
	}
	else {
		// VM側は0.5us単位
		nParcent = (int)m_dwTime;
		nParcent /= (int)m_dwTick;
		if (nParcent > 0) {
			nParcent /= 2;
		}
	}

	// 再セット
	m_dwTime = m_pScheduler->GetTotalTime();
	m_dwTick = dwTick;

	// ％加工(4捨5入)
	if ((nParcent % 10) >= 5) {
		nParcent /= 10;
		nParcent++;
	}
	else {
		nParcent /= 10;
	}

	// 異なっていれば更新して、TRUE
	if (m_nParcent != nParcent) {
		m_nParcent = nParcent;
		return TRUE;
	}

	// 前回と同じ。FALSE
	return FALSE;
}

//---------------------------------------------------------------------------
//
//	VMファイルチェック
//
//---------------------------------------------------------------------------
BOOL FASTCALL CInfo::CheckVM()
{
	Filepath path;
	LPCTSTR lpszPath;
	LPCTSTR lpszFileExt;

	ASSERT(this);
	ASSERT(m_pSASI);

	// VMパス取得
	::GetVM()->GetPath(path);
	lpszPath = path.GetPath();

	// フルとパス文字列が一致しているかどうか調べる
	if (_tcscmp(lpszPath, m_szVMFull) == 0) {
		// 変更なし
		return FALSE;
	}

	// フルに全てコピー
	_tcscpy(m_szVMFull, lpszPath);

	// ファイル名＋拡張子のみ取り出す
	lpszFileExt = path.GetFileExt();

	// 更新
	_tcscpy(m_szVM, lpszFileExt);

	// 変更あり
	return TRUE;
}

//---------------------------------------------------------------------------
//
//	MOファイルチェック
//
//---------------------------------------------------------------------------
BOOL FASTCALL CInfo::CheckMO()
{
	Filepath path;
	LPCTSTR lpszPath;
	LPCTSTR lpszFileExt;

	ASSERT(this);
	ASSERT(m_pSASI);

	// MOパス取得
	m_pSASI->GetPath(path);
	lpszPath = path.GetPath();

	// フルとパス文字列が一致しているかどうか調べる
	if (_tcscmp(lpszPath, m_szMOFull) == 0) {
		// 変更なし
		return FALSE;
	}

	// フルに全てコピー
	_tcscpy(m_szMOFull, lpszPath);

	// ファイル名＋拡張子のみ取り出す
	lpszFileExt = path.GetFileExt();

	// 更新
	_tcscpy(m_szMO, lpszFileExt);

	// 変更あり
	return TRUE;
}

//---------------------------------------------------------------------------
//
//	CDファイルチェック
//
//---------------------------------------------------------------------------
BOOL FASTCALL CInfo::CheckCD()
{
	Filepath path;
	LPCTSTR lpszPath;
	LPCTSTR lpszFileExt;

	ASSERT(this);
	ASSERT(m_pSCSI);

	// CDパス取得
	m_pSCSI->GetPath(path, FALSE);
	lpszPath = path.GetPath();

	// フルとパス文字列が一致しているかどうか調べる
	if (_tcscmp(lpszPath, m_szCDFull) == 0) {
		// 変更なし
		return FALSE;
	}

	// フルに全てコピー
	_tcscpy(m_szCDFull, lpszPath);

	// ファイル名＋拡張子のみ取り出す
	lpszFileExt = path.GetFileExt();

	// 更新
	_tcscpy(m_szCD, lpszFileExt);

	// 変更あり
	return TRUE;
}

//---------------------------------------------------------------------------
//
//	キャプション設定
//
//---------------------------------------------------------------------------
void FASTCALL CInfo::SetCaption()
{
	CString strCap;
	CString strSub;

	ASSERT(this);
	ASSERT(m_pFrmWnd);

	// 動作中 or 停止
	if (m_bRun) {
		// 動作中
		strCap = m_strRun;
	}
	else {
		// 停止中
		strCap = m_strStop;
	}

	// 設定(キャプションへ情報表示しない場合)
	if (m_bCaptionInfo == FALSE) {
		strCap = _T("XM6");

		// キャプション
		m_pFrmWnd->SetWindowText(strCap);

		// ステータスビュー
		if (m_pStatusView) {
			m_pStatusView->SetCaptionString(strCap);
		}
	}

	// %表示
	if (m_nParcent >= 0) {
		strSub.Format(_T(" - %3d%%"), m_nParcent);
		strCap += strSub;
	}

	// VM
	if (_tcslen(m_szVM) > 0) {
		strSub.Format(_T(" [%s] "), m_szVM);
		strCap += strSub;
	}

	// MO
	if (_tcslen(m_szMO) > 0) {
		strSub.Format(_T(" (%s) "), m_szMO);
		strCap += strSub;
	}

	// CD
	if (_tcslen(m_szCD) > 0) {
		strSub.Format(_T(" {%s} "), m_szCD);
		strCap += strSub;
	}

	//  設定(キャプションへ情報表示する場合)
	if (m_bCaptionInfo) {
		// キャプション
		m_pFrmWnd->SetWindowText(strCap);

		// ステータスビュー
		if (m_pStatusView) {
			m_pStatusView->SetCaptionString(strCap);
		}
	}
}

//===========================================================================
//
//	情報
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	情報設定
//
//---------------------------------------------------------------------------
void FASTCALL CInfo::SetInfo(CString& strInfo)
{
	ASSERT(this);

	// メッセージ記憶
	m_strInfo = strInfo;

	// フラグアップ、時間記憶
	m_bInfo = TRUE;
	m_dwInfo = ::GetTickCount();

	// ステータスバーへ設定
	ASSERT(m_pStatusBar);
	m_pStatusBar->SetPaneText(0, m_strInfo, TRUE);

	// ステータスビューへ設定
	if (m_pStatusView) {
		m_pStatusView->SetInfoString(m_strInfo);
	}
}

//---------------------------------------------------------------------------
//
//	通常メッセージ設定
//
//---------------------------------------------------------------------------
void FASTCALL CInfo::SetMessageString(const CString& strMessage) const
{
	LPCTSTR lpszMessage;

	// LPCTSTRへ変換
	lpszMessage = (LPCTSTR)strMessage;

	// const制約を抜けるため、C形式の関数を経由
	::SetInfoMsg(lpszMessage, FALSE);
}

//---------------------------------------------------------------------------
//
//	情報更新
//
//---------------------------------------------------------------------------
void FASTCALL CInfo::UpdateInfo()
{
	BOOL bPower;
	DWORD dwDiff;
	CString strText;

	ASSERT(this);

	// 電源状態をチェック
	bPower = ::GetVM()->IsPower();
	if (m_bPower && !bPower) {
		// 電源がON→OFFに遷移した。強制メッセージ
		SetInfo(m_strPower);
	}
	// 電源状態を常に更新
	m_bPower = bPower;

	// 情報有無フラグOFFなら
	if (!m_bInfo) {
		// 何もしない
		return;
	}

	// 時間計測。2000msで消去
	dwDiff = ::GetTickCount();
	dwDiff -= m_dwInfo;
	if (dwDiff < InfoTime) {
		// 表示中
		return;
	}

	// 情報有無フラグOFF
	m_bInfo = FALSE;

	// 通常メッセージを復元
	strText = m_szInfo;

	// ステータスバーへ設定
	ASSERT(m_pStatusBar);
	m_pStatusBar->SetPaneText(0, strText, TRUE);

	// ステータスビューへ設定
	if (m_pStatusView) {
		strText.Empty();
		m_pStatusView->SetInfoString(strText);
	}
}

//===========================================================================
//
//	ステータス
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	ステータスリセット
//
//---------------------------------------------------------------------------
void FASTCALL CInfo::ResetStatus()
{
	int nPane;

	ASSERT(this);

	// ペインをすべてクリア
	for (nPane=0; nPane<PaneMax; nPane++) {
		// 不定色
		m_colStatus[nPane] = (COLORREF)-1;

		// 表示テキストなし
		m_szStatus[nPane][0] = _T('\0');
	}
}

//---------------------------------------------------------------------------
//
//	ステータス更新
//
//---------------------------------------------------------------------------
void FASTCALL CInfo::UpdateStatus()
{
	COLORREF colStatus[PaneMax];
	TCHAR szStatus[PaneMax][_MAX_PATH];
	int nPane;
	BOOL bDraw;
	BOOL bAll;
	CString strNumber;

	ASSERT(this);
	ASSERT(m_pStatusBar);

	// VMロック
	::LockVM();

	// テキストとカラーを作成
	ASSERT(PaneMax == 5);
	colStatus[0] = StatusFloppy(szStatus[0], 0);
	colStatus[1] = StatusFloppy(szStatus[1], 1);
	colStatus[2] = StatusHardDisk(szStatus[2]);
	colStatus[3] = StatusTimer(szStatus[3]);
	colStatus[4] = StatusPower(szStatus[4]);

	// VMアンロック
	::UnlockVM();

	// ナンバ文字列作成(毎回の更新で、異なる文字列を作成)
	strNumber.Format(_T("%08X"), m_dwNumber);

	// 比較ループと描画
	bAll = FALSE;
	for (nPane=0; nPane<5; nPane++) {
		// 比較し、描画するかどうかを決める
		bDraw = FALSE;
		if (m_colStatus[nPane] != colStatus[nPane]) {
			bDraw = TRUE;
		}
		if (_tcscmp(m_szStatus[nPane], szStatus[nPane]) != 0) {
			bDraw = TRUE;
		}

		// 描画する必要がなければ次へ
		if (!bDraw) {
			continue;
		}

		// コピー
		m_colStatus[nPane] = colStatus[nPane];
		_tcscpy(m_szStatus[nPane], szStatus[nPane]);

		// 描画
		m_pStatusBar->SetPaneText(nPane + 1, strNumber, TRUE);

		// ステータスビュー
		if (m_pStatusView) {
			m_pStatusView->DrawStatus(nPane);
		}

		// フラグUp
		bAll = TRUE;
	}

	// ナンバ更新
	if (bAll) {
		m_dwNumber++;
	}
}

//---------------------------------------------------------------------------
//
//	ステータスFD
//
//---------------------------------------------------------------------------
COLORREF FASTCALL CInfo::StatusFloppy(LPTSTR lpszText, int nDrive) const
{
	COLORREF colStatus;
	int nStatus;
	BOOL bPower;
	FDD::drv_t drv;
	char name[60];
	LPCTSTR lpszName;

	ASSERT(this);
	ASSERT(lpszText);
	ASSERT((nDrive == 0) || (nDrive == 1));
	ASSERT(m_pFDD);
	ASSERT(m_pSch);

	// 初期化、FDDデータを得る
	bPower = ::GetVM()->IsPower();
	lpszText[0] = _T('\0');
	colStatus = RGB(1, 1, 1);
	nStatus = m_pFDD->GetStatus(nDrive);

	// 点滅中か
	if (nStatus & FDST_BLINK) {
		// 点滅中。点と滅のどちらか
		if ((nStatus & FDST_CURRENT) && bPower) {
			// 緑
			colStatus = RGB(15, 159, 15);
		}
		else {
			// 黒
			colStatus = RGB(0, 0, 0);
		}
	}

	// 挿入または誤挿入か
	if (nStatus & FDST_INSERT) {
		// 灰
		colStatus = RGB(95, 95, 95);

		// ディスク名取得
		m_pFDD->GetName(nDrive, name);
		lpszName = A2CT(name);
		_tcscpy(lpszText, lpszName);

		// アクセス中
		if (m_bFloppyLED) {
			// モータON&セレクトなら赤
			if ((nStatus & FDST_MOTOR) && (nStatus & FDST_SELECT)) {
				colStatus = RGB(208, 31, 31);
			}
		}
		else {
			// アクセス中なら赤
			if (nStatus & FDST_ACCESS) {
				colStatus = RGB(208, 31, 31);
			}
		}

		// 電源OFFなら必ず灰色
		if (!bPower) {
			colStatus = RGB(95, 95, 95);
		}
	}

	// スケジューラ停止中の場合の特例
	if (!m_pSch->IsEnable()) {
		// ドライブ詳細を取得
		m_pFDD->GetDrive(nDrive, &drv);
		if (!(nStatus & FDST_INSERT)) {
			// ディスクは挿入されていない
			if (drv.next) {
				// 次ディスクがある
				colStatus = RGB(95, 95, 95);

				drv.next->GetName(name, 0);
				lpszName = A2CT(name);
				_tcscpy(lpszText, lpszName);
			}
		}
	}

	// イジェクト禁止なら最上位を立てる
	if (!(nStatus & FDST_EJECT)) {
		colStatus |= (COLORREF)0x80000000;
	}

	return colStatus;
}

//---------------------------------------------------------------------------
//
//	ステータスHD
//
//---------------------------------------------------------------------------
COLORREF FASTCALL CInfo::StatusHardDisk(LPTSTR lpszText)
{
	DWORD dwID;
	COLORREF color;

	ASSERT(this);
	ASSERT(lpszText);
	ASSERT(m_pSASI);

	// 電源チェック
	if (!::GetVM()->IsPower()) {
		// 電源が入っていない。黒
		_tcscpy(lpszText, _T("HD BUSY"));
		m_dwDiskID = 0;
		m_dwDiskTime = DiskTypeTime;
		return RGB(0, 0, 0);
	}

	// デバイス取得
	dwID = m_pSASI->GetBusyDevice();
	switch (dwID) {
		// SASI-HD
		case MAKEID('S', 'A', 'H', 'D'):
			color = RGB(208, 31, 31);
			break;

		// SCSI-HD
		case MAKEID('S', 'C', 'H', 'D'):
			color = RGB(208, 31, 31);
			break;

		// SCSI-MO
		case MAKEID('S', 'C', 'M', 'O'):
			color = RGB(208, 32, 31);
			break;

		// SCSI-CD
		case MAKEID('S', 'C', 'C', 'D'):
			color = RGB(208, 31, 32);
			break;

		// BUSYでない
		case 0:
			color = RGB(0, 0, 0);
			break;

		// その他(ありえない)
		default:
			ASSERT(FALSE);
			_tcscpy(lpszText, _T("HD BUSY"));
			return RGB(0, 0, 0);
	}

	// 有効なデバイスの場合
	if (color != RGB(0, 0, 0)) {
		// 前回のデバイスと同じか
		if (dwID == m_dwDiskID) {
			// 時間を+1(最大はDiskTypeTime)
			if (m_dwDiskTime < DiskTypeTime) {
				m_dwDiskTime++;
			}
		}
		else {
			// デバイスが切り替わったので、記憶して時間を初期化
			m_dwDiskID = dwID;
			m_dwDiskTime = 0;
		}
	}
	else {
		// BUSYでなければ、時間チェック
		if (m_dwDiskTime >= DiskTypeTime) {
			// HD BUSYに戻す
			ASSERT(m_dwDiskTime == DiskTypeTime);
			_tcscpy(lpszText, _T("HD BUSY"));
			m_dwDiskID = 0;

			// 黒
			return RGB(0, 0, 0);
		}

		// しばらく、黒に近い色で、前回のデバイスで表示
		m_dwDiskTime++;
		dwID = m_dwDiskID;
		color = RGB(0, 0, 1);
	}

	// 文字列作成
	switch (dwID) {
		// SASI-HD
		case MAKEID('S', 'A', 'H', 'D'):
			_tcscpy(lpszText, _T("HD BUSY"));
			break;

		// SCSI-HD
		case MAKEID('S', 'C', 'H', 'D'):
			_tcscpy(lpszText, _T("HD BUSY"));
			break;

		// SCSI-MO
		case MAKEID('S', 'C', 'M', 'O'):
			_tcscpy(lpszText, _T("MO BUSY"));
			break;

		// SCSI-CD
		case MAKEID('S', 'C', 'C', 'D'):
			_tcscpy(lpszText, _T("CD BUSY"));
			break;

		// その他(ありえない)
		default:
			ASSERT(FALSE);
	}

	return color;
}

//---------------------------------------------------------------------------
//
//	ステータスTIMER
//
//---------------------------------------------------------------------------
COLORREF FASTCALL CInfo::StatusTimer(LPTSTR lpszText) const
{
	ASSERT(this);
	ASSERT(lpszText);
	ASSERT(m_pRTC);

	// テキスト
	_tcscpy(lpszText, _T("TIMER"));

	// タイマーONなら赤(電源は関係ない)
	if (m_pRTC->GetTimerLED()) {
		return RGB(208, 31, 31);
	}

	// 黒
	return RGB(0, 0, 0);
}

//---------------------------------------------------------------------------
//
//	ステータスPOWER
//
//---------------------------------------------------------------------------
COLORREF FASTCALL CInfo::StatusPower(LPTSTR lpszText) const
{
	VM *pVM;

	ASSERT(this);
	ASSERT(lpszText);
	ASSERT(m_pRTC);

	// VM取得
	pVM = m_pRTC->GetVM();
	ASSERT(pVM);

	// テキスト
	_tcscpy(lpszText, _T("POWER"));

	// 電源OFFか
	if (!pVM->IsPower()) {
		if (m_bPowerLED) {
			// 暗青
			return RGB(12, 23, 129);
		}
		else {
			// 赤
			return RGB(208, 31, 31);
		}
	}

	// 電源スイッチはONか
	if (pVM->IsPowerSW()) {
		// 緑または青
		if (m_bPowerLED) {
			return RGB(50, 50, 255);
		}
		else {
			return RGB(31, 208, 31);
		}
	}

	// 電源スイッチOFFの時は、ALARM出力によるトグル動作
	if (m_pRTC->GetAlarmOut()) {
		// 緑または青
		if (m_bPowerLED) {
			return RGB(50, 50, 255);
		}
		else {
			return RGB(31, 208, 31);
		}
	}
	if (m_bPowerLED) {
		// 暗青
		return RGB(12, 23, 129);
	}
	else {
		// 黒
		return RGB(0, 0, 0);
	}
}

//---------------------------------------------------------------------------
//
//	ステータス描画
//
//---------------------------------------------------------------------------
void FASTCALL CInfo::DrawStatus(int nPane, HDC hDC, CRect& rectDraw)
{
	BOOL bHalf;
	COLORREF colStatus;
	HDC hMemDC;
	HBITMAP hBitmap;
	HBITMAP hDefBitmap;
	HFONT hFont;
	HFONT hDefFont;
	CRect rectMem;
	CRect rectLine;
	int nLine;

	ASSERT(this);
	ASSERT((nPane >= 0) && (nPane < PaneMax));
	ASSERT(hDC);

	// カラーとハーフトーン(ライン抜き描画)の設定
	colStatus = m_colStatus[nPane];
	bHalf = FALSE;
	if (colStatus & 0x80000000) {
		bHalf = TRUE;
		colStatus &= (COLORREF)(0x7fffffff);
	}

	// メモリ矩形の設定
	rectMem.left = 0;
	rectMem.top = 0;
	rectMem.right = rectDraw.Width();
	rectMem.bottom = rectDraw.Height();

	// メモリDC作成
	hMemDC = ::CreateCompatibleDC(hDC);

	// ビットマップ作成、セレクト
	hBitmap = ::CreateCompatibleBitmap(hDC, rectDraw.Width(), rectDraw.Height());
	hDefBitmap = (HBITMAP)::SelectObject(hMemDC, hBitmap);
	ASSERT(hDefBitmap);

	// フォント作成、セレクト
	hFont = (HFONT)::GetStockObject(DEFAULT_GUI_FONT);
	hDefFont = (HFONT)::SelectObject(hMemDC, hFont);
	ASSERT(hDefFont);

	// 塗りつぶし
	::SetBkColor(hMemDC, colStatus);
	::ExtTextOut(hMemDC, 0, 0, ETO_OPAQUE, &rectMem, NULL, 0, NULL);

	// ハーフ処理
	if (bHalf) {
		// ラインおきに黒線を引く
		rectLine = rectMem;
		::SetBkColor(hMemDC, RGB(0, 0, 0));
		for (nLine=0; nLine<rectMem.bottom; nLine+=2) {
			rectLine.top = nLine;
			rectLine.bottom = nLine + 1;
			::ExtTextOut(hMemDC, 0, 0, ETO_OPAQUE, &rectLine, NULL, 0, NULL);
		}
		::SetBkColor(hMemDC, colStatus);
	}

	// テキスト描画(ただし、例外あり)
	if (!m_pStatusView || (nPane < 2) || (colStatus != 0)) {
		// 影を付けるため、1ドットだけ右下へシフト
		rectMem.left++;
		rectMem.top++;
		rectMem.right++;
		rectMem.bottom++;

		// 影を表示(黒)
		::SetTextColor(hMemDC, RGB(0, 0, 0));
		::SetBkMode(hMemDC, TRANSPARENT);
		::DrawText(hMemDC, m_szStatus[nPane], (int)_tcslen(m_szStatus[nPane]),
						 &rectMem, DT_NOPREFIX | DT_CENTER | DT_VCENTER | DT_SINGLELINE);

		// 戻すため、1ドットだけ左上へシフト
		rectMem.left--;
		rectMem.top--;
		rectMem.right--;
		rectMem.bottom--;

		// 本体を表示(白)
		::SetTextColor(hMemDC, RGB(255, 255, 255));
		::SetBkMode(hMemDC, TRANSPARENT);
		::DrawText(hMemDC, m_szStatus[nPane], (int)_tcslen(m_szStatus[nPane]),
						 &rectMem, DT_NOPREFIX | DT_CENTER | DT_VCENTER | DT_SINGLELINE);
	}

	// フロッピーディスクはドライブ番号を描画
	if ((nPane < 2) && (colStatus != 0)) {
		// 例外あり
		if (!m_pStatusView || (colStatus != RGB(1, 1, 1))) {
			::SetDIBitsToDevice(hMemDC, 0, 0, 8, 8, 0, 0, 0, 8,
								&m_bmpDrive[nPane][sizeof(BITMAPINFOHEADER)],
								(BITMAPINFO*)&m_bmpDrive[nPane][0],
								DIB_RGB_COLORS);
		}
	}

	// BitBlt
	::BitBlt(hDC, rectDraw.left, rectDraw.top, rectDraw.Width(), rectDraw.Height(),
						hMemDC, 0, 0, SRCCOPY);

	// フォント戻す
	::SelectObject(hMemDC, hDefFont);

	// ビットマップ戻す
	::SelectObject(hMemDC, hDefBitmap);
	::DeleteObject(hBitmap);

	// メモリDC戻す
	::DeleteDC(hMemDC);
}

//===========================================================================
//
//	その他
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	ステータスビュー通知
//
//---------------------------------------------------------------------------
void FASTCALL CInfo::SetStatusView(CStatusView *pView)
{
	ASSERT(this);

	// NULLにかかわらず、記憶
	m_pStatusView = pView;
}

#endif	// _WIN32
