//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 �o�h�D(ytanaka@ipc-tokai.or.jp)
//	[ �v�����^ ]
//
//---------------------------------------------------------------------------

#include "os.h"
#include "xm6.h"
#include "vm.h"
#include "cpu.h"
#include "log.h"
#include "iosc.h"
#include "sync.h"
#include "fileio.h"
#include "printer.h"

//===========================================================================
//
//	�v�����^
//
//===========================================================================
//#define PRINTER_LOG

//---------------------------------------------------------------------------
//
//	�R���X�g���N�^
//
//---------------------------------------------------------------------------
Printer::Printer(VM *p) : MemDevice(p)
{
	// �f�o�C�XID��������
	dev.id = MAKEID('P', 'R', 'N', ' ');
	dev.desc = "Printer";

	// �J�n�A�h���X�A�I���A�h���X
	memdev.first = 0xe8c000;
	memdev.last = 0xe8dfff;

	// ���̑�
	iosc = NULL;
	sync = NULL;
}

//---------------------------------------------------------------------------
//
//	������
//
//---------------------------------------------------------------------------
BOOL FASTCALL Printer::Init()
{
	ASSERT(this);

	// ��{�N���X
	if (!MemDevice::Init()) {
		return FALSE;
	}

	// IOSC���擾
	iosc = (IOSC*)vm->SearchDevice(MAKEID('I', 'O', 'S', 'C'));
	ASSERT(iosc);

	// Sync�쐬
	sync = new Sync;

	// �ڑ����Ȃ�
	printer.connect = FALSE;

	// �o�b�t�@���N���A
	sync->Lock();
	printer.read = 0;
	printer.write = 0;
	printer.num = 0;
	sync->Unlock();

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	�N���[���A�b�v
//
//---------------------------------------------------------------------------
void FASTCALL Printer::Cleanup()
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
void FASTCALL Printer::Reset()
{
	ASSERT(this);

	LOG0(Log::Normal, "���Z�b�g");

	// �X�g���[�u(���_��)
	printer.strobe = FALSE;

	// ���f�B(���_��)
	if (printer.connect) {
		// �ڑ�����Ă����READY
		printer.ready = TRUE;
	}
	else {
		// ��ڑ��Ȃ�BUSY
		printer.ready = FALSE;
	}

	// ���荞�ݖ���
	iosc->IntPRT(FALSE);
}

//---------------------------------------------------------------------------
//
//	�Z�[�u
//
//---------------------------------------------------------------------------
BOOL FASTCALL Printer::Save(Fileio *fio, int /* ver */)
{
	size_t sz;

	ASSERT(this);
	ASSERT(fio);
	LOG0(Log::Normal, "�Z�[�u");

	// �T�C�Y���Z�[�u
	sz = sizeof(printer_t);
	if (!fio->Write(&sz, (int)sizeof(sz))) {
		return FALSE;
	}

	// ���̂��Z�[�u
	if (!fio->Write(&printer, (int)sz)) {
		return FALSE;
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	���[�h
//
//---------------------------------------------------------------------------
BOOL FASTCALL Printer::Load(Fileio *fio, int /* ver */)
{
	size_t sz;

	ASSERT(this);
	ASSERT(fio);
	LOG0(Log::Normal, "���[�h");

	// �T�C�Y�����[�h�A�ƍ�
	if (!fio->Read(&sz, sizeof(sz))) {
		return FALSE;
	}
	if (sz != sizeof(printer_t)) {
		return FALSE;
	}

	// ���̂����[�h
	if (!fio->Read(&printer, (int)sz)) {
		return FALSE;
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	�ݒ�K�p
//
//---------------------------------------------------------------------------
void FASTCALL Printer::ApplyCfg(const Config* /*config*/)
{
	ASSERT(this);
	LOG0(Log::Normal, "�ݒ�K�p");

	// �ݒ�ύX���A�R���|�[�l���g����Connect()���Ă΂��̂ŁAREADY���ς��
}

//---------------------------------------------------------------------------
//
//	�o�C�g�ǂݍ���
//
//---------------------------------------------------------------------------
DWORD FASTCALL Printer::ReadByte(DWORD /*addr*/)
{
	ASSERT(this);

	// ���0xff(Write Only�̂���)
	return 0xff;
}

//---------------------------------------------------------------------------
//
//	���[�h�ǂݍ���
//
//---------------------------------------------------------------------------
DWORD FASTCALL Printer::ReadWord(DWORD /*addr*/)
{
	ASSERT(this);

	// ���0xff(Write Only�̂���)
	return 0xff;
}

//---------------------------------------------------------------------------
//
//	�o�C�g��������
//
//---------------------------------------------------------------------------
void FASTCALL Printer::WriteByte(DWORD addr, DWORD data)
{
	ASSERT(this);
	ASSERT((addr >= memdev.first) && (addr <= memdev.last));

	ASSERT(printer.num <= BufMax);
	ASSERT(printer.read < BufMax);
	ASSERT(printer.write < BufMax);

	// 4�o�C�g�P�ʂŃ��[�v(�\�z�BWriteOnly�|�[�g�̂��ߕs��)
	addr &= 0x03;

	// �f�R�[�h
	switch (addr) {
		// $E8C001 �o�̓f�[�^
		case 1:
			printer.data = (BYTE)data;
			break;

		// $E8C003 �X�g���[�u
		case 3:
			// �X�g���[�u��0(TRUE)��1(FALSE)�ւ̕ω��Ō��͂�����
			if ((data & 1) == 0) {
#if defined(PRINTER_LOG)
				LOG0(Log::Normal, "�X�g���[�u 0(TRUE)");
#endif	// PRINTER_LOG
				printer.strobe = TRUE;
				break;
			}

			// �X�g���[�u��TRUE�łȂ���΁A�����܂�
			if (!printer.strobe) {
				break;
			}

			// �X�g���[�u��FALSE��
			printer.strobe = FALSE;
#if defined(PRINTER_LOG)
			LOG0(Log::Normal, "�X�g���[�u 1(FALSE)");
#endif	// PRINTER_LOG

			// �ڑ�����Ă��Ȃ���΁A�����܂�
			if (!printer.connect) {
				break;
			}

			// �����Ńf�[�^�����b�`
#if defined(PRINTER_LOG)
			LOG1(Log::Normal, "�f�[�^�m�� $%02X", printer.data);
#endif	// PRINTER_LOG

			sync->Lock();
			// �f�[�^�}��
			printer.buf[printer.write] = printer.data;
			printer.write = (printer.write + 1) & (BufMax - 1);
			printer.num++;

			// �E�F�C�g���䂪���邽�߁A�o�b�t�@�͈��Ȃ��͂�
			if (printer.num > BufMax) {
				ASSERT(FALSE);
				LOG0(Log::Warning, "�o�̓o�b�t�@�I�[�o�[�t���[");
				printer.num = BufMax;
			}
			sync->Unlock();

			// READY�𗎂Ƃ�
			printer.ready = FALSE;

			// ���荞�ݖ���
			iosc->IntPRT(FALSE);
			break;
	}
}

//---------------------------------------------------------------------------
//
//	���[�h��������
//
//---------------------------------------------------------------------------
void FASTCALL Printer::WriteWord(DWORD addr, DWORD data)
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
DWORD FASTCALL Printer::ReadOnly(DWORD /*addr*/) const
{
	ASSERT(this);

	// ���0xff(Write Only�̂���)
	return 0xff;
}

//---------------------------------------------------------------------------
//
//	H-Sync�ʒm
//
//---------------------------------------------------------------------------
void FASTCALL Printer::HSync()
{
	ASSERT(this);

	// ���f�B��FALSE(�f�[�^�]����)�łȂ���Ί֌W�Ȃ�
	if (printer.ready) {
		return;
	}

	// �ڑ�����Ă��Ȃ����
	if (!printer.connect) {
		// printer.ready��FALSE�̂܂�
		return;
	}

	// �o�b�t�@����t�Ȃ�A���܂ň������΂�
	if (printer.num == BufMax) {
#if defined(PRINTER_LOG)
	LOG0(Log::Normal, "�o�b�t�@�t���̂��߃E�F�C�g");
#endif	// PRINTER_LOG
		return;
	}

#if defined(PRINTER_LOG)
	LOG0(Log::Normal, "�f�[�^���M�����AREADY");
#endif	// PRINTER_LOG

	// ���f�B���
	printer.ready = TRUE;

	// ���荞�ݐݒ�
	iosc->IntPRT(TRUE);
}

//---------------------------------------------------------------------------
//
//	�����f�[�^�擾
//
//---------------------------------------------------------------------------
void FASTCALL Printer::GetPrinter(printer_t *buffer) const
{
	ASSERT(this);
	ASSERT(buffer);

	// �������[�N���R�s�[
	*buffer = printer;
}

//---------------------------------------------------------------------------
//
//	�ڑ�
//
//---------------------------------------------------------------------------
void FASTCALL Printer::Connect(BOOL flag)
{
	ASSERT(this);
	ASSERT(printer.num <= BufMax);
	ASSERT(printer.read < BufMax);
	ASSERT(printer.write < BufMax);

	// ��v���Ă���Ή������Ȃ�
	if (printer.connect == flag) {
		return;
	}

	// �ݒ�
	printer.connect = flag;
#if defined(PRINTER_LOG)
	if (printer.connect) {
		LOG0(Log::Normal, "�v�����^�ڑ�");
	}
	else {
		LOG0(Log::Normal, "�v�����^�ؒf");
	}
#endif	// PRINTER_LOG

	// FALSE�ɂ���Ȃ�A���f�B���낷
	if (!printer.connect) {
		printer.ready = FALSE;
		iosc->IntPRT(FALSE);
		return;
	}

	// TRUE�ɂ���Ȃ�A����HSYNC�Ń��f�B�グ��
	sync->Lock();
	printer.read = 0;
	printer.write = 0;
	printer.num = 0;
	sync->Unlock();
}

//---------------------------------------------------------------------------
//
//	�擪�f�[�^�擾
//
//---------------------------------------------------------------------------
BOOL FASTCALL Printer::GetData(BYTE *ptr)
{
	ASSERT(this);
	ASSERT(ptr);
	ASSERT(printer.num <= BufMax);
	ASSERT(printer.read < BufMax);
	ASSERT(printer.write < BufMax);

	// ���b�N
	sync->Lock();

	// �f�[�^���Ȃ����FALSE
	if (printer.num == 0) {
		sync->Unlock();
		return FALSE;
	}

	// �f�[�^�擾
	*ptr = printer.buf[printer.read];

	// ����
	printer.read = (printer.read + 1) & (BufMax - 1);
	printer.num--;

	// �A�����b�N
	sync->Unlock();
	return TRUE;
}
