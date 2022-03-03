//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 �o�h�D(ytanaka@ipc-tokai.or.jp)
//	[ �O���t�B�b�NVRAM ]
//
//---------------------------------------------------------------------------

#if !defined(gvram_h)
#define gvram_h

#include "device.h"
#include "crtc.h"

//===========================================================================
//
//	�O���t�B�b�NVRAM�n���h��
//
//===========================================================================
class GVRAMHandler
{
public:
	GVRAMHandler(Render *rend, BYTE *mem, CPU *p);
										// �R���X�g���N�^
	virtual DWORD FASTCALL ReadByte(DWORD addr) = 0;
										// �o�C�g�ǂݍ���
	virtual DWORD FASTCALL ReadWord(DWORD addr) = 0;
										// ���[�h�ǂݍ���
	virtual void FASTCALL WriteByte(DWORD addr, DWORD data) = 0;
										// �o�C�g��������
	virtual void FASTCALL WriteWord(DWORD addr, DWORD data) = 0;
										// ���[�h��������
	virtual DWORD FASTCALL ReadOnly(DWORD addr) const = 0;
										// �ǂݍ��݂̂�

protected:
	Render *render;
										// �����_��
	BYTE *gvram;
										// �O���t�B�b�NVRAM
	CPU *cpu;
										// CPU
};

//===========================================================================
//
//	�O���t�B�b�NVRAM�n���h��(1024)
//
//===========================================================================
class GVRAM1024 : public GVRAMHandler
{
public:
	GVRAM1024(Render *render, BYTE *gvram, CPU *p);
										// �R���X�g���N�^
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
};

//===========================================================================
//
//	�O���t�B�b�NVRAM�n���h��(16�F)
//
//===========================================================================
class GVRAM16 : public GVRAMHandler
{
public:
	GVRAM16(Render *render, BYTE *gvram, CPU *p);
										// �R���X�g���N�^
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
};

//===========================================================================
//
//	�O���t�B�b�NVRAM�n���h��(256�F)
//
//===========================================================================
class GVRAM256 : public GVRAMHandler
{
public:
	GVRAM256(Render *render, BYTE *gvram, CPU *p);
										// �R���X�g���N�^
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
};

//===========================================================================
//
//	�O���t�B�b�NVRAM�n���h��(����)
//
//===========================================================================
class GVRAMNDef : public GVRAMHandler
{
public:
	GVRAMNDef(Render *render, BYTE *gvram, CPU *p);
										// �R���X�g���N�^
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
};

//===========================================================================
//
//	�O���t�B�b�NVRAM�n���h��(65536�F)
//
//===========================================================================
class GVRAM64K : public GVRAMHandler
{
public:
	GVRAM64K(Render *render, BYTE *gvram, CPU *p);
										// �R���X�g���N�^
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
};

//===========================================================================
//
//	�O���t�B�b�NVRAM
//
//===========================================================================
class GVRAM : public MemDevice
{
public:
	// �������[�N��`
	typedef struct {
		BOOL mem;						// 512KB�P���������t���O
		DWORD siz;						// 1024�~1024�t���O
		DWORD col;						// 16, 256, ����`, 65536
		int type;						// �n���h���^�C�v(0�`4)
		DWORD mask[4];					// �����N���A �}�X�N
		BOOL plane[4];					// �����N���A �v���[��
	} gvram_t;

public:
	// ��{�t�@���N�V����
	GVRAM(VM *p);
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
	void FASTCALL SetType(DWORD type);
										// GVRAM�^�C�v�ݒ�
	void FASTCALL FastSet(DWORD mask);
										// �����N���A�ݒ�
	void FASTCALL FastClr(const CRTC::crtc_t *p);
										// �����N���A
	const BYTE* FASTCALL GetGVRAM() const;
										// GVRAM�擾

private:
	void FASTCALL FastClr768(const CRTC::crtc_t *p);
										// �����N���A 1024x1024 512/768
	void FASTCALL FastClr256(const CRTC::crtc_t *p);
										// �����N���A 1024x1024 256
	void FASTCALL FastClr512(const CRTC::crtc_t *p);
										// �����N���A 512x512
	Render *render;
										// �����_��
	BYTE *gvram;
										// �O���t�B�b�NVRAM
	GVRAMHandler *handler;
										// �������n���h��(�J�����g)
	GVRAM1024 *hand1024;
										// �������n���h��(1024)
	GVRAM16 *hand16;
										// �������n���h��(16�F)
	GVRAM256 *hand256;
										// �������n���h��(256�F)
	GVRAMNDef *handNDef;
										// �������n���h��(����)
	GVRAM64K *hand64K;
										// �������n���h��(64K�F)
	gvram_t gvdata;
										// �������[�N
	DWORD gvcount;
										// GVRAM�A�N�Z�X�J�E���g(version2.04�ȍ~)
};

#endif	// gvram_h
