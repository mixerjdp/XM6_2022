//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 �o�h�D(ytanaka@ipc-tokai.or.jp)
//	[ PPI(i8255A) ]
//
//---------------------------------------------------------------------------

#if !defined(ppi_h)
#define ppi_h

#include "device.h"

//===========================================================================
//
//	PPI
//
//===========================================================================
class PPI : public MemDevice
{
public:
	// �萔��`
	enum {
		PortMax = 2,					// �|�[�g�ő吔
		AxisMax = 9,					// ���ő吔
		ButtonMax = 8					// �{�^���ő吔
	};

	// �W���C�X�e�B�b�N�f�[�^��`
	typedef struct {
		DWORD axis[AxisMax];				// �����
		BOOL button[ButtonMax];				// �{�^�����
	} joyinfo_t;

	// �����f�[�^��`
	typedef struct {
		DWORD portc;					// �|�[�gC
		int type[PortMax];				// �W���C�X�e�B�b�N�^�C�v
		DWORD ctl[PortMax];				// �W���C�X�e�B�b�N�R���g���[��
		joyinfo_t info[PortMax];		// �W���C�X�e�B�b�N���
	} ppi_t;

public:
	// ��{�t�@���N�V����
	PPI(VM *p);
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
#if defined(_DEBUG)
	void FASTCALL AssertDiag() const;
										// �f�f
#endif	// _DEBUG

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
	void FASTCALL GetPPI(ppi_t *buffer);
										// �����f�[�^�擾
	void FASTCALL SetJoyInfo(int port, const joyinfo_t *info);
										// �W���C�X�e�B�b�N���ݒ�
	const joyinfo_t* FASTCALL GetJoyInfo(int port) const;
										// �W���C�X�e�B�b�N���擾
	JoyDevice* FASTCALL CreateJoy(int port, int type);
										// �W���C�X�e�B�b�N�f�o�C�X�쐬

private:
	void FASTCALL SetPortC(DWORD data);
										// �|�[�gC�Z�b�g
	ADPCM *adpcm;
										// ADPCM
	JoyDevice *joy[PortMax];
										// �W���C�X�e�B�b�N
	ppi_t ppi;
										// �����f�[�^
};

//===========================================================================
//
//	�W���C�X�e�B�b�N�f�o�C�X
//
//===========================================================================
class JoyDevice
{
public:
	// ��{�t�@���N�V����
	JoyDevice(PPI *parent, int no);
										// �R���X�g���N�^
	virtual ~JoyDevice();
										// �f�X�g���N�^
	DWORD FASTCALL GetID() const		{ return id; }
										// ID�擾
	DWORD FASTCALL GetType() const		{ return type; }
										// �^�C�v�擾
	virtual void FASTCALL Reset();
										// ���Z�b�g
	virtual BOOL FASTCALL Save(Fileio *fio, int ver);
										// �Z�[�u
	virtual BOOL FASTCALL Load(Fileio *fio, int ver);
										// ���[�h

	// �A�N�Z�X
	virtual DWORD FASTCALL ReadPort(DWORD ctl);
										// �|�[�g�ǂݎ��
	virtual DWORD FASTCALL ReadOnly(DWORD ctl) const;
										// �|�[�g�ǂݎ��(Read Only)
	virtual void FASTCALL Control(DWORD ctl);
										// �R���g���[��

	// �L���b�V��
	void FASTCALL Notify()				{ changed = TRUE; }
										// �e�|�[�g�ύX�ʒm
	virtual void FASTCALL MakeData();
										// �f�[�^�쐬

	// �v���p�e�B
	int FASTCALL GetAxes() const		{ return axes; }
										// �����擾
	const char* FASTCALL GetAxisDesc(int axis) const;
										// ���\���擾
	int FASTCALL GetButtons() const		{ return buttons; }
										// �{�^�����擾
	const char* FASTCALL GetButtonDesc(int button) const;
										// �{�^���\���擾
	BOOL FASTCALL IsAnalog() const		{ return analog; }
										// �A�i���O�E�f�W�^���擾
	int FASTCALL GetDatas() const		{ return datas; }
										// �f�[�^���擾

protected:
	DWORD type;
										// �^�C�v
	DWORD id;
										// ID
	PPI *ppi;
										// PPI
	int port;
										// �|�[�g�ԍ�
	int axes;
										// ����
	const char **axis_desc;
										// ���\��
	int buttons;
										// �{�^����
	const char **button_desc;
										// �{�^���\��
	BOOL analog;
										// ���(�A�i���O�E�f�W�^��)
	DWORD *data;
										// �f�[�^����
	int datas;
										// �f�[�^��
	BOOL changed;
										// �W���C�X�e�B�b�N�ύX�ʒm
};

