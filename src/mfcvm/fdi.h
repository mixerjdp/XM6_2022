//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2005 ＰＩ．(ytanaka@ipc-tokai.or.jp)
//	[ フロッピーディスクイメージ ]
//
//---------------------------------------------------------------------------

#if !defined(fdi_h)
#define fdi_h

#include "filepath.h"

//---------------------------------------------------------------------------
//
//	クラス先行定義
//
//---------------------------------------------------------------------------
class FDI;
class FDIDisk;
class FDITrack;
class FDISector;

class FDIDisk2HD;
class FDITrack2HD;
class FDIDiskDIM;
class FDITrackDIM;
class FDIDiskD68;
class FDITrackD68;
class FDIDiskBAD;
class FDITrackBAD;
class FDIDisk2DD;
class FDITrack2DD;
class FDIDisk2HQ;
class FDITrack2HQ;

//---------------------------------------------------------------------------
//
//	物理フォーマット定義
//
//---------------------------------------------------------------------------
#define FDI_2HD			0x00			// 2HD
#define FDI_2HDA		0x01			// 2HDA
#define FDI_2HS			0x02			// 2HS
#define FDI_2HC			0x03			// 2HC
#define FDI_2HDE		0x04			// 2HDE
#define FDI_2HQ			0x05			// 2HQ
#define FDI_N88B		0x06			// N88-BASIC
#define FDI_OS9			0x07			// OS-9/68000
#define FDI_2DD			0x08			// 2DD

//===========================================================================
//
//	FDIセクタ
//
//===========================================================================
class FDISector
{
public:
	// 内部データ定義
	typedef struct {
		DWORD chrn[4];					// CHRN
		BOOL mfm;						// MFMフラグ
		int error;						// エラーコード
		int length;						// データ長
		int gap3;						// GAP3
		BYTE *buffer;					// データバッファ
		DWORD pos;						// ポジション
		BOOL changed;					// 変更済みフラグ
		FDISector *next;				// 次のセクタ
	} sector_t;

public:
	// 基本ファンクション
	FDISector(BOOL mfm, const DWORD *chrn);
										// コンストラクタ
	virtual ~FDISector();
										// デストラクタ
	void FASTCALL Load(const BYTE *buf, int len, int gap, int err);
										// 初期ロード

	// リード・ライト
	BOOL FASTCALL IsMatch(BOOL mfm, const DWORD *chrn) const;
										// セクタマッチするか
	void FASTCALL GetCHRN(DWORD *chrn)	const;
										// CHRNを取得
	BOOL FASTCALL IsMFM() const			{ return sec.mfm; }
										// MFMか
	int FASTCALL Read(BYTE *buf) const;
										// リード
	int FASTCALL Write(const BYTE *buf, BOOL deleted);
										// ライト
	int FASTCALL Fill(DWORD d);
										// フィル

	// プロパティ
	const BYTE* FASTCALL GetSector() const	{ return sec.buffer; }
										// セクタデータ取得
	int FASTCALL GetLength() const		{ return sec.length; }
										// データ長取得
	int FASTCALL GetError() const		{ return sec.error; }
										// エラーコード取得
	int FASTCALL GetGAP3() const		{ return sec.gap3; }
										// GAP3バイト数取得

	// ポジション
	void FASTCALL SetPos(DWORD pos)		{  sec.pos = pos; }
										// ポジション設定
	DWORD FASTCALL GetPos() const		{ return sec.pos; }
										// ポジション取得

	// 変更フラグ
	BOOL FASTCALL IsChanged() const		{ return sec.changed; }
										// 変更フラグチェック
	void FASTCALL ClrChanged()			{ sec.changed = FALSE; }
										// 変更フラグを落とす
	void FASTCALL ForceChanged()		{ sec.changed = TRUE; }
										// 変更フラグを上げる

	// インデックス・リンク
	void FASTCALL SetNext(FDISector *next)	{ sec.next = next; }
										// 次のセクタを設定
	FDISector* FASTCALL GetNext() const	{ return sec.next; }
										// 次のセクタを取得

private:
	// 内部データ
	sector_t sec;
										// セクタ内部データ
};

