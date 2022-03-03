//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 �o�h�D(ytanaka@ipc-tokai.or.jp)
//	[ �V�X�e���|�[�g ]
//
//---------------------------------------------------------------------------

#include "os.h"
#include "xm6.h"
#include "vm.h"
#include "log.h"
#include "sram.h"
#include "keyboard.h"
#include "cpu.h"
#include "crtc.h"
#include "render.h"
#include "memory.h"
#include "schedule.h"
#include "fileio.h"
#include "sysport.h"

//===========================================================================
//
//	�V�X�e���|�[�g
//
//===========================================================================
//#define SYSPORT_LOG

//---------------------------------------------------------------------------
//
//	�R���X�g���N�^
//
//---------------------------------------------------------------------------
SysPort::SysPort(VM *p) : MemDevice(p)
{
	// �f�o�C�XID��������
	dev.id = MAKEID('S', 'Y', 'S', 'P');
	dev.desc = "System (MESSIAH)";

	// �J�n�A�h���X�A�I���A�h���X
	memdev.first = 0xe8e000;
	memdev.last = 0xe8ffff;

	// ���ׂăN���A
	memset(&sysport, 0, sizeof(sysport));

	// �I�u�W�F�N�g
	memory = NULL;
	sram = NULL;
	keyboard = NULL;
	crtc = NULL;
	render = NULL;
}

