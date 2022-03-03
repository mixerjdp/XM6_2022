//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2005 ＰＩ．(ytanaka@ipc-tokai.or.jp)
//	[ MFC コンポーネント ]
//
//---------------------------------------------------------------------------

#if defined(_WIN32)

#if !defined(mfc_com_h)
#define mfc_com_h

//===========================================================================
//
//	コンポーネント
//
//===========================================================================
class CComponent : public CObject
{
public:
	// 基本ファンクション
	CComponent(CFrmWnd *pFrmWnd);
										// コンストラクタ
	virtual ~CComponent();
										// デストラクタ
	virtual BOOL FASTCALL Init();
										// 初期化
	virtual void FASTCALL Cleanup();
										// クリーンアップ
	virtual void FASTCALL Enable(BOOL bEnable)	{ m_bEnable = bEnable; }
										// 動作制御
	BOOL FASTCALL IsEnable() const		{ return m_bEnable; }
										// 動作状態を取得
	virtual BOOL FASTCALL Save(Fileio *pFio, int nVer);
										// セーブ
	virtual BOOL FASTCALL Load(Fileio *pFio, int nVer);
										// ロード
	virtual void FASTCALL ApplyCfg(const Config *pConfig);
										// 設定適用
#if !defined(NDEBUG)
		void AssertValid() const;
										// 診断
#endif	// NDEBUG

	// プロパティ
	DWORD FASTCALL GetID() const		{ return m_dwID; }
										// ID取得
	void FASTCALL GetDesc(CString& strDesc) const;
										// 名称取得

	// コンポーネント管理
	CComponent* FASTCALL SearchComponent(DWORD dwID);
										// コンポーネント検索
	void FASTCALL AddComponent(CComponent *pNewComponent);
										// コンポーネント追加
	CComponent* FASTCALL GetPrevComponent() const	{ return m_pPrev; }
										// 前のコンポーネントを取得
	CComponent* FASTCALL GetNextComponent() const	{ return m_pNext; }
										// 次のコンポーネントを取得

protected:
	CFrmWnd *m_pFrmWnd;
										// フレームウィンドウ
	DWORD m_dwID;
										// コンポーネントID
	BOOL m_bEnable;
										// 有効フラグ
	CString m_strDesc;
										// 名称
	CComponent *m_pPrev;
										// 前のコンポーネント
	CComponent *m_pNext;
										// 次のコンポーネント
};

#endif	// mfc_com_h
#endif	// _WIN32
