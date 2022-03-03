//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 ＰＩ．(ytanaka@ipc-tokai.or.jp)
//	[ システムポート ]
//
//---------------------------------------------------------------------------

#if !defined(sysport_h)
#define sysport_h

#include "device.h"

//===========================================================================
//
//	システムポート
//
//===========================================================================
class SysPort : public MemDevice
{
public:
	// 内部データ定義
	typedef struct {
		DWORD contrast;					// コントラスト
		DWORD scope_3d;					// 3Dスコープ制御
		DWORD image_unit;				// イメージユニット制御
		DWORD power_count;				// 電源制御カウンタ
		DWORD ver_count;				// バージョン管理カウンタ
	} sysport_t;

public:
	// 基本ファンクション
	SysPort(VM *p);
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

private:
	DWORD FASTCALL GetVR();
										// バージョンレジスタ読み出し
	sysport_t sysport;
										// 内部ワーク
	Memory *memory;
										// メモリ
	SRAM *sram;
										// スタティックRAM
	Keyboard *keyboard;
										// キーボード
	CRTC *crtc;
										// CRTC
	Render *render;
										// レンダラ
};

#endif	// sysport_h
