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

#if !defined(mfc_host_h)
#define mfc_host_h

#include "device.h"
#include "windrv.h"

//���t�@�C���l�[��������̃`�F�b�N���V�t�gJIS�ōs�Ȃ�Ȃ�(�p��OS�p)
//#define XM6_HOST_NO_JAPANESE_NAME

//���t�@�C���̃I�[�v��/�N���[�Y�𖈉�s�Ȃ�
// �I�[�v��/�N���[�Y�Ǘ���VM���ōs�Ȃ��Ă���̂ŗL���ɂ��Ă��Ӗ��͂Ȃ��B
// TODO: �X�e�[�g���[�h���̃n���h�����A�����̌���
//#define XM6_HOST_STRICT_CLOSE

//�������[�o�u�����f�B�A�̃L���b�V���L�����Ԃ𐳊m�ɑ��肷��
// �{�@�\��K�p���Ȃ������ꍇ�AHuman68k���̃A�v���P�[�V�����̍���
// �ɂ���ẮA�z�X�g���Ń��f�B�A�̃C�W�F�N�g����𒷎��ԍs�Ȃ��Ȃ�
// �Ȃ镾�Q����������̂Œ��ӁB
#define XM6_HOST_STRICT_TIMEOUT

//���t�@�C���������ɕԂ��t�@�C���T�C�Y�̍ő�l
// Human68k�G���[�R�[�h�Əd�����Ȃ��͈͂Őݒ肷��
#define XM6_HOST_FILESIZE_MAX 0xFFFFFF7F

//���t�@�C���ǂݏ������̃o�b�t�@�T�C�Y
// 1�o�C�g�ÂĂ΂ꂽ�肷�邱�Ƃ�����悤�Ȃ̂ŁA���܂��ʂɎ���Ă�
// ���ʂɂȂ��Ă��܂��\��������B
// TODO: ���炩���ߌŒ�o�b�t�@���m�ۂ��Ă����ׂ�������
// �X���b�h���[�h�ł́A���̃T�C�Y��]�����閈��VM�̊J�����s�Ȃ��邽�߁A
// �ő�]�����x�ɉe������B
#define XM6_HOST_FILE_BUFFER_READ 0x10000
#define XM6_HOST_FILE_BUFFER_WRITE 0x10000

//��FILES�p�o�b�t�@��
// �����̃X���b�h�������ɐ[���K�w�ɓn���č�Ƃ��鎞�Ȃǂ�
// ���̒l�𑝂₷�K�v������
#define XM6_HOST_FILES_MAX 20

//��FCB�p�o�b�t�@��
// �����ɃI�[�v���ł���t�@�C�����͂���Ō��܂�
#define XM6_HOST_FCB_MAX 100

//���[���Z�N�^/�N���X�^ �ő��
// �t�@�C�����̂̐擪�Z�N�^�ւ̃A�N�Z�X�ɑΉ�
// lzdsys�ɂ��A�N�Z�X���s�Ȃ��X���b�h�̐���葽�߂Ɋm�ۂ���
#define XM6_HOST_PSEUDO_CLUSTER_MAX 10

//���f�B���N�g���G���g�� �L���b�V����
// ������Α����قǍ����ɂȂ邪�A���₵������ƃz�X�gOS���ɕ��S��������
#define XM6_HOST_DIRENTRY_CACHE_MAX 30

//���f�B���N�g�����̃t�@�C���� �L���b�V����
// �S�Ẵt�@�C�������d�����肳�ꂽ�ꍇ�͖�6�疜�t�@�C��������ƂȂ�(36��5��)
#define XM6_HOST_FILENAME_CACHE_MAX (36 * 36 * 36 * 36 * 36)

//���t�@�C�����d���h�~�}�[�N
// �z�X�g���t�@�C������Human68k���t�@�C�����̖��̂̋�ʂ�����Ƃ��Ɏg������
// �R�}���h�V�F�����̃G�X�P�[�v�����Əd�Ȃ�Ȃ����̂�I�ԂƋg
#define XM6_HOST_FILENAME_MARK '@'

//�������[�o�u�����f�B�A�̑}�����R�}���h���s�������_�@�Ɍ��o����
// �m���Ɍ��o�ł��邪�A�኱���ʂȃA�N�Z�X����������\������
#define XM6_HOST_UPDATE_BY_SEQUENCE

//�������[�o�u�����f�B�A�̑}�����A�N�Z�X�p�x���_�@�Ɍ��o����
// �R�}���h���s�����ł̌��o�����܂������Ȃ��ꍇ�̂��ߎc���Ă���B
// �{�����ł́Amint2.xx�n�̑}���`�F�b�N���펞�L���ɂȂ邽�߁A���쑬
// �x���ቺ�����肠��B
//#define XM6_HOST_UPDATE_BY_FREQUENCY

//�������[�o�u�����f�B�A�̑}�����A�N�Z�X�p�x���펞�s�Ȃ�
// ��L�̓�����ł��܂������Ȃ��ꍇ�͂�����g���B�������蓮�C�W�F�N
// �g�̃��f�B�A�����݂���Ɠ��쑬�x���펞�ቺ����̂Œ��ӁB
//#define XM6_HOST_UPDATE_ALWAYS

#ifdef XM6_HOST_UPDATE_BY_FREQUENCY
//�������[�o�u�����f�B�A�̍X�V�������(ms)�ƍX�V�����
// CheckMedia($57)�ɂ�郁�f�B�A�����`�F�b�N���s�񐔂����̒l��菭�Ȃ���Ή������Ȃ�
// mint3.x�n�̏ꍇ�A100ms�ȓ���8��̃A�N�Z�X�Ń����[�h������B
#define XM6_HOST_REMOVABLE_RELOAD_TIME 100
#define XM6_HOST_REMOVABLE_RELOAD_COUNT 8
#endif // XM6_HOST_UPDATE_BY_FREQUENCY

//�������[�o�u�����f�B�A�̘A���A�N�Z�X�K�[�h����(ms)
// �Ō�̃��f�B�A�L���`�F�b�N�Ԋu���炱�̎��Ԍo�߂���܂ł͍ă`�F�b�N���Ȃ�
// ���f�B�A�����݂��Ȃ��Ƃ��`�F�b�N�Ɏ��Ԃ̂�����f�o�C�X�ւ̑΍�B
#define XM6_HOST_REMOVABLE_GUARD_TIME 3000

