//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2004 �o�h�D(ytanaka@ipc-tokai.or.jp)
//	[ MFC TrueKey ]
//
//---------------------------------------------------------------------------

#if defined(_WIN32)

#if !defined(mfc_tkey_h)
#define mfc_tkey_h

#include "fileio.h"
#include "mfc_que.h"

//===========================================================================
//
//	TrueKey
//
//===========================================================================
class CTKey : public CComponent
{
public:
	// ��{�t�@���N�V����
	CTKey(CFrmWnd *pWnd);
										// �R���X�g���N�^
	BOOL FASTCALL Init();
										// ������
	void FASTCALL Cleanup();
										// �N���[���A�b�v
	void FASTCALL ApplyCfg(const Config *pConfig);
										// �ݒ�K�p

	// �O��API
	void FASTCALL Run();
										// �X���b�h���s
	void FASTCALL GetTxQueue(CQueue::LPQUEUEINFO lpqi) const;
										// ���M�L���[���擾
	void FASTCALL GetRxQueue(CQueue::LPQUEUEINFO lpqi) const;
										// ��M�L���[���擾

	// �L�[�}�b�v
	void FASTCALL GetKeyMap(int *pMap);
										// �L�[�}�b�v�擾
	void FASTCALL SetKeyMap(const int *pMap);
										// �L�[�}�b�v�ݒ�
	LPCTSTR FASTCALL GetKeyID(int nVK);
										// VK�L�[ID�擾

private:
	typedef struct _VKEYID {
		int nVK;						// ���z�L�[�R�[�h
		LPCSTR lpszID;					// ���z�L�[��
	} VKEYID, *PVKEYID;
	enum {
		KeyMax = 0x73					// �L�[����(1<=nKey<=KeyMax)
	};

	BOOL FASTCALL Open();
										// �|�[�g�I�[�v��
	void FASTCALL Close();
										// �|�[�g�N���[�Y
	void FASTCALL Ctrl(BOOL bForce);
										// RTS����
	void FASTCALL Rx();
										// ��M
	void FASTCALL Tx();
										// ���M
	void FASTCALL BufSync();
										// �o�b�t�@����

	// �p�����[�^
	int m_nMode;
										// ���[�h(bit0:VM bit1:Win32)
	int m_nCOM;
										// �|�[�g�ԍ�(0�Ŏg�p���Ȃ�)
	BOOL m_bRTS;
										// RTS�_��(���M)
	BOOL m_bLine;
										// RTS���C�����

	// �n���h���E�X���b�h
	HANDLE m_hCOM;
										// �t�@�C���n���h��
	CWinThread *m_pCOM;
										// �X���b�h
	BOOL m_bReq;
										// �X���b�h�I���v��

	// ���M����
	CQueue m_TxQueue;
										// ���M�L���[
	OVERLAPPED m_TxOver;
										// �I�[�o�[���b�v
	BOOL m_bTxValid;
										// ���M�L���t���O
	BYTE m_TxBuf[0x1000];
										// ���M�I�[�o�[���b�v�o�b�t�@

	// ��M����
	CQueue m_RxQueue;
										// ��M�L���[
	OVERLAPPED m_RxOver;
										// �I�[�o�[���b�v
	BYTE m_RxBuf[0x1000];
										// ��M�I�[�o�[���b�v�o�b�t�@

	// �L�[�}�b�v
	BOOL m_bKey[KeyMax];
										// �L�[�����t���O
	BOOL m_bWin[KeyMax];
										// Windows�L�[�����t���O
	int m_nKey[KeyMax];
										// �L�[�ϊ��e�[�u��
	static const int KeyTable[KeyMax];
										// �L�[�ϊ��e�[�u��(�f�t�H���g)
	static const VKEYID KeyIDTable[];
										// VK�L�[ID�e�[�u��

	// �I�u�W�F�N�g
	MFP *m_pMFP;
										// MFP
	Keyboard *m_pKeyboard;
										// �L�[�{�[�h
	CScheduler *m_pSch;
										// �X�P�W���[��
};

#endif	// mfc_tkey_h
#endif	// _WIN32
