//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2004 ＰＩ．(ytanaka@ipc-tokai.or.jp)
//	Modified (C) 2006 co (cogood＠gmail.com)
//	[ Windrv ]
//
//---------------------------------------------------------------------------

#if !defined(windrv_h)
#define windrv_h

#include "device.h"

//■最大スレッド数
#define WINDRV_THREAD_MAX	3

//■WINDRV互換動作のサポート
#define WINDRV_SUPPORT_COMPATIBLE

//■ログ出力有効
#ifdef _DEBUG
#define WINDRV_LOG
#endif // _DEBUG

class FileSys;

//---------------------------------------------------------------------------
//
//	ステータスコード定義
//
//---------------------------------------------------------------------------
#define FS_INVALIDFUNC		0xffffffff	// 無効なファンクションコードを実行した
#define FS_FILENOTFND		0xfffffffe	// 指定したファイルが見つからない
#define FS_DIRNOTFND		0xfffffffd	// 指定したディレクトリが見つからない
#define FS_OVEROPENED		0xfffffffc	// オープンしているファイルが多すぎる
#define FS_CANTACCESS		0xfffffffb	// ディレクトリやボリュームラベルはアクセス不可
#define FS_NOTOPENED		0xfffffffa	// 指定したハンドルはオープンされていない
#define FS_INVALIDMEM		0xfffffff9	// メモリ管理領域が破壊された
#define FS_OUTOFMEM			0xfffffff8	// 実行に必要なメモリがない
#define FS_INVALIDPTR		0xfffffff7	// 無効なメモリ管理ポインタを指定した
#define FS_INVALIDENV		0xfffffff6	// 不正な環境を指定した
#define FS_ILLEGALFMT		0xfffffff5	// 実行ファイルのフォーマットが異常
#define FS_ILLEGALMOD		0xfffffff4	// オープンのアクセスモードが異常
#define FS_INVALIDPATH		0xfffffff3	// ファイル名の指定に誤りがある
#define FS_INVALIDPRM		0xfffffff2	// 無効なパラメータでコールした
#define FS_INVALIDDRV		0xfffffff1	// ドライブ指定に誤りがある
#define FS_DELCURDIR		0xfffffff0	// カレントディレクトリは削除できない
#define FS_NOTIOCTRL		0xffffffef	// IOCTRLできないデバイス
#define FS_LASTFILE			0xffffffee	// これ以上ファイルが見つからない
#define FS_CANTWRITE		0xffffffed	// 指定のファイルは書き込みできない
#define FS_DIRALREADY		0xffffffec	// 指定のディレクトリは既に登録されている
#define FS_CANTDELETE		0xffffffeb	// ファイルがあるので削除できない
#define FS_CANTRENAME		0xffffffea	// ファイルがあるのでリネームできない
#define FS_DISKFULL			0xffffffe9	// ディスクが一杯でファイルが作れない
#define FS_DIRFULL			0xffffffe8	// ディレクトリが一杯でファイルが作れない
#define FS_CANTSEEK			0xffffffe7	// 指定の位置にはシークできない
#define FS_SUPERVISOR		0xffffffe6	// スーパーバイザ状態でスーパバイザ指定した
#define FS_THREADNAME		0xffffffe5	// 同じスレッド名が存在する
#define FS_BUFWRITE			0xffffffe4	// プロセス間通信のバッファが書込み禁止
#define FS_BACKGROUND		0xffffffe3	// バックグラウンドプロセスを起動できない
#define FS_OUTOFLOCK		0xffffffe0	// ロック領域が足りない
#define FS_LOCKED			0xffffffdf	// ロックされていてアクセスできない
#define FS_DRIVEOPENED		0xffffffde	// 指定のドライブはハンドラがオープンされている
#define FS_LINKOVER			0xffffffdd	// シンボリックリンクネストが16回を超えた
#define FS_FILEEXIST		0xffffffb0	// ファイルが存在する

#define FS_FATAL_INVALIDUNIT	0xFFFFFFA0	// 不正なユニット番号
#define FS_FATAL_INVALIDCOMMAND	0xFFFFFFA1	// 不正なコマンド番号
#define FS_FATAL_WRITEPROTECT	0xFFFFFFA2	// 書き込み禁止違反
#define FS_FATAL_MEDIAOFFLINE	0xFFFFFFA3	// メディアが入っていない

