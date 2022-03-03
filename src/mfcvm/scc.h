//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2003 �o�h�D(ytanaka@ipc-tokai.or.jp)
//	[ SCC(Z8530) ]
//
//---------------------------------------------------------------------------

#if !defined(scc_h)
#define scc_h

#include "device.h"
#include "event.h"

//===========================================================================
//
//	SCC
//
//===========================================================================
class SCC : public MemDevice
{
public:
	// ���荞�݃^�C�v
	enum itype_t {
		rxi,							// ��M���荞��
		rsi,							// �X�y�V����Rx�R���f�B�V�������荞��
		txi,							// ���M���荞��
		exti							// �O���X�e�[�^�X�ω����荞��
	};

	// �`���l����`
	typedef struct {
		// �O���[�o��
		DWORD index;					// �`���l���ԍ�(0 or 1)

		// RR0
		BOOL ba;						// Break/Abort
		BOOL tu;						// Tx�A���_�[����
		BOOL cts;						// CTS
		BOOL sync;						// SYNC
		BOOL dcd;						// DCD
		BOOL zc;						// �[���J�E���g

		// WR0
		DWORD reg;						// �A�N�Z�X���W�X�^�I��
		BOOL ph;						// �|�C���g�n�C(��ʃ��W�X�^�I��)
		BOOL txpend;					// ���M���荞�݃y���f�B���O
		BOOL rxno;						// ��M�f�[�^�Ȃ�

		// RR1
		BOOL framing;					// �t���[�~���O�G���[
		BOOL overrun;					// �I�[�o�[�����G���[
		BOOL parerr;					// �p���e�B�G���[
		BOOL txsent;					// ���M����

		// WR1
		BOOL extie;						// �O���X�e�[�^�X���荞�݋���
		BOOL txie;						// ���M���荞�݋���
		BOOL parsp;						// �p���e�B�G���[��S-Rx���荞�݂ɂ���
		DWORD rxim;						// ��M���荞�݃��[�h

		// RR3
		BOOL rxip;						// ��M���荞�݃y���f�B���O
		BOOL rsip;						// �X�y�V����Rx���荞�݃y���f�B���O
		BOOL txip;						// ���M���荞�݃y���f�B���O
		BOOL extip;						// �O���X�e�[�^�X�ω����荞�݃y���f�B���O

		// WR3
		DWORD rxbit;					// ��M�L�����N�^�r�b�g��(5-8)
		BOOL aen;						// �I�[�g���[�h�C�l�[�u��
		BOOL rxen;						// ��M�C�l�[�u��

		// WR4
		DWORD clkm;						// �N���b�N���[�h
		DWORD stopbit;					// �X�g�b�v�r�b�g
		DWORD parity;					// �p���e�B���[�h

		// WR5
		BOOL dtr;						// DTR�M����
		DWORD txbit;					// ���M�L�����N�^�r�b�g��(5-8)
		BOOL brk;						// �u���[�N���o
		BOOL txen;						// ���M�C�l�[�u��
		BOOL rts;						// RTS�M����

		// WR8
		DWORD tdr;						// ���M�f�[�^���W�X�^
		BOOL tdf;						// ���M�f�[�^�L��

		// WR12, WR13
		DWORD tc;						// �{�[���[�g�ݒ�l

		// WR14
		BOOL loopback;					// ���[�v�o�b�N���[�h
		BOOL aecho;						// �I�[�g�G�R�[���[�h
		BOOL dtrreq;					// DTR�M�����L��
		BOOL brgsrc;					// �{�[���[�g�W�F�l���[�^�N���b�N��
		BOOL brgen;						// �{�[���[�g�W�F�l���[�^�C�l�[�u��

		// WR15
		BOOL baie;						// Break/Abort���荞�݃C�l�[�u��
		BOOL tuie;						// Tx�A���_�[�������荞�݃C�l�[�u��
		BOOL ctsie;						// CTS���荞�݃C�l�[�u��
		BOOL syncie;					// SYNC���荞�݃C�l�[�u��
		BOOL dcdie;						// DCD���荞�݃C�l�[�u��
		BOOL zcie;						// �[���J�E���g���荞�݃C�l�[�u��

		// �ʐM���x
		DWORD baudrate;					// �{�[���[�g
		DWORD cps;						// �L�����N�^/sec
		DWORD speed;					// ���x(hus�P��)

		// ��MFIFO
		DWORD rxfifo;					// ��MFIFO�L����
		DWORD rxdata[3];				// ��MFIFO�f�[�^

		// ��M�o�b�t�@
		BYTE rxbuf[0x1000];				// ��M�f�[�^
		DWORD rxnum;					// ��M�f�[�^��
		DWORD rxread;					// ��M�ǂݍ��݃|�C���^
		DWORD rxwrite;					// ��M�������݃|�C���^
		DWORD rxtotal;					// ��M�g�[�^��

		// ���M�o�b�t�@
		BYTE txbuf[0x1000];				// ���M�f�[�^
		DWORD txnum;					// ���M�f�[�^��
		DWORD txread;					// ���M�ǂݍ��݃|�C���^
		DWORD txwrite;					// ���M�������݃|�C���^
		DWORD txtotal;					// Tx�g�[�^��
		BOOL txwait;					// Tx�E�F�C�g�t���O
	} ch_t;

