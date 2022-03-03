//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2005 �o�h�D(ytanaka@ipc-tokai.or.jp)
//	[ �O���t�B�b�NVRAM ]
//
//---------------------------------------------------------------------------

#include "os.h"
#include "xm6.h"
#include "vm.h"
#include "log.h"
#include "schedule.h"
#include "fileio.h"
#include "render.h"
#include "renderin.h"
#include "gvram.h"

//===========================================================================
//
//	�O���t�B�b�NVRAM�n���h��
//
//===========================================================================
//#define GVRAM_LOG

//---------------------------------------------------------------------------
//
//	�R���X�g���N�^
//
//---------------------------------------------------------------------------
GVRAMHandler::GVRAMHandler(Render *rend, BYTE *mem, CPU *p)
{
	ASSERT(rend);
	ASSERT(mem);
	ASSERT(p);

	render = rend;
	gvram = mem;
	cpu = p;
}

//===========================================================================
//
//	�O���t�B�b�NVRAM�n���h��(1024�~1024)
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	�R���X�g���N�^
//
//---------------------------------------------------------------------------
GVRAM1024::GVRAM1024(Render *rend, BYTE *mem, CPU *cpu) : GVRAMHandler(rend, mem, cpu)
{
}

//---------------------------------------------------------------------------
//
//	�o�C�g�ǂݍ���
//
//---------------------------------------------------------------------------
DWORD FASTCALL GVRAM1024::ReadByte(DWORD addr)
{
	DWORD offset;

	ASSERT(this);
	ASSERT(addr <= 0x1fffff);

	// �����o�C�g�͓ǂݏo���Ă�0
	if ((addr & 1) == 0) {
		return 0x00;
	}

	// ���ʍ�
	offset = addr & 0x3ff;

	// �㉺���E�ŕ�����
	if (addr & 0x100000) {
		if (addr & 0x400) {
			// �y�[�W3���
			addr >>= 1;
			addr &= 0x7fc00;
			addr |= offset;
			return (gvram[addr] >> 4);
		}
		else {
			// �y�[�W2���
			addr >>= 1;
			addr &= 0x7fc00;
			addr |= offset;
			return (gvram[addr] & 0x0f);
		}
	}
	else {
		if (addr & 0x400) {
			// �y�[�W1���
			addr >>= 1;
			addr &= 0x7fc00;
			addr |= offset;
			return (gvram[addr ^ 1] >> 4);
		}
		else {
			// �y�[�W0���
			addr >>= 1;
			addr &= 0x7fc00;
			addr |= offset;
			return (gvram[addr ^ 1] & 0x0f);
		}
	}
}

//---------------------------------------------------------------------------
//
//	���[�h�ǂݍ���
//
//---------------------------------------------------------------------------
DWORD FASTCALL GVRAM1024::ReadWord(DWORD addr)
{
	DWORD offset;

	ASSERT(this);
	ASSERT(addr <= 0x1fffff);
	ASSERT((addr & 1) == 0);

	// ���ʍ�
	offset = addr & 0x3ff;

	// �㉺���E�ŕ�����
	if (addr & 0x100000) {
		if (addr & 0x400) {
			// �y�[�W3���
			addr >>= 1;
			addr &= 0x7fc00;
			addr |= offset;
			return (gvram[addr ^ 1] >> 4);
		}
		else {
			// �y�[�W2���
			addr >>= 1;
			addr &= 0x7fc00;
			addr |= offset;
			return (gvram[addr ^ 1] & 0x0f);
		}
	}
	else {
		if (addr & 0x400) {
			// �y�[�W1���
			addr >>= 1;
			addr &= 0x7fc00;
			addr |= offset;
			return (gvram[addr] >> 4);
		}
		else {
			// �y�[�W0���
			addr >>= 1;
			addr &= 0x7fc00;
			addr |= offset;
			return (gvram[addr] & 0x0f);
		}
	}
}

//---------------------------------------------------------------------------
//
//	�o�C�g��������
//
//---------------------------------------------------------------------------
void FASTCALL GVRAM1024::WriteByte(DWORD addr, DWORD data)
{
	DWORD offset;
	DWORD mem;

	ASSERT(this);
	ASSERT(addr <= 0x1fffff);
	ASSERT(data < 0x100);

	// �����o�C�g�͏������߂Ȃ�
	if ((addr & 1) == 0) {
		return;
	}

	// ���ʍ�
	offset = addr & 0x3ff;

	// �㉺���E�ŕ�����
	if (addr & 0x100000) {
		if (addr & 0x400) {
			// �y�[�W3���
			addr >>= 1;
			addr &= 0x7fc00;
			addr |= offset;

			// ��ʃj�u����
			mem = (gvram[addr] & 0x0f);
			mem |= (data << 4);

			// ��������
			if (gvram[addr] != mem) {
				gvram[addr] = (BYTE)mem;
				render->GrpMem(addr, 3);
			}
		}
		else {
			// �y�[�W2���
			addr >>= 1;
			addr &= 0x7fc00;
			addr |= offset;

			// ���ʃj�u����
			mem = (gvram[addr] & 0xf0);
			mem |= (data & 0x0f);

			// ��������
			if (gvram[addr] != mem) {
				gvram[addr] = (BYTE)mem;
				render->GrpMem(addr, 2);
			}
		}
	}
	else {
		if (addr & 0x400) {
			// �y�[�W1���
			addr >>= 1;
			addr &= 0x7fc00;
			addr |= offset;

			// ��ʃj�u����
			mem = (gvram[addr ^ 1] & 0x0f);
			mem |= (data << 4);

			// ��������
			if (gvram[addr ^ 1] != mem) {
				gvram[addr ^ 1] = (BYTE)mem;
				render->GrpMem(addr ^ 1, 1);
			}
		}
		else {
			// �y�[�W0���
			addr >>= 1;
			addr &= 0x7fc00;
			addr |= offset;

			// ���ʃj�u���w
			mem = (gvram[addr ^ 1] & 0xf0);
			mem |= (data & 0x0f);

			// ��������
			if (gvram[addr ^ 1] != mem) {
				gvram[addr ^ 1] = (BYTE)mem;
				render->GrpMem(addr ^ 1, 0);
			}
		}
	}
}

