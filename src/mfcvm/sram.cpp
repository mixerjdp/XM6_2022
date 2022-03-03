//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 ＰＩ．(ytanaka@ipc-tokai.or.jp)
//	[ スタティックRAM ]
//
//---------------------------------------------------------------------------

#include "os.h"
#include "xm6.h"
#include "vm.h"
#include "log.h"
#include "filepath.h"
#include "fileio.h"
#include "cpu.h"
#include "schedule.h"
#include "memory.h"
#include "config.h"
#include "sram.h"

//===========================================================================
//
//	スタティックRAM
//
//===========================================================================
//#define SRAM_LOG

//---------------------------------------------------------------------------
//
//	コンストラクタ
//
//---------------------------------------------------------------------------
SRAM::SRAM(VM *p) : MemDevice(p)
{
	// デバイスIDを初期化
	dev.id = MAKEID('S', 'R', 'A', 'M');
	dev.desc = "Static RAM";

	// 開始アドレス、終了アドレス
	memdev.first = 0xed0000;
	memdev.last = 0xedffff;

	// その他初期化
	sram_size = 16;
	write_en = FALSE;
	mem_sync = TRUE;
	changed = FALSE;
}

//---------------------------------------------------------------------------
//
//	初期化
//
//---------------------------------------------------------------------------
BOOL FASTCALL SRAM::Init()
{
	Fileio fio;
	int i;
	BYTE data;

	ASSERT(this);

	// 基本クラス
	if (!MemDevice::Init()) {
		return FALSE;
	}

	// 初期化
	memset(sram, 0xff, sizeof(sram));

	// パス作成、読み込み
	sram_path.SysFile(Filepath::SRAM);
	fio.Load(sram_path, sram, sizeof(sram));

	// エンディアン反転
	for (i=0; i<sizeof(sram); i+=2) {
		data = sram[i];
		sram[i] = sram[i + 1];
		sram[i + 1] = data;
	}

	// 変更なし
	ASSERT(!changed);

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	クリーンアップ
//
//---------------------------------------------------------------------------
void FASTCALL SRAM::Cleanup()
{
	Fileio fio;
	int i;
	BYTE data;

	ASSERT(this);

	// 変更あれば
	if (changed) {
		// エンディアン反転
		for (i=0; i<sizeof(sram); i+=2) {
			data = sram[i];
			sram[i] = sram[i + 1];
			sram[i + 1] = data;
		}

		// 書き込み
		fio.Save(sram_path, sram, sram_size << 10);
	}

	// 基本クラスへ
	MemDevice::Cleanup();
}

//---------------------------------------------------------------------------
//
//	リセット
//
//---------------------------------------------------------------------------
void FASTCALL SRAM::Reset()
{
	ASSERT(this);
	ASSERT_DIAG();

	LOG0(Log::Normal, "リセット");

	// 書き込み禁止に初期化
	write_en = FALSE;
}

//---------------------------------------------------------------------------
//
//	セーブ
//
//---------------------------------------------------------------------------
BOOL FASTCALL SRAM::Save(Fileio *fio, int /*ver*/)
{
	ASSERT(this);
	ASSERT(fio);
	ASSERT_DIAG();

	LOG0(Log::Normal, "ロード");

	// SRAMサイズ
	if (!fio->Write(&sram_size, sizeof(sram_size))) {
		return FALSE;
	}

	// SRAM本体(64KBまとめて)
	if (!fio->Write(&sram, sizeof(sram))) {
		return FALSE;
	}

	// 書き込み許可フラグ
	if (!fio->Write(&write_en, sizeof(write_en))) {
		return FALSE;
	}

	// 同期フラグ
	if (!fio->Write(&mem_sync, sizeof(mem_sync))) {
		return FALSE;
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	ロード
//
//---------------------------------------------------------------------------
BOOL FASTCALL SRAM::Load(Fileio *fio, int /*ver*/)
{
	BYTE *buf;

	ASSERT(this);
	ASSERT(fio);
	ASSERT_DIAG();

	LOG0(Log::Normal, "ロード");

	// バッファ確保
	try {
		buf = new BYTE[sizeof(sram)];
	}
	catch (...) {
		buf = NULL;
	}
	if (!buf) {
		return FALSE;
	}

	// SRAMサイズ
	if (!fio->Read(&sram_size, sizeof(sram_size))) {
		delete[] buf;
		return FALSE;
	}

	// SRAM本体(64KBまとめて)
	if (!fio->Read(buf, sizeof(sram))) {
		delete[] buf;
		return FALSE;
	}

	// 比較と転送
	if (memcmp(sram, buf, sizeof(sram)) != 0) {
		memcpy(sram, buf, sizeof(sram));
		changed = TRUE;
	}

	// 先にバッファ解放
	delete[] buf;
	buf = NULL;

	// 書き込み許可フラグ
	if (!fio->Read(&write_en, sizeof(write_en))) {
		return FALSE;
	}

	// 同期フラグ
	if (!fio->Read(&mem_sync, sizeof(mem_sync))) {
		return FALSE;
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	設定適用
//
//---------------------------------------------------------------------------
void FASTCALL SRAM::ApplyCfg(const Config *config)
{
	ASSERT(this);
	ASSERT(config);
	ASSERT_DIAG();

	LOG0(Log::Normal, "設定適用");

	// SRAMサイズ
	if (config->sram_64k) {
		sram_size = 64;
#if defined(SRAM_LOG)
		LOG0(Log::Detail, "メモリサイズ 64KB");
#endif	// SRAM_LOG
	}
	else {
		sram_size = 16;
	}

	// メモリスイッチ書き込み
	mem_sync = config->ram_sramsync;
}

#if !defined(NDEBUG)
//---------------------------------------------------------------------------
//
//	診断
//
//---------------------------------------------------------------------------
void FASTCALL SRAM::AssertDiag() const
{
	// 基本クラス
	MemDevice::AssertDiag();

	ASSERT(this);
	ASSERT(GetID() == MAKEID('S', 'R', 'A', 'M'));
	ASSERT(memdev.first == 0xed0000);
	ASSERT(memdev.last == 0xedffff);
	ASSERT((sram_size == 16) || (sram_size == 32) || (sram_size == 48) || (sram_size == 64));
	ASSERT((write_en == TRUE) || (write_en == FALSE));
	ASSERT((mem_sync == TRUE) || (mem_sync == FALSE));
	ASSERT((changed == TRUE) || (changed == FALSE));
}
#endif	// NDEBUG

//---------------------------------------------------------------------------
//
//	バイト読み込み
//
//---------------------------------------------------------------------------
DWORD FASTCALL SRAM::ReadByte(DWORD addr)
{
	DWORD size;

	ASSERT(this);
	ASSERT((addr >= memdev.first) && (addr <= memdev.last));
	ASSERT_DIAG();

	// オフセット算出
	addr -= memdev.first;
	size = (DWORD)(sram_size << 10);

	// 未実装チェック
	if (size <= addr) {
		// バスエラー
		cpu->BusErr(memdev.first + addr, TRUE);
		return 0xff;
	}

	// ウェイト
	scheduler->Wait(1);

	// 読み込み(エンディアンを反転させる)
	return (DWORD)sram[addr ^ 1];
}

//---------------------------------------------------------------------------
//
//	ワード読み込み
//
//---------------------------------------------------------------------------
DWORD FASTCALL SRAM::ReadWord(DWORD addr)
{
	DWORD size;

	ASSERT(this);
	ASSERT((addr >= memdev.first) && (addr <= memdev.last));
	ASSERT((addr & 1) == 0);
	ASSERT_DIAG();

	// オフセット算出
	addr -= memdev.first;
	size = (DWORD)(sram_size << 10);

	// 未実装チェック
	if (size <= addr) {
		// バスエラー
		cpu->BusErr(memdev.first + addr, TRUE);
		return 0xff;
	}

	// ウェイト
	scheduler->Wait(1);

	// 読み込み(エンディアンに注意)
	return (DWORD)(*(WORD *)&sram[addr]);
}

//---------------------------------------------------------------------------
//
//	バイト書き込み
//
//---------------------------------------------------------------------------
void FASTCALL SRAM::WriteByte(DWORD addr, DWORD data)
{
	DWORD size;

	ASSERT(this);
	ASSERT((addr >= memdev.first) && (addr <= memdev.last));
	ASSERT(data < 0x100);
	ASSERT_DIAG();

	// オフセット算出
	addr -= memdev.first;
	size = (DWORD)(sram_size << 10);

	// 未実装チェック
	if (size <= addr) {
		// バスエラー
		cpu->BusErr(memdev.first + addr, FALSE);
		return;
	}

	// 書き込み可能チェック
	if (!write_en) {
		LOG1(Log::Warning, "書き込み禁止 $%06X", memdev.first + addr);
		return;
	}

	// ウェイト
	scheduler->Wait(1);

	// アドレス$09に$00 or $10が書き込まれる場合、メモリスイッチ自動更新であればスキップさせる
	// (リセット時にMemory::Resetから書き込まれているため、上書きによる破壊を防ぐ)
	if ((addr == 0x09) && (data == 0x10)) {
		if (cpu->GetPC() == 0xff03a8) {
			if (mem_sync) {
				LOG2(Log::Warning, "スイッチ変更抑制 $%06X <- $%02X", memdev.first + addr, data);
				return;
			}
		}
	}

	// 書き込み(エンディアンを反転させる)
	if (addr < 0x100) {
		LOG2(Log::Detail, "メモリスイッチ変更 $%06X <- $%02X", memdev.first + addr, data);
	}
	if (sram[addr ^ 1] != (BYTE)data) {
		sram[addr ^ 1] = (BYTE)data;
		changed = TRUE;
	}
}

//---------------------------------------------------------------------------
//
//	ワード書き込み
//
//---------------------------------------------------------------------------
void FASTCALL SRAM::WriteWord(DWORD addr, DWORD data)
{
	DWORD size;

	ASSERT(this);
	ASSERT((addr >= memdev.first) && (addr <= memdev.last));
	ASSERT((addr & 1) == 0);
	ASSERT(data < 0x10000);
	ASSERT_DIAG();

	// オフセット算出
	addr -= memdev.first;
	size = (DWORD)(sram_size << 10);

	// 未実装チェック
	if (size <= addr) {
		// バスエラー
		cpu->BusErr(memdev.first + addr, FALSE);
		return;
	}

	// 書き込み可能チェック
	if (!write_en) {
		LOG1(Log::Warning, "書き込み禁止 $%06X", memdev.first + addr);
		return;
	}

	// ウェイト
	scheduler->Wait(1);

	// 書き込み(エンディアンに注意)
	if (addr < 0x100) {
		LOG2(Log::Detail, "メモリスイッチ変更 $%06X <- $%04X", memdev.first + addr, data);
	}
	if (*(WORD *)&sram[addr] != (WORD)data) {
		*(WORD *)&sram[addr] = (WORD)data;
		changed = TRUE;
	}
}

//---------------------------------------------------------------------------
//
//	読み込みのみ
//
//---------------------------------------------------------------------------
DWORD FASTCALL SRAM::ReadOnly(DWORD addr) const
{
	DWORD size;

	ASSERT(this);
	ASSERT((addr >= memdev.first) && (addr <= memdev.last));
	ASSERT_DIAG();

	// オフセット算出
	addr -= memdev.first;
	size = (DWORD)(sram_size << 10);

	// 未実装チェック
	if (size <= addr) {
		return 0xff;
	}

	// 読み込み(エンディアンを反転させる)
	return (DWORD)sram[addr ^ 1];
}

//---------------------------------------------------------------------------
//
//	SRAMアドレス取得
//
//---------------------------------------------------------------------------
const BYTE* FASTCALL SRAM::GetSRAM() const
{
	ASSERT(this);
	ASSERT_DIAG();

	return sram;
}

//---------------------------------------------------------------------------
//
//	SRAMサイズ取得
//
//---------------------------------------------------------------------------
int FASTCALL SRAM::GetSize() const
{
	ASSERT(this);
	ASSERT_DIAG();

	return sram_size;
}

//---------------------------------------------------------------------------
//
//	書き込み許可
//
//---------------------------------------------------------------------------
void FASTCALL SRAM::WriteEnable(BOOL enable)
{
	ASSERT(this);
	ASSERT_DIAG();

	write_en = enable;

	if (write_en) {
		LOG0(Log::Detail, "SRAM書き込み許可");
	}
	else {
		LOG0(Log::Detail, "SRAM書き込み禁止");
	}
}

//---------------------------------------------------------------------------
//
//	メモリスイッチ設定
//
//---------------------------------------------------------------------------
void FASTCALL SRAM::SetMemSw(DWORD offset, DWORD data)
{
	ASSERT(this);
	ASSERT(offset < 0x100);
	ASSERT(data < 0x100);
	ASSERT_DIAG();

	LOG2(Log::Detail, "メモリスイッチ設定 $%06X <- $%02X", memdev.first + offset, data);
	if (sram[offset ^ 1] != (BYTE)data) {
		sram[offset ^ 1] = (BYTE)data;
		changed = TRUE;
	}
}

//---------------------------------------------------------------------------
//
//	メモリスイッチ取得
//
//---------------------------------------------------------------------------
DWORD FASTCALL SRAM::GetMemSw(DWORD offset) const
{
	ASSERT(this);
	ASSERT(offset < 0x100);
	ASSERT_DIAG();

	return (DWORD)sram[offset ^ 1];
}

//---------------------------------------------------------------------------
//
//	起動カウンタ更新
//
//---------------------------------------------------------------------------
void FASTCALL SRAM::UpdateBoot()
{
	WORD *ptr;

	ASSERT(this);
	ASSERT_DIAG();

	// 常に変更あり
	changed = TRUE;

	// ポインタ設定($ED0044)
	ptr = (WORD *)&sram[0x0044];

	// インクリメント(Low)
	if (ptr[1] != 0xffff) {
		ptr[1] = ptr[1] + 1;
		return;
	}

	// インクリメント(High)
	ptr[1] = 0x0000;
	ptr[0] = ptr[0] + 1;
}
