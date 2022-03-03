//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2004 ＰＩ．(ytanaka@ipc-tokai.or.jp)
//	Modified (C) 2006 co (cogood＠gmail.com)
//	[ MFC Host ]
//
//---------------------------------------------------------------------------

#if defined(_WIN32)

#if !defined(mfc_host_h)
#define mfc_host_h

#include "device.h"
#include "windrv.h"

//■ファイルネーム検索後のチェックをシフトJISで行なわない(英語OS用)
//#define XM6_HOST_NO_JAPANESE_NAME

//■ファイルのオープン/クローズを毎回行なう
// オープン/クローズ管理はVM側で行なわれているので有効にしても意味はない。
// TODO: ステートロード時のハンドル復帰処理の検討
//#define XM6_HOST_STRICT_CLOSE

//■リムーバブルメディアのキャッシュ有効時間を正確に測定する
// 本機能を適用しなかった場合、Human68k側のアプリケーションの作りに
// によっては、ホスト側でメディアのイジェクト操作を長時間行なえなく
// なる弊害が発生するので注意。
#define XM6_HOST_STRICT_TIMEOUT

//■ファイル検索時に返すファイルサイズの最大値
// Human68kエラーコードと重複しない範囲で設定する
#define XM6_HOST_FILESIZE_MAX 0xFFFFFF7F

//■ファイル読み書き時のバッファサイズ
// 1バイトづつ呼ばれたりすることもあるようなので、あまり大量に取っても
// 無駄になってしまう可能性もある。
// TODO: あらかじめ固定バッファを確保しておくべきか検討
// スレッドモードでは、このサイズを転送する毎にVMの開放が行なわれるため、
// 最大転送速度に影響する。
#define XM6_HOST_FILE_BUFFER_READ 0x10000
#define XM6_HOST_FILE_BUFFER_WRITE 0x10000

//■FILES用バッファ個数
// 複数のスレッドが同時に深い階層に渡って作業する時などは
// この値を増やす必要がある
#define XM6_HOST_FILES_MAX 20

//■FCB用バッファ個数
// 同時にオープンできるファイル数はこれで決まる
#define XM6_HOST_FCB_MAX 100

//■擬似セクタ/クラスタ 最大個数
// ファイル実体の先頭セクタへのアクセスに対応
// lzdsysによるアクセスを行なうスレッドの数より多めに確保する
#define XM6_HOST_PSEUDO_CLUSTER_MAX 10

//■ディレクトリエントリ キャッシュ個数
// 多ければ多いほど高速になるが、増やしすぎるとホストOS側に負担がかかる
#define XM6_HOST_DIRENTRY_CACHE_MAX 30

//■ディレクトリ内のファイル名 キャッシュ個数
// 全てのファイル名が重複判定された場合は約6千万ファイルが上限となる(36の5乗)
#define XM6_HOST_FILENAME_CACHE_MAX (36 * 36 * 36 * 36 * 36)

//■ファイル名重複防止マーク
// ホスト側ファイル名とHuman68k側ファイル名の名称の区別をつけるときに使う文字
// コマンドシェル等のエスケープ文字と重ならないものを選ぶと吉
#define XM6_HOST_FILENAME_MARK '@'

//■リムーバブルメディアの挿入をコマンド発行順序を契機に検出する
// 確実に検出できるが、若干無駄なアクセスが発生する可能性あり
#define XM6_HOST_UPDATE_BY_SEQUENCE

//■リムーバブルメディアの挿入をアクセス頻度を契機に検出する
// コマンド発行順序での検出がうまくいかない場合のため残してある。
// 本方式では、mint2.xx系の挿入チェックが常時有効になるため、動作速
// 度が低下する問題あり。
//#define XM6_HOST_UPDATE_BY_FREQUENCY

//■リムーバブルメディアの挿入をアクセス頻度を常時行なう
// 上記の二方式でうまくいかない場合はこれを使う。ただし手動イジェク
// トのメディアが存在すると動作速度が常時低下するので注意。
//#define XM6_HOST_UPDATE_ALWAYS

#ifdef XM6_HOST_UPDATE_BY_FREQUENCY
//■リムーバブルメディアの更新判定期間(ms)と更新判定回数
// CheckMedia($57)によるメディア交換チェック発行回数がこの値より少なければ何もしない
// mint3.x系の場合、100ms以内に8回のアクセスでリロードさせる。
#define XM6_HOST_REMOVABLE_RELOAD_TIME 100
#define XM6_HOST_REMOVABLE_RELOAD_COUNT 8
#endif // XM6_HOST_UPDATE_BY_FREQUENCY

//■リムーバブルメディアの連続アクセスガード時間(ms)
// 最後のメディア有効チェック間隔からこの時間経過するまでは再チェックしない
// メディアが存在しないときチェックに時間のかかるデバイスへの対策。
#define XM6_HOST_REMOVABLE_GUARD_TIME 3000

//■リムーバブルメディアのキャッシュ有効時間(ms)
// ファイルアクセス間隔がこの時間より開いた場合、キャッシュがクリアされる
// 手動イジェクトメディアの場合、ファイルアクセスの直前には必ずメディ
// ア交換チェックを行なうため、連続してファイルアクセスが実行された
// 場合はキャッシュ内容をそのまま保持する。
#define XM6_HOST_REMOVABLE_CACHE_TIME 3000

//■リムーバブルメディアのキャッシュ確認間隔(ms)
// 値を小さくするとより正確に確認するようになるが、あまり意味はない
#define XM6_HOST_REMOVABLE_CHECK_TIME 1000

