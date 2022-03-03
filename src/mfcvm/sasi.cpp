//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 ＰＩ．(ytanaka@ipc-tokai.or.jp)
//	[ SASI ]
//
//---------------------------------------------------------------------------

#include "os.h"
#include "xm6.h"
#include "vm.h"
#include "log.h"
#include "schedule.h"
#include "fileio.h"
#include "dmac.h"
#include "iosc.h"
#include "sram.h"
#include "config.h"
#include "disk.h"
#include "memory.h"
#include "scsi.h"
#include "sasi.h"

//===========================================================================
//
//	SASI
//
//===========================================================================
//#define SASI_LOG

//---------------------------------------------------------------------------
//
//	コンストラクタ
//
//---------------------------------------------------------------------------
SASI::SASI(VM *p) : MemDevice(p)
{
	// デバイスIDを初期化
	dev.id = MAKEID('S', 'A', 'S', 'I');
	dev.desc = "SASI (IOSC-2)";

	// 開始アドレス、終了アドレス
	memdev.first = 0xe96000;
	memdev.last = 0xe97fff;
}

//---------------------------------------------------------------------------
//
//	初期化
//
//---------------------------------------------------------------------------
BOOL FASTCALL SASI::Init()
{
	int i;

	ASSERT(this);

	// 基本クラス
	if (!MemDevice::Init()) {
		return FALSE;
	}

	// DMAC取得
	dmac = (DMAC*)vm->SearchDevice(MAKEID('D', 'M', 'A', 'C'));
	ASSERT(dmac);

	// IOSC取得
	iosc = (IOSC*)vm->SearchDevice(MAKEID('I', 'O', 'S', 'C'));
	ASSERT(iosc);

	// SRAM取得
	sram = (SRAM*)vm->SearchDevice(MAKEID('S', 'R', 'A', 'M'));
	ASSERT(sram);

	// SCSI取得
	scsi = (SCSI*)vm->SearchDevice(MAKEID('S', 'C', 'S', 'I'));
	ASSERT(scsi);

	// 内部データ初期化
	memset(&sasi, 0, sizeof(sasi));
	sxsicpu = FALSE;

	// ディスク作成
	for (i=0; i<SASIMax; i++) {
		sasi.disk[i] = new Disk(this);
	}

	// カレントなし
	sasi.current = NULL;

	// イベント作成
	event.SetDevice(this);
	event.SetDesc("Data Transfer");
	event.SetUser(0);
	event.SetTime(0);
	scheduler->AddEvent(&event);

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	クリーンアップ
//
//---------------------------------------------------------------------------
void FASTCALL SASI::Cleanup()
{
	int i;

	ASSERT(this);

	// ディスクを削除
	for (i=0; i<SASIMax; i++) {
		if (sasi.disk[i]) {
			delete sasi.disk[i];
			sasi.disk[i] = NULL;
		}
	}

	// 基本クラスへ
	MemDevice::Cleanup();
}

//---------------------------------------------------------------------------
//
//	リセット
//
//---------------------------------------------------------------------------
void FASTCALL SASI::Reset()
{
	Memory *memory;
	Memory::memtype type;

	ASSERT(this);
	LOG0(Log::Normal, "リセット");

	// SCSIタイプ(メモリに問い合わせる)
	memory = (Memory*)vm->SearchDevice(MAKEID('M', 'E', 'M', ' '));
	ASSERT(memory);
	type = memory->GetMemType();
	switch (type) {
		// SASIのみ
		case Memory::None:
		case Memory::SASI:
			sasi.scsi_type = 0;
			break;

		// 外付
		case Memory::SCSIExt:
			sasi.scsi_type = 1;
			break;

		// その他(内蔵)
		default:
			sasi.scsi_type = 2;
			break;
	}

	// SCSIを使う場合、SxSIとは排他とする(SxSI禁止)
	if (sasi.scsi_type != 0) {
		if (sasi.sxsi_drives != 0) {
			sasi.sxsi_drives = 0;
			Construct();
		}
	}

	// メモリスイッチ書き込み
	if (sasi.memsw) {
		// SASIが存在する場合のみ、セット
		if (sasi.scsi_type < 2) {
			// $ED005A:SASIディスク数
			sram->SetMemSw(0x5a, sasi.sasi_drives);
		}
		else {
			// SASIインタフェースがないので0
			sram->SetMemSw(0x5a, 0x00);
		}

		// SCSIが存在しない場合のみ、クリア
		if (sasi.scsi_type == 0) {
			// $ED006F:SCSIフラグ('V'でSCSI有効)
			sram->SetMemSw(0x6f, 0x00);
			// $ED0070:SCSI種別(本体/外付)+本体SCSI ID
			sram->SetMemSw(0x70, 0x07);
			// $ED0071:SCSIによるSASIエミュレーションフラグ
			sram->SetMemSw(0x71, 0x00);
		}
	}

	// イベントリセット
	event.SetUser(0);
	event.SetTime(0);

	// バスリセット
	BusFree();

	// カレントデバイスなし
	sasi.current = NULL;
}

//---------------------------------------------------------------------------
//
//	セーブ
//
//---------------------------------------------------------------------------
BOOL FASTCALL SASI::Save(Fileio *fio, int ver)
{
	size_t sz;
	int i;

	ASSERT(this);
	ASSERT(fio);

	LOG0(Log::Normal, "セーブ");

	// ディスクをフラッシュ
	for (i=0; i<SASIMax; i++) {
		ASSERT(sasi.disk[i]);
		if (!sasi.disk[i]->Flush()) {
			return FALSE;
		}
	}

	// サイズをセーブ
	sz = sizeof(sasi_t);
	if (!fio->Write(&sz, sizeof(sz))) {
		return FALSE;
	}

	// 実体をセーブ
	if (!fio->Write(&sasi, (int)sz)) {
		return FALSE;
	}

	// イベントをセーブ
	if (!event.Save(fio, ver)) {
		return FALSE;
	}

	// パスをセーブ
	for (i=0; i<SASIMax; i++) {
		if (!sasihd[i].Save(fio, ver)) {
			return FALSE;
		}
	}
	for (i=0; i<SCSIMax; i++) {
		if (!scsihd[i].Save(fio, ver)) {
			return FALSE;
		}
	}
	if (!scsimo.Save(fio, ver)) {
		return FALSE;
	}

	// version2.02拡張
	if (!fio->Write(&sxsicpu, sizeof(sxsicpu))) {
		return FALSE;
	}

	// version2.03拡張
	for (i=0; i<SASIMax; i++) {
		ASSERT(sasi.disk[i]);
		if (!sasi.disk[i]->Save(fio, ver)) {
			return FALSE;
		}
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	ロード
//
//---------------------------------------------------------------------------
BOOL FASTCALL SASI::Load(Fileio *fio, int ver)
{
	sasi_t buf;
	size_t sz;
	int i;

	ASSERT(this);
	ASSERT(fio);

	LOG0(Log::Normal, "ロード");

	// サイズをロード、照合
	if (!fio->Read(&sz, sizeof(sz))) {
		return FALSE;
	}
	if (sz != sizeof(sasi_t)) {
		return FALSE;
	}

	// バッファへ実体をロード
	if (!fio->Read(&buf, (int)sz)) {
		return FALSE;
	}

	// ディスクを初期化
	for (i=0; i<SASIMax; i++) {
		ASSERT(sasi.disk[i]);
		delete sasi.disk[i];
		sasi.disk[i] = new Disk(this);
	}

	// ポインタを移す
	for (i=0; i<SASIMax; i++) {
		buf.disk[i] = sasi.disk[i];
	}

	// 移動
	sasi = buf;
	sasi.mo = NULL;
	sasi.current = NULL;

	// イベントをロード
	if (!event.Load(fio, ver)) {
		return FALSE;
	}

	// パスをロード
	for (i=0; i<SASIMax; i++) {
		if (!sasihd[i].Load(fio, ver)) {
			return FALSE;
		}
	}
	for (i=0; i<SCSIMax; i++) {
		if (!scsihd[i].Load(fio, ver)) {
			return FALSE;
		}
	}
	if (!scsimo.Load(fio, ver)) {
		return FALSE;
	}

	// version2.02拡張
	sxsicpu = FALSE;
	if (ver >= 0x0202) {
		if (!fio->Read(&sxsicpu, sizeof(sxsicpu))) {
			return FALSE;
		}
	}

	// ディスクを作り直す
	Construct();

	// version2.03拡張
	if (ver >= 0x0203) {
		for (i=0; i<SASIMax; i++) {
			ASSERT(sasi.disk[i]);
			if (!sasi.disk[i]->Load(fio, ver)) {
				return FALSE;
			}
		}
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	設定適用
//
//---------------------------------------------------------------------------
void FASTCALL SASI::ApplyCfg(const Config *config)
{
	int i;

	ASSERT(this);
	ASSERT(config);

	LOG0(Log::Normal, "設定適用");

	// SASIドライブ数
	sasi.sasi_drives = config->sasi_drives;

	// メモリスイッチ自動更新
	sasi.memsw = config->sasi_sramsync;

	// SASIファイル名
	for (i=0; i<SASIMax; i++) {
		sasihd[i].SetPath(config->sasi_file[i]);
	}

	// パリティ回路付加
	sasi.parity = config->sasi_parity;

	// SCSIドライブ数
	sasi.sxsi_drives = config->sxsi_drives;

	// SCSIファイル名
	for (i=0; i<SCSIMax; i++) {
		scsihd[i].SetPath(config->sxsi_file[i]);
	}

	// MO優先フラグ
	sasi.mo_first = config->sxsi_mofirst;

	// ディスク再構築
	Construct();
}

//---------------------------------------------------------------------------
//
//	バイト読み込み
//
//---------------------------------------------------------------------------
DWORD FASTCALL SASI::ReadByte(DWORD addr)
{
	DWORD data;

	ASSERT(this);
	ASSERT((addr >= memdev.first) && (addr <= memdev.last));

	// SCSI内蔵か
	if (sasi.scsi_type >= 2) {
		// 0x40単位でループ
		addr &= 0x3f;

		// 領域チェック
		if (addr >= 0x20) {
			// SCSI領域
			return scsi->ReadByte(addr - 0x20);
		}
		if ((addr & 1) == 0) {
			// 偶数バイトはデコードされていない
			return 0xff;
		}
		addr &= 0x07;
		if (addr >= 4) {
			return 0xff;
		}
		return 0;
	}

	// 奇数アドレスのみ
	if (addr & 1) {
		// 8バイト単位でループ
		addr &= 0x07;

		// ウェイト
		scheduler->Wait(1);

		// データレジスタ
		if (addr == 1) {
			return ReadData();
		}

		// ステータスレジスタ
		if (addr == 3) {
			data = 0;
			if (sasi.msg) {
				data |= 0x10;
			}
			if (sasi.cd) {
				data |= 0x08;
			}
			if (sasi.io) {
				data |= 0x04;
			}
			if (sasi.bsy) {
				data |= 0x02;
			}
			if (sasi.req) {
				data |= 0x01;
			}
			return data;
		}

		// それ以外のレジスタはWrite Only
		return 0xff;
	}

	// 偶数アドレスはデコードされていない
	return 0xff;
}

//---------------------------------------------------------------------------
//
//	ワード読み込み
//
//---------------------------------------------------------------------------
DWORD FASTCALL SASI::ReadWord(DWORD addr)
{
	ASSERT(this);
	ASSERT((addr >= memdev.first) && (addr <= memdev.last));
	ASSERT((addr & 1) == 0);

	return (0xff00 | ReadByte(addr + 1));
}

//---------------------------------------------------------------------------
//
//	バイト書き込み
//
//---------------------------------------------------------------------------
void FASTCALL SASI::WriteByte(DWORD addr, DWORD data)
{
	ASSERT(this);
	ASSERT((addr >= memdev.first) && (addr <= memdev.last));
	ASSERT(data < 0x100);

	// SCSI内蔵か
	if (sasi.scsi_type >= 2) {
		// 0x40単位でループ
		addr &= 0x3f;

		// 領域チェック
		if (addr >= 0x20) {
			// SCSI領域
			scsi->WriteByte(addr - 0x20, data);
		}
		// SASI領域は、何もしない
		return;
	}

	// 奇数アドレスのみ
	if (addr & 1) {
		// 8バイト単位でループ
		addr &= 0x07;
		addr >>= 1;

		// ウェイト
		scheduler->Wait(1);

		switch (addr) {
			// データ書き込み
			case 0:
				WriteData(data);
				return;

			// SELつきデータ書き込み
			case 1:
				sasi.sel = FALSE;
				WriteData(data);
				return;

			// バスリセット
			case 2:
#if defined(SASI_LOG)
				LOG0(Log::Normal, "バスリセット");
#endif	// SASI_LOG
				BusFree();
				return;

			// SELつきデータ書き込み
			case 3:
				sasi.sel = TRUE;
				WriteData(data);
				return;
		}

		// ここには来ない
		ASSERT(FALSE);
		return;
	}

	// 偶数アドレスはデコードされていない
}

//---------------------------------------------------------------------------
//
//	ワード書き込み
//
//---------------------------------------------------------------------------
void FASTCALL SASI::WriteWord(DWORD addr, DWORD data)
{
	ASSERT(this);
	ASSERT((addr >= memdev.first) && (addr <= memdev.last));
	ASSERT((addr & 1) == 0);
	ASSERT(data < 0x10000);

	WriteByte(addr + 1, (BYTE)data);
}

//---------------------------------------------------------------------------
//
//	読み込みのみ
//
//---------------------------------------------------------------------------
DWORD FASTCALL SASI::ReadOnly(DWORD addr) const
{
	DWORD data;

	ASSERT(this);
	ASSERT((addr >= memdev.first) && (addr <= memdev.last));

	// SCSI内蔵か
	if (sasi.scsi_type >= 2) {
		// 0x40単位でループ
		addr &= 0x3f;

		// 領域チェック
		if (addr >= 0x20) {
			// SCSI領域
			return scsi->ReadOnly(addr - 0x20);
		}
		if ((addr & 1) == 0) {
			// 偶数バイトはデコードされていない
			return 0xff;
		}
		addr &= 0x07;
		if (addr >= 4) {
			return 0xff;
		}
		return 0;
	}

	// 奇数アドレスのみ
	if (addr & 1) {
		// 8バイト単位でループ
		addr &= 0x07;
		addr >>= 1;

		switch (addr) {
			// データレジスタ
			case 0:
				return 0;

			// ステータスレジスタ
			case 1:
				data = 0;
				if (sasi.msg) {
					data |= 0x10;
				}
				if (sasi.cd) {
					data |= 0x08;
				}
				if (sasi.io) {
					data |= 0x04;
				}
				if (sasi.bsy) {
					data |= 0x02;
				}
				if (sasi.req) {
					data |= 0x01;
				}
				return data;
		}

		// それ以外のレジスタはWrite Only
		return 0xff;
	}

	// 偶数アドレスはデコードされていない
	return 0xff;
}

//---------------------------------------------------------------------------
//
//	イベントコールバック
//
//---------------------------------------------------------------------------
BOOL FASTCALL SASI::Callback(Event *ev)
{
	int thres;

	ASSERT(this);
	ASSERT(ev);

	switch (ev->GetUser()) {
		// 通常データ ホスト←→コントローラ
		case 0:
			// 時間調整
			if (ev->GetTime() != 32) {
				ev->SetTime(32);
			}

			// リクエスト
			sasi.req = TRUE;
			dmac->ReqDMA(1);

			// DMA転送の結果によっては続く
			return TRUE;

		// ブロックデータ コントローラ←→ホスト
		case 1:
			// 時間を再設定
			if (ev->GetTime() != 48) {
				ev->SetTime(48);
			}

			// DMAがアクティブでなければ、リクエスト設定のみ
			if (!dmac->IsAct(1)) {
				if ((sasi.phase == read) || (sasi.phase == write)) {
					sasi.req = TRUE;
				}

				// イベント継続
				return TRUE;
			}

			// 1回のイベントで、余剰CPUパワーの2/3だけ転送する
			thres = (int)scheduler->GetCPUSpeed();
			thres = (thres * 2) / 3;
			while ((sasi.phase == read) || (sasi.phase == write)) {
				// CPUパワーを見ながら途中で打ち切る
				if (scheduler->GetCPUCycle() > thres) {
					break;
				}

				// リクエスト設定
				sasi.req = TRUE;

				// SxSI CPUフラグが設定されていれば、ここまで(CPU転送させる)
				if (sxsicpu) {
					LOG0(Log::Warning, "SxSI CPU転送を検出");
					break;
				}

				// DMAリクエスト
				dmac->ReqDMA(1);
				if (sasi.req) {
#if defined(SASI_LOG)
					LOG0(Log::Normal, "DMA or CPU転送待ち");
#endif
					break;
				}
			}
			return TRUE;

		// それ以外はありえない
		default:
			ASSERT(FALSE);
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	内部データ取得
//
//---------------------------------------------------------------------------
void FASTCALL SASI::GetSASI(sasi_t *buffer) const
{
	ASSERT(this);
	ASSERT(buffer);

	// 内部ワークをコピー
	*buffer = sasi;
}

//---------------------------------------------------------------------------
//
//	ディスク再構築
//
//---------------------------------------------------------------------------
void FASTCALL SASI::Construct()
{
	int i;
	int index;
	int scmo;
	int schd;
	int map[SASIMax];
	Filepath path;
	Filepath mopath;
	BOOL moready;
	BOOL mowritep;
	BOOL moattn;
	SASIHD *sasitmp;
	SCSIHD *scsitmp;

	ASSERT(this);
	ASSERT((sasi.sasi_drives >= 0) && (sasi.sasi_drives <= SASIMax));
	ASSERT((sasi.sxsi_drives >= 0) && (sasi.sxsi_drives <= 7));

	// MOドライブの状態を保存し、一旦禁止
	moready = FALSE;
	moattn = FALSE;
	mowritep = FALSE;
	if (sasi.mo) {
		moready = sasi.mo->IsReady();
		if (moready) {
			sasi.mo->GetPath(mopath);
			mowritep = sasi.mo->IsWriteP();
			moattn = sasi.mo->IsAttn();
		}
	}
	sasi.mo = NULL;

	// 有効なSCSI-MO, SCSI-HD数を算出
	i = (sasi.sasi_drives + 1) >> 1;
	schd = 7 - i;

	// SASI制限から可能な最大SCSIドライブ数
	if (schd > 0) {
		// 設定のSCSIドライブ数に制限される
		if (sasi.sxsi_drives < schd) {
			// SCSIドライブ数確定
			schd = sasi.sxsi_drives;
		}
	}

	// MOの割り当て
	if (schd > 0) {
		// ドライブが1台以上あれば、必ずMOを割り当てる
		scmo = 1;
		schd--;
	}
	else {
		// ドライブがないので、MO,HDとも0
		scmo = 0;
		schd = 0;
	}

	// パリティがなければSxSI使用不可
	if (!sasi.parity) {
		scmo = 0;
		schd = 0;
	}
#if defined(SASI_LOG)
	LOG3(Log::Normal, "再割り当て SASI-HD:%d台 SCSI-HD:%d台 SCSI-MO:%d台",
					sasi.sasi_drives, schd, scmo);
#endif	// SASI_LOG

	// マップを作成(0:NULL 1:SASI-HD 2:SCSI-HD 3:SCSI-MO)
	for (i=0; i<SASIMax; i++) {
		// すべてNULL
		map[i] = 0;
	}
	for (i=0; i<sasi.sasi_drives; i++) {
		// 先にSASI
		map[i] = 1;
	}
	if (scmo > 0) {
		// SCSIあり
		index = ((sasi.sasi_drives + 1) >> 1) << 1;
		if (sasi.mo_first) {
			// MO優先
			map[index] = 3;
			index += 2;

			// SCSI-HDループ
			for (i=0; i<schd; i++) {
				map[index] = 2;
				index += 2;
			}
		}
		else {
			// HD優先
			for (i=0; i<schd; i++) {
				map[index] = 2;
				index += 2;
			}

			// 最後にMO
			map[index] = 3;
		}
		ASSERT(index <= 16);
	}

	// SCSIハードディスクの連番インデックスをクリア
	index = 0;

	// ループ
	for (i=0; i<SASIMax; i++) {
		switch (map[i]) {
			// ヌルディスク
			case 0:
				// ヌルディスクか
				if (!sasi.disk[i]->IsNULL()) {
					// ヌルディスクに差し替える
					delete sasi.disk[i];
					sasi.disk[i] = new Disk(this);
				}
				break;

			// SASIハードディスク
			case 1:
				// SASIハードディスクか
				if (sasi.disk[i]->IsSASI()) {
					// パスを取得し、一致すればok
					sasi.disk[i]->GetPath(path);
					if (path.CmpPath(sasihd[i])) {
						// パスが一致している
						break;
					}
				}

				// SASIハードディスクを作成してオープンを試みる
				sasitmp = new SASIHD(this);
				if (sasitmp->Open(sasihd[i])) {
					// LUN設定
					sasitmp->SetLUN(i & 1);
					// 入れ替え
					delete sasi.disk[i];
					sasi.disk[i] = sasitmp;
				}
				else {
					// エラー
					delete sasitmp;
					delete sasi.disk[i];
					sasi.disk[i] = new Disk(this);
				}
				break;

			// SCSIハードディスク
			case 2:
				// SCSIハードディスクか
				if (sasi.disk[i]->GetID() == MAKEID('S', 'C', 'H', 'D')) {
					// パスを取得し、一致すればok
					sasi.disk[i]->GetPath(path);
					if (path.CmpPath(scsihd[index])) {
						// パスが一致している
						index++;
						break;
					}
				}

				// SCSIハードディスクを作成してオープンを試みる
				scsitmp = new SCSIHD(this);
				if (scsitmp->Open(scsihd[index])) {
					// 入れ替え
					delete sasi.disk[i];
					sasi.disk[i] = scsitmp;
				}
				else {
					// エラー
					delete scsitmp;
					delete sasi.disk[i];
					sasi.disk[i] = new Disk(this);
				}
				index++;
				break;

			// SCSI光磁気ディスク
			case 3:
				// SCSI光磁気ディスクか
				if (sasi.disk[i]->GetID() == MAKEID('S', 'C', 'M', 'O')) {
					// 記憶
					sasi.mo = (SCSIMO*)sasi.disk[i];
				}
				else {
					// SCSI光磁気ディスクを作成して記憶
					delete sasi.disk[i];
					sasi.disk[i] = new SCSIMO(this);
					sasi.mo = (SCSIMO*)sasi.disk[i];
				}

				// 引き継がれていなければ、再オープン
				if (moready) {
					if (!sasi.mo->IsReady()) {
						if (sasi.mo->Open(mopath, moattn)) {
							sasi.mo->WriteP(mowritep);
						}
					}
				}
				break;

			// その他(あり得ない)
			default:
				ASSERT(FALSE);
				break;
		}
	}

	// BUSY中なら、sasi.currentを更新
	if (sasi.bsy) {
		// ドライブ作成
		ASSERT(sasi.ctrl < 8);
		index = sasi.ctrl << 1;
		if (sasi.cmd[1] & 0x20) {
			index++;
		}

		// カレント設定
		sasi.current = sasi.disk[index];
		ASSERT(sasi.current);
	}
}

//---------------------------------------------------------------------------
//
//	HD BUSYチェック
//
//---------------------------------------------------------------------------
BOOL FASTCALL SASI::IsBusy() const
{
	ASSERT(this);

	// SASI
	if (sasi.bsy) {
		return TRUE;
	}

	// SCSI
	if (scsi->IsBusy()) {
		return TRUE;
	}

	// どちらもFALSE
	return FALSE;
}

//---------------------------------------------------------------------------
//
//	BUSYデバイス取得
//
//---------------------------------------------------------------------------
DWORD FASTCALL SASI::GetBusyDevice() const
{
	ASSERT(this);

	// SASI
	if (sasi.bsy) {
		// セレクションフェーズ終了から実行フェーズ開始までの間は
		// アクセス対象デバイスが確定しない
		if (!sasi.current) {
			return 0;
		}
		if (sasi.current->IsNULL()) {
			return 0;
		}

		return sasi.current->GetID();
	}

	// SCSI
	return scsi->GetBusyDevice();
}

//---------------------------------------------------------------------------
//
//	データ読み込み
//
//---------------------------------------------------------------------------
DWORD FASTCALL SASI::ReadData()
{
	DWORD data;

	ASSERT(this);

	switch (sasi.phase) {
		// ステータスフェーズなら、ステータスを渡してメッセージフェーズへ
		case status:
			data = sasi.status;
			sasi.req = FALSE;

			// 割り込みリセットを兼ねる
			iosc->IntHDC(FALSE);
			Message();

			// イベント停止
			event.SetTime(0);
			return data;

		// メッセージフェーズなら、メッセージを渡してバスフリーへ
		case message:
			data = sasi.message;
			sasi.req = FALSE;
			BusFree();

			// イベント停止
			event.SetTime(0);
			return data;

		// 読み込みフェーズなら、転送後ステータスフェーズへ
		case read:
			// Non-DMA転送なら、SxSI CPUフラグをトグル変化させる
			if (!dmac->IsDMA()) {
#if defined(SASI_LOG)
				LOG1(Log::Normal, "データINフェーズ CPU転送 オフセット=%d", sasi.offset);
#endif	// SASI_LOG
				sxsicpu = !sxsicpu;
			}

			data = (DWORD)sasi.buffer[sasi.offset];
			sasi.offset++;
			sasi.length--;
			sasi.req = FALSE;

			// レングスチェック
			if (sasi.length == 0) {
				// 転送すべきブロック数を下げる
				sasi.blocks--;

				// 今回で終了か
				if (sasi.blocks == 0) {
					// イベント停止、ステータスフェーズ
					event.SetTime(0);
					Status();
					return data;
				}

				// 次のブロックを読む
				sasi.length = sasi.current->Read(sasi.buffer, sasi.next);
				if (sasi.length <= 0) {
					// エラー
					Error();
					return data;
				}

				// 次のブロックok、ワーク設定
				sasi.offset = 0;
				sasi.next++;
			}

			// 読み込み継続
			return data;
	}

	// イベント停止
	event.SetTime(0);

	// それ以外のオペレーションは想定せず
	if (sasi.phase == busfree) {
#if defined(SASI_LOG)
		LOG0(Log::Normal, "バスフリー状態での読み込み。0を返す");
#endif	// SASI_LOG
		return 0;
	}

	LOG0(Log::Warning, "想定していないデータ読み込み");
	BusFree();
	return 0;
}

//---------------------------------------------------------------------------
//
//	データ書き込み
//
//---------------------------------------------------------------------------
void FASTCALL SASI::WriteData(DWORD data)
{
	ASSERT(this);
	ASSERT(data < 0x100);

	switch (sasi.phase) {
		// バスフリーフェーズなら、次に許されるのはセレクションフェーズ
		case busfree:
			if (sasi.sel) {
				Selection(data);
			}
#if defined(SASI_LOG)
			else {
				LOG0(Log::Normal, "バスフリー状態、SEL無しでの書き込み。無視");
			}
#endif	// SASI_LOG
			event.SetTime(0);
			return;

		// セレクションフェーズなら、SELを戻してコマンドフェーズへ
		case selection:
			if (!sasi.sel) {
				Command();
				return;
			}
			event.SetTime(0);
			break;

		// コマンドフェーズなら、6/10バイト受信
		case command:
			// 最初のデータ(オフセット0)によりレングスを再設定
			sasi.cmd[sasi.offset] = data;
			if (sasi.offset == 0) {
				if ((data >= 0x20) && (data <= 0x3f)) {
					// 10バイトCDB
					sasi.length = 10;
				}
			}
			sasi.offset++;
			sasi.length--;
			sasi.req = FALSE;

			// 終わりか
			if (sasi.length == 0) {
				// イベント停止して実行フェーズへ
				event.SetTime(0);
				Execute();
				return;
			}
			// イベント継続
			return;

		// 書き込みフェーズなら、転送後ステータスフェーズへ
		case write:
			// Non-DMA転送なら、SxSI CPUフラグをトグル変化させる
			if (!dmac->IsDMA()) {
#if defined(SASI_LOG)
				LOG1(Log::Normal, "データOUTフェーズ CPU転送 オフセット=%d", sasi.offset);
#endif	// SASI_LOG
				sxsicpu = !sxsicpu;
			}

			sasi.buffer[sasi.offset] = (BYTE)data;
			sasi.offset++;
			sasi.length--;
			sasi.req = FALSE;

			// 終わりか
			if (sasi.length > 0) {
				return;
			}

			// WRITE(6)・WRITE(10)・WRITE&VERIFYは別処理
			switch (sasi.cmd[0]) {
				case 0x0a:
				case 0x2a:
				case 0x2e:
					break;
				// WRITE DATA以外のコマンドはここで終了
				default:
					event.SetTime(0);
					Status();
					return;
			}

			// WRITEコマンドなら、現在のバッファで書き込み
			if (!sasi.current->Write(sasi.buffer, sasi.next - 1)) {
				// エラー
				Error();
				return;
			}

			// 転送ブロック数を下げる
			sasi.blocks--;

			// 最後のブロックか
			if (sasi.blocks == 0) {
				// イベント停止してステータスフェーズへ
				event.SetTime(0);
				Status();
				return;
			}

			// 次のブロック準備
			sasi.length = sasi.current->WriteCheck(sasi.next);
			if (sasi.length <= 0) {
				// エラー
				Error();
				return;
			}

			// 次のブロックを転送
			sasi.next++;
			sasi.offset = 0;

			// イベント継続
			return;
	}

	// イベント停止
	event.SetTime(0);

	// それ以外のオペレーションは想定せず
	LOG1(Log::Warning, "想定していないデータ書き込み $%02X", data);
	BusFree();
}

//---------------------------------------------------------------------------
//
//	バスフリーフェーズ
//
//---------------------------------------------------------------------------
void FASTCALL SASI::BusFree()
{
	ASSERT(this);

#if defined(SASI_LOG)
	LOG0(Log::Normal, "バスフリーフェーズ");
#endif	// SASI_LOG

	// バスリセット
	sasi.msg = FALSE;
	sasi.cd = FALSE;
	sasi.io = FALSE;
	sasi.bsy = FALSE;
	sasi.req = FALSE;

	// バスフリーフェーズ
	sasi.phase = busfree;

	// イベントを停止
	event.SetTime(0);

	// 割り込みリセット
	iosc->IntHDC(FALSE);

	// SxSI CPU転送フラグ
	sxsicpu = FALSE;
}

//---------------------------------------------------------------------------
//
//	セレクションフェーズ
//
//---------------------------------------------------------------------------
void FASTCALL SASI::Selection(DWORD data)
{
	int c;
	int i;

	ASSERT(this);

	// セレクトされたコントローラを知る
	c = 8;
	for (i=0; i<8; i++) {
		if (data & 1) {
			c = i;
			break;
		}
		data >>= 1;
	}

	// ビットがなければバスフリーフェーズ
	if (c >= 8) {
		BusFree();
		return;
	}

#if defined(SASI_LOG)
	LOG1(Log::Normal, "セレクションフェーズ コントローラ%d", c);
#endif	// SASI_LOG

	// ドライブが存在しなければバスフリーフェーズ
	if (sasi.disk[(c << 1) + 0]->IsNULL()) {
		if (sasi.disk[(c << 1) + 1]->IsNULL()) {
			BusFree();
			return;
		}
	}

	// BSYを上げてセレクションフェーズ
	sasi.ctrl = (DWORD)c;
	sasi.phase = selection;
	sasi.bsy = TRUE;
}

//---------------------------------------------------------------------------
//
//	コマンドフェーズ
//
//---------------------------------------------------------------------------
void FASTCALL SASI::Command()
{
	ASSERT(this);

#if defined(SASI_LOG)
	LOG0(Log::Normal, "コマンドフェーズ");
#endif	// SASI_LOG

	// コマンドフェーズ
	sasi.phase = command;

	// I/O=0, C/D=1, MSG=0
	sasi.io = FALSE;
	sasi.cd = TRUE;
	sasi.msg = FALSE;

	// コマンド残り長さは6バイト(一部コマンドは10バイト。WriteByteで再設定)
	sasi.offset = 0;
	sasi.length = 6;

	// コマンドデータを要求
	event.SetUser(0);
	event.SetTime(32);
}

//---------------------------------------------------------------------------
//
//	実行フェーズ
//
//---------------------------------------------------------------------------
void FASTCALL SASI::Execute()
{
	int drive;

	ASSERT(this);

	// 実行フェーズ
	sasi.phase = execute;

	// ドライブ作成
	ASSERT(sasi.ctrl < 8);
	drive = sasi.ctrl << 1;
	if (sasi.cmd[1] & 0x20) {
		drive++;
	}

	// カレント設定
	sasi.current = sasi.disk[drive];
	ASSERT(sasi.current);

#if defined(SASI_LOG)
	LOG2(Log::Normal, "実行フェーズ コマンド$%02X ドライブ%d", sasi.cmd[0], drive);
#endif	// SASI_LOG

#if 0
	// アテンションならエラーを出す(MOのメディア入れ替えを通知)
	if (sasi.current->IsAttn()) {
#if defined(SASI_LOG)
	LOG0(Log::Normal, "メディア交換通知のためのアテンションエラー");
#endif	// SASI_LOG

		// アテンションをクリア
		sasi.current->ClrAttn();

		// エラー(リクエストセンスを要求)
		Error();
		return;
	}
#endif

	// 0x12以上0x2f以下のコマンドは、SCSI専用
	if ((sasi.cmd[0] >= 0x12) && (sasi.cmd[0] <= 0x2f)) {
		ASSERT(sasi.current);
		if (sasi.current->IsSASI()) {
			// SASIなので、未実装コマンドエラー
			sasi.current->InvalidCmd();
			Error();
			return;
		}
	}

	// コマンド別処理
	switch (sasi.cmd[0]) {
		// TEST UNIT READY
		case 0x00:
			TestUnitReady();
			return;

		// REZERO UNIT
		case 0x01:
			Rezero();
			return;

		// REQUEST SENSE
		case 0x03:
			RequestSense();
			return;

		// FORMAT UNIT
		case 0x04:
			Format();
			return;

		// FORMAT UNIT
		case 0x06:
			Format();
			return;

		// REASSIGN BLOCKS
		case 0x07:
			Reassign();
			return;

		// READ(6)
		case 0x08:
			Read6();
			return;

		// WRITE(6)
		case 0x0a:
			Write6();
			return;

		// SEEK(6)
		case 0x0b:
			Seek6();
			return;

		// ASSIGN(SASIのみ)
		case 0x0e:
			Assign();
			return;

		// INQUIRY(SCSIのみ)
		case 0x12:
			Inquiry();
			return;

		// MODE SENSE(SCSIのみ)
		case 0x1a:
			ModeSense();
			return;

		// START STOP UNIT(SCSIのみ)
		case 0x1b:
			StartStop();
			return;

		// PREVENT/ALLOW MEDIUM REMOVAL(SCSIのみ)
		case 0x1e:
			Removal();
			return;

		// READ CAPACITY(SCSIのみ)
		case 0x25:
			ReadCapacity();
			return;

		// READ(10)(SCSIのみ)
		case 0x28:
			Read10();
			return;

		// WRITE(10)(SCSIのみ)
		case 0x2a:
			Write10();
			return;

		// SEEK(10)(SCSIのみ)
		case 0x2b:
			Seek10();
			return;

		// WRITE and VERIFY(SCSIのみ)
		case 0x2e:
			Write10();
			return;

		// VERIFY(SCSIのみ)
		case 0x2f:
			Verify();
			return;

		// SPECIFY(SASIのみ)
		case 0xc2:
			Specify();
			return;
	}

	// それ以外は対応していない
	LOG1(Log::Warning, "未対応コマンド $%02X", sasi.cmd[0]);
	sasi.current->InvalidCmd();

	// エラー
	Error();
}

//---------------------------------------------------------------------------
//
//	ステータスフェーズ
//
//---------------------------------------------------------------------------
void FASTCALL SASI::Status()
{
	ASSERT(this);

#if defined(SASI_LOG)
	LOG1(Log::Normal, "ステータスフェーズ ステータス$%02X", sasi.status);
#endif	// SASI_LOG

	// ステータスフェーズ
	sasi.phase = status;

	// I/O=1 C/D=1 MSG=0
	sasi.io = TRUE;
	sasi.cd = TRUE;
	ASSERT(!sasi.msg);

	// ステータスデータの引き取りを要求
	sasi.req = TRUE;

	// 割り込みリクエスト
	iosc->IntHDC(TRUE);
}

//---------------------------------------------------------------------------
//
//	メッセージフェーズ
//
//---------------------------------------------------------------------------
void FASTCALL SASI::Message()
{
	ASSERT(this);

#if defined(SASI_LOG)
	LOG1(Log::Normal, "メッセージフェーズ メッセージ$%02X", sasi.message);
#endif	// SASI_LOG

	// メッセージフェーズ
	sasi.phase = message;

	// I/O=1 C/D=1 MSG=1
	ASSERT(sasi.io);
	ASSERT(sasi.cd);
	sasi.msg = 1;

	// ステータスデータの引き取りを要求
	sasi.req = TRUE;
}

//---------------------------------------------------------------------------
//
//	共通エラー
//
//---------------------------------------------------------------------------
void FASTCALL SASI::Error()
{
	ASSERT(this);

#if defined(SASI_LOG)
	LOG0(Log::Normal, "エラー(ステータスフェーズへ)");
#endif	// SASI_LOG

	// ステータスとメッセージを設定(CHECK CONDITION)
	sasi.status = (sasi.current->GetLUN() << 5) | 0x02;
	sasi.message = 0x00;

	// イベント停止
	event.SetTime(0);

	// ステータスフェーズ
	Status();
}

//---------------------------------------------------------------------------
//
//	TEST UNIT READY
//
//---------------------------------------------------------------------------
void FASTCALL SASI::TestUnitReady()
{
	BOOL status;

	ASSERT(this);
	ASSERT(sasi.current);

#if defined(SASI_LOG)
	LOG0(Log::Normal, "TEST UNIT READYコマンド");
#endif	// SASI_LOG

	// ドライブでコマンド処理
	status = sasi.current->TestUnitReady(sasi.cmd);
	if (!status) {
		// 失敗(エラー)
		Error();
		return;
	}

	// 成功
	sasi.status = 0x00;
	sasi.message = 0x00;

	// ステータスフェーズ
	Status();
}

//---------------------------------------------------------------------------
//
//	REZERO UNIT
//
//---------------------------------------------------------------------------
void FASTCALL SASI::Rezero()
{
	BOOL status;

	ASSERT(this);
	ASSERT(sasi.current);

#if defined(SASI_LOG)
	LOG0(Log::Normal, "REZEROコマンド");
#endif	// SASI_LOG

	// ドライブでコマンド処理
	status = sasi.current->Rezero(sasi.cmd);
	if (!status) {
		// 失敗(エラー)
		Error();
		return;
	}

	// 成功
	sasi.status = 0x00;
	sasi.message = 0x00;

	// ステータスフェーズ
	Status();
}

//---------------------------------------------------------------------------
//
//	REQUEST SENSE
//
//---------------------------------------------------------------------------
void FASTCALL SASI::RequestSense()
{
	ASSERT(this);
	ASSERT(sasi.current);

#if defined(SASI_LOG)
	LOG0(Log::Normal, "REQUEST SENSEコマンド");
#endif	// SASI_LOG

	// ドライブでコマンド処理
	sasi.length = sasi.current->RequestSense(sasi.cmd, sasi.buffer);
	if (sasi.length <= 0) {
		// 失敗(エラー)
		Error();
		return;
	}

#if defined(SASI_LOG)
	LOG1(Log::Normal, "センスキー $%02X", sasi.buffer[2]);
#endif

	// ドライブから返却されたデータを返信。I/O=1, C/D=0
	sasi.offset = 0;
	sasi.blocks = 1;
	sasi.phase = read;
	sasi.io = TRUE;
	sasi.cd = FALSE;

	// エラーステータスをクリア
	sasi.status = 0x00;
	sasi.message = 0x00;

	// イベント設定、データの引き取りを要求
	event.SetUser(0);
	event.SetTime(48);
}

//---------------------------------------------------------------------------
//
//	FORMAT UNIT
//
//---------------------------------------------------------------------------
void FASTCALL SASI::Format()
{
	BOOL status;

	ASSERT(this);
	ASSERT(sasi.current);

#if defined(SASI_LOG)
	LOG0(Log::Normal, "FORMAT UNITコマンド");
#endif	// SASI_LOG

	// ドライブでコマンド処理
	status = sasi.current->Format(sasi.cmd);
	if (!status) {
		// 失敗(エラー)
		Error();
		return;
	}

	// 成功
	sasi.status = 0x00;
	sasi.message = 0x00;

	// ステータスフェーズ
	Status();
}

//---------------------------------------------------------------------------
//
//	REASSIGN BLOCKS
//	※SASIのみ(規格上はSCSIもあり)
//
//---------------------------------------------------------------------------
void FASTCALL SASI::Reassign()
{
	BOOL status;

	ASSERT(this);
	ASSERT(sasi.current);

#if defined(SASI_LOG)
	LOG0(Log::Normal, "REASSIGN BLOCKSコマンド");
#endif	// SASI_LOG

	// SASI以外はエラーとする(欠陥リストを送信しないため)
	if (!sasi.current->IsSASI()) {
		sasi.current->InvalidCmd();
		Error();
		return;
	}

	// ドライブでコマンド処理
	status = sasi.current->Reassign(sasi.cmd);
	if (!status) {
		// 失敗(エラー)
		Error();
		return;
	}

	// 成功
	sasi.status = 0x00;
	sasi.message = 0x00;

	// ステータスフェーズ
	Status();
}

//---------------------------------------------------------------------------
//
//	READ(6)
//
//---------------------------------------------------------------------------
void FASTCALL SASI::Read6()
{
	DWORD record;

	ASSERT(this);
	ASSERT(sasi.current);

	// レコード番号とブロック数を取得
	record = sasi.cmd[1] & 0x1f;
	record <<= 8;
	record |= sasi.cmd[2];
	record <<= 8;
	record |= sasi.cmd[3];
	sasi.blocks = sasi.cmd[4];
	if (sasi.blocks == 0) {
		sasi.blocks = 0x100;
	}

#if defined(SASI_LOG)
	LOG2(Log::Normal, "READ(6)コマンド レコード=%06X ブロック=%d", record, sasi.blocks);
#endif	// SASI_LOG

	// ドライブでコマンド処理
	sasi.length = sasi.current->Read(sasi.buffer, record);
	if (sasi.length <= 0) {
		// 失敗(エラー)
		Error();
		return;
	}

	// ステータスをOKに
	sasi.status = 0x00;
	sasi.message = 0x00;

	// 次のブロックok、ワーク設定
	sasi.offset = 0;
	sasi.next = record + 1;

	// 読み込みフェーズ, I/O=1, C/D=0
	sasi.phase = read;
	sasi.io = TRUE;
	sasi.cd = FALSE;

	// イベントを設定、データの引き取りを要求
	event.SetUser(1);
	event.SetTime(48);
}

//---------------------------------------------------------------------------
//
//	WRITE(6)
//
//---------------------------------------------------------------------------
void FASTCALL SASI::Write6()
{
	DWORD record;

	ASSERT(this);
	ASSERT(sasi.current);

	// レコード番号とブロック数を取得
	record = sasi.cmd[1] & 0x1f;
	record <<= 8;
	record |= sasi.cmd[2];
	record <<= 8;
	record |= sasi.cmd[3];
	sasi.blocks = sasi.cmd[4];
	if (sasi.blocks == 0) {
		sasi.blocks = 0x100;
	}

#if defined(SASI_LOG)
	LOG2(Log::Normal, "WRITE(6)コマンド レコード=%06X ブロック=%d", record, sasi.blocks);
#endif	// SASI_LOG

	// ドライブでコマンド処理
	sasi.length = sasi.current->WriteCheck(record);
	if (sasi.length <= 0) {
		// 失敗(エラー)
		Error();
		return;
	}

	// ステータスをOKに
	sasi.status = 0x00;
	sasi.message = 0x00;

	// 転送ワーク
	sasi.next = record + 1;
	sasi.offset = 0;

	// 書き込みフェーズ, C/D=0
	sasi.phase = write;
	sasi.cd = FALSE;

	// イベントを設定、データの書き込みを要求
	event.SetUser(1);
	event.SetTime(48);
}

//---------------------------------------------------------------------------
//
//	SEEK(6)
//
//---------------------------------------------------------------------------
void FASTCALL SASI::Seek6()
{
	BOOL status;

	ASSERT(this);
	ASSERT(sasi.current);

#if defined(SASI_LOG)
	LOG0(Log::Normal, "SEEK(6)コマンド");
#endif	// SASI_LOG

	// ドライブでコマンド処理
	status = sasi.current->Seek(sasi.cmd);
	if (!status) {
		// 失敗(エラー)
		Error();
		return;
	}

	// 成功
	sasi.status = 0x00;
	sasi.message = 0x00;

	// ステータスフェーズ
	Status();
}

//---------------------------------------------------------------------------
//
//	ASSIGN
//	※SASIのみ
//
//---------------------------------------------------------------------------
void FASTCALL SASI::Assign()
{
	ASSERT(this);
	ASSERT(sasi.current);

#if defined(SASI_LOG)
	LOG0(Log::Normal, "ASSIGNコマンド");
#endif	// SASI_LOG

	// SASI以外はエラー(ベンダユニークコマンド)
	if (!sasi.current->IsSASI()) {
		sasi.current->InvalidCmd();
		Error();
		return;
	}

	// ステータスをOKに
	sasi.status = 0x00;
	sasi.message = 0x00;

	// 4バイトのデータをリクエスト
	sasi.phase = write;
	sasi.length = 4;
	sasi.offset = 0;

	// C/Dは0(データ書き込み)
	sasi.cd = FALSE;

	// イベント設定、データの書き込みを要求
	event.SetUser(0);
	event.SetTime(48);
}

//---------------------------------------------------------------------------
//
//	INQUIRY
//	※SCSIのみ
//
//---------------------------------------------------------------------------
void FASTCALL SASI::Inquiry()
{
	ASSERT(this);
	ASSERT(sasi.current);

#if defined(SASI_LOG)
	LOG0(Log::Normal, "INQUIRYコマンド");
#endif	// SASI_LOG

	// ドライブでコマンド処理
	sasi.length = sasi.current->Inquiry(sasi.cmd, sasi.buffer);
	if (sasi.length <= 0) {
		// 失敗(エラー)
		Error();
		return;
	}

	// ステータスをOKに
	sasi.status = 0x00;
	sasi.message = 0x00;

	// ドライブから返却されたデータを返信。I/O=1, C/D=0
	sasi.offset = 0;
	sasi.blocks = 1;
	sasi.phase = read;
	sasi.io = TRUE;
	sasi.cd = FALSE;

	// イベント設定、データの引き取りを要求
	event.SetUser(0);
	event.SetTime(48);
}

//---------------------------------------------------------------------------
//
//	MODE SENSE
//	※SCSIのみ
//
//---------------------------------------------------------------------------
void FASTCALL SASI::ModeSense()
{
	int length;

	ASSERT(this);
	ASSERT(sasi.current);

#if defined(SASI_LOG)
	LOG0(Log::Normal, "MODE SENSEコマンド");
#endif	// SASI_LOG

	// ドライブでコマンド処理
	length = sasi.current->ModeSense(sasi.cmd, sasi.buffer);

	// 結果評価
	if (length > 0) {
		// ステータスをOKに
		sasi.status = 0x00;
		sasi.message = 0x00;
		sasi.length = length;
	}
	else {
#if 1
		Error();
		return;
#else
		// ステータスをエラーに
		sasi.status = (sasi.current->GetLUN() << 5) | 0x02;
		sasi.message = 0;
		sasi.length = -length;
#endif
	}

	// ドライブから返却されたデータを返信。I/O=1, C/D=0
	sasi.offset = 0;
	sasi.blocks = 1;
	sasi.phase = read;
	sasi.io = TRUE;
	sasi.cd = FALSE;

	// イベント設定、データの引き取りを要求
	event.SetUser(0);
	event.SetTime(48);
}

//---------------------------------------------------------------------------
//
//	START STOP UNIT
//	※SCSIのみ
//
//---------------------------------------------------------------------------
void FASTCALL SASI::StartStop()
{
	BOOL status;

	ASSERT(this);
	ASSERT(sasi.current);

#if defined(SASI_LOG)
	LOG0(Log::Normal, "START STOP UNITコマンド");
#endif	// SASI_LOG

	// ドライブでコマンド処理
	status = sasi.current->StartStop(sasi.cmd);
	if (!status) {
		// 失敗(エラー)
		Error();
		return;
	}

	// 成功
	sasi.status = 0x00;
	sasi.message = 0x00;

	// ステータスフェーズ
	Status();
}

//---------------------------------------------------------------------------
//
//	PREVENT/ALLOW MEDIUM REMOVAL
//	※SCSIのみ
//
//---------------------------------------------------------------------------
void FASTCALL SASI::Removal()
{
	BOOL status;

	ASSERT(this);
	ASSERT(sasi.current);

#if defined(SASI_LOG)
	LOG0(Log::Normal, "PREVENT/ALLOW MEDIUM REMOVALコマンド");
#endif	// SASI_LOG

	// ドライブでコマンド処理
	status = sasi.current->Removal(sasi.cmd);
	if (!status) {
		// 失敗(エラー)
		Error();
		return;
	}

	// 成功
	sasi.status = 0x00;
	sasi.message = 0x00;

	// ステータスフェーズ
	Status();
}

//---------------------------------------------------------------------------
//
//	READ CAPACITY
//	※SCSIのみ
//
//---------------------------------------------------------------------------
void FASTCALL SASI::ReadCapacity()
{
	int length;

	ASSERT(this);
	ASSERT(sasi.current);

#if defined(SASI_LOG)
	LOG0(Log::Normal, "READ CAPACITYコマンド");
#endif	// SASI_LOG

	// ドライブでコマンド処理
	length = sasi.current->ReadCapacity(sasi.cmd, sasi.buffer);

	// 結果評価
	if (length > 0) {
		// ステータスをOKに
		sasi.status = 0x00;
		sasi.message = 0x00;
		sasi.length = length;
	}
	else {
#if 1
		// 失敗(エラー)
		Error();
		return;
#else
		// ステータスをエラーに
		sasi.status = (sasi.current->GetLUN() << 5) | 0x02;
		sasi.message = 0x00;
		sasi.length = -length;
#endif
	}

	// ドライブから返却されたデータを返信。I/O=1, C/D=0
	sasi.offset = 0;
	sasi.blocks = 1;
	sasi.phase = read;
	sasi.io = TRUE;
	sasi.cd = FALSE;

	// イベント設定、データの引き取りを要求
	event.SetUser(0);
	event.SetTime(48);
}

//---------------------------------------------------------------------------
//
//	READ(10)
//	※SCSIのみ
//
//---------------------------------------------------------------------------
void FASTCALL SASI::Read10()
{
	DWORD record;

	ASSERT(this);
	ASSERT(sasi.current);

	// レコード番号とブロック数を取得
	record = sasi.cmd[2];
	record <<= 8;
	record |= sasi.cmd[3];
	record <<= 8;
	record |= sasi.cmd[4];
	record <<= 8;
	record |= sasi.cmd[5];
	sasi.blocks = sasi.cmd[7];
	sasi.blocks <<= 8;
	sasi.blocks |= sasi.cmd[8];

#if defined(SASI_LOG)
	LOG2(Log::Normal, "READ(10)コマンド レコード=%08X ブロック=%d", record, sasi.blocks);
#endif	// SASI_LOG

	// ブロック数0は処理しない
	if (sasi.blocks == 0) {
		sasi.status = 0x00;
		sasi.message = 0x00;
		Status();
		return;
	}

	// ドライブでコマンド処理
	sasi.length = sasi.current->Read(sasi.buffer, record);
	if (sasi.length <= 0) {
		// 失敗(エラー)
		Error();
		return;
	}

	// ステータスをOKに
	sasi.status = 0x00;
	sasi.message = 0x00;

	// 次のブロックok、ワーク設定
	sasi.offset = 0;
	sasi.next = record + 1;

	// 読み込みフェーズ, I/O=1, C/D=0
	sasi.phase = read;
	sasi.io = TRUE;
	sasi.cd = FALSE;

	// イベントを設定、データの引き取りを要求
	event.SetUser(1);
	event.SetTime(48);
}

//---------------------------------------------------------------------------
//
//	WRITE(10)
//	※SCSIのみ
//
//---------------------------------------------------------------------------
void FASTCALL SASI::Write10()
{
	DWORD record;

	ASSERT(this);
	ASSERT(sasi.current);

	// レコード番号とブロック数を取得
	record = sasi.cmd[2];
	record <<= 8;
	record |= sasi.cmd[3];
	record <<= 8;
	record |= sasi.cmd[4];
	record <<= 8;
	record |= sasi.cmd[5];
	sasi.blocks = sasi.cmd[7];
	sasi.blocks <<= 8;
	sasi.blocks |= sasi.cmd[8];

#if defined(SASI_LOG)
	LOG2(Log::Normal, "WRITE(10)コマンド レコード=%08X ブロック=%d", record, sasi.blocks);
#endif	// SASI_LOG

	// ブロック数0は処理しない
	if (sasi.blocks == 0) {
		sasi.status = 0x00;
		sasi.message = 0x00;
		Status();
		return;
	}

	// ドライブでコマンド処理
	sasi.length = sasi.current->WriteCheck(record);
	if (sasi.length <= 0) {
		// 失敗(エラー)
		Error();
		return;
	}

	// ステータスをOKに
	sasi.status = 0x00;
	sasi.message = 0x00;

	// 転送ワーク
	sasi.next = record + 1;
	sasi.offset = 0;

	// 書き込みフェーズ, C/D=0
	sasi.phase = write;
	sasi.cd = FALSE;

	// イベントを設定、データの書き込みを要求
	event.SetUser(1);
	event.SetTime(48);
}

//---------------------------------------------------------------------------
//
//	SEEK(10)
//	※SCSIのみ
//
//---------------------------------------------------------------------------
void FASTCALL SASI::Seek10()
{
	BOOL status;

	ASSERT(this);
	ASSERT(sasi.current);

#if defined(SASI_LOG)
	LOG0(Log::Normal, "SEEK(10)コマンド");
#endif	// SASI_LOG

	// ドライブでコマンド処理(論理ブロックは関係ないのでSeekを呼ぶ)
	status = sasi.current->Seek(sasi.cmd);
	if (!status) {
		// 失敗(エラー)
		Error();
		return;
	}

	// 成功
	sasi.status = 0x00;
	sasi.message = 0x00;

	// ステータスフェーズ
	Status();
}

//---------------------------------------------------------------------------
//
//	VERIFY
//	※SCSIのみ
//
//---------------------------------------------------------------------------
void FASTCALL SASI::Verify()
{
	BOOL status;

	ASSERT(this);
	ASSERT(sasi.current);

#if defined(SASI_LOG)
	LOG0(Log::Normal, "VERIFYコマンド");
#endif	// SASI_LOG

	// ドライブでコマンド処理
	status = sasi.current->Verify(sasi.cmd);
	if (!status) {
		// 失敗(エラー)
		Error();
		return;
	}

	// 成功
	sasi.status = 0x00;
	sasi.message = 0x00;

	// ステータスフェーズ
	Status();
}

//---------------------------------------------------------------------------
//
//	SPECIFY
//	※SASIのみ
//
//---------------------------------------------------------------------------
void FASTCALL SASI::Specify()
{
	ASSERT(this);
	ASSERT(sasi.current);

#if defined(SASI_LOG)
	LOG0(Log::Normal, "SPECIFYコマンド");
#endif	// SASI_LOG

	// SASI以外はエラー(ベンダユニークコマンド)
	if (!sasi.current->IsSASI()) {
		sasi.current->InvalidCmd();
		Error();
		return;
	}

	// ステータスOK
	sasi.status = 0x00;
	sasi.message = 0x00;

	// 10バイトのデータをリクエスト
	sasi.phase = write;
	sasi.length = 10;
	sasi.offset = 0;

	// C/Dは0(データ書き込み)
	sasi.cd = FALSE;

	// イベント設定、データの書き込みを要求
	event.SetUser(0);
	event.SetTime(48);
}

//---------------------------------------------------------------------------
//
//	MO オープン
//
//---------------------------------------------------------------------------
BOOL FASTCALL SASI::Open(const Filepath& path)
{
	ASSERT(this);

	// SCSIが有効であれば、SCSIにまかせる
	if (sasi.scsi_type != 0) {
		return scsi->Open(path);
	}

	// 有効でなければエラー
	if (!IsValid()) {
		return FALSE;
	}

	// イジェクト
	Eject(FALSE);

	// ノットレディでなければエラー
	if (IsReady()) {
		return FALSE;
	}

	// ドライブに任せる
	ASSERT(sasi.mo);
	if (sasi.mo->Open(path, TRUE)) {
		// 成功であれば、ファイルパスと書き込み禁止状態を記憶
		sasi.writep = sasi.mo->IsWriteP();
		sasi.mo->GetPath(scsimo);
		return TRUE;
	}

	// 失敗
	return FALSE;
}

//---------------------------------------------------------------------------
//
//	MO イジェクト
//
//---------------------------------------------------------------------------
void FASTCALL SASI::Eject(BOOL force)
{
	ASSERT(this);

	// SCSIが有効であれば、SCSIにまかせる
	if (sasi.scsi_type != 0) {
		scsi->Eject(force);
		return;
	}

	// レディでなければ何もしない
	if (!IsReady()) {
		return;
	}

	// ドライブに通知
	ASSERT(sasi.mo);
	sasi.mo->Eject(force);
}

//---------------------------------------------------------------------------
//
//	MO 書き込み禁止
//
//---------------------------------------------------------------------------
void FASTCALL SASI::WriteP(BOOL flag)
{
	ASSERT(this);

	// SCSIが有効であれば、SCSIにまかせる
	if (sasi.scsi_type != 0) {
		scsi->WriteP(flag);
		return;
	}

	// レディでなければ何もしない
	if (!IsReady()) {
		return;
	}

	// ライトプロテクト設定を試みる
	ASSERT(sasi.mo);
	sasi.mo->WriteP(flag);

	// 現状を保存
	sasi.writep = sasi.mo->IsWriteP();
}

//---------------------------------------------------------------------------
//
//	MO 書き込み禁止取得
//
//---------------------------------------------------------------------------
BOOL FASTCALL SASI::IsWriteP() const
{
	ASSERT(this);

	// SCSIが有効であれば、SCSIにまかせる
	if (sasi.scsi_type != 0) {
		return scsi->IsWriteP();
	}

	// レディでなければライトプロテクトでない
	if (!IsReady()) {
		return FALSE;
	}

	// ドライブに聞く
	ASSERT(sasi.mo);
	return sasi.mo->IsWriteP();
}

//---------------------------------------------------------------------------
//
//	MO Read Onlyチェック
//
//---------------------------------------------------------------------------
BOOL FASTCALL SASI::IsReadOnly() const
{
	ASSERT(this);

	// SCSIが有効であれば、SCSIにまかせる
	if (sasi.scsi_type != 0) {
		return scsi->IsReadOnly();
	}

	// レディでなければリードオンリーでない
	if (!IsReady()) {
		return FALSE;
	}

	// ドライブに聞く
	ASSERT(sasi.mo);
	return sasi.mo->IsReadOnly();
}

//---------------------------------------------------------------------------
//
//	MO Lockedチェック
//
//---------------------------------------------------------------------------
BOOL FASTCALL SASI::IsLocked() const
{
	ASSERT(this);

	// SCSIが有効であれば、SCSIにまかせる
	if (sasi.scsi_type != 0) {
		return scsi->IsLocked();
	}

	// ドライブが存在するか
	if (!sasi.mo) {
		return FALSE;
	}

	// ドライブに聞く
	return sasi.mo->IsLocked();
}

//---------------------------------------------------------------------------
//
//	MO Readyチェック
//
//---------------------------------------------------------------------------
BOOL FASTCALL SASI::IsReady() const
{
	ASSERT(this);

	// SCSIが有効であれば、SCSIにまかせる
	if (sasi.scsi_type != 0) {
		return scsi->IsReady();
	}

	// ドライブが存在するか
	if (!sasi.mo) {
		return FALSE;
	}

	// ドライブに聞く
	return sasi.mo->IsReady();
}

//---------------------------------------------------------------------------
//
//	MO 有効チェック
//
//---------------------------------------------------------------------------
BOOL FASTCALL SASI::IsValid() const
{
	ASSERT(this);

	// SCSIが有効であれば、SCSIにまかせる
	if (sasi.scsi_type != 0) {
		return scsi->IsValid();
	}

	// ポインタで判別
	if (sasi.mo) {
		return TRUE;
	}
	else {
		return FALSE;
	}
}

//---------------------------------------------------------------------------
//
//	MO パス取得
//
//---------------------------------------------------------------------------
void FASTCALL SASI::GetPath(Filepath& path) const
{
	ASSERT(this);

	// SCSIが有効であれば、SCSIにまかせる
	if (sasi.scsi_type != 0) {
		scsi->GetPath(path);
		return;
	}

	if (sasi.mo) {
		if (sasi.mo->IsReady()) {
			// MOディスクから
			sasi.mo->GetPath(path);
			return;
		}
	}

	// 有効なパスでないので、クリア
	path.Clear();
}