//�������[�o�u�����f�B�A�̃L���b�V���L������(ms)
// �t�@�C���A�N�Z�X�Ԋu�����̎��Ԃ��J�����ꍇ�A�L���b�V�����N���A�����
// �蓮�C�W�F�N�g���f�B�A�̏ꍇ�A�t�@�C���A�N�Z�X�̒��O�ɂ͕K�����f�B
// �A�����`�F�b�N���s�Ȃ����߁A�A�����ăt�@�C���A�N�Z�X�����s���ꂽ
// �ꍇ�̓L���b�V�����e�����̂܂ܕێ�����B
#define XM6_HOST_REMOVABLE_CACHE_TIME 3000

//�������[�o�u�����f�B�A�̃L���b�V���m�F�Ԋu(ms)
// �l������������Ƃ�萳�m�Ɋm�F����悤�ɂȂ邪�A���܂�Ӗ��͂Ȃ�
#define XM6_HOST_REMOVABLE_CHECK_TIME 1000

//��TwentyOne�풓�m�F�Ԋu
// TwentyOne���풓���Ă��Ȃ��ꍇ�A���񌟍�����͎̂��Ԃ̖��ʂȂ̂Ŏw
// ��񐔂����Ɋm�F����B�풓���Ă����ꍇ�͌����͍ŏ���1��݂̂Ȃ̂�
// �C�ɂ��Ȃ��Ă悢�B�܂��A�������������Ȃ��ꍇ�͈Ӗ��������Ȃ��B
#define XM6_HOST_TWENTYONE_CHECK_COUNT 10

//��WINDRV����t���O
// Bit 0: �t�@�C�����ϊ� �X�y�[�X		0:�Ȃ� 1:'_'
// Bit 1: �t�@�C�����ϊ� �����ȕ���		0:�Ȃ� 1:'_'
// Bit 2: �t�@�C�����ϊ� ���Ԃ̃n�C�t��	0:�Ȃ� 1:'_'
// Bit 3: �t�@�C�����ϊ� �擪�̃n�C�t��	0:�Ȃ� 1:'_'
// Bit 4: �t�@�C�����ϊ� ���Ԃ̃s���I�h	0:�Ȃ� 1:'_'
// Bit 5: �t�@�C�����ϊ� �擪�̃s���I�h	0:�Ȃ� 1:'_'
//
// Bit 8: �t�@�C�������� Alphabet���	0:�Ȃ� 1:����	0:-C 1:+C
// Bit 9: �t�@�C������r ������(������)	0:18+3 1:8+3	0:+T 1:-T
// Bit10: �t�@�C�����ϊ� ������			0:18+3 1:8+3	0:-A 1:+A
// Bit11: �\�� (�����R�[�h)				0:SJIS 1:ANSI
// Bit12: �\��							0:�Ȃ� 1:��
// Bit13: ���f�B�A�o�C�g����			0:�Œ� 1:�ϓ�
// Bit14: TwentyOne�Ď�����				0:�Ȃ� 1:�Ď�
// Bit15: �t�@�C���폜����				0:���� 1:���ݔ�
//
// Bit16: �t�@�C�����Z�k �X�y�[�X		0:�Ȃ� 1:�Z�k
// Bit17: �t�@�C�����Z�k �����ȕ���		0:�Ȃ� 1:�Z�k
// Bit18: �t�@�C�����Z�k ���Ԃ̃n�C�t��	0:�Ȃ� 1:�Z�k
// Bit19: �t�@�C�����Z�k �擪�̃n�C�t��	0:�Ȃ� 1:�Z�k
// Bit20: �t�@�C�����Z�k ���Ԃ̃s���I�h	0:�Ȃ� 1:�Z�k
// Bit21: �t�@�C�����Z�k �擪�̃s���I�h	0:�Ȃ� 1:�Z�k
//
// Bit24�`30 �t�@�C���d���h�~�}�[�N	0:���� 1�`127:����
enum {
	WINDRV_OPT_CONVERT_SPACE =		0x00000001,
	WINDRV_OPT_CONVERT_BADCHAR =	0x00000002,
	WINDRV_OPT_CONVERT_HYPHENS =	0x00000004,
	WINDRV_OPT_CONVERT_HYPHEN =		0x00000008,
	WINDRV_OPT_CONVERT_PERIODS =	0x00000010,
	WINDRV_OPT_CONVERT_PERIOD =		0x00000020,

	WINDRV_OPT_ALPHABET =			0x00000100,
	WINDRV_OPT_COMPARE_LENGTH =		0x00000200,
	WINDRV_OPT_CONVERT_LENGTH =		0x00000400,

	WINDRV_OPT_MEDIABYTE =			0x00002000,
	WINDRV_OPT_TWENTYONE =			0x00004000,
	WINDRV_OPT_REMOVE =				0x00008000,

	WINDRV_OPT_REDUCED_SPACE =		0x00010000,
	WINDRV_OPT_REDUCED_BADCHAR =	0x00020000,
	WINDRV_OPT_REDUCED_HYPHENS =	0x00040000,
	WINDRV_OPT_REDUCED_HYPHEN =		0x00080000,
	WINDRV_OPT_REDUCED_PERIODS =	0x00100000,
	WINDRV_OPT_REDUCED_PERIOD =		0x00200000,
};

//���t�@�C���V�X�e������t���O
// �ʏ��0�ɂ���B���肪����ȃf�o�C�X(����USB�X�g���[�W�Ƃ�)�̂��߂̕ی��B
// Bit0: �����������݋֎~
// Bit1: �����ᑬ�f�o�C�X
// Bit2: ���������[�o�u�����f�B�A
// Bit3: �����蓮�C�W�F�N�g���f�B�A
enum {
	FSFLAG_WRITE_PROTECT =			0x00000001,
	FSFLAG_SLOW =					0x00000002,
	FSFLAG_REMOVABLE =				0x00000004,
	FSFLAG_MANUAL =					0x00000008,
};

//===========================================================================
//
//	Windows�t�@�C���h���C�u
//
//	�h���C�u���ɕK�v�ȏ��̕ێ��ɐ�O���A�Ǘ��͏�ʂōs�Ȃ��B
//	CWinFileSys�Ŏ��̂��Ǘ����ACHostEntry�ŗ��p����B
//	TODO: ���̂�CHostEntry�����ֈړ������邩����
//
//===========================================================================
class CWinFileDrv
{
public:
	// ��{�t�@���N�V����
	CWinFileDrv();
										// �R���X�g���N�^
	virtual ~CWinFileDrv();
										// �f�X�g���N�^
	void FASTCALL Init(LPCTSTR szBase, DWORD nFlag);
										// ������

