//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 ＰＩ．(ytanaka@ipc-tokai.or.jp)
//	[ テキストVRAM ]
//
//---------------------------------------------------------------------------

#if !defined(tvram_h)
#define tvram_h

#include "device.h"

//===========================================================================
//
//	テキストVRAMハンドラ
//
//===========================================================================
class TVRAMHandler
{
public:
	TVRAMHandler(Render *rend, BYTE *mem);
										// コンストラクタ
	virtual void FASTCALL WriteByte(DWORD addr, DWORD data) = 0;
										// バイト書き込み
	virtual void FASTCALL WriteWord(DWORD addr, DWORD data) = 0;
										// ワード書き込み

	// TVRAMワークのコピー
	DWORD multi;
										// 同時アクセス(bit0-bit3)
	DWORD mask;
										// アクセスマスク(1で変更なし)
	DWORD rev;
										// アクセスマスク反転
	DWORD maskh;
										// アクセスマスク上位バイト
	DWORD revh;
										// アクセスマスク上位反転

protected:
	Render *render;
										// レンダラ
	BYTE *tvram;
										// テキストVRAM
};

//===========================================================================
//
//	テキストVRAMハンドラ(通常)
//
//===========================================================================
class TVRAMNormal : public TVRAMHandler
{
public:
	TVRAMNormal(Render *rend, BYTE *mem);
										// コンストラクタ
	void FASTCALL WriteByte(DWORD addr, DWORD data);
										// バイト書き込み
	void FASTCALL WriteWord(DWORD addr, DWORD data);
										// ワード書き込み
};

//===========================================================================
//
//	テキストVRAMハンドラ(マスク)
//
//===========================================================================
class TVRAMMask : public TVRAMHandler
{
public:
	TVRAMMask(Render *rend, BYTE *mem);
										// コンストラクタ
	void FASTCALL WriteByte(DWORD addr, DWORD data);
										// バイト書き込み
	void FASTCALL WriteWord(DWORD addr, DWORD data);
										// ワード書き込み
};

//===========================================================================
//
//	テキストVRAMハンドラ(マルチ)
//
//===========================================================================
class TVRAMMulti : public TVRAMHandler
{
public:
	TVRAMMulti(Render *rend, BYTE *mem);
										// コンストラクタ
	void FASTCALL WriteByte(DWORD addr, DWORD data);
										// バイト書き込み
	void FASTCALL WriteWord(DWORD addr, DWORD data);
										// ワード書き込み
};

//===========================================================================
//
//	テキストVRAMハンドラ(マスク＋マルチ)
//
//===========================================================================
class TVRAMBoth : public TVRAMHandler
{
public:
	TVRAMBoth(Render *rend, BYTE *mem);
										// コンストラクタ
	void FASTCALL WriteByte(DWORD addr, DWORD data);
										// バイト書き込み
	void FASTCALL WriteWord(DWORD addr, DWORD data);
										// ワード書き込み
};

//===========================================================================
//
//	テキストVRAM
//
//===========================================================================
class TVRAM : public MemDevice
{
public:
	// 内部データ定義
	typedef struct {
		DWORD multi;					// 同時アクセス(bit0-bit3)
		DWORD mask;						// アクセスマスク(1で変更なし)
		DWORD rev;						// アクセスマスク反転
		DWORD maskh;					// アクセスマスク上位バイト
		DWORD revh;						// アクセスマスク上位反転
		DWORD src;						// ラスタコピー 元ラスタ
		DWORD dst;						// ラスタコピー 先ラスタ
		DWORD plane;					// ラスタコピー 対象プレーン
	} tvram_t;

public:
	// 基本ファンクション
	TVRAM(VM *p);
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
	const BYTE* FASTCALL GetTVRAM() const;
										// TVRAM取得
	void FASTCALL SetMulti(DWORD data);
										// 同時書き込み設定
	void FASTCALL SetMask(DWORD data);
										// アクセスマスク設定
	void FASTCALL SetCopyRaster(DWORD src, DWORD dst, DWORD plane);
										// コピーラスタ指定
	void FASTCALL RasterCopy();
										// ラスタコピー動作

private:
	void FASTCALL SelectHandler();
										// ハンドラ選択
	TVRAMNormal *normal;
										// ハンドラ(通常)
	TVRAMMask *mask;
										// ハンドラ(マスク)
	TVRAMMulti *multi;
										// ハンドラ(マルチ)
	TVRAMBoth *both;
										// ハンドラ(両方)
	TVRAMHandler *handler;
										// ハンドラ(現在選択中)
	Render *render;
										// レンダラ
	BYTE *tvram;
										// テキストVRAM (512KB)
	tvram_t tvdata;
										// 内部データ
	DWORD tvcount;
										// TVRAMアクセスカウント(version2.04以降)
};

#endif	// tvram_h
