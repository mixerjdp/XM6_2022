//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 ‚o‚hD(ytanaka@ipc-tokai.or.jp)
//	[ ƒtƒƒbƒs[ƒfƒBƒXƒNƒCƒ[ƒW ]
//
//---------------------------------------------------------------------------

#include "os.h"
#include "xm6.h"
#include "filepath.h"
#include "fileio.h"
#include "fdd.h"
#include "fdi.h"

//===========================================================================
//
//	FDIƒZƒNƒ^
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	ƒRƒ“ƒXƒgƒ‰ƒNƒ^
//
//---------------------------------------------------------------------------
FDISector::FDISector(BOOL mfm, const DWORD *chrn)
{
	int i;

	ASSERT(chrn);

	// CHRN,MFM‹L‰¯
	for (i=0; i<4; i++) {
		sec.chrn[i] = chrn[i];
	}
	sec.mfm = mfm;

	// ‚»‚Ì‘¼‰Šú‰»
	sec.error = FDD_NODATA;
	sec.length = 0;
	sec.gap3 = 0;
	sec.buffer = NULL;
	sec.pos = 0;
	sec.changed = FALSE;
	sec.next = NULL;
}

//---------------------------------------------------------------------------
//
//	ƒfƒXƒgƒ‰ƒNƒ^
//
//---------------------------------------------------------------------------
FDISector::~FDISector()
{
	// ƒƒ‚ƒŠ‰ğ•ú
	if (sec.buffer) {
		delete[] sec.buffer;
		sec.buffer = NULL;
	}
}

//---------------------------------------------------------------------------
//
//	‰Šúƒ[ƒh
//
//---------------------------------------------------------------------------
void FASTCALL FDISector::Load(const BYTE *buf, int len, int gap, int err)
{
	ASSERT(this);
	ASSERT(!sec.buffer);
	ASSERT(sec.length == 0);
	ASSERT(buf);
	ASSERT(len > 0);
	ASSERT(gap > 0);

	// ƒŒƒ“ƒOƒX‚¾‚¯æ‚Éİ’è
	sec.length = len;

	// ƒoƒbƒtƒ@Šm•Û
	try {
		sec.buffer = new BYTE[len];
	}
	catch (...) {
		sec.buffer = NULL;
		sec.length = 0;
	}
	if (!sec.buffer) {
		sec.length = 0;
	}

	// “]‘—
	memcpy(sec.buffer, buf, sec.length);

	// ƒ[ƒNİ’è
	sec.gap3 = gap;
	sec.error = err;
	sec.changed = FALSE;
}

