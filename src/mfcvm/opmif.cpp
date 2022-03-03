//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 ＰＩ．(ytanaka@ipc-tokai.or.jp)
//	[ OPM(YM2151) ]
//
//---------------------------------------------------------------------------

#include "os.h"
#include "xm6.h"
#include "vm.h"
#include "log.h"
#include "mfp.h"
#include "schedule.h"
#include "adpcm.h"
#include "fdd.h"
#include "fileio.h"
#include "config.h"
#include "opm.h"
#include "opmif.h"

//===========================================================================
//
//	OPM
//
//===========================================================================
//#define OPM_LOG

//---------------------------------------------------------------------------
//
//	コンストラクタ
//
//---------------------------------------------------------------------------
OPMIF::OPMIF(VM *p) : MemDevice(p)
{
	// デバイスIDを初期化
	dev.id = MAKEID('O', 'P', 'M', ' ');
	dev.desc = "OPM (YM2151)";

	// 開始アドレス、終了アドレス
	memdev.first = 0xe90000;
	memdev.last = 0xe91fff;

	// ワーククリア
	mfp = NULL;
	fdd = NULL;
	adpcm = NULL;
	engine = NULL;
	opmbuf = NULL;
}

//---------------------------------------------------------------------------
//
//	初期化
//
//---------------------------------------------------------------------------
BOOL FASTCALL OPMIF::Init()
{
	int i;

	ASSERT(this);

	// 基本クラス
	if (!MemDevice::Init()) {
		return FALSE;
	}

	// MFP取得
	ASSERT(!mfp);
	mfp = (MFP*)vm->SearchDevice(MAKEID('M', 'F', 'P', ' '));
	ASSERT(mfp);

	// ADPCM取得
	ASSERT(!adpcm);
	adpcm = (ADPCM*)vm->SearchDevice(MAKEID('A', 'P', 'C', 'M'));
	ASSERT(adpcm);

	// FDD取得
	ASSERT(!fdd);
	fdd = (FDD*)vm->SearchDevice(MAKEID('F', 'D', 'D', ' '));
	ASSERT(fdd);

	// イベント作成
	event[0].SetDevice(this);
	event[0].SetDesc("Timer-A");
	event[1].SetDevice(this);
	event[1].SetDesc("Timer-B");
	for (i=0; i<2; i++) {
		event[i].SetUser(i);
		event[i].SetTime(0);
		scheduler->AddEvent(&event[i]);
	}

	// バッファ確保
	try {
		opmbuf = new DWORD [BufMax * 2];
	}
	catch (...) {
		return FALSE;
	}
	if (!opmbuf) {
		return FALSE;
	}

	// ワーククリア
	memset(&opm, 0, sizeof(opm));
	memset(&bufinfo, 0, sizeof(bufinfo));
	bufinfo.max = BufMax;
	bufinfo.sound = TRUE;

	// バッファ初期化
	InitBuf(44100);

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	クリーンアップ
//
//---------------------------------------------------------------------------
void FASTCALL OPMIF::Cleanup()
{
	ASSERT(this);

	if (opmbuf) {
		delete[] opmbuf;
		opmbuf = NULL;
	}

	// 基本クラスへ
	MemDevice::Cleanup();
}

//---------------------------------------------------------------------------
//
//	リセット
//
//---------------------------------------------------------------------------
void FASTCALL OPMIF::Reset()
{
	int i;

	ASSERT(this);
	ASSERT_DIAG();

	LOG0(Log::Normal, "リセット");

	// イベントを止める
	event[0].SetTime(0);
	event[1].SetTime(0);

	// バッファクリア
	memset(opmbuf, 0, sizeof(DWORD) * (BufMax * 2));

	// エンジンが指定されていればリセット
	if (engine) {
		engine->Reset();
	}

	// レジスタクリア
	for (i=0; i<0x100; i++) {
		if (i == 8) {
			continue;
		}
		if ((i >= 0x60) && (i <= 0x7f)) {
			Output(i, 0x7f);
			continue;
		}
		if ((i >= 0xe0) && (i <= 0xff)) {
			Output(i, 0xff);
			continue;
		}
		Output(i, 0);
	}
	Output(0x19, 0x80);
	for (i=0; i<8; i++) {
		Output(8, i);
		opm.key[i] = i;
	}

	// その他ワークエリアを初期化
	opm.addr = 0;
	opm.busy = FALSE;
	for (i=0; i<2; i++) {
		opm.enable[i] = FALSE;
		opm.action[i] = FALSE;
		opm.interrupt[i] = FALSE;
		opm.time[i] = 0;
	}
	opm.started = FALSE;

	// 割り込み要求なし
	mfp->SetGPIP(3, 1);
}

//---------------------------------------------------------------------------
//
//	セーブ
//
//---------------------------------------------------------------------------
BOOL FASTCALL OPMIF::Save(Fileio *fio, int ver)
{
	size_t sz;

	ASSERT(this);
	ASSERT(fio);
	ASSERT_DIAG();

	LOG0(Log::Normal, "セーブ");

	// サイズをセーブ
	sz = sizeof(opm_t);
	if (!fio->Write(&sz, sizeof(sz))) {
		return FALSE;
	}

	// 本体をセーブ
	if (!fio->Write(&opm, (int)sz)) {
		return FALSE;
	}

	// バッファ情報のサイズをセーブ
	sz = sizeof(opmbuf_t);
	if (!fio->Write(&sz, sizeof(sz))) {
		return FALSE;
	}

	// バッファ情報をセーブ
	if (!fio->Write(&bufinfo, (int)sz)) {
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
BOOL FASTCALL OPMIF::Load(Fileio *fio, int ver)
{
	size_t sz;
	int i;

	ASSERT(this);
	ASSERT(fio);
	ASSERT_DIAG();

	LOG0(Log::Normal, "ロード");

	// サイズをロード、照合
	if (!fio->Read(&sz, sizeof(sz))) {
		return FALSE;
	}
	if (sz != sizeof(opm_t)) {
		return FALSE;
	}

	// 本体をロード
	if (!fio->Read(&opm, (int)sz)) {
		return FALSE;
	}

	// バッファ情報のサイズをロード、照合
	if (!fio->Read(&sz, sizeof(sz))) {
		return FALSE;
	}
	if (sz != sizeof(opmbuf_t)) {
		return FALSE;
	}

	// バッファ情報をロード
	if (!fio->Read(&bufinfo, (int)sz)) {
		return FALSE;
	}

	// イベントをロード
	if (!event[0].Load(fio, ver)) {
		return FALSE;
	}
	if (!event[1].Load(fio, ver)) {
		return FALSE;
	}

	// バッファをクリア
	InitBuf(bufinfo.rate * 100);

	// エンジンへのレジスタ再設定
	if (engine) {
		// レジスタ復旧:ノイズ、LFO、PMD、AMD、CSM
		engine->SetReg(0x0f, opm.reg[0x0f]);
		engine->SetReg(0x14, opm.reg[0x14] & 0x80);
		engine->SetReg(0x18, opm.reg[0x18]);
		engine->SetReg(0x19, opm.reg[0x19]);

		// レジスタ復旧:レジスタ
		for (i=0x20; i<0x100; i++) {
			engine->SetReg(i, opm.reg[i]);
		}

		// レジスタ復旧:キー
		for (i=0; i<8; i++) {
			engine->SetReg(8, opm.key[i]);
		}

		// レジスタ復旧:LFO
		engine->SetReg(1, 2);
		engine->SetReg(1, 0);
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	設定適用
//
//---------------------------------------------------------------------------
void FASTCALL OPMIF::ApplyCfg(const Config* /*config*/)
{
	ASSERT(this);
	ASSERT_DIAG();

	LOG0(Log::Normal, "設定適用");
}

#if !defined(NDEBUG)
//---------------------------------------------------------------------------
//
//	診断
//
//---------------------------------------------------------------------------
void FASTCALL OPMIF::AssertDiag() const
{
	// 基本クラス
	MemDevice::AssertDiag();

	ASSERT(this);
	ASSERT(GetID() == MAKEID('O', 'P', 'M', ' '));
	ASSERT(memdev.first == 0xe90000);
	ASSERT(memdev.last == 0xe91fff);
	ASSERT(mfp);
	ASSERT(mfp->GetID() == MAKEID('M', 'F', 'P', ' '));
	ASSERT(adpcm);
	ASSERT(adpcm->GetID() == MAKEID('A', 'P', 'C', 'M'));
	ASSERT(fdd);
	ASSERT(fdd->GetID() == MAKEID('F', 'D', 'D', ' '));
	ASSERT(opm.addr < 0x100);
	ASSERT((opm.busy == TRUE) || (opm.busy == FALSE));
	ASSERT((opm.enable[0] == TRUE) || (opm.enable[0] == FALSE));
	ASSERT((opm.enable[1] == TRUE) || (opm.enable[1] == FALSE));
	ASSERT((opm.action[0] == TRUE) || (opm.action[0] == FALSE));
	ASSERT((opm.action[1] == TRUE) || (opm.action[1] == FALSE));
	ASSERT((opm.interrupt[0] == TRUE) || (opm.interrupt[0] == FALSE));
	ASSERT((opm.interrupt[1] == TRUE) || (opm.interrupt[1] == FALSE));
	ASSERT((opm.started == TRUE) || (opm.started == FALSE));
	ASSERT(bufinfo.max == BufMax);
	ASSERT(bufinfo.num <= bufinfo.max);
	ASSERT(bufinfo.read < bufinfo.max);
	ASSERT(bufinfo.write < bufinfo.max);
	ASSERT((bufinfo.sound == TRUE) || (bufinfo.sound == FALSE));
}
#endif	// NDEBUG

//---------------------------------------------------------------------------
//
//	バイト読み込み
//
//---------------------------------------------------------------------------
DWORD FASTCALL OPMIF::ReadByte(DWORD addr)
{
	DWORD data;

	ASSERT(this);
	ASSERT((addr >= memdev.first) && (addr <= memdev.last));
	ASSERT_DIAG();

	// 奇数アドレスのみデコードされている
	if ((addr & 1) != 0) {
		// 4バイト単位でループ
		addr &= 3;

		// ウェイト
		scheduler->Wait(1);

		if (addr != 0x01) {
			// データポートはBUSYとタイマの状態
			data = 0;

			// BUSY(1回だけ)
			if (opm.busy) {
				data |= 0x80;
				opm.busy = FALSE;
			}

			// タイマ
			if (opm.interrupt[0]) {
				data |= 0x01;
			}
			if (opm.interrupt[1]) {
				data |= 0x02;
			}

			return data;
		}

		// アドレスポートはFF
		return 0xff;
	}

	return 0xff;
}

//---------------------------------------------------------------------------
//
//	ワード書き込み
//
//---------------------------------------------------------------------------
DWORD FASTCALL OPMIF::ReadWord(DWORD addr)
{
	ASSERT(this);
	ASSERT((addr >= memdev.first) && (addr <= memdev.last));
	ASSERT((addr & 1) == 0);
	ASSERT_DIAG();

	return (WORD)(0xff00 | ReadByte(addr + 1));
}

//---------------------------------------------------------------------------
//
//	バイト書き込み
//
//---------------------------------------------------------------------------
void FASTCALL OPMIF::WriteByte(DWORD addr, DWORD data)
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

		if (addr == 0x01) {
			// アドレス指定(BUSYに関わらず指定できることにする)
			opm.addr = data;

			// BUSYを降ろす。アドレス指定後の待ちは発生しないことにする
			opm.busy = FALSE;

			return;
		}
		else {
			// データ書き込み(BUSYに関わらず書き込めることにする)
			Output(opm.addr, data);

			// BUSYを上げる
			opm.busy = TRUE;
			return;
		}
	}
}

//---------------------------------------------------------------------------
//
//	ワード書き込み
//
//---------------------------------------------------------------------------
void FASTCALL OPMIF::WriteWord(DWORD addr, DWORD data)
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
DWORD FASTCALL OPMIF::ReadOnly(DWORD addr) const
{
	DWORD data;

	ASSERT(this);
	ASSERT((addr >= memdev.first) && (addr <= memdev.last));
	ASSERT_DIAG();

	// 奇数アドレスのみデコードされている
	if ((addr & 1) != 0) {
		// 4バイト単位でループ
		addr &= 3;

		if (addr != 0x01) {
			// データポートはBUSYとタイマの状態
			data = 0;

			// BUSY
			if (opm.busy) {
				data |= 0x80;
			}

			// タイマ
			if (opm.interrupt[0]) {
				data |= 0x01;
			}
			if (opm.interrupt[1]) {
				data |= 0x02;
			}

			return data;
		}

		// アドレスポートはFF
		return 0xff;
	}

	return 0xff;
}

//---------------------------------------------------------------------------
//
//	内部データ取得
//
//---------------------------------------------------------------------------
void FASTCALL OPMIF::GetOPM(opm_t *buffer)
{
	ASSERT(this);
	ASSERT(buffer);
	ASSERT_DIAG();

	// 内部データをコピー
	*buffer = opm;
}

//---------------------------------------------------------------------------
//
//	イベントコールバック
//
//---------------------------------------------------------------------------
BOOL FASTCALL OPMIF::Callback(Event *ev)
{
	int index;

	ASSERT(this);
	ASSERT(ev);
	ASSERT_DIAG();

	// ユーザデータ受信
	index = ev->GetUser();
	ASSERT((index >= 0) && (index <= 1));

#if defined(OPM_LOG)
	LOG2(Log::Normal, "タイマ%c コールバック Time %d", index + 'A', ev->GetTime());
#endif	// OPM_LOG

	// イネーブルかつ動作中なら、割り込みを起こす
	if (opm.enable[index] && opm.action[index]) {
		opm.action[index] = FALSE;
		opm.interrupt[index] = TRUE;
#if defined(OPM_LOG)
		LOG2(Log::Normal, "タイマ%c オーバーフロー割り込み Time %d", index + 'A', ev->GetTime());
#endif	// OPM_LOG
		mfp->SetGPIP(3, 0);
	}

	// 時間が違っていれば、再設定
	if (ev->GetTime() != opm.time[index]) {
		ev->SetTime(opm.time[index]);
#if defined(OPM_LOG)
		LOG2(Log::Normal, "タイマ%c イベントリスタート Time %d", index + 'A', ev->GetTime());
#endif	// OPM_LOG
	}

	// タイマAはCSMの場合がある
	if ((index == 0) && engine) {
		if (opm.reg[0x14] & 0x80) {
			ProcessBuf();
			engine->TimerA();
		}
	}

	// タイマは回し続ける
	return TRUE;
}

//---------------------------------------------------------------------------
//
//	エンジン指定
//
//---------------------------------------------------------------------------
void FASTCALL OPMIF::SetEngine(FM::OPM *p)
{
	int i;

	ASSERT(this);
	ASSERT_DIAG();

	// NULLを指定される場合もある
	engine = p;

	// ADPCMへ通知
	if (engine) {
		adpcm->Enable(TRUE);
	}
	else {
		adpcm->Enable(FALSE);
	}

	// engine指定ありなら、OPMレジスタを出力
	if (!engine) {
		return;
	}
	ProcessBuf();

	// レジスタ復旧:ノイズ、LFO、PMD、AMD、CSM
	engine->SetReg(0x0f, opm.reg[0x0f]);
	engine->SetReg(0x14, opm.reg[0x14] & 0x80);
	engine->SetReg(0x18, opm.reg[0x18]);
	engine->SetReg(0x19, opm.reg[0x19]);

	// レジスタ復旧:レジスタ
	for (i=0x20; i<0x100; i++) {
		engine->SetReg(i, opm.reg[i]);
	}

	// レジスタ復旧:キー
	for (i=0; i<8; i++) {
		engine->SetReg(8, opm.key[i]);
	}

	// レジスタ復旧:LFO
	engine->SetReg(1, 2);
	engine->SetReg(1, 0);
}

//---------------------------------------------------------------------------
//
//	レジスタ出力
//
//---------------------------------------------------------------------------
void FASTCALL OPMIF::Output(DWORD addr, DWORD data)
{
	ASSERT(this);
	ASSERT(addr < 0x100);
	ASSERT(data < 0x100);
	ASSERT_DIAG();

	// 特殊レジスタの処理
	switch (addr) {
		// タイマA
		case 0x10:
		case 0x11:
			opm.reg[addr] = data;
			CalcTimerA();
			return;

		// タイマB
		case 0x12:
			opm.reg[addr] = data;
			CalcTimerB();
			return;

		// タイマ制御
		case 0x14:
			CtrlTimer(data);
			data &= 0x80;
			break;

		// 汎用ポート付き
		case 0x1b:
			CtrlCT(data);
			opm.reg[addr] = data;
			data &= 0x3f;
			break;

		// キー
		case 0x08:
			opm.key[data & 0x07] = data;
			opm.reg[addr] = data;
			break;

		// その他
		default:
			opm.reg[addr] = data;
			break;
	}

	// エンジンが指定されていれば、バッファリングして出力
	if (engine) {
		if ((addr < 0x10) || (addr > 0x14)) {
			// タイマ関連ではバッファリングしない
			ProcessBuf();

			// キーオフ以外はスタートフラグUp
			if ((addr != 0x08) || (data >= 0x08)) {
				opm.started = TRUE;
			}
		}
		engine->SetReg(addr, data);
	}
}

//---------------------------------------------------------------------------
//
//	タイマA算出
//
//---------------------------------------------------------------------------
void FASTCALL OPMIF::CalcTimerA()
{
	DWORD hus;
	DWORD low;

	ASSERT(this);
	ASSERT_DIAG();

	// hus単位で計算
	hus = opm.reg[0x10];
	hus <<= 2;
	low = opm.reg[0x11] & 3;
	hus |= low;
	hus = (0x400 - hus);
	hus <<= 5;
	opm.time[0] = hus;
#if defined(OPM_LOG)
	LOG1(Log::Normal, "タイマA = %d", hus);
#endif	// OPM_LOG

	// 止まっていればスタート(YST)
	if (event[0].GetTime() == 0) {
		event[0].SetTime(hus);
#if defined(OPM_LOG)
		LOG1(Log::Normal, "タイマA スタート Time %d", event[0].GetTime());
#endif	// OPM_LOG
	}
}

//---------------------------------------------------------------------------
//
//	タイマB算出
//
//---------------------------------------------------------------------------
void FASTCALL OPMIF::CalcTimerB()
{
	DWORD hus;

	ASSERT(this);
	ASSERT_DIAG();

	// hus単位で計算
	hus = opm.reg[0x12];
	hus = (0x100 - hus);
	hus <<= 9;
	opm.time[1] = hus;
#if defined(OPM_LOG)
	LOG1(Log::Normal, "タイマB = %d", hus);
#endif	// OPM_LOG

	// 止まっていればスタート(YST)
	if (event[1].GetTime() == 0) {
		event[1].SetTime(hus);
#if defined(OPM_LOG)
		LOG1(Log::Normal, "タイマB スタート Time %d", event[1].GetTime());
#endif	// OPM_LOG
	}
}

//---------------------------------------------------------------------------
//
//	タイマ制御
//
//---------------------------------------------------------------------------
void FASTCALL OPMIF::CtrlTimer(DWORD data)
{
	ASSERT(this);
	ASSERT(data < 0x100);
	ASSERT_DIAG();

	// オーバーフローフラグのクリア
	if (data & 0x10) {
		opm.interrupt[0] = FALSE;
	}
	if (data & 0x20) {
		opm.interrupt[1] = FALSE;
	}

	// 両方落ちたら、割り込みを落とす
	if (!opm.interrupt[0] && !opm.interrupt[1]) {
		mfp->SetGPIP(3, 1);
	}

	// タイマAアクション制御
	if (data & 0x04) {
		opm.action[0] = TRUE;
	}
	else {
		opm.action[0] = FALSE;
	}

	// タイマAイネーブル制御
	if (data & 0x01) {
		// 0→1でタイマ値をロード、それ以外でもタイマON
		if (!opm.enable[0]) {
			CalcTimerA();
		}
		opm.enable[0] = TRUE;
	}
	else {
		// (マンハッタン・レクイエム)
		event[0].SetTime(0);
		opm.enable[0] = FALSE;
	}

	// タイマBアクション制御
	if (data & 0x08) {
		opm.action[1] = TRUE;
	}
	else {
		opm.action[1] = FALSE;
	}

	// タイマBイネーブル制御
	if (data & 0x02) {
		// 0→1でタイマ値をロード、それ以外でもタイマON
		if (!opm.enable[1]) {
			CalcTimerB();
		}
		opm.enable[1] = TRUE;
	}
	else {
		// (マンハッタン・レクイエム)
		event[1].SetTime(0);
		opm.enable[1] = FALSE;
	}

	// データを記憶
	opm.reg[0x14] = data;

#if defined(OPM_LOG)
	LOG1(Log::Normal, "タイマ制御 $%02X", data);
#endif	// OPM_LOG
}

//---------------------------------------------------------------------------
//
//	CT制御
//
//---------------------------------------------------------------------------
void FASTCALL OPMIF::CtrlCT(DWORD data)
{
	DWORD ct;

	ASSERT(this);
	ASSERT_DIAG();

	// CTポートのみ取り出す
	data &= 0xc0;

	// 以前のデータを得る
	ct = opm.reg[0x1b];
	ct &= 0xc0;

	// 一致していれば何もしない
	if (data == ct) {
		return;
	}

	// チェック(ADPCM)
	if ((data & 0x80) != (ct & 0x80)) {
		if (data & 0x80) {
			adpcm->SetClock(4);
		}
		else {
			adpcm->SetClock(8);
		}
	}

	// チェック(FDD)
	if ((data & 0x40) != (ct & 0x40)) {
		if (data & 0x40) {
			fdd->ForceReady(TRUE);
		}
		else {
			fdd->ForceReady(FALSE);
		}
	}
}

//---------------------------------------------------------------------------
//
//	バッファ内容を得る
//
//---------------------------------------------------------------------------
void FASTCALL OPMIF::GetBufInfo(opmbuf_t *buf)
{
	ASSERT(this);
	ASSERT(buf);
	ASSERT_DIAG();

	*buf = bufinfo;
}

//---------------------------------------------------------------------------
//
//	バッファ初期化
//
//---------------------------------------------------------------------------
void FASTCALL OPMIF::InitBuf(DWORD rate)
{
	ASSERT(this);
	ASSERT(rate > 0);
	ASSERT((rate % 100) == 0);
	ASSERT_DIAG();

	// ADPCMに先に通知
	adpcm->InitBuf(rate);

	// カウンタ、ポインタ
	bufinfo.num = 0;
	bufinfo.read = 0;
	bufinfo.write = 0;
	bufinfo.under = 0;
	bufinfo.over = 0;

	// サウンド時間と、連携するサンプル数
	scheduler->SetSoundTime(0);
	bufinfo.samples = 0;

	// 合成レート
	bufinfo.rate = rate / 100;
}

//---------------------------------------------------------------------------
//
//	バッファリング
//
//---------------------------------------------------------------------------
DWORD FASTCALL OPMIF::ProcessBuf()
{
	DWORD stime;
	DWORD sample;
	DWORD first;
	DWORD second;

	ASSERT(this);
	ASSERT_DIAG();

	// エンジンがなければリターン
	if (!engine) {
		return bufinfo.num;
	}

	// サウンド時間から、適切なサンプル数を得る
	sample = scheduler->GetSoundTime();
	stime = sample;

	sample *= bufinfo.rate;
	sample /= 20000;

	// bufmaxに制限
	if (sample > BufMax) {
		// オーバーしすぎているので、リセット
		scheduler->SetSoundTime(0);
		bufinfo.samples = 0;
		return bufinfo.num;
	}

	// 現状と一致していればリターン
	if (sample <= bufinfo.samples) {
		// シンクロさせる
		while (stime >= 40000) {
			stime -= 20000;
			scheduler->SetSoundTime(stime);
			bufinfo.samples -= bufinfo.rate;
		}
		return bufinfo.num;
	}

	// 現状と違う部分だけ生成する
	sample -= bufinfo.samples;

	// 1回または2回のどちらかチェック
	first = sample;
	if ((first + bufinfo.write) > BufMax) {
		first = BufMax - bufinfo.write;
	}
	second = sample - first;

	// 1回目
	memset(&opmbuf[bufinfo.write * 2], 0, first * 8);
	if (bufinfo.sound) {
		engine->Mix((int32*)&opmbuf[bufinfo.write * 2], first);
	}
	bufinfo.write += first;
	bufinfo.write &= (BufMax - 1);
	bufinfo.num += first;
	if (bufinfo.num > BufMax) {
		bufinfo.num = BufMax;
		bufinfo.read = bufinfo.write;
	}

	// 2回目
	if (second > 0) {
		memset(opmbuf, 0, second * 8);
		if (bufinfo.sound) {
			engine->Mix((int32*)opmbuf, second);
		}
		bufinfo.write += second;
		bufinfo.write &= (BufMax - 1);
		bufinfo.num += second;
		if (bufinfo.num > BufMax) {
			bufinfo.num = BufMax;
			bufinfo.read = bufinfo.write;
		}
	}

	// 合成済みサンプル数へ加算し、20000husごとにシンクロさせる
	bufinfo.samples += sample;
	while (stime >= 40000) {
		stime -= 20000;
		scheduler->SetSoundTime(stime);
		bufinfo.samples -= bufinfo.rate;
	}

	return bufinfo.num;
}

//---------------------------------------------------------------------------
//
//	バッファから取得
//
//---------------------------------------------------------------------------
void FASTCALL OPMIF::GetBuf(DWORD *buf, int samples)
{
	DWORD first;
	DWORD second;
	DWORD under;
	DWORD over;

	ASSERT(this);
	ASSERT(buf);
	ASSERT(samples > 0);
	ASSERT(engine);
	ASSERT_DIAG();

	// オーバーランチェックを先に
	over = 0;
	if (bufinfo.num > (DWORD)samples) {
		over = bufinfo.num - samples;
	}

	// 初回、2回目、アンダーランの要求数を決める
	first = samples;
	second = 0;
	under = 0;
	if (bufinfo.num < first) {
		first = bufinfo.num;
		under = samples - first;
		samples = bufinfo.num;
	}
	if ((first + bufinfo.read) > BufMax) {
		first = BufMax - bufinfo.read;
		second = samples - first;
	}

	// 初回読み取り
	memcpy(buf, &opmbuf[bufinfo.read * 2], (first * 8));
	buf += (first * 2);
	bufinfo.read += first;
	bufinfo.read &= (BufMax - 1);
	bufinfo.num -= first;

	// 2回目読み取り
	if (second > 0) {
		memcpy(buf, &opmbuf[bufinfo.read * 2], (second * 8));
		bufinfo.read += second;
		bufinfo.read &= (BufMax - 1);
		bufinfo.num -= second;
	}

	// アンダーラン
	if (under > 0) {
		// この1/4だけ、次回に合成されるよう仕組む
		bufinfo.samples = 0;
		under *= 5000;
		under /= bufinfo.rate;
		scheduler->SetSoundTime(under);

		// 記録
		bufinfo.under++;
	}

	// オーバーラン
	if (over > 0) {
		// この1/4だけ、次回遅らせるよう仕組む
		over *= 5000;
		over /= bufinfo.rate;
		under = scheduler->GetSoundTime();
		if (under < over) {
			scheduler->SetSoundTime(0);
		}
		else {
			under -= over;
			scheduler->SetSoundTime(under);
		}

		// 記録
		bufinfo.over++;
	}
}
