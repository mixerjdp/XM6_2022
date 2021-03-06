//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 ＰＩ．(ytanaka@ipc-tokai.or.jp)
//	[ SCSI(MB89352) ]
//
//---------------------------------------------------------------------------

#if !defined(scsi_h)
#define scsi_h

#include "device.h"
#include "event.h"
#include "disk.h"

//===========================================================================
//
//	SCSI
//
//===========================================================================
class SCSI : public MemDevice
{
public:
	// 最大数
	enum {
		DeviceMax = 8,					// 最大SCSIデバイス数
		HDMax = 5						// 最大SCSI HD数
	};

	// フェーズ定義
	enum phase_t {
		busfree,						// バスフリーフェーズ
		arbitration,					// アービトレーションフェーズ
		selection,						// セレクションフェーズ
		reselection,					// リセレクションフェーズ
		command,						// コマンドフェーズ
		execute,						// 実行フェーズ
		msgin,							// メッセージインフェーズ
		msgout,							// メッセージアウトフェーズ
		datain,							// データインフェーズ
		dataout,						// データアウトフェーズ
		status							// ステータスフェーズ
	};

	// 内部データ定義
	typedef struct {
		// 全般
		int type;						// SCSIタイプ(0:なし 1:外付 2:内蔵)
		phase_t phase;					// フェーズ
		int id;							// カレントID(0-7)

		// 割り込み
		int vector;						// 要求ベクタ(-1で要求なし)
		int ilevel;						// 割り込みレベル

		// 信号
		BOOL bsy;						// Busy信号
		BOOL sel;						// Select信号
		BOOL atn;						// Attention信号
		BOOL msg;						// Message信号
		BOOL cd;						// Command/Data信号
		BOOL io;						// Input/Output信号
		BOOL req;						// Request信号
		BOOL ack;						// Ack信号
		BOOL rst;						// Reset信号

		// レジスタ
		DWORD bdid;						// BDIDレジスタ(ビット表示)
		DWORD sctl;						// SCTLレジスタ
		DWORD scmd;						// SCMDレジスタ
		DWORD ints;						// INTSレジスタ
		DWORD sdgc;						// SDGCレジスタ
		DWORD pctl;						// PCTLレジスタ
		DWORD mbc;						// MBCレジスタ
		DWORD temp;						// TEMPレジスタ
		DWORD tc;						// TCH,TCM,TCLレジスタ

		// コマンド
		DWORD cmd[10];					// コマンドデータ
		DWORD status;					// ステータスデータ
		DWORD message;					// メッセージデータ

		// 転送
		BOOL trans;						// 転送フラグ
		BYTE buffer[0x800];				// 転送バッファ
		DWORD blocks;					// 転送ブロック数
		DWORD next;						// 次のレコード
		DWORD offset;					// 転送オフセット
		DWORD length;					// 転送残り長さ

		// コンフィグ
		int scsi_drives;				// SCSIドライブ数
		BOOL memsw;						// メモリスイッチ更新
		BOOL mo_first;					// MO優先フラグ(SxSI)

		// ディスク
		Disk *disk[DeviceMax];			// デバイス
		Disk *hd[HDMax];				// HD
		SCSIMO *mo;						// MO
		SCSICD *cdrom;					// CD-ROM
	} scsi_t;

public:
	// 基本ファンクション
	SCSI(VM *p);
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
	void FASTCALL GetSCSI(scsi_t *buffer) const;
										// 内部データ取得
	BOOL FASTCALL Callback(Event *ev);
										// イベントコールバック
	void FASTCALL IntAck(int level);
										// 割り込みACK
	int FASTCALL GetSCSIID() const;
										// SCSI-ID取得
	BOOL FASTCALL IsBusy() const;
										// BUSYか
	DWORD FASTCALL GetBusyDevice() const;
										// BUSYデバイス取得

	// MO/CDアクセス
	BOOL FASTCALL Open(const Filepath& path, BOOL mo = TRUE);
										// MO/CD オープン
	void FASTCALL Eject(BOOL force, BOOL mo = TRUE);
										// MO/CD イジェクト
	void FASTCALL WriteP(BOOL writep);
										// MO 書き込み禁止
	BOOL FASTCALL IsWriteP() const;
										// MO 書き込み禁止チェック
	BOOL FASTCALL IsReadOnly() const;
										// MO ReadOnlyチェック
	BOOL FASTCALL IsLocked(BOOL mo = TRUE) const;
										// MO/CD Lockチェック
	BOOL FASTCALL IsReady(BOOL mo = TRUE) const;
										// MO/CD Readyチェック
	BOOL FASTCALL IsValid(BOOL mo = TRUE) const;
										// MO/CD 有効チェック
	void FASTCALL GetPath(Filepath &path, BOOL mo = TRUE) const;
										// MO/CD パス取得

