//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 �o�h�D(ytanaka@ipc-tokai.or.jp)
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
		DWORD sec;						// �b
		DWORD min;						// ��
		DWORD hour;						// ����
		DWORD week;						// �T�̗j��
		DWORD day;						// ��
		DWORD month;					// ��
		DWORD year;						// �N
		BOOL carry;						// �b�L�����[

		BOOL timer_en;					// �^�C�}�[���싖��
		BOOL alarm_en;					// �A���[�����싖��
		DWORD bank;						// �o���N�ԍ�
		DWORD test;						// TEST���W�X�^
		BOOL alarm_1hz;					// 1Hz�p���X�o�͐���
		BOOL alarm_16hz;				// 16Hz�p���X�o�͐���
		BOOL under_reset;				// �b�A���_�[���Z�b�g
		BOOL alarm_reset;				// �A���[�����Z�b�g

		DWORD clkout;					// CLKOUT���W�X�^
		BOOL adjust;					// �A�W���X�g

		DWORD alarm_min;				// ��
		DWORD alarm_hour;				// ����
		DWORD alarm_week;				// �T�̗j��
		DWORD alarm_day;				// ��

		BOOL fullhour;					// 24���ԃt���O
		DWORD leap;						// �[�N�J�E���^

		BOOL signal_1hz;				// 1Hz�V�O�i��(500ms�����ɕω�)
		BOOL signal_16hz;				// 16Hz�V�O�i��(31.25ms�����ɕω�)
		DWORD signal_count;				// 16Hz�J�E���^(0�`15)
		DWORD signal_blink;				// �_�ŃV�O�i��(781.25ms�����ɕω�)
		BOOL alarm;						// �A���[���M��
		BOOL alarmout;					// ALARM OUT
	} rtc_t;

public:
	// ��{�t�@���N�V����
	RTC(VM *p);
										// �R���X�g���N�^
	BOOL FASTCALL Init();
										// ������
	void FASTCALL Cleanup();
										// �N���[���A�b�v
	void FASTCALL Reset();
										// ���Z�b�g
	BOOL FASTCALL Save(Fileio *fio, int ver);
										// �Z�[�u
	BOOL FASTCALL Load(Fileio *fio, int ver);
										// ���[�h
	void FASTCALL ApplyCfg(const Config *config);
										// �ݒ�K�p

	// �������f�o�C�X
	DWORD FASTCALL ReadByte(DWORD addr);
										// �o�C�g�ǂݍ���
	DWORD FASTCALL ReadWord(DWORD addr);
										// ���[�h�ǂݍ���
	void FASTCALL WriteByte(DWORD addr, DWORD data);
										// �o�C�g��������
	void FASTCALL WriteWord(DWORD addr, DWORD data);
										// ���[�h��������
	DWORD FASTCALL ReadOnly(DWORD addr) const;
										// �ǂݍ��݂̂�

	// �O��API
	void FASTCALL GetRTC(rtc_t *buffer);
										// �����f�[�^�擾
	BOOL FASTCALL Callback(Event *ev);
										// �C�x���g�R�[���o�b�N
	BOOL FASTCALL GetTimerLED() const;
										// �^�C�}�[LED�擾
	BOOL FASTCALL GetAlarmOut() const;
										// ALARM�M���擾
	BOOL FASTCALL GetBlink(int drive) const;
										// FDD�p�_�ŐM���擾
	void FASTCALL Adjust(BOOL alarm);
										// ���ݎ�����ݒ�

private:
	void FASTCALL AlarmOut();
										// ALARM�M���o��
	void FASTCALL SecUp();
										// �b�A�b�v
	void FASTCALL MinUp();
										// ���A�b�v
	void FASTCALL AlarmCheck();
										// �A���[���`�F�b�N
	MFP *mfp;
										// MFP
	rtc_t rtc;
										// �����f�[�^
	Event event;
										// �C�x���g
	static const DWORD DayTable[];
										// ���t�e�[�u��
};

#endif	// rtc_h
