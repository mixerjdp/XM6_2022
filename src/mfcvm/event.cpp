//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2005 ＰＩ．(ytanaka@ipc-tokai.or.jp)
//	[ イベント ]
//
//---------------------------------------------------------------------------

#include "os.h"
#include "xm6.h"
#include "vm.h"
#include "device.h"
#include "schedule.h"
#include "fileio.h"
#include "event.h"

//===========================================================================
//
//	イベント
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	コンストラクタ
//
//---------------------------------------------------------------------------
Event::Event()
{
	// メンバ変数初期化
	ev.device = NULL;
	ev.desc[0] = '\0';
	ev.user = 0;
	ev.time = 0;
	ev.remain = 0;
	ev.next = NULL;
}

//---------------------------------------------------------------------------
//
//	デストラクタ
//
//---------------------------------------------------------------------------
Event::~Event()
{
}

#if !defined(NDEBUG)
//---------------------------------------------------------------------------
//
//	診断
//
//---------------------------------------------------------------------------
void FASTCALL Event::AssertDiag() const
{
	ASSERT(this);
	ASSERT(ev.remain <=(ev.time + 625));
	ASSERT(ev.device);
	ASSERT(ev.scheduler);
	ASSERT(ev.scheduler->GetID() == MAKEID('S', 'C', 'H', 'E'));
	ASSERT(ev.desc[0] != '\0');
}
#endif	// NDEBUG

//---------------------------------------------------------------------------
//
//	セーブ
//
//---------------------------------------------------------------------------
BOOL FASTCALL Event::Save(Fileio *fio, int /*ver*/)
{
	size_t sz;

	ASSERT(this);
	ASSERT(fio);
	ASSERT_DIAG();

	// サイズをセーブ
	sz = sizeof(event_t);
	if (!fio->Write(&sz, sizeof(sz))) {
		return FALSE;
	}

	// 実体をセーブ
	if (!fio->Write(&ev, (int)sz)) {
		return FALSE;
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	ロード
//
//---------------------------------------------------------------------------
BOOL FASTCALL Event::Load(Fileio *fio, int ver)
{
	size_t sz;
	event_t lev;

	ASSERT(this);
	ASSERT(fio);

	// version2.01以前は、別ルーチン
	if (ver <= 0x0201) {
		return Load201(fio);
	}

	// サイズをロード、照合
	if (!fio->Read(&sz, sizeof(sz))) {
		return FALSE;
	}
	if (sz != sizeof(event_t)) {
		return FALSE;
	}

	// 実体をテンポラリ領域にロード
	if (!fio->Read(&lev, (int)sz)) {
		return FALSE;
	}

	// 現在のデータのうち、ポインタ類は残す
	lev.device = ev.device;
	lev.scheduler = ev.scheduler;
	lev.next = ev.next;

	// コピー、終了
	ev = lev;

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	ロード(version 2.01)
//
//---------------------------------------------------------------------------
BOOL FASTCALL Event::Load201(Fileio *fio)
{
	size_t sz;
	event201_t ev201;

	ASSERT(this);
	ASSERT(fio);

	// サイズをロード、照合
	if (!fio->Read(&sz, sizeof(sz))) {
		return FALSE;
	}
	if (sz != sizeof(event201_t)) {
		return FALSE;
	}

	// 実体をテンポラリ領域にロード
	if (!fio->Read(&ev201, (int)sz)) {
		return FALSE;
	}

	// コピー(共通)
	strcpy(ev.desc, ev201.desc);
	ev.user = ev201.user;
	ev.time = ev201.time;
	ev.remain = ev201.remain;

	// enableが降りていれば、remain,timeは0
	if (!ev201.enable) {
		ev.time = 0;
		ev.remain = 0;
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	デバイス設定
//
//---------------------------------------------------------------------------
void FASTCALL Event::SetDevice(Device *p)
{
	VM *vm;

	ASSERT(this);
	ASSERT(!ev.device);
	ASSERT(p);

	// スケジューラを得る
	vm = p->GetVM();
	ASSERT(vm);
	ev.scheduler = (Scheduler*)vm->SearchDevice(MAKEID('S', 'C', 'H', 'E'));
	ASSERT(ev.scheduler);

	// デバイスを記憶
	ev.device = p;
}

//---------------------------------------------------------------------------
//
//	名称設定
//
//---------------------------------------------------------------------------
void FASTCALL Event::SetDesc(const char *desc)
{
	ASSERT(this);
	ASSERT(desc);
	ASSERT(strlen(desc) < sizeof(ev.desc));

	strcpy(ev.desc, desc);
}

//---------------------------------------------------------------------------
//
//	名称取得
//
//---------------------------------------------------------------------------
const char* FASTCALL Event::GetDesc() const
{
	ASSERT(this);
	ASSERT_DIAG();

	return ev.desc;
}

//---------------------------------------------------------------------------
//
//	時間周期設定
//
//---------------------------------------------------------------------------
void FASTCALL Event::SetTime(DWORD hus)
{
	ASSERT(this);
	ASSERT_DIAG();

	// 時間周期設定(0ならイベント停止)
	ev.time = hus;

	// カウンタ設定。実行中は経過分だけ上げる
	// この際、上げすぎないよう注意する(Scheduler::GetPassedTime())
	ev.remain = ev.time;
	if (ev.remain > 0) {
		ev.remain += ev.scheduler->GetPassedTime();
	}
}

//---------------------------------------------------------------------------
//
//	時間を進める
//
//---------------------------------------------------------------------------
void FASTCALL Event::Exec(DWORD hus)
{
	ASSERT(this);
	ASSERT_DIAG();

	// 時間未設定なら、何もしない
	if (ev.time == 0) {
		ASSERT(ev.remain == 0);
		return;
	}

	// 時間を進める(オーバー分は無視)
	if (ev.remain <= hus) {
		// 時間をリロード
		ev.remain = ev.time;

		// コールバック実行
		ASSERT(ev.device);

		// FALSEで帰ってきたら、無効化
		if (!ev.device->Callback(this)) {
			ev.time = 0;
			ev.remain = 0;
		}
	}
	else {
		// 残りを減らす
		ev.remain -= hus;
	}
}