	TCHAR* FASTCALL GetBase() const { ASSERT(this); return (TCHAR*)m_szBase; }
										// �x�[�X�p�X�̎擾
	BOOL FASTCALL isWriteProtect() const { ASSERT(this); return m_bWriteProtect; }
										// �������݋����̎擾
	BOOL FASTCALL isSlow() const { ASSERT(this); return m_bSlow; }
										// �������݋����̎擾
	BOOL FASTCALL isRemovable() const { ASSERT(this); return m_bRemovable; }
										// �����[�o�u�����f�B�A���H
	BOOL FASTCALL isManual() const { ASSERT(this); return m_bManual; }
										// �蓮�C�W�F�N�g���f�B�A���H
	BOOL FASTCALL isEnable() const { ASSERT(this); return m_bEnable; }
										// �A�N�Z�X�\���H
	BOOL FASTCALL isSameDrive(const TCHAR* szDrive) const { ASSERT(this); return _tcscmp(m_szDrive, szDrive) == 0; }
										// ����h���C�u�����H
	DWORD FASTCALL GetStatus() const;
										// �h���C�u��Ԃ̎擾
	void FASTCALL SetEnable(BOOL bEnable);
										// ���f�B�A��Ԑݒ�
	void FASTCALL SetTimeout();
										// �����[�o�u�����f�B�A�̃t�@�C���L���b�V���L�����Ԃ��X�V
	BOOL FASTCALL CheckTimeout();
										// �����[�o�u�����f�B�A�̃t�@�C���L���b�V���L�����Ԃ��m�F
#ifdef XM6_HOST_UPDATE_BY_SEQUENCE
	void FASTCALL SetMediaUpdate(BOOL bDisable);
										// �����[�o�u�����f�B�A�̏�ԍX�V��L���ɂ���
#endif // XM6_HOST_UPDATE_BY_SEQUENCE
	BOOL FASTCALL CheckMediaUpdate();
										// �����[�o�u�����f�B�A�̏�ԍX�V�`�F�b�N
	BOOL FASTCALL CheckMediaAccess(BOOL bManual);
										// �����[�o�u�����f�B�A�̃A�N�Z�X���O�`�F�b�N
	BOOL FASTCALL CheckMedia();
										// ���f�B�A�L���`�F�b�N
	BOOL FASTCALL Eject();
										// �C�W�F�N�g
	void FASTCALL GetVolume(TCHAR* szLabel);
										// �{�����[�����x���̎擾
	BOOL FASTCALL GetVolumeCache(TCHAR* szLabel) const;
										// �L���b�V������{�����[�����x�����擾
	DWORD FASTCALL GetCapacity(Human68k::capacity_t* pCapacity);
										// �e�ʂ̎擾
	BOOL FASTCALL GetCapacityCache(Human68k::capacity_t* pCapacity) const;
										// �L���b�V������N���X�^�T�C�Y���擾

private:
	void FASTCALL OpenDevice();
										// �f�o�C�X�I�[�v��
	void FASTCALL CloseDevice();
										// �f�o�C�X�N���[�Y

	BOOL m_bWriteProtect;
										// �������݋֎~�Ȃ�TRUE
	BOOL m_bSlow;
										// �ᑬ�f�o�C�X�Ȃ�TRUE
	BOOL m_bRemovable;
										// �����[�o�u�����f�B�A�Ȃ�TRUE
	BOOL m_bManual;
										// �蓮�C�W�F�N�g�Ȃ�TRUE
	BOOL m_bEnable;
										// ���f�B�A�����p�\�Ȃ�TRUE
	BOOL m_bDevice;
										// WinNT�n�Ő������f�o�C�X���������Ă���Ȃ�TRUE
	HANDLE m_hDevice;
										// �f�o�C�X�n���h��(NT�n��p)
	TCHAR m_szDevice[8];
										// �f�o�C�X��(NT�n��p)
	TCHAR m_szDrive[4];
										// �h���C�u��
	Human68k::capacity_t m_capCache;
										// �Z�N�^���L���b�V�� sectors == 0 �Ȃ疢�L���b�V��
	BOOL m_bVolumeCache;
										// �{�����[�����x���ǂݍ��ݍς݂Ȃ�TRUE
	TCHAR m_szVolumeCache[24];
										// �{�����[�����x���L���b�V��
	DWORD m_nUpdate;
										// �Ō�Ƀ��f�B�A���p�\��Ԃ��l����������
	BOOL m_bUpdateFile;
										// �L���b�V���N���A�̔��肪�K�v�Ȃ�TRUE
	DWORD m_nUpdateFile;
										// �Ō�Ƀt�@�C���A�N�Z�X���s�Ȃ�������
#ifdef XM6_HOST_UPDATE_BY_SEQUENCE
	DWORD m_nUpdateMedia;
										// �����[�o�u�����f�B�A�̍X�V����X�e�[�g
#endif // XM6_HOST_UPDATE_BY_SEQUENCE
#ifdef XM6_HOST_UPDATE_BY_FREQUENCY
	DWORD m_nUpdateCount;
										// �����[�o�u�����f�B�A�̍X�V����o�b�t�@�ʒu
	DWORD m_nUpdateBuffer[XM6_HOST_REMOVABLE_RELOAD_COUNT];
										// �����[�o�u�����f�B�A�̍X�V����o�b�t�@
#endif // XM6_HOST_UPDATE_BY_FREQUENCY

	TCHAR m_szBase[_MAX_PATH];
										// �x�[�X�p�X
};

//===========================================================================
//
//	�܂���ƃ����O���X�g
//
//	�擪(root.next)���ł��V�����I�u�W�F�N�g
//	����(root.prev)���ł��Â�/���g�p�I�u�W�F�N�g
//
//===========================================================================
class CRing {
public:
	CRing() { next = prev = this; }
										// �R���X�g���N�^
	virtual ~CRing() { Remove(); }
										// �f�X�g���N�^�͉��z�֐����B�m���ɊJ��������
	CRing* FASTCALL Next() const { ASSERT(this); return next; }
	CRing* FASTCALL Prev() const { ASSERT(this); return prev; }
										// �ړ��p

	void FASTCALL Insert(class CRing* pRoot)
										// �I�u�W�F�N�g�؂藣�� & �����O�擪�֑}��
	{
		ASSERT(this);
		// �Y���I�u�W�F�N�g��؂藣��
		ASSERT(next);
		ASSERT(prev);
		next->prev = prev;
		prev->next = next;
		// �����O�擪�֑}��
		ASSERT(pRoot);
		ASSERT(pRoot->next);
		next = pRoot->next;
		prev = pRoot;
		pRoot->next->prev = this;
		pRoot->next = this;
	}

