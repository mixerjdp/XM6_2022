//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 ＰＩ．(ytanaka@ipc-tokai.or.jp)
//	[ Programador ]
//
//---------------------------------------------------------------------------

#include "os.h"
#include "xm6.h"
#include "schedule.h"
#include "vm.h"
#include "log.h"
#include "cpu.h"
#include "event.h"
#include "dmac.h"
#include "core_asm.h"
#include "config.h"
#include "fileio.h"

//===========================================================================
//
//	Programador
//
//===========================================================================
//#define SCHEDULER_LOG

//---------------------------------------------------------------------------
//
//	Busqueda y actualizacion de eventos ensamblados
//
//---------------------------------------------------------------------------
#if defined(_MSC_VER) && defined(_M_IX86)
#define SCHEDULER_ASM
#endif	// _MSC_VER

//---------------------------------------------------------------------------
//
//	Constructor
//
//---------------------------------------------------------------------------
Scheduler::Scheduler(VM *p) : Device(p)
{
	int i;

	// Inicializar el ID del dispositivo
	dev.id = MAKEID('S', 'C', 'H', 'E');
	dev.desc = "Scheduler";

	// Puntos de ruptura individuales
	for (i=0; i<BreakMax; i++) {
		breakp[i].use = FALSE;
		breakp[i].addr = 0;
		breakp[i].enable = FALSE;
		breakp[i].time = 0;
		breakp[i].count = 0;
	}

	// Tiempo
	sch.total = 0;
	sch.one = 0;
	sch.sound = 0;

	// CPU
	sch.clock = 0;
	sch.speed = 979;
	sch.cycle = 0;
	sch.time = 0;

	// Puntos de interrupcion
	sch.brk = FALSE;
	sch.check = FALSE;

	// Eventos
	sch.first = NULL;
	sch.exec = FALSE;

	// Dispositivos
	cpu = NULL;
	dmac = NULL;

	// Otros
	dma_active = FALSE;
}