//■TwentyOne常駐確認間隔
// TwentyOneが常駐していない場合、毎回検索するのは時間の無駄なので指
// 定回数おきに確認する。常駐していた場合は検索は最初の1回のみなので
// 気にしなくてよい。また、自動検索をしない場合は意味を持たない。
#define XM6_HOST_TWENTYONE_CHECK_COUNT 10

//■WINDRV動作フラグ
// Bit 0: ファイル名変換 スペース		0:なし 1:'_'
// Bit 1: ファイル名変換 無効な文字		0:なし 1:'_'
// Bit 2: ファイル名変換 中間のハイフン	0:なし 1:'_'
// Bit 3: ファイル名変換 先頭のハイフン	0:なし 1:'_'
// Bit 4: ファイル名変換 中間のピリオド	0:なし 1:'_'
// Bit 5: ファイル名変換 先頭のピリオド	0:なし 1:'_'
//
// Bit 8: ファイル名処理 Alphabet区別	0:なし 1:あり	0:-C 1:+C
// Bit 9: ファイル名比較 文字数(未実装)	0:18+3 1:8+3	0:+T 1:-T
// Bit10: ファイル名変換 文字数			0:18+3 1:8+3	0:-A 1:+A
// Bit11: 予約 (文字コード)				0:SJIS 1:ANSI
// Bit12: 予約							0:なし 1:謎
// Bit13: メディアバイト処理			0:固定 1:変動
// Bit14: TwentyOne監視処理				0:なし 1:監視
// Bit15: ファイル削除処理				0:直接 1:ごみ箱
//
// Bit16: ファイル名短縮 スペース		0:なし 1:短縮
// Bit17: ファイル名短縮 無効な文字		0:なし 1:短縮
// Bit18: ファイル名短縮 中間のハイフン	0:なし 1:短縮
// Bit19: ファイル名短縮 先頭のハイフン	0:なし 1:短縮
// Bit20: ファイル名短縮 中間のピリオド	0:なし 1:短縮
// Bit21: ファイル名短縮 先頭のピリオド	0:なし 1:短縮
//
// Bit24～30 ファイル重複防止マーク	0:自動 1～127:文字
enum {
	WINDRV_OPT_CONVERT_SPACE =		0x00000001,
	WINDRV_OPT_CONVERT_BADCHAR =	0x00000002,
	WINDRV_OPT_CONVERT_HYPHENS =	0x00000004,
	WINDRV_OPT_CONVERT_HYPHEN =		0x00000008,
	WINDRV_OPT_CONVERT_PERIODS =	0x00000010,
	WINDRV_OPT_CONVERT_PERIOD =		0x00000020,

	WINDRV_OPT_ALPHABET =			0x00000100,
	WINDRV_OPT_COMPARE_LENGTH =		0x00000200,
	WINDRV_OPT_CONVERT_LENGTH =		0x00000400,

	WINDRV_OPT_MEDIABYTE =			0x00002000,
	WINDRV_OPT_TWENTYONE =			0x00004000,
	WINDRV_OPT_REMOVE =				0x00008000,

	WINDRV_OPT_REDUCED_SPACE =		0x00010000,
	WINDRV_OPT_REDUCED_BADCHAR =	0x00020000,
	WINDRV_OPT_REDUCED_HYPHENS =	0x00040000,
	WINDRV_OPT_REDUCED_HYPHEN =		0x00080000,
	WINDRV_OPT_REDUCED_PERIODS =	0x00100000,
	WINDRV_OPT_REDUCED_PERIOD =		0x00200000,
};

//■ファイルシステム動作フラグ
// 通常は0にする。判定が困難なデバイス(自作USBストレージとか)のための保険。
// Bit0: 強制書き込み禁止
// Bit1: 強制低速デバイス
// Bit2: 強制リムーバブルメディア
// Bit3: 強制手動イジェクトメディア
enum {
	FSFLAG_WRITE_PROTECT =			0x00000001,
	FSFLAG_SLOW =					0x00000002,
	FSFLAG_REMOVABLE =				0x00000004,
	FSFLAG_MANUAL =					0x00000008,
};

//===========================================================================
//
//	Windowsファイルドライブ
//
//	ドライブ毎に必要な情報の保持に専念し、管理は上位で行なう。
//	CWinFileSysで実体を管理し、CHostEntryで利用する。
//	TODO: 実体をCHostEntry内部へ移動させるか検討
//
//===========================================================================
class CWinFileDrv
{
public:
	// 基本ファンクション
	CWinFileDrv();
										// コンストラクタ
	virtual ~CWinFileDrv();
										// デストラクタ
	void FASTCALL Init(LPCTSTR szBase, DWORD nFlag);
										// 初期化

