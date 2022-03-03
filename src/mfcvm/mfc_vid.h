//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2005 �o�h�D(ytanaka@ipc-tokai.or.jp)
//	[ MFC �T�u�E�B���h�E(�r�f�I) ]
//
//---------------------------------------------------------------------------

#if defined(_WIN32)

#if !defined(mfc_vid_h)
#define mfc_vid_h

#include "mfc_sub.h"

//===========================================================================
//
//	�T�u�r�f�I�E�B���h�E
//
//===========================================================================
class CSubVideoWnd : public CSubWnd
{
public:
	CSubVideoWnd();
										// �R���X�g���N�^
	BOOL PreCreateWindow(CREATESTRUCT& cs);
										// �E�B���h�E�쐬����
	void FASTCALL Refresh();
										// ���t���b�V��
	void FASTCALL Update();
										// ���b�Z�[�W�X���b�h����̍X�V
#if !defined(NDEBUG)
	void AssertValid() const;
										// �f�f
#endif	// NDEBUG

protected:
	// ���b�Z�[�W �n���h��
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
										// �E�B���h�E�쐬
	afx_msg void OnSizing(UINT nSide, LPRECT lpRect);
										// �T�C�Y�ύX��
	afx_msg void OnSize(UINT nType, int cx, int cy);
										// �T�C�Y�ύX

	// �X�V
	virtual void FASTCALL Setup(CRect& rect, BYTE *pBits) = 0;
										// �Z�b�g�A�b�v

	// �q�E�B���h�E
	CSubBMPWnd *m_pBMPWnd;
										// �r�b�g�}�b�v�E�B���h�E
	CStatusBar m_StatusBar;
										// �X�e�[�^�X�o�[
	int m_nScrlWidth;
										// ���z��ʕ�(�h�b�g�P��)
	int m_nScrlHeight;
										// ���z��ʍ���(�h�b�g�P��)
	int m_nPane;
										// �X�e�[�^�X�o�[�y�C����

	// ����
	DWORD FASTCALL GetColor(DWORD dwColor) const;
										// �J���[�쐬 (16bit����)
	DWORD FASTCALL GetPalette(DWORD dwPalette) const;
										// �J���[�쐬 (�p���b�g�C���f�b�N�X����)
	CRTC *m_pCRTC;
										// CRTC
	VC *m_pVC;
										// VC
	BOOL m_bScroll;
										// �X�N���[���l������
	BOOL m_bPalette;
										// �p���b�g�l������
	BOOL m_bContrast;
										// �R���g���X�g�l������

	DECLARE_MESSAGE_MAP()
										// ���b�Z�[�W �}�b�v����
};


//===========================================================================
//
//	�e�L�X�g��ʃE�B���h�E
//
//===========================================================================
class CTVRAMWnd : public CSubVideoWnd
{
public:
	CTVRAMWnd();
										// �R���X�g���N�^
	void FASTCALL Setup(CRect& rect, BYTE *pBits);
										// �Z�b�g�A�b�v
	void FASTCALL Update();
										// ���b�Z�[�W�X���b�h����̍X�V

private:
	TVRAM *m_pTVRAM;
										// TVRAM
};

//===========================================================================
//
//	�O���t�B�b�N���(1024�~1024)�E�B���h�E
//
//===========================================================================
class CG1024Wnd : public CSubBitmapWnd
{
public:
	CG1024Wnd();
										// �R���X�g���N�^
	void FASTCALL Setup(int x, int y, int width, int height, BYTE *ptr);
										// �Z�b�g�A�b�v
	void FASTCALL Update();
										// ���b�Z�[�W�X���b�h����̍X�V

private:
	const WORD *m_pPalette;
										// VC�p���b�g�A�h���X
	WORD m_WndPalette[16];
										// �E�B���h�E�ێ��p���b�g
	DWORD m_WndColor[16];
										// �E�B���h�E�J���[�e�[�u��
	const BYTE *m_pGVRAM;
										// GVRAM�A�h���X
};

//===========================================================================
//
//	�O���t�B�b�N���(16�F)�E�B���h�E
//
//===========================================================================
class CG16Wnd : public CSubBitmapWnd
{
public:
	CG16Wnd(int nPage);
										// �R���X�g���N�^
	void FASTCALL Setup(int x, int y, int width, int height, BYTE *ptr);
										// �Z�b�g�A�b�v
	void FASTCALL Update();
										// ���b�Z�[�W�X���b�h����̍X�V

private:
	int m_nPage;
										// �y�[�W�ԍ�
	const WORD *m_pPalette;
										// VC�p���b�g�A�h���X
	WORD m_WndPalette[16];
										// �E�B���h�E�ێ��p���b�g
	DWORD m_WndColor[16];
										// �E�B���h�E�J���[�e�[�u��
	const BYTE *m_pGVRAM;
										// GVRAM�A�h���X
};

