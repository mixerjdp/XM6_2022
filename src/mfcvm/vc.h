//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2005 ＰＩ．(ytanaka@ipc-tokai.or.jp)
//	[ ビデオコントローラ(CATHY & VIPS) ]
//
//---------------------------------------------------------------------------

#if !defined(vc_h)
#define vc_h

#include "device.h"

//===========================================================================
//
//	ビデオコントローラ
//
//===========================================================================
class VC : public MemDevice
{
public:
	// 内部データ定義
	typedef struct {
		DWORD vr1h;						// VR1(H)バックアップ
		DWORD vr1l;						// VR1(H)バックアップ
		DWORD vr2h;						// VR2(H)バックアップ
		DWORD vr2l;						// VR2(H)バックアップ
		BOOL siz;						// 実画面サイズ
		DWORD col;						// 色モード
		DWORD sp;						// スプライトプライオリティ
		DWORD tx;						// テキストプライオリティ
		DWORD gr;						// グラフィックプライオリティ(1024)
		DWORD gp[4];					// グラフィックプライオリティ(512)
		BOOL ys;						// Ys信号
		BOOL ah;						// テキストパレット半透明
		BOOL vht;						// 外部ビデオ半透明
		BOOL exon;						// 特殊プライオリティ・半透明
		BOOL hp;						// 半透明
		BOOL bp;						// 最下位ビット半透明フラグ
		BOOL gg;						// グラフィック半透明
		BOOL gt;						// テキスト半透明
		BOOL bcon;						// シャープ予約
		BOOL son;						// スプライトON
		BOOL ton;						// テキストON
		BOOL gon;						// グラフィックON(実画面1024時)
		BOOL gs[4];						// グラフィックON(実画面512時)
	} vc_t;

public:
	// 基本ファンクション
	VC(VM *p);
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
	void FASTCALL GetVC(vc_t *buffer);
										// 内部データ取得
	const BYTE* FASTCALL GetPalette() const	{ return palette; }
										// パレットRAM取得
	const vc_t* FASTCALL GetWorkAddr() const{ return &vc; }
										// ワークアドレス取得

private:
	// レジスタアクセス
	void FASTCALL SetVR0L(DWORD data);
										// レジスタ0(L)設定
	DWORD FASTCALL GetVR0() const;
										// レジスタ0取得
	void FASTCALL SetVR1H(DWORD data);
										// レジスタ1(H)設定
	void FASTCALL SetVR1L(DWORD data);
										// レジスタ1(L)設定
	DWORD FASTCALL GetVR1() const;
										// レジスタ1取得
	void FASTCALL SetVR2H(DWORD data);
										// レジスタ2(H)設定
	void FASTCALL SetVR2L(DWORD data);
										// レジスタ2(L)設定
	DWORD FASTCALL GetVR2() const;
										// レジスタ2取得

	// データ
	Render *render;
										// レンダラ
	vc_t vc;
										// 内部データ
	BYTE palette[0x400];
										// パレットRAM
};

#endif	// vc_h
