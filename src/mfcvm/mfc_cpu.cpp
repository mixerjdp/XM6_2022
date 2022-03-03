//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 ＰＩ．(ytanaka@ipc-tokai.or.jp)
//	[ MFC サブウィンドウ(CPU) ]
//
//---------------------------------------------------------------------------

#if defined(_WIN32)

#include "os.h"
#include "xm6.h"
#include "vm.h"
#include "cpu.h"
#include "memory.h"
#include "schedule.h"
#include "mfp.h"
#include "scc.h"
#include "dmac.h"
#include "iosc.h"
#include "render.h"
#include "mfc_sub.h"
#include "mfc_draw.h"
#include "mfc_res.h"
#include "mfc_cpu.h"

//---------------------------------------------------------------------------
//
//	cpudebug.cとの連絡
//
//---------------------------------------------------------------------------
#if defined(__cplusplus)
extern "C" {
#endif	// __cplusplus

void cpudebug_disassemble(int n);
										// 1行逆アセンブル
extern void (*cpudebug_put)(const char*);
										// 1行出力
extern DWORD debugpc;
										// 逆アセンブルPC

#if defined(__cplusplus)
}
#endif	// __cplusplus

static char debugbuf[0x200];
										// 出力バッファ

//===========================================================================
//
//	ヒストリ付きダイアログ
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	コンストラクタ
//
//---------------------------------------------------------------------------
CHistoryDlg::CHistoryDlg(UINT nID, CWnd *pParentWnd) : CDialog(nID, pParentWnd)
{
	// 初期化
	m_dwValue = 0;
	m_nBit = 32;
}

