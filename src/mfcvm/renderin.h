//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001,2002 �o�h�D(ytanaka@ipc-tokai.or.jp)
//	[ �����_��(�C�����C��) ]
//
//---------------------------------------------------------------------------

#if !defined(renderin_h)
#define renderin_h

#include "render.h"

//---------------------------------------------------------------------------
//
//	�p���b�g�ݒ�
//
//---------------------------------------------------------------------------
inline void FASTCALL Render::SetPalette(int index)
{
	// VC�̎��_�Ŕ�r�`�F�b�N���s��
	render.palmod[index] = TRUE;
	render.palette = TRUE;
}

//---------------------------------------------------------------------------
//
//	�e�L�X�gVRAM�ύX
//
//---------------------------------------------------------------------------
inline void FASTCALL Render::TextMem(DWORD addr)
{
	// �e�L�X�gVRAM�̎��_�Ŕ�r�`�F�b�N���s��
	addr &= 0x1ffff;
	addr >>= 2;
	render.textflag[addr] = TRUE;
	addr >>= 5;
	render.textmod[addr] = TRUE;
}

//---------------------------------------------------------------------------
//
//	�O���t�B�b�NVRAM�ύX
//
//---------------------------------------------------------------------------
inline void FASTCALL Render::GrpMem(DWORD addr, DWORD block)
{
	// �O���t�B�b�NVRAM�̎��_�Ŕ�r�`�F�b�N���s��
	ASSERT(addr <= 0x7ffff);
	ASSERT(block <= 3);

	// 16dot�t���O(16dot�P�ʁA(512/16)��512��4 = 0x10000)
	addr >>= 5;
	block <<= 14;
	render.grpflag[addr | block] = TRUE;
	// ���C���t���O(���C���P�ʁA512x4 = 2048)
	addr >>= 5;
	block >>= 5;
	render.grpmod[addr | block] = TRUE;
}

//---------------------------------------------------------------------------
//
//	�O���t�B�b�NVRAM�ύX(�S��)
//
//---------------------------------------------------------------------------
inline void FASTCALL Render::GrpAll(DWORD line, DWORD block)
{
	ASSERT(line <= 0x1ff);
	ASSERT(block <= 3);

	render.grppal[(block << 9) | line] = TRUE;
}

#endif	// renderin_h
