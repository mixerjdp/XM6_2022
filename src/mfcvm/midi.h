//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2005 ＰＩ．(ytanaka@ipc-tokai.or.jp)
//	[ MIDI(YM3802) ]
//
//---------------------------------------------------------------------------

#if !defined(midi_h)
#define midi_h

#include "device.h"
#include "event.h"

//===========================================================================
//
//	MIDI
//
//===========================================================================
class MIDI : public MemDevice
{
public:
	// 定数定義
	enum {
		TransMax = 0x2000,				// 送信バッファ個数
		RecvMax = 0x2000				// 受信バッファ個数
	};

	// MIDIバイトデータ定義
	typedef struct {
		DWORD data;						// データ実体(8bit)
		DWORD vtime;					// 仮想時間
	} mididata_t;

	// 内部データ定義
	typedef struct {
		// リセット
		BOOL reset;						// リセットフラグ
		BOOL access;					// アクセスフラグ

		// ボードデータ、割り込み
		DWORD bid;						// ボードID(0:ボード無し)
		DWORD ilevel;					// 割り込みレベル
		int vector;						// 割り込み要求ベクタ

		// MCSレジスタ(一般)
		DWORD wdr;						// 書き込みデータレジスタ
		DWORD rgr;						// レジスタグループレジスタ

		// MCSレジスタ(割り込み)
		DWORD ivr;						// 割り込みベクタレジスタ
		DWORD isr;						// 割り込みサービスレジスタ
		DWORD imr;						// 割り込みモードレジスタ
		DWORD ier;						// 割り込み許可レジスタ

		// MCSレジスタ(リアルタイムメッセージ)
		DWORD dmr;						// リアルタイムメッセージモードレジスタ
		DWORD dcr;						// リアルタイムメッセージコントロールレジスタ

		// MCSレジスタ(受信)
		DWORD rrr;						// 受信レートレジスタ
		DWORD rmr;						// 受信モードレジスタ
		DWORD amr;						// アドレスハンタモードレジスタ
		DWORD adr;						// アドレスハンタデバイスレジスタ
		DWORD asr;						// アドレスハンタステータスレジスタ
		DWORD rsr;						// 受信バッファステータスレジスタ
		DWORD rcr;						// 受信バッファコントロールレジスタ
		DWORD rcn;						// 無受信カウンタ

		// MCSレジスタ(送信)
		DWORD trr;						// 送信レートレジスタ
		DWORD tmr;						// 送信モードレジスタ
		BOOL tbs;						// 送信BUSYレジスタ
		DWORD tcr;						// 送信コントロールレジスタ
		DWORD tcn;						// 無送信カウンタ

		// MCSレジスタ(FSK)
		DWORD fsr;						// FSKステータスレジスタ
		DWORD fcr;						// FSKコントロールレジスタ

		// MCSレジスタ(カウンタ)
		DWORD ccr;						// クリックコントロールレジスタ
		DWORD cdr;						// クリックデータレジスタ
		DWORD ctr;						// クリックタイマレジスタ
		DWORD srr;						// レコーディングカウンタレジスタ
		DWORD scr;						// クロック補間レジスタ
		DWORD sct;						// クロック補間カウンタ
		DWORD spr;						// プレイバックカウンタレジスタ
		DWORD str;						// プレイバックタイマレジスタ
		DWORD gtr;						// 汎用タイマレジスタ
		DWORD mtr;						// MIDIクロックタイマレジスタ

		// MCSレジスタ(GPIO)
		DWORD edr;						// 外部ポートディレクションレジスタ
		DWORD eor;						// 外部ポートOutputレジスタ
		DWORD eir;						// 外部ポートInputレジスタ

		// 通常バッファ
		DWORD normbuf[16];				// 通常バッファ
		DWORD normread;					// 通常バッファRead
		DWORD normwrite;				// 通常バッファWrite
		DWORD normnum;					// 通常バッファ個数
		DWORD normtotal;				// 通常バッファトータル

		// リアルタイム送信バッファ
		DWORD rtbuf[4];					// リアルタイム送信バッファ
		DWORD rtread;					// リアルタイム送信バッファRead
		DWORD rtwrite;					// リアルタイム送信バッファWrite
		DWORD rtnum;					// リアルタイム送信バッファ個数
		DWORD rttotal;					// リアルタイム送信バッファトータル

		// 一般バッファ
		DWORD stdbuf[0x80];				// 一般バッファ
		DWORD stdread;					// 一般バッファRead
		DWORD stdwrite;					// 一般バッファWrite
		DWORD stdnum;					// 一般バッファ個数
		DWORD stdtotal;					// 一般バッファトータル

		// リアルタイム受信バッファ
		DWORD rrbuf[4];					// リアルタイム受信バッファ
		DWORD rrread;					// リアルタイム受信バッファRead
		DWORD rrwrite;					// リアルタイム受信バッファWrite
		DWORD rrnum;					// リアルタイム受信バッファ個数
		DWORD rrtotal;					// リアルタイム受信バッファトータル

		// 送信バッファ(デバイスとの受け渡し用)
		mididata_t *transbuf;			// 送信バッファ
		DWORD transread;				// 送信バッファRead
		DWORD transwrite;				// 送信バッファWrite
		DWORD transnum;					// 送信バッファ個数

		// 受信バッファ(デバイスとの受け渡し用)
		mididata_t *recvbuf;			// 受信バッファ
		DWORD recvread;					// 受信バッファRead
		DWORD recvwrite;				// 受信バッファWrite
		DWORD recvnum;					// 受信バッファ個数
	} midi_t;

public:
	// 基本ファンクション
	MIDI(VM *p);
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
	BOOL FASTCALL IsActive() const;
										// MIDIアクティブチェック
	BOOL FASTCALL Callback(Event *ev);
										// イベントコールバック
	void FASTCALL IntAck(int level);
										// 割り込みACK
	void FASTCALL GetMIDI(midi_t *buffer) const;
										// 内部データ取得
	DWORD FASTCALL GetExCount(int index) const;
										// エクスクルーシブカウント取得