	// �����f�[�^��`
	typedef struct {
		// �`���l��
		ch_t ch[2];						// �`���l���f�[�^

		// RR2
		DWORD request;					// ���荞�݃x�N�^(�v����)

		// WR2
		DWORD vbase;					// ���荞�݃x�N�^(�x�[�X)

		// WR9
		BOOL shsl;						// �x�N�^�ω����[�hb4-b6/b3-b1
		BOOL mie;						// ���荞�݃C�l�[�u��
		BOOL dlc;						// ���ʃ`�F�[���֎~
		BOOL nv;						// ���荞�݃x�N�^�o�̓C�l�[�u��
		BOOL vis;						// ���荞�݃x�N�^�ω����[�h

		int ireq;						// �v�����̊��荞�݃^�C�v
		int vector;						// �v�����̃x�N�^
	} scc_t;

public:
	// ��{�t�@���N�V����
	SCC(VM *p);
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
	void FASTCALL GetSCC(scc_t *buffer) const;
										// �����f�[�^�擾
	const SCC::scc_t* FASTCALL GetWork() const;
										// ���[�N�擾 
	DWORD FASTCALL GetVector(int type) const;
										// �x�N�^�擾
	BOOL FASTCALL Callback(Event *ev);
										// �C�x���g�R�[���o�b�N
	void FASTCALL IntAck();
										// ���荞�݉���

	// ���MAPI(SCC�֑��M)
	void FASTCALL Send(int channel, DWORD data);
										// �f�[�^���M
	void FASTCALL ParityErr(int channel);
										// �p���e�B�G���[�̐���
	void FASTCALL FramingErr(int channel);
										// �t���[�~���O�G���[�̐���
	void FASTCALL SetBreak(int channel, BOOL flag);
										// �u���[�N��Ԃ̒ʒm
	BOOL FASTCALL IsRxEnable(int channel) const;
										// ��M�`�F�b�N
	BOOL FASTCALL IsBaudRate(int channel, DWORD baudrate) const;
										// �{�[���[�g�`�F�b�N
	DWORD FASTCALL GetRxBit(int channel) const;
										// ��M�f�[�^�r�b�g���擾
	DWORD FASTCALL GetStopBit(int channel) const;
										// �X�g�b�v�r�b�g�擾
	DWORD FASTCALL GetParity(int channel) const;
										// �p���e�B�擾
	BOOL FASTCALL IsRxBufEmpty(int channel) const;
										// ��M�o�b�t�@�̋󂫃`�F�b�N