//---------------------------------------------------------------------------
//
//	���[�h��������
//
//---------------------------------------------------------------------------
void FASTCALL GVRAM1024::WriteWord(DWORD addr, DWORD data)
{
	DWORD offset;
	DWORD mem;

	ASSERT(this);
	ASSERT(addr <= 0x1fffff);
	ASSERT(data < 0x10000);

	// ���ʍ�
	offset = addr & 0x3ff;

	// �㉺���E�ŕ�����
	if (addr & 0x100000) {
		if (addr & 0x400) {
			// �y�[�W3���
			addr >>= 1;
			addr &= 0x7fc00;
			addr |= offset;

			// ��ʃj�u����
			mem = (gvram[addr ^ 1] & 0x0f);
			data &= 0x0f;
			mem |= (data << 4);

			// ��������
			if (gvram[addr ^ 1] != mem) {
				gvram[addr ^ 1] = (BYTE)mem;
				render->GrpMem(addr ^ 1, 3);
			}
		}
		else {
			// �y�[�W2���
			addr >>= 1;
			addr &= 0x7fc00;
			addr |= offset;

			// ���ʃj�u����
			mem = (gvram[addr ^ 1] & 0xf0);
			mem |= (data & 0x0f);

			// ��������
			if (gvram[addr ^ 1] != mem) {
				gvram[addr ^ 1] = (BYTE)mem;
				render->GrpMem(addr ^ 1, 2);
			}
		}
	}
	else {
		if (addr & 0x400) {
			// �y�[�W1���
			addr >>= 1;
			addr &= 0x7fc00;
			addr |= offset;

			// ��ʃj�u����
			mem = (gvram[addr] & 0x0f);
			data &= 0x0f;
			mem |= (data << 4);

			// ��������
			if (gvram[addr] != mem) {
				gvram[addr] = (BYTE)mem;
				render->GrpMem(addr, 1);
			}
		}
		else {
			// �y�[�W0���
			addr >>= 1;
			addr &= 0x7fc00;
			addr |= offset;

			// ���ʃj�u����
			mem = (gvram[addr] & 0xf0);
			mem |= (data & 0x0f);

			// ��������
			if (gvram[addr] != mem) {
				gvram[addr] = (BYTE)mem;
				render->GrpMem(addr, 0);
			}
		}
	}
}

//---------------------------------------------------------------------------
//
//	�ǂݍ��݂̂�
//
//---------------------------------------------------------------------------
DWORD FASTCALL GVRAM1024::ReadOnly(DWORD addr) const
{
	DWORD offset;

	ASSERT(this);
	ASSERT(addr <= 0x1fffff);

	// �����o�C�g�͓ǂݏo���Ă�0
	if ((addr & 1) == 0) {
		return 0x00;
	}

	// ���ʍ�
	offset = addr & 0x3ff;

	// �㉺���E�ŕ�����
	if (addr & 0x100000) {
		if (addr & 0x400) {
			// �y�[�W3���
			addr >>= 1;
			addr &= 0x7fc00;
			addr |= offset;
			return (gvram[addr] >> 4);
		}
		else {
			// �y�[�W2���
			addr >>= 1;
			addr &= 0x7fc00;
			addr |= offset;
			return (gvram[addr] & 0x0f);
		}
	}
	else {
		if (addr & 0x400) {
			// �y�[�W1���
			addr >>= 1;
			addr &= 0x7fc00;
			addr |= offset;
			return (gvram[addr ^ 1] >> 4);
		}
		else {
			// �y�[�W0���
			addr >>= 1;
			addr &= 0x7fc00;
			addr |= offset;
			return (gvram[addr ^ 1] & 0x0f);
		}
	}
}

//===========================================================================
//
//	�O���t�B�b�NVRAM�n���h��(16�F)
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	�R���X�g���N�^
//
//---------------------------------------------------------------------------
GVRAM16::GVRAM16(Render *rend, BYTE *mem, CPU *cpu) : GVRAMHandler(rend, mem, cpu)
{
}

//---------------------------------------------------------------------------
//
//	�o�C�g�ǂݍ���
//
//---------------------------------------------------------------------------
DWORD FASTCALL GVRAM16::ReadByte(DWORD addr)
{
	ASSERT(this);
	ASSERT(addr <= 0x1fffff);

	// ��A�h���X�̂�
	if (addr & 1) {
		if (addr < 0x80000) {
			// �y�[�W0:���[�h�̉��ʃo�C�gb0-b3
			return (gvram[addr ^ 1] & 0x0f);
		}

		if (addr < 0x100000) {
			// �y�[�W1:���[�h�̉��ʃo�C�gb4-b7
			addr &= 0x7ffff;
			return (gvram[addr ^ 1] >> 4);
		}

		if (addr < 0x180000) {
			// �y�[�W2:���[�h�̏�ʃo�C�gb0-b3
			addr &= 0x7ffff;
			return (gvram[addr] & 0x0f);
		}

		// �y�[�W3:���[�h�̏�ʃo�C�gb4-b7
		addr &= 0x7ffff;
		return (gvram[addr] >> 4);
	}

	// �����A�h���X�͏��0
	return 0;
}

//---------------------------------------------------------------------------
//
//	���[�h�ǂݍ���
//
//---------------------------------------------------------------------------
DWORD FASTCALL GVRAM16::ReadWord(DWORD addr)
{
	ASSERT(this);
	ASSERT(addr <= 0x1fffff);
	ASSERT((addr & 1) == 0);

	if (addr < 0x80000) {
		// �y�[�W0:���[�h�̉��ʃo�C�gb0-b3
		return (gvram[addr] & 0x0f);
	}

	if (addr < 0x100000) {
		// �y�[�W1:���[�h�̉��ʃo�C�gb4-b7
		addr &= 0x7ffff;
		return (gvram[addr] >> 4);
	}

	if (addr < 0x180000) {
		// �y�[�W2:���[�h�̏�ʃo�C�gb0-b3
		addr &= 0x7ffff;
		return (gvram[addr ^ 1] & 0x0f);
	}

	// �y�[�W3:���[�h�̏�ʃo�C�gb4-b7
	addr &= 0x7ffff;
	return (gvram[addr ^ 1] >> 4);
}

