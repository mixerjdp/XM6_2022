//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 �o�h�D(ytanaka@ipc-tokai.or.jp)
//	[ ���O ]
//
//---------------------------------------------------------------------------

#include "os.h"
#include "xm6.h"
#include "sync.h"
#include "log.h"
#include "device.h"
#include "vm.h"
#include "cpu.h"
#include "schedule.h"

//===========================================================================
//
//	���O
//
//===========================================================================
//#define LOG_WIN32

//---------------------------------------------------------------------------
//
//	�R���X�g���N�^
//
//---------------------------------------------------------------------------
Log::Log()
{
	// �Ǘ����[�N���N���A
	logtop = 0;
	lognum = 0;
	memset(logdata, 0, sizeof(logdata));
	logcount = 1;

	// �f�o�C�X�Ȃ�
	cpu = NULL;
	scheduler = NULL;
	sync = NULL;
}

//---------------------------------------------------------------------------
//
//	������
//
//---------------------------------------------------------------------------
BOOL FASTCALL Log::Init(VM *vm)
{
	ASSERT(this);
	ASSERT(vm);

	// CPU���擾
	ASSERT(!cpu);
	cpu = (CPU *)vm->SearchDevice(MAKEID('C', 'P', 'U', ' '));
	ASSERT(cpu);

	// �X�P�W���[�����擾
	ASSERT(!scheduler);
	scheduler = (Scheduler *)vm->SearchDevice(MAKEID('S', 'C', 'H', 'E'));
	ASSERT(scheduler);

	// �����I�u�W�F�N�g�쐬
	ASSERT(!sync);
	sync = new Sync;
	ASSERT(sync);

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	�N���[���A�b�v
//
//---------------------------------------------------------------------------
void FASTCALL Log::Cleanup()
{
	ASSERT(this);

	// �f�[�^��S�ăN���A(Init�������̂�)
	if (sync) {
		Clear();
	}

	// �����I�u�W�F�N�g���폜
	if (sync) {
		delete sync;
		sync = NULL;
	}
}

//---------------------------------------------------------------------------
//
//	���Z�b�g
//
//---------------------------------------------------------------------------
void FASTCALL Log::Reset()
{
	ASSERT(this);
	ASSERT_DIAG();

	// �f�[�^��S�ăN���A
	Clear();
}

#if !defined(NDEBUG)
//---------------------------------------------------------------------------
//
//	�f�f
//
//---------------------------------------------------------------------------
void FASTCALL Log::AssertDiag() const
{
	ASSERT(this);
	ASSERT(cpu);
	ASSERT(cpu->GetID() == MAKEID('C', 'P', 'U', ' '));
	ASSERT(scheduler);
	ASSERT(scheduler->GetID() == MAKEID('S', 'C', 'H', 'E'));
	ASSERT(sync);
	ASSERT((logtop >= 0) && (logtop < LogMax));
	ASSERT((lognum >= 0) && (lognum <= LogMax));
	ASSERT(logcount >= 1);
}
#endif	// NDEBUG

//---------------------------------------------------------------------------
//
//	�N���A
//
//---------------------------------------------------------------------------
void FASTCALL Log::Clear()
{
	int i;
	int index;

	ASSERT(this);

	// ���b�N
	sync->Lock();

	index = logtop;
	for (i=0; i<lognum; i++) {
		// �f�[�^�������
		if (logdata[index]) {
			// ������|�C���^�����
			delete[] logdata[index]->string;

			// �f�[�^�{�̂����
			delete logdata[index];

			// NULL��
			logdata[index] = NULL;
		}

		// �C���f�b�N�X�ړ�
		index++;
		index &= (LogMax - 1);
	}

	// �Ǘ����[�N���N���A
	logtop = 0;
	lognum = 0;
	logcount = 1;

	// �A�����b�N
	sync->Unlock();
}

//---------------------------------------------------------------------------
//
//	�t�H�[�}�b�g(...)
//
//---------------------------------------------------------------------------
void Log::Format(loglevel level, const Device *device, char *format, ...)
{
	char buffer[0x200];
	va_list args;
	va_start(args, format);

	ASSERT(this);
	ASSERT(device);
	ASSERT_DIAG();

	// �t�H�[�}�b�g
	vsprintf(buffer, format, args);

	// �ϒ������I��
	va_end(args);

	// ���b�Z�[�W�ǉ�
	AddString(device->GetID(), level, buffer);
}

//---------------------------------------------------------------------------
//
//	�t�H�[�}�b�g(va)
//
//---------------------------------------------------------------------------
void Log::vFormat(loglevel level, const Device *device, char *format, va_list args)
{
	char buffer[0x200];

	ASSERT(this);
	ASSERT(device);
	ASSERT_DIAG();

	// �t�H�[�}�b�g
	vsprintf(buffer, format, args);

	// ���b�Z�[�W�ǉ�
	AddString(device->GetID(), level, buffer);
}

//---------------------------------------------------------------------------
//
//	�f�[�^��ǉ�
//
//---------------------------------------------------------------------------
void FASTCALL Log::AddString(DWORD id, loglevel level, char *string)
{
	int index;

	ASSERT(this);
	ASSERT(string);
	ASSERT_DIAG();

	// ���b�N
	sync->Lock();

	// �ǉ�����ʒu�����߂�
	if (lognum < LogMax) {
		ASSERT(logtop == 0);
		index = lognum;
		ASSERT(logdata[index] == NULL);
		lognum++;
	}
	else {
		ASSERT(lognum == LogMax);
		index = logtop;
		logtop++;
		logtop &= (LogMax - 1);
	}

	// ���ɑ��݂���Ή��
	if (logdata[index]) {
		delete[] logdata[index]->string;
		delete logdata[index];
	}

	// �\���̂�z�u
	logdata[index] = new logdata_t;
	logdata[index]->number = logcount++;
	logdata[index]->time = 0;
	logdata[index]->time = scheduler->GetTotalTime();
	logdata[index]->pc = cpu->GetPC();
	logdata[index]->id = id;
	logdata[index]->level = level;

	// �����񏈗�
	logdata[index]->string = new char[strlen(string) + 1];
	strcpy(logdata[index]->string, string);

#if defined(LOG_WIN32)
	// Win32���b�Z�[�W�Ƃ��ďo��
	OutputDebugString(string);
	OutputDebugString(_T("\n"));
#endif	// LOG_WIN32

	// �A�����b�N
	sync->Unlock();

	// ���O��������������A�����I�Ɉ�x�N���A(��16������ڈ��Ƃ���)
	if (logcount >= 0x60000000) {
		Clear();
	}
}

//---------------------------------------------------------------------------
//
//	�f�[�^�����擾
//
//---------------------------------------------------------------------------
int FASTCALL Log::GetNum() const
{
	ASSERT(this);
	ASSERT_DIAG();

	return lognum;
}

//---------------------------------------------------------------------------
//
//	���O�ő�L�^�����擾
//
//---------------------------------------------------------------------------
int FASTCALL Log::GetMax() const
{
	ASSERT(this);
	ASSERT_DIAG();

	return LogMax;
}

//---------------------------------------------------------------------------
//
//	�f�[�^���擾
//
//---------------------------------------------------------------------------
BOOL FASTCALL Log::GetData(int index, logdata_t *ptr)
{
	char *string;

	ASSERT(this);
	ASSERT(index >= 0);
	ASSERT(ptr);
	ASSERT_DIAG();

	// ���b�N
	sync->Lock();

	// ���݃`�F�b�N
	if (index >= lognum) {
		sync->Unlock();
		return FALSE;
	}

	// �C���f�b�N�X�Z�o
	index += logtop;
	index &= (LogMax - 1);

	// �R�s�[(������|�C���^�ȊO)
	ASSERT(logdata[index]);
	*ptr = *logdata[index];

	// �R�s�[(������|�C���^)
	string = ptr->string;
	ptr->string = new char[strlen(string) + 1];
	strcpy(ptr->string, string);

	// �A�����b�N
	sync->Unlock();
	return TRUE;
}
