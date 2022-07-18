//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2005 �o�h�D(ytanaka@ipc-tokai.or.jp)
//	[ �L�[�{�[�h ]
//
//---------------------------------------------------------------------------

#include "os.h"
#include "xm6.h"
#include "vm.h"
#include "schedule.h"
#include "mfp.h"
#include "mouse.h"
#include "sync.h"
#include "fileio.h"
#include "config.h"
#include "keyboard.h"

//===========================================================================
//
//	�L�[�{�[�h
//
//===========================================================================
//#define KEYBOARD_LOG

//---------------------------------------------------------------------------
//
//	�R���X�g���N�^
//
//---------------------------------------------------------------------------
Keyboard::Keyboard(VM *p) : Device(p)
{
	// �f�o�C�XID��������
	dev.id = MAKEID('K', 'E', 'Y', 'B');
	dev.desc = "Keyboard";

	// �I�u�W�F�N�g
	sync = NULL;
	mfp = NULL;
	mouse = NULL;
}

//---------------------------------------------------------------------------
//
//	������
//
//---------------------------------------------------------------------------
BOOL FASTCALL Keyboard::Init()
{
	int i;
	Scheduler *scheduler;

	ASSERT(this);

	// ��{�N���X
	if (!Device::Init()) {
		return FALSE;
	}

	// Sync�쐬
	sync = new Sync;

	// MFP�擾
	mfp = (MFP*)vm->SearchDevice(MAKEID('M', 'F', 'P', ' '));
	ASSERT(mfp);

	// �X�P�W���[���擾
	scheduler = (Scheduler*)vm->SearchDevice(MAKEID('S', 'C', 'H', 'E'));
	ASSERT(scheduler);

	// �}�E�X�擾
	mouse = (Mouse*)vm->SearchDevice(MAKEID('M', 'O', 'U', 'S'));
	ASSERT(mouse);

	// �C�x���g�ǉ�
	event.SetDevice(this);
	event.SetDesc("Key Repeat");
	event.SetUser(0);
	event.SetTime(0);
	scheduler->AddEvent(&event);

	// ���[�N������(���Z�b�g�M���͂Ȃ��B�p���[�I�����Z�b�g�̂�)
	keyboard.connect = TRUE;
	for (i=0; i<0x80; i++) {
		keyboard.status[i] = FALSE;
	}
	keyboard.rep_code = 0;
	keyboard.rep_count = 0;
	keyboard.rep_start = 500 * 1000 * 2;
	keyboard.rep_next = 110 * 1000 * 2;
	keyboard.send_en = TRUE;
	keyboard.send_wait = FALSE;
	keyboard.msctrl = 0;
	keyboard.tv_mode = TRUE;
	keyboard.tv_ctrl = TRUE;
	keyboard.opt2_ctrl = TRUE;
	keyboard.bright = 0;
	keyboard.led = 0;

	// TrueKey�p�R�}���h�L���[
	sync->Lock();
	keyboard.cmdnum = 0;
	keyboard.cmdread = 0;
	keyboard.cmdwrite = 0;
	sync->Unlock();

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	�N���[���A�b�v
//
//---------------------------------------------------------------------------
void FASTCALL Keyboard::Cleanup()
{
	ASSERT(this);

	// Sync�폜
	if (sync) {
		delete sync;
		sync = NULL;
	}

	// ��{�N���X��
	Device::Cleanup();
}

//---------------------------------------------------------------------------
//
//	���Z�b�g
//
//---------------------------------------------------------------------------
void FASTCALL Keyboard::Reset()
{
	ASSERT(this);
	LOG0(Log::Normal, "���Z�b�g");

	// �V�X�e���|�[�g�����Z�b�g�����̂ŁAsend_wait�����������
	keyboard.send_wait = FALSE;

	// �R�}���h��S�ăN���A
	ClrCommand();

	// 0xff�𑗐M
	if (keyboard.send_en && !keyboard.send_wait && keyboard.connect) {
		mfp->KeyData(0xff);
	}
}

//---------------------------------------------------------------------------
//
//	�Z�[�u
//
//---------------------------------------------------------------------------
BOOL FASTCALL Keyboard::Save(Fileio *fio, int ver)
{
	size_t sz;

	ASSERT(this);
	ASSERT(fio);

	LOG0(Log::Normal, "�Z�[�u");

	// �T�C�Y���Z�[�u
	sz = sizeof(keyboard_t);
	if (!fio->Write(&sz, sizeof(sz))) {
		return FALSE;
	}

	// �{�̂��Z�[�u
	if (!fio->Write(&keyboard, (int)sz)) {
		return FALSE;
	}

	// �C�x���g���Z�[�u
	if (!event.Save(fio, ver)) {
		return FALSE;
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	���[�h
//
//---------------------------------------------------------------------------
BOOL FASTCALL Keyboard::Load(Fileio *fio, int ver)
{
	size_t sz;

	ASSERT(this);
	ASSERT(fio);

	LOG0(Log::Normal, "���[�h");

	// �T�C�Y�����[�h�A�ƍ�
	if (!fio->Read(&sz, sizeof(sz))) {
		return FALSE;
	}
	if (sz != sizeof(keyboard_t)) {
		return FALSE;
	}

	// �{�̂����[�h
	if (!fio->Read(&keyboard, (int)sz)) {
		return FALSE;
	}

	// �C�x���g�����[�h
	if (!event.Load(fio, ver)) {
		return FALSE;
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	�ݒ�K�p
//
//---------------------------------------------------------------------------
void FASTCALL Keyboard::ApplyCfg(const Config *config)
{
	ASSERT(config);
	LOG0(Log::Normal, "�ݒ�K�p");

	// �ڑ�
	Connect(config->kbd_connect);
}

//---------------------------------------------------------------------------
//
//	�ڑ�
//
//---------------------------------------------------------------------------
void FASTCALL Keyboard::Connect(BOOL connect)
{
	int i;

	ASSERT(this);

	// ��v���Ă���Ζ��Ȃ�
	if (keyboard.connect == connect) {
		return;
	}

#if defined(KEYBOARD_LOG)
	if (connect) {
		LOG0(Log::Normal, "�L�[�{�[�h�ڑ�");
	}
	else {
		LOG0(Log::Normal, "�L�[�{�[�h�ؒf");
	}
#endif	// KEYBOARD_LOG

	// �ڑ���Ԃ�ۑ�
	keyboard.connect = connect;

	// �L�[���I�t�ɁA�C�x���g���~�߂�
	for (i=0; i<0x80; i++) {
		keyboard.status[i] = FALSE;
	}
	keyboard.rep_code = 0;
	event.SetTime(0);

	// �ڑ����Ă����FF�𑗐M
	if (keyboard.connect) {
		mfp->KeyData(0xff);
	}
}

//---------------------------------------------------------------------------
//
//	�����f�[�^�擾
//
//---------------------------------------------------------------------------
void FASTCALL Keyboard::GetKeyboard(keyboard_t *buffer) const
{
	ASSERT(this);
	ASSERT(buffer);

	// �o�b�t�@�փR�s�[
	*buffer = keyboard;
}

//---------------------------------------------------------------------------
//
//	�C�x���g�R�[���o�b�N
//
//---------------------------------------------------------------------------
BOOL FASTCALL Keyboard::Callback(Event *ev)
{
	ASSERT(this);
	ASSERT(ev);

	// ���s�[�g�R�[�h���L���łȂ���΁A���~
	if (keyboard.rep_code == 0) {
		ev->SetTime(0);
		return FALSE;
	}

	// �L�[�{�[�h��������Ă���΁A���~
	if (!keyboard.connect) {
		ev->SetTime(0);
		return FALSE;
	}

	// ���s�[�g�J�E���g�A�b�v
	keyboard.rep_count++;

	// ���C�N�̂ݔ��s(�u���[�N�͏o���Ȃ�:maxim.x)
	if (keyboard.send_en && !keyboard.send_wait) {
#if defined(KEYBOARD_LOG)
		LOG2(Log::Normal, "���s�[�g$%02X (%d���)", keyboard.rep_code, keyboard.rep_count);
#endif	// KEYBOARD_LOG

		mfp->KeyData(keyboard.rep_code);
	}

	// ���̃C�x���g���Ԃ�ݒ�
	ev->SetTime(keyboard.rep_next);
	return TRUE;
}

//---------------------------------------------------------------------------
//
//	hacer
//
//---------------------------------------------------------------------------
void FASTCALL Keyboard::MakeKey(DWORD code)
{
	ASSERT(this);
	ASSERT((code >= 0x01) && (code <= 0x73));

	// No se puede acceder si el teclado esta desenchufado.
	if (!keyboard.connect) {
		return;
	}

	// Si ya esta inventado, pues nada.
	if (keyboard.status[code]) {
		return;
	}

#if defined(KEYBOARD_LOG)
	LOG1(Log::Normal, "���C�N $%02X", code);
#endif	// KEYBOARD_LOG

	// Configuracion del estado
	keyboard.status[code] = TRUE;

	// Empieza a repetir.
	keyboard.rep_code = code;
	keyboard.rep_count = 0;
	event.SetTime(keyboard.rep_start);

	// Enviar los datos de la marca a la MFP
	if (keyboard.send_en && !keyboard.send_wait) {
		mfp->KeyData(keyboard.rep_code);
	}







	/*
	CString sz;
	sz.Format(_T("\n\nTecla presionada: %d\n\n"), code);
	OutputDebugStringW(CT2W(sz));
	*/
	





}

//---------------------------------------------------------------------------
//
//	romper (por ejemplo, pedir que se separen dos boxeadores)
//
//---------------------------------------------------------------------------
void FASTCALL Keyboard::BreakKey(DWORD code)
{
	ASSERT(this);
	ASSERT((code >= 0x01) && (code <= 0x73));

	// No se puede acceder si el teclado esta desenchufado.
	if (!keyboard.connect) {
		return;
	}

	// Debe estar en condiciones de hacer un corte. Si ya esta roto, entonces nada.
	if (!keyboard.status[code]) {
		return;
	}

#if defined(KEYBOARD_LOG)
	LOG1(Log::Normal, "�u���[�N $%02X", code);
#endif	// KEYBOARD_LOG

	// Configuracion del estado
	keyboard.status[code] = FALSE;

	// Si la tecla se repite, se retira la repeticion.
	if (keyboard.rep_code == (DWORD)code) {
		keyboard.rep_code = 0;
		event.SetTime(0);
	}

	// Transmision de datos a las MFP
	code |= 0x80;
	if (keyboard.send_en && !keyboard.send_wait) {
		mfp->KeyData(code);
	}
}

//---------------------------------------------------------------------------
//
//	�L�[�f�[�^���M�E�F�C�g
//
//---------------------------------------------------------------------------
void FASTCALL Keyboard::SendWait(BOOL flag)
{
	ASSERT(this);

	keyboard.send_wait = flag;
}

//---------------------------------------------------------------------------
//
//	�R�}���h
//
//---------------------------------------------------------------------------
void FASTCALL Keyboard::Command(DWORD data)
{
	ASSERT(this);
	ASSERT(data < 0x100);

	// �L���[�`�F�b�N
	ASSERT(sync);
	ASSERT(keyboard.cmdnum <= 0x10);
	ASSERT(keyboard.cmdread < 0x10);
	ASSERT(keyboard.cmdwrite < 0x10);

	// �L���[�֒ǉ�(Sync���s��)
	sync->Lock();
	keyboard.cmdbuf[keyboard.cmdwrite] = data;
	keyboard.cmdwrite = (keyboard.cmdwrite + 1) & 0x0f;
	keyboard.cmdnum++;
	if (keyboard.cmdnum > 0x10) {
		ASSERT(keyboard.cmdnum == 0x11);
		keyboard.cmdnum = 0x10;
		keyboard.cmdread = (keyboard.cmdwrite + 1) & 0x0f;
	}
	sync->Unlock();

	// �L�[�{�[�h��������Ă���Γ͂��Ȃ�
	if (!keyboard.connect) {
		return;
	}

#if defined(KEYBOARD_LOG)
	LOG1(Log::Normal, "�R�}���h���o $%02X", data);
#endif	// KEYBOARD_LOG

	// �f�B�X�v���C����
	if (data < 0x40) {
		return;
	}

	// �L�[LED
	if (data >= 0x80) {
		keyboard.led = ~data;
		return;
	}

	// �L�[���s�[�g�J�n
	if ((data & 0xf0) == 0x60) {
		keyboard.rep_start = data & 0x0f;
		keyboard.rep_start *= 100 * 1000 * 2;
		keyboard.rep_start += 200 * 1000 * 2;
		return;
	}

	// �L�[���s�[�g�p��
	if ((data & 0xf0) == 0x70) {
		keyboard.rep_next = data & 0x0f;
		keyboard.rep_next *= (data & 0x0f);
		keyboard.rep_next *= 5 * 1000 * 2;
		keyboard.rep_next += 30 * 1000 * 2;
		return;
	}

	if ((data & 0xf0) == 0x40) {
		data &= 0x0f;
		if (data < 0x08) {
			// MSCTRL����
			keyboard.msctrl = data & 0x01;
			if (keyboard.msctrl) {
				mouse->MSCtrl(TRUE, 2);
			}
			else {
				mouse->MSCtrl(FALSE, 2);
			}
			return;
		}
		else {
			// �L�[�f�[�^���o���E�֎~
			if (data & 0x01) {
				keyboard.send_en = TRUE;
			}
			else {
				keyboard.send_en = FALSE;
			}
			return;
		}
	}

	// ���0x50��
	data &= 0x0f;

	switch (data >> 2) {
		// �f�B�X�v���C����L�[���[�h�؂�ւ�
		case 0:
			if (data & 0x01) {
				keyboard.tv_mode = TRUE;
			}
			else {
				keyboard.tv_mode = FALSE;
			}
			return;

		// �L�[LED���邳
		case 1:
			keyboard.bright = data & 0x03;
			return;

		// �e���r�R���g���[���L���E����
		case 2:
			if (data & 0x01) {
				keyboard.tv_ctrl = TRUE;
			}
			else {
				keyboard.tv_ctrl = FALSE;
			}
			return;

		// OPT2�R���g���[���L���E����
		case 3:
			if (data & 0x01) {
				keyboard.opt2_ctrl = TRUE;
			}
			else {
				keyboard.opt2_ctrl = FALSE;
			}
			return;
	}

	// �ʏ�A�����ɂ͂��Ȃ�
	ASSERT(FALSE);
}

//---------------------------------------------------------------------------
//
//	�R�}���h�擾
//
//---------------------------------------------------------------------------
BOOL Keyboard::GetCommand(DWORD& data)
{
	ASSERT(this);
	ASSERT(sync);
	ASSERT(keyboard.cmdnum <= 0x10);
	ASSERT(keyboard.cmdread < 0x10);
	ASSERT(keyboard.cmdwrite < 0x10);

	// ���b�N
	sync->Lock();

	// �f�[�^���Ȃ����FALSE
	if (keyboard.cmdnum == 0) {
		sync->Unlock();
		return FALSE;
	}

	// �f�[�^���擾
	data = keyboard.cmdbuf[keyboard.cmdread];
	keyboard.cmdread = (keyboard.cmdread + 1) & 0x0f;
	ASSERT(keyboard.cmdnum > 0);
	keyboard.cmdnum--;

	// �A�����b�N
	sync->Unlock();

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	�R�}���h�N���A
//
//---------------------------------------------------------------------------
void Keyboard::ClrCommand()
{
	ASSERT(this);
	ASSERT(sync);
	ASSERT(keyboard.cmdnum <= 0x10);
	ASSERT(keyboard.cmdread < 0x10);
	ASSERT(keyboard.cmdwrite < 0x10);

	// ���b�N
	sync->Lock();

	// ������
	keyboard.cmdnum = 0;
	keyboard.cmdread = 0;
	keyboard.cmdwrite = 0;

	// �A�����b�N
	sync->Unlock();
}
