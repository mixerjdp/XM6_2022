//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2005 ‚o‚hD(ytanaka@ipc-tokai.or.jp)
//	[ ƒŒƒ“ƒ_ƒ‰ ]
//
//---------------------------------------------------------------------------

#include "os.h"
#include "xm6.h"
#include "vm.h"
#include "crtc.h"
#include "vc.h"
#include "tvram.h"
#include "gvram.h"
#include "sprite.h"
#include "rend_asm.h"
#include "render.h"

//===========================================================================
//
//	ƒŒƒ“ƒ_ƒ‰
//
//===========================================================================
//#define REND_LOG

//---------------------------------------------------------------------------
//
//	’è”’è‹`
//
//---------------------------------------------------------------------------
#define REND_COLOR0		0x80000000		// ƒJƒ‰[0ƒtƒ‰ƒO(rend_asm.asm‚Åg—p)

//---------------------------------------------------------------------------
//
//	ƒRƒ“ƒXƒgƒ‰ƒNƒ^
//
//---------------------------------------------------------------------------
Render::Render(VM *p) : Device(p)
{
	// ƒfƒoƒCƒXID‚ğ‰Šú‰»
	dev.id = MAKEID('R', 'E', 'N', 'D');
	dev.desc = "Renderer";

	// ƒfƒoƒCƒXƒ|ƒCƒ“ƒ^
	crtc = NULL;
	vc = NULL;
	sprite = NULL;

	// ƒ[ƒNƒGƒŠƒA‰Šú‰»(CRTC)
	render.crtc = FALSE;
	render.width = 768;
	render.h_mul = 1;
	render.height = 512;
	render.v_mul = 1;

	// ƒ[ƒNƒGƒŠƒA‰Šú‰»(ƒpƒŒƒbƒg)
	render.palbuf = NULL;
	render.palptr = NULL;
	render.palvc = NULL;

	// ƒ[ƒNƒGƒŠƒA‰Šú‰»(ƒeƒLƒXƒg)
	render.textflag = NULL;
	render.texttv = NULL;
	render.textbuf = NULL;
	render.textout = NULL;

	// ƒ[ƒNƒGƒŠƒA‰Šú‰»(ƒOƒ‰ƒtƒBƒbƒN)
	render.grpflag = NULL;
	render.grpgv = NULL;
	render.grpbuf[0] = NULL;
	render.grpbuf[1] = NULL;
	render.grpbuf[2] = NULL;
	render.grpbuf[3] = NULL;

	// ƒ[ƒNƒGƒŠƒA‰Šú‰»(PCG,ƒXƒvƒ‰ƒCƒg,BG)
	render.pcgbuf = NULL;
	render.spptr = NULL;
	render.bgspbuf = NULL;
	render.zero = NULL;
	render.bgptr[0] = NULL;
	render.bgptr[1] = NULL;

	// ƒ[ƒNƒGƒŠƒA‰Šú‰»(‡¬)
	render.mixbuf = NULL;
	render.mixwidth = 0;
	render.mixheight = 0;
	render.mixlen = 0;
	render.mixtype = 0;
	memset(render.mixptr, 0, sizeof(render.mixptr));
	memset(render.mixshift, 0, sizeof(render.mixshift));
	memset(render.mixx, 0, sizeof(render.mixx));
	memset(render.mixy, 0, sizeof(render.mixy));
	memset(render.mixand, 0, sizeof(render.mixand));
	memset(render.mixmap, 0, sizeof(render.mixmap));

	// ƒ[ƒNƒGƒŠƒA‰Šú‰»(•`‰æ)
	render.drawflag = NULL;

	// ‚»‚Ì‘¼
	cmov = FALSE;
}

