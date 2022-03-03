//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2004 �o�h�D(ytanaka@ipc-tokai.or.jp)
//	Modified (C) 2006 co (cogood��gmail.com)
//	[ Windrv ]
//
//---------------------------------------------------------------------------

#include "os.h"
#include "xm6.h"
#include "vm.h"
#include "log.h"
#include "schedule.h"
#include "memory.h"
#include "cpu.h"
#include "config.h"
#include "windrv.h"
#include "mfc_com.h"
#include "mfc_cfg.h"

//===========================================================================
//
//	Human68k
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	�p�X�擾
//
//---------------------------------------------------------------------------
void FASTCALL Human68k::namests_t::GetCopyPath(BYTE* szPath) const
{
	ASSERT(this);
	ASSERT(szPath);
	// WARNING: Unicode�Ή����v�C��

	BYTE* p = szPath;
	for (int i = 0; i < 65; i++) {
		BYTE c = path[i];
		if (c == '\0') break;
		if (c == 0x09) {
			c = '/';
		}
		*p++ = c;
	}

	*p = '\0';
}

//---------------------------------------------------------------------------
//
//	�t�@�C�����擾
//
//	TwentyOne�Ő��������G���g���Ɠ����̃t�@�C�����Č��\�͂���������B
//
//	���z�f�B���N�g���G���g���@�\�������ȏꍇ:
//	Human68k�����Ń��C���h�J�[�h���������ׂ�'?'�ɓW�J����Ă��܂���
//	�߁A18 + 3������蒷���t�@�C�����̌������s�\�ƂȂ�B���̂悤��
//	�ꍇ�́A���C���h�J�[�h�����ϊ����s�Ȃ����ƂŁAFILES�ɂ��S�t�@
//	�C���̌������\�ƂȂ�B�Ȃ��A�������\�ɂȂ邾���ŃI�[�v���͂�
//	���Ȃ��̂Œ��ӁB
//
//	���ׂẴt�@�C�����I�[�v���������ꍇ�̓f�B���N�g���G���g���L���b
//	�V����L���ɂ��邱�ƁB�t�@�C�����ϊ����s�v�ɂȂ�f���ɓ��삷��B
//
//---------------------------------------------------------------------------
void FASTCALL Human68k::namests_t::GetCopyFilename(BYTE* szFilename) const
{
	ASSERT(this);
	ASSERT(szFilename);
	// WARNING: Unicode�Ή����v�C��

	int i;
	BYTE* p = szFilename;
#ifdef XM6_WINDRV_WILDCARD
	BYTE* pWild = NULL;
#endif // XM6_WINDRV_WILDCARD

	// �t�@�C�����{�̓]��
	for (i = 0; i < 8; i++) {
		BYTE c = name[i];
		if (c == ' ') {
			BOOL bTerminate = TRUE;
			// �t�@�C�������ɃX�y�[�X���o�������ꍇ�A�ȍ~�̃G���g���������Ă��邩�ǂ����m�F
			// add[0] ���L���ȕ����Ȃ瑱����
			if (add[0] != '\0') {
				bTerminate = FALSE;
			} else {
				// name[i] ����ɋ󔒈ȊO�̕��������݂���Ȃ瑱����
				for (int j = i + 1; j < 8; j++) {
					if (name[j] != ' ') {
						bTerminate = FALSE;
						break;
					}
				}
			}
			// �t�@�C�����I�[�Ȃ�]���I��
			if (bTerminate) break;
		}
#ifdef XM6_WINDRV_WILDCARD
		if (c == '?') {
			if (pWild == NULL) pWild = p;
		} else {
			pWild = NULL;
		}
#endif // XM6_WINDRV_WILDCARD
		*p++ = c;
	}
	// �S�Ă̕�����ǂݍ��ނƁA������ i >= 8 �ƂȂ�

	// �t�@�C�����{�̂�8�����ȏ�Ȃ�ǉ�������������
	if (i >= 8) {
		// �t�@�C�����ǉ������]��
		for (i = 0; i < 10; i++) {
			BYTE c = add[i];
			if (c == '\0') break;
#ifdef XM6_WINDRV_WILDCARD
			if (c == '?') {
				if (pWild == NULL) pWild = p;
			} else {
				pWild = NULL;
			}
#endif // XM6_WINDRV_WILDCARD
			*p++ = c;
		}
		// �S�Ă̕�����ǂݍ��ނƁA������ i >= 10 �ƂȂ�
	}

#ifdef XM6_WINDRV_WILDCARD
	// �t�@�C�������������C���h�J�[�h�ŁA���㔼10�����̂��ׂĂ��g�p���Ă��邩�H
	if (pWild && i >= 10) {
		p = pWild;
		*p++ = '*';
	}
#endif // XM6_WINDRV_WILDCARD

	// �t�@�C�����{�̂͏I���A�g���q�̏����J�n
#ifdef XM6_WINDRV_WILDCARD
	pWild = NULL;
#endif // XM6_WINDRV_WILDCARD
	i = 0;

	// �g���q�����݂��邩�H
	if (ext[0] == ' ' && ext[1] == ' ' && ext[2] == ' ') {
		// �g���q�Ȃ�
	} else {
		// �g���q�]��
		*p++ = '.';
		for (i = 0; i < 3; i++) {
			BYTE c = ext[i];
			if (c == ' ') {
				BOOL bTerminate = TRUE;
				// �g���q���ɃX�y�[�X���o�������ꍇ�A�ȍ~�̃G���g���������Ă��邩�ǂ����m�F
				// ext[i] ����ɋ󔒈ȊO�̕��������݂���Ȃ瑱����
				for (int j = i + 1; j < 3; j++) {
					if (ext[j] != ' ') {
						bTerminate = FALSE;
						break;
					}
				}
				// �g���q�I�[�Ȃ�]���I��
				if (bTerminate) break;
			}
#ifdef XM6_WINDRV_WILDCARD
			if (c == '?') {
				if (pWild == NULL) pWild = p;
			} else {
				pWild = NULL;
			}
#endif // XM6_WINDRV_WILDCARD
			*p++ = c;
		}
		// �S�Ă̕�����ǂݍ��ނƁA������ i >= 3 �ƂȂ�
	}

#ifdef XM6_WINDRV_WILDCARD
	// �g���q���������C���h�J�[�h�ŁA���g���q3�������ׂĂ��g�p���Ă��邩�H
	if (pWild && i >= 3) {
		p = pWild;
		*p++ = '*';
	}
#endif // XM6_WINDRV_WILDCARD

	// �ԕ��ǉ�
	*p = '\0';
}

//===========================================================================
//
//	�R�}���h�n���h��
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	�R���X�g���N�^
//
//---------------------------------------------------------------------------
CWindrv::CWindrv(Windrv* pWindrv, Memory* pMemory, DWORD nHandle)
{
	windrv = pWindrv;
	memory = pMemory;
	a5 = 0;
	unit = 0;
	command = 0;

	m_nHandle = nHandle;
	m_bAlloc = FALSE;
	m_bExecute = FALSE;
	m_bReady = FALSE;
	m_bTerminate = FALSE;
	m_hThread = NULL;
	m_hEventStart = NULL;
	m_hEventReady = NULL;
}

//---------------------------------------------------------------------------
//
//	�f�X�g���N�^
//
//---------------------------------------------------------------------------
CWindrv::~CWindrv()
{
	Terminate();
}

