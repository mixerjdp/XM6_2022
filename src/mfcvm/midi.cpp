//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2005 �o�h�D(ytanaka@ipc-tokai.or.jp)
//	[ MIDI(YM3802) ]
//
//---------------------------------------------------------------------------

#include "os.h"
#include "xm6.h"
#include "vm.h"
#include "log.h"
#include "schedule.h"
#include "sync.h"
#include "config.h"
#include "fileio.h"
#include "midi.h"

//===========================================================================
//
//	MIDI
//
//===========================================================================
//#define MIDI_LOG

//---------------------------------------------------------------------------
//
//	�R���X�g���N�^
//
//---------------------------------------------------------------------------
MIDI::MIDI(VM *p) : MemDevice(p)
{
	// �f�o�C�XID��������
	dev.id = MAKEID('M', 'I', 'D', 'I');
	dev.desc = "MIDI (YM3802)";

	// �J�n�A�h���X�A�I���A�h���X
	memdev.first = 0xeae000;
	memdev.last = 0xeaffff;

	// �I�u�W�F�N�g
	sync = NULL;

	// ���M�o�b�t�@�A��M�o�b�t�@
	midi.transbuf = NULL;
	midi.recvbuf = NULL;

	// ���[�N�ꕔ������(���Ȑf�f�p)
	midi.transnum = 0;
	midi.transread = 0;
	midi.transwrite = 0;
	midi.recvnum = 0;
	midi.recvread = 0;
	midi.recvwrite = 0;
	midi.normnum = 0;
	midi.normread = 0;
	midi.normwrite = 0;
	midi.rtnum = 0;
	midi.rtread = 0;
	midi.rtwrite = 0;
	midi.stdnum = 0;
	midi.stdread = 0;
	midi.stdwrite = 0;
	midi.rrnum = 0;
	midi.rrread = 0;
	midi.rrwrite = 0;
	midi.bid = 0;
	midi.ilevel = 4;
}