	void FASTCALL InsertTail(class CRing* pRoot)
										// �I�u�W�F�N�g�؂藣�� & �����O�����֑}��
	{
		ASSERT(this);
		// �Y���I�u�W�F�N�g��؂藣��
		ASSERT(next);
		ASSERT(prev);
		next->prev = prev;
		prev->next = next;
		// �����O�����֑}��
		ASSERT(pRoot);
		ASSERT(pRoot->prev);
		next = pRoot;
		prev = pRoot->prev;
		pRoot->prev->next = this;
		pRoot->prev = this;
	}

	void FASTCALL InsertRing(class CRing* pRoot)
										// �����ȊO�̃I�u�W�F�N�g�؂藣�� & �����O�擪�֑}��
	{
		ASSERT(this);
		if (next == prev) return;

		// �����O�擪�֑}��
		ASSERT(pRoot);
		ASSERT(pRoot->next);
		pRoot->next->prev = prev;
		prev->next = pRoot->next;
		pRoot->next = next;
		next->prev = pRoot;

		// �������g����ɂ���
		next = prev = this;
	}

	void FASTCALL Remove()
										// �I�u�W�F�N�g�؂藣��
	{
		ASSERT(this);
		// �Y���I�u�W�F�N�g��؂藣��
		ASSERT(next);
		ASSERT(prev);
		next->prev = prev;
		prev->next = next;
		// ���S�̂��ߎ������g���w���Ă��� (���x�؂藣���Ă����Ȃ�)
		next = prev = this;
	}

	friend void FASTCALL Insert(class CRing* pRoot);
	friend void FASTCALL InsertTail(class CRing* pRoot);
	friend void FASTCALL Remove();
										// �����o�ϐ��̐ݒ胁�\�b�h�����J���Ȃ����߁A
										// �I�u�W�F�N�g���상�\�b�h��friend�o�^���K�v

private:
	class CRing* next;
										// ���̗v�f
	class CRing* prev;
										// �O�̗v�f
};

//===========================================================================
//
//	�f�B���N�g���G���g�� �t�@�C����
//
//===========================================================================
class CHostFilename: public CRing {
public:
	CHostFilename();
										// �R���X�g���N�^
	virtual ~CHostFilename();
										// �f�X�g���N�^
	void FASTCALL FreeWin32() { ASSERT(this); if(m_pszWin32) { free(m_pszWin32); m_pszWin32 = NULL; } }
										// Win32�v�f�̊J��
	void FASTCALL SetWin32(const TCHAR* szWin32);
										// Win32���̖��̂̐ݒ�
	void FASTCALL SetHuman(int nCount = -1);
										// Human68k���̖��̂�ϊ�
	void FASTCALL CopyHuman(const BYTE* szHuman);
										// Human68k���̖��̂𕡐�
	BOOL FASTCALL SetEntry(const WIN32_FIND_DATA* pWin32Find);
										// �f�B���N�g���G���g�����̐ݒ�
	inline BOOL FASTCALL isReduce() const;
										// Human68k���̖��̂����H���ꂽ������
	BOOL FASTCALL isCorrect() const { ASSERT(this); return m_bCorrect; }
										// Human68k���̃t�@�C�����K���ɍ��v���Ă��邩����
	BYTE* FASTCALL GetHuman() const { ASSERT(this); return (BYTE*)m_szHuman; }
										// Human68k�t�@�C�������擾
	BYTE* FASTCALL GetHumanLast() const { ASSERT(this); return m_pszHumanLast; }
										// Human68k�t�@�C�������擾
	BYTE* FASTCALL GetHumanExt() const { ASSERT(this); return m_pszHumanExt; }
										// Human68k�t�@�C�������擾
	BOOL FASTCALL CheckAttribute(DWORD nHumanAttribute) const { ASSERT(this); return (m_dirEntry.attr & nHumanAttribute); }
										// Human68k���̑����̊Y������
	Human68k::dirent_t* FASTCALL GetEntry() const { ASSERT(this); return (Human68k::dirent_t*)&m_dirEntry; }
										// Human68k�f�B���N�g���G���g�����擾
	TCHAR* FASTCALL GetWin32() const { ASSERT(this); return m_pszWin32; }
										// Win32�t�@�C�������擾
	void FASTCALL SetChild(class CHostEntry* pEntry, class CHostPath* pChild) { ASSERT(this); m_pEntry = pEntry; m_pChild = pChild; }
										// �f�B���N�g���̎��̂�o�^
	BOOL FASTCALL isSameEntry(Human68k::dirent_t* pdirEntry) const { ASSERT(this); ASSERT(pdirEntry); return memcmp(&m_dirEntry, pdirEntry, sizeof(m_dirEntry)) == 0; }
										// �G���g���̈�v����

	// CHostFiles�����p����O��API
	void FASTCALL DeleteCache();
										// �Y���G���g���̃L���b�V�����폜

	// CWinFileSys�����p����O��API
	static void FASTCALL SetOption(DWORD nOption) { g_nOption = nOption; }
										// ����t���O�ݒ�

	// �ėp���[�`��
	static const BYTE* FASTCALL SeparateExt(const BYTE* szHuman);
										// Human68k�t�@�C��������g���q�𕪗�

private:
	inline BYTE* FASTCALL CopyName(BYTE* pWrite, const BYTE* pFirst, const BYTE* pLast);
										//	Human68k���̃t�@�C�����v�f���R�s�[

	TCHAR* m_pszWin32;
										// �Y���G���g���̎���
	BOOL m_bCorrect;
										// �Y���G���g����Human68k����������������� TRUE
	BYTE m_szHuman[24];
										// �Y���G���g����Human68k������ ���ڌ����p
	Human68k::dirent_t m_dirEntry;
										// �Y���G���g����Human68k�S���

	BYTE* m_pszHumanLast;
										// �Y���G���g�������E�������̍������p �I�[�ʒu
	BYTE* m_pszHumanExt;
										// �Y���G���g�������E�������̍������p �g���q�ʒu

	class CHostEntry* m_pEntry;
										// �Y���G���g���̊Ǘ��� (�L���b�V�����Ȃ����NULL)
	class CHostPath* m_pChild;
										// �Y���G���g���̓��e (�L���b�V�����Ȃ����NULL)

	static DWORD g_nOption;
										// ����t���O (�t�@�C�����ϊ�/�Z�k�֘A)
};

