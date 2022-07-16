//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 �o�h�D(ytanaka@ipc-tokai.or.jp)
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

	// CPU�擾
	ASSERT(!cpu);
	cpu = (CPU*)vm->SearchDevice(MAKEID('C', 'P', 'U', ' '));
	ASSERT(cpu);

	// DMAC�擾
	ASSERT(!dmac);
	dmac = (DMAC*)vm->SearchDevice(MAKEID('D', 'M', 'A', 'C'));
	ASSERT(dmac);

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	�N���[���A�b�v
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

	LOG0(Log::Normal, "���Z�b�g");

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

	LOG0(Log::Normal, "�Z�[�u");

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

	LOG0(Log::Normal, "���[�h");

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

	// �X�P�W���[�����̂����[�h
	if (!fio->Read(&sch, (int)sz)) {
		return FALSE;
	}

	// �T�C�N���e�[�u�������[�h
	if (!fio->Read(CycleTable, sizeof(CycleTable))) {
		return FALSE;
	}

	// �C�x���g�|�C���^�𕜋A
	sch.first = first;

	// �o�[�W����2.01�ȏ�Ȃ�Adma_active�����[�h
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

	LOG0(Log::Normal, "�ݒ�K�p");

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
				// �ʏ�
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
				LOG0(Log::Normal, "�u���[�N");
#endif	// SCHEDULER_LOG
				sch.brk = FALSE;
				return (DWORD)(sch.one | 0x80000000);
			}
			else {
				// Error de ejecucion
				result &= 0x7fffffff;

				if ((int)result > cycle) {
					// sch.time�Asch.cycle���X�V
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
					LOG0(Log::Normal, "�u���[�N");
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
					LOG0(Log::Normal, "�u���[�N");
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
					LOG0(Log::Normal, "�u���[�N");
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

		// sch.time���X�V
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
		// �ʏ�
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
		// sch.time, sch.cycle���X�V
		sch.cycle += result;
		sch.time += sch.one;

		// ���Ԃ�i�߂�
		ExecEvent(sch.one);

		while (sch.time >= 200) {
			sch.time -= 200;
			sch.cycle -= sch.speed;
		}
		// ���s�A�C�x���g����
		return 1;
	}

	// Todavia no es suficiente, aun hay tiempo antes del evento
	// sch.cycle���X�V
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

	LOG2(Log::Detail, "CPU���x�ݒ� %d.%02dMHz", speed / 100, (speed % 100));

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
	LOG2(Log::Normal, "�u���[�N�|�C���g�ݒ� $%06X enable=%d", addr, enable);
#endif	// SCHEDULER_LOG

	flag = FALSE;

	// ��v�`�F�b�N
	for (i=0; i<BreakMax; i++) {
		if (breakp[i].use) {
			if (breakp[i].addr == addr) {
				// �t���O�ύX�̂�
				breakp[i].enable = enable;
				flag = TRUE;
				break;
			}
		}
	}

	if (!flag) {
		// �󂫃T�[�`
		for (i=0; i<BreakMax; i++) {
			if (!breakp[i].use) {
				// �Z�b�g
				breakp[i].use = TRUE;
				breakp[i].addr = addr;
				breakp[i].enable = enable;
				breakp[i].time = 0;
				breakp[i].count = 0;
				break;
			}
		}
	}

	// �L���t���O��ݒ�
	flag = FALSE;
	for (i=0; i<BreakMax; i++) {
		if (breakp[i].use) {
			if (breakp[i].enable) {
				// �L���ȃu���[�N�|�C���g������
				flag = TRUE;
				break;
			}
		}
	}
	sch.check = flag;
}

//---------------------------------------------------------------------------
//
//	�u���[�N�|�C���g�폜
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
	LOG1(Log::Normal, "�u���[�N�|�C���g�폜 $%06X", addr);
