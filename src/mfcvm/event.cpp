//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2005 �o�h�D(ytanaka@ipc-tokai.or.jp)
//	[ �C�x���g ]
//
//---------------------------------------------------------------------------

#include "os.h"
#include "xm6.h"
#include "vm.h"
#include "device.h"
#include "schedule.h"
#include "fileio.h"
#include "event.h"

//===========================================================================
//
//	�C�x���g
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	�R���X�g���N�^
//
//---------------------------------------------------------------------------
Event::Event()
{
	// �����o�ϐ�������
	ev.device = NULL;
	ev.desc[0] = '\0';
	ev.user = 0;
	ev.time = 0;
	ev.remain = 0;
	ev.next = NULL;
}

//---------------------------------------------------------------------------
//
//	�f�X�g���N�^
//
//---------------------------------------------------------------------------
Event::~Event()
{
}

#if !defined(NDEBUG)
//---------------------------------------------------------------------------
//
//	�f�f
//
//---------------------------------------------------------------------------
void FASTCALL Event::AssertDiag() const
{
	ASSERT(this);
	ASSERT(ev.remain <=(ev.time + 625));
	ASSERT(ev.device);
	ASSERT(ev.scheduler);
	ASSERT(ev.scheduler->GetID() == MAKEID('S', 'C', 'H', 'E'));
	ASSERT(ev.desc[0] != '\0');
}
#endif	// NDEBUG

//---------------------------------------------------------------------------
//
//	�Z�[�u
//
//---------------------------------------------------------------------------
BOOL FASTCALL Event::Save(Fileio *fio, int /*ver*/)
{
	size_t sz;

	ASSERT(this);
	ASSERT(fio);
	ASSERT_DIAG();

	// �T�C�Y���Z�[�u
	sz = sizeof(event_t);
	if (!fio->Write(&sz, sizeof(sz))) {
		return FALSE;
	}

	// ���̂��Z�[�u
	if (!fio->Write(&ev, (int)sz)) {
		return FALSE;
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	���[�h
//
//---------------------------------------------------------------------------
BOOL FASTCALL Event::Load(Fileio *fio, int ver)
{
	size_t sz;
	event_t lev;

	ASSERT(this);
	ASSERT(fio);

	// version2.01�ȑO�́A�ʃ��[�`��
	if (ver <= 0x0201) {
		return Load201(fio);
	}

	// �T�C�Y�����[�h�A�ƍ�
	if (!fio->Read(&sz, sizeof(sz))) {
		return FALSE;
	}
	if (sz != sizeof(event_t)) {
		return FALSE;
	}

	// ���̂��e���|�����̈�Ƀ��[�h
	if (!fio->Read(&lev, (int)sz)) {
		return FALSE;
	}

	// ���݂̃f�[�^�̂����A�|�C���^�ނ͎c��
	lev.device = ev.device;
	lev.scheduler = ev.scheduler;
	lev.next = ev.next;

	// �R�s�[�A�I��
	ev = lev;

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	���[�h(version 2.01)
//
//---------------------------------------------------------------------------
BOOL FASTCALL Event::Load201(Fileio *fio)
{
	size_t sz;
	event201_t ev201;

	ASSERT(this);
	ASSERT(fio);

	// �T�C�Y�����[�h�A�ƍ�
	if (!fio->Read(&sz, sizeof(sz))) {
		return FALSE;
	}
	if (sz != sizeof(event201_t)) {
		return FALSE;
	}

	// ���̂��e���|�����̈�Ƀ��[�h
	if (!fio->Read(&ev201, (int)sz)) {
		return FALSE;
	}

	// �R�s�[(����)
	strcpy(ev.desc, ev201.desc);
	ev.user = ev201.user;
	ev.time = ev201.time;
	ev.remain = ev201.remain;

	// enable���~��Ă���΁Aremain,time��0
	if (!ev201.enable) {
		ev.time = 0;
		ev.remain = 0;
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	�f�o�C�X�ݒ�
//
//---------------------------------------------------------------------------
void FASTCALL Event::SetDevice(Device *p)
{
	VM *vm;

	ASSERT(this);
	ASSERT(!ev.device);
	ASSERT(p);

	// �X�P�W���[���𓾂�
	vm = p->GetVM();
	ASSERT(vm);
	ev.scheduler = (Scheduler*)vm->SearchDevice(MAKEID('S', 'C', 'H', 'E'));
	ASSERT(ev.scheduler);

	// �f�o�C�X���L��
	ev.device = p;
}

//---------------------------------------------------------------------------
//
//	���̐ݒ�
//
//---------------------------------------------------------------------------
void FASTCALL Event::SetDesc(const char *desc)
{
	ASSERT(this);
	ASSERT(desc);
	ASSERT(strlen(desc) < sizeof(ev.desc));

	strcpy(ev.desc, desc);
}

//---------------------------------------------------------------------------
//
//	���̎擾
//
//---------------------------------------------------------------------------
const char* FASTCALL Event::GetDesc() const
{
	ASSERT(this);
	ASSERT_DIAG();

	return ev.desc;
}

//---------------------------------------------------------------------------
//
//	���Ԏ����ݒ�
//
//---------------------------------------------------------------------------
void FASTCALL Event::SetTime(DWORD hus)
{
	ASSERT(this);
	ASSERT_DIAG();

	// ���Ԏ����ݒ�(0�Ȃ�C�x���g��~)
	ev.time = hus;

	// �J�E���^�ݒ�B���s���͌o�ߕ������グ��
	// ���̍ہA�グ�����Ȃ��悤���ӂ���(Scheduler::GetPassedTime())
	ev.remain = ev.time;
	if (ev.remain > 0) {
		ev.remain += ev.scheduler->GetPassedTime();
	}
}

//---------------------------------------------------------------------------
//
//	���Ԃ�i�߂�
//
//---------------------------------------------------------------------------
void FASTCALL Event::Exec(DWORD hus)
{
	ASSERT(this);
	ASSERT_DIAG();

	// ���Ԗ��ݒ�Ȃ�A�������Ȃ�
	if (ev.time == 0) {
		ASSERT(ev.remain == 0);
		return;
	}

	// ���Ԃ�i�߂�(�I�[�o�[���͖���)
	if (ev.remain <= hus) {
		// ���Ԃ������[�h
		ev.remain = ev.time;

		// �R�[���o�b�N���s
		ASSERT(ev.device);

		// FALSE�ŋA���Ă�����A������
		if (!ev.device->Callback(this)) {
			ev.time = 0;
			ev.remain = 0;
		}
	}
	else {
		// �c������炷
		ev.remain -= hus;
	}
}
