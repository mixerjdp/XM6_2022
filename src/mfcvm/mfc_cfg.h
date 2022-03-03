//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 ＰＩ．(ytanaka@ipc-tokai.or.jp)
//	[ MFC コンフィグ ]
//
//---------------------------------------------------------------------------

#if defined(_WIN32)

#if !defined(mfc_cfg_h)
#define mfc_cfg_h

#include "config.h"
#include "ppi.h"

//===========================================================================
//
//	コンフィグ
//
//===========================================================================
class CConfig : public CComponent
{
public:
	// 基本ファンクション
	CConfig(CFrmWnd *pWnd);
										// コンストラクタ
	BOOL FASTCALL Init();
										// 初期化
	void FASTCALL Cleanup();
										// クリーンアップ

	// 設定データ(全体)
	void FASTCALL GetConfig(Config *pConfigBuf) const;
										// 設定データ取得
	void FASTCALL SetConfig(Config *pConfigBuf);
										// 設定データ設定

	// 設定データ(個別)
	void FASTCALL SetStretch(BOOL bStretch);
										// 画面拡大設定
	void FASTCALL SetMIDIDevice(int nDevice, BOOL bIn);
										// MIDIデバイス設定

	// MRU
	void FASTCALL SetMRUFile(int nType, LPCTSTR pszFile);
										// MRUファイル設定(最も新しい)
	void FASTCALL GetMRUFile(int nType, int nIndex, LPTSTR pszFile) const;
										// MRUファイル取得
	int FASTCALL GetMRUNum(int nType) const;
										// MRUファイル個数取得

	// セーブ・ロード
	BOOL FASTCALL Save(Fileio *pFio, int nVer);
										// セーブ
	BOOL FASTCALL Load(Fileio *pFio, int nVer);
										// ロード
	BOOL FASTCALL IsApply();
										// 適用するか

private:
	// 設定データ
	typedef struct _INIKEY {
		void *pBuf;						// ポインタ
		LPCTSTR pszSection;				// セクション名
		LPCTSTR pszKey;					// キー名
		int nType;						// 型
		int nDef;						// デフォルト値
		int nMin;						// 最小値(一部タイプのみ)
		int nMax;						// 最大値(一部のみ)
	} INIKEY, *PINIKEY;

	// INIファイル
	TCHAR m_IniFile[FILEPATH_MAX];
										// INIファイル名

	// 設定データ
	void FASTCALL LoadConfig();
										// 設定データロード
	void FASTCALL SaveConfig() const;
										// 設定データセーブ
	static const INIKEY IniTable[];
										// 設定データINIテーブル
	static Config m_Config;
										// 設定データ

	// バージョン互換
	void FASTCALL ResetSASI();
										// SASI再設定
	void FASTCALL ResetCDROM();
										// CD-ROM再設定
	static BOOL m_bCDROM;
										// CD-ROM有効

	// MRU
	enum {
		MruTypes = 5					// MRUタイプ数
	};
	void FASTCALL ClearMRU(int nType);
										// MRUクリア
	void FASTCALL LoadMRU(int nType);
										// MRUロード
	void FASTCALL SaveMRU(int nType) const;
										// MRUセーブ
	TCHAR m_MRUFile[MruTypes][9][FILEPATH_MAX];
										// MRUファイル
	int m_MRUNum[MruTypes];
										// MRU数

	// キー
	void FASTCALL LoadKey() const;
										// キーロード
	void FASTCALL SaveKey() const;
										// キーセーブ

	// TrueKey
	void FASTCALL LoadTKey() const;
										// TrueKeyロード
	void FASTCALL SaveTKey() const;
										// TrueKeyセーブ

	// バージョン互換
	BOOL FASTCALL Load200(Fileio *pFio);
										// version2.00 or version2.01
	BOOL FASTCALL Load202(Fileio *pFio);
										// version2.02 or version2.03

	// ロード・セーブ
	BOOL m_bApply;
										// ロード後ApplyCfgを要求
};

//---------------------------------------------------------------------------
//
//	クラス先行定義
//
//---------------------------------------------------------------------------
class CConfigSheet;

