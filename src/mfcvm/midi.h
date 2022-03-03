//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2005 �o�h�D(ytanaka@ipc-tokai.or.jp)
//	[ MIDI(YM3802) ]
//
//---------------------------------------------------------------------------

#if !defined(midi_h)
#define midi_h

#include "device.h"
#include "event.h"

//===========================================================================
//
//	MIDI
//
//===========================================================================
class MIDI : public MemDevice
{
public:
	// �萔��`
	enum {
		TransMax = 0x2000,				// ���M�o�b�t�@��
		RecvMax = 0x2000				// ��M�o�b�t�@��
	};

	// MIDI�o�C�g�f�[�^��`
	typedef struct {
		DWORD data;						// �f�[�^����(8bit)
		DWORD vtime;					// ���z����
	} mididata_t;

	// �����f�[�^��`
	typedef struct {
		// ���Z�b�g
		BOOL reset;						// ���Z�b�g�t���O
		BOOL access;					// �A�N�Z�X�t���O

		// �{�[�h�f�[�^�A���荞��
		DWORD bid;						// �{�[�hID(0:�{�[�h����)
		DWORD ilevel;					// ���荞�݃��x��
		int vector;						// ���荞�ݗv���x�N�^

		// MCS���W�X�^(���)
		DWORD wdr;						// �������݃f�[�^���W�X�^
		DWORD rgr;						// ���W�X�^�O���[�v���W�X�^

		// MCS���W�X�^(���荞��)
		DWORD ivr;						// ���荞�݃x�N�^���W�X�^
		DWORD isr;						// ���荞�݃T�[�r�X���W�X�^
		DWORD imr;						// ���荞�݃��[�h���W�X�^
		DWORD ier;						// ���荞�݋����W�X�^

		// MCS���W�X�^(���A���^�C�����b�Z�[�W)
		DWORD dmr;						// ���A���^�C�����b�Z�[�W���[�h���W�X�^
		DWORD dcr;						// ���A���^�C�����b�Z�[�W�R���g���[�����W�X�^

		// MCS���W�X�^(��M)
		DWORD rrr;						// ��M���[�g���W�X�^
		DWORD rmr;						// ��M���[�h���W�X�^
		DWORD amr;						// �A�h���X�n���^���[�h���W�X�^
		DWORD adr;						// �A�h���X�n���^�f�o�C�X���W�X�^
		DWORD asr;						// �A�h���X�n���^�X�e�[�^�X���W�X�^
		DWORD rsr;						// ��M�o�b�t�@�X�e�[�^�X���W�X�^
		DWORD rcr;						// ��M�o�b�t�@�R���g���[�����W�X�^
		DWORD rcn;						// ����M�J�E���^

		// MCS���W�X�^(���M)
		DWORD trr;						// ���M���[�g���W�X�^
		DWORD tmr;						// ���M���[�h���W�X�^
		BOOL tbs;						// ���MBUSY���W�X�^
		DWORD tcr;						// ���M�R���g���[�����W�X�^
		DWORD tcn;						// �����M�J�E���^

		// MCS���W�X�^(FSK)
		DWORD fsr;						// FSK�X�e�[�^�X���W�X�^
		DWORD fcr;						// FSK�R���g���[�����W�X�^

		// MCS���W�X�^(�J�E���^)
		DWORD ccr;						// �N���b�N�R���g���[�����W�X�^
		DWORD cdr;						// �N���b�N�f�[�^���W�X�^
		DWORD ctr;						// �N���b�N�^�C�}���W�X�^
		DWORD srr;						// ���R�[�f�B���O�J�E���^���W�X�^
		DWORD scr;						// �N���b�N��ԃ��W�X�^
		DWORD sct;						// �N���b�N��ԃJ�E���^
		DWORD spr;						// �v���C�o�b�N�J�E���^���W�X�^
		DWORD str;						// �v���C�o�b�N�^�C�}���W�X�^
		DWORD gtr;						// �ėp�^�C�}���W�X�^
		DWORD mtr;						// MIDI�N���b�N�^�C�}���W�X�^

		// MCS���W�X�^(GPIO)
		DWORD edr;						// �O���|�[�g�f�B���N�V�������W�X�^
		DWORD eor;						// �O���|�[�gOutput���W�X�^
		DWORD eir;						// �O���|�[�gInput���W�X�^

		// �ʏ�o�b�t�@
		DWORD normbuf[16];				// �ʏ�o�b�t�@
		DWORD normread;					// �ʏ�o�b�t�@Read
		DWORD normwrite;				// �ʏ�o�b�t�@Write
		DWORD normnum;					// �ʏ�o�b�t�@��
		DWORD normtotal;				// �ʏ�o�b�t�@�g�[�^��

		// ���A���^�C�����M�o�b�t�@
		DWORD rtbuf[4];					// ���A���^�C�����M�o�b�t�@
		DWORD rtread;					// ���A���^�C�����M�o�b�t�@Read
		DWORD rtwrite;					// ���A���^�C�����M�o�b�t�@Write
		DWORD rtnum;					// ���A���^�C�����M�o�b�t�@��
		DWORD rttotal;					// ���A���^�C�����M�o�b�t�@�g�[�^��

		// ��ʃo�b�t�@
		DWORD stdbuf[0x80];				// ��ʃo�b�t�@
		DWORD stdread;					// ��ʃo�b�t�@Read
		DWORD stdwrite;					// ��ʃo�b�t�@Write
		DWORD stdnum;					// ��ʃo�b�t�@��
		DWORD stdtotal;					// ��ʃo�b�t�@�g�[�^��

		// ���A���^�C����M�o�b�t�@
		DWORD rrbuf[4];					// ���A���^�C����M�o�b�t�@
		DWORD rrread;					// ���A���^�C����M�o�b�t�@Read
		DWORD rrwrite;					// ���A���^�C����M�o�b�t�@Write
		DWORD rrnum;					// ���A���^�C����M�o�b�t�@��
		DWORD rrtotal;					// ���A���^�C����M�o�b�t�@�g�[�^��

		// ���M�o�b�t�@(�f�o�C�X�Ƃ̎󂯓n���p)
		mididata_t *transbuf;			// ���M�o�b�t�@
		DWORD transread;				// ���M�o�b�t�@Read
		DWORD transwrite;				// ���M�o�b�t�@Write
		DWORD transnum;					// ���M�o�b�t�@��

		// ��M�o�b�t�@(�f�o�C�X�Ƃ̎󂯓n���p)
		mididata_t *recvbuf;			// ��M�o�b�t�@
		DWORD recvread;					// ��M�o�b�t�@Read
		DWORD recvwrite;				// ��M�o�b�t�@Write
		DWORD recvnum;					// ��M�o�b�t�@��
	} midi_t;

public:
	// ��{�t�@���N�V����
	MIDI(VM *p);
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
	BOOL FASTCALL IsActive() const;
										// MIDI�A�N�e�B�u�`�F�b�N
	BOOL FASTCALL Callback(Event *ev);
										// �C�x���g�R�[���o�b�N
	void FASTCALL IntAck(int level);
										// ���荞��ACK
	void FASTCALL GetMIDI(midi_t *buffer) const;
										// �����f�[�^�擾
	DWORD FASTCALL GetExCount(int index) const;
										// �G�N�X�N���[�V�u�J�E���g�擾

