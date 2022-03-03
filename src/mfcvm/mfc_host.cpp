//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2004 ‚o‚hD(ytanaka@ipc-tokai.or.jp)
//	Modified (C) 2006 co (cogood—gmail.com)
//	[ MFC Host ]
//
//---------------------------------------------------------------------------

#if defined(_WIN32)

#include "os.h"
#include "xm6.h"
#include "vm.h"
#include "windrv.h"
#include "memory.h"
#include "mfc_com.h"
#include "mfc_cfg.h"
#include "mfc_host.h"

#include <shlobj.h>
#include <winioctl.h>

//===========================================================================
//
//	Windowsƒtƒ@ƒCƒ‹ƒhƒ‰ƒCƒu
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	ƒRƒ“ƒXƒgƒ‰ƒNƒ^
//
//---------------------------------------------------------------------------
CWinFileDrv::CWinFileDrv()
{
	// ‰Šú‰»
	m_bWriteProtect = FALSE;
	m_bSlow = FALSE;
	m_bRemovable = FALSE;
	m_bManual = FALSE;
	m_bEnable = FALSE;
	m_bDevice = FALSE;
	m_hDevice = INVALID_HANDLE_VALUE;
	m_szDevice[0] = _T('\0');
	m_szDrive[0] = _T('\0');
	memset(&m_capCache, 0, sizeof(m_capCache));
	m_bVolumeCache = FALSE;
	m_szVolumeCache[0] = _T('\0');
	m_nUpdate = 0;
	m_bUpdateFile = FALSE;
	m_nUpdateFile = 0;
#ifdef XM6_HOST_UPDATE_BY_SEQUENCE
	m_nUpdateMedia = 0;
#endif // XM6_HOST_UPDATE_BY_SEQUENCE
#ifdef XM6_HOST_UPDATE_BY_FREQUENCY
	m_nUpdateCount = 0;
	memset(m_nUpdateBuffer, 0, sizeof(m_nUpdateBuffer));
#endif // XM6_HOST_UPDATE_BY_FREQUENCY
	m_szBase[0] = _T('\0');
}

//---------------------------------------------------------------------------
//
//	ƒfƒXƒgƒ‰ƒNƒ^
//
//---------------------------------------------------------------------------
CWinFileDrv::~CWinFileDrv()
{
#ifdef XM6_HOST_KEEP_OPEN_ERROR
	// ƒfƒoƒCƒXƒNƒ[ƒY
	CloseDevice();
#endif // XM6_HOST_KEEP_OPEN_ERROR
}

//---------------------------------------------------------------------------
//
//	ƒfƒoƒCƒXƒI[ƒvƒ“
//
//---------------------------------------------------------------------------
void FASTCALL CWinFileDrv::OpenDevice()
{
	ASSERT(this);
	ASSERT(m_hDevice == INVALID_HANDLE_VALUE);

	m_hDevice = ::CreateFile(
		m_szDevice,
		GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL);
}

//---------------------------------------------------------------------------
//
//	ƒfƒoƒCƒXƒNƒ[ƒY
//
//---------------------------------------------------------------------------
void FASTCALL CWinFileDrv::CloseDevice()
{
	ASSERT(this);

	if (m_hDevice != INVALID_HANDLE_VALUE) {
		CloseHandle(m_hDevice);
		m_hDevice = INVALID_HANDLE_VALUE;
	}
}

//---------------------------------------------------------------------------
//
//	‰Šú‰»
//
//---------------------------------------------------------------------------
void FASTCALL CWinFileDrv::Init(LPCTSTR szBase, DWORD nFlag)
{
	ASSERT(this);
	ASSERT(szBase);
	ASSERT(_tcslen(szBase) < _MAX_PATH);
	ASSERT(m_bWriteProtect == FALSE);
	ASSERT(m_bSlow == FALSE);
	ASSERT(m_bRemovable == FALSE);
	ASSERT(m_bManual == FALSE);
	ASSERT(m_bEnable == FALSE);
	ASSERT(m_bDevice == FALSE);
	ASSERT(m_hDevice == INVALID_HANDLE_VALUE);
	ASSERT(m_szDevice[0] == _T('\0'));
	ASSERT(m_szDrive[0] == _T('\0'));
	ASSERT(m_capCache.sectors == 0);
	ASSERT(m_bVolumeCache == FALSE);
	ASSERT(m_szVolumeCache[0] == _T('\0'));
	ASSERT(m_nUpdate == 0);
	ASSERT(m_bUpdateFile == FALSE);
	ASSERT(m_nUpdateFile == 0);
#ifdef XM6_HOST_UPDATE_BY_SEQUENCE
	ASSERT(m_nUpdateMedia == 0);
#endif // XM6_HOST_UPDATE_BY_SEQUENCE
#ifdef XM6_HOST_UPDATE_BY_FREQUENCY
	ASSERT(m_nUpdateCount == 0);
	ASSERT(m_szBase[0] == _T('\0'));
#endif // XM6_HOST_UPDATE_BY_FREQUENCY

	// ƒpƒ‰ƒ[ƒ^‚ğó‚¯æ‚é
	if (nFlag & FSFLAG_WRITE_PROTECT) m_bWriteProtect = TRUE;
	if (nFlag & FSFLAG_SLOW) m_bSlow = TRUE;
	if (nFlag & FSFLAG_REMOVABLE) m_bRemovable = TRUE;
	if (nFlag & FSFLAG_MANUAL) m_bManual = TRUE;
	_tcscpy(m_szBase, szBase);

	// ƒx[ƒXƒpƒX‚ÌÅŒã‚ÌƒpƒX‹æØ‚èƒ}[ƒN‚ğíœ‚·‚é
	// WARNING: Unicode—˜—p‚ÍC³‚ª•K—v
	TCHAR* pClear = NULL;
	TCHAR* p = m_szBase;
	for (;;) {
		TCHAR c = *p;
		if (c == _T('\0')) break;
		if (c == '/' || c== '\\') {
			pClear = p;
		} else {
			pClear = NULL;
		}
		if ((0x80 <= c && c <= 0x9F) || 0xE0 <= c) {	// Œµ–§‚É‚Í 0x81`0x9F 0xE0`0xEF
			p++;
			if (*p == _T('\0')) break;
		}
		p++;
	}
	if (pClear) *pClear = _T('\0');

	// ƒhƒ‰ƒCƒu–¼Šl“¾
	DWORD n = m_szBase[0] & 0xDF;
	if (n < 'A' || 'Z' < n || m_szBase[1] != ':') {
		// ƒhƒ‰ƒCƒu–¼‚ª•s³‚È‚çƒlƒbƒgƒ[ƒNƒhƒ‰ƒCƒu‚ÌUNC‚Æ‚µ‚Äˆµ‚¤
		m_bSlow = TRUE;
		CheckMedia();
		return;
	}

	// ƒhƒ‰ƒCƒu–¼¶¬
	_stprintf(m_szDrive, _T("%c:\\"), n);

	// ƒhƒ‰ƒCƒuí—Ş‚Ì”»’è
	DWORD nResult = ::GetDriveType(m_szDrive);
	switch (nResult) {
	case DRIVE_REMOVABLE:
	{
		m_bSlow = TRUE;
		m_bRemovable = TRUE;

		// ƒtƒƒbƒs[ƒfƒBƒXƒN”»’è
		SHFILEINFO shfi;
		nResult = ::SHGetFileInfo(m_szDrive, 0, &shfi, sizeof(shfi), SHGFI_TYPENAME);
		if (nResult) {
			if (_tcsstr(shfi.szTypeName, _T("ƒtƒƒbƒs[")) != NULL ||
				_tcsstr(shfi.szTypeName, _T("Floppy")) != NULL)
				m_bManual = TRUE;
		}
		break;
	}

	case DRIVE_CDROM:
		m_bSlow = TRUE;
		m_bRemovable = TRUE;
		break;

	case DRIVE_FIXED:
	case DRIVE_RAMDISK:
		break;

	default:
		m_bSlow = TRUE;
		break;
	}

	// ƒfƒoƒCƒXƒI[ƒvƒ“—pUNC¶¬
	OSVERSIONINFO v;
	memset(&v, 0, sizeof(v));
	v.dwOSVersionInfoSize = sizeof(v);
	::GetVersionEx(&v);
	if (v.dwPlatformId == VER_PLATFORM_WIN32_NT) {
		m_bDevice = TRUE;
		_stprintf(m_szDevice, _T("\\\\.\\%c:"), n);
	}

#ifdef XM6_HOST_KEEP_OPEN_ERROR
	// –ˆ‰ñƒI[ƒvƒ“/ƒNƒ[ƒY‚·‚é‚Æ’x‚¢‚Ì‚ÅAƒI[ƒvƒ“‚µ‚½‚Ü‚Üg‚¨‚¤‚Æ
	// ‚µ‚½‚ªg‚¢‚à‚Ì‚É‚È‚ç‚È‚©‚Á‚½B
	// ‚±‚±‚ÅƒI[ƒvƒ“‚µ‚½ê‡AÅ‰‚©‚çƒhƒ‰ƒCƒu‚ÉƒƒfƒBƒA‚ª“ü‚Á‚Ä‚¢
	// ‚ê‚Î‰½“x‘}“üE”ro‚ğ‚µ‚Ä‚à–â‘è‚È‚©‚Á‚½‚ªAÅ‰ƒhƒ‰ƒCƒu‚ÉƒƒfƒB
	// ƒA‚ª“ü‚Á‚Ä‚¢‚È‚©‚Á‚½ê‡A‚±‚Ìƒnƒ“ƒhƒ‹‚ğ•Â‚¶‚é‚Ü‚ÅWindows‚ªƒ
	// ƒfƒBƒA‘}“ü‚ğ”F¯‚µ‚È‚­‚È‚Á‚Ä‚µ‚Ü‚Á‚½B

	// ƒfƒoƒCƒXƒI[ƒvƒ“
	OpenDevice();
#endif // XM6_HOST_KEEP_OPEN_ERROR

	// ƒƒfƒBƒA—LŒøƒ`ƒFƒbƒN
	if (m_bManual == FALSE) CheckMedia();
}

//---------------------------------------------------------------------------
//
//	ƒhƒ‰ƒCƒuó‘Ô‚Ìæ“¾
//
//---------------------------------------------------------------------------
DWORD FASTCALL CWinFileDrv::GetStatus() const
{
	ASSERT(this);

	return
		(m_bRemovable ? 0 : 0x40) |
		(m_bEnable ? (m_bWriteProtect ? 0x08 : 0) | 0x02 : 0);
}

//---------------------------------------------------------------------------
//
//	ƒƒfƒBƒAó‘Ôİ’è
//
//---------------------------------------------------------------------------
void FASTCALL CWinFileDrv::SetEnable(BOOL bEnable)
{
	ASSERT(this);

	m_bEnable = bEnable;

	// ƒƒfƒBƒA‚ª“ü‚Á‚Ä‚¢‚È‚¯‚ê‚ÎƒLƒƒƒbƒVƒ…Á‹
	if (bEnable == FALSE) {
		memset(&m_capCache, 0, sizeof(m_capCache));
		m_bVolumeCache = FALSE;
		m_szVolumeCache[0] = _T('\0');
	}
}

//---------------------------------------------------------------------------
//
//	ƒŠƒ€[ƒoƒuƒ‹ƒƒfƒBƒA‚Ìƒtƒ@ƒCƒ‹ƒLƒƒƒbƒVƒ…—LŒøŠÔ‚ğXV
//
//---------------------------------------------------------------------------
void FASTCALL CWinFileDrv::SetTimeout()
{
	ASSERT(this);

	// ƒŠƒ€[ƒoƒuƒ‹ƒƒfƒBƒA‚Å‚È‚¯‚ê‚Î‘¦I—¹
	if (m_bRemovable == FALSE) return;

	// —LŒøŠÔ‚ğXV
	m_bUpdateFile = TRUE;
	m_nUpdateFile = ::GetTickCount();
}

//---------------------------------------------------------------------------
//
//	ƒŠƒ€[ƒoƒuƒ‹ƒƒfƒBƒA‚Ìƒtƒ@ƒCƒ‹ƒLƒƒƒbƒVƒ…—LŒøŠÔ‚ğŠm”F
//
//---------------------------------------------------------------------------
BOOL FASTCALL CWinFileDrv::CheckTimeout()
{
	ASSERT(this);

	// ƒŠƒ€[ƒoƒuƒ‹ƒƒfƒBƒA‚Å‚È‚¯‚ê‚Î‘¦I—¹
	if (m_bRemovable == FALSE) return FALSE;

	// ƒ^ƒCƒ€ƒAƒEƒg‚ğŠm”F
	DWORD nCount = ::GetTickCount();
	DWORD nDelta = nCount - m_nUpdateFile;
	if (nDelta < XM6_HOST_REMOVABLE_CACHE_TIME) return FALSE;	// XV•s—v

	if (m_bUpdateFile == FALSE) return FALSE;	// Šù‚ÉXVÏ‚İ
	m_bUpdateFile = FALSE;

	return TRUE;	// XVƒZƒˆ
}

#ifdef XM6_HOST_UPDATE_BY_SEQUENCE
//---------------------------------------------------------------------------
//
//	ƒŠƒ€[ƒoƒuƒ‹ƒƒfƒBƒA‚Ìó‘ÔXV‚ğ—LŒø‚É‚·‚é
//
//	ƒRƒ}ƒ“ƒh$41(ƒfƒBƒŒƒNƒgƒŠƒ`ƒFƒbƒN)‚Ì’¼Œã‚ÌƒRƒ}ƒ“ƒh$57(ƒƒfƒBƒAŒğ
//	Š·ƒ`ƒFƒbƒN)‚ÅƒƒfƒBƒA‘}“üƒ`ƒFƒbƒN‚ğÀs‚·‚é‚½‚ßAƒXƒe[ƒg•Ï”‚Ì
//	ŠÇ—‚ğs‚È‚¤B
//
//	‚½‚¾‚µ—áŠO‚Æ‚µ‚ÄAƒRƒ}ƒ“ƒh$57(ƒƒfƒBƒAŒğŠ·ƒ`ƒFƒbƒN)‚ÌŒ‹‰Ê‚ªƒGƒ‰[
//	‚Ìê‡‚ÍA‚»‚Ì’¼Œã‚É•K‚¸Human68k‚ªƒRƒ}ƒ“ƒh$41(ƒfƒBƒŒƒNƒgƒŠƒ`ƒFƒb
//	ƒN)‚ğŒÄ‚Ñ‚¾‚·‚½‚ßA‚±‚Ì’¼Œã‚ÌƒƒfƒBƒAŒğŠ·ƒ`ƒFƒbƒN‚Å‚Í‰½‚àÀs‚³
//	‚ê‚È‚¢‚æ‚¤‚É‚·‚éB
//
//	‚â‚â‚±‚µ‚¢‚¯‚Çˆ—‚Ì—¬‚ê‚ÍˆÈ‰º‚Ì‚æ‚¤‚ÈŠ´‚¶B
//	ƒRƒ}ƒ“ƒh$57 - ƒƒfƒBƒAŒğŠ·ƒ`ƒFƒbƒN ƒƒfƒBƒA‚È‚µ ¨ SetMediaUpdate(TRUE) S2
//	“à•”“®ì$41 - ƒfƒBƒŒƒNƒgƒŠƒ`ƒFƒbƒN S2 SetMediaUpdate(FALSE) Disableó‘Ô‰ğœ S0
//	ƒRƒ}ƒ“ƒh$57 - ƒƒfƒBƒAŒğŠ·ƒ`ƒFƒbƒN S0 ‰½‚à‚µ‚È‚¢ ƒƒfƒBƒA‚È‚µ ¨ SetMediaUpdate(TRUE) S2
//	“à•”“®ì$41 - ƒfƒBƒŒƒNƒgƒŠƒ`ƒFƒbƒN S2 SetMediaUpdate(FALSE) Disableó‘Ô‰ğœ S0
//	ƒRƒ}ƒ“ƒh$41 - ƒfƒBƒŒƒNƒgƒŠƒ`ƒFƒbƒN S0 SetMediaUpdate(FALSE) Enableó‘Ôİ’è S1
//	ƒRƒ}ƒ“ƒh$57 - ƒƒfƒBƒAŒğŠ·ƒ`ƒFƒbƒN S1 ŒğŠ·ƒ`ƒFƒbƒNÀs ƒƒfƒBƒA‚È‚µ ¨ SetMediaUpdate(TRUE) S2
//	“à•”“®ì$41 - ƒfƒBƒŒƒNƒgƒŠƒ`ƒFƒbƒN S2 SetMediaUpdate(FALSE) Disableó‘Ô‰ğœ S0
//
//---------------------------------------------------------------------------
void FASTCALL CWinFileDrv::SetMediaUpdate(BOOL bDisable)
{
	ASSERT(this);
	if (bDisable) {
		m_nUpdateMedia = 2;
	} else {
		if (m_nUpdateMedia > 1) {
			m_nUpdateMedia = 0;
		} else {
			m_nUpdateMedia = 1;
		}
	}
}
#endif // XM6_HOST_UPDATE_BY_SEQUENCE

//---------------------------------------------------------------------------
//
//	ƒŠƒ€[ƒoƒuƒ‹ƒƒfƒBƒA‚Ìó‘ÔXVƒ`ƒFƒbƒN
//
//	è“®ƒCƒWƒFƒNƒg‚ÌƒƒfƒBƒA‚É‚¨‚¢‚ÄA’ZŠÔ“à‚É˜A‘±‚µ‚ÄCheckMedia‚ª
//	ŒÄ‚Ño‚³‚ê‚½ê‡AƒAƒNƒZƒX–‘Oƒ`ƒFƒbƒNˆ—‚ÉˆÚs‚·‚éB
//
//	mint ‚È‚Ç‚ÌƒƒfƒBƒAŠÄ‹ƒpƒ^[ƒ“‚ÍˆÈ‰º‚Ì’Ê‚èB
//	5000msˆÈ“à‚ÉCheckMedia‚ª1`2‰ñŒÄ‚Î‚ê‚é: ƒŠƒ€[ƒoƒuƒ‹ƒƒfƒBƒAŠÄ‹’†
//	100msˆÈ“à‚ÉCheckMedia‚ª12‰ñˆÈãŒÄ‚Î‚ê‚é: ƒŠƒ€[ƒoƒuƒ‹ƒƒfƒBƒA‚ğŠJ‚¢‚½
//
//	ŠT—v: Œ»İ‚ÌIBM-PC/ATŒİŠ·‹@‚Å‚ÍAFDD“à‚ÌƒƒfƒBƒA‚Ì‘¶İ‚ğAFDD“à
//	‚Ìƒf[ƒ^‚É’¼ÚƒAƒNƒZƒX‚·‚é‚±‚Æ‚Å‚µ‚©ŠÏ‘ª‚Å‚«‚È‚¢B‚±‚ÌŠÏ‘ªsˆ×‚É
//	‚Í3‚Â‚Ì–â‘è‚ª‘¶İ‚·‚éB
//
//	1. ˆê”Ê“I‚ÈATŒİŠ·‹@‚Å‚ÍAFDD‚Ìƒ‚[ƒ^[‚ª’â~‚µ‚Ä‚¢‚éó‘Ô‚©‚ç‚Ìƒh
//	ƒ‰ƒCƒu‚Ö‚ÌƒAƒNƒZƒXŠ®—¹‚Ü‚ÅA‚Ç‚ñ‚ÈAPI‚ğ—˜—p‚µ‚Ä‚à1000msˆÈã‚©‚Á
//	‚Ä‚µ‚Ü‚¤BX680x0‚Å‚ÍAmint“™‚Ì‘ã•\“I‚Èƒtƒ@ƒCƒ‰[‚É‚¨‚¢‚ÄAƒhƒ‰ƒC
//	ƒu•ÏX‚Ì“x‚É‘Sƒhƒ‰ƒCƒu‚ÌƒƒfƒBƒA‘}“üó‘Ô‚ğ•p”É‚ÉŠm”F‚µ‚Ä‚¢‚é‚½‚ßA
//	–ˆ‰ñŠÏ‘ª‚µ‚Ä‚µ‚Ü‚¤‚ÆA“®ì‘¬“x‚ª‹É’[‚É’á‰º‚µAƒ†[ƒU‚Ìg—pŠ´‚ğ’˜
//	‚µ‚­ˆ«‰»‚³‚¹‚éB
//
//	2. FDD‚Ìƒ‚[ƒ^[‚ª‰ñ“]‚µ‚Ä‚¢‚½ê‡‚Å‚àAŠÏ‘ªŠ®—¹‚Ü‚Å‚Í”10`
//	100ms’ö“x‚Ìˆ—ŠÔ‚ğ—v‚·‚éBX680x0‚Å‚ÍA’Êí‚ÌHuman68k‚Ì—˜—p‚É
//	‚¨‚¢‚Ä‚àAƒƒfƒBƒAó‘Ô‚Ìƒ`ƒFƒbƒN‚ª•p”É‚És‚È‚í‚ê‚é‚½‚ßAVM‘¤‚©‚ç
//	‚Ìw¦‚Ì“x‚ÉŠÏ‘ª‚·‚é‚Æ“®ì‘¬“x‚ª’á‰º‚µ‚Ä‚µ‚Ü‚¤B
//
//	3. FDD‚ÉƒƒfƒBƒA‚ª‘¶İ‚µ‚È‚¢ê‡‚ÉŠÏ‘ª‚ªs‚È‚í‚ê‚é‚ÆAFDD‚©‚çˆÙ
//	‰¹‚ª”­¶‚µ‚Ä‚µ‚Ü‚¤B‚±‚ê‚Íƒn[ƒhƒEƒFƒA‚Ì§–ñ‚Å‚ ‚èƒ\ƒtƒgƒEƒFƒA‘¤
//	‚©‚ç‚Í‰ğŒˆ‚Å‚«‚È‚¢Bˆê•ûAX680x0‚Å‚ÍAƒ|[ƒŠƒ“ƒO‚É‚æ‚èƒƒfƒBƒA‘¶
//	İŠm”F‚ğs‚È‚¤‚Ì‚ªˆê”Ê“I‚Å‚ ‚éBVM‘¤‚©‚ç‚Ìw¦’Ê‚è‚ÉŠÏ‘ªsˆ×‚ğs
//	‚È‚Á‚½ê‡Aˆê’èŠÔŠu‚ÅFDD‚ªˆÙ‰¹‚ğ‘t‚Å‚éÅˆ«‚Ì–‘Ô‚ª”­¶‚µA–^‘
//	–¯‹@‚Ìˆ«–²‚ªÄ—ˆ‚·‚éB
//
//	‚»‚±‚ÅAè“®ƒCƒWƒFƒNƒgŒ^‚ÌƒŠƒ€[ƒoƒuƒ‹ƒƒfƒBƒA‚É‚Â‚¢‚Ä‚ÍAˆÈ‰º‚Ì
//	ğŒ‚ÅƒƒfƒBƒA‚ÌŠÏ‘ª‚ğs‚È‚¤d—l‚Æ‚µ‚½B
//
//	1. ÀÛ‚ÉƒzƒXƒg‘¤OS‚É‚æ‚éƒAƒNƒZƒX‚ªs‚È‚í‚ê‚é’¼‘O‚ÉŠÏ‘ª‚·‚éB
//	   CHostEntry::CheckMediaAccess()‚Ìƒpƒ^[ƒ“1,2B
//	2. ’ZŠúŠÔ‚ÉW’†‚µ‚ÄƒƒfƒBƒAŠÏ‘ª—v‹‚ª”­¶‚µ‚½ê‡‚ÉŠÏ‘ª‚·‚éB
//	   ƒƒfƒBƒA‘}“ü‚ğŒŸo‚Å‚«‚È‚¢USBƒfƒoƒCƒX‚à‚ ‚é‚Ì‚ÅA‚±‚Ì”»’è‚Í
//	   ƒŠƒ€[ƒoƒuƒ‹ƒƒfƒBƒA‘S‚Ä‚Ås‚È‚¤‚æ‚¤•ÏXB
//     ¨mint2.xxŒn‚É‘Î‰‚³‚¹‚é‚½‚ßA”»’è‚ğŠÔ‚Å‚Í‚È‚­ƒfƒBƒŒƒNƒgƒŠ
//     ƒ`ƒFƒbƒN’¼Œã‚ÌƒƒfƒBƒAŠÏ‘ª—v‹‚Ìƒ^ƒCƒ~ƒ“ƒO‚Ås‚È‚¤‚æ‚¤•ÏXB
//	   CHostEntry::CheckMediaAccess()‚Ìƒpƒ^[ƒ“3B
//	3. ã‹L2‚Â‚¾‚¯‚Å‚ÍƒƒfƒBƒA‚ª“ü‚êŠ·‚í‚Á‚½‚±‚Æ‚ğŒŸo‚Å‚«‚È‚¢‰Â”\«
//	   ‚ª‚ ‚é‚½‚ßAƒLƒƒƒbƒVƒ…‚Ì—LŒøŠúŠÔ‚ğ”•bŠÔ‚É§ŒÀ‚µ‚Ä‘Îˆ‚·‚éB
//	   ƒLƒƒƒbƒVƒ…‚Å—˜—p‚µ‚Ä‚¢‚éFindFirstChangeNotification()‚ªƒhƒ‰ƒC
//	   ƒu‚ğƒƒbƒN‚µ‚ÄƒCƒWƒFƒNƒg•s‰Â‚É‚È‚é‚½‚ßA’ZŠÔ‚ÅŠJ•ú‚·‚é‚±‚ÆB
//	   CHostEntry::isMediaOffline()‚ÆCHostEntry::CheckTimeout()‚Åˆ—B
//	4. ˜A‘±‚µ‚ÄŠÏ‘ª‚ğs‚È‚í‚È‚¢‚æ‚¤Aˆê“xƒAƒNƒZƒX‚µ‚½‚ç”•bŠÔ‚ÌƒK[
//	   ƒhŠúŠÔ‚ğİ‚¯AVM‘¤‚Ö‚ÍƒLƒƒƒbƒVƒ…î•ñ‚ğ“n‚·B
//
//	ã‹Ld—l‚Ì‚½‚ßAè“®ƒCƒWƒFƒNƒgŒ^‚ÌƒŠƒ€[ƒoƒuƒ‹ƒƒfƒBƒA‚É‚Â‚¢‚Ä‚ÍA
//	ƒƒfƒBƒA‚ª‘¶İ‚µ‚È‚¢ê‡‚ÌƒAƒNƒZƒX‚Í³íŒn‚Æ‚İ‚È‚µA”’‘ÑƒGƒ‰[‚Í
//	o‚³‚È‚¢‚±‚Æ‚É‚·‚éB
//
//---------------------------------------------------------------------------
BOOL FASTCALL CWinFileDrv::CheckMediaUpdate()
{
	ASSERT(this);

	// ƒŠƒ€[ƒoƒuƒ‹ƒƒfƒBƒA‚Å‚È‚¯‚ê‚Î‘¦I—¹
	if (m_bRemovable == FALSE) return FALSE;

#ifdef XM6_HOST_UPDATE_BY_SEQUENCE
	// XVƒtƒ‰ƒO‚ğ”»’è
	if (m_nUpdateMedia == 1) {
		m_nUpdateMedia = 0;
		return TRUE;	// XVƒZƒˆ
	}
	m_nUpdateMedia = 0;	// Å‰‚Ì1‰ñ–Ú‚¾‚¯XV‚·‚ê‚Î‚¢‚¢‚Ì‚Åƒtƒ‰ƒO‚ğ~‚ë‚·
#endif // XM6_HOST_UPDATE_BY_SEQUENCE

#ifdef XM6_HOST_UPDATE_BY_FREQUENCY
	// ƒ`ƒFƒbƒN‚ğ‹L˜^
	DWORD n = ::GetTickCount();
	DWORD nOld = m_nUpdateBuffer[m_nUpdateCount];
	m_nUpdateBuffer[m_nUpdateCount] = n;
	m_nUpdateCount++;
	m_nUpdateCount %= XM6_HOST_REMOVABLE_RELOAD_COUNT;

	// ŠÔ‚ ‚½‚è‚ÌŒÄ‚Ño‚µ‰ñ”‚ª­‚È‚¯‚ê‚ÎƒLƒƒƒbƒVƒ…‚ğg—p‚·‚é
	if (n - nOld <= XM6_HOST_REMOVABLE_RELOAD_TIME) return TRUE;	// XVƒZƒˆ
#endif // XM6_HOST_UPDATE_BY_FREQUENCY

#ifdef XM6_HOST_UPDATE_ALWAYS
	return TRUE;	// XVƒZƒˆ
#endif //XM6_HOST_UPDATE_ALWAYS

	return FALSE;	// XV•s—v
}

//---------------------------------------------------------------------------
//
//	ƒŠƒ€[ƒoƒuƒ‹ƒƒfƒBƒA‚ÌƒAƒNƒZƒX–‘Oƒ`ƒFƒbƒN
//
//	è“®ƒCƒWƒFƒNƒg‚ÌƒƒfƒBƒA‚ÍAƒAƒNƒZƒX‚³‚ê‚é’¼‘O‚Éó‘Ô‚ğŠm”F‚·‚éB
//	˜A‘±‚µ‚ÄŒÄ‚Ño‚³‚ê‚½ê‡‚ÍƒLƒƒƒbƒVƒ…‚ğg—p‚·‚éB
//
//---------------------------------------------------------------------------
BOOL FASTCALL CWinFileDrv::CheckMediaAccess(BOOL bManual)
{
	ASSERT(this);

	if (bManual) {
		// è“®ƒCƒWƒFƒNƒg‚Å‚È‚¯‚ê‚Î‘¦I—¹
		if (m_bManual == FALSE) return FALSE;
	} else {
		// ƒŠƒ€[ƒoƒuƒ‹ƒƒfƒBƒA‚Å‚È‚¯‚ê‚Î‘¦I—¹
		if (m_bRemovable == FALSE) return FALSE;
	}

	// ˜A‘±‚µ‚ÄŒÄ‚Ño‚³‚ê‚½ê‡‚ÍƒLƒƒƒbƒVƒ…‚ğg—p‚·‚é
	DWORD nCount = ::GetTickCount();
	DWORD nDelta = nCount - m_nUpdate;
	if (nDelta < XM6_HOST_REMOVABLE_GUARD_TIME) return FALSE;	// XV•s—v
	m_nUpdate = nCount;

	return TRUE;	// XVƒZƒˆ
}

