//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2005 ＰＩ．(ytanaka@ipc-tokai.or.jp)
//	[ MFC サブウィンドウ(Win32) ]
//
//---------------------------------------------------------------------------

#if defined(_WIN32)

#if !defined(mfc_w32_h)
#define mfc_w32_h

#include "keyboard.h"
#include "mfc_sub.h"
#include "mfc_midi.h"

//===========================================================================
//
//	コンポーネントウィンドウ
//
//===========================================================================
class CComponentWnd : public CSubTextWnd
{
public:
	CComponentWnd();
										// コンストラクタ
	void FASTCALL Setup();
										// セットアップ

private:
	CComponent *m_pComponent;
										// 最初のコンポーネント
};

//===========================================================================
//
//	OS情報ウィンドウ
//
//===========================================================================
class COSInfoWnd : public CSubTextWnd
{
public:
	COSInfoWnd();
										// コンストラクタ
	void FASTCALL Setup();
										// セットアップ
};

//===========================================================================
//
//	サウンドウィンドウ
//
//===========================================================================
class CSoundWnd : public CSubTextWnd
{
public:
	CSoundWnd();
										// コンストラクタ
	void FASTCALL Setup();
										// セットアップ

private:
	Scheduler *m_pScheduler;
										// スケジューラ
	OPMIF *m_pOPMIF;
										// OPM
	ADPCM *m_pADPCM;
										// ADPCM
	CSound *m_pSound;
										// サウンドコンポーネント
};

//===========================================================================
//
//	インプットウィンドウ
//
//===========================================================================
class CInputWnd : public CSubTextWnd
{
public:
	CInputWnd();
										// コンストラクタ
	void FASTCALL Setup();
										// セットアップ

private:
	void FASTCALL SetupInput(int x, int y);
										// セットアップ(入力系全体)
	void FASTCALL SetupMouse(int x, int y);
										// セットアップ(マウス)
	void FASTCALL SetupKey(int x, int y);
										// セットアップ(キーボード)
	void FASTCALL SetupJoy(int x, int y, int nJoy);
										// セットアップ(ジョイスティック)
	CInput *m_pInput;
										// インプットコンポーネント
};

//===========================================================================
//
//	ポートウィンドウ
//
//===========================================================================
class CPortWnd : public CSubTextWnd
{
public:
	CPortWnd();
										// コンストラクタ
	void FASTCALL Setup();
										// セットアップ

private:
	CPort *m_pPort;
										// ポートコンポーネント
};

//===========================================================================
//
//	ビットマップウィンドウ
//
//===========================================================================
class CBitmapWnd : public CSubTextWnd
{
public:
	CBitmapWnd();
										// コンストラクタ
	void FASTCALL Setup();
										// セットアップ

private:
	CDrawView *m_pView;
										// 描画ウィンドウ
};

//===========================================================================
//
//	MIDIドライバウィンドウ
//
//===========================================================================
class CMIDIDrvWnd : public CSubTextWnd
{
public:
	CMIDIDrvWnd();
										// コンストラクタ
	void FASTCALL Setup();
										// セットアップ

private:
	void FASTCALL SetupInfo(int x, int y, CMIDI::LPMIDIINFO pInfo);
										// セットアップ(サブ)
	void FASTCALL SetupExCnt(int x, int y, DWORD dwStart, DWORD dwEnd);
										// セットアップ(エクスクルーシブカウンタ)
	static LPCTSTR DescTable[];
										// 文字列テーブル
	MIDI *m_pMIDI;
										// MIDI
	CMIDI *m_pMIDIDrv;
										// MIDIドライバ
};

