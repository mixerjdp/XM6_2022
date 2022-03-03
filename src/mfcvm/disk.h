//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 ＰＩ．(ytanaka@ipc-tokai.or.jp)
//	[ ディスク ]
//
//---------------------------------------------------------------------------

#if !defined(disk_h)
#define disk_h

//---------------------------------------------------------------------------
//
//	クラス先行定義
//
//---------------------------------------------------------------------------
class DiskTrack;
class DiskCache;
class Disk;
class SASIHD;
class SCSIHD;
class SCSIMO;
class SCSICDTrack;
class SCSICD;

//---------------------------------------------------------------------------
//
//	エラー定義(REQUEST SENSEで返されるセンスコード)
//
//	MSB		予約(0x00)
//			センスキー
//			拡張センスコード(ASC)
//	LSB		拡張センスコードクォリファイア(ASCQ)
//
//---------------------------------------------------------------------------
#define DISK_NOERROR		0x00000000	// NO ADDITIONAL SENSE INFO.
#define DISK_DEVRESET		0x00062900	// POWER ON OR RESET OCCURED
#define DISK_NOTREADY		0x00023a00	// MEDIUM NOT PRESENT
#define DISK_ATTENTION		0x00062800	// MEDIUIM MAY HAVE CHANGED
#define DISK_PREVENT		0x00045302	// MEDIUM REMOVAL PREVENTED
#define DISK_READFAULT		0x00031100	// UNRECOVERED READ ERROR
#define DISK_WRITEFAULT		0x00030300	// PERIPHERAL DEVICE WRITE FAULT
#define DISK_WRITEPROTECT	0x00042700	// WRITE PROTECTED
#define DISK_MISCOMPARE		0x000e1d00	// MISCOMPARE DURING VERIFY
#define DISK_INVALIDCMD		0x00052000	// INVALID COMMAND OPERATION CODE
#define DISK_INVALIDLBA		0x00052100	// LOGICAL BLOCK ADDR. OUT OF RANGE
#define DISK_INVALIDCDB		0x00052400	// INVALID FIELD IN CDB
#define DISK_INVALIDLUN		0x00052500	// LOGICAL UNIT NOT SUPPORTED
#define DISK_INVALIDPRM		0x00052600	// INVALID FIELD IN PARAMETER LIST
#define DISK_INVALIDMSG		0x00054900	// INVALID MESSAGE ERROR
#define DISK_PARAMLEN		0x00051a00	// PARAMETERS LIST LENGTH ERROR
#define DISK_PARAMNOT		0x00052601	// PARAMETERS NOT SUPPORTED
#define DISK_PARAMVALUE		0x00052602	// PARAMETERS VALUE INVALID
#define DISK_PARAMSAVE		0x00053900	// SAVING PARAMETERS NOT SUPPORTED

#if 0
#define DISK_AUDIOPROGRESS	0x00??0011	// AUDIO PLAY IN PROGRESS
#define DISK_AUDIOPAUSED	0x00??0012	// AUDIO PLAY PAUSED
#define DISK_AUDIOSTOPPED	0x00??0014	// AUDIO PLAY STOPPED DUE TO ERROR
#define DISK_AUDIOCOMPLETE	0x00??0013	// AUDIO PLAY SUCCESSFULLY COMPLETED
#endif

//===========================================================================
//
//	ディスクトラック
//
//===========================================================================
class DiskTrack
{
public:
	// 内部データ定義
	typedef struct {
		int track;						// トラックナンバー
		int size;						// セクタサイズ(8 or 9)
		int sectors;					// セクタ数(<=0x100)
		BYTE *buffer;					// データバッファ
		BOOL init;						// ロード済みか
		BOOL changed;					// 変更済みフラグ
		BOOL *changemap;				// 変更済みマップ
		BOOL raw;						// RAWモード
	} disktrk_t;

public:
	// 基本ファンクション
	DiskTrack(int track, int size, int sectors, BOOL raw = FALSE);
										// コンストラクタ
	virtual ~DiskTrack();
										// デストラクタ
	BOOL FASTCALL Load(const Filepath& path);
										// ロード
	BOOL FASTCALL Save(const Filepath& path);
										// セーブ

