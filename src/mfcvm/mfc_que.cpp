//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2004 �o�h�D(ytanaka@ipc-tokai.or.jp)
//	[ MFC �L���[ ]
//
//---------------------------------------------------------------------------

#if defined(_WIN32)

#include "os.h"
#include "xm6.h"
#include "mfc_que.h"

//===========================================================================
//
//	�L���[
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	�R���X�g���N�^
//
//---------------------------------------------------------------------------
CQueue::CQueue()
{
	// �|�C���^�Ȃ�
	m_Queue.pBuf = NULL;

	// �T�C�Y0
	m_Queue.dwSize = 0;
	m_Queue.dwMask = 0;

	// ���̑�
	Clear();
}

//---------------------------------------------------------------------------
//
//	�f�X�g���N�^
//
//---------------------------------------------------------------------------
CQueue::~CQueue()
{
	// ���������
	if (m_Queue.pBuf) {
		delete[] m_Queue.pBuf;
		m_Queue.pBuf = NULL;
	}
}

//---------------------------------------------------------------------------
//
//	������
//
//---------------------------------------------------------------------------
BOOL FASTCALL CQueue::Init(DWORD dwSize)
{
	ASSERT(this);
	ASSERT(!m_Queue.pBuf);
	ASSERT(m_Queue.dwSize == 0);
	ASSERT(dwSize > 0);

	// �������m��
	try {
		m_Queue.pBuf = new BYTE[dwSize];
	}
	catch (...) {
		m_Queue.pBuf = NULL;
		return FALSE;
	}
	if (!m_Queue.pBuf) {
		return FALSE;
	}

	// �T�C�Y�ݒ�
	m_Queue.dwSize = dwSize;
	m_Queue.dwMask = m_Queue.dwSize - 1;

	// �N���A
	Clear();

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	�N���A
//	��������Ή�
//
//---------------------------------------------------------------------------
void FASTCALL CQueue::Clear()
{
	ASSERT(this);

	m_Queue.dwRead = 0;
	m_Queue.dwWrite = 0;
	m_Queue.dwNum = 0;
	m_Queue.dwTotalRead = 0;
	m_Queue.dwTotalWrite = 0;
}

//---------------------------------------------------------------------------
//
//	�L���[���̃f�[�^�����ׂĎ擾
//	��������Ή�
//
//---------------------------------------------------------------------------
DWORD FASTCALL CQueue::Get(BYTE *pDest)
{
	DWORD dwLength;
	DWORD dwRest;

	ASSERT(this);
	ASSERT(m_Queue.dwSize > 0);
	ASSERT(m_Queue.pBuf);
	ASSERT((m_Queue.dwMask + 1) == m_Queue.dwSize);
	ASSERT(m_Queue.dwRead < m_Queue.dwSize);
	ASSERT(m_Queue.dwWrite < m_Queue.dwSize);
	ASSERT(m_Queue.dwNum <= m_Queue.dwSize);

	ASSERT(pDest);

	// �f�[�^���Ȃ����0
	if (m_Queue.dwNum == 0) {
		return 0;
	}

	// ���ݕ��A�I�[�o�[���b�v���̌������߂�
	dwLength = m_Queue.dwNum;
	dwRest = 0;
	if ((dwLength + m_Queue.dwRead) > m_Queue.dwSize) {
		dwRest = (dwLength + m_Queue.dwRead) - m_Queue.dwSize;
		dwLength -= dwRest;
	}

	// �J�����g
	memcpy(pDest, &m_Queue.pBuf[m_Queue.dwRead], dwLength);
	m_Queue.dwRead += dwLength;
	m_Queue.dwRead &= m_Queue.dwMask;
	m_Queue.dwNum -= dwLength;

	// �I�[�o�[���b�v
	memcpy(&pDest[dwLength], m_Queue.pBuf, dwRest);
	m_Queue.dwRead += dwRest;
	m_Queue.dwRead &= m_Queue.dwMask;
	m_Queue.dwNum -= dwRest;

	// �g�[�^��Read�𑝂₷
	m_Queue.dwTotalRead += dwLength;
	m_Queue.dwTotalRead += dwRest;

	// �R�s�[�����T�C�Y��Ԃ�
	return (DWORD)(dwLength + dwRest);
}

//---------------------------------------------------------------------------
//
//	�L���[���̃f�[�^�����ׂĎ擾(�L���[�i�߂Ȃ�)
//	��������Ή�
//
//---------------------------------------------------------------------------
DWORD FASTCALL CQueue::Copy(BYTE *pDest) const
{
	DWORD dwLength;
	DWORD dwRest;

	ASSERT(this);
	ASSERT(m_Queue.dwSize > 0);
	ASSERT(m_Queue.pBuf);
	ASSERT((m_Queue.dwMask + 1) == m_Queue.dwSize);
	ASSERT(m_Queue.dwRead < m_Queue.dwSize);
	ASSERT(m_Queue.dwWrite < m_Queue.dwSize);
	ASSERT(m_Queue.dwNum <= m_Queue.dwSize);

	ASSERT(pDest);

	// �f�[�^���Ȃ����0
	if (m_Queue.dwNum == 0) {
		return 0;
	}

	// ���ݕ��A�I�[�o�[���b�v���̌������߂�
	dwLength = m_Queue.dwNum;
	dwRest = 0;
	if ((dwLength + m_Queue.dwRead) > m_Queue.dwSize) {
		dwRest = (dwLength + m_Queue.dwRead) - m_Queue.dwSize;
		dwLength -= dwRest;
	}

	// �J�����g
	memcpy(pDest, &m_Queue.pBuf[m_Queue.dwRead], dwLength);

	// �I�[�o�[���b�v
	memcpy(&pDest[dwLength], m_Queue.pBuf, dwRest);

	// �R�s�[�����T�C�Y��Ԃ�
	return (DWORD)(dwLength + dwRest);
}

//---------------------------------------------------------------------------
//
//	�L���[��i�߂�
//	��������Ή��BCopy->Diskcard�Ԃ�Get���Ȃ�����
//
//---------------------------------------------------------------------------
void FASTCALL CQueue::Discard(DWORD dwNum)
{
	ASSERT(this);
	ASSERT(m_Queue.dwSize > 0);
	ASSERT(m_Queue.pBuf);
	ASSERT((m_Queue.dwMask + 1) == m_Queue.dwSize);
	ASSERT(m_Queue.dwRead < m_Queue.dwSize);
	ASSERT(m_Queue.dwWrite < m_Queue.dwSize);
	ASSERT(m_Queue.dwNum <= m_Queue.dwSize);

	ASSERT(dwNum <= m_Queue.dwSize);
	ASSERT(dwNum <= m_Queue.dwNum);

	// �i�߂�
	m_Queue.dwRead += dwNum;
	m_Queue.dwRead &= m_Queue.dwMask;

	// ��
	m_Queue.dwNum -= dwNum;

	// �g�[�^��
	m_Queue.dwTotalRead += dwNum;
}

//---------------------------------------------------------------------------
//
//	�L���[��߂�
//	��������Ή��BGet->Back�Ԃ�Insert���Ȃ�����
//
//---------------------------------------------------------------------------
void FASTCALL CQueue::Back(DWORD dwNum)
{
	DWORD dwRest;

	ASSERT(this);
	ASSERT(m_Queue.dwSize > 0);
	ASSERT(m_Queue.pBuf);
	ASSERT((m_Queue.dwMask + 1) == m_Queue.dwSize);
	ASSERT(m_Queue.dwRead < m_Queue.dwSize);
	ASSERT(m_Queue.dwWrite < m_Queue.dwSize);
	ASSERT(m_Queue.dwNum <= m_Queue.dwSize);

	ASSERT(dwNum <= m_Queue.dwSize);

	// dwRest�����Z�o
	dwRest = 0;
	if (m_Queue.dwRead < dwNum) {
		dwRest = dwNum - m_Queue.dwRead;
		dwNum = m_Queue.dwRead;
	}

	// �J�����g
	m_Queue.dwRead -= dwNum;
	m_Queue.dwRead &= m_Queue.dwMask;
	m_Queue.dwNum += dwNum;

	// �I�[�o�[���b�v
	m_Queue.dwRead -= dwRest;
	m_Queue.dwRead &= m_Queue.dwMask;
	m_Queue.dwNum += dwRest;

	// �g�[�^��
	m_Queue.dwTotalRead -= dwNum;
	m_Queue.dwTotalRead -= dwRest;
}

//---------------------------------------------------------------------------
//
//	�L���[�̋󂫌����擾
//	��������Ή��B
//
//---------------------------------------------------------------------------
DWORD FASTCALL CQueue::GetFree() const
{
	ASSERT(this);
	ASSERT(m_Queue.dwSize > 0);
	ASSERT(m_Queue.pBuf);
	ASSERT((m_Queue.dwMask + 1) == m_Queue.dwSize);
	ASSERT(m_Queue.dwRead < m_Queue.dwSize);
	ASSERT(m_Queue.dwWrite < m_Queue.dwSize);
	ASSERT(m_Queue.dwNum <= m_Queue.dwSize);

	return (DWORD)(m_Queue.dwSize - m_Queue.dwNum);
}

//---------------------------------------------------------------------------
//
//	�L���[�ɑ}��
//	��������Ή��B
//
//---------------------------------------------------------------------------
BOOL FASTCALL CQueue::Insert(const BYTE *pSrc, DWORD dwLength)
{
	DWORD dwRest;

	ASSERT(this);
	ASSERT(m_Queue.dwSize > 0);
	ASSERT(m_Queue.pBuf);
	ASSERT((m_Queue.dwMask + 1) == m_Queue.dwSize);
	ASSERT(m_Queue.dwRead < m_Queue.dwSize);
	ASSERT(m_Queue.dwWrite < m_Queue.dwSize);
	ASSERT(m_Queue.dwNum <= m_Queue.dwSize);

	ASSERT(pSrc);

	// ���ݕ��A�I�[�o�[���b�v���̌������߂�
	dwRest = 0;
	if ((dwLength + m_Queue.dwWrite) > m_Queue.dwSize) {
		dwRest = (dwLength + m_Queue.dwWrite) - m_Queue.dwSize;
		dwLength -= dwRest;
	}

	// �J�����g
	memcpy(&m_Queue.pBuf[m_Queue.dwWrite], pSrc, dwLength);
	m_Queue.dwWrite += dwLength;
	m_Queue.dwWrite &= m_Queue.dwMask;
	m_Queue.dwNum += dwLength;

	// �I�[�o�[���b�v
	memcpy(m_Queue.pBuf, &pSrc[dwLength], dwRest);
	m_Queue.dwWrite += dwRest;
	m_Queue.dwWrite &= m_Queue.dwMask;
	m_Queue.dwNum += dwRest;

	// �g�[�^��Write�𑝂₷
	m_Queue.dwTotalWrite += dwLength;
	m_Queue.dwTotalWrite += dwRest;

	// Read�ʒu�𒴂��Ă��܂����ꍇ�͕␳����FALSE
	if (m_Queue.dwNum > m_Queue.dwSize) {
		// �o�b�t�@�I�[�o�[����
		m_Queue.dwNum = m_Queue.dwSize;
		m_Queue.dwRead = m_Queue.dwWrite;
		return FALSE;
	}
	return TRUE;
}

//---------------------------------------------------------------------------
//
//	�L���[���擾
//	��������Ή��B
//
//---------------------------------------------------------------------------
void FASTCALL CQueue::GetQueue(QUEUEINFO *pInfo) const
{
	ASSERT(this);
	ASSERT(pInfo);

	*pInfo = m_Queue;
}

#endif	// _WIN32