	// ���M(MIDI OUT)
	DWORD FASTCALL GetTransNum() const;
										// ���M�o�b�t�@���擾
	const mididata_t* FASTCALL GetTransData(DWORD proceed);
										// ���M�o�b�t�@�f�[�^�擾
	void FASTCALL DelTransData(DWORD number);
										// ���M�o�b�t�@�폜
	void FASTCALL ClrTransData();
										// ���M�o�b�t�@�N���A

	// ��M(MIDI IN)
	void FASTCALL SetRecvData(const BYTE *ptr, DWORD length);
										// ��M�f�[�^�ݒ�
	void FASTCALL SetRecvDelay(int delay);
										// ��M�f�B���C�ݒ�

	// ���Z�b�g
	BOOL FASTCALL IsReset() const		{ return midi.reset; }
										// ���Z�b�g�t���O�擾
	void FASTCALL ClrReset()			{ midi.reset = FALSE; }
										// ���Z�b�g�t���O�N���A

private:
	void FASTCALL Receive();
										// ��M�R�[���o�b�N
	void FASTCALL Transmit();
										// ���M�R�[���o�b�N
	void FASTCALL Clock();
										// MIDI�N���b�N���o
	void FASTCALL General();
										// �ėp�^�C�}�R�[���o�b�N

