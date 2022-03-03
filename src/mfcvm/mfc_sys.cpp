//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 �o�h�D(ytanaka@ipc-tokai.or.jp)
//	[ MFC �T�u�E�B���h�E(�V�X�e��) ]
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
//	���O�E�B���h�E
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	�R���X�g���N�^
//
//---------------------------------------------------------------------------
CLogWnd::CLogWnd() : CSubListWnd()
{
	// �E�B���h�E�p�����[�^��`
	m_dwID = MAKEID('L', 'O', 'G', 'L');
	::GetMsg(IDS_SWND_LOG, m_strCaption);
	m_nWidth = 64;
	m_nHeight = 8;

	// �t�H�[�J�X�Ȃ�
	m_nFocus = -1;

	// ���̑�
	m_pLog = NULL;
}

//---------------------------------------------------------------------------
//
//	���b�Z�[�W �}�b�v
//
//---------------------------------------------------------------------------
BEGIN_MESSAGE_MAP(CLogWnd, CSubListWnd)
	ON_WM_CREATE()
	ON_WM_DRAWITEM()
	ON_NOTIFY(NM_DBLCLK, 0, OnDblClick)
END_MESSAGE_MAP()

//---------------------------------------------------------------------------
//
//	�E�B���h�E�쐬
//
//---------------------------------------------------------------------------
int CLogWnd::OnCreate(LPCREATESTRUCT lpcs)
{
	ASSERT(this);

	// ��{�N���X
	if (CSubListWnd::OnCreate(lpcs) != 0) {
		return -1;
	}

	// �������ǂݍ���
	::GetMsg(IDS_SWND_LOG_DETAIL, m_strDetail);
	::GetMsg(IDS_SWND_LOG_NORMAL, m_strNormal);
	::GetMsg(IDS_SWND_LOG_WARNING, m_strWarning);

	// ���O���擾
	ASSERT(!m_pLog);
	m_pLog = &(::GetVM()->log);
	ASSERT(m_pLog);

	return 0;
}

//---------------------------------------------------------------------------
//
//	�_�u���N���b�N
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

	// �t�H�[�J�X������ꍇ�Ɍ���
	if (m_nFocus < 0) {
		return;
	}

	// ���O�f�[�^�擾�A������o�b�t�@���
	bSuccess = m_pLog->GetData(m_nFocus, &logdata);
	if (!bSuccess) {
		// �o�b�t�@�̉���K�v���Ȃ�
		return;
	}

	// �t�A�Z���u���E�B���h�E�փA�h���X�w��
	pView = (CDrawView*)GetParent();
	pDisWnd = (CDisasmWnd*)pView->SearchSWnd(MAKEID('D', 'I', 'S', 'A'));
	if (pDisWnd) {
		pDisWnd->SetAddr(logdata.pc);
	}

	// GetData()�Ŋm�ۂ����o�b�t�@�����
	delete[] logdata.string;
}

