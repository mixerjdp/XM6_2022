//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 �o�h�D(ytanaka@ipc-tokai.or.jp)
//	[ CPU(MC68000) ]
//
//---------------------------------------------------------------------------

#include "os.h"
#include "xm6.h"
#include "iosc.h"
#include "mfp.h"
#include "vm.h"
#include "log.h"
#include "memory.h"
#include "dmac.h"
#include "scc.h"
#include "midi.h"
#include "scsi.h"
#include "fileio.h"
#include "cpu.h"

//---------------------------------------------------------------------------
//
//	�A�Z���u���R�A�Ƃ̃C���^�t�F�[�X
//
//---------------------------------------------------------------------------
#if defined(__cplusplus)
extern "C" {
#endif	// __cplusplus

//---------------------------------------------------------------------------
//
//	�X�^�e�B�b�N ���[�N
//
//---------------------------------------------------------------------------
static CPU *cpu;
										// CPU

//---------------------------------------------------------------------------
//
//	�O����`
//
//---------------------------------------------------------------------------
DWORD s68000fbpc(void);
										// PC�t�B�[�h�o�b�N
void s68000buserr(DWORD addr, DWORD param);
										// �o�X�G���[

//---------------------------------------------------------------------------
//
//	RESET���߃n���h��
//
//---------------------------------------------------------------------------
static void cpu_resethandler(void)
{
	cpu->ResetInst();
}

//---------------------------------------------------------------------------
//
//	���荞��ACK
//
//---------------------------------------------------------------------------
void s68000intack(void)
{
	int sr;

	sr = ::s68000context.sr;
	sr >>= 8;
	sr &= 0x0007;

	cpu->IntAck(sr);
}

//---------------------------------------------------------------------------
//
//	�o�X�G���[�L�^
//
//---------------------------------------------------------------------------
void s68000buserrlog(DWORD addr, DWORD stat)
{
	cpu->BusErrLog(addr, stat);
}

//---------------------------------------------------------------------------
//
//	�A�h���X�G���[�L�^
//
//---------------------------------------------------------------------------
void s68000addrerrlog(DWORD addr, DWORD stat)
{
	cpu->AddrErrLog(addr, stat);
}

#if defined(__cplusplus)
}
#endif	// __cplusplus

//===========================================================================
//
//	CPU
//
//===========================================================================
//#define CPU_LOG

//---------------------------------------------------------------------------
//
//	�R���X�g���N�^
//
//---------------------------------------------------------------------------
CPU::CPU(VM *p) : Device(p)
{
	// �f�o�C�XID��������
	dev.id = MAKEID('C', 'P', 'U', ' ');
	dev.desc = "MPU (MC68000)";

	// �|�C���^������
	memory = NULL;
	dmac = NULL;
	mfp = NULL;
	iosc = NULL;
	scc = NULL;
	midi = NULL;
	scsi = NULL;
	scheduler = NULL;
}