	TCHAR* FASTCALL GetBase() const { ASSERT(this); return (TCHAR*)m_szBase; }
										// ベースパスの取得
	BOOL FASTCALL isWriteProtect() const { ASSERT(this); return m_bWriteProtect; }
										// 書き込み許可情報の取得
	BOOL FASTCALL isSlow() const { ASSERT(this); return m_bSlow; }
										// 書き込み許可情報の取得
	BOOL FASTCALL isRemovable() const { ASSERT(this); return m_bRemovable; }
										// リムーバブルメディアか？
	BOOL FASTCALL isManual() const { ASSERT(this); return m_bManual; }
										// 手動イジェクトメディアか？
	BOOL FASTCALL isEnable() const { ASSERT(this); return m_bEnable; }
										// アクセス可能か？
	BOOL FASTCALL isSameDrive(const TCHAR* szDrive) const { ASSERT(this); return _tcscmp(m_szDrive, szDrive) == 0; }
										// 同一ドライブ名か？
	DWORD FASTCALL GetStatus() const;
										// ドライブ状態の取得
	void FASTCALL SetEnable(BOOL bEnable);
										// メディア状態設定
	void FASTCALL SetTimeout();
										// リムーバブルメディアのファイルキャッシュ有効時間を更新
	BOOL FASTCALL CheckTimeout();
										// リムーバブルメディアのファイルキャッシュ有効時間を確認
#ifdef XM6_HOST_UPDATE_BY_SEQUENCE
	void FASTCALL SetMediaUpdate(BOOL bDisable);
										// リムーバブルメディアの状態更新を有効にする
#endif // XM6_HOST_UPDATE_BY_SEQUENCE
	BOOL FASTCALL CheckMediaUpdate();
										// リムーバブルメディアの状態更新チェック
	BOOL FASTCALL CheckMediaAccess(BOOL bManual);
										// リムーバブルメディアのアクセス事前チェック
	BOOL FASTCALL CheckMedia();
										// メディア有効チェック
	BOOL FASTCALL Eject();
										// イジェクト
	void FASTCALL GetVolume(TCHAR* szLabel);
										// ボリュームラベルの取得
	BOOL FASTCALL GetVolumeCache(TCHAR* szLabel) const;
										// キャッシュからボリュームラベルを取得
	DWORD FASTCALL GetCapacity(Human68k::capacity_t* pCapacity);
										// 容量の取得
	BOOL FASTCALL GetCapacityCache(Human68k::capacity_t* pCapacity) const;
										// キャッシュからクラスタサイズを取得

private:
	void FASTCALL OpenDevice();
										// デバイスオープン
	void FASTCALL CloseDevice();
										// デバイスクローズ

	BOOL m_bWriteProtect;
										// 書き込み禁止ならTRUE
	BOOL m_bSlow;
										// 低速デバイスならTRUE
	BOOL m_bRemovable;
										// リムーバブルメディアならTRUE
	BOOL m_bManual;
										// 手動イジェクトならTRUE
	BOOL m_bEnable;
										// メディアが利用可能ならTRUE
	BOOL m_bDevice;
										// WinNT系で正しいデバイス名を持っているならTRUE
	HANDLE m_hDevice;
										// デバイスハンドル(NT系専用)
	TCHAR m_szDevice[8];
										// デバイス名(NT系専用)
	TCHAR m_szDrive[4];
										// ドライブ名
	Human68k::capacity_t m_capCache;
										// セクタ情報キャッシュ sectors == 0 なら未キャッシュ
	BOOL m_bVolumeCache;
										// ボリュームラベル読み込み済みならTRUE
	TCHAR m_szVolumeCache[24];
										// ボリュームラベルキャッシュ
	DWORD m_nUpdate;
										// 最後にメディア利用可能状態を獲得した時刻
	BOOL m_bUpdateFile;
										// キャッシュクリアの判定が必要ならTRUE
	DWORD m_nUpdateFile;
										// 最後にファイルアクセスを行なった時刻
#ifdef XM6_HOST_UPDATE_BY_SEQUENCE
	DWORD m_nUpdateMedia;
										// リムーバブルメディアの更新判定ステート
#endif // XM6_HOST_UPDATE_BY_SEQUENCE
#ifdef XM6_HOST_UPDATE_BY_FREQUENCY
	DWORD m_nUpdateCount;
										// リムーバブルメディアの更新判定バッファ位置
	DWORD m_nUpdateBuffer[XM6_HOST_REMOVABLE_RELOAD_COUNT];
										// リムーバブルメディアの更新判定バッファ
#endif // XM6_HOST_UPDATE_BY_FREQUENCY

	TCHAR m_szBase[_MAX_PATH];
										// ベースパス
};

//===========================================================================
//
//	まるっとリングリスト
//
//	先頭(root.next)が最も新しいオブジェクト
//	末尾(root.prev)が最も古い/未使用オブジェクト
//
//===========================================================================
class CRing {
public:
	CRing() { next = prev = this; }
										// コンストラクタ
	virtual ~CRing() { Remove(); }
										// デストラクタは仮想関数化。確実に開放させる
	CRing* FASTCALL Next() const { ASSERT(this); return next; }
	CRing* FASTCALL Prev() const { ASSERT(this); return prev; }
										// 移動用

	void FASTCALL Insert(class CRing* pRoot)
										// オブジェクト切り離し & リング先頭へ挿入
	{
		ASSERT(this);
		// 該当オブジェクトを切り離し
		ASSERT(next);
		ASSERT(prev);
		next->prev = prev;
		prev->next = next;
		// リング先頭へ挿入
		ASSERT(pRoot);
		ASSERT(pRoot->next);
		next = pRoot->next;
		prev = pRoot;
		pRoot->next->prev = this;
		pRoot->next = this;
	}

	void FASTCALL InsertTail(class CRing* pRoot)
										// オブジェクト切り離し & リング末尾へ挿入
	{
		ASSERT(this);
		// 該当オブジェクトを切り離し
		ASSERT(next);
		ASSERT(prev);
		next->prev = prev;
		prev->next = next;
		// リング末尾へ挿入
		ASSERT(pRoot);
		ASSERT(pRoot->prev);
		next = pRoot;
		prev = pRoot->prev;
		pRoot->prev->next = this;
		pRoot->prev = this;
	}

	void FASTCALL InsertRing(class CRing* pRoot)
										// 自分以外のオブジェクト切り離し & リング先頭へ挿入
	{
		ASSERT(this);
		if (next == prev) return;

		// リング先頭へ挿入
		ASSERT(pRoot);
		ASSERT(pRoot->next);
		pRoot->next->prev = prev;
		prev->next = pRoot->next;
		pRoot->next = next;
		next->prev = pRoot;

		// 自分自身を空にする
		next = prev = this;
	}

