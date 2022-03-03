//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 ＰＩ．(ytanaka@ipc-tokai.or.jp)
//	[ 仮想マシン ]
//
//---------------------------------------------------------------------------

#if !defined(vm_h)
#define vm_h

#include "log.h"
#include "schedule.h"
#include "cpu.h"
#include "filepath.h"

//===========================================================================
//
//	仮想マシン
//
//===========================================================================
class VM
{
public:
	// 基本ファンクション
	VM();
										// コンストラクタ
	BOOL FASTCALL Init();
										// 初期化
	void FASTCALL Cleanup();
										// クリーンアップ
	void FASTCALL Reset();
										// リセット
	void FASTCALL ApplyCfg(const Config *config);
										// 設定適用

	// ステート保存
	DWORD FASTCALL Save(const Filepath& path);
										// セーブ
	DWORD FASTCALL Load(const Filepath& path);
										// ロード
	void FASTCALL GetPath(Filepath& path) const;
										// パス取得
	void FASTCALL Clear();
										// パスをクリア

	// デバイス管理
	void FASTCALL AddDevice(Device *device);
										// デバイス追加(子から呼ばれる)
	void FASTCALL DelDevice(const Device *device);
										// デバイス削除(子から呼ばれる)
	Device* FASTCALL GetFirstDevice() const	{ return first_device; }
										// 最初のデバイスを取得
	Device* FASTCALL SearchDevice(DWORD id) const;
										// 任意IDのデバイスを取得

	// 実行
	BOOL FASTCALL Exec(DWORD hus);
										// 実行
	void FASTCALL Trace();
										// トレース
	void FASTCALL Break()				{ scheduler->Break(); }
										// 実行中止

	// バージョン
	void FASTCALL SetVersion(DWORD major, DWORD minor);
										// バージョン設定
	void FASTCALL GetVersion(DWORD& major, DWORD& minor);
										// バージョン取得

	// システム制御
	void FASTCALL PowerSW(BOOL sw);
										// 電源スイッチ制御
	BOOL FASTCALL IsPowerSW() const		{ return power_sw; }
										// 電源スイッチ状態取得
	void FASTCALL SetPower(BOOL flag);
										// 電源制御
	BOOL FASTCALL IsPower() const		{ return power; }
										// 電源状態取得
	void FASTCALL Interrupt() const		{ cpu->Interrupt(7, -1); }
										// NMI割り込み
	Log log;
										// ログ

private:
	BOOL status;
										// 初期化ステータス
	Device *first_device;
										// 最初のデバイス
	Scheduler *scheduler;
										// スケジューラ
	CPU *cpu;
										// CPU
	MFP *mfp;
										// MFP
	RTC *rtc;
										// RTC
	SRAM *sram;
										// SRAM
	BOOL power_sw;
										// 電源スイッチ
	BOOL power;
										// 電源
	DWORD major_ver;
										// メジャーバージョン
	DWORD minor_ver;
										// マイナーバージョン
	Filepath current;
										// カレントデータ
};

#endif	// vm_h
