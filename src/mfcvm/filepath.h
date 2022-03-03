//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 �o�h�D(ytanaka@ipc-tokai.or.jp)
//	[ �t�@�C���p�X ]
//
//---------------------------------------------------------------------------

#if !defined(filepath_h)
#define filepath_h

#if defined(_WIN32)

//---------------------------------------------------------------------------
//
//	�萔��`
//
//---------------------------------------------------------------------------
#define FILEPATH_MAX		_MAX_PATH

//===========================================================================
//
//	�t�@�C���p�X
//	��������Z�q��p�ӂ��邱��
//
//===========================================================================
class Filepath
{
public:
	// �V�X�e���t�@�C�����
	enum SysFileType {
		IPL,							// IPL(version 1.00)
		IPLXVI,							// IPL(version 1.10)
		IPLCompact,						// IPL(version 1.20)
		IPL030,							// IPL(version 1.30)�㔼
		ROM030,							// IPL(version 1.30)�O��
		CG,								// CG
		CGTMP,							// CG(Win����)
		SCSIInt,						// SCSI(����)
		SCSIExt,						// SCSI(�O�t)
		SRAM							// SRAM
	};

public:
	Filepath();
										// �R���X�g���N�^
	virtual ~Filepath();
										// �f�X�g���N�^
	Filepath& operator=(const Filepath& path);
										// ���

	void FASTCALL Clear();
										// �N���A
	void FASTCALL SysFile(SysFileType sys);
										// �t�@�C���ݒ�(�V�X�e��)
	void FASTCALL SetPath(LPCTSTR lpszPath);
										// �t�@�C���ݒ�(���[�U)
	void FASTCALL SetBaseDir();
										// �x�[�X�f�B���N�g���ݒ�
	void FASTCALL SetBaseFile();
										// �x�[�X�f�B���N�g���{�t�@�C�����ݒ�

	BOOL FASTCALL IsClear() const;
										// �N���A����Ă��邩
	LPCTSTR FASTCALL GetPath() const	{ return m_szPath; }
										// �p�X���擾
	const char* FASTCALL GetShort() const;
										// �V���[�g���擾(const char*)
	LPCTSTR FASTCALL GetFileExt() const;
										// �V���[�g���擾(LPCTSTR)
	BOOL FASTCALL CmpPath(const Filepath& path) const;
										// �p�X��r

	static void FASTCALL ClearDefaultDir();
										// �f�t�H���f�B���N�g����������
	static void FASTCALL SetDefaultDir(LPCTSTR lpszPath);
										// �f�t�H���g�f�B���N�g���ɐݒ�
	static LPCTSTR FASTCALL GetDefaultDir();
										// �f�t�H���g�f�B���N�g���擾

	BOOL FASTCALL Save(Fileio *fio, int ver);
										// �Z�[�u
	BOOL FASTCALL Load(Fileio *fio, int ver);
										// ���[�h

private:
	void FASTCALL Split();
										// �p�X����
	void FASTCALL Make();
										// �p�X����
	void FASTCALL SetCurDir();
										// �J�����g�f�B���N�g���ݒ�
	BOOL FASTCALL IsUpdate() const;
										// �Z�[�u��̍X�V���肩
	void FASTCALL GetUpdateTime(FILETIME *pSaved, FILETIME *pCurrent ) const;
										// �Z�[�u��̎��ԏ����擾
	TCHAR m_szPath[_MAX_PATH];
										// �t�@�C���p�X
	TCHAR m_szDrive[_MAX_DRIVE];
										// �h���C�u
	TCHAR m_szDir[_MAX_DIR];
										// �f�B���N�g��
	TCHAR m_szFile[_MAX_FNAME];
										// �t�@�C��
	TCHAR m_szExt[_MAX_EXT];
										// �g���q
	BOOL m_bUpdate;
										// �Z�[�u��̍X�V����
	FILETIME m_SavedTime;
										// �Z�[�u���̓��t
	FILETIME m_CurrentTime;
										// ���݂̓��t
	static LPCTSTR SystemFile[];
										// �V�X�e���t�@�C��
	static char ShortName[_MAX_FNAME + _MAX_DIR];
										// �V���[�g��(char)
	static TCHAR FileExt[_MAX_FNAME + _MAX_DIR];
										// �V���[�g��(TCHAR)
	static TCHAR DefaultDir[_MAX_PATH];
										// �f�t�H���g�f�B���N�g��
};

#endif	// _WIN32
#endif	// filepath_h
