//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2005 �o�h�D(ytanaka@ipc-tokai.or.jp)
//	[ �}�E�X ]
//
//---------------------------------------------------------------------------

#include "os.h"
#include "xm6.h"
#include "vm.h"
#include "schedule.h"
#include "scc.h"
#include "log.h"
#include "fileio.h"
#include "config.h"
#include "mouse.h"

//===========================================================================
//
//	�}�E�X
//
//===========================================================================
//#define MOUSE_LOG

//---------------------------------------------------------------------------
//
//	�R���X�g���N�^
//
//---------------------------------------------------------------------------
Mouse::Mouse(VM *p) : Device(p)
{
	// �f�o�C�XID��������
	dev.id = MAKEID('M', 'O', 'U', 'S');
	dev.desc = "Mouse";
}

//---------------------------------------------------------------------------
//
//	������
//
//---------------------------------------------------------------------------
BOOL FASTCALL Mouse::Init()
{
	Scheduler *scheduler;

	ASSERT(this);

	// ��{�N���X
	if (!Device::Init()) {
		return FALSE;
	}

	// SCC�擾
	scc = (SCC*)vm->SearchDevice(MAKEID('S', 'C', 'C', ' '));
	ASSERT(scc);

	// �X�P�W���[���擾
	scheduler = (Scheduler*)vm->SearchDevice(MAKEID('S', 'C', 'H', 'E'));
	ASSERT(scheduler);

	// �C�x���g�ݒ�
	event.SetDevice(this);
	event.SetDesc("Latency 725us");
	event.SetUser(0);
	event.SetTime(0);
	scheduler->AddEvent(&event);

	// �}�E�X�{��205/256�A���E���]�Ȃ��A�{�̐ڑ�
	mouse.mul = 205;
	mouse.rev = FALSE;
	mouse.port = 1;

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	�N���[���A�b�v
//
//---------------------------------------------------------------------------
void FASTCALL Mouse::Cleanup()
{
	ASSERT(this);

	// ��{�N���X��
	Device::Cleanup();
}

//---------------------------------------------------------------------------
//
//	���Z�b�g
//
//---------------------------------------------------------------------------
void FASTCALL Mouse::Reset()
{
	ASSERT(this);
	LOG0(Log::Normal, "���Z�b�g");

	// MSCTRL�M����'L'�ɐݒ�
	mouse.msctrl = FALSE;

	// ���Z�b�g�t���O���グ��
	mouse.reset = TRUE;
}

//---------------------------------------------------------------------------
//
//	�Z�[�u
//
//---------------------------------------------------------------------------
BOOL FASTCALL Mouse::Save(Fileio *fio, int ver)
{
	size_t sz;

	ASSERT(this);
	LOG0(Log::Normal, "�Z�[�u");

	// �T�C�Y���Z�[�u
	sz = sizeof(mouse_t);
	if (!fio->Write(&sz, sizeof(sz))) {
		return FALSE;
	}
	if (!fio->Write(&mouse, sizeof(mouse))) {
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
BOOL FASTCALL Mouse::Load(Fileio *fio, int ver)
{
	size_t sz;

	ASSERT(this);
	LOG0(Log::Normal, "���[�h");

	// �{�̂����[�h
	if (!fio->Read(&sz, sizeof(sz))) {
		return FALSE;
	}
	if (sz != sizeof(mouse_t)) {
		return FALSE;
	}
	if (!fio->Read(&mouse, sizeof(mouse_t))) {
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
void FASTCALL Mouse::ApplyCfg(const Config *config)
{
	ASSERT(this);
	ASSERT(config);
	LOG0(Log::Normal, "�ݒ�K�p");

	// �䗦
	if (mouse.mul != config->mouse_speed) {
		// ���������W���Z�b�g
		mouse.mul = config->mouse_speed;
		mouse.reset = TRUE;
	}

	// ���]�t���O
	mouse.rev = config->mouse_swap;

	// �|�[�g
	mouse.port = config->mouse_port;
	if (mouse.port == 0) {
		// �ڑ��Ȃ��Ȃ̂ŁA�C�x���g���~�߂�
		mouse.reset = TRUE;
		event.SetTime(0);
	}
}

//---------------------------------------------------------------------------
//
//	�����f�[�^�擾
//
//---------------------------------------------------------------------------
void FASTCALL Mouse::GetMouse(mouse_t *buffer)
{
	ASSERT(this);
	ASSERT(buffer);

	// �������[�N���R�s�[
	*buffer = mouse;
}

//---------------------------------------------------------------------------
//
//	�C�x���g�R�[���o�b�N
//
//---------------------------------------------------------------------------
BOOL FASTCALL Mouse::Callback(Event* /*ev*/)
{
	DWORD status;
	int dx;
	int dy;

	ASSERT(this);
	ASSERT(scc);

	// �X�e�[�^�X�쐬
	status = 0;

	// �{�^���쐬
	if (mouse.left) {
		status |= 0x01;
	}
	if (mouse.right) {
		status |= 0x02;
	}

	// X�����쐬
	dx = mouse.x - mouse.px;
	dx *= mouse.mul;
	dx >>= 8;
	if (dx > 0x7f) {
		dx = 0x7f;
		status |= 0x10;
	}
	if (dx < -0x80) {
		dx = -0x80;
		status |= 0x20;
	}

	// Y�����쐬
	dy = mouse.y - mouse.py;
	dy *= mouse.mul;
	dy >>= 8;
	if (dy > 0x7f) {
		dy = 0x7f;
		status |= 0x40;
	}
	if (dy < -0x80) {
		dy = -0x80;
		status |= 0x80;
	}

	// SCC����M���Ă��Ȃ���Ή������Ȃ�
	if (!scc->IsRxEnable(1)) {
		return FALSE;
	}

	// ��Mbps��4800�łȂ���΁A�t���[�~���O�G���[
	if (!scc->IsBaudRate(1, 4800)) {
#if defined(MOUSE_LOG)
		LOG0(Log::Normal, "SCC�{�[���[�g�G���[");
#endif	// MOUSE_LOG
		scc->FramingErr(1);
		return FALSE;
	}

	// �f�[�^��8bit�łȂ���΁A�t���[�~���O�G���[
	if (scc->GetRxBit(1) != 8) {
#if defined(MOUSE_LOG)
		LOG0(Log::Normal, "SCC�f�[�^�r�b�g���G���[");
#endif	// MOUSE_LOG
		scc->FramingErr(1);
		return FALSE;
	}

	// �X�g�b�v�r�b�g��2bit�łȂ���΁A�t���[�~���O�G���[
	if (scc->GetStopBit(1) != 3) {
#if defined(MOUSE_LOG)
		LOG0(Log::Normal, "SCC�X�g�b�v�r�b�g�G���[");
#endif	// MOUSE_LOG
		scc->FramingErr(1);
		return FALSE;
	}

	// �p���e�B�Ȃ��łȂ���΁A�p���e�B�G���[
	if (scc->GetParity(1) != 0) {
#if defined(MOUSE_LOG)
		LOG0(Log::Normal, "SCC�p���e�B�G���[");
#endif	// MOUSE_LOG
		scc->ParityErr(1);
		return FALSE;
	}

	// SCC�̎�M�o�b�t�@�Ƀf�[�^������Ȃ瑗�M���Ȃ�
	if (!scc->IsRxBufEmpty(1)) {
#if defined(MOUSE_LOG)
		LOG0(Log::Normal, "SCC��M�o�b�t�@�Ƀf�[�^����");
#endif	// MOUSE_LOG
		return FALSE;
	}

	// �f�[�^���M(3�o�C�g)
#if defined(MOUSE_LOG)
	LOG3(Log::Normal, "�f�[�^���o St:$%02X X:$%02X Y:$%02X", status, dx & 0xff, dy & 0xff);
#endif	// MOUSE_LOG
	scc->Send(1, status);
	scc->Send(1, dx);
	scc->Send(1, dy);

	// �O��f�[�^���X�V
	mouse.px = mouse.x;
	mouse.py = mouse.y;

	return FALSE;
}

//---------------------------------------------------------------------------
//
//	MSCTRL����
//
//---------------------------------------------------------------------------
void FASTCALL Mouse::MSCtrl(BOOL flag, int port)
{
	ASSERT(this);
	ASSERT((port == 1) || (port == 2));

#if defined(MOUSE_LOG)
	LOG2(Log::Normal, "PORT=%d MSCTRL=%d", port, flag);
#endif	// MOUSE_LOG

	// �|�[�g����v���Ȃ���Ή������Ȃ�
	if (port != mouse.port) {
		return;
	}

	// 'H'����'L'�ւ̗�����ŃC�x���g����
	if (flag) {
		mouse.msctrl = TRUE;
		return;
	}

	// �O��'H'���`�F�b�N
	if (!mouse.msctrl) {
		return;
	}

	// �L��
	mouse.msctrl = FALSE;

	// �C�x���g���Ȃ疳��
	if (event.GetTime() != 0) {
		return;
	}

	// �C�x���g����(725us)
	event.SetTime(725 * 2);
}

//---------------------------------------------------------------------------
//
//	�}�E�X�f�[�^�ݒ�
//
//---------------------------------------------------------------------------
void FASTCALL Mouse::SetMouse(int x, int y, BOOL left, BOOL right)
{
	ASSERT(this);

	// �f�[�^�L��
	mouse.x = x;
	mouse.y = y;
	if (mouse.rev) {
		mouse.left = right;
		mouse.right = left;
	}
	else {
		mouse.left = left;
		mouse.right = right;
	}

	// ���Z�b�g�t���O������΍��W�����Z�b�g
	if (mouse.reset) {
		mouse.reset = FALSE;
		mouse.px = x;
		mouse.py = y;
	}
}

//---------------------------------------------------------------------------
//
//	�}�E�X�f�[�^���Z�b�g
//
//---------------------------------------------------------------------------
void FASTCALL Mouse::ResetMouse()
{
	ASSERT(this);

	mouse.reset = TRUE;
}