//===========================================================================
//
//	�W���C�X�e�B�b�N(ATARI�W��)
//
//===========================================================================
class JoyAtari : public JoyDevice
{
public:
	JoyAtari(PPI *parent, int no);
										// �R���X�g���N�^

protected:
	DWORD FASTCALL ReadOnly(DWORD ctl) const;
										// �|�[�g�ǂݎ��(Read Only)
	void FASTCALL MakeData();
										// �f�[�^�쐬

private:
	static const char* AxisDescTable[];
										// ���\���e�[�u��
	static const char* ButtonDescTable[];
										// �{�^���\���e�[�u��
};

//===========================================================================
//
//	�W���C�X�e�B�b�N(ATARI�W��+START/SELECT)
//
//===========================================================================
class JoyASS : public JoyDevice
{
public:
	JoyASS(PPI *parent, int no);
										// �R���X�g���N�^

protected:
	DWORD FASTCALL ReadOnly(DWORD ctl) const;
										// �|�[�g�ǂݎ��(Read Only)
	void FASTCALL MakeData();
										// �f�[�^�쐬

private:
	static const char* AxisDescTable[];
										// ���\���e�[�u��
	static const char* ButtonDescTable[];
										// �{�^���\���e�[�u��
};

//===========================================================================
//
//	�W���C�X�e�B�b�N(�T�C�o�[�X�e�B�b�N�E�A�i���O)
//
//===========================================================================
class JoyCyberA : public JoyDevice
{
public:
	JoyCyberA(PPI *parent, int no);
										// �R���X�g���N�^

protected:
	void FASTCALL Reset();
										// ���Z�b�g
	DWORD FASTCALL ReadPort(DWORD ctl);
										// �|�[�g�ǂݎ��
	DWORD FASTCALL ReadOnly(DWORD ctl) const;
										// �|�[�g�ǂݎ��(Read Only)
	void FASTCALL Control(DWORD ctl);
										// �R���g���[��
	void FASTCALL MakeData();
										// �f�[�^�쐬
	BOOL FASTCALL Save(Fileio *fio, int ver);
										// �Z�[�u
	BOOL FASTCALL Load(Fileio *fio, int ver);
										// ���[�h

private:
	DWORD seq;
										// �V�[�P���X
	DWORD ctrl;
										// �O��̃R���g���[��(0 or 1)
	DWORD hus;
										// ����p����
	Scheduler *scheduler;
										// �X�P�W���[��
	static const char* AxisDescTable[];
										// ���\���e�[�u��
	static const char* ButtonDescTable[];
										// �{�^���\���e�[�u��
};

//===========================================================================
//
//	�W���C�X�e�B�b�N(�T�C�o�[�X�e�B�b�N�E�f�W�^��)
//
//===========================================================================
class JoyCyberD : public JoyDevice
{
public:
	JoyCyberD(PPI *parent, int no);
										// �R���X�g���N�^

protected:
	DWORD FASTCALL ReadOnly(DWORD ctl) const;
										// �|�[�g�ǂݎ��(Read Only)
	void FASTCALL MakeData();
										// �f�[�^�쐬

private:
	static const char* AxisDescTable[];
										// ���\���e�[�u��
	static const char* ButtonDescTable[];
										// �{�^���\���e�[�u��
};

//===========================================================================
//
//	�W���C�X�e�B�b�N(MD3�{�^��)
//
//===========================================================================
class JoyMd3 : public JoyDevice
{
public:
	JoyMd3(PPI *parent, int no);
										// �R���X�g���N�^

protected:
	DWORD FASTCALL ReadOnly(DWORD ctl) const;
										// �|�[�g�ǂݎ��(Read Only)
	void FASTCALL MakeData();
										// �f�[�^�쐬

private:
	static const char* AxisDescTable[];
										// ���\���e�[�u��
	static const char* ButtonDescTable[];
										// �{�^���\���e�[�u��
};