//---------------------------------------------------------------------------
//
//	ダイアログ初期化
//
//---------------------------------------------------------------------------
BOOL CHistoryDlg::OnInitDialog()
{
	int i;
	int nNum;
	DWORD *pData;
	CString strText;
	CComboBox *pComboBox;

	// 基本クラス
	if (!CDialog::OnInitDialog()) {
		return FALSE;
	}

	// マスク生成
	m_dwMask = 0;
	for (i=0; i<(int)m_nBit; i++) {
		m_dwMask <<= 1;
		m_dwMask |= 0x01;
	}

	// コンボボックスクリア
	pComboBox = (CComboBox*)GetDlgItem(IDC_ADDR_ADDRE);
	ASSERT(pComboBox);
	pComboBox->ResetContent();

	// コンボボックス追加
	nNum = *(int *)GetNumPtr();
	pData = GetDataPtr();
	for (i=0; i<nNum; i++) {
		if (pData[i] > m_dwMask) {
			// マスクより大きいので、一律32bitでOK
			strText.Format(_T("%08X"), pData[i]);
		}
		else {
			// マスク以下
			switch (m_nBit) {
				case 8:
					strText.Format(_T("%02X"), pData[i]);
					break;
				case 16:
					strText.Format(_T("%04X"), pData[i]);
					break;
				case 24:
					strText.Format(_T("%06X"), pData[i]);
					break;
				default:
					strText.Format(_T("%08X"), pData[i]);
					break;
			}
		}
		// 追加
		pComboBox->AddString(strText);
	}

	// dwValueは必ずマスク
	m_dwValue &= m_dwMask;
	switch (m_nBit) {
		case 8:
			strText.Format(_T("%02X"), m_dwValue);
			break;
		case 16:
			strText.Format(_T("%04X"), m_dwValue);
			break;
		case 24:
			strText.Format(_T("%06X"), m_dwValue);
			break;
		default:
			strText.Format(_T("%08X"), m_dwValue);
			break;
	}
	pComboBox->SetWindowText(strText);

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	ダイアログOK
//
//---------------------------------------------------------------------------
void CHistoryDlg::OnOK()
{
	CComboBox *pComboBox;
	CString strText;
	int i;
	int nHit;
	int nNum;
	DWORD *pData;

	// 入力数値を取得
	pComboBox = (CComboBox*)GetDlgItem(IDC_ADDR_ADDRE);
	ASSERT(pComboBox);
	pComboBox->GetWindowText(strText);
	m_dwValue = _tcstoul((LPCTSTR)strText, NULL, 16);

	// 既に入力されたものと同じかチェック
	nNum = *(int *)GetNumPtr();
	pData = GetDataPtr();
	nHit = -1;
	for (i=0; i<nNum; i++) {
		if (pData[i] == m_dwValue) {
			nHit = i;
			break;
		}
	}

	// 新規か、採用か
	if (nHit >= 0) {
		// 既にあるものと同じ。場所入れ替え
		for (i=(nHit - 1); i>=0; i--) {
			pData[i + 1] = pData[i];
		}
		pData[0] = m_dwValue;
	}
	else {
		// 新規。既存ものをひとつ下へ格下げ
		for (i=9; i>=1; i--) {
			pData[i] = pData[i - 1];
		}

		// 最新を[0]へ
		pData[0] = m_dwValue;

		// 10までは追加できる
		if (nNum < 10) {
			*(int *)GetNumPtr() = (nNum + 1);
		}
	}

	// dwValueをマスクしてOK
	m_dwValue &= m_dwMask;

	// 基本クラス
	CDialog::OnOK();
}

//===========================================================================
//
//	アドレス入力ダイアログ
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	コンストラクタ
//
//---------------------------------------------------------------------------
CAddrDlg::CAddrDlg(CWnd *pParent) : CHistoryDlg(IDD_ADDRDLG, pParent)
{
	// 英語環境への対応
	if (!::IsJapanese()) {
		m_lpszTemplateName = MAKEINTRESOURCE(IDD_US_ADDRDLG);
		m_nIDHelp = IDD_US_ADDRDLG;
	}

	m_nBit = 24;
}

//---------------------------------------------------------------------------
//
//	メニューセットアップ
//
//---------------------------------------------------------------------------
void CAddrDlg::SetupHisMenu(CMenu *pMenu)
{
	int i;
	CString string;

	ASSERT(pMenu);

	// メニュー文字列
	for (i=0; i<(int)m_Num; i++) {
		string.Format("$%06X", m_Data[i]);
		pMenu->ModifyMenu(IDM_HISTORY_0 + i, MF_BYCOMMAND | MF_STRING,
							IDM_HISTORY_0 + i, (LPCTSTR)string);
	}

	// メニュー削除
	for (i=m_Num; i<10; i++) {
		pMenu->DeleteMenu(IDM_HISTORY_0 + i, MF_BYCOMMAND);
	}
}

//---------------------------------------------------------------------------
//
//	メニュー結果取得
//
//---------------------------------------------------------------------------
DWORD CAddrDlg::GetAddr(UINT nID)
{
	DWORD dwAddr;
    int i;

	ASSERT((nID >= IDM_HISTORY_0) && (nID <= IDM_HISTORY_9));

	// アドレス取得
	nID -= IDM_HISTORY_0;
	ASSERT(nID < 10);
	dwAddr = m_Data[nID];

	// 場所入れ替え
	for (i=(int)(nID - 1); i>=0; i--) {
		m_Data[i + 1] = m_Data[i];
	}
	m_Data[0] = dwAddr;

	return dwAddr;
}

//---------------------------------------------------------------------------
//
//	ヒストリ個数ポインタ取得
//
//---------------------------------------------------------------------------
UINT* CAddrDlg::GetNumPtr()
{
	return &m_Num;
}

//---------------------------------------------------------------------------
//
//	ヒストリデータポインタ取得
//
//---------------------------------------------------------------------------
DWORD* CAddrDlg::GetDataPtr()
{
	return m_Data;
}

//---------------------------------------------------------------------------
//
//	ヒストリ個数
//
//---------------------------------------------------------------------------
UINT CAddrDlg::m_Num = 0;

//---------------------------------------------------------------------------
//
//	ヒストリデータ
//
//---------------------------------------------------------------------------
DWORD CAddrDlg::m_Data[10] = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

//===========================================================================
//
//	レジスタ入力ダイアログ
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	コンストラクタ
//
//---------------------------------------------------------------------------
CRegDlg::CRegDlg(CWnd *pParent) : CHistoryDlg(IDD_REGDLG, pParent)
{
	// 英語環境への対応
	if (!::IsJapanese()) {
		m_lpszTemplateName = MAKEINTRESOURCE(IDD_US_REGDLG);
		m_nIDHelp = IDD_US_REGDLG;
	}

	m_nIndex = 0;
	m_nBit = 32;
}

//---------------------------------------------------------------------------
//
//	ダイアログ初期化
//
//---------------------------------------------------------------------------
BOOL CRegDlg::OnInitDialog()
{
	CWnd *pWnd;
	CString strRegister;
	CPU *pCPU;
	CPU::cpu_t reg;

	ASSERT(this);
	ASSERT(m_nIndex < 20);

	// CPUレジスタ取得
	::LockVM();
	pCPU = (CPU*)::GetVM()->SearchDevice(MAKEID('C', 'P', 'U', ' '));
	ASSERT(pCPU);
	pCPU->GetCPU(&reg);
	::UnlockVM();

	// 文字列作成
	if (m_nIndex <= 7) {
		strRegister.Format("D%d", m_nIndex);
		m_dwValue = reg.dreg[m_nIndex];
	}
	if ((m_nIndex >= 8) && (m_nIndex <= 15)) {
		strRegister.Format("A%d", m_nIndex & 7);
		m_dwValue = reg.areg[m_nIndex & 7];
	}
	switch (m_nIndex) {
		// USP
		case 16:
			strRegister = "USP";
			if (reg.sr & 0x2000) {
				m_dwValue = reg.sp;
			}
			else {
				m_dwValue = reg.areg[7];
			}
			break;
		// SSP
		case 17:
			strRegister = "SSP";
			if (reg.sr & 0x2000) {
				m_dwValue = reg.areg[7];
			}
			else {
				m_dwValue = reg.sp;
			}
			break;
		// PC
		case 18:
			strRegister = "PC";
			m_dwValue = reg.pc;
			break;
		// SR
		case 19:
			strRegister = "SR";
			m_dwValue = reg.sr;
			break;
	}

	// 基本クラスをここで呼ぶ
	if (!CHistoryDlg::OnInitDialog()) {
		return FALSE;
	}

	// 値を設定
	pWnd = GetDlgItem(IDC_ADDR_ADDRL);
	ASSERT(pWnd);
	pWnd->SetWindowText(strRegister);

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	OK
//
//---------------------------------------------------------------------------
void CRegDlg::OnOK()
{
	CComboBox *pComboBox;
	CString string;
	DWORD dwValue;
	CPU *pCPU;
	CPU::cpu_t reg;

	ASSERT(this);

	// Valueを取得
	pComboBox = (CComboBox*)GetDlgItem(IDC_ADDR_ADDRE);
	ASSERT(pComboBox);
	pComboBox->GetWindowText(string);
	dwValue = ::strtoul((LPCTSTR)string, NULL, 16);

	// VMロック、レジスタ取得
	::LockVM();
	pCPU = (CPU*)::GetVM()->SearchDevice(MAKEID('C', 'P', 'U', ' '));
	ASSERT(pCPU);
	pCPU->GetCPU(&reg);

	// インデックス別
	if (m_nIndex <= 7) {
		reg.dreg[m_nIndex] = dwValue;
	}
	if ((m_nIndex >= 8) && (m_nIndex <= 15)) {
		reg.areg[m_nIndex & 7] = dwValue;
	}
	switch (m_nIndex) {
		// USP
		case 16:
			if (reg.sr & 0x2000) {
				reg.sp = dwValue;
			}
			else {
				reg.areg[7] = dwValue;
			}
			break;
		// SSP
		case 17:
			if (reg.sr & 0x2000) {
				reg.areg[7] = dwValue;
			}
			else {
				reg.sp = dwValue;
			}
			break;
		// PC
		case 18:
			reg.pc = dwValue & 0xfffffe;
			break;
		// SR
		case 19:
			reg.sr = (WORD)dwValue;
			break;
	}

	// レジスタ設定、VMアンロック
	pCPU->SetCPU(&reg);
	::UnlockVM();

	// 基本クラス
	CHistoryDlg::OnOK();
}

//---------------------------------------------------------------------------
//
//	ヒストリ個数ポインタ取得
//
//---------------------------------------------------------------------------
UINT* CRegDlg::GetNumPtr()
{
	return &m_Num;
}

//---------------------------------------------------------------------------
//
//	ヒストリデータポインタ取得
//
//---------------------------------------------------------------------------
DWORD* CRegDlg::GetDataPtr()
{
	return m_Data;
}

//---------------------------------------------------------------------------
//
//	ヒストリ個数
//
//---------------------------------------------------------------------------
UINT CRegDlg::m_Num = 0;

//---------------------------------------------------------------------------
//
//	ヒストリデータ
//
//---------------------------------------------------------------------------
DWORD CRegDlg::m_Data[10] = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

//===========================================================================
//
//	データ入力ダイアログ
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	コンストラクタ
//
//---------------------------------------------------------------------------
CDataDlg::CDataDlg(CWnd *pParent) : CHistoryDlg(IDD_DATADLG, pParent)
{
	// 英語環境への対応
	if (!::IsJapanese()) {
		m_lpszTemplateName = MAKEINTRESOURCE(IDD_US_DATADLG);
		m_nIDHelp = IDD_US_DATADLG;
	}

	// 一応、初期化
	m_dwAddr = 0;
	m_nSize = 0;
}

//---------------------------------------------------------------------------
//
//	ダイアログ初期化
//
//---------------------------------------------------------------------------
BOOL CDataDlg::OnInitDialog()
{
	CWnd *pWnd;
	CString string;

	ASSERT(this);
	ASSERT(m_dwAddr < 0x1000000);

	// アドレスとビット数決定
	switch (m_nSize) {
		// Byte
		case 0:
			string.Format("$%06X (B)", m_dwAddr);
			m_nBit = 8;
			break;
		// Word
		case 1:
			string.Format("$%06X (W)", m_dwAddr);
			m_nBit = 16;
			break;
		// Long
		case 2:
			string.Format("$%06X (L)", m_dwAddr);
			m_nBit = 32;
			break;
		// その他
		default:
			ASSERT(FALSE);
			break;
	}

	// 基本クラス
	if (!CHistoryDlg::OnInitDialog()) {
		return FALSE;
	}

	// 初期化確立の後で設定
	pWnd = GetDlgItem(IDC_ADDR_ADDRL);
	ASSERT(pWnd);
	pWnd->SetWindowText(string);

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	ヒストリ個数ポインタ取得
//
//---------------------------------------------------------------------------
UINT* CDataDlg::GetNumPtr()
{
	return &m_Num;
}

//---------------------------------------------------------------------------
//
//	ヒストリデータポインタ取得
//
//---------------------------------------------------------------------------
DWORD* CDataDlg::GetDataPtr()
{
	return m_Data;
}

//---------------------------------------------------------------------------
//
//	ヒストリ個数
//
//---------------------------------------------------------------------------
UINT CDataDlg::m_Num = 0;

//---------------------------------------------------------------------------
//
//	ヒストリデータ
//
//---------------------------------------------------------------------------
DWORD CDataDlg::m_Data[10] = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

//===========================================================================
//
//	CPUレジスタウィンドウ
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	コンストラクタ
//
//---------------------------------------------------------------------------
CCPURegWnd::CCPURegWnd()
{
	// ウィンドウパラメータ定義
	m_dwID = MAKEID('M', 'P', 'U', 'R');
	::GetMsg(IDS_SWND_CPUREG, m_strCaption);
	m_nWidth = 27;
	m_nHeight = 10;

	// CPU取得
	m_pCPU = (CPU*)::GetVM()->SearchDevice(MAKEID('C', 'P', 'U', ' '));
	ASSERT(m_pCPU);
}

//---------------------------------------------------------------------------
//
//	メッセージ マップ
//
//---------------------------------------------------------------------------
BEGIN_MESSAGE_MAP(CCPURegWnd, CSubTextWnd)
	ON_WM_LBUTTONDBLCLK()
	ON_WM_CONTEXTMENU()
	ON_COMMAND_RANGE(IDM_REG_D0, IDM_REG_SR, OnReg)
END_MESSAGE_MAP()

//---------------------------------------------------------------------------
//
//	セットアップ
//
//---------------------------------------------------------------------------
void FASTCALL CCPURegWnd::Setup()
{
	CPU::cpu_t buf;
	int i;
	CString string;

	ASSERT(this);

	// クリア
	Clear();

	// CPUレジスタ取得
	m_pCPU->GetCPU(&buf);

	// セット(D, A)
	for (i=0; i<8; i++) {
		string.Format("D%1d  %08X", i, buf.dreg[i]);
		SetString(0, i, string);

		string.Format("A%1d  %08X", i, buf.areg[i]);
		SetString(15, i, string);
	}

	// セット(スタック)
	if (buf.sr & 0x2000) {
		string.Format("USP %08X", buf.sp);
		SetString(0, 8, string);
		string.Format("SSP %08X", buf.areg[7]);
		SetString(15, 8, string);
	}
	else {
		string.Format("USP %08X", buf.areg[7]);
		SetString(0, 8, string);
		string.Format("SSP %08X", buf.sp);
		SetString(15, 8, string);
	}

	// セット(その他)
	string.Format("PC    %06X", buf.pc);
	SetString(0, 9, string);
	string.Format("SR      %04X", buf.sr);
	SetString(15, 9, string);
}

//---------------------------------------------------------------------------
//
//	左ボタンダブルクリック
//
//---------------------------------------------------------------------------
void CCPURegWnd::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	int x;
	int y;
	int index;
	CRegDlg dlg(this);

	// x,y算出
	x = point.x / m_tmWidth;
	y = point.y / m_tmHeight;

	// インデックスを出す
	if (y < 8) {
		if (x < 15) {
			// D0-D7
			index = y;
		}
		else {
			// A0-A7
			index = y + 8;
		}
	}
	else {
		index = y - 8;
		index <<= 1;
		if (x >= 15) {
			index++;
		}
		index += 16;
	}

	// ダイアログ実行
	dlg.m_nIndex = index;
	dlg.DoModal();
}

//---------------------------------------------------------------------------
//
//	コンテキストメニュー
//
//---------------------------------------------------------------------------
void CCPURegWnd::OnContextMenu(CWnd *pWnd, CPoint point)
{
	CRect rect;
	CMenu menu;
	CMenu *pMenu;

	// クライアント領域内で押されたか判定する
	GetClientRect(&rect);
	ClientToScreen(&rect);
	if (!rect.PtInRect(point)) {
		CSubTextWnd::OnContextMenu(pWnd, point);
		return;
	}

	// メニューロード
	menu.LoadMenu(IDR_REGMENU);
	pMenu = menu.GetSubMenu(0);

	// メニューセットアップ
	SetupRegMenu(pMenu, m_pCPU, TRUE);

	// メニュー実行
	pMenu->TrackPopupMenu(TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_RIGHTBUTTON,
							point.x, point.y, this);
}

//---------------------------------------------------------------------------
//
//	メニューセットアップ
//
//---------------------------------------------------------------------------
void CCPURegWnd::SetupRegMenu(CMenu *pMenu, CPU *pCPU, BOOL bSR)
{
	int i;
	CString string;
	CPU::cpu_t reg;

	ASSERT(pMenu);
	ASSERT(pCPU);

	// CPUレジスタ取得
	::LockVM();
	pCPU->GetCPU(&reg);
	::UnlockVM();

	// セットアップ(D)
	for (i=0; i<8; i++) {
		string.Format("D%1d ($%08X)", i, reg.dreg[i]);
		pMenu->ModifyMenu(IDM_REG_D0 + i, MF_BYCOMMAND | MF_STRING,
							IDM_REG_D0 + i, (LPCTSTR)string);
	}
	// セットアップ(A)
	for (i=0; i<8; i++) {
		string.Format("A%1d ($%08X)", i, reg.areg[i]);
		pMenu->ModifyMenu(IDM_REG_A0 + i, MF_BYCOMMAND | MF_STRING,
							IDM_REG_A0 + i, (LPCTSTR)string);
	}
	// セットアップ(USP,SSP)
	if (reg.sr & 0x2000) {
		string.Format("USP ($%08X)", reg.sp);
		pMenu->ModifyMenu(IDM_REG_USP, MF_BYCOMMAND | MF_STRING, IDM_REG_USP, (LPCTSTR)string);
		string.Format("SSP ($%08X)", reg.areg[7]);
		pMenu->ModifyMenu(IDM_REG_SSP, MF_BYCOMMAND | MF_STRING, IDM_REG_SSP, (LPCTSTR)string);
	}
	else {
		string.Format("USP ($%08X)", reg.areg[7]);
		pMenu->ModifyMenu(IDM_REG_USP, MF_BYCOMMAND | MF_STRING, IDM_REG_USP, (LPCTSTR)string);
		string.Format("SSP ($%08X)", reg.sp);
		pMenu->ModifyMenu(IDM_REG_SSP, MF_BYCOMMAND | MF_STRING, IDM_REG_SSP, (LPCTSTR)string);
	}

	// セットアップ(PC)
	string.Format("PC ($%06X)", reg.pc);
	pMenu->ModifyMenu(IDM_REG_PC, MF_BYCOMMAND | MF_STRING, IDM_REG_PC, (LPCTSTR)string);

	// セットアップ(SR)
	if (bSR) {
		string.Format("SR ($%04X)", reg.sr);
		pMenu->ModifyMenu(IDM_REG_SR, MF_BYCOMMAND | MF_STRING, IDM_REG_SR, (LPCTSTR)string);
	}
}

//---------------------------------------------------------------------------
//
//	レジスタ値取得
//
//---------------------------------------------------------------------------
DWORD CCPURegWnd::GetRegValue(CPU *pCPU, UINT uID)
{
	CPU::cpu_t reg;

	ASSERT(pCPU);
	ASSERT((uID >= IDM_REG_D0) && (uID <= IDM_REG_SR));

	// CPUレジスタ取得
	::LockVM();
	pCPU->GetCPU(&reg);
	::UnlockVM();

	// D0～D7
	if (uID <= IDM_REG_D7) {
		return reg.dreg[uID - IDM_REG_D0];
	}

	// A0-A7
	if (uID <= IDM_REG_A7) {
		return reg.areg[uID - IDM_REG_A0];
	}

	// USP
	if (uID == IDM_REG_USP) {
		if (reg.sr & 0x2000) {
			return reg.sp;
		}
		else {
			return reg.areg[7];
		}
	}

	// SSP
	if (uID == IDM_REG_SSP) {
		if (reg.sr & 0x2000) {
			return reg.areg[7];
		}
		else {
			return reg.sp;
		}
	}

	// PC
	if (uID == IDM_REG_PC) {
		return reg.pc;
	}

	// SR
	ASSERT(uID == IDM_REG_SR);
	return reg.sr;
}

//---------------------------------------------------------------------------
//
//	レジスタコマンド
//
//---------------------------------------------------------------------------
void CCPURegWnd::OnReg(UINT nID)
{
	CRegDlg dlg(this);

	ASSERT((nID >= IDM_REG_D0) && (nID <= IDM_REG_SR));

	// 換算して
	nID -= IDM_REG_D0;

	// ダイアログにまかせる
	dlg.m_nIndex = nID;
	dlg.DoModal();
}

//===========================================================================
//
//	割り込みウィンドウ
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	コンストラクタ
//
//---------------------------------------------------------------------------
CIntWnd::CIntWnd()
{
	// ウィンドウパラメータ定義
	m_dwID = MAKEID('I', 'N', 'T', ' ');
	::GetMsg(IDS_SWND_INT, m_strCaption);
	m_nWidth = 47;
	m_nHeight = 9;

	// CPU取得
	m_pCPU = (CPU*)::GetVM()->SearchDevice(MAKEID('C', 'P', 'U', ' '));
	ASSERT(m_pCPU);
}

//---------------------------------------------------------------------------
//
//	メッセージ マップ
//
//---------------------------------------------------------------------------
BEGIN_MESSAGE_MAP(CIntWnd, CSubTextWnd)
	ON_WM_LBUTTONDBLCLK()
END_MESSAGE_MAP()

//---------------------------------------------------------------------------
//
//	セットアップ
//
//---------------------------------------------------------------------------
void FASTCALL CIntWnd::Setup()
{
	static const char *desc_table[] = {
		"INTSW",
		"MFP",
		"SCC",
		"(Ext.)",
		"DMAC",
		"(Ext.)",
		"IOSC"
	};
	int y;
	int i;
	int level;
	CString string;
	CPU::cpu_t cpu;

	// CPUデータ取得
	m_pCPU->GetCPU(&cpu);
	level = (cpu.sr >> 8);
	level &= 0x07;

	// クリア
	Clear();
	y = 0;

	// ガイド
	SetString(0, y, "(High)  Device  Mask  Vector     Req        Ack");
	y++;

	// 7レベル処理
	for (i=7; i>=1; i--) {
		// 割り込み名称セット
		string.Format("Level%1d  ", i);
		string += desc_table[7 - i];
		SetString(0, y, string);

		// マスク
		if (i < 7) {
			if (i <= level) {
				SetString(16, y, "Mask");
			}
		}

		// リクエスト中か
		if (cpu.intr[0] & 0x80) {
			// リクエストあり。ベクタ表示
			string.Format("$%02X", cpu.intr[i]);
			SetString(22, y, string);
		}

		// リクエストカウンタ
		string.Format("%10d", cpu.intreq[i]);
		SetString(26, y, string);
		
		// 応答カウンタ
		string.Format("%10d", cpu.intack[i]);
		SetString(37, y, string);

		// 次へ
		y++;
		cpu.intr[0] <<= 1;
	}

	// ガイド
	SetString(0, y, "(Low)");
}

//---------------------------------------------------------------------------
//
//	左ボタンダブルクリック
//
//---------------------------------------------------------------------------
void CIntWnd::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	int y;
	int level;
	CPU::cpu_t cpu;

	// y算出
	y = point.y / m_tmHeight;

	// y=1,2,3,4,5,6,7がそれぞれint7,6,5,4,3,2,1に対応
	level = 8 - y;
	if ((level < 1) || (level > 7)) {
		return;
	}

	// ロック、データ取得
	::LockVM();
	m_pCPU->GetCPU(&cpu);

	// クリア
	ASSERT((level >= 1) && (level <= 7));
	cpu.intreq[level] = 0;
	cpu.intack[level] = 0;

	// データ設定、アンロック
	m_pCPU->SetCPU(&cpu);
	::UnlockVM();
}

//===========================================================================
//
//	逆アセンブルウィンドウ
//
//===========================================================================

#if defined(__cplusplus)
extern "C" {
#endif
//---------------------------------------------------------------------------
//
//	cpudebug.c 文字列出力
//
//---------------------------------------------------------------------------
void disasm_put(const char *s)
{
	strcpy(debugbuf, s);
}

//---------------------------------------------------------------------------
//
//	メモリデバイス
//
//---------------------------------------------------------------------------
static Memory* cpudebug_memory;

//---------------------------------------------------------------------------
//
//	cpudebug.c ワード読み出し
//
//---------------------------------------------------------------------------
WORD cpudebug_fetch(DWORD addr)
{
	WORD w;

	ASSERT(cpudebug_memory);

	addr &= 0xfffffe;
	w = cpudebug_memory->ReadOnly(addr);
	w <<= 8;
	w |= cpudebug_memory->ReadOnly(addr + 1);

	return w;
}
#if defined(__cplusplus)
};
#endif

//---------------------------------------------------------------------------
//
//	コンストラクタ
//
//---------------------------------------------------------------------------
CDisasmWnd::CDisasmWnd(int index)
{
	// 逆アセンブルウィンドウは8種類まで
	ASSERT((index >= 0) && (index <= 0x07));

	// ウィンドウパラメータ定義
	m_dwID = MAKEID('D', 'I', 'S', (index + 'A'));
	::GetMsg(IDS_SWND_DISASM, m_strCaption);
	m_nWidth = 70;
	m_nHeight = 16;

	// ウィンドウパラメータ定義(スクロール)
	m_ScrlWidth = 70;
	m_ScrlHeight = 0x8000;

	// 最初のウィンドウはPC同期あり、それ以外は無し
	if (index == 0) {
		m_bSync = TRUE;
	}
	else {
		m_bSync = FALSE;
	}

	// その他
	m_pAddrBuf = NULL;
	m_Caption = m_strCaption;
	m_CaptionSet = "";

	// デバイスを取得
	m_pCPU = (CPU*)::GetVM()->SearchDevice(MAKEID('C', 'P', 'U', ' '));
	ASSERT(m_pCPU);
	cpudebug_memory = (Memory*)::GetVM()->SearchDevice(MAKEID('M', 'E', 'M', ' '));
	ASSERT(cpudebug_memory);
	m_pScheduler = (Scheduler*)::GetVM()->SearchDevice(MAKEID('S', 'C', 'H', 'E'));
	ASSERT(m_pScheduler);
	m_pMFP = (MFP*)::GetVM()->SearchDevice(MAKEID('M', 'F', 'P', ' '));
	ASSERT(m_pMFP);
	m_pMemory = (Memory*)::GetVM()->SearchDevice(MAKEID('M', 'E', 'M', ' '));
	ASSERT(m_pMemory);
	m_pSCC = (SCC*)::GetVM()->SearchDevice(MAKEID('S', 'C', 'C', ' '));
	ASSERT(m_pSCC);
	m_pDMAC = (DMAC*)::GetVM()->SearchDevice(MAKEID('D', 'M', 'A', 'C'));
	ASSERT(m_pDMAC);
	m_pIOSC = (IOSC*)::GetVM()->SearchDevice(MAKEID('I', 'O', 'S', 'C'));
	ASSERT(m_pIOSC);

	// アドレスをPCに初期化
	m_dwSetAddr = m_pCPU->GetPC();
	m_dwAddr = m_dwSetAddr;
	m_dwAddr = m_dwAddr & 0xff0000;
	m_dwPC = 0xffffffff;

	// 逆アセンブルバッファ接続
	::cpudebug_put = ::disasm_put;
}

//---------------------------------------------------------------------------
//
//	メッセージ マップ
//
//---------------------------------------------------------------------------
BEGIN_MESSAGE_MAP(CDisasmWnd, CSubTextScrlWnd)
	ON_WM_CREATE()
	ON_WM_DESTROY()
	ON_WM_SIZE()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_VSCROLL()
	ON_WM_CONTEXTMENU()
	ON_COMMAND(IDM_DIS_NEWWIN, OnNewWin)
	ON_COMMAND(IDM_DIS_PC, OnPC)
	ON_COMMAND(IDM_DIS_SYNC, OnSync)
	ON_COMMAND(IDM_DIS_ADDR, OnAddr)
	ON_COMMAND_RANGE(IDM_REG_D0, IDM_REG_PC, OnReg)
	ON_COMMAND_RANGE(IDM_STACK_0, IDM_STACK_F, OnStack)
	ON_COMMAND_RANGE(IDM_DIS_BREAKP0, IDM_DIS_BREAKP7, OnBreak)
	ON_COMMAND_RANGE(IDM_HISTORY_0, IDM_HISTORY_9, OnHistory)
	ON_COMMAND_RANGE(IDM_DIS_RESET, IDM_DIS_FLINE, OnCPUExcept)
	ON_COMMAND_RANGE(IDM_DIS_TRAP0, IDM_DIS_TRAPF, OnTrap)
	ON_COMMAND_RANGE(IDM_DIS_MFP0, IDM_DIS_MFPF, OnMFP)
	ON_COMMAND_RANGE(IDM_DIS_SCC0, IDM_DIS_SCC7, OnSCC)
	ON_COMMAND_RANGE(IDM_DIS_DMAC0, IDM_DIS_DMAC7, OnDMAC)
	ON_COMMAND_RANGE(IDM_DIS_IOSC0, IDM_DIS_IOSC3, OnIOSC)
END_MESSAGE_MAP()

//---------------------------------------------------------------------------
//
//	ウィンドウ作成
//
//---------------------------------------------------------------------------
int CDisasmWnd::OnCreate(LPCREATESTRUCT lpcs)
{
	int i;

	// アドレスバッファを先に確保
	m_pAddrBuf = new DWORD[ m_nHeight ];
	for (i=0; i<m_nHeight; i++) {
		m_pAddrBuf[i] = 0xffffffff;
	}

	// 基本クラス
	if (CSubTextScrlWnd::OnCreate(lpcs) != 0) {
		return -1;
	}

	// アドレス初期化
	SetAddr(m_dwSetAddr);

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	ウィンドウ削除
//
//---------------------------------------------------------------------------
void CDisasmWnd::OnDestroy()
{
	m_bEnable = FALSE;

	// アドレスバッファ解放
	if (m_pAddrBuf) {
		delete[] m_pAddrBuf;
		m_pAddrBuf = NULL;
	}

	// 基本クラスへ
	CSubTextScrlWnd::OnDestroy();
}

//---------------------------------------------------------------------------
//
//	サイズ変更
//
//---------------------------------------------------------------------------
void CDisasmWnd::OnSize(UINT nType, int cx, int cy)
{
	CRect rect;
	int i;

	ASSERT(this);
	ASSERT(cx >= 0);
	ASSERT(cy >= 0);

	// アドレスバッファがあれば一度解放
	::LockVM();
	if (m_pAddrBuf) {
		delete[] m_pAddrBuf;
		m_pAddrBuf = NULL;
	}
	::UnlockVM();

	// 基本クラス(この中で、CDisasmWnd::OnSizeが再度呼ばれる場合あり)
	CSubTextScrlWnd::OnSize(nType, cx, cy);

	// アドレスバッファを再度確保。解放チェックも行う
	::LockVM();
	if (m_pAddrBuf) {
		delete[] m_pAddrBuf;
		m_pAddrBuf = NULL;
	}
	m_pAddrBuf = new DWORD[ m_nHeight ];
	for (i=0; i<m_nHeight; i++) {
		m_pAddrBuf[i] = 0xffffffff;
	}
	::UnlockVM();

	// 再アドレスセット
	SetAddr(m_dwSetAddr);
}

//---------------------------------------------------------------------------
//
//	左クリック
//
//---------------------------------------------------------------------------
void CDisasmWnd::OnLButtonDown(UINT nFlags, CPoint point)
{
	OnLButtonDblClk(nFlags, point);
}

//---------------------------------------------------------------------------
//
//	左ボタンダブルクリック
//
//---------------------------------------------------------------------------
void CDisasmWnd::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	int y;
	DWORD dwAddr;

	// アドレスバッファが無ければ終了
	if (!m_pAddrBuf) {
		return;
	}

	// アドレス取得、チェック
	y = point.y / m_tmHeight;
	dwAddr = m_pAddrBuf[y];
	if (dwAddr >= 0x01000000) {
		return;
	}

	// VMロック
	::LockVM();

	// アドレスがあれば削除、なければ設定
	if (m_pScheduler->IsBreak(dwAddr) >= 0) {
		m_pScheduler->DelBreak(dwAddr);
	}
	else {
		m_pScheduler->SetBreak(dwAddr);
	}

	// VMアンロック
	::UnlockVM();
}

//---------------------------------------------------------------------------
//
//	スクロール(垂直)
//
//---------------------------------------------------------------------------
void CDisasmWnd::OnVScroll(UINT nSBCode, UINT nPos, CScrollBar *pBar)
{
	SCROLLINFO si;
	DWORD dwDiff;
	DWORD dwAddr;
	int i;

	// スクロール情報を取得
	memset(&si, 0, sizeof(si));
	si.cbSize = sizeof(si);
	GetScrollInfo(SB_VERT, &si, SIF_ALL);

	// スクロールバーコード別
	switch (nSBCode) {
		// 上へ
		case SB_TOP:
			m_ScrlY = si.nMin;
			break;

		// 下へ
		case SB_BOTTOM:
			m_ScrlY = si.nMax;
			break;

		// 1ライン上へ
		case SB_LINEUP:
			// アドレスバッファをチェック
			if (m_pAddrBuf) {
				// 一つ前のアドレスを取得し、差を計算
				dwDiff = GetPrevAddr(m_pAddrBuf[0]);
				dwDiff = m_pAddrBuf[0] - dwDiff;

				// 差あれば減少、差がなければ-1
				if (dwDiff > 0) {
					dwDiff >>= 1;
					m_ScrlY -= dwDiff;
				}
				else {
					m_ScrlY--;
				}
			}
			break;

		// 1ライン下へ
		case SB_LINEDOWN:
			// アドレスバッファを見て、1命令分進める
			if (m_nHeight >= 2) {
				if (m_pAddrBuf) {
					dwDiff = m_pAddrBuf[1] - m_pAddrBuf[0];
					dwDiff >>= 1;
					m_ScrlY += dwDiff;
				}
			}
			break;

		// 1ページ上へ
		case SB_PAGEUP:
			// アドレスバッファをチェック
			if (m_pAddrBuf) {
				dwAddr = m_pAddrBuf[0];
				for (i=0; i<m_nHeight-1; i++) {
					// 一つ前のアドレスを取得し、差を計算
					dwDiff = GetPrevAddr(dwAddr);
					dwDiff = dwAddr - dwDiff;
					dwAddr -= dwDiff;

					// 差あれば減少、差がなければ-1
					if (dwDiff > 0) {
						dwDiff >>= 1;
						m_ScrlY -= dwDiff;
					}
					else {
						m_ScrlY--;
					}

					// オーバーチェック
					if (m_ScrlY < 0) {
						m_ScrlY = 0;
					}
				}

				// 全く戻れなかった場合を考慮
				if (dwAddr == m_pAddrBuf[0]) {
					m_ScrlY -= si.nPage;
				}
			}
			break;

		// 1ページ下へ
		case SB_PAGEDOWN:
			// アドレスバッファを見て、m_nHeight命令分進める
			if (m_pAddrBuf) {
				dwDiff = m_pAddrBuf[m_nHeight - 1] - m_pAddrBuf[0];
				dwDiff >>= 1;
				m_ScrlY += dwDiff;
			}
			break;

		// サム移動
		case SB_THUMBPOSITION:
			m_ScrlY = nPos;
			break;
		case SB_THUMBTRACK:
			m_ScrlY = nPos;
			break;
	}

	// オーバーチェック
	if (m_ScrlY < 0) {
		m_ScrlY = 0;
	}
	if (m_ScrlY > si.nMax) {
		m_ScrlY = si.nMax;
	}

	// セット
	SetupScrlV();
}

//---------------------------------------------------------------------------
//
//	コンテキストメニュー
//
//---------------------------------------------------------------------------
void CDisasmWnd::OnContextMenu(CWnd *pWnd, CPoint point)
{
	CRect rect;
	CMenu menu;
	CMenu *pMenu;

	// クライアント領域内で押されたか判定する
	GetClientRect(&rect);
	ClientToScreen(&rect);
	if (!rect.PtInRect(point)) {
		CSubTextScrlWnd::OnContextMenu(pWnd, point);
		return;
	}

	// メニューロード
	if (::IsJapanese()) {
		menu.LoadMenu(IDR_DISMENU);
	}
	else {
		menu.LoadMenu(IDR_US_DISMENU);
	}
	pMenu = menu.GetSubMenu(0);

	// セットアップ
	SetupContext(pMenu);

	// メニュー実行
	pMenu->TrackPopupMenu(TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_RIGHTBUTTON,
							point.x, point.y, this);
}

//---------------------------------------------------------------------------
//
//	コンテキストメニュー セットアップ
//
//---------------------------------------------------------------------------
void FASTCALL CDisasmWnd::SetupContext(CMenu *pMenu)
{
	CMenu *pSubMenu;
	int i;

	ASSERT(pMenu);

	// PCに同期
	if (m_bSync) {
		pMenu->CheckMenuItem(IDM_DIS_SYNC, MF_BYCOMMAND | MF_CHECKED);
	}

	// 新しいウィンドウ
	if (!m_pDrawView->IsNewWindow(TRUE)) {
		pMenu->EnableMenuItem(IDM_DIS_NEWWIN, MF_BYCOMMAND | MF_GRAYED);
	}

	// MPUレジスタ・スタック・アドレスヒストリ
	CCPURegWnd::SetupRegMenu(pMenu, m_pCPU, FALSE);
	CMemoryWnd::SetupStackMenu(pMenu, m_pMemory, m_pCPU);
	CAddrDlg::SetupHisMenu(pMenu);

	// ブレークポイント
	SetupBreakMenu(pMenu, m_pScheduler);

	// VMをロック
	::LockVM();

	// 割り込みベクタ準備
	pSubMenu = pMenu->GetSubMenu(9);

	// MPU標準
	SetupVector(pSubMenu, 0, 1, 11);

	// trap #x
	SetupVector(pSubMenu, 12, 0x20, 16);

	// MFP
	SetupVector(pSubMenu, 29, (m_pMFP->GetVR() & 0xf0), 16);

	// SCC
	for (i=0; i<8; i++) {
		SetupVector(pSubMenu, 46 + i, m_pSCC->GetVector(i), 1);
	}

	// DMAC
	for (i=0; i<8; i++) {
		SetupVector(pSubMenu, 55 + i, m_pDMAC->GetVector(i), 1);
	}

	// IOSC
	SetupVector(pSubMenu, 64, m_pIOSC->GetVector(), 4);

	// VMをアンロック
	::UnlockVM();
}

//---------------------------------------------------------------------------
//
//	割り込みベクタセットアップ
//
//---------------------------------------------------------------------------
void FASTCALL CDisasmWnd::SetupVector(CMenu *pMenu, UINT index, DWORD vector, int num)
{
	int i;
	DWORD handler;

	ASSERT(pMenu);
	ASSERT(num > 0);

	// 割り込みベクタアドレス初期化
	vector <<= 2;

	// ループ
	for (i=0; i<num; i++) {
		// 割り込みハンドラアドレス取得
		handler = (DWORD)m_pMemory->ReadOnly(vector + 1);
		handler <<= 8;
		handler |= (DWORD)m_pMemory->ReadOnly(vector + 2);
		handler <<= 8;
		handler |= (DWORD)m_pMemory->ReadOnly(vector + 3);
		vector += 4;

		// アドレスセット
		SetupAddress(pMenu, index, handler);
		index++;
	}
}

//---------------------------------------------------------------------------
//
//	アドレスセットアップ
//
//---------------------------------------------------------------------------
void FASTCALL CDisasmWnd::SetupAddress(CMenu *pMenu, UINT index, DWORD addr)
{
	CString string;
	CString menustr;
	int ext;
	UINT id;

	ASSERT(pMenu);
	ASSERT(addr <= 0xffffff);

	// 現在の文字列を取得
	pMenu->GetMenuString(index, string, MF_BYPOSITION);

	// カッコの先頭を探して、あればそれ以降のみ
	ext = string.Find(" : ");
	if (ext >= 0) {
		menustr = string.Mid(ext + 3);
	}
	else {
		menustr = string;
	}

	// ($)の文字列を作成
	string.Format("$%06X : ", addr);
	string += menustr;

	// 文字列セット
	id = pMenu->GetMenuItemID(index);
	pMenu->ModifyMenu(index, MF_BYPOSITION | MF_STRING, id, string);
}

//---------------------------------------------------------------------------
//
//	新しいウィンドウ
//
//---------------------------------------------------------------------------
void CDisasmWnd::OnNewWin()
{
	CDisasmWnd *pDisasmWnd;
	DWORD dwAddr;

	// 親ウィンドウに対し、新しいウィンドウの作成を要請
	pDisasmWnd = (CDisasmWnd*)m_pDrawView->NewWindow(TRUE);

	// 成功したら、自分と同じアドレスを渡す
	if (pDisasmWnd) {
		dwAddr = m_ScrlY * 2;
		dwAddr += m_dwAddr;
		pDisasmWnd->SetAddr(dwAddr);
	}
}

//---------------------------------------------------------------------------
//
//	PCへ移動
//
//---------------------------------------------------------------------------
void CDisasmWnd::OnPC()
{
	// 現在のPCにアドレスセット(内部でRefreshを行う)
	SetAddr(m_pCPU->GetPC());
}

//---------------------------------------------------------------------------
//
//	PCと同期
//
//---------------------------------------------------------------------------
void CDisasmWnd::OnSync()
{
	m_bSync = (!m_bSync);
}

//---------------------------------------------------------------------------
//
//	アドレス入力
//
//---------------------------------------------------------------------------
void CDisasmWnd::OnAddr()
{
	CAddrDlg dlg(this);

	// ダイアログ実行
	dlg.m_dwValue = m_dwSetAddr;
	if (dlg.DoModal() != IDOK) {
		return;
	}

	// アドレスセット
	SetAddr(dlg.m_dwValue);
}

//---------------------------------------------------------------------------
//
//	MPUレジスタ
//
//---------------------------------------------------------------------------
void CDisasmWnd::OnReg(UINT nID)
{
	DWORD dwAddr;

	ASSERT((nID >= IDM_REG_D0) && (nID <= IDM_REG_PC));

	dwAddr = CCPURegWnd::GetRegValue(m_pCPU, nID);
	SetAddr(dwAddr);
}

//---------------------------------------------------------------------------
//
//	スタック
//
//---------------------------------------------------------------------------
void CDisasmWnd::OnStack(UINT nID)
{
	DWORD dwAddr;

	ASSERT((nID >= IDM_STACK_0) && (nID <= IDM_STACK_F));

	dwAddr = CMemoryWnd::GetStackAddr(nID, m_pMemory, m_pCPU);
	SetAddr(dwAddr);
}

//---------------------------------------------------------------------------
//
//	ブレークポイント
//
//---------------------------------------------------------------------------
void CDisasmWnd::OnBreak(UINT nID)
{
	DWORD dwAddr;

	ASSERT((nID >= IDM_DIS_BREAKP0) && (nID <= IDM_DIS_BREAKP7));

	dwAddr = GetBreak(nID, m_pScheduler);
	SetAddr(dwAddr);
}

//---------------------------------------------------------------------------
//
//	アドレスヒストリ
//
//---------------------------------------------------------------------------
void CDisasmWnd::OnHistory(UINT nID)
{
	DWORD dwAddr;

	ASSERT((nID >= IDM_HISTORY_0) && (nID <= IDM_HISTORY_9));

	dwAddr = CAddrDlg::GetAddr(nID);
	SetAddr(dwAddr);
}

//---------------------------------------------------------------------------
//
//	CPU例外ベクタ
//
//---------------------------------------------------------------------------
void CDisasmWnd::OnCPUExcept(UINT nID)
{
	nID -= IDM_DIS_RESET;

	OnVector(nID + 1);
}

//---------------------------------------------------------------------------
//
//	trapベクタ
//
//---------------------------------------------------------------------------
void CDisasmWnd::OnTrap(UINT nID)
{
	nID -= IDM_DIS_TRAP0;

	OnVector(nID + 0x20);
}

//---------------------------------------------------------------------------
//
//	MFPベクタ
//
//---------------------------------------------------------------------------
void CDisasmWnd::OnMFP(UINT nID)
{
	nID -= IDM_DIS_MFP0;

	OnVector(nID + (m_pMFP->GetVR() & 0xf0));
}

//---------------------------------------------------------------------------
//
//	SCCベクタ
//
//---------------------------------------------------------------------------
void CDisasmWnd::OnSCC(UINT nID)
{
	DWORD vector;

	nID -= IDM_DIS_SCC0;
	ASSERT(nID <= 7);

	// ベクタ番号を得る
	::LockVM();
	vector = m_pSCC->GetVector(nID);
	::UnlockVM();

	OnVector(vector);
}

//---------------------------------------------------------------------------
//
//	DMACベクタ
//
//---------------------------------------------------------------------------
void CDisasmWnd::OnDMAC(UINT nID)
{
	DWORD vector;

	nID -= IDM_DIS_DMAC0;
	ASSERT(nID <= 7);

	// ベクタ番号を得る
	::LockVM();
	vector = m_pDMAC->GetVector(nID);
	::UnlockVM();

	OnVector(vector);
}

//---------------------------------------------------------------------------
//
//	IOSCベクタ
//
//---------------------------------------------------------------------------
void CDisasmWnd::OnIOSC(UINT nID)
{
	DWORD vector;

	nID -= IDM_DIS_IOSC0;
	ASSERT(nID <= 3);

	// ベクタ番号を得る
	::LockVM();
	vector = m_pIOSC->GetVector() + nID;
	::UnlockVM();

	OnVector(vector);
}

//---------------------------------------------------------------------------
//
//	ベクタ指定
//
//---------------------------------------------------------------------------
void FASTCALL CDisasmWnd::OnVector(UINT vector)
{
	DWORD addr;

	// ベクタ読み出し
	::LockVM();
	vector <<= 2;
	addr = (DWORD)m_pMemory->ReadOnly(vector + 1);
	addr <<= 8;
	addr |= (DWORD)m_pMemory->ReadOnly(vector + 2);
	addr <<= 8;
	addr |= (DWORD)m_pMemory->ReadOnly(vector + 3);
	::UnlockVM();

	// アドレス指定
	SetAddr(addr);
}

//---------------------------------------------------------------------------
//
//	アドレス指定
//
//---------------------------------------------------------------------------
void FASTCALL CDisasmWnd::SetAddr(DWORD dwAddr)
{
	int offset;
	CString string;

	::LockVM();

	// アドレス記憶
	dwAddr &= 0xffffff;
	m_dwSetAddr = dwAddr;
	m_dwAddr = dwAddr & 0xff0000;

	// 下位のみ取り出し
	offset = dwAddr & 0x00ffff;
	offset >>= 1;

	// スクロール
	m_ScrlY = offset;
	::UnlockVM();
	SetScrollPos(SB_VERT, offset, TRUE);

	// キャプション文字列を更新
	string.Format(" [%d] ($%06X - $%06X)", (m_dwID & 0xff) - 'A' + 1, m_dwAddr, m_dwAddr + 0xffff);
	string = m_strCaption + string;
	if (m_Caption != string) {
		m_Caption = string;
		SetWindowText(string);
	}
}

//---------------------------------------------------------------------------
//
//	PC指定
//
//---------------------------------------------------------------------------
void FASTCALL CDisasmWnd::SetPC(DWORD pc)
{
	ASSERT(pc <= 0xffffff);

	// 同期フラグが立っていれば、アドレスセット
	if (m_bSync) {
		m_dwPC = pc;
	}
	else {
		m_dwPC = 0xffffffff;
	}
}

//---------------------------------------------------------------------------
//
//	メッセージスレッドからの更新
//
//---------------------------------------------------------------------------
void FASTCALL CDisasmWnd::Update()
{
	// PC指定
	if (m_dwPC < 0x1000000) {
		SetAddr(m_dwPC);
		m_dwPC = 0xffffffff;
	}
}

//---------------------------------------------------------------------------
//
//	セットアップ
//
//---------------------------------------------------------------------------
void FASTCALL CDisasmWnd::Setup()
{
	DWORD dwAddr;
	DWORD dwPC;
	int i;
	int j;
	int k;

	// アドレス指定
	dwAddr = (m_dwAddr & 0xff0000);
	dwAddr |= (DWORD)(m_ScrlY * 2);
	::debugpc = dwAddr;

	// PC取得
	dwPC = m_pCPU->GetPC();

	// ループ
	for (i=0; i<m_nHeight; i++) {
		dwAddr = ::debugpc;

		// アドレス格納
		if (m_pAddrBuf) {
			m_pAddrBuf[i] = dwAddr;
		}

		// アドレスループチェック(FFFFFFFループを考慮)
		if (dwAddr > 0xffffff) {
			// ループした。アドレスはホールド
			ASSERT(i > 0);
			if (m_pAddrBuf) {
				m_pAddrBuf[i] = m_pAddrBuf[i - 1];
			}

			// 消去
			Reverse(FALSE);
			for (j=0; j<m_nWidth; j++) {
				SetChr(j, i, ' ');
			}
			continue;
		}

		// 属性決定
		k = m_pScheduler->IsBreak(dwAddr);
		if (k >= 0) {
			Reverse(TRUE);
		}
		else {
			Reverse(FALSE);
		}
		// 塗りつぶし
		for (j=0; j<m_nWidth; j++) {
			SetChr(j, i, ' ');
		}

		// 逆アセンブル
		::cpudebug_disassemble(1);

		// PCマーク、ブレークマーク
		if (k >= 0) {
			::debugbuf[0] = (char)(k + '1');
		}
		else {
			::debugbuf[0] = ' ';
		}
		if (dwAddr == dwPC) {
			::debugbuf[1] = '>';
		}
		else {
			::debugbuf[1] = ' ';
		}

		// 表示
		if (m_ScrlX < (int)strlen(::debugbuf)) {
			SetString(0, i, &debugbuf[m_ScrlX]);
		}

		// ブレークポイントで無効の場合に対応
		k = m_pScheduler->IsBreak(dwAddr, TRUE);
		if (k >= 0) {
			Reverse(TRUE);
			SetChr(0, i, (char)(k + '1'));
		}
	}
}

//---------------------------------------------------------------------------
//
//	手前のアドレスを取得
//
//---------------------------------------------------------------------------
DWORD FASTCALL CDisasmWnd::GetPrevAddr(DWORD dwAddr)
{
	int i;
	DWORD dwTest;

	ASSERT(dwAddr <= 0xffffff);

	// アドレス初期化
	dwTest = dwAddr;

	for (i=0; i<16; i++) {
		// dwTestを減少、オーバーチェック
		dwTest -= 2;
		if (dwTest >= 0x01000000) {
			return dwAddr;
		}

		// そこから逆アセンブルして、アドレス増加をみる
		::debugpc = dwTest;
		::cpudebug_disassemble(1);

		// 一致していれば"UNRECOG"チェック
		if (::debugpc == dwAddr) {
			if ((::debugbuf[35] == 'U') || (::debugbuf[36] == 'N') || (::debugbuf[37] == 'R')) {
				continue;
			}
			// ok、リターン
			return dwTest;
		}
	}

	// 不一致。dwAddrを返す
	return dwAddr;
}

//---------------------------------------------------------------------------
//
//	ブレークポイントメニュー セットアップ
//
//---------------------------------------------------------------------------
void FASTCALL CDisasmWnd::SetupBreakMenu(CMenu *pMenu, Scheduler *pScheduler)
{
	int num;
	int i;
	Scheduler::breakpoint_t buf;
	CString string;

	ASSERT(pMenu);
	ASSERT(pScheduler);

	// 個数クリア
	num = 0;

	// 設定
	::LockVM();
	for (i=0; i<Scheduler::BreakMax; i++) {
		pScheduler->GetBreak(i, &buf);
		if (buf.use) {
			// 使用中なので、設定
			string.Format("%1d : $%06X", num + 1, buf.addr);
			pMenu->ModifyMenu(IDM_DIS_BREAKP0 + num, MF_BYCOMMAND | MF_STRING,
				IDM_DIS_BREAKP0 + num, (LPCTSTR)string);

			// +1
			num++;
		}
	}
	::UnlockVM();

	// それ以外のとこはクリア
	for (i=num; i<Scheduler::BreakMax; i++) {
		pMenu->DeleteMenu(IDM_DIS_BREAKP0 + i, MF_BYCOMMAND);
	}
}

//---------------------------------------------------------------------------
//
//	ブレークポイントメニュー取得
//
//---------------------------------------------------------------------------
DWORD FASTCALL CDisasmWnd::GetBreak(UINT nID, Scheduler *pScheduler)
{
	int i;
	Scheduler::breakpoint_t buf;

	ASSERT((nID >= IDM_DIS_BREAKP0) && (nID <= IDM_DIS_BREAKP7));
	ASSERT(pScheduler);
	nID -= IDM_DIS_BREAKP0;

	// 検索ループ
	::LockVM();
	for (i=0; i<Scheduler::BreakMax; i++) {
		pScheduler->GetBreak(i, &buf);
		if (buf.use) {
			// 使用中なので、これか？
			if (nID == 0) {
				::UnlockVM();
				return buf.addr;
			}
			nID--;
		}
	}
	::UnlockVM();

	return 0;
}

//===========================================================================
//
//	メモリウィンドウ
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	コンストラクタ
//
//---------------------------------------------------------------------------
CMemoryWnd::CMemoryWnd(int nWnd)
{
	// メモリウィンドウは8種類まで
	ASSERT((nWnd >= 0) && (nWnd <= 7));

	// ウィンドウパラメータ定義
	m_dwID = MAKEID('M', 'E', 'M', (nWnd + 'A'));
	::GetMsg(IDS_SWND_MEMORY, m_strCaption);
	m_nWidth = 73;
	m_nHeight = 16;

	// ウィンドウパラメータ定義(スクロール)
	m_ScrlWidth = 73;
	m_ScrlHeight = 0x8000;

	// CPU取得
	m_pCPU = (CPU*)::GetVM()->SearchDevice(MAKEID('C', 'P', 'U', ' '));
	ASSERT(m_pCPU);

	// メモリ取得
	m_pMemory = (Memory*)::GetVM()->SearchDevice(MAKEID('M', 'E', 'M', ' '));
	ASSERT(m_pMemory);

	// その他
	m_dwAddr = 0;
	m_nUnit = 0;
	m_strCaptionReq.Empty();
	m_strCaptionSet.Empty();
}

//---------------------------------------------------------------------------
//
//	メッセージ マップ
//
//---------------------------------------------------------------------------
BEGIN_MESSAGE_MAP(CMemoryWnd, CSubTextScrlWnd)
	ON_WM_CREATE()
	ON_WM_PAINT()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_CONTEXTMENU()
	ON_COMMAND(IDM_MEMORY_ADDR, OnAddr)
	ON_COMMAND(IDM_MEMORY_NEWWIN, OnNewWin)
	ON_COMMAND_RANGE(IDM_MEMORY_BYTE, IDM_MEMORY_LONG, OnUnit)
	ON_COMMAND_RANGE(IDM_MEMORY_0, IDM_MEMORY_F, OnRange)
	ON_COMMAND_RANGE(IDM_REG_D0, IDM_REG_PC, OnReg)
	ON_COMMAND_RANGE(IDM_AREA_MPU, IDM_AREA_IPLROM, OnArea)
	ON_COMMAND_RANGE(IDM_HISTORY_0, IDM_HISTORY_9, OnHistory)
	ON_COMMAND_RANGE(IDM_STACK_0, IDM_STACK_F, OnStack)
END_MESSAGE_MAP()

//---------------------------------------------------------------------------
//
//	ウィンドウ作成
//
//---------------------------------------------------------------------------
int CMemoryWnd::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	// 基本クラス
	if (CSubTextScrlWnd::OnCreate(lpCreateStruct) != 0) {
		return -1;
	}

	// アドレス初期化
	SetAddr(0);
	return TRUE;
}