//===========================================================================
//
//	FDIトラック
//
//===========================================================================
class FDITrack
{
public:
	// 内部ワーク
	typedef struct {
		FDIDisk *disk;					// 親ディスク
		int track;						// トラック
		BOOL init;						// ロード済みか
		int sectors[3];					// 所有セクタ数(ALL/FM/MFM)
		BOOL hd;						// 密度フラグ
		BOOL mfm;						// 先頭セクタMFMフラグ
		FDISector *first;				// 最初のセクタ
		FDITrack *next;					// 次のトラック
	} track_t;

public:
	// 基本ファンクション
	FDITrack(FDIDisk *disk, int track, BOOL hd = TRUE);
										// コンストラクタ
	virtual ~FDITrack();
										// デストラクタ
	virtual BOOL FASTCALL Save(Fileio *fio, DWORD offset);
										// セーブ
	virtual BOOL FASTCALL Save(const Filepath& path, DWORD offset);
										// セーブ
	void FASTCALL Create(DWORD phyfmt);
										// 物理フォーマット
	BOOL FASTCALL IsHD() const			{ return trk.hd; }
										// HDフラグ取得

	// リード・ライト
	int FASTCALL ReadID(DWORD *buf, BOOL mfm);
										// リードID
	int FASTCALL ReadSector(BYTE *buf, int *len, BOOL mfm, const DWORD *chrn);
										// リードセクタ
	int FASTCALL WriteSector(const BYTE *buf, int *len, BOOL mfm, const DWORD *chrn, BOOL deleted);
										// ライトセクタ
	int FASTCALL ReadDiag(BYTE *buf, int *len, BOOL mfm, const DWORD *chrn);
										// リードダイアグ
	virtual int FASTCALL WriteID(const BYTE *buf, DWORD d, int sc, BOOL mfm, int gpl);
										// ライトID

	// インデックス・リンク
	int FASTCALL GetTrack() const		{ return trk.track; }
										// このトラックを取得
	void FASTCALL SetNext(FDITrack *p)	{ trk.next = p; }
										// 次のトラックを設定
	FDITrack* FASTCALL GetNext() const	{ return trk.next; }
										// 次のトラックを取得

	// セクタ
	BOOL FASTCALL IsChanged() const;
										// 変更フラグチェック
	DWORD FASTCALL GetTotalLength() const;
										// セクタレングス累計算出
	void FASTCALL ForceChanged();
										// 強制変更
	FDISector* FASTCALL Search(BOOL mfm, const DWORD *chrn);
										// 条件に合うセクタをサーチ
	FDISector* FASTCALL GetFirst() const{ return trk.first; }
										// 最初のセクタを取得

protected:
	// 物理フォーマット
	void FASTCALL Create2HD(int lim);
										// 物理フォーマット(2HD)
	void FASTCALL Create2HS();
										// 物理フォーマット(2HS)
	void FASTCALL Create2HC();
										// 物理フォーマット(2HC)
	void FASTCALL Create2HDE();
										// 物理フォーマット(2HDE)
	void FASTCALL Create2HQ();
										// 物理フォーマット(2HQ)
	void FASTCALL CreateN88B();
										// 物理フォーマット(N88-BASIC)
	void FASTCALL CreateOS9();
										// 物理フォーマット(OS-9)
	void FASTCALL Create2DD();
										// 物理フォーマット(2DD)

	// ディスク、回転管理
	FDIDisk* FASTCALL GetDisk() const	{ return trk.disk; }
										// ディスク取得
	BOOL FASTCALL IsMFM() const			{ return trk.mfm; }
										// 先頭セクタがMFMか
	int FASTCALL GetGAP1() const;
										// GAP1長さ取得
	int FASTCALL GetTotal() const;
										// トータル長さ取得
	void FASTCALL CalcPos();
										// セクタ先頭の位置を算出
	DWORD FASTCALL GetSize(FDISector *sector) const;
										// セクタのサイズ(ID,GAP3含む)を取得
	FDISector* FASTCALL GetCurSector() const;
										// カレント位置以降の最初のセクタを取得

	// トラック
	BOOL IsInit() const					{ return trk.init; }

	// セクタ
	void FASTCALL AddSector(FDISector *sector);
										// セクタ追加
	void FASTCALL ClrSector();
										// セクタ全削除
	int FASTCALL GetAllSectors() const	{ return trk. sectors[0]; }
										// セクタ数取得(All)
	int FASTCALL GetMFMSectors() const	{ return trk.sectors[1]; }
										// セクタ数取得(MFM)
	int FASTCALL GetFMSectors() const	{ return trk.sectors[2]; }
										// セクタ数取得(FM)

