//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 �o�h�D(ytanaka@ipc-tokai.or.jp)
//	[ SASI ]
//
//---------------------------------------------------------------------------

#include "os.h"
#include "xm6.h"
#include "vm.h"
#include "log.h"
#include "schedule.h"
#include "fileio.h"
#include "dmac.h"
#include "iosc.h"
#include "sram.h"
#include "config.h"
#include "disk.h"
#include "memory.h"
#include "scsi.h"
#include "sasi.h"

//===========================================================================
//
//	SASI
//
//===========================================================================
//#define SASI_LOG

//---------------------------------------------------------------------------
//
//	�R���X�g���N�^
//
//---------------------------------------------------------------------------
SASI::SASI(VM *p) : MemDevice(p)
{
	// �f�o�C�XID��������
	dev.id = MAKEID('S', 'A', 'S', 'I');
	dev.desc = "SASI (IOSC-2)";

	// �J�n�A�h���X�A�I���A�h���X
	memdev.first = 0xe96000;
	memdev.last = 0xe97fff;
}

//---------------------------------------------------------------------------
//
//	������
//
//---------------------------------------------------------------------------
BOOL FASTCALL SASI::Init()
{
	int i;

	ASSERT(this);

	// ��{�N���X
	if (!MemDevice::Init()) {
		return FALSE;
	}

	// DMAC�擾
	dmac = (DMAC*)vm->SearchDevice(MAKEID('D', 'M', 'A', 'C'));
	ASSERT(dmac);

	// IOSC�擾
	iosc = (IOSC*)vm->SearchDevice(MAKEID('I', 'O', 'S', 'C'));
	ASSERT(iosc);

	// SRAM�擾
	sram = (SRAM*)vm->SearchDevice(MAKEID('S', 'R', 'A', 'M'));
	ASSERT(sram);

	// SCSI�擾
	scsi = (SCSI*)vm->SearchDevice(MAKEID('S', 'C', 'S', 'I'));
	ASSERT(scsi);

	// �����f�[�^������
	memset(&sasi, 0, sizeof(sasi));
	sxsicpu = FALSE;

	// �f�B�X�N�쐬
	for (i=0; i<SASIMax; i++) {
		sasi.disk[i] = new Disk(this);
	}

	// �J�����g�Ȃ�
	sasi.current = NULL;

	// �C�x���g�쐬
	event.SetDevice(this);
	event.SetDesc("Data Transfer");
	event.SetUser(0);
	event.SetTime(0);
	scheduler->AddEvent(&event);

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	�N���[���A�b�v
//
//---------------------------------------------------------------------------
void FASTCALL SASI::Cleanup()
{
	int i;

	ASSERT(this);

	// �f�B�X�N���폜
	for (i=0; i<SASIMax; i++) {
		if (sasi.disk[i]) {
			delete sasi.disk[i];
			sasi.disk[i] = NULL;
		}
	}

	// ��{�N���X��
	MemDevice::Cleanup();
}

//---------------------------------------------------------------------------
//
//	���Z�b�g
//
//---------------------------------------------------------------------------
void FASTCALL SASI::Reset()
{
	Memory *memory;
	Memory::memtype type;

	ASSERT(this);
	LOG0(Log::Normal, "���Z�b�g");

	// SCSI�^�C�v(�������ɖ₢���킹��)
	memory = (Memory*)vm->SearchDevice(MAKEID('M', 'E', 'M', ' '));
	ASSERT(memory);
	type = memory->GetMemType();
	switch (type) {
		// SASI�̂�
		case Memory::None:
		case Memory::SASI:
			sasi.scsi_type = 0;
			break;

		// �O�t
		case Memory::SCSIExt:
			sasi.scsi_type = 1;
			break;

		// ���̑�(����)
		default:
			sasi.scsi_type = 2;
			break;
	}

	// SCSI���g���ꍇ�ASxSI�Ƃ͔r���Ƃ���(SxSI�֎~)
	if (sasi.scsi_type != 0) {
		if (sasi.sxsi_drives != 0) {
			sasi.sxsi_drives = 0;
			Construct();
		}
	}

	// �������X�C�b�`��������
	if (sasi.memsw) {
		// SASI�����݂���ꍇ�̂݁A�Z�b�g
		if (sasi.scsi_type < 2) {
			// $ED005A:SASI�f�B�X�N��
			sram->SetMemSw(0x5a, sasi.sasi_drives);
		}
		else {
			// SASI�C���^�t�F�[�X���Ȃ��̂�0
			sram->SetMemSw(0x5a, 0x00);
		}

		// SCSI�����݂��Ȃ��ꍇ�̂݁A�N���A
		if (sasi.scsi_type == 0) {
			// $ED006F:SCSI�t���O('V'��SCSI�L��)
			sram->SetMemSw(0x6f, 0x00);
			// $ED0070:SCSI���(�{��/�O�t)+�{��SCSI ID
			sram->SetMemSw(0x70, 0x07);
			// $ED0071:SCSI�ɂ��SASI�G�~�����[�V�����t���O
			sram->SetMemSw(0x71, 0x00);
		}
	}

	// �C�x���g���Z�b�g
	event.SetUser(0);
	event.SetTime(0);

	// �o�X���Z�b�g
	BusFree();

	// �J�����g�f�o�C�X�Ȃ�
	sasi.current = NULL;
}

//---------------------------------------------------------------------------
//
//	�Z�[�u
//
//---------------------------------------------------------------------------
BOOL FASTCALL SASI::Save(Fileio *fio, int ver)
{
	size_t sz;
	int i;

	ASSERT(this);
	ASSERT(fio);

	LOG0(Log::Normal, "�Z�[�u");

	// �f�B�X�N���t���b�V��
	for (i=0; i<SASIMax; i++) {
		ASSERT(sasi.disk[i]);
		if (!sasi.disk[i]->Flush()) {
			return FALSE;
		}
	}

	// �T�C�Y���Z�[�u
	sz = sizeof(sasi_t);
	if (!fio->Write(&sz, sizeof(sz))) {
		return FALSE;
	}

	// ���̂��Z�[�u
	if (!fio->Write(&sasi, (int)sz)) {
		return FALSE;
	}

	// �C�x���g���Z�[�u
	if (!event.Save(fio, ver)) {
		return FALSE;
	}

	// �p�X���Z�[�u
	for (i=0; i<SASIMax; i++) {
		if (!sasihd[i].Save(fio, ver)) {
			return FALSE;
		}
	}
	for (i=0; i<SCSIMax; i++) {
		if (!scsihd[i].Save(fio, ver)) {
			return FALSE;
		}
	}
	if (!scsimo.Save(fio, ver)) {
		return FALSE;
	}

	// version2.02�g��
	if (!fio->Write(&sxsicpu, sizeof(sxsicpu))) {
		return FALSE;
	}

	// version2.03�g��
	for (i=0; i<SASIMax; i++) {
		ASSERT(sasi.disk[i]);
		if (!sasi.disk[i]->Save(fio, ver)) {
			return FALSE;
		}
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	���[�h
//
//---------------------------------------------------------------------------
BOOL FASTCALL SASI::Load(Fileio *fio, int ver)
{
	sasi_t buf;
	size_t sz;
	int i;

	ASSERT(this);
	ASSERT(fio);

	LOG0(Log::Normal, "���[�h");

	// �T�C�Y�����[�h�A�ƍ�
	if (!fio->Read(&sz, sizeof(sz))) {
		return FALSE;
	}
	if (sz != sizeof(sasi_t)) {
		return FALSE;
	}

	// �o�b�t�@�֎��̂����[�h
	if (!fio->Read(&buf, (int)sz)) {
		return FALSE;
	}

	// �f�B�X�N��������
	for (i=0; i<SASIMax; i++) {
		ASSERT(sasi.disk[i]);
		delete sasi.disk[i];
		sasi.disk[i] = new Disk(this);
	}

	// �|�C���^���ڂ�
	for (i=0; i<SASIMax; i++) {
		buf.disk[i] = sasi.disk[i];
	}

	// �ړ�
	sasi = buf;
	sasi.mo = NULL;
	sasi.current = NULL;

	// �C�x���g�����[�h
	if (!event.Load(fio, ver)) {
		return FALSE;
	}

	// �p�X�����[�h
	for (i=0; i<SASIMax; i++) {
		if (!sasihd[i].Load(fio, ver)) {
			return FALSE;
		}
	}
	for (i=0; i<SCSIMax; i++) {
		if (!scsihd[i].Load(fio, ver)) {
			return FALSE;
		}
	}
	if (!scsimo.Load(fio, ver)) {
		return FALSE;
	}

	// version2.02�g��
	sxsicpu = FALSE;
	if (ver >= 0x0202) {
		if (!fio->Read(&sxsicpu, sizeof(sxsicpu))) {
			return FALSE;
		}
	}

	// �f�B�X�N����蒼��
	Construct();

	// version2.03�g��
	if (ver >= 0x0203) {
		for (i=0; i<SASIMax; i++) {
			ASSERT(sasi.disk[i]);
			if (!sasi.disk[i]->Load(fio, ver)) {
				return FALSE;
			}
		}
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	�ݒ�K�p
//
//---------------------------------------------------------------------------
void FASTCALL SASI::ApplyCfg(const Config *config)
{
	int i;

	ASSERT(this);
	ASSERT(config);

	LOG0(Log::Normal, "�ݒ�K�p");

	// SASI�h���C�u��
	sasi.sasi_drives = config->sasi_drives;

	// �������X�C�b�`�����X�V
	sasi.memsw = config->sasi_sramsync;

	// SASI�t�@�C����
	for (i=0; i<SASIMax; i++) {
		sasihd[i].SetPath(config->sasi_file[i]);
	}

	// �p���e�B��H�t��
	sasi.parity = config->sasi_parity;

	// SCSI�h���C�u��
	sasi.sxsi_drives = config->sxsi_drives;

	// SCSI�t�@�C����
	for (i=0; i<SCSIMax; i++) {
		scsihd[i].SetPath(config->sxsi_file[i]);
	}

	// MO�D��t���O
	sasi.mo_first = config->sxsi_mofirst;

	// �f�B�X�N�č\�z
	Construct();
}

//---------------------------------------------------------------------------
//
//	�o�C�g�ǂݍ���
//
//---------------------------------------------------------------------------
DWORD FASTCALL SASI::ReadByte(DWORD addr)
{
	DWORD data;

	ASSERT(this);
	ASSERT((addr >= memdev.first) && (addr <= memdev.last));

	// SCSI������
	if (sasi.scsi_type >= 2) {
		// 0x40�P�ʂŃ��[�v
		addr &= 0x3f;

		// �̈�`�F�b�N
		if (addr >= 0x20) {
			// SCSI�̈�
			return scsi->ReadByte(addr - 0x20);
		}
		if ((addr & 1) == 0) {
			// �����o�C�g�̓f�R�[�h����Ă��Ȃ�
			return 0xff;
		}
		addr &= 0x07;
		if (addr >= 4) {
			return 0xff;
		}
		return 0;
	}

	// ��A�h���X�̂�
	if (addr & 1) {
		// 8�o�C�g�P�ʂŃ��[�v
		addr &= 0x07;

		// �E�F�C�g
		scheduler->Wait(1);

		// �f�[�^���W�X�^
		if (addr == 1) {
			return ReadData();
		}

		// �X�e�[�^�X���W�X�^
		if (addr == 3) {
			data = 0;
			if (sasi.msg) {
				data |= 0x10;
			}
			if (sasi.cd) {
				data |= 0x08;
			}
			if (sasi.io) {
				data |= 0x04;
			}
			if (sasi.bsy) {
				data |= 0x02;
			}
			if (sasi.req) {
				data |= 0x01;
			}
			return data;
		}

		// ����ȊO�̃��W�X�^��Write Only
		return 0xff;
	}

	// �����A�h���X�̓f�R�[�h����Ă��Ȃ�
	return 0xff;
}

//---------------------------------------------------------------------------
//
//	���[�h�ǂݍ���
//
//---------------------------------------------------------------------------
DWORD FASTCALL SASI::ReadWord(DWORD addr)
{
	ASSERT(this);
	ASSERT((addr >= memdev.first) && (addr <= memdev.last));
	ASSERT((addr & 1) == 0);

	return (0xff00 | ReadByte(addr + 1));
}

//---------------------------------------------------------------------------
//
//	�o�C�g��������
//
//---------------------------------------------------------------------------
void FASTCALL SASI::WriteByte(DWORD addr, DWORD data)
{
	ASSERT(this);
	ASSERT((addr >= memdev.first) && (addr <= memdev.last));
	ASSERT(data < 0x100);

	// SCSI������
	if (sasi.scsi_type >= 2) {
		// 0x40�P�ʂŃ��[�v
		addr &= 0x3f;

		// �̈�`�F�b�N
		if (addr >= 0x20) {
			// SCSI�̈�
			scsi->WriteByte(addr - 0x20, data);
		}
		// SASI�̈�́A�������Ȃ�
		return;
	}

	// ��A�h���X�̂�
	if (addr & 1) {
		// 8�o�C�g�P�ʂŃ��[�v
		addr &= 0x07;
		addr >>= 1;

		// �E�F�C�g
		scheduler->Wait(1);

		switch (addr) {
			// �f�[�^��������
			case 0:
				WriteData(data);
				return;

			// SEL���f�[�^��������
			case 1:
				sasi.sel = FALSE;
				WriteData(data);
				return;

			// �o�X���Z�b�g
			case 2:
#if defined(SASI_LOG)
				LOG0(Log::Normal, "�o�X���Z�b�g");
#endif	// SASI_LOG
				BusFree();
				return;

			// SEL���f�[�^��������
			case 3:
				sasi.sel = TRUE;
				WriteData(data);
				return;
		}

		// �����ɂ͗��Ȃ�
		ASSERT(FALSE);
		return;
	}

	// �����A�h���X�̓f�R�[�h����Ă��Ȃ�
}

//---------------------------------------------------------------------------
//
//	���[�h��������
//
//---------------------------------------------------------------------------
void FASTCALL SASI::WriteWord(DWORD addr, DWORD data)
{
	ASSERT(this);
	ASSERT((addr >= memdev.first) && (addr <= memdev.last));
	ASSERT((addr & 1) == 0);
	ASSERT(data < 0x10000);

	WriteByte(addr + 1, (BYTE)data);
}

//---------------------------------------------------------------------------
//
//	�ǂݍ��݂̂�
//
//---------------------------------------------------------------------------
DWORD FASTCALL SASI::ReadOnly(DWORD addr) const
{
	DWORD data;

	ASSERT(this);
	ASSERT((addr >= memdev.first) && (addr <= memdev.last));

	// SCSI������
	if (sasi.scsi_type >= 2) {
		// 0x40�P�ʂŃ��[�v
		addr &= 0x3f;

		// �̈�`�F�b�N
		if (addr >= 0x20) {
			// SCSI�̈�
			return scsi->ReadOnly(addr - 0x20);
		}
		if ((addr & 1) == 0) {
			// �����o�C�g�̓f�R�[�h����Ă��Ȃ�
			return 0xff;
		}
		addr &= 0x07;
		if (addr >= 4) {
			return 0xff;
		}
		return 0;
	}

	// ��A�h���X�̂�
	if (addr & 1) {
		// 8�o�C�g�P�ʂŃ��[�v
		addr &= 0x07;
		addr >>= 1;

		switch (addr) {
			// �f�[�^���W�X�^
			case 0:
				return 0;

			// �X�e�[�^�X���W�X�^
			case 1:
				data = 0;
				if (sasi.msg) {
					data |= 0x10;
				}
				if (sasi.cd) {
					data |= 0x08;
				}
				if (sasi.io) {
					data |= 0x04;
				}
				if (sasi.bsy) {
					data |= 0x02;
				}
				if (sasi.req) {
					data |= 0x01;
				}
				return data;
		}

		// ����ȊO�̃��W�X�^��Write Only
		return 0xff;
	}

	// �����A�h���X�̓f�R�[�h����Ă��Ȃ�
	return 0xff;
}

//---------------------------------------------------------------------------
//
//	�C�x���g�R�[���o�b�N
//
//---------------------------------------------------------------------------
BOOL FASTCALL SASI::Callback(Event *ev)
{
	int thres;

	ASSERT(this);
	ASSERT(ev);

	switch (ev->GetUser()) {
		// �ʏ�f�[�^ �z�X�g�����R���g���[��
		case 0:
			// ���Ԓ���
			if (ev->GetTime() != 32) {
				ev->SetTime(32);
			}

			// ���N�G�X�g
			sasi.req = TRUE;
			dmac->ReqDMA(1);

			// DMA�]���̌��ʂɂ���Ă͑���
			return TRUE;

		// �u���b�N�f�[�^ �R���g���[�������z�X�g
		case 1:
			// ���Ԃ��Đݒ�
			if (ev->GetTime() != 48) {
				ev->SetTime(48);
			}

			// DMA���A�N�e�B�u�łȂ���΁A���N�G�X�g�ݒ�̂�
			if (!dmac->IsAct(1)) {
				if ((sasi.phase == read) || (sasi.phase == write)) {
					sasi.req = TRUE;
				}

				// �C�x���g�p��
				return TRUE;
			}

			// 1��̃C�x���g�ŁA�]��CPU�p���[��2/3�����]������
			thres = (int)scheduler->GetCPUSpeed();
			thres = (thres * 2) / 3;
			while ((sasi.phase == read) || (sasi.phase == write)) {
				// CPU�p���[�����Ȃ���r���őł��؂�
				if (scheduler->GetCPUCycle() > thres) {
					break;
				}

				// ���N�G�X�g�ݒ�
				sasi.req = TRUE;

				// SxSI CPU�t���O���ݒ肳��Ă���΁A�����܂�(CPU�]��������)
				if (sxsicpu) {
					LOG0(Log::Warning, "SxSI CPU�]�������o");
					break;
				}

				// DMA���N�G�X�g
				dmac->ReqDMA(1);
				if (sasi.req) {
#if defined(SASI_LOG)
					LOG0(Log::Normal, "DMA or CPU�]���҂�");
#endif
					break;
				}
			}
			return TRUE;

		// ����ȊO�͂��肦�Ȃ�
		default:
			ASSERT(FALSE);
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	�����f�[�^�擾
//
//---------------------------------------------------------------------------
void FASTCALL SASI::GetSASI(sasi_t *buffer) const
{
	ASSERT(this);
	ASSERT(buffer);

	// �������[�N���R�s�[
	*buffer = sasi;
}

//---------------------------------------------------------------------------
//
//	�f�B�X�N�č\�z
//
//---------------------------------------------------------------------------
void FASTCALL SASI::Construct()
{
	int i;
	int index;
	int scmo;
	int schd;
	int map[SASIMax];
	Filepath path;
	Filepath mopath;
	BOOL moready;
	BOOL mowritep;
	BOOL moattn;
	SASIHD *sasitmp;
	SCSIHD *scsitmp;

	ASSERT(this);
	ASSERT((sasi.sasi_drives >= 0) && (sasi.sasi_drives <= SASIMax));
	ASSERT((sasi.sxsi_drives >= 0) && (sasi.sxsi_drives <= 7));

	// MO�h���C�u�̏�Ԃ�ۑ����A��U�֎~
	moready = FALSE;
	moattn = FALSE;
	mowritep = FALSE;
	if (sasi.mo) {
		moready = sasi.mo->IsReady();
		if (moready) {
			sasi.mo->GetPath(mopath);
			mowritep = sasi.mo->IsWriteP();
			moattn = sasi.mo->IsAttn();
		}
	}
	sasi.mo = NULL;

	// �L����SCSI-MO, SCSI-HD�����Z�o
	i = (sasi.sasi_drives + 1) >> 1;
	schd = 7 - i;

	// SASI��������\�ȍő�SCSI�h���C�u��
	if (schd > 0) {
		// �ݒ��SCSI�h���C�u���ɐ��������
		if (sasi.sxsi_drives < schd) {
			// SCSI�h���C�u���m��
			schd = sasi.sxsi_drives;
		}
	}

	// MO�̊��蓖��
	if (schd > 0) {
		// �h���C�u��1��ȏ゠��΁A�K��MO�����蓖�Ă�
		scmo = 1;
		schd--;
	}
	else {
		// �h���C�u���Ȃ��̂ŁAMO,HD�Ƃ�0
		scmo = 0;
		schd = 0;
	}

	// �p���e�B���Ȃ����SxSI�g�p�s��
	if (!sasi.parity) {
		scmo = 0;
		schd = 0;
	}
#if defined(SASI_LOG)
	LOG3(Log::Normal, "�Ċ��蓖�� SASI-HD:%d�� SCSI-HD:%d�� SCSI-MO:%d��",
					sasi.sasi_drives, schd, scmo);
#endif	// SASI_LOG

	// �}�b�v���쐬(0:NULL 1:SASI-HD 2:SCSI-HD 3:SCSI-MO)
	for (i=0; i<SASIMax; i++) {
		// ���ׂ�NULL
		map[i] = 0;
	}
	for (i=0; i<sasi.sasi_drives; i++) {
		// ���SASI
		map[i] = 1;
	}
	if (scmo > 0) {
		// SCSI����
		index = ((sasi.sasi_drives + 1) >> 1) << 1;
		if (sasi.mo_first) {
			// MO�D��
			map[index] = 3;
			index += 2;

			// SCSI-HD���[�v
			for (i=0; i<schd; i++) {
				map[index] = 2;
				index += 2;
			}
		}
		else {
			// HD�D��
			for (i=0; i<schd; i++) {
				map[index] = 2;
				index += 2;
			}

			// �Ō��MO
			map[index] = 3;
		}
		ASSERT(index <= 16);
	}

	// SCSI�n�[�h�f�B�X�N�̘A�ԃC���f�b�N�X���N���A
	index = 0;

	// ���[�v
	for (i=0; i<SASIMax; i++) {
		switch (map[i]) {
			// �k���f�B�X�N
			case 0:
				// �k���f�B�X�N��
				if (!sasi.disk[i]->IsNULL()) {
					// �k���f�B�X�N�ɍ����ւ���
					delete sasi.disk[i];
					sasi.disk[i] = new Disk(this);
				}
				break;

			// SASI�n�[�h�f�B�X�N
			case 1:
				// SASI�n�[�h�f�B�X�N��
				if (sasi.disk[i]->IsSASI()) {
					// �p�X���擾���A��v�����ok
					sasi.disk[i]->GetPath(path);
					if (path.CmpPath(sasihd[i])) {
						// �p�X����v���Ă���
						break;
					}
				}

				// SASI�n�[�h�f�B�X�N���쐬���ăI�[�v�������݂�
				sasitmp = new SASIHD(this);
				if (sasitmp->Open(sasihd[i])) {
					// LUN�ݒ�
					sasitmp->SetLUN(i & 1);
					// ����ւ�
					delete sasi.disk[i];
					sasi.disk[i] = sasitmp;
				}
				else {
					// �G���[
					delete sasitmp;
					delete sasi.disk[i];
					sasi.disk[i] = new Disk(this);
				}
				break;

			// SCSI�n�[�h�f�B�X�N
			case 2:
				// SCSI�n�[�h�f�B�X�N��
				if (sasi.disk[i]->GetID() == MAKEID('S', 'C', 'H', 'D')) {
					// �p�X���擾���A��v�����ok
					sasi.disk[i]->GetPath(path);
					if (path.CmpPath(scsihd[index])) {
						// �p�X����v���Ă���
						index++;
						break;
					}
				}

				// SCSI�n�[�h�f�B�X�N���쐬���ăI�[�v�������݂�
				scsitmp = new SCSIHD(this);
				if (scsitmp->Open(scsihd[index])) {
					// ����ւ�
					delete sasi.disk[i];
					sasi.disk[i] = scsitmp;
				}
				else {
					// �G���[
					delete scsitmp;
					delete sasi.disk[i];
					sasi.disk[i] = new Disk(this);
				}
				index++;
				break;

			// SCSI�����C�f�B�X�N
			case 3:
				// SCSI�����C�f�B�X�N��
				if (sasi.disk[i]->GetID() == MAKEID('S', 'C', 'M', 'O')) {
					// �L��
					sasi.mo = (SCSIMO*)sasi.disk[i];
				}
				else {
					// SCSI�����C�f�B�X�N���쐬���ċL��
					delete sasi.disk[i];
					sasi.disk[i] = new SCSIMO(this);
					sasi.mo = (SCSIMO*)sasi.disk[i];
				}

				// �����p����Ă��Ȃ���΁A�ăI�[�v��
				if (moready) {
					if (!sasi.mo->IsReady()) {
						if (sasi.mo->Open(mopath, moattn)) {
							sasi.mo->WriteP(mowritep);
						}
					}
				}
				break;

			// ���̑�(���蓾�Ȃ�)
			default:
				ASSERT(FALSE);
				break;
		}
	}

	// BUSY���Ȃ�Asasi.current���X�V
	if (sasi.bsy) {
		// �h���C�u�쐬
		ASSERT(sasi.ctrl < 8);
		index = sasi.ctrl << 1;
		if (sasi.cmd[1] & 0x20) {
			index++;
		}

		// �J�����g�ݒ�
		sasi.current = sasi.disk[index];
		ASSERT(sasi.current);
	}
}

//---------------------------------------------------------------------------
//
//	HD BUSY�`�F�b�N
//
//---------------------------------------------------------------------------
BOOL FASTCALL SASI::IsBusy() const
{
	ASSERT(this);

	// SASI
	if (sasi.bsy) {
		return TRUE;
	}

	// SCSI
	if (scsi->IsBusy()) {
		return TRUE;
	}

	// �ǂ����FALSE
	return FALSE;
}

//---------------------------------------------------------------------------
//
//	BUSY�f�o�C�X�擾
//
//---------------------------------------------------------------------------
DWORD FASTCALL SASI::GetBusyDevice() const
{
	ASSERT(this);

	// SASI
	if (sasi.bsy) {
		// �Z���N�V�����t�F�[�Y�I��������s�t�F�[�Y�J�n�܂ł̊Ԃ�
		// �A�N�Z�X�Ώۃf�o�C�X���m�肵�Ȃ�
		if (!sasi.current) {
			return 0;
		}
		if (sasi.current->IsNULL()) {
			return 0;
		}

		return sasi.current->GetID();
	}

	// SCSI
	return scsi->GetBusyDevice();
}

//---------------------------------------------------------------------------
//
//	�f�[�^�ǂݍ���
//
//---------------------------------------------------------------------------
DWORD FASTCALL SASI::ReadData()
{
	DWORD data;

	ASSERT(this);

	switch (sasi.phase) {
		// �X�e�[�^�X�t�F�[�Y�Ȃ�A�X�e�[�^�X��n���ă��b�Z�[�W�t�F�[�Y��
		case status:
			data = sasi.status;
			sasi.req = FALSE;

			// ���荞�݃��Z�b�g�����˂�
			iosc->IntHDC(FALSE);
			Message();

			// �C�x���g��~
			event.SetTime(0);
			return data;

		// ���b�Z�[�W�t�F�[�Y�Ȃ�A���b�Z�[�W��n���ăo�X�t���[��
		case message:
			data = sasi.message;
			sasi.req = FALSE;
			BusFree();

			// �C�x���g��~
			event.SetTime(0);
			return data;

		// �ǂݍ��݃t�F�[�Y�Ȃ�A�]����X�e�[�^�X�t�F�[�Y��
		case read:
			// Non-DMA�]���Ȃ�ASxSI CPU�t���O���g�O���ω�������
			if (!dmac->IsDMA()) {
#if defined(SASI_LOG)
				LOG1(Log::Normal, "�f�[�^IN�t�F�[�Y CPU�]�� �I�t�Z�b�g=%d", sasi.offset);
#endif	// SASI_LOG
				sxsicpu = !sxsicpu;
			}

			data = (DWORD)sasi.buffer[sasi.offset];
			sasi.offset++;
			sasi.length--;
			sasi.req = FALSE;

			// �����O�X�`�F�b�N
			if (sasi.length == 0) {
				// �]�����ׂ��u���b�N����������
				sasi.blocks--;

				// ����ŏI����
				if (sasi.blocks == 0) {
					// �C�x���g��~�A�X�e�[�^�X�t�F�[�Y
					event.SetTime(0);
					Status();
					return data;
				}

				// ���̃u���b�N��ǂ�
				sasi.length = sasi.current->Read(sasi.buffer, sasi.next);
				if (sasi.length <= 0) {
					// �G���[
					Error();
					return data;
				}

				// ���̃u���b�Nok�A���[�N�ݒ�
				sasi.offset = 0;
				sasi.next++;
			}

			// �ǂݍ��݌p��
			return data;
	}

	// �C�x���g��~
	event.SetTime(0);

	// ����ȊO�̃I�y���[�V�����͑z�肹��
	if (sasi.phase == busfree) {
#if defined(SASI_LOG)
		LOG0(Log::Normal, "�o�X�t���[��Ԃł̓ǂݍ��݁B0��Ԃ�");
#endif	// SASI_LOG
		return 0;
	}

	LOG0(Log::Warning, "�z�肵�Ă��Ȃ��f�[�^�ǂݍ���");
	BusFree();
	return 0;
}

//---------------------------------------------------------------------------
//
//	�f�[�^��������
//
//---------------------------------------------------------------------------
void FASTCALL SASI::WriteData(DWORD data)
{
	ASSERT(this);
	ASSERT(data < 0x100);

	switch (sasi.phase) {
		// �o�X�t���[�t�F�[�Y�Ȃ�A���ɋ������̂̓Z���N�V�����t�F�[�Y
		case busfree:
			if (sasi.sel) {
				Selection(data);
			}
#if defined(SASI_LOG)
			else {
				LOG0(Log::Normal, "�o�X�t���[��ԁASEL�����ł̏������݁B����");
			}
#endif	// SASI_LOG
			event.SetTime(0);
			return;

		// �Z���N�V�����t�F�[�Y�Ȃ�ASEL��߂��ăR�}���h�t�F�[�Y��
		case selection:
			if (!sasi.sel) {
				Command();
				return;
			}
			event.SetTime(0);
			break;

		// �R�}���h�t�F�[�Y�Ȃ�A6/10�o�C�g��M
		case command:
			// �ŏ��̃f�[�^(�I�t�Z�b�g0)�ɂ�背���O�X���Đݒ�
			sasi.cmd[sasi.offset] = data;
			if (sasi.offset == 0) {
				if ((data >= 0x20) && (data <= 0x3f)) {
					// 10�o�C�gCDB
					sasi.length = 10;
				}
			}
			sasi.offset++;
			sasi.length--;
			sasi.req = FALSE;

			// �I��肩
			if (sasi.length == 0) {
				// �C�x���g��~���Ď��s�t�F�[�Y��
				event.SetTime(0);
				Execute();
				return;
			}
			// �C�x���g�p��
			return;

		// �������݃t�F�[�Y�Ȃ�A�]����X�e�[�^�X�t�F�[�Y��
		case write:
			// Non-DMA�]���Ȃ�ASxSI CPU�t���O���g�O���ω�������
			if (!dmac->IsDMA()) {
#if defined(SASI_LOG)
				LOG1(Log::Normal, "�f�[�^OUT�t�F�[�Y CPU�]�� �I�t�Z�b�g=%d", sasi.offset);
#endif	// SASI_LOG
				sxsicpu = !sxsicpu;
			}

			sasi.buffer[sasi.offset] = (BYTE)data;
			sasi.offset++;
			sasi.length--;
			sasi.req = FALSE;

			// �I��肩
			if (sasi.length > 0) {
				return;
			}

			// WRITE(6)�EWRITE(10)�EWRITE&VERIFY�͕ʏ���
			switch (sasi.cmd[0]) {
				case 0x0a:
				case 0x2a:
				case 0x2e:
					break;
				// WRITE DATA�ȊO�̃R�}���h�͂����ŏI��
				default:
					event.SetTime(0);
					Status();
					return;
			}

			// WRITE�R�}���h�Ȃ�A���݂̃o�b�t�@�ŏ�������
			if (!sasi.current->Write(sasi.buffer, sasi.next - 1)) {
				// �G���[
				Error();
				return;
			}

			// �]���u���b�N����������
			sasi.blocks--;

			// �Ō�̃u���b�N��
			if (sasi.blocks == 0) {
				// �C�x���g��~���ăX�e�[�^�X�t�F�[�Y��
				event.SetTime(0);
				Status();
				return;
			}

			// ���̃u���b�N����
			sasi.length = sasi.current->WriteCheck(sasi.next);
			if (sasi.length <= 0) {
				// �G���[
				Error();
				return;
			}

			// ���̃u���b�N��]��
			sasi.next++;
			sasi.offset = 0;

			// �C�x���g�p��
			return;
	}

	// �C�x���g��~
	event.SetTime(0);

	// ����ȊO�̃I�y���[�V�����͑z�肹��
	LOG1(Log::Warning, "�z�肵�Ă��Ȃ��f�[�^�������� $%02X", data);
	BusFree();
}

//---------------------------------------------------------------------------
//
//	�o�X�t���[�t�F�[�Y
//
//---------------------------------------------------------------------------
void FASTCALL SASI::BusFree()
{
	ASSERT(this);

#if defined(SASI_LOG)
	LOG0(Log::Normal, "�o�X�t���[�t�F�[�Y");
#endif	// SASI_LOG

	// �o�X���Z�b�g
	sasi.msg = FALSE;
	sasi.cd = FALSE;
	sasi.io = FALSE;
	sasi.bsy = FALSE;
	sasi.req = FALSE;

	// �o�X�t���[�t�F�[�Y
	sasi.phase = busfree;

	// �C�x���g���~
	event.SetTime(0);

	// ���荞�݃��Z�b�g
	iosc->IntHDC(FALSE);

	// SxSI CPU�]���t���O
	sxsicpu = FALSE;
}

//---------------------------------------------------------------------------
//
//	�Z���N�V�����t�F�[�Y
//
//---------------------------------------------------------------------------
void FASTCALL SASI::Selection(DWORD data)
{
	int c;
	int i;

	ASSERT(this);

	// �Z���N�g���ꂽ�R���g���[����m��
	c = 8;
	for (i=0; i<8; i++) {
		if (data & 1) {
			c = i;
			break;
		}
		data >>= 1;
	}

	// �r�b�g���Ȃ���΃o�X�t���[�t�F�[�Y
	if (c >= 8) {
		BusFree();
		return;
	}

#if defined(SASI_LOG)
	LOG1(Log::Normal, "�Z���N�V�����t�F�[�Y �R���g���[��%d", c);
#endif	// SASI_LOG

	// �h���C�u�����݂��Ȃ���΃o�X�t���[�t�F�[�Y
	if (sasi.disk[(c << 1) + 0]->IsNULL()) {
		if (sasi.disk[(c << 1) + 1]->IsNULL()) {
			BusFree();
			return;
		}
	}

	// BSY���グ�ăZ���N�V�����t�F�[�Y
	sasi.ctrl = (DWORD)c;
	sasi.phase = selection;
	sasi.bsy = TRUE;
}

//---------------------------------------------------------------------------
//
//	�R�}���h�t�F�[�Y
//
//---------------------------------------------------------------------------
void FASTCALL SASI::Command()
{
	ASSERT(this);

#if defined(SASI_LOG)
	LOG0(Log::Normal, "�R�}���h�t�F�[�Y");
#endif	// SASI_LOG

	// �R�}���h�t�F�[�Y
	sasi.phase = command;

	// I/O=0, C/D=1, MSG=0
	sasi.io = FALSE;
	sasi.cd = TRUE;
	sasi.msg = FALSE;

	// �R�}���h�c�蒷����6�o�C�g(�ꕔ�R�}���h��10�o�C�g�BWriteByte�ōĐݒ�)
	sasi.offset = 0;
	sasi.length = 6;

	// �R�}���h�f�[�^��v��
	event.SetUser(0);
	event.SetTime(32);
}

//---------------------------------------------------------------------------
//
//	���s�t�F�[�Y
//
//---------------------------------------------------------------------------
void FASTCALL SASI::Execute()
{
	int drive;

	ASSERT(this);

	// ���s�t�F�[�Y
	sasi.phase = execute;

	// �h���C�u�쐬
	ASSERT(sasi.ctrl < 8);
	drive = sasi.ctrl << 1;
	if (sasi.cmd[1] & 0x20) {
		drive++;
	}

	// �J�����g�ݒ�
	sasi.current = sasi.disk[drive];
	ASSERT(sasi.current);

#if defined(SASI_LOG)
	LOG2(Log::Normal, "���s�t�F�[�Y �R�}���h$%02X �h���C�u%d", sasi.cmd[0], drive);
#endif	// SASI_LOG

#if 0
	// �A�e���V�����Ȃ�G���[���o��(MO�̃��f�B�A����ւ���ʒm)
	if (sasi.current->IsAttn()) {
#if defined(SASI_LOG)
	LOG0(Log::Normal, "���f�B�A�����ʒm�̂��߂̃A�e���V�����G���[");
#endif	// SASI_LOG

		// �A�e���V�������N���A
		sasi.current->ClrAttn();

		// �G���[(���N�G�X�g�Z���X��v��)
		Error();
		return;
	}
#endif

	// 0x12�ȏ�0x2f�ȉ��̃R�}���h�́ASCSI��p
	if ((sasi.cmd[0] >= 0x12) && (sasi.cmd[0] <= 0x2f)) {
		ASSERT(sasi.current);
		if (sasi.current->IsSASI()) {
			// SASI�Ȃ̂ŁA�������R�}���h�G���[
			sasi.current->InvalidCmd();
			Error();
			return;
		}
	}

	// �R�}���h�ʏ���
	switch (sasi.cmd[0]) {
		// TEST UNIT READY
		case 0x00:
			TestUnitReady();
			return;

		// REZERO UNIT
		case 0x01:
			Rezero();
			return;

		// REQUEST SENSE
		case 0x03:
			RequestSense();
			return;

		// FORMAT UNIT
		case 0x04:
			Format();
			return;

		// FORMAT UNIT
		case 0x06:
			Format();
			return;

		// REASSIGN BLOCKS
		case 0x07:
			Reassign();
			return;

		// READ(6)
		case 0x08:
			Read6();
			return;

		// WRITE(6)
		case 0x0a:
			Write6();
			return;

		// SEEK(6)
		case 0x0b:
			Seek6();
			return;

		// ASSIGN(SASI�̂�)
		case 0x0e:
			Assign();
			return;

		// INQUIRY(SCSI�̂�)
		case 0x12:
			Inquiry();
			return;

		// MODE SENSE(SCSI�̂�)
		case 0x1a:
			ModeSense();
			return;

		// START STOP UNIT(SCSI�̂�)
		case 0x1b:
			StartStop();
			return;

		// PREVENT/ALLOW MEDIUM REMOVAL(SCSI�̂�)
		case 0x1e:
			Removal();
			return;

		// READ CAPACITY(SCSI�̂�)
		case 0x25:
			ReadCapacity();
			return;

		// READ(10)(SCSI�̂�)
		case 0x28:
			Read10();
			return;

		// WRITE(10)(SCSI�̂�)
		case 0x2a:
			Write10();
			return;

		// SEEK(10)(SCSI�̂�)
		case 0x2b:
			Seek10();
			return;

		// WRITE and VERIFY(SCSI�̂�)
		case 0x2e:
			Write10();
			return;

		// VERIFY(SCSI�̂�)
		case 0x2f:
			Verify();
			return;

		// SPECIFY(SASI�̂�)
		case 0xc2:
			Specify();
			return;
	}

	// ����ȊO�͑Ή����Ă��Ȃ�
	LOG1(Log::Warning, "���Ή��R�}���h $%02X", sasi.cmd[0]);
	sasi.current->InvalidCmd();

	// �G���[
	Error();
}

//---------------------------------------------------------------------------
//
//	�X�e�[�^�X�t�F�[�Y
//
//---------------------------------------------------------------------------
void FASTCALL SASI::Status()
{
	ASSERT(this);

#if defined(SASI_LOG)
	LOG1(Log::Normal, "�X�e�[�^�X�t�F�[�Y �X�e�[�^�X$%02X", sasi.status);
#endif	// SASI_LOG

	// �X�e�[�^�X�t�F�[�Y
	sasi.phase = status;

	// I/O=1 C/D=1 MSG=0
	sasi.io = TRUE;
	sasi.cd = TRUE;
	ASSERT(!sasi.msg);

	// �X�e�[�^�X�f�[�^�̈�������v��
	sasi.req = TRUE;

	// ���荞�݃��N�G�X�g
	iosc->IntHDC(TRUE);
}

//---------------------------------------------------------------------------
//
//	���b�Z�[�W�t�F�[�Y
//
//---------------------------------------------------------------------------
void FASTCALL SASI::Message()
{
	ASSERT(this);

#if defined(SASI_LOG)
	LOG1(Log::Normal, "���b�Z�[�W�t�F�[�Y ���b�Z�[�W$%02X", sasi.message);
#endif	// SASI_LOG

	// ���b�Z�[�W�t�F�[�Y
	sasi.phase = message;

	// I/O=1 C/D=1 MSG=1
	ASSERT(sasi.io);
	ASSERT(sasi.cd);
	sasi.msg = 1;

	// �X�e�[�^�X�f�[�^�̈�������v��
	sasi.req = TRUE;
}

//---------------------------------------------------------------------------
//
//	���ʃG���[
//
//---------------------------------------------------------------------------
void FASTCALL SASI::Error()
{
	ASSERT(this);

#if defined(SASI_LOG)
	LOG0(Log::Normal, "�G���[(�X�e�[�^�X�t�F�[�Y��)");
#endif	// SASI_LOG

	// �X�e�[�^�X�ƃ��b�Z�[�W��ݒ�(CHECK CONDITION)
	sasi.status = (sasi.current->GetLUN() << 5) | 0x02;
	sasi.message = 0x00;

	// �C�x���g��~
	event.SetTime(0);

	// �X�e�[�^�X�t�F�[�Y
	Status();
}

//---------------------------------------------------------------------------
//
//	TEST UNIT READY
//
//---------------------------------------------------------------------------
void FASTCALL SASI::TestUnitReady()
{
	BOOL status;

	ASSERT(this);
	ASSERT(sasi.current);

#if defined(SASI_LOG)
	LOG0(Log::Normal, "TEST UNIT READY�R�}���h");
#endif	// SASI_LOG

	// �h���C�u�ŃR�}���h����
	status = sasi.current->TestUnitReady(sasi.cmd);
	if (!status) {
		// ���s(�G���[)
		Error();
		return;
	}

	// ����
	sasi.status = 0x00;
	sasi.message = 0x00;

	// �X�e�[�^�X�t�F�[�Y
	Status();
}

//---------------------------------------------------------------------------
//
//	REZERO UNIT
//
//---------------------------------------------------------------------------
void FASTCALL SASI::Rezero()
{
	BOOL status;

	ASSERT(this);
	ASSERT(sasi.current);

#if defined(SASI_LOG)
	LOG0(Log::Normal, "REZERO�R�}���h");
#endif	// SASI_LOG

	// �h���C�u�ŃR�}���h����
	status = sasi.current->Rezero(sasi.cmd);
	if (!status) {
		// ���s(�G���[)
		Error();
		return;
	}

	// ����
	sasi.status = 0x00;
	sasi.message = 0x00;

	// �X�e�[�^�X�t�F�[�Y
	Status();
}

//---------------------------------------------------------------------------
//
//	REQUEST SENSE
//
//---------------------------------------------------------------------------
void FASTCALL SASI::RequestSense()
{
	ASSERT(this);
	ASSERT(sasi.current);

#if defined(SASI_LOG)
	LOG0(Log::Normal, "REQUEST SENSE�R�}���h");
#endif	// SASI_LOG

	// �h���C�u�ŃR�}���h����
	sasi.length = sasi.current->RequestSense(sasi.cmd, sasi.buffer);
	if (sasi.length <= 0) {
		// ���s(�G���[)
		Error();
		return;
	}

#if defined(SASI_LOG)
	LOG1(Log::Normal, "�Z���X�L�[ $%02X", sasi.buffer[2]);
#endif

	// �h���C�u����ԋp���ꂽ�f�[�^��ԐM�BI/O=1, C/D=0
	sasi.offset = 0;
	sasi.blocks = 1;
	sasi.phase = read;
	sasi.io = TRUE;
	sasi.cd = FALSE;

	// �G���[�X�e�[�^�X���N���A
	sasi.status = 0x00;
	sasi.message = 0x00;

	// �C�x���g�ݒ�A�f�[�^�̈�������v��
	event.SetUser(0);
	event.SetTime(48);
}

//---------------------------------------------------------------------------
//
//	FORMAT UNIT
//
//---------------------------------------------------------------------------
void FASTCALL SASI::Format()
{
	BOOL status;

	ASSERT(this);
	ASSERT(sasi.current);

#if defined(SASI_LOG)
	LOG0(Log::Normal, "FORMAT UNIT�R�}���h");
#endif	// SASI_LOG

	// �h���C�u�ŃR�}���h����
	status = sasi.current->Format(sasi.cmd);
	if (!status) {
		// ���s(�G���[)
		Error();
		return;
	}

	// ����
	sasi.status = 0x00;
	sasi.message = 0x00;

	// �X�e�[�^�X�t�F�[�Y
	Status();
}

//---------------------------------------------------------------------------
//
//	REASSIGN BLOCKS
//	��SASI�̂�(�K�i���SCSI������)
//
//---------------------------------------------------------------------------
void FASTCALL SASI::Reassign()
{
	BOOL status;

	ASSERT(this);
	ASSERT(sasi.current);

#if defined(SASI_LOG)
	LOG0(Log::Normal, "REASSIGN BLOCKS�R�}���h");
#endif	// SASI_LOG

	// SASI�ȊO�̓G���[�Ƃ���(���׃��X�g�𑗐M���Ȃ�����)
	if (!sasi.current->IsSASI()) {
		sasi.current->InvalidCmd();
		Error();
		return;
	}

	// �h���C�u�ŃR�}���h����
	status = sasi.current->Reassign(sasi.cmd);
	if (!status) {
		// ���s(�G���[)
		Error();
		return;
	}

	// ����
	sasi.status = 0x00;
	sasi.message = 0x00;

	// �X�e�[�^�X�t�F�[�Y
	Status();
}

//---------------------------------------------------------------------------
//
//	READ(6)
//
//---------------------------------------------------------------------------
void FASTCALL SASI::Read6()
{
	DWORD record;

	ASSERT(this);
	ASSERT(sasi.current);

	// ���R�[�h�ԍ��ƃu���b�N�����擾
	record = sasi.cmd[1] & 0x1f;
	record <<= 8;
	record |= sasi.cmd[2];
	record <<= 8;
	record |= sasi.cmd[3];
	sasi.blocks = sasi.cmd[4];
	if (sasi.blocks == 0) {
		sasi.blocks = 0x100;
	}

#if defined(SASI_LOG)
	LOG2(Log::Normal, "READ(6)�R�}���h ���R�[�h=%06X �u���b�N=%d", record, sasi.blocks);
#endif	// SASI_LOG

	// �h���C�u�ŃR�}���h����
	sasi.length = sasi.current->Read(sasi.buffer, record);
	if (sasi.length <= 0) {
		// ���s(�G���[)
		Error();
		return;
	}

	// �X�e�[�^�X��OK��
	sasi.status = 0x00;
	sasi.message = 0x00;

	// ���̃u���b�Nok�A���[�N�ݒ�
	sasi.offset = 0;
	sasi.next = record + 1;

	// �ǂݍ��݃t�F�[�Y, I/O=1, C/D=0
	sasi.phase = read;
	sasi.io = TRUE;
	sasi.cd = FALSE;

	// �C�x���g��ݒ�A�f�[�^�̈�������v��
	event.SetUser(1);
	event.SetTime(48);
}

//---------------------------------------------------------------------------
//
//	WRITE(6)
//
//---------------------------------------------------------------------------
void FASTCALL SASI::Write6()
{
	DWORD record;

	ASSERT(this);
	ASSERT(sasi.current);

	// ���R�[�h�ԍ��ƃu���b�N�����擾
	record = sasi.cmd[1] & 0x1f;
	record <<= 8;
	record |= sasi.cmd[2];
	record <<= 8;
	record |= sasi.cmd[3];
	sasi.blocks = sasi.cmd[4];
	if (sasi.blocks == 0) {
		sasi.blocks = 0x100;
	}

#if defined(SASI_LOG)
	LOG2(Log::Normal, "WRITE(6)�R�}���h ���R�[�h=%06X �u���b�N=%d", record, sasi.blocks);
#endif	// SASI_LOG

	// �h���C�u�ŃR�}���h����
	sasi.length = sasi.current->WriteCheck(record);
	if (sasi.length <= 0) {
		// ���s(�G���[)
		Error();
		return;
	}

	// �X�e�[�^�X��OK��
	sasi.status = 0x00;
	sasi.message = 0x00;

	// �]�����[�N
	sasi.next = record + 1;
	sasi.offset = 0;

	// �������݃t�F�[�Y, C/D=0
	sasi.phase = write;
	sasi.cd = FALSE;

	// �C�x���g��ݒ�A�f�[�^�̏������݂�v��
	event.SetUser(1);
	event.SetTime(48);
}

//---------------------------------------------------------------------------
//
//	SEEK(6)
//
//---------------------------------------------------------------------------
void FASTCALL SASI::Seek6()
{
	BOOL status;

	ASSERT(this);
	ASSERT(sasi.current);

#if defined(SASI_LOG)
	LOG0(Log::Normal, "SEEK(6)�R�}���h");
#endif	// SASI_LOG

	// �h���C�u�ŃR�}���h����
	status = sasi.current->Seek(sasi.cmd);
	if (!status) {
		// ���s(�G���[)
		Error();
		return;
	}

	// ����
	sasi.status = 0x00;
	sasi.message = 0x00;

	// �X�e�[�^�X�t�F�[�Y
	Status();
}

//---------------------------------------------------------------------------
//
//	ASSIGN
//	��SASI�̂�
//
//---------------------------------------------------------------------------
void FASTCALL SASI::Assign()
{
	ASSERT(this);
	ASSERT(sasi.current);

#if defined(SASI_LOG)
	LOG0(Log::Normal, "ASSIGN�R�}���h");
#endif	// SASI_LOG

	// SASI�ȊO�̓G���[(�x���_���j�[�N�R�}���h)
	if (!sasi.current->IsSASI()) {
		sasi.current->InvalidCmd();
		Error();
		return;
	}

	// �X�e�[�^�X��OK��
	sasi.status = 0x00;
	sasi.message = 0x00;

	// 4�o�C�g�̃f�[�^�����N�G�X�g
	sasi.phase = write;
	sasi.length = 4;
	sasi.offset = 0;

	// C/D��0(�f�[�^��������)
	sasi.cd = FALSE;

	// �C�x���g�ݒ�A�f�[�^�̏������݂�v��
	event.SetUser(0);
	event.SetTime(48);
}

//---------------------------------------------------------------------------
//
//	INQUIRY
//	��SCSI�̂�
//
//---------------------------------------------------------------------------
void FASTCALL SASI::Inquiry()
{
	ASSERT(this);
	ASSERT(sasi.current);

#if defined(SASI_LOG)
	LOG0(Log::Normal, "INQUIRY�R�}���h");
#endif	// SASI_LOG

	// �h���C�u�ŃR�}���h����
	sasi.length = sasi.current->Inquiry(sasi.cmd, sasi.buffer);
	if (sasi.length <= 0) {
		// ���s(�G���[)
		Error();
		return;
	}

	// �X�e�[�^�X��OK��
	sasi.status = 0x00;
	sasi.message = 0x00;

	// �h���C�u����ԋp���ꂽ�f�[�^��ԐM�BI/O=1, C/D=0
	sasi.offset = 0;
	sasi.blocks = 1;
	sasi.phase = read;
	sasi.io = TRUE;
	sasi.cd = FALSE;

	// �C�x���g�ݒ�A�f�[�^�̈�������v��
	event.SetUser(0);
	event.SetTime(48);
}

//---------------------------------------------------------------------------
//
//	MODE SENSE
//	��SCSI�̂�
//
//---------------------------------------------------------------------------
void FASTCALL SASI::ModeSense()
{
	int length;

	ASSERT(this);
	ASSERT(sasi.current);

#if defined(SASI_LOG)
	LOG0(Log::Normal, "MODE SENSE�R�}���h");
#endif	// SASI_LOG

	// �h���C�u�ŃR�}���h����
	length = sasi.current->ModeSense(sasi.cmd, sasi.buffer);

	// ���ʕ]��
	if (length > 0) {
		// �X�e�[�^�X��OK��
		sasi.status = 0x00;
		sasi.message = 0x00;
		sasi.length = length;
	}
	else {
#if 1
		Error();
		return;
#else
		// �X�e�[�^�X���G���[��
		sasi.status = (sasi.current->GetLUN() << 5) | 0x02;
		sasi.message = 0;
		sasi.length = -length;
#endif
	}

	// �h���C�u����ԋp���ꂽ�f�[�^��ԐM�BI/O=1, C/D=0
	sasi.offset = 0;
	sasi.blocks = 1;
	sasi.phase = read;
	sasi.io = TRUE;
	sasi.cd = FALSE;

	// �C�x���g�ݒ�A�f�[�^�̈�������v��
	event.SetUser(0);
	event.SetTime(48);
}

//---------------------------------------------------------------------------
//
//	START STOP UNIT
//	��SCSI�̂�
//
//---------------------------------------------------------------------------
void FASTCALL SASI::StartStop()
{
	BOOL status;

	ASSERT(this);
	ASSERT(sasi.current);

#if defined(SASI_LOG)
	LOG0(Log::Normal, "START STOP UNIT�R�}���h");
#endif	// SASI_LOG

	// �h���C�u�ŃR�}���h����
	status = sasi.current->StartStop(sasi.cmd);
	if (!status) {
		// ���s(�G���[)
		Error();
		return;
	}

	// ����
	sasi.status = 0x00;
	sasi.message = 0x00;

	// �X�e�[�^�X�t�F�[�Y
	Status();
}

//---------------------------------------------------------------------------
//
//	PREVENT/ALLOW MEDIUM REMOVAL
//	��SCSI�̂�
//
//---------------------------------------------------------------------------
void FASTCALL SASI::Removal()
{
	BOOL status;

	ASSERT(this);
	ASSERT(sasi.current);

#if defined(SASI_LOG)
	LOG0(Log::Normal, "PREVENT/ALLOW MEDIUM REMOVAL�R�}���h");
#endif	// SASI_LOG

	// �h���C�u�ŃR�}���h����
	status = sasi.current->Removal(sasi.cmd);
	if (!status) {
		// ���s(�G���[)
		Error();
		return;
	}

	// ����
	sasi.status = 0x00;
	sasi.message = 0x00;

	// �X�e�[�^�X�t�F�[�Y
	Status();
}

//---------------------------------------------------------------------------
//
//	READ CAPACITY
//	��SCSI�̂�
//
//---------------------------------------------------------------------------
void FASTCALL SASI::ReadCapacity()
{
	int length;

	ASSERT(this);
	ASSERT(sasi.current);

#if defined(SASI_LOG)
	LOG0(Log::Normal, "READ CAPACITY�R�}���h");
#endif	// SASI_LOG

	// �h���C�u�ŃR�}���h����
	length = sasi.current->ReadCapacity(sasi.cmd, sasi.buffer);

	// ���ʕ]��
	if (length > 0) {
		// �X�e�[�^�X��OK��
		sasi.status = 0x00;
		sasi.message = 0x00;
		sasi.length = length;
	}
	else {
#if 1
		// ���s(�G���[)
		Error();
		return;
#else
		// �X�e�[�^�X���G���[��
		sasi.status = (sasi.current->GetLUN() << 5) | 0x02;
		sasi.message = 0x00;
		sasi.length = -length;
#endif
	}

	// �h���C�u����ԋp���ꂽ�f�[�^��ԐM�BI/O=1, C/D=0
	sasi.offset = 0;
	sasi.blocks = 1;
	sasi.phase = read;
	sasi.io = TRUE;
	sasi.cd = FALSE;

	// �C�x���g�ݒ�A�f�[�^�̈�������v��
	event.SetUser(0);
	event.SetTime(48);
}

//---------------------------------------------------------------------------
//
//	READ(10)
//	��SCSI�̂�
//
//---------------------------------------------------------------------------
void FASTCALL SASI::Read10()
{
	DWORD record;

	ASSERT(this);
	ASSERT(sasi.current);

	// ���R�[�h�ԍ��ƃu���b�N�����擾
	record = sasi.cmd[2];
	record <<= 8;
	record |= sasi.cmd[3];
	record <<= 8;
	record |= sasi.cmd[4];
	record <<= 8;
	record |= sasi.cmd[5];
	sasi.blocks = sasi.cmd[7];
	sasi.blocks <<= 8;
	sasi.blocks |= sasi.cmd[8];

#if defined(SASI_LOG)
	LOG2(Log::Normal, "READ(10)�R�}���h ���R�[�h=%08X �u���b�N=%d", record, sasi.blocks);
#endif	// SASI_LOG

	// �u���b�N��0�͏������Ȃ�
	if (sasi.blocks == 0) {
		sasi.status = 0x00;
		sasi.message = 0x00;
		Status();
		return;
	}

	// �h���C�u�ŃR�}���h����
	sasi.length = sasi.current->Read(sasi.buffer, record);
	if (sasi.length <= 0) {
		// ���s(�G���[)
		Error();
		return;
	}

	// �X�e�[�^�X��OK��
	sasi.status = 0x00;
	sasi.message = 0x00;

	// ���̃u���b�Nok�A���[�N�ݒ�
	sasi.offset = 0;
	sasi.next = record + 1;

	// �ǂݍ��݃t�F�[�Y, I/O=1, C/D=0
	sasi.phase = read;
	sasi.io = TRUE;
	sasi.cd = FALSE;

	// �C�x���g��ݒ�A�f�[�^�̈�������v��
	event.SetUser(1);
	event.SetTime(48);
}

//---------------------------------------------------------------------------
//
//	WRITE(10)
//	��SCSI�̂�
//
//---------------------------------------------------------------------------
void FASTCALL SASI::Write10()
{
	DWORD record;

	ASSERT(this);
	ASSERT(sasi.current);

	// ���R�[�h�ԍ��ƃu���b�N�����擾
	record = sasi.cmd[2];
	record <<= 8;
	record |= sasi.cmd[3];
	record <<= 8;
	record |= sasi.cmd[4];
	record <<= 8;
	record |= sasi.cmd[5];
	sasi.blocks = sasi.cmd[7];
	sasi.blocks <<= 8;
	sasi.blocks |= sasi.cmd[8];

#if defined(SASI_LOG)
	LOG2(Log::Normal, "WRITE(10)�R�}���h ���R�[�h=%08X �u���b�N=%d", record, sasi.blocks);
#endif	// SASI_LOG

	// �u���b�N��0�͏������Ȃ�
	if (sasi.blocks == 0) {
		sasi.status = 0x00;
		sasi.message = 0x00;
		Status();
		return;
	}

	// �h���C�u�ŃR�}���h����
	sasi.length = sasi.current->WriteCheck(record);
	if (sasi.length <= 0) {
		// ���s(�G���[)
		Error();
		return;
	}

	// �X�e�[�^�X��OK��
	sasi.status = 0x00;
	sasi.message = 0x00;

	// �]�����[�N
	sasi.next = record + 1;
	sasi.offset = 0;

	// �������݃t�F�[�Y, C/D=0
	sasi.phase = write;
	sasi.cd = FALSE;

	// �C�x���g��ݒ�A�f�[�^�̏������݂�v��
	event.SetUser(1);
	event.SetTime(48);
}

//---------------------------------------------------------------------------
//
//	SEEK(10)
//	��SCSI�̂�
//
//---------------------------------------------------------------------------
void FASTCALL SASI::Seek10()
{
	BOOL status;

	ASSERT(this);
	ASSERT(sasi.current);

#if defined(SASI_LOG)
	LOG0(Log::Normal, "SEEK(10)�R�}���h");
#endif	// SASI_LOG

	// �h���C�u�ŃR�}���h����(�_���u���b�N�͊֌W�Ȃ��̂�Seek���Ă�)
	status = sasi.current->Seek(sasi.cmd);
	if (!status) {
		// ���s(�G���[)
		Error();
		return;
	}

	// ����
	sasi.status = 0x00;
	sasi.message = 0x00;

	// �X�e�[�^�X�t�F�[�Y
	Status();
}

//---------------------------------------------------------------------------
//
//	VERIFY
//	��SCSI�̂�
//
//---------------------------------------------------------------------------
void FASTCALL SASI::Verify()
{
	BOOL status;

	ASSERT(this);
	ASSERT(sasi.current);

#if defined(SASI_LOG)
	LOG0(Log::Normal, "VERIFY�R�}���h");
#endif	// SASI_LOG

	// �h���C�u�ŃR�}���h����
	status = sasi.current->Verify(sasi.cmd);
	if (!status) {
		// ���s(�G���[)
		Error();
		return;
	}

	// ����
	sasi.status = 0x00;
	sasi.message = 0x00;

	// �X�e�[�^�X�t�F�[�Y
	Status();
}

//---------------------------------------------------------------------------
//
//	SPECIFY
//	��SASI�̂�
//
//---------------------------------------------------------------------------
void FASTCALL SASI::Specify()
{
	ASSERT(this);
	ASSERT(sasi.current);

#if defined(SASI_LOG)
	LOG0(Log::Normal, "SPECIFY�R�}���h");
#endif	// SASI_LOG

	// SASI�ȊO�̓G���[(�x���_���j�[�N�R�}���h)
	if (!sasi.current->IsSASI()) {
		sasi.current->InvalidCmd();
		Error();
		return;
	}

	// �X�e�[�^�XOK
	sasi.status = 0x00;
	sasi.message = 0x00;

	// 10�o�C�g�̃f�[�^�����N�G�X�g
	sasi.phase = write;
	sasi.length = 10;
	sasi.offset = 0;

	// C/D��0(�f�[�^��������)
	sasi.cd = FALSE;

	// �C�x���g�ݒ�A�f�[�^�̏������݂�v��
	event.SetUser(0);
	event.SetTime(48);
}

//---------------------------------------------------------------------------
//
//	MO �I�[�v��
//
//---------------------------------------------------------------------------
BOOL FASTCALL SASI::Open(const Filepath& path)
{
	ASSERT(this);

	// SCSI���L���ł���΁ASCSI�ɂ܂�����
	if (sasi.scsi_type != 0) {
		return scsi->Open(path);
	}

	// �L���łȂ���΃G���[
	if (!IsValid()) {
		return FALSE;
	}

	// �C�W�F�N�g
	Eject(FALSE);

	// �m�b�g���f�B�łȂ���΃G���[
	if (IsReady()) {
		return FALSE;
	}

	// �h���C�u�ɔC����
	ASSERT(sasi.mo);
	if (sasi.mo->Open(path, TRUE)) {
		// �����ł���΁A�t�@�C���p�X�Ə������݋֎~��Ԃ��L��
		sasi.writep = sasi.mo->IsWriteP();
		sasi.mo->GetPath(scsimo);
		return TRUE;
	}

	// ���s
	return FALSE;
}

//---------------------------------------------------------------------------
//
//	MO �C�W�F�N�g
//
//---------------------------------------------------------------------------
void FASTCALL SASI::Eject(BOOL force)
{
	ASSERT(this);

	// SCSI���L���ł���΁ASCSI�ɂ܂�����
	if (sasi.scsi_type != 0) {
		scsi->Eject(force);
		return;
	}

	// ���f�B�łȂ���Ή������Ȃ�
	if (!IsReady()) {
		return;
	}

	// �h���C�u�ɒʒm
	ASSERT(sasi.mo);
	sasi.mo->Eject(force);
}

//---------------------------------------------------------------------------
//
//	MO �������݋֎~
//
//---------------------------------------------------------------------------
void FASTCALL SASI::WriteP(BOOL flag)
{
	ASSERT(this);

	// SCSI���L���ł���΁ASCSI�ɂ܂�����
	if (sasi.scsi_type != 0) {
		scsi->WriteP(flag);
		return;
	}

	// ���f�B�łȂ���Ή������Ȃ�
	if (!IsReady()) {
		return;
	}

	// ���C�g�v���e�N�g�ݒ�����݂�
	ASSERT(sasi.mo);
	sasi.mo->WriteP(flag);

	// �����ۑ�
	sasi.writep = sasi.mo->IsWriteP();
}

//---------------------------------------------------------------------------
//
//	MO �������݋֎~�擾
//
//---------------------------------------------------------------------------
BOOL FASTCALL SASI::IsWriteP() const
{
	ASSERT(this);

	// SCSI���L���ł���΁ASCSI�ɂ܂�����
	if (sasi.scsi_type != 0) {
		return scsi->IsWriteP();
	}

	// ���f�B�łȂ���΃��C�g�v���e�N�g�łȂ�
	if (!IsReady()) {
		return FALSE;
	}

	// �h���C�u�ɕ���
	ASSERT(sasi.mo);
	return sasi.mo->IsWriteP();
}

//---------------------------------------------------------------------------
//
//	MO Read Only�`�F�b�N
//
//---------------------------------------------------------------------------
BOOL FASTCALL SASI::IsReadOnly() const
{
	ASSERT(this);

	// SCSI���L���ł���΁ASCSI�ɂ܂�����
	if (sasi.scsi_type != 0) {
		return scsi->IsReadOnly();
	}

	// ���f�B�łȂ���΃��[�h�I�����[�łȂ�
	if (!IsReady()) {
		return FALSE;
	}

	// �h���C�u�ɕ���
	ASSERT(sasi.mo);
	return sasi.mo->IsReadOnly();
}

//---------------------------------------------------------------------------
//
//	MO Locked�`�F�b�N
//
//---------------------------------------------------------------------------
BOOL FASTCALL SASI::IsLocked() const
{
	ASSERT(this);

	// SCSI���L���ł���΁ASCSI�ɂ܂�����
	if (sasi.scsi_type != 0) {
		return scsi->IsLocked();
	}

	// �h���C�u�����݂��邩
	if (!sasi.mo) {
		return FALSE;
	}

	// �h���C�u�ɕ���
	return sasi.mo->IsLocked();
}

//---------------------------------------------------------------------------
//
//	MO Ready�`�F�b�N
//
//---------------------------------------------------------------------------
BOOL FASTCALL SASI::IsReady() const
{
	ASSERT(this);

	// SCSI���L���ł���΁ASCSI�ɂ܂�����
	if (sasi.scsi_type != 0) {
		return scsi->IsReady();
	}

	// �h���C�u�����݂��邩
	if (!sasi.mo) {
		return FALSE;
	}

	// �h���C�u�ɕ���
	return sasi.mo->IsReady();
}

//---------------------------------------------------------------------------
//
//	MO �L���`�F�b�N
//
//---------------------------------------------------------------------------
BOOL FASTCALL SASI::IsValid() const
{
	ASSERT(this);

	// SCSI���L���ł���΁ASCSI�ɂ܂�����
	if (sasi.scsi_type != 0) {
		return scsi->IsValid();
	}

	// �|�C���^�Ŕ���
	if (sasi.mo) {
		return TRUE;
	}
	else {
		return FALSE;
	}
}

//---------------------------------------------------------------------------
//
//	MO �p�X�擾
//
//---------------------------------------------------------------------------
void FASTCALL SASI::GetPath(Filepath& path) const
{
	ASSERT(this);

	// SCSI���L���ł���΁ASCSI�ɂ܂�����
	if (sasi.scsi_type != 0) {
		scsi->GetPath(path);
		return;
	}

	if (sasi.mo) {
		if (sasi.mo->IsReady()) {
			// MO�f�B�X�N����
			sasi.mo->GetPath(path);
			return;
		}
	}

	// �L���ȃp�X�łȂ��̂ŁA�N���A
	path.Clear();
}
