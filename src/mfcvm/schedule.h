//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 �o�h�D(ytanaka@ipc-tokai.or.jp)
//	[ �X�P�W���[�� ]
//
//---------------------------------------------------------------------------

#if !defined(scheduler_h)
#define scheduler_h

#include "device.h"
#include "starcpu.h"

//---------------------------------------------------------------------------
//
//	Pesos de alta velocidad (solo para Starscream)
//
//---------------------------------------------------------------------------
#define SCHEDULER_FASTWAIT
#if defined(SCHEDULER_FASTWAIT)
extern "C" {
extern DWORD s68000iocycle;
										// __io_cycle_counter(Starscream)
}
#endif	// SCHEDULER_FASTWAIT

//===========================================================================
//
//	Programador
//
//===========================================================================
class Scheduler : public Device
{
public:
	// Definicion del punto de interrupcion
	typedef struct{
		BOOL use;						// Bandera de uso
		DWORD addr;						// Direcciones
		BOOL enable;					// Bandera de Habilitar
		DWORD time;						// Tiempo de parada
		DWORD count;					// Numero de paradas
	} breakpoint_t;

	// �X�P�W���[����`
	typedef struct {
		// ����
		DWORD total;					// Tiempo total de ejecucion
		DWORD one;						// Un tiempo de ejecucion
		DWORD sound;					// Tiempo de actualizacion del sonido

		// CPU
		int clock;						// Reloj de la CPU (0 a 5)
		DWORD speed;					// Velocidad de la CPU (determinada por el reloj)
		int cycle;						// Numero de ciclos de la CPU
		DWORD time;						// Tiempo de ajuste del ciclo de la CPU

		// Puntos de interrupcion
		BOOL brk;						// Romper.
		BOOL check;						// Puntos de interrupcion validos disponibles

		// �C�x���g
		Event *first;					// El primer evento
		BOOL exec;						// Evento de carrera
	} scheduler_t;

	// ����`
	enum {
		BreakMax = 8					// Numero total de puntos de interrupcion
	};

public:
	// ��{�t�@���N�V����
	Scheduler(VM *p);
										// Constructor
	BOOL FASTCALL Init();
										// Inicializacion
	void FASTCALL Cleanup();
										// Limpieza
	void FASTCALL Reset();
										// Reiniciar
	BOOL FASTCALL Save(Fileio *fio, int ver);
										// Guardar
	BOOL FASTCALL Load(Fileio *fio, int ver);
										// Cargar
	void FASTCALL ApplyCfg(const Config *config);
										// Aplicar ajustes
#if defined(_DEBUG)
	void FASTCALL AssertDiag() const;
										// �f�f
#endif	// _DEBUG

	// �O��API
	void FASTCALL GetScheduler(scheduler_t *buffer) const;
										// Adquisicion de datos internos
	DWORD FASTCALL Exec(DWORD hus);
										// Ejecucion
	DWORD FASTCALL Trace(DWORD hus);
										// Rastreando
	void FASTCALL Break()				{ sch.brk = TRUE; }
										// Detener la ejecucion
#ifdef SCHEDULER_FASTWAIT
	void FASTCALL Wait(DWORD cycle)		{ sch.cycle += cycle; if (::s68000iocycle != (DWORD)-1) ::s68000iocycle -= cycle; }
										// Espera de la CPU (todo en linea)
#else
	void FASTCALL Wait(DWORD cycle)		{ ::s68000wait(cycle); sch.cycle += cycle; }
										// Espera de la CPU
#endif	// SCHEDULER_FASTWAIT

	// ���ꑀ��(DMAC����)
	int FASTCALL GetCPUCycle() const	{ return sch.cycle; }
										// Obtener el numero de ciclo
	void FASTCALL SetCPUCycle(int cycle) { sch.cycle = cycle; }
										// Ajuste del numero de ciclo

	// ���ԏ��
	DWORD FASTCALL GetTotalTime() const	{ return (GetPassedTime() + sch.total); }
										// Obtener el tiempo total de ejecucion
	DWORD FASTCALL GetOneTime() const	{ return sch.one; }
										// Obtener el tiempo de ejecucion del micro
	DWORD FASTCALL GetPassedTime() const;
										// Obtener el tiempo transcurrido
	DWORD FASTCALL GetCPUSpeed() const	{ return sch.speed; }
										// Adquisicion de la velocidad de la CPU
	void FASTCALL SetCPUSpeed(DWORD speed);
										// Ajuste de la velocidad de la CPU
	DWORD FASTCALL GetSoundTime() const	{ return sch.sound; }
										// Obtener el tiempo de sonido
	void FASTCALL SetSoundTime(DWORD hus) { sch.sound = hus; }
										// Establecer la duracion del sonido

	// �u���[�N�|�C���g
	void FASTCALL SetBreak(DWORD addr, BOOL enable = TRUE);
										// �u���[�N�|�C���g�ݒ�
	void FASTCALL DelBreak(DWORD addr);
										// �u���[�N�|�C���g�폜
	void FASTCALL GetBreak(int index, breakpoint_t *buf) const;
										// �u���[�N�|�C���g�擾
	void FASTCALL EnableBreak(int index, BOOL enable = TRUE);
										// �u���[�N�|�C���g�L���E����
	void FASTCALL ClearBreak(int index);
										// �u���[�N�񐔃N���A
	void FASTCALL AddrBreak(int index, DWORD addr);
										// �u���[�N�A�h���X�ύX
	int FASTCALL IsBreak(DWORD addr, BOOL any = FALSE) const;
										// �u���[�N�A�h���X�`�F�b�N

	// �C�x���g
	void FASTCALL AddEvent(Event *event);
										// �C�x���g�ǉ�
	void FASTCALL DelEvent(Event *event);
										// �C�x���g�폜
	BOOL FASTCALL HasEvent(Event *event) const;
										// �C�x���g���L�`�F�b�N
	Event* FASTCALL GetFirstEvent()	const { return sch.first; }
										// �ŏ��̃C�x���g���擾
	int FASTCALL GetEventNum() const;
										// �C�x���g�̌����擾

	// �O������t���O
	BOOL dma_active;
										// Peticion automatica de DMAC habilitada

private:
	DWORD FASTCALL GetMinRemain(DWORD hus);
										// Obtenga el evento mas corto posible
	void FASTCALL ExecEvent(DWORD hus);
										// Ejecucion del evento
	void FASTCALL OnBreak(DWORD addr);
										// Aplicacion del punto de interrupcion

	// �����f�[�^
	breakpoint_t breakp[BreakMax];
										// Puntos de interrupcion
	scheduler_t sch;
										// Programador

	// �f�o�C�X
	CPU *cpu;
										// CPU
	DMAC *dmac;
										// DMAC

	// �e�[�u��
	static const DWORD ClockTable[];
										// Mesa del reloj
	static int CycleTable[0x1000];
										// Tiempo (hus) �� numero de ciclos
};

#endif	// scheduler_h
