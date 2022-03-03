//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 ＰＩ．(ytanaka@ipc-tokai.or.jp)
//	[ メモリ ]
//
//---------------------------------------------------------------------------

#include "os.h"
#include "xm6.h"
#include "vm.h"
#include "log.h"
#include "filepath.h"
#include "fileio.h"
#include "cpu.h"
#include "areaset.h"
#include "gvram.h"
#include "tvram.h"
#include "sram.h"
#include "config.h"
#include "core_asm.h"
#include "memory.h"

//---------------------------------------------------------------------------
//
//	スタティック ワーク
//
//---------------------------------------------------------------------------
static CPU *pCPU;

//---------------------------------------------------------------------------
//
//	バスエラー再現部(メインメモリ未実装エリアのみ)
//
//---------------------------------------------------------------------------
extern "C" {

//---------------------------------------------------------------------------
//
//	読み込みバスエラー
//
//---------------------------------------------------------------------------
void ReadBusErr(DWORD addr)
{
	pCPU->BusErr(addr, TRUE);
}

//---------------------------------------------------------------------------
//
//	書き込みバスエラー
//
//---------------------------------------------------------------------------
void WriteBusErr(DWORD addr)
{
	pCPU->BusErr(addr, FALSE);
}
}

//===========================================================================
//
//	メモリ
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	コンストラクタ
//
//---------------------------------------------------------------------------
Memory::Memory(VM *p) : MemDevice(p)
{
	// デバイスIDを初期化
	dev.id = MAKEID('M', 'E', 'M', ' ');
	dev.desc = "Memory Ctrl (OHM2)";

	// 開始アドレス、終了アドレス
	memdev.first = 0;
	memdev.last = 0xffffff;

	// RAM/ROMバッファ
	mem.ram = NULL;
	mem.ipl = NULL;
	mem.cg = NULL;
	mem.scsi = NULL;

	// RAMは2MB
	mem.size = 2;
	mem.config = 0;
	mem.length = 0;

	// メモリタイプは未ロード
	mem.type = None;
	mem.now = None;

	// オブジェクト
	areaset = NULL;
	sram = NULL;

	// その他
	memset(mem.table, 0, sizeof(mem.table));
	mem.memsw = TRUE;

	// staticワーク
	::pCPU = NULL;
}

