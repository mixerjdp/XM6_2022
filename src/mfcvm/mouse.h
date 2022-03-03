//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2003 �o�h�D(ytanaka@ipc-tokai.or.jp)
//	[ �}�E�X ]
//
//---------------------------------------------------------------------------

#if !defined(mouse_h)
#define mouse_h

#include "device.h"
#include "event.h"

//===========================================================================
//
//	�}�E�X
//
//===========================================================================
class Mouse : public Device
{
public:
	// �����f�[�^��`
	typedef struct {
		BOOL msctrl;					// MSCTRL�M��
		BOOL reset;						// ���Z�b�g�t���O
		BOOL left;						// ���{�^��
		BOOL right;						// �E�{�^��
		int x;							// x���W
		int y;							// y���W
		int px;							// �O���x���W
		int py;							// �O���y���W
		int mul;						// �}�E�X�{��(256�ŕW��)
		BOOL rev;						// ���E���]�t���O
		int port;						// �ڑ���|�[�g
	} mouse_t;

public:
	// ��{�t�@���N�V����
	Mouse(VM *p);
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

	// �O��API
	void FASTCALL GetMouse(mouse_t *buffer);
										// �����f�[�^�擾
	BOOL FASTCALL Callback(Event *ev);
										// �C�x���g�R�[���o�b�N
	void FASTCALL MSCtrl(BOOL flag, int port = 1);
										// MSCTRL����
	void FASTCALL SetMouse(int x, int y, BOOL left, BOOL right);
										// �}�E�X�f�[�^�w��
	void FASTCALL ResetMouse();
										// �}�E�X�f�[�^���Z�b�g

private:
	mouse_t mouse;
										// �����f�[�^
	Event event;
										// �C�x���g
	SCC *scc;
										// SCC
};

#endif	// mouse_h
