//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2005 �o�h�D(ytanaka@ipc-tokai.or.jp)
//	[ MFC �T�u�E�B���h�E ]
//
//---------------------------------------------------------------------------

#if defined(_WIN32)

#if !defined (mfc_sub_h)
#define mfc_sub_h

//===========================================================================
//
//	�T�u�E�B���h�E
//
//===========================================================================
class CSubWnd : public CWnd
{
public:
	CSubWnd();
										// �R���X�g���N�^
	BOOL FASTCALL Init(CDrawView *pDrawView);
										// ������
	void FASTCALL Enable(BOOL m_bEnable);
										// ���쐧��
	DWORD FASTCALL GetID() const;
										// �E�B���h�EID��Ԃ�
	virtual void FASTCALL Refresh() = 0;
										// ���t���b�V���`��
	virtual void FASTCALL Update();
										// ���b�Z�[�W�X���b�h����X�V
	virtual BOOL FASTCALL Save(Fileio *pFio, int nVer);
										// �Z�[�u
	virtual BOOL FASTCALL Load(Fileio *pFio, int nVer);
										// ���[�h
	virtual void FASTCALL ApplyCfg(const Config *pConfig);
										// �ݒ�K�p
#if !defined(NDEBUG)
	void AssertValid() const;
										// �f�f
#endif	// NDEBUG

	CSubWnd *m_pNextWnd;
										// ���̃E�B���h�E

protected:
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
										// �E�B���h�E�쐬
	afx_msg void OnDestroy();
										// �E�B���h�E�폜
	afx_msg BOOL OnEraseBkgnd(CDC *pDC);
										// �w�i�`��
	afx_msg void OnActivate(UINT nState, CWnd *pWnd, BOOL bMinimized);
										// �A�N�e�B�x�[�g
	void PostNcDestroy();
										// �E�B���h�E�폜����
	CString m_strCaption;
										// �E�B���h�E�L���v�V����
	BOOL m_bEnable;
										// �L���t���O
	DWORD m_dwID;
										// �E�B���h�EID
	CScheduler *m_pSch;
										// �X�P�W���[��(Win32)
	CDrawView *m_pDrawView;
										// Draw�r���[
	CFont *m_pTextFont;
										// �e�L�X�g�t�H���g
	int m_tmWidth;
										// �e�L�X�g��
	int m_tmHeight;
										// �e�L�X�g����
	int m_nWidth;
										// �E�B���h�E��(�L�����N�^�P��)
	int m_nHeight;
										// �E�B���h�E����(�L�����N�^�P��)
	BOOL m_bPopup;
										// �|�b�v�A�b�v�^�C�v

private:
	void FASTCALL SetupTextFont();
										// �e�L�X�g�t�H���g�Z�b�g�A�b�v

	DECLARE_MESSAGE_MAP()
										// ���b�Z�[�W �}�b�v����
};

//===========================================================================
//
//	�T�u�e�L�X�g�E�B���h�E
//
//===========================================================================
class CSubTextWnd : public CSubWnd
{
public:
	CSubTextWnd();
										// �R���X�g���N�^
	void FASTCALL Refresh();
										// ���t���b�V���`��

protected:
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
										// �E�B���h�E�쐬
	afx_msg void OnDestroy();
										// �E�B���h�E�폜
	afx_msg void OnPaint();
										// �`��
	afx_msg void OnSize(UINT nType, int cx, int cy);
										// �T�C�Y�ύX
	virtual void FASTCALL OnDraw(CDC *pDC);
										// �`�惁�C��
	virtual void FASTCALL Setup() = 0;
										// �Z�b�g�A�b�v
	void FASTCALL Clear();
										// �e�L�X�g�o�b�t�@����
	void FASTCALL SetChr(int x, int y, TCHAR ch);
										// �����Z�b�g
	void FASTCALL SetString(int x, int y, LPCTSTR lpszText);
										// ������Z�b�g(x, y�w�肠��)
	void FASTCALL Reverse(BOOL bReverse);
										// ���]�����Z�b�g
	void FASTCALL ReSize(int nWidth, int nHeight);
										// ���T�C�Y
	BOOL m_bReverse;
										// ���]�t���O
	BYTE *m_pTextBuf;
										// �e�L�X�g�o�b�t�@
	BYTE *m_pDrawBuf;
										// �`��o�b�t�@

	DECLARE_MESSAGE_MAP()
										// ���b�Z�[�W �}�b�v����
};

//===========================================================================
//
//	�T�u�e�L�X�g�σE�B���h�E
//
//===========================================================================
class CSubTextSizeWnd : public CSubTextWnd
{
public:
	CSubTextSizeWnd();
										// �R���X�g���N�^
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
										// �E�B���h�E�쐬����

protected:
	virtual void FASTCALL OnDraw(CDC *pDC);
										// �`�惁�C��
};

//===========================================================================
//
//	�T�u�e�L�X�g�X�N���[���E�B���h�E
//
//===========================================================================
class CSubTextScrlWnd : public CSubTextSizeWnd
{
public:
	CSubTextScrlWnd();
										// �R���X�g���N�^
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
										// �E�B���h�E�쐬����

protected:
	afx_msg int OnCreate(LPCREATESTRUCT lpcs);
										// �E�B���h�E�쐬
	afx_msg void OnSize(UINT nType, int cx, int cy);
										// �T�C�Y�ύX
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar *pBar);
										// �����X�N���[��
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar *pBar);
										// �����X�N���[��
	void FASTCALL SetupScrl();
										// �X�N���[������
	virtual void FASTCALL SetupScrlH();
										// �X�N���[������(����)
	virtual void FASTCALL SetupScrlV();
										// �X�N���[������(����)
	int m_ScrlWidth;
										// ���z��ʕ�(�L�����N�^�P��)
	int m_ScrlHeight;
										// ���z��ʍ���(�L�����N�^�P��)
	BOOL m_bScrlH;
										// �����X�N���[���t���O
	BOOL m_bScrlV;
										// �����X�N���[���t���O
	int m_ScrlX;
										// �����I�t�Z�b�g(�L�����N�^�P��)
	int m_ScrlY;
										// �����I�t�Z�b�g(�L�����N�^�P��)

	DECLARE_MESSAGE_MAP()
										// ���b�Z�[�W �}�b�v����
};