	void FASTCALL Remove()
										// オブジェクト切り離し
	{
		ASSERT(this);
		// 該当オブジェクトを切り離し
		ASSERT(next);
		ASSERT(prev);
		next->prev = prev;
		prev->next = next;
		// 安全のため自分自身を指しておく (何度切り離しても問題ない)
		next = prev = this;
	}

	friend void FASTCALL Insert(class CRing* pRoot);
	friend void FASTCALL InsertTail(class CRing* pRoot);
	friend void FASTCALL Remove();
										// メンバ変数の設定メソッドを公開しないため、
										// オブジェクト操作メソッドはfriend登録が必要

private:
	class CRing* next;
										// 次の要素
	class CRing* prev;
										// 前の要素
};

//===========================================================================
//
//	ディレクトリエントリ ファイル名
//
//===========================================================================
class CHostFilename: public CRing {
public:
	CHostFilename();
										// コンストラクタ
	virtual ~CHostFilename();
										// デストラクタ
	void FASTCALL FreeWin32() { ASSERT(this); if(m_pszWin32) { free(m_pszWin32); m_pszWin32 = NULL; } }
										// Win32要素の開放
	void FASTCALL SetWin32(const TCHAR* szWin32);
										// Win32側の名称の設定
	void FASTCALL SetHuman(int nCount = -1);
										// Human68k側の名称を変換
	void FASTCALL CopyHuman(const BYTE* szHuman);
										// Human68k側の名称を複製
	BOOL FASTCALL SetEntry(const WIN32_FIND_DATA* pWin32Find);
										// ディレクトリエントリ情報の設定
	inline BOOL FASTCALL isReduce() const;
										// Human68k側の名称が加工されたか調査
	BOOL FASTCALL isCorrect() const { ASSERT(this); return m_bCorrect; }
										// Human68k側のファイル名規則に合致しているか調査
	BYTE* FASTCALL GetHuman() const { ASSERT(this); return (BYTE*)m_szHuman; }
										// Human68kファイル名を取得
	BYTE* FASTCALL GetHumanLast() const { ASSERT(this); return m_pszHumanLast; }
										// Human68kファイル名を取得
	BYTE* FASTCALL GetHumanExt() const { ASSERT(this); return m_pszHumanExt; }
										// Human68kファイル名を取得
	BOOL FASTCALL CheckAttribute(DWORD nHumanAttribute) const { ASSERT(this); return (m_dirEntry.attr & nHumanAttribute); }
										// Human68k側の属性の該当判定
	Human68k::dirent_t* FASTCALL GetEntry() const { ASSERT(this); return (Human68k::dirent_t*)&m_dirEntry; }
										// Human68kディレクトリエントリを取得
	TCHAR* FASTCALL GetWin32() const { ASSERT(this); return m_pszWin32; }
										// Win32ファイル名を取得
	void FASTCALL SetChild(class CHostEntry* pEntry, class CHostPath* pChild) { ASSERT(this); m_pEntry = pEntry; m_pChild = pChild; }
										// ディレクトリの実体を登録
	BOOL FASTCALL isSameEntry(Human68k::dirent_t* pdirEntry) const { ASSERT(this); ASSERT(pdirEntry); return memcmp(&m_dirEntry, pdirEntry, sizeof(m_dirEntry)) == 0; }
										// エントリの一致判定

	// CHostFilesが利用する外部API
	void FASTCALL DeleteCache();
										// 該当エントリのキャッシュを削除

	// CWinFileSysが利用する外部API
	static void FASTCALL SetOption(DWORD nOption) { g_nOption = nOption; }
										// 動作フラグ設定

	// 汎用ルーチン
	static const BYTE* FASTCALL SeparateExt(const BYTE* szHuman);
										// Human68kファイル名から拡張子を分離

private:
	inline BYTE* FASTCALL CopyName(BYTE* pWrite, const BYTE* pFirst, const BYTE* pLast);
										//	Human68k側のファイル名要素をコピー

	TCHAR* m_pszWin32;
										// 該当エントリの実体
	BOOL m_bCorrect;
										// 該当エントリのHuman68k内部名が正しければ TRUE
	BYTE m_szHuman[24];
										// 該当エントリのHuman68k内部名 直接検索用
	Human68k::dirent_t m_dirEntry;
										// 該当エントリのHuman68k全情報

	BYTE* m_pszHumanLast;
										// 該当エントリ生成・検索時の高速化用 終端位置
	BYTE* m_pszHumanExt;
										// 該当エントリ生成・検索時の高速化用 拡張子位置

	class CHostEntry* m_pEntry;
										// 該当エントリの管理者 (キャッシュがなければNULL)
	class CHostPath* m_pChild;
										// 該当エントリの内容 (キャッシュがなければNULL)

	static DWORD g_nOption;
										// 動作フラグ (ファイル名変換/短縮関連)
};