//---------------------------------------------------------------------------
//
//	�o�C�g��������
//
//---------------------------------------------------------------------------
void FASTCALL GVRAM16::WriteByte(DWORD addr, DWORD data)
{
	DWORD mem;

	ASSERT(this);
	ASSERT(addr <= 0x1fffff);
	ASSERT(data < 0x100);

	// ��A�h���X�̂�
	if (addr & 1) {
		if (addr < 0x80000) {
			// �y�[�W0:���[�h�̉��ʃo�C�gb0-b3
			mem = (gvram[addr ^ 1] & 0xf0);
			mem |= (data & 0x0f);

			// ��������
			if (gvram[addr ^ 1] != mem) {
				gvram[addr ^ 1] = (BYTE)mem;
				render->GrpMem(addr ^ 1, 0);
			}
			return;
		}

		if (addr < 0x100000) {
			// �y�[�W1:���[�h�̉��ʃo�C�gb4-b7
			addr &= 0x7ffff;
			mem = (gvram[addr ^ 1] & 0x0f);
			mem |= (data << 4);

			// ��������
			if (gvram[addr ^ 1] != mem) {
				gvram[addr ^ 1] = (BYTE)mem;
				render->GrpMem(addr ^ 1, 1);
			}
			return;
		}

		if (addr < 0x180000) {
			// �y�[�W2:���[�h�̏�ʃo�C�gb0-b3
			addr &= 0x7ffff;
			mem = (gvram[addr] & 0xf0);
			mem |= (data & 0x0f);

			// ��������
			if (gvram[addr] != mem) {
				gvram[addr] = (BYTE)mem;
				render->GrpMem(addr, 2);
			}
			return;
		}

		// �y�[�W3:���[�h�̏�ʃo�C�gb4-b7
		addr &= 0x7ffff;
		mem = (gvram[addr] & 0x0f);
		mem |= (data << 4);

		// ��������
		if (gvram[addr] != mem) {
			gvram[addr] = (BYTE)mem;
			render->GrpMem(addr, 3);
		}
		return;
	}

	// �����A�h���X�͏������߂Ȃ�
}

//---------------------------------------------------------------------------
//
//	���[�h��������
//
//---------------------------------------------------------------------------
void FASTCALL GVRAM16::WriteWord(DWORD addr, DWORD data)
{
	DWORD mem;

	ASSERT(this);
	ASSERT(addr <= 0x1fffff);
	ASSERT((addr & 1) == 0);
	ASSERT(data < 0x10000);

	if (addr < 0x80000) {
		// �y�[�W0:���[�h�̉��ʃo�C�gb0-b3
		mem = (gvram[addr] & 0xf0);
		mem |= (data & 0x0f);

		// ��������
		if (gvram[addr] != mem) {
			gvram[addr] = (BYTE)mem;
			render->GrpMem(addr, 0);
		}
		return;
	}
	if (addr < 0x100000) {
		// �y�[�W1:���[�h�̉��ʃo�C�gb4-b7
		addr &= 0x7ffff;
		mem = (gvram[addr] & 0x0f);
		data &= 0x0f;
		mem |= (data << 4);

		// ��������
		if (gvram[addr] != mem) {
			gvram[addr] = (BYTE)mem;
			render->GrpMem(addr, 1);
		}
		return;
	}
	if (addr < 0x180000) {
		// �y�[�W2:���[�h�̏�ʃo�C�gb0-b3
		addr &= 0x7ffff;
		mem = (gvram[addr ^ 1] & 0xf0);
		mem |= (data & 0x0f);

		// ��������
		if (gvram[addr ^ 1] != mem) {
			gvram[addr ^ 1] = (BYTE)mem;
			render->GrpMem(addr ^ 1, 2);
		}
		return;
	}

	// �y�[�W3:���[�h�̏�ʃo�C�gb4-b7
	addr &= 0x7ffff;
	mem = (gvram[addr ^ 1] & 0x0f);
	data &= 0x0f;
	mem |= (data << 4);

	// ��������
	if (gvram[addr ^ 1] != mem) {
		gvram[addr ^ 1] = (BYTE)mem;
		render->GrpMem(addr ^ 1, 3);
	}
}

//---------------------------------------------------------------------------
//
//	�ǂݍ��݂̂�
//
//---------------------------------------------------------------------------
DWORD FASTCALL GVRAM16::ReadOnly(DWORD addr) const
{
	ASSERT(this);
	ASSERT(addr <= 0x1fffff);

	// ��A�h���X�̂�
	if (addr & 1) {
		if (addr < 0x80000) {
			// �y�[�W0:���[�h�̉��ʃo�C�gb0-b3
			return (gvram[addr ^ 1] & 0x0f);
		}

		if (addr < 0x100000) {
			// �y�[�W1:���[�h�̉��ʃo�C�gb4-b7
			addr &= 0x7ffff;
			return (gvram[addr ^ 1] >> 4);
		}

		if (addr < 0x180000) {
			// �y�[�W2:���[�h�̏�ʃo�C�gb0-b3
			addr &= 0x7ffff;
			return (gvram[addr] & 0x0f);
		}

		// �y�[�W3:���[�h�̏�ʃo�C�gb4-b7
		addr &= 0x7ffff;
		return (gvram[addr] >> 4);
	}

	// �����A�h���X�͏��0
	return 0;
}

//===========================================================================
//
//	�O���t�B�b�NVRAM�n���h��(256�F)
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	�R���X�g���N�^
//
//---------------------------------------------------------------------------
GVRAM256::GVRAM256(Render *rend, BYTE *mem, CPU *p) : GVRAMHandler(rend, mem, p)
{
}