//---------------------------------------------------------------------------
//
//	�I�[�i�[�`��
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

	// DC���A�^�b�`
	VERIFY(dc.Attach(lpDIS->hDC));

	// �����Ȃ�󔒂�`��
	if (!m_bEnable) {
		dc.FillSolidRect(&rectItem, ::GetSysColor(COLOR_WINDOW));
		dc.Detach();
		return;
	}

	// ���O�f�[�^�擾
	if (!m_pLog->GetData(lpDIS->itemID, &logdata)) {
		// �Ȃ���΋󔒂�\��
		dc.FillSolidRect(&rectItem, ::GetSysColor(COLOR_WINDOW));
		dc.Detach();
		return;
	}

	// �\������
	VERIFY(mDC.CreateCompatibleDC(&dc));
	VERIFY(Bitmap.CreateCompatibleBitmap(&dc, rectItem.Width(), rectItem.Height()));
	pBitmap = mDC.SelectObject(&Bitmap);
	ASSERT(pBitmap);

	// �t�H���g�ړ]
	pFont = mDC.SelectObject(m_ListCtrl.GetFont());
	ASSERT(pFont);

	// �n�F�����߂ăx�^�h�肷��
	rectClient.left = 0;
	rectClient.right = rectItem.Width();
	rectClient.top = 0;
	rectClient.bottom = rectItem.Height();
	if (lpDIS->itemState & ODS_FOCUS) {
		// �t�H�[�J�X�ԍ����L�^����̂�Y�ꂸ��
		m_nFocus = lpDIS->itemID;
		mDC.FillSolidRect(&rectClient, GetSysColor(COLOR_HIGHLIGHT));
		mDC.SetTextColor(::GetSysColor(COLOR_HIGHLIGHTTEXT));
		mDC.SetBkColor(::GetSysColor(COLOR_HIGHLIGHT));
	}
	else {
		// �t�H�[�J�X�Ȃ�
		mDC.FillSolidRect(&rectClient, ::GetSysColor(COLOR_WINDOW));
		mDC.SetBkColor(::GetSysColor(COLOR_WINDOW));
	}

	// �T�u�A�C�e�������Ԃɕ\��
	for (nItem=0; nItem<6; nItem++) {
		// ���W�쐬
		nWidth = m_ListCtrl.GetColumnWidth(nItem);
		rectClient.right = rectClient.left + nWidth;

		// �e�L�X�g�擾
		TextSub(nItem, &logdata, strText);

		// �F�ݒ�
		if (!(lpDIS->itemState & ODS_FOCUS)) {
			mDC.SetTextColor(RGB(0, 0, 0));
			if (nItem == 3) {
				switch (logdata.level) {
					// �ڍ�(�D�F)
					case Log::Detail:
						mDC.SetTextColor(RGB(192, 192, 192));
						break;
					// �x��(�ԐF)
					case Log::Warning:
						mDC.SetTextColor(RGB(255, 0, 0));
						break;
				}
			}
		}

		// �\��
		switch (nItem) {
			// No.(�E�l��)
			case 0:
			// ���z����(�E�l��)
			case 1:
				mDC.DrawText(strText, &rectClient, DT_SINGLELINE | DT_RIGHT | DT_VCENTER);
				break;

			// PC(��������)
			case 2:
			// ���x��(��������)
			case 3:
			// �f�o�C�X(��������)
			case 4:
				mDC.DrawText(strText, &rectClient, DT_SINGLELINE | DT_CENTER | DT_VCENTER);
				break;

			// ���b�Z�[�W(���l��)
			case 5:
#if !defined(UNICODE)
				// ���{��Ȃ�AVM����̃V�t�gJIS����������̂܂ܕ\���ł���
				if (::IsJapanese()) {
					// ���{��
					mDC.DrawText(strText, &rectClient, DT_SINGLELINE | DT_LEFT | DT_VCENTER);
					break;
				}

				// CP932�T�|�[�g��
				if (::Support932()) {
					// CP932�T�|�[�g(UNICODE���T�|�[�g����Ă���)
					memset(&szWide[0], 0, sizeof(szWide));
					if (_tcslen(strText) < 0x100) {
						// �V�t�gJIS����AUNICODE�֕ϊ�����
						::MultiByteToWideChar(932, MB_PRECOMPOSED, (LPCSTR)(LPCTSTR)strText, -1, szWide, 100);

						// UNICODE�Ƃ��ĕ\��
						::DrawTextWide(mDC.m_hDC, szWide, -1, rectClient, DT_SINGLELINE | DT_LEFT | DT_VCENTER);
					}
					break;
				}

				// �\���ł��Ȃ�
				if (::IsWinNT()) {
					strText = _T("CP932(Japanese Shift-JIS) is required");
				}
				else {
					strText = _T("(Japanese Text)");
				}
#endif	// !UNICODE
				mDC.DrawText(strText, &rectClient, DT_SINGLELINE | DT_LEFT | DT_VCENTER);
				break;

			// ���̑�(���肦�Ȃ�)
			default:
				ASSERT(FALSE);
		}

		// ��`�X�V
		rectClient.left += nWidth;
	}

	// �\���I��
	VERIFY(dc.BitBlt(rectItem.left, rectItem.top, rectItem.Width(), rectItem.Height(),
						&mDC, 0, 0, SRCCOPY));
	VERIFY(mDC.SelectObject(pFont));
	VERIFY(mDC.SelectObject(pBitmap));
	VERIFY(Bitmap.DeleteObject());
	VERIFY(mDC.DeleteDC());
	dc.Detach();

	// ������o�b�t�@���
	delete[] logdata.string;
}

