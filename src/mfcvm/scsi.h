//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 �o�h�D(ytanaka@ipc-tokai.or.jp)
//	[ SCSI(MB89352) ]
//
//---------------------------------------------------------------------------

#if !defined(scsi_h)
#define scsi_h

#include "device.h"
#include "event.h"
#include "disk.h"

//===========================================================================
//
//	SCSI
//
//===========================================================================
class SCSI : public MemDevice
{
public:
	// �ő吔
	enum {
		DeviceMax = 8,					// �ő�SCSI�f�o�C�X��
		HDMax = 5						// �ő�SCSI HD��
	};

	// �t�F�[�Y��`
	enum phase_t {
		busfree,						// �o�X�t���[�t�F�[�Y
		arbitration,					// �A�[�r�g���[�V�����t�F�[�Y
		selection,						// �Z���N�V�����t�F�[�Y
		reselection,					// ���Z���N�V�����t�F�[�Y
		command,						// �R�}���h�t�F�[�Y
		execute,						// ���s�t�F�[�Y
		msgin,							// ���b�Z�[�W�C���t�F�[�Y
		msgout,							// ���b�Z�[�W�A�E�g�t�F�[�Y
		datain,							// �f�[�^�C���t�F�[�Y
		dataout,						// �f�[�^�A�E�g�t�F�[�Y
		status							// �X�e�[�^�X�t�F�[�Y
	};

	// �����f�[�^��`
	typedef struct {
		// �S��
		int type;						// SCSI�^�C�v(0:�Ȃ� 1:�O�t 2:����)
		phase_t phase;					// �t�F�[�Y
		int id;							// �J�����gID(0-7)

		// ���荞��
		int vector;						// �v���x�N�^(-1�ŗv���Ȃ�)
		int ilevel;						// ���荞�݃��x��

		// �M��
		BOOL bsy;						// Busy�M��
		BOOL sel;						// Select�M��
		BOOL atn;						// Attention�M��
		BOOL msg;						// Message�M��
		BOOL cd;						// Command/Data�M��
		BOOL io;						// Input/Output�M��
		BOOL req;						// Request�M��
		BOOL ack;						// Ack�M��
		BOOL rst;						// Reset�M��

		// ���W�X�^
		DWORD bdid;						// BDID���W�X�^(�r�b�g�\��)
		DWORD sctl;						// SCTL���W�X�^
		DWORD scmd;						// SCMD���W�X�^
		DWORD ints;						// INTS���W�X�^
		DWORD sdgc;						// SDGC���W�X�^
		DWORD pctl;						// PCTL���W�X�^
		DWORD mbc;						// MBC���W�X�^
		DWORD temp;						// TEMP���W�X�^
		DWORD tc;						// TCH,TCM,TCL���W�X�^

		// �R�}���h
		DWORD cmd[10];					// �R�}���h�f�[�^
		DWORD status;					// �X�e�[�^�X�f�[�^
		DWORD message;					// ���b�Z�[�W�f�[�^

		// �]��
		BOOL trans;						// �]���t���O
		BYTE buffer[0x800];				// �]���o�b�t�@
		DWORD blocks;					// �]���u���b�N��
		DWORD next;						// ���̃��R�[�h
		DWORD offset;					// �]���I�t�Z�b�g
		DWORD length;					// �]���c�蒷��

		// �R���t�B�O
		int scsi_drives;				// SCSI�h���C�u��
		BOOL memsw;						// �������X�C�b�`�X�V
		BOOL mo_first;					// MO�D��t���O(SxSI)

		// �f�B�X�N
		Disk *disk[DeviceMax];			// �f�o�C�X
		Disk *hd[HDMax];				// HD
		SCSIMO *mo;						// MO
		SCSICD *cdrom;					// CD-ROM
	} scsi_t;

public:
	// ��{�t�@���N�V����
	SCSI(VM *p);
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
	void FASTCALL GetSCSI(scsi_t *buffer) const;
										// �����f�[�^�擾
	BOOL FASTCALL Callback(Event *ev);
										// �C�x���g�R�[���o�b�N
	void FASTCALL IntAck(int level);
										// ���荞��ACK
	int FASTCALL GetSCSIID() const;
										// SCSI-ID�擾
	BOOL FASTCALL IsBusy() const;
										// BUSY��
	DWORD FASTCALL GetBusyDevice() const;
										// BUSY�f�o�C�X�擾

