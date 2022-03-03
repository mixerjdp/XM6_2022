//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2004 �o�h�D(ytanaka@ipc-tokai.or.jp)
//	[ �X�v���C�g(CYNTHIA) ]
//
//---------------------------------------------------------------------------

#if !defined(sprite_h)
#define sprite_h

#include "device.h"

//===========================================================================
//
//	�X�v���C�g
//
//===========================================================================
class Sprite : public MemDevice
{
public:
	// �����f�[�^��`
	typedef struct {
		BOOL connect;					// �A�N�Z�X�\�t���O
		BOOL disp;						// �\��(�E�F�C�g)�t���O
		BYTE *mem;						// �X�v���C�g������
		BYTE *pcg;						// �X�v���C�gPCG�G���A

		BOOL bg_on[2];					// BG�\��ON
		DWORD bg_area[2];				// BG�f�[�^�G���A
		DWORD bg_scrlx[2];				// BG�X�N���[��X
		DWORD bg_scrly[2];				// BG�X�N���[��Y
		BOOL bg_size;					// BG�T�C�Y

		DWORD h_total;					// �����g�[�^������
		DWORD h_disp;					// �����\������
		DWORD v_disp;					// �����\������
		BOOL lowres;					// 15kHz���[�h
		DWORD h_res;					// �����𑜓x
		DWORD v_res;					// �����𑜓x
	} sprite_t;

public:
	// ��{�t�@���N�V����
	Sprite(VM *p);
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
	void FASTCALL Connect(BOOL con)		{ spr.connect = con; }
										// �ڑ�
	BOOL FASTCALL IsConnect() const		{ return spr.connect; }
										// �ڑ��󋵎擾
	BOOL FASTCALL IsDisplay() const		{ return spr.disp; }
										// �\���󋵎擾
	void FASTCALL GetSprite(sprite_t *buffer) const;
										// �����f�[�^�擾
	const BYTE* FASTCALL GetMem() const;
										// �������G���A�擾
	const BYTE* FASTCALL GetPCG() const;
										// PCG�G���A�擾 

private:
	void FASTCALL Control(DWORD addr, DWORD ctrl);
										// �R���g���[��
	sprite_t spr;
										// �����f�[�^
	Render *render;
										// �����_��
	BYTE *sprite;
										// �X�v���C�gRAM(64KB)
};

#endif	// sprite_h