//===========================================================================
//
//	ディレクトリエントリ パス名
//
//===========================================================================
class CHostPath: public CRing {
public:
	// 検索用バッファ
	typedef struct {
		DWORD count;
										// 検索実行回数 + 1 (0のときは以下の値は無効)
		DWORD id;
										// 次回検索を続行するパスのエントリ識別ID
		CHostFilename* pos;
										// 次回検索を続行する位置 (識別ID一致時)
		Human68k::dirent_t entry;
										// 次回検索を続行するエントリ内容

		void FASTCALL Clear() { count = 0; }
										// 初期化
	} find_t;

public:
	CHostPath();
										// コンストラクタ
	virtual ~CHostPath();
										// デストラクタ
	void FASTCALL FreeHuman() { ASSERT(this); m_nHumanUnit = 0; if(m_pszHuman) { free(m_pszHuman); m_pszHuman = NULL; } }
										// Human68k要素の開放
	void FASTCALL FreeWin32() { ASSERT(this); if(m_pszWin32) { free(m_pszWin32); m_pszWin32 = NULL; } }
										// Win32要素の開放
	void FASTCALL SetHuman(DWORD nUnit, BYTE* szHuman);
										// Human68k側の名称を直接指定する
	void FASTCALL SetWin32(TCHAR* szWin32);
										// Win32側の名称を直接指定する
	BOOL FASTCALL isSameUnit(DWORD nUnit) const { ASSERT(this); return m_nHumanUnit == nUnit; }
										// Human68k側の名称を比較する
	BOOL FASTCALL isSameHuman(DWORD nUnit, const BYTE* szHuman) const;
										// Human68k側の名称を比較する
	TCHAR* FASTCALL GetWin32() const { ASSERT(this); return m_pszWin32; }
										// Win32パス名の獲得
	CHostFilename* FASTCALL FindFilename(BYTE* szHuman, DWORD nHumanAttribute = Human68k::AT_ALL);
										// ファイル名を検索
	CHostFilename* FASTCALL FindFilenameWildcard(BYTE* szHuman, find_t* pFind, DWORD nHumanAttribute = Human68k::AT_ALL);
										// ファイル名を検索 (ワイルドカード対応)
	void FASTCALL Clean();
										// 再利用のための初期化
	void FASTCALL CleanFilename();
										// 全ファイル名を開放
	BOOL FASTCALL isRefresh();
										// ファイル変更が行なわれたか確認
	BOOL FASTCALL Refresh();
										// ファイル再構成

	// CHostEntryが利用する外部API
	static void FASTCALL InitId() { g_nId = 0; }
										// 識別ID生成用カウンタ初期化

	// CWinFileSysが利用する外部API
	static void FASTCALL SetOption(DWORD nOption) { g_nOption = nOption; }
										// 動作フラグ設定

private:
	static inline int FASTCALL Compare(const BYTE* pFirst, const BYTE* pLast, const BYTE* pBufFirst, const BYTE* pBufLast);
										// 文字列比較 (ワイルドカード対応)

	DWORD m_nHumanUnit;
										// 該当エントリのHuman68kユニット番号
	BYTE* m_pszHuman;
										// 該当エントリのHuman68k内部名。末尾に / をつけない
	TCHAR* m_pszWin32;
										// 該当エントリの実体。末尾に / をつけない
	HANDLE m_hChange;
										// ファイル変更監視用ハンドル
	DWORD m_nId;
										// 識別ID (値が変化した場合は更新を意味する)
	CRing m_cRingFilename;
										// ファイル名 オブジェクト保持
										// 正しくディレクトリエントリ順に並べることを優先する

	static DWORD g_nId;
										// 識別ID生成用カウンタ

	static DWORD g_nOption;
										// 動作フラグ (パス名比較関連)
};

//===========================================================================
//
//	ディレクトリエントリ管理
//
//===========================================================================
class CHostEntry {
public:
	CHostEntry();
										// 初期化 (起動直後)
	virtual ~CHostEntry();
										// 開放 (終了時)
	void FASTCALL Init(CWinFileDrv** ppBase);
										// 初期化 (ドライバ組込み時)
	void FASTCALL Clean();
										// 開放 (起動・リセット時)

#ifdef XM6_HOST_STRICT_TIMEOUT
	// スレッド実体
	static DWORD WINAPI Run(VOID* pThis);
										// スレッド実行開始ポイント
	void FASTCALL Runner();
										// スレッド実体
#endif // XM6_HOST_STRICT_TIMEOUT

	void FASTCALL LockCache() { ASSERT(this); EnterCriticalSection(&m_csAccess); }
										// キャッシュアクセス開始
	void FASTCALL UnlockCache() { ASSERT(this); LeaveCriticalSection(&m_csAccess); }
										// キャッシュアクセス終了

	void FASTCALL CleanCache();
										// 全キャッシュを削除する
	void FASTCALL EraseCache(DWORD nUnit);
										// 該当するユニットのキャッシュを削除する
	void FASTCALL DeleteCache(CHostPath* pPath);
										// 該当するキャッシュを削除する
	CHostPath* FASTCALL FindCache(DWORD nUnit, const BYTE* szHuman);
										// 該当するパス名がキャッシュされているか検索する
	CHostPath* FASTCALL CopyCache(DWORD nUnit, const BYTE* szHuman, TCHAR* szWin32Buffer = NULL);
										// キャッシュ情報を元に、Win32名を獲得する
	CHostPath* FASTCALL MakeCache(CWindrv* ps, DWORD nUnit, const BYTE* szHuman, TCHAR* szWin32Buffer = NULL);
										// Win32名の検索に必要な情報をすべて取得する

	TCHAR* FASTCALL GetBase(DWORD nUnit) const;
										// ベースパス名を取得する
	BOOL FASTCALL isWriteProtect(CWindrv* ps) const;
										// 書き込み禁止かどうか確認する
	BOOL FASTCALL isMediaOffline(CWindrv* ps, BOOL bMedia = TRUE);
										// 低速メディアチェックとオフライン状態チェック
	BYTE FASTCALL GetMediaByte(DWORD nUnit) const;
										// メディアバイトの取得
	DWORD FASTCALL GetStatus(DWORD nUnit) const;
										// ドライブ状態の取得
	void FASTCALL CheckTimeout();
										// 全ドライブのタイムアウトチェック
#ifdef XM6_HOST_UPDATE_BY_SEQUENCE
	void FASTCALL SetMediaUpdate(CWindrv* ps, BOOL bDisable);
										// リムーバブルメディアの状態更新を有効にする
#endif // XM6_HOST_UPDATE_BY_SEQUENCE
	BOOL FASTCALL CheckMediaUpdate(CWindrv* ps);
										// リムーバブルメディアの状態更新チェック
	BOOL FASTCALL CheckMediaAccess(DWORD nUnit, BOOL bErase);
										// リムーバブルメディアのアクセス事前チェック
	BOOL FASTCALL Eject(DWORD nUnit);
										// イジェクト
	void FASTCALL GetVolume(DWORD nUnit, TCHAR* szLabel);
										// ボリュームラベルの取得
	BOOL FASTCALL GetVolumeCache(DWORD nUnit, TCHAR* szLabel);
										// キャッシュからボリュームラベルを取得
	DWORD FASTCALL GetCapacity(DWORD nUnit, Human68k::capacity_t* pCapacity);
										// 容量の取得
	BOOL FASTCALL GetCapacityCache(DWORD nUnit, Human68k::capacity_t* pCapacity);
										// キャッシュからクラスタサイズを取得

