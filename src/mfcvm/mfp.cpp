//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 ‚o‚hD(ytanaka@ipc-tokai.or.jp)
//	[ MFP(MC68901) ]
//
//---------------------------------------------------------------------------

#include "os.h"
#include "xm6.h"
#include "cpu.h"
#include "vm.h"
#include "log.h"
#include "event.h"
#include "schedule.h"
#include "keyboard.h"
#include "fileio.h"
#include "sync.h"
#include "mfp.h"

//===========================================================================
//
//	MFP
//
//===========================================================================
//#define MFP_LOG

//---------------------------------------------------------------------------
//
//	ƒRƒ“ƒXƒgƒ‰ƒNƒ^
//
//---------------------------------------------------------------------------
MFP::MFP(VM *p) : MemDevice(p)
{
	// ƒfƒoƒCƒXID‚ğ‰Šú‰»
	dev.id = MAKEID('M', 'F', 'P', ' ');
	dev.desc = "MFP (MC68901)";

	// ŠJnƒAƒhƒŒƒXAI—¹ƒAƒhƒŒƒX
	memdev.first = 0xe88000;
	memdev.last = 0xe89fff;

	// SyncƒIƒuƒWƒFƒNƒg
	sync = NULL;
}

//---------------------------------------------------------------------------
//
//	‰Šú‰»
//
//---------------------------------------------------------------------------
BOOL FASTCALL MFP::Init()
{
	int i;
	char buf[0x20];

	ASSERT(this);

	// Šî–{ƒNƒ‰ƒX
	if (!MemDevice::Init()) {
		return FALSE;
	}

	// Syncì¬
	sync = new Sync;

	// ƒ^ƒCƒ}ƒCƒxƒ“ƒg‰Šú‰»
	for (i=0; i<4; i++) {
		timer[i].SetDevice(this);
		sprintf(buf, "Timer-%c", 'A' + i);
		timer[i].SetDesc(buf);
		timer[i].SetUser(i);
		timer[i].SetTime(0);

		// Timer-B‚ÍƒCƒxƒ“ƒgƒJƒEƒ“ƒg‚Åg‚¤‹@‰ï‚ª‚È‚¢‚½‚ßAŠO‚·
		if (i != 1) {
			scheduler->AddEvent(&timer[i]);
		}
	}

	// ƒL[ƒ{[ƒh‚ğæ“¾
	keyboard = (Keyboard*)vm->SearchDevice(MAKEID('K', 'E', 'Y', 'B'));

	// USARTƒCƒxƒ“ƒg‰Šú‰»
	// 1(us)x13(‰ñ)x(ƒfƒ…[ƒeƒB50%)x16(•ªü)x10(bit)‚Å–ñ2400bps
	usart.SetDevice(this);
	usart.SetUser(4);
	usart.SetDesc("USART 2400bps");
	usart.SetTime(8320);
	scheduler->AddEvent(&usart);

	// ƒŠƒZƒbƒg‚Å‚Í‰Šú‰»‚³‚ê‚È‚¢ƒŒƒWƒXƒ^‚ğİ’è(ƒf[ƒ^ƒV[ƒg3.3)
	for (i=0; i<4; i++) {
		if (i == 0) {
			// ƒ^ƒCƒ}A‚Í1‚É‚µ‚ÄAVDISPST‚ª‚·‚®‹N‚±‚é‚æ‚¤‚É‚·‚é(DiskX)
			SetTDR(i, 1);
			mfp.tir[i] = 1;
		}
		else {
			// ƒ^ƒCƒ}B,ƒ^ƒCƒ}C,ƒ^ƒCƒ}D‚Í0
			SetTDR(i, 0);
			mfp.tir[i] = 0;
		}
	}
	mfp.tsr = 0;
	mfp.rur = 0;

	// ƒŠƒZƒbƒg‚ÉƒL[ƒ{[ƒh‚©‚ç$FF‚ª‘—M‚³‚ê‚é‚½‚ßAInit‚Å‰Šú‰»
	sync->Lock();
	mfp.datacount = 0;
	mfp.readpoint = 0;
	mfp.writepoint = 0;
	sync->Unlock();

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	ƒNƒŠ[ƒ“ƒAƒbƒv
//
//---------------------------------------------------------------------------
void FASTCALL MFP::Cleanup()
{
	ASSERT(this);

	// Syncíœ
	if (sync) {
		delete sync;
		sync = NULL;
	}

	// Šî–{ƒNƒ‰ƒX‚Ö
	MemDevice::Cleanup();
}

//---------------------------------------------------------------------------
//
//	ƒŠƒZƒbƒg
//
//---------------------------------------------------------------------------
void FASTCALL MFP::Reset()
{
	int i;

	ASSERT(this);
	LOG0(Log::Normal, "ƒŠƒZƒbƒg");

	// Š„‚è‚İƒRƒ“ƒgƒ[ƒ‹
	mfp.vr = 0;
	mfp.iidx = -1;
	for (i=0; i<0x10; i++) {
		mfp.ier[i] = FALSE;
		mfp.ipr[i] = FALSE;
		mfp.imr[i] = FALSE;
		mfp.isr[i] = FALSE;
		mfp.ireq[i] = FALSE;
	}

	// ƒ^ƒCƒ}
	for (i=0; i<4; i++) {
		timer[i].SetTime(0);
		SetTCR(i, 0);
	}
	mfp.tbr[0] = 0;
	mfp.tbr[1] = 0;
	mfp.sram = 0;
	mfp.tecnt = 0;

	// GPIP (GPIP5‚Íí‚ÉHƒŒƒxƒ‹)
	mfp.gpdr = 0;
	mfp.aer = 0;
	mfp.ddr = 0;
	mfp.ber = (DWORD)~mfp.aer;
	mfp.ber ^= mfp.gpdr;
	SetGPIP(5, 1);

	// USART
	mfp.scr = 0;
	mfp.ucr = 0;
	mfp.rsr = 0;
	mfp.tsr = (DWORD)(mfp.tsr & ~0x01);
	mfp.tur = 0;

	// GPIP‰Šú‰»(“dŒ¹ŠÖ˜A)
	SetGPIP(1, 1);
	if (vm->IsPowerSW()) {
		SetGPIP(2, 0);
	}
	else {
		SetGPIP(2, 1);
	}
}

//---------------------------------------------------------------------------
//
//	ƒZ[ƒu
//
//---------------------------------------------------------------------------
BOOL FASTCALL MFP::Save(Fileio *fio, int ver)
{
	size_t sz;
	int i;

	ASSERT(this);
	LOG0(Log::Normal, "ƒZ[ƒu");

	// –{‘Ì
	sz = sizeof(mfp_t);
	if (!fio->Write(&sz, sizeof(sz))) {
		return FALSE;
	}
	if (!fio->Write(&mfp, (int)sz)) {
		return FALSE;
	}

	// ƒCƒxƒ“ƒg(ƒ^ƒCƒ})
	for (i=0; i<4; i++) {
		if (!timer[i].Save(fio, ver)) {
			return FALSE;
		}
	}

	// ƒCƒxƒ“ƒg(USART)
	if (!usart.Save(fio, ver)) {
		return FALSE;
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	ƒ[ƒh
//
//---------------------------------------------------------------------------
BOOL FASTCALL MFP::Load(Fileio *fio, int ver)
{
	int i;
	size_t sz;

	ASSERT(this);
	LOG0(Log::Normal, "ƒ[ƒh");

	// –{‘Ì
	if (!fio->Read(&sz, sizeof(sz))) {
		return FALSE;
	}
	if (sz != sizeof(mfp_t)) {
		return FALSE;
	}
	if (!fio->Read(&mfp, (int)sz)) {
		return FALSE;
	}

	// ƒCƒxƒ“ƒg(ƒ^ƒCƒ})
	for (i=0; i<4; i++) {
		if (!timer[i].Load(fio, ver)) {
			return FALSE;
		}
	}

	// ƒCƒxƒ“ƒg(USART)
	if (!usart.Load(fio, ver)) {
		return FALSE;
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	İ’è“K—p
//
//---------------------------------------------------------------------------
void FASTCALL MFP::ApplyCfg(const Config* /*config*/)
{
	ASSERT(this);

	LOG0(Log::Normal, "İ’è“K—p");
}

//---------------------------------------------------------------------------
//
//	ƒoƒCƒg“Ç‚İ‚İ
//
//---------------------------------------------------------------------------
DWORD FASTCALL MFP::ReadByte(DWORD addr)
{
	DWORD data;

	ASSERT(this);
	ASSERT((addr >= memdev.first) && (addr <= memdev.last));

	// Šï”ƒAƒhƒŒƒX‚Ì‚İƒfƒR[ƒh‚³‚ê‚Ä‚¢‚é
	if ((addr & 1) != 0) {
		// ƒEƒFƒCƒg
		scheduler->Wait(3);

		// 64ƒoƒCƒg’PˆÊ‚Åƒ‹[ƒv
		addr &= 0x3f;
		addr >>= 1;

		switch (addr) {
			// GPIP
			case 0x00:
				return mfp.gpdr;

			// AER
			case 0x01:
				return mfp.aer;

			// DDR
			case 0x02:
				return mfp.ddr;

			// IER(A)
			case 0x03:
				return GetIER(0);

			// IER(B)
			case 0x04:
				return GetIER(1);

			// IPR(A)
			case 0x05:
				return GetIPR(0);

			// IPR(B)
			case 0x06:
				return GetIPR(1);

			// ISR(A)
			case 0x07:
				return GetISR(0);

			// ISR(B)
			case 0x08:
				return GetISR(1);

			// IMR(A)
			case 0x09:
				return GetIMR(0);

			// IMR(B)
			case 0x0a:
				return GetIMR(1);

			// VR
			case 0x0b:
				return GetVR();

			// ƒ^ƒCƒ}AƒRƒ“ƒgƒ[ƒ‹
			case 0x0c:
				return GetTCR(0);

			// ƒ^ƒCƒ}BƒRƒ“ƒgƒ[ƒ‹
			case 0x0d:
				return GetTCR(1);

			// ƒ^ƒCƒ}C&DƒRƒ“ƒgƒ[ƒ‹
			case 0x0e:
				data = GetTCR(2);
				data <<= 4;
				data |= GetTCR(3);
				return data;

			// ƒ^ƒCƒ}Aƒf[ƒ^
			case 0x0f:
				return GetTIR(0);

			// ƒ^ƒCƒ}Bƒf[ƒ^
			case 0x10:
				return GetTIR(1);

			// ƒ^ƒCƒ}Cƒf[ƒ^
			case 0x11:
				return GetTIR(2);

			// ƒ^ƒCƒ}Dƒf[ƒ^
			case 0x12:
				return GetTIR(3);

			// SYNCƒLƒƒƒ‰ƒNƒ^
			case 0x13:
				return mfp.scr;

			// USARTƒRƒ“ƒgƒ[ƒ‹
			case 0x14:
				return mfp.ucr;

			// ƒŒƒV[ƒoƒXƒe[ƒ^ƒX
			case 0x15:
				return mfp.rsr;

			// ƒgƒ‰ƒ“ƒXƒ~ƒbƒ^ƒXƒe[ƒ^ƒX
			case 0x16:
				// TEƒrƒbƒg‚ÍƒNƒŠƒA‚³‚ê‚é
				mfp.tsr = (DWORD)(mfp.tsr & ~0x40);
				return mfp.tsr;

			// USARTƒf[ƒ^
			case 0x17:
				Receive();
				return mfp.rur;

			// ‚»‚êˆÈŠO
			default:
				LOG1(Log::Warning, "–¢À‘•ƒŒƒWƒXƒ^“Ç‚İ‚İ R%02d", addr);
				return 0xff;
		}
	}

	return 0xff;
}

//---------------------------------------------------------------------------
//
//	ƒ[ƒh“Ç‚İ‚İ
//
//---------------------------------------------------------------------------
DWORD FASTCALL MFP::ReadWord(DWORD addr)
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
void FASTCALL MFP::WriteByte(DWORD addr, DWORD data)
{
	ASSERT(this);
	ASSERT((addr >= memdev.first) && (addr <= memdev.last));
	ASSERT(data < 0x100);

	// Šï”ƒAƒhƒŒƒX‚Ì‚İƒfƒR[ƒh‚³‚ê‚Ä‚¢‚é
	if ((addr & 1) == 0) {
		// ƒoƒXƒGƒ‰[‚Í”­¶‚µ‚È‚¢
		return;
	}

	// 64ƒoƒCƒg’PˆÊ‚Åƒ‹[ƒv
	addr &= 0x3f;
	addr >>= 1;

	// ƒEƒFƒCƒg
	scheduler->Wait(3);

	switch (addr) {
		// GPIP
		case 0x00:
			// VDISP‚ğAND.B‚Åƒ`ƒFƒbƒN‚·‚éê‡‚ª‚ ‚é(MOON FIGHTER)
			SetGPDR(data);
			return;

		// AER
		case 0x01:
			mfp.aer = data;
			mfp.ber = (DWORD)(~data);
			mfp.ber ^= mfp.gpdr;
			IntGPIP();
			return;

		// DDR
		case 0x02:
			mfp.ddr = data;
			if (mfp.ddr != 0) {
				LOG0(Log::Warning, "GPIPo—ÍƒfƒBƒŒƒNƒVƒ‡ƒ“");
			}
			return;

		// IER(A)
		case 0x03:
			SetIER(0, data);
			return;

		// IER(B)
		case 0x04:
			SetIER(1, data);
			return;

		// IPR(A)
		case 0x05:
			SetIPR(0, data);
			return;

		// IPR(B)
		case 0x06:
			SetIPR(1, data);
			return;

		// ISR(A)
		case 0x07:
			SetISR(0, data);
			return;

		// ISR(B)
		case 0x08:
			SetISR(1, data);
			return;

		// IMR(A)
		case 0x09:
			SetIMR(0, data);
			return;

		// IMR(B)
		case 0x0a:
			SetIMR(1, data);
			return;

		// VR
		case 0x0b:
			SetVR(data);
			return;

		// ƒ^ƒCƒ}AƒRƒ“ƒgƒ[ƒ‹
		case 0x0c:
			SetTCR(0, data);
			return;

		// ƒ^ƒCƒ}BƒRƒ“ƒgƒ[ƒ‹
		case 0x0d:
			SetTCR(1, data);
			return;

		// ƒ^ƒCƒ}C&DƒRƒ“ƒgƒ[ƒ‹
		case 0x0e:
			SetTCR(2, (DWORD)(data >> 4));
			SetTCR(3, (DWORD)(data & 0x0f));
			return;

		// ƒ^ƒCƒ}Aƒf[ƒ^
		case 0x0f:
			SetTDR(0, data);
			return;

		// ƒ^ƒCƒ}Bƒf[ƒ^
		case 0x10:
			SetTDR(1, data);
			return;

		// ƒ^ƒCƒ}Cƒf[ƒ^
		case 0x11:
			SetTDR(2, data);
			return;

		// ƒ^ƒCƒ}Dƒf[ƒ^
		case 0x12:
			SetTDR(3, data);
			return;

		// SYNCƒLƒƒƒ‰ƒNƒ^
		case 0x13:
			mfp.scr = data;
			return;

		// USARTƒRƒ“ƒgƒ[ƒ‹
		case 0x14:
			if (data != 0x88) {
				LOG1(Log::Warning, "USART ƒpƒ‰ƒ[ƒ^ƒGƒ‰[ %02X", data);
			}
			mfp.ucr = data;
			return;

		// ƒŒƒV[ƒoƒXƒe[ƒ^ƒX
		case 0x15:
			SetRSR(data);
			return;

		// ƒgƒ‰ƒ“ƒXƒ~ƒbƒ^ƒXƒe[ƒ^ƒX
		case 0x16:
			SetTSR(data);
			return;

		// USARTƒf[ƒ^
		case 0x17:
			Transmit(data);
			return;
	}

	LOG2(Log::Warning, "–¢À‘•ƒŒƒWƒXƒ^‘‚«‚İ R%02d <- $%02X",
							addr, data);
}

//---------------------------------------------------------------------------
//
//	ƒ[ƒh‘‚«‚İ
//
//---------------------------------------------------------------------------
void FASTCALL MFP::WriteWord(DWORD addr, DWORD data)
{
	ASSERT(this);
	ASSERT((addr >= memdev.first) && (addr <= memdev.last));
	ASSERT((addr & 1) == 0);
	ASSERT(data < 0x10000);

	WriteByte(addr + 1, (BYTE)data);
}

//---------------------------------------------------------------------------
//
//	“Ç‚İ‚İ‚Ì‚İ
//
//---------------------------------------------------------------------------
DWORD FASTCALL MFP::ReadOnly(DWORD addr) const
{
	DWORD data;

	ASSERT(this);
	ASSERT((addr >= memdev.first) && (addr <= memdev.last));

	// Šï”ƒAƒhƒŒƒX‚Ì‚İƒfƒR[ƒh‚³‚ê‚Ä‚¢‚é
	if ((addr & 1) == 0) {
		return 0xff;
	}

	// 64ƒoƒCƒg’PˆÊ‚Åƒ‹[ƒv
	addr &= 0x3f;
	addr >>= 1;

	switch (addr) {
		// GPIP
		case 0x00:
			return mfp.gpdr;

		// AER
		case 0x01:
			return mfp.aer;

		// DDR
		case 0x02:
			return mfp.ddr;

		// IER(A)
		case 0x03:
			return GetIER(0);

		// IER(B)
		case 0x04:
			return GetIER(1);

		// IPR(A)
		case 0x05:
			return GetIPR(0);

		// IPR(B)
		case 0x06:
			return GetIPR(1);

		// ISR(A)
		case 0x07:
			return GetISR(0);

		// ISR(B)
		case 0x08:
			return GetISR(1);

		// IMR(A)
		case 0x09:
			return GetIMR(0);

		// IMR(B)
		case 0x0a:
			return GetIMR(1);

		// VR
		case 0x0b:
			return GetVR();

		// ƒ^ƒCƒ}AƒRƒ“ƒgƒ[ƒ‹
		case 0x0c:
			return mfp.tcr[0];

		// ƒ^ƒCƒ}BƒRƒ“ƒgƒ[ƒ‹
		case 0x0d:
			return mfp.tcr[1];

		// ƒ^ƒCƒ}C&DƒRƒ“ƒgƒ[ƒ‹
		case 0x0e:
			data = mfp.tcr[2];
			data <<= 4;
			data |= mfp.tcr[3];
			return data;

		// ƒ^ƒCƒ}Aƒf[ƒ^
		case 0x0f:
			return mfp.tir[0];

		// ƒ^ƒCƒ}Bƒf[ƒ^(ƒ‰ƒ“ƒ_ƒ€‚È’l‚ğ•Ô‚·)
		case 0x10:
			return ((scheduler->GetTotalTime() % 13) + 1);

		// ƒ^ƒCƒ}Cƒf[ƒ^
		case 0x11:
			return mfp.tir[2];

		// ƒ^ƒCƒ}Dƒf[ƒ^
		case 0x12:
			return mfp.tir[3];

		// SYNCƒLƒƒƒ‰ƒNƒ^
		case 0x13:
			return mfp.scr;

		// USARTƒRƒ“ƒgƒ[ƒ‹
		case 0x14:
			return mfp.ucr;

		// ƒŒƒV[ƒoƒXƒe[ƒ^ƒX
		case 0x15:
			return mfp.rsr;

		// ƒgƒ‰ƒ“ƒXƒ~ƒbƒ^ƒXƒe[ƒ^ƒX
		case 0x16:
			return mfp.tsr;

		// USARTƒf[ƒ^
		case 0x17:
			return mfp.rur;
	}

	return 0xff;
}

//---------------------------------------------------------------------------
//
//	“à•”ƒf[ƒ^æ“¾
//
//---------------------------------------------------------------------------
void FASTCALL MFP::GetMFP(mfp_t *buffer) const
{
	ASSERT(this);
	ASSERT(buffer);

	// ƒf[ƒ^‚ğƒRƒs[
	*buffer = mfp;
}

//---------------------------------------------------------------------------
//
//	Š„‚è‚İ
//
//---------------------------------------------------------------------------
void FASTCALL MFP::Interrupt(int level, BOOL enable)
{
	int index;

	ASSERT(this);
	ASSERT((level >= 0) && (level < 0x10));

	index = 15 - level;
	if (enable) {
		// Šù‚É—v‹‚³‚ê‚Ä‚¢‚é‚©
		if (mfp.ireq[index]) {
			return;
		}

		// ƒCƒl[ƒuƒ‹ƒŒƒWƒXƒ^‚Í‚Ç‚¤‚©
		if (!mfp.ier[index]) {
			return;
		}

		// ƒtƒ‰ƒOUpAŠ„‚è‚İƒ`ƒFƒbƒN
		mfp.ireq[index] = TRUE;
		IntCheck();
	}
	else {
		// Šù‚Éæ‚èÁ‚µˆ—‚µ‚½‚©A—v‹ó—‚³‚ê‚½Œã‚©
		if (!mfp.ireq[index] && !mfp.ipr[index]) {
			return;
		}
		mfp.ireq[index] = FALSE;

		// ƒyƒ“ƒfƒBƒ“ƒO‚ğ‰º‚°AŠ„‚è‚İ‰ğœ‚·‚é
		mfp.ipr[index] = FALSE;
		IntCheck();
	}
}

//---------------------------------------------------------------------------
//
//	Š„‚è‚İ—Dæ‡ˆÊ”»’è
//
//---------------------------------------------------------------------------
void FASTCALL MFP::IntCheck()
{
	int i;
#if defined(MFP_LOG)
	char buffer[0x40];
#endif	// MFP_LOG

	ASSERT(this);

	// Š„‚è‚İƒLƒƒƒ“ƒZƒ‹ƒ`ƒFƒbƒN
	if (mfp.iidx >= 0) {
		// ƒyƒ“ƒfƒBƒ“ƒO‰ğœ‚Ü‚½‚Íƒ}ƒXƒN‚ÅAŠ„‚è‚İæ‚è‰º‚°(ƒf[ƒ^ƒV[ƒg2.7.1)
		if (!mfp.ipr[mfp.iidx] || !mfp.imr[mfp.iidx]) {
			cpu->IntCancel(6);
#if defined(MFP_LOG)
			sprintf(buffer, "Š„‚è‚İæ‚èÁ‚µ %s", IntDesc[mfp.iidx]);
			LOG0(Log::Normal, buffer);
#endif	// MFP_LOG
			mfp.iidx = -1;
		}
	}

	// Š„‚è‚İ”­¶ó—
	for (i=0; i<0x10; i++) {
		// Š„‚è‚İƒCƒl[ƒuƒ‹‚©
		if (mfp.ier[i]) {
			// Š„‚è‚İƒŠƒNƒGƒXƒg‚ª‚ ‚é‚©
			if (mfp.ireq[i]) {
				// ƒyƒ“ƒfƒBƒ“ƒOƒŒƒWƒXƒ^‚ğ1‚É‚·‚é(Š„‚è‚İó—)
				mfp.ipr[i] = TRUE;
				mfp.ireq[i] = FALSE;
			}
		}
		else {
			// ƒCƒl[ƒuƒ‹ƒŒƒWƒXƒ^0‚ÅAŠ„‚è‚İƒyƒ“ƒfƒBƒ“ƒO‰ğœ
			mfp.ipr[i] = FALSE;
			mfp.ireq[i] = FALSE;
		}
	}

	// Š„‚è‚İƒxƒNƒ^‘—o
	for (i=0; i<0x10; i++) {
		// Š„‚è‚İƒyƒ“ƒfƒBƒ“ƒO‚©
		if (!mfp.ipr[i]) {
			continue;
		}

		// Š„‚è‚İƒ}ƒXƒN‚©
		if (!mfp.imr[i]) {
			continue;
		}

		// ƒT[ƒrƒX’†‚Å‚È‚¢‚©
		if (mfp.isr[i]) {
			continue;
		}

		// Šù‚É—v‹‚µ‚Ä‚¢‚éŠ„‚è‚İ‚æ‚èãˆÊ‚È‚çAŠ„‚è‚İ‚ğæ‚Áæ‚é
		if (mfp.iidx > i) {
			cpu->IntCancel(6);
#if defined(MFP_LOG)
			sprintf(buffer, "Š„‚è‚İ—Dææ‚èÁ‚µ %s", IntDesc[mfp.iidx]);
			LOG0(Log::Normal, buffer);
#endif	// MFP_LOG
			mfp.iidx = -1;
		}

		// ƒxƒNƒ^‘—o
		if (cpu->Interrupt(6, (mfp.vr & 0xf0) + (15 - i))) {
			// CPU‚Éó‚¯•t‚¯‚ç‚ê‚½BƒCƒ“ƒfƒbƒNƒX‹L‰¯
#if defined(MFP_LOG)
			sprintf(buffer, "Š„‚è‚İ—v‹ %s", IntDesc[i]);
			LOG0(Log::Normal, buffer);
#endif	// MFP_LOG
			mfp.iidx = i;
			break;
		}
	}
}

//---------------------------------------------------------------------------
//
//	Š„‚è‚İ‰“š
//
//---------------------------------------------------------------------------
void FASTCALL MFP::IntAck()
{
#if defined(MFP_LOG)
	char buffer[0x40];
#endif	// MFP_LOG

	ASSERT(this);

	// ƒŠƒZƒbƒg’¼Œã‚ÉACPU‚©‚çŠ„‚è‚İ‚ªŠÔˆá‚Á‚Ä“ü‚éê‡‚ª‚ ‚é
	if (mfp.iidx < 0) {
		LOG0(Log::Warning, "—v‹‚µ‚Ä‚¢‚È‚¢Š„‚è‚İ");
		return;
	}

#if defined(MFP_LOG)
	sprintf(buffer, "Š„‚è‚İ‰“š %s", IntDesc[mfp.iidx]);
	LOG0(Log::Normal, buffer);
#endif	// MFP_LOG

	// Š„‚è‚İ‚ªó‚¯•t‚¯‚ç‚ê‚½Bƒyƒ“ƒfƒBƒ“ƒO‰ğœ
	mfp.ipr[mfp.iidx] = FALSE;

	// ƒCƒ“ƒT[ƒrƒX(ƒI[ƒgEOI‚Å0Aƒ}ƒjƒ…ƒAƒ‹EOI‚Å1)
	if (mfp.vr & 0x08) {
		mfp.isr[mfp.iidx] = TRUE;
	}
	else {
		mfp.isr[mfp.iidx] = FALSE;
	}

	// ƒCƒ“ƒfƒbƒNƒX‚ğŠ„‚è‚İ–³‚µ‚É•ÏX
	mfp.iidx = -1;

	// Ä“xAŠ„‚è‚İƒ`ƒFƒbƒN‚ğs‚¤
	IntCheck();
}

//---------------------------------------------------------------------------
//
//	IERİ’è
//
//---------------------------------------------------------------------------
void FASTCALL MFP::SetIER(int offset, DWORD data)
{
	int i;
#if defined(MFP_LOG)
	char buffer[0x40];
#endif	// MFP_LOG

	ASSERT(this);
	ASSERT((offset == 0) || (offset == 1));
	ASSERT(data < 0x100);

	// ‰Šúİ’è
	offset <<= 3;

	// 8‰ñAƒrƒbƒg•ª—£
	for (i=offset; i<offset+8; i++) {
		if (data & 0x80) {
			mfp.ier[i] = TRUE;
		}
		else {
			// IPR‚ğ‰º‚°‚é(ƒf[ƒ^ƒV[ƒg4.3.1)
			mfp.ier[i] = FALSE;
			mfp.ipr[i] = FALSE;
		}

		data <<= 1;
	}

	// Š„‚è‚İ—Dæ‡ˆÊ
	IntCheck();
}

//---------------------------------------------------------------------------
//
//	IERæ“¾
//
//---------------------------------------------------------------------------
DWORD FASTCALL MFP::GetIER(int offset) const
{
	int i;
	DWORD bit;

	ASSERT(this);
	ASSERT((offset == 0) || (offset == 1));

	// ‰Šúİ’è
	offset <<= 3;
	bit = 0;

	// 8‰ñAƒrƒbƒg‡¬
	for (i=offset; i<offset+8; i++) {
		bit <<= 1;
		if (mfp.ier[i]) {
			bit |= 0x01;
		}
	}

	return bit;
}

//---------------------------------------------------------------------------
//
//	IPRİ’è
//
//---------------------------------------------------------------------------
void FASTCALL MFP::SetIPR(int offset, DWORD data)
{
	int i;
#if defined(MFP_LOG)
	char buffer[0x40];
#endif	// MFP_LOG

	ASSERT(this);
	ASSERT((offset == 0) || (offset == 1));

	// ‰Šúİ’è
	offset <<= 3;

	// 8‰ñAƒrƒbƒg•ª—£
	for (i=offset; i<offset+8; i++) {
		if (!(data & 0x80)) {
			// IPR‚ÍCPU‚©‚ç1‚É‚·‚é‚±‚Æ‚Ío—ˆ‚È‚¢
			mfp.ipr[i] = FALSE;
		}

		data <<= 1;
	}

	// Š„‚è‚İ—Dæ‡ˆÊ
	IntCheck();
}

//---------------------------------------------------------------------------
//
//	IPRæ“¾
//
//---------------------------------------------------------------------------
DWORD FASTCALL MFP::GetIPR(int offset) const
{
	int i;
	DWORD bit;

	ASSERT(this);
	ASSERT((offset == 0) || (offset == 1));

	// ‰Šúİ’è
	offset <<= 3;
	bit = 0;

	// 8‰ñAƒrƒbƒg‡¬
	for (i=offset; i<offset+8; i++) {
		bit <<= 1;
		if (mfp.ipr[i]) {
			bit |= 0x01;
		}
	}

	return bit;
}

//---------------------------------------------------------------------------
//
//	ISRİ’è
//
//---------------------------------------------------------------------------
void FASTCALL MFP::SetISR(int offset, DWORD data)
{
	int i;
#if defined(MFP_LOG)
	char buffer[0x40];
#endif	// MFP_LOG

	ASSERT(this);
	ASSERT((offset == 0) || (offset == 1));
	ASSERT(data < 0x100);

	// ‰Šúİ’è
	offset <<= 3;

	// 8‰ñAƒrƒbƒg•ª—£
	for (i=offset; i<offset+8; i++) {
		if (!(data & 0x80)) {
			mfp.isr[i] = FALSE;
		}

		data <<= 1;
	}

	// Š„‚è‚İ—Dæ‡ˆÊ
	IntCheck();
}

//---------------------------------------------------------------------------
//
//	ISRæ“¾
//
//---------------------------------------------------------------------------
DWORD FASTCALL MFP::GetISR(int offset) const
{
	int i;
	DWORD bit;

	ASSERT(this);
	ASSERT((offset == 0) || (offset == 1));

	// ‰Šúİ’è
	offset <<= 3;
	bit = 0;

	// 8‰ñAƒrƒbƒg‡¬
	for (i=offset; i<offset+8; i++) {
		bit <<= 1;
		if (mfp.isr[i]) {
			bit |= 0x01;
		}
	}

	return bit;
}

//---------------------------------------------------------------------------
//
//	IMRİ’è
//
//---------------------------------------------------------------------------
void FASTCALL MFP::SetIMR(int offset, DWORD data)
{
	int i;
#if defined(MFP_LOG)
	char buffer[0x40];
#endif	// MFP_LOG

	ASSERT(this);
	ASSERT((offset == 0) || (offset == 1));
	ASSERT(data < 0x100);

	// ‰Šúİ’è
	offset <<= 3;

	// 8‰ñAƒrƒbƒg•ª—£
	for (i=offset; i<offset+8; i++) {
		if (data & 0x80) {
			mfp.imr[i] = TRUE;
		}
		else {
			mfp.imr[i] = FALSE;
		}

		data <<= 1;
	}

	// Š„‚è‚İ—Dæ‡ˆÊ
	IntCheck();
}

//---------------------------------------------------------------------------
//
//	IMRæ“¾
//
//---------------------------------------------------------------------------
DWORD FASTCALL MFP::GetIMR(int offset) const
{
	int i;
	DWORD bit;

	ASSERT(this);
	ASSERT((offset == 0) || (offset == 1));

	// ‰Šúİ’è
	offset <<= 3;
	bit = 0;

	// 8‰ñAƒrƒbƒg‡¬
	for (i=offset; i<offset+8; i++) {
		bit <<= 1;
		if (mfp.imr[i]) {
			bit |= 0x01;
		}
	}

	return bit;
}

//---------------------------------------------------------------------------
//
//	VRİ’è
//
//---------------------------------------------------------------------------
void FASTCALL MFP::SetVR(DWORD data)
{
	ASSERT(this);
	ASSERT(data < 0x100);

	if (mfp.vr != data) {
		mfp.vr = data;
		LOG1(Log::Detail, "Š„‚è‚İƒxƒNƒ^ƒx[ƒX $%02X", data & 0xf0);

		if (mfp.vr & 0x08) {
			LOG0(Log::Warning, "ƒ}ƒjƒ…ƒAƒ‹EOIƒ‚[ƒh");
		}
	}
}

//---------------------------------------------------------------------------
//
//	VRæ“¾
//
//---------------------------------------------------------------------------
DWORD FASTCALL MFP::GetVR() const
{
	ASSERT(this);
	return mfp.vr;
}

//---------------------------------------------------------------------------
//
//	Š„‚è‚İ–¼Ìƒe[ƒuƒ‹
//
//---------------------------------------------------------------------------
const char* MFP::IntDesc[0x10] = {
	"H-SYNC",
	"CIRQ",
	"Timer-A",
	"RxFull",
	"RxError",
	"TxEmpty",
	"TxError",
	"Timer-B",
	"(NoUse)",
	"V-DISP",
	"Timer-C",
	"Timer-D",
	"FMIRQ",
	"POW SW",
	"EXPON",
	"ALARM"
};

//---------------------------------------------------------------------------
//
//	ƒCƒxƒ“ƒgƒR[ƒ‹ƒoƒbƒN(ƒfƒBƒŒƒCƒ‚[ƒh‚Åg—p)
//
//---------------------------------------------------------------------------
BOOL FASTCALL MFP::Callback(Event *ev)
{
	int channel;
	DWORD low;

	ASSERT(this);
	ASSERT(ev);

	// ƒ†[ƒUƒf[ƒ^‚©‚çí•Ê‚ğ“¾‚é
	channel = (int)ev->GetUser();

	// ƒ^ƒCƒ}
	if ((channel >= 0) && (channel <= 3)) {
		low = (mfp.tcr[channel] & 0x0f);

		// ƒ^ƒCƒ}ƒIƒ“‚©
		if (low == 0) {
			// ƒ^ƒCƒ}ƒIƒt
			return FALSE;
		}

		// ƒfƒBƒŒƒCƒ‚[ƒh‚©
		if (low & 0x08) {
			// ƒJƒEƒ“ƒg0‚©‚ç­‚µ’x‚ê‚ÄAŠ„‚è‚İ”­¶
			Interrupt(TimerInt[channel], TRUE);

			// ƒƒ“ƒVƒ‡ƒbƒg
			return FALSE;
		}

		// ƒ^ƒCƒ}‚ği‚ß‚é
		Proceed(channel);
		return TRUE;
	}

	// USART
	ASSERT(channel == 4);
	USART();
	return TRUE;
}

//---------------------------------------------------------------------------
//
//	ƒ^ƒCƒ}ƒCƒxƒ“ƒg“ü—Í(ƒCƒxƒ“ƒgƒJƒEƒ“ƒgƒ‚[ƒh‚Åg—p)
//
//---------------------------------------------------------------------------
void FASTCALL MFP::EventCount(int channel, int value)
{
	DWORD edge;
	BOOL flag;

	ASSERT(this);
	ASSERT((channel >= 0) && (channel <= 1));
	ASSERT((value == 0) || (value == 1));
	ASSERT((mfp.tbr[channel] == 0) || (mfp.tbr[channel] == 1));

	// ƒCƒxƒ“ƒgƒJƒEƒ“ƒgƒ‚[ƒh‚©(ƒ^ƒCƒ}A,B‚Ì‚İ)
	if ((mfp.tcr[channel] & 0x0f) == 0x08) {
		// •ûŒü‚ÍGPIP4, GPIP3‚ÅŒˆ‚Ü‚é
		if (channel == 0) {
			edge = mfp.aer & 0x10;
		}
		else {
			edge = mfp.aer & 0x08;
		}

		// ƒtƒ‰ƒOƒIƒt
		flag = FALSE;

		// ƒGƒbƒW”»’è
		if (edge == 1) {
			// ƒGƒbƒW‚ª1‚Ì‚Æ‚«A0¨1‚Åƒ^ƒCƒ}‚ği‚ß‚é
			if ((mfp.tbr[channel] == 0) && (value == 1)) {
				flag = TRUE;
			}
		}
		else {
			// ƒGƒbƒW‚ª0‚Ì‚Æ‚«A1¨0‚Åƒ^ƒCƒ}‚ği‚ß‚é
			if ((mfp.tbr[channel] == 1) && (value == 0)) {
				flag = TRUE;
			}
		}

		// ƒ^ƒCƒ}‚ği‚ß‚é
		if (flag) {
			Proceed(channel);
		}
	}

	// TBR‚ğXV
	mfp.tbr[channel] = (DWORD)value;
}

//---------------------------------------------------------------------------
//
//	TCRİ’è
//
//---------------------------------------------------------------------------
void FASTCALL MFP::SetTCR(int channel, DWORD data)
{
	DWORD prev;
	DWORD now;
	DWORD speed;

	ASSERT(this);
	ASSERT((channel >= 0) && (channel <= 3));
	ASSERT(data < 0x100);

	// Œ^•ÏŠ·Aˆê’vƒ`ƒFƒbƒN
	now = data;
	now &= 0x0f;
	prev = mfp.tcr[channel];
	if (now == prev) {
		return;
	}
	mfp.tcr[channel] = now;

	// ƒ^ƒCƒ}B‚Í0x01‚Ì‚İ‰Â(ƒGƒ~ƒ…ƒŒ[ƒVƒ‡ƒ“‚µ‚È‚¢)
	if (channel == 1) {
		if ((now != 0x01) && (now != 0x00)) {
			LOG1(Log::Warning, "ƒ^ƒCƒ}BƒRƒ“ƒgƒ[ƒ‹ $%02X", now);
		}
		now = 0;
	}

	// ƒ^ƒCƒ}ƒXƒgƒbƒv‚©
	if (now == 0) {
#if defined(MFP_LOG)
		LOG1(Log::Normal, "ƒ^ƒCƒ}%c ’â~", channel + 'A');
#endif	// MFP_LOG
		timer[channel].SetTime(0);

		// Š„‚è‚İ‚Ìæ‚è‰º‚°‚ğs‚¤•K—v‚ ‚è(ˆ«–‚éƒhƒ‰ƒLƒ…ƒ‰)
		Interrupt(TimerInt[channel], FALSE);

		// Timer-D‚ÍCPUƒNƒƒbƒN‚ğ–ß‚·(CH30.SYS)
		if (channel == 3) {
			if (mfp.sram != 0) {
				// CPUƒNƒƒbƒN‚ğ–ß‚·
				scheduler->SetCPUSpeed(mfp.sram);
				mfp.sram = 0;
			}
		}
		return;
	}

	// ƒpƒ‹ƒX•‘ª’èƒ‚[ƒh‚ÍƒTƒ|[ƒg‚µ‚Ä‚¢‚È‚¢
	if (now > 0x08) {
		LOG2(Log::Warning, "ƒ^ƒCƒ}%c ƒpƒ‹ƒX•‘ª’èƒ‚[ƒh$%02X", channel + 'A', now);
		return;
	}

	// ƒCƒxƒ“ƒgƒJƒEƒ“ƒgƒ‚[ƒh‚©
	if (now == 0x08) {
#if defined(MFP_LOG)
		LOG1(Log::Normal, "ƒ^ƒCƒ}%c ƒCƒxƒ“ƒgƒJƒEƒ“ƒgƒ‚[ƒh", channel + 'A');
#endif	// MFP_LOG
		// ˆê“xƒCƒxƒ“ƒg‚ğ~‚ß‚é
		timer[channel].SetTime(0);

		// ƒ^ƒCƒ}OFF¨ON‚È‚çAƒJƒEƒ“ƒg‚ğƒ[ƒh
		if (prev == 0) {
			mfp.tir[channel] = mfp.tdr[channel];
		}
		return;
	}

	// ƒfƒBƒŒƒCƒ‚[ƒh‚Å‚ÍAƒvƒŠƒXƒP[ƒ‰‚ğİ’è
#if defined(MFP_LOG)
	LOG3(Log::Normal, "ƒ^ƒCƒ}%c ƒfƒBƒŒƒCƒ‚[ƒh %d.%dus",
				channel + 'A', (TimerHus[now] / 2), (TimerHus[now] & 1) * 5);
#endif	// MFP_LOG

	// ƒ^ƒCƒ}OFF¨ON‚È‚çAƒJƒEƒ“ƒ^‚ğƒ[ƒh(_VDISPSTƒpƒbƒ`)
	if (prev == 0) {
		mfp.tir[channel] = mfp.tdr[channel];
	}

	// ƒCƒxƒ“ƒg‚ğƒZƒbƒg
	timer[channel].SetTime(TimerHus[now]);

	// Timer-C‚Ìê‡AINFO.RAMê—p‘Îô‚ğs‚¤(CPU‘¬“xŒv‘ª‚ğ–³—‚â‚è‚ ‚í‚¹‚é)
	if (channel == 2) {
		if ((now == 3) && (mfp.sram == 0)) {
			speed = cpu->GetPC();
			if ((speed >= 0xed0100) && (speed <= 0xedffff)) {
				// CPUƒNƒƒbƒN‚ğ—‚Æ‚·
				speed = scheduler->GetCPUSpeed();
				mfp.sram = speed;
				speed *= 83;
				speed /= 96;
				scheduler->SetCPUSpeed(speed);
			}
		}
		if ((now != 3) && (mfp.sram != 0)) {
			// CPUƒNƒƒbƒN‚ğ–ß‚·
			scheduler->SetCPUSpeed(mfp.sram);
			mfp.sram = 0;
		}
	}

	// Timer-D‚Ìê‡ACH30.SYSê—p‘Îô‚ğs‚¤(CPU‘¬“xŒv‘ª‚ğ–³—‚â‚è‚ ‚í‚¹‚é)
	if (channel == 3) {
		if ((now == 7) && (mfp.sram == 0)) {
			speed = cpu->GetPC();
			if ((speed >= 0xed0100) && (speed <= 0xedffff)) {
				// CPUƒNƒƒbƒN‚ğ—‚Æ‚·
				speed = scheduler->GetCPUSpeed();
				mfp.sram = speed;
				speed *= 85;
				speed /= 96;
				scheduler->SetCPUSpeed(speed);
			}
		}
	}
}

//---------------------------------------------------------------------------
//
//	TCRæ“¾
//
//---------------------------------------------------------------------------
DWORD FASTCALL MFP::GetTCR(int channel) const
{
	ASSERT(this);
	ASSERT((channel >= 0) && (channel <= 3));

	return mfp.tcr[channel];
}

//---------------------------------------------------------------------------
//
//	TDRİ’è
//
//---------------------------------------------------------------------------
void FASTCALL MFP::SetTDR(int channel, DWORD data)
{
	ASSERT(this);
	ASSERT((channel >= 0) && (channel <= 3));
	ASSERT(data < 0x100);

	mfp.tdr[channel] = data;

	// ƒ^ƒCƒ}B‚ÍŒÅ’è’l‚Ì‚Í‚¸
	if (channel == 1) {
		if (data != 0x0d) {
			LOG1(Log::Warning, "ƒ^ƒCƒ}BƒŠƒ[ƒh’l %02X", data);
		}
	}
}

//---------------------------------------------------------------------------
//
//	TIRæ“¾
//
//---------------------------------------------------------------------------
DWORD FASTCALL MFP::GetTIR(int channel) const
{
	ASSERT(this);
	ASSERT((channel >= 0) && (channel <= 3));

	// ƒ^ƒCƒ}B‚ÍXM6‚Å‚Í“Ç‚İo‚µ‚ğ‹Ö~‚·‚é(À‹@‚Í1us x 14ƒ^ƒCƒ})
	if (channel == 1) {
		// (Œ¹•½“¢–‚“`)
		LOG0(Log::Warning, "ƒ^ƒCƒ}B ƒf[ƒ^ƒŒƒWƒXƒ^“Ç‚İo‚µ");
		return (DWORD)((scheduler->GetTotalTime() % 13) + 1);
	}

	return mfp.tir[channel];
}

//---------------------------------------------------------------------------
//
//	ƒ^ƒCƒ}‚ği‚ß‚é
//
//---------------------------------------------------------------------------
void FASTCALL MFP::Proceed(int channel)
{
	ASSERT(this);
	ASSERT((channel >= 0) && (channel <= 3));

	// ƒJƒEƒ“ƒ^‚ğŒ¸Z
	if (mfp.tir[channel] > 0) {
		mfp.tir[channel]--;
	}
	else {
		mfp.tir[channel] = 0xff;
	}

	// 0‚É‚È‚Á‚½‚çƒŠƒ[ƒhAŠ„‚è‚İ
	if (mfp.tir[channel] == 0) {
#if defined(MFP_LOG)
	LOG1(Log::Normal, "ƒ^ƒCƒ}%c ƒI[ƒo[ƒtƒ[", channel + 'A');
#endif	// MFP_LOG

		// ƒŠƒ[ƒh
		mfp.tir[channel] = mfp.tdr[channel];

		// ƒCƒxƒ“ƒgƒJƒEƒ“ƒgƒ‚[ƒh‚ÍŠ„‚è‚İƒCƒxƒ“ƒg”­¶(ƒXƒsƒ“ƒfƒBƒW[II)
		if (mfp.tcr[channel] == 0x08) {
			// GPIP•ÏX‚ğæ‚És‚¢A­‚µ’x‚ê‚ÄŠ„‚è‚İ‚ğo‚·‚æ‚¤‚É‚·‚é
			// ƒƒ^ƒ‹ƒIƒŒƒ“ƒWEX(12‚Å‚Íƒ_ƒA—v’²¸)
			timer[channel].SetTime(36);
		}
		else {
			// ’ÊíŠ„‚è‚İ
			Interrupt(TimerInt[channel], TRUE);
		}
	}
}

//---------------------------------------------------------------------------
//
//	ƒ^ƒCƒ}Š„‚è‚İƒe[ƒuƒ‹
//
//---------------------------------------------------------------------------
const int MFP::TimerInt[4] = {
	13,									// Timer-A
	8,									// Timer-B
	5,									// Timer-C
	4									// Timer-D
};

//---------------------------------------------------------------------------
//
//	ƒ^ƒCƒ}ŠÔƒe[ƒuƒ‹
//
//---------------------------------------------------------------------------
const DWORD MFP::TimerHus[8] = {
	0,									// ƒ^ƒCƒ}ƒXƒgƒbƒv
	2,									// 1.0us
	5,									// 2.5us
	8,									// 4us
	25,									// 12.5us
	32,									// 16us
	50,									// 25us
	100									// 50us
};

//---------------------------------------------------------------------------
//
//	GPDRİ’è
//
//---------------------------------------------------------------------------
void FASTCALL MFP::SetGPDR(DWORD data)
{
	int i;
	DWORD bit;

	ASSERT(this);
	ASSERT(data < 0x100);

	// DDR‚ª1‚Ìƒrƒbƒg‚Ì‚İ—LŒø
	for (i=0; i<8; i++) {
		bit = (DWORD)(1 << i);
		if (mfp.ddr & bit) {
			if (data & bit) {
				SetGPIP(i, 1);
			}
			else {
				SetGPIP(i, 0);
			}
		}
	}
}

//---------------------------------------------------------------------------
//
//	GPIPİ’è
//
//---------------------------------------------------------------------------
void FASTCALL MFP::SetGPIP(int num, int value)
{
	DWORD data;

	ASSERT(this);
	ASSERT((num >= 0) && (num < 8));
	ASSERT((value == 0) || (value == 1));

	// ƒoƒbƒNƒAƒbƒv‚ğæ‚é
	data = mfp.gpdr;

	// ƒrƒbƒgì¬
	mfp.gpdr &= (DWORD)(~(1 << num));
	if (value == 1) {
		mfp.gpdr |= (DWORD)(1 << num);
	}

	// ˆá‚Á‚Ä‚¢‚ê‚ÎŠ„‚è‚İƒ`ƒFƒbƒN
	if (mfp.gpdr != data) {
		IntGPIP();
	}
}

//---------------------------------------------------------------------------
//
//	GPIPŠ„‚è‚İƒ`ƒFƒbƒN
//
//---------------------------------------------------------------------------
void FASTCALL MFP::IntGPIP()
{
	DWORD data;
	int i;

	ASSERT(this);

	// Inside68k‚Ì‹Lq‚Í‹tIMFPƒf[ƒ^ƒV[ƒg‚É]‚¤‚ÆŸ‚Ì—lB
	// AER0 1->0‚ÅŠ„‚è‚İ
	// AER1 0->1‚ÅŠ„‚è‚İ

	// ~AER‚ÆGPDR‚ğXOR
	data = (DWORD)(~mfp.aer);
	data ^= (DWORD)mfp.gpdr;

	// BER‚ğŒ©‚ÄA1¨0‚É•Ï‰»‚·‚é‚Æ‚«Š„‚è‚İ”­¶
	// (‚½‚¾‚µƒpƒ‹ƒX•‘ª’èƒ^ƒCƒ}‚ª‚ ‚ê‚ÎGPIP4,GPIP3‚ÍŠ„‚è‚İˆ—‚µ‚È‚¢)
	for (i=0; i<8; i++) {
		if (data & 0x80) {
			if (!(mfp.ber & 0x80)) {
				if (i == 3) {
					// GPIP4Bƒ^ƒCƒ}A‚ğƒ`ƒFƒbƒN
					if ((mfp.tcr[0] & 0x0f) > 0x08) {
						data <<= 1;
						mfp.ber <<= 1;
						continue;
					}
				}
				if (i == 4) {
					// GPIP3Bƒ^ƒCƒ}B‚ğƒ`ƒFƒbƒN
					if ((mfp.tcr[1] & 0x0f) > 0x08) {
						data <<= 1;
						mfp.ber <<= 1;
						continue;
					}
				}

				// Š„‚è‚İ—v‹(SORCERIAN X1->88)
				Interrupt(GPIPInt[i], TRUE);
			}
		}

		// Ÿ‚Ö
		data <<= 1;
		mfp.ber <<= 1;
	}

	// BER‚ğì¬
	mfp.ber = (DWORD)(~mfp.aer);
	mfp.ber ^= mfp.gpdr;
}

//---------------------------------------------------------------------------
//
//	GPIPŠ„‚è‚İƒe[ƒuƒ‹
//
//---------------------------------------------------------------------------
const int MFP::GPIPInt[8] = {
	15,									// H-SYNC
	14,									// CIRQ
	7,									// (NoUse)
	6,									// V-DISP
	3,									// OPM
	2,									// POWER
	1,									// EXPON
	0									// ALARM
};

//---------------------------------------------------------------------------
//
//	ƒŒƒV[ƒoƒXƒe[ƒ^ƒXİ’è
//
//---------------------------------------------------------------------------
void FASTCALL MFP::SetRSR(DWORD data)
{
	ASSERT(this);
	ASSERT(data < 0x100);

	// RE‚Ì‚İİ’è‰Â”
	data &= 0x01;

	mfp.rsr &= ~0x01;
	mfp.rsr |= (DWORD)(mfp.rsr | data);
}

//---------------------------------------------------------------------------
//
//	CPU©MFP óM
//
//---------------------------------------------------------------------------
void FASTCALL MFP::Receive()
{
	ASSERT(this);

	// ƒ^ƒCƒ}BğŒƒ`ƒFƒbƒN
	if (mfp.tcr[1] != 0x01) {
		return;
	}
	if (mfp.tdr[1] != 0x0d) {
		return;
	}

	// USARTƒRƒ“ƒgƒ[ƒ‹ğŒƒ`ƒFƒbƒN
	if (mfp.ucr != 0x88) {
		return;
	}

	// ƒŒƒV[ƒoƒfƒBƒZ[ƒuƒ‹‚Ìê‡Aˆ—‚µ‚È‚¢
	if (!(mfp.rsr & 0x01)) {
		return;
	}

	// BF=1‚Ìê‡A“Á‚Éˆø‚«æ‚éƒf[ƒ^‚Í–³‚¢
	if (!(mfp.rsr & 0x80)) {
		return;
	}

	// ƒf[ƒ^ˆø‚«æ‚èBBFAOE‚ğ0‚Éİ’è
	mfp.rsr &= (DWORD)~0xc0;
}

//---------------------------------------------------------------------------
//
//	ƒgƒ‰ƒ“ƒXƒ~ƒbƒ^ƒXƒe[ƒ^ƒXİ’è
//
//---------------------------------------------------------------------------
void FASTCALL MFP::SetTSR(DWORD data)
{
	ASSERT(this);
	ASSERT(data < 0x100);

	// BE,UE,END‚Í’¼ÚƒNƒŠƒA‚Å‚«‚È‚¢
	mfp.tsr = (DWORD)(mfp.tsr & 0xd0);
	data &= (DWORD)~0xd0;
	mfp.tsr = (DWORD)(mfp.tsr | data);

	// TE=1‚ÅAUE,END‚ğƒNƒŠƒA
	if (mfp.tsr & 0x01) {
		mfp.tsr = (DWORD)(mfp.tsr & ~0x50);
	}
}

//---------------------------------------------------------------------------
//
//	CPU¨MFP ‘—M
//
//---------------------------------------------------------------------------
void FASTCALL MFP::Transmit(DWORD data)
{
	ASSERT(this);

	// ƒ^ƒCƒ}BğŒƒ`ƒFƒbƒN
	if (mfp.tcr[1] != 0x01) {
		return;
	}
	if (mfp.tdr[1] != 0x0d) {
		return;
	}

	// USARTƒRƒ“ƒgƒ[ƒ‹ğŒƒ`ƒFƒbƒN
	if (mfp.ucr != 0x88) {
		return;
	}

	// ƒgƒ‰ƒ“ƒXƒ~ƒbƒ^ƒfƒBƒZ[ƒuƒ‹‚Ìê‡Aˆ—‚µ‚È‚¢
	if (!(mfp.tsr & 0x01)) {
		return;
	}

	// ƒXƒe[ƒ^ƒX‹y‚Ñƒf[ƒ^‚ğƒZƒbƒg
	mfp.tsr = (DWORD)(mfp.tsr & ~0x80);
	mfp.tur = data;
#if defined(MFP_LOG)
	LOG1(Log::Normal, "USART‘—Mƒf[ƒ^ó•t %02X", (BYTE)data);
#endif	// MFP_LOG
	return;
}

//---------------------------------------------------------------------------
//
//	USARTˆ—
//
//---------------------------------------------------------------------------
void FASTCALL MFP::USART()
{
	ASSERT(this);
	ASSERT((mfp.readpoint >= 0) && (mfp.readpoint < 0x10));
	ASSERT((mfp.writepoint >= 0) && (mfp.writepoint < 0x10));
	ASSERT((mfp.datacount >= 0) && (mfp.datacount <= 0x10));

	// ƒ^ƒCƒ}‹y‚ÑUSARTİ’è‚ğƒ`ƒFƒbƒN
	if (mfp.tcr[1] != 0x01) {
		return;
	}
	if (mfp.tdr[1] != 0x0d) {
		return;
	}
	if (mfp.ucr != 0x88) {
		return;
	}

	//
	//	‘—M
	//

	if (!(mfp.tsr & 0x80)) {
		// ‚±‚±‚Åƒgƒ‰ƒ“ƒXƒ~ƒbƒ^ƒfƒBƒZ[ƒuƒ‹‚È‚çAEND‚ª”­¶
		if (!(mfp.tsr & 0x01)) {
			mfp.tsr = (DWORD)(mfp.tsr & ~0x80);
			mfp.tsr = (DWORD)(mfp.tsr | 0x10);
			LOG0(Log::Warning, "USART ‘—MI—¹ƒGƒ‰[");
			Interrupt(9, TRUE);
			return;
		}

		// ƒoƒbƒtƒ@ƒGƒ“ƒvƒeƒBAƒI[ƒgƒ^[ƒ“ƒAƒ‰ƒEƒ“ƒh
		mfp.tsr = (DWORD)(mfp.tsr | 0x80);
		if (mfp.tsr & 0x20) {
			mfp.tsr = (DWORD)(mfp.tsr & ~0x20);
			SetRSR((DWORD)(mfp.rsr | 0x01));
		}

		// ƒL[ƒ{[ƒh‚Öƒf[ƒ^‘—oA‘—Mƒoƒbƒtƒ@ƒGƒ“ƒvƒeƒBŠ„‚è‚İ
#if defined(MFP_LOG)
		LOG1(Log::Normal, "USART‘—M %02X", mfp.tur);
#endif	// MFP_LOG
		keyboard->Command(mfp.tur);
		Interrupt(10, TRUE);
	}
	else {
		if (!(mfp.tsr & 0x40)) {
			mfp.tsr = (DWORD)(mfp.tsr | 0x40);
			Interrupt(9, TRUE);
#if defined(MFP_LOG)
			LOG0(Log::Normal, "USART ƒAƒ“ƒ_[ƒ‰ƒ“ƒGƒ‰[");
#endif	// MPF_LOG
		}
	}

	//
	//	óM
	//

	// —LŒø‚ÈóMƒf[ƒ^‚ª‚È‚¯‚ê‚ÎA‰½‚à‚µ‚È‚¢
	if (mfp.datacount == 0) {
		return;
	}

	// ƒŒƒV[ƒoƒfƒBƒZ[ƒuƒ‹‚È‚çA‰½‚à‚µ‚È‚¢
	if (!(mfp.rsr & 0x01)) {
		return;
	}

	// ‚±‚±‚ÅƒƒbƒN
	sync->Lock();

	// Šù‚ÉóMƒf[ƒ^‚ª‚ ‚ê‚ÎAƒI[ƒo[ƒ‰ƒ“‚Æ‚µ‚Äƒf[ƒ^‚ğÌ‚Ä‚é
	if (mfp.rsr & 0x80) {
		mfp.rsr |= 0x40;
		mfp.readpoint = (mfp.readpoint + 1) & 0x0f;
		mfp.datacount--;
		sync->Unlock();

		LOG0(Log::Warning, "USART ƒI[ƒo[ƒ‰ƒ“ƒGƒ‰[");
		Interrupt(11, TRUE);
		return;
	}

	// ƒf[ƒ^óMBBF‚ğƒZƒbƒg‚µAƒf[ƒ^‚ğ‹L‰¯
	mfp.rsr |= 0x80;
	mfp.rur = mfp.buffer[mfp.readpoint];
	mfp.readpoint = (mfp.readpoint + 1) & 0x0f;
	mfp.datacount--;
	sync->Unlock();

	Interrupt(12, TRUE);
}

//---------------------------------------------------------------------------
//
//	ƒL[ƒf[ƒ^óM
//
//---------------------------------------------------------------------------
void FASTCALL MFP::KeyData(DWORD data)
{
	ASSERT(this);
	ASSERT((mfp.readpoint >= 0) && (mfp.readpoint < 0x10));
	ASSERT((mfp.writepoint >= 0) && (mfp.writepoint < 0x10));
	ASSERT((mfp.datacount >= 0) && (mfp.datacount <= 0x10));
	ASSERT(data < 0x100);

	// ƒƒbƒN
	sync->Lock();

	// ‘‚«‚İƒ|ƒCƒ“ƒg‚ÖŠi”[
	mfp.buffer[mfp.writepoint] = data;

	// ‘‚«‚İƒ|ƒCƒ“ƒgˆÚ“®AƒJƒEƒ“ƒ^{‚P
	mfp.writepoint = (mfp.writepoint + 1) & 0x0f;
	mfp.datacount++;
	if (mfp.datacount > 0x10) {
		mfp.datacount = 0x10;
	}

	// SyncƒAƒ“ƒƒbƒN
	sync->Unlock();
}
