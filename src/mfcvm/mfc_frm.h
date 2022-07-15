//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 ＰＩ．(ytanaka@ipc-tokai.or.jp)
//	[ MFC フレームウィンドウ ]
//
//---------------------------------------------------------------------------
#include <Windows.h>
#include <tchar.h>
#include <shlobj.h>
#include <assert.h>

#if defined(_WIN32)

#if !defined(mfc_frm_h)
#define mfc_frm_h

//---------------------------------------------------------------------------
//
//	ウィンドウメッセージ
//
//---------------------------------------------------------------------------
#define WM_KICK			WM_APP				// エミュレータスタート
#define WM_SHELLNOTIFY	(WM_USER + 5)		// ファイルシステム状態変化


//===========================================================================
//
//	フレームウィンドウ
//
//===========================================================================
class CFrmWnd : public CFrameWnd
{
public:
	// 初期化
	CFrmWnd();
										// コンストラクタ
	BOOL Init();
										// 初期化
	BOOL m_bFullScreen;
	// 取得
	CDrawView* FASTCALL GetView() const;
										// 描画ビュー取得
	CComponent* FASTCALL GetFirstComponent() const;
										// 最初のコンポーネントを取得
	CScheduler* FASTCALL GetScheduler() const;
										// スケジューラ取得
	CSound* FASTCALL GetSound() const;
										// サウンド取得
	CInput* FASTCALL GetInput() const;
										// インプット取得
	CPort* FASTCALL GetPort() const;
										// ポート取得
	CMIDI* FASTCALL GetMIDI() const;
										// MIDI取得
	CTKey* FASTCALL GetTKey() const;
										// TrueKey取得
	CHost* FASTCALL GetHost() const;
										// Host取得
	CInfo* FASTCALL GetInfo() const;
										// Info取得
	CConfig* FASTCALL GetConfig() const;
										// コンフィグ取得

	// ステータスビューサポート
	void FASTCALL RecalcStatusView();
										// ステータスビュー再配置

	// サブウィンドウサポート
	LPCTSTR FASTCALL GetWndClassName() const;
										// ウィンドウクラス名取得
	BOOL FASTCALL IsPopupSWnd() const;


	/* Nombre de Archivo XM6  */
	CString NombreArchivoXM6;
	CString RutaCompletaArchivoXM6;
	CString RutaSaveStates;



	// ドラッグ＆ドロップサポート
	BOOL FASTCALL InitCmdSub(int nDrive, LPCTSTR lpszPath);
										// コマンドライン処理 サブ

protected:
	// オーバーライド
	BOOL PreCreateWindow(CREATESTRUCT& cs);
										// ウィンドウ作成準備
	void GetMessageString(UINT nID, CString& rMessage) const;
										// メッセージ文字列提供

	// WMメッセージ
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
										// ウィンドウ作成
	afx_msg void OnClose();
										// ウィンドウクローズ
	afx_msg void OnDestroy();
										// ウィンドウ削除
	afx_msg void OnMove(int x, int y);
										// ウィンドウ移動
	afx_msg LRESULT OnDisplayChange(UINT uParam, LONG lParam);
										// ディスプレイ変更
	afx_msg BOOL OnEraseBkgnd(CDC *pDC);
										// ウィンドウ背景描画
	afx_msg void OnPaint();
										// ウィンドウ描画
	afx_msg void OnActivate(UINT nState, CWnd *pWnd, BOOL bMinimized);
										// アクティベート
#if _MFC_VER >= 0x700
	afx_msg void OnActivateApp(BOOL bActive, DWORD dwThreadID);
#else
	afx_msg void OnActivateApp(BOOL bActive, HTASK hTask);
#endif
										// タスク切り替え
	afx_msg void OnEnterMenuLoop(BOOL bTrackPopup);
										// メニューループ開始
	afx_msg void OnExitMenuLoop(BOOL bTrackPopup);
										// メニューループ終了
	afx_msg void OnParentNotify(UINT message, LPARAM lParam);
										// 親ウィンドウ通知
	afx_msg LONG OnKick(UINT uParam, LONG lParam);
										// キック
	afx_msg void OnDrawItem(int nIDCtl, LPDRAWITEMSTRUCT lpDIS);
										// オーナードロー
	afx_msg void OnContextMenu(CWnd *pWnd, CPoint pos);
										// コンテキストメニュー
	afx_msg LONG OnPowerBroadCast(UINT uParam, LONG lParam);
										// 電源変更通知
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
										// システムコマンド
#if _MFC_VER >= 0x700
	afx_msg BOOL OnCopyData(CWnd* pWnd, COPYDATASTRUCT* pCopyDataStruct);
#else
	afx_msg LONG OnCopyData(UINT uParam, LONG lParam);
										// データ転送
#endif
	afx_msg void OnEndSession(BOOL bEnding);
										// セッション終了
	afx_msg LONG OnShellNotify(UINT uParam, LONG lParam);
										// ファイルシステム状態変化

