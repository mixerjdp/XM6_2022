//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 ＰＩ．(ytanaka@ipc-tokai.or.jp)
//	[ CPU(MC68000) ]
//
//---------------------------------------------------------------------------

#include "os.h"
#include "xm6.h"
#include "iosc.h"
#include "mfp.h"
#include "vm.h"
#include "log.h"
#include "memory.h"
#include "dmac.h"
#include "scc.h"
#include "midi.h"
#include "scsi.h"
#include "fileio.h"
#include "cpu.h"

//---------------------------------------------------------------------------
//
//	アセンブラコアとのインタフェース
//
//---------------------------------------------------------------------------
#if defined(__cplusplus)
extern "C" {
#endif	// __cplusplus

//---------------------------------------------------------------------------
//
//	スタティック ワーク
//
//---------------------------------------------------------------------------
static CPU *cpu;
										// CPU

//---------------------------------------------------------------------------
//
//	外部定義
//
//---------------------------------------------------------------------------
DWORD s68000fbpc(void);
										// PCフィードバック
void s68000buserr(DWORD addr, DWORD param);
										// バスエラー

//---------------------------------------------------------------------------
//
//	RESET命令ハンドラ
//
//---------------------------------------------------------------------------
static void cpu_resethandler(void)
{
	cpu->ResetInst();
}

//---------------------------------------------------------------------------
//
//	割り込みACK
//
//---------------------------------------------------------------------------
void s68000intack(void)
{
	int sr;

	sr = ::s68000context.sr;
	sr >>= 8;
	sr &= 0x0007;

	cpu->IntAck(sr);
}

//---------------------------------------------------------------------------
//
//	バスエラー記録
//
//---------------------------------------------------------------------------
void s68000buserrlog(DWORD addr, DWORD stat)
{
	cpu->BusErrLog(addr, stat);
}

//---------------------------------------------------------------------------
//
//	アドレスエラー記録
//
//---------------------------------------------------------------------------
void s68000addrerrlog(DWORD addr, DWORD stat)
{
	cpu->AddrErrLog(addr, stat);
}

#if defined(__cplusplus)
}
#endif	// __cplusplus

//===========================================================================
//
//	CPU
//
//===========================================================================
//#define CPU_LOG

//---------------------------------------------------------------------------
//
//	コンストラクタ
//
//---------------------------------------------------------------------------
CPU::CPU(VM *p) : Device(p)
{
	// デバイスIDを初期化
	dev.id = MAKEID('C', 'P', 'U', ' ');
	dev.desc = "MPU (MC68000)";

	// ポインタ初期化
	memory = NULL;
	dmac = NULL;
	mfp = NULL;
	iosc = NULL;
	scc = NULL;
	midi = NULL;
	scsi = NULL;
	scheduler = NULL;
}

