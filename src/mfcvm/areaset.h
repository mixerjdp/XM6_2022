//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2003 ＰＩ．(ytanaka@ipc-tokai.or.jp)
//	[ エリアセット ]
//
//---------------------------------------------------------------------------

#if !defined(areaset_h)
#define areaset_h

#include "device.h"

//===========================================================================
//
//	エリアセット
//
//===========================================================================
class AreaSet : public MemDevice
{
public:
	// 基本ファンクション
	AreaSet(VM *p);
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
	DWORD FASTCALL GetArea() const;
										// 設定値取得

private:
	Memory *memory;
										// メモリ
	DWORD area;
										// エリアセットレジスタ
};

#endif	// areaset_h
