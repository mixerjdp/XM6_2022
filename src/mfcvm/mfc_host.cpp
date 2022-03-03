//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2004 �o�h�D(ytanaka@ipc-tokai.or.jp)
//	Modified (C) 2006 co (cogood��gmail.com)
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
//	Windows�t�@�C���h���C�u
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	�R���X�g���N�^
//
//---------------------------------------------------------------------------
CWinFileDrv::CWinFileDrv()
{
	// ������
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
//	�f�X�g���N�^
//
//---------------------------------------------------------------------------
CWinFileDrv::~CWinFileDrv()
{
#ifdef XM6_HOST_KEEP_OPEN_ERROR
	// �f�o�C�X�N���[�Y
	CloseDevice();
#endif // XM6_HOST_KEEP_OPEN_ERROR
}

//---------------------------------------------------------------------------
//
//	�f�o�C�X�I�[�v��
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
//	�f�o�C�X�N���[�Y
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
//	������
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

	// �p�����[�^���󂯎��
	if (nFlag & FSFLAG_WRITE_PROTECT) m_bWriteProtect = TRUE;
	if (nFlag & FSFLAG_SLOW) m_bSlow = TRUE;
	if (nFlag & FSFLAG_REMOVABLE) m_bRemovable = TRUE;
	if (nFlag & FSFLAG_MANUAL) m_bManual = TRUE;
	_tcscpy(m_szBase, szBase);

	// �x�[�X�p�X�̍Ō�̃p�X��؂�}�[�N���폜����
	// WARNING: Unicode���p���͏C�����K�v
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
		if ((0x80 <= c && c <= 0x9F) || 0xE0 <= c) {	// �����ɂ� 0x81�`0x9F 0xE0�`0xEF
			p++;
			if (*p == _T('\0')) break;
		}
		p++;
	}
	if (pClear) *pClear = _T('\0');

	// �h���C�u���l��
	DWORD n = m_szBase[0] & 0xDF;
	if (n < 'A' || 'Z' < n || m_szBase[1] != ':') {
		// �h���C�u�����s���Ȃ�l�b�g���[�N�h���C�u��UNC�Ƃ��Ĉ���
		m_bSlow = TRUE;
		CheckMedia();
		return;
	}

	// �h���C�u������
	_stprintf(m_szDrive, _T("%c:\\"), n);

	// �h���C�u��ނ̔���
	DWORD nResult = ::GetDriveType(m_szDrive);
	switch (nResult) {
	case DRIVE_REMOVABLE:
	{
		m_bSlow = TRUE;
		m_bRemovable = TRUE;

		// �t���b�s�[�f�B�X�N����
		SHFILEINFO shfi;
		nResult = ::SHGetFileInfo(m_szDrive, 0, &shfi, sizeof(shfi), SHGFI_TYPENAME);
		if (nResult) {
			if (_tcsstr(shfi.szTypeName, _T("�t���b�s�[")) != NULL ||
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

	// �f�o�C�X�I�[�v���pUNC����
	OSVERSIONINFO v;
	memset(&v, 0, sizeof(v));
	v.dwOSVersionInfoSize = sizeof(v);
	::GetVersionEx(&v);
	if (v.dwPlatformId == VER_PLATFORM_WIN32_NT) {
		m_bDevice = TRUE;
		_stprintf(m_szDevice, _T("\\\\.\\%c:"), n);
	}

#ifdef XM6_HOST_KEEP_OPEN_ERROR
	// ����I�[�v��/�N���[�Y����ƒx���̂ŁA�I�[�v�������܂܎g������
	// �������g�����̂ɂȂ�Ȃ������B
	// �����ŃI�[�v�������ꍇ�A�ŏ�����h���C�u�Ƀ��f�B�A�������Ă�
	// ��Ή��x�}���E�r�o�����Ă����Ȃ��������A�ŏ��h���C�u�Ƀ��f�B
	// �A�������Ă��Ȃ������ꍇ�A���̃n���h�������܂�Windows����
	// �f�B�A�}����F�����Ȃ��Ȃ��Ă��܂����B

	// �f�o�C�X�I�[�v��
	OpenDevice();
#endif // XM6_HOST_KEEP_OPEN_ERROR

	// ���f�B�A�L���`�F�b�N
	if (m_bManual == FALSE) CheckMedia();
}

//---------------------------------------------------------------------------
//
//	�h���C�u��Ԃ̎擾
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
//	���f�B�A��Ԑݒ�
//
//---------------------------------------------------------------------------
void FASTCALL CWinFileDrv::SetEnable(BOOL bEnable)
{
	ASSERT(this);

	m_bEnable = bEnable;

	// ���f�B�A�������Ă��Ȃ���΃L���b�V������
	if (bEnable == FALSE) {
		memset(&m_capCache, 0, sizeof(m_capCache));
		m_bVolumeCache = FALSE;
		m_szVolumeCache[0] = _T('\0');
	}
}

//---------------------------------------------------------------------------
//
//	�����[�o�u�����f�B�A�̃t�@�C���L���b�V���L�����Ԃ��X�V
//
//---------------------------------------------------------------------------
void FASTCALL CWinFileDrv::SetTimeout()
{
	ASSERT(this);

	// �����[�o�u�����f�B�A�łȂ���Α��I��
	if (m_bRemovable == FALSE) return;

	// �L�����Ԃ��X�V
	m_bUpdateFile = TRUE;
	m_nUpdateFile = ::GetTickCount();
}

//---------------------------------------------------------------------------
//
//	�����[�o�u�����f�B�A�̃t�@�C���L���b�V���L�����Ԃ��m�F
//
//---------------------------------------------------------------------------
BOOL FASTCALL CWinFileDrv::CheckTimeout()
{
	ASSERT(this);

	// �����[�o�u�����f�B�A�łȂ���Α��I��
	if (m_bRemovable == FALSE) return FALSE;

	// �^�C���A�E�g���m�F
	DWORD nCount = ::GetTickCount();
	DWORD nDelta = nCount - m_nUpdateFile;
	if (nDelta < XM6_HOST_REMOVABLE_CACHE_TIME) return FALSE;	// �X�V�s�v

	if (m_bUpdateFile == FALSE) return FALSE;	// ���ɍX�V�ς�
	m_bUpdateFile = FALSE;

	return TRUE;	// �X�V�Z��
}

#ifdef XM6_HOST_UPDATE_BY_SEQUENCE
//---------------------------------------------------------------------------
//
//	�����[�o�u�����f�B�A�̏�ԍX�V��L���ɂ���
//
//	�R�}���h$41(�f�B���N�g���`�F�b�N)�̒���̃R�}���h$57(���f�B�A��
//	���`�F�b�N)�Ń��f�B�A�}���`�F�b�N�����s���邽�߁A�X�e�[�g�ϐ���
//	�Ǘ����s�Ȃ��B
//
//	��������O�Ƃ��āA�R�}���h$57(���f�B�A�����`�F�b�N)�̌��ʂ��G���[
//	�̏ꍇ�́A���̒���ɕK��Human68k���R�}���h$41(�f�B���N�g���`�F�b
//	�N)���Ăт������߁A���̒���̃��f�B�A�����`�F�b�N�ł͉������s��
//	��Ȃ��悤�ɂ���B
//
//	��₱�������Ǐ����̗���͈ȉ��̂悤�Ȋ����B
//	�R�}���h$57 - ���f�B�A�����`�F�b�N ���f�B�A�Ȃ� �� SetMediaUpdate(TRUE) S2
//	��������$41 - �f�B���N�g���`�F�b�N S2 SetMediaUpdate(FALSE) Disable��ԉ��� S0
//	�R�}���h$57 - ���f�B�A�����`�F�b�N S0 �������Ȃ� ���f�B�A�Ȃ� �� SetMediaUpdate(TRUE) S2
//	��������$41 - �f�B���N�g���`�F�b�N S2 SetMediaUpdate(FALSE) Disable��ԉ��� S0
//	�R�}���h$41 - �f�B���N�g���`�F�b�N S0 SetMediaUpdate(FALSE) Enable��Ԑݒ� S1
//	�R�}���h$57 - ���f�B�A�����`�F�b�N S1 �����`�F�b�N���s ���f�B�A�Ȃ� �� SetMediaUpdate(TRUE) S2
//	��������$41 - �f�B���N�g���`�F�b�N S2 SetMediaUpdate(FALSE) Disable��ԉ��� S0
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
//	�����[�o�u�����f�B�A�̏�ԍX�V�`�F�b�N
//
//	�蓮�C�W�F�N�g�̃��f�B�A�ɂ����āA�Z���ԓ��ɘA������CheckMedia��
//	�Ăяo���ꂽ�ꍇ�A�A�N�Z�X���O�`�F�b�N�����Ɉڍs����B
//
//	mint �Ȃǂ̃��f�B�A�Ď��p�^�[���͈ȉ��̒ʂ�B
//	5000ms�ȓ���CheckMedia��1�`2��Ă΂��: �����[�o�u�����f�B�A�Ď���
//	100ms�ȓ���CheckMedia��12��ȏ�Ă΂��: �����[�o�u�����f�B�A���J����
//
//	�T�v: ���݂�IBM-PC/AT�݊��@�ł́AFDD���̃��f�B�A�̑��݂��AFDD��
//	�̃f�[�^�ɒ��ڃA�N�Z�X���邱�Ƃł����ϑ��ł��Ȃ��B���̊ϑ��s�ׂ�
//	��3�̖�肪���݂���B
//
//	1. ��ʓI��AT�݊��@�ł́AFDD�̃��[�^�[����~���Ă����Ԃ���̃h
//	���C�u�ւ̃A�N�Z�X�����܂ŁA�ǂ��API�𗘗p���Ă�1000ms�ȏォ��
//	�Ă��܂��BX680x0�ł́Amint���̑�\�I�ȃt�@�C���[�ɂ����āA�h���C
//	�u�ύX�̓x�ɑS�h���C�u�̃��f�B�A�}����Ԃ�p�ɂɊm�F���Ă��邽�߁A
//	����ϑ����Ă��܂��ƁA���쑬�x���ɒ[�ɒቺ���A���[�U�̎g�p����
//	��������������B
//
//	2. FDD�̃��[�^�[����]���Ă����ꍇ�ł��A�ϑ������܂ł͐�10�`
//	100ms���x�̏������Ԃ�v����BX680x0�ł́A�ʏ��Human68k�̗��p��
//	�����Ă��A���f�B�A��Ԃ̃`�F�b�N���p�ɂɍs�Ȃ��邽�߁AVM������
//	�̎w���̓x�Ɋϑ�����Ɠ��쑬�x���ቺ���Ă��܂��B
//
//	3. FDD�Ƀ��f�B�A�����݂��Ȃ��ꍇ�Ɋϑ����s�Ȃ���ƁAFDD�����
//	�����������Ă��܂��B����̓n�[�h�E�F�A�̐���ł���\�t�g�E�F�A��
//	����͉����ł��Ȃ��B����AX680x0�ł́A�|�[�����O�ɂ�胁�f�B�A��
//	�݊m�F���s�Ȃ��̂���ʓI�ł���BVM������̎w���ʂ�Ɋϑ��s�ׂ��s
//	�Ȃ����ꍇ�A���Ԋu��FDD���ى���t�ł�ň��̎��Ԃ��������A�^��
//	���@�̈������ė�����B
//
//	�����ŁA�蓮�C�W�F�N�g�^�̃����[�o�u�����f�B�A�ɂ��ẮA�ȉ���
//	�����Ń��f�B�A�̊ϑ����s�Ȃ��d�l�Ƃ����B
//
//	1. ���ۂɃz�X�g��OS�ɂ��A�N�Z�X���s�Ȃ��钼�O�Ɋϑ�����B
//	   CHostEntry::CheckMediaAccess()�̃p�^�[��1,2�B
//	2. �Z���ԂɏW�����ă��f�B�A�ϑ��v�������������ꍇ�Ɋϑ�����B
//	   ���f�B�A�}�������o�ł��Ȃ�USB�f�o�C�X������̂ŁA���̔����
//	   �����[�o�u�����f�B�A�S�Ăōs�Ȃ��悤�ύX�B
//     ��mint2.xx�n�ɑΉ������邽�߁A��������Ԃł͂Ȃ��f�B���N�g��
//     �`�F�b�N����̃��f�B�A�ϑ��v���̃^�C�~���O�ōs�Ȃ��悤�ύX�B
//	   CHostEntry::CheckMediaAccess()�̃p�^�[��3�B
//	3. ��L2�����ł̓��f�B�A�����ꊷ��������Ƃ����o�ł��Ȃ��\��
//	   �����邽�߁A�L���b�V���̗L�����Ԃ𐔕b�Ԃɐ������đΏ�����B
//	   �L���b�V���ŗ��p���Ă���FindFirstChangeNotification()���h���C
//	   �u�����b�N���ăC�W�F�N�g�s�ɂȂ邽�߁A�Z���ԂŊJ�����邱�ƁB
//	   CHostEntry::isMediaOffline()��CHostEntry::CheckTimeout()�ŏ����B
//	4. �A�����Ċϑ����s�Ȃ�Ȃ��悤�A��x�A�N�Z�X�����琔�b�Ԃ̃K�[
//	   �h���Ԃ�݂��AVM���ւ̓L���b�V������n���B
//
//	��L�d�l�̂��߁A�蓮�C�W�F�N�g�^�̃����[�o�u�����f�B�A�ɂ��ẮA
//	���f�B�A�����݂��Ȃ��ꍇ�̃A�N�Z�X�͐���n�Ƃ݂Ȃ��A���уG���[��
//	�o���Ȃ����Ƃɂ���B
//
//---------------------------------------------------------------------------
BOOL FASTCALL CWinFileDrv::CheckMediaUpdate()
{
	ASSERT(this);

	// �����[�o�u�����f�B�A�łȂ���Α��I��
	if (m_bRemovable == FALSE) return FALSE;

#ifdef XM6_HOST_UPDATE_BY_SEQUENCE
	// �X�V�t���O�𔻒�
	if (m_nUpdateMedia == 1) {
		m_nUpdateMedia = 0;
		return TRUE;	// �X�V�Z��
	}
	m_nUpdateMedia = 0;	// �ŏ���1��ڂ����X�V����΂����̂Ńt���O���~�낷
#endif // XM6_HOST_UPDATE_BY_SEQUENCE

#ifdef XM6_HOST_UPDATE_BY_FREQUENCY
	// �`�F�b�N�������L�^
	DWORD n = ::GetTickCount();
	DWORD nOld = m_nUpdateBuffer[m_nUpdateCount];
	m_nUpdateBuffer[m_nUpdateCount] = n;
	m_nUpdateCount++;
	m_nUpdateCount %= XM6_HOST_REMOVABLE_RELOAD_COUNT;

	// ���Ԃ�����̌Ăяo���񐔂����Ȃ���΃L���b�V�����g�p����
	if (n - nOld <= XM6_HOST_REMOVABLE_RELOAD_TIME) return TRUE;	// �X�V�Z��
#endif // XM6_HOST_UPDATE_BY_FREQUENCY

#ifdef XM6_HOST_UPDATE_ALWAYS
	return TRUE;	// �X�V�Z��
#endif //XM6_HOST_UPDATE_ALWAYS

	return FALSE;	// �X�V�s�v
}

//---------------------------------------------------------------------------
//
//	�����[�o�u�����f�B�A�̃A�N�Z�X���O�`�F�b�N
//
//	�蓮�C�W�F�N�g�̃��f�B�A�́A�A�N�Z�X����钼�O�ɏ�Ԃ��m�F����B
//	�A�����ČĂяo���ꂽ�ꍇ�̓L���b�V�����g�p����B
//
//---------------------------------------------------------------------------
BOOL FASTCALL CWinFileDrv::CheckMediaAccess(BOOL bManual)
{
	ASSERT(this);

	if (bManual) {
		// �蓮�C�W�F�N�g�łȂ���Α��I��
		if (m_bManual == FALSE) return FALSE;
	} else {
		// �����[�o�u�����f�B�A�łȂ���Α��I��
		if (m_bRemovable == FALSE) return FALSE;
	}

	// �A�����ČĂяo���ꂽ�ꍇ�̓L���b�V�����g�p����
	DWORD nCount = ::GetTickCount();
	DWORD nDelta = nCount - m_nUpdate;
	if (nDelta < XM6_HOST_REMOVABLE_GUARD_TIME) return FALSE;	// �X�V�s�v
	m_nUpdate = nCount;

	return TRUE;	// �X�V�Z��
}

//---------------------------------------------------------------------------
//
//	���f�B�A�L���`�F�b�N
//
//	�����[�o�u�����f�B�A�}������Ɏ��s����ƁAOpen��������
//	DeviceIoControl�����Œ�~���ď\���b�ԋA���Ă��Ȃ����Ƃ�����̂�
//	�Ăяo���^�C�~���O�ɒ��ӂ��邱�ƁB
//
//---------------------------------------------------------------------------
BOOL FASTCALL CWinFileDrv::CheckMedia()
{
	ASSERT(this);

	// ���f�B�A�}���Ƃ݂Ȃ�
	BOOL bEnable = TRUE;

	if (m_bDevice) {
		bEnable = FALSE;

		// �f�o�C�X�I�[�v��
		OpenDevice();

		if (m_hDevice != INVALID_HANDLE_VALUE) {
			// ��Ԋl��
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

		// �f�o�C�X�N���[�Y
		CloseDevice();
	}

	// ���f�B�A��Ԕ��f
	SetEnable(bEnable);

	// �ŏI�X�V����
	m_nUpdate = ::GetTickCount();

	return bEnable;
}

//---------------------------------------------------------------------------
//
//	�C�W�F�N�g
//
//---------------------------------------------------------------------------
BOOL FASTCALL CWinFileDrv::Eject()
{
	ASSERT(this);

	// �����[�o�u�����f�B�A�łȂ���Α��I��
	if (m_bRemovable == FALSE) return FALSE;

	// ���ɃC�W�F�N�g�ς݂Ȃ瑦�I��
	if (m_bEnable == FALSE) return FALSE;

	// �f�o�C�X���䂪�ł��Ȃ���Α��I��
	if (m_bDevice == FALSE) return FALSE;

	// �f�o�C�X�I�[�v��
	OpenDevice();
	if (m_hDevice == INVALID_HANDLE_VALUE) return FALSE;

	// �}�E���g����
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

	// �C�W�F�N�g
	bResult = ::DeviceIoControl(
		m_hDevice,
		IOCTL_STORAGE_EJECT_MEDIA,
		NULL,
		0,
		NULL,
		0,
		&nSize,
		NULL);

	// �f�o�C�X�N���[�Y
	CloseDevice();

	return bResult;
}

//---------------------------------------------------------------------------
//
//	�{�����[�����x���̎擾
//
//---------------------------------------------------------------------------
void FASTCALL CWinFileDrv::GetVolume(TCHAR* szLabel)
{
	ASSERT(this);
	ASSERT(szLabel);

#if 0	// �g���Ȃ����W�b�N
	// ���f�B�A��ԃ`�F�b�N
	if (m_bEnable == FALSE) {
		m_bVolumeCache = FALSE;
		m_szVolumeCache[0] = _T('\0');
		szLabel[0] = _T('\0');
		return;
	}
#endif

	// �{�����[�����x���̎擾
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

	// �L���b�V���X�V
	m_bVolumeCache = TRUE;
	_tcscpy(szLabel, m_szVolumeCache);
}

//---------------------------------------------------------------------------
//
//	�L���b�V������{�����[�����x�����擾
//
//	�L���b�V������Ă���{�����[�����x������]������B
//	�L���b�V�����e���L���Ȃ� TRUE ���A�����Ȃ� FALSE ��Ԃ��B
//
//---------------------------------------------------------------------------
BOOL FASTCALL CWinFileDrv::GetVolumeCache(TCHAR* szLabel) const
{
	ASSERT(this);
	ASSERT(szLabel);

	// ���e��]��
	_tcscpy(szLabel, m_szVolumeCache);

	// ���f�B�A���}���Ȃ��ɃL���b�V�����e���g��
	if (m_bEnable == FALSE) return TRUE;

	return m_bVolumeCache;
}

//---------------------------------------------------------------------------
//
//	�e�ʂ̎擾
//
//---------------------------------------------------------------------------
DWORD FASTCALL CWinFileDrv::GetCapacity(Human68k::capacity_t* pCapacity)
{
	ASSERT(this);
	ASSERT(pCapacity);

#if 0	// �g���Ȃ����W�b�N
	// ���f�B�A��ԃ`�F�b�N
	if (m_bEnable == FALSE) {
		// �L���b�V������
		memset(&m_capCache, 0, sizeof(m_capCache));
		memset(pCapacity, 0, sizeof(*pCapacity));
		return 0;
	}
#endif

	// ULARGE_INTEGER�`���Ŏ擾(Win95OSR2�ȍ~�AWinNT�ŃT�|�[�g���ꂽAPI)
	ULARGE_INTEGER ulFree;
	ULARGE_INTEGER ulTotal;
	ULARGE_INTEGER ulTotalFree;
	BOOL bResult = ::GetDiskFreeSpaceEx(m_szDrive, &ulFree, &ulTotal, &ulTotalFree);
	if (bResult == FALSE) {
		// �L���b�V������
		memset(&m_capCache, 0, sizeof(m_capCache));
		memset(pCapacity, 0, sizeof(*pCapacity));
		return 0;
	}

	// �g�p�\�o�C�g��(2GB�ŃN���b�v)
	DWORD nFree = 0x7FFFFFFF;
	if (ulFree.HighPart == 0 && ulFree.LowPart < 0x80000000) {
		nFree = ulFree.LowPart;
	}

	// �N���X�^�\���̌v�Z
	DWORD free;
	DWORD clusters;
	DWORD sectors;
	DWORD bytes;
	if (ulTotal.HighPart == 0 && ulTotal.LowPart < 0x80000000) {
		// 2GB�����Ȃ�GetDiskFreeSpace���g���Ēl���擾
		bResult = ::GetDiskFreeSpace(m_szDrive, &sectors, &bytes, &free, &clusters);
		if (bResult == FALSE) {	// �O�̂���
			free = nFree;
			clusters = ulTotal.LowPart;
			sectors = 1;
			bytes = 512;
		}

		// �Z�N�^�T�C�Y��512���傫���p�^�[����␳ (�������ƂȂ����O�̂���)
		while (bytes > 512) {
			bytes >>= 1;
			sectors <<= 1;

			free <<= 1;
		}

		// �Z�N�^�T�C�Y��256�ȉ��̃p�^�[����␳ (�������ƂȂ����O�̂���)
		while (1 <= bytes && bytes <= 256) {
			bytes <<= 1;
			if (sectors > 1) {
				sectors >>= 1;
			} else {
				clusters >>= 1;
			}

			free >>= 1;
		}

		// ���N���X�^����16�r�b�g�͈̔͂Ɏ��܂�Ȃ��p�^�[����␳
		while (clusters >= 0x10000) {
			clusters >>= 1;
			sectors <<= 1;

			free >>= 1;
		}
	} else {
		// 2GB�ȏ�Ȃ�A512(�Z�N�^)�~128(�N���X�^)�~32768�ɌŒ�
		sectors = 0x80;
		clusters = 0x8000;
		bytes = 512;

		// �󂫗e�ʂ�2GB�𒴂��邱�Ƃ͖���
		free = (WORD)(nFree >> 16);
		if (nFree & 0xFFFF) free++;
	}

	// �p�����[�^�͈̓`�F�b�N (�O�̂���)
	ASSERT(free < 0x10000);
	ASSERT(clusters < 0x10000);
	ASSERT(sectors < 0x10000);
	ASSERT(bytes == 512);

	// �S�p�����[�^�Z�b�g
	m_capCache.free = (WORD)free;
	m_capCache.clusters = (WORD)clusters;
	m_capCache.sectors = (WORD)sectors;
	m_capCache.bytes = 512;

	// ���e��]��
	memcpy(pCapacity, &m_capCache, sizeof(m_capCache));

	return nFree;
}

//---------------------------------------------------------------------------
//
//	�L���b�V������N���X�^�T�C�Y���擾
//
//---------------------------------------------------------------------------
BOOL FASTCALL CWinFileDrv::GetCapacityCache(Human68k::capacity_t* pCapacity) const
{
	ASSERT(this);
	ASSERT(pCapacity);

	// ���e��]��
	memcpy(pCapacity, &m_capCache, sizeof(m_capCache));

	// ���f�B�A���}���Ȃ��ɃL���b�V�����e���g��
	if (m_bEnable == FALSE) return TRUE;

	return (m_capCache.sectors != 0);
}

//===========================================================================
//
//	�f�B���N�g���G���g�� �t�@�C����
//
//===========================================================================

DWORD CHostFilename::g_nOption;			// ����t���O (�t�@�C�����ϊ��֘A)

//---------------------------------------------------------------------------
//
//	�R���X�g���N�^
//
//---------------------------------------------------------------------------
CHostFilename::CHostFilename()
{
	m_pszWin32 = NULL;
	m_bCorrect = FALSE;
	m_szHuman[0] = '\0';
	// memset(&m_dirEntry, 0, sizeof(m_dirEntry));		// �o�O�v���������͕s�v
	m_pszHumanLast = m_szHuman;
	m_pszHumanExt = m_szHuman;
	m_pEntry = NULL;
	m_pChild = NULL;
}

//---------------------------------------------------------------------------
//
//	�f�X�g���N�^
//
//---------------------------------------------------------------------------
CHostFilename::~CHostFilename()
{
	FreeWin32();
	DeleteCache();
}

//---------------------------------------------------------------------------
//
//	Win32���̖��̂̐ݒ�
//
//---------------------------------------------------------------------------
void FASTCALL CHostFilename::SetWin32(const TCHAR* szWin32)
{
	ASSERT(this);
	ASSERT(szWin32);
	ASSERT(_tcslen(szWin32) < _MAX_PATH);
	ASSERT(m_pszWin32 == NULL);

	// Win32���̃R�s�[
	m_pszWin32 = (TCHAR*)malloc(_tcslen(szWin32) + 1);
	ASSERT(m_pszWin32);
	_tcscpy(m_pszWin32, szWin32);
}

//---------------------------------------------------------------------------
//
//	Human68k���̃t�@�C�����v�f���R�s�[
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
//	Human68k���̖��̂�ϊ�
//
//	���炩���� SetWin32() �����s���Ă������ƁB
//	18+3�̖����K���ɏ]�������O�ϊ����s�Ȃ��B
//	�t�@�C�����擪����і����̋󔒂́AHuman68k�ň����Ȃ����ߎ����I�ɍ폜�����B
//	�f�B���N�g���G���g���̖��O�������A�t�@�C�����ϊ����̊g���q�̈ʒu�����g���Đ�������B
//	���̌�A�t�@�C�����ُ̈픻����s�Ȃ��B(�X�y�[�X8���������̃t�@�C�����Ȃ�)
//	�t�@�C�����̏d������͍s�Ȃ�Ȃ��̂Œ��ӁB�����̔���͏�ʃN���X�ōs�Ȃ��B
//	TwentyOne version 1.36c modified +14 patchlevel9 �ȍ~�̊g���q�K���ɑΉ�������B
//
//---------------------------------------------------------------------------
void FASTCALL CHostFilename::SetHuman(int nCount)
{
	ASSERT(this);
	ASSERT(m_pszWin32);
	ASSERT(_tcslen(m_pszWin32) < _MAX_PATH);

	// �T�u�f�B���N�g�����̏ꍇ�͕ϊ����Ȃ�
	if (m_pszWin32[0] == '.') {
		if (m_pszWin32[1] == '\0' || (m_pszWin32[1] == '.' && m_pszWin32[2] == '\0')) {
			strcpy((char*)m_szHuman, (char*)m_pszWin32);	// WARNING: Unicode���v�C��

			m_bCorrect = TRUE;
			m_pszHumanLast = m_szHuman + strlen((char*)m_szHuman);
			m_pszHumanExt = m_pszHumanLast;
			return;
		}
	}

	DWORD nMax = 18;	// �x�[�X����(�x�[�X���Ɗg���q��)�̃o�C�g��
	if (g_nOption & WINDRV_OPT_CONVERT_LENGTH) nMax = 8;

	// �x�[�X�������̕␳����
	BYTE szNumber[8];
	BYTE* pNumber = NULL;
	if (nCount >= 0) {
		pNumber = &szNumber[8];
		for (int i = 0; i < 5; i++) {	// �ő�5+1���܂� (�x�[�X���擪2�o�C�g�͕K���c��)
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

	// �����ϊ�
	// WARNING: Unicode���Ή��B������Unicode�̐��E�ɟZ�܂ꂽ���͂����ŕϊ����s�Ȃ�
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
				else if (pWrite == szHuman) continue;	// �擪�̋󔒂͖���
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
				if (pRead - 1 == pPeriod) {		// Human68k�g���q�͗�O�Ƃ���
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

	// �g���q�␳
	if (pExt) {
		// �����̋󔒂��폜����
		while(pExt < pLast - 1 && *(pLast - 1) == ' ') {
			pLast--;
			BYTE* p = (BYTE*)pLast;
			*p = '\0';
		}

		// �ϊ���Ɏ��̂��Ȃ��Ȃ����ꍇ�͍폜
		if (pExt + 1 >= pLast) {
			pLast = pExt;
			BYTE* p = (BYTE*)pLast;
			*p = '\0';		// �O�̂���
		}
	} else {
		pExt = pLast;
	}

	// �o��l���Љ�
	//
	// pFirst: ���̓��[�_�[�B�t�@�C�����擪
	// pCut: �ʏ̃t�F�C�X�B�ŏ��̃s���I�h�̏o���ʒu ���̌�x�[�X���I�[�ʒu�ƂȂ�
	// pSecond: �悧���܂��ǂ��B���l�����}�[�h�b�N�B�g���q���̊J�n�ʒu�B�����牽�B
	// pExt: B�EA�E�o���J�X�BHuman68k�g���q�̓V�˂��B�ł��A3������蒷�����O�͊��قȁB
	// �Ō�̃s���I�h�̏o���ʒu �Y�����Ȃ����pLast�Ɠ����l
	//
	// ��pFirst            ��pStop ��pSecond ��                ��pExt
	// T h i s _ i s _ a . V e r y . L o n g . F i l e n a m e . t x t \0
	//         ��pCut �� ��pCut�����ʒu                                ��pLast
	//
	// ��L�̏ꍇ�A�ϊ���� This.Long.Filename.txt �ƂȂ�

	// 1�����ڔ���
	const BYTE* pCut = pFirst;
	const BYTE* pStop = pExt - nMax;	// �g���q���͍ő�17�o�C�g�Ƃ���(�x�[�X�����c��)
	if (pFirst < pExt) {
		pCut++;		// �K��1�o�C�g�̓x�[�X�����g��
		BYTE c = *pFirst;
		if ((0x80 <= c && c <= 0x9F) || 0xE0 <= c) {	// �����ɂ� 0x81�`0x9F 0xE0�`0xEF
			pCut++;		// �x�[�X�� �ŏ�2�o�C�g
			pStop++;	// �g���q�� �ő�16�o�C�g
		}
	}
	if (pStop < pFirst) pStop = pFirst;

	// �x�[�X������
	pCut = (BYTE*)strchr((char*)pCut, '.');	// SJIS2�o�C�g�ڂ͕K��0x40�ȏ�Ȃ̂Ŗ��Ȃ�
	if (pCut == NULL) pCut = pLast;
	if ((DWORD)(pCut - pFirst) > nMax) {
		pCut = pFirst + nMax;	// ��ق�SJIS2�o�C�g����/�␳���s�Ȃ� �����Ŕ��肵�Ă͂����Ȃ�
	}

	// �g���q������
	const BYTE* pSecond = pExt;
	const BYTE* p = pExt - 1;
	for (const BYTE* p = pExt - 1; pStop < p; p--) {
		if (*p == '.') pSecond = p;	// SJIS2�o�C�g�ڂ͕K��0x40�ȏ�Ȃ̂Ŗ��Ȃ�
	}

	// �x�[�X����Z�k
	DWORD nExt = pExt - pSecond;	// �g���q�������̒���
	if ((DWORD)(pCut - pFirst) + nExt > nMax) pCut = pFirst + nMax - nExt;
	// 2�o�C�g�����̓r���Ȃ炳��ɒZ�k
	for (p = pFirst; p < pCut; p++) {
		BYTE c = *p;
		if ((0x80 <= c && c <= 0x9F) || 0xE0 <= c) {	// �����ɂ� 0x81�`0x9F 0xE0�`0xEF
			p++;
			if (p >= pCut) {
				pCut--;
				break;
			}
		}
	}

	// ���O�̌���
	BYTE* pWrite = m_szHuman;
	pWrite = CopyName(pWrite, pFirst, pCut);	// �x�[�X����]��
	if (pNumber) pWrite = CopyName(pWrite, pNumber, &szNumber[8]);	// �␳������]��
	pWrite = CopyName(pWrite, pSecond, pExt);	// �g���q����]��
	m_pszHumanExt = pWrite;						// �g���q�ʒu�ۑ�
	pWrite = CopyName(pWrite, pExt, pLast);		// Human68k�g���q��]��
	m_pszHumanLast = pWrite;					// �I�[�ʒu�ۑ�
	*pWrite = '\0';

	// �ϊ����ʂ̊m�F
	m_bCorrect = TRUE;
	int nSize = m_pszHumanExt - m_szHuman;				// �g���q�̈ʒu��ۑ�

	// �t�@�C�����{�̂����݂��Ȃ���Εs���i
	if (nSize <= 0) m_bCorrect = FALSE;

	// �t�@�C�����{�̂�1�����ȏ�ł��󔒂ŏI�����Ă���Εs���i
	// �t�@�C�����{�̂�8�����ȏ�̏ꍇ�A���_��͋󔒂ł̏I�����\����
	// �\�����AHuman68k�ł͐����������Ȃ����߁A������s���i�Ƃ���
	else if (m_szHuman[nSize - 1] == ' ') m_bCorrect = FALSE;

	// �ϊ����ʂ��f�B���N�g�����Ɠ����Ȃ�s���i
	if (m_szHuman[0] == '.') {
		if (m_szHuman[1] == '\0' || (m_szHuman[1] == '.' && m_szHuman[2] == '\0')) {
			m_bCorrect = FALSE;
		}
	}
}

//---------------------------------------------------------------------------
//
//	Human68k���̖��̂𕡐�
//
//	�t�@�C���������̏��𕡐����ASetHuman() �����̏�����������s�Ȃ��B
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
//	�f�B���N�g���G���g�����̐ݒ�
//
//	�z�X�g��Find�̌��ʂ���A�����E�T�C�Y�E���t�����f�B���N�g���G��
//	�g���ɔ��f����B�t�@�C��������Find�̌��ʂł͂Ȃ��N���X�ϐ��ɐ�
//	��ς݂̃f�[�^�𔽉f���邽�߁A���炩���� SetHuman() �����s���Ă�
//	������(���s���Ă��Ȃ��Ă����ɕs��͋N���Ȃ���)�B
//
//---------------------------------------------------------------------------
BOOL FASTCALL CHostFilename::SetEntry(const WIN32_FIND_DATA* pWin32Find)
{
	ASSERT(this);
	ASSERT(pWin32Find);

	// �t�@�C�����ݒ�
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

	// �����ݒ�
	DWORD n = pWin32Find->dwFileAttributes;
	BYTE nHumanAttribute = Human68k::AT_ARCHIVE;
	if ((n & FILE_ATTRIBUTE_DIRECTORY) != 0) {
		nHumanAttribute = Human68k::AT_DIRECTORY;
	}
	if ((n & FILE_ATTRIBUTE_SYSTEM) != 0) nHumanAttribute |= Human68k::AT_SYSTEM;
	if ((n & FILE_ATTRIBUTE_HIDDEN) != 0) nHumanAttribute |= Human68k::AT_HIDDEN;
	if ((n & FILE_ATTRIBUTE_READONLY) != 0) nHumanAttribute |= Human68k::AT_READONLY;
	m_dirEntry.attr = nHumanAttribute;

	// �T�C�Y�ݒ�
	m_dirEntry.size = pWin32Find->nFileSizeLow;
	if (pWin32Find->nFileSizeHigh > 0 || pWin32Find->nFileSizeLow > XM6_HOST_FILESIZE_MAX)
		m_dirEntry.size = XM6_HOST_FILESIZE_MAX;

	// ���t�E�����ݒ�
	m_dirEntry.date = 0;
	m_dirEntry.time = 0;
	FILETIME ft = pWin32Find->ftLastWriteTime;
	if (ft.dwLowDateTime == 0 && ft.dwHighDateTime == 0) {
		ft = pWin32Find->ftCreationTime;
	}
	FILETIME lt;
	if (::FileTimeToLocalFileTime(&ft, &lt) == 0) return FALSE;
	if (::FileTimeToDosDateTime(&lt, &m_dirEntry.date, &m_dirEntry.time) == 0) return FALSE;

	// �N���X�^�ԍ��ݒ�
	m_dirEntry.cluster = 0;

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	Human68k���̖��̂����H���ꂽ������
//
//---------------------------------------------------------------------------
BOOL FASTCALL CHostFilename::isReduce() const
{
	ASSERT(this);
	ASSERT(m_pszWin32);

	return strcmp((char*)m_pszWin32, (char*)m_szHuman) != 0; // WARNING: Unicode���v�C��
}

//---------------------------------------------------------------------------
//
//	�Y���G���g���̃L���b�V�����폜
//
//---------------------------------------------------------------------------
void FASTCALL CHostFilename::DeleteCache()
{
	ASSERT(this);

	if (m_pEntry) m_pEntry->DeleteCache(m_pChild);
}

//---------------------------------------------------------------------------
//
//	Human68k�t�@�C��������g���q�𕪗�
//
//---------------------------------------------------------------------------
const BYTE* FASTCALL CHostFilename::SeparateExt(const BYTE* szHuman)
{
	// �t�@�C�����̒������l��
	int nLength = strlen((char*)szHuman);
	const BYTE* pFirst = szHuman;
	const BYTE* pLast = pFirst + nLength;

	// Human68k�g���q�̈ʒu���m�F
	const BYTE* pExt = (BYTE*)strrchr((char*)pFirst, '.');	// SJIS2�o�C�g�ڂ͕K��0x40�ȏ�Ȃ̂Ŗ��Ȃ�
	if (pExt == NULL) pExt = pLast;
	// �t�@�C������20�`22��������19�����ڂ�'.'����'.'�ŏI���Ƃ����p�^�[������ʈ�������
	if (20 <= nLength && nLength <= 22 && pFirst[18] == '.' && pFirst[nLength-1] == '.')
		pExt = pFirst + 18;
	// �g���q�̕��������v�Z	(-1:�Ȃ� 0:�s���I�h���� 1�`3:Human68k�g���q 4�ȏ�:�g���q��)
	int nExt = pLast - pExt - 1;
	// '.' ��������擪�ȊO�ɑ��݂��āA����1�`3�����̏ꍇ�̂݊g���q�Ƃ݂Ȃ�
	if (pExt == pFirst || nExt < 1 || nExt > 3) pExt = pLast;

	return pExt;
}

//===========================================================================
//
//	�f�B���N�g���G���g�� �p�X��
//
//	Human68k���̃p�X���́A�K���擪�� / �Ŏn�܂�A������ / �����Ȃ��B
//	(������ / �ŏI���ꍇ�͋�̃t�@�C�����w��Ɣ��f�����)
//	�܂��A���j�b�g�ԍ��������A��Ƀy�A�ň����B
//	�]���� mfc_host �ł̈����ƈقȂ�̂Œ��ӁB
//	�������̂��߁AWin32�p�X���̓x�[�X�p�X�������܂ށB
//	�x�[�X�p�X�����Ȃ��ł�����\�Ƃ���B
//
//===========================================================================

DWORD CHostPath::g_nId;				// ����ID�����p�J�E���^
DWORD CHostPath::g_nOption;			// ����t���O (�p�X����r�֘A)

//---------------------------------------------------------------------------
//
//	�R���X�g���N�^
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
//	�f�X�g���N�^
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
//	Human68k���̖��̂𒼐ڎw�肷��
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
//	Win32���̖��̂𒼐ڎw�肷��
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
//	�������r (���C���h�J�[�h�Ή�)
//
//---------------------------------------------------------------------------
int FASTCALL CHostPath::Compare(const BYTE* pFirst, const BYTE* pLast, const BYTE* pBufFirst, const BYTE* pBufLast)
{
	ASSERT(pFirst);
	ASSERT(pLast);
	ASSERT(pBufFirst);
	ASSERT(pBufLast);

	// ������r
	BOOL bSkip0 = FALSE;
	BOOL bSkip1 = FALSE;
	for (const BYTE* p = pFirst; p < pLast; p++) {
		// 1�����ǂݍ���
		BYTE c = *p;
		BYTE d = '\0';
		if (pBufFirst < pBufLast) d = *pBufFirst++;

		// ��r�̂��߂̕����␳
		if (bSkip0 == FALSE) {
			if (bSkip1 == FALSE) {	// c��d��1�o�C�g��
				if ((0x80 <= c && c <= 0x9F) || 0xE0 <= c) {	// �����ɂ� 0x81�`0x9F 0xE0�`0xEF
					bSkip0 = TRUE;
				}
				if ((0x80 <= d && d <= 0x9F) || 0xE0 <= d) {	// �����ɂ� 0x81�`0x9F 0xE0�`0xEF
					bSkip1 = TRUE;
				}
				if (c == d) continue;	// ���m���Ŕ��芮������
				if ((g_nOption & WINDRV_OPT_ALPHABET) == 0) {
					if ('A' <= c && c <= 'Z') c += 'a' - 'A';	// ��������
					if ('A' <= d && d <= 'Z') d += 'a' - 'A';	// ��������
				}
			} else {		// c������1�o�C�g��
				if ((0x80 <= c && c <= 0x9F) || 0xE0 <= c) {	// �����ɂ� 0x81�`0x9F 0xE0�`0xEF
					bSkip0 = TRUE;
				}
				bSkip1 = FALSE;
			}
		} else {
			if (bSkip1 == FALSE) {	// d������1�o�C�g��
				bSkip0 = FALSE;
				if ((0x80 <= d && d <= 0x9F) || 0xE0 <= d) {	// �����ɂ� 0x81�`0x9F 0xE0�`0xEF
					bSkip1 = TRUE;
				}
			} else {		// c��d��2�o�C�g��
				bSkip0 = FALSE;
				bSkip1 = FALSE;
			}
		}

		// ��r
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
//	Human68k���̖��̂��r����
//
//---------------------------------------------------------------------------
BOOL FASTCALL CHostPath::isSameHuman(DWORD nUnit, const BYTE* szHuman) const
{
	ASSERT(this);
	ASSERT(szHuman);
	ASSERT(m_pszHuman);

	// ���j�b�g�ԍ��̔�r
	if (m_nHumanUnit != nUnit) return FALSE;

	// �������v�Z
	const BYTE* pFirst = m_pszHuman;
	DWORD nLength = strlen((char*)pFirst);
	const BYTE* pLast = pFirst + nLength;

	const BYTE* pBufFirst = szHuman;
	DWORD nBufLength = strlen((char*)pBufFirst);
	const BYTE* pBufLast = pBufFirst + nBufLength;

	// �������`�F�b�N
	if (nLength != nBufLength) return FALSE;

	// Human68k�p�X���̔�r
	return Compare(pFirst, pLast, pBufFirst, pBufLast) == 0;
}

//---------------------------------------------------------------------------
//
//	�t�@�C����������
//
//	���L����L���V���o�b�t�@�̒����猟�����A������΂��̖��̂�Ԃ��B
//	�p�X�������O���Ă������ƁB
//	�K����ʂŔr��������s�Ȃ����ƁB
//
//---------------------------------------------------------------------------
CHostFilename* FASTCALL CHostPath::FindFilename(BYTE* szHuman, DWORD nHumanAttribute)
{
	ASSERT(this);
	ASSERT(szHuman);

	// �������v�Z
	const BYTE* pFirst = szHuman;
	DWORD nLength = strlen((char*)pFirst);
	const BYTE* pLast = pFirst + nLength;

	// �������Ă���S�Ẵt�@�C�����̒����犮�S��v������̂�����
	for (CHostFilename* p = (CHostFilename*)m_cRingFilename.Next();
		 p != &m_cRingFilename; p = (CHostFilename*)p->Next()) {
		// �����`�F�b�N
		if (p->CheckAttribute(nHumanAttribute) == 0) continue;
		// �������v�Z
		const BYTE* pBufFirst = p->GetHuman();
		const BYTE* pBufLast = p->GetHumanLast();
		DWORD nBufLength = pBufLast - pBufFirst;
		// �������`�F�b�N
		if (nLength != nBufLength) continue;
		// �t�@�C�����`�F�b�N
		if (Compare(pFirst, pLast, pBufFirst, pBufLast) == 0) return p;
	}

	return NULL;
}

//---------------------------------------------------------------------------
//
//	�t�@�C���������� (���C���h�J�[�h�Ή�)
//
//	���L����o�b�t�@�̒����猟�����A������΂��̖��̂�Ԃ��B
//	�p�X�������O���Ă������ƁB
//	�K����ʂŔr��������s�Ȃ����ƁB
//
//---------------------------------------------------------------------------
CHostFilename* FASTCALL CHostPath::FindFilenameWildcard(BYTE* szHuman, find_t* pFind, DWORD nHumanAttribute)
{
	ASSERT(this);
	ASSERT(szHuman);
	ASSERT(pFind);

	// �����t�@�C������{�̂�Human68k�g���q�ɕ�����
	const BYTE* pFirst = szHuman;
	const BYTE* pLast = pFirst + strlen((char*)pFirst);
	const BYTE* pExt = CHostFilename::SeparateExt(pFirst);

	// �J�n�n�_�ֈړ�
	CHostFilename* p = (CHostFilename*)m_cRingFilename.Next();
	if (pFind->count > 0) {
		if (pFind->id == m_nId) {
			// �L���b�V�����e�ɕω����Ȃ��ꍇ�́A�O��̈ʒu���瑦����
			p = pFind->pos;
		} else {
			// �J�n�n�_���G���g�����e���猟������
			DWORD n = 0;
			for (; p != &m_cRingFilename; p = (CHostFilename*)p->Next()) {
				if (p->isSameEntry(&pFind->entry)) {
					pFind->count = n;
					break;
				}
				n++;
			}

			// �Y���G���g����������Ȃ������ꍇ�A�񐔎w����g��
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

	// �t�@�C������
	for (; p != &m_cRingFilename; p = (CHostFilename*)p->Next()) {
		pFind->count++;

		// �����`�F�b�N
		if (p->CheckAttribute(nHumanAttribute) == 0) continue;

		// �t�@�C������{�̂�Human68k�g���q�ɕ�����
		const BYTE* pBufFirst = p->GetHuman();
		const BYTE* pBufLast = p->GetHumanLast();
		const BYTE* pBufExt = p->GetHumanExt();

		// �{�̔�r
		if (Compare(pFirst, pExt, pBufFirst, pBufExt) != 0) continue;

		// Human68k�g���q��r
		// �g���q .??? (.*) �̏ꍇ�́AHuman68k�g���q�̃s���I�h�Ȃ��ɂ��}�b�`������
		if (strcmp((char*)pExt, ".???") == 0 ||	// strncmp((char*)pExt, ".*", 2) == 0 ||
			Compare(pExt, pLast, pBufExt, pBufLast) == 0) {
			// ���̌��̃G���g�����e���L�^
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
//	�ė��p�̂��߂̏�����
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
//	�S�t�@�C�������J��
//
//---------------------------------------------------------------------------
void FASTCALL CHostPath::CleanFilename()
{
	// ���̂��J��
	CHostFilename* p;
	while ((p = (CHostFilename*)m_cRingFilename.Next()) != &m_cRingFilename) {
		delete p;
	}
}

//---------------------------------------------------------------------------
//
//	�t�@�C���ύX���s�Ȃ�ꂽ���m�F
//
//---------------------------------------------------------------------------
BOOL FASTCALL CHostPath::isRefresh()
{
	ASSERT(this);

	if (m_hChange == INVALID_HANDLE_VALUE) return TRUE;		// ����������Ă��Ȃ���Ηv�X�V

	DWORD nResult = WaitForSingleObject(m_hChange, 0);
	if (nResult == WAIT_TIMEOUT) return FALSE;

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	�t�@�C���č\��
//
//	�����ŏ��߂āAWin32�t�@�C���V�X�e���̊ϑ����s�Ȃ���B
//	�K����ʂŔr��������s�Ȃ����ƁB
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

	// �p�X������
	TCHAR szPath[_MAX_PATH];
	_tcscpy(szPath, m_pszWin32);
	_tcscat(szPath, _T("\\"));

	// �X�V�`�F�b�N
	if (m_hChange == INVALID_HANDLE_VALUE) {
		// ����
		DWORD nFlag =
			FILE_NOTIFY_CHANGE_FILE_NAME |
			FILE_NOTIFY_CHANGE_DIR_NAME |
			FILE_NOTIFY_CHANGE_ATTRIBUTES |
			FILE_NOTIFY_CHANGE_SIZE |
			FILE_NOTIFY_CHANGE_LAST_WRITE;
		m_hChange = ::FindFirstChangeNotification(szPath, FALSE, nFlag);
		// �O���A�v���̉e���ŃG���[���o��\������B���̂܂ܑ��s���Ă����Ȃ�
	} else {
		// ���ڈȍ~
		for (int i = 0; i < 3; i++) {	// 1����s���������ł̓t���b�V������Ȃ��\��������
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

	// �ȑO�̃L���b�V�����e���ړ�
	CRing cRingBackup;
	m_cRingFilename.InsertRing(&cRingBackup);

	// �����p�t�@�C��������
	_tcscat(szPath, _T("*.*"));

	// �t�@�C�����o�^
	BOOL bUpdate = FALSE;
	HANDLE hFind = INVALID_HANDLE_VALUE;
	for (int i = 0; i < XM6_HOST_FILENAME_CACHE_MAX; i++) {
		WIN32_FIND_DATA w32Find;
		if (hFind == INVALID_HANDLE_VALUE) {
			// �ŏ��̃t�@�C�������l��
			hFind = ::FindFirstFile(szPath, &w32Find);
			if (hFind == INVALID_HANDLE_VALUE) break;
		} else {
			// ���̃t�@�C�������l��
			BOOL bResult = ::FindNextFile(hFind, &w32Find);
			if (bResult == FALSE) break;
		}

		// �V�K�t�@�C�����o�^
		CHostFilename* pFilename = new CHostFilename;
		ASSERT(pFilename);
		pFilename->SetWin32((TCHAR*)w32Find.cFileName);

		// �ȑO�̃L���b�V�����e�ɊY������t�@�C����������΂���Human68k���̂�D�悷��
		CHostFilename* pCache = (CHostFilename*)cRingBackup.Next();
		for (;;) {
			if (pCache == &cRingBackup) {
				pCache = NULL;			// �Y������G���g���Ȃ�(���̎��_�ŐV�K�G���g���Ɗm��)
				pFilename->SetHuman();
				break;
			}
			if (_tcscmp(pFilename->GetWin32(), pCache->GetWin32()) == 0) {
				pFilename->CopyHuman(pCache->GetHuman());	// Human68k���̂̃R�s�[
				break;
			}
			pCache = (CHostFilename*)pCache->Next();
		}

		// �V�K�G���g���̏ꍇ�̓t�@�C�����d�����`�F�b�N����
		// �t�@�C�����ɕύX���������ꍇ�́A�ȉ��̃`�F�b�N��S�ăp�X�������̂��g�p����
		// �E�������t�@�C�����ł��邱��
		// �E�ߋ��̃G���g���ɓ����̂��̂����݂��Ȃ�����
		// �E�����̎��t�@�C���������݂��Ȃ�����
		if (pFilename->isReduce()) {	// �t�@�C�����̕ύX���s�Ȃ�ꂽ�ꍇ�̂݃`�F�b�N
			for (int n = 0; n < 36 * 36 * 36 * 36 * 36; n++) {	// ��6�疜�p�^�[��(36��5��)
				// �������t�@�C�������ǂ����m�F
				if (pFilename->isCorrect()) {
					// �ߋ��̃G���g���ƈ�v���邩�m�F
					CHostFilename* pCheck = FindFilename(pFilename->GetHuman());
					if (pCheck == NULL) {
						// ��v������̂��Ȃ���΁A���t�@�C�������݂��邩�m�F
						TCHAR szBuf[_MAX_PATH];
						_tcscpy(szBuf, m_pszWin32);
						_tcscat(szBuf, _T("\\"));
						_tcscat(szBuf, (TCHAR*)pFilename->GetHuman());	// WARNING: Unicode���v�C��
						WIN32_FIND_DATA w32Check;
						HANDLE hCheck = ::FindFirstFile(szBuf, &w32Check);
						if (hCheck == INVALID_HANDLE_VALUE) break;	// ���p�\�p�^�[���𔭌�
						FindClose(hCheck);
					}
				}
				// �V�������O�𐶐�
				pFilename->SetHuman(n);
			}
		}

		// �f�B���N�g���G���g������
		pFilename->SetEntry(&w32Find);

		// �ȑO�̃L���b�V�����e�Ɣ�r
		if (pCache) {
			if (pCache->isSameEntry(pFilename->GetEntry())) {
				delete pFilename;		// ����쐬�����G���g���͔j����
				pFilename = pCache;		// �ȑO�̃L���b�V�����e���g��
			} else {
				bUpdate = TRUE;			// ��v���Ȃ���΍X�V����
				delete pCache;			// ����̌����Ώۂ��珜�O
			}
		} else {
			bUpdate = TRUE;				// �V�K�G���g���̂��ߍX�V����
		}

		// �����O�����֒ǉ�
		pFilename->InsertTail(&m_cRingFilename);
	}

	if (hFind != INVALID_HANDLE_VALUE) FindClose(hFind);

	// �c������L���b�V�����e���폜
	CHostFilename* p;
	while ((p = (CHostFilename*)cRingBackup.Next()) != &cRingBackup) {
		bUpdate = TRUE;			// �폜���s�Ȃ��̂ōX�V����
		delete p;
	}

	// �X�V���s�Ȃ�ꂽ�环��ID��ύX
	if (bUpdate) m_nId = g_nId++;

	// �Ō�ɃG���[������ʒm
	if (m_hChange == INVALID_HANDLE_VALUE) return FALSE;

	return TRUE;
}

//===========================================================================
//
//	�f�B���N�g���G���g���Ǘ�
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	������ (�N������)
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
//	�J�� (�I����)
//
//---------------------------------------------------------------------------
CHostEntry::~CHostEntry()
{
	Clean();

	DeleteCriticalSection(&m_csAccess);
}

//---------------------------------------------------------------------------
//
//	������ (�h���C�o�g���ݎ�)
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

	// �C�x���g�m��
	m_hEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);		// �������Z�b�g
	ASSERT(m_hEvent);

	// �Ď��X���b�h�J�n
	DWORD nThread;
	m_hThread = ::CreateThread(NULL, 0, Run, this, 0, &nThread);
	ASSERT(m_hThread);
#endif // XM6_HOST_STRICT_TIMEOUT
}

//---------------------------------------------------------------------------
//
//	�J�� (�N���E���Z�b�g��)
//
//---------------------------------------------------------------------------
void CHostEntry::Clean()
{
	ASSERT(this);

#ifdef XM6_HOST_STRICT_TIMEOUT
	// �Ď��X���b�h�I��
	if (m_hThread) {
		ASSERT(m_hEvent);

		// �X���b�h�֒�~�v��
		::SetEvent(m_hEvent);

		// �X���b�h�I���҂�
		DWORD nResult;
		nResult = ::WaitForSingleObject(m_hThread, 30 * 1000);	// �P�\���� 30�b
		if (nResult != WAIT_OBJECT_0) {
			// ������~
			ASSERT(FALSE);	// �O�̂���
			::TerminateThread(m_hThread, -1);
			nResult = ::WaitForSingleObject(m_hThread, 100);
		}

		// �X���b�h�n���h���J��
		::CloseHandle(m_hThread);
		m_hThread = NULL;
	}

	// �C�x���g�J��
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
//	�X���b�h���s�J�n�|�C���g
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
//	�X���b�h����
//
//---------------------------------------------------------------------------
void FASTCALL CHostEntry::Runner()
{
	ASSERT(this);
	ASSERT(m_hEvent);

	for (;;) {
		DWORD nResult = ::WaitForSingleObject(m_hEvent, XM6_HOST_REMOVABLE_CHECK_TIME);
		if (nResult == WAIT_OBJECT_0) break;

		// �S�h���C�u�̃^�C���A�E�g�`�F�b�N
		CheckTimeout();
	}
}
#endif // XM6_HOST_STRICT_TIMEOUT

//---------------------------------------------------------------------------
//
//	�w�肳�ꂽ���j�b�g�̃L���b�V����S�č폜����
//
//	�K����ʂŔr��������s�Ȃ����ƁB
//
//---------------------------------------------------------------------------
void FASTCALL CHostEntry::CleanCache()
{
	ASSERT(this);

	// ���̂��J��
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
//	�w�肳�ꂽ���j�b�g�̃L���b�V����S�č폜����
//
//	�����[�o�u�����f�B�A�̔r�o�ɔ����폜�����B
//	�K����ʂŔr��������s�Ȃ����ƁB
//
//---------------------------------------------------------------------------
void FASTCALL CHostEntry::EraseCache(DWORD nUnit)
{
	ASSERT(this);

	// �������Ă���S�Ẵt�@�C�����̒������v������̂�����
	for (CHostPath* p = (CHostPath*)m_cRingPath.Next(); p != &m_cRingPath;) {
		if (p->isSameUnit(nUnit)) {
			delete p;
			m_nRingPath--;

			// �A�������Ŏ��̃I�u�W�F�N�g��������\��������̂ōŏ����璲�ׂȂ���
			p = (CHostPath*)m_cRingPath.Next();
			continue;
		}
		p = (CHostPath*)p->Next();
	}
}

//---------------------------------------------------------------------------
//
//	�Y������L���b�V�����폜����
//
//	�L���b�V���͊��ɍ폜�ς݂̂��Ƃ����邽�߁A�m�F���s�Ȃ��Ă���폜����B
//	�K����ʂŔr��������s�Ȃ����ƁB
//
//---------------------------------------------------------------------------
void FASTCALL CHostEntry::DeleteCache(CHostPath* pPath)
{
	ASSERT(this);
	ASSERT(pPath);

	// �������Ă���S�Ẵt�@�C�����̒������v������̂�����
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
//	�Y������p�X�����L���b�V������Ă��邩��������
//
//	���L����L���V���o�b�t�@�̒����犮�S��v�Ō������A������΂��̖��̂�Ԃ��B
//	�t�@�C���������O���Ă������ƁB
//	�K����ʂŔr��������s�Ȃ����ƁB
//
//---------------------------------------------------------------------------
CHostPath* FASTCALL CHostEntry::FindCache(DWORD nUnit, const BYTE* szHuman)
{
	ASSERT(this);
	ASSERT(szHuman);

	// �������Ă���S�Ẵt�@�C�����̒����犮�S��v������̂�����
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
//	�L���b�V���������ɁAWin32�����l������
//
//	�p�X�E�t�@�C������1�i�K�����������A�L���b�V���ɂ��邩�m�F�B�Ȃ���΃G���[�B
//	���������L���b�V���̍X�V�`�F�b�N�B�X�V���K�v�Ȃ�G���[�B
//	�K����ʂŔr��������s�Ȃ����ƁB
//
//---------------------------------------------------------------------------
CHostPath* FASTCALL CHostEntry::CopyCache(DWORD nUnit, const BYTE* szHuman, TCHAR* szWin32Buffer)
{
	ASSERT(this);
	ASSERT(szHuman);
	ASSERT(strlen((char*)szHuman) < HUMAN68K_MAX_PATH);

	// �p�X���ƃt�@�C�����ɕ���
	const BYTE* pSeparate = SeparatePath(szHuman);
	if (pSeparate == NULL) return NULL;	// �G���[: �p�X�������s

	BYTE szHumanPath[HUMAN68K_MAX_PATH];
	int nPath = pSeparate - szHuman;
	strncpy((char*)szHumanPath, (char*)szHuman, nPath);
	szHumanPath[nPath] = '\0';

	BYTE szHumanFilename[_MAX_PATH];
	strcpy((char*)szHumanFilename, (char*)pSeparate + 1);

	// �p�X���̃L���b�V���c���`�F�b�N
	CHostPath* pPath = FindCache(nUnit, szHumanPath);
	if (pPath == NULL) return NULL;	// �G���[: �p�X�����L���b�V������Ă��Ȃ����߉�͕s�\

	// �����O�擪�ֈړ�
	pPath->Insert(&m_cRingPath);

	// �L���b�V���X�V�`�F�b�N
	if (pPath->isRefresh()) return NULL;	// �G���[: �L���b�V���X�V���K�v

	// Win32�p�X�����R�s�[
	if (szWin32Buffer) {
		_tcscpy(szWin32Buffer, pPath->GetWin32());
		_tcscat(szWin32Buffer, _T("\\"));
	}

	// �t�@�C�������Ȃ���ΏI��
	if (szHumanFilename[0] == '\0') {
		return pPath;	// ���ʂ͂����ŏI��
	}

	// �t�@�C��������
	CHostFilename* pFilename = pPath->FindFilename(szHumanFilename);
	if (pFilename == NULL) return NULL;	// �G���[: �r���̃p�X��/�t�@�C������������Ȃ�

	// Win32�t�@�C�������R�s�[
	if (szWin32Buffer) {
		_tcscat(szWin32Buffer, pFilename->GetWin32());
	}

	return pPath;
}

//---------------------------------------------------------------------------
//
//	Win32���̍\�z�ɕK�v�ȏ������ׂĎ擾����
//
//	�t�@�C�����͏ȗ��\�B(���ʂ͎w�肵�Ȃ�)
//	�K����ʂŔr��������s�Ȃ����ƁB
//	�x�[�X�p�X�����Ƀp�X��؂蕶�������Ȃ��悤���ӁB
//	�t�@�C���A�N�Z�X����������\��������Ƃ��́AVM�X���b�h�̓�����J�n������B
//
//	�g������:
//	CopyCache()���ăG���[�̏ꍇ��MakeCache()����B�K��������Win32�p�X���擾�ł���B
//
//	�t�@�C�����ƃp�X�������ׂĕ�������B
//	��ʃt�H���_���珇�ɁA�L���b�V������Ă��邩�ǂ����m�F�B
//	�L���b�V������Ă���Δj���`�F�b�N�B�j�������ꍇ���L���b�V�������ƂȂ�B
//	�L���b�V������Ă��Ȃ���΃L���b�V�����\�z�B
//	���Ԃɂ��ׂẴt�H���_�E�t�@�C�����ɑ΂��čs�Ȃ��I���B
//	�G���[�����������ꍇ��NULL�ƂȂ�B
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

	BYTE szHumanPath[HUMAN68K_MAX_PATH];	// ��ʃt�H���_���珇�Ƀp�X���������Ă䂭
	szHumanPath[0] = '\0';
	int nHumanPath = 0;

	TCHAR szWin32Path[_MAX_PATH];
	_tcscpy(szWin32Path, szWin32Base);
	int nWin32Path = _tcslen(szWin32Path);

	CHostFilename* pFilename = NULL;	// �e�t�H���_�̎��̂�����
	CHostPath* pPath;
	const BYTE* p = szHuman;
	for (;;) {
		// �t�@�C�������������
		BYTE szHumanFilename[24];			// �t�@�C��������
		p = SeparateCopyFilename(p, szHumanFilename);
		if (p == NULL) return NULL;		// �G���[: �t�@�C�����ǂݍ��ݎ��s

		// �Y���p�X���L���b�V������Ă��邩�H
		pPath = FindCache(nUnit, szHumanPath);
		if (pPath == NULL) {
			// �L���b�V���ő吔�`�F�b�N
			if (m_nRingPath >= XM6_HOST_DIRENTRY_CACHE_MAX) {
				// �����̃I�u�W�F�N�g�̂����A�ł��Â����̂��擾
				pPath = (CHostPath*)m_cRingPath.Prev();
				pPath->Clean();		// �S�t�@�C���J�� �X�V�`�F�b�N�p�n���h�����J��
			} else {
				// �V�K�o�^
				pPath = new CHostPath;
				ASSERT(pPath);
				m_nRingPath++;
			}
			pPath->SetHuman(nUnit, szHumanPath);
			pPath->SetWin32(szWin32Path);
		}

		// �L���b�V���X�V�`�F�b�N
		if (pPath->isRefresh()) {
			// �ᑬ���f�B�A�łȂ��Ă�VM�X���b�h�����s�J�n
			ps->Ready();
			BOOL bResult = CheckMediaAccess(nUnit, FALSE);
			if (bResult) bResult = pPath->Refresh();	// ���R�X�g����
			if (bResult == FALSE) {
				// �X�V���s���́A���ꃆ�j�b�g�̃L���b�V����S�ď�������
				delete pPath;
				m_nRingPath--;
				EraseCache(nUnit);
				return NULL;		// �G���[: �p�X���擾�ł��Ȃ�
			}
		}

		// �����O�擪��
		pPath->Insert(&m_cRingPath);

		// �e�t�H���_������΋L�^
		if (pFilename) pFilename->SetChild(this, pPath);

		// �t�@�C�������Ȃ���΂����ŏI��(�p�X�������������ꍇ)
		// �t�@�C�����Ȃ��ŋA���Ă���͕̂�����I�[�̏ꍇ�݂̂Ȃ̂ŏI������Ƃ��Ďg����
		if (szHumanFilename[0] == '\0') {
			// �p�X����A��
			if (nHumanPath + 1 + 1 > HUMAN68K_MAX_PATH) return NULL;	// �G���[: Human68k�p�X����������
			szHumanPath[nHumanPath++] = '/';
			szHumanPath[nHumanPath] = '\0';
			if (nWin32Path + 1 + 1 > _MAX_PATH) return NULL;	// �G���[: Win32�p�X����������
			szWin32Path[nWin32Path++] = '\\';
			szWin32Path[nWin32Path] = '\0';
			break;	// ���ʂ͂����ŏI��
		}

		// ���̃p�X������
		// �p�X�̓r���Ȃ�f�B���N�g�����ǂ����m�F
		if (*p != '\0') {
			pFilename = pPath->FindFilename(szHumanFilename, Human68k::AT_DIRECTORY);
		} else {
			pFilename = pPath->FindFilename(szHumanFilename);
		}
		if (pFilename == NULL) return NULL;	// �G���[: �r���̃p�X��/�t�@�C������������Ȃ�

		// �p�X����A��
		int n = strlen((char*)szHumanFilename);
		if (nHumanPath + n + 1 > HUMAN68K_MAX_PATH) return NULL;	// �G���[: Human68k�p�X����������
		szHumanPath[nHumanPath++] = '/';
		strcpy((char*)szHumanPath + nHumanPath, (char*)szHumanFilename);
		nHumanPath += n;

		n = _tcslen(pFilename->GetWin32());
		if (nWin32Path + n + 1 > _MAX_PATH) return NULL;	// �G���[: Win32�p�X����������
		szWin32Path[nWin32Path++] = '\\';
		_tcscpy(szWin32Path + nWin32Path, pFilename->GetWin32());
		nWin32Path += n;

		// PLEASE CONTINUE
		if (*p == '\0') break;
	}

	// Win32�����R�s�[
	if (szWin32Buffer) {
		_tcscpy(szWin32Buffer, szWin32Path);
	}

	return pPath;
}

//---------------------------------------------------------------------------
//
//	�x�[�X�p�X�����擾����
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
//	�������݋֎~���ǂ����m�F����
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
//	�ᑬ���f�B�A�`�F�b�N�ƃI�t���C����ԃ`�F�b�N
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

	// �ᑬ�f�o�C�X�Ȃ�VM�X���b�h�̓�����J�n����
	if (m_ppBase[nUnit]->isSlow()) ps->Ready();

	// �蓮�C�W�F�N�g�f�o�C�X�̃A�N�Z�X���O�`�F�b�N
	if (bMedia) CheckMediaAccess(nUnit, TRUE);

	// �����[�o�u�����f�B�A�̃L���b�V���L�����ԍX�V
	m_ppBase[nUnit]->SetTimeout();

	// �蓮�C�W�F�N�g�f�o�C�X�́u�f�B�X�N�������Ă��܂���v�ɂ��Ȃ�
	if (m_ppBase[nUnit]->isManual()) return FALSE;

	// �I�t���C����ԃ`�F�b�N
	return m_ppBase[nUnit]->isEnable() == FALSE;
}

//---------------------------------------------------------------------------
//
//	���f�B�A�o�C�g�̎擾
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
//	�h���C�u��Ԃ̎擾
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
//	�S�h���C�u�̃^�C���A�E�g�`�F�b�N
//
//---------------------------------------------------------------------------
void FASTCALL CHostEntry::CheckTimeout()
{
	ASSERT(this);
	ASSERT(m_ppBase);

#ifndef XM6_HOST_STRICT_TIMEOUT
	// �^�C���A�E�g���m�F
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
//	�����[�o�u�����f�B�A�̏�ԍX�V��L���ɂ���
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
//	�����[�o�u�����f�B�A�̏�ԍX�V�`�F�b�N
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

	// �X�V���� ���x��1
	BOOL bResult = m_ppBase[nUnit]->CheckMediaUpdate();
	if (bResult) {
		// �X�V���� ���x��2
		bResult = m_ppBase[nUnit]->CheckMediaAccess(FALSE);
		if (bResult) {
			// �ᑬ���f�B�A�`�F�b�N
			if (m_ppBase[nUnit]->isSlow()) ps->Ready();

			// ���f�B�A�L���`�F�b�N
			bResult = m_ppBase[nUnit]->CheckMedia();
			if (bResult == FALSE) {
				// ���f�B�A�g�p�s�\���̓L���b�V���N���A
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
//	�����[�o�u�����f�B�A�̃A�N�Z�X���O�`�F�b�N
//
//	�Ăяo�����ɂ��A���ꂼ��ȉ��̂悤�ȏ������s�Ȃ�
//	1: MakeCache()			�ᑬ����s�v/�N���A�s�v/�蓮�C�W�F�N�g���f�B�A�̂�
//	2: isMediaOffline()		�ᑬ����s�v/�N���A�K�v/�蓮�C�W�F�N�g���f�B�A�̂�
//	3: CheckMediaUpdate()�ɓ�����	�ᑬ����K�v/�N���A�K�v/�����[�o�u�����f�B�A�S��
//
//---------------------------------------------------------------------------
BOOL FASTCALL CHostEntry::CheckMediaAccess(DWORD nUnit, BOOL bErase)
{
	ASSERT(this);
	ASSERT(m_ppBase);
	ASSERT(nUnit < CWinFileSys::DrvMax);
	ASSERT(m_ppBase[nUnit]);

	// �X�V����
	BOOL bResult = m_ppBase[nUnit]->CheckMediaAccess(TRUE);
	if (bResult) {
		// ���f�B�A�L���`�F�b�N
		bResult = m_ppBase[nUnit]->CheckMedia();
		if (bResult == FALSE) {
			// ���f�B�A�g�p�s�\���̓L���b�V���N���A
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
//	�C�W�F�N�g
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
//	�{�����[�����x���̎擾
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
//	�L���b�V������{�����[�����x�����擾
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
//	�e�ʂ̎擾
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
//	�L���b�V������N���X�^�T�C�Y���擾
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
//	�t�@�C���V�X�e����Ԓʒm
//
//---------------------------------------------------------------------------
void FASTCALL CHostEntry::ShellNotify(DWORD nEvent, const TCHAR* szPath)
{
	ASSERT(this);
	ASSERT(m_ppBase);

	// �Y�����郆�j�b�g������ (�d���w����l������)
	for (int n = 0; n < CWinFileSys::DrvMax; n++) {
		if (m_ppBase[n]) {
			if (m_ppBase[n]->isSameDrive(szPath)) {
				// �r�o
				if (nEvent & (SHCNE_MEDIAREMOVED | SHCNE_DRIVEREMOVED)) {
					m_ppBase[n]->SetEnable(FALSE);
				}

				// �}��
				if (nEvent & (SHCNE_MEDIAINSERTED | SHCNE_DRIVEADD)) {
					m_ppBase[n]->SetEnable(TRUE);
				}

				// �Y������L���b�V��������
				LockCache();
				EraseCache(n);
				UnlockCache();
			}
		}
	}
}

//---------------------------------------------------------------------------
//
//	Human68k�t���p�X������Ō�̗v�f�𕪗�
//
//---------------------------------------------------------------------------
const BYTE* FASTCALL CHostEntry::SeparatePath(const BYTE* szHuman)
{
	ASSERT(szHuman);

	DWORD nMax = 22;
	const BYTE* p = szHuman;

	BYTE c = *p;
	if (c != '/' && c != '\\') return NULL;	// �G���[: �s���ȃp�X��

	// �����񒆂̍Ō�� / �������� \ ����������B
	const BYTE* pCut = NULL;
	for (;;) {
		c = *p;
		if (c == '\0') break;
		if ((0x80 <= c && c <= 0x9F) || 0xE0 <= c) {	// �����ɂ� 0x81�`0x9F 0xE0�`0xEF
			p++;
			if (*p == '\0') break;
		}
		if (c == '/' || c == '\\') {
			if (pCut == p - 1) return NULL;	// 2�����A�����ċ�؂蕶������������G���[
			pCut = p;
		}
		p++;
	}

	// �����\���ǂ����m�F
	if (pCut == NULL) return NULL;			// ��������������������Ȃ��ꍇ�̓G���[
	//if (pCut == szHuman) return NULL;		// �擪(���[�g)�������ꍇ ���ʈ������Ȃ�
	//if (pCut[1] == '\0') return NULL;		// �t�@�C�����������Ȃ��ꍇ ���ʈ������Ȃ�
	if (strlen((char*)pCut + 1) > nMax) return NULL;	// �t�@�C������������������ꍇ�̓G���[

	return pCut;
}

//---------------------------------------------------------------------------
//
//	Human68k�t���p�X������擪�̗v�f�𕪗��E�R�s�[
//
//	Human68k�p�X�͕K�� / �ŊJ�n���邱�ƁB
//	�r�� / ��2�ȏ�A�����ďo�������ꍇ�̓G���[�Ƃ���B
//	������I�[�� / �����̏ꍇ�A��̕�����Ƃ��ď�������B�G���[�ɂ͂��Ȃ��B
//
//---------------------------------------------------------------------------
const BYTE* FASTCALL CHostEntry::SeparateCopyFilename(const BYTE* szHuman, BYTE* szBuffer)
{
	ASSERT(szHuman);
	ASSERT(szBuffer);

	DWORD nMax = 22;
	const BYTE* p = szHuman;

	BYTE c = *p;
	if (c != '/' && c != '\\') return NULL;	// �G���[: �s���ȃp�X��
	p++;

	// �t�@�C�������������
	BYTE* pWrite = szBuffer;
	DWORD i = 0;
	for (;;) {
		c = *p;								// �ꕶ���ǂݍ��� �i�߂�
		if (c == '\0') break;				// ������I�[�Ȃ�I��
		if (c == '/' || c == '\\') {
			if (pWrite == szBuffer) return NULL;	// �G���[: �p�X��؂蕶�����A�����Ă���
			break;	// �p�X�̋�؂��ǂ񂾂�I��
		}
		p++;

		if (i >= nMax) return NULL;	// �G���[: 1�o�C�g�ڂ��o�b�t�@�I�[�ɂ�����
		i++;								// �������ޑO�ɏI�[�`�F�b�N
		*pWrite++ = c;						// �ꕶ����������

		if ((0x80 <= c && c <= 0x9F) || 0xE0 <= c) {	// �����ɂ� 0x81�`0x9F 0xE0�`0xEF
			c = *p;							// �ꕶ���ǂݍ��� �i�߂�
			if (c < 0x40) return NULL;		// �G���[: �s����SJIS2�o�C�g��
			p++;

			if (i >= nMax) return NULL;	// �G���[: 2�o�C�g�ڂ��o�b�t�@�I�[�ɂ�����
			i++;							// �������ޑO�ɏI�[�`�F�b�N
			*pWrite++ = c;					// �ꕶ����������
		}
	}
	*pWrite = '\0';

	return p;
}

//===========================================================================
//
//	�t�@�C����������
//
//	Human68k���̃t�@�C���������Unicode�ŏ�������̂͐����L�c���B��
//	�����킯�ŁA�S��BYTE�ɕϊ����ď�������B�ϊ������̓f�B���N�g���G
//	���g���L���b�V�������ɒS���AWINDRV���͂��ׂăV�t�gJIS�݂̂ň�
//	����悤�ɂ���B
//	�܂��AHuman68k�����̂́A���S�Ƀx�[�X�p�X�w�肩��Ɨ�������B
//
//	�t�@�C�����������O�ɁA�f�B���N�g���G���g���̃L���b�V���𐶐�����B
//	�f�B���N�g���G���g���̐��������͍��R�X�g�̂��߁A�L���b�V���\�z��
//	���O��VM�X���b�h�̓�����J�n������B
//
//	�t�@�C��������3�����B���ׂ�CHostFiles::Find()�ŏ�������B
//	1. �p�X���̂݌��� �����̓f�B���N�g���̂�	_CHKDIR _CREATE
//	2. �p�X��+�t�@�C����+�����̌��� _OPEN
//	3. �p�X��+���C���h�J�[�h+�����̌��� _FILES _NFILES
//	�������ʂ́A�f�B���N�g���G���g�����Ƃ��ĕێ����Ă����B
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	�N���A
//
//---------------------------------------------------------------------------
void FASTCALL CHostFiles::Clear()
{
	ASSERT(this);

	m_nKey = 0;
#if 0
	// ��������: SetPath()�Őݒ肳��邽�ߕs�v
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
//	�p�X���E�t�@�C����������Ő���
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
//	Win32�������� (�p�X�� + �t�@�C����(�ȗ���) + ����)
//
//	���炩���ߑS�Ă�Human68k�p�p�����[�^��ݒ肵�Ă������ƁB
//
//---------------------------------------------------------------------------
BOOL FASTCALL CHostFiles::Find(CWindrv* ps, CHostEntry* pEntry, BOOL bRemove)
{
	ASSERT(this);
	ASSERT(pEntry);

	// �r������ �J�n
	pEntry->LockCache();

	// �p�X���l������уL���b�V���\�z
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
		return FALSE;	// �G���[: �L���b�V���\�z���s
	}

	// �t�@�C�������Ȃ���ΏI��
	if (m_nHumanWildcard == 0xFF) {
		_tcscpy(m_szWin32Result, pPath->GetWin32());
		_tcscat(m_szWin32Result, _T("\\"));
		pEntry->UnlockCache();
		return TRUE;	// ����I��: �p�X���̂�
	}

	// �t�@�C�����l��
	CHostFilename* pFilename;
	if (m_nHumanWildcard == 0) {
		pFilename = pPath->FindFilename(m_szHumanFilename, m_nHumanAttribute);
	} else {
		pFilename = pPath->FindFilenameWildcard(m_szHumanFilename, &m_findNext, m_nHumanAttribute);
	}
	if (pFilename == NULL) {
		pEntry->UnlockCache();
		return FALSE;	// �G���[: �t�@�C�������l���ł��܂���
	}

	// �f�B���N�g���폜�̏ꍇ�͂����Ńn���h�����J��������
	if (bRemove) pFilename->DeleteCache();

	// Human68k�t�@�C�����ۑ�
	memcpy(&m_dirEntry, pFilename->GetEntry(), sizeof(m_dirEntry));

	// Human68k�t�@�C�����ۑ�
	strcpy((char*)m_szHumanResult, (char*)pFilename->GetHuman());

	// Win32�t���p�X���ۑ�
	_tcscpy(m_szWin32Result, pPath->GetWin32());
	_tcscat(m_szWin32Result, _T("\\"));
	_tcscat(m_szWin32Result, pFilename->GetWin32());

	// �r������ �I��
	pEntry->UnlockCache();

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	Win32���Ƀt�@�C������ǉ�
//
//---------------------------------------------------------------------------
void FASTCALL CHostFiles::AddFilename()
{
	ASSERT(this);
	ASSERT(_tcslen(m_szWin32Result) + strlen((char*)m_szHumanFilename) < _MAX_PATH);

	// WARNING: Unicode���Ή��B������Unicode�̐��E�ɟZ�܂ꂽ���͂����ŕϊ����s�Ȃ�
	_tcscat(m_szWin32Result, (TCHAR*)m_szHumanFilename);
}

//===========================================================================
//
//	�t�@�C�������̈� �}�l�[�W��
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	�t�@�C�������̈� ������ (�N������)
//
//---------------------------------------------------------------------------
CHostFilesManager::CHostFilesManager()
{
}

//---------------------------------------------------------------------------
//
//	�t�@�C�������̈� �m�F (�I����)
//
//---------------------------------------------------------------------------
CHostFilesManager::~CHostFilesManager()
{
#ifdef _DEBUG
	// ���̂����݂��Ȃ����Ƃ��m�F
	ASSERT(m_cRingFiles.Next() == &m_cRingFiles);
	ASSERT(m_cRingFiles.Prev() == &m_cRingFiles);

	// �O�̂���(���ۂɂ͎g���Ȃ�)
	Clean();
#endif // _DEBUG
}

//---------------------------------------------------------------------------
//
//	�t�@�C�������̈� ������ (�h���C�o�g���ݎ�)
//
//---------------------------------------------------------------------------
void FASTCALL CHostFilesManager::Init()
{
	ASSERT(this);

	// ���̂����݂��Ȃ����Ƃ��m�F (�O�̂���)
	ASSERT(m_cRingFiles.Next() == &m_cRingFiles);
	ASSERT(m_cRingFiles.Prev() == &m_cRingFiles);

	// ���̂��m��
	for (int i = 0; i < XM6_HOST_FILES_MAX; i++) {
		CHostFiles* p = new CHostFiles;
		ASSERT(p);
		p->InsertTail(&m_cRingFiles);
	}
}

//---------------------------------------------------------------------------
//
//	�t�@�C�������̈� �J�� (�N���E���Z�b�g��)
//
//---------------------------------------------------------------------------
void FASTCALL CHostFilesManager::Clean()
{
	ASSERT(this);

	// ���̂��J��
	CHostFiles* p;
	while ((p = (CHostFiles*)m_cRingFiles.Next()) != &m_cRingFiles) {
		delete p;
	}
}

//---------------------------------------------------------------------------
//
//	�t�@�C�������̈� �m��
//
//---------------------------------------------------------------------------
CHostFiles* FASTCALL CHostFilesManager::Alloc(DWORD nKey)
{
	ASSERT(this);
	ASSERT(nKey != 0);

	// ������1��I��
	CHostFiles* p = (CHostFiles*)m_cRingFiles.Prev();

	// �Y���I�u�W�F�N�g�������O�擪�ֈړ�
	p->Insert(&m_cRingFiles);

	// �L�[��ݒ�
	p->SetKey(nKey);

	return p;
}

//---------------------------------------------------------------------------
//
//	�t�@�C�������̈� ����
//
//---------------------------------------------------------------------------
CHostFiles* FASTCALL CHostFilesManager::Search(DWORD nKey)
{
	ASSERT(this);
	ASSERT(nKey != 0);

	// �Y������I�u�W�F�N�g������
	for (CHostFiles* p = (CHostFiles*)m_cRingFiles.Next();
		 p != &m_cRingFiles; p = (CHostFiles*)p->Next()) {
		if (p->isSameKey(nKey)) {
			// �Y���I�u�W�F�N�g�������O�擪�ֈړ�
			p->Insert(&m_cRingFiles);
			return p;
		}
	}

	return NULL;
}

//---------------------------------------------------------------------------
//
//	�t�@�C�������̈� �J��
//
//---------------------------------------------------------------------------
void FASTCALL CHostFilesManager::Free(CHostFiles* p)
{
	ASSERT(this);
	ASSERT(p);

	// ���̂̏����� (�ė��p���邽�ߗ̈�J���͍s�Ȃ�Ȃ�)
	p->Free();

	// �Y���I�u�W�F�N�g�������O�����ֈړ�
	p->InsertTail(&m_cRingFiles);
}

//===========================================================================
//
//	FCB����
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	�N���A
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
//	�t�@�C���I�[�v�����[�h�̐ݒ�
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
//	�t�@�C�����̐ݒ�
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
//	�t�@�C���쐬
//
//---------------------------------------------------------------------------
HANDLE FASTCALL CHostFcb::Create(DWORD nHumanAttribute, BOOL bForce)
{
	ASSERT(this);
	ASSERT(_tcslen(m_szFilename) > 0);
	ASSERT(m_hFile == INVALID_HANDLE_VALUE);

	// ��������
	DWORD nAttribute = 0;
	if ((nHumanAttribute & Human68k::AT_DIRECTORY) != 0) return INVALID_HANDLE_VALUE;
	if ((nHumanAttribute & Human68k::AT_SYSTEM) != 0) nAttribute |= FILE_ATTRIBUTE_SYSTEM;
	if ((nHumanAttribute & Human68k::AT_HIDDEN) != 0) nAttribute |= FILE_ATTRIBUTE_HIDDEN;
	if ((nHumanAttribute & Human68k::AT_READONLY) != 0) nAttribute |= FILE_ATTRIBUTE_READONLY;
	if (nAttribute == 0) nAttribute = FILE_ATTRIBUTE_NORMAL;

	DWORD nShare = FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE;
	DWORD nCreation = bForce ? CREATE_ALWAYS : CREATE_NEW;

	// �t�@�C���I�[�v��
	m_hFile = ::CreateFile(m_szFilename, m_nMode, nShare, NULL, nCreation, nAttribute, NULL);
	return m_hFile;
}

//---------------------------------------------------------------------------
//
//	�t�@�C���I�[�v��/�n���h���l��
//
//---------------------------------------------------------------------------
HANDLE FASTCALL CHostFcb::Open()
{
	ASSERT(this);
	ASSERT(_tcslen(m_szFilename) > 0);

	// �t�@�C���I�[�v��
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
//	�t�@�C���ǂݍ���
//
//	0�o�C�g�ǂݍ��݂ł����퓮��Ƃ���B
//	�G���[�̎��� -1 ��Ԃ��B
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
//	�t�@�C����������
//
//	0�o�C�g�������݂ł����퓮��Ƃ���B
//	�G���[�̎��� -1 ��Ԃ��B
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
//	�t�@�C���|�C���^�ݒ�
//
//	�G���[�̎��� -1 ��Ԃ��B
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
//	�t�@�C�������ݒ�
//
//	�G���[�̎��� -1 ��Ԃ��B
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
//	�t�@�C���N���[�Y
//
//---------------------------------------------------------------------------
BOOL FASTCALL CHostFcb::Close()
{
	ASSERT(this);

	BOOL bResult = TRUE;

	// �t�@�C���N���[�Y
	if (m_hFile != INVALID_HANDLE_VALUE) {
		bResult = ::CloseHandle(m_hFile);
		// �G���[������Close��Free(�����ōēxClose) �Ƃ������������̂�
		// ��d��Close���Ȃ��悤�ɂ�����Ɛݒ肵�Ă���
		m_hFile = INVALID_HANDLE_VALUE;
	}

	return bResult;
}

//===========================================================================
//
//	FCB���� �}�l�[�W��
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	FCB����̈� ������ (�N������)
//
//---------------------------------------------------------------------------
CHostFcbManager::CHostFcbManager()
{
}

//---------------------------------------------------------------------------
//
//	FCB����̈� �m�F (�I����)
//
//---------------------------------------------------------------------------
CHostFcbManager::~CHostFcbManager()
{
#ifdef _DEBUG
	// ���̂����݂��Ȃ����Ƃ��m�F
	ASSERT(m_cRingFcb.Next() == &m_cRingFcb);
	ASSERT(m_cRingFcb.Prev() == &m_cRingFcb);

	// �O�̂���(���ۂɂ͎g���Ȃ�)
	Clean();
#endif // _DEBUG
}

//---------------------------------------------------------------------------
//
//	FCB����̈� ������ (�h���C�o�g���ݎ�)
//
//---------------------------------------------------------------------------
void FASTCALL CHostFcbManager::Init()
{
	ASSERT(this);

	// ���̂����݂��Ȃ����Ƃ��m�F
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
//	FCB����̈� �J�� (�N���E���Z�b�g��)
//
//---------------------------------------------------------------------------
void CHostFcbManager::Clean()
{
	ASSERT(this);

	// ���̂��J��
	CHostFcb* p;
	while ((p = (CHostFcb*)m_cRingFcb.Next()) != &m_cRingFcb) {
		delete p;
	}
}

//---------------------------------------------------------------------------
//
//	FCB����̈� �m��
//
//---------------------------------------------------------------------------
CHostFcb* FASTCALL CHostFcbManager::Alloc(DWORD nKey)
{
	ASSERT(this);
	ASSERT(nKey != 0);

	// ������1��I��
	CHostFcb* p = (CHostFcb*)m_cRingFcb.Prev();

	// �g�p���Ȃ�G���[
	if (p->isSameKey(0) == FALSE) return NULL;

	// �Y���I�u�W�F�N�g�������O�擪�ֈړ�
	p->Insert(&m_cRingFcb);

	// �L�[��ݒ�
	p->SetKey(nKey);

	return p;
}

//---------------------------------------------------------------------------
//
//	FCB����̈� ����
//
//---------------------------------------------------------------------------
CHostFcb* FASTCALL CHostFcbManager::Search(DWORD nKey)
{
	ASSERT(this);
	ASSERT(nKey != 0);

	// �Y������I�u�W�F�N�g������
	for (CHostFcb* p = (CHostFcb*)m_cRingFcb.Next();
		 p != &m_cRingFcb; p = (CHostFcb*)p->Next()) {
		if (p->isSameKey(nKey)) {
			// �Y���I�u�W�F�N�g�������O�擪�ֈړ�
			p->Insert(&m_cRingFcb);
			return p;
		}
	}

	return NULL;
}

//---------------------------------------------------------------------------
//
//	FCB����̈� �J��
//
//---------------------------------------------------------------------------
void FASTCALL CHostFcbManager::Free(CHostFcb* p)
{
	ASSERT(this);
	ASSERT(p);

	// ���̂̏����� (�ė��p���邽�ߗ̈�J���͍s�Ȃ�Ȃ�)
	p->Free();

	// �Y���I�u�W�F�N�g�������O�����ֈړ�
	p->InsertTail(&m_cRingFcb);
}

//===========================================================================
//
//	Windows�t�@�C���V�X�e��
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	�R���X�g���N�^
//
//---------------------------------------------------------------------------
CWinFileSys::CWinFileSys()
{
	// �h���C�u�I�u�W�F�N�g������
	for (int n = 0; n < DrvMax; n++) {
		m_pDrv[n] = NULL;
	}

	// �R���t�B�O�f�[�^������
	m_bResume = FALSE;
	m_nDrives = 0;

	for (int n = 0; n < DrvMax; n++) {
		m_nFlag[n] = 0;
		m_szBase[n][0] = _T('\0');
	}

	// TwentyOne�I�v�V�����Ď�������
	m_nKernel = 0;
	m_nKernelSearch = 0;

	// ����t���O������
	m_nOptionDefault = 0;
	m_nOption = 0;
	CHostFilename::SetOption(0);
	CHostPath::SetOption(0);
}

//---------------------------------------------------------------------------
//
//	�f�X�g���N�^
//
//---------------------------------------------------------------------------
CWinFileSys::~CWinFileSys()
{
#ifdef _DEBUG
	int nDrv;

	// �I�u�W�F�N�g�m�F
	for (nDrv=0; nDrv<DrvMax; nDrv++) {
		// ���݂��Ȃ����Ƃ��m�F
		ASSERT(!m_pDrv[nDrv]);

		// �O�̂���(���ۂɂ͎g���Ȃ�)
		if (m_pDrv[nDrv]) {
			delete m_pDrv[nDrv];
			m_pDrv[nDrv] = NULL;
		}
	}
#endif // _DEBUG
}

//---------------------------------------------------------------------------
//
//	�ݒ�K�p
//
//---------------------------------------------------------------------------
void FASTCALL CWinFileSys::ApplyCfg(const Config* pConfig)
{
	ASSERT(this);
	ASSERT(pConfig);

	// �R���t�B�O�f�[�^�K�p
	m_bResume = pConfig->host_resume;
	m_nDrives = pConfig->host_drives;

	for (int n = 0; n < DrvMax; n++) {
		m_nFlag[n] = pConfig->host_flag[n];
		ASSERT(_tcslen(pConfig->host_path[n]) < _MAX_PATH);
		_tcscpy(m_szBase[n], pConfig->host_path[n]);
	}

	// ����t���O�ݒ�
	m_nOptionDefault = pConfig->host_option;
}

//---------------------------------------------------------------------------
//
//	$40 - ������
//
//---------------------------------------------------------------------------
DWORD FASTCALL CWinFileSys::Init(CWindrv* ps, DWORD nDriveMax, const BYTE* pOption)
{
	ASSERT(this);
	ASSERT(nDriveMax < 26);

	// VM�X���b�h�̓�����J�n
	ps->Ready();

	// �G���[���[�h�ݒ�
	::SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOOPENFILEERRORBOX);

	// �t�@�C�������̈� ������ (�h���C�o�g���ݎ�)
	m_cFiles.Init();

	// FCB����̈� ������ (�h���C�o�g���ݎ�)
	m_cFcb.Init();

	// �f�B���N�g���G���g�� ������ (�h���C�o�g���ݎ�)
	m_cEntry.Init(m_pDrv);

	// �I�v�V����������
	InitOption(pOption);

	// �S�h���C�u�X�L�����̗L���𔻒�
	DWORD nConfigDrives = m_nDrives;
	if (m_bResume == FALSE) {
		// �S�R���t�B�O����
		for (DWORD i = 0; i < DrvMax; i++) {
			m_nFlag[i] = 0;
			m_szBase[i][0] = _T('\0');
		}

		// �S�h���C�u�X�L����
		nConfigDrives = 0;
		DWORD nBits = ::GetLogicalDrives();
		for (DWORD n = 0; n < 26; n++) {
			// �r�b�g�`�F�b�N
			if (nBits & 1) {
				// �x�[�X�p�X�ݒ�
				_stprintf(m_szBase[nConfigDrives], _T("%c:\\"), 'A' + n);

				// ����t���O�ݒ�
				m_nFlag[nConfigDrives] = 0;

				// ���̃h���C�u��
				nConfigDrives++;

				// �ő吔�ɒB���Ă���ΏI��
				if (nConfigDrives >= DrvMax) break;
			}

			// ���̃r�b�g��
			nBits >>= 1;
		}
	}

	// �t�@�C���V�X�e����o�^
	DWORD nDrives = 0;
	for (DWORD n = 0; n < nConfigDrives; n++) {	// �S�Ă̗L���ȃR���t�B�O�f�[�^�𒲍�
		// �x�[�X�p�X�����݂��Ȃ��ꍇ�͖����ȃf�o�C�X�Ƃ݂Ȃ�
		if (m_szBase[n][0] == _T('\0')) continue;

		// ����ȏ�o�^�ł��Ȃ���ΏI�� (nDriveMax��0�̏ꍇ�͉����o�^���Ȃ�)
		if (nDrives >= nDriveMax) break;

		// �z�X�g�t�@�C���V�X�e����1����
		ASSERT(m_pDrv[nDrives] == NULL);
		m_pDrv[nDrives] = new CWinFileDrv;
		ASSERT(m_pDrv[nDrives]);

		// ������
		m_pDrv[nDrives]->Init(m_szBase[n], m_nFlag[n]);

		// ���̃h���C�u��
		nDrives++;
	}

	// �o�^�����h���C�u����Ԃ�
	return nDrives;
}

//---------------------------------------------------------------------------
//
//	���Z�b�g(�S�N���[�Y)
//
//---------------------------------------------------------------------------
void FASTCALL CWinFileSys::Reset()
{
	int nDrv;

	ASSERT(this);

	// ���z�Z�N�^�̈揉����
	m_nHostSectorCount = 0;
	memset(m_nHostSectorBuffer, 0, sizeof(m_nHostSectorBuffer));

	// �t�@�C�������̈� �J�� (�N���E���Z�b�g��)
	m_cFiles.Clean();

	// FCB����̈� �J�� (�N���E���Z�b�g��)
	m_cFcb.Clean();

	// �f�B���N�g���G���g�� �J�� (�N���E���Z�b�g��)
	m_cEntry.Clean();

	// �I�u�W�F�N�g�폜
	for (nDrv=0; nDrv<DrvMax; nDrv++) {
		if (m_pDrv[nDrv]) {
			delete m_pDrv[nDrv];
			m_pDrv[nDrv] = NULL;
		}
	}

	// TwentyOne�I�v�V�����Ď�������
	m_nKernel = 0;
	m_nKernelSearch = 0;
}

//---------------------------------------------------------------------------
//
//	$41 - �f�B���N�g���`�F�b�N
//
//---------------------------------------------------------------------------
int FASTCALL CWinFileSys::CheckDir(CWindrv* ps, const Human68k::namests_t* pNamests)
{
	ASSERT(this);
	ASSERT(pNamests);

	// ����ɕK�����̃R�}���h�����s����邽�߁A���f�B�A�`�F�b�N�͕s�v
	// �����Ń`�F�b�N�����mint�̃h���C�u�ύX�O�ɖ��ʂȃf�B�X�N�A�N�Z�X���������Ă��܂�
	//if (m_cEntry.isMediaOffline(ps)) return FS_FATAL_MEDIAOFFLINE;

#ifdef XM6_HOST_UPDATE_BY_SEQUENCE
	// ����̃��f�B�A�����`�F�b�N�R�}���h��L���ɂ��邽�߃t���O�𗧂Ă�
	m_cEntry.SetMediaUpdate(ps, FALSE);
#endif // XM6_HOST_UPDATE_BY_SEQUENCE

	// �p�X������
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
//	$42 - �f�B���N�g���쐬
//
//---------------------------------------------------------------------------
int FASTCALL CWinFileSys::MakeDir(CWindrv* ps, const Human68k::namests_t* pNamests)
{
	ASSERT(this);
	ASSERT(pNamests);

	// ���f�B�A�`�F�b�N
	if (m_cEntry.isMediaOffline(ps)) return FS_FATAL_MEDIAOFFLINE;

	// �������݋֎~�`�F�b�N
	if (m_cEntry.isWriteProtect(ps)) return FS_FATAL_WRITEPROTECT;

	// �p�X������
	CHostFiles f;
	DWORD nUnit = ps->GetUnit();
	f.SetPath(nUnit, pNamests);
	f.SetPathOnly();
	if (f.Find(ps, &m_cEntry) == FALSE) return FS_INVALIDPATH;
	f.AddFilename();

	// �f�B���N�g���쐬
	BOOL bResult = ::CreateDirectory(f.GetPath(), NULL);
	if (bResult == FALSE) return FS_INVALIDPATH;

	return 0;
}

//---------------------------------------------------------------------------
//
//	$43 - �f�B���N�g���폜
//
//---------------------------------------------------------------------------
int FASTCALL CWinFileSys::RemoveDir(CWindrv* ps, const Human68k::namests_t* pNamests)
{
	ASSERT(this);
	ASSERT(pNamests);

	// ���f�B�A�`�F�b�N
	if (m_cEntry.isMediaOffline(ps)) return FS_FATAL_MEDIAOFFLINE;

	// �������݋֎~�`�F�b�N
	if (m_cEntry.isWriteProtect(ps)) return FS_FATAL_WRITEPROTECT;

	// �p�X������
	CHostFiles f;
	DWORD nUnit = ps->GetUnit();
	f.SetPath(nUnit, pNamests);

	// �f�B���N�g���m�F
	f.SetAttribute(Human68k::AT_DIRECTORY);
	if (f.Find(ps, &m_cEntry) == FALSE) return FS_DIRNOTFND;

	// �f�B���N�g���폜
	if ((m_nOption & WINDRV_OPT_REMOVE) == 0) {
		BOOL bResult = ::RemoveDirectory(f.GetPath());
		if (bResult == FALSE) return FS_CANTDELETE;
	} else {
		// �f�B���N�g���������󂩂ǂ����m�F
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

		// WARNING: Unicode�Ή����v�C��
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

	// �L���b�V���폜
	f.Find(ps, &m_cEntry, TRUE);

	return 0;
}

//---------------------------------------------------------------------------
//
//	$44 - �t�@�C�����ύX
//
//---------------------------------------------------------------------------
int FASTCALL CWinFileSys::Rename(CWindrv* ps, const Human68k::namests_t* pNamests, const Human68k::namests_t* pNamestsNew)
{
	ASSERT(this);
	ASSERT(pNamests);

	// ���f�B�A�`�F�b�N
	if (m_cEntry.isMediaOffline(ps)) return FS_FATAL_MEDIAOFFLINE;

	// �������݋֎~�`�F�b�N
	if (m_cEntry.isWriteProtect(ps)) return FS_FATAL_WRITEPROTECT;

	// �p�X������
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

	// �t�@�C�����ύX
	BOOL bResult = ::MoveFile(f.GetPath(), fNew.GetPath());
	if (bResult == FALSE) return FS_FILENOTFND;

	return 0;
}

//---------------------------------------------------------------------------
//
//	$45 - �t�@�C���폜
//
//---------------------------------------------------------------------------
int FASTCALL CWinFileSys::Delete(CWindrv* ps, const Human68k::namests_t* pNamests)
{
	ASSERT(this);
	ASSERT(pNamests);

	// ���f�B�A�`�F�b�N
	if (m_cEntry.isMediaOffline(ps)) return FS_FATAL_MEDIAOFFLINE;

	// �������݋֎~�`�F�b�N
	if (m_cEntry.isWriteProtect(ps)) return FS_FATAL_WRITEPROTECT;

	// �p�X������
	CHostFiles f;
	DWORD nUnit = ps->GetUnit();
	f.SetPath(nUnit, pNamests);

	// �t�@�C���m�F
	if (f.Find(ps, &m_cEntry) == FALSE) return FS_FILENOTFND;

	// �t�@�C���폜
	if ((m_nOption & WINDRV_OPT_REMOVE) == 0) {
		BOOL bResult = ::DeleteFile(f.GetPath());
		if (bResult == FALSE) return FS_CANTDELETE;
	} else {
		// WARNING: Unicode�Ή����v�C��
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
//	$46 - �t�@�C�������擾/�ݒ�
//
//---------------------------------------------------------------------------
int FASTCALL CWinFileSys::Attribute(CWindrv* ps, const Human68k::namests_t* pNamests, DWORD nHumanAttribute)
{
	ASSERT(this);
	ASSERT(pNamests);

	// ���f�B�A�`�F�b�N
	if (m_cEntry.isMediaOffline(ps)) return FS_FATAL_MEDIAOFFLINE;

	// �p�X������
	CHostFiles f;
	DWORD nUnit = ps->GetUnit();
	f.SetPath(nUnit, pNamests);
	f.SetAttribute(Human68k::AT_ALL);
	if (f.Find(ps, &m_cEntry) == FALSE) return FS_FILENOTFND;

	// �����擾�Ȃ�I��
	if (nHumanAttribute == 0xFF) {
		return f.GetAttribute();
	}

	// �������݋֎~�`�F�b�N
	if (m_cEntry.isWriteProtect(ps)) return FS_FATAL_WRITEPROTECT;

#if 0
	if (f.GetAttribute() & Human68k::AT_DIRECTORY) {
		if ((nHumanAttribute & Human68k::AT_DIRECTORY) == 0) {
			OutputDebugString("Warning: �f�B���N�g���̑������t�@�C���ɕύX�ł��܂���\r\n");
		}
	}

	if ((f.GetAttribute() & Human68k::AT_DIRECTORY) == 0) {
		if (nHumanAttribute & Human68k::AT_DIRECTORY) {
			OutputDebugString("Warning: �t�@�C���̑������f�B���N�g���ɕύX�ł��܂���\r\n");
		}
	}
#endif

	// ��������
	DWORD nAttribute = 0;
	if ((nHumanAttribute & Human68k::AT_SYSTEM) != 0) nAttribute |= FILE_ATTRIBUTE_SYSTEM;
	if ((nHumanAttribute & Human68k::AT_HIDDEN) != 0) nAttribute |= FILE_ATTRIBUTE_HIDDEN;
	if ((nHumanAttribute & Human68k::AT_READONLY) != 0) nAttribute |= FILE_ATTRIBUTE_READONLY;
	if (nAttribute == 0) nAttribute = FILE_ATTRIBUTE_NORMAL;

	// �����ݒ�
	BOOL bResult = ::SetFileAttributes(f.GetPath(), nAttribute);
	if (bResult == FALSE) return FS_FILENOTFND;

	// �ύX��̑����擾
	if (f.Find(ps, &m_cEntry) == FALSE) return FS_FILENOTFND;
	return f.GetAttribute();
}

//---------------------------------------------------------------------------
//
//	$47 - �t�@�C������(First)
//
//---------------------------------------------------------------------------
int FASTCALL CWinFileSys::Files(CWindrv* ps, const Human68k::namests_t* pNamests, DWORD nKey, Human68k::files_t* pFiles)
{
	ASSERT(this);
	ASSERT(pNamests);
	ASSERT(nKey);
	ASSERT(pFiles);

	// ���ɓ����L�[�����̈悪����ΊJ�����Ă���
	CHostFiles* pHostFiles = m_cFiles.Search(nKey);
	if (pHostFiles != NULL) {
		m_cFiles.Free(pHostFiles);
	}

	// �{�����[�����x���̏ꍇ
	if (pFiles->fatr == Human68k::AT_VOLUME) {
		// �o�b�t�@���m�ۂ����A�����Ȃ茋�ʂ�Ԃ�
		if (FilesVolume(ps, pFiles) == FALSE) {
			return FS_FILENOTFND;
		}
		return 0;
	}

	// ���f�B�A�`�F�b�N
	if (m_cEntry.isMediaOffline(ps)) return FS_FATAL_MEDIAOFFLINE;

	// �o�b�t�@�m��
	pHostFiles = m_cFiles.Alloc(nKey);
	if (pHostFiles == NULL) {
		return FS_OUTOFMEM;
	}

	// �f�B���N�g���`�F�b�N
	DWORD nUnit = ps->GetUnit();
	pHostFiles->SetPath(nUnit, pNamests);
	if (pHostFiles->isRootPath() == FALSE) {
		pHostFiles->SetPathOnly();
		if (pHostFiles->Find(ps, &m_cEntry) == FALSE) {
			m_cFiles.Free(pHostFiles);
			return FS_DIRNOTFND;
		}
	}

	// ���C���h�J�[�h�g�p�\�ɐݒ�
	pHostFiles->SetPathWildcard();
	pHostFiles->SetAttribute(pFiles->fatr);

	// �t�@�C������
	if (pHostFiles->Find(ps, &m_cEntry) == FALSE) {
		m_cFiles.Free(pHostFiles);
		return FS_FILENOTFND;
	}

	// �������ʂ��i�[
	pFiles->attr = (BYTE)pHostFiles->GetAttribute();
	pFiles->date = pHostFiles->GetDate();
	pFiles->time = pHostFiles->GetTime();
	pFiles->size = pHostFiles->GetSize();
	strcpy((char*)pFiles->full, (char*)pHostFiles->GetHumanResult());

	// �[���f�B���N�g���G���g�����w��
	pFiles->sector = nKey;
	pFiles->offset = 0;

	// �t�@�C�����Ƀ��C���h�J�[�h���Ȃ���΁A���̎��_�Ńo�b�t�@���J����
	if (pNamests->wildcard == 0) {
		// �
		//m_cFiles.Free(pHostFiles);
	}

	return 0;
}

//---------------------------------------------------------------------------
//
//	$48 - �t�@�C������(Next)
//
//---------------------------------------------------------------------------
int FASTCALL CWinFileSys::NFiles(CWindrv* ps, DWORD nKey, Human68k::files_t* pFiles)
{
	ASSERT(this);
	ASSERT(nKey);
	ASSERT(pFiles);

	// ���f�B�A�`�F�b�N
	if (m_cEntry.isMediaOffline(ps)) return FS_FATAL_MEDIAOFFLINE;

	// �o�b�t�@����
	CHostFiles* pHostFiles = m_cFiles.Search(nKey);
	if (pHostFiles == NULL) {
		return FS_FILENOTFND;
	}

	// �t�@�C������
	if (pHostFiles->Find(ps, &m_cEntry) == FALSE) {
		m_cFiles.Free(pHostFiles);
		return FS_FILENOTFND;
	}

	// �������ʂ��i�[
	pFiles->attr = (BYTE)pHostFiles->GetAttribute();
	pFiles->date = pHostFiles->GetDate();
	pFiles->time = pHostFiles->GetTime();
	pFiles->size = pHostFiles->GetSize();
	strcpy((char*)pFiles->full, (char*)pHostFiles->GetHumanResult());

	return 0;
}

//---------------------------------------------------------------------------
//
//	$49 - �t�@�C���V�K�쐬
//
//---------------------------------------------------------------------------
int FASTCALL CWinFileSys::Create(CWindrv* ps, const Human68k::namests_t* pNamests, DWORD nKey, Human68k::fcb_t* pFcb, DWORD nHumanAttribute, BOOL bForce)
{
	ASSERT(this);
	ASSERT(pNamests);
	ASSERT(nKey);
	ASSERT(pFcb);

	// ���f�B�A�`�F�b�N
	if (m_cEntry.isMediaOffline(ps)) return FS_FATAL_MEDIAOFFLINE;

	// �������݋֎~�`�F�b�N
	if (m_cEntry.isWriteProtect(ps)) return FS_FATAL_WRITEPROTECT;

	// ���ɓ����L�[�����̈悪����΃G���[�Ƃ���
	CHostFcb* pHostFcb = m_cFcb.Search(nKey);
	if (pHostFcb != NULL) return FS_FILEEXIST;

	// �t�@�C���㏑���`�F�b�N
	CHostFiles f;
	DWORD nUnit = ps->GetUnit();
	f.SetPath(nUnit, pNamests);

	// �p�X������
	f.SetPathOnly();
	if (f.Find(ps, &m_cEntry) == FALSE) return FS_INVALIDPATH;
	f.AddFilename();

	// �p�X���ۑ�
	pHostFcb = m_cFcb.Alloc(nKey);
	if (pHostFcb == NULL) return FS_OUTOFMEM;
	pHostFcb->SetFilename(f.GetPath());

	// �I�[�v�����[�h�ݒ�
	pFcb->mode = (pFcb->mode & ~0x0F) | Human68k::OP_READWRITE;
	if (pHostFcb->SetOpenMode(pFcb->mode) == FALSE) {
		m_cFcb.Free(pHostFcb);
		return FS_ILLEGALMOD;
	}

	// �t�@�C���쐬
	HANDLE hFile = pHostFcb->Create(nHumanAttribute, bForce);
	if (hFile == INVALID_HANDLE_VALUE) {
		m_cFcb.Free(pHostFcb);
		return FS_FILEEXIST;
	}

#ifdef XM6_HOST_STRICT_CLOSE
	// ��������
	pHostFcb->Close();
#endif // XM6_HOST_STRICT_CLOSE

	return 0;
}

//---------------------------------------------------------------------------
//
//	$4A - �t�@�C���I�[�v��
//
//---------------------------------------------------------------------------
int FASTCALL CWinFileSys::Open(CWindrv* ps, const Human68k::namests_t* pNamests, DWORD nKey, Human68k::fcb_t* pFcb)
{
	ASSERT(this);
	ASSERT(pNamests);
	ASSERT(nKey);
	ASSERT(pFcb);

	// ���f�B�A�`�F�b�N
	if (m_cEntry.isMediaOffline(ps)) return FS_FATAL_MEDIAOFFLINE;

	// �������݋֎~�`�F�b�N
	switch (pFcb->mode) {
	case Human68k::OP_WRITE:
	case Human68k::OP_READWRITE:
		if (m_cEntry.isWriteProtect(ps)) return FS_FATAL_WRITEPROTECT;
	}

	// ���ɓ����L�[�����̈悪����΃G���[�Ƃ���
	CHostFcb* pHostFcb = m_cFcb.Search(nKey);
	if (pHostFcb != NULL) return FS_FILEEXIST;

	// �p�X������
	CHostFiles f;
	DWORD nUnit = ps->GetUnit();
	f.SetPath(nUnit, pNamests);

	// �t�@�C�����擾
	if (f.Find(ps, &m_cEntry) == FALSE) return FS_FILENOTFND;

	// �^�C���X�^���v
	pFcb->date = f.GetDate();
	pFcb->time = f.GetTime();

	// �t�@�C���T�C�Y
	pFcb->size = f.GetSize();

	// �p�X���ۑ�
	pHostFcb = m_cFcb.Alloc(nKey);
	if (pHostFcb == NULL) return FS_OUTOFMEM;
	pHostFcb->SetFilename(f.GetPath());

	// �I�[�v�����[�h�ݒ�
	if (pHostFcb->SetOpenMode(pFcb->mode) == FALSE) {
		m_cFcb.Free(pHostFcb);
		return FS_ILLEGALMOD;
	}

	// �t�@�C���I�[�v��
	HANDLE hFile = pHostFcb->Open();
	if (hFile == INVALID_HANDLE_VALUE) {
		m_cFcb.Free(pHostFcb);
		return FS_INVALIDPATH;
	}

#ifdef XM6_HOST_STRICT_CLOSE
	// ��������
	pHostFcb->Close();
#endif // XM6_HOST_STRICT_CLOSE

	return 0;
}

//---------------------------------------------------------------------------
//
//	$4B - �t�@�C���N���[�Y
//
//---------------------------------------------------------------------------
int FASTCALL CWinFileSys::Close(CWindrv* ps, DWORD nKey, Human68k::fcb_t* pFcb)
{
	ASSERT(this);
	ASSERT(nKey);
	ASSERT(pFcb);

	// ���f�B�A�`�F�b�N
	if (m_cEntry.isMediaOffline(ps)) return FS_FATAL_MEDIAOFFLINE;

	// ���ɓ����L�[�����̈悪�Ȃ���΃G���[�Ƃ���
	CHostFcb* pHostFcb = m_cFcb.Search(nKey);
	if (pHostFcb == NULL) return FS_INVALIDPRM;

	// �t�@�C���N���[�Y�Ɨ̈�J��
	//pHostFcb->Close();	// Free���Ɏ������s�����̂ŕs�v
	m_cFcb.Free(pHostFcb);

	return 0;
}

//---------------------------------------------------------------------------
//
//	$4C - �t�@�C���ǂݍ���
//
//	0�o�C�g�ǂݍ��݂ł�����I������B
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

	// ���f�B�A�`�F�b�N
	if (m_cEntry.isMediaOffline(ps)) return FS_FATAL_MEDIAOFFLINE;

	// ���ɓ����L�[�����̈悪�Ȃ���΃G���[�Ƃ���
	CHostFcb* pHostFcb = m_cFcb.Search(nKey);
	if (pHostFcb == NULL) return FS_NOTOPENED;

	// �t�@�C���I�[�v��/�n���h���l��
	HANDLE hFile = pHostFcb->Open();
	if (hFile == INVALID_HANDLE_VALUE) {
		m_cFcb.Free(pHostFcb);
		return FS_NOTOPENED;
	}

	DWORD nResult;
#ifdef XM6_HOST_STRICT_CLOSE
	// �t�@�C���|�C���^����
	nResult = pHostFcb->SetFilePointer(pFcb->fileptr);
	if (nResult == (DWORD)-1) {
		m_cFcb.Free(pHostFcb);
		return FS_CANTSEEK;
	}
#endif // XM6_HOST_STRICT_CLOSE

	// �t�@�C���ǂݍ���
	DWORD nTotal = 0;
	BYTE chBuffer[XM6_HOST_FILE_BUFFER_READ];
	for (DWORD nOffset = 0; nOffset < nSize; nOffset += XM6_HOST_FILE_BUFFER_READ) {
		// �����T�C�Y���傫���ꍇ��VM�X���b�h�̓�����J�n������
		if (nOffset == XM6_HOST_FILE_BUFFER_READ) ps->Ready();

		// �T�C�Y����
		DWORD n = nSize - nOffset;
		if (n > XM6_HOST_FILE_BUFFER_READ) n = XM6_HOST_FILE_BUFFER_READ;

		// �����ǂݍ���
		nResult = pHostFcb->ReadFile(chBuffer, n);
		if (nResult == (DWORD)-1) {
			m_cFcb.Free(pHostFcb);
			return FS_INVALIDFUNC;
		}

		ps->LockXM();

#if 0
		// 8�r�b�g�P�ʂŃf�[�^�]��
		for (DWORD i = 0; i < nResult; i++) pMemory->WriteByte(nAddress++, chBuffer[i]);
#else
		// �擪����A�h���X�Ȃ�ŏ���1�o�C�g�]��
		BYTE* pBuffer = chBuffer;
		DWORD nEnd = nAddress + nResult;
		if (nAddress < nEnd && (nAddress & 1) != 0)
			pMemory->WriteByte(nAddress++, *pBuffer++);

		// 16�r�b�g�P�ʂŃf�[�^�]��
		DWORD nEndWord = nEnd & ~1;
		while (nAddress < nEndWord) {
			DWORD nData = (*pBuffer++)<<8;
			nData |= *pBuffer++;
			pMemory->WriteWord(nAddress, nData);
			nAddress += 2;
		}

		// �f�[�^���c���Ă����(�����������A�h���X�Ȃ̂�)�Ō��1�o�C�g�]��
		if (nAddress < nEnd) pMemory->WriteByte(nAddress++, *pBuffer);
#endif

		ps->UnlockXM();

		// �T�C�Y�W�v
		nTotal += nResult;

		// �t�@�C���I�[�Ȃ�I��
		if (nResult != n) break;
	}

	// �t�@�C���|�C���^�ۑ�
	pFcb->fileptr += nTotal;

#ifdef XM6_HOST_STRICT_CLOSE
	// ��������
	pHostFcb->Close();
#endif // XM6_HOST_STRICT_CLOSE

	return nTotal;
}

//---------------------------------------------------------------------------
//
//	$4D - �t�@�C����������
//
//	0�o�C�g�������݂ł�����I������B
//	WARNING: �o�X�G���[����������A�h���X���w�肵���ꍇ�A���@�Ɠ��삪�قȂ�\������
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

	// ���f�B�A�`�F�b�N
	if (m_cEntry.isMediaOffline(ps)) return FS_FATAL_MEDIAOFFLINE;

	// ���ɓ����L�[�����̈悪�Ȃ���΃G���[�Ƃ���
	CHostFcb* pHostFcb = m_cFcb.Search(nKey);
	if (pHostFcb == NULL) return FS_NOTOPENED;

	// �t�@�C���I�[�v��/�n���h���l��
	HANDLE hFile = pHostFcb->Open();
	if (hFile == INVALID_HANDLE_VALUE) {
		m_cFcb.Free(pHostFcb);
		return FS_NOTOPENED;
	}

	DWORD nResult;
#ifdef XM6_HOST_STRICT_CLOSE
	// �t�@�C���|�C���^����
	nResult = pHostFcb->SetFilePointer(pFcb->fileptr);
	if (nResult == (DWORD)-1) {
		m_cFcb.Free(pHostFcb);
		return FS_CANTSEEK;
	}
#endif // XM6_HOST_STRICT_CLOSE

	// �t�@�C����������
	DWORD nTotal = 0;
	BYTE chBuffer[XM6_HOST_FILE_BUFFER_WRITE];
	for (DWORD nOffset = 0; nOffset < nSize; nOffset += XM6_HOST_FILE_BUFFER_WRITE) {
		// �����T�C�Y���傫���ꍇ��VM�X���b�h�̓�����J�n������
		if (nOffset == XM6_HOST_FILE_BUFFER_WRITE) ps->Ready();

		// �T�C�Y����
		DWORD n = nSize - nOffset;
		if (n > XM6_HOST_FILE_BUFFER_WRITE) n = XM6_HOST_FILE_BUFFER_WRITE;

		ps->LockXM();

#if 0
		// �f�[�^�]��
		for (DWORD i = 0; i < n; i++) chBuffer[i] = (BYTE)pMemory->ReadOnly(nAddress++);
#else
		// �擪����A�h���X�Ȃ�ŏ���1�o�C�g�]��
		BYTE* pBuffer = chBuffer;
		DWORD nEnd = nAddress + n;
		if (nAddress < nEnd && (nAddress & 1) != 0)
			*pBuffer++ = (BYTE)pMemory->ReadOnly(nAddress++);

		// 16�r�b�g�P�ʂŃf�[�^�]��
		DWORD nEndWord = nEnd & ~1;
		while (nAddress < nEndWord) {
			DWORD nData = pMemory->ReadWord(nAddress);
			*pBuffer++ = (BYTE)(nData>>8);
			*pBuffer++ = (BYTE)nData;
			nAddress += 2;
		}

		// �f�[�^���c���Ă����(�����������A�h���X�Ȃ̂�)�Ō��1�o�C�g�]��
		if (nAddress < nEnd) *pBuffer = (BYTE)pMemory->ReadOnly(nAddress++);
#endif

		ps->UnlockXM();

		// ��������
		nResult = pHostFcb->WriteFile(chBuffer, n);
		if (nResult == (DWORD)-1) {
			m_cFcb.Free(pHostFcb);
			return FS_CANTWRITE;
		}

		// �T�C�Y�W�v
		nTotal += nResult;

		// �t�@�C���I�[�Ȃ�I��
		if (nResult != n) break;
	}

	// �t�@�C���|�C���^�ۑ�
	pFcb->fileptr += nTotal;

	// �t�@�C���T�C�Y�X�V
	if (pFcb->size < pFcb->fileptr) pFcb->size = pFcb->fileptr;

#ifdef XM6_HOST_STRICT_CLOSE
	// ��������
	pHostFcb->Close();
#endif // XM6_HOST_STRICT_CLOSE

	return nTotal;
}

//---------------------------------------------------------------------------
//
//	$4E - �t�@�C���V�[�N
//
//---------------------------------------------------------------------------
int FASTCALL CWinFileSys::Seek(CWindrv* ps, DWORD nKey, Human68k::fcb_t* pFcb, DWORD nMode, int nOffset)
{
	ASSERT(this);
	ASSERT(pFcb);

	// ���f�B�A�`�F�b�N
	if (m_cEntry.isMediaOffline(ps)) return FS_FATAL_MEDIAOFFLINE;

	// ���ɓ����L�[�����̈悪�Ȃ���΃G���[�Ƃ���
	CHostFcb* pHostFcb = m_cFcb.Search(nKey);
	if (pHostFcb == NULL) return FS_NOTOPENED;

	// ����p�����[�^
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

	// �t�@�C���I�[�v��/�n���h���l��
	HANDLE hFile = pHostFcb->Open();
	if (hFile == INVALID_HANDLE_VALUE) {
		m_cFcb.Free(pHostFcb);
		return FS_NOTOPENED;
	}

	// �t�@�C���V�[�N
	DWORD nResult = pHostFcb->SetFilePointer(nOffset, nSeek);
	if (nResult == (DWORD)-1) {
		m_cFcb.Free(pHostFcb);
		return FS_CANTSEEK;
	}

	// �t�@�C���|�C���^�ۑ�
	pFcb->fileptr = nResult;

#ifdef XM6_HOST_STRICT_CLOSE
	//��������
	pHostFcb->Close();
#endif // XM6_HOST_STRICT_CLOSE

	return nResult;
}

//---------------------------------------------------------------------------
//
//	$4F - �t�@�C�������擾/�ݒ�
//
//	���ʂ̏��16Bit��$FFFF���ƃG���[�B
//
//---------------------------------------------------------------------------
DWORD FASTCALL CWinFileSys::TimeStamp(CWindrv* ps, DWORD nKey, Human68k::fcb_t* pFcb, WORD nFatDate, WORD nFatTime)
{
	ASSERT(this);
	ASSERT(nKey);
	ASSERT(pFcb);

	// ���f�B�A�`�F�b�N
	if (m_cEntry.isMediaOffline(ps)) return FS_FATAL_MEDIAOFFLINE;

	// �擾�̂�
	if (nFatDate == 0 && nFatTime == 0) {
		return ((DWORD)pFcb->date << 16) | pFcb->time;
	}

	// �������݋֎~�`�F�b�N
	if (m_cEntry.isWriteProtect(ps)) return FS_FATAL_WRITEPROTECT;

	// ���ɓ����L�[�����̈悪�Ȃ���΃G���[�Ƃ���
	CHostFcb* pHostFcb = m_cFcb.Search(nKey);
	if (pHostFcb == NULL) return FS_NOTOPENED;

	// Human68k�ł́A�ǂݍ��݃I�[�v�����ł��^�C���X�^���v���삪�\�ɂȂ��Ă��邽��
	// �ǂݍ��݃I�����[���[�h�ŊJ���Ă�����A�ꎞ�I�ɏ������݃��[�h�ŊJ���Ȃ���
	BOOL bReopen = FALSE;
	if ((pFcb->mode & 0x0F) == Human68k::OP_READ) {
		bReopen = TRUE;
#ifndef XM6_HOST_STRICT_CLOSE
		pHostFcb->Close();
#endif // XM6_HOST_STRICT_CLOSE
		pHostFcb->SetOpenMode(Human68k::OP_READWRITE);
	}

	// �t�@�C���I�[�v��/�n���h���l��
	HANDLE hFile = pHostFcb->Open();
	if (hFile == INVALID_HANDLE_VALUE) {
		m_cFcb.Free(pHostFcb);
		return FS_NOTOPENED;
	}

	// �����ݒ�
	if (pHostFcb->SetFileTime(nFatDate, nFatTime) == (DWORD)-1) {
		m_cFcb.Free(pHostFcb);
		return FS_CANTWRITE;
	}

	// �ꎞ�I�ɏ������݃��[�h�ŊJ���Ȃ������ꍇ�͌��ɖ߂�
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
	// ��������
	pHostFcb->Close();
#endif // XM6_HOST_STRICT_CLOSE

	return 0;
}

//---------------------------------------------------------------------------
//
//	$50 - �e�ʎ擾
//
//---------------------------------------------------------------------------
int FASTCALL CWinFileSys::GetCapacity(CWindrv* ps, Human68k::capacity_t* pCapacity)
{
	ASSERT(this);
	ASSERT(pCapacity);

	// ���f�B�A�`�F�b�N
	if (m_cEntry.isMediaOffline(ps)) return FS_FATAL_MEDIAOFFLINE;

	// �e�ʎ擾
	DWORD nUnit = ps->GetUnit();
	return m_cEntry.GetCapacity(nUnit, pCapacity);
}

//---------------------------------------------------------------------------
//
//	$51 - �h���C�u��Ԍ���/����
//
//---------------------------------------------------------------------------
int FASTCALL CWinFileSys::CtrlDrive(CWindrv* ps, Human68k::ctrldrive_t* pCtrlDrive)
{
	ASSERT(this);
	ASSERT(pCtrlDrive);

	DWORD nUnit = ps->GetUnit();

	switch (pCtrlDrive->status) {
	case 0:		// ��Ԍ���
	case 9:		// ��Ԍ���2
		pCtrlDrive->status = (BYTE)m_cEntry.GetStatus(nUnit);
		return pCtrlDrive->status;

	case 1:		// �C�W�F�N�g
		m_cEntry.isMediaOffline(ps, FALSE);	// �C�W�F�N�g��Ƀ`�F�b�N����̂Ŏ��O�`�F�b�N�s�v
		m_cEntry.Eject(nUnit);
		return 0;

	case 2:		// �C�W�F�N�g�֎~1 (������)
	case 3:		// �C�W�F�N�g����1 (������)
	case 4:		// ���f�B�A���}������LED�_�� (������)
	case 5:		// ���f�B�A���}������LED���� (������)
	case 6:		// �C�W�F�N�g�֎~2 (������)
	case 7:		// �C�W�F�N�g����2 (������)
		return 0;

	case 8:		// �C�W�F�N�g����
		return 1;
	}

	return FS_INVALIDFUNC;
}

//---------------------------------------------------------------------------
//
//	$52 - DPB�擾
//
//---------------------------------------------------------------------------
int FASTCALL CWinFileSys::GetDPB(CWindrv* ps, Human68k::dpb_t* pDpb)
{
	ASSERT(this);
	ASSERT(pDpb);

	DWORD nUnit = ps->GetUnit();

	// �Z�N�^���l��
	Human68k::capacity_t cap;
	BOOL bResult = m_cEntry.GetCapacityCache(nUnit, &cap);
	if (bResult == FALSE) {
		// ���f�B�A�`�F�b�N
		m_cEntry.isMediaOffline(ps);

		// �h���C�u��Ԏ擾
		m_cEntry.GetCapacity(nUnit, &cap);
	}

	// �V�t�g���v�Z
	DWORD nSize = 1;
	DWORD nShift = 0;
	for (;;) {
		if (nSize >= cap.sectors) break;
		nSize <<= 1;
		nShift++;
	}

	// �Z�N�^�ԍ��v�Z
	//
	// �ȉ��̏��ɕ��ׂ�B
	//	�N���X�^0:���g�p
	//	�N���X�^1:FAT
	//	�N���X�^2:���[�g�f�B���N�g��
	//	�N���X�^3:�f�[�^�̈�(�[���Z�N�^)
	DWORD nFat = 1 * cap.sectors;
	DWORD nRoot = 2 * cap.sectors;
	DWORD nData = 3 * cap.sectors;

	// DPB�ݒ�
	pDpb->sector_size = (WORD)cap.bytes;		// + 0	1�Z�N�^����̃o�C�g��
	pDpb->cluster_size = (BYTE)cap.sectors - 1;	// + 2	1�N���X�^����̃Z�N�^�� - 1
	pDpb->shift = (BYTE)nShift;					// + 3	�N���X�^���Z�N�^�̃V�t�g��
	pDpb->fat_sector = (WORD)nFat;				// + 4	FAT �̐擪�Z�N�^�ԍ�
	pDpb->fat_max = 1;							// + 6	FAT �̈�̌�
	pDpb->fat_size = (BYTE)cap.sectors;			// + 7	FAT �̐�߂�Z�N�^��(���ʕ�������)
	pDpb->file_max =							// + 8	���[�g�f�B���N�g���ɓ���t�@�C���̌�
		(WORD)(cap.sectors * cap.bytes / 0x20);
	pDpb->data_sector = (WORD)nData;		   	// +10	�f�[�^�̈�̐擪�Z�N�^�ԍ�
	pDpb->cluster_max =	(WORD)cap.clusters;		// +12	���N���X�^�� + 1
	pDpb->root_sector = (WORD)nRoot;			// +14	���[�g�f�B���N�g���̐擪�Z�N�^�ԍ�
	pDpb->media = 0xF3;							// +20	���f�B�A�o�C�g

	// ���f�B�A�o�C�g�ύX
	if (m_nOption & WINDRV_OPT_MEDIABYTE) {
		pDpb->media = m_cEntry.GetMediaByte(nUnit);
	}

	return 0;
}

//---------------------------------------------------------------------------
//
//	$53 - �Z�N�^�ǂݍ���
//
//---------------------------------------------------------------------------
int FASTCALL CWinFileSys::DiskRead(CWindrv* ps, DWORD nAddress, DWORD nSector, DWORD nSize)
{
	ASSERT(this);
	ASSERT(nAddress);

	Memory* pMemory = ps->GetMemory();
	ASSERT(pMemory);

	DWORD nUnit = ps->GetUnit();

	// �Z�N�^��1�ȊO�̏ꍇ�̓G���[
	if (nSize != 1) return FS_NOTIOCTRL;

	// �Z�N�^���l��
	Human68k::capacity_t cap;
	BOOL bResult = m_cEntry.GetCapacityCache(nUnit, &cap);
	if (bResult == FALSE) {
		// ���f�B�A�`�F�b�N
		m_cEntry.isMediaOffline(ps);

		// �h���C�u��Ԏ擾
		m_cEntry.GetCapacity(nUnit, &cap);
	}

	// �[���f�B���N�g���G���g���ւ̃A�N�Z�X
	CHostFiles* pHostFiles = m_cFiles.Search(nSector);
	if (pHostFiles) {
		// �[���f�B���N�g���G���g���𐶐�
		// WARNING: ���g���G���f�B�A����p
		Human68k::dirent_t dir;
		memcpy(&dir, pHostFiles->GetEntry(), sizeof(dir));

		// �[���f�B���N�g���G���g�����Ƀt�@�C�����̂��w���[���Z�N�^�ԍ����L�^
		// �Ȃ��Alzdsys �ł͈ȉ��̎��œǂݍ��݃Z�N�^�ԍ����Z�o���Ă���B
		// (dirent.cluster - 2) * (dpb.cluster_size + 1) + dpb.data_sector
		dir.cluster = (WORD)(m_nHostSectorCount + 2);	// �[���Z�N�^�ԍ�
		m_nHostSectorBuffer[m_nHostSectorCount] = nSector;	// �[���Z�N�^�̎w������
		m_nHostSectorCount++;
		m_nHostSectorCount %= XM6_HOST_PSEUDO_CLUSTER_MAX;

		ps->LockXM();

		// �[���f�B���N�g���G���g����]��
		BYTE* p = (BYTE*)&dir;
		for (int i = 0; i < 0x20; i++) pMemory->WriteByte(nAddress++, *p++);
		for (int i = 0x20; i < 0x200; i++) pMemory->WriteByte(nAddress++, 0xFF);

		ps->UnlockXM();

		return 0;
	}

	// �N���X�^�ԍ�����Z�N�^�ԍ����Z�o
	DWORD n = nSector - (3 * cap.sectors);
	DWORD nMod = 1;
	if (cap.sectors) {
		// ���f�B�A�����݂��Ȃ��ꍇ�� cap.sectors ��0�ɂȂ�̂Œ���
		nMod = n % cap.sectors;
		n /= cap.sectors;
	}

	// �t�@�C�����̂ւ̃A�N�Z�X
	if (nMod == 0 && n < XM6_HOST_PSEUDO_CLUSTER_MAX) {
		pHostFiles = m_cFiles.Search(m_nHostSectorBuffer[n]);	// ���̂�����
		if (pHostFiles) {
			// ���f�B�A�`�F�b�N
			if (m_cEntry.isMediaOffline(ps)) return FS_FATAL_MEDIAOFFLINE;

			// �[���Z�N�^�𐶐�
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

			// �[���Z�N�^��]��
			for (int i = 0; i < 0x200; i++) pMemory->WriteByte(nAddress++, chBuffer[i]);

			ps->UnlockXM();

			return 0;
		}
	}

	return FS_NOTIOCTRL;
}

//---------------------------------------------------------------------------
//
//	$54 - �Z�N�^��������
//
//---------------------------------------------------------------------------
int FASTCALL CWinFileSys::DiskWrite(CWindrv* ps, DWORD nAddress, DWORD nSector, DWORD nSize)
{
	ASSERT(this);
	ASSERT(nAddress);

	// ���f�B�A�`�F�b�N
	if (m_cEntry.isMediaOffline(ps)) return FS_FATAL_MEDIAOFFLINE;

	// �������݋֎~�`�F�b�N
	if (m_cEntry.isWriteProtect(ps)) return FS_FATAL_WRITEPROTECT;

	// ������˂�����
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
		// ���f�B�AID�̊l��
		pIoctrl->media = 0xF3;
		if (m_nOption & WINDRV_OPT_MEDIABYTE) {
			DWORD nUnit = ps->GetUnit();
			pIoctrl->media = m_cEntry.GetMediaByte(nUnit);
		}
		return 0;

	case 1:
		// Human68k�݊��̂��߂̃_�~�[
		pIoctrl->param = -1;
		return 0;

	case 2:
		switch (pIoctrl->param) {
		case -1:
			// ���f�B�A�ĔF��
			m_cEntry.isMediaOffline(ps);
			return 0;

		case 0:
		case 1:
			// Human68k�݊��̂��߂̃_�~�[
			return 0;
		}
		break;

	case -1:
		// �풓����
		memcpy(pIoctrl->buffer, "WindrvXM", 8);
		return 0;

	case -2:
		// �I�v�V�����ݒ�
		SetOption(pIoctrl->param);
		return 0;

	case -3:
		// �I�v�V�����l��
		pIoctrl->param = GetOption();
		return 0;
	}

	return FS_NOTIOCTRL;
}

//---------------------------------------------------------------------------
//
//	$56 - �t���b�V��
//
//---------------------------------------------------------------------------
int FASTCALL CWinFileSys::Flush(CWindrv* ps)
{
	ASSERT(this);

	DWORD nUnit = ps->GetUnit();

	// �t���b�V��
	m_cEntry.LockCache();
	m_cEntry.EraseCache(nUnit);
	m_cEntry.UnlockCache();

	// ��ɐ���
	return 0;
}

//---------------------------------------------------------------------------
//
//	$57 - ���f�B�A�����`�F�b�N
//
//---------------------------------------------------------------------------
int FASTCALL CWinFileSys::CheckMedia(CWindrv* ps)
{
	ASSERT(this);

	// TwentyOne�I�v�V�����Ď�
	CheckKernel(ps);

#ifndef XM6_HOST_STRICT_TIMEOUT
	// �^�C���A�E�g�`�F�b�N
	m_cEntry.CheckTimeout();
#endif // XM6_HOST_STRICT_TIMEOUT

	// �蓮�C�W�F�N�g���f�B�A�̏�ԍX�V�`�F�b�N
	BOOL bResult = m_cEntry.CheckMediaUpdate(ps);

	// ���f�B�A���}���Ȃ�G���[�Ƃ���
	if (bResult == FALSE) {
#ifdef XM6_HOST_UPDATE_BY_SEQUENCE
		// ����̃f�B���N�g���`�F�b�N�R�}���h�𖳌��ɂ��邽�߃t���O�𗧂Ă�
		m_cEntry.SetMediaUpdate(ps, TRUE);
#endif // XM6_HOST_UPDATE_BY_SEQUENCE

		return FS_INVALIDFUNC;
	}

	return 0;
}

//---------------------------------------------------------------------------
//
//	$58 - �r������
//
//---------------------------------------------------------------------------
int FASTCALL CWinFileSys::Lock(CWindrv* ps)
{
	ASSERT(this);

	// ���f�B�A�`�F�b�N
	if (m_cEntry.isMediaOffline(ps)) return FS_FATAL_MEDIAOFFLINE;

	// ��ɐ���
	return 0;
}

#if 0
//---------------------------------------------------------------------------
//
//	Win32�ŏI�G���[�擾 Human68k�G���[�ɕϊ�
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
//	�I�v�V�����ݒ�
//
//---------------------------------------------------------------------------
void FASTCALL CWinFileSys::SetOption(DWORD nOption)
{
	ASSERT(this);

	// �f�B���N�g���G���g���̍č\�����K�v�Ȃ�L���b�V���N���A
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
//	�I�v�V����������
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
			// �I�v�V�����w��ł͂Ȃ��̂Ŏ���
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

	// �I�v�V�����ݒ�
	if (nOption != m_nOption) {
		SetOption(nOption);
	}
}

//---------------------------------------------------------------------------
//
//	�{�����[�����x���擾
//
//---------------------------------------------------------------------------
BOOL FASTCALL CWinFileSys::FilesVolume(CWindrv* ps, Human68k::files_t* pFiles)
{
	ASSERT(this);
	ASSERT(pFiles);

	DWORD nUnit = ps->GetUnit();

	// �{�����[�����x���擾
	TCHAR szVolume[32];
	BOOL bResult = m_cEntry.GetVolumeCache(nUnit, szVolume);
	if (bResult == FALSE) {
		// ���f�B�A�`�F�b�N
		m_cEntry.isMediaOffline(ps);

		// �{�����[�����x���擾
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
//	TwentyOne�I�v�V�����Ď�
//
//	�J�[�l���s���̂��߁A��ނ𓾂������[�g�h���C�u���őΏ����Ȃ����
//	�Ȃ�Ȃ��p�����[�^���������f����B
//
//	TODO: �����J�[�l���E�h���C�o���C�����Ă��̏������̂��̂��Ȃ�������
//
//---------------------------------------------------------------------------
void FASTCALL CWinFileSys::CheckKernel(CWindrv* ps)
{
	ASSERT(this);

	if ((m_nOption & WINDRV_OPT_TWENTYONE) == 0) return;

	ps->LockXM();

	Memory* pMemory = ps->GetMemory();
	ASSERT(pMemory);

	// �܂������������Ă��Ȃ����Human68k�J�[�l�������[�N�𒲍�����
	if (m_nKernel < 1024) {
		// �Ē���ւ̃J�E���g�_�E��
		if (m_nKernel > 0) {
			m_nKernel--;		// ������Ē���
			goto CheckKernelExit;
		}

		// Step1: NUL�f�o�C�X����
		if (m_nKernelSearch == 0) {
			DWORD n = 0x6800;
			for (;;) {
				DWORD nData = pMemory->ReadWord(n);
				if (nData == 'NU') {
					if (pMemory->ReadWord(n + 2) == 'L ') break;
				}
				n += 2;
				if (n >= 0x20000 - 2) {
					// NUL�f�o�C�X�����ł���
					m_nKernel = 0xFFFFFFFF;		// �J�[�l���ُ�: ��x�ƌ������Ȃ�
					goto CheckKernelExit;
				}
			}
			n -= 14;
			m_nKernelSearch = n;
		}

		// Step2: NUL�f�o�C�X���N�_�Ƃ��đS�f�o�C�X����
		DWORD n = m_nKernelSearch;
		for (;;) {
			// ���̃f�o�C�X��
			n = (pMemory->ReadWord(n) << 16) | pMemory->ReadWord(n + 2);
			if (n == 0xFFFFFFFF) {
				// �Y���f�o�C�X�Ȃ�
				m_nKernel = XM6_HOST_TWENTYONE_CHECK_COUNT;	// �������s: ������Ē���
				goto CheckKernelExit;
			}

			DWORD x1 = (pMemory->ReadWord(n + 14) << 16) | pMemory->ReadWord(n + 16);

			if (x1 == '*Twe') {
				DWORD x2 = (pMemory->ReadWord(n + 18) << 16) | pMemory->ReadWord(n + 20);
				if (x2 == 'nty*') {
					// ���o�[�W�����𔭌�
					m_nKernel = 0xFFFFFFFF;		// TwentyOne���o�[�W����: ��x�ƌ������Ȃ�
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

		// ����
		m_nKernel = n + 22;
	} else {
		if (m_nKernel == 0xFFFFFFFF) {
			goto CheckKernelExit;
		}
	}

	{
		// �J�[�l�����I�v�V�����l��
		DWORD nKernelOption =
			(pMemory->ReadWord(m_nKernel) << 16) | pMemory->ReadWord(m_nKernel + 2);

		// �����[�g�h���C�u���I�v�V�����l��
		DWORD nOption = m_nOption &
			~(WINDRV_OPT_ALPHABET | WINDRV_OPT_COMPARE_LENGTH | WINDRV_OPT_CONVERT_LENGTH);

		// �I�v�V�������f
		if (nKernelOption & 0x40000000) {	// _TWON_C_BIT: Bit30
			nOption |= WINDRV_OPT_ALPHABET;
		}
		if (nKernelOption & 0x08000000) {	// _TWON_T_BIT: Bit27
			nOption |= WINDRV_OPT_COMPARE_LENGTH;
		}
		if (nKernelOption & 0x00400000) {	// _TWON_A_BIT: Bit22
			nOption |= WINDRV_OPT_CONVERT_LENGTH;
		}

		// �I�v�V�����ݒ�
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
//	�R���X�g���N�^
//
//---------------------------------------------------------------------------
CHost::CHost(CFrmWnd *pWnd) : CComponent(pWnd)
{
	// �R���|�[�l���g�p�����[�^
	m_dwID = MAKEID('H', 'O', 'S', 'T');
	m_strDesc = _T("Host FileSystem");

	// �I�u�W�F�N�g
	m_pWindrv = NULL;
}

//---------------------------------------------------------------------------
//
//	������
//
//---------------------------------------------------------------------------
BOOL FASTCALL CHost::Init()
{
	ASSERT(this);

	// ��{�N���X
	if (!CComponent::Init()) {
		return FALSE;
	}

	// Windrv�擾
	ASSERT(!m_pWindrv);
	m_pWindrv = (Windrv*)::GetVM()->SearchDevice(MAKEID('W', 'D', 'R', 'V'));
	ASSERT(m_pWindrv);

	// �t�@�C���V�X�e���ݒ�
	m_pWindrv->SetFileSys(&m_WinFileSys);

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	�N���[���A�b�v
//
//---------------------------------------------------------------------------
void FASTCALL CHost::Cleanup()
{
	ASSERT(this);

	// �t�@�C���V�X�e����؂藣��
	if (m_pWindrv) {
		m_pWindrv->SetFileSys(NULL);
	}

	// ���Z�b�g(�S�N���[�Y)
	m_WinFileSys.Reset();

	// ��{�N���X
	CComponent::Cleanup();
}

//---------------------------------------------------------------------------
//
//	�ݒ�K�p
//
//---------------------------------------------------------------------------
void FASTCALL CHost::ApplyCfg(const Config* pConfig)
{
	ASSERT(this);
	ASSERT(pConfig);

	// �t�@�C���V�X�e�����Ăяo��
	m_WinFileSys.ApplyCfg(pConfig);
}

#endif // _WIN32
