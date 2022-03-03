//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2005 ＰＩ．(ytanaka@ipc-tokai.or.jp)
//	[ エリアセット ]
//
//---------------------------------------------------------------------------

#include "os.h"
#include "xm6.h"
#include "vm.h"
#include "cpu.h"
#include "log.h"
#include "fileio.h"
#include "memory.h"
#include "areaset.h"

//===========================================================================
//
//	エリアセット
//
//===========================================================================
//#define AREASET_LOG

//---------------------------------------------------------------------------
//
//	コンストラクタ
//
//---------------------------------------------------------------------------
AreaSet::AreaSet(VM *p) : MemDevice(p)
{
	// デバイスIDを初期化
	dev.id = MAKEID('A', 'R', 'E', 'A');
	dev.desc = "Area Set";

	// 開始アドレス、終了アドレス
	memdev.first = 0xe86000;
	memdev.last = 0xe87fff;

	// オブジェクト
	memory = NULL;
}

//---------------------------------------------------------------------------
//
//	初期化
//
//---------------------------------------------------------------------------
BOOL FASTCALL AreaSet::Init()
{
	ASSERT(this);

	// 基本クラス
	if (!MemDevice::Init()) {
		return FALSE;
	}

	// メモリ取得
	memory = (Memory*)vm->SearchDevice(MAKEID('M', 'E', 'M', ' '));
	ASSERT(memory);

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	クリーンアップ
//
//---------------------------------------------------------------------------
void FASTCALL AreaSet::Cleanup()
{
	ASSERT(this);

	// 基本クラスへ
	MemDevice::Cleanup();
}

//---------------------------------------------------------------------------
//
//	リセット
//	※正規の順序でなく、Memory::MakeContextから呼ばれる
//
//---------------------------------------------------------------------------
void FASTCALL AreaSet::Reset()
{
	ASSERT(this);
	LOG0(Log::Normal, "リセット");

#if defined(AREASET_LOG)
	LOG0(Log::Normal, "エリアセット設定 $00");
#endif	// AREASET_LOG

	// エリア指定初期化
	area = 0;
}

//---------------------------------------------------------------------------
//
//	セーブ
//
//---------------------------------------------------------------------------
BOOL FASTCALL AreaSet::Save(Fileio *fio, int /*ver*/)
{
	size_t sz;

	ASSERT(this);
	LOG0(Log::Normal, "セーブ");

	// サイズをセーブ
	sz = sizeof(area);
	if (!fio->Write(&sz, (int)sizeof(sz))) {
		return FALSE;
	}

	// エリア情報をセーブ
	if (!fio->Write(&area, (int)sizeof(area))) {
		return FALSE;
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	ロード
//
//---------------------------------------------------------------------------
BOOL FASTCALL AreaSet::Load(Fileio *fio, int /*ver*/)
{
	size_t sz;

	ASSERT(this);
	LOG0(Log::Normal, "ロード");

	// サイズをロード
	if (!fio->Read(&sz, (int)sizeof(sz))) {
		return FALSE;
	}

	// サイズを比較
	if (sz != sizeof(area)) {
		return FALSE;
	}

	// エリア情報をロード
	if (!fio->Read(&area, (int)sizeof(area))) {
		return FALSE;
	}

	// 適用
	memory->MakeContext(FALSE);

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	設定適用
//
//---------------------------------------------------------------------------
void FASTCALL AreaSet::ApplyCfg(const Config* /*config*/)
{
	ASSERT(this);
	LOG0(Log::Normal, "設定適用");
}

//---------------------------------------------------------------------------
//
//	バイト読み込み
//
//---------------------------------------------------------------------------
DWORD FASTCALL AreaSet::ReadByte(DWORD addr)
{
	ASSERT(this);
	ASSERT((addr >= memdev.first) && (addr <= memdev.last));

	// 常にバスエラー
	cpu->BusErr(addr, TRUE);
	return 0xff;
}

//---------------------------------------------------------------------------
//
//	バイト書き込み
//
//---------------------------------------------------------------------------
void FASTCALL AreaSet::WriteByte(DWORD addr, DWORD data)
{
	ASSERT(this);
	ASSERT((addr >= memdev.first) && (addr <= memdev.last));

	// 2バイトおきにマップ
	addr &= 1;

	// 奇数アドレスはエリアセット
	if (addr & 1) {
		LOG1(Log::Detail, "エリアセット設定 $%02X", data);

		// データ記憶
		area = data;

		// メモリマップ再構築
		memory->MakeContext(FALSE);
		return;
	}

	// 偶数アドレスはデコードされていない
}

//---------------------------------------------------------------------------
//
//	読み込みのみ
//
//---------------------------------------------------------------------------
DWORD FASTCALL AreaSet::ReadOnly(DWORD addr) const
{
	ASSERT(this);
	ASSERT((addr >= memdev.first) && (addr <= memdev.last));

	// EVENは0xff、ODDは設定値を返す
	if (addr & 1) {
		return area;
	}
	return 0xff;
}

//---------------------------------------------------------------------------
//
//	エリアセット取得
//
//---------------------------------------------------------------------------
DWORD FASTCALL AreaSet::GetArea() const
{
	ASSERT(this);
	return area;
}