	// コマンド処理
	afx_msg void OnOpen();

	afx_msg void OnFastOpen();
										// 開く
	afx_msg void OnOpenUI(CCmdUI *pCmdUI);
										// 開く UI
	afx_msg void OnSave();
										// 上書き保存
	afx_msg void OnSaveUI(CCmdUI *pCmdUI);
										// 上書き保存 UI
	afx_msg void OnSaveAs();
										// 名前を付けて保存
	afx_msg void OnSaveAsUI(CCmdUI *pCmdUI);
										// 名前を付けて保存 UI
	afx_msg void OnMRU(UINT uID);
										// MRU
	afx_msg void OnMRUUI(CCmdUI *pCmdUI);
										// MRU UI
	afx_msg void OnReset();
										// リセット
	afx_msg void OnResetUI(CCmdUI *pCmdUI);

	afx_msg void OnScc();
	// リセット
	afx_msg void OnSccUI(CCmdUI* pCmdUI);
										// リセット UI
	afx_msg void OnInterrupt();
										// インタラプト
	afx_msg void OnInterruptUI(CCmdUI *pCmdUI);
										// インタラプト UI
	afx_msg void OnPower();
										// 電源スイッチ
	afx_msg void OnPowerUI(CCmdUI *pCmdUI);
										// 電源スイッチ UI
	afx_msg void OnExit();
										// 終了

	afx_msg void OnFD(UINT uID);
										// フロッピーディスクコマンド
	afx_msg void OnFDOpenUI(CCmdUI *pCmdUI);
										// フロッピーオープン UI
	afx_msg void OnFDEjectUI(CCmdUI *pCmdUI);
										// フロッピーイジェクト UI
	afx_msg void OnFDWritePUI(CCmdUI *pCmdUI);
										// フロッピー書き込み保護 UI
	afx_msg void OnFDForceUI(CCmdUI *pCmdUI);
										// フロッピー強制イジェクト UI
	afx_msg void OnFDInvalidUI(CCmdUI *pCmdUI);
										// フロッピー誤挿入 UI
	afx_msg void OnFDMediaUI(CCmdUI *pCmdUI);
										// フロッピーメディア UI
	afx_msg void OnFDMRUUI(CCmdUI *pCmdUI);
										// フロッピーMRU UI

	afx_msg void OnMOOpen();
										// MOオープン
	afx_msg void OnMOOpenUI(CCmdUI *pCmdUI);
										// MOオープン UI
	afx_msg void OnMOEject();
										// MOイジェクト
	afx_msg void OnMOEjectUI(CCmdUI *pCmdUI);
										// MOイジェクト UI
	afx_msg void OnMOWriteP();
										// MO書き込み保護
	afx_msg void OnMOWritePUI(CCmdUI *pCmdUI);
										// MO書き込み保護 UI
	afx_msg void OnMOForce();
										// MO強制イジェクト
	afx_msg void OnMOForceUI(CCmdUI *pCmdUI);
										// MO強制イジェクト UI
	afx_msg void OnMOMRU(UINT uID);
										// MOMRU
	afx_msg void OnMOMRUUI(CCmdUI *pCmdUI);
										// MOMRU UI

