//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2005 ‚o‚hD(ytanaka@ipc-tokai.or.jp)
//	[ CRTC(VICON) ]
//
//---------------------------------------------------------------------------

#include "os.h"
#include "xm6.h"
#include "vm.h"
#include "log.h"
#include "tvram.h"
#include "mfp.h"
#include "sprite.h"
#include "render.h"
#include "schedule.h"
#include "cpu.h"
#include "gvram.h"
#include "printer.h"
#include "fileio.h"
#include "crtc.h"

//===========================================================================
//
//	CRTC
//
//===========================================================================
//#define CRTC_LOG

//---------------------------------------------------------------------------
//
//	ƒRƒ“ƒXƒgƒ‰ƒNƒ^
//
//---------------------------------------------------------------------------
CRTC::CRTC(VM *p) : MemDevice(p)
{
	// ƒfƒoƒCƒXID‚ğ‰Šú‰»
	dev.id = MAKEID('C', 'R', 'T', 'C');
	dev.desc = "CRTC (VICON)";

	// ŠJnƒAƒhƒŒƒXAI—¹ƒAƒhƒŒƒX
	memdev.first = 0xe80000;
	memdev.last = 0xe81fff;

	// ‚»‚Ì‘¼ƒ[ƒN
	tvram = NULL;
	gvram = NULL;
	sprite = NULL;
	mfp = NULL;
	render = NULL;
	printer = NULL;
}