//===========================================================================
//
//	�W���C�X�e�B�b�N(MD6�{�^��)
//
//===========================================================================
class JoyMd6 : public JoyDevice
{
public:
	JoyMd6(PPI *parent, int no);
										// �R���X�g���N�^

protected:
	void FASTCALL Reset();
										// ���Z�b�g
	DWORD FASTCALL ReadOnly(DWORD ctl) const;
										// �|�[�g�ǂݎ��(Read Only)
	void FASTCALL Control(DWORD ctl);
										// �R���g���[��
	void FASTCALL MakeData();
										// �f�[�^�쐬
	BOOL FASTCALL Save(Fileio *fio, int ver);
										// �Z�[�u
	BOOL FASTCALL Load(Fileio *fio, int ver);
										// ���[�h

private:
	DWORD seq;
										// �V�[�P���X
	DWORD ctrl;
										// �O��̃R���g���[��(0 or 1)
	DWORD hus;
										// ����p����
	Scheduler *scheduler;
										// �X�P�W���[��
	static const char* AxisDescTable[];
										// ���\���e�[�u��
	static const char* ButtonDescTable[];
										// �{�^���\���e�[�u��
};

//===========================================================================
//
//	�W���C�X�e�B�b�N(CPSF-SFC)
//
//===========================================================================
class JoyCpsf : public JoyDevice
{
public:
	JoyCpsf(PPI *parent, int no);
										// �R���X�g���N�^

protected:
	DWORD FASTCALL ReadOnly(DWORD ctl) const;
										// �|�[�g�ǂݎ��(Read Only)
	void FASTCALL MakeData();
										// �f�[�^�쐬

private:
	static const char* AxisDescTable[];
										// ���\���e�[�u��
	static const char* ButtonDescTable[];
										// �{�^���\���e�[�u��
};

//===========================================================================
//
//	�W���C�X�e�B�b�N(CPSF-MD)
//
//===========================================================================
class JoyCpsfMd : public JoyDevice
{
public:
	JoyCpsfMd(PPI *parent, int no);
										// �R���X�g���N�^

protected:
	DWORD FASTCALL ReadOnly(DWORD ctl) const;
										// �|�[�g�ǂݎ��(Read Only)
	void FASTCALL MakeData();
										// �f�[�^�쐬

private:
	static const char* AxisDescTable[];
										// ���\���e�[�u��
	static const char* ButtonDescTable[];
										// �{�^���\���e�[�u��
};

//===========================================================================
//
//	�W���C�X�e�B�b�N(�}�W�J���p�b�h)
//
//===========================================================================
class JoyMagical : public JoyDevice
{
public:
	JoyMagical(PPI *parent, int no);
										// �R���X�g���N�^

protected:
	DWORD FASTCALL ReadOnly(DWORD ctl) const;
										// �|�[�g�ǂݎ��(Read Only)
	void FASTCALL MakeData();
										// �f�[�^�쐬

private:
	static const char* AxisDescTable[];
										// ���\���e�[�u��
	static const char* ButtonDescTable[];
										// �{�^���\���e�[�u��
};

//===========================================================================
//
//	�W���C�X�e�B�b�N(XPD-1LR)
//
//===========================================================================
class JoyLR : public JoyDevice
{
public:
	JoyLR(PPI *parent, int no);
										// �R���X�g���N�^

protected:
	DWORD FASTCALL ReadOnly(DWORD ctl) const;
										// �|�[�g�ǂݎ��(Read Only)
	void FASTCALL MakeData();
										// �f�[�^�쐬

private:
	static const char* AxisDescTable[];
										// ���\���e�[�u��
	static const char* ButtonDescTable[];
										// �{�^���\���e�[�u��
};

//===========================================================================
//
//	�W���C�X�e�B�b�N(�p�b�N�����h��p�p�b�h)
//
//===========================================================================
class JoyPacl : public JoyDevice
{
public:
	JoyPacl(PPI *parent, int no);
										// �R���X�g���N�^

protected:
	DWORD FASTCALL ReadOnly(DWORD ctl) const;
										// �|�[�g�ǂݎ��(Read Only)
	void FASTCALL MakeData();
										// �f�[�^�쐬

private:
	static const char* ButtonDescTable[];
										// �{�^���\���e�[�u��
};

//===========================================================================
//
//	�W���C�X�e�B�b�N(BM68��p�R���g���[��)
//
//===========================================================================
class JoyBM : public JoyDevice
{
public:
	JoyBM(PPI *parent, int no);
										// �R���X�g���N�^

protected:
	DWORD FASTCALL ReadOnly(DWORD ctl) const;
										// �|�[�g�ǂݎ��(Read Only)
	void FASTCALL MakeData();
										// �f�[�^�쐬

private:
	static const char* ButtonDescTable[];
										// �{�^���\���e�[�u��
};

#endif	// ppi_h
