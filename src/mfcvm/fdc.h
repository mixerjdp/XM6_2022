//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2005 ＰＩ．(ytanaka@ipc-tokai.or.jp)
//	[ FDC(uPD72065) ]
//
//---------------------------------------------------------------------------

#if !defined(fdc_h)
#define fdc_h

#include "device.h"
#include "event.h"

//===========================================================================
//
//	FDC
//
//===========================================================================
class FDC : public MemDevice
{
public:
	// フェーズ定義
	enum fdcphase {
		idle,							// アイドルフェーズ
		command,						// コマンドフェーズ
		execute,						// 実行フェーズ(通常)
		read,							// 実行フェーズ(Read)
		write,							// 実行フェーズ(Write)
		result							// リザルトフェーズ
	};

	// ステータスレジスタ定義
	enum {
		sr_rqm = 0x80,					// Request For Master
		sr_dio = 0x40,					// Data Input / Output
		sr_ndm = 0x20,					// Non-DMA Mode
		sr_cb = 0x10,					// FDC Busy
		sr_d3b = 0x08,					// Drive3 Seek
		sr_d2b = 0x04,					// Drive2 Seek
		sr_d1b = 0x02,					// Drive1 Seek
		sr_d0b = 0x01					// Drive0 Seek
	};

	// コマンド定義
	enum fdccmd {
		read_data,						// READ DATA
		read_del_data,					// READ DELETED DATA
		read_id,						// READ ID
		write_id,						// WRITE ID
		write_data,						// WRITE DATA
		write_del_data,					// WRITE DELETED DATA
		read_diag,						// READ DIAGNOSTIC
		scan_eq,						// SCAN EQUAL
		scan_lo_eq,						// SCAN LOW OR EQUAL
		scan_hi_eq, 					// SCAN HIGH OR EQUAL
		seek,							// SEEK
		recalibrate,					// RECALIBRATE
		sense_int_stat,					// SENSE INTERRUPT STATUS
		sense_dev_stat,					// SENSE DEVICE STATUS
		specify,						// SPECIFY
		set_stdby,						// SET STANDBY
		reset_stdby,					// RESET STANDBY
		fdc_reset,						// SOFTWARE RESET
		invalid,						// INVALID
		no_cmd							// (NO COMMAND)
	};

	// 内部ワーク定義
	typedef struct {
		fdcphase phase;					// フェーズ
		fdccmd cmd;						// コマンド

		int in_len;						// 入力レングス
		int in_cnt;						// 入力カウント
		DWORD in_pkt[0x10];				// 入力パケット
		int out_len;					// 出力レングス
		int out_cnt;					// 出力カウント
		DWORD out_pkt[0x10];			// 出力パケット

		DWORD dcr;						// ドライブコントロールレジスタ
		DWORD dsr;						// ドライブセレクトレジスタ
		DWORD sr;						// ステータスレジスタ
		DWORD dr;						// データレジスタ
		DWORD st[4];					// ST0-ST3

		DWORD srt;						// SRT
		DWORD hut;						// HUT
		DWORD hlt;						// HLT
		DWORD hd;						// HD
		DWORD us;						// US
		DWORD cyl[4];					// 内部トラック
		DWORD chrn[4];					// 要求されたC,H,R,N

		DWORD eot;						// EOT
		DWORD gsl;						// GSL
		DWORD dtl;						// DTL
		DWORD sc; 						// SC
		DWORD gpl;						// GAP3
		DWORD d;						// フォーマットデータ
		DWORD err;						// エラーコード
		BOOL seek;						// シーク系割り込み要求
		BOOL ndm;						// Non-DMAモード
		BOOL mfm;						// MFMモード
		BOOL mt;						// マルチトラック
		BOOL sk;						// Skip DDAM
		BOOL tc;						// TC
		BOOL load;						// ヘッドロード

		int offset;						// バッファオフセット
		int len;						// 残りレングス
		BYTE buffer[0x4000];			// データバッファ

		BOOL fast;						// 高速モード
		BOOL dual;						// デュアルドライブ
	} fdc_t;

public:
	// 基本ファンクション
	FDC(VM *p);
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
	BOOL FASTCALL Callback(Event *ev);
										// イベントコールバック
	const fdc_t* FASTCALL GetWork() const;
										// 内部ワークアドレス取得
	void FASTCALL CompleteSeek(int drive, BOOL result);
										// シーク完了
	void FASTCALL SetTC();
										// TCアサート

private:
	void FASTCALL Idle();
										// アイドルフェーズ
	void FASTCALL Command(DWORD data);
										// コマンドフェーズ
	void FASTCALL CommandRW(fdccmd cmd, DWORD data);
										// コマンドフェーズ(R/W系)
	void FASTCALL Execute();
										// 実行フェーズ
	void FASTCALL ReadID();
										// 実行フェーズ(Read ID)
	void FASTCALL ExecuteRW();
										// 実行フェーズ(R/W系)
	BYTE FASTCALL Read();
										// 実行フェーズ(Read)
	void FASTCALL Write(DWORD data);
										// 実行フェーズ(Write)
	void FASTCALL Compare(DWORD data);
										// 実行フェーズ(Compare)
	void FASTCALL Result();
										// リザルトフェーズ
	void FASTCALL ResultRW();
										// リザルトフェーズ(R/W系)
	void FASTCALL Interrupt(BOOL flag);
										// 割り込み
	void FASTCALL SoftReset();
										// ソフトウェアリセット
	void FASTCALL MakeST3();
										// ST3作成
	BOOL FASTCALL ReadData();
										// READ (DELETED) DATAコマンド
	BOOL FASTCALL WriteData();
										// WRITE (DELETED) DATAコマンド
	BOOL FASTCALL ReadDiag();
										// READ DIAGNOSTICコマンド
	BOOL FASTCALL WriteID();
										// WRITE IDコマンド
	BOOL FASTCALL Scan();
										// SCAN系コマンド
	void FASTCALL EventRW();
										// イベント処理(R/W)
	void FASTCALL EventErr(DWORD hus);
										// イベント処理(エラー)
	void FASTCALL WriteBack();
										// 書き込み完了
	BOOL FASTCALL NextSector();
										// マルチセクタ処理
	IOSC *iosc;
										// IOSC
	DMAC *dmac;
										// DMAC
	FDD *fdd;
										// FDD
	Event event;
										// イベント
	fdc_t fdc;
										// FDC内部データ
};

#endif	// fdc_h