	// ��MAPI(SCC�����M)
	DWORD FASTCALL Receive(int channel);
										// �f�[�^��M
	BOOL FASTCALL IsTxEmpty(int channel);
										// ���M�o�b�t�@�G���v�e�B�`�F�b�N
	BOOL FASTCALL IsTxFull(int channel);
										// ���M�o�b�t�@�t���`�F�b�N
	void FASTCALL WaitTx(int channel, BOOL wait);
										// ���M�u���b�N

	// �n�[�h�t���[
	void FASTCALL SetCTS(int channel, BOOL flag);
										// CTS�Z�b�g
	void FASTCALL SetDCD(int channel, BOOL flag);
										// DCD�Z�b�g
	BOOL FASTCALL GetRTS(int channel);
										// RTS�擾
	BOOL FASTCALL GetDTR(int channel);
										// DTR�擾
	BOOL FASTCALL GetBreak(int channel);
										// �u���[�N�擾

private:
	void FASTCALL ResetCh(ch_t *p);
										// �`���l�����Z�b�g
	DWORD FASTCALL ReadSCC(ch_t *p, DWORD reg);
										// �`���l���ǂݏo��
	DWORD FASTCALL ReadRR0(const ch_t *p) const;
										// RR0�ǂݏo��
	DWORD FASTCALL ReadRR1(const ch_t *p) const;
										// RR1�ǂݏo��
	DWORD FASTCALL ReadRR2(ch_t *p);
										// RR2�ǂݏo��
	DWORD FASTCALL ReadRR3(const ch_t *p) const;
										// RR3�ǂݏo��
	DWORD FASTCALL ReadRR8(ch_t *p);
										// RR8�ǂݏo��
	DWORD FASTCALL ReadRR15(const ch_t *p) const;
										// RR15�ǂݏo��
	DWORD FASTCALL ROSCC(const ch_t *p, DWORD reg) const;
										// �ǂݏo���̂�
	void FASTCALL WriteSCC(ch_t *p, DWORD reg, DWORD data);
										// �`���l����������
	void FASTCALL WriteWR0(ch_t *p, DWORD data);
										// WR0��������
	void FASTCALL WriteWR1(ch_t *p, DWORD data);
										// WR1��������
	void FASTCALL WriteWR3(ch_t *p, DWORD data);
										// WR3��������
	void FASTCALL WriteWR4(ch_t *p, DWORD data);
										// WR4��������
	void FASTCALL WriteWR5(ch_t *p, DWORD data);
										// WR5��������
	void FASTCALL WriteWR8(ch_t *p, DWORD data);
										// WR8��������
	void FASTCALL WriteWR9(DWORD data);
										// WR9��������
	void FASTCALL WriteWR10(ch_t *p, DWORD data);
										// WR10��������
	void FASTCALL WriteWR11(ch_t *p, DWORD data);
										// WR11��������
	void FASTCALL WriteWR12(ch_t *p, DWORD data);
										// WR12��������
	void FASTCALL WriteWR13(ch_t *p, DWORD data);
										// WR13��������
	void FASTCALL WriteWR14(ch_t *p, DWORD data);
										// WR14��������
	void FASTCALL WriteWR15(ch_t *p, DWORD data);
										// WR15��������
	void FASTCALL ResetSCC(ch_t *p);
										// ���Z�b�g
	void FASTCALL ClockSCC(ch_t *p);
										// �{�[���[�g�Čv�Z
	void FASTCALL IntSCC(ch_t *p, itype_t type, BOOL flag);
										// ���荞�݃��N�G�X�g
	void FASTCALL IntCheck();
										// ���荞�݃`�F�b�N
	void FASTCALL EventRx(ch_t *p);
										// �C�x���g(��M)
	void FASTCALL EventTx(ch_t *p);
										// �C�x���g(���M)
	Mouse *mouse;
										// �}�E�X
	scc_t scc;
										// �����f�[�^
	Event event[2];
										// �C�x���g
	BOOL clkup;
										// 7.5MHz���[�h
};

#endif	// scc_h