//===========================================================================
//
//	�f�B���N�g���G���g�� �p�X��
//
//===========================================================================
class CHostPath: public CRing {
public:
	// �����p�o�b�t�@
	typedef struct {
		DWORD count;
										// �������s�� + 1 (0�̂Ƃ��͈ȉ��̒l�͖���)
		DWORD id;
										// ���񌟍��𑱍s����p�X�̃G���g������ID
		CHostFilename* pos;
										// ���񌟍��𑱍s����ʒu (����ID��v��)
		Human68k::dirent_t entry;
										// ���񌟍��𑱍s����G���g�����e

		void FASTCALL Clear() { count = 0; }
										// ������
	} find_t;

public:
	CHostPath();
										// �R���X�g���N�^
	virtual ~CHostPath();
										// �f�X�g���N�^
	void FASTCALL FreeHuman() { ASSERT(this); m_nHumanUnit = 0; if(m_pszHuman) { free(m_pszHuman); m_pszHuman = NULL; } }
										// Human68k�v�f�̊J��
	void FASTCALL FreeWin32() { ASSERT(this); if(m_pszWin32) { free(m_pszWin32); m_pszWin32 = NULL; } }
										// Win32�v�f�̊J��
	void FASTCALL SetHuman(DWORD nUnit, BYTE* szHuman);
										// Human68k���̖��̂𒼐ڎw�肷��
	void FASTCALL SetWin32(TCHAR* szWin32);
										// Win32���̖��̂𒼐ڎw�肷��
	BOOL FASTCALL isSameUnit(DWORD nUnit) const { ASSERT(this); return m_nHumanUnit == nUnit; }
										// Human68k���̖��̂��r����
	BOOL FASTCALL isSameHuman(DWORD nUnit, const BYTE* szHuman) const;
										// Human68k���̖��̂��r����
	TCHAR* FASTCALL GetWin32() const { ASSERT(this); return m_pszWin32; }
										// Win32�p�X���̊l��
	CHostFilename* FASTCALL FindFilename(BYTE* szHuman, DWORD nHumanAttribute = Human68k::AT_ALL);
										// �t�@�C����������
	CHostFilename* FASTCALL FindFilenameWildcard(BYTE* szHuman, find_t* pFind, DWORD nHumanAttribute = Human68k::AT_ALL);
										// �t�@�C���������� (���C���h�J�[�h�Ή�)
	void FASTCALL Clean();
										// �ė��p�̂��߂̏�����
	void FASTCALL CleanFilename();
										// �S�t�@�C�������J��
	BOOL FASTCALL isRefresh();
										// �t�@�C���ύX���s�Ȃ�ꂽ���m�F
	BOOL FASTCALL Refresh();
										// �t�@�C���č\��

	// CHostEntry�����p����O��API
	static void FASTCALL InitId() { g_nId = 0; }
										// ����ID�����p�J�E���^������

	// CWinFileSys�����p����O��API
	static void FASTCALL SetOption(DWORD nOption) { g_nOption = nOption; }
										// ����t���O�ݒ�

private:
	static inline int FASTCALL Compare(const BYTE* pFirst, const BYTE* pLast, const BYTE* pBufFirst, const BYTE* pBufLast);
										// �������r (���C���h�J�[�h�Ή�)

	DWORD m_nHumanUnit;
										// �Y���G���g����Human68k���j�b�g�ԍ�
	BYTE* m_pszHuman;
										// �Y���G���g����Human68k�������B������ / �����Ȃ�
	TCHAR* m_pszWin32;
										// �Y���G���g���̎��́B������ / �����Ȃ�
	HANDLE m_hChange;
										// �t�@�C���ύX�Ď��p�n���h��
	DWORD m_nId;
										// ����ID (�l���ω������ꍇ�͍X�V���Ӗ�����)
	CRing m_cRingFilename;
										// �t�@�C���� �I�u�W�F�N�g�ێ�
										// �������f�B���N�g���G���g�����ɕ��ׂ邱�Ƃ�D�悷��

	static DWORD g_nId;
										// ����ID�����p�J�E���^

	static DWORD g_nOption;
										// ����t���O (�p�X����r�֘A)
};

//===========================================================================
//
//	�f�B���N�g���G���g���Ǘ�
//
//===========================================================================
class CHostEntry {
public:
	CHostEntry();
										// ������ (�N������)
	virtual ~CHostEntry();
										// �J�� (�I����)
	void FASTCALL Init(CWinFileDrv** ppBase);
										// ������ (�h���C�o�g���ݎ�)
	void FASTCALL Clean();
										// �J�� (�N���E���Z�b�g��)

#ifdef XM6_HOST_STRICT_TIMEOUT
	// �X���b�h����
	static DWORD WINAPI Run(VOID* pThis);
										// �X���b�h���s�J�n�|�C���g
	void FASTCALL Runner();
										// �X���b�h����
#endif // XM6_HOST_STRICT_TIMEOUT

	void FASTCALL LockCache() { ASSERT(this); EnterCriticalSection(&m_csAccess); }
										// �L���b�V���A�N�Z�X�J�n
	void FASTCALL UnlockCache() { ASSERT(this); LeaveCriticalSection(&m_csAccess); }
										// �L���b�V���A�N�Z�X�I��

	void FASTCALL CleanCache();
										// �S�L���b�V�����폜����
	void FASTCALL EraseCache(DWORD nUnit);
										// �Y�����郆�j�b�g�̃L���b�V�����폜����
	void FASTCALL DeleteCache(CHostPath* pPath);
										// �Y������L���b�V�����폜����
	CHostPath* FASTCALL FindCache(DWORD nUnit, const BYTE* szHuman);
										// �Y������p�X�����L���b�V������Ă��邩��������
	CHostPath* FASTCALL CopyCache(DWORD nUnit, const BYTE* szHuman, TCHAR* szWin32Buffer = NULL);
										// �L���b�V���������ɁAWin32�����l������
	CHostPath* FASTCALL MakeCache(CWindrv* ps, DWORD nUnit, const BYTE* szHuman, TCHAR* szWin32Buffer = NULL);
										// Win32���̌����ɕK�v�ȏ������ׂĎ擾����

	TCHAR* FASTCALL GetBase(DWORD nUnit) const;
										// �x�[�X�p�X�����擾����
	BOOL FASTCALL isWriteProtect(CWindrv* ps) const;
										// �������݋֎~���ǂ����m�F����
	BOOL FASTCALL isMediaOffline(CWindrv* ps, BOOL bMedia = TRUE);
										// �ᑬ���f�B�A�`�F�b�N�ƃI�t���C����ԃ`�F�b�N
	BYTE FASTCALL GetMediaByte(DWORD nUnit) const;
										// ���f�B�A�o�C�g�̎擾
	DWORD FASTCALL GetStatus(DWORD nUnit) const;
										// �h���C�u��Ԃ̎擾
	void FASTCALL CheckTimeout();
										// �S�h���C�u�̃^�C���A�E�g�`�F�b�N
#ifdef XM6_HOST_UPDATE_BY_SEQUENCE
	void FASTCALL SetMediaUpdate(CWindrv* ps, BOOL bDisable);
										// �����[�o�u�����f�B�A�̏�ԍX�V��L���ɂ���
#endif // XM6_HOST_UPDATE_BY_SEQUENCE
	BOOL FASTCALL CheckMediaUpdate(CWindrv* ps);
										// �����[�o�u�����f�B�A�̏�ԍX�V�`�F�b�N
	BOOL FASTCALL CheckMediaAccess(DWORD nUnit, BOOL bErase);
										// �����[�o�u�����f�B�A�̃A�N�Z�X���O�`�F�b�N
	BOOL FASTCALL Eject(DWORD nUnit);
										// �C�W�F�N�g
	void FASTCALL GetVolume(DWORD nUnit, TCHAR* szLabel);
										// �{�����[�����x���̎擾
	BOOL FASTCALL GetVolumeCache(DWORD nUnit, TCHAR* szLabel);
										// �L���b�V������{�����[�����x�����擾
	DWORD FASTCALL GetCapacity(DWORD nUnit, Human68k::capacity_t* pCapacity);
										// �e�ʂ̎擾
	BOOL FASTCALL GetCapacityCache(DWORD nUnit, Human68k::capacity_t* pCapacity);
										// �L���b�V������N���X�^�T�C�Y���擾

	// �t�@�C���V�X�e����Ԓʒm�p �O��API
	void FASTCALL ShellNotify(DWORD nEvent, const TCHAR* szPath);
										// �t�@�C���V�X�e����Ԓʒm

private:
	static inline const BYTE* FASTCALL SeparatePath(const BYTE* szHuman);
										// Human68k�t���p�X������Ō�̗v�f�𕪗�
	static inline const BYTE* FASTCALL SeparateCopyFilename(const BYTE* szHuman, BYTE* szBuffer);
										// Human68k�t���p�X������擪�̗v�f�𕪗��E�R�s�[

	CRITICAL_SECTION m_csAccess;
										// �r������p
	CWinFileDrv** m_ppBase;
										// Win32���̂փA�N�Z�X����ۂ̃x�[�X�p�X���

#ifdef XM6_HOST_STRICT_TIMEOUT
	HANDLE m_hEvent;
										// �X���b�h��~�C�x���g
	HANDLE m_hThread;
										// �^�C���A�E�g�`�F�b�N�p�X���b�h
#else // XM6_HOST_STRICT_TIMEOUT
	DWORD m_nTimeout;
										// �Ō�Ƀ^�C���A�E�g�`�F�b�N���s�Ȃ�������
#endif // XM6_HOST_STRICT_TIMEOUT

	DWORD m_nRingPath;
										// ���݂̃p�X���ێ���
	CRing m_cRingPath;
										// �p�X�� �I�u�W�F�N�g�ێ�
};

//===========================================================================
//
//	�t�@�C����������
//
//===========================================================================
class CHostFiles: public CRing {
public:
	CHostFiles() { Clear(); }
										// �R���X�g���N�^
	virtual ~CHostFiles() { Free(); }
										// �f�X�g���N�^
	void FASTCALL Free() { ASSERT(this); Clear(); }
										// �ė��p�̂��߂̊J��
	void FASTCALL Clear();
										// ������
	void FASTCALL SetKey(DWORD nKey) { ASSERT(this); m_nKey = nKey; }
										// �����L�[�ݒ�
	BOOL FASTCALL isSameKey(DWORD nKey) const { ASSERT(this); return m_nKey == nKey; }
										// �����L�[��r
	void FASTCALL SetPath(DWORD nUnit, const Human68k::namests_t* pNamests);
										// �p�X���E�t�@�C����������Ő���
	BOOL FASTCALL isRootPath() const { return (strlen((char*)m_szHumanPath) == 1); }
										// ���[�g�f�B���N�g������
	void FASTCALL SetPathWildcard() { m_nHumanWildcard = 1; }
										// ���C���h�J�[�h�ɂ��t�@�C��������L����
	void FASTCALL SetPathOnly() { m_nHumanWildcard = 0xFF; }
										// �p�X���݂̂�L����
	void FASTCALL SetAttribute(DWORD nHumanAttribute) { m_nHumanAttribute = nHumanAttribute; }
										// ����������ݒ�
	BOOL FASTCALL Find(CWindrv* ps, CHostEntry* pEntry, BOOL bRemove = FALSE);
										// Win32�������� (�p�X�� + �t�@�C����(�ȗ���) + ����)
	void FASTCALL AddFilename();
										// Win32���Ƀt�@�C������ǉ�
	const TCHAR* FASTCALL GetPath() const { ASSERT(this); return m_szWin32Result; }
										// Win32�����擾
	Human68k::dirent_t* FASTCALL GetEntry() const { ASSERT(this); return (Human68k::dirent_t*)&m_dirEntry; }
										// Human68k�f�B���N�g���G���g�����擾
	DWORD FASTCALL GetAttribute() const { ASSERT(this); return m_dirEntry.attr; }
										// Human68k�������擾
	WORD FASTCALL GetDate() const { ASSERT(this); return m_dirEntry.date; }
										// Human68k���t���擾
	WORD FASTCALL GetTime() const { ASSERT(this); return m_dirEntry.time; }
										// Human68k�������擾
	DWORD FASTCALL GetSize() const { ASSERT(this); return m_dirEntry.size; }
										// Human68k�t�@�C���T�C�Y���擾
	const BYTE* FASTCALL GetHumanResult() const { ASSERT(this); return m_szHumanResult; }
										// Human68k�t�@�C�����������ʂ��擾

private:
	DWORD m_nKey;
										// Human68k��FILES�o�b�t�@�A�h���X 0�Ȃ疢�g�p
	DWORD m_nHumanUnit;
										// Human68k�̃��j�b�g�ԍ�
	BYTE m_szHumanPath[HUMAN68K_MAX_PATH];
										// Human68k�̃p�X��
	BYTE m_szHumanFilename[24];
										// Human68k�̃t�@�C����
	DWORD m_nHumanWildcard;
										// Human68k�̃��C���h�J�[�h���
	DWORD m_nHumanAttribute;
										// Human68k�̌�������
	CHostPath::find_t m_findNext;
										// ���񌟍��ʒu���
	Human68k::dirent_t m_dirEntry;
										// �������� Human68k�t�@�C�����
	BYTE m_szHumanResult[24];
										// �������� Human68k�t�@�C����
	TCHAR m_szWin32Result[_MAX_PATH];
										// �������� Win32�t���p�X��
};