	// ファイルシステム状態通知用 外部API
	void FASTCALL ShellNotify(DWORD nEvent, const TCHAR* szPath);
										// ファイルシステム状態通知

private:
	static inline const BYTE* FASTCALL SeparatePath(const BYTE* szHuman);
										// Human68kフルパス名から最後の要素を分離
	static inline const BYTE* FASTCALL SeparateCopyFilename(const BYTE* szHuman, BYTE* szBuffer);
										// Human68kフルパス名から先頭の要素を分離・コピー

	CRITICAL_SECTION m_csAccess;
										// 排他制御用
	CWinFileDrv** m_ppBase;
										// Win32実体へアクセスする際のベースパス情報

#ifdef XM6_HOST_STRICT_TIMEOUT
	HANDLE m_hEvent;
										// スレッド停止イベント
	HANDLE m_hThread;
										// タイムアウトチェック用スレッド
#else // XM6_HOST_STRICT_TIMEOUT
	DWORD m_nTimeout;
										// 最後にタイムアウトチェックを行なった時刻
#endif // XM6_HOST_STRICT_TIMEOUT

	DWORD m_nRingPath;
										// 現在のパス名保持数
	CRing m_cRingPath;
										// パス名 オブジェクト保持
};

//===========================================================================
//
//	ファイル検索処理
//
//===========================================================================
class CHostFiles: public CRing {
public:
	CHostFiles() { Clear(); }
										// コンストラクタ
	virtual ~CHostFiles() { Free(); }
										// デストラクタ
	void FASTCALL Free() { ASSERT(this); Clear(); }
										// 再利用のための開放
	void FASTCALL Clear();
										// 初期化
	void FASTCALL SetKey(DWORD nKey) { ASSERT(this); m_nKey = nKey; }
										// 検索キー設定
	BOOL FASTCALL isSameKey(DWORD nKey) const { ASSERT(this); return m_nKey == nKey; }
										// 検索キー比較
	void FASTCALL SetPath(DWORD nUnit, const Human68k::namests_t* pNamests);
										// パス名・ファイル名を内部で生成
	BOOL FASTCALL isRootPath() const { return (strlen((char*)m_szHumanPath) == 1); }
										// ルートディレクトリ判定
	void FASTCALL SetPathWildcard() { m_nHumanWildcard = 1; }
										// ワイルドカードによるファイル検索を有効化
	void FASTCALL SetPathOnly() { m_nHumanWildcard = 0xFF; }
										// パス名のみを有効化
	void FASTCALL SetAttribute(DWORD nHumanAttribute) { m_nHumanAttribute = nHumanAttribute; }
										// 検索属性を設定
	BOOL FASTCALL Find(CWindrv* ps, CHostEntry* pEntry, BOOL bRemove = FALSE);
										// Win32名を検索 (パス名 + ファイル名(省略可) + 属性)
	void FASTCALL AddFilename();
										// Win32名にファイル名を追加
	const TCHAR* FASTCALL GetPath() const { ASSERT(this); return m_szWin32Result; }
										// Win32名を取得
	Human68k::dirent_t* FASTCALL GetEntry() const { ASSERT(this); return (Human68k::dirent_t*)&m_dirEntry; }
										// Human68kディレクトリエントリを取得
	DWORD FASTCALL GetAttribute() const { ASSERT(this); return m_dirEntry.attr; }
										// Human68k属性を取得
	WORD FASTCALL GetDate() const { ASSERT(this); return m_dirEntry.date; }
										// Human68k日付を取得
	WORD FASTCALL GetTime() const { ASSERT(this); return m_dirEntry.time; }
										// Human68k時刻を取得
	DWORD FASTCALL GetSize() const { ASSERT(this); return m_dirEntry.size; }
										// Human68kファイルサイズを取得
	const BYTE* FASTCALL GetHumanResult() const { ASSERT(this); return m_szHumanResult; }
										// Human68kファイル名検索結果を取得

private:
	DWORD m_nKey;
										// Human68kのFILESバッファアドレス 0なら未使用
	DWORD m_nHumanUnit;
										// Human68kのユニット番号
	BYTE m_szHumanPath[HUMAN68K_MAX_PATH];
										// Human68kのパス名
	BYTE m_szHumanFilename[24];
										// Human68kのファイル名
	DWORD m_nHumanWildcard;
										// Human68kのワイルドカード情報
	DWORD m_nHumanAttribute;
										// Human68kの検索属性
	CHostPath::find_t m_findNext;
										// 次回検索位置情報
	Human68k::dirent_t m_dirEntry;
										// 検索結果 Human68kファイル情報
	BYTE m_szHumanResult[24];
										// 検索結果 Human68kファイル名
	TCHAR m_szWin32Result[_MAX_PATH];
										// 検索結果 Win32フルパス名
};

