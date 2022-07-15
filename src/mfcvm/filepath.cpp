//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 �o�h�D(ytanaka@ipc-tokai.or.jp)
//	[ �t�@�C���p�X ]
//
//---------------------------------------------------------------------------

#if defined(_WIN32)

#include "os.h"
#include "xm6.h"
#include "filepath.h"
#include "fileio.h"

//===========================================================================
//
//	�t�@�C���p�X
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	�R���X�g���N�^
//
//---------------------------------------------------------------------------
Filepath::Filepath()
{
	// �N���A
	Clear();

	// �X�V�Ȃ�
	m_bUpdate = FALSE;
}

//---------------------------------------------------------------------------
//
//	�f�X�g���N�^
//
//---------------------------------------------------------------------------
Filepath::~Filepath()
{
}

//---------------------------------------------------------------------------
//
//	������Z�q
//
//---------------------------------------------------------------------------
Filepath& Filepath::operator=(const Filepath& path)
{
	// �p�X�ݒ�(������Split�����)
	SetPath(path.GetPath());

	// ���t�y�эX�V�����擾
	m_bUpdate = FALSE;
	if (path.IsUpdate()) {
		m_bUpdate = TRUE;
		path.GetUpdateTime(&m_SavedTime, &m_CurrentTime);
	}

	return *this;
}

//---------------------------------------------------------------------------
//
//	�N���A
//
//---------------------------------------------------------------------------
void FASTCALL Filepath::Clear()
{
	ASSERT(this);

	// �p�X����ъe�������N���A
	m_szPath[0] = _T('\0');
	m_szDrive[0] = _T('\0');
	m_szDir[0] = _T('\0');
	m_szFile[0] = _T('\0');
	m_szExt[0] = _T('\0');
}

//---------------------------------------------------------------------------
//
//	�t�@�C���ݒ�(�V�X�e��)
//
//---------------------------------------------------------------------------
void FASTCALL Filepath::SysFile(SysFileType sys)
{
	int nFile;

	ASSERT(this);

	// �L���X�g
	nFile = (int)sys;

	// �t�@�C�����R�s�[
	_tcscpy(m_szPath, SystemFile[nFile]);

	// ����
	Split();

	// �x�[�X�f�B���N�g���ݒ�
	SetBaseDir();
}

//---------------------------------------------------------------------------
//
//	�t�@�C���ݒ�(���[�U)
//
//---------------------------------------------------------------------------
void FASTCALL Filepath::SetPath(LPCTSTR lpszPath)
{
	ASSERT(this);
	ASSERT(lpszPath);
	ASSERT(_tcslen(lpszPath) < _MAX_PATH);

	// �p�X���R�s�[
	_tcscpy(m_szPath, lpszPath);

	// ����
	Split();

	// �h���C�u���̓f�B���N�g���������Ă����OK
	if (_tcslen(m_szPath) > 0) {
		if (_tcslen(m_szDrive) == 0) {
			if (_tcslen(m_szDir) == 0) {
				// �J�����g�f�B���N�g���ݒ�
				SetCurDir();
			}
		}
	}
}

//---------------------------------------------------------------------------
//
//	�p�X����
//
//---------------------------------------------------------------------------
void FASTCALL Filepath::Split()
{
	ASSERT(this);

	// �p�[�c��������
	m_szDrive[0] = _T('\0');
	m_szDir[0] = _T('\0');
	m_szFile[0] = _T('\0');
	m_szExt[0] = _T('\0');

	// ����
	_tsplitpath(m_szPath, m_szDrive, m_szDir, m_szFile, m_szExt);
}

//---------------------------------------------------------------------------
//
//	�p�X����
//
//---------------------------------------------------------------------------
void FASTCALL Filepath::Make()
{
	ASSERT(this);

	// ����
	_tmakepath(m_szPath, m_szDrive, m_szDir, m_szFile, m_szExt);
}

//---------------------------------------------------------------------------
//
//	�x�[�X�f�B���N�g���ݒ�
//
//---------------------------------------------------------------------------
void FASTCALL Filepath::SetBaseDir()
{
	TCHAR szModule[_MAX_PATH];

	ASSERT(this);

	// ���W���[���̃p�X���𓾂�
	::GetModuleFileName(NULL, szModule, _MAX_PATH);

	// ����(�t�@�C�����Ɗg���q�͏������܂Ȃ�)
	_tsplitpath(szModule, m_szDrive, m_szDir, NULL, NULL);

	// ����
	Make();
}

