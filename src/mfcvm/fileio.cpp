//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 �o�h�D(ytanaka@ipc-tokai.or.jp)
//	[ �t�@�C��I/O ]
//
//---------------------------------------------------------------------------

#include "os.h"
#include "xm6.h"
#include "filepath.h"
#include "fileio.h"

#if defined(_WIN32)

//===========================================================================
//
//	�t�@�C��I/O
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	�R���X�g���N�^
//
//---------------------------------------------------------------------------
Fileio::Fileio()
{
	// ���[�N������
	handle = -1;
}

//---------------------------------------------------------------------------
//
//	�f�X�g���N�^
//
//---------------------------------------------------------------------------
Fileio::~Fileio()
{
	ASSERT(handle == -1);

	// Release�ł̈��S��
	Close();
}

//---------------------------------------------------------------------------
//
//	���[�h
//
//---------------------------------------------------------------------------
BOOL FASTCALL Fileio::Load(const Filepath& path, void *buffer, int size)
{
	ASSERT(this);
	ASSERT(buffer);
	ASSERT(size > 0);
	ASSERT(handle < 0);

	// �I�[�v��
	if (!Open(path, ReadOnly)) {
		return FALSE;
	}

	// �ǂݍ���
	if (!Read(buffer, size)) {
		Close();
		return FALSE;
	}

	// �N���[�Y
	Close();

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	�Z�[�u
//
//---------------------------------------------------------------------------
BOOL FASTCALL Fileio::Save(const Filepath& path, void *buffer, int size)
{
	ASSERT(this);
	ASSERT(buffer);
	ASSERT(size > 0);
	ASSERT(handle < 0);

	// �I�[�v��
	if (!Open(path, WriteOnly)) {
		return FALSE;
	}

	// �ǂݍ���
	if (!Write(buffer, size)) {
		Close();
		return FALSE;
	}

	// �N���[�Y
	Close();

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	�I�[�v��
//
//---------------------------------------------------------------------------
BOOL FASTCALL Fileio::Open(LPCTSTR fname, OpenMode mode)
{
	ASSERT(this);
	ASSERT(fname);
	ASSERT(handle < 0);

	// �k�������񂩂�̓ǂݍ��݂͕K�����s������
	if (fname[0] == _T('\0')) {
		handle = -1;
		return FALSE;
	}

	// ���[�h��
	switch (mode) {
		// �ǂݍ��݂̂�
		case ReadOnly:
			handle = _topen(fname, _O_BINARY | _O_RDONLY);
			break;

		// �������݂̂�
		case WriteOnly:
			handle = _topen(fname, _O_BINARY | _O_CREAT | _O_WRONLY | _O_TRUNC,
						_S_IWRITE);
			break;

		// �ǂݏ�������
		case ReadWrite:
			// CD-ROM����̓ǂݍ��݂�RW���������Ă��܂�
			if (_taccess(fname, 0x06) != 0) {
				return FALSE;
			}
			handle = _topen(fname, _O_BINARY | _O_RDWR);
			break;

		// �A�y���h
		case Append:
			handle = _topen(fname, _O_BINARY | _O_CREAT | _O_WRONLY | _O_APPEND,
						_S_IWRITE);
			break;

		// ����ȊO
		default:
			ASSERT(FALSE);
			break;
	}

	// ���ʕ]��
	if (handle == -1) {
		return FALSE;
	}
	ASSERT(handle >= 0);
	return TRUE;
}

//---------------------------------------------------------------------------
//
//	�I�[�v��
//
//---------------------------------------------------------------------------
BOOL FASTCALL Fileio::Open(const Filepath& path, OpenMode mode)
{
	ASSERT(this);

	return Open(path.GetPath(), mode);
}

//---------------------------------------------------------------------------
//
//	�ǂݍ���
//
//---------------------------------------------------------------------------
BOOL FASTCALL Fileio::Read(void *buffer, int size)
{
	int count;

	ASSERT(this);
	ASSERT(buffer);
	ASSERT(size > 0);
	ASSERT(handle >= 0);

	// �ǂݍ���
	count = _read(handle, buffer, size);
	if (count != size) {
		return FALSE;
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	��������
//
//---------------------------------------------------------------------------
BOOL FASTCALL Fileio::Write(const void *buffer, int size)
{
	int count;

	ASSERT(this);
	ASSERT(buffer);
	ASSERT(size > 0);
	ASSERT(handle >= 0);

	// �ǂݍ���
	count = _write(handle, buffer, size);
	if (count != size) {
		return FALSE;
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	�V�[�N
//
//---------------------------------------------------------------------------
BOOL FASTCALL Fileio::Seek(long offset)
{
	ASSERT(this);
	ASSERT(handle >= 0);
	ASSERT(offset >= 0);

	if (_lseek(handle, offset, SEEK_SET) != offset) {
		return FALSE;
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	�t�@�C���T�C�Y�擾
//
//---------------------------------------------------------------------------
DWORD FASTCALL Fileio::GetFileSize() const
{
#if defined(_MSC_VER)
	__int64 len;

	ASSERT(this);
	ASSERT(handle >= 0);

	// �t�@�C���T�C�Y��64bit�Ŏ擾
	len = _filelengthi64(handle);

	// ��ʂ�����΁A0xffffffff�Ƃ��ĕԂ�
	if (len >= 0x100000000i64) {
		return 0xffffffff;
	}

	// ���ʂ̂�
	return (DWORD)len;
#else
	ASSERT(this);
	ASSERT(handle >= 0);

	return (DWORD)filelength(handle);
#endif	// _MSC_VER
}

//---------------------------------------------------------------------------
//
//	�t�@�C���ʒu�擾
//
//---------------------------------------------------------------------------
DWORD FASTCALL Fileio::GetFilePos() const
{
	ASSERT(this);
	ASSERT(handle >= 0);

	// �t�@�C���ʒu��32bit�Ŏ擾
	return _tell(handle);
}

//---------------------------------------------------------------------------
//
//	�N���[�Y
//
//---------------------------------------------------------------------------
void FASTCALL Fileio::Close()
{
	ASSERT(this);

	if (handle != -1) {
		_close(handle);
		handle = -1;
	}
}

#endif	// _WIN32
