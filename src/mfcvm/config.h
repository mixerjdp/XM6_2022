//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 ＰＩ．(ytanaka@ipc-tokai.or.jp)
//	[ コンフィギュレーション ]
//
//---------------------------------------------------------------------------

#if !defined(config_h)
#define config_h

#include "filepath.h"

//===========================================================================
//
//	コンフィギュレーション(version2.00～version2.01)
//
//===========================================================================
class Config200 {
public:
	// システム
	int system_clock;					// システムクロック(0～5)
	int ram_size;						// メインRAMサイズ(0～5)
	BOOL ram_sramsync;					// メモリスイッチ自動更新

	// スケジューラ
	BOOL mpu_fullspeed;					// MPUフルスピード
	BOOL vm_fullspeed;					// VMフルスピード

	// サウンド
	int sound_device;					// サウンドデバイス(0～15)
	int sample_rate;					// サンプリングレート(0～4)
	int primary_buffer;					// バッファサイズ(2～100)
	int polling_buffer;					// ポーリング間隔(0～99)
	BOOL adpcm_interp;					// ADPCM線形補間あり

	// 描画
	BOOL aspect_stretch;				// アスペクト比にあわせ拡大

	// 音量
	int master_volume;					// マスタ音量(0～100)
	BOOL fm_enable;						// FM有効
	int fm_volume;						// FM音量(0～100)
	BOOL adpcm_enable;					// ADPCM有効
	int adpcm_volume;					// ADPCM音量(0～100)

	// キーボード
	BOOL kbd_connect;					// 接続

	// マウス
	int mouse_speed;					// スピード
	int mouse_port;						// 接続ポート
	BOOL mouse_swap;					// ボタンスワップ
	BOOL mouse_mid;						// 中ボタンイネーブル
	BOOL mouse_trackb;					// トラックボールモード

	// ジョイスティック
	int joy_type[2];					// ジョイスティックタイプ
	int joy_dev[2];						// ジョイスティックデバイス
	int joy_button0[12];				// ジョイスティックボタン(デバイスA)
	int joy_button1[12];				// ジョイスティックボタン(デバイスB)

	// SASI
	int sasi_drives;					// SASIドライブ数
	BOOL sasi_sramsync;					// SASIメモリスイッチ自動更新
	TCHAR sasi_file[16][FILEPATH_MAX];	// SASIイメージファイル

	// SxSI
	int sxsi_drives;					// SxSIドライブ数
	BOOL sxsi_mofirst;					// MOドライブ優先割り当て
	TCHAR sxsi_file[6][FILEPATH_MAX];	// SxSIイメージファイル

	// ポート
	int port_com;						// COMxポート
	TCHAR port_recvlog[FILEPATH_MAX];	// シリアル受信ログ
	BOOL port_384;						// シリアル38400bps固定
	int port_lpt;						// LPTxポート
	TCHAR port_sendlog[FILEPATH_MAX];	// パラレル送信ログ

	// MIDI
	int midi_bid;						// MIDIボードID
	int midi_ilevel;					// MIDI割り込みレベル
	int midi_reset;						// MIDIリセットコマンド
	int midiin_device;					// MIDI INデバイス
	int midiin_delay;					// MIDI INディレイ(ms)
	int midiout_device;					// MIDI OUTデバイス
	int midiout_delay;					// MIDI OUTディレイ(ms)

	// 改造
	BOOL sram_64k;						// 64KB SRAM
	BOOL scc_clkup;						// SCCクロックアップ
	BOOL power_led;						// 青色電源LED
	BOOL dual_fdd;						// 2DD/2HD兼用FDD
	BOOL sasi_parity;					// SASIバスパリティ

	// TrueKey
	int tkey_mode;						// TrueKeyモード(bit0:VM bit1:WinApp)
	int tkey_com;						// キーボードCOMポート
	BOOL tkey_rts;						// RTS反転モード

	// その他
	BOOL floppy_speed;					// フロッピーディスク高速
	BOOL floppy_led;					// フロッピーディスクLEDモード
	BOOL popup_swnd;					// ポップアップサブウィンドウ
	BOOL auto_mouse;					// 自動マウスモード制御
	BOOL power_off;						// 電源OFFで開始
	TCHAR ruta_savestate[FILEPATH_MAX];
};

//===========================================================================
//
//	コンフィギュレーション(version2.02～version2.03)
//
//===========================================================================
class Config202 : public Config200 {
public:
	// システム
	int mem_type;						// メモリマップ種別

	// SCSI
	int scsi_ilevel;					// SCSI割り込みレベル
	int scsi_drives;					// SCSIドライブ数
	BOOL scsi_sramsync;					// SCSIメモリスイッチ自動更新
	BOOL scsi_mofirst;					// MOドライブ優先割り当て
	TCHAR scsi_file[5][FILEPATH_MAX];	// SCSIイメージファイル
};

//===========================================================================
//
//	コンフィギュレーション
//
//===========================================================================
class Config : public Config202 {
public:
	// レジューム
	BOOL resume_fd;						// FDレジューム
	BOOL resume_fdi[2];					// FD挿入フラグ
	BOOL resume_fdw[2];					// FD書き込み禁止
	int resume_fdm[2];					// FDメディアNo.
	BOOL resume_mo;						// MOレジューム
	BOOL resume_mos;					// MO挿入フラグ
	BOOL resume_mow;					// MO書き込み禁止
	BOOL resume_cd;						// CDレジューム
	BOOL resume_iso;					// CD挿入フラグ
	BOOL resume_state;					// ステートレジューム
	BOOL resume_xm6;					// ステート有効フラグ
	BOOL resume_screen;					// 画面モードレジューム
	BOOL resume_dir;					// デフォルトディレクトリレジューム
	TCHAR resume_path[FILEPATH_MAX];	// デフォルトディレクトリ

	// 描画
	BOOL caption_info;					// キャプション情報表示

	// ディスプレイ
	BOOL caption;						// キャプション
	BOOL menu_bar;						// メニューバー
	BOOL status_bar;					// ステータスバー
	int window_left;					// ウィンドウ矩形
	int window_top;						// ウィンドウ矩形
	BOOL window_full;					// フルスクリーン
	int window_mode;					// ワイドスクリーン

	// WINDRVモジュール
	DWORD windrv_enable;				// Windrvサポート 0:無効 1:WindrvXM (2:Windrv互換)

	// ホスト側ファイルシステム
	DWORD host_option;					// 動作フラグ (class CHostFilename 参照)
	BOOL host_resume;					// ベースパス状態復元有効 FALSEだと毎回スキャンする
	DWORD host_drives;					// 有効なドライブ数
	DWORD host_flag[10];				// 動作フラグ (class CWinFileDrv 参照)
	TCHAR host_path[10][_MAX_PATH];		// ベースパス
};

#endif	// config_h