//---------------------------------------------------------------------------
//
//	描画
//
//---------------------------------------------------------------------------
void CMemoryWnd::OnPaint()
{
	// 基本クラス
	CSubTextScrlWnd::OnPaint();

	// キャプション設定
	SetWindowText(m_strCaption);
}

//---------------------------------------------------------------------------
//
//	左ボタンダブルクリック
//
//---------------------------------------------------------------------------
void CMemoryWnd::OnLButtonDblClk(UINT /*nFlags*/, CPoint point)
{
	int x;
	int y;
	DWORD dwAddr;
	DWORD dwData;
	CDataDlg dlg(this);

	// x, y算出
	x = point.x / m_tmWidth;
	y = point.y / m_tmHeight;

	// xチェック
	if (x < 8) {
		return;
	}
	x -= 8;

	// yのアドレスを得る
	dwAddr = m_dwAddr | (m_ScrlY << 5);
	dwAddr += (y << 4);
	if ((dwAddr - m_dwAddr) >= 0x100000) {
		return;
	}

	// xからアドレスを得る
	switch (m_nUnit) {
		// Byte
		case 0:
			x /= 3;
			break;
		// Word
		case 1:
			x /= 5;
			x <<= 1;
			break;
		// Long
		case 2:
			x /= 9;
			x <<= 2;
			break;
		// その他
		default:
			ASSERT(FALSE);
			break;
	}
	if (x >= 16) {
		return;
	}
	dwAddr += x;

	// データ読み込み
	::LockVM();
	switch (m_nUnit) {
		// Byte
		case 0:
			dwData = m_pMemory->ReadOnly(dwAddr);
			break;
		// Word
		case 1:
			dwData = m_pMemory->ReadOnly(dwAddr);
			dwData <<= 8;
			dwData |= m_pMemory->ReadOnly(dwAddr + 1);
			break;
		// Long
		case 2:
			dwData = m_pMemory->ReadOnly(dwAddr);
			dwData <<= 8;
			dwData |= m_pMemory->ReadOnly(dwAddr + 1);
			dwData <<= 8;
			dwData |= m_pMemory->ReadOnly(dwAddr + 2);
			dwData <<= 8;
			dwData |= m_pMemory->ReadOnly(dwAddr + 3);
			break;
		// その他
		default:
			dwData = 0;
			ASSERT(FALSE);
			break;
	}
	::UnlockVM();

	// ダイアログ実行
	dlg.m_dwAddr = dwAddr;
	dlg.m_dwValue = dwData;
	dlg.m_nSize = m_nUnit;
	if (dlg.DoModal() != IDOK) {
		return;
	}

	// 書き込み
	dwData = dlg.m_dwValue;
	::LockVM();
	switch (m_nUnit) {
		// Byte
		case 0:
			m_pMemory->WriteByte(dwAddr, dwData);
			break;
		// Word
		case 1:
			m_pMemory->WriteWord(dwAddr, dwData);
			break;
		// Long
		case 2:
			m_pMemory->WriteWord(dwAddr, (WORD)(dwData >> 16));
			m_pMemory->WriteWord(dwAddr + 2, (WORD)dwData);
			break;
		// その他
		default:
			ASSERT(FALSE);
			break;
	}
	::UnlockVM();
}