//===========================================================================
//
//	�O���t�B�b�N���(256�F)�E�B���h�E
//
//===========================================================================
class CG256Wnd : public CSubBitmapWnd
{
public:
	CG256Wnd(int nPage);
										// �R���X�g���N�^
	void FASTCALL Setup(int x, int y, int width, int height, BYTE *ptr);
										// �Z�b�g�A�b�v
	void FASTCALL Update();
										// ���b�Z�[�W�X���b�h����̍X�V

private:
	int m_nPage;
										// �y�[�W�ԍ�
	const WORD *m_pPalette;
										// VC�p���b�g�A�h���X
	WORD m_WndPalette[256];
										// �E�B���h�E�ێ��p���b�g
	DWORD m_WndColor[256];
										// �E�B���h�E�J���[�e�[�u��
	const BYTE *m_pGVRAM;
										// GVRAM�A�h���X
};

//===========================================================================
//
//	�O���t�B�b�N���(65536�F)�E�B���h�E
//
//===========================================================================
class CG64KWnd : public CSubBitmapWnd
{
public:
	CG64KWnd();
										// �R���X�g���N�^
	void FASTCALL Setup(int x, int y, int width, int height, BYTE *ptr);
										// �Z�b�g�A�b�v
	void FASTCALL Update();
										// ���b�Z�[�W�X���b�h����̍X�V

private:
	void FASTCALL Palette();
										// �p���b�g�Đݒ�
	const WORD *m_pPalette;
										// VC�p���b�g�A�h���X
	WORD m_WndPalette[256];
										// �E�B���h�E�ێ��p���b�g
	DWORD m_WndColor[0x10000];
										// �E�B���h�E�J���[�e�[�u��
	const BYTE *m_pGVRAM;
										// GVRAM�A�h���X
};

//===========================================================================
//
//	PCG�E�B���h�E
//
//===========================================================================
class CPCGWnd : public CSubBitmapWnd
{
public:
	CPCGWnd();
										// �R���X�g���N�^
	void FASTCALL Setup(int x, int y, int width, int height, BYTE *ptr);
										// �Z�b�g�A�b�v
	void FASTCALL Update();
										// ���b�Z�[�W�X���b�h����̍X�V
	afx_msg void OnParentNotify(UINT message, LPARAM lParam);
										// �e�E�C���h�E�֒ʒm
	afx_msg void OnContextMenu(CWnd *pWnd, CPoint point);
										// �R���e�L�X�g���j���[
	afx_msg void OnPalette(UINT uID);
										// �p���b�g�ύX

private:
	const WORD *m_pPalette;
										// VC�p���b�g�A�h���X
	WORD m_WndPalette[256];
										// �E�B���h�E�ێ��p���b�g
	DWORD m_WndColor[256];
										// �E�B���h�E�J���[�e�[�u��
	const BYTE *m_pPCG;
										// PCG�A�h���X
	int m_nColor;
										// �J���[�u���b�N

	DECLARE_MESSAGE_MAP()
										// ���b�Z�[�W �}�b�v����
};

//===========================================================================
//
//	BG�E�B���h�E
//
//===========================================================================
class CBGWnd : public CSubBitmapWnd
{
public:
	CBGWnd(int nPage);
										// �R���X�g���N�^
	void FASTCALL Setup(int x, int y, int width, int height, BYTE *ptr);
										// �Z�b�g�A�b�v
	void FASTCALL Update();
										// ���b�Z�[�W�X���b�h����̍X�V

private:
	const WORD *m_pPalette;
										// VC�p���b�g�A�h���X
	WORD m_WndPalette[256];
										// �E�B���h�E�ێ��p���b�g
	DWORD m_WndColor[256];
										// �E�B���h�E�J���[�e�[�u��
	int m_nPage;
										// �y�[�W
	Sprite *m_pSprite;
										// �X�v���C�g�R���g���[��
};

//===========================================================================
//
//	�p���b�g�E�B���h�E
//
//===========================================================================
class CPaletteWnd : public CSubBitmapWnd
{
public:
	CPaletteWnd(BOOL bRend);
										// �R���X�g���N�^
	void FASTCALL Setup(int x, int y, int width, int height, BYTE *ptr);
										// �Z�b�g�A�b�v
	void FASTCALL Update();
										// ���b�Z�[�W�X���b�h����̍X�V

private:
	void FASTCALL SetupRend(DWORD *buf, int no);
										// �Z�b�g�A�b�v(�����_��)
	void FASTCALL SetupVC(DWORD *buf, int no);
										// �Z�b�g�A�b�v(VC)
	BOOL m_bRend;
										// �����_���t���O
	Render *m_pRender;
										// �����_��
	const WORD *m_pVCPal;
										// VC�p���b�g
};

#endif	// mfc_vid_h
#endif	// _WIN32
