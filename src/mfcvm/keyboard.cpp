//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2005 ＰＩ．(ytanaka@ipc-tokai.or.jp)
//	[ キーボード ]
//
//---------------------------------------------------------------------------

#include "os.h"
#include "xm6.h"
#include "vm.h"
#include "schedule.h"
#include "mfp.h"
#include "mouse.h"
#include "sync.h"
#include "fileio.h"
#include "config.h"
#include "keyboard.h"

//===========================================================================
//
//	キーボード
//
//===========================================================================
//#define KEYBOARD_LOG

//---------------------------------------------------------------------------
//
//	コンストラクタ
//
//---------------------------------------------------------------------------
Keyboard::Keyboard(VM *p) : Device(p)
{
	// デバイスIDを初期化
	dev.id = MAKEID('K', 'E', 'Y', 'B');
	dev.desc = "Keyboard";

	// オブジェクト
	sync = NULL;
	mfp = NULL;
	mouse = NULL;
}

//---------------------------------------------------------------------------
//
//	初期化
//
//---------------------------------------------------------------------------
BOOL FASTCALL Keyboard::Init()
{
	int i;
	Scheduler *scheduler;

	ASSERT(this);

	// 基本クラス
	if (!Device::Init()) {
		return FALSE;
	}

	// Sync作成
	sync = new Sync;

	// MFP取得
	mfp = (MFP*)vm->SearchDevice(MAKEID('M', 'F', 'P', ' '));
	ASSERT(mfp);

	// スケジューラ取得
	scheduler = (Scheduler*)vm->SearchDevice(MAKEID('S', 'C', 'H', 'E'));
	ASSERT(scheduler);

	// マウス取得
	mouse = (Mouse*)vm->SearchDevice(MAKEID('M', 'O', 'U', 'S'));
	ASSERT(mouse);

	// イベント追加
	event.SetDevice(this);
	event.SetDesc("Key Repeat");
	event.SetUser(0);
	event.SetTime(0);
	scheduler->AddEvent(&event);

	// ワーク初期化(リセット信号はない。パワーオンリセットのみ)
	keyboard.connect = TRUE;
	for (i=0; i<0x80; i++) {
		keyboard.status[i] = FALSE;
	}
	keyboard.rep_code = 0;
	keyboard.rep_count = 0;
	keyboard.rep_start = 500 * 1000 * 2;
	keyboard.rep_next = 110 * 1000 * 2;
	keyboard.send_en = TRUE;
	keyboard.send_wait = FALSE;
	keyboard.msctrl = 0;
	keyboard.tv_mode = TRUE;
	keyboard.tv_ctrl = TRUE;
	keyboard.opt2_ctrl = TRUE;
	keyboard.bright = 0;
	keyboard.led = 0;

	// TrueKey用コマンドキュー
	sync->Lock();
	keyboard.cmdnum = 0;
	keyboard.cmdread = 0;
	keyboard.cmdwrite = 0;
	sync->Unlock();

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	クリーンアップ
//
//---------------------------------------------------------------------------
void FASTCALL Keyboard::Cleanup()
{
	ASSERT(this);

	// Sync削除
	if (sync) {
		delete sync;
		sync = NULL;
	}

	// 基本クラスへ
	Device::Cleanup();
}

//---------------------------------------------------------------------------
//
//	リセット
//
//---------------------------------------------------------------------------
void FASTCALL Keyboard::Reset()
{
	ASSERT(this);
	LOG0(Log::Normal, "リセット");

	// システムポートがリセットされるので、send_waitも解除される
	keyboard.send_wait = FALSE;

	// コマンドを全てクリア
	ClrCommand();

	// 0xffを送信
	if (keyboard.send_en && !keyboard.send_wait && keyboard.connect) {
		mfp->KeyData(0xff);
	}
}

//---------------------------------------------------------------------------
//
//	セーブ
//
//---------------------------------------------------------------------------
BOOL FASTCALL Keyboard::Save(Fileio *fio, int ver)
{
	size_t sz;

	ASSERT(this);
	ASSERT(fio);

	LOG0(Log::Normal, "セーブ");

	// サイズをセーブ
	sz = sizeof(keyboard_t);
	if (!fio->Write(&sz, sizeof(sz))) {
		return FALSE;
	}

	// 本体をセーブ
	if (!fio->Write(&keyboard, (int)sz)) {
		return FALSE;
	}

	// イベントをセーブ
	if (!event.Save(fio, ver)) {
		return FALSE;
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	ロード
//
//---------------------------------------------------------------------------
BOOL FASTCALL Keyboard::Load(Fileio *fio, int ver)
{
	size_t sz;

	ASSERT(this);
	ASSERT(fio);

	LOG0(Log::Normal, "ロード");

	// サイズをロード、照合
	if (!fio->Read(&sz, sizeof(sz))) {
		return FALSE;
	}
	if (sz != sizeof(keyboard_t)) {
		return FALSE;
	}

	// 本体をロード
	if (!fio->Read(&keyboard, (int)sz)) {
		return FALSE;
	}

	// イベントをロード
	if (!event.Load(fio, ver)) {
		return FALSE;
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	設定適用
//
//---------------------------------------------------------------------------
void FASTCALL Keyboard::ApplyCfg(const Config *config)
{
	ASSERT(config);
	LOG0(Log::Normal, "設定適用");

	// 接続
	Connect(config->kbd_connect);
}

//---------------------------------------------------------------------------
//
//	接続
//
//---------------------------------------------------------------------------
void FASTCALL Keyboard::Connect(BOOL connect)
{
	int i;

	ASSERT(this);

	// 一致していれば問題ない
	if (keyboard.connect == connect) {
		return;
	}

#if defined(KEYBOARD_LOG)
	if (connect) {
		LOG0(Log::Normal, "キーボード接続");
	}
	else {
		LOG0(Log::Normal, "キーボード切断");
	}
#endif	// KEYBOARD_LOG

	// 接続状態を保存
	keyboard.connect = connect;

	// キーをオフに、イベントを止める
	for (i=0; i<0x80; i++) {
		keyboard.status[i] = FALSE;
	}
	keyboard.rep_code = 0;
	event.SetTime(0);

	// 接続していればFFを送信
	if (keyboard.connect) {
		mfp->KeyData(0xff);
	}
}

//---------------------------------------------------------------------------
//
//	内部データ取得
//
//---------------------------------------------------------------------------
void FASTCALL Keyboard::GetKeyboard(keyboard_t *buffer) const
{
	ASSERT(this);
	ASSERT(buffer);

	// バッファへコピー
	*buffer = keyboard;
}

//---------------------------------------------------------------------------
//
//	イベントコールバック
//
//---------------------------------------------------------------------------
BOOL FASTCALL Keyboard::Callback(Event *ev)
{
	ASSERT(this);
	ASSERT(ev);

	// リピートコードが有効でなければ、中止
	if (keyboard.rep_code == 0) {
		ev->SetTime(0);
		return FALSE;
	}

	// キーボードが抜かれていれば、中止
	if (!keyboard.connect) {
		ev->SetTime(0);
		return FALSE;
	}

	// リピートカウントアップ
	keyboard.rep_count++;

	// メイクのみ発行(ブレークは出さない:maxim.x)
	if (keyboard.send_en && !keyboard.send_wait) {
#if defined(KEYBOARD_LOG)
		LOG2(Log::Normal, "リピート$%02X (%d回目)", keyboard.rep_code, keyboard.rep_count);
#endif	// KEYBOARD_LOG

		mfp->KeyData(keyboard.rep_code);
	}

	// 次のイベント時間を設定
	ev->SetTime(keyboard.rep_next);
	return TRUE;
}

//---------------------------------------------------------------------------
//
//	メイク
//
//---------------------------------------------------------------------------
void FASTCALL Keyboard::MakeKey(DWORD code)
{
	ASSERT(this);
	ASSERT((code >= 0x01) && (code <= 0x73));

	// キーボードが抜かれていれば届かない
	if (!keyboard.connect) {
		return;
	}

	// 既にメイクなら、何もしない
	if (keyboard.status[code]) {
		return;
	}

#if defined(KEYBOARD_LOG)
	LOG1(Log::Normal, "メイク $%02X", code);
#endif	// KEYBOARD_LOG

	// ステータス設定
	keyboard.status[code] = TRUE;

	// リピートを開始
	keyboard.rep_code = code;
	keyboard.rep_count = 0;
	event.SetTime(keyboard.rep_start);

	// MFPへMakeデータ送信
	if (keyboard.send_en && !keyboard.send_wait) {
		mfp->KeyData(keyboard.rep_code);
	}
}

//---------------------------------------------------------------------------
//
//	ブレーク
//
//---------------------------------------------------------------------------
void FASTCALL Keyboard::BreakKey(DWORD code)
{
	ASSERT(this);
	ASSERT((code >= 0x01) && (code <= 0x73));

	// キーボードが抜かれていれば届かない
	if (!keyboard.connect) {
		return;
	}

	// メイク状態であることが必要。既にブレークなら何もしない
	if (!keyboard.status[code]) {
		return;
	}

#if defined(KEYBOARD_LOG)
	LOG1(Log::Normal, "ブレーク $%02X", code);
#endif	// KEYBOARD_LOG

	// ステータス設定
	keyboard.status[code] = FALSE;

	// リピート中のキーなら、リピート取り下げ
	if (keyboard.rep_code == (DWORD)code) {
		keyboard.rep_code = 0;
		event.SetTime(0);
	}

	// MFPへデータ送信
	code |= 0x80;
	if (keyboard.send_en && !keyboard.send_wait) {
		mfp->KeyData(code);
	}
}

//---------------------------------------------------------------------------
//
//	キーデータ送信ウェイト
//
//---------------------------------------------------------------------------
void FASTCALL Keyboard::SendWait(BOOL flag)
{
	ASSERT(this);

	keyboard.send_wait = flag;
}

//---------------------------------------------------------------------------
//
//	コマンド
//
//---------------------------------------------------------------------------
void FASTCALL Keyboard::Command(DWORD data)
{
	ASSERT(this);
	ASSERT(data < 0x100);

	// キューチェック
	ASSERT(sync);
	ASSERT(keyboard.cmdnum <= 0x10);
	ASSERT(keyboard.cmdread < 0x10);
	ASSERT(keyboard.cmdwrite < 0x10);

	// キューへ追加(Syncを行う)
	sync->Lock();
	keyboard.cmdbuf[keyboard.cmdwrite] = data;
	keyboard.cmdwrite = (keyboard.cmdwrite + 1) & 0x0f;
	keyboard.cmdnum++;
	if (keyboard.cmdnum > 0x10) {
		ASSERT(keyboard.cmdnum == 0x11);
		keyboard.cmdnum = 0x10;
		keyboard.cmdread = (keyboard.cmdwrite + 1) & 0x0f;
	}
	sync->Unlock();

	// キーボードが抜かれていれば届かない
	if (!keyboard.connect) {
		return;
	}

#if defined(KEYBOARD_LOG)
	LOG1(Log::Normal, "コマンド送出 $%02X", data);
#endif	// KEYBOARD_LOG

	// ディスプレイ制御
	if (data < 0x40) {
		return;
	}

	// キーLED
	if (data >= 0x80) {
		keyboard.led = ~data;
		return;
	}

	// キーリピート開始
	if ((data & 0xf0) == 0x60) {
		keyboard.rep_start = data & 0x0f;
		keyboard.rep_start *= 100 * 1000 * 2;
		keyboard.rep_start += 200 * 1000 * 2;
		return;
	}

	// キーリピート継続
	if ((data & 0xf0) == 0x70) {
		keyboard.rep_next = data & 0x0f;
		keyboard.rep_next *= (data & 0x0f);
		keyboard.rep_next *= 5 * 1000 * 2;
		keyboard.rep_next += 30 * 1000 * 2;
		return;
	}

	if ((data & 0xf0) == 0x40) {
		data &= 0x0f;
		if (data < 0x08) {
			// MSCTRL制御
			keyboard.msctrl = data & 0x01;
			if (keyboard.msctrl) {
				mouse->MSCtrl(TRUE, 2);
			}
			else {
				mouse->MSCtrl(FALSE, 2);
			}
			return;
		}
		else {
			// キーデータ送出許可・禁止
			if (data & 0x01) {
				keyboard.send_en = TRUE;
			}
			else {
				keyboard.send_en = FALSE;
			}
			return;
		}
	}

	// 後は0x50台
	data &= 0x0f;

	switch (data >> 2) {
		// ディスプレイ制御キーモード切り替え
		case 0:
			if (data & 0x01) {
				keyboard.tv_mode = TRUE;
			}
			else {
				keyboard.tv_mode = FALSE;
			}
			return;

		// キーLED明るさ
		case 1:
			keyboard.bright = data & 0x03;
			return;

		// テレビコントロール有効・無効
		case 2:
			if (data & 0x01) {
				keyboard.tv_ctrl = TRUE;
			}
			else {
				keyboard.tv_ctrl = FALSE;
			}
			return;

		// OPT2コントロール有効・無効
		case 3:
			if (data & 0x01) {
				keyboard.opt2_ctrl = TRUE;
			}
			else {
				keyboard.opt2_ctrl = FALSE;
			}
			return;
	}

	// 通常、ここにはこない
	ASSERT(FALSE);
}

//---------------------------------------------------------------------------
//
//	コマンド取得
//
//---------------------------------------------------------------------------
BOOL Keyboard::GetCommand(DWORD& data)
{
	ASSERT(this);
	ASSERT(sync);
	ASSERT(keyboard.cmdnum <= 0x10);
	ASSERT(keyboard.cmdread < 0x10);
	ASSERT(keyboard.cmdwrite < 0x10);

	// ロック
	sync->Lock();

	// データがなければFALSE
	if (keyboard.cmdnum == 0) {
		sync->Unlock();
		return FALSE;
	}

	// データを取得
	data = keyboard.cmdbuf[keyboard.cmdread];
	keyboard.cmdread = (keyboard.cmdread + 1) & 0x0f;
	ASSERT(keyboard.cmdnum > 0);
	keyboard.cmdnum--;

	// アンロック
	sync->Unlock();

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	コマンドクリア
//
//---------------------------------------------------------------------------
void Keyboard::ClrCommand()
{
	ASSERT(this);
	ASSERT(sync);
	ASSERT(keyboard.cmdnum <= 0x10);
	ASSERT(keyboard.cmdread < 0x10);
	ASSERT(keyboard.cmdwrite < 0x10);

	// ロック
	sync->Lock();

	// 初期化
	keyboard.cmdnum = 0;
	keyboard.cmdread = 0;
	keyboard.cmdwrite = 0;

	// アンロック
	sync->Unlock();
}
