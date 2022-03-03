//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 ＰＩ．(ytanaka@ipc-tokai.or.jp)
//	[ グラフィックVRAM ]
//
//---------------------------------------------------------------------------

#if !defined(gvram_h)
#define gvram_h

#include "device.h"
#include "crtc.h"

//===========================================================================
//
//	グラフィックVRAMハンドラ
//
//===========================================================================
class GVRAMHandler
{
public:
	GVRAMHandler(Render *rend, BYTE *mem, CPU *p);
										// コンストラクタ
	virtual DWORD FASTCALL ReadByte(DWORD addr) = 0;
										// バイト読み込み
	virtual DWORD FASTCALL ReadWord(DWORD addr) = 0;
										// ワード読み込み
	virtual void FASTCALL WriteByte(DWORD addr, DWORD data) = 0;
										// バイト書き込み
	virtual void FASTCALL WriteWord(DWORD addr, DWORD data) = 0;
										// ワード書き込み
	virtual DWORD FASTCALL ReadOnly(DWORD addr) const = 0;
										// 読み込みのみ

protected:
	Render *render;
										// レンダラ
	BYTE *gvram;
										// グラフィックVRAM
	CPU *cpu;
										// CPU
};

//===========================================================================
//
//	グラフィックVRAMハンドラ(1024)
//
//===========================================================================
class GVRAM1024 : public GVRAMHandler
{
public:
	GVRAM1024(Render *render, BYTE *gvram, CPU *p);
										// コンストラクタ
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
};

//===========================================================================
//
//	グラフィックVRAMハンドラ(16色)
//
//===========================================================================
class GVRAM16 : public GVRAMHandler
{
public:
	GVRAM16(Render *render, BYTE *gvram, CPU *p);
										// コンストラクタ
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
};

//===========================================================================
//
//	グラフィックVRAMハンドラ(256色)
//
//===========================================================================
class GVRAM256 : public GVRAMHandler
{
public:
	GVRAM256(Render *render, BYTE *gvram, CPU *p);
										// コンストラクタ
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
};

//===========================================================================
//
//	グラフィックVRAMハンドラ(無効)
//
//===========================================================================
class GVRAMNDef : public GVRAMHandler
{
public:
	GVRAMNDef(Render *render, BYTE *gvram, CPU *p);
										// コンストラクタ
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
};

//===========================================================================
//
//	グラフィックVRAMハンドラ(65536色)
//
//===========================================================================
class GVRAM64K : public GVRAMHandler
{
public:
	GVRAM64K(Render *render, BYTE *gvram, CPU *p);
										// コンストラクタ
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
};

//===========================================================================
//
//	グラフィックVRAM
//
//===========================================================================
class GVRAM : public MemDevice
{
public:
	// 内部ワーク定義
	typedef struct {
		BOOL mem;						// 512KB単純メモリフラグ
		DWORD siz;						// 1024×1024フラグ
		DWORD col;						// 16, 256, 未定義, 65536
		int type;						// ハンドラタイプ(0～4)
		DWORD mask[4];					// 高速クリア マスク
		BOOL plane[4];					// 高速クリア プレーン
	} gvram_t;

public:
	// 基本ファンクション
	GVRAM(VM *p);
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
	void FASTCALL SetType(DWORD type);
										// GVRAMタイプ設定
	void FASTCALL FastSet(DWORD mask);
										// 高速クリア設定
	void FASTCALL FastClr(const CRTC::crtc_t *p);
										// 高速クリア
	const BYTE* FASTCALL GetGVRAM() const;
										// GVRAM取得

private:
	void FASTCALL FastClr768(const CRTC::crtc_t *p);
										// 高速クリア 1024x1024 512/768
	void FASTCALL FastClr256(const CRTC::crtc_t *p);
										// 高速クリア 1024x1024 256
	void FASTCALL FastClr512(const CRTC::crtc_t *p);
										// 高速クリア 512x512
	Render *render;
										// レンダラ
	BYTE *gvram;
										// グラフィックVRAM
	GVRAMHandler *handler;
										// メモリハンドラ(カレント)
	GVRAM1024 *hand1024;
										// メモリハンドラ(1024)
	GVRAM16 *hand16;
										// メモリハンドラ(16色)
	GVRAM256 *hand256;
										// メモリハンドラ(256色)
	GVRAMNDef *handNDef;
										// メモリハンドラ(無効)
	GVRAM64K *hand64K;
										// メモリハンドラ(64K色)
	gvram_t gvdata;
										// 内部ワーク
	DWORD gvcount;
										// GVRAMアクセスカウント(version2.04以降)
};

#endif	// gvram_h
