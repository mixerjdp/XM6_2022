//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 �o�h�D(ytanaka@ipc-tokai.or.jp)
//	[ I/O�R���g���[��(IOSC-2) ]
//
//---------------------------------------------------------------------------

#include "os.h"
#include "xm6.h"
#include "cpu.h"
#include "vm.h"
#include "log.h"
#include "fileio.h"
#include "schedule.h"
#include "printer.h"
#include "iosc.h"

//===========================================================================
//
//	IOSC
//
//===========================================================================
//#define IOSC_LOG

//---------------------------------------------------------------------------
//
//	�R���X�g���N�^
//
//---------------------------------------------------------------------------
IOSC::IOSC(VM *p) : MemDevice(p)
{
	// �f�o�C�XID��������
	dev.id = MAKEID('I', 'O', 'S', 'C');
	dev.desc = "I/O Ctrl (IOSC-2)";

	// �J�n�A�h���X�A�I���A�h���X
	memdev.first = 0xe9c000;
	memdev.last = 0xe9dfff;

	// ���̑�
	printer = NULL;
}

//---------------------------------------------------------------------------
//
//	������
//
//---------------------------------------------------------------------------
BOOL FASTCALL IOSC::Init()
{
	ASSERT(this);

	// ��{�N���X
	if (!MemDevice::Init()) {
		return FALSE;
	}

	// �v�����^���擾
	printer = (Printer*)vm->SearchDevice(MAKEID('P', 'R', 'N', ' '));
	ASSERT(printer);

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	�N���[���A�b�v
//
//---------------------------------------------------------------------------
void FASTCALL IOSC::Cleanup()
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
void FASTCALL IOSC::Reset()
{
	LOG0(Log::Normal, "���Z�b�g");

	// ���[�N������
	iosc.prt_int = FALSE;
	iosc.prt_en = FALSE;
	iosc.fdd_int = FALSE;
	iosc.fdd_en = FALSE;
	iosc.fdc_int = FALSE;
	iosc.fdc_en = FALSE;
	iosc.hdc_int = FALSE;
	iosc.hdc_en = FALSE;
	iosc.vbase = 0;

	// �v�����̃x�N�^�Ȃ�
	iosc.vector = -1;
}

//---------------------------------------------------------------------------
//
//	�Z�[�u
//
//---------------------------------------------------------------------------
BOOL FASTCALL IOSC::Save(Fileio *fio, int /*ver*/)
{
	size_t sz;

	ASSERT(this);
	ASSERT(fio);
	LOG0(Log::Normal, "�Z�[�u");

	// �T�C�Y���Z�[�u
	sz = sizeof(iosc_t);
	if (!fio->Write(&sz, sizeof(sz))) {
		return FALSE;
	}

	// ���̂��Z�[�u
	if (!fio->Write(&iosc, sizeof(iosc))) {
		return FALSE;
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	���[�h
//
//---------------------------------------------------------------------------
BOOL FASTCALL IOSC::Load(Fileio *fio, int /*ver*/)
{
	size_t sz;

	ASSERT(this);
	ASSERT(fio);
	LOG0(Log::Normal, "���[�h");

	// �T�C�Y�����[�h�A�ƍ�
	if (!fio->Read(&sz, sizeof(sz))) {
		return FALSE;
	}
	if (sz != sizeof(iosc_t)) {
		return FALSE;
	}

	// ���̂����[�h
	if (!fio->Read(&iosc, sizeof(iosc))) {
		return FALSE;
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	�ݒ�K�p
//
//---------------------------------------------------------------------------
void FASTCALL IOSC::ApplyCfg(const Config* /*config*/)
{
	ASSERT(this);
	LOG0(Log::Normal, "�ݒ�K�p");
}

//---------------------------------------------------------------------------
//
//	�o�C�g�ǂݍ���
//
//---------------------------------------------------------------------------
DWORD FASTCALL IOSC::ReadByte(DWORD addr)
{
	DWORD data;

	ASSERT(this);
	ASSERT((addr >= memdev.first) && (addr <= memdev.last));

	// 16�o�C�g�P�ʂŃ��[�v
	addr &= 0x0f;

	// ��A�h���X�̂݃f�R�[�h����Ă���
	if ((addr & 1) != 0) {

		// �E�F�C�g
		scheduler->Wait(2);

		// $E9C001 ���荞�݃X�e�[�^�X
		if (addr == 1) {
			data = 0;
			if (iosc.fdc_int) {
				data |= 0x80;
			}
			if (iosc.fdd_int) {
				data |= 0x40;
			}
			if (iosc.hdc_int) {
				data |= 0x10;
			}
			if (iosc.hdc_en) {
				data |= 0x08;
			}
			if (iosc.fdc_en) {
				data |= 0x04;
			}
			if (iosc.fdd_en) {
				data |= 0x02;
			}
			if (iosc.prt_en) {
				data |= 0x01;
			}

			// �v�����^��READY�̕\��
			if (printer->IsReady()) {
				data |= 0x20;
			}

			// �v�����^���荞�݂͂��̎��_�ō~�낷
			if (iosc.prt_int) {
				iosc.prt_int = FALSE;
				IntChk();
			}
			return data;
		}

		// $E9C003 ���荞�݃x�N�^
		if (addr == 3) {
			return 0xff;
		}

		LOG1(Log::Warning, "���������W�X�^�ǂݍ��� $%06X", memdev.first + addr);
		return 0xff;
	}

	// �o�X�G���[�͔������Ȃ�
	return 0xff;
}

//---------------------------------------------------------------------------
//
//	���[�h�ǂݍ���
//
//---------------------------------------------------------------------------
DWORD FASTCALL IOSC::ReadWord(DWORD addr)
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
void FASTCALL IOSC::WriteByte(DWORD addr, DWORD data)
{
	ASSERT(this);
	ASSERT((addr >= memdev.first) && (addr <= memdev.last));
	ASSERT(data < 0x100);

	// 16�o�C�g�P�ʂŃ��[�v
	addr &= 0x0f;

	// ��A�h���X�̂݃f�R�[�h����Ă���
	if ((addr & 1) != 0) {

		// �E�F�C�g
		scheduler->Wait(2);

		// $E9C001 ���荞�݃}�X�N
		if (addr == 1) {
			if (data & 0x01) {
				iosc.prt_en = TRUE;
			}
			else {
				iosc.prt_en = FALSE;
			}
			if (data & 0x02) {
				iosc.fdd_en = TRUE;
			}
			else {
				iosc.fdd_en = FALSE;
			}
			if (data & 0x04) {
				iosc.fdc_en = TRUE;
			}
			else {
				iosc.fdc_en = FALSE;
			}
			if (data & 0x08) {
				iosc.hdc_en = TRUE;
			}
			else {
				iosc.hdc_en = FALSE;
			}

			// ���荞�݃`�F�b�N(FORMULA 68K)
			IntChk();
			return;
		}

		// $E9C003 ���荞�݃x�N�^
		if (addr == 3) {
			data &= 0xfc;
			iosc.vbase &= 0x03;
			iosc.vbase |= data;

			LOG1(Log::Detail, "���荞�݃x�N�^�x�[�X $%02X", iosc.vbase);
			return;
		}

		// ����`
		LOG2(Log::Warning, "���������W�X�^�������� $%06X <- $%02X",
										memdev.first + addr, data);
		return;
	}
}

//---------------------------------------------------------------------------
//
//	���[�h��������
//
//---------------------------------------------------------------------------
void FASTCALL IOSC::WriteWord(DWORD addr, DWORD data)
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
DWORD FASTCALL IOSC::ReadOnly(DWORD addr) const
{
	DWORD data;

	ASSERT(this);
	ASSERT((addr >= memdev.first) && (addr <= memdev.last));

	// 16�o�C�g�P�ʂŃ��[�v
	addr &= 0x0f;

	// ��A�h���X�̂݃f�R�[�h����Ă���
	if ((addr & 1) != 0) {

		// $E9C001 ���荞�݃X�e�[�^�X
		if (addr == 1) {
			data = 0;
			if (iosc.fdc_int) {
				data |= 0x80;
			}
			if (iosc.fdd_int) {
				data |= 0x40;
			}
			if (iosc.prt_int) {
				data |= 0x20;
			}
			if (iosc.hdc_int) {
				data |= 0x10;
			}
			if (iosc.hdc_en) {
				data |= 0x08;
			}
			if (iosc.fdc_en) {
				data |= 0x04;
			}
			if (iosc.fdd_en) {
				data |= 0x02;
			}
			if (iosc.prt_en) {
				data |= 0x01;
			}
			return data;
		}

		// $E9C003 ���荞�݃x�N�^
		if (addr == 3) {
			return 0xff;
		}

		// ����`�A�h���X
		return 0xff;
	}

	// �����A�h���X
	return 0xff;
}

//---------------------------------------------------------------------------
//
//	�����f�[�^�擾
//
//---------------------------------------------------------------------------
void FASTCALL IOSC::GetIOSC(iosc_t *buffer) const
{
	ASSERT(this);
	ASSERT(buffer);

	// �������[�N���R�s�[
	*buffer = iosc;
}

//---------------------------------------------------------------------------
//
//	���荞�݃`�F�b�N
//
//---------------------------------------------------------------------------
void FASTCALL IOSC::IntChk()
{
	int v;

	ASSERT(this);

	// ���荞�ݖ���
	v = -1;

	// �v�����^���荞��
	if (iosc.prt_int && iosc.prt_en) {
		v = iosc.vbase + 3;
	}

	// HDC���荞��
	if (iosc.hdc_int && iosc.hdc_en) {
		v = iosc.vbase + 2;
	}

	// FDD���荞��
	if (iosc.fdd_int && iosc.fdd_en) {
		v = iosc.vbase + 1;
	}

	// FDC���荞��
	if (iosc.fdc_int && iosc.fdc_en) {
		v = iosc.vbase;
	}

	// �v�����������荞�݂��Ȃ��ꍇ
	if (v < 0) {
		// ���荞�ݗv�����łȂ���΁AOK
		if (iosc.vector < 0) {
			return;
		}

		// ���炩�̊��荞�݂�v�����Ă���̂ŁA���荞�݃L�����Z��
#if defined(IOSC_LOG)
		LOG1(Log::Normal, "���荞�݃L�����Z�� �x�N�^$%02X", iosc.vector);
#endif	// IOSC_LOG
		cpu->IntCancel(1);
		iosc.vector = -1;
		return;
	}

	// ���ɗv�����Ă���x�N�^�Ɠ����ł���΁AOK
	if (iosc.vector == v) {
		return;
	}

	// �Ⴄ�̂ŁA�v�����Ȃ��x�L�����Z��
	if (iosc.vector >= 0) {
#if defined(IOSC_LOG)
		LOG1(Log::Normal, "���荞�݃L�����Z�� �x�N�^$%02X", iosc.vector);
#endif	// IOSC_LOG
		cpu->IntCancel(1);
		iosc.vector = -1;
	}

	// ���荞�ݗv��
#if defined(IOSC_LOG)
	LOG1(Log::Normal, "���荞�ݗv�� �x�N�^$%02X", v);
#endif	// IOSC_LOG
	cpu->Interrupt(1, (BYTE)v);

	// �L��
	iosc.vector = v;
}

//---------------------------------------------------------------------------
//
//	���荞�݉���
//
//---------------------------------------------------------------------------
void FASTCALL IOSC::IntAck()
{
	ASSERT(this);

	// ���Z�b�g����ɁACPU���犄�荞�݂��Ԉ���ē���ꍇ������
	// �܂��͑��̃f�o�C�X
	if (iosc.vector < 0) {
#if defined(IOSC_LOG)
		LOG0(Log::Warning, "�v�����Ă��Ȃ����荞��");
#endif	// IOSC_LOG
		return;
	}

#if defined(IOSC_LOG)
	LOG1(Log::Normal, "���荞��ACK �x�N�^$%02X", iosc.vector);
#endif	// IOSC_LOG

	// ���荞�݂Ȃ�
	iosc.vector = -1;
}

//---------------------------------------------------------------------------
//
//	FDC���荞��
//
//---------------------------------------------------------------------------
void FASTCALL IOSC::IntFDC(BOOL flag)
{
	ASSERT(this);

	iosc.fdc_int = flag;

	// ���荞�݃`�F�b�N
	IntChk();
}

//---------------------------------------------------------------------------
//
//	FDD���荞��
//
//---------------------------------------------------------------------------
void FASTCALL IOSC::IntFDD(BOOL flag)
{
	ASSERT(this);

	iosc.fdd_int = flag;

	// ���荞�݃`�F�b�N
	IntChk();
}

//---------------------------------------------------------------------------
//
//	HDC���荞��
//
//---------------------------------------------------------------------------
void FASTCALL IOSC::IntHDC(BOOL flag)
{
	ASSERT(this);

	iosc.hdc_int = flag;

	// ���荞�݃`�F�b�N
	IntChk();
}

//---------------------------------------------------------------------------
//
//	�v�����^���荞��
//
//---------------------------------------------------------------------------
void FASTCALL IOSC::IntPRT(BOOL flag)
{
	ASSERT(this);

	iosc.prt_int = flag;

	// ���荞�݃`�F�b�N
	IntChk();
}
