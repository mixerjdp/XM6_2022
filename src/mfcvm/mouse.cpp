//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2005 ＰＩ．(ytanaka@ipc-tokai.or.jp)
//	[ マウス ]
//
//---------------------------------------------------------------------------

#include "os.h"
#include "xm6.h"
#include "vm.h"
#include "schedule.h"
#include "scc.h"
#include "log.h"
#include "fileio.h"
#include "config.h"
#include "mouse.h"

//===========================================================================
//
//	マウス
//
//===========================================================================
//#define MOUSE_LOG

//---------------------------------------------------------------------------
//
//	コンストラクタ
//
//---------------------------------------------------------------------------
Mouse::Mouse(VM *p) : Device(p)
{
	// デバイスIDを初期化
	dev.id = MAKEID('M', 'O', 'U', 'S');
	dev.desc = "Mouse";
}

//---------------------------------------------------------------------------
//
//	初期化
//
//---------------------------------------------------------------------------
BOOL FASTCALL Mouse::Init()
{
	Scheduler *scheduler;

	ASSERT(this);

	// 基本クラス
	if (!Device::Init()) {
		return FALSE;
	}

	// SCC取得
	scc = (SCC*)vm->SearchDevice(MAKEID('S', 'C', 'C', ' '));
	ASSERT(scc);

	// スケジューラ取得
	scheduler = (Scheduler*)vm->SearchDevice(MAKEID('S', 'C', 'H', 'E'));
	ASSERT(scheduler);

	// イベント設定
	event.SetDevice(this);
	event.SetDesc("Latency 725us");
	event.SetUser(0);
	event.SetTime(0);
	scheduler->AddEvent(&event);

	// マウス倍率205/256、左右反転なし、本体接続
	mouse.mul = 205;
	mouse.rev = FALSE;
	mouse.port = 1;

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	クリーンアップ
//
//---------------------------------------------------------------------------
void FASTCALL Mouse::Cleanup()
{
	ASSERT(this);

	// 基本クラスへ
	Device::Cleanup();
}

//---------------------------------------------------------------------------
//
//	リセット
//
//---------------------------------------------------------------------------
void FASTCALL Mouse::Reset()
{
	ASSERT(this);
	LOG0(Log::Normal, "リセット");

	// MSCTRL信号を'L'に設定
	mouse.msctrl = FALSE;

	// リセットフラグを上げる
	mouse.reset = TRUE;
}

//---------------------------------------------------------------------------
//
//	セーブ
//
//---------------------------------------------------------------------------
BOOL FASTCALL Mouse::Save(Fileio *fio, int ver)
{
	size_t sz;

	ASSERT(this);
	LOG0(Log::Normal, "セーブ");

	// サイズをセーブ
	sz = sizeof(mouse_t);
	if (!fio->Write(&sz, sizeof(sz))) {
		return FALSE;
	}
	if (!fio->Write(&mouse, sizeof(mouse))) {
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
BOOL FASTCALL Mouse::Load(Fileio *fio, int ver)
{
	size_t sz;

	ASSERT(this);
	LOG0(Log::Normal, "ロード");

	// 本体をロード
	if (!fio->Read(&sz, sizeof(sz))) {
		return FALSE;
	}
	if (sz != sizeof(mouse_t)) {
		return FALSE;
	}
	if (!fio->Read(&mouse, sizeof(mouse_t))) {
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
void FASTCALL Mouse::ApplyCfg(const Config *config)
{
	ASSERT(this);
	ASSERT(config);
	LOG0(Log::Normal, "設定適用");

	// 比率
	if (mouse.mul != config->mouse_speed) {
		// 違ったら座標リセット
		mouse.mul = config->mouse_speed;
		mouse.reset = TRUE;
	}

	// 反転フラグ
	mouse.rev = config->mouse_swap;

	// ポート
	mouse.port = config->mouse_port;
	if (mouse.port == 0) {
		// 接続なしなので、イベントを止める
		mouse.reset = TRUE;
		event.SetTime(0);
	}
}

//---------------------------------------------------------------------------
//
//	内部データ取得
//
//---------------------------------------------------------------------------
void FASTCALL Mouse::GetMouse(mouse_t *buffer)
{
	ASSERT(this);
	ASSERT(buffer);

	// 内部ワークをコピー
	*buffer = mouse;
}

//---------------------------------------------------------------------------
//
//	イベントコールバック
//
//---------------------------------------------------------------------------
BOOL FASTCALL Mouse::Callback(Event* /*ev*/)
{
	DWORD status;
	int dx;
	int dy;

	ASSERT(this);
	ASSERT(scc);

	// ステータス作成
	status = 0;

	// ボタン作成
	if (mouse.left) {
		status |= 0x01;
	}
	if (mouse.right) {
		status |= 0x02;
	}

	// X差分作成
	dx = mouse.x - mouse.px;
	dx *= mouse.mul;
	dx >>= 8;
	if (dx > 0x7f) {
		dx = 0x7f;
		status |= 0x10;
	}
	if (dx < -0x80) {
		dx = -0x80;
		status |= 0x20;
	}

	// Y差分作成
	dy = mouse.y - mouse.py;
	dy *= mouse.mul;
	dy >>= 8;
	if (dy > 0x7f) {
		dy = 0x7f;
		status |= 0x40;
	}
	if (dy < -0x80) {
		dy = -0x80;
		status |= 0x80;
	}

	// SCCが受信していなければ何もしない
	if (!scc->IsRxEnable(1)) {
		return FALSE;
	}

	// 受信bpsが4800でなければ、フレーミングエラー
	if (!scc->IsBaudRate(1, 4800)) {
#if defined(MOUSE_LOG)
		LOG0(Log::Normal, "SCCボーレートエラー");
#endif	// MOUSE_LOG
		scc->FramingErr(1);
		return FALSE;
	}

	// データが8bitでなければ、フレーミングエラー
	if (scc->GetRxBit(1) != 8) {
#if defined(MOUSE_LOG)
		LOG0(Log::Normal, "SCCデータビット長エラー");
#endif	// MOUSE_LOG
		scc->FramingErr(1);
		return FALSE;
	}

	// ストップビットが2bitでなければ、フレーミングエラー
	if (scc->GetStopBit(1) != 3) {
#if defined(MOUSE_LOG)
		LOG0(Log::Normal, "SCCストップビットエラー");
#endif	// MOUSE_LOG
		scc->FramingErr(1);
		return FALSE;
	}

	// パリティなしでなければ、パリティエラー
	if (scc->GetParity(1) != 0) {
#if defined(MOUSE_LOG)
		LOG0(Log::Normal, "SCCパリティエラー");
#endif	// MOUSE_LOG
		scc->ParityErr(1);
		return FALSE;
	}

	// SCCの受信バッファにデータがあるなら送信しない
	if (!scc->IsRxBufEmpty(1)) {
#if defined(MOUSE_LOG)
		LOG0(Log::Normal, "SCC受信バッファにデータあり");
#endif	// MOUSE_LOG
		return FALSE;
	}

	// データ送信(3バイト)
#if defined(MOUSE_LOG)
	LOG3(Log::Normal, "データ送出 St:$%02X X:$%02X Y:$%02X", status, dx & 0xff, dy & 0xff);
#endif	// MOUSE_LOG
	scc->Send(1, status);
	scc->Send(1, dx);
	scc->Send(1, dy);

	// 前回データを更新
	mouse.px = mouse.x;
	mouse.py = mouse.y;

	return FALSE;
}

//---------------------------------------------------------------------------
//
//	MSCTRL制御
//
//---------------------------------------------------------------------------
void FASTCALL Mouse::MSCtrl(BOOL flag, int port)
{
	ASSERT(this);
	ASSERT((port == 1) || (port == 2));

#if defined(MOUSE_LOG)
	LOG2(Log::Normal, "PORT=%d MSCTRL=%d", port, flag);
#endif	// MOUSE_LOG

	// ポートが一致しなければ何もしない
	if (port != mouse.port) {
		return;
	}

	// 'H'から'L'への立下りでイベント発生
	if (flag) {
		mouse.msctrl = TRUE;
		return;
	}

	// 前回'H'かチェック
	if (!mouse.msctrl) {
		return;
	}

	// 記憶
	mouse.msctrl = FALSE;

	// イベント中なら無視
	if (event.GetTime() != 0) {
		return;
	}

	// イベント発生(725us)
	event.SetTime(725 * 2);
}

//---------------------------------------------------------------------------
//
//	マウスデータ設定
//
//---------------------------------------------------------------------------
void FASTCALL Mouse::SetMouse(int x, int y, BOOL left, BOOL right)
{
	ASSERT(this);

	// データ記憶
	mouse.x = x;
	mouse.y = y;
	if (mouse.rev) {
		mouse.left = right;
		mouse.right = left;
	}
	else {
		mouse.left = left;
		mouse.right = right;
	}

	// リセットフラグがあれば座標をリセット
	if (mouse.reset) {
		mouse.reset = FALSE;
		mouse.px = x;
		mouse.py = y;
	}
}

//---------------------------------------------------------------------------
//
//	マウスデータリセット
//
//---------------------------------------------------------------------------
void FASTCALL Mouse::ResetMouse()
{
	ASSERT(this);

	mouse.reset = TRUE;
}