	afx_msg void OnCDOpen();
										// CDオープン
	afx_msg void OnCDOpenUI(CCmdUI *pCmdUI);
										// CDオープン UI
	afx_msg void OnCDEject();
										// CDイジェクト
	afx_msg void OnCDEjectUI(CCmdUI *pCmdUI);
										// CDイジェクト UI
	afx_msg void OnCDForce();
										// CD強制イジェクト
	afx_msg void OnCDForceUI(CCmdUI *pCmdUI);
										// CD強制イジェクト UI
	afx_msg void OnCDMRU(UINT nID);
										// CDMRU
	afx_msg void OnCDMRUUI(CCmdUI *pCmdUI);
										// CDMRU UI

	afx_msg void OnLog();
										// ログ
	afx_msg void OnLogUI(CCmdUI *pCmdUI);
										// ログ UI
	afx_msg void OnScheduler();
										// スケジューラ
	afx_msg void OnSchedulerUI(CCmdUI *pCmdUI);
										// スケジューラ UI
	afx_msg void OnDevice();
										// デバイス
	afx_msg void OnDeviceUI(CCmdUI *pCmdUI);
										// デバイス UI
	afx_msg void OnCPUReg();
										// CPUレジスタ
	afx_msg void OnCPURegUI(CCmdUI *pCmdUI);
										// CPUレジスタ UI
	afx_msg void OnInt();
										// 割り込み
	afx_msg void OnIntUI(CCmdUI *pCmdUI);
										// 割り込み UI
	afx_msg void OnDisasm();
										// 逆アセンブル
	afx_msg void OnDisasmUI(CCmdUI *pCmdUI);
										// 逆アセンブル UI
	afx_msg void OnMemory();
										// メモリ
	afx_msg void OnMemoryUI(CCmdUI *pCmdUI);
										// メモリ UI
	afx_msg void OnBreakP();
										// ブレークポイント
	afx_msg void OnBreakPUI(CCmdUI *pCmdUI);
										// ブレークポイント UI
	afx_msg void OnMFP();
										// MFP
	afx_msg void OnMFPUI(CCmdUI *pCmdUI);
										// MFP UI
	afx_msg void OnDMAC();
										// DMAC
	afx_msg void OnDMACUI(CCmdUI *pCmdUI);
										// DMAC UI
	afx_msg void OnCRTC();
										// CRTC
	afx_msg void OnCRTCUI(CCmdUI *pCmdUI);
										// CRTC UI
	afx_msg void OnVC();
										// VC
	afx_msg void OnVCUI(CCmdUI *pCmdUI);
										// VC UI
	afx_msg void OnRTC();
										// RTC
	afx_msg void OnRTCUI(CCmdUI *pCmdUI);
										// RTC UI
	afx_msg void OnOPM();
										// OPM
	afx_msg void OnOPMUI(CCmdUI *pCmdUI);
										// OPM UI
	afx_msg void OnKeyboard();
										// キーボード
	afx_msg void OnKeyboardUI(CCmdUI *pCmdUI);
										// キーボード UI
	afx_msg void OnFDD();
										// FDD
	afx_msg void OnFDDUI(CCmdUI *pCmdUI);
										// FDD UI
	afx_msg void OnFDC();
										// FDC
	afx_msg void OnFDCUI(CCmdUI *pCmdUI);
										// FDC UI
	afx_msg void OnSCC();
										// SCC
	afx_msg void OnSCCUI(CCmdUI *pCmdUI);
										// SCC UI
	afx_msg void OnCynthia();
										// CYNTHIA
	afx_msg void OnCynthiaUI(CCmdUI *pCmdUI);
										// CYNTHIA UI
	afx_msg void OnSASI();
										// SASI
	afx_msg void OnSASIUI(CCmdUI *pCmdUI);
										// SASI UI
	afx_msg void OnMIDI();
										// MIDI
	afx_msg void OnMIDIUI(CCmdUI *pCmdUI);
										// MIDI UI
	afx_msg void OnSCSI();
										// SCSI
	afx_msg void OnSCSIUI(CCmdUI *pCmdUI);
										// SCSI UI
	afx_msg void OnTVRAM();
										// テキスト画面
	afx_msg void OnTVRAMUI(CCmdUI *pCmdUI);
										// テキスト画面 UI
	afx_msg void OnG1024();
										// グラフィック画面1024×1024
	afx_msg void OnG1024UI(CCmdUI *pCmdUI);
										// グラフィック画面1024×1024 UI
	afx_msg void OnG16(UINT uID);
										// グラフィック画面16色
	afx_msg void OnG16UI(CCmdUI *pCmdUI);
										// グラフィック画面16色 UI
	afx_msg void OnG256(UINT uID);
										// グラフィック画面256色
	afx_msg void OnG256UI(CCmdUI *pCmdUI);
										// グラフィック画面256色 UI
	afx_msg void OnG64K();
										// グラフィック画面65536色
	afx_msg void OnG64KUI(CCmdUI *pCmdUI);
										// グラフィック画面65536色 UI
	afx_msg void OnPCG();
										// PCG
	afx_msg void OnPCGUI(CCmdUI *pCmdUI);
										// PCG UI
	afx_msg void OnBG(UINT uID);
										// BG画面
	afx_msg void OnBGUI(CCmdUI *pCmdUI);
										// BG画面 UI
	afx_msg void OnPalet();
										// パレット
	afx_msg void OnPaletUI(CCmdUI *pCmdUI);
										// パレット UI
	afx_msg void OnTextBuf();
										// テキストバッファ
	afx_msg void OnTextBufUI(CCmdUI *pCmdUI);
										// テキストバッファ UI
	afx_msg void OnGrpBuf(UINT uID);
										// グラフィックバッファ
	afx_msg void OnGrpBufUI(CCmdUI *pCmdUI);
										// グラフィックバッファ UI
	afx_msg void OnPCGBuf();
										// PCGバッファ
	afx_msg void OnPCGBufUI(CCmdUI *pCmdUI);
										// PCGバッファ UI
	afx_msg void OnBGSpBuf();
										// BG/スプライトバッファ
	afx_msg void OnBGSpBufUI(CCmdUI *pCmdUI);
										// BG/スプライトバッファ UI
	afx_msg void OnPaletBuf();
										// パレットバッファ
	afx_msg void OnPaletBufUI(CCmdUI *pCmdUI);
										// パレットバッファ UI
	afx_msg void OnMixBuf();
										// 合成バッファ
	afx_msg void OnMixBufUI(CCmdUI *pCmdUI);
										// 合成バッファ UI
	afx_msg void OnComponent();
										// コンポーネント
	afx_msg void OnComponentUI(CCmdUI *pCmdUI);
										// コンポーネント UI
	afx_msg void OnOSInfo();
										// OS情報
	afx_msg void OnOSInfoUI(CCmdUI *pCmdUI);
										// OS情報 UI
	afx_msg void OnSound();
										// サウンド
	afx_msg void OnSoundUI(CCmdUI *pCmdUI);
										// サウンド UI
	afx_msg void OnInput();
										// インプット
	afx_msg void OnInputUI(CCmdUI *pCmdUI);
										// インプット UI
	afx_msg void OnPort();
										// ポート
	afx_msg void OnPortUI(CCmdUI *pCmdUI);
										// ポート UI
	afx_msg void OnBitmap();
										// ビットマップ
	afx_msg void OnBitmapUI(CCmdUI *pCmdUI);
										// ビットマップ UI
	afx_msg void OnMIDIDrv();
										// MIDIドライバ
	afx_msg void OnMIDIDrvUI(CCmdUI *pCmdUI);
										// MIDIドライバ UI
	afx_msg void OnCaption();
										// キャプション
	afx_msg void OnCaptionUI(CCmdUI *pCmdUI);
										// キャプション UI
	afx_msg void OnMenu();
										// メニューバー
	afx_msg void OnMenuUI(CCmdUI *pCmdUI);
										// メニューバー UI
	afx_msg void OnStatus();
										// ステータスバー
	afx_msg void OnStatusUI(CCmdUI *pCmdUI);
										// ステータスバー UI
	afx_msg void OnRefresh();
										// リフレッシュ
	afx_msg void OnStretch();
										// 拡大
	afx_msg void OnStretchUI(CCmdUI *pCmdUI);
										// 拡大 UI
	afx_msg void OnFullScreen();
										// フルスクリーン
	afx_msg void OnFullScreenUI(CCmdUI *pCmdUI);
										// フルスクリーンUI

