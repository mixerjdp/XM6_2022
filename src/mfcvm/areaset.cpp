//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2005 �o�h�D(ytanaka@ipc-tokai.or.jp)
//	[ �G���A�Z�b�g ]
//
//---------------------------------------------------------------------------

#include "os.h"
#include "xm6.h"
#include "vm.h"
#include "cpu.h"
#include "log.h"
#include "fileio.h"
#include "memory.h"
#include "areaset.h"

//===========================================================================
//
//	�G���A�Z�b�g
//
//===========================================================================
//#define AREASET_LOG

//---------------------------------------------------------------------------
//
//	�R���X�g���N�^
//
//---------------------------------------------------------------------------
AreaSet::AreaSet(VM *p) : MemDevice(p)
{
	// �f�o�C�XID��������
	dev.id = MAKEID('A', 'R', 'E', 'A');
	dev.desc = "Area Set";

	// �J�n�A�h���X�A�I���A�h���X
	memdev.first = 0xe86000;
	memdev.last = 0xe87fff;

	// �I�u�W�F�N�g
	memory = NULL;
}

//---------------------------------------------------------------------------
//
//	������
//
//---------------------------------------------------------------------------
BOOL FASTCALL AreaSet::Init()
{
	ASSERT(this);

	// ��{�N���X
	if (!MemDevice::Init()) {
		return FALSE;
	}

	// �������擾
	memory = (Memory*)vm->SearchDevice(MAKEID('M', 'E', 'M', ' '));
	ASSERT(memory);

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	�N���[���A�b�v
//
//---------------------------------------------------------------------------
void FASTCALL AreaSet::Cleanup()
{
	ASSERT(this);

	// ��{�N���X��
	MemDevice::Cleanup();
}

//---------------------------------------------------------------------------
//
//	���Z�b�g
//	�����K�̏����łȂ��AMemory::MakeContext����Ă΂��
//
//---------------------------------------------------------------------------
void FASTCALL AreaSet::Reset()
{
	ASSERT(this);
	LOG0(Log::Normal, "���Z�b�g");

#if defined(AREASET_LOG)
	LOG0(Log::Normal, "�G���A�Z�b�g�ݒ� $00");
#endif	// AREASET_LOG

	// �G���A�w�菉����
	area = 0;
}

//---------------------------------------------------------------------------
//
//	�Z�[�u
//
//---------------------------------------------------------------------------
BOOL FASTCALL AreaSet::Save(Fileio *fio, int /*ver*/)
{
	size_t sz;

	ASSERT(this);
	LOG0(Log::Normal, "�Z�[�u");

	// �T�C�Y���Z�[�u
	sz = sizeof(area);
	if (!fio->Write(&sz, (int)sizeof(sz))) {
		return FALSE;
	}

	// �G���A�����Z�[�u
	if (!fio->Write(&area, (int)sizeof(area))) {
		return FALSE;
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	���[�h
//
//---------------------------------------------------------------------------
BOOL FASTCALL AreaSet::Load(Fileio *fio, int /*ver*/)
{
	size_t sz;

	ASSERT(this);
	LOG0(Log::Normal, "���[�h");

	// �T�C�Y�����[�h
	if (!fio->Read(&sz, (int)sizeof(sz))) {
		return FALSE;
	}

	// �T�C�Y���r
	if (sz != sizeof(area)) {
		return FALSE;
	}

	// �G���A�������[�h
	if (!fio->Read(&area, (int)sizeof(area))) {
		return FALSE;
	}

	// �K�p
	memory->MakeContext(FALSE);

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	�ݒ�K�p
//
//---------------------------------------------------------------------------
void FASTCALL AreaSet::ApplyCfg(const Config* /*config*/)
{
	ASSERT(this);
	LOG0(Log::Normal, "�ݒ�K�p");
}

//---------------------------------------------------------------------------
//
//	�o�C�g�ǂݍ���
//
//---------------------------------------------------------------------------
DWORD FASTCALL AreaSet::ReadByte(DWORD addr)
{
	ASSERT(this);
	ASSERT((addr >= memdev.first) && (addr <= memdev.last));

	// ��Ƀo�X�G���[
	cpu->BusErr(addr, TRUE);
	return 0xff;
}

//---------------------------------------------------------------------------
//
//	�o�C�g��������
//
//---------------------------------------------------------------------------
void FASTCALL AreaSet::WriteByte(DWORD addr, DWORD data)
{
	ASSERT(this);
	ASSERT((addr >= memdev.first) && (addr <= memdev.last));

	// 2�o�C�g�����Ƀ}�b�v
	addr &= 1;

	// ��A�h���X�̓G���A�Z�b�g
	if (addr & 1) {
		LOG1(Log::Detail, "�G���A�Z�b�g�ݒ� $%02X", data);

		// �f�[�^�L��
		area = data;

		// �������}�b�v�č\�z
		memory->MakeContext(FALSE);
		return;
	}

	// �����A�h���X�̓f�R�[�h����Ă��Ȃ�
}

//---------------------------------------------------------------------------
//
//	�ǂݍ��݂̂�
//
//---------------------------------------------------------------------------
DWORD FASTCALL AreaSet::ReadOnly(DWORD addr) const
{
	ASSERT(this);
	ASSERT((addr >= memdev.first) && (addr <= memdev.last));

	// EVEN��0xff�AODD�͐ݒ�l��Ԃ�
	if (addr & 1) {
		return area;
	}
	return 0xff;
}

//---------------------------------------------------------------------------
//
//	�G���A�Z�b�g�擾
//
//---------------------------------------------------------------------------
DWORD FASTCALL AreaSet::GetArea() const
{
	ASSERT(this);
	return area;
}