//---------------------------------------------------------------------------
//
//	Establecer nombre base para el archivo de configuraci�n
//
//---------------------------------------------------------------------------
void FASTCALL Filepath::SetBaseFile(CString Nombre)
{
	TCHAR szModule[_MAX_PATH];

	ASSERT(this);
	ASSERT(_tcslen(m_szPath) > 0);

	// ���W���[���̃p�X���𓾂�
	::GetModuleFileName(NULL, szModule, _MAX_PATH);
	
	// En este lugar se determina el nombre del archivo de configuracion *-*
	_tsplitpath(szModule, m_szDrive, m_szDir, m_szFile, NULL);
	_tcscpy(m_szFile, Nombre);


	// ����
	Make();
}

//---------------------------------------------------------------------------
//
//	�J�����g�f�B���N�g���ݒ�
//
//---------------------------------------------------------------------------
void FASTCALL Filepath::SetCurDir()
{
	TCHAR szCurDir[_MAX_PATH];

	ASSERT(this);
	ASSERT(_tcslen(m_szPath) > 0);

	// �J�����g�f�B���N�g���擾
	::GetCurrentDirectory(_MAX_PATH, szCurDir);

	// ����(�t�@�C�����Ɗg���q�͖���)
	_tsplitpath(szCurDir, m_szDrive, m_szDir, NULL, NULL);

	// ����
	Make();
}

//---------------------------------------------------------------------------
//
//	�N���A����Ă��邩
//
//---------------------------------------------------------------------------
BOOL FASTCALL Filepath::IsClear() const
{
	// Clear()�̋t
	if ((m_szPath[0] == _T('\0')) &&
		(m_szDrive[0] == _T('\0')) &&
		(m_szDir[0] == _T('\0')) &&
		(m_szFile[0] == _T('\0')) &&
		(m_szExt[0] == _T('\0'))) {
		// �m���ɁA�N���A����Ă���
		return TRUE;
	}

	// �N���A����Ă��Ȃ�
	return FALSE;
}

//---------------------------------------------------------------------------
//
//	�V���[�g���擾
//	���Ԃ����|�C���^�͈ꎞ�I�Ȃ��́B�����R�s�[���邱��
//	��FDIDisk��disk.name�Ƃ̊֌W�ŁA������͍ő�59����+�I�[�Ƃ��邱��
//
//---------------------------------------------------------------------------
const char* FASTCALL Filepath::GetShort() const
{
	char *lpszFile;
	char *lpszExt;

	ASSERT(this);

	// TCHAR�����񂩂�char������֕ϊ�
	lpszFile = T2A((LPTSTR)&m_szFile[0]);
	lpszExt = T2A((LPTSTR)&m_szExt[0]);

	// �Œ�o�b�t�@�֍���
	strcpy(ShortName, lpszFile);
	strcat(ShortName, lpszExt);

	// strlen�Œ��ׂ��Ƃ��A�ő�59�ɂȂ�悤�ɍ׍H
	ShortName[59] = '\0';

	// const char�Ƃ��ĕԂ�
	return (const char*)ShortName;
}

//---------------------------------------------------------------------------
//
//	�t�@�C�����{�g���q�擾
//	���Ԃ����|�C���^�͈ꎞ�I�Ȃ��́B�����R�s�[���邱��
//
//---------------------------------------------------------------------------
LPCTSTR FASTCALL Filepath::GetFileExt() const
{
	ASSERT(this);

	// �Œ�o�b�t�@�֍���
	_tcscpy(FileExt, m_szFile);
	_tcscat(FileExt, m_szExt);

	// LPCTSTR�Ƃ��ĕԂ�
	return (LPCTSTR)FileExt;
}

//---------------------------------------------------------------------------
//
//	�p�X��r
//
//---------------------------------------------------------------------------
BOOL FASTCALL Filepath::CmpPath(const Filepath& path) const
{
	// �p�X�����S��v���Ă����TRUE
	if (_tcscmp(path.GetPath(), GetPath()) == 0) {
		return TRUE;
	}

	return FALSE;
}

//---------------------------------------------------------------------------
//
//	�f�t�H���g�f�B���N�g��������
//
//---------------------------------------------------------------------------
void FASTCALL Filepath::ClearDefaultDir()
{
	DefaultDir[0] = _T('\0');
}

//---------------------------------------------------------------------------
//
//	�f�t�H���g�f�B���N�g���ݒ�
//
//---------------------------------------------------------------------------
void FASTCALL Filepath::SetDefaultDir(LPCTSTR lpszPath)
{
	TCHAR szDrive[_MAX_DRIVE];
	TCHAR szDir[_MAX_DIR];

	ASSERT(lpszPath);

	// �^����ꂽ�p�X����A�h���C�u�ƃf�B���N�g���𐶐�
	_tsplitpath(lpszPath, szDrive, szDir, NULL, NULL);

	// �h���C�u�ƃf�B���N�g�����R�s�[
	_tcscpy(DefaultDir, szDrive);
	_tcscat(DefaultDir, szDir);
}

