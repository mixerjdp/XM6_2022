//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2003 �o�h�D(ytanaka@ipc-tokai.or.jp)
//	[ �G���A�Z�b�g ]
//
//---------------------------------------------------------------------------

#if !defined(areaset_h)
#define areaset_h

#include "device.h"

//===========================================================================
//
//	�G���A�Z�b�g
//
//===========================================================================
class AreaSet : public MemDevice
{
public:
	// ��{�t�@���N�V����
	AreaSet(VM *p);
										// �R���X�g���N�^
	BOOL FASTCALL Init();
										// ������
	void FASTCALL Cleanup();
										// �N���[���A�b�v
	void FASTCALL Reset();
										// ���Z�b�g
	BOOL FASTCALL Save(Fileio *fio, int ver);
										// �Z�[�u
	BOOL FASTCALL Load(Fileio *fio, int ver);
										// ���[�h
	void FASTCALL ApplyCfg(const Config *config);
										// �ݒ�K�p

	// �������f�o�C�X
	DWORD FASTCALL ReadByte(DWORD addr);
										// �o�C�g�ǂݍ���
	void FASTCALL WriteByte(DWORD addr, DWORD data);
										// �o�C�g��������
	DWORD FASTCALL ReadOnly(DWORD addr) const;
										// �ǂݍ��݂̂�

	// �O��API
	DWORD FASTCALL GetArea() const;
										// �ݒ�l�擾

private:
	Memory *memory;
										// ������
	DWORD area;
										// �G���A�Z�b�g���W�X�^
};

#endif	// areaset_h
