//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 ＰＩ．(ytanaka@ipc-tokai.or.jp)
//	[ ADPCM(MSM6258V) ]
//
//---------------------------------------------------------------------------

#if !defined(adpcm_h)
#define adpcm_h

#include "device.h"
#include "event.h"

//===========================================================================
//
//	ADPCM
//
//===========================================================================
class ADPCM : public MemDevice
{
public:
	// 内部データ定義
	typedef struct {
		DWORD panpot;					// パンポット
		BOOL play;						// 再生モード
		BOOL rec;						// 録音モード
		BOOL active;					// アクティブフラグ
		BOOL started;					// 再生後有為なデータを検出
		DWORD clock;					// 供給クロック(4 or 8)
		DWORD ratio;					// クロック比率 (0 or 1 or 2)
		DWORD speed;					// 進行速度(128,192,256,384,512)
		DWORD data;						// サンプルデータ(4bit * 2sample)

		int offset;						// 合成オフセット (0-48)
		int sample;						// サンプルデータ
		int out;						// 出力データ
		int vol;						// 音量

		BOOL enable;					// イネーブルフラグ
		BOOL sound;						// ADPCM出力有効フラグ
		DWORD readpoint;				// バッファ読み込みポイント
		DWORD writepoint;				// バッファ書き込みポイント
		DWORD number;					// バッファ有効データ数
		int wait;						// 合成ウェイト
		DWORD sync_cnt;					// 同期カウンタ
		DWORD sync_rate;				// 同期レート(882,960,etc...)
		DWORD sync_step;				// 同期ステップ(線形補間対応)
		BOOL interp;					// 補間フラグ
	} adpcm_t;

public:
	// 基本ファンクション
	ADPCM(VM *p);
										// コンストラクタ
	BOOL FASTCALL Init();
										// 初期化
	void FASTCALL Cleanup();
										// クリーンアップ
	void FASTCALL Reset();
										// リセット
	BOOL FASTCALL Save(Fileio *fio, int ver);
										// セーブ
	BOOL FASTCALL Load(Fileio *fio, int ver);
										// ロード
	void FASTCALL ApplyCfg(const Config *config);
										// 設定適用
#if !defined(NDEBUG)
	void FASTCALL AssertDiag() const;
										// 診断
#endif	// NDEBUG

	// メモリデバイス
	DWORD FASTCALL ReadByte(DWORD addr);
										// バイト読み込み
	DWORD FASTCALL ReadWord(DWORD addr);
										// ワード読み込み
	void FASTCALL WriteByte(DWORD addr, DWORD data);
										// バイト書き込み
	void FASTCALL WriteWord(DWORD addr, DWORD data);
										// ワード書き込み
	DWORD FASTCALL ReadOnly(DWORD addr) const;
										// 読み込みのみ

	// 外部API
	void FASTCALL GetADPCM(adpcm_t *buffer);
										// 内部データ取得
	BOOL FASTCALL Callback(Event *ev);
										// イベントコールバック
	void FASTCALL SetClock(DWORD clk);
										// 基準クロック指定
	void FASTCALL SetRatio(DWORD ratio);
										// クロック比率指定
	void FASTCALL SetPanpot(DWORD pan);
										// パンポット指定
	void FASTCALL Enable(BOOL enable);
										// 合成イネーブル
	void FASTCALL InitBuf(DWORD rate);
										// バッファ初期化
	void FASTCALL GetBuf(DWORD *buffer, int samples);
										// バッファ取得
	void FASTCALL Wait(int num);
										// ウェイト指定
	void FASTCALL EnableADPCM(BOOL flag) { adpcm.sound = flag; }
										// 再生有効
	void FASTCALL SetVolume(int volume);
										// 音量設定
	void FASTCALL ClrStarted()			{ adpcm.started = FALSE; }
										// スタートフラグクリア
	BOOL FASTCALL IsStarted() const		{ return adpcm.started; }
										// スタートフラグ取得

private:
	enum {
		BufMax = 0x10000				// バッファサイズ
	};
	void FASTCALL MakeTable();
										// テーブル作成
	void FASTCALL CalcSpeed();
										// 速度再計算
	void FASTCALL Start(int type);
										// 録音・再生スタート
	void FASTCALL Stop();
										// 録音・再生ストップ
	void FASTCALL Decode(int data, int num, BOOL valid);
										// 4bitデコード
	Event event;
										// タイマーイベント
	adpcm_t adpcm;
										// 内部データ
	DMAC *dmac;
										// DMAC
	DWORD *adpcmbuf;
										// 合成バッファ
	int DiffTable[49 * 16];
										// 差分テーブル
	static const int NextTable[16];
										// 変位テーブル
	static const int OffsetTable[58];
										// オフセットテーブル
};

#endif	// adpcm_h