//===========================================================================
//
//	Human68k
//
//===========================================================================
#define HUMAN68K_MAX_PATH	96
class Human68k {
public:
	// ファイル属性ビット
	enum {
		AT_READONLY	= 0x01,
		AT_HIDDEN	= 0x02,
		AT_SYSTEM	= 0x04,
		AT_VOLUME	= 0x08,
		AT_DIRECTORY= 0x10,
		AT_ARCHIVE	= 0x20,
		AT_ALL		= 0xFF,
	};

	// ファイルオープンモード
	enum {
		OP_READ		= 0,
		OP_WRITE	= 1,
		OP_READWRITE= 2,
	};

	// シーク種類
	enum {
		SK_BEGIN	= 0,
		SK_CURRENT	= 1,
		SK_END		= 2,
	};

	// namests構造体
	typedef struct {
		BYTE wildcard;			// ワイルドカード文字数
		BYTE drive;				// ドライブ番号
		BYTE path[65];			// パス(サブディレクトリ+/)
		BYTE name[8];			// ファイル名 (PADDING 0x20)
		BYTE ext[3];			// 拡張子 (PADDING 0x20)
		BYTE add[10];			// ファイル名追加 (PADDING 0x00)

		// 取得(Unicode対応可能)
		void FASTCALL GetCopyPath(BYTE* szPath) const;
		void FASTCALL GetCopyFilename(BYTE* szFilename) const;
	} namests_t;

	// files構造体
	typedef struct {
		BYTE fatr;				// + 0 検索する属性				読込専用
		BYTE drive;				// + 1 ドライブ番号				読込専用
		DWORD sector;			// + 2 ディレクトリのセクタNo	DOS _FILES 先頭アドレスで代用
		//WORD sector2;			// + 6 ディレクトリのセクタNo	詳細不明 未使用
		WORD offset;			// + 8 ディレクトリの位置		詳細不明
		//BYTE name[8];			// +10 作業用ファイル名			読込専用 未使用
		//BYTE ext[3];			// +18 作業用拡張子				読込専用 未使用
		BYTE attr;				// +21 ファイル属性				書込専用
		WORD time;				// +22 最終変更時刻				書込専用
		WORD date;				// +24 最終変更月日				書込専用
		DWORD size;				// +26 ファイルサイズ			書込専用
		BYTE full[23];			// +30 フルファイル名			書込専用
	} files_t;

	// FCB構造体
	typedef struct {
		//BYTE pad00[6];		// + 0～+ 5
		DWORD fileptr;			// + 6～+ 9	ファイルポインタ
		//BYTE pad01[4];		// +10～+13
		WORD mode;				// +14～+15	オープンモード
		//BYTE pad02[16];		// +16～+31
		//DWORD zero;			// +32～+35	オープンのとき0が書き込まれている 未使用
		//BYTE name[8];			// +36～+43	ファイル名 (PADDING 0x20) 未使用
		//BYTE ext[3];			// +44～+46	拡張子 (PADDING 0x20) 未使用
		BYTE attr;				// +47		ファイル属性
		//BYTE add[10];			// +48～+57	ファイル名追加 (PADDING 0x00) 未使用
		WORD time;				// +58～+59	最終変更時刻
		WORD date;				// +60～+61	最終変更月日
		//WORD cluster;			// +62～+63	クラスタ番号 未使用
		DWORD size;				// +64～+67	ファイルサイズ
		//BYTE pad03[28];		// +68～+95
	} fcb_t;

	// capacity構造体
	typedef struct {
		WORD free;				// + 0 使用可能なクラスタ数
		WORD clusters;			// + 2 総クラスタ数
		WORD sectors;			// + 4 クラスタあたりのセクタ数
		WORD bytes;				// + 6 セクタ当たりのバイト数
	} capacity_t;

	// ctrldrive構造体
	typedef struct {
		BYTE status;			// +13	状態
	} ctrldrive_t;

