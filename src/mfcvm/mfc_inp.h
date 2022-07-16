//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 ＰＩ．(ytanaka@ipc-tokai.or.jp)
//	[ MFC インプット ]
//
//---------------------------------------------------------------------------

#if defined(_WIN32)

#if !defined(mfc_inp_h)
#define mfc_inp_h

//===========================================================================
//
//	インプット
//
//===========================================================================
class CInput : public CComponent
{
public:
	// ジョイスティック定数
	enum {
		JoyDeviceMax = 16,				// サポートデバイス最大数
		JoyDevices = 2,					// 使用デバイス最大数
		JoyAxes = 9,					// 軸最大数
		JoyButtons = 12,				// ボタン最大数
		JoyRapids = 10,					// 連射レベル数
	};
	// ジョイスティック設定
	typedef struct _JOYCFG {
		int nDevice;					// デバイス番号(1～)、未使用=0
		DWORD dwAxis[JoyAxes];			// 軸変換 (1～)、未使用=0
		BOOL bAxis[JoyAxes];			// 軸反転
		DWORD dwButton[JoyButtons];		// ボタン変換 (1～)、未使用=0
		DWORD dwRapid[JoyButtons];		// 連射間隔 連射なし=0
		DWORD dwCount[JoyButtons];		// 連射カウンタ
	} JOYCFG, *LPJOYCFG;

public:
	// 基本ファンクション
	CInput(CFrmWnd *pWnd);
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

	// セーブ・ロード
	BOOL FASTCALL Save(Fileio *pFio, int nVer);
										// セーブ
	BOOL FASTCALL Load(Fileio *pFio, int nVer);
										// ロード

	// 外部API
	void FASTCALL Process(BOOL bRun);
										// 進行
	void FASTCALL Activate(BOOL bActivate);
										// アクティブ通知
	BOOL FASTCALL IsActive() const		{ return m_bActive; }
										// アクティブ状況取得
	void FASTCALL Menu(BOOL bMenu);
										// メニュー通知
	BOOL FASTCALL IsMenu() const		{ return m_bMenu; }
										// メニュー状況取得
	DWORD FASTCALL GetProcessCount() const	{ return m_dwProcessCount; }
										// 進行カウンタ取得
	DWORD FASTCALL GetAcquireCount(int nType) const;
										// 獲得カウンタ取得

	// キーボード
	void FASTCALL GetKeyBuf(BOOL *pKeyBuf) const;
										// キー入力情報取得
	void FASTCALL EnableKey(BOOL bEnable);
										// キー有効化・無効化
	void FASTCALL SetDefaultKeyMap(DWORD *pKeyMap);
										// デフォルトマップ設定
	int FASTCALL Key2DirectX(int nKey);
										// キー変換
	int FASTCALL Key2X68k(int nDXKey);
										// キー変換
	static LPCTSTR FASTCALL GetKeyName(int nKey);
										// キー名称取得
	static LPCTSTR FASTCALL GetKeyID(int nID);
										// キーID取得
	void FASTCALL GetKeyMap(DWORD *pKeyMap);
										// キーマップ取得
	void FASTCALL SetKeyMap(const DWORD *pKeyMap);
										// キーマップ設定
	BOOL FASTCALL IsKeyMapped(int nID) const;
										// キーマップ有無チェック

	// マウス
	void FASTCALL SetMouseMode(BOOL bMode);
										// マウスモード設定
	BOOL FASTCALL GetMouseMode() const	{ return m_bMouseMode; }
										// マウスモード取得
	void FASTCALL GetMouseInfo(int *pPos, BOOL *pBtn) const;
										// マウス情報取得

	// ジョイスティック
	static BOOL CALLBACK EnumCb(LPDIDEVICEINSTANCE pDevInst, LPVOID pvRef);
										// ジョイスティックコールバック
	void FASTCALL EnableJoy(BOOL bEnable);
										// ジョイスティック有効化・無効化
	int FASTCALL GetJoyDevice(int nJoy) const;
										// ジョイスティックデバイス取得
	LONG FASTCALL GetJoyAxis(int nJoy, int nAxis) const;
										// ジョイスティック軸取得
	DWORD FASTCALL GetJoyButton(int nJoy, int nButton) const;
										// ジョイスティックボタン取得
	BOOL FASTCALL GetJoyCaps(int nDevice, CString& strDesc, DIDEVCAPS *pCaps) const;
										// ジョイスティックCaps取得
	void FASTCALL GetJoyCfg(int nJoy, LPJOYCFG lpJoyCfg) const;
										// ジョイスティック設定取得
	void FASTCALL SetJoyCfg(int nJoy, const LPJOYCFG lpJoyCfg);
										// ジョイスティック設定セット

private:
	// 共通
	LPDIRECTINPUT m_lpDI;
										// DirectInput
	BOOL m_bActive;
										// アクティブフラグ
	BOOL m_bMenu;
										// メニューフラグ
	CRTC *m_pCRTC;
										// CRTC
	DWORD m_dwDispCount;
										// CRTC表示カウント
	DWORD m_dwProcessCount;
										// 進行カウント

