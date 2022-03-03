//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 ＰＩ．(ytanaka@ipc-tokai.or.jp)
//	[ イベント ]
//
//---------------------------------------------------------------------------

#if !defined(event_h)
#define event_h

//===========================================================================
//
//	イベント
//
//===========================================================================
class Event
{
public:
	// 内部データ定義
#if defined(_WIN32)
#pragma pack(push, 8)
#endif	// _WIN32
	typedef struct {
		DWORD remain;					// 残り時間
		DWORD time;						// トータル時間
		DWORD user;						// ユーザ定義データ
		Device *device;					// 親デバイス
		Scheduler *scheduler;			// スケジューラ
		Event *next;					// 次のイベント
		char desc[0x20];				// 名称
	} event_t;
#if defined(_WIN32)
#pragma pack(pop)
#endif	// _WIN32

public:
	// 基本ファンクション
	Event();
										// コンストラクタ
	virtual ~Event();
										// デストラクタ
#if !defined(NDEBUG)
	void FASTCALL AssertDiag() const;
										// 診断
#endif	// NDEBUG

	// ロード・セーブ
	BOOL FASTCALL Save(Fileio *fio, int ver);
										// セーブ
	BOOL FASTCALL Load(Fileio *fio, int ver);
										// ロード

	// プロパティ
	void FASTCALL SetDevice(Device *p);
										// 親デバイス設定
	Device* FASTCALL GetDevice() const	{ return ev.device; }
										// 親デバイス取得
	void FASTCALL SetDesc(const char *desc);
										// 名称設定
	const char* FASTCALL GetDesc() const;
										// 名称取得
	void FASTCALL SetUser(DWORD data)	{ ev.user = data; }
										// ユーザ定義データ設定
	DWORD FASTCALL GetUser() const		{ return ev.user; }
										// ユーザ定義データ取得

	// 時間管理
	void FASTCALL SetTime(DWORD hus);
										// 時間周期設定
	DWORD FASTCALL GetTime() const		{ return ev.time; }
										// 時間周期取得
	DWORD FASTCALL GetRemain() const	{ return ev.remain; }
										// 残り時間取得
	void FASTCALL Exec(DWORD hus);
										// 時間を進める

	// リンク設定・削除
	void FASTCALL SetNextEvent(Event *p) { ev.next = p; }
										// 次のイベントを設定
	Event* FASTCALL GetNextEvent() const { return ev.next; }
										// 次のイベントを取得

private:
	// 内部データ定義(Ver2.01まで。enableがある)
	typedef struct {
		Device *device;					// 親デバイス
		Scheduler *scheduler;			// スケジューラ
		Event *next;					// 次のイベント
		char desc[0x20];				// 名称
		DWORD user;						// ユーザ定義データ
		BOOL enable;					// イネーブル時間
		DWORD time;						// トータル時間
		DWORD remain;					// 残り時間
	} event201_t;

	BOOL FASTCALL Load201(Fileio *fio);
										// ロード(version 2.01以前)
	event_t ev;
										// 内部ワーク
};

#endif	// event_h
