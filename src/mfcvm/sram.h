//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 �o�h�D(ytanaka@ipc-tokai.or.jp)
//	[ �X�^�e�B�b�NRAM ]
//
//---------------------------------------------------------------------------

#if !defined(sram_h)
#define sram_h

#include "device.h"
#include "filepath.h"

//===========================================================================
//
//	�X�^�e�B�b�NRAM
//
//===========================================================================
class SRAM : public MemDevice
{
public:
	// ��{�t�@���N�V����
	SRAM(VM *p);
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
#if !defined(NDEBUG)
	void FASTCALL AssertDiag() const;
										// �f�f
#endif	// NDEBUG

	// �������f�o�C�X
	DWORD FASTCALL ReadByte(DWORD addr);
										// �o�C�g�ǂݍ���
	DWORD FASTCALL ReadWord(DWORD addr);
										// ���[�h�ǂݍ���
	void FASTCALL WriteByte(DWORD addr, DWORD data);
										// �o�C�g��������
	void FASTCALL WriteWord(DWORD addr, DWORD data);
										// ���[�h��������
	DWORD FASTCALL ReadOnly(DWORD addr) const;
										// �ǂݍ��݂̂�

	// �O��API
	const BYTE* FASTCALL GetSRAM() const;
										// SRAM�G���A�擾
	int FASTCALL GetSize() const;
										// SRAM�T�C�Y�擾
	void FASTCALL WriteEnable(BOOL enable);
										// �������݋���
	void FASTCALL SetMemSw(DWORD offset, DWORD data);
										// �������X�C�b�`�Z�b�g
	DWORD FASTCALL GetMemSw(DWORD offset) const;
										// �������X�C�b�`�擾
	void FASTCALL UpdateBoot();
										// �N���J�E���^�X�V

private:
	Filepath sram_path;
										// SRAM�t�@�C���p�X
	int sram_size;
										// SRAM�T�C�Y(16,32,48,64)
	BYTE sram[0x10000];
										// SRAM (64KB)
	BOOL write_en;
										// �������݋��t���O
	BOOL mem_sync;
										// ���C��RAM�T�C�Y�����t���O
	BOOL changed;
										// �ύX�t���O
};

#endif	// sram_h
