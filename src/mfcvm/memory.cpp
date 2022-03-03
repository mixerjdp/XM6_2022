//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 �o�h�D(ytanaka@ipc-tokai.or.jp)
//	[ ������ ]
//
//---------------------------------------------------------------------------

#include "os.h"
#include "xm6.h"
#include "vm.h"
#include "log.h"
#include "filepath.h"
#include "fileio.h"
#include "cpu.h"
#include "areaset.h"
#include "gvram.h"
#include "tvram.h"
#include "sram.h"
#include "config.h"
#include "core_asm.h"
#include "memory.h"

//---------------------------------------------------------------------------
//
//	�X�^�e�B�b�N ���[�N
//
//---------------------------------------------------------------------------
static CPU *pCPU;

//---------------------------------------------------------------------------
//
//	�o�X�G���[�Č���(���C���������������G���A�̂�)
//
//---------------------------------------------------------------------------
extern "C" {

//---------------------------------------------------------------------------
//
//	�ǂݍ��݃o�X�G���[
//
//---------------------------------------------------------------------------
void ReadBusErr(DWORD addr)
{
	pCPU->BusErr(addr, TRUE);
}

//---------------------------------------------------------------------------
//
//	�������݃o�X�G���[
//
//---------------------------------------------------------------------------
void WriteBusErr(DWORD addr)
{
	pCPU->BusErr(addr, FALSE);
}
}

//===========================================================================
//
//	������
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	�R���X�g���N�^
//
//---------------------------------------------------------------------------
Memory::Memory(VM *p) : MemDevice(p)
{
	// �f�o�C�XID��������
	dev.id = MAKEID('M', 'E', 'M', ' ');
	dev.desc = "Memory Ctrl (OHM2)";

	// �J�n�A�h���X�A�I���A�h���X
	memdev.first = 0;
	memdev.last = 0xffffff;

	// RAM/ROM�o�b�t�@
	mem.ram = NULL;
	mem.ipl = NULL;
	mem.cg = NULL;
	mem.scsi = NULL;

	// RAM��2MB
	mem.size = 2;
	mem.config = 0;
	mem.length = 0;

	// �������^�C�v�͖����[�h
	mem.type = None;
	mem.now = None;

	// �I�u�W�F�N�g
	areaset = NULL;
	sram = NULL;

	// ���̑�
	memset(mem.table, 0, sizeof(mem.table));
	mem.memsw = TRUE;

	// static���[�N
	::pCPU = NULL;
}