//---------------------------------------------------------------------------
//
//	�o�C�g�ǂݍ���
//
//---------------------------------------------------------------------------
DWORD FASTCALL GVRAM256::ReadByte(DWORD addr)
{
	ASSERT(this);
	ASSERT(addr <= 0x1fffff);

	// �y�[�W0
	if (addr < 0x80000) {
		if (addr & 1) {
			// ���[�h�̉��ʃo�C�g
			return gvram[addr ^ 1];
		}
		return 0;
	}

	// �y�[�W1
	if (addr < 0x100000) {
		addr &= 0x7ffff;
		if (addr & 1) {
			// ���[�h�̏�ʃo�C�g
			return gvram[addr];
		}
		return 0;
	}

	// �o�X�G���[
	cpu->BusErr(addr + 0xc00000, TRUE);
	return 0xff;
}

//---------------------------------------------------------------------------
//
//	���[�h�ǂݍ���
//
//---------------------------------------------------------------------------
DWORD FASTCALL GVRAM256::ReadWord(DWORD addr)
{
	ASSERT(this);
	ASSERT(addr <= 0x1fffff);
	ASSERT((addr & 1) == 0);

	// �y�[�W0
	if (addr < 0x80000) {
		// ���[�h�̉��ʃo�C�g
		return gvram[addr];
	}

	// �y�[�W1
	if (addr < 0x100000) {
		addr &= 0x7ffff;
		// ���[�h�̏�ʃo�C�g
		return gvram[addr ^ 1];
	}

	// �o�X�G���[
	cpu->BusErr(addr + 0xc00000, TRUE);
	return 0xff;
}

//---------------------------------------------------------------------------
//
//	�o�C�g��������
//
//---------------------------------------------------------------------------
void FASTCALL GVRAM256::WriteByte(DWORD addr, DWORD data)
{
	ASSERT(this);
	ASSERT(addr <= 0x1fffff);
	ASSERT(data < 0x100);

	// �y�[�W0
	if (addr < 0x80000) {
		if (addr & 1) {
			// ���[�h�̉��ʃo�C�g
			if (gvram[addr ^ 1] != data) {
				gvram[addr ^ 1] = (BYTE)data;
				render->GrpMem(addr ^ 1, 0);
			}
		}
		return;
	}

	// �y�[�W1(�u���b�N2)
	if (addr < 0x100000) {
		addr &= 0x7ffff;
		if (addr & 1) {
			// ���[�h�̏�ʃo�C�g
			if (gvram[addr] != data) {
				gvram[addr] = (BYTE)data;
				render->GrpMem(addr, 2);
			}
		}
		return;
	}

	// �o�X�G���[
	cpu->BusErr(addr + 0xc00000, FALSE);
}

//---------------------------------------------------------------------------
//
//	���[�h��������
//
//---------------------------------------------------------------------------
void FASTCALL GVRAM256::WriteWord(DWORD addr, DWORD data)
{
	ASSERT(this);
	ASSERT(addr <= 0x1fffff);
	ASSERT((addr & 1) == 0);
	ASSERT(data < 0x10000);

	// �y�[�W0
	if (addr < 0x80000) {
		// ���[�h�̉��ʃo�C�g
		if (gvram[addr] != data) {
			gvram[addr] = (BYTE)data;
			render->GrpMem(addr, 0);
		}
		return;
	}

	// �y�[�W1(�u���b�N2)
	if (addr < 0x100000) {
		addr &= 0x7ffff;
		// ���[�h�̏�ʃo�C�g
		if (gvram[addr ^ 1] != data) {
			gvram[addr ^ 1] = (BYTE)data;
			render->GrpMem(addr ^ 1, 2);
		}
		return;
	}

	// �o�X�G���[
	cpu->BusErr(addr + 0xc00000, FALSE);
}

//---------------------------------------------------------------------------
//
//	�ǂݍ��݂̂�
//
//---------------------------------------------------------------------------
DWORD FASTCALL GVRAM256::ReadOnly(DWORD addr) const
{
	ASSERT(this);
	ASSERT(addr <= 0x1fffff);

	// �y�[�W0
	if (addr < 0x80000) {
		if (addr & 1) {
			// ���[�h�̉��ʃo�C�g
			return gvram[addr ^ 1];
		}
		return 0;
	}

	// �y�[�W1
	if (addr < 0x100000) {
		addr &= 0x7ffff;
		if (addr & 1) {
			// ���[�h�̏�ʃo�C�g
			return gvram[addr];
		}
		return 0;
	}

	// �o�X�G���[
	return 0xff;
}

//===========================================================================
//
//	�O���t�B�b�NVRAM�n���h��(����)
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	�R���X�g���N�^
//
//---------------------------------------------------------------------------
GVRAMNDef::GVRAMNDef(Render *rend, BYTE *mem, CPU *p) : GVRAMHandler(rend, mem, p)
{
}

//---------------------------------------------------------------------------
//
//	�o�C�g�ǂݍ���
//
//---------------------------------------------------------------------------
DWORD FASTCALL GVRAMNDef::ReadByte(DWORD addr)
{
	ASSERT(this);
	ASSERT(addr <= 0x1fffff);

	// �����y�[�W
	if (addr & 0x80000) {
		return 0;
	}

	// ���ʃy�[�W
	addr &= 0x7ffff;
	if (addr & 1) {
		return gvram[addr ^ 1];
	}
	return 0;
}

//---------------------------------------------------------------------------
//
//	���[�h�ǂݍ���
//
//---------------------------------------------------------------------------
DWORD FASTCALL GVRAMNDef::ReadWord(DWORD addr)
{
	ASSERT(this);
	ASSERT(addr <= 0x1fffff);
	ASSERT((addr & 1) == 0);

	// �����y�[�W
	if (addr & 0x80000) {
		return 0;
	}

	// ���ʃy�[�W
	addr &= 0x7ffff;
	return gvram[addr ^ 1];
}

