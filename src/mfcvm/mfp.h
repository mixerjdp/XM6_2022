//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2004 ＰＩ．(ytanaka@ipc-tokai.or.jp)
//	[ MFP(MC68901) ]
//
//---------------------------------------------------------------------------

#if !defined(mfp_h)
#define mfp_h

#include "device.h"
#include "event.h"

//===========================================================================
//
//	MFP
//
//===========================================================================
class MFP : public MemDevice
{
public:
	// 内部データ定義
	typedef struct {
		// 割り込み
		BOOL ier[0x10];					// 割り込みイネーブルレジスタ
		BOOL ipr[0x10];					// 割り込みペンディングレジスタ
		BOOL isr[0x10];					// 割り込みインサービスレジスタ
		BOOL imr[0x10];					// 割り込みマスクレジスタ
		BOOL ireq[0x10];				// 割り込みリクエストレジスタ
		DWORD vr;						// ベクタレジスタ
		int iidx;						// 割り込みインデックス

		// タイマ
		DWORD tcr[4];					// タイマコントロールレジスタ
		DWORD tdr[4];					// タイマデータレジスタ
		DWORD tir[4];					// タイマインターナルレジスタ
		DWORD tbr[2];					// タイマバックアップレジスタ
		DWORD sram;						// si, info.ram対策フラグ
		DWORD tecnt;					// イベントカウントモードカウンタ

		// GPIP
		DWORD gpdr;						// GPIPデータレジスタ
		DWORD aer;						// アクティブエッジレジスタ
		DWORD ddr;						// データ方向レジスタ
		DWORD ber;						// バックアップエッジレジスタ

		// USART
		DWORD scr;						// SYNCキャラクタレジスタ
		DWORD ucr;						// USARTコントロールレジスタ
		DWORD rsr;						// レシーバステータスレジスタ
		DWORD tsr;						// トランスミッタステータスレジスタ
		DWORD rur;						// レシーバユーザレジスタ
		DWORD tur;						// トランスミッタユーザレジスタ
		DWORD buffer[0x10];				// USART FIFOバッファ
		int datacount;					// USART 有効データ数
		int readpoint;					// USART MFP読み取りポイント
		int writepoint;					// USART キーボード書き込みポイント
	} mfp_t;

public:
	// 基本ファンクション
	MFP(VM *p);
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
	void FASTCALL GetMFP(mfp_t *buffer) const;
										// 内部データ取得
	BOOL FASTCALL Callback(Event *ev);
										// イベントコールバック
	void FASTCALL IntAck();
										// 割り込み応答
	void FASTCALL EventCount(int channel, int value);
										// イベントカウント
	void FASTCALL SetGPIP(int num, int value);
										// GPIP設定
	void FASTCALL KeyData(DWORD data);
										// キーデータ設定
	DWORD FASTCALL GetVR() const;
										// ベクタレジスタ取得

private:
	// 割り込みコントロール
	void FASTCALL Interrupt(int level, BOOL enable);
										// 割り込み
	void FASTCALL IntCheck();
										// 割り込み優先順位チェック
	void FASTCALL SetIER(int offset, DWORD data);
										// IER設定
	DWORD FASTCALL GetIER(int offset) const;
										// IER取得
	void FASTCALL SetIPR(int offset, DWORD data);
										// IPR設定
	DWORD FASTCALL GetIPR(int offset) const;
										// IPR取得
	void FASTCALL SetISR(int offset, DWORD data);
										// ISR設定
	DWORD FASTCALL GetISR(int offset) const;
										// ISR取得
	void FASTCALL SetIMR(int offset, DWORD data);
										// IMR設定
	DWORD FASTCALL GetIMR(int offset) const;
										// IMR設定
	void FASTCALL SetVR(DWORD data);
										// VR設定
	static const char* IntDesc[0x10];
										// 割り込み名称テーブル

	// タイマ
	void FASTCALL SetTCR(int channel, DWORD data);
										// TCR設定
	DWORD FASTCALL GetTCR(int channel) const;
										// TCR取得
	void FASTCALL SetTDR(int channel, DWORD data);
										// TDR設定
	DWORD FASTCALL GetTIR(int channel) const;
										// TIR取得
	void FASTCALL Proceed(int channel);
										// タイマを進める
	Event timer[4];
										// タイマイベント
	static const int TimerInt[4];
										// タイマ割り込みテーブル
	static const DWORD TimerHus[8];
										// タイマ時間テーブル

	// GPIP
	void FASTCALL SetGPDR(DWORD data);
										// GPDR設定
	void FASTCALL IntGPIP();
										// GPIP割り込み
	static const int GPIPInt[8];
										// GPIP割り込みテーブル

	// USART
	void FASTCALL SetRSR(DWORD data);
										// RSR設定
	void FASTCALL Receive();
										// USARTデータ受信
	void FASTCALL SetTSR(DWORD data);
										// TSR設定
	void FASTCALL Transmit(DWORD data);
										// USARTデータ送信
	void FASTCALL USART();
										// USART処理
	Event usart;
										// USARTイベント
	Sync *sync;
										// USART Sync
	Keyboard *keyboard;
										// キーボード
	mfp_t mfp;
										// 内部データ
};

#endif	// mfp_h