	// リード・ライト
	BOOL FASTCALL Read(BYTE *buf, int sec) const;
										// セクタリード
	BOOL FASTCALL Write(const BYTE *buf, int sec);
										// セクタライト

	// その他
	int FASTCALL GetTrack() const		{ return dt.track; }
										// トラック取得
	BOOL FASTCALL IsChanged() const		{ return dt.changed; }
										// 変更フラグチェック

private:
	// 内部データ
	disktrk_t dt;
										// 内部データ
};

//===========================================================================
//
//	ディスクキャッシュ
//
//===========================================================================
class DiskCache
{
public:
	// 内部データ定義
	typedef struct {
		DiskTrack *disktrk;				// 割り当てトラック
		DWORD serial;					// 最終シリアル
	} cache_t;

	// キャッシュ数
	enum {
		CacheMax = 16					// キャッシュするトラック数
	};

public:
	// 基本ファンクション
	DiskCache(const Filepath& path, int size, int blocks);
										// コンストラクタ
	virtual ~DiskCache();
										// デストラクタ
	void FASTCALL SetRawMode(BOOL raw);
										// CD-ROM rawモード設定

	// アクセス
	BOOL FASTCALL Save();
										// 全セーブ＆解放
	BOOL FASTCALL Read(BYTE *buf, int block);
										// セクタリード
	BOOL FASTCALL Write(const BYTE *buf, int block);
										// セクタライト
	BOOL FASTCALL GetCache(int index, int& track, DWORD& serial) const;
										// キャッシュ情報取得

private:
	// 内部管理
	void FASTCALL Clear();
										// トラックをすべてクリア
	DiskTrack* FASTCALL Assign(int track);
										// トラックのロード
	BOOL FASTCALL Load(int index, int track);
										// トラックのロード
	void FASTCALL Update();
										// シリアル番号更新

	// 内部データ
	cache_t cache[CacheMax];
										// キャッシュ管理
	DWORD serial;
										// 最終アクセスシリアルナンバ
	Filepath sec_path;
										// パス
	int sec_size;
										// セクタサイズ(8 or 9 or 11)
	int sec_blocks;
										// セクタブロック数
	BOOL cd_raw;
										// CD-ROM RAWモード
};

//===========================================================================
//
//	ディスク
//
//===========================================================================
class Disk
{
public:
	// 内部ワーク
	typedef struct {
		DWORD id;						// メディアID
		BOOL ready;						// 有効なディスク
		BOOL writep;					// 書き込み禁止
		BOOL readonly;					// 読み込み専用
		BOOL removable;					// 取り外し
		BOOL lock;						// ロック
		BOOL attn;						// アテンション
		BOOL reset;						// リセット
		int size;						// セクタサイズ
		int blocks;						// 総セクタ数
		DWORD lun;						// LUN
		DWORD code;						// ステータスコード
		DiskCache *dcache;				// ディスクキャッシュ
	} disk_t;

public:
	// 基本ファンクション
	Disk(Device *dev);
										// コンストラクタ
	virtual ~Disk();
										// デストラクタ
	virtual void FASTCALL Reset();
										// デバイスリセット
	virtual BOOL FASTCALL Save(Fileio *fio, int ver);
										// セーブ
	virtual BOOL FASTCALL Load(Fileio *fio, int ver);
										// ロード

	// ID
	DWORD FASTCALL GetID() const		{ return disk.id; }
										// メディアID取得
	BOOL FASTCALL IsNULL() const;
										// NULLチェック
	BOOL FASTCALL IsSASI() const;
										// SASIチェック

