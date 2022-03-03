//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 ＰＩ．(ytanaka@ipc-tokai.or.jp)
//	[ ディスク ]
//
//---------------------------------------------------------------------------

#include "os.h"
#include "xm6.h"
#include "fileio.h"
#include "vm.h"
#include "sasi.h"
#include "disk.h"

//===========================================================================
//
//	ディスクトラック
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	コンストラクタ
//
//---------------------------------------------------------------------------
DiskTrack::DiskTrack(int track, int size, int sectors, BOOL raw)
{
	ASSERT(track >= 0);
	ASSERT((size == 8) || (size == 9) || (size == 11));
	ASSERT((sectors > 0) && (sectors <= 0x100));

	// パラメータを設定
	dt.track = track;
	dt.size = size;
	dt.sectors = sectors;
	dt.raw = raw;

	// 初期化されていない(ロードする必要あり)
	dt.init = FALSE;

	// 変更されていない
	dt.changed = FALSE;

	// 動的ワークは存在しない
	dt.buffer = NULL;
	dt.changemap = NULL;
}

//---------------------------------------------------------------------------
//
//	デストラクタ
//
//---------------------------------------------------------------------------
DiskTrack::~DiskTrack()
{
	// メモリ解放は行うが、自動セーブはしない
	if (dt.buffer) {
		delete[] dt.buffer;
		dt.buffer = NULL;
	}
	if (dt.changemap) {
		delete[] dt.changemap;
		dt.changemap = NULL;
	}
}

//---------------------------------------------------------------------------
//
//	ロード
//
//---------------------------------------------------------------------------
BOOL FASTCALL DiskTrack::Load(const Filepath& path)
{
	Fileio fio;
	DWORD offset;
	int i;
	int length;

	ASSERT(this);

	// 既にロードされていれば不要
	if (dt.init) {
		ASSERT(dt.buffer);
		ASSERT(dt.changemap);
		return TRUE;
	}

	ASSERT(!dt.buffer);
	ASSERT(!dt.changemap);

	// オフセットを計算(これ以前のトラックは256セクタ保持とみなす)
	offset = (dt.track << 8);
	if (dt.raw) {
		ASSERT(dt.size == 11);
		offset *= 0x930;
		offset += 0x10;
	}
	else {
		offset <<= dt.size;
	}

	// レングスを計算(このトラックのデータサイズ)
	length = dt.sectors << dt.size;

	// バッファのメモリを確保
	ASSERT((dt.size == 8) || (dt.size == 9) || (dt.size == 11));
	ASSERT((dt.sectors > 0) && (dt.sectors <= 0x100));
	try {
		dt.buffer = new BYTE[ length ];
	}
	catch (...) {
		dt.buffer = NULL;
		return FALSE;
	}
	if (!dt.buffer) {
		return FALSE;
	}

	// 変更マップのメモリを確保
	try {
		dt.changemap = new BOOL[dt.sectors];
	}
	catch (...) {
		dt.changemap = NULL;
		return FALSE;
	}
	if (!dt.changemap) {
		return FALSE;
	}

	// 変更マップをクリア
	for (i=0; i<dt.sectors; i++) {
		dt.changemap[i] = FALSE;
	}

	// ファイルから読み込む
	if (!fio.Open(path, Fileio::ReadOnly)) {
		return FALSE;
	}
	if (dt.raw) {
		// 分割読み
		for (i=0; i<dt.sectors; i++) {
			// シーク
			if (!fio.Seek(offset)) {
				fio.Close();
				return FALSE;
			}

			// 読み込み
			if (!fio.Read(&dt.buffer[i << dt.size], 1 << dt.size)) {
				fio.Close();
				return FALSE;
			}

			// 次のオフセット
			offset += 0x930;
		}
	}
	else {
		// 連続読み
		if (!fio.Seek(offset)) {
			fio.Close();
			return FALSE;
		}
		if (!fio.Read(dt.buffer, length)) {
			fio.Close();
			return FALSE;
		}
	}
	fio.Close();

	// フラグを立て、正常終了
	dt.init = TRUE;
	dt.changed = FALSE;
	return TRUE;
}

//---------------------------------------------------------------------------
//
//	セーブ
//
//---------------------------------------------------------------------------
BOOL FASTCALL DiskTrack::Save(const Filepath& path)
{
	DWORD offset;
	int i;
	Fileio fio;
	int length;

	ASSERT(this);

	// 初期化されていなければ不要
	if (!dt.init) {
		return TRUE;
	}

	// 変更されていなければ不要
	if (!dt.changed) {
		return TRUE;
	}

	// 書き込む必要がある
	ASSERT(dt.buffer);
	ASSERT(dt.changemap);
	ASSERT((dt.size == 8) || (dt.size == 9) || (dt.size == 11));
	ASSERT((dt.sectors > 0) && (dt.sectors <= 0x100));

	// RAWモードでは書き込みはありえない
	ASSERT(!dt.raw);

	// オフセットを計算(これ以前のトラックは256セクタ保持とみなす)
	offset = (dt.track << 8);
	offset <<= dt.size;

	// セクタあたりのレングスを計算
	length = 1 << dt.size;

	// ファイルオープン
	if (!fio.Open(path, Fileio::ReadWrite)) {
		return FALSE;
	}

	// 書き込みループ
	for (i=0; i<dt.sectors; i++) {
		// 変更されていれば
		if (dt.changemap[i]) {
			// シーク、書き込み
			if (!fio.Seek(offset + (i << dt.size))) {
				fio.Close();
				return FALSE;
			}
			if (!fio.Write(&dt.buffer[i << dt.size], length)) {
				fio.Close();
				return FALSE;
			}

			// 変更フラグを落とす
			dt.changemap[i] = FALSE;
		}
	}

	// クローズ
	fio.Close();

	// 変更フラグを落とし、終了
	dt.changed = FALSE;
	return TRUE;
}

//---------------------------------------------------------------------------
//
//	リードセクタ
//
//---------------------------------------------------------------------------
BOOL FASTCALL DiskTrack::Read(BYTE *buf, int sec) const
{
	ASSERT(this);
	ASSERT(buf);
	ASSERT((sec >= 0) & (sec < 0x100));

	// 初期化されていなければエラー
	if (!dt.init) {
		return FALSE;
	}

	// セクタが有効数を超えていればエラー
	if (sec >= dt.sectors) {
		return FALSE;
	}

	// コピー
	ASSERT(dt.buffer);
	ASSERT((dt.size == 8) || (dt.size == 9) || (dt.size == 11));
	ASSERT((dt.sectors > 0) && (dt.sectors <= 0x100));
	memcpy(buf, &dt.buffer[sec << dt.size], 1 << dt.size);

	// 成功
	return TRUE;
}

//---------------------------------------------------------------------------
//
//	ライトセクタ
//
//---------------------------------------------------------------------------
BOOL FASTCALL DiskTrack::Write(const BYTE *buf, int sec)
{
	int offset;
	int length;

	ASSERT(this);
	ASSERT(buf);
	ASSERT((sec >= 0) & (sec < 0x100));
	ASSERT(!dt.raw);

	// 初期化されていなければエラー
	if (!dt.init) {
		return FALSE;
	}

	// セクタが有効数を超えていればエラー
	if (sec >= dt.sectors) {
		return FALSE;
	}

	// オフセット、レングスを計算
	offset = sec << dt.size;
	length = 1 << dt.size;

	// 比較
	ASSERT(dt.buffer);
	ASSERT((dt.size == 8) || (dt.size == 9) || (dt.size == 11));
	ASSERT((dt.sectors > 0) && (dt.sectors <= 0x100));
	if (memcmp(buf, &dt.buffer[offset], length) == 0) {
		// 同じものを書き込もうとしているので、正常終了
		return TRUE;
	}

	// コピー、変更あり
	memcpy(&dt.buffer[offset], buf, length);
	dt.changemap[sec] = TRUE;
	dt.changed = TRUE;

	// 成功
	return TRUE;
}

//===========================================================================
//
//	ディスクキャッシュ
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	コンストラクタ
//
//---------------------------------------------------------------------------
DiskCache::DiskCache(const Filepath& path, int size, int blocks)
{
	int i;

	ASSERT((size == 8) || (size == 9) || (size == 11));
	ASSERT(blocks > 0);

	// キャッシュワーク
	for (i=0; i<CacheMax; i++) {
		cache[i].disktrk = NULL;
		cache[i].serial = 0;
	}

	// その他
	serial = 0;
	sec_path = path;
	sec_size = size;
	sec_blocks = blocks;
	cd_raw = FALSE;
}

//---------------------------------------------------------------------------
//
//	デストラクタ
//
//---------------------------------------------------------------------------
DiskCache::~DiskCache()
{
	// トラックをクリア
	Clear();
}

//---------------------------------------------------------------------------
//
//	RAWモード設定
//
//---------------------------------------------------------------------------
void FASTCALL DiskCache::SetRawMode(BOOL raw)
{
	ASSERT(this);
	ASSERT(sec_size == 11);

	// 設定
	cd_raw = raw;
}