	void FASTCALL InsertTrans(DWORD data);
										// ���M�o�b�t�@�֑}��
	void FASTCALL InsertRecv(DWORD data);
										// ��M�o�b�t�@�֑}��
	void FASTCALL InsertNorm(DWORD data);
										// �ʏ�o�b�t�@�֑}��
	void FASTCALL InsertRT(DWORD data);
										// ���A���^�C�����M�o�b�t�@�֑}��
	void FASTCALL InsertStd(DWORD data);
										// ��ʃo�b�t�@�֑}��
	void FASTCALL InsertRR(DWORD data);
										// ���A���^�C����M�o�b�t�@�֑}��

	void FASTCALL ResetReg();
										// ���W�X�^���Z�b�g
	DWORD FASTCALL ReadReg(DWORD reg);
										// ���W�X�^�ǂݏo��
	void FASTCALL WriteReg(DWORD reg, DWORD data);
										// ���W�X�^��������
	DWORD FASTCALL ReadRegRO(DWORD reg) const;
										// ���W�X�^�ǂݏo��(ReadOnly)

	void FASTCALL SetICR(DWORD data);
										// ICR�ݒ�
	void FASTCALL SetIOR(DWORD data);
										// IOR�ݒ�
	void FASTCALL SetIMR(DWORD data);
										// IMR�ݒ�
	void FASTCALL SetIER(DWORD data);
										// IER�ݒ�
	void FASTCALL SetDMR(DWORD data);
										// DMR�ݒ�
	void FASTCALL SetDCR(DWORD data);
										// DCR�ݒ�
	DWORD FASTCALL GetDSR() const;
										// DSR�擾
	void FASTCALL SetDNR(DWORD data);
										// DNR�ݒ�
	void FASTCALL SetRRR(DWORD data);
										// RRR�ݒ�
	void FASTCALL SetRMR(DWORD data);
										// RMR�ݒ�
	void FASTCALL SetAMR(DWORD data);
										// AMR�ݒ�
	void FASTCALL SetADR(DWORD data);
										// ADR�ݒ�
	DWORD FASTCALL GetRSR() const;
										// RSR�擾
	void FASTCALL SetRCR(DWORD data);
										// RCR�ݒ�
	DWORD FASTCALL GetRDR();
										// RDR�擾(�X�V����)
	DWORD FASTCALL GetRDRRO() const;
										// RDR�擾(Read Only)
	void FASTCALL SetTRR(DWORD data);
										// TRR�ݒ�
	void FASTCALL SetTMR(DWORD data);
										// TMR�ݒ�
	DWORD FASTCALL GetTSR() const;
										// TSR�擾
	void FASTCALL SetTCR(DWORD data);
										// TCR�ݒ�
	void FASTCALL SetTDR(DWORD data);
										// TDR�ݒ�
	DWORD FASTCALL GetFSR() const;
										// FSR�擾
	void FASTCALL SetFCR(DWORD data);
										// FCR�ݒ�
	void FASTCALL SetCCR(DWORD data);
										// CCR�ݒ�
	void FASTCALL SetCDR(DWORD data);
										// CDR�ݒ�
	DWORD FASTCALL GetSRR() const;
										// SRR�擾
	void FASTCALL SetSCR(DWORD data);
										// SCR�ݒ�
	void FASTCALL SetSPR(DWORD data, BOOL high);
										// SPR�ݒ�
	void FASTCALL SetGTR(DWORD data, BOOL high);
										// GTR�ݒ�
	void FASTCALL SetMTR(DWORD data, BOOL high);
										// MTR�ݒ�
	void FASTCALL SetEDR(DWORD data);
										// EDR�ݒ�
	void FASTCALL SetEOR(DWORD data);
										// EOR�ݒ�
	DWORD FASTCALL GetEIR() const;
										// EIR�擾

	void FASTCALL CheckRR();
										// ���A���^�C�����b�Z�[�W��M�o�b�t�@�`�F�b�N
	void FASTCALL Interrupt(int type, BOOL flag);
										// ���荞�ݔ���
	void FASTCALL IntCheck();
										// ���荞�݃`�F�b�N
	Event event[3];
										// �C�x���g
	midi_t midi;
										// �����f�[�^
	Sync *sync;
										// �f�[�^Sync
	DWORD recvdelay;
										// ��M�x�ꎞ��(hus)
	DWORD ex_cnt[4];
										// �G�N�X�N���[�V�u�J�E���g
};

#endif	// midi_h
