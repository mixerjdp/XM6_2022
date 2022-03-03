//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 �o�h�D(ytanaka@ipc-tokai.or.jp)
//	[ �e�L�X�gVRAM ]
//
//---------------------------------------------------------------------------

#if !defined(tvram_h)
#define tvram_h

#include "device.h"

//===========================================================================
//
//	�e�L�X�gVRAM�n���h��
//
//===========================================================================
class TVRAMHandler
{
public:
	TVRAMHandler(Render *rend, BYTE *mem);
										// �R���X�g���N�^
	virtual void FASTCALL WriteByte(DWORD addr, DWORD data) = 0;
										// �o�C�g��������
	virtual void FASTCALL WriteWord(DWORD addr, DWORD data) = 0;
										// ���[�h��������

	// TVRAM���[�N�̃R�s�[
	DWORD multi;
										// �����A�N�Z�X(bit0-bit3)
	DWORD mask;
										// �A�N�Z�X�}�X�N(1�ŕύX�Ȃ�)
	DWORD rev;
										// �A�N�Z�X�}�X�N���]
	DWORD maskh;
										// �A�N�Z�X�}�X�N��ʃo�C�g
	DWORD revh;
										// �A�N�Z�X�}�X�N��ʔ��]

protected:
	Render *render;
										// �����_��
	BYTE *tvram;
										// �e�L�X�gVRAM
};

//===========================================================================
//
//	�e�L�X�gVRAM�n���h��(�ʏ�)
//
//===========================================================================
class TVRAMNormal : public TVRAMHandler
{
public:
	TVRAMNormal(Render *rend, BYTE *mem);
										// �R���X�g���N�^
	void FASTCALL WriteByte(DWORD addr, DWORD data);
										// �o�C�g��������
	void FASTCALL WriteWord(DWORD addr, DWORD data);
										// ���[�h��������
};

//===========================================================================
//
//	�e�L�X�gVRAM�n���h��(�}�X�N)
//
//===========================================================================
class TVRAMMask : public TVRAMHandler
{
public:
	TVRAMMask(Render *rend, BYTE *mem);
										// �R���X�g���N�^
	void FASTCALL WriteByte(DWORD addr, DWORD data);
										// �o�C�g��������
	void FASTCALL WriteWord(DWORD addr, DWORD data);
										// ���[�h��������
};

//===========================================================================
//
//	�e�L�X�gVRAM�n���h��(�}���`)
//
//===========================================================================
class TVRAMMulti : public TVRAMHandler
{
public:
	TVRAMMulti(Render *rend, BYTE *mem);
										// �R���X�g���N�^
	void FASTCALL WriteByte(DWORD addr, DWORD data);
										// �o�C�g��������
	void FASTCALL WriteWord(DWORD addr, DWORD data);
										// ���[�h��������
};

//===========================================================================
//
//	�e�L�X�gVRAM�n���h��(�}�X�N�{�}���`)
//
//===========================================================================
class TVRAMBoth : public TVRAMHandler
{
public:
	TVRAMBoth(Render *rend, BYTE *mem);
										// �R���X�g���N�^
	void FASTCALL WriteByte(DWORD addr, DWORD data);
										// �o�C�g��������
	void FASTCALL WriteWord(DWORD addr, DWORD data);
										// ���[�h��������
};

//===========================================================================
//
//	�e�L�X�gVRAM
//
//===========================================================================
class TVRAM : public MemDevice
{
public:
	// �����f�[�^��`
	typedef struct {
		DWORD multi;					// �����A�N�Z�X(bit0-bit3)
		DWORD mask;						// �A�N�Z�X�}�X�N(1�ŕύX�Ȃ�)
		DWORD rev;						// �A�N�Z�X�}�X�N���]
		DWORD maskh;					// �A�N�Z�X�}�X�N��ʃo�C�g
		DWORD revh;						// �A�N�Z�X�}�X�N��ʔ��]
		DWORD src;						// ���X�^�R�s�[ �����X�^
		DWORD dst;						// ���X�^�R�s�[ �惉�X�^
		DWORD plane;					// ���X�^�R�s�[ �Ώۃv���[��
	} tvram_t;

public:
	// ��{�t�@���N�V����
	TVRAM(VM *p);
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
#if !defined(NDEBUG)
	void FASTCALL AssertDiag() const;
										// �f�f
#endif	// NDEBUG

	// �������f�o�C�X
	DWORD FASTCALL ReadByte(DWORD addr);
										// �o�C�g�ǂݍ���
	DWORD FASTCALL ReadWord(DWORD addr);
										// ���[�h�ǂݍ���
	void FASTCALL WriteByte(DWORD addr, DWORD data);
										// �o�C�g��������
	void FASTCALL WriteWord(DWORD addr, DWORD data);
										// ���[�h��������
	DWORD FASTCALL ReadOnly(DWORD addr) const;
										// �ǂݍ��݂̂�

	// �O��API
	const BYTE* FASTCALL GetTVRAM() const;
										// TVRAM�擾
	void FASTCALL SetMulti(DWORD data);
										// �����������ݐݒ�
	void FASTCALL SetMask(DWORD data);
										// �A�N�Z�X�}�X�N�ݒ�
	void FASTCALL SetCopyRaster(DWORD src, DWORD dst, DWORD plane);
										// �R�s�[���X�^�w��
	void FASTCALL RasterCopy();
										// ���X�^�R�s�[����

private:
	void FASTCALL SelectHandler();
										// �n���h���I��
	TVRAMNormal *normal;
										// �n���h��(�ʏ�)
	TVRAMMask *mask;
										// �n���h��(�}�X�N)
	TVRAMMulti *multi;
										// �n���h��(�}���`)
	TVRAMBoth *both;
										// �n���h��(����)
	TVRAMHandler *handler;
										// �n���h��(���ݑI��)
	Render *render;
										// �����_��
	BYTE *tvram;
										// �e�L�X�gVRAM (512KB)
	tvram_t tvdata;
										// �����f�[�^
	DWORD tvcount;
										// TVRAM�A�N�Z�X�J�E���g(version2.04�ȍ~)
};

#endif	// tvram_h
