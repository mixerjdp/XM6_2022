//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2005 �o�h�D(ytanaka@ipc-tokai.or.jp)
//	[ MFC �T�u�E�B���h�E(Win32) ]
//
//---------------------------------------------------------------------------

#if defined(_WIN32)

#if !defined(mfc_w32_h)
#define mfc_w32_h

#include "keyboard.h"
#include "mfc_sub.h"
#include "mfc_midi.h"

//===========================================================================
//
//	�R���|�[�l���g�E�B���h�E
//
//===========================================================================
class CComponentWnd : public CSubTextWnd
{
public:
	CComponentWnd();
										// �R���X�g���N�^
	void FASTCALL Setup();
										// �Z�b�g�A�b�v

private:
	CComponent *m_pComponent;
										// �ŏ��̃R���|�[�l���g
};

//===========================================================================
//
//	OS���E�B���h�E
//
//===========================================================================
class COSInfoWnd : public CSubTextWnd
{
public:
	COSInfoWnd();
										// �R���X�g���N�^
	void FASTCALL Setup();
										// �Z�b�g�A�b�v
};

//===========================================================================
//
//	�T�E���h�E�B���h�E
//
//===========================================================================
class CSoundWnd : public CSubTextWnd
{
public:
	CSoundWnd();
										// �R���X�g���N�^
	void FASTCALL Setup();
										// �Z�b�g�A�b�v

private:
	Scheduler *m_pScheduler;
										// �X�P�W���[��
	OPMIF *m_pOPMIF;
										// OPM
	ADPCM *m_pADPCM;
										// ADPCM
	CSound *m_pSound;
										// �T�E���h�R���|�[�l���g
};

//===========================================================================
//
//	�C���v�b�g�E�B���h�E
//
//===========================================================================
class CInputWnd : public CSubTextWnd
{
public:
	CInputWnd();
										// �R���X�g���N�^
	void FASTCALL Setup();
										// �Z�b�g�A�b�v

private:
	void FASTCALL SetupInput(int x, int y);
										// �Z�b�g�A�b�v(���͌n�S��)
	void FASTCALL SetupMouse(int x, int y);
										// �Z�b�g�A�b�v(�}�E�X)
	void FASTCALL SetupKey(int x, int y);
										// �Z�b�g�A�b�v(�L�[�{�[�h)
	void FASTCALL SetupJoy(int x, int y, int nJoy);
										// �Z�b�g�A�b�v(�W���C�X�e�B�b�N)
	CInput *m_pInput;
										// �C���v�b�g�R���|�[�l���g
};

//===========================================================================
//
//	�|�[�g�E�B���h�E
//
//===========================================================================
class CPortWnd : public CSubTextWnd
{
public:
	CPortWnd();
										// �R���X�g���N�^
	void FASTCALL Setup();
										// �Z�b�g�A�b�v

private:
	CPort *m_pPort;
										// �|�[�g�R���|�[�l���g
};

//===========================================================================
//
//	�r�b�g�}�b�v�E�B���h�E
//
//===========================================================================
class CBitmapWnd : public CSubTextWnd
{
public:
	CBitmapWnd();
										// �R���X�g���N�^
	void FASTCALL Setup();
										// �Z�b�g�A�b�v

private:
	CDrawView *m_pView;
										// �`��E�B���h�E
};

//===========================================================================
//
//	MIDI�h���C�o�E�B���h�E
//
//===========================================================================
class CMIDIDrvWnd : public CSubTextWnd
{
public:
	CMIDIDrvWnd();
										// �R���X�g���N�^
	void FASTCALL Setup();
										// �Z�b�g�A�b�v

private:
	void FASTCALL SetupInfo(int x, int y, CMIDI::LPMIDIINFO pInfo);
										// �Z�b�g�A�b�v(�T�u)
	void FASTCALL SetupExCnt(int x, int y, DWORD dwStart, DWORD dwEnd);
										// �Z�b�g�A�b�v(�G�N�X�N���[�V�u�J�E���^)
	static LPCTSTR DescTable[];
										// ������e�[�u��
	MIDI *m_pMIDI;
										// MIDI
	CMIDI *m_pMIDIDrv;
										// MIDI�h���C�o
};

