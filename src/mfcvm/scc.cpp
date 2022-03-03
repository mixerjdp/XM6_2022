//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 �o�h�D(ytanaka@ipc-tokai.or.jp)
//	[ SCC(Z8530) ]
//
//---------------------------------------------------------------------------

#include "os.h"
#include "xm6.h"
#include "vm.h"
#include "cpu.h"
#include "mouse.h"
#include "log.h"
#include "schedule.h"
#include "config.h"
#include "fileio.h"
#include "scc.h"

//===========================================================================
//
//	SCC
//
//===========================================================================
//#define SCC_LOG

//---------------------------------------------------------------------------
//
//	�R���X�g���N�^
//
//---------------------------------------------------------------------------
SCC::SCC(VM *p) : MemDevice(p)
{
	// �f�o�C�XID��������
	dev.id = MAKEID('S', 'C', 'C', ' ');
	dev.desc = "SCC (Z8530)";

	// �J�n�A�h���X�A�I���A�h���X
	memdev.first = 0xe98000;
	memdev.last = 0xe99fff;
}

//---------------------------------------------------------------------------
//
//	������
//
//---------------------------------------------------------------------------
BOOL FASTCALL SCC::Init()
{
	ASSERT(this);

	// ��{�N���X
	if (!MemDevice::Init()) {
		return FALSE;
	}

	// ���[�N�N���A(�n�[�h�E�F�A���Z�b�g�ł��ꕔ��Ԃ͎c��)
	memset(&scc, 0, sizeof(scc));
	scc.ch[0].index = 0;
	scc.ch[1].index = 1;
	scc.ireq = -1;
	scc.vector = -1;
	clkup = FALSE;

	// �}�E�X�擾
	mouse = (Mouse*)vm->SearchDevice(MAKEID('M', 'O', 'U', 'S'));
	ASSERT(mouse);

	// �C�x���g�ǉ�
	event[0].SetDevice(this);
	event[0].SetDesc("Channel-A");
	event[0].SetUser(0);
	event[0].SetTime(0);
	scheduler->AddEvent(&event[0]);
	event[1].SetDevice(this);
	event[1].SetDesc("Channel-B");
	event[1].SetUser(1);
	event[1].SetTime(0);
	scheduler->AddEvent(&event[1]);

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	�N���[���A�b�v
//
//---------------------------------------------------------------------------
void FASTCALL SCC::Cleanup()
{
	// ��{�N���X��
	MemDevice::Cleanup();
}

//---------------------------------------------------------------------------
//
//	���Z�b�g
//
//---------------------------------------------------------------------------
void FASTCALL SCC::Reset()
{
	int i;

	ASSERT(this);
	LOG0(Log::Normal, "���Z�b�g");

	// RESET�M���ɂ�郊�Z�b�g(�`���l�����Z�b�g�Ƃ͕�)
	ResetSCC(&scc.ch[0]);
	ResetSCC(&scc.ch[1]);
	ResetSCC(NULL);

	// ���x
	for (i=0; i<2; i++ ){
		scc.ch[i].brgen = FALSE;
		scc.ch[i].speed = 0;
		event[i].SetTime(0);
	}
}

//---------------------------------------------------------------------------
//
//	�Z�[�u
//
//---------------------------------------------------------------------------
BOOL FASTCALL SCC::Save(Fileio *fio, int ver)
{
	size_t sz;

	ASSERT(this);
	LOG0(Log::Normal, "�Z�[�u");

	// �T�C�Y���Z�[�u
	sz = sizeof(scc_t);
	if (!fio->Write(&sz, sizeof(sz))) {
		return FALSE;
	}
	if (!fio->Write(&scc, sizeof(scc))) {
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
BOOL FASTCALL SCC::Load(Fileio *fio, int ver)
{
	size_t sz;

	ASSERT(this);
	LOG0(Log::Normal, "���[�h");

	// �{�̂����[�h
	if (!fio->Read(&sz, sizeof(sz))) {
		return FALSE;
	}
	if (sz != sizeof(scc_t)) {
		return FALSE;
	}
	if (!fio->Read(&scc, sizeof(scc))) {
		return FALSE;
	}

	// �C�x���g
	if (!event[0].Load(fio, ver)) {
		return FALSE;
	}
	if (!event[1].Load(fio, ver)) {
		return FALSE;
	}

	// ���x�Čv�Z
	ClockSCC(&scc.ch[0]);
	ClockSCC(&scc.ch[1]);

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	�ݒ�K�p
//
//---------------------------------------------------------------------------
void FASTCALL SCC::ApplyCfg(const Config *config)
{
	ASSERT(this);
	ASSERT(config);
	LOG0(Log::Normal, "�ݒ�K�p");

	// SCC�N���b�N
	if (clkup != config->scc_clkup) {
		// ����Ă���̂Őݒ肵��
		clkup = config->scc_clkup;

		// ���x�Čv�Z
		ClockSCC(&scc.ch[0]);
		ClockSCC(&scc.ch[1]);
	}
}

//---------------------------------------------------------------------------
//
//	�o�C�g�ǂݍ���
//
//---------------------------------------------------------------------------
DWORD FASTCALL SCC::ReadByte(DWORD addr)
{
	static const DWORD table[] = {
		0, 1, 2, 3, 0, 1, 2, 3, 8, 13, 10, 15, 12, 13, 10, 15
	};
	DWORD reg;

	ASSERT(this);
	ASSERT((addr >= memdev.first) && (addr <= memdev.last));

	// ��A�h���X�̂݃f�R�[�h����Ă���
	if ((addr & 1) != 0) {
		// 8�o�C�g�P�ʂŃ��[�v
		addr &= 7;
		addr >>= 1;

		// �E�F�C�g
		scheduler->Wait(3);

		// ���W�X�^�U�蕪��
		switch (addr) {
			// �`���l��B�R�}���h�|�[�g
			case 0:
				// ���W�X�^����
				ASSERT(scc.ch[1].reg <= 7);
				if (scc.ch[1].ph) {
					reg = table[scc.ch[1].reg + 8];
				}
				else {
					reg = table[scc.ch[1].reg];
				}

				// �Z���N�g���n�C�|�C���g
				scc.ch[1].reg = 0;
				scc.ch[1].ph = FALSE;

				// �f�[�^���[�h
				return (BYTE)ReadSCC(&scc.ch[1], reg);

			// �`���l��B�f�[�^�|�[�g
			case 1:
				return (BYTE)ReadRR8(&scc.ch[1]);

			// �`���l��A�R�}���h�|�[�g
			case 2:
				// ���W�X�^����
				ASSERT(scc.ch[0].reg <= 7);
				if (scc.ch[0].ph) {
					reg = table[scc.ch[0].reg + 8];
				}
				else {
					reg = table[scc.ch[0].reg];
				}

				// �Z���N�g���n�C�|�C���g
				scc.ch[0].reg = 0;
				scc.ch[0].ph = FALSE;

				// �f�[�^���[�h
				return (BYTE)ReadSCC(&scc.ch[0], reg);

			// �`���l��A�f�[�^�|�[�g
			case 3:
				return (BYTE)ReadRR8(&scc.ch[0]);
		}

		// �����ɂ͗��Ȃ�
		ASSERT(FALSE);
		return 0xff;
	}

	// �����A�h���X��$FF��Ԃ�
	return 0xff;
}

//---------------------------------------------------------------------------
//
//	���[�h�ǂݍ���
//
//---------------------------------------------------------------------------
DWORD FASTCALL SCC::ReadWord(DWORD addr)
{
	ASSERT(this);
	ASSERT((addr >= memdev.first) && (addr <= memdev.last));
	ASSERT((addr & 1) == 0);

	return (WORD)(0xff00 | ReadByte(addr + 1));
}

//---------------------------------------------------------------------------
//
//	�o�C�g��������
//
//---------------------------------------------------------------------------
void FASTCALL SCC::WriteByte(DWORD addr, DWORD data)
{
	DWORD reg;

	ASSERT(this);
	ASSERT((addr >= memdev.first) && (addr <= memdev.last));

	// ��A�h���X�̂݃f�R�[�h����Ă���
	if ((addr & 1) != 0) {
		// 8�o�C�g�P�ʂŃ��[�v
		addr &= 7;
		addr >>= 1;

		// �E�F�C�g
		scheduler->Wait(3);

		// ���W�X�^�U�蕪��
		switch (addr) {
			// �`���l��B�R�}���h�|�[�g
			case 0:
				// ���W�X�^����
				ASSERT(scc.ch[1].reg <= 7);
				reg = scc.ch[1].reg;
				if (scc.ch[1].ph) {
					reg += 8;
				}

				// �Z���N�g���n�C�|�C���g
				scc.ch[1].reg = 0;
				scc.ch[1].ph = FALSE;

				// �f�[�^���C�g
				WriteSCC(&scc.ch[1], reg, (DWORD)data);
				return;

			// �`���l��B�f�[�^�|�[�g
			case 1:
				WriteWR8(&scc.ch[1], (DWORD)data);
				return;

			// �`���l��A�R�}���h�|�[�g
			case 2:
				// ���W�X�^����
				ASSERT(scc.ch[0].reg <= 7);
				reg = scc.ch[0].reg;
				if (scc.ch[0].ph) {
					reg += 8;
				}

				// �Z���N�g���n�C�|�C���g
				scc.ch[0].reg = 0;
				scc.ch[0].ph = FALSE;

				// �f�[�^���C�g
				WriteSCC(&scc.ch[0], reg, (DWORD)data);
				return;

			// �`���l��A�f�[�^�|�[�g
			case 3:
				WriteWR8(&scc.ch[0], (DWORD)data);
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
void FASTCALL SCC::WriteWord(DWORD addr, DWORD data)
{
	ASSERT(this);
	ASSERT((addr >= memdev.first) && (addr <= memdev.last));
	ASSERT((addr & 1) == 0);

	WriteByte(addr + 1, (BYTE)data);
}

//---------------------------------------------------------------------------
//
//	�ǂݍ��݂̂�
//
//---------------------------------------------------------------------------
DWORD FASTCALL SCC::ReadOnly(DWORD addr) const
{
	static const DWORD table[] = {
		0, 1, 2, 3, 0, 1, 2, 3, 8, 13, 10, 15, 12, 13, 10, 15
	};
	DWORD reg;

	ASSERT(this);
	ASSERT((addr >= memdev.first) && (addr <= memdev.last));

	// ��A�h���X�̂݃f�R�[�h����Ă���
	if ((addr & 1) != 0) {
		// 8�o�C�g�P�ʂŃ��[�v
		addr &= 7;
		addr >>= 1;

		// ���W�X�^�U�蕪��
		switch (addr) {
			// �`���l��B�R�}���h�|�[�g
			case 0:
				// ���W�X�^����
				ASSERT(scc.ch[1].reg <= 7);
				if (scc.ch[1].ph) {
					reg = table[scc.ch[1].reg + 8];
				}
				else {
					reg = table[scc.ch[1].reg];
				}

				// �f�[�^���[�h
				return (BYTE)ROSCC(&scc.ch[1], reg);

			// �`���l��B�f�[�^�|�[�g
			case 1:
				return (BYTE)ROSCC(&scc.ch[1], 8);

			// �`���l��A�R�}���h�|�[�g
			case 2:
				// ���W�X�^����
				ASSERT(scc.ch[0].reg <= 7);
				if (scc.ch[0].ph) {
					reg = table[scc.ch[0].reg + 8];
				}
				else {
					reg = table[scc.ch[0].reg];
				}

				// �f�[�^���[�h
				return (BYTE)ROSCC(&scc.ch[0], reg);

			// �`���l��A�f�[�^�|�[�g
			case 3:
				return (BYTE)ROSCC(&scc.ch[0], 8);
		}

		// �����ɂ͗��Ȃ�
		ASSERT(FALSE);
		return 0xff;
	}

	// �����A�h���X��$FF��Ԃ�
	return 0xff;
}

//---------------------------------------------------------------------------
//
//	�����f�[�^�擾
//
//---------------------------------------------------------------------------
void FASTCALL SCC::GetSCC(scc_t *buffer) const
{
	ASSERT(this);
	ASSERT(buffer);

	// �������[�N���R�s�[
	*buffer = scc;
}

//---------------------------------------------------------------------------
//
//	�������[�N�擾
//
//---------------------------------------------------------------------------
const SCC::scc_t* FASTCALL SCC::GetWork() const
{
	ASSERT(this);

	// �������[�N��Ԃ�
	return (const scc_t*)&scc;
}

//---------------------------------------------------------------------------
//
//	�x�N�^�擾
//
//---------------------------------------------------------------------------
DWORD FASTCALL SCC::GetVector(int type) const
{
	DWORD vector;

	ASSERT(this);
	ASSERT((type >= 0) && (type < 8));

	// NV=1�Ȃ�A0�Œ�
	if (scc.nv) {
		return 0;
	}

	// ��x�N�^��ݒ�
	vector = scc.vbase;

	// bit3-1���Abit6-4��
	if (scc.vis) {
		if (scc.shsl) {
			// bit6-4
			vector &= 0x8f;
			vector |= (type << 4);
		}
		else {
			// bit3-1
			vector &= 0xf1;
			vector |= (type << 1);
		}
	}

	return vector;
}

//---------------------------------------------------------------------------
//
//	�`���l�����Z�b�g
//
//---------------------------------------------------------------------------
void FASTCALL SCC::ResetSCC(ch_t *p)
{
	int i;

	ASSERT(this);

	// NULL��^����ƃn�[�h�E�F�A���Z�b�g
	if (p == NULL) {
#if defined(SCC_LOG)
		LOG0(Log::Normal, "�n�[�h�E�F�A���Z�b�g");
#endif	// SCC_LOG

		// WR9
		scc.shsl = FALSE;
		scc.mie = FALSE;
		scc.dlc = FALSE;

		// WR11
		return;
	}

#if defined(SCC_LOG)
	LOG1(Log::Normal, "�`���l��%d ���Z�b�g", p->index);
#endif	// SCC_LOG

	// RR0
	p->tu = TRUE;
	p->zc = FALSE;

	// WR0
	p->reg = 0;
	p->ph = FALSE;
	p->rxno = TRUE;
	p->txpend = FALSE;

	// RR1
	p->framing = FALSE;
	p->overrun = FALSE;
	p->parerr = FALSE;

	// WR1
	p->rxim = 0;
	p->txie = FALSE;
	p->extie = FALSE;

	// RR3
	for (i=0; i<2; i++) {
		scc.ch[i].rsip = FALSE;
		scc.ch[i].rxip = FALSE;
		scc.ch[i].txip = FALSE;
		scc.ch[i].extip = FALSE;
	}

	// WR3
	p->rxen = FALSE;

	// WR4
	p->stopbit |= 0x01;

	// WR5
	p->dtr = FALSE;
	p->brk = FALSE;
	p->txen = FALSE;
	p->rts = FALSE;
	if (p->index == 1) {
		mouse->MSCtrl(!p->rts, 1);
	}

	// WR14
	p->loopback = FALSE;
	p->aecho = FALSE;
	p->dtrreq = FALSE;

	// WR15
	p->baie = TRUE;
	p->tuie = TRUE;
	p->ctsie = TRUE;
	p->syncie = TRUE;
	p->dcdie = TRUE;
	p->zcie = FALSE;

	// ��MFIFO
	p->rxfifo = 0;

	// ��M�o�b�t�@
	p->rxnum = 0;
	p->rxread = 0;
	p->rxwrite = 0;
	p->rxtotal = 0;

	// ���M�o�b�t�@
	p->txnum = 0;
	p->txread = 0;
	p->txwrite = 0;
	p->tdf = FALSE;
	p->txtotal = 0;
	p->txwait = FALSE;

	// ���荞��
	IntCheck();
}

//---------------------------------------------------------------------------
//
//	�`���l���ǂݏo��
//
//---------------------------------------------------------------------------
DWORD FASTCALL SCC::ReadSCC(ch_t *p, DWORD reg)
{
	ASSERT(this);
	ASSERT(p);
	ASSERT((p->index == 0) || (p->index == 1));


#if defined(SCC_LOG)
	LOG2(Log::Normal, "�`���l��%d�ǂݏo�� R%d", p->index, reg);
#endif	// SCC_LOG

	switch (reg) {
		// RR0 - �g���X�e�[�^�X
		case 0:
			return ReadRR0(p);

		// RR1 - �X�y�V����Rx�R���f�B�V����
		case 1:
			return ReadRR1(p);

		// RR2 - �x�N�^
		case 2:
			return ReadRR2(p);

		// RR3 - ���荞�݃y���f�B���O
		case 3:
			return ReadRR3(p);

		// RR8 - ��M�f�[�^
		case 8:
			return ReadRR8(p);

		// RR10 - SDLC���[�v���[�h
		case 10:
			return 0;

		// RR12 - �{�[���[�g����
		case 12:
			return p->tc;

		// RR13 - �{�[���[�g���
		case 13:
			return (p->tc >> 8);

		// RR15 - �O���X�e�[�^�X���荞�ݐ���
		case 15:
			return ReadRR15(p);
	}

#if defined(SCC_LOG)
	LOG1(Log::Normal, "���������W�X�^�ǂݏo�� R%d", reg);
#endif	// SCC_LOG

	return 0xff;
}

//---------------------------------------------------------------------------
//
//	RR0�ǂݏo��
//
//---------------------------------------------------------------------------
DWORD FASTCALL SCC::ReadRR0(const ch_t *p) const
{
	DWORD data;

	ASSERT(this);
	ASSERT(p);
	ASSERT((p->index == 0) || (p->index == 1));

	// ������
	data = 0;

	// bit7 : Break/Abort�X�e�[�^�X
	if (p->ba) {
		data |= 0x80;
	}

	// bit6 : ���M�A���_�[�����X�e�[�^�X
	if (p->tu) {
		data |= 0x40;
	}

	// bit5 : CTS�X�e�[�^�X
	if (p->cts) {
		data |= 0x20;
	}

	// bit5 : Sync/Hunt�X�e�[�^�X
	if (p->sync) {
		data |= 0x10;
	}

	// bit3 : DCD�X�e�[�^�X
	if (p->dcd) {
		data |= 0x08;
	}

	// bit2 : ���M�o�b�t�@�G���v�e�B�X�e�[�^�X
	if (!p->txwait) {
		// ���M�E�F�C�g���͏�Ƀo�b�t�@�t��
		if (!p->tdf) {
			data |= 0x04;
		}
	}

	// bit1 : �[���J�E���g�X�e�[�^�X
	if (p->zc) {
		data |= 0x02;
	}

	// bit0 : ��M�o�b�t�@�L���X�e�[�^�X
	if (p->rxfifo > 0) {
		data |= 0x01;
	}

	return data;
}

//---------------------------------------------------------------------------
//
//	RR1�ǂݏo��
//
//---------------------------------------------------------------------------
DWORD FASTCALL SCC::ReadRR1(const ch_t *p) const
{
	DWORD data;

	ASSERT(this);
	ASSERT(p);
	ASSERT((p->index == 0) || (p->index == 1));

	// ������
	data = 0x06;

	// bit6 : �t���[�~���O�G���[
	if (p->framing) {
		data |= 0x40;
	}

	// bit5 : �I�[�o�[�����G���[
	if (p->overrun) {
		data |= 0x20;
	}

	// bit4 : �p���e�B�G���[
	if (p->parerr) {
		data |= 0x10;
	}

	// bit0 : ���M����
	if (p->txsent) {
		data |= 0x01;
	}

	return data;
}

//---------------------------------------------------------------------------
//
//	RR2�ǂݏo��
//
//---------------------------------------------------------------------------
DWORD FASTCALL SCC::ReadRR2(ch_t *p)
{
	DWORD data;

	ASSERT(this);
	ASSERT(p);
	ASSERT((p->index == 0) || (p->index == 1));

	// �`���l������
	if (p->index == 0) {
		// �`���l��A��WR2�̓��e�����̂܂ܕԂ�
		data = scc.vbase;
	}
	else {
		// �`���l��B�͌��ݗv�����Ă��銄�荞�݃x�N�^
		data = scc.request;
	}

	// ���荞�ݗv�����łȂ���Ή������Ȃ�
	if (scc.ireq < 0) {
		return data;
	}

	// �����ŁA���̊��荞�݂Ɉڂ�(Mac�G�~�����[�^)
	ASSERT((scc.ireq >= 0) && (scc.ireq <= 7));
	if (scc.ireq >= 4) {
		p = &scc.ch[1];
	}
	else {
		p = &scc.ch[0];
	}
	switch (scc.ireq & 3) {
		// Rs
		case 0:
			p->rsip = FALSE;
			break;
		// Rx
		case 1:
			p->rxip = FALSE;
			break;
		// Ext
		case 2:
			p->extip = FALSE;
			break;
		// Tx
		case 3:
			p->txip = FALSE;
			break;
	}
	scc.ireq = -1;

	// ���荞�݃`�F�b�N
	IntCheck();

	// �f�[�^�Ԃ�
	return data;
}

//---------------------------------------------------------------------------
//
//	RR3�ǂݏo��
//
//---------------------------------------------------------------------------
DWORD FASTCALL SCC::ReadRR3(const ch_t *p) const
{
	int i;
	DWORD data;

	ASSERT(this);
	ASSERT(p);
	ASSERT((p->index == 0) || (p->index == 1));

	// �`���l��A�̂ݗL���B�`���l��B��0��Ԃ�
	if (p->index == 1) {
		return 0;
	}

	// �N���A
	data = 0;

	// bit0����ext,tx,rx�ŁA�`���l��B���`���l��A�̏�
	for (i=0; i<2; i++) {
		data <<= 3;

		// �L����(�v��������)���荞�݃r�b�g�����ׂė��Ă�
		if (scc.ch[i].extip) {
			data |= 0x01;
		}
		if (scc.ch[i].txip) {
			data |= 0x02;
		}
		if (scc.ch[i].rxip) {
			data |= 0x04;
		}
		if (scc.ch[i].rsip) {
			data |= 0x04;
		}
	}

	return data;
}

//---------------------------------------------------------------------------
//
//	RR8�ǂݏo��
//
//---------------------------------------------------------------------------
DWORD FASTCALL SCC::ReadRR8(ch_t *p)
{
	DWORD data;

	ASSERT(this);
	ASSERT(p);
	ASSERT((p->index == 0) || (p->index == 1));

	// ��M�f�[�^�����邩
	if (p->rxfifo == 0) {
		if (p->index == 0) {
			LOG1(Log::Warning, "�`���l��%d ��M�f�[�^��ǂ�", p->index);
		}
		return 0;
	}

	// �K���Ō���Ƀf�[�^������
#if defined(SCC_LOG)
	LOG2(Log::Normal, "�`���l��%d �f�[�^�������$%02X", p->index, p->rxdata[2]);
#endif
	data = p->rxdata[2];

	// ��MFIFO�ړ�
	p->rxdata[2] = p->rxdata[1];
	p->rxdata[1] = p->rxdata[0];

	// ����������
	p->rxfifo--;

	// ��U���荞�݂𗎂Ƃ��BFIFO�̎c�肪����Ύ��̃C�x���g��
	IntSCC(p, rxi, FALSE);

#if defined(SCC_LOG)
	LOG2(Log::Normal, "�c��FIFO=%d �c��RxNum=%d", p->rxfifo, p->rxnum);
#endif

	return data;
}

//---------------------------------------------------------------------------
//
//	RR15�ǂݏo��
//
//---------------------------------------------------------------------------
DWORD FASTCALL SCC::ReadRR15(const ch_t *p) const
{
	DWORD data;

	ASSERT(this);
	ASSERT(p);
	ASSERT((p->index == 0) || (p->index == 1));

	// �f�[�^������
	data = 0;

	// bit7 - Break/Abort���荞��
	if (p->baie) {
		data |= 0x80;
	}

	// bit6 - Tx�A���_�[�������荞��
	if (p->tuie) {
		data |= 0x40;
	}

	// bit5 - CTS���荞��
	if (p->ctsie) {
		data |= 0x20;
	}

	// bit4 - SYNC���荞��
	if (p->syncie) {
		data |= 0x10;
	}

	// bit3 - DCD���荞��
	if (p->dcdie) {
		data |= 0x08;
	}

	// bit1 - �[���J�E���g���荞��
	if (p->zcie) {
		data |= 0x02;
	}

	return data;
}

//---------------------------------------------------------------------------
//
//	�ǂݏo���̂�
//
//---------------------------------------------------------------------------
DWORD FASTCALL SCC::ROSCC(const ch_t *p, DWORD reg) const
{
	DWORD data;

	ASSERT(this);
	ASSERT(p);
	ASSERT((p->index == 0) || (p->index == 1));

	switch (reg) {
		// RR0 - �g���X�e�[�^�X
		case 0:
			return ReadRR0(p);

		// RR1 - �X�y�V����Rx�R���f�B�V����
		case 1:
			return ReadRR1(p);

		// RR2 - �x�N�^
		case 2:
			if (p->index == 0) {
				return scc.vbase;
			}
			return scc.request;

		// RR3 - ���荞�݃y���f�B���O
		case 3:
			return ReadRR3(p);

		// RR8 - ��M�f�[�^
		case 8:
			break;

		// RR10 - SDLC���[�v���[�h
		case 10:
			return 0;

		// RR12 - �{�[���[�g����
		case 12:
			return p->tc;

		// RR13 - �{�[���[�g���
		case 13:
			return (p->tc >> 8);

		// RR15 - �O���X�e�[�^�X���荞�ݐ���
		case 15:
			return ReadRR15(p);
	}

	// �f�[�^������
	data = 0xff;

	// ��MFIFO���L���Ȃ�A�Ԃ�
	if (p->rxfifo > 0) {
		data = p->rxdata[2];
	}

	return data;
}

//---------------------------------------------------------------------------
//
//	�`���l����������
//
//---------------------------------------------------------------------------
void FASTCALL SCC::WriteSCC(ch_t *p, DWORD reg, DWORD data)
{
	ASSERT(this);
	ASSERT(p);
	ASSERT((p->index == 0) || (p->index == 1));
	ASSERT(data < 0x100);

#if defined(SCC_LOG)
	LOG3(Log::Normal, "�`���l��%d�������� R%d <- $%02X", p->index, reg, data);
#endif	// SCC_LOG

	switch (reg) {
		// WR0 - �S�̃R���g���[��
		case 0:
			WriteWR0(p, data);
			return;

		// WR1 - ���荞�݋���
		case 1:
			WriteWR1(p, data);
			return;

		// WR2 - �x�N�^�x�[�X
		case 2:
			LOG1(Log::Detail, "���荞�݃x�N�^�x�[�X $%02X", data);
			scc.vbase = data;
			return;

		// WR3 - ��M�R���g���[��
		case 3:
			WriteWR3(p, data);
			return;

		// WR4 - �ʐM���[�h�ݒ�
		case 4:
			WriteWR4(p, data);
			return;

		// WR5 - ���M�R���g���[��
		case 5:
			WriteWR5(p, data);
			return;

		// WR8 - ���M�o�b�t�@
		case 8:
			WriteWR8(p, data);
			return;

		// WR9 - ���荞�݃R���g���[��
		case 9:
			WriteWR9(data);
			return;

		// WR10 - SDLC�R���g���[��
		case 10:
			WriteWR10(p, data);
			return;

		// WR11 - �N���b�N�E�[�q�I��
		case 11:
			WriteWR11(p, data);

		// WR12 - �{�[���[�g����
		case 12:
			WriteWR12(p, data);
			return;

		// WR13 - �{�[���[�g���
		case 13:
			WriteWR13(p, data);
			return;

		// WR14 - �N���b�N�R���g���[��
		case 14:
			WriteWR14(p, data);
			return;

		// WR15 - �g���X�e�[�^�X���荞�݃R���g���[��
		case 15:
			WriteWR15(p, data);
			return;
	}

	LOG3(Log::Warning, "�`���l��%d���������W�X�^�������� R%d <- $%02X",
								p->index, reg, data);
}

//---------------------------------------------------------------------------
//
//	WR0��������
//
//---------------------------------------------------------------------------
void FASTCALL SCC::WriteWR0(ch_t *p, DWORD data)
{
	ASSERT(this);
	ASSERT(p);
	ASSERT((p->index == 0) || (p->index == 1));
	ASSERT(data < 0x100);

	// bit2-0 : �A�N�Z�X���W�X�^
	p->reg = (data & 0x07);

	// bit5-3 : SCC����R�}���h
	data >>= 3;
	data &= 0x07;
	switch (data) {
		// 000:�k���R�[�h
		case 0:
			break;

		// 001:��ʃ��W�X�^�I��
		case 1:
			p->ph = TRUE;
			break;

		// 010:�O���X�e�[�^�X���Z�b�g
		case 2:
#if defined(SCC_LOG)
			LOG1(Log::Normal, "�`���l��%d �O���X�e�[�^�X���Z�b�g", p->index);
#endif	// SCC_LOG
			break;

		// 100:��M�f�[�^���Z�b�g
		case 4:
#if defined(SCC_LOG)
			LOG1(Log::Normal, "�`���l��%d ��M�f�[�^���Z�b�g", p->index);
#endif	// SCC_LOG
			p->rxno = TRUE;
			break;

		// 101:���M���荞�݃y���f�B���O���Z�b�g
		case 5:
#if defined(SCC_LOG)
			LOG1(Log::Normal, "�`���l��%d ���M���荞�݃y���f�B���O���Z�b�g", p->index);
#endif	// SCC_LOG
			p->txpend = TRUE;
			IntSCC(p, txi, FALSE);

		// 110:��M�G���[���Z�b�g
		case 6:
#if defined(SCC_LOG)
			LOG1(Log::Normal, "�`���l��%d ��M�G���[���Z�b�g", p->index);
#endif	// SCC_LOG
			p->framing = FALSE;
			p->overrun = FALSE;
			p->parerr = FALSE;
			IntSCC(p, rsi, FALSE);
			break;

		// 111:�ō��ʃC���T�[�r�X���Z�b�g
		case 7:
			break;

		// ���̑�
		default:
			LOG2(Log::Warning, "�`���l��%d SCC���T�|�[�g�R�}���h $%02X",
								p->index, data);
			break;
	}
}

//---------------------------------------------------------------------------
//
//	WR1��������
//
//---------------------------------------------------------------------------
void FASTCALL SCC::WriteWR1(ch_t *p, DWORD data)
{
	ASSERT(this);
	ASSERT(p);
	ASSERT((p->index == 0) || (p->index == 1));
	ASSERT(data < 0x100);

	// bit4-3 : Rx���荞�݃��[�h
	p->rxim = (data >> 3) & 0x03;

	// bit2 : �p���e�B�G���[���X�y�V����Rx�R���f�B�V�������荞�݂�
	if (data & 0x04) {
		p->parsp = TRUE;
	}
	else {
		p->parsp = FALSE;
	}

	// bit1 : ���M���荞�݋���
	if (data & 0x02) {
		p->txie = TRUE;
	}
	else {
		p->txie = FALSE;
	}

	// bit0 : �O���X�e�[�^�X���荞�݋���
	if (data & 0x01) {
		p->extie = TRUE;
	}
	else {
		p->extie = FALSE;
	}
}

//---------------------------------------------------------------------------
//
//	WR3��������
//
//---------------------------------------------------------------------------
void FASTCALL SCC::WriteWR3(ch_t *p, DWORD data)
{
	ASSERT(this);
	ASSERT(p);
	ASSERT((p->index == 0) || (p->index == 1));
	ASSERT(data < 0x100);

	// bit7-6 : ��M�L�����N�^��
	p->rxbit = ((data & 0xc0) >> 6) + 5;
	ASSERT((p->rxbit >= 5) && (p->rxbit <= 8));

	// bit5 : �I�[�g�C�l�[�u��
	if (data & 0x20) {
		p->aen = TRUE;
	}
	else {
		p->aen = FALSE;
	}

	// bit0 : ��M�C�l�[�u��
	if (data & 0x01) {
		if (!p->rxen) {
			// �G���[��Ԃ��N���A
			p->framing = FALSE;
			p->overrun = FALSE;
			p->parerr = FALSE;
			IntSCC(p, rsi, FALSE);

			// ��M�o�b�t�@���N���A
			p->rxnum = 0;
			p->rxread = 0;
			p->rxwrite = 0;
			p->rxfifo = 0;

			// ��M�C�l�[�u��
			p->rxen = TRUE;
		}
	}
	else {
		p->rxen = FALSE;
	}

	// �{�[���[�g���Čv�Z
	ClockSCC(p);
}

//---------------------------------------------------------------------------
//
//	WR4��������
//
//---------------------------------------------------------------------------
void FASTCALL SCC::WriteWR4(ch_t *p, DWORD data)
{
	ASSERT(this);
	ASSERT(p);
	ASSERT((p->index == 0) || (p->index == 1));
	ASSERT(data < 0x100);

	// bit7-6 : �N���b�N���[�h
	switch ((data & 0xc0) >> 6) {
		// x1�N���b�N���[�h
		case 0:
			p->clkm = 1;
			break;
		// x16�N���b�N���[�h
		case 1:
			p->clkm = 16;
			break;
		// x32�N���b�N���[�h
		case 2:
			p->clkm = 32;
			break;
		// x64�N���b�N���[�h
		case 3:
			p->clkm = 64;
			break;
		// ���̑�(���肦�Ȃ�)
		default:
			ASSERT(FALSE);
			break;
	}

	// bit3-2 : �X�g�b�v�r�b�g
	p->stopbit = (data & 0x0c) >> 2;
	if (p->stopbit == 0) {
		LOG1(Log::Warning, "�`���l��%d �������[�h", p->index);
	}

	// bit1-0 : �p���e�B
	if (data & 1) {
		// 1:��p���e�B�A2:�����p���e�B
		p->parity = ((data & 2) >> 1) + 1;
	}
	else {
		// 0:�p���e�B�Ȃ�
		p->parity = 0;
	}

	// �{�[���[�g���Čv�Z
	ClockSCC(p);
}

//---------------------------------------------------------------------------
//
//	WR5��������
//
//---------------------------------------------------------------------------
void FASTCALL SCC::WriteWR5(ch_t *p, DWORD data)
{
	ASSERT(this);
	ASSERT(p);
	ASSERT((p->index == 0) || (p->index == 1));
	ASSERT(data < 0x100);

	// bit7 : DTR����(Low�A�N�e�B�u,p->dtrreq�����邱��)
	if (data & 0x80) {
		p->dtr = TRUE;
	}
	else {
		p->dtr = FALSE;
	}

	// bit6-5 : ���M�L�����N�^�r�b�g��
	p->txbit = ((data & 0x60) >> 5) + 5;
	ASSERT((p->txbit >= 5) && (p->txbit <= 8));

	// bit4 : �u���[�N�M�����o
	if (data & 0x10) {
#if defined(SCC_LOG)
		LOG1(Log::Normal, "�`���l��%d �u���[�N�M��ON", p->index);
#endif	// SCC_LOG
		p->brk = TRUE;
	}
	else {
#if defined(SCC_LOG)
		LOG1(Log::Normal, "�`���l��%d �u���[�N�M��OFF", p->index);
#endif	// SCC_LOG
		p->brk = FALSE;
	}

	// bit3 : ���M�C�l�[�u��
	if (data & 0x08) {
		if (!p->txen) {
			// ���M�o�b�t�@���N���A
			p->txnum = 0;
			if (!p->txpend) {
				IntSCC(p, txi, TRUE);
			}

			// ���M���[�N���N���A
			p->txsent = FALSE;
			p->tdf = FALSE;
			p->txwait = FALSE;

			// ���M�C�l�[�u��
			p->txen = TRUE;
		}
	}
	else {
		p->txen = FALSE;
	}

	// bit1 : RTS����
	if (data & 0x02) {
#if defined(SCC_LOG)
		LOG1(Log::Normal, "�`���l��%d RTS�M��ON", p->index);
#endif	// SCC_LOG
		p->rts = TRUE;
	}
	else {
#if defined(SCC_LOG)
		LOG1(Log::Normal, "�`���l��%d RTS�M��OFF", p->index);
#endif	// SCC_LOG
		p->rts = FALSE;
	}

	// �`���l��B�̏ꍇ�́A�}�E�X��MSCTRL��`����
	if (p->index == 1) {
		// RTS��Low�A�N�e�B�u�A���̐�ɃC���o�[�^�����Ă���(��H�}�Q��)
		mouse->MSCtrl(!(p->rts), 1);
	}
}

//---------------------------------------------------------------------------
//
//	WR8��������
//
//---------------------------------------------------------------------------
void FASTCALL SCC::WriteWR8(ch_t *p, DWORD data)
{
	ASSERT(this);
	ASSERT(p);
	ASSERT((p->index == 0) || (p->index == 1));
	ASSERT(data < 0x100);

	// ���M�f�[�^���L���A���M�f�[�^����
	p->tdr = data;
	p->tdf = TRUE;

	// ���M���荞�݂���艺��
	IntSCC(p, txi, FALSE);

	// ���M���荞�݃y���f�B���O�������I�t
	p->txpend = FALSE;
}

//---------------------------------------------------------------------------
//
//	WR9��������
//
//---------------------------------------------------------------------------
void FASTCALL SCC::WriteWR9(DWORD data)
{
	ASSERT(this);
	ASSERT(data < 0x100);

	// bit7-6 : ���Z�b�g
	switch ((data & 0xc0) >> 3) {
		case 1:
			ResetSCC(&scc.ch[0]);
			return;
		case 2:
			ResetSCC(&scc.ch[1]);
			return;
		case 3:
			ResetSCC(&scc.ch[0]);
			ResetSCC(&scc.ch[1]);
			ResetSCC(NULL);
			return;
		default:
			break;
	}

	// bit4 : ���荞�ݕω����[�h�I��
	if (data & 0x10) {
#if defined(SCC_LOG)
		LOG0(Log::Warning, "���荞�݃x�N�^�ω����[�h bit4-bit6");
#endif	// SCC_LOG
		scc.shsl = TRUE;
	}
	else {
#if defined(SCC_LOG)
		LOG0(Log::Normal, "���荞�݃x�N�^�ω����[�h bit3-bit1");
#endif	// SCC_LOG
		scc.shsl = FALSE;
	}

	// bit3 : ���荞�݃C�l�[�u��
	if (data & 0x08) {
#if defined(SCC_LOG)
		LOG0(Log::Normal, "���荞�݃C�l�[�u��");
#endif	// SCC_LOG
		scc.mie = TRUE;
	}
	else {
#if defined(SCC_LOG)
		LOG0(Log::Warning, "���荞�݃f�B�Z�[�u��");
#endif	// SCC_LOG
		scc.mie = FALSE;
	}

	// bit2 : �f�B�W�[�`�F�C���֎~
	if (data & 0x04) {
		scc.dlc = TRUE;
	}
	else {
		scc.dlc = FALSE;
	}

	// bit1 : ���荞�݃x�N�^�����Ȃ�
	if (data & 0x02) {
		scc.nv = TRUE;
#if defined(SCC_LOG)
		LOG0(Log::Warning, "���荞�݃A�N�m���b�W�Ȃ�");
#endif	// SCC_LOG
	}
	else {
#if defined(SCC_LOG)
		LOG0(Log::Normal, "���荞�݃A�N�m���b�W����");
#endif	// SCC_LOG
		scc.nv = FALSE;
	}

	// bit0 : ���荞�݃x�N�^�ϓ�
	if (data & 0x01) {
		scc.vis = TRUE;
#if defined(SCC_LOG)
		LOG0(Log::Normal, "���荞�݃x�N�^�ω�");
#endif	// SCC_LOG
	}
	else {
		scc.vis = FALSE;
#if defined(SCC_LOG)
		LOG0(Log::Warning, "���荞�݃x�N�^�Œ�");
#endif	// SCC_LOG
	}

	// ���荞�݃`�F�b�N
	IntCheck();
}

//---------------------------------------------------------------------------
//
//	WR10��������
//
//---------------------------------------------------------------------------
void FASTCALL SCC::WriteWR10(ch_t* /*p*/, DWORD data)
{
	ASSERT(this);
	ASSERT(data < 0x100);

	data &= 0x60;
	if (data != 0) {
		LOG0(Log::Warning, "NRZ�ȊO�̕ϒ����[�h");
	}
}

//---------------------------------------------------------------------------
//
//	WR11��������
//
//---------------------------------------------------------------------------
void FASTCALL SCC::WriteWR11(ch_t *p, DWORD data)
{
	ASSERT(this);
	ASSERT(p);
	ASSERT((p->index == 0) || (p->index == 1));
	ASSERT(data < 0x100);

	// bit7 : RTxC��������E�Ȃ�
	if (data & 0x80) {
		LOG1(Log::Warning, "�`���l��%d bRTxC��bSYNC�ŃN���b�N�쐬", p->index);
	}

	// bit6-5 : ��M�N���b�N���I��
	if ((data & 0x60) != 0x40) {
		LOG1(Log::Warning, "�`���l��%d �{�[���[�g�W�F�l���[�^�o�͂��g��Ȃ�", p->index);
	}
}

//---------------------------------------------------------------------------
//
//	WR12��������
//
//---------------------------------------------------------------------------
void FASTCALL SCC::WriteWR12(ch_t *p, DWORD data)
{
	ASSERT(this);
	ASSERT(p);
	ASSERT((p->index == 0) || (p->index == 1));
	ASSERT(data < 0x100);

	// ����8bit�̂�
	p->tc &= 0xff00;
	p->tc |= data;

	// �{�[���[�g���Čv�Z
	ClockSCC(p);
}

//---------------------------------------------------------------------------
//
//	WR13��������
//
//---------------------------------------------------------------------------
void FASTCALL SCC::WriteWR13(ch_t *p, DWORD data)
{
	ASSERT(this);
	ASSERT(p);
	ASSERT((p->index == 0) || (p->index == 1));
	ASSERT(data < 0x100);

	// ���8bit�̂�
	p->tc &= 0x00ff;
	p->tc |= (data << 8);

	// �{�[���[�g���Čv�Z
	ClockSCC(p);
}

//---------------------------------------------------------------------------
//
//	WR14��������
//
//---------------------------------------------------------------------------
void FASTCALL SCC::WriteWR14(ch_t *p, DWORD data)
{
	ASSERT(this);
	ASSERT(p);
	ASSERT((p->index == 0) || (p->index == 1));
	ASSERT(data < 0x100);

	// bit4 : ���[�J�����[�v�o�b�N
	if (data & 0x10) {
#if defined(SCC_LOG)
		LOG1(Log::Normal, "�`���l��%d ���[�J�����[�v�o�b�N���[�h", p->index);
#endif	// SCC_LOG
		p->loopback = TRUE;
	}
	else {
		p->loopback = FALSE;
	}

	// bit3 : �I�[�g�G�R�[
	if (data & 0x08) {
#if defined(SCC_LOG)
		LOG1(Log::Normal, "�`���l��%d �I�[�g�G�R�[���[�h", p->index);
#endif	// SCC_LOG
		p->aecho = TRUE;
	}
	else {
		p->aecho = FALSE;
	}

	// bit2 : DTR/REQ�Z���N�g
	if (data & 0x04) {
		p->dtrreq = TRUE;
	}
	else {
		p->dtrreq = FALSE;
	}

	// bit1 : �{�[���[�g�W�F�l���[�^�N���b�N������
	if (data & 0x02) {
		p->brgsrc = TRUE;
	}
	else {
		p->brgsrc = FALSE;
	}

	// bit0 : �{�[���[�g�W�F�l���[�^�C�l�[�u��
	if (data & 0x01) {
		p->brgen = TRUE;
	}
	else {
		p->brgen = FALSE;
	}

	// �{�[���[�g���Čv�Z
	ClockSCC(p);
}

//---------------------------------------------------------------------------
//
//	�{�[���[�g�Čv�Z
//
//---------------------------------------------------------------------------
void FASTCALL SCC::ClockSCC(ch_t *p)
{
	DWORD len;
	DWORD speed;

	ASSERT(this);
	ASSERT(p);
	ASSERT((p->index == 0) || (p->index == 1));

	// �{�[���[�g�W�F�l���[�^���L���łȂ���΁A�C�x���g��~
	if (!p->brgsrc || !p->brgen) {
		event[p->index].SetTime(0);
		p->speed = 0;
		return;
	}

	// ��N���b�N
	if (clkup) {
		p->baudrate = 3750000;
	}
	else {
		p->baudrate = 2500000;
	}

	// �{�[���[�g�ݒ�l�ƃN���b�N�{������A�{�[���[�g���Z�o
	speed = (p->tc + 2);
	speed *= p->clkm;
	p->baudrate /= speed;

	// �f�[�^�r�b�g���A�p���e�B�A�X�g�b�v�r�b�g����2�o�C�g�����O�X���Z�o
	len = p->rxbit + 1;
	if (p->parity != 0) {
		len++;
	}
	len <<= 1;
	switch (p->stopbit) {
		// �X�g�b�v�r�b�g�Ȃ�
		case 0:
			break;
		// �X�g�b�v�r�b�g1bit
		case 1:
			len += 2;
			break;
		// �X�g�b�v�r�b�g1.5bit
		case 2:
			len += 3;
		// �X�g�b�v�r�b�g2bit
		case 3:
			len += 4;
	}

	// CPS�ɐݒ肵�Ă���
	if (clkup) {
		p->cps = 3750000;
	}
	else {
		p->cps = 2500000;
	}
	p->cps /= ((len * speed) >> 1);

	// �ŏI���x�v�Z
	p->speed = 100;
	p->speed *= (len * speed);
	if (clkup) {
		p->speed /= 375;
	}
	else {
		p->speed /= 250;
	}

	// �ݒ�
	if (event[p->index].GetTime() == 0) {
		event[p->index].SetTime(p->speed);
	}
}

//---------------------------------------------------------------------------
//
//	WR15��������
//
//---------------------------------------------------------------------------
void FASTCALL SCC::WriteWR15(ch_t *p, DWORD data)
{
	ASSERT(this);
	ASSERT(p);
	ASSERT((p->index == 0) || (p->index == 1));
	ASSERT(data < 0x100);

	// bit7 - Break/Abort���荞��
	if (data & 0x80) {
		p->baie = TRUE;
	}
	else {
		p->baie = FALSE;
	}

	// bit6 - Tx�A���_�[�������荞��
	if (data & 0x40) {
		p->tuie = TRUE;
	}
	else {
		p->tuie = FALSE;
	}

	// bit5 - CTS���荞��
	if (data & 0x20) {
		p->ctsie = TRUE;
	}
	else {
		p->ctsie = FALSE;
	}

	// bit4 - SYNC���荞��
	if (data & 0x10) {
		p->syncie = TRUE;
	}
	else {
		p->syncie = FALSE;
	}

	// bit3 - DCD���荞��
	if (data & 0x08) {
		p->dcdie = TRUE;
	}
	else {
		p->dcdie = FALSE;
	}

	// bit1 - �[���J�E���g���荞��
	if (data & 0x02) {
		p->zcie = TRUE;
	}
	else {
		p->zcie = FALSE;
	}
}

//---------------------------------------------------------------------------
//
//	���荞�݃��N�G�X�g
//
//---------------------------------------------------------------------------
void FASTCALL SCC::IntSCC(ch_t *p, itype_t type, BOOL flag)
{
	ASSERT(this);
	ASSERT(p);
	ASSERT((p->index == 0) || (p->index == 1));

	// ���荞�ݗL���Ȃ�A�y���f�B���O�r�b�g���グ��
	switch (type) {
		// ��M���荞��
		case rxi:
			p->rxip = FALSE;
			if (flag) {
				if ((p->rxim == 1) || (p->rxim == 2)) {
					p->rxip = TRUE;
				}
			}
			break;

		// �X�y�V����Rx�R���f�B�V�������荞��
		case rsi:
			p->rsip = FALSE;
			if (flag) {
				if (p->rxim >= 2) {
					p->rsip = TRUE;
				}
			}
			break;

		// ���M���荞��
		case txi:
			p->txip = FALSE;
			if (flag) {
				if (p->txie) {
					p->txip = TRUE;
				}
			}
			break;

		// �O���X�e�[�^�X�ω����荞��
		case exti:
			p->extip = FALSE;
			if (flag) {
				if (p->extie) {
					p->extip = TRUE;
				}
			}
			break;

		// ���̑�(���肦�Ȃ�)
		default:
			ASSERT(FALSE);
			break;
	}

	// ���荞�݃`�F�b�N
	IntCheck();
}

//---------------------------------------------------------------------------
//
//	���荞�݃`�F�b�N
//
//---------------------------------------------------------------------------
void FASTCALL SCC::IntCheck()
{
	int i;
	DWORD v;

	ASSERT(this);

	// �v�����荞�݂��N���A
	scc.ireq = -1;

	// �}�X�^���荞�݋���
	if (scc.mie) {

		// �`���l��A���`���l��B�̏�
		for (i=0; i<2; i++) {
			// Rs,Rx,Tx,Ext�̏��Ńy���f�B���O�r�b�g������
			if (scc.ch[i].rsip) {
				scc.ireq = (i << 2) + 0;
				break;
			}
			if (scc.ch[i].rxip) {
				scc.ireq = (i << 2) + 1;
				break;
			}
			if (scc.ch[i].txip) {
				scc.ireq = (i << 2) + 3;
				break;
			}
			if (scc.ch[i].extip) {
				scc.ireq = (i << 2) + 2;
				break;
			}
		}

		// �^�C�v�����āA���荞�݃x�N�^�쐬
		if (scc.ireq >= 0) {
			// ��x�N�^���擾
			v = scc.vbase;

			// bit3-1���Abit6-4��
			if (scc.shsl) {
				// bit6-4
				v &= 0x8f;
				v |= ((7 - scc.ireq) << 4);
			}
			else {
				// bit3-1
				v &= 0xf1;
				v |= ((7 - scc.ireq) << 1);
			}

			// ���荞�݃x�N�^�L��
			scc.request = v;

			// RR2����ǂ߂�x�N�^��WR9��VIS�ɂ�炸��ɕω�������
			// �f�[�^�V�[�g�̋L�q�Ƃ͈قȂ�(Mac�G�~�����[�^)
			if (!scc.vis) {
				v = scc.vbase;
			}

			// NV(No Vector)���オ���Ă��Ȃ���Ί��荞�ݗv��
			if (!scc.nv) {
				// ���ɗv�����Ă����OK
				if (scc.vector == (int)v) {
					return;
				}

				// �ʂ̃x�N�^��v�����Ă���΁A��x�L�����Z��
				if (scc.vector >= 0) {
					cpu->IntCancel(5);
				}

				// ���荞�ݗv��
#if defined(SCC_LOG)
				LOG2(Log::Normal, "���荞�ݗv�� ���N�G�X�g$%02X �x�N�^$%02X ", scc.request, v);
#endif	// SCC_LOG
				cpu->Interrupt(5, (BYTE)v);
				scc.vector = (int)v;
			}
			return;
		}
	}

	// �v�����荞�݂Ȃ��̃x�N�^�𐶐�
	scc.request = scc.vbase;

	// bit3-1���Abit6-4��
	if (scc.shsl) {
		// bit6-4
		scc.request &= 0x8f;
		scc.request |= 0x60;
	}
	else {
		// bit3-1
		scc.request &= 0xf1;
		scc.request |= 0x06;
	}

	// ���ɗv�����Ă���Ί��荞�݃L�����Z��
	if (scc.vector >= 0) {
		cpu->IntCancel(5);
		scc.vector = -1;
	}
}

//---------------------------------------------------------------------------
//
//	���荞�݉���
//
//---------------------------------------------------------------------------
void FASTCALL SCC::IntAck()
{
	ASSERT(this);

	// ���Z�b�g����ɁACPU���犄�荞�݂��Ԉ���ē���ꍇ������
	if (scc.vector < 0) {
		LOG0(Log::Warning, "�v�����Ă��Ȃ����荞��");
		return;
	}

#if defined(SCC_LOG)
	LOG1(Log::Normal, "���荞�݉��� �x�N�^$%02X", scc.vector);
#endif

	// �v���x�N�^�Ȃ�
	scc.vector = -1;

	// �����ň�U���荞�݂���������Brxi���荞�݌p���̏ꍇ�͎���
	// �C�x���g��(�m�X�^���W�A1907�A������h���L����)
}

//---------------------------------------------------------------------------
//
//	�f�[�^���M
//
//---------------------------------------------------------------------------
void FASTCALL SCC::Send(int channel, DWORD data)
{
	ch_t *p;

	ASSERT(this);
	ASSERT((channel == 0) || (channel == 1));

	// BYTE�ɐ���(�}�E�X�����signed int�̕ϊ��ł��邽��)
	data &= 0xff;

#if defined(SCC_LOG)
	LOG2(Log::Normal, "�`���l��%d �f�[�^���M$%02X", channel, data);
#endif	// SCC_LOG

	// �|�C���^�ݒ�
	p = &scc.ch[channel];

	// ��M�o�b�t�@�֑}��
	p->rxbuf[p->rxwrite] = (BYTE)data;
	p->rxwrite = (p->rxwrite + 1) & (sizeof(p->rxbuf) - 1);
	p->rxnum++;
	if (p->rxnum >= sizeof(p->rxbuf)) {
		LOG1(Log::Warning, "�`���l��%d ��M�o�b�t�@�I�[�o�[�t���[", p->index);
		p->rxnum = sizeof(p->rxbuf);
		p->rxread = p->rxwrite;
	}
}

//----------------------------------------------------------------------------
//
//	�p���e�B�G���[
//
//---------------------------------------------------------------------------
void FASTCALL SCC::ParityErr(int channel)
{
	ch_t *p;

	ASSERT(this);
	ASSERT((channel == 0) || (channel == 1));

	// �|�C���^�ݒ�
	p = &scc.ch[channel];

	if (!p->parerr) {
		LOG1(Log::Normal, "�`���l��%d �p���e�B�G���[", p->index);
	}

	// �t���O�グ
	p->parerr = TRUE;

	// ���荞��
	if (p->parsp) {
		if (p->rxim >= 2) {
			IntSCC(p, rsi, TRUE);
		}
	}
}

//---------------------------------------------------------------------------
//
//	�t���[�~���O�G���[
//
//---------------------------------------------------------------------------
void FASTCALL SCC::FramingErr(int channel)
{
	ch_t *p;

	ASSERT(this);
	ASSERT((channel == 0) || (channel == 1));

	// �|�C���^�ݒ�
	p = &scc.ch[channel];

	if (!p->framing) {
		LOG1(Log::Normal, "�`���l��%d �t���[�~���O�G���[", p->index);
	}

	// �t���O�グ
	p->framing = TRUE;

	// ���荞��
	if (p->rxim >= 2) {
		IntSCC(p, rsi, TRUE);
	}
}

//---------------------------------------------------------------------------
//
//	��M�C�l�[�u����
//
//---------------------------------------------------------------------------
BOOL FASTCALL SCC::IsRxEnable(int channel) const
{
	const ch_t *p;

	ASSERT(this);
	ASSERT((channel == 0) || (channel == 1));

	// �|�C���^�ݒ�
	p = &scc.ch[channel];

	return p->rxen;
}

//---------------------------------------------------------------------------
//
//	��M�o�b�t�@���󂢂Ă��邩
//
//---------------------------------------------------------------------------
BOOL FASTCALL SCC::IsRxBufEmpty(int channel) const
{
	const ch_t *p;

	ASSERT(this);
	ASSERT((channel == 0) || (channel == 1));

	// �|�C���^�ݒ�
	p = &scc.ch[channel];

	if (p->rxnum == 0) {
		return TRUE;
	}
	return FALSE;
}

//---------------------------------------------------------------------------
//
//	�{�[���[�g�`�F�b�N
//
//---------------------------------------------------------------------------
BOOL FASTCALL SCC::IsBaudRate(int channel, DWORD baudrate) const
{
	const ch_t *p;
	DWORD offset;

	ASSERT(this);
	ASSERT((channel == 0) || (channel == 1));
	ASSERT(baudrate >= 75);

	// �|�C���^�ݒ�
	p = &scc.ch[channel];

	// �^����ꂽ�{�[���[�g��5%���v�Z
	offset = baudrate * 5;
	offset /= 100;

	// �͈͓��Ȃ�OK
	if (p->baudrate < (baudrate - offset)) {
		return FALSE;
	}
	if ((baudrate + offset) < p->baudrate) {
		return FALSE;
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	��M�f�[�^�r�b�g���擾
//
//---------------------------------------------------------------------------
DWORD FASTCALL SCC::GetRxBit(int channel) const
{
	const ch_t *p;

	ASSERT(this);
	ASSERT((channel == 0) || (channel == 1));

	// �|�C���^�ݒ�
	p = &scc.ch[channel];

	return p->rxbit;
}

//---------------------------------------------------------------------------
//
//	�X�g�b�v�r�b�g���擾
//
//---------------------------------------------------------------------------
DWORD FASTCALL SCC::GetStopBit(int channel) const
{
	const ch_t *p;

	ASSERT(this);
	ASSERT((channel == 0) || (channel == 1));

	// �|�C���^�ݒ�
	p = &scc.ch[channel];

	return p->stopbit;
}

//---------------------------------------------------------------------------
//
//	�p���e�B�擾
//
//---------------------------------------------------------------------------
DWORD FASTCALL SCC::GetParity(int channel) const
{
	const ch_t *p;

	ASSERT(this);
	ASSERT((channel == 0) || (channel == 1));

	// �|�C���^�ݒ�
	p = &scc.ch[channel];

	return p->parity;
}

//---------------------------------------------------------------------------
//
//	�f�[�^��M
//
//---------------------------------------------------------------------------
DWORD FASTCALL SCC::Receive(int channel)
{
	ch_t *p;
	DWORD data;

	ASSERT(this);
	ASSERT((channel == 0) || (channel == 1));

	// �|�C���^�ݒ�
	p = &scc.ch[channel];

	// �f�[�^������
	data = 0;

	// �f�[�^�������
	if (p->txnum > 0) {
		// ���M�o�b�t�@������o��
		data = p->txbuf[p->txread];
		p->txread = (p->txread + 1) & (sizeof(p->txbuf) - 1);
		p->txnum--;
	}

	return data;
}

//---------------------------------------------------------------------------
//
//	���M�o�b�t�@�G���v�e�B�`�F�b�N
//
//---------------------------------------------------------------------------
BOOL FASTCALL SCC::IsTxEmpty(int channel)
{
	ch_t *p;

	ASSERT(this);
	ASSERT((channel == 0) || (channel == 1));

	// �|�C���^�ݒ�
	p = &scc.ch[channel];

	// ���M�o�b�t�@���󂢂Ă����TRUE��Ԃ�
	if (p->txnum == 0) {
		return TRUE;
	}
	return FALSE;
}

//---------------------------------------------------------------------------
//
//	���M�o�b�t�@�t���`�F�b�N
//
//---------------------------------------------------------------------------
BOOL FASTCALL SCC::IsTxFull(int channel)
{
	ch_t *p;

	ASSERT(this);
	ASSERT((channel == 0) || (channel == 1));

	// �|�C���^�ݒ�
	p = &scc.ch[channel];

	// ���M�o�b�t�@��3/4�ȏ㖄�܂��Ă���΁ATRUE
	if (p->txnum >= (sizeof(p->txbuf) * 3 / 4)) {
		return TRUE;
	}
	return FALSE;
}

//---------------------------------------------------------------------------
//
//	���M�u���b�N
//
//---------------------------------------------------------------------------
void FASTCALL SCC::WaitTx(int channel, BOOL wait)
{
	ch_t *p;

	ASSERT(this);
	ASSERT((channel == 0) || (channel == 1));

	// �|�C���^�ݒ�
	p = &scc.ch[channel];

	// ��������Ȃ�A�t���O���삪�K�v
	if (!wait) {
		if (p->txwait) {
			p->txsent = TRUE;
		}
	}

	// �ݒ�
	p->txwait = wait;
}

//---------------------------------------------------------------------------
//
//	�u���[�N�Z�b�g
//
//---------------------------------------------------------------------------
void FASTCALL SCC::SetBreak(int channel, BOOL flag)
{
	ch_t *p;

	ASSERT(this);
	ASSERT((channel == 0) || (channel == 1));

	// �|�C���^�ݒ�
	p = &scc.ch[channel];

	// ���荞�݃`�F�b�N
	if (p->baie) {
		// ���荞�ݐ����R�[�h
	}

	// �Z�b�g
	p->ba = flag;
}

//---------------------------------------------------------------------------
//
//	CTS�Z�b�g
//
//---------------------------------------------------------------------------
void FASTCALL SCC::SetCTS(int channel, BOOL flag)
{
	ch_t *p;

	ASSERT(this);
	ASSERT((channel == 0) || (channel == 1));

	// �|�C���^�ݒ�
	p = &scc.ch[channel];

	// ���荞�݃`�F�b�N
	if (p->ctsie) {
		// ���荞�ݐ����R�[�h
	}

	// �Z�b�g
	p->cts = flag;
}

//---------------------------------------------------------------------------
//
//	DCD�Z�b�g
//
//---------------------------------------------------------------------------
void FASTCALL SCC::SetDCD(int channel, BOOL flag)
{
	ch_t *p;

	ASSERT(this);
	ASSERT((channel == 0) || (channel == 1));

	// �|�C���^�ݒ�
	p = &scc.ch[channel];

	// ���荞�݃`�F�b�N
	if (p->dcdie) {
		// ���荞�ݐ����R�[�h
	}

	// �Z�b�g
	p->dcd = flag;
}

//---------------------------------------------------------------------------
//
//	�u���[�N�擾
//
//---------------------------------------------------------------------------
BOOL FASTCALL SCC::GetBreak(int channel)
{
	ch_t *p;

	ASSERT(this);
	ASSERT((channel == 0) || (channel == 1));

	// �|�C���^�ݒ�
	p = &scc.ch[channel];

	// ���̂܂�
	return p->brk;
}

//---------------------------------------------------------------------------
//
//	RTS�擾
//
//---------------------------------------------------------------------------
BOOL FASTCALL SCC::GetRTS(int channel)
{
	ch_t *p;

	ASSERT(this);
	ASSERT((channel == 0) || (channel == 1));

	// �|�C���^�ݒ�
	p = &scc.ch[channel];

	// ���̂܂�
	return p->rts;
}

//---------------------------------------------------------------------------
//
//	DTR�擾
//
//---------------------------------------------------------------------------
BOOL FASTCALL SCC::GetDTR(int channel)
{
	ch_t *p;

	ASSERT(this);
	ASSERT((channel == 0) || (channel == 1));

	// �|�C���^�ݒ�
	p = &scc.ch[channel];

	// DTR/REQ��FALSE�Ȃ�A�\�t�g�E�F�A�w��
	if (!p->dtrreq) {
		return p->dtr;
	}

	// ���M�o�b�t�@���G���v�e�B���ǂ���������
	if (p->txwait) {
		return FALSE;
	}
	return !(p->tdf);
}

//---------------------------------------------------------------------------
//
//	�C�x���g�R�[���o�b�N
//
//---------------------------------------------------------------------------
BOOL FASTCALL SCC::Callback(Event *ev)
{
	ch_t *p;
	int channel;

	ASSERT(this);
	ASSERT(ev);

	// �`���l�����o��
	channel = ev->GetUser();
	ASSERT((channel == 0) || (channel == 1));
	p = &scc.ch[channel];

	// ��M
	if (p->rxen) {
		EventRx(p);
	}

	// ���M
	if (p->txen) {
		EventTx(p);
	}

	// ���x�ύX
	if (ev->GetTime() != p->speed) {
		ev->SetTime(p->speed);
	}

	// �C���^�[�o���p��
	return TRUE;
}

//---------------------------------------------------------------------------
//
//	�C�x���g(��M)
//
//---------------------------------------------------------------------------
void FASTCALL SCC::EventRx(ch_t *p)
{
	BOOL flag;

	ASSERT(this);
	ASSERT(p);
	ASSERT((p->index == 0) || (p->index == 1));
	ASSERT(p->rxen);

	// ��M�t���O
	flag = TRUE;

	// DCD�̃`�F�b�N
	if (p->dcd && p->aen) {
		// ���[�v�o�b�N���������Ă���Ύ�M�ł��Ȃ�
		if (!p->loopback) {
			// ��M���Ȃ�
			flag = FALSE;
		}
	}

	// ��M�o�b�t�@���L���ł���΁AFIFO�ɑ}��
	if ((p->rxnum > 0) && flag) {
		if (p->rxfifo >= 3) {
			ASSERT(p->rxfifo == 3);

			// ����FIFO�͈�t�B�I�[�o�[����
#if defined(SCC_LOG)
			LOG1(Log::Normal, "�`���l��%d �I�[�o�[�����G���[", p->index);
#endif	// SCC_LOG
			p->overrun = TRUE;
			p->rxfifo--;

			if (p->rxim >= 2) {
				IntSCC(p, rsi, TRUE);
			}
		}

		ASSERT(p->rxfifo <= 2);

		// ��MFIFO�ɑ}��
		p->rxdata[2 - p->rxfifo] = (DWORD)p->rxbuf[p->rxread];
		p->rxread = (p->rxread + 1) & (sizeof(p->rxbuf) - 1);
		p->rxnum--;
		p->rxfifo++;

		// ��M�g�[�^������
		p->rxtotal++;
	}

	// ��MFIFO���L���ł���΁A���荞��
	if (p->rxfifo > 0) {
		// ���荞�ݔ���
		switch (p->rxim) {
			// ����̂݊��荞��
			case 1:
				if (!p->rxno) {
					break;
				}
				p->rxno = FALSE;
				// ���̂܂ܑ�����

			// ��Ɋ��荞��
			case 2:
				IntSCC(p, rxi, TRUE);
				break;
		}
	}
}

//---------------------------------------------------------------------------
//
//	�C�x���g(���M)
//
//---------------------------------------------------------------------------
void FASTCALL SCC::EventTx(ch_t *p)
{
	ASSERT(this);
	ASSERT(p);
	ASSERT((p->index == 0) || (p->index == 1));
	ASSERT(p->txen);

	// �E�F�C�g���[�h�łȂ���΁A�O��̑��M������
	if (!p->txwait) {
		p->txsent = TRUE;
	}

	// CTS�̃`�F�b�N
	if (p->cts && p->aen) {
		// �e�X�g���[�h����CTS�Ɋւ�炸���M�ł���
		if (!p->aecho && !p->loopback) {
			return;
		}
	}

	// �f�[�^�����邩
	if (p->tdf) {
		// �I�[�g�G�R�[���[�h�łȂ���
		if (!p->aecho) {
			// ���M�o�b�t�@�֑}��
			p->txbuf[p->txwrite] = (BYTE)p->tdr;
			p->txwrite = (p->txwrite + 1) & (sizeof(p->txbuf) - 1);
			p->txnum++;

			// ���ӂꂽ��A�Â��f�[�^����̂Ă�
			if (p->txnum >= sizeof(p->txbuf)) {
				LOG1(Log::Warning, "�`���l��%d ���M�o�b�t�@�I�[�o�[�t���[", p->index);
				p->txnum = sizeof(p->txbuf);
				p->txread = p->txwrite;
			}

			// ���M�g�[�^������
			p->txtotal++;
		}

		// ���M���s��
		p->txsent = FALSE;

		// ���M�o�b�t�@�G���v�e�B
		p->tdf = FALSE;

		// ���[�J�����[�v�o�b�N���[�h�ł���΁A�f�[�^����M���֓����
		if (p->loopback && !p->aecho) {
			// ��M�o�b�t�@�֑}��
			p->rxbuf[p->rxwrite] = (BYTE)p->tdr;
			p->rxwrite = (p->rxwrite + 1) & (sizeof(p->rxbuf) - 1);
			p->rxnum++;

			// ���ӂꂽ��A�Â��f�[�^���珇���̂Ă�
			if (p->rxnum >= sizeof(p->rxbuf)) {
				LOG1(Log::Warning, "�`���l��%d ��M�o�b�t�@�I�[�o�[�t���[", p->index);
				p->rxnum = sizeof(p->rxbuf);
				p->rxread = p->rxwrite;
			}
		}
	}

	// ���M�y���f�B���O���Ȃ���Ί��荞��
	if (!p->txpend) {
		IntSCC(p, txi, TRUE);
	}
}