//---------------------------------------------------------------------------
//
//	セーブ
//
//---------------------------------------------------------------------------
BOOL FASTCALL DiskCache::Save()
{
	int i;

	ASSERT(this);

	// トラックを保存
	for (i=0; i<CacheMax; i++) {
		// 有効なトラックか
		if (cache[i].disktrk) {
			// 保存
			if (!cache[i].disktrk->Save(sec_path)) {
				return FALSE;
			}
		}
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	ディスクキャッシュ情報取得
//
//---------------------------------------------------------------------------
BOOL FASTCALL DiskCache::GetCache(int index, int& track, DWORD& serial) const
{
	ASSERT(this);
	ASSERT((index >= 0) && (index < CacheMax));

	// 未使用ならFALSE
	if (!cache[index].disktrk) {
		return FALSE;
	}

	// トラックとシリアルを設定
	track = cache[index].disktrk->GetTrack();
	serial = cache[index].serial;

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	クリア
//
//---------------------------------------------------------------------------
void FASTCALL DiskCache::Clear()
{
	int i;

	ASSERT(this);

	// キャッシュワークを解放
	for (i=0; i<CacheMax; i++) {
		if (cache[i].disktrk) {
			delete cache[i].disktrk;
			cache[i].disktrk = NULL;
		}
	}
}

//---------------------------------------------------------------------------
//
//	セクタリード
//
//---------------------------------------------------------------------------
BOOL FASTCALL DiskCache::Read(BYTE *buf, int block)
{
	int track;
	DiskTrack *disktrk;

	ASSERT(this);
	ASSERT(sec_size != 0);

	// 先に更新
	Update();

	// トラックを算出(256セクタ/トラックに固定)
	track = block >> 8;

	// そのトラックデータを得る
	disktrk = Assign(track);
	if (!disktrk) {
		return FALSE;
	}

	// トラックに任せる
	return disktrk->Read(buf, (BYTE)block);
}

//---------------------------------------------------------------------------
//
//	セクタライト
//
//---------------------------------------------------------------------------
BOOL FASTCALL DiskCache::Write(const BYTE *buf, int block)
{
	int track;
	DiskTrack *disktrk;

	ASSERT(this);
	ASSERT(sec_size != 0);

	// 先に更新
	Update();

	// トラックを算出(256セクタ/トラックに固定)
	track = block >> 8;

	// そのトラックデータを得る
	disktrk = Assign(track);
	if (!disktrk) {
		return FALSE;
	}

	// トラックに任せる
	return disktrk->Write(buf, (BYTE)block);
}

//---------------------------------------------------------------------------
//
//	トラックの割り当て
//
//---------------------------------------------------------------------------
DiskTrack* FASTCALL DiskCache::Assign(int track)
{
	int i;
	int c;
	DWORD s;

	ASSERT(this);
	ASSERT(sec_size != 0);
	ASSERT(track >= 0);

	// まず、既に割り当てされていないか調べる
	for (i=0; i<CacheMax; i++) {
		if (cache[i].disktrk) {
			if (cache[i].disktrk->GetTrack() == track) {
				// トラックが一致
				cache[i].serial = serial;
				return cache[i].disktrk;
			}
		}
	}

	// 次に、空いているものがないか調べる
	for (i=0; i<CacheMax; i++) {
		if (!cache[i].disktrk) {
			// ロードを試みる
			if (Load(i, track)) {
				// ロード成功
				cache[i].serial = serial;
				return cache[i].disktrk;
			}

			// ロード失敗
			return NULL;
		}
	}

	// 最後に、シリアル番号の一番若いものを探し、削除する

	// インデックス0を候補cとする
	s = cache[0].serial;
	c = 0;

	// 候補とシリアルを比較し、より小さいものへ更新する
	for (i=0; i<CacheMax; i++) {
		ASSERT(cache[i].disktrk);

		// 既に存在するシリアルと比較、更新
		if (cache[i].serial < s) {
			s = cache[i].serial;
			c = i;
		}
	}

	// このトラックを保存
	if (!cache[c].disktrk->Save(sec_path)) {
		return NULL;
	}

	// このトラックを削除
	delete cache[c].disktrk;
	cache[c].disktrk = NULL;

	// ロード
	if (Load(c, track)) {
		// ロード成功
		cache[c].serial = serial;
		return cache[c].disktrk;
	}

	// ロード失敗
	return NULL;
}

//---------------------------------------------------------------------------
//
//	トラックのロード
//
//---------------------------------------------------------------------------
BOOL FASTCALL DiskCache::Load(int index, int track)
{
	int sectors;
	DiskTrack *disktrk;

	ASSERT(this);
	ASSERT((index >= 0) && (index < CacheMax));
	ASSERT(track >= 0);
	ASSERT(!cache[index].disktrk);

	// このトラックのセクタ数を取得
	sectors = sec_blocks - (track << 8);
	ASSERT(sectors > 0);
	if (sectors > 0x100) {
		sectors = 0x100;
	}

	// ディスクトラックを作成
	disktrk = new DiskTrack(track, sec_size, sectors, cd_raw);

	// ロードを試みる
	if (!disktrk->Load(sec_path)) {
		// 失敗
		delete disktrk;
		return FALSE;
	}

	// 割り当て成功、ワークを設定
	cache[index].disktrk = disktrk;

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	シリアル番号の更新
//
//---------------------------------------------------------------------------
void FASTCALL DiskCache::Update()
{
	int i;

	ASSERT(this);

	// 更新して、0以外は何もしない
	serial++;
	if (serial != 0) {
		return;
	}

	// 全キャッシュのシリアルをクリア(32bitループしている)
	for (i=0; i<CacheMax; i++) {
		cache[i].serial = 0;
	}
}

//===========================================================================
//
//	ディスク
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	コンストラクタ
//
//---------------------------------------------------------------------------
Disk::Disk(Device *dev)
{
	// コントローラとなるデバイスを記憶
	ctrl = dev;

	// ワーク初期化
	disk.id = MAKEID('N', 'U', 'L', 'L');
	disk.ready = FALSE;
	disk.writep = FALSE;
	disk.readonly = FALSE;
	disk.removable = FALSE;
	disk.lock = FALSE;
	disk.attn = FALSE;
	disk.reset = FALSE;
	disk.size = 0;
	disk.blocks = 0;
	disk.lun = 0;
	disk.code = 0;
	disk.dcache = NULL;
}

//---------------------------------------------------------------------------
//
//	デストラクタ
//
//---------------------------------------------------------------------------
Disk::~Disk()
{
	// ディスクキャッシュの保存
	if (disk.ready) {
		// レディの場合のみ
		ASSERT(disk.dcache);
		disk.dcache->Save();
	}

	// ディスクキャッシュの削除
	if (disk.dcache) {
		delete disk.dcache;
		disk.dcache = NULL;
	}
}

//---------------------------------------------------------------------------
//
//	リセット
//
//---------------------------------------------------------------------------
void FASTCALL Disk::Reset()
{
	ASSERT(this);

	// ロックなし、アテンションなし、リセットあり
	disk.lock = FALSE;
	disk.attn = FALSE;
	disk.reset = TRUE;
}

//---------------------------------------------------------------------------
//
//	セーブ
//
//---------------------------------------------------------------------------
BOOL FASTCALL Disk::Save(Fileio *fio, int ver)
{
	size_t sz;

	ASSERT(this);
	ASSERT(fio);

	// サイズをセーブ
	sz = sizeof(disk_t);
	if (!fio->Write(&sz, sizeof(sz))) {
		return FALSE;
	}

	// 実体をセーブ
	if (!fio->Write(&disk, (int)sz)) {
		return FALSE;
	}

	// パスをセーブ
	if (!diskpath.Save(fio, ver)) {
		return FALSE;
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	ロード
//
//---------------------------------------------------------------------------
BOOL FASTCALL Disk::Load(Fileio *fio, int ver)
{
	size_t sz;
	disk_t buf;
	Filepath path;

	ASSERT(this);
	ASSERT(fio);

	// version2.03より前は、ディスクはセーブしていない
	if (ver <= 0x0202) {
		return TRUE;
	}

	// 現在のディスクキャッシュを削除
	if (disk.dcache) {
		disk.dcache->Save();
		delete disk.dcache;
		disk.dcache = NULL;
	}

	// サイズをロード、照合
	if (!fio->Read(&sz, sizeof(sz))) {
		return FALSE;
	}
	if (sz != sizeof(disk_t)) {
		return FALSE;
	}

	// バッファへロード
	if (!fio->Read(&buf, (int)sz)) {
		return FALSE;
	}

	// パスをロード
	if (!path.Load(fio, ver)) {
		return FALSE;
	}

	// IDが一致した場合のみ、移動
	if (disk.id == buf.id) {
		// NULLなら何もしない
		if (IsNULL()) {
			return TRUE;
		}

		// セーブした時と同じ種類のデバイス
		disk.ready = FALSE;
		if (Open(path)) {
			// Open内でディスクキャッシュは作成されている
			// プロパティのみ移動
			if (!disk.readonly) {
				disk.writep = buf.writep;
			}
			disk.lock = buf.lock;
			disk.attn = buf.attn;
			disk.reset = buf.reset;
			disk.lun = buf.lun;
			disk.code = buf.code;

			// 正常にロードできた
			return TRUE;
		}
	}

	// ディスクキャッシュ再作成
	if (!IsReady()) {
		disk.dcache = NULL;
	}
	else {
		disk.dcache = new DiskCache(diskpath, disk.size, disk.blocks);
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	NULLチェック
//
//---------------------------------------------------------------------------
BOOL FASTCALL Disk::IsNULL() const
{
	ASSERT(this);

	if (disk.id == MAKEID('N', 'U', 'L', 'L')) {
		return TRUE;
	}
	return FALSE;
}

//---------------------------------------------------------------------------
//
//	SASIチェック
//
//---------------------------------------------------------------------------
BOOL FASTCALL Disk::IsSASI() const
{
	ASSERT(this);

	if (disk.id == MAKEID('S', 'A', 'H', 'D')) {
		return TRUE;
	}
	return FALSE;
}

//---------------------------------------------------------------------------
//
//	オープン
//	※派生クラスで、オープン成功後の後処理として呼び出すこと
//
//---------------------------------------------------------------------------
BOOL FASTCALL Disk::Open(const Filepath& path)
{
	Fileio fio;

	ASSERT(this);
	ASSERT((disk.size == 8) || (disk.size == 9) || (disk.size == 11));
	ASSERT(disk.blocks > 0);

	// レディ
	disk.ready = TRUE;

	// キャッシュ初期化
	ASSERT(!disk.dcache);
	disk.dcache = new DiskCache(path, disk.size, disk.blocks);

	// 読み書きオープン可能か
	if (fio.Open(path, Fileio::ReadWrite)) {
		// 書き込み許可、リードオンリーでない
		disk.writep = FALSE;
		disk.readonly = FALSE;
		fio.Close();
	}
	else {
		// 書き込み禁止、リードオンリー
		disk.writep = TRUE;
		disk.readonly = TRUE;
	}

	// ロックされていない
	disk.lock = FALSE;

	// パス保存
	diskpath = path;

	// 成功
	return TRUE;
}

//---------------------------------------------------------------------------
//
//	イジェクト
//
//---------------------------------------------------------------------------
void FASTCALL Disk::Eject(BOOL force)
{
	ASSERT(this);

	// リムーバブルでなければイジェクトできない
	if (!disk.removable) {
		return;
	}

	// レディでなければイジェクト必要ない
	if (!disk.ready) {
		return;
	}

	// 強制フラグがない場合、ロックされていないことが必要
	if (!force) {
		if (disk.lock) {
			return;
		}
	}

	// ディスクキャッシュを削除
	disk.dcache->Save();
	delete disk.dcache;
	disk.dcache = NULL;

	// ノットレディ、アテンションなし
	disk.ready = FALSE;
	disk.writep = FALSE;
	disk.readonly = FALSE;
	disk.attn = FALSE;
}

//---------------------------------------------------------------------------
//
//	書き込み禁止
//
//---------------------------------------------------------------------------
void FASTCALL Disk::WriteP(BOOL writep)
{
	ASSERT(this);

	// レディであること
	if (!disk.ready) {
		return;
	}

	// Read Onlyの場合は、プロテクト状態のみ
	if (disk.readonly) {
		ASSERT(disk.writep);
		return;
	}

	// フラグ設定
	disk.writep = writep;
}

//---------------------------------------------------------------------------
//
//	内部ワーク取得
//
//---------------------------------------------------------------------------
void FASTCALL Disk::GetDisk(disk_t *buffer) const
{
	ASSERT(this);
	ASSERT(buffer);

	// 内部ワークをコピー
	*buffer = disk;
}

//---------------------------------------------------------------------------
//
//	パス取得
//
//---------------------------------------------------------------------------
void FASTCALL Disk::GetPath(Filepath& path) const
{
	path = diskpath;
}

//---------------------------------------------------------------------------
//
//	フラッシュ
//
//---------------------------------------------------------------------------
BOOL FASTCALL Disk::Flush()
{
	ASSERT(this);

	// キャッシュがなければ何もしない
	if (!disk.dcache) {
		return TRUE;
	}

	// キャッシュを保存
	return disk.dcache->Save();
}

//---------------------------------------------------------------------------
//
//	レディチェック
//
//---------------------------------------------------------------------------
BOOL FASTCALL Disk::CheckReady()
{
	ASSERT(this);

	// リセットなら、ステータスを返す
	if (disk.reset) {
		disk.code = DISK_DEVRESET;
		disk.reset = FALSE;
		return FALSE;
	}

	// アテンションなら、ステータスを返す
	if (disk.attn) {
		disk.code = DISK_ATTENTION;
		disk.attn = FALSE;
		return FALSE;
	}

	// ノットレディなら、ステータスを返す
	if (!disk.ready) {
		disk.code = DISK_NOTREADY;
		return FALSE;
	}

	// エラーなしに初期化
	disk.code = DISK_NOERROR;
	return TRUE;
}

//---------------------------------------------------------------------------
//
//	INQUIRY
//	※常時成功する必要がある
//
//---------------------------------------------------------------------------
int FASTCALL Disk::Inquiry(const DWORD* /*cdb*/, BYTE* /*buf*/)
{
	ASSERT(this);

	// デフォルトはINQUIRY失敗
	disk.code = DISK_INVALIDCMD;
	return 0;
}

//---------------------------------------------------------------------------
//
//	REQUEST SENSE
//	※SASIは別処理
//
//---------------------------------------------------------------------------
int FASTCALL Disk::RequestSense(const DWORD *cdb, BYTE *buf)
{
	int size;

	ASSERT(this);
	ASSERT(cdb);
	ASSERT(buf);

	// エラーがない場合に限り、ノットレディをチェック
	if (disk.code == DISK_NOERROR) {
		if (!disk.ready) {
			disk.code = DISK_NOTREADY;
		}
	}

	// サイズ決定(アロケーションレングスに従う)
	size = (int)cdb[4];
	ASSERT((size >= 0) && (size < 0x100));

	// SCSI-1では、サイズ0のときに4バイト転送する(SCSI-2ではこの仕様は削除)
	if (size == 0) {
		size = 4;
	}

	// バッファをクリア
	memset(buf, 0, size);

	// 拡張センスデータを含めた、18バイトを設定
	buf[0] = 0x70;
	buf[2] = (BYTE)(disk.code >> 16);
	buf[7] = 10;
	buf[12] = (BYTE)(disk.code >> 8);
	buf[13] = (BYTE)disk.code;

	// コードをクリア
	disk.code = 0x00;

	return size;
}

//---------------------------------------------------------------------------
//
//	MODE SELECTチェック
//	※disk.codeの影響を受けない
//
//---------------------------------------------------------------------------
int FASTCALL Disk::SelectCheck(const DWORD *cdb)
{
	int length;

	ASSERT(this);
	ASSERT(cdb);

	// セーブパラメータが設定されていればエラー
	if (cdb[1] & 0x01) {
		disk.code = DISK_INVALIDCDB;
		return 0;
	}

	// パラメータレングスで指定されたデータを受け取る
	length = (int)cdb[4];
	return length;
}

//---------------------------------------------------------------------------
//
//	MODE SELECT
//	※disk.codeの影響を受けない
//
//---------------------------------------------------------------------------
BOOL FASTCALL Disk::ModeSelect(const BYTE *buf, int size)
{
	ASSERT(this);
	ASSERT(buf);
	ASSERT(size >= 0);

	// 設定できない
	disk.code = DISK_INVALIDPRM;

	return FALSE;
}

//---------------------------------------------------------------------------
//
//	MODE SENSE
//	※disk.codeの影響を受けない
//
//---------------------------------------------------------------------------
int FASTCALL Disk::ModeSense(const DWORD *cdb, BYTE *buf)
{
	int page;
	int length;
	int size;
	BOOL valid;
	BOOL change;

	ASSERT(this);
	ASSERT(cdb);
	ASSERT(buf);
	ASSERT(cdb[0] == 0x1a);

	// レングス取得、バッファクリア
	length = (int)cdb[4];
	ASSERT((length >= 0) && (length < 0x100));
	memset(buf, 0, length);

	// 変更可能フラグ取得
	if ((cdb[2] & 0xc0) == 0x40) {
		change = TRUE;
	}
	else {
		change = FALSE;
	}

	// ページコード取得(0x00は最初からvalid)
	page = cdb[2] & 0x3f;
	if (page == 0x00) {
		valid = TRUE;
	}
	else {
		valid = FALSE;
	}

	// 基本情報
	size = 4;
	if (disk.writep) {
		buf[2] = 0x80;
	}

	// DBDが0なら、ブロックディスクリプタを追加
	if ((cdb[1] & 0x08) == 0) {
		// モードパラメータヘッダ
		buf[ 3] = 0x08;

		// レディの場合に限り
		if (disk.ready) {
			// ブロックディスクリプタ(ブロック数)
			buf[ 5] = (BYTE)(disk.blocks >> 16);
			buf[ 6] = (BYTE)(disk.blocks >> 8);
			buf[ 7] = (BYTE)disk.blocks;

			// ブロックディスクリプタ(ブロックレングス)
			size = 1 << disk.size;
			buf[ 9] = (BYTE)(size >> 16);
			buf[10] = (BYTE)(size >> 8);
			buf[11] = (BYTE)size;
		}

		// サイズ再設定
		size = 12;
	}

	// ページコード1(read-write error recovery)
	if ((page == 0x01) || (page == 0x3f)) {
		size += AddError(change, &buf[size]);
		valid = TRUE;
	}

	// ページコード3(format device)
	if ((page == 0x03) || (page == 0x3f)) {
		size += AddFormat(change, &buf[size]);
		valid = TRUE;
	}

	// ページコード6(optical)
	if (disk.id == MAKEID('S', 'C', 'M', 'O')) {
		if ((page == 0x06) || (page == 0x3f)) {
			size += AddOpt(change, &buf[size]);
			valid = TRUE;
		}
	}

	// ページコード8(caching)
	if ((page == 0x08) || (page == 0x3f)) {
		size += AddCache(change, &buf[size]);
		valid = TRUE;
	}

	// ページコード13(CD-ROM)
	if (disk.id == MAKEID('S', 'C', 'C', 'D')) {
		if ((page == 0x0d) || (page == 0x3f)) {
			size += AddCDROM(change, &buf[size]);
			valid = TRUE;
		}
	}

	// ページコード14(CD-DA)
	if (disk.id == MAKEID('S', 'C', 'C', 'D')) {
		if ((page == 0x0e) || (page == 0x3f)) {
			size += AddCDDA(change, &buf[size]);
			valid = TRUE;
		}
	}

	// モードデータレングスを最終設定
	buf[0] = (BYTE)(size - 1);

	// サポートしていないページか
	if (!valid) {
		disk.code = DISK_INVALIDCDB;
		return 0;
	}

	// Saved valuesはサポートしていない
	if ((cdb[2] & 0xc0) == 0xc0) {
		disk.code = DISK_PARAMSAVE;
		return 0;
	}

	// MODE SENSE成功
	disk.code = DISK_NOERROR;
	return length;
}

//---------------------------------------------------------------------------
//
//	エラーページ追加
//
//---------------------------------------------------------------------------
int FASTCALL Disk::AddError(BOOL change, BYTE *buf)
{
	ASSERT(this);
	ASSERT(buf);

	// コード・レングスを設定
	buf[0] = 0x01;
	buf[1] = 0x0a;

	// 変更可能領域はない
	if (change) {
		return 12;
	}

	// リトライカウントは0、リミットタイムは装置内部のデフォルト値を使用
	return 12;
}

//---------------------------------------------------------------------------
//
//	フォーマットページ追加
//
//---------------------------------------------------------------------------
int FASTCALL Disk::AddFormat(BOOL change, BYTE *buf)
{
	ASSERT(this);
	ASSERT(buf);

	// コード・レングスを設定
	buf[0] = 0x03;
	buf[1] = 0x16;

	// 変更可能領域はない
	if (change) {
		return 24;
	}

	// リムーバブル属性を設定
	if (disk.removable) {
		buf[20] = 0x20;
	}

	return 24;
}

//---------------------------------------------------------------------------
//
//	オプティカルページ追加
//
//---------------------------------------------------------------------------
int FASTCALL Disk::AddOpt(BOOL change, BYTE *buf)
{
	ASSERT(this);
	ASSERT(buf);

	// コード・レングスを設定
	buf[0] = 0x06;
	buf[1] = 0x02;

	// 変更可能領域はない
	if (change) {
		return 4;
	}

	// 更新ブロックのレポートは行わない
	return 4;
}

//---------------------------------------------------------------------------
//
//	キャッシュページ追加
//
//---------------------------------------------------------------------------
int FASTCALL Disk::AddCache(BOOL change, BYTE *buf)
{
	ASSERT(this);
	ASSERT(buf);

	// コード・レングスを設定
	buf[0] = 0x08;
	buf[1] = 0x0a;

	// 変更可能領域はない
	if (change) {
		return 12;
	}

	// 読み込みキャッシュのみ有効、プリフェッチは行わない
	return 12;
}

//---------------------------------------------------------------------------
//
//	CD-ROMページ追加
//
//---------------------------------------------------------------------------
int FASTCALL Disk::AddCDROM(BOOL change, BYTE *buf)
{
	ASSERT(this);
	ASSERT(buf);

	// コード・レングスを設定
	buf[0] = 0x0d;
	buf[1] = 0x06;

	// 変更可能領域はない
	if (change) {
		return 8;
	}

	// インアクティブタイマは2sec
	buf[3] = 0x05;

	// MSF倍数はそれぞれ60, 75
	buf[5] = 60;
	buf[7] = 75;

	return 8;
}

//---------------------------------------------------------------------------
//
//	CD-DAページ追加
//
//---------------------------------------------------------------------------
int FASTCALL Disk::AddCDDA(BOOL change, BYTE *buf)
{
	ASSERT(this);
	ASSERT(buf);

	// コード・レングスを設定
	buf[0] = 0x0e;
	buf[1] = 0x0e;

	// 変更可能領域はない
	if (change) {
		return 16;
	}

	// オーディオは操作完了を待ち、 複数トラックにまたがるPLAYを許可する
	return 16;
}

//---------------------------------------------------------------------------
//
//	TEST UNIT READY
//
//---------------------------------------------------------------------------
BOOL FASTCALL Disk::TestUnitReady(const DWORD* /*cdb*/)
{
	ASSERT(this);

	// 状態チェック
	if (!CheckReady()) {
		return FALSE;
	}

	// TEST UNIT READY成功
	return TRUE;
}

//---------------------------------------------------------------------------
//
//	REZERO UNIT
//
//---------------------------------------------------------------------------
BOOL FASTCALL Disk::Rezero(const DWORD* /*cdb*/)
{
	ASSERT(this);

	// 状態チェック
	if (!CheckReady()) {
		return FALSE;
	}

	// REZERO成功
	return TRUE;
}

//---------------------------------------------------------------------------
//
//	FORMAT UNIT
//	※SASIはオペコード$06、SCSIはオペコード$04
//
//---------------------------------------------------------------------------
BOOL FASTCALL Disk::Format(const DWORD *cdb)
{
	ASSERT(this);

	// 状態チェック
	if (!CheckReady()) {
		return FALSE;
	}

	// FMTDATA=1はサポートしない
	if (cdb[1] & 0x10) {
		disk.code = DISK_INVALIDCDB;
		return FALSE;
	}

	// FORMAT成功
	return TRUE;
}

//---------------------------------------------------------------------------
//
//	REASSIGN BLOCKS
//
//---------------------------------------------------------------------------
BOOL FASTCALL Disk::Reassign(const DWORD* /*cdb*/)
{
	ASSERT(this);

	// 状態チェック
	if (!CheckReady()) {
		return FALSE;
	}

	// REASSIGN BLOCKS成功
	return TRUE;
}

//---------------------------------------------------------------------------
//
//	READ
//
//---------------------------------------------------------------------------
int FASTCALL Disk::Read(BYTE *buf, int block)
{
	ASSERT(this);
	ASSERT(buf);
	ASSERT(block >= 0);

	// 状態チェック
	if (!CheckReady()) {
		return 0;
	}

	// トータルブロック数を超えていればエラー
	if (block >= disk.blocks) {
		disk.code = DISK_INVALIDLBA;
		return 0;
	}

	// キャッシュに任せる
	if (!disk.dcache->Read(buf, block)) {
		disk.code = DISK_READFAULT;
		return 0;
	}

	// 成功
	return (1 << disk.size);
}

//---------------------------------------------------------------------------
//
//	WRITEチェック
//
//---------------------------------------------------------------------------
int FASTCALL Disk::WriteCheck(int block)
{
	ASSERT(this);
	ASSERT(block >= 0);

	// 状態チェック
	if (!CheckReady()) {
		return 0;
	}

	// トータルブロック数を超えていればエラー
	if (block >= disk.blocks) {
		return 0;
	}

	// 書き込み禁止ならエラー
	if (disk.writep) {
		disk.code = DISK_WRITEPROTECT;
		return 0;
	}

	// 成功
	return (1 << disk.size);
}

//---------------------------------------------------------------------------
//
//	WRITE
//
//---------------------------------------------------------------------------
BOOL FASTCALL Disk::Write(const BYTE *buf, int block)
{
	ASSERT(this);
	ASSERT(buf);
	ASSERT(block >= 0);

	// レディでなければエラー
	if (!disk.ready) {
		disk.code = DISK_NOTREADY;
		return FALSE;
	}

	// トータルブロック数を超えていればエラー
	if (block >= disk.blocks) {
		disk.code = DISK_INVALIDLBA;
		return FALSE;
	}

	// 書き込み禁止ならエラー
	if (disk.writep) {
		disk.code = DISK_WRITEPROTECT;
		return FALSE;
	}

	// キャッシュに任せる
	if (!disk.dcache->Write(buf, block)) {
		disk.code = DISK_WRITEFAULT;
		return FALSE;
	}

	// 成功
	disk.code = DISK_NOERROR;
	return TRUE;
}

//---------------------------------------------------------------------------
//
//	SEEK
//	※LBAのチェックは行わない(SASI IOCS)
//
//---------------------------------------------------------------------------
BOOL FASTCALL Disk::Seek(const DWORD* /*cdb*/)
{
	ASSERT(this);

	// 状態チェック
	if (!CheckReady()) {
		return FALSE;
	}

	// SEEK成功
	return TRUE;
}

//---------------------------------------------------------------------------
//
//	START STOP UNIT
//
//---------------------------------------------------------------------------
BOOL FASTCALL Disk::StartStop(const DWORD *cdb)
{
	ASSERT(this);
	ASSERT(cdb);
	ASSERT(cdb[0] == 0x1b);

	// イジェクトビットを見て、必要ならイジェクト
	if (cdb[4] & 0x02) {
		if (disk.lock) {
			// ロックされているので、イジェクトできない
			disk.code = DISK_PREVENT;
			return FALSE;
		}

		// イジェクト
		Eject(FALSE);
	}

	// OK
	disk.code = DISK_NOERROR;
	return TRUE;
}

//---------------------------------------------------------------------------
//
//	SEND DIAGNOSTIC
//
//---------------------------------------------------------------------------
BOOL FASTCALL Disk::SendDiag(const DWORD *cdb)
{
	ASSERT(this);
	ASSERT(cdb);
	ASSERT(cdb[0] == 0x1d);

	// PFビットはサポートしない
	if (cdb[1] & 0x10) {
		disk.code = DISK_INVALIDCDB;
		return FALSE;
	}

	// パラメータリストはサポートしない
	if ((cdb[3] != 0) || (cdb[4] != 0)) {
		disk.code = DISK_INVALIDCDB;
		return FALSE;
	}

	// 常に成功
	disk.code = DISK_NOERROR;
	return TRUE;
}

//---------------------------------------------------------------------------
//
//	PREVENT/ALLOW MEDIUM REMOVAL
//
//---------------------------------------------------------------------------
BOOL FASTCALL Disk::Removal(const DWORD *cdb)
{
	ASSERT(this);
	ASSERT(cdb);
	ASSERT(cdb[0] == 0x1e);

	// 状態チェック
	if (!CheckReady()) {
		return FALSE;
	}

	// ロックフラグを設定
	if (cdb[4] & 0x01) {
		disk.lock = TRUE;
	}
	else {
		disk.lock = FALSE;
	}

	// REMOVAL成功
	return TRUE;
}

//---------------------------------------------------------------------------
//
//	READ CAPACITY
//
//---------------------------------------------------------------------------
int FASTCALL Disk::ReadCapacity(const DWORD* /*cdb*/, BYTE *buf)
{
	DWORD blocks;
	DWORD length;

	ASSERT(this);
	ASSERT(buf);

	// バッファクリア
	memset(buf, 0, 8);

	// 状態チェック
	if (!CheckReady()) {
		return 0;
	}

	// 論理ブロックアドレスの終端(disk.blocks - 1)を作成
	ASSERT(disk.blocks > 0);
	blocks = disk.blocks - 1;
	buf[0] = (BYTE)(blocks >> 24);
	buf[1] = (BYTE)(blocks >> 16);
	buf[2] = (BYTE)(blocks >>  8);
	buf[3] = (BYTE)blocks;

	// ブロックレングス(1 << disk.size)を作成
	length = 1 << disk.size;
	buf[4] = (BYTE)(length >> 24);
	buf[5] = (BYTE)(length >> 16);
	buf[6] = (BYTE)(length >> 8);
	buf[7] = (BYTE)length;

	// 返送サイズを返す
	return 8;
}

//---------------------------------------------------------------------------
//
//	VERIFY
//
//---------------------------------------------------------------------------
BOOL FASTCALL Disk::Verify(const DWORD *cdb)
{
	int record;
    int blocks;

	ASSERT(this);
	ASSERT(cdb);
	ASSERT(cdb[0] == 0x2f);

	// パラメータ取得
	record = cdb[2];
	record <<= 8;
	record |= cdb[3];
	record <<= 8;
	record |= cdb[4];
	record <<= 8;
	record |= cdb[5];
	blocks = cdb[7];
	blocks <<= 8;
	blocks |= cdb[8];

	// 状態チェック
	if (!CheckReady()) {
		return 0;
	}

	// パラメータチェック
	if (disk.blocks < (record + blocks)) {
		disk.code = DISK_INVALIDLBA;
		return FALSE;
	}

	// 成功
	return TRUE;
}

//---------------------------------------------------------------------------
//
//	READ TOC
//
//---------------------------------------------------------------------------
int FASTCALL Disk::ReadToc(const DWORD *cdb, BYTE *buf)
{
	ASSERT(this);
	ASSERT(cdb);
	ASSERT(cdb[0] == 0x43);
	ASSERT(buf);

	// このコマンドはサポートしない
	disk.code = DISK_INVALIDCMD;
	return FALSE;
}

//---------------------------------------------------------------------------
//
//	PLAY AUDIO
//
//---------------------------------------------------------------------------
BOOL FASTCALL Disk::PlayAudio(const DWORD *cdb)
{
	ASSERT(this);
	ASSERT(cdb);
	ASSERT(cdb[0] == 0x45);

	// このコマンドはサポートしない
	disk.code = DISK_INVALIDCMD;
	return FALSE;
}

//---------------------------------------------------------------------------
//
//	PLAY AUDIO MSF
//
//---------------------------------------------------------------------------
BOOL FASTCALL Disk::PlayAudioMSF(const DWORD *cdb)
{
	ASSERT(this);
	ASSERT(cdb);
	ASSERT(cdb[0] == 0x47);

	// このコマンドはサポートしない
	disk.code = DISK_INVALIDCMD;
	return FALSE;
}

//---------------------------------------------------------------------------
//
//	PLAY AUDIO TRACK
//
//---------------------------------------------------------------------------
BOOL FASTCALL Disk::PlayAudioTrack(const DWORD *cdb)
{
	ASSERT(this);
	ASSERT(cdb);
	ASSERT(cdb[0] == 0x48);

	// このコマンドはサポートしない
	disk.code = DISK_INVALIDCMD;
	return FALSE;
}

//===========================================================================
//
//	SASI ハードディスク
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	コンストラクタ
//
//---------------------------------------------------------------------------
SASIHD::SASIHD(Device *dev) : Disk(dev)
{
	// SASI ハードディスク
	disk.id = MAKEID('S', 'A', 'H', 'D');
}

//---------------------------------------------------------------------------
//
//	オープン
//
//---------------------------------------------------------------------------
BOOL FASTCALL SASIHD::Open(const Filepath& path)
{
	Fileio fio;
	DWORD size;

	ASSERT(this);
	ASSERT(!disk.ready);

	// 読み込みオープンが必要
	if (!fio.Open(path, Fileio::ReadOnly)) {
		return FALSE;
	}

	// ファイルサイズの取得
	size = fio.GetFileSize();
	fio.Close();

	// 10MB, 20MB, 40MBのみ
	switch (size) {
		// 10MB
		case 0x9f5400:
			break;

		// 20MB
		case 0x13c9800:
			break;

		// 40MB
		case 0x2793000:
			break;

		// その他(サポートしない)
		default:
			return FALSE;
	}

	// セクタサイズとブロック数
	disk.size = 8;
	disk.blocks = size >> 8;

	// 基本クラス
	return Disk::Open(path);
}

//---------------------------------------------------------------------------
//
//	デバイスリセット
//
//---------------------------------------------------------------------------
void FASTCALL SASIHD::Reset()
{
	ASSERT(this);

	// ロック状態解除、アテンション解除
	disk.lock = FALSE;
	disk.attn = FALSE;

	// リセットなし、コードをクリア
	disk.reset = FALSE;
	disk.code = 0x00;
}

//---------------------------------------------------------------------------
//
//	REQUEST SENSE
//
//---------------------------------------------------------------------------
int FASTCALL SASIHD::RequestSense(const DWORD *cdb, BYTE *buf)
{
	int size;

	ASSERT(this);
	ASSERT(cdb);
	ASSERT(buf);

	// サイズ決定
	size = (int)cdb[4];
	ASSERT((size >= 0) && (size < 0x100));

	// SASIは非拡張フォーマットに固定
	memset(buf, 0, size);
	buf[0] = (BYTE)(disk.code >> 16);
	buf[1] = (BYTE)(disk.lun << 5);

	// コードをクリア
	disk.code = 0x00;

	return size;
}

//===========================================================================
//
//	SCSI ハードディスク
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	コンストラクタ
//
//---------------------------------------------------------------------------
SCSIHD::SCSIHD(Device *dev) : Disk(dev)
{
	// SCSI ハードディスク
	disk.id = MAKEID('S', 'C', 'H', 'D');
}

//---------------------------------------------------------------------------
//
//	オープン
//
//---------------------------------------------------------------------------
BOOL FASTCALL SCSIHD::Open(const Filepath& path)
{
	Fileio fio;
	DWORD size;

	ASSERT(this);
	ASSERT(!disk.ready);

	// 読み込みオープンが必要
	if (!fio.Open(path, Fileio::ReadOnly)) {
		return FALSE;
	}

	// ファイルサイズの取得
	size = fio.GetFileSize();
	fio.Close();

	// 512バイト単位であること
	if (size & 0x1ff) {
		return FALSE;
	}

	// 10MB以上4GB未満
	if (size < 0x9f5400) {
		return FALSE;
	}
	if (size > 0xfff00000) {
		return FALSE;
	}

	// セクタサイズとブロック数
	disk.size = 9;
	disk.blocks = size >> 9;

	// 基本クラス
	return Disk::Open(path);
}

//---------------------------------------------------------------------------
//
//	INQUIRY
//
//---------------------------------------------------------------------------
int FASTCALL SCSIHD::Inquiry(const DWORD *cdb, BYTE *buf)
{
	DWORD major;
	DWORD minor;
	char string[32];
	int size;
	int len;

	ASSERT(this);
	ASSERT(cdb);
	ASSERT(buf);
	ASSERT(cdb[0] == 0x12);

	// EVPDチェック
	if (cdb[1] & 0x01) {
		disk.code = DISK_INVALIDCDB;
		return FALSE;
	}

	// レディチェック(イメージファイルがない場合、エラーとする)
	if (!disk.ready) {
		disk.code = DISK_NOTREADY;
		return FALSE;
	}

	// 基本データ
	// buf[0] ... Direct Access Device
	// buf[2] ... SCSI-2準拠のコマンド体系
	// buf[3] ... SCSI-2準拠のInquiryレスポンス
	// buf[4] ... Inquiry追加データ
	memset(buf, 0, 8);
	buf[2] = 0x02;
	buf[3] = 0x02;
	buf[4] = 0x1f;

	// ベンダ
	memset(&buf[8], 0x20, 28);
	memcpy(&buf[8], "XM6", 3);

	// 製品名
	size = disk.blocks >> 11;
	if (size < 300)
		sprintf(string, "PRODRIVE LPS%dS", size);
	else if (size < 600)
		sprintf(string, "MAVERICK%dS", size);
	else if (size < 800)
		sprintf(string, "LIGHTNING%dS", size);
	else if (size < 1000)
		sprintf(string, "TRAILBRAZER%dS", size);
	else if (size < 2000)
		sprintf(string, "FIREBALL%dS", size);
	else
		sprintf(string, "FBSE%d.%dS", size / 1000, (size % 1000) / 100);
	memcpy(&buf[16], string, strlen(string));

	// リビジョン(XM6のバージョンNo)
	ctrl->GetVM()->GetVersion(major, minor);
	sprintf(string, "0%01d%01d%01d",
				major, (minor >> 4), (minor & 0x0f));
	memcpy((char*)&buf[32], string, 4);

	// サイズ36バイトかアロケーションレングスのうち、短い方で転送
	size = 36;
	len = (int)cdb[4];
	if (len < size) {
		size = len;
	}

	// 成功
	disk.code = DISK_NOERROR;
	return size;
}

//===========================================================================
//
//	SCSI 光磁気ディスク
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	コンストラクタ
//
//---------------------------------------------------------------------------
SCSIMO::SCSIMO(Device *dev) : Disk(dev)
{
	// SCSI 光磁気ディスク
	disk.id = MAKEID('S', 'C', 'M', 'O');

	// リムーバブル
	disk.removable = TRUE;
}

//---------------------------------------------------------------------------
//
//	オープン
//
//---------------------------------------------------------------------------
BOOL FASTCALL SCSIMO::Open(const Filepath& path, BOOL attn)
{
	Fileio fio;
	DWORD size;

	ASSERT(this);
	ASSERT(!disk.ready);

	// 読み込みオープンが必要
	if (!fio.Open(path, Fileio::ReadOnly)) {
		return FALSE;
	}

	// ファイルサイズの取得
	size = fio.GetFileSize();
	fio.Close();

	switch (size) {
		// 128MB
		case 0x797f400:
			disk.size = 9;
			disk.blocks = 248826;
			break;

		// 230MB
		case 0xd9eea00:
			disk.size = 9;
			disk.blocks = 446325;
			break;

		// 540MB
		case 0x1fc8b800:
			disk.size = 9;
			disk.blocks = 1041500;
			break;

		// 640MB
		case 0x25e28000:
			disk.size = 11;
			disk.blocks = 310352;
			break;

		// その他(エラー)
		default:
			return FALSE;
	}

	// 基本クラス
	Disk::Open(path);

	// レディならアテンション
	if (disk.ready && attn) {
		disk.attn = TRUE;
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	ロード
//
//---------------------------------------------------------------------------
BOOL FASTCALL SCSIMO::Load(Fileio *fio, int ver)
{
	size_t sz;
	disk_t buf;
	Filepath path;

	ASSERT(this);
	ASSERT(fio);
	ASSERT(ver >= 0x0200);

	// version2.03より前は、ディスクはセーブしていない
	if (ver <= 0x0202) {
		return TRUE;
	}

	// サイズをロード、照合
	if (!fio->Read(&sz, sizeof(sz))) {
		return FALSE;
	}
	if (sz != sizeof(disk_t)) {
		return FALSE;
	}

	// バッファへロード
	if (!fio->Read(&buf, (int)sz)) {
		return FALSE;
	}

	// パスをロード
	if (!path.Load(fio, ver)) {
		return FALSE;
	}

	// 必ずイジェクト
	Eject(TRUE);

	// IDが一致した場合のみ、移動
	if (disk.id != buf.id) {
		// セーブ時にMOでなかった。イジェクト状態を維持
		return TRUE;
	}

	// 再オープンを試みる
	if (!Open(path, FALSE)) {
		// 再オープンできない。イジェクト状態を維持
		return TRUE;
	}

	// Open内でディスクキャッシュは作成されている。プロパティのみ移動
	if (!disk.readonly) {
		disk.writep = buf.writep;
	}
	disk.lock = buf.lock;
	disk.attn = buf.attn;
	disk.reset = buf.reset;
	disk.lun = buf.lun;
	disk.code = buf.code;

	// 正常にロードできた
	return TRUE;
}

//---------------------------------------------------------------------------
//
//	INQUIRY
//
//---------------------------------------------------------------------------
int FASTCALL SCSIMO::Inquiry(const DWORD *cdb, BYTE *buf)
{
	DWORD major;
	DWORD minor;
	char string[32];
	int size;
	int len;

	ASSERT(this);
	ASSERT(cdb);
	ASSERT(buf);
	ASSERT(cdb[0] == 0x12);

	// EVPDチェック
	if (cdb[1] & 0x01) {
		disk.code = DISK_INVALIDCDB;
		return FALSE;
	}

	// 基本データ
	// buf[0] ... Optical Memory Device
	// buf[1] ... リムーバブル
	// buf[2] ... SCSI-2準拠のコマンド体系
	// buf[3] ... SCSI-2準拠のInquiryレスポンス
	// buf[4] ... Inquiry追加データ
	memset(buf, 0, 8);
	buf[0] = 0x07;
	buf[1] = 0x80;
	buf[2] = 0x02;
	buf[3] = 0x02;
	buf[4] = 0x1f;

	// ベンダ
	memset(&buf[8], 0x20, 28);
	memcpy(&buf[8], "XM6", 3);

	// 製品名
	memcpy(&buf[16], "M2513A", 6);

	// リビジョン(XM6のバージョンNo)
	ctrl->GetVM()->GetVersion(major, minor);
	sprintf(string, "0%01d%01d%01d",
				major, (minor >> 4), (minor & 0x0f));
	memcpy((char*)&buf[32], string, 4);

	// サイズ36バイトかアロケーションレングスのうち、短い方で転送
	size = 36;
	len = cdb[4];
	if (len < size) {
		size = len;
	}

	// 成功
	disk.code = DISK_NOERROR;
	return size;
}

//===========================================================================
//
//	CDトラック
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	コンストラクタ
//
//---------------------------------------------------------------------------
CDTrack::CDTrack(SCSICD *scsicd)
{
	ASSERT(scsicd);

	// 親となるCD-ROMデバイスを設定
	cdrom = scsicd;

	// トラック無効
	valid = FALSE;

	// その他のデータを初期化
	track_no = -1;
	first_lba = 0;
	last_lba = 0;
	audio = FALSE;
	raw = FALSE;
}

//---------------------------------------------------------------------------
//
//	デストラクタ
//
//---------------------------------------------------------------------------
CDTrack::~CDTrack()
{
}

//---------------------------------------------------------------------------
//
//	初期化
//
//---------------------------------------------------------------------------
BOOL FASTCALL CDTrack::Init(int track, DWORD first, DWORD last)
{
	ASSERT(this);
	ASSERT(!valid);
	ASSERT(track >= 1);
	ASSERT(first < last);

	// トラック番号を設定、有効化
	track_no = track;
	valid = TRUE;

	// LBAを記憶
	first_lba = first;
	last_lba = last;

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	パス設定
//
//---------------------------------------------------------------------------
void FASTCALL CDTrack::SetPath(BOOL cdda, const Filepath& path)
{
	ASSERT(this);
	ASSERT(valid);

	// CD-DAか、データか
	audio = cdda;

	// パス記憶
	imgpath = path;
}

//---------------------------------------------------------------------------
//
//	パス取得
//
//---------------------------------------------------------------------------
void FASTCALL CDTrack::GetPath(Filepath& path) const
{
	ASSERT(this);
	ASSERT(valid);

	// パスを返す
	path = imgpath;
}

//---------------------------------------------------------------------------
//
//	インデックス追加
//
//---------------------------------------------------------------------------
void FASTCALL CDTrack::AddIndex(int index, DWORD lba)
{
	ASSERT(this);
	ASSERT(valid);
	ASSERT(index > 0);
	ASSERT(first_lba <= lba);
	ASSERT(lba <= last_lba);

	// 現在はインデックスはサポートしない
	ASSERT(FALSE);
}

//---------------------------------------------------------------------------
//
//	開始LBA取得
//
//---------------------------------------------------------------------------
DWORD FASTCALL CDTrack::GetFirst() const
{
	ASSERT(this);
	ASSERT(valid);
	ASSERT(first_lba < last_lba);

	return first_lba;
}

//---------------------------------------------------------------------------
//
//	終端LBA取得
//
//---------------------------------------------------------------------------
DWORD FASTCALL CDTrack::GetLast() const
{
	ASSERT(this);
	ASSERT(valid);
	ASSERT(first_lba < last_lba);

	return last_lba;
}

//---------------------------------------------------------------------------
//
//	ブロック数取得
//
//---------------------------------------------------------------------------
DWORD FASTCALL CDTrack::GetBlocks() const
{
	ASSERT(this);
	ASSERT(valid);
	ASSERT(first_lba < last_lba);

	// 開始LBAと最終LBAから算出
	return (DWORD)(last_lba - first_lba + 1);
}

//---------------------------------------------------------------------------
//
//	トラック番号取得
//
//---------------------------------------------------------------------------
int FASTCALL CDTrack::GetTrackNo() const
{
	ASSERT(this);
	ASSERT(valid);
	ASSERT(track_no >= 1);

	return track_no;
}

//---------------------------------------------------------------------------
//
//	有効なブロックか
//
//---------------------------------------------------------------------------
BOOL FASTCALL CDTrack::IsValid(DWORD lba) const
{
	ASSERT(this);

	// トラック自体が無効なら、FALSE
	if (!valid) {
		return FALSE;
	}

	// firstより前なら、FALSE
	if (lba < first_lba) {
		return FALSE;
	}

	// lastより後なら、FALSE
	if (last_lba < lba) {
		return FALSE;
	}

	// このトラック
	return TRUE;
}

//---------------------------------------------------------------------------
//
//	オーディオトラックか
//
//---------------------------------------------------------------------------
BOOL FASTCALL CDTrack::IsAudio() const
{
	ASSERT(this);
	ASSERT(valid);

	return audio;
}

//===========================================================================
//
//	CD-DAバッファ
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	コンストラクタ
//
//---------------------------------------------------------------------------
CDDABuf::CDDABuf()
{
}

//---------------------------------------------------------------------------
//
//	デストラクタ
//
//---------------------------------------------------------------------------
CDDABuf::~CDDABuf()
{
}

//===========================================================================
//
//	SCSI CD-ROM
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	コンストラクタ
//
//---------------------------------------------------------------------------
SCSICD::SCSICD(Device *dev) : Disk(dev)
{
	int i;

	// SCSI CD-ROM
	disk.id = MAKEID('S', 'C', 'C', 'D');

	// リムーバブル、書込み禁止
	disk.removable = TRUE;
	disk.writep = TRUE;

	// RAW形式でない
	rawfile = FALSE;

	// フレーム初期化
	frame = 0;

	// トラック初期化
	for (i=0; i<TrackMax; i++) {
		track[i] = NULL;
	}
	tracks = 0;
	dataindex = -1;
	audioindex = -1;
}

//---------------------------------------------------------------------------
//
//	デストラクタ
//
//---------------------------------------------------------------------------
SCSICD::~SCSICD()
{
	// トラッククリア
	ClearTrack();
}

//---------------------------------------------------------------------------
//
//	ロード
//
//---------------------------------------------------------------------------
BOOL FASTCALL SCSICD::Load(Fileio *fio, int ver)
{
	size_t sz;
	disk_t buf;
	Filepath path;

	ASSERT(this);
	ASSERT(fio);
	ASSERT(ver >= 0x0200);

	// version2.03より前は、ディスクはセーブしていない
	if (ver <= 0x0202) {
		return TRUE;
	}

	// サイズをロード、照合
	if (!fio->Read(&sz, sizeof(sz))) {
		return FALSE;
	}
	if (sz != sizeof(disk_t)) {
		return FALSE;
	}

	// バッファへロード
	if (!fio->Read(&buf, (int)sz)) {
		return FALSE;
	}

	// パスをロード
	if (!path.Load(fio, ver)) {
		return FALSE;
	}

	// 必ずイジェクト
	Eject(TRUE);

	// IDが一致した場合のみ、移動
	if (disk.id != buf.id) {
		// セーブ時にCD-ROMでなかった。イジェクト状態を維持
		return TRUE;
	}

	// 再オープンを試みる
	if (!Open(path, FALSE)) {
		// 再オープンできない。イジェクト状態を維持
		return TRUE;
	}

	// Open内でディスクキャッシュは作成されている。プロパティのみ移動
	if (!disk.readonly) {
		disk.writep = buf.writep;
	}
	disk.lock = buf.lock;
	disk.attn = buf.attn;
	disk.reset = buf.reset;
	disk.lun = buf.lun;
	disk.code = buf.code;

	// 再度、ディスクキャッシュを破棄
	if (disk.dcache) {
		delete disk.dcache;
		disk.dcache = NULL;
	}
	disk.dcache = NULL;

	// 暫定
	disk.blocks = track[0]->GetBlocks();
	if (disk.blocks > 0) {
		// ディスクキャッシュを作り直す
		track[0]->GetPath(path);
		disk.dcache = new DiskCache(path, disk.size, disk.blocks);
		disk.dcache->SetRawMode(rawfile);

		// データインデックスを再設定
		dataindex = 0;
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	オープン
//
//---------------------------------------------------------------------------
BOOL FASTCALL SCSICD::Open(const Filepath& path, BOOL attn)
{
	Fileio fio;
	DWORD size;
	char file[5];

	ASSERT(this);
	ASSERT(!disk.ready);

	// 初期化、トラッククリア
	disk.blocks = 0;
	rawfile = FALSE;
	ClearTrack();

	// 読み込みオープンが必要
	if (!fio.Open(path, Fileio::ReadOnly)) {
		return FALSE;
	}

	// サイズ取得
	size = fio.GetFileSize();
	if (size <= 4) {
		fio.Close();
		return FALSE;
	}

	// CUEシートか、ISOファイルかの判定を行う
	fio.Read(file, 4);
	file[4] = '\0';
	fio.Close();

	// FILEで始まっていれば、CUEシートとみなす
	if (strnicmp(file, "FILE", 4) == 0) {
		// CUEとしてオープン
		if (!OpenCue(path)) {
			return FALSE;
		}
	}
	else {
		// ISOとしてオープン
		if (!OpenIso(path)) {
			return FALSE;
		}
	}

	// オープン成功
	ASSERT(disk.blocks > 0);
	disk.size = 11;

	// 基本クラス
	Disk::Open(path);

	// RAWフラグを設定
	ASSERT(disk.dcache);
	disk.dcache->SetRawMode(rawfile);

	// ROMメディアなので、書き込みはできない
	disk.writep = TRUE;

	// レディならアテンション
	if (disk.ready && attn) {
		disk.attn = TRUE;
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	オープン(CUE)
//
//---------------------------------------------------------------------------
BOOL FASTCALL SCSICD::OpenCue(const Filepath& path)
{
	ASSERT(this);

	// 常に失敗
	return FALSE;
}

//---------------------------------------------------------------------------
//
//	オープン(ISO)
//
//---------------------------------------------------------------------------
BOOL FASTCALL SCSICD::OpenIso(const Filepath& path)
{
	Fileio fio;
	DWORD size;
	BYTE header[12];
	BYTE sync[12];

	ASSERT(this);

	// 読み込みオープンが必要
	if (!fio.Open(path, Fileio::ReadOnly)) {
		return FALSE;
	}

	// サイズ取得
	size = fio.GetFileSize();
	if (size < 0x800) {
		fio.Close();
		return FALSE;
	}

	// 最初の12バイトを読み取って、クローズ
	if (!fio.Read(header, sizeof(header))) {
		fio.Close();
		return FALSE;
	}

	// RAW形式かチェック
	memset(sync, 0xff, sizeof(sync));
	sync[0] = 0x00;
	sync[11] = 0x00;
	rawfile = FALSE;
	if (memcmp(header, sync, sizeof(sync)) == 0) {
		// 00,FFx10,00なので、RAW形式と推定される
		if (!fio.Read(header, 4)) {
			fio.Close();
			return FALSE;
		}

		// MODE1/2048またはMODE1/2352のみサポート
		if (header[3] != 0x01) {
			// モードが違う
			fio.Close();
			return FALSE;
		}

		// RAWファイルに設定
		rawfile = TRUE;
	}
	fio.Close();

	if (rawfile) {
		// サイズが2536の倍数で、700MB以下であること
		if (size % 0x930) {
			return FALSE;
		}
		if (size > 912579600) {
			return FALSE;
		}

		// ブロック数を設定
		disk.blocks = size / 0x930;
	}
	else {
		// サイズが2048の倍数で、700MB以下であること
		if (size & 0x7ff) {
			return FALSE;
		}
		if (size > 0x2bed5000) {
			return FALSE;
		}

		// ブロック数を設定
		disk.blocks = size >> 11;
	}

	// データトラック1つのみ作成
	ASSERT(!track[0]);
	track[0] = new CDTrack(this);
	track[0]->Init(1, 0, disk.blocks - 1);
	track[0]->SetPath(FALSE, path);
	tracks = 1;
	dataindex = 0;

	// オープン成功
	return TRUE;
}

//---------------------------------------------------------------------------
//
//	INQUIRY
//
//---------------------------------------------------------------------------
int FASTCALL SCSICD::Inquiry(const DWORD *cdb, BYTE *buf)
{
	DWORD major;
	DWORD minor;
	char string[32];
	int size;
	int len;

	ASSERT(this);
	ASSERT(cdb);
	ASSERT(buf);
	ASSERT(cdb[0] == 0x12);

	// EVPDチェック
	if (cdb[1] & 0x01) {
		disk.code = DISK_INVALIDCDB;
		return FALSE;
	}

	// 基本データ
	// buf[0] ... CD-ROM Device
	// buf[1] ... リムーバブル
	// buf[2] ... SCSI-2準拠のコマンド体系
	// buf[3] ... SCSI-2準拠のInquiryレスポンス
	// buf[4] ... Inquiry追加データ
	memset(buf, 0, 8);
	buf[0] = 0x05;
	buf[1] = 0x80;
	buf[2] = 0x02;
	buf[3] = 0x02;
	buf[4] = 0x1f;

	// ベンダ
	memset(&buf[8], 0x20, 28);
	memcpy(&buf[8], "XM6", 3);

	// 製品名
	memcpy(&buf[16], "CDU-55S", 7);

	// リビジョン(XM6のバージョンNo)
	ctrl->GetVM()->GetVersion(major, minor);
	sprintf(string, "0%01d%01d%01d",
				major, (minor >> 4), (minor & 0x0f));
	memcpy((char*)&buf[32], string, 4);

	// サイズ36バイトかアロケーションレングスのうち、短い方で転送
	size = 36;
	len = cdb[4];
	if (len < size) {
		size = len;
	}

	// 成功
	disk.code = DISK_NOERROR;
	return size;
}

//---------------------------------------------------------------------------
//
//	READ
//
//---------------------------------------------------------------------------
int FASTCALL SCSICD::Read(BYTE *buf, int block)
{
	int index;
	Filepath path;

	ASSERT(this);
	ASSERT(buf);
	ASSERT(block >= 0);

	// 状態チェック
	if (!CheckReady()) {
		return 0;
	}

	// トラック検索
	index = SearchTrack(block);

	// 無効なら、範囲外
	if (index < 0) {
		disk.code = DISK_INVALIDLBA;
		return 0;
	}
	ASSERT(track[index]);

	// 現在のデータトラックと異なっていれば
	if (dataindex != index) {
		// 現在のディスクキャッシュを削除(Saveの必要はない)
		delete disk.dcache;
		disk.dcache = NULL;

		// ブロック数を再設定
		disk.blocks = track[index]->GetBlocks();
		ASSERT(disk.blocks > 0);

		// ディスクキャッシュを作り直す
		track[index]->GetPath(path);
		disk.dcache = new DiskCache(path, disk.size, disk.blocks);
		disk.dcache->SetRawMode(rawfile);

		// データインデックスを再設定
		dataindex = index;
	}

	// 基本クラス
	ASSERT(dataindex >= 0);
	return Disk::Read(buf, block);
}

//---------------------------------------------------------------------------
//
//	READ TOC
//
//---------------------------------------------------------------------------
int FASTCALL SCSICD::ReadToc(const DWORD *cdb, BYTE *buf)
{
	int last;
	int index;
	int length;
	int loop;
	int i;
	BOOL msf;
	DWORD lba;

	ASSERT(this);
	ASSERT(cdb);
	ASSERT(cdb[0] == 0x43);
	ASSERT(buf);

	// レディチェック
	if (!CheckReady()) {
		return 0;
	}

	// レディであるなら、トラックが最低1つ以上存在する
	ASSERT(tracks > 0);
	ASSERT(track[0]);

	// アロケーションレングス取得、バッファクリア
	length = cdb[7] << 8;
	length |= cdb[8];
	memset(buf, 0, length);

	// MSFフラグ取得
	if (cdb[1] & 0x02) {
		msf = TRUE;
	}
	else {
		msf = FALSE;
	}

	// 最終トラック番号を取得、チェック
	last = track[tracks - 1]->GetTrackNo();
	if ((int)cdb[6] > last) {
		// ただしAAは除外
		if (cdb[6] != 0xaa) {
			disk.code = DISK_INVALIDCDB;
			return 0;
		}
	}

	// 開始インデックスをチェック
	index = 0;
	if (cdb[6] != 0x00) {
		// トラック番号が一致するまで、トラックを進める
		while (track[index]) {
			if ((int)cdb[6] == track[index]->GetTrackNo()) {
				break;
			}
			index++;
		}

		// 見つからなければAAか、内部エラー
		if (!track[index]) {
			if (cdb[6] == 0xaa) {
				// AAなので、最終LBA+1を返す
				buf[0] = 0x00;
				buf[1] = 0x0a;
				buf[2] = (BYTE)track[0]->GetTrackNo();
				buf[3] = (BYTE)last;
				buf[6] = 0xaa;
				lba = track[tracks -1]->GetLast() + 1;
				if (msf) {
					LBAtoMSF(lba, &buf[8]);
				}
				else {
					buf[10] = (BYTE)(lba >> 8);
					buf[11] = (BYTE)lba;
				}
				return length;
			}

			// それ以外はエラー
			disk.code = DISK_INVALIDCDB;
			return 0;
		}
	}

	// 今回返すトラックディスクリプタの個数(ループ数)
	loop = last - track[index]->GetTrackNo() + 1;
	ASSERT(loop >= 1);

	// ヘッダ作成
	buf[0] = (BYTE)(((loop << 3) + 2) >> 8);
	buf[1] = (BYTE)((loop << 3) + 2);
	buf[2] = (BYTE)track[0]->GetTrackNo();
	buf[3] = (BYTE)last;
	buf += 4;

	// ループ
	for (i=0; i<loop; i++) {
		// ADRとControl
		if (track[index]->IsAudio()) {
			// オーディオトラック
			buf[1] = 0x10;
		}
		else {
			// データトラック
			buf[1] = 0x14;
		}

		// トラック番号
		buf[2] = (BYTE)track[index]->GetTrackNo();

		// トラックアドレス
		if (msf) {
			LBAtoMSF(track[index]->GetFirst(), &buf[4]);
		}
		else {
			buf[6] = (BYTE)(track[index]->GetFirst() >> 8);
			buf[7] = (BYTE)(track[index]->GetFirst());
		}

		// バッファとインデックスを進める
		buf += 8;
		index++;
	}

	// アロケーションレングスだけ必ず返す
	return length;
}

//---------------------------------------------------------------------------
//
//	PLAY AUDIO
//
//---------------------------------------------------------------------------
BOOL FASTCALL SCSICD::PlayAudio(const DWORD *cdb)
{
	ASSERT(this);

	disk.code = DISK_INVALIDCDB;
	return FALSE;
}

//---------------------------------------------------------------------------
//
//	PLAY AUDIO MSF
//
//---------------------------------------------------------------------------
BOOL FASTCALL SCSICD::PlayAudioMSF(const DWORD *cdb)
{
	ASSERT(this);

	disk.code = DISK_INVALIDCDB;
	return FALSE;
}

//---------------------------------------------------------------------------
//
//	PLAY AUDIO TRACK
//
//---------------------------------------------------------------------------
BOOL FASTCALL SCSICD::PlayAudioTrack(const DWORD *cdb)
{
	ASSERT(this);

	disk.code = DISK_INVALIDCDB;
	return FALSE;
}

//---------------------------------------------------------------------------
//
//	LBA→MSF変換
//
//---------------------------------------------------------------------------
void FASTCALL SCSICD::LBAtoMSF(DWORD lba, BYTE *msf) const
{
	DWORD m;
	DWORD s;
	DWORD f;

	ASSERT(this);

	// 75、75*60でそれぞれ余りを出す
	m = lba / (75 * 60);
	s = lba % (75 * 60);
	f = s % 75;
	s /= 75;

	// 基点はM=0,S=2,F=0
	s += 2;
	if (s >= 60) {
		s -= 60;
		m++;
	}

	// 格納
	ASSERT(m < 0x100);
	ASSERT(s < 60);
	ASSERT(f < 75);
	msf[0] = 0x00;
	msf[1] = (BYTE)m;
	msf[2] = (BYTE)s;
	msf[3] = (BYTE)f;
}

//---------------------------------------------------------------------------
//
//	MSF→LBA変換
//
//---------------------------------------------------------------------------
DWORD FASTCALL SCSICD::MSFtoLBA(const BYTE *msf) const
{
	DWORD lba;

	ASSERT(this);
	ASSERT(msf[2] < 60);
	ASSERT(msf[3] < 75);

	// 1, 75, 75*60の倍数で合算
	lba = msf[1];
	lba *= 60;
	lba += msf[2];
	lba *= 75;
	lba += msf[3];

	// 基点はM=0,S=2,F=0なので、150を引く
	lba -= 150;

	return lba;
}

//---------------------------------------------------------------------------
//
//	トラッククリア
//
//---------------------------------------------------------------------------
void FASTCALL SCSICD::ClearTrack()
{
	int i;

	ASSERT(this);

	// トラックオブジェクトを削除
	for (i=0; i<TrackMax; i++) {
		if (track[i]) {
			delete track[i];
			track[i] = NULL;
		}
	}

	// トラック数0
	tracks = 0;

	// データ、オーディオとも設定なし
	dataindex = -1;
	audioindex = -1;
}

//---------------------------------------------------------------------------
//
//	トラック検索
//	※見つからなければ-1を返す
//
//---------------------------------------------------------------------------
int FASTCALL SCSICD::SearchTrack(DWORD lba) const
{
	int i;

	ASSERT(this);

	// トラックループ
	for (i=0; i<tracks; i++) {
		// トラックに聞く
		ASSERT(track[i]);
		if (track[i]->IsValid(lba)) {
			return i;
		}
	}

	// 見つからなかった
	return -1;
}

//---------------------------------------------------------------------------
//
//	フレーム通知
//
//---------------------------------------------------------------------------
BOOL FASTCALL SCSICD::NextFrame()
{
	ASSERT(this);
	ASSERT((frame >= 0) && (frame < 75));

	// フレームを0～74の範囲で設定
	frame = (frame + 1) % 75;

	// 1周したらFALSE
	if (frame != 0) {
		return TRUE;
	}
	else {
		return FALSE;
	}
}

//---------------------------------------------------------------------------
//
//	CD-DAバッファ取得
//
//---------------------------------------------------------------------------
void FASTCALL SCSICD::GetBuf(DWORD *buffer, int samples, DWORD rate)
{
	ASSERT(this);
}