	// メディア操作
	virtual BOOL FASTCALL Open(const Filepath& path);
										// オープン
	void FASTCALL GetPath(Filepath& path) const;
										// パス取得
	void FASTCALL Eject(BOOL force);
										// イジェクト
	BOOL FASTCALL IsReady() const		{ return disk.ready; }
										// Readyチェック
	void FASTCALL WriteP(BOOL flag);
										// 書き込み禁止
	BOOL FASTCALL IsWriteP() const		{ return disk.writep; }
										// 書き込み禁止チェック
	BOOL FASTCALL IsReadOnly() const	{ return disk.readonly; }
										// Read Onlyチェック
	BOOL FASTCALL IsRemovable() const	{ return disk.removable; }
										// リムーバブルチェック
	BOOL FASTCALL IsLocked() const		{ return disk.lock; }
										// ロックチェック
	BOOL FASTCALL IsAttn() const		{ return disk.attn; }
										// 交換チェック
	BOOL FASTCALL Flush();
										// キャッシュフラッシュ
	void FASTCALL GetDisk(disk_t *buffer) const;
										// 内部ワーク取得

	// プロパティ
	void FASTCALL SetLUN(DWORD lun)		{ disk.lun = lun; }
										// LUNセット
	DWORD FASTCALL GetLUN()				{ return disk.lun; }
										// LUN取得

	// コマンド
	virtual int FASTCALL Inquiry(const DWORD *cdb, BYTE *buf);
										// INQUIRYコマンド
	virtual int FASTCALL RequestSense(const DWORD *cdb, BYTE *buf);
										// REQUEST SENSEコマンド
	int FASTCALL SelectCheck(const DWORD *cdb);
										// SELECTチェック
	BOOL FASTCALL ModeSelect(const BYTE *buf, int size);
										// MODE SELECTコマンド
	int FASTCALL ModeSense(const DWORD *cdb, BYTE *buf);
										// MODE SENSEコマンド
	BOOL FASTCALL TestUnitReady(const DWORD *cdb);
										// TEST UNIT READYコマンド
	BOOL FASTCALL Rezero(const DWORD *cdb);
										// REZEROコマンド
	BOOL FASTCALL Format(const DWORD *cdb);
										// FORMAT UNITコマンド
	BOOL FASTCALL Reassign(const DWORD *cdb);
										// REASSIGN UNITコマンド
	virtual int FASTCALL Read(BYTE *buf, int block);
										// READコマンド
	int FASTCALL WriteCheck(int block);
										// WRITEチェック
	BOOL FASTCALL Write(const BYTE *buf, int block);
										// WRITEコマンド
	BOOL FASTCALL Seek(const DWORD *cdb);
										// SEEKコマンド
	BOOL FASTCALL StartStop(const DWORD *cdb);
										// START STOP UNITコマンド
	BOOL FASTCALL SendDiag(const DWORD *cdb);
										// SEND DIAGNOSTICコマンド
	BOOL FASTCALL Removal(const DWORD *cdb);
										// PREVENT/ALLOW MEDIUM REMOVALコマンド
	int FASTCALL ReadCapacity(const DWORD *cdb, BYTE *buf);
										// READ CAPACITYコマンド
	BOOL FASTCALL Verify(const DWORD *cdb);
										// VERIFYコマンド
	virtual int FASTCALL ReadToc(const DWORD *cdb, BYTE *buf);
										// READ TOCコマンド
	virtual BOOL FASTCALL PlayAudio(const DWORD *cdb);
										// PLAY AUDIOコマンド
	virtual BOOL FASTCALL PlayAudioMSF(const DWORD *cdb);
										// PLAY AUDIO MSFコマンド
	virtual BOOL FASTCALL PlayAudioTrack(const DWORD *cdb);
										// PLAY AUDIO TRACKコマンド
	void FASTCALL InvalidCmd()			{ disk.code = DISK_INVALIDCMD; }
										// サポートしていないコマンド

protected:
	// サブ処理
	int FASTCALL AddError(BOOL change, BYTE *buf);
										// エラーページ追加
	int FASTCALL AddFormat(BOOL change, BYTE *buf);
										// フォーマットページ追加
	int FASTCALL AddOpt(BOOL change, BYTE *buf);
										// オプティカルページ追加
	int FASTCALL AddCache(BOOL change, BYTE *buf);
										// キャッシュページ追加
	int FASTCALL AddCDROM(BOOL change, BYTE *buf);
										// CD-ROMページ追加
	int FASTCALL AddCDDA(BOOL change, BYTE *buf);
										// CD-DAページ追加
	BOOL FASTCALL CheckReady();
										// レディチェック

