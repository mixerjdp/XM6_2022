//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 ＰＩ．(ytanaka@ipc-tokai.or.jp)
//	[ SCC(Z8530) ]
//
//---------------------------------------------------------------------------

#include "os.h"
#include "xm6.h"
#include "vm.h"
#include "cpu.h"
#include "mouse.h"
#include "log.h"
#include "schedule.h"
#include "config.h"
#include "fileio.h"
#include "scc.h"

//===========================================================================
//
//	SCC
//
//===========================================================================
//#define SCC_LOG

//---------------------------------------------------------------------------
//
//	コンストラクタ
//
//---------------------------------------------------------------------------
SCC::SCC(VM *p) : MemDevice(p)
{
	// デバイスIDを初期化
	dev.id = MAKEID('S', 'C', 'C', ' ');
	dev.desc = "SCC (Z8530)";

	// 開始アドレス、終了アドレス
	memdev.first = 0xe98000;
	memdev.last = 0xe99fff;
}

//---------------------------------------------------------------------------
//
//	初期化
//
//---------------------------------------------------------------------------
BOOL FASTCALL SCC::Init()
{
	ASSERT(this);

	// 基本クラス
	if (!MemDevice::Init()) {
		return FALSE;
	}

	// ワーククリア(ハードウェアリセットでも一部状態は残る)
	memset(&scc, 0, sizeof(scc));
	scc.ch[0].index = 0;
	scc.ch[1].index = 1;
	scc.ireq = -1;
	scc.vector = -1;
	clkup = FALSE;

	// マウス取得
	mouse = (Mouse*)vm->SearchDevice(MAKEID('M', 'O', 'U', 'S'));
	ASSERT(mouse);

	// イベント追加
	event[0].SetDevice(this);
	event[0].SetDesc("Channel-A");
	event[0].SetUser(0);
	event[0].SetTime(0);
	scheduler->AddEvent(&event[0]);
	event[1].SetDevice(this);
	event[1].SetDesc("Channel-B");
	event[1].SetUser(1);
	event[1].SetTime(0);
	scheduler->AddEvent(&event[1]);

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	クリーンアップ
//
//---------------------------------------------------------------------------
void FASTCALL SCC::Cleanup()
{
	// 基本クラスへ
	MemDevice::Cleanup();
}

//---------------------------------------------------------------------------
//
//	リセット
//
//---------------------------------------------------------------------------
void FASTCALL SCC::Reset()
{
	int i;

	ASSERT(this);
	LOG0(Log::Normal, "リセット");

	// RESET信号によるリセット(チャネルリセットとは別)
	ResetSCC(&scc.ch[0]);
	ResetSCC(&scc.ch[1]);
	ResetSCC(NULL);

	// 速度
	for (i=0; i<2; i++ ){
		scc.ch[i].brgen = FALSE;
		scc.ch[i].speed = 0;
		event[i].SetTime(0);
	}
}

//---------------------------------------------------------------------------
//
//	セーブ
//
//---------------------------------------------------------------------------
BOOL FASTCALL SCC::Save(Fileio *fio, int ver)
{
	size_t sz;

	ASSERT(this);
	LOG0(Log::Normal, "セーブ");

	// サイズをセーブ
	sz = sizeof(scc_t);
	if (!fio->Write(&sz, sizeof(sz))) {
		return FALSE;
	}
	if (!fio->Write(&scc, sizeof(scc))) {
		return FALSE;
	}

	// イベントをセーブ
	if (!event[0].Save(fio, ver)) {
		return FALSE;
	}
	if (!event[1].Save(fio, ver)) {
		return FALSE;
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	ロード
//
//---------------------------------------------------------------------------
BOOL FASTCALL SCC::Load(Fileio *fio, int ver)
{
	size_t sz;

	ASSERT(this);
	LOG0(Log::Normal, "ロード");

	// 本体をロード
	if (!fio->Read(&sz, sizeof(sz))) {
		return FALSE;
	}
	if (sz != sizeof(scc_t)) {
		return FALSE;
	}
	if (!fio->Read(&scc, sizeof(scc))) {
		return FALSE;
	}

	// イベント
	if (!event[0].Load(fio, ver)) {
		return FALSE;
	}
	if (!event[1].Load(fio, ver)) {
		return FALSE;
	}

	// 速度再計算
	ClockSCC(&scc.ch[0]);
	ClockSCC(&scc.ch[1]);

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	設定適用
//
//---------------------------------------------------------------------------
void FASTCALL SCC::ApplyCfg(const Config *config)
{
	ASSERT(this);
	ASSERT(config);
	LOG0(Log::Normal, "設定適用");

	// SCCクロック
	if (clkup != config->scc_clkup) {
		// 違っているので設定して
		clkup = config->scc_clkup;

		// 速度再計算
		ClockSCC(&scc.ch[0]);
		ClockSCC(&scc.ch[1]);
	}
}

//---------------------------------------------------------------------------
//
//	バイト読み込み
//
//---------------------------------------------------------------------------
DWORD FASTCALL SCC::ReadByte(DWORD addr)
{
	static const DWORD table[] = {
		0, 1, 2, 3, 0, 1, 2, 3, 8, 13, 10, 15, 12, 13, 10, 15
	};
	DWORD reg;

	ASSERT(this);
	ASSERT((addr >= memdev.first) && (addr <= memdev.last));

	// 奇数アドレスのみデコードされている
	if ((addr & 1) != 0) {
		// 8バイト単位でループ
		addr &= 7;
		addr >>= 1;

		// ウェイト
		scheduler->Wait(3);

		// レジスタ振り分け
		switch (addr) {
			// チャネルBコマンドポート
			case 0:
				// レジスタ決定
				ASSERT(scc.ch[1].reg <= 7);
				if (scc.ch[1].ph) {
					reg = table[scc.ch[1].reg + 8];
				}
				else {
					reg = table[scc.ch[1].reg];
				}

				// セレクト＆ハイポイント
				scc.ch[1].reg = 0;
				scc.ch[1].ph = FALSE;

				// データリード
				return (BYTE)ReadSCC(&scc.ch[1], reg);

			// チャネルBデータポート
			case 1:
				return (BYTE)ReadRR8(&scc.ch[1]);

			// チャネルAコマンドポート
			case 2:
				// レジスタ決定
				ASSERT(scc.ch[0].reg <= 7);
				if (scc.ch[0].ph) {
					reg = table[scc.ch[0].reg + 8];
				}
				else {
					reg = table[scc.ch[0].reg];
				}

				// セレクト＆ハイポイント
				scc.ch[0].reg = 0;
				scc.ch[0].ph = FALSE;

				// データリード
				return (BYTE)ReadSCC(&scc.ch[0], reg);

			// チャネルAデータポート
			case 3:
				return (BYTE)ReadRR8(&scc.ch[0]);
		}

		// ここには来ない
		ASSERT(FALSE);
		return 0xff;
	}

	// 偶数アドレスは$FFを返す
	return 0xff;
}

//---------------------------------------------------------------------------
//
//	ワード読み込み
//
//---------------------------------------------------------------------------
DWORD FASTCALL SCC::ReadWord(DWORD addr)
{
	ASSERT(this);
	ASSERT((addr >= memdev.first) && (addr <= memdev.last));
	ASSERT((addr & 1) == 0);

	return (WORD)(0xff00 | ReadByte(addr + 1));
}

//---------------------------------------------------------------------------
//
//	バイト書き込み
//
//---------------------------------------------------------------------------
void FASTCALL SCC::WriteByte(DWORD addr, DWORD data)
{
	DWORD reg;

	ASSERT(this);
	ASSERT((addr >= memdev.first) && (addr <= memdev.last));

	// 奇数アドレスのみデコードされている
	if ((addr & 1) != 0) {
		// 8バイト単位でループ
		addr &= 7;
		addr >>= 1;

		// ウェイト
		scheduler->Wait(3);

		// レジスタ振り分け
		switch (addr) {
			// チャネルBコマンドポート
			case 0:
				// レジスタ決定
				ASSERT(scc.ch[1].reg <= 7);
				reg = scc.ch[1].reg;
				if (scc.ch[1].ph) {
					reg += 8;
				}

				// セレクト＆ハイポイント
				scc.ch[1].reg = 0;
				scc.ch[1].ph = FALSE;

				// データライト
				WriteSCC(&scc.ch[1], reg, (DWORD)data);
				return;

			// チャネルBデータポート
			case 1:
				WriteWR8(&scc.ch[1], (DWORD)data);
				return;

			// チャネルAコマンドポート
			case 2:
				// レジスタ決定
				ASSERT(scc.ch[0].reg <= 7);
				reg = scc.ch[0].reg;
				if (scc.ch[0].ph) {
					reg += 8;
				}

				// セレクト＆ハイポイント
				scc.ch[0].reg = 0;
				scc.ch[0].ph = FALSE;

				// データライト
				WriteSCC(&scc.ch[0], reg, (DWORD)data);
				return;

			// チャネルAデータポート
			case 3:
				WriteWR8(&scc.ch[0], (DWORD)data);
				return;
		}

		// ここには来ない
		ASSERT(FALSE);
		return;
	}

	// 偶数アドレスはデコードされていない
}

//---------------------------------------------------------------------------
//
//	ワード書き込み
//
//---------------------------------------------------------------------------
void FASTCALL SCC::WriteWord(DWORD addr, DWORD data)
{
	ASSERT(this);
	ASSERT((addr >= memdev.first) && (addr <= memdev.last));
	ASSERT((addr & 1) == 0);

	WriteByte(addr + 1, (BYTE)data);
}

//---------------------------------------------------------------------------
//
//	読み込みのみ
//
//---------------------------------------------------------------------------
DWORD FASTCALL SCC::ReadOnly(DWORD addr) const
{
	static const DWORD table[] = {
		0, 1, 2, 3, 0, 1, 2, 3, 8, 13, 10, 15, 12, 13, 10, 15
	};
	DWORD reg;

	ASSERT(this);
	ASSERT((addr >= memdev.first) && (addr <= memdev.last));

	// 奇数アドレスのみデコードされている
	if ((addr & 1) != 0) {
		// 8バイト単位でループ
		addr &= 7;
		addr >>= 1;

		// レジスタ振り分け
		switch (addr) {
			// チャネルBコマンドポート
			case 0:
				// レジスタ決定
				ASSERT(scc.ch[1].reg <= 7);
				if (scc.ch[1].ph) {
					reg = table[scc.ch[1].reg + 8];
				}
				else {
					reg = table[scc.ch[1].reg];
				}

				// データリード
				return (BYTE)ROSCC(&scc.ch[1], reg);

			// チャネルBデータポート
			case 1:
				return (BYTE)ROSCC(&scc.ch[1], 8);

			// チャネルAコマンドポート
			case 2:
				// レジスタ決定
				ASSERT(scc.ch[0].reg <= 7);
				if (scc.ch[0].ph) {
					reg = table[scc.ch[0].reg + 8];
				}
				else {
					reg = table[scc.ch[0].reg];
				}

				// データリード
				return (BYTE)ROSCC(&scc.ch[0], reg);

			// チャネルAデータポート
			case 3:
				return (BYTE)ROSCC(&scc.ch[0], 8);
		}

		// ここには来ない
		ASSERT(FALSE);
		return 0xff;
	}

	// 偶数アドレスは$FFを返す
	return 0xff;
}

//---------------------------------------------------------------------------
//
//	内部データ取得
//
//---------------------------------------------------------------------------
void FASTCALL SCC::GetSCC(scc_t *buffer) const
{
	ASSERT(this);
	ASSERT(buffer);

	// 内部ワークをコピー
	*buffer = scc;
}

//---------------------------------------------------------------------------
//
//	内部ワーク取得
//
//---------------------------------------------------------------------------
const SCC::scc_t* FASTCALL SCC::GetWork() const
{
	ASSERT(this);

	// 内部ワークを返す
	return (const scc_t*)&scc;
}

//---------------------------------------------------------------------------
//
//	ベクタ取得
//
//---------------------------------------------------------------------------
DWORD FASTCALL SCC::GetVector(int type) const
{
	DWORD vector;

	ASSERT(this);
	ASSERT((type >= 0) && (type < 8));

	// NV=1なら、0固定
	if (scc.nv) {
		return 0;
	}

	// 基準ベクタを設定
	vector = scc.vbase;

	// bit3-1か、bit6-4か
	if (scc.vis) {
		if (scc.shsl) {
			// bit6-4
			vector &= 0x8f;
			vector |= (type << 4);
		}
		else {
			// bit3-1
			vector &= 0xf1;
			vector |= (type << 1);
		}
	}

	return vector;
}

//---------------------------------------------------------------------------
//
//	チャネルリセット
//
//---------------------------------------------------------------------------
void FASTCALL SCC::ResetSCC(ch_t *p)
{
	int i;

	ASSERT(this);

	// NULLを与えるとハードウェアリセット
	if (p == NULL) {
#if defined(SCC_LOG)
		LOG0(Log::Normal, "ハードウェアリセット");
#endif	// SCC_LOG

		// WR9
		scc.shsl = FALSE;
		scc.mie = FALSE;
		scc.dlc = FALSE;

		// WR11
		return;
	}

#if defined(SCC_LOG)
	LOG1(Log::Normal, "チャネル%d リセット", p->index);
#endif	// SCC_LOG

	// RR0
	p->tu = TRUE;
	p->zc = FALSE;

	// WR0
	p->reg = 0;
	p->ph = FALSE;
	p->rxno = TRUE;
	p->txpend = FALSE;

	// RR1
	p->framing = FALSE;
	p->overrun = FALSE;
	p->parerr = FALSE;

	// WR1
	p->rxim = 0;
	p->txie = FALSE;
	p->extie = FALSE;

	// RR3
	for (i=0; i<2; i++) {
		scc.ch[i].rsip = FALSE;
		scc.ch[i].rxip = FALSE;
		scc.ch[i].txip = FALSE;
		scc.ch[i].extip = FALSE;
	}

	// WR3
	p->rxen = FALSE;

	// WR4
	p->stopbit |= 0x01;

	// WR5
	p->dtr = FALSE;
	p->brk = FALSE;
	p->txen = FALSE;
	p->rts = FALSE;
	if (p->index == 1) {
		mouse->MSCtrl(!p->rts, 1);
	}

	// WR14
	p->loopback = FALSE;
	p->aecho = FALSE;
	p->dtrreq = FALSE;

	// WR15
	p->baie = TRUE;
	p->tuie = TRUE;
	p->ctsie = TRUE;
	p->syncie = TRUE;
	p->dcdie = TRUE;
	p->zcie = FALSE;

	// 受信FIFO
	p->rxfifo = 0;

	// 受信バッファ
	p->rxnum = 0;
	p->rxread = 0;
	p->rxwrite = 0;
	p->rxtotal = 0;

	// 送信バッファ
	p->txnum = 0;
	p->txread = 0;
	p->txwrite = 0;
	p->tdf = FALSE;
	p->txtotal = 0;
	p->txwait = FALSE;

	// 割り込み
	IntCheck();
}

//---------------------------------------------------------------------------
//
//	チャネル読み出し
//
//---------------------------------------------------------------------------
DWORD FASTCALL SCC::ReadSCC(ch_t *p, DWORD reg)
{
	ASSERT(this);
	ASSERT(p);
	ASSERT((p->index == 0) || (p->index == 1));


#if defined(SCC_LOG)
	LOG2(Log::Normal, "チャネル%d読み出し R%d", p->index, reg);
#endif	// SCC_LOG

	switch (reg) {
		// RR0 - 拡張ステータス
		case 0:
			return ReadRR0(p);

		// RR1 - スペシャルRxコンディション
		case 1:
			return ReadRR1(p);

		// RR2 - ベクタ
		case 2:
			return ReadRR2(p);

		// RR3 - 割り込みペンディング
		case 3:
			return ReadRR3(p);

		// RR8 - 受信データ
		case 8:
			return ReadRR8(p);

		// RR10 - SDLCループモード
		case 10:
			return 0;

		// RR12 - ボーレート下位
		case 12:
			return p->tc;

		// RR13 - ボーレート上位
		case 13:
			return (p->tc >> 8);

		// RR15 - 外部ステータス割り込み制御
		case 15:
			return ReadRR15(p);
	}

#if defined(SCC_LOG)
	LOG1(Log::Normal, "未実装レジスタ読み出し R%d", reg);
#endif	// SCC_LOG

	return 0xff;
}

//---------------------------------------------------------------------------
//
//	RR0読み出し
//
//---------------------------------------------------------------------------
DWORD FASTCALL SCC::ReadRR0(const ch_t *p) const
{
	DWORD data;

	ASSERT(this);
	ASSERT(p);
	ASSERT((p->index == 0) || (p->index == 1));

	// 初期化
	data = 0;

	// bit7 : Break/Abortステータス
	if (p->ba) {
		data |= 0x80;
	}

	// bit6 : 送信アンダーランステータス
	if (p->tu) {
		data |= 0x40;
	}

	// bit5 : CTSステータス
	if (p->cts) {
		data |= 0x20;
	}

	// bit5 : Sync/Huntステータス
	if (p->sync) {
		data |= 0x10;
	}

	// bit3 : DCDステータス
	if (p->dcd) {
		data |= 0x08;
	}

	// bit2 : 送信バッファエンプティステータス
	if (!p->txwait) {
		// 送信ウェイト時は常にバッファフル
		if (!p->tdf) {
			data |= 0x04;
		}
	}

	// bit1 : ゼロカウントステータス
	if (p->zc) {
		data |= 0x02;
	}

	// bit0 : 受信バッファ有効ステータス
	if (p->rxfifo > 0) {
		data |= 0x01;
	}

	return data;
}

//---------------------------------------------------------------------------
//
//	RR1読み出し
//
//---------------------------------------------------------------------------
DWORD FASTCALL SCC::ReadRR1(const ch_t *p) const
{
	DWORD data;

	ASSERT(this);
	ASSERT(p);
	ASSERT((p->index == 0) || (p->index == 1));

	// 初期化
	data = 0x06;

	// bit6 : フレーミングエラー
	if (p->framing) {
		data |= 0x40;
	}

	// bit5 : オーバーランエラー
	if (p->overrun) {
		data |= 0x20;
	}

	// bit4 : パリティエラー
	if (p->parerr) {
		data |= 0x10;
	}

	// bit0 : 送信完了
	if (p->txsent) {
		data |= 0x01;
	}

	return data;
}

//---------------------------------------------------------------------------
//
//	RR2読み出し
//
//---------------------------------------------------------------------------
DWORD FASTCALL SCC::ReadRR2(ch_t *p)
{
	DWORD data;

	ASSERT(this);
	ASSERT(p);
	ASSERT((p->index == 0) || (p->index == 1));

	// チャネル判別
	if (p->index == 0) {
		// チャネルAはWR2の内容をそのまま返す
		data = scc.vbase;
	}
	else {
		// チャネルBは現在要求している割り込みベクタ
		data = scc.request;
	}

	// 割り込み要求中でなければ何もしない
	if (scc.ireq < 0) {
		return data;
	}

	// ここで、次の割り込みに移る(Macエミュレータ)
	ASSERT((scc.ireq >= 0) && (scc.ireq <= 7));
	if (scc.ireq >= 4) {
		p = &scc.ch[1];
	}
	else {
		p = &scc.ch[0];
	}
	switch (scc.ireq & 3) {
		// Rs
		case 0:
			p->rsip = FALSE;
			break;
		// Rx
		case 1:
			p->rxip = FALSE;
			break;
		// Ext
		case 2:
			p->extip = FALSE;
			break;
		// Tx
		case 3:
			p->txip = FALSE;
			break;
	}
	scc.ireq = -1;

	// 割り込みチェック
	IntCheck();

	// データ返す
	return data;
}

//---------------------------------------------------------------------------
//
//	RR3読み出し
//
//---------------------------------------------------------------------------
DWORD FASTCALL SCC::ReadRR3(const ch_t *p) const
{
	int i;
	DWORD data;

	ASSERT(this);
	ASSERT(p);
	ASSERT((p->index == 0) || (p->index == 1));

	// チャネルAのみ有効。チャネルBは0を返す
	if (p->index == 1) {
		return 0;
	}

	// クリア
	data = 0;

	// bit0からext,tx,rxで、チャネルB→チャネルAの順
	for (i=0; i<2; i++) {
		data <<= 3;

		// 有効な(要求したい)割り込みビットをすべて立てる
		if (scc.ch[i].extip) {
			data |= 0x01;
		}
		if (scc.ch[i].txip) {
			data |= 0x02;
		}
		if (scc.ch[i].rxip) {
			data |= 0x04;
		}
		if (scc.ch[i].rsip) {
			data |= 0x04;
		}
	}

	return data;
}

//---------------------------------------------------------------------------
//
//	RR8読み出し
//
//---------------------------------------------------------------------------
DWORD FASTCALL SCC::ReadRR8(ch_t *p)
{
	DWORD data;

	ASSERT(this);
	ASSERT(p);
	ASSERT((p->index == 0) || (p->index == 1));

	// 受信データがあるか
	if (p->rxfifo == 0) {
		if (p->index == 0) {
			LOG1(Log::Warning, "チャネル%d 受信データ空読み", p->index);
		}
		return 0;
	}

	// 必ず最後尾にデータがある
#if defined(SCC_LOG)
	LOG2(Log::Normal, "チャネル%d データ引き取り$%02X", p->index, p->rxdata[2]);
#endif
	data = p->rxdata[2];

	// 受信FIFO移動
	p->rxdata[2] = p->rxdata[1];
	p->rxdata[1] = p->rxdata[0];

	// 個数を下げる
	p->rxfifo--;

	// 一旦割り込みを落とす。FIFOの残りがあれば次のイベントで
	IntSCC(p, rxi, FALSE);

#if defined(SCC_LOG)
	LOG2(Log::Normal, "残りFIFO=%d 残りRxNum=%d", p->rxfifo, p->rxnum);
#endif

	return data;
}

//---------------------------------------------------------------------------
//
//	RR15読み出し
//
//---------------------------------------------------------------------------
DWORD FASTCALL SCC::ReadRR15(const ch_t *p) const
{
	DWORD data;

	ASSERT(this);
	ASSERT(p);
	ASSERT((p->index == 0) || (p->index == 1));

	// データ初期化
	data = 0;

	// bit7 - Break/Abort割り込み
	if (p->baie) {
		data |= 0x80;
	}

	// bit6 - Txアンダーラン割り込み
	if (p->tuie) {
		data |= 0x40;
	}

	// bit5 - CTS割り込み
	if (p->ctsie) {
		data |= 0x20;
	}

	// bit4 - SYNC割り込み
	if (p->syncie) {
		data |= 0x10;
	}

	// bit3 - DCD割り込み
	if (p->dcdie) {
		data |= 0x08;
	}

	// bit1 - ゼロカウント割り込み
	if (p->zcie) {
		data |= 0x02;
	}

	return data;
}

//---------------------------------------------------------------------------
//
//	読み出しのみ
//
//---------------------------------------------------------------------------
DWORD FASTCALL SCC::ROSCC(const ch_t *p, DWORD reg) const
{
	DWORD data;

	ASSERT(this);
	ASSERT(p);
	ASSERT((p->index == 0) || (p->index == 1));

	switch (reg) {
		// RR0 - 拡張ステータス
		case 0:
			return ReadRR0(p);

		// RR1 - スペシャルRxコンディション
		case 1:
			return ReadRR1(p);

		// RR2 - ベクタ
		case 2:
			if (p->index == 0) {
				return scc.vbase;
			}
			return scc.request;

		// RR3 - 割り込みペンディング
		case 3:
			return ReadRR3(p);

		// RR8 - 受信データ
		case 8:
			break;

		// RR10 - SDLCループモード
		case 10:
			return 0;

		// RR12 - ボーレート下位
		case 12:
			return p->tc;

		// RR13 - ボーレート上位
		case 13:
			return (p->tc >> 8);

		// RR15 - 外部ステータス割り込み制御
		case 15:
			return ReadRR15(p);
	}

	// データ初期化
	data = 0xff;

	// 受信FIFOが有効なら、返す
	if (p->rxfifo > 0) {
		data = p->rxdata[2];
	}

	return data;
}

//---------------------------------------------------------------------------
//
//	チャネル書き込み
//
//---------------------------------------------------------------------------
void FASTCALL SCC::WriteSCC(ch_t *p, DWORD reg, DWORD data)
{
	ASSERT(this);
	ASSERT(p);
	ASSERT((p->index == 0) || (p->index == 1));
	ASSERT(data < 0x100);

#if defined(SCC_LOG)
	LOG3(Log::Normal, "チャネル%d書き込み R%d <- $%02X", p->index, reg, data);
#endif	// SCC_LOG

	switch (reg) {
		// WR0 - 全体コントロール
		case 0:
			WriteWR0(p, data);
			return;

		// WR1 - 割り込み許可
		case 1:
			WriteWR1(p, data);
			return;

		// WR2 - ベクタベース
		case 2:
			LOG1(Log::Detail, "割り込みベクタベース $%02X", data);
			scc.vbase = data;
			return;

		// WR3 - 受信コントロール
		case 3:
			WriteWR3(p, data);
			return;

		// WR4 - 通信モード設定
		case 4:
			WriteWR4(p, data);
			return;

		// WR5 - 送信コントロール
		case 5:
			WriteWR5(p, data);
			return;

		// WR8 - 送信バッファ
		case 8:
			WriteWR8(p, data);
			return;

		// WR9 - 割り込みコントロール
		case 9:
			WriteWR9(data);
			return;

		// WR10 - SDLCコントロール
		case 10:
			WriteWR10(p, data);
			return;

		// WR11 - クロック・端子選択
		case 11:
			WriteWR11(p, data);

		// WR12 - ボーレート下位
		case 12:
			WriteWR12(p, data);
			return;

		// WR13 - ボーレート上位
		case 13:
			WriteWR13(p, data);
			return;

		// WR14 - クロックコントロール
		case 14:
			WriteWR14(p, data);
			return;

		// WR15 - 拡張ステータス割り込みコントロール
		case 15:
			WriteWR15(p, data);
			return;
	}

	LOG3(Log::Warning, "チャネル%d未実装レジスタ書き込み R%d <- $%02X",
								p->index, reg, data);
}

//---------------------------------------------------------------------------
//
//	WR0書き込み
//
//---------------------------------------------------------------------------
void FASTCALL SCC::WriteWR0(ch_t *p, DWORD data)
{
	ASSERT(this);
	ASSERT(p);
	ASSERT((p->index == 0) || (p->index == 1));
	ASSERT(data < 0x100);

	// bit2-0 : アクセスレジスタ
	p->reg = (data & 0x07);

	// bit5-3 : SCC動作コマンド
	data >>= 3;
	data &= 0x07;
	switch (data) {
		// 000:ヌルコード
		case 0:
			break;

		// 001:上位レジスタ選択
		case 1:
			p->ph = TRUE;
			break;

		// 010:外部ステータスリセット
		case 2:
#if defined(SCC_LOG)
			LOG1(Log::Normal, "チャネル%d 外部ステータスリセット", p->index);
#endif	// SCC_LOG
			break;

		// 100:受信データリセット
		case 4:
#if defined(SCC_LOG)
			LOG1(Log::Normal, "チャネル%d 受信データリセット", p->index);
#endif	// SCC_LOG
			p->rxno = TRUE;
			break;

		// 101:送信割り込みペンディングリセット
		case 5:
#if defined(SCC_LOG)
			LOG1(Log::Normal, "チャネル%d 送信割り込みペンディングリセット", p->index);
#endif	// SCC_LOG
			p->txpend = TRUE;
			IntSCC(p, txi, FALSE);

		// 110:受信エラーリセット
		case 6:
#if defined(SCC_LOG)
			LOG1(Log::Normal, "チャネル%d 受信エラーリセット", p->index);
#endif	// SCC_LOG
			p->framing = FALSE;
			p->overrun = FALSE;
			p->parerr = FALSE;
			IntSCC(p, rsi, FALSE);
			break;

		// 111:最高位インサービスリセット
		case 7:
			break;

		// その他
		default:
			LOG2(Log::Warning, "チャネル%d SCC未サポートコマンド $%02X",
								p->index, data);
			break;
	}
}

//---------------------------------------------------------------------------
//
//	WR1書き込み
//
//---------------------------------------------------------------------------
void FASTCALL SCC::WriteWR1(ch_t *p, DWORD data)
{
	ASSERT(this);
	ASSERT(p);
	ASSERT((p->index == 0) || (p->index == 1));
	ASSERT(data < 0x100);

	// bit4-3 : Rx割り込みモード
	p->rxim = (data >> 3) & 0x03;

	// bit2 : パリティエラーをスペシャルRxコンディション割り込みに
	if (data & 0x04) {
		p->parsp = TRUE;
	}
	else {
		p->parsp = FALSE;
	}

	// bit1 : 送信割り込み許可
	if (data & 0x02) {
		p->txie = TRUE;
	}
	else {
		p->txie = FALSE;
	}

	// bit0 : 外部ステータス割り込み許可
	if (data & 0x01) {
		p->extie = TRUE;
	}
	else {
		p->extie = FALSE;
	}
}

//---------------------------------------------------------------------------
//
//	WR3書き込み
//
//---------------------------------------------------------------------------
void FASTCALL SCC::WriteWR3(ch_t *p, DWORD data)
{
	ASSERT(this);
	ASSERT(p);
	ASSERT((p->index == 0) || (p->index == 1));
	ASSERT(data < 0x100);

	// bit7-6 : 受信キャラクタ長
	p->rxbit = ((data & 0xc0) >> 6) + 5;
	ASSERT((p->rxbit >= 5) && (p->rxbit <= 8));

	// bit5 : オートイネーブル
	if (data & 0x20) {
		p->aen = TRUE;
	}
	else {
		p->aen = FALSE;
	}

	// bit0 : 受信イネーブル
	if (data & 0x01) {
		if (!p->rxen) {
			// エラー状態をクリア
			p->framing = FALSE;
			p->overrun = FALSE;
			p->parerr = FALSE;
			IntSCC(p, rsi, FALSE);

			// 受信バッファをクリア
			p->rxnum = 0;
			p->rxread = 0;
			p->rxwrite = 0;
			p->rxfifo = 0;

			// 受信イネーブル
			p->rxen = TRUE;
		}
	}
	else {
		p->rxen = FALSE;
	}

	// ボーレートを再計算
	ClockSCC(p);
}

//---------------------------------------------------------------------------
//
//	WR4書き込み
//
//---------------------------------------------------------------------------
void FASTCALL SCC::WriteWR4(ch_t *p, DWORD data)
{
	ASSERT(this);
	ASSERT(p);
	ASSERT((p->index == 0) || (p->index == 1));
	ASSERT(data < 0x100);

	// bit7-6 : クロックモード
	switch ((data & 0xc0) >> 6) {
		// x1クロックモード
		case 0:
			p->clkm = 1;
			break;
		// x16クロックモード
		case 1:
			p->clkm = 16;
			break;
		// x32クロックモード
		case 2:
			p->clkm = 32;
			break;
		// x64クロックモード
		case 3:
			p->clkm = 64;
			break;
		// その他(ありえない)
		default:
			ASSERT(FALSE);
			break;
	}

	// bit3-2 : ストップビット
	p->stopbit = (data & 0x0c) >> 2;
	if (p->stopbit == 0) {
		LOG1(Log::Warning, "チャネル%d 同期モード", p->index);
	}

	// bit1-0 : パリティ
	if (data & 1) {
		// 1:奇数パリティ、2:偶数パリティ
		p->parity = ((data & 2) >> 1) + 1;
	}
	else {
		// 0:パリティなし
		p->parity = 0;
	}

	// ボーレートを再計算
	ClockSCC(p);
}

//---------------------------------------------------------------------------
//
//	WR5書き込み
//
//---------------------------------------------------------------------------
void FASTCALL SCC::WriteWR5(ch_t *p, DWORD data)
{
	ASSERT(this);
	ASSERT(p);
	ASSERT((p->index == 0) || (p->index == 1));
	ASSERT(data < 0x100);

	// bit7 : DTR制御(Lowアクティブ,p->dtrreqも見ること)
	if (data & 0x80) {
		p->dtr = TRUE;
	}
	else {
		p->dtr = FALSE;
	}

	// bit6-5 : 送信キャラクタビット長
	p->txbit = ((data & 0x60) >> 5) + 5;
	ASSERT((p->txbit >= 5) && (p->txbit <= 8));

	// bit4 : ブレーク信号送出
	if (data & 0x10) {
#if defined(SCC_LOG)
		LOG1(Log::Normal, "チャネル%d ブレーク信号ON", p->index);
#endif	// SCC_LOG
		p->brk = TRUE;
	}
	else {
#if defined(SCC_LOG)
		LOG1(Log::Normal, "チャネル%d ブレーク信号OFF", p->index);
#endif	// SCC_LOG
		p->brk = FALSE;
	}

	// bit3 : 送信イネーブル
	if (data & 0x08) {
		if (!p->txen) {
			// 送信バッファをクリア
			p->txnum = 0;
			if (!p->txpend) {
				IntSCC(p, txi, TRUE);
			}

			// 送信ワークをクリア
			p->txsent = FALSE;
			p->tdf = FALSE;
			p->txwait = FALSE;

			// 送信イネーブル
			p->txen = TRUE;
		}
	}
	else {
		p->txen = FALSE;
	}

	// bit1 : RTS制御
	if (data & 0x02) {
#if defined(SCC_LOG)
		LOG1(Log::Normal, "チャネル%d RTS信号ON", p->index);
#endif	// SCC_LOG
		p->rts = TRUE;
	}
	else {
#if defined(SCC_LOG)
		LOG1(Log::Normal, "チャネル%d RTS信号OFF", p->index);
#endif	// SCC_LOG
		p->rts = FALSE;
	}

	// チャネルBの場合は、マウスにMSCTRLを伝える
	if (p->index == 1) {
		// RTSはLowアクティブ、その先にインバータがついている(回路図参照)
		mouse->MSCtrl(!(p->rts), 1);
	}
}

//---------------------------------------------------------------------------
//
//	WR8書き込み
//
//---------------------------------------------------------------------------
void FASTCALL SCC::WriteWR8(ch_t *p, DWORD data)
{
	ASSERT(this);
	ASSERT(p);
	ASSERT((p->index == 0) || (p->index == 1));
	ASSERT(data < 0x100);

	// 送信データを記憶、送信データあり
	p->tdr = data;
	p->tdf = TRUE;

	// 送信割り込みを取り下げ
	IntSCC(p, txi, FALSE);

	// 送信割り込みペンディングを自動オフ
	p->txpend = FALSE;
}

//---------------------------------------------------------------------------
//
//	WR9書き込み
//
//---------------------------------------------------------------------------
void FASTCALL SCC::WriteWR9(DWORD data)
{
	ASSERT(this);
	ASSERT(data < 0x100);

	// bit7-6 : リセット
	switch ((data & 0xc0) >> 3) {
		case 1:
			ResetSCC(&scc.ch[0]);
			return;
		case 2:
			ResetSCC(&scc.ch[1]);
			return;
		case 3:
			ResetSCC(&scc.ch[0]);
			ResetSCC(&scc.ch[1]);
			ResetSCC(NULL);
			return;
		default:
			break;
	}

	// bit4 : 割り込み変化モード選択
	if (data & 0x10) {
#if defined(SCC_LOG)
		LOG0(Log::Warning, "割り込みベクタ変化モード bit4-bit6");
#endif	// SCC_LOG
		scc.shsl = TRUE;
	}
	else {
#if defined(SCC_LOG)
		LOG0(Log::Normal, "割り込みベクタ変化モード bit3-bit1");
#endif	// SCC_LOG
		scc.shsl = FALSE;
	}

	// bit3 : 割り込みイネーブル
	if (data & 0x08) {
#if defined(SCC_LOG)
		LOG0(Log::Normal, "割り込みイネーブル");
#endif	// SCC_LOG
		scc.mie = TRUE;
	}
	else {
#if defined(SCC_LOG)
		LOG0(Log::Warning, "割り込みディセーブル");
#endif	// SCC_LOG
		scc.mie = FALSE;
	}

	// bit2 : ディジーチェイン禁止
	if (data & 0x04) {
		scc.dlc = TRUE;
	}
	else {
		scc.dlc = FALSE;
	}

	// bit1 : 割り込みベクタ応答なし
	if (data & 0x02) {
		scc.nv = TRUE;
#if defined(SCC_LOG)
		LOG0(Log::Warning, "割り込みアクノリッジなし");
#endif	// SCC_LOG
	}
	else {
#if defined(SCC_LOG)
		LOG0(Log::Normal, "割り込みアクノリッジあり");
#endif	// SCC_LOG
		scc.nv = FALSE;
	}

	// bit0 : 割り込みベクタ変動
	if (data & 0x01) {
		scc.vis = TRUE;
#if defined(SCC_LOG)
		LOG0(Log::Normal, "割り込みベクタ変化");
#endif	// SCC_LOG
	}
	else {
		scc.vis = FALSE;
#if defined(SCC_LOG)
		LOG0(Log::Warning, "割り込みベクタ固定");
#endif	// SCC_LOG
	}

	// 割り込みチェック
	IntCheck();
}

//---------------------------------------------------------------------------
//
//	WR10書き込み
//
//---------------------------------------------------------------------------
void FASTCALL SCC::WriteWR10(ch_t* /*p*/, DWORD data)
{
	ASSERT(this);
	ASSERT(data < 0x100);

	data &= 0x60;
	if (data != 0) {
		LOG0(Log::Warning, "NRZ以外の変調モード");
	}
}

//---------------------------------------------------------------------------
//
//	WR11書き込み
//
//---------------------------------------------------------------------------
void FASTCALL SCC::WriteWR11(ch_t *p, DWORD data)
{
	ASSERT(this);
	ASSERT(p);
	ASSERT((p->index == 0) || (p->index == 1));
	ASSERT(data < 0x100);

	// bit7 : RTxC水晶あり・なし
	if (data & 0x80) {
		LOG1(Log::Warning, "チャネル%d bRTxCとbSYNCでクロック作成", p->index);
	}

	// bit6-5 : 受信クロック源選択
	if ((data & 0x60) != 0x40) {
		LOG1(Log::Warning, "チャネル%d ボーレートジェネレータ出力を使わない", p->index);
	}
}

//---------------------------------------------------------------------------
//
//	WR12書き込み
//
//---------------------------------------------------------------------------
void FASTCALL SCC::WriteWR12(ch_t *p, DWORD data)
{
	ASSERT(this);
	ASSERT(p);
	ASSERT((p->index == 0) || (p->index == 1));
	ASSERT(data < 0x100);

	// 下位8bitのみ
	p->tc &= 0xff00;
	p->tc |= data;

	// ボーレートを再計算
	ClockSCC(p);
}

//---------------------------------------------------------------------------
//
//	WR13書き込み
//
//---------------------------------------------------------------------------
void FASTCALL SCC::WriteWR13(ch_t *p, DWORD data)
{
	ASSERT(this);
	ASSERT(p);
	ASSERT((p->index == 0) || (p->index == 1));
	ASSERT(data < 0x100);

	// 上位8bitのみ
	p->tc &= 0x00ff;
	p->tc |= (data << 8);

	// ボーレートを再計算
	ClockSCC(p);
}

//---------------------------------------------------------------------------
//
//	WR14書き込み
//
//---------------------------------------------------------------------------
void FASTCALL SCC::WriteWR14(ch_t *p, DWORD data)
{
	ASSERT(this);
	ASSERT(p);
	ASSERT((p->index == 0) || (p->index == 1));
	ASSERT(data < 0x100);

	// bit4 : ローカルループバック
	if (data & 0x10) {
#if defined(SCC_LOG)
		LOG1(Log::Normal, "チャネル%d ローカルループバックモード", p->index);
#endif	// SCC_LOG
		p->loopback = TRUE;
	}
	else {
		p->loopback = FALSE;
	}

	// bit3 : オートエコー
	if (data & 0x08) {
#if defined(SCC_LOG)
		LOG1(Log::Normal, "チャネル%d オートエコーモード", p->index);
#endif	// SCC_LOG
		p->aecho = TRUE;
	}
	else {
		p->aecho = FALSE;
	}

	// bit2 : DTR/REQセレクト
	if (data & 0x04) {
		p->dtrreq = TRUE;
	}
	else {
		p->dtrreq = FALSE;
	}

	// bit1 : ボーレートジェネレータクロック供給元
	if (data & 0x02) {
		p->brgsrc = TRUE;
	}
	else {
		p->brgsrc = FALSE;
	}

	// bit0 : ボーレートジェネレータイネーブル
	if (data & 0x01) {
		p->brgen = TRUE;
	}
	else {
		p->brgen = FALSE;
	}

	// ボーレートを再計算
	ClockSCC(p);
}

//---------------------------------------------------------------------------
//
//	ボーレート再計算
//
//---------------------------------------------------------------------------
void FASTCALL SCC::ClockSCC(ch_t *p)
{
	DWORD len;
	DWORD speed;

	ASSERT(this);
	ASSERT(p);
	ASSERT((p->index == 0) || (p->index == 1));

	// ボーレートジェネレータが有効でなければ、イベント停止
	if (!p->brgsrc || !p->brgen) {
		event[p->index].SetTime(0);
		p->speed = 0;
		return;
	}

	// 基準クロック
	if (clkup) {
		p->baudrate = 3750000;
	}
	else {
		p->baudrate = 2500000;
	}

	// ボーレート設定値とクロック倍率から、ボーレートを算出
	speed = (p->tc + 2);
	speed *= p->clkm;
	p->baudrate /= speed;

	// データビット長、パリティ、ストップビットから2バイトレングスを算出
	len = p->rxbit + 1;
	if (p->parity != 0) {
		len++;
	}
	len <<= 1;
	switch (p->stopbit) {
		// ストップビットなし
		case 0:
			break;
		// ストップビット1bit
		case 1:
			len += 2;
			break;
		// ストップビット1.5bit
		case 2:
			len += 3;
		// ストップビット2bit
		case 3:
			len += 4;
	}

	// CPSに設定しておく
	if (clkup) {
		p->cps = 3750000;
	}
	else {
		p->cps = 2500000;
	}
	p->cps /= ((len * speed) >> 1);

	// 最終速度計算
	p->speed = 100;
	p->speed *= (len * speed);
	if (clkup) {
		p->speed /= 375;
	}
	else {
		p->speed /= 250;
	}

	// 設定
	if (event[p->index].GetTime() == 0) {
		event[p->index].SetTime(p->speed);
	}
}

//---------------------------------------------------------------------------
//
//	WR15書き込み
//
//---------------------------------------------------------------------------
void FASTCALL SCC::WriteWR15(ch_t *p, DWORD data)
{
	ASSERT(this);
	ASSERT(p);
	ASSERT((p->index == 0) || (p->index == 1));
	ASSERT(data < 0x100);

	// bit7 - Break/Abort割り込み
	if (data & 0x80) {
		p->baie = TRUE;
	}
	else {
		p->baie = FALSE;
	}

	// bit6 - Txアンダーラン割り込み
	if (data & 0x40) {
		p->tuie = TRUE;
	}
	else {
		p->tuie = FALSE;
	}

	// bit5 - CTS割り込み
	if (data & 0x20) {
		p->ctsie = TRUE;
	}
	else {
		p->ctsie = FALSE;
	}

	// bit4 - SYNC割り込み
	if (data & 0x10) {
		p->syncie = TRUE;
	}
	else {
		p->syncie = FALSE;
	}

	// bit3 - DCD割り込み
	if (data & 0x08) {
		p->dcdie = TRUE;
	}
	else {
		p->dcdie = FALSE;
	}

	// bit1 - ゼロカウント割り込み
	if (data & 0x02) {
		p->zcie = TRUE;
	}
	else {
		p->zcie = FALSE;
	}
}

//---------------------------------------------------------------------------
//
//	割り込みリクエスト
//
//---------------------------------------------------------------------------
void FASTCALL SCC::IntSCC(ch_t *p, itype_t type, BOOL flag)
{
	ASSERT(this);
	ASSERT(p);
	ASSERT((p->index == 0) || (p->index == 1));

	// 割り込み有効なら、ペンディングビットを上げる
	switch (type) {
		// 受信割り込み
		case rxi:
			p->rxip = FALSE;
			if (flag) {
				if ((p->rxim == 1) || (p->rxim == 2)) {
					p->rxip = TRUE;
				}
			}
			break;

		// スペシャルRxコンディション割り込み
		case rsi:
			p->rsip = FALSE;
			if (flag) {
				if (p->rxim >= 2) {
					p->rsip = TRUE;
				}
			}
			break;

		// 送信割り込み
		case txi:
			p->txip = FALSE;
			if (flag) {
				if (p->txie) {
					p->txip = TRUE;
				}
			}
			break;

		// 外部ステータス変化割り込み
		case exti:
			p->extip = FALSE;
			if (flag) {
				if (p->extie) {
					p->extip = TRUE;
				}
			}
			break;

		// その他(ありえない)
		default:
			ASSERT(FALSE);
			break;
	}

	// 割り込みチェック
	IntCheck();
}

//---------------------------------------------------------------------------
//
//	割り込みチェック
//
//---------------------------------------------------------------------------
void FASTCALL SCC::IntCheck()
{
	int i;
	DWORD v;

	ASSERT(this);

	// 要求割り込みをクリア
	scc.ireq = -1;

	// マスタ割り込み許可
	if (scc.mie) {

		// チャネルA→チャネルBの順
		for (i=0; i<2; i++) {
			// Rs,Rx,Tx,Extの順でペンディングビットを見る
			if (scc.ch[i].rsip) {
				scc.ireq = (i << 2) + 0;
				break;
			}
			if (scc.ch[i].rxip) {
				scc.ireq = (i << 2) + 1;
				break;
			}
			if (scc.ch[i].txip) {
				scc.ireq = (i << 2) + 3;
				break;
			}
			if (scc.ch[i].extip) {
				scc.ireq = (i << 2) + 2;
				break;
			}
		}

		// タイプを見て、割り込みベクタ作成
		if (scc.ireq >= 0) {
			// 基準ベクタを取得
			v = scc.vbase;

			// bit3-1か、bit6-4か
			if (scc.shsl) {
				// bit6-4
				v &= 0x8f;
				v |= ((7 - scc.ireq) << 4);
			}
			else {
				// bit3-1
				v &= 0xf1;
				v |= ((7 - scc.ireq) << 1);
			}

			// 割り込みベクタ記憶
			scc.request = v;

			// RR2から読めるベクタはWR9のVISによらず常に変化させる
			// データシートの記述とは異なる(Macエミュレータ)
			if (!scc.vis) {
				v = scc.vbase;
			}

			// NV(No Vector)が上がっていなければ割り込み要求
			if (!scc.nv) {
				// 既に要求していればOK
				if (scc.vector == (int)v) {
					return;
				}

				// 別のベクタを要求していれば、一度キャンセル
				if (scc.vector >= 0) {
					cpu->IntCancel(5);
				}

				// 割り込み要求
#if defined(SCC_LOG)
				LOG2(Log::Normal, "割り込み要求 リクエスト$%02X ベクタ$%02X ", scc.request, v);
#endif	// SCC_LOG
				cpu->Interrupt(5, (BYTE)v);
				scc.vector = (int)v;
			}
			return;
		}
	}

	// 要求割り込みなしのベクタを生成
	scc.request = scc.vbase;

	// bit3-1か、bit6-4か
	if (scc.shsl) {
		// bit6-4
		scc.request &= 0x8f;
		scc.request |= 0x60;
	}
	else {
		// bit3-1
		scc.request &= 0xf1;
		scc.request |= 0x06;
	}

	// 既に要求していれば割り込みキャンセル
	if (scc.vector >= 0) {
		cpu->IntCancel(5);
		scc.vector = -1;
	}
}

//---------------------------------------------------------------------------
//
//	割り込み応答
//
//---------------------------------------------------------------------------
void FASTCALL SCC::IntAck()
{
	ASSERT(this);

	// リセット直後に、CPUから割り込みが間違って入る場合がある
	if (scc.vector < 0) {
		LOG0(Log::Warning, "要求していない割り込み");
		return;
	}

#if defined(SCC_LOG)
	LOG1(Log::Normal, "割り込み応答 ベクタ$%02X", scc.vector);
#endif

	// 要求ベクタなし
	scc.vector = -1;

	// ここで一旦割り込みを解除する。rxi割り込み継続の場合は次の
	// イベントで(ノスタルジア1907、悪魔城ドラキュラ)
}

//---------------------------------------------------------------------------
//
//	データ送信
//
//---------------------------------------------------------------------------
void FASTCALL SCC::Send(int channel, DWORD data)
{
	ch_t *p;

	ASSERT(this);
	ASSERT((channel == 0) || (channel == 1));

	// BYTEに制限(マウスからはsigned intの変換でくるため)
	data &= 0xff;

#if defined(SCC_LOG)
	LOG2(Log::Normal, "チャネル%d データ送信$%02X", channel, data);
#endif	// SCC_LOG

	// ポインタ設定
	p = &scc.ch[channel];

	// 受信バッファへ挿入
	p->rxbuf[p->rxwrite] = (BYTE)data;
	p->rxwrite = (p->rxwrite + 1) & (sizeof(p->rxbuf) - 1);
	p->rxnum++;
	if (p->rxnum >= sizeof(p->rxbuf)) {
		LOG1(Log::Warning, "チャネル%d 受信バッファオーバーフロー", p->index);
		p->rxnum = sizeof(p->rxbuf);
		p->rxread = p->rxwrite;
	}
}

//----------------------------------------------------------------------------
//
//	パリティエラー
//
//---------------------------------------------------------------------------
void FASTCALL SCC::ParityErr(int channel)
{
	ch_t *p;

	ASSERT(this);
	ASSERT((channel == 0) || (channel == 1));

	// ポインタ設定
	p = &scc.ch[channel];

	if (!p->parerr) {
		LOG1(Log::Normal, "チャネル%d パリティエラー", p->index);
	}

	// フラグ上げ
	p->parerr = TRUE;

	// 割り込み
	if (p->parsp) {
		if (p->rxim >= 2) {
			IntSCC(p, rsi, TRUE);
		}
	}
}

//---------------------------------------------------------------------------
//
//	フレーミングエラー
//
//---------------------------------------------------------------------------
void FASTCALL SCC::FramingErr(int channel)
{
	ch_t *p;

	ASSERT(this);
	ASSERT((channel == 0) || (channel == 1));

	// ポインタ設定
	p = &scc.ch[channel];

	if (!p->framing) {
		LOG1(Log::Normal, "チャネル%d フレーミングエラー", p->index);
	}

	// フラグ上げ
	p->framing = TRUE;

	// 割り込み
	if (p->rxim >= 2) {
		IntSCC(p, rsi, TRUE);
	}
}

//---------------------------------------------------------------------------
//
//	受信イネーブルか
//
//---------------------------------------------------------------------------
BOOL FASTCALL SCC::IsRxEnable(int channel) const
{
	const ch_t *p;

	ASSERT(this);
	ASSERT((channel == 0) || (channel == 1));

	// ポインタ設定
	p = &scc.ch[channel];

	return p->rxen;
}

//---------------------------------------------------------------------------
//
//	受信バッファが空いているか
//
//---------------------------------------------------------------------------
BOOL FASTCALL SCC::IsRxBufEmpty(int channel) const
{
	const ch_t *p;

	ASSERT(this);
	ASSERT((channel == 0) || (channel == 1));

	// ポインタ設定
	p = &scc.ch[channel];

	if (p->rxnum == 0) {
		return TRUE;
	}
	return FALSE;
}

//---------------------------------------------------------------------------
//
//	ボーレートチェック
//
//---------------------------------------------------------------------------
BOOL FASTCALL SCC::IsBaudRate(int channel, DWORD baudrate) const
{
	const ch_t *p;
	DWORD offset;

	ASSERT(this);
	ASSERT((channel == 0) || (channel == 1));
	ASSERT(baudrate >= 75);

	// ポインタ設定
	p = &scc.ch[channel];

	// 与えられたボーレートの5%を計算
	offset = baudrate * 5;
	offset /= 100;

	// 範囲内ならOK
	if (p->baudrate < (baudrate - offset)) {
		return FALSE;
	}
	if ((baudrate + offset) < p->baudrate) {
		return FALSE;
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	受信データビット長取得
//
//---------------------------------------------------------------------------
DWORD FASTCALL SCC::GetRxBit(int channel) const
{
	const ch_t *p;

	ASSERT(this);
	ASSERT((channel == 0) || (channel == 1));

	// ポインタ設定
	p = &scc.ch[channel];

	return p->rxbit;
}

//---------------------------------------------------------------------------
//
//	ストップビット長取得
//
//---------------------------------------------------------------------------
DWORD FASTCALL SCC::GetStopBit(int channel) const
{
	const ch_t *p;

	ASSERT(this);
	ASSERT((channel == 0) || (channel == 1));

	// ポインタ設定
	p = &scc.ch[channel];

	return p->stopbit;
}

//---------------------------------------------------------------------------
//
//	パリティ取得
//
//---------------------------------------------------------------------------
DWORD FASTCALL SCC::GetParity(int channel) const
{
	const ch_t *p;

	ASSERT(this);
	ASSERT((channel == 0) || (channel == 1));

	// ポインタ設定
	p = &scc.ch[channel];

	return p->parity;
}

//---------------------------------------------------------------------------
//
//	データ受信
//
//---------------------------------------------------------------------------
DWORD FASTCALL SCC::Receive(int channel)
{
	ch_t *p;
	DWORD data;

	ASSERT(this);
	ASSERT((channel == 0) || (channel == 1));

	// ポインタ設定
	p = &scc.ch[channel];

	// データ初期化
	data = 0;

	// データがあれば
	if (p->txnum > 0) {
		// 送信バッファから取り出し
		data = p->txbuf[p->txread];
		p->txread = (p->txread + 1) & (sizeof(p->txbuf) - 1);
		p->txnum--;
	}

	return data;
}

//---------------------------------------------------------------------------
//
//	送信バッファエンプティチェック
//
//---------------------------------------------------------------------------
BOOL FASTCALL SCC::IsTxEmpty(int channel)
{
	ch_t *p;

	ASSERT(this);
	ASSERT((channel == 0) || (channel == 1));

	// ポインタ設定
	p = &scc.ch[channel];

	// 送信バッファが空いていればTRUEを返す
	if (p->txnum == 0) {
		return TRUE;
	}
	return FALSE;
}

//---------------------------------------------------------------------------
//
//	送信バッファフルチェック
//
//---------------------------------------------------------------------------
BOOL FASTCALL SCC::IsTxFull(int channel)
{
	ch_t *p;

	ASSERT(this);
	ASSERT((channel == 0) || (channel == 1));

	// ポインタ設定
	p = &scc.ch[channel];

	// 送信バッファが3/4以上埋まっていれば、TRUE
	if (p->txnum >= (sizeof(p->txbuf) * 3 / 4)) {
		return TRUE;
	}
	return FALSE;
}

//---------------------------------------------------------------------------
//
//	送信ブロック
//
//---------------------------------------------------------------------------
void FASTCALL SCC::WaitTx(int channel, BOOL wait)
{
	ch_t *p;

	ASSERT(this);
	ASSERT((channel == 0) || (channel == 1));

	// ポインタ設定
	p = &scc.ch[channel];

	// 解除するなら、フラグ操作が必要
	if (!wait) {
		if (p->txwait) {
			p->txsent = TRUE;
		}
	}

	// 設定
	p->txwait = wait;
}

//---------------------------------------------------------------------------
//
//	ブレークセット
//
//---------------------------------------------------------------------------
void FASTCALL SCC::SetBreak(int channel, BOOL flag)
{
	ch_t *p;

	ASSERT(this);
	ASSERT((channel == 0) || (channel == 1));

	// ポインタ設定
	p = &scc.ch[channel];

	// 割り込みチェック
	if (p->baie) {
		// 割り込み生成コード
	}

	// セット
	p->ba = flag;
}

//---------------------------------------------------------------------------
//
//	CTSセット
//
//---------------------------------------------------------------------------
void FASTCALL SCC::SetCTS(int channel, BOOL flag)
{
	ch_t *p;

	ASSERT(this);
	ASSERT((channel == 0) || (channel == 1));

	// ポインタ設定
	p = &scc.ch[channel];

	// 割り込みチェック
	if (p->ctsie) {
		// 割り込み生成コード
	}

	// セット
	p->cts = flag;
}

//---------------------------------------------------------------------------
//
//	DCDセット
//
//---------------------------------------------------------------------------
void FASTCALL SCC::SetDCD(int channel, BOOL flag)
{
	ch_t *p;

	ASSERT(this);
	ASSERT((channel == 0) || (channel == 1));

	// ポインタ設定
	p = &scc.ch[channel];

	// 割り込みチェック
	if (p->dcdie) {
		// 割り込み生成コード
	}

	// セット
	p->dcd = flag;
}

//---------------------------------------------------------------------------
//
//	ブレーク取得
//
//---------------------------------------------------------------------------
BOOL FASTCALL SCC::GetBreak(int channel)
{
	ch_t *p;

	ASSERT(this);
	ASSERT((channel == 0) || (channel == 1));

	// ポインタ設定
	p = &scc.ch[channel];

	// そのまま
	return p->brk;
}

//---------------------------------------------------------------------------
//
//	RTS取得
//
//---------------------------------------------------------------------------
BOOL FASTCALL SCC::GetRTS(int channel)
{
	ch_t *p;

	ASSERT(this);
	ASSERT((channel == 0) || (channel == 1));

	// ポインタ設定
	p = &scc.ch[channel];

	// そのまま
	return p->rts;
}

//---------------------------------------------------------------------------
//
//	DTR取得
//
//---------------------------------------------------------------------------
BOOL FASTCALL SCC::GetDTR(int channel)
{
	ch_t *p;

	ASSERT(this);
	ASSERT((channel == 0) || (channel == 1));

	// ポインタ設定
	p = &scc.ch[channel];

	// DTR/REQがFALSEなら、ソフトウェア指定
	if (!p->dtrreq) {
		return p->dtr;
	}

	// 送信バッファがエンプティかどうかを示す
	if (p->txwait) {
		return FALSE;
	}
	return !(p->tdf);
}

//---------------------------------------------------------------------------
//
//	イベントコールバック
//
//---------------------------------------------------------------------------
BOOL FASTCALL SCC::Callback(Event *ev)
{
	ch_t *p;
	int channel;

	ASSERT(this);
	ASSERT(ev);

	// チャネル取り出し
	channel = ev->GetUser();
	ASSERT((channel == 0) || (channel == 1));
	p = &scc.ch[channel];

	// 受信
	if (p->rxen) {
		EventRx(p);
	}

	// 送信
	if (p->txen) {
		EventTx(p);
	}

	// 速度変更
	if (ev->GetTime() != p->speed) {
		ev->SetTime(p->speed);
	}

	// インターバル継続
	return TRUE;
}

//---------------------------------------------------------------------------
//
//	イベント(受信)
//
//---------------------------------------------------------------------------
void FASTCALL SCC::EventRx(ch_t *p)
{
	BOOL flag;

	ASSERT(this);
	ASSERT(p);
	ASSERT((p->index == 0) || (p->index == 1));
	ASSERT(p->rxen);

	// 受信フラグ
	flag = TRUE;

	// DCDのチェック
	if (p->dcd && p->aen) {
		// ループバックが下がっていれば受信できない
		if (!p->loopback) {
			// 受信しない
			flag = FALSE;
		}
	}

	// 受信バッファが有効であれば、FIFOに挿入
	if ((p->rxnum > 0) && flag) {
		if (p->rxfifo >= 3) {
			ASSERT(p->rxfifo == 3);

			// 既にFIFOは一杯。オーバーラン
#if defined(SCC_LOG)
			LOG1(Log::Normal, "チャネル%d オーバーランエラー", p->index);
#endif	// SCC_LOG
			p->overrun = TRUE;
			p->rxfifo--;

			if (p->rxim >= 2) {
				IntSCC(p, rsi, TRUE);
			}
		}

		ASSERT(p->rxfifo <= 2);

		// 受信FIFOに挿入
		p->rxdata[2 - p->rxfifo] = (DWORD)p->rxbuf[p->rxread];
		p->rxread = (p->rxread + 1) & (sizeof(p->rxbuf) - 1);
		p->rxnum--;
		p->rxfifo++;

		// 受信トータル増加
		p->rxtotal++;
	}

	// 受信FIFOが有効であれば、割り込み
	if (p->rxfifo > 0) {
		// 割り込み発生
		switch (p->rxim) {
			// 初回のみ割り込み
			case 1:
				if (!p->rxno) {
					break;
				}
				p->rxno = FALSE;
				// そのまま続ける

			// 常に割り込み
			case 2:
				IntSCC(p, rxi, TRUE);
				break;
		}
	}
}

//---------------------------------------------------------------------------
//
//	イベント(送信)
//
//---------------------------------------------------------------------------
void FASTCALL SCC::EventTx(ch_t *p)
{
	ASSERT(this);
	ASSERT(p);
	ASSERT((p->index == 0) || (p->index == 1));
	ASSERT(p->txen);

	// ウェイトモードでなければ、前回の送信が完了
	if (!p->txwait) {
		p->txsent = TRUE;
	}

	// CTSのチェック
	if (p->cts && p->aen) {
		// テストモード時はCTSに関わらず送信できる
		if (!p->aecho && !p->loopback) {
			return;
		}
	}

	// データがあるか
	if (p->tdf) {
		// オートエコーモードでないか
		if (!p->aecho) {
			// 送信バッファへ挿入
			p->txbuf[p->txwrite] = (BYTE)p->tdr;
			p->txwrite = (p->txwrite + 1) & (sizeof(p->txbuf) - 1);
			p->txnum++;

			// あふれたら、古いデータから捨てる
			if (p->txnum >= sizeof(p->txbuf)) {
				LOG1(Log::Warning, "チャネル%d 送信バッファオーバーフロー", p->index);
				p->txnum = sizeof(p->txbuf);
				p->txread = p->txwrite;
			}

			// 送信トータル増加
			p->txtotal++;
		}

		// 送信実行中
		p->txsent = FALSE;

		// 送信バッファエンプティ
		p->tdf = FALSE;

		// ローカルループバックモードであれば、データを受信側へ入れる
		if (p->loopback && !p->aecho) {
			// 受信バッファへ挿入
			p->rxbuf[p->rxwrite] = (BYTE)p->tdr;
			p->rxwrite = (p->rxwrite + 1) & (sizeof(p->rxbuf) - 1);
			p->rxnum++;

			// あふれたら、古いデータから順次捨てる
			if (p->rxnum >= sizeof(p->rxbuf)) {
				LOG1(Log::Warning, "チャネル%d 受信バッファオーバーフロー", p->index);
				p->rxnum = sizeof(p->rxbuf);
				p->rxread = p->rxwrite;
			}
		}
	}

	// 送信ペンディングがなければ割り込み
	if (!p->txpend) {
		IntSCC(p, txi, TRUE);
	}
}