//===========================================================================
//
//	�t�@�C�������̈� �}�l�[�W��
//
//===========================================================================
class CHostFilesManager {
public:
	CHostFilesManager();
										// �t�@�C�������̈� ������ (�N������)
	virtual ~CHostFilesManager();
										// �t�@�C�������̈� �m�F (�I����)
	void FASTCALL Init();
										// �t�@�C�������̈� ������ (�h���C�o�g���ݎ�)
	void FASTCALL Clean();
										// �t�@�C�������̈� �J�� (�N���E���Z�b�g��)
	CHostFiles* FASTCALL Alloc(DWORD nKey);
										// �t�@�C�������̈� �m��
	CHostFiles* FASTCALL Search(DWORD nKey);
										// �t�@�C�������̈� ����
	void FASTCALL Free(CHostFiles* p);
										// �t�@�C�������̈� �J��
private:
	CRing m_cRingFiles;
										// �t�@�C�������̈� �I�u�W�F�N�g�ێ�
};

//===========================================================================
//
//	FCB����
//
//===========================================================================
class CHostFcb: public CRing {
public:
	CHostFcb() { Clear(); }
										// �R���X�g���N�^
	virtual ~CHostFcb() { Free(); }
										// �f�X�g���N�^
	void FASTCALL Free() { ASSERT(this); Close(); Clear(); }
										// �ė��p���邽�߂̊J��
	void FASTCALL Clear();
										// �N���A
	void FASTCALL SetKey(DWORD nKey) { ASSERT(this); m_nKey = nKey; }
										// �����L�[�ݒ�
	BOOL FASTCALL SetOpenMode(DWORD nHumanMode);
										// �t�@�C���I�[�v�����[�h�̐ݒ�
	void FASTCALL SetFilename(const TCHAR* szFilename);
										// �t�@�C�����̐ݒ�
	HANDLE FASTCALL Create(DWORD nHumanAttribute, BOOL bForce);
										// �t�@�C���쐬
	HANDLE FASTCALL Open();
										// �t�@�C���I�[�v��/�n���h���l��
	DWORD FASTCALL ReadFile(void* pBuffer, DWORD nSize);
										// �t�@�C���ǂݍ���
	DWORD FASTCALL WriteFile(void* pBuffer, DWORD nSize);
										// �t�@�C����������
	DWORD FASTCALL SetFilePointer(DWORD nOffset, DWORD nMode = FILE_BEGIN);
										// �t�@�C���|�C���^�ݒ�
	DWORD FASTCALL SetFileTime(WORD nFatDate, WORD nFatTime);
										// �t�@�C�������ݒ�
	BOOL FASTCALL Close();
										// �t�@�C���N���[�Y
	BOOL FASTCALL isSameKey(DWORD nKey) const { ASSERT(this); return m_nKey == nKey; }
										// �����L�[��r

private:
	DWORD m_nKey;
										// Human68k��FCB�o�b�t�@�A�h���X 0�Ȃ疢�g�p
	DWORD m_nMode;
										// Win32�̃t�@�C���I�[�v�����[�h
	HANDLE m_hFile;
										// Win32�̃t�@�C���n���h��
	TCHAR m_szFilename[_MAX_PATH];
										// Win32�̃t�@�C����
};

//===========================================================================
//
//	FCB���� �}�l�[�W��
//
//===========================================================================
class CHostFcbManager {
public:
	CHostFcbManager();
										// FCB����̈� ������ (�N������)
	virtual ~CHostFcbManager();
										// FCB����̈� �m�F (�I����)
	void FASTCALL Init();
										// FCB����̈� ������ (�h���C�o�g���ݎ�)
	void FASTCALL Clean();
										// FCB����̈� �J�� (�N���E���Z�b�g��)
	CHostFcb* FASTCALL Alloc(DWORD nKey);
										// FCB����̈� �m��
	CHostFcb* FASTCALL Search(DWORD nKey);
										// FCB����̈� ����
	void FASTCALL Free(CHostFcb* p);
										// FCB����̈� �J��

private:
	CRing m_cRingFcb;
										// FCB����̈� �I�u�W�F�N�g�ێ�
};

//===========================================================================
//
//	Windows�t�@�C���V�X�e��
//
//===========================================================================
class CWinFileSys : public FileSys
{
public:
	// ��{�t�@���N�V����
	CWinFileSys();
										// �R���X�g���N�^
	virtual ~CWinFileSys();
										// �f�X�g���N�^
	void FASTCALL ApplyCfg(const Config *pConfig);
										// �ݒ�K�p

	// �������E�I��
	DWORD FASTCALL Init(CWindrv* ps, DWORD nDriveMax, const BYTE* pOption);
										// $40 - ������
	void FASTCALL Reset();
										// ���Z�b�g(�S�N���[�Y)

