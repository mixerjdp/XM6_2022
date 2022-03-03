//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 �o�h�D(ytanaka@ipc-tokai.or.jp)
//	[ �C�x���g ]
//
//---------------------------------------------------------------------------

#if !defined(event_h)
#define event_h

//===========================================================================
//
//	�C�x���g
//
//===========================================================================
class Event
{
public:
	// �����f�[�^��`
#if defined(_WIN32)
#pragma pack(push, 8)
#endif	// _WIN32
	typedef struct {
		DWORD remain;					// �c�莞��
		DWORD time;						// �g�[�^������
		DWORD user;						// ���[�U��`�f�[�^
		Device *device;					// �e�f�o�C�X
		Scheduler *scheduler;			// �X�P�W���[��
		Event *next;					// ���̃C�x���g
		char desc[0x20];				// ����
	} event_t;
#if defined(_WIN32)
#pragma pack(pop)
#endif	// _WIN32

public:
	// ��{�t�@���N�V����
	Event();
										// �R���X�g���N�^
	virtual ~Event();
										// �f�X�g���N�^
#if !defined(NDEBUG)
	void FASTCALL AssertDiag() const;
										// �f�f
#endif	// NDEBUG

	// ���[�h�E�Z�[�u
	BOOL FASTCALL Save(Fileio *fio, int ver);
										// �Z�[�u
	BOOL FASTCALL Load(Fileio *fio, int ver);
										// ���[�h

	// �v���p�e�B
	void FASTCALL SetDevice(Device *p);
										// �e�f�o�C�X�ݒ�
	Device* FASTCALL GetDevice() const	{ return ev.device; }
										// �e�f�o�C�X�擾
	void FASTCALL SetDesc(const char *desc);
										// ���̐ݒ�
	const char* FASTCALL GetDesc() const;
										// ���̎擾
	void FASTCALL SetUser(DWORD data)	{ ev.user = data; }
										// ���[�U��`�f�[�^�ݒ�
	DWORD FASTCALL GetUser() const		{ return ev.user; }
										// ���[�U��`�f�[�^�擾

	// ���ԊǗ�
	void FASTCALL SetTime(DWORD hus);
										// ���Ԏ����ݒ�
	DWORD FASTCALL GetTime() const		{ return ev.time; }
										// ���Ԏ����擾
	DWORD FASTCALL GetRemain() const	{ return ev.remain; }
										// �c�莞�Ԏ擾
	void FASTCALL Exec(DWORD hus);
										// ���Ԃ�i�߂�

	// �����N�ݒ�E�폜
	void FASTCALL SetNextEvent(Event *p) { ev.next = p; }
										// ���̃C�x���g��ݒ�
	Event* FASTCALL GetNextEvent() const { return ev.next; }
										// ���̃C�x���g���擾

private:
	// �����f�[�^��`(Ver2.01�܂ŁBenable������)
	typedef struct {
		Device *device;					// �e�f�o�C�X
		Scheduler *scheduler;			// �X�P�W���[��
		Event *next;					// ���̃C�x���g
		char desc[0x20];				// ����
		DWORD user;						// ���[�U��`�f�[�^
		BOOL enable;					// �C�l�[�u������
		DWORD time;						// �g�[�^������
		DWORD remain;					// �c�莞��
	} event201_t;

	BOOL FASTCALL Load201(Fileio *fio);
										// ���[�h(version 2.01�ȑO)
	event_t ev;
										// �������[�N
};

#endif	// event_h
