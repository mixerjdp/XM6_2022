//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2004 �o�h�D(ytanaka@ipc-tokai.or.jp)
//	Modified (C) 2006 co (cogood��gmail.com)
//	[ Windrv ]
//
//---------------------------------------------------------------------------

#if !defined(windrv_h)
#define windrv_h

#include "device.h"

//���ő�X���b�h��
#define WINDRV_THREAD_MAX	3

//��WINDRV�݊�����̃T�|�[�g
#define WINDRV_SUPPORT_COMPATIBLE

//�����O�o�͗L��
#ifdef _DEBUG
#define WINDRV_LOG
#endif // _DEBUG

class FileSys;

//---------------------------------------------------------------------------
//
//	�X�e�[�^�X�R�[�h��`
//
//---------------------------------------------------------------------------
#define FS_INVALIDFUNC		0xffffffff	// �����ȃt�@���N�V�����R�[�h�����s����
#define FS_FILENOTFND		0xfffffffe	// �w�肵���t�@�C����������Ȃ�
#define FS_DIRNOTFND		0xfffffffd	// �w�肵���f�B���N�g����������Ȃ�
#define FS_OVEROPENED		0xfffffffc	// �I�[�v�����Ă���t�@�C������������
#define FS_CANTACCESS		0xfffffffb	// �f�B���N�g����{�����[�����x���̓A�N�Z�X�s��
#define FS_NOTOPENED		0xfffffffa	// �w�肵���n���h���̓I�[�v������Ă��Ȃ�
#define FS_INVALIDMEM		0xfffffff9	// �������Ǘ��̈悪�j�󂳂ꂽ
#define FS_OUTOFMEM			0xfffffff8	// ���s�ɕK�v�ȃ��������Ȃ�
#define FS_INVALIDPTR		0xfffffff7	// �����ȃ������Ǘ��|�C���^���w�肵��
#define FS_INVALIDENV		0xfffffff6	// �s���Ȋ����w�肵��
#define FS_ILLEGALFMT		0xfffffff5	// ���s�t�@�C���̃t�H�[�}�b�g���ُ�
#define FS_ILLEGALMOD		0xfffffff4	// �I�[�v���̃A�N�Z�X���[�h���ُ�
#define FS_INVALIDPATH		0xfffffff3	// �t�@�C�����̎w��Ɍ�肪����
#define FS_INVALIDPRM		0xfffffff2	// �����ȃp�����[�^�ŃR�[������
#define FS_INVALIDDRV		0xfffffff1	// �h���C�u�w��Ɍ�肪����
#define FS_DELCURDIR		0xfffffff0	// �J�����g�f�B���N�g���͍폜�ł��Ȃ�
#define FS_NOTIOCTRL		0xffffffef	// IOCTRL�ł��Ȃ��f�o�C�X
#define FS_LASTFILE			0xffffffee	// ����ȏ�t�@�C����������Ȃ�
#define FS_CANTWRITE		0xffffffed	// �w��̃t�@�C���͏������݂ł��Ȃ�
#define FS_DIRALREADY		0xffffffec	// �w��̃f�B���N�g���͊��ɓo�^����Ă���
#define FS_CANTDELETE		0xffffffeb	// �t�@�C��������̂ō폜�ł��Ȃ�
#define FS_CANTRENAME		0xffffffea	// �t�@�C��������̂Ń��l�[���ł��Ȃ�
#define FS_DISKFULL			0xffffffe9	// �f�B�X�N����t�Ńt�@�C�������Ȃ�
#define FS_DIRFULL			0xffffffe8	// �f�B���N�g������t�Ńt�@�C�������Ȃ�
#define FS_CANTSEEK			0xffffffe7	// �w��̈ʒu�ɂ̓V�[�N�ł��Ȃ�
#define FS_SUPERVISOR		0xffffffe6	// �X�[�p�[�o�C�U��ԂŃX�[�p�o�C�U�w�肵��
#define FS_THREADNAME		0xffffffe5	// �����X���b�h�������݂���
#define FS_BUFWRITE			0xffffffe4	// �v���Z�X�ԒʐM�̃o�b�t�@�������݋֎~
#define FS_BACKGROUND		0xffffffe3	// �o�b�N�O���E���h�v���Z�X���N���ł��Ȃ�
#define FS_OUTOFLOCK		0xffffffe0	// ���b�N�̈悪����Ȃ�
#define FS_LOCKED			0xffffffdf	// ���b�N����Ă��ăA�N�Z�X�ł��Ȃ�
#define FS_DRIVEOPENED		0xffffffde	// �w��̃h���C�u�̓n���h�����I�[�v������Ă���
#define FS_LINKOVER			0xffffffdd	// �V���{���b�N�����N�l�X�g��16��𒴂���
#define FS_FILEEXIST		0xffffffb0	// �t�@�C�������݂���

#define FS_FATAL_INVALIDUNIT	0xFFFFFFA0	// �s���ȃ��j�b�g�ԍ�
#define FS_FATAL_INVALIDCOMMAND	0xFFFFFFA1	// �s���ȃR�}���h�ԍ�
#define FS_FATAL_WRITEPROTECT	0xFFFFFFA2	// �������݋֎~�ᔽ
#define FS_FATAL_MEDIAOFFLINE	0xFFFFFFA3	// ���f�B�A�������Ă��Ȃ�

//===========================================================================
//
//	Human68k
//
//===========================================================================
#define HUMAN68K_MAX_PATH	96
class Human68k {
public:
	// �t�@�C�������r�b�g
	enum {
		AT_READONLY	= 0x01,
		AT_HIDDEN	= 0x02,
		AT_SYSTEM	= 0x04,
		AT_VOLUME	= 0x08,
		AT_DIRECTORY= 0x10,
		AT_ARCHIVE	= 0x20,
		AT_ALL		= 0xFF,
	};

	// �t�@�C���I�[�v�����[�h
	enum {
		OP_READ		= 0,
		OP_WRITE	= 1,
		OP_READWRITE= 2,
	};

	// �V�[�N���
	enum {
		SK_BEGIN	= 0,
		SK_CURRENT	= 1,
		SK_END		= 2,
	};

	// namests�\����
	typedef struct {
		BYTE wildcard;			// ���C���h�J�[�h������
		BYTE drive;				// �h���C�u�ԍ�
		BYTE path[65];			// �p�X(�T�u�f�B���N�g��+/)
		BYTE name[8];			// �t�@�C���� (PADDING 0x20)
		BYTE ext[3];			// �g���q (PADDING 0x20)
		BYTE add[10];			// �t�@�C�����ǉ� (PADDING 0x00)

		// �擾(Unicode�Ή��\)
		void FASTCALL GetCopyPath(BYTE* szPath) const;
		void FASTCALL GetCopyFilename(BYTE* szFilename) const;
	} namests_t;

	// files�\����
	typedef struct {
		BYTE fatr;				// + 0 �������鑮��				�Ǎ���p
		BYTE drive;				// + 1 �h���C�u�ԍ�				�Ǎ���p
		DWORD sector;			// + 2 �f�B���N�g���̃Z�N�^No	DOS _FILES �擪�A�h���X�ő�p
		//WORD sector2;			// + 6 �f�B���N�g���̃Z�N�^No	�ڍוs�� ���g�p
		WORD offset;			// + 8 �f�B���N�g���̈ʒu		�ڍוs��
		//BYTE name[8];			// +10 ��Ɨp�t�@�C����			�Ǎ���p ���g�p
		//BYTE ext[3];			// +18 ��Ɨp�g���q				�Ǎ���p ���g�p
		BYTE attr;				// +21 �t�@�C������				������p
		WORD time;				// +22 �ŏI�ύX����				������p
		WORD date;				// +24 �ŏI�ύX����				������p
		DWORD size;				// +26 �t�@�C���T�C�Y			������p
		BYTE full[23];			// +30 �t���t�@�C����			������p
	} files_t;