//===========================================================================
//
//	コンフィグプロパティページ
//
//===========================================================================
class CConfigPage : public CPropertyPage
{
public:
	CConfigPage();
										// コンストラクタ
	void FASTCALL Init(CConfigSheet *pSheet);
										// 初期化
	virtual BOOL OnInitDialog();
										// ダイアログ初期化
	virtual BOOL OnSetActive();
										// ページアクティブ
	DWORD FASTCALL GetID() const		{ return m_dwID; }
										// ID取得

protected:
	afx_msg BOOL OnSetCursor(CWnd *pWnd, UINT nHitTest, UINT nMsg);
										// マウスカーソル設定
	Config *m_pConfig;
										// 設定データ
	DWORD m_dwID;
										// ページID
	int m_nTemplate;
										// テンプレートID
	UINT m_uHelpID;
										// ヘルプID
	UINT m_uMsgID;
										// ヘルプメッセージID
	CConfigSheet *m_pSheet;
										// プロパティシート

	DECLARE_MESSAGE_MAP()
										// メッセージ マップあり
};

//===========================================================================
//
//	基本ページ
//
//===========================================================================
class CBasicPage : public CConfigPage
{
public:
	CBasicPage();
										// コンストラクタ
	BOOL OnInitDialog();
										// 初期化
	void OnOK();
										// 決定

protected:
	afx_msg void OnMPUFull();
										// MPUフルスピード

	DECLARE_MESSAGE_MAP()
										// メッセージ マップあり
};

//===========================================================================
//
//	サウンドページ
//
//===========================================================================
class CSoundPage : public CConfigPage
{
public:
	CSoundPage();
										// コンストラクタ
	BOOL OnInitDialog();
										// 初期化
	void OnOK();
										// 決定

protected:
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pBar);
										// 縦スクロール
	afx_msg void OnSelChange();
										// コンボボックス変更

private:
	void FASTCALL EnableControls(BOOL bEnable);
										// コントロール状態変更
	BOOL m_bEnableCtrl;
										// コントロール状態
	static const UINT ControlTable[];
										// コントロールテーブル

	DECLARE_MESSAGE_MAP()
										// メッセージ マップあり
};

//===========================================================================
//
//	音量ページ
//
//===========================================================================
class CVolPage : public CConfigPage
{
public:
	CVolPage();
										// コンストラクタ
	BOOL OnInitDialog();
										// 初期化
	void OnOK();
										// 決定
	void OnCancel();
										// キャンセル

protected:
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar *pBar);
										// 横スクロール
	afx_msg void OnFMCheck();
										// FM音源チェック
	afx_msg void OnADPCMCheck();
										// ADPCM音源チェック
#if _MFC_VER >= 0x700
	afx_msg void OnTimer(UINT_PTR nTimerID);
#else
	afx_msg void OnTimer(UINT nTimerID);
#endif
										// タイマ

private:
	CSound *m_pSound;
										// サウンド
	OPMIF *m_pOPMIF;
										// OPMインタフェース
	ADPCM *m_pADPCM;
										// ADPCM
	CMIDI *m_pMIDI;
										// MIDI
#if _MFC_VER >= 0x700
	UINT_PTR m_nTimerID;
#else
	UINT m_nTimerID;
#endif
										// タイマID
	int m_nMasterVol;
										// マスター音量
	int m_nMasterOrg;
										// マスター音量オリジナル
	int m_nMIDIVol;
										// MIDI音量
	int m_nMIDIOrg;
										// MIDI音量オリジナル

	DECLARE_MESSAGE_MAP()
										// メッセージ マップあり
};

//===========================================================================
//
//	キーボードページ
//
//===========================================================================
class CKbdPage : public CConfigPage
{
public:
	CKbdPage();
										// コンストラクタ
	BOOL OnInitDialog();
										// 初期化
	void OnOK();
										// 決定
	void OnCancel();
										// キャンセル

protected:
	afx_msg void OnEdit();
										// 編集
	afx_msg void OnDefault();
										// デフォルト
	afx_msg void OnClick(NMHDR *pNMHDR, LRESULT *pResult);
										// カラムクリック
	afx_msg void OnRClick(NMHDR *pNMHDR, LRESULT *pResult);
										// カラム右クリック
	afx_msg void OnConnect();
										// 接続

private:
	void FASTCALL UpdateReport();
										// レポート更新
	void FASTCALL EnableControls(BOOL bEnable);
										// コントロール状態変更
	DWORD m_dwEdit[0x100];
										// エディット
	DWORD m_dwBackup[0x100];
										// バックアップ
	CInput *m_pInput;
										// CInput
	BOOL m_bEnableCtrl;
										// コントロール状態
	static const UINT ControlTable[];
										// コントロールテーブル

