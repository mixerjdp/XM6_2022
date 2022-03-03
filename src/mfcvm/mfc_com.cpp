//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 ＰＩ．(ytanaka@ipc-tokai.or.jp)
//	[ MFC コンポーネント ]
//
//---------------------------------------------------------------------------

#if defined(_WIN32)

#include "os.h"
#include "xm6.h"
#include "mfc_frm.h"
#include "mfc_com.h"

//===========================================================================
//
//	コンポーネント
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	コンストラクタ
//
//---------------------------------------------------------------------------
CComponent::CComponent(CFrmWnd *pFrmWnd)
{
	// フレームウィンドウを記憶
	ASSERT(pFrmWnd);
	m_pFrmWnd = pFrmWnd;

	// ワークエリア初期化
	m_pPrev = NULL;
	m_pNext = NULL;
	m_dwID = 0;
	m_strDesc.Empty();
	m_bEnable = FALSE;
}

//---------------------------------------------------------------------------
//
//	デストラクタ
//
//---------------------------------------------------------------------------
CComponent::~CComponent()
{
	// 無効化(あまり意味はない)
	m_bEnable = FALSE;
}

//---------------------------------------------------------------------------
//
//	初期化
//
//---------------------------------------------------------------------------
BOOL FASTCALL CComponent::Init()
{
	ASSERT(this);
	return TRUE;
}

//---------------------------------------------------------------------------
//
//	クリーンアップ
//
//---------------------------------------------------------------------------
void FASTCALL CComponent::Cleanup()
{
	ASSERT(this);
}

//---------------------------------------------------------------------------
//
//	セーブ
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
//	ロード
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
//	設定適用
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
//	診断
//
//---------------------------------------------------------------------------
void CComponent::AssertValid() const
{
	ASSERT(this);

	// 基本クラス
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
//	名称を取得
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
//	コンポーネントを検索
//
//---------------------------------------------------------------------------
CComponent* FASTCALL CComponent::SearchComponent(DWORD dwID)
{
	CComponent *pComponent;

	ASSERT(this);

	// 先頭のコンポーネントを得る
	pComponent = this;
	while (pComponent->m_pPrev) {
		ASSERT(pComponent == pComponent->m_pPrev->m_pNext);
		pComponent = pComponent->m_pPrev;
	}

	// IDを検索
	while (pComponent) {
		if (pComponent->GetID() == dwID) {
			return pComponent;
		}

		// 次へ
		pComponent = pComponent->m_pNext;
	}

	// 見つからなかった
	return NULL;
}

//---------------------------------------------------------------------------
//
//	新しいコンポーネントを追加
//
//---------------------------------------------------------------------------
void FASTCALL CComponent::AddComponent(CComponent *pNewComponent)
{
	CComponent *pComponent;

	ASSERT(this);
	ASSERT(pNewComponent);
	ASSERT(!pNewComponent->m_pPrev);
	ASSERT(!pNewComponent->m_pNext);

	// 先頭のコンポーネントを得る
	pComponent = this;

	// 最後のコンポーネントを探す
	while (pComponent->m_pNext) {
		pComponent = pComponent->m_pNext;
	}

	// 双方向リンクで接続
	pComponent->m_pNext = pNewComponent;
	pNewComponent->m_pPrev = pComponent;
}

#endif	// _WIN32