	// DPB構造体
	typedef struct {
		WORD sector_size;		// + 0	1 セクタ当りのバイト数
		BYTE cluster_size;		// + 2	1 クラスタ当りのセクタ数-1
		BYTE shift;				// + 3	クラスタ→セクタのシフト数
		WORD fat_sector;		// + 4	FAT の先頭セクタ番号
		BYTE fat_max;			// + 6	FAT 領域の個数
		BYTE fat_size;			// + 7	FAT の占めるセクタ数(複写分を除く)
		WORD file_max;			// + 8	ルートディレクトリに入るファイルの個数
		WORD data_sector;		// +10	データ領域の先頭セクタ番号
		WORD cluster_max;		// +12	総クラスタ数+1
		WORD root_sector;		// +14	ルートディレクトリの先頭セクタ番号
		//DWORD driverentry;	// +16	デバイスドライバへのポインタ
		BYTE media;				// +20	メディア識別子
		//BYTE flag;			// +21	DPB 使用フラグ
	} dpb_t;

	// ディレクトリエントリ構造体
	typedef struct {
		BYTE name[8];			// + 0	ファイル名 (PADDING 0x20)
		BYTE ext[3];			// + 8	拡張子 (PADDING 0x20)
		BYTE attr;				// +11	ファイル属性
		BYTE add[10];			// +12	ファイル名追加 (PADDING 0x00)
		WORD time;				// +22	最終変更時刻
		WORD date;				// +24	最終変更月日
		WORD cluster;			// +26	クラスタ番号
		DWORD size;				// +28	ファイルサイズ
	} dirent_t;

	// IOCTRL構造体
	typedef union {
		BYTE buffer[8];
		DWORD param;
		WORD media;
	} ioctrl_t;
};

//===========================================================================
//
//	コマンドハンドラ
//
//===========================================================================
class CWindrv {
public:
	// 基本ファンクション
	CWindrv(Windrv* pWindrv, Memory* pMemory, DWORD nHandle = 0);
										// コンストラクタ
	virtual ~CWindrv();
										// デストラクタ

	// スレッド処理
	BOOL FASTCALL Start();
										// スレッド起動
	BOOL FASTCALL Terminate();
										// スレッド停止

	// Windrvが利用する外部API
	void FASTCALL Execute(DWORD nA5);
										// コマンド実行
#ifdef WINDRV_SUPPORT_COMPATIBLE
	DWORD FASTCALL ExecuteCompatible(DWORD nA5);
										// コマンド実行 (WINDRV互換)
#endif // WINDRV_SUPPORT_COMPATIBLE
	DWORD FASTCALL GetHandle() const { ASSERT(this); return m_nHandle; }
										// 自身のハンドル(配列番号)を獲得
	BOOL FASTCALL isExecute() const { ASSERT(this); return m_bExecute; }
										// コマンド実行中かどうか確認

	// CWindrvManagerが利用する外部API
	void FASTCALL SetAlloc(BOOL bAlloc) { ASSERT(this); m_bAlloc = bAlloc; }
	BOOL FASTCALL isAlloc() const { ASSERT(this); return m_bAlloc; }
										// ハンドル使用中かどうか確認

	// FileSysが利用する外部API
	DWORD FASTCALL GetUnit() const { ASSERT(this); return unit; }
										// ユニット番号取得
	Memory* FASTCALL GetMemory() const { ASSERT(this); return memory; }
										// ユニット番号取得
	void FASTCALL LockXM() { ASSERT(this); if (m_bReady) ::LockVM(); }
										// VMへのアクセス開始
	void FASTCALL UnlockXM() { ASSERT(this); if (m_bReady) ::UnlockVM(); }
										// VMへのアクセス終了
	void FASTCALL Ready();
										// コマンド完了を待たずにVMスレッド実行再開

	// スレッド実体
	static DWORD WINAPI Run(VOID* pThis);
										// スレッド実行開始ポイント
	void FASTCALL Runner();
										// スレッド実体

private:
	// コマンドハンドラ
	void FASTCALL ExecuteCommand();
										// コマンドハンドラ

