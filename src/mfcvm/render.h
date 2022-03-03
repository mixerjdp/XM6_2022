//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001,2002 ＰＩ．(ytanaka@ipc-tokai.or.jp)
//	[ レンダラ ]
//
//---------------------------------------------------------------------------

#if !defined(render_h)
#define render_h

#include "device.h"
#include "vc.h"

//===========================================================================
//
//	レンダラ
//
//===========================================================================
class Render : public Device
{
public:
	// 内部データ定義
	typedef struct {
		// 全体制御
		BOOL act;						// 合成しているか
		BOOL enable;					// 合成許可
		int count;						// スケジューラ連携カウンタ
		BOOL ready;						// 描画準備できているか
		int first;						// 未処理ラスタ
		int last;						// 表示終了ラスタ

		// CRTC
		BOOL crtc;						// CRTC変更フラグ
		int width;						// X方向ドット数(256～)
		int h_mul;						// X方向倍率(1,2)
		int height;						// Y方向ドット数(256～)
		int v_mul;						// Y方向倍率(0,1,2)
		BOOL lowres;					// 15kHzフラグ

		// VC
		BOOL vc;						// VC変更フラグ

		// 合成
		BOOL mix[1024];					// 合成フラグ(ライン)
		DWORD *mixbuf;					// 合成バッファ
		DWORD *mixptr[8];				// 合成ポインタ
		DWORD mixshift[8];				// 合成ポインタのYシフト
		DWORD *mixx[8];					// 合成ポインタのXスクロールポインタ
		DWORD *mixy[8];					// 合成ポインタのYスクロールポインタ
		DWORD mixand[8];				// 合成ポインタのスクロールAND値
		int mixmap[3];					// 合成マップ
		int mixtype;					// 合成タイプ
		int mixpage;					// 合成グラフィックページ数
		int mixwidth;					// 合成バッファ幅
		int mixheight;					// 合成バッファ高さ
		int mixlen;						// 合成時処理長さ(x方向)

		// 描画
		BOOL draw[1024];				// 描画フラグ(ライン)
		BOOL *drawflag;					// 描画フラグ(16dot)

		// コントラスト
		BOOL contrast;					// コントラスト変更フラグ
		int contlevel;					// コントラスト

		// パレット
		BOOL palette;					// パレット変更フラグ
		BOOL palmod[0x200];				// パレット変更フラグ
		DWORD *palbuf;					// パレットバッファ
		DWORD *palptr;					// パレットポインタ
		const WORD *palvc;				// パレットVCポインタ
		DWORD paldata[0x200];			// パレットデータ
		BYTE pal64k[0x200];				// パレットデータ変形

		// テキストVRAM
		BOOL texten;					// テキスト表示フラグ
		BOOL textpal[1024];				// テキストパレットフラグ
		BOOL textmod[1024];				// テキスト更新フラグ(ライン)
		BOOL *textflag;					// テキスト更新フラグ(32dot)
		BYTE *textbuf;					// テキストバッファ(パレット前)
		DWORD *textout;					// テキストバッファ(パレット後)
		const BYTE *texttv;				// テキストTVRAMポインタ
		DWORD textx;					// テキストスクロールX
		DWORD texty;					// テキストスクロールY

		// グラフィックVRAM
		int grptype;					// グラフィックタイプ(0～4)
		BOOL grpen[4];					// グラフィックブロック表示フラグ
		BOOL grppal[2048];				// グラフィックパレットフラグ
		BOOL grpmod[2048];				// グラフィック更新フラグ(ライン)
		BOOL *grpflag;					// グラフィック更新フラグ(16dot)
		DWORD *grpbuf[4];				// グラフィックブロックバッファ
		const BYTE* grpgv;				// グラフィックGVRAMポインタ
		DWORD grpx[4];					// グラフィックブロックスクロールX
		DWORD grpy[4];					// グラフィックブロックスクロールY

		// PCG
		BOOL pcgready[256 * 16];		// PCG準備OKフラグ
		DWORD pcguse[256 * 16];			// PCG使用中カウント
		DWORD pcgpal[16];				// PCGパレット使用カウント
		DWORD *pcgbuf;					// PCGバッファ
		const BYTE* sprmem;				// スプライトメモリ

		// スプライト
		DWORD **spptr;					// スプライトポインタバッファ
		DWORD spreg[0x200];				// スプライトレジスタ保存
		BOOL spuse[128];				// スプライト使用中フラグ

		// BG
		DWORD bgreg[2][64 * 64];		// BGレジスタ＋変更フラグ($10000)
		BOOL bgall[2][64];				// BG変更フラグ(ブロック単位)
		BOOL bgdisp[2];					// BG表示フラグ
		BOOL bgarea[2];					// BG表示エリア
		BOOL bgsize;					// BG表示サイズ(16dot=TRUE)
		DWORD **bgptr[2];				// BGポインタ+データ
		BOOL bgmod[2][1024];			// BG更新フラグ
		DWORD bgx[2];					// BGスクロール(X)
		DWORD bgy[2];					// BGスクロール(Y)

		// BG/スプライト合成
		BOOL bgspflag;					// BG/スプライト表示フラグ
		BOOL bgspdisp;					// BG/スプライトCPU/Videoフラグ
		BOOL bgspmod[512];				// BG/スプライト更新フラグ
		DWORD *bgspbuf;					// BG/スプライトバッファ
		DWORD zero;						// スクロールダミー(0)
	} render_t;

public:
	// 基本ファンクション
	Render(VM *p);
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