	// �R�}���h�n���h��
	int FASTCALL CheckDir(CWindrv* ps, const Human68k::namests_t* pNamests);
										// $41 - �f�B���N�g���`�F�b�N
	int FASTCALL MakeDir(CWindrv* ps, const Human68k::namests_t* pNamests);
										// $42 - �f�B���N�g���쐬
	int FASTCALL RemoveDir(CWindrv* ps, const Human68k::namests_t* pNamests);
										// $43 - �f�B���N�g���폜
	int FASTCALL Rename(CWindrv* ps, const Human68k::namests_t* pNamests, const Human68k::namests_t* pNamestsNew);
										// $44 - �t�@�C�����ύX
	int FASTCALL Delete(CWindrv* ps, const Human68k::namests_t* pNamests);
										// $45 - �t�@�C���폜
	int FASTCALL Attribute(CWindrv* ps, const Human68k::namests_t* pNamests, DWORD nHumanAttribute);
										// $46 - �t�@�C�������擾/�ݒ�
	int FASTCALL Files(CWindrv* ps, const Human68k::namests_t* pNamests, DWORD nKey, Human68k::files_t* pFiles);
										// $47 - �t�@�C������(First)
	int FASTCALL NFiles(CWindrv* ps, DWORD nKey, Human68k::files_t* pFiles);
										// $48 - �t�@�C������(Next)
	int FASTCALL Create(CWindrv* ps, const Human68k::namests_t* pNamests, DWORD nKey, Human68k::fcb_t* pFcb, DWORD nHumanAttribute, BOOL bForce);
										// $49 - �t�@�C���쐬
	int FASTCALL Open(CWindrv* ps, const Human68k::namests_t* pNamests, DWORD nKey, Human68k::fcb_t* pFcb);
										// $4A - �t�@�C���I�[�v��
	int FASTCALL Close(CWindrv* ps, DWORD nKey, Human68k::fcb_t* pFcb);
										// $4B - �t�@�C���N���[�Y
	int FASTCALL Read(CWindrv* ps, DWORD nKey, Human68k::fcb_t* pFcb, DWORD nAddress, DWORD nSize);
										// $4C - �t�@�C���ǂݍ���
	int FASTCALL Write(CWindrv* ps, DWORD nKey, Human68k::fcb_t* pFcb, DWORD nAddress, DWORD nSize);
										// $4D - �t�@�C����������
	int FASTCALL Seek(CWindrv* ps, DWORD nKey, Human68k::fcb_t* pFcb, DWORD nMode, int nOffset);
										// $4E - �t�@�C���V�[�N
	DWORD FASTCALL TimeStamp(CWindrv* ps, DWORD nKey, Human68k::fcb_t* pFcb, WORD nFatDate, WORD nFatTime);
										// $4F - �t�@�C�������擾/�ݒ�
	int FASTCALL GetCapacity(CWindrv* ps, Human68k::capacity_t *pCapacity);
										// $50 - �e�ʎ擾
	int FASTCALL CtrlDrive(CWindrv* ps, Human68k::ctrldrive_t* pCtrlDrive);
										// $51 - �h���C�u��Ԍ���/����
	int FASTCALL GetDPB(CWindrv* ps, Human68k::dpb_t* pDpb);
										// $52 - DPB�擾
	int FASTCALL DiskRead(CWindrv* ps, DWORD nAddress, DWORD nSector, DWORD nSize);
										// $53 - �Z�N�^�ǂݍ���
	int FASTCALL DiskWrite(CWindrv* ps, DWORD nAddress, DWORD nSector, DWORD nSize);
										// $54 - �Z�N�^��������
	int FASTCALL IoControl(CWindrv* ps, Human68k::ioctrl_t* pIoctrl, DWORD nFunction);
										// $55 - IOCTRL
	int FASTCALL Flush(CWindrv* ps);
										// $56 - �t���b�V��
	int FASTCALL CheckMedia(CWindrv* ps);
										// $57 - ���f�B�A�����`�F�b�N
	int FASTCALL Lock(CWindrv* ps);
										// $58 - �r������

	// �t�@�C���V�X�e����Ԓʒm�p �O��API
	void FASTCALL ShellNotify(DWORD nEvent, const TCHAR* szPath) { ASSERT(this); m_cEntry.ShellNotify(nEvent, szPath); }
										// �t�@�C���V�X�e����Ԓʒm

	enum {
		DrvMax = 10						// �h���C�u�ő��␔
	};

private:
	// �G���[�n���h��
	//DWORD FASTCALL GetLastError(DWORD nUnit) const;
										// Win32�ŏI�G���[�擾 Human68k�G���[�ɕϊ�

	// �����⏕�p
	DWORD FASTCALL GetOption() const { ASSERT(this); return m_nOption; }
										// �I�v�V�����擾
	void FASTCALL SetOption(DWORD nOption);
										// �I�v�V�����ݒ�
	void FASTCALL InitOption(const BYTE* pOption);
										// �I�v�V����������
	inline BOOL FASTCALL FilesVolume(CWindrv* ps, Human68k::files_t* pFiles);
										// �{�����[�����x���擾
	void FASTCALL CheckKernel(CWindrv* ps);
										// TwentyOne�I�v�V�����Ď�

	DWORD m_bResume;
										// �x�[�X�p�X��ԕ����L�� FALSE���ƃx�[�X�p�X�𖈉�X�L����
	DWORD m_nDrives;
										// �h���C�u��␔
	DWORD m_nFlag[DrvMax];
										// ����t���O���
	TCHAR m_szBase[DrvMax][_MAX_PATH];
										// �x�[�X�p�X���

	CWinFileDrv* m_pDrv[DrvMax];
										// �h���C�u�I�u�W�F�N�g���� CHostEntry���ŗ��p

	CHostFilesManager m_cFiles;
										// �t�@�C�������̈� �I�u�W�F�N�g�ێ�
	CHostFcbManager m_cFcb;
										// FCB����̈� �I�u�W�F�N�g�ێ�
	CHostEntry m_cEntry;
										// �f�B���N�g���G���g�� �I�u�W�F�N�g�ێ�
										// ������1�Â��݂���΂悢����new�������̂�u��

	DWORD m_nHostSectorCount;
										// �[���Z�N�^�ԍ�
	DWORD m_nHostSectorBuffer[XM6_HOST_PSEUDO_CLUSTER_MAX];
										// �[���Z�N�^�̎w���t�@�C������

	DWORD m_nKernel;
										// TwentyOne�I�v�V�����A�h���X / �����J�n�J�E���g
	DWORD m_nKernelSearch;
										// NUL�f�o�C�X�̐擪�A�h���X

	DWORD m_nOptionDefault;
										// ���Z�b�g���̓���t���O
	DWORD m_nOption;
										// ����t���O (�t�@�C���폜�֘A)
};

//===========================================================================
//
//	Host
//
//===========================================================================
class CHost : public CComponent
{
public:
	// ��{�t�@���N�V����
	CHost(CFrmWnd *pWnd);
										// �R���X�g���N�^
	BOOL FASTCALL Init();
										// ������
	void FASTCALL Cleanup();
										// �N���[���A�b�v
	void FASTCALL ApplyCfg(const Config *pConfig);
										// �ݒ�K�p

	// �t�@�C���V�X�e����Ԓʒm�p �O��API
	void FASTCALL ShellNotify(DWORD nEvent, const TCHAR* szPath) { ASSERT(this); m_WinFileSys.ShellNotify(nEvent, szPath); }
										// �t�@�C���V�X�e����Ԓʒm

private:
	CWinFileSys m_WinFileSys;
										// �t�@�C���V�X�e��
	Windrv *m_pWindrv;
										// Windrv
};

#endif // mfc_host_h
#endif // _WIN32
