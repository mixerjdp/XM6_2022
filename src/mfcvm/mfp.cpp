//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 �o�h�D(ytanaka@ipc-tokai.or.jp)
//	[ MFP(MC68901) ]
//
//---------------------------------------------------------------------------

#include "os.h"
#include "xm6.h"
#include "cpu.h"
#include "vm.h"
#include "log.h"
#include "event.h"
#include "schedule.h"
#include "keyboard.h"
#include "fileio.h"
#include "sync.h"
#include "mfp.h"

//===========================================================================
//
//	MFP
//
//===========================================================================
//#define MFP_LOG

//---------------------------------------------------------------------------
//
//	�R���X�g���N�^
//
//---------------------------------------------------------------------------
MFP::MFP(VM *p) : MemDevice(p)
{
	// �f�o�C�XID��������
	dev.id = MAKEID('M', 'F', 'P', ' ');
	dev.desc = "MFP (MC68901)";

	// �J�n�A�h���X�A�I���A�h���X
	memdev.first = 0xe88000;
	memdev.last = 0xe89fff;

	// Sync�I�u�W�F�N�g
	sync = NULL;
}

//---------------------------------------------------------------------------
//
//	������
//
//---------------------------------------------------------------------------
BOOL FASTCALL MFP::Init()
{
	int i;
	char buf[0x20];

	ASSERT(this);

	// ��{�N���X
	if (!MemDevice::Init()) {
		return FALSE;
	}

	// Sync�쐬
	sync = new Sync;

	// �^�C�}�C�x���g������
	for (i=0; i<4; i++) {
		timer[i].SetDevice(this);
		sprintf(buf, "Timer-%c", 'A' + i);
		timer[i].SetDesc(buf);
		timer[i].SetUser(i);
		timer[i].SetTime(0);

		// Timer-B�̓C�x���g�J�E���g�Ŏg���@��Ȃ����߁A�O��
		if (i != 1) {
			scheduler->AddEvent(&timer[i]);
		}
	}

	// �L�[�{�[�h���擾
	keyboard = (Keyboard*)vm->SearchDevice(MAKEID('K', 'E', 'Y', 'B'));

	// USART�C�x���g������
	// 1(us)x13(��)x(�f���[�e�B50%)x16(����)x10(bit)�Ŗ�2400bps
	usart.SetDevice(this);
	usart.SetUser(4);
	usart.SetDesc("USART 2400bps");
	usart.SetTime(8320);
	scheduler->AddEvent(&usart);

	// ���Z�b�g�ł͏���������Ȃ����W�X�^��ݒ�(�f�[�^�V�[�g3.3)
	for (i=0; i<4; i++) {
		if (i == 0) {
			// �^�C�}A��1�ɂ��āAVDISPST�������N����悤�ɂ���(DiskX)
			SetTDR(i, 1);
			mfp.tir[i] = 1;
		}
		else {
			// �^�C�}B,�^�C�}C,�^�C�}D��0
			SetTDR(i, 0);
			mfp.tir[i] = 0;
		}
	}
	mfp.tsr = 0;
	mfp.rur = 0;

	// ���Z�b�g���ɃL�[�{�[�h����$FF�����M����邽�߁AInit�ŏ�����
	sync->Lock();
	mfp.datacount = 0;
	mfp.readpoint = 0;
	mfp.writepoint = 0;
	sync->Unlock();

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	�N���[���A�b�v
//
//---------------------------------------------------------------------------
void FASTCALL MFP::Cleanup()
{
	ASSERT(this);

	// Sync�폜
	if (sync) {
		delete sync;
		sync = NULL;
	}

	// ��{�N���X��
	MemDevice::Cleanup();
}

//---------------------------------------------------------------------------
//
//	���Z�b�g
//
//---------------------------------------------------------------------------
void FASTCALL MFP::Reset()
{
	int i;

	ASSERT(this);
	LOG0(Log::Normal, "���Z�b�g");

	// ���荞�݃R���g���[��
	mfp.vr = 0;
	mfp.iidx = -1;
	for (i=0; i<0x10; i++) {
		mfp.ier[i] = FALSE;
		mfp.ipr[i] = FALSE;
		mfp.imr[i] = FALSE;
		mfp.isr[i] = FALSE;
		mfp.ireq[i] = FALSE;
	}

	// �^�C�}
	for (i=0; i<4; i++) {
		timer[i].SetTime(0);
		SetTCR(i, 0);
	}
	mfp.tbr[0] = 0;
	mfp.tbr[1] = 0;
	mfp.sram = 0;
	mfp.tecnt = 0;

	// GPIP (GPIP5�͏��H���x��)
	mfp.gpdr = 0;
	mfp.aer = 0;
	mfp.ddr = 0;
	mfp.ber = (DWORD)~mfp.aer;
	mfp.ber ^= mfp.gpdr;
	SetGPIP(5, 1);

	// USART
	mfp.scr = 0;
	mfp.ucr = 0;
	mfp.rsr = 0;
	mfp.tsr = (DWORD)(mfp.tsr & ~0x01);
	mfp.tur = 0;

	// GPIP������(�d���֘A)
	SetGPIP(1, 1);
	if (vm->IsPowerSW()) {
		SetGPIP(2, 0);
	}
	else {
		SetGPIP(2, 1);
	}
}

//---------------------------------------------------------------------------
//
//	�Z�[�u
//
//---------------------------------------------------------------------------
BOOL FASTCALL MFP::Save(Fileio *fio, int ver)
{
	size_t sz;
	int i;

	ASSERT(this);
	LOG0(Log::Normal, "�Z�[�u");

	// �{��
	sz = sizeof(mfp_t);
	if (!fio->Write(&sz, sizeof(sz))) {
		return FALSE;
	}
	if (!fio->Write(&mfp, (int)sz)) {
		return FALSE;
	}

	// �C�x���g(�^�C�})
	for (i=0; i<4; i++) {
		if (!timer[i].Save(fio, ver)) {
			return FALSE;
		}
	}

	// �C�x���g(USART)
	if (!usart.Save(fio, ver)) {
		return FALSE;
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	���[�h
//
//---------------------------------------------------------------------------
BOOL FASTCALL MFP::Load(Fileio *fio, int ver)
{
	int i;
	size_t sz;

	ASSERT(this);
	LOG0(Log::Normal, "���[�h");

	// �{��
	if (!fio->Read(&sz, sizeof(sz))) {
		return FALSE;
	}
	if (sz != sizeof(mfp_t)) {
		return FALSE;
	}
	if (!fio->Read(&mfp, (int)sz)) {
		return FALSE;
	}

	// �C�x���g(�^�C�})
	for (i=0; i<4; i++) {
		if (!timer[i].Load(fio, ver)) {
			return FALSE;
		}
	}

	// �C�x���g(USART)
	if (!usart.Load(fio, ver)) {
		return FALSE;
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	�ݒ�K�p
//
//---------------------------------------------------------------------------
void FASTCALL MFP::ApplyCfg(const Config* /*config*/)
{
	ASSERT(this);

	LOG0(Log::Normal, "�ݒ�K�p");
}

//---------------------------------------------------------------------------
//
//	�o�C�g�ǂݍ���
//
//---------------------------------------------------------------------------
DWORD FASTCALL MFP::ReadByte(DWORD addr)
{
	DWORD data;

	ASSERT(this);
	ASSERT((addr >= memdev.first) && (addr <= memdev.last));

	// ��A�h���X�̂݃f�R�[�h����Ă���
	if ((addr & 1) != 0) {
		// �E�F�C�g
		scheduler->Wait(3);

		// 64�o�C�g�P�ʂŃ��[�v
		addr &= 0x3f;
		addr >>= 1;

		switch (addr) {
			// GPIP
			case 0x00:
				return mfp.gpdr;

			// AER
			case 0x01:
				return mfp.aer;

			// DDR
			case 0x02:
				return mfp.ddr;

			// IER(A)
			case 0x03:
				return GetIER(0);

			// IER(B)
			case 0x04:
				return GetIER(1);

			// IPR(A)
			case 0x05:
				return GetIPR(0);

			// IPR(B)
			case 0x06:
				return GetIPR(1);

			// ISR(A)
			case 0x07:
				return GetISR(0);

			// ISR(B)
			case 0x08:
				return GetISR(1);

			// IMR(A)
			case 0x09:
				return GetIMR(0);

			// IMR(B)
			case 0x0a:
				return GetIMR(1);

			// VR
			case 0x0b:
				return GetVR();

			// �^�C�}A�R���g���[��
			case 0x0c:
				return GetTCR(0);

			// �^�C�}B�R���g���[��
			case 0x0d:
				return GetTCR(1);

			// �^�C�}C&D�R���g���[��
			case 0x0e:
				data = GetTCR(2);
				data <<= 4;
				data |= GetTCR(3);
				return data;

			// �^�C�}A�f�[�^
			case 0x0f:
				return GetTIR(0);

			// �^�C�}B�f�[�^
			case 0x10:
				return GetTIR(1);

			// �^�C�}C�f�[�^
			case 0x11:
				return GetTIR(2);

			// �^�C�}D�f�[�^
			case 0x12:
				return GetTIR(3);

			// SYNC�L�����N�^
			case 0x13:
				return mfp.scr;

			// USART�R���g���[��
			case 0x14:
				return mfp.ucr;

			// ���V�[�o�X�e�[�^�X
			case 0x15:
				return mfp.rsr;

			// �g�����X�~�b�^�X�e�[�^�X
			case 0x16:
				// TE�r�b�g�̓N���A�����
				mfp.tsr = (DWORD)(mfp.tsr & ~0x40);
				return mfp.tsr;

			// USART�f�[�^
			case 0x17:
				Receive();
				return mfp.rur;

			// ����ȊO
			default:
				LOG1(Log::Warning, "���������W�X�^�ǂݍ��� R%02d", addr);
				return 0xff;
		}
	}

	return 0xff;
}

//---------------------------------------------------------------------------
//
//	���[�h�ǂݍ���
//
//---------------------------------------------------------------------------
DWORD FASTCALL MFP::ReadWord(DWORD addr)
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
void FASTCALL MFP::WriteByte(DWORD addr, DWORD data)
{
	ASSERT(this);
	ASSERT((addr >= memdev.first) && (addr <= memdev.last));
	ASSERT(data < 0x100);

	// ��A�h���X�̂݃f�R�[�h����Ă���
	if ((addr & 1) == 0) {
		// �o�X�G���[�͔������Ȃ�
		return;
	}

	// 64�o�C�g�P�ʂŃ��[�v
	addr &= 0x3f;
	addr >>= 1;

	// �E�F�C�g
	scheduler->Wait(3);

	switch (addr) {
		// GPIP
		case 0x00:
			// VDISP��AND.B�Ń`�F�b�N����ꍇ������(MOON FIGHTER)
			SetGPDR(data);
			return;

		// AER
		case 0x01:
			mfp.aer = data;
			mfp.ber = (DWORD)(~data);
			mfp.ber ^= mfp.gpdr;
			IntGPIP();
			return;

		// DDR
		case 0x02:
			mfp.ddr = data;
			if (mfp.ddr != 0) {
				LOG0(Log::Warning, "GPIP�o�̓f�B���N�V����");
			}
			return;

		// IER(A)
		case 0x03:
			SetIER(0, data);
			return;

		// IER(B)
		case 0x04:
			SetIER(1, data);
			return;

		// IPR(A)
		case 0x05:
			SetIPR(0, data);
			return;

		// IPR(B)
		case 0x06:
			SetIPR(1, data);
			return;

		// ISR(A)
		case 0x07:
			SetISR(0, data);
			return;

		// ISR(B)
		case 0x08:
			SetISR(1, data);
			return;

		// IMR(A)
		case 0x09:
			SetIMR(0, data);
			return;

		// IMR(B)
		case 0x0a:
			SetIMR(1, data);
			return;

		// VR
		case 0x0b:
			SetVR(data);
			return;

		// �^�C�}A�R���g���[��
		case 0x0c:
			SetTCR(0, data);
			return;

		// �^�C�}B�R���g���[��
		case 0x0d:
			SetTCR(1, data);
			return;

		// �^�C�}C&D�R���g���[��
		case 0x0e:
			SetTCR(2, (DWORD)(data >> 4));
			SetTCR(3, (DWORD)(data & 0x0f));
			return;

		// �^�C�}A�f�[�^
		case 0x0f:
			SetTDR(0, data);
			return;

		// �^�C�}B�f�[�^
		case 0x10:
			SetTDR(1, data);
			return;

		// �^�C�}C�f�[�^
		case 0x11:
			SetTDR(2, data);
			return;

		// �^�C�}D�f�[�^
		case 0x12:
			SetTDR(3, data);
			return;

		// SYNC�L�����N�^
		case 0x13:
			mfp.scr = data;
			return;

		// USART�R���g���[��
		case 0x14:
			if (data != 0x88) {
				LOG1(Log::Warning, "USART �p�����[�^�G���[ %02X", data);
			}
			mfp.ucr = data;
			return;

		// ���V�[�o�X�e�[�^�X
		case 0x15:
			SetRSR(data);
			return;

		// �g�����X�~�b�^�X�e�[�^�X
		case 0x16:
			SetTSR(data);
			return;

		// USART�f�[�^
		case 0x17:
			Transmit(data);
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
void FASTCALL MFP::WriteWord(DWORD addr, DWORD data)
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
DWORD FASTCALL MFP::ReadOnly(DWORD addr) const
{
	DWORD data;

	ASSERT(this);
	ASSERT((addr >= memdev.first) && (addr <= memdev.last));

	// ��A�h���X�̂݃f�R�[�h����Ă���
	if ((addr & 1) == 0) {
		return 0xff;
	}

	// 64�o�C�g�P�ʂŃ��[�v
	addr &= 0x3f;
	addr >>= 1;

	switch (addr) {
		// GPIP
		case 0x00:
			return mfp.gpdr;

		// AER
		case 0x01:
			return mfp.aer;

		// DDR
		case 0x02:
			return mfp.ddr;

		// IER(A)
		case 0x03:
			return GetIER(0);

		// IER(B)
		case 0x04:
			return GetIER(1);

		// IPR(A)
		case 0x05:
			return GetIPR(0);

		// IPR(B)
		case 0x06:
			return GetIPR(1);

		// ISR(A)
		case 0x07:
			return GetISR(0);

		// ISR(B)
		case 0x08:
			return GetISR(1);

		// IMR(A)
		case 0x09:
			return GetIMR(0);

		// IMR(B)
		case 0x0a:
			return GetIMR(1);

		// VR
		case 0x0b:
			return GetVR();

		// �^�C�}A�R���g���[��
		case 0x0c:
			return mfp.tcr[0];

		// �^�C�}B�R���g���[��
		case 0x0d:
			return mfp.tcr[1];

		// �^�C�}C&D�R���g���[��
		case 0x0e:
			data = mfp.tcr[2];
			data <<= 4;
			data |= mfp.tcr[3];
			return data;

		// �^�C�}A�f�[�^
		case 0x0f:
			return mfp.tir[0];

		// �^�C�}B�f�[�^(�����_���Ȓl��Ԃ�)
		case 0x10:
			return ((scheduler->GetTotalTime() % 13) + 1);

		// �^�C�}C�f�[�^
		case 0x11:
			return mfp.tir[2];

		// �^�C�}D�f�[�^
		case 0x12:
			return mfp.tir[3];

		// SYNC�L�����N�^
		case 0x13:
			return mfp.scr;

		// USART�R���g���[��
		case 0x14:
			return mfp.ucr;

		// ���V�[�o�X�e�[�^�X
		case 0x15:
			return mfp.rsr;

		// �g�����X�~�b�^�X�e�[�^�X
		case 0x16:
			return mfp.tsr;

		// USART�f�[�^
		case 0x17:
			return mfp.rur;
	}

	return 0xff;
}

//---------------------------------------------------------------------------
//
//	�����f�[�^�擾
//
//---------------------------------------------------------------------------
void FASTCALL MFP::GetMFP(mfp_t *buffer) const
{
	ASSERT(this);
	ASSERT(buffer);

	// �f�[�^���R�s�[
	*buffer = mfp;
}

//---------------------------------------------------------------------------
//
//	���荞��
//
//---------------------------------------------------------------------------
void FASTCALL MFP::Interrupt(int level, BOOL enable)
{
	int index;

	ASSERT(this);
	ASSERT((level >= 0) && (level < 0x10));

	index = 15 - level;
	if (enable) {
		// ���ɗv������Ă��邩
		if (mfp.ireq[index]) {
			return;
		}

		// �C�l�[�u�����W�X�^�͂ǂ���
		if (!mfp.ier[index]) {
			return;
		}

		// �t���OUp�A���荞�݃`�F�b�N
		mfp.ireq[index] = TRUE;
		IntCheck();
	}
	else {
		// ���Ɏ����������������A�v���󗝂��ꂽ�ォ
		if (!mfp.ireq[index] && !mfp.ipr[index]) {
			return;
		}
		mfp.ireq[index] = FALSE;

		// �y���f�B���O�������A���荞�݉�������
		mfp.ipr[index] = FALSE;
		IntCheck();
	}
}

//---------------------------------------------------------------------------
//
//	���荞�ݗD�揇�ʔ���
//
//---------------------------------------------------------------------------
void FASTCALL MFP::IntCheck()
{
	int i;
#if defined(MFP_LOG)
	char buffer[0x40];
#endif	// MFP_LOG

	ASSERT(this);

	// ���荞�݃L�����Z���`�F�b�N
	if (mfp.iidx >= 0) {
		// �y���f�B���O�����܂��̓}�X�N�ŁA���荞�ݎ�艺��(�f�[�^�V�[�g2.7.1)
		if (!mfp.ipr[mfp.iidx] || !mfp.imr[mfp.iidx]) {
			cpu->IntCancel(6);
#if defined(MFP_LOG)
			sprintf(buffer, "���荞�ݎ����� %s", IntDesc[mfp.iidx]);
			LOG0(Log::Normal, buffer);
#endif	// MFP_LOG
			mfp.iidx = -1;
		}
	}

	// ���荞�ݔ�����
	for (i=0; i<0x10; i++) {
		// ���荞�݃C�l�[�u����
		if (mfp.ier[i]) {
			// ���荞�݃��N�G�X�g�����邩
			if (mfp.ireq[i]) {
				// �y���f�B���O���W�X�^��1�ɂ���(���荞�ݎ�)
				mfp.ipr[i] = TRUE;
				mfp.ireq[i] = FALSE;
			}
		}
		else {
			// �C�l�[�u�����W�X�^0�ŁA���荞�݃y���f�B���O����
			mfp.ipr[i] = FALSE;
			mfp.ireq[i] = FALSE;
		}
	}

	// ���荞�݃x�N�^���o
	for (i=0; i<0x10; i++) {
		// ���荞�݃y���f�B���O��
		if (!mfp.ipr[i]) {
			continue;
		}

		// ���荞�݃}�X�N��
		if (!mfp.imr[i]) {
			continue;
		}

		// �T�[�r�X���łȂ���
		if (mfp.isr[i]) {
			continue;
		}

		// ���ɗv�����Ă��銄�荞�݂���ʂȂ�A���荞�݂�������
		if (mfp.iidx > i) {
			cpu->IntCancel(6);
#if defined(MFP_LOG)
			sprintf(buffer, "���荞�ݗD������� %s", IntDesc[mfp.iidx]);
			LOG0(Log::Normal, buffer);
#endif	// MFP_LOG
			mfp.iidx = -1;
		}

		// �x�N�^���o
		if (cpu->Interrupt(6, (mfp.vr & 0xf0) + (15 - i))) {
			// CPU�Ɏ󂯕t����ꂽ�B�C���f�b�N�X�L��
#if defined(MFP_LOG)
			sprintf(buffer, "���荞�ݗv�� %s", IntDesc[i]);
			LOG0(Log::Normal, buffer);
#endif	// MFP_LOG
			mfp.iidx = i;
			break;
		}
	}
}

//---------------------------------------------------------------------------
//
//	���荞�݉���
//
//---------------------------------------------------------------------------
void FASTCALL MFP::IntAck()
{
#if defined(MFP_LOG)
	char buffer[0x40];
#endif	// MFP_LOG

	ASSERT(this);

	// ���Z�b�g����ɁACPU���犄�荞�݂��Ԉ���ē���ꍇ������
	if (mfp.iidx < 0) {
		LOG0(Log::Warning, "�v�����Ă��Ȃ����荞��");
		return;
	}

#if defined(MFP_LOG)
	sprintf(buffer, "���荞�݉��� %s", IntDesc[mfp.iidx]);
	LOG0(Log::Normal, buffer);
#endif	// MFP_LOG

	// ���荞�݂��󂯕t����ꂽ�B�y���f�B���O����
	mfp.ipr[mfp.iidx] = FALSE;

	// �C���T�[�r�X(�I�[�gEOI��0�A�}�j���A��EOI��1)
	if (mfp.vr & 0x08) {
		mfp.isr[mfp.iidx] = TRUE;
	}
	else {
		mfp.isr[mfp.iidx] = FALSE;
	}

	// �C���f�b�N�X�����荞�ݖ����ɕύX
	mfp.iidx = -1;

	// �ēx�A���荞�݃`�F�b�N���s��
	IntCheck();
}

//---------------------------------------------------------------------------
//
//	IER�ݒ�
//
//---------------------------------------------------------------------------
void FASTCALL MFP::SetIER(int offset, DWORD data)
{
	int i;
#if defined(MFP_LOG)
	char buffer[0x40];
#endif	// MFP_LOG

	ASSERT(this);
	ASSERT((offset == 0) || (offset == 1));
	ASSERT(data < 0x100);

	// �����ݒ�
	offset <<= 3;

	// 8��A�r�b�g����
	for (i=offset; i<offset+8; i++) {
		if (data & 0x80) {
			mfp.ier[i] = TRUE;
		}
		else {
			// IPR��������(�f�[�^�V�[�g4.3.1)
			mfp.ier[i] = FALSE;
			mfp.ipr[i] = FALSE;
		}

		data <<= 1;
	}

	// ���荞�ݗD�揇��
	IntCheck();
}

//---------------------------------------------------------------------------
//
//	IER�擾
//
//---------------------------------------------------------------------------
DWORD FASTCALL MFP::GetIER(int offset) const
{
	int i;
	DWORD bit;

	ASSERT(this);
	ASSERT((offset == 0) || (offset == 1));

	// �����ݒ�
	offset <<= 3;
	bit = 0;

	// 8��A�r�b�g����
	for (i=offset; i<offset+8; i++) {
		bit <<= 1;
		if (mfp.ier[i]) {
			bit |= 0x01;
		}
	}

	return bit;
}

//---------------------------------------------------------------------------
//
//	IPR�ݒ�
//
//---------------------------------------------------------------------------
void FASTCALL MFP::SetIPR(int offset, DWORD data)
{
	int i;
#if defined(MFP_LOG)
	char buffer[0x40];
#endif	// MFP_LOG

	ASSERT(this);
	ASSERT((offset == 0) || (offset == 1));

	// �����ݒ�
	offset <<= 3;

	// 8��A�r�b�g����
	for (i=offset; i<offset+8; i++) {
		if (!(data & 0x80)) {
			// IPR��CPU����1�ɂ��邱�Ƃ͏o���Ȃ�
			mfp.ipr[i] = FALSE;
		}

		data <<= 1;
	}

	// ���荞�ݗD�揇��
	IntCheck();
}

//---------------------------------------------------------------------------
//
//	IPR�擾
//
//---------------------------------------------------------------------------
DWORD FASTCALL MFP::GetIPR(int offset) const
{
	int i;
	DWORD bit;

	ASSERT(this);
	ASSERT((offset == 0) || (offset == 1));

	// �����ݒ�
	offset <<= 3;
	bit = 0;

	// 8��A�r�b�g����
	for (i=offset; i<offset+8; i++) {
		bit <<= 1;
		if (mfp.ipr[i]) {
			bit |= 0x01;
		}
	}

	return bit;
}

//---------------------------------------------------------------------------
//
//	ISR�ݒ�
//
//---------------------------------------------------------------------------
void FASTCALL MFP::SetISR(int offset, DWORD data)
{
	int i;
#if defined(MFP_LOG)
	char buffer[0x40];
#endif	// MFP_LOG

	ASSERT(this);
	ASSERT((offset == 0) || (offset == 1));
	ASSERT(data < 0x100);

	// �����ݒ�
	offset <<= 3;

	// 8��A�r�b�g����
	for (i=offset; i<offset+8; i++) {
		if (!(data & 0x80)) {
			mfp.isr[i] = FALSE;
		}

		data <<= 1;
	}

	// ���荞�ݗD�揇��
	IntCheck();
}

//---------------------------------------------------------------------------
//
//	ISR�擾
//
//---------------------------------------------------------------------------
DWORD FASTCALL MFP::GetISR(int offset) const
{
	int i;
	DWORD bit;

	ASSERT(this);
	ASSERT((offset == 0) || (offset == 1));

	// �����ݒ�
	offset <<= 3;
	bit = 0;

	// 8��A�r�b�g����
	for (i=offset; i<offset+8; i++) {
		bit <<= 1;
		if (mfp.isr[i]) {
			bit |= 0x01;
		}
	}

	return bit;
}

//---------------------------------------------------------------------------
//
//	IMR�ݒ�
//
//---------------------------------------------------------------------------
void FASTCALL MFP::SetIMR(int offset, DWORD data)
{
	int i;
#if defined(MFP_LOG)
	char buffer[0x40];
#endif	// MFP_LOG

	ASSERT(this);
	ASSERT((offset == 0) || (offset == 1));
	ASSERT(data < 0x100);

	// �����ݒ�
	offset <<= 3;

	// 8��A�r�b�g����
	for (i=offset; i<offset+8; i++) {
		if (data & 0x80) {
			mfp.imr[i] = TRUE;
		}
		else {
			mfp.imr[i] = FALSE;
		}

		data <<= 1;
	}

	// ���荞�ݗD�揇��
	IntCheck();
}

//---------------------------------------------------------------------------
//
//	IMR�擾
//
//---------------------------------------------------------------------------
DWORD FASTCALL MFP::GetIMR(int offset) const
{
	int i;
	DWORD bit;

	ASSERT(this);
	ASSERT((offset == 0) || (offset == 1));

	// �����ݒ�
	offset <<= 3;
	bit = 0;

	// 8��A�r�b�g����
	for (i=offset; i<offset+8; i++) {
		bit <<= 1;
		if (mfp.imr[i]) {
			bit |= 0x01;
		}
	}

	return bit;
}

//---------------------------------------------------------------------------
//
//	VR�ݒ�
//
//---------------------------------------------------------------------------
void FASTCALL MFP::SetVR(DWORD data)
{
	ASSERT(this);
	ASSERT(data < 0x100);

	if (mfp.vr != data) {
		mfp.vr = data;
		LOG1(Log::Detail, "���荞�݃x�N�^�x�[�X $%02X", data & 0xf0);

		if (mfp.vr & 0x08) {
			LOG0(Log::Warning, "�}�j���A��EOI���[�h");
		}
	}
}

//---------------------------------------------------------------------------
//
//	VR�擾
//
//---------------------------------------------------------------------------
DWORD FASTCALL MFP::GetVR() const
{
	ASSERT(this);
	return mfp.vr;
}

//---------------------------------------------------------------------------
//
//	���荞�ݖ��̃e�[�u��
//
//---------------------------------------------------------------------------
const char* MFP::IntDesc[0x10] = {
	"H-SYNC",
	"CIRQ",
	"Timer-A",
	"RxFull",
	"RxError",
	"TxEmpty",
	"TxError",
	"Timer-B",
	"(NoUse)",
	"V-DISP",
	"Timer-C",
	"Timer-D",
	"FMIRQ",
	"POW SW",
	"EXPON",
	"ALARM"
};

//---------------------------------------------------------------------------
//
//	�C�x���g�R�[���o�b�N(�f�B���C���[�h�Ŏg�p)
//
//---------------------------------------------------------------------------
BOOL FASTCALL MFP::Callback(Event *ev)
{
	int channel;
	DWORD low;

	ASSERT(this);
	ASSERT(ev);

	// ���[�U�f�[�^�����ʂ𓾂�
	channel = (int)ev->GetUser();

	// �^�C�}
	if ((channel >= 0) && (channel <= 3)) {
		low = (mfp.tcr[channel] & 0x0f);

		// �^�C�}�I����
		if (low == 0) {
			// �^�C�}�I�t
			return FALSE;
		}

		// �f�B���C���[�h��
		if (low & 0x08) {
			// �J�E���g0���班���x��āA���荞�ݔ���
			Interrupt(TimerInt[channel], TRUE);

			// �����V���b�g
			return FALSE;
		}

		// �^�C�}��i�߂�
		Proceed(channel);
		return TRUE;
	}

	// USART
	ASSERT(channel == 4);
	USART();
	return TRUE;
}

//---------------------------------------------------------------------------
//
//	�^�C�}�C�x���g����(�C�x���g�J�E���g���[�h�Ŏg�p)
//
//---------------------------------------------------------------------------
void FASTCALL MFP::EventCount(int channel, int value)
{
	DWORD edge;
	BOOL flag;

	ASSERT(this);
	ASSERT((channel >= 0) && (channel <= 1));
	ASSERT((value == 0) || (value == 1));
	ASSERT((mfp.tbr[channel] == 0) || (mfp.tbr[channel] == 1));

	// �C�x���g�J�E���g���[�h��(�^�C�}A,B�̂�)
	if ((mfp.tcr[channel] & 0x0f) == 0x08) {
		// ������GPIP4, GPIP3�Ō��܂�
		if (channel == 0) {
			edge = mfp.aer & 0x10;
		}
		else {
			edge = mfp.aer & 0x08;
		}

		// �t���O�I�t
		flag = FALSE;

		// �G�b�W����
		if (edge == 1) {
			// �G�b�W��1�̂Ƃ��A0��1�Ń^�C�}��i�߂�
			if ((mfp.tbr[channel] == 0) && (value == 1)) {
				flag = TRUE;
			}
		}
		else {
			// �G�b�W��0�̂Ƃ��A1��0�Ń^�C�}��i�߂�
			if ((mfp.tbr[channel] == 1) && (value == 0)) {
				flag = TRUE;
			}
		}

		// �^�C�}��i�߂�
		if (flag) {
			Proceed(channel);
		}
	}

	// TBR���X�V
	mfp.tbr[channel] = (DWORD)value;
}

//---------------------------------------------------------------------------
//
//	TCR�ݒ�
//
//---------------------------------------------------------------------------
void FASTCALL MFP::SetTCR(int channel, DWORD data)
{
	DWORD prev;
	DWORD now;
	DWORD speed;

	ASSERT(this);
	ASSERT((channel >= 0) && (channel <= 3));
	ASSERT(data < 0x100);

	// �^�ϊ��A��v�`�F�b�N
	now = data;
	now &= 0x0f;
	prev = mfp.tcr[channel];
	if (now == prev) {
		return;
	}
	mfp.tcr[channel] = now;

	// �^�C�}B��0x01�̂݉�(�G�~�����[�V�������Ȃ�)
	if (channel == 1) {
		if ((now != 0x01) && (now != 0x00)) {
			LOG1(Log::Warning, "�^�C�}B�R���g���[�� $%02X", now);
		}
		now = 0;
	}

	// �^�C�}�X�g�b�v��
	if (now == 0) {
#if defined(MFP_LOG)
		LOG1(Log::Normal, "�^�C�}%c ��~", channel + 'A');
#endif	// MFP_LOG
		timer[channel].SetTime(0);

		// ���荞�݂̎�艺�����s���K�v����(������h���L����)
		Interrupt(TimerInt[channel], FALSE);

		// Timer-D��CPU�N���b�N��߂�(CH30.SYS)
		if (channel == 3) {
			if (mfp.sram != 0) {
				// CPU�N���b�N��߂�
				scheduler->SetCPUSpeed(mfp.sram);
				mfp.sram = 0;
			}
		}
		return;
	}

	// �p���X�����胂�[�h�̓T�|�[�g���Ă��Ȃ�
	if (now > 0x08) {
		LOG2(Log::Warning, "�^�C�}%c �p���X�����胂�[�h$%02X", channel + 'A', now);
		return;
	}

	// �C�x���g�J�E���g���[�h��
	if (now == 0x08) {
#if defined(MFP_LOG)
		LOG1(Log::Normal, "�^�C�}%c �C�x���g�J�E���g���[�h", channel + 'A');
#endif	// MFP_LOG
		// ��x�C�x���g���~�߂�
		timer[channel].SetTime(0);

		// �^�C�}OFF��ON�Ȃ�A�J�E���g�����[�h
		if (prev == 0) {
			mfp.tir[channel] = mfp.tdr[channel];
		}
		return;
	}

	// �f�B���C���[�h�ł́A�v���X�P�[����ݒ�
#if defined(MFP_LOG)
	LOG3(Log::Normal, "�^�C�}%c �f�B���C���[�h %d.%dus",
				channel + 'A', (TimerHus[now] / 2), (TimerHus[now] & 1) * 5);
#endif	// MFP_LOG

	// �^�C�}OFF��ON�Ȃ�A�J�E���^�����[�h(_VDISPST�p�b�`)
	if (prev == 0) {
		mfp.tir[channel] = mfp.tdr[channel];
	}

	// �C�x���g���Z�b�g
	timer[channel].SetTime(TimerHus[now]);

	// Timer-C�̏ꍇ�AINFO.RAM��p�΍���s��(CPU���x�v���𖳗���肠�킹��)
	if (channel == 2) {
		if ((now == 3) && (mfp.sram == 0)) {
			speed = cpu->GetPC();
			if ((speed >= 0xed0100) && (speed <= 0xedffff)) {
				// CPU�N���b�N�𗎂Ƃ�
				speed = scheduler->GetCPUSpeed();
				mfp.sram = speed;
				speed *= 83;
				speed /= 96;
				scheduler->SetCPUSpeed(speed);
			}
		}
		if ((now != 3) && (mfp.sram != 0)) {
			// CPU�N���b�N��߂�
			scheduler->SetCPUSpeed(mfp.sram);
			mfp.sram = 0;
		}
	}

	// Timer-D�̏ꍇ�ACH30.SYS��p�΍���s��(CPU���x�v���𖳗���肠�킹��)
	if (channel == 3) {
		if ((now == 7) && (mfp.sram == 0)) {
			speed = cpu->GetPC();
			if ((speed >= 0xed0100) && (speed <= 0xedffff)) {
				// CPU�N���b�N�𗎂Ƃ�
				speed = scheduler->GetCPUSpeed();
				mfp.sram = speed;
				speed *= 85;
				speed /= 96;
				scheduler->SetCPUSpeed(speed);
			}
		}
	}
}

//---------------------------------------------------------------------------
//
//	TCR�擾
//
//---------------------------------------------------------------------------
DWORD FASTCALL MFP::GetTCR(int channel) const
{
	ASSERT(this);
	ASSERT((channel >= 0) && (channel <= 3));

	return mfp.tcr[channel];
}

//---------------------------------------------------------------------------
//
//	TDR�ݒ�
//
//---------------------------------------------------------------------------
void FASTCALL MFP::SetTDR(int channel, DWORD data)
{
	ASSERT(this);
	ASSERT((channel >= 0) && (channel <= 3));
	ASSERT(data < 0x100);

	mfp.tdr[channel] = data;

	// �^�C�}B�͌Œ�l�̂͂�
	if (channel == 1) {
		if (data != 0x0d) {
			LOG1(Log::Warning, "�^�C�}B�����[�h�l %02X", data);
		}
	}
}

//---------------------------------------------------------------------------
//
//	TIR�擾
//
//---------------------------------------------------------------------------
DWORD FASTCALL MFP::GetTIR(int channel) const
{
	ASSERT(this);
	ASSERT((channel >= 0) && (channel <= 3));

	// �^�C�}B��XM6�ł͓ǂݏo�����֎~����(���@��1us x 14�^�C�})
	if (channel == 1) {
		// (���������`)
		LOG0(Log::Warning, "�^�C�}B �f�[�^���W�X�^�ǂݏo��");
		return (DWORD)((scheduler->GetTotalTime() % 13) + 1);
	}

	return mfp.tir[channel];
}

//---------------------------------------------------------------------------
//
//	�^�C�}��i�߂�
//
//---------------------------------------------------------------------------
void FASTCALL MFP::Proceed(int channel)
{
	ASSERT(this);
	ASSERT((channel >= 0) && (channel <= 3));

	// �J�E���^�����Z
	if (mfp.tir[channel] > 0) {
		mfp.tir[channel]--;
	}
	else {
		mfp.tir[channel] = 0xff;
	}

	// 0�ɂȂ����烊���[�h�A���荞��
	if (mfp.tir[channel] == 0) {
#if defined(MFP_LOG)
	LOG1(Log::Normal, "�^�C�}%c �I�[�o�[�t���[", channel + 'A');
#endif	// MFP_LOG

		// �����[�h
		mfp.tir[channel] = mfp.tdr[channel];

		// �C�x���g�J�E���g���[�h�͊��荞�݃C�x���g����(�X�s���f�B�W�[II)
		if (mfp.tcr[channel] == 0x08) {
			// GPIP�ύX���ɍs���A�����x��Ċ��荞�݂��o���悤�ɂ���
			// ���^���I�����WEX(12�ł̓_���A�v����)
			timer[channel].SetTime(36);
		}
		else {
			// �ʏ튄�荞��
			Interrupt(TimerInt[channel], TRUE);
		}
	}
}

//---------------------------------------------------------------------------
//
//	�^�C�}���荞�݃e�[�u��
//
//---------------------------------------------------------------------------
const int MFP::TimerInt[4] = {
	13,									// Timer-A
	8,									// Timer-B
	5,									// Timer-C
	4									// Timer-D
};

//---------------------------------------------------------------------------
//
//	�^�C�}���ԃe�[�u��
//
//---------------------------------------------------------------------------
const DWORD MFP::TimerHus[8] = {
	0,									// �^�C�}�X�g�b�v
	2,									// 1.0us
	5,									// 2.5us
	8,									// 4us
	25,									// 12.5us
	32,									// 16us
	50,									// 25us
	100									// 50us
};

//---------------------------------------------------------------------------
//
//	GPDR�ݒ�
//
//---------------------------------------------------------------------------
void FASTCALL MFP::SetGPDR(DWORD data)
{
	int i;
	DWORD bit;

	ASSERT(this);
	ASSERT(data < 0x100);

	// DDR��1�̃r�b�g�̂ݗL��
	for (i=0; i<8; i++) {
		bit = (DWORD)(1 << i);
		if (mfp.ddr & bit) {
			if (data & bit) {
				SetGPIP(i, 1);
			}
			else {
				SetGPIP(i, 0);
			}
		}
	}
}

//---------------------------------------------------------------------------
//
//	GPIP�ݒ�
//
//---------------------------------------------------------------------------
void FASTCALL MFP::SetGPIP(int num, int value)
{
	DWORD data;

	ASSERT(this);
	ASSERT((num >= 0) && (num < 8));
	ASSERT((value == 0) || (value == 1));

	// �o�b�N�A�b�v�����
	data = mfp.gpdr;

	// �r�b�g�쐬
	mfp.gpdr &= (DWORD)(~(1 << num));
	if (value == 1) {
		mfp.gpdr |= (DWORD)(1 << num);
	}

	// ����Ă���Ί��荞�݃`�F�b�N
	if (mfp.gpdr != data) {
		IntGPIP();
	}
}

//---------------------------------------------------------------------------
//
//	GPIP���荞�݃`�F�b�N
//
//---------------------------------------------------------------------------
void FASTCALL MFP::IntGPIP()
{
	DWORD data;
	int i;

	ASSERT(this);

	// Inside68k�̋L�q�͋t�IMFP�f�[�^�V�[�g�ɏ]���Ǝ��̗l�B
	// AER0 1->0�Ŋ��荞��
	// AER1 0->1�Ŋ��荞��

	// ~AER��GPDR��XOR
	data = (DWORD)(~mfp.aer);
	data ^= (DWORD)mfp.gpdr;

	// BER�����āA1��0�ɕω�����Ƃ����荞�ݔ���
	// (�������p���X������^�C�}�������GPIP4,GPIP3�͊��荞�ݏ������Ȃ�)
	for (i=0; i<8; i++) {
		if (data & 0x80) {
			if (!(mfp.ber & 0x80)) {
				if (i == 3) {
					// GPIP4�B�^�C�}A���`�F�b�N
					if ((mfp.tcr[0] & 0x0f) > 0x08) {
						data <<= 1;
						mfp.ber <<= 1;
						continue;
					}
				}
				if (i == 4) {
					// GPIP3�B�^�C�}B���`�F�b�N
					if ((mfp.tcr[1] & 0x0f) > 0x08) {
						data <<= 1;
						mfp.ber <<= 1;
						continue;
					}
				}

				// ���荞�ݗv��(SORCERIAN X1->88)
				Interrupt(GPIPInt[i], TRUE);
			}
		}

		// ����
		data <<= 1;
		mfp.ber <<= 1;
	}

	// BER���쐬
	mfp.ber = (DWORD)(~mfp.aer);
	mfp.ber ^= mfp.gpdr;
}

//---------------------------------------------------------------------------
//
//	GPIP���荞�݃e�[�u��
//
//---------------------------------------------------------------------------
const int MFP::GPIPInt[8] = {
	15,									// H-SYNC
	14,									// CIRQ
	7,									// (NoUse)
	6,									// V-DISP
	3,									// OPM
	2,									// POWER
	1,									// EXPON
	0									// ALARM
};

//---------------------------------------------------------------------------
//
//	���V�[�o�X�e�[�^�X�ݒ�
//
//---------------------------------------------------------------------------
void FASTCALL MFP::SetRSR(DWORD data)
{
	ASSERT(this);
	ASSERT(data < 0x100);

	// RE�̂ݐݒ�
	data &= 0x01;

	mfp.rsr &= ~0x01;
	mfp.rsr |= (DWORD)(mfp.rsr | data);
}

//---------------------------------------------------------------------------
//
//	CPU��MFP ��M
//
//---------------------------------------------------------------------------
void FASTCALL MFP::Receive()
{
	ASSERT(this);

	// �^�C�}B�����`�F�b�N
	if (mfp.tcr[1] != 0x01) {
		return;
	}
	if (mfp.tdr[1] != 0x0d) {
		return;
	}

	// USART�R���g���[�������`�F�b�N
	if (mfp.ucr != 0x88) {
		return;
	}

	// ���V�[�o�f�B�Z�[�u���̏ꍇ�A�������Ȃ�
	if (!(mfp.rsr & 0x01)) {
		return;
	}

	// BF=1�̏ꍇ�A���Ɉ������f�[�^�͖���
	if (!(mfp.rsr & 0x80)) {
		return;
	}

	// �f�[�^�������BBF�AOE��0�ɐݒ�
	mfp.rsr &= (DWORD)~0xc0;
}

//---------------------------------------------------------------------------
//
//	�g�����X�~�b�^�X�e�[�^�X�ݒ�
//
//---------------------------------------------------------------------------
void FASTCALL MFP::SetTSR(DWORD data)
{
	ASSERT(this);
	ASSERT(data < 0x100);

	// BE,UE,END�͒��ڃN���A�ł��Ȃ�
	mfp.tsr = (DWORD)(mfp.tsr & 0xd0);
	data &= (DWORD)~0xd0;
	mfp.tsr = (DWORD)(mfp.tsr | data);

	// TE=1�ŁAUE,END���N���A
	if (mfp.tsr & 0x01) {
		mfp.tsr = (DWORD)(mfp.tsr & ~0x50);
	}
}

//---------------------------------------------------------------------------
//
//	CPU��MFP ���M
//
//---------------------------------------------------------------------------
void FASTCALL MFP::Transmit(DWORD data)
{
	ASSERT(this);

	// �^�C�}B�����`�F�b�N
	if (mfp.tcr[1] != 0x01) {
		return;
	}
	if (mfp.tdr[1] != 0x0d) {
		return;
	}

	// USART�R���g���[�������`�F�b�N
	if (mfp.ucr != 0x88) {
		return;
	}

	// �g�����X�~�b�^�f�B�Z�[�u���̏ꍇ�A�������Ȃ�
	if (!(mfp.tsr & 0x01)) {
		return;
	}

	// �X�e�[�^�X�y�уf�[�^���Z�b�g
	mfp.tsr = (DWORD)(mfp.tsr & ~0x80);
	mfp.tur = data;
#if defined(MFP_LOG)
	LOG1(Log::Normal, "USART���M�f�[�^��t %02X", (BYTE)data);
#endif	// MFP_LOG
	return;
}

//---------------------------------------------------------------------------
//
//	USART����
//
//---------------------------------------------------------------------------
void FASTCALL MFP::USART()
{
	ASSERT(this);
	ASSERT((mfp.readpoint >= 0) && (mfp.readpoint < 0x10));
	ASSERT((mfp.writepoint >= 0) && (mfp.writepoint < 0x10));
	ASSERT((mfp.datacount >= 0) && (mfp.datacount <= 0x10));

	// �^�C�}�y��USART�ݒ���`�F�b�N
	if (mfp.tcr[1] != 0x01) {
		return;
	}
	if (mfp.tdr[1] != 0x0d) {
		return;
	}
	if (mfp.ucr != 0x88) {
		return;
	}

	//
	//	���M
	//

	if (!(mfp.tsr & 0x80)) {
		// �����Ńg�����X�~�b�^�f�B�Z�[�u���Ȃ�AEND������
		if (!(mfp.tsr & 0x01)) {
			mfp.tsr = (DWORD)(mfp.tsr & ~0x80);
			mfp.tsr = (DWORD)(mfp.tsr | 0x10);
			LOG0(Log::Warning, "USART ���M�I���G���[");
			Interrupt(9, TRUE);
			return;
		}

		// �o�b�t�@�G���v�e�B�A�I�[�g�^�[���A���E���h
		mfp.tsr = (DWORD)(mfp.tsr | 0x80);
		if (mfp.tsr & 0x20) {
			mfp.tsr = (DWORD)(mfp.tsr & ~0x20);
			SetRSR((DWORD)(mfp.rsr | 0x01));
		}

		// �L�[�{�[�h�փf�[�^���o�A���M�o�b�t�@�G���v�e�B���荞��
#if defined(MFP_LOG)
		LOG1(Log::Normal, "USART���M %02X", mfp.tur);
#endif	// MFP_LOG
		keyboard->Command(mfp.tur);
		Interrupt(10, TRUE);
	}
	else {
		if (!(mfp.tsr & 0x40)) {
			mfp.tsr = (DWORD)(mfp.tsr | 0x40);
			Interrupt(9, TRUE);
#if defined(MFP_LOG)
			LOG0(Log::Normal, "USART �A���_�[�����G���[");
#endif	// MPF_LOG
		}
	}

	//
	//	��M
	//

	// �L���Ȏ�M�f�[�^���Ȃ���΁A�������Ȃ�
	if (mfp.datacount == 0) {
		return;
	}

	// ���V�[�o�f�B�Z�[�u���Ȃ�A�������Ȃ�
	if (!(mfp.rsr & 0x01)) {
		return;
	}

	// �����Ń��b�N
	sync->Lock();

	// ���Ɏ�M�f�[�^������΁A�I�[�o�[�����Ƃ��ăf�[�^���̂Ă�
	if (mfp.rsr & 0x80) {
		mfp.rsr |= 0x40;
		mfp.readpoint = (mfp.readpoint + 1) & 0x0f;
		mfp.datacount--;
		sync->Unlock();

		LOG0(Log::Warning, "USART �I�[�o�[�����G���[");
		Interrupt(11, TRUE);
		return;
	}

	// �f�[�^��M�BBF���Z�b�g���A�f�[�^���L��
	mfp.rsr |= 0x80;
	mfp.rur = mfp.buffer[mfp.readpoint];
	mfp.readpoint = (mfp.readpoint + 1) & 0x0f;
	mfp.datacount--;
	sync->Unlock();

	Interrupt(12, TRUE);
}

//---------------------------------------------------------------------------
//
//	�L�[�f�[�^��M
//
//---------------------------------------------------------------------------
void FASTCALL MFP::KeyData(DWORD data)
{
	ASSERT(this);
	ASSERT((mfp.readpoint >= 0) && (mfp.readpoint < 0x10));
	ASSERT((mfp.writepoint >= 0) && (mfp.writepoint < 0x10));
	ASSERT((mfp.datacount >= 0) && (mfp.datacount <= 0x10));
	ASSERT(data < 0x100);

	// ���b�N
	sync->Lock();

	// �������݃|�C���g�֊i�[
	mfp.buffer[mfp.writepoint] = data;

	// �������݃|�C���g�ړ��A�J�E���^�{�P
	mfp.writepoint = (mfp.writepoint + 1) & 0x0f;
	mfp.datacount++;
	if (mfp.datacount > 0x10) {
		mfp.datacount = 0x10;
	}

	// Sync�A�����b�N
	sync->Unlock();
}
