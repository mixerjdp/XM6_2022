//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 �o�h�D(ytanaka@ipc-tokai.or.jp)
//	[ MFC �o�[�W�������_�C�A���O ]
//
//---------------------------------------------------------------------------

#if defined(_WIN32)

#if !defined(mfc_ver_h)
#define mfc_ver_h

//===========================================================================
//
//	�o�[�W�������_�C�A���O
//
//===========================================================================
class CAboutDlg : public CDialog
{
public:
	CAboutDlg(CWnd *pParent = NULL);
										// �R���X�g���N�^
	BOOL OnInitDialog();
										// �_�C�A���O������
	void OnOK();
										// OK
	void OnCancel();
										// �L�����Z��

protected:
	afx_msg void OnPaint();
										// �`��
#if _MFC_VER >= 0x800
	afx_msg LRESULT OnNcHitTest(CPoint point);
										// �q�b�g�e�X�g
#else
	afx_msg UINT OnNcHitTest(CPoint point);
										// �q�b�g�e�X�g
#endif	// _MFC_VER
	afx_msg BOOL OnSetCursor(CWnd *pWnd, UINT nHitTest, UINT message);
										// �J�[�\���Z�b�g
#if _MFC_VER >= 0x700
	afx_msg void OnTimer(UINT_PTR nTimerID);
										// �^�C�}
#else
	afx_msg void OnTimer(UINT nTimerID);
										// �^�C�}
#endif	// _MFC_VER

private:
	void FASTCALL DrawURL(CDC *pDC);
										// URL�`��
	void FASTCALL DrawCRT(CDC *pDC);
										// CRT�`��
	void FASTCALL DrawX68k(CDC *pDC);
										// X68k�`��
	void FASTCALL DrawLED(int x, int y, CDC *pDC);
										// LED�`��
	void FASTCALL DrawView(int x, int y, CDC *pDC);
										// �r���[�`��
	CString m_URLString;
										// URL������
	CRect m_URLRect;
										// URL��`
	BOOL m_bURLHit;
										// URL�q�b�g�t���O
	CRect m_IconRect;
										// �A�C�R����`
#if _MFC_VER >= 0x700
	UINT_PTR m_nTimerID;
										// �^�C�}ID
#else
	UINT m_nTimerID;
										// �^�C�}ID
#endif
	RTC *m_pRTC;
										// RTC
	SASI *m_pSASI;
										// SASI
	FDD *m_pFDD;
										// FDD
	CDrawView *m_pDrawView;
										// �`��r���[
	BOOL m_bFloppyLED;
										// �t���b�s�[LED���[�h
	BOOL m_bPowerLED;
										// �d��LED���[�h

	DECLARE_MESSAGE_MAP()
										// ���b�Z�[�W �}�b�v����
};

#endif	// mfc_ver_h
#endif	// _WIN32
