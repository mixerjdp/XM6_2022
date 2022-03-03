//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2004 ＰＩ．(ytanaka@ipc-tokai.or.jp)
//	[ MFC 補助ツール ]
//
//---------------------------------------------------------------------------

#if defined(_WIN32)

#if !defined (mfc_tool_h)
#define mfc_tool_h

//===========================================================================
//
//	フロッピーディスクイメージ作成ダイアログ
//
//===========================================================================
class CFDIDlg : public CDialog
{
public:
	CFDIDlg(CWnd *pParent);
										// コンストラクタ
	BOOL OnInitDialog();
										// ダイアログ初期化
	void OnOK();
										// ダイアログOK
	void OnCancel();
										// ダイアログキャンセル

	TCHAR m_szFileName[_MAX_PATH];
										// ファイル名
	TCHAR m_szDiskName[60];
										// ディスク名
	DWORD m_dwType;
										// ファイル形式(2HD,DIM,D68)
	DWORD m_dwPhysical;
										// 物理フォーマット
	BOOL m_bLogical;
										// 論理フォーマット
	int m_nDrive;
										// マウントドライブ(-1でマウントしない)

protected:
	afx_msg void OnBrowse();
										// ファイル選択
	afx_msg void OnPhysical();
										// 物理フォーマットクリック

private:
	void FASTCALL MaskName();
										// ディスク名称マスク
	void FASTCALL SetPhysical();
										// 物理フォーマット設定
	void FASTCALL GetPhysical();
										// 物理フォーマット取得
	void FASTCALL MaskPhysical();
										// 物理フォーマットマスク
	void FASTCALL SetLogical();
										// 論理フォーマット設定
	void FASTCALL GetLogical();
										// 論理フォーマット取得
	void FASTCALL MaskLogical();
										// 論理フォーマットマスク
	static const DWORD IDTable[16];
										// 物理フォーマットIDテーブル
	BOOL FASTCALL GetFile();
	 									// ファイル名取得

	DECLARE_MESSAGE_MAP()
										// メッセージ マップあり
};

//===========================================================================
//
//	大容量ディスクイメージ作成ダイアログ
//
//===========================================================================
class CDiskDlg : public CDialog
{
public:
	CDiskDlg(CWnd *pParent);
										// コンストラクタ
	BOOL OnInitDialog();
										// ダイアログ初期化
	void OnOK();
										// ダイアログOK
	BOOL FASTCALL IsSucceeded() const	{ return m_bSucceed; }
										// 作成に成功したか
	BOOL FASTCALL IsCanceled() const	{ return m_bCancel; }
										// キャンセルしたか
	LPCTSTR FASTCALL GetPath() const	{ return m_szPath; }
										// パス名取得
	int m_nType;
										// 種別(0:SASI-HD 1:SCSI-HD 2:SCSI-MO)
	BOOL m_bMount;
										// マウントフラグ(SCSI-MOのみ)

protected:
	afx_msg void OnBrowse();
										// ファイル選択
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pBar);
										// 縦スクロール
	afx_msg void OnMOSize();
										// MOサイズ変更

private:
	void FASTCALL CtrlSASI();
										// SASI-HD コントロール初期化
	void FASTCALL CtrlSCSI();
										// SCSI-HD コントロール初期化
	void FASTCALL CtrlMO();
										// SCSI-MO コントロール初期化
	BOOL FASTCALL GetFile();
										// ファイル選択
	void FASTCALL CreateSASI();
										// SASI-HD ディスク作成
	void FASTCALL CreateSCSI();
										// SCSI-HD ディスク作成
	void FASTCALL CreateMO();
										// SCSI-MO ディスク作成

	TCHAR m_szPath[_MAX_PATH];
										// パス名
	BOOL m_bSucceed;
										// 作成に成功したか
	BOOL m_bCancel;
										// キャンセルしたか
	UINT m_nSize;
										// ディスクサイズ(MB)
	UINT m_nFormat;
										// 論理フォーマット
	static const UINT SASITable[];
										// SASI-HD IDテーブル
	static const UINT SCSITable[];
										// SCSI-HD IDテーブル
	static const UINT MOTable[];
										// SCSI-MO IDテーブル

	DECLARE_MESSAGE_MAP()
										// メッセージ マップあり
};

//===========================================================================
//
//	ディスクイメージ作成ダイアログ
//
//===========================================================================
class CDiskMakeDlg : public CDialog
{
public:
	// 基本ファンクション
	CDiskMakeDlg(CWnd *pParent);
										// コンストラクタ
	BOOL OnInitDialog();
										// ダイアログ初期化
	void OnOK();
										// ダイアログOK
	void OnCancel();
										// ダイアログキャンセル

