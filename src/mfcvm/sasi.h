//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 �o�h�D(ytanaka@ipc-tokai.or.jp)
//	[ SASI ]
//
//---------------------------------------------------------------------------

#if !defined(sasi_h)
#define sasi_h

#include "device.h"
#include "event.h"
#include "disk.h"

//===========================================================================
//
//	SASI
//
//===========================================================================
class SASI : public MemDevice
{
public:
	// �ő吔
	enum {
		SASIMax = 16,					// �ő�SASI�f�B�X�N��(LUN�܂�)
		SCSIMax = 6						// �ő�SCSI�n�[�h�f�B�X�N��
	};

	// �t�F�[�Y��`
	enum phase_t {
		busfree,						// �o�X�t���[�t�F�[�Y
		selection,						// �Z���N�V�����t�F�[�Y
		command,						// �R�}���h�t�F�[�Y
		execute,						// ���s�t�F�[�Y
		read,							// �ǂݍ��݃t�F�[�Y
		write,							// �������݃t�F�[�Y
		status,							// �X�e�[�^�X�t�F�[�Y
		message							// ���b�Z�[�W�t�F�[�Y
	};

	// �����f�[�^��`
	typedef struct {
		// SASI�R���g���[��
		phase_t phase;					// �t�F�[�Y
		BOOL sel;						// Select�M��
		BOOL msg;						// Message�M��
		BOOL cd;						// Command/Data�M��
		BOOL io;						// Input/Output�M��
		BOOL bsy;						// Busy�M��
		BOOL req;						// Request�M��
		DWORD ctrl;						// �Z���N�g���ꂽ�R���g���[��
		DWORD cmd[10];					// �R�}���h�f�[�^
		DWORD status;					// �X�e�[�^�X�f�[�^
		DWORD message;					// ���b�Z�[�W�f�[�^
		BYTE buffer[0x800];				// �]���o�b�t�@
		DWORD blocks;					// �]���u���b�N��
		DWORD next;						// ���̃��R�[�h
		DWORD offset;					// �]���I�t�Z�b�g
		DWORD length;					// �]���c�蒷��

		// �f�B�X�N
		Disk *disk[SASIMax];			// �f�B�X�N
		Disk *current;					// �f�B�X�N(�J�����g)
		SCSIMO *mo;						// �f�B�X�N(MO)

		// �R���t�B�O
		int sasi_drives;				// �h���C�u��(SASI)
		BOOL memsw;						// �������X�C�b�`�X�V
		BOOL parity;					// �p���e�B�t��
		int sxsi_drives;				// �h���C�u��(SxSI)
		BOOL mo_first;					// MO�D��t���O(SxSI)
		int scsi_type;					// SCSI�^�C�v

		// MO�p�����[�^
		BOOL writep;					// MO�������݋֎~�t���O
	} sasi_t;

public:
	// ��{�t�@���N�V����
	SASI(VM *p);
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

	// MO�A�N�Z�X
	BOOL FASTCALL Open(const Filepath& path);
										// MO �I�[�v��
	void FASTCALL Eject(BOOL force);
										// MO �C�W�F�N�g
	void FASTCALL WriteP(BOOL writep);
										// MO �������݋֎~
	BOOL FASTCALL IsWriteP() const;
										// MO �������݋֎~�`�F�b�N
	BOOL FASTCALL IsReadOnly() const;
										// MO ReadOnly�`�F�b�N
	BOOL FASTCALL IsLocked() const;
										// MO Lock�`�F�b�N
	BOOL FASTCALL IsReady() const;
										// MO Ready�`�F�b�N
	BOOL FASTCALL IsValid() const;
										// MO �L���`�F�b�N
	void FASTCALL GetPath(Filepath &path) const;
										// MO �p�X�擾

	// �O��API
	void FASTCALL GetSASI(sasi_t *buffer) const;
										// �����f�[�^�擾
	BOOL FASTCALL Callback(Event *ev);
										// �C�x���g�R�[���o�b�N
	void FASTCALL Construct();
										// �f�B�X�N�č\�z
	BOOL FASTCALL IsBusy() const;
										// HD BUSY�擾
	DWORD FASTCALL GetBusyDevice() const;
										// BUSY�f�o�C�X�擾

private:
	DWORD FASTCALL ReadData();
										// �f�[�^�ǂݏo��
	void FASTCALL WriteData(DWORD data);
										// �f�[�^��������

	// �t�F�[�Y����
	void FASTCALL BusFree();
										// �o�X�t���[�t�F�[�Y
	void FASTCALL Selection(DWORD data);
										// �Z���N�V�����t�F�[�Y
	void FASTCALL Command();
										// �R�}���h�t�F�[�Y
	void FASTCALL Execute();
										// ���s�t�F�[�Y
	void FASTCALL Status();
										// �X�e�[�^�X�t�F�[�Y
	void FASTCALL Message();
										// ���b�Z�[�W�t�F�[�Y
	void FASTCALL Error();
										// ���ʃG���[����

	// �R�}���h
	void FASTCALL TestUnitReady();
										// TEST UNIT READY�R�}���h
	void FASTCALL Rezero();
										// REZERO UNIT�R�}���h
	void FASTCALL RequestSense();
										// REQUEST SENSE�R�}���h
	void FASTCALL Format();
										// FORMAT�R�}���h
	void FASTCALL Reassign();
										// REASSIGN BLOCKS�R�}���h
	void FASTCALL Read6();
										// READ(6)�R�}���h
	void FASTCALL Write6();
										// WRITE(6)�R�}���h
	void FASTCALL Seek6();
										// SEEK(6)�R�}���h
	void FASTCALL Assign();
										// ASSIGN�R�}���h
	void FASTCALL Inquiry();
										// INQUIRY�R�}���h
	void FASTCALL ModeSense();
										// MODE SENSE�R�}���h
	void FASTCALL StartStop();
										// START STOP UNIT�R�}���h
	void FASTCALL Removal();
										// PREVENT/ALLOW MEDIUM REMOVAL�R�}���h
	void FASTCALL ReadCapacity();
										// READ CAPACITY�R�}���h
	void FASTCALL Read10();
										// READ(10)�R�}���h
	void FASTCALL Write10();
										// WRITE(10)�R�}���h
	void FASTCALL Seek10();
										// SEEK(10)�R�}���h
	void FASTCALL Verify();
										// VERIFY�R�}���h
	void FASTCALL Specify();
										// SPECIFY�R�}���h

	// ���[�N�G���A
	sasi_t sasi;
										// �����f�[�^
	Event event;
										// �C�x���g
	Filepath sasihd[SASIMax];
										// SASI-HD�t�@�C���p�X
	Filepath scsihd[SCSIMax];
										// SCSI-HD�t�@�C���p�X
	Filepath scsimo;
										// SCSI-MO�t�@�C���p�X
	DMAC *dmac;
										// DMAC
	IOSC *iosc;
										// IOSC
	SRAM *sram;
										// SRAM
	SCSI *scsi;
										// SCSI
	BOOL sxsicpu;
										// SxSI CPU�]���t���O
};

#endif	// sasi_h
