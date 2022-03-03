//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 ＰＩ．(ytanaka@ipc-tokai.or.jp)
//	[ DMAC(HD63450) ]
//
//---------------------------------------------------------------------------

#include "os.h"
#include "xm6.h"
#include "cpu.h"
#include "memory.h"
#include "vm.h"
#include "log.h"
#include "schedule.h"
#include "fdc.h"
#include "fileio.h"
#include "dmac.h"

//===========================================================================
//
//	DMAC
//
//===========================================================================
//#define DMAC_LOG

//---------------------------------------------------------------------------
//
//	コンストラクタ
//
//---------------------------------------------------------------------------
DMAC::DMAC(VM *p) : MemDevice(p)
{
	// デバイスIDを初期化
	dev.id = MAKEID('D', 'M', 'A', 'C');
	dev.desc = "DMAC (HD63450)";

	// 開始アドレス、終了アドレス
	memdev.first = 0xe84000;
	memdev.last = 0xe85fff;

	// その他
	memory = NULL;
	fdc = NULL;
}

//---------------------------------------------------------------------------
//
//	初期化
//
//---------------------------------------------------------------------------
BOOL FASTCALL DMAC::Init()
{
	int ch;

	ASSERT(this);

	// 基本クラス
	if (!MemDevice::Init()) {
		return FALSE;
	}

	// メモリ取得
	memory = (Memory*)vm->SearchDevice(MAKEID('M', 'E', 'M', ' '));
	ASSERT(memory);

	// FDC取得
	fdc = (FDC*)vm->SearchDevice(MAKEID('F', 'D', 'C', ' '));
	ASSERT(fdc);

	// チャネルワークを初期化
	for (ch=0; ch<4; ch++) {
		memset(&dma[ch], 0, sizeof(dma[ch]));
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	クリーンアップ
//
//---------------------------------------------------------------------------
void FASTCALL DMAC::Cleanup()
{
	ASSERT(this);

	// 基本クラスへ
	MemDevice::Cleanup();
}

//---------------------------------------------------------------------------
//
//	リセット
//
//---------------------------------------------------------------------------
void FASTCALL DMAC::Reset()
{
	int ch;

	ASSERT(this);
	LOG0(Log::Normal, "リセット");

	// グローバル
	dmactrl.transfer = 0;
	dmactrl.load = 0;
	dmactrl.exec = FALSE;
	dmactrl.current_ch = 0;
	dmactrl.cpu_cycle = 0;
	dmactrl.vector = -1;

	// DMACチャネルを回る
	for (ch=0; ch<4; ch++) {
		ResetDMA(ch);
	}
}

//---------------------------------------------------------------------------
//
//	セーブ
//
//---------------------------------------------------------------------------
BOOL FASTCALL DMAC::Save(Fileio *fio, int /*ver*/)
{
	size_t sz;
	int i;

	ASSERT(this);
	ASSERT(fio);
	LOG0(Log::Normal, "セーブ");

	// チャネル別
	sz = sizeof(dma_t);
	for (i=0; i<4; i++) {
		if (!fio->Write(&sz, sizeof(sz))) {
			return FALSE;
		}
		if (!fio->Write(&dma[i], (int)sz)) {
			return FALSE;
		}
	}

	// グローバル
	sz = sizeof(dmactrl_t);
	if (!fio->Write(&sz, sizeof(sz))) {
		return FALSE;
	}
	if (!fio->Write(&dmactrl, (int)sz)) {
		return FALSE;
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	ロード
//
//---------------------------------------------------------------------------
BOOL FASTCALL DMAC::Load(Fileio *fio, int /*ver*/)
{
	int i;
	size_t sz;

	ASSERT(this);
	ASSERT(fio);
	LOG0(Log::Normal, "ロード");

	// チャネル別
	for (i=0; i<4; i++) {
		// サイズをロード、照合
		if (!fio->Read(&sz, sizeof(sz))) {
			return FALSE;
		}
		if (sz != sizeof(dma_t)) {
			return FALSE;
		}

		// 実体をロード
		if (!fio->Read(&dma[i], (int)sz)) {
			return FALSE;
		}
	}

	// グローバル
	if (!fio->Read(&sz, sizeof(sz))) {
		return FALSE;
	}
	if (sz != sizeof(dmactrl_t)) {
		return FALSE;
	}

	if (!fio->Read(&dmactrl, (int)sz)) {
		return FALSE;
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	設定適用
//
//---------------------------------------------------------------------------
void FASTCALL DMAC::ApplyCfg(const Config* /*config*/)
{
	ASSERT(this);
//	ASSERT(config);
	LOG0(Log::Normal, "設定適用");
}

//---------------------------------------------------------------------------
//
//	バイト読み込み
//
//---------------------------------------------------------------------------
DWORD FASTCALL DMAC::ReadByte(DWORD addr)
{
	int ch;

	ASSERT(this);
	ASSERT((addr >= memdev.first) && (addr <= memdev.last));

	// ウェイト
	scheduler->Wait(7);

	// チャネルに割り当て
	ch = (int)(addr >> 6);
	ch &= 3;
	addr &= 0x3f;

	// チャネル単位で行う
	return ReadDMA(ch, addr);
}

//---------------------------------------------------------------------------
//
//	ワード読み込み
//
//---------------------------------------------------------------------------
DWORD FASTCALL DMAC::ReadWord(DWORD addr)
{
	int ch;

	ASSERT(this);
	ASSERT((addr >= memdev.first) && (addr <= memdev.last));
	ASSERT((addr & 1) == 0);

	// ウェイト
	scheduler->Wait(7);

	// チャネルに割り当て
	ch = (int)(addr >> 6);
	ch &= 3;
	addr &= 0x3f;

	// チャネル単位で行う
	return ((ReadDMA(ch, addr) << 8) | ReadDMA(ch, addr + 1));
}

//---------------------------------------------------------------------------
//
//	バイト書き込み
//
//---------------------------------------------------------------------------
void FASTCALL DMAC::WriteByte(DWORD addr, DWORD data)
{
	int ch;

	ASSERT(this);
	ASSERT((addr >= memdev.first) && (addr <= memdev.last));
	ASSERT(data < 0x100);

	// ウェイト
	scheduler->Wait(7);

	// チャネルに割り当て
	ch = (int)(addr >> 6);
	ch &= 3;
	addr &= 0x3f;

	// チャネル単位で行う
	WriteDMA(ch, addr, data);
}

//---------------------------------------------------------------------------
//
//	ワード書き込み
//
//---------------------------------------------------------------------------
void FASTCALL DMAC::WriteWord(DWORD addr, DWORD data)
{
	int ch;

	ASSERT(this);
	ASSERT((addr >= memdev.first) && (addr <= memdev.last));
	ASSERT((addr & 1) == 0);
	ASSERT(data < 0x10000);

	// ウェイト
	scheduler->Wait(7);

	// チャネルに割り当て
	ch = (int)(addr >> 6);
	ch &= 3;
	addr &= 0x3f;

	// チャネル単位で行う
	WriteDMA(ch, addr, (BYTE)(data >> 8));
	WriteDMA(ch, addr + 1, (BYTE)data);
}

//---------------------------------------------------------------------------
//
//	読み込みのみ
//
//---------------------------------------------------------------------------
DWORD FASTCALL DMAC::ReadOnly(DWORD addr) const
{
	int ch;

	ASSERT(this);
	ASSERT((addr >= memdev.first) && (addr <= memdev.last));

	// チャネルに割り当て
	ch = (int)(addr >> 6);
	ch &= 3;
	addr &= 0x3f;

	// チャネル単位で行う
	return ReadDMA(ch, addr);
}

//---------------------------------------------------------------------------
//
//	DMA読み込み
//	※上位ゼロ保証
//
//---------------------------------------------------------------------------
DWORD FASTCALL DMAC::ReadDMA(int ch, DWORD addr) const
{
	DWORD data;

	ASSERT(this);
	ASSERT((ch >= 0) && (ch <= 3));
	ASSERT(addr <= 0x3f);

	switch (addr) {
		// CSR
		case 0x00:
			return GetCSR(ch);

		// CER
		case 0x01:
			return dma[ch].ecode;

		// DCR
		case 0x04:
			return GetDCR(ch);

		// OCR
		case 0x05:
			return GetOCR(ch);

		// SCR
		case 0x06:
			return GetSCR(ch);

		// CCR
		case 0x07:
			return GetCCR(ch);

		// MTC
		case 0x0a:
			return (BYTE)(dma[ch].mtc >> 8);
		case 0x0b:
			return (BYTE)dma[ch].mtc;

		// MAR
		case 0x0c:
			return 0;
		case 0x0d:
			return (BYTE)(dma[ch].mar >> 16);
		case 0x0e:
			return (BYTE)(dma[ch].mar >> 8);
		case 0x0f:
			return (BYTE)dma[ch].mar;

		// DAR
		case 0x14:
			return 0;
		case 0x15:
			return (BYTE)(dma[ch].dar >> 16);
		case 0x16:
			return (BYTE)(dma[ch].dar >> 8);
		case 0x17:
			return (BYTE)dma[ch].dar;

		// BTC
		case 0x1a:
			return (BYTE)(dma[ch].btc >> 8);
		case 0x1b:
			return (BYTE)dma[ch].btc;

		// BAR
		case 0x1c:
			return 0;
		case 0x1d:
			return (BYTE)(dma[ch].bar >> 16);
		case 0x1e:
			return (BYTE)(dma[ch].bar >> 8);
		case 0x1f:
			return (BYTE)dma[ch].bar;

		// NIV
		case 0x25:
			return dma[ch].niv;

		// EIV
		case 0x27:
			return dma[ch].eiv;

		// MFC
		case 0x29:
			return dma[ch].mfc;

		// CPR
		case 0x2d:
			return dma[ch].cp;

		// DFC
		case 0x31:
			return dma[ch].dfc;

		// BFC
		case 0x39:
			return dma[ch].bfc;

		// GCR
		case 0x3f:
			if (ch == 3) {
				// チャネル3のみバースト転送情報を返す
				ASSERT(dma[ch].bt <= 3);
				ASSERT(dma[ch].br <= 3);

				data = dma[ch].bt;
				data <<= 2;
				data |= dma[ch].br;
				return data;
			}
			return 0xff;
	}

	return 0xff;
}

//---------------------------------------------------------------------------
//
//	DMA書き込み
//	※上位ゼロ保証を要求
//
//---------------------------------------------------------------------------
void FASTCALL DMAC::WriteDMA(int ch, DWORD addr, DWORD data)
{
	ASSERT(this);
	ASSERT((ch >= 0) && (ch <= 3));
	ASSERT(addr <= 0x3f);
	ASSERT(data < 0x100);

	switch (addr) {
		// CSR
		case 0x00:
			SetCSR(ch, data);
			return;

		// CER(Read Only)
		case 0x01:
			return;

		// DCR
		case 0x04:
			SetDCR(ch, data);
			return;

		// OCR
		case 0x05:
			SetOCR(ch, data);
			return;

		// SCR
		case 0x06:
			SetSCR(ch, data);
			return;

		// CCR
		case 0x07:
			SetCCR(ch, data);
			return;

		// MTC
		case 0x0a:
			dma[ch].mtc &= 0x00ff;
			dma[ch].mtc |= (data << 8);
			return;
		case 0x0b:
			dma[ch].mtc &= 0xff00;
			dma[ch].mtc |= data;
			return;

		// MAR
		case 0x0c:
			return;
		case 0x0d:
			dma[ch].mar &= 0x0000ffff;
			dma[ch].mar |= (data << 16);
			return;
		case 0x0e:
			dma[ch].mar &= 0x00ff00ff;
			dma[ch].mar |= (data << 8);
			return;
		case 0x0f:
			dma[ch].mar &= 0x00ffff00;
			dma[ch].mar |= data;
			return;

		// DAR
		case 0x14:
			return;
		case 0x15:
			dma[ch].dar &= 0x0000ffff;
			dma[ch].dar |= (data << 16);
			return;
		case 0x16:
			dma[ch].dar &= 0x00ff00ff;
			dma[ch].dar |= (data << 8);
			return;
		case 0x17:
			dma[ch].dar &= 0x00ffff00;
			dma[ch].dar |= data;
			return;

		// BTC
		case 0x1a:
			dma[ch].btc &= 0x00ff;
			dma[ch].btc |= (data << 8);
			return;
		case 0x1b:
			dma[ch].btc &= 0xff00;
			dma[ch].btc |= data;
			return;

		// BAR
		case 0x1c:
			return;
		case 0x1d:
			dma[ch].bar &= 0x0000ffff;
			dma[ch].bar |=(data << 16);
			return;
		case 0x1e:
			dma[ch].bar &= 0x00ff00ff;
			dma[ch].bar |= (data << 8);
			return;
		case 0x1f:
			dma[ch].bar &= 0x00ffff00;
			dma[ch].bar |= data;
			return;

		// NIV
		case 0x25:
			dma[ch].niv = data;
			return;

		// EIV
		case 0x27:
			dma[ch].eiv = data;
			return;

		// MFC
		case 0x29:
			dma[ch].mfc = data;
			return;

		// CPR
		case 0x2d:
			dma[ch].cp = data & 0x03;
			return;

		// DFC
		case 0x31:
			dma[ch].dfc = data;
			return;

		// BFC
		case 0x39:
			dma[ch].bfc = data;
			return;

		// GCR
		case 0x3f:
			if (ch == 3) {
				// チャネル3のみ
				SetGCR(data);
			}
			return;
	}
}

//---------------------------------------------------------------------------
//
//	DCRセット
//
//---------------------------------------------------------------------------
void FASTCALL DMAC::SetDCR(int ch, DWORD data)
{
	ASSERT(this);
	ASSERT((ch >= 0) && (ch <= 3));
	ASSERT(data < 0x100);

	// ACTが上がっていればタイミングエラー
	if (dma[ch].act) {
#if defined(DMAC_LOG)
		LOG1(Log::Normal, "チャネル%d タイミングエラー(SetDCR)", ch);
#endif	// DMAC_LOG
		ErrorDMA(ch, 0x02);
		return;
	}

	// XRM
	dma[ch].xrm = data >> 6;

	// DTYP
	dma[ch].dtyp = (data >> 4) & 0x03;

	// DPS
	if (data & 0x08) {
		dma[ch].dps = TRUE;
	}
	else {
		dma[ch].dps = FALSE;
	}

	// PCL
	dma[ch].pcl = (data & 0x03);

	// 割り込みチェック
	Interrupt();
}

//---------------------------------------------------------------------------
//
//	DCR取得
//
//---------------------------------------------------------------------------
DWORD FASTCALL DMAC::GetDCR(int ch) const
{
	DWORD data;

	ASSERT(this);
	ASSERT((ch >= 0) && (ch <= 3));
	ASSERT(dma[ch].xrm <= 3);
	ASSERT(dma[ch].dtyp <= 3);
	ASSERT(dma[ch].pcl <= 3);

	// データ作成
	data = dma[ch].xrm;
	data <<= 2;
	data |= dma[ch].dtyp;
	data <<= 1;
	if (dma[ch].dps) {
		data |= 0x01;
	}
	data <<= 3;
	data |= dma[ch].pcl;

	return data;
}

//---------------------------------------------------------------------------
//
//	OCRセット
//
//---------------------------------------------------------------------------
void FASTCALL DMAC::SetOCR(int ch, DWORD data)
{
	ASSERT(this);
	ASSERT((ch >= 0) && (ch <= 3));
	ASSERT(data < 0x100);

	// ACTが上がっていればタイミングエラー
	if (dma[ch].act) {
#if defined(DMAC_LOG)
		LOG1(Log::Normal, "チャネル%d タイミングエラー(SetOCR)", ch);
#endif	// DMAC_LOG
		ErrorDMA(ch, 0x02);
		return;
	}

	// DIR
	if (data & 0x80) {
		dma[ch].dir = TRUE;
	}
	else {
		dma[ch].dir = FALSE;
	}

	// BTD
	if (data & 0x40) {
		dma[ch].btd = TRUE;
	}
	else {
		dma[ch].btd = FALSE;
	}

	// SIZE
	dma[ch].size = (data >> 4) & 0x03;

	// CHAIN
	dma[ch].chain = (data >> 2) & 0x03;

	// REQG
	dma[ch].reqg = (data & 0x03);
}

//---------------------------------------------------------------------------
//
//	OCR取得
//
//---------------------------------------------------------------------------
DWORD FASTCALL DMAC::GetOCR(int ch) const
{
	DWORD data;

	ASSERT(this);
	ASSERT((ch >= 0) && (ch <= 3));
	ASSERT(dma[ch].size <= 3);
	ASSERT(dma[ch].chain <= 3);
	ASSERT(dma[ch].reqg <= 3);

	// データ作成
	data = 0;
	if (dma[ch].dir) {
		data |= 0x02;
	}
	if (dma[ch].btd) {
		data |= 0x01;
	}
	data <<= 2;
	data |= dma[ch].size;
	data <<= 2;
	data |= dma[ch].chain;
	data <<= 2;
	data |= dma[ch].reqg;

	return data;
}

//---------------------------------------------------------------------------
//
//	SCRセット
//
//---------------------------------------------------------------------------
void FASTCALL DMAC::SetSCR(int ch, DWORD data)
{
	ASSERT(this);
	ASSERT((ch >= 0) && (ch <= 3));
	ASSERT(data < 0x100);

	// ACTが上がっていればタイミングエラー
	if (dma[ch].act) {
#if defined(DMAC_LOG)
		LOG1(Log::Normal, "チャネル%d タイミングエラー(SetSCR)", ch);
#endif	// DMAC_LOG
		ErrorDMA(ch, 0x02);
		return;
	}

	dma[ch].mac = (data >> 2) & 0x03;
	dma[ch].dac = (data & 0x03);
}

//---------------------------------------------------------------------------
//
//	SCR取得
//
//---------------------------------------------------------------------------
DWORD FASTCALL DMAC::GetSCR(int ch) const
{
	DWORD data;

	ASSERT(this);
	ASSERT((ch >= 0) && (ch <= 3));
	ASSERT(dma[ch].mac <= 3);
	ASSERT(dma[ch].dac <= 3);

	// データ作成
	data = dma[ch].mac;
	data <<= 2;
	data |= dma[ch].dac;

	return data;
}

//---------------------------------------------------------------------------
//
//	CCRセット
//
//---------------------------------------------------------------------------
void FASTCALL DMAC::SetCCR(int ch, DWORD data)
{
	ASSERT(this);
	ASSERT((ch >= 0) && (ch <= 3));
	ASSERT(data < 0x100);

	// INT
	if (data & 0x08) {
		dma[ch].intr = TRUE;
	}
	else {
		dma[ch].intr = FALSE;
	}

	// HLT
	if (data & 0x20) {
		dma[ch].hlt = TRUE;
	}
	else {
		dma[ch].hlt = FALSE;
	}

	// STR
	if (data & 0x80) {
		dma[ch].str = TRUE;
		StartDMA(ch);
	}

	// CNT
	if (data & 0x40) {
		dma[ch].cnt = TRUE;
		ContDMA(ch);
	}

	// SAB
	if (data & 0x10) {
		dma[ch].sab = TRUE;
		AbortDMA(ch);
	}
}

//---------------------------------------------------------------------------
//
//	CCR取得
//
//---------------------------------------------------------------------------
DWORD FASTCALL DMAC::GetCCR(int ch) const
{
	DWORD data;

	ASSERT(this);
	ASSERT((ch >= 0) && (ch <= 3));

	// INT,HLT,STR,CNTだけ返す
	data = 0;
	if (dma[ch].intr) {
		data |= 0x08;
	}
	if (dma[ch].hlt) {
		data |= 0x20;
	}
	if (dma[ch].str) {
		data |= 0x80;
	}
	if (dma[ch].cnt) {
		data |= 0x40;
	}

	return data;
}

//---------------------------------------------------------------------------
//
//	CSRセット
//
//---------------------------------------------------------------------------
void FASTCALL DMAC::SetCSR(int ch, DWORD data)
{
	ASSERT(this);
	ASSERT((ch >= 0) && (ch <= 3));
	ASSERT(data < 0x100);

	// ACT,PCS以外は1を書き込むことによりクリアできる
	if (data & 0x80) {
		dma[ch].coc = FALSE;
	}
	if (data & 0x40) {
		dma[ch].boc = FALSE;
	}
	if (data & 0x20) {
		dma[ch].ndt = FALSE;
	}
	if (data & 0x10) {
		dma[ch].err = FALSE;
	}
	if (data & 0x04) {
		dma[ch].dit = FALSE;
	}
	if (data & 0x02) {
		dma[ch].pct = FALSE;
	}

	// 割り込み処理
	Interrupt();
}

//---------------------------------------------------------------------------
//
//	CSR取得
//
//---------------------------------------------------------------------------
DWORD FASTCALL DMAC::GetCSR(int ch) const
{
	DWORD data;

	ASSERT(this);
	ASSERT((ch >= 0) && (ch <= 3));

	// データ作成
	data = 0;
	if (dma[ch].coc) {
		data |= 0x80;
	}
	if (dma[ch].boc) {
		data |= 0x40;
	}
	if (dma[ch].ndt) {
		data |= 0x20;
	}
	if (dma[ch].err) {
		data |= 0x10;
	}
	if (dma[ch].act) {
		data |= 0x08;
	}
	if (dma[ch].dit) {
		data |= 0x04;
	}
	if (dma[ch].pct) {
		data |= 0x02;
	}
	if (dma[ch].pcs) {
		data |= 0x01;
	}

	return data;
}

//---------------------------------------------------------------------------
//
//	GCR設定
//
//---------------------------------------------------------------------------
void FASTCALL DMAC::SetGCR(DWORD data)
{
	int ch;
	DWORD bt;
	DWORD br;

	ASSERT(this);
	ASSERT(data < 0x100);

	// データ分離
	bt = (data >> 2) & 0x03;
	br = data & 0x03;

	// 全チャネルに設定
	for (ch=0; ch<4; ch++) {
		dma[ch].bt = bt;
		dma[ch].br = br;
	}
}

//---------------------------------------------------------------------------
//
//	DMAリセット
//
//---------------------------------------------------------------------------
void FASTCALL DMAC::ResetDMA(int ch)
{
	ASSERT(this);
	ASSERT((ch >= 0) && (ch <= 3));

	// GCR初期化
	dma[ch].bt = 0;
	dma[ch].br = 0;

	// DCR初期化
	dma[ch].xrm = 0;
	dma[ch].dtyp = 0;
	dma[ch].dps = FALSE;
	dma[ch].pcl = 0;

	// OCR初期化
	dma[ch].dir = FALSE;
	dma[ch].btd = FALSE;
	dma[ch].size = 0;
	dma[ch].chain = 0;
	dma[ch].reqg = 0;

	// SCR初期化
	dma[ch].mac = 0;
	dma[ch].dac = 0;

	// CCR初期化
	dma[ch].str = FALSE;
	dma[ch].cnt = FALSE;
	dma[ch].hlt = FALSE;
	dma[ch].sab = FALSE;
	dma[ch].intr = FALSE;

	// CSR初期化
	dma[ch].coc = FALSE;
	dma[ch].boc = FALSE;
	dma[ch].ndt = FALSE;
	dma[ch].err = FALSE;
	dma[ch].act = FALSE;
	dma[ch].dit = FALSE;
	dma[ch].pct = FALSE;
	if (ch == 0) {
		// FDCは'L'
		dma[ch].pcs = FALSE;
	}
	else {
		// それ以外は'H'
		dma[ch].pcs = TRUE;
	}

	// CPR初期化
	dma[ch].cp = 0;

	// CER初期化
	dma[ch].ecode = 0;

	// 割り込みベクタ初期化
	dma[ch].niv = 0x0f;
	dma[ch].eiv = 0x0f;

	// アドレス及びカウンタはリセットしない(データシートによる)
	dma[ch].mar &= 0x00ffffff;
	dma[ch].dar &= 0x00ffffff;
	dma[ch].bar &= 0x00ffffff;
	dma[ch].mtc &= 0x0000ffff;
	dma[ch].btc &= 0x0000ffff;

	// 転送タイプ、カウンタ初期化
	dma[ch].type = 0;
	dma[ch].startcnt = 0;
	dma[ch].errorcnt = 0;
}

//---------------------------------------------------------------------------
//
//	DMAスタート
//
//---------------------------------------------------------------------------
void FASTCALL DMAC::StartDMA(int ch)
{
	int c;

	ASSERT(this);
	ASSERT((ch >= 0) && (ch <= 3));

#if defined(DMAC_LOG)
	LOG1(Log::Normal, "チャネル%d スタート", ch);
#endif	// DMAC_LOG

	// ACT,COC,BOC,NDT,ERRが上がっていればタイミングエラー
	if (dma[ch].act || dma[ch].coc || dma[ch].boc || dma[ch].ndt || dma[ch].err) {
#if defined(DMAC_LOG)
		if (dma[ch].act) {
			LOG1(Log::Normal, "チャネル%d タイミングエラー (ACT)", ch);
		}
		if (dma[ch].coc) {
			LOG1(Log::Normal, "チャネル%d タイミングエラー (COC)", ch);
		}
		if (dma[ch].boc) {
			LOG1(Log::Normal, "チャネル%d タイミングエラー (BOC)", ch);
		}
		if (dma[ch].ndt) {
			LOG1(Log::Normal, "チャネル%d タイミングエラー (NDT)", ch);
		}
		if (dma[ch].err) {
			LOG1(Log::Normal, "チャネル%d タイミングエラー (ERR)", ch);
		}
#endif	// DMAC_LOG
		ErrorDMA(ch, 0x02);
		return;
	}

	// チェインなしの場合は、MTC=0ならメモリカウントエラー
	if (dma[ch].chain == 0) {
		if (dma[ch].mtc == 0) {
#if defined(DMAC_LOG)
			LOG1(Log::Normal, "チャネル%d メモリカウントエラー", ch);
#endif	// DMAC_LOG
			ErrorDMA(ch, 0x0d);
			return;
		}
	}

	// アレイチェインの場合は、BTC=0ならベースカウントエラー
	if (dma[ch].chain == 0x02) {
		if (dma[ch].btc == 0) {
#if defined(DMAC_LOG)
			LOG1(Log::Normal, "チャネル%d ベースカウントエラー", ch);
#endif	// DMAC_LOG
			ErrorDMA(ch, 0x0f);
			return;
		}
	}

	// コンフィギュレーションエラーチェック
	if ((dma[ch].xrm == 0x01) || (dma[ch].mac == 0x03) || (dma[ch].dac == 0x03)
			|| (dma[ch].chain == 0x01)) {
#if defined(DMAC_LOG)
		LOG1(Log::Normal, "チャネル%d コンフィグエラー", ch);
#endif	// DMAC_LOG
		ErrorDMA(ch, 0x01);
		return;
	}

	// 転送タイプ作成
	dma[ch].type = 0;
	if (dma[ch].dps) {
		dma[ch].type += 4;
	}
	dma[ch].type += dma[ch].size;

	// ワーク初期化
	dma[ch].str = FALSE;
	dma[ch].act = TRUE;
	dma[ch].cnt = FALSE;
	dma[ch].sab = FALSE;

	// カウントアップ
	dma[ch].startcnt++;

	// アレイチェインまたはリンクアレイチェインは、最初のブロックをロード
	if (dma[ch].chain != 0) {
		LoadDMA(ch);
		// ロード時にアドレスエラーまたはバスエラーが起きたら、エラーフラグが上がる
		if (dma[ch].err) {
			return;
		}
	}

	// CPUサイクルをクリアして、モード別
	dmactrl.cpu_cycle = 0;
	switch (dma[ch].reqg) {
		// オートリクエスト限定
		case 0:
		// オートリクエスト最大
		case 1:
			// 現在の残りだけDMAを動かして、CPUを止める
			dmactrl.current_ch = ch;
			dmactrl.cpu_cycle = 0;
			dmactrl.exec = TRUE;
			scheduler->dma_active = TRUE;
			c = AutoDMA(cpu->GetIOCycle());
			if (c == 0) {
				cpu->Release();
			}
			break;

		// 外部要求転送
		case 2:
			break;

		// オートリクエスト＋外部要求転送
		case 3:
			// 現在の残りだけDMAを動かして、CPUを止める
			dmactrl.current_ch = ch;
			dmactrl.cpu_cycle = 0;
			dmactrl.exec = TRUE;
			scheduler->dma_active= TRUE;
			dma[ch].reqg = 1;
			c = AutoDMA(cpu->GetIOCycle());
			if (c == 0) {
				cpu->Release();
			}
			dma[ch].reqg = 3;
			break;

		default:
			ASSERT(FALSE);
	}
}

//---------------------------------------------------------------------------
//
//	DMAコンティニュー
//
//---------------------------------------------------------------------------
void FASTCALL DMAC::ContDMA(int ch)
{
	ASSERT(this);
	ASSERT((ch >= 0) && (ch <= 3));

#if defined(DMAC_LOG)
	LOG1(Log::Normal, "チャネル%d コンティニュー", ch);
#endif	// DMAC_LOG

	// ACTが上がっていないと動作タイミングエラー
	if (!dma[ch].act) {
#if defined(DMAC_LOG)
		LOG1(Log::Normal, "チャネル%d 動作タイミングエラー(Cont)", ch);
#endif	// DMAC_LOG
		ErrorDMA(ch, 0x02);
		return;
}
	// チェインモードの場合はコンフィグエラー
	if (dma[ch].chain != 0) {
#if defined(DMAC_LOG)
		LOG1(Log::Normal, "チャネル%d コンフィグエラー", ch);
#endif	// DMAC_LOG
		ErrorDMA(ch, 0x01);
	}
}

//---------------------------------------------------------------------------
//
//	DMAソフトウェアアボート
//
//---------------------------------------------------------------------------
void FASTCALL DMAC::AbortDMA(int ch)
{
	ASSERT(this);
	ASSERT((ch >= 0) && (ch <= 3));

	// 非アクティブならエラー処理を行わない(Marianne.pan)
	if (!dma[ch].act) {
		// さらにCOCを落とす(バラデューク)
		dma[ch].coc = FALSE;
		return;
	}

#if defined(DMAC_LOG)
	LOG1(Log::Normal, "チャネル%d ソフトウェアアボート", ch);
#endif	// DMAC_LOG

	// 転送完了、非アクティブ
	dma[ch].coc = TRUE;
	dma[ch].act = FALSE;

	// ソフトウェアアボートでエラー発生
	ErrorDMA(ch, 0x11);
}

//---------------------------------------------------------------------------
//
//	DMA次ブロックのロード
//
//---------------------------------------------------------------------------
void FASTCALL DMAC::LoadDMA(int ch)
{
	DWORD base;

	ASSERT(this);
	ASSERT((ch >= 0) && (ch <= 3));
	ASSERT(dmactrl.load == 0);

	// ロード中(ReadWordでのアドレスエラー、バスエラーに注意)
	dmactrl.load = (ch + 1);

	if (dma[ch].bar & 1) {
		// BARアドレスエラー
		AddrErr(dma[ch].bar, TRUE);

		dmactrl.load = 0;
		return;
	}

	// MAR読み込み
	dma[ch].bar &= 0xfffffe;
	dma[ch].mar = (memory->ReadWord(dma[ch].bar) & 0x00ff);
	dma[ch].bar += 2;
	dma[ch].mar <<= 16;
	dma[ch].bar &= 0xfffffe;
	dma[ch].mar |= (memory->ReadWord(dma[ch].bar) & 0xffff);
	dma[ch].bar += 2;

	// MTC読み込み
	dma[ch].bar &= 0xfffffe;
	dma[ch].mtc = (memory->ReadWord(dma[ch].bar) & 0xffff);
	dma[ch].bar += 2;

	if (dma[ch].err) {
		// MAR,MTC読み込みエラー
		dmactrl.load = 0;
		return;
	}

	// アレイチェインではここまで
	if (dma[ch].chain == 0x02) {
#if defined(DMAC_LOG)
		LOG1(Log::Normal, "チャネル%d アレイチェイン次ブロック", ch);
#endif	// DMAC_LOG
		dma[ch].btc--;
		dmactrl.load = 0;
		return;
	}

	// リンクアレイチェイン(では次のリンクアドレスをBARへロード
#if defined(DMAC_LOG)
	LOG1(Log::Normal, "チャネル%d リンクアレイチェイン次ブロック", ch);
#endif	// DMAC_LOG
	dma[ch].bar &= 0xfffffe;
	base = (memory->ReadWord(dma[ch].bar) & 0x00ff);
	dma[ch].bar += 2;
	base <<= 16;
	dma[ch].bar &= 0xfffffe;
	base |= (memory->ReadWord(dma[ch].bar) & 0xffff);
	dma[ch].bar = base;

	// ロード終了
	dmactrl.load = 0;
}

//---------------------------------------------------------------------------
//
//	DMAエラー
//
//---------------------------------------------------------------------------
void FASTCALL DMAC::ErrorDMA(int ch, DWORD code)
{
	ASSERT(this);
	ASSERT((ch >= 0) && (ch <= 3));
	ASSERT((code >= 0x01) && (code <= 17));

#if defined(DMAC_LOG)
	LOG2(Log::Normal, "チャネル%d エラー発生$%02X", ch, code);
#endif	// DMAC_LOG

	// ACTを降ろす(ファランクス ADPCM)
	dma[ch].act = FALSE;

	// エラーコードを書き込む
	dma[ch].ecode = code;

	// エラーフラグを立てる
	dma[ch].err = TRUE;

	// カウントアップ
	dma[ch].errorcnt++;

	// 割り込み処理
	Interrupt();
}

//---------------------------------------------------------------------------
//
//	DMA割り込み
//
//---------------------------------------------------------------------------
void FASTCALL DMAC::Interrupt()
{
	DWORD cp;
	int ch;

	ASSERT(this);

	// DMAと同じ優先度で処理(データシートより)
	for (cp=0; cp<=3; cp++) {
		for (ch=0; ch<=3; ch++) {
			// CPチェック
			if (cp != dma[ch].cp) {
				continue;
			}

			// インタラプトイネーブルをチェック
			if (!dma[ch].intr) {
				continue;
			}

			// ERRによりEIVで出力
			if (dma[ch].err) {
				if (dmactrl.vector != (int)dma[ch].eiv) {
					// 別の割り込みを要求していれば、一旦キャンセル
					if (dmactrl.vector >= 0) {
						cpu->IntCancel(3);
					}
#if defined(DMAC_LOG)
					LOG1(Log::Normal, "チャネル%d エラー割り込み", ch);
#endif	// DMAC_LOG
					cpu->Interrupt(3, (BYTE)dma[ch].eiv);
					dmactrl.vector = (int)dma[ch].eiv;
				}
				return;
			}

			// COC,BOC,NDT,PCT(割り込みラインの場合)にNIVで出力
			if (dma[ch].coc || dma[ch].boc || dma[ch].ndt) {
				if (dmactrl.vector != (int)dma[ch].niv) {
					// 別の割り込みを要求していれば、一旦キャンセル
					if (dmactrl.vector >= 0) {
						cpu->IntCancel(3);
					}
#if defined(DMAC_LOG)
					LOG1(Log::Normal, "チャネル%d 通常割り込み", ch);
#endif	// DMAC_LOG
					cpu->Interrupt(3, (BYTE)dma[ch].niv);
					dmactrl.vector = (int)dma[ch].niv;
				}
				return;
			}

			if ((dma[ch].pcl == 0x01) && dma[ch].pct) {
				if (dmactrl.vector != (int)dma[ch].niv) {
					// 別の割り込みを要求していれば、一旦キャンセル
					if (dmactrl.vector >= 0) {
						cpu->IntCancel(3);
					}
#if defined(DMAC_LOG)
					LOG1(Log::Normal, "チャネル%d PCL割り込み", ch);
#endif	// DMAC_LOG
					cpu->Interrupt(3, (BYTE)dma[ch].niv);
					dmactrl.vector = (int)dma[ch].niv;
				}
				return;
			}
		}
	}

	// 要求中の割り込みはない
	if (dmactrl.vector >= 0) {
#if defined(DMAC_LOG)
		LOG1(Log::Normal, "割り込みキャンセル ベクタ$%02X", dmactrl.vector);
#endif	// DMAC_LOG

		cpu->IntCancel(3);
		dmactrl.vector = -1;
	}
}

//---------------------------------------------------------------------------
//
//	割り込みACK
//
//---------------------------------------------------------------------------
void FASTCALL DMAC::IntAck()
{
	ASSERT(this);

	// リセット直後に、CPUから割り込みが間違って入る場合がある
	if (dmactrl.vector < 0) {
		LOG0(Log::Warning, "要求していない割り込み");
		return;
	}

#if defined(DMAC_LOG)
	LOG1(Log::Normal, "割り込み応答 ベクタ$%02X", dmactrl.vector);
#endif	// DMAC_LOG

	// 要求中ベクタなし
	dmactrl.vector = -1;
}

//---------------------------------------------------------------------------
//
//	DMA情報取得
//
//---------------------------------------------------------------------------
void FASTCALL DMAC::GetDMA(int ch, dma_t *buffer) const
{
	ASSERT(this);
	ASSERT((ch >= 0) && (ch <= 3));
	ASSERT(buffer);

	// チャネルワークをコピー
	*buffer = dma[ch];
}

//---------------------------------------------------------------------------
//
//	DMA制御情報取得
//
//---------------------------------------------------------------------------
void FASTCALL DMAC::GetDMACtrl(dmactrl_t *buffer) const
{
	ASSERT(this);
	ASSERT(buffer);

	// 制御ワークをコピー
	*buffer = dmactrl;
}

//---------------------------------------------------------------------------
//
//	DMA転送中か
//
//---------------------------------------------------------------------------
BOOL FASTCALL DMAC::IsDMA() const
{
	ASSERT(this);

	// 転送中フラグ(チャネル兼用)と、ロードフラグを見る
	if ((dmactrl.transfer == 0) && (dmactrl.load == 0)) {
		return FALSE;
	}

	// どちらかが動いている
	return TRUE;
}

//---------------------------------------------------------------------------
//
//	DMA転送可能か
//
//---------------------------------------------------------------------------
BOOL FASTCALL DMAC::IsAct(int ch) const
{
	ASSERT(this);
	ASSERT((ch >= 0) && (ch <= 3));

	// ACTでないか、ERRか、HLTなら転送できない
	if (!dma[ch].act || dma[ch].err || dma[ch].hlt) {
		return FALSE;
	}

	// 転送できる
	return TRUE;
}

//---------------------------------------------------------------------------
//
//	DMAバスエラー
//
//---------------------------------------------------------------------------
void FASTCALL DMAC::BusErr(DWORD addr, BOOL read)
{
	ASSERT(this);
	ASSERT(addr <= 0xffffff);

	if (read) {
		LOG1(Log::Warning, "DMAバスエラー(読み込み) $%06X", addr);
	}
	else {
		LOG1(Log::Warning, "DMAバスエラー(書き込み) $%06X", addr);
	}

	// ロード中のエラーか
	if (dmactrl.load != 0) {
		// メモリ・デバイス・ベースの区別は考慮していない
		ASSERT((dmactrl.load >= 1) && (dmactrl.load <= 4));
		ErrorDMA(dmactrl.load - 1, 0x08);
		return;
	}

	// メモリ・デバイス・ベースの区別は考慮していない
	ASSERT((dmactrl.transfer >= 1) && (dmactrl.transfer <= 4));
	ErrorDMA(dmactrl.transfer - 1, 0x08);
}

//---------------------------------------------------------------------------
//
//	DMAアドレスエラー
//
//---------------------------------------------------------------------------
void FASTCALL DMAC::AddrErr(DWORD addr, BOOL read)
{
	ASSERT(this);
	ASSERT(addr <= 0xffffff);

	if (read) {
		LOG1(Log::Warning, "DMAアドレスエラー(読み込み) $%06X", addr);
	}
	else {
		LOG1(Log::Warning, "DMAアドレスエラー(書き込み) $%06X", addr);
	}

	// ロード中のエラーか
	if (dmactrl.load != 0) {
		// メモリ・デバイス・ベースの区別は考慮していない
		ASSERT((dmactrl.load >= 1) && (dmactrl.load <= 4));
		ErrorDMA(dmactrl.load - 1, 0x0c);
		return;
	}

	// メモリ・デバイス・ベースの区別は考慮していない
	ASSERT((dmactrl.transfer >= 1) && (dmactrl.transfer <= 4));
	ErrorDMA(dmactrl.transfer - 1, 0x0c);
}

//---------------------------------------------------------------------------
//
//	ベクタ取得
//
//---------------------------------------------------------------------------
DWORD FASTCALL DMAC::GetVector(int type) const
{
	ASSERT(this);
	ASSERT((type >= 0) && (type < 8));

	// ノーマル・エラーのベクタを交互に出力
	if (type & 1) {
		// エラー
		return dma[type >> 1].eiv;
	}
	else {
		// ノーマル
		return dma[type >> 1].niv;
	}
}

//---------------------------------------------------------------------------
//
//	DMA外部リクエスト
//
//---------------------------------------------------------------------------
BOOL FASTCALL DMAC::ReqDMA(int ch)
{
	ASSERT(this);
	ASSERT((ch >= 0) && (ch <= 3));

	// ACTでないか、ERRか、HLTなら何もしない
	if (!dma[ch].act || dma[ch].err || dma[ch].hlt) {
#if defined(DMAC_LOG)
		LOG1(Log::Normal, "チャネル%d 外部リクエスト失敗", ch);
#endif	// DMAC_LOG
		return FALSE;
	}

	// DMA転送
	TransDMA(ch);
	return TRUE;
}

//---------------------------------------------------------------------------
//
//	オートリクエスト
//
//---------------------------------------------------------------------------
DWORD FASTCALL DMAC::AutoDMA(DWORD cycle)
{
	int i;
	int ch;
	int mul;
	int remain;
	int used;
	DWORD backup;
	BOOL flag;

	ASSERT(this);

	// パラメータ記憶
	remain = (int)cycle;

	// 実行フラグが上がっていなければオートリクエストは無し
	if (!dmactrl.exec) {
		return cycle;
	}

	// 実行継続フラグをリセット
	flag = FALSE;

	// 最大速度オートリクエストのチャネルを先に処理
	for (i=0; i<4; i++) {
		// 見るべきチャネルを決定
		ch = (dmactrl.current_ch + i) & 3;

		// ACT, ERR, HLTのチェック
		if (!dma[ch].act || dma[ch].err || dma[ch].hlt) {
			continue;
		}

		// 最大速度オートリクエストか
		if (dma[ch].reqg != 1) {
			continue;
		}

		// 加算して、最低でも10サイクルは必要。
		dmactrl.cpu_cycle += cycle;
		if (dmactrl.cpu_cycle < 10) {
			// CPUは実行できない。DMA継続
			return 0;
		}

		// 2回以上加算させない、フラグUP
		cycle = 0;
		flag = TRUE;

		// スケジューラのcycle(オーバーサイクル計算)を保持し、リセット
		backup = scheduler->GetCPUCycle();
		scheduler->SetCPUCycle(0);

		// cpu_cycleがマイナスになるまで実行。消費時間はスケジューラより得る
		while (scheduler->GetCPUCycle() < dmactrl.cpu_cycle) {
			// ACT, ERR, HLTのチェック
			if (!dma[ch].act || dma[ch].err || dma[ch].hlt) {
				break;
			}

			// scheulder->GetCPUCycle()を使い、DMAC消費サイクル数を取得する
			TransDMA(ch);
		}

		// 消費サイクルを削り、復元
		dmactrl.cpu_cycle -= scheduler->GetCPUCycle();
		remain -= scheduler->GetCPUCycle();
		scheduler->SetCPUCycle(backup);

		// チャネルを次へ(ラウンドロビン)
		dmactrl.current_ch = (dmactrl.current_ch + 1) & 3;

		// すべて時間を使い切ったか
		if (dmactrl.cpu_cycle <= 0) {
			// CPUは実行できない
			// ここで全チャネルを完了した場合、次のAudoDMAでフラグ落とす
			return 0;
		}
	}

	// 最大速度オートリクエストがなかったか、完了して時間が余った
	// 限定速度オートリクエストのチャネルを処理
	for (i=0; i<4; i++) {
		// 見るべきチャネルを決定
		ch = (dmactrl.current_ch + i) & 3;

		// ACT, ERR, HLTのチェック
		if (!dma[ch].act || dma[ch].err || dma[ch].hlt) {
			continue;
		}

		// 最大速度オートリクエストはありえない(上の部分で必ず処理)
		ASSERT(dma[ch].reqg != 1);

		// 限定速度オートリクエストか
		if (dma[ch].reqg != 0) {
			continue;
		}

		// 加算して、最低でも10サイクルは必要。
		dmactrl.cpu_cycle += cycle;
		if (dmactrl.cpu_cycle < 10) {
			// CPUは実行できない。DMA継続
			return 0;
		}

		// 2回以上加算させない、フラグUP
		cycle = 0;
		flag = TRUE;

		// スケジューラのcycle(オーバーサイクル計算)を保持し、リセット
		backup = scheduler->GetCPUCycle();
		scheduler->SetCPUCycle(0);

		// バス占有率倍率を計算(BT=0なら2倍など)
		mul = (dma[ch].bt + 1);

		// cpu_cycleがバス占有率を考慮した値を超えているか
		while ((scheduler->GetCPUCycle() << mul) < dmactrl.cpu_cycle) {
			// ACT, ERR, HLTのチェック
			if (!dma[ch].act || dma[ch].err || dma[ch].hlt) {
				break;
			}

			// 転送
			TransDMA(ch);
		}

		// 使用サイクルを記憶(後で使うため)
		used = scheduler->GetCPUCycle();
		scheduler->SetCPUCycle(backup);

		// チャネルを次へ(ラウンドロビン)
		dmactrl.current_ch = (dmactrl.current_ch + 1) & 3;

		// ここで終了か
		if (dmactrl.cpu_cycle < (used << mul)) {
			// 予定されていたバス占有率を使い切った。残りはCPUに返却
			dmactrl.cpu_cycle -= used;
			if (used < remain) {
				// 十分余りがある
				return (remain - used);
			}
			else {
				// なぜか、使いすぎた。CPUは0
				return 0;
			}
		}

		// まだバスの使用が許されるので、他チャネルをまわる
		remain -= used;
	}

	if (!flag) {
		// DMAは使わなかった。dmactrl.execを降ろす
		dmactrl.exec = FALSE;
		scheduler->dma_active = FALSE;
	}

	return cycle;
}

//---------------------------------------------------------------------------
//
//	DMA1回転送
//
//---------------------------------------------------------------------------
BOOL FASTCALL DMAC::TransDMA(int ch)
{
	DWORD data;

	ASSERT(this);
	ASSERT((ch >= 0) && (ch <= 3));
	ASSERT(dmactrl.transfer == 0);

	// 転送フラグON
	dmactrl.transfer = ch + 1;

	// タイプ、ディレクションに応じて転送
	switch (dma[ch].type) {
		// 8bit, Packバイト, 8bitポート
		case 0:
		// 8bit, Unpackバイト, 8bitポート
		case 3:
		// 8bit, Unpackバイト, 16bitポート
		case 7:
			// SCSIディスク ベンチマーク(dskbench.x)より
			if (dma[ch].dir) {
				memory->WriteByte(dma[ch].mar, (BYTE)(memory->ReadByte(dma[ch].dar)));
				scheduler->Wait(11);
			}
			else {
				memory->WriteByte(dma[ch].dar, (BYTE)(memory->ReadByte(dma[ch].mar)));
				scheduler->Wait(11);
			}
			break;

		// 8bit, Packバイト, 16bitポート(Unpackより速くする:パロディウスだ!)
		// Wait12:パロディウスだ!、Wait?:Moon Fighter
		case 4:
			if (dma[ch].dir) {
				memory->WriteByte(dma[ch].mar, (BYTE)(memory->ReadByte(dma[ch].dar)));
				scheduler->Wait(10);
			}
			else {
				memory->WriteByte(dma[ch].dar, (BYTE)(memory->ReadByte(dma[ch].mar)));
				scheduler->Wait(10);
			}
			break;

		// 8bit, ワード
		case 1:
			if (dma[ch].dir) {
				data = (BYTE)(memory->ReadByte(dma[ch].dar));
				data <<= 8;
				data |= (BYTE)(memory->ReadByte(dma[ch].dar + 2));
				memory->WriteWord(dma[ch].mar, data);
				scheduler->Wait(19);
			}
			else {
				data = memory->ReadWord(dma[ch].mar);
				memory->WriteByte(dma[ch].dar, (BYTE)(data >> 8));
				memory->WriteByte(dma[ch].dar + 2, (BYTE)data);
				scheduler->Wait(19);
			}
			break;

		// 8bit, ロングワード
		case 2:
			if (dma[ch].dir) {
				data = (BYTE)(memory->ReadByte(dma[ch].dar));
				data <<= 8;
				data |= (BYTE)(memory->ReadByte(dma[ch].dar + 2));
				data <<= 8;
				data |= (BYTE)(memory->ReadByte(dma[ch].dar + 4));
				data <<= 8;
				data |= (BYTE)(memory->ReadByte(dma[ch].dar + 6));
				memory->WriteWord(dma[ch].mar, (WORD)(data >> 16));
				memory->WriteWord(dma[ch].mar + 2, (WORD)data);
				scheduler->Wait(38);
			}
			else {
				data = memory->ReadWord(dma[ch].mar);
				data <<= 16;
				data |= (WORD)(memory->ReadWord(dma[ch].mar + 2));
				memory->WriteByte(dma[ch].dar, (BYTE)(data >> 24));
				memory->WriteByte(dma[ch].dar + 2, (BYTE)(data >> 16));
				memory->WriteByte(dma[ch].dar + 4, (BYTE)(data >> 8));
				memory->WriteByte(dma[ch].dar + 6, (BYTE)data);
				scheduler->Wait(38);
			}
			break;

		// 16bit, ワード
		case 5:
			// あまり遅くするとFM音源割り込みがひきずられる(グラディウスII)
			if (dma[ch].dir) {
				data = memory->ReadWord(dma[ch].dar);
				memory->WriteWord(dma[ch].mar, (WORD)data);
				scheduler->Wait(10);
			}
			else {
				data = memory->ReadWord(dma[ch].mar);
				memory->WriteWord(dma[ch].dar, (WORD)data);
				scheduler->Wait(10);
			}
			break;

		// 16bit, ロングワード
		case 6:
			if (dma[ch].dir) {
				data = memory->ReadWord(dma[ch].dar);
				data <<= 16;
				data |= (WORD)(memory->ReadWord(dma[ch].dar + 2));
				memory->WriteWord(dma[ch].mar, (WORD)(data >> 16));
				memory->WriteWord(dma[ch].mar + 2, (WORD)data);
				scheduler->Wait(20);
			}
			else {
				data = memory->ReadWord(dma[ch].mar);
				data <<= 16;
				data |= (WORD)memory->ReadWord(dma[ch].mar + 2);
				memory->WriteWord(dma[ch].dar, (WORD)(data >> 16));
				memory->WriteWord(dma[ch].dar + 2, (WORD)data);
				scheduler->Wait(20);
			}
			break;

		// それ以外
		default:
			ASSERT(FALSE);
	}

	// 転送フラグOFF
	dmactrl.transfer = 0;

	// 転送エラーのチェック(バスエラー及びアドレスエラー)
	if (dma[ch].err) {
		// アドレス更新前に抜ける(データシートによる)
		return FALSE;
	}

	// アドレス更新(12bitに制限:Racing Champ)
	dma[ch].mar += MemDiffTable[ dma[ch].type ][ dma[ch].mac ];
	dma[ch].mar &= 0xffffff;
	dma[ch].dar += DevDiffTable[ dma[ch].type ][ dma[ch].dac ];
	dma[ch].dar &= 0xffffff;

	// メモリカウントをデクリメント
	dma[ch].mtc--;
	if (dma[ch].mtc > 0) {
		// 終わる直前にDONEをアサート→FDCのみTCを設定(DCII)
		if ((ch == 0) && (dma[ch].mtc == 1)) {
			fdc->SetTC();
		}
		return TRUE;
	}

	// コンティニューの処理
	if (dma[ch].cnt) {
#if defined(DMAC_LOG)
		LOG1(Log::Normal, "チャネル%d コンティニュー次ブロック", ch);
#endif	// DMAC_LOG

		// BAR,BFC,BTCをMAR,MFC,MTCに転送
		dma[ch].mar = dma[ch].bar;
		dma[ch].mfc = dma[ch].bfc;
		dma[ch].mtc = dma[ch].btc;

		// BOCを上げる
		dma[ch].boc = TRUE;
		Interrupt();
		return TRUE;
	}

	// アレイチェインの処理
	if (dma[ch].chain == 0x02) {
		if (dma[ch].btc > 0) {
			// 次のブロックがある
			LoadDMA(ch);
			return TRUE;
		}
	}

	// リンクアレイチェインの処理
	if (dma[ch].chain == 0x03) {
		if (dma[ch].bar != 0) {
			// 次のブロックがある
			LoadDMA(ch);
			return TRUE;
		}
	}

	// DMA完了
#if defined(DMAC_LOG)
	LOG1(Log::Normal, "チャネル%d DMA完了", ch);
#endif	// DMAC_LOG

	// フラグ設定、割り込み
	dma[ch].act = FALSE;
	dma[ch].coc = TRUE;
	Interrupt();

	return FALSE;
}

//---------------------------------------------------------------------------
//
//	メモリアドレス更新テーブル
//
//---------------------------------------------------------------------------
const int DMAC::MemDiffTable[8][4] = {
	{ 0, 1, -1, 0},		// 8bit, バイト
	{ 0, 2, -2, 0},		// 8bit, ワード
	{ 0, 4, -4, 0},		// 8bit, ロングワード
	{ 0, 1, -1, 0},		// 8bit, パックバイト
	{ 0, 1, -1, 0},		// 16bit, バイト
	{ 0, 2, -2, 0},		// 16bit, ワード
	{ 0, 4, -4, 0},		// 16bit, ロングワード
	{ 0, 1, -1, 0}		// 16bit, パックバイト
};

//---------------------------------------------------------------------------
//
//	デバイスアドレス更新テーブル
//
//---------------------------------------------------------------------------
const int DMAC::DevDiffTable[8][4] = {
	{ 0, 2, -2, 0},		// 8bit, バイト
	{ 0, 4, -4, 0},		// 8bit, ワード
	{ 0, 8, -8, 0},		// 8bit, ロングワード
	{ 0, 2, -2, 0},		// 8bit, パックバイト
	{ 0, 1, -1, 0},		// 16bit, バイト
	{ 0, 2, -2, 0},		// 16bit, ワード
	{ 0, 4, -4, 0},		// 16bit, ロングワード
	{ 0, 1, -1, 0}		// 16bit, パックバイト
};
