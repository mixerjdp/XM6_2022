//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 ＰＩ．(ytanaka@ipc-tokai.or.jp)
//	[ MFC サブウィンドウ(デバイス) ]
//
//---------------------------------------------------------------------------

#if defined(_WIN32)

#if !defined(mfc_dev_h)
#define mfc_dev_h

#include "mfp.h"
#include "dmac.h"
#include "scc.h"
#include "fdc.h"
#include "midi.h"
#include "sasi.h"
#include "scsi.h"
#include "mfc_sub.h"

//===========================================================================
//
//	MFPウィンドウ
//
//===========================================================================
class CMFPWnd : public CSubTextWnd
{
public:
	CMFPWnd();
										// コンストラクタ
	void FASTCALL Setup();
										// セットアップ

private:
	void FASTCALL SetupInt(int x, int y);
										// セットアップ(割り込み)
	void FASTCALL SetupGPIP(int x, int y);
										// セットアップ(GPIP)
	void FASTCALL SetupTimer(int x, int y);
										// セットアップ(タイマ)
	static LPCTSTR DescInt[];
										// 割り込みテーブル
	static LPCTSTR DescGPIP[];
										// GPIPテーブル
	static LPCTSTR DescTimer[];
										// タイマテーブル
	MFP *m_pMFP;
										// MFP
	MFP::mfp_t m_mfp;
										// MFP内部データ
};

//===========================================================================
//
//	DMACウィンドウ
//
//===========================================================================
class CDMACWnd : public CSubTextWnd
{
public:
	CDMACWnd();
										// コンストラクタ
	void FASTCALL Setup();
										// セットアップ

private:
	void FASTCALL SetupCh(int nCh, DMAC::dma_t *pDMA, LPCTSTR lpszTitle);
										// セットアップ(チャネル)
	DMAC *m_pDMAC;
										// DMAC
};

//===========================================================================
//
//	CRTCウィンドウ
//
//===========================================================================
class CCRTCWnd : public CSubTextWnd
{
public:
	CCRTCWnd();
										// コンストラクタ
	void FASTCALL Setup();
										// セットアップ

private:
	CRTC *m_pCRTC;
										// CRTC
};

//===========================================================================
//
//	VCウィンドウ
//
//===========================================================================
class CVCWnd : public CSubTextWnd
{
public:
	CVCWnd();
										// コンストラクタ
	void FASTCALL Setup();
										// セットアップ

private:
	VC *m_pVC;
										// VC
};

//===========================================================================
//
//	RTCウィンドウ
//
//===========================================================================
class CRTCWnd : public CSubTextWnd
{
public:
	CRTCWnd();
										// コンストラクタ
	void FASTCALL Setup();
										// セットアップ

private:
	RTC* m_pRTC;
										// FDC
};

//===========================================================================
//
//	OPMウィンドウ
//
//===========================================================================
class COPMWnd : public CSubTextWnd
{
public:
	COPMWnd();
										// コンストラクタ
	void FASTCALL Setup();
										// セットアップ

private:
	OPMIF *m_pOPM;
										// OPM
};

//===========================================================================
//
//	キーボードウィンドウ
//
//===========================================================================
class CKeyboardWnd : public CSubTextWnd
{
public:
	CKeyboardWnd();
										// コンストラクタ
	void FASTCALL Setup();
										// セットアップ

private:
	static LPCTSTR DescLED[];
										// LEDテーブル
	Keyboard *m_pKeyboard;
										// キーボード
};

//===========================================================================
//
//	FDDウィンドウ
//
//===========================================================================
class CFDDWnd : public CSubTextWnd
{
public:
	CFDDWnd();
										// コンストラクタ
	void FASTCALL Setup();
										// セットアップ

private:
	void FASTCALL SetupFDD(int nDrive, int x);
										// セットアップサブ
	BOOL FASTCALL SetupTrack();
										// セットアップトラック
	static LPCTSTR DescTable[];
										// 説明テーブル
	FDD *m_pFDD;
										// FDD
	FDC *m_pFDC;
										// FDC
	DWORD m_dwDrive;
										// アクセスドライブ
	DWORD m_dwHD;
										// アクセスヘッド
	DWORD m_CHRN[4];
										// アクセスCHRN
};

//===========================================================================
//
//	FDCウィンドウ
//
//===========================================================================
class CFDCWnd : public CSubTextWnd
{
public:
	CFDCWnd();
										// コンストラクタ
	void FASTCALL Setup();
										// セットアップ

private:
	void FASTCALL SetupGeneral(int x, int y);
										// セットアップ(一般)
	void FASTCALL SetupParam(int x, int y);
										// セットアップ(パラメータ)
	void FASTCALL SetupSR(int x, int y);
										// セットアップ(ステータスレジスタ)
	static LPCTSTR SRDesc[8];
										// 文字列(ステータスレジスタ)
	void FASTCALL SetupST0(int x, int y);
										// セットアップ(ST0)
	static LPCTSTR ST0Desc[8];
										// 文字列(ST0)
	void FASTCALL SetupST1(int x, int y);
										// セットアップ(ST1)
	static LPCTSTR ST1Desc[8];
										// 文字列(ST1)
	void FASTCALL SetupST2(int x, int y);
										// セットアップ(ST2)
	static LPCTSTR ST2Desc[8];
										// 文字列(ST2)
	void FASTCALL SetupSub(int x, int y, LPCTSTR lpszTitle, LPCTSTR *lpszDesc,
					DWORD data);		// セットアップ(サブ)
	FDC *m_pFDC;
										// FDC
	const FDC::fdc_t *m_pWork;
										// FDC内部データ
};