//---------------------------------------------------------------------------
//
//	コンテキストメニュー
//
//---------------------------------------------------------------------------
void CMemoryWnd::OnContextMenu(CWnd *pWnd, CPoint point)
{
	CRect rect;
	CMenu menu;
	CMenu *pMenu;

	// クライアント領域内で押されたか判定する
	GetClientRect(&rect);
	ClientToScreen(&rect);
	if (!rect.PtInRect(point)) {
		CSubTextScrlWnd::OnContextMenu(pWnd, point);
		return;
	}

	// メニュー実行
	if (::IsJapanese()) {
		menu.LoadMenu(IDR_MEMORYMENU);
	}
	else {
		menu.LoadMenu(IDR_US_MEMORYMENU);
	}
	pMenu = menu.GetSubMenu(0);
	SetupContext(pMenu);
	pMenu->TrackPopupMenu(TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_RIGHTBUTTON,
							point.x, point.y, this);
}

//---------------------------------------------------------------------------
//
//	コンテキストメニュー セットアップ
//
//---------------------------------------------------------------------------
void FASTCALL CMemoryWnd::SetupContext(CMenu *pMenu)
{
	ASSERT(pMenu);

	// 新しいウィンドウ
	if (!m_pDrawView->IsNewWindow(FALSE)) {
		pMenu->EnableMenuItem(IDM_MEMORY_NEWWIN, MF_BYCOMMAND | MF_GRAYED);
	}

	// サイズチェック
	pMenu->CheckMenuRadioItem(IDM_MEMORY_BYTE, IDM_MEMORY_LONG,
			IDM_MEMORY_BYTE + m_nUnit, MF_BYCOMMAND);

	// アドレスチェック
	pMenu->CheckMenuRadioItem(IDM_MEMORY_0, IDM_MEMORY_F,
			IDM_MEMORY_0 + (m_dwAddr >> 20), MF_BYCOMMAND);

	// MPUレジスタ
	CCPURegWnd::SetupRegMenu(pMenu, m_pCPU, FALSE);

	// アドレスヒストリ
	CAddrDlg::SetupHisMenu(pMenu);

	// スタック
	SetupStackMenu(pMenu, m_pMemory, m_pCPU);
}