	// 内部データ
	disk_t disk;
										// ディスク内部データ
	Device *ctrl;
										// コントローラデバイス
	Filepath diskpath;
										// パス(GetPath用)
};

//===========================================================================
//
//	SASI ハードディスク
//
//===========================================================================
class SASIHD : public Disk
{
public:
	// 基本ファンクション
	SASIHD(Device *dev);
										// コンストラクタ
	BOOL FASTCALL Open(const Filepath& path);
										// オープン

	// メディア操作
	void FASTCALL Reset();
										// デバイスリセット

	// コマンド
	int FASTCALL RequestSense(const DWORD *cdb, BYTE *buf);
										// REQUEST SENSEコマンド
};

//===========================================================================
//
//	SCSI ハードディスク
//
//===========================================================================
class SCSIHD : public Disk
{
public:
	// 基本ファンクション
	SCSIHD(Device *dev);
										// コンストラクタ
	BOOL FASTCALL Open(const Filepath& path);
										// オープン

	// コマンド
	int FASTCALL Inquiry(const DWORD *cdb, BYTE *buf);
										// INQUIRYコマンド
};

//===========================================================================
//
//	SCSI 光磁気ディスク
//
//===========================================================================
class SCSIMO : public Disk
{
public:
	// 基本ファンクション
	SCSIMO(Device *dev);
										// コンストラクタ
	BOOL FASTCALL Open(const Filepath& path, BOOL attn = TRUE);
										// オープン
	BOOL FASTCALL Load(Fileio *fio, int ver);
										// ロード

	// コマンド
	int FASTCALL Inquiry(const DWORD *cdb, BYTE *buf);
										// INQUIRYコマンド
};

//===========================================================================
//
//	CD-ROM トラック
//
//===========================================================================
class CDTrack
{
public:
	// 基本ファンクション
	CDTrack(SCSICD *scsicd);
										// コンストラクタ
	virtual ~CDTrack();
										// デストラクタ
	BOOL FASTCALL Init(int track, DWORD first, DWORD last);
										// 初期化

	// プロパティ
	void FASTCALL SetPath(BOOL cdda, const Filepath& path);
										// パス設定
	void FASTCALL GetPath(Filepath& path) const;
										// パス取得
	void FASTCALL AddIndex(int index, DWORD lba);
										// インデックス追加
	DWORD FASTCALL GetFirst() const;
										// 開始LBA取得
	DWORD FASTCALL GetLast() const;
										// 終端LBA取得
	DWORD FASTCALL GetBlocks() const;
										// ブロック数取得
	int FASTCALL GetTrackNo() const;
										// トラック番号取得
	BOOL FASTCALL IsValid(DWORD lba) const;
										// 有効なLBAか
	BOOL FASTCALL IsAudio() const;
										// オーディオトラックか

private:
	SCSICD *cdrom;
										// 親デバイス
	BOOL valid;
										// 有効なトラック
	int track_no;
										// トラック番号
	DWORD first_lba;
										// 開始LBA
	DWORD last_lba;
										// 終了LBA
	BOOL audio;
										// オーディオトラックフラグ
	BOOL raw;
										// RAWデータフラグ
	Filepath imgpath;
										// イメージファイルパス
};