	// ダイアグ
	int FASTCALL MakeGAP1(BYTE *buf, int offset) const;
										// GAP1作成
	int FASTCALL MakeSector(BYTE *buf, int offset, FDISector *sector) const;
										// セクタデータ作成
	int FASTCALL MakeGAP4(BYTE *buf, int offset) const;
										// GAP4作成
	int FASTCALL MakeData(BYTE *buf, int offset, BYTE data, int length) const;
										// Diagデータ作成
	WORD FASTCALL CalcCRC(BYTE *buf, int length) const;
										// CRC算出
	static const WORD CRCTable[0x100];
										// CRC算出テーブル

	// 内部データ
	track_t trk;
										// トラック内部データ
};

//===========================================================================
//
//	FDIディスク
//
//===========================================================================
class FDIDisk
{
public:
	// 新規オプション定義
	typedef struct {
		DWORD phyfmt;					// 物理フォーマット種別
		BOOL logfmt;					// 論理フォーマット有無
		char name[60];					// コメント(DIM)/ディスク名(D68)
	} option_t;

	// 内部データ定義
	typedef struct {
		int index;						// インデックス
		FDI *fdi;						// 親FDI
		DWORD id;						// ID
		BOOL writep;					// 書き込み禁止
		BOOL readonly;					// 読み込み専用
		char name[60];					// ディスク名
		Filepath path;					// パス
		DWORD offset;					// ファイルのオフセット
		FDITrack *first;				// 最初のトラック
		FDITrack *head[2];				// ヘッドに対応したトラック
		int search;						// 検索時間(１周=0x10000)
		FDIDisk *next;					// 次のディスク
	} disk_t;

public:
	FDIDisk(int index, FDI *fdi);
										// コンストラクタ
	virtual ~FDIDisk();
										// デストラクタ
	DWORD FASTCALL GetID() const		{ return disk.id; }
										// ID取得

	// メディア操作
	virtual BOOL FASTCALL Create(const Filepath& path, const option_t *opt);
										// 新規ディスク作成
	virtual BOOL FASTCALL Open(const Filepath& path, DWORD offset = 0);
										// オープン
	void FASTCALL GetName(char *buf) const;
										// ディスク名取得
	void FASTCALL GetPath(Filepath& path) const;
										// パス取得
	BOOL FASTCALL IsWriteP() const		{ return disk.writep; }
										// ライトプロテクトか
	BOOL FASTCALL IsReadOnly() const	{ return disk.readonly; }
										// Read Onlyディスクイメージか
	void FASTCALL WriteP(BOOL flag);
										// 書き込み禁止をセット
	virtual BOOL FASTCALL Flush();
										// バッファをフラッシュ

	// アクセス
	virtual void FASTCALL Seek(int c);
										// シーク
	int FASTCALL ReadID(DWORD *buf, BOOL mfm, int hd);
										// リードID
	int FASTCALL ReadSector(BYTE *buf, int *len, BOOL mfm, const DWORD *chrn, int hd);
										// リードセクタ
	int FASTCALL WriteSector(const BYTE *buf, int *len, BOOL mfm, const DWORD *chrn, int hd, BOOL deleted);
										// ライトセクタ
	int FASTCALL ReadDiag(BYTE *buf, int *len, BOOL mfm, const DWORD *chrn, int hd);
										// リードダイアグ
	int FASTCALL WriteID(const BYTE *buf, DWORD d, int sc, BOOL mfm, int hd, int gpl);
										// ライトID

	// 回転管理
	DWORD FASTCALL GetRotationPos() const;
										// 回転位置取得
	DWORD FASTCALL GetRotationTime() const;
										// 回転時間取得
	DWORD FASTCALL GetSearch() const	{ return disk.search; }
										// 検索長さ取得
	void FASTCALL SetSearch(DWORD len)	{ disk.search = len; }
										// 検索長さ設定
	void FASTCALL CalcSearch(DWORD pos);
										// 検索長さ算出
	BOOL FASTCALL IsHD() const;
										// ドライブHD状態取得
	FDITrack* FASTCALL Search(int track) const;
										// トラックサーチ