//---------------------------------------------------------------------------
//
//	アドレス入力
//
//---------------------------------------------------------------------------
void CMemoryWnd::OnAddr()
{
	CAddrDlg dlg(this);

	// ダイアログ実行
	dlg.m_dwValue = m_dwAddr | (m_ScrlY * 0x20);
	if (dlg.DoModal() != IDOK) {
		return;
	}

	// アドレスセット
	SetAddr(dlg.m_dwValue);
}

//---------------------------------------------------------------------------
//
//	新しいウィンドウ
//
//---------------------------------------------------------------------------
void CMemoryWnd::OnNewWin()
{
	CMemoryWnd *pWnd;

	// 親ウィンドウに対し、新しいウィンドウの作成を要請
	pWnd = (CMemoryWnd*)m_pDrawView->NewWindow(FALSE);

	// 作成できたら、自分と同じアドレス・サイズを指定
	if (pWnd) {
		pWnd->SetAddr(m_dwAddr | (m_ScrlY * 0x20));
		pWnd->SetUnit(m_nUnit);
	}
}

//---------------------------------------------------------------------------
//
//	表示単位指定
//
//---------------------------------------------------------------------------
void CMemoryWnd::OnUnit(UINT uID)
{
	int unit;

	unit = (int)(uID - IDM_MEMORY_BYTE);
	ASSERT((unit >= 0) && (unit <= 2));

	SetUnit(unit);
}