	void FASTCALL InitDrive();
										// $40 - 初期化
	void FASTCALL CheckDir();
										// $41 - ディレクトリチェック
	void FASTCALL MakeDir();
										// $42 - ディレクトリ作成
	void FASTCALL RemoveDir();
										// $43 - ディレクトリ削除
	void FASTCALL Rename();
										// $44 - ファイル名変更
	void FASTCALL Delete();
										// $45 - ファイル削除
	void FASTCALL Attribute();
										// $46 - ファイル属性取得/設定
	void FASTCALL Files();
										// $47 - ファイル検索(First)
	void FASTCALL NFiles();
										// $48 - ファイル検索(NFiles)
	void FASTCALL Create();
										// $49 - ファイル作成
	void FASTCALL Open();
										// $4A - ファイルオープン
	void FASTCALL Close();
										// $4B - ファイルクローズ
	void FASTCALL Read();
										// $4C - ファイル読み込み
	void FASTCALL Write();
										// $4D - ファイル書き込み
	void FASTCALL Seek();
										// $4E - ファイルシーク
	void FASTCALL TimeStamp();
										// $4F - ファイル時刻取得/設定
	void FASTCALL GetCapacity();
										// $50 - 容量取得
	void FASTCALL CtrlDrive();
										// $51 - ドライブ状態検査/制御
	void FASTCALL GetDPB();
										// $52 - DPB取得
	void FASTCALL DiskRead();
										// $53 - セクタ読み込み
	void FASTCALL DiskWrite();
										// $54 - セクタ書き込み
	void FASTCALL IoControl();
										// $55 - IOCTRL
	void FASTCALL Flush();
										// $56 - フラッシュ
	void FASTCALL CheckMedia();
										// $57 - メディア交換チェック
	void FASTCALL Lock();
										// $58 - 排他制御

	// 終了値
	void FASTCALL SetResult(DWORD result);
										// 終了値書き込み

	// メモリアクセス
	DWORD FASTCALL GetByte(DWORD addr) const;
										// バイト読み込み
	void FASTCALL SetByte(DWORD addr, DWORD data);
										// バイト書き込み
	DWORD FASTCALL GetWord(DWORD addr) const;
										// ワード読み込み
	void FASTCALL SetWord(DWORD addr, DWORD data);
										// ワード書き込み
	DWORD FASTCALL GetLong(DWORD addr) const;
										// ロング読み込み
	void FASTCALL SetLong(DWORD addr, DWORD data);
										// ロング書き込み
	DWORD FASTCALL GetAddr(DWORD addr) const;
										// アドレス読み込み
	void FASTCALL SetAddr(DWORD addr, DWORD data);
										// アドレス書き込み

	// 構造体変換
	void FASTCALL GetNameStsPath(DWORD addr, Human68k::namests_t* pNamests) const;
										// NAMESTSパス名読み込み
	void FASTCALL GetNameSts(DWORD addr, Human68k::namests_t* pNamests) const;
										// NAMESTS読み込み
	void FASTCALL GetFiles(DWORD addr, Human68k::files_t* pFiles) const;
										// FILES読み込み
	void FASTCALL SetFiles(DWORD addr, const Human68k::files_t* pFiles);
										// FILES書き込み
	void FASTCALL GetFcb(DWORD addr, Human68k::fcb_t* pFcb) const;
										// FCB読み込み
	void FASTCALL SetFcb(DWORD addr, const Human68k::fcb_t* pFcb);
										// FCB書き込み
	void FASTCALL SetCapacity(DWORD addr, const Human68k::capacity_t* pCapacity);
										// CAPACITY書き込み
	void FASTCALL SetDpb(DWORD addr, const Human68k::dpb_t* pDpb);
										// DPB書き込み
	void FASTCALL GetIoctrl(DWORD param, DWORD func, Human68k::ioctrl_t* pIoctrl);
										// IOCTRL読み込み
	void FASTCALL SetIoctrl(DWORD param, DWORD func, const Human68k::ioctrl_t* pIoctrl);
										// IOCTRL書き込み
	void FASTCALL GetParameter(DWORD addr, BYTE* pOption, DWORD nSize);
										// パラメータ読み込み

#ifdef WINDRV_LOG
	// ログ
	void Log(DWORD level, char* format, ...) const;
										// ログ出力
#endif // WINDRV_LOG

	// コマンドハンドラ用
	Windrv* windrv;
										// 設定内容
	Memory* memory;
										// メモリ
	DWORD a5;
										// リクエストヘッダ
	DWORD unit;
										// ユニット番号
	DWORD command;
										// コマンド番号