//---------------------------------------------------------------------------
//
//	ƒƒfƒBƒA—LŒøƒ`ƒFƒbƒN
//
//	ƒŠƒ€[ƒoƒuƒ‹ƒƒfƒBƒA‘}“ü’¼Œã‚ÉÀs‚·‚é‚ÆAOpen‚à‚µ‚­‚Í
//	DeviceIoControl“à•”‚Å’â~‚µ‚Ä\”•bŠÔ‹A‚Á‚Ä‚±‚È‚¢‚±‚Æ‚ª‚ ‚é‚Ì‚Å
//	ŒÄ‚Ño‚µƒ^ƒCƒ~ƒ“ƒO‚É’ˆÓ‚·‚é‚±‚ÆB
//
//---------------------------------------------------------------------------
BOOL FASTCALL CWinFileDrv::CheckMedia()
{
	ASSERT(this);

	// ƒƒfƒBƒA‘}“ü‚Æ‚İ‚È‚·
	BOOL bEnable = TRUE;

	if (m_bDevice) {
		bEnable = FALSE;

		// ƒfƒoƒCƒXƒI[ƒvƒ“
		OpenDevice();

		if (m_hDevice != INVALID_HANDLE_VALUE) {
			// ó‘ÔŠl“¾
			DWORD nSize;
			bEnable = ::DeviceIoControl(
				m_hDevice,
				IOCTL_STORAGE_CHECK_VERIFY,
				NULL,
				0,
				NULL,
				0,
				&nSize,
				NULL);
		}

		// ƒfƒoƒCƒXƒNƒ[ƒY
		CloseDevice();
	}

	// ƒƒfƒBƒAó‘Ô”½‰f
	SetEnable(bEnable);

	// ÅIXV
	m_nUpdate = ::GetTickCount();

	return bEnable;
}

//---------------------------------------------------------------------------
//
//	ƒCƒWƒFƒNƒg
//
//---------------------------------------------------------------------------
BOOL FASTCALL CWinFileDrv::Eject()
{
	ASSERT(this);

	// ƒŠƒ€[ƒoƒuƒ‹ƒƒfƒBƒA‚Å‚È‚¯‚ê‚Î‘¦I—¹
	if (m_bRemovable == FALSE) return FALSE;

	// Šù‚ÉƒCƒWƒFƒNƒgÏ‚İ‚È‚ç‘¦I—¹
	if (m_bEnable == FALSE) return FALSE;

	// ƒfƒoƒCƒX§Œä‚ª‚Å‚«‚È‚¯‚ê‚Î‘¦I—¹
	if (m_bDevice == FALSE) return FALSE;

	// ƒfƒoƒCƒXƒI[ƒvƒ“
	OpenDevice();
	if (m_hDevice == INVALID_HANDLE_VALUE) return FALSE;

	// ƒ}ƒEƒ“ƒg‰ğœ
	DWORD nSize;
	BOOL bResult = DeviceIoControl(
		m_hDevice,
		FSCTL_DISMOUNT_VOLUME,
		NULL,
		0,
		NULL,
		0,
		&nSize,
		NULL);

	// ƒCƒWƒFƒNƒg
	bResult = ::DeviceIoControl(
		m_hDevice,
		IOCTL_STORAGE_EJECT_MEDIA,
		NULL,
		0,
		NULL,
		0,
		&nSize,
		NULL);

	// ƒfƒoƒCƒXƒNƒ[ƒY
	CloseDevice();

	return bResult;
}

//---------------------------------------------------------------------------
//
//	ƒ{ƒŠƒ…[ƒ€ƒ‰ƒxƒ‹‚Ìæ“¾
//
//---------------------------------------------------------------------------
void FASTCALL CWinFileDrv::GetVolume(TCHAR* szLabel)
{
	ASSERT(this);
	ASSERT(szLabel);

#if 0	// g‚í‚ê‚È‚¢ƒƒWƒbƒN
	// ƒƒfƒBƒAó‘Ôƒ`ƒFƒbƒN
	if (m_bEnable == FALSE) {
		m_bVolumeCache = FALSE;
		m_szVolumeCache[0] = _T('\0');
		szLabel[0] = _T('\0');
		return;
	}
#endif

	// ƒ{ƒŠƒ…[ƒ€ƒ‰ƒxƒ‹‚Ìæ“¾
	DWORD dwMaximumComponentLength;
	DWORD dwFileSystemFlags;
	BOOL bResult = ::GetVolumeInformation(
		m_szDrive,
		m_szVolumeCache,
		sizeof(m_szVolumeCache) / sizeof(TCHAR),
		NULL,
		&dwMaximumComponentLength,
		&dwFileSystemFlags,
		NULL,
		0);
	if (bResult == 0) {
		m_bVolumeCache = FALSE;
		m_szVolumeCache[0] = _T('\0');
		szLabel[0] = _T('\0');
		return;
	}

	// ƒLƒƒƒbƒVƒ…XV
	m_bVolumeCache = TRUE;
	_tcscpy(szLabel, m_szVolumeCache);
}

//---------------------------------------------------------------------------
//
//	ƒLƒƒƒbƒVƒ…‚©‚çƒ{ƒŠƒ…[ƒ€ƒ‰ƒxƒ‹‚ğæ“¾
//
//	ƒLƒƒƒbƒVƒ…‚³‚ê‚Ä‚¢‚éƒ{ƒŠƒ…[ƒ€ƒ‰ƒxƒ‹î•ñ‚ğ“]‘—‚·‚éB
//	ƒLƒƒƒbƒVƒ…“à—e‚ª—LŒø‚È‚ç TRUE ‚ğA–³Œø‚È‚ç FALSE ‚ğ•Ô‚·B
//
//---------------------------------------------------------------------------
BOOL FASTCALL CWinFileDrv::GetVolumeCache(TCHAR* szLabel) const
{
	ASSERT(this);
	ASSERT(szLabel);

	// “à—e‚ğ“]‘—
	_tcscpy(szLabel, m_szVolumeCache);

	// ƒƒfƒBƒA–¢‘}“ü‚È‚çí‚ÉƒLƒƒƒbƒVƒ…“à—e‚ğg‚¤
	if (m_bEnable == FALSE) return TRUE;

	return m_bVolumeCache;
}

//---------------------------------------------------------------------------
//
//	—e—Ê‚Ìæ“¾
//
//---------------------------------------------------------------------------
DWORD FASTCALL CWinFileDrv::GetCapacity(Human68k::capacity_t* pCapacity)
{
	ASSERT(this);
	ASSERT(pCapacity);

#if 0	// g‚í‚ê‚È‚¢ƒƒWƒbƒN
	// ƒƒfƒBƒAó‘Ôƒ`ƒFƒbƒN
	if (m_bEnable == FALSE) {
		// ƒLƒƒƒbƒVƒ…Á‹
		memset(&m_capCache, 0, sizeof(m_capCache));
		memset(pCapacity, 0, sizeof(*pCapacity));
		return 0;
	}
#endif

	// ULARGE_INTEGERŒ`®‚Åæ“¾(Win95OSR2ˆÈ~AWinNT‚ÅƒTƒ|[ƒg‚³‚ê‚½API)
	ULARGE_INTEGER ulFree;
	ULARGE_INTEGER ulTotal;
	ULARGE_INTEGER ulTotalFree;
	BOOL bResult = ::GetDiskFreeSpaceEx(m_szDrive, &ulFree, &ulTotal, &ulTotalFree);
	if (bResult == FALSE) {
		// ƒLƒƒƒbƒVƒ…Á‹
		memset(&m_capCache, 0, sizeof(m_capCache));
		memset(pCapacity, 0, sizeof(*pCapacity));
		return 0;
	}

	// g—p‰Â”\ƒoƒCƒg”(2GB‚ÅƒNƒŠƒbƒv)
	DWORD nFree = 0x7FFFFFFF;
	if (ulFree.HighPart == 0 && ulFree.LowPart < 0x80000000) {
		nFree = ulFree.LowPart;
	}

	// ƒNƒ‰ƒXƒ^\¬‚ÌŒvZ
	DWORD free;
	DWORD clusters;
	DWORD sectors;
	DWORD bytes;
	if (ulTotal.HighPart == 0 && ulTotal.LowPart < 0x80000000) {
		// 2GB–¢–‚È‚çGetDiskFreeSpace‚ğg‚Á‚Ä’l‚ğæ“¾
		bResult = ::GetDiskFreeSpace(m_szDrive, &sectors, &bytes, &free, &clusters);
		if (bResult == FALSE) {	// ”O‚Ì‚½‚ß
			free = nFree;
			clusters = ulTotal.LowPart;
			sectors = 1;
			bytes = 512;
		}

		// ƒZƒNƒ^ƒTƒCƒY‚ª512‚æ‚è‘å‚«‚¢ƒpƒ^[ƒ“‚ğ•â³ (Œ©‚½‚±‚Æ‚È‚¢‚ª”O‚Ì‚½‚ß)
		while (bytes > 512) {
			bytes >>= 1;
			sectors <<= 1;

			free <<= 1;
		}

		// ƒZƒNƒ^ƒTƒCƒY‚ª256ˆÈ‰º‚Ìƒpƒ^[ƒ“‚ğ•â³ (Œ©‚½‚±‚Æ‚È‚¢‚ª”O‚Ì‚½‚ß)
		while (1 <= bytes && bytes <= 256) {
			bytes <<= 1;
			if (sectors > 1) {
				sectors >>= 1;
			} else {
				clusters >>= 1;
			}

			free >>= 1;
		}

		// ‘ƒNƒ‰ƒXƒ^”‚ª16ƒrƒbƒg‚Ì”ÍˆÍ‚Éû‚Ü‚ç‚È‚¢ƒpƒ^[ƒ“‚ğ•â³
		while (clusters >= 0x10000) {
			clusters >>= 1;
			sectors <<= 1;

			free >>= 1;
		}
	} else {
		// 2GBˆÈã‚È‚çA512(ƒZƒNƒ^)~128(ƒNƒ‰ƒXƒ^)~32768‚ÉŒÅ’è
		sectors = 0x80;
		clusters = 0x8000;
		bytes = 512;

		// ‹ó‚«—e—Ê‚ª2GB‚ğ’´‚¦‚é‚±‚Æ‚Í–³‚¢
		free = (WORD)(nFree >> 16);
		if (nFree & 0xFFFF) free++;
	}

	// ƒpƒ‰ƒ[ƒ^”ÍˆÍƒ`ƒFƒbƒN (”O‚Ì‚½‚ß)
	ASSERT(free < 0x10000);
	ASSERT(clusters < 0x10000);
	ASSERT(sectors < 0x10000);
	ASSERT(bytes == 512);

	// ‘Sƒpƒ‰ƒ[ƒ^ƒZƒbƒg
	m_capCache.free = (WORD)free;
	m_capCache.clusters = (WORD)clusters;
	m_capCache.sectors = (WORD)sectors;
	m_capCache.bytes = 512;

	// “à—e‚ğ“]‘—
	memcpy(pCapacity, &m_capCache, sizeof(m_capCache));

	return nFree;
}

//---------------------------------------------------------------------------
//
//	ƒLƒƒƒbƒVƒ…‚©‚çƒNƒ‰ƒXƒ^ƒTƒCƒY‚ğæ“¾
//
//---------------------------------------------------------------------------
BOOL FASTCALL CWinFileDrv::GetCapacityCache(Human68k::capacity_t* pCapacity) const
{
	ASSERT(this);
	ASSERT(pCapacity);

	// “à—e‚ğ“]‘—
	memcpy(pCapacity, &m_capCache, sizeof(m_capCache));

	// ƒƒfƒBƒA–¢‘}“ü‚È‚çí‚ÉƒLƒƒƒbƒVƒ…“à—e‚ğg‚¤
	if (m_bEnable == FALSE) return TRUE;

	return (m_capCache.sectors != 0);
}

//===========================================================================
//
//	ƒfƒBƒŒƒNƒgƒŠƒGƒ“ƒgƒŠ ƒtƒ@ƒCƒ‹–¼
//
//===========================================================================

DWORD CHostFilename::g_nOption;			// “®ìƒtƒ‰ƒO (ƒtƒ@ƒCƒ‹–¼•ÏŠ·ŠÖ˜A)

//---------------------------------------------------------------------------
//
//	ƒRƒ“ƒXƒgƒ‰ƒNƒ^
//
//---------------------------------------------------------------------------
CHostFilename::CHostFilename()
{
	m_pszWin32 = NULL;
	m_bCorrect = FALSE;
	m_szHuman[0] = '\0';
	// memset(&m_dirEntry, 0, sizeof(m_dirEntry));		// o‘Ov‘¬‰Šú‰»‚Í•s—v
	m_pszHumanLast = m_szHuman;
	m_pszHumanExt = m_szHuman;
	m_pEntry = NULL;
	m_pChild = NULL;
}

//---------------------------------------------------------------------------
//
//	ƒfƒXƒgƒ‰ƒNƒ^
//
//---------------------------------------------------------------------------
CHostFilename::~CHostFilename()
{
	FreeWin32();
	DeleteCache();
}

//---------------------------------------------------------------------------
//
//	Win32‘¤‚Ì–¼Ì‚Ìİ’è
//
//---------------------------------------------------------------------------
void FASTCALL CHostFilename::SetWin32(const TCHAR* szWin32)
{
	ASSERT(this);
	ASSERT(szWin32);
	ASSERT(_tcslen(szWin32) < _MAX_PATH);
	ASSERT(m_pszWin32 == NULL);

	// Win32–¼ÌƒRƒs[
	m_pszWin32 = (TCHAR*)malloc(_tcslen(szWin32) + 1);
	ASSERT(m_pszWin32);
	_tcscpy(m_pszWin32, szWin32);
}

//---------------------------------------------------------------------------
//
//	Human68k‘¤‚Ìƒtƒ@ƒCƒ‹–¼—v‘f‚ğƒRƒs[
//
//---------------------------------------------------------------------------
BYTE* FASTCALL CHostFilename::CopyName(BYTE* pWrite, const BYTE* pFirst, const BYTE* pLast)
{
	ASSERT(this);
	ASSERT(pWrite);
	ASSERT(pFirst);
	ASSERT(pLast);

	for (const BYTE* p = pFirst; p < pLast; p++) {
		*pWrite++ = *p;
	}

	return pWrite;
}

//---------------------------------------------------------------------------
//
//	Human68k‘¤‚Ì–¼Ì‚ğ•ÏŠ·
//
//	‚ ‚ç‚©‚¶‚ß SetWin32() ‚ğÀs‚µ‚Ä‚¨‚­‚±‚ÆB
//	18+3‚Ì–½–¼‹K‘¥‚É]‚Á‚½–¼‘O•ÏŠ·‚ğs‚È‚¤B
//	ƒtƒ@ƒCƒ‹–¼æ“ª‚¨‚æ‚Ñ––”ö‚Ì‹ó”’‚ÍAHuman68k‚Åˆµ‚¦‚È‚¢‚½‚ß©“®“I‚Éíœ‚³‚ê‚éB
//	ƒfƒBƒŒƒNƒgƒŠƒGƒ“ƒgƒŠ‚Ì–¼‘O•”•ª‚ğAƒtƒ@ƒCƒ‹–¼•ÏŠ·‚ÌŠg’£q‚ÌˆÊ’uî•ñ‚ğg‚Á‚Ä¶¬‚·‚éB
//	‚»‚ÌŒãAƒtƒ@ƒCƒ‹–¼‚ÌˆÙí”»’è‚ğs‚È‚¤B(ƒXƒy[ƒX8•¶š‚¾‚¯‚Ìƒtƒ@ƒCƒ‹–¼‚È‚Ç)
//	ƒtƒ@ƒCƒ‹–¼‚Ìd•¡”»’è‚Ís‚È‚í‚È‚¢‚Ì‚Å’ˆÓB‚±‚ê‚ç‚Ì”»’è‚ÍãˆÊƒNƒ‰ƒX‚Ås‚È‚¤B
//	TwentyOne version 1.36c modified +14 patchlevel9 ˆÈ~‚ÌŠg’£q‹K‘¥‚É‘Î‰‚³‚¹‚éB
//
//---------------------------------------------------------------------------
void FASTCALL CHostFilename::SetHuman(int nCount)
{
	ASSERT(this);
	ASSERT(m_pszWin32);
	ASSERT(_tcslen(m_pszWin32) < _MAX_PATH);

	// ƒTƒuƒfƒBƒŒƒNƒgƒŠ–¼‚Ìê‡‚Í•ÏŠ·‚µ‚È‚¢
	if (m_pszWin32[0] == '.') {
		if (m_pszWin32[1] == '\0' || (m_pszWin32[1] == '.' && m_pszWin32[2] == '\0')) {
			strcpy((char*)m_szHuman, (char*)m_pszWin32);	// WARNING: Unicode—vC³

			m_bCorrect = TRUE;
			m_pszHumanLast = m_szHuman + strlen((char*)m_szHuman);
			m_pszHumanExt = m_pszHumanLast;
			return;
		}
	}

	DWORD nMax = 18;	// ƒx[ƒX•”•ª(ƒx[ƒX–¼‚ÆŠg’£q–¼)‚ÌƒoƒCƒg”
	if (g_nOption & WINDRV_OPT_CONVERT_LENGTH) nMax = 8;

	// ƒx[ƒX–¼•”•ª‚Ì•â³€”õ
	BYTE szNumber[8];
	BYTE* pNumber = NULL;
	if (nCount >= 0) {
		pNumber = &szNumber[8];
		for (int i = 0; i < 5; i++) {	// Å‘å5+1Œ…‚Ü‚Å (ƒx[ƒX–¼æ“ª2ƒoƒCƒg‚Í•K‚¸c‚·)
			int n = nCount % 36;
			nMax--;
			pNumber--;
			*pNumber = (BYTE)n + (n < 10 ? '0' : 'A' - 10);
			nCount /= 36;
			if (nCount == 0) break;
		}
		nMax--;
		pNumber--;
		BYTE c = (BYTE)(g_nOption >> 24) & 0x7F;
		if (c == 0) c = XM6_HOST_FILENAME_MARK;
		*pNumber = c;
	}

	// •¶š•ÏŠ·
	// WARNING: Unicode–¢‘Î‰B‚¢‚¸‚êUnicode‚Ì¢ŠE‚ÉŸZ‚Ü‚ê‚½‚Í‚±‚±‚Å•ÏŠ·‚ğs‚È‚¤
	BYTE szHuman[_MAX_PATH];
	const BYTE* pFirst = szHuman;
	const BYTE* pLast;
	const BYTE* pExt = NULL;

	{
		const BYTE* pRead = (const BYTE*)m_pszWin32;
		BYTE* pWrite = szHuman;
		const BYTE* pPeriod = SeparateExt(pRead);

		for (bool bFirst = true; ; bFirst = false) {
			BYTE c = *pRead++;
			switch (c) {
			case ' ':
				if (g_nOption & WINDRV_OPT_REDUCED_SPACE) continue;
				if (g_nOption & WINDRV_OPT_CONVERT_SPACE) c = '_';
				else if (pWrite == szHuman) continue;	// æ“ª‚Ì‹ó”’‚Í–³‹
				break;
			case '=':
			case '+':
				if (g_nOption & WINDRV_OPT_REDUCED_BADCHAR) continue;
				if (g_nOption & WINDRV_OPT_CONVERT_BADCHAR) c = '_';
				break;
			case '-':
				if (bFirst) {
					if (g_nOption & WINDRV_OPT_REDUCED_HYPHEN) continue;
					if (g_nOption & WINDRV_OPT_CONVERT_HYPHEN) c = '_';
					break;
				}
				if (g_nOption & WINDRV_OPT_REDUCED_HYPHENS) continue;
				if (g_nOption & WINDRV_OPT_CONVERT_HYPHENS) c = '_';
				break;
			case '.':
				if (pRead - 1 == pPeriod) {		// Human68kŠg’£q‚Í—áŠO‚Æ‚·‚é
					pExt = pWrite;
					break;
				}
				if (bFirst) {
					if (g_nOption & WINDRV_OPT_REDUCED_PERIOD) continue;
					if (g_nOption & WINDRV_OPT_CONVERT_PERIOD) c = '_';
					break;
				}
				if (g_nOption & WINDRV_OPT_REDUCED_PERIODS) continue;
				if (g_nOption & WINDRV_OPT_CONVERT_PERIODS) c = '_';
				break;
			}
			*pWrite++ = c;
			if (c == '\0') break;
		}

		pLast = pWrite - 1;
	}

	// Šg’£q•â³
	if (pExt) {
		// ––”ö‚Ì‹ó”’‚ğíœ‚·‚é
		while(pExt < pLast - 1 && *(pLast - 1) == ' ') {
			pLast--;
			BYTE* p = (BYTE*)pLast;
			*p = '\0';
		}

		// •ÏŠ·Œã‚ÉÀ‘Ì‚ª‚È‚­‚È‚Á‚½ê‡‚Ííœ
		if (pExt + 1 >= pLast) {
			pLast = pExt;
			BYTE* p = (BYTE*)pLast;
			*p = '\0';		// ”O‚Ì‚½‚ß
		}
	} else {
		pExt = pLast;
	}

	// “oêl•¨Ğ‰î
	//
	// pFirst: ‰´‚ÍƒŠ[ƒ_[Bƒtƒ@ƒCƒ‹–¼æ“ª
	// pCut: ’ÊÌƒtƒFƒCƒXBÅ‰‚ÌƒsƒŠƒIƒh‚ÌoŒ»ˆÊ’u ‚»‚ÌŒãƒx[ƒX–¼I’[ˆÊ’u‚Æ‚È‚é
	// pSecond: ‚æ‚§‚¨‚Ü‚¿‚Ç‚¤B‰´—l‚±‚»ƒ}[ƒhƒbƒNBŠg’£q–¼‚ÌŠJnˆÊ’uB‚¾‚©‚ç‰½B
	// pExt: BEAEƒoƒ‰ƒJƒXBHuman68kŠg’£q‚Ì“VË‚¾B‚Å‚àA3•¶š‚æ‚è’·‚¢–¼‘O‚ÍŠ¨•Ù‚ÈB
	// ÅŒã‚ÌƒsƒŠƒIƒh‚ÌoŒ»ˆÊ’u ŠY“–‚µ‚È‚¯‚ê‚ÎpLast‚Æ“¯‚¶’l
	//
	// «pFirst            «pStop «pSecond ©                «pExt
	// T h i s _ i s _ a . V e r y . L o n g . F i l e n a m e . t x t \0
	//         ªpCut © ªpCut‰ŠúˆÊ’u                                ªpLast
	//
	// ã‹L‚Ìê‡A•ÏŠ·Œã‚Í This.Long.Filename.txt ‚Æ‚È‚é

	// 1•¶š–Ú”»’è
	const BYTE* pCut = pFirst;
	const BYTE* pStop = pExt - nMax;	// Šg’£q–¼‚ÍÅ‘å17ƒoƒCƒg‚Æ‚·‚é(ƒx[ƒX–¼‚ğc‚·)
	if (pFirst < pExt) {
		pCut++;		// •K‚¸1ƒoƒCƒg‚Íƒx[ƒX–¼‚ğg‚¤
		BYTE c = *pFirst;
		if ((0x80 <= c && c <= 0x9F) || 0xE0 <= c) {	// Œµ–§‚É‚Í 0x81`0x9F 0xE0`0xEF
			pCut++;		// ƒx[ƒX–¼ Å¬2ƒoƒCƒg
			pStop++;	// Šg’£q–¼ Å‘å16ƒoƒCƒg
		}
	}
	if (pStop < pFirst) pStop = pFirst;

	// ƒx[ƒX–¼”»’è
	pCut = (BYTE*)strchr((char*)pCut, '.');	// SJIS2ƒoƒCƒg–Ú‚Í•K‚¸0x40ˆÈã‚È‚Ì‚Å–â‘è‚È‚¢
	if (pCut == NULL) pCut = pLast;
	if ((DWORD)(pCut - pFirst) > nMax) {
		pCut = pFirst + nMax;	// Œã‚Ù‚ÇSJIS2ƒoƒCƒg”»’è/•â³‚ğs‚È‚¤ ‚±‚±‚Å”»’è‚µ‚Ä‚Í‚¢‚¯‚È‚¢
	}

	// Šg’£q–¼”»’è
	const BYTE* pSecond = pExt;
	const BYTE* p = pExt - 1;
	for (const BYTE* p = pExt - 1; pStop < p; p--) {
		if (*p == '.') pSecond = p;	// SJIS2ƒoƒCƒg–Ú‚Í•K‚¸0x40ˆÈã‚È‚Ì‚Å–â‘è‚È‚¢
	}

	// ƒx[ƒX–¼‚ğ’Zk
	DWORD nExt = pExt - pSecond;	// Šg’£q–¼•”•ª‚Ì’·‚³
	if ((DWORD)(pCut - pFirst) + nExt > nMax) pCut = pFirst + nMax - nExt;
	// 2ƒoƒCƒg•¶š‚Ì“r’†‚È‚ç‚³‚ç‚É’Zk
	for (p = pFirst; p < pCut; p++) {
		BYTE c = *p;
		if ((0x80 <= c && c <= 0x9F) || 0xE0 <= c) {	// Œµ–§‚É‚Í 0x81`0x9F 0xE0`0xEF
			p++;
			if (p >= pCut) {
				pCut--;
				break;
			}
		}
	}

	// –¼‘O‚ÌŒ‹‡
	BYTE* pWrite = m_szHuman;
	pWrite = CopyName(pWrite, pFirst, pCut);	// ƒx[ƒX–¼‚ğ“]‘—
	if (pNumber) pWrite = CopyName(pWrite, pNumber, &szNumber[8]);	// •â³•¶š‚ğ“]‘—
	pWrite = CopyName(pWrite, pSecond, pExt);	// Šg’£q–¼‚ğ“]‘—
	m_pszHumanExt = pWrite;						// Šg’£qˆÊ’u•Û‘¶
	pWrite = CopyName(pWrite, pExt, pLast);		// Human68kŠg’£q‚ğ“]‘—
	m_pszHumanLast = pWrite;					// I’[ˆÊ’u•Û‘¶
	*pWrite = '\0';

	// •ÏŠ·Œ‹‰Ê‚ÌŠm”F
	m_bCorrect = TRUE;
	int nSize = m_pszHumanExt - m_szHuman;				// Šg’£q‚ÌˆÊ’u‚ğ•Û‘¶

	// ƒtƒ@ƒCƒ‹–¼–{‘Ì‚ª‘¶İ‚µ‚È‚¯‚ê‚Î•s‡Ši
	if (nSize <= 0) m_bCorrect = FALSE;

	// ƒtƒ@ƒCƒ‹–¼–{‘Ì‚ª1•¶šˆÈã‚Å‚©‚Â‹ó”’‚ÅI—¹‚µ‚Ä‚¢‚ê‚Î•s‡Ši
	// ƒtƒ@ƒCƒ‹–¼–{‘Ì‚ª8•¶šˆÈã‚Ìê‡A—˜_ã‚Í‹ó”’‚Å‚ÌI—¹‚ª•\Œ»‰Â
	// ”\‚¾‚ªAHuman68k‚Å‚Í³‚µ‚­ˆµ‚¦‚È‚¢‚½‚ßA‚±‚ê‚à•s‡Ši‚Æ‚·‚é
	else if (m_szHuman[nSize - 1] == ' ') m_bCorrect = FALSE;

	// •ÏŠ·Œ‹‰Ê‚ªƒfƒBƒŒƒNƒgƒŠ–¼‚Æ“¯‚¶‚È‚ç•s‡Ši
	if (m_szHuman[0] == '.') {
		if (m_szHuman[1] == '\0' || (m_szHuman[1] == '.' && m_szHuman[2] == '\0')) {
			m_bCorrect = FALSE;
		}
	}
}

//---------------------------------------------------------------------------
//
//	Human68k‘¤‚Ì–¼Ì‚ğ•¡»
//
//	ƒtƒ@ƒCƒ‹–¼•”•ª‚Ìî•ñ‚ğ•¡»‚µASetHuman() ‘Š“–‚Ì‰Šú‰»“®ì‚ğs‚È‚¤B
//
//---------------------------------------------------------------------------
void FASTCALL CHostFilename::CopyHuman(const BYTE* szHuman)
{
	ASSERT(this);
	ASSERT(szHuman);
	ASSERT(strlen((char*)szHuman) < 23);

	strcpy((char*)m_szHuman, (char*)szHuman);
	m_bCorrect = TRUE;
	m_pszHumanLast = m_szHuman + strlen((char*)m_szHuman);
	m_pszHumanExt = (BYTE*)SeparateExt(m_szHuman);
}

