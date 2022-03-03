//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2004 ＰＩ．(ytanaka@ipc-tokai.or.jp)
//	[ I/Oコントローラ(IOSC-2) ]
//
//---------------------------------------------------------------------------

#if !defined(iosc_h)
#define iosc_h

#include "device.h"

//===========================================================================
//
//	I/Oコントローラ
//
//===========================================================================
class IOSC : public MemDevice
{
public:
	// 内部データ定義
	typedef struct {
		BOOL prt_int;					// プリンタ割り込み要求
		BOOL prt_en;					// プリンタ割り込み許可
		BOOL fdd_int;					// FDD割り込み要求
		BOOL fdd_en;					// FDD割り込み許可
		BOOL fdc_int;					// FDC割り込み要求
		BOOL fdc_en;					// FDC割り込み許可
		BOOL hdc_int;					// HDD割り込み要求
		BOOL hdc_en;					// HDD割り込み許可
		DWORD vbase;					// ベクタベース
		int vector;						// 要求中の割り込みベクタ
	} iosc_t;

public:
	// 基本ファンクション
	IOSC(VM *p);
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
	void FASTCALL GetIOSC(iosc_t *buffer) const;
										// 内部データ取得
	DWORD FASTCALL GetVector() const	{ return iosc.vbase; }
										// ベクタベース取得
	void FASTCALL IntAck();
										// 割り込み応答
	void FASTCALL IntFDC(BOOL flag);
										// FDC割り込み
	void FASTCALL IntFDD(BOOL flag);
										// FDD割り込み
	void FASTCALL IntHDC(BOOL flag);
										// HDC割り込み
	void FASTCALL IntPRT(BOOL flag);
										// プリンタ割り込み

private:
	void FASTCALL IntChk();
										// 割り込みチェック
	iosc_t iosc;
										// 内部データ
	Printer *printer;
										// プリンタ
};

#endif	// iosc_h
