//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 �o�h�D(ytanaka@ipc-tokai.or.jp)
//	[ OPM(YM2151) ]
//
//---------------------------------------------------------------------------

#if !defined(opmif_h)
#define opmif_h

#include "device.h"
#include "event.h"
#include "opm.h"

//===========================================================================
//
//	OPM
//
//===========================================================================
class OPMIF : public MemDevice
{
public:
	// �����f�[�^��`
	typedef struct {
		DWORD reg[0x100];				// ���W�X�^
		DWORD key[8];					// �L�[���
		DWORD addr;						// �Z���N�g�A�h���X
		BOOL busy;						// BUSY�t���O
		BOOL enable[2];					// �^�C�}�C�l�[�u��
		BOOL action[2];					// �^�C�}����
		BOOL interrupt[2];				// �^�C�}���荞��
		DWORD time[2];					// �^�C�}����
		BOOL started;					// �J�n�t���O
	} opm_t;

	// �o�b�t�@�Ǘ���`
	typedef struct {
		DWORD max;						// �ő吔
		DWORD num;						// �L���f�[�^��
		DWORD read;						// �ǂݎ��|�C���g
		DWORD write;					// �������݃|�C���g
		DWORD samples;					// �����T���v����
		DWORD rate;						// �������[�g
		DWORD under;					// �A���_�[����
		DWORD over;						// �I�[�o�[����
		BOOL sound;						// FM�L��
	} opmbuf_t;

public:
	// ��{�t�@���N�V����
	OPMIF(VM *p);
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
	void FASTCALL GetOPM(opm_t *buffer);
										// �����f�[�^�擾
	BOOL FASTCALL Callback(Event *ev);
										// �C�x���g�R�[���o�b�N
	void FASTCALL Output(DWORD addr, DWORD data);
										// ���W�X�^�o��
	void FASTCALL SetEngine(FM::OPM *p);
										// �G���W���w��
	void FASTCALL InitBuf(DWORD rate);
										// �o�b�t�@������
	DWORD FASTCALL ProcessBuf();
										// �o�b�t�@����
	void FASTCALL GetBuf(DWORD *buf, int samples);
										// �o�b�t�@���擾
	void FASTCALL GetBufInfo(opmbuf_t *buffer);
										// �o�b�t�@���𓾂�
	void FASTCALL EnableFM(BOOL flag)	{ bufinfo.sound = flag; }
										// FM�����L��
	void FASTCALL ClrStarted()			{ opm.started = FALSE; }
										// �X�^�[�g�t���O���~�낷
	BOOL FASTCALL IsStarted() const		{ return opm.started; }
										// �X�^�[�g�t���O�擾

private:
	void FASTCALL CalcTimerA();
										// �^�C�}A�Z�o
	void FASTCALL CalcTimerB();
										// �^�C�}B�Z�o
	void FASTCALL CtrlTimer(DWORD data);
										// �^�C�}����
	void FASTCALL CtrlCT(DWORD data);
										// CT����
	MFP *mfp;
										// MFP
	ADPCM *adpcm;
										// ADPCM
	FDD *fdd;
										// FDD
	opm_t opm;
										// OPM�����f�[�^
	opmbuf_t bufinfo;
										// �o�b�t�@���
	Event event[2];
										// �^�C�}�[�C�x���g
	FM::OPM *engine;
										// �����G���W��
	enum {
		BufMax = 0x10000				// �o�b�t�@�T�C�Y
	};
	DWORD *opmbuf;
										// �����o�b�t�@
};

#endif	// opmif_h