//---------------------------------------------------------------------------
//
//	ƒfƒBƒŒƒNƒgƒŠƒGƒ“ƒgƒŠî•ñ‚Ìİ’è
//
//	ƒzƒXƒg‘¤Find‚ÌŒ‹‰Ê‚©‚çA‘®«EƒTƒCƒYE“ú•tî•ñ‚ğƒfƒBƒŒƒNƒgƒŠƒGƒ“
//	ƒgƒŠ‚É”½‰f‚·‚éBƒtƒ@ƒCƒ‹–¼î•ñ‚ÍFind‚ÌŒ‹‰Ê‚Å‚Í‚È‚­ƒNƒ‰ƒX•Ï”‚Éİ
//	’èÏ‚İ‚Ìƒf[ƒ^‚ğ”½‰f‚·‚é‚½‚ßA‚ ‚ç‚©‚¶‚ß SetHuman() ‚ğÀs‚µ‚Ä‚¨
//	‚­‚±‚Æ(Às‚µ‚Ä‚¢‚È‚­‚Ä‚à“Á‚É•s‹ï‡‚Í‹N‚«‚È‚¢‚ª)B
//
//---------------------------------------------------------------------------
BOOL FASTCALL CHostFilename::SetEntry(const WIN32_FIND_DATA* pWin32Find)
{
	ASSERT(this);
	ASSERT(pWin32Find);

	// ƒtƒ@ƒCƒ‹–¼İ’è
	BYTE* p = m_szHuman;
	for (int i = 0; i < 8; i++) {
		if (p < m_pszHumanExt)
			m_dirEntry.name[i] = *p++;
		else
			m_dirEntry.name[i] = ' ';
	}

	for (int i = 0; i < 10; i++) {
		if (p < m_pszHumanExt)
			m_dirEntry.add[i] = *p++;
		else
			m_dirEntry.add[i] = '\0';
	}

	if (*p == '.') p++;
	for (int i = 0; i < 3; i++) {
		BYTE c = *p;
		if (c) p++;
		m_dirEntry.ext[i] = c;
	}

	// ‘®«İ’è
	DWORD n = pWin32Find->dwFileAttributes;
	BYTE nHumanAttribute = Human68k::AT_ARCHIVE;
	if ((n & FILE_ATTRIBUTE_DIRECTORY) != 0) {
		nHumanAttribute = Human68k::AT_DIRECTORY;
	}
	if ((n & FILE_ATTRIBUTE_SYSTEM) != 0) nHumanAttribute |= Human68k::AT_SYSTEM;
	if ((n & FILE_ATTRIBUTE_HIDDEN) != 0) nHumanAttribute |= Human68k::AT_HIDDEN;
	if ((n & FILE_ATTRIBUTE_READONLY) != 0) nHumanAttribute |= Human68k::AT_READONLY;
	m_dirEntry.attr = nHumanAttribute;

	// ƒTƒCƒYİ’è
	m_dirEntry.size = pWin32Find->nFileSizeLow;
	if (pWin32Find->nFileSizeHigh > 0 || pWin32Find->nFileSizeLow > XM6_HOST_FILESIZE_MAX)
		m_dirEntry.size = XM6_HOST_FILESIZE_MAX;

	// “ú•tEİ’è
	m_dirEntry.date = 0;
	m_dirEntry.time = 0;
	FILETIME ft = pWin32Find->ftLastWriteTime;
	if (ft.dwLowDateTime == 0 && ft.dwHighDateTime == 0) {
		ft = pWin32Find->ftCreationTime;
	}
	FILETIME lt;
	if (::FileTimeToLocalFileTime(&ft, &lt) == 0) return FALSE;
	if (::FileTimeToDosDateTime(&lt, &m_dirEntry.date, &m_dirEntry.time) == 0) return FALSE;

	// ƒNƒ‰ƒXƒ^”Ô†İ’è
	m_dirEntry.cluster = 0;

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	Human68k‘¤‚Ì–¼Ì‚ª‰ÁH‚³‚ê‚½‚©’²¸
//
//---------------------------------------------------------------------------
BOOL FASTCALL CHostFilename::isReduce() const
{
	ASSERT(this);
	ASSERT(m_pszWin32);

	return strcmp((char*)m_pszWin32, (char*)m_szHuman) != 0; // WARNING: Unicode—vC³
}

//---------------------------------------------------------------------------
//
//	ŠY“–ƒGƒ“ƒgƒŠ‚ÌƒLƒƒƒbƒVƒ…‚ğíœ
//
//---------------------------------------------------------------------------
void FASTCALL CHostFilename::DeleteCache()
{
	ASSERT(this);

	if (m_pEntry) m_pEntry->DeleteCache(m_pChild);
}

//---------------------------------------------------------------------------
//
//	Human68kƒtƒ@ƒCƒ‹–¼‚©‚çŠg’£q‚ğ•ª—£
//
//---------------------------------------------------------------------------
const BYTE* FASTCALL CHostFilename::SeparateExt(const BYTE* szHuman)
{
	// ƒtƒ@ƒCƒ‹–¼‚Ì’·‚³‚ğŠl“¾
	int nLength = strlen((char*)szHuman);
	const BYTE* pFirst = szHuman;
	const BYTE* pLast = pFirst + nLength;

	// Human68kŠg’£q‚ÌˆÊ’u‚ğŠm”F
	const BYTE* pExt = (BYTE*)strrchr((char*)pFirst, '.');	// SJIS2ƒoƒCƒg–Ú‚Í•K‚¸0x40ˆÈã‚È‚Ì‚Å–â‘è‚È‚¢
	if (pExt == NULL) pExt = pLast;
	// ƒtƒ@ƒCƒ‹–¼‚ª20`22•¶š‚©‚Â19•¶š–Ú‚ª'.'‚©‚Â'.'‚ÅI—¹‚Æ‚¢‚¤ƒpƒ^[ƒ“‚ğ“Á•Êˆµ‚¢‚·‚é
	if (20 <= nLength && nLength <= 22 && pFirst[18] == '.' && pFirst[nLength-1] == '.')
		pExt = pFirst + 18;
	// Šg’£q‚Ì•¶š”‚ğŒvZ	(-1:‚È‚µ 0:ƒsƒŠƒIƒh‚¾‚¯ 1`3:Human68kŠg’£q 4ˆÈã:Šg’£q–¼)
	int nExt = pLast - pExt - 1;
	// '.' ‚ª•¶š—ñæ“ªˆÈŠO‚É‘¶İ‚µ‚ÄA‚©‚Â1`3•¶š‚Ìê‡‚Ì‚İŠg’£q‚Æ‚İ‚È‚·
	if (pExt == pFirst || nExt < 1 || nExt > 3) pExt = pLast;

	return pExt;
}

//===========================================================================
//
//	ƒfƒBƒŒƒNƒgƒŠƒGƒ“ƒgƒŠ ƒpƒX–¼
//
//	Human68k‘¤‚ÌƒpƒX–¼‚ÍA•K‚¸æ“ª‚ª / ‚Ån‚Ü‚èA––”ö‚É / ‚ª‚Â‚©‚È‚¢B
//	(––”ö‚ª / ‚ÅI‚í‚éê‡‚Í‹ó‚Ìƒtƒ@ƒCƒ‹–¼w’è‚Æ”»’f‚³‚ê‚é)
//	‚Ü‚½Aƒ†ƒjƒbƒg”Ô†‚ğ‚¿Aí‚ÉƒyƒA‚Åˆµ‚¤B
//	]—ˆ‚Ì mfc_host ‚Å‚Ìˆµ‚¢‚ÆˆÙ‚È‚é‚Ì‚Å’ˆÓB
//	‚‘¬‰»‚Ì‚½‚ßAWin32ƒpƒX–¼‚Íƒx[ƒXƒpƒX•”•ª‚àŠÜ‚ŞB
//	ƒx[ƒXƒpƒX•”•ª‚È‚µ‚Å‚à“®ì‰Â”\‚Æ‚·‚éB
//
//===========================================================================

DWORD CHostPath::g_nId;				// ¯•ÊID¶¬—pƒJƒEƒ“ƒ^
DWORD CHostPath::g_nOption;			// “®ìƒtƒ‰ƒO (ƒpƒX–¼”äŠrŠÖ˜A)

//---------------------------------------------------------------------------
//
//	ƒRƒ“ƒXƒgƒ‰ƒNƒ^
//
//---------------------------------------------------------------------------
CHostPath::CHostPath()
{
	m_nHumanUnit = 0;
	m_pszHuman = NULL;
	m_pszWin32 = NULL;
	m_hChange = INVALID_HANDLE_VALUE;
	m_nId = g_nId++;
}

//---------------------------------------------------------------------------
//
//	ƒfƒXƒgƒ‰ƒNƒ^
//
//---------------------------------------------------------------------------
CHostPath::~CHostPath()
{
#if 0
	OutputDebugString("Cache Delete ");
	OutputDebugString(m_pszWin32);
	OutputDebugString("\r\n");
#endif

	Clean();
}

//---------------------------------------------------------------------------
//
//	Human68k‘¤‚Ì–¼Ì‚ğ’¼Úw’è‚·‚é
//
//---------------------------------------------------------------------------
void FASTCALL CHostPath::SetHuman(DWORD nUnit, BYTE* szHuman)
{
	ASSERT(this);
	ASSERT(szHuman);
	ASSERT(strlen((char*)szHuman) < HUMAN68K_MAX_PATH);
	ASSERT(m_pszHuman == NULL);

	m_pszHuman = (BYTE*)malloc(strlen((char*)szHuman) + 1);
	ASSERT(m_pszHuman);
	strcpy((char*)m_pszHuman, (char*)szHuman);
	m_nHumanUnit = nUnit;
}

//---------------------------------------------------------------------------
//
//	Win32‘¤‚Ì–¼Ì‚ğ’¼Úw’è‚·‚é
//
//---------------------------------------------------------------------------
void FASTCALL CHostPath::SetWin32(TCHAR* szWin32)
{
	ASSERT(this);
	ASSERT(szWin32);
	ASSERT(_tcslen(szWin32) < _MAX_PATH);
	ASSERT(m_pszWin32 == NULL);

	m_pszWin32 = (TCHAR*)malloc(_tcslen(szWin32) + 1);
	ASSERT(m_pszWin32);
	_tcscpy(m_pszWin32, szWin32);
}

//---------------------------------------------------------------------------
//
//	•¶š—ñ”äŠr (ƒƒCƒ‹ƒhƒJ[ƒh‘Î‰)
//
//---------------------------------------------------------------------------
int FASTCALL CHostPath::Compare(const BYTE* pFirst, const BYTE* pLast, const BYTE* pBufFirst, const BYTE* pBufLast)
{
	ASSERT(pFirst);
	ASSERT(pLast);
	ASSERT(pBufFirst);
	ASSERT(pBufLast);

	// •¶š”äŠr
	BOOL bSkip0 = FALSE;
	BOOL bSkip1 = FALSE;
	for (const BYTE* p = pFirst; p < pLast; p++) {
		// 1•¶š“Ç‚İ‚İ
		BYTE c = *p;
		BYTE d = '\0';
		if (pBufFirst < pBufLast) d = *pBufFirst++;

		// ”äŠr‚Ì‚½‚ß‚Ì•¶š•â³
		if (bSkip0 == FALSE) {
			if (bSkip1 == FALSE) {	// c‚àd‚à1ƒoƒCƒg–Ú
				if ((0x80 <= c && c <= 0x9F) || 0xE0 <= c) {	// Œµ–§‚É‚Í 0x81`0x9F 0xE0`0xEF
					bSkip0 = TRUE;
				}
				if ((0x80 <= d && d <= 0x9F) || 0xE0 <= d) {	// Œµ–§‚É‚Í 0x81`0x9F 0xE0`0xEF
					bSkip1 = TRUE;
				}
				if (c == d) continue;	// ‚Šm—¦‚Å”»’èŠ®—¹‚·‚é
				if ((g_nOption & WINDRV_OPT_ALPHABET) == 0) {
					if ('A' <= c && c <= 'Z') c += 'a' - 'A';	// ¬•¶š‰»
					if ('A' <= d && d <= 'Z') d += 'a' - 'A';	// ¬•¶š‰»
				}
			} else {		// c‚¾‚¯‚ª1ƒoƒCƒg–Ú
				if ((0x80 <= c && c <= 0x9F) || 0xE0 <= c) {	// Œµ–§‚É‚Í 0x81`0x9F 0xE0`0xEF
					bSkip0 = TRUE;
				}
				bSkip1 = FALSE;
			}
		} else {
			if (bSkip1 == FALSE) {	// d‚¾‚¯‚ª1ƒoƒCƒg–Ú
				bSkip0 = FALSE;
				if ((0x80 <= d && d <= 0x9F) || 0xE0 <= d) {	// Œµ–§‚É‚Í 0x81`0x9F 0xE0`0xEF
					bSkip1 = TRUE;
				}
			} else {		// c‚àd‚à2ƒoƒCƒg–Ú
				bSkip0 = FALSE;
				bSkip1 = FALSE;
			}
		}

		// ”äŠr
		if (c == d) continue;
		if (c == '?') continue;
		//if (c == '*') return 0;
		return 1;
	}
	if (pBufFirst < pBufLast) return 2;

	return 0;
}

//---------------------------------------------------------------------------
//
//	Human68k‘¤‚Ì–¼Ì‚ğ”äŠr‚·‚é
//
//---------------------------------------------------------------------------
BOOL FASTCALL CHostPath::isSameHuman(DWORD nUnit, const BYTE* szHuman) const
{
	ASSERT(this);
	ASSERT(szHuman);
	ASSERT(m_pszHuman);

	// ƒ†ƒjƒbƒg”Ô†‚Ì”äŠr
	if (m_nHumanUnit != nUnit) return FALSE;

	// •¶š”ŒvZ
	const BYTE* pFirst = m_pszHuman;
	DWORD nLength = strlen((char*)pFirst);
	const BYTE* pLast = pFirst + nLength;

	const BYTE* pBufFirst = szHuman;
	DWORD nBufLength = strlen((char*)pBufFirst);
	const BYTE* pBufLast = pBufFirst + nBufLength;

	// •¶š”ƒ`ƒFƒbƒN
	if (nLength != nBufLength) return FALSE;

	// Human68kƒpƒX–¼‚Ì”äŠr
	return Compare(pFirst, pLast, pBufFirst, pBufLast) == 0;
}

//---------------------------------------------------------------------------
//
//	ƒtƒ@ƒCƒ‹–¼‚ğŒŸõ
//
//	Š—L‚·‚éƒLƒƒƒVƒ…ƒoƒbƒtƒ@‚Ì’†‚©‚çŒŸõ‚µAŒ©‚Â‚©‚ê‚Î‚»‚Ì–¼Ì‚ğ•Ô‚·B
//	ƒpƒX–¼‚ğœŠO‚µ‚Ä‚¨‚­‚±‚ÆB
//	•K‚¸ãˆÊ‚Å”r‘¼§Œä‚ğs‚È‚¤‚±‚ÆB
//
//---------------------------------------------------------------------------
CHostFilename* FASTCALL CHostPath::FindFilename(BYTE* szHuman, DWORD nHumanAttribute)
{
	ASSERT(this);
	ASSERT(szHuman);

	// •¶š”ŒvZ
	const BYTE* pFirst = szHuman;
	DWORD nLength = strlen((char*)pFirst);
	const BYTE* pLast = pFirst + nLength;

	// Š‚µ‚Ä‚¢‚é‘S‚Ä‚Ìƒtƒ@ƒCƒ‹–¼‚Ì’†‚©‚çŠ®‘Sˆê’v‚·‚é‚à‚Ì‚ğŒŸõ
	for (CHostFilename* p = (CHostFilename*)m_cRingFilename.Next();
		 p != &m_cRingFilename; p = (CHostFilename*)p->Next()) {
		// ‘®«ƒ`ƒFƒbƒN
		if (p->CheckAttribute(nHumanAttribute) == 0) continue;
		// •¶š”ŒvZ
		const BYTE* pBufFirst = p->GetHuman();
		const BYTE* pBufLast = p->GetHumanLast();
		DWORD nBufLength = pBufLast - pBufFirst;
		// •¶š”ƒ`ƒFƒbƒN
		if (nLength != nBufLength) continue;
		// ƒtƒ@ƒCƒ‹–¼ƒ`ƒFƒbƒN
		if (Compare(pFirst, pLast, pBufFirst, pBufLast) == 0) return p;
	}

	return NULL;
}

//---------------------------------------------------------------------------
//
//	ƒtƒ@ƒCƒ‹–¼‚ğŒŸõ (ƒƒCƒ‹ƒhƒJ[ƒh‘Î‰)
//
//	Š—L‚·‚éƒoƒbƒtƒ@‚Ì’†‚©‚çŒŸõ‚µAŒ©‚Â‚©‚ê‚Î‚»‚Ì–¼Ì‚ğ•Ô‚·B
//	ƒpƒX–¼‚ğœŠO‚µ‚Ä‚¨‚­‚±‚ÆB
//	•K‚¸ãˆÊ‚Å”r‘¼§Œä‚ğs‚È‚¤‚±‚ÆB
//
//---------------------------------------------------------------------------
CHostFilename* FASTCALL CHostPath::FindFilenameWildcard(BYTE* szHuman, find_t* pFind, DWORD nHumanAttribute)
{
	ASSERT(this);
	ASSERT(szHuman);
	ASSERT(pFind);

	// ŒŸõƒtƒ@ƒCƒ‹–¼‚ğ–{‘Ì‚ÆHuman68kŠg’£q‚É•ª‚¯‚é
	const BYTE* pFirst = szHuman;
	const BYTE* pLast = pFirst + strlen((char*)pFirst);
	const BYTE* pExt = CHostFilename::SeparateExt(pFirst);

	// ŠJn’n“_‚ÖˆÚ“®
	CHostFilename* p = (CHostFilename*)m_cRingFilename.Next();
	if (pFind->count > 0) {
		if (pFind->id == m_nId) {
			// ƒLƒƒƒbƒVƒ…“à—e‚É•Ï‰»‚ª‚È‚¢ê‡‚ÍA‘O‰ñ‚ÌˆÊ’u‚©‚ç‘¦ŒŸõ
			p = pFind->pos;
		} else {
			// ŠJn’n“_‚ğƒGƒ“ƒgƒŠ“à—e‚©‚çŒŸõ‚·‚é
			DWORD n = 0;
			for (; p != &m_cRingFilename; p = (CHostFilename*)p->Next()) {
				if (p->isSameEntry(&pFind->entry)) {
					pFind->count = n;
					break;
				}
				n++;
			}

			// ŠY“–ƒGƒ“ƒgƒŠ‚ªŒ©‚Â‚©‚ç‚È‚©‚Á‚½ê‡A‰ñ”w’è‚ğg‚¤
			if (p == &m_cRingFilename) {
				CHostFilename* p = (CHostFilename*)m_cRingFilename.Next();
				n = 0;
				for (; p != &m_cRingFilename; p = (CHostFilename*)p->Next()) {
					if (n >= pFind->count) break;
					n++;
				}
			}
		}
	}

	// ƒtƒ@ƒCƒ‹ŒŸõ
	for (; p != &m_cRingFilename; p = (CHostFilename*)p->Next()) {
		pFind->count++;

		// ‘®«ƒ`ƒFƒbƒN
		if (p->CheckAttribute(nHumanAttribute) == 0) continue;

		// ƒtƒ@ƒCƒ‹–¼‚ğ–{‘Ì‚ÆHuman68kŠg’£q‚É•ª‚¯‚é
		const BYTE* pBufFirst = p->GetHuman();
		const BYTE* pBufLast = p->GetHumanLast();
		const BYTE* pBufExt = p->GetHumanExt();

		// –{‘Ì”äŠr
		if (Compare(pFirst, pExt, pBufFirst, pBufExt) != 0) continue;

		// Human68kŠg’£q”äŠr
		// Šg’£q .??? (.*) ‚Ìê‡‚ÍAHuman68kŠg’£q‚ÌƒsƒŠƒIƒh‚È‚µ‚É‚àƒ}ƒbƒ`‚³‚¹‚é
		if (strcmp((char*)pExt, ".???") == 0 ||	// strncmp((char*)pExt, ".*", 2) == 0 ||
			Compare(pExt, pLast, pBufExt, pBufLast) == 0) {
			// Ÿ‚ÌŒó•â‚ÌƒGƒ“ƒgƒŠ“à—e‚ğ‹L˜^
			CHostFilename* pNext = (CHostFilename*)p->Next();
			pFind->id = m_nId;
			pFind->pos = pNext;
			if (pNext != &m_cRingFilename) {
				memcpy(&pFind->entry, pNext->GetEntry(), sizeof(pFind->entry));
			} else {
				memset(&pFind->entry, 0, sizeof(pFind->entry));
			}
			return p;
		}
	}

	pFind->id = m_nId;
	pFind->pos = p;
	memset(&pFind->entry, 0, sizeof(pFind->entry));
	return NULL;
}

//---------------------------------------------------------------------------
//
//	Ä—˜—p‚Ì‚½‚ß‚Ì‰Šú‰»
//
//---------------------------------------------------------------------------
void FASTCALL CHostPath::Clean()
{
	ASSERT(this);

	if (m_hChange != INVALID_HANDLE_VALUE) {
		::FindCloseChangeNotification(m_hChange);
		m_hChange = INVALID_HANDLE_VALUE;
	}

	CleanFilename();

	FreeHuman();
	FreeWin32();
}

//---------------------------------------------------------------------------
//
//	‘Sƒtƒ@ƒCƒ‹–¼‚ğŠJ•ú
//
//---------------------------------------------------------------------------
void FASTCALL CHostPath::CleanFilename()
{
	// À‘Ì‚ğŠJ•ú
	CHostFilename* p;
	while ((p = (CHostFilename*)m_cRingFilename.Next()) != &m_cRingFilename) {
		delete p;
	}
}

//---------------------------------------------------------------------------
//
//	ƒtƒ@ƒCƒ‹•ÏX‚ªs‚È‚í‚ê‚½‚©Šm”F
//
//---------------------------------------------------------------------------
BOOL FASTCALL CHostPath::isRefresh()
{
	ASSERT(this);

	if (m_hChange == INVALID_HANDLE_VALUE) return TRUE;		// ‰Šú‰»‚³‚ê‚Ä‚¢‚È‚¯‚ê‚Î—vXV

	DWORD nResult = WaitForSingleObject(m_hChange, 0);
	if (nResult == WAIT_TIMEOUT) return FALSE;

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	ƒtƒ@ƒCƒ‹Ä\¬
//
//	‚±‚±‚Å‰‚ß‚ÄAWin32ƒtƒ@ƒCƒ‹ƒVƒXƒeƒ€‚ÌŠÏ‘ª‚ªs‚È‚í‚ê‚éB
//	•K‚¸ãˆÊ‚Å”r‘¼§Œä‚ğs‚È‚¤‚±‚ÆB
//
//---------------------------------------------------------------------------
BOOL FASTCALL CHostPath::Refresh()
{
	ASSERT(this);
	ASSERT(m_pszWin32);
	ASSERT(_tcslen(m_pszWin32) + 22 < _MAX_PATH);
#if 0
	OutputDebugString("Cache Refresh ");
	OutputDebugString(m_pszWin32);
	OutputDebugString("\r\n");
#endif

	// ƒpƒX–¼¶¬
	TCHAR szPath[_MAX_PATH];
	_tcscpy(szPath, m_pszWin32);
	_tcscat(szPath, _T("\\"));

	// XVƒ`ƒFƒbƒN
	if (m_hChange == INVALID_HANDLE_VALUE) {
		// ‰‰ñ
		DWORD nFlag =
			FILE_NOTIFY_CHANGE_FILE_NAME |
			FILE_NOTIFY_CHANGE_DIR_NAME |
			FILE_NOTIFY_CHANGE_ATTRIBUTES |
			FILE_NOTIFY_CHANGE_SIZE |
			FILE_NOTIFY_CHANGE_LAST_WRITE;
		m_hChange = ::FindFirstChangeNotification(szPath, FALSE, nFlag);
		// ŠO•”ƒAƒvƒŠ‚Ì‰e‹¿‚ÅƒGƒ‰[‚ªo‚é‰Â”\«‚ ‚èB‚±‚Ì‚Ü‚Ü‘±s‚µ‚Ä‚à–â‘è‚È‚¢
	} else {
		// “ñ‰ñ–ÚˆÈ~
		for (int i = 0; i < 3; i++) {	// 1‰ñÀs‚µ‚½‚¾‚¯‚Å‚Íƒtƒ‰ƒbƒVƒ…‚³‚ê‚È‚¢‰Â”\«‚ª‚ ‚é
			DWORD nResult = WaitForSingleObject(m_hChange, 0);
			if (nResult == WAIT_TIMEOUT) break;

			BOOL bResult = ::FindNextChangeNotification(m_hChange);
			if (bResult == FALSE) {
				::FindCloseChangeNotification(m_hChange);
				m_hChange = INVALID_HANDLE_VALUE;
				break;
			}
		}
	}

	// ˆÈ‘O‚ÌƒLƒƒƒbƒVƒ…“à—e‚ğˆÚ“®
	CRing cRingBackup;
	m_cRingFilename.InsertRing(&cRingBackup);

	// ŒŸõ—pƒtƒ@ƒCƒ‹–¼¶¬
	_tcscat(szPath, _T("*.*"));

	// ƒtƒ@ƒCƒ‹–¼“o˜^
	BOOL bUpdate = FALSE;
	HANDLE hFind = INVALID_HANDLE_VALUE;
	for (int i = 0; i < XM6_HOST_FILENAME_CACHE_MAX; i++) {
		WIN32_FIND_DATA w32Find;
		if (hFind == INVALID_HANDLE_VALUE) {
			// Å‰‚Ìƒtƒ@ƒCƒ‹–¼‚ğŠl“¾
			hFind = ::FindFirstFile(szPath, &w32Find);
			if (hFind == INVALID_HANDLE_VALUE) break;
		} else {
			// Ÿ‚Ìƒtƒ@ƒCƒ‹–¼‚ğŠl“¾
			BOOL bResult = ::FindNextFile(hFind, &w32Find);
			if (bResult == FALSE) break;
		}

		// V‹Kƒtƒ@ƒCƒ‹–¼“o˜^
		CHostFilename* pFilename = new CHostFilename;
		ASSERT(pFilename);
		pFilename->SetWin32((TCHAR*)w32Find.cFileName);

		// ˆÈ‘O‚ÌƒLƒƒƒbƒVƒ…“à—e‚ÉŠY“–‚·‚éƒtƒ@ƒCƒ‹–¼‚ª‚ ‚ê‚Î‚»‚ÌHuman68k–¼Ì‚ğ—Dæ‚·‚é
		CHostFilename* pCache = (CHostFilename*)cRingBackup.Next();
		for (;;) {
			if (pCache == &cRingBackup) {
				pCache = NULL;			// ŠY“–‚·‚éƒGƒ“ƒgƒŠ‚È‚µ(‚±‚Ì“_‚ÅV‹KƒGƒ“ƒgƒŠ‚ÆŠm’è)
				pFilename->SetHuman();
				break;
			}
			if (_tcscmp(pFilename->GetWin32(), pCache->GetWin32()) == 0) {
				pFilename->CopyHuman(pCache->GetHuman());	// Human68k–¼Ì‚ÌƒRƒs[
				break;
			}
			pCache = (CHostFilename*)pCache->Next();
		}

		// V‹KƒGƒ“ƒgƒŠ‚Ìê‡‚Íƒtƒ@ƒCƒ‹–¼d•¡‚ğƒ`ƒFƒbƒN‚·‚é
		// ƒtƒ@ƒCƒ‹–¼‚É•ÏX‚ª‚ ‚Á‚½ê‡‚ÍAˆÈ‰º‚Ìƒ`ƒFƒbƒN‚ğ‘S‚ÄƒpƒX‚µ‚½‚à‚Ì‚ğg—p‚·‚é
		// E³‚µ‚¢ƒtƒ@ƒCƒ‹–¼‚Å‚ ‚é‚±‚Æ
		// E‰ß‹‚ÌƒGƒ“ƒgƒŠ‚É“¯–¼‚Ì‚à‚Ì‚ª‘¶İ‚µ‚È‚¢‚±‚Æ
		// E“¯–¼‚ÌÀƒtƒ@ƒCƒ‹–¼‚ª‘¶İ‚µ‚È‚¢‚±‚Æ
		if (pFilename->isReduce()) {	// ƒtƒ@ƒCƒ‹–¼‚Ì•ÏX‚ªs‚È‚í‚ê‚½ê‡‚Ì‚İƒ`ƒFƒbƒN
			for (int n = 0; n < 36 * 36 * 36 * 36 * 36; n++) {	// –ñ6ç–œƒpƒ^[ƒ“(36‚Ì5æ)
				// ³‚µ‚¢ƒtƒ@ƒCƒ‹–¼‚©‚Ç‚¤‚©Šm”F
				if (pFilename->isCorrect()) {
					// ‰ß‹‚ÌƒGƒ“ƒgƒŠ‚Æˆê’v‚·‚é‚©Šm”F
					CHostFilename* pCheck = FindFilename(pFilename->GetHuman());
					if (pCheck == NULL) {
						// ˆê’v‚·‚é‚à‚Ì‚ª‚È‚¯‚ê‚ÎAÀƒtƒ@ƒCƒ‹‚ª‘¶İ‚·‚é‚©Šm”F
						TCHAR szBuf[_MAX_PATH];
						_tcscpy(szBuf, m_pszWin32);
						_tcscat(szBuf, _T("\\"));
						_tcscat(szBuf, (TCHAR*)pFilename->GetHuman());	// WARNING: Unicode—vC³
						WIN32_FIND_DATA w32Check;
						HANDLE hCheck = ::FindFirstFile(szBuf, &w32Check);
						if (hCheck == INVALID_HANDLE_VALUE) break;	// —˜—p‰Â”\ƒpƒ^[ƒ“‚ğ”­Œ©
						FindClose(hCheck);
					}
				}
				// V‚µ‚¢–¼‘O‚ğ¶¬
				pFilename->SetHuman(n);
			}
		}

		// ƒfƒBƒŒƒNƒgƒŠƒGƒ“ƒgƒŠ¶¬
		pFilename->SetEntry(&w32Find);

		// ˆÈ‘O‚ÌƒLƒƒƒbƒVƒ…“à—e‚Æ”äŠr
		if (pCache) {
			if (pCache->isSameEntry(pFilename->GetEntry())) {
				delete pFilename;		// ¡‰ñì¬‚µ‚½ƒGƒ“ƒgƒŠ‚Í”jŠü‚µ
				pFilename = pCache;		// ˆÈ‘O‚ÌƒLƒƒƒbƒVƒ…“à—e‚ğg‚¤
			} else {
				bUpdate = TRUE;			// ˆê’v‚µ‚È‚¯‚ê‚ÎXV‚ ‚è
				delete pCache;			// Ÿ‰ñ‚ÌŒŸõ‘ÎÛ‚©‚çœŠO
			}
		} else {
			bUpdate = TRUE;				// V‹KƒGƒ“ƒgƒŠ‚Ì‚½‚ßXV‚ ‚è
		}

		// ƒŠƒ“ƒO––”ö‚Ö’Ç‰Á
		pFilename->InsertTail(&m_cRingFilename);
	}

	if (hFind != INVALID_HANDLE_VALUE) FindClose(hFind);

	// c‘¶‚·‚éƒLƒƒƒbƒVƒ…“à—e‚ğíœ
	CHostFilename* p;
	while ((p = (CHostFilename*)cRingBackup.Next()) != &cRingBackup) {
		bUpdate = TRUE;			// íœ‚ğs‚È‚¤‚Ì‚ÅXV‚ ‚è
		delete p;
	}

	// XV‚ªs‚È‚í‚ê‚½‚ç¯•ÊID‚ğ•ÏX
	if (bUpdate) m_nId = g_nId++;

	// ÅŒã‚ÉƒGƒ‰[”­¶‚ğ’Ê’m
	if (m_hChange == INVALID_HANDLE_VALUE) return FALSE;

	return TRUE;
}

//===========================================================================
//
//	ƒfƒBƒŒƒNƒgƒŠƒGƒ“ƒgƒŠŠÇ—
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	‰Šú‰» (‹N“®’¼Œã)
//
//---------------------------------------------------------------------------
CHostEntry::CHostEntry()
{
#ifdef XM6_HOST_STRICT_TIMEOUT
	m_hEvent = NULL;
	m_hThread = NULL;
#else // XM6_HOST_STRICT_TIMEOUT
	m_nTimeout = 0;
#endif // XM6_HOST_STRICT_TIMEOUT
	m_nRingPath = 0;

	InitializeCriticalSection(&m_csAccess);
}

//---------------------------------------------------------------------------
//
//	ŠJ•ú (I—¹)
//
//---------------------------------------------------------------------------
CHostEntry::~CHostEntry()
{
	Clean();

	DeleteCriticalSection(&m_csAccess);
}

//---------------------------------------------------------------------------
//
//	‰Šú‰» (ƒhƒ‰ƒCƒo‘g‚İ)
//
//---------------------------------------------------------------------------
void CHostEntry::Init(CWinFileDrv** ppBase)
{
	ASSERT(this);
	ASSERT(ppBase);

	m_ppBase = ppBase;

#ifdef XM6_HOST_STRICT_TIMEOUT
	ASSERT(m_hEvent == NULL);
	ASSERT(m_hThread == NULL);

	// ƒCƒxƒ“ƒgŠm•Û
	m_hEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);		// ©“®ƒŠƒZƒbƒg
	ASSERT(m_hEvent);

	// ŠÄ‹ƒXƒŒƒbƒhŠJn
	DWORD nThread;
	m_hThread = ::CreateThread(NULL, 0, Run, this, 0, &nThread);
	ASSERT(m_hThread);
#endif // XM6_HOST_STRICT_TIMEOUT
}

//---------------------------------------------------------------------------
//
//	ŠJ•ú (‹N“®EƒŠƒZƒbƒg)
//
//---------------------------------------------------------------------------
void CHostEntry::Clean()
{
	ASSERT(this);

#ifdef XM6_HOST_STRICT_TIMEOUT
	// ŠÄ‹ƒXƒŒƒbƒhI—¹
	if (m_hThread) {
		ASSERT(m_hEvent);

		// ƒXƒŒƒbƒh‚Ö’â~—v‹
		::SetEvent(m_hEvent);

		// ƒXƒŒƒbƒhI—¹‘Ò‚¿
		DWORD nResult;
		nResult = ::WaitForSingleObject(m_hThread, 30 * 1000);	// —P—\ŠúŠÔ 30•b
		if (nResult != WAIT_OBJECT_0) {
			// ‹­§’â~
			ASSERT(FALSE);	// ”O‚Ì‚½‚ß
			::TerminateThread(m_hThread, -1);
			nResult = ::WaitForSingleObject(m_hThread, 100);
		}

		// ƒXƒŒƒbƒhƒnƒ“ƒhƒ‹ŠJ•ú
		::CloseHandle(m_hThread);
		m_hThread = NULL;
	}

	// ƒCƒxƒ“ƒgŠJ•ú
	if (m_hEvent) {
		::CloseHandle(m_hEvent);
		m_hEvent = NULL;
	}
#endif // XM6_HOST_STRICT_TIMEOUT

	LockCache();

	CleanCache();

	UnlockCache();
}

#ifdef XM6_HOST_STRICT_TIMEOUT
//---------------------------------------------------------------------------
//
//	ƒXƒŒƒbƒhÀsŠJnƒ|ƒCƒ“ƒg
//
//---------------------------------------------------------------------------
DWORD WINAPI CHostEntry::Run(VOID* pThis)
{
	ASSERT(pThis);

	((CHostEntry*)pThis)->Runner();

	::ExitThread(0);
	return 0;
}

//---------------------------------------------------------------------------
//
//	ƒXƒŒƒbƒhÀ‘Ì
//
//---------------------------------------------------------------------------
void FASTCALL CHostEntry::Runner()
{
	ASSERT(this);
	ASSERT(m_hEvent);

	for (;;) {
		DWORD nResult = ::WaitForSingleObject(m_hEvent, XM6_HOST_REMOVABLE_CHECK_TIME);
		if (nResult == WAIT_OBJECT_0) break;

		// ‘Sƒhƒ‰ƒCƒu‚Ìƒ^ƒCƒ€ƒAƒEƒgƒ`ƒFƒbƒN
		CheckTimeout();
	}
}
#endif // XM6_HOST_STRICT_TIMEOUT

//---------------------------------------------------------------------------
//
//	w’è‚³‚ê‚½ƒ†ƒjƒbƒg‚ÌƒLƒƒƒbƒVƒ…‚ğ‘S‚Äíœ‚·‚é
//
//	•K‚¸ãˆÊ‚Å”r‘¼§Œä‚ğs‚È‚¤‚±‚ÆB
//
//---------------------------------------------------------------------------
void FASTCALL CHostEntry::CleanCache()
{
	ASSERT(this);

	// À‘Ì‚ğŠJ•ú
	CHostPath* p;
	while ((p = (CHostPath*)m_cRingPath.Next()) != &m_cRingPath) {
		delete p;
		m_nRingPath--;
	}
	ASSERT(m_nRingPath == 0);

	CHostPath::InitId();
}

//---------------------------------------------------------------------------
//
//	w’è‚³‚ê‚½ƒ†ƒjƒbƒg‚ÌƒLƒƒƒbƒVƒ…‚ğ‘S‚Äíœ‚·‚é
//
//	ƒŠƒ€[ƒoƒuƒ‹ƒƒfƒBƒA‚Ì”ro‚É”º‚¤íœˆ—B
//	•K‚¸ãˆÊ‚Å”r‘¼§Œä‚ğs‚È‚¤‚±‚ÆB
//
//---------------------------------------------------------------------------
void FASTCALL CHostEntry::EraseCache(DWORD nUnit)
{
	ASSERT(this);

	// Š‚µ‚Ä‚¢‚é‘S‚Ä‚Ìƒtƒ@ƒCƒ‹–¼‚Ì’†‚©‚çˆê’v‚·‚é‚à‚Ì‚ğŒŸõ
	for (CHostPath* p = (CHostPath*)m_cRingPath.Next(); p != &m_cRingPath;) {
		if (p->isSameUnit(nUnit)) {
			delete p;
			m_nRingPath--;

			// ˜A½”½‰‚ÅŸ‚ÌƒIƒuƒWƒFƒNƒg‚ªÁ‚¦‚é‰Â”\«‚ª‚ ‚é‚Ì‚ÅÅ‰‚©‚ç’²‚×‚È‚¨‚·
			p = (CHostPath*)m_cRingPath.Next();
			continue;
		}
		p = (CHostPath*)p->Next();
	}
}

//---------------------------------------------------------------------------
//
//	ŠY“–‚·‚éƒLƒƒƒbƒVƒ…‚ğíœ‚·‚é
//
//	ƒLƒƒƒbƒVƒ…‚ÍŠù‚ÉíœÏ‚İ‚Ì‚±‚Æ‚à‚ ‚é‚½‚ßAŠm”F‚ğs‚È‚Á‚Ä‚©‚çíœ‚·‚éB
//	•K‚¸ãˆÊ‚Å”r‘¼§Œä‚ğs‚È‚¤‚±‚ÆB
//
//---------------------------------------------------------------------------
void FASTCALL CHostEntry::DeleteCache(CHostPath* pPath)
{
	ASSERT(this);
	ASSERT(pPath);

	// Š‚µ‚Ä‚¢‚é‘S‚Ä‚Ìƒtƒ@ƒCƒ‹–¼‚Ì’†‚©‚çˆê’v‚·‚é‚à‚Ì‚ğŒŸõ
	for (CHostPath* p = (CHostPath*)m_cRingPath.Next();
		 p != &m_cRingPath; p = (CHostPath*)p->Next()) {
		if (p == pPath) {
			delete p;
			m_nRingPath--;
			break;
		}
	}
}

//---------------------------------------------------------------------------
//
//	ŠY“–‚·‚éƒpƒX–¼‚ªƒLƒƒƒbƒVƒ…‚³‚ê‚Ä‚¢‚é‚©ŒŸõ‚·‚é
//
//	Š—L‚·‚éƒLƒƒƒVƒ…ƒoƒbƒtƒ@‚Ì’†‚©‚çŠ®‘Sˆê’v‚ÅŒŸõ‚µAŒ©‚Â‚©‚ê‚Î‚»‚Ì–¼Ì‚ğ•Ô‚·B
//	ƒtƒ@ƒCƒ‹–¼‚ğœŠO‚µ‚Ä‚¨‚­‚±‚ÆB
//	•K‚¸ãˆÊ‚Å”r‘¼§Œä‚ğs‚È‚¤‚±‚ÆB
//
//---------------------------------------------------------------------------
CHostPath* FASTCALL CHostEntry::FindCache(DWORD nUnit, const BYTE* szHuman)
{
	ASSERT(this);
	ASSERT(szHuman);

	// Š‚µ‚Ä‚¢‚é‘S‚Ä‚Ìƒtƒ@ƒCƒ‹–¼‚Ì’†‚©‚çŠ®‘Sˆê’v‚·‚é‚à‚Ì‚ğŒŸõ
	for (CHostPath* p = (CHostPath*)m_cRingPath.Next();
		 p != &m_cRingPath; p = (CHostPath*)p->Next()) {
		if (p->isSameHuman(nUnit, szHuman)) {
			return p;
		}
	}

	return NULL;
}

//---------------------------------------------------------------------------
//
//	ƒLƒƒƒbƒVƒ…î•ñ‚ğŒ³‚ÉAWin32–¼‚ğŠl“¾‚·‚é
//
//	ƒpƒXEƒtƒ@ƒCƒ‹–¼‚É1’iŠK‚¾‚¯•ª—£‚µAƒLƒƒƒbƒVƒ…‚É‚ ‚é‚©Šm”FB‚È‚¯‚ê‚ÎƒGƒ‰[B
//	Œ©‚Â‚©‚Á‚½ƒLƒƒƒbƒVƒ…‚ÌXVƒ`ƒFƒbƒNBXV‚ª•K—v‚È‚çƒGƒ‰[B
//	•K‚¸ãˆÊ‚Å”r‘¼§Œä‚ğs‚È‚¤‚±‚ÆB
//
//---------------------------------------------------------------------------
CHostPath* FASTCALL CHostEntry::CopyCache(DWORD nUnit, const BYTE* szHuman, TCHAR* szWin32Buffer)
{
	ASSERT(this);
	ASSERT(szHuman);
	ASSERT(strlen((char*)szHuman) < HUMAN68K_MAX_PATH);

	// ƒpƒX–¼‚Æƒtƒ@ƒCƒ‹–¼‚É•ª—£
	const BYTE* pSeparate = SeparatePath(szHuman);
	if (pSeparate == NULL) return NULL;	// ƒGƒ‰[: ƒpƒX•ª—£¸”s

	BYTE szHumanPath[HUMAN68K_MAX_PATH];
	int nPath = pSeparate - szHuman;
	strncpy((char*)szHumanPath, (char*)szHuman, nPath);
	szHumanPath[nPath] = '\0';

	BYTE szHumanFilename[_MAX_PATH];
	strcpy((char*)szHumanFilename, (char*)pSeparate + 1);

	// ƒpƒX–¼‚ÌƒLƒƒƒbƒVƒ…c‘¶ƒ`ƒFƒbƒN
	CHostPath* pPath = FindCache(nUnit, szHumanPath);
	if (pPath == NULL) return NULL;	// ƒGƒ‰[: ƒpƒX–¼‚ªƒLƒƒƒbƒVƒ…‚³‚ê‚Ä‚¢‚È‚¢‚½‚ß‰ğÍ•s”\

	// ƒŠƒ“ƒOæ“ª‚ÖˆÚ“®
	pPath->Insert(&m_cRingPath);

	// ƒLƒƒƒbƒVƒ…XVƒ`ƒFƒbƒN
	if (pPath->isRefresh()) return NULL;	// ƒGƒ‰[: ƒLƒƒƒbƒVƒ…XV‚ª•K—v

	// Win32ƒpƒX–¼‚ğƒRƒs[
	if (szWin32Buffer) {
		_tcscpy(szWin32Buffer, pPath->GetWin32());
		_tcscat(szWin32Buffer, _T("\\"));
	}

	// ƒtƒ@ƒCƒ‹–¼‚ª‚È‚¯‚ê‚ÎI—¹
	if (szHumanFilename[0] == '\0') {
		return pPath;	// •’Ê‚Í‚±‚±‚ÅI—¹
	}

	// ƒtƒ@ƒCƒ‹–¼ŒŸõ
	CHostFilename* pFilename = pPath->FindFilename(szHumanFilename);
	if (pFilename == NULL) return NULL;	// ƒGƒ‰[: “r’†‚ÌƒpƒX–¼/ƒtƒ@ƒCƒ‹–¼‚ªŒ©‚Â‚©‚ç‚È‚¢

	// Win32ƒtƒ@ƒCƒ‹–¼‚ğƒRƒs[
	if (szWin32Buffer) {
		_tcscat(szWin32Buffer, pFilename->GetWin32());
	}

	return pPath;
}

//---------------------------------------------------------------------------
//
//	Win32–¼‚Ì\’z‚É•K—v‚Èî•ñ‚ğ‚·‚×‚Äæ“¾‚·‚é
//
//	ƒtƒ@ƒCƒ‹–¼‚ÍÈ—ª‰Â”\B(•’Ê‚Íw’è‚µ‚È‚¢)
//	•K‚¸ãˆÊ‚Å”r‘¼§Œä‚ğs‚È‚¤‚±‚ÆB
//	ƒx[ƒXƒpƒX––”ö‚ÉƒpƒX‹æØ‚è•¶š‚ğ‚Â‚¯‚È‚¢‚æ‚¤’ˆÓB
//	ƒtƒ@ƒCƒ‹ƒAƒNƒZƒX‚ª‘½”­‚·‚é‰Â”\«‚ª‚ ‚é‚Æ‚«‚ÍAVMƒXƒŒƒbƒh‚Ì“®ì‚ğŠJn‚³‚¹‚éB
//
//	g‚¢‚©‚½:
//	CopyCache()‚µ‚ÄƒGƒ‰[‚Ìê‡‚ÍMakeCache()‚·‚éB•K‚¸³‚µ‚¢Win32ƒpƒX‚ªæ“¾‚Å‚«‚éB
//
//	ƒtƒ@ƒCƒ‹–¼‚ÆƒpƒX–¼‚ğ‚·‚×‚Ä•ª—£‚·‚éB
//	ãˆÊƒtƒHƒ‹ƒ_‚©‚ç‡‚ÉAƒLƒƒƒbƒVƒ…‚³‚ê‚Ä‚¢‚é‚©‚Ç‚¤‚©Šm”FB
//	ƒLƒƒƒbƒVƒ…‚³‚ê‚Ä‚¢‚ê‚Î”jŠüƒ`ƒFƒbƒNB”jŠü‚µ‚½ê‡–¢ƒLƒƒƒbƒVƒ…ˆµ‚¢‚Æ‚È‚éB
//	ƒLƒƒƒbƒVƒ…‚³‚ê‚Ä‚¢‚È‚¯‚ê‚ÎƒLƒƒƒbƒVƒ…‚ğ\’zB
//	‡”Ô‚É‚·‚×‚Ä‚ÌƒtƒHƒ‹ƒ_Eƒtƒ@ƒCƒ‹–¼‚É‘Î‚µ‚Äs‚È‚¢I—¹B
//	ƒGƒ‰[‚ª”­¶‚µ‚½ê‡‚ÍNULL‚Æ‚È‚éB
//
//---------------------------------------------------------------------------
CHostPath* FASTCALL CHostEntry::MakeCache(CWindrv* ps, DWORD nUnit, const BYTE* szHuman, TCHAR* szWin32Buffer)
{
	ASSERT(this);
	ASSERT(szHuman);
	ASSERT(strlen((char*)szHuman) < HUMAN68K_MAX_PATH);

	const TCHAR* szWin32Base = GetBase(nUnit);
	ASSERT(szWin32Base);
	ASSERT(_tcslen(szWin32Base) < _MAX_PATH);

	BYTE szHumanPath[HUMAN68K_MAX_PATH];	// ãˆÊƒtƒHƒ‹ƒ_‚©‚ç‡‚ÉƒpƒX–¼‚ª“ü‚Á‚Ä‚ä‚­
	szHumanPath[0] = '\0';
	int nHumanPath = 0;

	TCHAR szWin32Path[_MAX_PATH];
	_tcscpy(szWin32Path, szWin32Base);
	int nWin32Path = _tcslen(szWin32Path);

	CHostFilename* pFilename = NULL;	// eƒtƒHƒ‹ƒ_‚ÌÀ‘Ì‚ª“ü‚é
	CHostPath* pPath;
	const BYTE* p = szHuman;
	for (;;) {
		// ƒtƒ@ƒCƒ‹‚¢‚Á‚±‚¢‚ê‚é
		BYTE szHumanFilename[24];			// ƒtƒ@ƒCƒ‹–¼•”•ª
		p = SeparateCopyFilename(p, szHumanFilename);
		if (p == NULL) return NULL;		// ƒGƒ‰[: ƒtƒ@ƒCƒ‹–¼“Ç‚İ‚İ¸”s

		// ŠY“–ƒpƒX‚ªƒLƒƒƒbƒVƒ…‚³‚ê‚Ä‚¢‚é‚©H
		pPath = FindCache(nUnit, szHumanPath);
		if (pPath == NULL) {
			// ƒLƒƒƒbƒVƒ…Å‘å”ƒ`ƒFƒbƒN
			if (m_nRingPath >= XM6_HOST_DIRENTRY_CACHE_MAX) {
				// Šù‘¶‚ÌƒIƒuƒWƒFƒNƒg‚Ì‚¤‚¿AÅ‚àŒÃ‚¢‚à‚Ì‚ğæ“¾
				pPath = (CHostPath*)m_cRingPath.Prev();
				pPath->Clean();		// ‘Sƒtƒ@ƒCƒ‹ŠJ•ú XVƒ`ƒFƒbƒN—pƒnƒ“ƒhƒ‹‚àŠJ•ú
			} else {
				// V‹K“o˜^
				pPath = new CHostPath;
				ASSERT(pPath);
				m_nRingPath++;
			}
			pPath->SetHuman(nUnit, szHumanPath);
			pPath->SetWin32(szWin32Path);
		}

		// ƒLƒƒƒbƒVƒ…XVƒ`ƒFƒbƒN
		if (pPath->isRefresh()) {
			// ’á‘¬ƒƒfƒBƒA‚Å‚È‚­‚Ä‚àVMƒXƒŒƒbƒh‚ğÀsŠJn
			ps->Ready();
			BOOL bResult = CheckMediaAccess(nUnit, FALSE);
			if (bResult) bResult = pPath->Refresh();	// ‚ƒRƒXƒgˆ—
			if (bResult == FALSE) {
				// XV¸”s‚ÍA“¯ˆêƒ†ƒjƒbƒg‚ÌƒLƒƒƒbƒVƒ…‚ğ‘S‚ÄÁ‹‚·‚é
				delete pPath;
				m_nRingPath--;
				EraseCache(nUnit);
				return NULL;		// ƒGƒ‰[: ƒpƒX‚ªæ“¾‚Å‚«‚È‚¢
			}
		}

		// ƒŠƒ“ƒOæ“ª‚Ö
		pPath->Insert(&m_cRingPath);

		// eƒtƒHƒ‹ƒ_‚ª‚ ‚ê‚Î‹L˜^
		if (pFilename) pFilename->SetChild(this, pPath);

		// ƒtƒ@ƒCƒ‹–¼‚ª‚È‚¯‚ê‚Î‚±‚±‚ÅI—¹(ƒpƒX–¼‚¾‚¯‚¾‚Á‚½ê‡)
		// ƒtƒ@ƒCƒ‹–¼‚È‚µ‚Å‹A‚Á‚Ä‚­‚é‚Ì‚Í•¶š—ñI’[‚Ìê‡‚Ì‚İ‚È‚Ì‚ÅI—¹”»’è‚Æ‚µ‚Äg‚¦‚é
		if (szHumanFilename[0] == '\0') {
			// ƒpƒX–¼‚ğ˜AŒ‹
			if (nHumanPath + 1 + 1 > HUMAN68K_MAX_PATH) return NULL;	// ƒGƒ‰[: Human68kƒpƒX‚ª’·‚·‚¬‚é
			szHumanPath[nHumanPath++] = '/';
			szHumanPath[nHumanPath] = '\0';
			if (nWin32Path + 1 + 1 > _MAX_PATH) return NULL;	// ƒGƒ‰[: Win32ƒpƒX‚ª’·‚·‚¬‚é
			szWin32Path[nWin32Path++] = '\\';
			szWin32Path[nWin32Path] = '\0';
			break;	// •’Ê‚Í‚±‚±‚ÅI—¹
		}

		// Ÿ‚ÌƒpƒX‚ğŒŸõ
		// ƒpƒX‚Ì“r’†‚È‚çƒfƒBƒŒƒNƒgƒŠ‚©‚Ç‚¤‚©Šm”F
		if (*p != '\0') {
			pFilename = pPath->FindFilename(szHumanFilename, Human68k::AT_DIRECTORY);
		} else {
			pFilename = pPath->FindFilename(szHumanFilename);
		}
		if (pFilename == NULL) return NULL;	// ƒGƒ‰[: “r’†‚ÌƒpƒX–¼/ƒtƒ@ƒCƒ‹–¼‚ªŒ©‚Â‚©‚ç‚È‚¢

		// ƒpƒX–¼‚ğ˜AŒ‹
		int n = strlen((char*)szHumanFilename);
		if (nHumanPath + n + 1 > HUMAN68K_MAX_PATH) return NULL;	// ƒGƒ‰[: Human68kƒpƒX‚ª’·‚·‚¬‚é
		szHumanPath[nHumanPath++] = '/';
		strcpy((char*)szHumanPath + nHumanPath, (char*)szHumanFilename);
		nHumanPath += n;

		n = _tcslen(pFilename->GetWin32());
		if (nWin32Path + n + 1 > _MAX_PATH) return NULL;	// ƒGƒ‰[: Win32ƒpƒX‚ª’·‚·‚¬‚é
		szWin32Path[nWin32Path++] = '\\';
		_tcscpy(szWin32Path + nWin32Path, pFilename->GetWin32());
		nWin32Path += n;

		// PLEASE CONTINUE
		if (*p == '\0') break;
	}

	// Win32–¼‚ğƒRƒs[
	if (szWin32Buffer) {
		_tcscpy(szWin32Buffer, szWin32Path);
	}

	return pPath;
}

//---------------------------------------------------------------------------
//
//	ƒx[ƒXƒpƒX–¼‚ğæ“¾‚·‚é
//
//---------------------------------------------------------------------------
TCHAR* FASTCALL CHostEntry::GetBase(DWORD nUnit) const
{
	ASSERT(this);
	ASSERT(m_ppBase);
	ASSERT(nUnit < CWinFileSys::DrvMax);
	ASSERT(m_ppBase[nUnit]);

	return m_ppBase[nUnit]->GetBase();
}

//---------------------------------------------------------------------------
//
//	‘‚«‚İ‹Ö~‚©‚Ç‚¤‚©Šm”F‚·‚é
//
//---------------------------------------------------------------------------
BOOL FASTCALL CHostEntry::isWriteProtect(CWindrv* ps) const
{
	ASSERT(this);
	ASSERT(ps);
	ASSERT(m_ppBase);

	DWORD nUnit = ps->GetUnit();
	ASSERT(nUnit < CWinFileSys::DrvMax);
	ASSERT(m_ppBase[nUnit]);

	return m_ppBase[nUnit]->isWriteProtect();
}

//---------------------------------------------------------------------------
//
//	’á‘¬ƒƒfƒBƒAƒ`ƒFƒbƒN‚ÆƒIƒtƒ‰ƒCƒ“ó‘Ôƒ`ƒFƒbƒN
//
//---------------------------------------------------------------------------
BOOL FASTCALL CHostEntry::isMediaOffline(CWindrv* ps, BOOL bMedia)
{
	ASSERT(this);
	ASSERT(ps);
	ASSERT(m_ppBase);

	DWORD nUnit = ps->GetUnit();
	ASSERT(nUnit < CWinFileSys::DrvMax);
	ASSERT(m_ppBase[nUnit]);

	// ’á‘¬ƒfƒoƒCƒX‚È‚çVMƒXƒŒƒbƒh‚Ì“®ì‚ğŠJn‚·‚é
	if (m_ppBase[nUnit]->isSlow()) ps->Ready();

	// è“®ƒCƒWƒFƒNƒgƒfƒoƒCƒX‚ÌƒAƒNƒZƒX–‘Oƒ`ƒFƒbƒN
	if (bMedia) CheckMediaAccess(nUnit, TRUE);

	// ƒŠƒ€[ƒoƒuƒ‹ƒƒfƒBƒA‚ÌƒLƒƒƒbƒVƒ…—LŒøŠÔXV
	m_ppBase[nUnit]->SetTimeout();

	// è“®ƒCƒWƒFƒNƒgƒfƒoƒCƒX‚ÍuƒfƒBƒXƒN‚ª“ü‚Á‚Ä‚¢‚Ü‚¹‚ñv‚É‚µ‚È‚¢
	if (m_ppBase[nUnit]->isManual()) return FALSE;

	// ƒIƒtƒ‰ƒCƒ“ó‘Ôƒ`ƒFƒbƒN
	return m_ppBase[nUnit]->isEnable() == FALSE;
}

//---------------------------------------------------------------------------
//
//	ƒƒfƒBƒAƒoƒCƒg‚Ìæ“¾
//
//---------------------------------------------------------------------------
BYTE FASTCALL CHostEntry::GetMediaByte(DWORD nUnit) const
{
	ASSERT(this);
	ASSERT(m_ppBase);
	ASSERT(nUnit < CWinFileSys::DrvMax);
	ASSERT(m_ppBase[nUnit]);

	if (m_ppBase[nUnit]->isRemovable()) {
		if (m_ppBase[nUnit]->isManual()) {
			return 0xF1;
		}
		return 0xF2;
	}
	return 0xF3;
}

//---------------------------------------------------------------------------
//
//	ƒhƒ‰ƒCƒuó‘Ô‚Ìæ“¾
//
//---------------------------------------------------------------------------
DWORD FASTCALL CHostEntry::GetStatus(DWORD nUnit) const
{
	ASSERT(this);
	ASSERT(m_ppBase);
	ASSERT(nUnit < CWinFileSys::DrvMax);
	ASSERT(m_ppBase[nUnit]);

	return m_ppBase[nUnit]->GetStatus();
}

//---------------------------------------------------------------------------
//
//	‘Sƒhƒ‰ƒCƒu‚Ìƒ^ƒCƒ€ƒAƒEƒgƒ`ƒFƒbƒN
//
//---------------------------------------------------------------------------
void FASTCALL CHostEntry::CheckTimeout()
{
	ASSERT(this);
	ASSERT(m_ppBase);

#ifndef XM6_HOST_STRICT_TIMEOUT
	// ƒ^ƒCƒ€ƒAƒEƒg‚ğŠm”F
	DWORD nCount = ::GetTickCount();
	DWORD nDelta = nCount - m_nTimeout;
	if (nDelta < XM6_HOST_REMOVABLE_CHECK_TIME) return;
	m_nTimeout = nCount;
#endif // XM6_HOST_STRICT_TIMEOUT

	for (DWORD n = 0; n < CWinFileSys::DrvMax; n++) {
		if (m_ppBase[n]) {
			BOOL bResult = m_ppBase[n]->CheckTimeout();
			if (bResult) {
				LockCache();
				EraseCache(n);
				UnlockCache();
			}
		}
	}
}

#ifdef XM6_HOST_UPDATE_BY_SEQUENCE
//---------------------------------------------------------------------------
//
//	ƒŠƒ€[ƒoƒuƒ‹ƒƒfƒBƒA‚Ìó‘ÔXV‚ğ—LŒø‚É‚·‚é
//
//---------------------------------------------------------------------------
void FASTCALL CHostEntry::SetMediaUpdate(CWindrv* ps, BOOL bDisable)
{
	ASSERT(this);
	ASSERT(ps);
	ASSERT(m_ppBase);

	DWORD nUnit = ps->GetUnit();
	ASSERT(nUnit < CWinFileSys::DrvMax);
	ASSERT(m_ppBase[nUnit]);

	m_ppBase[nUnit]->SetMediaUpdate(bDisable);
}
#endif // XM6_HOST_UPDATE_BY_SEQUENCE

//---------------------------------------------------------------------------
//
//	ƒŠƒ€[ƒoƒuƒ‹ƒƒfƒBƒA‚Ìó‘ÔXVƒ`ƒFƒbƒN
//
//---------------------------------------------------------------------------
BOOL FASTCALL CHostEntry::CheckMediaUpdate(CWindrv* ps)
{
	ASSERT(this);
	ASSERT(ps);
	ASSERT(m_ppBase);

	DWORD nUnit = ps->GetUnit();
	ASSERT(nUnit < CWinFileSys::DrvMax);
	ASSERT(m_ppBase[nUnit]);

	// XV”»’è ƒŒƒxƒ‹1
	BOOL bResult = m_ppBase[nUnit]->CheckMediaUpdate();
	if (bResult) {
		// XV”»’è ƒŒƒxƒ‹2
		bResult = m_ppBase[nUnit]->CheckMediaAccess(FALSE);
		if (bResult) {
			// ’á‘¬ƒƒfƒBƒAƒ`ƒFƒbƒN
			if (m_ppBase[nUnit]->isSlow()) ps->Ready();

			// ƒƒfƒBƒA—LŒøƒ`ƒFƒbƒN
			bResult = m_ppBase[nUnit]->CheckMedia();
			if (bResult == FALSE) {
				// ƒƒfƒBƒAg—p•s”\‚ÍƒLƒƒƒbƒVƒ…ƒNƒŠƒA
				LockCache();
				EraseCache(nUnit);
				UnlockCache();
			}
		}
	}

	return m_ppBase[nUnit]->isEnable();
}

//---------------------------------------------------------------------------
//
//	ƒŠƒ€[ƒoƒuƒ‹ƒƒfƒBƒA‚ÌƒAƒNƒZƒX–‘Oƒ`ƒFƒbƒN
//
//	ŒÄ‚Ño‚µŒ³‚É‚æ‚èA‚»‚ê‚¼‚êˆÈ‰º‚Ì‚æ‚¤‚Èˆ—‚ğs‚È‚¤
//	1: MakeCache()			’á‘¬”»’è•s—v/ƒNƒŠƒA•s—v/è“®ƒCƒWƒFƒNƒgƒƒfƒBƒA‚Ì‚İ
//	2: isMediaOffline()		’á‘¬”»’è•s—v/ƒNƒŠƒA•K—v/è“®ƒCƒWƒFƒNƒgƒƒfƒBƒA‚Ì‚İ
//	3: CheckMediaUpdate()‚É“‡Ï	’á‘¬”»’è•K—v/ƒNƒŠƒA•K—v/ƒŠƒ€[ƒoƒuƒ‹ƒƒfƒBƒA‘S‚Ä
//
//---------------------------------------------------------------------------
BOOL FASTCALL CHostEntry::CheckMediaAccess(DWORD nUnit, BOOL bErase)
{
	ASSERT(this);
	ASSERT(m_ppBase);
	ASSERT(nUnit < CWinFileSys::DrvMax);
	ASSERT(m_ppBase[nUnit]);

	// XV”»’è
	BOOL bResult = m_ppBase[nUnit]->CheckMediaAccess(TRUE);
	if (bResult) {
		// ƒƒfƒBƒA—LŒøƒ`ƒFƒbƒN
		bResult = m_ppBase[nUnit]->CheckMedia();
		if (bResult == FALSE) {
			// ƒƒfƒBƒAg—p•s”\‚ÍƒLƒƒƒbƒVƒ…ƒNƒŠƒA
			if (bErase) {
				LockCache();
				EraseCache(nUnit);
				UnlockCache();
			}
		}
	}

	return m_ppBase[nUnit]->isEnable();
}

//---------------------------------------------------------------------------
//
//	ƒCƒWƒFƒNƒg
//
//---------------------------------------------------------------------------
BOOL FASTCALL CHostEntry::Eject(DWORD nUnit)
{
	ASSERT(this);
	ASSERT(m_ppBase);
	ASSERT(nUnit < CWinFileSys::DrvMax);
	ASSERT(m_ppBase[nUnit]);

	BOOL bResult = m_ppBase[nUnit]->Eject();
	if (bResult) {
		m_ppBase[nUnit]->CheckMedia();
	}

	return bResult;
}

//---------------------------------------------------------------------------
//
//	ƒ{ƒŠƒ…[ƒ€ƒ‰ƒxƒ‹‚Ìæ“¾
//
//---------------------------------------------------------------------------
void FASTCALL CHostEntry::GetVolume(DWORD nUnit, TCHAR* szLabel)
{
	ASSERT(this);
	ASSERT(m_ppBase);
	ASSERT(nUnit < CWinFileSys::DrvMax);
	ASSERT(m_ppBase[nUnit]);

	m_ppBase[nUnit]->GetVolume(szLabel);
}

//---------------------------------------------------------------------------
//
//	ƒLƒƒƒbƒVƒ…‚©‚çƒ{ƒŠƒ…[ƒ€ƒ‰ƒxƒ‹‚ğæ“¾
//
//---------------------------------------------------------------------------
BOOL FASTCALL CHostEntry::GetVolumeCache(DWORD nUnit, TCHAR* szLabel)
{
	ASSERT(this);
	ASSERT(m_ppBase);
	ASSERT(nUnit < CWinFileSys::DrvMax);
	ASSERT(m_ppBase[nUnit]);

	return m_ppBase[nUnit]->GetVolumeCache(szLabel);
}

//---------------------------------------------------------------------------
//
//	—e—Ê‚Ìæ“¾
//
//---------------------------------------------------------------------------
DWORD FASTCALL CHostEntry::GetCapacity(DWORD nUnit, Human68k::capacity_t* pCapacity)
{
	ASSERT(this);
	ASSERT(m_ppBase);
	ASSERT(nUnit < CWinFileSys::DrvMax);
	ASSERT(m_ppBase[nUnit]);

	return m_ppBase[nUnit]->GetCapacity(pCapacity);
}

//---------------------------------------------------------------------------
//
//	ƒLƒƒƒbƒVƒ…‚©‚çƒNƒ‰ƒXƒ^ƒTƒCƒY‚ğæ“¾
//
//---------------------------------------------------------------------------
BOOL FASTCALL CHostEntry::GetCapacityCache(DWORD nUnit, Human68k::capacity_t* pCapacity)
{
	ASSERT(this);
	ASSERT(m_ppBase);
	ASSERT(nUnit < CWinFileSys::DrvMax);
	ASSERT(m_ppBase[nUnit]);

	return m_ppBase[nUnit]->GetCapacityCache(pCapacity);
}

//---------------------------------------------------------------------------
//
//	ƒtƒ@ƒCƒ‹ƒVƒXƒeƒ€ó‘Ô’Ê’m
//
//---------------------------------------------------------------------------
void FASTCALL CHostEntry::ShellNotify(DWORD nEvent, const TCHAR* szPath)
{
	ASSERT(this);
	ASSERT(m_ppBase);

	// ŠY“–‚·‚éƒ†ƒjƒbƒg‚ğŒŸõ (d•¡w’è‚àl—¶‚·‚é)
	for (int n = 0; n < CWinFileSys::DrvMax; n++) {
		if (m_ppBase[n]) {
			if (m_ppBase[n]->isSameDrive(szPath)) {
				// ”ro
				if (nEvent & (SHCNE_MEDIAREMOVED | SHCNE_DRIVEREMOVED)) {
					m_ppBase[n]->SetEnable(FALSE);
				}

				// ‘}“ü
				if (nEvent & (SHCNE_MEDIAINSERTED | SHCNE_DRIVEADD)) {
					m_ppBase[n]->SetEnable(TRUE);
				}

				// ŠY“–‚·‚éƒLƒƒƒbƒVƒ…‚ğÁ‹
				LockCache();
				EraseCache(n);
				UnlockCache();
			}
		}
	}
}

//---------------------------------------------------------------------------
//
//	Human68kƒtƒ‹ƒpƒX–¼‚©‚çÅŒã‚Ì—v‘f‚ğ•ª—£
//
//---------------------------------------------------------------------------
const BYTE* FASTCALL CHostEntry::SeparatePath(const BYTE* szHuman)
{
	ASSERT(szHuman);

	DWORD nMax = 22;
	const BYTE* p = szHuman;

	BYTE c = *p;
	if (c != '/' && c != '\\') return NULL;	// ƒGƒ‰[: •s³‚ÈƒpƒX–¼

	// •¶š—ñ’†‚ÌÅŒã‚Ì / ‚à‚µ‚­‚Í \ ‚ğŒŸõ‚·‚éB
	const BYTE* pCut = NULL;
	for (;;) {
		c = *p;
		if (c == '\0') break;
		if ((0x80 <= c && c <= 0x9F) || 0xE0 <= c) {	// Œµ–§‚É‚Í 0x81`0x9F 0xE0`0xEF
			p++;
			if (*p == '\0') break;
		}
		if (c == '/' || c == '\\') {
			if (pCut == p - 1) return NULL;	// 2•¶š˜A‘±‚µ‚Ä‹æØ‚è•¶š‚ª‚ ‚Á‚½‚çƒGƒ‰[
			pCut = p;
		}
		p++;
	}

	// •ª—£‰Â”\‚©‚Ç‚¤‚©Šm”F
	if (pCut == NULL) return NULL;			// ‚»‚à‚»‚à•¶š‚ªŒ©‚Â‚©‚ç‚È‚¢ê‡‚ÍƒGƒ‰[
	//if (pCut == szHuman) return NULL;		// æ“ª(ƒ‹[ƒg)‚¾‚Á‚½ê‡ “Á•Êˆµ‚¢‚µ‚È‚¢
	//if (pCut[1] == '\0') return NULL;		// ƒtƒ@ƒCƒ‹–¼•”•ª‚ª‚È‚¢ê‡ “Á•Êˆµ‚¢‚µ‚È‚¢
	if (strlen((char*)pCut + 1) > nMax) return NULL;	// ƒtƒ@ƒCƒ‹–¼•”•ª‚ª’·‚·‚¬‚éê‡‚ÍƒGƒ‰[

	return pCut;
}

//---------------------------------------------------------------------------
//
//	Human68kƒtƒ‹ƒpƒX–¼‚©‚çæ“ª‚Ì—v‘f‚ğ•ª—£EƒRƒs[
//
//	Human68kƒpƒX‚Í•K‚¸ / ‚ÅŠJn‚·‚é‚±‚ÆB
//	“r’† / ‚ª2‚ÂˆÈã˜A‘±‚µ‚ÄoŒ»‚µ‚½ê‡‚ÍƒGƒ‰[‚Æ‚·‚éB
//	•¶š—ñI’[‚ª / ‚¾‚¯‚Ìê‡A‹ó‚Ì•¶š—ñ‚Æ‚µ‚Äˆ—‚·‚éBƒGƒ‰[‚É‚Í‚µ‚È‚¢B
//
//---------------------------------------------------------------------------
const BYTE* FASTCALL CHostEntry::SeparateCopyFilename(const BYTE* szHuman, BYTE* szBuffer)
{
	ASSERT(szHuman);
	ASSERT(szBuffer);

	DWORD nMax = 22;
	const BYTE* p = szHuman;

	BYTE c = *p;
	if (c != '/' && c != '\\') return NULL;	// ƒGƒ‰[: •s³‚ÈƒpƒX–¼
	p++;

	// ƒtƒ@ƒCƒ‹‚¢‚Á‚±‚¢‚ê‚é
	BYTE* pWrite = szBuffer;
	DWORD i = 0;
	for (;;) {
		c = *p;								// ˆê•¶š“Ç‚İ‚İ i‚ß‚é
		if (c == '\0') break;				// •¶š—ñI’[‚È‚çI—¹
		if (c == '/' || c == '\\') {
			if (pWrite == szBuffer) return NULL;	// ƒGƒ‰[: ƒpƒX‹æØ‚è•¶š‚ª˜A‘±‚µ‚Ä‚¢‚é
			break;	// ƒpƒX‚Ì‹æØ‚è‚ğ“Ç‚ñ‚¾‚çI—¹
		}
		p++;

		if (i >= nMax) return NULL;	// ƒGƒ‰[: 1ƒoƒCƒg–Ú‚ªƒoƒbƒtƒ@I’[‚É‚©‚©‚é
		i++;								// ‘‚«‚Ş‘O‚ÉI’[ƒ`ƒFƒbƒN
		*pWrite++ = c;						// ˆê•¶š‘‚«‚İ

		if ((0x80 <= c && c <= 0x9F) || 0xE0 <= c) {	// Œµ–§‚É‚Í 0x81`0x9F 0xE0`0xEF
			c = *p;							// ˆê•¶š“Ç‚İ‚İ i‚ß‚é
			if (c < 0x40) return NULL;		// ƒGƒ‰[: •s³‚ÈSJIS2ƒoƒCƒg–Ú
			p++;

			if (i >= nMax) return NULL;	// ƒGƒ‰[: 2ƒoƒCƒg–Ú‚ªƒoƒbƒtƒ@I’[‚É‚©‚©‚é
			i++;							// ‘‚«‚Ş‘O‚ÉI’[ƒ`ƒFƒbƒN
			*pWrite++ = c;					// ˆê•¶š‘‚«‚İ
		}
	}
	*pWrite = '\0';

	return p;
}

//===========================================================================
//
//	ƒtƒ@ƒCƒ‹ŒŸõˆ—
//
//	Human68k‘¤‚Ìƒtƒ@ƒCƒ‹–¼‚ğ“à•”Unicode‚Åˆ—‚·‚é‚Ì‚Í³’¼ƒLƒc‚¢B‚Æ
//	‚¢‚¤‚í‚¯‚ÅA‘S‚ÄBYTE‚É•ÏŠ·‚µ‚Äˆ—‚·‚éB•ÏŠ·ˆ—‚ÍƒfƒBƒŒƒNƒgƒŠƒG
//	ƒ“ƒgƒŠƒLƒƒƒbƒVƒ…‚ªˆêè‚É’S‚¢AWINDRV‘¤‚Í‚·‚×‚ÄƒVƒtƒgJIS‚Ì‚İ‚Åˆµ
//	‚¦‚é‚æ‚¤‚É‚·‚éB
//	‚Ü‚½AHuman68k‘¤–¼Ì‚ÍAŠ®‘S‚Éƒx[ƒXƒpƒXw’è‚©‚ç“Æ—§‚³‚¹‚éB
//
//	ƒtƒ@ƒCƒ‹‚ğˆµ‚¤’¼‘O‚ÉAƒfƒBƒŒƒNƒgƒŠƒGƒ“ƒgƒŠ‚ÌƒLƒƒƒbƒVƒ…‚ğ¶¬‚·‚éB
//	ƒfƒBƒŒƒNƒgƒŠƒGƒ“ƒgƒŠ‚Ì¶¬ˆ—‚Í‚ƒRƒXƒg‚Ì‚½‚ßAƒLƒƒƒbƒVƒ…\’z‚Ì
//	’¼‘O‚ÅVMƒXƒŒƒbƒh‚Ì“®ì‚ğŠJn‚³‚¹‚éB
//
//	ƒtƒ@ƒCƒ‹ŒŸõ‚Í3•û®B‚·‚×‚ÄCHostFiles::Find()‚Åˆ—‚·‚éB
//	1. ƒpƒX–¼‚Ì‚İŒŸõ ‘®«‚ÍƒfƒBƒŒƒNƒgƒŠ‚Ì‚İ	_CHKDIR _CREATE
//	2. ƒpƒX–¼+ƒtƒ@ƒCƒ‹–¼+‘®«‚ÌŒŸõ _OPEN
//	3. ƒpƒX–¼+ƒƒCƒ‹ƒhƒJ[ƒh+‘®«‚ÌŒŸõ _FILES _NFILES
//	ŒŸõŒ‹‰Ê‚ÍAƒfƒBƒŒƒNƒgƒŠƒGƒ“ƒgƒŠî•ñ‚Æ‚µ‚Ä•Û‚µ‚Ä‚¨‚­B
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	ƒNƒŠƒA
//
//---------------------------------------------------------------------------
void FASTCALL CHostFiles::Clear()
{
	ASSERT(this);

	m_nKey = 0;
#if 0
	// ŒŸõğŒ: SetPath()‚Åİ’è‚³‚ê‚é‚½‚ß•s—v
	m_nHumanUnit = 0;
	m_szHumanPath[0] = '\0';
	m_szHumanFilename[0] = '\0';
	m_nHumanWildcard = 0;
	m_nHumanAttribute = 0;
	m_findNext.Clear();
#endif
	memset(&m_dirEntry, 0, sizeof(m_dirEntry));
	m_szHumanResult[0] = '\0';
	m_szWin32Result[0] = _T('\0');
}

//---------------------------------------------------------------------------
//
//	ƒpƒX–¼Eƒtƒ@ƒCƒ‹–¼‚ğ“à•”‚Å¶¬
//
//---------------------------------------------------------------------------
void FASTCALL CHostFiles::SetPath(DWORD nUnit, const Human68k::namests_t* pNamests)
{
	ASSERT(this);
	ASSERT(pNamests);

	m_nHumanUnit = nUnit;
	pNamests->GetCopyPath(m_szHumanPath);
	pNamests->GetCopyFilename(m_szHumanFilename);
	m_nHumanWildcard = 0;
	m_nHumanAttribute = Human68k::AT_ARCHIVE;
	m_findNext.Clear();
}

//---------------------------------------------------------------------------
//
//	Win32–¼‚ğŒŸõ (ƒpƒX–¼ + ƒtƒ@ƒCƒ‹–¼(È—ª‰Â) + ‘®«)
//
//	‚ ‚ç‚©‚¶‚ß‘S‚Ä‚ÌHuman68k—pƒpƒ‰ƒ[ƒ^‚ğİ’è‚µ‚Ä‚¨‚­‚±‚ÆB
//
//---------------------------------------------------------------------------
BOOL FASTCALL CHostFiles::Find(CWindrv* ps, CHostEntry* pEntry, BOOL bRemove)
{
	ASSERT(this);
	ASSERT(pEntry);

	// ”r‘¼§Œä ŠJn
	pEntry->LockCache();

	// ƒpƒX–¼Šl“¾‚¨‚æ‚ÑƒLƒƒƒbƒVƒ…\’z
	CHostPath* pPath;
#ifdef XM6_HOST_STRICT_CACHE_CHECK
	pPath = pEntry->MakeCache(ps, m_nHumanUnit, m_szHumanPath, m_szWin32Result);
#else // XM6_HOST_STRICT_CACHE_CHECK
	pPath = pEntry->CopyCache(m_nHumanUnit, m_szHumanPath, m_szWin32Result);
	if (pPath == NULL) {
		pPath = pEntry->MakeCache(ps, m_nHumanUnit, m_szHumanPath, m_szWin32Result);
	}
#endif // XM6_HOST_STRICT_CACHE_CHECK
	if (pPath == NULL) {
		pEntry->UnlockCache();
		return FALSE;	// ƒGƒ‰[: ƒLƒƒƒbƒVƒ…\’z¸”s
	}

	// ƒtƒ@ƒCƒ‹–¼‚ª‚È‚¯‚ê‚ÎI—¹
	if (m_nHumanWildcard == 0xFF) {
		_tcscpy(m_szWin32Result, pPath->GetWin32());
		_tcscat(m_szWin32Result, _T("\\"));
		pEntry->UnlockCache();
		return TRUE;	// ³íI—¹: ƒpƒX–¼‚Ì‚İ
	}

	// ƒtƒ@ƒCƒ‹–¼Šl“¾
	CHostFilename* pFilename;
	if (m_nHumanWildcard == 0) {
		pFilename = pPath->FindFilename(m_szHumanFilename, m_nHumanAttribute);
	} else {
		pFilename = pPath->FindFilenameWildcard(m_szHumanFilename, &m_findNext, m_nHumanAttribute);
	}
	if (pFilename == NULL) {
		pEntry->UnlockCache();
		return FALSE;	// ƒGƒ‰[: ƒtƒ@ƒCƒ‹–¼‚ªŠl“¾‚Å‚«‚Ü‚¹‚ñ
	}

	// ƒfƒBƒŒƒNƒgƒŠíœ‚Ìê‡‚Í‚±‚±‚Åƒnƒ“ƒhƒ‹‚ğŠJ•ú‚³‚¹‚é
	if (bRemove) pFilename->DeleteCache();

	// Human68kƒtƒ@ƒCƒ‹î•ñ•Û‘¶
	memcpy(&m_dirEntry, pFilename->GetEntry(), sizeof(m_dirEntry));

	// Human68kƒtƒ@ƒCƒ‹–¼•Û‘¶
	strcpy((char*)m_szHumanResult, (char*)pFilename->GetHuman());

	// Win32ƒtƒ‹ƒpƒX–¼•Û‘¶
	_tcscpy(m_szWin32Result, pPath->GetWin32());
	_tcscat(m_szWin32Result, _T("\\"));
	_tcscat(m_szWin32Result, pFilename->GetWin32());

	// ”r‘¼§Œä I—¹
	pEntry->UnlockCache();

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	Win32–¼‚Éƒtƒ@ƒCƒ‹–¼‚ğ’Ç‰Á
//
//---------------------------------------------------------------------------
void FASTCALL CHostFiles::AddFilename()
{
	ASSERT(this);
	ASSERT(_tcslen(m_szWin32Result) + strlen((char*)m_szHumanFilename) < _MAX_PATH);

	// WARNING: Unicode–¢‘Î‰B‚¢‚¸‚êUnicode‚Ì¢ŠE‚ÉŸZ‚Ü‚ê‚½‚Í‚±‚±‚Å•ÏŠ·‚ğs‚È‚¤
	_tcscat(m_szWin32Result, (TCHAR*)m_szHumanFilename);
}

//===========================================================================
//
//	ƒtƒ@ƒCƒ‹ŒŸõ—Ìˆæ ƒ}ƒl[ƒWƒƒ
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	ƒtƒ@ƒCƒ‹ŒŸõ—Ìˆæ ‰Šú‰» (‹N“®’¼Œã)
//
//---------------------------------------------------------------------------
CHostFilesManager::CHostFilesManager()
{
}

//---------------------------------------------------------------------------
//
//	ƒtƒ@ƒCƒ‹ŒŸõ—Ìˆæ Šm”F (I—¹)
//
//---------------------------------------------------------------------------
CHostFilesManager::~CHostFilesManager()
{
#ifdef _DEBUG
	// À‘Ì‚ª‘¶İ‚µ‚È‚¢‚±‚Æ‚ğŠm”F
	ASSERT(m_cRingFiles.Next() == &m_cRingFiles);
	ASSERT(m_cRingFiles.Prev() == &m_cRingFiles);

	// ”O‚Ì‚½‚ß(ÀÛ‚É‚Íg‚í‚ê‚È‚¢)
	Clean();
#endif // _DEBUG
}

//---------------------------------------------------------------------------
//
//	ƒtƒ@ƒCƒ‹ŒŸõ—Ìˆæ ‰Šú‰» (ƒhƒ‰ƒCƒo‘g‚İ)
//
//---------------------------------------------------------------------------
void FASTCALL CHostFilesManager::Init()
{
	ASSERT(this);

	// À‘Ì‚ª‘¶İ‚µ‚È‚¢‚±‚Æ‚ğŠm”F (”O‚Ì‚½‚ß)
	ASSERT(m_cRingFiles.Next() == &m_cRingFiles);
	ASSERT(m_cRingFiles.Prev() == &m_cRingFiles);

	// À‘Ì‚ğŠm•Û
	for (int i = 0; i < XM6_HOST_FILES_MAX; i++) {
		CHostFiles* p = new CHostFiles;
		ASSERT(p);
		p->InsertTail(&m_cRingFiles);
	}
}

//---------------------------------------------------------------------------
//
//	ƒtƒ@ƒCƒ‹ŒŸõ—Ìˆæ ŠJ•ú (‹N“®EƒŠƒZƒbƒg)
//
//---------------------------------------------------------------------------
void FASTCALL CHostFilesManager::Clean()
{
	ASSERT(this);

	// À‘Ì‚ğŠJ•ú
	CHostFiles* p;
	while ((p = (CHostFiles*)m_cRingFiles.Next()) != &m_cRingFiles) {
		delete p;
	}
}

//---------------------------------------------------------------------------
//
//	ƒtƒ@ƒCƒ‹ŒŸõ—Ìˆæ Šm•Û
//
//---------------------------------------------------------------------------
CHostFiles* FASTCALL CHostFilesManager::Alloc(DWORD nKey)
{
	ASSERT(this);
	ASSERT(nKey != 0);

	// ––”ö‚Ì1‚Â‚ğ‘I‘ğ
	CHostFiles* p = (CHostFiles*)m_cRingFiles.Prev();

	// ŠY“–ƒIƒuƒWƒFƒNƒg‚ğƒŠƒ“ƒOæ“ª‚ÖˆÚ“®
	p->Insert(&m_cRingFiles);

	// ƒL[‚ğİ’è
	p->SetKey(nKey);

	return p;
}

//---------------------------------------------------------------------------
//
//	ƒtƒ@ƒCƒ‹ŒŸõ—Ìˆæ ŒŸõ
//
//---------------------------------------------------------------------------
CHostFiles* FASTCALL CHostFilesManager::Search(DWORD nKey)
{
	ASSERT(this);
	ASSERT(nKey != 0);

	// ŠY“–‚·‚éƒIƒuƒWƒFƒNƒg‚ğŒŸõ
	for (CHostFiles* p = (CHostFiles*)m_cRingFiles.Next();
		 p != &m_cRingFiles; p = (CHostFiles*)p->Next()) {
		if (p->isSameKey(nKey)) {
			// ŠY“–ƒIƒuƒWƒFƒNƒg‚ğƒŠƒ“ƒOæ“ª‚ÖˆÚ“®
			p->Insert(&m_cRingFiles);
			return p;
		}
	}

	return NULL;
}

//---------------------------------------------------------------------------
//
//	ƒtƒ@ƒCƒ‹ŒŸõ—Ìˆæ ŠJ•ú
//
//---------------------------------------------------------------------------
void FASTCALL CHostFilesManager::Free(CHostFiles* p)
{
	ASSERT(this);
	ASSERT(p);

	// À‘Ì‚Ì‰Šú‰» (Ä—˜—p‚·‚é‚½‚ß—ÌˆæŠJ•ú‚Ís‚È‚í‚È‚¢)
	p->Free();

	// ŠY“–ƒIƒuƒWƒFƒNƒg‚ğƒŠƒ“ƒO––”ö‚ÖˆÚ“®
	p->InsertTail(&m_cRingFiles);
}

//===========================================================================
//
//	FCBˆ—
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	ƒNƒŠƒA
//
//---------------------------------------------------------------------------
void FASTCALL CHostFcb::Clear()
{
	ASSERT(this);

	m_nKey = 0;
	m_nMode = 0;
	m_hFile = INVALID_HANDLE_VALUE;
	m_szFilename[0] = _T('\0');
}

//---------------------------------------------------------------------------
//
//	ƒtƒ@ƒCƒ‹ƒI[ƒvƒ“ƒ‚[ƒh‚Ìİ’è
//
//---------------------------------------------------------------------------
BOOL FASTCALL CHostFcb::SetOpenMode(DWORD nHumanMode)
{
	ASSERT(this);

	switch (nHumanMode & 0x0F) {
	case Human68k::OP_READ:
		m_nMode = GENERIC_READ;
		break;
	case Human68k::OP_WRITE:
		m_nMode = GENERIC_WRITE;
		break;
	case Human68k::OP_READWRITE:
		m_nMode = GENERIC_READ | GENERIC_WRITE;
		break;
	default:
		return FALSE;
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	ƒtƒ@ƒCƒ‹–¼‚Ìİ’è
//
//---------------------------------------------------------------------------
void FASTCALL CHostFcb::SetFilename(const TCHAR* szFilename)
{
	ASSERT(this);
	ASSERT(szFilename);
	ASSERT(_tcslen(szFilename) < _MAX_PATH);

	_tcscpy(m_szFilename, szFilename);
}

//---------------------------------------------------------------------------
//
//	ƒtƒ@ƒCƒ‹ì¬
//
//---------------------------------------------------------------------------
HANDLE FASTCALL CHostFcb::Create(DWORD nHumanAttribute, BOOL bForce)
{
	ASSERT(this);
	ASSERT(_tcslen(m_szFilename) > 0);
	ASSERT(m_hFile == INVALID_HANDLE_VALUE);

	// ‘®«¶¬
	DWORD nAttribute = 0;
	if ((nHumanAttribute & Human68k::AT_DIRECTORY) != 0) return INVALID_HANDLE_VALUE;
	if ((nHumanAttribute & Human68k::AT_SYSTEM) != 0) nAttribute |= FILE_ATTRIBUTE_SYSTEM;
	if ((nHumanAttribute & Human68k::AT_HIDDEN) != 0) nAttribute |= FILE_ATTRIBUTE_HIDDEN;
	if ((nHumanAttribute & Human68k::AT_READONLY) != 0) nAttribute |= FILE_ATTRIBUTE_READONLY;
	if (nAttribute == 0) nAttribute = FILE_ATTRIBUTE_NORMAL;

	DWORD nShare = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;
	DWORD nCreation = bForce ? CREATE_ALWAYS : CREATE_NEW;

	// ƒtƒ@ƒCƒ‹ƒI[ƒvƒ“
	m_hFile = ::CreateFile(m_szFilename, m_nMode, nShare, NULL, nCreation, nAttribute, NULL);
	return m_hFile;
}

//---------------------------------------------------------------------------
//
//	ƒtƒ@ƒCƒ‹ƒI[ƒvƒ“/ƒnƒ“ƒhƒ‹Šl“¾
//
//---------------------------------------------------------------------------
HANDLE FASTCALL CHostFcb::Open()
{
	ASSERT(this);
	ASSERT(_tcslen(m_szFilename) > 0);

	// ƒtƒ@ƒCƒ‹ƒI[ƒvƒ“
	if (m_hFile == INVALID_HANDLE_VALUE) {
		DWORD nShare = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;
		DWORD nCreation = OPEN_EXISTING;
		DWORD nAttribute = FILE_ATTRIBUTE_NORMAL;
		m_hFile = ::CreateFile(m_szFilename, m_nMode, nShare, NULL, nCreation, nAttribute, NULL);
	}
	return m_hFile;
}

//---------------------------------------------------------------------------
//
//	ƒtƒ@ƒCƒ‹“Ç‚İ‚İ
//
//	0ƒoƒCƒg“Ç‚İ‚İ‚Å‚à³í“®ì‚Æ‚·‚éB
//	ƒGƒ‰[‚Ì‚Í -1 ‚ğ•Ô‚·B
//
//---------------------------------------------------------------------------
DWORD FASTCALL CHostFcb::ReadFile(void* pBuffer, DWORD nSize)
{
	ASSERT(this);
	ASSERT(pBuffer);
	ASSERT(m_hFile != INVALID_HANDLE_VALUE);

	DWORD nResult;
	BOOL bResult = ::ReadFile(m_hFile, pBuffer, nSize, &nResult, NULL);
	if (bResult == FALSE) nResult = (DWORD)-1;

	return nResult;
}

//---------------------------------------------------------------------------
//
//	ƒtƒ@ƒCƒ‹‘‚«‚İ
//
//	0ƒoƒCƒg‘‚«‚İ‚Å‚à³í“®ì‚Æ‚·‚éB
//	ƒGƒ‰[‚Ì‚Í -1 ‚ğ•Ô‚·B
//
//---------------------------------------------------------------------------
DWORD FASTCALL CHostFcb::WriteFile(void* pBuffer, DWORD nSize)
{
	ASSERT(this);
	ASSERT(pBuffer);
	ASSERT(m_hFile != INVALID_HANDLE_VALUE);

	DWORD nResult;
	BOOL bResult = ::WriteFile(m_hFile, pBuffer, nSize, &nResult, NULL);
	if (bResult == FALSE) nResult = (DWORD)-1;

	return nResult;
}

//---------------------------------------------------------------------------
//
//	ƒtƒ@ƒCƒ‹ƒ|ƒCƒ“ƒ^İ’è
//
//	ƒGƒ‰[‚Ì‚Í -1 ‚ğ•Ô‚·B
//
//---------------------------------------------------------------------------
DWORD FASTCALL CHostFcb::SetFilePointer(DWORD nOffset, DWORD nMode)
{
	ASSERT(this);
	ASSERT(nMode == FILE_BEGIN || nMode == FILE_CURRENT || nMode == FILE_END);
	ASSERT(m_hFile != INVALID_HANDLE_VALUE);

	return ::SetFilePointer(m_hFile, nOffset, NULL, nMode);
}

//---------------------------------------------------------------------------
//
//	ƒtƒ@ƒCƒ‹İ’è
//
//	ƒGƒ‰[‚Ì‚Í -1 ‚ğ•Ô‚·B
//
//---------------------------------------------------------------------------
DWORD FASTCALL CHostFcb::SetFileTime(WORD nFatDate, WORD nFatTime)
{
	ASSERT(this);
	ASSERT(m_hFile != INVALID_HANDLE_VALUE);

	FILETIME lt;
	if (::DosDateTimeToFileTime(nFatDate, nFatTime, &lt) == 0) return (DWORD)-1;
	FILETIME ft;
	if (::LocalFileTimeToFileTime(&lt, &ft) == 0) return (DWORD)-1;

	if (::SetFileTime(m_hFile, NULL, &ft, &ft) == 0) return (DWORD)-1;
	return 0;
}

//---------------------------------------------------------------------------
//
//	ƒtƒ@ƒCƒ‹ƒNƒ[ƒY
//
//---------------------------------------------------------------------------
BOOL FASTCALL CHostFcb::Close()
{
	ASSERT(this);

	BOOL bResult = TRUE;

	// ƒtƒ@ƒCƒ‹ƒNƒ[ƒY
	if (m_hFile != INVALID_HANDLE_VALUE) {
		bResult = ::CloseHandle(m_hFile);
		// ƒGƒ‰[”­¶¨Close¨Free(“à•”‚ÅÄ“xClose) ‚Æ‚¢‚¤—¬‚ê‚à‚ ‚é‚Ì‚Å
		// “ñd‚ÉClose‚µ‚È‚¢‚æ‚¤‚É‚«‚¿‚ñ‚Æİ’è‚µ‚Ä‚¨‚­
		m_hFile = INVALID_HANDLE_VALUE;
	}

	return bResult;
}

//===========================================================================
//
//	FCBˆ— ƒ}ƒl[ƒWƒƒ
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	FCB‘€ì—Ìˆæ ‰Šú‰» (‹N“®’¼Œã)
//
//---------------------------------------------------------------------------
CHostFcbManager::CHostFcbManager()
{
}

//---------------------------------------------------------------------------
//
//	FCB‘€ì—Ìˆæ Šm”F (I—¹)
//
//---------------------------------------------------------------------------
CHostFcbManager::~CHostFcbManager()
{
#ifdef _DEBUG
	// À‘Ì‚ª‘¶İ‚µ‚È‚¢‚±‚Æ‚ğŠm”F
	ASSERT(m_cRingFcb.Next() == &m_cRingFcb);
	ASSERT(m_cRingFcb.Prev() == &m_cRingFcb);

	// ”O‚Ì‚½‚ß(ÀÛ‚É‚Íg‚í‚ê‚È‚¢)
	Clean();
#endif // _DEBUG
}

//---------------------------------------------------------------------------
//
//	FCB‘€ì—Ìˆæ ‰Šú‰» (ƒhƒ‰ƒCƒo‘g‚İ)
//
//---------------------------------------------------------------------------
void FASTCALL CHostFcbManager::Init()
{
	ASSERT(this);

	// À‘Ì‚ª‘¶İ‚µ‚È‚¢‚±‚Æ‚ğŠm”F
	ASSERT(m_cRingFcb.Next() == &m_cRingFcb);
	ASSERT(m_cRingFcb.Prev() == &m_cRingFcb);

	for (int i = 0; i < XM6_HOST_FCB_MAX; i++) {
		CHostFcb* p = new CHostFcb;
		ASSERT(p);
		p->InsertTail(&m_cRingFcb);
	}
}

//---------------------------------------------------------------------------
//
//	FCB‘€ì—Ìˆæ ŠJ•ú (‹N“®EƒŠƒZƒbƒg)
//
//---------------------------------------------------------------------------
void CHostFcbManager::Clean()
{
	ASSERT(this);

	// À‘Ì‚ğŠJ•ú
	CHostFcb* p;
	while ((p = (CHostFcb*)m_cRingFcb.Next()) != &m_cRingFcb) {
		delete p;
	}
}

//---------------------------------------------------------------------------
//
//	FCB‘€ì—Ìˆæ Šm•Û
//
//---------------------------------------------------------------------------
CHostFcb* FASTCALL CHostFcbManager::Alloc(DWORD nKey)
{
	ASSERT(this);
	ASSERT(nKey != 0);

	// ––”ö‚Ì1‚Â‚ğ‘I‘ğ
	CHostFcb* p = (CHostFcb*)m_cRingFcb.Prev();

	// g—p’†‚È‚çƒGƒ‰[
	if (p->isSameKey(0) == FALSE) return NULL;

	// ŠY“–ƒIƒuƒWƒFƒNƒg‚ğƒŠƒ“ƒOæ“ª‚ÖˆÚ“®
	p->Insert(&m_cRingFcb);

	// ƒL[‚ğİ’è
	p->SetKey(nKey);

	return p;
}

//---------------------------------------------------------------------------
//
//	FCB‘€ì—Ìˆæ ŒŸõ
//
//---------------------------------------------------------------------------
CHostFcb* FASTCALL CHostFcbManager::Search(DWORD nKey)
{
	ASSERT(this);
	ASSERT(nKey != 0);

	// ŠY“–‚·‚éƒIƒuƒWƒFƒNƒg‚ğŒŸõ
	for (CHostFcb* p = (CHostFcb*)m_cRingFcb.Next();
		 p != &m_cRingFcb; p = (CHostFcb*)p->Next()) {
		if (p->isSameKey(nKey)) {
			// ŠY“–ƒIƒuƒWƒFƒNƒg‚ğƒŠƒ“ƒOæ“ª‚ÖˆÚ“®
			p->Insert(&m_cRingFcb);
			return p;
		}
	}

	return NULL;
}

//---------------------------------------------------------------------------
//
//	FCB‘€ì—Ìˆæ ŠJ•ú
//
//---------------------------------------------------------------------------
void FASTCALL CHostFcbManager::Free(CHostFcb* p)
{
	ASSERT(this);
	ASSERT(p);

	// À‘Ì‚Ì‰Šú‰» (Ä—˜—p‚·‚é‚½‚ß—ÌˆæŠJ•ú‚Ís‚È‚í‚È‚¢)
	p->Free();

	// ŠY“–ƒIƒuƒWƒFƒNƒg‚ğƒŠƒ“ƒO––”ö‚ÖˆÚ“®
	p->InsertTail(&m_cRingFcb);
}

//===========================================================================
//
//	Windowsƒtƒ@ƒCƒ‹ƒVƒXƒeƒ€
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	ƒRƒ“ƒXƒgƒ‰ƒNƒ^
//
//---------------------------------------------------------------------------
CWinFileSys::CWinFileSys()
{
	// ƒhƒ‰ƒCƒuƒIƒuƒWƒFƒNƒg‰Šú‰»
	for (int n = 0; n < DrvMax; n++) {
		m_pDrv[n] = NULL;
	}

	// ƒRƒ“ƒtƒBƒOƒf[ƒ^‰Šú‰»
	m_bResume = FALSE;
	m_nDrives = 0;

	for (int n = 0; n < DrvMax; n++) {
		m_nFlag[n] = 0;
		m_szBase[n][0] = _T('\0');
	}

	// TwentyOneƒIƒvƒVƒ‡ƒ“ŠÄ‹‰Šú‰»
	m_nKernel = 0;
	m_nKernelSearch = 0;

	// “®ìƒtƒ‰ƒO‰Šú‰»
	m_nOptionDefault = 0;
	m_nOption = 0;
	CHostFilename::SetOption(0);
	CHostPath::SetOption(0);
}

//---------------------------------------------------------------------------
//
//	ƒfƒXƒgƒ‰ƒNƒ^
//
//---------------------------------------------------------------------------
CWinFileSys::~CWinFileSys()
{
#ifdef _DEBUG
	int nDrv;

	// ƒIƒuƒWƒFƒNƒgŠm”F
	for (nDrv=0; nDrv<DrvMax; nDrv++) {
		// ‘¶İ‚µ‚È‚¢‚±‚Æ‚ğŠm”F
		ASSERT(!m_pDrv[nDrv]);

		// ”O‚Ì‚½‚ß(ÀÛ‚É‚Íg‚í‚ê‚È‚¢)
		if (m_pDrv[nDrv]) {
			delete m_pDrv[nDrv];
			m_pDrv[nDrv] = NULL;
		}
	}
#endif // _DEBUG
}

//---------------------------------------------------------------------------
//
//	İ’è“K—p
//
//---------------------------------------------------------------------------
void FASTCALL CWinFileSys::ApplyCfg(const Config* pConfig)
{
	ASSERT(this);
	ASSERT(pConfig);

	// ƒRƒ“ƒtƒBƒOƒf[ƒ^“K—p
	m_bResume = pConfig->host_resume;
	m_nDrives = pConfig->host_drives;

	for (int n = 0; n < DrvMax; n++) {
		m_nFlag[n] = pConfig->host_flag[n];
		ASSERT(_tcslen(pConfig->host_path[n]) < _MAX_PATH);
		_tcscpy(m_szBase[n], pConfig->host_path[n]);
	}

	// “®ìƒtƒ‰ƒOİ’è
	m_nOptionDefault = pConfig->host_option;
}

//---------------------------------------------------------------------------
//
//	$40 - ‰Šú‰»
//
//---------------------------------------------------------------------------
DWORD FASTCALL CWinFileSys::Init(CWindrv* ps, DWORD nDriveMax, const BYTE* pOption)
{
	ASSERT(this);
	ASSERT(nDriveMax < 26);

	// VMƒXƒŒƒbƒh‚Ì“®ì‚ğŠJn
	ps->Ready();

	// ƒGƒ‰[ƒ‚[ƒhİ’è
	::SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX);

	// ƒtƒ@ƒCƒ‹ŒŸõ—Ìˆæ ‰Šú‰» (ƒhƒ‰ƒCƒo‘g‚İ)
	m_cFiles.Init();

	// FCB‘€ì—Ìˆæ ‰Šú‰» (ƒhƒ‰ƒCƒo‘g‚İ)
	m_cFcb.Init();

	// ƒfƒBƒŒƒNƒgƒŠƒGƒ“ƒgƒŠ ‰Šú‰» (ƒhƒ‰ƒCƒo‘g‚İ)
	m_cEntry.Init(m_pDrv);

	// ƒIƒvƒVƒ‡ƒ“‰Šú‰»
	InitOption(pOption);

	// ‘Sƒhƒ‰ƒCƒuƒXƒLƒƒƒ“‚Ì—L–³‚ğ”»’è
	DWORD nConfigDrives = m_nDrives;
	if (m_bResume == FALSE) {
		// ‘SƒRƒ“ƒtƒBƒOÁ‹
		for (DWORD i = 0; i < DrvMax; i++) {
			m_nFlag[i] = 0;
			m_szBase[i][0] = _T('\0');
		}

		// ‘Sƒhƒ‰ƒCƒuƒXƒLƒƒƒ“
		nConfigDrives = 0;
		DWORD nBits = ::GetLogicalDrives();
		for (DWORD n = 0; n < 26; n++) {
			// ƒrƒbƒgƒ`ƒFƒbƒN
			if (nBits & 1) {
				// ƒx[ƒXƒpƒXİ’è
				_stprintf(m_szBase[nConfigDrives], _T("%c:\\"), 'A' + n);

				// “®ìƒtƒ‰ƒOİ’è
				m_nFlag[nConfigDrives] = 0;

				// Ÿ‚Ìƒhƒ‰ƒCƒu‚Ö
				nConfigDrives++;

				// Å‘å”‚É’B‚µ‚Ä‚¢‚ê‚ÎI—¹
				if (nConfigDrives >= DrvMax) break;
			}

			// Ÿ‚Ìƒrƒbƒg‚Ö
			nBits >>= 1;
		}
	}

	// ƒtƒ@ƒCƒ‹ƒVƒXƒeƒ€‚ğ“o˜^
	DWORD nDrives = 0;
	for (DWORD n = 0; n < nConfigDrives; n++) {	// ‘S‚Ä‚Ì—LŒø‚ÈƒRƒ“ƒtƒBƒOƒf[ƒ^‚ğ’²¸
		// ƒx[ƒXƒpƒX‚ª‘¶İ‚µ‚È‚¢ê‡‚Í–³Œø‚ÈƒfƒoƒCƒX‚Æ‚İ‚È‚·
		if (m_szBase[n][0] == _T('\0')) continue;

		// ‚±‚êˆÈã“o˜^‚Å‚«‚È‚¯‚ê‚ÎI—¹ (nDriveMax‚ª0‚Ìê‡‚Í‰½‚à“o˜^‚µ‚È‚¢)
		if (nDrives >= nDriveMax) break;

		// ƒzƒXƒgƒtƒ@ƒCƒ‹ƒVƒXƒeƒ€‚ğ1‚Â¶¬
		ASSERT(m_pDrv[nDrives] == NULL);
		m_pDrv[nDrives] = new CWinFileDrv;
		ASSERT(m_pDrv[nDrives]);

		// ‰Šú‰»
		m_pDrv[nDrives]->Init(m_szBase[n], m_nFlag[n]);

		// Ÿ‚Ìƒhƒ‰ƒCƒu‚Ö
		nDrives++;
	}

	// “o˜^‚µ‚½ƒhƒ‰ƒCƒu”‚ğ•Ô‚·
	return nDrives;
}

//---------------------------------------------------------------------------
//
//	ƒŠƒZƒbƒg(‘SƒNƒ[ƒY)
//
//---------------------------------------------------------------------------
void FASTCALL CWinFileSys::Reset()
{
	int nDrv;

	ASSERT(this);

	// ‰¼‘zƒZƒNƒ^—Ìˆæ‰Šú‰»
	m_nHostSectorCount = 0;
	memset(m_nHostSectorBuffer, 0, sizeof(m_nHostSectorBuffer));

	// ƒtƒ@ƒCƒ‹ŒŸõ—Ìˆæ ŠJ•ú (‹N“®EƒŠƒZƒbƒg)
	m_cFiles.Clean();

	// FCB‘€ì—Ìˆæ ŠJ•ú (‹N“®EƒŠƒZƒbƒg)
	m_cFcb.Clean();

	// ƒfƒBƒŒƒNƒgƒŠƒGƒ“ƒgƒŠ ŠJ•ú (‹N“®EƒŠƒZƒbƒg)
	m_cEntry.Clean();

	// ƒIƒuƒWƒFƒNƒgíœ
	for (nDrv=0; nDrv<DrvMax; nDrv++) {
		if (m_pDrv[nDrv]) {
			delete m_pDrv[nDrv];
			m_pDrv[nDrv] = NULL;
		}
	}

	// TwentyOneƒIƒvƒVƒ‡ƒ“ŠÄ‹‰Šú‰»
	m_nKernel = 0;
	m_nKernelSearch = 0;
}

//---------------------------------------------------------------------------
//
//	$41 - ƒfƒBƒŒƒNƒgƒŠƒ`ƒFƒbƒN
//
//---------------------------------------------------------------------------
int FASTCALL CWinFileSys::CheckDir(CWindrv* ps, const Human68k::namests_t* pNamests)
{
	ASSERT(this);
	ASSERT(pNamests);

	// ’¼Œã‚É•K‚¸‘¼‚ÌƒRƒ}ƒ“ƒh‚ªÀs‚³‚ê‚é‚½‚ßAƒƒfƒBƒAƒ`ƒFƒbƒN‚Í•s—v
	// ‚±‚±‚Åƒ`ƒFƒbƒN‚·‚é‚Æmint‚Ìƒhƒ‰ƒCƒu•ÏX‘O‚É–³‘Ê‚ÈƒfƒBƒXƒNƒAƒNƒZƒX‚ª”­¶‚µ‚Ä‚µ‚Ü‚¤
	//if (m_cEntry.isMediaOffline(ps)) return FS_FATAL_MEDIAOFFLINE;

#ifdef XM6_HOST_UPDATE_BY_SEQUENCE
	// ’¼Œã‚ÌƒƒfƒBƒAŒğŠ·ƒ`ƒFƒbƒNƒRƒ}ƒ“ƒh‚ğ—LŒø‚É‚·‚é‚½‚ßƒtƒ‰ƒO‚ğ—§‚Ä‚é
	m_cEntry.SetMediaUpdate(ps, FALSE);
#endif // XM6_HOST_UPDATE_BY_SEQUENCE

	// ƒpƒX–¼¶¬
	CHostFiles f;
	DWORD nUnit = ps->GetUnit();
	f.SetPath(nUnit, pNamests);
	if (f.isRootPath()) return 0;
	f.SetPathOnly();
	if (f.Find(ps, &m_cEntry) == FALSE) return FS_DIRNOTFND;

	return 0;
}

//---------------------------------------------------------------------------
//
//	$42 - ƒfƒBƒŒƒNƒgƒŠì¬
//
//---------------------------------------------------------------------------
int FASTCALL CWinFileSys::MakeDir(CWindrv* ps, const Human68k::namests_t* pNamests)
{
	ASSERT(this);
	ASSERT(pNamests);

	// ƒƒfƒBƒAƒ`ƒFƒbƒN
	if (m_cEntry.isMediaOffline(ps)) return FS_FATAL_MEDIAOFFLINE;

	// ‘‚«‚İ‹Ö~ƒ`ƒFƒbƒN
	if (m_cEntry.isWriteProtect(ps)) return FS_FATAL_WRITEPROTECT;

	// ƒpƒX–¼¶¬
	CHostFiles f;
	DWORD nUnit = ps->GetUnit();
	f.SetPath(nUnit, pNamests);
	f.SetPathOnly();
	if (f.Find(ps, &m_cEntry) == FALSE) return FS_INVALIDPATH;
	f.AddFilename();

	// ƒfƒBƒŒƒNƒgƒŠì¬
	BOOL bResult = ::CreateDirectory(f.GetPath(), NULL);
	if (bResult == FALSE) return FS_INVALIDPATH;

	return 0;
}

//---------------------------------------------------------------------------
//
//	$43 - ƒfƒBƒŒƒNƒgƒŠíœ
//
//---------------------------------------------------------------------------
int FASTCALL CWinFileSys::RemoveDir(CWindrv* ps, const Human68k::namests_t* pNamests)
{
	ASSERT(this);
	ASSERT(pNamests);

	// ƒƒfƒBƒAƒ`ƒFƒbƒN
	if (m_cEntry.isMediaOffline(ps)) return FS_FATAL_MEDIAOFFLINE;

	// ‘‚«‚İ‹Ö~ƒ`ƒFƒbƒN
	if (m_cEntry.isWriteProtect(ps)) return FS_FATAL_WRITEPROTECT;

	// ƒpƒX–¼¶¬
	CHostFiles f;
	DWORD nUnit = ps->GetUnit();
	f.SetPath(nUnit, pNamests);

	// ƒfƒBƒŒƒNƒgƒŠŠm”F
	f.SetAttribute(Human68k::AT_DIRECTORY);
	if (f.Find(ps, &m_cEntry) == FALSE) return FS_DIRNOTFND;

	// ƒfƒBƒŒƒNƒgƒŠíœ
	if ((m_nOption & WINDRV_OPT_REMOVE) == 0) {
		BOOL bResult = ::RemoveDirectory(f.GetPath());
		if (bResult == FALSE) return FS_CANTDELETE;
	} else {
		// ƒfƒBƒŒƒNƒgƒŠ“à•”‚ª‹ó‚©‚Ç‚¤‚©Šm”F
		TCHAR szBuf[_MAX_PATH];
		_tcscpy(szBuf, f.GetPath());
		_tcscat(szBuf, "\\*.*");
		WIN32_FIND_DATA w32Find;
		HANDLE hFind = ::FindFirstFile(szBuf, &w32Find);
		if (hFind != INVALID_HANDLE_VALUE) {
			for (;;) {
				if (strcmp(w32Find.cFileName, ".") != 0 &&
					strcmp(w32Find.cFileName, "..") != 0) {
					FindClose(hFind);
					return FS_CANTDELETE;
				}
				BOOL bResult = ::FindNextFile(hFind, &w32Find);
				if (bResult == FALSE) break;
			}
			FindClose(hFind);
		}

		// WARNING: Unicode‘Î‰—vC³
		char szBuffer[_MAX_PATH + 1];
		strcpy(szBuffer, f.GetPath());
		szBuffer[strlen(szBuffer) + 1] = '\0';

		SHFILEOPSTRUCT sop;
		sop.hwnd = NULL;
		sop.wFunc = FO_DELETE;
		sop.pFrom = szBuffer;
		sop.pTo = NULL;
		sop.fFlags = FOF_ALLOWUNDO | FOF_NOCONFIRMATION | FOF_NOERRORUI | FOF_SILENT;
		sop.fAnyOperationsAborted = TRUE;
		sop.hNameMappings = NULL;
		sop.lpszProgressTitle = NULL;

		int nResult = ::SHFileOperation(&sop);
		if (nResult != 0) return FS_CANTDELETE;
	}

	// ƒLƒƒƒbƒVƒ…íœ
	f.Find(ps, &m_cEntry, TRUE);

	return 0;
}

//---------------------------------------------------------------------------
//
//	$44 - ƒtƒ@ƒCƒ‹–¼•ÏX
//
//---------------------------------------------------------------------------
int FASTCALL CWinFileSys::Rename(CWindrv* ps, const Human68k::namests_t* pNamests, const Human68k::namests_t* pNamestsNew)
{
	ASSERT(this);
	ASSERT(pNamests);

	// ƒƒfƒBƒAƒ`ƒFƒbƒN
	if (m_cEntry.isMediaOffline(ps)) return FS_FATAL_MEDIAOFFLINE;

	// ‘‚«‚İ‹Ö~ƒ`ƒFƒbƒN
	if (m_cEntry.isWriteProtect(ps)) return FS_FATAL_WRITEPROTECT;

	// ƒpƒX–¼¶¬
	CHostFiles f;
	DWORD nUnit = ps->GetUnit();
	f.SetPath(nUnit, pNamests);
	f.SetAttribute(Human68k::AT_ALL);
	if (f.Find(ps, &m_cEntry) == FALSE) return FS_FILENOTFND;

	CHostFiles fNew;
	fNew.SetPath(nUnit, pNamestsNew);
	fNew.SetPathOnly();
	if (fNew.Find(ps, &m_cEntry) == FALSE) return FS_INVALIDPATH;
	fNew.AddFilename();

	// ƒtƒ@ƒCƒ‹–¼•ÏX
	BOOL bResult = ::MoveFile(f.GetPath(), fNew.GetPath());
	if (bResult == FALSE) return FS_FILENOTFND;

	return 0;
}

//---------------------------------------------------------------------------
//
//	$45 - ƒtƒ@ƒCƒ‹íœ
//
//---------------------------------------------------------------------------
int FASTCALL CWinFileSys::Delete(CWindrv* ps, const Human68k::namests_t* pNamests)
{
	ASSERT(this);
	ASSERT(pNamests);

	// ƒƒfƒBƒAƒ`ƒFƒbƒN
	if (m_cEntry.isMediaOffline(ps)) return FS_FATAL_MEDIAOFFLINE;

	// ‘‚«‚İ‹Ö~ƒ`ƒFƒbƒN
	if (m_cEntry.isWriteProtect(ps)) return FS_FATAL_WRITEPROTECT;

	// ƒpƒX–¼¶¬
	CHostFiles f;
	DWORD nUnit = ps->GetUnit();
	f.SetPath(nUnit, pNamests);

	// ƒtƒ@ƒCƒ‹Šm”F
	if (f.Find(ps, &m_cEntry) == FALSE) return FS_FILENOTFND;

	// ƒtƒ@ƒCƒ‹íœ
	if ((m_nOption & WINDRV_OPT_REMOVE) == 0) {
		BOOL bResult = ::DeleteFile(f.GetPath());
		if (bResult == FALSE) return FS_CANTDELETE;
	} else {
		// WARNING: Unicode‘Î‰—vC³
		char szBuffer[_MAX_PATH + 1];
		strcpy(szBuffer, f.GetPath());
		szBuffer[strlen(szBuffer) + 1] = '\0';

		SHFILEOPSTRUCT sop;
		sop.hwnd = NULL;
		sop.wFunc = FO_DELETE;
		sop.pFrom = szBuffer;
		sop.pTo = NULL;
		sop.fFlags = FOF_ALLOWUNDO | FOF_NOCONFIRMATION | FOF_NOERRORUI | FOF_SILENT;
		sop.fAnyOperationsAborted = TRUE;
		sop.hNameMappings = NULL;
		sop.lpszProgressTitle = NULL;

		int nResult = ::SHFileOperation(&sop);
		if (nResult != 0) return FS_CANTDELETE;
	}

	return 0;
}

//---------------------------------------------------------------------------
//
//	$46 - ƒtƒ@ƒCƒ‹‘®«æ“¾/İ’è
//
//---------------------------------------------------------------------------
int FASTCALL CWinFileSys::Attribute(CWindrv* ps, const Human68k::namests_t* pNamests, DWORD nHumanAttribute)
{
	ASSERT(this);
	ASSERT(pNamests);

	// ƒƒfƒBƒAƒ`ƒFƒbƒN
	if (m_cEntry.isMediaOffline(ps)) return FS_FATAL_MEDIAOFFLINE;

	// ƒpƒX–¼¶¬
	CHostFiles f;
	DWORD nUnit = ps->GetUnit();
	f.SetPath(nUnit, pNamests);
	f.SetAttribute(Human68k::AT_ALL);
	if (f.Find(ps, &m_cEntry) == FALSE) return FS_FILENOTFND;

	// ‘®«æ“¾‚È‚çI—¹
	if (nHumanAttribute == 0xFF) {
		return f.GetAttribute();
	}

	// ‘‚«‚İ‹Ö~ƒ`ƒFƒbƒN
	if (m_cEntry.isWriteProtect(ps)) return FS_FATAL_WRITEPROTECT;

#if 0
	if (f.GetAttribute() & Human68k::AT_DIRECTORY) {
		if ((nHumanAttribute & Human68k::AT_DIRECTORY) == 0) {
			OutputDebugString("Warning: ƒfƒBƒŒƒNƒgƒŠ‚Ì‘®«‚ğƒtƒ@ƒCƒ‹‚É•ÏX‚Å‚«‚Ü‚¹‚ñ\r\n");
		}
	}

	if ((f.GetAttribute() & Human68k::AT_DIRECTORY) == 0) {
		if (nHumanAttribute & Human68k::AT_DIRECTORY) {
			OutputDebugString("Warning: ƒtƒ@ƒCƒ‹‚Ì‘®«‚ğƒfƒBƒŒƒNƒgƒŠ‚É•ÏX‚Å‚«‚Ü‚¹‚ñ\r\n");
		}
	}
#endif

	// ‘®«¶¬
	DWORD nAttribute = 0;
	if ((nHumanAttribute & Human68k::AT_SYSTEM) != 0) nAttribute |= FILE_ATTRIBUTE_SYSTEM;
	if ((nHumanAttribute & Human68k::AT_HIDDEN) != 0) nAttribute |= FILE_ATTRIBUTE_HIDDEN;
	if ((nHumanAttribute & Human68k::AT_READONLY) != 0) nAttribute |= FILE_ATTRIBUTE_READONLY;
	if (nAttribute == 0) nAttribute = FILE_ATTRIBUTE_NORMAL;

	// ‘®«İ’è
	BOOL bResult = ::SetFileAttributes(f.GetPath(), nAttribute);
	if (bResult == FALSE) return FS_FILENOTFND;

	// •ÏXŒã‚Ì‘®«æ“¾
	if (f.Find(ps, &m_cEntry) == FALSE) return FS_FILENOTFND;
	return f.GetAttribute();
}

//---------------------------------------------------------------------------
//
//	$47 - ƒtƒ@ƒCƒ‹ŒŸõ(First)
//
//---------------------------------------------------------------------------
int FASTCALL CWinFileSys::Files(CWindrv* ps, const Human68k::namests_t* pNamests, DWORD nKey, Human68k::files_t* pFiles)
{
	ASSERT(this);
	ASSERT(pNamests);
	ASSERT(nKey);
	ASSERT(pFiles);

	// Šù‚É“¯‚¶ƒL[‚ğ‚Â—Ìˆæ‚ª‚ ‚ê‚ÎŠJ•ú‚µ‚Ä‚¨‚­
	CHostFiles* pHostFiles = m_cFiles.Search(nKey);
	if (pHostFiles != NULL) {
		m_cFiles.Free(pHostFiles);
	}

	// ƒ{ƒŠƒ…[ƒ€ƒ‰ƒxƒ‹‚Ìê‡
	if (pFiles->fatr == Human68k::AT_VOLUME) {
		// ƒoƒbƒtƒ@‚ğŠm•Û‚¹‚¸A‚¢‚«‚È‚èŒ‹‰Ê‚ğ•Ô‚·
		if (FilesVolume(ps, pFiles) == FALSE) {
			return FS_FILENOTFND;
		}
		return 0;
	}

	// ƒƒfƒBƒAƒ`ƒFƒbƒN
	if (m_cEntry.isMediaOffline(ps)) return FS_FATAL_MEDIAOFFLINE;

	// ƒoƒbƒtƒ@Šm•Û
	pHostFiles = m_cFiles.Alloc(nKey);
	if (pHostFiles == NULL) {
		return FS_OUTOFMEM;
	}

	// ƒfƒBƒŒƒNƒgƒŠƒ`ƒFƒbƒN
	DWORD nUnit = ps->GetUnit();
	pHostFiles->SetPath(nUnit, pNamests);
	if (pHostFiles->isRootPath() == FALSE) {
		pHostFiles->SetPathOnly();
		if (pHostFiles->Find(ps, &m_cEntry) == FALSE) {
			m_cFiles.Free(pHostFiles);
			return FS_DIRNOTFND;
		}
	}

	// ƒƒCƒ‹ƒhƒJ[ƒhg—p‰Â”\‚Éİ’è
	pHostFiles->SetPathWildcard();
	pHostFiles->SetAttribute(pFiles->fatr);

	// ƒtƒ@ƒCƒ‹ŒŸõ
	if (pHostFiles->Find(ps, &m_cEntry) == FALSE) {
		m_cFiles.Free(pHostFiles);
		return FS_FILENOTFND;
	}

	// ŒŸõŒ‹‰Ê‚ğŠi”[
	pFiles->attr = (BYTE)pHostFiles->GetAttribute();
	pFiles->date = pHostFiles->GetDate();
	pFiles->time = pHostFiles->GetTime();
	pFiles->size = pHostFiles->GetSize();
	strcpy((char*)pFiles->full, (char*)pHostFiles->GetHumanResult());

	// ‹[—ƒfƒBƒŒƒNƒgƒŠƒGƒ“ƒgƒŠ‚ğw’è
	pFiles->sector = nKey;
	pFiles->offset = 0;

	// ƒtƒ@ƒCƒ‹–¼‚ÉƒƒCƒ‹ƒhƒJ[ƒh‚ª‚È‚¯‚ê‚ÎA‚±‚Ì“_‚Åƒoƒbƒtƒ@‚ğŠJ•ú‰Â
	if (pNamests->wildcard == 0) {
		// ‚
		//m_cFiles.Free(pHostFiles);
	}

	return 0;
}

//---------------------------------------------------------------------------
//
//	$48 - ƒtƒ@ƒCƒ‹ŒŸõ(Next)
//
//---------------------------------------------------------------------------
int FASTCALL CWinFileSys::NFiles(CWindrv* ps, DWORD nKey, Human68k::files_t* pFiles)
{
	ASSERT(this);
	ASSERT(nKey);
	ASSERT(pFiles);

	// ƒƒfƒBƒAƒ`ƒFƒbƒN
	if (m_cEntry.isMediaOffline(ps)) return FS_FATAL_MEDIAOFFLINE;

	// ƒoƒbƒtƒ@ŒŸõ
	CHostFiles* pHostFiles = m_cFiles.Search(nKey);
	if (pHostFiles == NULL) {
		return FS_FILENOTFND;
	}

	// ƒtƒ@ƒCƒ‹ŒŸõ
	if (pHostFiles->Find(ps, &m_cEntry) == FALSE) {
		m_cFiles.Free(pHostFiles);
		return FS_FILENOTFND;
	}

	// ŒŸõŒ‹‰Ê‚ğŠi”[
	pFiles->attr = (BYTE)pHostFiles->GetAttribute();
	pFiles->date = pHostFiles->GetDate();
	pFiles->time = pHostFiles->GetTime();
	pFiles->size = pHostFiles->GetSize();
	strcpy((char*)pFiles->full, (char*)pHostFiles->GetHumanResult());

	return 0;
}

//---------------------------------------------------------------------------
//
//	$49 - ƒtƒ@ƒCƒ‹V‹Kì¬
//
//---------------------------------------------------------------------------
int FASTCALL CWinFileSys::Create(CWindrv* ps, const Human68k::namests_t* pNamests, DWORD nKey, Human68k::fcb_t* pFcb, DWORD nHumanAttribute, BOOL bForce)
{
	ASSERT(this);
	ASSERT(pNamests);
	ASSERT(nKey);
	ASSERT(pFcb);

	// ƒƒfƒBƒAƒ`ƒFƒbƒN
	if (m_cEntry.isMediaOffline(ps)) return FS_FATAL_MEDIAOFFLINE;

	// ‘‚«‚İ‹Ö~ƒ`ƒFƒbƒN
	if (m_cEntry.isWriteProtect(ps)) return FS_FATAL_WRITEPROTECT;

	// Šù‚É“¯‚¶ƒL[‚ğ‚Â—Ìˆæ‚ª‚ ‚ê‚ÎƒGƒ‰[‚Æ‚·‚é
	CHostFcb* pHostFcb = m_cFcb.Search(nKey);
	if (pHostFcb != NULL) return FS_FILEEXIST;

	// ƒtƒ@ƒCƒ‹ã‘‚«ƒ`ƒFƒbƒN
	CHostFiles f;
	DWORD nUnit = ps->GetUnit();
	f.SetPath(nUnit, pNamests);

	// ƒpƒX–¼¶¬
	f.SetPathOnly();
	if (f.Find(ps, &m_cEntry) == FALSE) return FS_INVALIDPATH;
	f.AddFilename();

	// ƒpƒX–¼•Û‘¶
	pHostFcb = m_cFcb.Alloc(nKey);
	if (pHostFcb == NULL) return FS_OUTOFMEM;
	pHostFcb->SetFilename(f.GetPath());

	// ƒI[ƒvƒ“ƒ‚[ƒhİ’è
	pFcb->mode = (pFcb->mode & ~0x0F) | Human68k::OP_READWRITE;
	if (pHostFcb->SetOpenMode(pFcb->mode) == FALSE) {
		m_cFcb.Free(pHostFcb);
		return FS_ILLEGALMOD;
	}

	// ƒtƒ@ƒCƒ‹ì¬
	HANDLE hFile = pHostFcb->Create(nHumanAttribute, bForce);
	if (hFile == INVALID_HANDLE_VALUE) {
		m_cFcb.Free(pHostFcb);
		return FS_FILEEXIST;
	}

#ifdef XM6_HOST_STRICT_CLOSE
	// ‚·‚®•Â‚¶‚é
	pHostFcb->Close();
#endif // XM6_HOST_STRICT_CLOSE

	return 0;
}

//---------------------------------------------------------------------------
//
//	$4A - ƒtƒ@ƒCƒ‹ƒI[ƒvƒ“
//
//---------------------------------------------------------------------------
int FASTCALL CWinFileSys::Open(CWindrv* ps, const Human68k::namests_t* pNamests, DWORD nKey, Human68k::fcb_t* pFcb)
{
	ASSERT(this);
	ASSERT(pNamests);
	ASSERT(nKey);
	ASSERT(pFcb);

	// ƒƒfƒBƒAƒ`ƒFƒbƒN
	if (m_cEntry.isMediaOffline(ps)) return FS_FATAL_MEDIAOFFLINE;

	// ‘‚«‚İ‹Ö~ƒ`ƒFƒbƒN
	switch (pFcb->mode) {
	case Human68k::OP_WRITE:
	case Human68k::OP_READWRITE:
		if (m_cEntry.isWriteProtect(ps)) return FS_FATAL_WRITEPROTECT;
	}

	// Šù‚É“¯‚¶ƒL[‚ğ‚Â—Ìˆæ‚ª‚ ‚ê‚ÎƒGƒ‰[‚Æ‚·‚é
	CHostFcb* pHostFcb = m_cFcb.Search(nKey);
	if (pHostFcb != NULL) return FS_FILEEXIST;

	// ƒpƒX–¼¶¬
	CHostFiles f;
	DWORD nUnit = ps->GetUnit();
	f.SetPath(nUnit, pNamests);

	// ƒtƒ@ƒCƒ‹î•ñæ“¾
	if (f.Find(ps, &m_cEntry) == FALSE) return FS_FILENOTFND;

	// ƒ^ƒCƒ€ƒXƒ^ƒ“ƒv
	pFcb->date = f.GetDate();
	pFcb->time = f.GetTime();

	// ƒtƒ@ƒCƒ‹ƒTƒCƒY
	pFcb->size = f.GetSize();

	// ƒpƒX–¼•Û‘¶
	pHostFcb = m_cFcb.Alloc(nKey);
	if (pHostFcb == NULL) return FS_OUTOFMEM;
	pHostFcb->SetFilename(f.GetPath());

	// ƒI[ƒvƒ“ƒ‚[ƒhİ’è
	if (pHostFcb->SetOpenMode(pFcb->mode) == FALSE) {
		m_cFcb.Free(pHostFcb);
		return FS_ILLEGALMOD;
	}

	// ƒtƒ@ƒCƒ‹ƒI[ƒvƒ“
	HANDLE hFile = pHostFcb->Open();
	if (hFile == INVALID_HANDLE_VALUE) {
		m_cFcb.Free(pHostFcb);
		return FS_INVALIDPATH;
	}

#ifdef XM6_HOST_STRICT_CLOSE
	// ‚·‚®•Â‚¶‚é
	pHostFcb->Close();
#endif // XM6_HOST_STRICT_CLOSE

	return 0;
}

//---------------------------------------------------------------------------
//
//	$4B - ƒtƒ@ƒCƒ‹ƒNƒ[ƒY
//
//---------------------------------------------------------------------------
int FASTCALL CWinFileSys::Close(CWindrv* ps, DWORD nKey, Human68k::fcb_t* pFcb)
{
	ASSERT(this);
	ASSERT(nKey);
	ASSERT(pFcb);

	// ƒƒfƒBƒAƒ`ƒFƒbƒN
	if (m_cEntry.isMediaOffline(ps)) return FS_FATAL_MEDIAOFFLINE;

	// Šù‚É“¯‚¶ƒL[‚ğ‚Â—Ìˆæ‚ª‚È‚¯‚ê‚ÎƒGƒ‰[‚Æ‚·‚é
	CHostFcb* pHostFcb = m_cFcb.Search(nKey);
	if (pHostFcb == NULL) return FS_INVALIDPRM;

	// ƒtƒ@ƒCƒ‹ƒNƒ[ƒY‚Æ—ÌˆæŠJ•ú
	//pHostFcb->Close();	// Free‚É©“®Às‚³‚ê‚é‚Ì‚Å•s—v
	m_cFcb.Free(pHostFcb);

	return 0;
}

//---------------------------------------------------------------------------
//
//	$4C - ƒtƒ@ƒCƒ‹“Ç‚İ‚İ
//
//	0ƒoƒCƒg“Ç‚İ‚İ‚Å‚à³íI—¹‚·‚éB
//
//---------------------------------------------------------------------------
int FASTCALL CWinFileSys::Read(CWindrv* ps, DWORD nKey, Human68k::fcb_t* pFcb, DWORD nAddress, DWORD nSize)
{
	ASSERT(this);
	ASSERT(nKey);
	ASSERT(pFcb);
	ASSERT(nAddress);

	Memory* pMemory = ps->GetMemory();
	ASSERT(pMemory);

	// ƒƒfƒBƒAƒ`ƒFƒbƒN
	if (m_cEntry.isMediaOffline(ps)) return FS_FATAL_MEDIAOFFLINE;

	// Šù‚É“¯‚¶ƒL[‚ğ‚Â—Ìˆæ‚ª‚È‚¯‚ê‚ÎƒGƒ‰[‚Æ‚·‚é
	CHostFcb* pHostFcb = m_cFcb.Search(nKey);
	if (pHostFcb == NULL) return FS_NOTOPENED;

	// ƒtƒ@ƒCƒ‹ƒI[ƒvƒ“/ƒnƒ“ƒhƒ‹Šl“¾
	HANDLE hFile = pHostFcb->Open();
	if (hFile == INVALID_HANDLE_VALUE) {
		m_cFcb.Free(pHostFcb);
		return FS_NOTOPENED;
	}

	DWORD nResult;
#ifdef XM6_HOST_STRICT_CLOSE
	// ƒtƒ@ƒCƒ‹ƒ|ƒCƒ“ƒ^•œŒ³
	nResult = pHostFcb->SetFilePointer(pFcb->fileptr);
	if (nResult == (DWORD)-1) {
		m_cFcb.Free(pHostFcb);
		return FS_CANTSEEK;
	}
#endif // XM6_HOST_STRICT_CLOSE

	// ƒtƒ@ƒCƒ‹“Ç‚İ‚İ
	DWORD nTotal = 0;
	BYTE chBuffer[XM6_HOST_FILE_BUFFER_READ];
	for (DWORD nOffset = 0; nOffset < nSize; nOffset += XM6_HOST_FILE_BUFFER_READ) {
		// ˆµ‚¤ƒTƒCƒY‚ª‘å‚«‚¢ê‡‚ÍVMƒXƒŒƒbƒh‚Ì“®ì‚ğŠJn‚³‚¹‚é
		if (nOffset == XM6_HOST_FILE_BUFFER_READ) ps->Ready();

		// ƒTƒCƒYŒˆ’è
		DWORD n = nSize - nOffset;
		if (n > XM6_HOST_FILE_BUFFER_READ) n = XM6_HOST_FILE_BUFFER_READ;

		// •”•ª“Ç‚İ‚İ
		nResult = pHostFcb->ReadFile(chBuffer, n);
		if (nResult == (DWORD)-1) {
			m_cFcb.Free(pHostFcb);
			return FS_INVALIDFUNC;
		}

		ps->LockXM();

#if 0
		// 8ƒrƒbƒg’PˆÊ‚Åƒf[ƒ^“]‘—
		for (DWORD i = 0; i < nResult; i++) pMemory->WriteByte(nAddress++, chBuffer[i]);
#else
		// æ“ª‚ªŠï”ƒAƒhƒŒƒX‚È‚çÅ‰‚Ì1ƒoƒCƒg“]‘—
		BYTE* pBuffer = chBuffer;
		DWORD nEnd = nAddress + nResult;
		if (nAddress < nEnd && (nAddress & 1) != 0)
			pMemory->WriteByte(nAddress++, *pBuffer++);

		// 16ƒrƒbƒg’PˆÊ‚Åƒf[ƒ^“]‘—
		DWORD nEndWord = nEnd & ~1;
		while (nAddress < nEndWord) {
			DWORD nData = (*pBuffer++)<<8;
			nData |= *pBuffer++;
			pMemory->WriteWord(nAddress, nData);
			nAddress += 2;
		}

		// ƒf[ƒ^‚ªc‚Á‚Ä‚¢‚ê‚Î(––”ö‚ª‹ô”ƒAƒhƒŒƒX‚È‚Ì‚Å)ÅŒã‚Ì1ƒoƒCƒg“]‘—
		if (nAddress < nEnd) pMemory->WriteByte(nAddress++, *pBuffer);
#endif

		ps->UnlockXM();

		// ƒTƒCƒYWŒv
		nTotal += nResult;

		// ƒtƒ@ƒCƒ‹I’[‚È‚çI—¹
		if (nResult != n) break;
	}

	// ƒtƒ@ƒCƒ‹ƒ|ƒCƒ“ƒ^•Û‘¶
	pFcb->fileptr += nTotal;

#ifdef XM6_HOST_STRICT_CLOSE
	// ‚·‚®•Â‚¶‚é
	pHostFcb->Close();
#endif // XM6_HOST_STRICT_CLOSE

	return nTotal;
}

//---------------------------------------------------------------------------
//
//	$4D - ƒtƒ@ƒCƒ‹‘‚«‚İ
//
//	0ƒoƒCƒg‘‚«‚İ‚Å‚à³íI—¹‚·‚éB
//	WARNING: ƒoƒXƒGƒ‰[‚ª”­¶‚·‚éƒAƒhƒŒƒX‚ğw’è‚µ‚½ê‡AÀ‹@‚Æ“®ì‚ªˆÙ‚È‚é‰Â”\«‚ ‚è
//
//---------------------------------------------------------------------------
int FASTCALL CWinFileSys::Write(CWindrv* ps, DWORD nKey, Human68k::fcb_t* pFcb, DWORD nAddress, DWORD nSize)
{
	ASSERT(this);
	ASSERT(nKey);
	ASSERT(pFcb);
	ASSERT(nAddress);

	Memory* pMemory = ps->GetMemory();
	ASSERT(pMemory);

	// ƒƒfƒBƒAƒ`ƒFƒbƒN
	if (m_cEntry.isMediaOffline(ps)) return FS_FATAL_MEDIAOFFLINE;

	// Šù‚É“¯‚¶ƒL[‚ğ‚Â—Ìˆæ‚ª‚È‚¯‚ê‚ÎƒGƒ‰[‚Æ‚·‚é
	CHostFcb* pHostFcb = m_cFcb.Search(nKey);
	if (pHostFcb == NULL) return FS_NOTOPENED;

	// ƒtƒ@ƒCƒ‹ƒI[ƒvƒ“/ƒnƒ“ƒhƒ‹Šl“¾
	HANDLE hFile = pHostFcb->Open();
	if (hFile == INVALID_HANDLE_VALUE) {
		m_cFcb.Free(pHostFcb);
		return FS_NOTOPENED;
	}

	DWORD nResult;
#ifdef XM6_HOST_STRICT_CLOSE
	// ƒtƒ@ƒCƒ‹ƒ|ƒCƒ“ƒ^•œŒ³
	nResult = pHostFcb->SetFilePointer(pFcb->fileptr);
	if (nResult == (DWORD)-1) {
		m_cFcb.Free(pHostFcb);
		return FS_CANTSEEK;
	}
#endif // XM6_HOST_STRICT_CLOSE

	// ƒtƒ@ƒCƒ‹‘‚«‚İ
	DWORD nTotal = 0;
	BYTE chBuffer[XM6_HOST_FILE_BUFFER_WRITE];
	for (DWORD nOffset = 0; nOffset < nSize; nOffset += XM6_HOST_FILE_BUFFER_WRITE) {
		// ˆµ‚¤ƒTƒCƒY‚ª‘å‚«‚¢ê‡‚ÍVMƒXƒŒƒbƒh‚Ì“®ì‚ğŠJn‚³‚¹‚é
		if (nOffset == XM6_HOST_FILE_BUFFER_WRITE) ps->Ready();

		// ƒTƒCƒYŒˆ’è
		DWORD n = nSize - nOffset;
		if (n > XM6_HOST_FILE_BUFFER_WRITE) n = XM6_HOST_FILE_BUFFER_WRITE;

		ps->LockXM();

#if 0
		// ƒf[ƒ^“]‘—
		for (DWORD i = 0; i < n; i++) chBuffer[i] = (BYTE)pMemory->ReadOnly(nAddress++);
#else
		// æ“ª‚ªŠï”ƒAƒhƒŒƒX‚È‚çÅ‰‚Ì1ƒoƒCƒg“]‘—
		BYTE* pBuffer = chBuffer;
		DWORD nEnd = nAddress + n;
		if (nAddress < nEnd && (nAddress & 1) != 0)
			*pBuffer++ = (BYTE)pMemory->ReadOnly(nAddress++);

		// 16ƒrƒbƒg’PˆÊ‚Åƒf[ƒ^“]‘—
		DWORD nEndWord = nEnd & ~1;
		while (nAddress < nEndWord) {
			DWORD nData = pMemory->ReadWord(nAddress);
			*pBuffer++ = (BYTE)(nData>>8);
			*pBuffer++ = (BYTE)nData;
			nAddress += 2;
		}

		// ƒf[ƒ^‚ªc‚Á‚Ä‚¢‚ê‚Î(––”ö‚ª‹ô”ƒAƒhƒŒƒX‚È‚Ì‚Å)ÅŒã‚Ì1ƒoƒCƒg“]‘—
		if (nAddress < nEnd) *pBuffer = (BYTE)pMemory->ReadOnly(nAddress++);
#endif

		ps->UnlockXM();

		// ‘‚«‚İ
		nResult = pHostFcb->WriteFile(chBuffer, n);
		if (nResult == (DWORD)-1) {
			m_cFcb.Free(pHostFcb);
			return FS_CANTWRITE;
		}

		// ƒTƒCƒYWŒv
		nTotal += nResult;

		// ƒtƒ@ƒCƒ‹I’[‚È‚çI—¹
		if (nResult != n) break;
	}

	// ƒtƒ@ƒCƒ‹ƒ|ƒCƒ“ƒ^•Û‘¶
	pFcb->fileptr += nTotal;

	// ƒtƒ@ƒCƒ‹ƒTƒCƒYXV
	if (pFcb->size < pFcb->fileptr) pFcb->size = pFcb->fileptr;

#ifdef XM6_HOST_STRICT_CLOSE
	// ‚·‚®•Â‚¶‚é
	pHostFcb->Close();
#endif // XM6_HOST_STRICT_CLOSE

	return nTotal;
}

//---------------------------------------------------------------------------
//
//	$4E - ƒtƒ@ƒCƒ‹ƒV[ƒN
//
//---------------------------------------------------------------------------
int FASTCALL CWinFileSys::Seek(CWindrv* ps, DWORD nKey, Human68k::fcb_t* pFcb, DWORD nMode, int nOffset)
{
	ASSERT(this);
	ASSERT(pFcb);

	// ƒƒfƒBƒAƒ`ƒFƒbƒN
	if (m_cEntry.isMediaOffline(ps)) return FS_FATAL_MEDIAOFFLINE;

	// Šù‚É“¯‚¶ƒL[‚ğ‚Â—Ìˆæ‚ª‚È‚¯‚ê‚ÎƒGƒ‰[‚Æ‚·‚é
	CHostFcb* pHostFcb = m_cFcb.Search(nKey);
	if (pHostFcb == NULL) return FS_NOTOPENED;

	// §Œäƒpƒ‰ƒ[ƒ^
	DWORD nSeek;
	switch (nMode) {
	case Human68k::SK_BEGIN:
		nSeek = FILE_BEGIN;
		break;
	case Human68k::SK_CURRENT:
#ifdef XM6_HOST_STRICT_CLOSE
		nSeek = FILE_BEGIN;
		nOffset += pFcb->fileptr;
#else // XM6_HOST_STRICT_CLOSE
		nSeek = FILE_CURRENT;
#endif // XM6_HOST_STRICT_CLOSE
		break;
	case Human68k::SK_END:
		nSeek = FILE_END;
		break;
	default:
		m_cFcb.Free(pHostFcb);
		return FS_CANTSEEK;
	}

	// ƒtƒ@ƒCƒ‹ƒI[ƒvƒ“/ƒnƒ“ƒhƒ‹Šl“¾
	HANDLE hFile = pHostFcb->Open();
	if (hFile == INVALID_HANDLE_VALUE) {
		m_cFcb.Free(pHostFcb);
		return FS_NOTOPENED;
	}

	// ƒtƒ@ƒCƒ‹ƒV[ƒN
	DWORD nResult = pHostFcb->SetFilePointer(nOffset, nSeek);
	if (nResult == (DWORD)-1) {
		m_cFcb.Free(pHostFcb);
		return FS_CANTSEEK;
	}

	// ƒtƒ@ƒCƒ‹ƒ|ƒCƒ“ƒ^•Û‘¶
	pFcb->fileptr = nResult;

#ifdef XM6_HOST_STRICT_CLOSE
	//‚·‚®•Â‚¶‚é
	pHostFcb->Close();
#endif // XM6_HOST_STRICT_CLOSE

	return nResult;
}

//---------------------------------------------------------------------------
//
//	$4F - ƒtƒ@ƒCƒ‹æ“¾/İ’è
//
//	Œ‹‰Ê‚ÌãˆÊ16Bit‚ª$FFFF‚¾‚ÆƒGƒ‰[B
//
//---------------------------------------------------------------------------
DWORD FASTCALL CWinFileSys::TimeStamp(CWindrv* ps, DWORD nKey, Human68k::fcb_t* pFcb, WORD nFatDate, WORD nFatTime)
{
	ASSERT(this);
	ASSERT(nKey);
	ASSERT(pFcb);

	// ƒƒfƒBƒAƒ`ƒFƒbƒN
	if (m_cEntry.isMediaOffline(ps)) return FS_FATAL_MEDIAOFFLINE;

	// æ“¾‚Ì‚İ
	if (nFatDate == 0 && nFatTime == 0) {
		return ((DWORD)pFcb->date << 16) | pFcb->time;
	}

	// ‘‚«‚İ‹Ö~ƒ`ƒFƒbƒN
	if (m_cEntry.isWriteProtect(ps)) return FS_FATAL_WRITEPROTECT;

	// Šù‚É“¯‚¶ƒL[‚ğ‚Â—Ìˆæ‚ª‚È‚¯‚ê‚ÎƒGƒ‰[‚Æ‚·‚é
	CHostFcb* pHostFcb = m_cFcb.Search(nKey);
	if (pHostFcb == NULL) return FS_NOTOPENED;

	// Human68k‚Å‚ÍA“Ç‚İ‚İƒI[ƒvƒ“‚Å‚àƒ^ƒCƒ€ƒXƒ^ƒ“ƒv‘€ì‚ª‰Â”\‚É‚È‚Á‚Ä‚¢‚é‚½‚ß
	// “Ç‚İ‚İƒIƒ“ƒŠ[ƒ‚[ƒh‚ÅŠJ‚¢‚Ä‚¢‚½‚çAˆê“I‚É‘‚«‚İƒ‚[ƒh‚ÅŠJ‚«‚È‚¨‚·
	BOOL bReopen = FALSE;
	if ((pFcb->mode & 0x0F) == Human68k::OP_READ) {
		bReopen = TRUE;
#ifndef XM6_HOST_STRICT_CLOSE
		pHostFcb->Close();
#endif // XM6_HOST_STRICT_CLOSE
		pHostFcb->SetOpenMode(Human68k::OP_READWRITE);
	}

	// ƒtƒ@ƒCƒ‹ƒI[ƒvƒ“/ƒnƒ“ƒhƒ‹Šl“¾
	HANDLE hFile = pHostFcb->Open();
	if (hFile == INVALID_HANDLE_VALUE) {
		m_cFcb.Free(pHostFcb);
		return FS_NOTOPENED;
	}

	// İ’è
	if (pHostFcb->SetFileTime(nFatDate, nFatTime) == (DWORD)-1) {
		m_cFcb.Free(pHostFcb);
		return FS_CANTWRITE;
	}

	// ˆê“I‚É‘‚«‚İƒ‚[ƒh‚ÅŠJ‚«‚È‚¨‚µ‚½ê‡‚ÍŒ³‚É–ß‚·
	if (bReopen) {
		pHostFcb->SetOpenMode(pFcb->mode);
#ifndef XM6_HOST_STRICT_CLOSE
		pHostFcb->Close();
		hFile = pHostFcb->Open();
		if (hFile == INVALID_HANDLE_VALUE) {
			m_cFcb.Free(pHostFcb);
			return FS_NOTOPENED;
		}
		DWORD nResult = pHostFcb->SetFilePointer(pFcb->fileptr);
		if (nResult == (DWORD)-1) {
			m_cFcb.Free(pHostFcb);
			return FS_CANTSEEK;
		}
#endif // XM6_HOST_STRICT_CLOSE
	}

#ifdef XM6_HOST_STRICT_CLOSE
	// ‚·‚®•Â‚¶‚é
	pHostFcb->Close();
#endif // XM6_HOST_STRICT_CLOSE

	return 0;
}

//---------------------------------------------------------------------------
//
//	$50 - —e—Êæ“¾
//
//---------------------------------------------------------------------------
int FASTCALL CWinFileSys::GetCapacity(CWindrv* ps, Human68k::capacity_t* pCapacity)
{
	ASSERT(this);
	ASSERT(pCapacity);

	// ƒƒfƒBƒAƒ`ƒFƒbƒN
	if (m_cEntry.isMediaOffline(ps)) return FS_FATAL_MEDIAOFFLINE;

	// —e—Êæ“¾
	DWORD nUnit = ps->GetUnit();
	return m_cEntry.GetCapacity(nUnit, pCapacity);
}

//---------------------------------------------------------------------------
//
//	$51 - ƒhƒ‰ƒCƒuó‘ÔŒŸ¸/§Œä
//
//---------------------------------------------------------------------------
int FASTCALL CWinFileSys::CtrlDrive(CWindrv* ps, Human68k::ctrldrive_t* pCtrlDrive)
{
	ASSERT(this);
	ASSERT(pCtrlDrive);

	DWORD nUnit = ps->GetUnit();

	switch (pCtrlDrive->status) {
	case 0:		// ó‘ÔŒŸ¸
	case 9:		// ó‘ÔŒŸ¸2
		pCtrlDrive->status = (BYTE)m_cEntry.GetStatus(nUnit);
		return pCtrlDrive->status;

	case 1:		// ƒCƒWƒFƒNƒg
		m_cEntry.isMediaOffline(ps, FALSE);	// ƒCƒWƒFƒNƒgŒã‚Éƒ`ƒFƒbƒN‚·‚é‚Ì‚Å–‘Oƒ`ƒFƒbƒN•s—v
		m_cEntry.Eject(nUnit);
		return 0;

	case 2:		// ƒCƒWƒFƒNƒg‹Ö~1 (–¢À‘•)
	case 3:		// ƒCƒWƒFƒNƒg‹–‰Â1 (–¢À‘•)
	case 4:		// ƒƒfƒBƒA–¢‘}“ü‚ÉLED“_–Å (–¢À‘•)
	case 5:		// ƒƒfƒBƒA–¢‘}“ü‚ÉLEDÁ“” (–¢À‘•)
	case 6:		// ƒCƒWƒFƒNƒg‹Ö~2 (–¢À‘•)
	case 7:		// ƒCƒWƒFƒNƒg‹–‰Â2 (–¢À‘•)
		return 0;

	case 8:		// ƒCƒWƒFƒNƒgŒŸ¸
		return 1;
	}

	return FS_INVALIDFUNC;
}

//---------------------------------------------------------------------------
//
//	$52 - DPBæ“¾
//
//---------------------------------------------------------------------------
int FASTCALL CWinFileSys::GetDPB(CWindrv* ps, Human68k::dpb_t* pDpb)
{
	ASSERT(this);
	ASSERT(pDpb);

	DWORD nUnit = ps->GetUnit();

	// ƒZƒNƒ^î•ñŠl“¾
	Human68k::capacity_t cap;
	BOOL bResult = m_cEntry.GetCapacityCache(nUnit, &cap);
	if (bResult == FALSE) {
		// ƒƒfƒBƒAƒ`ƒFƒbƒN
		m_cEntry.isMediaOffline(ps);

		// ƒhƒ‰ƒCƒuó‘Ôæ“¾
		m_cEntry.GetCapacity(nUnit, &cap);
	}

	// ƒVƒtƒg”ŒvZ
	DWORD nSize = 1;
	DWORD nShift = 0;
	for (;;) {
		if (nSize >= cap.sectors) break;
		nSize <<= 1;
		nShift++;
	}

	// ƒZƒNƒ^”Ô†ŒvZ
	//
	// ˆÈ‰º‚Ì‡‚É•À‚×‚éB
	//	ƒNƒ‰ƒXƒ^0:–¢g—p
	//	ƒNƒ‰ƒXƒ^1:FAT
	//	ƒNƒ‰ƒXƒ^2:ƒ‹[ƒgƒfƒBƒŒƒNƒgƒŠ
	//	ƒNƒ‰ƒXƒ^3:ƒf[ƒ^—Ìˆæ(‹[—ƒZƒNƒ^)
	DWORD nFat = 1 * cap.sectors;
	DWORD nRoot = 2 * cap.sectors;
	DWORD nData = 3 * cap.sectors;

	// DPBİ’è
	pDpb->sector_size = (WORD)cap.bytes;		// + 0	1ƒZƒNƒ^“–‚è‚ÌƒoƒCƒg”
	pDpb->cluster_size = (BYTE)cap.sectors - 1;	// + 2	1ƒNƒ‰ƒXƒ^“–‚è‚ÌƒZƒNƒ^” - 1
	pDpb->shift = (BYTE)nShift;					// + 3	ƒNƒ‰ƒXƒ^¨ƒZƒNƒ^‚ÌƒVƒtƒg”
	pDpb->fat_sector = (WORD)nFat;				// + 4	FAT ‚Ìæ“ªƒZƒNƒ^”Ô†
	pDpb->fat_max = 1;							// + 6	FAT —Ìˆæ‚ÌŒÂ”
	pDpb->fat_size = (BYTE)cap.sectors;			// + 7	FAT ‚Ìè‚ß‚éƒZƒNƒ^”(•¡Ê•ª‚ğœ‚­)
	pDpb->file_max =							// + 8	ƒ‹[ƒgƒfƒBƒŒƒNƒgƒŠ‚É“ü‚éƒtƒ@ƒCƒ‹‚ÌŒÂ”
		(WORD)(cap.sectors * cap.bytes / 0x20);
	pDpb->data_sector = (WORD)nData;		   	// +10	ƒf[ƒ^—Ìˆæ‚Ìæ“ªƒZƒNƒ^”Ô†
	pDpb->cluster_max =	(WORD)cap.clusters;		// +12	‘ƒNƒ‰ƒXƒ^” + 1
	pDpb->root_sector = (WORD)nRoot;			// +14	ƒ‹[ƒgƒfƒBƒŒƒNƒgƒŠ‚Ìæ“ªƒZƒNƒ^”Ô†
	pDpb->media = 0xF3;							// +20	ƒƒfƒBƒAƒoƒCƒg

	// ƒƒfƒBƒAƒoƒCƒg•ÏX
	if (m_nOption & WINDRV_OPT_MEDIABYTE) {
		pDpb->media = m_cEntry.GetMediaByte(nUnit);
	}

	return 0;
}

//---------------------------------------------------------------------------
//
//	$53 - ƒZƒNƒ^“Ç‚İ‚İ
//
//---------------------------------------------------------------------------
int FASTCALL CWinFileSys::DiskRead(CWindrv* ps, DWORD nAddress, DWORD nSector, DWORD nSize)
{
	ASSERT(this);
	ASSERT(nAddress);

	Memory* pMemory = ps->GetMemory();
	ASSERT(pMemory);

	DWORD nUnit = ps->GetUnit();

	// ƒZƒNƒ^”1ˆÈŠO‚Ìê‡‚ÍƒGƒ‰[
	if (nSize != 1) return FS_NOTIOCTRL;

	// ƒZƒNƒ^î•ñŠl“¾
	Human68k::capacity_t cap;
	BOOL bResult = m_cEntry.GetCapacityCache(nUnit, &cap);
	if (bResult == FALSE) {
		// ƒƒfƒBƒAƒ`ƒFƒbƒN
		m_cEntry.isMediaOffline(ps);

		// ƒhƒ‰ƒCƒuó‘Ôæ“¾
		m_cEntry.GetCapacity(nUnit, &cap);
	}

	// ‹[—ƒfƒBƒŒƒNƒgƒŠƒGƒ“ƒgƒŠ‚Ö‚ÌƒAƒNƒZƒX
	CHostFiles* pHostFiles = m_cFiles.Search(nSector);
	if (pHostFiles) {
		// ‹[—ƒfƒBƒŒƒNƒgƒŠƒGƒ“ƒgƒŠ‚ğ¶¬
		// WARNING: ƒŠƒgƒ‹ƒGƒ“ƒfƒBƒAƒ“ê—p
		Human68k::dirent_t dir;
		memcpy(&dir, pHostFiles->GetEntry(), sizeof(dir));

		// ‹[—ƒfƒBƒŒƒNƒgƒŠƒGƒ“ƒgƒŠ“à‚Éƒtƒ@ƒCƒ‹À‘Ì‚ğw‚·‹[—ƒZƒNƒ^”Ô†‚ğ‹L˜^
		// ‚È‚¨Alzdsys ‚Å‚ÍˆÈ‰º‚Ì®‚Å“Ç‚İ‚İƒZƒNƒ^”Ô†‚ğZo‚µ‚Ä‚¢‚éB
		// (dirent.cluster - 2) * (dpb.cluster_size + 1) + dpb.data_sector
		dir.cluster = (WORD)(m_nHostSectorCount + 2);	// ‹[—ƒZƒNƒ^”Ô†
		m_nHostSectorBuffer[m_nHostSectorCount] = nSector;	// ‹[—ƒZƒNƒ^‚Ìw‚·À‘Ì
		m_nHostSectorCount++;
		m_nHostSectorCount %= XM6_HOST_PSEUDO_CLUSTER_MAX;

		ps->LockXM();

		// ‹[—ƒfƒBƒŒƒNƒgƒŠƒGƒ“ƒgƒŠ‚ğ“]‘—
		BYTE* p = (BYTE*)&dir;
		for (int i = 0; i < 0x20; i++) pMemory->WriteByte(nAddress++, *p++);
		for (int i = 0x20; i < 0x200; i++) pMemory->WriteByte(nAddress++, 0xFF);

		ps->UnlockXM();

		return 0;
	}

	// ƒNƒ‰ƒXƒ^”Ô†‚©‚çƒZƒNƒ^”Ô†‚ğZo
	DWORD n = nSector - (3 * cap.sectors);
	DWORD nMod = 1;
	if (cap.sectors) {
		// ƒƒfƒBƒA‚ª‘¶İ‚µ‚È‚¢ê‡‚Í cap.sectors ‚ª0‚É‚È‚é‚Ì‚Å’ˆÓ
		nMod = n % cap.sectors;
		n /= cap.sectors;
	}

	// ƒtƒ@ƒCƒ‹À‘Ì‚Ö‚ÌƒAƒNƒZƒX
	if (nMod == 0 && n < XM6_HOST_PSEUDO_CLUSTER_MAX) {
		pHostFiles = m_cFiles.Search(m_nHostSectorBuffer[n]);	// À‘Ì‚ğŒŸõ
		if (pHostFiles) {
			// ƒƒfƒBƒAƒ`ƒFƒbƒN
			if (m_cEntry.isMediaOffline(ps)) return FS_FATAL_MEDIAOFFLINE;

			// ‹[—ƒZƒNƒ^‚ğ¶¬
			CHostFcb f;
			f.SetFilename(pHostFiles->GetPath());
			f.SetOpenMode(Human68k::OP_READ);
			HANDLE hFile = f.Open();
			if (hFile == INVALID_HANDLE_VALUE) return FS_NOTIOCTRL;
			BYTE chBuffer[512];
			memset(chBuffer, 0, sizeof(chBuffer));
			DWORD nResult = f.ReadFile(chBuffer, 512);
			f.Close();
			if (nResult == (DWORD)-1) return FS_NOTIOCTRL;

			ps->LockXM();

			// ‹[—ƒZƒNƒ^‚ğ“]‘—
			for (int i = 0; i < 0x200; i++) pMemory->WriteByte(nAddress++, chBuffer[i]);

			ps->UnlockXM();

			return 0;
		}
	}

	return FS_NOTIOCTRL;
}

//---------------------------------------------------------------------------
//
//	$54 - ƒZƒNƒ^‘‚«‚İ
//
//---------------------------------------------------------------------------
int FASTCALL CWinFileSys::DiskWrite(CWindrv* ps, DWORD nAddress, DWORD nSector, DWORD nSize)
{
	ASSERT(this);
	ASSERT(nAddress);

	// ƒƒfƒBƒAƒ`ƒFƒbƒN
	if (m_cEntry.isMediaOffline(ps)) return FS_FATAL_MEDIAOFFLINE;

	// ‘‚«‚İ‹Ö~ƒ`ƒFƒbƒN
	if (m_cEntry.isWriteProtect(ps)) return FS_FATAL_WRITEPROTECT;

	// Œ»À‚ğ“Ë‚«‚Â‚¯‚é
	return FS_NOTIOCTRL;
}

//---------------------------------------------------------------------------
//
//	$55 - IOCTRL
//
//---------------------------------------------------------------------------
int FASTCALL CWinFileSys::IoControl(CWindrv* ps, Human68k::ioctrl_t* pIoctrl, DWORD nFunction)
{
	ASSERT(this);

	switch (nFunction) {
	case 0:
		// ƒƒfƒBƒAID‚ÌŠl“¾
		pIoctrl->media = 0xF3;
		if (m_nOption & WINDRV_OPT_MEDIABYTE) {
			DWORD nUnit = ps->GetUnit();
			pIoctrl->media = m_cEntry.GetMediaByte(nUnit);
		}
		return 0;

	case 1:
		// Human68kŒİŠ·‚Ì‚½‚ß‚Ìƒ_ƒ~[
		pIoctrl->param = -1;
		return 0;

	case 2:
		switch (pIoctrl->param) {
		case -1:
			// ƒƒfƒBƒAÄ”F¯
			m_cEntry.isMediaOffline(ps);
			return 0;

		case 0:
		case 1:
			// Human68kŒİŠ·‚Ì‚½‚ß‚Ìƒ_ƒ~[
			return 0;
		}
		break;

	case -1:
		// í’“”»’è
		memcpy(pIoctrl->buffer, "WindrvXM", 8);
		return 0;

	case -2:
		// ƒIƒvƒVƒ‡ƒ“İ’è
		SetOption(pIoctrl->param);
		return 0;

	case -3:
		// ƒIƒvƒVƒ‡ƒ“Šl“¾
		pIoctrl->param = GetOption();
		return 0;
	}

	return FS_NOTIOCTRL;
}

//---------------------------------------------------------------------------
//
//	$56 - ƒtƒ‰ƒbƒVƒ…
//
//---------------------------------------------------------------------------
int FASTCALL CWinFileSys::Flush(CWindrv* ps)
{
	ASSERT(this);

	DWORD nUnit = ps->GetUnit();

	// ƒtƒ‰ƒbƒVƒ…
	m_cEntry.LockCache();
	m_cEntry.EraseCache(nUnit);
	m_cEntry.UnlockCache();

	// í‚É¬Œ÷
	return 0;
}

//---------------------------------------------------------------------------
//
//	$57 - ƒƒfƒBƒAŒğŠ·ƒ`ƒFƒbƒN
//
//---------------------------------------------------------------------------
int FASTCALL CWinFileSys::CheckMedia(CWindrv* ps)
{
	ASSERT(this);

	// TwentyOneƒIƒvƒVƒ‡ƒ“ŠÄ‹
	CheckKernel(ps);

#ifndef XM6_HOST_STRICT_TIMEOUT
	// ƒ^ƒCƒ€ƒAƒEƒgƒ`ƒFƒbƒN
	m_cEntry.CheckTimeout();
#endif // XM6_HOST_STRICT_TIMEOUT

	// è“®ƒCƒWƒFƒNƒgƒƒfƒBƒA‚Ìó‘ÔXVƒ`ƒFƒbƒN
	BOOL bResult = m_cEntry.CheckMediaUpdate(ps);

	// ƒƒfƒBƒA–¢‘}“ü‚È‚çƒGƒ‰[‚Æ‚·‚é
	if (bResult == FALSE) {
#ifdef XM6_HOST_UPDATE_BY_SEQUENCE
		// ’¼Œã‚ÌƒfƒBƒŒƒNƒgƒŠƒ`ƒFƒbƒNƒRƒ}ƒ“ƒh‚ğ–³Œø‚É‚·‚é‚½‚ßƒtƒ‰ƒO‚ğ—§‚Ä‚é
		m_cEntry.SetMediaUpdate(ps, TRUE);
#endif // XM6_HOST_UPDATE_BY_SEQUENCE

		return FS_INVALIDFUNC;
	}

	return 0;
}

//---------------------------------------------------------------------------
//
//	$58 - ”r‘¼§Œä
//
//---------------------------------------------------------------------------
int FASTCALL CWinFileSys::Lock(CWindrv* ps)
{
	ASSERT(this);

	// ƒƒfƒBƒAƒ`ƒFƒbƒN
	if (m_cEntry.isMediaOffline(ps)) return FS_FATAL_MEDIAOFFLINE;

	// í‚É¬Œ÷
	return 0;
}

#if 0
//---------------------------------------------------------------------------
//
//	Win32ÅIƒGƒ‰[æ“¾ Human68kƒGƒ‰[‚É•ÏŠ·
//
//---------------------------------------------------------------------------
DWORD FASTCALL CWinFileSys::GetLastError(DWORD nUnit) const
{
	ASSERT(this);
	ASSERT(m_cEntry.GetBase(nUnit));

	return FS_INVALIDFUNC;
}
#endif

//---------------------------------------------------------------------------
//
//	ƒIƒvƒVƒ‡ƒ“İ’è
//
//---------------------------------------------------------------------------
void FASTCALL CWinFileSys::SetOption(DWORD nOption)
{
	ASSERT(this);

	// ƒfƒBƒŒƒNƒgƒŠƒGƒ“ƒgƒŠ‚ÌÄ\¬‚ª•K—v‚È‚çƒLƒƒƒbƒVƒ…ƒNƒŠƒA
	DWORD nDiff = m_nOption ^ nOption;
	if (nDiff & 0x7F3F1F3F) {
		m_cEntry.LockCache();
		m_cEntry.CleanCache();
		m_cEntry.UnlockCache();
	}

	m_nOption = nOption;
	CHostFilename::SetOption(nOption);
	CHostPath::SetOption(nOption);
}

//---------------------------------------------------------------------------
//
//	ƒIƒvƒVƒ‡ƒ“‰Šú‰»
//
//---------------------------------------------------------------------------
void FASTCALL CWinFileSys::InitOption(const BYTE* pOption)
{
	pOption += strlen((char*)pOption) + 1;

	DWORD nOption = m_nOptionDefault;
	for (;;) {
		const BYTE* p = pOption;
		BYTE c = *p++;
		if (c == '\0') break;

		DWORD nMode;
		if (c == '+') {
			nMode = 1;
		}
		else if (c == '-') {
			nMode = 0;
		} else {
			// ƒIƒvƒVƒ‡ƒ“w’è‚Å‚Í‚È‚¢‚Ì‚ÅŸ‚Ö
			pOption += strlen((char*)pOption) + 1;
			continue;
		}

		for (;;) {
			c = *p++;
			if (c == '\0') break;

			DWORD nBit = 0;
			switch (c) {
			case 'D': case 'd': nBit = WINDRV_OPT_REMOVE; break;
			case 'K': case 'k': nBit = WINDRV_OPT_TWENTYONE; break;
			case 'M': case 'm': nBit = WINDRV_OPT_MEDIABYTE; break;
			case 'A': case 'a': nBit = WINDRV_OPT_CONVERT_LENGTH; break;
			case 'T': case 't': nBit = WINDRV_OPT_COMPARE_LENGTH; nMode ^= 1; break;
			case 'C': case 'c': nBit = WINDRV_OPT_ALPHABET; break;

			case 'E': nBit = WINDRV_OPT_CONVERT_PERIOD; break;
			case 'P': nBit = WINDRV_OPT_CONVERT_PERIODS; break;
			case 'N': nBit = WINDRV_OPT_CONVERT_HYPHEN; break;
			case 'H': nBit = WINDRV_OPT_CONVERT_HYPHENS; break;
			case 'X': nBit = WINDRV_OPT_CONVERT_BADCHAR; break;
			case 'S': nBit = WINDRV_OPT_CONVERT_SPACE; break;

			case 'e': nBit = WINDRV_OPT_REDUCED_PERIOD; break;
			case 'p': nBit = WINDRV_OPT_REDUCED_PERIODS; break;
			case 'n': nBit = WINDRV_OPT_REDUCED_HYPHEN; break;
			case 'h': nBit = WINDRV_OPT_REDUCED_HYPHENS; break;
			case 'x': nBit = WINDRV_OPT_REDUCED_BADCHAR; break;
			case 's': nBit = WINDRV_OPT_REDUCED_SPACE; break;
			}

			if (nMode) {
				nOption |= nBit;
			} else {
				nOption &= ~nBit;
			}
		}

		pOption = p;
	}

	// ƒIƒvƒVƒ‡ƒ“İ’è
	if (nOption != m_nOption) {
		SetOption(nOption);
	}
}

//---------------------------------------------------------------------------
//
//	ƒ{ƒŠƒ…[ƒ€ƒ‰ƒxƒ‹æ“¾
//
//---------------------------------------------------------------------------
BOOL FASTCALL CWinFileSys::FilesVolume(CWindrv* ps, Human68k::files_t* pFiles)
{
	ASSERT(this);
	ASSERT(pFiles);

	DWORD nUnit = ps->GetUnit();

	// ƒ{ƒŠƒ…[ƒ€ƒ‰ƒxƒ‹æ“¾
	TCHAR szVolume[32];
	BOOL bResult = m_cEntry.GetVolumeCache(nUnit, szVolume);
	if (bResult == FALSE) {
		// ƒƒfƒBƒAƒ`ƒFƒbƒN
		m_cEntry.isMediaOffline(ps);

		// ƒ{ƒŠƒ…[ƒ€ƒ‰ƒxƒ‹æ“¾
		m_cEntry.GetVolume(nUnit, szVolume);
	}
	if (szVolume[0] == _T('\0')) return FALSE;

	pFiles->attr = Human68k::AT_VOLUME;
	pFiles->time = 0;
	pFiles->date = 0;
	pFiles->size = 0;

	CHostFilename fname;
	fname.SetWin32(szVolume);
	fname.SetHuman();
	strcpy((char*)pFiles->full, (char*)fname.GetHuman());

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	TwentyOneƒIƒvƒVƒ‡ƒ“ŠÄ‹
//
//	ƒJ[ƒlƒ‹•s”õ‚Ì‚½‚ßA‚â‚Ş‚ğ“¾‚¸ƒŠƒ‚[ƒgƒhƒ‰ƒCƒu‘¤‚Å‘Îˆ‚µ‚È‚¯‚ê‚Î
//	‚È‚ç‚È‚¢ƒpƒ‰ƒ[ƒ^‚ğ©“®”½‰f‚·‚éB
//
//	TODO: ‚¢‚Â‚©ƒJ[ƒlƒ‹Eƒhƒ‰ƒCƒo‚ğC³‚µ‚Ä‚±‚Ìˆ—‚»‚Ì‚à‚Ì‚ğ‚È‚­‚µ‚½‚¢
//
//---------------------------------------------------------------------------
void FASTCALL CWinFileSys::CheckKernel(CWindrv* ps)
{
	ASSERT(this);

	if ((m_nOption & WINDRV_OPT_TWENTYONE) == 0) return;

	ps->LockXM();

	Memory* pMemory = ps->GetMemory();
	ASSERT(pMemory);

	// ‚Ü‚¾ŒŸõŠ®—¹‚µ‚Ä‚¢‚È‚¯‚ê‚ÎHuman68kƒJ[ƒlƒ‹“àƒ[ƒN‚ğ’²¸‚·‚é
	if (m_nKernel < 1024) {
		// Ä’§í‚Ö‚ÌƒJƒEƒ“ƒgƒ_ƒEƒ“
		if (m_nKernel > 0) {
			m_nKernel--;		// ‚¢‚¸‚êÄ’§í
			goto CheckKernelExit;
		}

		// Step1: NULƒfƒoƒCƒXŒŸõ
		if (m_nKernelSearch == 0) {
			DWORD n = 0x6800;
			for (;;) {
				DWORD nData = pMemory->ReadWord(n);
				if (nData == 'NU') {
					if (pMemory->ReadWord(n + 2) == 'L ') break;
				}
				n += 2;
				if (n >= 0x20000 - 2) {
					// NULƒfƒoƒCƒX”­Œ©‚Å‚«‚¸
					m_nKernel = 0xFFFFFFFF;		// ƒJ[ƒlƒ‹ˆÙí: “ñ“x‚ÆŒŸõ‚µ‚È‚¢
					goto CheckKernelExit;
				}
			}
			n -= 14;
			m_nKernelSearch = n;
		}

		// Step2: NULƒfƒoƒCƒX‚ğ‹N“_‚Æ‚µ‚Ä‘SƒfƒoƒCƒXŒŸõ
		DWORD n = m_nKernelSearch;
		for (;;) {
			// Ÿ‚ÌƒfƒoƒCƒX‚Ö
			n = (pMemory->ReadWord(n) << 16) | pMemory->ReadWord(n + 2);
			if (n == 0xFFFFFFFF) {
				// ŠY“–ƒfƒoƒCƒX‚È‚µ
				m_nKernel = XM6_HOST_TWENTYONE_CHECK_COUNT;	// ŒŸõ¸”s: ‚¢‚¸‚êÄ’§í
				goto CheckKernelExit;
			}

			DWORD x1 = (pMemory->ReadWord(n + 14) << 16) | pMemory->ReadWord(n + 16);

			if (x1 == '*Twe') {
				DWORD x2 = (pMemory->ReadWord(n + 18) << 16) | pMemory->ReadWord(n + 20);
				if (x2 == 'nty*') {
					// ‹Œƒo[ƒWƒ‡ƒ“‚ğ”­Œ©
					m_nKernel = 0xFFFFFFFF;		// TwentyOne‹Œƒo[ƒWƒ‡ƒ“: “ñ“x‚ÆŒŸõ‚µ‚È‚¢
					goto CheckKernelExit;
				}
				continue;
			}

			if (x1 == '?Twe') {
				DWORD x2 = (pMemory->ReadWord(n + 18) << 16) | pMemory->ReadWord(n + 20);
				if (x2 == 'nty?' || x2 == 'ntyE') {
					break;
				}
				continue;
			}
		}

		// ”­Œ©
		m_nKernel = n + 22;
	} else {
		if (m_nKernel == 0xFFFFFFFF) {
			goto CheckKernelExit;
		}
	}

	{
		// ƒJ[ƒlƒ‹‘¤ƒIƒvƒVƒ‡ƒ“Šl“¾
		DWORD nKernelOption =
			(pMemory->ReadWord(m_nKernel) << 16) | pMemory->ReadWord(m_nKernel + 2);

		// ƒŠƒ‚[ƒgƒhƒ‰ƒCƒu‘¤ƒIƒvƒVƒ‡ƒ“Šl“¾
		DWORD nOption = m_nOption &
			~(WINDRV_OPT_ALPHABET | WINDRV_OPT_COMPARE_LENGTH | WINDRV_OPT_CONVERT_LENGTH);

		// ƒIƒvƒVƒ‡ƒ“”½‰f
		if (nKernelOption & 0x40000000) {	// _TWON_C_BIT: Bit30
			nOption |= WINDRV_OPT_ALPHABET;
		}
		if (nKernelOption & 0x08000000) {	// _TWON_T_BIT: Bit27
			nOption |= WINDRV_OPT_COMPARE_LENGTH;
		}
		if (nKernelOption & 0x00400000) {	// _TWON_A_BIT: Bit22
			nOption |= WINDRV_OPT_CONVERT_LENGTH;
		}

		// ƒIƒvƒVƒ‡ƒ“İ’è
		if (nOption != m_nOption) {
			SetOption(nOption);
		}
	}

CheckKernelExit:
	ps->UnlockXM();
}

//===========================================================================
//
//	Host
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	ƒRƒ“ƒXƒgƒ‰ƒNƒ^
//
//---------------------------------------------------------------------------
CHost::CHost(CFrmWnd *pWnd) : CComponent(pWnd)
{
	// ƒRƒ“ƒ|[ƒlƒ“ƒgƒpƒ‰ƒ[ƒ^
	m_dwID = MAKEID('H', 'O', 'S', 'T');
	m_strDesc = _T("Host FileSystem");

	// ƒIƒuƒWƒFƒNƒg
	m_pWindrv = NULL;
}

//---------------------------------------------------------------------------
//
//	‰Šú‰»
//
//---------------------------------------------------------------------------
BOOL FASTCALL CHost::Init()
{
	ASSERT(this);

	// Šî–{ƒNƒ‰ƒX
	if (!CComponent::Init()) {
		return FALSE;
	}

	// Windrvæ“¾
	ASSERT(!m_pWindrv);
	m_pWindrv = (Windrv*)::GetVM()->SearchDevice(MAKEID('W', 'D', 'R', 'V'));
	ASSERT(m_pWindrv);

	// ƒtƒ@ƒCƒ‹ƒVƒXƒeƒ€İ’è
	m_pWindrv->SetFileSys(&m_WinFileSys);

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	ƒNƒŠ[ƒ“ƒAƒbƒv
//
//---------------------------------------------------------------------------
void FASTCALL CHost::Cleanup()
{
	ASSERT(this);

	// ƒtƒ@ƒCƒ‹ƒVƒXƒeƒ€‚ğØ‚è—£‚·
	if (m_pWindrv) {
		m_pWindrv->SetFileSys(NULL);
	}

	// ƒŠƒZƒbƒg(‘SƒNƒ[ƒY)
	m_WinFileSys.Reset();

	// Šî–{ƒNƒ‰ƒX
	CComponent::Cleanup();
}

//---------------------------------------------------------------------------
//
//	İ’è“K—p
//
//---------------------------------------------------------------------------
void FASTCALL CHost::ApplyCfg(const Config* pConfig)
{
	ASSERT(this);
	ASSERT(pConfig);

	// ƒtƒ@ƒCƒ‹ƒVƒXƒeƒ€‚ğŒÄ‚Ño‚·
	m_WinFileSys.ApplyCfg(pConfig);
}

#endif // _WIN32