//---------------------------------------------------------------------------
//
//	������
//
//---------------------------------------------------------------------------
BOOL FASTCALL CPU::Init()
{
	ASSERT(this);

	// ��{�N���X
	if (!Device::Init()) {
		return FALSE;
	}

	// CPU�L��
	::cpu = this;

	// �������擾
	memory = (Memory*)vm->SearchDevice(MAKEID('M', 'E', 'M', ' '));
	ASSERT(memory);

	// DMAC�擾
	dmac = (DMAC*)vm->SearchDevice(MAKEID('D', 'M', 'A', 'C'));
	ASSERT(dmac);

	// MFP�擾
	mfp = (MFP*)vm->SearchDevice(MAKEID('M', 'F', 'P', ' '));
	ASSERT(mfp);

	// IOSC�擾
	iosc = (IOSC*)vm->SearchDevice(MAKEID('I', 'O', 'S', 'C'));
	ASSERT(iosc);

	// SCC�擾
	scc = (SCC*)vm->SearchDevice(MAKEID('S', 'C', 'C', ' '));
	ASSERT(scc);

	// MIDI�擾
	midi = (MIDI*)vm->SearchDevice(MAKEID('M', 'I', 'D', 'I'));
	ASSERT(midi);

	// SCSI�擾
	scsi = (SCSI*)vm->SearchDevice(MAKEID('S', 'C', 'S', 'I'));
	ASSERT(scsi);

	// �X�P�W���[���擾
	scheduler = (Scheduler*)vm->SearchDevice(MAKEID('S', 'C', 'H', 'E'));
	ASSERT(scheduler);

	// CPU�R�A�̃W�����v�e�[�u�����쐬
	::s68000init();

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	�N���[���A�b�v
//
//---------------------------------------------------------------------------
void FASTCALL CPU::Cleanup()
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
void FASTCALL CPU::Reset()
{
	int i;
	S68000CONTEXT context;
	DWORD bit;

	ASSERT(this);
	LOG0(Log::Normal, "���Z�b�g");

	// �G���[�A�h���X�A�G���[���ԃN���A
	sub.erraddr = 0;
	sub.errtime = 0;

	// ���荞�݃J�E���g�N���A
	for (i=0; i<8; i++) {
		sub.intreq[i] = 0;
		sub.intack[i] = 0;
	}

	// �������R���e�L�X�g�쐬(���Z�b�g��p)
	memory->MakeContext(TRUE);

	// ���Z�b�g
	::s68000reset();
	::s68000context.resethandler = cpu_resethandler;
	::s68000context.odometer = 0;

	// ���荞�݂����ׂĎ�����
	::s68000GetContext(&context);
	for (i=1; i<=7; i++) {
		bit = (1 << i);
		if (context.interrupts[0] & bit) {
			context.interrupts[0] &= (BYTE)(~bit);
			context.interrupts[i] = 0;
		}
	}
	::s68000SetContext(&context);

	// �������R���e�L�X�g�쐬(�ʏ�)
	memory->MakeContext(FALSE);
}

//---------------------------------------------------------------------------
//
//	�Z�[�u
//
//---------------------------------------------------------------------------
BOOL FASTCALL CPU::Save(Fileio *fio, int /*ver*/)
{
	size_t sz;
	cpu_t cpu;

	ASSERT(this);
	ASSERT(fio);

	LOG0(Log::Normal, "�Z�[�u");

	// �R���e�L�X�g�擾
	GetCPU(&cpu);

	// �T�C�Y���Z�[�u
	sz = sizeof(cpu_t);
	if (!fio->Write(&sz, sizeof(sz))) {
		return FALSE;
	}

	// ���̂��Z�[�u
	if (!fio->Write(&cpu, (int)sz)) {
		return FALSE;
	}

	// �T�C�Y���Z�[�u(�T�u)
	sz = sizeof(cpusub_t);
	if (!fio->Write(&sz, sizeof(sz))) {
		return FALSE;
	}

	// ���̂��Z�[�u(�T�u)
	if (!fio->Write(&sub, (int)sz)) {
		return FALSE;
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	���[�h
//
//---------------------------------------------------------------------------
BOOL FASTCALL CPU::Load(Fileio *fio, int /*ver*/)
{
	cpu_t cpu;
	size_t sz;

	ASSERT(this);
	ASSERT(fio);

	LOG0(Log::Normal, "���[�h");

	// �T�C�Y�����[�h�A�ƍ�
	if (!fio->Read(&sz, sizeof(sz))) {
		return FALSE;
	}
	if (sz != sizeof(cpu_t)) {
		return FALSE;
	}

	// ���̂����[�h
	if (!fio->Read(&cpu, (int)sz)) {
		return FALSE;
	}

	// �K�p(���Z�b�g���Ă���s��)
	memory->MakeContext(TRUE);
	::s68000reset();
	memory->MakeContext(FALSE);
	SetCPU(&cpu);

	// �T�C�Y�����[�h�A�ƍ�(�T�u)
	if (!fio->Read(&sz, sizeof(sz))) {
		return FALSE;
	}
	if (sz != sizeof(cpusub_t)) {
		return FALSE;
	}

	// ���̂����[�h(�T�u)
	if (!fio->Read(&sub, (int)sz)) {
		return FALSE;
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	�ݒ�K�p
//
//---------------------------------------------------------------------------
void FASTCALL CPU::ApplyCfg(const Config* /*config*/)
{
	ASSERT(this);

	LOG0(Log::Normal, "�ݒ�K�p");
}

//---------------------------------------------------------------------------
//
//	CPU���W�X�^�擾
//
//---------------------------------------------------------------------------
void FASTCALL CPU::GetCPU(cpu_t *buffer) const
{
	int i;

	ASSERT(this);
	ASSERT(buffer);

	// Dreg, Areg
	for (i=0; i<8; i++) {
		buffer->dreg[i] = ::s68000context.dreg[i];
		buffer->areg[i] = ::s68000context.areg[i];
	}

	// ���荞��
	for (i=0; i<8; i++) {
		buffer->intr[i] = (DWORD)::s68000context.interrupts[i];
		buffer->intreq[i] = sub.intreq[i];
		buffer->intack[i] = sub.intack[i];
	}

	// ���̑�
	buffer->sp = ::s68000context.asp;
	buffer->pc = ::s68000context.pc;
	buffer->sr = (DWORD)::s68000context.sr;
	buffer->odd = ::s68000context.odometer;
}

//---------------------------------------------------------------------------
//
//	CPU���W�X�^�ݒ�
//
//---------------------------------------------------------------------------
void FASTCALL CPU::SetCPU(const cpu_t *buffer)
{
	int i;
	S68000CONTEXT context;

	ASSERT(this);
	ASSERT(buffer);

	// �R���e�L�X�g�擾
	::s68000GetContext(&context);

	// Dreg, Areg
	for (i=0; i<8; i++) {
		context.dreg[i] = buffer->dreg[i];
		context.areg[i] = buffer->areg[i];
	}

	// ���荞��
	for (i=0; i<8; i++) {
		context.interrupts[i] = (BYTE)buffer->intr[i];
		sub.intreq[i] = buffer->intreq[i];
		sub.intack[i] = buffer->intack[i];
	}

	// ���̑�
	context.asp = buffer->sp;
	context.pc = buffer->pc;
	context.sr = (WORD)buffer->sr;
	context.odometer = buffer->odd;

	// �R���e�L�X�g�ݒ�
	::s68000SetContext(&context);
}

//---------------------------------------------------------------------------
//
//	���荞��
//
//---------------------------------------------------------------------------
BOOL FASTCALL CPU::Interrupt(int level, int vector)
{
	int ret;

	// INTERRUPT SWITCH�ɂ��NMI���荞�݂̓x�N�^-1
	ASSERT(this);
	ASSERT((level >= 1) && (level <= 7));
	ASSERT(vector >= -1);

	// ���N�G�X�g
	ret = ::s68000interrupt(level, vector);

	// ���ʕ]��
	if (ret == 0) {
#if defined(CPU_LOG)
		LOG2(Log::Normal, "���荞�ݗv���� ���x��%d �x�N�^$%02X", level, vector);
#endif	// CPU_LOG
		sub.intreq[level]++;
		return TRUE;
	}

	return FALSE;
}

//---------------------------------------------------------------------------
//
//	���荞��ACK
//
//---------------------------------------------------------------------------
void FASTCALL CPU::IntAck(int level)
{
	ASSERT(this);
	ASSERT((level >= 1) && (level <= 7));

#if defined(CPU_LOG)
	LOG1(Log::Normal, "���荞�ݗv��ACK ���x��%d", level);
#endif	// CPU_LOG

	// �J�E���g�A�b�v
	sub.intack[level]++;

	// ���荞�݃��x����
	switch (level) {
		// IOSC,SCSI(����)
		case 1:
			iosc->IntAck();
			scsi->IntAck(1);
			break;

		// MIDI,SCSI(���x��2)
		case 2:
			midi->IntAck(2);
			scsi->IntAck(2);
			break;

		// DMAC
		case 3:
			dmac->IntAck();
			break;

		// MIDI,SCSI(���x��4)
		case 4:
			midi->IntAck(4);
			scsi->IntAck(4);
			break;

		// SCC
		case 5:
			scc->IntAck();
			break;

		// MFP
		case 6:
			mfp->IntAck();
			break;

		// ���̑�
		default:
			break;
	}
}

//---------------------------------------------------------------------------
//
//	���荞�݃L�����Z��
//
//---------------------------------------------------------------------------
void FASTCALL CPU::IntCancel(int level)
{
	S68000CONTEXT context;
	DWORD bit;

	ASSERT(this);
	ASSERT((level >= 1) && (level <= 7));

	// �R���e�L�X�g�𒼐ڏ���������
	::s68000GetContext(&context);

	// �Y���r�b�g���I���Ȃ�
	bit = (1 << level);
	if (context.interrupts[0] & bit) {
#if defined(CPU_LOG)
		LOG1(Log::Normal, "���荞�݃L�����Z�� ���x��%d", level);
#endif	// CPU_LOG

		// �r�b�g���~�낷
		context.interrupts[0] &= (BYTE)(~bit);

		// �x�N�^��0
		context.interrupts[level] = 0;

		// ���N�G�X�g��������
		sub.intreq[level]--;
	}

	// �R���e�L�X�g����������
	::s68000SetContext(&context);
}

//---------------------------------------------------------------------------
//
//	RESET����
//
//---------------------------------------------------------------------------
void FASTCALL CPU::ResetInst()
{
	Device *device;

	ASSERT(this);
	LOG0(Log::Detail, "RESET����");

	// ���������擾
	device = (Device*)vm->SearchDevice(MAKEID('M', 'E', 'M', ' '));
	ASSERT(device);

	// �������f�o�C�X�ɑ΂��Ă��ׂă��Z�b�g�������Ă���
	// ���m�ɂ́ACPU��RESET�M�����ǂ��܂œ`����Ă��邩�ɂ��
	while (device) {
		device->Reset();
		device = device->GetNextDevice();
	}
}

//---------------------------------------------------------------------------
//
//	�o�X�G���[
//	��DMA�]���ɂ��o�X�G���[�������ɗ���
//	��CPU�R�A�����Ńo�X�G���[�Ɣ��肵���ꍇ�́A�������o�R���Ȃ�
//
//---------------------------------------------------------------------------
void FASTCALL CPU::BusErr(DWORD addr, BOOL read)
{
	DWORD pc;
	DWORD stat;

	ASSERT(this);
	ASSERT(addr <= 0xffffff);

	// DMAC�ɓ]�����������BDMAC���Ȃ�DMAC�ɔC����
	if (dmac->IsDMA()) {
		dmac->BusErr(addr, read);
		return;
	}

	// �A�h���X���O��̃A�h���X+2�ŁA�����Ԃ������Ȃ疳������(LONG�A�N�Z�X)
	if (addr == (sub.erraddr + 2)) {
		if (scheduler->GetTotalTime() == sub.errtime) {
			return;
		}
	}

	// �A�h���X�Ǝ��Ԃ��X�V
	sub.erraddr = addr;
	sub.errtime = scheduler->GetTotalTime();

	// PC�擾(�Y�����߂̃I�y�R�[�h�Ɉʒu����)
	pc = GetPC();

	// �ǂݏo��(Word)
	stat = memory->ReadOnly(pc);
	stat <<= 8;
	stat |= memory->ReadOnly(pc + 1);
	stat <<= 16;

	// �t�@���N�V�����R�[�h�쐬(��Ƀf�[�^�A�N�Z�X�Ƃ݂Ȃ�)
	stat |= 0x09;
	if (::s68000context.sr & 0x2000) {
		stat |= 0x04;
	}
	if (read) {
		stat |= 0x10;
	}

	// �o�X�G���[���s
	::s68000buserr(addr, stat);
}

//---------------------------------------------------------------------------
//
//	�A�h���X�G���[
//	��DMA�]���ɂ��A�h���X�G���[�������ɗ���
//	��CPU�R�A�����ŃA�h���X�G���[�Ɣ��肵���ꍇ�́A�������o�R���Ȃ�
//
//---------------------------------------------------------------------------
void FASTCALL CPU::AddrErr(DWORD addr, BOOL read)
{
	DWORD pc;
	DWORD stat;

	ASSERT(this);
	ASSERT(addr <= 0xffffff);
	ASSERT(addr & 1);

	// DMAC�ɓ]�����������BDMAC���Ȃ�DMAC�ɔC����
	if (dmac->IsDMA()) {
		dmac->AddrErr(addr, read);
		return;
	}

	// �A�h���X���O��̃A�h���X+2�ŁA�����Ԃ������Ȃ疳������(LONG�A�N�Z�X)
	if (addr == (sub.erraddr + 2)) {
		if (scheduler->GetTotalTime() == sub.errtime) {
			return;
		}
	}

	// �A�h���X�Ǝ��Ԃ��X�V
	sub.erraddr = addr;
	sub.errtime = scheduler->GetTotalTime();

	// PC�擾(�Y�����߂̃I�y�R�[�h�Ɉʒu����)
	pc = GetPC();

	// �ǂݏo��(Word)
	stat = memory->ReadOnly(pc);
	stat <<= 8;
	stat |= memory->ReadOnly(pc + 1);
	stat <<= 16;

	// �t�@���N�V�����R�[�h�쐬(��Ƀf�[�^�A�N�Z�X�Ƃ݂Ȃ�)
	stat |= 0x8009;
	if (::s68000context.sr & 0x2000) {
		stat |= 0x04;
	}
	if (read) {
		stat |= 0x10;
	}

	// �o�X�G���[���s(�����ŃA�h���X�G���[�֕���)
	::s68000buserr(addr, stat);
}

//---------------------------------------------------------------------------
//
//	�o�X�G���[�L�^
//	��CPU�R�A�����Ńo�X�G���[�Ɣ��肵���ꍇ���A������ʂ�
//
//---------------------------------------------------------------------------
void FASTCALL CPU::BusErrLog(DWORD addr, DWORD stat)
{
	ASSERT(this);

	// �K���}�X�N(24bit�𒴂���ꍇ������)
	addr &= 0xffffff;

	if (stat & 0x10) {
		LOG1(Log::Warning, "�o�X�G���[(�ǂݍ���) $%06X", addr);
	}
	else {
		LOG1(Log::Warning, "�o�X�G���[(��������) $%06X", addr);
	}
}

//---------------------------------------------------------------------------
//
//	�A�h���X�G���[�L�^
//	��CPU�R�A�����ŃA�h���X�G���[�Ɣ��肵���ꍇ���A������ʂ�
//
//---------------------------------------------------------------------------
void FASTCALL CPU::AddrErrLog(DWORD addr, DWORD stat)
{
	ASSERT(this);

	// �K���}�X�N(24bit�𒴂���ꍇ������)
	addr &= 0xffffff;

	if (stat & 0x10) {
		LOG1(Log::Warning, "�A�h���X�G���[(�ǂݍ���) $%06X", addr);
	}
	else {
		LOG1(Log::Warning, "�A�h���X�G���[(��������) $%06X", addr);
	}
}
