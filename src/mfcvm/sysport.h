//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 �o�h�D(ytanaka@ipc-tokai.or.jp)
//	[ �V�X�e���|�[�g ]
//
//---------------------------------------------------------------------------

#if !defined(sysport_h)
#define sysport_h

#include "device.h"

//===========================================================================
//
//	�V�X�e���|�[�g
//
//===========================================================================
class SysPort : public MemDevice
{
public:
	// �����f�[�^��`
	typedef struct {
		DWORD contrast;					// �R���g���X�g
		DWORD scope_3d;					// 3D�X�R�[�v����
		DWORD image_unit;				// �C���[�W���j�b�g����
		DWORD power_count;				// �d������J�E���^
		DWORD ver_count;				// �o�[�W�����Ǘ��J�E���^
	} sysport_t;

public:
	// ��{�t�@���N�V����
	SysPort(VM *p);
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

private:
	DWORD FASTCALL GetVR();
										// �o�[�W�������W�X�^�ǂݏo��
	sysport_t sysport;
										// �������[�N
	Memory *memory;
										// ������
	SRAM *sram;
										// �X�^�e�B�b�NRAM
	Keyboard *keyboard;
										// �L�[�{�[�h
	CRTC *crtc;
										// CRTC
	Render *render;
										// �����_��
};

#endif	// sysport_h
