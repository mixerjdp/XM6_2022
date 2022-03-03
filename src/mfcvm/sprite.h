//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2004 ＰＩ．(ytanaka@ipc-tokai.or.jp)
//	[ スプライト(CYNTHIA) ]
//
//---------------------------------------------------------------------------

#if !defined(sprite_h)
#define sprite_h

#include "device.h"

//===========================================================================
//
//	スプライト
//
//===========================================================================
class Sprite : public MemDevice
{
public:
	// 内部データ定義
	typedef struct {
		BOOL connect;					// アクセス可能フラグ
		BOOL disp;						// 表示(ウェイト)フラグ
		BYTE *mem;						// スプライトメモリ
		BYTE *pcg;						// スプライトPCGエリア

		BOOL bg_on[2];					// BG表示ON
		DWORD bg_area[2];				// BGデータエリア
		DWORD bg_scrlx[2];				// BGスクロールX
		DWORD bg_scrly[2];				// BGスクロールY
		BOOL bg_size;					// BGサイズ

		DWORD h_total;					// 水平トータル期間
		DWORD h_disp;					// 水平表示期間
		DWORD v_disp;					// 垂直表示期間
		BOOL lowres;					// 15kHzモード
		DWORD h_res;					// 水平解像度
		DWORD v_res;					// 垂直解像度
	} sprite_t;

public:
	// 基本ファンクション
	Sprite(VM *p);
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
	void FASTCALL Connect(BOOL con)		{ spr.connect = con; }
										// 接続
	BOOL FASTCALL IsConnect() const		{ return spr.connect; }
										// 接続状況取得
	BOOL FASTCALL IsDisplay() const		{ return spr.disp; }
										// 表示状況取得
	void FASTCALL GetSprite(sprite_t *buffer) const;
										// 内部データ取得
	const BYTE* FASTCALL GetMem() const;
										// メモリエリア取得
	const BYTE* FASTCALL GetPCG() const;
										// PCGエリア取得 

private:
	void FASTCALL Control(DWORD addr, DWORD ctrl);
										// コントロール
	sprite_t spr;
										// 内部データ
	Render *render;
										// レンダラ
	BYTE *sprite;
										// スプライトRAM(64KB)
};

#endif	// sprite_h