	// FCB�\����
	typedef struct {
		//BYTE pad00[6];		// + 0�`+ 5
		DWORD fileptr;			// + 6�`+ 9	�t�@�C���|�C���^
		//BYTE pad01[4];		// +10�`+13
		WORD mode;				// +14�`+15	�I�[�v�����[�h
		//BYTE pad02[16];		// +16�`+31
		//DWORD zero;			// +32�`+35	�I�[�v���̂Ƃ�0���������܂�Ă��� ���g�p
		//BYTE name[8];			// +36�`+43	�t�@�C���� (PADDING 0x20) ���g�p
		//BYTE ext[3];			// +44�`+46	�g���q (PADDING 0x20) ���g�p
		BYTE attr;				// +47		�t�@�C������
		//BYTE add[10];			// +48�`+57	�t�@�C�����ǉ� (PADDING 0x00) ���g�p
		WORD time;				// +58�`+59	�ŏI�ύX����
		WORD date;				// +60�`+61	�ŏI�ύX����
		//WORD cluster;			// +62�`+63	�N���X�^�ԍ� ���g�p
		DWORD size;				// +64�`+67	�t�@�C���T�C�Y
		//BYTE pad03[28];		// +68�`+95
	} fcb_t;

	// capacity�\����
	typedef struct {
		WORD free;				// + 0 �g�p�\�ȃN���X�^��
		WORD clusters;			// + 2 ���N���X�^��
		WORD sectors;			// + 4 �N���X�^������̃Z�N�^��
		WORD bytes;				// + 6 �Z�N�^������̃o�C�g��
	} capacity_t;

	// ctrldrive�\����
	typedef struct {
		BYTE status;			// +13	���
	} ctrldrive_t;

	// DPB�\����
	typedef struct {
		WORD sector_size;		// + 0	1 �Z�N�^����̃o�C�g��
		BYTE cluster_size;		// + 2	1 �N���X�^����̃Z�N�^��-1
		BYTE shift;				// + 3	�N���X�^���Z�N�^�̃V�t�g��
		WORD fat_sector;		// + 4	FAT �̐擪�Z�N�^�ԍ�
		BYTE fat_max;			// + 6	FAT �̈�̌�
		BYTE fat_size;			// + 7	FAT �̐�߂�Z�N�^��(���ʕ�������)
		WORD file_max;			// + 8	���[�g�f�B���N�g���ɓ���t�@�C���̌�
		WORD data_sector;		// +10	�f�[�^�̈�̐擪�Z�N�^�ԍ�
		WORD cluster_max;		// +12	���N���X�^��+1
		WORD root_sector;		// +14	���[�g�f�B���N�g���̐擪�Z�N�^�ԍ�
		//DWORD driverentry;	// +16	�f�o�C�X�h���C�o�ւ̃|�C���^
		BYTE media;				// +20	���f�B�A���ʎq
		//BYTE flag;			// +21	DPB �g�p�t���O
	} dpb_t;

	// �f�B���N�g���G���g���\����
	typedef struct {
		BYTE name[8];			// + 0	�t�@�C���� (PADDING 0x20)
		BYTE ext[3];			// + 8	�g���q (PADDING 0x20)
		BYTE attr;				// +11	�t�@�C������
		BYTE add[10];			// +12	�t�@�C�����ǉ� (PADDING 0x00)
		WORD time;				// +22	�ŏI�ύX����
		WORD date;				// +24	�ŏI�ύX����
		WORD cluster;			// +26	�N���X�^�ԍ�
		DWORD size;				// +28	�t�@�C���T�C�Y
	} dirent_t;

	// IOCTRL�\����
	typedef union {
		BYTE buffer[8];
		DWORD param;
		WORD media;
	} ioctrl_t;
};

//===========================================================================
//
//	�R�}���h�n���h��
//
//===========================================================================
class CWindrv {
public:
	// ��{�t�@���N�V����
	CWindrv(Windrv* pWindrv, Memory* pMemory, DWORD nHandle = 0);
										// �R���X�g���N�^
	virtual ~CWindrv();
										// �f�X�g���N�^

	// �X���b�h����
	BOOL FASTCALL Start();
										// �X���b�h�N��
	BOOL FASTCALL Terminate();
										// �X���b�h��~

	// Windrv�����p����O��API
	void FASTCALL Execute(DWORD nA5);
										// �R�}���h���s
#ifdef WINDRV_SUPPORT_COMPATIBLE
	DWORD FASTCALL ExecuteCompatible(DWORD nA5);
										// �R�}���h���s (WINDRV�݊�)
#endif // WINDRV_SUPPORT_COMPATIBLE
	DWORD FASTCALL GetHandle() const { ASSERT(this); return m_nHandle; }
										// ���g�̃n���h��(�z��ԍ�)���l��
	BOOL FASTCALL isExecute() const { ASSERT(this); return m_bExecute; }
										// �R�}���h���s�����ǂ����m�F

