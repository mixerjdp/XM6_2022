//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 ＰＩ．(ytanaka@ipc-tokai.or.jp)
//	[ プリンタ ]
//
//---------------------------------------------------------------------------

#include "os.h"
#include "xm6.h"
#include "vm.h"
#include "cpu.h"
#include "log.h"
#include "iosc.h"
#include "sync.h"
#include "fileio.h"
#include "printer.h"

//===========================================================================
//
//	プリンタ
//
//===========================================================================
//#define PRINTER_LOG

//---------------------------------------------------------------------------
//
//	コンストラクタ
//
//---------------------------------------------------------------------------
Printer::Printer(VM *p) : MemDevice(p)
{
	// デバイスIDを初期化
	dev.id = MAKEID('P', 'R', 'N', ' ');
	dev.desc = "Printer";

	// 開始アドレス、終了アドレス
	memdev.first = 0xe8c000;
	memdev.last = 0xe8dfff;

	// その他
	iosc = NULL;
	sync = NULL;
}

//---------------------------------------------------------------------------
//
//	初期化
//
//---------------------------------------------------------------------------
BOOL FASTCALL Printer::Init()
{
	ASSERT(this);

	// 基本クラス
	if (!MemDevice::Init()) {
		return FALSE;
	}

	// IOSCを取得
	iosc = (IOSC*)vm->SearchDevice(MAKEID('I', 'O', 'S', 'C'));
	ASSERT(iosc);

	// Sync作成
	sync = new Sync;

	// 接続しない
	printer.connect = FALSE;

	// バッファをクリア
	sync->Lock();
	printer.read = 0;
	printer.write = 0;
	printer.num = 0;
	sync->Unlock();

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	クリーンアップ
//
//---------------------------------------------------------------------------
void FASTCALL Printer::Cleanup()
{
	ASSERT(this);

	// Sync削除
	if (sync) {
		delete sync;
		sync = NULL;
	}

	// 基本クラスへ
	MemDevice::Cleanup();
}

//---------------------------------------------------------------------------
//
//	リセット
//
//---------------------------------------------------------------------------
void FASTCALL Printer::Reset()
{
	ASSERT(this);

	LOG0(Log::Normal, "リセット");

	// ストローブ(負論理)
	printer.strobe = FALSE;

	// レディ(正論理)
	if (printer.connect) {
		// 接続されていればREADY
		printer.ready = TRUE;
	}
	else {
		// 非接続ならBUSY
		printer.ready = FALSE;
	}

	// 割り込み無し
	iosc->IntPRT(FALSE);
}

//---------------------------------------------------------------------------
//
//	セーブ
//
//---------------------------------------------------------------------------
BOOL FASTCALL Printer::Save(Fileio *fio, int /* ver */)
{
	size_t sz;

	ASSERT(this);
	ASSERT(fio);
	LOG0(Log::Normal, "セーブ");

	// サイズをセーブ
	sz = sizeof(printer_t);
	if (!fio->Write(&sz, (int)sizeof(sz))) {
		return FALSE;
	}

	// 実体をセーブ
	if (!fio->Write(&printer, (int)sz)) {
		return FALSE;
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	ロード
//
//---------------------------------------------------------------------------
BOOL FASTCALL Printer::Load(Fileio *fio, int /* ver */)
{
	size_t sz;

	ASSERT(this);
	ASSERT(fio);
	LOG0(Log::Normal, "ロード");

	// サイズをロード、照合
	if (!fio->Read(&sz, sizeof(sz))) {
		return FALSE;
	}
	if (sz != sizeof(printer_t)) {
		return FALSE;
	}

	// 実体をロード
	if (!fio->Read(&printer, (int)sz)) {
		return FALSE;
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	設定適用
//
//---------------------------------------------------------------------------
void FASTCALL Printer::ApplyCfg(const Config* /*config*/)
{
	ASSERT(this);
	LOG0(Log::Normal, "設定適用");

	// 設定変更時、コンポーネントからConnect()が呼ばれるので、READYが変わる
}

//---------------------------------------------------------------------------
//
//	バイト読み込み
//
//---------------------------------------------------------------------------
DWORD FASTCALL Printer::ReadByte(DWORD /*addr*/)
{
	ASSERT(this);

	// 常に0xff(Write Onlyのため)
	return 0xff;
}

//---------------------------------------------------------------------------
//
//	ワード読み込み
//
//---------------------------------------------------------------------------
DWORD FASTCALL Printer::ReadWord(DWORD /*addr*/)
{
	ASSERT(this);

	// 常に0xff(Write Onlyのため)
	return 0xff;
}

//---------------------------------------------------------------------------
//
//	バイト書き込み
//
//---------------------------------------------------------------------------
void FASTCALL Printer::WriteByte(DWORD addr, DWORD data)
{
	ASSERT(this);
	ASSERT((addr >= memdev.first) && (addr <= memdev.last));

	ASSERT(printer.num <= BufMax);
	ASSERT(printer.read < BufMax);
	ASSERT(printer.write < BufMax);

	// 4バイト単位でループ(予想。WriteOnlyポートのため不明)
	addr &= 0x03;

	// デコード
	switch (addr) {
		// $E8C001 出力データ
		case 1:
			printer.data = (BYTE)data;
			break;

		// $E8C003 ストローブ
		case 3:
			// ストローブは0(TRUE)→1(FALSE)への変化で効力をもつ
			if ((data & 1) == 0) {
#if defined(PRINTER_LOG)
				LOG0(Log::Normal, "ストローブ 0(TRUE)");
#endif	// PRINTER_LOG
				printer.strobe = TRUE;
				break;
			}

			// ストローブがTRUEでなければ、ここまで
			if (!printer.strobe) {
				break;
			}

			// ストローブをFALSEに
			printer.strobe = FALSE;
#if defined(PRINTER_LOG)
			LOG0(Log::Normal, "ストローブ 1(FALSE)");
#endif	// PRINTER_LOG

			// 接続されていなければ、ここまで
			if (!printer.connect) {
				break;
			}

			// ここでデータをラッチ
#if defined(PRINTER_LOG)
			LOG1(Log::Normal, "データ確定 $%02X", printer.data);
#endif	// PRINTER_LOG

			sync->Lock();
			// データ挿入
			printer.buf[printer.write] = printer.data;
			printer.write = (printer.write + 1) & (BufMax - 1);
			printer.num++;

			// ウェイト制御が入るため、バッファは溢れないはず
			if (printer.num > BufMax) {
				ASSERT(FALSE);
				LOG0(Log::Warning, "出力バッファオーバーフロー");
				printer.num = BufMax;
			}
			sync->Unlock();

			// READYを落とす
			printer.ready = FALSE;

			// 割り込み無し
			iosc->IntPRT(FALSE);
			break;
	}
}

//---------------------------------------------------------------------------
//
//	ワード書き込み
//
//---------------------------------------------------------------------------
void FASTCALL Printer::WriteWord(DWORD addr, DWORD data)
{
	ASSERT(this);
	ASSERT((addr >= memdev.first) && (addr <= memdev.last));
	ASSERT((addr & 1) == 0);

	WriteByte(addr + 1, (BYTE)data);
}

//---------------------------------------------------------------------------
//
//	読み込みのみ
//
//---------------------------------------------------------------------------
DWORD FASTCALL Printer::ReadOnly(DWORD /*addr*/) const
{
	ASSERT(this);

	// 常に0xff(Write Onlyのため)
	return 0xff;
}

//---------------------------------------------------------------------------
//
//	H-Sync通知
//
//---------------------------------------------------------------------------
void FASTCALL Printer::HSync()
{
	ASSERT(this);

	// レディがFALSE(データ転送中)でなければ関係なし
	if (printer.ready) {
		return;
	}

	// 接続されていなければ
	if (!printer.connect) {
		// printer.readyはFALSEのまま
		return;
	}

	// バッファが一杯なら、次まで引き延ばす
	if (printer.num == BufMax) {
#if defined(PRINTER_LOG)
	LOG0(Log::Normal, "バッファフルのためウェイト");
#endif	// PRINTER_LOG
		return;
	}

#if defined(PRINTER_LOG)
	LOG0(Log::Normal, "データ送信完了、READY");
#endif	// PRINTER_LOG

	// レディ状態
	printer.ready = TRUE;

	// 割り込み設定
	iosc->IntPRT(TRUE);
}

//---------------------------------------------------------------------------
//
//	内部データ取得
//
//---------------------------------------------------------------------------
void FASTCALL Printer::GetPrinter(printer_t *buffer) const
{
	ASSERT(this);
	ASSERT(buffer);

	// 内部ワークをコピー
	*buffer = printer;
}

//---------------------------------------------------------------------------
//
//	接続
//
//---------------------------------------------------------------------------
void FASTCALL Printer::Connect(BOOL flag)
{
	ASSERT(this);
	ASSERT(printer.num <= BufMax);
	ASSERT(printer.read < BufMax);
	ASSERT(printer.write < BufMax);

	// 一致していれば何もしない
	if (printer.connect == flag) {
		return;
	}

	// 設定
	printer.connect = flag;
#if defined(PRINTER_LOG)
	if (printer.connect) {
		LOG0(Log::Normal, "プリンタ接続");
	}
	else {
		LOG0(Log::Normal, "プリンタ切断");
	}
#endif	// PRINTER_LOG

	// FALSEにするなら、レディ下ろす
	if (!printer.connect) {
		printer.ready = FALSE;
		iosc->IntPRT(FALSE);
		return;
	}

	// TRUEにするなら、次のHSYNCでレディ上げる
	sync->Lock();
	printer.read = 0;
	printer.write = 0;
	printer.num = 0;
	sync->Unlock();
}

//---------------------------------------------------------------------------
//
//	先頭データ取得
//
//---------------------------------------------------------------------------
BOOL FASTCALL Printer::GetData(BYTE *ptr)
{
	ASSERT(this);
	ASSERT(ptr);
	ASSERT(printer.num <= BufMax);
	ASSERT(printer.read < BufMax);
	ASSERT(printer.write < BufMax);

	// ロック
	sync->Lock();

	// データがなければFALSE
	if (printer.num == 0) {
		sync->Unlock();
		return FALSE;
	}

	// データ取得
	*ptr = printer.buf[printer.read];

	// 次へ
	printer.read = (printer.read + 1) & (BufMax - 1);
	printer.num--;

	// アンロック
	sync->Unlock();
	return TRUE;
}