//---------------------------------------------------------------------------
//
//	Inicializacion
//
//---------------------------------------------------------------------------
BOOL FASTCALL Scheduler::Init()
{
	ASSERT(this);

	// Clase basica
	if (!Device::Init()) {
		return FALSE;
	}

	// CPU取得
	ASSERT(!cpu);
	cpu = (CPU*)vm->SearchDevice(MAKEID('C', 'P', 'U', ' '));
	ASSERT(cpu);

	// DMAC取得
	ASSERT(!dmac);
	dmac = (DMAC*)vm->SearchDevice(MAKEID('D', 'M', 'A', 'C'));
	ASSERT(dmac);

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	クリーンアップ
//
//---------------------------------------------------------------------------
void FASTCALL Scheduler::Cleanup()
{
	ASSERT(this);
	ASSERT_DIAG();

	// A la clase basica
	Device::Cleanup();
}

//---------------------------------------------------------------------------
//
//	Reiniciar
//
//---------------------------------------------------------------------------
void FASTCALL Scheduler::Reset()
{
	ASSERT(this);
	ASSERT_DIAG();

	LOG0(Log::Normal, "リセット");

	// Restablecimiento del tiempo (excepto el sonido)
	sch.total = 0;
	sch.one = 0;

	// Restablecimiento del ciclo de la CPU
	sch.cycle = 0;
	sch.time = 0;

	// Evento que no se ejecuta
	sch.exec = FALSE;

	// No hay ejecucion de DMA
	dma_active = FALSE;

	// El ajuste de la velocidad de la CPU se realiza cada vez (para la rutina de contramedidas INFO.RAM)
	ASSERT((sch.clock >= 0) && (sch.clock <= 5));
	SetCPUSpeed(ClockTable[sch.clock]);
}

//---------------------------------------------------------------------------
//
//	Ahorra
//
//---------------------------------------------------------------------------
BOOL FASTCALL Scheduler::Save(Fileio *fio, int /*ver*/)
{
	size_t sz;

	ASSERT(this);
	ASSERT(fio);
	ASSERT_DIAG();

	LOG0(Log::Normal, "セーブ");

	// Guardar el tamano del punto de interrupcion
	sz = sizeof(breakp);
	if (!fio->Write(&sz, sizeof(sz))) {
		return FALSE;
	}

	// Guardar la entidad del punto de interrupcion
	if (!fio->Write(breakp, (int)sz)) {
		return FALSE;
	}

	// Guardar el tamano del programador
	sz = sizeof(scheduler_t);
	if (!fio->Write(&sz, sizeof(sz))) {
		return FALSE;
	}

	// Guardar la entidad programadora
	if (!fio->Write(&sch, (int)sz)) {
		return FALSE;
	}

	// Guardar tabla de ciclos
	if (!fio->Write(CycleTable, sizeof(CycleTable))) {
		return FALSE;
	}

	// Guardar dma_active (version 2.01)
	if (!fio->Write(&dma_active, sizeof(dma_active))) {
		return FALSE;
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	Cargar
//
//---------------------------------------------------------------------------
BOOL FASTCALL Scheduler::Load(Fileio *fio, int ver)
{
	size_t sz;
	Event *first;

	ASSERT(this);
	ASSERT(fio);
	ASSERT(ver >= 0x200);
	ASSERT_DIAG();

	LOG0(Log::Normal, "ロード");

	// Mantiene el puntero del evento
	first = sch.first;

	// Cargar y cotejar los tamanos de los puntos de rotura
	if (!fio->Read(&sz, sizeof(sz))) {
		return FALSE;
	}
	if (sz != sizeof(breakp)) {
		return FALSE;
	}

	// Carga de las entidades de punto de ruptura
	if (!fio->Read(breakp, (int)sz)) {
		return FALSE;
	}

	// Cargar y cotejar los tamanos de los programadores
	if (!fio->Read(&sz, sizeof(sz))) {
		return FALSE;
	}
	if (sz != sizeof(scheduler_t)) {
		return FALSE;
	}

	// スケジューラ実体をロード
	if (!fio->Read(&sch, (int)sz)) {
		return FALSE;
	}

	// サイクルテーブルをロード
	if (!fio->Read(CycleTable, sizeof(CycleTable))) {
		return FALSE;
	}

	// イベントポインタを復帰
	sch.first = first;

	// バージョン2.01以上なら、dma_activeをロード
	if (ver >= 0x0201) {
		if (!fio->Read(&dma_active, sizeof(dma_active))) {
			return FALSE;
		}
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	Aplicar ajustes
//
//---------------------------------------------------------------------------
void FASTCALL Scheduler::ApplyCfg(const Config *config)
{
	ASSERT(this);
	ASSERT(config);
	ASSERT_DIAG();

	LOG0(Log::Normal, "設定適用");

	// Comparar la configuracion del reloj del sistema
	if (sch.clock != config->system_clock) {
		// Reconstruccion de la tabla de ciclos debido a diferentes ajustes
		sch.clock = config->system_clock;
		ASSERT((sch.clock >= 0) && (sch.clock <= 5));
		SetCPUSpeed(ClockTable[sch.clock]);
	}
}

#if defined(_DEBUG)
//---------------------------------------------------------------------------
//
//	Diagnostico
//
//---------------------------------------------------------------------------
void FASTCALL Scheduler::AssertDiag() const
{
	ASSERT(this);
	ASSERT(GetID() == MAKEID('S', 'C', 'H', 'E'));
	ASSERT(cpu);
	ASSERT(cpu->GetID() == MAKEID('C', 'P', 'U', ' '));
	ASSERT(dmac);
	ASSERT(dmac->GetID() == MAKEID('D', 'M', 'A', 'C'));
}
#endif	// _DEBUG

//---------------------------------------------------------------------------
//
//	Adquisicion de datos internos
//
//---------------------------------------------------------------------------
void FASTCALL Scheduler::GetScheduler(scheduler_t *buffer) const
{
	ASSERT(this);
	ASSERT(buffer);
	ASSERT_DIAG();

	// Copiar datos internos
	*buffer = sch;
}

//---------------------------------------------------------------------------
//
//	Ejecucion
//
//---------------------------------------------------------------------------
DWORD FASTCALL Scheduler::Exec(DWORD hus)
{
	int cycle;
	DWORD result;
	DWORD dcycle;

	ASSERT(this);
	ASSERT(hus > 0);
	ASSERT_DIAG();

	// Sin puntos de interrupcion
	if (!sch.check) {
		// Encuentre el evento mas corto
#if defined(SCHEDULER_ASM)
		sch.one = GetMinEvent(hus);
#else
		sch.one = GetMinRemain(hus);
#endif	// SCHEDULER_ASM

		// ?Ha realizado ya tantos ciclos como   sch.one + sch.time?
		ASSERT((sch.one + sch.time) < 0x1000);
		cycle = CycleTable[sch.one + sch.time];
		if (cycle > sch.cycle) {

			// Averigue cuantos ciclos puede ejecutar esta vez, y ejecute
			cycle -= sch.cycle;
			if (!dma_active) {
				// 通常
				result = cpu->Exec(cycle);
			}
			else {
				// Peticion automatica de DMAC habilitada
				dcycle = dmac->AutoDMA(cycle);
				if (dcycle != 0) {
					// ?Hay un ligero margen de error?
					result = cpu->Exec(dcycle);
				}
				else {
					// Todo consumido por DMA
					result = cycle;
				}
			}

			// ?Terminacion normal?
			if (result < 0x80000000) {
				// Actualizar sch.time y sch.cycle
				sch.cycle += result;
				sch.time += sch.one;

				// Avanzar en el tiempo
				ExecEvent(sch.one);

				if (sch.time < 200) {
					return sch.one;
				}

				// Sincronizacion de tiempo
				while (sch.time >= 200) {
					if ((DWORD)sch.cycle < sch.speed) {
						break;
					}
					sch.time -= 200;
					sch.cycle -= sch.speed;
				}

				// Comprobacion de la pausa
				if (!sch.brk) {
					return sch.one;
				}

#if defined(SCHEDULER_LOG)
				LOG0(Log::Normal, "ブレーク");
#endif	// SCHEDULER_LOG
				sch.brk = FALSE;
				return (DWORD)(sch.one | 0x80000000);
			}
			else {
				// Error de ejecucion
				result &= 0x7fffffff;

				if ((int)result > cycle) {
					// sch.time、sch.cycleを更新
					sch.time += sch.one;
					sch.cycle += result;

					// Ejecucion del evento
					ExecEvent(sch.one);

					while (sch.time >= 200) {
						if ((DWORD)sch.cycle < sch.speed) {
							break;
						}
						sch.time -= 200;
						sch.cycle -= sch.speed;
					}
					// Error de ejecucion, evento completado
					return 0x80000000;
				}
				// Me dio un error de cpu antes de poder ejecutar todo.
				sch.cycle += result;
				// Error de ejecucion, evento no completado
				return 0x80000000;
			}
		}
		else {

			// Esta vez no es viable. Solo el tiempo lo dira.
			sch.time += sch.one;
			ExecEvent(sch.one);

			if (sch.time < 200) {
				return sch.one;
			}

			// Actualizar la hora de la escuela
			while (sch.time >= 200) {
				if ((DWORD)sch.cycle < sch.speed) {
					break;
				}
				sch.time -= 200;
				sch.cycle -= sch.speed;
			}

			// Sin instruccion de ejecucion, evento completo
			return sch.one;
		}

	}

	// Bucle
	for (;;) {
		result = Trace(hus);

		switch (result) {
			// Sin instruccion de ejecucion, evento completo
			case 0:
				return sch.one;

			// Listo para correr, evento completo
			case 1:
				if (sch.brk) {
#if defined(SCHEDULER_LOG)
					LOG0(Log::Normal, "ブレーク");
#endif	// SCHEDULER_LOG
					sch.brk = FALSE;
					return 0x80000000;
				}
				if (IsBreak(cpu->GetPC()) != -1) {
					OnBreak(cpu->GetPC());
					return 0x80000000;
				}
				return sch.one;

			// Ejecutar, evento no completado
			case 2:
				if (sch.brk) {
#if defined(SCHEDULER_LOG)
					LOG0(Log::Normal, "ブレーク");
#endif	// SCHEDULER_LOG
					sch.brk = FALSE;
					return 0x80000000;
				}
				if (IsBreak(cpu->GetPC()) != -1) {
					OnBreak(cpu->GetPC());
					return 0x80000000;
				}
				break;

			// Error de ejecucion
			case 3:
				if (sch.brk) {
#if defined(SCHEDULER_LOG)
					LOG0(Log::Normal, "ブレーク");
#endif	// SCHEDULER_LOG
					sch.brk = FALSE;
				}
				return 0x80000000;

			// Aparte de eso
			default:
				ASSERT(FALSE);
				return sch.one;
		}
	}
}

//---------------------------------------------------------------------------
//
//	Rastreando
//
//---------------------------------------------------------------------------
DWORD FASTCALL Scheduler::Trace(DWORD hus)
{
	int cycle;
	DWORD result;

	ASSERT(this);
	ASSERT(hus > 0);
	ASSERT_DIAG();

	// Encuentre el evento mas corto
#if defined(SCHEDULER_ASM)
	sch.one = GetMinEvent(hus);
#else
	sch.one = GetMinRemain(hus);
#endif	// SCHEDULER_ASM

	// ?Ha realizado ya tantos ciclos como sch.one + sch.time?
	ASSERT((sch.one + sch.time) < 0x1000);
	cycle = CycleTable[sch.one + sch.time];
	if (cycle <= sch.cycle) {
		// Esta vez no es viable. Solo el tiempo lo dira.
		sch.time += sch.one;
		ExecEvent(sch.one);

		// sch.timeを更新
		while (sch.time >= 200) {
			sch.time -= 200;
			sch.cycle -= sch.speed;
		}
		// Sin instruccion de ejecucion, evento completo
		return 0;
	}

	// Averigua cuantos ciclos puedes realizar esta vez
	cycle -= sch.cycle;

	// Dale un ciclo y trata de hacerlo funcionar.
	if (!dma_active) {
		// 通常
		result = cpu->Exec(1);
	}
	else {
		// Peticion automatica de DMAC habilitada
		result = dmac->AutoDMA(1);
		if (result != 0) {
			result = cpu->Exec(result);
		}
		else {
			result = 1;
		}
	}
	if (result >= 0x80000000) {
		// Error de ejecucion
		return 3;
	}

	// Si el resultado >= ciclo, el evento puede ser ejecutado
	if ((int)result >= cycle) {
		// sch.time, sch.cycleを更新
		sch.cycle += result;
		sch.time += sch.one;

		// 時間を進める
		ExecEvent(sch.one);

		while (sch.time >= 200) {
			sch.time -= 200;
			sch.cycle -= sch.speed;
		}
		// 実行可、イベント完了
		return 1;
	}

	// Todavia no es suficiente, aun hay tiempo antes del evento
	// sch.cycleを更新
	sch.cycle += result;

	// Ejecutar, evento no completado
	return 2;
}

//---------------------------------------------------------------------------
//
//	Ajustar la velocidad de la CPU
//
//---------------------------------------------------------------------------
void FASTCALL Scheduler::SetCPUSpeed(DWORD speed)
{
	int i;
	DWORD cycle;

	ASSERT(this);
	ASSERT(speed > 0);
	ASSERT_DIAG();

	LOG2(Log::Detail, "CPU速度設定 %d.%02dMHz", speed / 100, (speed % 100));

	// Memoria de la velocidad de la CPU
	sch.speed = speed;

	// Calculo del numero de ciclos correspondiente de 0 a 2048us en incrementos de 0,5us
	for (i=0; i<0x1000; i++) {
		cycle = (DWORD)i;
		cycle *= speed;
		cycle /= 200;
		CycleTable[i] = cycle;
	}
}

//---------------------------------------------------------------------------
//
//	Obtener el tiempo transcurrido
//
//---------------------------------------------------------------------------
DWORD FASTCALL Scheduler::GetPassedTime() const
{
	DWORD hus;

	ASSERT(this);
	ASSERT_DIAG();

	// 0 si el evento esta en marcha
	if (sch.exec) {
		return 0;
	}

	// Numero de ciclos de ejecucion, tiempo calculado a partir de cpu_cycle
	hus = cpu->GetCycle() + sch.cycle;
	hus *= 200;
	hus /= sch.speed;
	hus -= sch.time;

	// Si es mayor que one, el limite
	if (sch.one < hus) {
		hus = sch.one;
	}

	// Devoluciones en hus
	return hus;
}

//---------------------------------------------------------------------------
//
//	Ajuste del punto de interrupcion
//
//---------------------------------------------------------------------------
void FASTCALL Scheduler::SetBreak(DWORD addr, BOOL enable)
{
	int i;
	BOOL flag;

	ASSERT(this);
	ASSERT(addr <= 0xffffff);
	ASSERT_DIAG();

#if defined(SCHEDULER_LOG)
	LOG2(Log::Normal, "ブレークポイント設定 $%06X enable=%d", addr, enable);
#endif	// SCHEDULER_LOG

	flag = FALSE;

	// 一致チェック
	for (i=0; i<BreakMax; i++) {
		if (breakp[i].use) {
			if (breakp[i].addr == addr) {
				// フラグ変更のみ
				breakp[i].enable = enable;
				flag = TRUE;
				break;
			}
		}
	}

	if (!flag) {
		// 空きサーチ
		for (i=0; i<BreakMax; i++) {
			if (!breakp[i].use) {
				// セット
				breakp[i].use = TRUE;
				breakp[i].addr = addr;
				breakp[i].enable = enable;
				breakp[i].time = 0;
				breakp[i].count = 0;
				break;
			}
		}
	}

	// 有効フラグを設定
	flag = FALSE;
	for (i=0; i<BreakMax; i++) {
		if (breakp[i].use) {
			if (breakp[i].enable) {
				// 有効なブレークポイントが存在
				flag = TRUE;
				break;
			}
		}
	}
	sch.check = flag;
}

//---------------------------------------------------------------------------
//
//	ブレークポイント削除
//
//---------------------------------------------------------------------------
void FASTCALL Scheduler::DelBreak(DWORD addr)
{
	int i;
	BOOL flag;

	ASSERT(this);
	ASSERT(addr <= 0xffffff);
	ASSERT_DIAG();

#if defined(SCHEDULER_LOG)
	LOG1(Log::Normal, "ブレークポイント削除 $%06X", addr);
#endif	// SCHEDULER_LOG

	// 一致チェック
	for (i=0; i<BreakMax; i++) {
		if (breakp[i].use) {
			if (breakp[i].addr == addr) {
				// 削除
				breakp[i].use = FALSE;
				break;
			}
		}
	}

	// 有効フラグを設定
	flag = FALSE;
	for (i=0; i<BreakMax; i++) {
		if (breakp[i].use) {
			if (breakp[i].enable) {
				// 有効なブレークポイントが存在
				flag = TRUE;
				break;
			}
		}
	}
	sch.check = flag;
}

//---------------------------------------------------------------------------
//
//	ブレークポイント取得
//
//---------------------------------------------------------------------------
void FASTCALL Scheduler::GetBreak(int index, breakpoint_t *buf) const
{
	ASSERT(this);
	ASSERT((index >= 0) && (index < BreakMax));
	ASSERT(buf);
	ASSERT_DIAG();

	// コピー
	*buf = breakp[index];
}

//---------------------------------------------------------------------------
//
//	ブレークポイント有効・無効
//
//---------------------------------------------------------------------------
void FASTCALL Scheduler::EnableBreak(int index, BOOL enable)
{
	ASSERT(this);
	ASSERT((index >= 0) && (index < BreakMax));
	ASSERT(breakp[index].use);
	ASSERT_DIAG();

	breakp[index].enable = enable;
}

//---------------------------------------------------------------------------
//
//	ブレーク回数クリア
//
//---------------------------------------------------------------------------
void FASTCALL Scheduler::ClearBreak(int index)
{
	ASSERT(this);
	ASSERT((index >= 0) && (index < BreakMax));
	ASSERT(breakp[index].use);
	ASSERT_DIAG();

	breakp[index].count = 0;
	breakp[index].time = 0;
}

//---------------------------------------------------------------------------
//
//	ブレークアドレス変更
//
//---------------------------------------------------------------------------
void FASTCALL Scheduler::AddrBreak(int index, DWORD addr)
{
	ASSERT(this);
	ASSERT((index >= 0) && (index < BreakMax));
	ASSERT(addr <= 0xffffff);
	ASSERT(breakp[index].use);
	ASSERT_DIAG();

	breakp[index].addr = addr;
}

//---------------------------------------------------------------------------
//
//	ブレークアドレスチェック
//
//---------------------------------------------------------------------------
int FASTCALL Scheduler::IsBreak(DWORD addr, BOOL any) const
{
	int i;

	ASSERT(this);
	ASSERT(addr <= 0xffffff);
	ASSERT_DIAG();

	// 最初にフラグを見る
	if (!sch.check) {
		return -1;
	}

	// 一致チェック
	for (i=0; i<BreakMax; i++) {
		if (breakp[i].use) {
			if (breakp[i].addr == addr) {
				// 有効・無効を気にしないか、有効
				if (any || breakp[i].enable) {
					return i;
				}
			}
		}
	}

	// ブレークポイントはあるが、一致無し
	return -1;
}

//---------------------------------------------------------------------------
//
//	ブレークアドレス適用
//
//---------------------------------------------------------------------------
void FASTCALL Scheduler::OnBreak(DWORD addr)
{
	int i;

	ASSERT(this);
	ASSERT(addr <= 0xffffff);
	ASSERT(sch.check);
	ASSERT_DIAG();

	// 一致チェック
	for (i=0; i<BreakMax; i++) {
		if (breakp[i].use) {
			if (breakp[i].addr == addr) {
				break;
			}
		}
	}
	ASSERT(i < BreakMax);

	// 時間セット、カウントアップ
	breakp[i].time = GetTotalTime();
	breakp[i].count++;
}

//---------------------------------------------------------------------------
//
//	イベント追加
//
//---------------------------------------------------------------------------
void FASTCALL Scheduler::AddEvent(Event *event)
{
	Event *p;

	ASSERT(this);
	ASSERT(event);
	ASSERT_DIAG();

#if defined(SCHEDULER_LOG)
	LOG4(Log::Normal, "イベント追加 Device=%c%c%c%c",
					(char)(event->GetDevice()->GetID() >> 24),
					(char)(event->GetDevice()->GetID() >> 16),
					(char)(event->GetDevice()->GetID() >> 8),
					(char)(event->GetDevice()->GetID()));
	LOG1(Log::Normal, "イベント追加 %s", event->GetDesc());
#endif	// SCHEDULER_LOG

	// 最初のイベントか
	if (!sch.first) {
		// 最初のイベント
		sch.first = event;
		event->SetNextEvent(NULL);

#if defined(SCHEDULER_ASM)
		// 通知
		NotifyEvent(sch.first);
#endif	// SCHEDULER_ASM
		return;
	}

	// 最後のイベントを探す
	p = sch.first;
	while (p->GetNextEvent()) {
		p = p->GetNextEvent();
	}

	// pが最後のイベントなので、これに追加
	p->SetNextEvent(event);
	event->SetNextEvent(NULL);

#if defined(SCHEDULER_ASM)
	// 通知
	NotifyEvent(sch.first);
#endif	// SCHEDULER_ASM
}

//---------------------------------------------------------------------------
//
//	イベント削除
//
//---------------------------------------------------------------------------
void FASTCALL Scheduler::DelEvent(Event *event)
{
	Event *p;
	Event *prev;

	ASSERT(this);
	ASSERT(event);
	ASSERT_DIAG();

#if defined(SCHEDULER_LOG)
	LOG4(Log::Normal, "イベント削除 Device=%c%c%c%c",
					(char)(event->GetDevice()->GetID() >> 24),
					(char)(event->GetDevice()->GetID() >> 16),
					(char)(event->GetDevice()->GetID() >> 8),
					(char)(event->GetDevice()->GetID()));
	LOG1(Log::Normal, "イベント削除 %s", event->GetDesc());
#endif	// SCHEDULER_LOG

	// 最初のイベントか
	if (sch.first == event) {
		// 最初のイベント。nextを最初のイベントに割り当てる
		sch.first = event->GetNextEvent();
		event->SetNextEvent(NULL);

#if defined(SCHEDULER_ASM)
		// 通知
		NotifyEvent(sch.first);
#endif	// SCHEDULER_ASM
		return;
	}

	// このイベントが一致するまで検索
	p = sch.first;
	prev = p;
	while (p) {
		// 一致チェック
		if (p == event) {
			prev->SetNextEvent(event->GetNextEvent());
			event->SetNextEvent(NULL);

#if defined(SCHEDULER_ASM)
			// 通知
			NotifyEvent(sch.first);
#endif	// SCHEDULER_ASM
			return;
		}

		// 次へ
		prev = p;
		p = p->GetNextEvent();
	}

	// すべてのイベントが一致しない(あり得ない)
	ASSERT(FALSE);
}

//---------------------------------------------------------------------------
//
//	イベント所有チェック
//
//---------------------------------------------------------------------------
BOOL FASTCALL Scheduler::HasEvent(Event *event) const
{
	Event *p;

	ASSERT(this);
	ASSERT(event);
	ASSERT_DIAG();

	// 初期化
	p = sch.first;

	// 全てのイベントをまわる
	while (p) {
		// 一致チェック
		if (p == event) {
			return TRUE;
		}

		// 次へ
		p = p->GetNextEvent();
	}

	// このイベントはチェインに含まれていない
	return FALSE;
}

//---------------------------------------------------------------------------
//
//	イベントの個数を取得
//
//---------------------------------------------------------------------------
int FASTCALL Scheduler::GetEventNum() const
{
	int num;
	Event *p;

	ASSERT(this);
	ASSERT_DIAG();

	// 初期化
	num = 0;
	p = sch.first;

	// 全てのイベントをまわる
	while (p) {
		num++;

		// 次へ
		p = p->GetNextEvent();
	}

	// イベントの個数を返す
	return num;
}

//---------------------------------------------------------------------------
//
//	最短のイベントを探す
//	※別途アセンブラ版を用意
//
//---------------------------------------------------------------------------
DWORD FASTCALL Scheduler::GetMinRemain(DWORD hus)
{
	Event *p;
	DWORD minimum;
	DWORD remain;

	ASSERT(this);
	ASSERT(hus > 0);
	ASSERT_DIAG();

	// イベントポインタ初期化
	p = sch.first;

	// 初期化
	minimum = hus;

	// ループ
	while (p) {
		// 残り時間取得
		remain = p->GetRemain();

		// 有効か
		if (remain == 0) {
			// 次へ
			p = p->GetNextEvent();
			continue;
		}

		// 最小チェック
		if (remain >= minimum) {
			p = p->GetNextEvent();
			continue;
		}

		// 最小
		minimum = remain;
		p = p->GetNextEvent();
	}

	return minimum;
}

//---------------------------------------------------------------------------
//
//	Ejecucion del evento
//	Existe una version de ensamblador independiente.
//
//---------------------------------------------------------------------------
void FASTCALL Scheduler::ExecEvent(DWORD hus)
{
#if !defined(SCHEDULER_ASM)
	Event *p;
#endif	// !SCHEDULER_ASM

	ASSERT(this);
	ASSERT(hus >= 0);
	ASSERT_DIAG();

	// Inicio de la ejecucion del evento
	sch.exec = TRUE;

	// Aumento del tiempo total, aumento del tiempo de sonido
	sch.total += hus;
	sch.sound += hus;

#if defined(SCHEDULER_ASM)
	SubExecEvent(hus);
	sch.exec = FALSE;
#else

	// Inicializacion del puntero de eventos
	p = sch.first;

	// Eventos de giro y carrera
	while (p) {
		p->Exec(hus);
		p = p->GetNextEvent();
	}

	// Fin de la ejecucion del evento
	sch.exec = FALSE;
#endif
}

//---------------------------------------------------------------------------
//
//	Mesa del reloj
//
//---------------------------------------------------------------------------
const DWORD Scheduler::ClockTable[] = {
	979,			// 10MHz
	1171,			// 12MHz
	1460,			// 15MHz
	1556,			// 16MHz
	1689,			// 17.4MHz
	3000			// 30MHz
};

//---------------------------------------------------------------------------
//
//	サイクルテーブル
//
//---------------------------------------------------------------------------
int Scheduler::CycleTable[0x1000];