#endif	// SCHEDULER_LOG

	// ��v�`�F�b�N
	for (i=0; i<BreakMax; i++) {
		if (breakp[i].use) {
			if (breakp[i].addr == addr) {
				// �폜
				breakp[i].use = FALSE;
				break;
			}
		}
	}

	// �L���t���O��ݒ�
	flag = FALSE;
	for (i=0; i<BreakMax; i++) {
		if (breakp[i].use) {
			if (breakp[i].enable) {
				// �L���ȃu���[�N�|�C���g������
				flag = TRUE;
				break;
			}
		}
	}
	sch.check = flag;
}

//---------------------------------------------------------------------------
//
//	�u���[�N�|�C���g�擾
//
//---------------------------------------------------------------------------
void FASTCALL Scheduler::GetBreak(int index, breakpoint_t *buf) const
{
	ASSERT(this);
	ASSERT((index >= 0) && (index < BreakMax));
	ASSERT(buf);
	ASSERT_DIAG();

	// �R�s�[
	*buf = breakp[index];
}

//---------------------------------------------------------------------------
//
//	�u���[�N�|�C���g�L���E����
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
//	�u���[�N�񐔃N���A
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
//	�u���[�N�A�h���X�ύX
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
//	�u���[�N�A�h���X�`�F�b�N
//
//---------------------------------------------------------------------------
int FASTCALL Scheduler::IsBreak(DWORD addr, BOOL any) const
{
	int i;

	ASSERT(this);
	ASSERT(addr <= 0xffffff);
	ASSERT_DIAG();

	// �ŏ��Ƀt���O������
	if (!sch.check) {
		return -1;
	}

	// ��v�`�F�b�N
	for (i=0; i<BreakMax; i++) {
		if (breakp[i].use) {
			if (breakp[i].addr == addr) {
				// �L���E�������C�ɂ��Ȃ����A�L��
				if (any || breakp[i].enable) {
					return i;
				}
			}
		}
	}

	// �u���[�N�|�C���g�͂��邪�A��v����
	return -1;
}

//---------------------------------------------------------------------------
//
//	�u���[�N�A�h���X�K�p
//
//---------------------------------------------------------------------------
void FASTCALL Scheduler::OnBreak(DWORD addr)
{
	int i;

	ASSERT(this);
	ASSERT(addr <= 0xffffff);
	ASSERT(sch.check);
	ASSERT_DIAG();

	// ��v�`�F�b�N
	for (i=0; i<BreakMax; i++) {
		if (breakp[i].use) {
			if (breakp[i].addr == addr) {
				break;
			}
		}
	}
	ASSERT(i < BreakMax);

	// ���ԃZ�b�g�A�J�E���g�A�b�v
	breakp[i].time = GetTotalTime();
	breakp[i].count++;
}

//---------------------------------------------------------------------------
//
//	�C�x���g�ǉ�
//
//---------------------------------------------------------------------------
void FASTCALL Scheduler::AddEvent(Event *event)
{
	Event *p;

	ASSERT(this);
	ASSERT(event);
	ASSERT_DIAG();

#if defined(SCHEDULER_LOG)
	LOG4(Log::Normal, "�C�x���g�ǉ� Device=%c%c%c%c",
					(char)(event->GetDevice()->GetID() >> 24),
					(char)(event->GetDevice()->GetID() >> 16),
					(char)(event->GetDevice()->GetID() >> 8),
					(char)(event->GetDevice()->GetID()));
	LOG1(Log::Normal, "�C�x���g�ǉ� %s", event->GetDesc());
#endif	// SCHEDULER_LOG

	// �ŏ��̃C�x���g��
	if (!sch.first) {
		// �ŏ��̃C�x���g
		sch.first = event;
		event->SetNextEvent(NULL);

#if defined(SCHEDULER_ASM)
		// �ʒm
		NotifyEvent(sch.first);
#endif	// SCHEDULER_ASM
		return;
	}

	// �Ō�̃C�x���g��T��
	p = sch.first;
	while (p->GetNextEvent()) {
		p = p->GetNextEvent();
	}

	// p���Ō�̃C�x���g�Ȃ̂ŁA����ɒǉ�
	p->SetNextEvent(event);
	event->SetNextEvent(NULL);

#if defined(SCHEDULER_ASM)
	// �ʒm
	NotifyEvent(sch.first);
#endif	// SCHEDULER_ASM
}

