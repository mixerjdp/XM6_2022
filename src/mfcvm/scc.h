//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2003 ＰＩ．(ytanaka@ipc-tokai.or.jp)
//	[ SCC(Z8530) ]
//
//---------------------------------------------------------------------------

#if !defined(scc_h)
#define scc_h

#include "device.h"
#include "event.h"

//===========================================================================
//
//	SCC
//
//===========================================================================
class SCC : public MemDevice
{
public:
	// 割り込みタイプ
	enum itype_t {
		rxi,							// 受信割り込み
		rsi,							// スペシャルRxコンディション割り込み
		txi,							// 送信割り込み
		exti							// 外部ステータス変化割り込み
	};

	// チャネル定義
	typedef struct {
		// グローバル
		DWORD index;					// チャネル番号(0 or 1)

		// RR0
		BOOL ba;						// Break/Abort
		BOOL tu;						// Txアンダーラン
		BOOL cts;						// CTS
		BOOL sync;						// SYNC
		BOOL dcd;						// DCD
		BOOL zc;						// ゼロカウント

		// WR0
		DWORD reg;						// アクセスレジスタ選択
		BOOL ph;						// ポイントハイ(上位レジスタ選択)
		BOOL txpend;					// 送信割り込みペンディング
		BOOL rxno;						// 受信データなし

		// RR1
		BOOL framing;					// フレーミングエラー
		BOOL overrun;					// オーバーランエラー
		BOOL parerr;					// パリティエラー
		BOOL txsent;					// 送信完了

		// WR1
		BOOL extie;						// 外部ステータス割り込み許可
		BOOL txie;						// 送信割り込み許可
		BOOL parsp;						// パリティエラーをS-Rx割り込みにする
		DWORD rxim;						// 受信割り込みモード

		// RR3
		BOOL rxip;						// 受信割り込みペンディング
		BOOL rsip;						// スペシャルRx割り込みペンディング
		BOOL txip;						// 送信割り込みペンディング
		BOOL extip;						// 外部ステータス変化割り込みペンディング

		// WR3
		DWORD rxbit;					// 受信キャラクタビット長(5-8)
		BOOL aen;						// オートモードイネーブル
		BOOL rxen;						// 受信イネーブル

		// WR4
		DWORD clkm;						// クロックモード
		DWORD stopbit;					// ストップビット
		DWORD parity;					// パリティモード

		// WR5
		BOOL dtr;						// DTR信号線
		DWORD txbit;					// 送信キャラクタビット長(5-8)
		BOOL brk;						// ブレーク送出
		BOOL txen;						// 送信イネーブル
		BOOL rts;						// RTS信号線

		// WR8
		DWORD tdr;						// 送信データレジスタ
		BOOL tdf;						// 送信データ有効

		// WR12, WR13
		DWORD tc;						// ボーレート設定値

		// WR14
		BOOL loopback;					// ループバックモード
		BOOL aecho;						// オートエコーモード
		BOOL dtrreq;					// DTR信号線有効
		BOOL brgsrc;					// ボーレートジェネレータクロック源
		BOOL brgen;						// ボーレートジェネレータイネーブル

		// WR15
		BOOL baie;						// Break/Abort割り込みイネーブル
		BOOL tuie;						// Txアンダーラン割り込みイネーブル
		BOOL ctsie;						// CTS割り込みイネーブル
		BOOL syncie;					// SYNC割り込みイネーブル
		BOOL dcdie;						// DCD割り込みイネーブル
		BOOL zcie;						// ゼロカウント割り込みイネーブル

		// 通信速度
		DWORD baudrate;					// ボーレート
		DWORD cps;						// キャラクタ/sec
		DWORD speed;					// 速度(hus単位)

		// 受信FIFO
		DWORD rxfifo;					// 受信FIFO有効数
		DWORD rxdata[3];				// 受信FIFOデータ

		// 受信バッファ
		BYTE rxbuf[0x1000];				// 受信データ
		DWORD rxnum;					// 受信データ数
		DWORD rxread;					// 受信読み込みポインタ
		DWORD rxwrite;					// 受信書き込みポインタ
		DWORD rxtotal;					// 受信トータル

		// 送信バッファ
		BYTE txbuf[0x1000];				// 送信データ
		DWORD txnum;					// 送信データ数
		DWORD txread;					// 送信読み込みポインタ
		DWORD txwrite;					// 送信書き込みポインタ
		DWORD txtotal;					// Txトータル
		BOOL txwait;					// Txウェイトフラグ
	} ch_t;

	// 内部データ定義
	typedef struct {
		// チャネル
		ch_t ch[2];						// チャネルデータ

		// RR2
		DWORD request;					// 割り込みベクタ(要求中)

		// WR2
		DWORD vbase;					// 割り込みベクタ(ベース)

		// WR9
		BOOL shsl;						// ベクタ変化モードb4-b6/b3-b1
		BOOL mie;						// 割り込みイネーブル
		BOOL dlc;						// 下位チェーン禁止
		BOOL nv;						// 割り込みベクタ出力イネーブル
		BOOL vis;						// 割り込みベクタ変化モード

		int ireq;						// 要求中の割り込みタイプ
		int vector;						// 要求中のベクタ
	} scc_t;

public:
	// 基本ファンクション
	SCC(VM *p);
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
	void FASTCALL GetSCC(scc_t *buffer) const;
										// 内部データ取得
	const SCC::scc_t* FASTCALL GetWork() const;
										// ワーク取得 
	DWORD FASTCALL GetVector(int type) const;
										// ベクタ取得
	BOOL FASTCALL Callback(Event *ev);
										// イベントコールバック
	void FASTCALL IntAck();
										// 割り込み応答