//===========================================================================
//
//	CD-DA バッファ
//
//===========================================================================
class CDDABuf
{
public:
	// 基本ファンクション
	CDDABuf();
										// コンストラクタ
	virtual ~CDDABuf();
										// デストラクタ
#if 0
	BOOL Init();
										// 初期化
	BOOL FASTCALL Load(const Filepath& path);
										// ロード
	BOOL FASTCALL Save(const Filepath& path);
										// セーブ

	// API
	void FASTCALL Clear();
										// バッファクリア
	BOOL FASTCALL Open(Filepath& path);
										// ファイル指定
	BOOL FASTCALL GetBuf(DWORD *buffer, int frames);
										// バッファ取得
	BOOL FASTCALL IsValid();
										// 有効チェック
	BOOL FASTCALL ReadReq();
										// 読み込み要求
	BOOL FASTCALL IsEnd() const;
										// 終了チェック

private:
	Filepath wavepath;
										// Waveパス
	BOOL valid;
										// オープン結果
	DWORD *buf;
										// データバッファ
	DWORD read;
										// Readポインタ
	DWORD write;
										// Writeポインタ
	DWORD num;
										// データ有効数
	DWORD rest;
										// ファイル残りサイズ
#endif
};

//===========================================================================
//
//	SCSI CD-ROM
//
//===========================================================================
class SCSICD : public Disk
{
public:
	// トラック数
	enum {
		TrackMax = 96					// トラック最大数
	};

public:
	// 基本ファンクション
	SCSICD(Device *dev);
										// コンストラクタ
	virtual ~SCSICD();
										// デストラクタ
	BOOL FASTCALL Open(const Filepath& path, BOOL attn = TRUE);
										// オープン
	BOOL FASTCALL Load(Fileio *fio, int ver);
										// ロード

	// コマンド
	int FASTCALL Inquiry(const DWORD *cdb, BYTE *buf);
										// INQUIRYコマンド
	int FASTCALL Read(BYTE *buf, int block);
										// READコマンド
	int FASTCALL ReadToc(const DWORD *cdb, BYTE *buf);
										// READ TOCコマンド
	BOOL FASTCALL PlayAudio(const DWORD *cdb);
										// PLAY AUDIOコマンド
	BOOL FASTCALL PlayAudioMSF(const DWORD *cdb);
										// PLAY AUDIO MSFコマンド
	BOOL FASTCALL PlayAudioTrack(const DWORD *cdb);
										// PLAY AUDIO TRACKコマンド

	// CD-DA
	BOOL FASTCALL NextFrame();
										// フレーム通知
	void FASTCALL GetBuf(DWORD *buffer, int samples, DWORD rate);
										// CD-DAバッファ取得

	// LBA-MSF変換
	void FASTCALL LBAtoMSF(DWORD lba, BYTE *msf) const;
										// LBA→MSF変換
	DWORD FASTCALL MSFtoLBA(const BYTE *msf) const;
										// MSF→LBA変換

private:
	// オープン
	BOOL FASTCALL OpenCue(const Filepath& path);
										// オープン(CUE)
	BOOL FASTCALL OpenIso(const Filepath& path);
										// オープン(ISO)
	BOOL rawfile;
										// RAWフラグ

	// トラック管理
	void FASTCALL ClearTrack();
										// トラッククリア
	int FASTCALL SearchTrack(DWORD lba) const;
										// トラック検索
	CDTrack* track[TrackMax];
										// トラックオブジェクト
	int tracks;
										// トラックオブジェクト有効数
	int dataindex;
										// 現在のデータトラック
	int audioindex;
										// 現在のオーディオトラック

	int frame;
										// フレーム番号

#if 0
	CDDABuf da_buf;
										// CD-DAバッファ
	int da_num;
										// CD-DAトラック数
	int da_cur;
										// CD-DAカレントトラック
	int da_next;
										// CD-DAネクストトラック
	BOOL da_req;
										// CD-DAデータ要求
#endif
};

#endif	// disk_h