//---------------------------------------------------------------------------
//
//	�f�t�H���g�f�B���N�g���擾
//
//---------------------------------------------------------------------------
LPCTSTR FASTCALL Filepath::GetDefaultDir()
{
	return (LPCTSTR)DefaultDir;
}

//---------------------------------------------------------------------------
//
//	�Z�[�u
//
//---------------------------------------------------------------------------
BOOL FASTCALL Filepath::Save(Fileio *fio, int /*ver*/)
{
	TCHAR szPath[_MAX_PATH];
	CFile file;
	FILETIME ft;

	ASSERT(this);
	ASSERT(fio);

	// �[���N���A���āA�S�~�����������̂����
	memset(szPath, 0, sizeof(szPath));
	_tcscpy(szPath, m_szPath);

	// �t�@�C���p�X��ۑ�
	if (!fio->Write(szPath, sizeof(szPath))) {
		return FALSE;
	}

	// �t�@�C�����t���擾(2038�N��������邽�߁AWin32���擾)
	memset(&ft, 0, sizeof(ft));
	if (file.Open(szPath, CFile::modeRead)) {
		::GetFileTime((HANDLE)file.m_hFile, NULL, NULL, &ft);
		file.Close();
	}

	// �ŏI�������ݓ��t��ۑ�
	if (!fio->Write(&ft, sizeof(ft))) {
		return FALSE;
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	���[�h
//
//---------------------------------------------------------------------------
BOOL FASTCALL Filepath::Load(Fileio *fio, int /*ver*/)
{
	TCHAR szPath[_MAX_PATH];
	CFile file;

	ASSERT(this);
	ASSERT(fio);

	// �t���p�X��ǂݍ���
	if (!fio->Read(szPath, sizeof(szPath))) {
		return FALSE;
	}

	// �Z�b�g
	SetPath(szPath);

	// �ŏI�������ݓ��t��ǂݍ���
	if (!fio->Read(&m_SavedTime, sizeof(m_SavedTime))) {
		return FALSE;
	}

	// �t�@�C�����t���擾(2038�N��������邽�߁AWin32���擾)
	if (!file.Open(szPath, CFile::modeRead)) {
		// �t�@�C�������݂��Ȃ��Ă��A�G���[�Ƃ͂��Ȃ�
		return TRUE;
	}
	if (!::GetFileTime((HANDLE)file.m_hFile, NULL, NULL, &m_CurrentTime)) {
		return FALSE;
	}
	file.Close();

	// ft�̕����V���������ꍇ�A�X�V�t���OUp
	if (::CompareFileTime(&m_CurrentTime, &m_SavedTime) <= 0) {
		m_bUpdate = FALSE;
	}
	else {
		m_bUpdate = TRUE;
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	�Z�[�u��ɍX�V���ꂽ��
//
//---------------------------------------------------------------------------
BOOL FASTCALL Filepath::IsUpdate() const
{
	ASSERT(this);

	return m_bUpdate;
}

//---------------------------------------------------------------------------
//
//	�Z�[�u���ԏ����擾
//
//---------------------------------------------------------------------------
void FASTCALL Filepath::GetUpdateTime(FILETIME *pSaved, FILETIME *pCurrent) const
{
	ASSERT(this);
	ASSERT(m_bUpdate);

	// ���ԏ���n��
	*pSaved = m_SavedTime;
	*pCurrent = m_CurrentTime;
}

//---------------------------------------------------------------------------
//
//	�V�X�e���t�@�C���e�[�u��
//
//---------------------------------------------------------------------------
LPCTSTR Filepath::SystemFile[] = {
	_T("IPLROM.DAT"),
	_T("IPLROMXV.DAT"),
	_T("IPLROMCO.DAT"),
	_T("IPLROM30.DAT"),
	_T("ROM30.DAT"),
	_T("CGROM.DAT"),
	_T("CGROM.TMP"),
	_T("SCSIINROM.DAT"),
	_T("SCSIEXROM.DAT"),
	_T("SRAM.DAT")
};

//---------------------------------------------------------------------------
//
//	�V���[�g��
//
//---------------------------------------------------------------------------
char Filepath::ShortName[_MAX_FNAME + _MAX_DIR];

//---------------------------------------------------------------------------
//
//	�t�@�C�����{�g���q
//
//---------------------------------------------------------------------------
TCHAR Filepath::FileExt[_MAX_FNAME + _MAX_DIR];

//---------------------------------------------------------------------------
//
//	�f�t�H���g�f�B���N�g��
//
//---------------------------------------------------------------------------
TCHAR Filepath::DefaultDir[_MAX_PATH];

#endif	// WIN32