	// API
	BOOL FASTCALL IsSucceeded() const	{ return m_bSucceed; }
										// 作成に成功したか
	BOOL FASTCALL IsCanceled() const	{ return m_bCancel; }
										// キャンセルしたか
	static UINT ThreadFunc(LPVOID lpParam);
										// スレッド関数
	void FASTCALL Run();
										// スレッドメイン

	// パラメータ
	DWORD m_dwSize;
										// ディスクサイズ
	TCHAR m_szPath[_MAX_PATH];
										// パス名
	int m_nFormat;
										// 論理フォーマット種別

protected:
#if _MFC_VER >= 0x700
	afx_msg void OnTimer(UINT_PTR nTimerID);
#else
	afx_msg void OnTimer(UINT nTimerID);
#endif
										// タイマ
	afx_msg void OnDestroy();
										// ダイアログ削除
	virtual BOOL FASTCALL Format();
										// 論理フォーマット
	DWORD m_dwCurrent;
										// 作成管理

private:
	CWinThread *m_pThread;
										// 作成スレッド
	BOOL m_bThread;
										// スレッド終了フラグ
	CCriticalSection m_CSection;
										// クリティカルセクション
	BYTE *m_pBuffer;
										// バッファ
#if _MFC_VER >= 0x700
	UINT_PTR m_nTimerID;
#else
	UINT m_nTimerID;
#endif
										// タイマID
	DWORD m_dwParcent;
										// 進行パーセント
	BOOL m_bSucceed;
										// 成功フラグ
	BOOL m_bCancel;
										// キャンセルフラグ

	DECLARE_MESSAGE_MAP()
										// メッセージ マップあり
};

//===========================================================================
//
//	SASIディスクイメージ作成ダイアログ
//
//===========================================================================
class CSASIMakeDlg : public CDiskMakeDlg
{
public:
	CSASIMakeDlg(CWnd *pParent);
										// コンストラクタ

protected:
	BOOL FASTCALL Format();
										// フォーマット

private:
	static const BYTE MENU[];
										// メニュー
	static const BYTE IPL[];
										// IPL
};

//===========================================================================
//
//	SCSIディスクイメージ作成ダイアログ
//
//===========================================================================
class CSCSIMakeDlg : public CDiskMakeDlg
{
public:
	CSCSIMakeDlg(CWnd *pParent);
										// コンストラクタ
};

//===========================================================================
//
//	MOディスクイメージ作成ダイアログ
//
//===========================================================================
class CMOMakeDlg : public CDiskMakeDlg
{
public:
	CMOMakeDlg(CWnd *pParent);
										// コンストラクタ

protected:
	BOOL FASTCALL Format();
										// フォーマット

private:
	int m_nMedia;
										// メディアタイプ(0:128MB...3:640MB)
	BOOL FASTCALL FormatIBM();
										// IBMフォーマット
	void FASTCALL MakeBPB(BYTE *pBPB);
										// IBMフォーマットBPB作成
	void FASTCALL MakeSerial(BYTE *pSerial);
										// ボリュームシリアル作成
	BOOL FASTCALL FormatSHARP();
										// SHARPフォーマット
	static const BYTE PartTable128[];
										// パーティションテーブル(128MB)
	static const BYTE PartTable230[];
										// パーティションテーブル(230MB)
	static const BYTE PartTable540[];
										// パーティションテーブル(540MB)
	static const BYTE PartTop128[];
										// 第1パーティション(128MB)
	static const BYTE PartTop230[];
										// 第1パーティション(230MB)
	static const BYTE PartTop540[];
										// 第1パーティション(540MB)
	static const BYTE SCSIMENU[];
										// SCSIパーティション選択メニュー
	static const BYTE SCHDISK[];
										// SCSIディスクドライバ
	static const BYTE SCSIIOCS[];
										// SCSIIOCS補助ドライバ
	static const BYTE IPL[];
										// Human68k IPL
};

//===========================================================================
//
//	trap #0ダイアログ
//
//===========================================================================
class CTrapDlg : public CDialog
{
public:
	CTrapDlg(CWnd *pParent);
										// コンストラクタ
	BOOL OnInitDialog();
										// ダイアログ初期化
	void OnOK();
										// ダイアログOK
	DWORD m_dwCode;
										// コード値

protected:
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar *pBar);
										// 縦スクロール

	DECLARE_MESSAGE_MAP()
										// メッセージ マップあり
};

#endif	// mfc_tool_h
#endif	// _WIN32
