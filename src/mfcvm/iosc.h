//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2004 �o�h�D(ytanaka@ipc-tokai.or.jp)
//	[ I/O�R���g���[��(IOSC-2) ]
//
//---------------------------------------------------------------------------

#if !defined(iosc_h)
#define iosc_h

#include "device.h"

//===========================================================================
//
//	I/O�R���g���[��
//
//===========================================================================
class IOSC : public MemDevice
{
public:
	// �����f�[�^��`
	typedef struct {
		BOOL prt_int;					// �v�����^���荞�ݗv��
		BOOL prt_en;					// �v�����^���荞�݋���
		BOOL fdd_int;					// FDD���荞�ݗv��
		BOOL fdd_en;					// FDD���荞�݋���
		BOOL fdc_int;					// FDC���荞�ݗv��
		BOOL fdc_en;					// FDC���荞�݋���
		BOOL hdc_int;					// HDD���荞�ݗv��
		BOOL hdc_en;					// HDD���荞�݋���
		DWORD vbase;					// �x�N�^�x�[�X
		int vector;						// �v�����̊��荞�݃x�N�^
	} iosc_t;

public:
	// ��{�t�@���N�V����
	IOSC(VM *p);
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
	DWORD FASTCALL ReadWord(DWORD addr);
										// ���[�h�ǂݍ���
	void FASTCALL WriteByte(DWORD addr, DWORD data);
										// �o�C�g��������
	void FASTCALL WriteWord(DWORD addr, DWORD data);
										// ���[�h��������
	DWORD FASTCALL ReadOnly(DWORD addr) const;
										// �ǂݍ��݂̂�

	// �O��API
	void FASTCALL GetIOSC(iosc_t *buffer) const;
										// �����f�[�^�擾
	DWORD FASTCALL GetVector() const	{ return iosc.vbase; }
										// �x�N�^�x�[�X�擾
	void FASTCALL IntAck();
										// ���荞�݉���
	void FASTCALL IntFDC(BOOL flag);
										// FDC���荞��
	void FASTCALL IntFDD(BOOL flag);
										// FDD���荞��
	void FASTCALL IntHDC(BOOL flag);
										// HDC���荞��
	void FASTCALL IntPRT(BOOL flag);
										// �v�����^���荞��

private:
	void FASTCALL IntChk();
										// ���荞�݃`�F�b�N
	iosc_t iosc;
										// �����f�[�^
	Printer *printer;
										// �v�����^
};

#endif	// iosc_h