//---------------------------------------------------------------------------
//
//	初期化
//
//---------------------------------------------------------------------------
BOOL FASTCALL CPU::Init()
{
	ASSERT(this);

	// 基本クラス
	if (!Device::Init()) {
		return FALSE;
	}

	// CPU記憶
	::cpu = this;

	// メモリ取得
	memory = (Memory*)vm->SearchDevice(MAKEID('M', 'E', 'M', ' '));
	ASSERT(memory);

	// DMAC取得
	dmac = (DMAC*)vm->SearchDevice(MAKEID('D', 'M', 'A', 'C'));
	ASSERT(dmac);

	// MFP取得
	mfp = (MFP*)vm->SearchDevice(MAKEID('M', 'F', 'P', ' '));
	ASSERT(mfp);

	// IOSC取得
	iosc = (IOSC*)vm->SearchDevice(MAKEID('I', 'O', 'S', 'C'));
	ASSERT(iosc);

	// SCC取得
	scc = (SCC*)vm->SearchDevice(MAKEID('S', 'C', 'C', ' '));
	ASSERT(scc);

	// MIDI取得
	midi = (MIDI*)vm->SearchDevice(MAKEID('M', 'I', 'D', 'I'));
	ASSERT(midi);

	// SCSI取得
	scsi = (SCSI*)vm->SearchDevice(MAKEID('S', 'C', 'S', 'I'));
	ASSERT(scsi);

	// スケジューラ取得
	scheduler = (Scheduler*)vm->SearchDevice(MAKEID('S', 'C', 'H', 'E'));
	ASSERT(scheduler);

	// CPUコアのジャンプテーブルを作成
	::s68000init();

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	クリーンアップ
//
//---------------------------------------------------------------------------
void FASTCALL CPU::Cleanup()
{
	ASSERT(this);

	// 基本クラスへ
	Device::Cleanup();
}

//---------------------------------------------------------------------------
//
//	リセット
//
//---------------------------------------------------------------------------
void FASTCALL CPU::Reset()
{
	int i;
	S68000CONTEXT context;
	DWORD bit;

	ASSERT(this);
	LOG0(Log::Normal, "リセット");

	// エラーアドレス、エラー時間クリア
	sub.erraddr = 0;
	sub.errtime = 0;

	// 割り込みカウントクリア
	for (i=0; i<8; i++) {
		sub.intreq[i] = 0;
		sub.intack[i] = 0;
	}

	// メモリコンテキスト作成(リセット専用)
	memory->MakeContext(TRUE);

	// リセット
	::s68000reset();
	::s68000context.resethandler = cpu_resethandler;
	::s68000context.odometer = 0;

	// 割り込みをすべて取り消す
	::s68000GetContext(&context);
	for (i=1; i<=7; i++) {
		bit = (1 << i);
		if (context.interrupts[0] & bit) {
			context.interrupts[0] &= (BYTE)(~bit);
			context.interrupts[i] = 0;
		}
	}
	::s68000SetContext(&context);

	// メモリコンテキスト作成(通常)
	memory->MakeContext(FALSE);
}

//---------------------------------------------------------------------------
//
//	セーブ
//
//---------------------------------------------------------------------------
BOOL FASTCALL CPU::Save(Fileio *fio, int /*ver*/)
{
	size_t sz;
	cpu_t cpu;

	ASSERT(this);
	ASSERT(fio);

	LOG0(Log::Normal, "セーブ");

	// コンテキスト取得
	GetCPU(&cpu);

	// サイズをセーブ
	sz = sizeof(cpu_t);
	if (!fio->Write(&sz, sizeof(sz))) {
		return FALSE;
	}

	// 実体をセーブ
	if (!fio->Write(&cpu, (int)sz)) {
		return FALSE;
	}

	// サイズをセーブ(サブ)
	sz = sizeof(cpusub_t);
	if (!fio->Write(&sz, sizeof(sz))) {
		return FALSE;
	}

	// 実体をセーブ(サブ)
	if (!fio->Write(&sub, (int)sz)) {
		return FALSE;
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	ロード
//
//---------------------------------------------------------------------------
BOOL FASTCALL CPU::Load(Fileio *fio, int /*ver*/)
{
	cpu_t cpu;
	size_t sz;

	ASSERT(this);
	ASSERT(fio);

	LOG0(Log::Normal, "ロード");

	// サイズをロード、照合
	if (!fio->Read(&sz, sizeof(sz))) {
		return FALSE;
	}
	if (sz != sizeof(cpu_t)) {
		return FALSE;
	}

	// 実体をロード
	if (!fio->Read(&cpu, (int)sz)) {
		return FALSE;
	}

	// 適用(リセットしてから行う)
	memory->MakeContext(TRUE);
	::s68000reset();
	memory->MakeContext(FALSE);
	SetCPU(&cpu);

	// サイズをロード、照合(サブ)
	if (!fio->Read(&sz, sizeof(sz))) {
		return FALSE;
	}
	if (sz != sizeof(cpusub_t)) {
		return FALSE;
	}

	// 実体をロード(サブ)
	if (!fio->Read(&sub, (int)sz)) {
		return FALSE;
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	設定適用
//
//---------------------------------------------------------------------------
void FASTCALL CPU::ApplyCfg(const Config* /*config*/)
{
	ASSERT(this);

	LOG0(Log::Normal, "設定適用");
}

//---------------------------------------------------------------------------
//
//	CPUレジスタ取得
//
//---------------------------------------------------------------------------
void FASTCALL CPU::GetCPU(cpu_t *buffer) const
{
	int i;

	ASSERT(this);
	ASSERT(buffer);

	// Dreg, Areg
	for (i=0; i<8; i++) {
		buffer->dreg[i] = ::s68000context.dreg[i];
		buffer->areg[i] = ::s68000context.areg[i];
	}

	// 割り込み
	for (i=0; i<8; i++) {
		buffer->intr[i] = (DWORD)::s68000context.interrupts[i];
		buffer->intreq[i] = sub.intreq[i];
		buffer->intack[i] = sub.intack[i];
	}

	// その他
	buffer->sp = ::s68000context.asp;
	buffer->pc = ::s68000context.pc;
	buffer->sr = (DWORD)::s68000context.sr;
	buffer->odd = ::s68000context.odometer;
}

//---------------------------------------------------------------------------
//
//	CPUレジスタ設定
//
//---------------------------------------------------------------------------
void FASTCALL CPU::SetCPU(const cpu_t *buffer)
{
	int i;
	S68000CONTEXT context;

	ASSERT(this);
	ASSERT(buffer);

	// コンテキスト取得
	::s68000GetContext(&context);

	// Dreg, Areg
	for (i=0; i<8; i++) {
		context.dreg[i] = buffer->dreg[i];
		context.areg[i] = buffer->areg[i];
	}

	// 割り込み
	for (i=0; i<8; i++) {
		context.interrupts[i] = (BYTE)buffer->intr[i];
		sub.intreq[i] = buffer->intreq[i];
		sub.intack[i] = buffer->intack[i];
	}

	// その他
	context.asp = buffer->sp;
	context.pc = buffer->pc;
	context.sr = (WORD)buffer->sr;
	context.odometer = buffer->odd;

	// コンテキスト設定
	::s68000SetContext(&context);
}

//---------------------------------------------------------------------------
//
//	割り込み
//
//---------------------------------------------------------------------------
BOOL FASTCALL CPU::Interrupt(int level, int vector)
{
	int ret;

	// INTERRUPT SWITCHによるNMI割り込みはベクタ-1
	ASSERT(this);
	ASSERT((level >= 1) && (level <= 7));
	ASSERT(vector >= -1);

	// リクエスト
	ret = ::s68000interrupt(level, vector);

	// 結果評価
	if (ret == 0) {
#if defined(CPU_LOG)
		LOG2(Log::Normal, "割り込み要求受理 レベル%d ベクタ$%02X", level, vector);
#endif	// CPU_LOG
		sub.intreq[level]++;
		return TRUE;
	}

	return FALSE;
}

//---------------------------------------------------------------------------
//
//	割り込みACK
//
//---------------------------------------------------------------------------
void FASTCALL CPU::IntAck(int level)
{
	ASSERT(this);
	ASSERT((level >= 1) && (level <= 7));

#if defined(CPU_LOG)
	LOG1(Log::Normal, "割り込み要求ACK レベル%d", level);
#endif	// CPU_LOG

	// カウントアップ
	sub.intack[level]++;

	// 割り込みレベル別
	switch (level) {
		// IOSC,SCSI(内蔵)
		case 1:
			iosc->IntAck();
			scsi->IntAck(1);
			break;

		// MIDI,SCSI(レベル2)
		case 2:
			midi->IntAck(2);
			scsi->IntAck(2);
			break;

		// DMAC
		case 3:
			dmac->IntAck();
			break;

		// MIDI,SCSI(レベル4)
		case 4:
			midi->IntAck(4);
			scsi->IntAck(4);
			break;

		// SCC
		case 5:
			scc->IntAck();
			break;

		// MFP
		case 6:
			mfp->IntAck();
			break;

		// その他
		default:
			break;
	}
}

//---------------------------------------------------------------------------
//
//	割り込みキャンセル
//
//---------------------------------------------------------------------------
void FASTCALL CPU::IntCancel(int level)
{
	S68000CONTEXT context;
	DWORD bit;

	ASSERT(this);
	ASSERT((level >= 1) && (level <= 7));

	// コンテキストを直接書き換える
	::s68000GetContext(&context);

	// 該当ビットがオンなら
	bit = (1 << level);
	if (context.interrupts[0] & bit) {
#if defined(CPU_LOG)
		LOG1(Log::Normal, "割り込みキャンセル レベル%d", level);
#endif	// CPU_LOG

		// ビットを降ろす
		context.interrupts[0] &= (BYTE)(~bit);

		// ベクタは0
		context.interrupts[level] = 0;

		// リクエストを下げる
		sub.intreq[level]--;
	}

	// コンテキストを書き込む
	::s68000SetContext(&context);
}

//---------------------------------------------------------------------------
//
//	RESET命令
//
//---------------------------------------------------------------------------
void FASTCALL CPU::ResetInst()
{
	Device *device;

	ASSERT(this);
	LOG0(Log::Detail, "RESET命令");

	// メモリを取得
	device = (Device*)vm->SearchDevice(MAKEID('M', 'E', 'M', ' '));
	ASSERT(device);

	// メモリデバイスに対してすべてリセットをかけておく
	// 正確には、CPUのRESET信号がどこまで伝わっているかによる
	while (device) {
		device->Reset();
		device = device->GetNextDevice();
	}
}

//---------------------------------------------------------------------------
//
//	バスエラー
//	※DMA転送によるバスエラーもここに来る
//	※CPUコア内部でバスエラーと判定した場合は、ここを経由しない
//
//---------------------------------------------------------------------------
void FASTCALL CPU::BusErr(DWORD addr, BOOL read)
{
	DWORD pc;
	DWORD stat;

	ASSERT(this);
	ASSERT(addr <= 0xffffff);

	// DMACに転送中か聞く。DMAC中ならDMACに任せる
	if (dmac->IsDMA()) {
		dmac->BusErr(addr, read);
		return;
	}

	// アドレスが前回のアドレス+2で、かつ時間が同じなら無視する(LONGアクセス)
	if (addr == (sub.erraddr + 2)) {
		if (scheduler->GetTotalTime() == sub.errtime) {
			return;
		}
	}

	// アドレスと時間を更新
	sub.erraddr = addr;
	sub.errtime = scheduler->GetTotalTime();

	// PC取得(該当命令のオペコードに位置する)
	pc = GetPC();

	// 読み出し(Word)
	stat = memory->ReadOnly(pc);
	stat <<= 8;
	stat |= memory->ReadOnly(pc + 1);
	stat <<= 16;

	// ファンクションコード作成(常にデータアクセスとみなす)
	stat |= 0x09;
	if (::s68000context.sr & 0x2000) {
		stat |= 0x04;
	}
	if (read) {
		stat |= 0x10;
	}

	// バスエラー発行
	::s68000buserr(addr, stat);
}

//---------------------------------------------------------------------------
//
//	アドレスエラー
//	※DMA転送によるアドレスエラーもここに来る
//	※CPUコア内部でアドレスエラーと判定した場合は、ここを経由しない
//
//---------------------------------------------------------------------------
void FASTCALL CPU::AddrErr(DWORD addr, BOOL read)
{
	DWORD pc;
	DWORD stat;

	ASSERT(this);
	ASSERT(addr <= 0xffffff);
	ASSERT(addr & 1);

	// DMACに転送中か聞く。DMAC中ならDMACに任せる
	if (dmac->IsDMA()) {
		dmac->AddrErr(addr, read);
		return;
	}

	// アドレスが前回のアドレス+2で、かつ時間が同じなら無視する(LONGアクセス)
	if (addr == (sub.erraddr + 2)) {
		if (scheduler->GetTotalTime() == sub.errtime) {
			return;
		}
	}

	// アドレスと時間を更新
	sub.erraddr = addr;
	sub.errtime = scheduler->GetTotalTime();

	// PC取得(該当命令のオペコードに位置する)
	pc = GetPC();

	// 読み出し(Word)
	stat = memory->ReadOnly(pc);
	stat <<= 8;
	stat |= memory->ReadOnly(pc + 1);
	stat <<= 16;

	// ファンクションコード作成(常にデータアクセスとみなす)
	stat |= 0x8009;
	if (::s68000context.sr & 0x2000) {
		stat |= 0x04;
	}
	if (read) {
		stat |= 0x10;
	}

	// バスエラー発行(内部でアドレスエラーへ分岐)
	::s68000buserr(addr, stat);
}

//---------------------------------------------------------------------------
//
//	バスエラー記録
//	※CPUコア内部でバスエラーと判定した場合も、ここを通る
//
//---------------------------------------------------------------------------
void FASTCALL CPU::BusErrLog(DWORD addr, DWORD stat)
{
	ASSERT(this);

	// 必ずマスク(24bitを超える場合がある)
	addr &= 0xffffff;

	if (stat & 0x10) {
		LOG1(Log::Warning, "バスエラー(読み込み) $%06X", addr);
	}
	else {
		LOG1(Log::Warning, "バスエラー(書き込み) $%06X", addr);
	}
}

//---------------------------------------------------------------------------
//
//	アドレスエラー記録
//	※CPUコア内部でアドレスエラーと判定した場合も、ここを通る
//
//---------------------------------------------------------------------------
void FASTCALL CPU::AddrErrLog(DWORD addr, DWORD stat)
{
	ASSERT(this);

	// 必ずマスク(24bitを超える場合がある)
	addr &= 0xffffff;

	if (stat & 0x10) {
		LOG1(Log::Warning, "アドレスエラー(読み込み) $%06X", addr);
	}
	else {
		LOG1(Log::Warning, "アドレスエラー(書き込み) $%06X", addr);
	}
}
