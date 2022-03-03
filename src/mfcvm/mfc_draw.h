//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2005 �o�h�D(ytanaka@ipc-tokai.or.jp)
//	[ MFC Draw�r���[ ]
//
//---------------------------------------------------------------------------

#if defined(_WIN32)

#if !defined(mfc_draw_h)
#define mfc_draw_h

//===========================================================================
//
//	Draw�r���[
//
//===========================================================================
class CDrawView : public CView
{
public:
	// �������[�N��`
	typedef struct _DRAWINFO {
		BOOL bPower;					// �d��
		Render *pRender;				// �����_��
		Render::render_t *pWork;		// �����_�����[�N
        DWORD dwDrawCount;				// �`���

		// DIB�Z�N�V����
		HBITMAP hBitmap;				// DIB�Z�N�V����
		DWORD *pBits;					// �r�b�g�f�[�^
		int nBMPWidth;					// BMP��
		int nBMPHeight;					// BMP����

		// �����_���A�g
		int nRendWidth;					// �����_����
		int nRendHeight;				// �����_������
		int nRendHMul;					// �����_���������{��
		int nRendVMul;					// �����_���c�����{��
		int nLeft;						// ���}�[�W��
		int nTop;						// �c�}�[�W��
		int nWidth;						// BitBlt��
		int nHeight;					// BitBlt����

		// Blt�n
		int nBltTop;					// �`��J�nY
		int nBltBottom;					// �`��I��Y
		int nBltLeft;					// �`��J�nX
		int nBltRight;					// �`��I��X
		BOOL bBltAll;					// �S�\���t���O
		BOOL bBltStretch;				// �A�X�y�N�g��ɂ��킹�g��
	} DRAWINFO, *LPDRAWINFO;

public:
	// ��{�t�@���N�V����
	CDrawView();
										// �R���X�g���N�^
	void FASTCALL Enable(BOOL bEnable);
										// ���쐧��
	BOOL FASTCALL IsEnable() const;
										// ����t���O�擾
	BOOL FASTCALL Init(CWnd *pParent);
										// ������
	BOOL PreCreateWindow(CREATESTRUCT& cs);
										// �E�B���h�E�쐬����
	void FASTCALL Refresh();
										// ���t���b�V���`��
	void FASTCALL Draw(int index);
										// �`��(����E�B���h�E�̂�)
	void FASTCALL Update();
										// ���b�Z�[�W�X���b�h����̍X�V
	void FASTCALL ApplyCfg(const Config *pConfig);
										// �ݒ�K�p
	void FASTCALL GetDrawInfo(LPDRAWINFO pDrawInfo) const;
										// �`����擾

	// �����_�����O�`��
	void OnDraw(CDC *pDC);
										// �`��
	void FASTCALL Stretch(BOOL bStretch);
										// �g�僂�[�h
	BOOL IsStretch() const				{ return m_Info.bBltStretch; }
										// �g�僂�[�h�擾

	// �T�u�E�B���h�E�Ǘ�
	int FASTCALL GetNewSWnd() const;
										// �T�u�E�B���h�E�V�K�C���f�b�N�X�擾
	void FASTCALL AddSWnd(CSubWnd *pSubWnd);
										// �T�u�E�B���h�E�ǉ�(�q����Ă΂��)
	void FASTCALL DelSWnd(CSubWnd *pSubWnd);
										// �T�u�E�B���h�E�폜(�q����Ă΂��)
	void FASTCALL ClrSWnd();
										// �T�u�E�B���h�E�����ׂč폜
	CSubWnd* FASTCALL GetFirstSWnd() const;
										// �ŏ��̃T�u�E�B���h�E���擾
	CSubWnd* FASTCALL SearchSWnd(DWORD dwID) const;
										// �C��ID�̃T�u�E�B���h�E���擾
	CFont* FASTCALL GetTextFont();
										// �e�L�X�g�t�H���g�擾
	CSubWnd* FASTCALL NewWindow(BOOL bDis);
										// �V�K�E�B���h�E�쐬(Dis, Mem)
	BOOL FASTCALL IsNewWindow(BOOL bDis);
										// �V�K�E�B���h�E�쐬�\���₢���킹
	int FASTCALL GetSubWndNum() const;
										// �T�u�E�B���h�E�̌����擾
	LPCTSTR FASTCALL GetWndClassName() const;
										// �E�B���h�E�N���X���擾
	void FASTCALL SetupBitmap();

protected:
	// WM���b�Z�[�W
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
										// �E�B���h�E�쐬
	afx_msg void OnDestroy();
										// �E�B���h�E�폜
	afx_msg void OnSize(UINT nType, int cx, int cy);
										// �E�B���h�E�T�C�Y�ύX
	afx_msg void OnPaint();
										// �`��
	afx_msg BOOL OnEraseBkgnd(CDC *pDC);
										// �w�i�`��
	afx_msg LRESULT OnDisplayChange(WPARAM wParam, LPARAM lParam);
										// �f�B�X�v���C�ύX
	afx_msg void OnDropFiles(HDROP hDropInfo);
										// �t�@�C���h���b�v
	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
										// �}�E�X�z�C�[��
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
										// �L�[����
	afx_msg void OnSysKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
										// �V�X�e���L�[����
	afx_msg void OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags);
										// �L�[������
	afx_msg void OnSysKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags);
										// �V�X�e���L�[������
	afx_msg void OnMove(int x, int y);
										// �E�B���h�E�ړ�

	BOOL m_bEnable;
										// �L���t���O
	CFont m_TextFont;
										// �e�L�X�g�t�H���g
	
private:
	
										// �r�b�g�}�b�v����
	inline void FASTCALL ReCalc(CRect& rect);
										// �Čv�Z
	inline void FASTCALL DrawRect(CDC *pDC);
										// ���̗͂]����`��
	inline BOOL FASTCALL CalcRect();
										// �`��K�v�̈�𒲂ׂ�
	int FASTCALL MakeBits();
										// �r�b�g�쐬
	BOOL FASTCALL KeyUpDown(UINT nChar, UINT nFlags, BOOL bDown);
										// �L�[����
	CSubWnd *m_pSubWnd;
										// �ŏ��̃T�u�E�B���h�E
	CFrmWnd *m_pFrmWnd;
										// �t���[���E�B���h�E
	CScheduler *m_pScheduler;
										// �X�P�W���[��
	CInput *m_pInput;
										// �C���v�b�g
	DRAWINFO m_Info;
										// �������[�N

	DECLARE_MESSAGE_MAP()
										// ���b�Z�[�W �}�b�v����
};

#endif	// mfc_draw_h
#endif	// _WIN32
