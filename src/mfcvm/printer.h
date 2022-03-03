//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2004 �o�h�D(ytanaka@ipc-tokai.or.jp)
//	[ �v�����^ ]
//
//---------------------------------------------------------------------------

#if !defined(printer_h)
#define printer_h

#include "device.h"

//===========================================================================
//
//	�v�����^
//
//===========================================================================
class Printer : public MemDevice
{
public:
	// �萔�l
	enum {
		BufMax = 0x1000				// �o�b�t�@�T�C�Y(2�̔{��)
	};

	// �����f�[�^��`
	typedef struct {
		BOOL connect;					// �ڑ�
		BOOL strobe;					// �X�g���[�u
		BOOL ready;						// ���f�B
		BYTE data;						// �������݃f�[�^
		BYTE buf[BufMax];				// �o�b�t�@�f�[�^
		DWORD read;						// �o�b�t�@�ǂݍ��݈ʒu
		DWORD write;					// �o�b�t�@�������݈ʒu
		DWORD num;						// �o�b�t�@�L����
	} printer_t;

public:
	// ��{�t�@���N�V����
	Printer(VM *p);
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
	BOOL FASTCALL IsReady() const		{ return printer.ready; }
										// ���f�B�擾
	void FASTCALL HSync();
										// H-Sync�ʒm
	void FASTCALL GetPrinter(printer_t *buffer) const;
										// �����f�[�^�擾
	void FASTCALL Connect(BOOL flag);
										// �v�����^�ڑ�
	BOOL FASTCALL GetData(BYTE *ptr);
										// �擪�f�[�^�擾

private:
	IOSC *iosc;
										// IOSC
	Sync *sync;
										// �����I�u�W�F�N�g
	printer_t printer;
										// �����f�[�^
};

#endif	// printer_h
