//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 ＰＩ．(ytanaka@ipc-tokai.or.jp)
//	[ MFC インプット ]
//
//---------------------------------------------------------------------------

#if defined(_WIN32)

#include "os.h"
#include "xm6.h"
#include "vm.h"
#include "crtc.h"
#include "keyboard.h"
#include "mouse.h"
#include "ppi.h"
#include "fileio.h"
#include "mfc_frm.h"
#include "mfc_com.h"
#include "mfc_sch.h"
#include "mfc_res.h"
#include "mfc_cfg.h"
#include "mfc_inp.h"

//===========================================================================
//
//	インプット
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	constructor
//
//---------------------------------------------------------------------------
CInput::CInput(CFrmWnd *pWnd) : CComponent(pWnd)
{
	int i;
	int nAxis;
	int nButton;

	// parametro del componente
	m_dwID = MAKEID('I', 'N', 'P', ' ');
	m_strDesc = _T("Input Manager");

	// Inicializacion del trabajo comun
	m_lpDI = NULL;
	m_bActive = TRUE;
	m_bMenu = FALSE;
	m_pCRTC = NULL;
	m_dwDispCount = 0;
	m_dwProcessCount = 0;

	// inicializacion del trabajo del teclado
	m_pKeyboard = NULL;
	m_lpDIKey = NULL;
	m_dwKeyAcquire = 0;
	m_bKeyEnable = TRUE;

	// borrado del bufer de llaves
	for (i=0; i<0x100; i++) {
		m_KeyBuf[i] = FALSE;
		m_KeyMap[i] = 0;
	}	

	// Inicializacion de Mousework
	m_pMouse = NULL;
	m_lpDIMouse = NULL;
	m_dwMouseAcquire = 0;
	m_bMouseMode = FALSE;
	m_nMouseX = 0;
	m_nMouseY = 0;
	m_bMouseB[0] = FALSE;
	m_bMouseB[1] = FALSE;
	m_dwMouseMid = 0;
	m_bMouseMid = TRUE;

	// Inicializacion del trabajo del joystick
	m_pPPI = NULL;
	m_dwJoyDevs = 0;
	m_bJoyEnable = TRUE;
	for (i=0; i<JoyDevices; i++) {
		// dispositivo
		m_lpDIJoy[i] = NULL;
		m_lpDIDev2[i] = NULL;

		// configurar.
		memset(&m_JoyCfg[i], 0, sizeof(JOYCFG));
		// nConfiguracion del dispositivo :
		// Dispositivo asignado +1 (0 sin asignar)
		m_JoyCfg[i].nDevice = i + 1;
		for (nAxis=0; nAxis<JoyAxes; nAxis++) {
			if (nAxis < 4) {
				// Configuracion de dwAxis :
				// Palabra superior Puerto asignado (0x00000 o 0x10000)
				// Palabra inferior Eje asignado +1 (1-4, 0 sin asignar)
				m_JoyCfg[i].dwAxis[nAxis] = (DWORD)((i << 16) | (nAxis + 1));
			}
			m_JoyCfg[i].bAxis[nAxis] = FALSE;
		}
		for (nButton=0; nButton<JoyButtons; nButton++) {
			if (nButton < 8) {
				// Configuracion de dwButton :
				// Palabra superior Puerto asignado (0x00000 o 0x10000)
				// Palabra inferior Boton asignado +1 (1-8, 0 no asignado)
				m_JoyCfg[i].dwButton[nButton] = (DWORD)((i << 16) | (nButton + 1));
			}
			// No hay disparo continuo, contador inicializado
			m_JoyCfg[i].dwRapid[nButton] = 0;
			m_JoyCfg[i].dwCount[nButton] = 0;
		}

		// Inicializar Gama de ejes
		memset(m_lJoyAxisMin[i], 0, sizeof(m_lJoyAxisMin[i]));
		memset(m_lJoyAxisMax[i], 0, sizeof(m_lJoyAxisMax[i]));

		// contador de adquisiciones
		m_dwJoyAcquire[i] = 0;

		// datos de entrada
		memset(&m_JoyState[i], 0, sizeof(DIJOYSTATE));
	}

	// Inicializar el mapa de teclas (una medida disenada para trabajar con el Config Manager).
	SetDefaultKeyMap(m_KeyMap);
}

