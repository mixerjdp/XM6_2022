//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2004 �o�h�D(ytanaka@ipc-tokai.or.jp)
//	[ �L�[�{�[�h ]
//
//---------------------------------------------------------------------------

#if !defined(keyboard_h)
#define keyboard_h

#include "device.h"
#include "event.h"
#include "sync.h"

//===========================================================================
//
//	�L�[�{�[�h
//
//===========================================================================
class Keyboard : public Device
{
public:
	// �����f�[�^��`
	typedef struct {
		BOOL connect;					// �ڑ��t���O
		BOOL status[0x80];				// �����X�e�[�^�X
		DWORD rep_code;					// ���s�[�g�R�[�h
		DWORD rep_count;				// ���s�[�g�J�E���^
		DWORD rep_start;				// ���s�[�g����(hus�P��)
		DWORD rep_next;					// ���s�[�g����(hus�P��)
		BOOL send_en;					// �L�[�f�[�^���M��
		BOOL send_wait;					// �L�[�f�[�^���M�����~��
		DWORD msctrl;					// �}�E�X����M��
		BOOL tv_mode;					// X68000�e���r���[�h
		BOOL tv_ctrl;					// �R�}���h�ɂ��e���r�R���g���[��
		BOOL opt2_ctrl;					// OPT2�ɂ��e���r�R���g���[��
		DWORD bright;					// �L�[���邳
		DWORD led;						// �L�[LED(1�œ_��)
		DWORD cmdbuf[0x10];				// �R�}���h�o�b�t�@
		DWORD cmdread;					// �R�}���h���[�h�|�C���^
		DWORD cmdwrite;					// �R�}���h���C�g�|�C���^
		DWORD cmdnum;					// �R�}���h��
	} keyboard_t;

public:
	// ��{�t�@���N�V����
	Keyboard(VM *p);
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
	void FASTCALL Connect(BOOL connect);
										// �ڑ�
	BOOL FASTCALL IsConnect() const		{ return keyboard.connect; }
										// �ڑ��`�F�b�N
	BOOL FASTCALL Callback(Event *ev);
										// �C�x���g�R�[���o�b�N
	void FASTCALL MakeKey(DWORD code);
										// ���C�N
	void FASTCALL BreakKey(DWORD code);
										// �u���[�N
	void FASTCALL Command(DWORD data);
										// �R�}���h
	BOOL FASTCALL GetCommand(DWORD& data);
										// �R�}���h�擾
	void FASTCALL ClrCommand();
										// �R�}���h�N���A
	void FASTCALL SendWait(BOOL flag);
										// �L�[�f�[�^���M�E�F�C�g
	BOOL FASTCALL IsSendWait() const	{ return keyboard.send_wait; }
										// �L�[�f�[�^���M�E�F�C�g�擾
	void FASTCALL GetKeyboard(keyboard_t *buffer) const;
										// �����f�[�^�擾

private:
	MFP *mfp;
										// MFP
	Mouse *mouse;
										// �}�E�X
	keyboard_t keyboard;
										// �����f�[�^
	Event event;
										// �C�x���g
	Sync *sync;
										// �R�}���hSync
};

#endif	// keyboard_h