//===========================================================================
//
//	�L�[�{�[�h�f�B�X�v���C�E�B���h�E
//
//===========================================================================
class CKeyDispWnd : public CWnd
{
public:
	CKeyDispWnd();
										// �R���X�g���N�^
	void PostNcDestroy();
										// �E�B���h�E�폜����
	void FASTCALL SetShiftMode(UINT nMode);
										// �V�t�g���[�h�ݒ�
	void FASTCALL Refresh(const BOOL *m_pKeyBuf);
										// �L�[�X�V
	void FASTCALL SetKey(const BOOL *m_pKeyBuf);
										// �L�[�ꊇ�ݒ�

protected:
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
										// �E�B���h�E�쐬
	afx_msg void OnDestroy(void);
										// �E�B���h�E�폜
	afx_msg void OnSize(UINT nType, int cx, int cy);
										// �E�B���h�E�T�C�Y�ύX
	afx_msg BOOL OnEraseBkgnd(CDC *pDC);
										// �w�i�`��
	afx_msg void OnPaint();
										// �E�B���h�E�ĕ`��
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
										// ���{�^������
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
										// ���{�^��������
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
										// �E�{�^������
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
										// �E�{�^��������
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
										// �}�E�X�ړ�
	afx_msg UINT OnGetDlgCode();
										// �_�C�A���O�R�[�h�擾

private:
	void FASTCALL SetupBitmap();
										// �r�b�g�}�b�v����
	void FASTCALL OnDraw(CDC *pDC);
										// �E�B���h�E�`��
	LPCTSTR FASTCALL GetKeyString(int nKey);
										// �L�[������擾
	int FASTCALL PtInKey(CPoint& point);
										// ��`���L�[�R�[�h�擾
	void FASTCALL DrawKey(int nKey, BOOL bDown);
										// �L�[�\��
	void FASTCALL DrawBox(int nColorOut, int nColorIn, RECT& rect);
										// �L�[�{�b�N�X�`��
	void FASTCALL DrawCRBox(int nColorOut, int nColorIn, RECT& rect);
										// CR�L�[�{�b�N�X�`��
	void FASTCALL DrawChar(int x, int y, int nColor, DWORD dwChar);
										// �L�����N�^�`��
	void FASTCALL DrawCRChar(int x, int y, int nColor);
										// CR�L�����N�^�`��
	int FASTCALL CalcCGAddr(DWORD dwChar);
										// �S�pCGROM�A�h���X�Z�o
	UINT m_nMode;
										// SHIFT���[�h
	UINT m_nKey[0x80];
										// �L�[���(�\��)
	BOOL m_bKey[0x80];
										// �L�[���(�ŏI)
	int m_nPoint;
										// �}�E�X�ړ��|�C���g
	const BYTE* m_pCG;
										// CGROM
	HBITMAP m_hBitmap;
										// �r�b�g�}�b�v�n���h��
	BYTE *m_pBits;
										// �r�b�g�}�b�v�r�b�g
	UINT m_nBMPWidth;
										// �r�b�g�}�b�v��
	UINT m_nBMPHeight;
										// �r�b�g�}�b�v����
	UINT m_nBMPMul;
										// �r�b�g�}�b�v��Z��
	static RGBQUAD PalTable[0x10];
										// �p���b�g�e�[�u��
	static const RECT RectTable[0x75];
										// ��`�e�[�u��
	static LPCTSTR NormalTable[];
										// ������e�[�u��
	static LPCTSTR KanaTable[];
										// ������e�[�u��
	static LPCTSTR KanaShiftTable[];
										// ������e�[�u��
	static LPCTSTR MarkTable[];
										// ������e�[�u��
	static LPCTSTR AnotherTable[];
										// ������e�[�u��

	DECLARE_MESSAGE_MAP()
										// ���b�Z�[�W �}�b�v����
};

//===========================================================================
//
//	�\�t�g�E�F�A�L�[�{�[�h�E�B���h�E
//
//===========================================================================
class CSoftKeyWnd : public CSubWnd
{
public:
	CSoftKeyWnd();
										// �R���X�g���N�^
	void FASTCALL Refresh();
										// ���t���b�V��

protected:
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
										// �E�B���h�E�쐬
	afx_msg void OnDestroy();
										// �E�B���h�E�폜
	afx_msg void OnActivate(UINT nState, CWnd *pWnd, BOOL bMinimized);
										// �A�N�e�B�x�[�g
	afx_msg LONG OnApp(UINT uParam, LONG lParam);
										// ���[�U(���ʃE�B���h�E����̒ʒm)

private:
	void FASTCALL Analyze(Keyboard::keyboard_t *pKbd);
										// �L�[�{�[�h�f�[�^���
	Keyboard *m_pKeyboard;
										// �L�[�{�[�h
	CInput *m_pInput;
										// �C���v�b�g
	CStatusBar m_StatusBar;
										// �X�e�[�^�X�o�[
	CKeyDispWnd *m_pDispWnd;
										// �L�[�f�B�X�v���C�E�B���h�E
	UINT m_nSoftKey;
										// �\�t�g�������̃L�[

	DECLARE_MESSAGE_MAP()
										// ���b�Z�[�W �}�b�v����
};

#endif	// mfc_w32_h
#endif	// _WIN32