//---------------------------------------------------------------------------
//
//	���X�g�R���g���[��������
//
//---------------------------------------------------------------------------
void FASTCALL CLogWnd::InitList()
{
	CString strHeader;

	ASSERT(this);
	ASSERT_VALID(this);

	// �J����0(�ʂ��ԍ�)
	::GetMsg(IDS_SWND_LOG_NUMBER, strHeader);
	m_ListCtrl.InsertColumn(0, strHeader, LVCFMT_RIGHT, m_tmWidth * 7, 0);

	// �J����1(����)
	::GetMsg(IDS_SWND_LOG_TIME, strHeader);
	m_ListCtrl.InsertColumn(1, strHeader, LVCFMT_RIGHT, m_tmWidth * 10, 1);

	// �J����2(PC)
	::GetMsg(IDS_SWND_LOG_PC, strHeader);
	m_ListCtrl.InsertColumn(2, strHeader, LVCFMT_CENTER, m_tmWidth * 8, 2);

	// �J����3(���x��)
	::GetMsg(IDS_SWND_LOG_LEVEL, strHeader);
	m_ListCtrl.InsertColumn(3, strHeader, LVCFMT_CENTER, m_tmWidth * 6, 3);

	// �J����4(�f�o�C�X)
	::GetMsg(IDS_SWND_LOG_DEVICE, strHeader);
	m_ListCtrl.InsertColumn(4, strHeader, LVCFMT_CENTER, m_tmWidth * 8, 4);

	// �J����5(���b�Z�[�W)
	::GetMsg(IDS_SWND_LOG_MSG, strHeader);
	m_ListCtrl.InsertColumn(5, strHeader, LVCFMT_LEFT, m_tmWidth * 32, 5);

	// �S�č폜
	m_ListCtrl.DeleteAllItems();
}

//---------------------------------------------------------------------------
//
//	���t���b�V��
//
//---------------------------------------------------------------------------
void FASTCALL CLogWnd::Refresh()
{
	// Update�Ń��b�Z�[�W�X���b�h���珈�����邽�߁A�������Ȃ�
}

//---------------------------------------------------------------------------
//
//	���b�Z�[�W�X���b�h����̍X�V
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

	// �����Ȃ牽�����Ȃ�
	if (!m_bEnable) {
		return;
	}

	// ���݂̐��𓾂āA�A�C�e�����𒲐�
	nItem = m_ListCtrl.GetItemCount();
	nLog = m_pLog->GetNum();

	// �k��
	if (nItem > nLog) {
		m_ListCtrl.DeleteAllItems();
		nItem = 0;
	}

	// �g��
	if (nItem < nLog) {
		// �_�~�[�������p��
		strEmpty.Empty();

		// 1024�A�C�e���ȏ�̏ꍇ�A��UVM���~�߂āA�A�C�e���𑝂₷
		if ((nLog - nItem) >= 0x400) {
			// ���̊g��͎��Ԃ������邽�߁AVM���~�߂�
			bRun = m_pSch->IsEnable();
			m_pSch->Enable(FALSE);

			// �g��
			while (nItem < nLog) {
				m_ListCtrl.InsertItem(nItem, strEmpty);
				nItem++;
			}

			// VM�𕜌�
			m_pSch->Enable(bRun);
			m_pSch->Reset();
		}
		else {
			// 1023�A�C�e���ȉ��Ȃ̂ŁAVM�����s���Ȃ���g��
			while (nItem < nLog) {
				m_ListCtrl.InsertItem(nItem, strEmpty);
				nItem++;
			}
		}
	}

	// ���ɕ`��w���͂��Ȃ������悢���ʂ������炷
}

