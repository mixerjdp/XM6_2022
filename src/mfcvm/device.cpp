//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2005 �o�h�D(ytanaka@ipc-tokai.or.jp)
//	[ �f�o�C�X ]
//
//---------------------------------------------------------------------------

#include "os.h"
#include "xm6.h"
#include "vm.h"
#include "log.h"
#include "device.h"

//===========================================================================
//
//	�f�o�C�X
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	�R���X�g���N�^
//
//---------------------------------------------------------------------------
Device::Device(VM *p)
{
	// ���[�N������
	dev.next = NULL;
	dev.id = 0;
	dev.desc = NULL;

	// ���[�N�L���A�f�o�C�X�ǉ�
	vm = p;
	vm->AddDevice(this);
	log = &(vm->log);
}

//---------------------------------------------------------------------------
//
//	�f�X�g���N�^
//
//---------------------------------------------------------------------------
Device::~Device()
{
}

//---------------------------------------------------------------------------
//
//	������
//
//---------------------------------------------------------------------------
BOOL FASTCALL Device::Init()
{
	ASSERT(this);

	// �����ŁA�������m�ہE�t�@�C�����[�h�����s��
	return TRUE;
}

//---------------------------------------------------------------------------
//
//	�N���[���A�b�v
//
//---------------------------------------------------------------------------
void FASTCALL Device::Cleanup()
{
	ASSERT(this);

	// VM�ɑ΂��A�f�o�C�X�폜���˗�
	ASSERT(vm);
	vm->DelDevice(this);

	// �������g���폜
	delete this;
}

//---------------------------------------------------------------------------
//
//	���Z�b�g
//
//---------------------------------------------------------------------------
void FASTCALL Device::Reset()
{
	ASSERT(this);
	ASSERT_DIAG();
}

//---------------------------------------------------------------------------
//
//	�Z�[�u
//
//---------------------------------------------------------------------------
BOOL FASTCALL Device::Save(Fileio* /*fio*/, int /*ver*/)
{
	ASSERT(this);
	ASSERT_DIAG();

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	���[�h
//
//---------------------------------------------------------------------------
BOOL FASTCALL Device::Load(Fileio* /*fio*/, int /*ver*/)
{
	ASSERT(this);
	ASSERT_DIAG();

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	�ݒ�K�p
//
//---------------------------------------------------------------------------
void FASTCALL Device::ApplyCfg(const Config* /*config*/)
{
	ASSERT(this);
	ASSERT_DIAG();
}

#if !defined(NDEBUG)
//---------------------------------------------------------------------------
//
//	�f�f
//
//---------------------------------------------------------------------------
void FASTCALL Device::AssertDiag() const
{
	ASSERT(this);
	ASSERT(dev.id != 0);
	ASSERT(dev.desc);
	ASSERT(vm);
	ASSERT(log);
	ASSERT(log == &(vm->log));
}
#endif	// NDEBUG

//---------------------------------------------------------------------------
//
//	�C�x���g�R�[���o�b�N
//
//---------------------------------------------------------------------------
BOOL FASTCALL Device::Callback(Event* /*ev*/)
{
	ASSERT(this);
	ASSERT_DIAG();

	return TRUE;
}

//===========================================================================
//
//	�������}�b�v�h�f�o�C�X
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	�R���X�g���N�^
//
//---------------------------------------------------------------------------
MemDevice::MemDevice(VM *p) : Device(p)
{
	// �������A�h���X���蓖�ĂȂ�
	memdev.first = 0;
	memdev.last = 0;

	// �|�C���^������
	cpu = NULL;
	scheduler = NULL;
}

//---------------------------------------------------------------------------
//
//	������
//
//---------------------------------------------------------------------------
BOOL FASTCALL MemDevice::Init()
{
	ASSERT(this);
	ASSERT((memdev.first != 0) || (memdev.last != 0));
	ASSERT(memdev.first <= 0xffffff);
	ASSERT(memdev.last <= 0xffffff);
	ASSERT(memdev.first <= memdev.last);

	// �f�o�C�X�̏��������ɍs��
	if (!Device::Init()) {
		return FALSE;
	}

	// CPU�A�X�P�W���[���擾
	cpu = (CPU*)vm->SearchDevice(MAKEID('C', 'P', 'U', ' '));
	ASSERT(cpu);
	scheduler = (Scheduler*)vm->SearchDevice(MAKEID('S', 'C', 'H', 'E'));
	ASSERT(scheduler);

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	�o�C�g�ǂݍ���
//
//---------------------------------------------------------------------------
DWORD FASTCALL MemDevice::ReadByte(DWORD /*addr*/)
{
	ASSERT(this);
	ASSERT_DIAG();

	return 0xff;
}

//---------------------------------------------------------------------------
//
//	���[�h�ǂݍ���
//
//---------------------------------------------------------------------------
DWORD FASTCALL MemDevice::ReadWord(DWORD addr)
{
	DWORD data;

	ASSERT(this);
	ASSERT((addr >= memdev.first) && (addr <= memdev.last));
	ASSERT((addr & 1) == 0);
	ASSERT_DIAG();

	// ��ʁ����ʂ̏���Read
	data = ReadByte(addr);
	data <<= 8;
	data |= ReadByte(addr + 1);

	return data;
}

//---------------------------------------------------------------------------
//
//	�o�C�g��������
//
//---------------------------------------------------------------------------
void FASTCALL MemDevice::WriteByte(DWORD /*addr*/, DWORD /*data*/)
{
	ASSERT(this);
	ASSERT_DIAG();
}

//---------------------------------------------------------------------------
//
//	���[�h��������
//
//---------------------------------------------------------------------------
void FASTCALL MemDevice::WriteWord(DWORD addr, DWORD data)
{
	ASSERT(this);
	ASSERT((addr >= memdev.first) && (addr <= memdev.last));
	ASSERT((addr & 1) == 0);
	ASSERT_DIAG();

	// ��ʁ����ʂ̏���Write
	WriteByte(addr, (BYTE)(data >> 8));
	WriteByte(addr + 1, (BYTE)data);
}

//---------------------------------------------------------------------------
//
//	�ǂݍ��݂̂�
//
//---------------------------------------------------------------------------
DWORD FASTCALL MemDevice::ReadOnly(DWORD /*addr*/) const
{
	ASSERT(this);
	ASSERT_DIAG();

	return 0xff;
}

#if !defined(NDEBUG)
//---------------------------------------------------------------------------
//
//	�f�f
//
//---------------------------------------------------------------------------
void FASTCALL MemDevice::AssertDiag() const
{
	// ��{�N���X
	Device::AssertDiag();

	ASSERT(this);
	ASSERT((memdev.first != 0) || (memdev.last != 0));
	ASSERT(memdev.first <= 0xffffff);
	ASSERT(memdev.last <= 0xffffff);
	ASSERT(memdev.first <= memdev.last);
	ASSERT(cpu);
	ASSERT(cpu->GetID() == MAKEID('C', 'P', 'U', ' '));
	ASSERT(scheduler);
	ASSERT(scheduler->GetID() == MAKEID('S', 'C', 'H', 'E'));
}
#endif	// NDEBUG