	// 送信(MIDI OUT)
	DWORD FASTCALL GetTransNum() const;
										// 送信バッファ個数取得
	const mididata_t* FASTCALL GetTransData(DWORD proceed);
										// 送信バッファデータ取得
	void FASTCALL DelTransData(DWORD number);
										// 送信バッファ削除
	void FASTCALL ClrTransData();
										// 送信バッファクリア

	// 受信(MIDI IN)
	void FASTCALL SetRecvData(const BYTE *ptr, DWORD length);
										// 受信データ設定
	void FASTCALL SetRecvDelay(int delay);
										// 受信ディレイ設定

	// リセット
	BOOL FASTCALL IsReset() const		{ return midi.reset; }
										// リセットフラグ取得
	void FASTCALL ClrReset()			{ midi.reset = FALSE; }
										// リセットフラグクリア

private:
	void FASTCALL Receive();
										// 受信コールバック
	void FASTCALL Transmit();
										// 送信コールバック
	void FASTCALL Clock();
										// MIDIクロック検出
	void FASTCALL General();
										// 汎用タイマコールバック

	void FASTCALL InsertTrans(DWORD data);
										// 送信バッファへ挿入
	void FASTCALL InsertRecv(DWORD data);
										// 受信バッファへ挿入
	void FASTCALL InsertNorm(DWORD data);
										// 通常バッファへ挿入
	void FASTCALL InsertRT(DWORD data);
										// リアルタイム送信バッファへ挿入
	void FASTCALL InsertStd(DWORD data);
										// 一般バッファへ挿入
	void FASTCALL InsertRR(DWORD data);
										// リアルタイム受信バッファへ挿入

	void FASTCALL ResetReg();
										// レジスタリセット
	DWORD FASTCALL ReadReg(DWORD reg);
										// レジスタ読み出し
	void FASTCALL WriteReg(DWORD reg, DWORD data);
										// レジスタ書き込み
	DWORD FASTCALL ReadRegRO(DWORD reg) const;
										// レジスタ読み出し(ReadOnly)

	void FASTCALL SetICR(DWORD data);
										// ICR設定
	void FASTCALL SetIOR(DWORD data);
										// IOR設定
	void FASTCALL SetIMR(DWORD data);
										// IMR設定
	void FASTCALL SetIER(DWORD data);
										// IER設定
	void FASTCALL SetDMR(DWORD data);
										// DMR設定
	void FASTCALL SetDCR(DWORD data);
										// DCR設定
	DWORD FASTCALL GetDSR() const;
										// DSR取得
	void FASTCALL SetDNR(DWORD data);
										// DNR設定
	void FASTCALL SetRRR(DWORD data);
										// RRR設定
	void FASTCALL SetRMR(DWORD data);
										// RMR設定
	void FASTCALL SetAMR(DWORD data);
										// AMR設定
	void FASTCALL SetADR(DWORD data);
										// ADR設定
	DWORD FASTCALL GetRSR() const;
										// RSR取得
	void FASTCALL SetRCR(DWORD data);
										// RCR設定
	DWORD FASTCALL GetRDR();
										// RDR取得(更新あり)
	DWORD FASTCALL GetRDRRO() const;
										// RDR取得(Read Only)
	void FASTCALL SetTRR(DWORD data);
										// TRR設定
	void FASTCALL SetTMR(DWORD data);
										// TMR設定
	DWORD FASTCALL GetTSR() const;
										// TSR取得
	void FASTCALL SetTCR(DWORD data);
										// TCR設定
	void FASTCALL SetTDR(DWORD data);
										// TDR設定
	DWORD FASTCALL GetFSR() const;
										// FSR取得
	void FASTCALL SetFCR(DWORD data);
										// FCR設定
	void FASTCALL SetCCR(DWORD data);
										// CCR設定
	void FASTCALL SetCDR(DWORD data);
										// CDR設定
	DWORD FASTCALL GetSRR() const;
										// SRR取得
	void FASTCALL SetSCR(DWORD data);
										// SCR設定
	void FASTCALL SetSPR(DWORD data, BOOL high);
										// SPR設定
	void FASTCALL SetGTR(DWORD data, BOOL high);
										// GTR設定
	void FASTCALL SetMTR(DWORD data, BOOL high);
										// MTR設定
	void FASTCALL SetEDR(DWORD data);
										// EDR設定
	void FASTCALL SetEOR(DWORD data);
										// EOR設定
	DWORD FASTCALL GetEIR() const;
										// EIR取得

	void FASTCALL CheckRR();
										// リアルタイムメッセージ受信バッファチェック
	void FASTCALL Interrupt(int type, BOOL flag);
										// 割り込み発生
	void FASTCALL IntCheck();
										// 割り込みチェック
	Event event[3];
										// イベント
	midi_t midi;
										// 内部データ
	Sync *sync;
										// データSync
	DWORD recvdelay;
										// 受信遅れ時間(hus)
	DWORD ex_cnt[4];
										// エクスクルーシブカウント
};

#endif	// midi_h
