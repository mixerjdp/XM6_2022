//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 �o�h�D(ytanaka@ipc-tokai.or.jp)
//	[ MFC �R���|�[�l���g ]
//
//---------------------------------------------------------------------------

#if defined(_WIN32)

#include "os.h"
#include "xm6.h"
#include "mfc_frm.h"
#include "mfc_com.h"

//===========================================================================
//
//	�R���|�[�l���g
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	�R���X�g���N�^
//
//---------------------------------------------------------------------------
CComponent::CComponent(CFrmWnd *pFrmWnd)
{
	// �t���[���E�B���h�E���L��
	ASSERT(pFrmWnd);
	m_pFrmWnd = pFrmWnd;

	// ���[�N�G���A������
	m_pPrev = NULL;
	m_pNext = NULL;
	m_dwID = 0;
	m_strDesc.Empty();
	m_bEnable = FALSE;
}

//---------------------------------------------------------------------------
//
//	�f�X�g���N�^
//
//---------------------------------------------------------------------------
CComponent::~CComponent()
{
	// ������(���܂�Ӗ��͂Ȃ�)
	m_bEnable = FALSE;
}

//---------------------------------------------------------------------------
//
//	������
//
//---------------------------------------------------------------------------
BOOL FASTCALL CComponent::Init()
{
	ASSERT(this);
	return TRUE;
}

//---------------------------------------------------------------------------
//
//	�N���[���A�b�v
//
//---------------------------------------------------------------------------
void FASTCALL CComponent::Cleanup()
{
	ASSERT(this);
}

//---------------------------------------------------------------------------
//
//	�Z�[�u
//
//---------------------------------------------------------------------------
BOOL FASTCALL CComponent::Save(Fileio* /*pFio*/, int /*nVer*/)
{
	ASSERT(this);
	ASSERT_VALID(this);

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	���[�h
//
//---------------------------------------------------------------------------
BOOL FASTCALL CComponent::Load(Fileio* /*pFio*/, int /*nVer*/)
{
	ASSERT(this);
	ASSERT_VALID(this);

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	�ݒ�K�p
//
//---------------------------------------------------------------------------
void FASTCALL CComponent::ApplyCfg(const Config* /*pConfig*/)
{
	ASSERT(this);
	ASSERT_VALID(this);
}

#if !defined(NDEBUG)
//---------------------------------------------------------------------------
//
//	�f�f
//
//---------------------------------------------------------------------------
void CComponent::AssertValid() const
{
	ASSERT(this);

	// ��{�N���X
	CObject::AssertValid();

	ASSERT(m_pFrmWnd);
	ASSERT(m_dwID != 0);
	ASSERT(m_strDesc.GetLength() > 0);
	ASSERT(m_pPrev || m_pNext);
	ASSERT(!m_pPrev || (m_pPrev->GetNextComponent() == (CComponent*)this));
	ASSERT(!m_pNext || (m_pNext->GetPrevComponent() == (CComponent*)this));
}
#endif	// NDEBUG

//---------------------------------------------------------------------------
//
//	���̂��擾
//
//---------------------------------------------------------------------------
void FASTCALL CComponent::GetDesc(CString& strDesc) const
{
	ASSERT(this);
	ASSERT_VALID(this);

	strDesc = m_strDesc;
}

//---------------------------------------------------------------------------
//
//	�R���|�[�l���g������
//
//---------------------------------------------------------------------------
CComponent* FASTCALL CComponent::SearchComponent(DWORD dwID)
{
	CComponent *pComponent;

	ASSERT(this);

	// �擪�̃R���|�[�l���g�𓾂�
	pComponent = this;
	while (pComponent->m_pPrev) {
		ASSERT(pComponent == pComponent->m_pPrev->m_pNext);
		pComponent = pComponent->m_pPrev;
	}

	// ID������
	while (pComponent) {
		if (pComponent->GetID() == dwID) {
			return pComponent;
		}

		// ����
		pComponent = pComponent->m_pNext;
	}

	// ������Ȃ�����
	return NULL;
}

//---------------------------------------------------------------------------
//
//	�V�����R���|�[�l���g��ǉ�
//
//---------------------------------------------------------------------------
void FASTCALL CComponent::AddComponent(CComponent *pNewComponent)
{
	CComponent *pComponent;

	ASSERT(this);
	ASSERT(pNewComponent);
	ASSERT(!pNewComponent->m_pPrev);
	ASSERT(!pNewComponent->m_pNext);

	// �擪�̃R���|�[�l���g�𓾂�
	pComponent = this;

	// �Ō�̃R���|�[�l���g��T��
	while (pComponent->m_pNext) {
		pComponent = pComponent->m_pNext;
	}

	// �o���������N�Őڑ�
	pComponent->m_pNext = pNewComponent;
	pNewComponent->m_pPrev = pComponent;
}

#endif	// _WIN32