	// CD-DA
	void FASTCALL GetBuf(DWORD *buffer, int samples, DWORD rate);
										// CD-DAバッファ取得

private:
	// レジスタ
	void FASTCALL ResetReg();
										// レジスタリセット
	void FASTCALL ResetCtrl();
										// 転送リセット
	void FASTCALL ResetBus(BOOL reset);
										// バスリセット
	void FASTCALL SetBDID(DWORD data);
										// BDID設定
	void FASTCALL SetSCTL(DWORD data);
										// SCTL設定
	void FASTCALL SetSCMD(DWORD data);
										// SCMD設定
	void FASTCALL SetINTS(DWORD data);
										// INTS設定
	DWORD FASTCALL GetPSNS() const;
										// PSNS取得
	void FASTCALL SetSDGC(DWORD data);
										// SDGC設定
	DWORD FASTCALL GetSSTS() const;
										// SSTS取得
	DWORD FASTCALL GetSERR() const;
										// SERR取得
	void FASTCALL SetPCTL(DWORD data);
										// PCTL設定
	DWORD FASTCALL GetDREG();
										// DREG取得
	void FASTCALL SetDREG(DWORD data);
										// DREG設定
	void FASTCALL SetTEMP(DWORD data);
										// TEMP設定
	void FASTCALL SetTCH(DWORD data);
										// TCH設定
	void FASTCALL SetTCM(DWORD data);
										// TCM設定
	void FASTCALL SetTCL(DWORD data);
										// TCL設定

	// SPCコマンド
	void FASTCALL BusRelease();
										// バスリリース
	void FASTCALL Select();
										// セレクション/リセレクション
	void FASTCALL ResetATN();
										// ATNライン=0
	void FASTCALL SetATN();
										// ATNライン=1
	void FASTCALL Transfer();
										// 転送
	void FASTCALL TransPause();
										// 転送中断
	void FASTCALL TransComplete();
										// 転送完了
	void FASTCALL ResetACKREQ();
										// ACK/REQライン=0
	void FASTCALL SetACKREQ();
										// ACK/REQライン=1

	// データ転送
	void FASTCALL Xfer(DWORD *reg);
										// データ転送
	void FASTCALL XferNext();
										// データ転送継続
	BOOL FASTCALL XferIn();
										// データ転送IN
	BOOL FASTCALL XferOut(BOOL cont);
										// データ転送OUT
	BOOL FASTCALL XferMsg(DWORD msg);
										// データ転送MSG

	// フェーズ
	void FASTCALL BusFree();
										// バスフリーフェーズ
	void FASTCALL Arbitration();
										// アービトレーションフェーズ
	void FASTCALL Selection();
										// セレクションフェーズ
	void FASTCALL SelectTime();
										// セレクションフェーズ(時間設定)
	void FASTCALL Command();
										// コマンドフェーズ
	void FASTCALL Execute();
										// 実行フェーズ
	void FASTCALL MsgIn();
										// メッセージインフェーズ
	void FASTCALL MsgOut();
										// メッセージアウトフェーズ
	void FASTCALL DataIn();
										// データインフェーズ
	void FASTCALL DataOut();
										// データアウトフェーズ
	void FASTCALL Status();
										// ステータスフェーズ

	// 割り込み
	void FASTCALL Interrupt(int type, BOOL flag);
										// 割り込み要求
	void FASTCALL IntCheck();
										// 割り込みチェック

	// SCSIコマンド共通
	void FASTCALL Error();
										// 共通エラー

	// SCSIコマンド別
	void FASTCALL TestUnitReady();
										// TEST UNIT READYコマンド
	void FASTCALL Rezero();
										// REZERO UNITコマンド
	void FASTCALL RequestSense();
										// REQUEST SENSEコマンド
	void FASTCALL Format();
										// FORMAT UNITコマンド
	void FASTCALL Reassign();
										// REASSIGN BLOCKSコマンド
	void FASTCALL Read6();
										// READ(6)コマンド
	void FASTCALL Write6();
										// WRITE(6)コマンド
	void FASTCALL Seek6();
										// SEEK(6)コマンド
	void FASTCALL Inquiry();
										// INQUIRYコマンド
	void FASTCALL ModeSelect();
										// MODE SELECTコマンド
	void FASTCALL ModeSense();
										// MODE SENSEコマンド
	void FASTCALL StartStop();
										// START STOP UNITコマンド
	void FASTCALL SendDiag();
										// SEND DIAGNOSTICコマンド
	void FASTCALL Removal();
										// PREVENT/ALLOW MEDIUM REMOVALコマンド
	void FASTCALL ReadCapacity();
										// READ CAPACITYコマンド
	void FASTCALL Read10();
										// READ(10)コマンド
	void FASTCALL Write10();
										// WRITE(10)コマンド
	void FASTCALL Seek10();
										// SEEK(10)コマンド
	void FASTCALL Verify();
										// VERIFYコマンド
	void FASTCALL ReadToc();
										// READ TOCコマンド
	void FASTCALL PlayAudio10();
										// PLAY AUDIO(10)コマンド
	void FASTCALL PlayAudioMSF();
										// PLAY AUDIO MSFコマンド
	void FASTCALL PlayAudioTrack();
										// PLAY AUDIO TRACK INDEXコマンド

	// CD-ROM・CD-DA
	Event cdda;
										// フレームイベント

	// ドライブ・ファイルパス
	void FASTCALL Construct();
										// ドライブ構築
	Filepath scsihd[DeviceMax];
										// SCSI-HDファイルパス

	// その他
	scsi_t scsi;
										// 内部データ
	BYTE verifybuf[0x800];
										// ベリファイバッファ
	Event event;
										// イベント
	Memory *memory;
										// メモリ
	SRAM *sram;
										// SRAM
};

#endif	// scsi_h