//===========================================================================
//
//	SCCウィンドウ
//
//===========================================================================
class CSCCWnd : public CSubTextWnd
{
public:
	CSCCWnd();
										// コンストラクタ
	void FASTCALL Setup();
										// セットアップ

private:
	void FASTCALL SetupSCC(SCC::ch_t *pCh, int x, int y);
										// セットアップ(チャネル)
	SCC *m_pSCC;
										// SCC
	static LPCTSTR DescTable[];
										// 文字列テーブル
};

//===========================================================================
//
//	Cynthiaウィンドウ
//
//===========================================================================
class CCynthiaWnd : public CSubTextWnd
{
public:
	CCynthiaWnd();
										// コンストラクタ
	void FASTCALL Setup();
										// セットアップ

private:
	Sprite *m_pSprite;
										// CYNTHIA
};

//===========================================================================
//
//	SASIウィンドウ
//
//===========================================================================
class CSASIWnd : public CSubTextWnd
{
public:
	CSASIWnd();
										// コンストラクタ
	void FASTCALL Setup();
										// セットアップ

private:
	void FASTCALL SetupCmd(int x, int y);
										// セットアップ(コマンド)
	void FASTCALL SetupCtrl(int x, int y);
										// セットアップ(コントローラ)
	void FASTCALL SetupDrive(int x, int y);
										// セットアップ(ドライブ)
	void FASTCALL SetupCache(int x, int y);
										// セットアップ(キャッシュ)
	SASI *m_pSASI;
										// SASI
	SASI::sasi_t m_sasi;
										// SASI内部データ
};

//===========================================================================
//
//	MIDIウィンドウ
//
//===========================================================================
class CMIDIWnd : public CSubTextWnd
{
public:
	CMIDIWnd();
										// コンストラクタ
	void FASTCALL Setup();
										// セットアップ

private:
	void FASTCALL SetupCtrl(int x, int y);
										// セットアップ(コントロール)
	static LPCTSTR DescCtrl[];
										// 文字列テーブル(コントロール)
	void FASTCALL SetupInt(int x, int y);
										// セットアップ(割り込み)
	static LPCTSTR DescInt[];
										// 文字列テーブル(割り込み)
	void FASTCALL SetupTrans(int x, int y);
										// セットアップ(送信)
	static LPCTSTR DescTrans[];
										// 文字列テーブル(送信)
	void FASTCALL SetupRecv(int x, int y);
										// セットアップ(受信)
	static LPCTSTR DescRecv[];
										// 文字列テーブル(受信)
	void FASTCALL SetupRT(int x, int y);
										// セットアップ(リアルタイム送信)
	static LPCTSTR DescRT[];
										// 文字列テーブル(リアルタイム送信)
	void FASTCALL SetupRR(int x, int y);
										// セットアップ(リアルタイム受信)
	static LPCTSTR DescRR[];
										// 文字列テーブル(リアルタイム受信)
	void FASTCALL SetupCount(int x, int y);
										// セットアップ(カウンタ)
	static LPCTSTR DescCount[];
										// 文字列テーブル(カウンタ)
	void FASTCALL SetupHunter(int x, int y);
										// セットアップ(アドレスハンタ)
	static LPCTSTR DescHunter[];
										// 文字列テーブル(アドレスハンタ)
	void FASTCALL SetupFSK(int x, int y);
										// セットアップ(FSK)
	static LPCTSTR DescFSK[];
										// 文字列テーブル(FSK)
	void FASTCALL SetupGPIO(int x, int y);
										// セットアップ(GPIO)
	static LPCTSTR DescGPIO[];
										// 文字列テーブル(GPIO)

	MIDI *m_pMIDI;
										// MIDI
	MIDI::midi_t m_midi;
										// MIDI内部データ
};

//===========================================================================
//
//	SCSIウィンドウ
//
//===========================================================================
class CSCSIWnd : public CSubTextWnd
{
public:
	CSCSIWnd();
										// コンストラクタ
	void FASTCALL Setup();
										// セットアップ

private:
	void FASTCALL SetupCmd(int x, int y);
										// セットアップ(コマンド)
	void FASTCALL SetupCtrl(int x, int y);
										// セットアップ(コントローラ)
	void FASTCALL SetupDrive(int x, int y);
										// セットアップ(ドライブ)
	void FASTCALL SetupReg(int x, int y);
										// セットアップ(レジスタ)
	void FASTCALL SetupCDB(int x, int y);
										// セットアップ(CDB)
	SCSI *m_pSCSI;
										// SCSI
	SCSI::scsi_t m_scsi;
										// SCSI内部データ
};

#endif	// mfc_dev_h
#endif	// _WIN32