//---------------------------------------------------------------------------
//
//	������
//
//---------------------------------------------------------------------------
BOOL FASTCALL SysPort::Init()
{
	ASSERT(this);

	// ��{�N���X
	if (!MemDevice::Init()) {
		return FALSE;
	}

	// �������擾
	ASSERT(!memory);
	memory = (Memory*)vm->SearchDevice(MAKEID('M', 'E', 'M', ' '));
	ASSERT(memory);

	// SRAM�擾
	ASSERT(!sram);
	sram = (SRAM*)vm->SearchDevice(MAKEID('S', 'R', 'A', 'M'));
	ASSERT(sram);

	// �L�[�{�[�h�擾
	ASSERT(!keyboard);
	keyboard = (Keyboard*)vm->SearchDevice(MAKEID('K', 'E', 'Y', 'B'));
	ASSERT(keyboard);

	// CRTC�擾
	ASSERT(!crtc);
	crtc = (CRTC*)vm->SearchDevice(MAKEID('C', 'R', 'T', 'C'));
	ASSERT(crtc);

	// �����_���擾
	ASSERT(!render);
	render = (Render*)vm->SearchDevice(MAKEID('R', 'E', 'N', 'D'));
	ASSERT(render);

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	�N���[���A�b�v
//
//---------------------------------------------------------------------------
void FASTCALL SysPort::Cleanup()
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
void FASTCALL SysPort::Reset()
{
	ASSERT(this);
	ASSERT_DIAG();

	LOG0(Log::Normal, "���Z�b�g");

	// �ݒ�l�����Z�b�g
	sysport.contrast = 0;
	sysport.scope_3d = 0;
	sysport.image_unit = 0;
	sysport.power_count = 0;
	sysport.ver_count = 0;
}

//---------------------------------------------------------------------------
//
//	�Z�[�u
//
//---------------------------------------------------------------------------
BOOL FASTCALL SysPort::Save(Fileio *fio, int /*ver*/)
{
	size_t sz;

	ASSERT(this);
	ASSERT(fio);
	ASSERT_DIAG();

	LOG0(Log::Normal, "�Z�[�u");

	// �T�C�Y���Z�[�u
	sz = sizeof(sysport_t);
	if (!fio->Write(&sz, sizeof(sz))) {
		return FALSE;
	}

	// ���̂��Z�[�u
	if (!fio->Write(&sysport, (int)sz)) {
		return FALSE;
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	���[�h
//
//---------------------------------------------------------------------------
BOOL FASTCALL SysPort::Load(Fileio *fio, int /*ver*/)
{
	size_t sz;

	ASSERT(this);
	ASSERT(fio);
	ASSERT_DIAG();

	LOG0(Log::Normal, "���[�h");

	// �T�C�Y�����[�h�A�ƍ�
	if (!fio->Read(&sz, sizeof(sz))) {
		return FALSE;
	}
	if (sz != sizeof(sysport_t)) {
		return FALSE;
	}

	// ���̂����[�h
	if (!fio->Read(&sysport, (int)sz)) {
		return FALSE;
	}

	// �����_���֒ʒm
	render->SetContrast(sysport.contrast);

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	�ݒ�K�p
//
//---------------------------------------------------------------------------
void FASTCALL SysPort::ApplyCfg(const Config* /*config*/)
{
	ASSERT(this);
	ASSERT_DIAG();

	LOG0(Log::Normal, "�ݒ�K�p");
}

#if !defined(NDEBUG)
//---------------------------------------------------------------------------
//
//	�f�f
//
//---------------------------------------------------------------------------
void FASTCALL SysPort::AssertDiag() const
{
	// ��{�N���X
	MemDevice::AssertDiag();

	ASSERT(this);
	ASSERT(GetID() == MAKEID('S', 'Y', 'S', 'P'));
	ASSERT(memdev.first == 0xe8e000);
	ASSERT(memdev.last == 0xe8ffff);
	ASSERT(memory);
	ASSERT(memory->GetID() == MAKEID('M', 'E', 'M', ' '));
	ASSERT(sram);
	ASSERT(sram->GetID() == MAKEID('S', 'R', 'A', 'M'));
	ASSERT(keyboard);
	ASSERT(keyboard->GetID() == MAKEID('K', 'E', 'Y', 'B'));
	ASSERT(crtc);
	ASSERT(crtc->GetID() == MAKEID('C', 'R', 'T', 'C'));
	ASSERT(sysport.contrast <= 0x0f);
	ASSERT(sysport.scope_3d <= 0x03);
	ASSERT(sysport.image_unit <= 0x1f);
	ASSERT(sysport.power_count <= 0x03);
	ASSERT(sysport.ver_count <= 0x03);
}
#endif	// NDEBUG

//---------------------------------------------------------------------------
//
//	�o�C�g�ǂݍ���
//
//---------------------------------------------------------------------------
DWORD FASTCALL SysPort::ReadByte(DWORD addr)
{
	DWORD data;

	ASSERT(this);
	ASSERT((addr >= memdev.first) && (addr <= memdev.last));
	ASSERT_DIAG();

	// 4bit�̂݃f�R�[�h
	addr &= 0x0f;

	// ��o�C�g�̂݃f�R�[�h
	if ((addr & 1) == 0) {
		return 0xff;
	}
	addr >>= 1;

	// �E�F�C�g
	scheduler->Wait(1);

	switch (addr) {
		// �R���g���X�g
		case 0:
			data = 0xf0;
			data |= sysport.contrast;
			return data;

		// �f�B�X�v���C����M���E3D�X�R�[�v
		case 1:
			data = 0xf8;
			data |= sysport.scope_3d;
			return data;

		// �C���[�W���j�b�g����
		case 2:
			return 0xff;

		// �L�[����ENMI���Z�b�g�EHRL
		case 3:
			data = 0xf0;
			if (keyboard->IsConnect()) {
				data |= 0x08;
			}
			if (crtc->GetHRL()) {
				data |= 0x02;
			}
			return data;

		// ROM/DRAM�E�F�C�g����(X68030)
		case 4:
			return 0xff;

		// MPU��ʁE�N���b�N
		case 5:
			switch (memory->GetMemType()) {
				// ����/ACE/EXPERT/PRO/SUPER
				case Memory::SASI:
				case Memory::SCSIInt:
				case Memory::SCSIExt:
					// ����`�|�[�g�ł������̂ŁA0xff���ǂݏo����
					return 0xff;

				// XVI/Compact
				case Memory::XVI:
				case Memory::Compact:
					// �t�]�̔��z�ŁA����4bit�𓮍�N���b�N�Ɋ��蓖�Ă�
					// 1111:10MHz
					// 1110:16MHz
					// 1101:20MHz
					// 1100:25MHz
					// 1011:33MHz
					// 1010:40MHz
					// 1001:50MHz
					return 0xfe;

				// X68030
				case Memory::X68030:
					// ����ɁA���4bit��MPU��ʂɊ��蓖�Ă�
					// 1111:68000
					// 1110:68020
					// 1101:68030
					// 1100:68040
					return 0xdc;

				// ���̑�(���肦�Ȃ�)
				default:
					ASSERT(FALSE);
					break;
			}
			return 0xff;

		// SRAM�������݋���
		case 6:
			// �o�[�W�������(XM6�g��)
			if (sysport.ver_count != 0) {
				return GetVR();
			}
			return 0xff;

		// POWER����
		case 7:
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
DWORD FASTCALL SysPort::ReadWord(DWORD addr)
{
	ASSERT(this);
	ASSERT((addr >= memdev.first) && (addr <= memdev.last));
	ASSERT((addr & 1) == 0);
	ASSERT_DIAG();

	return (0xff00 | ReadByte(addr + 1));
}

//---------------------------------------------------------------------------
//
//	�o�C�g��������
//
//---------------------------------------------------------------------------
void FASTCALL SysPort::WriteByte(DWORD addr, DWORD data)
{
	ASSERT(this);
	ASSERT((addr >= memdev.first) && (addr <= memdev.last));
	ASSERT(data < 0x100);
	ASSERT_DIAG();

	// 4bit�̂݃f�R�[�h
	addr &= 0x0f;

	// ��o�C�g�̂݃f�R�[�h
	if ((addr & 1) == 0) {
		return;
	}
	addr >>= 1;

	// �E�F�C�g
	scheduler->Wait(1);

	switch (addr) {
		// �R���g���X�g
		case 0:
			data &= 0x0f;
			if (sysport.contrast != data) {
#if defined(SYSPORT_LOG)
				LOG1(Log::Normal, "�R���g���X�g�ݒ� %d", data);
#endif	// SYSPORT_LOG

				// ����Ă�����ݒ�
				sysport.contrast = data;
				render->SetContrast(data);
			}
			return;

		// �f�B�X�v���C����M���E3D�X�R�[�v
		case 1:
			data &= 3;
			if (sysport.scope_3d != data) {
#if defined(SYSPORT_LOG)
				LOG1(Log::Normal, "3D�X�R�[�v�ݒ� %d", data);
#endif	// SYSPORT_LOG

				// ����Ă�����ݒ�
				sysport.scope_3d = data;
			}
			return;

		// �L�[����ENMI���Z�b�g�EHRL
		case 3:
			if (data & 0x08) {
#if defined(SYSPORT_LOG)
				LOG0(Log::Normal, "�L�[�{�[�h����");
#endif	// SYSPORT_LOG
				keyboard->SendWait(FALSE);
			}
			else {
#if defined(SYSPORT_LOG)
				LOG0(Log::Normal, "�L�[�{�[�h�֎~");
#endif	// SYSPORT_LOG
				keyboard->SendWait(TRUE);
			}
			if (data & 0x02) {
#if defined(SYSPORT_LOG)
				LOG0(Log::Normal, "HRL=1");
#endif	// SYSPORT_LOG
				crtc->SetHRL(TRUE);
			}
			else {
#if defined(SYSPORT_LOG)
				LOG0(Log::Normal, "HRL=0");
#endif	// SYSPORT_LOG
				crtc->SetHRL(FALSE);
			}
			return;

		// �C���[�W���j�b�g����
		case 4:
#if defined(SYSPORT_LOG)
			LOG1(Log::Normal, "�C���[�W���j�b�g���� $%02X", data & 0x1f);
#endif	// SYSPORT_LOG
			sysport.image_unit = data & 0x1f;
			return;

		// ROM/DRAM�E�F�C�g����
		case 5:
			return;

		// SRAM�������݋���
		case 6:
			// SRAM��������
			if (data == 0x31) {
				sram->WriteEnable(TRUE);
			}
			else {
				sram->WriteEnable(FALSE);
			}
			// �o�[�W��������(XM6�g��)
			if (data == 'X') {
#if defined(SYSPORT_LOG)
				LOG0(Log::Normal, "XM6�o�[�W�����擾�J�n");
#endif	// SYSPORT_LOG
				sysport.ver_count = 1;
			}
			else {
				sysport.ver_count = 0;
			}
			return;

		// �d������(00, 0F, 0F�̏��ŗ�����)
		case 7:
			data &= 0x0f;
			switch (sysport.power_count) {
				// �A�N�Z�X�Ȃ�
				case 0:
					if (data == 0x00) {
						sysport.power_count++;
					}
					else {
						sysport.power_count = 0;
					}
					break;

				// 00����
				case 1:
					if (data == 0x0f) {
						sysport.power_count++;
					}
					else {
						sysport.power_count = 0;
					}
					break;

				// 00,0F����
				case 2:
					if (data == 0x0f) {
						sysport.power_count++;
						LOG0(Log::Normal, "�d��OFF");
						vm->SetPower(FALSE);
					}
					else {
						sysport.power_count = 0;
					}
					break;

				// 00,0F,0F
				default:
					break;
			}
			return;

		default:
			break;
	}

	// �ʏ�A�����ɂ͂��Ȃ�
	ASSERT(FALSE);
	return;
}

//---------------------------------------------------------------------------
//
//	���[�h��������
//
//---------------------------------------------------------------------------
void FASTCALL SysPort::WriteWord(DWORD addr, DWORD data)
{
	ASSERT(this);
	ASSERT((addr >= memdev.first) && (addr <= memdev.last));
	ASSERT((addr & 1) == 0);
	ASSERT(data < 0x10000);
	ASSERT_DIAG();

	WriteByte(addr + 1, (BYTE)data);
}

//---------------------------------------------------------------------------
//
//	�ǂݍ��݂̂�
//
//---------------------------------------------------------------------------
DWORD FASTCALL SysPort::ReadOnly(DWORD addr) const
{
	DWORD data;

	ASSERT(this);
	ASSERT((addr >= memdev.first) && (addr <= memdev.last));
	ASSERT_DIAG();

	// 4bit�̂݃f�R�[�h
	addr &= 0x0f;

	// ��o�C�g�̂݃f�R�[�h
	if ((addr & 1) == 0) {
		return 0xff;
	}

	switch (addr) {
		// �R���g���X�g
		case 0:
			data = 0xf0;
			data |= sysport.contrast;
			return data;

		// �f�B�X�v���C����M���E3D�X�R�[�v
		case 1:
			data = 0xf8;
			data |= sysport.scope_3d;
			return data;

		// �C���[�W���j�b�g����
		case 2:
			return 0xff;

		// �L�[����ENMI���Z�b�g�EHRL
		case 3:
			data = 0xf0;
			if (keyboard->IsConnect()) {
				data |= 0x08;
			}
			if (crtc->GetHRL()) {
				data |= 0x02;
			}
			return data;

		// ROM/DRAM�E�F�C�g����(X68030)
		case 4:
			return 0xff;

		// MPU��ʁE�N���b�N
		case 5:
			switch (memory->GetMemType()) {
				// ����/ACE/EXPERT/PRO/SUPER
				case Memory::SASI:
				case Memory::SCSIInt:
				case Memory::SCSIExt:
					// ����`�|�[�g�ł������̂ŁA0xff���ǂݏo����
					return 0xff;

				// XVI/Compact
				case Memory::XVI:
				case Memory::Compact:
					// �t�]�̔��z�ŁA����4bit�𓮍�N���b�N�Ɋ��蓖�Ă�
					// 1111:10MHz
					// 1110:16MHz
					// 1101:20MHz
					// 1100:25MHz
					// 1011:33MHz
					// 1010:40MHz
					// 1001:50MHz
					return 0xfe;

				// X68030
				case Memory::X68030:
					// ����ɁA���4bit��MPU��ʂɊ��蓖�Ă�
					// 1111:68000
					// 1110:68020
					// 1101:68030
					// 1100:68040
					return 0xdc;

				// ���̑�(���肦�Ȃ�)
				default:
					ASSERT(FALSE);
					break;
			}
			return 0xff;

		// SRAM�������݋���
		case 6:
			return 0xff;

		// POWER����
		case 7:
			return 0xff;
	}

	return 0xff;
}

//---------------------------------------------------------------------------
//
//	�o�[�W�������ǂݏo��
//
//---------------------------------------------------------------------------
DWORD FASTCALL SysPort::GetVR()
{
	DWORD major;
	DWORD minor;

	ASSERT(this);
	ASSERT_DIAG();

	switch (sysport.ver_count) {
		// 'X'�������ݒ���
		case 1:
			sysport.ver_count++;
			return '6';

		// ���W���[�o�[�W����
		case 2:
			vm->GetVersion(major, minor);
			sysport.ver_count++;
			return major;

		// �}�C�i�[�o�[�W����
		case 3:
			vm->GetVersion(major, minor);
			sysport.ver_count = 0;
			return minor;

		// ���̑�(���肦�Ȃ�)
		default:
			ASSERT(FALSE);
			break;
	}

	return 0xff;
}