	// MO/CD�A�N�Z�X
	BOOL FASTCALL Open(const Filepath& path, BOOL mo = TRUE);
										// MO/CD �I�[�v��
	void FASTCALL Eject(BOOL force, BOOL mo = TRUE);
										// MO/CD �C�W�F�N�g
	void FASTCALL WriteP(BOOL writep);
										// MO �������݋֎~
	BOOL FASTCALL IsWriteP() const;
										// MO �������݋֎~�`�F�b�N
	BOOL FASTCALL IsReadOnly() const;
										// MO ReadOnly�`�F�b�N
	BOOL FASTCALL IsLocked(BOOL mo = TRUE) const;
										// MO/CD Lock�`�F�b�N
	BOOL FASTCALL IsReady(BOOL mo = TRUE) const;
										// MO/CD Ready�`�F�b�N
	BOOL FASTCALL IsValid(BOOL mo = TRUE) const;
										// MO/CD �L���`�F�b�N
	void FASTCALL GetPath(Filepath &path, BOOL mo = TRUE) const;
										// MO/CD �p�X�擾

	// CD-DA
	void FASTCALL GetBuf(DWORD *buffer, int samples, DWORD rate);
										// CD-DA�o�b�t�@�擾

private:
	// ���W�X�^
	void FASTCALL ResetReg();
										// ���W�X�^���Z�b�g
	void FASTCALL ResetCtrl();
										// �]�����Z�b�g
	void FASTCALL ResetBus(BOOL reset);
										// �o�X���Z�b�g
	void FASTCALL SetBDID(DWORD data);
										// BDID�ݒ�
	void FASTCALL SetSCTL(DWORD data);
										// SCTL�ݒ�
	void FASTCALL SetSCMD(DWORD data);
										// SCMD�ݒ�
	void FASTCALL SetINTS(DWORD data);
										// INTS�ݒ�
	DWORD FASTCALL GetPSNS() const;
										// PSNS�擾
	void FASTCALL SetSDGC(DWORD data);
										// SDGC�ݒ�
	DWORD FASTCALL GetSSTS() const;
										// SSTS�擾
	DWORD FASTCALL GetSERR() const;
										// SERR�擾
	void FASTCALL SetPCTL(DWORD data);
										// PCTL�ݒ�
	DWORD FASTCALL GetDREG();
										// DREG�擾
	void FASTCALL SetDREG(DWORD data);
										// DREG�ݒ�
	void FASTCALL SetTEMP(DWORD data);
										// TEMP�ݒ�
	void FASTCALL SetTCH(DWORD data);
										// TCH�ݒ�
	void FASTCALL SetTCM(DWORD data);
										// TCM�ݒ�
	void FASTCALL SetTCL(DWORD data);
										// TCL�ݒ�

	// SPC�R�}���h
	void FASTCALL BusRelease();
										// �o�X�����[�X
	void FASTCALL Select();
										// �Z���N�V����/���Z���N�V����
	void FASTCALL ResetATN();
										// ATN���C��=0
	void FASTCALL SetATN();
										// ATN���C��=1
	void FASTCALL Transfer();
										// �]��
	void FASTCALL TransPause();
										// �]�����f
	void FASTCALL TransComplete();
										// �]������
	void FASTCALL ResetACKREQ();
										// ACK/REQ���C��=0
	void FASTCALL SetACKREQ();
										// ACK/REQ���C��=1