//---------------------------------------------------------------------------
//
//	表示単位セット
//
//---------------------------------------------------------------------------
void FASTCALL CMemoryWnd::SetUnit(int nUnit)
{
	ASSERT(this);
	ASSERT((nUnit >= 0) && (nUnit <= 2));

	// ロック、変更
	::LockVM();
	m_nUnit = nUnit;
	::UnlockVM();
}

//---------------------------------------------------------------------------
//
//	アドレス範囲指定
//
//---------------------------------------------------------------------------
void CMemoryWnd::OnRange(UINT uID)
{
	DWORD dwAddr;

	ASSERT((uID >= IDM_MEMORY_0) && (uID <= IDM_MEMORY_F));

	dwAddr = (DWORD)(uID - IDM_MEMORY_0);
	dwAddr *= 0x100000;
	dwAddr |= (DWORD)(m_ScrlY * 0x20);
	SetAddr(dwAddr);
}

//---------------------------------------------------------------------------
//
//	レジスタ指定
//
//---------------------------------------------------------------------------
void CMemoryWnd::OnReg(UINT uID)
{
	DWORD dwAddr;

	ASSERT((uID >= IDM_REG_D0) && (uID <= IDM_REG_PC));

	dwAddr = CCPURegWnd::GetRegValue(m_pCPU, uID);
	SetAddr(dwAddr);
}

