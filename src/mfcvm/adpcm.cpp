//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 ＰＩ．(ytanaka@ipc-tokai.or.jp)
//	[ ADPCM(MSM6258V) ]
//
//---------------------------------------------------------------------------

#include "os.h"
#include "xm6.h"
#include "vm.h"
#include "log.h"
#include "schedule.h"
#include "dmac.h"
#include "fileio.h"
#include "config.h"
#include "adpcm.h"

//===========================================================================
//
//	ADPCM
//
//===========================================================================
//#define ADPCM_LOG

//---------------------------------------------------------------------------
//
//	コンストラクタ
//
//---------------------------------------------------------------------------
ADPCM::ADPCM(VM *p) : MemDevice(p)
{
	// デバイスIDを初期化
	dev.id = MAKEID('A', 'P', 'C', 'M');
	dev.desc = "ADPCM (MSM6258V)";

	// 開始アドレス、終了アドレス
	memdev.first = 0xe92000;
	memdev.last = 0xe93fff;

	// その他、コンストラクタで初期化すべきワーク
	memset(&adpcm, 0, sizeof(adpcm));
	adpcm.sync_rate = 882;
	adpcm.sync_step = 0x9c4000 / 882;
	adpcm.vol = 0;
	adpcm.enable = FALSE;
	adpcm.sound = TRUE;
	dmac = NULL;
	adpcmbuf = NULL;
}

