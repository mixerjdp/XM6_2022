//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2004 ＰＩ．(ytanaka@ipc-tokai.or.jp)
//	[ キーボード ]
//
//---------------------------------------------------------------------------

#if !defined(keyboard_h)
#define keyboard_h

#include "device.h"
#include "event.h"
#include "sync.h"

//===========================================================================
//
//	キーボード
//
//===========================================================================
class Keyboard : public Device
{
public:
	// 内部データ定義
	typedef struct {
		BOOL connect;					// 接続フラグ
		BOOL status[0x80];				// 押下ステータス
		DWORD rep_code;					// リピートコード
		DWORD rep_count;				// リピートカウンタ
		DWORD rep_start;				// リピート時間(hus単位)
		DWORD rep_next;					// リピート時間(hus単位)
		BOOL send_en;					// キーデータ送信可
		BOOL send_wait;					// キーデータ送信差し止め
		DWORD msctrl;					// マウス制御信号
		BOOL tv_mode;					// X68000テレビモード
		BOOL tv_ctrl;					// コマンドによるテレビコントロール
		BOOL opt2_ctrl;					// OPT2によるテレビコントロール
		DWORD bright;					// キー明るさ
		DWORD led;						// キーLED(1で点灯)
		DWORD cmdbuf[0x10];				// コマンドバッファ
		DWORD cmdread;					// コマンドリードポインタ
		DWORD cmdwrite;					// コマンドライトポインタ
		DWORD cmdnum;					// コマンド数
	} keyboard_t;

public:
	// 基本ファンクション
	Keyboard(VM *p);
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
	void FASTCALL Connect(BOOL connect);
										// 接続
	BOOL FASTCALL IsConnect() const		{ return keyboard.connect; }
										// 接続チェック
	BOOL FASTCALL Callback(Event *ev);
										// イベントコールバック
	void FASTCALL MakeKey(DWORD code);
										// メイク
	void FASTCALL BreakKey(DWORD code);
										// ブレーク
	void FASTCALL Command(DWORD data);
										// コマンド
	BOOL FASTCALL GetCommand(DWORD& data);
										// コマンド取得
	void FASTCALL ClrCommand();
										// コマンドクリア
	void FASTCALL SendWait(BOOL flag);
										// キーデータ送信ウェイト
	BOOL FASTCALL IsSendWait() const	{ return keyboard.send_wait; }
										// キーデータ送信ウェイト取得
	void FASTCALL GetKeyboard(keyboard_t *buffer) const;
										// 内部データ取得

private:
	MFP *mfp;
										// MFP
	Mouse *mouse;
										// マウス
	keyboard_t keyboard;
										// 内部データ
	Event event;
										// イベント
	Sync *sync;
										// コマンドSync
};

#endif	// keyboard_h