	DECLARE_MESSAGE_MAP()
										// メッセージ マップあり
};

//===========================================================================
//
//	キーボードマップ編集ダイアログ
//
//===========================================================================
class CKbdMapDlg : public CDialog
{
public:
	CKbdMapDlg(CWnd *pParent, DWORD *pMap);
										// コンストラクタ
	BOOL OnInitDialog();
										// 初期化
	void OnOK();
										// OK
	void OnCancel();
										// キャンセル

protected:
	afx_msg void OnPaint();
										// ダイアログ描画
	afx_msg LONG OnKickIdle(UINT uParam, LONG lParam);
										// アイドル処理
	afx_msg LONG OnApp(UINT uParam, LONG lParam);
										// ユーザ(下位ウィンドウからの通知)

private:
	void FASTCALL OnDraw(CDC *pDC);
										// 描画サブ
	CRect m_rectStat;
										// ステータス矩形
	CString m_strStat;
										// ステータスメッセージ
	CString m_strGuide;
										// ガイドメッセージ
	CWnd *m_pDispWnd;
										// CKeyDispWnd
	CInput *m_pInput;
										// CInput
	DWORD *m_pEditMap;
										// 編集中のマップ

	DECLARE_MESSAGE_MAP()
										// メッセージ マップあり
};

//===========================================================================
//
//	キー入力ダイアログ
//
//===========================================================================
class CKeyinDlg : public CDialog
{
public:
	CKeyinDlg(CWnd *pParent);
										// コンストラクタ
	BOOL OnInitDialog();
										// 初期化
	void OnOK();
										// OK
	UINT m_nTarget;
										// ターゲットキー
	UINT m_nKey;
										// 割り当てキー

protected:
	afx_msg void OnPaint();
										// 描画
	afx_msg UINT OnGetDlgCode();
										// ダイアログコード取得
	afx_msg LONG OnKickIdle(UINT uParam, LONG lParam);
										// アイドル処理
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
										// 右クリック

private:
	void FASTCALL OnDraw(CDC *pDC);
										// 描画サブ
	CInput *m_pInput;
										// CInput
	BOOL m_bKey[0x100];
										// キー記憶用
	CRect m_GuideRect;
										// ガイド矩形
	CString m_GuideString;
										// ガイド文字列
	CRect m_AssignRect;
										// キー割り当て矩形
	CString m_AssignString;
										// キー割り当て文字列
	CRect m_KeyRect;
										// キー矩形
	CString m_KeyString;
										// キー文字列

	DECLARE_MESSAGE_MAP()
										// メッセージ マップあり
};

//===========================================================================
//
//	マウスページ
//
//===========================================================================
class CMousePage : public CConfigPage
{
public:
	CMousePage();
										// コンストラクタ
	BOOL OnInitDialog();
										// 初期化
	void OnOK();
										// 決定

protected:
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar *pBar);
										// 横スクロール
	afx_msg void OnPort();
										// ポート選択

private:
	void FASTCALL EnableControls(BOOL bEnable);
										// コントロール状態変更
	BOOL m_bEnableCtrl;
										// コントロール状態
	static const UINT ControlTable[];
										// コントロールテーブル

	DECLARE_MESSAGE_MAP()
										// メッセージ マップあり
};

//===========================================================================
//
//	ジョイスティックページ
//
//===========================================================================
class CJoyPage : public CConfigPage
{
public:
	CJoyPage();
										// コンストラクタ
	BOOL OnInitDialog();
										// 初期化
	void OnOK();
										// 決定
	void OnCancel();
										// キャンセル

protected:
	BOOL OnCommand(WPARAM wParam, LPARAM lParam);
										// コマンド通知

private:
	void FASTCALL OnSelChg(CComboBox* pComboBox);
										// コンボボックス変更
	void FASTCALL OnDetail(UINT nButton);
										// 詳細
	void FASTCALL OnSetting(UINT nButton);
										// 設定
	CButton* GetCorButton(UINT nComboBox);
										// 対応ボタン取得
	CComboBox* GetCorCombo(UINT nButton);
										// 対応コンボボックス取得
	CInput *m_pInput;
										// CInput
	static UINT ControlTable[];
										// コントロールテーブル
};

