//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 ‚o‚hD(ytanaka@ipc-tokai.or.jp)
//	[ FDC(uPD72065) ]
//
//---------------------------------------------------------------------------

#include "os.h"
#include "xm6.h"
#include "fdd.h"
#include "iosc.h"
#include "dmac.h"
#include "schedule.h"
#include "vm.h"
#include "log.h"
#include "fileio.h"
#include "config.h"
#include "fdc.h"

//===========================================================================
//
//	FDC
//
//===========================================================================
//#define FDC_LOG

//---------------------------------------------------------------------------
//
//	ƒRƒ“ƒXƒgƒ‰ƒNƒ^
//
//---------------------------------------------------------------------------
FDC::FDC(VM *p) : MemDevice(p)
{
	// ƒfƒoƒCƒXID‚ğ‰Šú‰»
	dev.id = MAKEID('F', 'D', 'C', ' ');
	dev.desc = "FDC (uPD72065)";

	// ŠJnƒAƒhƒŒƒXAI—¹ƒAƒhƒŒƒX
	memdev.first = 0xe94000;
	memdev.last = 0xe95fff;

	// ƒIƒuƒWƒFƒNƒg
	iosc = NULL;
	dmac = NULL;
	fdd = NULL;
}

//---------------------------------------------------------------------------
//
//	‰Šú‰»
//
//---------------------------------------------------------------------------
BOOL FASTCALL FDC::Init()
{
	ASSERT(this);

	// Šî–{ƒNƒ‰ƒX
	if (!MemDevice::Init()) {
		return FALSE;
	}

	// IOSCæ“¾
	iosc = (IOSC*)vm->SearchDevice(MAKEID('I', 'O', 'S', 'C'));
	ASSERT(iosc);

	// DMACæ“¾
	dmac = (DMAC*)vm->SearchDevice(MAKEID('D', 'M', 'A', 'C'));
	ASSERT(dmac);

	// FDDæ“¾
	fdd = (FDD*)vm->SearchDevice(MAKEID('F', 'D', 'D', ' '));
	ASSERT(fdd);

	// ƒCƒxƒ“ƒg‰Šú‰»
	event.SetDevice(this);
	event.SetDesc("Data Transfer");
	event.SetUser(0);
	event.SetTime(0);
	scheduler->AddEvent(&event);

	// ‚‘¬ƒ‚[ƒhƒtƒ‰ƒOAƒfƒ…ƒAƒ‹ƒhƒ‰ƒCƒuƒtƒ‰ƒO(ApplyCfg)
	fdc.fast = FALSE;
	fdc.dual = FALSE;

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	ƒNƒŠ[ƒ“ƒAƒbƒv
//
//---------------------------------------------------------------------------
void FASTCALL FDC::Cleanup()
{
	ASSERT(this);

	// Šî–{ƒNƒ‰ƒX‚Ö
	MemDevice::Cleanup();
}

//---------------------------------------------------------------------------
//
//	ƒŠƒZƒbƒg
//
//---------------------------------------------------------------------------
void FASTCALL FDC::Reset()
{
	ASSERT(this);
	LOG0(Log::Normal, "ƒŠƒZƒbƒg");

	// ƒf[ƒ^ƒŒƒWƒXƒ^EƒXƒe[ƒ^ƒXƒŒƒWƒXƒ^
	fdc.dr = 0;
	fdc.sr = 0;
	fdc.sr |= sr_rqm;
	fdc.sr &= ~sr_dio;
	fdc.sr &= ~sr_ndm;
	fdc.sr &= ~sr_cb;

	// ƒhƒ‰ƒCƒuƒZƒŒƒNƒgƒŒƒWƒXƒ^EST0-ST3
	fdc.dcr = 0;
	fdc.dsr = 0;
	fdc.st[0] = 0;
	fdc.st[1] = 0;
	fdc.st[2] = 0;
	fdc.st[3] = 0;

	// ƒRƒ}ƒ“ƒh‹¤’Êƒpƒ‰ƒ[ƒ^
	fdc.srt = 1 * 2000;
	fdc.hut = 16 * 2000;
	fdc.hlt = 2 * 2000;
	fdc.hd = 0;
	fdc.us = 0;
	fdc.cyl[0] = 0;
	fdc.cyl[1] = 0;
	fdc.cyl[2] = 0;
	fdc.cyl[3] = 0;
	fdc.chrn[0] = 0;
	fdc.chrn[1] = 0;
	fdc.chrn[2] = 0;
	fdc.chrn[3] = 0;

	// ‚»‚Ì‘¼
	fdc.eot = 0;
	fdc.gsl = 0;
	fdc.dtl = 0;
	fdc.sc = 0;
	fdc.gpl = 0;
	fdc.d = 0;
	fdc.err = 0;
	fdc.seek = FALSE;
	fdc.ndm = FALSE;
	fdc.mfm = FALSE;
	fdc.mt = FALSE;
	fdc.sk = FALSE;
	fdc.tc = FALSE;
	fdc.load = FALSE;

	// “]‘—Œn
	fdc.offset = 0;
	fdc.len = 0;
	memset(fdc.buffer, 0, sizeof(fdc.buffer));

	// ƒtƒF[ƒYAƒRƒ}ƒ“ƒh
	fdc.phase = idle;
	fdc.cmd = no_cmd;

	// ƒpƒPƒbƒgŠÇ—
	fdc.in_len = 0;
	fdc.in_cnt = 0;
	memset(fdc.in_pkt, 0, sizeof(fdc.in_pkt));
	fdc.out_len = 0;
	fdc.out_cnt = 0;
	memset(fdc.out_pkt, 0, sizeof(fdc.out_pkt));

	// ƒCƒxƒ“ƒg’â~
	event.SetTime(0);

	// ƒAƒNƒZƒX’â~(FDD‚àƒŠƒZƒbƒg‚Åfdd.selected=0‚Æ‚È‚é)
	fdd->Access(FALSE);
}

//---------------------------------------------------------------------------
//
//	ƒ\ƒtƒgƒEƒFƒAƒŠƒZƒbƒg
//
//---------------------------------------------------------------------------
void FASTCALL FDC::SoftReset()
{
	// “à•”ƒŒƒWƒXƒ^(FDC)
	fdc.dr = 0;
	fdc.sr = 0;
	fdc.sr |= sr_rqm;
	fdc.sr &= ~sr_dio;
	fdc.sr &= ~sr_ndm;
	fdc.sr &= ~sr_cb;

	fdc.st[0] = 0;
	fdc.st[1] = 0;
	fdc.st[2] = 0;
	fdc.st[3] = 0;

	fdc.srt = 1 * 2000;
	fdc.hut = 16 * 2000;
	fdc.hlt = 2 * 2000;
	fdc.hd = 0;
	fdc.us = 0;
	fdc.cyl[0] = 0;
	fdc.cyl[1] = 0;
	fdc.cyl[2] = 0;
	fdc.cyl[3] = 0;
	fdc.chrn[0] = 0;
	fdc.chrn[1] = 0;
	fdc.chrn[2] = 0;
	fdc.chrn[3] = 0;

	fdc.eot = 0;
	fdc.gsl = 0;
	fdc.dtl = 0;
	fdc.sc = 0;
	fdc.gpl = 0;
	fdc.d = 0;
	fdc.err = 0;
	fdc.seek = FALSE;
	fdc.ndm = FALSE;
	fdc.mfm = FALSE;
	fdc.mt = FALSE;
	fdc.sk = FALSE;
	fdc.tc = FALSE;
	fdc.load = FALSE;

	fdc.offset = 0;
	fdc.len = 0;

	// ƒtƒF[ƒYAƒRƒ}ƒ“ƒh
	fdc.phase = idle;
	fdc.cmd = fdc_reset;

	// ƒpƒPƒbƒgŠÇ—
	fdc.in_len = 0;
	fdc.in_cnt = 0;
	memset(fdc.in_pkt, 0, sizeof(fdc.in_pkt));
	fdc.out_len = 0;
	fdc.out_cnt = 0;
	memset(fdc.out_pkt, 0, sizeof(fdc.out_pkt));

	// ƒCƒxƒ“ƒg’â~
	event.SetTime(0);

	// ƒAƒNƒZƒX’â~
	fdd->Access(FALSE);
}

//---------------------------------------------------------------------------
//
//	ƒZ[ƒu
//
//---------------------------------------------------------------------------
BOOL FASTCALL FDC::Save(Fileio *fio, int ver)
{
	size_t sz;

	ASSERT(this);
	ASSERT(fio);

	LOG0(Log::Normal, "ƒZ[ƒu");

	// ƒTƒCƒY‚ğƒZ[ƒu
	sz = sizeof(fdc_t);
	if (!fio->Write(&sz, sizeof(sz))) {
		return FALSE;
	}

	// –{‘Ì‚ğƒZ[ƒu
	if (!fio->Write(&fdc, (int)sz)) {
		return FALSE;
	}

	// ƒCƒxƒ“ƒg‚ğƒZ[ƒu
	if (!event.Save(fio, ver)) {
		return FALSE;
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	ƒ[ƒh
//
//---------------------------------------------------------------------------
BOOL FASTCALL FDC::Load(Fileio *fio, int ver)
{
	size_t sz;

	ASSERT(this);
	ASSERT(fio);

	LOG0(Log::Normal, "ƒ[ƒh");

	// ƒTƒCƒY‚ğƒ[ƒhA”äŠr
	if (!fio->Read(&sz, sizeof(sz))) {
		return FALSE;
	}
	if (sz != sizeof(fdc_t)) {
		return FALSE;
	}

	// –{‘Ì‚ğƒ[ƒh
	if (!fio->Read(&fdc, (int)sz)) {
		return FALSE;
	}

	// ƒCƒxƒ“ƒg‚ğƒ[ƒh
	if (!event.Load(fio, ver)) {
		return FALSE;
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	İ’è“K—p
//
//---------------------------------------------------------------------------
void FASTCALL FDC::ApplyCfg(const Config *config)
{
	ASSERT(this);
	ASSERT(config);

	LOG0(Log::Normal, "İ’è“K—p");

	// ‚‘¬ƒ‚[ƒh
	fdc.fast = config->floppy_speed;
#if defined(FDC_LOG)
	if (fdc.fast) {
		LOG0(Log::Normal, "‚‘¬ƒ‚[ƒh ON");
	}
	else {
		LOG0(Log::Normal, "‚‘¬ƒ‚[ƒh OFF");
	}
#endif	// FDC_LOG

	// 2DD/2HDŒ“—pƒhƒ‰ƒCƒu
	fdc.dual = config->dual_fdd;
#if defined(FDC_LOG)
	if (fdc.dual) {
		LOG0(Log::Normal, "2DD/2HDŒ“—pƒhƒ‰ƒCƒu");
	}
	else {
		LOG0(Log::Normal, "2HDê—pƒhƒ‰ƒCƒu");
	}
#endif	// FDC_LOG
}

//---------------------------------------------------------------------------
//
//	ƒoƒCƒg“Ç‚İ‚İ
//
//---------------------------------------------------------------------------
DWORD FASTCALL FDC::ReadByte(DWORD addr)
{
	int i;
	int status;
	DWORD bit;
	DWORD data;

	ASSERT(this);
	ASSERT((addr >= memdev.first) && (addr <= memdev.last));

	// Šï”ƒAƒhƒŒƒX‚Ì‚İƒfƒR[ƒh‚³‚ê‚Ä‚¢‚é
	if ((addr & 1) == 0) {
		return 0xff;
	}

	// 8ƒoƒCƒg’PˆÊ‚Åƒ‹[ƒv
	addr &= 0x07;
	addr >>= 1;

	// ƒEƒFƒCƒg
	scheduler->Wait(1);

	switch (addr) {
		// ƒXƒe[ƒ^ƒXƒŒƒWƒXƒ^
		case 0:
			return fdc.sr;

		// ƒf[ƒ^ƒŒƒWƒXƒ^
		case 1:
			// SEEKŠ®—¹Š„‚è‚İ‚Å‚È‚¯‚ê‚ÎAŠ„‚è‚İƒlƒQ[ƒg
			if (!fdc.seek) {
				Interrupt(FALSE);
			}

			switch (fdc.phase) {
				// ÀsƒtƒF[ƒY(ER)
				case read:
					fdc.sr &= ~sr_rqm;
					return Read();

				// ƒŠƒUƒ‹ƒgƒtƒF[ƒY
				case result:
					ASSERT(fdc.out_cnt >= 0);
					ASSERT(fdc.out_cnt < 0x10);
					ASSERT(fdc.out_len > 0);

					// ƒpƒPƒbƒg‚©‚çƒf[ƒ^‚ğæ‚èo‚·
					data = fdc.out_pkt[fdc.out_cnt];
					fdc.out_cnt++;
					fdc.out_len--;

					// c‚èƒŒƒ“ƒOƒX‚ª0‚É‚È‚Á‚½‚çAƒAƒCƒhƒ‹ƒtƒF[ƒY‚Ö
					if (fdc.out_len == 0) {
						Idle();
					}
					return data;
			}
			LOG0(Log::Warning, "FDCƒf[ƒ^ƒŒƒWƒXƒ^“Ç‚İ‚İ–³Œø");
			return 0xff;

		// ƒhƒ‰ƒCƒuƒXƒe[ƒ^ƒXƒŒƒWƒXƒ^
		case 2:
			data = 0;
			bit = 0x08;
			for (i=3; i>=0; i--) {
				// DCR‚Ìƒrƒbƒg‚ª—§‚Á‚Ä‚¢‚é‚©
				if ((fdc.dcr & bit) != 0) {
					// ŠY“–ƒhƒ‰ƒCƒu‚ÌƒXƒe[ƒ^ƒX‚ğOR(b7,b6‚Ì‚İ)
					status = fdd->GetStatus(i);
					data |= (DWORD)(status & 0xc0);
				}
				bit >>= 1;
			}

			// FDDŠ„‚è‚İ‚ğ—‚Æ‚·(FDCŠ„‚è‚İ‚Å‚Í‚È‚¢A’ˆÓ)
			iosc->IntFDD(FALSE);
			return data;

		// ƒhƒ‰ƒCƒuƒZƒŒƒNƒgƒŒƒWƒXƒ^
		case 3:
			LOG0(Log::Warning, "ƒhƒ‰ƒCƒuƒZƒŒƒNƒgƒŒƒWƒXƒ^“Ç‚İ‚İ");
			return 0xff;
	}

	// ’ÊíA‚±‚±‚É‚Í‚±‚È‚¢
	ASSERT(FALSE);
	return 0xff;
}

//---------------------------------------------------------------------------
//
//	ƒ[ƒh“Ç‚İ‚İ
//
//---------------------------------------------------------------------------
DWORD FASTCALL FDC::ReadWord(DWORD addr)
{
	ASSERT(this);
	ASSERT((addr >= memdev.first) && (addr <= memdev.last));
	ASSERT((addr & 1) == 0);

	return (0xff00 | ReadByte(addr + 1));
}

//---------------------------------------------------------------------------
//
//	ƒoƒCƒg‘‚«‚İ
//
//---------------------------------------------------------------------------
void FASTCALL FDC::WriteByte(DWORD addr, DWORD data)
{
	int i;
	DWORD bit;

	ASSERT(this);
	ASSERT((addr >= memdev.first) && (addr <= memdev.last));

	// Šï”ƒAƒhƒŒƒX‚Ì‚İƒfƒR[ƒh‚³‚ê‚Ä‚¢‚é
	if ((addr & 1) == 0) {
		return;
	}

	// 8ƒoƒCƒg’PˆÊ‚Åƒ‹[ƒv
	addr &= 0x07;
	addr >>= 1;

	// ƒEƒFƒCƒg
	scheduler->Wait(1);

	switch (addr) {
		// “ÁêƒRƒ}ƒ“ƒhƒŒƒWƒXƒ^
		case 0:
			switch (data) {
				// RESET STANDBY
				case 0x34:
#if defined(FDC_LOG)
					LOG0(Log::Normal, "RESET STANDBYƒRƒ}ƒ“ƒh");
#endif	// FDC_LOG
					fdc.cmd = reset_stdby;
					Result();
					return;
				// SET STANDBY
				case 0x35:
#if defined(FDC_LOG)
					LOG0(Log::Normal, "SET STANDBYƒRƒ}ƒ“ƒh");
#endif	// FDC_LOG
					fdc.cmd = set_stdby;
					Idle();
					return;
				// SOFTWARE RESET
				case 0x36:
#if defined(FDC_LOG)
					LOG0(Log::Normal, "SOFTWARE RESETƒRƒ}ƒ“ƒh");
#endif	// FDC_LOG
					SoftReset();
					return;
			}
			LOG1(Log::Warning, "–³Œø‚È“ÁêƒRƒ}ƒ“ƒh‘‚«‚İ %02X", data);
			return;

		// ƒf[ƒ^ƒŒƒWƒXƒ^
		case 1:
			// SEEKŠ®—¹Š„‚è‚İ‚Å‚È‚¯‚ê‚ÎAŠ„‚è‚İƒlƒQ[ƒg
			if (!fdc.seek) {
				Interrupt(FALSE);
			}

			switch (fdc.phase) {
				// ƒAƒCƒhƒ‹ƒtƒF[ƒY
				case idle:
					Command(data);
					return;

				// ƒRƒ}ƒ“ƒhƒtƒF[ƒY
				case command:
					ASSERT(fdc.in_cnt >= 0);
					ASSERT(fdc.in_cnt < 0x10);
					ASSERT(fdc.in_len > 0);

					// ƒpƒPƒbƒg‚Éƒf[ƒ^‚ğƒZƒbƒg
					fdc.in_pkt[fdc.in_cnt] = data;
					fdc.in_cnt++;
					fdc.in_len--;

					// c‚èƒŒƒ“ƒOƒX‚ª0‚É‚È‚Á‚½‚çAÀsƒtƒF[ƒY‚Ö
					if (fdc.in_len == 0) {
						Execute();
					}
					return;

				// ÀsƒtƒF[ƒY(EW)
				case write:
					fdc.sr &= ~sr_rqm;
					Write(data);
					return;
			}
			LOG1(Log::Warning, "FDCƒf[ƒ^ƒŒƒWƒXƒ^‘‚«‚İ–³Œø $%02X", data);
			return;

		// ƒhƒ‰ƒCƒuƒRƒ“ƒgƒ[ƒ‹ƒŒƒWƒXƒ^
		case 2:
			// ‰ºˆÊ4bit‚ª1¨0‚É‚È‚Á‚½‚Æ‚±‚ë‚ğ’²‚×‚é
			bit = 0x01;
			for (i=0; i<4; i++) {
				if ((fdc.dcr & bit) != 0) {
					if ((data & bit) == 0) {
						// 1¨0‚ÌƒGƒbƒW‚ÅADCR‚ÌãˆÊ4ƒrƒbƒg‚ğ“K—p
						fdd->Control(i, fdc.dcr);
					}
				}
				bit <<= 1;
			}

			// ’l‚ğ•Û‘¶
			fdc.dcr = data;
			return;

		// ƒhƒ‰ƒCƒuƒZƒŒƒNƒgƒŒƒWƒXƒ^
		case 3:
			// ‰ºˆÊ2bit‚ÅƒAƒNƒZƒXƒhƒ‰ƒCƒu‘I‘ğ
			fdc.dsr = (DWORD)(data & 0x03);

			// ÅãˆÊ‚Åƒ‚[ƒ^§Œä
			if (data & 0x80) {
				fdd->SetMotor(fdc.dsr, TRUE);
			}
			else {
				fdd->SetMotor(fdc.dsr, FALSE);
			}

			// 2HD/2DDØ‚è‘Ö‚¦(ƒhƒ‰ƒCƒu‚ª2DD‘Î‰‚Å‚È‚¯‚ê‚Î–³Œø)
			if (fdc.dual) {
				if (data & 0x10) {
					fdd->SetHD(FALSE);
				}
				else {
					fdd->SetHD(TRUE);
				}
			}
			else {
				fdd->SetHD(TRUE);
			}
			return;
	}

	// ’ÊíA‚±‚±‚É‚Í‚±‚È‚¢
	ASSERT(FALSE);
}

//---------------------------------------------------------------------------
//
//	ƒ[ƒh‘‚«‚İ
//
//---------------------------------------------------------------------------
void FASTCALL FDC::WriteWord(DWORD addr, DWORD data)
{
	ASSERT(this);
	ASSERT((addr >= memdev.first) && (addr <= memdev.last));
	ASSERT((addr & 1) == 0);

	WriteByte(addr + 1, (BYTE)data);
}

//---------------------------------------------------------------------------
//
//	“Ç‚İ‚İ‚Ì‚İ
//
//---------------------------------------------------------------------------
DWORD FASTCALL FDC::ReadOnly(DWORD addr) const
{
	int i;
	int status;
	DWORD bit;
	DWORD data;

	ASSERT(this);
	ASSERT((addr >= memdev.first) && (addr <= memdev.last));

	// Šï”ƒAƒhƒŒƒX‚Ì‚İƒfƒR[ƒh‚³‚ê‚Ä‚¢‚é
	if ((addr & 1) == 0) {
		return 0xff;
	}

	// 8ƒoƒCƒg’PˆÊ‚Åƒ‹[ƒv
	addr &= 0x07;
	addr >>= 1;

	switch (addr) {
		// ƒXƒe[ƒ^ƒXƒŒƒWƒXƒ^
		case 0:
			return fdc.sr;

		// ƒf[ƒ^ƒŒƒWƒXƒ^
		case 1:
			if (fdc.phase == result) {
				// ƒpƒPƒbƒg‚©‚çƒf[ƒ^‚ğæ‚èo‚·(XV‚µ‚È‚¢);
				return fdc.out_pkt[fdc.out_cnt];
			}
			return 0xff;

		// ƒhƒ‰ƒCƒuƒXƒe[ƒ^ƒXƒŒƒWƒXƒ^
		case 2:
			data = 0;
			bit = 0x08;
			for (i=3; i>=0; i--) {
				// DCR‚Ìƒrƒbƒg‚ª—§‚Á‚Ä‚¢‚é‚©
				if ((fdc.dcr & bit) != 0) {
					// ŠY“–ƒhƒ‰ƒCƒu‚ÌƒXƒe[ƒ^ƒX‚ğOR(b7,b6‚Ì‚İ)
					status = fdd->GetStatus(i);
					data |= (DWORD)(status & 0xc0);
				}
				bit >>= 1;
			}
			return data;

		// ƒhƒ‰ƒCƒuƒZƒŒƒNƒgƒŒƒWƒXƒ^
		case 3:
			return 0xff;
	}

	return 0xff;
}

//---------------------------------------------------------------------------
//
//	ƒCƒxƒ“ƒgƒR[ƒ‹ƒoƒbƒN
//
//---------------------------------------------------------------------------
BOOL FASTCALL FDC::Callback(Event *ev)
{
	int i;
	int thres;

	ASSERT(this);
	ASSERT(ev);

	// ƒAƒCƒhƒ‹ƒtƒF[ƒY‚ÍƒwƒbƒhƒAƒ“ƒ[ƒh
	if (fdc.phase == idle) {
		fdc.load = FALSE;

		// ’P”­
		return FALSE;
	}

	// ƒwƒbƒhƒ[ƒh
	fdc.load = TRUE;

	// ÀsƒtƒF[ƒY
	if (fdc.phase == execute) {
		// ID‚à‚µ‚­‚ÍNO DATA‚ğŒ©‚Â‚¯‚é‚Ü‚Å‚ÌŠÔ
		Result();

		// ’P”­
		return FALSE;
	}

	// Write ID‚Íê—pˆ—
	if (fdc.cmd == write_id) {
		ASSERT(fdc.len > 0);
		ASSERT((fdc.len & 3) == 0);

		// ŠÔÄİ’è
		if (fdc.fast) {
			ev->SetTime(32 * 4);
		}
		else {
			ev->SetTime(fdd->GetRotationTime() / fdc.sc);
		}

		fdc.sr |= sr_rqm;
		dmac->ReqDMA(0);
		fdc.sr |= sr_rqm;
		dmac->ReqDMA(0);
		fdc.sr |= sr_rqm;
		dmac->ReqDMA(0);
		fdc.sr |= sr_rqm;
		dmac->ReqDMA(0);
		return TRUE;
	}

	// Read(Del)Data/Write(Del)Data/Scan/ReadDiagBŠÔÄİ’è
	EventRW();

	// ƒf[ƒ^“]‘—ƒŠƒNƒGƒXƒg
	fdc.sr |= sr_rqm;

	// ƒf[ƒ^“]‘—
	if (!fdc.ndm) {
		// DMAƒ‚[ƒh(DMAƒŠƒNƒGƒXƒg)B64ƒoƒCƒg‚Ü‚Æ‚ß‚Äs‚¤
		if (fdc.fast) {
			// 1‰ñ‚ÌƒCƒxƒ“ƒg‚ÅA—]èCPUƒpƒ[‚Ì2/3‚¾‚¯“]‘—‚·‚é
			thres = (int)scheduler->GetCPUSpeed();
			thres <<= 1;
			thres /= 3;

			// ƒŠƒUƒ‹ƒgƒtƒF[ƒY‚É“ü‚é‚Ü‚ÅŒJ‚è•Ô‚·
			while (fdc.phase != result) {
				// CPUƒpƒ[‚ğŒ©‚È‚ª‚ç“r’†‚Å‘Å‚¿Ø‚é
				if (scheduler->GetCPUCycle() > thres) {
					break;
				}

				// “]‘—
				dmac->ReqDMA(0);
			}
		}
		else {
			// ’ÊíB64ƒoƒCƒg‚Ü‚Æ‚ß‚Ä
			for (i=0; i<64; i++) {
				if (fdc.phase == result) {
					break;
				}
				dmac->ReqDMA(0);
			}
		}
		return TRUE;
	}

	// Non-DMAƒ‚[ƒh(Š„‚è‚İƒŠƒNƒGƒXƒg)
	Interrupt(TRUE);
	return TRUE;
}

//---------------------------------------------------------------------------
//
//	“à•”ƒ[ƒNƒAƒhƒŒƒXæ“¾
//
//---------------------------------------------------------------------------
const FDC::fdc_t* FASTCALL FDC::GetWork() const
{
	ASSERT(this);

	// ƒAƒhƒŒƒX‚ğ—^‚¦‚é(fdc.buffer‚ª‘å‚«‚¢‚½‚ß)
	return &fdc;
}

//---------------------------------------------------------------------------
//
//	ƒV[ƒNŠ®—¹
//
//---------------------------------------------------------------------------
void FASTCALL FDC::CompleteSeek(int drive, BOOL status)
{
	ASSERT(this);
	ASSERT((drive >= 0) && (drive <= 3));

#if defined(FDC_LOG)
	if (status) {
		LOG2(Log::Normal, "ƒV[ƒN¬Œ÷ ƒhƒ‰ƒCƒu%d ƒVƒŠƒ“ƒ_%02X",
					drive, fdd->GetCylinder(drive));
	}
	else {
		LOG2(Log::Normal, "ƒV[ƒN¸”s ƒhƒ‰ƒCƒu%d ƒVƒŠƒ“ƒ_%02X",
					drive, fdd->GetCylinder(drive));
	}
#endif	// FDC_LOG

	// recalibrate‚Ü‚½‚Íseek‚Ì‚İ—LŒø
	if ((fdc.cmd == recalibrate) || (fdc.cmd == seek)) {
		// ST0ì¬(‚½‚¾‚µUS‚Ì‚İ)
		fdc.st[0] = fdc.us;

		// ƒXƒe[ƒ^ƒX”»•Ê
		if (status) {
			// ƒhƒ‰ƒCƒu2,3‚ÍEC‚ğ—§‚Ä‚é
			if (drive <= 1) {
				// Seek End
				fdc.st[0] |= 0x20;
			}
			else {
				// Equipment Check, Attention Interrupt
				fdc.st[0] |= 0x10;
				fdc.st[0] |= 0xc0;
			}
		}
		else {
			if (drive <= 1) {
				// Seek End
				fdc.st[0] |= 0x20;
			}
			// Not Ready, Abnormal Terminate
			fdc.st[0] |= 0x08;
			fdc.st[0] |= 0x40;
		}

		// SEEKŠ®—¹Š„‚è‚İ
		Interrupt(TRUE);
		fdc.seek = TRUE;
		Idle();
		return;
	}

	LOG1(Log::Warning, "–³Œø‚ÈƒV[ƒNŠ®—¹’Ê’m ƒhƒ‰ƒCƒu%d", drive);
}

//---------------------------------------------------------------------------
//
//	TCƒAƒT[ƒg
//
//---------------------------------------------------------------------------
void FASTCALL FDC::SetTC()
{
	ASSERT(this);

	// ƒAƒCƒhƒ‹ƒtƒF[ƒY‚ÅƒNƒŠƒA‚·‚é‚½‚ßAƒAƒCƒhƒ‹ƒtƒF[ƒYˆÈŠO‚È‚ç‰Â
	if (fdc.phase != idle) {
		fdc.tc = TRUE;
	}
}

//---------------------------------------------------------------------------
//
//	ƒAƒCƒhƒ‹ƒtƒF[ƒY
//
//---------------------------------------------------------------------------
void FASTCALL FDC::Idle()
{
	ASSERT(this);

	// ƒtƒF[ƒYİ’è
	fdc.phase = idle;
	fdc.err = 0;
	fdc.tc = FALSE;

	// ƒCƒxƒ“ƒgI—¹
	event.SetTime(0);

	// ƒwƒbƒhƒ[ƒhó‘Ô‚È‚çAƒAƒ“ƒ[ƒh‚Ì‚½‚ß‚ÌƒCƒxƒ“ƒg‚ğİ’è
	if (fdc.load) {
		// ƒAƒ“ƒ[ƒh‚Ì•K—v‚ ‚è
		if (fdc.hut > 0) {
			event.SetTime(fdc.hut);
		}
	}

	// ƒXƒe[ƒ^ƒXƒŒƒWƒXƒ^‚ÍƒRƒ}ƒ“ƒh‘Ò‚¿
	fdc.sr = sr_rqm;

	// ƒAƒNƒZƒXI—¹
	fdd->Access(FALSE);
}

//---------------------------------------------------------------------------
//
//	ƒRƒ}ƒ“ƒhƒtƒF[ƒY
//
//---------------------------------------------------------------------------
void FASTCALL FDC::Command(DWORD data)
{
	DWORD mask;

	ASSERT(this);
	ASSERT(data < 0x100);

	// ƒRƒ}ƒ“ƒhƒtƒF[ƒY(FDC BUSY)
	fdc.phase = command;
	fdc.sr |= sr_cb;

	// “ü—ÍƒpƒPƒbƒg‰Šú‰»
	fdc.in_pkt[0] = data;
	fdc.in_cnt = 1;
	fdc.in_len = 0;

	// ƒ}ƒXƒN(1)
	mask = data;

	// FDCƒŠƒZƒbƒg‚Í‚¢‚Â‚Å‚àÀs‚Å‚«‚é
	switch (mask) {
		// RESET STANDBY
		case 0x34:
#if defined(FDC_LOG)
			LOG0(Log::Normal, "RESET STANDBYƒRƒ}ƒ“ƒh");
#endif	// FDC_LOG
			fdc.cmd = reset_stdby;
			Result();
			return;

		// SET STANDBY
		case 0x35:
#if defined(FDC_LOG)
			LOG0(Log::Normal, "SET STANDBYƒRƒ}ƒ“ƒh");
#endif	// FDC_LOG
			fdc.cmd = set_stdby;
			Idle();
			return;

		// SOFTWARE RESET
		case 0x36:
#if defined(FDC_LOG)
			LOG0(Log::Normal, "SOFTWARE RESETƒRƒ}ƒ“ƒh");
#endif	// FDC_LOG
			SoftReset();
			return;
	}

	// ƒV[ƒNŒnƒRƒ}ƒ“ƒhÀs’¼Œã‚ÍASENSE INTERRUPT STATUSˆÈŠO‹–‚³‚ê‚È‚¢
	if (fdc.seek) {
		// SENSE INTERRUPT STATUS
		if (mask == 0x08) {
#if defined(FDC_LOG)
			LOG0(Log::Normal, "SENSE INTERRUPT STATUSƒRƒ}ƒ“ƒh");
#endif	// FDC_LOG
			fdc.cmd = sense_int_stat;

			// Š„‚è‚İƒlƒQ[ƒg
			fdc.sr &= 0xf0;
			fdc.seek = FALSE;
			Interrupt(FALSE);

			// ƒpƒ‰ƒ[ƒ^‚È‚µAÀsƒtƒF[ƒY‚È‚µ
			Result();
			return;
		}

		// ‚»‚êˆÈŠO‚Í‘S‚Ä–³ŒøƒRƒ}ƒ“ƒh
#if defined(FDC_LOG)
		LOG0(Log::Normal, "INVALIDƒRƒ}ƒ“ƒh");
#endif	// FDC_LOG
		fdc.cmd = invalid;
		Result();
		return;
	}

	// SENSE INTERRUPT STATUS(–³Œø)
	if (mask == 0x08) {
#if defined(FDC_LOG)
		LOG0(Log::Normal, "INVALIDƒRƒ}ƒ“ƒh");
#endif	// FDC_LOG
		fdc.cmd = invalid;
		Result();
		return;
	}

	// ƒXƒe[ƒ^ƒX‚ğƒNƒŠƒA
	fdc.st[0] = 0;
	fdc.st[1] = 0;
	fdc.st[2] = 0;

	// ’Êí
	switch (mask) {
		// READ DIAGNOSTIC(FMƒ‚[ƒh)
		case 0x02:
#if defined(FDC_LOG)
			LOG0(Log::Normal, "READ DIAGNOSTICƒRƒ}ƒ“ƒh(FMƒ‚[ƒh)");
#endif	// FDC_LOG
			CommandRW(read_diag, data);
			return;

		// SPECIFY
		case 0x03:
#if defined(FDC_LOG)
			LOG0(Log::Normal, "SPECIFYƒRƒ}ƒ“ƒh");
#endif	// FDC_LOG
			fdc.cmd = specify;
			fdc.in_len = 2;
			return;

		// SENSE DEVICE STATUS
		case 0x04:
#if defined(FDC_LOG)
			LOG0(Log::Normal, "SENSE DEVICE STATUSƒRƒ}ƒ“ƒh");
#endif	// FDC_LOG
			fdc.cmd = sense_dev_stat;
			fdc.in_len = 1;
			return;

		// RECALIBRATE
		case 0x07:
#if defined(FDC_LOG)
			LOG0(Log::Normal, "RECALIBRATEƒRƒ}ƒ“ƒh");
#endif	// FDC_LOG
			fdc.cmd = recalibrate;
			fdc.in_len = 1;
			return;

		// READ ID(FMƒ‚[ƒh)
		case 0x0a:
#if defined(FDC_LOG)
			LOG0(Log::Normal, "READ IDƒRƒ}ƒ“ƒh(FMƒ‚[ƒh)");
#endif	// FDC_LOG
			fdc.cmd = read_id;
			fdc.mfm = FALSE;
			fdc.in_len = 1;
			return;

		// WRITE ID(FMƒ‚[ƒh)
		case 0x0d:
#if defined(FDC_LOG)
			LOG0(Log::Normal, "WRITE IDƒRƒ}ƒ“ƒh(FMƒ‚[ƒh)");
#endif	// FDC_LOG
			fdc.cmd = write_id;
			fdc.mfm = FALSE;
			fdc.in_len = 5;
			return;

		// SEEK
		case 0x0f:
#if defined(FDC_LOG)
			LOG0(Log::Normal, "SEEKƒRƒ}ƒ“ƒh");
#endif	// FDC_LOG
			fdc.cmd = seek;
			fdc.in_len = 2;
			return;

		// READ DIAGNOSTIC(MFMƒ‚[ƒh)
		case 0x42:
#if defined(FDC_LOG)
			LOG0(Log::Normal, "READ DIAGNOSTICƒRƒ}ƒ“ƒh(MFMƒ‚[ƒh)");
#endif	// FDC_LOG
			CommandRW(read_diag, data);
			return;

		// READ ID(MFMƒ‚[ƒh)
		case 0x4a:
#if defined(FDC_LOG)
			LOG0(Log::Normal, "READ IDƒRƒ}ƒ“ƒh(MFMƒ‚[ƒh)");
#endif	// FDC_LOG
			fdc.cmd = read_id;
			fdc.mfm = TRUE;
			fdc.in_len = 1;
			return;

		// WRITE ID(MFMƒ‚[ƒh)
		case 0x4d:
#if defined(FDC_LOG)
			LOG0(Log::Normal, "WRITE IDƒRƒ}ƒ“ƒh(MFMƒ‚[ƒh)");
#endif	// FDC_LOG
			fdc.cmd = write_id;
			fdc.mfm = TRUE;
			fdc.in_len = 5;
			return;
	}

	// ƒ}ƒXƒN(2)
	mask &= 0x3f;

	// WRITE DATA
	if (mask == 0x05) {
#if defined(FDC_LOG)
		LOG0(Log::Normal, "WRITE DATAƒRƒ}ƒ“ƒh");
#endif	// FDC_LOG
		CommandRW(write_data, data);
		return;
	}

	// WRITE DELETED DATA
	if (mask == 0x09) {
#if defined(FDC_LOG)
		LOG0(Log::Normal, "WRITE DELETED DATAƒRƒ}ƒ“ƒh");
#endif	// FDC_LOG
		CommandRW(write_del_data, data);
		return;
	}

	// ƒ}ƒXƒN(3);
	mask &= 0x1f;

	// READ DATA
	if (mask == 0x06) {
#if defined(FDC_LOG)
		LOG0(Log::Normal, "READ DATAƒRƒ}ƒ“ƒh");
#endif	// FDC_LOG
		CommandRW(read_data, data);
		return;
	}

	// READ DELETED DATA
	if (mask == 0x0c) {
#if defined(FDC_LOG)
		LOG0(Log::Normal, "READ DELETED DATAƒRƒ}ƒ“ƒh");
#endif	// FDC_LOG
		CommandRW(read_data, data);
		return;
	}

	// SCAN EQUAL
	if (mask == 0x11) {
#if defined(FDC_LOG)
		LOG0(Log::Normal, "SCAN EQUALƒRƒ}ƒ“ƒh");
#endif	// FDC_LOG
		CommandRW(scan_eq, data);
		return;
	}

	// SCAN LOW OR EQUAL
	if (mask == 0x19) {
#if defined(FDC_LOG)
		LOG0(Log::Normal, "SCAN LOW OR EQUALƒRƒ}ƒ“ƒh");
#endif	// FDC_LOG
		CommandRW(scan_lo_eq, data);
		return;
	}

	// SCAN HIGH OR EQUAL
	if (mask == 0x1d) {
#if defined(FDC_LOG)
		LOG0(Log::Normal, "SCAN HIGH OR EQUALƒRƒ}ƒ“ƒh");
#endif	// FDC_LOG
		CommandRW(scan_hi_eq, data);
		return;
	}

	// –¢À‘•
	LOG1(Log::Warning, "ƒRƒ}ƒ“ƒhƒtƒF[ƒY–¢‘Î‰ƒRƒ}ƒ“ƒh $%02X", data);
	Idle();
}

//---------------------------------------------------------------------------
//
//	ƒRƒ}ƒ“ƒhƒtƒF[ƒY(Read/WriteŒn)
//
//---------------------------------------------------------------------------
void FASTCALL FDC::CommandRW(fdccmd cmd, DWORD data)
{
	ASSERT(this);
	ASSERT(data < 0x100);

	// ƒRƒ}ƒ“ƒh
	fdc.cmd = cmd;

	// MT
	if (data & 0x80) {
		fdc.mt = TRUE;
	}
	else {
		fdc.mt = FALSE;
	}

	// MFM
	if (data & 0x40) {
		fdc.mfm = TRUE;
	}
	else {
		fdc.mfm = FALSE;
	}

	// SK(READ/SCAN‚Ì‚İ)
	if (data & 0x20) {
		fdc.sk = TRUE;
	}
	else {
		fdc.sk = FALSE;
	}

	// ƒRƒ}ƒ“ƒhƒtƒF[ƒY‚Ìc‚èƒoƒCƒg”
	fdc.in_len = 8;
}

//---------------------------------------------------------------------------
//
//	ÀsƒtƒF[ƒY
//
//---------------------------------------------------------------------------
void FASTCALL FDC::Execute()
{
	ASSERT(this);

	// ÀsƒtƒF[ƒY‚Ö
	fdc.phase = execute;

	// ƒAƒNƒZƒXŠJnAƒCƒxƒ“ƒg’â~
	fdd->Access(TRUE);
	event.SetTime(0);

	// ƒRƒ}ƒ“ƒh•Ê
	switch (fdc.cmd) {
		// SPECIFY
		case specify:
			// SRT
			fdc.srt = (fdc.in_pkt[1] >> 4) & 0x0f;
			fdc.srt = 16 - fdc.srt;
			fdc.srt <<= 11;

			// HUT (0‚Í16‚Æ“¯‚¶ˆµ‚¢Bi82078ƒf[ƒ^ƒV[ƒg‚É‚æ‚é)
			fdc.hut = fdc.in_pkt[1] & 0x0f;
			if (fdc.hut == 0) {
				fdc.hut = 16;
			}
			fdc.hut <<= 15;

			// HLT (0‚ÍHUT‚Æ“¯—l)
			fdc.hlt = (fdc.in_pkt[2] >> 1) & 0x7f;
			if (fdc.hlt == 0) {
				fdc.hlt = 0x80;
			}
			fdc.hlt <<= 12;

			// NDM
			if (fdc.in_pkt[2] & 1) {
				fdc.ndm = TRUE;
				LOG0(Log::Warning, "Non-DMAƒ‚[ƒh‚Éİ’è");
			}
			else {
				fdc.ndm = FALSE;
			}

			// ƒŠƒUƒ‹ƒgƒtƒF[ƒY•s—v
			Idle();
			return;

		// SENSE DEVICE STATUS
		case sense_dev_stat:
			fdc.us = fdc.in_pkt[1] & 0x03;
			fdc.hd = fdc.in_pkt[1] & 0x04;

			// ƒŠƒUƒ‹ƒgƒtƒF[ƒY
			Result();
			return;

		// RECALIBRATE
		case recalibrate:
			// ƒgƒ‰ƒbƒN0‚ÖƒV[ƒN
			fdc.us = fdc.in_pkt[1] & 0x03;
			fdc.cyl[fdc.us] = 0;

			// SRì¬(SEEKŒnƒRƒ}ƒ“ƒhÀs’†‚ÍNon-Busy)
			fdc.sr &= 0xf0;
			fdc.sr &= ~sr_cb;
			fdc.sr &= ~sr_rqm;
			fdc.sr |= (1 << fdc.dsr);

			// ÅŒã‚ÉÀs‚ğŒÄ‚Ô(“à•”‚ÅCompleteSeek‚ªŒÄ‚Î‚ê‚é‚½‚ß)
			fdd->Recalibrate(fdc.srt);
			return;

		// SEEK
		case seek:
			fdc.us = fdc.in_pkt[1] & 0x03;

			// SRì¬(SEEKŒnƒRƒ}ƒ“ƒhÀs’†‚ÍNon-Busy)
			fdc.sr &= 0xf0;
			fdc.sr &= ~sr_cb;
			fdc.sr &= ~sr_rqm;
			fdc.sr |= (1 << fdc.dsr);

			// ÅŒã‚ÉÀs‚ğŒÄ‚Ô(“à•”‚ÅCompleteSeek‚ªŒÄ‚Î‚ê‚é‚½‚ß)
			if (fdc.cyl[fdc.us] < fdc.in_pkt[2]) {
				// ƒXƒeƒbƒvƒCƒ“
				fdd->StepIn(fdc.in_pkt[2] - fdc.cyl[fdc.us], fdc.srt);
			}
			else {
				// ƒXƒeƒbƒvƒAƒEƒg
				fdd->StepOut(fdc.cyl[fdc.us] - fdc.in_pkt[2], fdc.srt);
			}
			fdc.cyl[fdc.us] = fdc.in_pkt[2];
			return;

		// READ ID
		case read_id:
			ReadID();
			return;

		// WRITE ID
		case write_id:
			fdc.us = fdc.in_pkt[1] & 0x03;
			fdc.hd = fdc.in_pkt[1] & 0x04;
			fdc.st[0] = fdc.us;
			fdc.st[0] |= fdc.hd;
			fdc.chrn[3] = fdc.in_pkt[2];
			fdc.sc = fdc.in_pkt[3];
			fdc.gpl = fdc.in_pkt[4];
			fdc.d = fdc.in_pkt[5];
			if (!WriteID()) {
				Result();
			}
			return;

		// READ DIAGNOSTIC
		case read_diag:
			ExecuteRW();
			if (!ReadDiag()) {
				Result();
			}
			return;

		// READ DATA
		case read_data:
			ExecuteRW();
			if (!ReadData()) {
				Result();
			}
			return;

		// READ DELETED DATA
		case read_del_data:
			ExecuteRW();
			if (!ReadData()) {
				Result();
			}
			return;

		// WRITE DATA
		case write_data:
			ExecuteRW();
			if (!WriteData()) {
				Result();
			}
			return;

		// WRITE DELETED_DATA
		case write_del_data:
			ExecuteRW();
			if (!WriteData()) {
				Result();
			}
			return;

		// SCANŒn
		case scan_eq:
		case scan_lo_eq:
		case scan_hi_eq:
			ExecuteRW();
			if (!Scan()) {
				Result();
			}
			return;
	}

	LOG1(Log::Warning, "ÀsƒtƒF[ƒY–¢‘Î‰ƒRƒ}ƒ“ƒh $%02X", fdc.in_pkt[0]);
}

//---------------------------------------------------------------------------
//
//	ÀsƒtƒF[ƒY(ReadID)
//
//---------------------------------------------------------------------------
void FASTCALL FDC::ReadID()
{
	DWORD hus;

	ASSERT(this);

	// HD, US‚ğ‹L‰¯
	fdc.us = fdc.in_pkt[1] & 0x03;
	fdc.hd = fdc.in_pkt[1] & 0x04;

	// FDD‚ÉÀs‚³‚¹‚éBNOTREADY, NODATA, MAM‚ªl‚¦‚ç‚ê‚é
	fdc.err = fdd->ReadID(&(fdc.out_pkt[3]), fdc.mfm, fdc.hd);

	// NOT READY‚È‚ç‚·‚®ƒŠƒUƒ‹ƒgƒtƒF[ƒY
	if (fdc.err & FDD_NOTREADY) {
		Result();
		return;
	}

	// ŒŸõ‚É‚©‚©‚éŠÔ‚ğİ’è
	hus = fdd->GetSearch();
	event.SetTime(hus);
	fdc.sr &= ~sr_rqm;
}

//---------------------------------------------------------------------------
//
//	ÀsƒtƒF[ƒY(Read/WriteŒn)
//
//---------------------------------------------------------------------------
void FASTCALL FDC::ExecuteRW()
{
	ASSERT(this);

	// 8ƒoƒCƒg‚ÌƒpƒPƒbƒg‚ğ•ªŠ„(ÅIƒoƒCƒg‚Íí‚ÉDTL‚ÉƒZƒbƒg)
	fdc.us = fdc.in_pkt[1] & 0x03;
	fdc.hd = fdc.in_pkt[1] & 0x04;
	fdc.st[0] = fdc.us;
	fdc.st[0] |= fdc.hd;

	fdc.chrn[0] = fdc.in_pkt[2];
	fdc.chrn[1] = fdc.in_pkt[3];
	fdc.chrn[2] = fdc.in_pkt[4];
	fdc.chrn[3] = fdc.in_pkt[5];

	fdc.eot = fdc.in_pkt[6];
	fdc.gsl = fdc.in_pkt[7];
	fdc.dtl = fdc.in_pkt[8];
}

//---------------------------------------------------------------------------
//
//	ÀsƒtƒF[ƒY(Read)
//
//---------------------------------------------------------------------------
BYTE FASTCALL FDC::Read()
{
	BYTE data;

	ASSERT(fdc.len > 0);
	ASSERT(fdc.offset < 0x4000);

	// ƒoƒbƒtƒ@‚©‚çƒf[ƒ^‚ğ‹Ÿ‹‹
	data = fdc.buffer[fdc.offset];
	fdc.offset++;
	fdc.len--;

	// ÅŒã‚Å‚È‚¯‚ê‚Î‚»‚Ì‚Ü‚Ü‘±‚¯‚é
	if (fdc.len > 0) {
		return data;
	}

	// READ DIAGNOSTIC‚Ìê‡‚Í‚±‚±‚ÅI—¹
	if (fdc.cmd == read_diag) {
		// ³íI—¹‚È‚çAƒZƒNƒ^‚ği‚ß‚é
		if (fdc.err == FDD_NOERROR) {
			NextSector();
		}
		// ƒCƒxƒ“ƒg‚ğ‘Å‚¿Ø‚èAƒŠƒUƒ‹ƒgƒtƒF[ƒY‚Ö
		event.SetTime(0);
		Result();
		return data;
	}

	// ˆÙíI—¹‚È‚çA‚±‚ÌƒZƒNƒ^‚Å‘Å‚¿Ø‚è
	if (fdc.err != FDD_NOERROR) {
		// ƒCƒxƒ“ƒg‚ğ‘Å‚¿Ø‚èAƒŠƒUƒ‹ƒgƒtƒF[ƒY‚Ö
		event.SetTime(0);
		Result();
		return data;
	}

	// ƒ}ƒ‹ƒ`ƒZƒNƒ^ˆ—
	if (!NextSector()) {
		// ƒCƒxƒ“ƒg‚ğ‘Å‚¿Ø‚èAƒŠƒUƒ‹ƒgƒtƒF[ƒY‚Ö
		event.SetTime(0);
		Result();
		return data;
	}

	// Ÿ‚ÌƒZƒNƒ^‚ª‚ ‚é‚Ì‚ÅA€”õ
	if (!ReadData()) {
		// ƒZƒNƒ^“Ç‚İæ‚è•s”
		event.SetTime(0);
		Result();
		return data;
	}

	// OKAŸ‚ÌƒZƒNƒ^‚Ö
	return data;
}

//---------------------------------------------------------------------------
//
//	ÀsƒtƒF[ƒY(Write)
//
//---------------------------------------------------------------------------
void FASTCALL FDC::Write(DWORD data)
{
	ASSERT(this);
	ASSERT(fdc.len > 0);
	ASSERT(fdc.offset < 0x4000);
	ASSERT(data < 0x100);

	// WRITE ID‚Ìê‡‚Íƒoƒbƒtƒ@‚É—­‚ß‚é‚Ì‚İ
	if (fdc.cmd == write_id) {
		fdc.buffer[fdc.offset] = (BYTE)data;
		fdc.offset++;
		fdc.len--;

		// I—¹ƒ`ƒFƒbƒN
		if (fdc.len == 0) {
			WriteBack();
			event.SetTime(0);
			Result();
		}
		return;
	}

	// ƒXƒLƒƒƒ“Œn‚Ìê‡‚Í”äŠr
	if ((fdc.cmd == scan_eq) || (fdc.cmd == scan_lo_eq) || (fdc.cmd == scan_hi_eq)) {
		Compare(data);
		return;
	}

	// ƒoƒbƒtƒ@‚Öƒf[ƒ^‚ğ‘‚«‚Ş
	fdc.buffer[fdc.offset] = (BYTE)data;
	fdc.offset++;
	fdc.len--;

	// ÅŒã‚Å‚È‚¯‚ê‚Î‚»‚Ì‚Ü‚Ü‘±‚¯‚é
	if (fdc.len > 0) {
		return;
	}

	// ‘‚«‚İI—¹
	WriteBack();
	if (fdc.err != FDD_NOERROR) {
		event.SetTime(0);
		Result();
		return;
	}

	// ƒ}ƒ‹ƒ`ƒZƒNƒ^ˆ—
	if (!NextSector()) {
		// ƒCƒxƒ“ƒg‚ğ‘Å‚¿Ø‚èAƒŠƒUƒ‹ƒgƒtƒF[ƒY‚Ö
		event.SetTime(0);
		Result();
		return;
	}

	// Ÿ‚ÌƒZƒNƒ^‚ª‚ ‚é‚Ì‚ÅA€”õ
	if (!WriteData()) {
		event.SetTime(0);
		Result();
	}
}

//---------------------------------------------------------------------------
//
//	ÀsƒtƒF[ƒY(Compare)
//
//---------------------------------------------------------------------------
void FASTCALL FDC::Compare(DWORD data)
{
	ASSERT(this);
	ASSERT(data < 0x100);

	if (data != 0xff) {
		// —LŒøƒoƒCƒg‚ÅA‚Ü‚¾”»’èo‚Ä‚È‚¢‚È‚ç
		if (!(fdc.err & FDD_SCANNOT)) {
			// ”äŠr‚ª•K—v
			switch (fdc.cmd) {
				case scan_eq:
					if (fdc.buffer[fdc.offset] != (BYTE)data) {
						fdc.err |= FDD_SCANNOT;
					}
					break;
				case scan_lo_eq:
					if (fdc.buffer[fdc.offset] > (BYTE)data) {
						fdc.err |= FDD_SCANNOT;
					}
					break;
				case scan_hi_eq:
					if (fdc.buffer[fdc.offset] < (BYTE)data) {
						fdc.err |= FDD_SCANNOT;
					}
					break;
				default:
					ASSERT(FALSE);
					break;
			}

		}
	}

	// Ÿ‚Ìƒf[ƒ^‚Ö
	fdc.offset++;
	fdc.len--;

	// ÅŒã‚Å‚È‚¯‚ê‚Î‚»‚Ì‚Ü‚Ü‘±‚¯‚é
	if (fdc.len > 0) {
		return;
	}

	// ÅŒã‚È‚Ì‚ÅAŒ‹‰Ê‚Ü‚Æ‚ß
	if (!(fdc.err & FDD_SCANNOT)) {
		// ok!
		fdc.err |= FDD_SCANEQ;
		event.SetTime(0);
		Result();
	}

	// STP‚ª2‚Ì‚Æ‚«‚ÍA+1
	if (fdc.dtl == 0x02) {
		fdc.chrn[2]++;
	}

	// ƒ}ƒ‹ƒ`ƒZƒNƒ^ˆ—
	if (!NextSector()) {
		// SCAN NOT‚Íã‚ª‚Á‚½‚Ü‚Ü‚È‚Ì‚Å“s‡‚ª‚æ‚¢
		event.SetTime(0);
		Result();
		return;
	}

	// Ÿ‚ÌƒZƒNƒ^‚ª‚ ‚é‚Ì‚ÅA€”õ
	if (!Scan()) {
		// SCAN NOT‚Íã‚ª‚Á‚½‚Ü‚Ü‚È‚Ì‚Å“s‡‚ª‚æ‚¢
		event.SetTime(0);
		Result();
	}

	// SCAN NOT‚ğ‰º‚°‚Ä‚à‚¤‚PƒZƒNƒ^
	fdc.err &= ~FDD_SCANNOT;
}

//---------------------------------------------------------------------------
//
//	ƒŠƒUƒ‹ƒgƒtƒF[ƒY
//
//---------------------------------------------------------------------------
void FASTCALL FDC::Result()
{
	ASSERT(this);

	// ƒŠƒUƒ‹ƒgƒtƒF[ƒY
	fdc.phase = result;
	fdc.sr |= sr_rqm;
	fdc.sr |= sr_dio;
	fdc.sr &= ~sr_ndm;

	// ƒRƒ}ƒ“ƒh•Ê
	switch (fdc.cmd) {
		// SENSE DEVICE STATUS
		case sense_dev_stat:
			// ST3‚ğì¬Aƒf[ƒ^“]‘—
			MakeST3();
			fdc.out_pkt[0] = fdc.st[3];
			fdc.out_len = 1;
			fdc.out_cnt = 0;
			return;

		// SENSE INTERRUPT STATUS
		case sense_int_stat:
			// ST0EƒVƒŠƒ“ƒ_‚ğ•Ô‚·Bƒf[ƒ^“]‘—
			fdc.out_pkt[0] = fdc.st[0];
			fdc.out_pkt[1] = fdc.cyl[fdc.us];
			fdc.out_len = 2;
			fdc.out_cnt = 0;
			return;

		// READ ID
		case read_id:
			// ST0,ST1,ST2ì¬BNOTREADY, NODATA, MAM‚ªl‚¦‚ç‚ê‚é
			fdc.st[0] = fdc.us;
			fdc.st[0] |= fdc.hd;
			if (fdc.err & FDD_NOTREADY) {
				// Not Ready
				fdc.st[0] |= 0x08;
				fdc.st[1] = 0;
				fdc.st[2] = 0;
			}
			else {
				if (fdc.err != FDD_NOERROR) {
					// Abnormal Teriminate
					fdc.st[0] |= 0x40;
				}
				fdc.st[1] = fdc.err >> 8;
				fdc.st[2] = fdc.err & 0xff;
			}

			// ƒf[ƒ^“]‘—AƒŠƒUƒ‹ƒgƒtƒF[ƒYŠ„‚è‚İ
			fdc.out_pkt[0] = fdc.st[0];
			fdc.out_pkt[1] = fdc.st[1];
			fdc.out_pkt[2] = fdc.st[2];
			fdc.out_len = 7;
			fdc.out_cnt = 0;
			Interrupt(TRUE);
			return;

		// INVALID, RESET STANDBY
		case invalid:
		case reset_stdby:
			fdc.out_pkt[0] = 0x80;
			fdc.out_len = 1;
			fdc.out_cnt = 0;
			return;

		// READ,WRITE,SCANŒn
		case read_data:
		case read_del_data:
		case write_data:
		case write_del_data:
		case scan_eq:
		case scan_lo_eq:
		case scan_hi_eq:
		case read_diag:
		case write_id:
			ResultRW();
			return;
	}

	LOG1(Log::Warning, "ƒŠƒUƒ‹ƒgƒtƒF[ƒY–¢‘Î‰ƒRƒ}ƒ“ƒh $%02X", fdc.in_pkt[0]);
}

//---------------------------------------------------------------------------
//
//	ƒŠƒUƒ‹ƒgƒtƒF[ƒY(Read/WriteŒn)
//
//---------------------------------------------------------------------------
void FASTCALL FDC::ResultRW()
{
	ASSERT(this);

	// ST0,ST1,ST2ì¬
	if (fdc.err & FDD_NOTREADY) {
		// Not Ready
		fdc.st[0] |= 0x08;
		fdc.st[1] = 0;
		fdc.st[2] = 0;
	}
	else {
		if ((fdc.err != FDD_NOERROR) && (fdc.err != FDD_SCANEQ)) {
			// Abnormal Teriminate
			fdc.st[0] |= 0x40;
		}
		fdc.st[1] = fdc.err >> 8;
		fdc.st[2] = fdc.err & 0xff;
	}

	// READ DIAGNOSTIC‚Í0x40‚ğo‚³‚È‚¢
	if (fdc.cmd == read_diag) {
		if (fdc.st[0] & 0x40) {
			fdc.st[0] &= ~0x40;
		}
	}

	// ƒŠƒUƒ‹ƒgƒpƒPƒbƒg‚ğİ’è
	fdc.out_pkt[0] = fdc.st[0];
	fdc.out_pkt[1] = fdc.st[1];
	fdc.out_pkt[2] = fdc.st[2];
	fdc.out_pkt[3] = fdc.chrn[0];
	fdc.out_pkt[4] = fdc.chrn[1];
	fdc.out_pkt[5] = fdc.chrn[2];
	fdc.out_pkt[6] = fdc.chrn[3];
	fdc.out_len = 7;
	fdc.out_cnt = 0;

	// ’Êí‚ÍƒŠƒUƒ‹ƒgƒtƒF[ƒYŠ„‚è‚İ
	Interrupt(TRUE);
}

//---------------------------------------------------------------------------
//
//	Š„‚è‚İ
//
//---------------------------------------------------------------------------
void FASTCALL FDC::Interrupt(BOOL flag)
{
	ASSERT(this);

	// IOSC‚É’Ê’m
	iosc->IntFDC(flag);
}

//---------------------------------------------------------------------------
//
//	ST3ì¬
//
//---------------------------------------------------------------------------
void FASTCALL FDC::MakeST3()
{
	ASSERT(this);

	// HD,US‚ğƒZƒbƒg
	fdc.st[3] = fdc.hd;
	fdc.st[3] |= fdc.us;

	// ƒŒƒfƒB‚©
	if (fdd->IsReady(fdc.dsr)) {
		// ƒŒƒfƒB
		fdc.st[3] |= 0x20;

		// ƒ‰ƒCƒgƒvƒƒeƒNƒg‚©
		if (fdd->IsWriteP(fdc.dsr)) {
			fdc.st[3] |= 0x40;
		}
	}
	else {
		// ƒŒƒfƒB‚Å‚È‚¢
		fdc.st[3] = 0x40;
	}

	// TRACK0‚©
	if (fdd->GetCylinder(fdc.dsr) == 0) {
		fdc.st[3] |= 0x10;
	}
}

//---------------------------------------------------------------------------
//
//	READ (DELETED) DATAƒRƒ}ƒ“ƒh
//
//---------------------------------------------------------------------------
BOOL FASTCALL FDC::ReadData()
{
	int len;
	DWORD hus;

	ASSERT(this);
	ASSERT((fdc.cmd == read_data) || (fdc.cmd == read_del_data));

	// SRİ’è
	fdc.sr |= sr_cb;
	fdc.sr |= sr_dio;
	fdc.sr &= ~sr_d3b;
	fdc.sr &= ~sr_d2b;
	fdc.sr &= ~sr_d1b;
	fdc.sr &= ~sr_d0b;

	// ƒhƒ‰ƒCƒu‚É”C‚¹‚éBNOTREADY,NODATA,MAM,CYLŒn,CRCŒn,DDAM
#if defined(FDC_LOG)
	LOG4(Log::Normal, "(C:%02X H:%02X R:%02X N:%02X)",
		fdc.chrn[0], fdc.chrn[1], fdc.chrn[2], fdc.chrn[3]);
#endif
	fdc.err = fdd->ReadSector(fdc.buffer, &fdc.len,
									fdc.mfm, fdc.chrn, fdc.hd);

	// DDAM(Deleted Sector)‚Ì—L–³‚ÅACM(Control Mark)‚ğŒˆ‚ß‚é
	if (fdc.cmd == read_data) {
		// Read Data (DDAM‚ÍƒGƒ‰[)
		if (fdc.err & FDD_DDAM) {
			fdc.err &= ~FDD_DDAM;
			fdc.err |= FDD_CM;
		}
	}
	else {
		// Read Deleted Data (DDAM‚Å‚È‚¯‚ê‚ÎƒGƒ‰[)
		if (!(fdc.err & FDD_DDAM)) {
			fdc.err |= FDD_CM;
		}
		fdc.err &= ~FDD_DDAM;
	}

	// IDCRC‚Ü‚½‚ÍDATACRC‚È‚çADATAERR‚ğæ‚¹‚é
	if ((fdc.err & FDD_IDCRC) || (fdc.err & FDD_DATACRC)) {
		fdc.err &= ~FDD_IDCRC;
		fdc.err |= FDD_DATAERR;
	}

	// N=0‚Å‚ÍN‚Æ‚µ‚ÄDTL‚ğg‚¤
	if (fdc.chrn[3] == 0) {
		len = 1 << (fdc.dtl + 7);
		if (len < fdc.len) {
			fdc.len = len;
		}
	}
	else {
		// (MacƒGƒ~ƒ…ƒŒ[ƒ^)
		len = (1 << (fdc.chrn[3] + 7));
		if (len < fdc.len) {
			fdc.len = len;
		}
	}

	// Not Ready‚ÍƒŠƒUƒ‹ƒgƒtƒF[ƒY‚Ö
	if (fdc.err & FDD_NOTREADY) {
		return FALSE;
	}

	// CM‚ÍSK=1‚È‚çƒŠƒUƒ‹ƒgƒtƒF[ƒY‚Ö(i82078ƒf[ƒ^ƒV[ƒg‚É‚æ‚é)
	if (fdc.err & FDD_CM) {
		if (fdc.sk) {
			return FALSE;
		}
	}

	// ŒŸõŠÔ‚ğŒvZ(ƒwƒbƒhƒ[ƒh‚Íl‚¦‚È‚¢)
	hus = fdd->GetSearch();

	// No Data‚Í‚±‚ÌŠÔŒãAƒŠƒUƒ‹ƒgƒtƒF[ƒY‚Ö
	if (fdc.err & FDD_NODATA) {
		EventErr(hus);
		return TRUE;
	}

	// ƒIƒtƒZƒbƒg‰Šú‰»AƒCƒxƒ“ƒgƒXƒ^[ƒgAERƒtƒF[ƒYŠJn
	fdc.offset = 0;
	EventRW();
	fdc.phase = read;

	// ŒŸõŠÔ‚ªƒwƒbƒhƒ[ƒhŠÔ‚æ‚è’Z‚¯‚ê‚ÎA‚Pü‘Ò‚¿
	if (!fdc.load) {
		if (hus < fdc.hlt) {
			hus += fdd->GetRotationTime();
		}
	}

	// ŠÔ‚ğ‰ÁZ
	if (!fdc.fast) {
		hus += event.GetTime();
		event.SetTime(hus);
	}
	return TRUE;
}

//---------------------------------------------------------------------------
//
//	WRITE (DELETED) DATAƒRƒ}ƒ“ƒh
//
//---------------------------------------------------------------------------
BOOL FASTCALL FDC::WriteData()
{
	int len;
	DWORD hus;
	BOOL deleted;

	ASSERT(this);
	ASSERT((fdc.cmd == write_data) || (fdc.cmd == write_del_data));

	// SRİ’è
	fdc.sr |= sr_cb;
	fdc.sr &= ~sr_dio;
	fdc.sr &= ~sr_d3b;
	fdc.sr &= ~sr_d2b;
	fdc.sr &= ~sr_d1b;
	fdc.sr &= ~sr_d0b;

	// ƒhƒ‰ƒCƒu‚É”C‚¹‚éBNOTREADY,NOTWRITE,NODATA,MAM,CYLŒn,IDCRC,DDAM
	deleted = FALSE;
	if (fdc.cmd == write_del_data) {
		deleted = TRUE;
	}
#if defined(FDC_LOG)
	LOG4(Log::Normal, "(C:%02X H:%02X R:%02X N:%02X)",
		fdc.chrn[0], fdc.chrn[1], fdc.chrn[2], fdc.chrn[3]);
#endif
	fdc.err = fdd->WriteSector(NULL, &fdc.len,
									fdc.mfm, fdc.chrn, fdc.hd, deleted);
	fdc.err &= ~FDD_DDAM;

	// IDCRC‚È‚çADATAERR‚ğæ‚¹‚é
	if (fdc.err & FDD_IDCRC) {
		fdc.err &= ~FDD_IDCRC;
		fdc.err |= FDD_DATAERR;
	}

	// N=0‚Å‚ÍN‚Æ‚µ‚ÄDTL‚ğg‚¤
	if (fdc.chrn[3] == 0) {
		len = 1 << (fdc.dtl + 7);
		if (len < fdc.len) {
			fdc.len = len;
		}
	}
	else {
		len = (1 << (fdc.chrn[3] + 7));
		if (len < fdc.len) {
			fdc.len = len;
		}
	}

	// Not Ready, Not Writable‚ÍƒŠƒUƒ‹ƒgƒtƒF[ƒY‚Ö
	if ((fdc.err & FDD_NOTREADY) || (fdc.err & FDD_NOTWRITE)) {
		return FALSE;
	}

	// ŒŸõŠÔ‚ğŒvZ(ƒwƒbƒhƒ[ƒh‚Íl‚¦‚È‚¢)
	hus = fdd->GetSearch();

	// No Data‚ÍÀsŒãƒŠƒUƒ‹ƒg‚Ö
	if (fdc.err & FDD_NODATA) {
		EventErr(hus);
		return TRUE;
	}

	// ƒIƒtƒZƒbƒg‰Šú‰»AƒCƒxƒ“ƒgİ’èAEWƒtƒF[ƒYŠJn
	fdc.offset = 0;
	EventRW();
	fdc.phase = write;

	// ŒŸõŠÔ‚ªƒwƒbƒhƒ[ƒhŠÔ‚æ‚è’Z‚¯‚ê‚ÎA‚Pü‘Ò‚¿
	if (!fdc.load) {
		if (hus < fdc.hlt) {
			hus += fdd->GetRotationTime();
		}
	}

	// ŠÔ‚ğ‰ÁZ
	if (!fdc.fast) {
		hus += event.GetTime();
		event.SetTime(hus);
	}
	return TRUE;
}

//---------------------------------------------------------------------------
//
//	SCANŒnƒRƒ}ƒ“ƒh
//
//---------------------------------------------------------------------------
BOOL FASTCALL FDC::Scan()
{
	int len;
	DWORD hus;

	ASSERT(this);

	// SRİ’è
	fdc.sr |= sr_cb;
	fdc.sr &= ~sr_dio;
	fdc.sr &= ~sr_d3b;
	fdc.sr &= ~sr_d2b;
	fdc.sr &= ~sr_d1b;
	fdc.sr &= ~sr_d0b;

	// ƒhƒ‰ƒCƒu‚É”C‚¹‚éBNOTREADY,NODATA,MAM,CYLŒn,CRCŒn,DDAM
#if defined(FDC_LOG)
	LOG4(Log::Normal, "(C:%02X H:%02X R:%02X N:%02X)",
		fdc.chrn[0], fdc.chrn[1], fdc.chrn[2], fdc.chrn[3]);
#endif
	fdc.err = fdd->ReadSector(fdc.buffer, &fdc.len,
									fdc.mfm, fdc.chrn, fdc.hd);

	// DDAM(Deleted Sector)‚Ì—L–³‚ÅACM(Control Mark)‚ğŒˆ‚ß‚é
	if (fdc.err & FDD_DDAM) {
		fdc.err &= ~FDD_DDAM;
		fdc.err |= FDD_CM;
	}

	// IDCRC‚Ü‚½‚ÍDATACRC‚È‚çADATAERR‚ğæ‚¹‚é
	if ((fdc.err & FDD_IDCRC) || (fdc.err & FDD_DATACRC)) {
		fdc.err &= ~FDD_IDCRC;
		fdc.err |= FDD_DATAERR;
	}

	// N=0‚Å‚ÍN‚Æ‚µ‚ÄDTL‚ğg‚¤
	if (fdc.chrn[3] == 0) {
		len = 1 << (fdc.dtl + 7);
		if (len < fdc.len) {
			fdc.len = len;
		}
	}
	else {
		len = (1 << (fdc.chrn[3] + 7));
		if (len < fdc.len) {
			fdc.len = len;
		}
	}

	// Not Ready‚ÍƒŠƒUƒ‹ƒgƒtƒF[ƒY‚Ö
	if (fdc.err & FDD_NOTREADY) {
		return FALSE;
	}

	// CM‚ÍSK=1‚È‚çƒŠƒUƒ‹ƒgƒtƒF[ƒY‚Ö(i82078ƒf[ƒ^ƒV[ƒg‚É‚æ‚é)
	if (fdc.err & FDD_CM) {
		if (fdc.sk) {
			return FALSE;
		}
	}

	// ŒŸõŠÔ‚ğŒvZ(ƒwƒbƒhƒ[ƒh‚Íl‚¦‚È‚¢)
	hus = fdd->GetSearch();

	// No Data‚Í‚±‚ÌŠÔŒãAƒŠƒUƒ‹ƒgƒtƒF[ƒY‚Ö
	if (fdc.err & FDD_NODATA) {
		EventErr(hus);
		return TRUE;
	}

	// ƒIƒtƒZƒbƒg‰Šú‰»AƒCƒxƒ“ƒgƒXƒ^[ƒgAERƒtƒF[ƒYŠJn
	fdc.offset = 0;
	EventRW();
	fdc.phase = write;

	// ŒŸõŠÔ‚ªƒwƒbƒhƒ[ƒhŠÔ‚æ‚è’Z‚¯‚ê‚ÎA‚Pü‘Ò‚¿
	if (!fdc.load) {
		if (hus < fdc.hlt) {
			hus += fdd->GetRotationTime();
		}
	}

	// ŠÔ‚ğ‰ÁZ
	if (!fdc.fast) {
		hus += event.GetTime();
		event.SetTime(hus);
	}
	return TRUE;
}

//---------------------------------------------------------------------------
//
//	READ DIAGNOSTICƒRƒ}ƒ“ƒh
//
//---------------------------------------------------------------------------
BOOL FASTCALL FDC::ReadDiag()
{
	DWORD hus;

	ASSERT(this);
	ASSERT(fdc.cmd == read_diag);

	// SRİ’è
	fdc.sr |= sr_cb;
	fdc.sr |= sr_dio;
	fdc.sr &= ~sr_d3b;
	fdc.sr &= ~sr_d2b;
	fdc.sr &= ~sr_d1b;
	fdc.sr &= ~sr_d0b;

	// EOT=0‚ÍƒŠƒUƒ‹ƒgƒtƒF[ƒY‚Ö(NO DATA)
	if (fdc.eot == 0) {
		if (fdd->IsReady(fdc.dsr)) {
			fdc.err = FDD_NODATA;
		}
		else {
			fdc.err = FDD_NOTREADY;
		}
		return FALSE;
	}

	// ƒhƒ‰ƒCƒu‚É”C‚¹‚éBNOTREADY,NODATA,MAM,CRCŒn,DDAM
	fdc.err = fdd->ReadDiag(fdc.buffer, &fdc.len, fdc.mfm,
								fdc.chrn, fdc.hd);
	// Not Ready‚ÍƒŠƒUƒ‹ƒgƒtƒF[ƒY‚Ö
	if (fdc.err & FDD_NOTREADY) {
		return FALSE;
	}

	// ŒŸõŠÔ‚ğŒvZ(ƒwƒbƒhƒ[ƒh‚Íl‚¦‚È‚¢)
	hus = fdd->GetSearch();

	// MAM‚È‚çŠÔ‘Ò‚¿ŒãAƒŠƒUƒ‹ƒgƒtƒF[ƒY‚ÖBNODATA‚Å‚à‘±‚¯‚é‚½‚ß
	if (fdc.err & FDD_MAM) {
		EventErr(hus);
		return TRUE;
	}

	ASSERT(fdc.len > 0);

	// ƒIƒtƒZƒbƒg‰Šú‰»AƒCƒxƒ“ƒgƒXƒ^[ƒgAERƒtƒF[ƒYŠJn
	fdc.offset = 0;
	EventRW();
	fdc.phase = read;

	// ŠÔ‚ğ‰ÁZ
	if (!fdc.fast) {
		hus += event.GetTime();
		event.SetTime(hus);
	}
	return TRUE;
}

//---------------------------------------------------------------------------
//
//	WRITE IDƒRƒ}ƒ“ƒh
//
//---------------------------------------------------------------------------
BOOL FASTCALL FDC::WriteID()
{
	DWORD hus;

	ASSERT(this);
	ASSERT(fdc.cmd == write_id);

	// SRİ’è
	fdc.sr |= sr_cb;
	fdc.sr &= ~sr_dio;
	fdc.sr &= ~sr_d3b;
	fdc.sr &= ~sr_d2b;
	fdc.sr &= ~sr_d1b;
	fdc.sr &= ~sr_d0b;

	// SC=0ƒ`ƒFƒbƒN
	if (fdc.sc == 0) {
		fdc.err = 0;
		return FALSE;
	}

	// ƒhƒ‰ƒCƒu‚É”C‚¹‚éBNOTREADY,NOTWRITE
	fdc.err = fdd->WriteID(NULL, fdc.d, fdc.sc, fdc.mfm, fdc.hd, fdc.gpl);
	// Not Ready, Not Writable‚ÍƒŠƒUƒ‹ƒgƒtƒF[ƒY‚Ö
	if ((fdc.err & FDD_NOTREADY) || (fdc.err & FDD_NOTWRITE)) {
		return FALSE;
	}

	// ƒIƒtƒZƒbƒg‰Šú‰»
	fdc.offset = 0;
	fdc.len = fdc.sc * 4;

	// ƒCƒxƒ“ƒgİ’è
	if (fdc.ndm) {
		fdc.sr |= sr_ndm;
		LOG0(Log::Warning, "Non-DMAƒ‚[ƒh‚ÅWrite ID");
	}
	else {
		fdc.sr &= ~sr_ndm;
	}
	// N‚Í7‚Ü‚Å‚É§ŒÀ(N=7‚Í16KB/sector, ƒAƒ“ƒtƒH[ƒ}ƒbƒg)
	if (fdc.chrn[3] > 7) {
		fdc.chrn[3] = 7;
	}

	// ŠÔ‚ğİ’èB‚Püã‚ğƒZƒNƒ^‚ÅŠ„‚Á‚½”
	hus = fdd->GetSearch();
	hus += (fdd->GetRotationTime() / fdc.sc);
	if (fdc.fast) {
		hus = 32 * 4;
	}

	// ƒCƒxƒ“ƒgƒXƒ^[ƒgARQM‚ğ—‚Æ‚·
	event.SetTime(hus);
	fdc.sr &= ~sr_rqm;
	fdc.phase = write;

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	ƒCƒxƒ“ƒg(Read/WriteŒn)
//
//---------------------------------------------------------------------------
void FASTCALL FDC::EventRW()
{
	DWORD hus;

	// SRİ’è(Non-DMA)
	if (fdc.ndm) {
		fdc.sr |= sr_ndm;
	}
	else {
		fdc.sr &= ~sr_ndm;
	}

	// ƒCƒxƒ“ƒgì¬
	if (fdc.ndm) {
		// Non-DMAB16us/32us
		if (fdc.mfm) {
			hus = 32;
		}
		else {
			hus = 64;
		}
	}
	else {
		// DMA‚Í64ƒoƒCƒg‚Ü‚Æ‚ß‚Äs‚¤B1024us/2048us
		if (fdc.mfm) {
			hus = 32 * 64;
		}
		else {
			hus = 64 * 64;
		}
	}

	// DD‚Í‚»‚Ì”{
	if (!fdd->IsHD()) {
		hus <<= 1;
	}

	// fast‚Í64usŒÅ’è(DMA‚ÉŒÀ‚é)
	if (fdc.fast) {
		if (!fdc.ndm) {
			hus = 128;
		}
	}

	// ƒCƒxƒ“ƒgƒXƒ^[ƒgARQM‚ğ—‚Æ‚·
	event.SetTime(hus);
	fdc.sr &= ~sr_rqm;
}

//---------------------------------------------------------------------------
//
//	ƒCƒxƒ“ƒg(ƒGƒ‰[)
//
//---------------------------------------------------------------------------
void FASTCALL FDC::EventErr(DWORD hus)
{
	// SRİ’è(Non-DMA)
	if (fdc.ndm) {
		fdc.sr |= sr_ndm;
	}
	else {
		fdc.sr &= ~sr_ndm;
	}

	// ƒCƒxƒ“ƒgƒXƒ^[ƒgARQM‚ğ—‚Æ‚·
	event.SetTime(hus);
	fdc.sr &= ~sr_rqm;
	fdc.phase = execute;
}

//---------------------------------------------------------------------------
//
//	‘‚«‚İŠ®—¹
//
//---------------------------------------------------------------------------
void FASTCALL FDC::WriteBack()
{
	switch (fdc.cmd) {
		// Write Data
		case write_data:
			fdc.err = fdd->WriteSector(fdc.buffer, &fdc.len,
							fdc.mfm, fdc.chrn, fdc.hd, FALSE);
			return;

		// Write Deleted Data
		case write_del_data:
			fdc.err = fdd->WriteSector(fdc.buffer, &fdc.len,
							fdc.mfm, fdc.chrn, fdc.hd, TRUE);
			return;

		// Write ID
		case write_id:
			fdc.err = fdd->WriteID(fdc.buffer, fdc.d, fdc.sc,
							fdc.mfm, fdc.hd, fdc.gpl);
			return;
	}

	// ‚ ‚è‚¦‚È‚¢
	ASSERT(FALSE);
}

//---------------------------------------------------------------------------
//
//	ŸƒZƒNƒ^
//
//---------------------------------------------------------------------------
BOOL FASTCALL FDC::NextSector()
{
	// TCƒ`ƒFƒbƒN
	if (fdc.tc) {
		// C,H,R,N‚ğˆÚ“®
		if (fdc.chrn[2] < fdc.eot) {
			fdc.chrn[2]++;
			return FALSE;
		}
		fdc.chrn[2] = 0x01;
		// MT‚É‚æ‚Á‚Ä•ª‚¯‚é
		if (fdc.mt && (!(fdc.chrn[1] & 0x01))) {
			// ƒTƒCƒh1‚Ö
			fdc.chrn[1] |= 0x01;
			fdc.hd |= 0x04;
			return FALSE;
		}
		// C+1, R=1‚ÅI—¹
		fdc.chrn[0]++;
		return FALSE;
	}

	// EOTƒ`ƒFƒbƒN
	if (fdc.chrn[2] < fdc.eot) {
		fdc.chrn[2]++;
		return TRUE;
	}

	// EOTBR=1
	fdc.err |= FDD_EOT;
	fdc.chrn[2] = 0x01;

	// MT‚É‚æ‚Á‚Ä•ª‚¯‚é
	if (fdc.mt && (!(fdc.chrn[1] & 0x01))) {
		// ƒTƒCƒh1‚Ö
		fdc.chrn[1] |= 0x01;
		fdc.hd |= 0x04;
		return TRUE;
	}

	// C+1, R=1‚ÅI—¹
	fdc.chrn[0]++;
	return FALSE;
}