//---------------------------------------------------------------------------
//
//	ƒZƒNƒ^ƒ}ƒbƒ`‚·‚é‚©
//
//---------------------------------------------------------------------------
BOOL FASTCALL FDISector::IsMatch(BOOL mfm, const DWORD *chrn) const
{
	int i;

	ASSERT(this);
	ASSERT(chrn);

	// MFM‚ğ”äŠr
	if (sec.mfm != mfm) {
		return FALSE;
	}

	// CHR‚ğ”äŠr
	for (i=0; i<3; i++) {
		if (chrn[i] != sec.chrn[i]) {
			return FALSE;
		}
	}

	// ˆø”‚ÌN‚ª!=0‚Ìê‡‚Ì‚İAN”äŠr
	if (chrn[3] != 0) {
		if (chrn[3] != sec.chrn[3]) {
			return FALSE;
		}
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	CHRN‚ğæ“¾
//
//---------------------------------------------------------------------------
void FASTCALL FDISector::GetCHRN(DWORD *chrn) const
{
	int i;

	ASSERT(this);
	ASSERT(chrn);

	for (i=0; i<4; i++) {
		chrn[i] = sec.chrn[i];
	}
}

//---------------------------------------------------------------------------
//
//	ƒŠ[ƒh
//
//---------------------------------------------------------------------------
int FASTCALL FDISector::Read(BYTE *buf) const
{
	ASSERT(this);
	ASSERT(buf);

	// ƒZƒNƒ^ƒoƒbƒtƒ@‚ª‚È‚¯‚ê‚Î‰½‚à‚µ‚È‚¢
	if (!sec.buffer) {
		return sec.error;
	}

	// “]‘—{ƒGƒ‰[‚ğ•Ô‚·
	memcpy(buf, sec.buffer, sec.length);
	return sec.error;
}

//---------------------------------------------------------------------------
//
//	ƒ‰ƒCƒg
//
//---------------------------------------------------------------------------
int FASTCALL FDISector::Write(const BYTE *buf, BOOL deleted)
{
	ASSERT(this);
	ASSERT(buf);

	// ƒZƒNƒ^ƒoƒbƒtƒ@‚ª‚È‚¯‚ê‚Î‰½‚à‚µ‚È‚¢
	if (!sec.buffer) {
		return sec.error;
	}

	// ƒGƒ‰[ˆ—‚ğæ‚És‚¤
	sec.error &= ~FDD_DATACRC;
	sec.error &= ~FDD_DDAM;
	if (deleted) {
		sec.error |= FDD_DDAM;
	}

	// ˆê’v‚·‚ê‚Î‰½‚à‚µ‚È‚¢
	if (memcmp(sec.buffer, buf, sec.length) == 0) {
		return sec.error;
	}

	// “]‘—
	memcpy(sec.buffer, buf, sec.length);

	// XVƒtƒ‰ƒO‚ğ‚½‚ÄAƒGƒ‰[‚ğ•Ô‚·
	sec.changed = TRUE;
	return sec.error;
}

//---------------------------------------------------------------------------
//
//	ƒtƒBƒ‹
//
//---------------------------------------------------------------------------
int FASTCALL FDISector::Fill(DWORD d)
{
	int i;
	BOOL changed;

	ASSERT(this);

	// ƒZƒNƒ^ƒoƒbƒtƒ@‚ª‚È‚¯‚ê‚Î‰½‚à‚µ‚È‚¢
	if (!sec.buffer) {
		return sec.error;
	}

	// ”äŠr‚µ‚È‚ª‚ç‘‚«‚İ
	changed = FALSE;
	for (i=0; i<sec.length; i++) {
		if (sec.buffer[i] != (BYTE)d) {
			// 1‰ñ‚Å‚àˆá‚Á‚½‚çAƒtƒBƒ‹‚µ‚Äbreak
			memset(sec.buffer, d, sec.length);
			changed = TRUE;
			break;
		}
	}

	// ‘‚«‚İ‚Å‚Íƒf[ƒ^CRC‚Í”­¶‚µ‚È‚¢‚à‚Ì‚Æ‚·‚é
	sec.error &= ~FDD_DATACRC;

	// XVƒtƒ‰ƒO‚ğ‚½‚ÄAƒGƒ‰[‚ğ•Ô‚·
	if (changed) {
		sec.changed = TRUE;
	}
	return sec.error;
}

//===========================================================================
//
//	FDIƒgƒ‰ƒbƒN
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	ƒRƒ“ƒXƒgƒ‰ƒNƒ^
//
//---------------------------------------------------------------------------
FDITrack::FDITrack(FDIDisk *disk, int track, BOOL hd)
{
	ASSERT(disk);
	ASSERT((track >= 0) && (track <= 163));

	// ƒfƒBƒXƒNAƒgƒ‰ƒbƒN‹L‰¯
	trk.disk = disk;
	trk.track = track;

	// ‚»‚Ì‘¼ƒ[ƒNƒGƒŠƒA
	trk.init = FALSE;
	trk.sectors[0] = 0;
	trk.sectors[1] = 0;
	trk.sectors[2] = 0;
	trk.hd = hd;
	trk.mfm = TRUE;
	trk.first = NULL;
	trk.next = NULL;
}

//---------------------------------------------------------------------------
//
//	ƒfƒXƒgƒ‰ƒNƒ^
//
//---------------------------------------------------------------------------
FDITrack::~FDITrack()
{
	// ƒNƒŠƒA
	ClrSector();
}

//---------------------------------------------------------------------------
//
//	ƒZ[ƒu
//	¦2HD,DIM‚È‚Ç‚ÌƒZƒNƒ^˜A‘±‘‚«‚İƒ^ƒCƒvŒü‚¯
//
//---------------------------------------------------------------------------
BOOL FASTCALL FDITrack::Save(const Filepath& path, DWORD offset)
{
	Fileio fio;
	FDISector *sector;
	BOOL changed;

	ASSERT(this);

	// ‰Šú‰»‚³‚ê‚Ä‚¢‚È‚¯‚ê‚Î‘‚«‚Ş•K—v‚È‚µ
	if (!IsInit()) {
		return TRUE;
	}

	// ƒZƒNƒ^‚ğ‚Ü‚í‚Á‚ÄA‘‚«‚Ü‚ê‚Ä‚¢‚éƒZƒNƒ^‚ª‚ ‚é‚©
	sector = GetFirst();
	changed = FALSE;
	while (sector) {
		if (sector->IsChanged()) {
			changed = TRUE;
		}
		sector = sector->GetNext();
	}

	// ‚Ç‚ê‚à‘‚«‚Ü‚ê‚Ä‚¢‚È‚¯‚ê‚Î‰½‚à‚µ‚È‚¢
	if (!changed) {
		return TRUE;
	}

	// ƒtƒ@ƒCƒ‹ƒI[ƒvƒ“
	if (!fio.Open(path, Fileio::ReadWrite)) {
		return FALSE;
	}

	// ƒ‹[ƒv
	sector = GetFirst();
	while (sector) {
		// •ÏX‚³‚ê‚Ä‚¢‚È‚¯‚ê‚ÎAŸ‚Ö
		if (!sector->IsChanged()) {
			offset += sector->GetLength();
			sector = sector->GetNext();
			continue;
		}

		// ƒV[ƒN
		if (!fio.Seek(offset)) {
			fio.Close();
			return FALSE;
		}

		// ‘‚«‚İ
		if (!fio.Write(sector->GetSector(), sector->GetLength())) {
			fio.Close();
			return FALSE;
		}

		// ƒtƒ‰ƒO‚ğ—‚Æ‚·
		sector->ClrChanged();

		// Ÿ‚Ö
		offset += sector->GetLength();
		sector = sector->GetNext();
	}

	// I—¹
	fio.Close();
	return TRUE;
}

//---------------------------------------------------------------------------
//
//	ƒZ[ƒu
//	¦2HD,DIM‚È‚Ç‚ÌƒZƒNƒ^˜A‘±‘‚«‚İƒ^ƒCƒvŒü‚¯
//
//---------------------------------------------------------------------------
BOOL FASTCALL FDITrack::Save(Fileio *fio, DWORD offset)
{
	FDISector *sector;
	BOOL changed;

	ASSERT(this);
	ASSERT(fio);

	// ‰Šú‰»‚³‚ê‚Ä‚¢‚È‚¯‚ê‚Î‘‚«‚Ş•K—v‚È‚µ
	if (!IsInit()) {
		return TRUE;
	}

	// ƒZƒNƒ^‚ğ‚Ü‚í‚Á‚ÄA‘‚«‚Ü‚ê‚Ä‚¢‚éƒZƒNƒ^‚ª‚ ‚é‚©
	sector = GetFirst();
	changed = FALSE;
	while (sector) {
		if (sector->IsChanged()) {
			changed = TRUE;
		}
		sector = sector->GetNext();
	}

	// ‚Ç‚ê‚à‘‚«‚Ü‚ê‚Ä‚¢‚È‚¯‚ê‚Î‰½‚à‚µ‚È‚¢
	if (!changed) {
		return TRUE;
	}

	// ƒ‹[ƒv
	sector = GetFirst();
	while (sector) {
		// •ÏX‚³‚ê‚Ä‚¢‚È‚¯‚ê‚ÎAŸ‚Ö
		if (!sector->IsChanged()) {
			offset += sector->GetLength();
			sector = sector->GetNext();
			continue;
		}

		// ƒV[ƒN
		if (!fio->Seek(offset)) {
			return FALSE;
		}

		// ‘‚«‚İ
		if (!fio->Write(sector->GetSector(), sector->GetLength())) {
			return FALSE;
		}

		// ƒtƒ‰ƒO‚ğ—‚Æ‚·
		sector->ClrChanged();

		// Ÿ‚Ö
		offset += sector->GetLength();
		sector = sector->GetNext();
	}

	// I—¹
	return TRUE;
}

//---------------------------------------------------------------------------
//
//	•¨—ƒtƒH[ƒ}ƒbƒg
//	¦ƒZ[ƒu‚Í•Ê‚És‚¤‚±‚Æ
//
//---------------------------------------------------------------------------
void FASTCALL FDITrack::Create(DWORD phyfmt)
{
	ASSERT(this);

	// ‚·‚×‚Ä‚ÌƒZƒNƒ^‚ğíœ
	ClrSector();

	// •¨—ƒtƒH[ƒ}ƒbƒg•Ê
	switch (phyfmt) {
		// •W€2HD
		case FDI_2HD:
			Create2HD(153);
			break;

		// ƒI[ƒoƒgƒ‰ƒbƒN2HD
		case FDI_2HDA:
			Create2HD(159);
			break;

		// 2HS
		case FDI_2HS:
			Create2HS();
			break;

		// 2HC
		case FDI_2HC:
			Create2HC();
			break;

		// 2HDE
		case FDI_2HDE:
			Create2HDE();
			break;

		// 2HQ
		case FDI_2HQ:
			Create2HQ();
			break;

		// N88-BASIC
		case FDI_N88B:
			CreateN88B();
			break;

		// OS-9/68000
		case FDI_OS9:
			CreateOS9();
			break;

		// 2DD
		case FDI_2DD:
			Create2DD();
			break;

		// ‚»‚Ì‘¼
		default:
			ASSERT(FALSE);
			return;
	}

	// ƒZƒNƒ^‚ª‚ ‚é‚È‚çAŒã‚Å•K‚¸ƒZ[ƒu‚³‚¹‚é‚½‚ß‚ÉA‰Šú‰»•‘S•ÏXó‘Ô‚Æ‚·‚é
	if (GetAllSectors() > 0) {
		trk.init = TRUE;
		ForceChanged();
	}
}

//---------------------------------------------------------------------------
//
//	•¨—ƒtƒH[ƒ}ƒbƒg(2HD, 2HDA)
//
//---------------------------------------------------------------------------
void FASTCALL FDITrack::Create2HD(int lim)
{
	int i;
	FDISector *sector;
	DWORD chrn[4];
	BYTE buf[0x400];

	ASSERT(this);
	ASSERT(trk.hd);

	// ƒgƒ‰ƒbƒN‚Íw’è”‚Ü‚Å
	if (trk.track > lim) {
		return;
	}

	// C,H,Nì¬
	chrn[0] = trk.track >> 1;
	chrn[1] = trk.track & 0x01;
	chrn[3] = 0x03;

	// ƒoƒbƒtƒ@‰Šú‰»
	memset(buf, 0xe5, sizeof(buf));

	// ƒŒƒ“ƒOƒX3~8ƒZƒNƒ^(MFM)
	for (i=0; i<8; i++) {
		// Rì¬
		chrn[2] = i + 1;

		// ƒZƒNƒ^ì¬
		sector = new FDISector(TRUE, chrn);

		// ƒf[ƒ^ƒ[ƒh
		sector->Load(buf, 0x400, 0x74, FDD_NOERROR);

		// ’Ç‰Á
		AddSector(sector);
	}
}

//---------------------------------------------------------------------------
//
//	•¨—ƒtƒH[ƒ}ƒbƒg(2HS)
//
//---------------------------------------------------------------------------
void FASTCALL FDITrack::Create2HS()
{
	int i;
	FDISector *sector;
	DWORD chrn[4];
	BYTE buf[0x400];

	ASSERT(this);
	ASSERT(trk.hd);

	// ƒgƒ‰ƒbƒN‚Í159‚Ü‚Å
	if (trk.track > 159) {
		return;
	}

	// C,H,Nì¬
	chrn[0] = trk.track >> 1;
	chrn[1] = trk.track & 0x01;
	chrn[3] = 0x03;

	// ƒoƒbƒtƒ@‰Šú‰»
	memset(buf, 0xe5, sizeof(buf));

	// ƒŒƒ“ƒOƒX3~9ƒZƒNƒ^(MFM)
	for (i=0; i<9; i++) {
		// Rì¬(“Á—á‚ ‚è)
		if ((trk.track == 0) && (i == 0)) {
			chrn[2] = 0x01;
		}
		else {
			chrn[2] = 10 + i;
		}

		// ƒZƒNƒ^ì¬
		sector = new FDISector(TRUE, chrn);

		// ƒf[ƒ^ƒ[ƒh
		sector->Load(buf, 0x400, 0x39, FDD_NOERROR);

		// ’Ç‰Á
		AddSector(sector);
	}
}

//---------------------------------------------------------------------------
//
//	•¨—ƒtƒH[ƒ}ƒbƒg(2HC)
//
//---------------------------------------------------------------------------
void FASTCALL FDITrack::Create2HC()
{
	int i;
	FDISector *sector;
	DWORD chrn[4];
	BYTE buf[0x200];

	ASSERT(this);
	ASSERT(trk.hd);

	// ƒgƒ‰ƒbƒN‚Í159‚Ü‚Å
	if (trk.track > 159) {
		return;
	}

	// C,H,Nì¬
	chrn[0] = trk.track >> 1;
	chrn[1] = trk.track & 0x01;
	chrn[3] = 0x02;

	// ƒoƒbƒtƒ@‰Šú‰»
	memset(buf, 0xe5, sizeof(buf));

	// ƒŒƒ“ƒOƒX2~15ƒZƒNƒ^(MFM)
	for (i=0; i<15; i++) {
		// Rì¬
		chrn[2] = i + 1;

		// ƒZƒNƒ^ì¬
		sector = new FDISector(TRUE, chrn);

		// ƒf[ƒ^ƒ[ƒh
		sector->Load(buf, 0x200, 0x54, FDD_NOERROR);

		// ’Ç‰Á
		AddSector(sector);
	}
}

//---------------------------------------------------------------------------
//
//	•¨—ƒtƒH[ƒ}ƒbƒg(2HDE)
//
//---------------------------------------------------------------------------
void FASTCALL FDITrack::Create2HDE()
{
	int i;
	FDISector *sector;
	DWORD chrn[4];
	BYTE buf[0x400];

	ASSERT(this);
	ASSERT(trk.hd);

	// ƒgƒ‰ƒbƒN‚Í159‚Ü‚Å
	if (trk.track > 159) {
		return;
	}

	// C,Nì¬
	chrn[0] = trk.track >> 1;
	chrn[3] = 0x03;

	// ƒoƒbƒtƒ@‰Šú‰»
	memset(buf, 0xe5, sizeof(buf));

	// ƒŒƒ“ƒOƒX3~9ƒZƒNƒ^(MFM)
	for (i=0; i<9; i++) {
		// Hì¬(“Á—á‚ ‚è)
		chrn[1] = 0x80 + (trk.track & 1);
		if ((trk.track == 0) && (i == 0)) {
			chrn[1] = 0x00;
		}

		// Rì¬
		chrn[2] = i + 1;

		// ƒZƒNƒ^ì¬
		sector = new FDISector(TRUE, chrn);

		// ƒf[ƒ^ƒ[ƒh
		sector->Load(buf, 0x400, 0x39, FDD_NOERROR);

		// ’Ç‰Á
		AddSector(sector);
	}
}

//---------------------------------------------------------------------------
//
//	•¨—ƒtƒH[ƒ}ƒbƒg(2HQ)
//
//---------------------------------------------------------------------------
void FASTCALL FDITrack::Create2HQ()
{
	int i;
	FDISector *sector;
	DWORD chrn[4];
	BYTE buf[0x200];

	ASSERT(this);
	ASSERT(trk.hd);

	// ƒgƒ‰ƒbƒN‚Í159‚Ü‚Å
	if (trk.track > 159) {
		return;
	}

	// C,H,Nì¬
	chrn[0] = trk.track >> 1;
	chrn[1] = trk.track & 0x01;
	chrn[3] = 0x02;

	// ƒoƒbƒtƒ@‰Šú‰»
	memset(buf, 0xe5, sizeof(buf));

	// ƒŒƒ“ƒOƒX2~18ƒZƒNƒ^(MFM)
	for (i=0; i<18; i++) {
		// Rì¬
		chrn[2] = i + 1;

		// ƒZƒNƒ^ì¬
		sector = new FDISector(TRUE, chrn);

		// ƒf[ƒ^ƒ[ƒh
		sector->Load(buf, 0x200, 0x54, FDD_NOERROR);

		// ’Ç‰Á
		AddSector(sector);
	}
}

//---------------------------------------------------------------------------
//
//	•¨—ƒtƒH[ƒ}ƒbƒg(N88-BASIC)
//
//---------------------------------------------------------------------------
void FASTCALL FDITrack::CreateN88B()
{
	int i;
	FDISector *sector;
	DWORD chrn[4];
	BYTE buf[0x100];

	ASSERT(this);
	ASSERT(trk.hd);

	// ƒgƒ‰ƒbƒN‚Í153‚Ü‚Å
	if (trk.track > 153) {
		return;
	}

	// C,Hì¬
	chrn[0] = trk.track >> 1;
	chrn[1] = trk.track & 0x01;

	// ƒoƒbƒtƒ@‰Šú‰»
	memset(buf, 0xe5, sizeof(buf));

	// ƒgƒ‰ƒbƒN0‚Í—áŠO
	if (trk.track == 0) {
		// ƒŒƒ“ƒOƒX0~26ƒZƒNƒ^(FM)
		chrn[3] = 0;
		for (i=0; i<26; i++) {
			// Rì¬
			chrn[2] = i + 1;

			// ƒZƒNƒ^ì¬
			sector = new FDISector(FALSE, chrn);

			// ƒf[ƒ^ƒ[ƒh
			sector->Load(buf, 0x80, 0x1a, FDD_NOERROR);

			// ’Ç‰Á
			AddSector(sector);
		}
		return;
	}

	// ƒŒƒ“ƒOƒX1~26ƒZƒNƒ^(MFM)
	chrn[3] = 1;
	for (i=0; i<26; i++) {
		// Rì¬
		chrn[2] = i + 1;

		// ƒZƒNƒ^ì¬
		sector = new FDISector(TRUE, chrn);

		// ƒf[ƒ^ƒ[ƒh
		sector->Load(buf, 0x100, 0x33, FDD_NOERROR);

		// ’Ç‰Á
		AddSector(sector);
	}
}

//---------------------------------------------------------------------------
//
//	•¨—ƒtƒH[ƒ}ƒbƒg(OS-9/68000)
//
//---------------------------------------------------------------------------
void FASTCALL FDITrack::CreateOS9()
{
	int i;
	FDISector *sector;
	DWORD chrn[4];
	BYTE buf[0x100];

	ASSERT(this);
	ASSERT(trk.hd);

	// ƒgƒ‰ƒbƒN‚Í153‚Ü‚Å
	if (trk.track > 153) {
		return;
	}

	// C,H,Nì¬
	chrn[0] = trk.track >> 1;
	chrn[1] = trk.track & 0x01;
	chrn[3] = 1;

	// ƒoƒbƒtƒ@‰Šú‰»
	memset(buf, 0xe5, sizeof(buf));

	// ƒŒƒ“ƒOƒX1~26ƒZƒNƒ^(MFM)
	for (i=0; i<26; i++) {
		// Rì¬
		chrn[2] = i + 1;

		// ƒZƒNƒ^ì¬
		sector = new FDISector(TRUE, chrn);

		// ƒf[ƒ^ƒ[ƒh
		sector->Load(buf, 0x100, 0x33, FDD_NOERROR);

		// ’Ç‰Á
		AddSector(sector);
	}
}

//---------------------------------------------------------------------------
//
//	•¨—ƒtƒH[ƒ}ƒbƒg(2DD)
//
//---------------------------------------------------------------------------
void FASTCALL FDITrack::Create2DD()
{
	int i;
	FDISector *sector;
	DWORD chrn[4];
	BYTE buf[0x200];

	ASSERT(this);
	ASSERT(!trk.hd);

	// ƒgƒ‰ƒbƒN‚Í159‚Ü‚Å
	if (trk.track > 159) {
		return;
	}

	// C,H,Nì¬
	chrn[0] = trk.track >> 1;
	chrn[1] = trk.track & 0x01;
	chrn[3] = 0x02;

	// ƒoƒbƒtƒ@‰Šú‰»
	memset(buf, 0xe5, sizeof(buf));

	// ƒŒƒ“ƒOƒX2~9ƒZƒNƒ^(MFM)
	for (i=0; i<9; i++) {
		// Rì¬
		chrn[2] = i + 1;

		// ƒZƒNƒ^ì¬
		sector = new FDISector(TRUE, chrn);

		// ƒf[ƒ^ƒ[ƒh
		sector->Load(buf, 0x200, 0x54, FDD_NOERROR);

		// ’Ç‰Á
		AddSector(sector);
	}
}

//---------------------------------------------------------------------------
//
//	ƒŠ[ƒhID
//	¦Ÿ‚ÌƒXƒe[ƒ^ƒX‚ğ•Ô‚·(ƒGƒ‰[‚ÍOR‚³‚ê‚é)
//		FDD_NOERROR		ƒGƒ‰[‚È‚µ
//		FDD_MAM			w’è–§“x‚Å‚ÍƒAƒ“ƒtƒH[ƒ}ƒbƒg
//		FDD_NODATA		w’è–§“x‚Å‚ÍƒAƒ“ƒtƒH[ƒ}ƒbƒg‚©A
//						‚Ü‚½‚Í—LŒø‚ÈƒZƒNƒ^‚·‚×‚ÄID CRC
//
//---------------------------------------------------------------------------
int FASTCALL FDITrack::ReadID(DWORD *buf, BOOL mfm)
{
	FDISector *sector;
	DWORD pos;
	int status;
	int num;
	int match;

	ASSERT(this);
	ASSERT(buf);
	ASSERT(trk.disk);

	// ƒXƒe[ƒ^ƒX‰Šú‰»
	status = FDD_NOERROR;

	// HDƒtƒ‰ƒO‚ª‡‚í‚È‚¯‚ê‚ÎAƒGƒ‰[ˆ—‚·‚é
	if (GetDisk()->IsHD() != trk.hd) {
		status |= FDD_MAM;
		status |= FDD_NODATA;
		pos = GetDisk()->GetRotationTime();
		pos = (pos * 2) - GetDisk()->GetRotationPos();
		GetDisk()->SetSearch(pos);
		return status;
	}

	// –§“x‚ª‡‚¢AID CRC‚Ì‚È‚¢ƒZƒNƒ^‚ÌŒÂ”‚ğ”‚¦‚é
	num = 0;
	match = 0;
	sector = GetFirst();
	while (sector) {
		// –§“x‚ªƒ}ƒbƒ`‚·‚é‚©
		if (sector->IsMFM() == mfm) {
			match++;

			// ID CRCƒGƒ‰[‚ª‚È‚¢‚©
			if (!(sector->GetError() & FDD_IDCRC)) {
				num++;
			}
		}
		sector = sector->GetNext();
	}

	// –§“x‚ªƒ}ƒbƒ`‚·‚éƒf[ƒ^‚ª‚È‚¢
	if (match == 0) {
		status |= FDD_MAM;
	}

	// ID CRC‚Ì‚È‚¢ƒZƒNƒ^‚ª‚È‚¢
	if (num == 0) {
		status |= FDD_NODATA;
	}

	// ‚Ç‚¿‚ç‚Å‚à¸”sBŒŸõ‚É‚©‚©‚éŠÔ‚ÍƒCƒ“ƒfƒbƒNƒXƒz[ƒ‹‚Q‰ñŒŸo
	if (status != FDD_NOERROR) {
		pos = GetDisk()->GetRotationTime();
		pos = (pos * 2) - GetDisk()->GetRotationPos();
		GetDisk()->SetSearch(pos);
		return status;
	}

	// ƒJƒŒƒ“ƒgƒ|ƒWƒVƒ‡ƒ“ˆÈ~‚ÌÅ‰‚ÌƒZƒNƒ^‚ğæ“¾B‚½‚¾‚µ–§“x‚ª‡‚¤‚±‚Æ
	sector = GetCurSector();
	ASSERT(sector);
	for (;;) {
		if (!sector) {
			sector = GetFirst();
		}
		ASSERT(sector);

		// –§“x‚ªˆê’v‚µ‚È‚¯‚ê‚ÎƒXƒLƒbƒv
		if (sector->IsMFM() != mfm) {
			sector = sector->GetNext();
			continue;
		}

		// ID CRC‚È‚çƒXƒLƒbƒv
		if (sector->GetError() & FDD_IDCRC) {
			sector = sector->GetNext();
			continue;
		}

		// I—¹
		break;
	}

	// CHRN‚ğæ“¾
	sector->GetCHRN(buf);

	// ŒŸõ‚É‚©‚©‚éŠÔ‚ğİ’è
	pos = sector->GetPos();
	GetDisk()->CalcSearch(pos);

	// ƒGƒ‰[–³‚µ
	return status;
}

//---------------------------------------------------------------------------
//
//	ƒŠ[ƒhƒZƒNƒ^
//	¦Ÿ‚ÌƒXƒe[ƒ^ƒX‚ğ•Ô‚·(ƒGƒ‰[‚ÍOR‚³‚ê‚é)
//		FDD_NOERROR		ƒGƒ‰[‚È‚µ
//		FDD_MAM			w’è–§“x‚Å‚ÍƒAƒ“ƒtƒH[ƒ}ƒbƒg
//		FDD_NODATA		w’è–§“x‚Å‚ÍƒAƒ“ƒtƒH[ƒ}ƒbƒg‚©A
//						‚Ü‚½‚Í—LŒø‚ÈƒZƒNƒ^‚ğ‘SŒŸõ‚µ‚ÄR‚ªˆê’v‚µ‚È‚¢
//		FDD_NOCYL		ŒŸõ’†‚ÉID‚ÌC‚ªˆê’v‚¹‚¸AFF‚Å‚È‚¢ƒZƒNƒ^‚ğŒ©‚Â‚¯‚½
//		FDD_BADCYL		ŒŸõ’†‚ÉID‚ÌC‚ªˆê’v‚¹‚¸AFF‚Æ‚È‚Á‚Ä‚¢‚éƒZƒNƒ^‚ğŒ©‚Â‚¯‚½
//		FDD_IDCRC		IDƒtƒB[ƒ‹ƒh‚ÉCRCƒGƒ‰[‚ª‚ ‚é
//		FDD_DATACRC		DATAƒtƒB[ƒ‹ƒh‚ÉCRCƒGƒ‰[‚ª‚ ‚é
//		FDD_DDAM		ƒfƒŠ[ƒeƒbƒhƒZƒNƒ^‚Å‚ ‚é
//
//---------------------------------------------------------------------------
int FASTCALL FDITrack::ReadSector(BYTE *buf, int *len, BOOL mfm, const DWORD *chrn)
{
	FDISector *sector;
	DWORD work[4];
	DWORD pos;
	int status;
	int i;
	int num;

	ASSERT(this);
	ASSERT(len);
	ASSERT(chrn);

	// –§“x‚ª‡‚¤ƒZƒNƒ^”‚ğæ“¾
	if (mfm) {
		num = GetMFMSectors();
	}
	else {
		num = GetFMSectors();
	}

	// HDƒtƒ‰ƒO‚ª‡‚í‚È‚¯‚ê‚ÎA‹­§“I‚ÉƒZƒNƒ^”0‚Æ‚·‚é
	if (GetDisk()->IsHD() != trk.hd) {
		num = 0;
	}

	// 0‚È‚çMAM,NODATA(ƒZƒNƒ^‚Í‚P‚Â‚à‚È‚¢)
	if (num == 0) {
		// ŒŸõ‚É‚©‚©‚éŠÔ‚ÍƒCƒ“ƒfƒbƒNƒXƒz[ƒ‹‚Q‰ñŒŸo
		pos = GetDisk()->GetRotationTime();
		pos = (pos * 2) - GetDisk()->GetRotationPos();
		GetDisk()->SetSearch(pos);
		return FDD_NODATA | FDD_MAM;
	}

	// ƒJƒŒƒ“ƒgƒ|ƒWƒVƒ‡ƒ“ˆÈ~‚ÌÅ‰‚ÌƒZƒNƒ^‚ğæ“¾
	sector = GetCurSector();

	// Number‚¾‚¯ƒ‹[ƒvAƒZƒNƒ^ŒŸõ(R‚µ‚©Œ©‚È‚¢)
	status = FDD_NOERROR;
	num = GetAllSectors();
	for (i=0; i<num; i++) {
		if (!sector) {
			sector = GetFirst();
		}
		ASSERT(sector);

		// –§“x‚ªƒ}ƒbƒ`‚µ‚È‚¯‚ê‚ÎŒJ‚è•Ô‚·
		if (sector->IsMFM() != mfm) {
			sector = sector->GetNext();
			continue;
		}

		// CHRN‚ğæ“¾AC‚ğƒ`ƒFƒbƒN
		sector->GetCHRN(work);
		if (work[0] != chrn[0]) {
			if (work[0] == 0xff) {
				status |= FDD_BADCYL;
			}
			else {
				status |= FDD_NOCYL;
			}
		}

		// Rˆê’v‚Å”²‚¯‚é
		if (work[2] == chrn[2]) {
			break;
		}

		// Ÿ‚ÌƒZƒNƒ^‚Ö
		sector = sector->GetNext();
	}

	// Rˆê’v‚µ‚È‚¯‚ê‚ÎANODATA‚Å•Ô‚·
	if (work[2] != chrn[2]) {
		status |= FDD_NODATA;

		// ŒŸõ‚É‚©‚©‚éŠÔ‚ÍƒCƒ“ƒfƒbƒNƒXƒz[ƒ‹‚Q‰ñŒŸo
		pos = GetDisk()->GetRotationTime();
		pos = (pos * 2) - GetDisk()->GetRotationPos();
		GetDisk()->SetSearch(pos);
		return status;
	}

	// ŒŸõ‚É‚©‚©‚éŠÔ‚ğİ’è
	pos = sector->GetPos();
	GetDisk()->CalcSearch(pos);

	// buf‚ªw’è‚³‚ê‚Ä‚¢‚éê‡‚Ì‚İAƒoƒbƒtƒ@‚Öƒf[ƒ^‚ğ“ü‚ê‚éBNULL‚È‚çƒXƒe[ƒ^ƒX‚Ì‚İ
	*len = sector->GetLength();
	if (buf) {
		status = sector->Read(buf);
	}
	else {
		status = sector->GetError();
	}

	// ƒXƒe[ƒ^ƒX‚ğƒ}ƒXƒN
	status &= (FDD_IDCRC | FDD_DATACRC | FDD_DDAM);
	return status;
}

//---------------------------------------------------------------------------
//
//	ƒ‰ƒCƒgƒZƒNƒ^
//	¦Ÿ‚ÌƒXƒe[ƒ^ƒX‚ğ•Ô‚·(ƒGƒ‰[‚ÍOR‚³‚ê‚é)
//		FDD_NOERROR		ƒGƒ‰[‚È‚µ
//		FDD_MAM			w’è–§“x‚Å‚ÍƒAƒ“ƒtƒH[ƒ}ƒbƒg
//		FDD_NODATA		w’è–§“x‚Å‚ÍƒAƒ“ƒtƒH[ƒ}ƒbƒg‚©A
//						‚Ü‚½‚Í—LŒø‚ÈƒZƒNƒ^‚ğ‘SŒŸõ‚µ‚ÄCHRN‚ªˆê’v‚µ‚È‚¢
//		FDD_NOCYL		ŒŸõ’†‚ÉID‚ÌC‚ªˆê’v‚¹‚¸AFF‚Å‚È‚¢ƒZƒNƒ^‚ğŒ©‚Â‚¯‚½
//		FDD_BADCYL		ŒŸõ’†‚ÉID‚ÌC‚ªˆê’v‚¹‚¸AFF‚Æ‚È‚Á‚Ä‚¢‚éƒZƒNƒ^‚ğŒ©‚Â‚¯‚½
//		FDD_IDCRC		IDƒtƒB[ƒ‹ƒh‚ÉCRCƒGƒ‰[‚ª‚ ‚é
//		FDD_DDAM		ƒfƒŠ[ƒeƒbƒhƒZƒNƒ^‚Å‚ ‚é
//
//---------------------------------------------------------------------------
int FASTCALL FDITrack::WriteSector(const BYTE *buf, int *len, BOOL mfm, const DWORD *chrn, BOOL deleted)
{
	FDISector *sector;
	DWORD work[4];
	DWORD pos;
	int status;
	int i;
	int num;

	ASSERT(this);
	ASSERT(len);
	ASSERT(chrn);

	// –§“x‚ª‡‚¤ƒZƒNƒ^”‚ğæ“¾
	if (mfm) {
		num = GetMFMSectors();
	}
	else {
		num = GetFMSectors();
	}

	// HDƒtƒ‰ƒO‚ª‡‚í‚È‚¯‚ê‚ÎA‹­§“I‚ÉƒZƒNƒ^”0‚Æ‚·‚é
	if (GetDisk()->IsHD() != trk.hd) {
		num = 0;
	}

	// 0‚È‚çMAM,NODATA(ƒZƒNƒ^‚Í‚P‚Â‚à‚È‚¢)
	if (num == 0) {
		// ŒŸõ‚É‚©‚©‚éŠÔ‚ÍƒCƒ“ƒfƒbƒNƒXƒz[ƒ‹‚Q‰ñŒŸo
		pos = GetDisk()->GetRotationTime();
		pos = (pos * 2) - GetDisk()->GetRotationPos();
		GetDisk()->SetSearch(pos);
		return FDD_NODATA | FDD_MAM;
	}

	// ƒJƒŒƒ“ƒgƒ|ƒWƒVƒ‡ƒ“ˆÈ~‚ÌÅ‰‚ÌƒZƒNƒ^‚ğæ“¾
	sector = GetCurSector();

	// Number‚¾‚¯ƒ‹[ƒvAƒZƒNƒ^ŒŸõ(CHRNƒ`ƒFƒbƒN)
	status = FDD_NOERROR;
	num = GetAllSectors();
	for (i=0; i<num; i++) {
		if (!sector) {
			sector = GetFirst();
		}
		ASSERT(sector);

		// –§“x‚ªƒ}ƒbƒ`‚µ‚È‚¯‚ê‚ÎŒJ‚è•Ô‚·
		if (sector->IsMFM() != mfm) {
			sector = sector->GetNext();
			continue;
		}

		// CHRN‚ğæ“¾AC‚ğƒ`ƒFƒbƒN
		sector->GetCHRN(work);
		if (work[0] != chrn[0]) {
			if (work[0] == 0xff) {
				status |= FDD_BADCYL;
			}
			else {
				status |= FDD_NOCYL;
			}
		}

		// CHRNˆê’v‚Å”²‚¯‚é
		if (sector->IsMatch(mfm, chrn)) {
			break;
		}

		// Ÿ‚ÌƒZƒNƒ^‚Ö
		sector = sector->GetNext();
	}

	// ƒZƒNƒ^‚ªŒ©‚Â‚©‚ç‚È‚¯‚ê‚ÎANODATA
	if (i >= num) {
		status |= FDD_NODATA;

		// ŒŸõ‚É‚©‚©‚éŠÔ‚ÍƒCƒ“ƒfƒbƒNƒXƒz[ƒ‹‚Q‰ñŒŸo
		pos = GetDisk()->GetRotationTime();
		pos = (pos * 2) - GetDisk()->GetRotationPos();
		GetDisk()->SetSearch(pos);
		return status;
	}

	// ŒŸõ‚É‚©‚©‚éŠÔ‚ğİ’è
	pos = sector->GetPos();
	GetDisk()->CalcSearch(pos);

	// buf‚ªw’è‚³‚ê‚Ä‚¢‚éê‡‚Ì‚İA‘‚«‚ŞBNULL‚È‚çƒXƒe[ƒ^ƒX‚Ì‚İ
	*len = sector->GetLength();
	if (buf) {
		status = sector->Write(buf, deleted);
	}
	else {
		status = sector->GetError();
	}

	// ƒXƒe[ƒ^ƒX‚ğƒ}ƒXƒN
	status &= (FDD_IDCRC | FDD_DDAM);
	return status;
}

//---------------------------------------------------------------------------
//
//	ƒŠ[ƒhƒ_ƒCƒAƒO
//	¦Ÿ‚ÌƒXƒe[ƒ^ƒX‚ğ•Ô‚·(ƒGƒ‰[‚ÍOR‚³‚ê‚é)
//		FDD_NOERROR		ƒGƒ‰[‚È‚µ
//		FDD_MAM			w’è–§“x‚Å‚ÍƒAƒ“ƒtƒH[ƒ}ƒbƒg
//		FDD_NODATA		w’è–§“x‚Å‚ÍƒAƒ“ƒtƒH[ƒ}ƒbƒg‚©A
//						‚Ü‚½‚Í—LŒø‚ÈƒZƒNƒ^‚ğ‘SŒŸõ‚µ‚ÄR‚ªˆê’v‚µ‚È‚¢
//		FDD_IDCRC		IDƒtƒB[ƒ‹ƒh‚ÉCRCƒGƒ‰[‚ª‚ ‚é
//		FDD_DATACRC		ƒf[ƒ^ƒtƒB[ƒ‹ƒh‚ÉCRCƒGƒ‰[‚ª‚ ‚é
//		FDD_DDAM		ƒfƒŠ[ƒeƒbƒhƒZƒNƒ^‚Å‚ ‚é
//
//---------------------------------------------------------------------------
int FASTCALL FDITrack::ReadDiag(BYTE *buf, int *len, BOOL mfm, const DWORD *chrn)
{
	FDISector *sector;
	DWORD work[4];
	int num;
	int total;
	int start;
	int length;
	int status;
	int error;
	BYTE *ptr;
	DWORD pos;
	BOOL found;

	ASSERT(this);
	ASSERT(len);
	ASSERT(chrn);

	// –§“x‚ª‡‚¤ƒZƒNƒ^”‚ğæ“¾
	if (mfm) {
		num = GetMFMSectors();
	}
	else {
		num = GetFMSectors();
	}

	// HDƒtƒ‰ƒO‚ª‡‚í‚È‚¯‚ê‚ÎA‹­§“I‚ÉƒZƒNƒ^”0‚Æ‚·‚é
	if (GetDisk()->IsHD() != trk.hd) {
		num = 0;
	}

	// 0‚È‚çMAM,NODATA(ƒZƒNƒ^‚Í‚P‚Â‚à‚È‚¢)
	if (num == 0) {
		// ŒŸõ‚É‚©‚©‚éŠÔ‚ÍƒCƒ“ƒfƒbƒNƒXƒz[ƒ‹2‰ñŒŸo
		pos = GetDisk()->GetRotationTime();
		pos = (pos * 2) - GetDisk()->GetRotationPos();
		GetDisk()->SetSearch(pos);
		return FDD_NODATA | FDD_MAM;
	}

	// ƒ[ƒNŠm•Û
	try {
		ptr = new BYTE[0x4000];
	}
	catch (...) {
		return FDD_NODATA | FDD_MAM;
	}
	if (!ptr) {
		return FDD_NODATA | FDD_MAM;
	}

	// ƒŒƒ“ƒOƒX‚ğŒˆ‚ß‚éBÅ‘åN=7(4000h)
	length = chrn[3];
	if (length > 7) {
		length = 7;
	}
	length = 1 << (length + 7);

	// ŒŸõ‚É‚©‚©‚éŠÔ‚Íæ“ªƒZƒNƒ^æ“¾ŠÔ
	sector = GetFirst();
	pos = sector->GetPos();
	GetDisk()->SetSearch(pos);

	// ƒXƒe[ƒ^ƒX‰Šú‰»
	status = FDD_NOERROR;

	// GAP1ì¬
	total = MakeGAP1(ptr, 0);

	// ƒ‹[ƒv
	found = FALSE;
	while (sector) {
		// ƒZƒNƒ^‚ÌMFM‚ªˆê’v‚µ‚È‚¯‚ê‚ÎAcontinue
		if (sector->IsMFM() != mfm) {
			sector = sector->GetNext();
			continue;
		}

		// ƒZƒNƒ^ƒf[ƒ^‚ğì¬
		total = MakeSector(ptr, total, sector);

		// CHRNæ“¾
		sector->GetCHRN(work);

		// R, N‚Æ‚à‚Éˆê’v‚µ‚½ê‡‚Ì‚İAfound
		if (work[2] == chrn[2]) {
			if (work[3] == chrn[3]) {
				found = TRUE;
			}
		}

		// IDCRC, DATACRC, DDAM
		error = sector->GetError();
		error &= (FDD_IDCRC | FDD_DATACRC | FDD_DDAM);
		status |= error;

		// Ÿ‚Ö
		sector = sector->GetNext();
	}

	// Œ‹‰Ê‚ğŒ©‚é
	if (!found) {
		// (ƒOƒ[ƒfƒBƒAŒn)
		status |= (FDD_NODATA | FDD_DATAERR);
	}

	// GAP4ì¬
	total = MakeGAP4(ptr, total);

	// ƒoƒbƒtƒ@‚ª—^‚¦‚ç‚ê‚Ä‚¢‚È‚¯‚ê‚ÎA‚±‚±‚ÅI—¹
	if (!buf) {
		*len = 0;
		delete[] ptr;
		return status;
	}

	// ƒXƒ^[ƒgˆÊ’u‚ğŒˆ‚ß‚é(Å‰‚ÌƒZƒNƒ^‚Ìƒf[ƒ^’¼‘O‚Ü‚ÅƒXƒLƒbƒv)
	if (mfm) {
		start = 60 + GetGAP1();
	}
	else {
		start = 31 + GetGAP1();
	}

	// ‚P‰ñ‚ÅI‚í‚éê‡
	if (length <= (total - start)) {
		memcpy(buf, &ptr[start], length);
		*len = length;
		delete[] ptr;
		return status;
	}

	// Å‰‚Ì1‰ñ‚ğˆ—
	memcpy(buf, &ptr[start], (total - start));
	*len = (total - start);
	length -= (total - start);
	buf += (total - start);

	// Ÿ‚Ìƒ‹[ƒv
	while (length > 0) {
		if (length <= total) {
			// û‚Ü‚é
			memcpy(buf, ptr, length);
			*len += length;
			break;
		}
		// ‘S‚Ä“ü‚ê‚é
		memcpy(buf, ptr, total);
		*len += total;
		length -= total;
		buf += total;
	}

	// ptr‰ğ•ú
	delete[] ptr;

	// I—¹
	return status;
}

//---------------------------------------------------------------------------
//
//	ƒ‰ƒCƒgID
//	¦2HD,DIM‚È‚Ç‚ÌƒZƒNƒ^ŒÅ’èƒ^ƒCƒvŒü‚¯
//	¦Ÿ‚ÌƒXƒe[ƒ^ƒX‚ğ•Ô‚·(ƒGƒ‰[‚ÍOR‚³‚ê‚é)
//		FDD_NOERROR		ƒGƒ‰[‚È‚µ
//		FDD_NOTWRITE	‘‚«‚İ‹Ö~
//
//---------------------------------------------------------------------------
int FASTCALL FDITrack::WriteID(const BYTE *buf, DWORD d, int sc, BOOL mfm, int /*gpl*/)
{
	FDISector *sector;
	DWORD chrn[4];
	int num;
	int i;
	DWORD pos;

	ASSERT(this);
	ASSERT(sc > 0);

	// Œ»İ‚ÌƒZƒNƒ^‚ğæ“¾
	if (IsMFM()) {
		num = GetMFMSectors();
	}
	else {
		num = GetFMSectors();
	}

	// HDƒtƒ‰ƒO‚ª‡‚í‚È‚¯‚ê‚ÎA‹­§“I‚ÉƒZƒNƒ^”0‚Æ‚·‚é
	if (GetDisk()->IsHD() != trk.hd) {
		num = 0;
	}

	// ƒZƒNƒ^”‚ªˆê’v‚µ‚Ä‚¢‚é‚±‚Æ‚ª•K—v
	if (num != sc) {
		return FDD_NOTWRITE;
	}

	// ’P”{¬İ‚Í•s‰Â
	if (GetAllSectors() != num) {
		return FDD_NOTWRITE;
	}

	// ŠÔ‚ğİ’è(index‚Ü‚Å)
	pos = GetDisk()->GetRotationTime();
	pos -= GetDisk()->GetRotationPos();
	GetDisk()->SetSearch(pos);

	// buf‚ª—^‚¦‚ç‚ê‚Ä‚¢‚È‚¯‚ê‚Î‚±‚±‚Ü‚Å
	if (!buf) {
		return FDD_NOERROR;
	}

	// CHRN‚ªd•¡‚È‚­Aˆê’v‚µ‚Ä‚¢‚é‚±‚Æ
	sector = GetFirst();
	while (sector) {
		// buf‚Ì’†‚©‚ç’²‚×‚é
		for (i=0; i<sc; i++) {
			chrn[0] = (DWORD)buf[i * 4 + 0];
			chrn[1] = (DWORD)buf[i * 4 + 1];
			chrn[2] = (DWORD)buf[i * 4 + 2];
			chrn[3] = (DWORD)buf[i * 4 + 3];
			if (sector->IsMatch(mfm, chrn)) {
				break;
			}
		}

		// ˆê’v‚·‚é‚à‚Ì‚ª‚È‚©‚Á‚½
		if (i >= sc) {
			ASSERT(i == sc);
			return FDD_NOTWRITE;
		}

		// Ÿ‚Ö
		sector = sector->GetNext();
	}

	// ‘‚«‚İƒ‹[ƒv(‘SƒZƒNƒ^‚ğ–„‚ß‚é)
	sector = GetFirst();
	while (sector) {
		sector->Fill(d);
		sector = sector->GetNext();
	}

	return FDD_NOERROR;
}

//---------------------------------------------------------------------------
//
//	•ÏXƒ`ƒFƒbƒN
//
//---------------------------------------------------------------------------
BOOL FASTCALL FDITrack::IsChanged() const
{
	BOOL changed;
	FDISector *sector;

	ASSERT(this);

	// ‰Šú‰»
	changed = FALSE;
	sector = GetFirst();

	// OR‚Å
	while (sector) {
		if (sector->IsChanged()) {
			changed = TRUE;
		}
		sector = sector->GetNext();
	}

	return changed;
}

//---------------------------------------------------------------------------
//
//	ƒZƒNƒ^ƒŒƒ“ƒOƒX—İŒvZo
//
//---------------------------------------------------------------------------
DWORD FASTCALL FDITrack::GetTotalLength() const
{
	DWORD total;
	FDISector *sector;

	ASSERT(this);

	// ‰Šú‰»
	total = 0;
	sector = GetFirst();

	// ƒ‹[ƒv
	while (sector) {
		total += sector->GetLength();
		sector = sector->GetNext();
	}

	return total;
}

//---------------------------------------------------------------------------
//
//	‹­§•ÏX
//
//---------------------------------------------------------------------------
void FASTCALL FDITrack::ForceChanged()
{
	FDISector *sector;

	ASSERT(this);

	// ‰Šú‰»
	sector = GetFirst();

	// ƒ‹[ƒv
	while (sector) {
		sector->ForceChanged();
		sector = sector->GetNext();
	}
}

//---------------------------------------------------------------------------
//
//	ƒZƒNƒ^’Ç‰Á
//
//---------------------------------------------------------------------------
void FASTCALL FDITrack::AddSector(FDISector *sector)
{
	FDISector *ptr;

	ASSERT(this);
	ASSERT(sector);

	// ƒZƒNƒ^‚ğ‚Á‚Ä‚¢‚È‚¯‚ê‚ÎA‚»‚Ì‚Ü‚Ü’Ç‰Á
	if (!trk.first) {
		trk.first = sector;
		sector->SetNext(NULL);

		// MFMŒˆ’è
		trk.mfm = sector->IsMFM();
	}
	else {
		// ÅIƒZƒNƒ^‚ğ“¾‚é
		ptr = trk.first;
			while (ptr->GetNext()) {
			ptr = ptr->GetNext();
		}

		// ÅIƒZƒNƒ^‚É’Ç‰Á
		ptr->SetNext(sector);
		sector->SetNext(NULL);
	}

	// ŒÂ”‰ÁZ
	trk.sectors[0]++;
	if (sector->IsMFM()) {
		trk.sectors[1]++;
	}
	else {
		trk.sectors[2]++;
	}
}

//---------------------------------------------------------------------------
//
//	ƒZƒNƒ^‘Síœ
//
//---------------------------------------------------------------------------
void FASTCALL FDITrack::ClrSector()
{
	FDISector *sector;

	ASSERT(this);

	// ƒZƒNƒ^‚ğ‚·‚×‚Äíœ
	while (trk.first) {
		sector = trk.first->GetNext();
		delete trk.first;
		trk.first = sector;
	}

	// ŒÂ”0
	trk.sectors[0] = 0;
	trk.sectors[1] = 0;
	trk.sectors[2] = 0;
}

//---------------------------------------------------------------------------
//
//	ƒZƒNƒ^ŒŸõ
//
//---------------------------------------------------------------------------
FDISector* FASTCALL FDITrack::Search(BOOL mfm, const DWORD *chrn)
{
	FDISector *sector;

	ASSERT(this);
	ASSERT(chrn);

	// Å‰‚ÌƒZƒNƒ^‚ğæ“¾
	sector = GetFirst();

	// ƒ‹[ƒv
	while (sector) {
		// ƒ}ƒbƒ`‚·‚ê‚Î‚»‚ÌƒZƒNƒ^‚ğ•Ô‚·
		if (sector->IsMatch(mfm, chrn)) {
			return sector;
		}

		sector = sector->GetNext();
	}

	// ƒ}ƒbƒ`‚µ‚È‚¢
	return NULL;
}

//---------------------------------------------------------------------------
//
//	GAP1‚Ì’·‚³‚ğæ“¾
//
//---------------------------------------------------------------------------
int FASTCALL FDITrack::GetGAP1() const
{
	ASSERT(this);

	if (IsMFM()) {
		// GAP4a 80ƒoƒCƒgASYNC12ƒoƒCƒgAIAM4ƒoƒCƒgAGAP1 50ƒoƒCƒg
		return 146;
	}
	else {
		// GAP4a 40ƒoƒCƒgASYNC6ƒoƒCƒgAIAM1ƒoƒCƒgAGAP1 26ƒoƒCƒg
		return 73;
	}
}

//---------------------------------------------------------------------------
//
//	ƒg[ƒ^ƒ‹‚Ì’·‚³‚ğæ“¾
//
//---------------------------------------------------------------------------
int FASTCALL FDITrack::GetTotal() const
{
	ASSERT(this);

	// 2DD‚Í•Êˆµ‚¢
	if (!trk.hd) {
		// PC-9801BX4‚Å‚ÌÀ‘ª’l
		if (IsMFM()) {
			return 6034 + GetGAP1() + 60;
		}
		else {
			return 3016 + GetGAP1() + 31;
		}
	}

	// XVI‚Å‚ÌÀ‘ª’l
	if (IsMFM()) {
		return 10193 + GetGAP1() + 60;
	}
	else {
		return 5095 + GetGAP1() + 31;
	}
}

//---------------------------------------------------------------------------
//
//	ŠeƒZƒNƒ^æ“ª‚ÌˆÊ’u‚ğZo
//
//---------------------------------------------------------------------------
void FASTCALL FDITrack::CalcPos()
{
	FDISector *sector;
	DWORD total;
	DWORD prev;
	DWORD hus;
	FDISector *p;

	ASSERT(this);

	// Å‰‚ÌƒZƒNƒ^‚ğƒZƒbƒg
	sector = GetFirst();

	// ƒ‹[ƒv
	while (sector) {
		// GAP1
		prev = GetGAP1();

		// ‘S‚Ä‚ÌƒZƒNƒ^‚ğ‚Ü‚í‚Á‚ÄAƒTƒCƒY‚ğ“¾‚é
		p = GetFirst();
		while (p) {
			if (p == sector) {
				break;
			}

			// ‚Ü‚¾Œ»‚ê‚Ä‚¢‚È‚¯‚ê‚ÎAprev‚ğ‰ÁZ
			prev += GetSize(p);
			p = p->GetNext();
		}

		// IDƒtƒB[ƒ‹ƒh‚ÆSYNC‚Ì•”•ª‚ğ‰Á‚¦‚é
		if (sector->IsMFM()) {
			prev += 60;
		}
		else {
			prev += 31;
		}

		// GAP4‚ğ‰Á‚¦‚½ƒg[ƒ^ƒ‹‚Ì’l‚ğ“¾‚é
		total = GetTotal();

		// prev‚Ætotal‚Ì”ä‚ğZoB‚Pü‚ÅGetDisk()->GetRotationTime()‚É‚È‚é‚æ‚¤‚É
		if (prev >= total) {
			prev = total;
		}
		ASSERT(total <= 0x5000);
		hus = GetDisk()->GetRotationTime();
		prev >>= 1;
		total >>= 1;
		prev *= hus;
		prev /= total;
		if (prev >= hus) {
			prev = hus - 1;
		}

		// Ši”[
		sector->SetPos(prev);

		// Ÿ‚Ö
		sector = sector->GetNext();
	}
}

//---------------------------------------------------------------------------
//
//	ƒZƒNƒ^‚ÌƒTƒCƒY‚ğ“¾‚é
//
//---------------------------------------------------------------------------
DWORD FASTCALL FDITrack::GetSize(FDISector *sector) const
{
	DWORD len;

	ASSERT(this);
	ASSERT(sector);

	// ‚Ü‚¸ƒZƒNƒ^‚ÌÀƒf[ƒ^—Ìˆæ+CRC+GAP3
	len = sector->GetLength();
	len += 2;
	len += sector->GetGAP3();

	if (sector->IsMFM()) {
		// SYNC12ƒoƒCƒg
		len += 12;

		// IDƒAƒhƒŒƒXƒ}[ƒNACHRNACRC2ƒoƒCƒg
		len += 10;

		// GAP2ASYNCAƒf[ƒ^ƒ}[ƒN
		len += 38;
	}
	else {
		// SYNC6ƒoƒCƒg
		len += 6;

		// IDƒAƒhƒŒƒXƒ}[ƒNACHRNACRC2ƒoƒCƒg
		len += 7;

		// GAP2ASYNCAƒf[ƒ^ƒ}[ƒN
		len += 18;
	}

	return len;
}

//---------------------------------------------------------------------------
//
//	ƒJƒŒƒ“ƒgƒ|ƒWƒVƒ‡ƒ“ˆÈ~‚ÌƒZƒNƒ^‚ğæ“¾
//
//---------------------------------------------------------------------------
FDISector* FASTCALL FDITrack::GetCurSector() const
{
	DWORD cur;
	DWORD pos;
	FDISector *sector;

	ASSERT(this);

	// ƒJƒŒƒ“ƒgƒ|ƒWƒVƒ‡ƒ“‚ğ“¾‚é
	cur = GetDisk()->GetRotationPos();

	// æ“ªƒZƒNƒ^‚ğ“¾‚é
	sector = GetFirst();
	if (!sector) {
		return NULL;
	}

	// ƒZƒNƒ^‚ğ“ª‚©‚çŒ©‚ÄAˆÈã‚Å‚ ‚ê‚Îok
	while (sector) {
		pos = sector->GetPos();
		if (pos >= cur) {
			return sector;
		}
		sector = sector->GetNext();
	}

	// ÅIƒZƒNƒ^‚ÌˆÊ’u‚ğ’´‚¦‚é‚Æ‚±‚ë‚ªw’è‚³‚ê‚Ä‚¢‚é‚Ì‚ÅAæ“ª‚É–ß‚·
	return GetFirst();
}

//---------------------------------------------------------------------------
//
//	GAP1ì¬
//
//---------------------------------------------------------------------------
int FASTCALL FDITrack::MakeGAP1(BYTE *buf, int offset) const
{
	ASSERT(this);
	ASSERT(buf);
	ASSERT(offset >= 0);

	// MFM‚©
	if (IsMFM()) {
		ASSERT(GetMFMSectors() > 0);

		// GAP1ì¬
		offset = MakeData(buf, offset, 0x4e, 80);
		offset = MakeData(buf, offset, 0x00, 12);
		offset = MakeData(buf, offset, 0xc2, 3);
		offset = MakeData(buf, offset, 0xfc, 1);
		offset = MakeData(buf, offset, 0x4e, 50);
		return offset;
	}

	// FM
	ASSERT(GetFMSectors() > 0);

	// GAP1ì¬
	offset = MakeData(buf, offset, 0xff, 40);
	offset = MakeData(buf, offset, 0x00, 6);
	offset = MakeData(buf, offset, 0xfc, 1);
	offset = MakeData(buf, offset, 0xff, 26);
	return offset;
}

//---------------------------------------------------------------------------
//
//	ƒZƒNƒ^ƒf[ƒ^ì¬
//
//---------------------------------------------------------------------------
int FASTCALL FDITrack::MakeSector(BYTE *buf, int offset, FDISector *sector) const
{
	DWORD chrn[4];
	int i;
	WORD crc;
	const BYTE *ptr;
	int len;

	ASSERT(this);
	ASSERT(buf);
	ASSERT(offset >= 0);
	ASSERT(sector);

	// CHRNAƒZƒNƒ^ƒf[ƒ^AƒŒƒ“ƒOƒXæ“¾
	sector->GetCHRN(chrn);
	ptr = sector->GetSector();
	len = sector->GetLength();

	// MFM‚©
	if (sector->IsMFM()) {
		// MFM(ID•”)
		offset = MakeData(buf, offset, 0x00, 12);
		offset = MakeData(buf, offset, 0xa1, 3);
		offset = MakeData(buf, offset, 0xfe, 1);
		for (i=0; i<4; i++) {
			buf[offset + i] = (BYTE)chrn[i];
		}
		offset += 4;
		crc = CalcCRC(&buf[offset - 8], 8);
		buf[offset + 0] = (BYTE)(crc >> 8);
		buf[offset + 1] = (BYTE)crc;
		offset += 2;
		offset = MakeData(buf, offset, 0x4e, 22);

		// MFM(ƒf[ƒ^•”)
		offset = MakeData(buf, offset, 0x00, 12);
		offset = MakeData(buf, offset, 0xa1, 3);
		if (sector->GetError() & FDD_DDAM) {
			offset = MakeData(buf, offset, 0xf8, 1);
		}
		else {
			offset = MakeData(buf, offset, 0xfb, 1);
		}
		memcpy(&buf[offset], ptr, len);
		crc = CalcCRC(&buf[offset - 4], len + 4);
		offset += len;
		buf[offset + 0] = (BYTE)(crc >> 8);
		buf[offset + 1] = (BYTE)crc;
		offset += 2;
		offset = MakeData(buf, offset, 0x4e, sector->GetGAP3());
		return offset;
	}

	// FM(ID•”)
	offset = MakeData(buf, offset, 0x00, 6);
	offset = MakeData(buf, offset, 0xfe, 1);
	for (i=0; i<4; i++) {
		buf[offset + i] = (BYTE)chrn[i];
	}
	offset += 4;
	crc = CalcCRC(&buf[offset - 5], 5);
	buf[offset + 0] = (BYTE)(crc >> 8);
	buf[offset + 1] = (BYTE)crc;
	offset += 2;
	offset = MakeData(buf, offset, 0xff, 11);

	// FM(ƒf[ƒ^•”)
	offset = MakeData(buf, offset, 0x00, 6);
	if (sector->GetError() & FDD_DDAM) {
		offset = MakeData(buf, offset, 0xf8, 1);
	}
	else {
		offset = MakeData(buf, offset, 0xfb, 1);
	}
	memcpy(&buf[offset], ptr, len);
	crc = CalcCRC(&buf[offset - 1], len + 1);
	offset += len;
	buf[offset + 0] = (BYTE)(crc >> 8);
	buf[offset + 1] = (BYTE)crc;
	offset += 2;
	offset = MakeData(buf, offset, 0xff, sector->GetGAP3());

	return offset;
}

//---------------------------------------------------------------------------
//
//	GAP4ì¬
//
//---------------------------------------------------------------------------
int FASTCALL FDITrack::MakeGAP4(BYTE *buf, int offset) const
{
	ASSERT(this);
	ASSERT(buf);
	ASSERT(offset >= 0);

	if (IsMFM()) {
		return MakeData(buf, offset, 0x4e, GetTotal() - offset);
	}
	else {
		return MakeData(buf, offset, 0xff, GetTotal() - offset);
	}
}

//---------------------------------------------------------------------------
//
//	CRCZo
//
//---------------------------------------------------------------------------
WORD FASTCALL FDITrack::CalcCRC(BYTE *ptr, int len) const
{
	WORD crc;
	int i;

	ASSERT(this);
	ASSERT(ptr);
	ASSERT(len >= 0);

	// ‰Šú‰»
	crc = 0xffff;

	// ƒ‹[ƒv
	for (i=0; i<len; i++) {
		crc = (WORD)((crc << 8) ^ CRCTable[ (BYTE)(crc >> 8) ^ (BYTE)*ptr++ ]);
	}

	return crc;
}

//---------------------------------------------------------------------------
//
//	CRCZoƒe[ƒuƒ‹
//
//---------------------------------------------------------------------------
const WORD FDITrack::CRCTable[0x100] = {
	0x0000,  0x1021,  0x2042,  0x3063,  0x4084,  0x50a5,  0x60c6,  0x70e7,
	0x8108,  0x9129,  0xa14a,  0xb16b,  0xc18c,  0xd1ad,  0xe1ce,  0xf1ef,
	0x1231,  0x0210,  0x3273,  0x2252,  0x52b5,  0x4294,  0x72f7,  0x62d6,
	0x9339,  0x8318,  0xb37b,  0xa35a,  0xd3bd,  0xc39c,  0xf3ff,  0xe3de,
	0x2462,  0x3443,  0x0420,  0x1401,  0x64e6,  0x74c7,  0x44a4,  0x5485,
	0xa56a,  0xb54b,  0x8528,  0x9509,  0xe5ee,  0xf5cf,  0xc5ac,  0xd58d,
	0x3653,  0x2672,  0x1611,  0x0630,  0x76d7,  0x66f6,  0x5695,  0x46b4,
	0xb75b,  0xa77a,  0x9719,  0x8738,  0xf7df,  0xe7fe,  0xd79d,  0xc7bc,
	0x48c4,  0x58e5,  0x6886,  0x78a7,  0x0840,  0x1861,  0x2802,  0x3823,
	0xc9cc,  0xd9ed,  0xe98e,  0xf9af,  0x8948,  0x9969,  0xa90a,  0xb92b,
	0x5af5,  0x4ad4,  0x7ab7,  0x6a96,  0x1a71,  0x0a50,  0x3a33,  0x2a12,
	0xdbfd,  0xcbdc,  0xfbbf,  0xeb9e,  0x9b79,  0x8b58,  0xbb3b,  0xab1a,
	0x6ca6,  0x7c87,  0x4ce4,  0x5cc5,  0x2c22,  0x3c03,  0x0c60,  0x1c41,
	0xedae,  0xfd8f,  0xcdec,  0xddcd,  0xad2a,  0xbd0b,  0x8d68,  0x9d49,
	0x7e97,  0x6eb6,  0x5ed5,  0x4ef4,  0x3e13,  0x2e32,  0x1e51,  0x0e70,
	0xff9f,  0xefbe,  0xdfdd,  0xcffc,  0xbf1b,  0xaf3a,  0x9f59,  0x8f78,
	0x9188,  0x81a9,  0xb1ca,  0xa1eb,  0xd10c,  0xc12d,  0xf14e,  0xe16f,
	0x1080,  0x00a1,  0x30c2,  0x20e3,  0x5004,  0x4025,  0x7046,  0x6067,
	0x83b9,  0x9398,  0xa3fb,  0xb3da,  0xc33d,  0xd31c,  0xe37f,  0xf35e,
	0x02b1,  0x1290,  0x22f3,  0x32d2,  0x4235,  0x5214,  0x6277,  0x7256,
	0xb5ea,  0xa5cb,  0x95a8,  0x8589,  0xf56e,  0xe54f,  0xd52c,  0xc50d,
	0x34e2,  0x24c3,  0x14a0,  0x0481,  0x7466,  0x6447,  0x5424,  0x4405,
	0xa7db,  0xb7fa,  0x8799,  0x97b8,  0xe75f,  0xf77e,  0xc71d,  0xd73c,
	0x26d3,  0x36f2,  0x0691,  0x16b0,  0x6657,  0x7676,  0x4615,  0x5634,
	0xd94c,  0xc96d,  0xf90e,  0xe92f,  0x99c8,  0x89e9,  0xb98a,  0xa9ab,
	0x5844,  0x4865,  0x7806,  0x6827,  0x18c0,  0x08e1,  0x3882,  0x28a3,
	0xcb7d,  0xdb5c,  0xeb3f,  0xfb1e,  0x8bf9,  0x9bd8,  0xabbb,  0xbb9a,
	0x4a75,  0x5a54,  0x6a37,  0x7a16,  0x0af1,  0x1ad0,  0x2ab3,  0x3a92,
	0xfd2e,  0xed0f,  0xdd6c,  0xcd4d,  0xbdaa,  0xad8b,  0x9de8,  0x8dc9,
	0x7c26,  0x6c07,  0x5c64,  0x4c45,  0x3ca2,  0x2c83,  0x1ce0,  0x0cc1,
	0xef1f,  0xff3e,  0xcf5d,  0xdf7c,  0xaf9b,  0xbfba,  0x8fd9,  0x9ff8,
	0x6e17,  0x7e36,  0x4e55,  0x5e74,  0x2e93,  0x3eb2,  0x0ed1,  0x1ef0
};

//---------------------------------------------------------------------------
//
//	Diagƒf[ƒ^ì¬
//
//---------------------------------------------------------------------------
int FASTCALL FDITrack::MakeData(BYTE *buf, int offset, BYTE data, int length) const
{
	int i;

	ASSERT(this);
	ASSERT(buf);
	ASSERT(offset >= 0);
	ASSERT((length > 0) && (length < 0x400));

	for (i=0; i<length; i++) {
		buf[offset + i] = data;
	}

	return (offset + length);
}

//===========================================================================
//
//	FDIƒfƒBƒXƒN
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	ƒRƒ“ƒXƒgƒ‰ƒNƒ^
//
//---------------------------------------------------------------------------
FDIDisk::FDIDisk(int index, FDI *fdi)
{
	ASSERT((index >= 0) && (index < 0x10));

	// ƒCƒ“ƒfƒbƒNƒXAIDw’è
	disk.index = index;
	disk.fdi = fdi;
	disk.id = MAKEID('I', 'N', 'I', 'T');

	// ó‘Ô
	disk.writep = FALSE;
	disk.readonly = FALSE;

	// –¼Ì‚È‚µ
	disk.name[0] = '\0';
	disk.offset = 0;

	// •Ûƒgƒ‰ƒbƒN‚È‚µ
	disk.first = NULL;
	disk.head[0] = NULL;
	disk.head[1] = NULL;

	// ƒ|ƒWƒVƒ‡ƒ“
	disk.search = 0;

	// ƒŠƒ“ƒN‚È‚µ
	disk.next = NULL;
}

//---------------------------------------------------------------------------
//
//	ƒfƒXƒgƒ‰ƒNƒ^
//	¦”h¶ƒNƒ‰ƒX‚Ì’ˆÓ“_F
//		Œ»İ‚Ìhead[]‚ğƒtƒ@ƒCƒ‹‚É‘‚«–ß‚·
//
//---------------------------------------------------------------------------
FDIDisk::~FDIDisk()
{
	// ƒNƒŠƒA
	ClrTrack();
}

//---------------------------------------------------------------------------
//
//	ì¬
//	¦”h¶ƒNƒ‰ƒX‚Ì’ˆÓ“_F
//		•¨—ƒtƒH[ƒ}ƒbƒg‚ğs‚¤(‰¼‘zŠÖ”‚ÌÅŒã‚ÅA‚±‚±‚ğŒÄ‚Ô‚±‚Æ)
//
//---------------------------------------------------------------------------
BOOL FASTCALL FDIDisk::Create(const Filepath& /*path*/, const option_t *opt)
{
	ASSERT(this);
	ASSERT(opt);

	// ˜_—ƒtƒH[ƒ}ƒbƒg‚ª•K—v‚È‚¯‚ê‚ÎI—¹
	if (!opt->logfmt) {
		return TRUE;
	}

	// •¨—ƒtƒH[ƒ}ƒbƒg•Ê‚ÉA˜_—ƒtƒH[ƒ}ƒbƒg‚ğs‚¤
	switch (opt->phyfmt) {
		// 2HD
		case FDI_2HD:
			Create2HD(TRUE);
			break;

		// 2HDA
		case FDI_2HDA:
			Create2HD(FALSE);
			break;

		// 2HS
		case FDI_2HS:
			Create2HS();
			break;

		// 2HC
		case FDI_2HC:
			Create2HC();
			break;

		// 2HDE
		case FDI_2HDE:
			Create2HDE();
			break;

		// 2HQ
		case FDI_2HQ:
			Create2HQ();
			break;

		// 2DD
		case FDI_2DD:
			Create2DD();
			break;

		// ‚»‚Ì‘¼
		default:
			return FALSE;
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	˜_—ƒtƒH[ƒ}ƒbƒg(2HD,2HDA)
//
//---------------------------------------------------------------------------
void FASTCALL FDIDisk::Create2HD(BOOL flag2hd)
{
	FDITrack *track;
	FDISector *sector;
	BYTE buf[0x400];
	DWORD chrn[4];
	int i;

	ASSERT(this);

	// ’Êí‰Šú‰»
	memset(buf, 0, sizeof(buf));

	// ƒgƒ‰ƒbƒN0ƒZƒNƒ^1`8‚Ö‚·‚×‚Ä‘‚«‚Ş
	track = Search(0);
	ASSERT(track);
	chrn[0] = 0;
	chrn[1] = 0;
	chrn[3] = 3;
	for (i=1; i<=8; i++) {
		chrn[2] = i;
		sector = track->Search(TRUE, chrn);
		ASSERT(sector);
		sector->Write(buf, FALSE);
	}

	// ƒgƒ‰ƒbƒN1ƒZƒNƒ^1`3‚Ö‚·‚×‚Ä‘‚«‚Ş
	track = Search(1);
	ASSERT(track);
	chrn[0] = 0;
	chrn[1] = 1;
	chrn[3] = 3;
	for (i=1; i<=3; i++) {
		chrn[2] = i;
		sector = track->Search(TRUE, chrn);
		ASSERT(sector);
		sector->Write(buf, FALSE);
	}

	// ƒgƒ‰ƒbƒN0‚ÖƒV[ƒN
	track = Search(0);
	ASSERT(track);

	// IPL‘‚«‚İ
	memcpy(buf, IPL2HD, 0x200);
	if (!flag2hd) {
		// 2HDA‚Í˜_—ƒZƒNƒ^”=1280ƒZƒNƒ^
		buf[0x13] = 0x00;
		buf[0x14] = 0x05;
	}
	chrn[0] = 0;
	chrn[1] = 0;
	chrn[2] = 1;
	chrn[3] = 3;
	sector = track->Search(TRUE, chrn);
	ASSERT(sector);
	sector->Write(buf, FALSE);

	// FATæ“ªƒZƒNƒ^‰Šú‰»
	memset(buf, 0, sizeof(buf));
	buf[0] = 0xfe;
	buf[1] = 0xff;
	buf[2] = 0xff;

	// ‘æ1FAT‘‚«‚İ
	chrn[0] = 0;
	chrn[1] = 0;
	chrn[2] = 2;
	chrn[3] = 3;
	sector = track->Search(TRUE, chrn);
	ASSERT(sector);
	sector->Write(buf, FALSE);

	// ‘æ2FAT‘‚«‚İ
	chrn[0] = 0;
	chrn[1] = 0;
	chrn[2] = 4;
	chrn[3] = 3;
	sector = track->Search(TRUE, chrn);
	ASSERT(sector);
	sector->Write(buf, FALSE);
}

//---------------------------------------------------------------------------
//
//	IPL(2HD,2HDA)
//	¦FORMAT.x v2.31‚æ‚èæ“¾‚µ‚½‚à‚Ì
//
//---------------------------------------------------------------------------
const BYTE FDIDisk::IPL2HD[0x200] = {
	0x60,0x3c,0x90,0x58,0x36,0x38,0x49,0x50,
	0x4c,0x33,0x30,0x00,0x04,0x01,0x01,0x00,
	0x02,0xc0,0x00,0xd0,0x04,0xfe,0x02,0x00,
	0x08,0x00,0x02,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x20,0x20,0x20,0x20,0x20,
	0x20,0x20,0x20,0x20,0x20,0x20,0x46,0x41,
	0x54,0x31,0x32,0x20,0x20,0x20,0x4f,0xfa,
	0xff,0xc0,0x4d,0xfa,0x01,0xb8,0x4b,0xfa,
	0x00,0xe0,0x49,0xfa,0x00,0xea,0x43,0xfa,
	0x01,0x20,0x4e,0x94,0x70,0x8e,0x4e,0x4f,
	0x7e,0x70,0xe1,0x48,0x8e,0x40,0x26,0x3a,
	0x01,0x02,0x22,0x4e,0x24,0x3a,0x01,0x00,
	0x32,0x07,0x4e,0x95,0x66,0x28,0x22,0x4e,
	0x32,0x3a,0x00,0xfa,0x20,0x49,0x45,0xfa,
	0x01,0x78,0x70,0x0a,0x00,0x10,0x00,0x20,
	0xb1,0x0a,0x56,0xc8,0xff,0xf8,0x67,0x38,
	0xd2,0xfc,0x00,0x20,0x51,0xc9,0xff,0xe6,
	0x45,0xfa,0x00,0xe0,0x60,0x10,0x45,0xfa,
	0x00,0xfa,0x60,0x0a,0x45,0xfa,0x01,0x10,
	0x60,0x04,0x45,0xfa,0x01,0x28,0x61,0x00,
	0x00,0x94,0x22,0x4a,0x4c,0x99,0x00,0x06,
	0x70,0x23,0x4e,0x4f,0x4e,0x94,0x32,0x07,
	0x70,0x4f,0x4e,0x4f,0x70,0xfe,0x4e,0x4f,
	0x74,0x00,0x34,0x29,0x00,0x1a,0xe1,0x5a,
	0xd4,0x7a,0x00,0xa4,0x84,0xfa,0x00,0x9c,
	0x84,0x7a,0x00,0x94,0xe2,0x0a,0x64,0x04,
	0x08,0xc2,0x00,0x18,0x48,0x42,0x52,0x02,
	0x22,0x4e,0x26,0x3a,0x00,0x7e,0x32,0x07,
	0x4e,0x95,0x34,0x7c,0x68,0x00,0x22,0x4e,
	0x0c,0x59,0x48,0x55,0x66,0xa6,0x54,0x89,
	0xb5,0xd9,0x66,0xa6,0x2f,0x19,0x20,0x59,
	0xd1,0xd9,0x2f,0x08,0x2f,0x11,0x32,0x7c,
	0x67,0xc0,0x76,0x40,0xd6,0x88,0x4e,0x95,
	0x22,0x1f,0x24,0x1f,0x22,0x5f,0x4a,0x80,
	0x66,0x00,0xff,0x7c,0xd5,0xc2,0x53,0x81,
	0x65,0x04,0x42,0x1a,0x60,0xf8,0x4e,0xd1,
	0x70,0x46,0x4e,0x4f,0x08,0x00,0x00,0x1e,
	0x66,0x02,0x70,0x00,0x4e,0x75,0x70,0x21,
	0x4e,0x4f,0x4e,0x75,0x72,0x0f,0x70,0x22,
	0x4e,0x4f,0x72,0x19,0x74,0x0c,0x70,0x23,
	0x4e,0x4f,0x61,0x08,0x72,0x19,0x74,0x0d,
	0x70,0x23,0x4e,0x4f,0x76,0x2c,0x72,0x20,
	0x70,0x20,0x4e,0x4f,0x51,0xcb,0xff,0xf8,
	0x4e,0x75,0x00,0x00,0x04,0x00,0x03,0x00,
	0x00,0x06,0x00,0x08,0x00,0x1f,0x00,0x09,
	0x1a,0x00,0x00,0x22,0x00,0x0d,0x48,0x75,
	0x6d,0x61,0x6e,0x2e,0x73,0x79,0x73,0x20,
	0x82,0xaa,0x20,0x8c,0xa9,0x82,0xc2,0x82,
	0xa9,0x82,0xe8,0x82,0xdc,0x82,0xb9,0x82,
	0xf1,0x00,0x00,0x25,0x00,0x0d,0x83,0x66,
	0x83,0x42,0x83,0x58,0x83,0x4e,0x82,0xaa,
	0x81,0x40,0x93,0xc7,0x82,0xdf,0x82,0xdc,
	0x82,0xb9,0x82,0xf1,0x00,0x00,0x00,0x23,
	0x00,0x0d,0x48,0x75,0x6d,0x61,0x6e,0x2e,
	0x73,0x79,0x73,0x20,0x82,0xaa,0x20,0x89,
	0xf3,0x82,0xea,0x82,0xc4,0x82,0xa2,0x82,
	0xdc,0x82,0xb7,0x00,0x00,0x20,0x00,0x0d,
	0x48,0x75,0x6d,0x61,0x6e,0x2e,0x73,0x79,
	0x73,0x20,0x82,0xcc,0x20,0x83,0x41,0x83,
	0x68,0x83,0x8c,0x83,0x58,0x82,0xaa,0x88,
	0xd9,0x8f,0xed,0x82,0xc5,0x82,0xb7,0x00,
	0x68,0x75,0x6d,0x61,0x6e,0x20,0x20,0x20,
	0x73,0x79,0x73,0x00,0x00,0x00,0x00,0x00
};

//---------------------------------------------------------------------------
//
//	˜_—ƒtƒH[ƒ}ƒbƒg(2HS)
//
//---------------------------------------------------------------------------
void FASTCALL FDIDisk::Create2HS()
{
	FDITrack *track;
	FDISector *sector;
	BYTE buf[0x400];
	DWORD chrn[4];
	int i;

	ASSERT(this);

	// ’Êí‰Šú‰»
	memset(buf, 0, sizeof(buf));

	// ‡Œv10ƒZƒNƒ^‚Ö‘‚«‚Ş(1ƒgƒ‰ƒbƒN‚ ‚½‚è9ƒZƒNƒ^)
	track = Search(0);
	ASSERT(track);
	chrn[0] = 0;
	chrn[1] = 0;
	chrn[3] = 3;
	for (i=11; i<=18; i++) {
		chrn[2] = i;
		sector = track->Search(TRUE, chrn);
		ASSERT(sector);
		sector->Write(buf, FALSE);
	}
	track = Search(1);
	ASSERT(track);
	chrn[0] = 0;
	chrn[1] = 1;
	chrn[2] = 10;
	chrn[3] = 3;
	sector = track->Search(TRUE, chrn);
	ASSERT(sector);
	sector->Write(buf, FALSE);

	// ƒgƒ‰ƒbƒN0‚ÖƒV[ƒN
	track = Search(0);
	ASSERT(track);

	// IPL‘‚«‚İ(1)
	chrn[0] = 0;
	chrn[1] = 0;
	chrn[2] = 1;
	chrn[3] = 3;
	sector = track->Search(TRUE, chrn);
	ASSERT(sector);
	sector->Write(&IPL2HS[0x000], FALSE);

	// IPL‘‚«‚İ(2)
	chrn[0] = 0;
	chrn[1] = 0;
	chrn[2] = 13;
	chrn[3] = 3;
	sector = track->Search(TRUE, chrn);
	ASSERT(sector);
	sector->Write(&IPL2HS[0x400], FALSE);

	// FATæ“ªƒZƒNƒ^‰Šú‰»
	buf[0] = 0xfb;
	buf[1] = 0xff;
	buf[2] = 0xff;

	// FAT‘‚«‚İ
	chrn[0] = 0;
	chrn[1] = 0;
	chrn[2] = 11;
	chrn[3] = 3;
	sector = track->Search(TRUE, chrn);
	ASSERT(sector);
	sector->Write(buf, FALSE);
}

//---------------------------------------------------------------------------
//
//	IPL(2HS)
//	¦9scdrv.x v3.00‚æ‚èæ“¾‚µ‚½‚à‚Ì
//
//---------------------------------------------------------------------------
const BYTE FDIDisk::IPL2HS[0x800] = {
	0x60,0x1e,0x39,0x53,0x43,0x46,0x4d,0x54,
	0x20,0x49,0x50,0x4c,0x20,0x76,0x31,0x2e,
	0x30,0x32,0x04,0x00,0x01,0x03,0x00,0x01,
	0x00,0xc0,0x05,0xa0,0xfb,0x01,0x90,0x70,
	0x60,0x00,0x03,0x5a,0x08,0x01,0x00,0x0c,
	0x66,0x08,0x4d,0xfa,0xff,0xd4,0x2c,0x56,
	0x4e,0xd6,0x61,0x00,0x00,0xba,0x48,0xe7,
	0x4f,0x00,0x61,0x00,0x02,0xf0,0x61,0x00,
	0x00,0xc4,0x08,0x00,0x00,0x1b,0x66,0x4e,
	0xc2,0x3c,0x00,0xc0,0x82,0x3c,0x00,0x06,
	0x61,0x00,0x00,0xd0,0xe1,0x9a,0x54,0x88,
	0x20,0xc2,0xe0,0x9a,0x10,0xc2,0x10,0xc7,
	0x10,0x86,0x61,0x00,0x00,0xf0,0x41,0xf8,
	0x09,0xee,0x70,0x08,0x61,0x00,0x01,0x0c,
	0x61,0x00,0x01,0x42,0x61,0x00,0x01,0x60,
	0x61,0x00,0x01,0x7a,0x08,0x00,0x00,0x0e,
	0x66,0x0c,0x08,0x00,0x00,0x1e,0x67,0x26,
	0x08,0x00,0x00,0x1b,0x66,0x08,0x61,0x00,
	0x01,0x7a,0x51,0xcc,0xff,0xbc,0x4c,0xdf,
	0x00,0xf2,0x4a,0x38,0x09,0xe1,0x67,0x0c,
	0x31,0xf8,0x09,0xc2,0x09,0xc4,0x11,0xfc,
	0x00,0x40,0x09,0xe1,0x4e,0x75,0x08,0x00,
	0x00,0x1f,0x66,0xe2,0xd3,0xc5,0x96,0x85,
	0x63,0xdc,0x20,0x04,0x48,0x40,0x38,0x00,
	0x30,0x3c,0x00,0x12,0x52,0x02,0xb0,0x02,
	0x64,0x86,0x14,0x3c,0x00,0x0a,0x0a,0x42,
	0x01,0x00,0x08,0x02,0x00,0x08,0x66,0x00,
	0xff,0x78,0xd4,0xbc,0x00,0x01,0x00,0x00,
	0x61,0x00,0x01,0xb8,0x08,0x00,0x00,0x1b,
	0x66,0xac,0x60,0x00,0xff,0x64,0x08,0x38,
	0x00,0x07,0x09,0xe1,0x66,0x0c,0x48,0xe7,
	0xc0,0x00,0x61,0x00,0x01,0x46,0x4c,0xdf,
	0x00,0x03,0x4e,0x75,0x70,0x00,0x78,0x00,
	0x08,0x01,0x00,0x05,0x67,0x08,0x78,0x09,
	0x48,0x44,0x38,0x3c,0x00,0x09,0x08,0x01,
	0x00,0x04,0x67,0x04,0x61,0x00,0x01,0x7c,
	0x4e,0x75,0x2f,0x01,0x41,0xf8,0x09,0xee,
	0x10,0x81,0xe0,0x99,0xc2,0x3c,0x00,0x03,
	0x08,0x02,0x00,0x08,0x67,0x04,0x08,0xc1,
	0x00,0x02,0x11,0x41,0x00,0x01,0x22,0x1f,
	0x4e,0x75,0x13,0xfc,0x00,0xff,0x00,0xe8,
	0x40,0x00,0x13,0xfc,0x00,0x32,0x00,0xe8,
	0x40,0x05,0x60,0x10,0x13,0xfc,0x00,0xff,
	0x00,0xe8,0x40,0x00,0x13,0xfc,0x00,0xb2,
	0x00,0xe8,0x40,0x05,0x23,0xc9,0x00,0xe8,
	0x40,0x0c,0x33,0xc5,0x00,0xe8,0x40,0x0a,
	0x13,0xfc,0x00,0x80,0x00,0xe8,0x40,0x07,
	0x4e,0x75,0x48,0xe7,0x40,0x60,0x43,0xf9,
	0x00,0xe9,0x40,0x01,0x45,0xf9,0x00,0xe9,
	0x40,0x03,0x40,0xe7,0x00,0x7c,0x07,0x00,
	0x12,0x11,0x08,0x01,0x00,0x04,0x66,0xf8,
	0x12,0x11,0x08,0x01,0x00,0x07,0x67,0xf8,
	0x08,0x01,0x00,0x06,0x66,0xf2,0x14,0x98,
	0x51,0xc8,0xff,0xee,0x46,0xdf,0x4c,0xdf,
	0x06,0x02,0x4e,0x75,0x10,0x39,0x00,0xe8,
	0x40,0x00,0x08,0x00,0x00,0x04,0x66,0x0e,
	0x10,0x39,0x00,0xe9,0x40,0x01,0xc0,0x3c,
	0x00,0x1f,0x66,0xf4,0x4e,0x75,0x10,0x39,
	0x00,0xe8,0x40,0x01,0x4e,0x75,0x10,0x39,
	0x00,0xe8,0x40,0x00,0x08,0x00,0x00,0x07,
	0x66,0x08,0x13,0xfc,0x00,0x10,0x00,0xe8,
	0x40,0x07,0x13,0xfc,0x00,0xff,0x00,0xe8,
	0x40,0x00,0x4e,0x75,0x30,0x01,0xe0,0x48,
	0xc0,0xbc,0x00,0x00,0x00,0x03,0xe7,0x40,
	0x41,0xf8,0x0c,0x90,0xd1,0xc0,0x20,0x10,
	0x4e,0x75,0x2f,0x00,0xc0,0xbc,0x00,0x35,
	0xff,0x00,0x67,0x2a,0xb8,0x3c,0x00,0x05,
	0x64,0x24,0x2f,0x38,0x09,0xee,0x2f,0x38,
	0x09,0xf2,0x3f,0x38,0x09,0xf6,0x61,0x00,
	0x00,0xc4,0x70,0x64,0x51,0xc8,0xff,0xfe,
	0x61,0x68,0x31,0xdf,0x09,0xf6,0x21,0xdf,
	0x09,0xf2,0x21,0xdf,0x09,0xee,0x20,0x1f,
	0x4e,0x75,0x30,0x01,0xe0,0x48,0x4a,0x00,
	0x67,0x3c,0xc0,0x3c,0x00,0x03,0x80,0x3c,
	0x00,0x80,0x08,0xf8,0x00,0x07,0x09,0xe1,
	0x13,0xc0,0x00,0xe9,0x40,0x07,0x08,0xf8,
	0x00,0x06,0x09,0xe1,0x66,0x18,0x31,0xf8,
	0x09,0xc2,0x09,0xc4,0x61,0x00,0x00,0x90,
	0x08,0x00,0x00,0x1d,0x66,0x08,0x0c,0x78,
	0x00,0x64,0x09,0xc4,0x64,0xee,0x08,0xb8,
	0x00,0x06,0x09,0xe1,0x4e,0x75,0x4a,0x38,
	0x09,0xe1,0x67,0x0c,0x31,0xf8,0x09,0xc2,
	0x09,0xc4,0x11,0xfc,0x00,0x40,0x09,0xe1,
	0x4e,0x75,0x61,0x12,0x08,0x00,0x00,0x1b,
	0x66,0x26,0x48,0x40,0x48,0x42,0xb4,0x00,
	0x67,0x1a,0x48,0x42,0x61,0x3e,0x2f,0x01,
	0x12,0x3c,0x00,0x0f,0x61,0x00,0xfe,0x6c,
	0x48,0x42,0x11,0x42,0x00,0x02,0x48,0x42,
	0x70,0x02,0x60,0x08,0x48,0x42,0x48,0x40,
	0x4e,0x75,0x2f,0x01,0x61,0x00,0xfe,0xac,
	0x61,0x00,0xfe,0xee,0x22,0x1f,0x30,0x01,
	0xe0,0x48,0xc0,0xbc,0x00,0x00,0x00,0x03,
	0xe7,0x40,0x41,0xf8,0x0c,0x90,0xd1,0xc0,
	0x20,0x10,0x4e,0x75,0x2f,0x01,0x12,0x3c,
	0x00,0x07,0x61,0x00,0xfe,0x2e,0x70,0x01,
	0x61,0xd0,0x22,0x1f,0x4e,0x75,0x2f,0x01,
	0x12,0x3c,0x00,0x04,0x61,0x00,0xfe,0x1c,
	0x22,0x1f,0x70,0x01,0x61,0x00,0xfe,0x6c,
	0x10,0x39,0x00,0xe9,0x40,0x01,0xc0,0x3c,
	0x00,0xd0,0xb0,0x3c,0x00,0xd0,0x66,0xf0,
	0x70,0x00,0x10,0x39,0x00,0xe9,0x40,0x03,
	0xe0,0x98,0x4e,0x75,0x53,0x02,0x7e,0x00,
	0x3a,0x02,0xe0,0x5d,0x4a,0x05,0x67,0x04,
	0x06,0x45,0x08,0x00,0xe0,0x4d,0x48,0x42,
	0x02,0x82,0x00,0x00,0x00,0xff,0xe9,0x8a,
	0xd4,0x45,0x0c,0x42,0x00,0x04,0x65,0x02,
	0x53,0x42,0x84,0xfc,0x00,0x12,0x48,0x42,
	0x3e,0x02,0x8e,0xfc,0x00,0x09,0x48,0x47,
	0xe1,0x4f,0xe0,0x8f,0x34,0x07,0x06,0x82,
	0x03,0x00,0x00,0x0a,0x2a,0x3c,0x00,0x00,
	0x04,0x00,0x3c,0x3c,0x00,0xff,0x3e,0x3c,
	0x09,0x28,0x4e,0x75,0x4f,0xfa,0xfc,0x82,
	0x43,0xfa,0xfc,0xa2,0x4d,0xfa,0xfc,0x7a,
	0x2c,0xb9,0x00,0x00,0x05,0x18,0x23,0xc9,
	0x00,0x00,0x05,0x18,0x43,0xfa,0x00,0xda,
	0x4d,0xfa,0xfc,0x6a,0x2c,0xb9,0x00,0x00,
	0x05,0x14,0x23,0xc9,0x00,0x00,0x05,0x14,
	0x43,0xfa,0x01,0x6e,0x4d,0xfa,0xfc,0x5a,
	0x2c,0xb9,0x00,0x00,0x05,0x04,0x23,0xc9,
	0x00,0x00,0x05,0x04,0x24,0x3c,0x03,0x00,
	0x00,0x04,0x20,0x3c,0x00,0x00,0x00,0x8e,
	0x4e,0x4f,0x12,0x00,0xe1,0x41,0x12,0x3c,
	0x00,0x70,0x33,0xc1,0x00,0x00,0x00,0x64,
	0x26,0x3c,0x00,0x00,0x04,0x00,0x43,0xfa,
	0x00,0x20,0x61,0x04,0x60,0x00,0x01,0xf0,
	0x48,0xe7,0x78,0x40,0x70,0x46,0x4e,0x4f,
	0x08,0x00,0x00,0x1e,0x66,0x02,0x70,0x00,
	0x4c,0xdf,0x02,0x1e,0x4e,0x75,0x4e,0x75,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x08,0x01,0x00,0x0c,0x66,0x08,0x4d,0xfa,
	0xfb,0x8c,0x2c,0x56,0x4e,0xd6,0x61,0x00,
	0xfc,0x6e,0x48,0xe7,0x4f,0x00,0x61,0x00,
	0xfe,0xa4,0x61,0x00,0xfc,0x78,0x08,0x00,
	0x00,0x1b,0x66,0x30,0xc2,0x3c,0x00,0xc0,
	0x82,0x3c,0x00,0x05,0x60,0x08,0x30,0x3c,
	0x01,0xac,0x51,0xc8,0xff,0xfe,0x61,0x00,
	0x01,0x00,0x08,0x00,0x00,0x1e,0x67,0x2c,
	0x08,0x00,0x00,0x1b,0x66,0x0e,0x08,0x00,
	0x00,0x11,0x66,0x08,0x61,0x00,0xfd,0x4c,
	0x51,0xcc,0xff,0xe4,0x4c,0xdf,0x00,0xf2,
	0x4a,0x38,0x09,0xe1,0x67,0x0c,0x31,0xf8,
	0x09,0xc2,0x09,0xc4,0x11,0xfc,0x00,0x40,
	0x09,0xe1,0x4e,0x75,0x08,0x00,0x00,0x1f,
	0x66,0xe2,0xd3,0xc5,0x96,0x85,0x63,0xdc,
	0x20,0x04,0x48,0x40,0x38,0x00,0x30,0x3c,
	0x00,0x12,0x52,0x02,0xb0,0x02,0x64,0xae,
	0x14,0x3c,0x00,0x0a,0x0a,0x42,0x01,0x00,
	0x08,0x02,0x00,0x08,0x66,0x98,0xd4,0xbc,
	0x00,0x01,0x00,0x00,0x61,0x00,0xfd,0x8c,
	0x08,0x00,0x00,0x1b,0x66,0xae,0x60,0x8e,
	0x08,0x01,0x00,0x0c,0x66,0x08,0x4d,0xfa,
	0xfa,0xe0,0x2c,0x56,0x4e,0xd6,0x61,0x00,
	0xfb,0xc6,0x48,0xe7,0x4f,0x00,0x61,0x00,
	0xfd,0xfc,0x61,0x00,0xfb,0xd0,0x08,0x00,
	0x00,0x1b,0x66,0x24,0xc2,0x3c,0x00,0xc0,
	0x82,0x3c,0x00,0x11,0x61,0x62,0x08,0x00,
	0x00,0x0a,0x66,0x14,0x08,0x00,0x00,0x1e,
	0x67,0x16,0x08,0x00,0x00,0x1b,0x66,0x08,
	0x61,0x00,0xfc,0xb0,0x51,0xcc,0xff,0xe6,
	0x4c,0xdf,0x00,0xf2,0x60,0x00,0xfb,0x34,
	0x08,0x00,0x00,0x1f,0x66,0xf2,0xd3,0xc5,
	0x96,0x85,0x63,0xec,0x20,0x04,0x48,0x40,
	0x38,0x00,0x30,0x3c,0x00,0x12,0x30,0x3c,
	0x00,0x12,0x52,0x02,0xb0,0x02,0x64,0xbc,
	0x14,0x3c,0x00,0x0a,0x0a,0x42,0x01,0x00,
	0x08,0x02,0x00,0x08,0x66,0xae,0xd4,0xbc,
	0x00,0x01,0x00,0x00,0x61,0x00,0xfc,0xfc,
	0x08,0x00,0x00,0x1b,0x66,0xba,0x60,0x9c,
	0x61,0x00,0xfb,0x78,0xe1,0x9a,0x54,0x88,
	0x20,0xc2,0xe0,0x9a,0x10,0xc2,0x10,0xc7,
	0x10,0x86,0x61,0x00,0xfb,0x86,0x41,0xf8,
	0x09,0xee,0x70,0x08,0x61,0x00,0xfb,0xb4,
	0x61,0x00,0xfb,0xea,0x61,0x00,0xfc,0x08,
	0x61,0x00,0xfc,0x22,0x4e,0x75,0x43,0xfa,
	0x01,0x8c,0x61,0x00,0x01,0x76,0x24,0x3c,
	0x03,0x00,0x00,0x06,0x32,0x39,0x00,0x00,
	0x00,0x64,0x26,0x3c,0x00,0x00,0x04,0x00,
	0x43,0xf8,0x28,0x00,0x61,0x00,0xfd,0xf2,
	0x4a,0x80,0x66,0x00,0x01,0x20,0x43,0xf8,
	0x28,0x00,0x49,0xfa,0x01,0x54,0x78,0x1f,
	0x24,0x49,0x26,0x4c,0x7a,0x0a,0x10,0x1a,
	0x80,0x3c,0x00,0x20,0xb0,0x1b,0x66,0x06,
	0x51,0xcd,0xff,0xf4,0x60,0x0c,0x43,0xe9,
	0x00,0x20,0x51,0xcc,0xff,0xe4,0x66,0x00,
	0x00,0xf4,0x30,0x29,0x00,0x1a,0xe1,0x58,
	0x55,0x40,0xd0,0x7c,0x00,0x0b,0x34,0x00,
	0xc4,0x7c,0x00,0x07,0x52,0x02,0xe8,0x48,
	0x64,0x04,0x84,0x7c,0x01,0x00,0x48,0x42,
	0x34,0x3c,0x03,0x00,0x14,0x00,0x48,0x42,
	0x26,0x29,0x00,0x1c,0xe1,0x5b,0x48,0x43,
	0xe1,0x5b,0x43,0xf8,0x67,0xc0,0x61,0x00,
	0xfd,0x88,0x0c,0x51,0x48,0x55,0x66,0x00,
	0x00,0xb4,0x4b,0xf8,0x68,0x00,0x49,0xfa,
	0x00,0x4c,0x22,0x4d,0x43,0xf1,0x38,0xc0,
	0x2c,0x3c,0x00,0x04,0x00,0x00,0x0c,0x69,
	0x4e,0xd4,0xff,0xd2,0x66,0x36,0x0c,0xad,
	0x4c,0x5a,0x58,0x20,0x00,0x04,0x66,0x16,
	0x2b,0x46,0x00,0x04,0x2b,0x4d,0x00,0x08,
	0x42,0xad,0x00,0x20,0x51,0xf9,0x00,0x00,
	0x07,0x9e,0x4e,0xed,0x00,0x02,0x0c,0x6d,
	0x4e,0xec,0x00,0x1a,0x66,0x0e,0x0c,0x6d,
	0x4e,0xea,0x00,0x2a,0x66,0x06,0x43,0xfa,
	0x01,0x1f,0x60,0x64,0x10,0x3c,0x00,0xc0,
	0x41,0xf8,0x68,0x00,0x36,0x3c,0xff,0xff,
	0xb0,0x18,0x67,0x26,0x51,0xcb,0xff,0xfa,
	0x43,0xf8,0x68,0x00,0x4a,0x39,0x00,0x00,
	0x07,0x9e,0x67,0x14,0x41,0xf8,0x67,0xcc,
	0x24,0x18,0xd4,0x98,0x22,0x10,0xd1,0xc2,
	0x53,0x81,0x65,0x04,0x42,0x18,0x60,0xf8,
	0x4e,0xd1,0x0c,0x10,0x00,0x04,0x66,0xd0,
	0x52,0x88,0x0c,0x10,0x00,0xd0,0x66,0xc8,
	0x52,0x88,0x0c,0x10,0x00,0xfe,0x66,0xc0,
	0x52,0x88,0x0c,0x10,0x00,0x02,0x66,0xb8,
	0x57,0x88,0x30,0xfc,0x05,0xa1,0x10,0xbc,
	0x00,0xfb,0x60,0xac,0x43,0xfa,0x00,0x92,
	0x2f,0x09,0x43,0xfa,0x00,0x47,0x61,0x2a,
	0x43,0xfa,0x00,0x46,0x61,0x24,0x43,0xfa,
	0x00,0x52,0x61,0x1e,0x43,0xfa,0x00,0x43,
	0x61,0x18,0x43,0xfa,0x00,0x46,0x61,0x12,
	0x22,0x5f,0x61,0x0e,0x32,0x39,0x00,0x00,
	0x00,0x64,0x70,0x4f,0x4e,0x4f,0x70,0xfe,
	0x4e,0x4f,0x70,0x21,0x4e,0x4f,0x4e,0x75,
	0x68,0x75,0x6d,0x61,0x6e,0x20,0x20,0x20,
	0x73,0x79,0x73,0x00,0x39,0x53,0x43,0x49,
	0x50,0x4c,0x00,0x1b,0x5b,0x34,0x37,0x6d,
	0x1b,0x5b,0x31,0x33,0x3b,0x32,0x36,0x48,
	0x00,0x1b,0x5b,0x31,0x34,0x3b,0x32,0x36,
	0x48,0x00,0x20,0x20,0x20,0x20,0x20,0x20,
	0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
	0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
	0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
	0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
	0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x00,
	0x1b,0x5b,0x31,0x34,0x3b,0x33,0x34,0x48,
	0x48,0x75,0x6d,0x61,0x6e,0x2e,0x73,0x79,
	0x73,0x20,0x82,0xcc,0x93,0xc7,0x82,0xdd,
	0x8d,0x9e,0x82,0xdd,0x83,0x47,0x83,0x89,
	0x81,0x5b,0x82,0xc5,0x82,0xb7,0x00,0x1b,
	0x5b,0x31,0x34,0x3b,0x33,0x34,0x48,0x4c,
	0x5a,0x58,0x2e,0x58,0x20,0x82,0xcc,0x83,
	0x6f,0x81,0x5b,0x83,0x57,0x83,0x87,0x83,
	0x93,0x82,0xaa,0x8c,0xc3,0x82,0xb7,0x82,
	0xac,0x82,0xdc,0x82,0xb7,0x00,0x00,0x00
};

//---------------------------------------------------------------------------
//
//	˜_—ƒtƒH[ƒ}ƒbƒg(2HC)
//
//---------------------------------------------------------------------------
void FASTCALL FDIDisk::Create2HC()
{
	FDITrack *track;
	FDISector *sector;
	BYTE buf[0x200];
	DWORD chrn[4];
	int i;

	ASSERT(this);

	// ’Êí‰Šú‰»
	memset(buf, 0, sizeof(buf));

	// ‡Œv29ƒZƒNƒ^‚Ö‘‚«‚Ş(1ƒgƒ‰ƒbƒN‚ ‚½‚è15ƒZƒNƒ^)
	track = Search(0);
	ASSERT(track);
	chrn[0] = 0;
	chrn[1] = 0;
	chrn[3] = 2;
	for (i=1; i<=15; i++) {
		chrn[2] = (BYTE)i;
		sector = track->Search(TRUE, chrn);
		ASSERT(sector);
		sector->Write(buf, FALSE);
	}
	track = Search(1);
	ASSERT(track);
	chrn[0] = 0;
	chrn[1] = 1;
	chrn[3] = 2;
	for (i=1; i<=14; i++) {
		chrn[2] = (BYTE)i;
		sector = track->Search(TRUE, chrn);
		ASSERT(sector);
		sector->Write(buf, FALSE);
	}

	// ƒgƒ‰ƒbƒN0‚ÖƒV[ƒN
	track = Search(0);
	ASSERT(track);

	// IPL‘‚«‚İ
	chrn[0] = 0;
	chrn[1] = 0;
	chrn[2] = 1;
	chrn[3] = 2;
	sector = track->Search(TRUE, chrn);
	ASSERT(sector);
	sector->Write(IPL2HC, FALSE);

	// FATæ“ªƒZƒNƒ^‰Šú‰»
	buf[0] = 0xf9;
	buf[1] = 0xff;
	buf[2] = 0xff;

	// ‘æ1FAT‘‚«‚İ
	chrn[0] = 0;
	chrn[1] = 0;
	chrn[2] = 2;
	chrn[3] = 2;
	sector = track->Search(TRUE, chrn);
	ASSERT(sector);
	sector->Write(buf, FALSE);

	// ‘æ2FAT‘‚«‚İ
	chrn[0] = 0;
	chrn[1] = 0;
	chrn[2] = 9;
	chrn[3] = 2;
	sector = track->Search(TRUE, chrn);
	ASSERT(sector);
	sector->Write(buf, FALSE);
}

//---------------------------------------------------------------------------
//
//	IPL(2HC)
//	¦FORMAT.x v2.31‚æ‚èæ“¾‚µ‚½‚à‚Ì
//
//---------------------------------------------------------------------------
const BYTE FDIDisk::IPL2HC[0x200] = {
	0x60,0x3c,0x90,0x58,0x36,0x38,0x49,0x50,
	0x4c,0x33,0x30,0x00,0x02,0x01,0x01,0x00,
	0x02,0xe0,0x00,0x60,0x09,0xf9,0x07,0x00,
	0x0f,0x00,0x02,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x20,0x20,0x20,0x20,0x20,
	0x20,0x20,0x20,0x20,0x20,0x20,0x46,0x41,
	0x54,0x31,0x32,0x20,0x20,0x20,0x4f,0xfa,
	0xff,0xc0,0x4d,0xfa,0x01,0xb8,0x4b,0xfa,
	0x00,0xe0,0x49,0xfa,0x00,0xea,0x43,0xfa,
	0x01,0x20,0x4e,0x94,0x70,0x8e,0x4e,0x4f,
	0x7e,0x70,0xe1,0x48,0x8e,0x40,0x26,0x3a,
	0x01,0x02,0x22,0x4e,0x24,0x3a,0x01,0x00,
	0x32,0x07,0x4e,0x95,0x66,0x28,0x22,0x4e,
	0x32,0x3a,0x00,0xfa,0x20,0x49,0x45,0xfa,
	0x01,0x78,0x70,0x0a,0x00,0x10,0x00,0x20,
	0xb1,0x0a,0x56,0xc8,0xff,0xf8,0x67,0x38,
	0xd2,0xfc,0x00,0x20,0x51,0xc9,0xff,0xe6,
	0x45,0xfa,0x00,0xe0,0x60,0x10,0x45,0xfa,
	0x00,0xfa,0x60,0x0a,0x45,0xfa,0x01,0x10,
	0x60,0x04,0x45,0xfa,0x01,0x28,0x61,0x00,
	0x00,0x94,0x22,0x4a,0x4c,0x99,0x00,0x06,
	0x70,0x23,0x4e,0x4f,0x4e,0x94,0x32,0x07,
	0x70,0x4f,0x4e,0x4f,0x70,0xfe,0x4e,0x4f,
	0x74,0x00,0x34,0x29,0x00,0x1a,0xe1,0x5a,
	0xd4,0x7a,0x00,0xa4,0x84,0xfa,0x00,0x9c,
	0x84,0x7a,0x00,0x94,0xe2,0x0a,0x64,0x04,
	0x08,0xc2,0x00,0x18,0x48,0x42,0x52,0x02,
	0x22,0x4e,0x26,0x3a,0x00,0x7e,0x32,0x07,
	0x4e,0x95,0x34,0x7c,0x68,0x00,0x22,0x4e,
	0x0c,0x59,0x48,0x55,0x66,0xa6,0x54,0x89,
	0xb5,0xd9,0x66,0xa6,0x2f,0x19,0x20,0x59,
	0xd1,0xd9,0x2f,0x08,0x2f,0x11,0x32,0x7c,
	0x67,0xc0,0x76,0x40,0xd6,0x88,0x4e,0x95,
	0x22,0x1f,0x24,0x1f,0x22,0x5f,0x4a,0x80,
	0x66,0x00,0xff,0x7c,0xd5,0xc2,0x53,0x81,
	0x65,0x04,0x42,0x1a,0x60,0xf8,0x4e,0xd1,
	0x70,0x46,0x4e,0x4f,0x08,0x00,0x00,0x1e,
	0x66,0x02,0x70,0x00,0x4e,0x75,0x70,0x21,
	0x4e,0x4f,0x4e,0x75,0x72,0x0f,0x70,0x22,
	0x4e,0x4f,0x72,0x19,0x74,0x0c,0x70,0x23,
	0x4e,0x4f,0x61,0x08,0x72,0x19,0x74,0x0d,
	0x70,0x23,0x4e,0x4f,0x76,0x2c,0x72,0x20,
	0x70,0x20,0x4e,0x4f,0x51,0xcb,0xff,0xf8,
	0x4e,0x75,0x00,0x00,0x02,0x00,0x02,0x00,
	0x01,0x01,0x00,0x0f,0x00,0x0f,0x00,0x1b,
	0x1a,0x00,0x00,0x22,0x00,0x0d,0x48,0x75,
	0x6d,0x61,0x6e,0x2e,0x73,0x79,0x73,0x20,
	0x82,0xaa,0x20,0x8c,0xa9,0x82,0xc2,0x82,
	0xa9,0x82,0xe8,0x82,0xdc,0x82,0xb9,0x82,
	0xf1,0x00,0x00,0x25,0x00,0x0d,0x83,0x66,
	0x83,0x42,0x83,0x58,0x83,0x4e,0x82,0xaa,
	0x81,0x40,0x93,0xc7,0x82,0xdf,0x82,0xdc,
	0x82,0xb9,0x82,0xf1,0x00,0x00,0x00,0x23,
	0x00,0x0d,0x48,0x75,0x6d,0x61,0x6e,0x2e,
	0x73,0x79,0x73,0x20,0x82,0xaa,0x20,0x89,
	0xf3,0x82,0xea,0x82,0xc4,0x82,0xa2,0x82,
	0xdc,0x82,0xb7,0x00,0x00,0x20,0x00,0x0d,
	0x48,0x75,0x6d,0x61,0x6e,0x2e,0x73,0x79,
	0x73,0x20,0x82,0xcc,0x20,0x83,0x41,0x83,
	0x68,0x83,0x8c,0x83,0x58,0x82,0xaa,0x88,
	0xd9,0x8f,0xed,0x82,0xc5,0x82,0xb7,0x00,
	0x68,0x75,0x6d,0x61,0x6e,0x20,0x20,0x20,
	0x73,0x79,0x73,0x00,0x00,0x00,0x00,0x00
};

//---------------------------------------------------------------------------
//
//	˜_—ƒtƒH[ƒ}ƒbƒg(2HDE)
//
//---------------------------------------------------------------------------
void FASTCALL FDIDisk::Create2HDE()
{
	FDITrack *track;
	FDISector *sector;
	BYTE buf[0x400];
	DWORD chrn[4];
	int i;

	ASSERT(this);

	// ’Êí‰Šú‰»
	memset(buf, 0, sizeof(buf));

	// ‡Œv13ƒZƒNƒ^‚Ö‘‚«‚Ş(1ƒgƒ‰ƒbƒN‚ ‚½‚è9ƒZƒNƒ^)
	track = Search(0);
	ASSERT(track);
	chrn[0] = 0;
	chrn[1] = 0x80;
	chrn[3] = 3;
	for (i=2; i<=9; i++) {
		chrn[2] = i;
		sector = track->Search(TRUE, chrn);
		ASSERT(sector);
		sector->Write(buf, FALSE);
	}
	track = Search(1);
	ASSERT(track);
	chrn[0] = 0;
	chrn[1] = 0x81;
	chrn[3] = 3;
	for (i=1; i<=4; i++) {
		chrn[2] = i;
		sector = track->Search(TRUE, chrn);
		ASSERT(sector);
		sector->Write(buf, FALSE);
	}

	// ƒgƒ‰ƒbƒN0‚ÖƒV[ƒN
	track = Search(0);
	ASSERT(track);

	// IPL‘‚«‚İ(1)
	chrn[0] = 0;
	chrn[1] = 0;
	chrn[2] = 1;
	chrn[3] = 3;
	sector = track->Search(TRUE, chrn);
	ASSERT(sector);
	sector->Write(&IPL2HDE[0x000], FALSE);

	// IPL‘‚«‚İ(2)
	chrn[0] = 0;
	chrn[1] = 0x80;
	chrn[2] = 4;
	chrn[3] = 3;
	sector = track->Search(TRUE, chrn);
	ASSERT(sector);
	sector->Write(&IPL2HDE[0x400], FALSE);

	// FATæ“ªƒZƒNƒ^‰Šú‰»
	buf[0] = 0xf8;
	buf[1] = 0xff;
	buf[2] = 0xff;

	// ‘æ1FAT‘‚«‚İ
	chrn[0] = 0;
	chrn[1] = 0x80;
	chrn[2] = 2;
	chrn[3] = 3;
	sector = track->Search(TRUE, chrn);
	ASSERT(sector);
	sector->Write(buf, FALSE);

	// ‘æ2FAT‘‚«‚İ
	chrn[0] = 0;
	chrn[1] = 0x80;
	chrn[2] = 5;
	chrn[3] = 3;
	sector = track->Search(TRUE, chrn);
	ASSERT(sector);
	sector->Write(buf, FALSE);
}

//---------------------------------------------------------------------------
//
//	IPL(2HDE)
//	¦9scdrv.x v3.00‚æ‚èæ“¾‚µ‚½‚à‚Ì
//
//---------------------------------------------------------------------------
const BYTE FDIDisk::IPL2HDE[0x800] = {
	0x60,0x20,0x32,0x48,0x44,0x45,0x20,0x76,
	0x31,0x2e,0x31,0x00,0x00,0x04,0x01,0x01,
	0x00,0x02,0xc0,0x00,0xa0,0x05,0x03,0x03,
	0x00,0x09,0x00,0x02,0x00,0x00,0x00,0x00,
	0x90,0x70,0x60,0x00,0x03,0x5a,0x08,0x01,
	0x00,0x0c,0x66,0x08,0x4d,0xfa,0xff,0xd2,
	0x2c,0x56,0x4e,0xd6,0x61,0x00,0x00,0xba,
	0x48,0xe7,0x4f,0x00,0x61,0x00,0x02,0xf0,
	0x61,0x00,0x00,0xc4,0x08,0x00,0x00,0x1b,
	0x66,0x4e,0xc2,0x3c,0x00,0xc0,0x82,0x3c,
	0x00,0x06,0x61,0x00,0x00,0xd0,0xe1,0x9a,
	0x54,0x88,0x20,0xc2,0xe0,0x9a,0x10,0xc2,
	0x10,0xc7,0x10,0x86,0x61,0x00,0x00,0xf0,
	0x41,0xf8,0x09,0xee,0x70,0x08,0x61,0x00,
	0x01,0x0c,0x61,0x00,0x01,0x42,0x61,0x00,
	0x01,0x60,0x61,0x00,0x01,0x7a,0x08,0x00,
	0x00,0x0e,0x66,0x0c,0x08,0x00,0x00,0x1e,
	0x67,0x26,0x08,0x00,0x00,0x1b,0x66,0x08,
	0x61,0x00,0x01,0x7a,0x51,0xcc,0xff,0xbc,
	0x4c,0xdf,0x00,0xf2,0x4a,0x38,0x09,0xe1,
	0x67,0x0c,0x31,0xf8,0x09,0xc2,0x09,0xc4,
	0x11,0xfc,0x00,0x40,0x09,0xe1,0x4e,0x75,
	0x08,0x00,0x00,0x1f,0x66,0xe2,0xd3,0xc5,
	0x96,0x85,0x63,0xdc,0x20,0x04,0x48,0x40,
	0x38,0x00,0x30,0x3c,0x00,0x09,0x52,0x02,
	0xb0,0x02,0x64,0x86,0x14,0x3c,0x00,0x01,
	0x0a,0x42,0x01,0x00,0x08,0x02,0x00,0x08,
	0x66,0x00,0xff,0x78,0xd4,0xbc,0x00,0x01,
	0x00,0x00,0x61,0x00,0x01,0xb8,0x08,0x00,
	0x00,0x1b,0x66,0xac,0x60,0x00,0xff,0x64,
	0x08,0x38,0x00,0x07,0x09,0xe1,0x66,0x0c,
	0x48,0xe7,0xc0,0x00,0x61,0x00,0x01,0x46,
	0x4c,0xdf,0x00,0x03,0x4e,0x75,0x70,0x00,
	0x78,0x00,0x08,0x01,0x00,0x05,0x67,0x08,
	0x78,0x09,0x48,0x44,0x38,0x3c,0x00,0x09,
	0x08,0x01,0x00,0x04,0x67,0x04,0x61,0x00,
	0x01,0x7c,0x4e,0x75,0x2f,0x01,0x41,0xf8,
	0x09,0xee,0x10,0x81,0xe0,0x99,0xc2,0x3c,
	0x00,0x03,0x08,0x02,0x00,0x08,0x67,0x04,
	0x08,0xc1,0x00,0x02,0x11,0x41,0x00,0x01,
	0x22,0x1f,0x4e,0x75,0x13,0xfc,0x00,0xff,
	0x00,0xe8,0x40,0x00,0x13,0xfc,0x00,0x32,
	0x00,0xe8,0x40,0x05,0x60,0x10,0x13,0xfc,
	0x00,0xff,0x00,0xe8,0x40,0x00,0x13,0xfc,
	0x00,0xb2,0x00,0xe8,0x40,0x05,0x23,0xc9,
	0x00,0xe8,0x40,0x0c,0x33,0xc5,0x00,0xe8,
	0x40,0x0a,0x13,0xfc,0x00,0x80,0x00,0xe8,
	0x40,0x07,0x4e,0x75,0x48,0xe7,0x40,0x60,
	0x43,0xf9,0x00,0xe9,0x40,0x01,0x45,0xf9,
	0x00,0xe9,0x40,0x03,0x40,0xe7,0x00,0x7c,
	0x07,0x00,0x12,0x11,0x08,0x01,0x00,0x04,
	0x66,0xf8,0x12,0x11,0x08,0x01,0x00,0x07,
	0x67,0xf8,0x08,0x01,0x00,0x06,0x66,0xf2,
	0x14,0x98,0x51,0xc8,0xff,0xee,0x46,0xdf,
	0x4c,0xdf,0x06,0x02,0x4e,0x75,0x10,0x39,
	0x00,0xe8,0x40,0x00,0x08,0x00,0x00,0x04,
	0x66,0x0e,0x10,0x39,0x00,0xe9,0x40,0x01,
	0xc0,0x3c,0x00,0x1f,0x66,0xf4,0x4e,0x75,
	0x10,0x39,0x00,0xe8,0x40,0x01,0x4e,0x75,
	0x10,0x39,0x00,0xe8,0x40,0x00,0x08,0x00,
	0x00,0x07,0x66,0x08,0x13,0xfc,0x00,0x10,
	0x00,0xe8,0x40,0x07,0x13,0xfc,0x00,0xff,
	0x00,0xe8,0x40,0x00,0x4e,0x75,0x30,0x01,
	0xe0,0x48,0xc0,0xbc,0x00,0x00,0x00,0x03,
	0xe7,0x40,0x41,0xf8,0x0c,0x90,0xd1,0xc0,
	0x20,0x10,0x4e,0x75,0x2f,0x00,0xc0,0xbc,
	0x00,0x35,0xff,0x00,0x67,0x2a,0xb8,0x3c,
	0x00,0x05,0x64,0x24,0x2f,0x38,0x09,0xee,
	0x2f,0x38,0x09,0xf2,0x3f,0x38,0x09,0xf6,
	0x61,0x00,0x00,0xc4,0x70,0x64,0x51,0xc8,
	0xff,0xfe,0x61,0x68,0x31,0xdf,0x09,0xf6,
	0x21,0xdf,0x09,0xf2,0x21,0xdf,0x09,0xee,
	0x20,0x1f,0x4e,0x75,0x30,0x01,0xe0,0x48,
	0x4a,0x00,0x67,0x3c,0xc0,0x3c,0x00,0x03,
	0x80,0x3c,0x00,0x80,0x08,0xf8,0x00,0x07,
	0x09,0xe1,0x13,0xc0,0x00,0xe9,0x40,0x07,
	0x08,0xf8,0x00,0x06,0x09,0xe1,0x66,0x18,
	0x31,0xf8,0x09,0xc2,0x09,0xc4,0x61,0x00,
	0x00,0x90,0x08,0x00,0x00,0x1d,0x66,0x08,
	0x0c,0x78,0x00,0x64,0x09,0xc4,0x64,0xee,
	0x08,0xb8,0x00,0x06,0x09,0xe1,0x4e,0x75,
	0x4a,0x38,0x09,0xe1,0x67,0x0c,0x31,0xf8,
	0x09,0xc2,0x09,0xc4,0x11,0xfc,0x00,0x40,
	0x09,0xe1,0x4e,0x75,0x61,0x12,0x08,0x00,
	0x00,0x1b,0x66,0x26,0x48,0x40,0x48,0x42,
	0xb4,0x00,0x67,0x1a,0x48,0x42,0x61,0x3e,
	0x2f,0x01,0x12,0x3c,0x00,0x0f,0x61,0x00,
	0xfe,0x6c,0x48,0x42,0x11,0x42,0x00,0x02,
	0x48,0x42,0x70,0x02,0x60,0x08,0x48,0x42,
	0x48,0x40,0x4e,0x75,0x2f,0x01,0x61,0x00,
	0xfe,0xac,0x61,0x00,0xfe,0xee,0x22,0x1f,
	0x30,0x01,0xe0,0x48,0xc0,0xbc,0x00,0x00,
	0x00,0x03,0xe7,0x40,0x41,0xf8,0x0c,0x90,
	0xd1,0xc0,0x20,0x10,0x4e,0x75,0x2f,0x01,
	0x12,0x3c,0x00,0x07,0x61,0x00,0xfe,0x2e,
	0x70,0x01,0x61,0xd0,0x22,0x1f,0x4e,0x75,
	0x2f,0x01,0x12,0x3c,0x00,0x04,0x61,0x00,
	0xfe,0x1c,0x22,0x1f,0x70,0x01,0x61,0x00,
	0xfe,0x6c,0x10,0x39,0x00,0xe9,0x40,0x01,
	0xc0,0x3c,0x00,0xd0,0xb0,0x3c,0x00,0xd0,
	0x66,0xf0,0x70,0x00,0x10,0x39,0x00,0xe9,
	0x40,0x03,0xe0,0x98,0x4e,0x75,0x53,0x02,
	0x7e,0x00,0x3a,0x02,0xe0,0x5d,0x4a,0x05,
	0x67,0x04,0x06,0x45,0x08,0x00,0xe0,0x4d,
	0x48,0x42,0x02,0x82,0x00,0x00,0x00,0xff,
	0xe9,0x8a,0xd4,0x45,0x0c,0x42,0x00,0x04,
	0x65,0x02,0x54,0x42,0x84,0xfc,0x00,0x12,
	0x48,0x42,0x3e,0x02,0x8e,0xfc,0x00,0x09,
	0x48,0x47,0xe1,0x4f,0xe0,0x8f,0x34,0x07,
	0x06,0x82,0x03,0x00,0x80,0x01,0x2a,0x3c,
	0x00,0x00,0x04,0x00,0x3c,0x3c,0x00,0xff,
	0x3e,0x3c,0x09,0x28,0x4e,0x75,0x4f,0xfa,
	0xfc,0x80,0x43,0xfa,0xfc,0xa2,0x4d,0xfa,
	0xfc,0x78,0x2c,0xb9,0x00,0x00,0x05,0x18,
	0x23,0xc9,0x00,0x00,0x05,0x18,0x43,0xfa,
	0x00,0xda,0x4d,0xfa,0xfc,0x68,0x2c,0xb9,
	0x00,0x00,0x05,0x14,0x23,0xc9,0x00,0x00,
	0x05,0x14,0x43,0xfa,0x01,0x6e,0x4d,0xfa,
	0xfc,0x58,0x2c,0xb9,0x00,0x00,0x05,0x04,
	0x23,0xc9,0x00,0x00,0x05,0x04,0x24,0x3c,
	0x03,0x00,0x00,0x04,0x20,0x3c,0x00,0x00,
	0x00,0x8e,0x4e,0x4f,0x12,0x00,0xe1,0x41,
	0x12,0x3c,0x00,0x70,0x33,0xc1,0x00,0x00,
	0x00,0x66,0x26,0x3c,0x00,0x00,0x04,0x00,
	0x43,0xfa,0x00,0x20,0x61,0x04,0x60,0x00,
	0x01,0xec,0x48,0xe7,0x78,0x40,0x70,0x46,
	0x4e,0x4f,0x08,0x00,0x00,0x1e,0x66,0x02,
	0x70,0x00,0x4c,0xdf,0x02,0x1e,0x4e,0x75,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x08,0x01,0x00,0x0c,0x66,0x08,0x4d,0xfa,
	0xfb,0x8a,0x2c,0x56,0x4e,0xd6,0x61,0x00,
	0xfc,0x6e,0x48,0xe7,0x4f,0x00,0x61,0x00,
	0xfe,0xa4,0x61,0x00,0xfc,0x78,0x08,0x00,
	0x00,0x1b,0x66,0x30,0xc2,0x3c,0x00,0xc0,
	0x82,0x3c,0x00,0x05,0x60,0x08,0x30,0x3c,
	0x01,0xac,0x51,0xc8,0xff,0xfe,0x61,0x00,
	0x00,0xfc,0x08,0x00,0x00,0x1e,0x67,0x2c,
	0x08,0x00,0x00,0x1b,0x66,0x0e,0x08,0x00,
	0x00,0x11,0x66,0x08,0x61,0x00,0xfd,0x4c,
	0x51,0xcc,0xff,0xe4,0x4c,0xdf,0x00,0xf2,
	0x4a,0x38,0x09,0xe1,0x67,0x0c,0x31,0xf8,
	0x09,0xc2,0x09,0xc4,0x11,0xfc,0x00,0x40,
	0x09,0xe1,0x4e,0x75,0x08,0x00,0x00,0x1f,
	0x66,0xe2,0xd3,0xc5,0x96,0x85,0x63,0xdc,
	0x20,0x04,0x48,0x40,0x38,0x00,0x30,0x3c,
	0x00,0x09,0x52,0x02,0xb0,0x02,0x64,0xae,
	0x14,0x3c,0x00,0x01,0x0a,0x42,0x01,0x00,
	0x08,0x02,0x00,0x08,0x66,0x98,0xd4,0xbc,
	0x00,0x01,0x00,0x00,0x61,0x00,0xfd,0x8c,
	0x08,0x00,0x00,0x1b,0x66,0xae,0x60,0x8e,
	0x08,0x01,0x00,0x0c,0x66,0x08,0x4d,0xfa,
	0xfa,0xde,0x2c,0x56,0x4e,0xd6,0x61,0x00,
	0xfb,0xc6,0x48,0xe7,0x4f,0x00,0x61,0x00,
	0xfd,0xfc,0x61,0x00,0xfb,0xd0,0x08,0x00,
	0x00,0x1b,0x66,0x24,0xc2,0x3c,0x00,0xc0,
	0x82,0x3c,0x00,0x11,0x61,0x5e,0x08,0x00,
	0x00,0x0a,0x66,0x14,0x08,0x00,0x00,0x1e,
	0x67,0x16,0x08,0x00,0x00,0x1b,0x66,0x08,
	0x61,0x00,0xfc,0xb0,0x51,0xcc,0xff,0xe6,
	0x4c,0xdf,0x00,0xf2,0x60,0x00,0xfb,0x34,
	0x08,0x00,0x00,0x1f,0x66,0xf2,0xd3,0xc5,
	0x96,0x85,0x63,0xec,0x20,0x04,0x48,0x40,
	0x38,0x00,0x30,0x3c,0x00,0x09,0x52,0x02,
	0xb0,0x02,0x64,0xc0,0x14,0x3c,0x00,0x01,
	0x0a,0x42,0x01,0x00,0x08,0x02,0x00,0x08,
	0x66,0xb2,0xd4,0xbc,0x00,0x01,0x00,0x00,
	0x61,0x00,0xfd,0x00,0x08,0x00,0x00,0x1b,
	0x66,0xbe,0x60,0xa0,0x61,0x00,0xfb,0x7c,
	0xe1,0x9a,0x54,0x88,0x20,0xc2,0xe0,0x9a,
	0x10,0xc2,0x10,0xc7,0x10,0x86,0x61,0x00,
	0xfb,0x8a,0x41,0xf8,0x09,0xee,0x70,0x08,
	0x61,0x00,0xfb,0xb8,0x61,0x00,0xfb,0xee,
	0x61,0x00,0xfc,0x0c,0x61,0x00,0xfc,0x26,
	0x4e,0x75,0x43,0xfa,0x01,0x8c,0x61,0x00,
	0x01,0x76,0x24,0x3c,0x03,0x00,0x00,0x06,
	0x32,0x39,0x00,0x00,0x00,0x66,0x26,0x3c,
	0x00,0x00,0x04,0x00,0x43,0xf8,0x28,0x00,
	0x61,0x00,0xfd,0xf6,0x4a,0x80,0x66,0x00,
	0x01,0x20,0x43,0xf8,0x28,0x00,0x49,0xfa,
	0x01,0x54,0x78,0x1f,0x24,0x49,0x26,0x4c,
	0x7a,0x0a,0x10,0x1a,0x80,0x3c,0x00,0x20,
	0xb0,0x1b,0x66,0x06,0x51,0xcd,0xff,0xf4,
	0x60,0x0c,0x43,0xe9,0x00,0x20,0x51,0xcc,
	0xff,0xe4,0x66,0x00,0x00,0xf4,0x30,0x29,
	0x00,0x1a,0xe1,0x58,0x55,0x40,0xd0,0x7c,
	0x00,0x0b,0x34,0x00,0xc4,0x7c,0x00,0x07,
	0x52,0x02,0xe8,0x48,0x64,0x04,0x84,0x7c,
	0x01,0x00,0x48,0x42,0x34,0x3c,0x03,0x00,
	0x14,0x00,0x48,0x42,0x26,0x29,0x00,0x1c,
	0xe1,0x5b,0x48,0x43,0xe1,0x5b,0x43,0xf8,
	0x67,0xc0,0x61,0x00,0xfd,0x8c,0x0c,0x51,
	0x48,0x55,0x66,0x00,0x00,0xb4,0x4b,0xf8,
	0x68,0x00,0x49,0xfa,0x00,0x4c,0x22,0x4d,
	0x43,0xf1,0x38,0xc0,0x2c,0x3c,0x00,0x04,
	0x00,0x00,0x0c,0x69,0x4e,0xd4,0xff,0xd2,
	0x66,0x36,0x0c,0xad,0x4c,0x5a,0x58,0x20,
	0x00,0x04,0x66,0x16,0x2b,0x46,0x00,0x04,
	0x2b,0x4d,0x00,0x08,0x42,0xad,0x00,0x20,
	0x51,0xf9,0x00,0x00,0x07,0x9c,0x4e,0xed,
	0x00,0x02,0x0c,0x6d,0x4e,0xec,0x00,0x1a,
	0x66,0x0e,0x0c,0x6d,0x4e,0xea,0x00,0x2a,
	0x66,0x06,0x43,0xfa,0x01,0x20,0x60,0x64,
	0x10,0x3c,0x00,0xc0,0x41,0xf8,0x68,0x00,
	0x36,0x3c,0xff,0xff,0xb0,0x18,0x67,0x26,
	0x51,0xcb,0xff,0xfa,0x43,0xf8,0x68,0x00,
	0x4a,0x39,0x00,0x00,0x07,0x9c,0x67,0x14,
	0x41,0xf8,0x67,0xcc,0x24,0x18,0xd4,0x98,
	0x22,0x10,0xd1,0xc2,0x53,0x81,0x65,0x04,
	0x42,0x18,0x60,0xf8,0x4e,0xd1,0x0c,0x10,
	0x00,0x04,0x66,0xd0,0x52,0x88,0x0c,0x10,
	0x00,0xd0,0x66,0xc8,0x52,0x88,0x0c,0x10,
	0x00,0xfe,0x66,0xc0,0x52,0x88,0x0c,0x10,
	0x00,0x02,0x66,0xb8,0x57,0x88,0x30,0xfc,
	0x05,0x9e,0x10,0xbc,0x00,0xfb,0x60,0xac,
	0x43,0xfa,0x00,0x93,0x2f,0x09,0x43,0xfa,
	0x00,0x48,0x61,0x2a,0x43,0xfa,0x00,0x47,
	0x61,0x24,0x43,0xfa,0x00,0x53,0x61,0x1e,
	0x43,0xfa,0x00,0x44,0x61,0x18,0x43,0xfa,
	0x00,0x47,0x61,0x12,0x22,0x5f,0x61,0x0e,
	0x32,0x39,0x00,0x00,0x00,0x66,0x70,0x4f,
	0x4e,0x4f,0x70,0xfe,0x4e,0x4f,0x70,0x21,
	0x4e,0x4f,0x4e,0x75,0x68,0x75,0x6d,0x61,
	0x6e,0x20,0x20,0x20,0x73,0x79,0x73,0x00,
	0x32,0x48,0x44,0x45,0x49,0x50,0x4c,0x00,
	0x1b,0x5b,0x34,0x37,0x6d,0x1b,0x5b,0x31,
	0x33,0x3b,0x32,0x36,0x48,0x00,0x1b,0x5b,
	0x31,0x34,0x3b,0x32,0x36,0x48,0x00,0x20,
	0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
	0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
	0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
	0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
	0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
	0x20,0x20,0x20,0x20,0x00,0x1b,0x5b,0x31,
	0x34,0x3b,0x33,0x34,0x48,0x48,0x75,0x6d,
	0x61,0x6e,0x2e,0x73,0x79,0x73,0x20,0x82,
	0xcc,0x93,0xc7,0x82,0xdd,0x8d,0x9e,0x82,
	0xdd,0x83,0x47,0x83,0x89,0x81,0x5b,0x82,
	0xc5,0x82,0xb7,0x00,0x1b,0x5b,0x31,0x34,
	0x3b,0x33,0x34,0x48,0x4c,0x5a,0x58,0x2e,
	0x58,0x20,0x82,0xcc,0x83,0x6f,0x81,0x5b,
	0x83,0x57,0x83,0x87,0x83,0x93,0x82,0xaa,
	0x8c,0xc3,0x82,0xb7,0x82,0xac,0x82,0xdc,
	0x82,0xb7,0x00,0x00,0x00,0x00,0x00,0x00
};

//---------------------------------------------------------------------------
//
//	˜_—ƒtƒH[ƒ}ƒbƒg(2HQ)
//
//---------------------------------------------------------------------------
void FASTCALL FDIDisk::Create2HQ()
{
	FDITrack *track;
	FDISector *sector;
	BYTE buf[0x200];
	DWORD chrn[4];
	int i;

	ASSERT(this);

	// ’Êí‰Šú‰»
	memset(buf, 0, sizeof(buf));

	// ‡Œv33ƒZƒNƒ^‚Ö‘‚«‚Ş(1ƒgƒ‰ƒbƒN‚ ‚½‚è18ƒZƒNƒ^)
	track = Search(0);
	ASSERT(track);
	chrn[0] = 0;
	chrn[1] = 0;
	chrn[3] = 2;
	for (i=1; i<=18; i++) {
		chrn[2] = i;
		sector = track->Search(TRUE, chrn);
		ASSERT(sector);
		sector->Write(buf, FALSE);
	}
	track = Search(1);
	ASSERT(track);
	chrn[0] = 0;
	chrn[1] = 1;
	chrn[3] = 2;
	for (i=1; i<=15; i++) {
		chrn[2] = i;
		sector = track->Search(TRUE, chrn);
		ASSERT(sector);
		sector->Write(buf, FALSE);
	}

	// ƒgƒ‰ƒbƒN0‚ÖƒV[ƒN
	track = Search(0);
	ASSERT(track);

	// IPL‘‚«‚İ
	chrn[0] = 0;
	chrn[1] = 0;
	chrn[2] = 1;
	chrn[3] = 2;
	sector = track->Search(TRUE, chrn);
	ASSERT(sector);
	sector->Write(IPL2HQ, FALSE);

	// FATæ“ªƒZƒNƒ^‰Šú‰»
	buf[0] = 0xf0;
	buf[1] = 0xff;
	buf[2] = 0xff;

	// ‘æ1FAT‘‚«‚İ
	chrn[0] = 0;
	chrn[1] = 0;
	chrn[2] = 2;
	chrn[3] = 2;
	sector = track->Search(TRUE, chrn);
	ASSERT(sector);
	sector->Write(buf, FALSE);

	// ‘æ2FAT‘‚«‚İ
	chrn[0] = 0;
	chrn[1] = 0;
	chrn[2] = 11;
	chrn[3] = 2;
	sector = track->Search(TRUE, chrn);
	ASSERT(sector);
	sector->Write(buf, FALSE);
}

//---------------------------------------------------------------------------
//
//	IPL(2HQ)
//	¦FORMAT.x v2.31‚æ‚èæ“¾‚µ‚½‚à‚Ì
//
//---------------------------------------------------------------------------
const BYTE FDIDisk::IPL2HQ[0x200] = {
	0xeb,0xfe,0x90,0x58,0x36,0x38,0x49,0x50,
	0x4c,0x33,0x30,0x00,0x02,0x01,0x01,0x00,
	0x02,0xe0,0x00,0x40,0x0b,0xf0,0x09,0x00,
	0x12,0x00,0x02,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x20,0x20,0x20,0x20,0x20,
	0x20,0x20,0x20,0x20,0x20,0x20,0x46,0x41,
	0x54,0x31,0x32,0x20,0x20,0x20,0x4f,0xfa,
	0xff,0xc0,0x4d,0xfa,0x01,0xb8,0x4b,0xfa,
	0x00,0xe0,0x49,0xfa,0x00,0xea,0x43,0xfa,
	0x01,0x20,0x4e,0x94,0x70,0x8e,0x4e,0x4f,
	0x7e,0x70,0xe1,0x48,0x8e,0x40,0x26,0x3a,
	0x01,0x02,0x22,0x4e,0x24,0x3a,0x01,0x00,
	0x32,0x07,0x4e,0x95,0x66,0x28,0x22,0x4e,
	0x32,0x3a,0x00,0xfa,0x20,0x49,0x45,0xfa,
	0x01,0x78,0x70,0x0a,0x00,0x10,0x00,0x20,
	0xb1,0x0a,0x56,0xc8,0xff,0xf8,0x67,0x38,
	0xd2,0xfc,0x00,0x20,0x51,0xc9,0xff,0xe6,
	0x45,0xfa,0x00,0xe0,0x60,0x10,0x45,0xfa,
	0x00,0xfa,0x60,0x0a,0x45,0xfa,0x01,0x10,
	0x60,0x04,0x45,0xfa,0x01,0x28,0x61,0x00,
	0x00,0x94,0x22,0x4a,0x4c,0x99,0x00,0x06,
	0x70,0x23,0x4e,0x4f,0x4e,0x94,0x32,0x07,
	0x70,0x4f,0x4e,0x4f,0x70,0xfe,0x4e,0x4f,
	0x74,0x00,0x34,0x29,0x00,0x1a,0xe1,0x5a,
	0xd4,0x7a,0x00,0xa4,0x84,0xfa,0x00,0x9c,
	0x84,0x7a,0x00,0x94,0xe2,0x0a,0x64,0x04,
	0x08,0xc2,0x00,0x18,0x48,0x42,0x52,0x02,
	0x22,0x4e,0x26,0x3a,0x00,0x7e,0x32,0x07,
	0x4e,0x95,0x34,0x7c,0x68,0x00,0x22,0x4e,
	0x0c,0x59,0x48,0x55,0x66,0xa6,0x54,0x89,
	0xb5,0xd9,0x66,0xa6,0x2f,0x19,0x20,0x59,
	0xd1,0xd9,0x2f,0x08,0x2f,0x11,0x32,0x7c,
	0x67,0xc0,0x76,0x40,0xd6,0x88,0x4e,0x95,
	0x22,0x1f,0x24,0x1f,0x22,0x5f,0x4a,0x80,
	0x66,0x00,0xff,0x7c,0xd5,0xc2,0x53,0x81,
	0x65,0x04,0x42,0x1a,0x60,0xf8,0x4e,0xd1,
	0x70,0x46,0x4e,0x4f,0x08,0x00,0x00,0x1e,
	0x66,0x02,0x70,0x00,0x4e,0x75,0x70,0x21,
	0x4e,0x4f,0x4e,0x75,0x72,0x0f,0x70,0x22,
	0x4e,0x4f,0x72,0x19,0x74,0x0c,0x70,0x23,
	0x4e,0x4f,0x61,0x08,0x72,0x19,0x74,0x0d,
	0x70,0x23,0x4e,0x4f,0x76,0x2c,0x72,0x20,
	0x70,0x20,0x4e,0x4f,0x51,0xcb,0xff,0xf8,
	0x4e,0x75,0x00,0x00,0x02,0x00,0x02,0x00,
	0x01,0x02,0x00,0x12,0x00,0x0f,0x00,0x1f,
	0x1a,0x00,0x00,0x22,0x00,0x0d,0x48,0x75,
	0x6d,0x61,0x6e,0x2e,0x73,0x79,0x73,0x20,
	0x82,0xaa,0x20,0x8c,0xa9,0x82,0xc2,0x82,
	0xa9,0x82,0xe8,0x82,0xdc,0x82,0xb9,0x82,
	0xf1,0x00,0x00,0x25,0x00,0x0d,0x83,0x66,
	0x83,0x42,0x83,0x58,0x83,0x4e,0x82,0xaa,
	0x81,0x40,0x93,0xc7,0x82,0xdf,0x82,0xdc,
	0x82,0xb9,0x82,0xf1,0x00,0x00,0x00,0x23,
	0x00,0x0d,0x48,0x75,0x6d,0x61,0x6e,0x2e,
	0x73,0x79,0x73,0x20,0x82,0xaa,0x20,0x89,
	0xf3,0x82,0xea,0x82,0xc4,0x82,0xa2,0x82,
	0xdc,0x82,0xb7,0x00,0x00,0x20,0x00,0x0d,
	0x48,0x75,0x6d,0x61,0x6e,0x2e,0x73,0x79,
	0x73,0x20,0x82,0xcc,0x20,0x83,0x41,0x83,
	0x68,0x83,0x8c,0x83,0x58,0x82,0xaa,0x88,
	0xd9,0x8f,0xed,0x82,0xc5,0x82,0xb7,0x00,
	0x68,0x75,0x6d,0x61,0x6e,0x20,0x20,0x20,
	0x73,0x79,0x73,0x00,0x00,0x00,0x00,0x00
};

//---------------------------------------------------------------------------
//
//	˜_—ƒtƒH[ƒ}ƒbƒg(2DD)
//
//---------------------------------------------------------------------------
void FASTCALL FDIDisk::Create2DD()
{
	FDITrack *track;
	FDISector *sector;
	BYTE buf[0x200];
	DWORD chrn[4];
	int i;

	ASSERT(this);

	// ’Êí‰Šú‰»
	memset(buf, 0, sizeof(buf));

	// ‡Œv14ƒZƒNƒ^‚Ö‘‚«‚Ş(1ƒgƒ‰ƒbƒN‚ ‚½‚è18ƒZƒNƒ^)
	track = Search(0);
	ASSERT(track);
	chrn[0] = 0;
	chrn[1] = 0;
	chrn[3] = 2;
	for (i=1; i<=9; i++) {
		chrn[2] = i;
		sector = track->Search(TRUE, chrn);
		ASSERT(sector);
		sector->Write(buf, FALSE);
	}
	track = Search(1);
	ASSERT(track);
	chrn[0] = 0;
	chrn[1] = 1;
	chrn[3] = 2;
	for (i=1; i<=5; i++) {
		chrn[2] = i;
		sector = track->Search(TRUE, chrn);
		ASSERT(sector);
		sector->Write(buf, FALSE);
	}

	// ƒgƒ‰ƒbƒN0‚ÖƒV[ƒN
	track = Search(0);
	ASSERT(track);

	// IPL‰ÁH(2HQ—p‚ğƒx[ƒX‚Éì¬)
	memcpy(buf, IPL2HQ, sizeof(buf));
	buf[0] = 0x60;
	buf[1] = 0x3c;
	buf[13] = 0x02;
	buf[17] = 0x70;
	buf[19] = 0xa0;
	buf[20] = 0x05;
	buf[21] = 0xf9;
	buf[22] = 0x03;
	buf[24] = 0x09;
	buf[0x168] = 0x00;
	buf[0x169] = 0x08;
	buf[0x16b] = 0x09;
	buf[0x16f] = 0x0c;

	// IPL‘‚«‚İ
	chrn[0] = 0;
	chrn[1] = 0;
	chrn[2] = 1;
	chrn[3] = 2;
	sector = track->Search(TRUE, chrn);
	ASSERT(sector);
	sector->Write(buf, FALSE);
	memset(buf, 0, sizeof(buf));

	// FATæ“ªƒZƒNƒ^‰Šú‰»
	buf[0] = 0xf9;
	buf[1] = 0xff;
	buf[2] = 0xff;

	// ‘æ1FAT‘‚«‚İ
	chrn[0] = 0;
	chrn[1] = 0;
	chrn[2] = 2;
	chrn[3] = 2;
	sector = track->Search(TRUE, chrn);
	ASSERT(sector);
	sector->Write(buf, FALSE);

	// ‘æ2FAT‘‚«‚İ
	chrn[0] = 0;
	chrn[1] = 0;
	chrn[2] = 5;
	chrn[3] = 2;
	sector = track->Search(TRUE, chrn);
	ASSERT(sector);
	sector->Write(buf, FALSE);
}

//---------------------------------------------------------------------------
//
//	ƒI[ƒvƒ“
//	¦”h¶ƒNƒ‰ƒX‚Ì’ˆÓ“_F
//		writep, readonly‚ğ“KØ‚Éİ’è‚·‚é
//		path, name‚ğ“KØ‚Éİ’è‚·‚é
//	¦ãˆÊƒNƒ‰ƒX‚Ì’ˆÓ“_F
//		ŒÄ‚Ño‚µ‚½Œã‚ÅAŒ»İ‚ÌƒVƒŠƒ“ƒ_‚ÖƒV[ƒN‚·‚é
//
//---------------------------------------------------------------------------
BOOL FASTCALL FDIDisk::Open(const Filepath& /*path*/, DWORD /*offset*/)
{
	// ƒˆ‰¼‘zƒNƒ‰ƒX“I
	ASSERT(FALSE);
	return FALSE;
}

//---------------------------------------------------------------------------
//
//	‘‚«‚İ‹Ö~İ’è
//
//---------------------------------------------------------------------------
void FASTCALL FDIDisk::WriteP(BOOL flag)
{
	ASSERT(this);

	// ReadOnly‚È‚çAí‚É‘‚«‚İ‹Ö~
	if (IsReadOnly()) {
		disk.writep = TRUE;
		return;
	}

	// İ’è
	disk.writep = flag;
}

//---------------------------------------------------------------------------
//
//	ƒtƒ‰ƒbƒVƒ…
//
//---------------------------------------------------------------------------
BOOL FASTCALL FDIDisk::Flush()
{
	ASSERT(this);

	// ‰½‚à‚µ‚È‚¢
	return TRUE;
}

//---------------------------------------------------------------------------
//
//	ƒfƒBƒXƒN–¼æ“¾
//
//---------------------------------------------------------------------------
void FASTCALL FDIDisk::GetName(char *buf) const
{
	ASSERT(this);
	ASSERT(buf);

	strcpy(buf, disk.name);
}

//---------------------------------------------------------------------------
//
//	ƒpƒX–¼æ“¾
//
//---------------------------------------------------------------------------
void FASTCALL FDIDisk::GetPath(Filepath& path) const
{
	ASSERT(this);

	// ‘ã“ü
	path = disk.path;
}

//---------------------------------------------------------------------------
//
//	ƒV[ƒN
//	¦”h¶ƒNƒ‰ƒX‚Ì’ˆÓ“_F
//		Œ»İ‚Ìhead[]‚ğƒtƒ@ƒCƒ‹‚É‘‚«–ß‚µAV‚µ‚¢ƒgƒ‰ƒbƒN‚ğƒ[ƒhAhead[]‚ÖƒZƒbƒg
//
//---------------------------------------------------------------------------
void FASTCALL FDIDisk::Seek(int /*c*/)
{
	ASSERT(this);
}

//---------------------------------------------------------------------------
//
//	ƒŠ[ƒhID
//	¦Ÿ‚ÌƒXƒe[ƒ^ƒX‚ğ•Ô‚·(ƒGƒ‰[‚ÍOR‚³‚ê‚é)
//		FDD_NOERROR		ƒGƒ‰[‚È‚µ
//		FDD_MAM			w’è–§“x‚Å‚ÍƒAƒ“ƒtƒH[ƒ}ƒbƒg
//		FDD_NODATA		w’è–§“x‚Å‚ÍƒAƒ“ƒtƒH[ƒ}ƒbƒg‚©A
//						‚Ü‚½‚Í—LŒø‚ÈƒZƒNƒ^‚·‚×‚ÄID CRC
//
//---------------------------------------------------------------------------
int FASTCALL FDIDisk::ReadID(DWORD *buf, BOOL mfm, int hd)
{
	FDITrack *track;
	DWORD pos;

	ASSERT(this);
	ASSERT(buf);
	ASSERT((hd == 0) || (hd == 4));

	// ƒgƒ‰ƒbƒNæ“¾
	if (hd == 0) {
		track = GetHead(0);
	}
	else {
		track = GetHead(1);
	}

	// NULL‚È‚çNODATA
	if (!track) {
		// ŒŸõ‚É‚©‚©‚éŠÔ‚ğİ’è
		pos = GetRotationTime();
		pos = (pos * 2) - GetRotationPos();
		SetSearch(pos);
		return FDD_MAM | FDD_NODATA;
	}

	// ƒgƒ‰ƒbƒN‚É”C‚¹‚é
	return track->ReadID(buf, mfm);
}

//---------------------------------------------------------------------------
//
//	ƒŠ[ƒhƒZƒNƒ^
//	¦Ÿ‚ÌƒXƒe[ƒ^ƒX‚ğ•Ô‚·(ƒGƒ‰[‚ÍOR‚³‚ê‚é)
//		FDD_NOERROR		ƒGƒ‰[‚È‚µ
//		FDD_MAM			w’è–§“x‚Å‚ÍƒAƒ“ƒtƒH[ƒ}ƒbƒg
//		FDD_NODATA		w’è–§“x‚Å‚ÍƒAƒ“ƒtƒH[ƒ}ƒbƒg‚©A
//						‚Ü‚½‚Í—LŒø‚ÈƒZƒNƒ^‚ğ‘SŒŸõ‚µ‚ÄR‚ªˆê’v‚µ‚È‚¢
//		FDD_NOCYL		ŒŸõ’†‚ÉID‚ÌC‚ªˆê’v‚Å‚¸AFF‚Å‚È‚¢ƒZƒNƒ^‚ğŒ©‚Â‚¯‚½
//		FDD_BADCYL		ŒŸõ’†‚ÉID‚ÌC‚ªˆê’v‚¹‚¸AFF‚Æ‚È‚Á‚Ä‚¢‚éƒZƒNƒ^‚ğŒ©‚Â‚¯‚½
//		FDD_IDCRC		IDƒtƒB[ƒ‹ƒh‚ÉCRCƒGƒ‰[‚ª‚ ‚é
//		FDD_DATACRC		DATAƒtƒB[ƒ‹ƒh‚ÉCRCƒGƒ‰[‚ª‚ ‚é
//		FDD_DDAM		ƒfƒŠ[ƒeƒbƒhƒZƒNƒ^‚Å‚ ‚é
//
//---------------------------------------------------------------------------
int FASTCALL FDIDisk::ReadSector(BYTE *buf, int *len, BOOL mfm, const DWORD *chrn, int hd)
{
	FDITrack *track;
	DWORD pos;

	ASSERT(this);
	ASSERT(len);
	ASSERT(chrn);
	ASSERT((hd == 0) || (hd == 4));

	// ƒgƒ‰ƒbƒNæ“¾
	if (hd == 0) {
		track = GetHead(0);
	}
	else {
		track = GetHead(1);
	}

	// NULL‚È‚çNODATA
	if (!track) {
		// ŒŸõ‚É‚©‚©‚éŠÔ‚ğİ’è
		pos = GetRotationTime();
		pos = (pos * 2) - GetRotationPos();
		SetSearch(pos);
		return FDD_MAM | FDD_NODATA;
	}

	// ƒgƒ‰ƒbƒN‚É”C‚¹‚é
	return track->ReadSector(buf, len, mfm, chrn);
}

//---------------------------------------------------------------------------
//
//	ƒ‰ƒCƒgƒZƒNƒ^
//	¦Ÿ‚ÌƒXƒe[ƒ^ƒX‚ğ•Ô‚·(ƒGƒ‰[‚ÍOR‚³‚ê‚é)
//		FDD_NOERROR		ƒGƒ‰[‚È‚µ
//		FDD_NOTWRITE	ƒƒfƒBƒA‚Í‘‚«‚İ‹Ö~
//		FDD_MAM			w’è–§“x‚Å‚ÍƒAƒ“ƒtƒH[ƒ}ƒbƒg
//		FDD_NODATA		w’è–§“x‚Å‚ÍƒAƒ“ƒtƒH[ƒ}ƒbƒg‚©A
//						‚Ü‚½‚Í—LŒø‚ÈƒZƒNƒ^‚ğ‘SŒŸõ‚µ‚ÄR‚ªˆê’v‚µ‚È‚¢
//		FDD_NOCYL		ŒŸõ’†‚ÉID‚ÌC‚ªˆê’v‚Å‚¸AFF‚Å‚È‚¢ƒZƒNƒ^‚ğŒ©‚Â‚¯‚½
//		FDD_BADCYL		ŒŸõ’†‚ÉID‚ÌC‚ªˆê’v‚¹‚¸AFF‚Æ‚È‚Á‚Ä‚¢‚éƒZƒNƒ^‚ğŒ©‚Â‚¯‚½
//		FDD_IDCRC		IDƒtƒB[ƒ‹ƒh‚ÉCRCƒGƒ‰[‚ª‚ ‚é
//		FDD_DDAM		ƒfƒŠ[ƒeƒbƒhƒZƒNƒ^‚Å‚ ‚é
//
//---------------------------------------------------------------------------
int FASTCALL FDIDisk::WriteSector(const BYTE *buf, int *len, BOOL mfm, const DWORD *chrn, int hd, BOOL deleted)
{
	FDITrack *track;
	DWORD pos;

	ASSERT(this);
	ASSERT(len);
	ASSERT(chrn);
	ASSERT((hd == 0) || (hd == 4));

	// ‘‚«‚İƒ`ƒFƒbƒN
	if (IsWriteP()) {
		return FDD_NOTWRITE;
	}

	// ƒgƒ‰ƒbƒNæ“¾
	if (hd == 0) {
		track = GetHead(0);
	}
	else {
		track = GetHead(1);
	}

	// NULL‚È‚çNODATA
	if (!track) {
		// ŒŸõ‚É‚©‚©‚éŠÔ‚ğİ’è
		pos = GetRotationTime();
		pos = (pos * 2) - GetRotationPos();
		SetSearch(pos);
		return FDD_MAM | FDD_NODATA;
	}

	// ƒgƒ‰ƒbƒN‚É”C‚¹‚é
	return track->WriteSector(buf, len, mfm, chrn, deleted);
}

//---------------------------------------------------------------------------
//
//	ƒŠ[ƒhƒ_ƒCƒAƒO
//	¦Ÿ‚ÌƒXƒe[ƒ^ƒX‚ğ•Ô‚·(ƒGƒ‰[‚ÍOR‚³‚ê‚é)
//		FDD_NOERROR		ƒGƒ‰[‚È‚µ
//		FDD_MAM			w’è–§“x‚Å‚ÍƒAƒ“ƒtƒH[ƒ}ƒbƒg
//		FDD_NODATA		w’è–§“x‚Å‚ÍƒAƒ“ƒtƒH[ƒ}ƒbƒg‚©A
//						‚Ü‚½‚Í—LŒø‚ÈƒZƒNƒ^‚ğ‘SŒŸõ‚µ‚ÄR‚ªˆê’v‚µ‚È‚¢
//		FDD_IDCRC		IDƒtƒB[ƒ‹ƒh‚ÉCRCƒGƒ‰[‚ª‚ ‚é
//		FDD_DATACRC		ƒf[ƒ^ƒtƒB[ƒ‹ƒh‚ÉCRCƒGƒ‰[‚ª‚ ‚é
//		FDD_DDAM		ƒfƒŠ[ƒeƒbƒhƒZƒNƒ^‚Å‚ ‚é
//
//---------------------------------------------------------------------------
int FASTCALL FDIDisk::ReadDiag(BYTE *buf, int *len, BOOL mfm, const DWORD *chrn, int hd)
{
	FDITrack *track;
	DWORD pos;

	ASSERT(this);
	ASSERT(len);
	ASSERT(chrn);
	ASSERT((hd == 0) || (hd == 4));

	// ƒgƒ‰ƒbƒNæ“¾
	if (hd == 0) {
		track = GetHead(0);
	}
	else {
		track = GetHead(1);
	}

	// NULL‚È‚çNODATA
	if (!track) {
		// ŒŸõ‚É‚©‚©‚éŠÔ‚ğİ’è
		pos = GetRotationTime();
		pos = (pos * 2) - GetRotationPos();
		SetSearch(pos);
		return FDD_MAM | FDD_NODATA;
	}

	// ƒgƒ‰ƒbƒN‚É”C‚¹‚é
	return track->ReadDiag(buf, len, mfm, chrn);
}

//---------------------------------------------------------------------------
//
//	ƒ‰ƒCƒgID
//	¦Ÿ‚ÌƒXƒe[ƒ^ƒX‚ğ•Ô‚·(ƒGƒ‰[‚ÍOR‚³‚ê‚é)
//		FDD_NOERROR		ƒGƒ‰[‚È‚µ
//		FDD_NOTWRITE	ƒƒfƒBƒA‚Í‘‚«‚İ‹Ö~
//
//---------------------------------------------------------------------------
int FASTCALL FDIDisk::WriteID(const BYTE *buf, DWORD d, int sc, BOOL mfm, int hd, int gpl)
{
	FDITrack *track;

	ASSERT(this);
	ASSERT(sc > 0);

	// ‘‚«‚İƒ`ƒFƒbƒN
	if (IsWriteP()) {
		return FDD_NOTWRITE;
	}

	// ƒgƒ‰ƒbƒNæ“¾
	if (hd == 0) {
		track = GetHead(0);
	}
	else {
		track = GetHead(1);
	}

	// NULL‚È‚çNo Error‚Æ‚·‚é(format.x v2.20‚Å154ƒgƒ‰ƒbƒN‚ÉWrite ID)
	if (!track) {
		return FDD_NOERROR;
	}

	// ƒgƒ‰ƒbƒN‚É”C‚¹‚é
	return track->WriteID(buf, d, sc, mfm, gpl);
}

//---------------------------------------------------------------------------
//
//	‰ñ“]ˆÊ’uæ“¾
//
//---------------------------------------------------------------------------
DWORD FASTCALL FDIDisk::GetRotationPos() const
{
	ASSERT(this);
	ASSERT(GetFDI());

	// e‚É•·‚­
	return GetFDI()->GetRotationPos();
}

//---------------------------------------------------------------------------
//
//	‰ñ“]ŠÔæ“¾
//
//---------------------------------------------------------------------------
DWORD FASTCALL FDIDisk::GetRotationTime() const
{
	ASSERT(this);

	ASSERT(GetFDI());

	// e‚É•·‚­
	return GetFDI()->GetRotationTime();
}

//---------------------------------------------------------------------------
//
//	ŒŸõ’·‚³Zo
//
//---------------------------------------------------------------------------
void FASTCALL FDIDisk::CalcSearch(DWORD pos)
{
	DWORD cur;
	DWORD hus;

	ASSERT(this);

	// æ“¾
	cur = GetRotationPos();
	hus = GetRotationTime();

	// ƒJƒŒƒ“ƒg<ƒ|ƒWƒVƒ‡ƒ“‚È‚çA·‚ğo‚·‚Ì‚İ
	if (cur < pos) {
		SetSearch(pos - cur);
		return;
	}

	// ƒ|ƒWƒVƒ‡ƒ“ < ƒJƒŒƒ“ƒg‚ÍA‚Pü‚ğ’´‚¦‚Ä‚¢‚é
	ASSERT(cur <= hus);
	pos += (hus - cur);
	SetSearch(pos);
}

//---------------------------------------------------------------------------
//
//	HDƒtƒ‰ƒOæ“¾
//
//---------------------------------------------------------------------------
BOOL FASTCALL FDIDisk::IsHD() const
{
	ASSERT(this);
	ASSERT(GetFDI());

	// e‚É•·‚­
	return GetFDI()->IsHD();
}

//---------------------------------------------------------------------------
//
//	ƒgƒ‰ƒbƒN’Ç‰Á
//
//---------------------------------------------------------------------------
void FASTCALL FDIDisk::AddTrack(FDITrack *track)
{
	FDITrack *ptr;

	ASSERT(this);
	ASSERT(track);

	// ƒgƒ‰ƒbƒN‚ğ‚Á‚Ä‚¢‚È‚¯‚ê‚ÎA‚»‚Ì‚Ü‚Ü’Ç‰Á
	if (!disk.first) {
		disk.first = track;
		disk.first->SetNext(NULL);
		return;
	}

	// ÅIƒgƒ‰ƒbƒN‚ğ“¾‚é
	ptr = disk.first;
	while (ptr->GetNext()) {
		ptr = ptr->GetNext();
	}

	// ÅIƒgƒ‰ƒbƒN‚É’Ç‰Á
	ptr->SetNext(track);
	track->SetNext(NULL);
}

//---------------------------------------------------------------------------
//
//	ƒgƒ‰ƒbƒN‘Síœ
//
//---------------------------------------------------------------------------
void FASTCALL FDIDisk::ClrTrack()
{
	FDITrack *track;

	ASSERT(this);

	// ƒgƒ‰ƒbƒN‚ğ‚·‚×‚Äíœ
	while (disk.first) {
		track = disk.first->GetNext();
		delete disk.first;
		disk.first = track;
	}
}

//---------------------------------------------------------------------------
//
//	ƒgƒ‰ƒbƒNƒT[ƒ`
//
//---------------------------------------------------------------------------
FDITrack* FASTCALL FDIDisk::Search(int track) const
{
	FDITrack *p;

	ASSERT(this);
	ASSERT((track >= 0) && (track <= 163));

	// Å‰‚Ìƒgƒ‰ƒbƒN‚ğæ“¾
	p = GetFirst();

	// ƒ‹[ƒv
	while (p) {
		if (p->GetTrack() == track) {
			return p;
		}
		p = p->GetNext();
	}

	// Œ©‚Â‚©‚ç‚È‚¢
	return NULL;
}

//===========================================================================
//
//	FDI
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	ƒRƒ“ƒXƒgƒ‰ƒNƒ^
//
//---------------------------------------------------------------------------
FDI::FDI(FDD *fdd)
{
	ASSERT(fdd);

	// ƒ[ƒN‰Šú‰»
	fdi.fdd = fdd;
	fdi.disks = 0;
	fdi.first = NULL;
	fdi.disk = NULL;
}

//---------------------------------------------------------------------------
//
//	ƒfƒXƒgƒ‰ƒNƒ^
//
//---------------------------------------------------------------------------
FDI::~FDI()
{
	ClrDisk();
}

//---------------------------------------------------------------------------
//
//	ƒI[ƒvƒ“
//	¦ãˆÊƒNƒ‰ƒX‚Ì’ˆÓ“_F
//		ŒÄ‚Ño‚µ‚½Œã‚ÅAŒ»İ‚ÌƒVƒŠƒ“ƒ_‚ÖƒV[ƒN‚·‚é
//
//---------------------------------------------------------------------------
BOOL FASTCALL FDI::Open(const Filepath& path, int media = 0)
{
	FDIDisk2HD *disk2hd;
	FDIDiskDIM *diskdim;
	FDIDiskD68 *diskd68;
	FDIDisk2DD *disk2dd;
	FDIDisk2HQ *disk2hq;
	FDIDiskBAD *diskbad;
	int i;
	int num;
	DWORD offset[0x10];

	ASSERT(this);
	ASSERT((media >= 0) && (media < 0x10));

	// Šù‚ÉƒI[ƒvƒ“‚³‚ê‚Ä‚¢‚é‚È‚ç‚¨‚©‚µ‚¢
	ASSERT(!GetDisk());
	ASSERT(!GetFirst());
	ASSERT(fdi.disks == 0);

	// DIMƒtƒ@ƒCƒ‹‚Æ‚µ‚Äƒgƒ‰ƒC
	diskdim = new FDIDiskDIM(0, this);
	if (diskdim->Open(path, 0)) {
		AddDisk(diskdim);
		fdi.disk = diskdim;
		return TRUE;
	}
	// ¸”s
	delete diskdim;

	// D68ƒtƒ@ƒCƒ‹‚Æ‚µ‚Äƒgƒ‰ƒC(–‡”‚ğ”‚¦‚é)
	num = FDIDiskD68::CheckDisks(path, offset);
	if (num > 0) {
		// D68ƒfƒBƒXƒNì¬ƒ‹[ƒv(–‡”•ª‚¾‚¯’Ç‰Á)
		for (i=0; i<num; i++) {
			diskd68 = new FDIDiskD68(i, this);
			if (!diskd68->Open(path, offset[i])) {
				// ¸”s
				delete diskd68;
				ClrDisk();
				return FALSE;
			}
			AddDisk(diskd68);
		}
		// ƒƒfƒBƒAƒZƒŒƒNƒg
		fdi.disk = Search(media);
		if (!fdi.disk) {
			ClrDisk();
			return FALSE;
		}
		return TRUE;
	}

	// 2HDƒtƒ@ƒCƒ‹‚Æ‚µ‚Äƒgƒ‰ƒC
	disk2hd = new FDIDisk2HD(0, this);
	if (disk2hd->Open(path, 0)) {
		AddDisk(disk2hd);
		fdi.disk = disk2hd;
		return TRUE;
	}
	// ¸”s
	delete disk2hd;

	// 2DDƒtƒ@ƒCƒ‹‚Æ‚µ‚Äƒgƒ‰ƒC
	disk2dd = new FDIDisk2DD(0, this);
	if (disk2dd->Open(path, 0)) {
		AddDisk(disk2dd);
		fdi.disk = disk2dd;
		return TRUE;
	}
	// ¸”s
	delete disk2dd;

	// 2HQƒtƒ@ƒCƒ‹‚Æ‚µ‚Äƒgƒ‰ƒC
	disk2hq = new FDIDisk2HQ(0, this);
	if (disk2hq->Open(path, 0)) {
		AddDisk(disk2hq);
		fdi.disk = disk2hq;
		return TRUE;
	}
	// ¸”s
	delete disk2hq;

	// BADƒtƒ@ƒCƒ‹‚Æ‚µ‚Äƒgƒ‰ƒC
	diskbad = new FDIDiskBAD(0, this);
	if (diskbad->Open(path, 0)) {
		AddDisk(diskbad);
		fdi.disk = diskbad;
		return TRUE;
	}
	// ¸”s
	delete diskbad;

	// ƒGƒ‰[
	return FALSE;
}

//---------------------------------------------------------------------------
//
//	IDæ“¾
//
//---------------------------------------------------------------------------
DWORD FASTCALL FDI::GetID() const
{
	ASSERT(this);

	// ƒmƒbƒgƒŒƒfƒB‚È‚çNULL
	if (!IsReady()) {
		return MAKEID('N', 'U', 'L', 'L');
	}

	// ƒfƒBƒXƒN‚É•·‚­
	return GetDisk()->GetID();
}

//---------------------------------------------------------------------------
//
//	ƒ}ƒ‹ƒ`ƒfƒBƒXƒN‚©
//
//---------------------------------------------------------------------------
BOOL FASTCALL FDI::IsMulti() const
{
	ASSERT(this);

	// ƒfƒBƒXƒN‚ª2–‡ˆÈã‚È‚çTRUE
	if (GetDisks() >= 2) {
		return TRUE;
	}
	return FALSE;
}

//---------------------------------------------------------------------------
//
//	ƒƒfƒBƒA”Ô†‚ğæ“¾
//
//---------------------------------------------------------------------------
int FASTCALL FDI::GetMedia() const
{
	ASSERT(this);

	// ƒfƒBƒXƒN‚ª‚È‚¯‚ê‚Î0
	if (!GetDisk()) {
		return 0;
	}

	// ƒfƒBƒXƒN‚É•·‚­‚¾‚¯
	return GetDisk()->GetIndex();
}

//---------------------------------------------------------------------------
//
//	ƒfƒBƒXƒN–¼æ“¾
//
//---------------------------------------------------------------------------
void FASTCALL FDI::GetName(char *buf, int index) const
{
	FDIDisk *disk;

	ASSERT(this);
	ASSERT(buf);
	ASSERT(index >= -1);
	ASSERT(index < GetDisks());

	// -1‚Ì‚ÍAƒJƒŒƒ“ƒg‚ğˆÓ–¡‚·‚é
	if (index < 0) {
		// ƒmƒbƒgƒŒƒfƒB‚È‚çA‹ó•¶š—ñ
		if (!IsReady()) {
			buf[0] = '\0';
			return;
		}

		// ƒJƒŒƒ“ƒg‚É•·‚­
		GetDisk()->GetName(buf);
		return;
	}

	// ƒCƒ“ƒfƒbƒNƒX‚Â‚«‚È‚Ì‚ÅAŒŸõ
	disk = Search(index);
	if (!disk) {
		buf[0] = '\0';
		return;
	}
	disk->GetName(buf);
}

//---------------------------------------------------------------------------
//
//	ƒpƒXæ“¾
//
//---------------------------------------------------------------------------
void FASTCALL FDI::GetPath(Filepath& path) const
{
	ASSERT(this);

	// ƒmƒbƒgƒŒƒfƒB‚È‚çA‹ó•¶š—ñ
	if (!IsReady()) {
		path.Clear();
		return;
	}

	// ƒfƒBƒXƒN‚É•·‚­
	GetDisk()->GetPath(path);
}

//---------------------------------------------------------------------------
//
//	ƒŒƒfƒBƒ`ƒFƒbƒN
//
//---------------------------------------------------------------------------
BOOL FASTCALL FDI::IsReady() const
{
	ASSERT(this);

	// ƒJƒŒƒ“ƒgƒƒfƒBƒA‚ª‚ ‚ê‚ÎTRUEA‚È‚¯‚ê‚ÎFALSE
	if (GetDisk()) {
		return TRUE;
	}
	return FALSE;
}

//---------------------------------------------------------------------------
//
//	‘‚«‚İ‹Ö~‚©
//
//---------------------------------------------------------------------------
BOOL FASTCALL FDI::IsWriteP() const
{
	// ƒmƒbƒgƒŒƒfƒB‚È‚çFALSE
	if (!IsReady()) {
		return FALSE;
	}

	// ƒfƒBƒXƒN‚É•·‚­
	return GetDisk()->IsWriteP();
}

//---------------------------------------------------------------------------
//
//	Read OnlyƒfƒBƒXƒNƒCƒ[ƒW‚©
//
//---------------------------------------------------------------------------
BOOL FASTCALL FDI::IsReadOnly() const
{
	// ƒmƒbƒgƒŒƒfƒB‚È‚çFALSE
	if (!IsReady()) {
		return FALSE;
	}

	// ƒfƒBƒXƒN‚É•·‚­
	return GetDisk()->IsReadOnly();
}

//---------------------------------------------------------------------------
//
//	‘‚«‚İ‹Ö~ƒZƒbƒg
//
//---------------------------------------------------------------------------
void FASTCALL FDI::WriteP(BOOL flag)
{
	ASSERT(this);

	// ƒŒƒfƒB‚È‚çAw¦
	if (IsReady()) {
		GetDisk()->WriteP(flag);
	}
}

//---------------------------------------------------------------------------
//
//	ƒV[ƒN
//
//---------------------------------------------------------------------------
void FASTCALL FDI::Seek(int c)
{
	ASSERT(this);
	ASSERT((c >= 0) && (c < 82));

	// ƒmƒbƒgƒŒƒfƒB‚È‚ç‰½‚à‚µ‚È‚¢
	if (!IsReady()) {
		return;
	}

	// ƒfƒBƒXƒN‚É’Ê’m
	GetDisk()->Seek(c);
}

//---------------------------------------------------------------------------
//
//	ƒŠ[ƒhID
//	¦Ÿ‚ÌƒXƒe[ƒ^ƒX‚ğ•Ô‚·(ƒGƒ‰[‚ÍOR‚³‚ê‚é)
//		FDD_NOERROR		ƒGƒ‰[‚È‚µ
//		FDD_NOTREADY	ƒmƒbƒgƒŒƒfƒB
//		FDD_MAM			w’è–§“x‚Å‚ÍƒAƒ“ƒtƒH[ƒ}ƒbƒg
//		FDD_NODATA		w’è–§“x‚Å‚ÍƒAƒ“ƒtƒH[ƒ}ƒbƒg‚©A
//						‚Ü‚½‚Í—LŒø‚ÈƒZƒNƒ^‚·‚×‚ÄID CRC
//
//---------------------------------------------------------------------------
int FASTCALL FDI::ReadID(DWORD *buf, BOOL mfm, int hd)
{
	ASSERT(this);
	ASSERT(buf);
	ASSERT((hd == 0) || (hd == 4));

	// ƒmƒbƒgƒŒƒfƒB”»’è
	if (!IsReady()) {
		return FDD_NOTREADY;
	}

	// ƒfƒBƒXƒN‚É”C‚¹‚é
	return GetDisk()->ReadID(buf, mfm, hd);
}

//---------------------------------------------------------------------------
//
//	ƒŠ[ƒhƒZƒNƒ^
//	¦Ÿ‚ÌƒXƒe[ƒ^ƒX‚ğ•Ô‚·(ƒGƒ‰[‚ÍOR‚³‚ê‚é)
//		FDD_NOERROR		ƒGƒ‰[‚È‚µ
//		FDD_NOTREADY	ƒmƒbƒgƒŒƒfƒB
//		FDD_MAM			w’è–§“x‚Å‚ÍƒAƒ“ƒtƒH[ƒ}ƒbƒg
//		FDD_NODATA		w’è–§“x‚Å‚ÍƒAƒ“ƒtƒH[ƒ}ƒbƒg‚©A
//						‚Ü‚½‚Í—LŒø‚ÈƒZƒNƒ^‚ğ‘SŒŸõ‚µ‚ÄR‚ªˆê’v‚µ‚È‚¢
//		FDD_NOCYL		ŒŸõ’†‚ÉID‚ÌC‚ªˆê’v‚Å‚¸AFF‚Å‚È‚¢ƒZƒNƒ^‚ğŒ©‚Â‚¯‚½
//		FDD_BADCYL		ŒŸõ’†‚ÉID‚ÌC‚ªˆê’v‚¹‚¸AFF‚Æ‚È‚Á‚Ä‚¢‚éƒZƒNƒ^‚ğŒ©‚Â‚¯‚½
//		FDD_IDCRC		IDƒtƒB[ƒ‹ƒh‚ÉCRCƒGƒ‰[‚ª‚ ‚é
//		FDD_DATACRC		DATAƒtƒB[ƒ‹ƒh‚ÉCRCƒGƒ‰[‚ª‚ ‚é
//		FDD_DDAM		ƒfƒŠ[ƒeƒbƒhƒZƒNƒ^‚Å‚ ‚é
//
//---------------------------------------------------------------------------
int FASTCALL FDI::ReadSector(BYTE *buf, int *len, BOOL mfm, const DWORD *chrn, int hd)
{
	ASSERT(this);
	ASSERT(len);
	ASSERT(chrn);
	ASSERT((hd == 0) || (hd == 4));

	// ƒmƒbƒgƒŒƒfƒB”»’è
	if (!IsReady()) {
		return FDD_NOTREADY;
	}

	// ƒfƒBƒXƒN‚É”C‚¹‚é
	return GetDisk()->ReadSector(buf, len, mfm, chrn, hd);
}

//---------------------------------------------------------------------------
//
//	ƒ‰ƒCƒgƒZƒNƒ^
//	¦Ÿ‚ÌƒXƒe[ƒ^ƒX‚ğ•Ô‚·(ƒGƒ‰[‚ÍOR‚³‚ê‚é)
//		FDD_NOERROR		ƒGƒ‰[‚È‚µ
//		FDD_NOTREADY	ƒmƒbƒgƒŒƒfƒB
//		FDD_NOTWRITE	ƒƒfƒBƒA‚Í‘‚«‚İ‹Ö~
//		FDD_MAM			w’è–§“x‚Å‚ÍƒAƒ“ƒtƒH[ƒ}ƒbƒg
//		FDD_NODATA		w’è–§“x‚Å‚ÍƒAƒ“ƒtƒH[ƒ}ƒbƒg‚©A
//						‚Ü‚½‚Í—LŒø‚ÈƒZƒNƒ^‚ğ‘SŒŸõ‚µ‚ÄR‚ªˆê’v‚µ‚È‚¢
//		FDD_NOCYL		ŒŸõ’†‚ÉID‚ÌC‚ªˆê’v‚Å‚¸AFF‚Å‚È‚¢ƒZƒNƒ^‚ğŒ©‚Â‚¯‚½
//		FDD_BADCYL		ŒŸõ’†‚ÉID‚ÌC‚ªˆê’v‚¹‚¸AFF‚Æ‚È‚Á‚Ä‚¢‚éƒZƒNƒ^‚ğŒ©‚Â‚¯‚½
//		FDD_IDCRC		IDƒtƒB[ƒ‹ƒh‚ÉCRCƒGƒ‰[‚ª‚ ‚é
//		FDD_DDAM		ƒfƒŠ[ƒeƒbƒhƒZƒNƒ^‚Å‚ ‚é
//
//---------------------------------------------------------------------------
int FASTCALL FDI::WriteSector(const BYTE *buf, int *len, BOOL mfm, const DWORD *chrn, int hd, BOOL deleted)
{
	ASSERT(this);
	ASSERT(len);
	ASSERT(chrn);
	ASSERT((hd == 0) || (hd == 4));

	// ƒmƒbƒgƒŒƒfƒB”»’è
	if (!IsReady()) {
		return FDD_NOTREADY;
	}

	// ƒfƒBƒXƒN‚É”C‚¹‚é
	return GetDisk()->WriteSector(buf, len, mfm, chrn, hd, deleted);
}

//---------------------------------------------------------------------------
//
//	ƒŠ[ƒhƒ_ƒCƒAƒO
//	¦Ÿ‚ÌƒXƒe[ƒ^ƒX‚ğ•Ô‚·(ƒGƒ‰[‚ÍOR‚³‚ê‚é)
//		FDD_NOERROR		ƒGƒ‰[‚È‚µ
//		FDD_MAM			w’è–§“x‚Å‚ÍƒAƒ“ƒtƒH[ƒ}ƒbƒg
//		FDD_NODATA		w’è–§“x‚Å‚ÍƒAƒ“ƒtƒH[ƒ}ƒbƒg‚©A
//						‚Ü‚½‚Í—LŒø‚ÈƒZƒNƒ^‚ğ‘SŒŸõ‚µ‚ÄR‚ªˆê’v‚µ‚È‚¢
//		FDD_IDCRC		IDƒtƒB[ƒ‹ƒh‚ÉCRCƒGƒ‰[‚ª‚ ‚é
//		FDD_DATACRC		ƒf[ƒ^ƒtƒB[ƒ‹ƒh‚ÉCRCƒGƒ‰[‚ª‚ ‚é
//		FDD_DDAM		ƒfƒŠ[ƒeƒbƒhƒZƒNƒ^‚Å‚ ‚é
//
//---------------------------------------------------------------------------
int FASTCALL FDI::ReadDiag(BYTE *buf, int *len, BOOL mfm, const DWORD *chrn, int hd)
{
	ASSERT(this);
	ASSERT(len);
	ASSERT(chrn);
	ASSERT((hd == 0) || (hd == 4));

	// ƒmƒbƒgƒŒƒfƒB”»’è
	if (!IsReady()) {
		return FDD_NOTREADY;
	}

	// ƒfƒBƒXƒN‚É”C‚¹‚é
	return GetDisk()->ReadDiag(buf, len, mfm, chrn, hd);
}

//---------------------------------------------------------------------------
//
//	ƒ‰ƒCƒgID
//	¦Ÿ‚ÌƒXƒe[ƒ^ƒX‚ğ•Ô‚·(ƒGƒ‰[‚ÍOR‚³‚ê‚é)
//		FDD_NOERROR		ƒGƒ‰[‚È‚µ
//		FDD_NOTREADY	ƒmƒbƒgƒŒƒfƒB
//		FDD_NOTWRITE	ƒƒfƒBƒA‚Í‘‚«‚İ‹Ö~
//
//---------------------------------------------------------------------------
int FASTCALL FDI::WriteID(const BYTE *buf, DWORD d, int sc, BOOL mfm, int hd, int gpl)
{
	ASSERT(this);
	ASSERT(sc > 0);
	ASSERT((hd == 0) || (hd == 4));

	// ƒmƒbƒgƒŒƒfƒB”»’è
	if (!IsReady()) {
		return FDD_NOTREADY;
	}

	// ƒfƒBƒXƒN‚É”C‚¹‚é
	return GetDisk()->WriteID(buf, d, sc, mfm, hd, gpl);
}

//---------------------------------------------------------------------------
//
//	‰ñ“]ˆÊ’uæ“¾
//
//---------------------------------------------------------------------------
DWORD FASTCALL FDI::GetRotationPos() const
{
	ASSERT(this);
	ASSERT(GetFDD());

	// e‚É•·‚­
	return GetFDD()->GetRotationPos();
}

//---------------------------------------------------------------------------
//
//	‰ñ“]ŠÔæ“¾
//
//---------------------------------------------------------------------------
DWORD FASTCALL FDI::GetRotationTime() const
{
	ASSERT(this);
	ASSERT(GetFDD());

	// e‚É•·‚­
	return GetFDD()->GetRotationTime();
}

//---------------------------------------------------------------------------
//
//	ŒŸõŠÔæ“¾
//
//---------------------------------------------------------------------------
DWORD FASTCALL FDI::GetSearch() const
{
	FDIDisk *disk;

	ASSERT(this);

	// ƒmƒbƒgƒŒƒfƒB‚È‚çí‚É0
	disk = GetDisk();
	if (!disk) {
		return 0;
	}

	// ƒfƒBƒXƒN‚É•·‚­
	return disk->GetSearch();
}

//---------------------------------------------------------------------------
//
//	HDƒtƒ‰ƒOæ“¾
//
//---------------------------------------------------------------------------
BOOL FASTCALL FDI::IsHD() const
{
	ASSERT(this);
	ASSERT(GetFDD());

	// e‚É•·‚­
	return GetFDD()->IsHD();
}

//---------------------------------------------------------------------------
//
//	ƒfƒBƒXƒN’Ç‰Á
//
//---------------------------------------------------------------------------
void FASTCALL FDI::AddDisk(FDIDisk *disk)
{
	FDIDisk *ptr;

	ASSERT(this);
	ASSERT(disk);

	// ƒfƒBƒXƒN‚ğ‚Á‚Ä‚¢‚È‚¯‚ê‚ÎA‚»‚Ì‚Ü‚Ü’Ç‰Á
	if (!fdi.first) {
		fdi.first = disk;
		fdi.first->SetNext(NULL);

		// ƒJƒEƒ“ƒ^Up
		fdi.disks++;
		return;
	}

	// ÅIƒfƒBƒXƒN‚ğ“¾‚é
	ptr = fdi.first;
	while (ptr->GetNext()) {
		ptr = ptr->GetNext();
	}

	// ÅIƒfƒBƒXƒN‚É’Ç‰Á
	ptr->SetNext(disk);
	disk->SetNext(NULL);

	// ƒJƒEƒ“ƒ^Up
	fdi.disks++;
}

//---------------------------------------------------------------------------
//
//	ƒfƒBƒXƒN‘Síœ
//
//---------------------------------------------------------------------------
void FASTCALL FDI::ClrDisk()
{
	FDIDisk *disk;

	ASSERT(this);

	// ƒfƒBƒXƒN‚ğ‚·‚×‚Äíœ
	while (fdi.first) {
		disk = fdi.first->GetNext();
		delete fdi.first;
		fdi.first = disk;
	}

	// ƒJƒEƒ“ƒ^0
	fdi.disks = 0;
}

//---------------------------------------------------------------------------
//
//	ƒfƒBƒXƒNŒŸõ
//
//---------------------------------------------------------------------------
FDIDisk* FASTCALL FDI::Search(int index) const
{
	FDIDisk *disk;

	ASSERT(this);
	ASSERT(index >= 0);
	ASSERT(index < GetDisks());

	// Å‰‚ÌƒfƒBƒXƒN‚ğæ“¾
	disk = GetFirst();

	// ”äŠrƒ‹[ƒv
	while (disk) {
		if (disk->GetIndex() == index) {
			return disk;
		}

		// Ÿ‚Ö
		disk = disk->GetNext();
	}

	// Œ©‚Â‚©‚ç‚È‚¢
	return NULL;
}

//---------------------------------------------------------------------------
//
//	ƒZ[ƒu
//
//---------------------------------------------------------------------------
BOOL FASTCALL FDI::Save(Fileio *fio, int ver)
{
	BOOL ready;
	FDIDisk *disk;
	Filepath path;
	int i;
	int disks;
	int media;

	ASSERT(this);
	ASSERT(fio);

	// ƒŒƒfƒBƒtƒ‰ƒO‚ğ‘‚«‚Ş
	ready = IsReady();
	if (!fio->Write(&ready, sizeof(ready))) {
		return FALSE;
	}

	// ƒŒƒfƒB‚Å‚È‚¯‚ê‚ÎI—¹
	if (!ready) {
		return TRUE;
	}

	// ‘SƒƒfƒBƒA‚ğƒtƒ‰ƒbƒVƒ…
	disks = GetDisks();
	for (i=0; i<disks; i++) {
		disk = Search(i);
		ASSERT(disk);
		if (!disk->Flush()) {
			return FALSE;
		}
	}

	// ƒƒfƒBƒA‚ğ‘‚«‚Ş
	media = GetMedia();
	if (!fio->Write(&media, sizeof(media))) {
		return FALSE;
	}

	// ƒpƒX‚ğ‘‚«‚Ş
	GetPath(path);
	if (!path.Save(fio, ver)) {
		return FALSE;
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	ƒ[ƒh
//
//---------------------------------------------------------------------------
BOOL FASTCALL FDI::Load(Fileio *fio, int ver, BOOL *ready, int *media, Filepath& path)
{
	ASSERT(this);
	ASSERT(fio);
	ASSERT(ready);
	ASSERT(media);
	ASSERT(!IsReady());
	ASSERT(GetDisks() == 0);

	// ƒŒƒfƒBƒtƒ‰ƒO‚ğ“Ç‚İ‚Ş
	if (!fio->Read(ready, sizeof(BOOL))) {
		return FALSE;
	}

	// ƒŒƒfƒB‚Å‚È‚¯‚ê‚ÎI—¹
	if (!(*ready)) {
		return TRUE;
	}

	// ƒƒfƒBƒA‚ğ“Ç‚İ‚Ş
	if (!fio->Read(media, sizeof(int))) {
		return FALSE;
	}

	// ƒpƒX‚ğ“Ç‚İ‚Ş
	if (!path.Load(fio, ver)) {
		return FALSE;
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	’²®(“Áê)
//
//---------------------------------------------------------------------------
void FASTCALL FDI::Adjust()
{
	FDIDisk *disk;
	FDIDiskD68 *disk68;

	ASSERT(this);

	// •¡”ƒCƒ[ƒW‚Ìê‡‚Ì‚İ
	if (!IsMulti()) {
		return;
	}

	disk = GetFirst();
	while (disk) {
		// D68‚Ìê‡‚Ì‚İ
		if (disk->GetID() == MAKEID('D', '6', '8', ' ')) {
			disk68 = (FDIDiskD68*)disk;
			disk68->AdjustOffset();
		}

		disk = disk->GetNext();
	}
}

//===========================================================================
//
//	FDIƒgƒ‰ƒbƒN(2HD)
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	ƒRƒ“ƒXƒgƒ‰ƒNƒ^
//
//---------------------------------------------------------------------------
FDITrack2HD::FDITrack2HD(FDIDisk *disk, int track) : FDITrack(disk, track)
{
	ASSERT(disk);
}

//---------------------------------------------------------------------------
//
//	ƒ[ƒh
//
//---------------------------------------------------------------------------
BOOL FASTCALL FDITrack2HD::Load(const Filepath& path, DWORD offset)
{
	Fileio fio;
	BYTE buf[0x400];
	DWORD chrn[4];
	int i;
	FDISector *sector;

	ASSERT(this);
	ASSERT((offset & 0x1fff) == 0);
	ASSERT(offset < 0x134000);

	// ‰Šú‰»Ï‚İ‚È‚ç•s—v(ƒV[ƒN–ˆ‚ÉŒÄ‚Î‚ê‚é‚Ì‚ÅA‚P“x‚¾‚¯“Ç‚ñ‚ÅƒLƒƒƒbƒVƒ…‚·‚é)
	if (IsInit()) {
		return TRUE;
	}

	// ƒZƒNƒ^‚ª‘¶İ‚µ‚È‚¢‚±‚Æ
	ASSERT(!GetFirst());
	ASSERT(GetAllSectors() == 0);
	ASSERT(GetMFMSectors() == 0);
	ASSERT(GetFMSectors() == 0);

	// CEHENŒˆ’è
	chrn[0] = GetTrack() >> 1;
	chrn[1] = GetTrack() & 1;
	chrn[3] = 3;

	// “Ç‚İ‚İƒI[ƒvƒ“
	if (!fio.Open(path, Fileio::ReadOnly)) {
		return FALSE;
	}

	// ƒV[ƒN
	if (!fio.Seek(offset)) {
		fio.Close();
		return FALSE;
	}

	// ƒ‹[ƒv
	for (i=0; i<8; i++) {
		// ƒf[ƒ^“Ç‚İ‚İ
		if (!fio.Read(buf, sizeof(buf))) {
			// “r’†‚Ü‚Å’Ç‰Á‚µ‚½•ª‚ğíœ‚·‚é
			ClrSector();
			fio.Close();
			return FALSE;
		}

		// ƒZƒNƒ^ì¬
		chrn[2] = i + 1;
		sector = new FDISector(TRUE, chrn);
		sector->Load(buf, sizeof(buf), 0x74, FDD_NOERROR);

		// ƒZƒNƒ^’Ç‰Á
		AddSector(sector);
	}

	// ƒNƒ[ƒY
	fio.Close();

	// ƒ|ƒWƒVƒ‡ƒ“ŒvZ
	CalcPos();

	// ‰Šú‰»ok
	trk.init = TRUE;
	return TRUE;
}

//===========================================================================
//
//	FDIƒfƒBƒXƒN(2HD)
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	ƒRƒ“ƒXƒgƒ‰ƒNƒ^
//
//---------------------------------------------------------------------------
FDIDisk2HD::FDIDisk2HD(int index, FDI *fdi) : FDIDisk(index, fdi)
{
	// IDİ’è
	disk.id = MAKEID('2', 'H', 'D', ' ');
}

//---------------------------------------------------------------------------
//
//	ƒfƒXƒgƒ‰ƒNƒ^
//
//---------------------------------------------------------------------------
FDIDisk2HD::~FDIDisk2HD()
{
	int i;
	DWORD offset;
	FDITrack *track;

	// ÅŒã‚Ìƒgƒ‰ƒbƒNƒf[ƒ^‚ğ‘‚«‚Ş
	for (i=0; i<2; i++) {
		// ƒgƒ‰ƒbƒN‚ª‚ ‚é‚©
		track = GetHead(i);
		if (!track) {
			continue;
		}

		// ƒgƒ‰ƒbƒNƒiƒ“ƒo‚©‚çAƒIƒtƒZƒbƒg‚ğZo(x8KB)
		offset = track->GetTrack();
		offset <<= 13;

		// ‘‚«‚İ
		track->Save(disk.path, offset);
		disk.head[i] = NULL;
	}
}

//---------------------------------------------------------------------------
//
//	ƒI[ƒvƒ“
//
//---------------------------------------------------------------------------
BOOL FASTCALL FDIDisk2HD::Open(const Filepath& path, DWORD offset)
{
	Fileio fio;
	DWORD size;
	FDITrack2HD *track;
	int i;

	ASSERT(this);
	ASSERT(offset == 0);
	ASSERT(!GetFirst());
	ASSERT(!GetHead(0));
	ASSERT(!GetHead(1));

	// ‘‚«‚İ‰Â”\‚Æ‚µ‚Ä‰Šú‰»
	disk.writep = FALSE;
	disk.readonly = FALSE;

	// ƒI[ƒvƒ“‚Å‚«‚é‚±‚Æ‚ğŠm‚©‚ß‚é
	if (!fio.Open(path, Fileio::ReadWrite)) {
		// “Ç‚İ‚İƒI[ƒvƒ“‚ğ‚İ‚é
		if (!fio.Open(path, Fileio::ReadOnly)) {
			return FALSE;
		}

		// “Ç‚İ‚İ‚Í‰Â”
		disk.writep = TRUE;
		disk.readonly = TRUE;
	}

	// ƒtƒ@ƒCƒ‹ƒTƒCƒY‚ª1261568‚Å‚ ‚é‚±‚Æ‚ğŠm‚©‚ß‚é
	size = fio.GetFileSize();
	if (size != 1261568) {
		fio.Close();
		return FALSE;
	}
	fio.Close();

	// ƒpƒXAƒIƒtƒZƒbƒg‚ğ‹L‰¯
	disk.path = path;
	disk.offset = offset;

	// ƒfƒBƒXƒN–¼‚Íƒtƒ@ƒCƒ‹–¼{Šg’£q‚Æ‚·‚é
	strcpy(disk.name, path.GetShort());

	// ƒgƒ‰ƒbƒN‚ğì¬(0`76ƒVƒŠƒ“ƒ_‚Ü‚ÅA77*2ƒgƒ‰ƒbƒN)
	for (i=0; i<154; i++) {
		track = new FDITrack2HD(this, i);
		AddTrack(track);
	}

	// I—¹
	return TRUE;
}

//---------------------------------------------------------------------------
//
//	ƒV[ƒN
//
//---------------------------------------------------------------------------
void FASTCALL FDIDisk2HD::Seek(int c)
{
	int i;
	FDITrack2HD *track;
	DWORD offset;

	ASSERT(this);
	ASSERT((c >= 0) && (c < 82));

	// ƒgƒ‰ƒbƒNƒf[ƒ^‚ğ‘‚«‚Ş
	for (i=0; i<2; i++) {
		// ƒgƒ‰ƒbƒN‚ª‚ ‚é‚©
		track = (FDITrack2HD*)GetHead(i);
		if (!track) {
			continue;
		}

		// ƒgƒ‰ƒbƒNƒiƒ“ƒo‚©‚çAƒIƒtƒZƒbƒg‚ğZo(x8KB)
		offset = track->GetTrack();
		offset <<= 13;

		// ‘‚«‚İ
		track->Save(disk.path, offset);
	}

	// c‚Í75‚Ü‚Å‹–‰ÂB”ÍˆÍŠO‚Å‚ ‚ê‚Îhead[i]=NULL‚Æ‚·‚é
	if ((c < 0) || (c > 76)) {
		disk.head[0] = NULL;
		disk.head[1] = NULL;
		return;
	}

	// ŠY“–‚·‚éƒgƒ‰ƒbƒN‚ğŒŸõ‚µAƒ[ƒh
	for (i=0; i<2; i++) {
		// ƒgƒ‰ƒbƒN‚ğŒŸõ
		track = (FDITrack2HD*)Search(c * 2 + i);
		ASSERT(track);
		disk.head[i] = track;

		// ƒgƒ‰ƒbƒNƒiƒ“ƒo‚©‚çAƒIƒtƒZƒbƒg‚ğZo(x8KB)
		offset = track->GetTrack();
		offset <<= 13;

		// ƒ[ƒh
		track->Load(disk.path, offset);
	}
}

//---------------------------------------------------------------------------
//
//	V‹KƒfƒBƒXƒNì¬
//
//---------------------------------------------------------------------------
BOOL FASTCALL FDIDisk2HD::Create(const Filepath& path, const option_t *opt)
{
	int i;
	FDITrack2HD *track;
	DWORD offset;
	Fileio fio;

	ASSERT(this);
	ASSERT(opt);

	// •¨—ƒtƒH[ƒ}ƒbƒg‚Í2HD‚Ì‚İ‹–‰Â
	if (opt->phyfmt != FDI_2HD) {
		return FALSE;
	}

	// ƒtƒ@ƒCƒ‹ì¬‚ğ‚İ‚é
	if (!fio.Open(path, Fileio::WriteOnly)) {
		return FALSE;
	}

	// ‘‚«‚İ‰Â”\‚Æ‚µ‚Ä‰Šú‰»
	disk.writep = FALSE;
	disk.readonly = FALSE;

	// ƒpƒX–¼AƒIƒtƒZƒbƒg‚ğ‹L˜^
	disk.path = path;
	disk.offset = 0;

	// ƒfƒBƒXƒN–¼‚Íƒtƒ@ƒCƒ‹–¼{Šg’£q‚Æ‚·‚é
	strcpy(disk.name, path.GetShort());

	// 0`153‚ÉŒÀ‚èAƒgƒ‰ƒbƒN‚ğì¬‚µ‚Ä•¨—ƒtƒH[ƒ}ƒbƒg
	for (i=0; i<154; i++) {
		track = new FDITrack2HD(this, i);
		track->Create(opt->phyfmt);
		AddTrack(track);
	}

	// ˜_—ƒtƒH[ƒ}ƒbƒg
	FDIDisk::Create(path, opt);

	// ‘‚«‚İƒ‹[ƒv
	offset = 0;
	for (i=0; i<154; i++) {
		// ƒgƒ‰ƒbƒNæ“¾
		track = (FDITrack2HD*)Search(i);
		ASSERT(track);
		ASSERT(track->IsChanged());

		// ‘‚«‚İ
		if (!track->Save(&fio, offset)) {
			fio.Close();
			return FALSE;
		}

		// Ÿ‚Ö
		offset += (0x400 * 8);
	}

	// ¬Œ÷
	fio.Close();
	return TRUE;
}

//---------------------------------------------------------------------------
//
//	ƒtƒ‰ƒbƒVƒ…
//
//---------------------------------------------------------------------------
BOOL FASTCALL FDIDisk2HD::Flush()
{
	int i;
	DWORD offset;
	FDITrack *track;

	ASSERT(this);

	// ÅŒã‚Ìƒgƒ‰ƒbƒNƒf[ƒ^‚ğ‘‚«‚Ş
	for (i=0; i<2; i++) {
		// ƒgƒ‰ƒbƒN‚ª‚ ‚é‚©
		track = GetHead(i);
		if (!track) {
			continue;
		}

		// ƒgƒ‰ƒbƒNƒiƒ“ƒo‚©‚çAƒIƒtƒZƒbƒg‚ğZo(x8KB)
		offset = track->GetTrack();
		offset <<= 13;

		// ‘‚«‚İ
		if (!track->Save(disk.path, offset)) {
			return FALSE;
		}
	}

	return TRUE;
}

//===========================================================================
//
//	FDIƒgƒ‰ƒbƒN(DIM)
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	ƒRƒ“ƒXƒgƒ‰ƒNƒ^
//
//---------------------------------------------------------------------------
FDITrackDIM::FDITrackDIM(FDIDisk *disk, int track, int type) : FDITrack(disk, track)
{
	// ƒ^ƒCƒv‚É‰‚¶‚Ädim_mfm, dim_secs, dim_n‚ğŒˆ‚ß‚é
	switch (type) {
		// 2HD (N=3,8ƒZƒNƒ^)
		case 0:
			dim_mfm = TRUE;
			dim_secs = 8;
			dim_n = 3;
			break;

		// 2HS (N=3,9ƒZƒNƒ^)
		case 1:
			dim_mfm = TRUE;
			dim_secs = 9;
			dim_n = 3;
			break;

		// 2HC (N=2,15ƒZƒNƒ^)
		case 2:
			dim_mfm = TRUE;
			dim_secs = 15;
			dim_n = 2;
			break;

		// 2HDE (N=3,9ƒZƒNƒ^)
		case 3:
			dim_mfm = TRUE;
			dim_secs = 9;
			dim_n = 3;
			break;

		// 2HQ (N=2,18ƒZƒNƒ^)
		case 9:
			dim_mfm = TRUE;
			dim_secs = 18;
			dim_n = 2;
			break;

		// N88-BASIC (26ƒZƒNƒ^Aƒgƒ‰ƒbƒN0‚Ì‚İ’P–§)
		case 17:
			dim_secs = 26;
			if (track == 0) {
				dim_mfm = FALSE;
				dim_n = 0;
			}
			else {
				dim_mfm = TRUE;
				dim_n = 1;
			}
			break;

		// ‚»‚Ì‘¼
		default:
			ASSERT(FALSE);
			break;
	}

	dim_type = type;
}

//---------------------------------------------------------------------------
//
//	ƒ[ƒh
//
//---------------------------------------------------------------------------
BOOL FASTCALL FDITrackDIM::Load(const Filepath& path, DWORD offset, BOOL load)
{
	Fileio fio;
	BYTE buf[0x400];
	DWORD chrn[4];
	int i;
	int num;
	int len;
	int gap;
	FDISector *sector;

	ASSERT(this);

	// ‰Šú‰»Ï‚İ‚È‚ç•s—v(ƒV[ƒN–ˆ‚ÉŒÄ‚Î‚ê‚é‚Ì‚ÅA‚P“x‚¾‚¯“Ç‚ñ‚ÅƒLƒƒƒbƒVƒ…‚·‚é)
	if (IsInit()) {
		return TRUE;
	}

	// ƒZƒNƒ^‚ª‘¶İ‚µ‚È‚¢‚±‚Æ
	ASSERT(!GetFirst());
	ASSERT(GetAllSectors() == 0);
	ASSERT(GetMFMSectors() == 0);
	ASSERT(GetFMSectors() == 0);

	// CEHENŒˆ’è
	chrn[0] = GetTrack() >> 1;
	chrn[3] = GetDIMN();

	// “Ç‚İ‚İƒI[ƒvƒ“
	if (load) {
		if (!fio.Open(path, Fileio::ReadOnly)) {
			return FALSE;
		}

		// ƒV[ƒN
		if (!fio.Seek(offset)) {
			fio.Close();
			return FALSE;
		}
	}

	// ƒ[ƒh€”õ
	num = GetDIMSectors();
	len = 1 << (GetDIMN() + 7);
	ASSERT(len <= sizeof(buf));
	switch (GetDIMN()) {
		case 0:
			gap = 0x14;
			break;
		case 1:
			gap = 0x33;
			break;
		case 2:
			gap = 0x54;
			break;
		case 3:
			if (GetDIMSectors() == 8) {
				gap = 0x74;
			}
			else {
				gap = 0x39;
			}
			break;
		default:
			ASSERT(FALSE);
			fio.Close();
			return FALSE;
	}

	// ‰Šúƒf[ƒ^ì¬(load==FALSE‚Ìê‡)
	memset(buf, 0xe5, len);

	// ƒ‹[ƒv
	for (i=0; i<num; i++) {
		// ƒf[ƒ^“Ç‚İ‚İ
		if (load) {
			if (!fio.Read(buf, len)) {
				// “r’†‚Ü‚Å’Ç‰Á‚µ‚½•ª‚ğíœ‚·‚é
				ClrSector();
				fio.Close();
				return FALSE;
			}
		}

		// H‚ÆR‚ğŒˆ‚ß‚é (2HS, 2HDE‚Å“Á—á‚ ‚è)
		chrn[1] = GetTrack() & 1;
		chrn[2] = i + 1;
		if (dim_type == 1) {
			chrn[2] = i + 10;
			if ((GetTrack() == 0) && (i == 0)) {
				chrn[2] = 1;
			}
		}
		if (dim_type == 3) {
			chrn[1] = chrn[1] + 0x80;
			if ((GetTrack() == 0) && (i == 0)) {
				chrn[1] = 0;
			}
		}

		// ƒZƒNƒ^ì¬
		sector = new FDISector(IsDIMMFM(), chrn);
		sector->Load(buf, len, gap, FDD_NOERROR);

		// ƒZƒNƒ^’Ç‰Á
		AddSector(sector);
	}

	// ƒNƒ[ƒY
	if (load) {
		fio.Close();
	}

	// ƒ|ƒWƒVƒ‡ƒ“ŒvZ
	CalcPos();

	// ‰Šú‰»ok
	trk.init = TRUE;
	return TRUE;
}

//===========================================================================
//
//	FDIƒfƒBƒXƒN(DIM)
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	ƒRƒ“ƒXƒgƒ‰ƒNƒ^
//
//---------------------------------------------------------------------------
FDIDiskDIM::FDIDiskDIM(int index, FDI *fdi) : FDIDisk(index, fdi)
{
	// IDİ’è
	disk.id = MAKEID('D', 'I', 'M', ' ');

	// ƒwƒbƒ_‚ğƒNƒŠƒA
	memset(dim_hdr, 0, sizeof(dim_hdr));

	// ƒ[ƒh‚È‚µ
	dim_load = FALSE;
}

//---------------------------------------------------------------------------
//
//	ƒfƒXƒgƒ‰ƒNƒ^
//
//---------------------------------------------------------------------------
FDIDiskDIM::~FDIDiskDIM()
{
	// ƒ[ƒh‚³‚ê‚Ä‚¢‚ê‚Î‘‚«‚İ
	if (dim_load) {
		Save();
	}
}

//---------------------------------------------------------------------------
//
//	ƒI[ƒvƒ“
//
//---------------------------------------------------------------------------
BOOL FASTCALL FDIDiskDIM::Open(const Filepath& path, DWORD offset)
{
	Fileio fio;
	DWORD size;
	FDITrackDIM *track;
	int i;

	ASSERT(this);
	ASSERT(offset == 0);
	ASSERT(!GetFirst());
	ASSERT(!GetHead(0));
	ASSERT(!GetHead(1));

	// ‘‚«‚İ‰Â”\‚Æ‚µ‚Ä‰Šú‰»
	disk.writep = FALSE;
	disk.readonly = FALSE;

	// ƒI[ƒvƒ“‚Å‚«‚é‚±‚Æ‚ğŠm‚©‚ß‚é
	if (!fio.Open(path, Fileio::ReadWrite)) {
		// “Ç‚İ‚İƒI[ƒvƒ“‚ğ‚İ‚é
		if (!fio.Open(path, Fileio::ReadOnly)) {
			return FALSE;
		}

		// “Ç‚İ‚İ‚Í‰Â”
		disk.writep = TRUE;
		disk.readonly = TRUE;
	}

	// ƒtƒ@ƒCƒ‹ƒTƒCƒY‚ª256ˆÈã‚ ‚é‚±‚Æ‚ğŠm‚©‚ß‚é
	size = fio.GetFileSize();
	if (size < 0x100) {
		fio.Close();
		return FALSE;
	}

	// ƒwƒbƒ_“Ç‚İ‚İA”F¯•¶š—ñƒ`ƒFƒbƒN
	fio.Read(dim_hdr, sizeof(dim_hdr));
	fio.Close();
	if (strcmp((char*)&dim_hdr[171], "DIFC HEADER  ") != 0) {
		return FALSE;
	}

	// ƒpƒX–¼{ƒIƒtƒZƒbƒg‚ğ‹L˜^
	disk.path = path;
	disk.offset = offset;

	// ƒRƒƒ“ƒg‚ª‚ ‚é‚©
	if (dim_hdr[0xc2] != '\0') {
		// ƒfƒBƒXƒN–¼‚ÍƒRƒƒ“ƒg‚Æ‚·‚é(•K‚¸60•¶š‚ÅØ‚é)
		dim_hdr[0xc2 + 60] = '\0';
		strcpy(disk.name, (char*)&dim_hdr[0xc2]);
	}
	else {
		// ƒfƒBƒXƒN–¼‚Íƒtƒ@ƒCƒ‹–¼{Šg’£q‚Æ‚·‚é
		strcpy(disk.name, path.GetShort());
	}

	// ƒgƒ‰ƒbƒN‚ğì¬(0`81ƒVƒŠƒ“ƒ_‚Ü‚ÅA82*2ƒgƒ‰ƒbƒN)
	for (i=0; i<164; i++) {
		track = new FDITrackDIM(this, i, dim_hdr[0]);
		AddTrack(track);
	}

	// ƒtƒ‰ƒOUpAI—¹
	dim_load = TRUE;
	return TRUE;
}

//---------------------------------------------------------------------------
//
//	ƒV[ƒN
//
//---------------------------------------------------------------------------
void FASTCALL FDIDiskDIM::Seek(int c)
{
	int i;
	FDITrackDIM *track;
	DWORD offset;
	BOOL flag;

	ASSERT(this);
	ASSERT((c >= 0) && (c < 82));
	ASSERT(dim_load);

	// ŠY“–‚·‚éƒgƒ‰ƒbƒN‚ğŒŸõ‚µAƒ[ƒh
	for (i=0; i<2; i++) {
		// ƒgƒ‰ƒbƒN‚ğŒŸõ
		track = (FDITrackDIM*)Search(c * 2 + i);
		ASSERT(track);
		disk.head[i] = track;

		// ƒ}ƒbƒv‚ğŒ©‚ÄA—LŒøƒgƒ‰ƒbƒN‚È‚çƒ[ƒh
		flag = FALSE;
		offset = 0;
		if (GetDIMMap(c * 2 + i)) {
			// ƒIƒtƒZƒbƒgŒvZ
			offset = GetDIMOffset(c * 2 + i);
			flag = TRUE;
		}

		// ƒ[ƒh‚Ü‚½‚Íì¬
		track->Load(disk.path, offset, flag);
	}
}

//---------------------------------------------------------------------------
//
//	DIMƒgƒ‰ƒbƒNƒ}ƒbƒv‚ğæ“¾
//
//---------------------------------------------------------------------------
BOOL FASTCALL FDIDiskDIM::GetDIMMap(int track) const
{
	ASSERT(this);
	ASSERT((track >= 0) && (track <= 163));
	ASSERT(dim_load);

	if (dim_hdr[track + 1] != 0) {
		return TRUE;
	}
	return FALSE;
}

//---------------------------------------------------------------------------
//
//	DIMƒgƒ‰ƒbƒNƒIƒtƒZƒbƒg‚ğæ“¾
//
//---------------------------------------------------------------------------
DWORD FASTCALL FDIDiskDIM::GetDIMOffset(int track) const
{
	int i;
	DWORD offset;
	int length;
	FDITrackDIM *dim;

	ASSERT(this);
	ASSERT((track >= 0) && (track <= 163));
	ASSERT(dim_load);

	// ƒx[ƒX‚Í256
	offset = 0x100;

	// ‘O‚ÌƒZƒNƒ^‚Ü‚Å‚Ì‡Z‚Æ‚·‚é
	for (i=0; i<track; i++) {
		// —LŒøƒgƒ‰ƒbƒN‚È‚ç‡Z
		if (GetDIMMap(i)) {
			dim = (FDITrackDIM*)Search(track);
			ASSERT(dim);
			length = 1 << (dim->GetDIMN() + 7);
			length *= dim->GetDIMSectors();
			offset += length;
		}
	}

	return offset;
}

//---------------------------------------------------------------------------
//
//	ƒZ[ƒu
//
//---------------------------------------------------------------------------
BOOL FASTCALL FDIDiskDIM::Save()
{
	BOOL changed;
	int i;
	FDITrackDIM *track;
	DWORD offset;
	Fileio fio;
	DWORD total;
	BYTE *ptr;

	ASSERT(this);
	ASSERT(dim_load);

	// ƒ}ƒbƒvˆÈŠO‚Ìƒgƒ‰ƒbƒN‚Å•ÏX‚³‚ê‚½‚à‚Ì‚ª‚ ‚é‚©A‚Ü‚¸’²‚×‚é
	changed = FALSE;
	for (i=0; i<164; i++) {
		if (!GetDIMMap(i)) {
			// ƒ}ƒbƒv‚É‚È‚¢ƒgƒ‰ƒbƒNB•ÏX‚ ‚é‚©
			track = (FDITrackDIM*)Search(i);
			ASSERT(track);
			if (track->IsChanged()) {
				changed = TRUE;
			}
		}
	}

	// •ÏX‚ªƒ}ƒbƒv‚Ì‚İ‚È‚çAŒÂ•Ê‘Î‰
	if (!changed) {
		for (i=0; i<164; i++) {
			track = (FDITrackDIM*)Search(i);
			ASSERT(track);
			if (track->IsChanged()) {
				// ƒ}ƒbƒvÏ‚İ‚Ìƒgƒ‰ƒbƒN
				ASSERT(GetDIMMap(i));
				offset = GetDIMOffset(i);
				if (!track->Save(disk.path, offset)) {
					return FALSE;
				}
			}
		}
		// ‘Sƒgƒ‰ƒbƒNI—¹
		return TRUE;
	}

	// ƒ}ƒbƒvÏ‚İƒgƒ‰ƒbƒN‚ğ‚·‚×‚Äƒ[ƒh
	for (i=0; i<164; i++) {
		if (GetDIMMap(i)) {
			track = (FDITrackDIM*)Search(i);
			ASSERT(track);
			offset = GetDIMOffset(i);
			if (!track->Load(disk.path, offset, TRUE)) {
				return FALSE;
			}
		}
	}

	// ƒ}ƒbƒv‚³‚ê‚Ä‚¢‚È‚­‚Ä¡‰ñ’Ç‰Á‚³‚ê‚½‚Æ‚±‚ë‚ğƒ}ƒbƒvB“¯‚Éƒg[ƒ^ƒ‹ƒTƒCƒY‚ğ“¾‚é
	total = 0;
	for (i=0; i<164; i++) {
		track = (FDITrackDIM*)Search(i);
		ASSERT(track);
		if (!GetDIMMap(i)) {
			if (track->IsChanged()) {
				// V‚µ‚­ƒ}ƒbƒv‚É’Ç‰Á
				dim_hdr[i + 1] = 0x01;
				total += track->GetTotalLength();
			}
		}
		else {
			// Šù‚Éƒ}ƒbƒv‚³‚ê‚Ä‚¢‚é
			total += track->GetTotalLength();
		}
	}

	// 2HD‚Å154ƒgƒ‰ƒbƒNˆÈ~‚ªg‚í‚ê‚Ä‚¢‚ê‚ÎAOverTrackƒtƒ‰ƒO‚ğ—§‚Ä‚é
	dim_hdr[0xff] = 0x00;
	if (dim_hdr[0] == 0x00) {
		for (i=154; i<164; i++) {
			if (GetDIMMap(i)) {
				dim_hdr[0xff] = 0x01;
			}
		}
	}

	// V‹KƒZ[ƒu(ƒTƒCƒY‚ğ‚Â‚­‚é)
	if (!fio.Open(disk.path, Fileio::WriteOnly)) {
		return FALSE;
	}
	if (!fio.Write(dim_hdr, sizeof(dim_hdr))) {
		fio.Close();
		return FALSE;
	}

	// 256ƒoƒCƒg‚Ìƒwƒbƒ_ˆÈ~‚ÍAE5ƒf[ƒ^‚ğÅ‰‚ÉƒZ[ƒu
	try {
		ptr = new BYTE[total];
	}
	catch (...) {
		fio.Close();
		return FALSE;
	}
	if (!ptr) {
		fio.Close();
		return FALSE;
	}
	memset(ptr, 0xe5, total);
	if (!fio.Write(ptr, total)) {
		fio.Close();
		delete[] ptr;
		return FALSE;
	}
	delete[] ptr;

	// ‘S‚Ä‘‚«‚Ş(ƒtƒ@ƒCƒ‹‚ÍŠJ‚¢‚½‚Ü‚Ü)
	for (i=0; i<164; i++) {
		if (GetDIMMap(i)) {
			track = (FDITrackDIM*)Search(i);
			ASSERT(track);
			offset = GetDIMOffset(i);
			track->ForceChanged();
			if (!track->Save(&fio, offset)) {
				return FALSE;
			}
		}
	}

	fio.Close();
	return TRUE;
}

//---------------------------------------------------------------------------
//
//	V‹KƒfƒBƒXƒNì¬
//
//---------------------------------------------------------------------------
BOOL FASTCALL FDIDiskDIM::Create(const Filepath& path, const option_t *opt)
{
	int i;
	FDITrackDIM *track;
	Fileio fio;
	static BYTE iocsdata[] = {
		0x04, 0x21, 0x03, 0x22, 0x01, 0x00, 0x00, 0x00
	};

	ASSERT(this);
	ASSERT(opt);

	// ƒwƒbƒ_‚ğƒNƒŠƒA
	memset(dim_hdr, 0, sizeof(dim_hdr));

	// ƒtƒH[ƒ}ƒbƒg‚Ìƒ`ƒFƒbƒN‚Æƒ^ƒCƒv‘‚«‚İ
	switch (opt->phyfmt) {
		// 2HD(ƒI[ƒo[ƒgƒ‰ƒbƒNg—p‚ğŠÜ‚Ş)
		case FDI_2HD:
		case FDI_2HDA:
			dim_hdr[0] = 0x00;
			break;

		// 2HS
		case FDI_2HS:
			dim_hdr[0] = 0x01;
			break;

		// 2HC
		case FDI_2HC:
			dim_hdr[0] = 0x02;
			break;

		// 2HDE(68)
		case FDI_2HDE:
			dim_hdr[0] = 0x03;
			break;

		// 2HQ
		case FDI_2HQ:
			dim_hdr[0] = 0x09;
			break;

		// N88-BASIC
		case FDI_N88B:
			dim_hdr[0] = 0x11;
			break;

		// ƒTƒ|[ƒg‚µ‚Ä‚¢‚È‚¢•¨—ƒtƒH[ƒ}ƒbƒg
		default:
			return FALSE;
	}

	// ƒwƒbƒ_c‚è(“ú•t‚Í2001-03-22 00:00:00‚Æ‚·‚é; XM6ŠJ”­ŠJn“ú)
	strcpy((char*)&dim_hdr[0xab], "DIFC HEADER  ");
	dim_hdr[0xfe] = 0x19;
	if (opt->phyfmt == FDI_2HDA) {
		dim_hdr[0xff] = 0x01;
	}
	memcpy(&dim_hdr[0xba], iocsdata, 8);
	ASSERT(strlen(opt->name) < 60);
	strcpy((char*)&dim_hdr[0xc2], opt->name);

	// ƒwƒbƒ_‘‚«‚İ
	if (!fio.Open(path, Fileio::WriteOnly)) {
		return FALSE;
	}
	if (!fio.Write(&dim_hdr[0], sizeof(dim_hdr))) {
		return FALSE;
	}
	fio.Close();

	// ƒtƒ‰ƒOİ’è
	disk.writep = FALSE;
	disk.readonly = FALSE;
	dim_load = TRUE;

	// ƒpƒX–¼{ƒIƒtƒZƒbƒg‚ğ‹L˜^
	disk.path = path;
	disk.offset = 0;

	// ƒfƒBƒXƒN–¼‚Íƒtƒ@ƒCƒ‹–¼{Šg’£q‚Æ‚·‚é
	strcpy(disk.name, path.GetShort());

	// ƒgƒ‰ƒbƒN‚ğì¬‚µ‚Ä•¨—ƒtƒH[ƒ}ƒbƒg
	for (i=0; i<164; i++) {
		track = new FDITrackDIM(this, i, dim_hdr[0x00]);
		track->Create(opt->phyfmt);
		AddTrack(track);
	}

	// ˜_—ƒtƒH[ƒ}ƒbƒg
	FDIDisk::Create(path, opt);

	// •Û‘¶
	if (!Save()) {
		return FALSE;
	}

	// ¬Œ÷
	return TRUE;
}

//---------------------------------------------------------------------------
//
//	ƒtƒ‰ƒbƒVƒ…
//
//---------------------------------------------------------------------------
BOOL FASTCALL FDIDiskDIM::Flush()
{
	ASSERT(this);

	// ƒ[ƒh‚³‚ê‚Ä‚¢‚ê‚Î‘‚«‚İ
	if (dim_load) {
		return Save();
	}

	// ƒ[ƒh‚³‚ê‚Ä‚¢‚È‚¢
	return TRUE;
}

//===========================================================================
//
//	FDIƒgƒ‰ƒbƒN(D68)
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	ƒRƒ“ƒXƒgƒ‰ƒNƒ^
//
//---------------------------------------------------------------------------
FDITrackD68::FDITrackD68(FDIDisk *disk, int track, BOOL hd) : FDITrack(disk, track, hd)
{
	// ƒtƒH[ƒ}ƒbƒg•ÏX‚È‚µ
	d68_format = FALSE;
}

//---------------------------------------------------------------------------
//
//	ƒ[ƒh
//
//---------------------------------------------------------------------------
BOOL FASTCALL FDITrackD68::Load(const Filepath& path, DWORD offset)
{
	Fileio fio;
	BYTE header[0x10];
	DWORD chrn[4];
	BYTE buf[0x2000];
	BOOL mfm;
	int len;
	int i;
	int num;
	int gap;
	int stat;
	FDISector *sector;
	BYTE *ptr;
	const int *table;

	ASSERT(this);
	ASSERT(offset > 0);

	// ‰Šú‰»Ï‚İ‚È‚ç•s—v(ƒV[ƒN–ˆ‚ÉŒÄ‚Î‚ê‚é‚Ì‚ÅA‚P“x‚¾‚¯“Ç‚ñ‚ÅƒLƒƒƒbƒVƒ…‚·‚é)
	if (IsInit()) {
		return TRUE;
	}

	// ƒZƒNƒ^‚ª‘¶İ‚µ‚È‚¢‚±‚Æ
	ASSERT(!GetFirst());
	ASSERT(GetAllSectors() == 0);
	ASSERT(GetMFMSectors() == 0);
	ASSERT(GetFMSectors() == 0);

	// ƒI[ƒvƒ“‚ÆƒV[ƒN
	if (!fio.Open(path, Fileio::ReadOnly)) {
		return FALSE;
	}
	if (!fio.Seek(offset)) {
		return FALSE;
	}

	// Å’á1‚Â‚ÍƒZƒNƒ^‚ª‚ ‚é‚Æ‰¼’è(ƒIƒtƒZƒbƒg!=0‚Ì‚½‚ß)
	i = 0;
	num = 1;
	while (i < num) {
		// ƒwƒbƒ_“Ç‚İ
		if (!fio.Read(header, sizeof(header))) {
			break;
		}

		// ‰‰ñ‚ÍƒZƒNƒ^”‚ğæ“¾
		if (i == 0) {
			ptr = &header[0x04];
			num = (int)ptr[1];
			num <<= 8;
			num |= (int)ptr[0];
		}

		// MFM
		mfm = TRUE;
		if (header[0x06] != 0) {
			mfm = FALSE;
		}

		// ƒŒƒ“ƒOƒX
		ptr = &header[0x0e];
		len = (int)ptr[1];
		len <<= 8;
		len |= (int)ptr[0];

		// GAP3(D68ƒtƒ@ƒCƒ‹‚É‚Íî•ñ‚ª‚È‚¢‚Ì‚ÅA‚æ‚­‚ ‚éƒpƒ^[ƒ“‚ğ—pˆÓ)
		gap = 0x12;
		table = &Gap3Table[0];
		while (table[0] != 0) {
			// GAPƒe[ƒuƒ‹ŒŸõ
			if ((table[0] == num) && (table[1] == (int)header[3])) {
				gap = table[2];
				break;
			}
			table += 3;
		}

		// DELETED SECTOR‚ğŠÜ‚ŞƒXƒe[ƒ^ƒX
		stat = FDD_NOERROR;
		if (header[0x07] != 0) {
			stat |= FDD_DDAM;
		}
		if ((header[0x08] & 0xf0) == 0xa0) {
			stat |= FDD_DATACRC;
		}

		// ƒoƒbƒtƒ@‚Öƒf[ƒ^‚ğƒ[ƒh
		if (sizeof(buf) < len) {
			break;
		}
		if (!fio.Read(buf, len)) {
			break;
		}

		// ƒZƒNƒ^ì¬
		chrn[0] = (DWORD)header[0];
		chrn[1] = (DWORD)header[1];
		chrn[2] = (DWORD)header[2];
		chrn[3] = (DWORD)header[3];
		sector = new FDISector(mfm, chrn);
		sector->Load(buf, len, gap, stat);

		// Ÿ‚Ö
		AddSector(sector);
		i++;
	}

	fio.Close();

	// ƒ|ƒWƒVƒ‡ƒ“ŒvZ
	CalcPos();

	// ‰Šú‰»ok
	trk.init = TRUE;
	return TRUE;
}

//---------------------------------------------------------------------------
//
//	ƒZ[ƒu
//
//---------------------------------------------------------------------------
BOOL FASTCALL FDITrackD68::Save(const Filepath& path, DWORD offset)
{
	FDISector *sector;
	BYTE header[0x10];
	DWORD chrn[4];
	Fileio fio;
	int secs;
	int len;
	BYTE *ptr;

	ASSERT(this);
	ASSERT(offset > 0);

	// ƒZƒNƒ^‰Šú‰»
	sector = GetFirst();

	while (sector) {
		if (!sector->IsChanged()) {
			// •ÏX‚³‚ê‚Ä‚¢‚È‚¢‚Ì‚ÅƒXƒLƒbƒv
			offset += 0x10;
			offset += sector->GetLength();
			sector = sector->GetNext();
			continue;
		}

		// •ÏX‚³‚ê‚Ä‚¢‚éBƒwƒbƒ_‚ğì‚é
		memset(header, 0, sizeof(header));
		sector->GetCHRN(chrn);
		header[0] = (BYTE)chrn[0];
		header[1] = (BYTE)chrn[1];
		header[2] = (BYTE)chrn[2];
		header[3] = (BYTE)chrn[3];
		secs = GetAllSectors();
		ptr = &header[0x04];
		ptr[1] = (BYTE)(secs >> 8);
		ptr[0] = (BYTE)secs;
		if (!sector->IsMFM()) {
			header[0x06] = 0x40;
		}
		if (sector->GetError() & FDD_DDAM) {
			header[0x07] = 0x10;
		}
		if (sector->GetError() & FDD_IDCRC) {
			header[0x08] = 0xa0;
		}
		if (sector->GetError() & FDD_DATACRC) {
			header[0x08] = 0xa0;
		}
		ptr = &header[0x0e];
		len = sector->GetLength();
		ptr[1] = (BYTE)(len >> 8);
		ptr[0] = (BYTE)len;

		// ‘‚«‚İ
		if (!fio.IsValid()) {
			// ‰‰ñ‚È‚çƒtƒ@ƒCƒ‹ƒI[ƒvƒ“
			if (!fio.Open(path, Fileio::ReadWrite)) {
				return FALSE;
			}
		}
		if (!fio.Seek(offset)) {
			fio.Close();
			return FALSE;
		}
		if (!fio.Write(header, sizeof(header))) {
			fio.Close();
			return FALSE;
		}
		if (!fio.Write(sector->GetSector(), sector->GetLength())) {
			fio.Close();
			return FALSE;
		}

		// ‘‚«‚İŠ®—¹
		sector->ClrChanged();

		// Ÿ‚Ö
		offset += 0x10;
		offset += sector->GetLength();
		sector = sector->GetNext();
	}

	// —LŒø‚È‚çƒtƒ@ƒCƒ‹ƒNƒ[ƒY
	if (fio.IsValid()) {
		fio.Close();
	}

	// ƒtƒH[ƒ}ƒbƒgƒtƒ‰ƒO‚à~‚ë‚µ‚Ä‚¨‚­
	d68_format = FALSE;
	return TRUE;
}

//---------------------------------------------------------------------------
//
//	ƒZ[ƒu
//
//---------------------------------------------------------------------------
BOOL FASTCALL FDITrackD68::Save(Fileio *fio, DWORD offset)
{
	FDISector *sector;
	BYTE header[0x10];
	DWORD chrn[4];
	int secs;
	int len;
	BYTE *ptr;

	ASSERT(this);
	ASSERT(fio);
	ASSERT(offset > 0);

	// ƒZƒNƒ^‰Šú‰»
	sector = GetFirst();

	while (sector) {
		if (!sector->IsChanged()) {
			// •ÏX‚³‚ê‚Ä‚¢‚È‚¢‚Ì‚ÅƒXƒLƒbƒv
			offset += 0x10;
			offset += sector->GetLength();
			sector = sector->GetNext();
			continue;
		}

		// •ÏX‚³‚ê‚Ä‚¢‚éBƒwƒbƒ_‚ğì‚é
		memset(header, 0, sizeof(header));
		sector->GetCHRN(chrn);
		header[0] = (BYTE)chrn[0];
		header[1] = (BYTE)chrn[1];
		header[2] = (BYTE)chrn[2];
		header[3] = (BYTE)chrn[3];
		secs = GetAllSectors();
		ptr = &header[0x04];
		ptr[1] = (BYTE)(secs >> 8);
		ptr[0] = (BYTE)secs;
		if (!sector->IsMFM()) {
			header[0x06] = 0x40;
		}
		if (sector->GetError() & FDD_DDAM) {
			header[0x07] = 0x10;
		}
		if (sector->GetError() & FDD_IDCRC) {
			header[0x08] = 0xa0;
		}
		if (sector->GetError() & FDD_DATACRC) {
			header[0x08] = 0xa0;
		}
		ptr = &header[0x0e];
		len = sector->GetLength();
		ptr[1] = (BYTE)(len >> 8);
		ptr[0] = (BYTE)len;

		// ‘‚«‚İ
		fio->Seek(offset);
		if (!fio->Write(header, sizeof(header))) {
			return FALSE;
		}
		if (!fio->Write(sector->GetSector(), sector->GetLength())) {
			return FALSE;
		}

		// ‘‚«‚İŠ®—¹
		sector->ClrChanged();

		// Ÿ‚Ö
		offset += 0x10;
		offset += sector->GetLength();
		sector = sector->GetNext();
	}

	// ƒtƒH[ƒ}ƒbƒgƒtƒ‰ƒO‚à~‚ë‚µ‚Ä‚¨‚­
	d68_format = FALSE;
	return TRUE;
}

//---------------------------------------------------------------------------
//
//	ƒ‰ƒCƒgID
//	¦Ÿ‚ÌƒXƒe[ƒ^ƒX‚ğ•Ô‚·(ƒGƒ‰[‚ÍOR‚³‚ê‚é)
//		FDD_NOERROR		ƒGƒ‰[‚È‚µ
//		FDD_NOTWRITE	‘‚«‚İ‹Ö~
//
//---------------------------------------------------------------------------
int FASTCALL FDITrackD68::WriteID(const BYTE *buf, DWORD d, int sc, BOOL mfm, int gpl)
{
	int stat;
	FDISector *sector;
	DWORD pos;
	int i;
	BYTE fillbuf[0x2000];
	DWORD chrn[4];

	ASSERT(this);
	ASSERT(sc > 0);

	// ƒIƒŠƒWƒiƒ‹‚ğŒÄ‚Ô(ƒ‰ƒCƒgƒvƒƒeƒNƒg‚Ìƒ`ƒFƒbƒN‚ÍFDIDisk‚ÅŠù‚És‚í‚ê‚Ä‚¢‚é)
	stat = FDITrack::WriteID(buf, d, sc, mfm, gpl);
	if (stat == FDD_NOERROR) {
		// ƒtƒH[ƒ}ƒbƒg¬Œ÷(ˆÈ‘O‚Æ“¯ˆê‚Ì•¨—ƒtƒH[ƒ}ƒbƒg)
		return stat;
	}

	// ˆÙ‚È‚éƒtƒH[ƒ}ƒbƒg
	d68_format = TRUE;

	// ŠÔ‚ğİ’è(index‚Ü‚Å)
	pos = GetDisk()->GetRotationTime();
	pos -= GetDisk()->GetRotationPos();
	GetDisk()->SetSearch(pos);

	// buf‚ª—^‚¦‚ç‚ê‚Ä‚¢‚È‚¯‚ê‚Î‚±‚±‚Ü‚Å
	if (!buf) {
		return FDD_NOERROR;
	}

	// ƒZƒNƒ^‚ğƒNƒŠƒA
	ClrSector();
	memset(fillbuf, d, sizeof(fillbuf));

	// ‡‚ÉƒZƒNƒ^‚ğì¬
	for (i=0; i<sc; i++) {
		// ƒŒƒ“ƒOƒX>=7‚ÍƒAƒ“ƒtƒH[ƒ}ƒbƒg
		if (buf[i * 4 + 3] >= 0x07) {
			ClrSector();
			return FDD_NOERROR;
		}

		// ƒZƒNƒ^‚ğì¬
		chrn[0] = (DWORD)buf[i * 4 + 0];
		chrn[1] = (DWORD)buf[i * 4 + 1];
		chrn[2] = (DWORD)buf[i * 4 + 2];
		chrn[3] = (DWORD)buf[i * 4 + 3];
		sector = new FDISector(mfm, chrn);
		sector->Load(fillbuf, 1 << (buf[i * 4 + 3] + 7), gpl, FDD_NOERROR);

		// ƒZƒNƒ^‚ğ’Ç‰Á
		AddSector(sector);
	}

	// ƒ|ƒWƒVƒ‡ƒ“ŒvZ
	CalcPos();

	return FDD_NOERROR;
}

//---------------------------------------------------------------------------
//
//	D68‚Å‚Ì’·‚³æ“¾
//
//---------------------------------------------------------------------------
DWORD FASTCALL FDITrackD68::GetD68Length() const
{
	DWORD length;
	FDISector *sector;

	ASSERT(this);

	// ‰Šú‰»
	length = 0;
	sector = GetFirst();

	// ƒ‹[ƒv
	while (sector) {
		length += 0x10;
		length += sector->GetLength();
		sector = sector->GetNext();
	}

	return length;
}

//---------------------------------------------------------------------------
//
//	GAP3ƒe[ƒuƒ‹
//
//---------------------------------------------------------------------------
const int FDITrackD68::Gap3Table[] = {
	// SEC, N, GAP3
	 8, 3, 0x74,						// 2HD
	 9, 3, 0x39,						// 2HS, 2HDE
	15, 2, 0x54,						// 2HC
	18, 2, 0x54,						// 2HQ
	26, 1, 0x33,						// OS-9/68000, N88-BASIC
	26, 0, 0x1a,						// N88-BASIC
	 9, 2, 0x54,						// 2DD(720KB)
	 8, 2, 0x54,						// 2DD(640KB)
	 5, 3, 0x74,						// 2D(Falcom)
	16, 1, 0x33,						// 2D(BASIC)
	 0, 0, 0
};

//===========================================================================
//
//	FDIƒfƒBƒXƒN(D68)
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	ƒRƒ“ƒXƒgƒ‰ƒNƒ^
//
//---------------------------------------------------------------------------
FDIDiskD68::FDIDiskD68(int index, FDI *fdi) : FDIDisk(index, fdi)
{
	// IDİ’è
	disk.id = MAKEID('D', '6', '8', ' ');

	// ƒwƒbƒ_‚ğƒNƒŠƒA
	memset(d68_hdr, 0, sizeof(d68_hdr));

	// ƒ[ƒh‚È‚µ
	d68_load = FALSE;
}

//---------------------------------------------------------------------------
//
//	ƒfƒXƒgƒ‰ƒNƒ^
//
//---------------------------------------------------------------------------
FDIDiskD68::~FDIDiskD68()
{
	// ƒ[ƒh‚³‚ê‚Ä‚¢‚ê‚Î‘‚«‚İ
	if (d68_load) {
		Save();
	}
}

//---------------------------------------------------------------------------
//
//	ƒI[ƒvƒ“
//
//---------------------------------------------------------------------------
BOOL FASTCALL FDIDiskD68::Open(const Filepath& path, DWORD offset)
{
	Fileio fio;
	int i;
	FDITrackD68 *track;
	BOOL hd;

	ASSERT(this);
	ASSERT(!GetFirst());
	ASSERT(!GetHead(0));
	ASSERT(!GetHead(1));

	// ‘‚«‚İ‰Â”\‚Æ‚µ‚Ä‰Šú‰»
	disk.writep = FALSE;
	disk.readonly = FALSE;

	// ƒI[ƒvƒ“‚Å‚«‚é‚±‚Æ‚ğŠm‚©‚ß‚é
	if (!fio.Open(path, Fileio::ReadWrite)) {
		// “Ç‚İ‚İƒI[ƒvƒ“‚ğ‚İ‚é
		if (!fio.Open(path, Fileio::ReadOnly)) {
			return FALSE;
		}

		// “Ç‚İ‚İ‚Í‰Â”
		disk.writep = TRUE;
		disk.readonly = TRUE;
	}

	// ƒV[ƒNAƒwƒbƒ_“Ç‚İ‚İ
	if (!fio.Seek(offset)) {
		fio.Close();
		return FALSE;
	}
	if (!fio.Read(d68_hdr, sizeof(d68_hdr))) {
		fio.Close();
		return FALSE;
	}
	fio.Close();

	// ƒpƒX–¼{ƒIƒtƒZƒbƒg‚ğ‹L˜^
	disk.path = path;
	disk.offset = offset;

	// ƒfƒBƒXƒN–¼(•K‚¸16•¶š‚ÅØ‚é)
	d68_hdr[0x10] = 0;
	strcpy(disk.name, (char*)d68_hdr);
	// ‚½‚¾‚µƒVƒ“ƒOƒ‹ƒfƒBƒXƒN‚ÅANULL‚©Default‚È‚çƒtƒ@ƒCƒ‹–¼+Šg’£q
	if (!GetFDI()->IsMulti()) {
		if (strcmp(disk.name, "Default") == 0) {
			strcpy(disk.name, path.GetShort());
		}
		if (strlen(disk.name) == 0) {
			strcpy(disk.name, path.GetShort());
		}
	}

	// ƒ‰ƒCƒgƒvƒƒeƒNƒg
	if (d68_hdr[0x1a] != 0) {
		disk.writep = TRUE;
	}

	// HDƒtƒ‰ƒO
	switch (d68_hdr[0x1b]) {
		// 2D,2DD
		case 0x00:
		case 0x10:
			hd = FALSE;
			break;
		// 2HD
		case 0x20:
			hd = TRUE;
			break;
		default:
			return FALSE;
	}

	// ƒgƒ‰ƒbƒN‚ğì¬(0`81ƒVƒŠƒ“ƒ_‚Ü‚ÅA82*2ƒgƒ‰ƒbƒN)
	for (i=0; i<164; i++) {
		track = new FDITrackD68(this, i, hd);
		AddTrack(track);
	}

	// ƒtƒ‰ƒOUpAI—¹
	d68_load = TRUE;
	return TRUE;
}

//---------------------------------------------------------------------------
//
//	ƒV[ƒN
//
//---------------------------------------------------------------------------
void FASTCALL FDIDiskD68::Seek(int c)
{
	int i;
	FDITrackD68 *track;
	DWORD offset;

	ASSERT(this);
	ASSERT((c >= 0) && (c < 82));
	ASSERT(d68_load);

	// ŠY“–‚·‚éƒgƒ‰ƒbƒN‚ğŒŸõ‚µAƒ[ƒh
	for (i=0; i<2; i++) {
		// ƒgƒ‰ƒbƒN‚ğŒŸõ
		track = (FDITrackD68*)Search(c * 2 + i);
		ASSERT(track);
		disk.head[i] = track;

		// ƒIƒtƒZƒbƒgæ“¾A—LŒøƒgƒ‰ƒbƒN‚È‚çƒ[ƒh
		if (d68_hdr[0x1b] == 0x00) {
			// 2D
			if (c == 0) {
				offset = GetD68Offset(i);
			}
			else {
				if (c & 1) {
					// 1,3,5...Šï”ƒVƒŠƒ“ƒ_‚Í‚»‚ê‚¼‚ê1,2,3ƒVƒŠƒ“ƒ_
					offset = GetD68Offset(c + 1 + i);
				}
				else {
					// 2,4,6...‹ô”ƒVƒŠƒ“ƒ_‚ÍUnformat
					offset = 0;
				}
			}
		}
		else {
			// 2DD,2HD
			offset = GetD68Offset(c * 2 + i);
		}
		if (offset > 0) {
			track->Load(disk.path, disk.offset + offset);
		}
	}
}

//---------------------------------------------------------------------------
//
//	D68ƒgƒ‰ƒbƒNƒIƒtƒZƒbƒg‚ğæ“¾
//	¦–³Œøƒgƒ‰ƒbƒN‚Í0
//
//---------------------------------------------------------------------------
DWORD FASTCALL FDIDiskD68::GetD68Offset(int track) const
{
	DWORD offset;
	const BYTE *ptr;

	ASSERT(this);
	ASSERT((track >= 0) && (track <= 163));
	ASSERT(d68_load);

	// ƒ|ƒCƒ“ƒ^æ“¾
	ptr = &d68_hdr[0x20 + (track << 2)];

	// ƒIƒtƒZƒbƒgæ“¾(ƒŠƒgƒ‹ƒGƒ“ƒfƒBƒAƒ“)
	offset = (DWORD)ptr[2];
	offset <<= 8;
	offset |= (DWORD)ptr[1];
	offset <<= 8;
	offset |= (DWORD)ptr[0];

	return offset;
}

//---------------------------------------------------------------------------
//
//	ƒZ[ƒu
//
//---------------------------------------------------------------------------
BOOL FASTCALL FDIDiskD68::Save()
{
	DWORD diskoff[16 + 1];
	BOOL format;
	int i;
	FDITrackD68 *track;
	DWORD offset;
	DWORD length;
	BYTE *fileptr;
	DWORD filelen;
	Fileio fio;
	BYTE *ptr;

	ASSERT(this);

	// ƒ[ƒh‚³‚ê‚Ä‚¢‚È‚¯‚ê‚Î‰½‚à‚µ‚È‚¢
	if (!d68_load) {
		return TRUE;
	}

	// ƒIƒtƒZƒbƒg‚ğÄæ“¾‚·‚é
	memset(diskoff, 0, sizeof(diskoff));
	CheckDisks(disk.path, diskoff);
	disk.offset = diskoff[disk.index];

	// ƒtƒH[ƒ}ƒbƒg‚Ì•ÏX‚ª¶‚¶‚Ä‚¢‚é‚©‚Ç‚¤‚©’²‚×‚é
	format = FALSE;
	for (i=0; i<164; i++) {
		track = (FDITrackD68*)Search(i);
		ASSERT(track);
		if (track->IsFormated()) {
			format = TRUE;
		}
	}

	// ¶‚¶‚Ä‚¢‚È‚¯‚ê‚ÎAƒgƒ‰ƒbƒN’PˆÊ‚Å•Û‘¶
	if (!format) {
		for (i=0; i<164; i++) {
			track = (FDITrackD68*)Search(i);
			ASSERT(track);
			offset = GetD68Offset(i);
			if (offset > 0) {
				if (!track->Save(disk.path, disk.offset + offset)) {
					return FALSE;
				}
			}
		}

		// ƒ‰ƒCƒgƒvƒƒeƒNƒg‚ÌH‚¢ˆá‚¢‚ª‚ ‚ê‚ÎA‚»‚±‚¾‚¯•Û‘¶
		i = 0;
		if (IsWriteP()) {
			i = 0x10;
		}
		if (d68_hdr[0x1a] != (BYTE)i) {
			d68_hdr[0x1a] = (BYTE)i;
			if (!fio.Open(disk.path, Fileio::ReadWrite)) {
				return FALSE;
			}
			if (!fio.Seek(diskoff[disk.index])) {
				fio.Close();
				return FALSE;
			}
			if (!fio.Write(d68_hdr, sizeof(d68_hdr))) {
				fio.Close();
				return FALSE;
			}
			fio.Close();
		}
		return TRUE;
	}

	// ƒtƒ@ƒCƒ‹‚ğ‘S‚Äƒƒ‚ƒŠ‚É—‚Æ‚·
	if (!fio.Open(disk.path, Fileio::ReadOnly)) {
		return FALSE;
	}
	filelen = fio.GetFileSize();
	try {
		fileptr = new BYTE[filelen];
	}
	catch (...) {
		fio.Close();
		return FALSE;
	}
	if (!fileptr) {
		fio.Close();
		return FALSE;
	}
	if (!fio.Read(fileptr, filelen)) {
		delete[] fileptr;
		fio.Close();
		return FALSE;
	}
	fio.Close();

	// ƒwƒbƒ_‚ğÄ\’z
	offset = sizeof(d68_hdr);
	for (i=0; i<164; i++) {
		track = (FDITrackD68*)Search(i);
		ASSERT(track);
		length = track->GetD68Length();
		ptr = &d68_hdr[0x20 + (i << 2)];
		if (length == 0) {
			memset(ptr, 0, 4);
		}
		else {
			ptr[3] = (BYTE)(offset >> 24);
			ptr[2] = (BYTE)(offset >> 16);
			ptr[1] = (BYTE)(offset >> 8);
			ptr[0] = (BYTE)offset;
			offset += length;
		}
	}
	d68_hdr[0x1a] = 0;
	if (IsWriteP()) {
		d68_hdr[0x1a] = 0x10;
	}
	ptr = &d68_hdr[0x1c];
	ptr[3] = (BYTE)(offset >> 24);
	ptr[2] = (BYTE)(offset >> 16);
	ptr[1] = (BYTE)(offset >> 8);
	ptr[0] = (BYTE)offset;

	// ƒtƒ@ƒCƒ‹‚Ì‘O‚ğ•Û‘¶
	if (!fio.Open(disk.path, Fileio::WriteOnly)) {
		delete[] fileptr;
		return FALSE;
	}
	if (diskoff[disk.index] != 0) {
		if (!fio.Write(fileptr, diskoff[disk.index])) {
			delete[] fileptr;
			fio.Close();
			return FALSE;
		}
	}

	// ƒwƒbƒ_‚ğ•Û‘¶
	if (!fio.Write(d68_hdr, sizeof(d68_hdr))) {
		delete[] fileptr;
		fio.Close();
		return FALSE;
	}
	offset -= sizeof(d68_hdr);

	// ƒTƒCƒY•ª‚ğ‚Â‚­‚é
	while (offset > 0) {
		// 1‰ñ‚Å‘‚«‚İ‚ªÏ‚Şê‡
		if (offset < filelen) {
			if (!fio.Write(fileptr, offset)) {
				delete[] fileptr;
				fio.Close();
				return FALSE;
			}
			break;
		}

		// •¡”‰ñ‚É‚í‚½‚éê‡
		if (!fio.Write(fileptr, filelen)) {
			delete[] fileptr;
			fio.Close();
			return FALSE;
		}
		offset -= filelen;
	}

	// ƒtƒ@ƒCƒ‹‚ÌŒã‚ğ•Û‘¶
	if (diskoff[disk.index + 1] != 0) {
		ASSERT(filelen >= diskoff[disk.index + 1]);
		if (!fio.Write(&fileptr[ diskoff[disk.index + 1] ],
				filelen - diskoff[disk.index + 1])) {
			delete[] fileptr;
			fio.Close();
			return FALSE;
		}
	}
	delete[] fileptr;

	// ƒŒƒ“ƒOƒX!=0‚É‚Â‚¢‚Ä‘S‚ÄƒZ[ƒu(‹­§•ÏXAƒtƒ@ƒCƒ‹‚ÍŠJ‚¢‚½‚Ü‚Ü)
	for (i=0; i<164; i++) {
		track = (FDITrackD68*)Search(i);
		ASSERT(track);
		length = track->GetD68Length();
		if (length != 0) {
			offset = GetD68Offset(i);
			ASSERT(offset != 0);
			track->ForceChanged();
			if (!track->Save(&fio, disk.offset + offset)) {
				return FALSE;
			}
		}
	}

	fio.Close();
	return TRUE;
}

//---------------------------------------------------------------------------
//
//	V‹KƒfƒBƒXƒNì¬
//
//---------------------------------------------------------------------------
BOOL FASTCALL FDIDiskD68::Create(const Filepath& path, const option_t *opt)
{
	Fileio fio;
	int i;
	FDITrackD68 *track;
	BOOL hd;

	ASSERT(this);
	ASSERT(opt);
	ASSERT(!GetFirst());
	ASSERT(!GetHead(0));
	ASSERT(!GetHead(1));

	// ƒwƒbƒ_‚ğì‚é
	memset(&d68_hdr, 0, sizeof(d68_hdr));
	ASSERT(strlen(opt->name) <= 16);
	strcpy((char*)d68_hdr, opt->name);
	if (opt->phyfmt == FDI_2DD) {
		hd = FALSE;
		d68_hdr[0x1b] = 0x10;
	}
	else {
		hd = TRUE;
		d68_hdr[0x1b] = 0x20;
	}

	// ƒwƒbƒ_‚ğ‘‚«‚Ş
	if (!fio.Open(path, Fileio::WriteOnly)) {
		return FALSE;
	}
	if (!fio.Write(d68_hdr, sizeof(d68_hdr))) {
		fio.Close();
		return FALSE;
	}
	fio.Close();

	// ƒpƒXAƒfƒBƒXƒN–¼AƒIƒtƒZƒbƒg
	disk.path = path;
	strcpy(disk.name, opt->name);
	disk.offset = 0;

	// ƒgƒ‰ƒbƒN‚ğì¬(0`81ƒVƒŠƒ“ƒ_‚Ü‚ÅA82*2ƒgƒ‰ƒbƒN)
	for (i=0; i<164; i++) {
		track = new FDITrackD68(this, i, hd);
		track->Create(opt->phyfmt);
		track->ForceFormat();
		AddTrack(track);
	}

	// ƒtƒ‰ƒOİ’è
	disk.writep = FALSE;
	disk.readonly = FALSE;
	d68_load = TRUE;

	// ˜_—ƒtƒH[ƒ}ƒbƒg
	FDIDisk::Create(path, opt);

	// •Û‘¶
	if (!Save()) {
		return FALSE;
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	D68ƒtƒ@ƒCƒ‹‚ÌŒŸ¸
//
//---------------------------------------------------------------------------
int FASTCALL FDIDiskD68::CheckDisks(const Filepath& path, DWORD *offbuf)
{
	Fileio fio;
	DWORD fsize;
	DWORD dsize;
	DWORD base;
	DWORD prev;
	DWORD offset;
	int disks;
	BYTE header[0x2b0];
	BYTE *ptr;
	int i;

	ASSERT(offbuf);

	// ‰Šú‰»
	disks = 0;
	base = 0;

	// ƒtƒ@ƒCƒ‹ƒTƒCƒYæ“¾
	if (!fio.Open(path, Fileio::ReadOnly)) {
		return 0;
	}
	fsize = fio.GetFileSize();
	fio.Close();
	if (fsize < sizeof(header)) {
		return 0;
	}

	// ƒfƒBƒXƒNƒ‹[ƒv
	while (disks < 16) {
		// ƒTƒCƒYƒI[ƒo‚È‚çI—¹
		if (base >= fsize) {
			break;
		}

		// ƒwƒbƒ_‚ğ“Ç‚Ş
		if (!fio.Open(path, Fileio::ReadOnly)) {
			return 0;
		}
		if (!fio.Seek(base)) {
			fio.Close();
			break;
		}
		if (!fio.Read(header, sizeof(header))) {
			fio.Close();
			break;
		}
		fio.Close();

		// –§“x‚ğƒ`ƒFƒbƒN
		switch (header[0x1b]) {
			case 0x00:
			case 0x10:
			case 0x20:
				break;
			default:
				return 0;
		}

		// ‚±‚ÌƒfƒBƒXƒNƒTƒCƒY‚ğæ“¾(0x200ˆÈãA1.92MBˆÈ‰º‚ÆŒÀ’è)
		ptr = &header[0x1c];
		dsize = (DWORD)ptr[3];
		dsize <<= 8;
		dsize |= (DWORD)ptr[2];
		dsize <<= 8;
		dsize |= (DWORD)ptr[1];
		dsize <<= 8;
		dsize |= (DWORD)ptr[0];

		if ((dsize + base) > fsize) {
			return 0;
		}
		if (dsize < 0x200) {
			return 0;
		}
		if (dsize > 1920 * 1024) {
			return 0;
		}

		// ƒIƒtƒZƒbƒg‚ğŒŸ¸Bdsize‚ğ’´‚¦‚Ä‚¢‚È‚­‚Ä’P’²‘‰Á‚Å‚ ‚é‚±‚Æ
		prev = 0;
		for (i=0; i<164; i++) {
			// 2DƒCƒ[ƒW‚Å‚ ‚ê‚Î84ƒgƒ‰ƒbƒNˆÈã‚ÍŒŸ¸‚µ‚È‚¢(•Ï‚È’l‚ª‘‚«‚Ü‚ê‚Ä‚¢‚éê‡‚ª‚ ‚é)
			if (header[0x1b] == 0x00) {
				if (i >= 84) {
					break;
				}
			}

			// ‚±‚Ìƒgƒ‰ƒbƒN‚ÌƒIƒtƒZƒbƒg‚ğ“¾‚é
			ptr = &header[0x20 + (i << 2)];
			offset = (DWORD)ptr[3];
			offset <<= 8;
			offset |= (DWORD)ptr[2];
			offset <<= 8;
			offset |= (DWORD)ptr[1];
			offset <<= 8;
			offset |= (DWORD)ptr[0];

			// ƒIƒtƒZƒbƒg‚ª0x10‚ÅŠ„‚èØ‚ê‚È‚¯‚ê‚ÎƒGƒ‰[
			if (offset & 0x0f) {
				return 0;
			}

			// 0‚ÍA‚»‚Ìƒgƒ‰ƒbƒN‚ªƒAƒ“ƒtƒH[ƒ}ƒbƒg‚Å‚ ‚é‚±‚Æ‚ğ¦‚·
			if (offset != 0) {
				// 0‚Å‚È‚¯‚ê‚Î
				if (prev == 0) {
					// Å‰‚Ìƒgƒ‰ƒbƒN‚Í2X0‚©‚çn‚Ü‚é‚±‚Æ
					if ((offset & 0xffffff0f) != 0x200) {
						return 0;
					}
				}
				else {
					// ’P’²‘‰Á‚Å
					if (offset <= prev) {
						return 0;
					}
					// ƒfƒBƒXƒNƒTƒCƒY‚ğ’´‚¦‚Ä‚¢‚È‚¢‚±‚Æ
					if (offset > dsize) {
						return 0;
					}
				}
				prev = offset;
			}
		}

		// –‡”UpAƒoƒbƒtƒ@‚É“o˜^AŸ‚Ö
		offbuf[disks] = base;
		disks++;
		base += dsize;
	}

	return disks;
}

//---------------------------------------------------------------------------
//
//	ƒIƒtƒZƒbƒgXV
//
//---------------------------------------------------------------------------
void FASTCALL FDIDiskD68::AdjustOffset()
{
	DWORD offset[0x10];

	ASSERT(this);

	memset(offset, 0, sizeof(offset));
	CheckDisks(disk.path, offset);
	disk.offset = offset[disk.index];
}

//---------------------------------------------------------------------------
//
//	ƒtƒ‰ƒbƒVƒ…
//
//---------------------------------------------------------------------------
BOOL FASTCALL FDIDiskD68::Flush()
{
	ASSERT(this);

	// ƒ[ƒh‚³‚ê‚Ä‚¢‚ê‚Î‘‚«‚İ
	if (d68_load) {
		return Save();
	}

	// ƒ[ƒh‚³‚ê‚Ä‚¢‚È‚¢
	return TRUE;
}

//===========================================================================
//
//	FDIƒgƒ‰ƒbƒN(BAD)
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	ƒRƒ“ƒXƒgƒ‰ƒNƒ^
//
//---------------------------------------------------------------------------
FDITrackBAD::FDITrackBAD(FDIDisk *disk, int track) : FDITrack(disk, track)
{
	ASSERT(disk);

	// ƒtƒ@ƒCƒ‹—LŒøƒZƒNƒ^”0
	bad_secs = 0;
}

//---------------------------------------------------------------------------
//
//	ƒ[ƒh
//
//---------------------------------------------------------------------------
BOOL FASTCALL FDITrackBAD::Load(const Filepath& path, DWORD offset)
{
	Fileio fio;
	BYTE buf[0x400];
	DWORD chrn[4];
	int i;
	FDISector *sector;

	ASSERT(this);

	// ‰Šú‰»Ï‚İ‚È‚ç•s—v(ƒV[ƒN–ˆ‚ÉŒÄ‚Î‚ê‚é‚Ì‚ÅA‚P“x‚¾‚¯“Ç‚ñ‚ÅƒLƒƒƒbƒVƒ…‚·‚é)
	if (IsInit()) {
		return TRUE;
	}

	// ƒZƒNƒ^‚ª‘¶İ‚µ‚È‚¢‚±‚Æ
	ASSERT(!GetFirst());
	ASSERT(GetAllSectors() == 0);
	ASSERT(GetMFMSectors() == 0);
	ASSERT(GetFMSectors() == 0);

	// ƒtƒ@ƒCƒ‹—LŒøƒZƒNƒ^”0
	bad_secs = 0;

	// CEHENŒˆ’è
	chrn[0] = GetTrack() >> 1;
	chrn[1] = GetTrack() & 1;
	chrn[3] = 3;

	// “Ç‚İ‚İƒI[ƒvƒ“
	if (!fio.Open(path, Fileio::ReadOnly)) {
		return FALSE;
	}

	// ƒV[ƒN(¸”s‚Å‚à‚æ‚¢)
	if (!fio.Seek(offset)) {
		// ƒV[ƒN¸”s‚È‚Ì‚ÅAE5‚Å–„‚ß‚é
		memset(buf, 0xe5, sizeof(buf));

		// ƒZƒNƒ^ƒ‹[ƒv
		for (i=0; i<8; i++) {
			chrn[2] = i + 1;
			sector = new FDISector(TRUE, chrn);
			sector->Load(buf, sizeof(buf), 0x74, FDD_NOERROR);
			AddSector(sector);
		}

		// ƒNƒ[ƒYA‰Šú‰»OK
		fio.Close();
		trk.init = TRUE;
		return TRUE;
	}

	// ƒ‹[ƒv
	for (i=0; i<8; i++) {
		// ƒoƒbƒtƒ@‚ğ–ˆ‰ñE5‚Å–„‚ß‚é
		memset(buf, 0xe5, sizeof(buf));

		// ƒf[ƒ^“Ç‚İ‚İ(¸”s‚µ‚Ä‚à‚æ‚¢)
		if (fio.Read(buf, sizeof(buf))) {
			// ƒtƒ@ƒCƒ‹—LŒøƒZƒNƒ^‚ğ‘‚â‚·(0`8)
			bad_secs++;
		}

		// ƒZƒNƒ^ì¬
		chrn[2] = i + 1;
		sector = new FDISector(TRUE, chrn);
		sector->Load(buf, sizeof(buf), 0x74, FDD_NOERROR);

		// ƒZƒNƒ^’Ç‰Á
		AddSector(sector);
	}

	// ƒNƒ[ƒY
	fio.Close();

	// ƒ|ƒWƒVƒ‡ƒ“ŒvZ
	CalcPos();

	// ‰Šú‰»ok
	trk.init = TRUE;
	return TRUE;
}

//---------------------------------------------------------------------------
//
//	ƒZ[ƒu
//
//---------------------------------------------------------------------------
BOOL FASTCALL FDITrackBAD::Save(const Filepath& path, DWORD offset)
{
	Fileio fio;
	FDISector *sector;
	BOOL changed;
	int index;

	ASSERT(this);

	// ‰Šú‰»‚³‚ê‚Ä‚¢‚È‚¯‚ê‚Î‘‚«‚Ş•K—v‚È‚µ
	if (!IsInit()) {
		return TRUE;
	}

	// ƒZƒNƒ^‚ğ‚Ü‚í‚Á‚ÄA‘‚«‚Ü‚ê‚Ä‚¢‚éƒZƒNƒ^‚ª‚ ‚é‚©
	sector = GetFirst();
	changed = FALSE;
	while (sector) {
		if (sector->IsChanged()) {
			changed = TRUE;
		}
		sector = sector->GetNext();
	}

	// ‚Ç‚ê‚à‘‚«‚Ü‚ê‚Ä‚¢‚È‚¯‚ê‚Î‰½‚à‚µ‚È‚¢
	if (!changed) {
		return TRUE;
	}

	// ƒtƒ@ƒCƒ‹ƒI[ƒvƒ“
	if (!fio.Open(path, Fileio::ReadWrite)) {
		return FALSE;
	}

	// ƒ‹[ƒv
	sector = GetFirst();
	index = 1;
	while (sector) {
		// •ÏX‚³‚ê‚Ä‚¢‚È‚¯‚ê‚ÎAŸ‚Ö
		if (!sector->IsChanged()) {
			offset += sector->GetLength();
			sector = sector->GetNext();
			index++;
			continue;
		}

		// —LŒø”ÍˆÍ“à‚©
		if (index > bad_secs) {
			// ƒtƒ@ƒCƒ‹‚ğ’´‚¦‚Ä‚¢‚é‚Ì‚ÅAƒ_ƒ~[ˆ—
			sector->ClrChanged();
			offset += sector->GetLength();
			sector = sector->GetNext();
			index++;
			continue;
		}

		// ƒV[ƒN
		if (!fio.Seek(offset)) {
			fio.Close();
			return FALSE;
		}

		// ‘‚«‚İ
		if (!fio.Write(sector->GetSector(), sector->GetLength())) {
			fio.Close();
			return FALSE;
		}

		// ƒtƒ‰ƒO‚ğ—‚Æ‚·
		sector->ClrChanged();

		// Ÿ‚Ö
		offset += sector->GetLength();
		sector = sector->GetNext();
		index++;
	}

	// I—¹
	fio.Close();
	return TRUE;
}

//===========================================================================
//
//	FDIƒfƒBƒXƒN(BAD)
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	ƒRƒ“ƒXƒgƒ‰ƒNƒ^
//
//---------------------------------------------------------------------------
FDIDiskBAD::FDIDiskBAD(int index, FDI *fdi) : FDIDisk(index, fdi)
{
	// IDİ’è
	disk.id = MAKEID('B', 'A', 'D', ' ');
}

//---------------------------------------------------------------------------
//
//	ƒfƒXƒgƒ‰ƒNƒ^
//
//---------------------------------------------------------------------------
FDIDiskBAD::~FDIDiskBAD()
{
	int i;
	DWORD offset;
	FDITrack *track;

	// ÅŒã‚Ìƒgƒ‰ƒbƒNƒf[ƒ^‚ğ‘‚«‚Ş
	for (i=0; i<2; i++) {
		// ƒgƒ‰ƒbƒN‚ª‚ ‚é‚©
		track = GetHead(i);
		if (!track) {
			continue;
		}

		// ƒgƒ‰ƒbƒNƒiƒ“ƒo‚©‚çAƒIƒtƒZƒbƒg‚ğZo(x8KB)
		offset = track->GetTrack();
		offset <<= 13;

		// ‘‚«‚İ
		track->Save(disk.path, offset);
		disk.head[i] = NULL;
	}
}

//---------------------------------------------------------------------------
//
//	ƒI[ƒvƒ“
//
//---------------------------------------------------------------------------
BOOL FASTCALL FDIDiskBAD::Open(const Filepath& path, DWORD offset)
{
	Fileio fio;
	DWORD size;
	FDITrackBAD *track;
	int i;

	ASSERT(this);
	ASSERT(offset == 0);
	ASSERT(!GetFirst());
	ASSERT(!GetHead(0));
	ASSERT(!GetHead(1));

	// ‘‚«‚İ‰Â”\‚Æ‚µ‚Ä‰Šú‰»
	disk.writep = FALSE;
	disk.readonly = FALSE;

	// ƒI[ƒvƒ“‚Å‚«‚é‚±‚Æ‚ğŠm‚©‚ß‚é
	if (!fio.Open(path, Fileio::ReadWrite)) {
		// “Ç‚İ‚İƒI[ƒvƒ“‚ğ‚İ‚é
		if (!fio.Open(path, Fileio::ReadOnly)) {
			return FALSE;
		}

		// “Ç‚İ‚İ‚Í‰Â”
		disk.writep = TRUE;
		disk.readonly = TRUE;
	}

	// ƒtƒ@ƒCƒ‹ƒTƒCƒY‚ª1024‚ÅŠ„‚èØ‚ê‚é‚±‚ÆA1280KBˆÈ‰º‚Å‚ ‚é‚±‚Æ‚ğŠm‚©‚ß‚é
	size = fio.GetFileSize();
	if (size & 0x3ff) {
		fio.Close();
		return FALSE;
	}
	if (size > 1310720) {
		fio.Close();
		return FALSE;
	}
	fio.Close();

	// ƒpƒXAƒIƒtƒZƒbƒg‚ğ‹L‰¯
	disk.path = path;
	disk.offset = offset;

	// ƒfƒBƒXƒN–¼‚Íƒtƒ@ƒCƒ‹–¼{Šg’£q‚Æ‚·‚é
	strcpy(disk.name, path.GetShort());

	// ƒgƒ‰ƒbƒN‚ğì¬(0`76ƒVƒŠƒ“ƒ_‚Ü‚ÅA77*2ƒgƒ‰ƒbƒN)
	for (i=0; i<154; i++) {
		track = new FDITrackBAD(this, i);
		AddTrack(track);
	}

	// I—¹
	return TRUE;
}

//---------------------------------------------------------------------------
//
//	ƒV[ƒN
//
//---------------------------------------------------------------------------
void FASTCALL FDIDiskBAD::Seek(int c)
{
	int i;
	FDITrackBAD *track;
	DWORD offset;

	ASSERT(this);
	ASSERT((c >= 0) && (c < 82));

	// ƒgƒ‰ƒbƒNƒf[ƒ^‚ğ‘‚«‚Ş
	for (i=0; i<2; i++) {
		// ƒgƒ‰ƒbƒN‚ª‚ ‚é‚©
		track = (FDITrackBAD*)GetHead(i);
		if (!track) {
			continue;
		}

		// ƒgƒ‰ƒbƒNƒiƒ“ƒo‚©‚çAƒIƒtƒZƒbƒg‚ğZo(x8KB)
		offset = track->GetTrack();
		offset <<= 13;

		// ‘‚«‚İ
		track->Save(disk.path, offset);
	}

	// c‚Í76‚Ü‚Å‹–‰ÂB”ÍˆÍŠO‚Å‚ ‚ê‚Îhead[i]=NULL‚Æ‚·‚é
	if ((c < 0) || (c > 76)) {
		disk.head[0] = NULL;
		disk.head[1] = NULL;
		return;
	}

	// ŠY“–‚·‚éƒgƒ‰ƒbƒN‚ğŒŸõ‚µAƒ[ƒh
	for (i=0; i<2; i++) {
		// ƒgƒ‰ƒbƒN‚ğŒŸõ
		track = (FDITrackBAD*)Search(c * 2 + i);
		ASSERT(track);
		disk.head[i] = track;

		// ƒgƒ‰ƒbƒNƒiƒ“ƒo‚©‚çAƒIƒtƒZƒbƒg‚ğZo(x8KB)
		offset = track->GetTrack();
		offset <<= 13;

		// ƒ[ƒh
		track->Load(disk.path, offset);
	}
}

//---------------------------------------------------------------------------
//
//	ƒtƒ‰ƒbƒVƒ…
//
//---------------------------------------------------------------------------
BOOL FASTCALL FDIDiskBAD::Flush()
{
	ASSERT(this);

	int i;
	DWORD offset;
	FDITrack *track;

	// ÅŒã‚Ìƒgƒ‰ƒbƒNƒf[ƒ^‚ğ‘‚«‚Ş
	for (i=0; i<2; i++) {
		// ƒgƒ‰ƒbƒN‚ª‚ ‚é‚©
		track = GetHead(i);
		if (!track) {
			continue;
		}

		// ƒgƒ‰ƒbƒNƒiƒ“ƒo‚©‚çAƒIƒtƒZƒbƒg‚ğZo(x8KB)
		offset = track->GetTrack();
		offset <<= 13;

		// ‘‚«‚İ
		if(!track->Save(disk.path, offset)) {
			return FALSE;
		}
	}

	return TRUE;
}

//===========================================================================
//
//	FDIƒgƒ‰ƒbƒN(2DD)
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	ƒRƒ“ƒXƒgƒ‰ƒNƒ^
//
//---------------------------------------------------------------------------
FDITrack2DD::FDITrack2DD(FDIDisk *disk, int track) : FDITrack(disk, track, FALSE)
{
	ASSERT(disk);
}

//---------------------------------------------------------------------------
//
//	ƒ[ƒh
//
//---------------------------------------------------------------------------
BOOL FASTCALL FDITrack2DD::Load(const Filepath& path, DWORD offset)
{
	Fileio fio;
	BYTE buf[0x200];
	DWORD chrn[4];
	int i;
	FDISector *sector;

	ASSERT(this);
	ASSERT((offset % 0x1200) == 0);
	ASSERT(offset < 0xb4000);

	// ‰Šú‰»Ï‚İ‚È‚ç•s—v(ƒV[ƒN–ˆ‚ÉŒÄ‚Î‚ê‚é‚Ì‚ÅA‚P“x‚¾‚¯“Ç‚ñ‚ÅƒLƒƒƒbƒVƒ…‚·‚é)
	if (IsInit()) {
		return TRUE;
	}

	// ƒZƒNƒ^‚ª‘¶İ‚µ‚È‚¢‚±‚Æ
	ASSERT(!GetFirst());
	ASSERT(GetAllSectors() == 0);
	ASSERT(GetMFMSectors() == 0);
	ASSERT(GetFMSectors() == 0);

	// CEHENŒˆ’è
	chrn[0] = GetTrack() >> 1;
	chrn[1] = GetTrack() & 1;
	chrn[3] = 2;

	// “Ç‚İ‚İƒI[ƒvƒ“
	if (!fio.Open(path, Fileio::ReadOnly)) {
		return FALSE;
	}

	// ƒV[ƒN
	if (!fio.Seek(offset)) {
		fio.Close();
		return FALSE;
	}

	// ƒ‹[ƒv
	for (i=0; i<9; i++) {
		// ƒf[ƒ^“Ç‚İ‚İ
		if (!fio.Read(buf, sizeof(buf))) {
			// “r’†‚Ü‚Å’Ç‰Á‚µ‚½•ª‚ğíœ‚·‚é
			ClrSector();
			fio.Close();
			return FALSE;
		}

		// ƒZƒNƒ^ì¬
		chrn[2] = i + 1;
		sector = new FDISector(TRUE, chrn);
		sector->Load(buf, sizeof(buf), 0x54, FDD_NOERROR);

		// ƒZƒNƒ^’Ç‰Á
		AddSector(sector);
	}

	// ƒNƒ[ƒY
	fio.Close();

	// ƒ|ƒWƒVƒ‡ƒ“ŒvZ
	CalcPos();

	// ‰Šú‰»ok
	trk.init = TRUE;
	return TRUE;
}

//===========================================================================
//
//	FDIƒfƒBƒXƒN(2DD)
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	ƒRƒ“ƒXƒgƒ‰ƒNƒ^
//
//---------------------------------------------------------------------------
FDIDisk2DD::FDIDisk2DD(int index, FDI *fdi) : FDIDisk(index, fdi)
{
	// IDİ’è
	disk.id = MAKEID('2', 'D', 'D', ' ');
}

//---------------------------------------------------------------------------
//
//	ƒfƒXƒgƒ‰ƒNƒ^
//
//---------------------------------------------------------------------------
FDIDisk2DD::~FDIDisk2DD()
{
	int i;
	DWORD offset;
	FDITrack *track;

	// ÅŒã‚Ìƒgƒ‰ƒbƒNƒf[ƒ^‚ğ‘‚«‚Ş
	for (i=0; i<2; i++) {
		// ƒgƒ‰ƒbƒN‚ª‚ ‚é‚©
		track = GetHead(i);
		if (!track) {
			continue;
		}

		// ƒgƒ‰ƒbƒNƒiƒ“ƒo‚©‚çAƒIƒtƒZƒbƒg‚ğZo(x0x1200)
		offset = track->GetTrack();
		offset *= 0x1200;

		// ‘‚«‚İ
		track->Save(disk.path, offset);
		disk.head[i] = NULL;
	}
}

//---------------------------------------------------------------------------
//
//	ƒI[ƒvƒ“
//
//---------------------------------------------------------------------------
BOOL FASTCALL FDIDisk2DD::Open(const Filepath& path, DWORD offset)
{
	Fileio fio;
	DWORD size;
	FDITrack2DD *track;
	int i;

	ASSERT(this);
	ASSERT(offset == 0);
	ASSERT(!GetFirst());
	ASSERT(!GetHead(0));
	ASSERT(!GetHead(1));

	// ‘‚«‚İ‰Â”\‚Æ‚µ‚Ä‰Šú‰»
	disk.writep = FALSE;
	disk.readonly = FALSE;

	// ƒI[ƒvƒ“‚Å‚«‚é‚±‚Æ‚ğŠm‚©‚ß‚é
	if (!fio.Open(path, Fileio::ReadWrite)) {
		// “Ç‚İ‚İƒI[ƒvƒ“‚ğ‚İ‚é
		if (!fio.Open(path, Fileio::ReadOnly)) {
			return FALSE;
		}

		// “Ç‚İ‚İ‚Í‰Â”
		disk.writep = TRUE;
		disk.readonly = TRUE;
	}

	// ƒtƒ@ƒCƒ‹ƒTƒCƒY‚ª737280‚Å‚ ‚é‚±‚Æ‚ğŠm‚©‚ß‚é
	size = fio.GetFileSize();
	if (size != 0xb4000) {
		fio.Close();
		return FALSE;
	}
	fio.Close();

	// ƒpƒXAƒIƒtƒZƒbƒg‚ğ‹L‰¯
	disk.path = path;
	disk.offset = offset;

	// ƒfƒBƒXƒN–¼‚Íƒtƒ@ƒCƒ‹–¼{Šg’£q‚Æ‚·‚é
	strcpy(disk.name, path.GetShort());

	// ƒgƒ‰ƒbƒN‚ğì¬(0`79ƒVƒŠƒ“ƒ_‚Ü‚ÅA80*2ƒgƒ‰ƒbƒN)
	for (i=0; i<160; i++) {
		track = new FDITrack2DD(this, i);
		AddTrack(track);
	}

	// I—¹
	return TRUE;
}

//---------------------------------------------------------------------------
//
//	ƒV[ƒN
//
//---------------------------------------------------------------------------
void FASTCALL FDIDisk2DD::Seek(int c)
{
	int i;
	FDITrack2DD *track;
	DWORD offset;

	ASSERT(this);
	ASSERT((c >= 0) && (c < 82));

	// ƒgƒ‰ƒbƒNƒf[ƒ^‚ğ‘‚«‚Ş
	for (i=0; i<2; i++) {
		// ƒgƒ‰ƒbƒN‚ª‚ ‚é‚©
		track = (FDITrack2DD*)GetHead(i);
		if (!track) {
			continue;
		}

		// ƒgƒ‰ƒbƒNƒiƒ“ƒo‚©‚çAƒIƒtƒZƒbƒg‚ğZo(x0x1200)
		offset = track->GetTrack();
		offset *= 0x1200;

		// ‘‚«‚İ
		track->Save(disk.path, offset);
	}

	// c‚Í79‚Ü‚Å‹–‰ÂB”ÍˆÍŠO‚Å‚ ‚ê‚Îhead[i]=NULL‚Æ‚·‚é
	if ((c < 0) || (c > 79)) {
		disk.head[0] = NULL;
		disk.head[1] = NULL;
		return;
	}

	// ŠY“–‚·‚éƒgƒ‰ƒbƒN‚ğŒŸõ‚µAƒ[ƒh
	for (i=0; i<2; i++) {
		// ƒgƒ‰ƒbƒN‚ğŒŸõ
		track = (FDITrack2DD*)Search(c * 2 + i);
		ASSERT(track);
		disk.head[i] = track;

		// ƒgƒ‰ƒbƒNƒiƒ“ƒo‚©‚çAƒIƒtƒZƒbƒg‚ğZo(x0x1200)
		offset = track->GetTrack();
		offset *= 0x1200;

		// ƒ[ƒh
		track->Load(disk.path, offset);
	}
}

//---------------------------------------------------------------------------
//
//	V‹KƒfƒBƒXƒNì¬
//
//---------------------------------------------------------------------------
BOOL FASTCALL FDIDisk2DD::Create(const Filepath& path, const option_t *opt)
{
	int i;
	FDITrack2DD *track;
	DWORD offset;
	Fileio fio;

	ASSERT(this);
	ASSERT(opt);

	// •¨—ƒtƒH[ƒ}ƒbƒg‚Í2DD‚Ì‚İ‹–‰Â
	if (opt->phyfmt != FDI_2DD) {
		return FALSE;
	}

	// ƒtƒ@ƒCƒ‹ì¬‚ğ‚İ‚é
	if (!fio.Open(path, Fileio::WriteOnly)) {
		return FALSE;
	}

	// ‘‚«‚İ‰Â”\‚Æ‚µ‚Ä‰Šú‰»
	disk.writep = FALSE;
	disk.readonly = FALSE;

	// ƒpƒX–¼AƒIƒtƒZƒbƒg‚ğ‹L˜^
	disk.path = path;
	disk.offset = 0;

	// ƒfƒBƒXƒN–¼‚Íƒtƒ@ƒCƒ‹–¼{Šg’£q‚Æ‚·‚é
	strcpy(disk.name, path.GetShort());

	// 0`159‚ÉŒÀ‚èAƒgƒ‰ƒbƒN‚ğì¬‚µ‚Ä•¨—ƒtƒH[ƒ}ƒbƒg
	for (i=0; i<160; i++) {
		track = new FDITrack2DD(this, i);
		track->Create(opt->phyfmt);
		AddTrack(track);
	}

	// ˜_—ƒtƒH[ƒ}ƒbƒg
	FDIDisk::Create(path, opt);

	// ‘‚«‚İƒ‹[ƒv
	offset = 0;
	for (i=0; i<160; i++) {
		// ƒgƒ‰ƒbƒNæ“¾
		track = (FDITrack2DD*)Search(i);
		ASSERT(track);
		ASSERT(track->IsChanged());

		// ‘‚«‚İ
		if (!track->Save(&fio, offset)) {
			fio.Close();
			return FALSE;
		}

		// Ÿ‚Ö
		offset += (0x200 * 9);
	}

	// ¬Œ÷
	fio.Close();
	return TRUE;
}

//---------------------------------------------------------------------------
//
//	ƒtƒ‰ƒbƒVƒ…
//
//---------------------------------------------------------------------------
BOOL FASTCALL FDIDisk2DD::Flush()
{
	int i;
	DWORD offset;
	FDITrack *track;

	ASSERT(this);

	// ÅŒã‚Ìƒgƒ‰ƒbƒNƒf[ƒ^‚ğ‘‚«‚Ş
	for (i=0; i<2; i++) {
		// ƒgƒ‰ƒbƒN‚ª‚ ‚é‚©
		track = GetHead(i);
		if (!track) {
			continue;
		}

		// ƒgƒ‰ƒbƒNƒiƒ“ƒo‚©‚çAƒIƒtƒZƒbƒg‚ğZo(x0x1200)
		offset = track->GetTrack();
		offset *= 0x1200;

		// ‘‚«‚İ
		if (!track->Save(disk.path, offset)) {
			return FALSE;
		}
	}

	return TRUE;
}

//===========================================================================
//
//	FDIƒgƒ‰ƒbƒN(2HQ)
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	ƒRƒ“ƒXƒgƒ‰ƒNƒ^
//
//---------------------------------------------------------------------------
FDITrack2HQ::FDITrack2HQ(FDIDisk *disk, int track) : FDITrack(disk, track)
{
	ASSERT(disk);
}

//---------------------------------------------------------------------------
//
//	ƒ[ƒh
//
//---------------------------------------------------------------------------
BOOL FASTCALL FDITrack2HQ::Load(const Filepath& path, DWORD offset)
{
	Fileio fio;
	BYTE buf[0x200];
	DWORD chrn[4];
	int i;
	FDISector *sector;

	ASSERT(this);
	ASSERT((offset % 0x2400) == 0);
	ASSERT(offset < 0x168000);

	// ‰Šú‰»Ï‚İ‚È‚ç•s—v(ƒV[ƒN–ˆ‚ÉŒÄ‚Î‚ê‚é‚Ì‚ÅA‚P“x‚¾‚¯“Ç‚ñ‚ÅƒLƒƒƒbƒVƒ…‚·‚é)
	if (IsInit()) {
		return TRUE;
	}

	// ƒZƒNƒ^‚ª‘¶İ‚µ‚È‚¢‚±‚Æ
	ASSERT(!GetFirst());
	ASSERT(GetAllSectors() == 0);
	ASSERT(GetMFMSectors() == 0);
	ASSERT(GetFMSectors() == 0);

	// CEHENŒˆ’è
	chrn[0] = GetTrack() >> 1;
	chrn[1] = GetTrack() & 1;
	chrn[3] = 2;

	// “Ç‚İ‚İƒI[ƒvƒ“
	if (!fio.Open(path, Fileio::ReadOnly)) {
		return FALSE;
	}

	// ƒV[ƒN
	if (!fio.Seek(offset)) {
		fio.Close();
		return FALSE;
	}

	// ƒ‹[ƒv
	for (i=0; i<18; i++) {
		// ƒf[ƒ^“Ç‚İ‚İ
		if (!fio.Read(buf, sizeof(buf))) {
			// “r’†‚Ü‚Å’Ç‰Á‚µ‚½•ª‚ğíœ‚·‚é
			ClrSector();
			fio.Close();
			return FALSE;
		}

		// ƒZƒNƒ^ì¬
		chrn[2] = i + 1;
		sector = new FDISector(TRUE, chrn);
		sector->Load(buf, sizeof(buf), 0x54, FDD_NOERROR);

		// ƒZƒNƒ^’Ç‰Á
		AddSector(sector);
	}

	// ƒNƒ[ƒY
	fio.Close();

	// ƒ|ƒWƒVƒ‡ƒ“ŒvZ
	CalcPos();

	// ‰Šú‰»ok
	trk.init = TRUE;
	return TRUE;
}

//===========================================================================
//
//	FDIƒfƒBƒXƒN(2HQ)
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	ƒRƒ“ƒXƒgƒ‰ƒNƒ^
//
//---------------------------------------------------------------------------
FDIDisk2HQ::FDIDisk2HQ(int index, FDI *fdi) : FDIDisk(index, fdi)
{
	// IDİ’è
	disk.id = MAKEID('2', 'H', 'Q', ' ');
}

//---------------------------------------------------------------------------
//
//	ƒfƒXƒgƒ‰ƒNƒ^
//
//---------------------------------------------------------------------------
FDIDisk2HQ::~FDIDisk2HQ()
{
	int i;
	DWORD offset;
	FDITrack *track;

	// ÅŒã‚Ìƒgƒ‰ƒbƒNƒf[ƒ^‚ğ‘‚«‚Ş
	for (i=0; i<2; i++) {
		// ƒgƒ‰ƒbƒN‚ª‚ ‚é‚©
		track = GetHead(i);
		if (!track) {
			continue;
		}

		// ƒgƒ‰ƒbƒNƒiƒ“ƒo‚©‚çAƒIƒtƒZƒbƒg‚ğZo(x0x2400)
		offset = track->GetTrack();
		offset *= 0x2400;

		// ‘‚«‚İ
		track->Save(disk.path, offset);
		disk.head[i] = NULL;
	}
}

//---------------------------------------------------------------------------
//
//	ƒI[ƒvƒ“
//
//---------------------------------------------------------------------------
BOOL FASTCALL FDIDisk2HQ::Open(const Filepath& path, DWORD offset)
{
	Fileio fio;
	DWORD size;
	FDITrack2HQ *track;
	int i;

	ASSERT(this);
	ASSERT(offset == 0);
	ASSERT(!GetFirst());
	ASSERT(!GetHead(0));
	ASSERT(!GetHead(1));

	// ‘‚«‚İ‰Â”\‚Æ‚µ‚Ä‰Šú‰»
	disk.writep = FALSE;
	disk.readonly = FALSE;

	// ƒI[ƒvƒ“‚Å‚«‚é‚±‚Æ‚ğŠm‚©‚ß‚é
	if (!fio.Open(path, Fileio::ReadWrite)) {
		// “Ç‚İ‚İƒI[ƒvƒ“‚ğ‚İ‚é
		if (!fio.Open(path, Fileio::ReadOnly)) {
			return FALSE;
		}

		// “Ç‚İ‚İ‚Í‰Â”
		disk.writep = TRUE;
		disk.readonly = TRUE;
	}

	// ƒtƒ@ƒCƒ‹ƒTƒCƒY‚ª1474560‚Å‚ ‚é‚±‚Æ‚ğŠm‚©‚ß‚é
	size = fio.GetFileSize();
	if (size != 0x168000) {
		fio.Close();
		return FALSE;
	}
	fio.Close();

	// ƒpƒXAƒIƒtƒZƒbƒg‚ğ‹L‰¯
	disk.path = path;
	disk.offset = offset;

	// ƒfƒBƒXƒN–¼‚Íƒtƒ@ƒCƒ‹–¼{Šg’£q‚Æ‚·‚é
	strcpy(disk.name, path.GetShort());

	// ƒgƒ‰ƒbƒN‚ğì¬(0`79ƒVƒŠƒ“ƒ_‚Ü‚ÅA80*2ƒgƒ‰ƒbƒN)
	for (i=0; i<160; i++) {
		track = new FDITrack2HQ(this, i);
		AddTrack(track);
	}

	// I—¹
	return TRUE;
}

//---------------------------------------------------------------------------
//
//	ƒV[ƒN
//
//---------------------------------------------------------------------------
void FASTCALL FDIDisk2HQ::Seek(int c)
{
	int i;
	FDITrack2HQ *track;
	DWORD offset;

	ASSERT(this);
	ASSERT((c >= 0) && (c < 82));

	// ƒgƒ‰ƒbƒNƒf[ƒ^‚ğ‘‚«‚Ş
	for (i=0; i<2; i++) {
		// ƒgƒ‰ƒbƒN‚ª‚ ‚é‚©
		track = (FDITrack2HQ*)GetHead(i);
		if (!track) {
			continue;
		}

		// ƒgƒ‰ƒbƒNƒiƒ“ƒo‚©‚çAƒIƒtƒZƒbƒg‚ğZo(x0x2400)
		offset = track->GetTrack();
		offset *= 0x2400;

		// ‘‚«‚İ
		track->Save(disk.path, offset);
	}

	// c‚Í79‚Ü‚Å‹–‰ÂB”ÍˆÍŠO‚Å‚ ‚ê‚Îhead[i]=NULL‚Æ‚·‚é
	if ((c < 0) || (c > 79)) {
		disk.head[0] = NULL;
		disk.head[1] = NULL;
		return;
	}

	// ŠY“–‚·‚éƒgƒ‰ƒbƒN‚ğŒŸõ‚µAƒ[ƒh
	for (i=0; i<2; i++) {
		// ƒgƒ‰ƒbƒN‚ğŒŸõ
		track = (FDITrack2HQ*)Search(c * 2 + i);
		ASSERT(track);
		disk.head[i] = track;

		// ƒgƒ‰ƒbƒNƒiƒ“ƒo‚©‚çAƒIƒtƒZƒbƒg‚ğZo(x0x2400)
		offset = track->GetTrack();
		offset *= 0x2400;

		// ƒ[ƒh
		track->Load(disk.path, offset);
	}
}

//---------------------------------------------------------------------------
//
//	V‹KƒfƒBƒXƒNì¬
//
//---------------------------------------------------------------------------
BOOL FASTCALL FDIDisk2HQ::Create(const Filepath& path, const option_t *opt)
{
	int i;
	FDITrack2HQ *track;
	DWORD offset;
	Fileio fio;

	ASSERT(this);
	ASSERT(opt);

	// •¨—ƒtƒH[ƒ}ƒbƒg‚Í2HQ‚Ì‚İ‹–‰Â
	if (opt->phyfmt != FDI_2HQ) {
		return FALSE;
	}

	// ƒtƒ@ƒCƒ‹ì¬‚ğ‚İ‚é
	if (!fio.Open(path, Fileio::WriteOnly)) {
		return FALSE;
	}

	// ‘‚«‚İ‰Â”\‚Æ‚µ‚Ä‰Šú‰»
	disk.writep = FALSE;
	disk.readonly = FALSE;

	// ƒpƒX–¼AƒIƒtƒZƒbƒg‚ğ‹L˜^
	disk.path = path;
	disk.offset = 0;

	// ƒfƒBƒXƒN–¼‚Íƒtƒ@ƒCƒ‹–¼{Šg’£q‚Æ‚·‚é
	strcpy(disk.name, path.GetShort());

	// 0`159‚ÉŒÀ‚èAƒgƒ‰ƒbƒN‚ğì¬‚µ‚Ä•¨—ƒtƒH[ƒ}ƒbƒg
	for (i=0; i<160; i++) {
		track = new FDITrack2HQ(this, i);
		track->Create(opt->phyfmt);
		AddTrack(track);
	}

	// ˜_—ƒtƒH[ƒ}ƒbƒg
	FDIDisk::Create(path, opt);

	// ‘‚«‚İƒ‹[ƒv
	offset = 0;
	for (i=0; i<160; i++) {
		// ƒgƒ‰ƒbƒNæ“¾
		track = (FDITrack2HQ*)Search(i);
		ASSERT(track);
		ASSERT(track->IsChanged());

		// ‘‚«‚İ
		if (!track->Save(&fio, offset)) {
			fio.Close();
			return FALSE;
		}

		// Ÿ‚Ö
		offset += (0x200 * 18);
	}

	// ¬Œ÷
	fio.Close();
	return TRUE;
}

//---------------------------------------------------------------------------
//
//	ƒtƒ‰ƒbƒVƒ…
//
//---------------------------------------------------------------------------
BOOL FASTCALL FDIDisk2HQ::Flush()
{
	ASSERT(this);

	int i;
	DWORD offset;
	FDITrack *track;

	// ÅŒã‚Ìƒgƒ‰ƒbƒNƒf[ƒ^‚ğ‘‚«‚Ş
	for (i=0; i<2; i++) {
		// ƒgƒ‰ƒbƒN‚ª‚ ‚é‚©
		track = GetHead(i);
		if (!track) {
			continue;
		}

		// ƒgƒ‰ƒbƒNƒiƒ“ƒo‚©‚çAƒIƒtƒZƒbƒg‚ğZo(x0x2400)
		offset = track->GetTrack();
		offset *= 0x2400;

		// ‘‚«‚İ
		if (!track->Save(disk.path, offset)) {
			return FALSE;
		}
	}

	return TRUE;
}
