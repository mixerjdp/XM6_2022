//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2005 �o�h�D(ytanaka@ipc-tokai.or.jp)
//	[ �r�f�I�R���g���[��(CATHY & VIPS) ]
//
//---------------------------------------------------------------------------

#if !defined(vc_h)
#define vc_h

#include "device.h"

//===========================================================================
//
//	�r�f�I�R���g���[��
//
//===========================================================================
class VC : public MemDevice
{
public:
	// �����f�[�^��`
	typedef struct {
		DWORD vr1h;						// VR1(H)�o�b�N�A�b�v
		DWORD vr1l;						// VR1(H)�o�b�N�A�b�v
		DWORD vr2h;						// VR2(H)�o�b�N�A�b�v
		DWORD vr2l;						// VR2(H)�o�b�N�A�b�v
		BOOL siz;						// ����ʃT�C�Y
		DWORD col;						// �F���[�h
		DWORD sp;						// �X�v���C�g�v���C�I���e�B
		DWORD tx;						// �e�L�X�g�v���C�I���e�B
		DWORD gr;						// �O���t�B�b�N�v���C�I���e�B(1024)
		DWORD gp[4];					// �O���t�B�b�N�v���C�I���e�B(512)
		BOOL ys;						// Ys�M��
		BOOL ah;						// �e�L�X�g�p���b�g������
		BOOL vht;						// �O���r�f�I������
		BOOL exon;						// ����v���C�I���e�B�E������
		BOOL hp;						// ������
		BOOL bp;						// �ŉ��ʃr�b�g�������t���O
		BOOL gg;						// �O���t�B�b�N������
		BOOL gt;						// �e�L�X�g������
		BOOL bcon;						// �V���[�v�\��
		BOOL son;						// �X�v���C�gON
		BOOL ton;						// �e�L�X�gON
		BOOL gon;						// �O���t�B�b�NON(�����1024��)
		BOOL gs[4];						// �O���t�B�b�NON(�����512��)
	} vc_t;

public:
	// ��{�t�@���N�V����
	VC(VM *p);
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
	void FASTCALL GetVC(vc_t *buffer);
										// �����f�[�^�擾
	const BYTE* FASTCALL GetPalette() const	{ return palette; }
										// �p���b�gRAM�擾
	const vc_t* FASTCALL GetWorkAddr() const{ return &vc; }
										// ���[�N�A�h���X�擾

private:
	// ���W�X�^�A�N�Z�X
	void FASTCALL SetVR0L(DWORD data);
										// ���W�X�^0(L)�ݒ�
	DWORD FASTCALL GetVR0() const;
										// ���W�X�^0�擾
	void FASTCALL SetVR1H(DWORD data);
										// ���W�X�^1(H)�ݒ�
	void FASTCALL SetVR1L(DWORD data);
										// ���W�X�^1(L)�ݒ�
	DWORD FASTCALL GetVR1() const;
										// ���W�X�^1�擾
	void FASTCALL SetVR2H(DWORD data);
										// ���W�X�^2(H)�ݒ�
	void FASTCALL SetVR2L(DWORD data);
										// ���W�X�^2(L)�ݒ�
	DWORD FASTCALL GetVR2() const;
										// ���W�X�^2�擾

	// �f�[�^
	Render *render;
										// �����_��
	vc_t vc;
										// �����f�[�^
	BYTE palette[0x400];
										// �p���b�gRAM
};

#endif	// vc_h