//---------------------------------------------------------------------------
//
//	������Z�b�g
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
		// �ʂ��ԍ�
		case 0:
			string.Format("%d", pLogData->number);
			return;

		// ���� (s.ms.us)
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

		// ���x��
		case 3:
			switch (pLogData->level) {
				// �ڍ�
				case Log::Detail:
					string = m_strDetail;
					break;
				// �ʏ�
				case Log::Normal:
					string = m_strNormal;
					break;
				// �x��
				case Log::Warning:
					string = m_strWarning;
					break;
				// ���̑�(���肦�Ȃ�)
				default:
					ASSERT(FALSE);
					string.Empty();
					break;
			}
			return;

		// �f�o�C�X
		case 4:
			string.Format(_T("%c%c%c%c"),
						(BYTE)(pLogData->id >> 24),
						(BYTE)(pLogData->id >> 16),
						(BYTE)(pLogData->id >> 8),
						(BYTE)(pLogData->id));
			return;

		// ���b�Z�[�W
		case 5:
#if defined(UNICODE)
			// ������CP932����ϊ����Ă���
			if (::Support932()) {
				// �V�t�gJIS����AUNICODE�֕ϊ�����
				::MultiByteToWideChar(932, MB_PRECOMPOSED, (LPCSTR)pLogData->string, -1, szWide, 100);

				// �Z�b�g
				string = szWide;
			}
			else {
				if (::IsWinNT()) {
					// UNICODE�T�|�[�g����Ă��邪�ACP932������
					string = _T("CP932(Japanese Shift-JIS) is required");
				}
				else {
					// UNICODE�w��Ȃ̂ɁAWin9x�œ������Ă���
					string = _T("(Japanese Text)");
				}
			}
#else
			// UNICODE�łȂ��̂ŁAstring���}���`�o�C�g
			string = pLogData->string;
#endif	// UNICODE
			return;

		// ���̑�(���肦�Ȃ�)
		default:
			break;
	}

	// �ʏ�A�����ɂ͂��Ȃ�
	ASSERT(FALSE);
}

//===========================================================================
//
//	�X�P�W���[���E�B���h�E
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	�R���X�g���N�^
//
//---------------------------------------------------------------------------
CSchedulerWnd::CSchedulerWnd()
{
	// �E�B���h�E�p�����[�^��`
	m_dwID = MAKEID('S', 'C', 'H', 'E');
	::GetMsg(IDS_SWND_SCHEDULER, m_strCaption);
	m_nWidth = 72;
	m_nHeight = 23;

	// �X�P�W���[���擾
	m_pScheduler = (Scheduler*)::GetVM()->SearchDevice(MAKEID('S', 'C', 'H', 'E'));
	ASSERT(m_pScheduler);
}

//---------------------------------------------------------------------------
//
//	�Z�b�g�A�b�v
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

	// �X�P�W���[���������[�N�擾
	m_pScheduler->GetScheduler(&sch);

	// �N���A
	Clear();
	x = 44;

	// ������
	y = 0;

	// �g�[�^�����s����
	SetString(0, y, _T("Passed Time"));
	strText.Format(_T("%12dms"), sch.total / 2000);
	SetString(15, y, strText);
	y++;

	// �������s����
	SetString(0, y, _T("Execution Time"));
	strText.Format(_T("%6d.%1dus"), sch.one / 2, (sch.one & 1) * 5);
	SetString(19, y, strText);
	y++;

	// �t���[�����[�g
	dwRate = m_pSch->GetFrameRate();
	SetString(0, y, _T("Frame Rate"));
	strText.Format(_T("%2d.%1dfps"), dwRate / 10, (dwRate % 10));
	SetString(22, y, strText);

	// �E����
	y = 0;

	// CPU���(�������x)
	SetString(x, y, _T("MPU Speed"));
	strText.Format(_T("%d.%02dMHz"), sch.speed / 100, sch.speed % 100);
	if (sch.speed >= 1000) {
		SetString(x + 20, y, strText);
	}
	else {
		SetString(x + 21, y, strText);
	}
	y++;

	// CPU���(�������ԃT�C�N��)
	SetString(x, y, _T("MPU Over Cycle"));
	strText.Format(_T("%5u"), sch.cycle);
	SetString(x + 23, y, strText);
	y++;

	// CPU���(��������)
	SetString(x, y, _T("MPU Over Time"));
	strText.Format(_T("%3d.%1dus"), sch.time / 2, (sch.time & 1) * 5);
	SetString(x + 21, y, strText);
	y++;

	// �C�x���g�K�C�h
	y++;
	SetString(0, y, _T("No."));
	SetString(6, y, _T("Remain"));
	SetString(20, y, _T("Time"));
	SetString(34, y, _T("User"));
	SetString(42, y, _T("Flag"));
	SetString(48, y, _T("Device"));
	SetString(56, y, _T("Description"));
	y++;

	// �C�x���g���[�v
	pEvent = m_pScheduler->GetFirstEvent();
	nIndex = 0;
	while (pEvent) {
		// ��{�p�����[�^
		strText.Format(_T("%2d %5d.%04dms (%5d.%04dms) $%08X "),
			nIndex + 1,
			pEvent->GetRemain() / 2000,
			(pEvent->GetRemain() % 2000) * 5,
			pEvent->GetTime() / 2000,
			(pEvent->GetTime() % 2000) * 5,
			pEvent->GetUser());
		SetString(0, y, strText);

		// �L���t���O
		if (pEvent->GetTime() != 0) {
			SetString(42, y, _T("Enable"));
		}

		// �f�o�C�X��
		pDevice = pEvent->GetDevice();
		ASSERT(pDevice);
		dwID = pDevice->GetID();
		strText.Format(_T("%c%c%c%c"),
						(BYTE)(dwID >> 24),
						(BYTE)(dwID >> 16),
						(BYTE)(dwID >> 8),
						(BYTE)dwID);
		SetString(49, y, strText);

		// ����
		SetString(54, y, pEvent->GetDesc());

		// ���̃C�x���g��
		pEvent = pEvent->GetNextEvent();
		nIndex++;
		y++;
	}
}

