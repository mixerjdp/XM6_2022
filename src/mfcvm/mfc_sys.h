//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 �o�h�D(ytanaka@ipc-tokai.or.jp)
//	[ MFC �T�u�E�B���h�E(�V�X�e��) ]
//
//---------------------------------------------------------------------------

#if defined(_WIN32)

#if !defined(mfc_sys_h)
#define mfc_sys_h

#include "mfc_sub.h"
#include "log.h"

//===========================================================================
//
//	���O�E�B���h�E
//
//===========================================================================
class CLogWnd : public CSubListWnd
{
public:
	CLogWnd();
										// �R���X�g���N�^
	void FASTCALL Refresh();
										// ���t���b�V���`��
	void FASTCALL Update();
										// ���b�Z�[�W�X���b�h����̍X�V

protected:
	afx_msg int OnCreate(LPCREATESTRUCT lpcs);
										// �E�B���h�E�쐬
	afx_msg void OnDblClick(NMHDR *pNotifyStruct, LRESULT *result);
										// �_�u���N���b�N
	afx_msg void OnDrawItem(int nID, LPDRAWITEMSTRUCT lpdis);
										// �I�[�i�[�`��
	void FASTCALL InitList();
										// ���X�g�R���g���[��������

private:
	void FASTCALL TextSub(int nType, Log::logdata_t *pLogData, CString& string);
										// ������Z�b�g
	Log *m_pLog;
										// ���O
	CString m_strDetail;
										// �ڍ�
	CString m_strNormal;
										// �ʏ�
	CString m_strWarning;
										// �x��
	int m_nFocus;
										// �t�H�[�J�X�ԍ�

	DECLARE_MESSAGE_MAP()
										// ���b�Z�[�W �}�b�v����
};

//===========================================================================
//
//	�X�P�W���[���E�B���h�E
//
//===========================================================================
class CSchedulerWnd : public CSubTextWnd
{
public:
	CSchedulerWnd();
										// �R���X�g���N�^
	void FASTCALL Setup();
										// �Z�b�g�A�b�v
	void FASTCALL Update();
										// ���b�Z�[�W�X���b�h����X�V

private:
	Scheduler *m_pScheduler;
										// �X�P�W���[��
};

//===========================================================================
//
//	�f�o�C�X�E�B���h�E
//
//===========================================================================
class CDeviceWnd : public CSubTextWnd
{
public:
	CDeviceWnd();
										// �R���X�g���N�^
	void FASTCALL Setup();
										// �Z�b�g�A�b�v
};

#endif	// mfc_sys_h
#endif	// _WIN32