//---------------------------------------------------------------------------
//
//	�X���b�h�N��
//
//---------------------------------------------------------------------------
BOOL FASTCALL CWindrv::Start()
{
	ASSERT(this);
	ASSERT(m_bAlloc == FALSE);
	ASSERT(m_bExecute == FALSE);
	ASSERT(m_bTerminate == FALSE);
	ASSERT(m_hThread == NULL);
	ASSERT(m_hEventStart == NULL);
	ASSERT(m_hEventReady == NULL);

	// �n���h���m��
	m_hEventStart = ::CreateEvent(NULL, FALSE, FALSE, NULL);	// �������Z�b�g
	ASSERT(m_hEventStart);

	m_hEventReady = ::CreateEvent(NULL, TRUE, FALSE, NULL);	// �蓮���Z�b�g ����
	ASSERT(m_hEventReady);

	// �X���b�h����
	DWORD nThread;
	m_hThread = ::CreateThread(NULL, 0, Run, this, 0, &nThread);
	ASSERT(m_hThread);

	// �G���[�`�F�b�N
	if (m_hEventStart == NULL ||
		m_hEventReady == NULL ||
		m_hThread == NULL) return FALSE;

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	�X���b�h��~
//
//---------------------------------------------------------------------------
BOOL FASTCALL CWindrv::Terminate()
{
	ASSERT(this);

	BOOL bResult = TRUE;

	if (m_hThread) {
		ASSERT(m_bTerminate == FALSE);
		ASSERT(m_hEventStart);

		// �X���b�h�֒�~�v��
		m_bTerminate = TRUE;
		::SetEvent(m_hEventStart);

		// �X���b�h�I���҂�
		DWORD nResult;
		nResult = ::WaitForSingleObject(m_hThread, 30 * 1000);	// �P�\���� 30�b
		if (nResult != WAIT_OBJECT_0) {
			// ������~
			ASSERT(FALSE);	// �O�̂���
			::TerminateThread(m_hThread, -1);
			nResult = ::WaitForSingleObject(m_hThread, 100);
			bResult = FALSE;
		}

		// �X���b�h�n���h���J��
		::CloseHandle(m_hThread);
		m_hThread = NULL;

		// ��~�v������艺����
		m_bTerminate = FALSE;
	}

	// �C�x���g�n���h���J��
	if (m_hEventStart) {
		::CloseHandle(m_hEventStart);
		m_hEventStart = NULL;
	}

	if (m_hEventReady) {
		::CloseHandle(m_hEventReady);
		m_hEventReady = NULL;
	}

	// ���̂܂ܓ����I�u�W�F�N�g�ŃX���b�h�N���ł���悤������
	m_bAlloc = FALSE;
	m_bExecute = FALSE;
	m_bReady = FALSE;
	m_bTerminate = FALSE;

	return bResult;
}

//---------------------------------------------------------------------------
//
//	�R�}���h���s
//
//---------------------------------------------------------------------------
void FASTCALL CWindrv::Execute(DWORD nA5)
{
	ASSERT(this);
	ASSERT(m_bExecute == FALSE);
	ASSERT(m_bTerminate == FALSE);
	ASSERT(m_hThread);
	ASSERT(m_hEventStart);
	ASSERT(m_hEventReady);
	ASSERT(nA5 <= 0xFFFFFF);

	// ���s�J�n�t���O�Z�b�g
	m_bExecute = TRUE;

	// ���N�G�X�g�w�b�_�̃A�h���X
	a5 = nA5;

	// �G���[�����N���A
	SetByte(a5 + 3, 0);
	SetByte(a5 + 4, 0);

	// ���j�b�g�ԍ��l��
	unit = GetByte(a5 + 1);

	// �R�}���h�l��
	command = GetByte(a5 + 2);

	// �X���b�h�ҋ@����
	m_bReady = FALSE;
	::ResetEvent(m_hEventReady);

	// ���s�J�n�C�x���g�Z�b�g
	::SetEvent(m_hEventStart);

	// �}���`�X���b�h�J�n�v��������܂ł͎����V���O���X���b�h�Ƃ��ĐU�镑��
	DWORD nResult = ::WaitForSingleObject(m_hEventReady, INFINITE);
	ASSERT(nResult == WAIT_OBJECT_0);	// �O�̂���
}

#ifdef WINDRV_SUPPORT_COMPATIBLE
//---------------------------------------------------------------------------
//
//	�R�}���h���s (WINDRV�݊�)
//
//---------------------------------------------------------------------------
DWORD FASTCALL CWindrv::ExecuteCompatible(DWORD nA5)
{
	ASSERT(this);
	ASSERT(nA5 <= 0xFFFFFF);

	// ���N�G�X�g�w�b�_�̃A�h���X
	a5 = nA5;

	// �G���[�����N���A
	SetByte(a5 + 3, 0);
	SetByte(a5 + 4, 0);

	// ���j�b�g�ԍ��l��
	unit = GetByte(a5 + 1);

	// �R�}���h�l��
	command = GetByte(a5 + 2);

	// �R�}���h���s
	ExecuteCommand();

	return GetByte(a5 + 3) | (GetByte(a5 + 4) << 8);
}
#endif // WINDRV_SUPPORT_COMPATIBLE

//---------------------------------------------------------------------------
//
//	�R�}���h������҂�����VM�X���b�h���s�ĊJ
//
//---------------------------------------------------------------------------
void FASTCALL CWindrv::Ready()
{
	ASSERT(this);
	ASSERT(m_hEventReady);

	m_bReady = TRUE;
	::SetEvent(m_hEventReady);
}

//---------------------------------------------------------------------------
//
//	�X���b�h���s�J�n�|�C���g
//
//---------------------------------------------------------------------------
DWORD WINAPI CWindrv::Run(VOID* pThis)
{
	ASSERT(pThis);

	((CWindrv*)pThis)->Runner();

	::ExitThread(0);
	return 0;
}

//---------------------------------------------------------------------------
//
//	�X���b�h����
//
//---------------------------------------------------------------------------
void FASTCALL CWindrv::Runner()
{
	ASSERT(this);
	ASSERT(m_hEventStart);
	ASSERT(m_hEventReady);
	//ASSERT(m_bTerminate == FALSE);
	// �X���b�h�J�n����ɂ����Ȃ�Terminate����ƁA���̎��_�� m_bTerminate ��
	// TRUE�ƂȂ�\���������B���Ԍo�߂ɂ���ďI�����A�I���̏u�Ԃɓ�������
	// ���邽��ASSERT�Ȃ��ł����Ȃ��B

	for (;;) {
		DWORD nResult = ::WaitForSingleObject(m_hEventStart, INFINITE);
		if (nResult != WAIT_OBJECT_0) {
			ASSERT(FALSE);	// �O�̂���
			break;
		}
		if (m_bTerminate) break;

		// ���s
		ExecuteCommand();

		// ���s�I���O�ɒʒm
		Ready();

		// ���s����
		m_bExecute = FALSE;
	}

	Ready();	// �O�̂���
}

//===========================================================================
//
//	�R�}���h�n���h���Ǘ�
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	������
//
//	�S�X���b�h�m�ۂ��A�ҋ@��Ԃɂ���B
//
//---------------------------------------------------------------------------
void FASTCALL CWindrvManager::Init(Windrv* pWindrv, Memory* pMemory)
{
	ASSERT(this);
	ASSERT(pWindrv);
	ASSERT(pMemory);

	for (int i = 0; i < WINDRV_THREAD_MAX; i++) {
		m_pThread[i] = new CWindrv(pWindrv, pMemory, i);
		m_pThread[i]->Start();
	}
}

//---------------------------------------------------------------------------
//
//	�I��
//
//	�S�ẴX���b�h���~������B
//
//---------------------------------------------------------------------------
void FASTCALL CWindrvManager::Clean()
{
	ASSERT(this);

	for (int i = 0; i < WINDRV_THREAD_MAX; i++) {
		ASSERT(m_pThread[i]);	// �O�̂���
		delete m_pThread[i];
		m_pThread[i] = NULL;
	}
}

//---------------------------------------------------------------------------
//
//	���Z�b�g
//
//	���s���̃X���b�h�̂ݍċN��������B
//
//---------------------------------------------------------------------------
void FASTCALL CWindrvManager::Reset()
{
	ASSERT(this);

	for (int i = 0; i < WINDRV_THREAD_MAX; i++) {
		ASSERT(m_pThread[i]);	// �O�̂���
		if (m_pThread[i]->isAlloc()) {
			m_pThread[i]->Terminate();
			m_pThread[i]->Start();
		}
	}
}

//---------------------------------------------------------------------------
//
//	�󂫃X���b�h�m��
//
//---------------------------------------------------------------------------
CWindrv* FASTCALL CWindrvManager::Alloc()
{
	ASSERT(this);

	for (int i = 0; i < WINDRV_THREAD_MAX; i++) {
		if (m_pThread[i]->isAlloc() == FALSE) {
			m_pThread[i]->SetAlloc(TRUE);
			return m_pThread[i];
		}
	}

	return NULL;
}

//---------------------------------------------------------------------------
//
//	�X���b�h����
//
//---------------------------------------------------------------------------
CWindrv* FASTCALL CWindrvManager::Search(DWORD nHandle)
{
	ASSERT(this);

	if (nHandle >= WINDRV_THREAD_MAX) return NULL;
	return m_pThread[nHandle];
}

//---------------------------------------------------------------------------
//
//	�X���b�h�J��
//
//---------------------------------------------------------------------------
void FASTCALL CWindrvManager::Free(CWindrv* p)
{
	ASSERT(this);
	ASSERT(p);

	p->SetAlloc(FALSE);
}

//===========================================================================
//
//	Windrv
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	�R���X�g���N�^
//
//---------------------------------------------------------------------------
Windrv::Windrv(VM *p) : MemDevice(p)
{
	// �f�o�C�XID��������
	dev.id = MAKEID('W', 'D', 'R', 'V');
	dev.desc = "Windrv";

	// �J�n�A�h���X�A�I���A�h���X
	memdev.first = 0xe9e000;
	memdev.last = 0xe9ffff;
}

//---------------------------------------------------------------------------
//
//	������
//
//---------------------------------------------------------------------------
BOOL FASTCALL Windrv::Init()
{
	ASSERT(this);

	// ��{�N���X
	if (!MemDevice::Init()) {
		return FALSE;
	}

	// �O���[�o���t���O��������
	windrv.enable = WINDRV_MODE_NONE;

	// �h���C�u0�A�t�@�C���V�X�e������
	windrv.drives = 0;
	windrv.fs = NULL;

	// �X���b�h������
	Memory* memory = (Memory*)vm->SearchDevice(MAKEID('M', 'E', 'M', ' '));
	ASSERT(memory);
	m_cThread.Init(this, memory);

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	�N���[���A�b�v
//
//---------------------------------------------------------------------------
void FASTCALL Windrv::Cleanup()
{
	ASSERT(this);

	// �X���b�h�S�J��
	m_cThread.Clean();

	// �t�@�C���V�X�e��������΁A���Z�b�g
	if (windrv.fs) {
		windrv.fs->Reset();
	}

	// ��{�N���X��
	MemDevice::Cleanup();
}

//---------------------------------------------------------------------------
//
//	���Z�b�g
//
//---------------------------------------------------------------------------
void FASTCALL Windrv::Reset()
{
	ASSERT(this);

	LOG(Log::Normal, "���Z�b�g");

	// �X���b�h�����s���ł���΁A���Z�b�g
	m_cThread.Reset();

	// �t�@�C���V�X�e��������΁A���Z�b�g
	if (windrv.fs) {
		windrv.fs->Reset();
	}

	// �h���C�u��������
	windrv.drives = 0;
}

//---------------------------------------------------------------------------
//
//	�Z�[�u
//
//---------------------------------------------------------------------------
BOOL FASTCALL Windrv::Save(Fileio *fio, int ver)
{
	ASSERT(this);
	LOG(Log::Normal, "�Z�[�u");

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	���[�h
//
//---------------------------------------------------------------------------
BOOL FASTCALL Windrv::Load(Fileio *fio, int ver)
{
	ASSERT(this);
	LOG(Log::Normal, "���[�h");

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	�ݒ�K�p
//
//---------------------------------------------------------------------------
void FASTCALL Windrv::ApplyCfg(const Config* pConfig)
{
	ASSERT(this);
	ASSERT(pConfig);
	LOG(Log::Normal, "�ݒ�K�p");

	// ����t���O��ݒ�
	windrv.enable = pConfig->windrv_enable;
}

//---------------------------------------------------------------------------
//
//	�o�C�g�ǂݍ���
//
//---------------------------------------------------------------------------
DWORD FASTCALL Windrv::ReadByte(DWORD addr)
{
	ASSERT(this);
	ASSERT((addr >= memdev.first) && (addr <= memdev.last));

	// �|�[�g1
	if (addr == 0xE9F001) {
		return StatusAsynchronous();
	}

	DWORD result = ReadOnly(addr);

#ifdef WINDRV_LOG
	switch (result) {
	case 'X':
		LOG(Log::Normal, "Windrv�|�[�g ���T�|�[�g");
		break;
	case 'Y':
		LOG(Log::Normal, "Windrv");
		break;
#ifdef WINDRV_SUPPORT_COMPATIBLE
	case 'W':
		LOG(Log::Normal, "Windrv�|�[�g �݊�����");
		break;
#endif // WINDRV_SUPPORT_COMPATIBLE
	}
#endif // WINDRV_LOG

	if (result == 0xFF) {
		cpu->BusErr(addr, TRUE);
	}

	return result;
}

//---------------------------------------------------------------------------
//
//	�ǂݍ��݂̂�
//
//---------------------------------------------------------------------------
DWORD FASTCALL Windrv::ReadOnly(DWORD addr) const
{
	ASSERT(this);
	ASSERT((addr >= memdev.first) && (addr <= memdev.last));

	// ���ʃ|�[�g�ȊO��-1
	if (addr != 0xE9F000) {
		return 0xFF;
	}

	switch (windrv.enable) {
	case WINDRV_MODE_DUAL:
	case WINDRV_MODE_ENABLE:
		return 'Y';			// WindrvXM

	case WINDRV_MODE_COMPATIBLE:
#ifdef WINDRV_SUPPORT_COMPATIBLE
		return 'W';			// WINDRV�݊�
#endif // WINDRV_SUPPORT_COMPATIBLE

	case WINDRV_MODE_NONE:
		return 'X';			// �g�p�s�� (I/O��Ԃ͑��݂���)
	}

	// I/O��Ԃ����݂��Ȃ��ꍇ��-1
	return 0xFF;
}

//---------------------------------------------------------------------------
//
//	�o�C�g��������
//
//	windrv�|�[�g�ւ̏������݌�Ad0.w�Ń��U���g��ʒm����B
//
//	���8bit�͎��̒ʂ�B
//		$1x	�G���[(���~�̂�)
//		$2x	�G���[(�Ď��s�̂�)
//		$3x	�G���[(�Ď��s�����~)
//		$4x	�G���[(�����̂�)
//		$5x	�G���[(���������~)
//		$6x	�G���[(�������Ď��s)
//		$7x	�G���[(�������Ď��s�����~)
//	����8bit�͎��̒ʂ�B
//		$01	�����ȃ��j�b�g�ԍ����w�肵��.
//		$02	�f�B�X�N�������Ă��Ȃ�.
//		$03	�f�o�C�X�h���C�o�ɖ����ȃR�}���h���w�肵��.
//		$04	CRC �G���[.
//		$05	�f�B�X�N�̊Ǘ��̈悪�j�󂳂�Ă���.
//		$06	�V�[�N�G���[.
//		$07	�����ȃ��f�B�A.
//		$08	�Z�N�^��������Ȃ�.
//		$09	�v�����^���q�����Ă��Ȃ�.
//		$0a	�������݃G���[.
//		$0b	�ǂݍ��݃G���[.
//		$0c	���̑��̃G���[.
//		$0d	���C�g�v���e�N�g(�v���e�N�g���O���ē����f�B�X�N������).
//		$0e	�������ݕs�\.
//		$0f	�t�@�C�����L�ᔽ.
//
//---------------------------------------------------------------------------
void FASTCALL Windrv::WriteByte(DWORD addr, DWORD data)
{
	ASSERT(this);
	ASSERT((addr >= memdev.first) && (addr <= memdev.last));

	// �T�|�[�g���̂݃|�[�g����
	switch (windrv.enable) {
	case WINDRV_MODE_DUAL:
	case WINDRV_MODE_ENABLE:
		switch (addr) {
		case 0xE9F000:
#ifdef WINDRV_SUPPORT_COMPATIBLE
			if (windrv.enable == WINDRV_MODE_DUAL) {
				Execute();	// �\�Ȃ炱�������ł��c������
			}
#endif // WINDRV_SUPPORT_COMPATIBLE
			return;		// �|�[�g�������ł��o�X�G���[�ɂ��Ȃ�(�d�v)
		case 0xE9F001:
			switch (data) {
			case 0x00: ExecuteAsynchronous(); return;
			case 0xFF: ReleaseAsynchronous(); return;
			}
			return;		// �s���Ȓl�̏������݂ł��o�X�G���[�ɂ��Ȃ�(�g���p)
		}
		break;

	case WINDRV_MODE_COMPATIBLE:
#ifdef WINDRV_SUPPORT_COMPATIBLE
		switch (addr) {
		case 0xE9F000:
			Execute();
			return;
		}
		break;
#endif // WINDRV_SUPPORT_COMPATIBLE

	case WINDRV_MODE_NONE:
		switch (addr) {
		case 0xE9F000:
		case 0xE9F001:
			return;		// �|�[�g�A�h���X�̂ݎ��� (���o�[�W������XM6�Ƃ̌݊����̂���)
		}
		break;
	}

	// �������ݐ悪�Ȃ���΃o�X�G���[
	cpu->BusErr(addr, FALSE);
	return;
}

#ifdef WINDRV_SUPPORT_COMPATIBLE
//---------------------------------------------------------------------------
//
//	�R�}���h�n���h��
//
//---------------------------------------------------------------------------
void FASTCALL Windrv::Execute()
{
	ASSERT(this);

	// ���N�G�X�g�w�b�_�󂯎��
	CPU::cpu_t reg;
	cpu->GetCPU(&reg);
	DWORD a5 = reg.areg[5];
	ASSERT(a5 <= 0xFFFFFF);

	// ���s
	CWindrv* ps = m_cThread.Alloc();
	ASSERT(ps);
	DWORD result = ps->ExecuteCompatible(a5);
	m_cThread.Free(ps);

	// WINDRV�݊�����̂���D0.L�ݒ�
	reg.dreg[0] = result;
	cpu->SetCPU(&reg);

	// �����^�͑S�ẴR�}���h��128us������
	scheduler->Wait(128);	// TODO: ���m�Ȓl���Z�o����
}
#endif // WINDRV_SUPPORT_COMPATIBLE

//---------------------------------------------------------------------------
//
//	�R�}���h���s�J�n �񓯊�
//
//---------------------------------------------------------------------------
void FASTCALL Windrv::ExecuteAsynchronous()
{
	ASSERT(this);

	// ���N�G�X�g�w�b�_�󂯎��
	CPU::cpu_t reg;
	cpu->GetCPU(&reg);
	DWORD a5 = reg.areg[5];
	ASSERT(a5 <= 0xFFFFFF);

	// �󂫃X���b�h��{��
	CWindrv* thread = m_cThread.Alloc();
	if (thread) {
		// �X���b�h���s�J�n
		thread->Execute(a5);
		reg.dreg[0] = thread->GetHandle();
	} else {
		// �X���b�h��������Ȃ����-1
		reg.dreg[0] = (DWORD)-1;
		::Sleep(0);		// �X���b�h���s������u�ڂ�
	}

	// �n���h�����Z�b�g���đ��I��
	cpu->SetCPU(&reg);
}

//---------------------------------------------------------------------------
//
//	�X�e�[�^�X�l�� �񓯊�
//
//---------------------------------------------------------------------------
DWORD FASTCALL Windrv::StatusAsynchronous()
{
	ASSERT(this);

	// �n���h���󂯎��
	CPU::cpu_t reg;
	cpu->GetCPU(&reg);
	DWORD handle = reg.dreg[0];

	// �Y������n���h���̃X�e�[�^�X���l��
	CWindrv* thread = m_cThread.Search(handle);
	if (thread) {
		if (thread->isExecute()) {
			::Sleep(0);		// �X���b�h���s������u�ڂ�
			return 0;		// �܂����s���Ȃ�0
		}

		return 1;			// ���s�����Ȃ�1
	}

	return 0xFF;			// �s���ȃn���h���Ȃ�-1
}

//---------------------------------------------------------------------------
//
//	�n���h���J�� �񓯊�
//
//---------------------------------------------------------------------------
void FASTCALL Windrv::ReleaseAsynchronous()
{
	ASSERT(this);

	// D0.L(�n���h��)�󂯎��
	CPU::cpu_t reg;
	cpu->GetCPU(&reg);
	DWORD handle = reg.dreg[0];

	// �Y������n���h�����J��
	CWindrv* thread = m_cThread.Search(handle);
	if (thread) {
		m_cThread.Free(thread);
	}
}

//---------------------------------------------------------------------------
//
//	�R�}���h���s
//
//---------------------------------------------------------------------------
void FASTCALL CWindrv::ExecuteCommand()
{
	ASSERT(this);
	ASSERT(a5 <= 0xFFFFFF);
	ASSERT(windrv);

	if (command == 0x40) {
		// $40 - ������
		InitDrive();
		return;
	}

	if (windrv->isInvalidUnit(unit)) {
		LockXM();

		// �����ȃ��j�b�g�ԍ����w�肵��
		SetResult(FS_FATAL_INVALIDUNIT);

		UnlockXM();
		return;
	}

	// �R�}���h����
	switch (command & 0x7F) {
		case 0x41: CheckDir(); return;		// $41 - �f�B���N�g���`�F�b�N
		case 0x42: MakeDir(); return;		// $42 - �f�B���N�g���쐬
		case 0x43: RemoveDir(); return;		// $43 - �f�B���N�g���폜
		case 0x44: Rename(); return;		// $44 - �t�@�C�����ύX
		case 0x45: Delete(); return;		// $45 - �t�@�C���폜
		case 0x46: Attribute(); return;		// $46 - �t�@�C�������擾/�ݒ�
		case 0x47: Files(); return;			// $47 - �t�@�C������(First)
		case 0x48: NFiles(); return;		// $48 - �t�@�C������(Next);
		case 0x49: Create(); return;		// $49 - �t�@�C���쐬
		case 0x4A: Open(); return;			// $4A - �t�@�C���I�[�v��
		case 0x4B: Close(); return;			// $4B - �t�@�C���N���[�Y
		case 0x4C: Read(); return;			// $4C - �t�@�C���ǂݍ���
		case 0x4D: Write(); return;			// $4D - �t�@�C����������
		case 0x4E: Seek(); return;			// $4E - �t�@�C���V�[�N
		case 0x4F: TimeStamp(); return;		// $4F - �t�@�C���X�V�����̎擾/�ݒ�
		case 0x50: GetCapacity(); return;	// $50 - �e�ʎ擾
		case 0x51: CtrlDrive(); return;		// $51 - �h���C�u����/��Ԍ���
		case 0x52: GetDPB(); return;		// $52 - DPB�擾
		case 0x53: DiskRead(); return;		// $53 - �Z�N�^�ǂݍ���
		case 0x54: DiskWrite(); return;		// $54 - �Z�N�^��������
		case 0x55: IoControl(); return;		// $55 - IOCTRL
		case 0x56: Flush(); return;			// $56 - �t���b�V��
		case 0x57: CheckMedia(); return;	// $57 - ���f�B�A�����`�F�b�N
		case 0x58: Lock(); return;			// $58 - �r������
	}


	LockXM();

	// �f�o�C�X�h���C�o�ɖ����ȃR�}���h���w�肵��
	SetResult(FS_FATAL_INVALIDCOMMAND);

	UnlockXM();
}

//---------------------------------------------------------------------------
//
//	�t�@�C���V�X�e���ݒ�
//
//---------------------------------------------------------------------------
void FASTCALL Windrv::SetFileSys(FileSys *fs)
{
	ASSERT(this);

	// ���ݑ��݂����ꍇ�A��v���Ȃ���Έ�U����
	if (windrv.fs) {
		if (windrv.fs != fs) {
			// ����
			Reset();
		}
	}

	// NULL�܂��͎���
	windrv.fs = fs;
}

//---------------------------------------------------------------------------
//
//	�t�@�C���V�X�e���ݒ�
//
//---------------------------------------------------------------------------
BOOL FASTCALL Windrv::isInvalidUnit(DWORD unit) const
{
	ASSERT(this);

	if (unit >= windrv.drives) return TRUE;	// ���E���傫����Εs���ȃ��j�b�g�ԍ�

	ASSERT(windrv.fs);
	// �����ɂ��Ȃ�A�ΏۂƂȂ�FileSys�̓��샂�[�h��₢���킹��ׂ������A
	// �A�N�Z�X�s�\�ȃt�@�C���V�X�e���͂��������o�^����Ȃ��̂Ń`�F�b�N�s�v
	return FALSE;
}

//---------------------------------------------------------------------------
//
//	$40 - ������
//
//	in	(offset	size)
//		 0	1.b	�萔(22)
//		 2	1.b	�R�}���h($40/$c0)
//		18	1.l	�p�����[�^�A�h���X
//		22	1.b	�h���C�u�ԍ�
//	out	(offset	size)
//		 3	1.b	�G���[�R�[�h(����)
//		 4	1.b	�V	    (���)
//		13	1.b	���j�b�g��
//		14	1.l	�f�o�C�X�h���C�o�̏I���A�h���X + 1
//
//	�@���[�J���h���C�u�̃R�}���h 0 �Ɠ��l�ɑg�ݍ��ݎ��ɌĂ΂�邪�ABPB �y
//	�т��̃|�C���^�̔z���p�ӂ���K�v�͂Ȃ�.
//
//---------------------------------------------------------------------------
void FASTCALL CWindrv::InitDrive()
{
	ASSERT(this);
	ASSERT(a5 <= 0xFFFFFF);
	ASSERT(windrv);
	ASSERT(windrv->GetFilesystem());

#ifdef WINDRV_LOG
	Log(Log::Normal, "$40 - ������");
#endif // WINDRV_LOG

	LockXM();

	// �x�[�X�h���C�u�����l��
	DWORD base = GetByte(a5 + 22);
	ASSERT(base < 26);
	// DRIVE�R�}���h�ŏ������ς��ƒl�������ɂȂ邽�߁Awindrv.base�͔p�~

	// �I�v�V�������e���l��
	BYTE chOption[256];
	GetParameter(GetAddr(a5 + 18), chOption, sizeof(chOption));

	UnlockXM();

	// Human68k���ŗ��p�\�ȃh���C�u���͈̔͂ŁA�t�@�C���V�X�e�����\�z
	DWORD max_unit = windrv->GetFilesystem()->Init(this, 26 - base, chOption);

	// ���ۂɊm�ۂ��ꂽ���j�b�g����ۑ�
	windrv->SetUnitMax(max_unit);

#ifdef WINDRV_LOG
	Log(Log::Detail, "�x�[�X�h���C�u:%c �T�|�[�g�h���C�u��:%d",
					'A' + base, max_unit);
#endif // WINDRV_LOG

	LockXM();

	// �h���C�u����ԐM
	SetByte(a5 + 13, max_unit);

	// �I���l
	SetResult(0);

	UnlockXM();
}

//---------------------------------------------------------------------------
//
//	$41 - �f�B���N�g���`�F�b�N
//
//	in
//		14	1.L	NAMESTS�\���̃A�h���X
//
//---------------------------------------------------------------------------
void FASTCALL CWindrv::CheckDir()
{
	ASSERT(this);
	ASSERT(a5 <= 0xFFFFFF);
	ASSERT(windrv);
	ASSERT(windrv->isValidUnit(unit));
	ASSERT(windrv->GetFilesystem());

	LockXM();

	// �����Ώۃt�@�C�����l��
	Human68k::namests_t ns;
	GetNameStsPath(GetAddr(a5 + 14), &ns);

	UnlockXM();

#ifdef WINDRV_LOG
	Log(Log::Normal, "$41 %d �f�B���N�g���`�F�b�N", unit);
#endif // WINDRV_LOG

	// �t�@�C���V�X�e���Ăяo��
	DWORD nResult = windrv->GetFilesystem()->CheckDir(this, &ns);

	LockXM();

	// �I���l
	SetResult(nResult);

	UnlockXM();
}

//---------------------------------------------------------------------------
//
//	$42 - �f�B���N�g���쐬
//
//	in
//		14	1.L	NAMESTS�\���̃A�h���X
//
//---------------------------------------------------------------------------
void FASTCALL CWindrv::MakeDir()
{
	ASSERT(this);
	ASSERT(a5 <= 0xFFFFFF);
	ASSERT(windrv);
	ASSERT(windrv->isValidUnit(unit));
	ASSERT(windrv->GetFilesystem());

	LockXM();

	// �����Ώۃt�@�C�����l��
	Human68k::namests_t ns;
	GetNameSts(GetAddr(a5 + 14), &ns);

	UnlockXM();

#ifdef WINDRV_LOG
	Log(Log::Normal, "$42 %d �f�B���N�g���쐬", unit);
#endif // WINDRV_LOG

	// �t�@�C���V�X�e���Ăяo��
	DWORD nResult = windrv->GetFilesystem()->MakeDir(this, &ns);

	LockXM();

	// �I���l
	SetResult(nResult);

	UnlockXM();
}

//---------------------------------------------------------------------------
//
//	$43 - �f�B���N�g���폜
//
//	in
//		14	1.L	NAMESTS�\���̃A�h���X
//
//---------------------------------------------------------------------------
void FASTCALL CWindrv::RemoveDir()
{
	ASSERT(this);
	ASSERT(a5 <= 0xFFFFFF);
	ASSERT(windrv);
	ASSERT(windrv->isValidUnit(unit));
	ASSERT(windrv->GetFilesystem());

	LockXM();

	// �����Ώۃt�@�C�����l��
	Human68k::namests_t ns;
	GetNameSts(GetAddr(a5 + 14), &ns);

	UnlockXM();

#ifdef WINDRV_LOG
	Log(Log::Normal, "$43 %d �f�B���N�g���폜", unit);
#endif // WINDRV_LOG

	// �t�@�C���V�X�e���Ăяo��
	DWORD nResult = windrv->GetFilesystem()->RemoveDir(this, &ns);

	LockXM();

	// �I���l
	SetResult(nResult);

	UnlockXM();
}

//---------------------------------------------------------------------------
//
//	$44 - �t�@�C�����ύX
//
//	in
//		14	1.L	NAMESTS�\���̃A�h���X ���t�@�C����
//		18	1.L	NAMESTS�\���̃A�h���X �V�t�@�C����
//
//---------------------------------------------------------------------------
void FASTCALL CWindrv::Rename()
{
	ASSERT(this);
	ASSERT(a5 <= 0xFFFFFF);
	ASSERT(windrv);
	ASSERT(windrv->isValidUnit(unit));
	ASSERT(windrv->GetFilesystem());

	LockXM();

	// �����Ώۃt�@�C�����l��
	Human68k::namests_t ns;
	GetNameSts(GetAddr(a5 + 14), &ns);
	Human68k::namests_t ns_new;
	GetNameSts(GetAddr(a5 + 18), &ns_new);

	UnlockXM();

#ifdef WINDRV_LOG
	Log(Log::Normal, "$44 %d �t�@�C�����ύX", unit);
#endif // WINDRV_LOG

	// �t�@�C���V�X�e���Ăяo��
	DWORD nResult = windrv->GetFilesystem()->Rename(this, &ns, &ns_new);

	LockXM();

	// �I���l
	SetResult(nResult);

	UnlockXM();
}

//---------------------------------------------------------------------------
//
//	$45 - �t�@�C���폜
//
//	in
//		14	1.L	NAMESTS�\���̃A�h���X
//
//---------------------------------------------------------------------------
void FASTCALL CWindrv::Delete()
{
	ASSERT(this);
	ASSERT(a5 <= 0xFFFFFF);
	ASSERT(windrv);
	ASSERT(windrv->isValidUnit(unit));
	ASSERT(windrv->GetFilesystem());

	LockXM();

	// �����Ώۃt�@�C�����l��
	Human68k::namests_t ns;
	GetNameSts(GetAddr(a5 + 14), &ns);

	UnlockXM();

#ifdef WINDRV_LOG
	Log(Log::Normal, "$45 %d �t�@�C���폜", unit);
#endif // WINDRV_LOG

	// �t�@�C���V�X�e���Ăяo��
	DWORD nResult = windrv->GetFilesystem()->Delete(this, &ns);

	LockXM();

	// �I���l
	SetResult(nResult);

	UnlockXM();
}

//---------------------------------------------------------------------------
//
//	$46 - �t�@�C�������擾/�ݒ�
//
//	in
//		12	1.B	�ǂݏo������0x01�ɂȂ�̂Œ���
//		13	1.B	���� $FF���Ɠǂݏo��
//		14	1.L	NAMESTS�\���̃A�h���X
//
//---------------------------------------------------------------------------
void FASTCALL CWindrv::Attribute()
{
	ASSERT(this);
	ASSERT(a5 <= 0xFFFFFF);
	ASSERT(windrv);
	ASSERT(windrv->isValidUnit(unit));
	ASSERT(windrv->GetFilesystem());

	LockXM();

	// �����Ώۃt�@�C�����l��
	Human68k::namests_t ns;
	GetNameSts(GetAddr(a5 + 14), &ns);

	// �Ώۑ���
	DWORD attr = GetByte(a5 + 13);

	UnlockXM();

#ifdef WINDRV_LOG
	Log(Log::Normal, "$46 %d �t�@�C�������擾/�ݒ� ����:%02X", unit, attr);
#endif // WINDRV_LOG

	// �t�@�C���V�X�e���Ăяo��
	DWORD nResult = windrv->GetFilesystem()->Attribute(this, &ns, attr);

	LockXM();

	// �I���l
	SetResult(nResult);

	UnlockXM();
}

//---------------------------------------------------------------------------
//
//	$47 - �t�@�C������(First)
//
//	in	(offset	size)
//		 0	1.b	�萔(26)
//		 1	1.b	���j�b�g�ԍ�
//		 2	1.b	�R�}���h($47/$c7)
//		13	1.b	��������
//		14	1.l	�t�@�C�����o�b�t�@(namests �`��)
//		18	1.l	�����o�b�t�@(files �`��) ���̃o�b�t�@�Ɍ����r�����ƌ������ʂ���������
//	out	(offset	size)
//		 3	1.b	�G���[�R�[�h(����)
//		 4	1.b	�V	    (���)
//		18	1.l	���U���g�X�e�[�^�X
//
//	�@�f�B���N�g������w��t�@�C������������. DOS _FILES ����Ăяo�����.
//	�@�����Ɏ��s�����ꍇ�A�Ⴕ���͌����ɐ������Ă����C���h�J�[�h���g����
//	���Ȃ��ꍇ�́A���񌟍����ɕK�����s������ׂɌ����o�b�t�@�̃I�t�Z�b�g��
//	-1 ����������. ���������������ꍇ�͌��������t�@�C���̏���ݒ肷��
//	�Ƌ��ɁA�������p�̏��̃Z�N�^�ԍ��A�I�t�Z�b�g�A���[�g�f�B���N�g���̏�
//	���͍X�Ɏc��Z�N�^����ݒ肷��. �����h���C�u�E�����A�p�X���� DOS �R�[
//	���������Őݒ肳���̂ŏ������ޕK�v�͂Ȃ�.
//
//	<NAMESTS�\����>
//	offset	size
//	0	1.b	NAMWLD	0:���C���h�J�[�h�Ȃ� -1:�t�@�C���w��Ȃ�
//				(���C���h�J�[�h�̕�����)
//	1	1.b	NAMDRV	�h���C�u�ԍ�(A=0,B=1,�c,Z=25)
//	2	65.b	NAMPTH	�p�X('\'�{����΃T�u�f�B���N�g�����{'\')
//	67	8.b	NAMNM1	�t�@�C����(�擪 8 ����)
//	75	3.b	NAMEXT	�g���q
//	78	10.b	NAMNM2	�t�@�C����(�c��� 10 ����)
//
// �p�X��؂蕶����0x2F(/)��0x5C(\)�ł͂Ȃ�0x09(TAB)���g���Ă���̂Œ���
//
//---------------------------------------------------------------------------
void FASTCALL CWindrv::Files()
{
	ASSERT(this);
	ASSERT(a5 <= 0xFFFFFF);
	ASSERT(windrv);
	ASSERT(windrv->isValidUnit(unit));
	ASSERT(windrv->GetFilesystem());

	LockXM();

	// �����r���o�ߊi�[�̈�
	DWORD nFiles = GetAddr(a5 + 18);
	Human68k::files_t info;
	GetFiles(nFiles, &info);
	//info.fatr =  (BYTE)GetByte(a5 + 13);	// ���������ۑ�(���ɏ������܂�Ă���)

	// �����Ώۃt�@�C�����l��
	Human68k::namests_t ns;
	GetNameSts(GetAddr(a5 + 14), &ns);

	UnlockXM();

#ifdef WINDRV_LOG
	Log(Log::Normal, "$47 %d �t�@�C������(First) Info:%X ����:%02X", unit, nFiles, info.fatr);
#endif // WINDRV_LOG

	// �t�@�C���V�X�e���Ăяo��
	int nResult = windrv->GetFilesystem()->Files(this, &ns, nFiles, &info);

	LockXM();

	// �������ʂ̔��f
	if (nResult >= 0) {
		SetFiles(nFiles, &info);
	}

	// �I���l
	SetResult(nResult);

	UnlockXM();
}

//---------------------------------------------------------------------------
//
//	$48 - �t�@�C������(Next)
//
//	in
//		18	1.L	FILES�\���̃A�h���X
//
//---------------------------------------------------------------------------
void FASTCALL CWindrv::NFiles()
{
	ASSERT(this);
	ASSERT(a5 <= 0xFFFFFF);
	ASSERT(windrv);
	ASSERT(windrv->GetFilesystem());

	LockXM();

	// ���[�N�̈�̓ǂݍ���
	DWORD nFiles = GetAddr(a5 + 18);
	Human68k::files_t info;
	GetFiles(nFiles, &info);

	UnlockXM();

#ifdef WINDRV_LOG
	Log(Log::Normal, "$48 - �t�@�C������(Next) Info:%X", nFiles);
#endif // WINDRV_LOG

	// �t�@�C���V�X�e���Ăяo��
	int nResult = windrv->GetFilesystem()->NFiles(this, nFiles, &info);

	LockXM();

	// �������ʂ̔��f
	if (nResult >= 0) {
		SetFiles(nFiles, &info);
	}

	// �I���l
	SetResult(nResult);

	UnlockXM();
}

//---------------------------------------------------------------------------
//
//	$49 - �t�@�C���쐬(Create)
//
//	in
//		 1	1.B	���j�b�g�ԍ�
//		13	1.B	����
//		14	1.L	NAMESTS�\���̃A�h���X
//		18	1.L	���[�h (0:_NEWFILE 1:_CREATE)
//		22	1.L	FCB�\���̃A�h���X
//
//---------------------------------------------------------------------------
void FASTCALL CWindrv::Create()
{
	ASSERT(this);
	ASSERT(a5 <= 0xFFFFFF);
	ASSERT(windrv);
	ASSERT(windrv->isValidUnit(unit));
	ASSERT(windrv->GetFilesystem());

	LockXM();

	// �Ώۃt�@�C�����l��
	Human68k::namests_t ns;
	GetNameSts(GetAddr(a5 + 14), &ns);

	// FCB�l��
	DWORD nFcb = GetAddr(a5 + 22);	// FCB�A�h���X�ۑ�
	Human68k::fcb_t fcb;
	GetFcb(nFcb, &fcb);

	// ����
	DWORD attr = GetByte(a5 + 13);

	// �����㏑�����[�h
	BOOL force = (BOOL)GetLong(a5 + 18);

	UnlockXM();

#ifdef WINDRV_LOG
	Log(Log::Normal, "$49 %d �t�@�C���쐬 FCB:%X ����:%02X Mode:%d", unit, nFcb, attr, force);
#endif // WINDRV_LOG

	// �t�@�C���V�X�e���Ăяo��
	int nResult = windrv->GetFilesystem()->Create(this, &ns, nFcb, &fcb, attr, force);

	LockXM();

	// ���ʂ̔��f
	if (nResult >= 0) {
		SetFcb(nFcb, &fcb);
	}

	// �I���l
	SetResult(nResult);

	UnlockXM();
}

//---------------------------------------------------------------------------
//
//	$4A - �t�@�C���I�[�v��
//
//	in
//		 1	1.B	���j�b�g�ԍ�
//		14	1.L	NAMESTS�\���̃A�h���X
//		22	1.L	FCB�\���̃A�h���X
//				����FCB�ɂ͂قƂ�ǂ̃p�����[�^���ݒ�ς�
//				�����E���t�̓I�[�v�������u�Ԃ̂��̂ɂȂ��Ă�̂ŏ㏑��
//				�T�C�Y��0�ɂȂ��Ă���̂ŏ㏑��
//
//---------------------------------------------------------------------------
void FASTCALL CWindrv::Open()
{
	ASSERT(this);
	ASSERT(a5 <= 0xFFFFFF);
	ASSERT(windrv);
	ASSERT(windrv->isValidUnit(unit));
	ASSERT(windrv->GetFilesystem());

	LockXM();

	// �Ώۃt�@�C�����l��
	Human68k::namests_t ns;
	GetNameSts(GetAddr(a5 + 14), &ns);

	// FCB�l��
	DWORD nFcb = GetAddr(a5 + 22);	// FCB�A�h���X�ۑ�
	Human68k::fcb_t fcb;
	GetFcb(nFcb, &fcb);

	UnlockXM();

#ifdef WINDRV_LOG
	Log(Log::Normal, "$4A %d �t�@�C���I�[�v�� FCB:%X Mode:%X", unit, nFcb, fcb.mode);
#endif // WINDRV_LOG

	// �t�@�C���V�X�e���Ăяo��
	int nResult = windrv->GetFilesystem()->Open(this, &ns, nFcb, &fcb);

	LockXM();

	// ���ʂ̔��f
	if (nResult >= 0) {
		SetFcb(nFcb, &fcb);
	}

	// �I���l
	SetResult(nResult);

	UnlockXM();
}

//---------------------------------------------------------------------------
//
//	$4B - �t�@�C���N���[�Y
//
//	in
//		22	1.L	FCB�\���̃A�h���X
//
//---------------------------------------------------------------------------
void FASTCALL CWindrv::Close()
{
	ASSERT(this);
	ASSERT(a5 <= 0xFFFFFF);
	ASSERT(windrv);
	ASSERT(windrv->GetFilesystem());

	LockXM();

	// FCB�l��
	DWORD nFcb = GetAddr(a5 + 22);	// FCB�A�h���X�ۑ�
	Human68k::fcb_t fcb;
	GetFcb(nFcb, &fcb);

	UnlockXM();

#ifdef WINDRV_LOG
	Log(Log::Normal, "$4B - �t�@�C���N���[�Y FCB:%X", nFcb);
#endif // WINDRV_LOG

	// �t�@�C���V�X�e���Ăяo��
	DWORD nResult = windrv->GetFilesystem()->Close(this, nFcb, &fcb);

	LockXM();

	// �I���l
	SetResult(nResult);

	UnlockXM();
}

//---------------------------------------------------------------------------
//
//	$4C - �t�@�C���ǂݍ���
//
//	in
//		14	1.L	�ǂݍ��݃o�b�t�@ �����Ƀt�@�C�����e��ǂݍ���
//		18	1.L	�T�C�Y ���̐��Ȃ�t�@�C���T�C�Y���w�肵���̂Ɠ���
//		22	1.L	FCB�\���̃A�h���X
//
//---------------------------------------------------------------------------
void FASTCALL CWindrv::Read()
{
	ASSERT(this);
	ASSERT(a5 <= 0xFFFFFF);
	ASSERT(windrv);
	ASSERT(windrv->GetFilesystem());

	LockXM();

	// FCB�l��
	DWORD nFcb = GetAddr(a5 + 22);	// FCB�A�h���X�ۑ�
	Human68k::fcb_t fcb;
	GetFcb(nFcb, &fcb);

	// �ǂݍ��݃o�b�t�@
	DWORD nAddress = GetAddr(a5 + 14);

	// �ǂݍ��݃T�C�Y
	DWORD nSize = GetLong(a5 + 18);

	UnlockXM();

#ifdef WINDRV_LOG
	Log(Log::Normal, "$4C - �t�@�C���ǂݍ��� FCB:%X �A�h���X:%X �T�C�Y:%d", nFcb, nAddress, nSize);
#endif // WINDRV_LOG

	// �t�@�C���V�X�e���Ăяo��
	int nResult = windrv->GetFilesystem()->Read(this, nFcb, &fcb, nAddress, nSize);

	LockXM();

	// ���ʂ̔��f
	if (nResult >= 0) {
		SetFcb(nFcb, &fcb);
	}

	// �I���l
	SetResult(nResult);

	UnlockXM();
}

//---------------------------------------------------------------------------
//
//	$4D - �t�@�C����������
//
//	in
//		14	1.L	�������݃o�b�t�@ �����Ƀt�@�C�����e����������
//		18	1.L	�T�C�Y ���̐��Ȃ�t�@�C���T�C�Y���w�肵���̂Ɠ���
//		22	1.L	FCB�\���̃A�h���X
//
//---------------------------------------------------------------------------
void FASTCALL CWindrv::Write()
{
	ASSERT(this);
	ASSERT(a5 <= 0xFFFFFF);
	ASSERT(windrv);
	ASSERT(windrv->GetFilesystem());

	LockXM();

	// FCB�l��
	DWORD nFcb = GetAddr(a5 + 22);	// FCB�A�h���X�ۑ�
	Human68k::fcb_t fcb;
	GetFcb(nFcb, &fcb);

	// �������݃o�b�t�@
	DWORD nAddress = GetAddr(a5 + 14);

	// �������݃T�C�Y
	DWORD nSize = GetLong(a5 + 18);

	UnlockXM();

#ifdef WINDRV_LOG
	Log(Log::Normal, "$4D - �t�@�C���������� FCB:%X �A�h���X:%X �T�C�Y:%d", nFcb, nAddress, nSize);
#endif // WINDRV_LOG

	// �t�@�C���V�X�e���Ăяo��
	int nResult = windrv->GetFilesystem()->Write(this, nFcb, &fcb, nAddress, nSize);

	LockXM();

	// ���ʂ̔��f
	if (nResult >= 0) {
		SetFcb(nFcb, &fcb);
	}

	// �I���l
	SetResult(nResult);

	UnlockXM();
}

//---------------------------------------------------------------------------
//
//	$4E - �t�@�C���V�[�N
//
//	in
//		12	1.B	0x2B �ɂȂ��Ă�Ƃ������� 0�̂Ƃ�������
//		13	1.B	���[�h
//		18	1.L	�I�t�Z�b�g
//		22	1.L	FCB�\���̃A�h���X
//
//---------------------------------------------------------------------------
void FASTCALL CWindrv::Seek()
{
	ASSERT(this);
	ASSERT(a5 <= 0xFFFFFF);
	ASSERT(windrv);
	ASSERT(windrv->GetFilesystem());

	LockXM();

	// FCB�l��
	DWORD nFcb = GetAddr(a5 + 22);	// FCB�A�h���X�ۑ�
	Human68k::fcb_t fcb;
	GetFcb(nFcb, &fcb);

	// �V�[�N���[�h
	DWORD nMode = GetByte(a5 + 13);

	// �V�[�N�I�t�Z�b�g
	DWORD nOffset = GetLong(a5 + 18);

	UnlockXM();

#ifdef WINDRV_LOG
	Log(Log::Normal, "$4E - �t�@�C���V�[�N FCB:%X ���[�h:%d �I�t�Z�b�g:%d", nFcb, nMode, nOffset);
#endif // WINDRV_LOG

	// �t�@�C���V�X�e���Ăяo��
	int nResult = windrv->GetFilesystem()->Seek(this, nFcb, &fcb, nMode, nOffset);

	LockXM();

	// ���ʂ̔��f
	if (nResult >= 0) {
		SetFcb(nFcb, &fcb);
	}

	// �I���l
	SetResult(nResult);

	UnlockXM();
}

//---------------------------------------------------------------------------
//
//	$4F - �t�@�C�������擾/�ݒ�
//
//	in
//		18	1.W	DATE
//		20	1.W	TIME
//		22	1.L	FCB�\���̃A�h���X
//
//---------------------------------------------------------------------------
void FASTCALL CWindrv::TimeStamp()
{
	ASSERT(this);
	ASSERT(a5 <= 0xFFFFFF);
	ASSERT(windrv);
	ASSERT(windrv->GetFilesystem());

	LockXM();

	// FCB�l��
	DWORD nFcb = GetAddr(a5 + 22);	// FCB�A�h���X�ۑ�
	Human68k::fcb_t fcb;
	GetFcb(nFcb, &fcb);

	// �����l��
	WORD nFatDate = (WORD)GetWord(a5 + 18);
	WORD nFatTime = (WORD)GetWord(a5 + 20);

	UnlockXM();

#ifdef WINDRV_LOG
	Log(Log::Normal, "$4F - �t�@�C�������擾/�ݒ� FCB:%X ����:%04X%04X", nFcb, nFatDate, nFatTime);
#endif // WINDRV_LOG

	// �t�@�C���V�X�e���Ăяo��
	DWORD nResult = windrv->GetFilesystem()->TimeStamp(this, nFcb, &fcb, nFatDate, nFatTime);

	LockXM();

	// ���ʂ̔��f
	if ((nResult >> 16) != 0xFFFF) {
		SetFcb(nFcb, &fcb);
	}

	// �I���l
	SetResult(nResult);

	UnlockXM();
}

//---------------------------------------------------------------------------
//
//	$50 - �e�ʎ擾
//
//	in	(offset	size)
//		 0	1.b	�萔(26)
//		 1	1.b	���j�b�g�ԍ�
//		 2	1.b	�R�}���h($50/$d0)
//		14	1.l	�o�b�t�@�A�h���X
//	out	(offset	size)
//		 3	1.b	�G���[�R�[�h(����)
//		 4	1.b	�V	    (���)
//		18	1.l	���U���g�X�e�[�^�X
//
//	�@���f�B�A�̑��e��/�󂫗e�ʁA�N���X�^/�Z�N�^�T�C�Y����������. �o�b�t�@
//	�ɏ������ޓ��e�͈ȉ��̒ʂ�. ���U���g�X�e�[�^�X�Ƃ��Ďg�p�\�ȃo�C�g��
//	��Ԃ�����.
//
//	offset	size
//	0	1.w	�g�p�\�ȃN���X�^��
//	2	1.w	���N���X�^��
//	4	1.w	1 �N���X�^����̃Z�N�^��
//	6	1.w	1 �Z�N�^����̃o�C�g��
//
//---------------------------------------------------------------------------
void FASTCALL CWindrv::GetCapacity()
{
	ASSERT(this);
	ASSERT(a5 <= 0xFFFFFF);
	ASSERT(windrv);
	ASSERT(windrv->isValidUnit(unit));
	ASSERT(windrv->GetFilesystem());

#ifdef WINDRV_LOG
	Log(Log::Normal, "$50 %d �e�ʎ擾", unit);
#endif // WINDRV_LOG

	LockXM();

	// �o�b�t�@�擾
	DWORD nCapacity = GetAddr(a5 + 14);
	Human68k::capacity_t cap;
	//GetCapacity(nCapacity, &cap);	// �擾�͕s�v ���������s�v �o�O�v�� ���������p

	UnlockXM();

	// �t�@�C���V�X�e���Ăяo��
	int nResult = windrv->GetFilesystem()->GetCapacity(this, &cap);

	LockXM();

	// ���ʂ̔��f
	if (nResult >= 0) {
		SetCapacity(nCapacity, &cap);
	}

	// �I���l
	SetResult(nResult);

	UnlockXM();

#ifdef WINDRV_LOG
	Log(Log::Detail, "���j�b�g%d �󂫗e�� %d", unit, nResult);
#endif // WINDRV_LOG
}

//---------------------------------------------------------------------------
//
//	$51 - �h���C�u��Ԍ���/����
//
//	in	(offset	size)
//		 1	1.B	���j�b�g�ԍ�
//		13	1.B	���	0: ��Ԍ��� 1: �C�W�F�N�g
//
//---------------------------------------------------------------------------
void FASTCALL CWindrv::CtrlDrive()
{
	ASSERT(this);
	ASSERT(a5 <= 0xFFFFFF);
	ASSERT(windrv);
	ASSERT(windrv->isValidUnit(unit));
	ASSERT(windrv->GetFilesystem());

	LockXM();

	// �h���C�u��Ԏ擾
	Human68k::ctrldrive_t ctrl;
	ctrl.status = (BYTE)GetByte(a5 + 13);

	UnlockXM();

#ifdef WINDRV_LOG
	Log(Log::Normal, "$51 %d �h���C�u��Ԍ���/���� Mode:%d", unit, ctrl.status);
#endif // WINDRV_LOG

	// �t�@�C���V�X�e���Ăяo��
	int nResult = windrv->GetFilesystem()->CtrlDrive(this, &ctrl);

	LockXM();

	// ���ʂ̔��f
	if (nResult >= 0) {
		SetByte(a5 + 13, ctrl.status);
	}

	// �I���l
	SetResult(nResult);

	UnlockXM();
}

//---------------------------------------------------------------------------
//
//	$52 - DPB�擾
//
//	in	(offset	size)
//		 0	1.b	�萔(26)
//		 1	1.b	���j�b�g�ԍ�
//		 2	1.b	�R�}���h($52/$d2)
//		14	1.l	�o�b�t�@�A�h���X(�擪�A�h���X + 2 ���w��)
//	out	(offset	size)
//		 3	1.b	�G���[�R�[�h(����)
//		 4	1.b	�V	    (���)
//		18	1.l	���U���g�X�e�[�^�X
//
//	�@�w�胁�f�B�A�̏��� v1 �`�� DPB �ŕԂ�. ���̃R�}���h�Őݒ肷��K�v
//	��������͈ȉ��̒ʂ�(���ʓ��� DOS �R�[�����ݒ肷��). �������A�o�b�t
//	�@�A�h���X�̓I�t�Z�b�g 2 ���w�����A�h���X���n�����̂Œ��ӂ��邱��.
//
//	offset	size
//	 0	1.b	(�h���C�u�ԍ�)
//	 1	1.b	(���j�b�g�ԍ�)
//	 2	1.w	1 �Z�N�^����̃o�C�g��
//	 4	1.b	1 �N���X�^����̃Z�N�^�� - 1
//	 5	1.b	�N���X�^���Z�N�^�̃V�t�g��
//			bit 7 = 1 �� MS-DOS �`�� FAT(16bit Intel �z��)
//	 6	1.w	FAT �̐擪�Z�N�^�ԍ�
//	 8	1.b	FAT �̈�̌�
//	 9	1.b	FAT �̐�߂�Z�N�^��(���ʕ�������)
//	10	1.w	���[�g�f�B���N�g���ɓ���t�@�C���̌�
//	12	1.w	�f�[�^�̈�̐擪�Z�N�^�ԍ�
//	14	1.w	���N���X�^�� + 1
//	16	1.w	���[�g�f�B���N�g���̐擪�Z�N�^�ԍ�
//	18	1.l	(�h���C�o�w�b�_�̃A�h���X)
//	22	1.b	(�������̕����h���C�u��)
//	23	1.b	(DPB �g�p�t���O:��� 0)
//	24	1.l	(���� DPB �̃A�h���X)
//	28	1.w	(�J�����g�f�B���N�g���̃N���X�^�ԍ�:��� 0)
//	30	64.b	(�J�����g�f�B���N�g����)
//
//---------------------------------------------------------------------------
void FASTCALL CWindrv::GetDPB()
{
	ASSERT(this);
	ASSERT(a5 <= 0xFFFFFF);
	ASSERT(windrv);
	ASSERT(windrv->isValidUnit(unit));
	ASSERT(windrv->GetFilesystem());

	LockXM();

	// DPB�擾
	DWORD nDpb = GetAddr(a5 + 14);
	Human68k::dpb_t dpb;
	//GetDpb(nDpb, &dpb);	// �擾�͕s�v ���������s�v �o�O�v�� ���������p

	UnlockXM();

#ifdef WINDRV_LOG
	Log(Log::Normal, "$52 %d DPB�擾 DPB:%X", unit, nDpb);
#endif // WINDRV_LOG

	// �t�@�C���V�X�e���Ăяo��
	int nResult = windrv->GetFilesystem()->GetDPB(this, &dpb);

	LockXM();

	// ���ʂ̔��f
	if (nResult >= 0) {
		SetDpb(nDpb, &dpb);
	}

	// �I���l
	SetResult(nResult);

	UnlockXM();
}

//---------------------------------------------------------------------------
//
//	$53 - �Z�N�^�ǂݍ���
//
//	in	(offset	size)
//		 1	1.B	���j�b�g�ԍ�
//		14	1.L	�o�b�t�@�A�h���X
//		18	1.L	�Z�N�^��
//		22	1.L	�Z�N�^�ԍ�
//
//---------------------------------------------------------------------------
void FASTCALL CWindrv::DiskRead()
{
	ASSERT(this);
	ASSERT(a5 <= 0xFFFFFF);
	ASSERT(windrv);
	ASSERT(windrv->isValidUnit(unit));
	ASSERT(windrv->GetFilesystem());

	LockXM();

	DWORD nAddress = GetAddr(a5 + 14);	// �A�h���X(��ʃr�b�g���g���t���O)
	DWORD nSize = GetLong(a5 + 18);		// �Z�N�^��
	DWORD nSector = GetLong(a5 + 22);	// �Z�N�^�ԍ�

	UnlockXM();

#ifdef WINDRV_LOG
	Log(Log::Normal, "$53 %d �Z�N�^�ǂݍ��� �A�h���X:%X �Z�N�^:%X ��:%X", unit, nAddress, nSector, nSize);
#endif // WINDRV_LOG

	// �t�@�C���V�X�e���Ăяo��
	DWORD nResult = windrv->GetFilesystem()->DiskRead(this, nAddress, nSector, nSize);

	LockXM();

	// �I���l
	SetResult(nResult);

	UnlockXM();
}

//---------------------------------------------------------------------------
//
//	$54 - �Z�N�^��������
//
//---------------------------------------------------------------------------
void FASTCALL CWindrv::DiskWrite()
{
	ASSERT(this);
	ASSERT(a5 <= 0xFFFFFF);
	ASSERT(windrv);
	ASSERT(windrv->isValidUnit(unit));
	ASSERT(windrv->GetFilesystem());

	LockXM();

	DWORD nAddress = GetAddr(a5 + 14);	// �A�h���X(��ʃr�b�g���g���t���O)
	DWORD nSize = GetLong(a5 + 18);		// �Z�N�^��
	DWORD nSector = GetLong(a5 + 22);	// �Z�N�^�ԍ�

	UnlockXM();

#ifdef WINDRV_LOG
	Log(Log::Normal, "$53 %d �Z�N�^�������� �A�h���X:%X �Z�N�^:%X ��:%X", unit, nAddress, nSector, nSize);
#endif // WINDRV_LOG

	// �t�@�C���V�X�e���Ăяo��
	DWORD nResult = windrv->GetFilesystem()->DiskWrite(this, nAddress, nSector, nSize);

	LockXM();

	// �I���l
	SetResult(nResult);

	UnlockXM();
}

//---------------------------------------------------------------------------
//
//	$55 - IOCTRL
//
//---------------------------------------------------------------------------
void FASTCALL CWindrv::IoControl()
{
	ASSERT(this);
	ASSERT(a5 <= 0xFFFFFF);
	ASSERT(windrv);
	ASSERT(windrv->isValidUnit(unit));
	ASSERT(windrv->GetFilesystem());

	LockXM();

	// IOCTRL�擾
	DWORD nParameter = GetLong(a5 + 14);	// �p�����[�^
	DWORD nFunction = GetWord(a5 + 18);		// �@�\�ԍ�
	Human68k::ioctrl_t ioctrl;
	GetIoctrl(nParameter, nFunction, &ioctrl);

	UnlockXM();

#ifdef WINDRV_LOG
	Log(Log::Normal, "$55 %d IOCTRL %X %X", unit, nParameter, nFunction);
#endif // WINDRV_LOG

	// �t�@�C���V�X�e���Ăяo��
	int nResult = windrv->GetFilesystem()->IoControl(this, &ioctrl, nFunction);

	LockXM();

	// ���ʂ̔��f
	if (nResult >= 0) {
		SetIoctrl(nParameter, nFunction, &ioctrl);
	}

	// �I���l
	SetResult(nResult);

	UnlockXM();
}

//---------------------------------------------------------------------------
//
//	$56 - �t���b�V��
//
//---------------------------------------------------------------------------
void FASTCALL CWindrv::Flush()
{
	ASSERT(this);
	ASSERT(a5 <= 0xFFFFFF);
	ASSERT(windrv);
	ASSERT(windrv->isValidUnit(unit));
	ASSERT(windrv->GetFilesystem());

#ifdef WINDRV_LOG
	Log(Log::Normal, "$56 %d �t���b�V��", unit);
#endif // WINDRV_LOG

	// �t�@�C���V�X�e���Ăяo��
	DWORD nResult = windrv->GetFilesystem()->Flush(this);

	LockXM();

	// �I���l
	SetResult(nResult);

	UnlockXM();
}

//---------------------------------------------------------------------------
//
//	$57 - ���f�B�A�����`�F�b�N
//
//	in	(offset	size)
//		 0	1.b	�萔(26)
//		 1	1.b	���j�b�g�ԍ�
//		 2	1.b	�R�}���h($57/$d7)
//	out	(offset	size)
//		 3	1.b	�G���[�R�[�h(����)
//		 4	1.b	�V	    (���)
//		18	1.l	���U���g�X�e�[�^�X
//
//	�@���f�B�A���������ꂽ���ۂ��𒲂ׂ�. ��������Ă����ꍇ�̃t�H�[�}�b�g
//	�m�F�͂��̃R�}���h���ōs������.
//
//---------------------------------------------------------------------------
void FASTCALL CWindrv::CheckMedia()
{
	ASSERT(this);
	ASSERT(a5 <= 0xFFFFFF);
	ASSERT(windrv);
	ASSERT(windrv->isValidUnit(unit));
	ASSERT(windrv->GetFilesystem());

#ifdef WINDRV_LOG
	Log(Log::Normal, "$57 %d ���f�B�A�����`�F�b�N", unit);
#endif // WINDRV_LOG

	// �t�@�C���V�X�e���Ăяo��
	DWORD nResult = windrv->GetFilesystem()->CheckMedia(this);

	LockXM();

	// �I���l
	SetResult(nResult);

	UnlockXM();
}

//---------------------------------------------------------------------------
//
//	$58 - �r������
//
//---------------------------------------------------------------------------
void FASTCALL CWindrv::Lock()
{
	ASSERT(this);
	ASSERT(a5 <= 0xFFFFFF);
	ASSERT(windrv);
	ASSERT(windrv->isValidUnit(unit));
	ASSERT(windrv->GetFilesystem());

#ifdef WINDRV_LOG
	Log(Log::Normal, "$58 %d �r������", unit);
#endif // WINDRV_LOG

	// �t�@�C���V�X�e���Ăяo��
	DWORD nResult = windrv->GetFilesystem()->Lock(this);

	LockXM();

	// �I���l
	SetResult(nResult);

	UnlockXM();
}

//---------------------------------------------------------------------------
//
//	�I���l��������
//
//---------------------------------------------------------------------------
void FASTCALL CWindrv::SetResult(DWORD result)
{
	ASSERT(this);
	ASSERT(a5 <= 0xFFFFFF);

	// �v���I�G���[����
	DWORD fatal = 0;
	switch (result) {
	case FS_FATAL_INVALIDUNIT: fatal = 0x5001; break;
	case FS_FATAL_INVALIDCOMMAND: fatal = 0x5003; break;
	case FS_FATAL_WRITEPROTECT: fatal = 0x700D; break;
	case FS_FATAL_MEDIAOFFLINE: fatal = 0x7002; break;
	}

	// ���ʂ��i�[
	if (fatal) {
		// ���g���C�\��Ԃ��Ƃ��́A(a5 + 18)�����������Ă͂����Ȃ��B
		// ���̌㔒�т� Retry ��I�������ꍇ�A�����������l��ǂݍ���Ō듮�삵�Ă��܂��B
		result = FS_INVALIDFUNC;
		SetByte(a5 + 3, fatal & 255);
		SetByte(a5 + 4, fatal >> 8);
		if ((fatal & 0x2000) == 0) {
			SetLong(a5 + 18, result);
		}
	} else {
		SetLong(a5 + 18, result);
	}

#if defined(WINDRV_LOG)
	// �G���[���Ȃ���ΏI��
	if (result < 0xFFFFFF80) return;

	// �G���[���b�Z�[�W
	TCHAR* szError;
	switch (result) {
	case FS_INVALIDFUNC: szError = "�����ȃt�@���N�V�����R�[�h�����s���܂���"; break;
	case FS_FILENOTFND: szError = "�w�肵���t�@�C����������܂���"; break;
	case FS_DIRNOTFND: szError = "�w�肵���f�B���N�g����������܂���"; break;
	case FS_OVEROPENED: szError = "�I�[�v�����Ă���t�@�C�����������܂�"; break;
	case FS_CANTACCESS: szError = "�f�B���N�g����{�����[�����x���̓A�N�Z�X�s�ł�"; break;
	case FS_NOTOPENED: szError = "�w�肵���n���h���̓I�[�v������Ă��܂���"; break;
	case FS_INVALIDMEM: szError = "�������Ǘ��̈悪�j�󂳂�܂���"; break;
	case FS_OUTOFMEM: szError = "���s�ɕK�v�ȃ�����������܂���"; break;
	case FS_INVALIDPTR: szError = "�����ȃ������Ǘ��|�C���^���w�肵�܂���"; break;
	case FS_INVALIDENV: szError = "�s���Ȋ����w�肵�܂���"; break;
	case FS_ILLEGALFMT: szError = "���s�t�@�C���̃t�H�[�}�b�g���ُ�ł�"; break;
	case FS_ILLEGALMOD: szError = "�I�[�v���̃A�N�Z�X���[�h���ُ�ł�"; break;
	case FS_INVALIDPATH: szError = "�t�@�C�����̎w��Ɍ�肪����܂�"; break;
	case FS_INVALIDPRM: szError = "�����ȃp�����[�^�ŃR�[�����܂���"; break;
	case FS_INVALIDDRV: szError = "�h���C�u�w��Ɍ�肪����܂�"; break;
	case FS_DELCURDIR: szError = "�J�����g�f�B���N�g���͍폜�ł��܂���"; break;
	case FS_NOTIOCTRL: szError = "IOCTRL�ł��Ȃ��f�o�C�X�ł�"; break;
	case FS_LASTFILE: szError = "����ȏ�t�@�C����������܂���"; break;
	case FS_CANTWRITE: szError = "�w��̃t�@�C���͏������݂ł��܂���"; break;
	case FS_DIRALREADY: szError = "�w��̃f�B���N�g���͊��ɓo�^����Ă��܂�"; break;
	case FS_CANTDELETE: szError = "�t�@�C��������̂ō폜�ł��܂���"; break;
	case FS_CANTRENAME: szError = "�t�@�C��������̂Ń��l�[���ł��܂���"; break;
	case FS_DISKFULL: szError = "�f�B�X�N����t�Ńt�@�C�������܂���"; break;
	case FS_DIRFULL: szError = "�f�B���N�g������t�Ńt�@�C�������܂���"; break;
	case FS_CANTSEEK: szError = "�w��̈ʒu�ɂ̓V�[�N�ł��܂���"; break;
	case FS_SUPERVISOR: szError = "�X�[�p�[�o�C�U��ԂŃX�[�p�o�C�U�w�肵�܂���"; break;
	case FS_THREADNAME: szError = "�����X���b�h�������݂��܂�"; break;
	case FS_BUFWRITE: szError = "�v���Z�X�ԒʐM�̃o�b�t�@�������݋֎~�ł�"; break;
	case FS_BACKGROUND: szError = "�o�b�N�O���E���h�v���Z�X���N���ł��܂���"; break;
	case FS_OUTOFLOCK: szError = "���b�N�̈悪����܂���"; break;
	case FS_LOCKED: szError = "���b�N����Ă��ăA�N�Z�X�ł��܂���"; break;
	case FS_DRIVEOPENED: szError = "�w��̃h���C�u�̓n���h�����I�[�v������Ă��܂�"; break;
	case FS_LINKOVER: szError = "�V���{���b�N�����N�l�X�g��16��𒴂��܂���"; break;
	case FS_FILEEXIST: szError = "�t�@�C�������݂��܂�"; break;
	default: szError = "����`�̃G���[�ł�"; break;
	}
	switch (fatal & 255) {
	case 0: break;
	case 1: szError = "�����ȃ��j�b�g�ԍ����w�肵�܂���"; break;
	case 2: szError = "�f�B�X�N�������Ă��܂���"; break;
	case 3: szError = "�����ȃR�}���h�ԍ����w�肵�܂���"; break;
	case 4: szError = "CRC�G���[���������܂���"; break;
	case 5: szError = "�f�B�X�N�̊Ǘ��̈悪�j�󂳂�Ă��܂�"; break;
	case 6: szError = "�V�[�N�Ɏ��s���܂���"; break;
	case 7: szError = "�����ȃ��f�B�A���g�p���܂���"; break;
	case 8: szError = "�Z�N�^��������܂���"; break;
	case 9: szError = "�v�����^���Ȃ����Ă��܂���"; break;
	case 10: szError = "�������݃G���[���������܂���"; break;
	case 11: szError = "�ǂݍ��݃G���[���������܂���"; break;
	case 12: szError = "���̑��̃G���[���������܂���"; break;
	case 13: szError = "�������݋֎~�̂��ߎ��s�ł��܂���"; break;
	case 14: szError = "�������݂ł��܂���"; break;
	case 15: szError = "�t�@�C�����L�ᔽ���������܂���"; break;
	default: szError = "����`�̒v���I�G���[�ł�"; break;
	}

	Log(Log::Warning, "$%X %d %s(%d)", command, unit, szError, result);
#endif // WINDRV_LOG
}

//---------------------------------------------------------------------------
//
//	�o�C�g�ǂݍ���
//
//---------------------------------------------------------------------------
DWORD FASTCALL CWindrv::GetByte(DWORD addr) const
{
	ASSERT(this);
	ASSERT(memory);
	ASSERT(addr <= 0xffffff);

	// 8bit
	return memory->ReadOnly(addr);
}

//---------------------------------------------------------------------------
//
//	�o�C�g��������
//
//---------------------------------------------------------------------------
void FASTCALL CWindrv::SetByte(DWORD addr, DWORD data)
{
	ASSERT(this);
	ASSERT(memory);
	ASSERT(addr <= 0xffffff);
	ASSERT(data < 0x100);

	// 8bit
	memory->WriteByte(addr, data);
}

//---------------------------------------------------------------------------
//
//	���[�h�ǂݍ���
//
//---------------------------------------------------------------------------
DWORD FASTCALL CWindrv::GetWord(DWORD addr) const
{
	DWORD data;

	ASSERT(this);
	ASSERT(memory);
	ASSERT(addr <= 0xffffff);

	// 16bit
	data = memory->ReadOnly(addr);
	data <<= 8;
	data |= memory->ReadOnly(addr + 1);

	return data;
}

//---------------------------------------------------------------------------
//
//	���[�h��������
//
//---------------------------------------------------------------------------
void FASTCALL CWindrv::SetWord(DWORD addr, DWORD data)
{
	ASSERT(this);
	ASSERT(memory);
	ASSERT(addr <= 0xffffff);
	ASSERT(data < 0x10000);

	// 16bit
	memory->WriteWord(addr, data);
}

//---------------------------------------------------------------------------
//
//	�����O�ǂݍ���
//
//---------------------------------------------------------------------------
DWORD FASTCALL CWindrv::GetLong(DWORD addr) const
{
	DWORD data;

	ASSERT(this);
	ASSERT(memory);
	ASSERT(addr <= 0xffffff);

	// 32bit
	data = memory->ReadOnly(addr);
	data <<= 8;
	data |= memory->ReadOnly(addr + 1);
	data <<= 8;
	data |= memory->ReadOnly(addr + 2);
	data <<= 8;
	data |= memory->ReadOnly(addr + 3);

	return data;
}

//---------------------------------------------------------------------------
//
//	�����O��������
//
//---------------------------------------------------------------------------
void FASTCALL CWindrv::SetLong(DWORD addr, DWORD data)
{
	ASSERT(this);
	ASSERT(memory);
	ASSERT(addr <= 0xffffff);

	// 32bit
	memory->WriteWord(addr, (WORD)(data >> 16));
	memory->WriteWord(addr + 2, (WORD)data);
}

//---------------------------------------------------------------------------
//
//	�A�h���X�ǂݍ���
//
//---------------------------------------------------------------------------
DWORD FASTCALL CWindrv::GetAddr(DWORD addr) const
{
	DWORD data;

	ASSERT(this);
	ASSERT(memory);
	ASSERT(addr <= 0xffffff);

	// 24bit(�ŏ���1�o�C�g�͖���)
	data = memory->ReadOnly(addr + 1);
	data <<= 8;
	data |= memory->ReadOnly(addr + 2);
	data <<= 8;
	data |= memory->ReadOnly(addr + 3);

	return data;
}

//---------------------------------------------------------------------------
//
//	�A�h���X��������
//
//---------------------------------------------------------------------------
void FASTCALL CWindrv::SetAddr(DWORD addr, DWORD data)
{
	ASSERT(this);
	ASSERT(memory);
	ASSERT(addr <= 0xffffff);
	ASSERT(data <= 0xffffff);

	// 24bit(�ŏ���1�o�C�g�͕K��0)
	data &= 0xffffff;
	memory->WriteWord(addr, (WORD)(data >> 16));
	memory->WriteWord(addr + 2, (WORD)data);
}

//---------------------------------------------------------------------------
//
//	NAMESTS�p�X���ǂݍ���
//
//---------------------------------------------------------------------------
void FASTCALL CWindrv::GetNameStsPath(DWORD addr, Human68k::namests_t *pNamests) const
{
	DWORD offset;

	ASSERT(this);
	ASSERT(pNamests);
	ASSERT(addr <= 0xFFFFFF);

	// ���C���h�J�[�h���
	pNamests->wildcard = (BYTE)GetByte(addr + 0);

	// �h���C�u�ԍ�
	pNamests->drive = (BYTE)GetByte(addr + 1);

	// �p�X��
	for (offset=0; offset<sizeof(pNamests->path); offset++) {
		pNamests->path[offset] = (BYTE)GetByte(addr + 2 + offset);
	}

	// �t�@�C����1
	memset(pNamests->name, 0x20, sizeof(pNamests->name));

	// �g���q
	memset(pNamests->ext, 0x20, sizeof(pNamests->ext));

	// �t�@�C����2
	memset(pNamests->add, 0, sizeof(pNamests->add));

#if defined(WINDRV_LOG)
{
	TCHAR szPath[_MAX_PATH];
	pNamests->GetCopyPath((BYTE*)szPath);	// WARNING: Unicode���v�C��

	Log(Log::Detail, "Path %s", szPath);
}
#endif // WINDRV_LOG
}

//---------------------------------------------------------------------------
//
//	NAMESTS�ǂݍ���
//
//---------------------------------------------------------------------------
void FASTCALL CWindrv::GetNameSts(DWORD addr, Human68k::namests_t *pNamests) const
{
	DWORD offset;

	ASSERT(this);
	ASSERT(pNamests);
	ASSERT(addr <= 0xFFFFFF);

	// ���C���h�J�[�h���
	pNamests->wildcard = (BYTE)GetByte(addr + 0);

	// �h���C�u�ԍ�
	pNamests->drive = (BYTE)GetByte(addr + 1);

	// �p�X��
	for (offset=0; offset<sizeof(pNamests->path); offset++) {
		pNamests->path[offset] = (BYTE)GetByte(addr + 2 + offset);
	}

	// �t�@�C����1
	for (offset=0; offset<sizeof(pNamests->name); offset++) {
		pNamests->name[offset] = (BYTE)GetByte(addr + 67 + offset);
	}

	// �g���q
	for (offset=0; offset<sizeof(pNamests->ext); offset++) {
		pNamests->ext[offset] = (BYTE)GetByte(addr + 75 + offset);
	}

	// �t�@�C����2
	for (offset=0; offset<sizeof(pNamests->add); offset++) {
		pNamests->add[offset] = (BYTE)GetByte(addr + 78 + offset);
	}

#if defined(WINDRV_LOG)
{
	TCHAR szPath[_MAX_PATH];
	pNamests->GetCopyPath((BYTE*)szPath);	// WARNING: Unicode���v�C��

	TCHAR szFilename[_MAX_PATH];
	pNamests->GetCopyFilename((BYTE*)szFilename);	// WARNING: Unicode���v�C��

	if (pNamests->wildcard == 0xFF) {
		Log(Log::Detail, "Path %s", szPath);
	} else {
		Log(Log::Detail, "Filename %s%s", szPath, szFilename);
	}
}
#endif // WINDRV_LOG
}

//---------------------------------------------------------------------------
//
//	FILES�ǂݍ���
//
//---------------------------------------------------------------------------
void FASTCALL CWindrv::GetFiles(DWORD addr, Human68k::files_t* pFiles) const
{
	ASSERT(this);
	ASSERT(pFiles);
	ASSERT(addr <= 0xFFFFFF);

	// �������
	pFiles->fatr = (BYTE)GetByte(addr + 0);		// Read only
	pFiles->drive = (BYTE)GetByte(addr + 1);	// Read only
	pFiles->sector = GetLong(addr + 2);
	//pFiles->sector2 = (WORD)GetWord(addr + 6);	// Read only
	pFiles->offset = (WORD)GetWord(addr + 8);

#if 0
	DWORD offset;

	// �t�@�C����
	for (offset=0; offset<sizeof(pFiles->name); offset++) {
		pFiles->name[offset] = (BYTE)GetByte(addr + 10 + offset);
	}

	// �g���q
	for (offset=0; offset<sizeof(pFiles->ext); offset++) {
		pFiles->ext[offset] = (BYTE)GetByte(addr + 18 + offset);
	}
#endif

#if 0
	// �t�@�C�����
	pFiles->attr = (BYTE)GetByte(addr + 21);
	pFiles->time = (WORD)GetWord(addr + 22);
	pFiles->date = (WORD)GetWord(addr + 24);
	pFiles->size = GetLong(addr + 26);

	// �t���t�@�C����
	for (offset=0; offset<sizeof(pFiles->full); offset++) {
		pFiles->full[offset] = (BYTE)GetByte(addr + 30 + offset);
	}
#else
	pFiles->attr = 0;
	pFiles->time = 0;
	pFiles->date = 0;
	pFiles->size = 0;
	memset(pFiles->full, 0, sizeof(pFiles->full));
#endif
}

//---------------------------------------------------------------------------
//
//	FILES��������
//
//---------------------------------------------------------------------------
void FASTCALL CWindrv::SetFiles(DWORD addr, const Human68k::files_t* pFiles)
{
	DWORD offset;

	ASSERT(this);
	ASSERT(pFiles);
	ASSERT(addr <= 0xFFFFFF);

	SetLong(addr + 2, pFiles->sector);
	SetWord(addr + 8, pFiles->offset);

#if 0
	// �������
	SetByte(addr + 0, pFiles->fatr);	// Read only
	SetByte(addr + 1, pFiles->drive);	// Read only
	SetWord(addr + 6, pFiles->sector2);	// Read only

	// �t�@�C����
	for (offset=0; offset<sizeof(pFiles->name); offset++) {
		SetByte(addr + 10 + offset, pFiles->name[offset]);
	}

	// �g���q
	for (offset=0; offset<sizeof(pFiles->ext); offset++) {
		SetByte(addr + 18 + offset, pFiles->ext[offset]);
	}
#endif

	// �t�@�C�����
	SetByte(addr + 21, pFiles->attr);
	SetWord(addr + 22, pFiles->time);
	SetWord(addr + 24, pFiles->date);
	SetLong(addr + 26, pFiles->size);

	// �t���t�@�C����
	for (offset=0; offset<sizeof(pFiles->full); offset++) {
		SetByte(addr + 30 + offset, pFiles->full[offset]);
	}

#if defined(WINDRV_LOG)
{
	TCHAR szAttribute[16];
	BYTE nAttribute = pFiles->attr;
	for (int i = 0; i < 8; i++) {
		TCHAR c = '-';
		if ((nAttribute & 0x80) != 0)
			c = "XLADVSHR"[i];
		szAttribute[i] = c;
		nAttribute <<= 1;
	}
	szAttribute[8] = '\0';

	Log(Log::Detail, "%s %s %d", szAttribute, pFiles->full, pFiles->size);
}
#endif // WINDRV_LOG
}

//---------------------------------------------------------------------------
//
//	FCB�ǂݍ���
//
//---------------------------------------------------------------------------
void FASTCALL CWindrv::GetFcb(DWORD addr, Human68k::fcb_t* pFcb) const
{
	ASSERT(this);
	ASSERT(pFcb);
	ASSERT(addr <= 0xFFFFFF);

	// FCB���
	pFcb->fileptr = GetLong(addr + 6);
	pFcb->mode = (WORD)GetWord(addr + 14);
	//pFcb->zero = GetLong(addr + 32);

#if 0
	DWORD i;

	// �t�@�C����
	for (i = 0; i < sizeof(pFcb->name); i++) {
		pFcb->name[i] = (BYTE)GetByte(addr + 36 + i);
	}

	// �g���q
	for (i = 0; i < sizeof(pFcb->ext); i++) {
		pFcb->ext[i] = (BYTE)GetByte(addr + 44 + i);
	}

	// �t�@�C����2
	for (i = 0; i < sizeof(pFcb->add); i++) {
		pFcb->add[i] = (BYTE)GetByte(addr + 48 + i);
	}
#endif

	// ����
	pFcb->attr = (BYTE)GetByte(addr + 47);

	// FCB���
	pFcb->time = (WORD)GetWord(addr + 58);
	pFcb->date = (WORD)GetWord(addr + 60);
	//pFcb->cluster = (WORD)GetWord(addr + 62);
	pFcb->size = GetLong(addr + 64);
}

//---------------------------------------------------------------------------
//
//	FCB��������
//
//---------------------------------------------------------------------------
void FASTCALL CWindrv::SetFcb(DWORD addr, const Human68k::fcb_t* pFcb)
{
	ASSERT(this);
	ASSERT(pFcb);
	ASSERT(addr <= 0xFFFFFF);

	// FCB���
	SetLong(addr + 6, pFcb->fileptr);
	SetWord(addr + 14, pFcb->mode);
	// SetLong(addr + 32, pFcb->zero);

#if 0
	DWORD i;

	// �t�@�C����
	for (i = 0; i < sizeof(pFcb->name); i++) {
		SetByte(addr + 36 + i, pFcb->name[i]);
	}

	// �g���q
	for (i = 0; i < sizeof(pFcb->ext); i++) {
		SetByte(addr + 44 + i, pFcb->ext[i]);
	}

	// �t�@�C����2
	for (i = 0; i < sizeof(pFcb->add); i++) {
		SetByte(addr + 48 + i, pFcb->add[i]);
	}
#endif

	// ����
	SetByte(addr + 47, pFcb->attr);

	// FCB���
	SetWord(addr + 58, pFcb->time);
	SetWord(addr + 60, pFcb->date);
	//SetWord(addr + 62, pFcb->cluster);
	SetLong(addr + 64, pFcb->size);
}

//---------------------------------------------------------------------------
//
//	CAPACITY��������
//
//---------------------------------------------------------------------------
void FASTCALL CWindrv::SetCapacity(DWORD addr, const Human68k::capacity_t* pCapacity)
{
	ASSERT(this);
	ASSERT(pCapacity);
	ASSERT(addr <= 0xffffff);

#ifdef WINDRV_LOG
	Log(Log::Detail, "Free:%d Clusters:%d Sectors:%d Bytes:%d",
		pCapacity->free, pCapacity->clusters, pCapacity->sectors, pCapacity->bytes);
#endif // WINDRV_LOG

	SetWord(addr + 0, pCapacity->free);
	SetWord(addr + 2, pCapacity->clusters);
	SetWord(addr + 4, pCapacity->sectors);
	SetWord(addr + 6, pCapacity->bytes);
}

//---------------------------------------------------------------------------
//
//	DPB��������
//
//---------------------------------------------------------------------------
void FASTCALL CWindrv::SetDpb(DWORD addr, const Human68k::dpb_t* pDpb)
{
	ASSERT(this);
	ASSERT(pDpb);
	ASSERT(addr <= 0xFFFFFF);

	// DPB���
	SetWord(addr + 0, pDpb->sector_size);
	SetByte(addr + 2, pDpb->cluster_size);
	SetByte(addr + 3, pDpb->shift);
	SetWord(addr + 4, pDpb->fat_sector);
	SetByte(addr + 6, pDpb->fat_max);
	SetByte(addr + 7, pDpb->fat_size);
	SetWord(addr + 8, pDpb->file_max);
	SetWord(addr + 10, pDpb->data_sector);
	SetWord(addr + 12, pDpb->cluster_max);
	SetWord(addr + 14, pDpb->root_sector);
	SetByte(addr + 20, pDpb->media);
}

//---------------------------------------------------------------------------
//
//	IOCTRL�ǂݍ���
//
//---------------------------------------------------------------------------
void FASTCALL CWindrv::GetIoctrl(DWORD param, DWORD func, Human68k::ioctrl_t* pIoctrl)
{
	ASSERT(this);
	ASSERT(pIoctrl);

	switch (func) {
	case 2:
		// ���f�B�A�ĔF��
		pIoctrl->param = param;
		return;

	case -2:
		// �I�v�V�����ݒ�
		ASSERT(param <= 0xFFFFFF);
		pIoctrl->param = GetLong(param);
		return;

#if 0
	case 0:
		// ���f�B�AID�̊l��
		return;

	case 1:
		// Human68k�݊��̂��߂̃_�~�[
		return;

	case -1:
		// �풓����
		return;

	case -3:
		// �I�v�V�����l��
		return;
#endif
	}
}

//---------------------------------------------------------------------------
//
//	IOCTRL��������
//
//---------------------------------------------------------------------------
void FASTCALL CWindrv::SetIoctrl(DWORD param, DWORD func, const Human68k::ioctrl_t* pIoctrl)
{
	ASSERT(this);
	ASSERT(pIoctrl);

	switch (func) {
	case 0:
		// ���f�B�AID�̊l��
		ASSERT(param <= 0xFFFFFF);
		SetWord(param, pIoctrl->media);
		return;

	case 1:
		// Human68k�݊��̂��߂̃_�~�[
		ASSERT(param <= 0xFFFFFF);
		SetLong(param, pIoctrl->param);
		return;

	case -1:
		// �풓����
		ASSERT(param <= 0xFFFFFF);
		{
			for (int i = 0; i < 8; i++) SetByte(param + i, pIoctrl->buffer[i]);
		}
		return;

	case -3:
		// �I�v�V�����l��
		ASSERT(param <= 0xFFFFFF);
		SetLong(param, pIoctrl->param);
		return;

#if 0
	case 2:
		// ���f�B�A�ĔF��
		return;

	case -2:
		// �I�v�V�����ݒ�
		return;
#endif
	}
}

//---------------------------------------------------------------------------
//
//	�p�����[�^�ǂݍ���
//
//---------------------------------------------------------------------------
void FASTCALL CWindrv::GetParameter(DWORD addr, BYTE* pOption, DWORD nSize)
{
	ASSERT(this);
	ASSERT(addr <= 0xFFFFFF);
	ASSERT(pOption);

	DWORD nMode = 0;
	BYTE* p = pOption;
	for (DWORD i = 0; i < nSize - 2; i++) {
		BYTE c = (BYTE)GetByte(addr + i);
		*p++ = c;
		switch (nMode) {
		case 0:
			if (c == '\0') {
				return;
			}
			nMode = 1;
			break;
		case 1:
			if (c == '\0') nMode = 0;
			break;
		}
	}

	*p++ = '\0';
	*p++ = '\0';
}

#ifdef WINDRV_LOG
//---------------------------------------------------------------------------
//
//	���O�o��
//
//---------------------------------------------------------------------------
void FASTCALL CWindrv::Log(DWORD level, char* format, ...) const
{
	ASSERT(this);
	ASSERT(format);
	ASSERT(windrv);

	va_list args;
	va_start(args, format);
	char message[512];
	vsprintf(message, format, args);
	va_end(args);

	windrv->Log(level, message);
}

//---------------------------------------------------------------------------
//
//	���O�o��
//
//---------------------------------------------------------------------------
void FASTCALL Windrv::Log(DWORD level, char* message) const
{
	ASSERT(this);
	ASSERT(message);

	LOG((enum Log::loglevel)level, "%s", message);
}
#endif // WINDRV_LOG