	// CWindrvManager�����p����O��API
	void FASTCALL SetAlloc(BOOL bAlloc) { ASSERT(this); m_bAlloc = bAlloc; }
	BOOL FASTCALL isAlloc() const { ASSERT(this); return m_bAlloc; }
										// �n���h���g�p�����ǂ����m�F

	// FileSys�����p����O��API
	DWORD FASTCALL GetUnit() const { ASSERT(this); return unit; }
										// ���j�b�g�ԍ��擾
	Memory* FASTCALL GetMemory() const { ASSERT(this); return memory; }
										// ���j�b�g�ԍ��擾
	void FASTCALL LockXM() { ASSERT(this); if (m_bReady) ::LockVM(); }
										// VM�ւ̃A�N�Z�X�J�n
	void FASTCALL UnlockXM() { ASSERT(this); if (m_bReady) ::UnlockVM(); }
										// VM�ւ̃A�N�Z�X�I��
	void FASTCALL Ready();
										// �R�}���h������҂�����VM�X���b�h���s�ĊJ

	// �X���b�h����
	static DWORD WINAPI Run(VOID* pThis);
										// �X���b�h���s�J�n�|�C���g
	void FASTCALL Runner();
										// �X���b�h����

private:
	// �R�}���h�n���h��
	void FASTCALL ExecuteCommand();
										// �R�}���h�n���h��

	void FASTCALL InitDrive();
										// $40 - ������
	void FASTCALL CheckDir();
										// $41 - �f�B���N�g���`�F�b�N
	void FASTCALL MakeDir();
										// $42 - �f�B���N�g���쐬
	void FASTCALL RemoveDir();
										// $43 - �f�B���N�g���폜
	void FASTCALL Rename();
										// $44 - �t�@�C�����ύX
	void FASTCALL Delete();
										// $45 - �t�@�C���폜
	void FASTCALL Attribute();
										// $46 - �t�@�C�������擾/�ݒ�
	void FASTCALL Files();
										// $47 - �t�@�C������(First)
	void FASTCALL NFiles();
										// $48 - �t�@�C������(NFiles)
	void FASTCALL Create();
										// $49 - �t�@�C���쐬
	void FASTCALL Open();
										// $4A - �t�@�C���I�[�v��
	void FASTCALL Close();
										// $4B - �t�@�C���N���[�Y
	void FASTCALL Read();
										// $4C - �t�@�C���ǂݍ���
	void FASTCALL Write();
										// $4D - �t�@�C����������
	void FASTCALL Seek();
										// $4E - �t�@�C���V�[�N
	void FASTCALL TimeStamp();
										// $4F - �t�@�C�������擾/�ݒ�
	void FASTCALL GetCapacity();
										// $50 - �e�ʎ擾
	void FASTCALL CtrlDrive();
										// $51 - �h���C�u��Ԍ���/����
	void FASTCALL GetDPB();
										// $52 - DPB�擾
	void FASTCALL DiskRead();
										// $53 - �Z�N�^�ǂݍ���
	void FASTCALL DiskWrite();
										// $54 - �Z�N�^��������
	void FASTCALL IoControl();
										// $55 - IOCTRL
	void FASTCALL Flush();
										// $56 - �t���b�V��
	void FASTCALL CheckMedia();
										// $57 - ���f�B�A�����`�F�b�N
	void FASTCALL Lock();
										// $58 - �r������

	// �I���l
	void FASTCALL SetResult(DWORD result);
										// �I���l��������

	// �������A�N�Z�X
	DWORD FASTCALL GetByte(DWORD addr) const;
										// �o�C�g�ǂݍ���
	void FASTCALL SetByte(DWORD addr, DWORD data);
										// �o�C�g��������
	DWORD FASTCALL GetWord(DWORD addr) const;
										// ���[�h�ǂݍ���
	void FASTCALL SetWord(DWORD addr, DWORD data);
										// ���[�h��������
	DWORD FASTCALL GetLong(DWORD addr) const;
										// �����O�ǂݍ���
	void FASTCALL SetLong(DWORD addr, DWORD data);
										// �����O��������
	DWORD FASTCALL GetAddr(DWORD addr) const;
										// �A�h���X�ǂݍ���
	void FASTCALL SetAddr(DWORD addr, DWORD data);
										// �A�h���X��������

