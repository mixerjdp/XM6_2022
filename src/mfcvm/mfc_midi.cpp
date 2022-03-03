//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 �o�h�D(ytanaka@ipc-tokai.or.jp)
//	[ MFC MIDI ]
//
//---------------------------------------------------------------------------

#if defined(_WIN32)

#include "os.h"
#include "xm6.h"
#include "vm.h"
#include "midi.h"
#include "config.h"
#include "mfc_com.h"
#include "mfc_sch.h"
#include "mfc_cfg.h"
#include "mfc_midi.h"

//===========================================================================
//
//	MIDI
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	MIDI���b�Z�[�W��`
//
//---------------------------------------------------------------------------
#define MIDI_NOTEOFF		0x80		// �m�[�g�I�t
#define MIDI_NOTEON			0x90		// �m�[�g�I��(�ꕔ�m�[�g�I�t)
#define MIDI_PKEYPRS		0xa0		// �|���t�H�j�b�N�L�[�v���b�V���[
#define MIDI_CTRLCHG		0xb0		// �R���g���[���`�F���W
#define MIDI_PROGCHG		0xc0		// �v���O�����`�F���W
#define MIDI_CHPRS			0xd0		// �`���l���v���b�V���[
#define MIDI_PITCHCHG		0xe0		// �s�b�`�x���h�`�F���W
#define MIDI_EXSTART		0xf0		// �G�N�X�N���[�V�u�J�n
#define MIDI_QFRAME			0xf1		// �^�C���R�[�h�N�H�[�^�[�t���[��
#define MIDI_SONGPTR		0xf2		// �\���O�|�W�V�����|�C���^
#define MIDI_SONGSEL		0xf3		// �\���O�Z���N�g
#define MIDI_TUNEREQ		0xf6		// �`���[���Z���N�g
#define MIDI_EXEND			0xf7		// �G�N�X�N���[�V�u�I��
#define MIDI_TIMINGCLK		0xf8		// �^�C�~���O�N���b�N
#define MIDI_START			0xfa		// �X�^�[�g
#define MIDI_CONTINUE		0xfb		// �R���e�B�j���[
#define MIDI_STOP			0xfc		// �X�g�b�v
#define MIDI_ACTSENSE		0xfe		// �A�N�e�B�u�Z���V���O
#define MIDI_SYSRESET		0xff		// �V�X�e�����Z�b�g

//---------------------------------------------------------------------------
//
//	�R���X�g���N�^
//
//---------------------------------------------------------------------------
CMIDI::CMIDI(CFrmWnd *pWnd) : CComponent(pWnd)
{
	// �R���|�[�l���g�p�����[�^
	m_dwID = MAKEID('M', 'I', 'D', 'I');
	m_strDesc = _T("MIDI Driver");

	// �I�u�W�F�N�g
	m_pMIDI = NULL;
	m_pScheduler = NULL;
	m_pSch = NULL;

	// Out
	m_dwOutDevice = 0;
	m_hOut = NULL;
	m_pOutThread = NULL;
	m_bOutThread = FALSE;
	m_dwOutDevs = 0;
	m_pOutCaps = NULL;
	m_dwOutDelay = (84 * 2000);
	m_nOutReset = 0;
	m_dwShortSend = 0;
	m_dwExSend = 0;
	m_dwUnSend = 0;

	// In
	m_dwInDevice = 0;
	m_hIn = NULL;
	m_pInThread = NULL;
	m_bInThread = FALSE;
	m_dwInDevs = 0;
	m_pInCaps = NULL;
	m_InState[0] = InNotUsed;
	m_InState[1] = InNotUsed;
	m_dwShortRecv = 0;
	m_dwExRecv = 0;
	m_dwUnRecv = 0;
}

//---------------------------------------------------------------------------
//
//	������
//
//---------------------------------------------------------------------------
BOOL FASTCALL CMIDI::Init()
{
	ASSERT(this);

	// ��{�N���X
	if (!CComponent::Init()) {
		return FALSE;
	}

	// MIDI�擾
	ASSERT(!m_pMIDI);
	m_pMIDI = (MIDI*)::GetVM()->SearchDevice(MAKEID('M', 'I', 'D', 'I'));
	ASSERT(m_pMIDI);

	// �X�P�W���[���擾
	ASSERT(!m_pScheduler);
	m_pScheduler = (Scheduler*)::GetVM()->SearchDevice(MAKEID('S', 'C', 'H', 'E'));
	ASSERT(m_pScheduler);

	// �X�P�W���[��(Win32)�擾
	ASSERT(!m_pSch);
	m_pSch = (CScheduler*)SearchComponent(MAKEID('S', 'C', 'H', 'E'));
	ASSERT(m_pSch);

	// Out�f�o�C�X���
	m_dwOutDevs = ::midiOutGetNumDevs();
	if (m_dwOutDevs > 0) {
		
		try {
			m_pOutCaps = new MIDIOUTCAPS[m_dwOutDevs];
		}
		catch (...) {
			return FALSE;
		}
		if (!m_pOutCaps) {
			return FALSE;
		}

		// CAPS���擾
		GetOutCaps();
	}

	// In�f�o�C�X���
	m_dwInDevs = ::midiInGetNumDevs();
	if (m_dwInDevs > 0) {
		
		try {
			m_pInCaps = new MIDIINCAPS[m_dwInDevs];
		}
		catch (...) {
			return FALSE;
		}
		if (!m_pInCaps) {
			return FALSE;
		}

		// CAPS���擾
		GetInCaps();
	}

	// In�L���[������
	if (!m_InQueue.Init(InBufMax)) {
		return FALSE;
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	�N���[���A�b�v
//
//---------------------------------------------------------------------------
void FASTCALL CMIDI::Cleanup()
{
	ASSERT(this);

	// �N���[�Y
	CloseOut();
	CloseIn();

	// Caps�폜
	if (m_pOutCaps) {
		delete[] m_pOutCaps;
		m_pOutCaps = NULL;
	}
	if (m_pInCaps) {
		delete[] m_pInCaps;
		m_pInCaps = NULL;
	}

	// ��{�N���X
	CComponent::Cleanup();
}

//---------------------------------------------------------------------------
//
//	�ݒ�K�p
//
//---------------------------------------------------------------------------
void FASTCALL CMIDI::ApplyCfg(const Config* pConfig)
{
	CConfig *pComponent;

	ASSERT(this);
	ASSERT(pConfig);
	ASSERT_VALID(this);

	// �R���t�B�O�R���|�[�l���g���擾
	pComponent = (CConfig*)SearchComponent(MAKEID('C', 'F', 'G', ' '));
	ASSERT(pComponent);

	// ���Z�b�g�R�}���h
	m_nOutReset = pConfig->midi_reset;

	// Out�f�B���C
	m_dwOutDelay = (DWORD)(pConfig->midiout_delay * 2000);

	// Out�f�o�C�X
	if (pConfig->midiout_device != (int)m_dwOutDevice) {
		// ��U����
		CloseOut();

		// �w��f�o�C�X�Ńg���C
		if (pConfig->midiout_device > 0) {
			OpenOut(pConfig->midiout_device);
		}

		// ��������΁A�R���t�B�O�ɔ��f������
		if (m_dwOutDevice != 0) {
			pComponent->SetMIDIDevice((int)m_dwOutDevice, FALSE);
		}
	}

	// In�f�B���C
	SetInDelay(pConfig->midiin_delay * 2000);

	// In�f�o�C�X
	if (pConfig->midiin_device != (int)m_dwInDevice) {
		// ��U����
		CloseIn();

		// �w��f�o�C�X�Ńg���C
		if (pConfig->midiin_device > 0) {
			OpenIn(pConfig->midiin_device);
		}

		// ��������΁A�R���t�B�O�ɔ��f������
		if (m_dwInDevice > 0) {
			pComponent->SetMIDIDevice((int)m_dwInDevice, TRUE);
		}
	}
}

#if !defined(NDEBUG)
//---------------------------------------------------------------------------
//
//	�f�f
//
//---------------------------------------------------------------------------
void CMIDI::AssertValid() const
{
	// ��{�N���X
	CComponent::AssertValid();

	ASSERT(this);
	ASSERT(m_pMIDI);
	ASSERT(m_pMIDI->GetID() == MAKEID('M', 'I', 'D', 'I'));
	ASSERT(m_pScheduler);
	ASSERT(m_pScheduler->GetID() == MAKEID('S', 'C', 'H', 'E'));
	ASSERT(m_pSch);
	ASSERT(m_pSch->GetID() == MAKEID('S', 'C', 'H', 'E'));
	ASSERT((m_dwOutDevs == 0) || m_pOutCaps);
	ASSERT((m_dwInDevs == 0) || m_pInCaps);
	ASSERT((m_nOutReset >= 0) && (m_nOutReset <= 3));
}
#endif	// NDEBUG

//===========================================================================
//
//	OUT����
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	Out�I�[�v��
//	��dwDevice=0�͊��蓖�ĂȂ��AdwDevice=1��MIDI�}�b�p�[
//
//---------------------------------------------------------------------------
void FASTCALL CMIDI::OpenOut(DWORD dwDevice)
{
	MMRESULT mmResult;
	UINT uDeviceID;

	ASSERT(this);
	ASSERT(m_dwOutDevice == 0);
	ASSERT(!m_pOutThread);
	ASSERT_VALID(this);

	// ���蓖�Ă��Ȃ��̂ł���΃��^�[��
	if (dwDevice == 0) {
		return;
	}

	// �f�o�C�X���Ȃ���΃��^�[��
	if (m_dwOutDevs == 0) {
		return;
	}

	// �f�o�C�X���𒴂��Ă����MIDI�}�b�p�[
	if (dwDevice > (m_dwOutDevs + 1)) {
		dwDevice = 1;
	}

	// �f�o�C�X����
	if (dwDevice == 1) {
		uDeviceID = MIDI_MAPPER;
	}
	else {
		uDeviceID = (UINT)(dwDevice - 2);
	}

	// �I�[�v��
#if _MFC_VER >= 0x700
	mmResult = ::midiOutOpen(&m_hOut, uDeviceID,
							(DWORD_PTR)OutProc, (DWORD_PTR)this, CALLBACK_FUNCTION);
#else
	mmResult = ::midiOutOpen(&m_hOut, uDeviceID,
							(DWORD)OutProc, (DWORD)this, CALLBACK_FUNCTION);
#endif

	// ���s�Ȃ�I��
	if (mmResult != MMSYSERR_NOERROR) {
		return;
	}

	// �f�o�C�X�ԍ����X�V
	m_dwOutDevice = dwDevice;

	// �]���o�b�t�@���N���A
	m_pMIDI->ClrTransData();

	// �X���b�h�𗧂Ă�
	m_bOutThread = FALSE;
	m_pOutThread = AfxBeginThread(OutThread, this);
}

//---------------------------------------------------------------------------
//
//	Out�N���[�Y
//
//---------------------------------------------------------------------------
void FASTCALL CMIDI::CloseOut()
{
	ASSERT(this);
	ASSERT_VALID(this);

	// �X���b�h���~�߂�
	if (m_pOutThread) {
		// �I���t���O���グ
		m_bOutThread = TRUE;

		// �I���҂�(������)
		::WaitForSingleObject(m_pOutThread->m_hThread, INFINITE);

		// �X���b�h�Ȃ�
		m_pOutThread = NULL;
	}

	// �f�o�C�X�����
	if (m_dwOutDevice != 0) {
		// �S�Ẵm�[�g���L�[�I�t���āA�N���[�Y
		SendAllNoteOff();
		::midiOutReset(m_hOut);
		::midiOutClose(m_hOut);

		// �I�[�v���f�o�C�X�Ȃ�
		m_dwOutDevice = 0;
	}
}

//---------------------------------------------------------------------------
//
//	Out�X���b�h
//
//---------------------------------------------------------------------------
void FASTCALL CMIDI::RunOut()
{
	BOOL bEnable;
	BOOL bActive;
	OutState outState;
	DWORD dwTime;
	const MIDI::mididata_t *pData;
	DWORD dwLen;
	DWORD dwCnt;
	DWORD dwCmd;
	DWORD dwShortMsg;

	ASSERT(this);
	ASSERT(m_hOut);
	ASSERT_VALID(this);

	// �X�P�W���[���̏�Ԃ��L��
	bEnable = m_pSch->IsEnable();

	// �G�N�X�N���[�V�u���M�Ȃ��A�w�b�_�g�p�Ȃ�
	m_bSendEx = FALSE;
	m_bSendExHdr = FALSE;

	// �J�E���^������
	m_dwShortSend = 0;
	m_dwExSend = 0;
	m_dwUnSend = 0;

	// �f�[�^�Ȃ��A�R�}���h�������A�X�e�[�g���f�B
	pData = NULL;
	dwCmd = 0;
	dwLen = 0;
	dwCnt = 0;
	dwShortMsg = 0;
	outState = OutReady;

	// ���Z�b�g
	SendReset();

	// �A�N�e�B�u(�L���ȃf�[�^��MIDI OUT�֑�������)
	bActive = FALSE;

	// ���[�v
	while (!m_bOutThread) {
		// �G�N�X�N���[�V�u���M���Ȃ�҂�
		if (m_bSendEx) {
			::Sleep(1);
			continue;
		}

		// �G�N�X�N���[�V�u���M�w�b�_���g������Ȃ�A��Еt��
		if (m_bSendExHdr) {
			// ���b�N���čs��
			m_OutSection.Lock();
			::midiOutUnprepareHeader(m_hOut, &m_ExHdr, sizeof(MIDIHDR));
			m_OutSection.Unlock();

			// �G�N�X�N���[�V�u�w�b�_�g�p�t���O�����낷
			m_bSendExHdr = FALSE;
			pData = NULL;
			outState = OutReady;

			// �J�E���g�A�b�v
			m_dwUnSend++;
		}

		// �����Ȃ�
		if (!m_bEnable) {
			// �A�N�e�B�u�Ȃ�A�S�m�[�g�I�t�ƃ��Z�b�g
			if (bActive) {
				SendAllNoteOff();
				SendReset();
				bActive = FALSE;

				// �L���f�[�^�Ȃ�
				outState = OutReady;
				pData = NULL;
			}

			::Sleep(10);
			continue;
		}

		// �X�P�W���[���������Ȃ�
		if (!m_pSch->IsEnable()) {
			// ����܂ŗL���ł������Ȃ�
			if (bEnable) {
				// �S�m�[�g�[�I�t
				SendAllNoteOff();

				// ����ȊO�̏�Ԃ̓L�[�v
				bEnable = FALSE;
			}

			// ����܂ł�����
			::Sleep(10);
			continue;
		}

		// �X�P�W���[���͗L��
		bEnable = TRUE;

		// MIDI�����Z�b�g���ꂽ��
		if (m_pMIDI->IsReset()) {
			// �t���O���Ƃ���
			m_pMIDI->ClrReset();

			// �S�m�[�g�I�t�ƃ��Z�b�g
			SendAllNoteOff();
			SendReset();
			pData = NULL;
			outState = OutReady;
		}

		// MIDI���A�N�e�B�u�łȂ���Ή������Ȃ�
		if (!m_pMIDI->IsActive()) {
			::Sleep(10);
			continue;
		}

		// MIDI���M�o�b�t�@�̌����擾�B0�Ȃ�Sleep
		if (!pData) {
			if (m_pMIDI->GetTransNum() == 0) {
				// 15ms�ȏ�f�B���C������̂ŁA10ms�E�F�C�g
				if (m_dwOutDelay > 30000) {
					::Sleep(10);
					continue;
				}
				// �f�B���C��14ms�ȉ��B1ms�E�F�C�g
				::Sleep(1);
				continue;
			}

			// ���M�f�[�^�̐擪���擾
			pData = m_pMIDI->GetTransData(0);
		}

		// 1�o�C�g�ȏ�f�[�^������̂ŁA�A�N�e�B�u�Ƃ���
		bActive = TRUE;

		// ���f�B�ŁA�G�N�X�N���[�V�u�R�}���h�łȂ���΁A���ԑ҂�
		if ((outState == OutReady) && (pData->data != MIDI_EXSTART)) {
			dwTime = m_pScheduler->GetTotalTime() - pData->vtime;
			if (dwTime < m_dwOutDelay) {
				if (dwTime > 30000) {
					// 15ms�ȏ�󂢂Ă���΁A1.5ms�҂�
					::Sleep(10);
					continue;
				}
				if (dwTime > 3000) {
					// 1.5ms�ȏ�󂢂Ă���΁A1.5ms�҂�
					::Sleep(1);
					continue;
				}
				// 1.5ms�����̑҂���Sleep(0)�ŋz��
				::Sleep(0);
				continue;
			}
		}

		// �f�[�^����(����)
		if (outState == OutReady) {
			// �G�N�X�N���[�V�u���M���Ȃ�A�����ŏo��(�ł̌���)
			if ((pData->data & 0x80) && (pData->data != MIDI_EXEND)) {
				if (outState == OutEx) {
					m_ExBuf[dwCnt] = MIDI_EXEND;
					dwCnt++;
					SendEx(m_ExBuf);
					continue;
				}
			}

			// ����f�[�^
			if ((pData->data >= MIDI_NOTEOFF) && (pData->data < MIDI_EXSTART)) {
				// 80-EF
				outState = OutShort;
				dwCmd = pData->data;
				dwCnt = 1;
				dwShortMsg = dwCmd;
				dwLen = 3;

				if ((pData->data >= MIDI_PROGCHG) && (pData->data < MIDI_PITCHCHG)) {
					// C0-DF
					dwLen = 2;
				}
			}

			// �R�������b�Z�[�W�A���A���^�C�����b�Z�[�W
			switch (pData->data) {
				// �G�N�X�N���[�V�u�J�n
				case MIDI_EXSTART:
					outState = OutEx;
					m_ExBuf[0] = (BYTE)pData->data;
					dwCnt = 1;
					break;
				// �^�C���R�[�h�N�H�[�^�[�t���[��
				case MIDI_QFRAME:
				// �\���O�Z���N�g
				case MIDI_SONGSEL:
					outState = OutShort;
					dwCnt = 1;
					dwShortMsg = pData->data;
					dwLen = 2;
					break;
				// �\���O�|�W�V�����|�C���^
				case MIDI_SONGPTR:
					outState = OutShort;
					dwCnt = 1;
					dwShortMsg = pData->data;
					dwLen = 3;
					break;
				// �`���[�����N�G�X�g�A���A���^�C�����b�Z�[�W
				case MIDI_TUNEREQ:
				case MIDI_TIMINGCLK:
				case MIDI_START:
				case MIDI_CONTINUE:
				case MIDI_STOP:
				case MIDI_ACTSENSE:
				case MIDI_SYSRESET:
					// ���A���^�C�����b�Z�[�W�̗ނ͏o�͂��Ȃ�(�喂�E��)
					m_pMIDI->DelTransData(1);
					pData = NULL;
					continue;
			}

			// �����j���O�X�e�[�^�X
			if (pData->data < MIDI_NOTEOFF) {
				dwCnt = 2;
				dwShortMsg = (pData->data << 8) | dwCmd;
				outState = OutShort;
			}

#if defined(_DEBUG)
			// �f�[�^�`�F�b�N(DEBUG�̂�)
			if (outState == OutReady) {
				TRACE("MIDI�ُ�f�[�^���o $%02X\n", pData->data);
			}
#endif	// _DEBUG

			// ���̃f�[�^���폜
			m_pMIDI->DelTransData(1);
			pData = NULL;

			// �����ő��M�ł�����̂Ɍ���A�o��
			if ((outState == OutShort) && (dwCnt == dwLen)) {
				m_OutSection.Lock();
				::midiOutShortMsg(m_hOut, dwShortMsg);
				m_OutSection.Unlock();

				// �J�E���g�A�b�v
				m_dwShortSend++;
				outState = OutReady;
			}
			continue;
		}

		// �f�[�^����(2�o�C�g�ڈȍ~)
		if (pData->data & 0x80) {
			// �G�N�X�N���[�V�u���M���Ȃ�A�����ŏo��(�ł̌���)
			if (pData->data != MIDI_EXEND) {
				if (outState == OutEx) {
					m_ExBuf[dwCnt] = MIDI_EXEND;
					dwCnt++;
					SendEx(m_ExBuf);
					continue;
				}
			}

			if ((pData->data >= MIDI_NOTEOFF) && (pData->data < MIDI_EXSTART)) {
				// 80-EF
				outState = OutShort;
				dwCmd = pData->data;
				dwCnt = 1;
				dwShortMsg = dwCmd;
				dwLen = 3;

				if ((pData->data >= MIDI_PROGCHG) && (pData->data < MIDI_PITCHCHG)) {
					// C0-DF
					dwLen = 2;
				}
			}

			// �R�������b�Z�[�W�A���A���^�C�����b�Z�[�W
			switch (pData->data) {
				// �G�N�X�N���[�V�u�J�n
				case MIDI_EXSTART:
					outState = OutEx;
					m_ExBuf[0] = (BYTE)pData->data;
					dwCnt = 1;
					break;
				// �^�C���R�[�h�N�H�[�^�[�t���[��
				case MIDI_QFRAME:
				// �\���O�Z���N�g
				case MIDI_SONGSEL:
					outState = OutShort;
					dwCnt = 1;
					dwShortMsg = pData->data;
					dwLen = 2;
					break;
				// �\���O�|�W�V�����|�C���^
				case MIDI_SONGPTR:
					outState = OutShort;
					dwCnt = 1;
					dwShortMsg = pData->data;
					dwLen = 3;
					break;
				// �`���[�����N�G�X�g�A���A���^�C�����b�Z�[�W
				case MIDI_TUNEREQ:
				case MIDI_TIMINGCLK:
				case MIDI_START:
				case MIDI_CONTINUE:
				case MIDI_STOP:
				case MIDI_ACTSENSE:
				case MIDI_SYSRESET:
					// ���A���^�C�����b�Z�[�W�̗ނ͏o�͂��Ȃ�(�喂�E��)
					break;
				// �G�N�X�N���[�V�u�I��
				case MIDI_EXEND:
					if (outState == OutEx) {
						// �G�N�X�N���[�V�u���M���Ȃ�A�����ŏo��
						m_ExBuf[dwCnt] = (BYTE)pData->data;
						dwCnt++;
						SendEx(m_ExBuf);
						outState = OutReady;

						// ���̃f�[�^���폜
						m_pMIDI->DelTransData(1);
						pData = NULL;
						continue;
					}
			}
		}
		else {
			// 00-7F
			if (outState == OutEx) {
				// �G�N�X�N���[�V�u
				m_ExBuf[dwCnt] = (BYTE)pData->data;
				dwCnt++;
				if (dwCnt >= sizeof(m_ExBuf)) {
					outState = OutReady;
				}
			}
			if (outState == OutShort) {
				// �V���[�g���b�Z�[�W
				dwShortMsg |= (pData->data << (dwCnt << 3));
				dwCnt++;
			}
		}

		// ���̃f�[�^���폜
		m_pMIDI->DelTransData(1);
		pData = NULL;

		// �����ő��M�ł�����̂Ɍ���A�o��
		if ((outState == OutShort) && (dwCnt == dwLen)) {
			m_OutSection.Lock();
			::midiOutShortMsg(m_hOut, dwShortMsg);
			m_OutSection.Unlock();

			// �J�E���g�A�b�v
			m_dwShortSend++;
			outState = OutReady;
		}
	}

	// �G�N�X�N���[�V�u���M��̃E�F�C�g
	SendExWait();

	// �S�m�[�g�I�t
	SendAllNoteOff();
}

//---------------------------------------------------------------------------
//
//	Out�R�[���o�b�N
//
//---------------------------------------------------------------------------
void FASTCALL CMIDI::CallbackOut(UINT wMsg, DWORD /*dwParam1*/, DWORD /*dwParam2*/)
{
	ASSERT(this);
	ASSERT_VALID(this);

	// �G�N�X�N���[�V�u���M�����̂݃n���h�����O
	if (wMsg == MM_MOM_DONE) {

		// �G�N�X�N���[�V�u���M�t���OOFF(BOOL�f�[�^�̂��߁A�����͎��Ȃ�)
		m_bSendEx = FALSE;
	}
}

//---------------------------------------------------------------------------
//
//	Out�f�o�C�XCaps�擾
//
//---------------------------------------------------------------------------
void FASTCALL CMIDI::GetOutCaps()
{
	MMRESULT mmResult;
#if _MFC_VER >= 0x700
	UINT_PTR uDeviceID;
#else
	UINT uDeviceID;
#endif	// MFC_VER

	// �f�o�C�XID������
	uDeviceID = 0;

	// ���[�v
	while ((DWORD)uDeviceID < m_dwOutDevs) {
		// midiOutGetDevCaps�Ŏ擾
		mmResult = ::midiOutGetDevCaps( uDeviceID,
										&m_pOutCaps[uDeviceID],
										sizeof(MIDIOUTCAPS));

		// ���ʂ��L�^
		if (mmResult == MMSYSERR_NOERROR) {
			m_pOutCaps[uDeviceID].dwSupport = 0;
		}
		else {
			m_pOutCaps[uDeviceID].dwSupport = 1;
		}

		// �f�o�C�XID������
		uDeviceID++;
	}
}

//---------------------------------------------------------------------------
//
//	Out�X���b�h
//
//---------------------------------------------------------------------------
UINT CMIDI::OutThread(LPVOID pParam)
{
	CMIDI *pMIDI;

	// �p�����[�^�󂯎��
	pMIDI = (CMIDI*)pParam;

	// ���s
	pMIDI->RunOut();

	// �I��
	return 0;
}

//---------------------------------------------------------------------------
//
//	Out�R�[���o�b�N
//
//---------------------------------------------------------------------------
#if _MFC_VER >= 0x700
void CALLBACK CMIDI::OutProc(HMIDIOUT hOut, UINT wMsg, DWORD_PTR dwInstance,
							DWORD dwParam1, DWORD dwParam2)
#else
void CALLBACK CMIDI::OutProc(HMIDIOUT hOut, UINT wMsg, DWORD dwInstance,
							DWORD dwParam1, DWORD dwParam2)
#endif
{
	CMIDI *pMIDI;

	// �p�����[�^�󂯎��
	pMIDI = (CMIDI*)dwInstance;

	// �Ăяo��
	if (hOut == pMIDI->GetOutHandle()) {
		pMIDI->CallbackOut(wMsg, dwParam1, dwParam2);
	}
}

//---------------------------------------------------------------------------
//
//	�G�N�X�N���[�V�u���M
//	���O��̑��M���������A�w�b�_�̌�Еt�����I����Ă���Ăяo������
//
//---------------------------------------------------------------------------
BOOL FASTCALL CMIDI::SendEx(const BYTE *pExData)
{
	MMRESULT mmResult;
	DWORD dwLength;

	ASSERT(this);
	ASSERT(pExData);
	ASSERT(!m_bSendEx);
	ASSERT(!m_bSendExHdr);
	ASSERT_VALID(this);

	// ���𐔂���
	dwLength = 1;
	while (pExData[dwLength - 1] != 0xf7) {
		ASSERT(dwLength <= sizeof(m_ExBuf));
		dwLength++;
	}

	// ���߂ăf�[�^������
	ASSERT(pExData[0] == 0xf0);
	ASSERT(pExData[dwLength - 1] == 0xf7);

	// �w�b�_����
	memset(&m_ExHdr, 0, sizeof(m_ExHdr));
	m_ExHdr.lpData = (LPSTR)pExData;
	m_ExHdr.dwBufferLength = dwLength;
	m_ExHdr.dwBytesRecorded = dwLength;
	m_OutSection.Lock();
	mmResult = ::midiOutPrepareHeader(m_hOut, &m_ExHdr, sizeof(MIDIHDR));
	m_OutSection.Unlock();
	if (mmResult != MMSYSERR_NOERROR) {
		return FALSE;
	}

	// �G�N�X�N���[�V�u�w�b�_�g�p�t���OON
	m_bSendExHdr = TRUE;

	// �G�N�X�N���[�V�u���M�t���O�𗧂Ă�
	m_bSendEx = TRUE;

	// LongMsg���M
	m_OutSection.Lock();
	mmResult = ::midiOutLongMsg(m_hOut, &m_ExHdr, sizeof(MIDIHDR));
	m_OutSection.Unlock();
	if (mmResult != MMSYSERR_NOERROR) {
		// ���s������t���O���Ƃ�
		m_bSendEx = FALSE;
		return FALSE;
	}

	// ����
	m_dwExSend++;
	return TRUE;
}

//---------------------------------------------------------------------------
//
//	�G�N�X�N���[�V�u���M�����҂�
//
//---------------------------------------------------------------------------
BOOL FASTCALL CMIDI::SendExWait()
{
	DWORD dwTime;
	DWORD dwDiff;
	BOOL bResult;

	ASSERT(this);
	ASSERT_VALID(this);

	// �t���O�͐���
	bResult = TRUE;

	// �G�N�X�N���[�V�u���M���Ȃ�
	if (m_bSendEx) {
		// ���ݎ��Ԃ��擾
		dwTime = ::timeGetTime();

		// ���M�����܂ŃE�F�C�g(�^�C���A�E�g2000ms)
		while (m_bSendEx) {
			// �o�ߎ��Ԃ��Z�o
			dwDiff = ::timeGetTime();
			dwDiff -= dwTime;

			// �^�C���A�E�g�Ȃ玸�s�Ƃ��Ĕ�����
			if (dwDiff >= 2000) {
				bResult = FALSE;
				break;
			}

			// �X���[�v
			::Sleep(1);
		}
	}

	// �G�N�X�N���[�V�u���M�w�b�_���g������Ȃ�A��Еt��
	if (m_bSendExHdr) {
		m_OutSection.Lock();
		::midiOutUnprepareHeader(m_hOut, &m_ExHdr, sizeof(MIDIHDR));
		m_OutSection.Unlock();
		m_bSendExHdr = FALSE;
	}

	return bResult;
}

//---------------------------------------------------------------------------
//
//	�S�m�[�g�I�t
//
//---------------------------------------------------------------------------
void FASTCALL CMIDI::SendAllNoteOff()
{
	int nCh;
	DWORD dwMsg;

	ASSERT(this);
	ASSERT_VALID(this);

	// ���[�h���b�Z�[�WAll Sound Off�𑗐M
	m_OutSection.Lock();
	for (nCh=0; nCh<16; nCh++) {
		dwMsg = (DWORD)(0x78b0 + nCh);
		::midiOutShortMsg(m_hOut, dwMsg);
	}
	m_OutSection.Unlock();

	// �m�[�g�I�t(MT-32�΍�)
	m_OutSection.Lock();
	for (nCh=0; nCh<16; nCh++) {
		::midiOutShortMsg(m_hOut, (DWORD)(0x7bb0 + nCh));
		::midiOutShortMsg(m_hOut, (DWORD)(0x7db0 + nCh));
		::midiOutShortMsg(m_hOut, (DWORD)(0x7fb0 + nCh));
	}
	m_OutSection.Unlock();
}

//---------------------------------------------------------------------------
//
//	�������Z�b�g
//
//---------------------------------------------------------------------------
BOOL FASTCALL CMIDI::SendReset()
{
	int nCh;

	ASSERT(this);
	ASSERT_VALID(this);

	// Reset All Controller�R�}���h
	m_OutSection.Lock();
	for (nCh=0; nCh<16; nCh++) {
		::midiOutShortMsg(m_hOut, (DWORD)(0x79b0 + nCh));
	}
	m_OutSection.Unlock();

	// ���Z�b�g�^�C�v��
	switch (m_nOutReset) {
		// GM
		case 0:
			SendEx(ResetGM);
			break;

		// GS
		case 1:
			// GM->GS�̏��ő��o
			SendEx(ResetGM);
			SendExWait();
			SendEx(ResetGS);
			break;

		// XG
		case 2:
			// GM->XG�̏��ő��o
			SendEx(ResetGM);
			SendExWait();
			SendEx(ResetXG);
			break;

		// LA
		case 3:
			SendEx(ResetLA);
			break;

		// ���̑�(���蓾�Ȃ�)
		default:
			ASSERT(FALSE);
			break;
	};

	// �G�N�X�N���[�V�u���M������҂�
	SendExWait();

	// �J�E���g�N���A
	m_dwShortSend = 0;
	m_dwExSend = 0;
	m_dwUnSend = 0;

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	Out�f�o�C�X�����擾
//
//---------------------------------------------------------------------------
DWORD FASTCALL CMIDI::GetOutDevs() const
{
	ASSERT(this);
	ASSERT_VALID(this);

	return m_dwOutDevs;
}

//---------------------------------------------------------------------------
//
//	Out�f�o�C�X���̂��擾
//
//---------------------------------------------------------------------------
BOOL FASTCALL CMIDI::GetOutDevDesc(int nDevice, CString& strDesc) const
{
	LPCTSTR lpstrDesc;

	ASSERT(this);
	ASSERT(nDevice >= 0);
	ASSERT_VALID(this);

	// �C���f�b�N�X�`�F�b�N
	if (nDevice >= (int)m_dwOutDevs) {
		strDesc.Empty();
		return FALSE;
	}

	// LPCTSTR�֕ϊ�
	lpstrDesc = A2T(m_pOutCaps[nDevice].szPname);
	strDesc = lpstrDesc;

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	Out�f�B���C�ݒ�
//
//---------------------------------------------------------------------------
void FASTCALL CMIDI::SetOutDelay(int nDelay)
{
	ASSERT(this);
	ASSERT(nDelay >= 0);
	ASSERT_VALID(this);

	m_dwOutDelay = (DWORD)(nDelay * 2000);
}

//---------------------------------------------------------------------------
//
//	Out���ʎ擾
//
//---------------------------------------------------------------------------
int FASTCALL CMIDI::GetOutVolume()
{
	MMRESULT mmResult;
	DWORD dwVolume;
	int nVolume;

	ASSERT(this);
	ASSERT_VALID(this);

	// �n���h�����L���ŁA�X���b�h�������Ă��邱��
	if ((m_dwOutDevice > 0) && m_pOutThread) {
		// �N���e�B�J���Z�N�V����������ŁA���ʂ��擾
		m_OutSection.Lock();
		mmResult = ::midiOutGetVolume(m_hOut, &dwVolume);
		m_OutSection.Unlock();

		// ���������ꍇ�ALOWORD(��)��D�悷��
		if (mmResult == MMSYSERR_NOERROR) {
			// ������l��16bit
			nVolume = LOWORD(dwVolume);

			// ����
			return nVolume;
		}
	}

	// MIDI�~�L�T�͖���
	return -1;
}

//---------------------------------------------------------------------------
//
//	Out���ʐݒ�
//
//---------------------------------------------------------------------------
void FASTCALL CMIDI::SetOutVolume(int nVolume)
{
	DWORD dwVolume;

	ASSERT(this);
	ASSERT((nVolume >= 0) && (nVolume < 0x10000));
	ASSERT_VALID(this);

	// �n���h�����L���ŁA�X���b�h�������Ă��邱��
	if ((m_dwOutDevice > 0) && m_pOutThread) {
		// �l���쐬
		dwVolume = (DWORD)nVolume;
		dwVolume <<= 16;
		dwVolume |= (DWORD)nVolume;

		// �N���e�B�J���Z�N�V����������ŁA���ʂ�ݒ�
		m_OutSection.Lock();
		::midiOutSetVolume(m_hOut, dwVolume);
		m_OutSection.Unlock();
	}
}

//---------------------------------------------------------------------------
//
//	Out���擾
//
//---------------------------------------------------------------------------
void FASTCALL CMIDI::GetOutInfo(LPMIDIINFO pInfo) const
{
	ASSERT(this);
	ASSERT(pInfo);
	ASSERT_VALID(this);

	// �f�o�C�X
	pInfo->dwDevices = m_dwOutDevs;
	pInfo->dwDevice = m_dwOutDevice;

	// �J�E���^
	pInfo->dwShort = m_dwShortSend;
	pInfo->dwLong = m_dwExSend;
	pInfo->dwUnprepare = m_dwUnSend;

	// �o�b�t�@����VM�f�o�C�X�������̂ŁA�����ł͐ݒ肵�Ȃ�
}

//---------------------------------------------------------------------------
//
//	GM�������Z�b�g�R�}���h
//
//---------------------------------------------------------------------------
const BYTE CMIDI::ResetGM[] = {
	0xf0, 0x7e, 0x7f, 0x09, 0x01, 0xf7
};

//---------------------------------------------------------------------------
//
//	GS�������Z�b�g�R�}���h
//
//---------------------------------------------------------------------------
const BYTE CMIDI::ResetGS[] = {
	0xf0, 0x41, 0x10, 0x42, 0x12, 0x40, 0x00, 0x7f,
	0x00, 0x41, 0xf7
};

//---------------------------------------------------------------------------
//
//	XG�������Z�b�g�R�}���h
//
//---------------------------------------------------------------------------
const BYTE CMIDI::ResetXG[] = {
	0xf0, 0x43, 0x10, 0x4c, 0x00, 0x00, 0x7e, 0x00,
	0xf7
};

//---------------------------------------------------------------------------
//
//	LA�������Z�b�g�R�}���h
//
//---------------------------------------------------------------------------
const BYTE CMIDI::ResetLA[] = {
	0xf0, 0x41, 0x10, 0x16, 0x12, 0x7f, 0x00, 0x00,
	0x00, 0x01, 0xf7
};

//===========================================================================
//
//	IN����
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	In�I�[�v��
//	��dwDevice=0�͊��蓖�ĂȂ�
//
//---------------------------------------------------------------------------
void FASTCALL CMIDI::OpenIn(DWORD dwDevice)
{
	MMRESULT mmResult;
	UINT uDeviceID;

	ASSERT(this);
	ASSERT(m_dwInDevice == 0);
	ASSERT(!m_pInThread);
	ASSERT_VALID(this);

	// ���蓖�Ă��Ȃ��̂ł���΃��^�[��
	if (dwDevice == 0) {
		return;
	}

	// �f�o�C�X���Ȃ���΃��^�[��
	if (m_dwInDevs == 0) {
		return;
	}

	// �f�o�C�X���𒴂��Ă���΃f�o�C�X0
	if (dwDevice > m_dwInDevs) {
		dwDevice = 1;
	}

	// �f�o�C�X����
	ASSERT(dwDevice >= 1);
	uDeviceID = (UINT)(dwDevice - 1);

	// �I�[�v��

#if _MFC_VER >= 0x700
	mmResult = ::midiInOpen(&m_hIn, uDeviceID,
							(DWORD_PTR)InProc, (DWORD_PTR)this, CALLBACK_FUNCTION);
#else
	mmResult = ::midiInOpen(&m_hIn, uDeviceID,
							(DWORD)InProc, (DWORD)this, CALLBACK_FUNCTION);
#endif

	// ���s�Ȃ�I��
	if (mmResult != MMSYSERR_NOERROR) {
		return;
	}

	// �f�o�C�X�ԍ����X�V
	m_dwInDevice = dwDevice;

	// �X���b�h�𗧂Ă�
	m_bInThread = FALSE;
	m_pInThread = AfxBeginThread(InThread, this);
}

//---------------------------------------------------------------------------
//
//	In�N���[�Y
//
//---------------------------------------------------------------------------
void FASTCALL CMIDI::CloseIn()
{
	ASSERT(this);
	ASSERT_VALID(this);

	// �X���b�h���~�߂�
	if (m_pInThread) {
		// �I���t���O���グ
		m_bInThread = TRUE;

		// �I���҂�(������)
		::WaitForSingleObject(m_pInThread->m_hThread, INFINITE);

		// �X���b�h�Ȃ�
		m_pInThread = NULL;
	}

	// �f�o�C�X�����
	if (m_dwInDevice != 0) {
		// ���Z�b�g���āA�N���[�Y
		::midiInReset(m_hIn);
		::midiInClose(m_hIn);

		// �I�[�v���f�o�C�X�Ȃ�
		m_dwInDevice = 0;
	}
}

//---------------------------------------------------------------------------
//
//	In�X���b�h
//
//---------------------------------------------------------------------------
void FASTCALL CMIDI::RunIn()
{
	int i;
	BOOL bActive;
	BOOL bValid;
	BOOL bMsg;

	ASSERT(this);
	ASSERT(m_hIn);
	ASSERT_VALID(this);

	// ��A�N�e�B�u
	bActive = FALSE;

	// �G�N�X�N���[�V�u�o�b�t�@���g�p
	m_InState[0] = InNotUsed;
	m_InState[1] = InNotUsed;

	// �J�E���^���Z�b�g
	m_dwShortRecv = 0;
	m_dwExRecv = 0;
	m_dwUnRecv = 0;

	// ���Z�b�g
	::midiInReset(m_hIn);

	// ���[�v
	while (!m_bInThread) {
		// �L���t���O�ݒ�
		bValid = FALSE;
		if (m_pSch->IsEnable()) {
			// �X�P�W���[�����쒆��
			if (m_pMIDI->IsActive()) {
				// MIDI�A�N�e�B�u�Ȃ�
				bValid = TRUE;
			}
		}

		// �����ȏꍇ
		if (!bValid) {
			// Sleep
			::Sleep(10);
			continue;
		}

		// �L���Ȃ̂ŁA�܂��X�^�[�g���Ă��Ȃ���΃X�^�[�g
		if (!bActive) {
			StartIn();
			bActive = TRUE;
		}

		// ���b�Z�[�W�����Ȃ�
		bMsg = FALSE;

		// �����O���b�Z�[�W�̌���
		for (i=0; i<2; i++) {
			if (m_InState[i] == InDone) {
				// �����O���b�Z�[�W��M�ς�
				LongIn(i);
				bMsg = TRUE;
			}
		}

		// �V���[�g���b�Z�[�W�̌���
		m_InSection.Lock();
		if (!m_InQueue.IsEmpty()) {
			// �V���[�g���b�Z�[�W��M�ς�
			ShortIn();
			bMsg = TRUE;
		}
		m_InSection.Unlock();

		// ���b�Z�[�W�����Ȃ����Sleep
		if (!bMsg) {
			::Sleep(1);
		}
	}

	// ��~
	if (bActive) {
		bActive = FALSE;
		StopIn();
	}
}

//---------------------------------------------------------------------------
//
//	In�J�n
//
//---------------------------------------------------------------------------
BOOL FASTCALL CMIDI::StartIn()
{
	int i;
	MMRESULT mmResult;

	ASSERT(this);
	ASSERT(m_hIn);
	ASSERT_VALID(this);

	// �G�N�X�N���[�V�u�o�b�t�@��o�^
	for (i=0; i<2; i++) {
		if (m_InState[i] == InNotUsed) {
			// MIDI�w�b�_�쐬
			memset(&m_InHdr[i], 0, sizeof(MIDIHDR));
			m_InHdr[i].lpData = (LPSTR)&m_InBuf[i][0];
			m_InHdr[i].dwBufferLength = InBufMax;
			m_InHdr[i].dwUser = (DWORD)i;

			// �w�b�_����
			mmResult = ::midiInPrepareHeader(m_hIn, &m_InHdr[i], sizeof(MIDIHDR));
			if (mmResult != MMSYSERR_NOERROR) {
				// �w�b�_�������s
				continue;
			}

			// �w�b�_�ǉ�
			mmResult = ::midiInAddBuffer(m_hIn, &m_InHdr[i], sizeof(MIDIHDR));
			if (mmResult != MMSYSERR_NOERROR) {
				// �w�b�_�ǉ����s
				::midiInUnprepareHeader(m_hIn, &m_InHdr[i], sizeof(MIDIHDR));
				continue;
			}

			// �X�e�[�g�ݒ�
			m_InState[i] = InReady;
		}
	}

	// In�L���[���N���A
	m_InQueue.Clear();

	// MIDI���͊J�n
	mmResult = ::midiInStart(m_hIn);
	if (mmResult != MMSYSERR_NOERROR) {
		return FALSE;
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	In��~(�o�b�t�@�̂�)
//
//---------------------------------------------------------------------------
void FASTCALL CMIDI::StopIn()
{
	int i;

	ASSERT(this);
	ASSERT(m_hIn);
	ASSERT_VALID(this);

	// �G�N�X�N���[�V�u�o�b�t�@���폜
	for (i=0; i<2; i++) {
		// �w�b�_�폜
		if (m_InState[i] != InNotUsed) {
			::midiInUnprepareHeader(m_hIn, &m_InHdr[i], sizeof(MIDIHDR));
			m_InState[i] = InNotUsed;
		}
	}
}

//---------------------------------------------------------------------------
//
//	In�R�[���o�b�N
//
//---------------------------------------------------------------------------
void FASTCALL CMIDI::CallbackIn(UINT wMsg, DWORD dwParam1, DWORD /*dwParam2*/)
{
	MIDIHDR *lpMidiHdr;
	DWORD dwLength;
	BYTE msg[3];

	ASSERT(this);
	ASSERT_VALID(this);

	switch (wMsg) {
		// �V���[�g���b�Z�[�W
		case MIM_DATA:
		case MIM_MOREDATA:
			// ���b�Z�[�W���쐬
			msg[0] = (BYTE)dwParam1;
			msg[1] = (BYTE)(dwParam1 >> 8);
			msg[2] = (BYTE)(dwParam1 >> 16);

			// �X�e�[�^�X�o�C�g�ɉ����Ē��������߂�
			if (msg[0] < MIDI_NOTEOFF) {
				// �����j���O�X�e�[�^�X�͖����̂ŁA�ُ�f�[�^
				break;
			}
			dwLength = 3;
			if ((msg[0] >= MIDI_PROGCHG) && (msg[0] < MIDI_PITCHCHG)){
				dwLength = 2;
			}
			if (msg[0] >= MIDI_EXSTART) {
				// F0�ȏ�̃��b�Z�[�W
				switch (msg[0]) {
					// �\���O�|�W�V�����|�C���^
					case MIDI_SONGPTR:
						dwLength = 3;
						break;

					// �^�C���R�[�h�N�H�[�^�[�t���[��
					case MIDI_QFRAME:
					// �\���O�Z���N�g
					case MIDI_SONGSEL:
						dwLength = 2;
						break;

					// �`���[���Z���N�g
					case MIDI_TUNEREQ:
					// �^�C�~���O�N���b�N
					case MIDI_TIMINGCLK:
					// �X�^�[�g
					case MIDI_START:
					// �R���e�B�j���[
					case MIDI_CONTINUE:
					// �X�g�b�v
					case MIDI_STOP:
					// �A�N�e�B�u�Z���V���O
					case MIDI_ACTSENSE:
					// �V�X�e�����Z�b�g
					case MIDI_SYSRESET:
						dwLength = 1;

					// ���̑��͎󂯎��Ȃ�
					default:
						dwLength = 0;
				}
			}

			// �f�[�^�ǉ�
			if (dwLength > 0) {
				// ���b�N
				m_InSection.Lock();

				// �}��
				m_InQueue.Insert(msg, dwLength);

				// �A�����b�N
				m_InSection.Unlock();

				// �J�E���g�A�b�v
				m_dwShortRecv++;
			}
			break;

		// �����O���b�Z�[�W
		case MIM_LONGDATA:
		case MIM_LONGERROR:
			// �t���O�ύX
			lpMidiHdr = (MIDIHDR*)dwParam1;
			if (lpMidiHdr->dwUser < 2) {
				m_InState[lpMidiHdr->dwUser] = InDone;

				// �J�E���g�A�b�v
				m_dwExRecv++;
			}
			break;

		// ���̑��͏������Ȃ�
		default:
			break;
	}
}

//---------------------------------------------------------------------------
//
//	�V���[�g���b�Z�[�W��M
//	���N���e�B�J���Z�N�V�����̓��b�N���
//
//---------------------------------------------------------------------------
void FASTCALL CMIDI::ShortIn()
{
	BYTE buf[InBufMax];
	DWORD dwReceived;

	ASSERT(this);
	ASSERT(m_pMIDI);
	ASSERT_VALID(this);

	// �X�^�b�N��ɂ��ׂĎ擾
	dwReceived = m_InQueue.Get(buf);
	ASSERT(dwReceived > 0);

	// MIDI�f�o�C�X�֑��M
	m_pMIDI->SetRecvData(buf, (int)dwReceived);
}

//---------------------------------------------------------------------------
//
//	�����O���b�Z�[�W��M
//
//---------------------------------------------------------------------------
void FASTCALL CMIDI::LongIn(int nHdr)
{
	int i;
	MMRESULT mmResult;

	ASSERT(this);
	ASSERT((nHdr == 0) || (nHdr == 1));
	ASSERT(m_hIn);
	ASSERT_VALID(this);

	// ����Ɏ�M�ł������Ƃ��m�F
	ASSERT(m_InHdr[nHdr].dwFlags & MHDR_DONE);

	// MIDI�f�o�C�X�֑��M
	m_pMIDI->SetRecvData((const BYTE*)m_InHdr[nHdr].lpData,
						 (int)m_InHdr[nHdr].dwBytesRecorded);

	// �w�b�_�����
	::midiInUnprepareHeader(m_hIn, &m_InHdr[nHdr], sizeof(MIDIHDR));
	m_InState[nHdr] = InNotUsed;
	m_dwUnRecv++;

	// �ăX�^�[�g
	for (i=0; i<2; i++) {
		if (m_InState[i] == InNotUsed) {
			// MIDI�w�b�_�쐬
			memset(&m_InHdr[i], 0, sizeof(MIDIHDR));
			m_InHdr[i].lpData = (LPSTR)&m_InBuf[i][0];
			m_InHdr[i].dwBufferLength = InBufMax;
			m_InHdr[i].dwUser = (DWORD)i;

			// �w�b�_����
			mmResult = ::midiInPrepareHeader(m_hIn, &m_InHdr[i], sizeof(MIDIHDR));
			if (mmResult != MMSYSERR_NOERROR) {
				// �w�b�_�������s
				continue;
			}

			// �w�b�_�ǉ�
			mmResult = ::midiInAddBuffer(m_hIn, &m_InHdr[i], sizeof(MIDIHDR));
			if (mmResult != MMSYSERR_NOERROR) {
				// �w�b�_�ǉ����s
				::midiInUnprepareHeader(m_hIn, &m_InHdr[i], sizeof(MIDIHDR));
				continue;
			}

			// �X�e�[�g�ݒ�
			m_InState[i] = InReady;
		}
	}
}

//---------------------------------------------------------------------------
//
//	In�f�o�C�XCaps�擾
//
//---------------------------------------------------------------------------
void FASTCALL CMIDI::GetInCaps()
{
	MMRESULT mmResult;
#if _MFC_VER >= 0x700
	UINT_PTR uDeviceID;
#else
	UINT uDeviceID;
#endif	// MFC_VER

	// �f�o�C�XID������
	uDeviceID = 0;

	// ���[�v
	while ((DWORD)uDeviceID < m_dwInDevs) {
		// midiInGetDevCaps�Ŏ擾
		mmResult = ::midiInGetDevCaps(uDeviceID,
									  &m_pInCaps[uDeviceID],
									  sizeof(MIDIINCAPS));

		// ���ʂ��L�^
		if (mmResult == MMSYSERR_NOERROR) {
			m_pInCaps[uDeviceID].dwSupport = 0;
		}
		else {
			m_pInCaps[uDeviceID].dwSupport = 1;
		}

		// �f�o�C�XID������
		uDeviceID++;
	}
}

//---------------------------------------------------------------------------
//
//	In�X���b�h
//
//---------------------------------------------------------------------------
UINT CMIDI::InThread(LPVOID pParam)
{
	CMIDI *pMIDI;

	// �p�����[�^�󂯎��
	pMIDI = (CMIDI*)pParam;

	// ���s
	pMIDI->RunIn();

	// �I��
	return 0;
}

//---------------------------------------------------------------------------
//
//	In�R�[���o�b�N
//
//---------------------------------------------------------------------------
#if _MFC_VER >= 0x700
void CALLBACK CMIDI::InProc(HMIDIIN hIn, UINT wMsg, DWORD_PTR dwInstance,
							DWORD dwParam1, DWORD dwParam2)
#else
void CALLBACK CMIDI::InProc(HMIDIIN hIn, UINT wMsg, DWORD dwInstance,
							DWORD dwParam1, DWORD dwParam2)
#endif
{
	CMIDI *pMIDI;

	// �p�����[�^�󂯎��
	pMIDI = (CMIDI*)dwInstance;

	// �Ăяo��
	if (hIn == pMIDI->GetInHandle()) {
		pMIDI->CallbackIn(wMsg, dwParam1, dwParam2);
	}
}

//---------------------------------------------------------------------------
//
//	In�f�o�C�X�����擾
//
//---------------------------------------------------------------------------
DWORD FASTCALL CMIDI::GetInDevs() const
{
	ASSERT(this);
	ASSERT_VALID(this);

	return m_dwInDevs;
}

//---------------------------------------------------------------------------
//
//	In�f�o�C�X���̂��擾
//
//---------------------------------------------------------------------------
BOOL FASTCALL CMIDI::GetInDevDesc(int nDevice, CString& strDesc) const
{
	LPCTSTR lpstrDesc;

	ASSERT(this);
	ASSERT(nDevice >= 0);
	ASSERT_VALID(this);

	// �C���f�b�N�X�`�F�b�N
	if (nDevice >= (int)m_dwInDevs) {
		strDesc.Empty();
		return FALSE;
	}

	// LPCTSTR�֕ϊ�
	lpstrDesc = A2T(m_pInCaps[nDevice].szPname);
	strDesc = lpstrDesc;

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	In�f�B���C�ݒ�
//
//---------------------------------------------------------------------------
void FASTCALL CMIDI::SetInDelay(int nDelay)
{
	ASSERT(this);
	ASSERT(nDelay >= 0);
	ASSERT_VALID(this);

	// �f�o�C�X�֒ʒm
	m_pMIDI->SetRecvDelay(nDelay);
}

//---------------------------------------------------------------------------
//
//	In���擾
//
//---------------------------------------------------------------------------
void FASTCALL CMIDI::GetInInfo(LPMIDIINFO pInfo) const
{
	ASSERT(this);
	ASSERT(pInfo);
	ASSERT_VALID(this);

	// �f�o�C�X
	pInfo->dwDevices = m_dwInDevs;
	pInfo->dwDevice = m_dwInDevice;

	// �J�E���^
	pInfo->dwShort = m_dwShortRecv;
	pInfo->dwLong = m_dwExRecv;
	pInfo->dwUnprepare = m_dwUnRecv;

	// �o�b�t�@����VM�f�o�C�X�������̂ŁA�����ł͐ݒ肵�Ȃ�
}

#endif	// _WIN32
