//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2005 �o�h�D(ytanaka@ipc-tokai.or.jp)
//	[ �f�o�C�X���� ]
//
//---------------------------------------------------------------------------

#if !defined(device_h)
#define device_h

//===========================================================================
//
//	�f�o�C�X
//
//===========================================================================
class Device
{
public:
	// �����f�[�^��`
	typedef struct {
		DWORD id;						// ID
		const char *desc;				// ����
		Device* next;					// ���̃f�o�C�X
	} device_t;

public:
	// ��{�t�@���N�V����
	Device(VM *p);
										// �R���X�g���N�^
	virtual ~Device();
										// �f�X�g���N�^
	virtual BOOL FASTCALL Init();
										// ������
	virtual void FASTCALL Cleanup();
										// �N���[���A�b�v
	virtual void FASTCALL Reset();
										// ���Z�b�g
	virtual BOOL FASTCALL Save(Fileio *fio, int ver);
										// �Z�[�u
	virtual BOOL FASTCALL Load(Fileio *fio, int ver);
										// ���[�h
	virtual void FASTCALL ApplyCfg(const Config *config);
										// �ݒ�K�p
#if !defined(NDEBUG)
	void FASTCALL AssertDiag() const;
										// �f�f
#endif	// NDEBUG

	// �O��API
	Device* FASTCALL GetNextDevice() const { return dev.next; }
										// ���̃f�o�C�X���擾
	void FASTCALL SetNextDevice(Device *p) { dev.next = p; }
										// ���̃f�o�C�X��ݒ�
	DWORD FASTCALL GetID() const		{ return dev.id; }
										// �f�o�C�XID�擾
	const char* FASTCALL GetDesc() const { return dev.desc; }
										// �f�o�C�X���̎擾
	VM* FASTCALL GetVM() const			{ return vm; }
										// VM�擾
	virtual BOOL FASTCALL Callback(Event *ev);
										// �C�x���g�R�[���o�b�N

protected:
	Log* FASTCALL GetLog() const		{ return log; }
										// ���O�擾
	device_t dev;
										// �����f�[�^
	VM *vm;
										// ���z�}�V���{��
	Log *log;
										// ���O
};

//===========================================================================
//
//	�������}�b�v�h�f�o�C�X
//
//===========================================================================
class MemDevice : public Device
{
public:
	// �����f�[�^��`
	typedef struct {
		DWORD first;					// �J�n�A�h���X
		DWORD last;						// �ŏI�A�h���X
	} memdev_t;

public:
	// ��{�t�@���N�V����
	MemDevice(VM *p);
										// �R���X�g���N�^
	virtual BOOL FASTCALL Init();
										// ������

	// �������f�o�C�X
	virtual DWORD FASTCALL ReadByte(DWORD addr);
										// �o�C�g�ǂݍ���
	virtual DWORD FASTCALL ReadWord(DWORD addr);
										// ���[�h�ǂݍ���
	virtual void FASTCALL WriteByte(DWORD addr, DWORD data);
										// �o�C�g��������
	virtual void FASTCALL WriteWord(DWORD addr, DWORD data);
										// ���[�h��������
	virtual DWORD FASTCALL ReadOnly(DWORD addr) const;
										// �ǂݍ��݂̂�
#if !defined(NDEBUG)
	void FASTCALL AssertDiag() const;
										// �f�f
#endif	// NDEBUG

	// �O��API
	DWORD FASTCALL GetFirstAddr() const	{ return memdev.first; }
										// �ŏ��̃A�h���X���擾
	DWORD FASTCALL GetLastAddr() const	{ return memdev.last; }
										// �Ō�̃A�h���X���擾

protected:
	memdev_t memdev;
										// �����f�[�^
	CPU *cpu;
										// CPU
	Scheduler *scheduler;
										// �X�P�W���[��
};

#endif	// device_h