	// �\���̕ϊ�
	void FASTCALL GetNameStsPath(DWORD addr, Human68k::namests_t* pNamests) const;
										// NAMESTS�p�X���ǂݍ���
	void FASTCALL GetNameSts(DWORD addr, Human68k::namests_t* pNamests) const;
										// NAMESTS�ǂݍ���
	void FASTCALL GetFiles(DWORD addr, Human68k::files_t* pFiles) const;
										// FILES�ǂݍ���
	void FASTCALL SetFiles(DWORD addr, const Human68k::files_t* pFiles);
										// FILES��������
	void FASTCALL GetFcb(DWORD addr, Human68k::fcb_t* pFcb) const;
										// FCB�ǂݍ���
	void FASTCALL SetFcb(DWORD addr, const Human68k::fcb_t* pFcb);
										// FCB��������
	void FASTCALL SetCapacity(DWORD addr, const Human68k::capacity_t* pCapacity);
										// CAPACITY��������
	void FASTCALL SetDpb(DWORD addr, const Human68k::dpb_t* pDpb);
										// DPB��������
	void FASTCALL GetIoctrl(DWORD param, DWORD func, Human68k::ioctrl_t* pIoctrl);
										// IOCTRL�ǂݍ���
	void FASTCALL SetIoctrl(DWORD param, DWORD func, const Human68k::ioctrl_t* pIoctrl);
										// IOCTRL��������
	void FASTCALL GetParameter(DWORD addr, BYTE* pOption, DWORD nSize);
										// �p�����[�^�ǂݍ���

#ifdef WINDRV_LOG
	// ���O
	void Log(DWORD level, char* format, ...) const;
										// ���O�o��
#endif // WINDRV_LOG

	// �R�}���h�n���h���p
	Windrv* windrv;
										// �ݒ���e
	Memory* memory;
										// ������
	DWORD a5;
										// ���N�G�X�g�w�b�_
	DWORD unit;
										// ���j�b�g�ԍ�
	DWORD command;
										// �R�}���h�ԍ�

	// �X���b�h�Ǘ��p
	DWORD m_nHandle;
										// �o�b�t�@�ԍ����n���h���Ƃ��Ďg��(0�`THREAD_MAX - 1)
	BOOL m_bAlloc;
										// �n���h���g�p���̂Ƃ�TRUE
	BOOL m_bExecute;
										// �R�}���h���s���̂Ƃ�TRUE
	BOOL m_bReady;
										// VM�X���b�h���s�ĊJ�̂Ƃ�TRUE
	BOOL m_bTerminate;
										// �X���b�h���p�I���̂Ƃ�TRUE
	HANDLE m_hThread;
										// �X���b�h�n���h��
	HANDLE m_hEventStart;
										// �R�}���h���s�J�n�ʒm
	HANDLE m_hEventReady;
										// VM�X���b�h���s�ĊJ�ʒm
};

//===========================================================================
//
//	�R�}���h�n���h���Ǘ�
//
//===========================================================================
class CWindrvManager {
public:
	void FASTCALL Init(Windrv* pWindrv, Memory* pMemory);
										// ������
	void FASTCALL Clean();
										// �I��
	void FASTCALL Reset();
										// ���Z�b�g

	CWindrv* FASTCALL Alloc();
										// �󂫃X���b�h�m��
	CWindrv* FASTCALL Search(DWORD nHandle);
										// �X���b�h����
	void FASTCALL Free(CWindrv* p);
										// �X���b�h�J��

private:
	CWindrv* m_pThread[WINDRV_THREAD_MAX];
										// �R�}���h�n���h������
};

//===========================================================================
//
//	Windrv
//
//===========================================================================
class Windrv : public MemDevice
{
public:
	// Windrv����
	enum {
		WINDRV_MODE_NONE = 0,
		WINDRV_MODE_ENABLE = 1,
		WINDRV_MODE_COMPATIBLE = 2,
		WINDRV_MODE_DUAL = 3,
		WINDRV_MODE_DISABLE = 255,
	};

