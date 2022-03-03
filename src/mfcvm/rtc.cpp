//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2005 ＰＩ．(ytanaka@ipc-tokai.or.jp)
//	[ RTC(RP5C15) ]
//
//---------------------------------------------------------------------------

#include "os.h"
#include "xm6.h"
#include "vm.h"
#include "log.h"
#include "mfp.h"
#include "schedule.h"
#include "fileio.h"
#include "rtc.h"

//===========================================================================
//
//	RTC
//
//===========================================================================
//#define RTC_LOG

//---------------------------------------------------------------------------
//
//	コンストラクタ
//
//---------------------------------------------------------------------------
RTC::RTC(VM *p) : MemDevice(p)
{
	// デバイスIDを初期化
	dev.id = MAKEID('R', 'T', 'C', ' ');
	dev.desc = "RTC (RP5C15)";

	// 開始アドレス、終了アドレス
	memdev.first = 0xe8a000;
	memdev.last = 0xe8bfff;

	// ワーククリア
	mfp = NULL;
}

//---------------------------------------------------------------------------
//
//	初期化
//
//---------------------------------------------------------------------------
BOOL FASTCALL RTC::Init()
{
	ASSERT(this);

	// 基本クラス
	if (!MemDevice::Init()) {
		return FALSE;
	}

	// MFPを取得
	ASSERT(!mfp);
	mfp = (MFP*)vm->SearchDevice(MAKEID('M', 'F', 'P', ' '));
	ASSERT(mfp);

	// イベントを作成(32Hz)
	event.SetDevice(this);
	event.SetDesc("Clock 16Hz");
	event.SetUser(0);
	event.SetTime(62500);
	scheduler->AddEvent(&event);

	// 内部レジスタをクリア
	rtc.sec = 0;
	rtc.min = 0;
	rtc.hour = 0;
	rtc.week = 0;
	rtc.day = 0;
	rtc.month = 0;
	rtc.year = 0;
	rtc.carry = FALSE;

	rtc.timer_en = FALSE;
	rtc.alarm_en = FALSE;
	rtc.bank = 0;
	rtc.test = 0;
	rtc.alarm_1hz = FALSE;
	rtc.alarm_16hz = FALSE;
	rtc.under_reset = FALSE;
	rtc.alarm_reset = FALSE;

	rtc.clkout = 7;
	rtc.adjust = FALSE;

	rtc.alarm_min = 0;
	rtc.alarm_hour = 0;
	rtc.alarm_week = 0;
	rtc.alarm_day = 0;

	rtc.fullhour = TRUE;
	rtc.leap = 0;

	rtc.signal_1hz = TRUE;
	rtc.signal_16hz = TRUE;
	rtc.signal_blink = 1;
	rtc.signal_count = 0;
	rtc.alarm = FALSE;
	rtc.alarmout = FALSE;

	// 現在時刻を設定
	Adjust(TRUE);

	// MFPへ通知(ALARM信号オフ)
	mfp->SetGPIP(0, 1);

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	クリーンアップ
//
//---------------------------------------------------------------------------
void FASTCALL RTC::Cleanup()
{
	ASSERT(this);

	// 基本クラスへ
	MemDevice::Cleanup();
}

//---------------------------------------------------------------------------
//
//	リセット
//
//---------------------------------------------------------------------------
void FASTCALL RTC::Reset()
{
	ASSERT(this);
	LOG0(Log::Normal, "リセット");
}

//---------------------------------------------------------------------------
//
//	セーブ
//
//---------------------------------------------------------------------------
BOOL FASTCALL RTC::Save(Fileio *fio, int ver)
{
	size_t sz;

	ASSERT(this);
	ASSERT(fio);

	LOG0(Log::Normal, "セーブ");

	// サイズをセーブ
	sz = sizeof(rtc_t);
	if (!fio->Write(&sz, sizeof(sz))) {
		return FALSE;
	}

	// 本体をセーブ
	if (!fio->Write(&rtc, (int)sz)) {
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
BOOL FASTCALL RTC::Load(Fileio *fio, int ver)
{
	size_t sz;

	ASSERT(this);
	ASSERT(fio);

	LOG0(Log::Normal, "ロード");

	// サイズをロード、照合
	if (!fio->Read(&sz, sizeof(sz))) {
		return FALSE;
	}
	if (sz != sizeof(rtc_t)) {
		return FALSE;
	}

	// 本体をロード
	if (!fio->Read(&rtc, (int)sz)) {
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
void FASTCALL RTC::ApplyCfg(const Config* /*config*/)
{
	ASSERT(this);

	LOG0(Log::Normal, "設定適用");
}

//---------------------------------------------------------------------------
//
//	バイト読み込み
//
//---------------------------------------------------------------------------
DWORD FASTCALL RTC::ReadByte(DWORD addr)
{
	DWORD data;

	ASSERT(this);
	ASSERT((addr >= memdev.first) && (addr <= memdev.last));

	// 奇数アドレスのみデコードされている
	if ((addr & 1) == 0) {
		// ROM IOCSでワード読み出しを行っている。バスエラーは発生しない
		return 0xff;
	}

	// 32バイト単位でループ
	addr &= 0x1f;

	// ウェイト
	scheduler->Wait(1);

	// レジスタ別に分類(レジスタ0～レジスタ28)
	addr >>= 1;
	ASSERT((rtc.bank == 0) || (rtc.bank == 1));
	if (rtc.bank > 0) {
		// バンク1は先頭13バイトに有効
		if (addr <= 0x0c) {
			addr += 0x10;
		}
	}

	switch (addr) {
		// 秒(1)
		case 0x00:
			data = rtc.sec % 10;
			return (0xf0 | data);

		// 秒(10)
		case 0x01:
			data = rtc.sec / 10;
			return (0xf0 | data);

		// 分(1)
		case 0x02:
			data = rtc.min % 10;
			return (0xf0 | data);

		// 分(10)
		case 0x03:
			data = rtc.min / 10;
			return (0xf0 | data);

		// 時(1)
		case 0x04:
			// 12h,24hで分ける
			data = rtc.hour;
			if (!rtc.fullhour) {
				data %= 12;
			}
			data %= 10;
			return (0xf0 | data);

		// 時(10)
		case 0x05:
			// 12h,24hで分ける。12hのPMはb1を立てる
			data = rtc.hour;
			if (!rtc.fullhour) {
				data %= 12;
			}
			data /= 10;
			if (!rtc.fullhour && (rtc.hour >= 12)) {
				data |= 0x02;
			}
			return (0xf0 | data);

		// 曜日
		case 0x06:
			return (0xf0 | rtc.week);

		// 日(1)
		case 0x07:
			data = rtc.day % 10;
			return (0xf0 | data);

		// 日(10)
		case 0x08:
			data = rtc.day / 10;
			return (0xf0 | data);

		// 月(1)
		case 0x09:
			data = rtc.month % 10;
			return (0xf0 | data);

		// 月(10)
		case 0x0a:
			data = rtc.month / 10;
			return (0xf0 | data);

		// 年(1)
		case 0x0b:
			data = rtc.year % 10;
			return (0xf0 | data);

		// 年(10)
		case 0x0c:
			data = rtc.year / 10;
			return (0xf0 | data);

		// MODEレジスタ
		case 0x0d:
			data = 0xf0;
			if (rtc.timer_en) {
				data |= 0x08;
			}
			if (rtc.alarm_en) {
				data |= 0x04;
			}
			if (rtc.bank > 0) {
				data |= 0x01;
			}
			return data;

		// TESTレジスタ(Write Only)
		case 0x0e:
			return 0xf0;

		// RESETレジスタ(Write onlyではない？Si.x v2);
		case 0x0f:
			data = 0xf0;
			if (!rtc.alarm_1hz) {
				data |= 0x08;
			}
			if (!rtc.alarm_16hz) {
				data |= 0x04;
			}
			return data;

		// CLKOUTレジスタ
		case 0x10:
			ASSERT(rtc.clkout < 0x08);
			return (0xf0 | rtc.clkout);

		// ADJUSTレジスタ
		case 0x11:
			return (0xf0 | rtc.adjust);

		// アラーム分(1)
		case 0x12:
			data = rtc.alarm_min % 10;
			return (0xf0 | data);

		// アラーム分(10)
		case 0x13:
			data = rtc.alarm_min / 10;
			return (0xf0 | data);

		// アラーム時(1)
		case 0x14:
			// 12h,24hで分ける
			data = rtc.alarm_hour;
			if (!rtc.fullhour) {
				data %= 12;
			}
			data %= 10;
			return (0xf0 | data);

		// アラーム時(10)
		case 0x15:
			// 12h,24hで分ける。12hのPMはb1を立てる
			data = rtc.alarm_hour;
			if (!rtc.fullhour) {
				data %= 12;
			}
			data /= 10;
			if (!rtc.fullhour && (rtc.alarm_hour >= 12)) {
				data |= 0x02;
			}
			return (0xf0 | data);

		// アラーム曜日
		case 0x16:
			return (0xf0 | rtc.alarm_week);

		// アラーム日(1)
		case 0x17:
			data = rtc.alarm_day % 10;
			return (0xf0 | data);

		// アラーム日(10)
		case 0x18:
			data = rtc.alarm_day / 10;
			return (0xf0 | data);

		// 12h,24h切り替え
		case 0x1a:
			if (rtc.fullhour) {
				return 0xf1;
			}
			return 0xf0;

		// 閏年カウンタ
		case 0x1b:
			ASSERT(rtc.leap <= 3);
			return (0xf0 | rtc.leap);
	}

	LOG1(Log::Warning, "未実装レジスタ読み込み R%02d", addr);
	return 0xff;
}

//---------------------------------------------------------------------------
//
//	ワード読み込み
//
//---------------------------------------------------------------------------
DWORD FASTCALL RTC::ReadWord(DWORD addr)
{
	ASSERT(this);
	ASSERT((addr >= memdev.first) && (addr <= memdev.last));
	ASSERT((addr & 1) == 0);

	return (0xff00 | ReadByte(addr + 1));
}

//---------------------------------------------------------------------------
//
//	バイト書き込み
//
//---------------------------------------------------------------------------
void FASTCALL RTC::WriteByte(DWORD addr, DWORD data)
{
	ASSERT(this);
	ASSERT((addr >= memdev.first) && (addr <= memdev.last));
	ASSERT(data < 0x100);

	// 奇数アドレスのみデコードされている
	if ((addr & 1) == 0) {
		// バスエラーは発生しない
		return;
	}

	// 32バイト単位でループ
	addr &= 0x1f;

	// ウェイト
	scheduler->Wait(1);

	// レジスタ別に分類(レジスタ0～レジスタ28)
	addr >>= 1;
	ASSERT((rtc.bank == 0) || (rtc.bank == 1));
	if (rtc.bank > 0) {
		// バンク1は先頭13バイトに有効
		if (addr <= 0x0c) {
			addr += 0x10;
		}
	}

	switch (addr) {
		// 秒(1)
		case 0x00:
			data &= 0x0f;
			rtc.sec /= 10;
			rtc.sec *= 10;
			rtc.sec += data;
			return;

		// 秒(10)
		case 0x01:
			data &= 0x07;
			data *= 10;
			rtc.sec %= 10;
			rtc.sec += data;
			return;

		// 分(1)
		case 0x02:
			data &= 0x0f;
			rtc.min /= 10;
			rtc.min *= 10;
			rtc.min += data;
			return;

		// 分(10)
		case 0x03:
			data &= 0x07;
			data *= 10;
			rtc.min %= 10;
			rtc.min += data;
			return;

		// 時(1)
		case 0x04:
			data &= 0x0f;
			// 12h,24hで分ける
			if (rtc.fullhour || (rtc.hour < 12)) {
				// 24h or 12h(AM)
				rtc.hour /= 10;
				rtc.hour *= 10;
				rtc.hour += data;
			}
			else {
				// 12h(PM)
				rtc.hour -= 12;
				rtc.hour /= 10;
				rtc.hour *= 10;
				rtc.hour += data;
				rtc.hour += 12;
			}
			return;

		// 時(10)
		case 0x05:
			data &= 0x03;
			// 12h,24hで分ける
			if (rtc.fullhour) {
				// 24h
				data *= 10;
				rtc.hour %= 10;
				rtc.hour += data;
			}
			else {
				if (data & 0x02) {
					// 12h,PM強制選択(データシートによる)
					data &= 0x01;
					data *= 10;
					rtc.hour %= 10;
					rtc.hour += data;
					rtc.hour += 12;
				}
				else {
					// 12h,AM強制選択(データシートによる)
					data &= 0x01;
					data *= 10;
					rtc.hour %= 10;
					rtc.hour += data;
				}
			}
			return;

		// 曜日
		case 0x06:
			data &= 0x07;
			rtc.week = data;
			return;

		// 日(1)
		case 0x07:
			data &= 0x0f;
			rtc.day /= 10;
			rtc.day *= 10;
			rtc.day += data;
			return;

		// 日(10)
		case 0x08:
			data &= 0x03;
			data *= 10;
			rtc.day %= 10;
			rtc.day += data;
			return;

		// 月(1)
		case 0x09:
			data &= 0x0f;
			rtc.month /= 10;
			rtc.month *= 10;
			rtc.month += data;
			return;

		// 月(10)
		case 0x0a:
			data &= 0x01;
			data *= 10;
			rtc.month %= 10;
			rtc.month += data;
			return;

		// 年(1)
		case 0x0b:
			data &= 0x0f;
			rtc.year /= 10;
			rtc.year *= 10;
			rtc.year += data;
			return;

		// 年(10)
		case 0x0c:
			data &= 0x0f;
			data *= 10;
			rtc.year %= 10;
			rtc.year += data;
			return;

		// MODEレジスタ
		case 0x0d:
			// タイマイネーブル
			if (data & 0x08) {
				rtc.timer_en = TRUE;
				// キャリーが残っていれば、はきだす
				if (rtc.carry) {
					SecUp();
				}
			}
			else {
				rtc.timer_en = FALSE;
			}

			// アラームイネーブル
			if (data & 0x04) {
				rtc.alarm_en = TRUE;
			}
			else {
				rtc.alarm_en = FALSE;
			}
			AlarmOut();

			// バンクセレクト
			if (data & 0x01) {
				rtc.bank = 1;
			}
			else {
				rtc.bank = 0;
			}
			return;

		// TESTレジスタ
		case 0x0e:
			rtc.test = (data & 0x0f);
			if (rtc.test != 0) {
				LOG1(Log::Warning, "テストモード設定 $%02X", rtc.test);
			}
			return;

		// RESETレジスタ
		case 0x0f:
			// 1Hz, 16Hzパルス出力
			if (data & 0x08) {
				rtc.alarm_1hz = FALSE;
			}
			else {
				rtc.alarm_1hz = TRUE;
			}
			if (data & 0x04) {
				rtc.alarm_16hz = FALSE;
			}
			else {
				rtc.alarm_16hz = TRUE;
			}
			AlarmOut();

			if (data & 0x02) {
				rtc.under_reset = TRUE;
				// イベントをリセットする
				event.SetTime(event.GetTime());
			}
			else {
				rtc.under_reset = FALSE;
			}
			if (data & 0x01) {
				// ALARM信号をHレベルにした後で
				rtc.alarm_reset = TRUE;
				rtc.alarm = FALSE;
				AlarmOut();
				// 現在時刻と一致させる
				rtc.alarm_min = rtc.min;
				rtc.alarm_hour = rtc.hour;
				rtc.alarm_week = rtc.week;
				rtc.alarm_day = rtc.day;
			}
			else {
				rtc.alarm_reset = FALSE;
			}
			return;

		// CLKOUTレジスタ
		case 0x10:
			rtc.clkout = (data & 0x07);
#if defined(RTC_LOG)
			LOG1(Log::Normal, "CLKOUTセット %d", rtc.clkout);
#endif	// RTC_LOG
			return;

		// ADJレジスタ
		case 0x11:
			rtc.adjust = (data & 0x01);
			if (data & 0x01) {
				// 1で秒アジャスト
				if (rtc.sec < 30) {
					// 切り捨て
					rtc.sec = 0;
				}
				else {
					// 切り上げ
					MinUp();
				}
			}
			return;

		// アラーム分(1)
		case 0x12:
			data &= 0x0f;
			rtc.alarm_min /= 10;
			rtc.alarm_min *= 10;
			rtc.alarm_min += data;
			return;

		// アラーム分(10)
		case 0x13:
			data &= 0x07;
			data *= 10;
			rtc.alarm_min %= 10;
			rtc.alarm_min += data;
			return;

		// アラーム時(1)
		case 0x14:
			data &= 0x0f;
			// 12h,24hで分ける
			if (rtc.fullhour || (rtc.alarm_hour < 12)) {
				// 24h or 12h(AM)
				rtc.alarm_hour /= 10;
				rtc.alarm_hour *= 10;
				rtc.alarm_hour += data;
			}
			else {
				// 12h(PM)
				rtc.alarm_hour -= 12;
				rtc.alarm_hour /= 10;
				rtc.alarm_hour *= 10;
				rtc.alarm_hour += data;
				rtc.alarm_hour += 12;
			}
			return;

		// アラーム時(10)
		case 0x15:
			data &= 0x03;
			// 12h,24hで分ける
			if (rtc.fullhour) {
				// 24h
				data *= 10;
				rtc.alarm_hour %= 10;
				rtc.alarm_hour += data;
			}
			else {
				if (data & 0x02) {
					// 12h,PM強制選択(データシートによる)
					data &= 0x01;
					data *= 10;
					rtc.alarm_hour %= 10;
					rtc.alarm_hour += data;
					rtc.alarm_hour += 12;
				}
				else {
					// 12h,AM強制選択(データシートによる)
					data &= 0x01;
					data *= 10;
					rtc.alarm_hour %= 10;
					rtc.alarm_hour += data;
				}
			}
			return;

		// アラーム曜日
		case 0x16:
			data &= 0x07;
			rtc.alarm_week = data;
			return;

		// アラーム日(1)
		case 0x17:
			data &= 0x0f;
			rtc.alarm_day /= 10;
			rtc.alarm_day *= 10;
			rtc.alarm_day += data;
			return;

		// アラーム日(10)
		case 0x18:
			data &= 0x03;
			data *= 10;
			rtc.alarm_day %= 10;
			rtc.alarm_day += data;
			return;

		// 12h,24h切り替え
		case 0x1a:
			if (data & 0x01) {
				rtc.fullhour = TRUE;
#if defined(RTC_LOG)
				LOG0(Log::Normal, "24時間制");
#endif	// RTC_LOG
			}
			else {
				rtc.fullhour = FALSE;
#if defined(RTC_LOG)
				LOG0(Log::Normal, "12時間制");
#endif	// RTC_LOG
			}
			return;

		// 閏年カウンタ
		case 0x1b:
			rtc.leap = (data & 0x03);
			return;
	}

	LOG2(Log::Warning, "未実装レジスタ書き込み R%02d <- $%02X",
							addr, data);
}

//---------------------------------------------------------------------------
//
//	ワード書き込み
//
//---------------------------------------------------------------------------
void FASTCALL RTC::WriteWord(DWORD addr, DWORD data)
{
	ASSERT(this);
	ASSERT((addr >= memdev.first) && (addr <= memdev.last));
	ASSERT((addr & 1) == 0);
	ASSERT(data < 0x10000);

	WriteByte(addr + 1, (BYTE)data);
}

//---------------------------------------------------------------------------
//
//	読み込みのみ
//
//---------------------------------------------------------------------------
DWORD FASTCALL RTC::ReadOnly(DWORD addr) const
{
	DWORD data;

	ASSERT(this);
	ASSERT((addr >= memdev.first) && (addr <= memdev.last));

	// 奇数アドレスのみデコードされている
	if ((addr & 1) == 0) {
		return 0xff;
	}

	// 32バイト単位でループ
	addr &= 0x1f;

	// レジスタ別に分類(レジスタ0～レジスタ28)
	addr >>= 1;
	ASSERT((rtc.bank == 0) || (rtc.bank == 1));
	if (rtc.bank > 0) {
		// バンク1は先頭13バイトに有効
		if (addr <= 0x0c) {
			addr += 0x10;
		}
	}

	switch (addr) {
		// 秒(1)
		case 0x00:
			data = rtc.sec % 10;
			return (0xf0 | data);

		// 秒(10)
		case 0x01:
			data = rtc.sec / 10;
			return (0xf0 | data);

		// 分(1)
		case 0x02:
			data = rtc.min % 10;
			return (0xf0 | data);

		// 分(10)
		case 0x03:
			data = rtc.min / 10;
			return (0xf0 | data);

		// 時(1)
		case 0x04:
			// 12h,24hで分ける
			data = rtc.hour;
			if (!rtc.fullhour) {
				data %= 12;
			}
			data %= 10;
			return (0xf0 | data);

		// 時(10)
		case 0x05:
			// 12h,24hで分ける。12hのPMはb1を立てる
			data = rtc.hour;
			if (!rtc.fullhour) {
				data %= 12;
			}
			data /= 10;
			if (!rtc.fullhour && (rtc.hour >= 12)) {
				data |= 0x02;
			}
			return (0xf0 | data);

		// 曜日
		case 0x06:
			return (0xf0 | rtc.week);

		// 日(1)
		case 0x07:
			data = rtc.day % 10;
			return (0xf0 | data);

		// 日(10)
		case 0x08:
			data = rtc.day / 10;
			return (0xf0 | data);

		// 月(1)
		case 0x09:
			data = rtc.month % 10;
			return (0xf0 | data);

		// 月(10)
		case 0x0a:
			data = rtc.month / 10;
			return (0xf0 | data);

		// 年(1)
		case 0x0b:
			data = rtc.year % 10;
			return (0xf0 | data);

		// 年(10)
		case 0x0c:
			data = rtc.year / 10;
			return (0xf0 | data);

		// MODEレジスタ
		case 0x0d:
			data = 0xf0;
			if (rtc.timer_en) {
				data |= 0x08;
			}
			if (rtc.alarm_en) {
				data |= 0x04;
			}
			if (rtc.bank > 0) {
				data |= 0x01;
			}
			return data;

		// TESTレジスタ(Write Only)
		case 0x0e:
			return 0xf0;

		// RESETレジスタ(Write onlyではない？Si.x v2);
		case 0x0f:
			data = 0xf0;
			if (!rtc.alarm_1hz) {
				data |= 0x08;
			}
			if (!rtc.alarm_16hz) {
				data |= 0x04;
			}
			return data;

		// CLKOUTレジスタ
		case 0x10:
			ASSERT(rtc.clkout < 0x08);
			return (0xf0 | rtc.clkout);

		// ADJUSTレジスタ
		case 0x11:
			return (0xf0 | rtc.adjust);

		// アラーム分(1)
		case 0x12:
			data = rtc.alarm_min % 10;
			return (0xf0 | data);

		// アラーム分(10)
		case 0x13:
			data = rtc.alarm_min / 10;
			return (0xf0 | data);

		// アラーム時(1)
		case 0x14:
			// 12h,24hで分ける
			data = rtc.alarm_hour;
			if (!rtc.fullhour) {
				data %= 12;
			}
			data %= 10;
			return (0xf0 | data);

		// アラーム時(10)
		case 0x15:
			// 12h,24hで分ける。12hのPMはb1を立てる
			data = rtc.alarm_hour;
			if (!rtc.fullhour) {
				data %= 12;
			}
			data /= 10;
			if (!rtc.fullhour && (rtc.alarm_hour >= 12)) {
				data |= 0x02;
			}
			return (0xf0 | data);

		// アラーム曜日
		case 0x16:
			return (0xf0 | rtc.alarm_week);

		// アラーム日(1)
		case 0x17:
			data = rtc.alarm_day % 10;
			return (0xf0 | data);

		// アラーム日(10)
		case 0x18:
			data = rtc.alarm_day / 10;
			return (0xf0 | data);

		// 12h,24h切り替え
		case 0x1a:
			if (rtc.fullhour) {
				return 0xf1;
			}
			return 0xf0;

		// 閏年カウンタ
		case 0x1b:
			ASSERT(rtc.leap <= 3);
			return (0xf0 | rtc.leap);
	}

	return 0xff;
}

//---------------------------------------------------------------------------
//
//	内部データ取得
//
//---------------------------------------------------------------------------
void FASTCALL RTC::GetRTC(rtc_t *buffer)
{
	ASSERT(this);
	ASSERT(buffer);

	// 内部データをコピー
	*buffer = rtc;
}

//---------------------------------------------------------------------------
//
//	イベントコールバック
//
//---------------------------------------------------------------------------
BOOL FASTCALL RTC::Callback(Event* /*ev*/)
{
	ASSERT(this);

	// ブリンク処理
	rtc.signal_blink--;
	if (rtc.signal_blink == 0) {
		rtc.signal_blink = 25;
	}

	// 信号を反転
	rtc.signal_16hz = !(rtc.signal_16hz);

	// 16回おきに処理
	rtc.signal_count++;
	if (rtc.signal_count < 0x10) {
		// ALARM信号の合成と出力
		AlarmOut();
		return TRUE;
	}
	rtc.signal_count = 0;

	// 1Hz信号を反転
	rtc.signal_1hz = !(rtc.signal_1hz);

	// ALARM信号の合成と出力
	AlarmOut();

	// TRUE→FALSE(立ち下がり)で処理(Si.x v2)
	if (rtc.signal_1hz) {
		return TRUE;
	}

	// タイマーdisableなら、キャリーをセットして終了
	if (!rtc.timer_en) {
		rtc.carry = TRUE;
		return TRUE;
	}

	// 秒アップ
	SecUp();

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	現在時刻をセット
//
//---------------------------------------------------------------------------
void FASTCALL RTC::Adjust(BOOL alarm)
{
	time_t ltime;
	struct tm *now;
	int leap;

    ASSERT(this);

	// 時刻を取得
	ltime = time(NULL);
	now = localtime(&ltime);

	// 変換
	rtc.year = (now->tm_year + 20) % 100;
	rtc.month = now->tm_mon + 1;
	rtc.day = now->tm_mday;
	rtc.week = now->tm_wday;
	rtc.hour = now->tm_hour;
	rtc.min = now->tm_min;
	rtc.sec = now->tm_sec;

	// leapを計算(2100年には未対応)
	leap = now->tm_year;
	leap %= 4;
	rtc.leap = leap;

	// アラームには同じ時間を設定
	if (alarm) {
		rtc.alarm_min = rtc.min;
		rtc.alarm_hour = rtc.hour;
		rtc.alarm_week = rtc.week;
		rtc.alarm_day = rtc.day;
	}
}

//---------------------------------------------------------------------------
//
//	アラーム合成
//
//---------------------------------------------------------------------------
void FASTCALL RTC::AlarmOut()
{
	BOOL flag;

	flag = FALSE;

	// アラーム合成
	if (rtc.alarm_en) {
		flag = rtc.alarm;
	}

	// 1Hz合成
	if (rtc.alarm_1hz) {
		flag = rtc.signal_1hz;
	}

	// 16Hz合成
	if (rtc.alarm_16hz) {
		flag = rtc.signal_16hz;
	}

	// 記憶
	rtc.alarmout = flag;

	// MFPへ通知
	if (flag) {
		mfp->SetGPIP(0, 0);
	}
	else {
		mfp->SetGPIP(0, 1);
	}
}

//---------------------------------------------------------------------------
//
//	アラーム信号取得
//
//---------------------------------------------------------------------------
BOOL FASTCALL RTC::GetAlarmOut() const
{
	ASSERT(this);

	return rtc.alarmout;
}

//---------------------------------------------------------------------------
//
//	FDD用点滅信号取得
//
//---------------------------------------------------------------------------
BOOL FASTCALL RTC::GetBlink(int drive) const
{
	ASSERT(this);
	ASSERT((drive == 0) || (drive == 1));

	// ドライブ0、ドライブ1で分ける
	if (drive == 0) {
		if (rtc.signal_blink >  13) {
			return FALSE;
		}
		return TRUE;
	}

	// 0～25まで動くので、1/4で
	if ((rtc.signal_blink >  6) && (rtc.signal_blink < 19)) {
		return FALSE;
	}
	return TRUE;
}

//---------------------------------------------------------------------------
//
//	タイマーLEDを取得
//
//---------------------------------------------------------------------------
BOOL FASTCALL RTC::GetTimerLED() const
{
	BOOL led;

	ASSERT(this);
	ASSERT(rtc.clkout <= 7);

	// 0(Hレベル)から3(128Hz)までは常に点灯とみなす
	led = TRUE;
	if (rtc.clkout <= 3) {
		return led;
	}

	switch (rtc.clkout) {
		// 16Hz
		case 4:
			led = rtc.signal_16hz;
			break;

		// 1Hz
		case 5:
			led = rtc.signal_1hz;
			break;

		// 1/60Hz(secが0～29, 30～59で分ける)
		case 6:
			if (rtc.sec < 30) {
				led = TRUE;
			}
			else {
				led = FALSE;
			}
			break;

		// Lレベル
		case 7:
			led = FALSE;
			break;
	}

	return led;
}

//---------------------------------------------------------------------------
//
//	秒アップ
//
//---------------------------------------------------------------------------
void FASTCALL RTC::SecUp()
{
	ASSERT(this);

	// 真っ先にキャリーを降ろす
	rtc.carry = FALSE;

	// カウントアップ
	rtc.sec++;

	// 60でなければ終了
	if (rtc.sec < 60) {
		return;
	}

	// 分アップへ引き継ぐ
	MinUp();
}

//---------------------------------------------------------------------------
//
//	分アップ
//
//---------------------------------------------------------------------------
void FASTCALL RTC::MinUp()
{
	ASSERT(this);

	// 秒クリア
	rtc.sec = 0;

	// カウントアップ
	rtc.min++;

	// 60でなければアラームチェックして終了
	if (rtc.min < 60) {
		AlarmCheck();
		return;
	}

	// 時間アップ
	rtc.min = 0;
	rtc.hour++;

	// 24でなければアラームチェックして終了
	if (rtc.hour < 24) {
		AlarmCheck();
		return;
	}

	// 日アップ
	rtc.hour = 0;
	rtc.day++;
	rtc.week++;
	if (rtc.week >= 7) {
		rtc.week = 0;
	}

	// 月エンドチェック
	if (rtc.day <= DayTable[rtc.month]) {
		// まだ月の残りがある。アラームチェックして終了
		AlarmCheck();
		return;
	}

	// 2月は閏年をチェック
	if ((rtc.month == 2) && (rtc.day == 29)) {
		// leapが0なら閏年なので、次の月に行ってはいけない
		if (rtc.leap == 0) {
			AlarmCheck();
			return;
		}
	}

	// 月アップ
	rtc.day = 1;
	rtc.month++;

	// 13でなければアラームチェックして終了
	if (rtc.hour < 13) {
		AlarmCheck();
		return;
	}

	// 翌年
	rtc.month = 1;
	rtc.year++;
	AlarmCheck();
}

//---------------------------------------------------------------------------
//
//	アラームチェック
//
//---------------------------------------------------------------------------
void FASTCALL RTC::AlarmCheck()
{
	BOOL flag;

	flag = TRUE;

	// 分、時、曜日、日がすべて一致するとフラグアップ
	if (rtc.alarm_min != rtc.min) {
		flag = FALSE;
	}
	if (rtc.alarm_hour != rtc.hour) {
		flag = FALSE;
	}
	if (rtc.alarm_week != rtc.week) {
		flag = FALSE;
	}
	if (rtc.alarm_day != rtc.day) {
		flag = FALSE;
	}
	rtc.alarm = flag;

	AlarmOut();
}

//---------------------------------------------------------------------------
//
//	日付テーブル
//
//---------------------------------------------------------------------------
const DWORD RTC::DayTable[] = {
	31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
};
