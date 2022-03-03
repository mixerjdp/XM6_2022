//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 �o�h�D(ytanaka@ipc-tokai.or.jp)
//	[ MFC MIDI ]
//
//---------------------------------------------------------------------------

#if defined(_WIN32)

#if !defined(mfc_midi_h)
#define mfc_midi_h

#include "mfc_com.h"
#include "mfc_que.h"

//===========================================================================
//
//	MIDI
//
//===========================================================================
class CMIDI : public CComponent
{
public:
	// ��񑗐M�^��`
	typedef struct _MIDIINFO {
		DWORD dwDevices;				// �f�o�C�X��
		DWORD dwDevice;					// �f�o�C�X�ԍ�
		DWORD dwHandle;					// �n���h��(HMIDI*)
		CWinThread *pThread;			// �X���b�h
		DWORD dwShort;					// �V���[�g���b�Z�[�W�J�E���^
		DWORD dwLong;					// �G�N�X�N���[�V�u�J�E���^
		DWORD dwUnprepare;				// �w�b�_����J�E���^
		DWORD dwBufNum;					// �o�b�t�@�L����
		DWORD dwBufRead;				// �o�b�t�@�ǂݎ��ʒu
		DWORD dwBufWrite;				// �o�b�t�@�������݈ʒu
	} MIDIINFO, *LPMIDIINFO;

public:
	// ��{�t�@���N�V����
	CMIDI(CFrmWnd *pWnd);
										// �R���X�g���N�^
	BOOL FASTCALL Init();
										// ������
	void FASTCALL Cleanup();
										// �N���[���A�b�v
	void FASTCALL ApplyCfg(const Config *pConfig);
										// �ݒ�K�p
#if !defined(NDEBUG)
	void AssertValid() const;
										// �f�f
#endif	// NDEBUG

	// �R�[���o�b�N����
	HMIDIOUT GetOutHandle() const		{ return m_hOut; }
										// Out�f�o�C�X�n���h���擾
	HMIDIIN GetInHandle() const			{ return m_hIn; }
										// In�f�o�C�X�n���h���擾

	// �f�o�C�X
	DWORD FASTCALL GetOutDevs() const;
										// Out�f�o�C�X���擾
	BOOL FASTCALL GetOutDevDesc(int nDevice, CString& strDesc) const;
										// Out�f�o�C�X���̎擾
	DWORD FASTCALL GetInDevs() const;
										// IN�f�o�C�X���擾
	BOOL FASTCALL GetInDevDesc(int nDevice, CString& strDesc) const;
										// IN�f�o�C�X���̎擾

	// �f�B���C
	void FASTCALL SetOutDelay(int nDelay);
										// Out�f�B���C�ݒ�
	void FASTCALL SetInDelay(int nDelay);
										// In�f�B���C�ݒ�

	// �~�L�T
	int FASTCALL GetOutVolume();
										// Out���ʎ擾
	void FASTCALL SetOutVolume(int nVolume);
										// Out���ʐݒ�

	// ���擾
	void FASTCALL GetOutInfo(LPMIDIINFO pInfo) const;
										// Out���擾
	void FASTCALL GetInInfo(LPMIDIINFO pInfo) const;
										// In���擾

private:
	MIDI *m_pMIDI;
										// MIDI
	Scheduler *m_pScheduler;
										// �X�P�W���[��
	CScheduler *m_pSch;
										// �X�P�W���[��

	// Out
	enum OutState {
		OutReady,						// �f�[�^�҂���ԂȂ�
		OutEx,							// �G�N�X�N���[�V�u���b�Z�[�W�o�͒�
		OutShort,						// �V���[�g���b�Z�[�W�o�͒�
	};
	void FASTCALL OpenOut(DWORD dwDevice);
										// Out�I�[�v��
	void FASTCALL CloseOut();
										// Out�N���[�Y
	void FASTCALL RunOut();
										// Out�X���b�h
	void FASTCALL CallbackOut(UINT wMsg, DWORD dwParam1, DWORD dwParam2);
										// Out�R�[���o�b�N
	void FASTCALL GetOutCaps();
										// Out�f�o�C�XCaps�擾
	BOOL FASTCALL SendEx(const BYTE *pExData);
										// �G�N�X�N���[�V�u���M
	BOOL FASTCALL SendExWait();
										// �G�N�X�N���[�V�u�����҂�
	void FASTCALL SendAllNoteOff();
										// �S�m�[�g�I�t
	BOOL FASTCALL SendReset();
										// ���Z�b�g���M
	static UINT OutThread(LPVOID pParam);
										// Out�X���b�h
#if _MFC_VER >= 0x700
	static void CALLBACK OutProc(HMIDIOUT hOut, UINT wMsg, DWORD_PTR dwInstance,
		DWORD dwParam1, DWORD dwParam2);// Out�R�[���o�b�N
#else
	static void CALLBACK OutProc(HMIDIOUT hOut, UINT wMsg, DWORD dwInstance,
		DWORD dwParam1, DWORD dwParam2);// Out�R�[���o�b�N
#endif
	DWORD m_dwOutDevice;
										// Out�f�o�C�XiD
	HMIDIOUT m_hOut;
										// Out�f�o�C�X�n���h��
	CWinThread *m_pOutThread;
										// Out�X���b�h
	BOOL m_bOutThread;
										// Out�X���b�h�I���t���O
	DWORD m_dwOutDevs;
										// Out�f�o�C�X��
	LPMIDIOUTCAPS m_pOutCaps;
										// Out�f�o�C�XCaps
	DWORD m_dwOutDelay;
										// Out�f�B���C(ms)
	BOOL m_bSendEx;
										// �G�N�X�N���[�V�u���M�t���O
	BOOL m_bSendExHdr;
										// �G�N�X�N���[�V�u�w�b�_�g�p�t���O
	BYTE m_ExBuf[0x2000];
										// �G�N�X�N���[�V�u�o�b�t�@
	MIDIHDR m_ExHdr;
										// �G�N�X�N���[�V�u�w�b�_
	int m_nOutReset;
										// ���Z�b�g�R�}���h���
	DWORD m_dwShortSend;
										// �V���[�g�R�}���h���M�J�E���^
	DWORD m_dwExSend;
										// �G�N�X�N���[�V�u���M���s�J�E���^
	DWORD m_dwUnSend;
										// �w�b�_����J�E���^
	CCriticalSection m_OutSection;
										// �N���e�B�J���Z�N�V����
	static const BYTE ResetGM[];
										// GM���Z�b�g�R�}���h
	static const BYTE ResetGS[];
										// GS���Z�b�g�R�}���h
	static const BYTE ResetXG[];
										// XG���Z�b�g�R�}���h
	static const BYTE ResetLA[];
										// LA���Z�b�g�R�}���h

	// In
	enum InState {
		InNotUsed,						// ���g�p
		InReady,						// �f�[�^�҂�
		InDone							// �f�[�^����
	};
	enum {
		InBufMax = 0x800				// �o�b�t�@�T�C�Y
	};
	void FASTCALL OpenIn(DWORD dwDevice);
										// In�I�[�v��
	void FASTCALL CloseIn();
										// In�N���[�Y
	void FASTCALL RunIn();
										// In�X���b�h
	void FASTCALL CallbackIn(UINT wMsg, DWORD dwParam1, DWORD dwParam2);
										// In�R�[���o�b�N
	void FASTCALL GetInCaps();
										// In�f�o�C�XCaps�擾
	BOOL FASTCALL StartIn();
										// In�J�n
	void FASTCALL StopIn();
										// In��~
	void FASTCALL ShortIn();
										// In�V���[�g���b�Z�[�W
	void FASTCALL LongIn(int nHdr);
										// In�����O���b�Z�[�W
	static UINT InThread(LPVOID pParam);
										// In�X���b�h
#if _MFC_VER >= 0x700
	static void CALLBACK InProc(HMIDIIN hIn, UINT wMsg, DWORD_PTR dwInstance,
		DWORD dwParam1, DWORD dwParam2);// In�R�[���o�b�N
#else
	static void CALLBACK InProc(HMIDIIN hIn, UINT wMsg, DWORD dwInstance,
		DWORD dwParam1, DWORD dwParam2);// In�R�[���o�b�N
#endif
	DWORD m_dwInDevice;
										// In�f�o�C�XiD
	HMIDIIN m_hIn;
										// In�f�o�C�X�n���h��
	CWinThread *m_pInThread;
										// InT�X���b�h
	BOOL m_bInThread;
										// In�X���b�h�I���t���O
	DWORD m_dwInDevs;
										// In�f�o�C�X��
	LPMIDIINCAPS m_pInCaps;
										// In�f�o�C�XCaps
	MIDIHDR m_InHdr[2];
										// �G�N�X�N���[�V�u�w�b�_
	BYTE m_InBuf[2][InBufMax];
										// �G�N�X�N���[�V�u�o�b�t�@
	InState m_InState[2];
										// �G�N�X�N���[�V�u�o�b�t�@���
	CQueue m_InQueue;
										// �V���[�g���b�Z�[�W���̓L���[
	DWORD m_dwShortRecv;
										// �V���[�g�R�}���h��M�J�E���^
	DWORD m_dwExRecv;
										// �G�N�X�N���[�V�u��M�J�E���^
	DWORD m_dwUnRecv;
										// �w�b�_����J�E���^
	CCriticalSection m_InSection;
										// �N���e�B�J���Z�N�V����
};

#endif	// mfc_midi_h
#endif	// _WIN32