//===========================================================================
//
//	ジョイスティック詳細ダイアログ
//
//===========================================================================
class CJoyDetDlg : public CDialog
{
public:
	CJoyDetDlg(CWnd *pParent);
										// コンストラクタ
	BOOL OnInitDialog();
										// 初期化

	CString m_strDesc;
										// デバイス名称
	int m_nPort;
										// ポート番号(0 or 1)
	int m_nType;
										// タイプ(0～12)
};

//===========================================================================
//
//	ボタン設定ページ
//
//===========================================================================
class CBtnSetPage : public CPropertyPage
{
public:
	CBtnSetPage();
										// コンストラクタ
	void FASTCALL Init(CPropertySheet *pSheet);
										// 作成
	BOOL OnInitDialog();
										// 初期化
	void OnOK();
										// 決定
	void OnCancel();
										// キャンセル
	int m_nJoy;
										// ジョイスティック番号(0 or 1)
	int m_nType[PPI::PortMax];
										// ジョイスティックタイプ(0～12)

protected:
	afx_msg void OnPaint();
										// ダイアログ描画
#if _MFC_VER >= 0x700
	afx_msg void OnTimer(UINT_PTR nTimerID);
#else
	afx_msg void OnTimer(UINT nTimerID);
#endif
										// タイマ
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar *pBar);
										// 水平スクロール
	BOOL OnCommand(WPARAM wParam, LPARAM lParam);
										// コマンド通知

private:
	enum CtrlType {
		BtnLabel,						// ラベル(ボタンn)
		BtnCombo,						// コンボボックス
		BtnRapid,						// 連射スライダ
		BtnValue						// 連射ラベル
	};
	void FASTCALL OnDraw(CDC *pDC, BOOL *pButton, BOOL bForce);
										// 描画メイン
	void FASTCALL OnSlider(int nButton);
										// スライダ変更
	void FASTCALL OnSelChg(int nButton);
										// コンボボックス変更
	void FASTCALL GetButtonDesc(const char *pszDesc, CString &strDesc);
										// ボタン表示取得
	UINT FASTCALL GetControl(int nButton, CtrlType ctlType) const;
										// コントロールID取得
	CPropertySheet *m_pSheet;
										// 親シート
	CInput *m_pInput;
										// CInput
	CRect m_rectLabel[12];
										// ラベル位置
	BOOL m_bButton[12];
										// ボタン押下記憶
#if _MFC_VER >= 0x700
	UINT_PTR m_nTimerID;
#else
	UINT m_nTimerID;
#endif
										// タイマID
	static const UINT ControlTable[];
										// コントロールテーブル
	static const int RapidTable[];
										// 連射テーブル

	DECLARE_MESSAGE_MAP()
										// メッセージ マップあり
};

//===========================================================================
//
//	ジョイスティックプロパティシート
//
//===========================================================================
class CJoySheet : public CPropertySheet
{
public:
	CJoySheet(CWnd *pParent);
										// コンストラクタ
	void FASTCALL SetParam(int nJoy, int nCombo, int nType[]);
										// パラメータ設定
	void FASTCALL InitSheet();
										// シート初期化
	int FASTCALL GetAxes() const;
										// 軸数取得
	int FASTCALL GetButtons() const;
										// ボタン数取得

private:
	CBtnSetPage m_BtnSet;
										// ボタン設定ページ
	CInput *m_pInput;
										// CInput
	int m_nJoy;
										// ジョイスティック番号(0 or 1)
	int m_nCombo;
										// コンボボックス選択
	int m_nType[PPI::PortMax];
										// VM側タイプ選択
	DIDEVCAPS m_DevCaps;
										// デバイスCaps
};

