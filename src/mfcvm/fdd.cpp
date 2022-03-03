//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 �o�h�D(ytanaka@ipc-tokai.or.jp)
//	[ FDD(FD55GFR) ]
//
//---------------------------------------------------------------------------

#include "os.h"
#include "xm6.h"
#include "vm.h"
#include "log.h"
#include "schedule.h"
#include "filepath.h"
#include "fileio.h"
#include "rtc.h"
#include "iosc.h"
#include "config.h"
#include "fdc.h"
#include "fdi.h"
#include "fdd.h"

//===========================================================================
//
//	FDD
//
//===========================================================================
//#define FDD_LOG

//---------------------------------------------------------------------------
//
//	�R���X�g���N�^
//
//---------------------------------------------------------------------------
FDD::FDD(VM *p) : Device(p)
{
	// �f�o�C�XID��������
	dev.id = MAKEID('F', 'D', 'D', ' ');
	dev.desc = "Floppy Drive";

	// �I�u�W�F�N�g
	fdc = NULL;
	iosc = NULL;
	rtc = NULL;
}

//---------------------------------------------------------------------------
//
//	������
//
//---------------------------------------------------------------------------
BOOL FASTCALL FDD::Init()
{
	int i;
	Scheduler *scheduler;

	ASSERT(this);

	// ��{�N���X
	if (!Device::Init()) {
		return FALSE;
	}

	// FDC�擾
	fdc = (FDC*)vm->SearchDevice(MAKEID('F', 'D', 'C', ' '));
	ASSERT(fdc);

	// IOSC�擾
	iosc = (IOSC*)vm->SearchDevice(MAKEID('I', 'O', 'S', 'C'));
	ASSERT(iosc);

	// �X�P�W���[���擾
	scheduler = (Scheduler*)vm->SearchDevice(MAKEID('S', 'C', 'H', 'E'));
	ASSERT(scheduler);

	// RTC�擾
	rtc = (RTC*)vm->SearchDevice(MAKEID('R', 'T', 'C', ' '));
	ASSERT(rtc);

	// �h���C�u�ʂ̏�����
	for (i=0; i<4; i++) {
		drv[i].fdi = new FDI(this);
		drv[i].next = NULL;
		drv[i].seeking = FALSE;
		drv[i].cylinder = 0;
		drv[i].insert = FALSE;
		drv[i].invalid = FALSE;
		drv[i].eject = TRUE;
		drv[i].blink = FALSE;
		drv[i].access = FALSE;
	}

	// ApplyCfg����
	fdd.fast = FALSE;

	// ���ʕ����̏�����
	fdd.motor = FALSE;
	fdd.settle = FALSE;
	fdd.force = FALSE;
	fdd.first = TRUE;
	fdd.selected = 0;
	fdd.hd = TRUE;

	// �V�[�N�C�x���g������
	seek.SetDevice(this);
	seek.SetDesc("Seek");
	seek.SetUser(0);
	seek.SetTime(0);
	scheduler->AddEvent(&seek);

	// ��]���C�x���g������(�Z�g�����O���p)
	rotation.SetDevice(this);
	rotation.SetDesc("Rotation Stopped");
	rotation.SetUser(1);
	rotation.SetTime(0);
	scheduler->AddEvent(&rotation);

	// �C�W�F�N�g�C�x���g������(��}�����p)
	eject.SetDevice(this);
	eject.SetDesc("Eject");
	eject.SetUser(2);
	eject.SetTime(0);
	scheduler->AddEvent(&eject);

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	�N���[���A�b�v
//
//---------------------------------------------------------------------------
void FASTCALL FDD::Cleanup()
{
	int i;

	ASSERT(this);

	// �C���[�W�t�@�C�������
	for (i=0; i<4; i++) {
		if (drv[i].fdi) {
			delete drv[i].fdi;
			drv[i].fdi = NULL;
		}
		if (drv[i].next) {
			delete drv[i].next;
			drv[i].next = NULL;
		}
	}

	// ��{�N���X��
	Device::Cleanup();
}

//---------------------------------------------------------------------------
//
//	���Z�b�g
//
//---------------------------------------------------------------------------
void FASTCALL FDD::Reset()
{
	int i;

	ASSERT(this);
	LOG0(Log::Normal, "���Z�b�g");

	// �h���C�u�ʂ̃��Z�b�g
	for (i=0; i<4; i++) {
		drv[i].seeking = FALSE;
		drv[i].eject = TRUE;
		drv[i].blink = FALSE;
		drv[i].access = FALSE;

		// next������΁A�i�グ
		if (drv[i].next) {
			delete drv[i].fdi;
			drv[i].fdi = drv[i].next;
			drv[0].fdi->Adjust();
			drv[1].fdi->Adjust();
			drv[i].next = NULL;
			drv[i].insert = TRUE;
			drv[i].invalid = FALSE;
		}

		// invalid�Ȃ�A�C�W�F�N�g���
		if (drv[i].invalid) {
			delete drv[i].fdi;
			drv[i].fdi = new FDI(this);
			drv[i].insert = FALSE;
			drv[i].invalid = FALSE;
		}

		// �V�����_0�փV�[�N
		drv[i].cylinder = 0;
		drv[i].fdi->Seek(0);
	}

	// ���ʕ����̃��Z�b�g(selected��FDC��DSR�ƍ��킹�鎖)
	fdd.motor = FALSE;
	fdd.settle = FALSE;
	fdd.force = FALSE;
	fdd.first = TRUE;
	fdd.selected = 0;
	fdd.hd = TRUE;

	// �V�[�N�C�x���g�Ȃ�(seeking=FALSE)
	seek.SetTime(0);

	// ��]���E�Z�g�����O�C�x���g�Ȃ�(motor=FALSE, settle=FALSE)
	rotation.SetDesc("Rotation Stopped");
	rotation.SetTime(0);

	// �C�W�F�N�g�C�x���g�Ȃ�(�i�グ��invalid)
	eject.SetTime(0);
}

//---------------------------------------------------------------------------
//
//	�Z�[�u
//
//---------------------------------------------------------------------------
BOOL FASTCALL FDD::Save(Fileio *fio, int ver)
{
	int i;
	size_t sz;

	ASSERT(this);
	ASSERT(fio);

	LOG0(Log::Normal, "�Z�[�u");

	// �h���C�u�ʕ������Z�[�u
	for (i=0; i<4; i++) {
		// �T�C�Y���Z�[�u
		sz = sizeof(drv_t);
		if (!fio->Write(&sz, sizeof(sz))) {
			return FALSE;
		}

		// ���̂��Z�[�u
		if (!fio->Write(&drv[i], (int)sz)) {
			return FALSE;
		}

		// �C���[�W�����͔C����
		if (drv[i].fdi) {
			if (!drv[i].fdi->Save(fio, ver)) {
				return FALSE;
			}
		}
		if (drv[i].next) {
			if (!drv[i].next->Save(fio, ver)) {
				return FALSE;
			}
		}
	}

	// ���ʕ������Z�[�u
	sz = sizeof(fdd);
	if (!fio->Write(&sz, sizeof(sz))) {
		return FALSE;
	}
	if (!fio->Write(&fdd, (int)sz)) {
		return FALSE;
	}

	// �C�x���g���Z�[�u
	if (!seek.Save(fio, ver)) {
		return FALSE;
	}
	if (!rotation.Save(fio, ver)) {
		return FALSE;
	}
	if (!eject.Save(fio, ver)) {
		return FALSE;
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	���[�h
//
//---------------------------------------------------------------------------
BOOL FASTCALL FDD::Load(Fileio *fio, int ver)
{
	int i;
	size_t sz;
	drv_t work;
	BOOL ready;
	Filepath path;
	int media;
	BOOL success;
	BOOL failed;

	ASSERT(this);
	ASSERT(fio);

	LOG0(Log::Normal, "���[�h");

	// �h���C�u�ʕ������Z�[�u
	for (i=0; i<4; i++) {
		// �T�C�Y�����[�h
		if (!fio->Read(&sz, sizeof(sz))) {
			return FALSE;
		}
		if (sz != sizeof(drv_t)) {
			return FALSE;
		}

		// ���̂����[�h
		if (!fio->Read(&work, (int)sz)) {
			return FALSE;
		}

		// ���݂̃C���[�W�����ׂč폜
		if (drv[i].fdi) {
			delete drv[i].fdi;
			drv[i].fdi = NULL;
		}
		if (drv[i].next) {
			delete drv[i].next;
			drv[i].next = NULL;
		}

		// �]��
		drv[i] = work;

		// �C���[�W�������č\�z
		failed = FALSE;
		if (drv[i].fdi) {
			// �\�z(���݂�drv[i].fdi�͈Ӗ��������Ȃ�����)
			drv[i].fdi = new FDI(this);

			// ���[�h�����݂�
			success = FALSE;
			if (drv[i].fdi->Load(fio, ver, &ready, &media, path)) {
				// ���f�B�̏ꍇ
				if (ready) {
					// �I�[�v�������݂�
					if (drv[i].fdi->Open(path, media)) {
						// ����
						drv[i].fdi->Seek(drv[i].cylinder);
						success = TRUE;
					}
					else {
						// ���s(�Z�[�u���鎞�_�̓��f�B�������̂ŁA�C�W�F�N�g)
						failed = TRUE;
					}
				}
				else {
					// ���f�B�łȂ���΁A���[�hOK
					success = TRUE;
				}
			}

			// ���s�����ꍇ�A�O�̂��ߍč쐬
			if (!success) {
				delete drv[i].fdi;
				drv[i].fdi = new FDI(this);
			}
		}
		if (drv[i].next) {
			// �\�z(���݂�drv[i].next�͈Ӗ��������Ȃ�����)
			drv[i].next = new FDI(this);

			// ���[�h�����݂�
			success = FALSE;
			if (drv[i].next->Load(fio, ver, &ready, &media, path)) {
				// ���f�B�̏ꍇ
				if (ready) {
					// �I�[�v�������݂�
					if (drv[i].next->Open(path, media)) {
						// ����
						drv[i].next->Seek(drv[i].cylinder);
						success = TRUE;
					}
				}
			}

			// ���s�����ꍇ�́Adelete����next�����O��
			if (!success) {
				delete drv[i].next;
				drv[i].next = NULL;
			}
		}

		// �{��FDI�̍ăI�[�v���Ɏ��s�����ꍇ�A�����C�W�F�N�g���N����
		if (failed) {
			Eject(i, TRUE);
		}
	}

	// ���ʕ��������[�h
	if (!fio->Read(&sz, sizeof(sz))) {
		return FALSE;
	}
	if (sz != sizeof(fdd)) {
		return FALSE;
	}
	if (!fio->Read(&fdd, (int)sz)) {
		return FALSE;
	}

	// �C�x���g�����[�h
	if (!seek.Load(fio, ver)) {
		return FALSE;
	}
	if (!rotation.Load(fio, ver)) {
		return FALSE;
	}
	if (!eject.Load(fio, ver)) {
		return FALSE;
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	�ݒ�K�p
//
//---------------------------------------------------------------------------
void FASTCALL FDD::ApplyCfg(const Config *config)
{
	ASSERT(this);
	ASSERT(config);
	LOG0(Log::Normal, "�ݒ�K�p");

	// �������[�h
	fdd.fast = config->floppy_speed;
#if defined(FDD_LOG)
	if (fdd.fast) {
		LOG0(Log::Normal, "�������[�h ON");
	}
	else {
		LOG0(Log::Normal, "�������[�h OFF");
	}
#endif	// FDD_LOG
}

//---------------------------------------------------------------------------
//
//	�h���C�u���[�N�擾
//
//---------------------------------------------------------------------------
void FASTCALL FDD::GetDrive(int drive, drv_t *buffer) const
{
	ASSERT(this);
	ASSERT(buffer);
	ASSERT((drive >= 0) && (drive <= 3));

	// �h���C�u���[�N���R�s�[
	*buffer = drv[drive];
}

//---------------------------------------------------------------------------
//
//	�������[�N�擾
//
//---------------------------------------------------------------------------
void FASTCALL FDD::GetFDD(fdd_t *buffer) const
{
	ASSERT(this);
	ASSERT(buffer);

	// �������[�N���R�s�[
	*buffer = fdd;
}

//---------------------------------------------------------------------------
//
//	FDI�擾
//
//---------------------------------------------------------------------------
FDI* FASTCALL FDD::GetFDI(int drive)
{
	ASSERT(this);
	ASSERT((drive == 0) || (drive == 1));

	// next������΁Anext��D��
	if (drv[drive].next) {
		return drv[drive].next;
	}

	// fdi�͕K������
	ASSERT(drv[drive].fdi);
	return drv[drive].fdi;
}

//---------------------------------------------------------------------------
//
//	�C�x���g�R�[���o�b�N
//
//---------------------------------------------------------------------------
BOOL FASTCALL FDD::Callback(Event *ev)
{
	DWORD user;
	int i;

	ASSERT(this);
	ASSERT(ev);

	// ���[�U�f�[�^���󂯎��
	user = ev->GetUser();

	switch (user) {
		// 0:�V�[�N
		case 0:
			for (i=0; i<4; i++) {
				// �V�[�N����
				if (drv[i].seeking) {
					// �V�[�N����
					drv[i].seeking = FALSE;
#if defined(FDD_LOG)
					LOG1(Log::Normal, "�V�[�N���� �h���C�u%d", i);
#endif	// FDD_LOG

					// ���f�B��Ԃɂ���ĕ�����
					fdc->CompleteSeek(i, IsReady(i));
				}
			}
			// �P���Ȃ̂�break
			break;

		// 1:��]�E�Z�g�����O
		case 1:
			// �X�^���o�C���Ȃ�P��
			if (!fdd.settle && !fdd.motor) {
				return FALSE;
			}

			// �Z�g�����O������
			if (fdd.settle) {
				fdd.settle = FALSE;
				fdd.motor = TRUE;
				fdd.first = TRUE;

				// ��]
				Rotation();
			}
			// �p��
			return TRUE;

		// 2:�C�W�F�N�g�E��}��
		case 2:
			for (i=0; i<4; i++) {
				// Next������΁A����ւ�
				if (drv[i].next) {
					delete drv[i].fdi;
					drv[i].fdi = drv[i].next;
					drv[0].fdi->Adjust();
					drv[1].fdi->Adjust();
					drv[i].next = NULL;
					Insert(i);
				}

				// invalid�ŃC�W�F�N�g
				if (drv[i].invalid) {
					Eject(i, TRUE);
				}
			}
			// �P���Ȃ̂�break
			break;

		// ���̑�(���肦�Ȃ�)
		default:
			ASSERT(FALSE);
			break;
	}

	// �P������
	return FALSE;
}

//---------------------------------------------------------------------------
//
//	�I�[�v��
//
//---------------------------------------------------------------------------
BOOL FASTCALL FDD::Open(int drive, const Filepath& path, int media)
{
	FDI *fdi;

	ASSERT(this);
	ASSERT((drive >= 0) && (drive <= 3));
	ASSERT((media >= 0) && (media < 0x10));

#if defined(FDD_LOG)
	LOG2(Log::Normal, "�f�B�X�N�I�[�v�� �h���C�u%d ���f�B�A%d", drive, media);
#endif	// FDD_LOG

	// FDI�쐬+�I�[�v��
	fdi = new FDI(this);
	if (!fdi->Open(path, media)) {
		delete fdi;
		return FALSE;
	}

	// �C���[�W������s���ăV�[�N
	fdi->Seek(drv[drive].cylinder);

	// ���Ɏ��C���[�W���\�񂳂�Ă���ꍇ
	if (drv[drive].next) {
		// ���C���[�W���폜���čēx�C�x���g(300ms)
		delete drv[drive].next;
		drv[drive].next = fdi;
		eject.SetTime(300 * 1000 * 2);
		return TRUE;
	}

	// ���f�B�A������ꍇ
	if (drv[drive].insert && !drv[drive].invalid) {
		// ���C���[�W�ɃZ�b�g���ăC�W�F�N�g�A�C�x���g���s(300ms)
		Eject(drive, FALSE);
		drv[drive].next = fdi;
		eject.SetTime(300 * 1000 * 2);
		return TRUE;
	}

	// �ʏ�͂��̂܂܃C���T�[�g
	delete drv[drive].fdi;
	drv[drive].fdi = fdi;
	drv[0].fdi->Adjust();
	drv[1].fdi->Adjust();
	Insert(drive);
	return TRUE;
}

//---------------------------------------------------------------------------
//
//	�f�B�X�N�}��
//
//---------------------------------------------------------------------------
void FASTCALL FDD::Insert(int drive)
{
	ASSERT(this);
	ASSERT((drive >= 0) && (drive <= 3));

#if defined(FDD_LOG)
	LOG1(Log::Normal, "�f�B�X�N�}�� �h���C�u%d", drive);
#endif	// FDD_LOG

	// �V�[�N�͈�U����
	if (drv[drive].seeking) {
		drv[drive].seeking = FALSE;
		fdc->CompleteSeek(drive, FALSE);
	}

	// �f�B�X�N�}������A��}���Ȃ�
	drv[drive].insert = TRUE;
	drv[drive].invalid = FALSE;

	// ���݂̃V�����_�փV�[�N
	ASSERT(drv[drive].fdi);
	drv[drive].fdi->Seek(drv[drive].cylinder);

	// ���荞�݂��N����
	iosc->IntFDD(TRUE);
}

//---------------------------------------------------------------------------
//
//	�f�B�X�N�C�W�F�N�g
//
//---------------------------------------------------------------------------
void FASTCALL FDD::Eject(int drive, BOOL force)
{
	ASSERT(this);
	ASSERT((drive >= 0) && (drive <= 3));

	// �����t���O�������Ă��Ȃ���΁A�C�W�F�N�g�֎~���l��
	if (!force && !drv[drive].eject) {
		return;
	}

	// ���ɃC�W�F�N�g����Ă���Εs�v
	if (!drv[drive].insert && !drv[drive].invalid) {
		return;
	}

#if defined(FDD_LOG)
	LOG1(Log::Normal, "�f�B�X�N�C�W�F�N�g �h���C�u%d", drive);
#endif	// FDD_LOG

	// �V�[�N�͈�U����
	if (drv[drive].seeking) {
		drv[drive].seeking = FALSE;
		fdc->CompleteSeek(drive, FALSE);
	}

	// �C���[�W�t�@�C����FDI�ɓ���ւ���
	ASSERT(drv[drive].fdi);
	delete drv[drive].fdi;
	drv[drive].fdi = new FDI(this);

	// �f�B�X�N�}���Ȃ��A��}���Ȃ�
	drv[drive].insert = FALSE;
	drv[drive].invalid = FALSE;

	// �A�N�Z�X�Ȃ�(LED��̓s��)
	drv[drive].access = FALSE;

	// next�͈�U����
	if (drv[drive].next) {
		delete drv[drive].next;
		drv[drive].next = NULL;
	}

	// ���荞�݂��N����
	iosc->IntFDD(TRUE);
}

//---------------------------------------------------------------------------
//
//	�f�B�X�N��}��
//
//---------------------------------------------------------------------------
void FASTCALL FDD::Invalid(int drive)
{
	ASSERT(this);
	ASSERT((drive >= 0) && (drive <= 3));

#if defined(FDD_LOG)
	LOG1(Log::Normal, "�f�B�X�N��}�� �h���C�u%d", drive);
#endif	// FDD_LOG

	// ���Ɍ�}���ł���Εs�v
	if (drv[drive].insert && drv[drive].invalid) {
		return;
	}

	// �V�[�N�͈�U����
	if (drv[drive].seeking) {
		drv[drive].seeking = FALSE;
		fdc->CompleteSeek(drive, FALSE);
	}

	// �f�B�X�N�}������A��}������
	drv[drive].insert = TRUE;
	drv[drive].invalid = TRUE;

	// next�͈�U����
	if (drv[drive].next) {
		delete drv[drive].next;
		drv[drive].next = NULL;
	}

	// ���荞�݂��N����
	iosc->IntFDD(TRUE);

	// �C�x���g��ݒ�(300ms)
	eject.SetTime(300 * 1000 * 2);
}

//---------------------------------------------------------------------------
//
//	�h���C�u�R���g���[��
//
//---------------------------------------------------------------------------
void FASTCALL FDD::Control(int drive, DWORD func)
{
	ASSERT(this);
	ASSERT((drive >= 0) && (drive <= 3));

	// bit7 - LED����
	if (func & 0x80) {
		if (!drv[drive].blink) {
			// �_�łȂ����_�ł���
#if defined(FDD_LOG)
			LOG1(Log::Normal, "LED�_�ł��� �h���C�u%d", drive);
#endif	// FDD_LOG
			drv[drive].blink = TRUE;
		}
	}
	else {
		drv[drive].blink = FALSE;
	}

	// bit6 - �C�W�F�N�g�L��
	if (func & 0x40) {
		if (drv[drive].eject) {
#if defined(FDD_LOG)
			LOG1(Log::Normal, "�C�W�F�N�g�֎~ �h���C�u%d", drive);
#endif	// FDD_LOG
			drv[drive].eject = FALSE;
		}
	}
	else {
		drv[drive].eject = TRUE;
	}

	// bit5 - �C�W�F�N�g
	if (func & 0x20) {
		Eject(drive, TRUE);
	}
}

//---------------------------------------------------------------------------
//
//	�������f�B
//
//---------------------------------------------------------------------------
void FASTCALL FDD::ForceReady(BOOL flag)
{
	ASSERT(this);

#if defined(FDD_LOG)
	if (flag) {
		LOG0(Log::Normal, "�������f�B ON");
	}
	else {
		LOG0(Log::Normal, "�������f�B OFF");
	}
#endif	// FDD_LOG

	// �Z�b�g�̂�
	fdd.force = flag;
}

//---------------------------------------------------------------------------
//
//	��]�ʒu�擾
//
//---------------------------------------------------------------------------
DWORD FASTCALL FDD::GetRotationPos() const
{
	DWORD remain;
	DWORD hus;

	ASSERT(this);

	// ��~���Ă����0
	if (rotation.GetTime() == 0) {
		return 0;
	}

	// ��]���𓾂�
	hus = GetRotationTime();

	// �_�E���J�E���^�𓾂�
	remain = rotation.GetRemain();
	if (remain > hus) {
		remain = 0;
	}

	// �t����
	return (DWORD)(hus - remain);
}

//---------------------------------------------------------------------------
//
//	��]���Ԏ擾
//
//---------------------------------------------------------------------------
DWORD FASTCALL FDD::GetRotationTime() const
{
	ASSERT(this);

	// 2HD��360rpm�A2DD��300rpm
	if (fdd.hd) {
		return 333333;
	}
	else {
		return 400000;
	}
}

//---------------------------------------------------------------------------
//
//	��]
//
//---------------------------------------------------------------------------
void FASTCALL FDD::Rotation()
{
	DWORD rpm;
	DWORD hus;
	char desc[0x20];

	ASSERT(this);
	ASSERT(!fdd.settle);
	ASSERT(fdd.motor);

	rpm = 2000 * 1000 * 60;
	hus = GetRotationTime();
	rpm /= hus;
	sprintf(desc, "Rotation %drpm", rpm);
	rotation.SetDesc(desc);
	rotation.SetTime(hus);
}

//---------------------------------------------------------------------------
//
//	�������Ԏ擾
//
//---------------------------------------------------------------------------
DWORD FASTCALL FDD::GetSearch()
{
	DWORD schtime;

	ASSERT(this);

	// �������[�h����64us�Œ�
	if (fdd.fast) {
		return 128;
	}

	// �C���[�W�t�@�C���ɕ���
	schtime = drv[ fdd.selected ].fdi->GetSearch();

	return schtime;
}

//---------------------------------------------------------------------------
//
//	HD�ݒ�
//
//---------------------------------------------------------------------------
void FASTCALL FDD::SetHD(BOOL hd)
{

	ASSERT(this);

	if (hd) {
		if (fdd.hd) {
			return;
		}
#if defined(FDD_LOG)
		LOG0(Log::Normal, "�h���C�u���x�ύX 2HD");
#endif	// FDD_LOG
		fdd.hd = TRUE;
	}
	else {
		if (!fdd.hd) {
			return;
		}
#if defined(FDD_LOG)
		LOG0(Log::Normal, "�h���C�u���x�ύX 2DD");
#endif	// FDD_LOG
		fdd.hd = FALSE;
	}

	// ���[�^���쒆�Ȃ�A���x�ύX
	if (!fdd.motor || fdd.settle) {
		return;
	}

	// ��]���Đݒ�
	Rotation();
}

//---------------------------------------------------------------------------
//
//	HD�擾
//
//---------------------------------------------------------------------------
BOOL FASTCALL FDD::IsHD() const
{
	ASSERT(this);

	return fdd.hd;
}

//---------------------------------------------------------------------------
//
//	�A�N�Z�XLED�ݒ�
//
//---------------------------------------------------------------------------
void FASTCALL FDD::Access(BOOL flag)
{
	int i;

	ASSERT(this);

	// ���ׂĉ��낷
	for (i=0; i<4; i++) {
		drv[i].access = FALSE;
	}

	// flag���オ���Ă���΁A�Z���N�g�h���C�u���L��
	if (flag) {
		drv[ fdd.selected ].access = TRUE;
	}
}

//---------------------------------------------------------------------------
//
//	���f�B�`�F�b�N
//
//---------------------------------------------------------------------------
BOOL FASTCALL FDD::IsReady(int drive, BOOL fdc) const
{
	ASSERT(this);
	ASSERT((drive >= 0) && (drive <= 3));

	// FDC����̃A�N�Z�X�Ȃ�
	if (fdc) {
		// �������f�B�Ȃ烌�f�B
		if (fdd.force) {
			return TRUE;
		}

		// ���[�^�I�t�Ȃ�m�b�g���f�B
		if (!fdd.motor) {
			return FALSE;
		}
	}

	// next������΁Anext��D��
	if (drv[drive].next) {
		return drv[drive].next->IsReady();
	}

	// �C���[�W�t�@�C���ɕ���
	return drv[drive].fdi->IsReady();
}

//---------------------------------------------------------------------------
//
//	�������݋֎~�`�F�b�N
//
//---------------------------------------------------------------------------
BOOL FASTCALL FDD::IsWriteP(int drive) const
{
	ASSERT(this);
	ASSERT((drive >= 0) && (drive <= 3));

	// next������΁Anext��D��
	if (drv[drive].next) {
		return drv[drive].next->IsWriteP();
	}

	// �C���[�W�t�@�C���ɕ���
	return drv[drive].fdi->IsWriteP();
}

//---------------------------------------------------------------------------
//
//	Read Only�`�F�b�N
//
//---------------------------------------------------------------------------
BOOL FASTCALL FDD::IsReadOnly(int drive) const
{
	ASSERT(this);
	ASSERT((drive >= 0) && (drive <= 3));

	// next������΁Anext��D��
	if (drv[drive].next) {
		return drv[drive].next->IsReadOnly();
	}

	// �C���[�W�t�@�C���ɕ���
	return drv[drive].fdi->IsReadOnly();
}

//---------------------------------------------------------------------------
//
//	�������݋֎~
//
//---------------------------------------------------------------------------
void FASTCALL FDD::WriteP(int drive, BOOL flag)
{
	ASSERT(this);
	ASSERT((drive >= 0) && (drive <= 3));

	// �C���[�W�t�@�C���ŏ���
	drv[drive].fdi->WriteP(flag);
}

//---------------------------------------------------------------------------
//
//	�h���C�u�X�e�[�^�X�擾
//
//	b7 : Insert
//	b6 : Invalid Insert
//	b5 : Inhibit Eject
//	b4 : Blink (Global)
//	b3 : Blink (Current)
//	b2 : Motor
//	b1 : Select
//	b0 : Access
//---------------------------------------------------------------------------
int FASTCALL FDD::GetStatus(int drive) const
{
	int st;

	ASSERT(this);
	ASSERT((drive >= 0) && (drive <= 3));

	// �N���A
	st = 0;

	// bit7 - �}��
	if (drv[drive].insert) {
		st |= FDST_INSERT;
	}

	// bit6 - ��}��(�}�����܂�)
	if (drv[drive].invalid) {
		st |= FDST_INVALID;
	}

	// bit5 - �C�W�F�N�g�ł���
	if (drv[drive].eject) {
		st |= FDST_EJECT;
	}

	// bit4 - �_��(�O���[�o���A�}�����܂܂Ȃ�)
	if (drv[drive].blink && !drv[drive].insert) {
		st |= FDST_BLINK;

		// bit3 - �_��(����)
		if (rtc->GetBlink(drive)) {
			st |= FDST_CURRENT;
		}
	}

	// bit2 - ���[�^
	if (fdd.motor) {
		st |= FDST_MOTOR;
	}

	// bit1 - �Z���N�g
	if (drive == fdd.selected) {
		st |= FDST_SELECT;
	}

	// bit0 - �A�N�Z�X
	if (drv[drive].access) {
		st |= FDST_ACCESS;
	}

	return st;
}

//---------------------------------------------------------------------------
//
//	���[�^�ݒ�{�h���C�u�Z���N�g
//
//---------------------------------------------------------------------------
void FASTCALL FDD::SetMotor(int drive, BOOL flag)
{
	ASSERT(this);
	ASSERT((drive >= 0) && (drive <= 3));

	// flag=FALSE�Ȃ�ȒP
	if (!flag) {
		// ���ł�OFF�Ȃ�K�v�Ȃ�
		if (!fdd.motor) {
			return;
		}
#if defined(FDD_LOG)
		LOG0(Log::Normal, "���[�^OFF");
#endif	// FDD_LOG

		// ���[�^��~
		fdd.motor = FALSE;
		fdd.settle = FALSE;

		// �Z���N�g�h���C�u�ݒ�
		fdd.selected = drive;

		// �X�^���o�C�C�x���g��ݒ�
		rotation.SetDesc("Standby 54000ms");
		rotation.SetTime(54 * 1000 * 2 * 1000);
		return;
	}

	// �Z���N�g�h���C�u�ݒ�
	fdd.selected = drive;

	// ���[�^ON�܂��̓Z�g�����O���Ȃ���Ȃ�
	if (fdd.motor || fdd.settle) {
		return;
	}

#if defined(FDD_LOG)
	LOG1(Log::Normal, "���[�^ON �h���C�u%d�Z���N�g", drive);
#endif	// FDD_LOG

	// �����ŃC�x���g�������Ă���΁A�X�^���o�C��
	if (rotation.GetTime() != 0) {
		// �����[�^ON
		fdd.settle = FALSE;
		fdd.motor = TRUE;
		fdd.first = TRUE;

		// ��]
		Rotation();
		return;
	}

	// ���[�^OFF�A�Z�g�����O��
	fdd.motor = FALSE;
	fdd.settle = TRUE;

	// �Z�g�����O�C�x���g��ݒ�(�������[�h��64us�A�ʏ탂�[�h��384ms)
	rotation.SetDesc("Settle 384ms");
	if (fdd.fast) {
		rotation.SetTime(128);
	}
	else {
		rotation.SetTime(384 * 1000 * 2);
	}
}

//---------------------------------------------------------------------------
//
//	�V�����_�擾
//
//---------------------------------------------------------------------------
int FASTCALL FDD::GetCylinder(int drive) const
{
	ASSERT(this);
	ASSERT((drive >= 0) && (drive <= 3));

	// ���[�N��Ԃ�
	return drv[drive].cylinder;
}

//---------------------------------------------------------------------------
//
//	�f�B�X�N���擾
//
//---------------------------------------------------------------------------
void FASTCALL FDD::GetName(int drive, char *buf, int media) const
{
	ASSERT(this);
	ASSERT((drive >= 0) && (drive <= 3));
	ASSERT(buf);
	ASSERT((media >= -1) && (media < 0x10));

	// next������΁Anext��D��
	if (drv[drive].next) {
		drv[drive].next->GetName(buf, media);
		return;
	}

	// �C���[�W�ɕ���
	drv[drive].fdi->GetName(buf, media);
}

//---------------------------------------------------------------------------
//
//	�p�X�擾
//
//---------------------------------------------------------------------------
void FASTCALL FDD::GetPath(int drive, Filepath& path) const
{
	ASSERT(this);
	ASSERT((drive >= 0) && (drive <= 3));

	// next������΁Anext��D��
	if (drv[drive].next) {
		drv[drive].next->GetPath(path);
		return;
	}

	// �C���[�W�ɕ���
	drv[drive].fdi->GetPath(path);
}

//---------------------------------------------------------------------------
//
//	�f�B�X�N�����擾
//
//---------------------------------------------------------------------------
int FASTCALL FDD::GetDisks(int drive) const
{
	ASSERT(this);
	ASSERT((drive >= 0) && (drive <= 3));

	// next������΁Anext��D��
	if (drv[drive].next) {
		return drv[drive].next->GetDisks();
	}

	// �C���[�W�ɕ���
	return drv[drive].fdi->GetDisks();
}

//---------------------------------------------------------------------------
//
//	�J�����g���f�B�A�ԍ��擾
//
//---------------------------------------------------------------------------
int FASTCALL FDD::GetMedia(int drive) const
{
	ASSERT(this);
	ASSERT((drive >= 0) && (drive <= 3));

	// next������΁Anext��D��
	if (drv[drive].next) {
		return drv[drive].next->GetMedia();
	}

	// �C���[�W�ɕ���
	return drv[drive].fdi->GetMedia();
}

//---------------------------------------------------------------------------
//
//	���L�����u���[�g
//
//---------------------------------------------------------------------------
void FASTCALL FDD::Recalibrate(DWORD srt)
{
	ASSERT(this);

	// ��v�`�F�b�N
	if (drv[ fdd.selected ].cylinder == 0) {
		fdc->CompleteSeek(fdd.selected, IsReady(fdd.selected));
		return;
	}

#if defined(FDD_LOG)
	LOG1(Log::Normal, "���L�����u���[�g �h���C�u%d", fdd.selected);
#endif	// FDD_LOG

	// fdd[drive].cylinder�����X�e�b�v�A�E�g
	StepOut(drv[ fdd.selected ].cylinder, srt);
}

//---------------------------------------------------------------------------
//
//	�X�e�b�v�C��
//
//---------------------------------------------------------------------------
void FASTCALL FDD::StepIn(int step, DWORD srt)
{
	int cylinder;

	ASSERT(this);
	ASSERT((step >= 0) && (step < 256));

	// �X�e�b�v�C��
	cylinder = drv[ fdd.selected ].cylinder;
	cylinder += step;
	if (cylinder >= 82) {
		cylinder = 81;
	}

	// ��v�`�F�b�N
	if (drv[ fdd.selected ].cylinder == cylinder) {
		// �������荞�݂��o��
		fdc->CompleteSeek(fdd.selected, IsReady(fdd.selected));
		return;
	}

#if defined(FDD_LOG)
	LOG2(Log::Normal, "�X�e�b�v�C�� �h���C�u%d �X�e�b�v%d", fdd.selected, step);
#endif	// FDD_LOG

	// �V�[�N���ʂ�
	SeekInOut(cylinder, srt);
}

//---------------------------------------------------------------------------
//
//	�X�e�b�v�A�E�g
//
//---------------------------------------------------------------------------
void FASTCALL FDD::StepOut(int step, DWORD srt)
{
	int cylinder;

	ASSERT(this);
	ASSERT((step >= 0) && (step < 256));

	// �X�e�b�v�A�E�g
	cylinder = drv[ fdd.selected ].cylinder;
	cylinder -= step;
	if (cylinder < 0) {
		cylinder = 0;
	}

	// ��v�`�F�b�N
	if (drv[ fdd.selected ].cylinder == cylinder) {
		fdc->CompleteSeek(fdd.selected, IsReady(fdd.selected));
		return;
	}

#if defined(FDD_LOG)
	LOG2(Log::Normal, "�X�e�b�v�A�E�g �h���C�u%d �X�e�b�v%d", fdd.selected, step);
#endif	// FDD_LOG

	// �V�[�N���ʂ�
	SeekInOut(cylinder, srt);
}

//---------------------------------------------------------------------------
//
//	�V�[�N����
//
//---------------------------------------------------------------------------
void FASTCALL FDD::SeekInOut(int cylinder, DWORD srt)
{
	int step;

	ASSERT(this);
	ASSERT((cylinder >= 0) && (cylinder < 82));

	// �X�e�b�v�����v�Z
	ASSERT(drv[ fdd.selected ].cylinder != cylinder);
	if (drv[ fdd.selected ].cylinder < cylinder) {
		step = cylinder - drv[ fdd.selected ].cylinder;
	}
	else {
		step = drv[ fdd.selected ].cylinder - cylinder;
	}

	// ��ɃV�[�N������s���Ă��܂�(���s���t���O�͗��Ă�)
	drv[ fdd.selected ].cylinder = cylinder;
	drv[ fdd.selected ].fdi->Seek(cylinder);
	drv[ fdd.selected ].seeking = TRUE;

	// �C�x���g�ݒ�(SRT * �X�e�b�v�� + 0.64ms)
	if (fdd.fast) {
		// �������[�h����64us�Œ�
		seek.SetTime(128);
	}
	else {
		srt *= step;
		srt += 1280;

		// ���[�^ON��̏����128ms��������
		if (fdd.first) {
			srt += (128 * 0x800);
			fdd.first = FALSE;
		}
		seek.SetTime(srt);
	}
}

//---------------------------------------------------------------------------
//
//	���[�hID
//	�����̃X�e�[�^�X��Ԃ�(�G���[��OR�����)
//		FDD_NOERROR		�G���[�Ȃ�
//		FDD_NOTREADY	�m�b�g���f�B
//		FDD_MAM			�w�薧�x�ł̓A���t�H�[�}�b�g
//		FDD_NODATA		�w�薧�x�ł̓A���t�H�[�}�b�g���A
//						�܂��͗L���ȃZ�N�^���ׂ�ID CRC
//
//---------------------------------------------------------------------------
int FASTCALL FDD::ReadID(DWORD *buf, BOOL mfm, int hd)
{
	ASSERT(this);
	ASSERT(buf);
	ASSERT((hd == 0) || (hd == 4));

#if defined(FDD_LOG)
	LOG2(Log::Normal, "���[�hID �h���C�u%d �w�b�h%d", fdd.selected, hd);
#endif	// FDD_LOG

	// �C���[�W�ɔC����
	return drv[ fdd.selected ].fdi->ReadID(buf, mfm, hd);
}

//---------------------------------------------------------------------------
//
//	���[�h�Z�N�^
//	�����̃X�e�[�^�X��Ԃ�(�G���[��OR�����)
//		FDD_NOERROR		�G���[�Ȃ�
//		FDD_NOTREADY	�m�b�g���f�B
//		FDD_MAM			�w�薧�x�ł̓A���t�H�[�}�b�g
//		FDD_NODATA		�w�薧�x�ł̓A���t�H�[�}�b�g���A
//						�܂��͗L���ȃZ�N�^��S��������R����v���Ȃ�
//		FDD_NOCYL		��������ID��C����v�ł��AFF�łȂ��Z�N�^��������
//		FDD_BADCYL		��������ID��C����v�����AFF�ƂȂ��Ă���Z�N�^��������
//		FDD_IDCRC		ID�t�B�[���h��CRC�G���[������
//		FDD_DATACRC		DATA�t�B�[���h��CRC�G���[������
//		FDD_DDAM		�f���[�e�b�h�Z�N�^�ł���
//
//---------------------------------------------------------------------------
int FASTCALL FDD::ReadSector(BYTE *buf, int *len, BOOL mfm, DWORD *chrn, int hd)
{
	ASSERT(this);
	ASSERT(buf);
	ASSERT(len);
	ASSERT(chrn);
	ASSERT((hd == 0) || (hd == 4));

#if defined(FDD_LOG)
	LOG4(Log::Normal, "���[�h�Z�N�^ C:%02X H:%02X R:%02X N:%02X",
						chrn[0], chrn[1], chrn[2], chrn[3]);
#endif	// FDD_LOG

	// �C���[�W�ɔC����
	return drv[ fdd.selected ].fdi->ReadSector(buf, len, mfm, chrn, hd);
}

//---------------------------------------------------------------------------
//
//	���C�g�Z�N�^
//	�����̃X�e�[�^�X��Ԃ�(�G���[��OR�����)
//		FDD_NOERROR		�G���[�Ȃ�
//		FDD_NOTREADY	�m�b�g���f�B
//		FDD_NOTWRITE	���f�B�A�͏������݋֎~
//		FDD_MAM			�w�薧�x�ł̓A���t�H�[�}�b�g
//		FDD_NODATA		�w�薧�x�ł̓A���t�H�[�}�b�g���A
//						�܂��͗L���ȃZ�N�^��S��������R����v���Ȃ�
//		FDD_NOCYL		��������ID��C����v�ł��AFF�łȂ��Z�N�^��������
//		FDD_BADCYL		��������ID��C����v�����AFF�ƂȂ��Ă���Z�N�^��������
//		FDD_IDCRC		ID�t�B�[���h��CRC�G���[������
//		FDD_DDAM		�f���[�e�b�h�Z�N�^�ł���
//
//---------------------------------------------------------------------------
int FASTCALL FDD::WriteSector(const BYTE *buf, int *len, BOOL mfm, DWORD *chrn, int hd, BOOL deleted)
{
	ASSERT(this);
	ASSERT(len);
	ASSERT(chrn);
	ASSERT((hd == 0) || (hd == 4));

#if defined(FDD_LOG)
	LOG4(Log::Normal, "���C�g�Z�N�^ C:%02X H:%02X R:%02X N:%02X",
						chrn[0], chrn[1], chrn[2], chrn[3]);
#endif	// FDD_LOG

	// �C���[�W�ɔC����
	return drv[ fdd.selected ].fdi->WriteSector(buf, len, mfm, chrn, hd, deleted);
}

//---------------------------------------------------------------------------
//
//	���[�h�_�C�A�O
//
//---------------------------------------------------------------------------
int FASTCALL FDD::ReadDiag(BYTE *buf, int *len, BOOL mfm, DWORD *chrn, int hd)
{
	ASSERT(this);
	ASSERT(len);
	ASSERT(chrn);
	ASSERT((hd == 0) || (hd == 4));

#if defined(FDD_LOG)
	LOG4(Log::Normal, "���[�h�_�C�A�O C:%02X H:%02X R:%02X N:%02X",
						chrn[0], chrn[1], chrn[2], chrn[3]);
#endif	// FDD_LOG

	// �C���[�W�ɔC����
	return drv[ fdd.selected ].fdi->ReadDiag(buf, len, mfm, chrn, hd);
}

//---------------------------------------------------------------------------
//
//	���C�gID
//
//---------------------------------------------------------------------------
int FASTCALL FDD::WriteID(const BYTE *buf, DWORD d, int sc, BOOL mfm, int hd, int gpl)
{
	ASSERT(this);
	ASSERT(sc > 0);
	ASSERT((hd == 0) || (hd == 4));

#if defined(FDD_LOG)
	LOG3(Log::Normal, "���C�gID �h���C�u%d �V�����_%d �w�b�h%d",
					fdd.selected, drv[ fdd.selected ].cylinder, hd);
#endif	// FDD_LOG

	// �C���[�W�ɔC����
	return drv[ fdd.selected ].fdi->WriteID(buf, d, sc, mfm, hd, gpl);
}