	// �����f�[�^��`
	typedef struct {
		// �R���t�B�O
		DWORD enable;					// Windrv�T�|�[�g 0:���� 1:WindrvXM (2:Windrv�݊�)

		// �h���C�u�E�t�@�C���Ǘ�
		DWORD drives;					// �m�ۂ��ꂽ�h���C�u�� (FileSys�A�N�Z�X���E�`�F�b�N�p)

		// �v���Z�X
		FileSys *fs;					// �t�@�C���V�X�e��
	} windrv_t;

public:
	// ��{�t�@���N�V����
	Windrv(VM *p);
										// �R���X�g���N�^
	BOOL FASTCALL Init();
										// ������
	void FASTCALL Cleanup();
										// �N���[���A�b�v
	void FASTCALL Reset();
										// ���Z�b�g
	BOOL FASTCALL Save(Fileio *fio, int ver);
										// �Z�[�u
	BOOL FASTCALL Load(Fileio *fio, int ver);
										// ���[�h
	void FASTCALL ApplyCfg(const Config *config);
										// �ݒ�K�p

	// �������f�o�C�X
	DWORD FASTCALL ReadByte(DWORD addr);
										// �o�C�g�ǂݍ���
	void FASTCALL WriteByte(DWORD addr, DWORD data);
										// �o�C�g��������
	DWORD FASTCALL ReadOnly(DWORD addr) const;
										// �ǂݍ��݂̂�

	// �O��API
	void FASTCALL SetFileSys(FileSys *fs);
										// �t�@�C���V�X�e���ݒ�

	// �R�}���h�n���h���p �O��API
	void FASTCALL SetUnitMax(DWORD nUnitMax) { ASSERT(this); windrv.drives = nUnitMax; }
										// ���݂̃��j�b�g�ԍ�����ݒ�
	BOOL FASTCALL isInvalidUnit(DWORD unit) const;
										// ���j�b�g�ԍ��`�F�b�N
#ifdef _DEBUG
	BOOL FASTCALL isValidUnit(DWORD unit) const { ASSERT(this); return unit < windrv.drives; }
										// ���j�b�g�ԍ��`�F�b�N (ASSERT��p)
#endif // _DEBUG
	FileSys* FASTCALL GetFilesystem() const { ASSERT(this); return windrv.fs; }
										// �t�@�C���V�X�e���̊l��

#ifdef WINDRV_LOG
	// ���O
	void FASTCALL Log(DWORD level, char* message) const;
										// ���O�o��
#endif // WINDRV_LOG

private:
#ifdef WINDRV_SUPPORT_COMPATIBLE
	void FASTCALL Execute();
										// �R�}���h���s
#endif // WINDRV_SUPPORT_COMPATIBLE
	void FASTCALL ExecuteAsynchronous();
										// �R�}���h���s�J�n �񓯊�
	DWORD FASTCALL StatusAsynchronous();
										// �X�e�[�^�X�l�� �񓯊�
	void FASTCALL ReleaseAsynchronous();
										// �n���h���J�� �񓯊�

	windrv_t windrv;
										// �����f�[�^
	CWindrvManager m_cThread;
										// �R�}���h�n���h���Ǘ�
};

//===========================================================================
//
//	�t�@�C���V�X�e��
//
//	���ׂď������z�֐��Ƃ���B
//
//===========================================================================
class FileSys
{
public:
	// ��{�t�@���N�V����
	//FileSys();
										// �R���X�g���N�^
	//virtual ~FileSys() = 0;
										// �f�X�g���N�^

	// �������E�I��
	virtual DWORD FASTCALL Init(CWindrv* ps, DWORD nDriveMax, const BYTE* pOption) = 0;
										// $40 - ������
	virtual void FASTCALL Reset() = 0;
										// ���Z�b�g(�S�N���[�Y)