//===========================================================================
//
//	SASIページ
//
//===========================================================================
class CSASIPage : public CConfigPage
{
public:
	CSASIPage();
										// コンストラクタ
	BOOL OnInitDialog();
										// 初期化
	void OnOK();
										// 決定
	BOOL OnSetActive();
										// ページアクティブ
	int FASTCALL GetDrives(const Config *pConfig) const;
										// SASIドライブ数取得

protected:
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pBar);
										// 縦スクロール
	afx_msg void OnClick(NMHDR *pNMHDR, LRESULT *pResult);
										// カラムクリック

private:
	void FASTCALL UpdateList();
										// リストコントロール更新
	void FASTCALL CheckSASI(DWORD *pDisk);
										// SASIファイルチェック
	void FASTCALL EnableControls(BOOL bEnable, BOOL bDrive = TRUE);
										// コントロール状態変更
	SASI *m_pSASI;
										// SASI
	BOOL m_bInit;
										// 初期化フラグ
	int m_nDrives;
										// ドライブ数
	TCHAR m_szFile[16][FILEPATH_MAX];
										// SASIハードディスクファイル名
	CString m_strError;
										// エラー文字列

	DECLARE_MESSAGE_MAP()
										// メッセージ マップあり
};

//===========================================================================
//
//	SxSIページ
//
//===========================================================================
class CSxSIPage : public CConfigPage
{
public:
	CSxSIPage();
										// コンストラクタ
	BOOL OnInitDialog();
										// 初期化
	BOOL OnSetActive();
										// ページアクティブ
	void OnOK();
										// 決定
	int FASTCALL GetDrives(const Config *pConfig) const;
										// SxSIドライブ数取得

protected:
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pBar);
										// 縦スクロール
	afx_msg void OnClick(NMHDR *pNMHDR, LRESULT *pResult);
										// カラムクリック
	afx_msg void OnCheck();
										// チェックボックスクリック

private:
	enum DevType {
		DevSASI,						// SASI ハードディスクドライブ
		DevSCSI,						// SCSI ハードディスクドライブ
		DevMO,							// SCSI MOドライブ
		DevInit,						// SCSI イニシエータ(ホスト)
		DevNone							// デバイスなし
	};
	void FASTCALL UpdateList();
										// リストコントロール更新
	void FASTCALL BuildMap();
										// デバイスマップ作成
	int FASTCALL CheckSCSI(int nDrive);
										// SCSIデバイスチェック
	void FASTCALL EnableControls(BOOL bEnable, BOOL bDrive = TRUE);
										// コントロール状態変更
	BOOL m_bInit;
										// 初期化フラグ
	int m_nSASIDrives;
										// SASIドライブ数
	DevType m_DevMap[8];
										// デバイスマップ
	TCHAR m_szFile[6][FILEPATH_MAX];
										// SCSIハードディスクファイル名
	CString m_strSASI;
										// SASI HD文字列
	CString m_strMO;
										// SCSI MO文字列
	CString m_strInit;
										// イニシエータ文字列
	CString m_strNone;
										// n/aを示す文字列
	CString m_strError;
										// デバイスエラー文字列
	static const UINT ControlTable[];
										// コントロールテーブル

	DECLARE_MESSAGE_MAP()
										// メッセージ マップあり
};

//===========================================================================
//
//	SCSIページ
//
//===========================================================================
class CSCSIPage : public CConfigPage
{
public:
	CSCSIPage();
										// コンストラクタ
	BOOL OnInitDialog();
										// 初期化
	void OnOK();
										// 決定
	int FASTCALL GetInterface(const Config *pConfig) const;
										// インタフェース種別取得

protected:
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar* pBar);
										// 縦スクロール
	afx_msg void OnClick(NMHDR *pNMHDR, LRESULT *pResult);
										// カラムクリック
	afx_msg void OnButton();
										// ラジオボタン選択
	afx_msg void OnCheck();
										// チェックボックスクリック

