//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2005 �o�h�D(ytanaka@ipc-tokai.or.jp)
//	[ MFC �R���|�[�l���g ]
//
//---------------------------------------------------------------------------

#if defined(_WIN32)

#if !defined(mfc_com_h)
#define mfc_com_h

//===========================================================================
//
//	�R���|�[�l���g
//
//===========================================================================
class CComponent : public CObject
{
public:
	// ��{�t�@���N�V����
	CComponent(CFrmWnd *pFrmWnd);
										// �R���X�g���N�^
	virtual ~CComponent();
										// �f�X�g���N�^
	virtual BOOL FASTCALL Init();
										// ������
	virtual void FASTCALL Cleanup();
										// �N���[���A�b�v
	virtual void FASTCALL Enable(BOOL bEnable)	{ m_bEnable = bEnable; }
										// ���쐧��
	BOOL FASTCALL IsEnable() const		{ return m_bEnable; }
										// �����Ԃ��擾
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

	// �v���p�e�B
	DWORD FASTCALL GetID() const		{ return m_dwID; }
										// ID�擾
	void FASTCALL GetDesc(CString& strDesc) const;
										// ���̎擾

	// �R���|�[�l���g�Ǘ�
	CComponent* FASTCALL SearchComponent(DWORD dwID);
										// �R���|�[�l���g����
	void FASTCALL AddComponent(CComponent *pNewComponent);
										// �R���|�[�l���g�ǉ�
	CComponent* FASTCALL GetPrevComponent() const	{ return m_pPrev; }
										// �O�̃R���|�[�l���g���擾
	CComponent* FASTCALL GetNextComponent() const	{ return m_pNext; }
										// ���̃R���|�[�l���g���擾

protected:
	CFrmWnd *m_pFrmWnd;
										// �t���[���E�B���h�E
	DWORD m_dwID;
										// �R���|�[�l���gID
	BOOL m_bEnable;
										// �L���t���O
	CString m_strDesc;
										// ����
	CComponent *m_pPrev;
										// �O�̃R���|�[�l���g
	CComponent *m_pNext;
										// ���̃R���|�[�l���g
};

#endif	// mfc_com_h
#endif	// _WIN32
