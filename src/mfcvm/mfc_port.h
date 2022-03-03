//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2004 �o�h�D(ytanaka@ipc-tokai.or.jp)
//	[ MFC �|�[�g ]
//
//---------------------------------------------------------------------------

#if defined(_WIN32)

#if !defined(mfc_port_h)
#define mfc_port_h

#include "fileio.h"
#include "mfc_que.h"

//===========================================================================
//
//	�|�[�g
//
//===========================================================================
class CPort : public CComponent
{
public:
	// ��{�t�@���N�V����
	CPort(CFrmWnd *pWnd);
										// �R���X�g���N�^
	BOOL FASTCALL Init();
										// ������
	void FASTCALL Cleanup();
										// �N���[���A�b�v
	void FASTCALL ApplyCfg(const Config *pConfig);
										// �ݒ�K�p

	// �O��API
	void FASTCALL RunCOM();
										// COM���s
	BOOL FASTCALL GetCOMInfo(LPTSTR lpszDevFile, DWORD *dwLogFile) const;
										// COM���擾
	void FASTCALL GetTxQueue(CQueue::LPQUEUEINFO lpqi) const;
										// ���M�L���[���擾
	void FASTCALL GetRxQueue(CQueue::LPQUEUEINFO lpqi) const;
										// ��M�L���[���擾
	void FASTCALL RunLPT();
										// LPT���s
	BOOL FASTCALL GetLPTInfo(LPTSTR lpszDevFile, DWORD *dwLogFile) const;
										// LPT���擾
	void FASTCALL GetLPTQueue(CQueue::LPQUEUEINFO lpqi) const;
										// LPT�L���[�擾

private:
	// �V���A���|�[�g
	BOOL FASTCALL OpenCOM();
										// COM�I�[�v��
	void FASTCALL AppendCOM();
										// COM�t�@�C��
	void FASTCALL CloseCOM();
										// COM�N���[�Y
	void FASTCALL AdjustCOM(DCB *pDCB);
										// �p�����[�^���킹
	BOOL FASTCALL MatchCOM(DWORD dwSCC, DWORD dwBase);
										// �{�[���[�g�}�b�`
	void FASTCALL SignalCOM();
										// �M�������킹
	void FASTCALL BufCOM();
										// �o�b�t�@���킹
	void FASTCALL CtrlCOM();
										// �M��������
	void FASTCALL OnCTSDSR();
										// CTS, DSR�ω�
	void FASTCALL OnErr();
										// ��M�G���[
	void FASTCALL OnRx();
										// ��M�T�u
	void FASTCALL OnTx();
										// ���M�T�u
	UINT m_nCOM;
										// COM�|�[�g(0�Ŏg�p���Ȃ�)
	HANDLE m_hCOM;
										// �t�@�C���n���h��
	CWinThread *m_pCOM;
										// �X���b�h
	BOOL m_bCOMReq;
										// �X���b�h�I���v��
	BOOL m_bBreak;
										// X68000���o�u���[�N
	BOOL m_bRTS;
										// RTS(���M)
	BOOL m_bDTR;
										// DTR(���M)
	BOOL m_bCTS;
										// CTS(��M)
	BOOL m_bDSR;
										// DSR(��M)
	OVERLAPPED m_TxOver;
										// �I�[�o�[���b�v
	CQueue m_TxQueue;
										// ���M�L���[
	BYTE m_TxBuf[0x1000];
										// ���M�I�[�o�[���b�v�o�b�t�@
	BOOL m_bTxValid;
										// ���M�L���t���O
	OVERLAPPED m_RxOver;
										// �I�[�o�[���b�v
	CQueue m_RxQueue;
										// ��M�L���[
	BYTE m_RxBuf[0x1000];
										// ��M�I�[�o�[���b�v�o�b�t�@
	DWORD m_dwErr;
										// ��M�G���[
	TCHAR m_RecvLog[_MAX_PATH];
										// ��M���O
	Fileio m_RecvFile;
										// ��M���O
	BOOL m_bForce;
										// ����38400bps���[�h
	SCC *m_pSCC;
										// SCC

	// �p�������|�[�g
	BOOL FASTCALL OpenLPT();
										// LPT�I�[�v��
	void FASTCALL AppendLPT();
										// LPT�t�@�C��
	void FASTCALL CloseLPT();
										// LPT�N���[�Y
	void FASTCALL RecvLPT();
										// LPT��M
	void FASTCALL SendLPT();
										// LPT���M

	UINT m_nLPT;
										// LPT�|�[�g(0�Ŏg�p���Ȃ�)
	HANDLE m_hLPT;
										// �t�@�C���n���h��
	CWinThread *m_pLPT;
										// �X���b�h
	BOOL m_bLPTReq;
										// �X���b�h�I���v��
	BOOL m_bLPTValid;
										// ���M�L���t���O
	OVERLAPPED m_LPTOver;
										// �I�[�o�[���b�v
	CQueue m_LPTQueue;
										// �L���[
	TCHAR m_SendLog[_MAX_PATH];
										// ���M���O
	Fileio m_SendFile;
										// ���M���O
	Printer *m_pPrinter;
										// �v�����^
};

#endif	// mfc_port_h
#endif	// _WIN32