//---------------------------------------------------------------------------
//
//	�C�x���g�폜
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
	LOG4(Log::Normal, "�C�x���g�폜 Device=%c%c%c%c",
					(char)(event->GetDevice()->GetID() >> 24),
					(char)(event->GetDevice()->GetID() >> 16),
					(char)(event->GetDevice()->GetID() >> 8),
					(char)(event->GetDevice()->GetID()));
	LOG1(Log::Normal, "�C�x���g�폜 %s", event->GetDesc());
#endif	// SCHEDULER_LOG

	// �ŏ��̃C�x���g��
	if (sch.first == event) {
		// �ŏ��̃C�x���g�Bnext���ŏ��̃C�x���g�Ɋ��蓖�Ă�
		sch.first = event->GetNextEvent();
		event->SetNextEvent(NULL);

#if defined(SCHEDULER_ASM)
		// �ʒm
		NotifyEvent(sch.first);
#endif	// SCHEDULER_ASM
		return;
	}

	// ���̃C�x���g����v����܂Ō���
	p = sch.first;
	prev = p;
	while (p) {
		// ��v�`�F�b�N
		if (p == event) {
			prev->SetNextEvent(event->GetNextEvent());
			event->SetNextEvent(NULL);

#if defined(SCHEDULER_ASM)
			// �ʒm
			NotifyEvent(sch.first);
#endif	// SCHEDULER_ASM
			return;
		}

		// ����
		prev = p;
		p = p->GetNextEvent();
	}

	// ���ׂẴC�x���g����v���Ȃ�(���蓾�Ȃ�)
	ASSERT(FALSE);
}

//---------------------------------------------------------------------------
//
//	�C�x���g���L�`�F�b�N
//
//---------------------------------------------------------------------------
BOOL FASTCALL Scheduler::HasEvent(Event *event) const
{
	Event *p;

	ASSERT(this);
	ASSERT(event);
	ASSERT_DIAG();

	// ������
	p = sch.first;

	// �S�ẴC�x���g���܂��
	while (p) {
		// ��v�`�F�b�N
		if (p == event) {
			return TRUE;
		}

		// ����
		p = p->GetNextEvent();
	}

	// ���̃C�x���g�̓`�F�C���Ɋ܂܂�Ă��Ȃ�
	return FALSE;
}

//---------------------------------------------------------------------------
//
//	�C�x���g�̌����擾
//
//---------------------------------------------------------------------------
int FASTCALL Scheduler::GetEventNum() const
{
	int num;
	Event *p;

	ASSERT(this);
	ASSERT_DIAG();

	// ������
	num = 0;
	p = sch.first;

	// �S�ẴC�x���g���܂��
	while (p) {
		num++;

		// ����
		p = p->GetNextEvent();
	}

	// �C�x���g�̌���Ԃ�
	return num;
}

//---------------------------------------------------------------------------
//
//	�ŒZ�̃C�x���g��T��
//	���ʓr�A�Z���u���ł�p��
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

	// �C�x���g�|�C���^������
	p = sch.first;

	// ������
	minimum = hus;

	// ���[�v
	while (p) {
		// �c�莞�Ԏ擾
		remain = p->GetRemain();

		// �L����
		if (remain == 0) {
			// ����
			p = p->GetNextEvent();
			continue;
		}

		// �ŏ��`�F�b�N
		if (remain >= minimum) {
			p = p->GetNextEvent();
			continue;
		}

		// �ŏ�
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
//	�T�C�N���e�[�u��
//
//---------------------------------------------------------------------------
int Scheduler::CycleTable[0x1000];