//---------------------------------------------------------------------------
//
//	エリア指定
//
//---------------------------------------------------------------------------
void CMemoryWnd::OnArea(UINT uID)
{
	CMenu menu;
	TCHAR buf[0x100];
	DWORD dwAddr;

	ASSERT((uID >= IDM_AREA_MPU) && (uID <= IDM_AREA_IPLROM));

	// メニューをロード
	if (::IsJapanese()) {
		menu.LoadMenu(IDR_MEMORYMENU);
	}
	else {
		menu.LoadMenu(IDR_US_MEMORYMENU);
	}

	// 指定IDのメニュー文字列を取得
	menu.GetMenuString(uID, buf, 0x100, MF_BYCOMMAND);

	// "$000000 : "の形式を仮定する
	buf[0] = _T('0');
	buf[7] = _T('\0');
	dwAddr = ::_tcstoul(buf, NULL, 16);
	SetAddr(dwAddr);
}

//---------------------------------------------------------------------------
//
//	ヒストリ指定
//
//---------------------------------------------------------------------------
void CMemoryWnd::OnHistory(UINT uID)
{
	DWORD dwAddr;

	ASSERT((uID >= IDM_HISTORY_0) && (uID <= IDM_HISTORY_9));
	dwAddr = CAddrDlg::GetAddr(uID);
	SetAddr(dwAddr);
}

//---------------------------------------------------------------------------
//
//	スタック指定
//
//---------------------------------------------------------------------------
void CMemoryWnd::OnStack(UINT uID)
{
	DWORD dwAddr;

	ASSERT((uID >= IDM_STACK_0) && (uID <= IDM_STACK_F));
	dwAddr = GetStackAddr(uID, m_pMemory, m_pCPU);
	SetAddr(dwAddr);
}

//---------------------------------------------------------------------------
//
//	アドレス指定
//
//---------------------------------------------------------------------------
void FASTCALL CMemoryWnd::SetAddr(DWORD dwAddr)
{
	int offset;
	CString strCap;

	ASSERT(this);
	ASSERT(dwAddr <= 0x1000000);

	// 上位のみ取り出し
	m_dwAddr = dwAddr & 0xf00000;

	// 下位のみ取り出し
	offset = dwAddr & 0x0fffff;
	offset /= 0x20;

	// スクロール
	m_ScrlY = offset;
	SetScrollPos(SB_VERT, offset, TRUE);

	// キャプション文字列作成
	strCap.Format(_T(" [%d] ($%06X - $%06X)"), (m_dwID & 0xff) - 'A' + 1,
									m_dwAddr, m_dwAddr + 0x0fffff);
	m_CSection.Lock();
	m_strCaptionReq = m_strCaption + strCap;
	m_CSection.Unlock();
}

//---------------------------------------------------------------------------
//
//	メッセージスレッドからの更新
//
//---------------------------------------------------------------------------
void FASTCALL CMemoryWnd::Update()
{
	CString strCap;

	// キャプション文字列を取得
	m_CSection.Lock();
	strCap = m_strCaptionReq;
	m_CSection.Unlock();

	// 比較
	if (m_strCaptionSet != strCap) {
		m_strCaptionSet = strCap;
		SetWindowText(m_strCaptionSet);
	}
}

//---------------------------------------------------------------------------
//
//	セットアップ
//
//---------------------------------------------------------------------------
void FASTCALL CMemoryWnd::Setup()
{
	int x;
	int y;
	CString strText;
	CString strHex;
	DWORD dwAddr;
	DWORD dwOffset;
	TCHAR szAscii[2];

	// クリア、アドレス初期化
	Clear();
	dwAddr = (m_dwAddr & 0xf00000);
	dwOffset = (DWORD)(m_ScrlY << 5);
	dwAddr |= dwOffset;

	// 文字列初期化
	szAscii[1] = _T('\0');

	// yループ
	for (y=0; y<m_nHeight; y++) {
		// オーバーチェック
		if (dwOffset >= 0x100000) {
			break;
		}

		// アドレス表示
		strText.Format(_T("%06X:"), dwAddr);

		// xループ
		switch (m_nUnit) {
			// Byte
			case 0:
				for (x=0; x<16; x++) {
					strHex.Format(_T(" %02X"), m_pMemory->ReadOnly(dwAddr));
					strText += strHex;
					dwAddr++;
				}
				break;
			// Word
			case 1:
				for (x=0; x<8; x++) {
					strHex.Format(_T(" %02X%02X"),  m_pMemory->ReadOnly(dwAddr),
													m_pMemory->ReadOnly(dwAddr + 1));
					strText += strHex;
					dwAddr += 2;
				}
				break;
			// Long
			case 2:
				for (x=0; x<4; x++) {
					strHex.Format(" %02X%02X%02X%02X",  m_pMemory->ReadOnly(dwAddr),
														m_pMemory->ReadOnly(dwAddr + 1),
														m_pMemory->ReadOnly(dwAddr + 2),
														m_pMemory->ReadOnly(dwAddr + 3));
					strText += strHex;
					dwAddr += 4;
				}
				break;
			// その他(ありえない)
			default:
				ASSERT(FALSE);
				break;
		}

		// 一度戻す
		dwAddr -= 0x10;
		dwAddr &= 0xffffff;

		// ASCIIキャラクタ追加
		strText += _T("  ");
		for (x=0; x<16; x++) {
			szAscii[0] = (TCHAR)m_pMemory->ReadOnly(dwAddr + x);
			if ((szAscii[0] >= 0) && (szAscii[0] < 0x20)) {
				szAscii[0] = TCHAR('.');
			}
			if ((szAscii[0] < 0) || (szAscii[0] >= 0x7f)) {
				szAscii[0] = TCHAR('.');
			}
			strText += szAscii;
		}
		dwAddr += 0x10;
		dwAddr &= 0xffffff;

		// オフセットを進める
		dwOffset += 0x10;

		// 表示
		if (m_ScrlX < strText.GetLength()) {
			SetString(0, y, (LPCTSTR)(strText) + m_ScrlX * sizeof(TCHAR));
		}
	}
}

