//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 ＰＩ．(ytanaka@ipc-tokai.or.jp)
//	[ ログ ]
//
//---------------------------------------------------------------------------

#include "os.h"
#include "xm6.h"
#include "sync.h"
#include "log.h"
#include "device.h"
#include "vm.h"
#include "cpu.h"
#include "schedule.h"

//===========================================================================
//
//	ログ
//
//===========================================================================
//#define LOG_WIN32

//---------------------------------------------------------------------------
//
//	コンストラクタ
//
//---------------------------------------------------------------------------
Log::Log()
{
	// 管理ワークをクリア
	logtop = 0;
	lognum = 0;
	memset(logdata, 0, sizeof(logdata));
	logcount = 1;

	// デバイスなし
	cpu = NULL;
	scheduler = NULL;
	sync = NULL;
}

//---------------------------------------------------------------------------
//
//	初期化
//
//---------------------------------------------------------------------------
BOOL FASTCALL Log::Init(VM *vm)
{
	ASSERT(this);
	ASSERT(vm);

	// CPUを取得
	ASSERT(!cpu);
	cpu = (CPU *)vm->SearchDevice(MAKEID('C', 'P', 'U', ' '));
	ASSERT(cpu);

	// スケジューラを取得
	ASSERT(!scheduler);
	scheduler = (Scheduler *)vm->SearchDevice(MAKEID('S', 'C', 'H', 'E'));
	ASSERT(scheduler);

	// 同期オブジェクト作成
	ASSERT(!sync);
	sync = new Sync;
	ASSERT(sync);

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	クリーンアップ
//
//---------------------------------------------------------------------------
void FASTCALL Log::Cleanup()
{
	ASSERT(this);

	// データを全てクリア(Init成功時のみ)
	if (sync) {
		Clear();
	}

	// 同期オブジェクトを削除
	if (sync) {
		delete sync;
		sync = NULL;
	}
}

//---------------------------------------------------------------------------
//
//	リセット
//
//---------------------------------------------------------------------------
void FASTCALL Log::Reset()
{
	ASSERT(this);
	ASSERT_DIAG();

	// データを全てクリア
	Clear();
}

#if !defined(NDEBUG)
//---------------------------------------------------------------------------
//
//	診断
//
//---------------------------------------------------------------------------
void FASTCALL Log::AssertDiag() const
{
	ASSERT(this);
	ASSERT(cpu);
	ASSERT(cpu->GetID() == MAKEID('C', 'P', 'U', ' '));
	ASSERT(scheduler);
	ASSERT(scheduler->GetID() == MAKEID('S', 'C', 'H', 'E'));
	ASSERT(sync);
	ASSERT((logtop >= 0) && (logtop < LogMax));
	ASSERT((lognum >= 0) && (lognum <= LogMax));
	ASSERT(logcount >= 1);
}
#endif	// NDEBUG

//---------------------------------------------------------------------------
//
//	クリア
//
//---------------------------------------------------------------------------
void FASTCALL Log::Clear()
{
	int i;
	int index;

	ASSERT(this);

	// ロック
	sync->Lock();

	index = logtop;
	for (i=0; i<lognum; i++) {
		// データがあれば
		if (logdata[index]) {
			// 文字列ポインタを解放
			delete[] logdata[index]->string;

			// データ本体を解放
			delete logdata[index];

			// NULLに
			logdata[index] = NULL;
		}

		// インデックス移動
		index++;
		index &= (LogMax - 1);
	}

	// 管理ワークをクリア
	logtop = 0;
	lognum = 0;
	logcount = 1;

	// アンロック
	sync->Unlock();
}

//---------------------------------------------------------------------------
//
//	フォーマット(...)
//
//---------------------------------------------------------------------------
void Log::Format(loglevel level, const Device *device, char *format, ...)
{
	char buffer[0x200];
	va_list args;
	va_start(args, format);

	ASSERT(this);
	ASSERT(device);
	ASSERT_DIAG();

	// フォーマット
	vsprintf(buffer, format, args);

	// 可変長引数終了
	va_end(args);

	// メッセージ追加
	AddString(device->GetID(), level, buffer);
}

//---------------------------------------------------------------------------
//
//	フォーマット(va)
//
//---------------------------------------------------------------------------
void Log::vFormat(loglevel level, const Device *device, char *format, va_list args)
{
	char buffer[0x200];

	ASSERT(this);
	ASSERT(device);
	ASSERT_DIAG();

	// フォーマット
	vsprintf(buffer, format, args);

	// メッセージ追加
	AddString(device->GetID(), level, buffer);
}

//---------------------------------------------------------------------------
//
//	データを追加
//
//---------------------------------------------------------------------------
void FASTCALL Log::AddString(DWORD id, loglevel level, char *string)
{
	int index;

	ASSERT(this);
	ASSERT(string);
	ASSERT_DIAG();

	// ロック
	sync->Lock();

	// 追加する位置を決める
	if (lognum < LogMax) {
		ASSERT(logtop == 0);
		index = lognum;
		ASSERT(logdata[index] == NULL);
		lognum++;
	}
	else {
		ASSERT(lognum == LogMax);
		index = logtop;
		logtop++;
		logtop &= (LogMax - 1);
	}

	// 既に存在すれば解放
	if (logdata[index]) {
		delete[] logdata[index]->string;
		delete logdata[index];
	}

	// 構造体を配置
	logdata[index] = new logdata_t;
	logdata[index]->number = logcount++;
	logdata[index]->time = 0;
	logdata[index]->time = scheduler->GetTotalTime();
	logdata[index]->pc = cpu->GetPC();
	logdata[index]->id = id;
	logdata[index]->level = level;

	// 文字列処理
	logdata[index]->string = new char[strlen(string) + 1];
	strcpy(logdata[index]->string, string);

#if defined(LOG_WIN32)
	// Win32メッセージとして出力
	OutputDebugString(string);
	OutputDebugString(_T("\n"));
#endif	// LOG_WIN32

	// アンロック
	sync->Unlock();

	// ログが増えすぎたら、自動的に一度クリア(約16億件を目安とする)
	if (logcount >= 0x60000000) {
		Clear();
	}
}

//---------------------------------------------------------------------------
//
//	データ数を取得
//
//---------------------------------------------------------------------------
int FASTCALL Log::GetNum() const
{
	ASSERT(this);
	ASSERT_DIAG();

	return lognum;
}

//---------------------------------------------------------------------------
//
//	ログ最大記録数を取得
//
//---------------------------------------------------------------------------
int FASTCALL Log::GetMax() const
{
	ASSERT(this);
	ASSERT_DIAG();

	return LogMax;
}

//---------------------------------------------------------------------------
//
//	データを取得
//
//---------------------------------------------------------------------------
BOOL FASTCALL Log::GetData(int index, logdata_t *ptr)
{
	char *string;

	ASSERT(this);
	ASSERT(index >= 0);
	ASSERT(ptr);
	ASSERT_DIAG();

	// ロック
	sync->Lock();

	// 存在チェック
	if (index >= lognum) {
		sync->Unlock();
		return FALSE;
	}

	// インデックス算出
	index += logtop;
	index &= (LogMax - 1);

	// コピー(文字列ポインタ以外)
	ASSERT(logdata[index]);
	*ptr = *logdata[index];

	// コピー(文字列ポインタ)
	string = ptr->string;
	ptr->string = new char[strlen(string) + 1];
	strcpy(ptr->string, string);

	// アンロック
	sync->Unlock();
	return TRUE;
}
