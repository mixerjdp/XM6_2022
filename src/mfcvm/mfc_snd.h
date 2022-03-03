//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 ＰＩ．(ytanaka@ipc-tokai.or.jp)
//	[ MFC サウンド ]
//
//---------------------------------------------------------------------------

#if defined(_WIN32)

#if !defined(mfc_snd_h)
#define mfc_snd_h

#include "opm.h"
#include "fileio.h"

//===========================================================================
//
//	サウンド
//
//===========================================================================
class CSound : public CComponent
{
public:
	// 基本ファンクション
	CSound(CFrmWnd *pWnd);
										// コンストラクタ
	void FASTCALL Enable(BOOL bEnable);
										// 動作制御
	BOOL FASTCALL Init();
										// 初期化
	void FASTCALL Cleanup();
										// クリーンアップ
	void FASTCALL ApplyCfg(const Config *pConfig);
										// 設定適用

	// 外部API
	void FASTCALL Process(BOOL bRun);
										// 進行
	BOOL FASTCALL IsPlay() const		{ return m_bPlay; }
										// 再生フラグを取得
	void FASTCALL SetVolume(int nVolume);
										// 音量セット
	int FASTCALL GetVolume();
										// 音量取得
	void FASTCALL SetFMVol(int nVolume);
										// FM音源音量セット
	void FASTCALL SetADPCMVol(int nVolume);
										// ADPCM音源音量セット
	int FASTCALL GetMasterVol(int& nMaximum);
										// マスタ音量取得
	void FASTCALL SetMasterVol(int nVolume);
										// マスタ音量セット
	BOOL FASTCALL StartSaveWav(LPCTSTR lpszWavFile);
										// WAVセーブ開始
	BOOL FASTCALL IsSaveWav() const;
										// WAVセーブ中か
	void FASTCALL EndSaveWav();
										// WAVセーブ終了

	// デバイス
	LPGUID m_lpGUID[16];
										// DirectSoundデバイスのGUID
	CString m_DeviceDescr[16];
										// DirectSoundデバイスの名前
	int m_nDeviceNum;
										// 検出したデバイス数

private:
	// 初期設定
	BOOL FASTCALL InitSub();
										// 初期化サブ
	void FASTCALL CleanupSub();
										// クリーンアップサブ
	void FASTCALL Play();
										// 演奏開始
	void FASTCALL Stop();
										// 演奏停止
	void FASTCALL EnumDevice();
										// デバイス列挙
	static BOOL CALLBACK EnumCallback(LPGUID lpGuid, LPCSTR lpDescr, LPCSTR lpModule,
					void *lpContext);	// デバイス列挙コールバック
	int m_nSelectDevice;
										// 選択したデバイス

	// WAVセーブ
	void FASTCALL ProcessSaveWav(int *pStream, DWORD dwLength);
										// WAVセーブ実行
	Fileio m_WavFile;
										// WAVファイル
	WORD *m_pWav;
										// WAVデータバッファ(フラグ兼用)
	UINT m_nWav;
										// WAVデータバッファオフセット
	DWORD m_dwWav;
										// WAVトータル書き込みサイズ

	// 再生
	UINT m_uRate;
										// サンプリングレート
	UINT m_uTick;
										// バッファサイズ(ms)
	UINT m_uPoll;
										// ポーリング間隔(ms)
	UINT m_uCount;
										// ポーリングカウント
	UINT m_uBufSize;
										// バッファサイズ(バイト)
	BOOL m_bPlay;
										// 再生フラグ
	DWORD m_dwWrite;
										// 書き込み完了位置
	int m_nMaster;
										// マスタ音量
	int m_nFMVol;
										// FM音量(0～100)
	int m_nADPCMVol;
										// ADPCM音量(0～100)
	LPDIRECTSOUND m_lpDS;
										// DirectSound
	LPDIRECTSOUNDBUFFER m_lpDSp;
										// DirectSoundBuffer(プライマリ)
	LPDIRECTSOUNDBUFFER m_lpDSb;
										// DirectSoundBuffer(セカンダリ)
	DWORD *m_lpBuf;
										// サウンドバッファ
	static const UINT RateTable[];
										// サンプリングレートテーブル

	// オブジェクト
	OPMIF *m_pOPMIF;
										// OPMインタフェース
	ADPCM *m_pADPCM;
										// ADPCM
	SCSI *m_pSCSI;
										// SCSI
	FM::OPM *m_pOPM;
										// OPMデバイス
	Scheduler *m_pScheduler;
										// スケジューラ
};

#endif	// mfc_snd_h
#endif	// _WIN32
