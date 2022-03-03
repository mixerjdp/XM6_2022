//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 ＰＩ．(ytanaka@ipc-tokai.or.jp)
//	[ スタティックRAM ]
//
//---------------------------------------------------------------------------

#if !defined(sram_h)
#define sram_h

#include "device.h"
#include "filepath.h"

//===========================================================================
//
//	スタティックRAM
//
//===========================================================================
class SRAM : public MemDevice
{
public:
	// 基本ファンクション
	SRAM(VM *p);
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
	const BYTE* FASTCALL GetSRAM() const;
										// SRAMエリア取得
	int FASTCALL GetSize() const;
										// SRAMサイズ取得
	void FASTCALL WriteEnable(BOOL enable);
										// 書き込み許可
	void FASTCALL SetMemSw(DWORD offset, DWORD data);
										// メモリスイッチセット
	DWORD FASTCALL GetMemSw(DWORD offset) const;
										// メモリスイッチ取得
	void FASTCALL UpdateBoot();
										// 起動カウンタ更新

private:
	Filepath sram_path;
										// SRAMファイルパス
	int sram_size;
										// SRAMサイズ(16,32,48,64)
	BYTE sram[0x10000];
										// SRAM (64KB)
	BOOL write_en;
										// 書き込み許可フラグ
	BOOL mem_sync;
										// メインRAMサイズ同期フラグ
	BOOL changed;
										// 変更フラグ
};

#endif	// sram_h
