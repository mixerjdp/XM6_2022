//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 �o�h�D(ytanaka@ipc-tokai.or.jp)
//	[ PPI(i8255A) ]
//
//---------------------------------------------------------------------------

#include "os.h"
#include "xm6.h"
#include "vm.h"
#include "log.h"
#include "adpcm.h"
#include "schedule.h"
#include "config.h"
#include "fileio.h"
#include "ppi.h"

//===========================================================================
//
//	PPI
//
//===========================================================================
//#define PPI_LOG

//---------------------------------------------------------------------------
//
//	�R���X�g���N�^
//
//---------------------------------------------------------------------------
PPI::PPI(VM *p) : MemDevice(p)
{
	int i;

	// �f�o�C�XID��������
	dev.id = MAKEID('P', 'P', 'I', ' ');
	dev.desc = "PPI (i8255A)";

	// �J�n�A�h���X�A�I���A�h���X
	memdev.first = 0xe9a000;
	memdev.last = 0xe9bfff;

	// �������[�N
	memset(&ppi, 0, sizeof(ppi));

	// �I�u�W�F�N�g
	adpcm = NULL;
	for (i=0; i<PortMax; i++) {
		joy[i] = NULL;
	}
}

//---------------------------------------------------------------------------
//
//	������
//
//---------------------------------------------------------------------------
BOOL FASTCALL PPI::Init()
{
	int i;

	ASSERT(this);

	// ��{�N���X
	if (!MemDevice::Init()) {
		return FALSE;
	}

	// ADPCM�擾
	ASSERT(!adpcm);
	adpcm = (ADPCM*)vm->SearchDevice(MAKEID('A', 'P', 'C', 'M'));
	ASSERT(adpcm);

	// �W���C�X�e�B�b�N�^�C�v
	for (i=0; i<PortMax; i++) {
		ppi.type[i] = 0;
		ASSERT(!joy[i]);
		joy[i] = new JoyDevice(this, i);
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	�N���[���A�b�v
//
//---------------------------------------------------------------------------
void FASTCALL PPI::Cleanup()
{
	int i;

	ASSERT(this);

	// �W���C�X�e�B�b�N�f�o�C�X�����
	for (i=0; i<PortMax; i++) {
		ASSERT(joy[i]);
		delete joy[i];
		joy[i] = NULL;
	}

	// ��{�N���X��
	MemDevice::Cleanup();
}

//---------------------------------------------------------------------------
//
//	���Z�b�g
//
//---------------------------------------------------------------------------
void FASTCALL PPI::Reset()
{
	int i;

	ASSERT(this);
	ASSERT_DIAG();

	LOG0(Log::Normal, "���Z�b�g");

	// �|�[�gC
	ppi.portc = 0;

	// �R���g���[��
	for (i=0; i<PortMax; i++) {
		ppi.ctl[i] = 0;
	}

	// �W���C�X�e�B�b�N�f�o�C�X�ɑ΂��āA���Z�b�g��ʒm
	for (i=0; i<PortMax; i++) {
		joy[i]->Reset();
	}
}

//---------------------------------------------------------------------------
//
//	�Z�[�u
//
//---------------------------------------------------------------------------
BOOL FASTCALL PPI::Save(Fileio *fio, int ver)
{
	size_t sz;
	int i;
	DWORD type;

	ASSERT(this);
	ASSERT(fio);
	ASSERT(ver >= 0x0200);
	ASSERT_DIAG();

	LOG0(Log::Normal, "�Z�[�u");

	// �T�C�Y���Z�[�u
	sz = sizeof(ppi_t);
	if (!fio->Write(&sz, sizeof(sz))) {
		return FALSE;
	}

	// ���̂��Z�[�u
	if (!fio->Write(&ppi, (int)sz)) {
		return FALSE;
	}

	// �f�o�C�X���Z�[�u
	for (i=0; i<PortMax; i++) {
		// �f�o�C�X�^�C�v
		type = joy[i]->GetType();
		if (!fio->Write(&type, sizeof(type))) {
			return FALSE;
		}

		// �f�o�C�X����
		if (!joy[i]->Save(fio, ver)) {
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
BOOL FASTCALL PPI::Load(Fileio *fio, int ver)
{
	size_t sz;
	int i;
	DWORD type;

	ASSERT(this);
	ASSERT(fio);
	ASSERT(ver >= 0x0200);
	ASSERT_DIAG();

	LOG0(Log::Normal, "���[�h");

	// �T�C�Y�����[�h�A�ƍ�
	if (!fio->Read(&sz, sizeof(sz))) {
		return FALSE;
	}
	if (sz != sizeof(ppi_t)) {
		return FALSE;
	}

	// ���̂����[�h
	if (!fio->Read(&ppi, (int)sz)) {
		return FALSE;
	}

	// version2.00�ł���΂����܂�
	if (ver <= 0x200) {
		return TRUE;
	}

	// �f�o�C�X�����[�h
	for (i=0; i<PortMax; i++) {
		// �f�o�C�X�^�C�v�𓾂�
		if (!fio->Read(&type, sizeof(type))) {
			return FALSE;
		}

		// ���݂̃f�o�C�X�ƈ�v���Ă��Ȃ���΁A��蒼��
		if (joy[i]->GetType() != type) {
			delete joy[i];
			joy[i] = NULL;

			// PPI�ɋL�����Ă���^�C�v�����킹�ĕύX
			ppi.type[i] = (int)type;

			// �č쐬
			joy[i] = CreateJoy(i, ppi.type[i]);
			ASSERT(joy[i]->GetType() == type);
		}

		// �f�o�C�X�ŗL
		if (!joy[i]->Load(fio, ver)) {
			return FALSE;
		}
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	�ݒ�K�p
//
//---------------------------------------------------------------------------
void FASTCALL PPI::ApplyCfg(const Config *config)
{
	int i;

	ASSERT(this);
	ASSERT(config);
	ASSERT_DIAG();

	LOG0(Log::Normal, "�ݒ�K�p");

	// �|�[�g���[�v
	for (i=0; i<PortMax; i++) {
		// �^�C�v��v�Ȃ玟��
		if (config->joy_type[i] == ppi.type[i]) {
			continue;
		}

		// ���݂̃f�o�C�X���폜
		ASSERT(joy[i]);
		delete joy[i];
		joy[i] = NULL;

		// �^�C�v�L���A�W���C�X�e�B�b�N�쐬
		ppi.type[i] = config->joy_type[i];
		joy[i] = CreateJoy(i, config->joy_type[i]);
	}
}

#if defined(_DEBUG)
//---------------------------------------------------------------------------
//
//	�f�f
//
//---------------------------------------------------------------------------
void FASTCALL PPI::AssertDiag() const
{
	ASSERT(this);
	ASSERT(GetID() == MAKEID('P', 'P', 'I', ' '));
	ASSERT(adpcm);
	ASSERT(adpcm->GetID() == MAKEID('A', 'P', 'C', 'M'));
	ASSERT(joy[0]);
	ASSERT(joy[1]);
}
#endif	// _DEBUG

//---------------------------------------------------------------------------
//
//	�o�C�g�ǂݍ���
//
//---------------------------------------------------------------------------
DWORD FASTCALL PPI::ReadByte(DWORD addr)
{
	DWORD data;

	ASSERT(this);
	ASSERT((addr >= memdev.first) && (addr <= memdev.last));
	ASSERT(PortMax >= 2);
	ASSERT_DIAG();

	// ��A�h���X�̂݃f�R�[�h����Ă���
	if ((addr & 1) == 0) {
		return 0xff;
	}

	// 8�o�C�g�P�ʂŃ��[�v
	addr &= 7;

	// �E�F�C�g
	scheduler->Wait(1);

	// �f�R�[�h
	addr >>= 1;
	switch (addr) {
		// �|�[�gA
		case 0:
#if defined(PPI_LOG)
			data = joy[0]->ReadPort(ppi.ctl[0]);
			LOG2(Log::Normal, "�|�[�g1�ǂݏo�� �R���g���[��$%02X �f�[�^$%02X",
								ppi.ctl[0], data);
#else
			data = joy[0]->ReadPort(ppi.ctl[0]);
#endif	// PPI_LOG

			// PC7,PC6���l��
			if (ppi.ctl[0] & 0x80) {
				data &= ~0x40;
			}
			if (ppi.ctl[0] & 0x40) {
				data &= ~0x20;
			}
			return data;

		// �|�[�gB
		case 1:
#if defined(PPI_LOG)
			data = joy[1]->ReadPort(ppi.ctl[1]);
			LOG2(Log::Normal, "�|�[�g2�ǂݏo�� �R���g���[��$%02X �f�[�^$%02X",
								ppi.ctl[1], data);
			return data;
#else
			return joy[1]->ReadPort(ppi.ctl[1]);
#endif	// PPI_LOG

		// �|�[�gC
		case 2:
			return ppi.portc;
	}

	LOG1(Log::Warning, "���������W�X�^�ǂݍ��� R%02d", addr);
	return 0xff;
}

//---------------------------------------------------------------------------
//
//	���[�h�ǂݍ���
//
//---------------------------------------------------------------------------
DWORD FASTCALL PPI::ReadWord(DWORD addr)
{
	ASSERT(this);
	ASSERT((addr >= memdev.first) && (addr <= memdev.last));
	ASSERT((addr & 1) == 0);
	ASSERT_DIAG();

	return (0xff00 | ReadByte(addr + 1));
}

//---------------------------------------------------------------------------
//
//	�o�C�g��������
//
//---------------------------------------------------------------------------
void FASTCALL PPI::WriteByte(DWORD addr, DWORD data)
{
	DWORD bit;
	int i;

	ASSERT(this);
	ASSERT((addr >= memdev.first) && (addr <= memdev.last));
	ASSERT(data < 0x100);
	ASSERT_DIAG();

	// ��A�h���X�̂݃f�R�[�h����Ă���
	if ((addr & 1) == 0) {
		return;
	}

	// 8�o�C�g�P�ʂŃ��[�v
	addr &= 7;

	// �E�F�C�g
	scheduler->Wait(1);

	// �|�[�gC�ւ�Write
	if (addr == 5) {
		// �W���C�X�e�B�b�N�EADPCM�R���g���[��
		SetPortC(data);
		return;
	}

	// ���[�h�R���g���[��
	if (addr == 7) {
		if (data < 0x80) {
			// �r�b�g�Z�b�g�E���Z�b�g���[�h
			i = ((data >> 1) & 0x07);
			bit = (DWORD)(1 << i);
			if (data & 1) {
				SetPortC(DWORD(ppi.portc | bit));
			}
			else {
				SetPortC(DWORD(ppi.portc & ~bit));
			}
			return;
		}

		// ���[�h�R���g���[��
		if (data != 0x92) {
			LOG0(Log::Warning, "�T�|�[�g�O���[�h�w�� $%02X");
		}
		return;
	}

	LOG2(Log::Warning, "���������W�X�^�������� R%02d <- $%02X",
							addr, data);
}

//---------------------------------------------------------------------------
//
//	���[�h��������
//
//---------------------------------------------------------------------------
void FASTCALL PPI::WriteWord(DWORD addr, DWORD data)
{
	ASSERT(this);
	ASSERT((addr >= memdev.first) && (addr <= memdev.last));
	ASSERT((addr & 1) == 0);
	ASSERT(data < 0x10000);
	ASSERT_DIAG();

	WriteByte(addr + 1, (BYTE)data);
}

//---------------------------------------------------------------------------
//
//	�ǂݍ��݂̂�
//
//---------------------------------------------------------------------------
DWORD FASTCALL PPI::ReadOnly(DWORD addr) const
{
	DWORD data;

	ASSERT(this);
	ASSERT((addr >= memdev.first) && (addr <= memdev.last));
	ASSERT(PortMax >= 2);
	ASSERT_DIAG();

	// ��A�h���X�̂݃f�R�[�h����Ă���
	if ((addr & 1) == 0) {
		return 0xff;
	}

	// 8�o�C�g�P�ʂŃ��[�v
	addr &= 7;

	// �f�R�[�h
	addr >>= 1;
	switch (addr) {
		// �|�[�gA
		case 0:
			// �f�[�^�擾
			data = joy[0]->ReadOnly(ppi.ctl[0]);

			// PC7,PC6���l��
			if (ppi.ctl[0] & 0x80) {
				data &= ~0x40;
			}
			if (ppi.ctl[0] & 0x40) {
				data &= ~0x20;
			}
			return data;

		// �|�[�gB
		case 1:
			return joy[1]->ReadOnly(ppi.ctl[1]);

		// �|�[�gC
		case 2:
			return ppi.portc;
	}

	return 0xff;
}

//---------------------------------------------------------------------------
//
//	�|�[�gC�Z�b�g
//
//---------------------------------------------------------------------------
void FASTCALL PPI::SetPortC(DWORD data)
{
	ASSERT(this);
	ASSERT(data < 0x100);
	ASSERT(PortMax >= 2);
	ASSERT_DIAG();

	// �f�[�^���L��
	ppi.portc = data;

	// �R���g���[���f�[�^�g�ݗ���(�|�[�gA)...bit0��PC4�Abit6,7��PC6,7������
	ppi.ctl[0] = ppi.portc & 0xc0;
	if (ppi.portc & 0x10) {
		ppi.ctl[0] |= 0x01;
	}
#if defined(PPI_LOG)
	LOG1(Log::Normal, "�|�[�g1 �R���g���[�� $%02X", ppi.ctl[0]);
#endif	// PPI_LOG
	joy[0]->Control(ppi.ctl[0]);

	// �R���g���[���f�[�^�g�ݗ���(�|�[�gB)...bit0��PC5������
	if (ppi.portc & 0x20) {
		ppi.ctl[1] = 0x01;
	}
	else {
		ppi.ctl[1] = 0x00;
	}
#if defined(PPI_LOG)
	LOG1(Log::Normal, "�|�[�g2 �R���g���[�� $%02X", ppi.ctl[1]);
#endif	// PPI_LOG
	joy[1]->Control(ppi.ctl[1]);

	// ADPCM�p���|�b�g
	adpcm->SetPanpot(data & 3);

	// ADPCM���x�䗦
	adpcm->SetRatio((data >> 2) & 3);
}

//---------------------------------------------------------------------------
//
//	�����f�[�^�擾
//
//---------------------------------------------------------------------------
void FASTCALL PPI::GetPPI(ppi_t *buffer)
{
	ASSERT(this);
	ASSERT(buffer);
	ASSERT_DIAG();

	// �������[�N���R�s�[
	*buffer = ppi;
}

//---------------------------------------------------------------------------
//
//	�W���C�X�e�B�b�N���ݒ�
//
//---------------------------------------------------------------------------
void FASTCALL PPI::SetJoyInfo(int port, const joyinfo_t *info)
{
	ASSERT(this);
	ASSERT((port >= 0) && (port < PortMax));
	ASSERT(info);
	ASSERT(PortMax >= 2);
	ASSERT_DIAG();

	// ��r���āA��v���Ă���Ή������Ȃ�
	if (memcmp(&ppi.info[port], info, sizeof(joyinfo_t)) == 0) {
		return;
	}

	// �ۑ�
	memcpy(&ppi.info[port], info, sizeof(joyinfo_t));

	// ���̃|�[�g�ɑΉ�����W���C�X�e�B�b�N�f�o�C�X�ցA�ʒm
	ASSERT(joy[port]);
	joy[port]->Notify();
}

//---------------------------------------------------------------------------
//
//	�W���C�X�e�B�b�N���擾
//
//---------------------------------------------------------------------------
const PPI::joyinfo_t* FASTCALL PPI::GetJoyInfo(int port) const
{
	ASSERT(this);
	ASSERT((port >= 0) && (port < PortMax));
	ASSERT(PortMax >= 2);
	ASSERT_DIAG();

	return &(ppi.info[port]);
}

//---------------------------------------------------------------------------
//
//	�W���C�X�e�B�b�N�f�o�C�X�쐬
//
//---------------------------------------------------------------------------
JoyDevice* FASTCALL PPI::CreateJoy(int port, int type)
{
	ASSERT(this);
	ASSERT(type >= 0);
	ASSERT((port >= 0) && (port < PortMax));
	ASSERT(PortMax >= 2);

	// �^�C�v��
	switch (type) {
		// �ڑ��Ȃ�
		case 0:
			return new JoyDevice(this, port);

		// ATARI�W��
		case 1:
			return new JoyAtari(this, port);

		// ATARI�W��+START/SELCT
		case 2:
			return new JoyASS(this, port);

		// �T�C�o�[�X�e�B�b�N(�A�i���O)
		case 3:
			return new JoyCyberA(this, port);

		// �T�C�o�[�X�e�B�b�N(�f�W�^��)
		case 4:
			return new JoyCyberD(this, port);

		// MD3�{�^��
		case 5:
			return new JoyMd3(this, port);

		// MD6�{�^��
		case 6:
			return new JoyMd6(this, port);

		// CPSF-SFC
		case 7:
			return new JoyCpsf(this, port);

		// CPSF-MD
		case 8:
			return new JoyCpsfMd(this, port);

		// �}�W�J���p�b�h
		case 9:
			return new JoyMagical(this, port);

		// XPD-1LR
		case 10:
			return new JoyLR(this, port);

		// �p�b�N�����h��p�p�b�h
		case 11:
			return new JoyPacl(this, port);

		// BM68��p�R���g���[��
		case 12:
			return new JoyBM(this, port);

		// ���̑�
		default:
			ASSERT(FALSE);
			break;
	}

	// �ʏ�A�����ɂ͂��Ȃ�
	return new JoyDevice(this, port);
}

//===========================================================================
//
//	�W���C�X�e�B�b�N�f�o�C�X
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	�R���X�g���N�^
//
//---------------------------------------------------------------------------
JoyDevice::JoyDevice(PPI *parent, int no)
{
	ASSERT((no >= 0) || (no < PPI::PortMax));

	// �^�C�vNULL
	id = MAKEID('N', 'U', 'L', 'L');
	type = 0;

	// �e�f�o�C�X(PPI)���L���A�|�[�g�ԍ��ݒ�
	ppi = parent;
	port = no;

	// ���E�{�^���Ȃ��A�f�W�^���A�f�[�^��0
	axes = 0;
	buttons = 0;
	analog = FALSE;
	datas = 0;

	// �\��
	axis_desc = NULL;
	button_desc = NULL;

	// �f�[�^�o�b�t�@��NULL
	data = NULL;

	// �X�V�`�F�b�N�v
	changed = TRUE;
}

//---------------------------------------------------------------------------
//
//	�f�X�g���N�^
//
//---------------------------------------------------------------------------
JoyDevice::~JoyDevice()
{
	// �f�[�^�o�b�t�@������Ή��
	if (data) {
		delete[] data;
		data = NULL;
	}
}

//---------------------------------------------------------------------------
//
//	���Z�b�g
//
//---------------------------------------------------------------------------
void FASTCALL JoyDevice::Reset()
{
	ASSERT(this);
}

//---------------------------------------------------------------------------
//
//	�Z�[�u
//
//---------------------------------------------------------------------------
BOOL FASTCALL JoyDevice::Save(Fileio *fio, int /*ver*/)
{
	ASSERT(this);
	ASSERT(fio);

	// �f�[�^����0�Ȃ�Z�[�u���Ȃ�
	if (datas <= 0) {
		ASSERT(datas == 0);
		return TRUE;
	}

	// �f�[�^�������ۑ�
	if (!fio->Write(data, sizeof(DWORD) * datas)) {
		return FALSE;
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	���[�h
//
//---------------------------------------------------------------------------
BOOL FASTCALL JoyDevice::Load(Fileio *fio, int /*ver*/)
{
	ASSERT(this);
	ASSERT(fio);

	// �f�[�^����0�Ȃ烍�[�h���Ȃ�
	if (datas <= 0) {
		ASSERT(datas == 0);
		return TRUE;
	}

	// �X�V����
	changed = TRUE;

	// �f�[�^���̂����[�h
	if (!fio->Read(data, sizeof(DWORD) * datas)) {
		return FALSE;
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	�|�[�g�ǂݎ��
//
//---------------------------------------------------------------------------
DWORD FASTCALL JoyDevice::ReadPort(DWORD ctl)
{
	ASSERT(this);
	ASSERT((port >= 0) && (port < PPI::PortMax));
	ASSERT(ppi);
	ASSERT(ctl < 0x100);

	// �ύX�t���O���`�F�b�N
	if (changed) {
		// �t���O���Ƃ�
		changed = FALSE;

		// �f�[�^�쐬
		MakeData();
	}

	// ReadOnly�Ɠ����f�[�^��Ԃ�
	return ReadOnly(ctl);
}

//---------------------------------------------------------------------------
//
//	�|�[�g�ǂݎ��(Read Only)
//
//---------------------------------------------------------------------------
DWORD FASTCALL JoyDevice::ReadOnly(DWORD /*ctl*/) const
{
	ASSERT(this);

	// ���ڑ�
	return 0xff;
}

//---------------------------------------------------------------------------
//
//	�R���g���[��
//
//---------------------------------------------------------------------------
void FASTCALL JoyDevice::Control(DWORD /*ctl*/)
{
	ASSERT(this);
}

//---------------------------------------------------------------------------
//
//	�f�[�^�쐬
//
//---------------------------------------------------------------------------
void FASTCALL JoyDevice::MakeData()
{
	ASSERT(this);
}

//---------------------------------------------------------------------------
//
//	���\��
//
//---------------------------------------------------------------------------
const char* FASTCALL JoyDevice::GetAxisDesc(int axis) const
{
	ASSERT(this);
	ASSERT(axis >= 0);

	// �����𒴂��Ă����NULL
	if (axis >= axes) {
		return NULL;
	}

	// ���\���e�[�u�����Ȃ����NULL
	if (!axis_desc) {
		return NULL;
	}

	// ���\���e�[�u������Ԃ�
	return axis_desc[axis];
}

//---------------------------------------------------------------------------
//
//	�{�^���\��
//
//---------------------------------------------------------------------------
const char* FASTCALL JoyDevice::GetButtonDesc(int button) const
{
	ASSERT(this);
	ASSERT(button >= 0);

	// �{�^�����𒴂��Ă����NULL
	if (button >= buttons) {
		return NULL;
	}

	// �{�^���\���e�[�u�����Ȃ����NULL
	if (!button_desc) {
		return NULL;
	}

	// �{�^���\���e�[�u������Ԃ�
	return button_desc[button];
}

//===========================================================================
//
//	�W���C�X�e�B�b�N(ATARI�W��)
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	�R���X�g���N�^
//
//---------------------------------------------------------------------------
JoyAtari::JoyAtari(PPI *parent, int no) : JoyDevice(parent, no)
{
	// �^�C�vATAR
	id = MAKEID('A', 'T', 'A', 'R');
	type = 1;

	// 2��2�{�^���A�f�[�^��1
	axes = 2;
	buttons = 2;
	datas = 1;

	// �\���e�[�u��
	axis_desc = AxisDescTable;
	button_desc = ButtonDescTable;

	// �f�[�^�o�b�t�@�m��
	data = new DWORD[datas];

	// �����f�[�^�ݒ�
	data[0] = 0xff;
}

//---------------------------------------------------------------------------
//
//	�|�[�g�ǂݎ��(Read Only)
//
//---------------------------------------------------------------------------
DWORD FASTCALL JoyAtari::ReadOnly(DWORD ctl) const
{
	ASSERT(this);
	ASSERT(ctl < 0x100);

	// PC4��1�Ȃ�A0xff
	if (ctl & 1) {
		return 0xff;
	}

	// �쐬�ς݂̃f�[�^��Ԃ�
	return data[0];
}

//---------------------------------------------------------------------------
//
//	�f�[�^�쐬
//
//---------------------------------------------------------------------------
void FASTCALL JoyAtari::MakeData()
{
	const PPI::joyinfo_t *info;
	DWORD axis;

	ASSERT(this);
	ASSERT(ppi);

	// �f�[�^������
	info = ppi->GetJoyInfo(port);
	data[0] = 0xff;

	// Up
	axis = info->axis[1];
	if ((axis >= 0xfffff800) && (axis < 0xfffffc00)) {
		data[0] &= ~0x01;
	}
	// Down
	if ((axis >= 0x00000400) && (axis < 0x00000800)) {
		data[0] &= ~0x02;
	}

	// Left
	axis = info->axis[0];
	if ((axis >= 0xfffff800) && (axis < 0xfffffc00)) {
		data[0] &= ~0x04;
	}
	// Right
	if ((axis >= 0x00000400) && (axis < 0x00000800)) {
		data[0] &= ~0x08;
	}

	// �{�^��A
	if (info->button[0]) {
		data[0] &= ~0x40;
	}

	// �{�^��B
	if (info->button[1]) {
		data[0] &= ~0x20;
	}
}

//---------------------------------------------------------------------------
//
//	���\���e�[�u��
//
//---------------------------------------------------------------------------
const char* JoyAtari::AxisDescTable[] = {
	"X",
	"Y"
};

//---------------------------------------------------------------------------
//
//	�{�^���\���e�[�u��
//
//---------------------------------------------------------------------------
const char* JoyAtari::ButtonDescTable[] = {
	"A",
	"B"
};

//===========================================================================
//
//	�W���C�X�e�B�b�N(ATARI�W��+START/SELECT)
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	�R���X�g���N�^
//
//---------------------------------------------------------------------------
JoyASS::JoyASS(PPI *parent, int no) : JoyDevice(parent, no)
{
	// �^�C�vATSS
	id = MAKEID('A', 'T', 'S', 'S');
	type = 2;

	// 2��4�{�^���A�f�[�^��1
	axes = 2;
	buttons = 4;
	datas = 1;

	// �\���e�[�u��
	axis_desc = AxisDescTable;
	button_desc = ButtonDescTable;

	// �f�[�^�o�b�t�@�m��
	data = new DWORD[datas];

	// �����f�[�^�ݒ�
	data[0] = 0xff;
}

//---------------------------------------------------------------------------
//
//	�|�[�g�ǂݎ��(Read Only)
//
//---------------------------------------------------------------------------
DWORD FASTCALL JoyASS::ReadOnly(DWORD ctl) const
{
	ASSERT(this);
	ASSERT(ctl < 0x100);

	// PC4��1�Ȃ�A0xff
	if (ctl & 1) {
		return 0xff;
	}

	// �쐬�ς݂̃f�[�^��Ԃ�
	return data[0];
}

//---------------------------------------------------------------------------
//
//	�f�[�^�쐬
//
//---------------------------------------------------------------------------
void FASTCALL JoyASS::MakeData()
{
	const PPI::joyinfo_t *info;
	DWORD axis;

	ASSERT(this);
	ASSERT(ppi);

	// �f�[�^������
	info = ppi->GetJoyInfo(port);
	data[0] = 0xff;

	// Up
	axis = info->axis[1];
	if ((axis >= 0xfffff800) && (axis < 0xfffffc00)) {
		data[0] &= ~0x01;
	}
	// Down
	if ((axis >= 0x00000400) && (axis < 0x00000800)) {
		data[0] &= ~0x02;
	}

	// Left
	axis = info->axis[0];
	if ((axis >= 0xfffff800) && (axis < 0xfffffc00)) {
		data[0] &= ~0x04;
	}
	// Right
	if ((axis >= 0x00000400) && (axis < 0x00000800)) {
		data[0] &= ~0x08;
	}

	// �{�^��A
	if (info->button[0]) {
		data[0] &= ~0x40;
	}

	// �{�^��B
	if (info->button[1]) {
		data[0] &= ~0x20;
	}

	// START(���E���������Ƃ��ĕ\��)
	if (info->button[2]) {
		data[0] &= ~0x0c;
	}

	// SELECT(�㉺���������Ƃ��ĕ\��)
	if (info->button[3]) {
		data[0] &= ~0x03;
	}
}

//---------------------------------------------------------------------------
//
//	���\���e�[�u��
//
//---------------------------------------------------------------------------
const char* JoyASS::AxisDescTable[] = {
	"X",
	"Y"
};

//---------------------------------------------------------------------------
//
//	�{�^���\���e�[�u��
//
//---------------------------------------------------------------------------
const char* JoyASS::ButtonDescTable[] = {
	"A",
	"B",
	"START",
	"SELECT"
};

//===========================================================================
//
//	�W���C�X�e�B�b�N(�T�C�o�[�X�e�B�b�N�E�A�i���O)
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	�R���X�g���N�^
//
//---------------------------------------------------------------------------
JoyCyberA::JoyCyberA(PPI *parent, int no) : JoyDevice(parent, no)
{
	int i;

	// �^�C�vCYBA
	id = MAKEID('C', 'Y', 'B', 'A');
	type = 3;

	// 3��8�{�^���A�A�i���O�A�f�[�^��11
	axes = 3;
	buttons = 8;
	analog = TRUE;
	datas = 12;

	// �\���e�[�u��
	axis_desc = AxisDescTable;
	button_desc = ButtonDescTable;

	// �f�[�^�o�b�t�@�m��
	data = new DWORD[datas];

	// �����f�[�^�ݒ�
	for (i=0; i<12; i++) {
		// ACK,L/H,�{�^��
		if (i & 1) {
			data[i] = 0xbf;
		}
		else {
			data[i] = 0x9f;
		}

		// �Z���^�l��0x7f�Ƃ���
		if ((i >= 2) && (i <= 5)) {
			// �A�i���O�f�[�^H��7�ɂ���
			data[i] &= 0xf7;
		}
	}

	// �X�P�W���[���擾
	scheduler = (Scheduler*)ppi->GetVM()->SearchDevice(MAKEID('S', 'C', 'H', 'E'));
	ASSERT(scheduler);

	// �������Z�b�g(�R���g���[���������ւ����ꍇ�ɔ�����)
	Reset();
}

//---------------------------------------------------------------------------
//
//	���Z�b�g
//
//---------------------------------------------------------------------------
void FASTCALL JoyCyberA::Reset()
{
	ASSERT(this);
	ASSERT(scheduler);

	// ��{�N���X
	JoyDevice::Reset();

	// �V�[�P���X������
	seq = 0;

	// �R���g���[��0
	ctrl = 0;

	// ���ԋL��
	hus = scheduler->GetTotalTime();
}

//---------------------------------------------------------------------------
//
//	�Z�[�u
//
//---------------------------------------------------------------------------
BOOL FASTCALL JoyCyberA::Save(Fileio *fio, int ver)
{
	ASSERT(this);
	ASSERT(fio);

	// ��{�N���X
	if (!JoyDevice::Save(fio, ver)) {
		return FALSE;
	}

	// �V�[�P���X���Z�[�u
	if (!fio->Write(&seq, sizeof(seq))) {
		return FALSE;
	}

	// �R���g���[�����Z�[�u
	if (!fio->Write(&ctrl, sizeof(ctrl))) {
		return FALSE;
	}

	// ���Ԃ��Z�[�u
	if (!fio->Write(&hus, sizeof(hus))) {
		return FALSE;
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	���[�h
//
//---------------------------------------------------------------------------
BOOL FASTCALL JoyCyberA::Load(Fileio *fio, int ver)
{
	ASSERT(this);
	ASSERT(fio);

	// ��{�N���X
	if (!JoyDevice::Load(fio, ver)) {
		return FALSE;
	}

	// �V�[�P���X�����[�h
	if (!fio->Read(&seq, sizeof(seq))) {
		return FALSE;
	}

	// �R���g���[�������[�h
	if (!fio->Read(&ctrl, sizeof(ctrl))) {
		return FALSE;
	}

	// ���Ԃ����[�h
	if (!fio->Read(&hus, sizeof(hus))) {
		return FALSE;
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	�������x�ݒ�(���@�Ƃ̔�r�Ɋ�Â�)
//	�����m�ɂ�PC4���ŏ��ɗ��ĂĂ���APA4��b5��b6��������܂ł�100us�ȏ�
//	�������Ă��镵�͋C�����邪�A�����܂ł̓G�~�����[�V�������Ȃ�
//
//---------------------------------------------------------------------------
#define JOYCYBERA_CYCLE		108

//---------------------------------------------------------------------------
//
//	�|�[�g�ǂݎ��
//
//---------------------------------------------------------------------------
DWORD FASTCALL JoyCyberA::ReadPort(DWORD ctl)
{
	DWORD diff;
	DWORD n;

	// �V�[�P���X0�͖���
	if (seq == 0) {
		return 0xff;
	}

	// �V�[�P���X12�ȏ������
	if (seq >= 13) {
		// �V�[�P���X0
		seq = 0;
		return 0xff;
	}

	// �ύX�t���O���`�F�b�N
	if (changed) {
		// �t���O���Ƃ�
		changed = FALSE;

		// �f�[�^�쐬
		MakeData();
	}

	ASSERT((seq >= 1) && (seq <= 12));

	// �����擾
	diff = scheduler->GetTotalTime();
	diff -= hus;

	// ��������v�Z
	if (diff >= JOYCYBERA_CYCLE) {
		n = diff / JOYCYBERA_CYCLE;
		diff %= JOYCYBERA_CYCLE;

		// �V�[�P���X�����Z�b�g
		if ((seq & 1) == 0) {
			seq--;
		}
		// 2�P�ʂŐi�߂�
		seq += (2 * n);

		// ���Ԃ�␳
		hus += (JOYCYBERA_CYCLE * n);

		// +1
		if (diff >= (JOYCYBERA_CYCLE / 2)) {
			diff -= (JOYCYBERA_CYCLE / 2);
			seq++;
		}

		// �V�[�P���X�I�[�o�[�΍�
		if (seq >= 13) {
			seq = 0;
			return 0xff;
		}
	}
	if (diff >= (JOYCYBERA_CYCLE / 2)) {
		// �㔼�ɐݒ�
		if (seq & 1) {
			seq++;
		}
	}

	// �f�[�^�擾
	return ReadOnly(ctl);
}

//---------------------------------------------------------------------------
//
//	�|�[�g�ǂݎ��(Read Only)
//
//---------------------------------------------------------------------------
DWORD FASTCALL JoyCyberA::ReadOnly(DWORD /*ctl*/) const
{
	ASSERT(this);

	// �V�[�P���X0�͖���
	if (seq == 0) {
		return 0xff;
	}

	// �V�[�P���X12�ȏ������
	if (seq >= 13) {
		return 0xff;
	}

	// �V�[�P���X�ɏ]�����f�[�^��Ԃ�
	ASSERT((seq >= 1) && (seq <= 12));
	return data[seq - 1];
}

//---------------------------------------------------------------------------
//
//	�R���g���[��
//
//---------------------------------------------------------------------------
void FASTCALL JoyCyberA::Control(DWORD ctl)
{
	ASSERT(this);
	ASSERT(ctl < 0x100);

	// �V�[�P���X0(����)���ƃV�[�P���X11�ȍ~
	if ((seq == 0) || (seq >= 11)) {
		// 1��0�ŁA�V�[�P���X�J�n
		if (ctl) {
			// ����1�ɂ���
			ctrl = 1;
		}
		else {
			// ����0�ɂ���
			if (ctrl) {
				// 1��0�ւ̗�������
				seq = 1;
				hus = scheduler->GetTotalTime();
			}
			ctrl = 0;
		}
		return;
	}

	// �V�[�P���X1�ȍ~�ł́AACK�̂ݗL��
	ctrl = ctl;
	if (ctl) {
		return;
	}

	// �V�[�P���X��2�P�ʂŐi�߂���ʂ�����
	if ((seq & 1) == 0) {
		seq--;
	}
	seq += 2;

	// ���Ԃ��L��
	hus = scheduler->GetTotalTime();
}

//---------------------------------------------------------------------------
//
//	�f�[�^�쐬
//
//---------------------------------------------------------------------------
void FASTCALL JoyCyberA::MakeData()
{
	const PPI::joyinfo_t *info;
	DWORD axis;

	ASSERT(this);
	ASSERT(ppi);

	// �W���C�X�e�B�b�N���擾
	info = ppi->GetJoyInfo(port);

	// data[0]:�{�^��A,�{�^��B,�{�^��C,�{�^��D
	data[0] |= 0x0f;
	if (info->button[0]) {
		data[0] &= ~0x08;
	}
	if (info->button[1]) {
		data[0] &= ~0x04;
	}
	if (info->button[2]) {
		data[0] &= ~0x02;
	}
	if (info->button[3]) {
		data[0] &= ~0x01;
	}

	// data[1]:�{�^��E1,�{�^��E2,�{�^��F,�{�^��G
	data[1] |= 0x0f;
	if (info->button[4]) {
		data[1] &= ~0x08;
	}
	if (info->button[5]) {
		data[1] &= ~0x04;
	}
	if (info->button[6]) {
		data[1] &= ~0x02;
	}
	if (info->button[7]) {
		data[1] &= ~0x01;
	}

	// data[2]:1H
	axis = info->axis[1];
	axis = (axis + 0x800) >> 4;
	data[2] &= 0xf0;
	data[2] |= (axis >> 4);

	// data[3]:2H
	axis = info->axis[0];
	axis = (axis + 0x800) >> 4;
	data[3] &= 0xf0;
	data[3] |= (axis >> 4);

	// data[4]:3H
	axis = info->axis[3];
	axis = (axis + 0x800) >> 4;
	data[4] &= 0xf0;
	data[4] |= (axis >> 4);

	// data[5]:4H(�\��ƂȂ��Ă���B���@�ł�0)
	data[5] &= 0xf0;

	// data[6]:1L
	axis = info->axis[1];
	axis = (axis + 0x800) >> 4;
	data[6] &= 0xf0;
	data[6] |= (axis & 0x0f);

	// data[7]:2L
	axis = info->axis[0];
	axis = (axis + 0x800) >> 4;
	data[7] &= 0xf0;
	data[7] |= (axis & 0x0f);

	// data[8]:3L
	axis = info->axis[3];
	axis = (axis + 0x800) >> 4;
	data[8] &= 0xf0;
	data[8] |= (axis & 0x0f);

	// data[9]:4L(�\��ƂȂ��Ă���B���@�ł�0)
	data[9] &= 0xf0;

	// data[10]:A,B,A',B'
	// A,B�̓��o�[��̂̃~�j�{�^���AA'B'�͒ʏ�̉����{�^��
	// ���o�[��̂̃~�j�{�^���Ƃ��Ĉ���(�A�t�^�[�o�[�i�[II)
	data[10] &= 0xf0;
	data[10] |= 0x0f;
	if (info->button[0]) {
		data[10] &= ~0x08;
	}
	if (info->button[1]) {
		data[10] &= ~0x04;
	}
}

//---------------------------------------------------------------------------
//
//	���\���e�[�u��
//
//---------------------------------------------------------------------------
const char* JoyCyberA::AxisDescTable[] = {
	"Stick X",
	"Stick Y",
	"Throttle"
};

//---------------------------------------------------------------------------
//
//	�{�^���\���e�[�u��
//
//---------------------------------------------------------------------------
const char* JoyCyberA::ButtonDescTable[] = {
	"A",
	"B",
	"C",
	"D",
	"E1",
	"E2",
	"START",
	"SELECT"
};

//===========================================================================
//
//	�W���C�X�e�B�b�N(�T�C�o�[�X�e�B�b�N�E�f�W�^��)
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	�R���X�g���N�^
//
//---------------------------------------------------------------------------
JoyCyberD::JoyCyberD(PPI *parent, int no) : JoyDevice(parent, no)
{
	// �^�C�vCYBD
	id = MAKEID('C', 'Y', 'B', 'D');
	type = 4;

	// 3��6�{�^���A�f�[�^��2
	axes = 3;
	buttons = 6;
	datas = 2;

	// �\���e�[�u��
	axis_desc = AxisDescTable;
	button_desc = ButtonDescTable;

	// �f�[�^�o�b�t�@�m��
	data = new DWORD[datas];

	// �����f�[�^�ݒ�
	data[0] = 0xff;
	data[1] = 0xff;
}

//---------------------------------------------------------------------------
//
//	�|�[�g�ǂݎ��(Read Only)
//
//---------------------------------------------------------------------------
DWORD FASTCALL JoyCyberD::ReadOnly(DWORD ctl) const
{
	ASSERT(this);
	ASSERT(ctl < 0x100);
	ASSERT(data[0] < 0x100);
	ASSERT(data[1] < 0x100);

	// PC4�ɂ���ĕ�����
	if (ctl & 1) {
		return data[1];
	}
	else {
		return data[0];
	}
}

//---------------------------------------------------------------------------
//
//	�f�[�^�쐬
//
//---------------------------------------------------------------------------
void FASTCALL JoyCyberD::MakeData()
{
	const PPI::joyinfo_t *info;
	DWORD axis;

	ASSERT(this);
	ASSERT(ppi);

	// �f�[�^������
	info = ppi->GetJoyInfo(port);
	data[0] = 0xff;
	data[1] = 0xff;

	// Up
	axis = info->axis[1];
	if ((axis >= 0xfffff800) && (axis < 0xfffffc00)) {
		data[0] &= ~0x01;
	}
	// Down
	if ((axis >= 0x00000400) && (axis < 0x00000800)) {
		data[0] &= ~0x02;
	}

	// Left
	axis = info->axis[0];
	if ((axis >= 0xfffff800) && (axis < 0xfffffc00)) {
		data[0] &= ~0x04;
	}
	// Right
	if ((axis >= 0x00000400) && (axis < 0x00000800)) {
		data[0] &= ~0x08;
	}

	// �{�^��A
	if (info->button[0]) {
		data[0] &= ~0x20;
	}

	// �{�^��B
	if (info->button[1]) {
		data[0] &= ~0x40;
	}

	// �X���b�g��Up
	axis = info->axis[2];
	if ((axis >= 0xfffff800) && (axis < 0xfffffc00)) {
		data[1] &= ~0x01;
	}
	// �X���b�g��Down
	if ((axis >= 0x00000400) && (axis < 0x00000800)) {
		data[1] &= ~0x02;
	}

	// �{�^��C
	if (info->button[2]) {
		data[1] &= ~0x04;
	}

	// �{�^��D
	if (info->button[3]) {
		data[1] &= ~0x08;
	}

	// �{�^��E1
	if (info->button[4]) {
		data[1] &= ~0x20;
	}

	// �{�^��E2
	if (info->button[5]) {
		data[1] &= ~0x40;
	}
}

//---------------------------------------------------------------------------
//
//	���\���e�[�u��
//
//---------------------------------------------------------------------------
const char* JoyCyberD::AxisDescTable[] = {
	"X",
	"Y",
	"Throttle"
};

//---------------------------------------------------------------------------
//
//	�{�^���\���e�[�u��
//
//---------------------------------------------------------------------------
const char* JoyCyberD::ButtonDescTable[] = {
	"A",
	"B",
	"C",
	"D",
	"E1",
	"E2"
};

//===========================================================================
//
//	�W���C�X�e�B�b�N(MD3�{�^��)
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	�R���X�g���N�^
//
//---------------------------------------------------------------------------
JoyMd3::JoyMd3(PPI *parent, int no) : JoyDevice(parent, no)
{
	// �^�C�vMD3B
	id = MAKEID('M', 'D', '3', 'B');
	type = 5;

	// 2��4�{�^���A�f�[�^��2
	axes = 2;
	buttons = 4;
	datas = 2;

	// �\���e�[�u��
	axis_desc = AxisDescTable;
	button_desc = ButtonDescTable;

	// �f�[�^�o�b�t�@�m��
	data = new DWORD[datas];

	// �����f�[�^�ݒ�
	data[0] = 0xf3;
	data[1] = 0xff;
}

//---------------------------------------------------------------------------
//
//	�|�[�g�ǂݎ��(Read Only)
//
//---------------------------------------------------------------------------
DWORD FASTCALL JoyMd3::ReadOnly(DWORD ctl) const
{
	ASSERT(this);
	ASSERT(ctl < 0x100);
	ASSERT(data[0] < 0x100);
	ASSERT(data[1] < 0x100);

	// PC4�ɂ���ĕ�����
	if (ctl & 1) {
		return data[1];
	}
	else {
		return data[0];
	}
}

//---------------------------------------------------------------------------
//
//	�f�[�^�쐬
//
//---------------------------------------------------------------------------
void FASTCALL JoyMd3::MakeData()
{
	const PPI::joyinfo_t *info;
	DWORD axis;

	ASSERT(this);
	ASSERT(ppi);

	// �f�[�^������
	info = ppi->GetJoyInfo(port);
	data[0] = 0xf3;
	data[1] = 0xff;

	// Up
	axis = info->axis[1];
	if ((axis >= 0xfffff800) && (axis < 0xfffffc00)) {
		data[1] &= ~0x01;
		data[0] &= ~0x01;
	}
	// Down
	if ((axis >= 0x00000400) && (axis < 0x00000800)) {
		data[1] &= ~0x02;
		data[0] &= ~0x02;
	}

	// Left
	axis = info->axis[0];
	if ((axis >= 0xfffff800) && (axis < 0xfffffc00)) {
		data[1] &= ~0x04;
	}
	// Right
	if ((axis >= 0x00000400) && (axis < 0x00000800)) {
		data[1] &= ~0x08;
	}

	// �{�^��B
	if (info->button[1]) {
		data[1] &= ~0x20;
	}

	// �{�^��C
	if (info->button[2]) {
		data[1] &= ~0x40;
	}

	// �{�^��A
	if (info->button[0]) {
		data[0] &= ~0x20;
	}

	// �X�^�[�g�{�^��
	if (info->button[3]) {
		data[0] &= ~0x40;
	}
}

//---------------------------------------------------------------------------
//
//	���\���e�[�u��
//
//---------------------------------------------------------------------------
const char* JoyMd3::AxisDescTable[] = {
	"X",
	"Y"
};

//---------------------------------------------------------------------------
//
//	�{�^���\���e�[�u��
//
//---------------------------------------------------------------------------
const char* JoyMd3::ButtonDescTable[] = {
	"A",
	"B",
	"C",
	"START"
};

//===========================================================================
//
//	�W���C�X�e�B�b�N(MD6�{�^��)
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	�R���X�g���N�^
//
//---------------------------------------------------------------------------
JoyMd6::JoyMd6(PPI *parent, int no) : JoyDevice(parent, no)
{
	// �^�C�vMD6B
	id = MAKEID('M', 'D', '6', 'B');
	type = 6;

	// 2��8�{�^���A�f�[�^��3
	axes = 2;
	buttons = 8;
	datas = 5;

	// �\���e�[�u��
	axis_desc = AxisDescTable;
	button_desc = ButtonDescTable;

	// �f�[�^�o�b�t�@�m��
	data = new DWORD[datas];

	// �����f�[�^�ݒ�
	data[0] = 0xf3;
	data[1] = 0xff;
	data[2] = 0xf0;
	data[3] = 0xff;
	data[4] = 0xff;

	// �X�P�W���[���擾
	scheduler = (Scheduler*)ppi->GetVM()->SearchDevice(MAKEID('S', 'C', 'H', 'E'));
	ASSERT(scheduler);

	// �������Z�b�g(�R���g���[���������ւ����ꍇ�ɔ�����)
	Reset();
}

//---------------------------------------------------------------------------
//
//	���Z�b�g
//
//---------------------------------------------------------------------------
void FASTCALL JoyMd6::Reset()
{
	ASSERT(this);
	ASSERT(scheduler);

	// ��{�N���X
	JoyDevice::Reset();

	// �V�[�P���X�A�R���g���[���A���Ԃ�������
	seq = 0;
	ctrl = 0;
	hus = scheduler->GetTotalTime();
}

//---------------------------------------------------------------------------
//
//	�Z�[�u
//
//---------------------------------------------------------------------------
BOOL FASTCALL JoyMd6::Save(Fileio *fio, int ver)
{
	ASSERT(this);
	ASSERT(fio);

	// ��{�N���X
	if (!JoyDevice::Save(fio, ver)) {
		return FALSE;
	}

	// �V�[�P���X���Z�[�u
	if (!fio->Write(&seq, sizeof(seq))) {
		return FALSE;
	}

	// �R���g���[�����Z�[�u
	if (!fio->Write(&ctrl, sizeof(ctrl))) {
		return FALSE;
	}

	// ���Ԃ��Z�[�u
	if (!fio->Write(&hus, sizeof(hus))) {
		return FALSE;
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	���[�h
//
//---------------------------------------------------------------------------
BOOL FASTCALL JoyMd6::Load(Fileio *fio, int ver)
{
	ASSERT(this);
	ASSERT(fio);

	// ��{�N���X
	if (!JoyDevice::Load(fio, ver)) {
		return FALSE;
	}

	// �V�[�P���X�����[�h
	if (!fio->Read(&seq, sizeof(seq))) {
		return FALSE;
	}

	// �R���g���[�������[�h
	if (!fio->Read(&ctrl, sizeof(ctrl))) {
		return FALSE;
	}

	// ���Ԃ����[�h
	if (!fio->Read(&hus, sizeof(hus))) {
		return FALSE;
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	�|�[�g�ǂݎ��(Read Only)
//
//---------------------------------------------------------------------------
DWORD FASTCALL JoyMd6::ReadOnly(DWORD /*ctl*/) const
{
	ASSERT(this);
	ASSERT(data[0] < 0x100);
	ASSERT(data[1] < 0x100);
	ASSERT(data[2] < 0x100);
	ASSERT(data[3] < 0x100);
	ASSERT(data[4] < 0x100);

	// �V�[�P���X��
	switch (seq) {
		// ������� CTL=0
		case 0:
			return data[0];

		// 1��� CTL=1
		case 1:
			return data[1];

		// 1��� CTL=0
		case 2:
			return data[0];

		// 2��� CTL=1
		case 3:
			return data[1];

		// 6B�m��� CTL=0
		case 4:
			return data[2];

		// 6B�m��� CTL=1
		case 5:
			return data[3];

		// 6B�m��� CTL=0
		case 6:
			return data[4];

		// 6B�m��� CTL=1
		case 7:
			return data[1];

		// 6B�m��� CTL=0
		case 8:
			return data[0];

		// ���̑�(���蓾�Ȃ�)
		default:
			ASSERT(FALSE);
			break;
	}

	return 0xff;
}

//---------------------------------------------------------------------------
//
//	�R���g���[��
//
//---------------------------------------------------------------------------
void FASTCALL JoyMd6::Control(DWORD ctl)
{
	DWORD diff;

	ASSERT(this);
	ASSERT(ctl < 0x100);

	// bit0�̂ݕK�v
	ctl &= 0x01;

	// �K���X�V
	ctrl = ctl;

	// seq >= 3�Ȃ�A�O��̋N������1.8ms(3600hus)�o�߂������`�F�b�N
	// �o�߂��Ă���΁Aseq=0 or seq=1�Ƀ��Z�b�g(�����LV4)
	if (seq >= 3) {
		diff = scheduler->GetTotalTime();
		diff -= hus;
		if (diff >= 3600) {
			// ���Z�b�g
			if (ctl) {
				seq = 1;
				hus = scheduler->GetTotalTime();
			}
			else {
				seq = 0;
			}
			return;
		}
	}

	switch (seq) {
		// �V�[�P���X�O CTL=0
		case 0:
			// 1�Ȃ�A�V�[�P���X1�Ǝ��ԋL��
			if (ctl) {
				seq = 1;
				hus = scheduler->GetTotalTime();
			}
			break;

		// �ŏ���1�̌� CTL=1
		case 1:
			// 0�Ȃ�A�V�[�P���X2
			if (!ctl) {
				seq = 2;
			}
			break;

		// 1��0�̌� CTL=0
		case 2:
			// 1�Ȃ�A���ԃ`�F�b�N
			if (ctl) {
				diff = scheduler->GetTotalTime();
				diff -= hus;
				if (diff <= 2200) {
					// 1.1ms(2200hus)�ȉ��Ȃ玟�̃V�[�P���X��(6B�ǂݎ��)
					seq = 3;
				}
				else {
					// �\�����Ԃ��󂢂Ă���̂ŁA�V�[�P���X1�Ɠ����Ƃ݂Ȃ�(3B�ǂݎ��)
					seq = 1;
					hus = scheduler->GetTotalTime();
				}
			}
			break;

		// 6B�m��� CTL=1
		case 3:
			if (!ctl) {
				seq = 4;
			}
			break;

		// 6B�m��� CTL=0
		case 4:
			if (ctl) {
				seq = 5;
			}
			break;

		// 6B�m��� CTL=1
		case 5:
			if (!ctl) {
				seq = 6;
			}
			break;

		// 6B�m��� CTL=0
		case 6:
			if (ctl) {
				seq = 7;
			}
			break;

		// 1.8ms�̑҂�
		case 7:
			if (!ctl) {
				seq = 8;
			}
			break;

		// 1.8ms�̑҂�
		case 8:
			if (ctl) {
				seq = 7;
			}
			break;

		// ���̑�(���蓾�Ȃ�)
		default:
			ASSERT(FALSE);
			break;
	}
}

//---------------------------------------------------------------------------
//
//	�f�[�^�쐬
//
//---------------------------------------------------------------------------
void FASTCALL JoyMd6::MakeData()
{
	const PPI::joyinfo_t *info;
	DWORD axis;

	ASSERT(this);
	ASSERT(ppi);

	// �f�[�^������
	info = ppi->GetJoyInfo(port);
	data[0] = 0xf3;
	data[1] = 0xff;
	data[2] = 0xf0;
	data[3] = 0xff;
	data[4] = 0xff;

	// Up(data[0], data[1], data[4])
	axis = info->axis[1];
	if ((axis >= 0xfffff800) && (axis < 0xfffffc00)) {
		data[0] &= ~0x01;
		data[1] &= ~0x01;
		data[4] &= ~0x01;
	}
	// Down(data[0], data[1], data[4])
	if ((axis >= 0x00000400) && (axis < 0x00000800)) {
		data[0] &= ~0x02;
		data[1] &= ~0x02;
		data[4] &= ~0x02;
	}

	// Left(data[1], data[4])
	axis = info->axis[0];
	if ((axis >= 0xfffff800) && (axis < 0xfffffc00)) {
		data[1] &= ~0x04;
		data[4] &= ~0x04;
	}
	// Right(data[1], data[4])
	if ((axis >= 0x00000400) && (axis < 0x00000800)) {
		data[1] &= ~0x08;
		data[4] &= ~0x08;
	}

	// �{�^��B(data[1], data[3], data[4])
	if (info->button[1]) {
		// 3B�݊�
		data[1] &= ~0x20;

		// (�����LV4)
		data[3] &= ~0x20;

		// (SFII'patch)
		data[4] &= ~0x40;
	}

	// �{�^��C(data[1], data[3])
	if (info->button[2]) {
		// 3B�݊�
		data[1] &= ~0x40;

		// (SFII'patch)
		data[3] &= ~0x20;

		// (�����LV4)
		data[3] &= ~0x40;
	}

	// �{�^��A(data[0], data[2], data[4])
	if (info->button[0]) {
		// 3B�݊�
		data[0] &= ~0x20;

		// 6B�}�[�J
		data[2] &= ~0x20;

		// (SFII'patch)
		data[4] &= ~0x20;
	}

	// �X�^�[�g�{�^��(data[0], data[2])
	if (info->button[6]) {
		// 3B�݊�
		data[0] &= ~0x40;

		// 6B�}�[�J
		data[2] &= ~0x40;
	}

	// �{�^��X(data[3])
	if (info->button[3]) {
		data[3] &= ~0x04;
	}

	// �{�^��Y(data[3])
	if (info->button[4]) {
		data[3] &= ~0x02;
	}

	// �{�^��Z(data[3])
	if (info->button[5]) {
		data[3] &= ~0x01;
	}

	// MODE�{�^��(data[3])
	if (info->button[7]) {
		data[3] &= ~0x08;
	}
}

//---------------------------------------------------------------------------
//
//	���\���e�[�u��
//
//---------------------------------------------------------------------------
const char* JoyMd6::AxisDescTable[] = {
	"X",
	"Y"
};

//---------------------------------------------------------------------------
//
//	�{�^���\���e�[�u��
//
//---------------------------------------------------------------------------
const char* JoyMd6::ButtonDescTable[] = {
	"A",
	"B",
	"C",
	"X",
	"Y",
	"Z",
	"START",
	"MODE"
};

//===========================================================================
//
//	�W���C�X�e�B�b�N(CPSF-SFC)
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	�R���X�g���N�^
//
//---------------------------------------------------------------------------
JoyCpsf::JoyCpsf(PPI *parent, int no) : JoyDevice(parent, no)
{
	// �^�C�vCPSF
	id = MAKEID('C', 'P', 'S', 'F');
	type = 7;

	// 2��8�{�^���A�f�[�^��2
	axes = 2;
	buttons = 8;
	datas = 2;

	// �\���e�[�u��
	axis_desc = AxisDescTable;
	button_desc = ButtonDescTable;

	// �f�[�^�o�b�t�@�m��
	data = new DWORD[datas];

	// �����f�[�^�ݒ�
	data[0] = 0xff;
	data[1] = 0xff;
}

//---------------------------------------------------------------------------
//
//	�|�[�g�ǂݎ��(Read Only)
//
//---------------------------------------------------------------------------
DWORD FASTCALL JoyCpsf::ReadOnly(DWORD ctl) const
{
	ASSERT(this);
	ASSERT(ctl < 0x100);
	ASSERT(data[0] < 0x100);
	ASSERT(data[1] < 0x100);

	// PC4�ɂ���ĕ�����
	if (ctl & 1) {
		return data[1];
	}
	else {
		return data[0];
	}
}

//---------------------------------------------------------------------------
//
//	�f�[�^�쐬
//
//---------------------------------------------------------------------------
void FASTCALL JoyCpsf::MakeData()
{
	const PPI::joyinfo_t *info;
	DWORD axis;

	ASSERT(this);
	ASSERT(ppi);

	// �f�[�^������
	info = ppi->GetJoyInfo(port);
	data[0] = 0xff;
	data[1] = 0xff;

	// Up
	axis = info->axis[1];
	if ((axis >= 0xfffff800) && (axis < 0xfffffc00)) {
		data[0] &= ~0x01;
	}
	// Down
	if ((axis >= 0x00000400) && (axis < 0x00000800)) {
		data[0] &= ~0x02;
	}

	// Left
	axis = info->axis[0];
	if ((axis >= 0xfffff800) && (axis < 0xfffffc00)) {
		data[0] &= ~0x04;
	}
	// Right
	if ((axis >= 0x00000400) && (axis < 0x00000800)) {
		data[0] &= ~0x08;
	}

	// �{�^��Y
	if (info->button[0]) {
		data[1] &= ~0x02;
	}

	// �{�^��X
	if (info->button[1]) {
		data[1] &= ~0x04;
	}

	// �{�^��B
	if (info->button[2]) {
		data[0] &= ~0x40;
	}

	// �{�^��A
	if (info->button[3]) {
		data[0] &= ~0x20;
	}

	// �{�^��L
	if (info->button[4]) {
		data[1] &= ~0x20;
	}

	// �{�^��R
	if (info->button[5]) {
		data[1] &= ~0x01;
	}

	// �X�^�[�g�{�^��
	if (info->button[6]) {
		data[1] &= ~0x40;
	}

	// �Z���N�g�{�^��
	if (info->button[7]) {
		data[1] &= ~0x08;
	}
}

//---------------------------------------------------------------------------
//
//	���\���e�[�u��
//
//---------------------------------------------------------------------------
const char* JoyCpsf::AxisDescTable[] = {
	"X",
	"Y"
};

//---------------------------------------------------------------------------
//
//	�{�^���\���e�[�u��
//
//---------------------------------------------------------------------------
const char* JoyCpsf::ButtonDescTable[] = {
	"Y",
	"X",
	"B",
	"A",
	"L",
	"R",
	"START",
	"SELECT"
};

//===========================================================================
//
//	�W���C�X�e�B�b�N(CPSF-MD)
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	�R���X�g���N�^
//
//---------------------------------------------------------------------------
JoyCpsfMd::JoyCpsfMd(PPI *parent, int no) : JoyDevice(parent, no)
{
	// �^�C�vCPSM
	id = MAKEID('C', 'P', 'S', 'M');
	type = 8;

	// 2��8�{�^���A�f�[�^��2
	axes = 2;
	buttons = 8;
	datas = 2;

	// �\���e�[�u��
	axis_desc = AxisDescTable;
	button_desc = ButtonDescTable;

	// �f�[�^�o�b�t�@�m��
	data = new DWORD[datas];

	// �����f�[�^�ݒ�
	data[0] = 0xff;
	data[1] = 0xff;
}

//---------------------------------------------------------------------------
//
//	�|�[�g�ǂݎ��(Read Only)
//
//---------------------------------------------------------------------------
DWORD FASTCALL JoyCpsfMd::ReadOnly(DWORD ctl) const
{
	ASSERT(this);
	ASSERT(ctl < 0x100);
	ASSERT(data[0] < 0x100);
	ASSERT(data[1] < 0x100);

	// PC4�ɂ���ĕ�����
	if (ctl & 1) {
		return data[1];
	}
	else {
		return data[0];
	}
}

//---------------------------------------------------------------------------
//
//	�f�[�^�쐬
//
//---------------------------------------------------------------------------
void FASTCALL JoyCpsfMd::MakeData()
{
	const PPI::joyinfo_t *info;
	DWORD axis;

	ASSERT(this);
	ASSERT(ppi);

	// �f�[�^������
	info = ppi->GetJoyInfo(port);
	data[0] = 0xff;
	data[1] = 0xff;

	// Up
	axis = info->axis[1];
	if ((axis >= 0xfffff800) && (axis < 0xfffffc00)) {
		data[0] &= ~0x01;
	}
	// Down
	if ((axis >= 0x00000400) && (axis < 0x00000800)) {
		data[0] &= ~0x02;
	}

	// Left
	axis = info->axis[0];
	if ((axis >= 0xfffff800) && (axis < 0xfffffc00)) {
		data[0] &= ~0x04;
	}
	// Right
	if ((axis >= 0x00000400) && (axis < 0x00000800)) {
		data[0] &= ~0x08;
	}

	// �{�^��A
	if (info->button[0]) {
		data[0] &= ~0x20;
	}

	// �{�^��B
	if (info->button[1]) {
		data[0] &= ~0x40;
	}

	// �{�^��C
	if (info->button[2]) {
		data[1] &= ~0x20;
	}

	// �{�^��X
	if (info->button[3]) {
		data[1] &= ~0x04;
	}

	// �{�^��Y
	if (info->button[4]) {
		data[1] &= ~0x02;
	}

	// �{�^��Z
	if (info->button[5]) {
		data[1] &= ~0x01;
	}

	// �X�^�[�g�{�^��
	if (info->button[6]) {
		data[1] &= ~0x40;
	}

	// MODE�{�^��
	if (info->button[7]) {
		data[1] &= ~0x08;
	}
}

//---------------------------------------------------------------------------
//
//	���\���e�[�u��
//
//---------------------------------------------------------------------------
const char* JoyCpsfMd::AxisDescTable[] = {
	"X",
	"Y"
};

//---------------------------------------------------------------------------
//
//	�{�^���\���e�[�u��
//
//---------------------------------------------------------------------------
const char* JoyCpsfMd::ButtonDescTable[] = {
	"A",
	"B",
	"C",
	"X",
	"Y",
	"Z",
	"START",
	"MODE"
};

//===========================================================================
//
//	�W���C�X�e�B�b�N(�}�W�J���p�b�h)
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	�R���X�g���N�^
//
//---------------------------------------------------------------------------
JoyMagical::JoyMagical(PPI *parent, int no) : JoyDevice(parent, no)
{
	// �^�C�vMAGI
	id = MAKEID('M', 'A', 'G', 'I');
	type = 9;

	// 2��6�{�^���A�f�[�^��2
	axes = 2;
	buttons = 6;
	datas = 2;

	// �\���e�[�u��
	axis_desc = AxisDescTable;
	button_desc = ButtonDescTable;

	// �f�[�^�o�b�t�@�m��
	data = new DWORD[datas];

	// �����f�[�^�ݒ�
	data[0] = 0xff;
	data[1] = 0xfc;
}

//---------------------------------------------------------------------------
//
//	�|�[�g�ǂݎ��(Read Only)
//
//---------------------------------------------------------------------------
DWORD FASTCALL JoyMagical::ReadOnly(DWORD ctl) const
{
	ASSERT(this);
	ASSERT(ctl < 0x100);
	ASSERT(data[0] < 0x100);
	ASSERT(data[1] < 0x100);

	// PC4�ɂ���ĕ�����
	if (ctl & 1) {
		return data[1];
	}
	else {
		return data[0];
	}
}

//---------------------------------------------------------------------------
//
//	�f�[�^�쐬
//
//---------------------------------------------------------------------------
void FASTCALL JoyMagical::MakeData()
{
	const PPI::joyinfo_t *info;
	DWORD axis;

	ASSERT(this);
	ASSERT(ppi);

	// �f�[�^������
	info = ppi->GetJoyInfo(port);
	data[0] = 0xff;
	data[1] = 0xfc;

	// Up
	axis = info->axis[1];
	if ((axis >= 0xfffff800) && (axis < 0xfffffc00)) {
		data[0] &= ~0x01;
	}
	// Down
	if ((axis >= 0x00000400) && (axis < 0x00000800)) {
		data[0] &= ~0x02;
	}

	// Left
	axis = info->axis[0];
	if ((axis >= 0xfffff800) && (axis < 0xfffffc00)) {
		data[0] &= ~0x04;
	}
	// Right
	if ((axis >= 0x00000400) && (axis < 0x00000800)) {
		data[0] &= ~0x08;
	}

	// �{�^��A
	if (info->button[0]) {
		data[0] &= ~0x40;
	}

	// �{�^��B
	if (info->button[1]) {
		data[1] &= ~0x40;
	}

	// �{�^��C
	if (info->button[2]) {
		data[0] &= ~0x20;
	}

	// �{�^��D
	if (info->button[3]) {
		data[1] &= ~0x40;
	}

	// �{�^��R
	if (info->button[4]) {
		data[1] &= ~0x08;
	}

	// �{�^��L
	if (info->button[5]) {
		data[1] &= ~0x04;
	}
}

//---------------------------------------------------------------------------
//
//	���\���e�[�u��
//
//---------------------------------------------------------------------------
const char* JoyMagical::AxisDescTable[] = {
	"X",
	"Y"
};

//---------------------------------------------------------------------------
//
//	�{�^���\���e�[�u��
//
//---------------------------------------------------------------------------
const char* JoyMagical::ButtonDescTable[] = {
	"A",
	"B",
	"C",
	"D",
	"R",
	"L"
};

//===========================================================================
//
//	�W���C�X�e�B�b�N(XPD-1LR)
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	�R���X�g���N�^
//
//---------------------------------------------------------------------------
JoyLR::JoyLR(PPI *parent, int no) : JoyDevice(parent, no)
{
	// �^�C�vXPLR
	id = MAKEID('X', 'P', 'L', 'R');
	type = 10;

	// 4��2�{�^���A�f�[�^��2
	axes = 4;
	buttons = 2;
	datas = 2;

	// �\���e�[�u��
	axis_desc = AxisDescTable;
	button_desc = ButtonDescTable;

	// �f�[�^�o�b�t�@�m��
	data = new DWORD[datas];

	// �����f�[�^�ݒ�
	data[0] = 0xff;
	data[1] = 0xff;
}

//---------------------------------------------------------------------------
//
//	�|�[�g�ǂݎ��(Read Only)
//
//---------------------------------------------------------------------------
DWORD FASTCALL JoyLR::ReadOnly(DWORD ctl) const
{
	ASSERT(this);
	ASSERT(ctl < 0x100);
	ASSERT(data[0] < 0x100);
	ASSERT(data[1] < 0x100);

	// PC4�ɂ���ĕ�����
	if (ctl & 1) {
		return data[1];
	}
	else {
		return data[0];
	}
}

//---------------------------------------------------------------------------
//
//	�f�[�^�쐬
//
//---------------------------------------------------------------------------
void FASTCALL JoyLR::MakeData()
{
	const PPI::joyinfo_t *info;
	DWORD axis;

	ASSERT(this);
	ASSERT(ppi);

	// �f�[�^������
	info = ppi->GetJoyInfo(port);
	data[0] = 0xff;
	data[1] = 0xff;

	// �E��Up
	axis = info->axis[3];
	if ((axis >= 0xfffff800) && (axis < 0xfffffc00)) {
		data[1] &= ~0x01;
	}
	// �E��Down
	if ((axis >= 0x00000400) && (axis < 0x00000800)) {
		data[1] &= ~0x02;
	}

	// �E��Left
	axis = info->axis[2];
	if ((axis >= 0xfffff800) && (axis < 0xfffffc00)) {
		data[1] &= ~0x04;
	}
	// �E��Right
	if ((axis >= 0x00000400) && (axis < 0x00000800)) {
		data[1] &= ~0x08;
	}

	// �{�^��A
	if (info->button[0]) {
		data[1] &= ~0x40;
	}

	// �{�^��B
	if (info->button[1]) {
		data[1] &= ~0x20;
	}

	// ����Up
	axis = info->axis[1];
	if ((axis >= 0xfffff800) && (axis < 0xfffffc00)) {
		data[0] &= ~0x01;
	}
	// ����Down
	if ((axis >= 0x00000400) && (axis < 0x00000800)) {
		data[0] &= ~0x02;
	}

	// �E��Left
	axis = info->axis[0];
	if ((axis >= 0xfffff800) && (axis < 0xfffffc00)) {
		data[0] &= ~0x04;
	}
	// �E��Right
	if ((axis >= 0x00000400) && (axis < 0x00000800)) {
		data[0] &= ~0x08;
	}

	// �{�^��A
	if (info->button[0]) {
		data[0] &= ~0x40;
	}

	// �{�^��B
	if (info->button[1]) {
		data[0] &= ~0x20;
	}
}

//---------------------------------------------------------------------------
//
//	���\���e�[�u��
//
//---------------------------------------------------------------------------
const char* JoyLR::AxisDescTable[] = {
	"Left-X",
	"Left-Y",
	"Right-X",
	"Right-Y"
};

//---------------------------------------------------------------------------
//
//	�{�^���\���e�[�u��
//
//---------------------------------------------------------------------------
const char* JoyLR::ButtonDescTable[] = {
	"A",
	"B"
};

//===========================================================================
//
//	�W���C�X�e�B�b�N(�p�b�N�����h��p�p�b�h)
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	�R���X�g���N�^
//
//---------------------------------------------------------------------------
JoyPacl::JoyPacl(PPI *parent, int no) : JoyDevice(parent, no)
{
	// �^�C�vPACL
	id = MAKEID('P', 'A', 'C', 'L');
	type = 11;

	// 0��3�{�^���A�f�[�^��1
	axes = 0;
	buttons = 3;
	datas = 1;

	// �\���e�[�u��
	button_desc = ButtonDescTable;

	// �f�[�^�o�b�t�@�m��
	data = new DWORD[datas];

	// �����f�[�^�ݒ�
	data[0] = 0xff;
}

//---------------------------------------------------------------------------
//
//	�|�[�g�ǂݎ��(Read Only)
//
//---------------------------------------------------------------------------
DWORD FASTCALL JoyPacl::ReadOnly(DWORD ctl) const
{
	ASSERT(this);
	ASSERT(ctl < 0x100);
	ASSERT(data[0] < 0x100);

	// PC4��1�Ȃ�A0xff
	if (ctl & 1) {
		return 0xff;
	}

	// �쐬�ς݂̃f�[�^��Ԃ�
	return data[0];
}

//---------------------------------------------------------------------------
//
//	�f�[�^�쐬
//
//---------------------------------------------------------------------------
void FASTCALL JoyPacl::MakeData()
{
	const PPI::joyinfo_t *info;

	ASSERT(this);
	ASSERT(ppi);

	// �f�[�^������
	info = ppi->GetJoyInfo(port);
	data[0] = 0xff;

	// �{�^��A(Left)
	if (info->button[0]) {
		data[0] &= ~0x04;
	}

	// �{�^��B(Jump)
	if (info->button[1]) {
		data[0] &= ~0x20;
	}

	// �{�^��C(Right)
	if (info->button[2]) {
		data[0] &= ~0x08;
	}
}

//---------------------------------------------------------------------------
//
//	�{�^���\���e�[�u��
//
//---------------------------------------------------------------------------
const char* JoyPacl::ButtonDescTable[] = {
	"Left",
	"Jump",
	"Right",
};

//===========================================================================
//
//	�W���C�X�e�B�b�N(BM68��p�R���g���[��)
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	�R���X�g���N�^
//
//---------------------------------------------------------------------------
JoyBM::JoyBM(PPI *parent, int no) : JoyDevice(parent, no)
{
	// �^�C�vBM68
	id = MAKEID('B', 'M', '6', '8');
	type = 12;

	// 0��6�{�^���A�f�[�^��1
	axes = 0;
	buttons = 6;
	datas = 1;

	// �\���e�[�u��
	button_desc = ButtonDescTable;

	// �f�[�^�o�b�t�@�m��
	data = new DWORD[datas];

	// �����f�[�^�ݒ�
	data[0] = 0xff;
}

//---------------------------------------------------------------------------
//
//	�|�[�g�ǂݎ��(Read Only)
//
//---------------------------------------------------------------------------
DWORD FASTCALL JoyBM::ReadOnly(DWORD ctl) const
{
	ASSERT(this);
	ASSERT(ctl < 0x100);
	ASSERT(data[0] < 0x100);

	// PC4��1�Ȃ�A0xff
	if (ctl & 1) {
		return 0xff;
	}

	// �쐬�ς݂̃f�[�^��Ԃ�
	return data[0];
}

//---------------------------------------------------------------------------
//
//	�f�[�^�쐬
//
//---------------------------------------------------------------------------
void FASTCALL JoyBM::MakeData()
{
	const PPI::joyinfo_t *info;

	ASSERT(this);
	ASSERT(ppi);

	// �f�[�^������
	info = ppi->GetJoyInfo(port);
	data[0] = 0xff;

	// �{�^��1(C)
	if (info->button[0]) {
		data[0] &= ~0x08;
	}

	// �{�^��2(C+,D-)
	if (info->button[1]) {
		data[0] &= ~0x04;
	}

	// �{�^��3(D)
	if (info->button[2]) {
		data[0] &= ~0x40;
	}

	// �{�^��4(D+,E-)
	if (info->button[3]) {
		data[0] &= ~0x20;
	}

	// �{�^��5(E)
	if (info->button[4]) {
		data[0] &= ~0x02;
	}

	// �{�^��F(Hat)
	if (info->button[5]) {
		data[0] &= ~0x01;
	}
}

//---------------------------------------------------------------------------
//
//	�{�^���\���e�[�u��
//
//---------------------------------------------------------------------------
const char* JoyBM::ButtonDescTable[] = {
	"C",
	"C#",
	"D",
	"D#",
	"E",
	"HiHat"
};
