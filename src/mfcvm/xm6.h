//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2005 ＰＩ．(ytanaka@ipc-tokai.or.jp)
//	[ 共通定義 ]
//
//---------------------------------------------------------------------------

#if !defined(xm6_h)
#define xm6_h

//---------------------------------------------------------------------------
//
//	基本定数
//
//---------------------------------------------------------------------------
#if !defined(FALSE)
#define FALSE		0
#define TRUE		(!FALSE)
#endif	// FALSE
#if !defined(NULL)
#define NULL		0
#endif	// NULL

//---------------------------------------------------------------------------
//
//	基本マクロ
//
//---------------------------------------------------------------------------
#if !defined(ASSERT)
#if !defined(NDEBUG)
#define ASSERT(cond)	assert(cond)
#else
#define ASSERT(cond)	((void)0)
#endif	// NDEBUG
#endif	// ASSERT

#if !defined(ASSERT_DIAG)
#if !defined(NDEBUG)
#define ASSERT_DIAG()	AssertDiag()
#else
#define ASSERT_DIAG()	((void)0)
#endif	// NDEBUG
#endif	// ASSERT_DIAG

//---------------------------------------------------------------------------
//
//	基本型定義
//
//---------------------------------------------------------------------------
typedef unsigned char BYTE;
typedef unsigned short WORD;
typedef unsigned long DWORD;
typedef int BOOL;

//---------------------------------------------------------------------------
//
//	IDマクロ
//
//---------------------------------------------------------------------------
#define MAKEID(a, b, c, d)	((DWORD)((a<<24) | (b<<16) | (c<<8) | d))

//---------------------------------------------------------------------------
//
//	クラス宣言
//
//---------------------------------------------------------------------------
class VM;
										// 仮想マシン
class Config;
										// コンフィギュレーション
class Device;
										// デバイス汎用
class MemDevice;
										// メモリマップドデバイス汎用
class Log;
										// ログ
class Event;
										// イベント
class Scheduler;
										// スケジューラ
class CPU;
										// CPU MC68000
class Memory;
										// アドレス空間 OHM2
class Fileio;
										// ファイル入出力
class SRAM;
										// スタティックRAM
class SysPort;
										// システムポート MESSIAH
class TVRAM;
										// テキストVRAM
class VC;
										// ビデオコントローラ VIPS&CATHY
class CRTC;
										// CRTC VICON
class RTC;
										// RTC RP5C15
class PPI;
										// PPI i8255A
class DMAC;
										// DMAC HD63450
class MFP;
										// MFP MC68901
class FDC;
										// FDC uPD72065
class FDD;
										// FDD FD55GFR
class IOSC;
										// I/Oコントローラ IOSC-2
class SASI;
										// SASI
class Sync;
										// 同期オブジェクト
class OPMIF;
										// OPM YM2151
class Keyboard;
										// キーボード
class ADPCM;
										// ADPCM MSM6258V
class GVRAM;
										// グラフィックVRAM
class Sprite;
										// スプライトRAM
class SCC;
										// SCC Z8530
class Mouse;
										// マウス
class Printer;
										// プリンタ
class AreaSet;
										// エリアセット
class Render;
										// レンダラ
class Windrv;
										// Windrv
class FDI;
										// フロッピーディスクイメージ
class Disk;
										// SASI/SCSIディスク
class MIDI;
										// MIDI YM3802
class Filepath;
										// ファイルパス
class JoyDevice;
										// ジョイスティックデバイス
class FileSys;
										// ファイルシステム
class SCSI;
										// SCSI MB89352
class Mercury;
										// Mercury-Unit

#endif	// xm6_h