	afx_msg void GetDesktopResolution(int& horizontal, int& vertical);

	afx_msg void OnExec();
										// 実行
	afx_msg void OnExecUI(CCmdUI *pCmdUI);
										// 実行 UI
	afx_msg void OnBreak();
										// 停止
	afx_msg void OnBreakUI(CCmdUI *pCmdUI);
										// 停止 UI
	afx_msg void OnTrace();
										// トレース
	afx_msg void OnTraceUI(CCmdUI *pCmdUI);
										// トレース UI

	afx_msg void OnMouseMode();
										// マウスモード
	afx_msg void OnSoftKey();
										// ソフトウェアキーボード
	afx_msg void OnSoftKeyUI(CCmdUI *pCmdUI);
										// ソフトウェアキーボード UI
	afx_msg void OnTimeAdj();
										// 時刻合わせ
	afx_msg void OnTrap();
										// trap#0
	afx_msg void OnTrapUI(CCmdUI *pCmdUI);
										// trap#0 UI
	afx_msg void OnSaveWav();
										// WAVキャプチャ
	afx_msg void OnSaveWavUI(CCmdUI *pCmdUI);
										// WAVキャプチャ UI
	afx_msg void OnNewFD();
										// 新しいフロッピーディスク
	afx_msg void OnNewDisk(UINT uID);
										// 新しい大容量ディスク
	afx_msg void OnOptions();
										// オプション