	// インデックス・リンク
	int FASTCALL GetIndex() const		{ return disk.index; }
										// インデックス取得
	void FASTCALL SetNext(FDIDisk *p)	{ disk.next = p; }
										// 次のトラックを設定
	FDIDisk* FASTCALL GetNext() const	{ return disk.next; }
										// 次のトラックを取得

protected:
	// 論理フォーマット
	void FASTCALL Create2HD(BOOL flag2hd);
										// 論理フォーマット(2HD, 2HDA)
	static const BYTE IPL2HD[0x200];
										// IPL(2HD, 2HDA)
	void FASTCALL Create2HS();
										// 論理フォーマット(2HS)
	static const BYTE IPL2HS[0x800];
										// IPL(2HS)
	void FASTCALL Create2HC();
										// 論理フォーマット(2HC)
	static const BYTE IPL2HC[0x200];
										// IPL(2HC)
	void FASTCALL Create2HDE();
										// 論理フォーマット(2HDE)
	static const BYTE IPL2HDE[0x800];
										// IPL(2HDE)
	void FASTCALL Create2HQ();
										// 論理フォーマット(2HQ)
	static const BYTE IPL2HQ[0x200];
										// IPL(2HQ)
	void FASTCALL Create2DD();
										// 論理フォーマット(2DD)

	// イメージ
	FDI* FASTCALL GetFDI() const		{ return disk.fdi; }
										// 親イメージ取得

	// トラック
	void FASTCALL AddTrack(FDITrack *track);
										// トラック追加
	void FASTCALL ClrTrack();
										// トラック全削除
	FDITrack* FASTCALL GetFirst() const	{ return disk.first; }
										// 最初のトラックを取得
	FDITrack* FASTCALL GetHead(int idx) { ASSERT((idx==0)||(idx==1)); return disk.head[idx]; }
										// ヘッドに対応するトラックを取得

	// 内部データ
	disk_t disk;
										// ディスク内部データ
};

//===========================================================================
//
//	FDI
//
//===========================================================================
class FDI
{
public:
	// 内部データ定義
	typedef struct {
		FDD *fdd;						// FDD
		int disks;						// ディスク数
		FDIDisk *first;					// 最初のディスク
		FDIDisk *disk;					// 現在のディスク
	} fdi_t;

public:
	FDI(FDD *fdd);
										// コンストラクタ
	virtual ~FDI();
										// デストラクタ

	// メディア操作
	BOOL FASTCALL Open(const Filepath& path, int media);
										// オープン
	DWORD FASTCALL GetID() const;
										// ID取得
	BOOL FASTCALL IsMulti() const;
										// マルチディスクイメージか
	FDIDisk* GetDisk() const			{ return fdi.disk; }
										// 現在のディスクを取得
	int FASTCALL GetDisks() const		{ return fdi.disks; }
										// ディスク数を取得
	int FASTCALL GetMedia() const;
										// マルチディスクインデックス取得
	void FASTCALL GetName(char *buf, int index = -1) const;
										// ディスク名取得
	void FASTCALL GetPath(Filepath& path) const;
										// パス取得
	BOOL FASTCALL Save(Fileio *fio, int ver);
										// セーブ
	BOOL FASTCALL Load(Fileio *fio, int ver, BOOL *ready, int *media, Filepath& path);
										// ロード
	void FASTCALL Adjust();
										// 調整(特殊)

	// メディア状態
	BOOL FASTCALL IsReady() const;
										// メディアがセットされているか
	BOOL FASTCALL IsWriteP() const;
										// ライトプロテクトか
	BOOL FASTCALL IsReadOnly() const;
										// Read Onlyディスクイメージか
	void FASTCALL WriteP(BOOL flag);
										// 書き込み禁止をセット

	// アクセス
	void FASTCALL Seek(int c);
										// シーク
	int FASTCALL ReadID(DWORD *buf, BOOL mfm, int hd);
										// リードID
	int FASTCALL ReadSector(BYTE *buf, int *len, BOOL mfm, const DWORD *chrn, int hd);
										// リードセクタ
	int FASTCALL WriteSector(const BYTE *buf, int *len, BOOL mfm, const DWORD *chrn, int hd, BOOL deleted);
										// ライトセクタ
	int FASTCALL ReadDiag(BYTE *buf, int *len, BOOL mfm, const DWORD *chrn, int hd);
										// リードダイアグ
	int FASTCALL WriteID(const BYTE *buf, DWORD d, int sc, BOOL mfm, int hd, int gpl);
										// ライトID

	// 回転管理
	DWORD FASTCALL GetRotationPos() const;
										// 回転位置取得
	DWORD FASTCALL GetRotationTime() const;
										// 回転時間取得
	DWORD FASTCALL GetSearch() const;
										// 検索時間取得
	BOOL FASTCALL IsHD() const;
										// ドライブHD状態取得

private:
	// ドライブ
	FDD* FASTCALL GetFDD() const		{ return fdi.fdd; }

