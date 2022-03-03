//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 �o�h�D(ytanaka@ipc-tokai.or.jp)
//	[ SCSI(MB89352) ]
//
//---------------------------------------------------------------------------

#include "os.h"
#include "xm6.h"
#include "vm.h"
#include "log.h"
#include "schedule.h"
#include "memory.h"
#include "sram.h"
#include "config.h"
#include "disk.h"
#include "fileio.h"
#include "filepath.h"
#include "scsi.h"

//===========================================================================
//
//	SCSI
//
//===========================================================================
//#define SCSI_LOG

//---------------------------------------------------------------------------
//
//	�R���X�g���N�^
//
//---------------------------------------------------------------------------
SCSI::SCSI(VM *p) : MemDevice(p)
{
	// �f�o�C�XID��������
	dev.id = MAKEID('S', 'C', 'S', 'I');
	dev.desc = "SCSI (MB89352)";

	// �J�n�A�h���X�A�I���A�h���X
	memdev.first = 0xea0000;
	memdev.last = 0xea1fff;

	// ���[�N������
	memset(&scsi, 0, sizeof(scsi));

	// �f�o�C�X
	memory = NULL;
	sram = NULL;
}

//---------------------------------------------------------------------------
//
//	������
//
//---------------------------------------------------------------------------
BOOL FASTCALL SCSI::Init()
{
	int i;

	ASSERT(this);

	// ��{�N���X
	if (!MemDevice::Init()) {
		return FALSE;
	}

	// �������擾
	ASSERT(!memory);
	memory = (Memory*)vm->SearchDevice(MAKEID('M', 'E', 'M', ' '));
	ASSERT(memory);

	// SRAM�擾
	ASSERT(!sram);
	sram = (SRAM*)vm->SearchDevice(MAKEID('S', 'R', 'A', 'M'));
	ASSERT(sram);

	// ���[�N������
	scsi.bdid = 0x80;
	scsi.ilevel = 2;
	scsi.vector = -1;
	scsi.id = -1;
	for (i=0; i<sizeof(scsi.cmd); i++) {
		scsi.cmd[i] = 0;
	}

	// �R���t�B�O������
	scsi.memsw = TRUE;
	scsi.mo_first = FALSE;

	// �C�x���g������
	event.SetDevice(this);
	event.SetDesc("Selection");
	event.SetUser(0);
	event.SetTime(0);
	cdda.SetDevice(this);
	cdda.SetDesc("CD-DA 75fps");
	cdda.SetUser(1);
	cdda.SetTime(0);

	// �f�B�X�N������
	for (i=0; i<DeviceMax; i++) {
		scsi.disk[i] = NULL;
	}
	for (i=0; i<HDMax; i++) {
		scsi.hd[i] = NULL;
	}
	scsi.mo = NULL;
	scsi.cdrom = NULL;

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	�N���[���A�b�v
//
//---------------------------------------------------------------------------
void FASTCALL SCSI::Cleanup()
{
	int i;

	ASSERT(this);

	// HD�폜
	for (i=0; i<HDMax; i++) {
		if (scsi.hd[i]) {
			delete scsi.hd[i];
			scsi.hd[i] = NULL;
		}
	}

	// MO�폜
	if (scsi.mo) {
		delete scsi.mo;
		scsi.mo = NULL;
	}

	// CD�폜
	if (scsi.cdrom) {
		delete scsi.cdrom;
		scsi.cdrom = NULL;
	}

	// ��{�N���X��
	MemDevice::Cleanup();
}

//---------------------------------------------------------------------------
//
//	���Z�b�g
//
//---------------------------------------------------------------------------
void FASTCALL SCSI::Reset()
{
	Memory::memtype type;
	int i;

	ASSERT(this);
	LOG0(Log::Normal, "���Z�b�g");

	// SCSI�^�C�v(�������ɖ₢���킹��)
	type = memory->GetMemType();
	switch (type) {
		// SASI�̂�
		case Memory::None:
		case Memory::SASI:
			scsi.type = 0;
			break;

		// �O�t
		case Memory::SCSIExt:
			scsi.type = 1;
			break;

		// ���̑�(����)
		default:
			scsi.type = 2;
			break;
	}

	// �C�x���g�𓮓I�ɒǉ��E�폜
	if (scsi.type == 0) {
		// �C�x���g�폜
		if (scheduler->HasEvent(&event)) {
			scheduler->DelEvent(&event);
			scheduler->DelEvent(&cdda);
		}
	}
	else {
		// �C�x���g�ǉ�
		if (!scheduler->HasEvent(&event)) {
			scheduler->AddEvent(&event);
			scheduler->AddEvent(&cdda);
		}
	}

	// �������X�C�b�`��������
	if (scsi.memsw) {
		// SCSI�����݂���ꍇ�̂�
		if (scsi.type != 0) {
			// $ED006F:SCSI�t���O('V'��SCSI�L��)
			if (sram->GetMemSw(0x6f) != 'V') {
				sram->SetMemSw(0x6f, 'V');

				// �{��ID��7�ɏ�����
				sram->SetMemSw(0x70, 0x07);
			}

			// $ED0070:�{�[�h���+�{��ID
			if (scsi.type == 1) {
				// �O�t
				sram->SetMemSw(0x70, sram->GetMemSw(0x70) | 0x08);
			}
			else {
				// ����
				sram->SetMemSw(0x70, sram->GetMemSw(0x70) & ~0x08);
			}

			// $ED0071:SASI�G�~�����[�V�����t���O
			sram->SetMemSw(0x71, 0x00);
		}
	}

	// �f�B�X�N�č\�z
	Construct();

	// �f�B�X�N���Z�b�g
	for (i=0; i<HDMax; i++) {
		if (scsi.hd[i]) {
			scsi.hd[i]->Reset();
		}
	}
	if (scsi.mo) {
		scsi.mo->Reset();
	}
	if (scsi.cdrom) {
		scsi.cdrom->Reset();
	}

	// �R�}���h�N���A
	for (i=0; i<sizeof(scsi.cmd); i++) {
		scsi.cmd[i] = 0;
	}

	// ���W�X�^���Z�b�g
	ResetReg();

	// �o�X�t���[�t�F�[�Y
	BusFree();
}

//---------------------------------------------------------------------------
//
//	�Z�[�u
//
//---------------------------------------------------------------------------
BOOL FASTCALL SCSI::Save(Fileio *fio, int ver)
{
	size_t sz;
	int i;
	Disk *disk;

	ASSERT(this);
	ASSERT(fio);

	LOG0(Log::Normal, "�Z�[�u");

	// �f�B�X�N���t���b�V��
	for (i=0; i<HDMax; i++) {
		if (scsi.hd[i]) {
			if (!scsi.hd[i]->Flush()) {
				return FALSE;
			}
		}
	}
	if (scsi.mo) {
		if (!scsi.mo->Flush()) {
			return FALSE;
		}
	}
	if (scsi.cdrom) {
		if (!scsi.cdrom->Flush()) {
			return FALSE;
		}
	}

	// �T�C�Y���Z�[�u
	sz = sizeof(scsi_t);
	if (!fio->Write(&sz, sizeof(sz))) {
		return FALSE;
	}

	// ���̂��Z�[�u
	if (!fio->Write(&scsi, (int)sz)) {
		return FALSE;
	}

	// �C�x���g���Z�[�u
	if (!event.Save(fio, ver)) {
		return FALSE;
	}
	if (!cdda.Save(fio, ver)) {
		return FALSE;
	}

	// �p�X���Z�[�u
	for (i=0; i<HDMax; i++) {
		if (!scsihd[i].Save(fio, ver)) {
			return FALSE;
		}
	}

	// �f�B�X�N���Z�[�u
	for (i=0; i<HDMax; i++) {
		if (scsi.hd[i]) {
			if (!scsi.hd[i]->Save(fio, ver)) {
				return FALSE;
			}
		}
		else {
			// �����킹�̂��߁A�_�~�[�ł��悢����ۑ�����(version2.04)
			disk = new SCSIHD(this);
			if (!disk->Save(fio, ver)) {
				delete disk;
				return FALSE;
			}
			delete disk;
		}
	}
	if (scsi.mo) {
		if (!scsi.mo->Save(fio, ver)) {
			return FALSE;
		}
	}
	if (scsi.cdrom) {
		if (!scsi.cdrom->Save(fio, ver)) {
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
BOOL FASTCALL SCSI::Load(Fileio *fio, int ver)
{
	size_t sz;
	int i;
	Disk *disk;

	ASSERT(this);
	ASSERT(fio);
	ASSERT(ver >= 0x0200);

	LOG0(Log::Normal, "���[�h");

	// version2.01�ȑO�́ASCSI���T�|�[�g���Ă��Ȃ�
	if (ver <= 0x201) {
		// SCSI�Ȃ��A�h���C�u��0
		scsi.type = 0;
		scsi.scsi_drives = 0;

		// �C�x���g�폜
		if (scheduler->HasEvent(&event)) {
			scheduler->DelEvent(&event);
			scheduler->DelEvent(&cdda);
		}
		return TRUE;
	}

	// �f�B�X�N���폜���A��U����������
	for (i=0; i<HDMax; i++) {
		if (scsi.hd[i]) {
			delete scsi.hd[i];
			scsi.hd[i] = NULL;
		}
	}
	if (scsi.mo) {
		delete scsi.mo;
		scsi.mo = NULL;
	}
	if (scsi.cdrom) {
		delete scsi.cdrom;
		scsi.cdrom = NULL;
	}
	scsi.type = 0;
	scsi.scsi_drives = 0;

	// �T�C�Y�����[�h�A�ƍ�
	if (!fio->Read(&sz, sizeof(sz))) {
		return FALSE;
	}
	if (sz != sizeof(scsi_t)) {
		return FALSE;
	}

	// ���̂����[�h
	if (!fio->Read(&scsi, (int)sz)) {
		return FALSE;
	}

	// �f�B�X�N���N���A(���[�h���ɏ㏑������邽��)
	for (i=0; i<HDMax; i++) {
		scsi.hd[i] = NULL;
	}
	scsi.mo = NULL;
	scsi.cdrom = NULL;

	// �C�x���g�����[�h
	if (!event.Load(fio, ver)) {
		return FALSE;
	}
	if (ver >= 0x0203) {
		// version2.03�ŁACD-DA�C�x���g���ǉ����ꂽ
		if (!cdda.Load(fio, ver)) {
			return FALSE;
		}
	}

	// �p�X�����[�h
	for (i=0; i<HDMax; i++) {
		if (!scsihd[i].Load(fio, ver)) {
			return FALSE;
		}
	}

	// �f�B�X�N�č\�z
	Construct();

	// �f�B�X�N�����[�h(version2.03�Œǉ�)
	if (ver >= 0x0203) {
		for (i=0; i<HDMax; i++) {
			if (scsi.hd[i]) {
				if (!scsi.hd[i]->Load(fio, ver)) {
					return FALSE;
				}
			}
			else {
				// �����킹�̂��߁A�_�~�[�ł��悢�̂Ń��[�h(version2.04)
				if (ver >= 0x0204) {
					disk = new SCSIHD(this);
					if (!disk->Load(fio, ver)) {
						delete disk;
						return FALSE;
					}
					delete disk;
				}
			}
		}
		if (scsi.mo) {
			if (!scsi.mo->Load(fio, ver)) {
				return FALSE;
			}
		}
		if (scsi.cdrom) {
			if (!scsi.cdrom->Load(fio, ver)) {
				return FALSE;
			}
		}
	}

	// �C�x���g�𓮓I�ɒǉ��E�폜
	if (scsi.type == 0) {
		// �C�x���g�폜
		if (scheduler->HasEvent(&event)) {
			scheduler->DelEvent(&event);
			scheduler->DelEvent(&cdda);
		}
	}
	else {
		// �C�x���g�ǉ�
		if (!scheduler->HasEvent(&event)) {
			scheduler->AddEvent(&event);
			scheduler->AddEvent(&cdda);
		}
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	�ݒ�K�p
//
//---------------------------------------------------------------------------
void FASTCALL SCSI::ApplyCfg(const Config *config)
{
	int i;

	ASSERT(this);
	ASSERT(config);
	LOG0(Log::Normal, "�ݒ�K�p");

	// SCSI�h���C�u��
	scsi.scsi_drives = config->scsi_drives;

	// �������X�C�b�`�����X�V
	scsi.memsw = config->scsi_sramsync;

	// MO�D��t���O
	scsi.mo_first = config->scsi_mofirst;

	// SCSI�t�@�C����
	for (i=0; i<HDMax; i++) {
		scsihd[i].SetPath(config->scsi_file[i]);
	}

	// �f�B�X�N�č\�z
	Construct();
}

#if !defined(NDEBUG)
//---------------------------------------------------------------------------
//
//	�f�f
//
//---------------------------------------------------------------------------
void FASTCALL SCSI::AssertDiag() const
{
	// ��{�N���X
	MemDevice::AssertDiag();

	ASSERT(this);
	ASSERT(GetID() == MAKEID('S', 'C', 'S', 'I'));
	ASSERT(memory);
	ASSERT(memory->GetID() == MAKEID('M', 'E', 'M', ' '));
	ASSERT(sram);
	ASSERT(sram->GetID() == MAKEID('S', 'R', 'A', 'M'));
	ASSERT((scsi.type == 0) || (scsi.type == 1) || (scsi.type == 2));
	ASSERT((scsi.vector == -1) || (scsi.vector == 0x6c) || (scsi.vector == 0xf6));
	ASSERT((scsi.id >= -1) && (scsi.id <= 7));
	ASSERT(scsi.bdid != 0);
	ASSERT(scsi.bdid < 0x100);
	ASSERT(scsi.ints < 0x100);
	ASSERT(scsi.mbc < 0x10);
}
#endif	// NDEBUG

//---------------------------------------------------------------------------
//
//	�o�C�g�ǂݍ���
//
//---------------------------------------------------------------------------
DWORD FASTCALL SCSI::ReadByte(DWORD addr)
{
	const BYTE *rom;

	ASSERT(this);
	ASSERT(addr <= 0xffffff);

	// �ʏ�̓ǂݍ��݂̏ꍇ
	if (addr >= memdev.first) {
		// SASI�̂� or �����̏ꍇ�A���̋�Ԃ͌����Ȃ�
		if (scsi.type != 1) {
			// �o�X�G���[
			cpu->BusErr(addr, TRUE);
			return 0xff;
		}
	}

	// �A�h���X�}�X�N
	addr &= 0x1fff;

	// ROM�f�[�^
	if (addr >= 0x20) {
		// �E�F�C�g
		scheduler->Wait(1);

		// ROM��Ԃ�
		rom = memory->GetSCSI();
		return rom[addr];
	}

	// ���W�X�^
	if ((addr & 1) == 0) {
		// �����A�h���X�̓f�R�[�h����Ă��Ȃ�
		return 0xff;
	}
	addr &= 0x1f;
	addr >>= 1;

	// �E�F�C�g
	scheduler->Wait(1);

	// ���W�X�^��
	switch (addr) {
		// BDID
		case 0:
			return scsi.bdid;

		// SCTL
		case 1:
			return scsi.sctl;

		// SCMD
		case 2:
			return scsi.scmd;

		// ���U�[�u
		case 3:
			return 0xff;

		// INTS
		case 4:
			return scsi.ints;

		// PSNS
		case 5:
			return GetPSNS();

		// SSTS
		case 6:
			return GetSSTS();

		// SERR
		case 7:
			return GetSERR();

		// PCTL
		case 8:
			return scsi.pctl;

		// MBC
		case 9:
			return scsi.mbc;

		// DREG
		case 10:
			return GetDREG();

		// TEMP
		case 11:
			return scsi.temp;

		// TCH
		case 12:
			return (BYTE)(scsi.tc >> 16);

		// TCM
		case 13:
			return (BYTE)(scsi.tc >> 8);

		// TCL
		case 14:
			return (BYTE)scsi.tc;

		// ���U�[�u
		case 15:
			return 0xff;
	}

	// �ʏ�A�����ɂ͗��Ȃ�
	LOG1(Log::Warning, "����`���W�X�^�ǂݍ��� R%d", addr);
	return 0xff;
}

//---------------------------------------------------------------------------
//
//	���[�h�ǂݍ���
//
//---------------------------------------------------------------------------
DWORD FASTCALL SCSI::ReadWord(DWORD addr)
{
	ASSERT(this);
	ASSERT((addr & 1) == 0);

	return (0xff00 | ReadByte(addr + 1));
}

//---------------------------------------------------------------------------
//
//	�o�C�g��������
//
//---------------------------------------------------------------------------
void FASTCALL SCSI::WriteByte(DWORD addr, DWORD data)
{
	ASSERT(this);
	ASSERT(addr <= 0xffffff);
	ASSERT(data < 0x100);

	// �ʏ�̓ǂݍ��݂̏ꍇ
	if (addr >= memdev.first) {
		// SASI�̂� or �����̏ꍇ�A���̋�Ԃ͌����Ȃ�
		if (scsi.type != 1) {
			// �o�X�G���[
			cpu->BusErr(addr, FALSE);
			return;
		}
	}

	// �A�h���X�}�X�N
	addr &= 0x1fff;

	// ROM�f�[�^
	if (addr >= 0x20) {
		// ROM�ւ̏�������
		return;
	}

	// ���W�X�^
	if ((addr & 1) == 0) {
		// �����A�h���X�̓f�R�[�h����Ă��Ȃ�
		return;
	}
	addr &= 0x1f;
	addr >>= 1;

	// �E�F�C�g
	scheduler->Wait(1);

	// ���W�X�^��
	switch (addr) {
		// BDID
		case 0:
			SetBDID(data);
			return;

		// SCTL
		case 1:
			SetSCTL(data);
			return;

		// SCMD
		case 2:
			SetSCMD(data);
			return;

		// ���U�[�u
		case 3:
			break;

		// INTS
		case 4:
			SetINTS(data);
			return;

		// SDGC
		case 5:
			SetSDGC(data);
			return;

		// SSTS(Read Only)
		case 6:
			// SCSI�h���C�ov1.04�̏C���p�b�`�ŁA���ԉ҂��̂��߂ɏ�������ł���l�q
			return;

		// SERR(Read Only)
		case 7:
			break;

		// PCTL
		case 8:
			SetPCTL(data);
			return;

		// MBC(Read Only)
		case 9:
			break;

		// DREG
		case 10:
			SetDREG(data);
			return;

		// TEMP
		case 11:
			SetTEMP(data);
			return;

		// TCH
		case 12:
			SetTCH(data);
			return;

		// TCM
		case 13:
			SetTCM(data);
			return;

		// TCL
		case 14:
			SetTCL(data);
			return;

		// ���U�[�u
		case 15:
			break;
	}

	// �ʏ�A�����ɂ͗��Ȃ�
	LOG2(Log::Warning, "����`���W�X�^�������� R%d <- $%02X", addr, data);
}

//---------------------------------------------------------------------------
//
//	���[�h��������
//
//---------------------------------------------------------------------------
void FASTCALL SCSI::WriteWord(DWORD addr, DWORD data)
{
	ASSERT(this);
	ASSERT((addr & 1) == 0);
	ASSERT(data < 0x10000);

	WriteByte(addr + 1, (BYTE)data);
}

//---------------------------------------------------------------------------
//
//	�ǂݍ��݂̂�
//
//---------------------------------------------------------------------------
DWORD FASTCALL SCSI::ReadOnly(DWORD addr) const
{
	const BYTE *rom;

	ASSERT(this);

	// �ʏ�̓ǂݍ��݂̏ꍇ
	if (addr >= memdev.first) {
		// SASI�̂� or �����̏ꍇ�A���̋�Ԃ͌����Ȃ�
		if (scsi.type != 1) {
			return 0xff;
		}
	}

	// �A�h���X�}�X�N
	addr &= 0x1fff;

	// ROM�f�[�^
	if (addr >= 0x20) {
		// ROM��Ԃ�
		rom = memory->GetSCSI();
		return rom[addr];
	}

	// ���W�X�^
	if ((addr & 1) == 0) {
		// �����A�h���X�̓f�R�[�h����Ă��Ȃ�
		return 0xff;
	}
	addr &= 0x1f;
	addr >>= 1;

	// ���W�X�^��
	switch (addr) {
		// BDID
		case 0:
			return scsi.bdid;

		// SCTL
		case 1:
			return scsi.sctl;

		// SCMD
		case 2:
			return scsi.scmd;

		// ���U�[�u
		case 3:
			break;

		// INTS
		case 4:
			return scsi.ints;

		// PSNS
		case 5:
			return GetPSNS();

		// SSTS
		case 6:
			return GetSSTS();

		// SERR
		case 7:
			return GetSERR();

		// PCTL
		case 8:
			return scsi.pctl;

		// MBC
		case 9:
			return scsi.mbc;

		// DREG
		case 10:
			return scsi.buffer[scsi.offset];

		// TEMP
		case 11:
			return scsi.temp;

		// TCH
		case 12:
			return (BYTE)(scsi.tc >> 16);

		// TCM
		case 13:
			return (BYTE)(scsi.tc >> 8);

		// TCL
		case 14:
			return (BYTE)scsi.tc;

		// ���U�[�u
		case 15:
			break;
	}

	return 0xff;
}

//---------------------------------------------------------------------------
//
//	�����f�[�^�擾
//
//---------------------------------------------------------------------------
void FASTCALL SCSI::GetSCSI(scsi_t *buffer) const
{
	ASSERT(this);
	ASSERT(buffer);
	ASSERT_DIAG();

	// �������[�N���R�s�[
	*buffer = scsi;
}

//---------------------------------------------------------------------------
//
//	�C�x���g�R�[���o�b�N
//
//---------------------------------------------------------------------------
BOOL FASTCALL SCSI::Callback(Event* ev)
{
	BOOL success;

	ASSERT(this);
	ASSERT(ev);
	ASSERT_DIAG();

	// CD-DA�C�x���g��p����
	if (ev->GetUser() == 1) {
		if (scsi.cdrom) {
			// CD-ROM�֒ʒm
			if (scsi.cdrom->NextFrame()) {
				// �ʏ푬�x
				ev->SetTime(26666);
			}
			else {
				// 75���1�񂾂��A�␳����
				ev->SetTime(26716);
			}
		}

		// �p��
		return TRUE;
	}

	// �Z���N�V�����t�F�[�Y�̂ݗL��
	ASSERT(ev->GetUser() == 0);
	if (scsi.phase != selection) {
		// �P��
		return FALSE;
	}

	// �����t���O�쐬
	success = FALSE;
	if (scsi.id != -1) {
		// ID���C�j�V�G�[�^�ƃ^�[�Q�b�g�A�����w�肳��Ă���
		if (scsi.disk[scsi.id]) {
			// �L���ȃf�B�X�N���}�E���g����Ă���
			success = TRUE;
		}
	}

	// �����t���O�ɂ�蕪����
	if (success) {
#if defined(SCSI_LOG)
		LOG1(Log::Normal, "�Z���N�V�������� ID=%d", scsi.id);
#endif	// SCSI_LOG

		// TC���f�N�������g
		scsi.tc &= 0x00ffff00;
		scsi.tc -= 0x2800;

		// �R���v���[�g(�Z���N�V��������)
		Interrupt(4, TRUE);

		// ������BSY=1
		scsi.bsy = TRUE;

		// ATN=1�Ȃ烁�b�Z�[�W�A�E�g�t�F�[�Y�A�����łȂ���΃R�}���h�t�F�[�Y
		if (scsi.atn) {
			MsgOut();
		}
		else {
			Command();
		}
	}
	else {
#if defined(SCSI_LOG)
		LOG1(Log::Normal, "�Z���N�V�������s TEMP=$%02X", scsi.temp);
#endif	// SCSI_LOG

		// TC=0�Ƃ���(�^�C���A�E�g)
		scsi.tc = 0;

		// �^�C���A�E�g(�Z���N�V�������s)
		Interrupt(2, TRUE);

		// BSY=FALSE�Ȃ�o�X�t���[
		if (!scsi.bsy) {
			BusFree();
		}
	}

	// �P��
	return FALSE;
}

//---------------------------------------------------------------------------
//
//	���W�X�^���Z�b�g
//
//---------------------------------------------------------------------------
void FASTCALL SCSI::ResetReg()
{
	ASSERT(this);

#if defined(SCSI_LOG)
	LOG0(Log::Normal, "���W�X�^���Z�b�g");
#endif	// SCSI_LOG

	// �J�����gID
	scsi.id = -1;

	// ���荞��
	scsi.vector = -1;

	// ���W�X�^
	scsi.bdid = 0x80;
	scsi.sctl = 0x80;
	scsi.scmd = 0;
	scsi.ints = 0;
	scsi.sdgc = 0;
	scsi.pctl = 0;
	scsi.mbc = 0;
	scsi.temp = 0;
	scsi.tc = 0;

	// �]���R�}���h��~
	scsi.trans = FALSE;

	// �t�F�[�Y(SPC�̓o�X�t���[�̂���)
	scsi.phase = busfree;

	// ���荞�݃`�F�b�N
	IntCheck();
}

//---------------------------------------------------------------------------
//
//	�]�����Z�b�g
//
//---------------------------------------------------------------------------
void FASTCALL SCSI::ResetCtrl()
{
	ASSERT(this);

#if defined(SCSI_LOG)
	LOG0(Log::Normal, "�]�����Z�b�g");
#endif	// SCSI_LOG

	// �]���R�}���h��~
	scsi.trans = FALSE;
}

//---------------------------------------------------------------------------
//
//	�o�X���Z�b�g
//
//---------------------------------------------------------------------------
void FASTCALL SCSI::ResetBus(BOOL reset)
{
	ASSERT(this);
	ASSERT_DIAG();

#if defined(SCSI_LOG)
	if (reset) {
		LOG0(Log::Normal, "SCSI�o�X ���Z�b�g");
	}
	else {
		LOG0(Log::Normal, "SCSI�o�X ���Z�b�g����");
	}
#endif	// SCSI_LOG

	// �L��
	scsi.rst = reset;

	// �M����(�o�X�t���[�Ɠ���)
	scsi.bsy = FALSE;
	scsi.sel = FALSE;
	scsi.atn = FALSE;
	scsi.msg = FALSE;
	scsi.cd = FALSE;
	scsi.io = FALSE;
	scsi.req = FALSE;
	scsi.ack = FALSE;

	// �]���R�}���h��~
	scsi.trans = FALSE;

	// �t�F�[�Y(SPC�̓o�X�t���[�̂���)
	scsi.phase = busfree;

	// �o�X���Z�b�g���荞�ݔ����E����
	Interrupt(0, reset);
}

//---------------------------------------------------------------------------
//
//	BDID�ݒ�
//
//---------------------------------------------------------------------------
void FASTCALL SCSI::SetBDID(DWORD data)
{
	DWORD bdid;

	ASSERT(this);
	ASSERT(data < 0x100);

	// 3bit�̂ݗL��
	data &= 0x07;

#if defined(SCSI_LOG)
	LOG1(Log::Normal, "�{�[�hID�ݒ� ID%d", data);
#endif	// SCSI_LOG

	// ���l����r�b�g�֕ϊ�
	bdid = (DWORD)(1 << data);

	// �قȂ�Ƃ��́A�f�B�X�N�č\�z���K�v
	if (scsi.bdid != bdid) {
		scsi.bdid = bdid;
		Construct();
	}
}

//---------------------------------------------------------------------------
//
//	SCTL�ݒ�
//
//---------------------------------------------------------------------------
void FASTCALL SCSI::SetSCTL(DWORD data)
{
	ASSERT(this);
	ASSERT(data < 0x100);

#if defined(SCSI_LOG)
	LOG1(Log::Normal, "SCTL�ݒ� $%02X", data);
#endif	// SCSI_LOG

	scsi.sctl = data;

	// bit7:Reset & Disable
	if (scsi.sctl & 0x80) {
		// ���W�X�^���Z�b�g
		ResetReg();
	}

	// bit6:Reset Control
	if (scsi.sctl & 0x40) {
		// �]�����Z�b�g
		ResetCtrl();
	}

	// ���荞�݃`�F�b�N
	IntCheck();
}

//---------------------------------------------------------------------------
//
//	SCMD�ݒ�
//
//---------------------------------------------------------------------------
void FASTCALL SCSI::SetSCMD(DWORD data)
{
	ASSERT(this);
	ASSERT(data < 0x100);
	ASSERT_DIAG();

	// bit4: RST Out
	if (data & 0x10) {
		if ((scsi.scmd & 0x10) == 0) {
			// RST��0��1
			if ((scsi.sctl & 0x80) == 0) {
				// SPC�����Z�b�g���łȂ�
				ResetBus(TRUE);
			}
		}
	}
	else {
		if (scsi.scmd & 0x10) {
			// RST��1��0
			if ((scsi.sctl & 0x80) == 0) {
				// SPC�����Z�b�g���łȂ�
				ResetBus(FALSE);
			}
		}
	}

	// �R�}���h�L��
	scsi.scmd = data;

	// bit7-5:Command
	switch (scsi.scmd >> 5) {
		// Bus Release
		case 0:
			BusRelease();
			break;

		// Select
		case 1:
			Select();
			break;

		// Reset ATN
		case 2:
			ResetATN();
			break;

		// Set ATN
		case 3:
			SetATN();
			break;

		// Transfer
		case 4:
			Transfer();
			break;

		// Transfer Pause
		case 5:
			TransPause();
			break;

		// Reset ACK/REQ
		case 6:
			ResetACKREQ();
			break;

		// Set ACK/REQ
		case 7:
			SetACKREQ();
			break;

		// ���̑�(���肦�Ȃ�)
		default:
			ASSERT(FALSE);
			break;
	}
}

//---------------------------------------------------------------------------
//
//	INTS�ݒ�
//
//---------------------------------------------------------------------------
void FASTCALL SCSI::SetINTS(DWORD data)
{
	ASSERT(this);
	ASSERT(data < 0x100);
	ASSERT_DIAG();

#if defined(SCSI_LOG)
	LOG1(Log::Normal, "���荞�݃N���A $%02X", data);
#endif	// SCSI_LOG

	// �r�b�g�𗧂Ă����荞�݂���艺����
	scsi.ints &= ~data;

	// ���荞�݃`�F�b�N
	IntCheck();
}

//---------------------------------------------------------------------------
//
//	PSNS�擾
//
//---------------------------------------------------------------------------
DWORD FASTCALL SCSI::GetPSNS() const
{
	DWORD data;

	ASSERT(this);
	ASSERT_DIAG();

	// �f�[�^������
	data = 0;

	// bit7:REQ
	if (scsi.req) {
		data |= 0x80;
	}

	// bit6:ACK
	if (scsi.ack) {
		data |= 0x40;
	}

	// bit5:ATN
	if (scsi.atn) {
		data |= 0x20;
	}

	// bit4:SEL
	if (scsi.sel) {
		data |= 0x10;
	}

	// bit3:BSY
	if (scsi.bsy) {
		data |= 0x08;
	}

	// bit2:MSG
	if (scsi.msg) {
		data |= 0x04;
	}

	// bit1:C/D
	if (scsi.cd) {
		data |= 0x02;
	}

	// bit0:I/O
	if (scsi.io) {
		data |= 0x01;
	}

	return data;
}

//---------------------------------------------------------------------------
//
//	SDGC�ݒ�
//
//---------------------------------------------------------------------------
void FASTCALL SCSI::SetSDGC(DWORD data)
{
	ASSERT(this);
	ASSERT(data < 0x100);

#if defined(SCSI_LOG)
	LOG1(Log::Normal, "SDGC�ݒ� $%02X", data);
#endif	// SCSI_LOG

	scsi.sdgc = data;
}

//---------------------------------------------------------------------------
//
//	SSTS�擾
//
//---------------------------------------------------------------------------
DWORD FASTCALL SCSI::GetSSTS() const
{
	DWORD data;

	ASSERT(this);
	ASSERT_DIAG();

	// �f�[�^������
	data = 0;

	// bit6-7:Connected
	if (scsi.phase != busfree) {
		// �o�X�t���[�ȊO�́A�C�j�V�G�[�^�Ƃ��Č�����
		data = 0x80;
	}

	// bit5:SPC BUSY
	if (scsi.bsy) {
		data |= 0x20;
	}

	// bit4:Transfer Progress
	if (scsi.trans) {
		data |= 0x10;
	}

	// bit3:Reset
	if (scsi.rst) {
		data |= 0x08;
	}

	// bit2:TC=0
	if (scsi.tc == 0) {
		data |= 0x04;
	}

	// bit0-1:FIFO�� (00:1�`7�o�C�g, 01:Empty, 10:8�o�C�g, 11:���g�p)
	if (scsi.trans) {
		// �]������
		if (scsi.io) {
			// Read���Ɍ���AFIFO�Ƀf�[�^�����܂�\��������
			if ((scsi.length > 0) && (scsi.tc > 0)) {
				if ((scsi.length >= 8) && (scsi.tc >= 8)) {
					// 8�o�C�g�ȏ�(TS-6BS1mkIII)
					data |= 0x02;
					return data;
				}
				else {
					// 7�o�C�g�ȉ�
					return data;
				}
			}
		}
	}

	// FIFO�Ƀf�[�^�͂Ȃ�
	data |= 0x01;
	return data;
}

//---------------------------------------------------------------------------
//
//	SERR�擾
//
//---------------------------------------------------------------------------
DWORD FASTCALL SCSI::GetSERR() const
{
	ASSERT(this);
	ASSERT_DIAG();

	// �v���O�����]�����A���N�G�X�g�M�����o���Ȃ�
	if (scsi.sdgc & 0x20) {
		if (scsi.trans) {
			if (scsi.tc != 0) {
				// Xfer Out������
				return 0x20;
			}
		}
	}

	// ����ȊO��0
	return 0x00;
}

//---------------------------------------------------------------------------
//
//	PCTL�ݒ�
//
//---------------------------------------------------------------------------
void FASTCALL SCSI::SetPCTL(DWORD data)
{
	ASSERT(this);
	ASSERT(data < 0x100);

	scsi.pctl = data;

}

//---------------------------------------------------------------------------
//
//	DREG�擾
//
//---------------------------------------------------------------------------
DWORD FASTCALL SCSI::GetDREG()
{
	DWORD data;

	ASSERT(this);
	ASSERT_DIAG();

	// �]�����łȂ���΁AFF��Ԃ�
	if (!scsi.trans) {
		return 0xff;
	}

	// ���N�G�X�g���オ���Ă��Ȃ���΁AFF��Ԃ�
	if (!scsi.req) {
		return 0xff;
	}

	// TC��0�Ȃ�AFF��Ԃ�
	if (scsi.tc == 0) {
		return 0xff;
	}

	// REQ-ACK�n���h�V�F�[�N�������ōs��
	Xfer(&data);
	XferNext();

	// MBC
	scsi.mbc = (scsi.mbc - 1) & 0x0f;

	// TC
	scsi.tc--;
	if (scsi.tc == 0) {
		// �]������(scsi.length != 0�Ȃ�ASCSI�o�X�̓��b�N)
		TransComplete();
		return data;
	}

	// �X�e�[�^�X�t�F�[�Y�ɂȂ��Ă���΁A�]������or�G���[
	if (scsi.phase == status) {
		// �]������
		TransComplete();
		return data;
	}

	// �ǂݍ��݌p��
	return data;
}

//---------------------------------------------------------------------------
//
//	DREG�ݒ�
//
//---------------------------------------------------------------------------
void FASTCALL SCSI::SetDREG(DWORD data)
{
	ASSERT(this);
	ASSERT(data < 0x100);
	ASSERT_DIAG();

	// �]�����łȂ���΁A�������Ȃ�
	if (!scsi.trans) {
		return;
	}

	// ���N�G�X�g���オ���Ă��Ȃ���΁A�������Ȃ�
	if (!scsi.req) {
		return;
	}

	// TC��0�Ȃ�A�������Ȃ�
	if (scsi.tc == 0) {
		return;
	}

	// REQ-ACK�n���h�V�F�[�N�������ōs��
	Xfer(&data);
	XferNext();

	// MBC
	scsi.mbc = (scsi.mbc - 1) & 0x0f;

	// TC
	scsi.tc--;
	if (scsi.tc == 0) {
		// �]������(scsi.length != 0�Ȃ�ASCSI�o�X�̓��b�N)
		TransComplete();
		return;
	}

	// �X�e�[�^�X�t�F�[�Y�ɂȂ��Ă���΁A�]������or�G���[
	if (scsi.phase == status) {
		// �]������
		TransComplete();
		return;
	}
}

//---------------------------------------------------------------------------
//
//	TEMP�ݒ�
//
//---------------------------------------------------------------------------
void FASTCALL SCSI::SetTEMP(DWORD data)
{
	ASSERT(this);
	ASSERT(data < 0x100);

	scsi.temp = data;

	// �Z���N�V�����t�F�[�Y�̏ꍇ�A0x00�̃Z�b�g�ɂ��SEL=0�ƂȂ�
	if (scsi.phase == selection) {
		if (data == 0x00) {
			// SEL=0, BSY=0
			scsi.sel = FALSE;
			scsi.bsy = FALSE;
		}
	}
}

//---------------------------------------------------------------------------
//
//	TCH�ݒ�
//
//---------------------------------------------------------------------------
void FASTCALL SCSI::SetTCH(DWORD data)
{
	ASSERT(this);
	ASSERT(data < 0x100);
	ASSERT_DIAG();

	// b23-b16�ɐݒ�
	scsi.tc &= 0x0000ffff;
	data <<= 16;
	scsi.tc |= data;

#if defined(SCSI_LOG)
	LOG1(Log::Normal, "TCH�ݒ� TC=$%06X", scsi.tc);
#endif	// SCSI_LOG
}

//---------------------------------------------------------------------------
//
//	TCM�ݒ�
//
//---------------------------------------------------------------------------
void FASTCALL SCSI::SetTCM(DWORD data)
{
	ASSERT(this);
	ASSERT(data < 0x100);

	// b15-b8�ɐݒ�
	scsi.tc &= 0x00ff00ff;
	data <<= 8;
	scsi.tc |= data;

#if defined(SCSI_LOG)
	LOG1(Log::Normal, "TCM�ݒ� TC=$%06X", scsi.tc);
#endif	// SCSI_LOG
}

//---------------------------------------------------------------------------
//
//	TCL�ݒ�
//
//---------------------------------------------------------------------------
void FASTCALL SCSI::SetTCL(DWORD data)
{
	ASSERT(this);
	ASSERT(data < 0x100);

	// b7-b0�ɐݒ�
	scsi.tc &= 0x00ffff00;
	scsi.tc |= data;

#if defined(SCSI_LOG)
	LOG1(Log::Normal, "TCL�ݒ� TC=$%06X", scsi.tc);
#endif	// SCSI_LOG

	// SCMD��$20�Ȃ�A�Z���N�V�����p��
	if ((scsi.scmd & 0xe0) == 0x20) {
		SelectTime();
	}
}

//---------------------------------------------------------------------------
//
//	�o�X�����[�X
//
//---------------------------------------------------------------------------
void FASTCALL SCSI::BusRelease()
{
	ASSERT(this);
	ASSERT_DIAG();

#if defined(SCSI_LOG)
	LOG0(Log::Normal, "Bus Release�R�}���h");
#endif	// SCSI_LOG

	// �o�X�t���[
	BusFree();
}

//---------------------------------------------------------------------------
//
//	�Z���N�g
//
//---------------------------------------------------------------------------
void FASTCALL SCSI::Select()
{
	ASSERT(this);
	ASSERT_DIAG();

#if defined(SCSI_LOG)
	LOG0(Log::Normal, "Select�R�}���h");
#endif	// SCSI_LOG

	// SPC���Z�b�g���ASCSI�o�X���Z�b�g���́A��������Ȃ�
	if (scsi.sctl & 0x80) {
		return;
	}
	if (scsi.rst) {
		return;
	}

	// SCTL��bit4��1�Ȃ�A�A�[�r�g���[�V�����t�F�[�Y����Ɏ��s�����
	if (scsi.sctl & 0x10) {
		Arbitration();
	}

	// PCTL��bit0��1�Ȃ�A���Z���N�V����(�^�[�Q�b�g�̂݋������)
	if (scsi.pctl & 1) {
		LOG0(Log::Warning, "���Z���N�V�����t�F�[�Y");
		BusFree();
		return;
	}

	// �Z���N�V�����t�F�[�Y
	Selection();
}

//---------------------------------------------------------------------------
//
//	ATN���Z�b�g
//
//---------------------------------------------------------------------------
void FASTCALL SCSI::ResetATN()
{
	ASSERT(this);
	ASSERT_DIAG();

#if defined(SCSI_LOG)
	LOG0(Log::Normal, "Reset ATN�R�}���h");
#endif	// SCSI_LOG

	// �C�j�V�G�[�^�����삷��M����
	scsi.atn = FALSE;
}

//---------------------------------------------------------------------------
//
//	ATN�Z�b�g
//
//---------------------------------------------------------------------------
void FASTCALL SCSI::SetATN()
{
	ASSERT(this);
	ASSERT_DIAG();

#if defined(SCSI_LOG)
	LOG0(Log::Normal, "Set ATN�R�}���h");
#endif	// SCSI_LOG

	// �C�j�V�G�[�^�����삷��M����
	scsi.atn = TRUE;
}

//---------------------------------------------------------------------------
//
//	�]��
//
//---------------------------------------------------------------------------
void FASTCALL SCSI::Transfer()
{
	ASSERT(this);
	ASSERT_DIAG();
	ASSERT(scsi.req);

#if defined(SCSI_LOG)
	LOG0(Log::Normal, "Transfer�R�}���h");
#endif	// SCSI_LOG

	// �f�o�C�X���̏������ł��Ă��邩
	if (scsi.length <= 0) {
		// Service Required(�t�F�[�Y������Ȃ�)
		Interrupt(3, TRUE);
		return;
	}

	// �t�F�[�Y����v���Ă��邩

	// �R���g���[�����̏������ł��Ă��邩
	if (scsi.tc == 0) {
		// �R���v���[�g(�]���I��)
		Interrupt(4, TRUE);
	}

	// �]���R�}���h�J�n
	scsi.trans = TRUE;
}

//---------------------------------------------------------------------------
//
//	�]�����f
//
//---------------------------------------------------------------------------
void FASTCALL SCSI::TransPause()
{
	ASSERT(this);
	ASSERT_DIAG();

	// �^�[�Q�b�g�̂ݔ��s�ł���
	LOG0(Log::Warning, "Transfer Pause�R�}���h");
}

//---------------------------------------------------------------------------
//
//	�]������
//
//---------------------------------------------------------------------------
void FASTCALL SCSI::TransComplete()
{
	ASSERT(this);
	ASSERT(!scsi.ack);
	ASSERT(scsi.trans);
	ASSERT_DIAG();

	// �]������
	scsi.trans = FALSE;

	// �]���������荞��
	Interrupt(4, TRUE);
}

//---------------------------------------------------------------------------
//
//	ACKREQ���Z�b�g
//
//---------------------------------------------------------------------------
void FASTCALL SCSI::ResetACKREQ()
{
	ASSERT(this);
	ASSERT_DIAG();

	// ACK���オ���Ă���ꍇ�݈̂Ӗ�������
	if (!scsi.ack) {
		return;
	}

	// ACK���オ��t�F�[�Y�͌�����
	ASSERT((scsi.phase >= command) && (scsi.phase != execute));

	// �f�[�^�]�������ɐi�߂�
	XferNext();
}

//---------------------------------------------------------------------------
//
//	ACK/REQ�Z�b�g
//
//---------------------------------------------------------------------------
void FASTCALL SCSI::SetACKREQ()
{
	ASSERT(this);
	ASSERT_DIAG();

	// REQ���オ���Ă���ꍇ�݈̂Ӗ�������
	if (!scsi.req) {
		return;
	}

	// REQ���オ��t�F�[�Y�͌�����
	ASSERT((scsi.phase >= command) && (scsi.phase != execute));

	// TEMP���W�X�^-SCSI�f�[�^�o�X�Ԃ̃f�[�^�]�����s��
	Xfer(&scsi.temp);
}

//---------------------------------------------------------------------------
//
//	�f�[�^�]��
//
//---------------------------------------------------------------------------
void FASTCALL SCSI::Xfer(DWORD *reg)
{
	ASSERT(this);
	ASSERT(reg);
	ASSERT_DIAG();

	// REQ���オ���Ă��邱��
	ASSERT(scsi.req);
	ASSERT(!scsi.ack);
	ASSERT((scsi.phase >= command) && (scsi.phase != execute));

	// �C�j�V�G�[�^�����삷��M����
	scsi.ack = TRUE;

	// SCSI�f�[�^�o�X�Ƀf�[�^���悹��
	switch (scsi.phase) {
		// �R�}���h�t�F�[�Y
		case command:
#if defined(SCSI_LOG)
			LOG1(Log::Normal, "�R�}���h�t�F�[�Y $%02X", *reg);
#endif	// SCSI_LOG
			// �ŏ��̃f�[�^(�I�t�Z�b�g0)�ɂ�背���O�X���Đݒ�
			scsi.cmd[scsi.offset] = *reg;
			if (scsi.offset == 0) {
				if (scsi.temp >= 0x20) {
					// 10�o�C�gCDB
					scsi.length = 10;
				}
			}
			break;

		// ���b�Z�[�W�C���t�F�[�Y
		case msgin:
			*reg = scsi.message;
#if defined(SCSI_LOG)
			LOG1(Log::Normal, "���b�Z�[�W�C���t�F�[�Y $%02X", *reg);
#endif	// SCSI_LOG
			break;

		// ���b�Z�[�W�A�E�g�t�F�[�Y
		case msgout:
			scsi.message = *reg;
#if defined(SCSI_LOG)
			LOG1(Log::Normal, "���b�Z�[�W�A�E�g�t�F�[�Y $%02X", *reg);
#endif	// SCSI_LOG
			break;

		// �f�[�^�C���t�F�[�Y
		case datain:
			*reg = (DWORD)scsi.buffer[scsi.offset];
			break;

		// �f�[�^�A�E�g�t�F�[�Y
		case dataout:
			scsi.buffer[scsi.offset] = (BYTE)*reg;
			break;

		// �X�e�[�^�X�t�F�[�Y
		case status:
			*reg = scsi.status;
#if defined(SCSI_LOG)
			LOG1(Log::Normal, "�X�e�[�^�X�t�F�[�Y $%02X", *reg);
#endif	// SCSI_LOG
			break;

		// ���̑�(���肦�Ȃ�)
		default:
			ASSERT(FALSE);
			break;
	}

	// �^�[�Q�b�g�����삷��M����
	scsi.req = FALSE;
}

//---------------------------------------------------------------------------
//
//	�f�[�^�]���p��
//
//---------------------------------------------------------------------------
void FASTCALL SCSI::XferNext()
{
	BOOL result;

	ASSERT(this);
	ASSERT_DIAG();

	// ACK���オ���Ă��邱��
	ASSERT(!scsi.req);
	ASSERT(scsi.ack);
	ASSERT((scsi.phase >= command) && (scsi.phase != execute));

	// �I�t�Z�b�g�ƃ����O�X
	ASSERT(scsi.length >= 1);
	scsi.offset++;
	scsi.length--;

	// �C�j�V�G�[�^�����삷��M����
	scsi.ack = FALSE;

	// �����O�X!=0�Ȃ�A�Ă�req���Z�b�g
	if (scsi.length != 0) {
		scsi.req = TRUE;
		return;
	}

	// �u���b�N���Z�A���U���g������
	scsi.blocks--;
	result = TRUE;

	// �f�[�^�󗝌�̏���(�t�F�[�Y��)
	switch (scsi.phase) {
		// �f�[�^�C���t�F�[�Y
		case datain:
			if (scsi.blocks != 0) {
				// ���̃o�b�t�@��ݒ�(offset, length���Z�b�g���邱��)
				result = XferIn();
			}
			break;

		// �f�[�^�A�E�g�t�F�[�Y
		case dataout:
			if (scsi.blocks == 0) {
				// ���̃o�b�t�@�ŏI��
				result = XferOut(FALSE);
			}
			else {
				// ���̃o�b�t�@�ɑ���(offset, length���Z�b�g���邱��)
				result = XferOut(TRUE);
			}
			break;

		// ���b�Z�[�W�A�E�g�t�F�[�Y
		case msgout:
			if (!XferMsg(scsi.message)) {
				// ���b�Z�[�W�A�E�g�Ɏ��s������A�����Ƀo�X�t���[
				BusFree();
				return;
			}
			// ���b�Z�[�W�C���ɔ����A���b�Z�[�W�f�[�^���N���A���Ă���
			scsi.message = 0x00;
			break;
	}

	// ���U���gFALSE�Ȃ�A�X�e�[�^�X�t�F�[�Y�ֈڍs
	if (!result) {
		Error();
		return;
	}

	// �u���b�N!=0�Ȃ�A�Ă�req���Z�b�g
	if (scsi.blocks != 0){
		ASSERT(scsi.length > 0);
		ASSERT(scsi.offset == 0);
		scsi.req = TRUE;
		return;
	}

	// ���t�F�[�Y�Ɉړ�
	switch (scsi.phase) {
		// �R�}���h�t�F�[�Y
		case command:
			// ���s�t�F�[�Y
			Execute();
			break;

		// ���b�Z�[�W�C���t�F�[�Y
		case msgin:
			// �o�X�t���[�t�F�[�Y
			BusFree();
			break;

		// ���b�Z�[�W�A�E�g�t�F�[�Y
		case msgout:
			// �R�}���h�t�F�[�Y
			Command();
			break;

		// �f�[�^�C���t�F�[�Y
		case datain:
			// �X�e�[�^�X�t�F�[�Y
			Status();
			break;

		// �f�[�^�A�E�g�t�F�[�Y
		case dataout:
			// �X�e�[�^�X�t�F�[�Y
			Status();
			break;

		// �X�e�[�^�X�t�F�[�Y
		case status:
			// ���b�Z�[�W�C���t�F�[�Y
			MsgIn();
			break;

		// ���̑�(���肦�Ȃ�)
		default:
			ASSERT(FALSE);
			break;
	}
}

//---------------------------------------------------------------------------
//
//	�f�[�^�]��IN
//	��offset, length���Đݒ肷�邱��
//
//---------------------------------------------------------------------------
BOOL FASTCALL SCSI::XferIn()
{
	ASSERT(this);
	ASSERT(scsi.phase == datain);
	ASSERT(scsi.disk[scsi.id]);
	ASSERT_DIAG();

	// �X�e�[�g���[�h�ɂ��f�B�X�N�������Ȃ����ꍇ
	if (!scsi.disk[scsi.id]) {
		// �f�[�^�C�����~
		return FALSE;
	}

	// READ�n�R�}���h�Ɍ���
	switch (scsi.cmd[0]) {
		// READ(6)
		case 0x08:
		// READ(10)
		case 0x28:
			// �f�B�X�N����ǂݎ����s��
			scsi.length = scsi.disk[scsi.id]->Read(scsi.buffer, scsi.next);
			scsi.next++;

			// �G���[�Ȃ�A�X�e�[�^�X�t�F�[�Y��
			if (scsi.length <= 0) {
				// �f�[�^�C�����~
				return FALSE;
			}

			// ����Ȃ�A���[�N�ݒ�
			scsi.offset = 0;
			break;

		// ���̑�(���肦�Ȃ�)
		default:
			ASSERT(FALSE);
			return FALSE;
	}

	// �o�b�t�@�ݒ�ɐ�������
	return TRUE;
}

//---------------------------------------------------------------------------
//
//	�f�[�^�]��OUT
//	��cont=TRUE�̏ꍇ�Aoffset, length���Đݒ肷�邱��
//
//---------------------------------------------------------------------------
BOOL FASTCALL SCSI::XferOut(BOOL cont)
{
	ASSERT(this);
	ASSERT(scsi.phase == dataout);
	ASSERT(scsi.disk[scsi.id]);
	ASSERT_DIAG();

	// �X�e�[�g���[�h�ɂ��f�B�X�N�������Ȃ����ꍇ
	if (!scsi.disk[scsi.id]) {
		// �f�[�^�A�E�g���~
		return FALSE;
	}

	// MODE SELECT�܂��́AWRITE�n
	switch (scsi.cmd[0]) {
		// MODE SELECT
		case 0x15:
			if (!scsi.disk[scsi.id]->ModeSelect(scsi.buffer, scsi.offset)) {
				// MODE SELECT�Ɏ��s
				return FALSE;
			}
			break;

		// WRITE(6)
		case 0x0a:
		// WRITE(10)
		case 0x2a:
			// �������݂��s��
			if (!scsi.disk[scsi.id]->Write(scsi.buffer, scsi.next - 1)) {
				// �������ݎ��s
				return FALSE;
			}

			// ���̃u���b�N���K�v�Ȃ��Ȃ炱���܂�
			scsi.next++;
			if (!cont) {
				break;
			}

			// ���̃u���b�N���`�F�b�N
			scsi.length = scsi.disk[scsi.id]->WriteCheck(scsi.next);
			if (scsi.length <= 0) {
				// �������݂ł��Ȃ�
				return FALSE;
			}

			// ����Ȃ�A���[�N�ݒ�
			scsi.offset = 0;
			break;
	}

	// �o�b�t�@�ۑ��ɐ�������
	return TRUE;
}

//---------------------------------------------------------------------------
//
//	�f�[�^�]��MSG
//
//---------------------------------------------------------------------------
BOOL FASTCALL SCSI::XferMsg(DWORD /*msg*/)
{
	ASSERT(this);
	ASSERT(scsi.phase == msgout);
	ASSERT_DIAG();

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	�o�X�t���[�t�F�[�Y
//
//---------------------------------------------------------------------------
void FASTCALL SCSI::BusFree()
{
	ASSERT(this);
	ASSERT_DIAG();

#if defined(SCSI_LOG)
	LOG0(Log::Normal, "�o�X�t���[�t�F�[�Y");
#endif	// SCSI_LOG

	// �t�F�[�Y�ݒ�
	scsi.phase = busfree;

	// �M����
	scsi.bsy = FALSE;
	scsi.sel = FALSE;
	scsi.atn = FALSE;
	scsi.msg = FALSE;
	scsi.cd = FALSE;
	scsi.io = FALSE;
	scsi.req = FALSE;
	scsi.ack = FALSE;

	// �X�e�[�^�X�ƃ��b�Z�[�W��������(GOOD)
	scsi.status = 0x00;
	scsi.message = 0x00;

	// �C�x���g��~
	event.SetTime(0);

	// PCTL��Busfree INT Enable���`�F�b�N
	if (scsi.pctl & 0x8000) {
		// Disconnect���荞��
		Interrupt(5, TRUE);
	}
	else {
		// ���荞�݃I�t
		Interrupt(5, FALSE);
	}
}

//---------------------------------------------------------------------------
//
//	�A�[�r�g���[�V�����t�F�[�Y
//
//---------------------------------------------------------------------------
void FASTCALL SCSI::Arbitration()
{
	ASSERT(this);
	ASSERT_DIAG();

#if defined(SCSI_LOG)
	LOG0(Log::Normal, "�A�[�r�g���[�V�����t�F�[�Y");
#endif	// SCSI_LOG

	// �t�F�[�Y�ݒ�
	scsi.phase = arbitration;

	// �M����
	scsi.bsy = TRUE;
	scsi.sel = TRUE;
}

//---------------------------------------------------------------------------
//
//	�Z���N�V�����t�F�[�Y
//
//---------------------------------------------------------------------------
void FASTCALL SCSI::Selection()
{
	int i;
	DWORD temp;

	ASSERT(this);
	ASSERT_DIAG();

	// �t�F�[�Y�ݒ�
	scsi.phase = selection;

	// �^�[�Q�b�gID������
	scsi.id = -1;

	// TEMP���W�X�^�ɂ̓C�j�V�G�[�^�ƃ^�[�Q�b�g�A�����̃r�b�g���K�v
	temp = scsi.temp;
	if ((temp & scsi.bdid) == scsi.bdid) {
		// �C�j�V�G�[�^�̃r�b�g���폜
		temp &= ~(scsi.bdid);

		// �^�[�Q�b�g��T��
		for (i=0; i<8; i++) {
			if (temp == (DWORD)(1 << i)) {
				scsi.id = i;
				break;
			}
		}
	}

#if defined(SCSI_LOG)
	if (scsi.id != -1) {
		if (scsi.disk[scsi.id]) {
			LOG1(Log::Normal, "�Z���N�V�����t�F�[�Y ID=%d (�f�o�C�X����)", scsi.id);
		}
		else {
			LOG1(Log::Normal, "�Z���N�V�����t�F�[�Y ID=%d (���ڑ�)", scsi.id);
		}
	}
	else {
		LOG0(Log::Normal, "�Z���N�V�����t�F�[�Y (����)");
	}
#endif	// SCSI_LOG

	// �Z���N�V�����^�C������
	SelectTime();
}

//---------------------------------------------------------------------------
//
//	�Z���N�V�����t�F�[�Y ���Ԑݒ�
//
//---------------------------------------------------------------------------
void FASTCALL SCSI::SelectTime()
{
	DWORD tc;

	ASSERT(this);
	ASSERT((scsi.scmd & 0xe0) == 0x20);
	ASSERT_DIAG();

	tc = scsi.tc;
	if (tc == 0) {
		LOG0(Log::Warning, "�Z���N�V�����^�C���A�E�g��������");
		tc = 0x1000000;
	}
	tc &= 0x00ffff00;
	tc += 15;

	// �����TCL���l������
	tc += (((scsi.tc & 0xff) + 7) >> 1);

	// 400ns�P�ʂ���A500ns�P��(hus)�֕ϊ�
	tc = (tc * 4 / 5);

	// �t�F�[�Y
	scsi.phase = selection;

	// �M����
	scsi.sel = TRUE;

	// �C�x���g�^�C���ݒ�
	event.SetDesc("Selection");
	if (scsi.id != -1) {
		if (scsi.disk[scsi.id]) {
			// �w��ID������̏ꍇ�A16us�ŃZ���N�V�����𐬌�������
			event.SetTime(32);
			return;
		}
	}

	// �^�C���A�E�g�̏ꍇ�ATC�ʂ�ɐݒ�
	event.SetTime(tc);
}

//---------------------------------------------------------------------------
//
//	�R�}���h�t�F�[�Y
//
//---------------------------------------------------------------------------
void FASTCALL SCSI::Command()
{
	ASSERT(this);
	ASSERT((scsi.id >= 0) && (scsi.id <= 7));
	ASSERT(scsi.disk[scsi.id]);
	ASSERT_DIAG();

#if defined(SCSI_LOG)
	LOG0(Log::Normal, "�R�}���h�t�F�[�Y");
#endif	// SCSI_LOG

	// �X�e�[�g���[�h�ɂ��f�B�X�N�������Ȃ����ꍇ
	if (!scsi.disk[scsi.id]) {
		// �R�}���h�t�F�[�Y���~
		BusFree();
		return;
	}

	// �t�F�[�Y�ݒ�
	scsi.phase = command;

	// �C�j�V�G�[�^�����삷��M����
	scsi.atn = FALSE;

	// �^�[�Q�b�g�����삷��M����
	scsi.msg = FALSE;
	scsi.cd = TRUE;
	scsi.io = FALSE;

	// �f�[�^�]����6�o�C�gx1�u���b�N
	scsi.offset = 0;
	scsi.length = 6;
	scsi.blocks = 1;

	// �R�}���h��v��
	scsi.req = TRUE;
}

//---------------------------------------------------------------------------
//
//	���s�t�F�[�Y
//
//---------------------------------------------------------------------------
void FASTCALL SCSI::Execute()
{
	ASSERT(this);
	ASSERT((scsi.id >= 0) && (scsi.id <= 7));
	ASSERT(scsi.disk[scsi.id]);
	ASSERT_DIAG();

#if defined(SCSI_LOG)
	LOG1(Log::Normal, "���s�t�F�[�Y �R�}���h$%02X", scsi.cmd[0]);
#endif	// SCSI_LOG

	// �X�e�[�g���[�h�ɂ��f�B�X�N�������Ȃ����ꍇ
	if (!scsi.disk[scsi.id]) {
		// �R�}���h�t�F�[�Y���~
		Error();
		return;
	}

	// �t�F�[�Y�ݒ�
	scsi.phase = execute;

	// �f�[�^�]���̂��߂̏�����
	scsi.offset = 0;
	scsi.blocks = 1;

	// �R�}���h�ʏ���
	switch (scsi.cmd[0]) {
		// TEST UNIT READY
		case 0x00:
			TestUnitReady();
			return;

		// REZERO
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

		// INQUIRY
		case 0x12:
			Inquiry();
			return;

		// MODE SELECT
		case 0x15:
			ModeSelect();
			return;

		// MDOE SENSE
		case 0x1a:
			ModeSense();
			return;

		// START STOP UNIT
		case 0x1b:
			StartStop();
			return;

		// SEND DIAGNOSTIC
		case 0x1d:
			SendDiag();
			return;

		// PREVENT/ALLOW MEDIUM REMOVAL
		case 0x1e:
			Removal();
			return;

		// READ CAPACITY
		case 0x25:
			ReadCapacity();
			return;

		// READ(10)
		case 0x28:
			Read10();
			return;

		// WRITE(10)
		case 0x2a:
			Write10();
			return;

		// SEEK(10)
		case 0x2b:
			Seek10();
			return;

		// WRITE AND VERIFY
		case 0x2e:
			Write10();
			return;

		// VERIFY
		case 0x2f:
			Verify();
			return;

		// READ TOC
		case 0x43:
			ReadToc();
			return;

		// PLAY AUDIO(10)
		case 0x45:
			PlayAudio10();
			return;

		// PLAY AUDIO MSF
		case 0x47:
			PlayAudioMSF();
			return;

		// PLAY AUDIO TRACK
		case 0x48:
			PlayAudioTrack();
			return;
	}

	// ����ȊO�͑Ή����Ă��Ȃ�
	LOG1(Log::Warning, "���Ή��R�}���h $%02X", scsi.cmd[0]);
	scsi.disk[scsi.id]->InvalidCmd();

	// �G���[
	Error();
}

//---------------------------------------------------------------------------
//
//	���b�Z�[�W�C���t�F�[�Y
//
//---------------------------------------------------------------------------
void FASTCALL SCSI::MsgIn()
{
	ASSERT(this);
	ASSERT_DIAG();

#if defined(SCSI_LOG)
	LOG0(Log::Normal, "���b�Z�[�W�C���t�F�[�Y");
#endif	// SCSI_LOG

	// �t�F�[�Y�ݒ�
	scsi.phase = msgin;

	// �^�[�Q�b�g�����삷��M����
	scsi.msg = TRUE;
	scsi.cd = TRUE;
	scsi.io = TRUE;

	// �f�[�^�]����1�o�C�gx1�u���b�N
	scsi.offset = 0;
	scsi.length = 1;
	scsi.blocks = 1;

	// ���b�Z�[�W��v��
	scsi.req = TRUE;
}

//---------------------------------------------------------------------------
//
//	���b�Z�[�W�A�E�g�t�F�[�Y
//
//---------------------------------------------------------------------------
void FASTCALL SCSI::MsgOut()
{
	ASSERT(this);
	ASSERT_DIAG();

#if defined(SCSI_LOG)
	LOG0(Log::Normal, "���b�Z�[�W�A�E�g�t�F�[�Y");
#endif	// SCSI_LOG

	// �t�F�[�Y�ݒ�
	scsi.phase = msgout;

	// �^�[�Q�b�g�����삷��M����
	scsi.msg = TRUE;
	scsi.cd = TRUE;
	scsi.io = FALSE;

	// �f�[�^�]����1�o�C�gx1�u���b�N
	scsi.offset = 0;
	scsi.length = 1;
	scsi.blocks = 1;

	// ���b�Z�[�W��v��
	scsi.req = TRUE;
}

//---------------------------------------------------------------------------
//
//	�f�[�^�C���t�F�[�Y
//
//---------------------------------------------------------------------------
void FASTCALL SCSI::DataIn()
{
	ASSERT(this);
	ASSERT(scsi.length >= 0);
	ASSERT_DIAG();

#if defined(SCSI_LOG)
	LOG0(Log::Normal, "�f�[�^�C���t�F�[�Y");
#endif	// SCSI_LOG

	// �����O�X0�Ȃ�A�X�e�[�^�X�t�F�[�Y��
	if (scsi.length == 0) {
		Status();
		return;
	}

	// �t�F�[�Y�ݒ�
	scsi.phase = datain;

	// �^�[�Q�b�g�����삷��M����
	scsi.msg = FALSE;
	scsi.cd = FALSE;
	scsi.io = TRUE;

	// length, blocks�͐ݒ�ς�
	ASSERT(scsi.length > 0);
	ASSERT(scsi.blocks > 0);
	scsi.offset = 0;

	// �f�[�^��v��
	scsi.req = TRUE;
}

//---------------------------------------------------------------------------
//
//	�f�[�^�A�E�g�t�F�[�Y
//
//---------------------------------------------------------------------------
void FASTCALL SCSI::DataOut()
{
	ASSERT(this);
	ASSERT(scsi.length >= 0);
	ASSERT_DIAG();

#if defined(SCSI_LOG)
	LOG0(Log::Normal, "�f�[�^�A�E�g�t�F�[�Y");
#endif	// SCSI_LOG

	// �����O�X0�Ȃ�A�X�e�[�^�X�t�F�[�Y��
	if (scsi.length == 0) {
		Status();
		return;
	}

	// �t�F�[�Y�ݒ�
	scsi.phase = dataout;

	// �^�[�Q�b�g�����삷��M����
	scsi.msg = FALSE;
	scsi.cd = FALSE;
	scsi.io = FALSE;

	// length, blocks�͐ݒ�ς�
	ASSERT(scsi.length > 0);
	ASSERT(scsi.blocks > 0);
	scsi.offset = 0;

	// �f�[�^��v��
	scsi.req = TRUE;
}

//---------------------------------------------------------------------------
//
//	�X�e�[�^�X�t�F�[�Y
//
//---------------------------------------------------------------------------
void FASTCALL SCSI::Status()
{
	ASSERT(this);
	ASSERT_DIAG();

#if defined(SCSI_LOG)
	LOG0(Log::Normal, "�X�e�[�^�X�t�F�[�Y");
#endif	// SCSI_LOG

	// �t�F�[�Y�ݒ�
	scsi.phase = status;

	// �^�[�Q�b�g�����삷��M����
	scsi.msg = FALSE;
	scsi.cd = TRUE;
	scsi.io = TRUE;

	// �f�[�^�]����1�o�C�gx1�u���b�N
	scsi.offset = 0;
	scsi.length = 1;
	scsi.blocks = 1;

	// �X�e�[�^�X��v��
	scsi.req = TRUE;
}

//---------------------------------------------------------------------------
//
//	���荞�ݗv��
//
//---------------------------------------------------------------------------
void FASTCALL SCSI::Interrupt(int type, BOOL flag)
{
	DWORD ints;

	ASSERT(this);
	ASSERT((type >= 0) && (type <= 7));
	ASSERT_DIAG();

	// INTS�̃o�b�N�A�b�v�����
	ints = scsi.ints;

	// INTS���W�X�^�ݒ�
	if (flag) {
		// ���荞�ݗv��
		scsi.ints |= (1 << type);
	}
	else {
		// ���荞�݃L�����Z��
		scsi.ints &= ~(1 << type);
	}

	// �ω����Ă���΁A���荞�݃`�F�b�N
	if (ints != scsi.ints) {
		IntCheck();
	}
}

//---------------------------------------------------------------------------
//
//	���荞�݃`�F�b�N
//
//---------------------------------------------------------------------------
void FASTCALL SCSI::IntCheck()
{
	int v;
	int lev;

	ASSERT(this);
	ASSERT_DIAG();

	// ���荞�݃��x���ݒ�
	if (scsi.type == 1) {
		// �O�t(���x����2�܂���4����I��)
		lev = scsi.ilevel;
	}
	else {
		// ����(���x����1�Œ�)
		lev = 1;
	}

	// �v���x�N�^�쐬
	v = -1;
	if (scsi.sctl & 0x01) {
		// ���荞�݋���
		if (scsi.ints != 0) {
			// ���荞�ݗv������B�x�N�^���쐬
			if (scsi.type == 1) {
				// �O�t(�x�N�^��$F6)
				v = 0xf6;
			}
			else {
				// ����(�x�N�^��$6C)
				v = 0x6c;
			}
		}
	}

	// ��v���Ă����OK
	if (scsi.vector == v) {
		return;
	}

	// ���Ɋ��荞�ݗv������Ă���΁A�L�����Z��
	if (scsi.vector >= 0) {
		cpu->IntCancel(lev);
	}

	// �L�����Z���v���Ȃ�A�����܂�
	if (v < 0) {
#if defined(SCSI_LOG)
		LOG2(Log::Normal, "���荞�݃L�����Z�� ���x��%d �x�N�^$%02X",
							lev, scsi.vector);
#endif	// SCSI_LOG
		scsi.vector = -1;
		return;
	}

	// ���荞�ݗv��
#if defined(SCSI_LOG)
	LOG3(Log::Normal, "���荞�ݗv�� ���x��%d �x�N�^$%02X INTS=%02X",
						lev, v, scsi.ints);
#endif	// SCSI_LOG
	cpu->Interrupt(lev, v);
	scsi.vector = v;
}

//---------------------------------------------------------------------------
//
//	���荞��ACK
//
//---------------------------------------------------------------------------
void FASTCALL SCSI::IntAck(int level)
{
	ASSERT(this);
	ASSERT((level == 1) || (level == 2) || (level == 4));
	ASSERT_DIAG();

	// ���Z�b�g����ɁACPU���犄�荞�݂��Ԉ���ē���ꍇ���A�܂��͑��̃f�o�C�X
	if (scsi.vector < 0) {
		return;
	}

	// ���荞�݃��x�����قȂ��Ă���΁A���̃f�o�C�X
	if (scsi.type == 1) {
		// �O�t(���x����2�܂���4����I��)
		if (level != scsi.ilevel) {
			return;
		}
	}
	else {
		// ����(���x����1�Œ�)
		if (level != 1) {
			return;
		}
	}

	// ���荞�݂Ȃ�
	scsi.vector = -1;

	// ���荞�݃`�F�b�N
	IntCheck();
}

//---------------------------------------------------------------------------
//
//	SCSI-ID�擾
//
//---------------------------------------------------------------------------
int FASTCALL SCSI::GetSCSIID() const
{
	int id;
	DWORD bdid;

	ASSERT(this);
	ASSERT_DIAG();

	// ���݂��Ȃ����7�Œ�
	if (scsi.type == 0) {
		return 7;
	}

	// BDID����쐬
	ASSERT(scsi.bdid != 0);
	ASSERT(scsi.bdid < 0x100);

	// ������
	id = 0;
	bdid = scsi.bdid;

	// �r�b�g���琔�l�֕ϊ�
	while ((bdid & 1) == 0) {
		bdid >>= 1;
		id++;
	}

	ASSERT((id >= 0) && (id <= 7));
	return id;
}

//---------------------------------------------------------------------------
//
//	BUSY�`�F�b�N
//
//---------------------------------------------------------------------------
BOOL FASTCALL SCSI::IsBusy() const
{
	ASSERT(this);
	ASSERT_DIAG();

	// BSY�������Ă��Ȃ����FALSE
	if (!scsi.bsy) {
		return FALSE;
	}

	// �Z���N�V�����t�F�[�Y�Ȃ�FALSE
	if (scsi.phase == selection) {
		return FALSE;
	}

	// BUSY
	return TRUE;
}

//---------------------------------------------------------------------------
//
//	BUSY�`�F�b�N
//
//---------------------------------------------------------------------------
DWORD FASTCALL SCSI::GetBusyDevice() const
{
	ASSERT(this);
	ASSERT_DIAG();

	// BSY�������Ă��Ȃ����0
	if (!scsi.bsy) {
		return 0;
	}

	// �Z���N�V�����t�F�[�Y�Ȃ�0
	if (scsi.phase == selection) {
		return 0;
	}

	// �Z���N�V�������I����Ă���̂ŁA�ʏ�̓f�o�C�X�����݂���
	ASSERT((scsi.id >= 0) && (scsi.id <= 7));
	ASSERT(scsi.disk[scsi.id]);

	// �f�o�C�X���Ȃ����0
	if (!scsi.disk[scsi.id]) {
		return 0;
	}

	return scsi.disk[scsi.id]->GetID();
}

//---------------------------------------------------------------------------
//
//	���ʃG���[
//
//---------------------------------------------------------------------------
void FASTCALL SCSI::Error()
{
	ASSERT(this);
	ASSERT_DIAG();

#if defined(SCSI_LOG)
	LOG0(Log::Normal, "�G���[(�X�e�[�^�X�t�F�[�Y��)");
#endif	// SCSI_LOG

	// �X�e�[�^�X�ƃ��b�Z�[�W��ݒ�(CHECK CONDITION)
	scsi.status = 0x02;
	scsi.message = 0x00;

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
void FASTCALL SCSI::TestUnitReady()
{
	BOOL status;

	ASSERT(this);
	ASSERT(scsi.disk[scsi.id]);
	ASSERT_DIAG();

#if defined(SCSI_LOG)
	LOG0(Log::Normal, "TEST UNIT READY�R�}���h");
#endif	// SCSI_LOG

	// �h���C�u�ŃR�}���h����
	status = scsi.disk[scsi.id]->TestUnitReady(scsi.cmd);
	if (!status) {
		// ���s(�G���[)
		Error();
		return;
	}

	// �X�e�[�^�X�t�F�[�Y
	Status();
}

//---------------------------------------------------------------------------
//
//	REZERO UNIT
//
//---------------------------------------------------------------------------
void FASTCALL SCSI::Rezero()
{
	BOOL status;

	ASSERT(this);
	ASSERT(scsi.disk[scsi.id]);
	ASSERT_DIAG();

#if defined(SCSI_LOG)
	LOG0(Log::Normal, "REZERO UNIT�R�}���h");
#endif	// SCSI_LOG

	// �h���C�u�ŃR�}���h����
	status = scsi.disk[scsi.id]->Rezero(scsi.cmd);
	if (!status) {
		// ���s(�G���[)
		Error();
		return;
	}

	// �X�e�[�^�X�t�F�[�Y
	Status();
}

//---------------------------------------------------------------------------
//
//	REQUEST SENSE
//
//---------------------------------------------------------------------------
void FASTCALL SCSI::RequestSense()
{
	ASSERT(this);
	ASSERT(scsi.disk[scsi.id]);
	ASSERT_DIAG();

#if defined(SCSI_LOG)
	LOG0(Log::Normal, "REQUEST SENSE�R�}���h");
#endif	// SCSI_LOG

	// �h���C�u�ŃR�}���h����
	scsi.length = scsi.disk[scsi.id]->RequestSense(scsi.cmd, scsi.buffer);
	ASSERT(scsi.length > 0);

#if defined(SCSI_LOG)
	LOG1(Log::Normal, "�Z���X�L�[ $%02X", scsi.buffer[2]);
#endif

	// �f�[�^�C���t�F�[�Y
	DataIn();
}

//---------------------------------------------------------------------------
//
//	FORMAT UNIT
//
//---------------------------------------------------------------------------
void FASTCALL SCSI::Format()
{
	BOOL status;

	ASSERT(this);
	ASSERT(scsi.disk[scsi.id]);
	ASSERT_DIAG();

#if defined(SCSI_LOG)
	LOG0(Log::Normal, "FORMAT UNIT�R�}���h");
#endif	// SCSI_LOG

	// �h���C�u�ŃR�}���h����
	status = scsi.disk[scsi.id]->Format(scsi.cmd);
	if (!status) {
		// ���s(�G���[)
		Error();
		return;
	}

	// �X�e�[�^�X�t�F�[�Y
	Status();
}

//---------------------------------------------------------------------------
//
//	REASSIGN BLOCKS
//
//---------------------------------------------------------------------------
void FASTCALL SCSI::Reassign()
{
	BOOL status;

	ASSERT(this);
	ASSERT(scsi.disk[scsi.id]);
	ASSERT_DIAG();

#if defined(SCSI_LOG)
	LOG0(Log::Normal, "REASSIGN BLOCKS�R�}���h");
#endif	// SCSI_LOG

	// �h���C�u�ŃR�}���h����
	status = scsi.disk[scsi.id]->Reassign(scsi.cmd);
	if (!status) {
		// ���s(�G���[)
		Error();
		return;
	}

	// �X�e�[�^�X�t�F�[�Y
	Status();
}

//---------------------------------------------------------------------------
//
//	READ(6)
//
//---------------------------------------------------------------------------
void FASTCALL SCSI::Read6()
{
	DWORD record;

	ASSERT(this);
	ASSERT(scsi.disk[scsi.id]);
	ASSERT_DIAG();

	// ���R�[�h�ԍ��ƃu���b�N�����擾
	record = scsi.cmd[1] & 0x1f;
	record <<= 8;
	record |= scsi.cmd[2];
	record <<= 8;
	record |= scsi.cmd[3];
	scsi.blocks = scsi.cmd[4];
	if (scsi.blocks == 0) {
		scsi.blocks = 0x100;
	}

#if defined(SCSI_LOG)
	LOG2(Log::Normal, "READ(6)�R�}���h ���R�[�h=%06X �u���b�N=%d", record, scsi.blocks);
#endif	// SCSI_LOG

	// �h���C�u�ŃR�}���h����
	scsi.length = scsi.disk[scsi.id]->Read(scsi.buffer, record);
	if (scsi.length <= 0) {
		// ���s(�G���[)
		Error();
		return;
	}

	// ���̃u���b�N��ݒ�
	scsi.next = record + 1;

	// �f�[�^�C���t�F�[�Y
	DataIn();
}

//---------------------------------------------------------------------------
//
//	WRITE(6)
//
//---------------------------------------------------------------------------
void FASTCALL SCSI::Write6()
{
	DWORD record;

	ASSERT(this);
	ASSERT(scsi.disk[scsi.id]);
	ASSERT_DIAG();

	// ���R�[�h�ԍ��ƃu���b�N�����擾
	record = scsi.cmd[1] & 0x1f;
	record <<= 8;
	record |= scsi.cmd[2];
	record <<= 8;
	record |= scsi.cmd[3];
	scsi.blocks = scsi.cmd[4];
	if (scsi.blocks == 0) {
		scsi.blocks = 0x100;
	}

#if defined(SCSI_LOG)
	LOG2(Log::Normal, "WRITE(6)�R�}���h ���R�[�h=%06X �u���b�N=%d", record, scsi.blocks);
#endif	// SCSI_LOG

	// �h���C�u�ŃR�}���h����
	scsi.length = scsi.disk[scsi.id]->WriteCheck(record);
	if (scsi.length <= 0) {
		// ���s(�G���[)
		Error();
		return;
	}

	// ���̃u���b�N��ݒ�
	scsi.next = record + 1;

	// �f�[�^�A�E�g�t�F�[�Y
	DataOut();
}

//---------------------------------------------------------------------------
//
//	SEEK(6)
//
//---------------------------------------------------------------------------
void FASTCALL SCSI::Seek6()
{
	BOOL status;

	ASSERT(this);
	ASSERT(scsi.disk[scsi.id]);
	ASSERT_DIAG();

#if defined(SCSI_LOG)
	LOG0(Log::Normal, "SEEK(6)�R�}���h");
#endif	// SCSI_LOG

	// �h���C�u�ŃR�}���h����
	status = scsi.disk[scsi.id]->Seek(scsi.cmd);
	if (!status) {
		// ���s(�G���[)
		Error();
		return;
	}

	// �X�e�[�^�X�t�F�[�Y
	Status();
}

//---------------------------------------------------------------------------
//
//	INQUIRY
//
//---------------------------------------------------------------------------
void FASTCALL SCSI::Inquiry()
{
	ASSERT(this);
	ASSERT(scsi.disk[scsi.id]);
	ASSERT_DIAG();

#if defined(SCSI_LOG)
	LOG0(Log::Normal, "INQUIRY�R�}���h");
#endif	// SCSI_LOG

	// �h���C�u�ŃR�}���h����
	scsi.length = scsi.disk[scsi.id]->Inquiry(scsi.cmd, scsi.buffer);
	if (scsi.length <= 0) {
		// ���s(�G���[)
		Error();
		return;
	}

	// �f�[�^�C���t�F�[�Y
	DataIn();
}

//---------------------------------------------------------------------------
//
//	MODE SELECT
//
//---------------------------------------------------------------------------
void FASTCALL SCSI::ModeSelect()
{
	ASSERT(this);
	ASSERT(scsi.disk[scsi.id]);
	ASSERT_DIAG();

#if defined(SCSI_LOG)
	LOG0(Log::Normal, "MODE SELECT�R�}���h");
#endif	// SCSI_LOG

	// �h���C�u�ŃR�}���h����
	scsi.length = scsi.disk[scsi.id]->SelectCheck(scsi.cmd);
	if (scsi.length <= 0) {
		// ���s(�G���[)
		Error();
		return;
	}

	// �f�[�^�A�E�g�t�F�[�Y
	DataOut();
}

//---------------------------------------------------------------------------
//
//	MODE SENSE
//
//---------------------------------------------------------------------------
void FASTCALL SCSI::ModeSense()
{
	ASSERT(this);
	ASSERT(scsi.disk[scsi.id]);
	ASSERT_DIAG();

#if defined(SCSI_LOG)
	LOG0(Log::Normal, "MODE SENSE�R�}���h");
#endif	// SCSI_LOG

	// �h���C�u�ŃR�}���h����
	scsi.length = scsi.disk[scsi.id]->ModeSense(scsi.cmd, scsi.buffer);
	ASSERT(scsi.length >= 0);
	if (scsi.length == 0) {
		LOG1(Log::Warning, "�T�|�[�g���Ă��Ȃ�MODE SENSE�y�[�W $%02X", scsi.cmd[2]);

		// ���s(�G���[)
		Error();
		return;
	}

	// �f�[�^�C���t�F�[�Y
	DataIn();
}

//---------------------------------------------------------------------------
//
//	START STOP UNIT
//
//---------------------------------------------------------------------------
void FASTCALL SCSI::StartStop()
{
	BOOL status;

	ASSERT(this);
	ASSERT(scsi.disk[scsi.id]);
	ASSERT_DIAG();

#if defined(SCSI_LOG)
	LOG0(Log::Normal, "START STOP UNIT�R�}���h");
#endif	// SCSI_LOG

	// �h���C�u�ŃR�}���h����
	status = scsi.disk[scsi.id]->StartStop(scsi.cmd);
	if (!status) {
		// ���s(�G���[)
		Error();
		return;
	}

	// �X�e�[�^�X�t�F�[�Y
	Status();
}

//---------------------------------------------------------------------------
//
//	SEND DIAGNOSTIC
//
//---------------------------------------------------------------------------
void FASTCALL SCSI::SendDiag()
{
	BOOL status;

	ASSERT(this);
	ASSERT(scsi.disk[scsi.id]);
	ASSERT_DIAG();

#if defined(SCSI_LOG)
	LOG0(Log::Normal, "SEND DIAGNOSTIC�R�}���h");
#endif	// SCSI_LOG

	// �h���C�u�ŃR�}���h����
	status = scsi.disk[scsi.id]->SendDiag(scsi.cmd);
	if (!status) {
		// ���s(�G���[)
		Error();
		return;
	}

	// �X�e�[�^�X�t�F�[�Y
	Status();
}

//---------------------------------------------------------------------------
//
//	PREVENT/ALLOW MEDIUM REMOVAL
//
//---------------------------------------------------------------------------
void FASTCALL SCSI::Removal()
{
	BOOL status;

	ASSERT(this);
	ASSERT(scsi.disk[scsi.id]);
	ASSERT_DIAG();

#if defined(SCSI_LOG)
	LOG0(Log::Normal, "PREVENT/ALLOW MEDIUM REMOVAL�R�}���h");
#endif	// SCSI_LOG

	// �h���C�u�ŃR�}���h����
	status = scsi.disk[scsi.id]->Removal(scsi.cmd);
	if (!status) {
		// ���s(�G���[)
		Error();
		return;
	}

	// �X�e�[�^�X�t�F�[�Y
	Status();
}

//---------------------------------------------------------------------------
//
//	READ CAPACITY
//
//---------------------------------------------------------------------------
void FASTCALL SCSI::ReadCapacity()
{
	int length;

	ASSERT(this);
	ASSERT(scsi.disk[scsi.id]);
	ASSERT_DIAG();

#if defined(SCSI_LOG)
	LOG0(Log::Normal, "READ CAPACITY�R�}���h");
#endif	// SCSI_LOG

	// �h���C�u�ŃR�}���h����
	length = scsi.disk[scsi.id]->ReadCapacity(scsi.cmd, scsi.buffer);
	ASSERT(length >= 0);
	if (length <= 0) {
		Error();
		return;
	}

	// �����O�X�ݒ�
	scsi.length = length;

	// �f�[�^�C���t�F�[�Y
	DataIn();
}

//---------------------------------------------------------------------------
//
//	READ(10)
//
//---------------------------------------------------------------------------
void FASTCALL SCSI::Read10()
{
	DWORD record;

	ASSERT(this);
	ASSERT(scsi.disk[scsi.id]);
	ASSERT_DIAG();

	// ���R�[�h�ԍ��ƃu���b�N�����擾
	record = scsi.cmd[2];
	record <<= 8;
	record |= scsi.cmd[3];
	record <<= 8;
	record |= scsi.cmd[4];
	record <<= 8;
	record |= scsi.cmd[5];
	scsi.blocks = scsi.cmd[7];
	scsi.blocks <<= 8;
	scsi.blocks |= scsi.cmd[8];

#if defined(SCSI_LOG)
	LOG2(Log::Normal, "READ(10)�R�}���h ���R�[�h=%08X �u���b�N=%d", record, scsi.blocks);
#endif	// SCSI_LOG

	// �u���b�N��0�͏������Ȃ�
	if (scsi.blocks == 0) {
		Status();
		return;
	}

	// �h���C�u�ŃR�}���h����
	scsi.length = scsi.disk[scsi.id]->Read(scsi.buffer, record);
	if (scsi.length <= 0) {
		// ���s(�G���[)
		Error();
		return;
	}

	// ���̃u���b�N��ݒ�
	scsi.next = record + 1;

	// �f�[�^�C���t�F�[�Y
	DataIn();
}

//---------------------------------------------------------------------------
//
//	WRITE(10)
//
//---------------------------------------------------------------------------
void FASTCALL SCSI::Write10()
{
	DWORD record;

	ASSERT(this);
	ASSERT(scsi.disk[scsi.id]);
	ASSERT_DIAG();

	// ���R�[�h�ԍ��ƃu���b�N�����擾
	record = scsi.cmd[2];
	record <<= 8;
	record |= scsi.cmd[3];
	record <<= 8;
	record |= scsi.cmd[4];
	record <<= 8;
	record |= scsi.cmd[5];
	scsi.blocks = scsi.cmd[7];
	scsi.blocks <<= 8;
	scsi.blocks |= scsi.cmd[8];

#if defined(SCSI_LOG)
	LOG2(Log::Normal, "WRTIE(10)�R�}���h ���R�[�h=%08X �u���b�N=%d", record, scsi.blocks);
#endif	// SCSI_LOG

	// �u���b�N��0�͏������Ȃ�
	if (scsi.blocks == 0) {
		Status();
		return;
	}

	// �h���C�u�ŃR�}���h����
	scsi.length = scsi.disk[scsi.id]->WriteCheck(record);
	if (scsi.length <= 0) {
		// ���s(�G���[)
		Error();
		return;
	}

	// ���̃u���b�N��ݒ�
	scsi.next = record + 1;

	// �f�[�^�A�E�g�t�F�[�Y
	DataOut();
}

//---------------------------------------------------------------------------
//
//	SEEK(10)
//
//---------------------------------------------------------------------------
void FASTCALL SCSI::Seek10()
{
	BOOL status;

	ASSERT(this);
	ASSERT(scsi.disk[scsi.id]);
	ASSERT_DIAG();

#if defined(SCSI_LOG)
	LOG0(Log::Normal, "SEEK(10)�R�}���h");
#endif	// SCSI_LOG

	// �h���C�u�ŃR�}���h����
	status = scsi.disk[scsi.id]->Seek(scsi.cmd);
	if (!status) {
		// ���s(�G���[)
		Error();
		return;
	}

	// �X�e�[�^�X�t�F�[�Y
	Status();
}

//---------------------------------------------------------------------------
//
//	VERIFY
//
//---------------------------------------------------------------------------
void FASTCALL SCSI::Verify()
{
	BOOL status;
	DWORD record;

	ASSERT(this);
	ASSERT(scsi.disk[scsi.id]);
	ASSERT_DIAG();

	// ���R�[�h�ԍ��ƃu���b�N�����擾
	record = scsi.cmd[2];
	record <<= 8;
	record |= scsi.cmd[3];
	record <<= 8;
	record |= scsi.cmd[4];
	record <<= 8;
	record |= scsi.cmd[5];
	scsi.blocks = scsi.cmd[7];
	scsi.blocks <<= 8;
	scsi.blocks |= scsi.cmd[8];

#if defined(SCSI_LOG)
	LOG2(Log::Normal, "VERIFY�R�}���h ���R�[�h=%08X �u���b�N=%d", record, scsi.blocks);
#endif	// SCSI_LOG

	// �u���b�N��0�͏������Ȃ�
	if (scsi.blocks == 0) {
		Status();
		return;
	}

	// BytChk=0�Ȃ�
	if ((scsi.cmd[1] & 0x02) == 0) {
		// �h���C�u�ŃR�}���h����
		status = scsi.disk[scsi.id]->Seek(scsi.cmd);
		if (!status) {
			// ���s(�G���[)
			Error();
			return;
		}

		// �X�e�[�^�X�t�F�[�Y
		Status();
		return;
	}

	// �e�X�g�ǂݍ���
	scsi.length = scsi.disk[scsi.id]->Read(scsi.buffer, record);
	if (scsi.length <= 0) {
		// ���s(�G���[)
		Error();
		return;
	}

	// ���̃u���b�N��ݒ�
	scsi.next = record + 1;

	// �f�[�^�A�E�g�t�F�[�Y
	DataOut();
}

//---------------------------------------------------------------------------
//
//	READ TOC
//
//---------------------------------------------------------------------------
void FASTCALL SCSI::ReadToc()
{
	ASSERT(this);
	ASSERT(scsi.disk[scsi.id]);
	ASSERT_DIAG();

	// �h���C�u�ŃR�}���h����
	scsi.length = scsi.disk[scsi.id]->ReadToc(scsi.cmd, scsi.buffer);
	if (scsi.length <= 0) {
		// ���s(�G���[)
		Error();
		return;
	}

	// �f�[�^�C���t�F�[�Y
	DataIn();
}

//---------------------------------------------------------------------------
//
//	PLAY AUDIO(10)
//
//---------------------------------------------------------------------------
void FASTCALL SCSI::PlayAudio10()
{
	BOOL status;

	ASSERT(this);
	ASSERT(scsi.disk[scsi.id]);
	ASSERT_DIAG();

	// �h���C�u�ŃR�}���h����
	status = scsi.disk[scsi.id]->PlayAudio(scsi.cmd);
	if (!status) {
		// ���s(�G���[)
		Error();
		return;
	}

	// CD-DA�C�x���g�X�^�[�g(26666�~74+26716�ŁA���v1sec)
	if (cdda.GetTime() == 0) {
		cdda.SetTime(26666);
	}

	// �X�e�[�^�X�t�F�[�Y
	Status();
}

//---------------------------------------------------------------------------
//
//	PLAY AUDIO MSF
//
//---------------------------------------------------------------------------
void FASTCALL SCSI::PlayAudioMSF()
{
	BOOL status;

	ASSERT(this);
	ASSERT(scsi.disk[scsi.id]);
	ASSERT_DIAG();

	// �h���C�u�ŃR�}���h����
	status = scsi.disk[scsi.id]->PlayAudioMSF(scsi.cmd);
	if (!status) {
		// ���s(�G���[)
		Error();
		return;
	}

	// CD-DA�C�x���g�X�^�[�g(26666�~74+26716�ŁA���v1sec)
	if (cdda.GetTime() == 0) {
		cdda.SetTime(26666);
	}

	// �X�e�[�^�X�t�F�[�Y
	Status();
}

//---------------------------------------------------------------------------
//
//	PLAY AUDIO TRACK
//
//---------------------------------------------------------------------------
void FASTCALL SCSI::PlayAudioTrack()
{
	BOOL status;

	ASSERT(this);
	ASSERT(scsi.disk[scsi.id]);
	ASSERT_DIAG();

	// �h���C�u�ŃR�}���h����
	status = scsi.disk[scsi.id]->PlayAudioTrack(scsi.cmd);
	if (!status) {
		// ���s(�G���[)
		Error();
		return;
	}

	// CD-DA�C�x���g�X�^�[�g(26666�~74+26716�ŁA���v1sec)
	if (cdda.GetTime() == 0) {
		cdda.SetTime(26666);
	}

	// �X�e�[�^�X�t�F�[�Y
	Status();
}

//---------------------------------------------------------------------------
//
//	�f�B�X�N�č\�z
//
//---------------------------------------------------------------------------
void FASTCALL SCSI::Construct()
{
	int hd;
	BOOL mo;
	BOOL cd;
	int i;
	int init;
	int index;
	Filepath path;

	ASSERT(this);
	ASSERT_DIAG();

	// ��U�N���A
	hd = 0;
	mo = FALSE;
	cd = FALSE;
	for (i=0; i<DeviceMax; i++) {
		scsi.disk[i] = NULL;
	}

	// SCSI�����݂��Ȃ��Ȃ�A�f�B�X�N�폜���ďI��
	if (scsi.type == 0) {
		for (i=0; i<HDMax; i++) {
			if (scsi.hd[i]) {
				delete scsi.hd[i];
				scsi.hd[i] = NULL;
			}
			if (scsi.mo) {
				delete scsi.mo;
				scsi.mo = NULL;
			}
			if (scsi.cdrom) {
				delete scsi.cdrom;
				scsi.cdrom = NULL;
			}
		}
		return;
	}

	// �f�B�X�N��������
	switch (scsi.scsi_drives) {
		// 0��
		case 0:
			break;

		// 1��
		case 1:
			// MO�D�悩�AHD�D�悩�ŕ�����
			if (scsi.mo_first) {
				mo = TRUE;
			}
			else {
				hd = 1;
			}
			break;

		// 2��
		case 2:
			// HD,MO�Ƃ�1��
			hd = 1;
			mo = TRUE;
			break;

		// 3��
		case 3:
			// HD,MO,CD�Ƃ�1��
			hd = 1;
			mo = TRUE;
			cd = TRUE;

		// 4��ȏ�
		default:
			ASSERT(scsi.scsi_drives <= 7);
			hd = scsi.scsi_drives - 2;
			mo = TRUE;
			cd = TRUE;
			break;
	}

	// HD�쐬
	for (i=0; i<HDMax; i++) {
		// ���𒴂��Ă���΁A�폜
		if (i >= hd) {
			if (scsi.hd[i]) {
				delete scsi.hd[i];
				scsi.hd[i] = NULL;
			}
			continue;
		}

		// �f�B�X�N��ʂ��m�F
		if (scsi.hd[i]) {
			if (scsi.hd[i]->GetID() != MAKEID('S', 'C', 'H', 'D')) {
				delete scsi.hd[i];
				scsi.hd[i] = NULL;
			}
		}

		// �f�B�X�N��������
		if (scsi.hd[i]) {
			// �p�X���擾���A��v�����ok
			scsi.hd[i]->GetPath(path);
			if (path.CmpPath(scsihd[i])) {
				// �p�X����v���Ă���
				continue;
			}

			// �p�X���Ⴄ�̂ŁA�f�B�X�N����������폜
			delete scsi.hd[i];
			scsi.hd[i] = NULL;
		}

		// SCSI�n�[�h�f�B�X�N���쐬���ăI�[�v�������݂�
		scsi.hd[i] = new SCSIHD(this);
		if (!scsi.hd[i]->Open(scsihd[i])) {
			// �I�[�v�����s�B���̔ԍ���scsi.disk��NULL�Ƃ���
			delete scsi.hd[i];
			scsi.hd[i] = NULL;
		}
	}

	// MO�쐬
	if (mo) {
		// MO����
		if (scsi.mo) {
			if (scsi.mo->GetID() != MAKEID('S', 'C', 'M', 'O')) {
				delete scsi.mo;
				scsi.mo = NULL;
			}
		}
		if (!scsi.mo) {
			scsi.mo = new SCSIMO(this);
		}
	}
	else {
		// MO�Ȃ�
		if (scsi.mo) {
			delete scsi.mo;
			scsi.mo = NULL;
		}
	}

	// CD-ROM�쐬
	if (cd) {
		// CD-ROM����
		if (scsi.cdrom) {
			if (scsi.cdrom->GetID() != MAKEID('S', 'C', 'C', 'D')) {
				delete scsi.cdrom;
				scsi.cdrom = NULL;
			}
		}
		if (!scsi.cdrom) {
			scsi.cdrom = new SCSICD(this);
		}
	}
	else {
		// CD-ROM�Ȃ�
		if (scsi.cdrom) {
			delete scsi.cdrom;
			scsi.cdrom = NULL;
		}
	}

	// �z�X�g�EMO�D����l�����āA�f�B�X�N����ׂ�
	init = GetSCSIID();
	index = 0;
	for (i=0; i<DeviceMax; i++) {
		// �z�X�g(�C�j�V�G�[�^)�̓X�L�b�v
		if (i == init) {
			continue;
		}

		// MO�D��̏ꍇ�AMO����ׂ�
		if (scsi.mo_first) {
			if (mo) {
				ASSERT(scsi.mo);
				scsi.disk[i] = scsi.mo;
				mo = FALSE;
				continue;
			}
		}

		// HD����ׂ�
		if (index < hd) {
			// NULL�ł��悢
			scsi.disk[i] = scsi.hd[index];
			index++;
			continue;
		}

		// MO����ׂ�
		if (mo) {
			ASSERT(scsi.mo);
			scsi.disk[i] = scsi.mo;
			mo = FALSE;
			continue;
		}
	}

	// CD��ID6�Œ�BID6����t�Ȃ�ID7
	if (cd) {
		ASSERT(scsi.cdrom);
		if (!scsi.disk[6]) {
			scsi.disk[6] = scsi.cdrom;
		}
		else {
			ASSERT(!scsi.disk[7]);
			scsi.disk[7] = scsi.cdrom;
		}
	}

#if defined(SCSI_LOG)
	for (i=0; i<DeviceMax; i++) {
		if (!scsi.disk[i]) {
			LOG1(Log::Detail, "ID=%d : ���ڑ��܂��̓C�j�V�G�[�^", i);
			continue;
		}
		switch (scsi.disk[i]->GetID()) {
			case MAKEID('S', 'C', 'H', 'D'):
				LOG1(Log::Detail, "ID=%d : SCSI �n�[�h�f�B�X�N", i);
				break;
			case MAKEID('S', 'C', 'M', 'O'):
				LOG1(Log::Detail, "ID=%d : SCSI MO�f�B�X�N", i);
				break;
			case MAKEID('S', 'C', 'C', 'D'):
				LOG1(Log::Detail, "ID=%d : SCSI CD-ROM�h���C�u", i);
				break;
			default:
				LOG2(Log::Detail, "ID=%d : ����`(%08X)", i, scsi.disk[i]->GetID());
				break;
		}
	}
#endif	//	SCSI_LOG
}

//---------------------------------------------------------------------------
//
//	MO/CD �I�[�v��
//
//---------------------------------------------------------------------------
BOOL FASTCALL SCSI::Open(const Filepath& path, BOOL mo)
{
	ASSERT(this);
	ASSERT_DIAG();

	// �L���łȂ���΃G���[
	if (!IsValid(mo)) {
		return FALSE;
	}

	// �C�W�F�N�g
	Eject(FALSE, mo);

	// �m�b�g���f�B�łȂ���΃G���[
	if (IsReady(mo)) {
		return FALSE;
	}

	// �h���C�u�ɔC����
	if (mo) {
		ASSERT(scsi.mo);
		if (scsi.mo->Open(path, TRUE)) {
			// ����(MO)
			return TRUE;
		}
	}
	else {
		ASSERT(scsi.cdrom);
		if (scsi.cdrom->Open(path, TRUE)) {
			// ����(CD)
			return TRUE;
		}
	}

	// ���s
	return FALSE;
}

//---------------------------------------------------------------------------
//
//	MO/CD �C�W�F�N�g
//
//---------------------------------------------------------------------------
void FASTCALL SCSI::Eject(BOOL force, BOOL mo)
{
	ASSERT(this);
	ASSERT_DIAG();

	// ���f�B�łȂ���Ή������Ȃ�
	if (!IsReady(mo)) {
		return;
	}

	// �h���C�u�ɒʒm
	if (mo) {
		ASSERT(scsi.mo);
		scsi.mo->Eject(force);
	}
	else {
		ASSERT(scsi.cdrom);
		scsi.cdrom->Eject(force);
	}
}

//---------------------------------------------------------------------------
//
//	MO �������݋֎~
//
//---------------------------------------------------------------------------
void FASTCALL SCSI::WriteP(BOOL flag)
{
	ASSERT(this);
	ASSERT_DIAG();

	// ���f�B�łȂ���Ή������Ȃ�
	if (!IsReady()) {
		return;
	}

	// ���C�g�v���e�N�g�ݒ�����݂�
	ASSERT(scsi.mo);
	scsi.mo->WriteP(flag);
}

//---------------------------------------------------------------------------
//
//	MO �������݋֎~�擾
//
//---------------------------------------------------------------------------
BOOL FASTCALL SCSI::IsWriteP() const
{
	ASSERT(this);
	ASSERT_DIAG();

	// ���f�B�łȂ���΃��C�g�v���e�N�g�łȂ�
	if (!IsReady()) {
		return FALSE;
	}

	// �h���C�u�ɕ���
	ASSERT(scsi.mo);
	return scsi.mo->IsWriteP();
}

//---------------------------------------------------------------------------
//
//	MO ReadOnly�`�F�b�N
//
//---------------------------------------------------------------------------
BOOL FASTCALL SCSI::IsReadOnly() const
{
	ASSERT(this);
	ASSERT_DIAG();

	// ���f�B�łȂ���΃��[�h�I�����[�łȂ�
	if (!IsReady()) {
		return FALSE;
	}

	// �h���C�u�ɕ���
	ASSERT(scsi.mo);
	return scsi.mo->IsReadOnly();
}

//---------------------------------------------------------------------------
//
//	MO/CD Locked�`�F�b�N
//
//---------------------------------------------------------------------------
BOOL FASTCALL SCSI::IsLocked(BOOL mo) const
{
	ASSERT(this);
	ASSERT_DIAG();

	if (mo) {
		// �h���C�u�����݂��邩(MO)
		if (!scsi.mo) {
			return FALSE;
		}

		// �h���C�u�ɕ���(MO)
		return scsi.mo->IsLocked();
	}
	else {
		// �h���C�u�����݂��邩(CD)
		if (!scsi.cdrom) {
			return FALSE;
		}

		// �h���C�u�ɕ���(CD)
		return scsi.cdrom->IsLocked();
	}
}

//---------------------------------------------------------------------------
//
//	MO/CD Ready�`�F�b�N
//
//---------------------------------------------------------------------------
BOOL FASTCALL SCSI::IsReady(BOOL mo) const
{
	ASSERT(this);
	ASSERT_DIAG();

	if (mo) {
		// �h���C�u�����݂��邩(MO)
		if (!scsi.mo) {
			return FALSE;
		}

		// �h���C�u�ɕ���(MO)
		return scsi.mo->IsReady();
	}
	else {
		// �h���C�u�����݂��邩(CD)
		if (!scsi.cdrom) {
			return FALSE;
		}

		// �h���C�u�ɕ���(CD)
		return scsi.cdrom->IsReady();
	}
}

//---------------------------------------------------------------------------
//
//	MO/CD �L���`�F�b�N
//
//---------------------------------------------------------------------------
BOOL FASTCALL SCSI::IsValid(BOOL mo) const
{
	ASSERT(this);
	ASSERT_DIAG();

	if (mo) {
		// �|�C���^�Ŕ���(MO)
		if (scsi.mo) {
			return TRUE;
		}
	}
	else {
		// �|�C���^�Ŕ���(CD)
		if (scsi.cdrom) {
			return TRUE;
		}
	}

	// �h���C�u�Ȃ�
	return FALSE;
}

//---------------------------------------------------------------------------
//
//	MO/CD �p�X�擾
//
//---------------------------------------------------------------------------
void FASTCALL SCSI::GetPath(Filepath& path, BOOL mo) const
{
	ASSERT(this);
	ASSERT_DIAG();

	if (mo) {
		// MO
		if (scsi.mo) {
			// ���f�B��
			if (scsi.mo->IsReady()) {
				// �p�X�擾
				scsi.mo->GetPath(path);
				return;
			}
		}
	}
	else {
		// CD
		if (scsi.cdrom) {
			// ���f�B��
			if (scsi.cdrom->IsReady()) {
				// �p�X�擾
				scsi.cdrom->GetPath(path);
				return;
			}
		}
	}

	// �L���ȃp�X�łȂ��̂ŁA�N���A
	path.Clear();
}

//---------------------------------------------------------------------------
//
//	CD-DA�o�b�t�@�擾
//
//---------------------------------------------------------------------------
void FASTCALL SCSI::GetBuf(DWORD *buffer, int samples, DWORD rate)
{
	ASSERT(this);
	ASSERT(buffer);
	ASSERT(samples >= 0);
	ASSERT(rate != 0);
	ASSERT((rate % 100) == 0);
	ASSERT_DIAG();

	// �C���^�t�F�[�X���L����
	if (scsi.type != 0) {
		// CD-ROM�����݂���ꍇ�Ɍ���
		if (scsi.cdrom) {
			// CD-ROM����v��
			scsi.cdrom->GetBuf(buffer, samples, rate);
		}
	}
}