//---------------------------------------------------------------------------
//
//	スクロール準備(垂直)
//
//---------------------------------------------------------------------------
void FASTCALL CMemoryWnd::SetupScrlV()
{
	SCROLLINFO si;
	CRect rect;
	int height;

	// 垂直表示キャラクタ数を取得
	GetClientRect(&rect);
	height = rect.bottom / m_tmHeight;

	// 補正(スクロール単位につき2行のため)
	height >>= 1;

	// スクロール情報をセット
	memset(&si, 0, sizeof(si));
	si.cbSize = sizeof(si);
	si.fMask = SIF_ALL;
	si.nMin = 0;
	si.nMax = m_ScrlHeight - 1;
	si.nPage = height;

	// 位置は、必要なら補正する
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
//	スタックメニューセットアップ
//
//---------------------------------------------------------------------------
void CMemoryWnd::SetupStackMenu(CMenu *pMenu, Memory *pMemory, CPU *pCPU)
{
	int i;
	CPU::cpu_t reg;
	DWORD dwAddr;
	DWORD dwValue;
	CString strMenu;

	ASSERT(pMenu);
	ASSERT(pMemory);
	ASSERT(pCPU);

	// VMロック、レジスタ取得
	::LockVM();
	pCPU->GetCPU(&reg);

	// 16レベル
	for (i=0; i<16; i++) {
		// アドレス算出
		dwAddr = reg.areg[7];
		dwAddr += (i << 1);
		dwAddr &= 0xfffffe;

		// データ取得
		dwValue = pMemory->ReadOnly(dwAddr + 1);
		dwAddr = (dwAddr + 2) & 0xfffffe;
		dwValue <<= 8;
		dwValue |= pMemory->ReadOnly(dwAddr);
		dwValue <<= 8;
		dwValue |= pMemory->ReadOnly(dwAddr + 1);

		// メニュー更新
		strMenu.Format(_T("(A7+%1X) : $%06X"), (i << 1), dwValue);
		pMenu->ModifyMenu(IDM_STACK_0 + i, MF_BYCOMMAND | MF_STRING,
							IDM_STACK_0 + i, (LPCTSTR)strMenu);
	}

	// VMアンロック
	::UnlockVM();
}

//---------------------------------------------------------------------------
//
//	スタックアドレス取得
//
//---------------------------------------------------------------------------
DWORD CMemoryWnd::GetStackAddr(UINT nID, Memory *pMemory, CPU *pCPU)
{
	CPU::cpu_t reg;
	DWORD dwAddr;
	DWORD dwValue;

	ASSERT((nID >= IDM_STACK_0) && (nID <= IDM_STACK_F));
	ASSERT(pMemory);
	ASSERT(pCPU);

	// オフセット算出
	nID -= IDM_STACK_0;
	ASSERT(nID <= 15);
	nID <<= 1;

	// CPUレジスタからアドレス、メモリ算出
	::LockVM();
	pCPU->GetCPU(&reg);
	dwAddr = reg.areg[7];
	dwAddr += nID;
	dwAddr &= 0xfffffe;

	// データ取得
	dwValue = pMemory->ReadOnly(dwAddr + 1);
	dwAddr = (dwAddr + 2) & 0xfffffe;
	dwValue <<= 8;
	dwValue |= pMemory->ReadOnly(dwAddr);
	dwValue <<= 8;
	dwValue |= pMemory->ReadOnly(dwAddr + 1);
	::UnlockVM();

	return dwValue;
}

//===========================================================================
//
//	ブレークポイントウィンドウ
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	コンストラクタ
//
//---------------------------------------------------------------------------
CBreakPWnd::CBreakPWnd()
{
	// ウィンドウパラメータ定義
	m_dwID = MAKEID('B', 'R', 'K', 'P');
	::GetMsg(IDS_SWND_BREAKP, m_strCaption);
	m_nWidth = 43;
	m_nHeight = Scheduler::BreakMax + 1;

	// スケジューラ取得
	m_pScheduler = (Scheduler*)::GetVM()->SearchDevice(MAKEID('S', 'C', 'H', 'E'));
	ASSERT(m_pScheduler);
}

//---------------------------------------------------------------------------
//
//	メッセージ マップ
//
//---------------------------------------------------------------------------
BEGIN_MESSAGE_MAP(CBreakPWnd, CSubTextWnd)
	ON_WM_LBUTTONDBLCLK()
	ON_WM_CONTEXTMENU()
	ON_COMMAND(IDM_BREAKP_ENABLE, OnEnable)
	ON_COMMAND(IDM_BREAKP_CLEAR, OnClear)
	ON_COMMAND(IDM_BREAKP_DEL, OnDel)
	ON_COMMAND(IDM_BREAKP_ADDR, OnAddr)
	ON_COMMAND_RANGE(IDM_HISTORY_0, IDM_HISTORY_9, OnHistory)
	ON_COMMAND(IDM_BREAKP_ALL, OnAll)
END_MESSAGE_MAP()

//---------------------------------------------------------------------------
//
//	左ボタンダブルクリック
//
//---------------------------------------------------------------------------
void CBreakPWnd::OnLButtonDblClk(UINT /*nFlags*/, CPoint point)
{
	int y;
	Scheduler::breakpoint_t buf;

	// y取得、補正(-1)、チェック
	y = point.y / m_tmHeight;
	y--;
	if ((y < 0) || (y >= Scheduler::BreakMax)) {
		return;
	}

	// ロック、ブレークポイント取得
	::LockVM();
	m_pScheduler->GetBreak(y, &buf);

	// 使用中なら反転
	if (buf.use) {
		m_pScheduler->EnableBreak(y, !(buf.enable));
	}

	// アンロック
	::UnlockVM();
}

//---------------------------------------------------------------------------
//
//	コンテキストメニュー
//
//---------------------------------------------------------------------------
void CBreakPWnd::OnContextMenu(CWnd *pWnd, CPoint point)
{
	CRect rect;
	CMenu menu;
	CMenu *pMenu;

	// クライアント領域内で押されたか判定する
	GetClientRect(&rect);
	ClientToScreen(&rect);
	if (!rect.PtInRect(point)) {
		CSubTextWnd::OnContextMenu(pWnd, point);
		return;
	}

	// 位置記憶
	m_Point = point;

	// メニュー実行
	if (::IsJapanese()) {
		menu.LoadMenu(IDR_BREAKPMENU);
	}
	else {
		menu.LoadMenu(IDR_US_BREAKPMENU);
	}
	pMenu = menu.GetSubMenu(0);
	SetupContext(pMenu);
	pMenu->TrackPopupMenu(TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_RIGHTBUTTON,
							point.x, point.y, this);
}

//---------------------------------------------------------------------------
//
//	コンテキストメニューセットアップ
//
//---------------------------------------------------------------------------
void FASTCALL CBreakPWnd::SetupContext(CMenu *pMenu)
{
	int y;
	CPoint point;
	Scheduler::breakpoint_t buf;
	int nCount;
	int nBreak;

	ASSERT(pMenu);

	// 初期化
	buf.enable = FALSE;
	buf.use = FALSE;

	// y取得
	point = m_Point;
	ScreenToClient(&point);
	y = point.y / m_tmHeight;
	y--;

	// ブレークポイントの使用数取得と、カレントの取得
	nCount = 0;
	::LockVM();
	for (nBreak=0; nBreak<Scheduler::BreakMax; nBreak++) {
		m_pScheduler->GetBreak(nBreak, &buf);
		if (buf.use) {
			nCount++;
		}
	}
	buf.use = FALSE;
	if ((y >= 0) && (y < Scheduler::BreakMax)) {
		m_pScheduler->GetBreak(y, &buf);
	}
	::UnlockVM();

	// 全て削除
	if (nCount > 0) {
		pMenu->EnableMenuItem(IDM_BREAKP_ALL, MF_BYCOMMAND | MF_ENABLED);
	}
	else {
		pMenu->EnableMenuItem(IDM_BREAKP_ALL, MF_BYCOMMAND | MF_GRAYED);
	}

	// アドレスヒストリ
	CAddrDlg::SetupHisMenu(pMenu);

	// カレントが未使用なら、変更系は無効
	if (!buf.use) {
		pMenu->EnableMenuItem(IDM_BREAKP_ENABLE, MF_BYCOMMAND | MF_GRAYED);
		pMenu->EnableMenuItem(IDM_BREAKP_CLEAR, MF_BYCOMMAND | MF_GRAYED);
		pMenu->EnableMenuItem(IDM_BREAKP_DEL, MF_BYCOMMAND | MF_GRAYED);
		return;
	}

	// 使用中なら、Enableチェック
	if (buf.enable) {
		pMenu->CheckMenuItem(IDM_BREAKP_ENABLE, MF_BYCOMMAND | MF_CHECKED);
	}
	else {
		pMenu->CheckMenuItem(IDM_BREAKP_ENABLE, MF_BYCOMMAND | MF_UNCHECKED);
	}
}

//---------------------------------------------------------------------------
//
//	有効・無効
//
//---------------------------------------------------------------------------
void CBreakPWnd::OnEnable()
{
	int y;
	CPoint point;
	Scheduler::breakpoint_t buf;

	// y取得
	point = m_Point;
	ScreenToClient(&point);
	y = point.y / m_tmHeight;
	y--;
	ASSERT((y >= 0) && (y < Scheduler::BreakMax));

	// ブレークポイント反転
	::LockVM();
	m_pScheduler->GetBreak(y, &buf);
	ASSERT(buf.use);
	m_pScheduler->EnableBreak(y, !(buf.enable));
	::UnlockVM();
}

//---------------------------------------------------------------------------
//
//	回数クリア
//
//---------------------------------------------------------------------------
void CBreakPWnd::OnClear()
{
	int y;
	CPoint point;

	// y取得
	point = m_Point;
	ScreenToClient(&point);
	y = point.y / m_tmHeight;
	y--;
	ASSERT((y >= 0) && (y < Scheduler::BreakMax));

	// 回数クリア
	::LockVM();
	m_pScheduler->ClearBreak(y);
	::UnlockVM();
}

//---------------------------------------------------------------------------
//
//	ブレークポイント削除
//
//---------------------------------------------------------------------------
void CBreakPWnd::OnDel()
{
	int y;
	CPoint point;
	Scheduler::breakpoint_t buf;

	// y取得
	point = m_Point;
	ScreenToClient(&point);
	y = point.y / m_tmHeight;
	y--;
	ASSERT((y >= 0) && (y < Scheduler::BreakMax));

	// 回数クリア
	::LockVM();
	m_pScheduler->GetBreak(y, &buf);
	ASSERT(buf.use);
	m_pScheduler->DelBreak(buf.addr);
	::UnlockVM();
}

//---------------------------------------------------------------------------
//
//	アドレス指定
//
//---------------------------------------------------------------------------
void CBreakPWnd::OnAddr()
{
	int y;
	CPoint point;
	Scheduler::breakpoint_t buf;
	CPU *pCPU;
	CPU::cpu_t reg;
	DWORD dwAddr;
	CAddrDlg dlg(this);

	// y取得
	point = m_Point;
	ScreenToClient(&point);
	y = point.y / m_tmHeight;
	y--;

	::LockVM();
	// 有効なブレークポイントを指していれば、そのアドレス
	dwAddr = 0xffffffff;
	if ((y >= 0) && (y < Scheduler::BreakMax)) {
		m_pScheduler->GetBreak(y, &buf);
		if (buf.use) {
			dwAddr = buf.addr & 0xffffff;
		}
	}
	// そうでなければ、PC
	if (dwAddr == 0xffffffff) {
		pCPU = (CPU*)::GetVM()->SearchDevice(MAKEID('C', 'P', 'U', ' '));
		ASSERT(pCPU);
		pCPU->GetCPU(&reg);
		dwAddr = reg.pc & 0xffffff;
	}
	::UnlockVM();
	ASSERT(dwAddr <= 0xffffff);

	// 入力ダイアログ
	dlg.m_dwValue = dwAddr;
	if (dlg.DoModal() != IDOK) {
		return;
	}

	// 共通ルーチンにまかせる
	SetAddr(dlg.m_dwValue);
}

//---------------------------------------------------------------------------
//
//	アドレスヒストリ
//
//---------------------------------------------------------------------------
void CBreakPWnd::OnHistory(UINT nID)
{
	DWORD dwAddr;

	ASSERT((nID >= IDM_HISTORY_0) && (nID <= IDM_HISTORY_9));

	// 共通ルーチンにまかせる
	dwAddr = CAddrDlg::GetAddr(nID);
	SetAddr(dwAddr);
}

//---------------------------------------------------------------------------
//
//	全て削除
//
//---------------------------------------------------------------------------
void CBreakPWnd::OnAll()
{
	Scheduler::breakpoint_t buf;
	int i;

	// 全てクリア
	::LockVM();
	for (i=0; i<Scheduler::BreakMax; i++) {
		m_pScheduler->GetBreak(i, &buf);
		if (buf.use) {
			m_pScheduler->DelBreak(buf.addr);
		}
	}
	::UnlockVM();
}

//---------------------------------------------------------------------------
//
//	アドレス設定
//
//---------------------------------------------------------------------------
void FASTCALL CBreakPWnd::SetAddr(DWORD dwAddr)
{
	int y;
	CPoint point;
	Scheduler::breakpoint_t buf;

	ASSERT(dwAddr <= 0xffffff);

	// 既に登録されているアドレスなら、無効
	::LockVM();
	for (y=0; y<Scheduler::BreakMax; y++) {
		m_pScheduler->GetBreak(y, &buf);
		if (buf.use) {
			if (buf.addr == dwAddr) {
				::UnlockVM();
				return;
			}
		}
	}
	::UnlockVM();

	// y取得
	point = m_Point;
	ScreenToClient(&point);
	y = point.y / m_tmHeight;
	y--;

	// 使用中のブレークポイントを指していれば、そこを差し替え
	::LockVM();
	if ((y >= 0) && (y < Scheduler::BreakMax)) {
		m_pScheduler->GetBreak(y, &buf);
		if (buf.use) {
			m_pScheduler->AddrBreak(y, dwAddr);
			::UnlockVM();
			return;
		}
	}

	// そうでなければ、新規設定
	m_pScheduler->SetBreak(dwAddr);
	::UnlockVM();
}

//---------------------------------------------------------------------------
//
//	セットアップ
//
//---------------------------------------------------------------------------
void FASTCALL CBreakPWnd::Setup()
{
	int i;
	CString strText;
	CString strFmt;
	Scheduler::breakpoint_t buf;

	// クリア
	Clear();

	// ガイド表示
	SetString(0, 0, _T("No."));
	SetString(5, 0, _T("Address"));
	SetString(14, 0, _T("Flag"));
	SetString(28, 0, _T("Time"));
	SetString(38, 0, _T("Count"));

	// ループ
	for (i=0; i<Scheduler::BreakMax; i++) {
		// 番号
		strText.Format(_T("%2d "), i + 1);

		// 取得、有効チェック
		m_pScheduler->GetBreak(i, &buf);
		if (buf.use) {
			// アドレス
			strFmt.Format(_T("  $%06X "), buf.addr);
			strText += strFmt;

			// フラグ
			if (buf.enable) {
				strText += _T(" Enable");
			}
			else {
				strText += _T("Disable");
			}

			// 時間
			if (buf.count > 0) {
				strFmt.Format(_T(" %7d.%05dms"), (buf.time / 2000), (buf.time % 2000) * 5);
				strText += strFmt;

				// カウント
				strFmt.Format(_T("   %4d"), buf.count);
				strText += strFmt;
			}
		}

		// 文字列セット
		SetString(0, i + 1, strText);
	}
}

#endif	// _WIN32
