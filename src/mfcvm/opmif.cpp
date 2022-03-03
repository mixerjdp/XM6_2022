//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 �o�h�D(ytanaka@ipc-tokai.or.jp)
//	[ OPM(YM2151) ]
//
//---------------------------------------------------------------------------

#include "os.h"
#include "xm6.h"
#include "vm.h"
#include "log.h"
#include "mfp.h"
#include "schedule.h"
#include "adpcm.h"
#include "fdd.h"
#include "fileio.h"
#include "config.h"
#include "opm.h"
#include "opmif.h"

//===========================================================================
//
//	OPM
//
//===========================================================================
//#define OPM_LOG

//---------------------------------------------------------------------------
//
//	�R���X�g���N�^
//
//---------------------------------------------------------------------------
OPMIF::OPMIF(VM *p) : MemDevice(p)
{
	// �f�o�C�XID��������
	dev.id = MAKEID('O', 'P', 'M', ' ');
	dev.desc = "OPM (YM2151)";

	// �J�n�A�h���X�A�I���A�h���X
	memdev.first = 0xe90000;
	memdev.last = 0xe91fff;

	// ���[�N�N���A
	mfp = NULL;
	fdd = NULL;
	adpcm = NULL;
	engine = NULL;
	opmbuf = NULL;
}

