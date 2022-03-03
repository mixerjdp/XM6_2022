//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2005 �o�h�D(ytanaka@ipc-tokai.or.jp)
//	[ FDC(uPD72065) ]
//
//---------------------------------------------------------------------------

#if !defined(fdc_h)
#define fdc_h

#include "device.h"
#include "event.h"

//===========================================================================
//
//	FDC
//
//===========================================================================
class FDC : public MemDevice
{
public:
	// �t�F�[�Y��`
	enum fdcphase {
		idle,							// �A�C�h���t�F�[�Y
		command,						// �R�}���h�t�F�[�Y
		execute,						// ���s�t�F�[�Y(�ʏ�)
		read,							// ���s�t�F�[�Y(Read)
		write,							// ���s�t�F�[�Y(Write)
		result							// ���U���g�t�F�[�Y
	};

	// �X�e�[�^�X���W�X�^��`
	enum {
		sr_rqm = 0x80,					// Request For Master
		sr_dio = 0x40,					// Data Input / Output
		sr_ndm = 0x20,					// Non-DMA Mode
		sr_cb = 0x10,					// FDC Busy
		sr_d3b = 0x08,					// Drive3 Seek
		sr_d2b = 0x04,					// Drive2 Seek
		sr_d1b = 0x02,					// Drive1 Seek
		sr_d0b = 0x01					// Drive0 Seek
	};

	// �R�}���h��`
	enum fdccmd {
		read_data,						// READ DATA
		read_del_data,					// READ DELETED DATA
		read_id,						// READ ID
		write_id,						// WRITE ID
		write_data,						// WRITE DATA
		write_del_data,					// WRITE DELETED DATA
		read_diag,						// READ DIAGNOSTIC
		scan_eq,						// SCAN EQUAL
		scan_lo_eq,						// SCAN LOW OR EQUAL
		scan_hi_eq, 					// SCAN HIGH OR EQUAL
		seek,							// SEEK
		recalibrate,					// RECALIBRATE
		sense_int_stat,					// SENSE INTERRUPT STATUS
		sense_dev_stat,					// SENSE DEVICE STATUS
		specify,						// SPECIFY
		set_stdby,						// SET STANDBY
		reset_stdby,					// RESET STANDBY
		fdc_reset,						// SOFTWARE RESET
		invalid,						// INVALID
		no_cmd							// (NO COMMAND)
	};

	// �������[�N��`
	typedef struct {
		fdcphase phase;					// �t�F�[�Y
		fdccmd cmd;						// �R�}���h

		int in_len;						// ���̓����O�X
		int in_cnt;						// ���̓J�E���g
		DWORD in_pkt[0x10];				// ���̓p�P�b�g
		int out_len;					// �o�̓����O�X
		int out_cnt;					// �o�̓J�E���g
		DWORD out_pkt[0x10];			// �o�̓p�P�b�g

		DWORD dcr;						// �h���C�u�R���g���[�����W�X�^
		DWORD dsr;						// �h���C�u�Z���N�g���W�X�^
		DWORD sr;						// �X�e�[�^�X���W�X�^
		DWORD dr;						// �f�[�^���W�X�^
		DWORD st[4];					// ST0-ST3

		DWORD srt;						// SRT
		DWORD hut;						// HUT
		DWORD hlt;						// HLT
		DWORD hd;						// HD
		DWORD us;						// US
		DWORD cyl[4];					// �����g���b�N
		DWORD chrn[4];					// �v�����ꂽC,H,R,N

		DWORD eot;						// EOT
		DWORD gsl;						// GSL
		DWORD dtl;						// DTL
		DWORD sc; 						// SC
		DWORD gpl;						// GAP3
		DWORD d;						// �t�H�[�}�b�g�f�[�^
		DWORD err;						// �G���[�R�[�h
		BOOL seek;						// �V�[�N�n���荞�ݗv��
		BOOL ndm;						// Non-DMA���[�h
		BOOL mfm;						// MFM���[�h
		BOOL mt;						// �}���`�g���b�N
		BOOL sk;						// Skip DDAM
		BOOL tc;						// TC
		BOOL load;						// �w�b�h���[�h

		int offset;						// �o�b�t�@�I�t�Z�b�g
		int len;						// �c�背���O�X
		BYTE buffer[0x4000];			// �f�[�^�o�b�t�@

		BOOL fast;						// �������[�h
		BOOL dual;						// �f���A���h���C�u
	} fdc_t;

public:
	// ��{�t�@���N�V����
	FDC(VM *p);
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
	BOOL FASTCALL Callback(Event *ev);
										// �C�x���g�R�[���o�b�N
	const fdc_t* FASTCALL GetWork() const;
										// �������[�N�A�h���X�擾
	void FASTCALL CompleteSeek(int drive, BOOL result);
										// �V�[�N����
	void FASTCALL SetTC();
										// TC�A�T�[�g

private:
	void FASTCALL Idle();
										// �A�C�h���t�F�[�Y
	void FASTCALL Command(DWORD data);
										// �R�}���h�t�F�[�Y
	void FASTCALL CommandRW(fdccmd cmd, DWORD data);
										// �R�}���h�t�F�[�Y(R/W�n)
	void FASTCALL Execute();
										// ���s�t�F�[�Y
	void FASTCALL ReadID();
										// ���s�t�F�[�Y(Read ID)
	void FASTCALL ExecuteRW();
										// ���s�t�F�[�Y(R/W�n)
	BYTE FASTCALL Read();
										// ���s�t�F�[�Y(Read)
	void FASTCALL Write(DWORD data);
										// ���s�t�F�[�Y(Write)
	void FASTCALL Compare(DWORD data);
										// ���s�t�F�[�Y(Compare)
	void FASTCALL Result();
										// ���U���g�t�F�[�Y
	void FASTCALL ResultRW();
										// ���U���g�t�F�[�Y(R/W�n)
	void FASTCALL Interrupt(BOOL flag);
										// ���荞��
	void FASTCALL SoftReset();
										// �\�t�g�E�F�A���Z�b�g
	void FASTCALL MakeST3();
										// ST3�쐬
	BOOL FASTCALL ReadData();
										// READ (DELETED) DATA�R�}���h
	BOOL FASTCALL WriteData();
										// WRITE (DELETED) DATA�R�}���h
	BOOL FASTCALL ReadDiag();
										// READ DIAGNOSTIC�R�}���h
	BOOL FASTCALL WriteID();
										// WRITE ID�R�}���h
	BOOL FASTCALL Scan();
										// SCAN�n�R�}���h
	void FASTCALL EventRW();
										// �C�x���g����(R/W)
	void FASTCALL EventErr(DWORD hus);
										// �C�x���g����(�G���[)
	void FASTCALL WriteBack();
										// �������݊���
	BOOL FASTCALL NextSector();
										// �}���`�Z�N�^����
	IOSC *iosc;
										// IOSC
	DMAC *dmac;
										// DMAC
	FDD *fdd;
										// FDD
	Event event;
										// �C�x���g
	fdc_t fdc;
										// FDC�����f�[�^
};

#endif	// fdc_h
