//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 ＰＩ．(ytanaka@ipc-tokai.or.jp)
//	[ DMAC(HD63450) ]
//
//---------------------------------------------------------------------------

#if !defined(dmac_h)
#define dmac_h

#include "device.h"

//===========================================================================
//
//	DMAC
//
//===========================================================================
class DMAC : public MemDevice
{
public:
	// 内部データ定義(チャネル別)
	typedef struct {
		// 基本パラメータ
		DWORD xrm;						// リクエストモード
		DWORD dtyp;						// デバイスタイプ
		BOOL dps;						// ポートサイズ (TRUEで16bit)
		DWORD pcl;						// PCLセレクタ
		BOOL dir;						// 方向 (TRUEでDAR→メモリ)
		BOOL btd;						// DONEで次ブロックへ
		DWORD size;						// オペランドサイズ
		DWORD chain;					// チェイン動作
		DWORD reqg;						// REQ生成モード
		DWORD mac;						// メモリアドレス更新モード
		DWORD dac;						// デバイスアドレス更新モード

		// 制御フラグ
		BOOL str;						// スタートフラグ
		BOOL cnt;						// コンティニューフラグ
		BOOL hlt;						// HALTフラグ
		BOOL sab;						// ソフトウェアアボートフラグ
		BOOL intr;						// 割り込み可能フラグ
		BOOL coc;						// チャンネル動作完了フラグ
		BOOL boc;						// ブロック動作完了フラグ
		BOOL ndt;						// 正常終了フラグ
		BOOL err;						// エラーフラグ
		BOOL act;						// アクティブフラグ
		BOOL dit;						// DONE入力フラグ
		BOOL pct;						// PCL negedge検出フラグ
		BOOL pcs;						// PCLの状態 (TRUEでHレベル)
		DWORD ecode;					// エラーコード

		// アドレス、レングス
		DWORD mar;						// メモリアドレスカウンタ
		DWORD dar;						// デバイスアドレスレジスタ
		DWORD bar;						// ベースアドレスレジスタ
		DWORD mtc;						// メモリトランスファカウンタ
		DWORD btc;						// ベーストランスファカウンタ
		DWORD mfc;						// メモリファンクションコード
		DWORD dfc;						// デバイスファンクションコード
		DWORD bfc;						// ベースファンクションコード
		DWORD niv;						// ノーマルインタラプトベクタ
		DWORD eiv;						// エラーインタラプトベクタ

		// バースト転送
		DWORD cp;						// プライオリティ
		DWORD bt;						// バースト転送タイム
		DWORD br;						// バンド幅
		int type;						// 転送タイプ

		// 動作カウンタ(デバッグ向け)
		DWORD startcnt;					// スタートカウンタ
		DWORD errorcnt;					// エラーカウンタ
	} dma_t;

	// 内部データ定義(グローバル)
	typedef struct {
		int transfer;					// 転送中フラグ(チャネル兼用)
		int load;						// チェインロードフラグ(チャネル兼用)
		BOOL exec;						// オートリクエスト有無フラグ
		int current_ch;					// オートリクエスト処理チャネル
		int cpu_cycle;					// CPUサイクルカウンタ
		int vector;						// 割り込み要求中ベクタ
	} dmactrl_t;

public:
	// 基本ファンクション
	DMAC(VM *p);
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
										// ワード読み込み
	DWORD FASTCALL ReadOnly(DWORD addr) const;
										// 読み込みのみ

	// 外部API
	void FASTCALL GetDMA(int ch, dma_t *buffer) const;
										// DMA情報取得
	void FASTCALL GetDMACtrl(dmactrl_t *buffer) const;
										// DMA制御情報取得
	BOOL FASTCALL ReqDMA(int ch);
										// DMA転送要求
	DWORD FASTCALL AutoDMA(DWORD cycle);
										// DMAオートリクエスト
	BOOL FASTCALL IsDMA() const;
										// DMA転送中か問い合わせ
	void FASTCALL BusErr(DWORD addr, BOOL read);
										// バスエラー
	void FASTCALL AddrErr(DWORD addr, BOOL read);
										// アドレスエラー
	DWORD FASTCALL GetVector(int type) const;
										// ベクタ取得
	void FASTCALL IntAck();
										// 割り込みACK
	BOOL FASTCALL IsAct(int ch) const;
										// DMA転送可能か問い合わせ

private:
	// チャネルメモリアクセス
	DWORD FASTCALL ReadDMA(int ch, DWORD addr) const;
										// DMA読み込み
	void FASTCALL WriteDMA(int ch, DWORD addr, DWORD data);
										// DMA書き込み
	void FASTCALL SetDCR(int ch, DWORD data);
										// DCRセット
	DWORD FASTCALL GetDCR(int ch) const;
										// DCR取得
	void FASTCALL SetOCR(int ch, DWORD data);
										// OCRセット
	DWORD FASTCALL GetOCR(int ch) const;
										// OCR取得
	void FASTCALL SetSCR(int ch, DWORD data);
										// SCRセット
	DWORD FASTCALL GetSCR(int ch) const;
										// SCR取得
	void FASTCALL SetCCR(int ch, DWORD data);
										// CCRセット
	DWORD FASTCALL GetCCR(int ch) const;
										// CCR取得
	void FASTCALL SetCSR(int ch, DWORD data);
										// CSRセット
	DWORD FASTCALL GetCSR(int ch) const;
										// CSR取得
	void FASTCALL SetGCR(DWORD data);
										// GCRセット

	// チャネルオペレーション
	void FASTCALL ResetDMA(int ch);
										// DMAリセット
	void FASTCALL StartDMA(int ch);
										// DMAスタート
	void FASTCALL ContDMA(int ch);
										// DMAコンティニュー
	void FASTCALL AbortDMA(int ch);
										// DMAソフトウェアアボート
	void FASTCALL LoadDMA(int ch);
										// DMAブロックロード
	void FASTCALL ErrorDMA(int ch, DWORD code);
										// エラー
	void FASTCALL Interrupt();
										// 割り込み
	BOOL FASTCALL TransDMA(int ch);
										// DMA1回転送

	// テーブル、内部ワーク
	static const int MemDiffTable[8][4];
										// メモリ更新テーブル
	static const int DevDiffTable[8][4];
										// デバイス更新テーブル
	Memory *memory;
										// メモリ
	FDC *fdc;
										// FDC
	dma_t dma[4];
										// 内部ワーク(チャネル)
	dmactrl_t dmactrl;
										// 内部ワーク(グローバル)
};

#endif	// dmac_h
