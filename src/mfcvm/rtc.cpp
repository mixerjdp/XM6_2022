//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2005 �o�h�D(ytanaka@ipc-tokai.or.jp)
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
//	�R���X�g���N�^
//
//---------------------------------------------------------------------------
RTC::RTC(VM *p) : MemDevice(p)
{
	// �f�o�C�XID��������
	dev.id = MAKEID('R', 'T', 'C', ' ');
	dev.desc = "RTC (RP5C15)";

	// �J�n�A�h���X�A�I���A�h���X
	memdev.first = 0xe8a000;
	memdev.last = 0xe8bfff;

	// ���[�N�N���A
	mfp = NULL;
}

//---------------------------------------------------------------------------
//
//	������
//
//---------------------------------------------------------------------------
BOOL FASTCALL RTC::Init()
{
	ASSERT(this);

	// ��{�N���X
	if (!MemDevice::Init()) {
		return FALSE;
	}

	// MFP���擾
	ASSERT(!mfp);
	mfp = (MFP*)vm->SearchDevice(MAKEID('M', 'F', 'P', ' '));
	ASSERT(mfp);

	// �C�x���g���쐬(32Hz)
	event.SetDevice(this);
	event.SetDesc("Clock 16Hz");
	event.SetUser(0);
	event.SetTime(62500);
	scheduler->AddEvent(&event);

	// �������W�X�^���N���A
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

	// ���ݎ�����ݒ�
	Adjust(TRUE);

	// MFP�֒ʒm(ALARM�M���I�t)
	mfp->SetGPIP(0, 1);

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	�N���[���A�b�v
//
//---------------------------------------------------------------------------
void FASTCALL RTC::Cleanup()
{
	ASSERT(this);

	// ��{�N���X��
	MemDevice::Cleanup();
}

//---------------------------------------------------------------------------
//
//	���Z�b�g
//
//---------------------------------------------------------------------------
void FASTCALL RTC::Reset()
{
	ASSERT(this);
	LOG0(Log::Normal, "���Z�b�g");
}

//---------------------------------------------------------------------------
//
//	�Z�[�u
//
//---------------------------------------------------------------------------
BOOL FASTCALL RTC::Save(Fileio *fio, int ver)
{
	size_t sz;

	ASSERT(this);
	ASSERT(fio);

	LOG0(Log::Normal, "�Z�[�u");

	// �T�C�Y���Z�[�u
	sz = sizeof(rtc_t);
	if (!fio->Write(&sz, sizeof(sz))) {
		return FALSE;
	}

	// �{�̂��Z�[�u
	if (!fio->Write(&rtc, (int)sz)) {
		return FALSE;
	}

	// �C�x���g���Z�[�u
	if (!event.Save(fio, ver)) {
		return FALSE;
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	���[�h
//
//---------------------------------------------------------------------------
BOOL FASTCALL RTC::Load(Fileio *fio, int ver)
{
	size_t sz;

	ASSERT(this);
	ASSERT(fio);

	LOG0(Log::Normal, "���[�h");

	// �T�C�Y�����[�h�A�ƍ�
	if (!fio->Read(&sz, sizeof(sz))) {
		return FALSE;
	}
	if (sz != sizeof(rtc_t)) {
		return FALSE;
	}

	// �{�̂����[�h
	if (!fio->Read(&rtc, (int)sz)) {
		return FALSE;
	}

	// �C�x���g�����[�h
	if (!event.Load(fio, ver)) {
		return FALSE;
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	�ݒ�K�p
//
//---------------------------------------------------------------------------
void FASTCALL RTC::ApplyCfg(const Config* /*config*/)
{
	ASSERT(this);

	LOG0(Log::Normal, "�ݒ�K�p");
}

//---------------------------------------------------------------------------
//
//	�o�C�g�ǂݍ���
//
//---------------------------------------------------------------------------
DWORD FASTCALL RTC::ReadByte(DWORD addr)
{
	DWORD data;

	ASSERT(this);
	ASSERT((addr >= memdev.first) && (addr <= memdev.last));

	// ��A�h���X�̂݃f�R�[�h����Ă���
	if ((addr & 1) == 0) {
		// ROM IOCS�Ń��[�h�ǂݏo�����s���Ă���B�o�X�G���[�͔������Ȃ�
		return 0xff;
	}

	// 32�o�C�g�P�ʂŃ��[�v
	addr &= 0x1f;

	// �E�F�C�g
	scheduler->Wait(1);

	// ���W�X�^�ʂɕ���(���W�X�^0�`���W�X�^28)
	addr >>= 1;
	ASSERT((rtc.bank == 0) || (rtc.bank == 1));
	if (rtc.bank > 0) {
		// �o���N1�͐擪13�o�C�g�ɗL��
		if (addr <= 0x0c) {
			addr += 0x10;
		}
	}

	switch (addr) {
		// �b(1)
		case 0x00:
			data = rtc.sec % 10;
			return (0xf0 | data);

		// �b(10)
		case 0x01:
			data = rtc.sec / 10;
			return (0xf0 | data);

		// ��(1)
		case 0x02:
			data = rtc.min % 10;
			return (0xf0 | data);

		// ��(10)
		case 0x03:
			data = rtc.min / 10;
			return (0xf0 | data);

		// ��(1)
		case 0x04:
			// 12h,24h�ŕ�����
			data = rtc.hour;
			if (!rtc.fullhour) {
				data %= 12;
			}
			data %= 10;
			return (0xf0 | data);

		// ��(10)
		case 0x05:
			// 12h,24h�ŕ�����B12h��PM��b1�𗧂Ă�
			data = rtc.hour;
			if (!rtc.fullhour) {
				data %= 12;
			}
			data /= 10;
			if (!rtc.fullhour && (rtc.hour >= 12)) {
				data |= 0x02;
			}
			return (0xf0 | data);

		// �j��
		case 0x06:
			return (0xf0 | rtc.week);

		// ��(1)
		case 0x07:
			data = rtc.day % 10;
			return (0xf0 | data);

		// ��(10)
		case 0x08:
			data = rtc.day / 10;
			return (0xf0 | data);

		// ��(1)
		case 0x09:
			data = rtc.month % 10;
			return (0xf0 | data);

		// ��(10)
		case 0x0a:
			data = rtc.month / 10;
			return (0xf0 | data);

		// �N(1)
		case 0x0b:
			data = rtc.year % 10;
			return (0xf0 | data);

		// �N(10)
		case 0x0c:
			data = rtc.year / 10;
			return (0xf0 | data);

		// MODE���W�X�^
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

		// TEST���W�X�^(Write Only)
		case 0x0e:
			return 0xf0;

		// RESET���W�X�^(Write only�ł͂Ȃ��HSi.x v2);
		case 0x0f:
			data = 0xf0;
			if (!rtc.alarm_1hz) {
				data |= 0x08;
			}
			if (!rtc.alarm_16hz) {
				data |= 0x04;
			}
			return data;

		// CLKOUT���W�X�^
		case 0x10:
			ASSERT(rtc.clkout < 0x08);
			return (0xf0 | rtc.clkout);

		// ADJUST���W�X�^
		case 0x11:
			return (0xf0 | rtc.adjust);

		// �A���[����(1)
		case 0x12:
			data = rtc.alarm_min % 10;
			return (0xf0 | data);

		// �A���[����(10)
		case 0x13:
			data = rtc.alarm_min / 10;
			return (0xf0 | data);

		// �A���[����(1)
		case 0x14:
			// 12h,24h�ŕ�����
			data = rtc.alarm_hour;
			if (!rtc.fullhour) {
				data %= 12;
			}
			data %= 10;
			return (0xf0 | data);

		// �A���[����(10)
		case 0x15:
			// 12h,24h�ŕ�����B12h��PM��b1�𗧂Ă�
			data = rtc.alarm_hour;
			if (!rtc.fullhour) {
				data %= 12;
			}
			data /= 10;
			if (!rtc.fullhour && (rtc.alarm_hour >= 12)) {
				data |= 0x02;
			}
			return (0xf0 | data);

		// �A���[���j��
		case 0x16:
			return (0xf0 | rtc.alarm_week);

		// �A���[����(1)
		case 0x17:
			data = rtc.alarm_day % 10;
			return (0xf0 | data);

		// �A���[����(10)
		case 0x18:
			data = rtc.alarm_day / 10;
			return (0xf0 | data);

		// 12h,24h�؂�ւ�
		case 0x1a:
			if (rtc.fullhour) {
				return 0xf1;
			}
			return 0xf0;

		// �[�N�J�E���^
		case 0x1b:
			ASSERT(rtc.leap <= 3);
			return (0xf0 | rtc.leap);
	}

	LOG1(Log::Warning, "���������W�X�^�ǂݍ��� R%02d", addr);
	return 0xff;
}

//---------------------------------------------------------------------------
//
//	���[�h�ǂݍ���
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
//	�o�C�g��������
//
//---------------------------------------------------------------------------
void FASTCALL RTC::WriteByte(DWORD addr, DWORD data)
{
	ASSERT(this);
	ASSERT((addr >= memdev.first) && (addr <= memdev.last));
	ASSERT(data < 0x100);

	// ��A�h���X�̂݃f�R�[�h����Ă���
	if ((addr & 1) == 0) {
		// �o�X�G���[�͔������Ȃ�
		return;
	}

	// 32�o�C�g�P�ʂŃ��[�v
	addr &= 0x1f;

	// �E�F�C�g
	scheduler->Wait(1);

	// ���W�X�^�ʂɕ���(���W�X�^0�`���W�X�^28)
	addr >>= 1;
	ASSERT((rtc.bank == 0) || (rtc.bank == 1));
	if (rtc.bank > 0) {
		// �o���N1�͐擪13�o�C�g�ɗL��
		if (addr <= 0x0c) {
			addr += 0x10;
		}
	}

	switch (addr) {
		// �b(1)
		case 0x00:
			data &= 0x0f;
			rtc.sec /= 10;
			rtc.sec *= 10;
			rtc.sec += data;
			return;

		// �b(10)
		case 0x01:
			data &= 0x07;
			data *= 10;
			rtc.sec %= 10;
			rtc.sec += data;
			return;

		// ��(1)
		case 0x02:
			data &= 0x0f;
			rtc.min /= 10;
			rtc.min *= 10;
			rtc.min += data;
			return;

		// ��(10)
		case 0x03:
			data &= 0x07;
			data *= 10;
			rtc.min %= 10;
			rtc.min += data;
			return;

		// ��(1)
		case 0x04:
			data &= 0x0f;
			// 12h,24h�ŕ�����
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

		// ��(10)
		case 0x05:
			data &= 0x03;
			// 12h,24h�ŕ�����
			if (rtc.fullhour) {
				// 24h
				data *= 10;
				rtc.hour %= 10;
				rtc.hour += data;
			}
			else {
				if (data & 0x02) {
					// 12h,PM�����I��(�f�[�^�V�[�g�ɂ��)
					data &= 0x01;
					data *= 10;
					rtc.hour %= 10;
					rtc.hour += data;
					rtc.hour += 12;
				}
				else {
					// 12h,AM�����I��(�f�[�^�V�[�g�ɂ��)
					data &= 0x01;
					data *= 10;
					rtc.hour %= 10;
					rtc.hour += data;
				}
			}
			return;

		// �j��
		case 0x06:
			data &= 0x07;
			rtc.week = data;
			return;

		// ��(1)
		case 0x07:
			data &= 0x0f;
			rtc.day /= 10;
			rtc.day *= 10;
			rtc.day += data;
			return;

		// ��(10)
		case 0x08:
			data &= 0x03;
			data *= 10;
			rtc.day %= 10;
			rtc.day += data;
			return;

		// ��(1)
		case 0x09:
			data &= 0x0f;
			rtc.month /= 10;
			rtc.month *= 10;
			rtc.month += data;
			return;

		// ��(10)
		case 0x0a:
			data &= 0x01;
			data *= 10;
			rtc.month %= 10;
			rtc.month += data;
			return;

		// �N(1)
		case 0x0b:
			data &= 0x0f;
			rtc.year /= 10;
			rtc.year *= 10;
			rtc.year += data;
			return;

		// �N(10)
		case 0x0c:
			data &= 0x0f;
			data *= 10;
			rtc.year %= 10;
			rtc.year += data;
			return;

		// MODE���W�X�^
		case 0x0d:
			// �^�C�}�C�l�[�u��
			if (data & 0x08) {
				rtc.timer_en = TRUE;
				// �L�����[���c���Ă���΁A�͂�����
				if (rtc.carry) {
					SecUp();
				}
			}
			else {
				rtc.timer_en = FALSE;
			}

			// �A���[���C�l�[�u��
			if (data & 0x04) {
				rtc.alarm_en = TRUE;
			}
			else {
				rtc.alarm_en = FALSE;
			}
			AlarmOut();

			// �o���N�Z���N�g
			if (data & 0x01) {
				rtc.bank = 1;
			}
			else {
				rtc.bank = 0;
			}
			return;

		// TEST���W�X�^
		case 0x0e:
			rtc.test = (data & 0x0f);
			if (rtc.test != 0) {
				LOG1(Log::Warning, "�e�X�g���[�h�ݒ� $%02X", rtc.test);
			}
			return;

		// RESET���W�X�^
		case 0x0f:
			// 1Hz, 16Hz�p���X�o��
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
				// �C�x���g�����Z�b�g����
				event.SetTime(event.GetTime());
			}
			else {
				rtc.under_reset = FALSE;
			}
			if (data & 0x01) {
				// ALARM�M����H���x���ɂ������
				rtc.alarm_reset = TRUE;
				rtc.alarm = FALSE;
				AlarmOut();
				// ���ݎ����ƈ�v������
				rtc.alarm_min = rtc.min;
				rtc.alarm_hour = rtc.hour;
				rtc.alarm_week = rtc.week;
				rtc.alarm_day = rtc.day;
			}
			else {
				rtc.alarm_reset = FALSE;
			}
			return;

		// CLKOUT���W�X�^
		case 0x10:
			rtc.clkout = (data & 0x07);
#if defined(RTC_LOG)
			LOG1(Log::Normal, "CLKOUT�Z�b�g %d", rtc.clkout);
#endif	// RTC_LOG
			return;

		// ADJ���W�X�^
		case 0x11:
			rtc.adjust = (data & 0x01);
			if (data & 0x01) {
				// 1�ŕb�A�W���X�g
				if (rtc.sec < 30) {
					// �؂�̂�
					rtc.sec = 0;
				}
				else {
					// �؂�グ
					MinUp();
				}
			}
			return;

		// �A���[����(1)
		case 0x12:
			data &= 0x0f;
			rtc.alarm_min /= 10;
			rtc.alarm_min *= 10;
			rtc.alarm_min += data;
			return;

		// �A���[����(10)
		case 0x13:
			data &= 0x07;
			data *= 10;
			rtc.alarm_min %= 10;
			rtc.alarm_min += data;
			return;

		// �A���[����(1)
		case 0x14:
			data &= 0x0f;
			// 12h,24h�ŕ�����
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

		// �A���[����(10)
		case 0x15:
			data &= 0x03;
			// 12h,24h�ŕ�����
			if (rtc.fullhour) {
				// 24h
				data *= 10;
				rtc.alarm_hour %= 10;
				rtc.alarm_hour += data;
			}
			else {
				if (data & 0x02) {
					// 12h,PM�����I��(�f�[�^�V�[�g�ɂ��)
					data &= 0x01;
					data *= 10;
					rtc.alarm_hour %= 10;
					rtc.alarm_hour += data;
					rtc.alarm_hour += 12;
				}
				else {
					// 12h,AM�����I��(�f�[�^�V�[�g�ɂ��)
					data &= 0x01;
					data *= 10;
					rtc.alarm_hour %= 10;
					rtc.alarm_hour += data;
				}
			}
			return;

		// �A���[���j��
		case 0x16:
			data &= 0x07;
			rtc.alarm_week = data;
			return;

		// �A���[����(1)
		case 0x17:
			data &= 0x0f;
			rtc.alarm_day /= 10;
			rtc.alarm_day *= 10;
			rtc.alarm_day += data;
			return;

		// �A���[����(10)
		case 0x18:
			data &= 0x03;
			data *= 10;
			rtc.alarm_day %= 10;
			rtc.alarm_day += data;
			return;

		// 12h,24h�؂�ւ�
		case 0x1a:
			if (data & 0x01) {
				rtc.fullhour = TRUE;
#if defined(RTC_LOG)
				LOG0(Log::Normal, "24���Ԑ�");
#endif	// RTC_LOG
			}
			else {
				rtc.fullhour = FALSE;
#if defined(RTC_LOG)
				LOG0(Log::Normal, "12���Ԑ�");
#endif	// RTC_LOG
			}
			return;

		// �[�N�J�E���^
		case 0x1b:
			rtc.leap = (data & 0x03);
			return;
	}

	LOG2(Log::Warning, "���������W�X�^�������� R%02d <- $%02X",
							addr, data);
}

//---------------------------------------------------------------------------
//
//	���[�h��������
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
//	�ǂݍ��݂̂�
//
//---------------------------------------------------------------------------
DWORD FASTCALL RTC::ReadOnly(DWORD addr) const
{
	DWORD data;

	ASSERT(this);
	ASSERT((addr >= memdev.first) && (addr <= memdev.last));

	// ��A�h���X�̂݃f�R�[�h����Ă���
	if ((addr & 1) == 0) {
		return 0xff;
	}

	// 32�o�C�g�P�ʂŃ��[�v
	addr &= 0x1f;

	// ���W�X�^�ʂɕ���(���W�X�^0�`���W�X�^28)
	addr >>= 1;
	ASSERT((rtc.bank == 0) || (rtc.bank == 1));
	if (rtc.bank > 0) {
		// �o���N1�͐擪13�o�C�g�ɗL��
		if (addr <= 0x0c) {
			addr += 0x10;
		}
	}

	switch (addr) {
		// �b(1)
		case 0x00:
			data = rtc.sec % 10;
			return (0xf0 | data);

		// �b(10)
		case 0x01:
			data = rtc.sec / 10;
			return (0xf0 | data);

		// ��(1)
		case 0x02:
			data = rtc.min % 10;
			return (0xf0 | data);

		// ��(10)
		case 0x03:
			data = rtc.min / 10;
			return (0xf0 | data);

		// ��(1)
		case 0x04:
			// 12h,24h�ŕ�����
			data = rtc.hour;
			if (!rtc.fullhour) {
				data %= 12;
			}
			data %= 10;
			return (0xf0 | data);

		// ��(10)
		case 0x05:
			// 12h,24h�ŕ�����B12h��PM��b1�𗧂Ă�
			data = rtc.hour;
			if (!rtc.fullhour) {
				data %= 12;
			}
			data /= 10;
			if (!rtc.fullhour && (rtc.hour >= 12)) {
				data |= 0x02;
			}
			return (0xf0 | data);

		// �j��
		case 0x06:
			return (0xf0 | rtc.week);

		// ��(1)
		case 0x07:
			data = rtc.day % 10;
			return (0xf0 | data);

		// ��(10)
		case 0x08:
			data = rtc.day / 10;
			return (0xf0 | data);

		// ��(1)
		case 0x09:
			data = rtc.month % 10;
			return (0xf0 | data);

		// ��(10)
		case 0x0a:
			data = rtc.month / 10;
			return (0xf0 | data);

		// �N(1)
		case 0x0b:
			data = rtc.year % 10;
			return (0xf0 | data);

		// �N(10)
		case 0x0c:
			data = rtc.year / 10;
			return (0xf0 | data);

		// MODE���W�X�^
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

		// TEST���W�X�^(Write Only)
		case 0x0e:
			return 0xf0;

		// RESET���W�X�^(Write only�ł͂Ȃ��HSi.x v2);
		case 0x0f:
			data = 0xf0;
			if (!rtc.alarm_1hz) {
				data |= 0x08;
			}
			if (!rtc.alarm_16hz) {
				data |= 0x04;
			}
			return data;

		// CLKOUT���W�X�^
		case 0x10:
			ASSERT(rtc.clkout < 0x08);
			return (0xf0 | rtc.clkout);

		// ADJUST���W�X�^
		case 0x11:
			return (0xf0 | rtc.adjust);

		// �A���[����(1)
		case 0x12:
			data = rtc.alarm_min % 10;
			return (0xf0 | data);

		// �A���[����(10)
		case 0x13:
			data = rtc.alarm_min / 10;
			return (0xf0 | data);

		// �A���[����(1)
		case 0x14:
			// 12h,24h�ŕ�����
			data = rtc.alarm_hour;
			if (!rtc.fullhour) {
				data %= 12;
			}
			data %= 10;
			return (0xf0 | data);

		// �A���[����(10)
		case 0x15:
			// 12h,24h�ŕ�����B12h��PM��b1�𗧂Ă�
			data = rtc.alarm_hour;
			if (!rtc.fullhour) {
				data %= 12;
			}
			data /= 10;
			if (!rtc.fullhour && (rtc.alarm_hour >= 12)) {
				data |= 0x02;
			}
			return (0xf0 | data);

		// �A���[���j��
		case 0x16:
			return (0xf0 | rtc.alarm_week);

		// �A���[����(1)
		case 0x17:
			data = rtc.alarm_day % 10;
			return (0xf0 | data);

		// �A���[����(10)
		case 0x18:
			data = rtc.alarm_day / 10;
			return (0xf0 | data);

		// 12h,24h�؂�ւ�
		case 0x1a:
			if (rtc.fullhour) {
				return 0xf1;
			}
			return 0xf0;

		// �[�N�J�E���^
		case 0x1b:
			ASSERT(rtc.leap <= 3);
			return (0xf0 | rtc.leap);
	}

	return 0xff;
}

//---------------------------------------------------------------------------
//
//	�����f�[�^�擾
//
//---------------------------------------------------------------------------
void FASTCALL RTC::GetRTC(rtc_t *buffer)
{
	ASSERT(this);
	ASSERT(buffer);

	// �����f�[�^���R�s�[
	*buffer = rtc;
}

//---------------------------------------------------------------------------
//
//	�C�x���g�R�[���o�b�N
//
//---------------------------------------------------------------------------
BOOL FASTCALL RTC::Callback(Event* /*ev*/)
{
	ASSERT(this);

	// �u�����N����
	rtc.signal_blink--;
	if (rtc.signal_blink == 0) {
		rtc.signal_blink = 25;
	}

	// �M���𔽓]
	rtc.signal_16hz = !(rtc.signal_16hz);

	// 16�񂨂��ɏ���
	rtc.signal_count++;
	if (rtc.signal_count < 0x10) {
		// ALARM�M���̍����Əo��
		AlarmOut();
		return TRUE;
	}
	rtc.signal_count = 0;

	// 1Hz�M���𔽓]
	rtc.signal_1hz = !(rtc.signal_1hz);

	// ALARM�M���̍����Əo��
	AlarmOut();

	// TRUE��FALSE(����������)�ŏ���(Si.x v2)
	if (rtc.signal_1hz) {
		return TRUE;
	}

	// �^�C�}�[disable�Ȃ�A�L�����[���Z�b�g���ďI��
	if (!rtc.timer_en) {
		rtc.carry = TRUE;
		return TRUE;
	}

	// �b�A�b�v
	SecUp();

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	���ݎ������Z�b�g
//
//---------------------------------------------------------------------------
void FASTCALL RTC::Adjust(BOOL alarm)
{
	time_t ltime;
	struct tm *now;
	int leap;

    ASSERT(this);

	// �������擾
	ltime = time(NULL);
	now = localtime(&ltime);

	// �ϊ�
	rtc.year = (now->tm_year + 20) % 100;
	rtc.month = now->tm_mon + 1;
	rtc.day = now->tm_mday;
	rtc.week = now->tm_wday;
	rtc.hour = now->tm_hour;
	rtc.min = now->tm_min;
	rtc.sec = now->tm_sec;

	// leap���v�Z(2100�N�ɂ͖��Ή�)
	leap = now->tm_year;
	leap %= 4;
	rtc.leap = leap;

	// �A���[���ɂ͓������Ԃ�ݒ�
	if (alarm) {
		rtc.alarm_min = rtc.min;
		rtc.alarm_hour = rtc.hour;
		rtc.alarm_week = rtc.week;
		rtc.alarm_day = rtc.day;
	}
}

//---------------------------------------------------------------------------
//
//	�A���[������
//
//---------------------------------------------------------------------------
void FASTCALL RTC::AlarmOut()
{
	BOOL flag;

	flag = FALSE;

	// �A���[������
	if (rtc.alarm_en) {
		flag = rtc.alarm;
	}

	// 1Hz����
	if (rtc.alarm_1hz) {
		flag = rtc.signal_1hz;
	}

	// 16Hz����
	if (rtc.alarm_16hz) {
		flag = rtc.signal_16hz;
	}

	// �L��
	rtc.alarmout = flag;

	// MFP�֒ʒm
	if (flag) {
		mfp->SetGPIP(0, 0);
	}
	else {
		mfp->SetGPIP(0, 1);
	}
}

//---------------------------------------------------------------------------
//
//	�A���[���M���擾
//
//---------------------------------------------------------------------------
BOOL FASTCALL RTC::GetAlarmOut() const
{
	ASSERT(this);

	return rtc.alarmout;
}

//---------------------------------------------------------------------------
//
//	FDD�p�_�ŐM���擾
//
//---------------------------------------------------------------------------
BOOL FASTCALL RTC::GetBlink(int drive) const
{
	ASSERT(this);
	ASSERT((drive == 0) || (drive == 1));

	// �h���C�u0�A�h���C�u1�ŕ�����
	if (drive == 0) {
		if (rtc.signal_blink >  13) {
			return FALSE;
		}
		return TRUE;
	}

	// 0�`25�܂œ����̂ŁA1/4��
	if ((rtc.signal_blink >  6) && (rtc.signal_blink < 19)) {
		return FALSE;
	}
	return TRUE;
}

//---------------------------------------------------------------------------
//
//	�^�C�}�[LED���擾
//
//---------------------------------------------------------------------------
BOOL FASTCALL RTC::GetTimerLED() const
{
	BOOL led;

	ASSERT(this);
	ASSERT(rtc.clkout <= 7);

	// 0(H���x��)����3(128Hz)�܂ł͏�ɓ_���Ƃ݂Ȃ�
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

		// 1/60Hz(sec��0�`29, 30�`59�ŕ�����)
		case 6:
			if (rtc.sec < 30) {
				led = TRUE;
			}
			else {
				led = FALSE;
			}
			break;

		// L���x��
		case 7:
			led = FALSE;
			break;
	}

	return led;
}

//---------------------------------------------------------------------------
//
//	�b�A�b�v
//
//---------------------------------------------------------------------------
void FASTCALL RTC::SecUp()
{
	ASSERT(this);

	// �^����ɃL�����[���~�낷
	rtc.carry = FALSE;

	// �J�E���g�A�b�v
	rtc.sec++;

	// 60�łȂ���ΏI��
	if (rtc.sec < 60) {
		return;
	}

	// ���A�b�v�ֈ����p��
	MinUp();
}

//---------------------------------------------------------------------------
//
//	���A�b�v
//
//---------------------------------------------------------------------------
void FASTCALL RTC::MinUp()
{
	ASSERT(this);

	// �b�N���A
	rtc.sec = 0;

	// �J�E���g�A�b�v
	rtc.min++;

	// 60�łȂ���΃A���[���`�F�b�N���ďI��
	if (rtc.min < 60) {
		AlarmCheck();
		return;
	}

	// ���ԃA�b�v
	rtc.min = 0;
	rtc.hour++;

	// 24�łȂ���΃A���[���`�F�b�N���ďI��
	if (rtc.hour < 24) {
		AlarmCheck();
		return;
	}

	// ���A�b�v
	rtc.hour = 0;
	rtc.day++;
	rtc.week++;
	if (rtc.week >= 7) {
		rtc.week = 0;
	}

	// ���G���h�`�F�b�N
	if (rtc.day <= DayTable[rtc.month]) {
		// �܂����̎c�肪����B�A���[���`�F�b�N���ďI��
		AlarmCheck();
		return;
	}

	// 2���͉[�N���`�F�b�N
	if ((rtc.month == 2) && (rtc.day == 29)) {
		// leap��0�Ȃ�[�N�Ȃ̂ŁA���̌��ɍs���Ă͂����Ȃ�
		if (rtc.leap == 0) {
			AlarmCheck();
			return;
		}
	}

	// ���A�b�v
	rtc.day = 1;
	rtc.month++;

	// 13�łȂ���΃A���[���`�F�b�N���ďI��
	if (rtc.hour < 13) {
		AlarmCheck();
		return;
	}

	// ���N
	rtc.month = 1;
	rtc.year++;
	AlarmCheck();
}

//---------------------------------------------------------------------------
//
//	�A���[���`�F�b�N
//
//---------------------------------------------------------------------------
void FASTCALL RTC::AlarmCheck()
{
	BOOL flag;

	flag = TRUE;

	// ���A���A�j���A�������ׂĈ�v����ƃt���O�A�b�v
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
//	���t�e�[�u��
//
//---------------------------------------------------------------------------
const DWORD RTC::DayTable[] = {
	31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
};