//---------------------------------------------------------------------------
//
//	‰Šú‰»
//
//---------------------------------------------------------------------------
BOOL FASTCALL CRTC::Init()
{
	ASSERT(this);

	// Šî–{ƒNƒ‰ƒX
	if (!MemDevice::Init()) {
		return FALSE;
	}

	// ƒeƒLƒXƒgVRAM‚ğæ“¾
	tvram = (TVRAM*)vm->SearchDevice(MAKEID('T', 'V', 'R', 'M'));
	ASSERT(tvram);

	// ƒOƒ‰ƒtƒBƒbƒNVRAM‚ğæ“¾
	gvram = (GVRAM*)vm->SearchDevice(MAKEID('G', 'V', 'R', 'M'));
	ASSERT(gvram);

	// ƒXƒvƒ‰ƒCƒgƒRƒ“ƒgƒ[ƒ‰‚ğæ“¾
	sprite = (Sprite*)vm->SearchDevice(MAKEID('S', 'P', 'R', ' '));
	ASSERT(sprite);

	// MFP‚ğæ“¾
	mfp = (MFP*)vm->SearchDevice(MAKEID('M', 'F', 'P', ' '));
	ASSERT(mfp);

	// ƒŒƒ“ƒ_ƒ‰‚ğæ“¾
	render = (Render*)vm->SearchDevice(MAKEID('R', 'E', 'N', 'D'));
	ASSERT(render);

	// ƒvƒŠƒ“ƒ^‚ğæ“¾
	printer = (Printer*)vm->SearchDevice(MAKEID('P', 'R', 'N', ' '));
	ASSERT(printer);

	// ƒCƒxƒ“ƒg‰Šú‰»
	event.SetDevice(this);
	event.SetDesc("H-Sync");
	event.SetTime(0);
	scheduler->AddEvent(&event);

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	ƒNƒŠ[ƒ“ƒAƒbƒv
//
//---------------------------------------------------------------------------
void FASTCALL CRTC::Cleanup()
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
void FASTCALL CRTC::Reset()
{
	int i;

	ASSERT(this);
	LOG0(Log::Normal, "ƒŠƒZƒbƒg");

	// ƒŒƒWƒXƒ^‚ğƒNƒŠƒA
	memset(crtc.reg, 0, sizeof(crtc.reg));
	for (i=0; i<18; i++) {
		crtc.reg[i] = ResetTable[i];
	}
	for (i=0; i<8; i++) {
		crtc.reg[i + 0x28] = ResetTable[i + 18];
	}

	// ‰ğ‘œ“x
	crtc.hrl = FALSE;
	crtc.lowres = FALSE;
	crtc.textres = TRUE;
	crtc.changed = FALSE;

	// “Áê‹@”
	crtc.raster_count = 0;
	crtc.raster_int = 0;
	crtc.raster_copy = FALSE;
	crtc.raster_exec = FALSE;
	crtc.fast_clr = 0;

	// …•½
	crtc.h_sync = 31745;
	crtc.h_pulse = 3450;
	crtc.h_back = 4140;
	crtc.h_front = 2070;
	crtc.h_dots = 768;
	crtc.h_mul = 1;
	crtc.hd = 2;

	// ‚’¼
	crtc.v_sync = 568;
	crtc.v_pulse = 6;
	crtc.v_back = 35;
	crtc.v_front = 15;
	crtc.v_dots = 512;
	crtc.v_mul = 1;
	crtc.vd = 1;

	// ƒCƒxƒ“ƒg
	crtc.ns = 0;
	crtc.hus = 0;
	crtc.v_synccnt = 1;
	crtc.v_blankcnt = 1;
	crtc.h_disp = TRUE;
	crtc.v_disp = TRUE;
	crtc.v_blank = TRUE;
	crtc.v_count = 0;
	crtc.v_scan = 0;

	// ˆÈ‰º‚¢‚ç‚È‚¢
	crtc.h_disptime = 0;
	crtc.h_synctime = 0;
	crtc.v_cycletime = 0;
	crtc.v_blanktime = 0;
	crtc.v_backtime = 0;
	crtc.v_synctime = 0;

	// ƒƒ‚ƒŠƒ‚[ƒh
	crtc.tmem = FALSE;
	crtc.gmem = TRUE;
	crtc.siz = 0;
	crtc.col = 3;

	// ƒXƒNƒ[ƒ‹
	crtc.text_scrlx = 0;
	crtc.text_scrly = 0;
	for (i=0; i<4; i++) {
		crtc.grp_scrlx[i] = 0;
		crtc.grp_scrly[i] = 0;
	}

	// H-SyncƒCƒxƒ“ƒg‚ğİ’è(31.5us)
	event.SetTime(63);
}

//---------------------------------------------------------------------------
//
//	CRTCƒŠƒZƒbƒgƒf[ƒ^
//
//---------------------------------------------------------------------------
const BYTE CRTC::ResetTable[] = {
	0x00, 0x89, 0x00, 0x0e, 0x00, 0x1c, 0x00, 0x7c,
	0x02, 0x37, 0x00, 0x05, 0x00, 0x28, 0x02, 0x28,
	0x00, 0x1b,
	0x0b, 0x16, 0x00, 0x33, 0x7a, 0x7b, 0x00, 0x00
};

//---------------------------------------------------------------------------
//
//	ƒZ[ƒu
//
//---------------------------------------------------------------------------
BOOL FASTCALL CRTC::Save(Fileio *fio, int ver)
{
	size_t sz;

	ASSERT(this);
	ASSERT(fio);
	LOG0(Log::Normal, "ƒZ[ƒu");

	// ƒTƒCƒY‚ğƒZ[ƒu
	sz = sizeof(crtc_t);
	if (!fio->Write(&sz, sizeof(sz))) {
		return FALSE;
	}

	// À‘Ì‚ğƒZ[ƒu
	if (!fio->Write(&crtc, (int)sz)) {
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
BOOL FASTCALL CRTC::Load(Fileio *fio, int ver)
{
	size_t sz;

	ASSERT(this);
	ASSERT(fio);
	LOG0(Log::Normal, "ƒ[ƒh");

	// ƒTƒCƒY‚ğƒ[ƒh
	if (!fio->Read(&sz, sizeof(sz))) {
		return FALSE;
	}
	if (sz != sizeof(crtc_t)) {
		return FALSE;
	}

	// À‘Ì‚ğƒ[ƒh
	if (!fio->Read(&crtc, (int)sz)) {
		return FALSE;
	}

	// ƒCƒxƒ“ƒg‚ğƒ[ƒh
	if (!event.Load(fio, ver)) {
		return FALSE;
	}

	// ƒŒƒ“ƒ_ƒ‰‚Ö’Ê’m
	render->TextScrl(crtc.text_scrlx, crtc.text_scrly);
	render->GrpScrl(0, crtc.grp_scrlx[0], crtc.grp_scrly[0]);
	render->GrpScrl(1, crtc.grp_scrlx[1], crtc.grp_scrly[1]);
	render->GrpScrl(2, crtc.grp_scrlx[2], crtc.grp_scrly[2]);
	render->GrpScrl(3, crtc.grp_scrlx[3], crtc.grp_scrly[3]);
	render->SetCRTC();

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	İ’è“K—p
//
//---------------------------------------------------------------------------
void FASTCALL CRTC::ApplyCfg(const Config *config)
{
	ASSERT(this);
	ASSERT(config);
	LOG0(Log::Normal, "İ’è“K—p");
}

//---------------------------------------------------------------------------
//
//	ƒoƒCƒg“Ç‚İ‚İ
//
//---------------------------------------------------------------------------
DWORD FASTCALL CRTC::ReadByte(DWORD addr)
{
	BYTE data;

	ASSERT(this);
	ASSERT((addr >= memdev.first) && (addr <= memdev.last));

	// $800’PˆÊ‚Åƒ‹[ƒv
	addr &= 0x7ff;

	// ƒEƒFƒCƒg
	scheduler->Wait(1);

	// $E80000-$E803FF : ƒŒƒWƒXƒ^ƒGƒŠƒA
	if (addr < 0x400) {
		addr &= 0x3f;
		if (addr >= 0x30) {
			return 0xff;
		}

		// R20, R21‚Ì‚İ“Ç‚İ‘‚«‰Â”\B‚»‚êˆÈŠO‚Í$00
		if ((addr < 40) || (addr > 43)) {
			return 0;
		}

		// “Ç‚İ‚İ(ƒGƒ“ƒfƒBƒAƒ“‚ğ”½“]‚³‚¹‚é)
		addr ^= 1;
		return crtc.reg[addr];
	}

	// $E80480-$E804FF : “®ìƒ|[ƒg
	if ((addr >= 0x480) && (addr <= 0x4ff)) {
		// ãˆÊƒoƒCƒg‚Í 0
		if ((addr & 1) == 0) {
			return 0;
		}

		// ‰ºˆÊƒoƒCƒg‚Íƒ‰ƒXƒ^ƒRƒs[AƒOƒ‰ƒtƒBƒbƒN‚‘¬ƒNƒŠƒA‚Ì‚İ
		data = 0;
		if (crtc.raster_copy) {
			data |= 0x08;
		}
		if (crtc.fast_clr == 2) {
			data |= 0x02;
		}
		return data;
	}

	LOG1(Log::Warning, "–¢À‘•ƒAƒhƒŒƒX“Ç‚İ‚İ $%06X", memdev.first + addr);
	return 0xff;
}

//---------------------------------------------------------------------------
//
//	ƒoƒCƒg‘‚«‚İ
//
//---------------------------------------------------------------------------
void FASTCALL CRTC::WriteByte(DWORD addr, DWORD data)
{
	int reg;

	ASSERT(this);
	ASSERT((addr >= memdev.first) && (addr <= memdev.last));

	// $800’PˆÊ‚Åƒ‹[ƒv
	addr &= 0x7ff;

	// ƒEƒFƒCƒg
	scheduler->Wait(1);

	// $E80000-$E803FF : ƒŒƒWƒXƒ^ƒGƒŠƒA
	if (addr < 0x400) {
		addr &= 0x3f;
		if (addr >= 0x30) {
			return;
		}

		// ‘‚«‚İ(ƒGƒ“ƒfƒBƒAƒ“‚ğ”½“]‚³‚¹‚é)
		addr ^= 1;
		if (crtc.reg[addr] == data) {
			return;
		}
		crtc.reg[addr] = (BYTE)data;

		// GVRAMƒAƒhƒŒƒX\¬
		if (addr == 0x29) {
			if (data & 0x10) {
				crtc.tmem = TRUE;
			}
			else {
				crtc.tmem = FALSE;
			}
			if (data & 0x08) {
				crtc.gmem = TRUE;
			}
			else {
				crtc.gmem = FALSE;
			}
			crtc.siz = (data & 4) >> 2;
			crtc.col = (data & 3);

			// ƒOƒ‰ƒtƒBƒbƒNVRAM‚Ö’Ê’m
			gvram->SetType(data & 0x0f);
			return;
		}

		// ‰ğ‘œ“x•ÏX
		if ((addr <= 15) || (addr == 40)) {
			// ƒXƒvƒ‰ƒCƒgƒƒ‚ƒŠ‚ÌÚ‘±EØ’f‚Íu‚És‚¤(OS-9/68000)
			if (addr == 0x28) {
				if ((crtc.reg[0x28] & 3) >= 2) {
					sprite->Connect(FALSE);
				}
				else {
					sprite->Connect(TRUE);
				}
			}

			// Ÿ‚ÌüŠú‚ÅÄŒvZ
			crtc.changed = TRUE;
			return;
		}

		// ƒ‰ƒXƒ^Š„‚è‚İ
		if ((addr == 18) || (addr == 19)) {
			crtc.raster_int = (crtc.reg[19] << 8) + crtc.reg[18];
			crtc.raster_int &= 0x3ff;
			CheckRaster();
			return;
		}

		// ƒeƒLƒXƒgƒXƒNƒ[ƒ‹
		if ((addr >= 20) && (addr <= 23)) {
			crtc.text_scrlx = (crtc.reg[21] << 8) + crtc.reg[20];
			crtc.text_scrlx &= 0x3ff;
			crtc.text_scrly = (crtc.reg[23] << 8) + crtc.reg[22];
			crtc.text_scrly &= 0x3ff;
			render->TextScrl(crtc.text_scrlx, crtc.text_scrly);

#if defined(CRTC_LOG)
			LOG2(Log::Normal, "ƒeƒLƒXƒgƒXƒNƒ[ƒ‹ x=%d y=%d", crtc.text_scrlx, crtc.text_scrly);
#endif	// CRTC_LOG
			return;
		}

		// ƒOƒ‰ƒtƒBƒbƒNƒXƒNƒ[ƒ‹
		if ((addr >= 24) && (addr <= 39)) {
			reg = addr & ~3;
			addr -= 24;
			addr >>= 2;
			ASSERT(addr <= 3);
			crtc.grp_scrlx[addr] = (crtc.reg[reg+1] << 8) + crtc.reg[reg+0];
			crtc.grp_scrly[addr] = (crtc.reg[reg+3] << 8) + crtc.reg[reg+2];
			if (addr == 0) {
				crtc.grp_scrlx[addr] &= 0x3ff;
				crtc.grp_scrly[addr] &= 0x3ff;
			}
			else {
				crtc.grp_scrlx[addr] &= 0x1ff;
				crtc.grp_scrly[addr] &= 0x1ff;
			}
			render->GrpScrl(addr, crtc.grp_scrlx[addr], crtc.grp_scrly[addr]);
			return;
		}

		// ƒeƒLƒXƒgVRAM
		if ((addr >= 42) && (addr <= 47)) {
			TextVRAM();
		}
		return;
	}

	// $E80480-$E804FF : “®ìƒ|[ƒg
	if ((addr >= 0x480) && (addr <= 0x4ff)) {
		// ãˆÊƒoƒCƒg‚Í‰½‚à‚È‚¢
		if ((addr & 1) == 0) {
			return;
		}

		// ‰ºˆÊƒoƒCƒg‚Íƒ‰ƒXƒ^ƒRƒs[E‚‘¬ƒNƒŠƒA§Œä
		if (data & 0x08) {
			crtc.raster_copy = TRUE;
		}
		else {
			crtc.raster_copy = FALSE;
		}
		if (data & 0x02) {
			// ƒ‰ƒXƒ^ƒRƒs[‚Æ‹¤—pAƒ‰ƒXƒ^ƒRƒs[—Dæ(‘åí—ªIII'90)
			if ((crtc.fast_clr == 0) && !crtc.raster_copy) {
#if defined(CRTC_LOG)
				LOG0(Log::Normal, "ƒOƒ‰ƒtƒBƒbƒN‚‘¬ƒNƒŠƒAw¦");
#endif	// CRTC_LOG
				crtc.fast_clr = 1;
			}
#if defined(CRTC_LOG)
			else {
				LOG1(Log::Normal, "ƒOƒ‰ƒtƒBƒbƒN‚‘¬ƒNƒŠƒAw¦–³Œø State=%d", crtc.fast_clr);
			}
#endif	//CRTC_LOG
		}
		return;
	}

	LOG2(Log::Warning, "–¢À‘•ƒAƒhƒŒƒX‘‚«‚İ $%06X <- $%02X",
							memdev.first + addr, data);
}

//---------------------------------------------------------------------------
//
//	“Ç‚İ‚İ‚Ì‚İ
//
//---------------------------------------------------------------------------
DWORD FASTCALL CRTC::ReadOnly(DWORD addr) const
{
	BYTE data;

	ASSERT(this);
	ASSERT((addr >= memdev.first) && (addr <= memdev.last));

	// $800’PˆÊ‚Åƒ‹[ƒv
	addr &= 0x7ff;

	// $E80000-$E803FF : ƒŒƒWƒXƒ^ƒGƒŠƒA
	if (addr < 0x400) {
		addr &= 0x3f;
		if (addr >= 0x30) {
			return 0xff;
		}

		// “Ç‚İ‚İ(ƒGƒ“ƒfƒBƒAƒ“‚ğ”½“]‚³‚¹‚é)
		addr ^= 1;
		return crtc.reg[addr];
	}

	// $E80480-$E804FF : “®ìƒ|[ƒg
	if ((addr >= 0x480) && (addr <= 0x4ff)) {
		// ãˆÊƒoƒCƒg‚Í0
		if ((addr & 1) == 0) {
			return 0;
		}

		// ‰ºˆÊƒoƒCƒg‚ÍƒOƒ‰ƒtƒBƒbƒN‚‘¬ƒNƒŠƒA‚Ì‚İ
		data = 0;
		if (crtc.raster_copy) {
			data |= 0x08;
		}
		if (crtc.fast_clr == 2) {
			data |= 0x02;
		}
		return data;
	}

	return 0xff;
}

//---------------------------------------------------------------------------
//
//	“à•”ƒf[ƒ^æ“¾
//
//---------------------------------------------------------------------------
void FASTCALL CRTC::GetCRTC(crtc_t *buffer) const
{
	ASSERT(buffer);

	// “à•”ƒf[ƒ^‚ğƒRƒs[
	*buffer = crtc;
}

//---------------------------------------------------------------------------
//
//	ƒCƒxƒ“ƒgƒR[ƒ‹ƒoƒbƒN
//
//---------------------------------------------------------------------------
BOOL FASTCALL CRTC::Callback(Event* /*ev*/)
{
	ASSERT(this);

	// HSync,HDisp‚Ì2‚Â‚ğŒÄ‚Ñ•ª‚¯‚é
	if (crtc.h_disp) {
		HSync();
	}
	else {
		HDisp();
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	H-SYNCŠJn
//
//---------------------------------------------------------------------------
void FASTCALL CRTC::HSync()
{
	int hus;

	ASSERT(this);

	// ƒvƒŠƒ“ƒ^‚É’Ê’m(’èŠú“I‚ÉBUSY‚ğ—‚Æ‚·‚½‚ß)
	ASSERT(printer);
	printer->HSync();

	// V-SYNCƒJƒEƒ“ƒg
	crtc.v_synccnt--;
	if (crtc.v_synccnt == 0) {
		VSync();
	}

	// V-BLANKƒJƒEƒ“ƒg
	crtc.v_blankcnt--;
	if (crtc.v_blankcnt == 0) {
		VBlank();
	}

	// Ÿ‚Ìƒ^ƒCƒ~ƒ“ƒO(H-DISPŠJn)‚Ü‚Å‚ÌŠÔ‚ğİ’è
	crtc.ns += crtc.h_pulse;
	hus = Ns2Hus(crtc.ns);
	hus -= crtc.hus;
	event.SetTime(hus);
	crtc.hus += hus;

	// “¯Šúˆ—(40ms‚²‚Æ)
	if (crtc.hus >= 80000) {
		crtc.hus -= 80000;
		ASSERT(crtc.ns >= 40000000);
		crtc.ns -= 40000000;
	}

	// ƒtƒ‰ƒOİ’è
	crtc.h_disp = FALSE;

	// GPIPİ’è
	mfp->SetGPIP(7, 1);

	// •`‰æ
	crtc.v_scan++;
	if (!crtc.v_blank) {
		// ƒŒƒ“ƒ_ƒŠƒ“ƒO
		render->HSync(crtc.v_scan);
	}

	// ƒ‰ƒXƒ^Š„‚è‚İ
#if 0
	CheckRaster();
	crtc.raster_count++;
#endif

	// ƒeƒLƒXƒg‰æ–Êƒ‰ƒXƒ^ƒRƒs[
	if (crtc.raster_copy && crtc.raster_exec) {
		tvram->RasterCopy();
		crtc.raster_exec = FALSE;
	}

	// ƒOƒ‰ƒtƒBƒbƒN‰æ–Ê‚‘¬ƒNƒŠƒA
	if (crtc.fast_clr == 2) {
		gvram->FastClr(&crtc);
	}
}

//---------------------------------------------------------------------------
//
//	H-DISPŠJn
//
//---------------------------------------------------------------------------
void FASTCALL CRTC::HDisp()
{
	int ns;
	int hus;

	ASSERT(this);

#if 1
	// ƒ‰ƒXƒ^Š„‚è‚İ
	CheckRaster();
	crtc.raster_count++;
#endif

	// Ÿ‚Ìƒ^ƒCƒ~ƒ“ƒO(H-SYNCŠJn)‚Ü‚Å‚ÌŠÔ‚ğİ’è
	ns = crtc.h_sync - crtc.h_pulse;
	ASSERT(ns > 0);
	crtc.ns += ns;
	hus = Ns2Hus(crtc.ns);
	hus -= crtc.hus;
	event.SetTime(hus);
	crtc.hus += hus;

	// ƒtƒ‰ƒOİ’è
	crtc.h_disp = TRUE;

	// GPIPİ’è
	mfp->SetGPIP(7,0);

	// ƒ‰ƒXƒ^ƒRƒs[‹–‰Â
	crtc.raster_exec = TRUE;
}

//---------------------------------------------------------------------------
//
//	V-SYNCŠJn(V-DISPŠJn‚ğŠÜ‚Ş)
//
//---------------------------------------------------------------------------
void FASTCALL CRTC::VSync()
{
	ASSERT(this);

	// V-SYNCI—¹‚È‚ç
	if (!crtc.v_disp) {
		// ƒtƒ‰ƒOİ’è
		crtc.v_disp = TRUE;

		// ŠÔİ’è
		crtc.v_synccnt = (crtc.v_sync - crtc.v_pulse);
		return;
	}

	// ‰ğ‘œ“x•ÏX‚ª‚ ‚ê‚ÎA‚±‚±‚Å•ÏX
	if (crtc.changed) {
		ReCalc();
	}

	// V-SYNCI—¹‚Ü‚Å‚ÌŠÔ‚ğİ’è
	crtc.v_synccnt = crtc.v_pulse;

	// V-BLANK‚Ìó‘Ô‚ÆAŠÔ‚ğİ’è
	if (crtc.v_front < 0) {
		// ‚Ü‚¾•\¦’†(“Áê)
		crtc.v_blank = FALSE;
		crtc.v_blankcnt = (-crtc.v_front) + 1;
	}
	else {
		// ‚·‚Å‚Éƒuƒ‰ƒ“ƒN’†(’Êí)
		crtc.v_blank = TRUE;
		crtc.v_blankcnt = (crtc.v_pulse + crtc.v_back + 1);
	}

	// ƒtƒ‰ƒOİ’è
	crtc.v_disp = FALSE;

	// ƒ‰ƒXƒ^ƒJƒEƒ“ƒg‰Šú‰»
	crtc.raster_count = 0;
}

//---------------------------------------------------------------------------
//
//	ÄŒvZ
//
//---------------------------------------------------------------------------
void FASTCALL CRTC::ReCalc()
{
	int dc;
	int over;
	WORD *p;

	ASSERT(this);
	ASSERT(crtc.changed);

	// CRTCƒŒƒWƒXƒ^0‚ªƒNƒŠƒA‚³‚ê‚Ä‚¢‚ê‚ÎA–³Œø(MacƒGƒ~ƒ…ƒŒ[ƒ^)
	if (crtc.reg[0x0] != 0) {
#if defined(CRTC_LOG)
		LOG0(Log::Normal, "ÄŒvZ");
#endif	// CRTC_LOG

		// ƒhƒbƒgƒNƒƒbƒN‚ğæ“¾
		dc = Get8DotClock();

		// …•½(‚·‚×‚Äns’PˆÊ)
		crtc.h_sync = (crtc.reg[0x0] + 1) * dc / 100;
		crtc.h_pulse = (crtc.reg[0x02] + 1) * dc / 100;
		crtc.h_back = (crtc.reg[0x04] + 5 - crtc.reg[0x02] - 1) * dc / 100;
		crtc.h_front = (crtc.reg[0x0] + 1 - crtc.reg[0x06] - 5) * dc / 100;

		// ‚’¼(‚·‚×‚ÄH-Sync’PˆÊ)
		p = (WORD *)crtc.reg;
		crtc.v_sync = ((p[4] & 0x3ff) + 1);
		crtc.v_pulse = ((p[5] & 0x3ff) + 1);
		crtc.v_back = ((p[6] & 0x3ff) + 1) - crtc.v_pulse;
		crtc.v_front = crtc.v_sync - ((p[7] & 0x3ff) + 1);

		// V-FRONT‚ªƒ}ƒCƒiƒX‚·‚¬‚éê‡‚ÍA1…•½ŠúŠÔ•ª‚Ì‚İ(ƒwƒ‹ƒnƒEƒ“ƒhAƒRƒbƒgƒ“)
		if (crtc.v_front < 0) {
			over = -crtc.v_front;
			over -= crtc.v_back;
			if (over >= crtc.v_pulse) {
				crtc.v_front = -1;
			}
		}

		// ƒhƒbƒg”‚ğZo
		crtc.h_dots = (crtc.reg[0x0] + 1);
		crtc.h_dots -= (crtc.reg[0x02] + 1);
		crtc.h_dots -= (crtc.reg[0x04] + 5 - crtc.reg[0x02] - 1);
		crtc.h_dots -= (crtc.reg[0x0] + 1 - crtc.reg[0x06] - 5);
		crtc.h_dots *= 8;
		crtc.v_dots = crtc.v_sync - crtc.v_pulse - crtc.v_back - crtc.v_front;
	}

	// ”{—¦İ’è(…•½)
	crtc.hd = (crtc.reg[0x28] & 3);
	if (crtc.hd == 3) {
		LOG0(Log::Warning, "‰¡ƒhƒbƒg”50MHzƒ‚[ƒh(CompactXVI)");
	}
	if (crtc.hd == 0) {
		crtc.h_mul = 2;
	}
	else {
		crtc.h_mul = 1;
	}

	// crtc.hd‚ª2ˆÈã‚Ìê‡AƒXƒvƒ‰ƒCƒg‚ÍØ‚è—£‚³‚ê‚é
	if (crtc.hd >= 2) {
		// 768x512 or VGAƒ‚[ƒh(ƒXƒvƒ‰ƒCƒg‚È‚µ)
		sprite->Connect(FALSE);
		crtc.textres = TRUE;
	}
	else {
		// 256x256 or 512x512ƒ‚[ƒh(ƒXƒvƒ‰ƒCƒg‚ ‚è)
		sprite->Connect(TRUE);
		crtc.textres = FALSE;
	}

	// ”{—¦İ’è(‚’¼)
	crtc.vd = (crtc.reg[0x28] >> 2) & 3;
	if (crtc.reg[0x28] & 0x10) {
		// 31kHz
		crtc.lowres = FALSE;
		if (crtc.vd == 3) {
			// ƒCƒ“ƒ^ƒŒ[ƒX1024dotƒ‚[ƒh
			crtc.v_mul = 0;
		}
		else {
			// ƒCƒ“ƒ^ƒŒ[ƒXA’Êí512ƒ‚[ƒh(x1)A”{256dotƒ‚[ƒh(x2)
			crtc.v_mul = 2 - crtc.vd;
		}
	}
	else {
		// 15kHz
		crtc.lowres = TRUE;
		if (crtc.vd == 0) {
			// ’Êí‚Ì256dotƒ‚[ƒh(x2)
			crtc.v_mul = 2;
		}
		else {
			// ƒCƒ“ƒ^ƒŒ[ƒX512dotƒ‚[ƒh(x1)
			crtc.v_mul = 0;
		}
	}

	// ƒŒƒ“ƒ_ƒ‰‚Ö’Ê’m
	render->SetCRTC();

	// ƒtƒ‰ƒO‚¨‚ë‚·
	crtc.changed = FALSE;
}


//---------------------------------------------------------------------------
//
//	V-BLANKŠJn(V-SCREENŠJn‚ğŠÜ‚Ş)
//
//---------------------------------------------------------------------------
void FASTCALL CRTC::VBlank()
{
	ASSERT(this);

	// •\¦’†‚Å‚ ‚ê‚ÎAƒuƒ‰ƒ“ƒNŠJn
	if (!crtc.v_blank) {
		// ƒuƒ‰ƒ“ƒN‹æŠÔ‚ğİ’è
		crtc.v_blankcnt = crtc.v_pulse + crtc.v_back + crtc.v_front;
		ASSERT((crtc.v_front < 0) || ((int)crtc.v_synccnt == crtc.v_front));

		// ƒtƒ‰ƒO
		crtc.v_blank = TRUE;

		// GPIP
		mfp->EventCount(0, 0);
		mfp->SetGPIP(4, 0);

		// ƒOƒ‰ƒtƒBƒbƒN‚‘¬ƒNƒŠƒA
		if (crtc.fast_clr == 2) {
#if defined(CRTC_LOG)
			LOG0(Log::Normal, "ƒOƒ‰ƒtƒBƒbƒN‚‘¬ƒNƒŠƒAI—¹");
#endif	// CRTC_LOG
			crtc.fast_clr = 0;
		}

		// ƒŒƒ“ƒ_ƒ‰‡¬I—¹
		render->EndFrame();
		crtc.v_scan = crtc.v_dots + 1;
		return;
	}

	// •\¦‹æŠÔ‚ğİ’è
	crtc.v_blankcnt = crtc.v_sync;
	crtc.v_blankcnt -= (crtc.v_pulse + crtc.v_back + crtc.v_front);

	// ƒtƒ‰ƒO
	crtc.v_blank = FALSE;

	// GPIP
	mfp->EventCount(0, 1);
	mfp->SetGPIP(4, 1);

	// ƒOƒ‰ƒtƒBƒbƒN‚‘¬ƒNƒŠƒA
	if (crtc.fast_clr == 1) {
#if defined(CRTC_LOG)
		LOG1(Log::Normal, "ƒOƒ‰ƒtƒBƒbƒN‚‘¬ƒNƒŠƒAŠJn data=%02X", crtc.reg[42]);
#endif	// CRTC_LOG
		crtc.fast_clr = 2;
		gvram->FastSet((DWORD)crtc.reg[42]);
		gvram->FastClr(&crtc);
	}

	// ƒŒƒ“ƒ_ƒ‰‡¬ŠJnAƒJƒEƒ“ƒ^ƒAƒbƒv
	crtc.v_scan = 0;
	render->StartFrame();
	crtc.v_count++;
}

//---------------------------------------------------------------------------
//
//	•\¦ü”g”æ“¾
//
//---------------------------------------------------------------------------
void FASTCALL CRTC::GetHVHz(DWORD *h, DWORD *v) const
{
	DWORD d;
	DWORD t;

	// assert
	ASSERT(h);
	ASSERT(v);

	// ƒ`ƒFƒbƒN
	if ((crtc.h_sync == 0) || (crtc.v_sync < 100)) {
		// NO SIGNAL
		*h = 0;
		*v = 0;
		return;
	}

	// ex. 31.5kHz = 3150
	d = 100 * 1000 * 1000;
	d /= crtc.h_sync;
	*h = d;

	// ex. 55.46Hz = 5546
	t = crtc.v_sync;
    t *= crtc.h_sync;
	t /= 100;
	d = 1000 * 1000 * 1000;
	d /= t;
	*v = d;
}

//---------------------------------------------------------------------------
//
//	8ƒhƒbƒgƒNƒƒbƒN‚ğæ“¾(~100)
//
//---------------------------------------------------------------------------
int FASTCALL CRTC::Get8DotClock() const
{
	int hf;
	int hd;
	int index;

	ASSERT(this);

	// HF, HD‚ğCRTC R20‚æ‚èæ“¾
	hf = (crtc.reg[0x28] >> 4) & 1;
	hd = (crtc.reg[0x28] & 3);

	// ƒCƒ“ƒfƒbƒNƒXì¬
	index = hf * 4 + hd;
	if (crtc.hrl) {
		index += 8;
	}

	return DotClockTable[index];
}

//---------------------------------------------------------------------------
//
//	8ƒhƒbƒgƒNƒƒbƒNƒe[ƒuƒ‹
//	(HRL,HF,HD‚©‚ç“¾‚ç‚ê‚é’lB0.01ns’PˆÊ)
//
//---------------------------------------------------------------------------
const int CRTC::DotClockTable[16] = {
	// HRL=0
	164678, 82339, 164678, 164678,
	69013, 34507, 23004, 31778,
	// HRL=1
	164678, 82339, 164678, 164678,
	92017, 46009, 23004, 31778
};

//---------------------------------------------------------------------------
//
//	HRLİ’è
//
//---------------------------------------------------------------------------
void FASTCALL CRTC::SetHRL(BOOL flag)
{
	if (crtc.hrl != flag) {
		// Ÿ‚ÌüŠú‚ÅÄŒvZ
		crtc.hrl = flag;
		crtc.changed = TRUE;
	}
}

//---------------------------------------------------------------------------
//
//	HRLæ“¾
//
//---------------------------------------------------------------------------
BOOL FASTCALL CRTC::GetHRL() const
{
	return crtc.hrl;
}

//---------------------------------------------------------------------------
//
//	ƒ‰ƒXƒ^Š„‚è‚İƒ`ƒFƒbƒN
//	¦ƒCƒ“ƒ^ƒŒ[ƒXƒ‚[ƒh‚É‚Í–¢‘Î‰
//
//---------------------------------------------------------------------------
void FASTCALL CRTC::CheckRaster()
{
#if 1
	if (crtc.raster_count == crtc.raster_int) {
#else
	if (crtc.raster_count == crtc.raster_int) {
#endif
		// —v‹
		mfp->SetGPIP(6, 0);
#if defined(CRTC_LOG)
		LOG2(Log::Normal, "ƒ‰ƒXƒ^Š„‚è‚İ—v‹ raster=%d scan=%d", crtc.raster_count, crtc.v_scan);
#endif	// CRTC_LOG
	}
	else {
		// æ‚è‰º‚°
		mfp->SetGPIP(6, 1);
	}
}

//---------------------------------------------------------------------------
//
//	ƒeƒLƒXƒgVRAMŒø‰Ê
//
//---------------------------------------------------------------------------
void FASTCALL CRTC::TextVRAM()
{
	DWORD b;
	DWORD w;

	// “¯ƒAƒNƒZƒX
	if (crtc.reg[43] & 1) {
		b = (DWORD)crtc.reg[42];
		b >>= 4;

		// b4‚Íƒ}ƒ‹ƒ`ƒtƒ‰ƒO
		b |= 0x10;
		tvram->SetMulti(b);
	}
	else {
		tvram->SetMulti(0);
	}

	// ƒAƒNƒZƒXƒ}ƒXƒN
	if (crtc.reg[43] & 2) {
		w = (DWORD)crtc.reg[47];
		w <<= 8;
		w |= (DWORD)crtc.reg[46];
		tvram->SetMask(w);
	}
	else {
		tvram->SetMask(0);
	}

	// ƒ‰ƒXƒ^ƒRƒs[
	tvram->SetCopyRaster((DWORD)crtc.reg[45], (DWORD)crtc.reg[44],
						(DWORD)(crtc.reg[42] & 0x0f));
}