	// �f�[�^�]��
	void FASTCALL Xfer(DWORD *reg);
										// �f�[�^�]��
	void FASTCALL XferNext();
										// �f�[�^�]���p��
	BOOL FASTCALL XferIn();
										// �f�[�^�]��IN
	BOOL FASTCALL XferOut(BOOL cont);
										// �f�[�^�]��OUT
	BOOL FASTCALL XferMsg(DWORD msg);
										// �f�[�^�]��MSG

	// �t�F�[�Y
	void FASTCALL BusFree();
										// �o�X�t���[�t�F�[�Y
	void FASTCALL Arbitration();
										// �A�[�r�g���[�V�����t�F�[�Y
	void FASTCALL Selection();
										// �Z���N�V�����t�F�[�Y
	void FASTCALL SelectTime();
										// �Z���N�V�����t�F�[�Y(���Ԑݒ�)
	void FASTCALL Command();
										// �R�}���h�t�F�[�Y
	void FASTCALL Execute();
										// ���s�t�F�[�Y
	void FASTCALL MsgIn();
										// ���b�Z�[�W�C���t�F�[�Y
	void FASTCALL MsgOut();
										// ���b�Z�[�W�A�E�g�t�F�[�Y
	void FASTCALL DataIn();
										// �f�[�^�C���t�F�[�Y
	void FASTCALL DataOut();
										// �f�[�^�A�E�g�t�F�[�Y
	void FASTCALL Status();
										// �X�e�[�^�X�t�F�[�Y

	// ���荞��
	void FASTCALL Interrupt(int type, BOOL flag);
										// ���荞�ݗv��
	void FASTCALL IntCheck();
										// ���荞�݃`�F�b�N

	// SCSI�R�}���h����
	void FASTCALL Error();
										// ���ʃG���[

	// SCSI�R�}���h��
	void FASTCALL TestUnitReady();
										// TEST UNIT READY�R�}���h
	void FASTCALL Rezero();
										// REZERO UNIT�R�}���h
	void FASTCALL RequestSense();
										// REQUEST SENSE�R�}���h
	void FASTCALL Format();
										// FORMAT UNIT�R�}���h
	void FASTCALL Reassign();
										// REASSIGN BLOCKS�R�}���h
	void FASTCALL Read6();
										// READ(6)�R�}���h
	void FASTCALL Write6();
										// WRITE(6)�R�}���h
	void FASTCALL Seek6();
										// SEEK(6)�R�}���h
	void FASTCALL Inquiry();
										// INQUIRY�R�}���h
	void FASTCALL ModeSelect();
										// MODE SELECT�R�}���h
	void FASTCALL ModeSense();
										// MODE SENSE�R�}���h
	void FASTCALL StartStop();
										// START STOP UNIT�R�}���h
	void FASTCALL SendDiag();
										// SEND DIAGNOSTIC�R�}���h
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
	void FASTCALL ReadToc();
										// READ TOC�R�}���h
	void FASTCALL PlayAudio10();
										// PLAY AUDIO(10)�R�}���h
	void FASTCALL PlayAudioMSF();
										// PLAY AUDIO MSF�R�}���h
	void FASTCALL PlayAudioTrack();
										// PLAY AUDIO TRACK INDEX�R�}���h

	// CD-ROM�ECD-DA
	Event cdda;
										// �t���[���C�x���g

	// �h���C�u�E�t�@�C���p�X
	void FASTCALL Construct();
										// �h���C�u�\�z
	Filepath scsihd[DeviceMax];
										// SCSI-HD�t�@�C���p�X

	// ���̑�
	scsi_t scsi;
										// �����f�[�^
	BYTE verifybuf[0x800];
										// �x���t�@�C�o�b�t�@
	Event event;
										// �C�x���g
	Memory *memory;
										// ������
	SRAM *sram;
										// SRAM
};

#endif	// scsi_h