	// 外部API(コントロール)
	void FASTCALL EnableAct(BOOL enable){ render.enable = enable; }
										// 合成許可
	BOOL FASTCALL IsActive() const		{ return render.act; }
										// アクティブか
	BOOL FASTCALL IsReady() const		{ return (BOOL)(render.count > 0); }
										// 描画レディ状況取得
	void FASTCALL Complete()			{ render.count = 0; }
										// 描画完了
	void FASTCALL StartFrame();
										// フレーム開始(V-DISP)
	void FASTCALL EndFrame();
										// フレーム終了(V-BLANK)
	void FASTCALL HSync(int raster)		{ render.last = raster; if (render.act) Process(); }
										// 水平同期(rasterまで終わり)
	void FASTCALL SetMixBuf(DWORD *buf, int width, int height);
										// 合成バッファ指定
	render_t* FASTCALL GetWorkAddr() 	{ return &render; }
										// ワークアドレス取得

	// 外部API(画面)
	void FASTCALL SetCRTC();
										// CRTCセット
	void FASTCALL SetVC();
										// VCセット
	void FASTCALL SetContrast(int cont);
										// コントラスト設定
	int FASTCALL GetContrast() const;
										// コントラスト取得
	void FASTCALL SetPalette(int index);
										// パレット設定
	const DWORD* FASTCALL GetPalette() const;
										// パレットバッファ取得
	void FASTCALL TextMem(DWORD addr);
										// テキストVRAM変更
	void FASTCALL TextScrl(DWORD x, DWORD y);
										// テキストスクロール変更
	void FASTCALL TextCopy(DWORD src, DWORD dst, DWORD plane);
										// ラスタコピー
	void FASTCALL GrpMem(DWORD addr, DWORD block);
										// グラフィックVRAM変更
	void FASTCALL GrpAll(DWORD line, DWORD block);
										// グラフィックVRAM変更
	void FASTCALL GrpScrl(int block, DWORD x, DWORD y);
										// グラフィックスクロール変更
	void FASTCALL SpriteReg(DWORD addr, DWORD data);
										// スプライトレジスタ変更
	void FASTCALL BGScrl(int page, DWORD x, DWORD y);
										// BGスクロール変更
	void FASTCALL BGCtrl(int index, BOOL flag);
										// BGコントロール変更
	void FASTCALL BGMem(DWORD addr, WORD data);
										// BG変更
	void FASTCALL PCGMem(DWORD addr);
										// PCG変更

	const DWORD* FASTCALL GetTextBuf() const;
										// テキストバッファ取得
	const DWORD* FASTCALL GetGrpBuf(int index) const;
										// グラフィックバッファ取得
	const DWORD* FASTCALL GetPCGBuf() const;
										// PCGバッファ取得
	const DWORD* FASTCALL GetBGSpBuf() const;
										// BG/スプライトバッファ取得
	const DWORD* FASTCALL GetMixBuf() const;
										// 合成バッファ取得

private:
	void FASTCALL Process();
										// レンダリング
	void FASTCALL Video();
										// VC処理
	void FASTCALL SetupGrp(int first);
										// グラフィックセットアップ
	void FASTCALL Contrast();
										// コントラスト処理
	void FASTCALL Palette();
										// パレット処理
	void FASTCALL MakePalette();
										// パレット作成
	DWORD FASTCALL ConvPalette(int color, int ratio);
										// 色変換
	void FASTCALL Text(int raster);
										// テキスト
	void FASTCALL Grp(int block, int raster);
										// グラフィック
	void FASTCALL SpriteReset();
										// スプライトリセット
	void FASTCALL BGSprite(int raster);
										// BG/スプライト
	void FASTCALL BG(int page, int raster, DWORD *buf);
										// BG
	void FASTCALL BGBlock(int page, int y);
										// BG(横ブロック)
	void FASTCALL Mix(int offset);
										// 合成
	void FASTCALL MixGrp(int y, DWORD *buf);
										// 合成(グラフィック)
	CRTC *crtc;
										// CRTC
	VC *vc;
										// VC
	Sprite *sprite;
										// スプライト
	render_t render;
										// 内部データ
	BOOL cmov;
										// CMOVキャッシュ
};

#endif	// render_h