//---------------------------------------------------------------------------
//
//	������
//
//---------------------------------------------------------------------------
BOOL FASTCALL Memory::Init()
{
	ASSERT(this);

	// ��{�N���X
	if (!MemDevice::Init()) {
		return FALSE;
	}

	// ���C��������
	mem.length = mem.size * 0x100000;
	try {
		mem.ram = new BYTE[ mem.length ];
	}
	catch (...) {
		return FALSE;
	}
	if (!mem.ram) {
		return FALSE;
	}

	// ���C�����������[���N���A����
	memset(mem.ram, 0x00, mem.length);

	// IPL ROM
	try {
		mem.ipl = new BYTE[ 0x20000 ];
	}
	catch (...) {
		return FALSE;
	}
	if (!mem.ipl) {
		return FALSE;
	}

	// CG ROM
	try {
		mem.cg = new BYTE[ 0xc0000 ];
	}
	catch (...) {
		return FALSE;
	}
	if (!mem.cg) {
		return FALSE;
	}

	// SCSI ROM
	try {
		mem.scsi = new BYTE[ 0x20000 ];
	}
	catch (...) {
		return FALSE;
	}
	if (!mem.scsi) {
		return FALSE;
	}

	// SASI��ROM�͕K�{�Ȃ̂ŁA��Ƀ��[�h����
	if (!LoadROM(SASI)) {
		// IPLROM.DAT, CGROM.DAT�����݂��Ȃ��p�^�[��
		return FALSE;
	}

	// ����ROM������΁AXVI��Compact��030�̏��ŁA��Ɍ����������̂�D�悷��
	if (LoadROM(XVI)) {
		mem.now = XVI;
	}
	if (mem.type == None) {
		if (LoadROM(Compact)) {
			mem.now = Compact;
		}
	}
	if (mem.type == None) {
		if (LoadROM(X68030)) {
			mem.now = X68030;
		}
	}

	// XVI,Compact,030����������݂��Ȃ���΁A�ēxSASI��ǂ�
	if (mem.type == None) {
		LoadROM(SASI);
		mem.now = SASI;
	}

	// ���[�W�����G���A��ݒ�
	::s68000context.u_fetch = u_pgr;
	::s68000context.s_fetch = s_pgr;
	::s68000context.u_readbyte = u_rbr;
	::s68000context.s_readbyte = s_rbr;
	::s68000context.u_readword = u_rwr;
	::s68000context.s_readword = s_rwr;
	::s68000context.u_writebyte = u_wbr;
	::s68000context.s_writebyte = s_wbr;
	::s68000context.u_writeword = u_wwr;
	::s68000context.s_writeword = s_wwr;

	// �G���A�Z�b�g�擾
	areaset = (AreaSet*)vm->SearchDevice(MAKEID('A', 'R', 'E', 'A'));
	ASSERT(areaset);

	// SRAM�擾
	sram = (SRAM*)vm->SearchDevice(MAKEID('S', 'R', 'A', 'M'));
	ASSERT(sram);

	// static���[�N
	::pCPU = cpu;

	// �������e�[�u���ݒ�
	InitTable();

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	ROM���[�h
//
//---------------------------------------------------------------------------
BOOL FASTCALL Memory::LoadROM(memtype target)
{
	Filepath path;
	Fileio fio;
	int i;
	BYTE data;
	BYTE *ptr;
	BOOL scsi_req;
	int scsi_size;

	ASSERT(this);

	// ��U���ׂĂ�ROM�G���A���������ANone��
	memset(mem.ipl, 0xff, 0x20000);
	memset(mem.cg, 0xff, 0xc0000);
	memset(mem.scsi, 0xff, 0x20000);
	mem.type = None;

	// IPL
	switch (target) {
		case SASI:
		case SCSIInt:
		case SCSIExt:
			path.SysFile(Filepath::IPL);
			break;
		case XVI:
			path.SysFile(Filepath::IPLXVI);
			break;
		case Compact:
			path.SysFile(Filepath::IPLCompact);
			break;
		case X68030:
			path.SysFile(Filepath::IPL030);
			break;
		default:
			ASSERT(FALSE);
			return FALSE;
	}
	if (!fio.Load(path, mem.ipl, 0x20000)) {
		return FALSE;
	}

	// IPL�o�C�g�X���b�v
	ptr = mem.ipl;
	for (i=0; i<0x10000; i++) {
		data = ptr[0];
		ptr[0] = ptr[1];
		ptr[1] = data;
		ptr += 2;
	}

	// CG
	path.SysFile(Filepath::CG);
	if (!fio.Load(path, mem.cg, 0xc0000)) {
		// �t�@�C�����Ȃ���΁ACGTMP�Ń��g���C
		path.SysFile(Filepath::CGTMP);
		if (!fio.Load(path, mem.cg, 0xc0000)) {
			return FALSE;
		}
	}

	// CG�o�C�g�X���b�v
	ptr = mem.cg;
	for (i=0; i<0x60000; i++) {
		data = ptr[0];
		ptr[0] = ptr[1];
		ptr[1] = data;
		ptr += 2;
	}

	// SCSI
	scsi_req = FALSE;
	switch (target) {
		// ����
		case SCSIInt:
		case XVI:
		case Compact:
			path.SysFile(Filepath::SCSIInt);
			scsi_req = TRUE;
			break;
		case X68030:
			path.SysFile(Filepath::ROM030);
			scsi_req = TRUE;
			break;
		// �O�t
		case SCSIExt:
			path.SysFile(Filepath::SCSIExt);
			scsi_req = TRUE;
			break;
		// SASI(ROM�K�v�Ȃ�)
		case SASI:
			break;
		// ���̑�(���蓾�Ȃ�)
		default:
			ASSERT(FALSE);
			break;
	}
	if (scsi_req) {
		// X68030�̂�ROM30.DAT(0x20000�o�C�g)�A���̑���0x2000�o�C�g�Ńg���C
		if (target == X68030) {
			scsi_size = 0x20000;
		}
		else {
			scsi_size = 0x2000;
		}

		// ��Ƀ|�C���^��ݒ�
		ptr = mem.scsi;

		// ���[�h
		if (!fio.Load(path, mem.scsi, scsi_size)) {
			// SCSIExt��0x1fe0�o�C�g������(WinX68k�����łƌ݊����Ƃ�)
			if (target != SCSIExt) {
				return FALSE;
			}

			// 0x1fe0�o�C�g�ōăg���C
			scsi_size = 0x1fe0;
			ptr = &mem.scsi[0x20];
			if (!fio.Load(path, &mem.scsi[0x0020], scsi_size)) {
				return FALSE;
			}
		}

		// SCSI�o�C�g�X���b�v
		for (i=0; i<scsi_size; i+=2) {
			data = ptr[0];
			ptr[0] = ptr[1];
			ptr[1] = data;
			ptr += 2;
		}
	}

	// �^�[�Q�b�g���J�����g�ɃZ�b�g���āA����
	mem.type = target;
	return TRUE;
}

//---------------------------------------------------------------------------
//
//	�������e�[�u���쐬
//	���������f�R�[�_�Ɉˑ�
//
//---------------------------------------------------------------------------
void FASTCALL Memory::InitTable()
{
#if defined(_WIN32)
#pragma pack(push, 1)
#endif	// _WIN32
	MemDevice* devarray[0x40];
#if defined(_WIN32)
#pragma pack(pop)
#endif	// _WIN32

	MemDevice *mdev;
	BYTE *table;
	DWORD ptr;
	int i;

	ASSERT(this);

	// �|�C���^������
	mdev = this;
	i = 0;

	// Memory�ȍ~�̃f�o�C�X������āA�|�C���^��z��ɗ��Ƃ�
	while (mdev) {
		devarray[i] = mdev;

		// ����
		i++;
		mdev = (MemDevice*)mdev->GetNextDevice();
	}

	// �A�Z���u�����[�`�����Ăяo���A�e�[�u���������n��
	MemInitDecode(this, devarray);

	// �A�Z���u�����[�`���ŏo�����e�[�u�����t�ɖ߂�(�A���C�������g�ɒ���)
	table = MemDecodeTable;
	for (i=0; i<0x180; i++) {
		// 4�o�C�g���Ƃ�DWORD�l����荞�݁A�|�C���^�ɃL���X�g
		ptr = *(DWORD*)table;
		mem.table[i] = (MemDevice*)ptr;

		// ����
		table += 4;
	}
}

//---------------------------------------------------------------------------
//
//	�N���[���A�b�v
//
//---------------------------------------------------------------------------
void FASTCALL Memory::Cleanup()
{
	ASSERT(this);

	// ���������
	if (mem.ram) {
		delete[] mem.ram;
		mem.ram = NULL;
	}
	if (mem.ipl) {
		delete[] mem.ipl;
		mem.ipl = NULL;
	}
	if (mem.cg) {
		delete[] mem.cg;
		mem.cg = NULL;
	}
	if (mem.scsi) {
		delete[] mem.scsi;
		mem.scsi = NULL;
	}

	// ��{�N���X��
	MemDevice::Cleanup();
}

//---------------------------------------------------------------------------
//
//	���Z�b�g
//
//---------------------------------------------------------------------------
void FASTCALL Memory::Reset()
{
	int size;

	ASSERT(this);
	LOG0(Log::Normal, "���Z�b�g");

	// �������^�C�v����v���Ă��邩
	if (mem.type != mem.now) {
		if (LoadROM(mem.type)) {
			// ROM�����݂��Ă���B���[�h�ł���
			mem.now = mem.type;
		}
		else {
			// ROM�����݂��Ȃ��BSASI�^�C�v�Ƃ��āA�ݒ��SASI�ɖ߂�
			LoadROM(SASI);
			mem.now = SASI;
			mem.type = SASI;
		}

		// �R���e�L�X�g����蒼��(CPU::Reset�͊������Ă��邽�߁A�K��FALSE)
		MakeContext(FALSE);
	}

	// �������T�C�Y����v���Ă��邩
	if (mem.size == ((mem.config + 1) * 2)) {
		// ��v���Ă���̂ŁA�������X�C�b�`�����X�V�`�F�b�N
		if (mem.memsw) {
			// $ED0008 : ���C��RAM�T�C�Y
			size = mem.size << 4;
			sram->SetMemSw(0x08, 0x00);
			sram->SetMemSw(0x09, size);
			sram->SetMemSw(0x0a, 0x00);
			sram->SetMemSw(0x0b, 0x00);
		}
		return;
	}

	// �ύX
	mem.size = (mem.config + 1) * 2;

	// �Ċm��
	ASSERT(mem.ram);
	delete[] mem.ram;
	mem.ram = NULL;
	mem.length = mem.size * 0x100000;
	try {
		mem.ram = new BYTE[ mem.length ];
	}
	catch (...) {
		// �������s���̏ꍇ��2MB�ɌŒ�
		mem.config = 0;
		mem.size = 2;
		mem.length = mem.size * 0x100000;
		mem.ram = new BYTE[ mem.length ];
	}
	if (!mem.ram) {
		// �������s���̏ꍇ��2MB�ɌŒ�
		mem.config = 0;
		mem.size = 2;
		mem.length = mem.size * 0x100000;
		mem.ram = new BYTE[ mem.length ];
	}

	// ���������m�ۂł��Ă���ꍇ�̂�
	if (mem.ram) {
		memset(mem.ram, 0x00, mem.length);

		// �R���e�L�X�g����蒼��(CPU::Reset�͊������Ă��邽�߁A�K��FALSE)
		MakeContext(FALSE);
	}

	// �������X�C�b�`�����X�V
	if (mem.memsw) {
		// $ED0008 : ���C��RAM�T�C�Y
		size = mem.size << 4;
		sram->SetMemSw(0x08, 0x00);
		sram->SetMemSw(0x09, size);
		sram->SetMemSw(0x0a, 0x00);
		sram->SetMemSw(0x0b, 0x00);
	}
}

//---------------------------------------------------------------------------
//
//	�Z�[�u
//
//---------------------------------------------------------------------------
BOOL FASTCALL Memory::Save(Fileio *fio, int /*ver*/)
{
	ASSERT(this);
	LOG0(Log::Normal, "�Z�[�u");

	// �^�C�v������
	if (!fio->Write(&mem.now, sizeof(mem.now))) {
		return FALSE;
	}

	// SCSI ROM�̓��e������ (X68030�ȊO)
	if (mem.now != X68030) {
		if (!fio->Write(mem.scsi, 0x2000)) {
			return FALSE;
		}
	}

	// mem.size������
	if (!fio->Write(&mem.size, sizeof(mem.size))) {
		return FALSE;
	}

	// ������������
	if (!fio->Write(mem.ram, mem.length)) {
		return FALSE;
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	���[�h
//
//---------------------------------------------------------------------------
BOOL FASTCALL Memory::Load(Fileio *fio, int /*ver*/)
{
	int size;
	BOOL context;

	ASSERT(this);
	LOG0(Log::Normal, "���[�h");

	// �R���e�L�X�g����蒼���Ȃ�
	context = FALSE;

	// �^�C�v��ǂ�
	if (!fio->Read(&mem.type, sizeof(mem.type))) {
		return FALSE;
	}

	// �^�C�v�����݂̂��̂ƈ���Ă����
	if (mem.type != mem.now) {
		// ROM��ǂݒ���
		if (!LoadROM(mem.type)) {
			// �Z�[�u���ɑ��݂��Ă���ROM���A�Ȃ��Ȃ��Ă���
			LoadROM(mem.now);
			return FALSE;
		}

		// ROM�̓ǂݒ����ɐ�������
		mem.now = mem.type;
		context = TRUE;
	}

	// SCSI ROM�̓��e��ǂ� (X68030�ȊO)
	if (mem.type != X68030) {
		if (!fio->Read(mem.scsi, 0x2000)) {
			return FALSE;
		}
	}

	// mem.size��ǂ�
	if (!fio->Read(&size, sizeof(size))) {
		return FALSE;
	}

	// mem.size�ƈ�v���Ă��Ȃ����
	if (mem.size != size) {
		// �ύX����
		mem.size = size;

		// �Ċm��
		delete[] mem.ram;
		mem.ram = NULL;
		mem.length = mem.size * 0x100000;
		try {
			mem.ram = new BYTE[ mem.length ];
		}
		catch (...) {
			mem.ram = NULL;
		}
		if (!mem.ram) {
			// �������s���̏ꍇ��2MB�ɌŒ�
			mem.config = 0;
			mem.size = 2;
			mem.length = mem.size * 0x100000;
			mem.ram = new BYTE[ mem.length ];

			// ���[�h���s
			return FALSE;
		}

		// �R���e�L�X�g�č쐬���K�v
		context = TRUE;
	}

	// ��������ǂ�
	if (!fio->Read(mem.ram, mem.length)) {
		return FALSE;
	}

	// �K�v�ł���΁A�R���e�L�X�g����蒼��
	if (context) {
		MakeContext(FALSE);
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	�ݒ�K�p
//
//---------------------------------------------------------------------------
void FASTCALL Memory::ApplyCfg(const Config *config)
{
	ASSERT(this);
	ASSERT(config);
	LOG0(Log::Normal, "�ݒ�K�p");

	// ���������(ROM���[�h�͎��񃊃Z�b�g��)
	mem.type = (memtype)config->mem_type;

	// RAM�T�C�Y(�������m�ۂ͎��񃊃Z�b�g��)
	mem.config = config->ram_size;
	ASSERT((mem.config >= 0) && (mem.config <= 5));

	// �������X�C�b�`�����X�V
	mem.memsw = config->ram_sramsync;
}

//---------------------------------------------------------------------------
//
//	�o�C�g�ǂݍ���
//
//---------------------------------------------------------------------------
DWORD FASTCALL Memory::ReadByte(DWORD addr)
{
	DWORD index;

	ASSERT(this);
	ASSERT(addr <= 0xffffff);
	ASSERT(mem.now != None);

	// ���C��RAM
	if (addr < mem.length) {
		return (DWORD)mem.ram[addr ^ 1];
	}

	// IPL
	if (addr >= 0xfe0000) {
		addr &= 0x1ffff;
		addr ^= 1;
		return (DWORD)mem.ipl[addr];
	}

	// IPL�C���[�W or SCSI����
	if (addr >= 0xfc0000) {
		// IPL�C���[�W��
		if ((mem.now == SASI) || (mem.now == SCSIExt)) {
			// IPL�C���[�W
			addr &= 0x1ffff;
			addr ^= 1;
			return (DWORD)mem.ipl[addr];
		}
		// SCSI������(�͈̓`�F�b�N)
		if (addr < 0xfc2000) {
			// SCSI����
			addr &= 0x1fff;
			addr ^= 1;
			return (DWORD)mem.scsi[addr];
		}
		// X68030 IPL�O����
		if (mem.now == X68030) {
			// X68030 IPL�O��
			addr &= 0x1ffff;
			addr ^= 1;
			return (DWORD)mem.scsi[addr];
		}
		// SCSI�������f���ŁAROM�͈͊O
		return 0xff;
	}

	// CG
	if (addr >= 0xf00000) {
		addr &= 0xfffff;
		addr ^= 1;
		return (DWORD)mem.cg[addr];
	}

	// SCSI�O�t
	if (mem.now == SCSIExt) {
		if ((addr >= 0xea0020) && (addr <= 0xea1fff)) {
			addr &= 0x1fff;
			addr ^= 1;
			return (DWORD)mem.scsi[addr];
		}
	}

	// �f�o�C�X�f�B�X�p�b�`
	if (addr >= 0xc00000) {
		index = addr - 0xc00000;
		index >>= 13;
		ASSERT(index < 0x180);
		if (mem.table[index] != (MemDevice*)this) {
			return mem.table[index]->ReadByte(addr);
		}
	}

	LOG1(Log::Warning, "����`�o�C�g�ǂݍ��� $%06X", addr);
	cpu->BusErr(addr, TRUE);
	return 0xff;
}

//---------------------------------------------------------------------------
//
//	���[�h�ǂݍ���
//
//---------------------------------------------------------------------------
DWORD FASTCALL Memory::ReadWord(DWORD addr)
{
	DWORD data;
	DWORD index;
	WORD *ptr;

	ASSERT(this);
	ASSERT(addr <= 0xffffff);
	ASSERT(mem.now != None);

	// CPU����̏ꍇ�͋����ۏ؂���Ă��邪�ADMAC����̏ꍇ�̓`�F�b�N�K�v����
	if (addr & 1) {
		// ��UCPU�֓n��(CPU�o�R��DMA��)
		cpu->AddrErr(addr, TRUE);
		return 0xffff;
	}

	// ���C��RAM
	if (addr < mem.length) {
		ptr = (WORD*)(&mem.ram[addr]);
		data = (DWORD)*ptr;
		return data;
	}

	// IPL
	if (addr >= 0xfe0000) {
		addr &= 0x1ffff;
		ptr = (WORD*)(&mem.ipl[addr]);
		data = (DWORD)*ptr;
		return data;
	}

	// IPL�C���[�W or SCSI����
	if (addr >= 0xfc0000) {
		// IPL�C���[�W��
		if ((mem.now == SASI) || (mem.now == SCSIExt)) {
			// IPL�C���[�W
			addr &= 0x1ffff;
			ptr = (WORD*)(&mem.ipl[addr]);
			data = (DWORD)*ptr;
			return data;
		}
		// SCSI������(�͈̓`�F�b�N)
		if (addr < 0xfc2000) {
			// SCSI����
			addr &= 0x1fff;
			ptr = (WORD*)(&mem.scsi[addr]);
			data = (DWORD)*ptr;
			return data;
		}
		// X68030 IPL�O����
		if (mem.now == X68030) {
			// X68030 IPL�O��
			addr &= 0x1ffff;
			ptr = (WORD*)(&mem.scsi[addr]);
			data = (DWORD)*ptr;
			return data;
		}
		// SCSI�������f���ŁAROM�͈͊O
		return 0xffff;
	}

	// CG
	if (addr >= 0xf00000) {
		addr &= 0xfffff;
		ptr = (WORD*)(&mem.cg[addr]);
		data = (DWORD)*ptr;
		return data;
	}

	// SCSI�O�t
	if (mem.now == SCSIExt) {
		if ((addr >= 0xea0020) && (addr <= 0xea1fff)) {
			addr &= 0x1fff;
			ptr = (WORD*)(&mem.scsi[addr]);
			data = (DWORD)*ptr;
			return data;
		}
	}

	// �f�o�C�X�f�B�X�p�b�`
	if (addr >= 0xc00000) {
		index = addr - 0xc00000;
		index >>= 13;
		ASSERT(index < 0x180);
		if (mem.table[index] != (MemDevice*)this) {
			return mem.table[index]->ReadWord(addr);
		}
	}

	// �o�X�G���[
	LOG1(Log::Warning, "����`���[�h�ǂݍ��� $%06X", addr);
	cpu->BusErr(addr, TRUE);
	return 0xffff;
}

//---------------------------------------------------------------------------
//
//	�o�C�g��������
//
//---------------------------------------------------------------------------
void FASTCALL Memory::WriteByte(DWORD addr, DWORD data)
{
	DWORD index;

	ASSERT(this);
	ASSERT(addr <= 0xffffff);
	ASSERT(data < 0x100);
	ASSERT(mem.now != None);

	// ���C��RAM
	if (addr < mem.length) {
		mem.ram[addr ^ 1] = (BYTE)data;
		return;
	}

	// IPL,SCSI,CG
	if (addr >= 0xf00000) {
		return;
	}

	// �f�o�C�X�f�B�X�p�b�`
	if (addr >= 0xc00000) {
		index = addr - 0xc00000;
		index >>= 13;
		ASSERT(index < 0x180);
		if (mem.table[index] != (MemDevice*)this) {
			mem.table[index]->WriteByte(addr, data);
			return;
		}
	}

	// �o�X�G���[
	cpu->BusErr(addr, FALSE);
	LOG2(Log::Warning, "����`�o�C�g�������� $%06X <- $%02X", addr, data);
}

//---------------------------------------------------------------------------
//
//	���[�h��������
//
//---------------------------------------------------------------------------
void FASTCALL Memory::WriteWord(DWORD addr, DWORD data)
{
	WORD *ptr;
	DWORD index;

	ASSERT(this);
	ASSERT(addr <= 0xffffff);
	ASSERT(data < 0x10000);
	ASSERT(mem.now != None);

	// CPU����̏ꍇ�͋����ۏ؂���Ă��邪�ADMAC����̏ꍇ�̓`�F�b�N�K�v����
	if (addr & 1) {
		// ��UCPU�֓n��(CPU�o�R��DMA��)
		cpu->AddrErr(addr, FALSE);
		return;
	}

	// ���C��RAM
	if (addr < mem.length) {
		ptr = (WORD*)(&mem.ram[addr]);
		*ptr = (WORD)data;
		return;
	}

	// IPL,SCSI,CG
	if (addr >= 0xf00000) {
		return;
	}

	// �f�o�C�X�f�B�X�p�b�`
	if (addr >= 0xc00000) {
		index = addr - 0xc00000;
		index >>= 13;
		ASSERT(index < 0x180);
		if (mem.table[index] != (MemDevice*)this) {
			mem.table[index]->WriteWord(addr, data);
			return;
		}
	}

	// �o�X�G���[
	cpu->BusErr(addr, FALSE);
	LOG2(Log::Warning, "����`���[�h�������� $%06X <- $%04X", addr, data);
}

//---------------------------------------------------------------------------
//
//	�ǂݍ��݂̂�
//
//---------------------------------------------------------------------------
DWORD FASTCALL Memory::ReadOnly(DWORD addr) const
{
	DWORD index;

	ASSERT(this);
	ASSERT(addr <= 0xffffff);
	ASSERT(mem.now != None);

	// ���C��RAM
	if (addr < mem.length) {
		return (DWORD)mem.ram[addr ^ 1];
	}

	// IPL
	if (addr >= 0xfe0000) {
		addr &= 0x1ffff;
		addr ^= 1;
		return (DWORD)mem.ipl[addr];
	}

	// IPL�C���[�W or SCSI����
	if (addr >= 0xfc0000) {
		// IPL�C���[�W��
		if ((mem.now == SASI) || (mem.now == SCSIExt)) {
			// IPL�C���[�W
			addr &= 0x1ffff;
			addr ^= 1;
			return (DWORD)mem.ipl[addr];
		}
		// SCSI������(�͈̓`�F�b�N)
		if (addr < 0xfc2000) {
			// SCSI����
			addr &= 0x1fff;
			addr ^= 1;
			return (DWORD)mem.scsi[addr];
		}
		// X68030 IPL�O����
		if (mem.now == X68030) {
			// X68030 IPL�O��
			addr &= 0x1ffff;
			addr ^= 1;
			return (DWORD)mem.scsi[addr];
		}
		// SCSI�������f���ŁAROM�͈͊O
		return 0xff;
	}

	// CG
	if (addr >= 0xf00000) {
		addr &= 0xfffff;
		addr ^= 1;
		return (DWORD)mem.cg[addr];
	}

	// SCSI�O�t
	if (mem.now == SCSIExt) {
		if ((addr >= 0xea0020) && (addr <= 0xea1fff)) {
			addr &= 0x1fff;
			addr ^= 1;
			return (DWORD)mem.scsi[addr];
		}
	}

	// �f�o�C�X�f�B�X�p�b�`
	if (addr >= 0xc00000) {
		index = addr - 0xc00000;
		index >>= 13;
		ASSERT(index < 0x180);
		if (mem.table[index] != (MemDevice*)this) {
			return mem.table[index]->ReadOnly(addr);
		}
	}

	// �}�b�v����Ă��Ȃ�
	return 0xff;
}

//---------------------------------------------------------------------------
//
//	�R���e�L�X�g�쐬
//
//---------------------------------------------------------------------------
void FASTCALL Memory::MakeContext(BOOL reset)
{
	int index;
	int area;
	GVRAM *gvram;
	TVRAM *tvram;

	ASSERT(this);

	// ���Z�b�g��
	if (reset) {
		// �G���A�Z�b�g�����Z�b�g(CPU::Reset����MakeContext���Ă΂�邽��)
		ASSERT(areaset);
		areaset->Reset();

		// ���Z�b�g��p�R���e�L�X�g($FF00000�`���A$0000000�`�Ɍ�����)
		s_pgr[0].lowaddr = 0;
		s_pgr[0].highaddr = 0xffff;
		s_pgr[0].offset = ((DWORD)mem.ipl) + 0x10000;
		u_pgr[0].lowaddr = 0;
		u_pgr[0].highaddr = 0xffff;
		u_pgr[0].offset = ((DWORD)mem.ipl) + 0x10000;

		// �v���O�����I��
		TerminateProgramRegion(1, s_pgr);
		TerminateProgramRegion(1, u_pgr);

		// �f�[�^�͑S�Ė���
		TerminateDataRegion(0, u_rbr);
		TerminateDataRegion(0, s_rbr);
		TerminateDataRegion(0, u_rwr);
		TerminateDataRegion(0, s_rwr);
		TerminateDataRegion(0, u_wbr);
		TerminateDataRegion(0, s_wbr);
		TerminateDataRegion(0, u_wwr);
		TerminateDataRegion(0, s_wwr);
		return;
	}

	// �ʏ�R���e�L�X�g - �v���O����(User)
	index = 0;
	area = areaset->GetArea();
	u_pgr[index].lowaddr = (area + 1) << 13;
	u_pgr[index].highaddr = mem.length - 1;
	u_pgr[index].offset = (DWORD)mem.ram;
	index++;
	TerminateProgramRegion(index, u_pgr);

	// �ʏ�R���e�L�X�g - �v���O����(Super)
	index = 0;
	s_pgr[index].lowaddr = 0;
	s_pgr[index].highaddr = mem.length - 1;
	s_pgr[index].offset = (DWORD)mem.ram;
	index++;

	// IPL
	s_pgr[index].lowaddr = 0xfe0000;
	s_pgr[index].highaddr = 0xffffff;
	s_pgr[index].offset = ((DWORD)mem.ipl) - 0xfe0000;
	index++;

	// SCSI�O�t
	if (mem.now == SCSIExt) {
		s_pgr[index].lowaddr = 0xea0000;
		s_pgr[index].highaddr = 0xea1fff;
		s_pgr[index].offset = ((DWORD)mem.scsi) - 0xea0000;
		index++;
	}

	// IPL�C���[�W or SCSI����
	if ((mem.now == SASI) || (mem.now == SCSIExt)) {
		// IPL�C���[�W
		s_pgr[index].lowaddr = 0xfc0000;
		s_pgr[index].highaddr = 0xfdffff;
		s_pgr[index].offset = ((DWORD)mem.ipl) - 0xfc0000;
		index++;
	}
	else {
		// SCSI����
		s_pgr[index].lowaddr = 0xfc0000;
		s_pgr[index].highaddr = 0xfc1fff;
		s_pgr[index].offset = ((DWORD)mem.scsi) - 0xfc0000;
		if (mem.now == X68030) {
			// X68030 IPL�O��
			s_pgr[index].lowaddr = 0xfc0000;
			s_pgr[index].highaddr = 0xfdffff;
			s_pgr[index].offset = ((DWORD)mem.scsi) - 0xfc0000;
		}
		index++;
	}

	// �O���t�B�b�NVRAM
	gvram = (GVRAM*)vm->SearchDevice(MAKEID('G', 'V', 'R', 'M'));
	ASSERT(gvram);
	s_pgr[index].lowaddr = 0xc00000;
	s_pgr[index].highaddr = 0xdfffff;
	s_pgr[index].offset = ((DWORD)gvram->GetGVRAM()) - 0xc00000;
	index++;

	// �e�L�X�gVRAM
	tvram = (TVRAM*)vm->SearchDevice(MAKEID('T', 'V', 'R', 'M'));
	ASSERT(tvram);
	s_pgr[index].lowaddr = 0xe00000;
	s_pgr[index].highaddr = 0xe7ffff;
	s_pgr[index].offset = ((DWORD)tvram->GetTVRAM()) - 0xe00000;
	index++;

	// SRAM
	ASSERT(sram);
	s_pgr[index].lowaddr = 0xed0000;
	s_pgr[index].highaddr = 0xed0000 + (sram->GetSize() << 10) - 1;
	s_pgr[index].offset = ((DWORD)sram->GetSRAM()) - 0xed0000;
	index++;
	TerminateProgramRegion(index, s_pgr);

	// �ʏ�R���e�L�X�g - �ǂݏo��(User)
	index = 0;
	area = areaset->GetArea();

	// ���[�U�A�N�Z�X�\���
	u_rbr[index].lowaddr = (area + 1) << 13;
	u_rbr[index].highaddr = mem.length - 1;
	u_rbr[index].memorycall = NULL;
	u_rbr[index].userdata = (void*)&mem.ram[(area + 1) << 13];
	index++;

	// �X�[�p�o�C�U���
	u_rbr[index].lowaddr = 0;
	u_rbr[index].highaddr = ((area + 1) << 13) - 1;
	u_rbr[index].memorycall = ::ReadErrC;
	u_rbr[index].userdata = NULL;
	index++;

	// ���C����������������ԁ{�X�[�p�[�o�C�UI/O���
	u_rbr[index].lowaddr = (mem.size << 20);
	u_rbr[index].highaddr = 0xebffff;
	u_rbr[index].memorycall = ::ReadErrC;
	s_rbr[index].userdata = NULL;
	index++;

	// ���[�UI/O���($EC0000-$ECFFFF)
	u_rbr[index].lowaddr = 0xec0000;
	u_rbr[index].highaddr = 0xecffff;
	u_rbr[index].memorycall = ::ReadByteC;
	s_rbr[index].userdata = NULL;
	index++;

	// �X�[�p�o�C�U���(SRAM,CG,IPL,SCSI)
	u_rbr[index].lowaddr = 0xed0000;
	u_rbr[index].highaddr = 0xffffff;
	u_rbr[index].memorycall = ReadErrC;
	s_rbr[index].userdata = NULL;
	index++;
	TerminateDataRegion(index, u_rbr);

	// ���ֈڂ�(ReadWord, User)
	memcpy(u_rwr, u_rbr, sizeof(u_rbr));
	u_rwr[index - 2].memorycall = ::ReadWordC;

	// ���ֈڂ�(WriteByte, User)
	memcpy(u_wbr, u_rbr, sizeof(u_rbr));
	u_wbr[index - 2].memorycall = ::WriteByteC;
	u_wbr[index - 1].memorycall = ::WriteErrC;
	u_wbr[index - 3].memorycall = ::WriteErrC;
	u_wbr[index - 4].memorycall = ::WriteErrC;

	// ���ֈڂ�(WriteWord, User)
	memcpy(u_wwr, u_wbr, sizeof(u_wbr));
	u_wwr[index - 2].memorycall = ::WriteWordC;

	// �ʏ�R���e�L�X�g - �ǂݏo��(Super)
	index = 0;
	s_rbr[index].lowaddr = 0;
	s_rbr[index].highaddr = mem.length - 1;
	s_rbr[index].memorycall = NULL;
	s_rbr[index].userdata = (void*)mem.ram;
	index++;

	// CG
	s_rbr[index].lowaddr = 0xf00000;
	s_rbr[index].highaddr = 0xfbffff;
	s_rbr[index].memorycall = NULL;
	s_rbr[index].userdata = (void*)mem.cg;
	index++;

	// IPL
	s_rbr[index].lowaddr = 0xfe0000;
	s_rbr[index].highaddr = 0xffffff;
	s_rbr[index].memorycall = NULL;
	s_rbr[index].userdata = (void*)mem.ipl;
	index++;

	// SCSI�O�t
	if (mem.now == SCSIExt) {
		s_rbr[index].lowaddr = 0xea0020;
		s_rbr[index].highaddr = 0xea1fff;
		s_rbr[index].memorycall = NULL;
		s_rbr[index].userdata = (void*)(&mem.scsi[0x20]);
		index++;
	}

	// IPL�C���[�W or SCSI����
	if ((mem.now == SASI) || (mem.now == SCSIExt)) {
		// IPL�C���[�W
		s_rbr[index].lowaddr = 0xfc0000;
		s_rbr[index].highaddr = 0xfdffff;
		s_rbr[index].memorycall = NULL;
		s_rbr[index].userdata = (void*)mem.ipl;
		index++;
	}
	else {
		// SCSI����
		s_rbr[index].lowaddr = 0xfc0000;
		s_rbr[index].highaddr = 0xfc1fff;
		s_rbr[index].memorycall = NULL;
		s_rbr[index].userdata = (void*)mem.scsi;
		if (mem.now == X68030) {
			// X68030 IPL�O��
			s_rbr[index].lowaddr = 0xfc0000;
			s_rbr[index].highaddr = 0xfdffff;
			s_rbr[index].memorycall = NULL;
			s_rbr[index].userdata = (void*)mem.scsi;
		}
		index++;
	}

	// ����ȊO(�O���R�[��)
	s_rbr[index].lowaddr = (mem.size << 20);
	s_rbr[index].highaddr = 0xefffff;
	s_rbr[index].memorycall = ::ReadByteC;
	s_rbr[index].userdata = NULL;
	index++;
	TerminateDataRegion(index, s_rbr);

	// ���ֈڂ�
	memcpy(s_rwr, s_rbr, sizeof(s_rbr));
	s_rwr[index - 1].memorycall = ::ReadWordC;

	// �ʏ�R���e�L�X�g - ��������(Super)
	index = 0;
	s_wbr[index].lowaddr = 0;
	s_wbr[index].highaddr = mem.length - 1;
	s_wbr[index].memorycall = NULL;
	s_wbr[index].userdata = (void*)mem.ram;
	index++;

	// ����ȊO(�O���R�[��)
	s_wbr[index].lowaddr = (mem.size << 20);
	s_wbr[index].highaddr = 0xefffff;
	s_wbr[index].memorycall = ::WriteByteC;
	s_wbr[index].userdata = NULL;
	index++;
	TerminateDataRegion(index, s_wbr);

	// ���ֈڂ�
	memcpy(s_wwr, s_wbr, sizeof(s_wbr));
	s_wwr[index - 1].memorycall = ::WriteWordC;

	// cpu->Release��Y�ꂸ��
	cpu->Release();
}

//---------------------------------------------------------------------------
//
//	�v���O�������[�W�����I��
//
//---------------------------------------------------------------------------
void FASTCALL Memory::TerminateProgramRegion(int index, STARSCREAM_PROGRAMREGION *spr)
{
	ASSERT(this);
	ASSERT((index >= 0) && (index < 10));
	ASSERT(spr);

	spr[index].lowaddr = (DWORD)-1;
	spr[index].highaddr = (DWORD)-1;
	spr[index].offset = 0;
}

//---------------------------------------------------------------------------
//
//	�f�[�^���[�W�����I��
//
//---------------------------------------------------------------------------
void FASTCALL Memory::TerminateDataRegion(int index, STARSCREAM_DATAREGION *sdr)
{
	ASSERT(this);
	ASSERT((index >= 0) && (index < 10));
	ASSERT(sdr);

	sdr[index].lowaddr = (DWORD)-1;
	sdr[index].highaddr = (DWORD)-1;
	sdr[index].memorycall = NULL;
	sdr[index].userdata = NULL;
}

//---------------------------------------------------------------------------
//
//	IPL�o�[�W�����`�F�b�N
//	��IPL��version1.00(87/05/07)�ł��邩�ۂ����`�F�b�N
//
//---------------------------------------------------------------------------
BOOL FASTCALL Memory::CheckIPL() const
{
	ASSERT(this);
	ASSERT(mem.now != None);

	// ���݃`�F�b�N
	if (!mem.ipl) {
		return FALSE;
	}

	// SASI�^�C�v�̏ꍇ�̂݃`�F�b�N����
	if (mem.now != SASI) {
		return TRUE;
	}

	// ���t(BCD)���`�F�b�N
	if (mem.ipl[0x1000a] != 0x87) {
		return FALSE;
	}
	if (mem.ipl[0x1000c] != 0x07) {
		return FALSE;
	}
	if (mem.ipl[0x1000d] != 0x05) {
		return FALSE;
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	CG�`�F�b�N
//	��8x8�h�b�g�t�H���g(�S�@�틤��)��Sum,Xor�Ń`�F�b�N
//
//---------------------------------------------------------------------------
BOOL FASTCALL Memory::CheckCG() const
{
	BYTE add;
	BYTE eor;
	BYTE *ptr;
	int i;

	ASSERT(this);
	ASSERT(mem.now != None);

	// ���݃`�F�b�N
	if (!mem.cg) {
		return FALSE;
	}

	// �����ݒ�
	add = 0;
	eor = 0;
	ptr = &mem.cg[0x3a800];

	// ADD, XOR���[�v
	for (i=0; i<0x1000; i++) {
		add = (BYTE)(add + *ptr);
		eor ^= *ptr;
		ptr++;
	}

	// �`�F�b�N(XVI�ł̎����l)
	if ((add != 0xec) || (eor != 0x84)) {
		return FALSE;
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	CG�擾
//
//---------------------------------------------------------------------------
const BYTE* FASTCALL Memory::GetCG() const
{
	ASSERT(this);
	ASSERT(mem.cg);

	return mem.cg;
}

//---------------------------------------------------------------------------
//
//	SCSI�擾
//
//---------------------------------------------------------------------------
const BYTE* FASTCALL Memory::GetSCSI() const
{
	ASSERT(this);
	ASSERT(mem.scsi);

	return mem.scsi;
}