//---------------------------------------------------------------------------
//
//	������
//
//---------------------------------------------------------------------------
BOOL FASTCALL OPMIF::Init()
{
	int i;

	ASSERT(this);

	// ��{�N���X
	if (!MemDevice::Init()) {
		return FALSE;
	}

	// MFP�擾
	ASSERT(!mfp);
	mfp = (MFP*)vm->SearchDevice(MAKEID('M', 'F', 'P', ' '));
	ASSERT(mfp);

	// ADPCM�擾
	ASSERT(!adpcm);
	adpcm = (ADPCM*)vm->SearchDevice(MAKEID('A', 'P', 'C', 'M'));
	ASSERT(adpcm);

	// FDD�擾
	ASSERT(!fdd);
	fdd = (FDD*)vm->SearchDevice(MAKEID('F', 'D', 'D', ' '));
	ASSERT(fdd);

	// �C�x���g�쐬
	event[0].SetDevice(this);
	event[0].SetDesc("Timer-A");
	event[1].SetDevice(this);
	event[1].SetDesc("Timer-B");
	for (i=0; i<2; i++) {
		event[i].SetUser(i);
		event[i].SetTime(0);
		scheduler->AddEvent(&event[i]);
	}

	// �o�b�t�@�m��
	try {
		opmbuf = new DWORD [BufMax * 2];
	}
	catch (...) {
		return FALSE;
	}
	if (!opmbuf) {
		return FALSE;
	}

	// ���[�N�N���A
	memset(&opm, 0, sizeof(opm));
	memset(&bufinfo, 0, sizeof(bufinfo));
	bufinfo.max = BufMax;
	bufinfo.sound = TRUE;

	// �o�b�t�@������
	InitBuf(44100);

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	�N���[���A�b�v
//
//---------------------------------------------------------------------------
void FASTCALL OPMIF::Cleanup()
{
	ASSERT(this);

	if (opmbuf) {
		delete[] opmbuf;
		opmbuf = NULL;
	}

	// ��{�N���X��
	MemDevice::Cleanup();
}

//---------------------------------------------------------------------------
//
//	���Z�b�g
//
//---------------------------------------------------------------------------
void FASTCALL OPMIF::Reset()
{
	int i;

	ASSERT(this);
	ASSERT_DIAG();

	LOG0(Log::Normal, "���Z�b�g");

	// �C�x���g���~�߂�
	event[0].SetTime(0);
	event[1].SetTime(0);

	// �o�b�t�@�N���A
	memset(opmbuf, 0, sizeof(DWORD) * (BufMax * 2));

	// �G���W�����w�肳��Ă���΃��Z�b�g
	if (engine) {
		engine->Reset();
	}

	// ���W�X�^�N���A
	for (i=0; i<0x100; i++) {
		if (i == 8) {
			continue;
		}
		if ((i >= 0x60) && (i <= 0x7f)) {
			Output(i, 0x7f);
			continue;
		}
		if ((i >= 0xe0) && (i <= 0xff)) {
			Output(i, 0xff);
			continue;
		}
		Output(i, 0);
	}
	Output(0x19, 0x80);
	for (i=0; i<8; i++) {
		Output(8, i);
		opm.key[i] = i;
	}

	// ���̑����[�N�G���A��������
	opm.addr = 0;
	opm.busy = FALSE;
	for (i=0; i<2; i++) {
		opm.enable[i] = FALSE;
		opm.action[i] = FALSE;
		opm.interrupt[i] = FALSE;
		opm.time[i] = 0;
	}
	opm.started = FALSE;

	// ���荞�ݗv���Ȃ�
	mfp->SetGPIP(3, 1);
}

//---------------------------------------------------------------------------
//
//	�Z�[�u
//
//---------------------------------------------------------------------------
BOOL FASTCALL OPMIF::Save(Fileio *fio, int ver)
{
	size_t sz;

	ASSERT(this);
	ASSERT(fio);
	ASSERT_DIAG();

	LOG0(Log::Normal, "�Z�[�u");

	// �T�C�Y���Z�[�u
	sz = sizeof(opm_t);
	if (!fio->Write(&sz, sizeof(sz))) {
		return FALSE;
	}

	// �{�̂��Z�[�u
	if (!fio->Write(&opm, (int)sz)) {
		return FALSE;
	}

	// �o�b�t�@���̃T�C�Y���Z�[�u
	sz = sizeof(opmbuf_t);
	if (!fio->Write(&sz, sizeof(sz))) {
		return FALSE;
	}

	// �o�b�t�@�����Z�[�u
	if (!fio->Write(&bufinfo, (int)sz)) {
		return FALSE;
	}

	// �C�x���g���Z�[�u
	if (!event[0].Save(fio, ver)) {
		return FALSE;
	}
	if (!event[1].Save(fio, ver)) {
		return FALSE;
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	���[�h
//
//---------------------------------------------------------------------------
BOOL FASTCALL OPMIF::Load(Fileio *fio, int ver)
{
	size_t sz;
	int i;

	ASSERT(this);
	ASSERT(fio);
	ASSERT_DIAG();

	LOG0(Log::Normal, "���[�h");

	// �T�C�Y�����[�h�A�ƍ�
	if (!fio->Read(&sz, sizeof(sz))) {
		return FALSE;
	}
	if (sz != sizeof(opm_t)) {
		return FALSE;
	}

	// �{�̂����[�h
	if (!fio->Read(&opm, (int)sz)) {
		return FALSE;
	}

	// �o�b�t�@���̃T�C�Y�����[�h�A�ƍ�
	if (!fio->Read(&sz, sizeof(sz))) {
		return FALSE;
	}
	if (sz != sizeof(opmbuf_t)) {
		return FALSE;
	}

	// �o�b�t�@�������[�h
	if (!fio->Read(&bufinfo, (int)sz)) {
		return FALSE;
	}

	// �C�x���g�����[�h
	if (!event[0].Load(fio, ver)) {
		return FALSE;
	}
	if (!event[1].Load(fio, ver)) {
		return FALSE;
	}

	// �o�b�t�@���N���A
	InitBuf(bufinfo.rate * 100);

	// �G���W���ւ̃��W�X�^�Đݒ�
	if (engine) {
		// ���W�X�^����:�m�C�Y�ALFO�APMD�AAMD�ACSM
		engine->SetReg(0x0f, opm.reg[0x0f]);
		engine->SetReg(0x14, opm.reg[0x14] & 0x80);
		engine->SetReg(0x18, opm.reg[0x18]);
		engine->SetReg(0x19, opm.reg[0x19]);

		// ���W�X�^����:���W�X�^
		for (i=0x20; i<0x100; i++) {
			engine->SetReg(i, opm.reg[i]);
		}

		// ���W�X�^����:�L�[
		for (i=0; i<8; i++) {
			engine->SetReg(8, opm.key[i]);
		}

		// ���W�X�^����:LFO
		engine->SetReg(1, 2);
		engine->SetReg(1, 0);
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	�ݒ�K�p
//
//---------------------------------------------------------------------------
void FASTCALL OPMIF::ApplyCfg(const Config* /*config*/)
{
	ASSERT(this);
	ASSERT_DIAG();

	LOG0(Log::Normal, "�ݒ�K�p");
}

#if !defined(NDEBUG)
//---------------------------------------------------------------------------
//
//	�f�f
//
//---------------------------------------------------------------------------
void FASTCALL OPMIF::AssertDiag() const
{
	// ��{�N���X
	MemDevice::AssertDiag();

	ASSERT(this);
	ASSERT(GetID() == MAKEID('O', 'P', 'M', ' '));
	ASSERT(memdev.first == 0xe90000);
	ASSERT(memdev.last == 0xe91fff);
	ASSERT(mfp);
	ASSERT(mfp->GetID() == MAKEID('M', 'F', 'P', ' '));
	ASSERT(adpcm);
	ASSERT(adpcm->GetID() == MAKEID('A', 'P', 'C', 'M'));
	ASSERT(fdd);
	ASSERT(fdd->GetID() == MAKEID('F', 'D', 'D', ' '));
	ASSERT(opm.addr < 0x100);
	ASSERT((opm.busy == TRUE) || (opm.busy == FALSE));
	ASSERT((opm.enable[0] == TRUE) || (opm.enable[0] == FALSE));
	ASSERT((opm.enable[1] == TRUE) || (opm.enable[1] == FALSE));
	ASSERT((opm.action[0] == TRUE) || (opm.action[0] == FALSE));
	ASSERT((opm.action[1] == TRUE) || (opm.action[1] == FALSE));
	ASSERT((opm.interrupt[0] == TRUE) || (opm.interrupt[0] == FALSE));
	ASSERT((opm.interrupt[1] == TRUE) || (opm.interrupt[1] == FALSE));
	ASSERT((opm.started == TRUE) || (opm.started == FALSE));
	ASSERT(bufinfo.max == BufMax);
	ASSERT(bufinfo.num <= bufinfo.max);
	ASSERT(bufinfo.read < bufinfo.max);
	ASSERT(bufinfo.write < bufinfo.max);
	ASSERT((bufinfo.sound == TRUE) || (bufinfo.sound == FALSE));
}
#endif	// NDEBUG

//---------------------------------------------------------------------------
//
//	�o�C�g�ǂݍ���
//
//---------------------------------------------------------------------------
DWORD FASTCALL OPMIF::ReadByte(DWORD addr)
{
	DWORD data;

	ASSERT(this);
	ASSERT((addr >= memdev.first) && (addr <= memdev.last));
	ASSERT_DIAG();

	// ��A�h���X�̂݃f�R�[�h����Ă���
	if ((addr & 1) != 0) {
		// 4�o�C�g�P�ʂŃ��[�v
		addr &= 3;

		// �E�F�C�g
		scheduler->Wait(1);

		if (addr != 0x01) {
			// �f�[�^�|�[�g��BUSY�ƃ^�C�}�̏��
			data = 0;

			// BUSY(1�񂾂�)
			if (opm.busy) {
				data |= 0x80;
				opm.busy = FALSE;
			}

			// �^�C�}
			if (opm.interrupt[0]) {
				data |= 0x01;
			}
			if (opm.interrupt[1]) {
				data |= 0x02;
			}

			return data;
		}

		// �A�h���X�|�[�g��FF
		return 0xff;
	}

	return 0xff;
}

//---------------------------------------------------------------------------
//
//	���[�h��������
//
//---------------------------------------------------------------------------
DWORD FASTCALL OPMIF::ReadWord(DWORD addr)
{
	ASSERT(this);
	ASSERT((addr >= memdev.first) && (addr <= memdev.last));
	ASSERT((addr & 1) == 0);
	ASSERT_DIAG();

	return (WORD)(0xff00 | ReadByte(addr + 1));
}

//---------------------------------------------------------------------------
//
//	�o�C�g��������
//
//---------------------------------------------------------------------------
void FASTCALL OPMIF::WriteByte(DWORD addr, DWORD data)
{
	ASSERT(this);
	ASSERT((addr >= memdev.first) && (addr <= memdev.last));
	ASSERT(data < 0x100);
	ASSERT_DIAG();

	// ��A�h���X�̂݃f�R�[�h����Ă���
	if ((addr & 1) != 0) {
		// 4�o�C�g�P�ʂŃ��[�v
		addr &= 3;

		// �E�F�C�g
		scheduler->Wait(1);

		if (addr == 0x01) {
			// �A�h���X�w��(BUSY�Ɋւ�炸�w��ł��邱�Ƃɂ���)
			opm.addr = data;

			// BUSY���~�낷�B�A�h���X�w���̑҂��͔������Ȃ����Ƃɂ���
			opm.busy = FALSE;

			return;
		}
		else {
			// �f�[�^��������(BUSY�Ɋւ�炸�������߂邱�Ƃɂ���)
			Output(opm.addr, data);

			// BUSY���グ��
			opm.busy = TRUE;
			return;
		}
	}
}

//---------------------------------------------------------------------------
//
//	���[�h��������
//
//---------------------------------------------------------------------------
void FASTCALL OPMIF::WriteWord(DWORD addr, DWORD data)
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
DWORD FASTCALL OPMIF::ReadOnly(DWORD addr) const
{
	DWORD data;

	ASSERT(this);
	ASSERT((addr >= memdev.first) && (addr <= memdev.last));
	ASSERT_DIAG();

	// ��A�h���X�̂݃f�R�[�h����Ă���
	if ((addr & 1) != 0) {
		// 4�o�C�g�P�ʂŃ��[�v
		addr &= 3;

		if (addr != 0x01) {
			// �f�[�^�|�[�g��BUSY�ƃ^�C�}�̏��
			data = 0;

			// BUSY
			if (opm.busy) {
				data |= 0x80;
			}

			// �^�C�}
			if (opm.interrupt[0]) {
				data |= 0x01;
			}
			if (opm.interrupt[1]) {
				data |= 0x02;
			}

			return data;
		}

		// �A�h���X�|�[�g��FF
		return 0xff;
	}

	return 0xff;
}

//---------------------------------------------------------------------------
//
//	�����f�[�^�擾
//
//---------------------------------------------------------------------------
void FASTCALL OPMIF::GetOPM(opm_t *buffer)
{
	ASSERT(this);
	ASSERT(buffer);
	ASSERT_DIAG();

	// �����f�[�^���R�s�[
	*buffer = opm;
}

//---------------------------------------------------------------------------
//
//	�C�x���g�R�[���o�b�N
//
//---------------------------------------------------------------------------
BOOL FASTCALL OPMIF::Callback(Event *ev)
{
	int index;

	ASSERT(this);
	ASSERT(ev);
	ASSERT_DIAG();

	// ���[�U�f�[�^��M
	index = ev->GetUser();
	ASSERT((index >= 0) && (index <= 1));

#if defined(OPM_LOG)
	LOG2(Log::Normal, "�^�C�}%c �R�[���o�b�N Time %d", index + 'A', ev->GetTime());
#endif	// OPM_LOG

	// �C�l�[�u�������쒆�Ȃ�A���荞�݂��N����
	if (opm.enable[index] && opm.action[index]) {
		opm.action[index] = FALSE;
		opm.interrupt[index] = TRUE;
#if defined(OPM_LOG)
		LOG2(Log::Normal, "�^�C�}%c �I�[�o�[�t���[���荞�� Time %d", index + 'A', ev->GetTime());
#endif	// OPM_LOG
		mfp->SetGPIP(3, 0);
	}

	// ���Ԃ�����Ă���΁A�Đݒ�
	if (ev->GetTime() != opm.time[index]) {
		ev->SetTime(opm.time[index]);
#if defined(OPM_LOG)
		LOG2(Log::Normal, "�^�C�}%c �C�x���g���X�^�[�g Time %d", index + 'A', ev->GetTime());
#endif	// OPM_LOG
	}

	// �^�C�}A��CSM�̏ꍇ������
	if ((index == 0) && engine) {
		if (opm.reg[0x14] & 0x80) {
			ProcessBuf();
			engine->TimerA();
		}
	}

	// �^�C�}�͉񂵑�����
	return TRUE;
}

//---------------------------------------------------------------------------
//
//	�G���W���w��
//
//---------------------------------------------------------------------------
void FASTCALL OPMIF::SetEngine(FM::OPM *p)
{
	int i;

	ASSERT(this);
	ASSERT_DIAG();

	// NULL���w�肳���ꍇ������
	engine = p;

	// ADPCM�֒ʒm
	if (engine) {
		adpcm->Enable(TRUE);
	}
	else {
		adpcm->Enable(FALSE);
	}

	// engine�w�肠��Ȃ�AOPM���W�X�^���o��
	if (!engine) {
		return;
	}
	ProcessBuf();

	// ���W�X�^����:�m�C�Y�ALFO�APMD�AAMD�ACSM
	engine->SetReg(0x0f, opm.reg[0x0f]);
	engine->SetReg(0x14, opm.reg[0x14] & 0x80);
	engine->SetReg(0x18, opm.reg[0x18]);
	engine->SetReg(0x19, opm.reg[0x19]);

	// ���W�X�^����:���W�X�^
	for (i=0x20; i<0x100; i++) {
		engine->SetReg(i, opm.reg[i]);
	}

	// ���W�X�^����:�L�[
	for (i=0; i<8; i++) {
		engine->SetReg(8, opm.key[i]);
	}

	// ���W�X�^����:LFO
	engine->SetReg(1, 2);
	engine->SetReg(1, 0);
}

//---------------------------------------------------------------------------
//
//	���W�X�^�o��
//
//---------------------------------------------------------------------------
void FASTCALL OPMIF::Output(DWORD addr, DWORD data)
{
	ASSERT(this);
	ASSERT(addr < 0x100);
	ASSERT(data < 0x100);
	ASSERT_DIAG();

	// ���ꃌ�W�X�^�̏���
	switch (addr) {
		// �^�C�}A
		case 0x10:
		case 0x11:
			opm.reg[addr] = data;
			CalcTimerA();
			return;

		// �^�C�}B
		case 0x12:
			opm.reg[addr] = data;
			CalcTimerB();
			return;

		// �^�C�}����
		case 0x14:
			CtrlTimer(data);
			data &= 0x80;
			break;

		// �ėp�|�[�g�t��
		case 0x1b:
			CtrlCT(data);
			opm.reg[addr] = data;
			data &= 0x3f;
			break;

		// �L�[
		case 0x08:
			opm.key[data & 0x07] = data;
			opm.reg[addr] = data;
			break;

		// ���̑�
		default:
			opm.reg[addr] = data;
			break;
	}

	// �G���W�����w�肳��Ă���΁A�o�b�t�@�����O���ďo��
	if (engine) {
		if ((addr < 0x10) || (addr > 0x14)) {
			// �^�C�}�֘A�ł̓o�b�t�@�����O���Ȃ�
			ProcessBuf();

			// �L�[�I�t�ȊO�̓X�^�[�g�t���OUp
			if ((addr != 0x08) || (data >= 0x08)) {
				opm.started = TRUE;
			}
		}
		engine->SetReg(addr, data);
	}
}

//---------------------------------------------------------------------------
//
//	�^�C�}A�Z�o
//
//---------------------------------------------------------------------------
void FASTCALL OPMIF::CalcTimerA()
{
	DWORD hus;
	DWORD low;

	ASSERT(this);
	ASSERT_DIAG();

	// hus�P�ʂŌv�Z
	hus = opm.reg[0x10];
	hus <<= 2;
	low = opm.reg[0x11] & 3;
	hus |= low;
	hus = (0x400 - hus);
	hus <<= 5;
	opm.time[0] = hus;
#if defined(OPM_LOG)
	LOG1(Log::Normal, "�^�C�}A = %d", hus);
#endif	// OPM_LOG

	// �~�܂��Ă���΃X�^�[�g(YST)
	if (event[0].GetTime() == 0) {
		event[0].SetTime(hus);
#if defined(OPM_LOG)
		LOG1(Log::Normal, "�^�C�}A �X�^�[�g Time %d", event[0].GetTime());
#endif	// OPM_LOG
	}
}

//---------------------------------------------------------------------------
//
//	�^�C�}B�Z�o
//
//---------------------------------------------------------------------------
void FASTCALL OPMIF::CalcTimerB()
{
	DWORD hus;

	ASSERT(this);
	ASSERT_DIAG();

	// hus�P�ʂŌv�Z
	hus = opm.reg[0x12];
	hus = (0x100 - hus);
	hus <<= 9;
	opm.time[1] = hus;
#if defined(OPM_LOG)
	LOG1(Log::Normal, "�^�C�}B = %d", hus);
#endif	// OPM_LOG

	// �~�܂��Ă���΃X�^�[�g(YST)
	if (event[1].GetTime() == 0) {
		event[1].SetTime(hus);
#if defined(OPM_LOG)
		LOG1(Log::Normal, "�^�C�}B �X�^�[�g Time %d", event[1].GetTime());
#endif	// OPM_LOG
	}
}

//---------------------------------------------------------------------------
//
//	�^�C�}����
//
//---------------------------------------------------------------------------
void FASTCALL OPMIF::CtrlTimer(DWORD data)
{
	ASSERT(this);
	ASSERT(data < 0x100);
	ASSERT_DIAG();

	// �I�[�o�[�t���[�t���O�̃N���A
	if (data & 0x10) {
		opm.interrupt[0] = FALSE;
	}
	if (data & 0x20) {
		opm.interrupt[1] = FALSE;
	}

	// ������������A���荞�݂𗎂Ƃ�
	if (!opm.interrupt[0] && !opm.interrupt[1]) {
		mfp->SetGPIP(3, 1);
	}

	// �^�C�}A�A�N�V��������
	if (data & 0x04) {
		opm.action[0] = TRUE;
	}
	else {
		opm.action[0] = FALSE;
	}

	// �^�C�}A�C�l�[�u������
	if (data & 0x01) {
		// 0��1�Ń^�C�}�l�����[�h�A����ȊO�ł��^�C�}ON
		if (!opm.enable[0]) {
			CalcTimerA();
		}
		opm.enable[0] = TRUE;
	}
	else {
		// (�}���n�b�^���E���N�C�G��)
		event[0].SetTime(0);
		opm.enable[0] = FALSE;
	}

	// �^�C�}B�A�N�V��������
	if (data & 0x08) {
		opm.action[1] = TRUE;
	}
	else {
		opm.action[1] = FALSE;
	}

	// �^�C�}B�C�l�[�u������
	if (data & 0x02) {
		// 0��1�Ń^�C�}�l�����[�h�A����ȊO�ł��^�C�}ON
		if (!opm.enable[1]) {
			CalcTimerB();
		}
		opm.enable[1] = TRUE;
	}
	else {
		// (�}���n�b�^���E���N�C�G��)
		event[1].SetTime(0);
		opm.enable[1] = FALSE;
	}

	// �f�[�^���L��
	opm.reg[0x14] = data;

#if defined(OPM_LOG)
	LOG1(Log::Normal, "�^�C�}���� $%02X", data);
#endif	// OPM_LOG
}

//---------------------------------------------------------------------------
//
//	CT����
//
//---------------------------------------------------------------------------
void FASTCALL OPMIF::CtrlCT(DWORD data)
{
	DWORD ct;

	ASSERT(this);
	ASSERT_DIAG();

	// CT�|�[�g�̂ݎ��o��
	data &= 0xc0;

	// �ȑO�̃f�[�^�𓾂�
	ct = opm.reg[0x1b];
	ct &= 0xc0;

	// ��v���Ă���Ή������Ȃ�
	if (data == ct) {
		return;
	}

	// �`�F�b�N(ADPCM)
	if ((data & 0x80) != (ct & 0x80)) {
		if (data & 0x80) {
			adpcm->SetClock(4);
		}
		else {
			adpcm->SetClock(8);
		}
	}

	// �`�F�b�N(FDD)
	if ((data & 0x40) != (ct & 0x40)) {
		if (data & 0x40) {
			fdd->ForceReady(TRUE);
		}
		else {
			fdd->ForceReady(FALSE);
		}
	}
}

//---------------------------------------------------------------------------
//
//	�o�b�t�@���e�𓾂�
//
//---------------------------------------------------------------------------
void FASTCALL OPMIF::GetBufInfo(opmbuf_t *buf)
{
	ASSERT(this);
	ASSERT(buf);
	ASSERT_DIAG();

	*buf = bufinfo;
}

//---------------------------------------------------------------------------
//
//	�o�b�t�@������
//
//---------------------------------------------------------------------------
void FASTCALL OPMIF::InitBuf(DWORD rate)
{
	ASSERT(this);
	ASSERT(rate > 0);
	ASSERT((rate % 100) == 0);
	ASSERT_DIAG();

	// ADPCM�ɐ�ɒʒm
	adpcm->InitBuf(rate);

	// �J�E���^�A�|�C���^
	bufinfo.num = 0;
	bufinfo.read = 0;
	bufinfo.write = 0;
	bufinfo.under = 0;
	bufinfo.over = 0;

	// �T�E���h���ԂƁA�A�g����T���v����
	scheduler->SetSoundTime(0);
	bufinfo.samples = 0;

	// �������[�g
	bufinfo.rate = rate / 100;
}

//---------------------------------------------------------------------------
//
//	�o�b�t�@�����O
//
//---------------------------------------------------------------------------
DWORD FASTCALL OPMIF::ProcessBuf()
{
	DWORD stime;
	DWORD sample;
	DWORD first;
	DWORD second;

	ASSERT(this);
	ASSERT_DIAG();

	// �G���W�����Ȃ���΃��^�[��
	if (!engine) {
		return bufinfo.num;
	}

	// �T�E���h���Ԃ���A�K�؂ȃT���v�����𓾂�
	sample = scheduler->GetSoundTime();
	stime = sample;

	sample *= bufinfo.rate;
	sample /= 20000;

	// bufmax�ɐ���
	if (sample > BufMax) {
		// �I�[�o�[�������Ă���̂ŁA���Z�b�g
		scheduler->SetSoundTime(0);
		bufinfo.samples = 0;
		return bufinfo.num;
	}

	// ����ƈ�v���Ă���΃��^�[��
	if (sample <= bufinfo.samples) {
		// �V���N��������
		while (stime >= 40000) {
			stime -= 20000;
			scheduler->SetSoundTime(stime);
			bufinfo.samples -= bufinfo.rate;
		}
		return bufinfo.num;
	}

	// ����ƈႤ����������������
	sample -= bufinfo.samples;

	// 1��܂���2��̂ǂ��炩�`�F�b�N
	first = sample;
	if ((first + bufinfo.write) > BufMax) {
		first = BufMax - bufinfo.write;
	}
	second = sample - first;

	// 1���
	memset(&opmbuf[bufinfo.write * 2], 0, first * 8);
	if (bufinfo.sound) {
		engine->Mix((int32*)&opmbuf[bufinfo.write * 2], first);
	}
	bufinfo.write += first;
	bufinfo.write &= (BufMax - 1);
	bufinfo.num += first;
	if (bufinfo.num > BufMax) {
		bufinfo.num = BufMax;
		bufinfo.read = bufinfo.write;
	}

	// 2���
	if (second > 0) {
		memset(opmbuf, 0, second * 8);
		if (bufinfo.sound) {
			engine->Mix((int32*)opmbuf, second);
		}
		bufinfo.write += second;
		bufinfo.write &= (BufMax - 1);
		bufinfo.num += second;
		if (bufinfo.num > BufMax) {
			bufinfo.num = BufMax;
			bufinfo.read = bufinfo.write;
		}
	}

	// �����ς݃T���v�����։��Z���A20000hus���ƂɃV���N��������
	bufinfo.samples += sample;
	while (stime >= 40000) {
		stime -= 20000;
		scheduler->SetSoundTime(stime);
		bufinfo.samples -= bufinfo.rate;
	}

	return bufinfo.num;
}

//---------------------------------------------------------------------------
//
//	�o�b�t�@����擾
//
//---------------------------------------------------------------------------
void FASTCALL OPMIF::GetBuf(DWORD *buf, int samples)
{
	DWORD first;
	DWORD second;
	DWORD under;
	DWORD over;

	ASSERT(this);
	ASSERT(buf);
	ASSERT(samples > 0);
	ASSERT(engine);
	ASSERT_DIAG();

	// �I�[�o�[�����`�F�b�N����
	over = 0;
	if (bufinfo.num > (DWORD)samples) {
		over = bufinfo.num - samples;
	}

	// ����A2��ځA�A���_�[�����̗v���������߂�
	first = samples;
	second = 0;
	under = 0;
	if (bufinfo.num < first) {
		first = bufinfo.num;
		under = samples - first;
		samples = bufinfo.num;
	}
	if ((first + bufinfo.read) > BufMax) {
		first = BufMax - bufinfo.read;
		second = samples - first;
	}

	// ����ǂݎ��
	memcpy(buf, &opmbuf[bufinfo.read * 2], (first * 8));
	buf += (first * 2);
	bufinfo.read += first;
	bufinfo.read &= (BufMax - 1);
	bufinfo.num -= first;

	// 2��ړǂݎ��
	if (second > 0) {
		memcpy(buf, &opmbuf[bufinfo.read * 2], (second * 8));
		bufinfo.read += second;
		bufinfo.read &= (BufMax - 1);
		bufinfo.num -= second;
	}

	// �A���_�[����
	if (under > 0) {
		// ����1/4�����A����ɍ��������悤�d�g��
		bufinfo.samples = 0;
		under *= 5000;
		under /= bufinfo.rate;
		scheduler->SetSoundTime(under);

		// �L�^
		bufinfo.under++;
	}

	// �I�[�o�[����
	if (over > 0) {
		// ����1/4�����A����x�点��悤�d�g��
		over *= 5000;
		over /= bufinfo.rate;
		under = scheduler->GetSoundTime();
		if (under < over) {
			scheduler->SetSoundTime(0);
		}
		else {
			under -= over;
			scheduler->SetSoundTime(under);
		}

		// �L�^
		bufinfo.over++;
	}
}