	afx_msg void OnCascade();
										// 重ねて表示
	afx_msg void OnCascadeUI(CCmdUI *pCmdUI);
										// 重ねて表示 UI
	afx_msg void OnTile();
										// 並べて表示
	afx_msg void OnTileUI(CCmdUI *pCmdUI);
										// 並べて表示 UI
	afx_msg void OnIconic();
										// 全てアイコン化
	afx_msg void OnIconicUI(CCmdUI *pCmdUI);
										// 全てアイコン化 UI
	afx_msg void OnArrangeIcon();
										// アイコンの整列
	afx_msg void OnArrangeIconUI(CCmdUI *pCmdUI);
										// アイコンの整列 UI
	afx_msg void OnHide();
										// 全て隠す
	afx_msg void OnHideUI(CCmdUI *pCmdUI);
										// 全て隠す UI
	afx_msg void OnRestore();
										// 全て復元
	afx_msg void OnRestoreUI(CCmdUI *pCmdUI);
										// 全て復元 UI
	afx_msg void OnWindow(UINT uID);
										// ウィンドウ選択
	afx_msg void OnAbout();
										// バージョン情報

private:
	// 初期化
	BOOL FASTCALL InitChild();
										// チャイルドウィンドウ初期化
	void FASTCALL InitPos(BOOL bStart = TRUE);
										// 位置・矩形初期化
	void FASTCALL InitShell();
										// シェル連携初期化
	BOOL FASTCALL InitVM();
										// VM初期化
	BOOL FASTCALL InitComponent();
										// コンポーネント初期化
	void FASTCALL InitVer();
										// バージョン初期化
	void FASTCALL InitCmd(LPCTSTR lpszCmd);

	void FASTCALL ReadFile(LPCTSTR pszFileName, CString& str);
	CString FASTCALL CFrmWnd::ProcesarM3u(CString str);

										// コマンドライン処理
	void FASTCALL ApplyCfg();
										// 設定適用
	void FASTCALL SizeStatus();
										// ステータスバーサイズ変更
	void FASTCALL HideTaskBar(BOOL bHide, BOOL bFore);
										// タスクバー隠す
	BOOL RestoreFrameWnd(BOOL bFullScreen);
										// ウィンドウ復元
	void RestoreDiskState();
										// ディスク・ステート復元
	int m_nStatus;
										// ステータスコード
	static const DWORD SigTable[];
										// SRAMシグネチャテーブル

	// 終了
	void SaveFrameWnd();
										// ウィンドウ保存
	void SaveDiskState();
										// ディスク・ステート保存
	void FASTCALL CleanSub();
										// クリーンアップ
	BOOL m_bExit;
										// 終了フラグ
	BOOL m_bSaved;
										// フレーム・ディスク・ステート保存フラグ

	// セーブ・ロード
	BOOL FASTCALL SaveComponent(const Filepath& path, DWORD dwPos);
										// セーブ
	BOOL FASTCALL LoadComponent(const Filepath& path, DWORD dwPos);
										// ロード