//===========================================================================
//
//	�T�u���X�g�E�B���h�E
//
//===========================================================================
class CSubListWnd : public CSubWnd
{
public:
	CSubListWnd();
										// �R���X�g���N�^
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
										// �E�B���h�E�쐬����
	virtual void FASTCALL Refresh() = 0;
										// ���t���b�V��

protected:
	afx_msg int OnCreate(LPCREATESTRUCT lpcs);
										// �E�B���h�E�쐬
	afx_msg void OnSize(UINT nType, int cx, int cy);
										// �T�C�Y�ύX
	afx_msg void OnDrawItem(int nID, LPDRAWITEMSTRUCT lpDIS);
										// �I�[�i�[�h���[
	virtual void FASTCALL InitList() = 0;
										// ���X�g�R���g���[��������
	CListCtrl m_ListCtrl;
										// ���X�g�R���g���[��

	DECLARE_MESSAGE_MAP()
										// ���b�Z�[�W �}�b�v����
};

//===========================================================================
//
//	�T�uBMP�E�B���h�E
//
//===========================================================================
class CSubBMPWnd : public CWnd
{
public:
	CSubBMPWnd();
										// �R���X�g���N�^
	void FASTCALL Refresh();
										// �`��
#if !defined(NDEBUG)
	void AssertValid() const;
										// �f�f
#endif	// NDEBUG

	// ��ʌ���
	void FASTCALL GetMaximumRect(LPRECT lpRect, BOOL bScroll);
										// �ő�E�B���h�E��`�擾
	void FASTCALL GetFitRect(LPRECT lpRect);
										// �t�B�b�g��`�擾
	void FASTCALL GetDrawRect(LPRECT lpRect);
										// �`���`�擾
	BYTE* FASTCALL GetBits() const;
										// �r�b�g�|�C���^�擾
	void FASTCALL DrawBits(CDC *pDC, CPoint point);
										// �`��

	int m_nScrlWidth;
										// ���z��ʕ�
	int m_nScrlHeight;
										// ���z��ʍ���
	int m_nScrlX;
										// �����I�t�Z�b�g
	int m_nScrlY;
										// �����I�t�Z�b�g
	int m_nCursorX;
										// �}�E�X�J�[�\��X
	int m_nCursorY;
										// �}�E�X�J�[�\��Y

protected:
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
										// �E�B���h�E�쐬
	afx_msg void OnDestroy();
										// �E�B���h�E�폜
	afx_msg void OnSize(UINT nType, int cx, int cy);
										// �T�C�Y�ύX
	afx_msg BOOL OnEraseBkgnd(CDC *pDC);
										// �w�i�`��
	afx_msg void OnPaint();
										// �ĕ`��
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar *pBar);
										// �����X�N���[��
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar *pBar);
										// �����X�N���[��
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
										// �}�E�X�ړ�
	void PostNcDestroy();
										// �E�B���h�E�폜����

	// �r�b�g�}�b�v
	BITMAPINFOHEADER m_bmi;
										// �r�b�g�}�b�v�w�b�_
	HANDLE m_hBitmap;
										// �r�b�g�}�b�v�n���h��
	BYTE *m_pBits;
										// �r�b�g�}�b�v�r�b�g
	int m_nMul;
										// �{��(50%�P��)

	// �X�N���[��
	void FASTCALL SetupScrlH();
										// �X�N���[������(����)
	void FASTCALL SetupScrlV();
										// �X�N���[������(����)

	DECLARE_MESSAGE_MAP()
										// ���b�Z�[�W �}�b�v����
};

//===========================================================================
//
//	�T�u�r�b�g�}�b�v�E�B���h�E
//
//===========================================================================
class CSubBitmapWnd : public CSubWnd
{
public:
	CSubBitmapWnd();
										// �R���X�g���N�^
	BOOL PreCreateWindow(CREATESTRUCT& cs);
										// �E�B���h�E�쐬����
	void FASTCALL Refresh();
										// ���t���b�V��
	virtual void FASTCALL Setup(int x, int y, int width, int height, BYTE *ptr) = 0;
										// �Z�b�g�A�b�v

protected:
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
										// �E�B���h�E�쐬
	afx_msg void OnSizing(UINT nSide, LPRECT lpRect);
										// �T�C�Y�ύX��
	afx_msg void OnSize(UINT nType, int cx, int cy);
										// �T�C�Y�ύX
	DWORD FASTCALL ConvPalette(WORD value);
										// �p���b�g�ϊ�
	CSubBMPWnd *m_pBMPWnd;
										// �r�b�g�}�b�v�E�B���h�E
	CStatusBar m_StatusBar;
										// �X�e�[�^�X�o�[
	int m_nScrlWidth;
										// ���z��ʕ�(�h�b�g�P��)
	int m_nScrlHeight;
										// ���z��ʍ���(�h�b�g�P��)

	DECLARE_MESSAGE_MAP()
										// ���b�Z�[�W �}�b�v����
};

#endif	// mfc_sub_h
#endif	// _WIN32