//---------------------------------------------------------------------------
//
//	‰Šú‰»
//
//---------------------------------------------------------------------------
BOOL FASTCALL Render::Init()
{
	int i;

	ASSERT(this);

	// Šî–{ƒNƒ‰ƒX
	if (!Device::Init()) {
		return FALSE;
	}

	// CRTCæ“¾
	crtc = (CRTC*)vm->SearchDevice(MAKEID('C', 'R', 'T', 'C'));
	ASSERT(crtc);

	// VCæ“¾
	vc = (VC*)vm->SearchDevice(MAKEID('V', 'C', ' ', ' '));
	ASSERT(vc);

	// ƒpƒŒƒbƒgƒoƒbƒtƒ@Šm•Û(4MB)
	try {
		render.palbuf = new DWORD[0x10000 * 16];
	}
	catch (...) {
		return FALSE;
	}
	if (!render.palbuf) {
		return FALSE;
	}

	// ƒeƒLƒXƒgVRAMƒoƒbƒtƒ@Šm•Û(4.7MB)
	try {
		render.textflag = new BOOL[1024 * 32];
		render.textbuf = new BYTE[1024 * 512];
		render.textout = new DWORD[1024 * (1024 + 1)];
	}
	catch (...) {
		return FALSE;
	}
	if (!render.textflag) {
		return FALSE;
	}
	if (!render.textbuf) {
		return FALSE;
	}
	if (!render.textout) {
		return FALSE;
	}
	for (i=0; i<1024*32; i++) {
		render.textflag[i] = TRUE;
	}
	for (i=0; i<1024; i++) {
		render.textmod[i] = TRUE;
	}

	// ƒOƒ‰ƒtƒBƒbƒNVRAMƒoƒbƒtƒ@Šm•Û(8.2MB)
	try {
		render.grpflag = new BOOL[512 * 32 * 4];
		render.grpbuf[0] = new DWORD[512 * 1024 * 4];
	}
	catch (...) {
		return FALSE;
	}
	if (!render.grpflag) {
		return FALSE;
	}
	if (!render.grpbuf[0]) {
		return FALSE;
	}
	render.grpbuf[1] = render.grpbuf[0] + 512 * 1024;
	render.grpbuf[2] = render.grpbuf[1] + 512 * 1024;
	render.grpbuf[3] = render.grpbuf[2] + 512 * 1024;
	memset(render.grpflag, 0, sizeof(BOOL) * 32 * 512 * 4);
	for (i=0; i<512*4; i++) {
		render.grpmod[i] = FALSE;
		render.grppal[i] = TRUE;
	}

	// PCGƒoƒbƒtƒ@Šm•Û(4MB)
	try {
		render.pcgbuf = new DWORD[ 16 * 256 * 16 * 16 ];
	}
	catch (...) {
		return FALSE;
	}
	if (!render.pcgbuf) {
		return FALSE;
	}

	// ƒXƒvƒ‰ƒCƒgƒ|ƒCƒ“ƒ^Šm•Û(256KB)
	try {
		render.spptr = new DWORD*[ 128 * 512 ];
	}
	catch (...) {
		return FALSE;
	}
	if (!render.spptr) {
		return FALSE;
	}

	// BGƒ|ƒCƒ“ƒ^Šm•Û(768KB)
	try {
		render.bgptr[0] = new DWORD*[ (64 * 2) * 1024 ];
		memset(render.bgptr[0], 0, sizeof(DWORD*) * (64 * 2 * 1024));
		render.bgptr[1] = new DWORD*[ (64 * 2) * 1024 ];	// from 512 to 1024 since version2.04
		memset(render.bgptr[1], 0, sizeof(DWORD*) * (64 * 2 * 1024));
	}
	catch (...) {
		return FALSE;
	}
	if (!render.bgptr[0]) {
		return FALSE;
	}
	if (!render.bgptr[1]) {
		return FALSE;
	}
	memset(render.bgall, 0, sizeof(render.bgall));
	memset(render.bgmod, 0, sizeof(render.bgmod));

	// BG/ƒXƒvƒ‰ƒCƒgƒoƒbƒtƒ@Šm•Û(1MB)
	try {
		render.bgspbuf = new DWORD[ 512 * 512 + 16];	// +16‚Íb’è‘[’u
	}
	catch (...) {
		return FALSE;
	}
	if (!render.bgspbuf) {
		return FALSE;
	}

	// •`‰æƒtƒ‰ƒOƒoƒbƒtƒ@Šm•Û(256KB)
	try {
		render.drawflag = new BOOL[64 * 1024];
	}
	catch (...) {
		return FALSE;
	}
	if (!render.drawflag) {
		return FALSE;
	}
	memset(render.drawflag, 0, sizeof(BOOL) * 64 * 1024);

	// ƒpƒŒƒbƒgì¬
	MakePalette();

	// ‚»‚Ì‘¼ƒ[ƒNƒGƒŠƒA
	render.contlevel = 0;
	cmov = ::IsCMOV();

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	ƒNƒŠ[ƒ“ƒAƒbƒv
//
//---------------------------------------------------------------------------
void FASTCALL Render::Cleanup()
{
	int i;

	ASSERT(this);

	// •`‰æƒtƒ‰ƒO
	if (render.drawflag) {
		delete[] render.drawflag;
		render.drawflag = NULL;
	}

	// BG/ƒXƒvƒ‰ƒCƒgƒoƒbƒtƒ@
	if (render.bgspbuf) {
		delete[] render.bgspbuf;
		render.bgspbuf = NULL;
	}

	// BGƒ|ƒCƒ“ƒ^
	if (render.bgptr[0]) {
		delete[] render.bgptr[0];
		render.bgptr[0] = NULL;
	}
	if (render.bgptr[1]) {
		delete[] render.bgptr[1];
		render.bgptr[1] = NULL;
	}

	// ƒXƒvƒ‰ƒCƒgƒ|ƒCƒ“ƒ^
	if (render.spptr) {
		delete[] render.spptr;
		render.spptr = NULL;
	}

	// PCGƒoƒbƒtƒ@
	if (render.pcgbuf) {
		delete[] render.pcgbuf;
		render.pcgbuf = NULL;
	}

	// ƒOƒ‰ƒtƒBƒbƒNVRAMƒoƒbƒtƒ@
	if (render.grpflag) {
		delete[] render.grpflag;
		render.grpflag = NULL;
	}
	if (render.grpbuf[0]) {
		delete[] render.grpbuf[0];
		for (i=0; i<4; i++) {
			render.grpbuf[i] = NULL;
		}
	}

	// ƒeƒLƒXƒgVRAMƒoƒbƒtƒ@
	if (render.textflag) {
		delete[] render.textflag;
		render.textflag = NULL;
	}
	if (render.textbuf) {
		delete[] render.textbuf;
		render.textbuf = NULL;
	}
	if (render.textout) {
		delete[] render.textout;
		render.textout = NULL;
	}

	// ƒpƒŒƒbƒgƒoƒbƒtƒ@
	if (render.palbuf) {
		delete[] render.palbuf;
		render.palbuf = NULL;
	}

	// Šî–{ƒNƒ‰ƒX‚Ö
	Device::Cleanup();
}

//---------------------------------------------------------------------------
//
//	ƒŠƒZƒbƒg
//
//---------------------------------------------------------------------------
void FASTCALL Render::Reset()
{
	TVRAM *tvram;
	GVRAM *gvram;
	int i;
	int j;
	int k;
	DWORD **ptr;

	ASSERT(this);
	LOG0(Log::Normal, "ƒŠƒZƒbƒg");

	// ƒrƒfƒIƒRƒ“ƒgƒ[ƒ‰‚æ‚èƒ|ƒCƒ“ƒ^æ“¾
	ASSERT(vc);
	render.palvc = (const WORD*)vc->GetPalette();

	// ƒeƒLƒXƒgVRAM‚æ‚èƒ|ƒCƒ“ƒ^æ“¾
	tvram = (TVRAM*)vm->SearchDevice(MAKEID('T', 'V', 'R', 'M'));
	ASSERT(tvram);
	render.texttv = tvram->GetTVRAM();

	// ƒOƒ‰ƒtƒBƒbƒNVRAM‚æ‚èƒ|ƒCƒ“ƒ^æ“¾
	gvram = (GVRAM*)vm->SearchDevice(MAKEID('G', 'V', 'R', 'M'));
	ASSERT(gvram);
	render.grpgv = gvram->GetGVRAM();

	// ƒXƒvƒ‰ƒCƒgƒRƒ“ƒgƒ[ƒ‰‚æ‚èƒ|ƒCƒ“ƒ^æ“¾
	sprite = (Sprite*)vm->SearchDevice(MAKEID('S', 'P', 'R', ' '));
	ASSERT(sprite);
	render.sprmem = sprite->GetPCG() - 0x8000;

	// ƒ[ƒNƒGƒŠƒA‰Šú‰»
	render.first = 0;
	render.last = 0;
	render.enable = TRUE;
	render.act = TRUE;
	render.count = 2;

	// ƒ[ƒNƒGƒŠƒA‰Šú‰»(crtc, vc)
	render.crtc = FALSE;
	render.vc = FALSE;

	// ƒ[ƒNƒGƒŠƒA‰Šú‰»(ƒRƒ“ƒgƒ‰ƒXƒg)
	render.contrast = FALSE;

	// ƒ[ƒNƒGƒŠƒA‰Šú‰»(ƒpƒŒƒbƒg)
	render.palette = FALSE;
	render.palptr = render.palbuf;

	// ƒ[ƒNƒGƒŠƒA‰Šú‰»(ƒeƒLƒXƒg)
	render.texten = FALSE;
	render.textx = 0;
	render.texty = 0;

	// ƒ[ƒNƒGƒŠƒA‰Šú‰»(ƒOƒ‰ƒtƒBƒbƒN)
	for (i=0; i<4; i++) {
		render.grpen[i] = FALSE;
		render.grpx[i] = 0;
		render.grpy[i] = 0;
	}
	render.grptype = 4;

	// ƒ[ƒNƒGƒŠƒA‰Šú‰»(PCG)
	// ƒŠƒZƒbƒg’¼Œã‚ÍBG,Sprite‚Æ‚à‚·‚×‚Ä•\¦‚µ‚È‚¢¨PCG‚Í–¢g—p
	memset(render.pcgready, 0, sizeof(render.pcgready));
	memset(render.pcguse, 0, sizeof(render.pcguse));
	memset(render.pcgpal, 0, sizeof(render.pcgpal));

	// ƒ[ƒNƒGƒŠƒA‰Šú‰»(ƒXƒvƒ‰ƒCƒg)
	memset(render.spptr, 0, sizeof(DWORD*) * 128 * 512);
	memset(render.spreg, 0, sizeof(render.spreg));
	memset(render.spuse, 0, sizeof(render.spuse));

	// ƒ[ƒNƒGƒŠƒA‰Šú‰»(BG)
	memset(render.bgreg, 0, sizeof(render.bgreg));
	render.bgdisp[0] = FALSE;
	render.bgdisp[1] = FALSE;
	render.bgarea[0] = FALSE;
	render.bgarea[1] = TRUE;
	render.bgsize = FALSE;
	render.bgx[0] = 0;
	render.bgx[1] = 0;
	render.bgy[0] = 0;
	render.bgy[1] = 0;

	// ƒ[ƒNƒGƒŠƒA‰Šú‰»(BG/ƒXƒvƒ‰ƒCƒg)
	render.bgspflag = FALSE;
	render.bgspdisp = FALSE;
	memset(render.bgspmod, 0, sizeof(render.bgspmod));

	// BG‚Ì‰Šú‰»ó‘Ô‚ğ‚Â‚­‚é(‚·‚×‚Ä0000)
	for (i=0; i<(64*64); i++) {
		render.bgreg[0][i] = 0x10000;
		render.bgreg[1][i] = 0x10000;
	}
	render.pcgready[0] = TRUE;
	render.pcguse[0] = (64 * 64) * 2;
	render.pcgpal[0] = (64 * 64) * 2;
	memset(render.pcgbuf, 0, (16 * 16) * sizeof(DWORD));
	for (i=0; i<64; i++) {
		ptr = &render.bgptr[0][i << 3];
		for (j=0; j<64; j++) {
			for (k=0; k<8; k++) {
				ptr[(k << 7) + 0] = &render.pcgbuf[k << 4];
				ptr[(k << 7) + 1] = (DWORD*)0x10000;
			}
			ptr += 2;
		}
		ptr = &render.bgptr[0][(512 + (i << 3)) << 7];
		for (j=0; j<64; j++) {
			for (k=0; k<8; k++) {
				ptr[(k << 7) + 0] = &render.pcgbuf[k << 4];
				ptr[(k << 7) + 1] = (DWORD*)0x10000;
			}
			ptr += 2;
		}
		ptr = &render.bgptr[1][i << 10];
		for (j=0; j<64; j++) {
			for (k=0; k<8; k++) {
				ptr[(k << 7) + 0] = &render.pcgbuf[k << 4];
				ptr[(k << 7) + 1] = (DWORD*)0x10000;
			}
			ptr += 2;
		}
	}

	// ƒ[ƒNƒGƒŠƒA‰Šú‰»(‡¬)
	render.mixtype = 0;
}

//---------------------------------------------------------------------------
//
//	ƒZ[ƒu
//
//---------------------------------------------------------------------------
BOOL FASTCALL Render::Save(Fileio *fio, int ver)
{
	ASSERT(this);
	LOG0(Log::Normal, "ƒZ[ƒu");

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	ƒ[ƒh
//
//---------------------------------------------------------------------------
BOOL FASTCALL Render::Load(Fileio *fio, int ver)
{
	ASSERT(this);
	LOG0(Log::Normal, "ƒ[ƒh");

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	İ’è“K—p
//
//---------------------------------------------------------------------------
void FASTCALL Render::ApplyCfg(const Config *config)
{
	ASSERT(config);
	LOG0(Log::Normal, "İ’è“K—p");
}

//---------------------------------------------------------------------------
//
//	ƒtƒŒ[ƒ€ŠJn
//
//---------------------------------------------------------------------------
void FASTCALL Render::StartFrame()
{
	CRTC::crtc_t crtcdata;
	int i;

	ASSERT(this);

	// ‚±‚ÌƒtƒŒ[ƒ€‚ÍƒXƒLƒbƒv‚·‚é‚©
	if ((render.count != 0) || !render.enable) {
		render.act = FALSE;
		return;
	}

	// ‚±‚ÌƒtƒŒ[ƒ€‚ÍƒŒƒ“ƒ_ƒŠƒ“ƒO‚·‚é
	render.act = TRUE;

	// ƒ‰ƒXƒ^‚ğƒNƒŠƒA
	render.first = 0;
	render.last = -1;

	// CRTCƒtƒ‰ƒO‚ğŒŸ¸
	if (render.crtc) {
#if defined(REND_LOG)
		LOG0(Log::Normal, "CRTCˆ—");
#endif	// REND_LOG

		// ƒf[ƒ^æ“¾
		crtc->GetCRTC(&crtcdata);

		// h_dotsAv_dots‚ª0‚È‚ç•Û—¯
		if ((crtcdata.h_dots == 0) || (crtcdata.v_dots == 0)) {
			return;
		}

		// î•ñ‚ğƒRƒs[
		render.width = crtcdata.h_dots;
		render.h_mul = crtcdata.h_mul;
		render.height = crtcdata.v_dots;
		render.v_mul = crtcdata.v_mul;
		render.lowres = crtcdata.lowres;
		if ((render.v_mul == 2) && !render.lowres) {
			render.height >>= 1;
		}

		// ‡¬ƒoƒbƒtƒ@‚Ìˆ—’·‚ğ’²®
		render.mixlen = render.width;
		if (render.mixwidth < render.width) {
			render.mixlen = render.mixwidth;
		}

		// ƒXƒvƒ‰ƒCƒgƒŠƒZƒbƒg(mixlen‚ÉˆË‘¶‚·‚é‚½‚ß)
		SpriteReset();

		// ‘Sƒ‰ƒCƒ“‡¬
		for (i=0; i<1024; i++) {
			render.mix[i] = TRUE;
		}

		// ƒIƒt
		render.crtc = FALSE;
	}
}

//---------------------------------------------------------------------------
//
//	ƒtƒŒ[ƒ€I—¹
//
//---------------------------------------------------------------------------
void FASTCALL Render::EndFrame()
{
	ASSERT(this);

	// –³Œø‚È‚ç‰½‚à‚µ‚È‚¢
	if (!render.act) {
		return;
	}

	// ‚±‚±‚Ü‚Å‚Ìƒ‰ƒXƒ^‚ğˆ—
	if (render.last > 0) {
		render.last = render.height;
		Process();
	}

	// ƒJƒEƒ“ƒgUp
	render.count++;

	// –³Œø‰»
	render.act = FALSE;
}

//---------------------------------------------------------------------------
//
//	‡¬ƒoƒbƒtƒ@ƒZƒbƒg
//
//---------------------------------------------------------------------------
void FASTCALL Render::SetMixBuf(DWORD *buf, int width, int height)
{
	int i;

	ASSERT(this);
	ASSERT(width >= 0);
	ASSERT(height >= 0);

	// İ’è
	render.mixbuf = buf;
	render.mixwidth = width;
	render.mixheight = height;

	// ‡¬ƒoƒbƒtƒ@‚Ìˆ—’·‚ğ’²®
	render.mixlen = render.width;
	if (render.mixwidth < render.width) {
		render.mixlen = render.mixwidth;
	}

	// ‚·‚×‚Ä‚Ì‡¬‚ğw¦
	for (i=0; i<1024; i++) {
		render.mix[i] = TRUE;
	}
}

//---------------------------------------------------------------------------
//
//	CRTCƒZƒbƒg
//
//---------------------------------------------------------------------------
void FASTCALL Render::SetCRTC()
{
	ASSERT(this);

	// ƒtƒ‰ƒOON‚Ì‚İ
	render.crtc = TRUE;
	render.vc = TRUE;
}

//---------------------------------------------------------------------------
//
//	VCƒZƒbƒg
//
//---------------------------------------------------------------------------
void FASTCALL Render::SetVC()
{
	ASSERT(this);

	// ƒtƒ‰ƒOON‚Ì‚İ
	render.vc = TRUE;
}

//---------------------------------------------------------------------------
//
//	VCˆ—
//
//---------------------------------------------------------------------------
void FASTCALL Render::Video()
{
	const VC::vc_t *p;
	const CRTC::crtc_t *q;
	int type;
	int i;
	int j;
	int sp;
	int gr;
	int tx;
	int map[4];
	DWORD *ptr[4];
	DWORD shift[4];
	DWORD an[4];

	// VCƒtƒ‰ƒO‚ğ~‚ë‚·
	render.vc = FALSE;

	// ƒtƒ‰ƒOON
	for (i=0; i<1024; i++) {
		render.mix[i] = TRUE;
	}

	// VCƒf[ƒ^ACRTCƒf[ƒ^‚ğæ“¾
	p = vc->GetWorkAddr();
	q = crtc->GetWorkAddr();

	// ƒeƒLƒXƒgƒCƒl[ƒuƒ‹
	if (p->ton && !q->tmem) {
		render.texten = TRUE;
	}
	else {
		render.texten = FALSE;
	}

	// ƒOƒ‰ƒtƒBƒbƒNƒ^ƒCƒv
	type = 0;
	if (!p->siz) {
		type = (int)(p->col + 1);
	}
	if (type != render.grptype) {
		render.grptype = type;
		for (i=0; i<512*4; i++) {
			render.grppal[i] = TRUE;
		}
	}

	// ƒOƒ‰ƒtƒBƒbƒNƒCƒl[ƒuƒ‹A—Dæ“xƒ}ƒbƒv
	render.mixpage = 0;
	for (i=0; i<4; i++) {
		render.grpen[i] = FALSE;
		map[i] = -1;
		an[i] = 512 - 1;
	}
	render.mixpage = 0;
	if (!q->gmem) {
		switch (render.grptype) {
			// 1024x1024x1
			case 0:
				render.grpen[0] = p->gon;
				if (render.grpen[0]) {
					map[0] = 0;
					render.mixpage = 1;
					an[0] = 1024 - 1;
				}
				break;
			// 512x512x4
			case 1:
				for (i=0; i<4; i++) {
					if (p->gs[i]) {
						ASSERT((p->gp[i] >= 0) && (p->gp[i] < 4));
						render.grpen[ p->gp[i] ] = TRUE;
						map[i] = p->gp[i];
						render.mixpage++;
					}
				}
				break;
			// 512x512x2
			case 2:
				for (i=0; i<2; i++) {
					// ƒy[ƒW0‚Ìƒ`ƒFƒbƒN
					if ((p->gp[i * 2 + 0] == 0) && (p->gp[i * 2 + 1] == 1)) {
						if (p->gs[i * 2 + 0] && p->gs[i * 2 + 1]) {
							map[i] = 0;
							render.grpen[0] = TRUE;
							render.mixpage++;
						}
					}
					// ƒy[ƒW1‚Ìƒ`ƒFƒbƒN
					if ((p->gp[i * 2 + 0] == 2) && (p->gp[i * 2 + 1] == 3)) {
						if (p->gs[i * 2 + 0] && p->gs[i * 2 + 1]) {
							map[i] = 2;
							render.grpen[2] = TRUE;
							render.mixpage++;
						}
					}
				}
				break;
			// 512x512x1
			case 3:
			case 4:
				render.grpen[0] = TRUE;
				render.mixpage = 1;
				map[0] = 0;
				for (i=0; i<4; i++) {
					if (!p->gs[i]) {
						render.grpen[0] = FALSE;
						render.mixpage = 0;
						map[0] = -1;
						break;
					}
				}
				break;
			default:
				ASSERT(FALSE);
				break;
		}
	}

	// ƒOƒ‰ƒtƒBƒbƒNƒoƒbƒtƒ@‚ğƒZƒbƒg
	j = 0;
	for (i=0; i<4; i++) {
		if (map[i] >= 0) {
			ASSERT((map[i] >= 0) && (map[i] <= 3));
			ptr[j] = render.grpbuf[ map[i] ];
			if (render.grptype == 0) {
				shift[j] = 11;
			}
			else {
				shift[j] = 10;
			}
			ASSERT(j <= i);
			map[j] = map[i];
			j++;
		}
	}

	// —Dæ‡ˆÊ‚ğæ“¾
	tx = p->tx;
	sp = p->sp;
	gr = p->gr;

	// ƒ^ƒCƒv‰Šú‰»
	render.mixtype = 0;

	// BG/ƒXƒvƒ‰ƒCƒg•\¦Ø‘Ö‚©
	if ((q->hd >= 2) || (!p->son)) {
		if (render.bgspflag) {
			// BG/ƒXƒvƒ‰ƒCƒg•\¦ON->OFF
			render.bgspflag = FALSE;
			for (i=0; i<512; i++) {
				render.bgspmod[i] = TRUE;
			}
			render.bgspdisp = sprite->IsDisplay();
		}
	}
	else {
		if (!render.bgspflag) {
			// BG/ƒXƒvƒ‰ƒCƒg•\¦OFF->ON
			render.bgspflag = TRUE;
			for (i=0; i<512; i++) {
				render.bgspmod[i] = TRUE;
			}
			render.bgspdisp = sprite->IsDisplay();
		}
	}

	// İ’è(q->hd >= 2‚Ìê‡‚ÍƒXƒvƒ‰ƒCƒg–Ê‚È‚µ)
	if ((q->hd >= 2) || (!p->son)) {
		// ƒXƒvƒ‰ƒCƒg‚È‚µ
		if (!render.texten) {
			// ƒeƒLƒXƒg‚È‚µ
			if (render.mixpage == 0) {
				// ƒOƒ‰ƒtƒBƒbƒN‚È‚µ(type=0)
				render.mixtype = 0;
				return;
			}
			if (render.mixpage == 1) {
				// ƒOƒ‰ƒtƒBƒbƒN1–Ê‚Ì‚İ(type=1)
				render.mixptr[0] = ptr[0];
				render.mixshift[0] = shift[0];
				render.mixx[0] = &render.grpx[ map[0] ];
				render.mixy[0] = &render.grpy[ map[0] ];
				render.mixand[0] = an[0];
				render.mixtype = 1;
				return;
			}
			if (render.mixpage == 2) {
				// ƒOƒ‰ƒtƒBƒbƒN2–Ê‚Ì‚İ(type=2)
				for (i=0; i<2; i++) {
					render.mixptr[i] = ptr[i];
					render.mixshift[i] = shift[i];
					render.mixx[i] = &render.grpx[ map[i] ];
					render.mixy[i] = &render.grpy[ map[i] ];
					render.mixand[i] = an[i];
				}
				render.mixtype = 2;
				return;
			}
			ASSERT((render.mixpage == 3) || (render.mixpage == 4));
			// ƒOƒ‰ƒtƒBƒbƒN3–ÊˆÈã‚Ì‚İ(type=4)
			for (i=0; i<render.mixpage; i++) {
				render.mixptr[i + 4] = ptr[i];
				render.mixshift[i + 4] = shift[i];
				render.mixx[i + 4] = &render.grpx[ map[i] ];
				render.mixy[i + 4] = &render.grpy[ map[i] ];
				render.mixand[i + 4] = an[i];
			}
			render.mixtype = 4;
			return;
		}
		// ƒeƒLƒXƒg‚ ‚è
		if (render.mixpage == 0) {
			// ƒOƒ‰ƒtƒBƒbƒN‚È‚µBƒeƒLƒXƒg‚Ì‚İ(type=1)
			render.mixptr[0] = render.textout;
			render.mixshift[0] = 10;
			render.mixx[0] = &render.textx;
			render.mixy[0] = &render.texty;
			render.mixand[0] = 1024 - 1;
			render.mixtype = 1;
				return;
		}
		if (render.mixpage == 1) {
			// ƒeƒLƒXƒg+ƒOƒ‰ƒtƒBƒbƒN1–Ê
			if (tx < gr) {
				// ƒeƒLƒXƒg‘O–Ê(type=3)
				render.mixptr[0] = render.textout;
				render.mixshift[0] = 10;
				render.mixx[0] = &render.textx;
				render.mixy[0] = &render.texty;
				render.mixand[0] = 1024 - 1;
				render.mixptr[1] = ptr[0];
				render.mixshift[1] = shift[0];
				render.mixx[1] = &render.grpx[ map[0] ];
				render.mixy[1] = &render.grpy[ map[0] ];
				render.mixand[1] = an[0];
				render.mixtype = 3;
				return;
			}
			// ƒOƒ‰ƒtƒBƒbƒN‘O–Ê(type=3,tx=gr‚ÍƒOƒ‰ƒtƒBƒbƒN‘O–ÊA‘åí—ªII)
			render.mixptr[1] = render.textout;
			render.mixshift[1] = 10;
			render.mixx[1] = &render.textx;
			render.mixy[1] = &render.texty;
			render.mixand[1] = 1024 - 1;
			render.mixptr[0] = ptr[0];
			render.mixshift[0] = shift[0];
			render.mixx[0] = &render.grpx[ map[0] ];
			render.mixy[0] = &render.grpy[ map[0] ];
			render.mixand[0] = an[0];
			render.mixtype = 3;
			return;
		}
		// ƒeƒLƒXƒg+ƒOƒ‰ƒtƒBƒbƒN2–ÊˆÈã(type=5, type=6)
		ASSERT((render.mixpage >= 2) && (render.mixpage <= 6));
		render.mixptr[0] = render.textout;
		render.mixshift[0] = 10;
		render.mixx[0] = &render.textx;
		render.mixy[0] = &render.texty;
		render.mixand[0] = 1024 - 1;
		for (i=0; i<render.mixpage; i++) {
			render.mixptr[i + 4] = ptr[i];
			render.mixshift[i + 4] = shift[i];
			render.mixx[i + 4] = &render.grpx[ map[i] ];
			render.mixy[i + 4] = &render.grpy[ map[i] ];
			render.mixand[i + 4] = an[i];
		}
		if (tx < gr) {
			render.mixtype = 5;
		}
		else {
			render.mixtype = 6;
		}
		return;
	}

	// ƒXƒvƒ‰ƒCƒg‚ ‚è
	if (!render.texten) {
		// ƒeƒLƒXƒg‚È‚µ
		if (render.mixpage == 0) {
			// ƒOƒ‰ƒtƒBƒbƒN‚È‚µAƒXƒvƒ‰ƒCƒg‚Ì‚İ(type=1)
			render.mixptr[0] = render.bgspbuf;
			render.mixshift[0] = 9;
			render.mixx[0] = &render.zero;
			render.mixy[0] = &render.zero;
			render.mixand[0] = 512 - 1;
			render.mixtype = 1;
			return;
		}
		if (render.mixpage == 1) {
			// ƒXƒvƒ‰ƒCƒg+ƒOƒ‰ƒtƒBƒbƒN1–Ê(type=3)
			if (sp < gr) {
				// ƒXƒvƒ‰ƒCƒg‘O–Ê
				render.mixptr[0] = render.bgspbuf;
				render.mixshift[0] = 9;
				render.mixx[0] = &render.zero;
				render.mixy[0] = &render.zero;
				render.mixand[0] = 512 - 1;
				render.mixptr[1] = ptr[0];
				render.mixshift[1] = shift[0];
				render.mixx[1] = &render.grpx[ map[0] ];
				render.mixy[1] = &render.grpy[ map[0] ];
				render.mixand[1] = an[0];
				render.mixtype = 3;
				return;
			}
			// ƒOƒ‰ƒtƒBƒbƒN‘O–Ê(sp=gr‚Í•s–¾)
			render.mixptr[1] = render.bgspbuf;
			render.mixshift[1] = 9;
			render.mixx[1] = &render.zero;
			render.mixy[1] = &render.zero;
			render.mixand[1] = 512 - 1;
			render.mixptr[0] = ptr[0];
			render.mixshift[0] = shift[0];
			render.mixx[0] = &render.grpx[ map[0] ];
			render.mixy[0] = &render.grpy[ map[0] ];
			render.mixand[0] = an[0];
			render.mixtype = 3;
			return;
		}
		// ƒXƒvƒ‰ƒCƒg+ƒOƒ‰ƒtƒBƒbƒN2–ÊˆÈã(type=5, type=6)
		ASSERT((render.mixpage >= 2) && (render.mixpage <= 4));
		render.mixptr[0] = render.bgspbuf;
		render.mixshift[0] = 9;
		render.mixx[0] = &render.zero;
		render.mixy[0] = &render.zero;
		render.mixand[0] = 512 - 1;
		for (i=0; i<render.mixpage; i++) {
			render.mixptr[i + 4] = ptr[i];
			render.mixshift[i + 4] = shift[i];
			render.mixx[i + 4] = &render.grpx[ map[i] ];
			render.mixy[i + 4] = &render.grpy[ map[i] ];
			render.mixand[i + 4] = an[i];
		}
		if (sp < gr) {
			render.mixtype = 5;
		}
		else {
			render.mixtype = 6;
		}
		return;
	}

	// ƒeƒLƒXƒg‚ ‚è
	if (render.mixpage == 0) {
		// ƒOƒ‰ƒtƒBƒbƒN‚È‚µBƒeƒLƒXƒg{ƒXƒvƒ‰ƒCƒg(type=3)
		if (tx <= sp) {
			// tx=sp‚ÍƒeƒLƒXƒg‘O–Ê(LMZ2)
			render.mixptr[0] = render.textout;
			render.mixshift[0] = 10;
			render.mixx[0] = &render.textx;
			render.mixy[0] = &render.texty;
			render.mixand[0] = 1024 - 1;
			render.mixptr[1] = render.bgspbuf;
			render.mixshift[1] = 9;
			render.mixx[1] = &render.zero;
			render.mixy[1] = &render.zero;
			render.mixand[1] = 512 - 1;
			render.mixtype = 3;
			return;
		}
		// ƒXƒvƒ‰ƒCƒg‘O–Ê
		render.mixptr[1] = render.textout;
		render.mixshift[1] = 10;
		render.mixx[1] = &render.textx;
		render.mixy[1] = &render.texty;
		render.mixand[1] = 1024 - 1;
		render.mixptr[0] = render.bgspbuf;
		render.mixshift[0] = 9;
		render.mixx[0] = &render.zero;
		render.mixy[0] = &render.zero;
		render.mixand[0] = 512 - 1;
		render.mixtype = 3;
		return;
	}

	// —Dæ‡ˆÊŒˆ’è
	if (tx == 3) tx--;
	if (sp == 3) sp--;
	if (gr == 3) gr--;
	if (tx == sp) {
		// “K“–‚ÉŒˆ‚ß‚Ä‚¢‚é
		if (tx < gr) {
			tx = 0;
			sp = 1;
			gr = 2;
		}
		else {
			gr = 0;
			tx = 1;
			sp = 2;
		}
	}
	if (tx == gr) {
		// “K“–‚ÉŒˆ‚ß‚Ä‚¢‚é
		if (tx < sp) {
			tx = 0;
			gr = 1;
			sp = 2;
		}
		else {
			sp = 0;
			tx = 1;
			gr = 2;
		}
	}
	if (sp == gr) {
		// “K“–‚ÉŒˆ‚ß‚Ä‚¢‚é
		if (sp < tx) {
			sp = 0;
			gr = 1;
			tx = 2;
		}
		else {
			tx = 0;
			sp = 1;
			gr = 2;
		}
	}
	ASSERT((tx != gr) && (gr != sp) && (tx != sp));
	ASSERT((tx >= 0) && (tx < 3));
	ASSERT((sp >= 0) && (sp < 3));
	ASSERT((gr >= 0) && (gr < 3));
	render.mixmap[tx] = 0;
	render.mixmap[sp] = 1;
	render.mixmap[gr] = 2;

	if (render.mixpage == 1) {
		// ƒeƒLƒXƒg{ƒXƒvƒ‰ƒCƒg{ƒOƒ‰ƒtƒBƒbƒN1–Ê(type=7)
		render.mixptr[0] = render.textout;
		render.mixshift[0] = 10;
		render.mixx[0] = &render.textx;
		render.mixy[0] = &render.texty;
		render.mixand[0] = 1024 - 1;
		render.mixptr[1] = render.bgspbuf;
		render.mixshift[1] = 9;
		render.mixx[1] = &render.zero;
		render.mixy[1] = &render.zero;
		render.mixand[1] = 512 - 1;
		render.mixptr[2] = ptr[0];
		render.mixshift[2] = shift[0];
		render.mixx[2] = &render.grpx[ map[0] ];
		render.mixy[2] = &render.grpy[ map[0] ];
		render.mixand[2] = an[0];
		render.mixtype = 7;
		return;
	}

	// ƒeƒLƒXƒg{ƒXƒvƒ‰ƒCƒg{ƒOƒ‰ƒtƒBƒbƒN‚Q–ÊˆÈã(type=8)
	render.mixptr[0] = render.textout;
	render.mixshift[0] = 10;
	render.mixx[0] = &render.textx;
	render.mixy[0] = &render.texty;
	render.mixand[0] = 1024 - 1;
	render.mixptr[1] = render.bgspbuf;
	render.mixshift[1] = 9;
	render.mixx[1] = &render.zero;
	render.mixy[1] = &render.zero;
	render.mixand[1] = 512 - 1;
	for (i=0; i<render.mixpage; i++) {
		render.mixptr[i + 4] = ptr[i];
		render.mixshift[i + 4] = shift[i];
		render.mixx[i + 4] = &render.grpx[ map[i] ];
		render.mixy[i + 4] = &render.grpy[ map[i] ];
		render.mixand[i + 4] = an[i];
	}
	render.mixtype = 8;
}

//---------------------------------------------------------------------------
//
//	ƒRƒ“ƒgƒ‰ƒXƒgİ’è
//
//---------------------------------------------------------------------------
void FASTCALL Render::SetContrast(int cont)
{
	// ƒVƒXƒeƒ€ƒ|[ƒg‚Ì“_‚Åˆê’vƒ`ƒFƒbƒN‚ğs‚¤‚Ì‚ÅAˆÙ‚È‚Á‚Ä‚¢‚éê‡‚Ì‚İ
	ASSERT(this);
	ASSERT((cont >= 0) && (cont <= 15));

	// •ÏX‚Æƒtƒ‰ƒOON
	render.contlevel = cont;
	render.contrast = TRUE;
}

//---------------------------------------------------------------------------
//
//	ƒRƒ“ƒgƒ‰ƒXƒgæ“¾
//
//---------------------------------------------------------------------------
int FASTCALL Render::GetContrast() const
{
	ASSERT(this);
	ASSERT((render.contlevel >= 0) && (render.contlevel <= 15));

	return render.contlevel;
}

//---------------------------------------------------------------------------
//
//	ƒRƒ“ƒgƒ‰ƒXƒgˆ—
//
//---------------------------------------------------------------------------
void FASTCALL Render::Contrast()
{
	int i;

	// ƒ|ƒCƒ“ƒgˆÊ’u‚ğ•ÏXAƒtƒ‰ƒODown
	render.palptr = render.palbuf;
	render.palptr += (render.contlevel << 16);
	render.contrast = FALSE;

	// ƒpƒŒƒbƒgƒtƒ‰ƒO‚ğ‘S‚ÄUp
	for (i=0; i<0x200; i++) {
		render.palmod[i] = TRUE;
	}
	render.palette = TRUE;
}

//---------------------------------------------------------------------------
//
//	ƒpƒŒƒbƒgì¬
//
//---------------------------------------------------------------------------
void FASTCALL Render::MakePalette()
{
	DWORD *p;
	int ratio;
	int i;
	int j;

	ASSERT(render.palbuf);

	// ‰Šú‰»
	p = render.palbuf;

	// ƒRƒ“ƒgƒ‰ƒXƒgƒ‹[ƒv
	for (i=0; i<16; i++) {
		// ”ä—¦‚ğZo
		ratio = 256 - ((15 - i) << 4);

		// ì¬ƒ‹[ƒv
		for (j=0; j<0x10000; j++) {
			*p++ = ConvPalette(j, ratio);
		}
	}
}

//---------------------------------------------------------------------------
//
//	ƒpƒŒƒbƒg•ÏŠ·
//
//---------------------------------------------------------------------------
DWORD FASTCALL Render::ConvPalette(int color, int ratio)
{
	DWORD r;
	DWORD g;
	DWORD b;

	// assert
	ASSERT((color >= 0) && (color < 0x10000));
	ASSERT((ratio >= 0) && (ratio <= 0x100));

	// ‘S‚ÄƒRƒs[
	r = (DWORD)color;
	g = (DWORD)color;
	b = (DWORD)color;

	// MSB‚©‚çG:5AR:5AB:5AI:1‚Ì‡‚É‚È‚Á‚Ä‚¢‚é
	// ‚±‚ê‚ğ R:8 G:8 B:8‚ÌDWORD‚É•ÏŠ·Bb31-b24‚Íg‚í‚È‚¢
	r <<= 13;
	r &= 0xf80000;
	g &= 0x00f800;
	b <<= 2;
	b &= 0x0000f8;

	// ‹P“xƒrƒbƒg‚Íˆê—¥Up(Œ³ƒf[ƒ^‚ª0‚Ìê‡‚àA!=0‚É‚·‚éŒø‰Ê‚ ‚è)
	if (color & 1) {
		r |= 0x070000;
		g |= 0x000700;
		b |= 0x000007;
	}

	// ƒRƒ“ƒgƒ‰ƒXƒg‚ğ‰e‹¿‚³‚¹‚é
	b *= ratio;
	b >>= 8;
	g *= ratio;
	g >>= 8;
	g &= 0xff00;
	r *= ratio;
	r >>= 8;
	r &= 0xff0000;

	return (DWORD)(r | g | b);
}

//---------------------------------------------------------------------------
//
//	ƒpƒŒƒbƒgæ“¾
//
//---------------------------------------------------------------------------
const DWORD* FASTCALL Render::GetPalette() const
{
	ASSERT(this);
	ASSERT(render.paldata);

	return render.paldata;
}

//---------------------------------------------------------------------------
//
//	ƒpƒŒƒbƒgˆ—
//
//---------------------------------------------------------------------------
void FASTCALL Render::Palette()
{
	DWORD data;
	BOOL tx;
	BOOL gr;
	BOOL sp;
	int i;
	int j;

	// ƒtƒ‰ƒOOFF
	tx = FALSE;
	gr = FALSE;
	sp = FALSE;

	// ƒOƒ‰ƒtƒBƒbƒN
	for (i=0; i<0x100; i++) {
		if (render.palmod[i]) {
			data = (DWORD)render.palvc[i];
			render.paldata[i] = render.palptr[data];

			// ƒOƒ‰ƒtƒBƒbƒN‚É‰e‹¿Aƒtƒ‰ƒOOFF
			gr = TRUE;
			render.palmod[i] = FALSE;

			// “§–¾F‚Ìˆ—
			if (i == 0) {
				render.paldata[i] |= REND_COLOR0;
			}

			// 65536F‚Ì‚½‚ß‚ÌƒpƒŒƒbƒgƒf[ƒ^İ’è
			j = i >> 1;
			if (i & 1) {
				j += 128;
			}
			render.pal64k[j * 2 + 0] = (BYTE)(data >> 8);
			render.pal64k[j * 2 + 1] = (BYTE)data;
		}
	}

	// ƒeƒLƒXƒgŒ“ƒXƒvƒ‰ƒCƒg
	for (i=0x100; i<0x110; i++) {
		if (render.palmod[i]) {
			data = (DWORD)render.palvc[i];
			render.paldata[i] = render.palptr[data];

			// ƒeƒLƒXƒg‚É‰e‹¿Aƒtƒ‰ƒOOFF
			tx = TRUE;
			render.palmod[i] = FALSE;

			// “§–¾F‚Ìˆ—
			if (i == 0x100) {
				render.paldata[i] |= REND_COLOR0;
				// 0x100‚ÍBGEƒXƒvƒ‰ƒCƒg‚É‚à•K‚¸‰e‹¿
				sp = TRUE;
			}

			// PCGŒŸ¸
			memset(&render.pcgready[0], 0, sizeof(BOOL) * 256);
			if (render.pcgpal[0] > 0) {
				sp = TRUE;
			}
		}
	}

	// ƒXƒvƒ‰ƒCƒg
	for (i=0x110; i<0x200; i++) {
		if (render.palmod[i]) {
			// ƒXƒvƒ‰ƒCƒg‚É‰e‹¿Aƒtƒ‰ƒOOFF
			data = (DWORD)render.palvc[i];
			render.paldata[i] = render.palptr[data];
			render.palmod[i] = FALSE;

			// “§–¾F‚Ìˆ—
			if ((i & 0x00f) == 0) {
				render.paldata[i] |= REND_COLOR0;
			}

			// PCGŒŸ¸
			memset(&render.pcgready[(i & 0xf0) << 4], 0, sizeof(BOOL) * 256);
			if (render.pcgpal[(i & 0xf0) >> 4] > 0) {
				sp = TRUE;
			}
		}
	}

	// ƒOƒ‰ƒtƒBƒbƒNƒtƒ‰ƒO
	if (gr) {
		// ƒtƒ‰ƒOON
		for (i=0; i<512*4; i++) {
			render.grppal[i] = TRUE;
		}
	}

	// ƒeƒLƒXƒgƒtƒ‰ƒO
	if (tx) {
		for (i=0; i<1024; i++) {
			render.textpal[i] = TRUE;
		}
	}

	// ƒXƒvƒ‰ƒCƒgƒtƒ‰ƒO
	if (sp) {
		for (i=0; i<512; i++) {
			render.bgspmod[i] = TRUE;
		}
	}

	// ƒpƒŒƒbƒgƒtƒ‰ƒOOFF
	render.palette = FALSE;
}

//---------------------------------------------------------------------------
//
//	ƒeƒLƒXƒgƒXƒNƒ[ƒ‹
//
//---------------------------------------------------------------------------
void FASTCALL Render::TextScrl(DWORD x, DWORD y)
{
	int i;

	ASSERT(this);
	ASSERT(x < 1024);
	ASSERT(y < 1024);

	// ”äŠrƒ`ƒFƒbƒN
	if ((render.textx == x) && (render.texty == y)) {
		return;
	}

	// ƒ[ƒNXV
	render.textx = x;
	render.texty = y;

	// ƒtƒ‰ƒOON
	if (render.texten) {
#if defined(REND_LOG)
		LOG2(Log::Normal, "ƒeƒLƒXƒgƒXƒNƒ[ƒ‹ x=%d y=%d", x, y);
#endif	// REND_LOG

		for (i=0; i<1024; i++) {
			render.mix[i] = TRUE;
		}
	}
}

//---------------------------------------------------------------------------
//
//	ƒeƒLƒXƒgƒRƒs[
//
//---------------------------------------------------------------------------
void FASTCALL Render::TextCopy(DWORD src, DWORD dst, DWORD plane)
{
	ASSERT(this);
	ASSERT((src >= 0) && (src < 256));
	ASSERT((dst >= 0) && (dst < 256));
	ASSERT(plane < 16);

	// ƒAƒZƒ“ƒuƒ‰ƒTƒu
	RendTextCopy(&render.texttv[src << 9],
				 &render.texttv[dst << 9],
				 plane,
				 &render.textflag[dst << 7],
				 &render.textmod[dst << 2]);
}

//---------------------------------------------------------------------------
//
//	ƒeƒLƒXƒgƒoƒbƒtƒ@æ“¾
//
//---------------------------------------------------------------------------
const DWORD* FASTCALL Render::GetTextBuf() const
{
	ASSERT(this);
	ASSERT(render.textout);

	return render.textout;
}

//---------------------------------------------------------------------------
//
//	ƒeƒLƒXƒgˆ—
//
//---------------------------------------------------------------------------
void FASTCALL Render::Text(int raster)
{
	int y;

	// assert
	ASSERT((raster >= 0) && (raster < 1024));
	ASSERT(render.texttv);
	ASSERT(render.textflag);
	ASSERT(render.textbuf);
	ASSERT(render.palbuf);

	// ƒfƒBƒZ[ƒuƒ‹‚È‚ç‰½‚à‚µ‚È‚¢
	if (!render.texten) {
		return;
	}

	// À‰æ–ÊYZo
	y = (raster + render.texty) & 0x3ff;

	// •ÏXƒtƒ‰ƒO(’€ŸŒ^)
	if (render.textmod[y]) {
		// ƒtƒ‰ƒOˆ—
		render.textmod[y] = FALSE;
		render.mix[raster] = TRUE;

		// …•½‚’¼•ÏŠ·
		RendTextMem(render.texttv + (y << 7),
					render.textflag + (y << 5),
					render.textbuf + (y << 9));

		// ‚’¼ƒpƒŒƒbƒg•ÏŠ·
		RendTextPal(render.textbuf + (y << 9),
					render.textout + (y << 10),
					render.textflag + (y << 5),
					render.paldata + 0x100);
	}

	// ƒpƒŒƒbƒg(ˆêŠ‡Œ^)
	if (render.textpal[y]) {
		// ƒtƒ‰ƒOˆ—
		render.textpal[y] = FALSE;

		// ‚’¼ƒpƒŒƒbƒg•ÏŠ·
		RendTextAll(render.textbuf + (y << 9),
					render.textout + (y << 10),
					render.paldata + 0x100);
		render.mix[raster] = TRUE;

		// y == 1023‚È‚çƒRƒs[‚·‚é
		if (y == 1023) {
			memcpy(render.textout + (1024 << 10), render.textout + (1023 << 10), sizeof(DWORD) * 1024);
		}
	}
}

//---------------------------------------------------------------------------
//
//	ƒOƒ‰ƒtƒBƒbƒNƒoƒbƒtƒ@æ“¾
//
//---------------------------------------------------------------------------
const DWORD* FASTCALL Render::GetGrpBuf(int index) const
{
	ASSERT(this);
	ASSERT((index >= 0) && (index <= 3));

	ASSERT(render.grpbuf[index]);
	return render.grpbuf[index];
}

//---------------------------------------------------------------------------
//
//	ƒOƒ‰ƒtƒBƒbƒNƒXƒNƒ[ƒ‹
//
//---------------------------------------------------------------------------
void FASTCALL Render::GrpScrl(int block, DWORD x, DWORD y)
{
	BOOL flag;
	int i;

	ASSERT(this);
	ASSERT((block >= 0) && (block <= 3));
	ASSERT(x < 1024);
	ASSERT(y < 1024);

	// ”äŠrƒ`ƒFƒbƒNB”ñ•\¦‚È‚çXV‚È‚µ
	flag = FALSE;
	if ((render.grpx[block] != x) || (render.grpy[block] != y)) {
		render.grpx[block] = x;
		render.grpy[block] = y;
		flag = render.grpen[block];
	}

	// ƒtƒ‰ƒOˆ—
	if (!flag) {
		return;
	}

#if defined(REND_LOG)
	LOG3(Log::Normal, "ƒOƒ‰ƒtƒBƒbƒNƒXƒNƒ[ƒ‹ block=%d x=%d y=%d", block, x, y);
#endif	// REND_LOG

	for (i=0; i<1024; i++) {
		render.mix[i] = TRUE;
	}
}

//---------------------------------------------------------------------------
//
//	ƒOƒ‰ƒtƒBƒbƒNˆ—
//
//---------------------------------------------------------------------------
void FASTCALL Render::Grp(int block, int raster)
{
	int i;
	int y;
	int offset;

	ASSERT((block >= 0) && (block <= 3));
	ASSERT((raster >= 0) && (raster < 1024));
	ASSERT(render.grpbuf[block]);
	ASSERT(render.grpgv);

	if (render.grptype == 0) {
		// 1024ƒ‚[ƒh‚Íƒy[ƒW0‚ğ‚İ‚é
		if (!render.grpen[0]) {
			return;
		}
	}
	else {
		// ‚»‚êˆÈŠO
		if (!render.grpen[block]) {
			return;
		}
	}

	// ƒ^ƒCƒv•Ê
	switch (render.grptype) {
		// ƒ^ƒCƒv0:1024~1024 16Color
		case 0:
			// ƒIƒtƒZƒbƒgZo
			offset = (raster + render.grpy[0]) & 0x3ff;
			y = offset & 0x1ff;

			// •\¦‘ÎÛƒ`ƒFƒbƒN
			if ((offset < 512) && (block >= 2)) {
				return;
			}
			if ((offset >= 512) && (block < 2)) {
				return;
			}

			// ƒpƒŒƒbƒg‚Ìê‡‚Í‘S—Ìˆæˆ—
			if (render.grppal[y + (block << 9)]) {
				render.grppal[y + (block << 9)] = FALSE;
				render.grpmod[y + (block << 9)] = FALSE;
				for (i=0; i<32; i++) {
					render.grpflag[(y << 5) + (block << 14) + i] = FALSE;
				}
				switch (block) {
					// ã”¼•ª‚ÍƒuƒƒbƒN0‚Å‘ã•
					case 0:
						if (Rend1024A(render.grpgv + (y << 10),
									render.grpbuf[0] + (offset << 11),
									render.paldata) != 0) {
							render.mix[raster] = TRUE;
						}
					case 1:
						break;
					// ‰º”¼•ª‚ÍƒuƒƒbƒN2‚Å‘ã•
					case 2:
						if (Rend1024B(render.grpgv + (y << 10),
									render.grpbuf[0] + (offset << 11),
									render.paldata) != 0) {
							render.mix[raster] = TRUE;
						}
					case 3:
						break;
				}
				return;
			}

			// ‚»‚êˆÈŠO‚Ígrpmod‚ğŒ©‚Äˆ—
			if (!render.grpmod[y + (block << 9)]) {
				return;
			}
			render.grpmod[y + (block << 9)] = FALSE;
			render.mix[raster] = TRUE;
			switch (block) {
				// ƒuƒƒbƒN0-¶ã
				case 0:
					Rend1024C(render.grpgv + (y << 10),
								render.grpbuf[0] + (offset << 11),
								render.grpflag + (y << 5),
								render.paldata);
					break;
				// ƒuƒƒbƒN1-‰Eã
				case 1:
					Rend1024D(render.grpgv + (y << 10),
								render.grpbuf[0] + (offset << 11),
								render.grpflag + (y << 5) + 0x4000,
								render.paldata);
					break;
				// ƒuƒƒbƒN2-¶‰º
				case 2:
					Rend1024E(render.grpgv + (y << 10),
								render.grpbuf[0] + (offset << 11),
								render.grpflag + (y << 5) + 0x8000,
								render.paldata);
					break;
				// ƒuƒƒbƒN3-‰E‰º
				case 3:
					Rend1024F(render.grpgv + (y << 10),
								render.grpbuf[0] + (offset << 11),
								render.grpflag + (y << 5) + 0xc000,
								render.paldata);
					break;
			}
			return;

		// ƒ^ƒCƒv1:512~512 16Color
		case 1:
			switch (block) {
				// ƒy[ƒW0
				case 0:
					y = (raster + render.grpy[0]) & 0x1ff;
					// ƒpƒŒƒbƒg
					if (render.grppal[y]) {
						render.grppal[y] = FALSE;
						render.grpmod[y] = FALSE;
						for (i=0; i<32; i++) {
							render.grpflag[(y << 5) + i] = FALSE;
						}
						if (Rend16A(render.grpgv + (y << 10),
										render.grpbuf[0] + (y << 10),
										render.paldata) != 0) {
							render.mix[raster] = TRUE;
						}
						return;
					}
					// ’Êí
					if (render.grpmod[y]) {
						render.grpmod[y] = FALSE;
						render.mix[raster] = TRUE;
						Rend16B(render.grpgv + (y << 10),
								render.grpbuf[0] + (y << 10),
								render.grpflag + (y << 5),
								render.paldata);
					}
					return;
				// ƒy[ƒW1
				case 1:
					y = (raster + render.grpy[1]) & 0x1ff;
					// ƒpƒŒƒbƒg
					if (render.grppal[y + 512]) {
						render.grppal[y + 512] = FALSE;
						render.grpmod[y + 512] = FALSE;
						for (i=0; i<32; i++) {
							render.grpflag[(y << 5) + i + 0x4000] = FALSE;
						}
						if (Rend16C(render.grpgv + (y << 10),
										render.grpbuf[1] + (y << 10),
										render.paldata) != 0) {
							render.mix[raster] = TRUE;
						}
						return;
					}
					// ’Êí
					if (render.grpmod[y + 512]) {
						render.grpmod[y + 512] = FALSE;
						render.mix[raster] = TRUE;
						Rend16D(render.grpgv + (y << 10),
								render.grpbuf[1] + (y << 10),
								render.grpflag + (y << 5) + 0x4000,
								render.paldata);
					}
					return;
				// ƒy[ƒW2
				case 2:
					y = (raster + render.grpy[2]) & 0x1ff;
					// ƒpƒŒƒbƒg
					if (render.grppal[y + 1024]) {
						render.grppal[y + 1024] = FALSE;
						render.grpmod[y + 1024] = FALSE;
						for (i=0; i<32; i++) {
							render.grpflag[(y << 5) + i + 0x8000] = FALSE;
						}
						if (Rend16E(render.grpgv + (y << 10),
										render.grpbuf[2] + (y << 10),
										render.paldata) != 0) {
							render.mix[raster] = TRUE;
						}
						return;
					}
					// ’Êí
					if (render.grpmod[y + 1024]) {
						render.grpmod[y + 1024] = FALSE;
						render.mix[raster] = TRUE;
						Rend16F(render.grpgv + (y << 10),
								render.grpbuf[2] + (y << 10),
								render.grpflag + (y << 5) + 0x8000,
								render.paldata);
					}
					return;
				// ƒy[ƒW3
				case 3:
					y = (raster + render.grpy[3]) & 0x1ff;
					// ƒpƒŒƒbƒg
					if (render.grppal[y + 1536]) {
						render.grppal[y + 1536] = FALSE;
						render.grpmod[y + 1536] = FALSE;
						for (i=0; i<32; i++) {
							render.grpflag[(y << 5) + i + 0xC000] = FALSE;
						}
						if (Rend16G(render.grpgv + (y << 10),
										render.grpbuf[3] + (y << 10),
										render.paldata) != 0) {
							render.mix[raster] = TRUE;
						}
						return;
					}
					// ’Êí
					if (render.grpmod[y + 1536]) {
						render.grpmod[y + 1536] = FALSE;
						render.mix[raster] = TRUE;
						Rend16H(render.grpgv + (y << 10),
								render.grpbuf[3] + (y << 10),
								render.grpflag + (y << 5) + 0xC000,
								render.paldata);
					}
					return;
			}
			return;

		// ƒ^ƒCƒv2:512~512 256Color
		case 2:
			ASSERT((block == 0) || (block == 2));
			if (block == 0) {
				// ƒIƒtƒZƒbƒgZo
				y = (raster + render.grpy[0]) & 0x1ff;

				// ƒpƒŒƒbƒg‚Ìê‡‚Í‘S—Ìˆæˆ—
				if (render.grppal[y]) {
					render.grppal[y] = FALSE;
					render.grpmod[y] = FALSE;
					for (i=0; i<32; i++) {
						render.grpflag[(y << 5) + i] = FALSE;
					}
					if (Rend256C(render.grpgv + (y << 10),
									render.grpbuf[0] + (y << 10),
									render.paldata) != 0) {
						render.mix[raster] = TRUE;
					}
					return;
				}

				// ‚»‚êˆÈŠO‚Ígrpmod‚ğŒ©‚Äˆ—
				if (!render.grpmod[y]) {
					return;
				}

				render.grpmod[y] = FALSE;
				render.mix[raster] = TRUE;
				Rend256A(render.grpgv + (y << 10),
							render.grpbuf[0] + (y << 10),
							render.grpflag + (y << 5),
							render.paldata);
			}
			else {
				// ƒIƒtƒZƒbƒgZo
				y = (raster + render.grpy[2]) & 0x1ff;

				// ƒpƒŒƒbƒg‚Ìê‡‚Í‘S—Ìˆæˆ—
				if (render.grppal[0x400 + y]) {
					render.grppal[0x400 + y] = FALSE;
					render.grpmod[0x400 + y] = FALSE;
					for (i=0; i<32; i++) {
						render.grpflag[(y << 5) + i + 0x8000] = FALSE;
					}
					if (Rend256D(render.grpgv + (y << 10),
									render.grpbuf[2] + (y << 10),
									render.paldata) != 0) {
						render.mix[raster] = TRUE;
					}
					return;
				}

				// ‚»‚êˆÈŠO‚Ígrpmod‚ğŒ©‚Äˆ—
				if (!render.grpmod[0x400 + y]) {
					return;
				}

				render.grpmod[0x400 + y] = FALSE;
				render.mix[raster] = TRUE;

				// ƒŒƒ“ƒ_ƒŠƒ“ƒO
				Rend256B(render.grpgv + (y << 10),
							render.grpbuf[2] + (y << 10),
							render.grpflag + 0x8000 + (y << 5),
							render.paldata);
			}
			return;

		// ƒ^ƒCƒv3:512x512 –¢’è‹`
		case 3:
		// ƒ^ƒCƒv4:512x512 65536Color
		case 4:
			ASSERT(block == 0);
			// ƒIƒtƒZƒbƒgZo
			y = (raster + render.grpy[0]) & 0x1ff;

			// ƒpƒŒƒbƒg‚Ìê‡‚Í‘S—Ìˆæˆ—
			if (render.grppal[y]) {
				render.grppal[y] = FALSE;
				render.grpmod[y] = FALSE;
				for (i=0; i<32; i++) {
					render.grpflag[(y << 5) + i] = FALSE;
				}
				if (Rend64KB(render.grpgv + (y << 10),
								render.grpbuf[0] + (y << 10),
								render.pal64k,
								render.palptr) != 0) {
					render.mix[raster] = TRUE;
				}
				return;
			}

			// ‚»‚êˆÈŠO‚Ígrpmod‚ğŒ©‚Äˆ—
			if (!render.grpmod[y]) {
				return;
			}
			render.grpmod[y] = FALSE;
			render.mix[raster] = TRUE;
			Rend64KA(render.grpgv + (y << 10),
						render.grpbuf[0] + (y << 10),
						render.grpflag + (y << 5),
						render.pal64k,
						render.palptr);
			return;
	}
}

//===========================================================================
//
//	ƒŒƒ“ƒ_ƒ‰(BGEƒXƒvƒ‰ƒCƒg•”)
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	ƒXƒvƒ‰ƒCƒgƒŒƒWƒXƒ^ƒŠƒZƒbƒg
//
//---------------------------------------------------------------------------
void FASTCALL Render::SpriteReset()
{
	DWORD addr;
	WORD data;

	// ƒXƒvƒ‰ƒCƒgƒŒƒWƒXƒ^İ’è
	for (addr=0; addr<0x400; addr+=2) {
		data = *(WORD*)(&render.sprmem[addr]);
		SpriteReg(addr, data);
	}
}

//---------------------------------------------------------------------------
//
//	ƒXƒvƒ‰ƒCƒgƒŒƒWƒXƒ^•ÏX
//
//---------------------------------------------------------------------------
void FASTCALL Render::SpriteReg(DWORD addr, DWORD data)
{
	BOOL use;
	DWORD reg[4];
	DWORD *next;
	DWORD **ptr;
	int index;
	int i;
	int j;
	int offset;
	DWORD pcgno;

	ASSERT(this);
	ASSERT(addr < 0x400);
	ASSERT((addr & 1) == 0);

	// ƒCƒ“ƒfƒNƒVƒ“ƒO‚Æƒf[ƒ^§ŒÀ
	index = (int)(addr >> 3);
	switch ((addr & 7) >> 1) {
		// X,Y(0`1023)
		case 0:
		case 1:
			data &= 0x3ff;
			break;
		// V,H,PAL,PCG
		case 2:
			data &= 0xcfff;
			break;
		// PRW(0,1,2,3)
		case 3:
			data &= 0x0003;
			break;
	}

	// ptrİ’è(&spptr[index << 9])
	ptr = &render.spptr[index << 9];

	// ƒŒƒWƒXƒ^‚ÌƒoƒbƒNƒAƒbƒv
	next = &render.spreg[index << 2];
	reg[0] = next[0];
	reg[1] = next[1];
	reg[2] = next[2];
	reg[3] = next[3];

	// ƒŒƒWƒXƒ^‚Ö‘‚«‚İ
	render.spreg[addr >> 1] = data;

	// ¡Œã—LŒø‚É‚È‚é‚©ƒ`ƒFƒbƒN
	use = TRUE;
	if (next[0] == 0) {
		use = FALSE;
	}
	if (next[0] >= (DWORD)(render.mixlen + 16)) {
		use = FALSE;
	}
	if (next[1] == 0) {
		use = FALSE;
	}
	if (next[1] >= (512 + 16)) {
		use = FALSE;
	}
	if (next[3] == 0) {
		use = FALSE;
	}

	// ‚¢‚Ü‚Ü‚Å–³Œø‚ÅA‚±‚ê‚©‚ç‚à–³Œø‚È‚ç‰½‚à‚µ‚È‚¢
	if (!render.spuse[index]) {
		if (!use) {
			return;
		}
	}

	// ‚¢‚Ü‚Ü‚Å—LŒø‚È‚Ì‚ÅAˆê“x‚Æ‚ß‚é
	if (render.spuse[index]) {
		// –³Œøˆ—(PCG)
		pcgno = reg[2] & 0xfff;
		ASSERT(render.pcguse[ pcgno ] > 0);
		render.pcguse[ pcgno ]--;
		pcgno >>= 8;
		ASSERT(render.pcgpal[ pcgno ] > 0);
		render.pcgpal[ pcgno ]--;

		// –³Œøˆ—(ƒ|ƒCƒ“ƒ^)
		for (i=0; i<16; i++) {
			j = (int)(reg[1] - 16 + i);
			if ((j >= 0) && (j < 512)) {
				ptr[j] = NULL;
				render.bgspmod[j] = TRUE;
			}
		}

		// ¡Œã–³Œø‚È‚çA‚±‚±‚ÅI—¹
		if (!use) {
			render.spuse[index] = FALSE;
			return;
		}
	}

	// “o˜^ˆ—(g—pƒtƒ‰ƒO)
	render.spuse[index] = TRUE;

	// “o˜^ˆ—(PCG)
	pcgno = next[2] & 0xfff;
	render.pcguse[ pcgno ]++;
	offset = pcgno << 8;
	pcgno >>= 8;
	render.pcgpal[ pcgno ]++;

	// PCGƒAƒhƒŒƒX‚ğŒvZAƒ|ƒCƒ“ƒ^ƒZƒbƒg
	if (next[2] & 0x8000) {
		// V”½“]
		offset += 0xf0;
		for (i=0; i<16; i++) {
			j = (int)(next[1] - 16 + i);
			if ((j >= 0) && (j < 512)) {
				ptr[j] = &render.pcgbuf[offset];
				render.bgspmod[j] = TRUE;
			}
			offset -= 16;
		}
	}
	else {
		// ƒm[ƒ}ƒ‹
		for (i=0; i<16; i++) {
			j = (int)(next[1] - 16 + i);
			if ((j >= 0) && (j < 512)) {
				ptr[j] = &render.pcgbuf[offset];
				render.bgspmod[j] = TRUE;
			}
			offset += 16;
		}
	}
}

//---------------------------------------------------------------------------
//
//	BGƒXƒNƒ[ƒ‹•ÏX
//
//---------------------------------------------------------------------------
void FASTCALL Render::BGScrl(int page, DWORD x, DWORD y)
{
	BOOL flag;
	int i;

	ASSERT((page == 0) || (page == 1));
	ASSERT(x < 1024);
	ASSERT(y < 1024);

	// ”äŠrAˆê’v‚µ‚Ä‚ê‚Î‰½‚à‚µ‚È‚¢
	if ((render.bgx[page] == x) && (render.bgy[page] == y)) {
		return;
	}

	// XV
	render.bgx[page] = x;
	render.bgy[page] = y;

	// 768~512‚È‚ç–³ˆÓ–¡
	if (!render.bgspflag) {
		return;
	}

	// •\¦’†‚È‚çABGSPMOD‚ğã‚°‚é
	flag = FALSE;
	if (render.bgdisp[0]) {
		flag = TRUE;
	}
	if (render.bgdisp[1] && !render.bgsize) {
		flag = TRUE;
	}
	if (flag) {
		for (i=0; i<512; i++) {
			render.bgspmod[i] = TRUE;
		}
	}
}

//---------------------------------------------------------------------------
//
//	BGƒRƒ“ƒgƒ[ƒ‹•ÏX
//
//---------------------------------------------------------------------------
void FASTCALL Render::BGCtrl(int index, BOOL flag)
{
	int i;
	int j;
	BOOL areaflag[2];
	DWORD *reg;
	WORD *area;
	DWORD pcgno;
	DWORD low;
	DWORD mid;
	DWORD high;

	// ƒtƒ‰ƒOOFF
	areaflag[0] = FALSE;
	areaflag[1] = FALSE;

	// ƒ^ƒCƒv•Ê
	switch (index) {
		// BG0 •\¦ƒtƒ‰ƒO
		case 0:
			if (render.bgdisp[0] == flag) {
				return;
			}
			render.bgdisp[0] = flag;
			break;

		// BG1 •\¦ƒtƒ‰ƒO
		case 1:
			if (render.bgdisp[1] == flag) {
				return;
			}
			render.bgdisp[1] = flag;
			break;

		// BG0 ƒGƒŠƒA•ÏX
		case 2:
			if (render.bgarea[0] == flag) {
				return;
			}
			render.bgarea[0] = flag;
			areaflag[0] = TRUE;
			break;

		// BG1 ƒGƒŠƒA•ÏX
		case 3:
			if (render.bgarea[1] == flag) {
				return;
			}
			render.bgarea[1] = flag;
			areaflag[1] = TRUE;
			break;

		// BGƒTƒCƒY•ÏX
		case 4:
			if (render.bgsize == flag) {
				return;
			}
			render.bgsize = flag;
			areaflag[0] = TRUE;
			areaflag[1] = TRUE;
			break;

		// ‚»‚Ì‘¼(‚ ‚è‚¦‚È‚¢)
		default:
			ASSERT(FALSE);
			return;
	}

	// ƒtƒ‰ƒOˆ—
	for (i=0; i<2; i++) {
		if (areaflag[i]) {
			// Œ»ó‚Åg‚Á‚Ä‚¢‚érender.pcguse‚ğƒJƒbƒg
			reg = render.bgreg[i];
			for (j=0; j<(64 * 64); j++) {
				pcgno = reg[j];
				if (pcgno & 0x10000) {
					pcgno &= 0xfff;
					ASSERT(render.pcguse[ pcgno ] > 0);
					render.pcguse[ pcgno ]--;
					pcgno = (pcgno >> 8) & 0x0f;
					ASSERT(render.pcgpal[ pcgno ] > 0);
					render.pcgpal[ pcgno ]--;
				}
			}

			// ƒf[ƒ^ƒAƒhƒŒƒX‚ğZo($EBE000,$EBC000)
			area = (WORD*)render.sprmem;
			area += 0x6000;
			if (render.bgarea[i]) {
				area += 0x1000;
			}

			// 64~64ƒ[ƒhƒRƒs[B$10000‚Ìƒrƒbƒg‚Íí‚É0
			if (render.bgsize) {
				// 16x16‚Í‚»‚Ì‚Ü‚Ü
				for (j=0; j<(64*64); j++) {
					render.bgreg[i][j] = (DWORD)area[j];
				}
			}
			else {
				// 8x8‚ÍH•v‚ª•K—vBPCG(0-255)‚ğ>>2‚µAÁ‚¦‚½bit0,1‚ğbit17,18‚Ö
				for (j=0; j<(64*64); j++) {
					low = (DWORD)area[j];
					mid = low;
					high = low;
					low >>= 2;
					low &= (64 - 1);
					mid &= 0xff00;
					high <<= 17;
					high &= 0x60000;
					render.bgreg[i][j] = (DWORD)(low | mid | high);
				}
			}

			// bgall‚ÌƒZƒbƒg
			for (j=0; j<64; j++) {
				render.bgall[i][j] = TRUE;
			}
		}
	}

	// ‚Ç‚Ì•ÏX‚Å‚àA768~512ˆÈŠO‚È‚çbgspmod‚ğã‚°‚é
	if (render.bgspflag) {
		for (i=0; i<512; i++) {
			render.bgspmod[i] = TRUE;
		}
	}
}

//---------------------------------------------------------------------------
//
//	BGƒƒ‚ƒŠ•ÏX
//
//---------------------------------------------------------------------------
void FASTCALL Render::BGMem(DWORD addr, WORD data)
{
	BOOL flag;
	int i;
	int j;
	int index;
	int raster;
	DWORD pcgno;
	DWORD low;
	DWORD mid;
	DWORD high;

	ASSERT((addr >= 0xc000) && (addr < 0x10000));

	// ƒy[ƒWƒ‹[ƒv
	for (i=0; i<2; i++) {
		// ŠY“–ƒy[ƒW‚Ìƒf[ƒ^ƒGƒŠƒA‚Æˆê’v‚µ‚Ä‚¢‚é‚©
		flag = FALSE;
		if ((render.bgarea[i] == FALSE) && (addr < 0xe000)) {
			flag = TRUE;
		}
		if ((render.bgarea[i] == TRUE) && (addr >= 0xe000)) {
			flag = TRUE;
		}
		if (!flag) {
			continue;
		}

		// ƒCƒ“ƒfƒbƒNƒX(<64x64)AƒŒƒWƒXƒ^ƒ|ƒCƒ“ƒ^æ“¾
		index = (int)(addr & 0x1fff);
		index >>= 1;
		ASSERT((index >= 0) && (index < 64*64));
		pcgno = render.bgreg[i][index];

		// ˆÈ‘O‚Ìpcguse‚ğÁ‚·
		if (pcgno & 0x10000) {
			pcgno &= 0xfff;
			ASSERT(render.pcguse[ pcgno ] > 0);
			render.pcguse[ pcgno ]--;
			pcgno = (pcgno >> 8) & 0x0f;
			ASSERT(render.pcgpal[ pcgno ] > 0);
			render.pcgpal[ pcgno ]--;
		}

		// ƒRƒs[
		if (render.bgsize) {
			// 16x16‚Í‚»‚Ì‚Ü‚Ü
			render.bgreg[i][index] = (DWORD)data;
		}
		else {
			// 8x8‚ÍH•v‚ª•K—vBPCG(0-255)‚ğ>>2‚µAÁ‚¦‚½bit0,1‚ğbit17,18‚Ö
			low = (DWORD)data;
			mid = low;
			high = low;
			low >>= 2;
			low &= (64 - 1);
			mid &= 0xff00;
			high <<= 17;
			high &= 0x60000;
			render.bgreg[i][index] = (DWORD)(low | mid | high);
		}

		// bgall‚ğã‚°‚é
		render.bgall[i][index >> 6] = TRUE;

		// •\¦’†‚Å‚È‚¯‚ê‚ÎI—¹Bbgsize=1‚Åƒy[ƒW1‚Ìê‡‚àI—¹
		if (!render.bgspflag || !render.bgdisp[i]) {
			continue;
		}
		if (render.bgsize && (i == 1)) {
			continue;
		}

		// ƒXƒNƒ[ƒ‹ˆÊ’u‚©‚çŒvZ‚µAbgspmod‚ğã‚°‚é
		index >>= 6;
		if (render.bgsize) {
			// 16x16
			raster = render.bgy[i] + (index << 4);
			for (j=0; j<16; j++) {
				raster &= (1024 - 1);
				if ((raster >= 0) && (raster < 512)) {
					render.bgspmod[raster] = TRUE;
				}
				raster++;
			}
		}
		else {
			// 8x8
			raster = render.bgy[i] + (index << 3);
			for (j=0; j<16; j++) {
				raster &= (512 - 1);
				render.bgspmod[raster] = TRUE;
				raster++;
			}
		}
	}
}

//---------------------------------------------------------------------------
//
//	PCGƒƒ‚ƒŠ•ÏX
//
//---------------------------------------------------------------------------
void FASTCALL Render::PCGMem(DWORD addr)
{
	int index;
	DWORD count;
	int i;

	ASSERT(this);
	ASSERT(addr >= 0x8000);
	ASSERT(addr < 0x10000);
	ASSERT((addr & 1) == 0);

	// ƒCƒ“ƒfƒbƒNƒX‚ğo‚·
	addr &= 0x7fff;
	index = (int)(addr >> 7);
	ASSERT((index >= 0) && (index < 256));

	// render.pcgready‚ğÁ‚·
	for (i=0; i<16; i++) {
		render.pcgready[index + (i << 8)] = FALSE;
	}

	// render.pcguse‚ª>0‚È‚ç
	count = 0;
	for (i=0; i<16; i++) {
		count += render.pcguse[index + (i << 8)];
	}
	if (count > 0) {
		// d•û‚È‚¢‚Ì‚ÅABG/ƒXƒvƒ‰ƒCƒgÄ‡¬‚ğŒˆ’è
		for (i=0; i<512; i++) {
			render.bgspmod[i] = TRUE;
		}
	}
}

//---------------------------------------------------------------------------
//
//	PCGƒoƒbƒtƒ@æ“¾
//
//---------------------------------------------------------------------------
const DWORD* FASTCALL Render::GetPCGBuf() const
{
	ASSERT(this);
	ASSERT(render.pcgbuf);

	return render.pcgbuf;
}

//---------------------------------------------------------------------------
//
//	BG/ƒXƒvƒ‰ƒCƒgƒoƒbƒtƒ@æ“¾
//
//---------------------------------------------------------------------------
const DWORD* FASTCALL Render::GetBGSpBuf() const
{
	ASSERT(this);
	ASSERT(render.bgspbuf);

	return render.bgspbuf;
}

//---------------------------------------------------------------------------
//
//	BG/ƒXƒvƒ‰ƒCƒg
//
//---------------------------------------------------------------------------
void FASTCALL Render::BGSprite(int raster)
{
	int i;
	DWORD *reg;
	DWORD **ptr;
	DWORD *buf;
	DWORD pcgno;

	// BGƒXƒvƒ‰ƒCƒg‚ğMix‚µ‚È‚¢ƒtƒ‰ƒO‚Ìƒ`ƒFƒbƒN‚ª•K—v‚©B‰º‚ÌASSERTB

	// BG,ƒXƒvƒ‰ƒCƒg‚Æ‚à512‚Ü‚Å‚µ‚©l‚¦‚Ä‚¢‚È‚¢
	if (raster >= 512) return;
//	ASSERT((raster >= 0) && (raster < 512));

	// ‰¡•‚à512‚Ü‚ÅB‚±‚ê‚à‘å‘O’ñ
	if (render.mixlen > 512) return;
//	ASSERT(render.mixlen <= 512);

	// ƒtƒ‰ƒOƒ`ƒFƒbƒNAƒIƒtA‡¬w¦
	if (!render.bgspmod[raster]) {
		return;
	}
	render.bgspmod[raster] = FALSE;
	render.mix[raster] = TRUE;

	// ƒoƒbƒtƒ@ƒNƒŠƒA
	// ‚±‚±‚ÅƒpƒŒƒbƒg$100‚Å–„‚ß‚é(o‚½ƒcƒCLoading)
	buf = &render.bgspbuf[raster << 9];
	RendClrSprite(buf, render.paldata[0x100], render.mixlen);
	if (!sprite->IsDisplay()) {
		// ”ñ•\¦‚È‚ç$80000000‚Ìƒrƒbƒg‚Í—‚Æ‚·(o‚½ƒcƒCF3)
		RendClrSprite(buf, render.paldata[0x100] & 0x00ffffff, render.mixlen);
		return;
	}

	// ˆê”ÔŒã‚ë‚É‚­‚é(PRW=1)ƒXƒvƒ‰ƒCƒg
	reg = &render.spreg[127 << 2];
	ptr = &render.spptr[127 << 9];
	ptr += raster;
	for (i=127; i>=0; i--) {
		if (render.spuse[i]) {
			// g—p’†
			if (reg[3] == 1) {
				// PRW=1
				if (*ptr) {
					// •\¦
					pcgno = reg[2] & 0xfff;
					if (!render.pcgready[pcgno]) {
						ASSERT(render.pcguse[pcgno] > 0);
						render.pcgready[pcgno] = TRUE;
						RendPCGNew(pcgno, render.sprmem, render.pcgbuf, render.paldata);
					}
					if (cmov) {
						RendSpriteC(*ptr, buf, reg[0], reg[2] & 0x4000);
					}
					else {
						RendSprite(*ptr, buf, reg[0], reg[2] & 0x4000);
					}
				}
			}
		}
		// Ÿ‚ÌƒXƒvƒ‰ƒCƒg(SP0‚ª‚à‚Á‚Æ‚àè‘O)
		reg -= 4;
		ptr -= 512;
	}

	// BG1‚ğ•\¦
	if (render.bgdisp[1] && !render.bgsize) {
		BG(1, raster, buf);
	}

	// ’†ŠÔ‚É‚­‚é(PRW=2)ƒXƒvƒ‰ƒCƒg
	reg = &render.spreg[127 << 2];
	ptr = &render.spptr[127 << 9];
	ptr += raster;
	for (i=127; i>=0; i--) {
		if (render.spuse[i]) {
			// g—p’†
			if (reg[3] == 2) {
				// PRW=2
				if (*ptr) {
					// •\¦
					pcgno = reg[2] & 0xfff;
					if (!render.pcgready[pcgno]) {
						ASSERT(render.pcguse[pcgno] > 0);
						render.pcgready[pcgno] = TRUE;
						RendPCGNew(pcgno, render.sprmem, render.pcgbuf, render.paldata);
					}
					if (cmov) {
						RendSpriteC(*ptr, buf, reg[0], reg[2] & 0x4000);
					}
					else {
						RendSprite(*ptr, buf, reg[0], reg[2] & 0x4000);
					}
				}
			}
		}
		// Ÿ‚ÌƒXƒvƒ‰ƒCƒg(SP0‚ª‚à‚Á‚Æ‚àè‘O)
		reg -= 4;
		ptr -= 512;
	}

	// BG0‚ğ•\¦
	if (render.bgdisp[0]) {
		BG(0, raster, buf);
	}

	// è‘O‚É‚­‚é(PRW=3)ƒXƒvƒ‰ƒCƒg
	reg = &render.spreg[127 << 2];
	ptr = &render.spptr[127 << 9];
	ptr += raster;
	for (i=127; i>=0; i--) {
		if (render.spuse[i]) {
			// g—p’†
			if (reg[3] == 3) {
				// PRW=3
				if (*ptr) {
					// •\¦
					pcgno = reg[2] & 0xfff;
					if (!render.pcgready[pcgno]) {
						ASSERT(render.pcguse[pcgno] > 0);
						render.pcgready[pcgno] = TRUE;
						RendPCGNew(pcgno, render.sprmem, render.pcgbuf, render.paldata);
					}
					if (cmov) {
						RendSpriteC(*ptr, buf, reg[0], reg[2] & 0x4000);
					}
					else {
						RendSprite(*ptr, buf, reg[0], reg[2] & 0x4000);
					}
				}
			}
		}
		// Ÿ‚ÌƒXƒvƒ‰ƒCƒg(SP0‚ª‚à‚Á‚Æ‚àè‘O)
		reg -= 4;
		ptr -= 512;
	}
}

//---------------------------------------------------------------------------
//
//	BG
//
//---------------------------------------------------------------------------
void FASTCALL Render::BG(int page, int raster, DWORD *buf)
{
	int x;
	int y;
	DWORD **ptr;
	int len;
	int rest;

	ASSERT((page == 0) || (page == 1));
	ASSERT((raster >= 0) && (raster < 512));
	ASSERT(buf);

	// yƒuƒƒbƒN‚ğŠ„‚èo‚·
	y = render.bgy[page] + raster;
	if (render.bgsize) {
		// 16x16ƒ‚[ƒh
		y &= (1024 - 1);
		y >>= 4;
	}
	else {
		// 8x8ƒ‚[ƒh
		y &= (512 - 1);
		y >>= 3;
	}
	ASSERT((y >= 0) && (y < 64));

	// bgall‚ªTRUE‚È‚çA‚»‚ÌyƒuƒƒbƒN‚Å•ÏXƒf[ƒ^‚ ‚è
	if (render.bgall[page][y]) {
		render.bgall[page][y] = FALSE;
		BGBlock(page, y);
	}

	// •\¦
	ptr = render.bgptr[page];
	if (!render.bgsize) {
		// 8x8‚Ì•\¦
		x = render.bgx[page] & (512 - 1);
		ptr += (((render.bgy[page] + raster) & (512 - 1)) << 7);

		// Š„‚èØ‚ê‚é‚©ƒ`ƒFƒbƒN
		if ((x & 7) == 0) {
			// 8x8AŠ„‚èØ‚ê‚é
			x >>= 3;
			if (cmov) {
				RendBG8C(ptr, buf, x, render.mixlen, render.pcgready,
					render.sprmem, render.pcgbuf, render.paldata);
			}
			else {
				RendBG8(ptr, buf, x, render.mixlen, render.pcgready,
					render.sprmem, render.pcgbuf, render.paldata);
			}
			return;
		}

		// Å‰‚Ì”¼’[ƒuƒƒbƒN‚ğÀs
		rest = 8 - (x & 7);
		ASSERT((rest > 0) && (rest < 8));
		RendBG8P(&ptr[(x & 0xfff8) >> 2], buf, (x & 7), rest, render.pcgready,
				render.sprmem, render.pcgbuf, render.paldata);

		// —]‚è‚ğ’²‚×‚Ä8dot’PˆÊ•ª‚ğˆ—
		len = render.mixlen - rest;
		x += rest;
		x &= (512 - 1);
		ASSERT((x & 7) == 0);
		if (cmov) {
			RendBG8C(ptr, &buf[rest], (x >> 3), (len & 0xfff8), render.pcgready,
				render.sprmem, render.pcgbuf, render.paldata);
		}
		else {
			RendBG8(ptr, &buf[rest], (x >> 3), (len & 0xfff8), render.pcgready,
				render.sprmem, render.pcgbuf, render.paldata);
		}

		// ÅŒã
		if (len & 7) {
			x += (len & 0xfff8);
			x &= (512 - 1);
			RendBG8P(&ptr[x >> 2], &buf[rest + (len & 0xfff8)], 0, (len & 7),
				render.pcgready, render.sprmem, render.pcgbuf, render.paldata);
		}
		return;
	}

	// 16x16‚Ì•\¦
	x = render.bgx[page] & (1024 - 1);
	ptr += (((render.bgy[page] + raster) & (1024 - 1)) << 7);

	// Š„‚èØ‚ê‚é‚©ƒ`ƒFƒbƒN
	if ((x & 15) == 0) {
		// 16x16AŠ„‚èØ‚ê‚é
		x >>= 4;
		if (cmov) {
			RendBG16C(ptr, buf, x, render.mixlen, render.pcgready,
				render.sprmem, render.pcgbuf, render.paldata);
		}
		else {
			RendBG16(ptr, buf, x, render.mixlen, render.pcgready,
				render.sprmem, render.pcgbuf, render.paldata);
		}
		return;
	}

	// Å‰‚Ì”¼’[ƒuƒƒbƒN‚ğÀs
	rest = 16 - (x & 15);
	ASSERT((rest > 0) && (rest < 16));
	RendBG16P(&ptr[(x & 0xfff0) >> 3], buf, (x & 15), rest, render.pcgready,
			render.sprmem, render.pcgbuf, render.paldata);

	// —]‚è‚ğ’²‚×‚Ä16dot’PˆÊ•ª‚ğˆ—
	len = render.mixlen - rest;
	x += rest;
	x &= (1024 - 1);
	ASSERT((x & 15) == 0);
	if (cmov) {
		RendBG16C(ptr, &buf[rest], (x >> 4), (len & 0xfff0), render.pcgready,
			render.sprmem, render.pcgbuf, render.paldata);
	}
	else {
		RendBG16(ptr, &buf[rest], (x >> 4), (len & 0xfff0), render.pcgready,
			render.sprmem, render.pcgbuf, render.paldata);
	}

	// ÅŒã
	if (len & 15) {
		x += (len & 0xfff0);
		x &= (1024 - 1);
		x >>= 4;
		RendBG16P(&ptr[x << 1], &buf[rest + (len & 0xfff0)], 0, (len & 15),
			render.pcgready, render.sprmem, render.pcgbuf, render.paldata);
	}
}

//---------------------------------------------------------------------------
//
//	BG(ƒuƒƒbƒNˆ—)
//
//---------------------------------------------------------------------------
void FASTCALL Render::BGBlock(int page, int y)
{
	int i;
	int j;
	DWORD *reg;
	DWORD **ptr;
	DWORD *pcgbuf;
	DWORD bgdata;
	DWORD pcgno;

	ASSERT((page == 0) || (page == 1));
	ASSERT((y >= 0) && (y < 64));

	// ƒŒƒWƒXƒ^ƒ|ƒCƒ“ƒ^‚ğ“¾‚é
	reg = &render.bgreg[page][y << 6];

	// BGƒ|ƒCƒ“ƒ^‚ğ“¾‚é
	ptr = render.bgptr[page];
	if (render.bgsize) {
		ptr += (y << 11);
	}
	else {
		ptr += (y << 10);
	}

	// ƒ‹[ƒv
	for (i=0; i<64; i++) {
		// æ“¾
		bgdata = reg[i];

		// $10000‚ª—§‚Á‚Ä‚¢‚ê‚ÎOK
		if (bgdata & 0x10000) {
			ptr += 2;
			continue;
		}

		// $10000‚ğOR
		reg[i] |= 0x10000;

		// pcgno‚ğ“¾‚é
		pcgno = bgdata & 0xfff;

		// ƒTƒCƒY•Ê
		if (render.bgsize) {
			// 16x16
			pcgbuf = &render.pcgbuf[ (pcgno << 8) ];
			if (bgdata & 0x8000) {
				// ã‰º”½“]
				pcgbuf += 0xf0;
				for (j=0; j<16; j++) {
					ptr[0] = pcgbuf;
					ptr[1] = (DWORD*)bgdata;
					pcgbuf -= 0x10;
					ptr += 128;
				}
			}
			else {
				// ’Êí
				for (j=0; j<16; j++) {
					ptr[0] = pcgbuf;
					ptr[1] = (DWORD*)bgdata;
					pcgbuf += 0x10;
					ptr += 128;
				}
			}
			ptr -= 2048;
		}
		else {
			// 8x8Bbit17,bit18‚ğl—¶‚·‚é
			pcgbuf = &render.pcgbuf[ (pcgno << 8) ];
			if (bgdata & 0x20000) {
				pcgbuf += 0x80;
			}
			if (bgdata & 0x40000) {
				pcgbuf += 8;
			}

			if (bgdata & 0x8000) {
				// ã‰º”½“]
				pcgbuf += 0x70;
				for (j=0; j<8; j++) {
					ptr[0] = pcgbuf;
					ptr[1] = (DWORD*)bgdata;
					pcgbuf -= 0x10;
					ptr += 128;
				}
			}
			else {
				// ’Êí
				for (j=0; j<8; j++) {
					ptr[0] = pcgbuf;
					ptr[1] = (DWORD*)bgdata;
					pcgbuf += 0x10;
					ptr += 128;
				}
			}
			ptr -= 1024;
		}

		// “o˜^ˆ—(PCG)
		render.pcguse[ pcgno ]++;
		pcgno = (pcgno >> 8) & 0x0f;
		render.pcgpal[ pcgno ]++;

		// ƒ|ƒCƒ“ƒ^‚ği‚ß‚é
		ptr += 2;
	}
}

//===========================================================================
//
//	ƒŒƒ“ƒ_ƒ‰(‡¬•”)
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	‡¬
//
//---------------------------------------------------------------------------
void FASTCALL Render::Mix(int y)
{
	DWORD *p;
	DWORD *q;
	DWORD *r;
	DWORD *ptr[3];
	int offset;
	DWORD buf[1024];

	// ‡¬w¦‚ª–³‚¢ê‡A‡¬ƒoƒbƒtƒ@‚ª–³‚¢ê‡AyƒI[ƒo[‚Ìê‡return
	if ((!render.mix[y]) || (!render.mixbuf)) {
		return;
	}
	if (render.mixheight <= y) {
		return;
	}
	ASSERT(render.mixlen > 0);

#if defined(REND_LOG)
	LOG1(Log::Normal, "‡¬ y=%d", y);
#endif	// REND_LOG

	// ƒtƒ‰ƒOOFFA‡¬ƒoƒbƒtƒ@ƒAƒhƒŒƒX‰Šú‰»
	render.mix[y] = FALSE;
	q = &render.mixbuf[render.mixwidth * y];

	switch (render.mixtype) {
		// ƒ^ƒCƒv0(•\¦‚µ‚È‚¢)
		case 0:
			RendMix00(q, render.drawflag + (y << 6), render.mixlen);
			return;

		// ƒ^ƒCƒv1(1–Ê‚Ì‚İ)
		case 1:
			offset = (*render.mixy[0] + y) & render.mixand[0];
			p = render.mixptr[0];
			ASSERT(p);
			p += (offset << render.mixshift[0]);
			p += (*render.mixx[0] & render.mixand[0]);
			RendMix01(q, p, render.drawflag + (y << 6), render.mixlen);
			return;

		// ƒ^ƒCƒv2(2–ÊAƒJƒ‰[0d‚Ë‡‚í‚¹)
		case 2:
			offset = (*render.mixy[0] + y) & render.mixand[0];
			p = render.mixptr[0];
			ASSERT(p);
			p += (offset << render.mixshift[0]);
			p += (*render.mixx[0] & render.mixand[0]);
			offset = (*render.mixy[1] + y) & render.mixand[1];
			r = render.mixptr[1];
			ASSERT(r);
			r += (offset << render.mixshift[1]);
			r += (*render.mixx[1] & render.mixand[1]);
			if (cmov) {
				RendMix02C(q, p, r, render.drawflag + (y << 6), render.mixlen);
			}
			else {
				RendMix02(q, p, r, render.drawflag + (y << 6), render.mixlen);
			}
			return;

		// ƒ^ƒCƒv3(2–ÊA’Êíd‚Ë‡‚í‚¹)
		case 3:
			offset = (*render.mixy[0] + y) & render.mixand[0];
			p = render.mixptr[0];
			ASSERT(p);
			p += (offset << render.mixshift[0]);
			p += (*render.mixx[0] & render.mixand[0]);
			offset = (*render.mixy[1] + y) & render.mixand[1];
			r = render.mixptr[1];
			ASSERT(r);
			r += (offset << render.mixshift[1]);
			r += (*render.mixx[1] & render.mixand[1]);
			if (cmov) {
				RendMix03C(q, p, r, render.drawflag + (y << 6), render.mixlen);
			}
			else {
				RendMix03(q, p, r, render.drawflag + (y << 6), render.mixlen);
			}
			return;

		// ƒ^ƒCƒv4(ƒOƒ‰ƒtƒBƒbƒN‚Ì‚İ3–Ê or 4–Ê)
		case 4:
			MixGrp(y, buf);
			RendMix01(q, buf, render.drawflag + (y << 6), render.mixlen);
			break;

		// ƒ^ƒCƒv5(ƒOƒ‰ƒtƒBƒbƒN{ƒeƒLƒXƒgAƒeƒLƒXƒg—Dæ’Êíd‚Ë‡‚í‚¹)
		case 5:
			MixGrp(y, buf);
			offset = (*render.mixy[0] + y) & render.mixand[0];
			p = render.mixptr[0];
			ASSERT(p);
			p += (offset << render.mixshift[0]);
			p += (*render.mixx[0] & render.mixand[0]);
			if (cmov) {
				RendMix03C(q, p, buf, render.drawflag + (y << 6), render.mixlen);
			}
			else {
				RendMix03(q, p, buf, render.drawflag + (y << 6), render.mixlen);
			}
			return;

		// ƒ^ƒCƒv6(ƒOƒ‰ƒtƒBƒbƒN{ƒeƒLƒXƒgAƒOƒ‰ƒtƒBƒbƒN—Dæ’Êíd‚Ë‡‚í‚¹)
		case 6:
			MixGrp(y, buf);
			offset = (*render.mixy[0] + y) & render.mixand[0];
			p = render.mixptr[0];
			p += (offset << render.mixshift[0]);
			p += (*render.mixx[0] & render.mixand[0]);
			if (cmov) {
				RendMix03C(q, buf, p, render.drawflag + (y << 6), render.mixlen);
			}
			else {
				RendMix03(q, buf, p, render.drawflag + (y << 6), render.mixlen);
			}
			return;

		// ƒ^ƒCƒv7(ƒeƒLƒXƒg{ƒXƒvƒ‰ƒCƒg{ƒOƒ‰ƒtƒBƒbƒN1–Ê)
		case 7:
			offset = (*render.mixy[0] + y) & render.mixand[0];
			ptr[0] = render.mixptr[0];
			ptr[0] += (offset << render.mixshift[0]);
			ptr[0] += (*render.mixx[0] & render.mixand[0]);
			offset = (*render.mixy[1] + y) & render.mixand[1];
			ptr[1] = render.mixptr[1];
			ptr[1] += (offset << render.mixshift[1]);
			ptr[1] += (*render.mixx[1] & render.mixand[1]);
			offset = (*render.mixy[2] + y) & render.mixand[2];
			ptr[2] = render.mixptr[2];
			ptr[2] += (offset << render.mixshift[2]);
			ptr[2] += (*render.mixx[2] & render.mixand[2]);
			if (cmov) {
				RendMix04C(q, ptr[render.mixmap[0]], ptr[render.mixmap[1]],
					ptr[render.mixmap[2]], render.drawflag + (y << 6), render.mixlen);
			}
			else {
				RendMix04(q, ptr[render.mixmap[0]], ptr[render.mixmap[1]],
					ptr[render.mixmap[2]], render.drawflag + (y << 6), render.mixlen);
			}
			return;

		// ƒ^ƒCƒv8(ƒeƒLƒXƒg+ƒXƒvƒ‰ƒCƒg+ƒOƒ‰ƒtƒBƒbƒN‚Q–ÊˆÈã)
		case 8:
			MixGrp(y, buf);
			offset = (*render.mixy[0] + y) & render.mixand[0];
			ptr[0] = render.mixptr[0];
			ptr[0] += (offset << render.mixshift[0]);
			ptr[0] += (*render.mixx[0] & render.mixand[0]);
			offset = (*render.mixy[1] + y) & render.mixand[1];
			ptr[1] = render.mixptr[1];
			ptr[1] += (offset << render.mixshift[1]);
			ptr[1] += (*render.mixx[1] & render.mixand[1]);
			ptr[2] = buf;
			if (cmov) {
				RendMix04C(q, ptr[render.mixmap[0]], ptr[render.mixmap[1]],
					ptr[render.mixmap[2]], render.drawflag + (y << 6), render.mixlen);
			}
			else {
				RendMix04(q, ptr[render.mixmap[0]], ptr[render.mixmap[1]],                                    	
					ptr[render.mixmap[2]], render.drawflag + (y << 6), render.mixlen);
			}
			return;

		// ‚»‚Ì‘¼
		default:
			ASSERT(FALSE);
			break;
	}
}

//---------------------------------------------------------------------------
//
//	ƒOƒ‰ƒtƒBƒbƒN‡¬
//
//---------------------------------------------------------------------------
void FASTCALL Render::MixGrp(int y, DWORD *buf)
{
	DWORD *p;
	DWORD *q;
	DWORD *r;
	DWORD *s;
	int offset;

	ASSERT(buf);
	ASSERT((y >= 0) && (y < render.mixheight));

	switch (render.mixpage) {
		// ‚È‚µ(‘¼‚Ìrender.mixpage‚Å‹zû)
		case 0:
			ASSERT(FALSE);
			return;

		// 1–Ê(‘¼‚Ìrender.mixpage‚Å‹zû)
		case 1:
			ASSERT(FALSE);
			return;

		// 2–Ê
		case 2:
			offset = (*render.mixy[4] + y) & render.mixand[4];
			p = render.mixptr[4];
			ASSERT(p);
			p += (offset << render.mixshift[4]);
			p += (*render.mixx[4] & render.mixand[4]);
			offset = (*render.mixy[5] + y) & render.mixand[5];
			q = render.mixptr[5];
			ASSERT(q);
			q += (offset << render.mixshift[5]);
			q += (*render.mixx[5] & render.mixand[5]);

			if (cmov) {
				RendGrp02C(buf, p, q, render.mixlen);
			}
			else {
				RendGrp02(buf, p, q, render.mixlen);
			}
			break;

		// 3–Ê
		case 3:
			offset = (*render.mixy[4] + y) & render.mixand[4];
			p = render.mixptr[4];
			ASSERT(p);
			p += (offset << render.mixshift[4]);
			p += (*render.mixx[4] & render.mixand[4]);

			offset = (*render.mixy[5] + y) & render.mixand[5];
			q = render.mixptr[5];
			ASSERT(q);
			q += (offset << render.mixshift[5]);
			q += (*render.mixx[5] & render.mixand[5]);

			offset = (*render.mixy[6] + y) & render.mixand[6];
			r = render.mixptr[6];
			ASSERT(r);
			r += (offset << render.mixshift[6]);
			r += (*render.mixx[6] & render.mixand[6]);

			if (cmov) {
				RendGrp03C(buf, p, q, r, render.mixlen);
			}
			else {
				RendGrp03(buf, p, q, r, render.mixlen);
			}
			break;

		// 4–Ê
		case 4:
			offset = (*render.mixy[4] + y) & render.mixand[4];
			p = render.mixptr[4];
			ASSERT(p);
			p += (offset << render.mixshift[4]);
			p += (*render.mixx[4] & render.mixand[4]);

			offset = (*render.mixy[5] + y) & render.mixand[5];
			q = render.mixptr[5];
			ASSERT(q);
			q += (offset << render.mixshift[5]);
			q += (*render.mixx[5] & render.mixand[5]);

			offset = (*render.mixy[6] + y) & render.mixand[6];
			r = render.mixptr[6];
			ASSERT(r);
			r += (offset << render.mixshift[6]);
			r += (*render.mixx[6] & render.mixand[6]);

			offset = (*render.mixy[7] + y) & render.mixand[7];
			s = render.mixptr[7];
			ASSERT(s);
			s += (offset << render.mixshift[7]);
			s += (*render.mixx[7] & render.mixand[7]);

			RendGrp04(buf, p, q, r, s, render.mixlen);
			return;

		// ‚»‚Ì‘¼
		default:
			ASSERT(FALSE);
			break;
	}
}

//---------------------------------------------------------------------------
//
//	‡¬ƒoƒbƒtƒ@æ“¾
//
//---------------------------------------------------------------------------
const DWORD* FASTCALL Render::GetMixBuf() const
{
	ASSERT(this);

	// NULL‚Ìê‡‚à‚ ‚è
	return render.mixbuf;
}

//---------------------------------------------------------------------------
//
//	ƒŒƒ“ƒ_ƒŠƒ“ƒO
//
//---------------------------------------------------------------------------
void FASTCALL Render::Process()
{
	int i;

	// s‚«‚·‚¬‚Ä‚¢‚éê‡‚Í•s—v
	if (render.first >= render.last) {
		return;
	}

	// VC
	if (render.vc) {
#if defined(REND_LOG)
		LOG0(Log::Normal, "ƒrƒfƒIˆ—");
#endif	// RENDER_LOG
		Video();
	}

	// ƒRƒ“ƒgƒ‰ƒXƒg
	if (render.contrast) {
#if defined(REND_LOG)
		LOG0(Log::Normal, "ƒRƒ“ƒgƒ‰ƒXƒgˆ—");
#endif	// RENDER_LOG
		Contrast();
	}

	// ƒpƒŒƒbƒg
	if (render.palette) {
#if defined(REND_LOG)
		LOG0(Log::Normal, "ƒpƒŒƒbƒgˆ—");
#endif	// RENDER_LOG
		Palette();
	}

	// first==0‚ÍAƒXƒvƒ‰ƒCƒg‚Ì•\¦ON/OFF‚ğŒŸ¸
	if (render.first == 0) {
		if (sprite->IsDisplay()) {
			if (!render.bgspdisp) {
				// ƒXƒvƒ‰ƒCƒgCPU¨Video
				for (i=0; i<512; i++) {
					render.bgspmod[i] = TRUE;
				}
				render.bgspdisp = TRUE;
			}
		}
		else {
			if (render.bgspdisp) {
				// ƒXƒvƒ‰ƒCƒgVideo¨CPU
				for (i=0; i<512; i++) {
					render.bgspmod[i] = TRUE;
				}
				render.bgspdisp = FALSE;
			}
		}
	}

	// ‚’¼x2‚Ìê‡
	if ((render.v_mul == 2) && !render.lowres) {
		// I/O‘¤‚ÅŠg‘å‚·‚é‚½‚ßAc•ûŒü‚Í”¼•ª‚µ‚©ì‚ç‚È‚¢
		for (i=render.first; i<render.last; i++) {
			if ((i & 1) == 0) {
				Text(i >> 1);
				Grp(0, i >> 1);
				Grp(1, i >> 1);
				Grp(2, i >> 1);
				Grp(3, i >> 1);
				BGSprite(i >> 1);
				Mix(i >> 1);
			}
		}
		// XV
		render.first = render.last;
		return;
	}

	// ƒCƒ“ƒ^ƒŒ[ƒX‚Ìê‡
	if ((render.v_mul == 0) && render.lowres) {
		// ‹ô”EŠï”‚ğ“¯‚Éì‚é(À‹@‚Æ‚ÍˆÙ‚È‚é)
		for (i=render.first; i<render.last; i++) {
			// ƒeƒLƒXƒgEƒOƒ‰ƒtƒBƒbƒN
			Text((i << 1) + 0);
			Text((i << 1) + 1);
			Grp(0, (i << 1) + 0);
			Grp(0, (i << 1) + 1);
			Grp(1, (i << 1) + 0);
			Grp(1, (i << 1) + 1);
			Grp(2, (i << 1) + 0);
			Grp(2, (i << 1) + 1);
			Grp(3, (i << 1) + 0);
			Grp(3, (i << 1) + 1);
			BGSprite((i << 1) + 0);
			BGSprite((i << 1) + 1);
			Mix((i << 1) + 0);
			Mix((i << 1) + 1);
		}
		// XV
		render.first = render.last;
		return;
	}

	// ’Êíƒ‹[ƒv
	for (i=render.first; i<render.last; i++) {
		Text(i);
		Grp(0, i);
		Grp(1, i);
		Grp(2, i);
		Grp(3, i);
		BGSprite(i);
		Mix(i);
	}
	// XV
	render.first = render.last;
}

