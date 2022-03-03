//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 ＰＩ．(ytanaka@ipc-tokai.or.jp)
//	[ MFC スケジューラ ]
//
//---------------------------------------------------------------------------

#if defined(_WIN32)

#if !defined(mfc_sch_h)
#define mfc_sch_h

//===========================================================================
//
//	スケジューラ
//
//===========================================================================
class CScheduler : public CComponent
{
public:
	// 基本ファンクション
	CScheduler(CFrmWnd *pFrmWnd);
										// コンストラクタ
	BOOL FASTCALL Init();
										// 初期化
	void FASTCALL Cleanup();
										// クリーンアップ
	void FASTCALL ApplyCfg(const Config *pConfig);
										// 設定適用
#if defined(_DEBUG)
	void AssertValid() const;
										// 診断
#endif	// _DEBUG

	// 実行制御
	void FASTCALL Reset();
										// 時間をリセット
	void FASTCALL Run();
										// 実行
	void FASTCALL Stop();
										// スケジューラ停止

	// セーブ・ロード
	BOOL FASTCALL Save(Fileio *pFio, int nVer);
										// セーブ
	BOOL FASTCALL Load(Fileio *pFio, int nVer);
										// ロード
	BOOL FASTCALL HasSavedEnable() const { return m_bSavedValid; }
										// セーブ時にEnable状態を保存しているか
	BOOL FASTCALL GetSavedEnable() const { return m_bSavedEnable; }
										// セーブ時にEnable状態だったか
	void FASTCALL SetSavedEnable(BOOL bEnable) { m_bSavedEnable = bEnable; }
										// セーブ時の状態を設定

	// その他
	void FASTCALL Menu(BOOL bMenu)		{ m_bMenu = bMenu; }
										// メニュー通知
	void FASTCALL Activate(BOOL bAct)	{ m_bActivate = bAct; }
										// アクティブ通知
	void FASTCALL SyncDisasm();
										// 逆アセンブル同期
	int FASTCALL GetFrameRate();
										// フレームレート取得

private:
	static UINT ThreadFunc(LPVOID pParam);
										// スレッド関数
	DWORD FASTCALL GetTime()			{ return timeGetTime(); }
										// 時間取得
	void FASTCALL Lock()				{ ::LockVM(); }
										// VMロック
	void FASTCALL Unlock()				{ ::UnlockVM(); }
										// VMアンロック
	void FASTCALL Refresh();
										// リフレッシュ
	CPU *m_pCPU;
										// CPU
	Render *m_pRender;
										// レンダラ
	CWinThread *m_pThread;
										// スレッドポインタ
	CSound *m_pSound;
										// サウンド
	CInput *m_pInput;
										// インプット
	BOOL m_bExitReq;
										// スレッド終了要求
	DWORD m_dwExecTime;
										// タイマーカウント(実行)
	int m_nSubWndNum;
										// サブウィンドウの個数
	int m_nSubWndDisp;
										// サブウィンドウの表示(-1:メイン画面)
	BOOL m_bMPUFull;
										// MPU高速フラグ
	BOOL m_bVMFull;
										// VM高速フラグ
	DWORD m_dwDrawCount;
										// メインウィンドウ表示回数
	DWORD m_dwDrawPrev;
										// メインウィンドウ表示回数(前)
	DWORD m_dwDrawTime;
										// メインウィンドウ表示時間
	DWORD m_dwDrawBackup;
										// メインウィンドウ表示回数(前)
	BOOL m_bMenu;
										// メニューフラグ
	BOOL m_bActivate;
										// アクティブフラグ
	BOOL m_bBackup;
										// Enableフラグバックアップ
	BOOL m_bSavedValid;
										// セーブ時にEnable状態を保存しているか
	BOOL m_bSavedEnable;
										// セーブ時にEnableだったか
};

#endif	// mfc_sch_h
#endif	// _WIN32