	// スレッド管理用
	DWORD m_nHandle;
										// バッファ番号をハンドルとして使う(0～THREAD_MAX - 1)
	BOOL m_bAlloc;
										// ハンドル使用中のときTRUE
	BOOL m_bExecute;
										// コマンド実行中のときTRUE
	BOOL m_bReady;
										// VMスレッド実行再開のときTRUE
	BOOL m_bTerminate;
										// スレッド利用終了のときTRUE
	HANDLE m_hThread;
										// スレッドハンドル
	HANDLE m_hEventStart;
										// コマンド実行開始通知
	HANDLE m_hEventReady;
										// VMスレッド実行再開通知
};

//===========================================================================
//
//	コマンドハンドラ管理
//
//===========================================================================
class CWindrvManager {
public:
	void FASTCALL Init(Windrv* pWindrv, Memory* pMemory);
										// 初期化
	void FASTCALL Clean();
										// 終了
	void FASTCALL Reset();
										// リセット

	CWindrv* FASTCALL Alloc();
										// 空きスレッド確保
	CWindrv* FASTCALL Search(DWORD nHandle);
										// スレッド検索
	void FASTCALL Free(CWindrv* p);
										// スレッド開放

private:
	CWindrv* m_pThread[WINDRV_THREAD_MAX];
										// コマンドハンドラ実体
};

//===========================================================================
//
//	Windrv
//
//===========================================================================
class Windrv : public MemDevice
{
public:
	// Windrv動作
	enum {
		WINDRV_MODE_NONE = 0,
		WINDRV_MODE_ENABLE = 1,
		WINDRV_MODE_COMPATIBLE = 2,
		WINDRV_MODE_DUAL = 3,
		WINDRV_MODE_DISABLE = 255,
	};

	// 内部データ定義
	typedef struct {
		// コンフィグ
		DWORD enable;					// Windrvサポート 0:無効 1:WindrvXM (2:Windrv互換)

		// ドライブ・ファイル管理
		DWORD drives;					// 確保されたドライブ数 (FileSysアクセス境界チェック用)

		// プロセス
		FileSys *fs;					// ファイルシステム
	} windrv_t;

public:
	// 基本ファンクション
	Windrv(VM *p);
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
	void FASTCALL SetFileSys(FileSys *fs);
										// ファイルシステム設定

	// コマンドハンドラ用 外部API
	void FASTCALL SetUnitMax(DWORD nUnitMax) { ASSERT(this); windrv.drives = nUnitMax; }
										// 現在のユニット番号上限設定
	BOOL FASTCALL isInvalidUnit(DWORD unit) const;
										// ユニット番号チェック
#ifdef _DEBUG
	BOOL FASTCALL isValidUnit(DWORD unit) const { ASSERT(this); return unit < windrv.drives; }
										// ユニット番号チェック (ASSERT専用)
#endif // _DEBUG
	FileSys* FASTCALL GetFilesystem() const { ASSERT(this); return windrv.fs; }
										// ファイルシステムの獲得

#ifdef WINDRV_LOG
	// ログ
	void FASTCALL Log(DWORD level, char* message) const;
										// ログ出力
#endif // WINDRV_LOG

private:
#ifdef WINDRV_SUPPORT_COMPATIBLE
	void FASTCALL Execute();
										// コマンド実行
#endif // WINDRV_SUPPORT_COMPATIBLE
	void FASTCALL ExecuteAsynchronous();
										// コマンド実行開始 非同期
	DWORD FASTCALL StatusAsynchronous();
										// ステータス獲得 非同期
	void FASTCALL ReleaseAsynchronous();
										// ハンドル開放 非同期

	windrv_t windrv;
										// 内部データ
	CWindrvManager m_cThread;
										// コマンドハンドラ管理
};

//===========================================================================
//
//	ファイルシステム
//
//	すべて純粋仮想関数とする。
//
//===========================================================================
class FileSys
{
public:
	// 基本ファンクション
	//FileSys();
										// コンストラクタ
	//virtual ~FileSys() = 0;
										// デストラクタ

	// 初期化・終了
	virtual DWORD FASTCALL Init(CWindrv* ps, DWORD nDriveMax, const BYTE* pOption) = 0;
										// $40 - 初期化
	virtual void FASTCALL Reset() = 0;
										// リセット(全クローズ)