//---------------------------------------------------------------------------
//
//	������
//
//---------------------------------------------------------------------------
BOOL FASTCALL MIDI::Init()
{
	ASSERT(this);

	// ��{�N���X
	if (!MemDevice::Init()) {
		return FALSE;
	}

	// Sync�쐬
	ASSERT(!sync);
	sync = new Sync;
	ASSERT(sync);

	// ���M�o�b�t�@�m��
	try {
		midi.transbuf = new mididata_t[TransMax];
	}
	catch (...) {
		midi.transbuf = NULL;
		return FALSE;
	}
	if (!midi.transbuf) {
		return FALSE;
	}

	// ��M�o�b�t�@�m��
	try {
		midi.recvbuf = new mididata_t[RecvMax];
	}
	catch (...) {
		midi.recvbuf = NULL;
		return FALSE;
	}
	if (!midi.recvbuf) {
		return FALSE;
	}

	// �C�x���g������
	event[0].SetDevice(this);
	event[0].SetDesc("MIDI 31250bps");
	event[0].SetUser(0);
	event[0].SetTime(0);
	event[1].SetDevice(this);
	event[1].SetDesc("Clock");
	event[1].SetUser(1);
	event[1].SetTime(0);
	event[2].SetDevice(this);
	event[2].SetDesc("General Timer");
	event[2].SetUser(2);
	event[2].SetTime(0);

	// �{�[�h�Ȃ��A���荞�݃��x��4
	midi.bid = 0;
	midi.ilevel = 4;

	// ��M�x�ꎞ��0ms
	recvdelay = 0;

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	�N���[���A�b�v
//
//---------------------------------------------------------------------------
void FASTCALL MIDI::Cleanup()
{
	ASSERT(this);

	// ��M�o�b�t�@�폜
	if (midi.recvbuf) {
		delete[] midi.recvbuf;
		midi.recvbuf = NULL;
	}

	// ���M�o�b�t�@�폜
	if (midi.transbuf) {
		delete[] midi.transbuf;
		midi.transbuf = NULL;
	}

	// Sync�폜
	if (sync) {
		delete sync;
		sync = NULL;
	}

	// ��{�N���X��
	MemDevice::Cleanup();
}

//---------------------------------------------------------------------------
//
//	���Z�b�g
//
//---------------------------------------------------------------------------
void FASTCALL MIDI::Reset()
{
	int i;

	ASSERT(this);
	ASSERT_DIAG();

	LOG0(Log::Normal, "���Z�b�g");

	// �A�N�Z�X�Ȃ�
	midi.access = FALSE;

	// ���Z�b�g����
	midi.reset = TRUE;

	// ���W�X�^���Z�b�g
	ResetReg();

	// �C�x���g������
	for (i=0; i<3; i++) {
		event[i].SetTime(0);
	}
}

//---------------------------------------------------------------------------
//
//	�Z�[�u
//
//---------------------------------------------------------------------------
BOOL FASTCALL MIDI::Save(Fileio *fio, int ver)
{
	size_t sz;
	int i;

	ASSERT(this);
	ASSERT(fio);
	ASSERT(ver >= 0x0200);
	ASSERT_DIAG();

	LOG0(Log::Normal, "�Z�[�u");

	// �T�C�Y���Z�[�u
	sz = sizeof(midi_t);
	if (!fio->Write(&sz, sizeof(sz))) {
		return FALSE;
	}

	// ���̂��Z�[�u
	if (!fio->Write(&midi, (int)sz)) {
		return FALSE;
	}

	// �C�x���g���Z�[�u
	for (i=0; i<3; i++) {
		if (!event[i].Save(fio, ver)) {
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
BOOL FASTCALL MIDI::Load(Fileio *fio, int ver)
{
	midi_t bk;
	size_t sz;
	int i;

	ASSERT(this);
	ASSERT(fio);
	ASSERT(ver >= 0x0200);
	ASSERT_DIAG();

	LOG0(Log::Normal, "���[�h");

	// ���݂̃��[�N��ۑ�
	bk = midi;

	// �T�C�Y�����[�h�A�ƍ�
	if (!fio->Read(&sz, sizeof(sz))) {
		return FALSE;
	}
	if (sz != sizeof(midi_t)) {
		return FALSE;
	}

	// ���̂����[�h
	if (!fio->Read(&midi, (int)sz)) {
		return FALSE;
	}

	// �C�x���g�����[�h
	for (i=0; i<3; i++) {
		if (!event[i].Load(fio, ver)) {
			return FALSE;
		}
	}

	// �C�x���g�𓮓I�ɒǉ��E�폜
	if (midi.bid == 0) {
		// �C�x���g�폜
		if (scheduler->HasEvent(&event[0])) {
			scheduler->DelEvent(&event[0]);
			scheduler->DelEvent(&event[1]);
			scheduler->DelEvent(&event[2]);
		}
	}
	else {
		// �C�x���g�ǉ�
		if (!scheduler->HasEvent(&event[0])) {
			scheduler->AddEvent(&event[0]);
			scheduler->AddEvent(&event[1]);
			scheduler->AddEvent(&event[2]);
		}
	}

	// ���M�o�b�t�@�𕜋A
	midi.transbuf = bk.transbuf;
	midi.transread = bk.transread;
	midi.transwrite = bk.transwrite;
	midi.transnum = bk.transnum;

	// ��M�o�b�t�@�𕜋A
	midi.recvbuf = bk.recvbuf;
	midi.recvread = bk.recvread;
	midi.recvwrite = bk.recvwrite;
	midi.recvnum = bk.recvnum;

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	�ݒ�K�p
//
//---------------------------------------------------------------------------
void FASTCALL MIDI::ApplyCfg(const Config *config)
{
	ASSERT(this);
	ASSERT(config);
	ASSERT_DIAG();

	LOG0(Log::Normal, "�ݒ�K�p");

	if (midi.bid != (DWORD)config->midi_bid) {
		midi.bid = (DWORD)config->midi_bid;

		// �C�x���g�𓮓I�ɒǉ��E�폜
		if (midi.bid == 0) {
			// �C�x���g�폜
			if (scheduler->HasEvent(&event[0])) {
				scheduler->DelEvent(&event[0]);
				scheduler->DelEvent(&event[1]);
				scheduler->DelEvent(&event[2]);
			}
		}
		else {
			// �C�x���g�ǉ�
			if (!scheduler->HasEvent(&event[0])) {
				scheduler->AddEvent(&event[0]);
				scheduler->AddEvent(&event[1]);
				scheduler->AddEvent(&event[2]);
			}
		}
	}
}

#if !defined(NDEBUG)
//---------------------------------------------------------------------------
//
//	�f�f
//
//---------------------------------------------------------------------------
void FASTCALL MIDI::AssertDiag() const
{
	// ��{�N���X
	MemDevice::AssertDiag();

	ASSERT(this);
	ASSERT(GetID() == MAKEID('M', 'I', 'D', 'I'));
	ASSERT(memdev.first == 0xeae000);
	ASSERT(memdev.last == 0xeaffff);
	ASSERT((midi.bid >= 0) && (midi.bid < 2));
	ASSERT((midi.ilevel == 2) || (midi.ilevel == 4));
	ASSERT(midi.transnum <= TransMax);
	ASSERT(midi.transread < TransMax);
	ASSERT(midi.transwrite < TransMax);
	ASSERT(midi.recvnum <= RecvMax);
	ASSERT(midi.recvread < RecvMax);
	ASSERT(midi.recvwrite < RecvMax);
	ASSERT(midi.normnum <= 16);
	ASSERT(midi.normread < 16);
	ASSERT(midi.normwrite < 16);
	ASSERT(midi.rtnum <= 4);
	ASSERT(midi.rtread < 4);
	ASSERT(midi.rtwrite < 4);
	ASSERT(midi.stdnum <= 0x80);
	ASSERT(midi.stdread < 0x80);
	ASSERT(midi.stdwrite < 0x80);
	ASSERT(midi.rrnum <= 4);
	ASSERT(midi.rrread < 4);
	ASSERT(midi.rrwrite < 4);
}
#endif	// NDEBUG

//---------------------------------------------------------------------------
//
//	�o�C�g�ǂݍ���
//
//---------------------------------------------------------------------------
DWORD FASTCALL MIDI::ReadByte(DWORD addr)
{
	ASSERT(this);
	ASSERT((addr >= memdev.first) && (addr <= memdev.last));
	ASSERT_DIAG();

	switch (midi.bid) {
		// �{�[�h����
		case 0:
			break;

		// �{�[�hID1
		case 1:
			// �͈͓���
			if ((addr >= 0xeafa00) && (addr < 0xeafa10)) {
				// ��A�h���X�̂݃f�R�[�h����Ă���
				if ((addr & 1) == 0) {
					return 0xff;
				}

				// ���W�X�^���[�h
				addr -= 0xeafa00;
				addr >>= 1;
				return ReadReg(addr);
			}
			break;

		// �{�[�hID2
		case 2:
			// �͈͓���
			if ((addr >= 0xeafa10) && (addr < 0xeafa20)) {
				// ��A�h���X�̂݃f�R�[�h����Ă���
				if ((addr & 1) == 0) {
					return 0xff;
				}

				// ���W�X�^���[�h
				addr -= 0xeafa10;
				addr >>= 1;
				return ReadReg(addr);
			}
			break;

		// ���̑�(���蓾�Ȃ�)
		default:
			ASSERT(FALSE);
			break;
	}

	// �o�X�G���[
	cpu->BusErr(addr, TRUE);

	return 0xff;
}

//---------------------------------------------------------------------------
//
//	���[�h�ǂݍ���
//
//---------------------------------------------------------------------------
DWORD FASTCALL MIDI::ReadWord(DWORD addr)
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
void FASTCALL MIDI::WriteByte(DWORD addr, DWORD data)
{
	ASSERT(this);
	ASSERT((addr >= memdev.first) && (addr <= memdev.last));
	ASSERT(data < 0x100);
	ASSERT_DIAG();

	switch (midi.bid) {
		// �{�[�h����
		case 0:
			break;

		// �{�[�hID1
		case 1:
			// �͈͓���
			if ((addr >= 0xeafa00) && (addr < 0xeafa10)) {
				// ��A�h���X�̂݃f�R�[�h����Ă���
				if ((addr & 1) == 0) {
					return;
				}

				// ���W�X�^���[�h
				addr -= 0xeafa00;
				addr >>= 1;
				WriteReg(addr, data);
				return;
			}
			break;

		// �{�[�hID2
		case 2:
			// �͈͓���
			if ((addr >= 0xeafa10) && (addr < 0xeafa20)) {
				// ��A�h���X�̂݃f�R�[�h����Ă���
				if ((addr & 1) == 0) {
					return;
				}

				// ���W�X�^���[�h
				addr -= 0xeafa10;
				addr >>= 1;
				WriteReg(addr, data);
				return;
			}
			break;

		// ���̑�(���蓾�Ȃ�)
		default:
			ASSERT(FALSE);
			break;
	}

	// �o�X�G���[
	cpu->BusErr(addr, FALSE);
}

//---------------------------------------------------------------------------
//
//	���[�h��������
//
//---------------------------------------------------------------------------
void FASTCALL MIDI::WriteWord(DWORD addr, DWORD data)
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
DWORD FASTCALL MIDI::ReadOnly(DWORD addr) const
{
	ASSERT(this);
	ASSERT((addr >= memdev.first) && (addr <= memdev.last));
	ASSERT_DIAG();

	switch (midi.bid) {
		// �{�[�h����
		case 0:
			break;

		// �{�[�hID1
		case 1:
			// �͈͓���
			if ((addr >= 0xeafa00) && (addr < 0xeafa10)) {
				// ��A�h���X�̂݃f�R�[�h����Ă���
				if ((addr & 1) == 0) {
					return 0xff;
				}

				// ���W�X�^���[�h
				addr -= 0xeafa00;
				addr >>= 1;
				return ReadRegRO(addr);
			}
			break;

		// �{�[�hID2
		case 2:
			// �͈͓���
			if ((addr >= 0xeafa10) && (addr < 0xeafa20)) {
				// ��A�h���X�̂݃f�R�[�h����Ă���
				if ((addr & 1) == 0) {
					return 0xff;
				}

				// ���W�X�^���[�h
				addr -= 0xeafa10;
				addr >>= 1;
				return ReadRegRO(addr);
			}
			break;

		// ���̑�(���蓾�Ȃ�)
		default:
			ASSERT(FALSE);
			break;
	}

	return 0xff;
}

//---------------------------------------------------------------------------
//
//	MIDI�A�N�e�B�u�`�F�b�N
//
//---------------------------------------------------------------------------
BOOL FASTCALL MIDI::IsActive() const
{
	ASSERT(this);
	ASSERT_DIAG();

	// �{�[�hID��0�ȊO�ł���
	if (midi.bid != 0) {
		// ���Z�b�g��A1��ȏ�A�N�Z�X����Ă��邱��
		if (midi.access) {
			return TRUE;
		}
	}

	// �A�N�e�B�u�łȂ�
	return FALSE;
}

//---------------------------------------------------------------------------
//
//	�C�x���g�R�[���o�b�N
//
//---------------------------------------------------------------------------
BOOL FASTCALL MIDI::Callback(Event *ev)
{
	DWORD mtr;
	DWORD hus;

	ASSERT(this);
	ASSERT(ev);
	ASSERT_DIAG();

	// �A�N�Z�X�t���O��FALSE�Ȃ�AMIDI�A�v���P�[�V�����͑��݂��Ȃ�
	if (!midi.access) {
		// �������̂��߁A�擪�Ń`�F�b�N
		return TRUE;
	}

	// �p�����[�^�ɂ���ĐU�蕪��
	switch (ev->GetUser()) {
		// MIDI����M 31250bps
		case 0:
			// ��M����ё��M
			Receive();
			Transmit();
			return TRUE;

		// MIDI�N���b�N(��Ԃ���)
		case 1:
			// ���MTR���Z�o���Ă���
			if (midi.mtr <= 1) {
				mtr = 0x40000;
			}
			else {
				mtr = midi.mtr << 4;
			}

			// ���R�[�f�B���O�J�E���^(8bit�J�E���^)
			midi.srr++;
			midi.srr &= 0xff;
			if (midi.srr == 0) {
				// ���荞��
				Interrupt(3, TRUE);
			}

			// �v���C�o�b�N�J�E���^(15bit�_�E���J�E���^)
			if (midi.str != 0xffff8000) {
				midi.str--;
			}
			if (midi.str >= 0xffff8000) {
				// ���荞��
				Interrupt(2, TRUE);
			}

			// �N���b�N��ԃJ�E���^���f�N�������g���A0�`�F�b�N
			midi.sct--;
			if (midi.sct != 0) {
				// SCT=1�̏ꍇ�A���Z�ɂ��؎̂Ă�₤���߁A�␳��������
				if (midi.sct == 1) {
					hus = mtr - (mtr / midi.scr) * (midi.scr - 1);
					if (event[1].GetTime() != hus) {
						event[1].SetTime(hus);
					}
				}
				return TRUE;
			}
			midi.sct = midi.scr;

			// �N���b�N����
			Clock();

			// ��M�L����
			if (midi.rcr & 1) {
				// �N���b�N�^�C�}���g����
				if ((midi.dmr & 0x07) == 0x03) {
					if (midi.dmr & 0x08) {
						// ���A���^�C����M�o�b�t�@�֑}��
						InsertRR(0xf8);
					}
				}
			}

			// �o�b�t�@�`�F�b�N
			CheckRR();

			// �^�C�}�l�͖��񃍁[�h����(Mu-1)
			mtr /= midi.scr;
			if (ev->GetTime() != mtr) {
				ev->SetTime(mtr);
			}
			return TRUE;

		// �ėp�^�C�}
		case 2:
			General();
			return TRUE;
	}

	// ����ȊO�̓G���[�B�ł��؂�
	ASSERT(FALSE);
	ev->SetTime(0);
	return FALSE;
}

//---------------------------------------------------------------------------
//
//	��M�R�[���o�b�N
//
//---------------------------------------------------------------------------
void FASTCALL MIDI::Receive()
{
	BOOL recv;
	mididata_t *p;
	DWORD diff;
	DWORD data;

	ASSERT(this);
	ASSERT_DIAG();

	// ���V�[�oREADY
	midi.rsr &= ~0x01;

	// ��M�f�B�Z�[�u���Ȃ牽�����Ȃ�
	if ((midi.rcr & 1) == 0) {
		// ��M���Ȃ�����
		return;
	}

	// ��M�p�����[�^���`�F�b�N
	if (midi.rrr != 0x08) {
		// 31250bps�łȂ�
		return;
	}
	if ((midi.rmr & 0x32) != 0) {
		// 8bit�L�����N�^�A1�X�g�b�v�r�b�g�A�m���p���e�B�łȂ�
		return;
	}

	// ��M�t���OOFF
	recv = FALSE;

	// ��M�o�b�t�@�̃`�F�b�N
	sync->Lock();
	if (midi.recvnum > 0) {
		// �L���ȃf�[�^������B���ԍ������݂�
		diff = scheduler->GetTotalTime();
		p = &midi.recvbuf[midi.recvread];
		diff -= p->vtime;
		if (diff > recvdelay) {
			// �����M����
			recv = TRUE;
		}
	}
	sync->Unlock();

	// �����M���Ȃ��̂ł���΁A��M�Ȃ����Ԃ�����
	if (!recv) {
		if (midi.rcn == 0x3ff) {
			// 327.68ms��M�Ȃ��Ȃ̂ŁA�I�t���C���Ɗ��荞��
			midi.rsr |= 0x04;
			if ((midi.imr & 4) == 0) {
				Interrupt(4, TRUE);
			}
		}
		midi.rcn++;
		if (midi.rcn >= 0x400) {
			midi.rcn = 0;
		}

		// ��M���Ȃ�����
		return;
	}

	// ��M�J�E���^���Z�b�g
	midi.rcn = 0;

	// ��M�o�b�t�@����f�[�^����荞��
	sync->Lock();
	data = midi.recvbuf[midi.recvread].data;
	midi.recvread = (midi.recvread + 1) & (RecvMax - 1);
	midi.recvnum--;
	sync->Unlock();

	if (data == 0xf8) {
		// ���A���^�C����M�o�b�t�@�֑}��
		if (midi.dmr & 0x08) {
			InsertRR(0xf8);

			// �N���b�N����
			Clock();

			// �o�b�t�@�`�F�b�N
			CheckRR();
		}

		// �N���b�N�t�B���^
		if (midi.rcr & 0x10) {
#if defined(MIDI_LOG)
			LOG0(Log::Normal, "MIDI�N���b�N�t�B���^���� $F8�X�L�b�v");
#endif	// MIDI_LOG
			return;
		}
	}
	if ((data >= 0xf9) && (data <= 0xfd)) {
		// ���A���^�C����M�o�b�t�@�֑}��
		if (midi.dmr & 0x08) {
			InsertRR(data);
		}
	}

	// �A�h���X�n���^
	if (midi.rcr & 2) {
		if (data == 0xf0) {
			// ���[�JID�`�F�b�N��
			midi.asr = 1;
		}
		if (data == 0xf7) {
			// �A�h���X�n���^�I��
			midi.asr = 0;
		}
		switch (midi.asr) {
			// ���[�JID�`�F�b�N��
			case 1:
				if ((midi.amr & 0x7f) == data) {
#if defined(MIDI_LOG)
					LOG1(Log::Normal, "���[�JID�}�b�` $%02X", midi.amr & 0x7f);
#endif	// MIDI_LOG
					midi.asr = 2;
				}
				else {
#if defined(MIDI_LOG)
					LOG1(Log::Normal, "���[�JID�A���}�b�` $%02X", data);
#endif	// MIDI_LOG
					midi.asr = 3;
				}
				break;

			// �f�o�C�XID�`�F�b�N��
			case 2:
				if (midi.amr & 0x80) {
					if ((midi.adr & 0x7f) != data) {
#if defined(MIDI_LOG)
						LOG1(Log::Normal, "�f�o�C�XID�A���}�b�` $%02X", data);
#endif	// MIDI_LOG
						midi.asr = 3;
					}
				}
				break;

			default:
				break;
		}
		if (midi.asr == 3) {
			// �A�h���X�n���^���쒆�B�X�L�b�v
			return;
		}
	}
	else {
		// �A�h���X�n���^ �f�B�Z�[�u��
		midi.asr = 0;
	}

	// ���V�[�oBUSY
	midi.rsr |= 0x01;

	// �W���o�b�t�@��
	InsertStd(data);
}

//---------------------------------------------------------------------------
//
//	���M�R�[���o�b�N
//
//---------------------------------------------------------------------------
void FASTCALL MIDI::Transmit()
{
	ASSERT(this);
	ASSERT_DIAG();

	// ���MReady
	midi.tbs = FALSE;

	// ���A���^�C�����M�o�b�t�@�̃`�F�b�N
	if (midi.rtnum > 0) {
		// ���A���^�C�����M�o�b�t�@�̃f�[�^�𑗐M�o�b�t�@�֓]��
		ASSERT(midi.rtnum <= 4);
		if (midi.tcr & 1) {
			InsertTrans(midi.rtbuf[midi.rtread]);
		}
		midi.rtnum--;
		midi.rtread = (midi.rtread + 1) & 3;

		// ���MBUSY�A�����M�Ȃ�
		midi.tbs = TRUE;
		midi.tcn = 0;
		return;
	}

	// �ʏ�o�b�t�@�̃`�F�b�N
	if (midi.normnum > 0) {
		// �ʏ�o�b�t�@�̃f�[�^�𑗐M�o�b�t�@�֓]��
		ASSERT(midi.normnum <= 16);
		if (midi.tcr & 1) {
			InsertTrans(midi.normbuf[midi.normread]);
		}
		midi.normnum--;
		midi.normread = (midi.normread + 1) & 15;

		// �ʏ�o�b�t�@���N���A�����΁A���M�o�b�t�@�G���v�e�B���荞��
		if (midi.normnum == 0) {
			Interrupt(6, TRUE);
		}

		// ���MBUSY�A�����M�Ȃ�
		midi.tbs = TRUE;
		midi.tcn = 0;
		return;
	}

	// ����͑��M���Ȃ������B�����M���Ԃ�����
	if (midi.tcn == 0xff) {
		// 81.92ms�����M
		if (midi.tcr & 1) {
			if (midi.dmr & 0x20) {
				// �A�N�e�B�u�Z���V���O���M(DMR�̃r�b�g�w���ɂ��)
				InsertRT(0xfe);
			}
		}
	}
	midi.tcn++;
	if (midi.tcn >= 0x100) {
		midi.tcn = 0x100;
	}
}

//---------------------------------------------------------------------------
//
//	MIDI�N���b�N���o
//
//---------------------------------------------------------------------------
void FASTCALL MIDI::Clock()
{
	ASSERT(this);
	ASSERT_DIAG();

	// �N���b�N�J�E���^
	midi.ctr--;
	if (midi.ctr == 0) {
		// �����[�h
		if (midi.cdr == 0) {
			midi.ctr = 0x80;
		}
		else {
			midi.ctr = midi.cdr;
		}

		// ���荞��
		if ((midi.imr & 8) == 0) {
			Interrupt(1, TRUE);
		}
	}
}

//---------------------------------------------------------------------------
//
//	�ėp�^�C�}�R�[���o�b�N
//
//---------------------------------------------------------------------------
void FASTCALL MIDI::General()
{
	DWORD hus;

	ASSERT(this);
	ASSERT_DIAG();

	// ���荞�ݔ���
	Interrupt(7, TRUE);

	// GTR���ω����Ă���΁A���Ԃ��Đݒ�
	hus = midi.gtr << 4;
	if (event[2].GetTime() != hus) {
		event[2].SetTime(hus);
	}
}

//---------------------------------------------------------------------------
//
//	���M�o�b�t�@�֑}��
//
//---------------------------------------------------------------------------
void FASTCALL MIDI::InsertTrans(DWORD data)
{
	ASSERT(this);
	ASSERT(data < 0x100);
	ASSERT_DIAG();

#if defined(MIDI_LOG)
	LOG1(Log::Normal, "���M�o�b�t�@�}�� $%02X", data);
#endif	// MIDI_LOG

	// ���b�N
	sync->Lock();

	// �f�[�^�}��
	midi.transbuf[midi.transwrite].data = data;
	midi.transbuf[midi.transwrite].vtime = scheduler->GetTotalTime();
	midi.transwrite = (midi.transwrite + 1) & (TransMax - 1);
	midi.transnum++;
	if (midi.transnum > TransMax) {
		// ���M�o�b�t�@�̃I�[�o�[�t���[�͖���(I/O�G�~�����[�V�������s��Ȃ��ꍇ)
		midi.transnum = TransMax;
		midi.transread = midi.transwrite;
	}

	// �A�����b�N
	sync->Unlock();
}

//---------------------------------------------------------------------------
//
//	��M�o�b�t�@�֑}��
//
//---------------------------------------------------------------------------
void FASTCALL MIDI::InsertRecv(DWORD data)
{
	ASSERT(this);
	ASSERT(data < 0x100);
	ASSERT_DIAG();

#if defined(MIDI_LOG)
	LOG1(Log::Normal, "��M�o�b�t�@�}�� $%02X", data);
#endif	// MIDI_LOG

	// ���b�N
	sync->Lock();

	// �f�[�^�}��
	midi.recvbuf[midi.recvwrite].data = data;
	midi.recvbuf[midi.recvwrite].vtime = scheduler->GetTotalTime();
	midi.recvwrite = (midi.recvwrite + 1) & (RecvMax - 1);
	midi.recvnum++;
	if (midi.recvnum > RecvMax) {
		LOG0(Log::Warning, "��M�o�b�t�@ �I�[�o�[�t���[");
		midi.recvnum = RecvMax;
		midi.recvread = midi.recvwrite;
	}

	// �A�����b�N
	sync->Unlock();
}

//---------------------------------------------------------------------------
//
//	�ʏ�o�b�t�@�֑}��
//
//---------------------------------------------------------------------------
void FASTCALL MIDI::InsertNorm(DWORD data)
{
	ASSERT(this);
	ASSERT(data < 0x100);
	ASSERT_DIAG();

#if defined(MIDI_LOG)
	LOG1(Log::Normal, "�ʏ�o�b�t�@�}�� $%02X", data);
#endif	// MIDI_LOG

	// ���b�N
	sync->Lock();

	// �f�[�^�}��
	midi.normbuf[midi.normwrite] = data;
	midi.normwrite = (midi.normwrite + 1) & 15;
	midi.normnum++;
	midi.normtotal++;
	if (midi.normnum > 16) {
		LOG0(Log::Warning, "�ʏ�o�b�t�@ �I�[�o�[�t���[");
		midi.normnum = 16;
		midi.normread = midi.normwrite;
	}

	// �A�����b�N
	sync->Unlock();
}

//---------------------------------------------------------------------------
//
//	���A���^�C�����M�o�b�t�@�֑}��
//
//---------------------------------------------------------------------------
void FASTCALL MIDI::InsertRT(DWORD data)
{
	ASSERT(this);
	ASSERT(data < 0x100);
	ASSERT_DIAG();

#if defined(MIDI_LOG)
	LOG1(Log::Normal, "���A���^�C�����M�o�b�t�@�}�� $%02X", data);
#endif	// MIDI_LOG

	// ���b�N
	sync->Lock();

	// �f�[�^�}��
	midi.rtbuf[midi.rtwrite] = data;
	midi.rtwrite = (midi.rtwrite + 1) & 3;
	midi.rtnum++;
	midi.rttotal++;
	if (midi.rtnum > 4) {
		LOG0(Log::Warning, "���A���^�C�����M�o�b�t�@ �I�[�o�[�t���[");
		midi.rtnum = 4;
		midi.rtread = midi.rtwrite;
	}

	// �A�����b�N
	sync->Unlock();
}

//---------------------------------------------------------------------------
//
//	��ʃo�b�t�@�֑}��
//
//---------------------------------------------------------------------------
void FASTCALL MIDI::InsertStd(DWORD data)
{
	ASSERT(this);
	ASSERT(data < 0x100);
	ASSERT_DIAG();

#if defined(MIDI_LOG)
	LOG1(Log::Normal, "��ʃo�b�t�@�}�� $%02X", data);
#endif	// MIDI_LOG

	// ���b�N
	sync->Lock();

	// �f�[�^�}��
	midi.stdbuf[midi.stdwrite] = data;
	midi.stdwrite = (midi.stdwrite + 1) & 0x7f;
	midi.stdnum++;
	midi.stdtotal++;
	if (midi.stdnum > 0x80) {
#if defined(MIDI_LOG)
		LOG0(Log::Warning, "��ʃo�b�t�@ �I�[�o�[�t���[");
#endif	// MIDI_LOG
		midi.stdnum = 0x80;
		midi.stdread = midi.stdwrite;

		// ��M�X�e�[�^�X�ݒ�(�I�[�o�[�t���[)
		midi.rsr |= 0x40;
	}

	// �A�����b�N
	sync->Unlock();

	// ��M�X�e�[�^�X�ݒ�(��M�f�[�^����)
	midi.rsr |= 0x80;

	// ���荞�ݔ���(�G���v�e�B����M�f�[�^����)
	if (midi.stdnum == 1) {
		Interrupt(5, TRUE);
	}
}

//---------------------------------------------------------------------------
//
//	���A���^�C����M�o�b�t�@�֑}��
//
//---------------------------------------------------------------------------
void FASTCALL MIDI::InsertRR(DWORD data)
{
	ASSERT(this);
	ASSERT(data < 0x100);
	ASSERT_DIAG();

#if defined(MIDI_LOG)
	LOG1(Log::Normal, "���A���^�C����M�o�b�t�@�}�� $%02X", data);
#endif	// MIDI_LOG

	// ���b�N
	sync->Lock();

	// �f�[�^�}��
	midi.rrbuf[midi.rrwrite] = data;
	midi.rrwrite = (midi.rrwrite + 1) & 3;
	midi.rrnum++;
	midi.rrtotal++;
	if (midi.rrnum > 4) {
#if defined(MIDI_LOG)
		LOG0(Log::Warning, "���A���^�C����M�o�b�t�@ �I�[�o�[�t���[");
#endif	// MIDI_LOG
		midi.rrnum = 4;
		midi.rrread = midi.rrwrite;
	}

	// �A�����b�N
	sync->Unlock();
}

//---------------------------------------------------------------------------
//
//	���M�o�b�t�@���擾
//
//---------------------------------------------------------------------------
DWORD FASTCALL MIDI::GetTransNum() const
{
	ASSERT(this);
	ASSERT_DIAG();

	// 4�o�C�g�f�[�^�ǂ݂̂��߁A���b�N���Ȃ�
	// (1��̃������o�X�A�N�Z�X�Ńf�[�^�m�肷�邱�Ƃ��O��)
	return  midi.transnum;
}

//---------------------------------------------------------------------------
//
//	���M�o�b�t�@�f�[�^�擾
//
//---------------------------------------------------------------------------
const MIDI::mididata_t* FASTCALL MIDI::GetTransData(DWORD proceed)
{
	const mididata_t *ptr;
	int offset;

	ASSERT(this);
	ASSERT(proceed < TransMax);
	ASSERT_DIAG();

	// ���b�N
	sync->Lock();

	// �擾
	offset = midi.transread;
	offset += proceed;
	offset &= (TransMax - 1);
	ptr = &(midi.transbuf[offset]);

	// �A�����b�N
	sync->Unlock();

	// �Ԃ�
	return ptr;
}

//---------------------------------------------------------------------------
//
//	���M�o�b�t�@�폜
//
//---------------------------------------------------------------------------
void FASTCALL MIDI::DelTransData(DWORD number)
{
	ASSERT(this);
	ASSERT(number < TransMax);
	ASSERT_DIAG();

	// ���b�N
	sync->Lock();

	// ������
	if (number > midi.transnum) {
		number = midi.transnum;
	}

	// ����read�|�C���^��ύX
	midi.transnum -= number;
	midi.transread = (midi.transread + number) & (TransMax - 1);

	// �A�����b�N
	sync->Unlock();
}

//---------------------------------------------------------------------------
//
//	���M�o�b�t�@�N���A
//
//---------------------------------------------------------------------------
void FASTCALL MIDI::ClrTransData()
{
	ASSERT(this);
	ASSERT_DIAG();

	// ���b�N
	sync->Lock();

	// �N���A
	midi.transnum = 0;
	midi.transread = 0;
	midi.transwrite = 0;

	// �A�����b�N
	sync->Unlock();
}

//---------------------------------------------------------------------------
//
//	��M�o�b�t�@�f�[�^�ݒ�
//
//---------------------------------------------------------------------------
void FASTCALL MIDI::SetRecvData(const BYTE *ptr, DWORD length)
{
	DWORD lp;

	ASSERT(this);
	ASSERT(ptr);
	ASSERT_DIAG();

	// �{�[�hID��0�ł���Ή������Ȃ�
	if (midi.bid == 0) {
		return;
	}

	// ��M�ł���ꍇ�̂݁A�󂯕t����
	if (midi.rcr & 1) {
		// ��M�o�b�t�@�֑}��
		for (lp=0; lp<length; lp++) {
			InsertRecv((DWORD)ptr[lp]);
		}
	}

	// ��M�ł���A�ł��Ȃ��ɂ�����炸�ATHRU�w�肪����Α��M�o�b�t�@��
	if (midi.trr & 0x40) {
		for (lp=0; lp<length; lp++) {
			InsertTrans((DWORD)ptr[lp]);
		}
	}
}

//---------------------------------------------------------------------------
//
//	��M�f�B���C�ݒ�
//
//---------------------------------------------------------------------------
void FASTCALL MIDI::SetRecvDelay(int delay)
{
	ASSERT(this);
	ASSERT(delay >= 0);
	ASSERT_DIAG();

	// hus�P�ʂł��邱��
	recvdelay = delay;
}

//---------------------------------------------------------------------------
//
//	���W�X�^���Z�b�g
//
//---------------------------------------------------------------------------
void FASTCALL MIDI::ResetReg()
{
	ASSERT(this);
	ASSERT_DIAG();

#if defined(MIDI_LOG)
	LOG0(Log::Normal, "���W�X�^���Z�b�g");
#endif	// MIDI_LOG

	// ���荞��
	midi.vector = -1;

	// MCS���W�X�^(���)
	midi.wdr = 0;
	midi.rgr = 0xff;

	// MCS���W�X�^(���荞��)
	// IMR��bit1�𗧂ĂĂ���(�X�[�p�[�n���O�I��)
	midi.ivr = 0x10;
	midi.isr = 0;
	midi.imr = 0x02;
	midi.ier = 0;

	// MCS���W�X�^(���A���^�C�����b�Z�[�W)
	midi.dmr = 0;
	midi.dcr = 0;

	// MCS���W�X�^(��M)
	midi.rrr = 0;
	midi.rmr = 0;
	midi.rcn = 0;
	midi.amr = 0;
	midi.adr = 0;
	midi.asr = 0;
	midi.rsr = 0;
	midi.rcr = 0;

	// MCS���W�X�^(���M)
	midi.trr = 0;
	midi.tmr = 0;
	midi.tbs = FALSE;
	midi.tcn = 0;
	midi.tcr = 0;

	// MCS���W�X�^(FSK)
	midi.fsr = 0;
	midi.fcr = 0;

	// MCS���W�X�^(�J�E���^)
	midi.ccr = 2;
	midi.cdr = 0;
	midi.ctr = 0x80;
	midi.srr = 0;
	midi.scr = 1;
	midi.sct = 1;
	midi.spr = 0;
	midi.str = 0xffff8000;
	midi.gtr = 0;
	midi.mtr = 0;

	// MCS���W�X�^(GPIO)
	midi.edr = 0;
	midi.eor = 0;
	midi.eir = 0;

	// �ʏ�o�b�t�@
	midi.normnum = 0;
	midi.normread = 0;
	midi.normwrite = 0;
	midi.normtotal = 0;

	// ���A���^�C�����M�o�b�t�@
	midi.rtnum = 0;
	midi.rtread = 0;
	midi.rtwrite = 0;
	midi.rttotal = 0;

	// ��ʃo�b�t�@
	midi.stdnum = 0;
	midi.stdread = 0;
	midi.stdwrite = 0;
	midi.stdtotal = 0;

	// ���A���^�C����M�o�b�t�@
	midi.rrnum = 0;
	midi.rrread = 0;
	midi.rrwrite = 0;
	midi.rrtotal = 0;

	// ���M�o�b�t�@
	sync->Lock();
	midi.transnum = 0;
	midi.transread = 0;
	midi.transwrite = 0;
	sync->Unlock();

	// ��M�o�b�t�@
	sync->Lock();
	midi.recvnum = 0;
	midi.recvread = 0;
	midi.recvwrite = 0;
	sync->Unlock();

	// �C�x���g��~(�ėp�^�C�})
	event[2].SetTime(0);

	// �C�x���g�J�n(MIDI�N���b�N)(OPMDRV3.X)
	event[1].SetDesc("Clock");
	event[1].SetTime(0x40000);

	// �G�N�X�N���[�V�u�J�E���^
	ex_cnt[0] = 0;
	ex_cnt[1] = 0;
	ex_cnt[2] = 0;
	ex_cnt[3] = 0;

	// �S�Ă̊��荞�݂��L�����Z��
	IntCheck();

	// �ʏ�o�b�t�@�G���v�e�B���荞��(���ۂ�ISR�̂ݕω�)
	Interrupt(6, TRUE);
}

//---------------------------------------------------------------------------
//
//	���W�X�^�ǂݏo��
//
//---------------------------------------------------------------------------
DWORD FASTCALL MIDI::ReadReg(DWORD reg)
{
	DWORD group;

	ASSERT(this);
	ASSERT(reg < 8);
	ASSERT_DIAG();

	// �A�N�Z�X�t���OON
	if (!midi.access) {
		event[0].SetTime(640);
		midi.access = TRUE;
	}

	// �E�F�C�g
	scheduler->Wait(2);

	switch (reg) {
		// IVR
		case 0:
#if defined(MIDI_LOG)
			LOG1(Log::Normal, "���荞�݃x�N�^�ǂݏo�� $%02X", midi.ivr);
#endif	// MIDI_LOG
			return midi.ivr;

		// RGR
		case 1:
			return midi.rgr;

		// ISR
		case 2:
#if defined(MIDI_LOG)
			LOG1(Log::Normal, "���荞�݃T�[�r�X���W�X�^�ǂݏo�� $%02X", midi.isr);
#endif	// MIDI_LOG
			return midi.isr;

		// ICR
		case 3:
			LOG1(Log::Normal, "���W�X�^�ǂݏo�� R%d", reg);
			return midi.wdr;
	}

	// �O���[�v����A���W�X�^�ԍ����쐬
	group = midi.rgr & 0x0f;
	if (group >= 0x0a) {
		return 0xff;
	}

	// ���W�X�^�ԍ�����
	reg += (group * 10);

	// ���W�X�^��(���W�X�^4�ȍ~)
	switch (reg) {
		// DSR
		case 16:
			return GetDSR();

		// RSR
		case 34:
			return GetRSR();

		// RDR(�X�V����)
		case 36:
			return GetRDR();

		// TSR
		case 54:
			return GetTSR();

		// FSR
		case 64:
			return GetFSR();

		// SRR
		case 74:
			return GetSRR();

		// EIR
		case 96:
			return GetEIR();
	}

	LOG1(Log::Normal, "���W�X�^�ǂݏo�� R%d", reg);
	return midi.wdr;
}

//---------------------------------------------------------------------------
//
//	���W�X�^��������
//
//---------------------------------------------------------------------------
void FASTCALL MIDI::WriteReg(DWORD reg, DWORD data)
{
	DWORD group;

	ASSERT(this);
	ASSERT(reg < 8);
	ASSERT(data < 0x100);
	ASSERT_DIAG();

	// �A�N�Z�X�t���OON
	if (!midi.access) {
		event[0].SetTime(640);
		midi.access = TRUE;
	}

	// �E�F�C�g
	scheduler->Wait(2);

	// �������݃f�[�^��ۑ�(����`�̃��W�X�^�œǂ߂�)
	midi.wdr = data;

	// ���W�X�^��
	switch (reg) {
		// IVR
		case 0:
			LOG2(Log::Normal, "���W�X�^�������� R%d <- $%02X", reg, data);
			return;

		// RGR
		case 1:
			// �ŏ�ʃr�b�g�̓��W�X�^���Z�b�g
			if (data & 0x80) {
				ResetReg();
			}
			midi.rgr = data;
			return;

		// ISR
		case 2:
			LOG2(Log::Normal, "���W�X�^�������� R%d <- $%02X", reg, data);
			return;

		// ICR
		case 3:
			SetICR(data);
			return;
	}

	// �O���[�v����A���W�X�^�ԍ����쐬
	group = midi.rgr & 0x0f;
	if (group >= 0x0a) {
		return;
	}

	// ���W�X�^�ԍ�����
	reg += (group * 10);

	// ���W�X�^��(���W�X�^4�ȍ~)
	switch (reg) {
		// IOR
		case 4:
			SetIOR(data);
			return;

		// IMR
		case 5:
			SetIMR(data);
			return;

		// IER
		case 6:
			SetIER(data);
			return;

		// DMR
		case 14:
			SetDMR(data);
			return;

		// DCR
		case 15:
			SetDCR(data);
			return;

		// DNR
		case 17:
			SetDNR(data);
			return;

		// RRR
		case 24:
			SetRRR(data);
			return;

		// RMR
		case 25:
			SetRMR(data);
			return;

		// AMR
		case 26:
			SetAMR(data);
			return;

		// ADR
		case 27:
			SetADR(data);
			return;

		// RCR
		case 35:
			SetRCR(data);
			return;

		// TRR
		case 44:
			SetTRR(data);
			return;

		// TMR
		case 45:
			SetTMR(data);
			return;

		// TCR
		case 55:
			SetTCR(data);
			return;

		// TDR
		case 56:
			SetTDR(data);
			return;

		// FCR
		case 65:
			SetFCR(data);
			return;

		// CCR
		case 66:
			SetCCR(data);
			return;

		// CDR
		case 67:
			SetCDR(data);
			return;

		// SCR
		case 75:
			SetSCR(data);
			return;

		// SPR(L)
		case 76:
			SetSPR(data, FALSE);
			return;

		// SPR(H)
		case 77:
			SetSPR(data, TRUE);
			return;

		// GTR(L)
		case 84:
			SetGTR(data, FALSE);
			return;

		// GTR(H)
		case 85:
			SetGTR(data, TRUE);
			return;

		// MTR(L)
		case 86:
			SetMTR(data, FALSE);
			return;

		// MTR(H)
		case 87:
			SetMTR(data, TRUE);
			return;

		// EDR
		case 94:
			SetEDR(data);
			return;

		// EOR
		case 96:
			SetEOR(data);
			return;
	}

	LOG2(Log::Normal, "���W�X�^�������� R%d <- $%02X", reg, data);
}

//---------------------------------------------------------------------------
//
//	���W�X�^�ǂݏo��(ReadOnly)
//
//---------------------------------------------------------------------------
DWORD FASTCALL MIDI::ReadRegRO(DWORD reg) const
{
	DWORD group;

	ASSERT(this);
	ASSERT(reg < 8);
	ASSERT_DIAG();

	switch (reg) {
		// IVR
		case 0:
			return midi.ivr;

		// RGR
		case 1:
			return midi.rgr;

		// ISR
		case 2:
			return midi.isr;

		// ICR
		case 3:
			return midi.wdr;
	}

	// �O���[�v����A���W�X�^�ԍ����쐬
	group = midi.rgr & 0x0f;
	if (group >= 0x0a) {
		return 0xff;
	}

	// ���W�X�^�ԍ�����
	reg += (group * 10);

	// ���W�X�^��(���W�X�^4�ȍ~)
	switch (reg) {
		// DSR
		case 16:
			return GetDSR();

		// RSR
		case 34:
			return GetRSR();

		// RDR(ReadOnly)
		case 36:
			return GetRDRRO();

		// TSR
		case 54:
			return GetTSR();

		// FSR
		case 64:
			return GetFSR();

		// SRR
		case 74:
			return GetSRR();

		// EIR
		case 96:
			return GetEIR();
	}

	return midi.wdr;
}

//---------------------------------------------------------------------------
//
//	ICR�ݒ�
//
//---------------------------------------------------------------------------
void FASTCALL MIDI::SetICR(DWORD data)
{
	ASSERT(this);
	ASSERT(data < 0x100);
	ASSERT_DIAG();

#if defined(MIDI_LOG)
	LOG1(Log::Normal, "���荞�݃N���A���W�X�^�ݒ� $%02X", data);
#endif

	// �r�b�g���]�f�[�^���g���āA�C���T�[�r�X���N���A
	midi.isr &= ~data;

	// ���荞�݃`�F�b�N
	IntCheck();
}

//---------------------------------------------------------------------------
//
//	IOR�ݒ�
//
//---------------------------------------------------------------------------
void FASTCALL MIDI::SetIOR(DWORD data)
{
	ASSERT(this);
	ASSERT(data < 0x100);
	ASSERT_DIAG();

	// ���3bit�̂ݗL��
	data &= 0xe0;

	LOG1(Log::Detail, "���荞�݃x�N�^�x�[�X $%02X", data);

	// IVR��ύX����
	midi.ivr &= 0x1f;
	midi.ivr |= data;

	// ���荞�݃`�F�b�N
	IntCheck();
}

//---------------------------------------------------------------------------
//
//	IMR�ݒ�
//
//---------------------------------------------------------------------------
void FASTCALL MIDI::SetIMR(DWORD data)
{
	ASSERT(this);
	ASSERT(data < 0x100);
	ASSERT_DIAG();

	data &= 0x0f;

	if (midi.imr != data) {
#if defined(MIDI_LOG)
		LOG1(Log::Normal, "���荞�݃��[�h���W�X�^�ݒ� $%02X", data);
#endif	// MIDI_LOG
		midi.imr = data;

		// ���荞�݃`�F�b�N
		IntCheck();
	}
}

//---------------------------------------------------------------------------
//
//	IER�ݒ�
//
//---------------------------------------------------------------------------
void FASTCALL MIDI::SetIER(DWORD data)
{
	ASSERT(this);
	ASSERT(data < 0x100);
	ASSERT_DIAG();

	if (midi.ier != data) {
#if defined(MIDI_LOG)
		LOG1(Log::Normal, "���荞�݋����W�X�^�ݒ� $%02X", data);
#endif	// MIDI_LOG

		// �ݒ�
		midi.ier = data;

		// ���荞�݃`�F�b�N
		IntCheck();
	}
}

//---------------------------------------------------------------------------
//
//	DMR�ݒ�
//
//---------------------------------------------------------------------------
void FASTCALL MIDI::SetDMR(DWORD data)
{
	ASSERT(this);
	ASSERT(data < 0x100);
	ASSERT_DIAG();

	// �L���f�[�^�����c��
	data &= 0x3f;

	if (midi.dmr != data) {
#if defined(MIDI_LOG)
		LOG1(Log::Normal, "���A���^�C�����b�Z�[�W���[�h���W�X�^�ݒ� $%02X", data);
#endif

		// �ݒ�
		midi.dmr = data;
	}
}

//---------------------------------------------------------------------------
//
//	DCR�ݒ�
//
//---------------------------------------------------------------------------
void FASTCALL MIDI::SetDCR(DWORD data)
{
	DWORD msg;

	ASSERT(this);
	ASSERT(data < 0x100);
	ASSERT_DIAG();

#if defined(MIDI_LOG)
	LOG1(Log::Normal, "���A���^�C�����b�Z�[�W�R���g���[�����W�X�^�ݒ� $%02X", data);
#endif

	// �ݒ�
	midi.dcr = data;

	// ���b�Z�[�W�쐬
	msg = data & 0x07;
	msg += 0xf8;
	if (msg >= 0xfe) {
		LOG1(Log::Warning, "�s���ȃ��A���^�C�����b�Z�[�W $%02X", msg);
		return;
	}

	// MIDI�N���b�N���b�Z�[�W��DMR��bit2�ŕ�����
	if (msg == 0xf8) {
		if ((midi.dmr & 4) == 0) {
			// MIDI�N���b�N�^�C�}�A$F8��M�ȂǂŃN���b�N�Ƃ���
			return;
		}

		// ���̃^�C�~���O��MIDI�N���b�N��M�Ƃ���
		if (midi.rcr & 1) {
			if (midi.dmr & 8) {
				// ���A���^�C����M�o�b�t�@�֑}��
				InsertRR(0xf8);

				// �N���b�N����
				Clock();

				// �o�b�t�@�`�F�b�N
				CheckRR();
			}
		}
		return;
	}
	else {
		// ����ȊO�̃��b�Z�[�W�́A���A���^�C�����M�o�b�t�@����
		if ((data & 0x80) == 0) {
			return;
		}
	}

	// ���A���^�C�����M�o�b�t�@�փf�[�^��}��
	InsertRT(msg);
}

//---------------------------------------------------------------------------
//
//	DSR�擾
//
//---------------------------------------------------------------------------
DWORD FASTCALL MIDI::GetDSR() const
{
	ASSERT(this);
	ASSERT_DIAG();

	// FIFO-IRx�̃f�[�^��ǂݏo���B�f�[�^���Ȃ����0
	if (midi.rrnum == 0) {
		return 0;
	}

#if defined(MIDI_LOG)
	LOG1(Log::Normal, "FIFO-IRx�擾 $%02X", midi.rrbuf[midi.rrread]);
#endif	// MIDI_LOG

	return midi.rrbuf[midi.rrread];
}

//---------------------------------------------------------------------------
//
//	DNR�ݒ�
//
//---------------------------------------------------------------------------
void FASTCALL MIDI::SetDNR(DWORD data)
{
	ASSERT(this);
	ASSERT(data < 0x100);
	ASSERT_DIAG();

	if (data & 0x01) {
#if defined(MIDI_LOG)
		LOG0(Log::Normal, "FIFO-IRx�X�V");
#endif	// MIDI_LOG

		// ���A���^�C����M�o�b�t�@���X�V
		if (midi.rrnum > 0) {
			midi.rrnum--;
			midi.rrread = (midi.rrread + 1) & 3;
		}

		// ���A���^�C����M�o�b�t�@���荞�݃`�F�b�N
		CheckRR();
	}
}

//---------------------------------------------------------------------------
//
//	RRR�ݒ�
//
//---------------------------------------------------------------------------
void FASTCALL MIDI::SetRRR(DWORD data)
{
	ASSERT(this);
	ASSERT(data < 0x100);
	ASSERT_DIAG();

	// �L�����������c��
	data &= 0x38;

	if (midi.rrr != data) {
#if defined(MIDI_LOG)
		LOG1(Log::Normal, "��M���[�g�ݒ� $%02X", data);
#endif	// MIDI_LOG

		// �ݒ�
		midi.rrr = data;
	}

	// ��M���[�g�`�F�b�N
	if (midi.rrr != 0x08) {
		LOG1(Log::Warning, "��M���[�g�ُ� $%02X", midi.rrr);
	}
}

//---------------------------------------------------------------------------
//
//	RMR�ݒ�
//
//---------------------------------------------------------------------------
void FASTCALL MIDI::SetRMR(DWORD data)
{
	ASSERT(this);
	ASSERT(data < 0x100);
	ASSERT_DIAG();

	// �L�����������c��
	data &= 0x3f;

	if (midi.rmr != data) {
#if defined(MIDI_LOG)
		LOG1(Log::Normal, "��M�p�����[�^�ݒ� $%02X", data);
#endif	// MIDI_LOG

		// �ݒ�
		midi.rmr = data;
	}

	// ��M�p�����[�^�`�F�b�N
	if ((midi.rmr & 0x32) != 0) {
		LOG1(Log::Warning, "��M�p�����[�^�ُ� $%02X", midi.rmr);
	}
}

//---------------------------------------------------------------------------
//
//	AMR�ݒ�
//
//---------------------------------------------------------------------------
void FASTCALL MIDI::SetAMR(DWORD data)
{
	ASSERT(this);
	ASSERT(data < 0x100);
	ASSERT_DIAG();

	if (midi.amr != data) {
#if defined(MIDI_LOG)
		if (data & 0x80) {
			LOG1(Log::Normal, "��M�����[�JID+�f�o�C�XID�`�F�b�N�B���[�JID $%02X", data & 0x7f);
		}
		else {
			LOG1(Log::Normal, "��M�����[�JID�̂݃`�F�b�N�B���[�JID $%02X", data & 0x7f);
		}
#endif	// MIDI_LOG

		midi.amr = data;
	}
}

//---------------------------------------------------------------------------
//
//	ADR�ݒ�
//
//---------------------------------------------------------------------------
void FASTCALL MIDI::SetADR(DWORD data)
{
	ASSERT(this);
	ASSERT(data < 0x100);
	ASSERT_DIAG();

	if (midi.adr != data) {
#if defined(MIDI_LOG)
		if (data & 0x80) {
			LOG1(Log::Normal, "�u���[�h�L���X�g���B�f�o�C�XID $%02X", data & 0x7f);
		}
		else {
			LOG1(Log::Normal, "�u���[�h�L���X�g���B�f�o�C�XID $%02X", data & 0x7f);
		}
#endif	// MIDI_LOG

		midi.adr = data;
	}
}

//---------------------------------------------------------------------------
//
//	RSR�擾
//
//---------------------------------------------------------------------------
DWORD FASTCALL MIDI::GetRSR() const
{
	ASSERT(this);
	ASSERT_DIAG();

#if defined(MIDI_LOG)
	LOG1(Log::Normal, "��M�o�b�t�@�X�e�[�^�X�擾 $%02X", midi.rsr);
#endif	// MIDI_LOG

	return midi.rsr;
}

//---------------------------------------------------------------------------
//
//	RCR�ݒ�
//
//---------------------------------------------------------------------------
void FASTCALL MIDI::SetRCR(DWORD data)
{
	ASSERT(this);
	ASSERT(data < 0x100);
	ASSERT_DIAG();

	// �L�����������c��
	data &= 0xdf;

#if defined(MIDI_LOG)
	LOG1(Log::Normal, "��M�R���g���[�� $%02X", data);
#endif	// MIDI_LOG

	// �f�[�^�ݒ�
	midi.rcr = data;

	// �I�t���C���t���O�N���A
	if (midi.rcr & 0x04) {
		midi.rsr &= ~0x04;

		// �I�t���C�����荞�݉���
		if ((midi.imr & 4) == 0) {
			Interrupt(4, FALSE);
		}
	}

	// �u���[�N�t���O�N���A
	if (midi.rcr & 0x08) {
		midi.rsr &= ~0x08;

		// �u���[�N���荞�݉���
		if (midi.imr & 4) {
			Interrupt(4, FALSE);
		}
	}

	// �I�[�o�[�t���[�t���O�N���A
	if (midi.rcr & 0x40) {
		midi.rsr &= ~0x40;
	}

	// ��M�o�b�t�@�N���A
	if (midi.rcr & 0x80) {
		// �ʏ�o�b�t�@
		midi.stdnum = 0;
		midi.stdread = 0;
		midi.stdwrite = 0;

		// ���A���^�C����M�o�b�t�@
		midi.rrnum = 0;
		midi.rrread = 0;
		midi.rrwrite = 0;

		// ���荞�݃I�t
		Interrupt(5, FALSE);
		Interrupt(0, FALSE);
		if (midi.imr & 8) {
			Interrupt(1, FALSE);
		}
	}

	// �A�h���X�n���^�C�l�[�u��
	if (midi.rcr & 0x02) {
#if defined(MIDI_LOG)
		LOG0(Log::Normal, "�A�h���X�n���^ �C�l�[�u��");
#endif	// MIDI_LOG
		midi.rsr |= 0x02;
	}
	else {
#if defined(MIDI_LOG)
		LOG0(Log::Normal, "�A�h���X�n���^ �f�B�Z�[�u��");
#endif	// MIDI_LOG
		midi.rsr &= ~0x02;
	}

	// ��M�C�l�[�u��
	if (midi.rcr & 0x01) {
#if defined(MIDI_LOG)
		LOG0(Log::Normal, "��M�C�l�[�u��");
#endif	// MIDI_LOG
	}
	else {
#if defined(MIDI_LOG)
		LOG0(Log::Normal, "��M�f�B�Z�[�u��");
#endif	// MIDI_LOG
	}
}

//---------------------------------------------------------------------------
//
//	RDR�擾
//
//---------------------------------------------------------------------------
DWORD FASTCALL MIDI::GetRDR()
{
	DWORD data;

	ASSERT(this);
	ASSERT_DIAG();

	// �f�[�^��wdr(������)
	data = midi.wdr;

	// �o�b�t�@�ɗL���f�[�^�����邩
	if (midi.stdnum > 0) {
#if defined(MIDI_LOG)
		LOG1(Log::Normal, "��ʃo�b�t�@�擾 $%02X", midi.stdbuf[midi.stdread]);
#endif	// MIDI_LOG

		// �o�b�t�@����f�[�^�𓾂�
		data = midi.stdbuf[midi.stdread];

		// �o�b�t�@����
		midi.stdnum--;
		midi.stdread = (midi.stdread + 1) & 0x7f;

		// ���荞�݉���
		Interrupt(5, FALSE);

		// �o�b�t�@����ɂȂ��
		if (midi.stdnum == 0) {
			// ��M�o�b�t�@�L���t���O����
			midi.rsr &= 0x7f;
		}
		else {
			// ��M�o�b�t�@�L���t���O�ݒ�
			midi.rsr |= 0x80;
		}

		// �f�[�^�`�F�b�N(����m�F�p�ɁA�G�N�X�N���[�V�u�J�E���^���グ��)
		if (data == 0xf0) {
			ex_cnt[2]++;
		}
		if (data == 0xf7) {
			ex_cnt[3]++;
		}
	}

	return data;
}

//---------------------------------------------------------------------------
//
//	RDR�擾 (Read Only)
//
//---------------------------------------------------------------------------
DWORD FASTCALL MIDI::GetRDRRO() const
{
	DWORD data;

	ASSERT(this);
	ASSERT_DIAG();

	// �f�[�^��wdr(������)
	data = midi.wdr;

	// �o�b�t�@�ɗL���f�[�^�����邩
	if (midi.stdnum > 0) {
		data = midi.stdbuf[midi.stdread];
	}

	return data;
}

//---------------------------------------------------------------------------
//
//	TRR�ݒ�
//
//---------------------------------------------------------------------------
void FASTCALL MIDI::SetTRR(DWORD data)
{
	ASSERT(this);
	ASSERT(data < 0x100);
	ASSERT_DIAG();

	// �L�����������c��
	data &= 0x78;

	if (midi.trr != data) {
#if defined(MIDI_LOG)
		LOG1(Log::Normal, "���M���[�g�ݒ� $%02X", data);
#endif	// MIDI_LOG

		// �ݒ�
		midi.trr = data;
	}

	// ���M���[�g�`�F�b�N
	if ((midi.trr & 0x38) != 0x08) {
		LOG1(Log::Warning, "���M���[�g�ُ� $%02X", midi.trr);
	}

#if defined(MIDI_LOG)
	if (midi.trr & 0x40) {
		LOG0(Log::Normal, "MIDI THRU�ݒ�");
	}
#endif	// MIDI_LOG
}

//---------------------------------------------------------------------------
//
//	TMR�ݒ�
//
//---------------------------------------------------------------------------
void FASTCALL MIDI::SetTMR(DWORD data)
{
	ASSERT(this);
	ASSERT(data < 0x100);
	ASSERT_DIAG();

	// �L�����������c��
	data &= 0x3f;

	if (midi.tmr != data) {
#if defined(MIDI_LOG)
		LOG1(Log::Normal, "���M�p�����[�^�ݒ� $%02X", data);
#endif	// MIDI_LOG

		// �ݒ�
		midi.tmr = data;
	}

	// ���M�p�����[�^�`�F�b�N
	if ((midi.tmr & 0x32) != 0) {
		LOG1(Log::Warning, "���M�p�����[�^�ُ� $%02X", midi.tmr);
	}
}

//---------------------------------------------------------------------------
//
//	TSR�擾
//
//---------------------------------------------------------------------------
DWORD FASTCALL MIDI::GetTSR() const
{
	DWORD data;

	ASSERT(this);
	ASSERT_DIAG();

	// �N���A
	data = 0;

	// bit7:�ʏ�o�b�t�@��t���O
	if (midi.normnum == 0) {
		data |= 0x80;
	}

	// bit6:�ʏ�o�b�t�@�f�[�^�L���t���O
	if (midi.normnum < 16) {
		data |= 0x40;
	}

	// bit2:�����M(81.920ms)�t���O
	if (midi.tcn >= 0x100) {
		ASSERT(midi.tcn == 0x100);
		data |= 0x04;
	}

	// bit0:�g�����X�~�b�^BUSY�t���O
	if (midi.tbs) {
		data |= 0x01;
	}

	return data;
}

//---------------------------------------------------------------------------
//
//	TCR�ݒ�
//
//---------------------------------------------------------------------------
void FASTCALL MIDI::SetTCR(DWORD data)
{
	ASSERT(this);
	ASSERT(data < 0x100);
	ASSERT_DIAG();

	// �L�����������c��
	data &= 0x8d;

#if defined(MIDI_LOG)
	if (midi.tcr != data) {
		LOG1(Log::Normal, "���M�R���g���[�� $%02X", data);
	}
#endif	// MIDI_LOG

	// �L���ȃr�b�g�̂ݕۑ�
	midi.tcr = data;

	// �ʏ�o�b�t�@�A���A���^�C����p���M�o�b�t�@ �N���A
	if (data & 0x80) {
		// �ʏ�o�b�t�@
		midi.normnum = 0;
		midi.normread = 0;
		midi.normwrite = 0;

		// ���A���^�C����p���M�o�b�t�@
		midi.rtnum = 0;
		midi.rtread = 0;
		midi.rtwrite = 0;

		// ���荞�݃I��
		Interrupt(6, TRUE);
	}

	// �����M���(81.920ms)�t���O �N���A
	if (data & 0x04) {
		midi.tcn = 0;
	}

	// ���M�C�l�[�u��
#if defined(MIDI_LOG)
	if (data & 0x01) {
		LOG0(Log::Normal, "���M�C�l�[�u��");
	}
	else {
		LOG0(Log::Normal, "���M�f�B�Z�[�u��");
	}
#endif	// MIDI_LOG
}

//---------------------------------------------------------------------------
//
//	TDR�ݒ�
//
//---------------------------------------------------------------------------
void FASTCALL MIDI::SetTDR(DWORD data)
{
	ASSERT(this);
	ASSERT(data < 0x100);
	ASSERT_DIAG();

#if defined(MIDI_LOG)
	LOG1(Log::Normal, "���M�o�b�t�@�������� $%02X", data);
#endif	// MIDI_LOG

	// �f�[�^�`�F�b�N(����m�F�p�ɁA�G�N�X�N���[�V�u�J�E���^���グ��)
	if (data == 0xf0) {
		ex_cnt[0]++;
	}
	if (data == 0xf7) {
		ex_cnt[1]++;
	}

	// ���M�����`�F�b�N(���M�f�B�Z�[�u���ł��������߂�:�W�I�O���t�V�[��)
	if ((midi.trr & 0x38) != 0x08) {
		// ���M���[�g��31250bps�łȂ�
		return;
	}
	if ((midi.tmr & 0x32) != 0) {
		// 8bit�f�[�^�A1�X�g�b�v�r�b�g�A�m���p���e�B�łȂ�
		return;
	}

	// �ʏ�o�b�t�@�Ƀf�[�^��}��
	InsertNorm(data);

	// ���M�o�b�t�@�G���v�e�B���荞�݂̓N���A
	Interrupt(6, FALSE);
}

//---------------------------------------------------------------------------
//
//	FSR�擾
//
//---------------------------------------------------------------------------
DWORD FASTCALL MIDI::GetFSR() const
{
	ASSERT(this);
	ASSERT_DIAG();

#if defined(MIDI_LOG)
	LOG1(Log::Normal, "�e�[�vSYNC�X�e�[�^�X�擾 $%02X", midi.fsr);
#endif	// MIDI_LOG

	return midi.fsr;
}

//---------------------------------------------------------------------------
//
//	FCR�ݒ�
//
//---------------------------------------------------------------------------
void FASTCALL MIDI::SetFCR(DWORD data)
{
	ASSERT(this);
	ASSERT(data < 0x100);
	ASSERT_DIAG();

	// �L�����������c��
	data &= 0x9f;

	if (midi.fcr != data) {
#if defined(MIDI_LOG)
		LOG1(Log::Normal, "�e�[�vSYNC�R�}���h�ݒ� $%02X", data);
#endif	// MIDI_LOG

		// �ݒ�
		midi.fcr = data;
	}
}

//---------------------------------------------------------------------------
//
//	CCR�ݒ�
//
//---------------------------------------------------------------------------
void FASTCALL MIDI::SetCCR(DWORD data)
{
	ASSERT(this);
	ASSERT(data < 0x100);
	ASSERT_DIAG();

#if defined(MIDI_LOG)
	LOG1(Log::Normal, "�N���b�N�R���g���[�����W�X�^�ݒ� $%02X", data);
#endif	// MIDI_LOG

	// ����2bit�̂ݗL��
	data &= 0x03;

	// bit1�͕K��1
	if ((data & 2) == 0) {
		LOG0(Log::Warning, "CLKM�N���b�N���g���I���G���[");
	}

	// �ݒ�
	midi.ccr = data;
}

//---------------------------------------------------------------------------
//
//	CDR�ݒ�
//
//---------------------------------------------------------------------------
void FASTCALL MIDI::SetCDR(DWORD data)
{
	ASSERT(this);
	ASSERT(data < 0x100);
	ASSERT_DIAG();

	// CDR�ݒ�
	if (midi.cdr != (data & 0x7f)) {
		midi.cdr = (data & 0x7f);

#if defined(MIDI_LOG)
		LOG1(Log::Normal, "�N���b�N�J�E���^���萔�ݒ� $%02X", midi.cdr);
#endif	// MIDI_LOG
	}

	// bit7��CTR�ɒl�𑦎����[�h
	if (data & 0x80) {
		// CTR�ݒ�
		midi.ctr = midi.cdr;

#if defined(MIDI_LOG)
		LOG0(Log::Normal, "�N���b�N�J�E���^���萔�ݒ�����[�h");
#endif	// MIDI_LOG
	}
}

//---------------------------------------------------------------------------
//
//	SRR�擾
//
//---------------------------------------------------------------------------
DWORD FASTCALL MIDI::GetSRR() const
{
	ASSERT(this);
	ASSERT_DIAG();

#if defined(MIDI_LOG)
	LOG1(Log::Normal, "���R�[�f�B���O�J�E���^�擾 $%02X", midi.srr);
#endif	// MIDI_LOG

	return midi.srr;
}

//---------------------------------------------------------------------------
//
//	SCR�ݒ�
//
//---------------------------------------------------------------------------
void FASTCALL MIDI::SetSCR(DWORD data)
{
	DWORD mtr;
	char desc[16];

	ASSERT(this);
	ASSERT(data < 0x100);
	ASSERT_DIAG();

	// ��ԃ��[�g�ݒ�
	if (midi.scr != (data & 0x0f)) {
#if defined(MIDI_LOG)
		LOG1(Log::Normal, "MIDI�N���b�N��Ԋ�ݒ� $%02X", data & 0x0f);
#endif	// MIDI_LOG
		midi.scr = (data & 0x0f);

		// ���[�g0�͐ݒ�֎~
		if (midi.scr == 0) {
			LOG0(Log::Warning, "MIDI�N���b�N��Ԋ� �ݒ�֎~�l");
			midi.scr = 1;
		}

		// �C�x���g�Đݒ�
		if (midi.mtr <= 1) {
			mtr = 0x40000;
		}
		else {
			mtr = midi.mtr << 4;
		}
		mtr /= midi.scr;
		if (event[1].GetTime() != mtr) {
			event[1].SetTime(mtr);
		}

		// SCT���`�F�b�N
		if (midi.sct > midi.scr) {
			midi.sct = midi.scr;
		}
		ASSERT(midi.sct > 0);

		// �C�x���g������
		if (midi.scr == 1) {
			event[1].SetDesc("Clock");
		}
		else {
			sprintf(desc, "Clock (Div%d)", midi.scr);
			event[1].SetDesc(desc);
		}
	}

	// �v���C�o�b�N�J�E���^�N���A
	if (data & 0x10) {
#if defined(MIDI_LOG)
		LOG0(Log::Normal, "�v���C�o�b�N�J�E���^�N���A");
#endif	// MIDI_LOG
		midi.str = 0;
	}

	// �v���C�o�b�N�J�E���^���Z
	if (data & 0x20) {
		// ���Z
		midi.str += midi.spr;

		// �I�[�o�[�t���[�`�F�b�N
		if ((midi.str >= 0x8000) && (midi.str < 0x10000)) {
			midi.str = 0x8000;
		}

#if defined(MIDI_LOG)
		LOG2(Log::Normal, "�v���C�o�b�N�J�E���^���Z $%04X->$%04X", midi.spr, midi.str);
#endif	// MIDI_LOG
	}
}

//---------------------------------------------------------------------------
//
//	SPR�ݒ�
//
//---------------------------------------------------------------------------
void FASTCALL MIDI::SetSPR(DWORD data, BOOL high)
{
	ASSERT(this);
	ASSERT(data < 0x100);
	ASSERT_DIAG();

	if (high) {
		// ���
		midi.spr &= 0x00ff;
		midi.spr |= ((data & 0x7f) << 8);
	}
	else {
		// ����
		midi.spr &= 0xff00;
		midi.spr |= data;
	}

#if defined(MIDI_LOG)
	LOG1(Log::Normal, "�v���C�o�b�N�J�E���^���萔�ݒ� $%04X", midi.spr);
#endif	// MIDI_LOG
}

//---------------------------------------------------------------------------
//
//	GTR�ݒ�
//
//---------------------------------------------------------------------------
void FASTCALL MIDI::SetGTR(DWORD data, BOOL high)
{
	ASSERT(this);
	ASSERT(data < 0x100);
	ASSERT_DIAG();

	if (high) {
		// ���
		midi.gtr &= 0x00ff;
		midi.gtr |= ((data & 0x3f) << 8);
	}
	else {
		// ����
		midi.gtr &= 0xff00;
		midi.gtr |= data;
	}

#if defined(MIDI_LOG)
	LOG1(Log::Normal, "�ėp�^�C�}���W�X�^�ݒ� $%04X", midi.gtr);
#endif	// MIDI_LOG

	// bit7�Ńf�[�^�����[�h
	if (high && (data & 0x80)) {
#if defined(MIDI_LOG)
		LOG0(Log::Normal, "�ėp�^�C�}�ݒ�l�����[�h");
#endif	// MIDI_LOG

		// �f�[�^�����[�h���āA�^�C�}�J�n
		event[2].SetTime(midi.gtr << 4);
	}
}

//---------------------------------------------------------------------------
//
//	MTR�ݒ�
//
//---------------------------------------------------------------------------
void FASTCALL MIDI::SetMTR(DWORD data, BOOL high)
{
	DWORD mtr;

	ASSERT(this);
	ASSERT(data < 0x100);
	ASSERT_DIAG();

	if (high) {
		// ���
		midi.mtr &= 0x00ff;
		midi.mtr |= ((data & 0x3f) << 8);
	}
	else {
		// ����
		midi.mtr &= 0xff00;
		midi.mtr |= data;
	}

#if defined(MIDI_LOG)
	LOG1(Log::Normal, "MIDI�N���b�N�^�C�}���W�X�^�ݒ� $%04X", midi.mtr);
#endif	// MIDI_LOG

	// bit7�Ńf�[�^�����[�h
	if (high) {
		if (midi.mtr <= 1) {
			mtr = 0x40000;
		}
		else {
			mtr = midi.mtr << 4;
		}

		// SCR�Ŋ������l
		ASSERT(midi.scr != 0);
		event[1].SetTime(mtr / midi.scr);
	}
}

//---------------------------------------------------------------------------
//
//	EDR�ݒ�
//
//---------------------------------------------------------------------------
void FASTCALL MIDI::SetEDR(DWORD data)
{
	ASSERT(this);
	ASSERT(data < 0x100);
	ASSERT_DIAG();

	if (midi.edr != data) {
#if defined(MIDI_LOG)
		LOG1(Log::Normal, "GPIO�f�B���N�V�����ݒ� $%02X", data);
#endif	// MIDI_LOG

		midi.edr = data;
	}
}

//---------------------------------------------------------------------------
//
//	EOR�ݒ�
//
//---------------------------------------------------------------------------
void FASTCALL MIDI::SetEOR(DWORD data)
{
	ASSERT(this);
	ASSERT(data < 0x100);
	ASSERT_DIAG();

	if (midi.eor != data) {
#if defined(MIDI_LOG)
		LOG1(Log::Normal, "GPIO�o�̓f�[�^�ݒ� $%02X", data);
#endif	// MIDI_LOG

		midi.eor = data;
	}
}

//---------------------------------------------------------------------------
//
//	EIR�擾
//
//---------------------------------------------------------------------------
DWORD FASTCALL MIDI::GetEIR() const
{
	ASSERT(this);
	ASSERT_DIAG();

#if defined(MIDI_LOG)
	LOG1(Log::Normal, "GPIO�ǂݏo�� $%02X", midi.eir);
#endif	// MIDI_LOG

	return midi.eir;
}

//---------------------------------------------------------------------------
//
//	���A���^�C����M�o�b�t�@�`�F�b�N
//
//---------------------------------------------------------------------------
void FASTCALL MIDI::CheckRR()
{
	DWORD data;

	ASSERT(this);
	ASSERT_DIAG();

	// ��M�o�b�t�@�Ƀf�[�^���Ȃ����
	if (midi.rrnum == 0) {
		// ���荞�݃I�t
		Interrupt(0, FALSE);
		if (midi.imr & 8) {
			Interrupt(1, FALSE);
		}
		return;
	}

	// �ł��Â��f�[�^�𓾂�
	data = midi.rrbuf[midi.rrread];

	// IRQ-1
	if (data == 0xf8) {
		if (midi.imr & 8) {
			Interrupt(1, TRUE);
			Interrupt(0, FALSE);
			return;
		}
	}

	// IRQ-0
	Interrupt(1, FALSE);
	if ((data >= 0xf9) && (data <= 0xfd)) {
		Interrupt(0, TRUE);
	}
	else {
		Interrupt(0, FALSE);
	}
}

//---------------------------------------------------------------------------
//
//	���荞�ݗv��
//
//---------------------------------------------------------------------------
void FASTCALL MIDI::Interrupt(int type, BOOL flag)
{
	ASSERT(this);
	ASSERT((type >= 0) && (type <= 7));
	ASSERT_DIAG();

	// ISR�̊Y������r�b�g��ω�������
	if (flag) {
		midi.isr |= (1 << type);
	}
	else {
		midi.isr &= ~(1 << type);
	}

	// ���荞�݃`�F�b�N
	IntCheck();
}

//---------------------------------------------------------------------------
//
//	���荞��ACK
//
//---------------------------------------------------------------------------
void FASTCALL MIDI::IntAck(int level)
{
	ASSERT(this);
	ASSERT((level == 2) || (level == 4));
	ASSERT_DIAG();

	// ���荞�݃��x������v���Ȃ���Ή������Ȃ�
	if (level != (int)midi.ilevel) {
#if defined(MIDI_LOG)
		LOG1(Log::Normal, "���荞�݃��x���ُ� ���x��%d", level);
#endif	// MIDI_LOG
		return;
	}

	// ���荞�ݗv�����łȂ���΁A�������Ȃ�
	if (midi.vector < 0) {
#if defined(MIDI_LOG)
		LOG0(Log::Normal, "�v�����Ă��Ȃ����荞��");
#endif	// MIDI_LOG
		return;
	}

#if defined(MIDI_LOG)
	LOG1(Log::Normal, "���荞��ACK ���x��%d", level);
#endif	// MIDI_LOG

	// �v���x�N�^�Ȃ�
	midi.vector = -1;

	// ���̊��荞�݂��X�P�W���[��
	IntCheck();
}

//---------------------------------------------------------------------------
//
//	���荞�݃`�F�b�N
//
//---------------------------------------------------------------------------
void FASTCALL MIDI::IntCheck()
{
	int i;
	int vector;
	DWORD bit;

	ASSERT(this);
	ASSERT_DIAG();

	// ���荞�݃��[�h�R���g���[��bit1=1�łȂ��ƁA68000���荞�݂ɓK�����Ȃ�
	if ((midi.imr & 0x02) == 0) {
		// ���荞�݂�����΃L�����Z������
		if (midi.vector != -1) {
			cpu->IntCancel(midi.ilevel);
			midi.ivr &= 0xe0;
			midi.ivr |= 0x10;
			midi.vector = -1;
		}
		return;
	}

	// ISR���`�F�b�N
	bit = 0x80;
	for (i=7; i>=0; i--) {
		if (midi.isr & bit) {
			if (midi.ier & bit) {
				// ���荞�ݔ����B�x�N�^�Z�o
				vector = midi.ivr;
				vector &= 0xe0;
				vector += (i << 1);

				// ���݂̂��̂ƈ������A�v��
				if (midi.vector != vector) {
					// ���ɑ��̂��̂��v������Ă���΁A�L�����Z������
					if (midi.vector >= 0) {
						cpu->IntCancel(midi.ilevel);
					}
					// ���荞�ݗv��
					cpu->Interrupt(midi.ilevel, vector);
					midi.vector = vector;
					midi.ivr = (DWORD)vector;

#if defined(MIDI_LOG)
					LOG3(Log::Normal, "���荞�ݗv�� ���x��%d �^�C�v%d �x�N�^$%02X", midi.ilevel, i, vector);
#endif	// MIDI_LOG
				}
				return;
			}
		}

		// ����
		bit >>= 1;
	}

	// ���������銄�荞�݂͂Ȃ��̂ŁA�L�����Z��
	if (midi.vector != -1) {
		// ���荞�݃L�����Z��
		cpu->IntCancel(midi.ilevel);
		midi.vector = -1;
		midi.ivr &= 0xe0;
		midi.ivr |= 0x10;
#if defined(MIDI_LOG)
		LOG0(Log::Normal, "���荞�݃L�����Z��");
#endif	// MIDI_LOG
	}
}

//---------------------------------------------------------------------------
//
//	�����f�[�^�擾
//
//---------------------------------------------------------------------------
void FASTCALL MIDI::GetMIDI(midi_t *buffer) const
{
	ASSERT(this);
	ASSERT(buffer);
	ASSERT_DIAG();

	// �����f�[�^���R�s�[
	*buffer = midi;
}

//---------------------------------------------------------------------------
//
//	�G�N�X�N���[�V�u�J�E���g�擾
//
//---------------------------------------------------------------------------
DWORD FASTCALL MIDI::GetExCount(int index) const
{
	ASSERT(this);
	ASSERT((index >= 0) && (index < 4));
	ASSERT_DIAG();

	return ex_cnt[index];
}
