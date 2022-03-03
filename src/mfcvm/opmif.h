//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 ＰＩ．(ytanaka@ipc-tokai.or.jp)
//	[ OPM(YM2151) ]
//
//---------------------------------------------------------------------------

#if !defined(opmif_h)
#define opmif_h

#include "device.h"
#include "event.h"
#include "opm.h"

//===========================================================================
//
//	OPM
//
//===========================================================================
class OPMIF : public MemDevice
{
public:
	// 内部データ定義
	typedef struct {
		DWORD reg[0x100];				// レジスタ
		DWORD key[8];					// キー情報
		DWORD addr;						// セレクトアドレス
		BOOL busy;						// BUSYフラグ
		BOOL enable[2];					// タイマイネーブル
		BOOL action[2];					// タイマ動作
		BOOL interrupt[2];				// タイマ割り込み
		DWORD time[2];					// タイマ時間
		BOOL started;					// 開始フラグ
	} opm_t;

	// バッファ管理定義
	typedef struct {
		DWORD max;						// 最大数
		DWORD num;						// 有効データ数
		DWORD read;						// 読み取りポイント
		DWORD write;					// 書き込みポイント
		DWORD samples;					// 合成サンプル数
		DWORD rate;						// 合成レート
		DWORD under;					// アンダーラン
		DWORD over;						// オーバーラン
		BOOL sound;						// FM有効
	} opmbuf_t;

public:
	// 基本ファンクション
	OPMIF(VM *p);
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
	void FASTCALL GetOPM(opm_t *buffer);
										// 内部データ取得
	BOOL FASTCALL Callback(Event *ev);
										// イベントコールバック
	void FASTCALL Output(DWORD addr, DWORD data);
										// レジスタ出力
	void FASTCALL SetEngine(FM::OPM *p);
										// エンジン指定
	void FASTCALL InitBuf(DWORD rate);
										// バッファ初期化
	DWORD FASTCALL ProcessBuf();
										// バッファ処理
	void FASTCALL GetBuf(DWORD *buf, int samples);
										// バッファより取得
	void FASTCALL GetBufInfo(opmbuf_t *buffer);
										// バッファ情報を得る
	void FASTCALL EnableFM(BOOL flag)	{ bufinfo.sound = flag; }
										// FM音源有効
	void FASTCALL ClrStarted()			{ opm.started = FALSE; }
										// スタートフラグを降ろす
	BOOL FASTCALL IsStarted() const		{ return opm.started; }
										// スタートフラグ取得

private:
	void FASTCALL CalcTimerA();
										// タイマA算出
	void FASTCALL CalcTimerB();
										// タイマB算出
	void FASTCALL CtrlTimer(DWORD data);
										// タイマ制御
	void FASTCALL CtrlCT(DWORD data);
										// CT制御
	MFP *mfp;
										// MFP
	ADPCM *adpcm;
										// ADPCM
	FDD *fdd;
										// FDD
	opm_t opm;
										// OPM内部データ
	opmbuf_t bufinfo;
										// バッファ情報
	Event event[2];
										// タイマーイベント
	FM::OPM *engine;
										// 合成エンジン
	enum {
		BufMax = 0x10000				// バッファサイズ
	};
	DWORD *opmbuf;
										// 合成バッファ
};

#endif	// opmif_h