//---------------------------------------------------------------------------
//
//	初期化
//
//---------------------------------------------------------------------------
BOOL FASTCALL CInput::Init()
{
	ASSERT(this);

	// 基本クラス
	if (!CComponent::Init()) {
		return FALSE;
	}

	// CRTC取得
	ASSERT(!m_pCRTC);
	m_pCRTC = (CRTC*)::GetVM()->SearchDevice(MAKEID('C', 'R', 'T', 'C'));
	ASSERT(m_pCRTC);

	// キーボード取得
	ASSERT(!m_pKeyboard);
	m_pKeyboard = (Keyboard*)::GetVM()->SearchDevice(MAKEID('K', 'E', 'Y', 'B'));
	ASSERT(m_pKeyboard);

	// マウス取得
	ASSERT(!m_pMouse);
	m_pMouse = (Mouse*)::GetVM()->SearchDevice(MAKEID('M', 'O', 'U', 'S'));
	ASSERT(m_pMouse);

	// PPI取得
	ASSERT(!m_pPPI);
	m_pPPI = (PPI*)::GetVM()->SearchDevice(MAKEID('P', 'P', 'I', ' '));
	ASSERT(m_pPPI);

	// DirectInputオブジェクトを作成
	if (FAILED(DirectInputCreate(AfxGetApp()->m_hInstance, DIRECTINPUT_VERSION, &m_lpDI, NULL))) {
		return FALSE;
	}

	// キーボード
	if (!InitKey()) {
		return FALSE;
	}

	// マウス
	if (!InitMouse()) {
		return FALSE;
	}

	// ジョイスティック
	EnumJoy();
	InitJoy();

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	クリーンアップ
//
//---------------------------------------------------------------------------
void FASTCALL CInput::Cleanup()
{
	int i;

	ASSERT(this);
	ASSERT_VALID(this);

	// マウスモード
	SetMouseMode(FALSE);

	// ジョイスティックデバイスを解放
	for (i=0; i<JoyDevices; i++) {
		if (m_lpDIDev2[i]) {
			m_lpDIDev2[i]->Release();
			m_lpDIDev2[i] = NULL;
		}

		if (m_lpDIJoy[i]) {
			m_lpDIJoy[i]->Unacquire();
			m_lpDIJoy[i]->Release();
			m_lpDIJoy[i] = NULL;
		}
	}

	// マウスデバイスを解放
	if (m_lpDIMouse) {
		m_lpDIMouse->Unacquire();
		m_lpDIMouse->Release();
		m_lpDIMouse = NULL;
	}

	// キーボードデバイスを解放
	if (m_lpDIKey) {
		m_lpDIKey->Unacquire();
		m_lpDIKey->Release();
		m_lpDIKey = NULL;
	}

	// DirectInputオブジェクトを解放
	if (m_lpDI) {
		m_lpDI->Release();
		m_lpDI = NULL;
	}

	// 基本クラス
	CComponent::Cleanup();
}

//---------------------------------------------------------------------------
//
//	設定適用
//
//---------------------------------------------------------------------------
void FASTCALL CInput::ApplyCfg(const Config* pConfig)
{
	BOOL bFlag;
	int i;
	int nButton;
	int nConfig;

	ASSERT(this);
	ASSERT(pConfig);
	ASSERT_VALID(this);

	// マウス中ボタン
	m_bMouseMid = pConfig->mouse_mid;

	// 中央ボタンカウントを無効化
	m_dwMouseMid = 5;

	// ジョイスティックデバイス(デバイス変更系)
	bFlag = FALSE;
	for (i=0; i<JoyDevices; i++) {
		// 使うデバイスNo.が変わっていたら、再初期化が必要
		if (pConfig->joy_dev[i] != m_JoyCfg[i].nDevice) {
			m_JoyCfg[i].nDevice = pConfig->joy_dev[i];
			bFlag = TRUE;
		}
	}
	if (bFlag) {
		// 再初期化
		InitJoy();
	}

	// ジョイスティックデバイス(ボタン系)
	for (i=0; i<JoyDevices; i++) {
		for (nButton=0; nButton<JoyButtons; nButton++) {
			// コンフィグデータ取得(bit16:ポート bit15-8:連射 bit7-0:ボタン)
			if (i == 0) {
				nConfig = pConfig->joy_button0[nButton];
			}
			else {
				nConfig = pConfig->joy_button1[nButton];
			}

			// 初期化
			m_JoyCfg[i].dwButton[nButton] = 0;
			m_JoyCfg[i].dwRapid[nButton] = 0;
			m_JoyCfg[i].dwCount[nButton] = 0;

			// 未割り当てかチェック
			if ((nConfig & 0xff) == 0) {
				continue;
			}

			// ボタン数が制限を超えていないかチェック
			if ((nConfig & 0xff) > PPI::ButtonMax) {
				continue;
			}

			// ボタン割り当て設定
			m_JoyCfg[i].dwButton[nButton] = (DWORD)(nConfig & 0xff00ff);

			// 連射設定
			m_JoyCfg[i].dwRapid[nButton] = (DWORD)((nConfig >> 8) & 0xff);
			if (m_JoyCfg[i].dwRapid[nButton] > JoyRapids) {
				// 範囲オーバの場合、連射なしとする
				m_JoyCfg[i].dwRapid[nButton] = 0;
			}
		}
	}
}

#if defined(_DEBUG)
//---------------------------------------------------------------------------
//
//	診断
//
//---------------------------------------------------------------------------
void CInput::AssertValid() const
{
	ASSERT(this);
	ASSERT(GetID() == MAKEID('I', 'N', 'P', ' '));
	ASSERT(m_pCRTC);
	ASSERT(m_pCRTC->GetID() == MAKEID('C', 'R', 'T', 'C'));
	ASSERT(m_pKeyboard);
	ASSERT(m_pKeyboard->GetID() == MAKEID('K', 'E', 'Y', 'B'));
	ASSERT(m_pMouse);
	ASSERT(m_pMouse->GetID() == MAKEID('M', 'O', 'U', 'S'));
	ASSERT(m_pPPI);
	ASSERT(m_pPPI->GetID() == MAKEID('P', 'P', 'I', ' '));
}
#endif	// _DEBUG

//---------------------------------------------------------------------------
//
//	セーブ
//
//---------------------------------------------------------------------------
BOOL FASTCALL CInput::Save(Fileio *pFio, int /*nVer*/)
{
	BOOL bResult;

	ASSERT(this);
	ASSERT(pFio);
	ASSERT_VALID(this);

	// VMロックし、一時的に進行を止める
	::LockVM();

	// セーブ本体
	bResult = SaveMain(pFio);

	// VMアンロック
	::UnlockVM();

	// 結果を持ち帰る
	return bResult;
}

//---------------------------------------------------------------------------
//
//	セーブ本体
//
//---------------------------------------------------------------------------
BOOL CInput::SaveMain(Fileio *pFio)
{
	ASSERT(this);
	ASSERT(pFio);
	ASSERT_VALID(this);

	//
	//	version2.00
	//

	// 全般
	if (!pFio->Write(&m_dwProcessCount, sizeof(m_dwProcessCount))) {
		return FALSE;
	}
	if (!pFio->Write(&m_dwDispCount, sizeof(m_dwDispCount))) {
		return FALSE;
	}

	// キーボード
	if (!pFio->Write(m_KeyBuf, sizeof(m_KeyBuf))) {
		return FALSE;
	}

	// マウス
	if (!pFio->Write(&m_nMouseX, sizeof(m_nMouseX))) {
		return FALSE;
	}
	if (!pFio->Write(&m_nMouseY, sizeof(m_nMouseY))) {
		return FALSE;
	}
	if (!pFio->Write(&m_dwMouseMid, sizeof(m_dwMouseMid))) {
		return FALSE;
	}

	// ジョイスティック
	if (!pFio->Write(m_JoyState, sizeof(m_JoyState))) {
		return FALSE;
	}

	//
	//	version2.01
	//

	// キーボード
	if (!pFio->Write(m_KeyMap, sizeof(m_KeyMap))) {
		return FALSE;
	}

	// マウス
	if (!pFio->Write(m_bMouseB, sizeof(m_bMouseB))) {
		return FALSE;
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	ロード
//
//---------------------------------------------------------------------------
BOOL FASTCALL CInput::Load(Fileio *pFio, int nVer)
{
	ASSERT(this);
	ASSERT(pFio);
	ASSERT(nVer >= 0x0200);
	ASSERT_VALID(this);

	// VMロックし、一時的に進行を止める
	::LockVM();

	// ロード(version2.00)
	if (!Load200(pFio)) {
		::UnlockVM();
		return FALSE;
	}

	if (nVer >= 0x0201) {
		// version2.01
		if (!Load201(pFio)) {
			::UnlockVM();
			return FALSE;
		}
	}

	// VMアンロック
	::UnlockVM();

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	ロード本体 (version2.00)
//
//---------------------------------------------------------------------------
BOOL FASTCALL CInput::Load200(Fileio *pFio)
{
	ASSERT(this);
	ASSERT(pFio);
	ASSERT_VALID(this);

	// 全般
	if (!pFio->Read(&m_dwProcessCount, sizeof(m_dwProcessCount))) {
		return FALSE;
	}
	if (!pFio->Read(&m_dwDispCount, sizeof(m_dwDispCount))) {
		return FALSE;
	}

	// キーボード
	if (!pFio->Read(m_KeyBuf, sizeof(m_KeyBuf))) {
		return FALSE;
	}

	// マウス
	if (!pFio->Read(&m_nMouseX, sizeof(m_nMouseX))) {
		return FALSE;
	}
	if (!pFio->Read(&m_nMouseY, sizeof(m_nMouseY))) {
		return FALSE;
	}
	if (!pFio->Read(&m_dwMouseMid, sizeof(m_dwMouseMid))) {
		return FALSE;
	}

	// ジョイスティック
	if (!pFio->Read(m_JoyState, sizeof(m_JoyState))) {
		return FALSE;
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	ロード本体 (version2.01)
//
//---------------------------------------------------------------------------
BOOL FASTCALL CInput::Load201(Fileio *pFio)
{
	ASSERT(this);
	ASSERT(pFio);
	ASSERT_VALID(this);

	// キーボード
	if (!pFio->Read(m_KeyMap, sizeof(m_KeyMap))) {
		return FALSE;
	}

	// マウス
	if (!pFio->Read(m_bMouseB, sizeof(m_bMouseB))) {
		return FALSE;
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	進行
//
//---------------------------------------------------------------------------
void FASTCALL CInput::Process(BOOL bRun)
{
	DWORD dwDispCount;

	ASSERT(this);
	ASSERT_VALID(this);

	// ディセーブルなら何もしない
	if (!m_bEnable) {
		return;
	}

	// bRun = FALSEなら、スケジューラ停止中(10msおきに呼ばれる)
	if (!bRun) {
		// 進行カウンタUp
		m_dwProcessCount++;

		// ダミー入力
		InputKey(FALSE);
		InputMouse(FALSE);
		InputJoy(FALSE);
		return;
	}

	// CRTCの表示カウンタを見て、フレームごとに処理する
	ASSERT(m_pCRTC);
	dwDispCount = m_pCRTC->GetDispCount();
	if (dwDispCount == m_dwDispCount) {
		return;
	}
	m_dwDispCount = dwDispCount;

	// 進行カウンタUp
	m_dwProcessCount++;

	// アクティブでないか、またはメニュー中ならダミー入力
	if (!m_bActive || m_bMenu) {
		// ダミー入力
		InputKey(FALSE);
		InputMouse(FALSE);
		InputJoy(FALSE);
		MakeJoy(FALSE);
		return;
	}

	// ok、入力処理できる
	InputKey(TRUE);
	InputMouse(TRUE);
	InputJoy(TRUE);
	MakeJoy(m_bJoyEnable);
}

//---------------------------------------------------------------------------
//
//	アクティベート通知
//
//---------------------------------------------------------------------------
void FASTCALL CInput::Activate(BOOL bActive)
{
	ASSERT(this);
	ASSERT_VALID(this);

	// アクティブフラグに反映
	m_bActive = bActive;
}

//---------------------------------------------------------------------------
//
//	メニュー通知
//
//---------------------------------------------------------------------------
void FASTCALL CInput::Menu(BOOL bMenu)
{
	ASSERT(this);
	ASSERT_VALID(this);

	// メニューフラグに反映
	m_bMenu = bMenu;
}

//---------------------------------------------------------------------------
//
//	獲得カウンタ取得
//
//---------------------------------------------------------------------------
DWORD FASTCALL CInput::GetAcquireCount(int nType) const
{
	ASSERT(this);
	ASSERT(JoyDevices >= 2);
	ASSERT_VALID(this);

	switch (nType) {
		// 0:キーボード
		case 0:
			return m_dwKeyAcquire;

		// 1:マウス
		case 1:
			return m_dwMouseAcquire;

		// 2:ジョイスティックA
		case 2:
			return m_dwJoyAcquire[0];

		// 3:ジョイスティックB
		case 3:
			return m_dwJoyAcquire[1];

		// その他
		default:
			break;
	}

	// 通常、ここにはこない
	ASSERT(FALSE);
	return 0;
}

//===========================================================================
//
//	キーボード
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	キーボード初期化
//
//---------------------------------------------------------------------------
BOOL FASTCALL CInput::InitKey()
{
	ASSERT(this);
	ASSERT(m_lpDI);
	ASSERT(!m_lpDIKey);
	ASSERT_VALID(this);

	// キーボードデバイスを作成
	if (FAILED(m_lpDI->CreateDevice(GUID_SysKeyboard, &m_lpDIKey, NULL))) {
		return FALSE;
	}

	// キーボードデータ形式を設定
	if (FAILED(m_lpDIKey->SetDataFormat(&c_dfDIKeyboard))) {
		return FALSE;
	}

	// 協調レベルを設定
	if (FAILED(m_lpDIKey->SetCooperativeLevel(m_pFrmWnd->m_hWnd,
						DISCL_BACKGROUND | DISCL_NONEXCLUSIVE))) {
		return FALSE;
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	キーボード入力
//
//---------------------------------------------------------------------------
void FASTCALL CInput::InputKey(BOOL bEnable)
{
	HRESULT hr;
	BYTE buf[0x100];
	DWORD dwCode;
	int nKey;

	ASSERT(this);
	ASSERT(m_lpDIKey);
	ASSERT_VALID(this);

	// 存在チェック
	if (!m_lpDIKey) {
		return;
	}

	// OKであればデバイス状態を取得、そうでなければすべてOFF
	if (bEnable || !m_bKeyEnable) {
		hr = m_lpDIKey->GetDeviceState(sizeof(buf), buf);
		if (hr != DI_OK) {
			// エラーならアクセス権を取得しておく
			m_lpDIKey->Acquire();
			m_dwKeyAcquire++;
			return;
		}
	}
	else {
		// 現在はキーボード入力無効
		memset(buf, 0, sizeof(buf));
	}

	// Win2000,WinXPはDIK_KANJI,DIK_KANAがロックするのでONしない
	if (::IsWinNT()) {
		buf[DIK_KANJI] = 0;
		buf[DIK_KANA] = 0;
	}

	// キー無効の場合はWin32側に乗っ取られている
	if (!m_bKeyEnable) {
		// 加工せずキーバッファに記憶
		for (nKey=0; nKey<0x100; nKey++) {
			if (buf[nKey] & 0x80) {
				m_KeyBuf[nKey] = TRUE;
			}
			else {
				m_KeyBuf[nKey] = FALSE;
			}
		}
		return;
	}

	// ALT+キーはマップされていなければ、無視する
	if (buf[DIK_LMENU] & 0x80) {
		if (m_KeyMap[DIK_LMENU] == 0) {
			// マップされていないので、無視
			return;
		}
	}
	if (buf[DIK_RMENU] & 0x80) {
		if (m_KeyMap[DIK_RMENU] == 0) {
			// マップされていないので、無視
			return;
		}
	}

	// シフトキー押下状態を補正
	if (buf[DIK_LSHIFT] | buf[DIK_RSHIFT] & 0x80) {
		buf[DIK_LSHIFT] |= 0x80;
		buf[DIK_RSHIFT] |= 0x80;
	}

	// ループ
	for (nKey=0; nKey<0x100; nKey++) {
		if (buf[nKey] & 0x80) {
			// キーが押されている、バッファはどうか
			if (m_KeyBuf[nKey] == FALSE) {
				// 状態遷移(押された)
				m_KeyBuf[nKey] = TRUE;
				// キー変換マップをチェックして、有効なら送出
				dwCode = m_KeyMap[nKey];
				if (dwCode != 0) {
					m_pKeyboard->MakeKey(dwCode);
				}
			}
		}
		else {
			// キーが離されている、バッファはどうか
			if (m_KeyBuf[nKey] == TRUE) {
				// 状態遷移(離された)
				m_KeyBuf[nKey] = FALSE;
				// キー変換マップをチェックして、有効なら送出
				dwCode = m_KeyMap[nKey];
				if (dwCode != 0) {
					m_pKeyboard->BreakKey(dwCode);
				}
			}
		}
	}
}

//---------------------------------------------------------------------------
//
//	デフォルトキーマップ設定
//
//---------------------------------------------------------------------------
void FASTCALL CInput::SetDefaultKeyMap(DWORD *pKeyMap)
{
	const DWORD *pTable;
	DWORD dwDX;
	DWORD dw68;

	// ASSERT_VALIDは入れない(コンストラクタから呼ばれる為)
	ASSERT(this);
	ASSERT(pKeyMap);

	// テーブル初期化(日本語106 Only)
	pTable = m_KeyMap106;

	// 一旦クリア
	memset(pKeyMap, 0, sizeof(DWORD) * 0x100);

	// ループ(X68k順から、DIKコード順へ)
	dw68 = 0x0001;
	for (;;) {
		// DIKコード取得
		dwDX = *pTable++;

		// 0xffで終了
		if (dwDX == 0xff) {
			break;
		}

		// 0でなければ、そのキーにX68kコードを割り当てる
		if (dwDX != 0) {
			pKeyMap[dwDX] = dw68;
			// DIKがLSHIFTなら、RSHIFTも同じ
			if (dwDX == DIK_LSHIFT) {
				pKeyMap[DIK_RSHIFT] = dw68;
			}
		}

		// 次へ
		dw68++;
	}
}

//---------------------------------------------------------------------------
//
//	デフォルトマップ(106キーボード向け)
//
//---------------------------------------------------------------------------
const DWORD CInput::m_KeyMap106[] = {
	DIK_ESCAPE,							// 01 [ESC]
	DIK_1,								// 02 [1]
	DIK_2,								// 03 [2]
	DIK_3,								// 04 [3]
	DIK_4,								// 05 [4]
	DIK_5,								// 06 [5]
	DIK_6,								// 07 [6]
	DIK_7,								// 08 [7]
	DIK_8,								// 09 [8]
	DIK_9,								// 0A [9]
	DIK_0,								// 0B [0]
	DIK_MINUS,							// 0C [-]
	DIK_CIRCUMFLEX,						// 0D [^]
	DIK_YEN,							// 0E [\]
	DIK_BACK,							// 0F [BS]

	DIK_TAB,							// 10 [TAB]
	DIK_Q,								// 11 [Q]
	DIK_W,								// 12 [W]
	DIK_E,								// 13 [E]
	DIK_R,								// 14 [R]
	DIK_T,								// 15 [T]
	DIK_Y,								// 16 [Y]
	DIK_U,								// 17 [U]
	DIK_I,								// 18 [I]
	DIK_O,								// 19 [O]
	DIK_P,								// 1A [P]
	DIK_AT,								// 1B [@]
	DIK_LBRACKET,						// 1C [[]
	DIK_RETURN,							// 1D [CR]

	DIK_A,								// 1E [A]
	DIK_S,								// 1F [S]
	DIK_D,								// 20 [D]
	DIK_F,								// 21 [F]
	DIK_G,								// 22 [G]
	DIK_H,								// 23 [H]
	DIK_J,								// 24 [J]
	DIK_K,								// 25 [K]
	DIK_L,								// 26 [L]
	DIK_SEMICOLON,						// 27 [;]
	DIK_COLON,							// 28 [:]
	DIK_RBRACKET,						// 29 []]

	DIK_Z,								// 2A [Z]
	DIK_X,								// 2B [X]
	DIK_C,								// 2C [C]
	DIK_V,								// 2D [V]
	DIK_B,								// 2E [B]
	DIK_N,								// 2F [N]
	DIK_M,								// 30 [M]
	DIK_COMMA,							// 31 [,]
	DIK_PERIOD,							// 32 [.]
	DIK_SLASH,							// 33 [/]
	DIK_BACKSLASH,						// 34 [_]

	DIK_SPACE,							// 35 [SPACE]

	DIK_HOME,							// 36 [HOME]
	DIK_DELETE,							// 37 [DEL]
	DIK_NEXT,							// 38 [ROLL UP]
	DIK_PRIOR,							// 39 [ROLL DOWN]
	DIK_END,							// 3A [UNDO]

	DIK_LEFT,							// 3B [LEFT]
	DIK_UP,								// 3C [UP]
	DIK_RIGHT,							// 3D [RIGHT]
	DIK_DOWN,							// 3E [DOWN]

	DIK_NUMLOCK,						// 3F [Tenkey CLR]
	DIK_DIVIDE,							// 40 [Tenkey /]
	DIK_MULTIPLY,						// 41 [Tenkey *]
	DIK_SUBTRACT,						// 42 [Tenkey -]
	DIK_NUMPAD7,						// 43 [Tenkey 7]
	DIK_NUMPAD8,						// 44 [Tenkey 8]
	DIK_NUMPAD9,						// 45 [Tenkey 9]
	DIK_ADD,							// 46 [Tenkey +]
	DIK_NUMPAD4,						// 47 [Tenkey 4]
	DIK_NUMPAD5,						// 48 [Tenkey 5]
	DIK_NUMPAD6,						// 49 [Tenkey 6]
	0,									// 4A [Tenkey =]
	DIK_NUMPAD1,						// 4B [Tenkey 1]
	DIK_NUMPAD2,						// 4C [Tenkey 2]
	DIK_NUMPAD3,						// 4D [Tenkey 3]
	DIK_NUMPADENTER,					// 4E [Tenkey CR]
	DIK_NUMPAD0,						// 4F [Tenkey 0]
	0,									// 50 [Tenkey ,]
	DIK_DECIMAL,						// 51 [Tenkey .]

	0,									// 52 [記号入力]
	0,									// 53 [登録]
	0,									// 54 [HELP]

	0,									// 55 [XF1]
	0,									// 56 [XF2]
	0,									// 57 [XF3]
	0,									// 58 [XF4]
	0,									// 59 [XF5]

	0,									// 5A [かな]
	0,									// 5B [ローマ字]
	0,									// 5C [コード入力]
	DIK_CAPITAL,						// 5D [CAPS]

	DIK_INSERT,							// 5E [INS]
	0,									// 5F [ひらがな]
	0,									// 60 [全角]

	DIK_PAUSE,							// 61 [BREAK]
	0,									// 62 [COPY]
	DIK_F1,								// 63 [F1]
	DIK_F2,								// 64 [F2]
	DIK_F3,								// 65 [F3]
	DIK_F4,								// 66 [F4]
	DIK_F5,								// 67 [F5]
	DIK_F6,								// 68 [F6]
	DIK_F7,								// 69 [F7]
	DIK_F8,								// 6A [F8]
	DIK_F9,								// 6B [F9]
	DIK_F11,							// 6C [F10]
	0,									// 6D (Reserved)
	0,									// 6E (Reserved)
	0,									// 6F (Reserved)
	DIK_LSHIFT,							// 70 [SHIFT]
	DIK_LCONTROL,						// 71 [CTRL]
	0,									// 72 [OPT1]
	0,									// 73 [OPT2]
	0xff
};

//---------------------------------------------------------------------------
//
//	キー入力情報取得
//
//---------------------------------------------------------------------------
void FASTCALL CInput::GetKeyBuf(BOOL *pKeyBuf) const
{
	ASSERT(this);
	ASSERT(pKeyBuf);
	ASSERT_VALID(this);

	// 現在のバッファをコピー
	memcpy(pKeyBuf, m_KeyBuf, sizeof(m_KeyBuf));
}

//---------------------------------------------------------------------------
//
//	キー有効化・無効化
//
//---------------------------------------------------------------------------
void FASTCALL CInput::EnableKey(BOOL bEnable)
{
	DWORD dwCode;
	int nKey;

	ASSERT(this);
	ASSERT_VALID(this);

	// 同じなら必要なし
	if (m_bKeyEnable == bEnable) {
		return;
	}

	// 変更
	m_bKeyEnable = bEnable;

	// TRUEの場合、何もしない
	if (m_bKeyEnable) {
		return;
	}

	// FALSEの場合、現在押されているキーをすべてBreak
	for (nKey=0; nKey<0x100; nKey++) {
		// キーが離されている、バッファはどうか
		if (m_KeyBuf[nKey] == TRUE) {
			// キー変換マップをチェックして、有効なら送出
			dwCode = m_KeyMap[nKey];
			if (dwCode != 0) {
				m_pKeyboard->BreakKey(dwCode);
			}
		}
	}
}

//---------------------------------------------------------------------------
//
//	X68000キーコード→DirectXキーコード
//
//---------------------------------------------------------------------------
int FASTCALL CInput::Key2DirectX(int nKey)
{
	int nIndex;

	ASSERT(this);
	ASSERT((nKey >= 0) && (nKey <= 0x73));

	for (nIndex=0; nIndex<0x100; nIndex++) {
		// キーマップに登録されていればok
		if (m_KeyMap[nIndex] == (UINT)nKey) {
			return nIndex;
		}
	}

	// 該当なし
	return 0;
}

//---------------------------------------------------------------------------
//
//	DirectXキーコード→X68000キーコード
//
//---------------------------------------------------------------------------
int FASTCALL CInput::Key2X68k(int nDXKey)
{
	ASSERT(this);
	ASSERT((nDXKey >= 0) && (nDXKey < 0x100));
	ASSERT_VALID(this);

	// キーマップをそのまま返す。割り当てなしは0
	return m_KeyMap[nDXKey];
}

//---------------------------------------------------------------------------
//
//	キー名称取得
//
//---------------------------------------------------------------------------
LPCTSTR FASTCALL CInput::GetKeyName(int nKey)
{
	ASSERT((nKey >= 0) && (nKey <= 0x73));

	return KeyNameTable[nKey];
}

//---------------------------------------------------------------------------
//
//	キー名称テーブル
//
//---------------------------------------------------------------------------
LPCTSTR CInput::KeyNameTable[] = {
	NULL,
	_T("ESC"),
	_T("1!ﾇ"),
	_T("2\x22ﾌ"),
	_T("3#ｱｧ"),
	_T("4$ｳｩ"),
	_T("5%ｴｪ"),
	_T("6&ｵｫ"),
	_T("7'ﾔｬ"),
	_T("8(ﾕｭ"),
	_T("9)ﾖｮ"),
	_T("0 ﾜｦ"),
	_T("-=ﾎ"),
	_T("^~ﾍ"),
	_T("\\|ｰ"),
	_T("BS"),

	_T("TAB"),
	_T("Q ﾀ"),
	_T("W ﾃ"),
	_T("E ﾄ"),
	_T("R ｽ"),
	_T("T ｶ"),
	_T("Y ﾝ"),
	_T("U ﾅ"),
	_T("I ﾆ"),
	_T("O ﾗ"),
	_T("P ｾ"),
	_T("@`ﾞ"),
	_T("[{｡｢"),
	_T("RETURN"),

	_T("A ﾁ"),
	_T("S ﾄ"),
	_T("D ｼ"),
	_T("F ﾎ"),
	_T("G ｷ"),
	_T("H ｸ"),
	_T("J ﾏ"),
	_T("K ﾉ"),
	_T("L ﾘ"),
	_T(";+ﾚ"),
	_T(":*ｹ"),
	_T("]}ﾑ｣"),

	_T("Z ﾂｯ"),
	_T("X ｻ"),
	_T("C ｿ"),
	_T("V ﾋ"),
	_T("B ｺ"),
	_T("N ﾐ"),
	_T("M ﾓ"),
	_T(",<ﾈ､"),
	_T(".>ﾙ｡"),
	_T("/?ﾒ･"),
	_T(" _ﾛ"),
	_T("SPACE"),

	_T("HOME"),
	_T("DEL"),
	_T("ROLL UP"),
	_T("ROLL DOWN"),
	_T("UNDO"),
	_T("←"),
	_T("↑"),
	_T("→"),
	_T("↓"),

	_T("CLR"),
	_T("/"),
	_T("*"),
	_T("-"),
	_T("7"),
	_T("8"),
	_T("9"),
	_T("+"),
	_T("4"),
	_T("5"),
	_T("6"),
	_T("="),
	_T("1"),
	_T("2"),
	_T("3"),
	_T("ENTER"),
	_T("0"),
	_T(","),
	_T("."),

	_T("記号入力"),
	_T("登録"),
	_T("HELP"),

	_T("XF1"),
	_T("XF2"),
	_T("XF3"),
	_T("XF4"),
	_T("XF5"),

	_T("かな"),
	_T("ﾛｰﾏ字"),
	_T("ｺｰﾄﾞ入力"),
	_T("CAPS"),
	_T("INS"),
	_T("ひらがな"),
	_T("全角"),

	_T("BREAK"),
	_T("COPY"),
	_T("F1"),
	_T("F2"),
	_T("F3"),
	_T("F4"),
	_T("F5"),
	_T("F6"),
	_T("F7"),
	_T("F8"),
	_T("F9"),
	_T("F10"),
	NULL,
	NULL,
	NULL,

	_T("SHIFT"),
	_T("CTRL"),
	_T("OPT.1"),
	_T("OPT.2")
};

//---------------------------------------------------------------------------
//
//	キーID取得
//
//---------------------------------------------------------------------------
LPCSTR FASTCALL CInput::GetKeyID(int nID)
{
	ASSERT((nID >= 0) && (nID < 0x100));

	return KeyIDTable[nID];
}

//---------------------------------------------------------------------------
//
//	DirectX キーIDテーブル
//
//---------------------------------------------------------------------------
LPCSTR CInput::KeyIDTable[] = {
	NULL,							// 0x00
	_T("DIK_ESCAPE"),				// 0x01
	_T("DIK_1"),					// 0x02
	_T("DIK_2"),					// 0x03
	_T("DIK_3"),					// 0x04
	_T("DIK_4"),					// 0x05
	_T("DIK_5"),					// 0x06
	_T("DIK_6"),					// 0x07
	_T("DIK_7"),					// 0x08
	_T("DIK_8"),					// 0x09
	_T("DIK_9"),					// 0x0A
	_T("DIK_0"),					// 0x0B
	_T("DIK_MINUS"),				// 0x0C
	_T("DIK_EQUALS"),				// 0x0D
	_T("DIK_BACK"),					// 0x0E
	_T("DIK_TAB"),					// 0x0F
	_T("DIK_Q"),					// 0x10
	_T("DIK_W"),					// 0x11
	_T("DIK_E"),					// 0x12
	_T("DIK_R"),					// 0x13
	_T("DIK_T"),					// 0x14
	_T("DIK_Y"),					// 0x15
	_T("DIK_U"),					// 0x16
	_T("DIK_I"),					// 0x17
	_T("DIK_O"),					// 0x18
	_T("DIK_P"),					// 0x19
	_T("DIK_LBRACKET"),				// 0x1A
	_T("DIK_RBRACKET"),				// 0x1B
	_T("DIK_RETURN"),				// 0x1C
	_T("DIK_LCONTROL"),				// 0x1D
	_T("DIK_A"),					// 0x1E
	_T("DIK_S"),					// 0x1F
	_T("DIK_D"),					// 0x20
	_T("DIK_F"),					// 0x21
	_T("DIK_G"),					// 0x22
	_T("DIK_H"),					// 0x23
	_T("DIK_J"),					// 0x24
	_T("DIK_K"),					// 0x25
	_T("DIK_L"),					// 0x26
	_T("DIK_SEMICOLON"),			// 0x27
	_T("DIK_APOSTROPHE"),			// 0x28
	_T("DIK_GRAVE"),				// 0x29
	_T("DIK_LSHIFT"),				// 0x2A
	_T("DIK_BACKSLASH"),			// 0x2B
	_T("DIK_Z"),					// 0x2C
	_T("DIK_X"),					// 0x2D
	_T("DIK_C"),					// 0x2E
	_T("DIK_V"),					// 0x2F
	_T("DIK_B"),					// 0x30
	_T("DIK_N"),					// 0x31
	_T("DIK_M"),					// 0x32
	_T("DIK_COMMA"),				// 0x33
	_T("DIK_PERIOD"),				// 0x34
	_T("DIK_SLASH"),				// 0x35
	_T("DIK_RSHIFT"),				// 0x36
	_T("DIK_MULTIPLY"),				// 0x37
	_T("DIK_LMENU"),				// 0x38
	_T("DIK_SPACE"),				// 0x39
	_T("DIK_CAPITAL"),				// 0x3A
	_T("DIK_F1"),					// 0x3B
	_T("DIK_F2"),					// 0x3C
	_T("DIK_F3"),					// 0x3D
	_T("DIK_F4"),					// 0x3E
	_T("DIK_F5"),					// 0x3F
	_T("DIK_F6"),					// 0x40
	_T("DIK_F7"),					// 0x41
	_T("DIK_F8"),					// 0x42
	_T("DIK_F9"),					// 0x43
	_T("DIK_F10"),					// 0x44
	_T("DIK_NUMLOCK"),				// 0x45
	_T("DIK_SCROLL"),				// 0x46
	_T("DIK_NUMPAD7"),				// 0x47
	_T("DIK_NUMPAD8"),				// 0x48
	_T("DIK_NUMPAD9"),				// 0x49
	_T("DIK_SUBTRACT"),				// 0x4A
	_T("DIK_NUMPAD4"),				// 0x4B
	_T("DIK_NUMPAD5"),				// 0x4C
	_T("DIK_NUMPAD6"),				// 0x4D
	_T("DIK_ADD"),					// 0x4E
	_T("DIK_NUMPAD1"),				// 0x4F
	_T("DIK_NUMPAD2"),				// 0x50
	_T("DIK_NUMPAD3"),				// 0x51
	_T("DIK_NUMPAD0"),				// 0x52
	_T("DIK_DECIMAL"),				// 0x53
	NULL,							// 0x54
	NULL,							// 0x55
	_T("DIK_OEM_102"),				// 0x56
	_T("DIK_F11"),					// 0x57
	_T("DIK_F12"),					// 0x58
	NULL,							// 0x59
	NULL,							// 0x5A
	NULL,							// 0x5B
	NULL,							// 0x5C
	NULL,							// 0x5D
	NULL,							// 0x5E
	NULL,							// 0x5F
	NULL,							// 0x60
	NULL,							// 0x61
	NULL,							// 0x62
	NULL,							// 0x63
	_T("DIK_F13"),					// 0x64
	_T("DIK_F14"),					// 0x65
	_T("DIK_F15"),					// 0x66
	NULL,							// 0x67
	NULL,							// 0x68
	NULL,							// 0x69
	NULL,							// 0x6A
	NULL,							// 0x6B
	NULL,							// 0x6C
	NULL,							// 0x6D
	NULL,							// 0x6E
	NULL,							// 0x6F
	_T("DIK_KANA"),					// 0x70
	NULL,							// 0x71
	NULL,							// 0x72
	_T("DIK_ABNT_C1"),				// 0x73
	NULL,							// 0x74
	NULL,							// 0x75
	NULL,							// 0x76
	NULL,							// 0x77
	NULL,							// 0x78
	_T("DIK_CONVERT"),				// 0x79
	NULL,							// 0x7A
	_T("DIK_NOCONVERT"),			// 0x7B
	NULL,							// 0x7C
	_T("DIK_YEN"),					// 0x7D
	_T("DIK_ABNT_C2"),				// 0x7E
	NULL,							// 0x7F
	NULL,							// 0x80
	NULL,							// 0x81
	NULL,							// 0x82
	NULL,							// 0x83
	NULL,							// 0x84
	NULL,							// 0x85
	NULL,							// 0x86
	NULL,							// 0x87
	NULL,							// 0x88
	NULL,							// 0x89
	NULL,							// 0x8A
	NULL,							// 0x8B
	NULL,							// 0x8C
	_T("DIK_NUMPADEQUALS"),			// 0x8D
	NULL,							// 0x8E
	NULL,							// 0x8F
	_T("DIK_PREVTRACK"),			// 0x90
	_T("DIK_AT"),					// 0x91
	_T("DIK_COLON"),				// 0x92
	_T("DIK_UNDERLINE"),			// 0x93
	_T("DIK_KANJI"),				// 0x94
	_T("DIK_STOP"),					// 0x95
	_T("DIK_AX"),					// 0x96
	_T("DIK_UNLABELED"),			// 0x97
	NULL,							// 0x98
	_T("DIK_NEXTTRACK"),			// 0x99
	NULL,							// 0x9A
	NULL,							// 0x9B
	_T("DIK_NUMPADENTER"),			// 0x9C
	_T("DIK_RCONTROL"),				// 0x9D
	NULL,							// 0x9E
	NULL,							// 0x9F
	_T("DIK_MUTE"),					// 0xA0
	_T("DIK_CALCULATOR"),			// 0xA1
	_T("DIK_PLAYPAUSE"),			// 0xA2
	NULL,							// 0xA3
	_T("DIK_MEDIASTOP"),			// 0xA4
	NULL,							// 0xA5
	NULL,							// 0xA6
	NULL,							// 0xA7
	NULL,							// 0xA8
	NULL,							// 0xA9
	NULL,							// 0xAA
	NULL,							// 0xAB
	NULL,							// 0xAC
	NULL,							// 0xAD
	_T("DIK_VOLUMEDOWN"),			// 0xAE
	NULL,							// 0xAF
	_T("DIK_VOLUMEUP"),				// 0xB0
	NULL,							// 0xB1
	_T("DIK_WEBHOME"),				// 0xB2
	_T("DIK_NUMPADCOMMA"),			// 0xB3
	NULL,							// 0xB4
	_T("DIK_DIVIDE"),				// 0xB5
	NULL,							// 0xB6
	_T("DIK_SYSRQ"),				// 0xB7
	_T("DIK_RMENU"),				// 0xB8
	NULL,							// 0xB9
	NULL,							// 0xBA
	NULL,							// 0xBB
	NULL,							// 0xBC
	NULL,							// 0xBD
	NULL,							// 0xBE
	NULL,							// 0xBF
	NULL,							// 0xC0
	NULL,							// 0xC1
	NULL,							// 0xC2
	NULL,							// 0xC3
	NULL,							// 0xC4
	_T("DIK_PAUSE"),				// 0xC5
	NULL,							// 0xC6
	_T("DIK_HOME"),					// 0xC7
	_T("DIK_UP"),					// 0xC8
	_T("DIK_PRIOR"),				// 0xC9
	NULL,							// 0xCA
	_T("DIK_LEFT"),					// 0xCB
	NULL,							// 0xCC
	_T("DIK_RIGHT"),				// 0xCD
	NULL,							// 0xCE
	_T("DIK_END"),					// 0xCF
	_T("DIK_DOWN"),					// 0xD0
	_T("DIK_NEXT"),					// 0xD1
	_T("DIK_INSERT"),				// 0xD2
	_T("DIK_DELETE"),				// 0xD3
	NULL,							// 0xD4
	NULL,							// 0xD5
	NULL,							// 0xD6
	NULL,							// 0xD7
	NULL,							// 0xD8
	NULL,							// 0xD9
	NULL,							// 0xDA
	_T("DIK_LWIN"),					// 0xDB
	_T("DIK_RWIN"),					// 0xDC
	_T("DIK_APPS"),					// 0xDD
	_T("DIK_POWER"),				// 0xDE
	_T("DIK_SLEEP"),				// 0xDF
	NULL,							// 0xE0
	NULL,							// 0xE1
	NULL,							// 0xE2
	_T("DIK_WAKE"),					// 0xE3
	NULL,							// 0xE4
	_T("DIK_WEBSEARCH"),			// 0xE5
	_T("DIK_WEBFAVORITES"),			// 0xE6
	_T("DIK_WEBREFRESH"),			// 0xE7
	_T("DIK_WEBSTOP"),				// 0xE8
	_T("DIK_WEBFORWARD"),			// 0xE9
	_T("DIK_WEBBACK"),				// 0xEA
	_T("DIK_MYCOMPUTER"),			// 0xEB
	_T("DIK_MAIL"),					// 0xEC
	_T("DIK_MEDIASELECT"),			// 0xED
	NULL,							// 0xEE
	NULL,							// 0xEF
	NULL,							// 0xF0
	NULL,							// 0xF1
	NULL,							// 0xF2
	NULL,							// 0xF3
	NULL,							// 0xF4
	NULL,							// 0xF5
	NULL,							// 0xF6
	NULL,							// 0xF7
	NULL,							// 0xF8
	NULL,							// 0xF9
	NULL,							// 0xFA
	NULL,							// 0xFB
	NULL,							// 0xFC
	NULL,							// 0xFD
	NULL,							// 0xFE
	NULL							// 0xFF
};

//---------------------------------------------------------------------------
//
//	キーマップ取得
//
//---------------------------------------------------------------------------
void FASTCALL CInput::GetKeyMap(DWORD *pKeyMap)
{
	ASSERT(this);
	ASSERT(pKeyMap);
	ASSERT_VALID(this);

	// 現在のキーマップをコピー
	memcpy(pKeyMap, m_KeyMap, sizeof(m_KeyMap));
}

//---------------------------------------------------------------------------
//
//	キーマップ設定
//
//---------------------------------------------------------------------------
void FASTCALL CInput::SetKeyMap(const DWORD *pKeyMap)
{
	// ASSERT_VALIDは入れない(Configから呼ばれる為)
	ASSERT(this);
	ASSERT(pKeyMap);

	// キーマップに設定
	memcpy(m_KeyMap, pKeyMap, sizeof(m_KeyMap));
}

//---------------------------------------------------------------------------
//
//	キー割り当ての有無をチェック
//	※コマンドハンドラから呼ばれるため、XM6で割り当てるキーはすべてチェック
//
//---------------------------------------------------------------------------
BOOL FASTCALL CInput::IsKeyMapped(int nID) const
{
	ASSERT(this);
	ASSERT((nID >= 0) && (nID < 0x100));
	ASSERT_VALID(this);

	// 割り当てコードが0なら未使用(マップされていない)
	if (m_KeyMap[nID] == 0) {
		return FALSE;
	}

	// マップされている
	return TRUE;
}

//===========================================================================
//
//	マウス
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	マウス初期化
//
//---------------------------------------------------------------------------
BOOL FASTCALL CInput::InitMouse()
{
	ASSERT(this);
	ASSERT(m_lpDI);
	ASSERT(!m_lpDIMouse);
	ASSERT_VALID(this);

	// マウスデバイスを作成
	if (FAILED(m_lpDI->CreateDevice(GUID_SysMouse, &m_lpDIMouse, NULL))) {
		return FALSE;
	}

	// マウスデータ形式を設定
	if (FAILED(m_lpDIMouse->SetDataFormat(&c_dfDIMouse))) {
		return FALSE;
	}

	// 協調レベルを設定(Win9x/WinNTで挙動が異なるため、分ける)
	if (::IsWinNT()) {
		// WindowsNT
		if (FAILED(m_lpDIMouse->SetCooperativeLevel(m_pFrmWnd->m_hWnd,
						DISCL_BACKGROUND | DISCL_NONEXCLUSIVE))) {
			return FALSE;
		}
	}
	else {
		// Windows9x
		if (FAILED(m_lpDIMouse->SetCooperativeLevel(m_pFrmWnd->m_hWnd,
						DISCL_FOREGROUND | DISCL_EXCLUSIVE))) {
			return FALSE;
		}
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	マウス入力
//
//---------------------------------------------------------------------------
void FASTCALL CInput::InputMouse(BOOL bEnable)
{
	HRESULT hr;
	DIMOUSESTATE dims;

	ASSERT(this);
	ASSERT(m_pFrmWnd);
	ASSERT(m_lpDIMouse);
	ASSERT(m_pMouse);
	ASSERT_VALID(this);

	// 処理してよいか
	if (!bEnable) {
		// マウスモードOFF
		if (m_bMouseMode) {
			m_pFrmWnd->PostMessage(WM_COMMAND, IDM_MOUSEMODE, 0);
		}

		// ボタンUPを通知
		m_pMouse->SetMouse(m_nMouseX, m_nMouseY, FALSE, FALSE);
		return;
	}

	// マウスモードONか
	if (!m_bMouseMode) {
		// ボタンUPを通知
		m_pMouse->SetMouse(m_nMouseX, m_nMouseY, FALSE, FALSE);
		return;
	}

	// デバイス状態を取得
	hr = m_lpDIMouse->GetDeviceState(sizeof(dims), &dims);
	if (hr != DI_OK) {
		// Acquireを試みる
		m_lpDIMouse->Acquire();
		m_dwMouseAcquire++;

		// マウスリセット
		m_pMouse->ResetMouse();
		return;
	}

	// データをいったん確保
	m_nMouseX += dims.lX;
	m_nMouseY += dims.lY;
	if (dims.rgbButtons[0] & 0x80) {
		m_bMouseB[0] = TRUE;
	}
	else {
		m_bMouseB[0] = FALSE;
	}
	if (dims.rgbButtons[1] & 0x80) {
		m_bMouseB[1] = TRUE;
	}
	else {
		m_bMouseB[1] = FALSE;
	}

	// マウスデバイスへ通知
	m_pMouse->SetMouse(m_nMouseX, m_nMouseY, m_bMouseB[0], m_bMouseB[1]);

	// 中央ボタン機能が禁止されていれば、終了
	if (!m_bMouseMid) {
		m_dwMouseMid = 5;
		return;
	}

	// 中央ボタンをチェック。連続して押して離されたらマウスモードoff
	if (dims.rgbButtons[2] & 0x80) {
		// 押されている
		if (m_dwMouseMid < 4) {
			// リセット状態から
			m_dwMouseMid++;
			if (m_dwMouseMid == 4) {
				// 十分押されつづけているので、ホールド
				m_dwMouseMid = 3;
			}
		}
	}
	else {
		// 離されている
		if ((m_dwMouseMid == 3) || (m_dwMouseMid == 4)) {
			// 十分押された後か、その後の離しを１回検出した後に限る
			m_dwMouseMid++;
			if (m_dwMouseMid == 5) {
				// 3フレーム以上押されて、その後、2フレーム以上離された
				m_pFrmWnd->PostMessage(WM_COMMAND, IDM_MOUSEMODE, 0);
				m_dwMouseMid++;
			}
		}
		else {
			// 十分押されていないまま離された。リセット
			m_dwMouseMid = 0;
		}
	}
}

//---------------------------------------------------------------------------
//
//	マウスモード設定
//
//---------------------------------------------------------------------------
void FASTCALL CInput::SetMouseMode(BOOL bMode)
{
	ASSERT(this);
	ASSERT_VALID(this);

	// 現在のモードと違っていれば
	if (m_bMouseMode != bMode) {
		// モードを確保
		m_bMouseMode = bMode;

		// とにかくUnacquire
		if (m_lpDIMouse) {
			m_lpDIMouse->Unacquire();
		}

		// 中央ボタンカウントを無効化
		m_dwMouseMid = 5;
	}
}

//---------------------------------------------------------------------------
//
//	マウス情報取得
//
//---------------------------------------------------------------------------
void FASTCALL CInput::GetMouseInfo(int *pPos, BOOL *pBtn) const
{
	ASSERT(this);
	ASSERT(pPos);
	ASSERT(pBtn);
	ASSERT_VALID(this);

	// それぞれ3要素
	pPos[0] = m_nMouseX;
	pPos[1] = m_nMouseY;
	pPos[2] = (int)m_dwMouseMid;

	// ボタン
	pBtn[0] = m_bMouseB[0];
	pBtn[1] = m_bMouseB[1];
	pBtn[2] = m_bMouseMid;
}

//===========================================================================
//
//	Seccion de Joystick
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	Enumeracion del joystick
//
//---------------------------------------------------------------------------
void FASTCALL CInput::EnumJoy()
{
	ASSERT(this);
	ASSERT(m_lpDI);

	// Borra el numero de joysticks.
	m_dwJoyDevs = 0;

	// inicio de la enumeracion
	m_lpDI->EnumDevices(DIDEVTYPE_JOYSTICK, (LPDIENUMDEVICESCALLBACK)EnumCb,
							this, DIEDFL_ATTACHEDONLY);
}

//---------------------------------------------------------------------------
//
//	Llamada de retorno del joystick
//
//---------------------------------------------------------------------------
BOOL CALLBACK CInput::EnumCb(LPDIDEVICEINSTANCE pDevInst, LPVOID pvRef)
{
	CInput *pInput;

	ASSERT(pDevInst);
	ASSERT(pvRef);

	// Convertido a CInput.
	pInput = (CInput*)pvRef;

	// ujier que dice los nombres de los luchadores, barre el ring, etc.
	return pInput->EnumDev(pDevInst);
}

//---------------------------------------------------------------------------
//
//	Joystick anadido.
//
//---------------------------------------------------------------------------
BOOL FASTCALL CInput::EnumDev(LPDIDEVICEINSTANCE pDevInst)
{
	LPDIRECTINPUTDEVICE pInputDev;

	ASSERT(this);
	ASSERT(pDevInst);
	ASSERT(m_lpDI);

	// Comprobacion del numero maximo. Solo admite el maximo de dispositivos JoyDeviceMax.
	if (m_dwJoyDevs >= JoyDeviceMax) {
		ASSERT(m_dwJoyDevs == JoyDeviceMax);
		return DIENUM_STOP;
	}

	// Instancia segura.
	memcpy(&m_JoyDevInst[m_dwJoyDevs], pDevInst, sizeof(DIDEVICEINSTANCE));

	// Creacion de dispositivos
	pInputDev = NULL;
	if (FAILED(m_lpDI->CreateDevice(pDevInst->guidInstance,
									&pInputDev,
									NULL))) {
		return DIENUM_CONTINUE;
	}
	ASSERT(pInputDev);

	// especificacion del formato de datos
	if (FAILED(pInputDev->SetDataFormat(&c_dfDIJoystick))) {
		// liberacion del dispositivo
		pInputDev->Unacquire();
		pInputDev->Release();
		return DIENUM_CONTINUE;
	}

	// Adquisicion de Caps
	memset(&m_JoyDevCaps[m_dwJoyDevs], 0, sizeof(DIDEVCAPS));
	m_JoyDevCaps[m_dwJoyDevs].dwSize = sizeof(DIDEVCAPS);
	if (FAILED(pInputDev->GetCapabilities(&m_JoyDevCaps[m_dwJoyDevs]))) {
		// liberacion del dispositivo
		pInputDev->Unacquire();
		pInputDev->Release();
		return DIENUM_CONTINUE;
	}

	// Suelte el dispositivo una vez
	pInputDev->Unacquire();
	pInputDev->Release();

	// Ampliaciones y continuaciones
	m_dwJoyDevs++;
	return DIENUM_CONTINUE;
}

//---------------------------------------------------------------------------
//
//	Inicializacion del joystick
//	*Cuando se llama desde ApplyCfg, solo se hace si dwDevice es diferente.
//
//---------------------------------------------------------------------------
void FASTCALL CInput::InitJoy()
{
	int i;
	int nDevice;
	int nAxis;
	BOOL bError[JoyDevices];
	DIPROPDWORD dpd;
	DIPROPRANGE dpr;

	ASSERT(this);
	ASSERT(m_lpDI);

	// Suelte el dispositivo una vez
	for (i=0; i<JoyDevices; i++) {
		if (m_lpDIDev2[i]) {
			m_lpDIDev2[i]->Release();
			m_lpDIDev2[i] = NULL;
		}

		if (m_lpDIJoy[i]) {
			m_lpDIJoy[i]->Unacquire();
			m_lpDIJoy[i]->Release();
			m_lpDIJoy[i] = NULL;
		}
	}

	// Borrar datos de entrada.
	for (i=0; i<JoyDevices; i++) {
		memset(&m_JoyState[i], 0, sizeof(DIJOYSTATE));
	}

	// bucle de inicializacion
	for (i=0; i<JoyDevices; i++) {
		// Indicador de error OFF
		bError[i] = FALSE;

		// Si no se usa, nada.
		if (m_JoyCfg[i].nDevice == 0) {
			continue;
		}

		// Creacion de dispositivos
		nDevice = m_JoyCfg[i].nDevice - 1;
		if (FAILED(m_lpDI->CreateDevice(m_JoyDevInst[nDevice].guidInstance,
										&m_lpDIJoy[i],
										NULL))) {
			continue;
		}

		// Indicador de error ON
		bError[i] = TRUE;

		// especificacion del formato de datos
		if (FAILED(m_lpDIJoy[i]->SetDataFormat(&c_dfDIJoystick))) {
			continue;
		}

		// Ajuste del nivel de coordinacion
		if (FAILED(m_lpDIJoy[i]->SetCooperativeLevel(m_pFrmWnd->m_hWnd,
							DISCL_BACKGROUND | DISCL_NONEXCLUSIVE))) {
			continue;
		}

		// Ajuste del modo de valor (valor absoluto)
		memset(&dpd, 0, sizeof(dpd));
		dpd.diph.dwSize = sizeof(DIPROPDWORD); 
		dpd.diph.dwHeaderSize = sizeof(DIPROPHEADER);
		dpd.diph.dwHow = DIPH_DEVICE;
		dpd.dwData = DIPROPAXISMODE_ABS;
		if (FAILED(m_lpDIJoy[i]->SetProperty(DIPROP_AXISMODE, (LPDIPROPHEADER)&dpd))) {
			continue;
		}

		// Designacion de la zona muerta (no hay zona muerta)
		memset(&dpd, 0, sizeof(dpd));
		dpd.diph.dwSize = sizeof(DIPROPDWORD);
		dpd.diph.dwHeaderSize = sizeof(DIPROPHEADER);
		dpd.diph.dwHow = DIPH_DEVICE;
		dpd.dwData = 0;
		if (FAILED(m_lpDIJoy[i]->SetProperty(DIPROP_DEADZONE, (LPDIPROPHEADER)&dpd))) {
			continue;
		}

		//  Obtenga el rango por eje (intente obtenerlo para todos los ejes).
		for (nAxis=0; nAxis<JoyAxes; nAxis++) {
			//Adquisicion (o error).
			memset(&dpr, 0, sizeof(dpr));
			dpr.diph.dwSize = sizeof(DIPROPRANGE);
			dpr.diph.dwHeaderSize = sizeof(DIPROPHEADER);
			dpr.diph.dwHow = DIPH_BYOFFSET;
			dpr.diph.dwObj = JoyAxisOffsetTable[nAxis];
			m_lpDIJoy[i]->GetProperty(DIPROP_RANGE, (LPDIPROPHEADER)&dpr);

			// asegurar
			m_lJoyAxisMin[i][nAxis] = dpr.lMin;
			m_lJoyAxisMax[i][nAxis] = dpr.lMax;
		}

		// Obtener IDirectInputDevice2.
		if (FAILED(m_lpDIJoy[i]->QueryInterface(IID_IDirectInputDevice2,
										(LPVOID*)&m_lpDIDev2[i]))) {
			// Si no se puede obtener IDirectInputDevice2
			m_lpDIDev2[i] = NULL;
		}

		// Bandera de error OFF (exito)
		bError[i] = FALSE;
	}

	// Para los dispositivos con errores, libere
	for (i=0; i<JoyDevices; i++) {
		if (bError[i]) {
			ASSERT(m_lpDIJoy[i]);
			m_lpDIJoy[i]->Unacquire();
			m_lpDIJoy[i]->Release();
			m_lpDIJoy[i] = NULL;
		}
	}
}

//---------------------------------------------------------------------------
//
//	Entrada del joystick
//
//---------------------------------------------------------------------------
void FASTCALL CInput::InputJoy(BOOL bEnable)
{
	int i;
	int nAxis;
	BYTE *pOffset;
	LONG *pAxis;
	LONG lRange;
	LONG lAxis;

	ASSERT(this);

	// Si bEnable=FALSE y m_bJoyEnable=TRUE (lado VM), los datos de entrada se borran
	if (!bEnable && m_bJoyEnable) {
		for (i=0; i<JoyDevices; i++) {
			// datos de entrada en ceros
			memset(&m_JoyState[i], 0, sizeof(DIJOYSTATE));
		}
		return;
	}

	// bucle del dispositivo
	for (i=0; i<JoyDevices; i++) {
		// Omitir si no hay dispositivo
		if (!m_lpDIJoy[i]) {
			continue;
		}

		// sondeo
		if (m_lpDIDev2[i]) {
			if (FAILED(m_lpDIDev2[i]->Poll())) {
				// Intento de adquisicion.
				m_lpDIJoy[i]->Acquire();
				m_dwJoyAcquire[i]++;
				continue;
			}
		}

		// adquisicion de datos
		if (FAILED(m_lpDIJoy[i]->GetDeviceState(sizeof(DIJOYSTATE),
												&m_JoyState[i]))) {
			// Intento de adquisicion.
			m_lpDIJoy[i]->Acquire();
			m_dwJoyAcquire[i]++;
			continue;
		}

		// Conversion de ejes (conversion de -800 a 7FF)
		for (nAxis=0; nAxis<JoyAxes; nAxis++) {
			// adquisicion de punteros
			pOffset = (BYTE*)&m_JoyState[i];
			pOffset += JoyAxisOffsetTable[nAxis];
			pAxis = (LONG*)pOffset;








			/*
			CString sz;
			sz.Format(_T("\nValor rgdwPOV0: %d\n"), m_JoyState[i].rgdwPOV[0]);
			OutputDebugStringW(CT2W(sz));

			*/









			// Omitir si el eje no es valido.
			if (m_lJoyAxisMin[i][nAxis] == m_lJoyAxisMax[i][nAxis]) {
				continue;
			}

			// De abajo hacia arriba solo por -lMin (puntos finales alineados a 0).
			lAxis = *pAxis - m_lJoyAxisMin[i][nAxis];

			// Conversion de rango y prevencion de desbordamiento.
			lRange = m_lJoyAxisMax[i][nAxis] - m_lJoyAxisMin[i][nAxis] + 1;
			if (lRange >= 0x100000) {
				lRange >>= 12;
				lAxis >>= 12;
			}

			// transformacion
			lAxis <<= 12;
			lAxis /= lRange;
			lAxis -= 0x800;
			*pAxis = lAxis;

			/*
			CString sz;
			sz.Format(_T("\nValor laxis: %d\n"), lAxis);
			OutputDebugStringW(CT2W(sz));
			*/


		}
	}
}

//---------------------------------------------------------------------------
//
//	Sintesis del joystick
//
//---------------------------------------------------------------------------
void FASTCALL CInput::MakeJoy(BOOL bEnable)
{
	int i;
	int nAxis;
	int nButton;
	BYTE *pOffset;
	LONG *pAxis;
	LONG lAxis;
	PPI::joyinfo_t ji[PPI::PortMax];

	int dmy;

	ASSERT(this);
	ASSERT(m_pPPI);

	// Despejar todo
	memset(ji, 0, sizeof(ji));

	// Desactivar seria una buena opcion.
	if (!bEnable) {
		for (i=0; i<PPI::PortMax; i++) {
			// Enviar datos sin entrada.
			m_pPPI->SetJoyInfo(i, &ji[i]);

			// Borrar la secuencia de botones.
			for (nButton=0; nButton<JoyButtons; nButton++) {
				m_JoyCfg[i].dwCount[nButton] = 0;
			}
		}
		return;
	}

	// Interpretar los datos de entrada.
	for (i=0; i<JoyDevices; i++) {
		dmy = -1;

		// eje
		for (nAxis=0; nAxis<JoyAxes; nAxis++) {
#if 0
			// anulacion
			if (LOWORD(m_JoyCfg[i].dwAxis[nAxis]) == 0) {
				//continue;
			}
#else
			// Omitir si el eje no es valido.
			//if (m_lJoyAxisMin[i][nAxis] == m_lJoyAxisMax[i][nAxis]) {
			//	continue;
			//}
			dmy++; // dmy = naxis
			// if (dmy >= 4) {
			//	break;
			// }
#endif

			// adquisicion de punteros
			pOffset = (BYTE*)&m_JoyState[i];
			pOffset += JoyAxisOffsetTable[nAxis];
			pAxis = (LONG*)pOffset;

			// adquisicion de datos
			lAxis = *pAxis;

			//  El cero se ignora  ya que se repite, excepcion es el Pad direccion (POV).
			//  Se hace comparacion con el primer Axis para comprobar que sea entrada nula para el continue
			if ((lAxis == 0 && nAxis != 8)  || (lAxis == 0 && ji[HIWORD(m_JoyCfg[i].dwAxis[0])].axis[0] == 0))
			{							
				continue;
			}
			
			// inversi
			if (m_JoyCfg[i].bAxis[nAxis]) {
				// 7FF→-800 -800→7FF
				lAxis = -1 - lAxis;
			}

#if 0
			// Almacenado en la posicion de destino
			ASSERT(HIWORD(m_JoyCfg[i].dwAxis[nAxis]) >= 0);
			ASSERT(HIWORD(m_JoyCfg[i].dwAxis[nAxis]) < 2);
			ASSERT(LOWORD(m_JoyCfg[i].dwAxis[nAxis]) > 0);
			ASSERT(LOWORD(m_JoyCfg[i].dwAxis[nAxis]) <= 4);
			ji[HIWORD(m_JoyCfg[i].dwAxis[nAxis])].axis[LOWORD(m_JoyCfg[i].dwAxis[nAxis]) - 1]
				 = (DWORD)lAxis;
#else
			ji[HIWORD(m_JoyCfg[i].dwAxis[nAxis])].axis[dmy] = (DWORD)lAxis;
			


			/*
			PAD a Axis
			
			Arriba = 0
			Arriba-Derecha = 4500
			Derecha = 9000
			Abajo-Derecha = 13500
			Abajo = 18000
			Abajo-Izq = 22500
			Izq = 27000
			Izq-Arriba = 31500

			Axis 1 Arriba = -2048
			Arriba-Derecha = 1 -2048 / 0 2047
			Axis 0 Derecha = 2047
			Abajo-Derecha = 1 2047 / 0 2047
			Axis 1 Abajo = 2047
			Axis 0 Izq = -2048
			
			*/
	    				 
			
			if (nAxis == 8) // Movimientos de PAD direccional se mapean a Axis 0 y 1 *-*
			{
				//CString sz;
				//sz.Format(_T("\nAxis: %d  lAxis: %d \n"), nAxis, lAxis);
				//OutputDebugStringW(CT2W(sz));

				if (lAxis == 0) // Arriba
					ji[HIWORD(m_JoyCfg[i].dwAxis[1])].axis[1] = (DWORD)-2048;
				if (lAxis == 4500) // Arriba-Derecha
				{
					ji[HIWORD(m_JoyCfg[i].dwAxis[1])].axis[1] = (DWORD)-2048;
					ji[HIWORD(m_JoyCfg[i].dwAxis[0])].axis[0] = (DWORD)2047;
				}
				if (lAxis == 9000) // Derecha
					ji[HIWORD(m_JoyCfg[i].dwAxis[0])].axis[0] = (DWORD)2047;
				if (lAxis == 13500) // Abajo-Derecha
				{
					ji[HIWORD(m_JoyCfg[i].dwAxis[0])].axis[0] = (DWORD)2047;
					ji[HIWORD(m_JoyCfg[i].dwAxis[1])].axis[1] = (DWORD)2047;
				}
				if (lAxis == 18000) // Abajo
					ji[HIWORD(m_JoyCfg[i].dwAxis[1])].axis[1] = (DWORD)2047;
				if (lAxis == 22500) // Abajo-Izquierda
				{
					ji[HIWORD(m_JoyCfg[i].dwAxis[1])].axis[1] = (DWORD)2047;
					ji[HIWORD(m_JoyCfg[i].dwAxis[0])].axis[0] = (DWORD)-2048;
				}
				if (lAxis == 27000) // Izquierda
					ji[HIWORD(m_JoyCfg[i].dwAxis[0])].axis[0] = (DWORD)-2048;
				if (lAxis == 31500) // Arriba-Izquierda
				{
					ji[HIWORD(m_JoyCfg[i].dwAxis[0])].axis[0] = (DWORD)-2048;
					ji[HIWORD(m_JoyCfg[i].dwAxis[1])].axis[1] = (DWORD)-2048;
				}
			}






#endif
		}

		// boton
		for (nButton=0; nButton<JoyButtons; nButton++) {
			// anulacion
			if (LOWORD(m_JoyCfg[i].dwButton[nButton]) == 0) {
				continue;
			}

			// apagar
			if ((m_JoyState[i].rgbButtons[nButton] & 0x80) == 0) {
				// Solo se borra el contador de disparo continuo (la informacion de la pulsacion del boton se borra al principio).
				m_JoyCfg[i].dwCount[nButton] = 0;
				continue;
			}

			ASSERT(HIWORD(m_JoyCfg[i].dwButton[nButton]) >= 0);
			ASSERT(HIWORD(m_JoyCfg[i].dwButton[nButton]) < PPI::PortMax);
			ASSERT(LOWORD(m_JoyCfg[i].dwButton[nButton]) > 0);
			ASSERT(LOWORD(m_JoyCfg[i].dwButton[nButton]) <= PPI::ButtonMax);

			// Disparo cero.
			if (m_JoyCfg[i].dwRapid[nButton] == 0) {
				// Almacenado en la posicion de destino
				ji[HIWORD(m_JoyCfg[i].dwButton[nButton])].button[LOWORD(m_JoyCfg[i].dwButton[nButton]) - 1]
					= TRUE;
				continue;
			}

			// Hay un disparo continuo
			if (m_JoyCfg[i].dwCount[nButton] == 0) {
				// ON y recarga del contador como si fuera la primera vez
				ji[HIWORD(m_JoyCfg[i].dwButton[nButton])].button[LOWORD(m_JoyCfg[i].dwButton[nButton]) - 1]
					= TRUE;
				m_JoyCfg[i].dwCount[nButton] = JoyRapidTable[m_JoyCfg[i].dwRapid[nButton]];
				continue;
			}

			// Cuenta atras de fuego continuo; si es 0, ON y recarga del contador.
			m_JoyCfg[i].dwCount[nButton]--;
			if (m_JoyCfg[i].dwCount[nButton] == 0) {
				ji[HIWORD(m_JoyCfg[i].dwButton[nButton])].button[LOWORD(m_JoyCfg[i].dwButton[nButton]) - 1]
					= TRUE;
				m_JoyCfg[i].dwCount[nButton] = JoyRapidTable[m_JoyCfg[i].dwRapid[nButton]];
				continue;
			}

			// Separe ON/OFF en la primera y segunda mitad del contador.
			if (m_JoyCfg[i].dwCount[nButton] > (JoyRapidTable[m_JoyCfg[i].dwRapid[nButton]] >> 1)) {
				ji[HIWORD(m_JoyCfg[i].dwButton[nButton])].button[LOWORD(m_JoyCfg[i].dwButton[nButton]) - 1]
					= TRUE;
			}
			else {
				ji[HIWORD(m_JoyCfg[i].dwButton[nButton])].button[LOWORD(m_JoyCfg[i].dwButton[nButton]) - 1]
					= FALSE;
			}
		}
	}

	// Compuesto con teclado


	// Transmision a PPI
	for (i=0; i<PPI::PortMax; i++) {
		m_pPPI->SetJoyInfo(i, &ji[i]);
	}
}

//---------------------------------------------------------------------------
//
//	Activacion/desactivacion del joystick
//
//---------------------------------------------------------------------------
void FASTCALL CInput::EnableJoy(BOOL bEnable)
{
	PPI::joyinfo_t ji;

	ASSERT(this);

	// No es necesario si es lo mismo.
	if (m_bJoyEnable == bEnable) {
		return;
	}

	// cambiar
	m_bJoyEnable = bEnable;

	// Si se cambia a FALSE, se envia informacion nula al PPI
	if (!bEnable) {
		memset(&ji, 0, sizeof(ji));
		m_pPPI->SetJoyInfo(0, &ji);
		m_pPPI->SetJoyInfo(1, &ji);
	}
}

//---------------------------------------------------------------------------
//
//	Adquisicion de dispositivos de joystick
//
//---------------------------------------------------------------------------
int FASTCALL CInput::GetJoyDevice(int nJoy) const
{
	ASSERT(this);
	ASSERT((nJoy >= 0) && (nJoy < JoyDevices));

	// El 0 no esta asignado
	if (m_JoyCfg[nJoy].nDevice == 0) {
		return 0;
	}

	// Si no tiene un puntero de dispositivo, -1 como error de inicializacion
	if (!m_lpDIJoy[nJoy]) {
		return -1;
	}

	// Inicializacion exitosa, por lo que el dispositivo no.
	return m_JoyCfg[nJoy].nDevice;
}

//---------------------------------------------------------------------------
//
//	Adquisicion del eje del joystick
//
//---------------------------------------------------------------------------
LONG FASTCALL CInput::GetJoyAxis(int nJoy, int nAxis) const
{
	BYTE *pOffset;
	LONG *pAxis;

	ASSERT(this);
	ASSERT((nJoy >= 0) && (nJoy < JoyDevices));
	ASSERT((nAxis >= 0) && (nAxis < JoyAxes));

	// El 0 no esta asignado
	if (m_JoyCfg[nJoy].nDevice == 0) {
		return 0x10000;
	}

	// Error de inicializacion si no tiene un puntero de dispositivo
	if (!m_lpDIJoy[nJoy]) {
		return 0x10000;
	}

	// Si el eje no existe, error de especificacion
	if (m_lJoyAxisMin[nJoy][nAxis] == m_lJoyAxisMax[nJoy][nAxis]) {
		//return 0x10000;
	}

	// Valor de retorno
	pOffset = (BYTE*)&m_JoyState[nJoy];
	pOffset += JoyAxisOffsetTable[nAxis];
	pAxis = (LONG*)pOffset;
	return *pAxis;
}

//---------------------------------------------------------------------------
//
//	Adquisicion del boton del joystick
//
//---------------------------------------------------------------------------
DWORD FASTCALL CInput::GetJoyButton(int nJoy, int nButton) const
{
	ASSERT(this);
	ASSERT((nJoy >= 0) && (nJoy < JoyDevices));
	ASSERT((nButton >= 0) && (nButton < JoyButtons));

	// El 0 no esta asignado
	if (m_JoyCfg[nJoy].nDevice == 0) {
		return 0x10000;
	}

	// Error de inicializacion si no tiene un puntero de dispositivo
	if (!m_lpDIJoy[nJoy]) {
		return 0x10000;
	}

	// Si el numero de botones no coincide, error de especificacion
	if (nButton >= (int)m_JoyDevCaps[m_JoyCfg[nJoy].nDevice - 1].dwButtons) {
		return 0x10000;
	}
		

	// Valor de retorno
	return (DWORD)m_JoyState[nJoy].rgbButtons[nButton];
}

//---------------------------------------------------------------------------
//
//	Adquisicion de tapas de joystick
//
//---------------------------------------------------------------------------
BOOL FASTCALL CInput::GetJoyCaps(int nDevice, CString& strDesc, DIDEVCAPS *pCaps) const
{
	ASSERT(this);
	ASSERT(nDevice >= 0);
	ASSERT(pCaps);

	// Numero de dispositivos de joystick y comparacion
	if (nDevice >= (int)m_dwJoyDevs) {
		// El dispositivo en el indice especificado no existe.
		return FALSE;
	}

	// Ajuste de la descripcion
	ASSERT(nDevice < JoyDeviceMax);
	strDesc = m_JoyDevInst[nDevice].tszInstanceName;

	// Capsコピー
	*pCaps = m_JoyDevCaps[nDevice];

	// 成功
	return TRUE;
}

//---------------------------------------------------------------------------
//
//	Adquisicion de la configuracion del joystick
//
//---------------------------------------------------------------------------
void FASTCALL CInput::GetJoyCfg(int nJoy, LPJOYCFG lpJoyCfg) const
{
	ASSERT(this);
	ASSERT((nJoy >= 0) && (nJoy < JoyDevices));

	// Copiar la configuracion
	*lpJoyCfg = m_JoyCfg[nJoy];
}

//---------------------------------------------------------------------------
//
//	Ajuste del joystick
//
//---------------------------------------------------------------------------
void FASTCALL CInput::SetJoyCfg(int nJoy, const LPJOYCFG lpJoyCfg)
{
	ASSERT(this);
	ASSERT((nJoy >= 0) && (nJoy < JoyDevices));

	// Copiar la configuracion
	m_JoyCfg[nJoy] = *lpJoyCfg;
}

//---------------------------------------------------------------------------
//
//	Tabla de ejes del joystick
//
//---------------------------------------------------------------------------
const DWORD CInput::JoyAxisOffsetTable[JoyAxes] = {
	DIJOFS_X,
	DIJOFS_Y,
	DIJOFS_Z,
	DIJOFS_RX,
	DIJOFS_RY,
	DIJOFS_RZ,
	DIJOFS_SLIDER(0),
	DIJOFS_SLIDER(1),
	DIJOFS_POV(0)
};

//---------------------------------------------------------------------------
//
//	Mesa de fuego continuo con joystick
//	*La velocidad de disparo continuo supone 60 fotogramas/seg.
//
//---------------------------------------------------------------------------
const DWORD CInput::JoyRapidTable[JoyRapids + 1] = {
	0,									// (未使用エリア)
	30,									// 2発
	20,									// 3発
	15,									// 4発
	12,									// 5発
	8,									// 7.5発
	6,									// 10発
	5,									// 12発
	4,									// 15発
	3,									// 20発
	2									// 30発
};

#endif	// _WIN32