	// 送信API(SCCへ送信)
	void FASTCALL Send(int channel, DWORD data);
										// データ送信
	void FASTCALL ParityErr(int channel);
										// パリティエラーの生成
	void FASTCALL FramingErr(int channel);
										// フレーミングエラーの生成
	void FASTCALL SetBreak(int channel, BOOL flag);
										// ブレーク状態の通知
	BOOL FASTCALL IsRxEnable(int channel) const;
										// 受信チェック
	BOOL FASTCALL IsBaudRate(int channel, DWORD baudrate) const;
										// ボーレートチェック
	DWORD FASTCALL GetRxBit(int channel) const;
										// 受信データビット数取得
	DWORD FASTCALL GetStopBit(int channel) const;
										// ストップビット取得
	DWORD FASTCALL GetParity(int channel) const;
										// パリティ取得
	BOOL FASTCALL IsRxBufEmpty(int channel) const;
										// 受信バッファの空きチェック

	// 受信API(SCCから受信)
	DWORD FASTCALL Receive(int channel);
										// データ受信
	BOOL FASTCALL IsTxEmpty(int channel);
										// 送信バッファエンプティチェック
	BOOL FASTCALL IsTxFull(int channel);
										// 送信バッファフルチェック
	void FASTCALL WaitTx(int channel, BOOL wait);
										// 送信ブロック

	// ハードフロー
	void FASTCALL SetCTS(int channel, BOOL flag);
										// CTSセット
	void FASTCALL SetDCD(int channel, BOOL flag);
										// DCDセット
	BOOL FASTCALL GetRTS(int channel);
										// RTS取得
	BOOL FASTCALL GetDTR(int channel);
										// DTR取得
	BOOL FASTCALL GetBreak(int channel);
										// ブレーク取得

private:
	void FASTCALL ResetCh(ch_t *p);
										// チャネルリセット
	DWORD FASTCALL ReadSCC(ch_t *p, DWORD reg);
										// チャネル読み出し
	DWORD FASTCALL ReadRR0(const ch_t *p) const;
										// RR0読み出し
	DWORD FASTCALL ReadRR1(const ch_t *p) const;
										// RR1読み出し
	DWORD FASTCALL ReadRR2(ch_t *p);
										// RR2読み出し
	DWORD FASTCALL ReadRR3(const ch_t *p) const;
										// RR3読み出し
	DWORD FASTCALL ReadRR8(ch_t *p);
										// RR8読み出し
	DWORD FASTCALL ReadRR15(const ch_t *p) const;
										// RR15読み出し
	DWORD FASTCALL ROSCC(const ch_t *p, DWORD reg) const;
										// 読み出しのみ
	void FASTCALL WriteSCC(ch_t *p, DWORD reg, DWORD data);
										// チャネル書き込み
	void FASTCALL WriteWR0(ch_t *p, DWORD data);
										// WR0書き込み
	void FASTCALL WriteWR1(ch_t *p, DWORD data);
										// WR1書き込み
	void FASTCALL WriteWR3(ch_t *p, DWORD data);
										// WR3書き込み
	void FASTCALL WriteWR4(ch_t *p, DWORD data);
										// WR4書き込み
	void FASTCALL WriteWR5(ch_t *p, DWORD data);
										// WR5書き込み
	void FASTCALL WriteWR8(ch_t *p, DWORD data);
										// WR8書き込み
	void FASTCALL WriteWR9(DWORD data);
										// WR9書き込み
	void FASTCALL WriteWR10(ch_t *p, DWORD data);
										// WR10書き込み
	void FASTCALL WriteWR11(ch_t *p, DWORD data);
										// WR11書き込み
	void FASTCALL WriteWR12(ch_t *p, DWORD data);
										// WR12書き込み
	void FASTCALL WriteWR13(ch_t *p, DWORD data);
										// WR13書き込み
	void FASTCALL WriteWR14(ch_t *p, DWORD data);
										// WR14書き込み
	void FASTCALL WriteWR15(ch_t *p, DWORD data);
										// WR15書き込み
	void FASTCALL ResetSCC(ch_t *p);
										// リセット
	void FASTCALL ClockSCC(ch_t *p);
										// ボーレート再計算
	void FASTCALL IntSCC(ch_t *p, itype_t type, BOOL flag);
										// 割り込みリクエスト
	void FASTCALL IntCheck();
										// 割り込みチェック
	void FASTCALL EventRx(ch_t *p);
										// イベント(受信)
	void FASTCALL EventTx(ch_t *p);
										// イベント(送信)
	Mouse *mouse;
										// マウス
	scc_t scc;
										// 内部データ
	Event event[2];
										// イベント
	BOOL clkup;
										// 7.5MHzモード
};

#endif	// scc_h