//---------------------------------------------------------------------------
//
//	�o�C�g��������
//
//---------------------------------------------------------------------------
void FASTCALL GVRAMNDef::WriteByte(DWORD addr, DWORD data)
{
	ASSERT(this);
	ASSERT(addr <= 0x1fffff);
	ASSERT(data < 0x100);

	// �����y�[�W
	if (addr & 0x80000) {
		return;
	}

	// ���ʃy�[�W
	addr &= 0x7ffff;
	if (addr & 1) {
		if (gvram[addr ^ 1] != data) {
			gvram[addr ^ 1] = (BYTE)data;
			render->GrpMem(addr ^ 1, 0);
		}
	}
}

//---------------------------------------------------------------------------
//
//	���[�h��������
//
//---------------------------------------------------------------------------
void FASTCALL GVRAMNDef::WriteWord(DWORD addr, DWORD data)
{
	ASSERT(this);
	ASSERT(addr <= 0x1fffff);
	ASSERT((addr & 1) == 0);
	ASSERT(data < 0x10000);

	// �����y�[�W
	if (addr & 0x80000) {
		return;
	}

	// ���ʃy�[�W
	addr &= 0x7ffff;
	if (gvram[addr ^ 1] != data) {
		gvram[addr ^ 1] = (BYTE)data;
		render->GrpMem(addr ^ 1, 0);
	}
}

//---------------------------------------------------------------------------
//
//	�ǂݍ��݂̂�
//
//---------------------------------------------------------------------------
DWORD FASTCALL GVRAMNDef::ReadOnly(DWORD addr) const
{
	ASSERT(this);
	ASSERT(addr <= 0x1fffff);

	// �����y�[�W
	if (addr & 0x80000) {
		return 0;
	}

	// ���ʃy�[�W
	addr &= 0x7ffff;
	if (addr & 1) {
		return gvram[addr ^ 1];
	}
	return 0;
}

//===========================================================================
//
//	�O���t�B�b�NVRAM�n���h��(65536�F)
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	�R���X�g���N�^
//
//---------------------------------------------------------------------------
GVRAM64K::GVRAM64K(Render *rend, BYTE *mem, CPU *p) : GVRAMHandler(rend, mem, p)
{
}

//---------------------------------------------------------------------------
//
//	�o�C�g�ǂݍ���
//
//---------------------------------------------------------------------------
DWORD FASTCALL GVRAM64K::ReadByte(DWORD addr)
{
	ASSERT(this);
	ASSERT(addr <= 0x1fffff);

	if (addr < 0x80000) {
		return gvram[addr ^ 1];
	}

	// �o�X�G���[
	cpu->BusErr(addr + 0xc00000, TRUE);
	return 0xff;
}

//---------------------------------------------------------------------------
//
//	���[�h�ǂݍ���
//
//---------------------------------------------------------------------------
DWORD FASTCALL GVRAM64K::ReadWord(DWORD addr)
{
	ASSERT(this);
	ASSERT(addr <= 0x1fffff);
	ASSERT((addr & 1) == 0);

	if (addr < 0x80000) {
		return *(WORD*)(&gvram[addr]);
	}

	// �o�X�G���[
	cpu->BusErr(addr + 0xc00000, TRUE);
	return 0xffff;
}

//---------------------------------------------------------------------------
//
//	�o�C�g��������
//
//---------------------------------------------------------------------------
void FASTCALL GVRAM64K::WriteByte(DWORD addr, DWORD data)
{
	ASSERT(this);
	ASSERT(addr <= 0x1fffff);
	ASSERT(data < 0x100);

	if (addr < 0x80000) {
		if (gvram[addr ^ 1] != data) {
			gvram[addr ^ 1] = (BYTE)data;
			render->GrpMem(addr ^ 1, 0);
		}
		return;
	}

	// �o�X�G���[
	cpu->BusErr(addr + 0xc00000, FALSE);
}

//---------------------------------------------------------------------------
//
//	���[�h��������
//
//---------------------------------------------------------------------------
void FASTCALL GVRAM64K::WriteWord(DWORD addr, DWORD data)
{
	ASSERT(this);
	ASSERT(addr <= 0x1fffff);
	ASSERT((addr & 1) == 0);
	ASSERT(data < 0x10000);

	if (addr < 0x80000) {
		if (*(WORD*)(&gvram[addr]) != data) {
			*(WORD*)(&gvram[addr]) = (WORD)data;
			render->GrpMem(addr, 0);
		}
		return;
	}

	// �o�X�G���[
	cpu->BusErr(addr + 0xc00000, FALSE);
}

//---------------------------------------------------------------------------
//
//	�ǂݍ��݂̂�
//
//---------------------------------------------------------------------------
DWORD FASTCALL GVRAM64K::ReadOnly(DWORD addr) const
{
	ASSERT(this);
	ASSERT(addr <= 0x1fffff);

	if (addr < 0x80000) {
		return gvram[addr ^ 1];
	}

	// �o�X�G���[
	return 0xff;
}

//===========================================================================
//
//	�O���t�B�b�NVRAM
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	�R���X�g���N�^
//
//---------------------------------------------------------------------------
GVRAM::GVRAM(VM *p) : MemDevice(p)
{
	// �f�o�C�XID��������
	dev.id = MAKEID('G', 'V', 'R', 'M');
	dev.desc = "Graphic VRAM";

	// �J�n�A�h���X�A�I���A�h���X
	memdev.first = 0xc00000;
	memdev.last = 0xdfffff;

	// ���[�N�G���A
	gvram = NULL;
	render = NULL;

	// �n���h��
	handler = NULL;
	hand1024 = NULL;
	hand16 = NULL;
	hand256 = NULL;
	handNDef = NULL;
	hand64K = NULL;
}

