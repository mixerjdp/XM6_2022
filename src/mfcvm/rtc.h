//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 ＰＩ．(ytanaka@ipc-tokai.or.jp)
//	[ RTC(RP5C15) ]
//
//---------------------------------------------------------------------------

#if !defined(rtc_h)
#define rtc_h

#include "device.h"
#include "event.h"

//===========================================================================
//
//	RTC
//
//===========================================================================
class RTC : public MemDevice
{
public:
	typedef struct {
		DWORD sec;						// 秒
		DWORD min;						// 分
		DWORD hour;						// 時間
		DWORD week;						// 週の曜日
		DWORD day;						// 日
		DWORD month;					// 月
		DWORD year;						// 年
		BOOL carry;						// 秒キャリー

		BOOL timer_en;					// タイマー動作許可
		BOOL alarm_en;					// アラーム動作許可
		DWORD bank;						// バンク番号
		DWORD test;						// TESTレジスタ
		BOOL alarm_1hz;					// 1Hzパルス出力制御
		BOOL alarm_16hz;				// 16Hzパルス出力制御
		BOOL under_reset;				// 秒アンダーリセット
		BOOL alarm_reset;				// アラームリセット

		DWORD clkout;					// CLKOUTレジスタ
		BOOL adjust;					// アジャスト

		DWORD alarm_min;				// 分
		DWORD alarm_hour;				// 時間
		DWORD alarm_week;				// 週の曜日
		DWORD alarm_day;				// 日

		BOOL fullhour;					// 24時間フラグ
		DWORD leap;						// 閏年カウンタ

		BOOL signal_1hz;				// 1Hzシグナル(500msおきに変化)
		BOOL signal_16hz;				// 16Hzシグナル(31.25msおきに変化)
		DWORD signal_count;				// 16Hzカウンタ(0～15)
		DWORD signal_blink;				// 点滅シグナル(781.25msおきに変化)
		BOOL alarm;						// アラーム信号
		BOOL alarmout;					// ALARM OUT
	} rtc_t;

public:
	// 基本ファンクション
	RTC(VM *p);
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

	// メモリデバイス
	DWORD FASTCALL ReadByte(DWORD addr);
										// バイト読み込み
	DWORD FASTCALL ReadWord(DWORD addr);
										// ワード読み込み
	void FASTCALL WriteByte(DWORD addr, DWORD data);
										// バイト書き込み
	void FASTCALL WriteWord(DWORD addr, DWORD data);
										// ワード書き込み
	DWORD FASTCALL ReadOnly(DWORD addr) const;
										// 読み込みのみ

	// 外部API
	void FASTCALL GetRTC(rtc_t *buffer);
										// 内部データ取得
	BOOL FASTCALL Callback(Event *ev);
										// イベントコールバック
	BOOL FASTCALL GetTimerLED() const;
										// タイマーLED取得
	BOOL FASTCALL GetAlarmOut() const;
										// ALARM信号取得
	BOOL FASTCALL GetBlink(int drive) const;
										// FDD用点滅信号取得
	void FASTCALL Adjust(BOOL alarm);
										// 現在時刻を設定

private:
	void FASTCALL AlarmOut();
										// ALARM信号出力
	void FASTCALL SecUp();
										// 秒アップ
	void FASTCALL MinUp();
										// 分アップ
	void FASTCALL AlarmCheck();
										// アラームチェック
	MFP *mfp;
										// MFP
	rtc_t rtc;
										// 内部データ
	Event event;
										// イベント
	static const DWORD DayTable[];
										// 日付テーブル
};

#endif	// rtc_h
