//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 ＰＩ．(ytanaka@ipc-tokai.or.jp)
//	[ MFC サウンド ]
//
//---------------------------------------------------------------------------

#if defined(_WIN32)

#include "os.h"
#include "xm6.h"
#include "vm.h"
#include "opmif.h"
#include "opm.h"
#include "adpcm.h"
#include "scsi.h"
#include "schedule.h"
#include "config.h"
#include "mfc_frm.h"
#include "mfc_com.h"
#include "mfc_asm.h"
#include "mfc_snd.h"

//===========================================================================
//
//	サウンド
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	コンストラクタ
//
//---------------------------------------------------------------------------
CSound::CSound(CFrmWnd *pWnd) : CComponent(pWnd)
{
	// コンポーネントパラメータ
	m_dwID = MAKEID('S', 'N', 'D', ' ');
	m_strDesc = _T("Sound Renderer");

	// ワーク初期化(設定パラメータ)
	m_uRate = 0;
	m_uTick = 90;
	m_uPoll = 7;
	m_uBufSize = 0;
	m_bPlay = FALSE;
	m_uCount = 0;
	m_dwWrite = 0;
	m_nMaster = 90;
	m_nFMVol = 54;
	m_nADPCMVol = 52;

	// ワーク初期化(DirectSoundとオブジェクト)
	m_lpDS = NULL;
	m_lpDSp = NULL;
	m_lpDSb = NULL;
	m_lpBuf = NULL;
	m_pScheduler = NULL;
	m_pOPM = NULL;
	m_pOPMIF = NULL;
	m_pADPCM = NULL;
	m_pSCSI = NULL;
	m_nDeviceNum = 0;
	m_nSelectDevice = 0;

	// ワーク初期化(WAV録音)
	m_pWav = NULL;
	m_nWav = 0;
	m_dwWav = 0;
}

//---------------------------------------------------------------------------
//
//	初期化
//
//---------------------------------------------------------------------------
BOOL FASTCALL CSound::Init()
{
	// 基本クラス
	if (!CComponent::Init()) {
		return FALSE;
	}

	// スケジューラ取得
	m_pScheduler = (Scheduler*)::GetVM()->SearchDevice(MAKEID('S', 'C', 'H', 'E'));
	ASSERT(m_pScheduler);

	// OPMIF取得
	m_pOPMIF = (OPMIF*)::GetVM()->SearchDevice(MAKEID('O', 'P', 'M', ' '));
	ASSERT(m_pOPMIF);

	// ADPCM取得
	m_pADPCM = (ADPCM*)::GetVM()->SearchDevice(MAKEID('A', 'P', 'C', 'M'));
	ASSERT(m_pADPCM);

	// SCSI取得
	m_pSCSI = (SCSI*)::GetVM()->SearchDevice(MAKEID('S', 'C', 'S', 'I'));
	ASSERT(m_pSCSI);

	// デバイス列挙
	EnumDevice();

	// ここでは初期化しない(ApplyCfgに任せる)
	return TRUE;
}

