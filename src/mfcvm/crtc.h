//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2003 ＰＩ．(ytanaka@ipc-tokai.or.jp)
//	[ CRTC(VICON) ]
//
//---------------------------------------------------------------------------

#if !defined(crtc_h)
#define crtc_h

#include "device.h"
#include "event.h"

//===========================================================================
//
//	CRTC
//
//===========================================================================
class CRTC : public MemDevice
{
public:
	// 内部データ定義
	typedef struct {
		BYTE reg[24 * 2];				// CRTCレジスタ
		BOOL hrl;						// HRL(システムポート)
		BOOL lowres;					// 15kHzモード
		BOOL textres;					// 768×512モード
		BOOL changed;					// 解像度変更フラグ

		int h_sync;						// 水平同期期間
		int h_pulse;					// 水平同期パルス幅
		int h_back;						// 水平バックポーチ
		int h_front;					// 水平フロントポーチ
		int h_dots;						// 水平ドット数
		int h_mul;						// 水平倍率
		int hd;							// 256,512,768,未定義

		int v_sync;						// 垂直同期期間(H単位)
		int v_pulse;					// 垂直同期パルス幅(H単位)
		int v_back;						// 垂直バックポーチ(H単位)
		int v_front;					// 垂直フロントポーチ(H単位)
		int v_dots;						// 垂直ドット数
		int v_mul;						// 垂直倍率(0:interlace)
		int vd;							// 256,512,未定義,未定義

		DWORD ns;						// nsカウンタ
		DWORD hus;						// husカウンタ
		DWORD v_synccnt;				// V-SYNCカウンタ
		DWORD v_blankcnt;				// V-BLANKカウンタ
		BOOL h_disp;					// 水平表示フラグ
		BOOL v_disp;					// V-DISPフラグ
		BOOL v_blank;					// V-BLANKフラグ
		DWORD v_count;					// V-DISPカウンタ
		int v_scan;						// スキャンライン

		// 以下いらない
		int h_synctime;					// 水平同期(hus)
		int h_disptime;					// 水平表示(hus)
		int v_cycletime;				// 垂直周期(hus)
		int v_blanktime;				// 垂直ブランク(hus)
		int v_synctime;					// 垂直同期(hus)
		int v_backtime;					// 垂直バックポーチ(hus)

		BOOL tmem;						// テキストVRAM非表示
		BOOL gmem;						// グラフィックVRAM非表示
		DWORD siz;						// グラフィックVRAM1024×1024モード
		DWORD col;						// グラフィックVRAM色モード

		DWORD text_scrlx;				// テキストスクロールX
		DWORD text_scrly;				// テキストスクロールY
		DWORD grp_scrlx[4];				// グラフィックスクロールX
		DWORD grp_scrly[4];				// グラフィックスクロールY

		int raster_count;				// ラスタカウンタ
		int raster_int;					// ラスタ割り込み位置
		BOOL raster_copy;				// ラスタコピーフラグ
		BOOL raster_exec;				// ラスタコピー実行フラグ
		DWORD fast_clr;					// グラフィック高速クリア
	} crtc_t;

public:
	// 基本ファンクション
	CRTC(VM *p);
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
	void FASTCALL WriteByte(DWORD addr, DWORD data);
										// バイト書き込み
	DWORD FASTCALL ReadOnly(DWORD addr) const;
										// 読み込みのみ

	// 外部API
	void FASTCALL GetCRTC(crtc_t *buffer) const;
										// 内部データ取得
	BOOL FASTCALL Callback(Event *ev);
										// イベントコールバック
	void FASTCALL SetHRL(BOOL h);
										// HRL設定
	BOOL FASTCALL GetHRL() const;
										// HRL取得
	void FASTCALL GetHVHz(DWORD *h, DWORD *v) const;
										// 表示周波数取得
	DWORD FASTCALL GetDispCount() const	{ return crtc.v_count; }
										// 表示カウンタ取得
	const crtc_t* FASTCALL GetWorkAddr() const { return &crtc; }
										// ワークアドレス取得

private:
	void FASTCALL ReCalc();
										// 再計算
	void FASTCALL HSync();
										// H-SYNC開始
	void FASTCALL HDisp();
										// H-DISP開始
	void FASTCALL VSync();
										// V-SYNC開始
	void FASTCALL VBlank();
										// V-BLANK開始
	int FASTCALL Ns2Hus(int ns)			{ return ns / 500; }
										// ns→0.5us換算
	int FASTCALL Hus2Ns(int hus)		{ return hus * 500; }
										// 0.5us→ns換算
	void FASTCALL CheckRaster();
										// ラスタ割り込みチェック
	void FASTCALL TextVRAM();
										// テキストVRAM効果
	int FASTCALL Get8DotClock() const;
										// 8ドットクロックを得る
	static const int DotClockTable[16];
										// 8ドットクロックテーブル
	static const BYTE ResetTable[26];
										// RESETレジスタテーブル
	crtc_t crtc;
										// CRTC内部データ
	Event event;
										// イベント
	TVRAM *tvram;
										// テキストVRAM
	GVRAM *gvram;
										// グラフィックVRAM
	Sprite *sprite;
										// スプライトコントローラ
	MFP *mfp;
										// MFP
	Render *render;
										// レンダラ
	Printer *printer;
										// プリンタ
};

#endif	// crtc_h