private:
	enum DevType {
		DevSCSI,						// SCSI ハードディスクドライブ
		DevMO,							// SCSI MOドライブ
		DevCD,							// SCSI CD-ROMドライブ
		DevInit,						// SCSI イニシエータ(ホスト)
		DevNone							// デバイスなし
	};
	int FASTCALL GetIfCtrl() const;
										// インタフェース種別取得(コントロールより)
	BOOL FASTCALL CheckROM(int nType) const;
										// ROMチェック
	void FASTCALL UpdateList();
										// リストコントロール更新
	void FASTCALL BuildMap();
										// デバイスマップ作成
	int FASTCALL CheckSCSI(int nDrive);
										// SCSIデバイスチェック
	void FASTCALL EnableControls(BOOL bEnable, BOOL bDrive = TRUE);
										// コントロール状態変更
	SCSI *m_pSCSI;
										// SCSIデバイス
	BOOL m_bInit;
										// 初期化フラグ
	int m_nDrives;
										// ドライブ数
	BOOL m_bMOFirst;
										// MO先頭フラグ
	DevType m_DevMap[8];
										// デバイスマップ
	TCHAR m_szFile[5][FILEPATH_MAX];
										// SCSIハードディスクファイル名
	CString m_strMO;
										// SCSI MO文字列
	CString m_strCD;
										// SCSI CD文字列
	CString m_strInit;
										// イニシエータ文字列
	CString m_strNone;
										// n/aを示す文字列
	CString m_strError;
										// デバイスエラー文字列
	static const UINT ControlTable[];
										// コントロールテーブル

	DECLARE_MESSAGE_MAP()
										// メッセージ マップあり
};

//===========================================================================
//
//	ポートページ
//
//===========================================================================
class CPortPage : public CConfigPage
{
public:
	CPortPage();
										// コンストラクタ
	BOOL OnInitDialog();
										// 初期化
	void OnOK();
										// 決定
};

//===========================================================================
//
//	MIDIページ
//
//===========================================================================
class CMIDIPage : public CConfigPage
{
public:
	CMIDIPage();
										// コンストラクタ
	BOOL OnInitDialog();
										// 初期化
	void OnOK();
										// 決定
	void OnCancel();
										// キャンセル

protected:
	afx_msg void OnVScroll(UINT nSBCode, UINT nPos, CScrollBar *pBar);
										// 縦スクロール
	afx_msg void OnBIDClick();
										// ボードIDクリック

private:
	void FASTCALL EnableControls(BOOL bEnable);
										// コントロール状態変更
	CMIDI *m_pMIDI;
										// MIDIコンポーネント
	BOOL m_bEnableCtrl;
										// コントロール状態
	int m_nInDelay;
										// Inディレイ(ms)
	int m_nOutDelay;
										// Outディレイ(ms)
	static const UINT ControlTable[];
										// コントロールテーブル

	DECLARE_MESSAGE_MAP()
										// メッセージ マップあり
};

//===========================================================================
//
//	改造ページ
//
//===========================================================================
class CAlterPage : public CConfigPage
{
public:
	CAlterPage();
										// コンストラクタ
	BOOL OnInitDialog();
										// 初期化
	BOOL OnKillActive();
										// ページ移動
	BOOL FASTCALL HasParity(const Config *pConfig) const;
										// SASIパリティ機能チェック

protected:
	void DoDataExchange(CDataExchange *pDX);
										// データ交換

private:
	BOOL m_bInit;
										// 初期化
	BOOL m_bParity;
										// パリティあり
};

//===========================================================================
//
//	レジュームページ
//
//===========================================================================
class CResumePage : public CConfigPage
{
public:
	CResumePage();
										// コンストラクタ
	BOOL OnInitDialog();
										// 初期化

protected:
	void DoDataExchange(CDataExchange *pDX);
										// データ交換
};

//===========================================================================
//
//	TrueKey入力ダイアログ
//
//===========================================================================
class CTKeyDlg : public CDialog
{
public:
	CTKeyDlg(CWnd *pParent);
										// コンストラクタ
	BOOL OnInitDialog();
										// 初期化
	void OnOK();
										// OK
	void OnCancel();
										// キャンセル
	int m_nTarget;
										// ターゲットキー
	int m_nKey;
										// 割り当てキー

protected:
	afx_msg void OnPaint();
										// 描画
	afx_msg UINT OnGetDlgCode();
										// ダイアログコード取得
#if _MFC_VER >= 0x700
	afx_msg void OnTimer(UINT_PTR nTimerID);
										// タイマ
#else
	afx_msg void OnTimer(UINT nTimerID);
										// タイマ
#endif
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
										// 右クリック

private:
	void FASTCALL OnDraw(CDC *pDC);
										// 描画メイン
#if _MFC_VER >= 0x700
	UINT_PTR m_nTimerID;
										// タイマID
#else
	UINT m_nTimerID;
										// タイマID
#endif
	BYTE m_KeyState[0x100];
										// VKキー状態
	CTKey *m_pTKey;
										// TrueKey
	CRect m_rectGuide;
										// ガイド矩形
	CString m_strGuide;
										// ガイド文字列
	CRect m_rectAssign;
										// キー割り当て矩形
	CString m_strAssign;
										// キー割り当て文字列
	CRect m_rectKey;
										// キー矩形
	CString m_strKey;
										// キー文字列