//---------------------------------------------------------------------------
//
//	初期化サブ
//
//---------------------------------------------------------------------------
BOOL FASTCALL CSound::InitSub()
{
	PCMWAVEFORMAT pcmwf;
	DSBUFFERDESC dsbd;
	WAVEFORMATEX wfex;

	// rate==0なら、何もしない
	if (m_uRate == 0) {
		return TRUE;
	}

	ASSERT(!m_lpDS);
	ASSERT(!m_lpDSp);
	ASSERT(!m_lpDSb);
	ASSERT(!m_lpBuf);
	ASSERT(!m_pOPM);

	// デバイスがなければ0で試し、それでもなければreturn
	if (m_nDeviceNum <= m_nSelectDevice) {
		if (m_nDeviceNum == 0) {
			return TRUE;
		}
		m_nSelectDevice = 0;
	}

	// DiectSoundオブジェクト作成
	if (FAILED(DirectSoundCreate(m_lpGUID[m_nSelectDevice], &m_lpDS, NULL))) {
		// デバイスは使用中
		return TRUE;
	}

	// 協調レベルを設定(優先協調)
	if (FAILED(m_lpDS->SetCooperativeLevel(m_pFrmWnd->m_hWnd, DSSCL_PRIORITY))) {
		return FALSE;
	}

	// プライマリバッファを作成
	memset(&dsbd, 0, sizeof(dsbd));
	dsbd.dwSize = sizeof(dsbd);
	dsbd.dwFlags = DSBCAPS_PRIMARYBUFFER;
	if (FAILED(m_lpDS->CreateSoundBuffer(&dsbd, &m_lpDSp, NULL))) {
		return FALSE;
	}

	// プライマリバッファのフォーマットを指定
	memset(&wfex, 0, sizeof(wfex));
	wfex.wFormatTag = WAVE_FORMAT_PCM;
	wfex.nChannels = 2;
	wfex.nSamplesPerSec = m_uRate;
	wfex.nBlockAlign = 4;
	wfex.nAvgBytesPerSec = wfex.nSamplesPerSec * wfex.nBlockAlign;
	wfex.wBitsPerSample = 16;
	if (FAILED(m_lpDSp->SetFormat(&wfex))) {
		return FALSE;
	}

	// セカンダリバッファを作成
	memset(&pcmwf, 0, sizeof(pcmwf));
	pcmwf.wf.wFormatTag = WAVE_FORMAT_PCM;
	pcmwf.wf.nChannels = 2;
	pcmwf.wf.nSamplesPerSec = m_uRate;
	pcmwf.wf.nBlockAlign = 4;
	pcmwf.wf.nAvgBytesPerSec = pcmwf.wf.nSamplesPerSec * pcmwf.wf.nBlockAlign;
	pcmwf.wBitsPerSample = 16;
	memset(&dsbd, 0, sizeof(dsbd));
	dsbd.dwSize = sizeof(dsbd);
	dsbd.dwFlags = DSBCAPS_STICKYFOCUS | DSBCAPS_GETCURRENTPOSITION2 | DSBCAPS_CTRLVOLUME;
	dsbd.dwBufferBytes = (pcmwf.wf.nAvgBytesPerSec * m_uTick) / 1000;
	dsbd.dwBufferBytes = ((dsbd.dwBufferBytes + 7) >> 3) << 3;	// 8バイト境界
	m_uBufSize = dsbd.dwBufferBytes;
	dsbd.lpwfxFormat = (LPWAVEFORMATEX)&pcmwf;
	if (FAILED(m_lpDS->CreateSoundBuffer(&dsbd, &m_lpDSb, NULL))) {
		return FALSE;
	}

	// サウンドバッファを作成(セカンダリバッファと同一の長さ、1単位DWORD)
	try {
		m_lpBuf = new DWORD [ m_uBufSize / 2 ];
	}
	catch (...) {
		return FALSE;
	}
	if (!m_lpBuf) {
		return FALSE;
	}
	memset(m_lpBuf, sizeof(DWORD) * (m_uBufSize / 2), m_uBufSize);

	// OPMデバイス(標準)を作成
	m_pOPM = new FM::OPM;
	m_pOPM->Init(4000000, m_uRate, true);
	m_pOPM->Reset();
	m_pOPM->SetVolume(m_nFMVol);

	// OPMIFへ通知
	m_pOPMIF->InitBuf(m_uRate);
	m_pOPMIF->SetEngine(m_pOPM);

	// イネーブルなら演奏開始
	if (m_bEnable) {
		Play();
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	クリーンアップ
//
//---------------------------------------------------------------------------
void FASTCALL CSound::Cleanup()
{
	// クリーンアップサブ
	CleanupSub();

	// 基本クラス
	CComponent::Cleanup();
}

//---------------------------------------------------------------------------
//
//	クリーンアップサブ
//
//---------------------------------------------------------------------------
void FASTCALL CSound::CleanupSub()
{
	// サウンド停止
	Stop();

	// OPMIFへ通知
	if (m_pOPMIF) {
		m_pOPMIF->SetEngine(NULL);
	}

	// OPMを解放
	if (m_pOPM) {
		delete m_pOPM;
		m_pOPM = NULL;
	}

	// サウンド作成バッファを解放
	if (m_lpBuf) {
		delete[] m_lpBuf;
		m_lpBuf = NULL;
	}

	// DirectSoundBufferを解放
	if (m_lpDSb) {
		m_lpDSb->Release();
		m_lpDSb = NULL;
	}
	if (m_lpDSp) {
		m_lpDSp->Release();
		m_lpDSp = NULL;
	}

	// DirectSoundを解放
	if (m_lpDS) {
		m_lpDS->Release();
		m_lpDS = NULL;
	}

	// uRateをクリア
	m_uRate = 0;
}

//---------------------------------------------------------------------------
//
//	設定適用
//
//---------------------------------------------------------------------------
void FASTCALL CSound::ApplyCfg(const Config *pConfig)
{
	BOOL bFlag;

	ASSERT(this);
	ASSERT(pConfig);

	// 再初期化チェック
	bFlag = FALSE;
	if (m_nSelectDevice != pConfig->sound_device) {
		bFlag = TRUE;
	}
	if (m_uRate != RateTable[pConfig->sample_rate]) {
		bFlag = TRUE;
	}
	if (m_uTick != (UINT)(pConfig->primary_buffer * 10)) {
		bFlag = TRUE;
	}

	// 再初期化
	if (bFlag) {
		CleanupSub();
		m_nSelectDevice = pConfig->sound_device;
		m_uRate = RateTable[pConfig->sample_rate];
		m_uTick = pConfig->primary_buffer * 10;

		// 62.5kHzの場合は、一度96kHzにセットしてから(Prodigy7.1対策)
		if (m_uRate == 62500) {
			// 96kHzで初期化
			m_uRate = 96000;
			InitSub();

			// 初期化できた場合は、少しだけ演奏する
			if (m_lpDSb) {
				// スタート
				if (!m_bEnable) {
					m_lpDSb->Play(0, 0, DSBPLAY_LOOPING);
				}

				// 少しだけ
				::Sleep(20);

				// 止める
				if (!m_bEnable) {
					m_lpDSb->Stop();
				}
			}

			// 62.5kHzで初期化
			CleanupSub();
			m_uRate = 62500;
		}
		InitSub();
	}

	// 常に設定
	if (m_pOPM) {
		SetVolume(pConfig->master_volume);
		m_pOPMIF->EnableFM(pConfig->fm_enable);
		SetFMVol(pConfig->fm_volume);
		m_pADPCM->EnableADPCM(pConfig->adpcm_enable);
		SetADPCMVol(pConfig->adpcm_volume);
	}
	m_nMaster = pConfig->master_volume;
	m_uPoll = (UINT)pConfig->polling_buffer;
}

//---------------------------------------------------------------------------
//
//	サンプリングレートテーブル
//
//---------------------------------------------------------------------------
const UINT CSound::RateTable[] = {
	0,
	44100,
	48000,
	88200,
	96000,
	62500
};

//---------------------------------------------------------------------------
//
//	有効フラグ設定
//
//---------------------------------------------------------------------------
void FASTCALL CSound::Enable(BOOL bEnable)
{
	if (bEnable) {
		// 無効→有効 演奏開始
		if (!m_bEnable) {
			m_bEnable = TRUE;
			Play();
		}
	}
	else {
		// 有効→無効 演奏停止
		if (m_bEnable) {
			m_bEnable = FALSE;
			Stop();
		}
	}
}

//---------------------------------------------------------------------------
//
//	演奏開始
//
//---------------------------------------------------------------------------
void FASTCALL CSound::Play()
{
	ASSERT(m_bEnable);

	// 既に演奏開始なら必要なし
	if (m_bPlay) {
		return;
	}

	// ポインタが有効なら演奏開始
	if (m_pOPM) {
		m_lpDSb->Play(0, 0, DSBPLAY_LOOPING);
		m_bPlay = TRUE;
		m_uCount = 0;
		m_dwWrite = 0;
	}
}

//---------------------------------------------------------------------------
//
//	演奏停止
//
//---------------------------------------------------------------------------
void FASTCALL CSound::Stop()
{
	// 既に演奏停止なら必要なし
	if (!m_bPlay) {
		return;
	}

	// WAVセーブ終了
	if (m_pWav) {
		EndSaveWav();
	}

	// ポインタが有効なら演奏停止
	if (m_pOPM) {
		m_lpDSb->Stop();
		m_bPlay = FALSE;
	}
}

//---------------------------------------------------------------------------
//
//	進行
//
//---------------------------------------------------------------------------
void FASTCALL CSound::Process(BOOL bRun)
{
	HRESULT hr;
	DWORD dwOffset;
	DWORD dwWrite;
	DWORD dwRequest;
	DWORD dwReady;
	WORD *pBuf1;
	WORD *pBuf2;
	DWORD dwSize1;
	DWORD dwSize2;

	ASSERT(this);

	// カウント処理(m_nPoll回に１回、ただしVM停止中は常時)
	m_uCount++;
	if ((m_uCount < m_uPoll) && bRun) {
		return;
	}
	m_uCount = 0;

	// ディセーブルなら、何もしない
	if (!m_bEnable) {
		return;
	}

	// 初期化されていなければ、何もしない
	if (!m_pOPM) {
		m_pScheduler->SetSoundTime(0);
		return;
	}

	// プレイ状態でなければ、関係なし
	if (!m_bPlay) {
		m_pScheduler->SetSoundTime(0);
		return;
	}

	// 現在のプレイ位置を得る(バイト単位)
	ASSERT(m_lpDSb);
	ASSERT(m_lpBuf);
	if (FAILED(m_lpDSb->GetCurrentPosition(&dwOffset, &dwWrite))) {
		return;
	}
	ASSERT(m_lpDSb);
	ASSERT(m_lpBuf);

	// 前回書き込んだ位置から、空きサイズを計算(バイト単位)
	if (m_dwWrite <= dwOffset) {
		dwRequest = dwOffset - m_dwWrite;
	}
	else {
		dwRequest = m_uBufSize - m_dwWrite;
		dwRequest += dwOffset;
	}

	// 空きサイズが全体の1/4を超えていなければ、次の機会に
	if (dwRequest < (m_uBufSize / 4)) {
		return;
	}

	// 空きサンプルに換算(L,Rで1つと数える)
	ASSERT((dwRequest & 3) == 0);
	dwRequest /= 4;

	// m_lpBufにバッファデータを作成。まずbRunチェック
	if (!bRun) {
		memset(m_lpBuf, 0, m_uBufSize * 2);
		m_pOPMIF->InitBuf(m_uRate);
	}
	else {
		// OPMに対して、処理要求と速度制御
		dwReady = m_pOPMIF->ProcessBuf();
		m_pOPMIF->GetBuf(m_lpBuf, (int)dwRequest);
		if (dwReady < dwRequest) {
			dwRequest = dwReady;
		}

		// ADPCMに対して、データを要求(加算すること)
		m_pADPCM->GetBuf(m_lpBuf, (int)dwRequest);

		// ADPCMの同期処理
		if (dwReady > dwRequest) {
			m_pADPCM->Wait(dwReady - dwRequest);
		}
		else {
			m_pADPCM->Wait(0);
		}

		// SCSIに対して、データを要求(加算すること)
		m_pSCSI->GetBuf(m_lpBuf, (int)dwRequest, m_uRate);
	}

	// 次いでロック
	hr = m_lpDSb->Lock(m_dwWrite, (dwRequest * 4),
						(void**)&pBuf1, &dwSize1,
						(void**)&pBuf2, &dwSize2,
						0);
	// バッファが失われていれば、リストア
	if (hr == DSERR_BUFFERLOST) {
		m_lpDSb->Restore();
	}
	// ロック成功しなければ、続けても意味がない
	if (FAILED(hr)) {
		m_dwWrite = dwOffset;
		return;
	}

	// 量子化bit=16を前提とする
	ASSERT((dwSize1 & 1) == 0);
	ASSERT((dwSize2 & 1) == 0);

	// MMX命令によるパック(dwSize1+dwSize2で、平均5000～15000程度は処理する)
	SoundMMX(m_lpBuf, pBuf1, dwSize1);
	if (dwSize2 > 0) {
		SoundMMX(&m_lpBuf[dwSize1 / 2], pBuf2, dwSize2);
	}
	SoundEMMS();

	// アンロック
	m_lpDSb->Unlock(pBuf1, dwSize1, pBuf2, dwSize2);

	// m_dwWrite更新
	m_dwWrite += dwSize1;
	m_dwWrite += dwSize2;
	if (m_dwWrite >= m_uBufSize) {
		m_dwWrite -= m_uBufSize;
	}
	ASSERT(m_dwWrite < m_uBufSize);

	// 動作中ならWAV更新
	if (bRun && m_pWav) {
		ProcessSaveWav((int*)m_lpBuf, (dwSize1 + dwSize2));
	}
}

//---------------------------------------------------------------------------
//
//	音量セット
//
//---------------------------------------------------------------------------
void FASTCALL CSound::SetVolume(int nVolume)
{
	LONG lVolume;

	ASSERT(this);
	ASSERT((nVolume >= 0) && (nVolume <= 100));

	// 初期化されていなければ設定しない
	if (!m_pOPM) {
		return;
	}

	// 値を変換
	lVolume = 100 - nVolume;
	lVolume *= (DSBVOLUME_MAX - DSBVOLUME_MIN);
	lVolume /= -200;

	// 設定
	m_lpDSb->SetVolume(lVolume);
}

//---------------------------------------------------------------------------
//
//	音量取得
//
//---------------------------------------------------------------------------
int FASTCALL CSound::GetVolume()
{
	LONG lVolume;

	ASSERT(this);

	// 初期化されていなければ、既定の値を受け取る
	if (!m_pOPM) {
		return m_nMaster;
	}

	// 値を取得
	ASSERT(m_lpDSb);
	if (FAILED(m_lpDSb->GetVolume(&lVolume))) {
		return 0;
	}

	// 値を補正
	lVolume *= -200;
	lVolume /= (DSBVOLUME_MAX - DSBVOLUME_MIN);
	ASSERT((lVolume >= 0) && (lVolume <= 200));
	if (lVolume >= 100) {
		lVolume = 0;
	}
	else {
		lVolume = 100 - lVolume;
	}

	return (int)lVolume;
}

//---------------------------------------------------------------------------
//
//	FM音源音量セット
//
//---------------------------------------------------------------------------
void FASTCALL CSound::SetFMVol(int nVol)
{
	ASSERT(this);
	ASSERT((nVol >= 0) && (nVol <= 100));

	// コピー
	m_nFMVol = nVol;

	// 設定
	if (m_pOPM) {
		m_pOPM->SetVolume(m_nFMVol);
	}
}

//---------------------------------------------------------------------------
//
//	ADPCM音源音量セット
//
//---------------------------------------------------------------------------
void FASTCALL CSound::SetADPCMVol(int nVol)
{
	ASSERT(this);
	ASSERT((nVol >= 0) && (nVol <= 100));

	// コピー
	m_nADPCMVol = nVol;

	// 設定
	ASSERT(m_pADPCM);
	m_pADPCM->SetVolume(m_nADPCMVol);
}

//---------------------------------------------------------------------------
//
//	デバイス列挙
//
//---------------------------------------------------------------------------
void FASTCALL CSound::EnumDevice()
{
	// 初期化
	m_nDeviceNum = 0;

	// 列挙開始
	DirectSoundEnumerate(EnumCallback, this);
}

//---------------------------------------------------------------------------
//
//	デバイス列挙コールバック
//
//---------------------------------------------------------------------------
BOOL CALLBACK CSound::EnumCallback(LPGUID lpGuid, LPCTSTR lpDescr, LPCTSTR /*lpModule*/, LPVOID lpContext)
{
	CSound *pSound;
	int index;

	// thisポインタ受け取り
	pSound = (CSound*)lpContext;
	ASSERT(pSound);

	// カレントが16未満なら記憶
	if (pSound->m_nDeviceNum < 16) {
		index = pSound->m_nDeviceNum;

		// 登録
		pSound->m_lpGUID[index] = lpGuid;
		pSound->m_DeviceDescr[index] = lpDescr;
		pSound->m_nDeviceNum++;
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	マスタ音量取得
//
//---------------------------------------------------------------------------
int FASTCALL CSound::GetMasterVol(int& nMaximum)
{
	MMRESULT mmResult;
	HMIXER hMixer;
	MIXERLINE mixLine;
	MIXERLINECONTROLS mixLCs;
	MIXERCONTROL mixCtrl;
	MIXERCONTROLDETAILS mixDetail;
	MIXERCONTROLDETAILS_UNSIGNED *pData;
	int nNum;
	int nValue;

	ASSERT(this);

	// 使用しているデバイス番号が0であることが必要
	if ((m_nSelectDevice != 0) || (m_uRate == 0)) {
		return -1;
	}

	// ミキサをオープン
	mmResult = ::mixerOpen(&hMixer, 0, 0, 0,
					MIXER_OBJECTF_MIXER);
	if (mmResult != MMSYSERR_NOERROR) {
		// ミキサオープンエラー
		return -1;
	}

	// ライン情報を得る
	memset(&mixLine, 0, sizeof(mixLine));
	mixLine.cbStruct = sizeof(mixLine);
	mixLine.dwComponentType = MIXERLINE_COMPONENTTYPE_DST_SPEAKERS;
	mmResult = ::mixerGetLineInfo((HMIXEROBJ)hMixer, &mixLine,
					MIXER_OBJECTF_HMIXER | MIXER_GETLINEINFOF_COMPONENTTYPE);
	if (mmResult != MMSYSERR_NOERROR) {
		// クローズして終了
		::mixerClose(hMixer);
		return -1;
	}

	// コントロール情報を得る
	memset(&mixLCs, 0, sizeof(mixLCs));
	mixLCs.cbStruct = sizeof(mixLCs);
	mixLCs.dwLineID = mixLine.dwLineID;
	mixLCs.dwControlType = MIXERCONTROL_CONTROLTYPE_VOLUME;
	mixLCs.cbmxctrl = sizeof(mixCtrl);
	mixLCs.pamxctrl = &mixCtrl;
	memset(&mixCtrl, 0, sizeof(mixCtrl));
	mixCtrl.cbStruct = sizeof(mixCtrl);
	mmResult = ::mixerGetLineControls((HMIXEROBJ)hMixer, &mixLCs,
					MIXER_OBJECTF_HMIXER | MIXER_GETLINECONTROLSF_ONEBYTYPE);
	if (mmResult != MMSYSERR_NOERROR) {
		// クローズして終了
		::mixerClose(hMixer);
		return -1;
	}

	// コントロールの個数を見て、メモリ確保
	nNum = 1;
	if (mixLine.cChannels > 0) {
		nNum *= mixLine.cChannels;
	}
	if (mixCtrl.cMultipleItems > 0) {
		nNum *= mixCtrl.cMultipleItems;
	}
	pData = new MIXERCONTROLDETAILS_UNSIGNED[nNum];

	// コントロールの値を得る
	memset(&mixDetail, 0, sizeof(mixDetail));
	mixDetail.cbStruct = sizeof(mixDetail);
	mixDetail.dwControlID = mixCtrl.dwControlID;
	mixDetail.cChannels = mixLine.cChannels;
	mixDetail.cMultipleItems = mixCtrl.cMultipleItems;
	mixDetail.cbDetails = nNum * sizeof(MIXERCONTROLDETAILS_UNSIGNED);
	mixDetail.paDetails = pData;
	mmResult = ::mixerGetControlDetails((HMIXEROBJ)hMixer, &mixDetail,
					MIXER_OBJECTF_HMIXER | MIXER_GETCONTROLDETAILSF_VALUE);
	if (mmResult != MMSYSERR_NOERROR) {
		// クローズして終了
		delete[] pData;
		::mixerClose(hMixer);
		return -1;
	}

	// 最小値が0の場合のみ
	if (mixCtrl.Bounds.lMinimum != 0) {
		// クローズして終了
		delete[] pData;
		::mixerClose(hMixer);
		return -1;
	}

	// 値を処理
	nValue = pData[0].dwValue;
	nMaximum = mixCtrl.Bounds.lMaximum;

	// 成功
	delete[] pData;
	::mixerClose(hMixer);

	return nValue;
}

//---------------------------------------------------------------------------
//
//	マスタ音量セット
//
//---------------------------------------------------------------------------
void FASTCALL CSound::SetMasterVol(int nVolume)
{
	MMRESULT mmResult;
	HMIXER hMixer;
	MIXERLINE mixLine;
	MIXERLINECONTROLS mixLCs;
	MIXERCONTROL mixCtrl;
	MIXERCONTROLDETAILS mixDetail;
	MIXERCONTROLDETAILS_UNSIGNED *pData;
	int nIndex;
	int nNum;

	ASSERT(this);

	// 使用しているデバイス番号が0であることが必要
	if ((m_nSelectDevice != 0) || (m_uRate == 0)) {
		return;
	}

	// ミキサをオープン
	mmResult = ::mixerOpen(&hMixer, 0, 0, 0,
					MIXER_OBJECTF_MIXER);
	if (mmResult != MMSYSERR_NOERROR) {
		// ミキサオープンエラー
		return;
	}

	// ライン情報を得る
	memset(&mixLine, 0, sizeof(mixLine));
	mixLine.cbStruct = sizeof(mixLine);
	mixLine.dwComponentType = MIXERLINE_COMPONENTTYPE_DST_SPEAKERS;
	mmResult = ::mixerGetLineInfo((HMIXEROBJ)hMixer, &mixLine,
					MIXER_OBJECTF_HMIXER | MIXER_GETLINEINFOF_COMPONENTTYPE);
	if (mmResult != MMSYSERR_NOERROR) {
		// クローズして終了
		::mixerClose(hMixer);
		return;
	}

	// コントロール情報を得る
	memset(&mixLCs, 0, sizeof(mixLCs));
	mixLCs.cbStruct = sizeof(mixLCs);
	mixLCs.dwLineID = mixLine.dwLineID;
	mixLCs.dwControlType = MIXERCONTROL_CONTROLTYPE_VOLUME;
	mixLCs.cbmxctrl = sizeof(mixCtrl);
	mixLCs.pamxctrl = &mixCtrl;
	memset(&mixCtrl, 0, sizeof(mixCtrl));
	mixCtrl.cbStruct = sizeof(mixCtrl);
	mmResult = ::mixerGetLineControls((HMIXEROBJ)hMixer, &mixLCs,
					MIXER_OBJECTF_HMIXER | MIXER_GETLINECONTROLSF_ONEBYTYPE);
	if (mmResult != MMSYSERR_NOERROR) {
		// クローズして終了
		::mixerClose(hMixer);
		return;
	}

	// コントロールの個数を見て、メモリ確保
	nNum = 1;
	if (mixLine.cChannels > 0) {
		nNum *= mixLine.cChannels;
	}
	if (mixCtrl.cMultipleItems > 0) {
		nNum *= mixCtrl.cMultipleItems;
	}
	pData = new MIXERCONTROLDETAILS_UNSIGNED[nNum];

	// コントロールの値を得る
	memset(&mixDetail, 0, sizeof(mixDetail));
	mixDetail.cbStruct = sizeof(mixDetail);
	mixDetail.dwControlID = mixCtrl.dwControlID;
	mixDetail.cChannels = mixLine.cChannels;
	mixDetail.cMultipleItems = mixCtrl.cMultipleItems;
	mixDetail.cbDetails = nNum * sizeof(MIXERCONTROLDETAILS_UNSIGNED);
	mixDetail.paDetails = pData;
	mmResult = ::mixerGetControlDetails((HMIXEROBJ)hMixer, &mixDetail,
					MIXER_OBJECTF_HMIXER | MIXER_GETCONTROLDETAILSF_VALUE);
	if (mmResult != MMSYSERR_NOERROR) {
		// クローズして終了
		delete[] pData;
		::mixerClose(hMixer);
		return;
	}

	// 値を処理
	ASSERT(mixCtrl.Bounds.lMinimum <= nVolume);
	ASSERT(nVolume <= mixCtrl.Bounds.lMaximum);
	for (nIndex=0; nIndex<nNum; nIndex++) {
		pData[nIndex].dwValue = (DWORD)nVolume;
	}

	// コントロールの値を設定
	mmResult = mixerSetControlDetails((HMIXEROBJ)hMixer, &mixDetail,
					MIXER_OBJECTF_HMIXER | MIXER_GETCONTROLDETAILSF_VALUE);
	if (mmResult != MMSYSERR_NOERROR) {
		delete[] pData;
		::mixerClose(hMixer);
		return;
	}

	// 成功
	::mixerClose(hMixer);
	delete[] pData;
}

//---------------------------------------------------------------------------
//
//	WAVセーブ開始
//
//---------------------------------------------------------------------------
BOOL FASTCALL CSound::StartSaveWav(LPCTSTR lpszWavFile)
{
	DWORD dwSize;
	WAVEFORMATEX wfex;

	ASSERT(this);
	ASSERT(lpszWavFile);

	// 既に録音中ならエラー
	if (m_pWav) {
		return FALSE;
	}
	// 再生中でなければエラー
	if (!m_bEnable || !m_bPlay) {
		return FALSE;
	}

	// ファイル作成を試みる
	if (!m_WavFile.Open(lpszWavFile, Fileio::WriteOnly)) {
		return FALSE;
	}

	// RIFFヘッダ書き込み
	if (!m_WavFile.Write((BYTE*)"RIFF0123WAVEfmt ", 16)) {
		m_WavFile.Close();
		return FALSE;
	}

	// WAVEFORMATEX書き込み
	dwSize = sizeof(wfex);
	if (!m_WavFile.Write((BYTE*)&dwSize, sizeof(dwSize))) {
		m_WavFile.Close();
		return FALSE;
	}
	memset(&wfex, 0, sizeof(wfex));
	wfex.cbSize = sizeof(wfex);
	wfex.wFormatTag = WAVE_FORMAT_PCM;
	wfex.nChannels = 2;
	wfex.nSamplesPerSec = m_uRate;
	wfex.nBlockAlign = 4;
	wfex.nAvgBytesPerSec = wfex.nSamplesPerSec * wfex.nBlockAlign;
	wfex.wBitsPerSample = 16;
	if (!m_WavFile.Write((BYTE*)&wfex, sizeof(wfex))) {
		m_WavFile.Close();
		return FALSE;
	}

	// dataサブヘッダ書き込み
	if (!m_WavFile.Write((BYTE*)"data0123", 8)) {
		m_WavFile.Close();
		return FALSE;
	}

	// 録音バッファを確保
	try {
		m_pWav = new WORD[0x20000];
	}
	catch (...) {
		return FALSE;
	}
	if (!m_pWav) {
		return FALSE;
	}

	// ワーク初期化
	m_nWav = 0;
	m_dwWav = 0;

	// コンポーネントのフラグをクリア
	m_pOPMIF->ClrStarted();
	m_pADPCM->ClrStarted();

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	WAVセーブ中か
//
//---------------------------------------------------------------------------
BOOL FASTCALL CSound::IsSaveWav() const
{
	ASSERT(this);

	// バッファでチェック
	if (m_pWav) {
		return TRUE;
	}

	return FALSE;
}

//---------------------------------------------------------------------------
//
//	WAVセーブ
//
//---------------------------------------------------------------------------
void FASTCALL CSound::ProcessSaveWav(int *pStream, DWORD dwLength)
{
	DWORD dwPrev;
	int i;
	int nLen;
	int rawData;

	ASSERT(this);
	ASSERT(pStream);
	ASSERT(dwLength > 0);
	ASSERT((dwLength & 3) == 0);

	// Startedフラグを調べ、ともにクリアならスキップ
	if (!m_pOPMIF->IsStarted() && !m_pADPCM->IsStarted()) {
		return;
	}

	// 前半・後半に分ける必要があるかチェック
	dwPrev = 0;
	if ((dwLength + m_nWav) >= 256 * 1024) {
		dwPrev = 256 * 1024 - m_nWav;
		dwLength -= dwPrev;
	}

	// 前半
	if (dwPrev > 0) {
		nLen = (int)(dwPrev >> 1);
		for (i=0; i<nLen; i++) {
			rawData = *pStream++;
			if (rawData > 0x7fff) {
				rawData = 0x7fff;
			}
			if (rawData < -0x8000) {
				rawData = -0x8000;
			}
			m_pWav[(m_nWav >> 1) + i] = (WORD)rawData;
		}

		m_WavFile.Write(m_pWav, 256 * 1024);
		m_nWav = 0;
	}

	// 後半
	nLen = (int)(dwLength >> 1);
	for (i=0; i<nLen; i++) {
		rawData = *pStream++;
		if (rawData > 0x7fff) {
			rawData = 0x7fff;
		}
		if (rawData < -0x8000) {
			rawData = -0x8000;
		}
		m_pWav[(m_nWav >> 1) + i] = (WORD)rawData;
	}
	m_nWav += dwLength;
	m_dwWav += dwPrev;
	m_dwWav += dwLength;
}

//---------------------------------------------------------------------------
//
//	WAVセーブ終了
//
//---------------------------------------------------------------------------
void FASTCALL CSound::EndSaveWav()
{
	DWORD dwLength;

	// 残ったデータを書き込む
	if (m_nWav > 0) {
		m_WavFile.Write(m_pWav, m_nWav);
		m_nWav = 0;
	}

	// ヘッダ修正
	m_WavFile.Seek(4);
	dwLength = m_dwWav + sizeof(WAVEFORMATEX) + 20;
	m_WavFile.Write((BYTE*)&dwLength, sizeof(dwLength));
	m_WavFile.Seek(sizeof(WAVEFORMATEX) + 24);
	m_WavFile.Write((BYTE*)&m_dwWav, sizeof(m_dwWav));

	// ファイルクローズ
	m_WavFile.Close();

	// メモリ解放
	delete[] m_pWav;
	m_pWav = NULL;
}

#endif	// _WIN32
