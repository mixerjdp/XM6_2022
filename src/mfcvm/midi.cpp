//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2005 ＰＩ．(ytanaka@ipc-tokai.or.jp)
//	[ MIDI(YM3802) ]
//
//---------------------------------------------------------------------------

#include "os.h"
#include "xm6.h"
#include "vm.h"
#include "log.h"
#include "schedule.h"
#include "sync.h"
#include "config.h"
#include "fileio.h"
#include "midi.h"

//===========================================================================
//
//	MIDI
//
//===========================================================================
//#define MIDI_LOG

//---------------------------------------------------------------------------
//
//	コンストラクタ
//
//---------------------------------------------------------------------------
MIDI::MIDI(VM *p) : MemDevice(p)
{
	// デバイスIDを初期化
	dev.id = MAKEID('M', 'I', 'D', 'I');
	dev.desc = "MIDI (YM3802)";

	// 開始アドレス、終了アドレス
	memdev.first = 0xeae000;
	memdev.last = 0xeaffff;

	// オブジェクト
	sync = NULL;

	// 送信バッファ、受信バッファ
	midi.transbuf = NULL;
	midi.recvbuf = NULL;

	// ワーク一部初期化(自己診断用)
	midi.transnum = 0;
	midi.transread = 0;
	midi.transwrite = 0;
	midi.recvnum = 0;
	midi.recvread = 0;
	midi.recvwrite = 0;
	midi.normnum = 0;
	midi.normread = 0;
	midi.normwrite = 0;
	midi.rtnum = 0;
	midi.rtread = 0;
	midi.rtwrite = 0;
	midi.stdnum = 0;
	midi.stdread = 0;
	midi.stdwrite = 0;
	midi.rrnum = 0;
	midi.rrread = 0;
	midi.rrwrite = 0;
	midi.bid = 0;
	midi.ilevel = 4;
}

