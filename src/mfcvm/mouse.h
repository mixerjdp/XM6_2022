//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2003 ＰＩ．(ytanaka@ipc-tokai.or.jp)
//	[ マウス ]
//
//---------------------------------------------------------------------------

#if !defined(mouse_h)
#define mouse_h

#include "device.h"
#include "event.h"

//===========================================================================
//
//	マウス
//
//===========================================================================
class Mouse : public Device
{
public:
	// 内部データ定義
	typedef struct {
		BOOL msctrl;					// MSCTRL信号
		BOOL reset;						// リセットフラグ
		BOOL left;						// 左ボタン
		BOOL right;						// 右ボタン
		int x;							// x座標
		int y;							// y座標
		int px;							// 前回のx座標
		int py;							// 前回のy座標
		int mul;						// マウス倍率(256で標準)
		BOOL rev;						// 左右反転フラグ
		int port;						// 接続先ポート
	} mouse_t;

public:
	// 基本ファンクション
	Mouse(VM *p);
										// コンストラクタ
	BOOL FASTCALL Init();
										// 初期化
	void FASTCALL Cleanup();
										// クリーンアップ
	void FASTCALL Reset();
										// リセット
	BOOL FASTCALL Save(Fileio *fio, int ver);
										// セーブ
	BOOL FASTCALL Load(Fileio *fio, int ver);
										// ロード
	void FASTCALL ApplyCfg(const Config *config);
										// 設定適用

	// 外部API
	void FASTCALL GetMouse(mouse_t *buffer);
										// 内部データ取得
	BOOL FASTCALL Callback(Event *ev);
										// イベントコールバック
	void FASTCALL MSCtrl(BOOL flag, int port = 1);
										// MSCTRL制御
	void FASTCALL SetMouse(int x, int y, BOOL left, BOOL right);
										// マウスデータ指定
	void FASTCALL ResetMouse();
										// マウスデータリセット

private:
	mouse_t mouse;
										// 内部データ
	Event event;
										// イベント
	SCC *scc;
										// SCC
};

#endif	// mouse_h
