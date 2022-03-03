//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2004 ＰＩ．(ytanaka@ipc-tokai.or.jp)
//	[ レンダラ アセンブラサブ ]
//
//---------------------------------------------------------------------------

#if !defined (rend_asm_h)
#define rend_asm_h

#if defined(__cplusplus)
extern "C" {
#endif	//__cplusplus

//---------------------------------------------------------------------------
//
//	プロトタイプ宣言
//
//---------------------------------------------------------------------------
void RendTextMem(const BYTE *tvrm, BOOL *flag, BYTE *buf);
										// テキストレンダリング(水平垂直変換)
void RendTextPal(const BYTE *buf, DWORD *out, BOOL *flag, const DWORD *pal);
										// テキストレンダリング(パレット)
void RendTextAll(const BYTE *buf, DWORD *out, const DWORD *pal);
										// テキストレンダリング(パレット全て)
void RendTextCopy(const BYTE *src, const BYTE *dst, DWORD plane, BOOL *textmem, BOOL *textflag);
										// テキストラスタコピー

int Rend1024A(const BYTE *gvrm, DWORD *buf, const DWORD *pal);
										// グラフィック1024レンダリング(ページ0,1-All)
int Rend1024B(const BYTE *gvrm, DWORD *buf, const DWORD *pal);
										// グラフィック1024レンダリング(ページ2,3-All)
void Rend1024C(const BYTE *gvrm, DWORD *buf, BOOL *flag, const DWORD *pal);
										// グラフィック1024レンダリング(ページ0)
void Rend1024D(const BYTE *gvrm, DWORD *buf, BOOL *flag, const DWORD *pal);
										// グラフィック1024レンダリング(ページ1)
void Rend1024E(const BYTE *gvrm, DWORD *buf, BOOL *flag, const DWORD *pal);
										// グラフィック1024レンダリング(ページ2)
void Rend1024F(const BYTE *gvrm, DWORD *buf, BOOL *flag, const DWORD *pal);
										// グラフィック1024レンダリング(ページ3)
int Rend16A(const BYTE *gvrm, DWORD *buf, const DWORD *pal);
										// グラフィック16レンダリング(ページ0-All)
int Rend16B(const BYTE *gvrm, DWORD *buf, BOOL *flag, const DWORD *pal);
										// グラフィック16レンダリング(ページ0)
int Rend16C(const BYTE *gvrm, DWORD *buf, const DWORD *pal);
										// グラフィック16レンダリング(ページ1-All)
int Rend16D(const BYTE *gvrm, DWORD *buf, BOOL *flag, const DWORD *pal);
										// グラフィック16レンダリング(ページ1)
int Rend16E(const BYTE *gvrm, DWORD *buf, const DWORD *pal);
										// グラフィック16レンダリング(ページ2-All)
int Rend16F(const BYTE *gvrm, DWORD *buf, BOOL *flag, const DWORD *pal);
										// グラフィック16レンダリング(ページ2)
int Rend16G(const BYTE *gvrm, DWORD *buf, const DWORD *pal);
										// グラフィック16レンダリング(ページ3-All)
int Rend16H(const BYTE *gvrm, DWORD *buf, BOOL *flag, const DWORD *pal);
										// グラフィック16レンダリング(ページ3)
void Rend256A(const BYTE *gvrm, DWORD *buf, BOOL *flag, const DWORD *pal);
										// グラフィック256レンダリング(ページ0)
void Rend256B(const BYTE *gvrm, DWORD *buf, BOOL *flag, const DWORD *pal);
										// グラフィック256レンダリング(ページ1)
int Rend256C(const BYTE *gvrm, DWORD *buf, const DWORD *pal);
										// グラフィック256レンダリング(ページ0-All)
int Rend256D(const BYTE *gvrm, DWORD *buf, const DWORD *pal);
										// グラフィック256レンダリング(ページ1-All)
void Rend64KA(const BYTE *gvrm, DWORD *buf, BOOL *flag, BYTE *plt, DWORD *pal);
										// グラフィック64Kレンダリング
int Rend64KB(const BYTE *gvrm, DWORD *buf, BYTE *plt, DWORD *pal);
										// グラフィック64Kレンダリング(All)

void RendClrSprite(DWORD *buf, DWORD color, int len);
										// スプライトバッファクリア
void RendSprite(const DWORD *line, DWORD *buf, DWORD x, DWORD flag);
										// スプライトレンダリング(単体)
void RendSpriteC(const DWORD *line, DWORD *buf, DWORD x, DWORD flag);
										// スプライトレンダリング(単体、CMOV)
void RendPCGNew(DWORD index, const BYTE *mem, DWORD *buf, DWORD *pal);
										// PCGレンダリング(NewVer)
void RendBG8(DWORD **ptr, DWORD *buf, int x, int len, BOOL *ready, const BYTE *mem,
			DWORD *pcgbuf, DWORD *pal);	// BG(8x8、割り切れる)
void RendBG8C(DWORD **ptr, DWORD *buf, int x, int len, BOOL *ready, const BYTE *mem,
			DWORD *pcgbuf, DWORD *pal);	// BG(8x8、割り切れる、CMOV)
void RendBG8P(DWORD **ptr, DWORD *buf, int offset, int length, BOOL *ready, const BYTE *mem,
			DWORD *pcgbuf, DWORD *pal);	// BG(8x8、部分のみ)
void RendBG16(DWORD **ptr, DWORD *buf, int x, int len, BOOL *ready, const BYTE *mem,
			DWORD *pcgbuf, DWORD *pal);	// BG(16x16、割り切れる)
void RendBG16C(DWORD **ptr, DWORD *buf, int x, int len, BOOL *ready, const BYTE *mem,
			DWORD *pcgbuf, DWORD *pal);	// BG(16x16、割り切れる、CMOV)
void RendBG16P(DWORD **ptr, DWORD *buf, int offset, int length, BOOL *ready, const BYTE *mem,
			DWORD *pcgbuf, DWORD *pal);	// BG(16x16、部分のみ)

void RendMix00(DWORD *buf, BOOL *flag, int len);
										// 合成(0面)
void RendMix01(DWORD *buf, const DWORD *src, BOOL *flag, int len);
										// 合成(1面)
void RendMix02(DWORD *buf, const DWORD *f, const DWORD *s, BOOL *flag, int len);
										// 合成(2面、カラー0重ね合わせ)
void RendMix02C(DWORD *buf, const DWORD *f, const DWORD *s, BOOL *flag, int len);
										// 合成(2面、カラー0重ね合わせ、CMOV)
void RendMix03(DWORD *buf, const DWORD *f, const DWORD *s, BOOL *flag, int len);
										// 合成(2面、通常重ね合わせ)
void RendMix03C(DWORD *buf, const DWORD *f, const DWORD *s, BOOL *flag, int len);
										// 合成(2面、通常重ね合わせ、CMOV)
void RendMix04(DWORD *buf, const DWORD *f, const DWORD *s, DWORD *t, BOOL *flag, int len);
										// 合成(3面、通常重ね合わせ、CMOV)
void RendMix04C(DWORD *buf, const DWORD *f, const DWORD *s, DWORD *t, BOOL *flag, int len);
										// 合成(3面、通常重ね合わせ、CMOV)
void RendGrp02(DWORD *buf, const DWORD *f, const DWORD *s, int len);
										// グラフィック合成(2面)
void RendGrp02C(DWORD *buf, const DWORD *f, const DWORD *s, int len);
										// グラフィック合成(2面、CMOV)
void RendGrp03(DWORD *buf, const DWORD *f, const DWORD *s, const DWORD *t, int len);
										// グラフィック合成(3面)
void RendGrp03C(DWORD *buf, const DWORD *f, const DWORD *s, const DWORD *t, int len);
										// グラフィック合成(3面、CMOV)
void RendGrp04(DWORD *buf, DWORD *f, DWORD *s, DWORD *t, DWORD *e, int len);
										// グラフィック合成(4面)

#if defined(__cplusplus)
}
#endif	//__cplusplus

#endif	// rend_asm_h
