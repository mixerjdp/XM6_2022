//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2004 �o�h�D(ytanaka@ipc-tokai.or.jp)
//	[ MFP(MC68901) ]
//
//---------------------------------------------------------------------------

#if !defined(mfp_h)
#define mfp_h

#include "device.h"
#include "event.h"

//===========================================================================
//
//	MFP
//
//===========================================================================
class MFP : public MemDevice
{
public:
	// �����f�[�^��`
	typedef struct {
		// ���荞��
		BOOL ier[0x10];					// ���荞�݃C�l�[�u�����W�X�^
		BOOL ipr[0x10];					// ���荞�݃y���f�B���O���W�X�^
		BOOL isr[0x10];					// ���荞�݃C���T�[�r�X���W�X�^
		BOOL imr[0x10];					// ���荞�݃}�X�N���W�X�^
		BOOL ireq[0x10];				// ���荞�݃��N�G�X�g���W�X�^
		DWORD vr;						// �x�N�^���W�X�^
		int iidx;						// ���荞�݃C���f�b�N�X

		// �^�C�}
		DWORD tcr[4];					// �^�C�}�R���g���[�����W�X�^
		DWORD tdr[4];					// �^�C�}�f�[�^���W�X�^
		DWORD tir[4];					// �^�C�}�C���^�[�i�����W�X�^
		DWORD tbr[2];					// �^�C�}�o�b�N�A�b�v���W�X�^
		DWORD sram;						// si, info.ram�΍�t���O
		DWORD tecnt;					// �C�x���g�J�E���g���[�h�J�E���^

		// GPIP
		DWORD gpdr;						// GPIP�f�[�^���W�X�^
		DWORD aer;						// �A�N�e�B�u�G�b�W���W�X�^
		DWORD ddr;						// �f�[�^�������W�X�^
		DWORD ber;						// �o�b�N�A�b�v�G�b�W���W�X�^

		// USART
		DWORD scr;						// SYNC�L�����N�^���W�X�^
		DWORD ucr;						// USART�R���g���[�����W�X�^
		DWORD rsr;						// ���V�[�o�X�e�[�^�X���W�X�^
		DWORD tsr;						// �g�����X�~�b�^�X�e�[�^�X���W�X�^
		DWORD rur;						// ���V�[�o���[�U���W�X�^
		DWORD tur;						// �g�����X�~�b�^���[�U���W�X�^
		DWORD buffer[0x10];				// USART FIFO�o�b�t�@
		int datacount;					// USART �L���f�[�^��
		int readpoint;					// USART MFP�ǂݎ��|�C���g
		int writepoint;					// USART �L�[�{�[�h�������݃|�C���g
	} mfp_t;

public:
	// ��{�t�@���N�V����
	MFP(VM *p);
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
	void FASTCALL GetMFP(mfp_t *buffer) const;
										// �����f�[�^�擾
	BOOL FASTCALL Callback(Event *ev);
										// �C�x���g�R�[���o�b�N
	void FASTCALL IntAck();
										// ���荞�݉���
	void FASTCALL EventCount(int channel, int value);
										// �C�x���g�J�E���g
	void FASTCALL SetGPIP(int num, int value);
										// GPIP�ݒ�
	void FASTCALL KeyData(DWORD data);
										// �L�[�f�[�^�ݒ�
	DWORD FASTCALL GetVR() const;
										// �x�N�^���W�X�^�擾

private:
	// ���荞�݃R���g���[��
	void FASTCALL Interrupt(int level, BOOL enable);
										// ���荞��
	void FASTCALL IntCheck();
										// ���荞�ݗD�揇�ʃ`�F�b�N
	void FASTCALL SetIER(int offset, DWORD data);
										// IER�ݒ�
	DWORD FASTCALL GetIER(int offset) const;
										// IER�擾
	void FASTCALL SetIPR(int offset, DWORD data);
										// IPR�ݒ�
	DWORD FASTCALL GetIPR(int offset) const;
										// IPR�擾
	void FASTCALL SetISR(int offset, DWORD data);
										// ISR�ݒ�
	DWORD FASTCALL GetISR(int offset) const;
										// ISR�擾
	void FASTCALL SetIMR(int offset, DWORD data);
										// IMR�ݒ�
	DWORD FASTCALL GetIMR(int offset) const;
										// IMR�ݒ�
	void FASTCALL SetVR(DWORD data);
										// VR�ݒ�
	static const char* IntDesc[0x10];
										// ���荞�ݖ��̃e�[�u��

	// �^�C�}
	void FASTCALL SetTCR(int channel, DWORD data);
										// TCR�ݒ�
	DWORD FASTCALL GetTCR(int channel) const;
										// TCR�擾
	void FASTCALL SetTDR(int channel, DWORD data);
										// TDR�ݒ�
	DWORD FASTCALL GetTIR(int channel) const;
										// TIR�擾
	void FASTCALL Proceed(int channel);
										// �^�C�}��i�߂�
	Event timer[4];
										// �^�C�}�C�x���g
	static const int TimerInt[4];
										// �^�C�}���荞�݃e�[�u��
	static const DWORD TimerHus[8];
										// �^�C�}���ԃe�[�u��

	// GPIP
	void FASTCALL SetGPDR(DWORD data);
										// GPDR�ݒ�
	void FASTCALL IntGPIP();
										// GPIP���荞��
	static const int GPIPInt[8];
										// GPIP���荞�݃e�[�u��

	// USART
	void FASTCALL SetRSR(DWORD data);
										// RSR�ݒ�
	void FASTCALL Receive();
										// USART�f�[�^��M
	void FASTCALL SetTSR(DWORD data);
										// TSR�ݒ�
	void FASTCALL Transmit(DWORD data);
										// USART�f�[�^���M
	void FASTCALL USART();
										// USART����
	Event usart;
										// USART�C�x���g
	Sync *sync;
										// USART Sync
	Keyboard *keyboard;
										// �L�[�{�[�h
	mfp_t mfp;
										// �����f�[�^
};

#endif	// mfp_h
