//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001,2002 �o�h�D(ytanaka@ipc-tokai.or.jp)
//	[ MFC �T�u�E�B���h�E(�����_��) ]
//
//---------------------------------------------------------------------------

#if defined(_WIN32)

#if !defined(mfc_rend_h)
#define mfc_rend_h

#include "mfc_sub.h"

//===========================================================================
//
//	�ėp�o�b�t�@�E�B���h�E
//
//===========================================================================
class CRendBufWnd : public CSubBitmapWnd
{
public:
	CRendBufWnd(int nType);
										// �R���X�g���N�^
	void FASTCALL Setup(int x, int y, int width, int height, BYTE *ptr);
										// �Z�b�g�A�b�v
	void FASTCALL Update();
										// ���b�Z�[�W�X���b�h����̍X�V

private:
	int m_nType;
										// �^�C�v
	const DWORD *m_pRendBuf;
										// �e�L�X�g�o�b�t�@
};

//===========================================================================
//
//	�����o�b�t�@�E�B���h�E
//
//===========================================================================
class CMixBufWnd : public CSubBitmapWnd
{
public:
	CMixBufWnd();
										// �R���X�g���N�^
	void FASTCALL Setup(int x, int y, int width, int height, BYTE *ptr);
										// �Z�b�g�A�b�v
	void FASTCALL Update();
										// ���b�Z�[�W�X���b�h����̍X�V

private:
	const Render::render_t *m_pRendWork;
										// �����_�����[�N
};

//===========================================================================
//
//	PCG�o�b�t�@�E�B���h�E
//
//===========================================================================
class CPCGBufWnd : public CSubBitmapWnd
{
public:
	CPCGBufWnd();
										// �R���X�g���N�^
	void FASTCALL Setup(int x, int y, int width, int height, BYTE *ptr);
										// �Z�b�g�A�b�v
	void FASTCALL Update();
										// ���b�Z�[�W�X���b�h����̍X�V

private:
	const DWORD *m_pPCGBuf;
										// PCG�o�b�t�@����
	const DWORD *m_dwPCGBuf;
										// PCG�o�b�t�@�g�p�J�E���^
};

#endif	// mfc_rend_h
#endif	// _WIN32