	// コマンドハンドラ
	virtual int FASTCALL CheckDir(CWindrv* ps, const Human68k::namests_t* pNamests) = 0;
										// $41 - ディレクトリチェック
	virtual int FASTCALL MakeDir(CWindrv* ps, const Human68k::namests_t* pNamests) = 0;
										// $42 - ディレクトリ作成
	virtual int FASTCALL RemoveDir(CWindrv* ps, const Human68k::namests_t* pNamests) = 0;
										// $43 - ディレクトリ削除
	virtual int FASTCALL Rename(CWindrv* ps, const Human68k::namests_t* pNamests, const Human68k::namests_t* pNamestsNew) = 0;
										// $44 - ファイル名変更
	virtual int FASTCALL Delete(CWindrv* ps, const Human68k::namests_t* pNamests) = 0;
										// $45 - ファイル削除
	virtual int FASTCALL Attribute(CWindrv* ps, const Human68k::namests_t* pNamests, DWORD nHumanAttribute) = 0;
										// $46 - ファイル属性取得/設定
	virtual int FASTCALL Files(CWindrv* ps, const Human68k::namests_t* pNamests, DWORD nKey, Human68k::files_t* info) = 0;
										// $47 - ファイル検索(First)
	virtual int FASTCALL NFiles(CWindrv* ps, DWORD nKey, Human68k::files_t* info) = 0;
										// $48 - ファイル検索(Next)
	virtual int FASTCALL Create(CWindrv* ps, const Human68k::namests_t* pNamests, DWORD nKey, Human68k::fcb_t* pFcb, DWORD nAttribute, BOOL bForce) = 0;
										// $49 - ファイル作成
	virtual int FASTCALL Open(CWindrv* ps, const Human68k::namests_t* pNamests, DWORD nKey, Human68k::fcb_t* pFcb) = 0;
										// $4A - ファイルオープン
	virtual int FASTCALL Close(CWindrv* ps, DWORD nKey, Human68k::fcb_t* pFcb) = 0;
										// $4B - ファイルクローズ
	virtual int FASTCALL Read(CWindrv* ps, DWORD nKey, Human68k::fcb_t* pFcb, DWORD nAddress, DWORD nSize) = 0;
										// $4C - ファイル読み込み
	virtual int FASTCALL Write(CWindrv* ps, DWORD nKey, Human68k::fcb_t* pFcb, DWORD nAddress, DWORD nSize) = 0;
										// $4D - ファイル書き込み
	virtual int FASTCALL Seek(CWindrv* ps, DWORD nKey, Human68k::fcb_t* pFcb, DWORD nMode, int nOffset) = 0;
										// $4E - ファイルシーク
	virtual DWORD FASTCALL TimeStamp(CWindrv* ps, DWORD nKey, Human68k::fcb_t* pFcb, WORD nFatDate, WORD nFatTime) = 0;
										// $4F - ファイル時刻取得/設定
	virtual int FASTCALL GetCapacity(CWindrv* ps, Human68k::capacity_t* cap) = 0;
										// $50 - 容量取得
	virtual int FASTCALL CtrlDrive(CWindrv* ps, Human68k::ctrldrive_t* pCtrlDrive) = 0;
										// $51 - ドライブ状態検査/制御
	virtual int FASTCALL GetDPB(CWindrv* ps, Human68k::dpb_t* pDpb) = 0;
										// $52 - DPB取得
	virtual int FASTCALL DiskRead(CWindrv* ps, DWORD nAddress, DWORD nSector, DWORD nSize) = 0;
										// $53 - セクタ読み込み
	virtual int FASTCALL DiskWrite(CWindrv* ps, DWORD nAddress, DWORD nSector, DWORD nSize) = 0;
										// $54 - セクタ書き込み
	virtual int FASTCALL IoControl(CWindrv* ps, Human68k::ioctrl_t* pIoctrl, DWORD nFunction) = 0;
										// $55 - IOCTRL
	virtual int FASTCALL Flush(CWindrv* ps) = 0;
										// $56 - フラッシュ
	virtual int FASTCALL CheckMedia(CWindrv* ps) = 0;
										// $57 - メディア交換チェック
	virtual int FASTCALL Lock(CWindrv* ps) = 0;
										// $58 - 排他制御

	// エラーハンドラ
	//virtual DWORD FASTCALL GetLastError(DWORD nUnit) const = 0;
										// 最終エラー取得
};

#endif // windrv_h
