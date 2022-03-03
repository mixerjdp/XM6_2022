//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 �o�h�D(ytanaka@ipc-tokai.or.jp)
//	[ FDC(uPD72065) ]
//
//---------------------------------------------------------------------------

#include "os.h"
#include "xm6.h"
#include "fdd.h"
#include "iosc.h"
#include "dmac.h"
#include "schedule.h"
#include "vm.h"
#include "log.h"
#include "fileio.h"
#include "config.h"
#include "fdc.h"

//===========================================================================
//
//	FDC
//
//===========================================================================
//#define FDC_LOG

//---------------------------------------------------------------------------
//
//	�R���X�g���N�^
//
//---------------------------------------------------------------------------
FDC::FDC(VM *p) : MemDevice(p)
{
	// �f�o�C�XID��������
	dev.id = MAKEID('F', 'D', 'C', ' ');
	dev.desc = "FDC (uPD72065)";

	// �J�n�A�h���X�A�I���A�h���X
	memdev.first = 0xe94000;
	memdev.last = 0xe95fff;

	// �I�u�W�F�N�g
	iosc = NULL;
	dmac = NULL;
	fdd = NULL;
}

//---------------------------------------------------------------------------
//
//	������
//
//---------------------------------------------------------------------------
BOOL FASTCALL FDC::Init()
{
	ASSERT(this);

	// ��{�N���X
	if (!MemDevice::Init()) {
		return FALSE;
	}

	// IOSC�擾
	iosc = (IOSC*)vm->SearchDevice(MAKEID('I', 'O', 'S', 'C'));
	ASSERT(iosc);

	// DMAC�擾
	dmac = (DMAC*)vm->SearchDevice(MAKEID('D', 'M', 'A', 'C'));
	ASSERT(dmac);

	// FDD�擾
	fdd = (FDD*)vm->SearchDevice(MAKEID('F', 'D', 'D', ' '));
	ASSERT(fdd);

	// �C�x���g������
	event.SetDevice(this);
	event.SetDesc("Data Transfer");
	event.SetUser(0);
	event.SetTime(0);
	scheduler->AddEvent(&event);

	// �������[�h�t���O�A�f���A���h���C�u�t���O(ApplyCfg)
	fdc.fast = FALSE;
	fdc.dual = FALSE;

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	�N���[���A�b�v
//
//---------------------------------------------------------------------------
void FASTCALL FDC::Cleanup()
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
void FASTCALL FDC::Reset()
{
	ASSERT(this);
	LOG0(Log::Normal, "���Z�b�g");

	// �f�[�^���W�X�^�E�X�e�[�^�X���W�X�^
	fdc.dr = 0;
	fdc.sr = 0;
	fdc.sr |= sr_rqm;
	fdc.sr &= ~sr_dio;
	fdc.sr &= ~sr_ndm;
	fdc.sr &= ~sr_cb;

	// �h���C�u�Z���N�g���W�X�^�EST0-ST3
	fdc.dcr = 0;
	fdc.dsr = 0;
	fdc.st[0] = 0;
	fdc.st[1] = 0;
	fdc.st[2] = 0;
	fdc.st[3] = 0;

	// �R�}���h���ʃp�����[�^
	fdc.srt = 1 * 2000;
	fdc.hut = 16 * 2000;
	fdc.hlt = 2 * 2000;
	fdc.hd = 0;
	fdc.us = 0;
	fdc.cyl[0] = 0;
	fdc.cyl[1] = 0;
	fdc.cyl[2] = 0;
	fdc.cyl[3] = 0;
	fdc.chrn[0] = 0;
	fdc.chrn[1] = 0;
	fdc.chrn[2] = 0;
	fdc.chrn[3] = 0;

	// ���̑�
	fdc.eot = 0;
	fdc.gsl = 0;
	fdc.dtl = 0;
	fdc.sc = 0;
	fdc.gpl = 0;
	fdc.d = 0;
	fdc.err = 0;
	fdc.seek = FALSE;
	fdc.ndm = FALSE;
	fdc.mfm = FALSE;
	fdc.mt = FALSE;
	fdc.sk = FALSE;
	fdc.tc = FALSE;
	fdc.load = FALSE;

	// �]���n
	fdc.offset = 0;
	fdc.len = 0;
	memset(fdc.buffer, 0, sizeof(fdc.buffer));

	// �t�F�[�Y�A�R�}���h
	fdc.phase = idle;
	fdc.cmd = no_cmd;

	// �p�P�b�g�Ǘ�
	fdc.in_len = 0;
	fdc.in_cnt = 0;
	memset(fdc.in_pkt, 0, sizeof(fdc.in_pkt));
	fdc.out_len = 0;
	fdc.out_cnt = 0;
	memset(fdc.out_pkt, 0, sizeof(fdc.out_pkt));

	// �C�x���g��~
	event.SetTime(0);

	// �A�N�Z�X��~(FDD�����Z�b�g��fdd.selected=0�ƂȂ�)
	fdd->Access(FALSE);
}

//---------------------------------------------------------------------------
//
//	�\�t�g�E�F�A���Z�b�g
//
//---------------------------------------------------------------------------
void FASTCALL FDC::SoftReset()
{
	// �������W�X�^(FDC)
	fdc.dr = 0;
	fdc.sr = 0;
	fdc.sr |= sr_rqm;
	fdc.sr &= ~sr_dio;
	fdc.sr &= ~sr_ndm;
	fdc.sr &= ~sr_cb;

	fdc.st[0] = 0;
	fdc.st[1] = 0;
	fdc.st[2] = 0;
	fdc.st[3] = 0;

	fdc.srt = 1 * 2000;
	fdc.hut = 16 * 2000;
	fdc.hlt = 2 * 2000;
	fdc.hd = 0;
	fdc.us = 0;
	fdc.cyl[0] = 0;
	fdc.cyl[1] = 0;
	fdc.cyl[2] = 0;
	fdc.cyl[3] = 0;
	fdc.chrn[0] = 0;
	fdc.chrn[1] = 0;
	fdc.chrn[2] = 0;
	fdc.chrn[3] = 0;

	fdc.eot = 0;
	fdc.gsl = 0;
	fdc.dtl = 0;
	fdc.sc = 0;
	fdc.gpl = 0;
	fdc.d = 0;
	fdc.err = 0;
	fdc.seek = FALSE;
	fdc.ndm = FALSE;
	fdc.mfm = FALSE;
	fdc.mt = FALSE;
	fdc.sk = FALSE;
	fdc.tc = FALSE;
	fdc.load = FALSE;

	fdc.offset = 0;
	fdc.len = 0;

	// �t�F�[�Y�A�R�}���h
	fdc.phase = idle;
	fdc.cmd = fdc_reset;

	// �p�P�b�g�Ǘ�
	fdc.in_len = 0;
	fdc.in_cnt = 0;
	memset(fdc.in_pkt, 0, sizeof(fdc.in_pkt));
	fdc.out_len = 0;
	fdc.out_cnt = 0;
	memset(fdc.out_pkt, 0, sizeof(fdc.out_pkt));

	// �C�x���g��~
	event.SetTime(0);

	// �A�N�Z�X��~
	fdd->Access(FALSE);
}

//---------------------------------------------------------------------------
//
//	�Z�[�u
//
//---------------------------------------------------------------------------
BOOL FASTCALL FDC::Save(Fileio *fio, int ver)
{
	size_t sz;

	ASSERT(this);
	ASSERT(fio);

	LOG0(Log::Normal, "�Z�[�u");

	// �T�C�Y���Z�[�u
	sz = sizeof(fdc_t);
	if (!fio->Write(&sz, sizeof(sz))) {
		return FALSE;
	}

	// �{�̂��Z�[�u
	if (!fio->Write(&fdc, (int)sz)) {
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
BOOL FASTCALL FDC::Load(Fileio *fio, int ver)
{
	size_t sz;

	ASSERT(this);
	ASSERT(fio);

	LOG0(Log::Normal, "���[�h");

	// �T�C�Y�����[�h�A��r
	if (!fio->Read(&sz, sizeof(sz))) {
		return FALSE;
	}
	if (sz != sizeof(fdc_t)) {
		return FALSE;
	}

	// �{�̂����[�h
	if (!fio->Read(&fdc, (int)sz)) {
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
void FASTCALL FDC::ApplyCfg(const Config *config)
{
	ASSERT(this);
	ASSERT(config);

	LOG0(Log::Normal, "�ݒ�K�p");

	// �������[�h
	fdc.fast = config->floppy_speed;
#if defined(FDC_LOG)
	if (fdc.fast) {
		LOG0(Log::Normal, "�������[�h ON");
	}
	else {
		LOG0(Log::Normal, "�������[�h OFF");
	}
#endif	// FDC_LOG

	// 2DD/2HD���p�h���C�u
	fdc.dual = config->dual_fdd;
#if defined(FDC_LOG)
	if (fdc.dual) {
		LOG0(Log::Normal, "2DD/2HD���p�h���C�u");
	}
	else {
		LOG0(Log::Normal, "2HD��p�h���C�u");
	}
#endif	// FDC_LOG
}

//---------------------------------------------------------------------------
//
//	�o�C�g�ǂݍ���
//
//---------------------------------------------------------------------------
DWORD FASTCALL FDC::ReadByte(DWORD addr)
{
	int i;
	int status;
	DWORD bit;
	DWORD data;

	ASSERT(this);
	ASSERT((addr >= memdev.first) && (addr <= memdev.last));

	// ��A�h���X�̂݃f�R�[�h����Ă���
	if ((addr & 1) == 0) {
		return 0xff;
	}

	// 8�o�C�g�P�ʂŃ��[�v
	addr &= 0x07;
	addr >>= 1;

	// �E�F�C�g
	scheduler->Wait(1);

	switch (addr) {
		// �X�e�[�^�X���W�X�^
		case 0:
			return fdc.sr;

		// �f�[�^���W�X�^
		case 1:
			// SEEK�������荞�݂łȂ���΁A���荞�݃l�Q�[�g
			if (!fdc.seek) {
				Interrupt(FALSE);
			}

			switch (fdc.phase) {
				// ���s�t�F�[�Y(ER)
				case read:
					fdc.sr &= ~sr_rqm;
					return Read();

				// ���U���g�t�F�[�Y
				case result:
					ASSERT(fdc.out_cnt >= 0);
					ASSERT(fdc.out_cnt < 0x10);
					ASSERT(fdc.out_len > 0);

					// �p�P�b�g����f�[�^�����o��
					data = fdc.out_pkt[fdc.out_cnt];
					fdc.out_cnt++;
					fdc.out_len--;

					// �c�背���O�X��0�ɂȂ�����A�A�C�h���t�F�[�Y��
					if (fdc.out_len == 0) {
						Idle();
					}
					return data;
			}
			LOG0(Log::Warning, "FDC�f�[�^���W�X�^�ǂݍ��ݖ���");
			return 0xff;

		// �h���C�u�X�e�[�^�X���W�X�^
		case 2:
			data = 0;
			bit = 0x08;
			for (i=3; i>=0; i--) {
				// DCR�̃r�b�g�������Ă��邩
				if ((fdc.dcr & bit) != 0) {
					// �Y���h���C�u�̃X�e�[�^�X��OR(b7,b6�̂�)
					status = fdd->GetStatus(i);
					data |= (DWORD)(status & 0xc0);
				}
				bit >>= 1;
			}

			// FDD���荞�݂𗎂Ƃ�(FDC���荞�݂ł͂Ȃ��A����)
			iosc->IntFDD(FALSE);
			return data;

		// �h���C�u�Z���N�g���W�X�^
		case 3:
			LOG0(Log::Warning, "�h���C�u�Z���N�g���W�X�^�ǂݍ���");
			return 0xff;
	}

	// �ʏ�A�����ɂ͂��Ȃ�
	ASSERT(FALSE);
	return 0xff;
}

//---------------------------------------------------------------------------
//
//	���[�h�ǂݍ���
//
//---------------------------------------------------------------------------
DWORD FASTCALL FDC::ReadWord(DWORD addr)
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
void FASTCALL FDC::WriteByte(DWORD addr, DWORD data)
{
	int i;
	DWORD bit;

	ASSERT(this);
	ASSERT((addr >= memdev.first) && (addr <= memdev.last));

	// ��A�h���X�̂݃f�R�[�h����Ă���
	if ((addr & 1) == 0) {
		return;
	}

	// 8�o�C�g�P�ʂŃ��[�v
	addr &= 0x07;
	addr >>= 1;

	// �E�F�C�g
	scheduler->Wait(1);

	switch (addr) {
		// ����R�}���h���W�X�^
		case 0:
			switch (data) {
				// RESET STANDBY
				case 0x34:
#if defined(FDC_LOG)
					LOG0(Log::Normal, "RESET STANDBY�R�}���h");
#endif	// FDC_LOG
					fdc.cmd = reset_stdby;
					Result();
					return;
				// SET STANDBY
				case 0x35:
#if defined(FDC_LOG)
					LOG0(Log::Normal, "SET STANDBY�R�}���h");
#endif	// FDC_LOG
					fdc.cmd = set_stdby;
					Idle();
					return;
				// SOFTWARE RESET
				case 0x36:
#if defined(FDC_LOG)
					LOG0(Log::Normal, "SOFTWARE RESET�R�}���h");
#endif	// FDC_LOG
					SoftReset();
					return;
			}
			LOG1(Log::Warning, "�����ȓ���R�}���h�������� %02X", data);
			return;

		// �f�[�^���W�X�^
		case 1:
			// SEEK�������荞�݂łȂ���΁A���荞�݃l�Q�[�g
			if (!fdc.seek) {
				Interrupt(FALSE);
			}

			switch (fdc.phase) {
				// �A�C�h���t�F�[�Y
				case idle:
					Command(data);
					return;

				// �R�}���h�t�F�[�Y
				case command:
					ASSERT(fdc.in_cnt >= 0);
					ASSERT(fdc.in_cnt < 0x10);
					ASSERT(fdc.in_len > 0);

					// �p�P�b�g�Ƀf�[�^���Z�b�g
					fdc.in_pkt[fdc.in_cnt] = data;
					fdc.in_cnt++;
					fdc.in_len--;

					// �c�背���O�X��0�ɂȂ�����A���s�t�F�[�Y��
					if (fdc.in_len == 0) {
						Execute();
					}
					return;

				// ���s�t�F�[�Y(EW)
				case write:
					fdc.sr &= ~sr_rqm;
					Write(data);
					return;
			}
			LOG1(Log::Warning, "FDC�f�[�^���W�X�^�������ݖ��� $%02X", data);
			return;

		// �h���C�u�R���g���[�����W�X�^
		case 2:
			// ����4bit��1��0�ɂȂ����Ƃ���𒲂ׂ�
			bit = 0x01;
			for (i=0; i<4; i++) {
				if ((fdc.dcr & bit) != 0) {
					if ((data & bit) == 0) {
						// 1��0�̃G�b�W�ŁADCR�̏��4�r�b�g��K�p
						fdd->Control(i, fdc.dcr);
					}
				}
				bit <<= 1;
			}

			// �l��ۑ�
			fdc.dcr = data;
			return;

		// �h���C�u�Z���N�g���W�X�^
		case 3:
			// ����2bit�ŃA�N�Z�X�h���C�u�I��
			fdc.dsr = (DWORD)(data & 0x03);

			// �ŏ�ʂŃ��[�^����
			if (data & 0x80) {
				fdd->SetMotor(fdc.dsr, TRUE);
			}
			else {
				fdd->SetMotor(fdc.dsr, FALSE);
			}

			// 2HD/2DD�؂�ւ�(�h���C�u��2DD�Ή��łȂ���Ζ���)
			if (fdc.dual) {
				if (data & 0x10) {
					fdd->SetHD(FALSE);
				}
				else {
					fdd->SetHD(TRUE);
				}
			}
			else {
				fdd->SetHD(TRUE);
			}
			return;
	}

	// �ʏ�A�����ɂ͂��Ȃ�
	ASSERT(FALSE);
}

//---------------------------------------------------------------------------
//
//	���[�h��������
//
//---------------------------------------------------------------------------
void FASTCALL FDC::WriteWord(DWORD addr, DWORD data)
{
	ASSERT(this);
	ASSERT((addr >= memdev.first) && (addr <= memdev.last));
	ASSERT((addr & 1) == 0);

	WriteByte(addr + 1, (BYTE)data);
}

//---------------------------------------------------------------------------
//
//	�ǂݍ��݂̂�
//
//---------------------------------------------------------------------------
DWORD FASTCALL FDC::ReadOnly(DWORD addr) const
{
	int i;
	int status;
	DWORD bit;
	DWORD data;

	ASSERT(this);
	ASSERT((addr >= memdev.first) && (addr <= memdev.last));

	// ��A�h���X�̂݃f�R�[�h����Ă���
	if ((addr & 1) == 0) {
		return 0xff;
	}

	// 8�o�C�g�P�ʂŃ��[�v
	addr &= 0x07;
	addr >>= 1;

	switch (addr) {
		// �X�e�[�^�X���W�X�^
		case 0:
			return fdc.sr;

		// �f�[�^���W�X�^
		case 1:
			if (fdc.phase == result) {
				// �p�P�b�g����f�[�^�����o��(�X�V���Ȃ�);
				return fdc.out_pkt[fdc.out_cnt];
			}
			return 0xff;

		// �h���C�u�X�e�[�^�X���W�X�^
		case 2:
			data = 0;
			bit = 0x08;
			for (i=3; i>=0; i--) {
				// DCR�̃r�b�g�������Ă��邩
				if ((fdc.dcr & bit) != 0) {
					// �Y���h���C�u�̃X�e�[�^�X��OR(b7,b6�̂�)
					status = fdd->GetStatus(i);
					data |= (DWORD)(status & 0xc0);
				}
				bit >>= 1;
			}
			return data;

		// �h���C�u�Z���N�g���W�X�^
		case 3:
			return 0xff;
	}

	return 0xff;
}

//---------------------------------------------------------------------------
//
//	�C�x���g�R�[���o�b�N
//
//---------------------------------------------------------------------------
BOOL FASTCALL FDC::Callback(Event *ev)
{
	int i;
	int thres;

	ASSERT(this);
	ASSERT(ev);

	// �A�C�h���t�F�[�Y�̓w�b�h�A�����[�h
	if (fdc.phase == idle) {
		fdc.load = FALSE;

		// �P��
		return FALSE;
	}

	// �w�b�h���[�h
	fdc.load = TRUE;

	// ���s�t�F�[�Y
	if (fdc.phase == execute) {
		// ID��������NO DATA��������܂ł̎���
		Result();

		// �P��
		return FALSE;
	}

	// Write ID�͐�p����
	if (fdc.cmd == write_id) {
		ASSERT(fdc.len > 0);
		ASSERT((fdc.len & 3) == 0);

		// ���ԍĐݒ�
		if (fdc.fast) {
			ev->SetTime(32 * 4);
		}
		else {
			ev->SetTime(fdd->GetRotationTime() / fdc.sc);
		}

		fdc.sr |= sr_rqm;
		dmac->ReqDMA(0);
		fdc.sr |= sr_rqm;
		dmac->ReqDMA(0);
		fdc.sr |= sr_rqm;
		dmac->ReqDMA(0);
		fdc.sr |= sr_rqm;
		dmac->ReqDMA(0);
		return TRUE;
	}

	// Read(Del)Data/Write(Del)Data/Scan/ReadDiag�B���ԍĐݒ�
	EventRW();

	// �f�[�^�]�����N�G�X�g
	fdc.sr |= sr_rqm;

	// �f�[�^�]��
	if (!fdc.ndm) {
		// DMA���[�h(DMA���N�G�X�g)�B64�o�C�g�܂Ƃ߂čs��
		if (fdc.fast) {
			// 1��̃C�x���g�ŁA�]��CPU�p���[��2/3�����]������
			thres = (int)scheduler->GetCPUSpeed();
			thres <<= 1;
			thres /= 3;

			// ���U���g�t�F�[�Y�ɓ���܂ŌJ��Ԃ�
			while (fdc.phase != result) {
				// CPU�p���[�����Ȃ���r���őł��؂�
				if (scheduler->GetCPUCycle() > thres) {
					break;
				}

				// �]��
				dmac->ReqDMA(0);
			}
		}
		else {
			// �ʏ�B64�o�C�g�܂Ƃ߂�
			for (i=0; i<64; i++) {
				if (fdc.phase == result) {
					break;
				}
				dmac->ReqDMA(0);
			}
		}
		return TRUE;
	}

	// Non-DMA���[�h(���荞�݃��N�G�X�g)
	Interrupt(TRUE);
	return TRUE;
}

//---------------------------------------------------------------------------
//
//	�������[�N�A�h���X�擾
//
//---------------------------------------------------------------------------
const FDC::fdc_t* FASTCALL FDC::GetWork() const
{
	ASSERT(this);

	// �A�h���X��^����(fdc.buffer���傫������)
	return &fdc;
}

//---------------------------------------------------------------------------
//
//	�V�[�N����
//
//---------------------------------------------------------------------------
void FASTCALL FDC::CompleteSeek(int drive, BOOL status)
{
	ASSERT(this);
	ASSERT((drive >= 0) && (drive <= 3));

#if defined(FDC_LOG)
	if (status) {
		LOG2(Log::Normal, "�V�[�N���� �h���C�u%d �V�����_%02X",
					drive, fdd->GetCylinder(drive));
	}
	else {
		LOG2(Log::Normal, "�V�[�N���s �h���C�u%d �V�����_%02X",
					drive, fdd->GetCylinder(drive));
	}
#endif	// FDC_LOG

	// recalibrate�܂���seek�̂ݗL��
	if ((fdc.cmd == recalibrate) || (fdc.cmd == seek)) {
		// ST0�쐬(������US�̂�)
		fdc.st[0] = fdc.us;

		// �X�e�[�^�X����
		if (status) {
			// �h���C�u2,3��EC�𗧂Ă�
			if (drive <= 1) {
				// Seek End
				fdc.st[0] |= 0x20;
			}
			else {
				// Equipment Check, Attention Interrupt
				fdc.st[0] |= 0x10;
				fdc.st[0] |= 0xc0;
			}
		}
		else {
			if (drive <= 1) {
				// Seek End
				fdc.st[0] |= 0x20;
			}
			// Not Ready, Abnormal Terminate
			fdc.st[0] |= 0x08;
			fdc.st[0] |= 0x40;
		}

		// SEEK�������荞��
		Interrupt(TRUE);
		fdc.seek = TRUE;
		Idle();
		return;
	}

	LOG1(Log::Warning, "�����ȃV�[�N�����ʒm �h���C�u%d", drive);
}

//---------------------------------------------------------------------------
//
//	TC�A�T�[�g
//
//---------------------------------------------------------------------------
void FASTCALL FDC::SetTC()
{
	ASSERT(this);

	// �A�C�h���t�F�[�Y�ŃN���A���邽�߁A�A�C�h���t�F�[�Y�ȊO�Ȃ��
	if (fdc.phase != idle) {
		fdc.tc = TRUE;
	}
}

//---------------------------------------------------------------------------
//
//	�A�C�h���t�F�[�Y
//
//---------------------------------------------------------------------------
void FASTCALL FDC::Idle()
{
	ASSERT(this);

	// �t�F�[�Y�ݒ�
	fdc.phase = idle;
	fdc.err = 0;
	fdc.tc = FALSE;

	// �C�x���g�I��
	event.SetTime(0);

	// �w�b�h���[�h��ԂȂ�A�A�����[�h�̂��߂̃C�x���g��ݒ�
	if (fdc.load) {
		// �A�����[�h�̕K�v����
		if (fdc.hut > 0) {
			event.SetTime(fdc.hut);
		}
	}

	// �X�e�[�^�X���W�X�^�̓R�}���h�҂�
	fdc.sr = sr_rqm;

	// �A�N�Z�X�I��
	fdd->Access(FALSE);
}

//---------------------------------------------------------------------------
//
//	�R�}���h�t�F�[�Y
//
//---------------------------------------------------------------------------
void FASTCALL FDC::Command(DWORD data)
{
	DWORD mask;

	ASSERT(this);
	ASSERT(data < 0x100);

	// �R�}���h�t�F�[�Y(FDC BUSY)
	fdc.phase = command;
	fdc.sr |= sr_cb;

	// ���̓p�P�b�g������
	fdc.in_pkt[0] = data;
	fdc.in_cnt = 1;
	fdc.in_len = 0;

	// �}�X�N(1)
	mask = data;

	// FDC���Z�b�g�͂��ł����s�ł���
	switch (mask) {
		// RESET STANDBY
		case 0x34:
#if defined(FDC_LOG)
			LOG0(Log::Normal, "RESET STANDBY�R�}���h");
#endif	// FDC_LOG
			fdc.cmd = reset_stdby;
			Result();
			return;

		// SET STANDBY
		case 0x35:
#if defined(FDC_LOG)
			LOG0(Log::Normal, "SET STANDBY�R�}���h");
#endif	// FDC_LOG
			fdc.cmd = set_stdby;
			Idle();
			return;

		// SOFTWARE RESET
		case 0x36:
#if defined(FDC_LOG)
			LOG0(Log::Normal, "SOFTWARE RESET�R�}���h");
#endif	// FDC_LOG
			SoftReset();
			return;
	}

	// �V�[�N�n�R�}���h���s����́ASENSE INTERRUPT STATUS�ȊO������Ȃ�
	if (fdc.seek) {
		// SENSE INTERRUPT STATUS
		if (mask == 0x08) {
#if defined(FDC_LOG)
			LOG0(Log::Normal, "SENSE INTERRUPT STATUS�R�}���h");
#endif	// FDC_LOG
			fdc.cmd = sense_int_stat;

			// ���荞�݃l�Q�[�g
			fdc.sr &= 0xf0;
			fdc.seek = FALSE;
			Interrupt(FALSE);

			// �p�����[�^�Ȃ��A���s�t�F�[�Y�Ȃ�
			Result();
			return;
		}

		// ����ȊO�͑S�Ė����R�}���h
#if defined(FDC_LOG)
		LOG0(Log::Normal, "INVALID�R�}���h");
#endif	// FDC_LOG
		fdc.cmd = invalid;
		Result();
		return;
	}

	// SENSE INTERRUPT STATUS(����)
	if (mask == 0x08) {
#if defined(FDC_LOG)
		LOG0(Log::Normal, "INVALID�R�}���h");
#endif	// FDC_LOG
		fdc.cmd = invalid;
		Result();
		return;
	}

	// �X�e�[�^�X���N���A
	fdc.st[0] = 0;
	fdc.st[1] = 0;
	fdc.st[2] = 0;

	// �ʏ�
	switch (mask) {
		// READ DIAGNOSTIC(FM���[�h)
		case 0x02:
#if defined(FDC_LOG)
			LOG0(Log::Normal, "READ DIAGNOSTIC�R�}���h(FM���[�h)");
#endif	// FDC_LOG
			CommandRW(read_diag, data);
			return;

		// SPECIFY
		case 0x03:
#if defined(FDC_LOG)
			LOG0(Log::Normal, "SPECIFY�R�}���h");
#endif	// FDC_LOG
			fdc.cmd = specify;
			fdc.in_len = 2;
			return;

		// SENSE DEVICE STATUS
		case 0x04:
#if defined(FDC_LOG)
			LOG0(Log::Normal, "SENSE DEVICE STATUS�R�}���h");
#endif	// FDC_LOG
			fdc.cmd = sense_dev_stat;
			fdc.in_len = 1;
			return;

		// RECALIBRATE
		case 0x07:
#if defined(FDC_LOG)
			LOG0(Log::Normal, "RECALIBRATE�R�}���h");
#endif	// FDC_LOG
			fdc.cmd = recalibrate;
			fdc.in_len = 1;
			return;

		// READ ID(FM���[�h)
		case 0x0a:
#if defined(FDC_LOG)
			LOG0(Log::Normal, "READ ID�R�}���h(FM���[�h)");
#endif	// FDC_LOG
			fdc.cmd = read_id;
			fdc.mfm = FALSE;
			fdc.in_len = 1;
			return;

		// WRITE ID(FM���[�h)
		case 0x0d:
#if defined(FDC_LOG)
			LOG0(Log::Normal, "WRITE ID�R�}���h(FM���[�h)");
#endif	// FDC_LOG
			fdc.cmd = write_id;
			fdc.mfm = FALSE;
			fdc.in_len = 5;
			return;

		// SEEK
		case 0x0f:
#if defined(FDC_LOG)
			LOG0(Log::Normal, "SEEK�R�}���h");
#endif	// FDC_LOG
			fdc.cmd = seek;
			fdc.in_len = 2;
			return;

		// READ DIAGNOSTIC(MFM���[�h)
		case 0x42:
#if defined(FDC_LOG)
			LOG0(Log::Normal, "READ DIAGNOSTIC�R�}���h(MFM���[�h)");
#endif	// FDC_LOG
			CommandRW(read_diag, data);
			return;

		// READ ID(MFM���[�h)
		case 0x4a:
#if defined(FDC_LOG)
			LOG0(Log::Normal, "READ ID�R�}���h(MFM���[�h)");
#endif	// FDC_LOG
			fdc.cmd = read_id;
			fdc.mfm = TRUE;
			fdc.in_len = 1;
			return;

		// WRITE ID(MFM���[�h)
		case 0x4d:
#if defined(FDC_LOG)
			LOG0(Log::Normal, "WRITE ID�R�}���h(MFM���[�h)");
#endif	// FDC_LOG
			fdc.cmd = write_id;
			fdc.mfm = TRUE;
			fdc.in_len = 5;
			return;
	}

	// �}�X�N(2)
	mask &= 0x3f;

	// WRITE DATA
	if (mask == 0x05) {
#if defined(FDC_LOG)
		LOG0(Log::Normal, "WRITE DATA�R�}���h");
#endif	// FDC_LOG
		CommandRW(write_data, data);
		return;
	}

	// WRITE DELETED DATA
	if (mask == 0x09) {
#if defined(FDC_LOG)
		LOG0(Log::Normal, "WRITE DELETED DATA�R�}���h");
#endif	// FDC_LOG
		CommandRW(write_del_data, data);
		return;
	}

	// �}�X�N(3);
	mask &= 0x1f;

	// READ DATA
	if (mask == 0x06) {
#if defined(FDC_LOG)
		LOG0(Log::Normal, "READ DATA�R�}���h");
#endif	// FDC_LOG
		CommandRW(read_data, data);
		return;
	}

	// READ DELETED DATA
	if (mask == 0x0c) {
#if defined(FDC_LOG)
		LOG0(Log::Normal, "READ DELETED DATA�R�}���h");
#endif	// FDC_LOG
		CommandRW(read_data, data);
		return;
	}

	// SCAN EQUAL
	if (mask == 0x11) {
#if defined(FDC_LOG)
		LOG0(Log::Normal, "SCAN EQUAL�R�}���h");
#endif	// FDC_LOG
		CommandRW(scan_eq, data);
		return;
	}

	// SCAN LOW OR EQUAL
	if (mask == 0x19) {
#if defined(FDC_LOG)
		LOG0(Log::Normal, "SCAN LOW OR EQUAL�R�}���h");
#endif	// FDC_LOG
		CommandRW(scan_lo_eq, data);
		return;
	}

	// SCAN HIGH OR EQUAL
	if (mask == 0x1d) {
#if defined(FDC_LOG)
		LOG0(Log::Normal, "SCAN HIGH OR EQUAL�R�}���h");
#endif	// FDC_LOG
		CommandRW(scan_hi_eq, data);
		return;
	}

	// ������
	LOG1(Log::Warning, "�R�}���h�t�F�[�Y���Ή��R�}���h $%02X", data);
	Idle();
}

//---------------------------------------------------------------------------
//
//	�R�}���h�t�F�[�Y(Read/Write�n)
//
//---------------------------------------------------------------------------
void FASTCALL FDC::CommandRW(fdccmd cmd, DWORD data)
{
	ASSERT(this);
	ASSERT(data < 0x100);

	// �R�}���h
	fdc.cmd = cmd;

	// MT
	if (data & 0x80) {
		fdc.mt = TRUE;
	}
	else {
		fdc.mt = FALSE;
	}

	// MFM
	if (data & 0x40) {
		fdc.mfm = TRUE;
	}
	else {
		fdc.mfm = FALSE;
	}

	// SK(READ/SCAN�̂�)
	if (data & 0x20) {
		fdc.sk = TRUE;
	}
	else {
		fdc.sk = FALSE;
	}

	// �R�}���h�t�F�[�Y�̎c��o�C�g��
	fdc.in_len = 8;
}

//---------------------------------------------------------------------------
//
//	���s�t�F�[�Y
//
//---------------------------------------------------------------------------
void FASTCALL FDC::Execute()
{
	ASSERT(this);

	// ���s�t�F�[�Y��
	fdc.phase = execute;

	// �A�N�Z�X�J�n�A�C�x���g��~
	fdd->Access(TRUE);
	event.SetTime(0);

	// �R�}���h��
	switch (fdc.cmd) {
		// SPECIFY
		case specify:
			// SRT
			fdc.srt = (fdc.in_pkt[1] >> 4) & 0x0f;
			fdc.srt = 16 - fdc.srt;
			fdc.srt <<= 11;

			// HUT (0��16�Ɠ��������Bi82078�f�[�^�V�[�g�ɂ��)
			fdc.hut = fdc.in_pkt[1] & 0x0f;
			if (fdc.hut == 0) {
				fdc.hut = 16;
			}
			fdc.hut <<= 15;

			// HLT (0��HUT�Ɠ��l)
			fdc.hlt = (fdc.in_pkt[2] >> 1) & 0x7f;
			if (fdc.hlt == 0) {
				fdc.hlt = 0x80;
			}
			fdc.hlt <<= 12;

			// NDM
			if (fdc.in_pkt[2] & 1) {
				fdc.ndm = TRUE;
				LOG0(Log::Warning, "Non-DMA���[�h�ɐݒ�");
			}
			else {
				fdc.ndm = FALSE;
			}

			// ���U���g�t�F�[�Y�s�v
			Idle();
			return;

		// SENSE DEVICE STATUS
		case sense_dev_stat:
			fdc.us = fdc.in_pkt[1] & 0x03;
			fdc.hd = fdc.in_pkt[1] & 0x04;

			// ���U���g�t�F�[�Y
			Result();
			return;

		// RECALIBRATE
		case recalibrate:
			// �g���b�N0�փV�[�N
			fdc.us = fdc.in_pkt[1] & 0x03;
			fdc.cyl[fdc.us] = 0;

			// SR�쐬(SEEK�n�R�}���h���s����Non-Busy)
			fdc.sr &= 0xf0;
			fdc.sr &= ~sr_cb;
			fdc.sr &= ~sr_rqm;
			fdc.sr |= (1 << fdc.dsr);

			// �Ō�Ɏ��s���Ă�(������CompleteSeek���Ă΂�邽��)
			fdd->Recalibrate(fdc.srt);
			return;

		// SEEK
		case seek:
			fdc.us = fdc.in_pkt[1] & 0x03;

			// SR�쐬(SEEK�n�R�}���h���s����Non-Busy)
			fdc.sr &= 0xf0;
			fdc.sr &= ~sr_cb;
			fdc.sr &= ~sr_rqm;
			fdc.sr |= (1 << fdc.dsr);

			// �Ō�Ɏ��s���Ă�(������CompleteSeek���Ă΂�邽��)
			if (fdc.cyl[fdc.us] < fdc.in_pkt[2]) {
				// �X�e�b�v�C��
				fdd->StepIn(fdc.in_pkt[2] - fdc.cyl[fdc.us], fdc.srt);
			}
			else {
				// �X�e�b�v�A�E�g
				fdd->StepOut(fdc.cyl[fdc.us] - fdc.in_pkt[2], fdc.srt);
			}
			fdc.cyl[fdc.us] = fdc.in_pkt[2];
			return;

		// READ ID
		case read_id:
			ReadID();
			return;

		// WRITE ID
		case write_id:
			fdc.us = fdc.in_pkt[1] & 0x03;
			fdc.hd = fdc.in_pkt[1] & 0x04;
			fdc.st[0] = fdc.us;
			fdc.st[0] |= fdc.hd;
			fdc.chrn[3] = fdc.in_pkt[2];
			fdc.sc = fdc.in_pkt[3];
			fdc.gpl = fdc.in_pkt[4];
			fdc.d = fdc.in_pkt[5];
			if (!WriteID()) {
				Result();
			}
			return;

		// READ DIAGNOSTIC
		case read_diag:
			ExecuteRW();
			if (!ReadDiag()) {
				Result();
			}
			return;

		// READ DATA
		case read_data:
			ExecuteRW();
			if (!ReadData()) {
				Result();
			}
			return;

		// READ DELETED DATA
		case read_del_data:
			ExecuteRW();
			if (!ReadData()) {
				Result();
			}
			return;

		// WRITE DATA
		case write_data:
			ExecuteRW();
			if (!WriteData()) {
				Result();
			}
			return;

		// WRITE DELETED_DATA
		case write_del_data:
			ExecuteRW();
			if (!WriteData()) {
				Result();
			}
			return;

		// SCAN�n
		case scan_eq:
		case scan_lo_eq:
		case scan_hi_eq:
			ExecuteRW();
			if (!Scan()) {
				Result();
			}
			return;
	}

	LOG1(Log::Warning, "���s�t�F�[�Y���Ή��R�}���h $%02X", fdc.in_pkt[0]);
}

//---------------------------------------------------------------------------
//
//	���s�t�F�[�Y(ReadID)
//
//---------------------------------------------------------------------------
void FASTCALL FDC::ReadID()
{
	DWORD hus;

	ASSERT(this);

	// HD, US���L��
	fdc.us = fdc.in_pkt[1] & 0x03;
	fdc.hd = fdc.in_pkt[1] & 0x04;

	// FDD�Ɏ��s������BNOTREADY, NODATA, MAM���l������
	fdc.err = fdd->ReadID(&(fdc.out_pkt[3]), fdc.mfm, fdc.hd);

	// NOT READY�Ȃ炷�����U���g�t�F�[�Y
	if (fdc.err & FDD_NOTREADY) {
		Result();
		return;
	}

	// �����ɂ����鎞�Ԃ�ݒ�
	hus = fdd->GetSearch();
	event.SetTime(hus);
	fdc.sr &= ~sr_rqm;
}

//---------------------------------------------------------------------------
//
//	���s�t�F�[�Y(Read/Write�n)
//
//---------------------------------------------------------------------------
void FASTCALL FDC::ExecuteRW()
{
	ASSERT(this);

	// 8�o�C�g�̃p�P�b�g�𕪊�(�ŏI�o�C�g�͏��DTL�ɃZ�b�g)
	fdc.us = fdc.in_pkt[1] & 0x03;
	fdc.hd = fdc.in_pkt[1] & 0x04;
	fdc.st[0] = fdc.us;
	fdc.st[0] |= fdc.hd;

	fdc.chrn[0] = fdc.in_pkt[2];
	fdc.chrn[1] = fdc.in_pkt[3];
	fdc.chrn[2] = fdc.in_pkt[4];
	fdc.chrn[3] = fdc.in_pkt[5];

	fdc.eot = fdc.in_pkt[6];
	fdc.gsl = fdc.in_pkt[7];
	fdc.dtl = fdc.in_pkt[8];
}

//---------------------------------------------------------------------------
//
//	���s�t�F�[�Y(Read)
//
//---------------------------------------------------------------------------
BYTE FASTCALL FDC::Read()
{
	BYTE data;

	ASSERT(fdc.len > 0);
	ASSERT(fdc.offset < 0x4000);

	// �o�b�t�@����f�[�^������
	data = fdc.buffer[fdc.offset];
	fdc.offset++;
	fdc.len--;

	// �Ō�łȂ���΂��̂܂ܑ�����
	if (fdc.len > 0) {
		return data;
	}

	// READ DIAGNOSTIC�̏ꍇ�͂����ŏI��
	if (fdc.cmd == read_diag) {
		// ����I���Ȃ�A�Z�N�^��i�߂�
		if (fdc.err == FDD_NOERROR) {
			NextSector();
		}
		// �C�x���g��ł��؂�A���U���g�t�F�[�Y��
		event.SetTime(0);
		Result();
		return data;
	}

	// �ُ�I���Ȃ�A���̃Z�N�^�őł��؂�
	if (fdc.err != FDD_NOERROR) {
		// �C�x���g��ł��؂�A���U���g�t�F�[�Y��
		event.SetTime(0);
		Result();
		return data;
	}

	// �}���`�Z�N�^����
	if (!NextSector()) {
		// �C�x���g��ł��؂�A���U���g�t�F�[�Y��
		event.SetTime(0);
		Result();
		return data;
	}

	// ���̃Z�N�^������̂ŁA����
	if (!ReadData()) {
		// �Z�N�^�ǂݎ��s�
		event.SetTime(0);
		Result();
		return data;
	}

	// OK�A���̃Z�N�^��
	return data;
}

//---------------------------------------------------------------------------
//
//	���s�t�F�[�Y(Write)
//
//---------------------------------------------------------------------------
void FASTCALL FDC::Write(DWORD data)
{
	ASSERT(this);
	ASSERT(fdc.len > 0);
	ASSERT(fdc.offset < 0x4000);
	ASSERT(data < 0x100);

	// WRITE ID�̏ꍇ�̓o�b�t�@�ɗ��߂�̂�
	if (fdc.cmd == write_id) {
		fdc.buffer[fdc.offset] = (BYTE)data;
		fdc.offset++;
		fdc.len--;

		// �I���`�F�b�N
		if (fdc.len == 0) {
			WriteBack();
			event.SetTime(0);
			Result();
		}
		return;
	}

	// �X�L�����n�̏ꍇ�͔�r
	if ((fdc.cmd == scan_eq) || (fdc.cmd == scan_lo_eq) || (fdc.cmd == scan_hi_eq)) {
		Compare(data);
		return;
	}

	// �o�b�t�@�փf�[�^����������
	fdc.buffer[fdc.offset] = (BYTE)data;
	fdc.offset++;
	fdc.len--;

	// �Ō�łȂ���΂��̂܂ܑ�����
	if (fdc.len > 0) {
		return;
	}

	// �������ݏI��
	WriteBack();
	if (fdc.err != FDD_NOERROR) {
		event.SetTime(0);
		Result();
		return;
	}

	// �}���`�Z�N�^����
	if (!NextSector()) {
		// �C�x���g��ł��؂�A���U���g�t�F�[�Y��
		event.SetTime(0);
		Result();
		return;
	}

	// ���̃Z�N�^������̂ŁA����
	if (!WriteData()) {
		event.SetTime(0);
		Result();
	}
}

//---------------------------------------------------------------------------
//
//	���s�t�F�[�Y(Compare)
//
//---------------------------------------------------------------------------
void FASTCALL FDC::Compare(DWORD data)
{
	ASSERT(this);
	ASSERT(data < 0x100);

	if (data != 0xff) {
		// �L���o�C�g�ŁA�܂�����o�ĂȂ��Ȃ�
		if (!(fdc.err & FDD_SCANNOT)) {
			// ��r���K�v
			switch (fdc.cmd) {
				case scan_eq:
					if (fdc.buffer[fdc.offset] != (BYTE)data) {
						fdc.err |= FDD_SCANNOT;
					}
					break;
				case scan_lo_eq:
					if (fdc.buffer[fdc.offset] > (BYTE)data) {
						fdc.err |= FDD_SCANNOT;
					}
					break;
				case scan_hi_eq:
					if (fdc.buffer[fdc.offset] < (BYTE)data) {
						fdc.err |= FDD_SCANNOT;
					}
					break;
				default:
					ASSERT(FALSE);
					break;
			}

		}
	}

	// ���̃f�[�^��
	fdc.offset++;
	fdc.len--;

	// �Ō�łȂ���΂��̂܂ܑ�����
	if (fdc.len > 0) {
		return;
	}

	// �Ō�Ȃ̂ŁA���ʂ܂Ƃ�
	if (!(fdc.err & FDD_SCANNOT)) {
		// ok!
		fdc.err |= FDD_SCANEQ;
		event.SetTime(0);
		Result();
	}

	// STP��2�̂Ƃ��́A+1
	if (fdc.dtl == 0x02) {
		fdc.chrn[2]++;
	}

	// �}���`�Z�N�^����
	if (!NextSector()) {
		// SCAN NOT�͏オ�����܂܂Ȃ̂œs�����悢
		event.SetTime(0);
		Result();
		return;
	}

	// ���̃Z�N�^������̂ŁA����
	if (!Scan()) {
		// SCAN NOT�͏オ�����܂܂Ȃ̂œs�����悢
		event.SetTime(0);
		Result();
	}

	// SCAN NOT�������Ă����P�Z�N�^
	fdc.err &= ~FDD_SCANNOT;
}

//---------------------------------------------------------------------------
//
//	���U���g�t�F�[�Y
//
//---------------------------------------------------------------------------
void FASTCALL FDC::Result()
{
	ASSERT(this);

	// ���U���g�t�F�[�Y
	fdc.phase = result;
	fdc.sr |= sr_rqm;
	fdc.sr |= sr_dio;
	fdc.sr &= ~sr_ndm;

	// �R�}���h��
	switch (fdc.cmd) {
		// SENSE DEVICE STATUS
		case sense_dev_stat:
			// ST3���쐬�A�f�[�^�]��
			MakeST3();
			fdc.out_pkt[0] = fdc.st[3];
			fdc.out_len = 1;
			fdc.out_cnt = 0;
			return;

		// SENSE INTERRUPT STATUS
		case sense_int_stat:
			// ST0�E�V�����_��Ԃ��B�f�[�^�]��
			fdc.out_pkt[0] = fdc.st[0];
			fdc.out_pkt[1] = fdc.cyl[fdc.us];
			fdc.out_len = 2;
			fdc.out_cnt = 0;
			return;

		// READ ID
		case read_id:
			// ST0,ST1,ST2�쐬�BNOTREADY, NODATA, MAM���l������
			fdc.st[0] = fdc.us;
			fdc.st[0] |= fdc.hd;
			if (fdc.err & FDD_NOTREADY) {
				// Not Ready
				fdc.st[0] |= 0x08;
				fdc.st[1] = 0;
				fdc.st[2] = 0;
			}
			else {
				if (fdc.err != FDD_NOERROR) {
					// Abnormal Teriminate
					fdc.st[0] |= 0x40;
				}
				fdc.st[1] = fdc.err >> 8;
				fdc.st[2] = fdc.err & 0xff;
			}

			// �f�[�^�]���A���U���g�t�F�[�Y���荞��
			fdc.out_pkt[0] = fdc.st[0];
			fdc.out_pkt[1] = fdc.st[1];
			fdc.out_pkt[2] = fdc.st[2];
			fdc.out_len = 7;
			fdc.out_cnt = 0;
			Interrupt(TRUE);
			return;

		// INVALID, RESET STANDBY
		case invalid:
		case reset_stdby:
			fdc.out_pkt[0] = 0x80;
			fdc.out_len = 1;
			fdc.out_cnt = 0;
			return;

		// READ,WRITE,SCAN�n
		case read_data:
		case read_del_data:
		case write_data:
		case write_del_data:
		case scan_eq:
		case scan_lo_eq:
		case scan_hi_eq:
		case read_diag:
		case write_id:
			ResultRW();
			return;
	}

	LOG1(Log::Warning, "���U���g�t�F�[�Y���Ή��R�}���h $%02X", fdc.in_pkt[0]);
}

//---------------------------------------------------------------------------
//
//	���U���g�t�F�[�Y(Read/Write�n)
//
//---------------------------------------------------------------------------
void FASTCALL FDC::ResultRW()
{
	ASSERT(this);

	// ST0,ST1,ST2�쐬
	if (fdc.err & FDD_NOTREADY) {
		// Not Ready
		fdc.st[0] |= 0x08;
		fdc.st[1] = 0;
		fdc.st[2] = 0;
	}
	else {
		if ((fdc.err != FDD_NOERROR) && (fdc.err != FDD_SCANEQ)) {
			// Abnormal Teriminate
			fdc.st[0] |= 0x40;
		}
		fdc.st[1] = fdc.err >> 8;
		fdc.st[2] = fdc.err & 0xff;
	}

	// READ DIAGNOSTIC��0x40���o���Ȃ�
	if (fdc.cmd == read_diag) {
		if (fdc.st[0] & 0x40) {
			fdc.st[0] &= ~0x40;
		}
	}

	// ���U���g�p�P�b�g��ݒ�
	fdc.out_pkt[0] = fdc.st[0];
	fdc.out_pkt[1] = fdc.st[1];
	fdc.out_pkt[2] = fdc.st[2];
	fdc.out_pkt[3] = fdc.chrn[0];
	fdc.out_pkt[4] = fdc.chrn[1];
	fdc.out_pkt[5] = fdc.chrn[2];
	fdc.out_pkt[6] = fdc.chrn[3];
	fdc.out_len = 7;
	fdc.out_cnt = 0;

	// �ʏ�̓��U���g�t�F�[�Y���荞��
	Interrupt(TRUE);
}

//---------------------------------------------------------------------------
//
//	���荞��
//
//---------------------------------------------------------------------------
void FASTCALL FDC::Interrupt(BOOL flag)
{
	ASSERT(this);

	// IOSC�ɒʒm
	iosc->IntFDC(flag);
}

//---------------------------------------------------------------------------
//
//	ST3�쐬
//
//---------------------------------------------------------------------------
void FASTCALL FDC::MakeST3()
{
	ASSERT(this);

	// HD,US���Z�b�g
	fdc.st[3] = fdc.hd;
	fdc.st[3] |= fdc.us;

	// ���f�B��
	if (fdd->IsReady(fdc.dsr)) {
		// ���f�B
		fdc.st[3] |= 0x20;

		// ���C�g�v���e�N�g��
		if (fdd->IsWriteP(fdc.dsr)) {
			fdc.st[3] |= 0x40;
		}
	}
	else {
		// ���f�B�łȂ�
		fdc.st[3] = 0x40;
	}

	// TRACK0��
	if (fdd->GetCylinder(fdc.dsr) == 0) {
		fdc.st[3] |= 0x10;
	}
}

//---------------------------------------------------------------------------
//
//	READ (DELETED) DATA�R�}���h
//
//---------------------------------------------------------------------------
BOOL FASTCALL FDC::ReadData()
{
	int len;
	DWORD hus;

	ASSERT(this);
	ASSERT((fdc.cmd == read_data) || (fdc.cmd == read_del_data));

	// SR�ݒ�
	fdc.sr |= sr_cb;
	fdc.sr |= sr_dio;
	fdc.sr &= ~sr_d3b;
	fdc.sr &= ~sr_d2b;
	fdc.sr &= ~sr_d1b;
	fdc.sr &= ~sr_d0b;

	// �h���C�u�ɔC����BNOTREADY,NODATA,MAM,CYL�n,CRC�n,DDAM
#if defined(FDC_LOG)
	LOG4(Log::Normal, "(C:%02X H:%02X R:%02X N:%02X)",
		fdc.chrn[0], fdc.chrn[1], fdc.chrn[2], fdc.chrn[3]);
#endif
	fdc.err = fdd->ReadSector(fdc.buffer, &fdc.len,
									fdc.mfm, fdc.chrn, fdc.hd);

	// DDAM(Deleted Sector)�̗L���ŁACM(Control Mark)�����߂�
	if (fdc.cmd == read_data) {
		// Read Data (DDAM�̓G���[)
		if (fdc.err & FDD_DDAM) {
			fdc.err &= ~FDD_DDAM;
			fdc.err |= FDD_CM;
		}
	}
	else {
		// Read Deleted Data (DDAM�łȂ���΃G���[)
		if (!(fdc.err & FDD_DDAM)) {
			fdc.err |= FDD_CM;
		}
		fdc.err &= ~FDD_DDAM;
	}

	// IDCRC�܂���DATACRC�Ȃ�ADATAERR���悹��
	if ((fdc.err & FDD_IDCRC) || (fdc.err & FDD_DATACRC)) {
		fdc.err &= ~FDD_IDCRC;
		fdc.err |= FDD_DATAERR;
	}

	// N=0�ł�N�Ƃ���DTL���g��
	if (fdc.chrn[3] == 0) {
		len = 1 << (fdc.dtl + 7);
		if (len < fdc.len) {
			fdc.len = len;
		}
	}
	else {
		// (Mac�G�~�����[�^)
		len = (1 << (fdc.chrn[3] + 7));
		if (len < fdc.len) {
			fdc.len = len;
		}
	}

	// Not Ready�̓��U���g�t�F�[�Y��
	if (fdc.err & FDD_NOTREADY) {
		return FALSE;
	}

	// CM��SK=1�Ȃ烊�U���g�t�F�[�Y��(i82078�f�[�^�V�[�g�ɂ��)
	if (fdc.err & FDD_CM) {
		if (fdc.sk) {
			return FALSE;
		}
	}

	// �������Ԃ��v�Z(�w�b�h���[�h�͍l���Ȃ�)
	hus = fdd->GetSearch();

	// No Data���͂��̎��Ԍ�A���U���g�t�F�[�Y��
	if (fdc.err & FDD_NODATA) {
		EventErr(hus);
		return TRUE;
	}

	// �I�t�Z�b�g�������A�C�x���g�X�^�[�g�AER�t�F�[�Y�J�n
	fdc.offset = 0;
	EventRW();
	fdc.phase = read;

	// �������Ԃ��w�b�h���[�h���Ԃ��Z����΁A�P���҂�
	if (!fdc.load) {
		if (hus < fdc.hlt) {
			hus += fdd->GetRotationTime();
		}
	}

	// ���Ԃ����Z
	if (!fdc.fast) {
		hus += event.GetTime();
		event.SetTime(hus);
	}
	return TRUE;
}

//---------------------------------------------------------------------------
//
//	WRITE (DELETED) DATA�R�}���h
//
//---------------------------------------------------------------------------
BOOL FASTCALL FDC::WriteData()
{
	int len;
	DWORD hus;
	BOOL deleted;

	ASSERT(this);
	ASSERT((fdc.cmd == write_data) || (fdc.cmd == write_del_data));

	// SR�ݒ�
	fdc.sr |= sr_cb;
	fdc.sr &= ~sr_dio;
	fdc.sr &= ~sr_d3b;
	fdc.sr &= ~sr_d2b;
	fdc.sr &= ~sr_d1b;
	fdc.sr &= ~sr_d0b;

	// �h���C�u�ɔC����BNOTREADY,NOTWRITE,NODATA,MAM,CYL�n,IDCRC,DDAM
	deleted = FALSE;
	if (fdc.cmd == write_del_data) {
		deleted = TRUE;
	}
#if defined(FDC_LOG)
	LOG4(Log::Normal, "(C:%02X H:%02X R:%02X N:%02X)",
		fdc.chrn[0], fdc.chrn[1], fdc.chrn[2], fdc.chrn[3]);
#endif
	fdc.err = fdd->WriteSector(NULL, &fdc.len,
									fdc.mfm, fdc.chrn, fdc.hd, deleted);
	fdc.err &= ~FDD_DDAM;

	// IDCRC�Ȃ�ADATAERR���悹��
	if (fdc.err & FDD_IDCRC) {
		fdc.err &= ~FDD_IDCRC;
		fdc.err |= FDD_DATAERR;
	}

	// N=0�ł�N�Ƃ���DTL���g��
	if (fdc.chrn[3] == 0) {
		len = 1 << (fdc.dtl + 7);
		if (len < fdc.len) {
			fdc.len = len;
		}
	}
	else {
		len = (1 << (fdc.chrn[3] + 7));
		if (len < fdc.len) {
			fdc.len = len;
		}
	}

	// Not Ready, Not Writable�̓��U���g�t�F�[�Y��
	if ((fdc.err & FDD_NOTREADY) || (fdc.err & FDD_NOTWRITE)) {
		return FALSE;
	}

	// �������Ԃ��v�Z(�w�b�h���[�h�͍l���Ȃ�)
	hus = fdd->GetSearch();

	// No Data�͎��s�ナ�U���g��
	if (fdc.err & FDD_NODATA) {
		EventErr(hus);
		return TRUE;
	}

	// �I�t�Z�b�g�������A�C�x���g�ݒ�AEW�t�F�[�Y�J�n
	fdc.offset = 0;
	EventRW();
	fdc.phase = write;

	// �������Ԃ��w�b�h���[�h���Ԃ��Z����΁A�P���҂�
	if (!fdc.load) {
		if (hus < fdc.hlt) {
			hus += fdd->GetRotationTime();
		}
	}

	// ���Ԃ����Z
	if (!fdc.fast) {
		hus += event.GetTime();
		event.SetTime(hus);
	}
	return TRUE;
}

//---------------------------------------------------------------------------
//
//	SCAN�n�R�}���h
//
//---------------------------------------------------------------------------
BOOL FASTCALL FDC::Scan()
{
	int len;
	DWORD hus;

	ASSERT(this);

	// SR�ݒ�
	fdc.sr |= sr_cb;
	fdc.sr &= ~sr_dio;
	fdc.sr &= ~sr_d3b;
	fdc.sr &= ~sr_d2b;
	fdc.sr &= ~sr_d1b;
	fdc.sr &= ~sr_d0b;

	// �h���C�u�ɔC����BNOTREADY,NODATA,MAM,CYL�n,CRC�n,DDAM
#if defined(FDC_LOG)
	LOG4(Log::Normal, "(C:%02X H:%02X R:%02X N:%02X)",
		fdc.chrn[0], fdc.chrn[1], fdc.chrn[2], fdc.chrn[3]);
#endif
	fdc.err = fdd->ReadSector(fdc.buffer, &fdc.len,
									fdc.mfm, fdc.chrn, fdc.hd);

	// DDAM(Deleted Sector)�̗L���ŁACM(Control Mark)�����߂�
	if (fdc.err & FDD_DDAM) {
		fdc.err &= ~FDD_DDAM;
		fdc.err |= FDD_CM;
	}

	// IDCRC�܂���DATACRC�Ȃ�ADATAERR���悹��
	if ((fdc.err & FDD_IDCRC) || (fdc.err & FDD_DATACRC)) {
		fdc.err &= ~FDD_IDCRC;
		fdc.err |= FDD_DATAERR;
	}

	// N=0�ł�N�Ƃ���DTL���g��
	if (fdc.chrn[3] == 0) {
		len = 1 << (fdc.dtl + 7);
		if (len < fdc.len) {
			fdc.len = len;
		}
	}
	else {
		len = (1 << (fdc.chrn[3] + 7));
		if (len < fdc.len) {
			fdc.len = len;
		}
	}

	// Not Ready�̓��U���g�t�F�[�Y��
	if (fdc.err & FDD_NOTREADY) {
		return FALSE;
	}

	// CM��SK=1�Ȃ烊�U���g�t�F�[�Y��(i82078�f�[�^�V�[�g�ɂ��)
	if (fdc.err & FDD_CM) {
		if (fdc.sk) {
			return FALSE;
		}
	}

	// �������Ԃ��v�Z(�w�b�h���[�h�͍l���Ȃ�)
	hus = fdd->GetSearch();

	// No Data���͂��̎��Ԍ�A���U���g�t�F�[�Y��
	if (fdc.err & FDD_NODATA) {
		EventErr(hus);
		return TRUE;
	}

	// �I�t�Z�b�g�������A�C�x���g�X�^�[�g�AER�t�F�[�Y�J�n
	fdc.offset = 0;
	EventRW();
	fdc.phase = write;

	// �������Ԃ��w�b�h���[�h���Ԃ��Z����΁A�P���҂�
	if (!fdc.load) {
		if (hus < fdc.hlt) {
			hus += fdd->GetRotationTime();
		}
	}

	// ���Ԃ����Z
	if (!fdc.fast) {
		hus += event.GetTime();
		event.SetTime(hus);
	}
	return TRUE;
}

//---------------------------------------------------------------------------
//
//	READ DIAGNOSTIC�R�}���h
//
//---------------------------------------------------------------------------
BOOL FASTCALL FDC::ReadDiag()
{
	DWORD hus;

	ASSERT(this);
	ASSERT(fdc.cmd == read_diag);

	// SR�ݒ�
	fdc.sr |= sr_cb;
	fdc.sr |= sr_dio;
	fdc.sr &= ~sr_d3b;
	fdc.sr &= ~sr_d2b;
	fdc.sr &= ~sr_d1b;
	fdc.sr &= ~sr_d0b;

	// EOT=0�̓��U���g�t�F�[�Y��(NO DATA)
	if (fdc.eot == 0) {
		if (fdd->IsReady(fdc.dsr)) {
			fdc.err = FDD_NODATA;
		}
		else {
			fdc.err = FDD_NOTREADY;
		}
		return FALSE;
	}

	// �h���C�u�ɔC����BNOTREADY,NODATA,MAM,CRC�n,DDAM
	fdc.err = fdd->ReadDiag(fdc.buffer, &fdc.len, fdc.mfm,
								fdc.chrn, fdc.hd);
	// Not Ready�̓��U���g�t�F�[�Y��
	if (fdc.err & FDD_NOTREADY) {
		return FALSE;
	}

	// �������Ԃ��v�Z(�w�b�h���[�h�͍l���Ȃ�)
	hus = fdd->GetSearch();

	// MAM�Ȃ玞�ԑ҂���A���U���g�t�F�[�Y�ցBNODATA�ł������邽��
	if (fdc.err & FDD_MAM) {
		EventErr(hus);
		return TRUE;
	}

	ASSERT(fdc.len > 0);

	// �I�t�Z�b�g�������A�C�x���g�X�^�[�g�AER�t�F�[�Y�J�n
	fdc.offset = 0;
	EventRW();
	fdc.phase = read;

	// ���Ԃ����Z
	if (!fdc.fast) {
		hus += event.GetTime();
		event.SetTime(hus);
	}
	return TRUE;
}

//---------------------------------------------------------------------------
//
//	WRITE ID�R�}���h
//
//---------------------------------------------------------------------------
BOOL FASTCALL FDC::WriteID()
{
	DWORD hus;

	ASSERT(this);
	ASSERT(fdc.cmd == write_id);

	// SR�ݒ�
	fdc.sr |= sr_cb;
	fdc.sr &= ~sr_dio;
	fdc.sr &= ~sr_d3b;
	fdc.sr &= ~sr_d2b;
	fdc.sr &= ~sr_d1b;
	fdc.sr &= ~sr_d0b;

	// SC=0�`�F�b�N
	if (fdc.sc == 0) {
		fdc.err = 0;
		return FALSE;
	}

	// �h���C�u�ɔC����BNOTREADY,NOTWRITE
	fdc.err = fdd->WriteID(NULL, fdc.d, fdc.sc, fdc.mfm, fdc.hd, fdc.gpl);
	// Not Ready, Not Writable�̓��U���g�t�F�[�Y��
	if ((fdc.err & FDD_NOTREADY) || (fdc.err & FDD_NOTWRITE)) {
		return FALSE;
	}

	// �I�t�Z�b�g������
	fdc.offset = 0;
	fdc.len = fdc.sc * 4;

	// �C�x���g�ݒ�
	if (fdc.ndm) {
		fdc.sr |= sr_ndm;
		LOG0(Log::Warning, "Non-DMA���[�h��Write ID");
	}
	else {
		fdc.sr &= ~sr_ndm;
	}
	// N��7�܂łɐ���(N=7��16KB/sector, �A���t�H�[�}�b�g)
	if (fdc.chrn[3] > 7) {
		fdc.chrn[3] = 7;
	}

	// ���Ԃ�ݒ�B�P������Z�N�^�Ŋ�������
	hus = fdd->GetSearch();
	hus += (fdd->GetRotationTime() / fdc.sc);
	if (fdc.fast) {
		hus = 32 * 4;
	}

	// �C�x���g�X�^�[�g�ARQM�𗎂Ƃ�
	event.SetTime(hus);
	fdc.sr &= ~sr_rqm;
	fdc.phase = write;

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	�C�x���g(Read/Write�n)
//
//---------------------------------------------------------------------------
void FASTCALL FDC::EventRW()
{
	DWORD hus;

	// SR�ݒ�(Non-DMA)
	if (fdc.ndm) {
		fdc.sr |= sr_ndm;
	}
	else {
		fdc.sr &= ~sr_ndm;
	}

	// �C�x���g�쐬
	if (fdc.ndm) {
		// Non-DMA�B16us/32us
		if (fdc.mfm) {
			hus = 32;
		}
		else {
			hus = 64;
		}
	}
	else {
		// DMA��64�o�C�g�܂Ƃ߂čs���B1024us/2048us
		if (fdc.mfm) {
			hus = 32 * 64;
		}
		else {
			hus = 64 * 64;
		}
	}

	// DD�͂��̔{
	if (!fdd->IsHD()) {
		hus <<= 1;
	}

	// fast����64us�Œ�(DMA�Ɍ���)
	if (fdc.fast) {
		if (!fdc.ndm) {
			hus = 128;
		}
	}

	// �C�x���g�X�^�[�g�ARQM�𗎂Ƃ�
	event.SetTime(hus);
	fdc.sr &= ~sr_rqm;
}

//---------------------------------------------------------------------------
//
//	�C�x���g(�G���[)
//
//---------------------------------------------------------------------------
void FASTCALL FDC::EventErr(DWORD hus)
{
	// SR�ݒ�(Non-DMA)
	if (fdc.ndm) {
		fdc.sr |= sr_ndm;
	}
	else {
		fdc.sr &= ~sr_ndm;
	}

	// �C�x���g�X�^�[�g�ARQM�𗎂Ƃ�
	event.SetTime(hus);
	fdc.sr &= ~sr_rqm;
	fdc.phase = execute;
}

//---------------------------------------------------------------------------
//
//	�������݊���
//
//---------------------------------------------------------------------------
void FASTCALL FDC::WriteBack()
{
	switch (fdc.cmd) {
		// Write Data
		case write_data:
			fdc.err = fdd->WriteSector(fdc.buffer, &fdc.len,
							fdc.mfm, fdc.chrn, fdc.hd, FALSE);
			return;

		// Write Deleted Data
		case write_del_data:
			fdc.err = fdd->WriteSector(fdc.buffer, &fdc.len,
							fdc.mfm, fdc.chrn, fdc.hd, TRUE);
			return;

		// Write ID
		case write_id:
			fdc.err = fdd->WriteID(fdc.buffer, fdc.d, fdc.sc,
							fdc.mfm, fdc.hd, fdc.gpl);
			return;
	}

	// ���肦�Ȃ�
	ASSERT(FALSE);
}

//---------------------------------------------------------------------------
//
//	���Z�N�^
//
//---------------------------------------------------------------------------
BOOL FASTCALL FDC::NextSector()
{
	// TC�`�F�b�N
	if (fdc.tc) {
		// C,H,R,N���ړ�
		if (fdc.chrn[2] < fdc.eot) {
			fdc.chrn[2]++;
			return FALSE;
		}
		fdc.chrn[2] = 0x01;
		// MT�ɂ���ĕ�����
		if (fdc.mt && (!(fdc.chrn[1] & 0x01))) {
			// �T�C�h1��
			fdc.chrn[1] |= 0x01;
			fdc.hd |= 0x04;
			return FALSE;
		}
		// C+1, R=1�ŏI��
		fdc.chrn[0]++;
		return FALSE;
	}

	// EOT�`�F�b�N
	if (fdc.chrn[2] < fdc.eot) {
		fdc.chrn[2]++;
		return TRUE;
	}

	// EOT�BR=1
	fdc.err |= FDD_EOT;
	fdc.chrn[2] = 0x01;

	// MT�ɂ���ĕ�����
	if (fdc.mt && (!(fdc.chrn[1] & 0x01))) {
		// �T�C�h1��
		fdc.chrn[1] |= 0x01;
		fdc.hd |= 0x04;
		return TRUE;
	}

	// C+1, R=1�ŏI��
	fdc.chrn[0]++;
	return FALSE;
}