//---------------------------------------------------------------------------
//
//	������
//
//---------------------------------------------------------------------------
BOOL FASTCALL GVRAM::Init()
{
	ASSERT(this);

	// ��{�N���X
	if (!MemDevice::Init()) {
		return FALSE;
	}

	// �������m�ہA�N���A
	try {
		gvram = new BYTE[ 0x80000 ];
	}
	catch (...) {
		return FALSE;
	}
	if (!gvram) {
		return FALSE;
	}
	memset(gvram, 0, 0x80000);

	// �����_���擾
	render = (Render*)vm->SearchDevice(MAKEID('R', 'E', 'N', 'D'));
	ASSERT(render);

	// �n���h���쐬
	hand1024 = new GVRAM1024(render, gvram, cpu);
	hand16 = new GVRAM16(render, gvram, cpu);
	hand256 = new GVRAM256(render, gvram, cpu);
	handNDef = new GVRAMNDef(render, gvram, cpu);
	hand64K = new GVRAM64K(render, gvram, cpu);

	// �f�[�^������
	gvdata.mem = TRUE;
	gvdata.siz = 0;
	gvdata.col = 3;
	gvcount = 0;

	// �n���h��������(64K)
	gvdata.type = 4;
	handler = hand64K;

	// �����N���A�}�X�N
	gvdata.mask[0] = 0xfff0;
	gvdata.mask[1] = 0xff0f;
	gvdata.mask[2] = 0xf0ff;
	gvdata.mask[3] = 0x0fff;

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	�N���[���A�b�v
//
//---------------------------------------------------------------------------
void FASTCALL GVRAM::Cleanup()
{
	ASSERT(this);

	// �n���h�����
	if (hand64K) {
		delete hand64K;
		hand64K = NULL;
	}
	if (handNDef) {
		delete handNDef;
		handNDef = NULL;
	}
	if (hand256) {
		delete hand256;
		hand256 = NULL;
	}
	if (hand16) {
		delete hand16;
		hand16 = NULL;
	}
	if (hand1024) {
		delete hand1024;
		hand1024 = NULL;
	}
	handler = NULL;

	// ���������
	if (gvram) {
		delete[] gvram;
		gvram = NULL;
	}

	// ��{�N���X��
	MemDevice::Cleanup();
}

//---------------------------------------------------------------------------
//
//	���Z�b�g
//
//---------------------------------------------------------------------------
void FASTCALL GVRAM::Reset()
{
	ASSERT(this);
	ASSERT_DIAG();

	LOG0(Log::Normal, "���Z�b�g");

	// �����N���A�Ȃ�
	gvdata.plane[0] = FALSE;
	gvdata.plane[1] = FALSE;
	gvdata.plane[2] = FALSE;
	gvdata.plane[3] = FALSE;

	// �n���h��������(64K)
	gvdata.mem = TRUE;
	gvdata.siz = 0;
	gvdata.col = 3;
	gvdata.type = 4;
	handler = hand64K;

	// �A�N�Z�X�J�E���g0
	gvcount = 0;
}

//---------------------------------------------------------------------------
//
//	�Z�[�u
//
//---------------------------------------------------------------------------
BOOL FASTCALL GVRAM::Save(Fileio *fio, int /*ver*/)
{
	size_t sz;

	ASSERT(this);
	ASSERT(fio);
	ASSERT_DIAG();

	LOG0(Log::Normal, "�Z�[�u");

	// ���������Z�[�u
	if (!fio->Write(gvram, 0x80000)) {
		return FALSE;
	}

	// �T�C�Y���Z�[�u
	sz = sizeof(gvram_t);
	if (!fio->Write(&sz, sizeof(sz))) {
		return FALSE;
	}

	// ���̂��Z�[�u
	if (!fio->Write(&gvdata, (int)sz)) {
		return FALSE;
	}

	// gvcount(version2.04�Œǉ�)
	if (!fio->Write(&gvcount, sizeof(gvcount))) {
		return FALSE;
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	���[�h
//
//---------------------------------------------------------------------------
BOOL FASTCALL GVRAM::Load(Fileio *fio, int ver)
{
	size_t sz;
	int line;

	ASSERT(this);
	ASSERT(fio);
	ASSERT(ver >= 0x0200);
	ASSERT_DIAG();

	LOG0(Log::Normal, "���[�h");

	// �����������[�h
	if (!fio->Read(gvram, 0x80000)) {
		return FALSE;
	}

	// �T�C�Y�����[�h�A�ƍ�
	if (!fio->Read(&sz, sizeof(sz))) {
		return FALSE;
	}
	if (sz != sizeof(gvram_t)) {
		return FALSE;
	}

	// ���̂����[�h
	if (!fio->Read(&gvdata, (int)sz)) {
		return FALSE;
	}

	// gvcount(version2.04�Œǉ�)
	gvcount = 0;
	if (ver >= 0x0204) {
		if (!fio->Read(&gvcount, sizeof(gvcount))) {
			return FALSE;
		}
	}

	// �����_���֒ʒm
	for (line=0; line<0x200; line++) {
		render->GrpAll(line, 0);
		render->GrpAll(line, 1);
		render->GrpAll(line, 2);
		render->GrpAll(line, 3);
	}

	// �n���h����I��
	switch (gvdata.type) {
		case 0:
			ASSERT(hand1024);
			handler = hand1024;
			break;
		// 16�F�^�C�v
		case 1:
			ASSERT(hand16);
			handler = hand16;
			break;
		// 256�F�^�C�v
		case 2:
			ASSERT(hand256);
			handler = hand256;
			break;
		// ����`�^�C�v
		case 3:
			ASSERT(handNDef);
			handler = handNDef;
			break;
		// 64K�F�^�C�v
		case 4:
			ASSERT(hand64K);
			handler = hand64K;
			break;
		// ���̑�
		default:
			ASSERT(FALSE);
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	�ݒ�K�p
//
//---------------------------------------------------------------------------
void FASTCALL GVRAM::ApplyCfg(const Config* /*config*/)
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
void FASTCALL GVRAM::AssertDiag() const
{
	// ��{�N���X
	MemDevice::AssertDiag();

	ASSERT(this);
	ASSERT(GetID() == MAKEID('G', 'V', 'R', 'M'));
	ASSERT(memdev.first == 0xc00000);
	ASSERT(memdev.last == 0xdfffff);
	ASSERT(gvram);
	ASSERT(hand1024);
	ASSERT(hand16);
	ASSERT(hand256);
	ASSERT(handNDef);
	ASSERT(hand64K);
	ASSERT(handler);
	ASSERT((gvdata.mem == TRUE) || (gvdata.mem == FALSE));
	ASSERT((gvdata.siz == 0) || (gvdata.siz == 1));
	ASSERT((gvdata.col >= 0) && (gvdata.col <= 3));
	ASSERT((gvdata.type >= 0) && (gvdata.type <= 4));
	ASSERT((gvcount == 0) || (gvcount == 1));
}
#endif	// NDEBUG

//---------------------------------------------------------------------------
//
//	�o�C�g�ǂݍ���
//
//---------------------------------------------------------------------------
DWORD FASTCALL GVRAM::ReadByte(DWORD addr)
{
	ASSERT(this);
	ASSERT((addr >= memdev.first) && (addr <= memdev.last));
	ASSERT_DIAG();

	// �E�F�C�g(0.5�E�F�C�g)
	scheduler->Wait(gvcount);
	gvcount ^= 1;

	// �n���h���ɔC����
	return handler->ReadByte(addr & 0x1fffff);
}

//---------------------------------------------------------------------------
//
//	���[�h�ǂݍ���
//
//---------------------------------------------------------------------------
DWORD FASTCALL GVRAM::ReadWord(DWORD addr)
{
	ASSERT(this);
	ASSERT((addr >= memdev.first) && (addr <= memdev.last));
	ASSERT((addr & 1) == 0);
	ASSERT_DIAG();

	// �E�F�C�g(0.5�E�F�C�g)
	scheduler->Wait(gvcount);
	gvcount ^= 1;

	// �n���h���ɔC����
	return handler->ReadWord(addr & 0x1fffff);
}

//---------------------------------------------------------------------------
//
//	�o�C�g��������
//
//---------------------------------------------------------------------------
void FASTCALL GVRAM::WriteByte(DWORD addr, DWORD data)
{
	ASSERT(this);
	ASSERT((addr >= memdev.first) && (addr <= memdev.last));
	ASSERT(data < 0x100);
	ASSERT_DIAG();

	// �E�F�C�g(0.5�E�F�C�g)
	scheduler->Wait(gvcount);
	gvcount ^= 1;

	// �n���h���ɔC����
	handler->WriteByte(addr & 0x1fffff, data);
}

//---------------------------------------------------------------------------
//
//	���[�h��������
//
//---------------------------------------------------------------------------
void FASTCALL GVRAM::WriteWord(DWORD addr, DWORD data)
{
	ASSERT(this);
	ASSERT((addr >= memdev.first) && (addr <= memdev.last));
	ASSERT((addr & 1) == 0);
	ASSERT(data < 0x10000);
	ASSERT_DIAG();

	// �E�F�C�g(0.5�E�F�C�g)
	scheduler->Wait(gvcount);
	gvcount ^= 1;

	// �n���h���ɔC����
	handler->WriteWord(addr & 0x1fffff, data);
}

//---------------------------------------------------------------------------
//
//	�ǂݍ��݂̂�
//
//---------------------------------------------------------------------------
DWORD FASTCALL GVRAM::ReadOnly(DWORD addr) const
{
	ASSERT(this);
	ASSERT((addr >= memdev.first) && (addr <= memdev.last));
	ASSERT_DIAG();

	// �n���h���ɔC����
	return handler->ReadOnly(addr & 0x1fffff);
}

//---------------------------------------------------------------------------
//
//	GVRAM�擾
//
//---------------------------------------------------------------------------
const BYTE* FASTCALL GVRAM::GetGVRAM() const
{
	ASSERT(this);
	ASSERT_DIAG();

	return gvram;
}

//---------------------------------------------------------------------------
//
//	�^�C�v�ݒ�
//
//---------------------------------------------------------------------------
void FASTCALL GVRAM::SetType(DWORD type)
{
	int next;
	int prev;
	BOOL mem;
	DWORD siz;

	ASSERT(this);
	ASSERT_DIAG();

	// ���݂̒l��ۑ�
	mem = gvdata.mem;
	siz = gvdata.siz;

	// �ݒ�
	if (type & 8) {
		gvdata.mem = TRUE;
	}
	else {
		gvdata.mem = FALSE;
	}
	if (type & 4) {
		gvdata.siz = 1;
	}
	else {
		gvdata.siz = 0;
	}
	gvdata.col = type & 3;

	// ���݂�gvdata.type���L��
	prev = gvdata.type;

	// �V����type������
	if (gvdata.mem) {
		next = 4;
	}
	else {
		if (gvdata.siz) {
			next = 0;
		}
		else {
			next = gvdata.col + 1;
		}
	}

	// ����Ă�������Ȃ���
	if (prev != next) {
		switch (next) {
			// 1024�F�^�C�v
			case 0:
				ASSERT(hand1024);
				handler = hand1024;
				break;
			// 16�F�^�C�v
			case 1:
				ASSERT(hand16);
				handler = hand16;
				break;
			// 256�F�^�C�v
			case 2:
				ASSERT(hand256);
				handler = hand256;
				break;
			// ����`�^�C�v
			case 3:
				LOG0(Log::Warning, "�O���t�B�b�NVRAM ����`�^�C�v");
				ASSERT(handNDef);
				handler = handNDef;
				break;
			// 64K�F�^�C�v
			case 4:
				ASSERT(hand64K);
				handler = hand64K;
				break;
			// ���̑�
			default:
				ASSERT(FALSE);
		}
		gvdata.type = next;
	}

	// �������^�C�v���͎���ʃT�C�Y���قȂ��Ă���΁A�����_���֒ʒm
	if ((gvdata.mem != mem) || (gvdata.siz != siz)) {
		render->SetVC();
	}
}

//---------------------------------------------------------------------------
//
//	�����N���A�ݒ�
//
//---------------------------------------------------------------------------
void FASTCALL GVRAM::FastSet(DWORD mask)
{
	ASSERT(this);
	ASSERT_DIAG();

#if defined(GVRAM_LOG)
	LOG1(Log::Normal, "�����N���A�v���[���w�� %02X", mask);
#endif	// GVRAM_LOG

	if (mask & 0x08) {
		gvdata.plane[3] = TRUE;
	}
	else {
		gvdata.plane[3] = FALSE;
	}

	if (mask & 0x04) {
		gvdata.plane[2] = TRUE;
	}
	else {
		gvdata.plane[2] = FALSE;
	}

	if (mask & 0x02) {
		gvdata.plane[1] = TRUE;
	}
	else {
		gvdata.plane[1] = FALSE;
	}

	if (mask & 0x01) {
		gvdata.plane[0] = TRUE;
	}
	else {
		gvdata.plane[0] = FALSE;
	}
}

//---------------------------------------------------------------------------
//
//	�����N���A
//
//---------------------------------------------------------------------------
void FASTCALL GVRAM::FastClr(const CRTC::crtc_t *p)
{
	ASSERT(this);
	ASSERT_DIAG();

	if (gvdata.siz) {
		// 1024x1024
		if (p->hd >= 1) {
			// 1024�~1024�A512 or 768
			FastClr768(p);
		}
		else {
			// 1024�~1024�A256
			FastClr256(p);
		}
	}
	else {
		// 512�~512
		FastClr512(p);
	}
}

//---------------------------------------------------------------------------
//
//	�����N���A 1024�~1024 512/768
//
//---------------------------------------------------------------------------
void FASTCALL GVRAM::FastClr768(const CRTC::crtc_t *p)
{
	int y;
	int n;
	int j;
	int k;
	DWORD offset;
	WORD *q;

	ASSERT(this);
	ASSERT_DIAG();

#if defined(GVRAM_LOG)
	LOG0(Log::Normal, "�����N���A 1024x1024 (512/768��)");
#endif	// GVRAM_LOG

	// �I�t�Z�b�gy�A���C����n�𓾂�
	y = p->v_scan;
	n = 1;
	if ((p->v_mul == 2) && !(p->lowres)) {
		if (y & 1) {
			return;
		}
		y >>= 1;
	}
	if (p->v_mul == 0) {
		y <<= 1;
		n = 2;
	}

	// ���C�����[�v
	for (j=0; j<n; j++) {
		// �X�N���[�����W�X�^����I�t�Z�b�g�𓾂�
		offset = (y + p->grp_scrly[0]) & 0x3ff;

		// �㔼���A�������ŕ�����
		if (offset < 512) {
			// �|�C���^�쐬
			q = (WORD*)&gvram[offset << 10];

			// ���ʃo�C�g�N���A
			for (k=0; k<512; k++) {
				*q++ &= 0xff00;
			}

			// �t���OUp
			render->GrpAll(offset, 0);
			render->GrpAll(offset, 1);
		}
		else {
			// �|�C���^�쐬
			offset &= 0x1ff;
			q = (WORD*)&gvram[offset << 10];

			// ��ʃo�C�g�N���A
			for (k=0; k<512; k++) {
				*q++ &= 0x00ff;
			}

			// �t���OUp
			render->GrpAll(offset, 2);
			render->GrpAll(offset, 3);
		}

		// ���̃��C����
		y++;
	}
}

//---------------------------------------------------------------------------
//
//	�����N���A 1024�~1024 256
//
//---------------------------------------------------------------------------
void FASTCALL GVRAM::FastClr256(const CRTC::crtc_t *p)
{
#if defined(GVRAM_LOG)
	LOG0(Log::Normal, "�����N���A 1024x1024 (256��)");
#endif	// GVRAM_LOG

	// �b��[�u
	FastClr768(p);
}

//---------------------------------------------------------------------------
//
//	�����N���A 512x512
//
//---------------------------------------------------------------------------
void FASTCALL GVRAM::FastClr512(const CRTC::crtc_t *p)
{
	int y;
	int n;
	int i;
	int j;
	int k;
	int w[2];
	DWORD offset;
	WORD *q;

	ASSERT(this);
	ASSERT_DIAG();

#if defined(GVRAM_LOG)
	LOG1(Log::Normal, "�����N���A 512x512 Scan=%d", p->v_scan);
#endif	// GVRAM_LOG

	// �I�t�Z�b�gy�A���C����n�𓾂�
	y = p->v_scan;
	n = 1;
	if ((p->v_mul == 2) && !(p->lowres)) {
		if (y & 1) {
			return;
		}
		y >>= 1;
	}
	if (p->v_mul == 0) {
		y <<= 1;
		n = 2;
	}

	// �v���[�����[�v
	for (i=0; i<4; i++) {
		if (!gvdata.plane[i]) {
			continue;
		}

		// �����Z�o
		w[0] = p->h_dots;
		w[1] = 0;
		if (((p->grp_scrlx[i] & 0x1ff) + w[0]) > 512) {
			w[1] = (p->grp_scrlx[i] & 0x1ff) + w[0] - 512;
			w[0] = 512 - (p->grp_scrlx[i] & 0x1ff);
		}

		// ���C�����[�v
		for (j=0; j<n; j++) {
			// �X�N���[�����W�X�^����I�t�Z�b�g�𓾂�
			offset = ((y + p->grp_scrly[i]) & 0x1ff) << 10;
			q = (WORD*)&gvram[offset + ((p->grp_scrlx[i] & 0x1ff) << 1)];

			// �N���A(1)
			for (k=0; k<w[0]; k++) {
				*q++ &= gvdata.mask[i];
			}
			if (w[1] > 0) {
				// �N���A(2)
				q = (WORD*)&gvram[offset];
				for (k=0; k<w[1]; k++) {
					*q++ &= gvdata.mask[i];
				}
			}

			// �t���OUp
			render->GrpAll(offset >> 10, i);

			// ���̃��C����
			y++;
		}

		// ���C���߂�
		y -= n;
	}
}