//---------------------------------------------------------------------------
//
//	初期化
//
//---------------------------------------------------------------------------
BOOL FASTCALL MIDI::Init()
{
	ASSERT(this);

	// 基本クラス
	if (!MemDevice::Init()) {
		return FALSE;
	}

	// Sync作成
	ASSERT(!sync);
	sync = new Sync;
	ASSERT(sync);

	// 送信バッファ確保
	try {
		midi.transbuf = new mididata_t[TransMax];
	}
	catch (...) {
		midi.transbuf = NULL;
		return FALSE;
	}
	if (!midi.transbuf) {
		return FALSE;
	}

	// 受信バッファ確保
	try {
		midi.recvbuf = new mididata_t[RecvMax];
	}
	catch (...) {
		midi.recvbuf = NULL;
		return FALSE;
	}
	if (!midi.recvbuf) {
		return FALSE;
	}

	// イベント初期化
	event[0].SetDevice(this);
	event[0].SetDesc("MIDI 31250bps");
	event[0].SetUser(0);
	event[0].SetTime(0);
	event[1].SetDevice(this);
	event[1].SetDesc("Clock");
	event[1].SetUser(1);
	event[1].SetTime(0);
	event[2].SetDevice(this);
	event[2].SetDesc("General Timer");
	event[2].SetUser(2);
	event[2].SetTime(0);

	// ボードなし、割り込みレベル4
	midi.bid = 0;
	midi.ilevel = 4;

	// 受信遅れ時間0ms
	recvdelay = 0;

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	クリーンアップ
//
//---------------------------------------------------------------------------
void FASTCALL MIDI::Cleanup()
{
	ASSERT(this);

	// 受信バッファ削除
	if (midi.recvbuf) {
		delete[] midi.recvbuf;
		midi.recvbuf = NULL;
	}

	// 送信バッファ削除
	if (midi.transbuf) {
		delete[] midi.transbuf;
		midi.transbuf = NULL;
	}

	// Sync削除
	if (sync) {
		delete sync;
		sync = NULL;
	}

	// 基本クラスへ
	MemDevice::Cleanup();
}

//---------------------------------------------------------------------------
//
//	リセット
//
//---------------------------------------------------------------------------
void FASTCALL MIDI::Reset()
{
	int i;

	ASSERT(this);
	ASSERT_DIAG();

	LOG0(Log::Normal, "リセット");

	// アクセスなし
	midi.access = FALSE;

	// リセットあり
	midi.reset = TRUE;

	// レジスタリセット
	ResetReg();

	// イベント初期化
	for (i=0; i<3; i++) {
		event[i].SetTime(0);
	}
}

//---------------------------------------------------------------------------
//
//	セーブ
//
//---------------------------------------------------------------------------
BOOL FASTCALL MIDI::Save(Fileio *fio, int ver)
{
	size_t sz;
	int i;

	ASSERT(this);
	ASSERT(fio);
	ASSERT(ver >= 0x0200);
	ASSERT_DIAG();

	LOG0(Log::Normal, "セーブ");

	// サイズをセーブ
	sz = sizeof(midi_t);
	if (!fio->Write(&sz, sizeof(sz))) {
		return FALSE;
	}

	// 実体をセーブ
	if (!fio->Write(&midi, (int)sz)) {
		return FALSE;
	}

	// イベントをセーブ
	for (i=0; i<3; i++) {
		if (!event[i].Save(fio, ver)) {
			return FALSE;
		}
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	ロード
//
//---------------------------------------------------------------------------
BOOL FASTCALL MIDI::Load(Fileio *fio, int ver)
{
	midi_t bk;
	size_t sz;
	int i;

	ASSERT(this);
	ASSERT(fio);
	ASSERT(ver >= 0x0200);
	ASSERT_DIAG();

	LOG0(Log::Normal, "ロード");

	// 現在のワークを保存
	bk = midi;

	// サイズをロード、照合
	if (!fio->Read(&sz, sizeof(sz))) {
		return FALSE;
	}
	if (sz != sizeof(midi_t)) {
		return FALSE;
	}

	// 実体をロード
	if (!fio->Read(&midi, (int)sz)) {
		return FALSE;
	}

	// イベントをロード
	for (i=0; i<3; i++) {
		if (!event[i].Load(fio, ver)) {
			return FALSE;
		}
	}

	// イベントを動的に追加・削除
	if (midi.bid == 0) {
		// イベント削除
		if (scheduler->HasEvent(&event[0])) {
			scheduler->DelEvent(&event[0]);
			scheduler->DelEvent(&event[1]);
			scheduler->DelEvent(&event[2]);
		}
	}
	else {
		// イベント追加
		if (!scheduler->HasEvent(&event[0])) {
			scheduler->AddEvent(&event[0]);
			scheduler->AddEvent(&event[1]);
			scheduler->AddEvent(&event[2]);
		}
	}

	// 送信バッファを復帰
	midi.transbuf = bk.transbuf;
	midi.transread = bk.transread;
	midi.transwrite = bk.transwrite;
	midi.transnum = bk.transnum;

	// 受信バッファを復帰
	midi.recvbuf = bk.recvbuf;
	midi.recvread = bk.recvread;
	midi.recvwrite = bk.recvwrite;
	midi.recvnum = bk.recvnum;

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	設定適用
//
//---------------------------------------------------------------------------
void FASTCALL MIDI::ApplyCfg(const Config *config)
{
	ASSERT(this);
	ASSERT(config);
	ASSERT_DIAG();

	LOG0(Log::Normal, "設定適用");

	if (midi.bid != (DWORD)config->midi_bid) {
		midi.bid = (DWORD)config->midi_bid;

		// イベントを動的に追加・削除
		if (midi.bid == 0) {
			// イベント削除
			if (scheduler->HasEvent(&event[0])) {
				scheduler->DelEvent(&event[0]);
				scheduler->DelEvent(&event[1]);
				scheduler->DelEvent(&event[2]);
			}
		}
		else {
			// イベント追加
			if (!scheduler->HasEvent(&event[0])) {
				scheduler->AddEvent(&event[0]);
				scheduler->AddEvent(&event[1]);
				scheduler->AddEvent(&event[2]);
			}
		}
	}
}

#if !defined(NDEBUG)
//---------------------------------------------------------------------------
//
//	診断
//
//---------------------------------------------------------------------------
void FASTCALL MIDI::AssertDiag() const
{
	// 基本クラス
	MemDevice::AssertDiag();

	ASSERT(this);
	ASSERT(GetID() == MAKEID('M', 'I', 'D', 'I'));
	ASSERT(memdev.first == 0xeae000);
	ASSERT(memdev.last == 0xeaffff);
	ASSERT((midi.bid >= 0) && (midi.bid < 2));
	ASSERT((midi.ilevel == 2) || (midi.ilevel == 4));
	ASSERT(midi.transnum <= TransMax);
	ASSERT(midi.transread < TransMax);
	ASSERT(midi.transwrite < TransMax);
	ASSERT(midi.recvnum <= RecvMax);
	ASSERT(midi.recvread < RecvMax);
	ASSERT(midi.recvwrite < RecvMax);
	ASSERT(midi.normnum <= 16);
	ASSERT(midi.normread < 16);
	ASSERT(midi.normwrite < 16);
	ASSERT(midi.rtnum <= 4);
	ASSERT(midi.rtread < 4);
	ASSERT(midi.rtwrite < 4);
	ASSERT(midi.stdnum <= 0x80);
	ASSERT(midi.stdread < 0x80);
	ASSERT(midi.stdwrite < 0x80);
	ASSERT(midi.rrnum <= 4);
	ASSERT(midi.rrread < 4);
	ASSERT(midi.rrwrite < 4);
}
#endif	// NDEBUG

//---------------------------------------------------------------------------
//
//	バイト読み込み
//
//---------------------------------------------------------------------------
DWORD FASTCALL MIDI::ReadByte(DWORD addr)
{
	ASSERT(this);
	ASSERT((addr >= memdev.first) && (addr <= memdev.last));
	ASSERT_DIAG();

	switch (midi.bid) {
		// ボード無し
		case 0:
			break;

		// ボードID1
		case 1:
			// 範囲内か
			if ((addr >= 0xeafa00) && (addr < 0xeafa10)) {
				// 奇数アドレスのみデコードされている
				if ((addr & 1) == 0) {
					return 0xff;
				}

				// レジスタリード
				addr -= 0xeafa00;
				addr >>= 1;
				return ReadReg(addr);
			}
			break;

		// ボードID2
		case 2:
			// 範囲内か
			if ((addr >= 0xeafa10) && (addr < 0xeafa20)) {
				// 奇数アドレスのみデコードされている
				if ((addr & 1) == 0) {
					return 0xff;
				}

				// レジスタリード
				addr -= 0xeafa10;
				addr >>= 1;
				return ReadReg(addr);
			}
			break;

		// その他(あり得ない)
		default:
			ASSERT(FALSE);
			break;
	}

	// バスエラー
	cpu->BusErr(addr, TRUE);

	return 0xff;
}

//---------------------------------------------------------------------------
//
//	ワード読み込み
//
//---------------------------------------------------------------------------
DWORD FASTCALL MIDI::ReadWord(DWORD addr)
{
	ASSERT(this);
	ASSERT((addr >= memdev.first) && (addr <= memdev.last));
	ASSERT((addr & 1) == 0);
	ASSERT_DIAG();

	return (0xff00 | ReadByte(addr + 1));
}

//---------------------------------------------------------------------------
//
//	バイト書き込み
//
//---------------------------------------------------------------------------
void FASTCALL MIDI::WriteByte(DWORD addr, DWORD data)
{
	ASSERT(this);
	ASSERT((addr >= memdev.first) && (addr <= memdev.last));
	ASSERT(data < 0x100);
	ASSERT_DIAG();

	switch (midi.bid) {
		// ボード無し
		case 0:
			break;

		// ボードID1
		case 1:
			// 範囲内か
			if ((addr >= 0xeafa00) && (addr < 0xeafa10)) {
				// 奇数アドレスのみデコードされている
				if ((addr & 1) == 0) {
					return;
				}

				// レジスタリード
				addr -= 0xeafa00;
				addr >>= 1;
				WriteReg(addr, data);
				return;
			}
			break;

		// ボードID2
		case 2:
			// 範囲内か
			if ((addr >= 0xeafa10) && (addr < 0xeafa20)) {
				// 奇数アドレスのみデコードされている
				if ((addr & 1) == 0) {
					return;
				}

				// レジスタリード
				addr -= 0xeafa10;
				addr >>= 1;
				WriteReg(addr, data);
				return;
			}
			break;

		// その他(あり得ない)
		default:
			ASSERT(FALSE);
			break;
	}

	// バスエラー
	cpu->BusErr(addr, FALSE);
}

//---------------------------------------------------------------------------
//
//	ワード書き込み
//
//---------------------------------------------------------------------------
void FASTCALL MIDI::WriteWord(DWORD addr, DWORD data)
{
	ASSERT(this);
	ASSERT((addr >= memdev.first) && (addr <= memdev.last));
	ASSERT((addr & 1) == 0);
	ASSERT(data < 0x10000);
	ASSERT_DIAG();

	WriteByte(addr + 1, (BYTE)data);
}

//---------------------------------------------------------------------------
//
//	読み込みのみ
//
//---------------------------------------------------------------------------
DWORD FASTCALL MIDI::ReadOnly(DWORD addr) const
{
	ASSERT(this);
	ASSERT((addr >= memdev.first) && (addr <= memdev.last));
	ASSERT_DIAG();

	switch (midi.bid) {
		// ボード無し
		case 0:
			break;

		// ボードID1
		case 1:
			// 範囲内か
			if ((addr >= 0xeafa00) && (addr < 0xeafa10)) {
				// 奇数アドレスのみデコードされている
				if ((addr & 1) == 0) {
					return 0xff;
				}

				// レジスタリード
				addr -= 0xeafa00;
				addr >>= 1;
				return ReadRegRO(addr);
			}
			break;

		// ボードID2
		case 2:
			// 範囲内か
			if ((addr >= 0xeafa10) && (addr < 0xeafa20)) {
				// 奇数アドレスのみデコードされている
				if ((addr & 1) == 0) {
					return 0xff;
				}

				// レジスタリード
				addr -= 0xeafa10;
				addr >>= 1;
				return ReadRegRO(addr);
			}
			break;

		// その他(あり得ない)
		default:
			ASSERT(FALSE);
			break;
	}

	return 0xff;
}

//---------------------------------------------------------------------------
//
//	MIDIアクティブチェック
//
//---------------------------------------------------------------------------
BOOL FASTCALL MIDI::IsActive() const
{
	ASSERT(this);
	ASSERT_DIAG();

	// ボードIDが0以外であり
	if (midi.bid != 0) {
		// リセット後、1回以上アクセスされていること
		if (midi.access) {
			return TRUE;
		}
	}

	// アクティブでない
	return FALSE;
}

//---------------------------------------------------------------------------
//
//	イベントコールバック
//
//---------------------------------------------------------------------------
BOOL FASTCALL MIDI::Callback(Event *ev)
{
	DWORD mtr;
	DWORD hus;

	ASSERT(this);
	ASSERT(ev);
	ASSERT_DIAG();

	// アクセスフラグがFALSEなら、MIDIアプリケーションは存在しない
	if (!midi.access) {
		// 高速化のため、先頭でチェック
		return TRUE;
	}

	// パラメータによって振り分け
	switch (ev->GetUser()) {
		// MIDI送受信 31250bps
		case 0:
			// 受信および送信
			Receive();
			Transmit();
			return TRUE;

		// MIDIクロック(補間あり)
		case 1:
			// 先にMTRを算出しておく
			if (midi.mtr <= 1) {
				mtr = 0x40000;
			}
			else {
				mtr = midi.mtr << 4;
			}

			// レコーディングカウンタ(8bitカウンタ)
			midi.srr++;
			midi.srr &= 0xff;
			if (midi.srr == 0) {
				// 割り込み
				Interrupt(3, TRUE);
			}

			// プレイバックカウンタ(15bitダウンカウンタ)
			if (midi.str != 0xffff8000) {
				midi.str--;
			}
			if (midi.str >= 0xffff8000) {
				// 割り込み
				Interrupt(2, TRUE);
			}

			// クロック補間カウンタをデクリメントし、0チェック
			midi.sct--;
			if (midi.sct != 0) {
				// SCT=1の場合、除算による切捨てを補うため、補正をかける
				if (midi.sct == 1) {
					hus = mtr - (mtr / midi.scr) * (midi.scr - 1);
					if (event[1].GetTime() != hus) {
						event[1].SetTime(hus);
					}
				}
				return TRUE;
			}
			midi.sct = midi.scr;

			// クロック処理
			Clock();

			// 受信有効か
			if (midi.rcr & 1) {
				// クロックタイマを使うか
				if ((midi.dmr & 0x07) == 0x03) {
					if (midi.dmr & 0x08) {
						// リアルタイム受信バッファへ挿入
						InsertRR(0xf8);
					}
				}
			}

			// バッファチェック
			CheckRR();

			// タイマ値は毎回ロードする(Mu-1)
			mtr /= midi.scr;
			if (ev->GetTime() != mtr) {
				ev->SetTime(mtr);
			}
			return TRUE;

		// 汎用タイマ
		case 2:
			General();
			return TRUE;
	}

	// それ以外はエラー。打ち切り
	ASSERT(FALSE);
	ev->SetTime(0);
	return FALSE;
}

//---------------------------------------------------------------------------
//
//	受信コールバック
//
//---------------------------------------------------------------------------
void FASTCALL MIDI::Receive()
{
	BOOL recv;
	mididata_t *p;
	DWORD diff;
	DWORD data;

	ASSERT(this);
	ASSERT_DIAG();

	// レシーバREADY
	midi.rsr &= ~0x01;

	// 受信ディセーブルなら何もしない
	if ((midi.rcr & 1) == 0) {
		// 受信しなかった
		return;
	}

	// 受信パラメータをチェック
	if (midi.rrr != 0x08) {
		// 31250bpsでない
		return;
	}
	if ((midi.rmr & 0x32) != 0) {
		// 8bitキャラクタ、1ストップビット、ノンパリティでない
		return;
	}

	// 受信フラグOFF
	recv = FALSE;

	// 受信バッファのチェック
	sync->Lock();
	if (midi.recvnum > 0) {
		// 有効なデータがある。時間差分をみる
		diff = scheduler->GetTotalTime();
		p = &midi.recvbuf[midi.recvread];
		diff -= p->vtime;
		if (diff > recvdelay) {
			// 今回受信する
			recv = TRUE;
		}
	}
	sync->Unlock();

	// 今回受信しないのであれば、受信なし時間を延長
	if (!recv) {
		if (midi.rcn == 0x3ff) {
			// 327.68ms受信なしなので、オフラインと割り込み
			midi.rsr |= 0x04;
			if ((midi.imr & 4) == 0) {
				Interrupt(4, TRUE);
			}
		}
		midi.rcn++;
		if (midi.rcn >= 0x400) {
			midi.rcn = 0;
		}

		// 受信しなかった
		return;
	}

	// 受信カウンタリセット
	midi.rcn = 0;

	// 受信バッファからデータを取り込む
	sync->Lock();
	data = midi.recvbuf[midi.recvread].data;
	midi.recvread = (midi.recvread + 1) & (RecvMax - 1);
	midi.recvnum--;
	sync->Unlock();

	if (data == 0xf8) {
		// リアルタイム受信バッファへ挿入
		if (midi.dmr & 0x08) {
			InsertRR(0xf8);

			// クロック処理
			Clock();

			// バッファチェック
			CheckRR();
		}

		// クロックフィルタ
		if (midi.rcr & 0x10) {
#if defined(MIDI_LOG)
			LOG0(Log::Normal, "MIDIクロックフィルタ動作 $F8スキップ");
#endif	// MIDI_LOG
			return;
		}
	}
	if ((data >= 0xf9) && (data <= 0xfd)) {
		// リアルタイム受信バッファへ挿入
		if (midi.dmr & 0x08) {
			InsertRR(data);
		}
	}

	// アドレスハンタ
	if (midi.rcr & 2) {
		if (data == 0xf0) {
			// メーカIDチェックへ
			midi.asr = 1;
		}
		if (data == 0xf7) {
			// アドレスハンタ終了
			midi.asr = 0;
		}
		switch (midi.asr) {
			// メーカIDチェック中
			case 1:
				if ((midi.amr & 0x7f) == data) {
#if defined(MIDI_LOG)
					LOG1(Log::Normal, "メーカIDマッチ $%02X", midi.amr & 0x7f);
#endif	// MIDI_LOG
					midi.asr = 2;
				}
				else {
#if defined(MIDI_LOG)
					LOG1(Log::Normal, "メーカIDアンマッチ $%02X", data);
#endif	// MIDI_LOG
					midi.asr = 3;
				}
				break;

			// デバイスIDチェック中
			case 2:
				if (midi.amr & 0x80) {
					if ((midi.adr & 0x7f) != data) {
#if defined(MIDI_LOG)
						LOG1(Log::Normal, "デバイスIDアンマッチ $%02X", data);
#endif	// MIDI_LOG
						midi.asr = 3;
					}
				}
				break;

			default:
				break;
		}
		if (midi.asr == 3) {
			// アドレスハンタ動作中。スキップ
			return;
		}
	}
	else {
		// アドレスハンタ ディセーブル
		midi.asr = 0;
	}

	// レシーバBUSY
	midi.rsr |= 0x01;

	// 標準バッファへ
	InsertStd(data);
}

//---------------------------------------------------------------------------
//
//	送信コールバック
//
//---------------------------------------------------------------------------
void FASTCALL MIDI::Transmit()
{
	ASSERT(this);
	ASSERT_DIAG();

	// 送信Ready
	midi.tbs = FALSE;

	// リアルタイム送信バッファのチェック
	if (midi.rtnum > 0) {
		// リアルタイム送信バッファのデータを送信バッファへ転送
		ASSERT(midi.rtnum <= 4);
		if (midi.tcr & 1) {
			InsertTrans(midi.rtbuf[midi.rtread]);
		}
		midi.rtnum--;
		midi.rtread = (midi.rtread + 1) & 3;

		// 送信BUSY、無送信なし
		midi.tbs = TRUE;
		midi.tcn = 0;
		return;
	}

	// 通常バッファのチェック
	if (midi.normnum > 0) {
		// 通常バッファのデータを送信バッファへ転送
		ASSERT(midi.normnum <= 16);
		if (midi.tcr & 1) {
			InsertTrans(midi.normbuf[midi.normread]);
		}
		midi.normnum--;
		midi.normread = (midi.normread + 1) & 15;

		// 通常バッファがクリアされれば、送信バッファエンプティ割り込み
		if (midi.normnum == 0) {
			Interrupt(6, TRUE);
		}

		// 送信BUSY、無送信なし
		midi.tbs = TRUE;
		midi.tcn = 0;
		return;
	}

	// 今回は送信しなかった。無送信時間を延長
	if (midi.tcn == 0xff) {
		// 81.92ms無送信
		if (midi.tcr & 1) {
			if (midi.dmr & 0x20) {
				// アクティブセンシング送信(DMRのビット指示による)
				InsertRT(0xfe);
			}
		}
	}
	midi.tcn++;
	if (midi.tcn >= 0x100) {
		midi.tcn = 0x100;
	}
}

//---------------------------------------------------------------------------
//
//	MIDIクロック検出
//
//---------------------------------------------------------------------------
void FASTCALL MIDI::Clock()
{
	ASSERT(this);
	ASSERT_DIAG();

	// クリックカウンタ
	midi.ctr--;
	if (midi.ctr == 0) {
		// リロード
		if (midi.cdr == 0) {
			midi.ctr = 0x80;
		}
		else {
			midi.ctr = midi.cdr;
		}

		// 割り込み
		if ((midi.imr & 8) == 0) {
			Interrupt(1, TRUE);
		}
	}
}

//---------------------------------------------------------------------------
//
//	汎用タイマコールバック
//
//---------------------------------------------------------------------------
void FASTCALL MIDI::General()
{
	DWORD hus;

	ASSERT(this);
	ASSERT_DIAG();

	// 割り込み発生
	Interrupt(7, TRUE);

	// GTRが変化していれば、時間を再設定
	hus = midi.gtr << 4;
	if (event[2].GetTime() != hus) {
		event[2].SetTime(hus);
	}
}

//---------------------------------------------------------------------------
//
//	送信バッファへ挿入
//
//---------------------------------------------------------------------------
void FASTCALL MIDI::InsertTrans(DWORD data)
{
	ASSERT(this);
	ASSERT(data < 0x100);
	ASSERT_DIAG();

#if defined(MIDI_LOG)
	LOG1(Log::Normal, "送信バッファ挿入 $%02X", data);
#endif	// MIDI_LOG

	// ロック
	sync->Lock();

	// データ挿入
	midi.transbuf[midi.transwrite].data = data;
	midi.transbuf[midi.transwrite].vtime = scheduler->GetTotalTime();
	midi.transwrite = (midi.transwrite + 1) & (TransMax - 1);
	midi.transnum++;
	if (midi.transnum > TransMax) {
		// 送信バッファのオーバーフローは無視(I/Oエミュレーションを行わない場合)
		midi.transnum = TransMax;
		midi.transread = midi.transwrite;
	}

	// アンロック
	sync->Unlock();
}

//---------------------------------------------------------------------------
//
//	受信バッファへ挿入
//
//---------------------------------------------------------------------------
void FASTCALL MIDI::InsertRecv(DWORD data)
{
	ASSERT(this);
	ASSERT(data < 0x100);
	ASSERT_DIAG();

#if defined(MIDI_LOG)
	LOG1(Log::Normal, "受信バッファ挿入 $%02X", data);
#endif	// MIDI_LOG

	// ロック
	sync->Lock();

	// データ挿入
	midi.recvbuf[midi.recvwrite].data = data;
	midi.recvbuf[midi.recvwrite].vtime = scheduler->GetTotalTime();
	midi.recvwrite = (midi.recvwrite + 1) & (RecvMax - 1);
	midi.recvnum++;
	if (midi.recvnum > RecvMax) {
		LOG0(Log::Warning, "受信バッファ オーバーフロー");
		midi.recvnum = RecvMax;
		midi.recvread = midi.recvwrite;
	}

	// アンロック
	sync->Unlock();
}

//---------------------------------------------------------------------------
//
//	通常バッファへ挿入
//
//---------------------------------------------------------------------------
void FASTCALL MIDI::InsertNorm(DWORD data)
{
	ASSERT(this);
	ASSERT(data < 0x100);
	ASSERT_DIAG();

#if defined(MIDI_LOG)
	LOG1(Log::Normal, "通常バッファ挿入 $%02X", data);
#endif	// MIDI_LOG

	// ロック
	sync->Lock();

	// データ挿入
	midi.normbuf[midi.normwrite] = data;
	midi.normwrite = (midi.normwrite + 1) & 15;
	midi.normnum++;
	midi.normtotal++;
	if (midi.normnum > 16) {
		LOG0(Log::Warning, "通常バッファ オーバーフロー");
		midi.normnum = 16;
		midi.normread = midi.normwrite;
	}

	// アンロック
	sync->Unlock();
}

//---------------------------------------------------------------------------
//
//	リアルタイム送信バッファへ挿入
//
//---------------------------------------------------------------------------
void FASTCALL MIDI::InsertRT(DWORD data)
{
	ASSERT(this);
	ASSERT(data < 0x100);
	ASSERT_DIAG();

#if defined(MIDI_LOG)
	LOG1(Log::Normal, "リアルタイム送信バッファ挿入 $%02X", data);
#endif	// MIDI_LOG

	// ロック
	sync->Lock();

	// データ挿入
	midi.rtbuf[midi.rtwrite] = data;
	midi.rtwrite = (midi.rtwrite + 1) & 3;
	midi.rtnum++;
	midi.rttotal++;
	if (midi.rtnum > 4) {
		LOG0(Log::Warning, "リアルタイム送信バッファ オーバーフロー");
		midi.rtnum = 4;
		midi.rtread = midi.rtwrite;
	}

	// アンロック
	sync->Unlock();
}

//---------------------------------------------------------------------------
//
//	一般バッファへ挿入
//
//---------------------------------------------------------------------------
void FASTCALL MIDI::InsertStd(DWORD data)
{
	ASSERT(this);
	ASSERT(data < 0x100);
	ASSERT_DIAG();

#if defined(MIDI_LOG)
	LOG1(Log::Normal, "一般バッファ挿入 $%02X", data);
#endif	// MIDI_LOG

	// ロック
	sync->Lock();

	// データ挿入
	midi.stdbuf[midi.stdwrite] = data;
	midi.stdwrite = (midi.stdwrite + 1) & 0x7f;
	midi.stdnum++;
	midi.stdtotal++;
	if (midi.stdnum > 0x80) {
#if defined(MIDI_LOG)
		LOG0(Log::Warning, "一般バッファ オーバーフロー");
#endif	// MIDI_LOG
		midi.stdnum = 0x80;
		midi.stdread = midi.stdwrite;

		// 受信ステータス設定(オーバーフロー)
		midi.rsr |= 0x40;
	}

	// アンロック
	sync->Unlock();

	// 受信ステータス設定(受信データあり)
	midi.rsr |= 0x80;

	// 割り込み発生(エンプティ→受信データあり)
	if (midi.stdnum == 1) {
		Interrupt(5, TRUE);
	}
}

//---------------------------------------------------------------------------
//
//	リアルタイム受信バッファへ挿入
//
//---------------------------------------------------------------------------
void FASTCALL MIDI::InsertRR(DWORD data)
{
	ASSERT(this);
	ASSERT(data < 0x100);
	ASSERT_DIAG();

#if defined(MIDI_LOG)
	LOG1(Log::Normal, "リアルタイム受信バッファ挿入 $%02X", data);
#endif	// MIDI_LOG

	// ロック
	sync->Lock();

	// データ挿入
	midi.rrbuf[midi.rrwrite] = data;
	midi.rrwrite = (midi.rrwrite + 1) & 3;
	midi.rrnum++;
	midi.rrtotal++;
	if (midi.rrnum > 4) {
#if defined(MIDI_LOG)
		LOG0(Log::Warning, "リアルタイム受信バッファ オーバーフロー");
#endif	// MIDI_LOG
		midi.rrnum = 4;
		midi.rrread = midi.rrwrite;
	}

	// アンロック
	sync->Unlock();
}

//---------------------------------------------------------------------------
//
//	送信バッファ個数取得
//
//---------------------------------------------------------------------------
DWORD FASTCALL MIDI::GetTransNum() const
{
	ASSERT(this);
	ASSERT_DIAG();

	// 4バイトデータ読みのため、ロックしない
	// (1回のメモリバスアクセスでデータ確定することが前提)
	return  midi.transnum;
}

//---------------------------------------------------------------------------
//
//	送信バッファデータ取得
//
//---------------------------------------------------------------------------
const MIDI::mididata_t* FASTCALL MIDI::GetTransData(DWORD proceed)
{
	const mididata_t *ptr;
	int offset;

	ASSERT(this);
	ASSERT(proceed < TransMax);
	ASSERT_DIAG();

	// ロック
	sync->Lock();

	// 取得
	offset = midi.transread;
	offset += proceed;
	offset &= (TransMax - 1);
	ptr = &(midi.transbuf[offset]);

	// アンロック
	sync->Unlock();

	// 返す
	return ptr;
}

//---------------------------------------------------------------------------
//
//	送信バッファ削除
//
//---------------------------------------------------------------------------
void FASTCALL MIDI::DelTransData(DWORD number)
{
	ASSERT(this);
	ASSERT(number < TransMax);
	ASSERT_DIAG();

	// ロック
	sync->Lock();

	// 個数調整
	if (number > midi.transnum) {
		number = midi.transnum;
	}

	// 個数とreadポインタを変更
	midi.transnum -= number;
	midi.transread = (midi.transread + number) & (TransMax - 1);

	// アンロック
	sync->Unlock();
}

//---------------------------------------------------------------------------
//
//	送信バッファクリア
//
//---------------------------------------------------------------------------
void FASTCALL MIDI::ClrTransData()
{
	ASSERT(this);
	ASSERT_DIAG();

	// ロック
	sync->Lock();

	// クリア
	midi.transnum = 0;
	midi.transread = 0;
	midi.transwrite = 0;

	// アンロック
	sync->Unlock();
}

//---------------------------------------------------------------------------
//
//	受信バッファデータ設定
//
//---------------------------------------------------------------------------
void FASTCALL MIDI::SetRecvData(const BYTE *ptr, DWORD length)
{
	DWORD lp;

	ASSERT(this);
	ASSERT(ptr);
	ASSERT_DIAG();

	// ボードIDが0であれば何もしない
	if (midi.bid == 0) {
		return;
	}

	// 受信できる場合のみ、受け付ける
	if (midi.rcr & 1) {
		// 受信バッファへ挿入
		for (lp=0; lp<length; lp++) {
			InsertRecv((DWORD)ptr[lp]);
		}
	}

	// 受信できる、できないにかかわらず、THRU指定があれば送信バッファへ
	if (midi.trr & 0x40) {
		for (lp=0; lp<length; lp++) {
			InsertTrans((DWORD)ptr[lp]);
		}
	}
}

//---------------------------------------------------------------------------
//
//	受信ディレイ設定
//
//---------------------------------------------------------------------------
void FASTCALL MIDI::SetRecvDelay(int delay)
{
	ASSERT(this);
	ASSERT(delay >= 0);
	ASSERT_DIAG();

	// hus単位であること
	recvdelay = delay;
}

//---------------------------------------------------------------------------
//
//	レジスタリセット
//
//---------------------------------------------------------------------------
void FASTCALL MIDI::ResetReg()
{
	ASSERT(this);
	ASSERT_DIAG();

#if defined(MIDI_LOG)
	LOG0(Log::Normal, "レジスタリセット");
#endif	// MIDI_LOG

	// 割り込み
	midi.vector = -1;

	// MCSレジスタ(一般)
	midi.wdr = 0;
	midi.rgr = 0xff;

	// MCSレジスタ(割り込み)
	// IMRはbit1を立てておく(スーパーハングオン)
	midi.ivr = 0x10;
	midi.isr = 0;
	midi.imr = 0x02;
	midi.ier = 0;

	// MCSレジスタ(リアルタイムメッセージ)
	midi.dmr = 0;
	midi.dcr = 0;

	// MCSレジスタ(受信)
	midi.rrr = 0;
	midi.rmr = 0;
	midi.rcn = 0;
	midi.amr = 0;
	midi.adr = 0;
	midi.asr = 0;
	midi.rsr = 0;
	midi.rcr = 0;

	// MCSレジスタ(送信)
	midi.trr = 0;
	midi.tmr = 0;
	midi.tbs = FALSE;
	midi.tcn = 0;
	midi.tcr = 0;

	// MCSレジスタ(FSK)
	midi.fsr = 0;
	midi.fcr = 0;

	// MCSレジスタ(カウンタ)
	midi.ccr = 2;
	midi.cdr = 0;
	midi.ctr = 0x80;
	midi.srr = 0;
	midi.scr = 1;
	midi.sct = 1;
	midi.spr = 0;
	midi.str = 0xffff8000;
	midi.gtr = 0;
	midi.mtr = 0;

	// MCSレジスタ(GPIO)
	midi.edr = 0;
	midi.eor = 0;
	midi.eir = 0;

	// 通常バッファ
	midi.normnum = 0;
	midi.normread = 0;
	midi.normwrite = 0;
	midi.normtotal = 0;

	// リアルタイム送信バッファ
	midi.rtnum = 0;
	midi.rtread = 0;
	midi.rtwrite = 0;
	midi.rttotal = 0;

	// 一般バッファ
	midi.stdnum = 0;
	midi.stdread = 0;
	midi.stdwrite = 0;
	midi.stdtotal = 0;

	// リアルタイム受信バッファ
	midi.rrnum = 0;
	midi.rrread = 0;
	midi.rrwrite = 0;
	midi.rrtotal = 0;

	// 送信バッファ
	sync->Lock();
	midi.transnum = 0;
	midi.transread = 0;
	midi.transwrite = 0;
	sync->Unlock();

	// 受信バッファ
	sync->Lock();
	midi.recvnum = 0;
	midi.recvread = 0;
	midi.recvwrite = 0;
	sync->Unlock();

	// イベント停止(汎用タイマ)
	event[2].SetTime(0);

	// イベント開始(MIDIクロック)(OPMDRV3.X)
	event[1].SetDesc("Clock");
	event[1].SetTime(0x40000);

	// エクスクルーシブカウンタ
	ex_cnt[0] = 0;
	ex_cnt[1] = 0;
	ex_cnt[2] = 0;
	ex_cnt[3] = 0;

	// 全ての割り込みをキャンセル
	IntCheck();

	// 通常バッファエンプティ割り込み(実際はISRのみ変化)
	Interrupt(6, TRUE);
}

//---------------------------------------------------------------------------
//
//	レジスタ読み出し
//
//---------------------------------------------------------------------------
DWORD FASTCALL MIDI::ReadReg(DWORD reg)
{
	DWORD group;

	ASSERT(this);
	ASSERT(reg < 8);
	ASSERT_DIAG();

	// アクセスフラグON
	if (!midi.access) {
		event[0].SetTime(640);
		midi.access = TRUE;
	}

	// ウェイト
	scheduler->Wait(2);

	switch (reg) {
		// IVR
		case 0:
#if defined(MIDI_LOG)
			LOG1(Log::Normal, "割り込みベクタ読み出し $%02X", midi.ivr);
#endif	// MIDI_LOG
			return midi.ivr;

		// RGR
		case 1:
			return midi.rgr;

		// ISR
		case 2:
#if defined(MIDI_LOG)
			LOG1(Log::Normal, "割り込みサービスレジスタ読み出し $%02X", midi.isr);
#endif	// MIDI_LOG
			return midi.isr;

		// ICR
		case 3:
			LOG1(Log::Normal, "レジスタ読み出し R%d", reg);
			return midi.wdr;
	}

	// グループから、レジスタ番号を作成
	group = midi.rgr & 0x0f;
	if (group >= 0x0a) {
		return 0xff;
	}

	// レジスタ番号生成
	reg += (group * 10);

	// レジスタ別(レジスタ4以降)
	switch (reg) {
		// DSR
		case 16:
			return GetDSR();

		// RSR
		case 34:
			return GetRSR();

		// RDR(更新あり)
		case 36:
			return GetRDR();

		// TSR
		case 54:
			return GetTSR();

		// FSR
		case 64:
			return GetFSR();

		// SRR
		case 74:
			return GetSRR();

		// EIR
		case 96:
			return GetEIR();
	}

	LOG1(Log::Normal, "レジスタ読み出し R%d", reg);
	return midi.wdr;
}

//---------------------------------------------------------------------------
//
//	レジスタ書き込み
//
//---------------------------------------------------------------------------
void FASTCALL MIDI::WriteReg(DWORD reg, DWORD data)
{
	DWORD group;

	ASSERT(this);
	ASSERT(reg < 8);
	ASSERT(data < 0x100);
	ASSERT_DIAG();

	// アクセスフラグON
	if (!midi.access) {
		event[0].SetTime(640);
		midi.access = TRUE;
	}

	// ウェイト
	scheduler->Wait(2);

	// 書き込みデータを保存(未定義のレジスタで読める)
	midi.wdr = data;

	// レジスタ別
	switch (reg) {
		// IVR
		case 0:
			LOG2(Log::Normal, "レジスタ書き込み R%d <- $%02X", reg, data);
			return;

		// RGR
		case 1:
			// 最上位ビットはレジスタリセット
			if (data & 0x80) {
				ResetReg();
			}
			midi.rgr = data;
			return;

		// ISR
		case 2:
			LOG2(Log::Normal, "レジスタ書き込み R%d <- $%02X", reg, data);
			return;

		// ICR
		case 3:
			SetICR(data);
			return;
	}

	// グループから、レジスタ番号を作成
	group = midi.rgr & 0x0f;
	if (group >= 0x0a) {
		return;
	}

	// レジスタ番号生成
	reg += (group * 10);

	// レジスタ別(レジスタ4以降)
	switch (reg) {
		// IOR
		case 4:
			SetIOR(data);
			return;

		// IMR
		case 5:
			SetIMR(data);
			return;

		// IER
		case 6:
			SetIER(data);
			return;

		// DMR
		case 14:
			SetDMR(data);
			return;

		// DCR
		case 15:
			SetDCR(data);
			return;

		// DNR
		case 17:
			SetDNR(data);
			return;

		// RRR
		case 24:
			SetRRR(data);
			return;

		// RMR
		case 25:
			SetRMR(data);
			return;

		// AMR
		case 26:
			SetAMR(data);
			return;

		// ADR
		case 27:
			SetADR(data);
			return;

		// RCR
		case 35:
			SetRCR(data);
			return;

		// TRR
		case 44:
			SetTRR(data);
			return;

		// TMR
		case 45:
			SetTMR(data);
			return;

		// TCR
		case 55:
			SetTCR(data);
			return;

		// TDR
		case 56:
			SetTDR(data);
			return;

		// FCR
		case 65:
			SetFCR(data);
			return;

		// CCR
		case 66:
			SetCCR(data);
			return;

		// CDR
		case 67:
			SetCDR(data);
			return;

		// SCR
		case 75:
			SetSCR(data);
			return;

		// SPR(L)
		case 76:
			SetSPR(data, FALSE);
			return;

		// SPR(H)
		case 77:
			SetSPR(data, TRUE);
			return;

		// GTR(L)
		case 84:
			SetGTR(data, FALSE);
			return;

		// GTR(H)
		case 85:
			SetGTR(data, TRUE);
			return;

		// MTR(L)
		case 86:
			SetMTR(data, FALSE);
			return;

		// MTR(H)
		case 87:
			SetMTR(data, TRUE);
			return;

		// EDR
		case 94:
			SetEDR(data);
			return;

		// EOR
		case 96:
			SetEOR(data);
			return;
	}

	LOG2(Log::Normal, "レジスタ書き込み R%d <- $%02X", reg, data);
}

//---------------------------------------------------------------------------
//
//	レジスタ読み出し(ReadOnly)
//
//---------------------------------------------------------------------------
DWORD FASTCALL MIDI::ReadRegRO(DWORD reg) const
{
	DWORD group;

	ASSERT(this);
	ASSERT(reg < 8);
	ASSERT_DIAG();

	switch (reg) {
		// IVR
		case 0:
			return midi.ivr;

		// RGR
		case 1:
			return midi.rgr;

		// ISR
		case 2:
			return midi.isr;

		// ICR
		case 3:
			return midi.wdr;
	}

	// グループから、レジスタ番号を作成
	group = midi.rgr & 0x0f;
	if (group >= 0x0a) {
		return 0xff;
	}

	// レジスタ番号生成
	reg += (group * 10);

	// レジスタ別(レジスタ4以降)
	switch (reg) {
		// DSR
		case 16:
			return GetDSR();

		// RSR
		case 34:
			return GetRSR();

		// RDR(ReadOnly)
		case 36:
			return GetRDRRO();

		// TSR
		case 54:
			return GetTSR();

		// FSR
		case 64:
			return GetFSR();

		// SRR
		case 74:
			return GetSRR();

		// EIR
		case 96:
			return GetEIR();
	}

	return midi.wdr;
}

//---------------------------------------------------------------------------
//
//	ICR設定
//
//---------------------------------------------------------------------------
void FASTCALL MIDI::SetICR(DWORD data)
{
	ASSERT(this);
	ASSERT(data < 0x100);
	ASSERT_DIAG();

#if defined(MIDI_LOG)
	LOG1(Log::Normal, "割り込みクリアレジスタ設定 $%02X", data);
#endif

	// ビット反転データを使って、インサービスをクリア
	midi.isr &= ~data;

	// 割り込みチェック
	IntCheck();
}

//---------------------------------------------------------------------------
//
//	IOR設定
//
//---------------------------------------------------------------------------
void FASTCALL MIDI::SetIOR(DWORD data)
{
	ASSERT(this);
	ASSERT(data < 0x100);
	ASSERT_DIAG();

	// 上位3bitのみ有効
	data &= 0xe0;

	LOG1(Log::Detail, "割り込みベクタベース $%02X", data);

	// IVRを変更する
	midi.ivr &= 0x1f;
	midi.ivr |= data;

	// 割り込みチェック
	IntCheck();
}

//---------------------------------------------------------------------------
//
//	IMR設定
//
//---------------------------------------------------------------------------
void FASTCALL MIDI::SetIMR(DWORD data)
{
	ASSERT(this);
	ASSERT(data < 0x100);
	ASSERT_DIAG();

	data &= 0x0f;

	if (midi.imr != data) {
#if defined(MIDI_LOG)
		LOG1(Log::Normal, "割り込みモードレジスタ設定 $%02X", data);
#endif	// MIDI_LOG
		midi.imr = data;

		// 割り込みチェック
		IntCheck();
	}
}

//---------------------------------------------------------------------------
//
//	IER設定
//
//---------------------------------------------------------------------------
void FASTCALL MIDI::SetIER(DWORD data)
{
	ASSERT(this);
	ASSERT(data < 0x100);
	ASSERT_DIAG();

	if (midi.ier != data) {
#if defined(MIDI_LOG)
		LOG1(Log::Normal, "割り込み許可レジスタ設定 $%02X", data);
#endif	// MIDI_LOG

		// 設定
		midi.ier = data;

		// 割り込みチェック
		IntCheck();
	}
}

//---------------------------------------------------------------------------
//
//	DMR設定
//
//---------------------------------------------------------------------------
void FASTCALL MIDI::SetDMR(DWORD data)
{
	ASSERT(this);
	ASSERT(data < 0x100);
	ASSERT_DIAG();

	// 有効データだけ残す
	data &= 0x3f;

	if (midi.dmr != data) {
#if defined(MIDI_LOG)
		LOG1(Log::Normal, "リアルタイムメッセージモードレジスタ設定 $%02X", data);
#endif

		// 設定
		midi.dmr = data;
	}
}

//---------------------------------------------------------------------------
//
//	DCR設定
//
//---------------------------------------------------------------------------
void FASTCALL MIDI::SetDCR(DWORD data)
{
	DWORD msg;

	ASSERT(this);
	ASSERT(data < 0x100);
	ASSERT_DIAG();

#if defined(MIDI_LOG)
	LOG1(Log::Normal, "リアルタイムメッセージコントロールレジスタ設定 $%02X", data);
#endif

	// 設定
	midi.dcr = data;

	// メッセージ作成
	msg = data & 0x07;
	msg += 0xf8;
	if (msg >= 0xfe) {
		LOG1(Log::Warning, "不正なリアルタイムメッセージ $%02X", msg);
		return;
	}

	// MIDIクロックメッセージはDMRのbit2で分ける
	if (msg == 0xf8) {
		if ((midi.dmr & 4) == 0) {
			// MIDIクロックタイマ、$F8受信などでクロックとする
			return;
		}

		// このタイミングでMIDIクロック受信とする
		if (midi.rcr & 1) {
			if (midi.dmr & 8) {
				// リアルタイム受信バッファへ挿入
				InsertRR(0xf8);

				// クロック処理
				Clock();

				// バッファチェック
				CheckRR();
			}
		}
		return;
	}
	else {
		// それ以外のメッセージは、リアルタイム送信バッファ宛か
		if ((data & 0x80) == 0) {
			return;
		}
	}

	// リアルタイム送信バッファへデータを挿入
	InsertRT(msg);
}

//---------------------------------------------------------------------------
//
//	DSR取得
//
//---------------------------------------------------------------------------
DWORD FASTCALL MIDI::GetDSR() const
{
	ASSERT(this);
	ASSERT_DIAG();

	// FIFO-IRxのデータを読み出す。データがなければ0
	if (midi.rrnum == 0) {
		return 0;
	}

#if defined(MIDI_LOG)
	LOG1(Log::Normal, "FIFO-IRx取得 $%02X", midi.rrbuf[midi.rrread]);
#endif	// MIDI_LOG

	return midi.rrbuf[midi.rrread];
}

//---------------------------------------------------------------------------
//
//	DNR設定
//
//---------------------------------------------------------------------------
void FASTCALL MIDI::SetDNR(DWORD data)
{
	ASSERT(this);
	ASSERT(data < 0x100);
	ASSERT_DIAG();

	if (data & 0x01) {
#if defined(MIDI_LOG)
		LOG0(Log::Normal, "FIFO-IRx更新");
#endif	// MIDI_LOG

		// リアルタイム受信バッファを更新
		if (midi.rrnum > 0) {
			midi.rrnum--;
			midi.rrread = (midi.rrread + 1) & 3;
		}

		// リアルタイム受信バッファ割り込みチェック
		CheckRR();
	}
}

//---------------------------------------------------------------------------
//
//	RRR設定
//
//---------------------------------------------------------------------------
void FASTCALL MIDI::SetRRR(DWORD data)
{
	ASSERT(this);
	ASSERT(data < 0x100);
	ASSERT_DIAG();

	// 有効部分だけ残す
	data &= 0x38;

	if (midi.rrr != data) {
#if defined(MIDI_LOG)
		LOG1(Log::Normal, "受信レート設定 $%02X", data);
#endif	// MIDI_LOG

		// 設定
		midi.rrr = data;
	}

	// 受信レートチェック
	if (midi.rrr != 0x08) {
		LOG1(Log::Warning, "受信レート異常 $%02X", midi.rrr);
	}
}

//---------------------------------------------------------------------------
//
//	RMR設定
//
//---------------------------------------------------------------------------
void FASTCALL MIDI::SetRMR(DWORD data)
{
	ASSERT(this);
	ASSERT(data < 0x100);
	ASSERT_DIAG();

	// 有効部分だけ残す
	data &= 0x3f;

	if (midi.rmr != data) {
#if defined(MIDI_LOG)
		LOG1(Log::Normal, "受信パラメータ設定 $%02X", data);
#endif	// MIDI_LOG

		// 設定
		midi.rmr = data;
	}

	// 受信パラメータチェック
	if ((midi.rmr & 0x32) != 0) {
		LOG1(Log::Warning, "受信パラメータ異常 $%02X", midi.rmr);
	}
}

//---------------------------------------------------------------------------
//
//	AMR設定
//
//---------------------------------------------------------------------------
void FASTCALL MIDI::SetAMR(DWORD data)
{
	ASSERT(this);
	ASSERT(data < 0x100);
	ASSERT_DIAG();

	if (midi.amr != data) {
#if defined(MIDI_LOG)
		if (data & 0x80) {
			LOG1(Log::Normal, "受信時メーカID+デバイスIDチェック。メーカID $%02X", data & 0x7f);
		}
		else {
			LOG1(Log::Normal, "受信時メーカIDのみチェック。メーカID $%02X", data & 0x7f);
		}
#endif	// MIDI_LOG

		midi.amr = data;
	}
}

//---------------------------------------------------------------------------
//
//	ADR設定
//
//---------------------------------------------------------------------------
void FASTCALL MIDI::SetADR(DWORD data)
{
	ASSERT(this);
	ASSERT(data < 0x100);
	ASSERT_DIAG();

	if (midi.adr != data) {
#if defined(MIDI_LOG)
		if (data & 0x80) {
			LOG1(Log::Normal, "ブロードキャスト許可。デバイスID $%02X", data & 0x7f);
		}
		else {
			LOG1(Log::Normal, "ブロードキャスト許可。デバイスID $%02X", data & 0x7f);
		}
#endif	// MIDI_LOG

		midi.adr = data;
	}
}

//---------------------------------------------------------------------------
//
//	RSR取得
//
//---------------------------------------------------------------------------
DWORD FASTCALL MIDI::GetRSR() const
{
	ASSERT(this);
	ASSERT_DIAG();

#if defined(MIDI_LOG)
	LOG1(Log::Normal, "受信バッファステータス取得 $%02X", midi.rsr);
#endif	// MIDI_LOG

	return midi.rsr;
}

//---------------------------------------------------------------------------
//
//	RCR設定
//
//---------------------------------------------------------------------------
void FASTCALL MIDI::SetRCR(DWORD data)
{
	ASSERT(this);
	ASSERT(data < 0x100);
	ASSERT_DIAG();

	// 有効部分だけ残す
	data &= 0xdf;

#if defined(MIDI_LOG)
	LOG1(Log::Normal, "受信コントロール $%02X", data);
#endif	// MIDI_LOG

	// データ設定
	midi.rcr = data;

	// オフラインフラグクリア
	if (midi.rcr & 0x04) {
		midi.rsr &= ~0x04;

		// オフライン割り込み解除
		if ((midi.imr & 4) == 0) {
			Interrupt(4, FALSE);
		}
	}

	// ブレークフラグクリア
	if (midi.rcr & 0x08) {
		midi.rsr &= ~0x08;

		// ブレーク割り込み解除
		if (midi.imr & 4) {
			Interrupt(4, FALSE);
		}
	}

	// オーバーフローフラグクリア
	if (midi.rcr & 0x40) {
		midi.rsr &= ~0x40;
	}

	// 受信バッファクリア
	if (midi.rcr & 0x80) {
		// 通常バッファ
		midi.stdnum = 0;
		midi.stdread = 0;
		midi.stdwrite = 0;

		// リアルタイム受信バッファ
		midi.rrnum = 0;
		midi.rrread = 0;
		midi.rrwrite = 0;

		// 割り込みオフ
		Interrupt(5, FALSE);
		Interrupt(0, FALSE);
		if (midi.imr & 8) {
			Interrupt(1, FALSE);
		}
	}

	// アドレスハンタイネーブル
	if (midi.rcr & 0x02) {
#if defined(MIDI_LOG)
		LOG0(Log::Normal, "アドレスハンタ イネーブル");
#endif	// MIDI_LOG
		midi.rsr |= 0x02;
	}
	else {
#if defined(MIDI_LOG)
		LOG0(Log::Normal, "アドレスハンタ ディセーブル");
#endif	// MIDI_LOG
		midi.rsr &= ~0x02;
	}

	// 受信イネーブル
	if (midi.rcr & 0x01) {
#if defined(MIDI_LOG)
		LOG0(Log::Normal, "受信イネーブル");
#endif	// MIDI_LOG
	}
	else {
#if defined(MIDI_LOG)
		LOG0(Log::Normal, "受信ディセーブル");
#endif	// MIDI_LOG
	}
}

//---------------------------------------------------------------------------
//
//	RDR取得
//
//---------------------------------------------------------------------------
DWORD FASTCALL MIDI::GetRDR()
{
	DWORD data;

	ASSERT(this);
	ASSERT_DIAG();

	// データはwdr(未検証)
	data = midi.wdr;

	// バッファに有効データがあるか
	if (midi.stdnum > 0) {
#if defined(MIDI_LOG)
		LOG1(Log::Normal, "一般バッファ取得 $%02X", midi.stdbuf[midi.stdread]);
#endif	// MIDI_LOG

		// バッファからデータを得る
		data = midi.stdbuf[midi.stdread];

		// バッファ処理
		midi.stdnum--;
		midi.stdread = (midi.stdread + 1) & 0x7f;

		// 割り込み解除
		Interrupt(5, FALSE);

		// バッファが空になれば
		if (midi.stdnum == 0) {
			// 受信バッファ有効フラグ解除
			midi.rsr &= 0x7f;
		}
		else {
			// 受信バッファ有効フラグ設定
			midi.rsr |= 0x80;
		}

		// データチェック(動作確認用に、エクスクルーシブカウンタを上げる)
		if (data == 0xf0) {
			ex_cnt[2]++;
		}
		if (data == 0xf7) {
			ex_cnt[3]++;
		}
	}

	return data;
}

//---------------------------------------------------------------------------
//
//	RDR取得 (Read Only)
//
//---------------------------------------------------------------------------
DWORD FASTCALL MIDI::GetRDRRO() const
{
	DWORD data;

	ASSERT(this);
	ASSERT_DIAG();

	// データはwdr(未検証)
	data = midi.wdr;

	// バッファに有効データがあるか
	if (midi.stdnum > 0) {
		data = midi.stdbuf[midi.stdread];
	}

	return data;
}

//---------------------------------------------------------------------------
//
//	TRR設定
//
//---------------------------------------------------------------------------
void FASTCALL MIDI::SetTRR(DWORD data)
{
	ASSERT(this);
	ASSERT(data < 0x100);
	ASSERT_DIAG();

	// 有効部分だけ残す
	data &= 0x78;

	if (midi.trr != data) {
#if defined(MIDI_LOG)
		LOG1(Log::Normal, "送信レート設定 $%02X", data);
#endif	// MIDI_LOG

		// 設定
		midi.trr = data;
	}

	// 送信レートチェック
	if ((midi.trr & 0x38) != 0x08) {
		LOG1(Log::Warning, "送信レート異常 $%02X", midi.trr);
	}

#if defined(MIDI_LOG)
	if (midi.trr & 0x40) {
		LOG0(Log::Normal, "MIDI THRU設定");
	}
#endif	// MIDI_LOG
}

//---------------------------------------------------------------------------
//
//	TMR設定
//
//---------------------------------------------------------------------------
void FASTCALL MIDI::SetTMR(DWORD data)
{
	ASSERT(this);
	ASSERT(data < 0x100);
	ASSERT_DIAG();

	// 有効部分だけ残す
	data &= 0x3f;

	if (midi.tmr != data) {
#if defined(MIDI_LOG)
		LOG1(Log::Normal, "送信パラメータ設定 $%02X", data);
#endif	// MIDI_LOG

		// 設定
		midi.tmr = data;
	}

	// 送信パラメータチェック
	if ((midi.tmr & 0x32) != 0) {
		LOG1(Log::Warning, "送信パラメータ異常 $%02X", midi.tmr);
	}
}

//---------------------------------------------------------------------------
//
//	TSR取得
//
//---------------------------------------------------------------------------
DWORD FASTCALL MIDI::GetTSR() const
{
	DWORD data;

	ASSERT(this);
	ASSERT_DIAG();

	// クリア
	data = 0;

	// bit7:通常バッファ空フラグ
	if (midi.normnum == 0) {
		data |= 0x80;
	}

	// bit6:通常バッファデータ有無フラグ
	if (midi.normnum < 16) {
		data |= 0x40;
	}

	// bit2:無送信(81.920ms)フラグ
	if (midi.tcn >= 0x100) {
		ASSERT(midi.tcn == 0x100);
		data |= 0x04;
	}

	// bit0:トランスミッタBUSYフラグ
	if (midi.tbs) {
		data |= 0x01;
	}

	return data;
}

//---------------------------------------------------------------------------
//
//	TCR設定
//
//---------------------------------------------------------------------------
void FASTCALL MIDI::SetTCR(DWORD data)
{
	ASSERT(this);
	ASSERT(data < 0x100);
	ASSERT_DIAG();

	// 有効部分だけ残す
	data &= 0x8d;

#if defined(MIDI_LOG)
	if (midi.tcr != data) {
		LOG1(Log::Normal, "送信コントロール $%02X", data);
	}
#endif	// MIDI_LOG

	// 有効なビットのみ保存
	midi.tcr = data;

	// 通常バッファ、リアルタイム専用送信バッファ クリア
	if (data & 0x80) {
		// 通常バッファ
		midi.normnum = 0;
		midi.normread = 0;
		midi.normwrite = 0;

		// リアルタイム専用送信バッファ
		midi.rtnum = 0;
		midi.rtread = 0;
		midi.rtwrite = 0;

		// 割り込みオン
		Interrupt(6, TRUE);
	}

	// 無送信状態(81.920ms)フラグ クリア
	if (data & 0x04) {
		midi.tcn = 0;
	}

	// 送信イネーブル
#if defined(MIDI_LOG)
	if (data & 0x01) {
		LOG0(Log::Normal, "送信イネーブル");
	}
	else {
		LOG0(Log::Normal, "送信ディセーブル");
	}
#endif	// MIDI_LOG
}

//---------------------------------------------------------------------------
//
//	TDR設定
//
//---------------------------------------------------------------------------
void FASTCALL MIDI::SetTDR(DWORD data)
{
	ASSERT(this);
	ASSERT(data < 0x100);
	ASSERT_DIAG();

#if defined(MIDI_LOG)
	LOG1(Log::Normal, "送信バッファ書き込み $%02X", data);
#endif	// MIDI_LOG

	// データチェック(動作確認用に、エクスクルーシブカウンタを上げる)
	if (data == 0xf0) {
		ex_cnt[0]++;
	}
	if (data == 0xf7) {
		ex_cnt[1]++;
	}

	// 送信条件チェック(送信ディセーブルでも書き込める:ジオグラフシール)
	if ((midi.trr & 0x38) != 0x08) {
		// 送信レートが31250bpsでない
		return;
	}
	if ((midi.tmr & 0x32) != 0) {
		// 8bitデータ、1ストップビット、ノンパリティでない
		return;
	}

	// 通常バッファにデータを挿入
	InsertNorm(data);

	// 送信バッファエンプティ割り込みはクリア
	Interrupt(6, FALSE);
}

//---------------------------------------------------------------------------
//
//	FSR取得
//
//---------------------------------------------------------------------------
DWORD FASTCALL MIDI::GetFSR() const
{
	ASSERT(this);
	ASSERT_DIAG();

#if defined(MIDI_LOG)
	LOG1(Log::Normal, "テープSYNCステータス取得 $%02X", midi.fsr);
#endif	// MIDI_LOG

	return midi.fsr;
}

//---------------------------------------------------------------------------
//
//	FCR設定
//
//---------------------------------------------------------------------------
void FASTCALL MIDI::SetFCR(DWORD data)
{
	ASSERT(this);
	ASSERT(data < 0x100);
	ASSERT_DIAG();

	// 有効部分だけ残す
	data &= 0x9f;

	if (midi.fcr != data) {
#if defined(MIDI_LOG)
		LOG1(Log::Normal, "テープSYNCコマンド設定 $%02X", data);
#endif	// MIDI_LOG

		// 設定
		midi.fcr = data;
	}
}

//---------------------------------------------------------------------------
//
//	CCR設定
//
//---------------------------------------------------------------------------
void FASTCALL MIDI::SetCCR(DWORD data)
{
	ASSERT(this);
	ASSERT(data < 0x100);
	ASSERT_DIAG();

#if defined(MIDI_LOG)
	LOG1(Log::Normal, "クリックコントロールレジスタ設定 $%02X", data);
#endif	// MIDI_LOG

	// 下位2bitのみ有効
	data &= 0x03;

	// bit1は必ず1
	if ((data & 2) == 0) {
		LOG0(Log::Warning, "CLKMクロック周波数選択エラー");
	}

	// 設定
	midi.ccr = data;
}

//---------------------------------------------------------------------------
//
//	CDR設定
//
//---------------------------------------------------------------------------
void FASTCALL MIDI::SetCDR(DWORD data)
{
	ASSERT(this);
	ASSERT(data < 0x100);
	ASSERT_DIAG();

	// CDR設定
	if (midi.cdr != (data & 0x7f)) {
		midi.cdr = (data & 0x7f);

#if defined(MIDI_LOG)
		LOG1(Log::Normal, "クリックカウンタ時定数設定 $%02X", midi.cdr);
#endif	// MIDI_LOG
	}

	// bit7でCTRに値を即時ロード
	if (data & 0x80) {
		// CTR設定
		midi.ctr = midi.cdr;

#if defined(MIDI_LOG)
		LOG0(Log::Normal, "クリックカウンタ時定数設定をロード");
#endif	// MIDI_LOG
	}
}

//---------------------------------------------------------------------------
//
//	SRR取得
//
//---------------------------------------------------------------------------
DWORD FASTCALL MIDI::GetSRR() const
{
	ASSERT(this);
	ASSERT_DIAG();

#if defined(MIDI_LOG)
	LOG1(Log::Normal, "レコーディングカウンタ取得 $%02X", midi.srr);
#endif	// MIDI_LOG

	return midi.srr;
}

//---------------------------------------------------------------------------
//
//	SCR設定
//
//---------------------------------------------------------------------------
void FASTCALL MIDI::SetSCR(DWORD data)
{
	DWORD mtr;
	char desc[16];

	ASSERT(this);
	ASSERT(data < 0x100);
	ASSERT_DIAG();

	// 補間レート設定
	if (midi.scr != (data & 0x0f)) {
#if defined(MIDI_LOG)
		LOG1(Log::Normal, "MIDIクロック補間器設定 $%02X", data & 0x0f);
#endif	// MIDI_LOG
		midi.scr = (data & 0x0f);

		// レート0は設定禁止
		if (midi.scr == 0) {
			LOG0(Log::Warning, "MIDIクロック補間器 設定禁止値");
			midi.scr = 1;
		}

		// イベント再設定
		if (midi.mtr <= 1) {
			mtr = 0x40000;
		}
		else {
			mtr = midi.mtr << 4;
		}
		mtr /= midi.scr;
		if (event[1].GetTime() != mtr) {
			event[1].SetTime(mtr);
		}

		// SCTもチェック
		if (midi.sct > midi.scr) {
			midi.sct = midi.scr;
		}
		ASSERT(midi.sct > 0);

		// イベント文字列
		if (midi.scr == 1) {
			event[1].SetDesc("Clock");
		}
		else {
			sprintf(desc, "Clock (Div%d)", midi.scr);
			event[1].SetDesc(desc);
		}
	}

	// プレイバックカウンタクリア
	if (data & 0x10) {
#if defined(MIDI_LOG)
		LOG0(Log::Normal, "プレイバックカウンタクリア");
#endif	// MIDI_LOG
		midi.str = 0;
	}

	// プレイバックカウンタ加算
	if (data & 0x20) {
		// 加算
		midi.str += midi.spr;

		// オーバーフローチェック
		if ((midi.str >= 0x8000) && (midi.str < 0x10000)) {
			midi.str = 0x8000;
		}

#if defined(MIDI_LOG)
		LOG2(Log::Normal, "プレイバックカウンタ加算 $%04X->$%04X", midi.spr, midi.str);
#endif	// MIDI_LOG
	}
}

//---------------------------------------------------------------------------
//
//	SPR設定
//
//---------------------------------------------------------------------------
void FASTCALL MIDI::SetSPR(DWORD data, BOOL high)
{
	ASSERT(this);
	ASSERT(data < 0x100);
	ASSERT_DIAG();

	if (high) {
		// 上位
		midi.spr &= 0x00ff;
		midi.spr |= ((data & 0x7f) << 8);
	}
	else {
		// 下位
		midi.spr &= 0xff00;
		midi.spr |= data;
	}

#if defined(MIDI_LOG)
	LOG1(Log::Normal, "プレイバックカウンタ時定数設定 $%04X", midi.spr);
#endif	// MIDI_LOG
}

//---------------------------------------------------------------------------
//
//	GTR設定
//
//---------------------------------------------------------------------------
void FASTCALL MIDI::SetGTR(DWORD data, BOOL high)
{
	ASSERT(this);
	ASSERT(data < 0x100);
	ASSERT_DIAG();

	if (high) {
		// 上位
		midi.gtr &= 0x00ff;
		midi.gtr |= ((data & 0x3f) << 8);
	}
	else {
		// 下位
		midi.gtr &= 0xff00;
		midi.gtr |= data;
	}

#if defined(MIDI_LOG)
	LOG1(Log::Normal, "汎用タイマレジスタ設定 $%04X", midi.gtr);
#endif	// MIDI_LOG

	// bit7でデータをロード
	if (high && (data & 0x80)) {
#if defined(MIDI_LOG)
		LOG0(Log::Normal, "汎用タイマ設定値をロード");
#endif	// MIDI_LOG

		// データをロードして、タイマ開始
		event[2].SetTime(midi.gtr << 4);
	}
}

//---------------------------------------------------------------------------
//
//	MTR設定
//
//---------------------------------------------------------------------------
void FASTCALL MIDI::SetMTR(DWORD data, BOOL high)
{
	DWORD mtr;

	ASSERT(this);
	ASSERT(data < 0x100);
	ASSERT_DIAG();

	if (high) {
		// 上位
		midi.mtr &= 0x00ff;
		midi.mtr |= ((data & 0x3f) << 8);
	}
	else {
		// 下位
		midi.mtr &= 0xff00;
		midi.mtr |= data;
	}

#if defined(MIDI_LOG)
	LOG1(Log::Normal, "MIDIクロックタイマレジスタ設定 $%04X", midi.mtr);
#endif	// MIDI_LOG

	// bit7でデータをロード
	if (high) {
		if (midi.mtr <= 1) {
			mtr = 0x40000;
		}
		else {
			mtr = midi.mtr << 4;
		}

		// SCRで割った値
		ASSERT(midi.scr != 0);
		event[1].SetTime(mtr / midi.scr);
	}
}

//---------------------------------------------------------------------------
//
//	EDR設定
//
//---------------------------------------------------------------------------
void FASTCALL MIDI::SetEDR(DWORD data)
{
	ASSERT(this);
	ASSERT(data < 0x100);
	ASSERT_DIAG();

	if (midi.edr != data) {
#if defined(MIDI_LOG)
		LOG1(Log::Normal, "GPIOディレクション設定 $%02X", data);
#endif	// MIDI_LOG

		midi.edr = data;
	}
}

//---------------------------------------------------------------------------
//
//	EOR設定
//
//---------------------------------------------------------------------------
void FASTCALL MIDI::SetEOR(DWORD data)
{
	ASSERT(this);
	ASSERT(data < 0x100);
	ASSERT_DIAG();

	if (midi.eor != data) {
#if defined(MIDI_LOG)
		LOG1(Log::Normal, "GPIO出力データ設定 $%02X", data);
#endif	// MIDI_LOG

		midi.eor = data;
	}
}

//---------------------------------------------------------------------------
//
//	EIR取得
//
//---------------------------------------------------------------------------
DWORD FASTCALL MIDI::GetEIR() const
{
	ASSERT(this);
	ASSERT_DIAG();

#if defined(MIDI_LOG)
	LOG1(Log::Normal, "GPIO読み出し $%02X", midi.eir);
#endif	// MIDI_LOG

	return midi.eir;
}

//---------------------------------------------------------------------------
//
//	リアルタイム受信バッファチェック
//
//---------------------------------------------------------------------------
void FASTCALL MIDI::CheckRR()
{
	DWORD data;

	ASSERT(this);
	ASSERT_DIAG();

	// 受信バッファにデータがなければ
	if (midi.rrnum == 0) {
		// 割り込みオフ
		Interrupt(0, FALSE);
		if (midi.imr & 8) {
			Interrupt(1, FALSE);
		}
		return;
	}

	// 最も古いデータを得る
	data = midi.rrbuf[midi.rrread];

	// IRQ-1
	if (data == 0xf8) {
		if (midi.imr & 8) {
			Interrupt(1, TRUE);
			Interrupt(0, FALSE);
			return;
		}
	}

	// IRQ-0
	Interrupt(1, FALSE);
	if ((data >= 0xf9) && (data <= 0xfd)) {
		Interrupt(0, TRUE);
	}
	else {
		Interrupt(0, FALSE);
	}
}

//---------------------------------------------------------------------------
//
//	割り込み要求
//
//---------------------------------------------------------------------------
void FASTCALL MIDI::Interrupt(int type, BOOL flag)
{
	ASSERT(this);
	ASSERT((type >= 0) && (type <= 7));
	ASSERT_DIAG();

	// ISRの該当するビットを変化させる
	if (flag) {
		midi.isr |= (1 << type);
	}
	else {
		midi.isr &= ~(1 << type);
	}

	// 割り込みチェック
	IntCheck();
}

//---------------------------------------------------------------------------
//
//	割り込みACK
//
//---------------------------------------------------------------------------
void FASTCALL MIDI::IntAck(int level)
{
	ASSERT(this);
	ASSERT((level == 2) || (level == 4));
	ASSERT_DIAG();

	// 割り込みレベルが一致しなければ何もしない
	if (level != (int)midi.ilevel) {
#if defined(MIDI_LOG)
		LOG1(Log::Normal, "割り込みレベル異常 レベル%d", level);
#endif	// MIDI_LOG
		return;
	}

	// 割り込み要求中でなければ、何もしない
	if (midi.vector < 0) {
#if defined(MIDI_LOG)
		LOG0(Log::Normal, "要求していない割り込み");
#endif	// MIDI_LOG
		return;
	}

#if defined(MIDI_LOG)
	LOG1(Log::Normal, "割り込みACK レベル%d", level);
#endif	// MIDI_LOG

	// 要求ベクタなし
	midi.vector = -1;

	// 次の割り込みをスケジュール
	IntCheck();
}

//---------------------------------------------------------------------------
//
//	割り込みチェック
//
//---------------------------------------------------------------------------
void FASTCALL MIDI::IntCheck()
{
	int i;
	int vector;
	DWORD bit;

	ASSERT(this);
	ASSERT_DIAG();

	// 割り込みモードコントロールbit1=1でないと、68000割り込みに適合しない
	if ((midi.imr & 0x02) == 0) {
		// 割り込みがあればキャンセルする
		if (midi.vector != -1) {
			cpu->IntCancel(midi.ilevel);
			midi.ivr &= 0xe0;
			midi.ivr |= 0x10;
			midi.vector = -1;
		}
		return;
	}

	// ISRをチェック
	bit = 0x80;
	for (i=7; i>=0; i--) {
		if (midi.isr & bit) {
			if (midi.ier & bit) {
				// 割り込み発生。ベクタ算出
				vector = midi.ivr;
				vector &= 0xe0;
				vector += (i << 1);

				// 現在のものと違ったら、要求
				if (midi.vector != vector) {
					// 既に他のものが要求されていれば、キャンセルする
					if (midi.vector >= 0) {
						cpu->IntCancel(midi.ilevel);
					}
					// 割り込み要求
					cpu->Interrupt(midi.ilevel, vector);
					midi.vector = vector;
					midi.ivr = (DWORD)vector;

#if defined(MIDI_LOG)
					LOG3(Log::Normal, "割り込み要求 レベル%d タイプ%d ベクタ$%02X", midi.ilevel, i, vector);
#endif	// MIDI_LOG
				}
				return;
			}
		}

		// 次へ
		bit >>= 1;
	}

	// 発生させる割り込みはないので、キャンセル
	if (midi.vector != -1) {
		// 割り込みキャンセル
		cpu->IntCancel(midi.ilevel);
		midi.vector = -1;
		midi.ivr &= 0xe0;
		midi.ivr |= 0x10;
#if defined(MIDI_LOG)
		LOG0(Log::Normal, "割り込みキャンセル");
#endif	// MIDI_LOG
	}
}

//---------------------------------------------------------------------------
//
//	内部データ取得
//
//---------------------------------------------------------------------------
void FASTCALL MIDI::GetMIDI(midi_t *buffer) const
{
	ASSERT(this);
	ASSERT(buffer);
	ASSERT_DIAG();

	// 内部データをコピー
	*buffer = midi;
}

//---------------------------------------------------------------------------
//
//	エクスクルーシブカウント取得
//
//---------------------------------------------------------------------------
DWORD FASTCALL MIDI::GetExCount(int index) const
{
	ASSERT(this);
	ASSERT((index >= 0) && (index < 4));
	ASSERT_DIAG();

	return ex_cnt[index];
}