//===========================================================================
//
//	ファイル検索領域 マネージャ
//
//===========================================================================
class CHostFilesManager {
public:
	CHostFilesManager();
										// ファイル検索領域 初期化 (起動直後)
	virtual ~CHostFilesManager();
										// ファイル検索領域 確認 (終了時)
	void FASTCALL Init();
										// ファイル検索領域 初期化 (ドライバ組込み時)
	void FASTCALL Clean();
										// ファイル検索領域 開放 (起動・リセット時)
	CHostFiles* FASTCALL Alloc(DWORD nKey);
										// ファイル検索領域 確保
	CHostFiles* FASTCALL Search(DWORD nKey);
										// ファイル検索領域 検索
	void FASTCALL Free(CHostFiles* p);
										// ファイル検索領域 開放
private:
	CRing m_cRingFiles;
										// ファイル検索領域 オブジェクト保持
};

//===========================================================================
//
//	FCB処理
//
//===========================================================================
class CHostFcb: public CRing {
public:
	CHostFcb() { Clear(); }
										// コンストラクタ
	virtual ~CHostFcb() { Free(); }
										// デストラクタ
	void FASTCALL Free() { ASSERT(this); Close(); Clear(); }
										// 再利用するための開放
	void FASTCALL Clear();
										// クリア
	void FASTCALL SetKey(DWORD nKey) { ASSERT(this); m_nKey = nKey; }
										// 検索キー設定
	BOOL FASTCALL SetOpenMode(DWORD nHumanMode);
										// ファイルオープンモードの設定
	void FASTCALL SetFilename(const TCHAR* szFilename);
										// ファイル名の設定
	HANDLE FASTCALL Create(DWORD nHumanAttribute, BOOL bForce);
										// ファイル作成
	HANDLE FASTCALL Open();
										// ファイルオープン/ハンドル獲得
	DWORD FASTCALL ReadFile(void* pBuffer, DWORD nSize);
										// ファイル読み込み
	DWORD FASTCALL WriteFile(void* pBuffer, DWORD nSize);
										// ファイル書き込み
	DWORD FASTCALL SetFilePointer(DWORD nOffset, DWORD nMode = FILE_BEGIN);
										// ファイルポインタ設定
	DWORD FASTCALL SetFileTime(WORD nFatDate, WORD nFatTime);
										// ファイル時刻設定
	BOOL FASTCALL Close();
										// ファイルクローズ
	BOOL FASTCALL isSameKey(DWORD nKey) const { ASSERT(this); return m_nKey == nKey; }
										// 検索キー比較

private:
	DWORD m_nKey;
										// Human68kのFCBバッファアドレス 0なら未使用
	DWORD m_nMode;
										// Win32のファイルオープンモード
	HANDLE m_hFile;
										// Win32のファイルハンドル
	TCHAR m_szFilename[_MAX_PATH];
										// Win32のファイル名
};

//===========================================================================
//
//	FCB処理 マネージャ
//
//===========================================================================
class CHostFcbManager {
public:
	CHostFcbManager();
										// FCB操作領域 初期化 (起動直後)
	virtual ~CHostFcbManager();
										// FCB操作領域 確認 (終了時)
	void FASTCALL Init();
										// FCB操作領域 初期化 (ドライバ組込み時)
	void FASTCALL Clean();
										// FCB操作領域 開放 (起動・リセット時)
	CHostFcb* FASTCALL Alloc(DWORD nKey);
										// FCB操作領域 確保
	CHostFcb* FASTCALL Search(DWORD nKey);
										// FCB操作領域 検索
	void FASTCALL Free(CHostFcb* p);
										// FCB操作領域 開放

private:
	CRing m_cRingFcb;
										// FCB操作領域 オブジェクト保持
};

//===========================================================================
//
//	Windowsファイルシステム
//
//===========================================================================
class CWinFileSys : public FileSys
{
public:
	// 基本ファンクション
	CWinFileSys();
										// コンストラクタ
	virtual ~CWinFileSys();
										// デストラクタ
	void FASTCALL ApplyCfg(const Config *pConfig);
										// 設定適用

	// 初期化・終了
	DWORD FASTCALL Init(CWindrv* ps, DWORD nDriveMax, const BYTE* pOption);
										// $40 - 初期化
	void FASTCALL Reset();
										// リセット(全クローズ)

