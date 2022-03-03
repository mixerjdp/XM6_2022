//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 �o�h�D(ytanaka@ipc-tokai.or.jp)
//	[ MFC �T�E���h ]
//
//---------------------------------------------------------------------------

#if defined(_WIN32)

#if !defined(mfc_snd_h)
#define mfc_snd_h

#include "opm.h"
#include "fileio.h"

//===========================================================================
//
//	�T�E���h
//
//===========================================================================
class CSound : public CComponent
{
public:
	// ��{�t�@���N�V����
	CSound(CFrmWnd *pWnd);
										// �R���X�g���N�^
	void FASTCALL Enable(BOOL bEnable);
										// ���쐧��
	BOOL FASTCALL Init();
										// ������
	void FASTCALL Cleanup();
										// �N���[���A�b�v
	void FASTCALL ApplyCfg(const Config *pConfig);
										// �ݒ�K�p

	// �O��API
	void FASTCALL Process(BOOL bRun);
										// �i�s
	BOOL FASTCALL IsPlay() const		{ return m_bPlay; }
										// �Đ��t���O���擾
	void FASTCALL SetVolume(int nVolume);
										// ���ʃZ�b�g
	int FASTCALL GetVolume();
										// ���ʎ擾
	void FASTCALL SetFMVol(int nVolume);
										// FM�������ʃZ�b�g
	void FASTCALL SetADPCMVol(int nVolume);
										// ADPCM�������ʃZ�b�g
	int FASTCALL GetMasterVol(int& nMaximum);
										// �}�X�^���ʎ擾
	void FASTCALL SetMasterVol(int nVolume);
										// �}�X�^���ʃZ�b�g
	BOOL FASTCALL StartSaveWav(LPCTSTR lpszWavFile);
										// WAV�Z�[�u�J�n
	BOOL FASTCALL IsSaveWav() const;
										// WAV�Z�[�u����
	void FASTCALL EndSaveWav();
										// WAV�Z�[�u�I��

	// �f�o�C�X
	LPGUID m_lpGUID[16];
										// DirectSound�f�o�C�X��GUID
	CString m_DeviceDescr[16];
										// DirectSound�f�o�C�X�̖��O
	int m_nDeviceNum;
										// ���o�����f�o�C�X��

private:
	// �����ݒ�
	BOOL FASTCALL InitSub();
										// �������T�u
	void FASTCALL CleanupSub();
										// �N���[���A�b�v�T�u
	void FASTCALL Play();
										// ���t�J�n
	void FASTCALL Stop();
										// ���t��~
	void FASTCALL EnumDevice();
										// �f�o�C�X��
	static BOOL CALLBACK EnumCallback(LPGUID lpGuid, LPCSTR lpDescr, LPCSTR lpModule,
					void *lpContext);	// �f�o�C�X�񋓃R�[���o�b�N
	int m_nSelectDevice;
										// �I�������f�o�C�X

	// WAV�Z�[�u
	void FASTCALL ProcessSaveWav(int *pStream, DWORD dwLength);
										// WAV�Z�[�u���s
	Fileio m_WavFile;
										// WAV�t�@�C��
	WORD *m_pWav;
										// WAV�f�[�^�o�b�t�@(�t���O���p)
	UINT m_nWav;
										// WAV�f�[�^�o�b�t�@�I�t�Z�b�g
	DWORD m_dwWav;
										// WAV�g�[�^���������݃T�C�Y

	// �Đ�
	UINT m_uRate;
										// �T���v�����O���[�g
	UINT m_uTick;
										// �o�b�t�@�T�C�Y(ms)
	UINT m_uPoll;
										// �|�[�����O�Ԋu(ms)
	UINT m_uCount;
										// �|�[�����O�J�E���g
	UINT m_uBufSize;
										// �o�b�t�@�T�C�Y(�o�C�g)
	BOOL m_bPlay;
										// �Đ��t���O
	DWORD m_dwWrite;
										// �������݊����ʒu
	int m_nMaster;
										// �}�X�^����
	int m_nFMVol;
										// FM����(0�`100)
	int m_nADPCMVol;
										// ADPCM����(0�`100)
	LPDIRECTSOUND m_lpDS;
										// DirectSound
	LPDIRECTSOUNDBUFFER m_lpDSp;
										// DirectSoundBuffer(�v���C�}��)
	LPDIRECTSOUNDBUFFER m_lpDSb;
										// DirectSoundBuffer(�Z�J���_��)
	DWORD *m_lpBuf;
										// �T�E���h�o�b�t�@
	static const UINT RateTable[];
										// �T���v�����O���[�g�e�[�u��

	// �I�u�W�F�N�g
	OPMIF *m_pOPMIF;
										// OPM�C���^�t�F�[�X
	ADPCM *m_pADPCM;
										// ADPCM
	SCSI *m_pSCSI;
										// SCSI
	FM::OPM *m_pOPM;
										// OPM�f�o�C�X
	Scheduler *m_pScheduler;
										// �X�P�W���[��
};

#endif	// mfc_snd_h
#endif	// _WIN32