//---------------------------------------------------------------------------
//
//	初期化
//
//---------------------------------------------------------------------------
BOOL FASTCALL ADPCM::Init()
{
	ASSERT(this);

	// 基本クラス
	if (!MemDevice::Init()) {
		return FALSE;
	}

	// DMAC取得
	ASSERT(!dmac);
	dmac = (DMAC*)vm->SearchDevice(MAKEID('D', 'M', 'A', 'C'));
	ASSERT(dmac);

	// バッファ確保
	try {
		adpcmbuf = new DWORD[ BufMax * 2 ];
	}
	catch (...) {
		return FALSE;
	}
	if (!adpcmbuf) {
		return FALSE;
	}
	memset(adpcmbuf, 0, sizeof(DWORD) * (BufMax * 2));

	// イベント作成
	event.SetDevice(this);
	event.SetDesc("Sampling");
	event.SetUser(0);
	event.SetTime(0);
	scheduler->AddEvent(&event);

	// 線形補間パラメータ初期化
	adpcm.interp = FALSE;

	// リセット時OPMIFからCTが初期化されるため、ここで初期化しておく
	adpcm.ratio = 0;
	adpcm.speed = 0x400;
	adpcm.clock = 8;

	// テーブル作成、音量設定
	MakeTable();
	SetVolume(52);

	// バッファ初期化
	InitBuf(adpcm.sync_rate * 50);

	// 速度初期化
	CalcSpeed();

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	クリーンアップ
//
//---------------------------------------------------------------------------
void FASTCALL ADPCM::Cleanup()
{
	ASSERT(this);

	// バッファ削除
	if (adpcmbuf) {
		delete[] adpcmbuf;
		adpcmbuf = NULL;
	}

	// 基本クラスへ
	MemDevice::Cleanup();
}

//---------------------------------------------------------------------------
//
//	リセット
//
//---------------------------------------------------------------------------
void FASTCALL ADPCM::Reset()
{
	ASSERT(this);
	ASSERT_DIAG();

	LOG0(Log::Normal, "リセット");

	// 内部ワーク初期化
	adpcm.play = FALSE;
	adpcm.rec = FALSE;
	adpcm.active = FALSE;
	adpcm.started = FALSE;
	adpcm.panpot = 0;
	adpcm.ratio = 0;
	adpcm.speed = 0x400;
	adpcm.data = 0;
	adpcm.offset = 0;
	adpcm.out = 0;
	adpcm.sample = 0;
	adpcm.clock = 8;
	CalcSpeed();

	// バッファ初期化
	InitBuf(adpcm.sync_rate * 50);

	// イベントを止める
	event.SetUser(0);
	event.SetTime(0);
	event.SetDesc("Sampling");
}

//---------------------------------------------------------------------------
//
//	セーブ
//
//---------------------------------------------------------------------------
BOOL FASTCALL ADPCM::Save(Fileio *fio, int ver)
{
	size_t sz;

	ASSERT(this);
	ASSERT(fio);
	ASSERT_DIAG();

	LOG0(Log::Normal, "セーブ");

	// サイズをセーブ
	sz = sizeof(adpcm_t);
	if (!fio->Write(&sz, sizeof(sz))) {
		return FALSE;
	}

	// 実体をセーブ
	if (!fio->Write(&adpcm, (int)sz)) {
		return FALSE;
	}

	// イベントをセーブ
	if (!event.Save(fio, ver)) {
		return FALSE;
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	ロード
//
//---------------------------------------------------------------------------
BOOL FASTCALL ADPCM::Load(Fileio *fio, int ver)
{
	size_t sz;

	ASSERT(this);
	ASSERT(fio);
	ASSERT_DIAG();

	LOG0(Log::Normal, "ロード");

	// サイズをロード、照合
	if (!fio->Read(&sz, sizeof(sz))) {
		return FALSE;
	}
	if (sz != sizeof(adpcm_t)) {
		return FALSE;
	}

	// 実体をロード
	if (!fio->Read(&adpcm, (int)sz)) {
		return FALSE;
	}

	// イベントをロード
	if (!event.Load(fio, ver)) {
		return FALSE;
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	設定適用
//
//---------------------------------------------------------------------------
void FASTCALL ADPCM::ApplyCfg(const Config *config)
{
	ASSERT(this);
	ASSERT(config);
	ASSERT_DIAG();

	LOG0(Log::Normal, "設定適用");

	// 線形補間
	adpcm.interp = config->adpcm_interp;
}

#if !defined(NDEBUG)
//---------------------------------------------------------------------------
//
//	診断
//
//---------------------------------------------------------------------------
void FASTCALL ADPCM::AssertDiag() const
{
	// 基本クラス
	MemDevice::AssertDiag();

	ASSERT(this);
	ASSERT(GetID() == MAKEID('A', 'P', 'C', 'M'));
	ASSERT(memdev.first == 0xe92000);
	ASSERT(memdev.last == 0xe93fff);
	ASSERT(dmac);
	ASSERT(dmac->GetID() == MAKEID('D', 'M', 'A', 'C'));
	ASSERT((adpcm.panpot >= 0) && (adpcm.panpot <= 3));
	ASSERT((adpcm.play == TRUE) || (adpcm.play == FALSE));
	ASSERT((adpcm.rec == TRUE) || (adpcm.rec == FALSE));
	ASSERT((adpcm.active == TRUE) || (adpcm.active == FALSE));
	ASSERT((adpcm.started == TRUE) || (adpcm.started == FALSE));
	ASSERT((adpcm.clock == 4) || (adpcm.clock == 8));
	ASSERT((adpcm.ratio == 0) || (adpcm.ratio == 1) || (adpcm.ratio == 2));
	ASSERT((adpcm.speed & 0x007f) == 0);
	ASSERT((adpcm.speed >= 0x0100) && (adpcm.speed <= 0x400));
	ASSERT(adpcm.data < 0x100);
	ASSERT((adpcm.offset >= 0) && (adpcm.offset <= 48));
	ASSERT((adpcm.enable == TRUE) || (adpcm.enable == FALSE));
	ASSERT((adpcm.sound == TRUE) || (adpcm.sound == FALSE));
	ASSERT(adpcm.readpoint < BufMax);
	ASSERT(adpcm.writepoint < BufMax);
	ASSERT(adpcm.number <= BufMax);
	ASSERT(adpcm.sync_cnt < 0x4000);
	ASSERT((adpcm.interp == TRUE)  || (adpcm.interp == FALSE));
	ASSERT(adpcmbuf);
}
#endif	// NDEBUG

//---------------------------------------------------------------------------
//
//	バイト読み込み
//
//---------------------------------------------------------------------------
DWORD FASTCALL ADPCM::ReadByte(DWORD addr)
{
	ASSERT(this);
	ASSERT((addr >= memdev.first) && (addr <= memdev.last));
	ASSERT_DIAG();

	// 奇数アドレスのみデコードされている
	if ((addr & 1) != 0) {
		// 4バイト単位でループ
		addr &= 3;

		// ウェイト
		scheduler->Wait(1);

		// アドレス振り分け
		if (addr == 3) {
			// データレジスタ
			if (adpcm.rec && adpcm.active) {
				// 録音データとして0x80を返す
				return 0x80;
			}
			return adpcm.data;
		}

		// ステータスレジスタ
		if (adpcm.play) {
			// 再生中、または再生完了後
			return 0x7f;
		}
		else {
			// 録音モード、または再生指示なし
			return 0xff;
		}
	}

	// 偶数アドレスはデコードされていない
	return 0xff;
}

//---------------------------------------------------------------------------
//
//	ワード読み込み
//
//---------------------------------------------------------------------------
DWORD FASTCALL ADPCM::ReadWord(DWORD addr)
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
void FASTCALL ADPCM::WriteByte(DWORD addr, DWORD data)
{
	ASSERT(this);
	ASSERT((addr >= memdev.first) && (addr <= memdev.last));
	ASSERT(data < 0x100);
	ASSERT_DIAG();

	// 奇数アドレスのみデコードされている
	if ((addr & 1) != 0) {
		// 4バイト単位でループ
		addr &= 3;

		// ウェイト
		scheduler->Wait(1);

		// アドレス振り分け
		if (addr == 3) {
			// データレジスタ
			adpcm.data = data;
			return;
		}

#if defined(ADPCM_LOG)
		LOG1(Log::Normal, "ADPCMコマンド $%02X", data);
#endif	// ADPCM_LOG

		// コマンドレジスタ
		if (data & 1) {
			// 動作停止
			Stop();
		}
		if (data & 2) {
			// 再生スタート
			adpcm.play = TRUE;
			Start(0);
			return;
		}
		if (data & 4) {
			// 録音スタート
			adpcm.rec = TRUE;
			Start(1);
			return;
		}
		return;
	}

	// 偶数アドレスはデコードされていない
}

//---------------------------------------------------------------------------
//
//	ワード書き込み
//
//---------------------------------------------------------------------------
void FASTCALL ADPCM::WriteWord(DWORD addr, DWORD data)
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
DWORD FASTCALL ADPCM::ReadOnly(DWORD addr) const
{
	ASSERT(this);
	ASSERT((addr >= memdev.first) && (addr <= memdev.last));
	ASSERT_DIAG();

	// 奇数アドレスのみデコードされている
	if (addr & 1) {
		// 4バイト単位でループ
		addr &= 3;

		// アドレス振り分け
		if (addr == 3) {
			// データレジスタ
			if (adpcm.rec && adpcm.active) {
				return 0x80;
			}
			return adpcm.data;
		}

		// ステータスレジスタ
		if (adpcm.play) {
			// 再生中、または再生完了後
			return 0x7f;
		}
		else {
			// 録音モード、または再生指示なし
			return 0xff;
		}
	}

	return 0xff;
}

//---------------------------------------------------------------------------
//
//	内部データ取得
//
//---------------------------------------------------------------------------
void FASTCALL ADPCM::GetADPCM(adpcm_t *buffer)
{
	ASSERT(this);
	ASSERT(buffer);
	ASSERT_DIAG();

	// 内部データをコピー
	*buffer = adpcm;
}

//---------------------------------------------------------------------------
//
//	イベントコールバック
//
//---------------------------------------------------------------------------
BOOL FASTCALL ADPCM::Callback(Event *ev)
{
	BOOL valid;
	DWORD num;
	char string[64];

	ASSERT(this);
	ASSERT(ev);
	ASSERT_DIAG();

	// ウェイトがあれば合成せず、次の時間まで引き延ばす
	if (adpcm.wait <= 0) {
		while (adpcm.wait <= 0) {
			// インアクティブまたはReqDMA失敗の場合、80とする
			adpcm.data = 0x80;
			valid = FALSE;

			// 1回のイベントで1バイト(2サンプル)の処理を行う
			if (adpcm.active) {
				if (dmac->ReqDMA(3)) {
					// DMA転送成功
					valid = TRUE;
				}
			}

			// 再生か
			if (ev->GetUser() == 0) {
				// 0x88,0x80,0x00以外はスタートフラグON
				if ((adpcm.data != 0x88) && (adpcm.data != 0x80) && (adpcm.data != 0x00)) {
#if defined(ADPCM_LOG)
					if (!adpcm.started) {
						LOG0(Log::Normal, "初回有効データ検出");
					}
#endif	// ADPCM_LOG
					adpcm.started = TRUE;
				}

				// ADPCM→PCM変換、バッファへストア
				num = adpcm.speed;
				num >>= 7;
				ASSERT((num >= 2) && (num <= 16));

				Decode((int)adpcm.data, num, valid);
				Decode((int)(adpcm.data >> 4), num, valid);
			}
			adpcm.wait++;
		}

		// ウェイトをリセット
		adpcm.wait = 0;

		// 速度変更に対応
		if (ev->GetTime() == adpcm.speed) {
			return TRUE;
		}
		sprintf(string, "Sampling %dHz", (2 * 1000 * 1000) / adpcm.speed);
		ev->SetDesc(string);
		ev->SetTime(adpcm.speed);
		return TRUE;
	}

	// ウェイトを減らす
	adpcm.wait--;

	// 速度変更に対応
	if (ev->GetTime() == adpcm.speed) {
		return TRUE;
	}
	sprintf(string, "Sampling %dHz", (2 * 1000 * 1000) / adpcm.speed);
	ev->SetDesc(string);
	ev->SetTime(adpcm.speed);
	return TRUE;
}

//---------------------------------------------------------------------------
//
//	基準クロック指定
//
//---------------------------------------------------------------------------
void FASTCALL ADPCM::SetClock(DWORD clk)
{
	ASSERT(this);
	ASSERT((clk == 4) || (clk == 8));
	ASSERT_DIAG();

#if defined(ADPCM_LOG)
	LOG1(Log::Normal, "クロック %dMHz", clk);
#endif	// ADPCM_LOG

	// 速度を再計算
	adpcm.clock = clk;
	CalcSpeed();
}

//---------------------------------------------------------------------------
//
//	クロック比率指定
//
//---------------------------------------------------------------------------
void FASTCALL ADPCM::SetRatio(DWORD ratio)
{
	ASSERT(this);
	ASSERT((ratio >= 0) || (ratio <= 3));
	ASSERT_DIAG();

#if defined(ADPCM_LOG)
	LOG1(Log::Normal, "速度比率 %d", ratio);
#endif	// ADPCM_LOG

	// ratio=3は2とみなす
	if (ratio == 3) {
		LOG0(Log::Warning, "未定義レート指定 $03");
		ratio = 2;
	}

	// 速度を再計算
	if (adpcm.ratio != ratio) {
		adpcm.ratio = ratio;
		CalcSpeed();
	}
}

//---------------------------------------------------------------------------
//
//	パンポット指定
//
//---------------------------------------------------------------------------
void FASTCALL ADPCM::SetPanpot(DWORD panpot)
{
	ASSERT(this);
	ASSERT((panpot >= 0) || (panpot <= 3));
	ASSERT_DIAG();

#if defined(ADPCM_LOG)
	LOG1(Log::Normal, "パンポット指定 %d", panpot);
#endif	// ADPCM_LOG

	adpcm.panpot = panpot;
}

//---------------------------------------------------------------------------
//
//	速度再計算
//
//---------------------------------------------------------------------------
void FASTCALL ADPCM::CalcSpeed()
{
	ASSERT(this);

	// 速い方から順に、2, 3, 4をつくる
	adpcm.speed = 2 - adpcm.ratio;
	ASSERT(adpcm.speed <= 2);
	adpcm.speed += 2;

	// クロック4MHzなら256、8MHzなら128を乗ずる
	adpcm.speed <<= 7;
	if (adpcm.clock == 4) {
		adpcm.speed <<= 1;
	}
}

//---------------------------------------------------------------------------
//
//	録音・再生開始
//
//---------------------------------------------------------------------------
void FASTCALL ADPCM::Start(int type)
{
	char string[64];

	ASSERT(this);
	ASSERT((type == 0) || (type == 1));
	ASSERT_DIAG();

#if defined(ADPCM_LOG)
	LOG1(Log::Normal, "再生開始 %d", type);
#endif	// ADPCM_LOG

	// アクティブフラグを上げる
	adpcm.active = TRUE;

	// データを初期化
	adpcm.offset = 0;

	// イベントを設定
	event.SetUser(type);

	// ここで必ず時間設定(実機とは異なる可能性があるが、FM音源との同期を優先)
	sprintf(string, "Sampling %dHz", (2 * 1000 * 1000) / adpcm.speed);
	event.SetDesc(string);
	event.SetTime(adpcm.speed);

	// 初回のイベントをここで実行(FM音源との同期をとる特別措置)
	Callback(&event);
}

//---------------------------------------------------------------------------
//
//	録音・再生停止
//
//---------------------------------------------------------------------------
void FASTCALL ADPCM::Stop()
{
	ASSERT(this);
	ASSERT_DIAG();

#if defined(ADPCM_LOG)
	LOG0(Log::Normal, "停止");
#endif	// ADPCM_LOG

	// フラグを降ろす
	adpcm.active = FALSE;
	adpcm.play = FALSE;
	adpcm.rec = FALSE;
}

//---------------------------------------------------------------------------
//
//	変位テーブル
//
//---------------------------------------------------------------------------
const int ADPCM::NextTable[] = {
	-1, -1, -1, -1, 2, 4, 6, 8,
	-1, -1, -1, -1, 2, 4, 6, 8
};

//---------------------------------------------------------------------------
//
//	オフセットテーブル
//
//---------------------------------------------------------------------------
const int ADPCM::OffsetTable[] = {
	 0,
	 0,  1,  2,  3,  4,  5,  6,  7,
	 8,  9, 10, 11, 12, 13, 14, 15,
	16, 17, 18, 19, 20, 21, 22, 23,
	24, 25, 26, 27, 28, 29, 30, 31,
	32, 33, 34, 35, 36, 37, 38, 39,
	40, 41, 42, 43, 44, 45, 46, 47,
	48, 48, 48, 48, 48, 48, 48, 48,
	48
};

//---------------------------------------------------------------------------
//
//	デコード
//
//---------------------------------------------------------------------------
void FASTCALL ADPCM::Decode(int data, int num, BOOL valid)
{
	int i;
	int store;
	int sample;
	int diff;
	int base;

	ASSERT(this);
	ASSERT((data >= 0) && (data < 0x100));
	ASSERT(num >= 2);
	ASSERT_DIAG();

	// ディセーブルなら何もしない
	if (!adpcm.enable) {
		return;
	}

	// データをマスク
	data &= 0x0f;

	// 差分テーブルから得る
	i = adpcm.offset << 4;
	i |= data;
	sample = DiffTable[i];

	// 次のオフセットを求めておく
	adpcm.offset += NextTable[data];
	adpcm.offset = OffsetTable[adpcm.offset + 1];

	// ストアデータを演算
	store = (sample << 8) + (adpcm.sample * 245);
	store >>= 8;

	// 音量処理＋値記憶
	base = adpcm.sample;
	base *= adpcm.vol;
	base >>= 9;
	adpcm.sample = store;
	store *= adpcm.vol;
	store >>= 9;
	adpcm.out = store;

	// 有効なデータが来ていないなら、サンプルを上げる
	if (!valid) {
		if (adpcm.sample < 0) {
			adpcm.sample++;
		}
		if (adpcm.sample > 0) {
			adpcm.sample--;
		}
	}

	// 「ピー」音の消去
	if ((adpcm.sample == 0) || (adpcm.sample == -1) || (adpcm.sample == 1)) {
		store = 0;
	}

	// 差分を得る
	diff = store - base;

	// 音量を考慮して、numだけサンプルをストア
	switch (adpcm.panpot) {
		// 左右とも出力
		case 0:
			for (i=0; i<num; i++) {
				store = base + (diff * (i + 1)) / num;
				adpcmbuf[adpcm.writepoint++] = store;
				adpcmbuf[adpcm.writepoint++] = store;
				adpcm.writepoint &= (BufMax - 1);
			}
			break;

		// 左のみ出力
		case 1:
			for (i=0; i<num; i++) {
				store = base + (diff * (i + 1)) / num;
				adpcmbuf[adpcm.writepoint++] = store;
				adpcmbuf[adpcm.writepoint++] = 0;
				adpcm.writepoint &= (BufMax - 1);
			}
			break;

		// 右のみ出力
		case 2:
			for (i=0; i<num; i++) {
				store = base + (diff * (i + 1)) / num;
				adpcmbuf[adpcm.writepoint++] = 0;
				adpcmbuf[adpcm.writepoint++] = store;
				adpcm.writepoint &= (BufMax - 1);
			}
			break;

		// 両方オフ
		case 3:
			for (i=0; i<num; i++) {
				adpcmbuf[adpcm.writepoint++] = 0;
				adpcmbuf[adpcm.writepoint++] = 0;
				adpcm.writepoint &= (BufMax - 1);
			}
			break;

		// 通常、あり得ない
		default:
			ASSERT(FALSE);
	}

	// 個数を更新
	adpcm.number += (num << 1);
	if (adpcm.number >= BufMax) {
#if defined(ADPCM_LOG)
		LOG0(Log::Warning, "ADPCMバッファ オーバーラン");
#endif	// ADPCM_LOG
		adpcm.number = BufMax;
		adpcm.readpoint = adpcm.writepoint;
	}
}

//---------------------------------------------------------------------------
//
//	合成イネーブル
//
//---------------------------------------------------------------------------
void FASTCALL ADPCM::Enable(BOOL enable)
{
	ASSERT(this);
	ASSERT_DIAG();

	adpcm.enable = enable;
}

//---------------------------------------------------------------------------
//
//	バッファ初期化
//
//---------------------------------------------------------------------------
void FASTCALL ADPCM::InitBuf(DWORD rate)
{
	ASSERT(this);
	ASSERT(rate > 0);
	ASSERT((rate % 100) == 0);

#if defined(ADPCM_LOG)
	LOG0(Log::Normal, "バッファ初期化");
#endif	// ADPCM_LOG

	// カウンタ、ポインタ
	adpcm.number = 0;
	adpcm.readpoint = 0;
	adpcm.writepoint = 0;
	adpcm.wait = 0;
	adpcm.sync_cnt = 0;
	adpcm.sync_rate = rate / 50;
	adpcm.sync_step = 0x9c4000 / adpcm.sync_rate;

	// 最初のデータに0をセット
	if (adpcmbuf) {
		adpcmbuf[0] = 0;
		adpcmbuf[1] = 0;
	}
}

//---------------------------------------------------------------------------
//
//	バッファからデータ取得
//
//---------------------------------------------------------------------------
void FASTCALL ADPCM::GetBuf(DWORD *buffer, int samples)
{
	int i;
	int j;
	int l;
	int r;
	int *ptr;
	DWORD point;

	ASSERT(this);
	ASSERT(buffer);
	ASSERT(samples >= 0);
	ASSERT_DIAG();

	// 無効、またはチャネルカットの場合は、バッファクリア
	if (!adpcm.enable || !adpcm.sound) {
		ASSERT(adpcm.sync_rate != 0);
		InitBuf(adpcm.sync_rate * 50);
		return;
	}

	// 初期化
	ptr = (int*)buffer;

	// バッファに何もないときは、最後のデータを続けて入れる
	if (adpcm.number <= 2) {
		l = adpcmbuf[adpcm.readpoint];
		r = adpcmbuf[adpcm.readpoint + 1];
		for (i=samples; i>0; i--) {
			*ptr += l;
			ptr++;
			*ptr += r;
			ptr++;
		}
		return;
	}

	// 線形補間の有無で分ける
	if (adpcm.interp) {

		// 補間あり
		for (i=samples; i>0; i--) {
			// ステップUp
			adpcm.sync_cnt += adpcm.sync_step;
			if (adpcm.sync_cnt >= 0x4000) {
				// 同期処理
				adpcm.sync_cnt &= 0x3fff;

				// 次へ
				if (adpcm.number >= 4) {
					adpcm.readpoint += 2;
					adpcm.readpoint &= (BufMax - 1);
					adpcm.number -= 2;
				}
			}

			// 次のデータがあるか
			if (adpcm.number < 4 ) {
				// 次のデータがないので、今のデータをそのまま入れる
				*ptr += adpcmbuf[adpcm.readpoint];
				ptr++;
				*ptr += adpcmbuf[adpcm.readpoint + 1];
				ptr++;
			}
			else {
				// 次のポイントをもらう
				point = adpcm.readpoint + 2;
				point &= (BufMax - 1);

				// 現データと次のデータで補間[L]
				l = adpcmbuf[point] * (int)adpcm.sync_cnt;
				r = adpcmbuf[adpcm.readpoint + 0] * (int)(0x4000 - adpcm.sync_cnt);
				*ptr += ((l + r) >> 14);
				ptr++;

				// 現データと次のデータで補間[R]
				l = adpcmbuf[point + 1] * (int)adpcm.sync_cnt;
				r = adpcmbuf[adpcm.readpoint + 1] * (int)(0x4000 - adpcm.sync_cnt);
				*ptr += ((l + r) >> 14);
				ptr++;
			}
		}
	}
	else {
		// 補間なし
		for (i=samples; i>0; i--) {
			// 現在の位置からデータを格納
			*buffer += adpcmbuf[adpcm.readpoint];
			buffer++;
			*buffer += adpcmbuf[adpcm.readpoint + 1];
			buffer++;

			// sync_stepを加算
			adpcm.sync_cnt += adpcm.sync_step;

			// 0x4000と同期処理
			if (adpcm.sync_cnt < 0x4000) {
				continue;
			}
			adpcm.sync_cnt &= 0x3fff;

			// 次のADPCMサンプルへ移動
			if (adpcm.number <= 2) {
				// 最後のデータを続けて入れる
				l = adpcmbuf[adpcm.readpoint];
				r = adpcmbuf[adpcm.readpoint + 1];
				for (j=i-1; j>0; j--) {
					*buffer += l;
					buffer++;
					*buffer += r;
					buffer++;
					adpcm.sync_cnt += adpcm.sync_step;
				}
				adpcm.sync_cnt &= 0x3fff;
				return;
			}
			adpcm.readpoint += 2;
			adpcm.readpoint &= (BufMax - 1);
			adpcm.number -= 2;
		}
	}
}

//---------------------------------------------------------------------------
//
//	ウェイトをかける
//
//---------------------------------------------------------------------------
void FASTCALL ADPCM::Wait(int num)
{
	int i;

	ASSERT(this);
	ASSERT_DIAG();

	// イベント開始してなければ0
	if (event.GetTime() == 0) {
		adpcm.wait = 0;
		return;
	}

	// 少なければOPMの方が上回っている
	if ((int)adpcm.number <= num) {
		// 差分をつくる。差分の1/4をウェイトとする
		num -= (adpcm.number);
		num >>= 2;

		// 計算は↓と同様。符号反転
		i = adpcm.speed >> 6;
		i *= adpcm.sync_rate;
		adpcm.wait = -((625 * num) / i);

#if defined(ADPCM_LOG)
		if (adpcm.wait != 0) {
			LOG1(Log::Normal, "ウェイト設定 %d", adpcm.wait);
		}
#endif	// ADPCM_LOG
		return;
	}

	// 差分をつくる。差分の1/4をウェイトとする
	num = adpcm.number - num;
	num >>= 2;

	// 44.1k,48k etc.での余剰サンプル数をxとすると
	// ウェイト回数は (625 * x) / (adpcm.sync_rate * (adpcm.speed >> 6))
	i = adpcm.speed >> 6;
	i *= adpcm.sync_rate;
	adpcm.wait = (625 * num) / i;

#if defined(ADPCM_LOG)
	if (adpcm.wait != 0) {
		LOG1(Log::Normal, "ウェイト設定 %d", adpcm.wait);
	}
#endif	// ADPCM_LOG
}

//---------------------------------------------------------------------------
//
//	テーブル作成
//
//---------------------------------------------------------------------------
void FASTCALL ADPCM::MakeTable()
{
	int i;
	int j;
	int base;
	int diff;
	int *p;

	ASSERT(this);

	// テーブル作成(floorで丸めた方がpanic等で良い結果が得られる)
	p = DiffTable;
	for (i=0; i<49; i++) {
		base = (int)floor(16.0 * pow(1.1, i));

		// 演算もすべてintで行う
		for (j=0; j<16; j++) {
			diff = 0;
			if (j & 4) {
				diff += base;
			}
			if (j & 2) {
				diff += (base >> 1);
			}
			if (j & 1) {
				diff += (base >> 2);
			}
			diff += (base >> 3);
			if (j & 8) {
				diff = -diff;
			}

			*p++ = diff;
		}
	}
}

//---------------------------------------------------------------------------
//
//	音量設定
//
//---------------------------------------------------------------------------
void FASTCALL ADPCM::SetVolume(int volume)
{
	double offset;
	double vol;

	ASSERT(this);
	ASSERT((volume >= 0) && (volume < 100));

	// 16384 * 10^((volume-140) / 200)を算出、セット
	offset = (double)(volume - 140);
	offset /= (double)200.0;
	vol = pow((double)10.0, offset);
	vol *= (double)16384.0;
	adpcm.vol = int(vol);
}