	// �R�}���h�n���h��
	virtual int FASTCALL CheckDir(CWindrv* ps, const Human68k::namests_t* pNamests) = 0;
										// $41 - �f�B���N�g���`�F�b�N
	virtual int FASTCALL MakeDir(CWindrv* ps, const Human68k::namests_t* pNamests) = 0;
										// $42 - �f�B���N�g���쐬
	virtual int FASTCALL RemoveDir(CWindrv* ps, const Human68k::namests_t* pNamests) = 0;
										// $43 - �f�B���N�g���폜
	virtual int FASTCALL Rename(CWindrv* ps, const Human68k::namests_t* pNamests, const Human68k::namests_t* pNamestsNew) = 0;
										// $44 - �t�@�C�����ύX
	virtual int FASTCALL Delete(CWindrv* ps, const Human68k::namests_t* pNamests) = 0;
										// $45 - �t�@�C���폜
	virtual int FASTCALL Attribute(CWindrv* ps, const Human68k::namests_t* pNamests, DWORD nHumanAttribute) = 0;
										// $46 - �t�@�C�������擾/�ݒ�
	virtual int FASTCALL Files(CWindrv* ps, const Human68k::namests_t* pNamests, DWORD nKey, Human68k::files_t* info) = 0;
										// $47 - �t�@�C������(First)
	virtual int FASTCALL NFiles(CWindrv* ps, DWORD nKey, Human68k::files_t* info) = 0;
										// $48 - �t�@�C������(Next)
	virtual int FASTCALL Create(CWindrv* ps, const Human68k::namests_t* pNamests, DWORD nKey, Human68k::fcb_t* pFcb, DWORD nAttribute, BOOL bForce) = 0;
										// $49 - �t�@�C���쐬
	virtual int FASTCALL Open(CWindrv* ps, const Human68k::namests_t* pNamests, DWORD nKey, Human68k::fcb_t* pFcb) = 0;
										// $4A - �t�@�C���I�[�v��
	virtual int FASTCALL Close(CWindrv* ps, DWORD nKey, Human68k::fcb_t* pFcb) = 0;
										// $4B - �t�@�C���N���[�Y
	virtual int FASTCALL Read(CWindrv* ps, DWORD nKey, Human68k::fcb_t* pFcb, DWORD nAddress, DWORD nSize) = 0;
										// $4C - �t�@�C���ǂݍ���
	virtual int FASTCALL Write(CWindrv* ps, DWORD nKey, Human68k::fcb_t* pFcb, DWORD nAddress, DWORD nSize) = 0;
										// $4D - �t�@�C����������
	virtual int FASTCALL Seek(CWindrv* ps, DWORD nKey, Human68k::fcb_t* pFcb, DWORD nMode, int nOffset) = 0;
										// $4E - �t�@�C���V�[�N
	virtual DWORD FASTCALL TimeStamp(CWindrv* ps, DWORD nKey, Human68k::fcb_t* pFcb, WORD nFatDate, WORD nFatTime) = 0;
										// $4F - �t�@�C�������擾/�ݒ�
	virtual int FASTCALL GetCapacity(CWindrv* ps, Human68k::capacity_t* cap) = 0;
										// $50 - �e�ʎ擾
	virtual int FASTCALL CtrlDrive(CWindrv* ps, Human68k::ctrldrive_t* pCtrlDrive) = 0;
										// $51 - �h���C�u��Ԍ���/����
	virtual int FASTCALL GetDPB(CWindrv* ps, Human68k::dpb_t* pDpb) = 0;
										// $52 - DPB�擾
	virtual int FASTCALL DiskRead(CWindrv* ps, DWORD nAddress, DWORD nSector, DWORD nSize) = 0;
										// $53 - �Z�N�^�ǂݍ���
	virtual int FASTCALL DiskWrite(CWindrv* ps, DWORD nAddress, DWORD nSector, DWORD nSize) = 0;
										// $54 - �Z�N�^��������
	virtual int FASTCALL IoControl(CWindrv* ps, Human68k::ioctrl_t* pIoctrl, DWORD nFunction) = 0;
										// $55 - IOCTRL
	virtual int FASTCALL Flush(CWindrv* ps) = 0;
										// $56 - �t���b�V��
	virtual int FASTCALL CheckMedia(CWindrv* ps) = 0;
										// $57 - ���f�B�A�����`�F�b�N
	virtual int FASTCALL Lock(CWindrv* ps) = 0;
										// $58 - �r������

	// �G���[�n���h��
	//virtual DWORD FASTCALL GetLastError(DWORD nUnit) const = 0;
										// �ŏI�G���[�擾
};

#endif // windrv_h