//---------------------------------------------------------------------------
//
//	���b�Z�[�W�X���b�h����̍X�V
//
//---------------------------------------------------------------------------
void FASTCALL CSchedulerWnd::Update()
{
	int nNum;

	ASSERT(this);

	// �X�P�W���[���̂��C�x���g�����擾���A���T�C�Y
	nNum = m_pScheduler->GetEventNum();
	ReSize(m_nWidth, nNum + 5);
}

//===========================================================================
//
//	�f�o�C�X�E�B���h�E
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	�R���X�g���N�^
//
//---------------------------------------------------------------------------
CDeviceWnd::CDeviceWnd()
{
	// �E�B���h�E�p�����[�^��`
	m_dwID = MAKEID('D', 'E', 'V', 'I');
	::GetMsg(IDS_SWND_DEVICE, m_strCaption);

	m_nWidth = 57;
	m_nHeight = 32;
}

//---------------------------------------------------------------------------
//
//	�Z�b�g�A�b�v
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

	// �N���A
	Clear();

	// �ŏ��̃f�o�C�X���擾�A������ID�쐬
	pDevice = ::GetVM()->GetFirstDevice();
	ASSERT(pDevice);
	dwMemID = MAKEID('M', 'E', 'M', ' ');
	bMem = FALSE;

	SetString(0, 0, _T("No. Device Object"));
	SetString(23, 0, _T("Address Range"));
	SetString(41, 0, _T("Description"));

	// ���[�v
	nDevice = 0;
	while (pDevice) {
		// �ԍ�
		strText.Format(_T("%2d  "), nDevice + 1);

		// ID
		dwID = pDevice->GetID();
		strTemp.Format(_T("%c%c%c%c"),
						(BYTE)(dwID >> 24),
						(BYTE)(dwID >> 16),
						(BYTE)(dwID >> 8),
						(BYTE)(dwID));
		strText += strTemp;

		// �|�C���^
		strTemp.Format(_T("  %08x  "), pDevice);
		strText += strTemp;

		// �������t���O
		if (dwID == dwMemID) {
			bMem = TRUE;
		}

		// �������͈�
		if (bMem) {
			pMemDev = (MemDevice *)pDevice;
			strTemp.Format(_T("$%06X - $%06X"),
						pMemDev->GetFirstAddr(), pMemDev->GetLastAddr());
		}
		else {
			strTemp = _T("(NO MEMORY)");
		}
		strText += strTemp;

		// �Z�b�g
		SetString(0, nDevice + 1, strText);

		// ����
		strText = pDevice->GetDesc();
		SetString(39, nDevice + 1, strText);

		// ����
		nDevice++;
		pDevice = pDevice->GetNextDevice();
	}
}

#endif	// _WIN32
