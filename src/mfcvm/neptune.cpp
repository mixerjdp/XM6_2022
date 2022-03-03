//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 �o�h�D(ytanaka@ipc-tokai.or.jp)
//	[ Neptune-X ]
//
//---------------------------------------------------------------------------

#include "os.h"
#include "xm6.h"
#include "vm.h"
#include "log.h"
#include "schedule.h"
#include "neptune.h"

//===========================================================================
//
//	Neptune-X
//
//===========================================================================
//#define NEPTUNE_LOG

//---------------------------------------------------------------------------
//
//	�R���X�g���N�^
//
//---------------------------------------------------------------------------
Neptune::Neptune(VM *p) : MemDevice(p)
{
	// �f�o�C�XID��������
	dev.id = MAKEID('N', 'E', 'P', 'X');
	dev.desc = "Neptune-X (DP8390)";

	// �J�n�A�h���X�A�I���A�h���X
	memdev.first = 0xece000;
	memdev.last = 0xecffff;
}

//---------------------------------------------------------------------------
//
//	������
//
//---------------------------------------------------------------------------
BOOL FASTCALL Neptune::Init()
{
	ASSERT(this);

	// ��{�N���X
	if (!MemDevice::Init()) {
		return FALSE;
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	�N���[���A�b�v
//
//---------------------------------------------------------------------------
void FASTCALL Neptune::Cleanup()
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
void FASTCALL Neptune::Reset()
{
	ASSERT(this);
	ASSERT_DIAG();

	LOG0(Log::Normal, "���Z�b�g");
}

//---------------------------------------------------------------------------
//
//	�Z�[�u
//
//---------------------------------------------------------------------------
BOOL FASTCALL Neptune::Save(Fileio* /*fio*/, int /*ver*/)
{
	ASSERT(this);
	ASSERT_DIAG();

	LOG0(Log::Normal, "�Z�[�u");

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	���[�h
//
//---------------------------------------------------------------------------
BOOL FASTCALL Neptune::Load(Fileio* /*fio*/, int /*ver*/)
{
	ASSERT(this);
	ASSERT_DIAG();

	LOG0(Log::Normal, "���[�h");

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	�ݒ�K�p
//
//---------------------------------------------------------------------------
void FASTCALL Neptune::ApplyCfg(const Config* /*config*/)
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
void FASTCALL Neptune::AssertDiag() const
{
	// ��{�N���X
	MemDevice::AssertDiag();

	ASSERT(this);
	ASSERT(GetID() == MAKEID('N', 'E', 'P', 'X'));
	ASSERT(memdev.first == 0xece000);
	ASSERT(memdev.last == 0xecffff);
}
#endif	// NDEBUG

//---------------------------------------------------------------------------
//
//	�o�C�g�ǂݍ���
//
//---------------------------------------------------------------------------
DWORD FASTCALL Neptune::ReadByte(DWORD addr)
{
	ASSERT(this);
	ASSERT((addr >= memdev.first) && (addr <= memdev.last));
	ASSERT_DIAG();

	// �o�X�G���[
	cpu->BusErr(addr, TRUE);

	return 0xff;
}

//---------------------------------------------------------------------------
//
//	���[�h�ǂݍ���
//
//---------------------------------------------------------------------------
DWORD FASTCALL Neptune::ReadWord(DWORD addr)
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
void FASTCALL Neptune::WriteByte(DWORD addr, DWORD data)
{
	ASSERT(this);
	ASSERT((addr >= memdev.first) && (addr <= memdev.last));
	ASSERT(data < 0x100);
	ASSERT_DIAG();

	// �o�X�G���[
	cpu->BusErr(addr, FALSE);
}

//---------------------------------------------------------------------------
//
//	���[�h��������
//
//---------------------------------------------------------------------------
void FASTCALL Neptune::WriteWord(DWORD addr, DWORD data)
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
DWORD FASTCALL Neptune::ReadOnly(DWORD addr) const
{
	ASSERT(this);
	ASSERT((addr >= memdev.first) && (addr <= memdev.last));
	ASSERT_DIAG();

	return 0xff;
}