	// セーブ・ロード
	BOOL FASTCALL SaveMain(Fileio *pFio);
										// セーブ本体
	BOOL FASTCALL Load200(Fileio *pFio);
										// ロード本体 (version2.00)
	BOOL FASTCALL Load201(Fileio *pFio);
										// ロード本体 (version2.01)

	// キーボード
	BOOL FASTCALL InitKey();
										// キーボード初期化
	void FASTCALL InputKey(BOOL bEnable);
										// キーボード入力
	Keyboard *m_pKeyboard;
										// キーボード
	LPDIRECTINPUTDEVICE m_lpDIKey;
										// キーボードデバイス
	DWORD m_dwKeyAcquire;
										// キーボード獲得カウンタ
	BOOL m_bKeyEnable;
										// キーボード有効フラグ
	BOOL m_KeyBuf[0x100];
										// キーボードバッファ
	DWORD m_KeyMap[0x100];
										// キー変換マップ
	static const DWORD m_KeyMap106[];
										// デフォルトマップ(106)
	static LPCTSTR KeyNameTable[];
										// キー名称テーブル
	static LPCSTR KeyIDTable[];
										// DirectXキーIDテーブル

	// マウス
	BOOL FASTCALL InitMouse();
										// マウス初期化
	void FASTCALL InputMouse(BOOL bEnable);
										// マウス入力
	Mouse *m_pMouse;
										// マウス
	LPDIRECTINPUTDEVICE m_lpDIMouse;
										// マウスデバイス
	DWORD m_dwMouseAcquire;
										// マウス獲得カウンタ
	BOOL m_bMouseMode;
										// マウスモードフラグ
	int m_nMouseX;
										// マウスx座標
	int m_nMouseY;
										// マウスy座標
	BOOL m_bMouseB[2];
										// マウス左右ボタン
	DWORD m_dwMouseMid;
										// マウス中央ボタンカウント
	BOOL m_bMouseMid;
										// マウス中央ボタン使用フラグ

	// ジョイスティック
	void FASTCALL EnumJoy();
										// ジョイスティック列挙
	BOOL FASTCALL EnumDev(LPDIDEVICEINSTANCE pDevInst);
										// ジョイスティック追加
	void FASTCALL InitJoy();
										// ジョイスティック初期化
	void FASTCALL InputJoy(BOOL bEnable);
										// ジョイスティック入力
	void FASTCALL MakeJoy(BOOL bEnable);
										// ジョイスティック合成
	PPI *m_pPPI;
										// PPI
	BOOL m_bJoyEnable;
										// ジョイスティック有効・無効
	LPDIRECTINPUTDEVICE m_lpDIJoy[JoyDevices];
										// ジョイスティックデバイス
	LPDIRECTINPUTDEVICE2 m_lpDIDev2[JoyDevices];
										// フォースフィードバックデバイス
	JOYCFG m_JoyCfg[JoyDevices];
										// ジョイスティックコンフィグ
	LONG m_lJoyAxisMin[JoyDevices][JoyAxes];
										// ジョイスティック軸最小値
	LONG m_lJoyAxisMax[JoyDevices][JoyAxes];
										// ジョイスティック軸最大値
	DWORD m_dwJoyAcquire[JoyDevices];
										// ジョイスティック獲得カウンタ
	DIJOYSTATE m_JoyState[JoyDevices];
										// ジョイスティック状態
	DWORD m_dwJoyDevs;
										// ジョイスティックデバイス数
	DIDEVCAPS m_JoyDevCaps[JoyDeviceMax];
										// ジョイスティックCaps
	DIDEVICEINSTANCE m_JoyDevInst[JoyDeviceMax];
										// ジョイスティックインスタンス
	static const DWORD JoyAxisOffsetTable[JoyAxes];
										// ジョイスティック軸オフセットテーブル
	static const DWORD JoyRapidTable[JoyRapids + 1];
										// ジョイスティック連射テーブル
};

#endif	// mfc_inp_h
#endif	// _WIN32
