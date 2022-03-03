//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2004 ＰＩ．(ytanaka@ipc-tokai.or.jp)
//	[ 仮想マシンコア アセンブラサブ ]
//
//---------------------------------------------------------------------------

#if !defined (core_asm_h)
#define core_asm_h

//#if _MSC_VER >= 1200

#if defined(__cplusplus)
extern "C" {
#endif	//__cplusplus

//---------------------------------------------------------------------------
//
//	プロトタイプ宣言
//
//---------------------------------------------------------------------------
void MemInitDecode(Memory *mem, MemDevice* list[]);
										// メモリデコーダ初期化
DWORD ReadByteC(DWORD addr);
										// バイト読み込み
DWORD ReadWordC(DWORD addr);
										// ワード読み込み
void WriteByteC(DWORD addr, DWORD data);
										// バイト書き込み
void WriteWordC(DWORD addr, DWORD data);
										// ワード書き込み
void ReadErrC(DWORD addr);
										// バスエラー読み込み
void WriteErrC(DWORD addr, DWORD data);
										// バスエラー書き込み
void NotifyEvent(Event *first);
										// イベント群 指定
DWORD GetMinEvent(DWORD hus);
										// イベント群 最小のものを探す
BOOL SubExecEvent(DWORD hus);
										// イベント群 減算＆実行
extern BYTE MemDecodeTable[];
										// メモリデコードテーブル

#if defined(__cplusplus)
}
#endif	//__cplusplus

//#endif	// _MSC_VER
#endif	// mem_asm_h