//---------------------------------------------------------------------------
//
//	初期化
//
//---------------------------------------------------------------------------
BOOL FASTCALL Memory::Init()
{
	ASSERT(this);

	// 基本クラス
	if (!MemDevice::Init()) {
		return FALSE;
	}

	// メインメモリ
	mem.length = mem.size * 0x100000;
	try {
		mem.ram = new BYTE[ mem.length ];
	}
	catch (...) {
		return FALSE;
	}
	if (!mem.ram) {
		return FALSE;
	}

	// メインメモリをゼロクリアする
	memset(mem.ram, 0x00, mem.length);

	// IPL ROM
	try {
		mem.ipl = new BYTE[ 0x20000 ];
	}
	catch (...) {
		return FALSE;
	}
	if (!mem.ipl) {
		return FALSE;
	}

	// CG ROM
	try {
		mem.cg = new BYTE[ 0xc0000 ];
	}
	catch (...) {
		return FALSE;
	}
	if (!mem.cg) {
		return FALSE;
	}

	// SCSI ROM
	try {
		mem.scsi = new BYTE[ 0x20000 ];
	}
	catch (...) {
		return FALSE;
	}
	if (!mem.scsi) {
		return FALSE;
	}

	// SASIのROMは必須なので、先にロードする
	if (!LoadROM(SASI)) {
		// IPLROM.DAT, CGROM.DATが存在しないパターン
		return FALSE;
	}

	// 他のROMがあれば、XVI→Compact→030の順で、先に見つかったものを優先する
	if (LoadROM(XVI)) {
		mem.now = XVI;
	}
	if (mem.type == None) {
		if (LoadROM(Compact)) {
			mem.now = Compact;
		}
	}
	if (mem.type == None) {
		if (LoadROM(X68030)) {
			mem.now = X68030;
		}
	}

	// XVI,Compact,030いずれも存在しなければ、再度SASIを読む
	if (mem.type == None) {
		LoadROM(SASI);
		mem.now = SASI;
	}

	// リージョンエリアを設定
	::s68000context.u_fetch = u_pgr;
	::s68000context.s_fetch = s_pgr;
	::s68000context.u_readbyte = u_rbr;
	::s68000context.s_readbyte = s_rbr;
	::s68000context.u_readword = u_rwr;
	::s68000context.s_readword = s_rwr;
	::s68000context.u_writebyte = u_wbr;
	::s68000context.s_writebyte = s_wbr;
	::s68000context.u_writeword = u_wwr;
	::s68000context.s_writeword = s_wwr;

	// エリアセット取得
	areaset = (AreaSet*)vm->SearchDevice(MAKEID('A', 'R', 'E', 'A'));
	ASSERT(areaset);

	// SRAM取得
	sram = (SRAM*)vm->SearchDevice(MAKEID('S', 'R', 'A', 'M'));
	ASSERT(sram);

	// staticワーク
	::pCPU = cpu;

	// 初期化テーブル設定
	InitTable();

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	ROMロード
//
//---------------------------------------------------------------------------
BOOL FASTCALL Memory::LoadROM(memtype target)
{
	Filepath path;
	Fileio fio;
	int i;
	BYTE data;
	BYTE *ptr;
	BOOL scsi_req;
	int scsi_size;

	ASSERT(this);

	// 一旦すべてのROMエリアを消去し、Noneに
	memset(mem.ipl, 0xff, 0x20000);
	memset(mem.cg, 0xff, 0xc0000);
	memset(mem.scsi, 0xff, 0x20000);
	mem.type = None;

	// IPL
	switch (target) {
		case SASI:
		case SCSIInt:
		case SCSIExt:
			path.SysFile(Filepath::IPL);
			break;
		case XVI:
			path.SysFile(Filepath::IPLXVI);
			break;
		case Compact:
			path.SysFile(Filepath::IPLCompact);
			break;
		case X68030:
			path.SysFile(Filepath::IPL030);
			break;
		default:
			ASSERT(FALSE);
			return FALSE;
	}
	if (!fio.Load(path, mem.ipl, 0x20000)) {
		return FALSE;
	}

	// IPLバイトスワップ
	ptr = mem.ipl;
	for (i=0; i<0x10000; i++) {
		data = ptr[0];
		ptr[0] = ptr[1];
		ptr[1] = data;
		ptr += 2;
	}

	// CG
	path.SysFile(Filepath::CG);
	if (!fio.Load(path, mem.cg, 0xc0000)) {
		// ファイルがなければ、CGTMPでリトライ
		path.SysFile(Filepath::CGTMP);
		if (!fio.Load(path, mem.cg, 0xc0000)) {
			return FALSE;
		}
	}

	// CGバイトスワップ
	ptr = mem.cg;
	for (i=0; i<0x60000; i++) {
		data = ptr[0];
		ptr[0] = ptr[1];
		ptr[1] = data;
		ptr += 2;
	}

	// SCSI
	scsi_req = FALSE;
	switch (target) {
		// 内蔵
		case SCSIInt:
		case XVI:
		case Compact:
			path.SysFile(Filepath::SCSIInt);
			scsi_req = TRUE;
			break;
		case X68030:
			path.SysFile(Filepath::ROM030);
			scsi_req = TRUE;
			break;
		// 外付
		case SCSIExt:
			path.SysFile(Filepath::SCSIExt);
			scsi_req = TRUE;
			break;
		// SASI(ROM必要なし)
		case SASI:
			break;
		// その他(あり得ない)
		default:
			ASSERT(FALSE);
			break;
	}
	if (scsi_req) {
		// X68030のみROM30.DAT(0x20000バイト)、その他は0x2000バイトでトライ
		if (target == X68030) {
			scsi_size = 0x20000;
		}
		else {
			scsi_size = 0x2000;
		}

		// 先にポインタを設定
		ptr = mem.scsi;

		// ロード
		if (!fio.Load(path, mem.scsi, scsi_size)) {
			// SCSIExtは0x1fe0バイトも許す(WinX68k高速版と互換をとる)
			if (target != SCSIExt) {
				return FALSE;
			}

			// 0x1fe0バイトで再トライ
			scsi_size = 0x1fe0;
			ptr = &mem.scsi[0x20];
			if (!fio.Load(path, &mem.scsi[0x0020], scsi_size)) {
				return FALSE;
			}
		}

		// SCSIバイトスワップ
		for (i=0; i<scsi_size; i+=2) {
			data = ptr[0];
			ptr[0] = ptr[1];
			ptr[1] = data;
			ptr += 2;
		}
	}

	// ターゲットをカレントにセットして、成功
	mem.type = target;
	return TRUE;
}

//---------------------------------------------------------------------------
//
//	初期化テーブル作成
//	※メモリデコーダに依存
//
//---------------------------------------------------------------------------
void FASTCALL Memory::InitTable()
{
#if defined(_WIN32)
#pragma pack(push, 1)
#endif	// _WIN32
	MemDevice* devarray[0x40];
#if defined(_WIN32)
#pragma pack(pop)
#endif	// _WIN32

	MemDevice *mdev;
	BYTE *table;
	DWORD ptr;
	int i;

	ASSERT(this);

	// ポインタ初期化
	mdev = this;
	i = 0;

	// Memory以降のデバイスを回って、ポインタを配列に落とす
	while (mdev) {
		devarray[i] = mdev;

		// 次へ
		i++;
		mdev = (MemDevice*)mdev->GetNextDevice();
	}

	// アセンブラルーチンを呼び出し、テーブルを引き渡す
	MemInitDecode(this, devarray);

	// アセンブラルーチンで出来たテーブルを逆に戻す(アラインメントに注意)
	table = MemDecodeTable;
	for (i=0; i<0x180; i++) {
		// 4バイトごとにDWORD値を取り込み、ポインタにキャスト
		ptr = *(DWORD*)table;
		mem.table[i] = (MemDevice*)ptr;

		// 次へ
		table += 4;
	}
}

//---------------------------------------------------------------------------
//
//	クリーンアップ
//
//---------------------------------------------------------------------------
void FASTCALL Memory::Cleanup()
{
	ASSERT(this);

	// メモリ解放
	if (mem.ram) {
		delete[] mem.ram;
		mem.ram = NULL;
	}
	if (mem.ipl) {
		delete[] mem.ipl;
		mem.ipl = NULL;
	}
	if (mem.cg) {
		delete[] mem.cg;
		mem.cg = NULL;
	}
	if (mem.scsi) {
		delete[] mem.scsi;
		mem.scsi = NULL;
	}

	// 基本クラスへ
	MemDevice::Cleanup();
}

//---------------------------------------------------------------------------
//
//	リセット
//
//---------------------------------------------------------------------------
void FASTCALL Memory::Reset()
{
	int size;

	ASSERT(this);
	LOG0(Log::Normal, "リセット");

	// メモリタイプが一致しているか
	if (mem.type != mem.now) {
		if (LoadROM(mem.type)) {
			// ROMが存在している。ロードできた
			mem.now = mem.type;
		}
		else {
			// ROMが存在しない。SASIタイプとして、設定もSASIに戻す
			LoadROM(SASI);
			mem.now = SASI;
			mem.type = SASI;
		}

		// コンテキストを作り直す(CPU::Resetは完了しているため、必ずFALSE)
		MakeContext(FALSE);
	}

	// メモリサイズが一致しているか
	if (mem.size == ((mem.config + 1) * 2)) {
		// 一致しているので、メモリスイッチ自動更新チェック
		if (mem.memsw) {
			// $ED0008 : メインRAMサイズ
			size = mem.size << 4;
			sram->SetMemSw(0x08, 0x00);
			sram->SetMemSw(0x09, size);
			sram->SetMemSw(0x0a, 0x00);
			sram->SetMemSw(0x0b, 0x00);
		}
		return;
	}

	// 変更
	mem.size = (mem.config + 1) * 2;

	// 再確保
	ASSERT(mem.ram);
	delete[] mem.ram;
	mem.ram = NULL;
	mem.length = mem.size * 0x100000;
	try {
		mem.ram = new BYTE[ mem.length ];
	}
	catch (...) {
		// メモリ不足の場合は2MBに固定
		mem.config = 0;
		mem.size = 2;
		mem.length = mem.size * 0x100000;
		mem.ram = new BYTE[ mem.length ];
	}
	if (!mem.ram) {
		// メモリ不足の場合は2MBに固定
		mem.config = 0;
		mem.size = 2;
		mem.length = mem.size * 0x100000;
		mem.ram = new BYTE[ mem.length ];
	}

	// メモリが確保できている場合のみ
	if (mem.ram) {
		memset(mem.ram, 0x00, mem.length);

		// コンテキストを作り直す(CPU::Resetは完了しているため、必ずFALSE)
		MakeContext(FALSE);
	}

	// メモリスイッチ自動更新
	if (mem.memsw) {
		// $ED0008 : メインRAMサイズ
		size = mem.size << 4;
		sram->SetMemSw(0x08, 0x00);
		sram->SetMemSw(0x09, size);
		sram->SetMemSw(0x0a, 0x00);
		sram->SetMemSw(0x0b, 0x00);
	}
}

//---------------------------------------------------------------------------
//
//	セーブ
//
//---------------------------------------------------------------------------
BOOL FASTCALL Memory::Save(Fileio *fio, int /*ver*/)
{
	ASSERT(this);
	LOG0(Log::Normal, "セーブ");

	// タイプを書く
	if (!fio->Write(&mem.now, sizeof(mem.now))) {
		return FALSE;
	}

	// SCSI ROMの内容を書く (X68030以外)
	if (mem.now != X68030) {
		if (!fio->Write(mem.scsi, 0x2000)) {
			return FALSE;
		}
	}

	// mem.sizeを書く
	if (!fio->Write(&mem.size, sizeof(mem.size))) {
		return FALSE;
	}

	// メモリを書く
	if (!fio->Write(mem.ram, mem.length)) {
		return FALSE;
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	ロード
//
//---------------------------------------------------------------------------
BOOL FASTCALL Memory::Load(Fileio *fio, int /*ver*/)
{
	int size;
	BOOL context;

	ASSERT(this);
	LOG0(Log::Normal, "ロード");

	// コンテキストを作り直さない
	context = FALSE;

	// タイプを読む
	if (!fio->Read(&mem.type, sizeof(mem.type))) {
		return FALSE;
	}

	// タイプが現在のものと違っていれば
	if (mem.type != mem.now) {
		// ROMを読み直す
		if (!LoadROM(mem.type)) {
			// セーブ時に存在していたROMが、なくなっている
			LoadROM(mem.now);
			return FALSE;
		}

		// ROMの読み直しに成功した
		mem.now = mem.type;
		context = TRUE;
	}

	// SCSI ROMの内容を読む (X68030以外)
	if (mem.type != X68030) {
		if (!fio->Read(mem.scsi, 0x2000)) {
			return FALSE;
		}
	}

	// mem.sizeを読む
	if (!fio->Read(&size, sizeof(size))) {
		return FALSE;
	}

	// mem.sizeと一致していなければ
	if (mem.size != size) {
		// 変更して
		mem.size = size;

		// 再確保
		delete[] mem.ram;
		mem.ram = NULL;
		mem.length = mem.size * 0x100000;
		try {
			mem.ram = new BYTE[ mem.length ];
		}
		catch (...) {
			mem.ram = NULL;
		}
		if (!mem.ram) {
			// メモリ不足の場合は2MBに固定
			mem.config = 0;
			mem.size = 2;
			mem.length = mem.size * 0x100000;
			mem.ram = new BYTE[ mem.length ];

			// ロード失敗
			return FALSE;
		}

		// コンテキスト再作成が必要
		context = TRUE;
	}

	// メモリを読む
	if (!fio->Read(mem.ram, mem.length)) {
		return FALSE;
	}

	// 必要であれば、コンテキストを作り直す
	if (context) {
		MakeContext(FALSE);
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	設定適用
//
//---------------------------------------------------------------------------
void FASTCALL Memory::ApplyCfg(const Config *config)
{
	ASSERT(this);
	ASSERT(config);
	LOG0(Log::Normal, "設定適用");

	// メモリ種別(ROMロードは次回リセット時)
	mem.type = (memtype)config->mem_type;

	// RAMサイズ(メモリ確保は次回リセット時)
	mem.config = config->ram_size;
	ASSERT((mem.config >= 0) && (mem.config <= 5));

	// メモリスイッチ自動更新
	mem.memsw = config->ram_sramsync;
}

//---------------------------------------------------------------------------
//
//	バイト読み込み
//
//---------------------------------------------------------------------------
DWORD FASTCALL Memory::ReadByte(DWORD addr)
{
	DWORD index;

	ASSERT(this);
	ASSERT(addr <= 0xffffff);
	ASSERT(mem.now != None);

	// メインRAM
	if (addr < mem.length) {
		return (DWORD)mem.ram[addr ^ 1];
	}

	// IPL
	if (addr >= 0xfe0000) {
		addr &= 0x1ffff;
		addr ^= 1;
		return (DWORD)mem.ipl[addr];
	}

	// IPLイメージ or SCSI内蔵
	if (addr >= 0xfc0000) {
		// IPLイメージか
		if ((mem.now == SASI) || (mem.now == SCSIExt)) {
			// IPLイメージ
			addr &= 0x1ffff;
			addr ^= 1;
			return (DWORD)mem.ipl[addr];
		}
		// SCSI内蔵か(範囲チェック)
		if (addr < 0xfc2000) {
			// SCSI内蔵
			addr &= 0x1fff;
			addr ^= 1;
			return (DWORD)mem.scsi[addr];
		}
		// X68030 IPL前半か
		if (mem.now == X68030) {
			// X68030 IPL前半
			addr &= 0x1ffff;
			addr ^= 1;
			return (DWORD)mem.scsi[addr];
		}
		// SCSI内蔵モデルで、ROM範囲外
		return 0xff;
	}

	// CG
	if (addr >= 0xf00000) {
		addr &= 0xfffff;
		addr ^= 1;
		return (DWORD)mem.cg[addr];
	}

	// SCSI外付
	if (mem.now == SCSIExt) {
		if ((addr >= 0xea0020) && (addr <= 0xea1fff)) {
			addr &= 0x1fff;
			addr ^= 1;
			return (DWORD)mem.scsi[addr];
		}
	}

	// デバイスディスパッチ
	if (addr >= 0xc00000) {
		index = addr - 0xc00000;
		index >>= 13;
		ASSERT(index < 0x180);
		if (mem.table[index] != (MemDevice*)this) {
			return mem.table[index]->ReadByte(addr);
		}
	}

	LOG1(Log::Warning, "未定義バイト読み込み $%06X", addr);
	cpu->BusErr(addr, TRUE);
	return 0xff;
}

//---------------------------------------------------------------------------
//
//	ワード読み込み
//
//---------------------------------------------------------------------------
DWORD FASTCALL Memory::ReadWord(DWORD addr)
{
	DWORD data;
	DWORD index;
	WORD *ptr;

	ASSERT(this);
	ASSERT(addr <= 0xffffff);
	ASSERT(mem.now != None);

	// CPUからの場合は偶数保証されているが、DMACからの場合はチェック必要あり
	if (addr & 1) {
		// 一旦CPUへ渡す(CPU経由でDMAへ)
		cpu->AddrErr(addr, TRUE);
		return 0xffff;
	}

	// メインRAM
	if (addr < mem.length) {
		ptr = (WORD*)(&mem.ram[addr]);
		data = (DWORD)*ptr;
		return data;
	}

	// IPL
	if (addr >= 0xfe0000) {
		addr &= 0x1ffff;
		ptr = (WORD*)(&mem.ipl[addr]);
		data = (DWORD)*ptr;
		return data;
	}

	// IPLイメージ or SCSI内蔵
	if (addr >= 0xfc0000) {
		// IPLイメージか
		if ((mem.now == SASI) || (mem.now == SCSIExt)) {
			// IPLイメージ
			addr &= 0x1ffff;
			ptr = (WORD*)(&mem.ipl[addr]);
			data = (DWORD)*ptr;
			return data;
		}
		// SCSI内蔵か(範囲チェック)
		if (addr < 0xfc2000) {
			// SCSI内蔵
			addr &= 0x1fff;
			ptr = (WORD*)(&mem.scsi[addr]);
			data = (DWORD)*ptr;
			return data;
		}
		// X68030 IPL前半か
		if (mem.now == X68030) {
			// X68030 IPL前半
			addr &= 0x1ffff;
			ptr = (WORD*)(&mem.scsi[addr]);
			data = (DWORD)*ptr;
			return data;
		}
		// SCSI内蔵モデルで、ROM範囲外
		return 0xffff;
	}

	// CG
	if (addr >= 0xf00000) {
		addr &= 0xfffff;
		ptr = (WORD*)(&mem.cg[addr]);
		data = (DWORD)*ptr;
		return data;
	}

	// SCSI外付
	if (mem.now == SCSIExt) {
		if ((addr >= 0xea0020) && (addr <= 0xea1fff)) {
			addr &= 0x1fff;
			ptr = (WORD*)(&mem.scsi[addr]);
			data = (DWORD)*ptr;
			return data;
		}
	}

	// デバイスディスパッチ
	if (addr >= 0xc00000) {
		index = addr - 0xc00000;
		index >>= 13;
		ASSERT(index < 0x180);
		if (mem.table[index] != (MemDevice*)this) {
			return mem.table[index]->ReadWord(addr);
		}
	}

	// バスエラー
	LOG1(Log::Warning, "未定義ワード読み込み $%06X", addr);
	cpu->BusErr(addr, TRUE);
	return 0xffff;
}

//---------------------------------------------------------------------------
//
//	バイト書き込み
//
//---------------------------------------------------------------------------
void FASTCALL Memory::WriteByte(DWORD addr, DWORD data)
{
	DWORD index;

	ASSERT(this);
	ASSERT(addr <= 0xffffff);
	ASSERT(data < 0x100);
	ASSERT(mem.now != None);

	// メインRAM
	if (addr < mem.length) {
		mem.ram[addr ^ 1] = (BYTE)data;
		return;
	}

	// IPL,SCSI,CG
	if (addr >= 0xf00000) {
		return;
	}

	// デバイスディスパッチ
	if (addr >= 0xc00000) {
		index = addr - 0xc00000;
		index >>= 13;
		ASSERT(index < 0x180);
		if (mem.table[index] != (MemDevice*)this) {
			mem.table[index]->WriteByte(addr, data);
			return;
		}
	}

	// バスエラー
	cpu->BusErr(addr, FALSE);
	LOG2(Log::Warning, "未定義バイト書き込み $%06X <- $%02X", addr, data);
}

//---------------------------------------------------------------------------
//
//	ワード書き込み
//
//---------------------------------------------------------------------------
void FASTCALL Memory::WriteWord(DWORD addr, DWORD data)
{
	WORD *ptr;
	DWORD index;

	ASSERT(this);
	ASSERT(addr <= 0xffffff);
	ASSERT(data < 0x10000);
	ASSERT(mem.now != None);

	// CPUからの場合は偶数保証されているが、DMACからの場合はチェック必要あり
	if (addr & 1) {
		// 一旦CPUへ渡す(CPU経由でDMAへ)
		cpu->AddrErr(addr, FALSE);
		return;
	}

	// メインRAM
	if (addr < mem.length) {
		ptr = (WORD*)(&mem.ram[addr]);
		*ptr = (WORD)data;
		return;
	}

	// IPL,SCSI,CG
	if (addr >= 0xf00000) {
		return;
	}

	// デバイスディスパッチ
	if (addr >= 0xc00000) {
		index = addr - 0xc00000;
		index >>= 13;
		ASSERT(index < 0x180);
		if (mem.table[index] != (MemDevice*)this) {
			mem.table[index]->WriteWord(addr, data);
			return;
		}
	}

	// バスエラー
	cpu->BusErr(addr, FALSE);
	LOG2(Log::Warning, "未定義ワード書き込み $%06X <- $%04X", addr, data);
}

//---------------------------------------------------------------------------
//
//	読み込みのみ
//
//---------------------------------------------------------------------------
DWORD FASTCALL Memory::ReadOnly(DWORD addr) const
{
	DWORD index;

	ASSERT(this);
	ASSERT(addr <= 0xffffff);
	ASSERT(mem.now != None);

	// メインRAM
	if (addr < mem.length) {
		return (DWORD)mem.ram[addr ^ 1];
	}

	// IPL
	if (addr >= 0xfe0000) {
		addr &= 0x1ffff;
		addr ^= 1;
		return (DWORD)mem.ipl[addr];
	}

	// IPLイメージ or SCSI内蔵
	if (addr >= 0xfc0000) {
		// IPLイメージか
		if ((mem.now == SASI) || (mem.now == SCSIExt)) {
			// IPLイメージ
			addr &= 0x1ffff;
			addr ^= 1;
			return (DWORD)mem.ipl[addr];
		}
		// SCSI内蔵か(範囲チェック)
		if (addr < 0xfc2000) {
			// SCSI内蔵
			addr &= 0x1fff;
			addr ^= 1;
			return (DWORD)mem.scsi[addr];
		}
		// X68030 IPL前半か
		if (mem.now == X68030) {
			// X68030 IPL前半
			addr &= 0x1ffff;
			addr ^= 1;
			return (DWORD)mem.scsi[addr];
		}
		// SCSI内蔵モデルで、ROM範囲外
		return 0xff;
	}

	// CG
	if (addr >= 0xf00000) {
		addr &= 0xfffff;
		addr ^= 1;
		return (DWORD)mem.cg[addr];
	}

	// SCSI外付
	if (mem.now == SCSIExt) {
		if ((addr >= 0xea0020) && (addr <= 0xea1fff)) {
			addr &= 0x1fff;
			addr ^= 1;
			return (DWORD)mem.scsi[addr];
		}
	}

	// デバイスディスパッチ
	if (addr >= 0xc00000) {
		index = addr - 0xc00000;
		index >>= 13;
		ASSERT(index < 0x180);
		if (mem.table[index] != (MemDevice*)this) {
			return mem.table[index]->ReadOnly(addr);
		}
	}

	// マップされていない
	return 0xff;
}

//---------------------------------------------------------------------------
//
//	コンテキスト作成
//
//---------------------------------------------------------------------------
void FASTCALL Memory::MakeContext(BOOL reset)
{
	int index;
	int area;
	GVRAM *gvram;
	TVRAM *tvram;

	ASSERT(this);

	// リセットか
	if (reset) {
		// エリアセットをリセット(CPU::ResetからMakeContextが呼ばれるため)
		ASSERT(areaset);
		areaset->Reset();

		// リセット専用コンテキスト($FF00000～が、$0000000～に見える)
		s_pgr[0].lowaddr = 0;
		s_pgr[0].highaddr = 0xffff;
		s_pgr[0].offset = ((DWORD)mem.ipl) + 0x10000;
		u_pgr[0].lowaddr = 0;
		u_pgr[0].highaddr = 0xffff;
		u_pgr[0].offset = ((DWORD)mem.ipl) + 0x10000;

		// プログラム終了
		TerminateProgramRegion(1, s_pgr);
		TerminateProgramRegion(1, u_pgr);

		// データは全て無し
		TerminateDataRegion(0, u_rbr);
		TerminateDataRegion(0, s_rbr);
		TerminateDataRegion(0, u_rwr);
		TerminateDataRegion(0, s_rwr);
		TerminateDataRegion(0, u_wbr);
		TerminateDataRegion(0, s_wbr);
		TerminateDataRegion(0, u_wwr);
		TerminateDataRegion(0, s_wwr);
		return;
	}

	// 通常コンテキスト - プログラム(User)
	index = 0;
	area = areaset->GetArea();
	u_pgr[index].lowaddr = (area + 1) << 13;
	u_pgr[index].highaddr = mem.length - 1;
	u_pgr[index].offset = (DWORD)mem.ram;
	index++;
	TerminateProgramRegion(index, u_pgr);

	// 通常コンテキスト - プログラム(Super)
	index = 0;
	s_pgr[index].lowaddr = 0;
	s_pgr[index].highaddr = mem.length - 1;
	s_pgr[index].offset = (DWORD)mem.ram;
	index++;

	// IPL
	s_pgr[index].lowaddr = 0xfe0000;
	s_pgr[index].highaddr = 0xffffff;
	s_pgr[index].offset = ((DWORD)mem.ipl) - 0xfe0000;
	index++;

	// SCSI外付
	if (mem.now == SCSIExt) {
		s_pgr[index].lowaddr = 0xea0000;
		s_pgr[index].highaddr = 0xea1fff;
		s_pgr[index].offset = ((DWORD)mem.scsi) - 0xea0000;
		index++;
	}

	// IPLイメージ or SCSI内蔵
	if ((mem.now == SASI) || (mem.now == SCSIExt)) {
		// IPLイメージ
		s_pgr[index].lowaddr = 0xfc0000;
		s_pgr[index].highaddr = 0xfdffff;
		s_pgr[index].offset = ((DWORD)mem.ipl) - 0xfc0000;
		index++;
	}
	else {
		// SCSI内蔵
		s_pgr[index].lowaddr = 0xfc0000;
		s_pgr[index].highaddr = 0xfc1fff;
		s_pgr[index].offset = ((DWORD)mem.scsi) - 0xfc0000;
		if (mem.now == X68030) {
			// X68030 IPL前半
			s_pgr[index].lowaddr = 0xfc0000;
			s_pgr[index].highaddr = 0xfdffff;
			s_pgr[index].offset = ((DWORD)mem.scsi) - 0xfc0000;
		}
		index++;
	}

	// グラフィックVRAM
	gvram = (GVRAM*)vm->SearchDevice(MAKEID('G', 'V', 'R', 'M'));
	ASSERT(gvram);
	s_pgr[index].lowaddr = 0xc00000;
	s_pgr[index].highaddr = 0xdfffff;
	s_pgr[index].offset = ((DWORD)gvram->GetGVRAM()) - 0xc00000;
	index++;

	// テキストVRAM
	tvram = (TVRAM*)vm->SearchDevice(MAKEID('T', 'V', 'R', 'M'));
	ASSERT(tvram);
	s_pgr[index].lowaddr = 0xe00000;
	s_pgr[index].highaddr = 0xe7ffff;
	s_pgr[index].offset = ((DWORD)tvram->GetTVRAM()) - 0xe00000;
	index++;

	// SRAM
	ASSERT(sram);
	s_pgr[index].lowaddr = 0xed0000;
	s_pgr[index].highaddr = 0xed0000 + (sram->GetSize() << 10) - 1;
	s_pgr[index].offset = ((DWORD)sram->GetSRAM()) - 0xed0000;
	index++;
	TerminateProgramRegion(index, s_pgr);

	// 通常コンテキスト - 読み出し(User)
	index = 0;
	area = areaset->GetArea();

	// ユーザアクセス可能空間
	u_rbr[index].lowaddr = (area + 1) << 13;
	u_rbr[index].highaddr = mem.length - 1;
	u_rbr[index].memorycall = NULL;
	u_rbr[index].userdata = (void*)&mem.ram[(area + 1) << 13];
	index++;

	// スーパバイザ空間
	u_rbr[index].lowaddr = 0;
	u_rbr[index].highaddr = ((area + 1) << 13) - 1;
	u_rbr[index].memorycall = ::ReadErrC;
	u_rbr[index].userdata = NULL;
	index++;

	// メインメモリ未実装空間＋スーパーバイザI/O空間
	u_rbr[index].lowaddr = (mem.size << 20);
	u_rbr[index].highaddr = 0xebffff;
	u_rbr[index].memorycall = ::ReadErrC;
	s_rbr[index].userdata = NULL;
	index++;

	// ユーザI/O空間($EC0000-$ECFFFF)
	u_rbr[index].lowaddr = 0xec0000;
	u_rbr[index].highaddr = 0xecffff;
	u_rbr[index].memorycall = ::ReadByteC;
	s_rbr[index].userdata = NULL;
	index++;

	// スーパバイザ空間(SRAM,CG,IPL,SCSI)
	u_rbr[index].lowaddr = 0xed0000;
	u_rbr[index].highaddr = 0xffffff;
	u_rbr[index].memorycall = ReadErrC;
	s_rbr[index].userdata = NULL;
	index++;
	TerminateDataRegion(index, u_rbr);

	// 他へ移す(ReadWord, User)
	memcpy(u_rwr, u_rbr, sizeof(u_rbr));
	u_rwr[index - 2].memorycall = ::ReadWordC;

	// 他へ移す(WriteByte, User)
	memcpy(u_wbr, u_rbr, sizeof(u_rbr));
	u_wbr[index - 2].memorycall = ::WriteByteC;
	u_wbr[index - 1].memorycall = ::WriteErrC;
	u_wbr[index - 3].memorycall = ::WriteErrC;
	u_wbr[index - 4].memorycall = ::WriteErrC;

	// 他へ移す(WriteWord, User)
	memcpy(u_wwr, u_wbr, sizeof(u_wbr));
	u_wwr[index - 2].memorycall = ::WriteWordC;

	// 通常コンテキスト - 読み出し(Super)
	index = 0;
	s_rbr[index].lowaddr = 0;
	s_rbr[index].highaddr = mem.length - 1;
	s_rbr[index].memorycall = NULL;
	s_rbr[index].userdata = (void*)mem.ram;
	index++;

	// CG
	s_rbr[index].lowaddr = 0xf00000;
	s_rbr[index].highaddr = 0xfbffff;
	s_rbr[index].memorycall = NULL;
	s_rbr[index].userdata = (void*)mem.cg;
	index++;

	// IPL
	s_rbr[index].lowaddr = 0xfe0000;
	s_rbr[index].highaddr = 0xffffff;
	s_rbr[index].memorycall = NULL;
	s_rbr[index].userdata = (void*)mem.ipl;
	index++;

	// SCSI外付
	if (mem.now == SCSIExt) {
		s_rbr[index].lowaddr = 0xea0020;
		s_rbr[index].highaddr = 0xea1fff;
		s_rbr[index].memorycall = NULL;
		s_rbr[index].userdata = (void*)(&mem.scsi[0x20]);
		index++;
	}

	// IPLイメージ or SCSI内蔵
	if ((mem.now == SASI) || (mem.now == SCSIExt)) {
		// IPLイメージ
		s_rbr[index].lowaddr = 0xfc0000;
		s_rbr[index].highaddr = 0xfdffff;
		s_rbr[index].memorycall = NULL;
		s_rbr[index].userdata = (void*)mem.ipl;
		index++;
	}
	else {
		// SCSI内蔵
		s_rbr[index].lowaddr = 0xfc0000;
		s_rbr[index].highaddr = 0xfc1fff;
		s_rbr[index].memorycall = NULL;
		s_rbr[index].userdata = (void*)mem.scsi;
		if (mem.now == X68030) {
			// X68030 IPL前半
			s_rbr[index].lowaddr = 0xfc0000;
			s_rbr[index].highaddr = 0xfdffff;
			s_rbr[index].memorycall = NULL;
			s_rbr[index].userdata = (void*)mem.scsi;
		}
		index++;
	}

	// それ以外(外部コール)
	s_rbr[index].lowaddr = (mem.size << 20);
	s_rbr[index].highaddr = 0xefffff;
	s_rbr[index].memorycall = ::ReadByteC;
	s_rbr[index].userdata = NULL;
	index++;
	TerminateDataRegion(index, s_rbr);

	// 他へ移す
	memcpy(s_rwr, s_rbr, sizeof(s_rbr));
	s_rwr[index - 1].memorycall = ::ReadWordC;

	// 通常コンテキスト - 書き込み(Super)
	index = 0;
	s_wbr[index].lowaddr = 0;
	s_wbr[index].highaddr = mem.length - 1;
	s_wbr[index].memorycall = NULL;
	s_wbr[index].userdata = (void*)mem.ram;
	index++;

	// それ以外(外部コール)
	s_wbr[index].lowaddr = (mem.size << 20);
	s_wbr[index].highaddr = 0xefffff;
	s_wbr[index].memorycall = ::WriteByteC;
	s_wbr[index].userdata = NULL;
	index++;
	TerminateDataRegion(index, s_wbr);

	// 他へ移す
	memcpy(s_wwr, s_wbr, sizeof(s_wbr));
	s_wwr[index - 1].memorycall = ::WriteWordC;

	// cpu->Releaseを忘れずに
	cpu->Release();
}

//---------------------------------------------------------------------------
//
//	プログラムリージョン終了
//
//---------------------------------------------------------------------------
void FASTCALL Memory::TerminateProgramRegion(int index, STARSCREAM_PROGRAMREGION *spr)
{
	ASSERT(this);
	ASSERT((index >= 0) && (index < 10));
	ASSERT(spr);

	spr[index].lowaddr = (DWORD)-1;
	spr[index].highaddr = (DWORD)-1;
	spr[index].offset = 0;
}

//---------------------------------------------------------------------------
//
//	データリージョン終了
//
//---------------------------------------------------------------------------
void FASTCALL Memory::TerminateDataRegion(int index, STARSCREAM_DATAREGION *sdr)
{
	ASSERT(this);
	ASSERT((index >= 0) && (index < 10));
	ASSERT(sdr);

	sdr[index].lowaddr = (DWORD)-1;
	sdr[index].highaddr = (DWORD)-1;
	sdr[index].memorycall = NULL;
	sdr[index].userdata = NULL;
}

//---------------------------------------------------------------------------
//
//	IPLバージョンチェック
//	※IPLがversion1.00(87/05/07)であるか否かをチェック
//
//---------------------------------------------------------------------------
BOOL FASTCALL Memory::CheckIPL() const
{
	ASSERT(this);
	ASSERT(mem.now != None);

	// 存在チェック
	if (!mem.ipl) {
		return FALSE;
	}

	// SASIタイプの場合のみチェックする
	if (mem.now != SASI) {
		return TRUE;
	}

	// 日付(BCD)をチェック
	if (mem.ipl[0x1000a] != 0x87) {
		return FALSE;
	}
	if (mem.ipl[0x1000c] != 0x07) {
		return FALSE;
	}
	if (mem.ipl[0x1000d] != 0x05) {
		return FALSE;
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	CGチェック
//	※8x8ドットフォント(全機種共通)のSum,Xorでチェック
//
//---------------------------------------------------------------------------
BOOL FASTCALL Memory::CheckCG() const
{
	BYTE add;
	BYTE eor;
	BYTE *ptr;
	int i;

	ASSERT(this);
	ASSERT(mem.now != None);

	// 存在チェック
	if (!mem.cg) {
		return FALSE;
	}

	// 初期設定
	add = 0;
	eor = 0;
	ptr = &mem.cg[0x3a800];

	// ADD, XORループ
	for (i=0; i<0x1000; i++) {
		add = (BYTE)(add + *ptr);
		eor ^= *ptr;
		ptr++;
	}

	// チェック(XVIでの実測値)
	if ((add != 0xec) || (eor != 0x84)) {
		return FALSE;
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	CG取得
//
//---------------------------------------------------------------------------
const BYTE* FASTCALL Memory::GetCG() const
{
	ASSERT(this);
	ASSERT(mem.cg);

	return mem.cg;
}

//---------------------------------------------------------------------------
//
//	SCSI取得
//
//---------------------------------------------------------------------------
const BYTE* FASTCALL Memory::GetSCSI() const
{
	ASSERT(this);
	ASSERT(mem.scsi);

	return mem.scsi;
}