	// ディスク
	void FASTCALL AddDisk(FDIDisk *disk);
										// ディスク追加
	void FASTCALL ClrDisk();
										// ディスク全削除
	FDIDisk* GetFirst() const			{ return fdi.first; }
										// 最初のディスクを取得
	FDIDisk* FASTCALL Search(int index) const;
										// ディスクサーチ

	// 内部データ
	fdi_t fdi;
										// FDI内部データ
};

//===========================================================================
//
//	FDIトラック(2HD)
//
//===========================================================================
class FDITrack2HD : public FDITrack
{
public:
	// 基本ファンクション
	FDITrack2HD(FDIDisk *disk, int track);
										// コンストラクタ
	BOOL FASTCALL Load(const Filepath& path, DWORD offset);
										// ロード
};

//===========================================================================
//
//	FDIディスク(2HD)
//
//===========================================================================
class FDIDisk2HD : public FDIDisk
{
public:
	FDIDisk2HD(int index, FDI *fdi);
										// コンストラクタ
	virtual ~FDIDisk2HD();
										// デストラクタ
	BOOL FASTCALL Open(const Filepath& path, DWORD offset = 0);
										// オープン
	void FASTCALL Seek(int c);
										// シーク
	BOOL FASTCALL Create(const Filepath& path, const option_t *opt);
										// 新規ディスク作成
	BOOL FASTCALL Flush();
										// バッファをフラッシュ
};

//===========================================================================
//
//	FDIトラック(DIM)
//
//===========================================================================
class FDITrackDIM : public FDITrack
{
public:
	// 基本ファンクション
	FDITrackDIM(FDIDisk *disk, int track, int type);
										// コンストラクタ
	BOOL FASTCALL Load(const Filepath& path, DWORD offset, BOOL load);
										// ロード
	BOOL FASTCALL IsDIMMFM() const		{ return dim_mfm; }
										// DIM MFMフラグ取得
	int FASTCALL GetDIMSectors() const	{ return dim_secs; }
										// DIM セクタ数取得
	int FASTCALL GetDIMN() const		{ return dim_n; }
										// DIM レングス取得

private:
	BOOL dim_mfm;
										// DIM MFMフラグ
	int dim_secs;
										// DIM セクタ数
	int dim_n;
										// DIM レングス
	int dim_type;
										// DIM タイプ
};

//===========================================================================
//
//	FDIディスク(DIM)
//
//===========================================================================
class FDIDiskDIM : public FDIDisk
{
public:
	FDIDiskDIM(int index, FDI *fdi);
										// コンストラクタ
	virtual ~FDIDiskDIM();
										// デストラクタ
	BOOL FASTCALL Open(const Filepath& path, DWORD offset = 0);
										// オープン
	void FASTCALL Seek(int c);
										// シーク
	BOOL FASTCALL Create(const Filepath& path, const option_t *opt);
										// 新規ディスク作成
	BOOL FASTCALL Flush();
										// バッファをフラッシュ

private:
	BOOL FASTCALL GetDIMMap(int track) const;
										// トラックマップ取得
	DWORD FASTCALL GetDIMOffset(int track) const;
										// トラックオフセット取得
	BOOL FASTCALL Save();
										// 削除前の保存
	BYTE dim_hdr[0x100];
										// DIMヘッダ
	BOOL dim_load;
										// ヘッダ確認フラグ
};

//===========================================================================
//
//	FDIトラック(D68)
//
//===========================================================================
class FDITrackD68 : public FDITrack
{
public:
	// 基本ファンクション
	FDITrackD68(FDIDisk *disk, int track, BOOL hd);
										// コンストラクタ
	BOOL FASTCALL Load(const Filepath& path, DWORD offset);
										// ロード
	BOOL FASTCALL Save(const Filepath& path, DWORD offset);
										// セーブ
	BOOL FASTCALL Save(Fileio *fio, DWORD offset);
										// セーブ
	int FASTCALL WriteID(const BYTE *buf, DWORD d, int sc, BOOL mfm, int gpl);
										// ライトID
	void FASTCALL ForceFormat()			{ d68_format = TRUE; }
										// 強制フォーマット
	BOOL FASTCALL IsFormated() const	{ return d68_format; }
										// フォーマット変更されているか
	DWORD FASTCALL GetD68Length() const;
										// D68形式での長さを取得

private:
	BOOL d68_format;
										// フォーマット変更フラグ
	static const int Gap3Table[];
										// GAP3テーブル
};