	DECLARE_MESSAGE_MAP()
										// メッセージ マップあり
};

//===========================================================================
//
//	TrueKeyページ
//
//===========================================================================
class CTKeyPage : public CConfigPage
{
public:
	CTKeyPage();
										// コンストラクタ
	BOOL OnInitDialog();
										// 初期化
	void OnOK();
										// 決定

protected:
	afx_msg void OnSelChange();
										// コンボボックス変更
	afx_msg void OnClick(NMHDR *pNMHDR, LRESULT *pResult);
										// カラムクリック
	afx_msg void OnRClick(NMHDR *pNMHDR, LRESULT *pResult);
										// カラム右クリック

private:
	void FASTCALL EnableControls(BOOL bEnable);
										// コントロール状態変更
	void FASTCALL UpdateReport();
										// レポート更新
	BOOL m_bEnableCtrl;
										// コントロール有効フラグ
	CInput *m_pInput;
										// CInput
	CTKey *m_pTKey;
										// TrueKey
	int m_nKey[0x73];
										// 編集中の変換テーブル
	static const UINT ControlTable[];
										// コントロールテーブル

	DECLARE_MESSAGE_MAP()
										// メッセージ マップあり
};

//===========================================================================
//
//	その他ページ
//
//===========================================================================
class CMiscPage : public CConfigPage
{
public:
	CMiscPage();
	BOOL OnInitDialog();
	void OnBuscarFolder();
	void OnOK();
										// コンストラクタ


	void DoDataExchange(CDataExchange *pDX);
										// データ交換
										
DECLARE_MESSAGE_MAP()
};

//===========================================================================
//
//	コンフィグプロパティシート
//
//===========================================================================
class CConfigSheet : public CPropertySheet
{
public:
	CConfigSheet(CWnd *pParent);
										// コンストラクタ
	Config *m_pConfig;
										// 設定データ
	CConfigPage* FASTCALL SearchPage(DWORD dwID) const;
										// ページ検索

protected:
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
										// ウィンドウ作成
	afx_msg void OnDestroy();
										// ウィンドウ削除
#if _MFC_VER >= 0x700
	afx_msg void OnTimer(UINT_PTR nTimerID);
#else
	afx_msg void OnTimer(UINT nTimerID);
#endif
										// タイマ

private:
	CFrmWnd *m_pFrmWnd;
										// フレームウィンドウ
#if _MFC_VER >= 0x700
	UINT_PTR m_nTimerID;
#else
	UINT m_nTimerID;
#endif
										// タイマID

	CBasicPage m_Basic;
										// 基本
	CSoundPage m_Sound;
										// サウンド
	CVolPage m_Vol;
										// 音量
	CKbdPage m_Kbd;
										// キーボード
	CMousePage m_Mouse;
										// マウス
	CJoyPage m_Joy;
										// ジョイスティック
	CSASIPage m_SASI;
										// SASI
	CSxSIPage m_SxSI;
										// SxSI
	CSCSIPage m_SCSI;
										// SCSI
	CPortPage m_Port;
										// ポート
	CMIDIPage m_MIDI;
										// MIDI
	CAlterPage m_Alter;
										// 改造
	CResumePage m_Resume;
										// レジューム
	CTKeyPage m_TKey;
										// TrueKey
	CMiscPage m_Misc;
										// その他

	DECLARE_MESSAGE_MAP()
										// メッセージ マップあり
};

#endif	// mfc_cfg_h
#endif	// _WIN32
