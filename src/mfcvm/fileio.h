//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2005 �o�h�D(ytanaka@ipc-tokai.or.jp)
//	[ �t�@�C��I/O ]
//
//---------------------------------------------------------------------------

#if !defined(fileio_h)
#define fileio_h

//===========================================================================
//
//	�t�@�C��I/O
//
//===========================================================================
class Fileio
{
public:
	enum OpenMode {
		ReadOnly,						// �ǂݍ��݂̂�
		WriteOnly,						// �������݂̂�
		ReadWrite,						// �ǂݏ�������
		Append							// �A�y���h
	};

public:
	Fileio();
										// �R���X�g���N�^
	virtual ~Fileio();
										// �f�X�g���N�^
	BOOL FASTCALL Load(const Filepath& path, void *buffer, int size);
										// ROM,RAM���[�h
	BOOL FASTCALL Save(const Filepath& path, void *buffer, int size);
										// RAM�Z�[�u

#if defined(_WIN32)
	BOOL FASTCALL Open(LPCTSTR fname, OpenMode mode);
										// �I�[�v��
#endif	// _WIN32
	BOOL FASTCALL Open(const Filepath& path, OpenMode mode);
										// �I�[�v��
	BOOL FASTCALL Seek(long offset);
										// �V�[�N
	BOOL FASTCALL Read(void *buffer, int size);
										// �ǂݍ���
	BOOL FASTCALL Write(const void *buffer, int size);
										// ��������
	DWORD FASTCALL GetFileSize() const;
										// �t�@�C���T�C�Y�擾
	DWORD FASTCALL GetFilePos() const;
										// �t�@�C���ʒu�擾
	void FASTCALL Close();
										// �N���[�Y
	BOOL FASTCALL IsValid() const		{ return (BOOL)(handle != -1); }
										// �L���`�F�b�N
	int FASTCALL GetHandle() const		{ return handle; }
										// �n���h���擾

private:
	int handle;							// �t�@�C���n���h��
};

#endif	// fileio_h
