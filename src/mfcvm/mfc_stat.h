//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2005 �o�h�D(ytanaka@ipc-tokai.or.jp)
//	[ MFC �X�e�[�^�X�r���[ ]
//
//---------------------------------------------------------------------------

#if defined(_WIN32)

#if !defined (mfc_stat_h)
#define mfc_stat_h

//===========================================================================
//
//	�X�e�[�^�X�r���[
//
//===========================================================================
class CStatusView : public CWnd
{
public:
	CStatusView();
										// �R���X�g���N�^
	BOOL FASTCALL Init(CFrmWnd *pFrmWnd);
										// ������
	void FASTCALL SetCaptionString(CString& strCap);
										// �L���v�V����������w��
	void FASTCALL SetMenuString(CString& strMenu);
										// ���j���[������w��
	void FASTCALL SetInfoString(CString& strInfo);
										// ��񕶎���w��
	void FASTCALL DrawStatus(int nPane);
										// �X�e�[�^�X�`��

protected:
	void PostNcDestroy();
										// �E�B���h�E�폜����
	afx_msg void OnPaint();
										// �E�B���h�E�ĕ`��
	afx_msg BOOL OnEraseBkgnd(CDC *pDC);
										// �E�B���h�E�w�i�`��
	afx_msg void OnSize(UINT nType, int cx, int cy);
										// �E�B���h�E�T�C�Y�ύX
	afx_msg void OnMove(int x, int y);
										// �E�B���h�E�ʒu�ύX

private:
	// �S��
	CFrmWnd *m_pFrmWnd;
										// �t���[���E�B���h�E
	CRect m_rectClient;
										// �N���C�A���g��`
	LONG m_tmWidth;
										// �L�����N�^����
	LONG m_tmHeight;
										// �L�����N�^����

	// ���b�Z�[�W
	void FASTCALL DrawMessage(CDC *pDC) const;
										// ���b�Z�[�W�`��
	CString m_strIdle;
										// �A�C�h��������
	CString m_strCaption;
										// �L���v�V����������
	CString m_strMenu;
										// ���j���[������
	CString m_strInfo;
										// ���\��������

	// �X�e�[�^�X
	CRect m_rectFDD[2];
										// FDD��`
	CRect m_rectLED[3];
										// LED��`

	DECLARE_MESSAGE_MAP()
										// ���b�Z�[�W �}�b�v����
};

#endif	// mfc_stat_h
#endif	// _WIN32