	// コマンドハンドラサブ
	BOOL FASTCALL OnOpenSub(const Filepath& path);
										// オープンサブ
	BOOL FASTCALL OnOpenPrep(const Filepath& path, BOOL bWarning = TRUE);
										// オープンチェック
	void FASTCALL OnSaveSub(const Filepath& path);
										// 保存サブ
	void FASTCALL OnFDOpen(int nDrive);
										// フロッピーオープン
	void FASTCALL OnFDEject(int nDrive);
										// フロッピーイジェクト
	void FASTCALL OnFDWriteP(int nDrive);
										// フロッピー書き込み保護
	void FASTCALL OnFDForce(int nDrive);
										// フロッピー強制イジェクト
	void FASTCALL OnFDInvalid(int nDrive);
										// フロッピー誤挿入
	void FASTCALL OnFDMedia(int nDrive, int nMedia);
										// フロッピーメディア
	void FASTCALL OnFDMRU(int nDrive, int nMRU);
										// フロッピーMRU
	int m_nFDDStatus[2];
										// フロッピーステータス

	// デバイス・ビュー・コンポーネント
	FDD *m_pFDD;
										// FDD
	SASI *m_pSASI;
										// SASI
	SCSI *m_pSCSI;
										// SCSI
	Scheduler *m_pScheduler;
										// Scheduler
	Keyboard *m_pKeyboard;
										// Keyboard
	Mouse *m_pMouse;
										// Mouse
	CDrawView *m_pDrawView;
										// 描画ビュー
	CComponent *m_pFirstComponent;
										// 最初のコンポーネント
	CScheduler *m_pSch;
										// スケジューラ
	CSound *m_pSound;
										// サウンド
	CInput *m_pInput;
										// インプット
	CPort *m_pPort;
										// ポート
	CMIDI *m_pMIDI;
										// MIDI
	CTKey *m_pTKey;
										// TrueKey
	CHost *m_pHost;
										// Host
	CInfo *m_pInfo;
										// Info
	CConfig *m_pConfig;
										// コンフィグ

	// フルスクリーン
	
										// フルスクリーンフラグ
	DEVMODE m_DevMode;
										// スクリーンパラメータ記憶
	HWND m_hTaskBar;
										// タスクバー
	int m_nWndLeft;
										// ウィンドウモード時x
	int m_nWndTop;
										// ウィンドウモード時y

	// サブウィンドウ
	CString m_strWndClsName;
										// ウィンドウクラス名

	// ステータスビュー
	void FASTCALL CreateStatusView();
										// ステータスビュー作成
	void FASTCALL DestroyStatusView();
										// ステータスビュー削除
	CStatusView *m_pStatusView;
										// ステータスビュー

	// ステータスバー
	void FASTCALL ShowStatus();
										// ステータスバー表示
	void FASTCALL ResetStatus();
										// ステータスバーリセット
	CStatusBar m_StatusBar;
										// ステータスバー
	BOOL m_bStatusBar;
										// ステータスバー表示フラグ

	// メニュー
	void FASTCALL ShowMenu();
										// メニューバー表示
	CMenu m_Menu;
										// メインメニュー
	BOOL m_bMenuBar;
										// メニューバー表示フラグ
	CMenu m_PopupMenu;
										// ポップアップメニュー
	BOOL m_bPopupMenu;
										// ポップアップメニュー実行中

	// キャプション
	void FASTCALL ShowCaption();
										// キャプション表示
	void FASTCALL ResetCaption();
										// キャプションリセット
	BOOL m_bCaption;
										// キャプション表示フラグ

	// 情報
	void FASTCALL SetInfo(CString& strInfo);
										// 情報文字列セット	
	ULONG m_uNotifyId;
	
	
	SHChangeNotifyEntry m_fsne[1];							

	// ステートファイル
	void FASTCALL UpdateExec();
										// 更新(実行)
	DWORD m_dwExec;
										// セーブ後実行カウンタ

	// コンフィギュレーション
	BOOL m_bMouseMid;
										// マウス中ボタン有効
	BOOL m_bPopup;
										// ポップアップモード
	BOOL m_bAutoMouse;
										// 自動マウスモード

	DECLARE_MESSAGE_MAP()
										// メッセージ マップあり
};

#endif	// mfc_frm_h
#endif	// _WIN32
