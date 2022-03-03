//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2004 ＰＩ．(ytanaka@ipc-tokai.or.jp)
//	[ プリンタ ]
//
//---------------------------------------------------------------------------

#if !defined(printer_h)
#define printer_h

#include "device.h"

//===========================================================================
//
//	プリンタ
//
//===========================================================================
class Printer : public MemDevice
{
public:
	// 定数値
	enum {
		BufMax = 0x1000				// バッファサイズ(2の倍数)
	};

	// 内部データ定義
	typedef struct {
		BOOL connect;					// 接続
		BOOL strobe;					// ストローブ
		BOOL ready;						// レディ
		BYTE data;						// 書き込みデータ
		BYTE buf[BufMax];				// バッファデータ
		DWORD read;						// バッファ読み込み位置
		DWORD write;					// バッファ書き込み位置
		DWORD num;						// バッファ有効数
	} printer_t;

public:
	// 基本ファンクション
	Printer(VM *p);
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
	BOOL FASTCALL IsReady() const		{ return printer.ready; }
										// レディ取得
	void FASTCALL HSync();
										// H-Sync通知
	void FASTCALL GetPrinter(printer_t *buffer) const;
										// 内部データ取得
	void FASTCALL Connect(BOOL flag);
										// プリンタ接続
	BOOL FASTCALL GetData(BYTE *ptr);
										// 先頭データ取得

private:
	IOSC *iosc;
										// IOSC
	Sync *sync;
										// 同期オブジェクト
	printer_t printer;
										// 内部データ
};

#endif	// printer_h