//===========================================================================
//
//	FDIディスク(D68)
//
//===========================================================================
class FDIDiskD68 : public FDIDisk
{
public:
	FDIDiskD68(int index, FDI *fdi);
										// コンストラクタ
	virtual ~FDIDiskD68();
										// デストラクタ
	BOOL FASTCALL Open(const Filepath& path, DWORD offset = 0);
										// オープン
	void FASTCALL Seek(int c);
										// シーク
	void FASTCALL AdjustOffset();
										// オフセット更新
	static int FASTCALL CheckDisks(const Filepath& path, DWORD *offbuf);
										// D68ヘッダチェック
	BOOL FASTCALL Create(const Filepath& path, const option_t *opt);
										// 新規ディスク作成
	BOOL FASTCALL Flush();
										// バッファをフラッシュ

private:
	DWORD FASTCALL GetD68Offset(int track) const;
										// トラックオフセット取得
	BOOL FASTCALL Save();
										// 削除前の保存
	BYTE d68_hdr[0x2b0];
										// D68ヘッダ
	BOOL d68_load;
										// ヘッダ確認フラグ
};

//===========================================================================
//
//	FDIトラック(BAD)
//
//===========================================================================
class FDITrackBAD : public FDITrack
{
public:
	// 基本ファンクション
	FDITrackBAD(FDIDisk *disk, int track);
										// コンストラクタ
	BOOL FASTCALL Load(const Filepath& path, DWORD offset);
										// ロード
	BOOL FASTCALL Save(const Filepath& path, DWORD offset);
										// セーブ

private:
	int bad_secs;
										// セクタ数
};

//===========================================================================
//
//	FDIディスク(BAD)
//
//===========================================================================
class FDIDiskBAD : public FDIDisk
{
public:
	FDIDiskBAD(int index, FDI *fdi);
										// コンストラクタ
	virtual ~FDIDiskBAD();
										// デストラクタ
	BOOL FASTCALL Open(const Filepath& path, DWORD offset = 0);
										// オープン
	void FASTCALL Seek(int c);
										// シーク
	BOOL FASTCALL Flush();
										// バッファをフラッシュ
};

//===========================================================================
//
//	FDIトラック(2DD)
//
//===========================================================================
class FDITrack2DD : public FDITrack
{
public:
	// 基本ファンクション
	FDITrack2DD(FDIDisk *disk, int track);
										// コンストラクタ
	BOOL FASTCALL Load(const Filepath& path, DWORD offset);
										// ロード
};

//===========================================================================
//
//	FDIディスク(2DD)
//
//===========================================================================
class FDIDisk2DD : public FDIDisk
{
public:
	FDIDisk2DD(int index, FDI *fdi);
										// コンストラクタ
	virtual ~FDIDisk2DD();
										// デストラクタ
	BOOL FASTCALL Open(const Filepath& path, DWORD offset = 0);
										// オープン
	void FASTCALL Seek(int c);
										// シーク
	BOOL FASTCALL Create(const Filepath& path, const option_t *opt);
										// 新規ディスク作成
	BOOL FASTCALL Flush();
										// バッファをフラッシュ
};

//===========================================================================
//
//	FDIトラック(2HQ)
//
//===========================================================================
class FDITrack2HQ : public FDITrack
{
public:
	// 基本ファンクション
	FDITrack2HQ(FDIDisk *disk, int track);
										// コンストラクタ
	BOOL FASTCALL Load(const Filepath& path, DWORD offset);
										// ロード
};

//===========================================================================
//
//	FDIディスク(2HQ)
//
//===========================================================================
class FDIDisk2HQ : public FDIDisk
{
public:
	FDIDisk2HQ(int index, FDI *fdi);
										// コンストラクタ
	virtual ~FDIDisk2HQ();
										// デストラクタ
	BOOL FASTCALL Open(const Filepath& path, DWORD offset = 0);
										// オープン
	void FASTCALL Seek(int c);
										// シーク
	BOOL FASTCALL Create(const Filepath& path, const option_t *opt);
										// 新規ディスク作成
	BOOL FASTCALL Flush();
										// バッファをフラッシュ
};

#endif	// fdi_h