	// コマンドハンドラ
	int FASTCALL CheckDir(CWindrv* ps, const Human68k::namests_t* pNamests);
										// $41 - ディレクトリチェック
	int FASTCALL MakeDir(CWindrv* ps, const Human68k::namests_t* pNamests);
										// $42 - ディレクトリ作成
	int FASTCALL RemoveDir(CWindrv* ps, const Human68k::namests_t* pNamests);
										// $43 - ディレクトリ削除
	int FASTCALL Rename(CWindrv* ps, const Human68k::namests_t* pNamests, const Human68k::namests_t* pNamestsNew);
										// $44 - ファイル名変更
	int FASTCALL Delete(CWindrv* ps, const Human68k::namests_t* pNamests);
										// $45 - ファイル削除
	int FASTCALL Attribute(CWindrv* ps, const Human68k::namests_t* pNamests, DWORD nHumanAttribute);
										// $46 - ファイル属性取得/設定
	int FASTCALL Files(CWindrv* ps, const Human68k::namests_t* pNamests, DWORD nKey, Human68k::files_t* pFiles);
										// $47 - ファイル検索(First)
	int FASTCALL NFiles(CWindrv* ps, DWORD nKey, Human68k::files_t* pFiles);
										// $48 - ファイル検索(Next)
	int FASTCALL Create(CWindrv* ps, const Human68k::namests_t* pNamests, DWORD nKey, Human68k::fcb_t* pFcb, DWORD nHumanAttribute, BOOL bForce);
										// $49 - ファイル作成
	int FASTCALL Open(CWindrv* ps, const Human68k::namests_t* pNamests, DWORD nKey, Human68k::fcb_t* pFcb);
										// $4A - ファイルオープン
	int FASTCALL Close(CWindrv* ps, DWORD nKey, Human68k::fcb_t* pFcb);
										// $4B - ファイルクローズ
	int FASTCALL Read(CWindrv* ps, DWORD nKey, Human68k::fcb_t* pFcb, DWORD nAddress, DWORD nSize);
										// $4C - ファイル読み込み
	int FASTCALL Write(CWindrv* ps, DWORD nKey, Human68k::fcb_t* pFcb, DWORD nAddress, DWORD nSize);
										// $4D - ファイル書き込み
	int FASTCALL Seek(CWindrv* ps, DWORD nKey, Human68k::fcb_t* pFcb, DWORD nMode, int nOffset);
										// $4E - ファイルシーク
	DWORD FASTCALL TimeStamp(CWindrv* ps, DWORD nKey, Human68k::fcb_t* pFcb, WORD nFatDate, WORD nFatTime);
										// $4F - ファイル時刻取得/設定
	int FASTCALL GetCapacity(CWindrv* ps, Human68k::capacity_t *pCapacity);
										// $50 - 容量取得
	int FASTCALL CtrlDrive(CWindrv* ps, Human68k::ctrldrive_t* pCtrlDrive);
										// $51 - ドライブ状態検査/制御
	int FASTCALL GetDPB(CWindrv* ps, Human68k::dpb_t* pDpb);
										// $52 - DPB取得
	int FASTCALL DiskRead(CWindrv* ps, DWORD nAddress, DWORD nSector, DWORD nSize);
										// $53 - セクタ読み込み
	int FASTCALL DiskWrite(CWindrv* ps, DWORD nAddress, DWORD nSector, DWORD nSize);
										// $54 - セクタ書き込み
	int FASTCALL IoControl(CWindrv* ps, Human68k::ioctrl_t* pIoctrl, DWORD nFunction);
										// $55 - IOCTRL
	int FASTCALL Flush(CWindrv* ps);
										// $56 - フラッシュ
	int FASTCALL CheckMedia(CWindrv* ps);
										// $57 - メディア交換チェック
	int FASTCALL Lock(CWindrv* ps);
										// $58 - 排他制御

	// ファイルシステム状態通知用 外部API
	void FASTCALL ShellNotify(DWORD nEvent, const TCHAR* szPath) { ASSERT(this); m_cEntry.ShellNotify(nEvent, szPath); }
										// ファイルシステム状態通知

	enum {
		DrvMax = 10						// ドライブ最大候補数
	};

private:
	// エラーハンドラ
	//DWORD FASTCALL GetLastError(DWORD nUnit) const;
										// Win32最終エラー取得 Human68kエラーに変換

	// 内部補助用
	DWORD FASTCALL GetOption() const { ASSERT(this); return m_nOption; }
										// オプション取得
	void FASTCALL SetOption(DWORD nOption);
										// オプション設定
	void FASTCALL InitOption(const BYTE* pOption);
										// オプション初期化
	inline BOOL FASTCALL FilesVolume(CWindrv* ps, Human68k::files_t* pFiles);
										// ボリュームラベル取得
	void FASTCALL CheckKernel(CWindrv* ps);
										// TwentyOneオプション監視

	DWORD m_bResume;
										// ベースパス状態復元有効 FALSEだとベースパスを毎回スキャン
	DWORD m_nDrives;
										// ドライブ候補数
	DWORD m_nFlag[DrvMax];
										// 動作フラグ候補
	TCHAR m_szBase[DrvMax][_MAX_PATH];
										// ベースパス候補

	CWinFileDrv* m_pDrv[DrvMax];
										// ドライブオブジェクト実体 CHostEntry内で利用

	CHostFilesManager m_cFiles;
										// ファイル検索領域 オブジェクト保持
	CHostFcbManager m_cFcb;
										// FCB操作領域 オブジェクト保持
	CHostEntry m_cEntry;
										// ディレクトリエントリ オブジェクト保持
										// これらは1つづつ存在すればよいためnewせず実体を置く

	DWORD m_nHostSectorCount;
										// 擬似セクタ番号
	DWORD m_nHostSectorBuffer[XM6_HOST_PSEUDO_CLUSTER_MAX];
										// 擬似セクタの指すファイル実体

	DWORD m_nKernel;
										// TwentyOneオプションアドレス / 検索開始カウント
	DWORD m_nKernelSearch;
										// NULデバイスの先頭アドレス

	DWORD m_nOptionDefault;
										// リセット時の動作フラグ
	DWORD m_nOption;
										// 動作フラグ (ファイル削除関連)
};

//===========================================================================
//
//	Host
//
//===========================================================================
class CHost : public CComponent
{
public:
	// 基本ファンクション
	CHost(CFrmWnd *pWnd);
										// コンストラクタ
	BOOL FASTCALL Init();
										// 初期化
	void FASTCALL Cleanup();
										// クリーンアップ
	void FASTCALL ApplyCfg(const Config *pConfig);
										// 設定適用

	// ファイルシステム状態通知用 外部API
	void FASTCALL ShellNotify(DWORD nEvent, const TCHAR* szPath) { ASSERT(this); m_WinFileSys.ShellNotify(nEvent, szPath); }
										// ファイルシステム状態通知

private:
	CWinFileSys m_WinFileSys;
										// ファイルシステム
	Windrv *m_pWindrv;
										// Windrv
};

#endif // mfc_host_h
#endif // _WIN32