//===========================================================================
//
//	キーボードディスプレイウィンドウ
//
//===========================================================================
class CKeyDispWnd : public CWnd
{
public:
	CKeyDispWnd();
										// コンストラクタ
	void PostNcDestroy();
										// ウィンドウ削除完了
	void FASTCALL SetShiftMode(UINT nMode);
										// シフトモード設定
	void FASTCALL Refresh(const BOOL *m_pKeyBuf);
										// キー更新
	void FASTCALL SetKey(const BOOL *m_pKeyBuf);
										// キー一括設定

protected:
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
										// ウィンドウ作成
	afx_msg void OnDestroy(void);
										// ウィンドウ削除
	afx_msg void OnSize(UINT nType, int cx, int cy);
										// ウィンドウサイズ変更
	afx_msg BOOL OnEraseBkgnd(CDC *pDC);
										// 背景描画
	afx_msg void OnPaint();
										// ウィンドウ再描画
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
										// 左ボタン押下
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
										// 左ボタン離した
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
										// 右ボタン押下
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
										// 右ボタン離した
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
										// マウス移動
	afx_msg UINT OnGetDlgCode();
										// ダイアログコード取得

private:
	void FASTCALL SetupBitmap();
										// ビットマップ準備
	void FASTCALL OnDraw(CDC *pDC);
										// ウィンドウ描画
	LPCTSTR FASTCALL GetKeyString(int nKey);
										// キー文字列取得
	int FASTCALL PtInKey(CPoint& point);
										// 矩形内キーコード取得
	void FASTCALL DrawKey(int nKey, BOOL bDown);
										// キー表示
	void FASTCALL DrawBox(int nColorOut, int nColorIn, RECT& rect);
										// キーボックス描画
	void FASTCALL DrawCRBox(int nColorOut, int nColorIn, RECT& rect);
										// CRキーボックス描画
	void FASTCALL DrawChar(int x, int y, int nColor, DWORD dwChar);
										// キャラクタ描画
	void FASTCALL DrawCRChar(int x, int y, int nColor);
										// CRキャラクタ描画
	int FASTCALL CalcCGAddr(DWORD dwChar);
										// 全角CGROMアドレス算出
	UINT m_nMode;
										// SHIFTモード
	UINT m_nKey[0x80];
										// キー状態(表示)
	BOOL m_bKey[0x80];
										// キー状態(最終)
	int m_nPoint;
										// マウス移動ポイント
	const BYTE* m_pCG;
										// CGROM
	HBITMAP m_hBitmap;
										// ビットマップハンドル
	BYTE *m_pBits;
										// ビットマップビット
	UINT m_nBMPWidth;
										// ビットマップ幅
	UINT m_nBMPHeight;
										// ビットマップ高さ
	UINT m_nBMPMul;
										// ビットマップ乗算幅
	static RGBQUAD PalTable[0x10];
										// パレットテーブル
	static const RECT RectTable[0x75];
										// 矩形テーブル
	static LPCTSTR NormalTable[];
										// 文字列テーブル
	static LPCTSTR KanaTable[];
										// 文字列テーブル
	static LPCTSTR KanaShiftTable[];
										// 文字列テーブル
	static LPCTSTR MarkTable[];
										// 文字列テーブル
	static LPCTSTR AnotherTable[];
										// 文字列テーブル

	DECLARE_MESSAGE_MAP()
										// メッセージ マップあり
};

//===========================================================================
//
//	ソフトウェアキーボードウィンドウ
//
//===========================================================================
class CSoftKeyWnd : public CSubWnd
{
public:
	CSoftKeyWnd();
										// コンストラクタ
	void FASTCALL Refresh();
										// リフレッシュ

protected:
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
										// ウィンドウ作成
	afx_msg void OnDestroy();
										// ウィンドウ削除
	afx_msg void OnActivate(UINT nState, CWnd *pWnd, BOOL bMinimized);
										// アクティベート
	afx_msg LONG OnApp(UINT uParam, LONG lParam);
										// ユーザ(下位ウィンドウからの通知)

private:
	void FASTCALL Analyze(Keyboard::keyboard_t *pKbd);
										// キーボードデータ解析
	Keyboard *m_pKeyboard;
										// キーボード
	CInput *m_pInput;
										// インプット
	CStatusBar m_StatusBar;
										// ステータスバー
	CKeyDispWnd *m_pDispWnd;
										// キーディスプレイウィンドウ
	UINT m_nSoftKey;
										// ソフト押下中のキー

	DECLARE_MESSAGE_MAP()
										// メッセージ マップあり
};

#endif	// mfc_w32_h
#endif	// _WIN32
