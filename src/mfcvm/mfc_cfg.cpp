//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 �o�h�D(ytanaka@ipc-tokai.or.jp)
//	[ MFC �R���t�B�O ]
//
//---------------------------------------------------------------------------

#if defined(_WIN32)

#include "os.h"
#include "xm6.h"
#include "vm.h"
#include "memory.h"
#include "opmif.h"
#include "adpcm.h"
#include "config.h"
#include "render.h"
#include "sasi.h"
#include "scsi.h"
#include "disk.h"
#include "filepath.h"
#include "ppi.h"
#include "mfc_frm.h"
#include "mfc_com.h"
#include "mfc_res.h"
#include "mfc_snd.h"
#include "mfc_inp.h"
#include "mfc_midi.h"
#include "mfc_tkey.h"
#include "mfc_w32.h"
#include "mfc_info.h"
#include "mfc_cfg.h"

//===========================================================================
//
//	Configuracion
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	Constructor
//
//---------------------------------------------------------------------------
CConfig::CConfig(CFrmWnd *pWnd) : CComponent(pWnd)
{
	// Parametros de los componentes
	m_dwID = MAKEID('C', 'F', 'G', ' ');
	m_strDesc = _T("Config Manager");
}

//---------------------------------------------------------------------------
//
//	Inicializacion
//
//---------------------------------------------------------------------------
BOOL FASTCALL CConfig::Init()
{
	int i;
	Filepath path;

	ASSERT(this);

	// Clase basica
	if (!CComponent::Init()) {
		return FALSE;
	}

	// Determinacion de la ruta del archivo INI
	path.SetPath(_T("XM6.ini"));
	path.SetBaseFile();
	_tcscpy(m_IniFile, path.GetPath());

	// Datos de configuracion
	LoadConfig();

	// Mantener la compatibilidad
	ResetSASI();
	ResetCDROM();

	// MRU
	for (i=0; i<MruTypes; i++) {
		ClearMRU(i);
		LoadMRU(i);
	}

	// Clave
	LoadKey();

	// TrueKey
	LoadTKey();

	// Guardar y cargar
	m_bApply = FALSE;

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	Limpieza
//
//---------------------------------------------------------------------------
void FASTCALL CConfig::Cleanup()
{
	int i;

	ASSERT(this);

	// �ݒ�f�[�^
	SaveConfig();

	// MRU
	for (i=0; i<MruTypes; i++) {
		SaveMRU(i);
	}

	// �L�[
	SaveKey();

	// TrueKey
	SaveTKey();

	// ��{�N���X
	CComponent::Cleanup();
}

//---------------------------------------------------------------------------
//
//	Entidad de datos de configuracion
//
//---------------------------------------------------------------------------
Config CConfig::m_Config;

//---------------------------------------------------------------------------
//
//	Adquisicion de datos de configuracion
//
//---------------------------------------------------------------------------
void FASTCALL CConfig::GetConfig(Config *pConfigBuf) const
{
	ASSERT(this);
	ASSERT(pConfigBuf);

	// �������[�N���R�s�[
	*pConfigBuf = m_Config;
}

//---------------------------------------------------------------------------
//
//	Establecer la configuracion de los datos
//
//---------------------------------------------------------------------------
void FASTCALL CConfig::SetConfig(Config *pConfigBuf)
{
	ASSERT(this);
	ASSERT(pConfigBuf);

	// Copia a trabajo interno
	m_Config = *pConfigBuf;
}

//---------------------------------------------------------------------------
//
//	Configuracion de la ampliacion de la pantalla
//
//---------------------------------------------------------------------------
void FASTCALL CConfig::SetStretch(BOOL bStretch)
{
	ASSERT(this);

	m_Config.aspect_stretch = bStretch;
}

//---------------------------------------------------------------------------
//
//	Configuracion del dispositivo MIDI
//
//---------------------------------------------------------------------------
void FASTCALL CConfig::SetMIDIDevice(int nDevice, BOOL bIn)
{
	ASSERT(this);
	ASSERT(nDevice >= 0);

	// In�܂���Out
	if (bIn) {
		m_Config.midiin_device = nDevice;
	}
	else {
		m_Config.midiout_device = nDevice;
	}
}

//---------------------------------------------------------------------------
//
//	Tabla del archivo INI
//	Puntero, nombre de la seccion, nombre de la clave, tipo, valor por defecto, valor minimo, valor maximo, en ese orden
//
//---------------------------------------------------------------------------
const CConfig::INIKEY CConfig::IniTable[] = {
	{ &CConfig::m_Config.system_clock, _T("Basic"), _T("Clock"), 0, 0, 0, 5 },
	{ &CConfig::m_Config.mpu_fullspeed, NULL, _T("MPUFullSpeed"), 1, FALSE, 0, 0 },
	{ &CConfig::m_Config.vm_fullspeed, NULL, _T("VMFullSpeed"), 1, FALSE, 0, 0 },
	{ &CConfig::m_Config.ram_size, NULL, _T("Memory"), 0, 0, 0, 5 },
	{ &CConfig::m_Config.ram_sramsync, NULL, _T("AutoMemSw"), 1, TRUE, 0, 0 },
	{ &CConfig::m_Config.mem_type, NULL, _T("Map"), 0, 1, 1, 6 },

	{ &CConfig::m_Config.sound_device, _T("Sound"), _T("Device"), 0, 0, 0, 15 },
	{ &CConfig::m_Config.sample_rate, NULL, _T("Rate"), 0, 5, 0, 5 },
	{ &CConfig::m_Config.primary_buffer, NULL, _T("Primary"), 0, 10, 2, 100 },
	{ &CConfig::m_Config.polling_buffer, NULL, _T("Polling"), 0, 5, 1, 100 },
	{ &CConfig::m_Config.adpcm_interp, NULL, _T("ADPCMInterP"), 1, TRUE, 0, 0 },

	{ &CConfig::m_Config.aspect_stretch, _T("Display"), _T("Stretch"), 1, TRUE, 0, 0 },
	{ &CConfig::m_Config.caption_info, NULL, _T("Info"), 1, TRUE, 0, 0 },

	{ &CConfig::m_Config.master_volume, _T("Volume"), _T("Master"), 0, 100, 0, 100 },
	{ &CConfig::m_Config.fm_enable, NULL, _T("FMEnable"), 1, TRUE, 0, 0 },
	{ &CConfig::m_Config.fm_volume, NULL, _T("FM"), 0, 54, 0, 100 },
	{ &CConfig::m_Config.adpcm_enable, NULL, _T("ADPCMEnable"), 1, TRUE, 0, 0 },
	{ &CConfig::m_Config.adpcm_volume, NULL, _T("ADPCM"), 0, 52, 0, 100 },

	{ &CConfig::m_Config.kbd_connect, _T("Keyboard"), _T("Connect"), 1, TRUE, 0, 0 },

	{ &CConfig::m_Config.mouse_speed, _T("Mouse"), _T("Speed"), 0, 205, 0, 512 },
	{ &CConfig::m_Config.mouse_port, NULL, _T("Port"), 0, 1, 0, 2 },
	{ &CConfig::m_Config.mouse_swap, NULL, _T("Swap"), 1, FALSE, 0, 0 },
	{ &CConfig::m_Config.mouse_mid, NULL, _T("MidBtn"), 1, TRUE, 0, 0 },
	{ &CConfig::m_Config.mouse_trackb, NULL, _T("TrackBall"), 1, FALSE, 0, 0 },

	{ &CConfig::m_Config.joy_type[0], _T("Joystick"), _T("Port1"), 0, 1, 0, 15 },
	{ &CConfig::m_Config.joy_type[1], NULL, _T("Port2"), 0, 1, 0, 15 },
	{ &CConfig::m_Config.joy_dev[0], NULL, _T("Device1"), 0, 1, 0, 15 },
	{ &CConfig::m_Config.joy_dev[1], NULL, _T("Device2"), 0, 2, 0, 15 },
	{ &CConfig::m_Config.joy_button0[0], NULL, _T("Button11"), 0, 1, 0, 131071 },
	{ &CConfig::m_Config.joy_button0[1], NULL, _T("Button12"), 0, 2, 0, 131071 },
	{ &CConfig::m_Config.joy_button0[2], NULL, _T("Button13"), 0, 3, 0, 131071 },
	{ &CConfig::m_Config.joy_button0[3], NULL, _T("Button14"), 0, 4, 0, 131071 },
	{ &CConfig::m_Config.joy_button0[4], NULL, _T("Button15"), 0, 5, 0, 131071 },
	{ &CConfig::m_Config.joy_button0[5], NULL, _T("Button16"), 0, 6, 0, 131071 },
	{ &CConfig::m_Config.joy_button0[6], NULL, _T("Button17"), 0, 7, 0, 131071 },
	{ &CConfig::m_Config.joy_button0[7], NULL, _T("Button18"), 0, 8, 0, 131071 },
	{ &CConfig::m_Config.joy_button0[8], NULL, _T("Button19"), 0, 0, 0, 131071 },
	{ &CConfig::m_Config.joy_button0[9], NULL, _T("Button1A"), 0, 0, 0, 131071 },
	{ &CConfig::m_Config.joy_button0[10], NULL, _T("Button1B"), 0, 0, 0, 131071 },
	{ &CConfig::m_Config.joy_button0[11], NULL, _T("Button1C"), 0, 0, 0, 131071 },
	{ &CConfig::m_Config.joy_button1[0], NULL, _T("Button21"), 0, 65537, 0, 131071 },
	{ &CConfig::m_Config.joy_button1[1], NULL, _T("Button22"), 0, 65538, 0, 131071 },
	{ &CConfig::m_Config.joy_button1[2], NULL, _T("Button23"), 0, 65539, 0, 131071 },
	{ &CConfig::m_Config.joy_button1[3], NULL, _T("Button24"), 0, 65540, 0, 131071 },
	{ &CConfig::m_Config.joy_button1[4], NULL, _T("Button25"), 0, 65541, 0, 131071 },
	{ &CConfig::m_Config.joy_button1[5], NULL, _T("Button26"), 0, 65542, 0, 131071 },
	{ &CConfig::m_Config.joy_button1[6], NULL, _T("Button27"), 0, 65543, 0, 131071 },
	{ &CConfig::m_Config.joy_button1[7], NULL, _T("Button28"), 0, 65544, 0, 131071 },
	{ &CConfig::m_Config.joy_button1[8], NULL, _T("Button29"), 0, 0, 0, 131071 },
	{ &CConfig::m_Config.joy_button1[9], NULL, _T("Button2A"), 0, 0, 0, 131071 },
	{ &CConfig::m_Config.joy_button1[10], NULL, _T("Button2B"), 0, 0, 0, 131071 },
	{ &CConfig::m_Config.joy_button1[11], NULL, _T("Button2C"), 0, 0, 0, 131071 },

	{ &CConfig::m_Config.sasi_drives, _T("SASI"), _T("Drives"), 0, -1, 0, 16 },
	{ &CConfig::m_Config.sasi_sramsync, NULL, _T("AutoMemSw"), 1, TRUE, 0, 0 },
	{ CConfig::m_Config.sasi_file[ 0], NULL, _T("File0") , 2, FILEPATH_MAX, 0, 0 },
	{ CConfig::m_Config.sasi_file[ 1], NULL, _T("File1") , 2, FILEPATH_MAX, 0, 0 },
	{ CConfig::m_Config.sasi_file[ 2], NULL, _T("File2") , 2, FILEPATH_MAX, 0, 0 },
	{ CConfig::m_Config.sasi_file[ 3], NULL, _T("File3") , 2, FILEPATH_MAX, 0, 0 },
	{ CConfig::m_Config.sasi_file[ 4], NULL, _T("File4") , 2, FILEPATH_MAX, 0, 0 },
	{ CConfig::m_Config.sasi_file[ 5], NULL, _T("File5") , 2, FILEPATH_MAX, 0, 0 },
	{ CConfig::m_Config.sasi_file[ 6], NULL, _T("File6") , 2, FILEPATH_MAX, 0, 0 },
	{ CConfig::m_Config.sasi_file[ 7], NULL, _T("File7") , 2, FILEPATH_MAX, 0, 0 },
	{ CConfig::m_Config.sasi_file[ 8], NULL, _T("File8") , 2, FILEPATH_MAX, 0, 0 },
	{ CConfig::m_Config.sasi_file[ 9], NULL, _T("File9") , 2, FILEPATH_MAX, 0, 0 },
	{ CConfig::m_Config.sasi_file[10], NULL, _T("File10"), 2, FILEPATH_MAX, 0, 0 },
	{ CConfig::m_Config.sasi_file[11], NULL, _T("File11"), 2, FILEPATH_MAX, 0, 0 },
	{ CConfig::m_Config.sasi_file[12], NULL, _T("File12"), 2, FILEPATH_MAX, 0, 0 },
	{ CConfig::m_Config.sasi_file[13], NULL, _T("File13"), 2, FILEPATH_MAX, 0, 0 },
	{ CConfig::m_Config.sasi_file[14], NULL, _T("File14"), 2, FILEPATH_MAX, 0, 0 },
	{ CConfig::m_Config.sasi_file[15], NULL, _T("File15"), 2, FILEPATH_MAX, 0, 0 },

	{ &CConfig::m_Config.sxsi_drives, _T("SxSI"), _T("Drives"), 0, 0, 0, 7 },
	{ &CConfig::m_Config.sxsi_mofirst, NULL, _T("FirstMO"), 1, FALSE, 0, 0 },
	{ CConfig::m_Config.sxsi_file[ 0], NULL, _T("File0"), 2, FILEPATH_MAX, 0, 0 },
	{ CConfig::m_Config.sxsi_file[ 1], NULL, _T("File1"), 2, FILEPATH_MAX, 0, 0 },
	{ CConfig::m_Config.sxsi_file[ 2], NULL, _T("File2"), 2, FILEPATH_MAX, 0, 0 },
	{ CConfig::m_Config.sxsi_file[ 3], NULL, _T("File3"), 2, FILEPATH_MAX, 0, 0 },
	{ CConfig::m_Config.sxsi_file[ 4], NULL, _T("File4"), 2, FILEPATH_MAX, 0, 0 },
	{ CConfig::m_Config.sxsi_file[ 5], NULL, _T("File5"), 2, FILEPATH_MAX, 0, 0 },

	{ &CConfig::m_Config.scsi_ilevel, _T("SCSI"), _T("IntLevel"), 0, 1, 0, 1 },
	{ &CConfig::m_Config.scsi_drives, NULL, _T("Drives"), 0, 0, 0, 7 },
	{ &CConfig::m_Config.scsi_sramsync, NULL, _T("AutoMemSw"), 1, TRUE, 0, 0 },
	{ &m_bCDROM,						NULL, _T("CDROM"), 1, FALSE, 0, 0 },
	{ &CConfig::m_Config.scsi_mofirst, NULL, _T("FirstMO"), 1, FALSE, 0, 0 },
	{ CConfig::m_Config.scsi_file[ 0], NULL, _T("File0"), 2, FILEPATH_MAX, 0, 0 },
	{ CConfig::m_Config.scsi_file[ 1], NULL, _T("File1"), 2, FILEPATH_MAX, 0, 0 },
	{ CConfig::m_Config.scsi_file[ 2], NULL, _T("File2"), 2, FILEPATH_MAX, 0, 0 },
	{ CConfig::m_Config.scsi_file[ 3], NULL, _T("File3"), 2, FILEPATH_MAX, 0, 0 },
	{ CConfig::m_Config.scsi_file[ 4], NULL, _T("File4"), 2, FILEPATH_MAX, 0, 0 },

	{ &CConfig::m_Config.port_com, _T("Port"), _T("COM"), 0, 0, 0, 9 },
	{ CConfig::m_Config.port_recvlog, NULL, _T("RecvLog"), 2, FILEPATH_MAX, 0, 0 },
	{ &CConfig::m_Config.port_384, NULL, _T("Force38400"), 1, FALSE, 0, 0 },
	{ &CConfig::m_Config.port_lpt, NULL, _T("LPT"), 0, 0, 0, 9 },
	{ CConfig::m_Config.port_sendlog, NULL, _T("SendLog"), 2, FILEPATH_MAX, 0, 0 },

	{ &CConfig::m_Config.midi_bid, _T("MIDI"), _T("ID"), 0, 0, 0, 2 },
	{ &CConfig::m_Config.midi_ilevel, NULL, _T("IntLevel"), 0, 0, 0, 1 },
	{ &CConfig::m_Config.midi_reset, NULL, _T("ResetCmd"), 0, 0, 0, 3 },
	{ &CConfig::m_Config.midiin_device, NULL, _T("InDevice"), 0, 0, 0, 15 },
	{ &CConfig::m_Config.midiin_delay, NULL, _T("InDelay"), 0, 0, 0, 200 },
	{ &CConfig::m_Config.midiout_device, NULL, _T("OutDevice"), 0, 0, 0, 15 },
	{ &CConfig::m_Config.midiout_delay, NULL, _T("OutDelay"), 0, 84, 20, 200 },

	{ &CConfig::m_Config.windrv_enable, _T("Windrv"), _T("Enable"), 0, 0, 0, 255 },
	{ &CConfig::m_Config.host_option, NULL, _T("Option"), 0, 0, 0, 0x7FFFFFFF },
	{ &CConfig::m_Config.host_resume, NULL, _T("Resume"), 1, FALSE, 0, 0 },
	{ &CConfig::m_Config.host_drives, NULL, _T("Drives"), 0, 0, 0, 10 },
	{ &CConfig::m_Config.host_flag[ 0], NULL, _T("Flag0"), 0, 0, 0, 0x7FFFFFFF },
	{ &CConfig::m_Config.host_flag[ 1], NULL, _T("Flag1"), 0, 0, 0, 0x7FFFFFFF },
	{ &CConfig::m_Config.host_flag[ 2], NULL, _T("Flag2"), 0, 0, 0, 0x7FFFFFFF },
	{ &CConfig::m_Config.host_flag[ 3], NULL, _T("Flag3"), 0, 0, 0, 0x7FFFFFFF },
	{ &CConfig::m_Config.host_flag[ 4], NULL, _T("Flag4"), 0, 0, 0, 0x7FFFFFFF },
	{ &CConfig::m_Config.host_flag[ 5], NULL, _T("Flag5"), 0, 0, 0, 0x7FFFFFFF },
	{ &CConfig::m_Config.host_flag[ 6], NULL, _T("Flag6"), 0, 0, 0, 0x7FFFFFFF },
	{ &CConfig::m_Config.host_flag[ 7], NULL, _T("Flag7"), 0, 0, 0, 0x7FFFFFFF },
	{ &CConfig::m_Config.host_flag[ 8], NULL, _T("Flag8"), 0, 0, 0, 0x7FFFFFFF },
	{ &CConfig::m_Config.host_flag[ 9], NULL, _T("Flag9"), 0, 0, 0, 0x7FFFFFFF },
	{ CConfig::m_Config.host_path[ 0], NULL, _T("Path0"), 2, FILEPATH_MAX, 0, 0 },
	{ CConfig::m_Config.host_path[ 1], NULL, _T("Path1"), 2, FILEPATH_MAX, 0, 0 },
	{ CConfig::m_Config.host_path[ 2], NULL, _T("Path2"), 2, FILEPATH_MAX, 0, 0 },
	{ CConfig::m_Config.host_path[ 3], NULL, _T("Path3"), 2, FILEPATH_MAX, 0, 0 },
	{ CConfig::m_Config.host_path[ 4], NULL, _T("Path4"), 2, FILEPATH_MAX, 0, 0 },
	{ CConfig::m_Config.host_path[ 5], NULL, _T("Path5"), 2, FILEPATH_MAX, 0, 0 },
	{ CConfig::m_Config.host_path[ 6], NULL, _T("Path6"), 2, FILEPATH_MAX, 0, 0 },
	{ CConfig::m_Config.host_path[ 7], NULL, _T("Path7"), 2, FILEPATH_MAX, 0, 0 },
	{ CConfig::m_Config.host_path[ 8], NULL, _T("Path8"), 2, FILEPATH_MAX, 0, 0 },
	{ CConfig::m_Config.host_path[ 9], NULL, _T("Path9"), 2, FILEPATH_MAX, 0, 0 },

	{ &CConfig::m_Config.sram_64k, _T("Alter"), _T("SRAM64K"), 1, FALSE, 0, 0 },
	{ &CConfig::m_Config.scc_clkup, NULL, _T("SCCClock"), 1, FALSE, 0, 0 },
	{ &CConfig::m_Config.power_led, NULL, _T("BlueLED"), 1, FALSE, 0, 0 },
	{ &CConfig::m_Config.dual_fdd, NULL, _T("DualFDD"), 1, FALSE, 0, 0 },
	{ &CConfig::m_Config.sasi_parity, NULL, _T("SASIParity"), 1, TRUE, 0, 0 },

	{ &CConfig::m_Config.caption, _T("Window"), _T("Caption"), 1, TRUE, 0, 0 },
	{ &CConfig::m_Config.menu_bar, NULL, _T("MenuBar"), 1, TRUE, 0, 0 },
	{ &CConfig::m_Config.status_bar, NULL, _T("StatusBar"), 1, TRUE, 0, 0 },
	{ &CConfig::m_Config.window_left, NULL, _T("Left"), 0, -0x8000, -0x8000, 0x7fff },
	{ &CConfig::m_Config.window_top, NULL, _T("Top"), 0, -0x8000, -0x8000, 0x7fff },
	{ &CConfig::m_Config.window_full, NULL, _T("Full"), 1, FALSE, 0, 0 },
	{ &CConfig::m_Config.window_mode, NULL, _T("Mode"), 0, 0, 0, 0 },

	{ &CConfig::m_Config.resume_fd, _T("Resume"), _T("FD"), 1, FALSE, 0, 0 },
	{ &CConfig::m_Config.resume_fdi[0], NULL, _T("FDI0"), 1, FALSE, 0, 0 },
	{ &CConfig::m_Config.resume_fdi[1], NULL, _T("FDI1"), 1, FALSE, 0, 0 },
	{ &CConfig::m_Config.resume_fdw[0], NULL, _T("FDW0"), 1, FALSE, 0, 0 },
	{ &CConfig::m_Config.resume_fdw[1], NULL, _T("FDW1"), 1, FALSE, 0, 0 },
	{ &CConfig::m_Config.resume_fdm[0], NULL, _T("FDM0"), 0, 0, 0, 15 },
	{ &CConfig::m_Config.resume_fdm[1], NULL, _T("FDM1"), 0, 0, 0, 15 },
	{ &CConfig::m_Config.resume_mo, NULL, _T("MO"), 1, FALSE, 0, 0 },
	{ &CConfig::m_Config.resume_mos, NULL, _T("MOS"), 1, FALSE, 0, 0 },
	{ &CConfig::m_Config.resume_mow, NULL, _T("MOW"), 1, FALSE, 0, 0 },
	{ &CConfig::m_Config.resume_cd, NULL, _T("CD"), 1, FALSE, 0, 0 },
	{ &CConfig::m_Config.resume_iso, NULL, _T("ISO"), 1, FALSE, 0, 0 },
	{ &CConfig::m_Config.resume_state, NULL, _T("State"), 1, FALSE, 0, 0 },
	{ &CConfig::m_Config.resume_xm6, NULL, _T("XM6"), 1, FALSE, 0, 0 },
	{ &CConfig::m_Config.resume_screen, NULL, _T("Screen"), 1, FALSE, 0, 0 },
	{ &CConfig::m_Config.resume_dir, NULL, _T("Dir"), 1, FALSE, 0, 0 },
	{ &CConfig::m_Config.resume_path, NULL, _T("Path"), 2, FILEPATH_MAX, 0, 0 },

	{ &CConfig::m_Config.tkey_mode, _T("TrueKey"), _T("Mode"), 0, 1, 0, 3 },
	{ &CConfig::m_Config.tkey_com, NULL, _T("COM"), 0, 0, 0, 9 },
	{ &CConfig::m_Config.tkey_rts, NULL, _T("RTS"), 1, FALSE, 0, 0 },

	{ &CConfig::m_Config.floppy_speed, _T("Misc"), _T("FloppySpeed"), 1, FALSE, 0, 0 },
	{ &CConfig::m_Config.floppy_led, NULL, _T("FloppyLED"), 1, TRUE, 0, 0 },
	{ &CConfig::m_Config.popup_swnd, NULL, _T("PopupWnd"), 1, FALSE, 0, 0 },
	{ &CConfig::m_Config.auto_mouse, NULL, _T("AutoMouse"), 1, FALSE, 0, 0 },
	{ &CConfig::m_Config.power_off, NULL, _T("PowerOff"), 1, FALSE, 0, 0 },
	{ &CConfig::m_Config.ruta_savestate, NULL, _T("RutaSave"), 2, FILEPATH_MAX, 0, 0 },
	{ NULL, NULL, NULL, 0, 0, 0, 0 }
};

//---------------------------------------------------------------------------
//
//	Carga de datos de configuracion
//
//---------------------------------------------------------------------------
void FASTCALL CConfig::LoadConfig()
{
	PINIKEY pIni;
	int nValue;
	BOOL bFlag;
	LPCTSTR pszSection;
	TCHAR szDef[1];
	TCHAR szBuf[FILEPATH_MAX];

	ASSERT(this);

	// �e�[�u���̐擪�ɍ��킹��
	pIni = (const PINIKEY)&IniTable[0];
	pszSection = NULL;
	szDef[0] = _T('\0');

	// �e�[�u�����[�v
	while (pIni->pBuf) {
		// �Z�N�V�����ݒ�
		if (pIni->pszSection) {
			pszSection = pIni->pszSection;
		}
		ASSERT(pszSection);

		// �^�C�v�`�F�b�N
		switch (pIni->nType) {
			// �����^(�͈͂𒴂�����f�t�H���g�l)
			case 0:
				nValue = ::GetPrivateProfileInt(pszSection, pIni->pszKey, pIni->nDef, m_IniFile);
				if ((nValue < pIni->nMin) || (pIni->nMax < nValue)) {
					nValue = pIni->nDef;
				}
				*((int*)pIni->pBuf) = nValue;
				break;

			// �_���^(0,1�̂ǂ���ł��Ȃ���΃f�t�H���g�l)
			case 1:
				nValue = ::GetPrivateProfileInt(pszSection, pIni->pszKey, -1, m_IniFile);
				switch (nValue) {
					case 0:
						bFlag = FALSE;
						break;
					case 1:
						bFlag = TRUE;
						break;
					default:
						bFlag = (BOOL)pIni->nDef;
						break;
				}
				*((BOOL*)pIni->pBuf) = bFlag;
				break;

			// ������^(�o�b�t�@�T�C�Y�͈͓��ł̃^�[�~�l�[�g��ۏ�)
			case 2:
				ASSERT(pIni->nDef <= (sizeof(szBuf)/sizeof(TCHAR)));
				::GetPrivateProfileString(pszSection, pIni->pszKey, szDef, szBuf,
										sizeof(szBuf)/sizeof(TCHAR), m_IniFile);

				// �f�t�H���g�l�ɂ̓o�b�t�@�T�C�Y���L�����邱��
				ASSERT(pIni->nDef > 0);
				szBuf[pIni->nDef - 1] = _T('\0');
				_tcscpy((LPTSTR)pIni->pBuf, szBuf);
				break;

			// ���̑�
			default:
				ASSERT(FALSE);
				break;
		}

		// ����
		pIni++;
	}
}

//---------------------------------------------------------------------------
//
//	Guardar los datos de configuracion
//
//---------------------------------------------------------------------------
void FASTCALL CConfig::SaveConfig() const
{
	PINIKEY pIni;
	CString string;
	LPCTSTR pszSection;

	ASSERT(this);

	// �e�[�u���̐擪�ɍ��킹��
	pIni = (const PINIKEY)&IniTable[0];
	pszSection = NULL;

	// �e�[�u�����[�v
	while (pIni->pBuf) {
		// �Z�N�V�����ݒ�
		if (pIni->pszSection) {
			pszSection = pIni->pszSection;
		}
		ASSERT(pszSection);

		// �^�C�v�`�F�b�N
		switch (pIni->nType) {
			// �����^
			case 0:
				string.Format(_T("%d"), *((int*)pIni->pBuf));
				::WritePrivateProfileString(pszSection, pIni->pszKey,
											string, m_IniFile);
				break;

			// �_���^
			case 1:
				if (*(BOOL*)pIni->pBuf) {
					string = _T("1");
				}
				else {
					string = _T("0");
				}
				::WritePrivateProfileString(pszSection, pIni->pszKey,
											string, m_IniFile);
				break;

			// ������^
			case 2:
				::WritePrivateProfileString(pszSection, pIni->pszKey,
											(LPCTSTR)pIni->pBuf, m_IniFile);
				break;

			// ���̑�
			default:
				ASSERT(FALSE);
				break;
		}

		// ����
		pIni++;
	}
}

//---------------------------------------------------------------------------
//
//	Reinicio del SASI
//	Hasta la version 1.44, la busqueda de archivos es automatica, por lo que es necesario volver a buscar y configurar la busqueda de archivos aqui.
//	Transicion sin problemas a la version 1.45 o posterior
//
//---------------------------------------------------------------------------
void FASTCALL CConfig::ResetSASI()
{
	int i;
	Filepath path;
	Fileio fio;
	TCHAR szPath[FILEPATH_MAX];

	ASSERT(this);

	// �h���C�u��>=0�̏ꍇ�͕s�v(�ݒ�ς�)
	if (m_Config.sasi_drives >= 0) {
		return;
	}

	// �h���C�u��0
	m_Config.sasi_drives = 0;

	// �t�@�C�����쐬���[�v
	for (i=0; i<16; i++) {
		_stprintf(szPath, _T("HD%d.HDF"), i);
		path.SetPath(szPath);
		path.SetBaseDir();
		_tcscpy(m_Config.sasi_file[i], path.GetPath());
	}

	// �ŏ�����`�F�b�N���āA�L���ȃh���C�u�������߂�
	for (i=0; i<16; i++) {
		path.SetPath(m_Config.sasi_file[i]);
		if (!fio.Open(path, Fileio::ReadOnly)) {
			return;
		}

		// �T�C�Y�`�F�b�N(version1.44�ł�40MB�h���C�u�̂݃T�|�[�g)
		if (fio.GetFileSize() != 0x2793000) {
			fio.Close();
			return;
		}

		// �J�E���g�A�b�v�ƃN���[�Y
		m_Config.sasi_drives++;
		fio.Close();
	}
}

//---------------------------------------------------------------------------
//
//	Reinicio del CD-ROM
//	Hasta la version 2.02, el numero de unidades SCSI debe aumentarse en 1, ya que el CD-ROM no es compatible.
//
//---------------------------------------------------------------------------
void FASTCALL CConfig::ResetCDROM()
{
	ASSERT(this);

	// No es necesario si el indicador de CD-ROM esta activado (ya esta activado)
	if (m_bCDROM) {
		return;
	}

	// CD-ROM�t���O���Z�b�g
	m_bCDROM = TRUE;

	// Solo si el numero de unidades SCSI esta entre 3 y 6, +1
	if ((m_Config.scsi_drives >= 3) && (m_Config.scsi_drives <= 6)) {
		m_Config.scsi_drives++;
	}
}

//---------------------------------------------------------------------------
//
//	Bandera del CD-ROM
//
//---------------------------------------------------------------------------
BOOL CConfig::m_bCDROM;

//---------------------------------------------------------------------------
//
//	MRU limpieza
//
//---------------------------------------------------------------------------
void FASTCALL CConfig::ClearMRU(int nType)
{
	int i;

	ASSERT(this);
	ASSERT((nType >= 0) && (nType < MruTypes));

	// ���̃N���A
	for (i=0; i<9; i++) {
		memset(m_MRUFile[nType][i], 0, FILEPATH_MAX * sizeof(TCHAR));
	}

	// ���N���A
	m_MRUNum[nType] = 0;
}

//---------------------------------------------------------------------------
//
//	Cargar MRU
//
//---------------------------------------------------------------------------
void FASTCALL CConfig::LoadMRU(int nType)
{
	CString strSection;
	CString strKey;
	int i;

	ASSERT(this);
	ASSERT((nType >= 0) && (nType < MruTypes));

	// Creacion de la seccion
	strSection.Format(_T("MRU%d"), nType);

	// ���[�v
	for (i=0; i<9; i++) {
		strKey.Format(_T("File%d"), i);
		::GetPrivateProfileString(strSection, strKey, _T(""), m_MRUFile[nType][i],
								FILEPATH_MAX, m_IniFile);
		if (m_MRUFile[nType][i][0] == _T('\0')) {
			break;
		}
		m_MRUNum[nType]++;
	}
}

//---------------------------------------------------------------------------
//
//	MRU�Z�[�u
//
//---------------------------------------------------------------------------
void FASTCALL CConfig::SaveMRU(int nType) const
{
	CString strSection;
	CString strKey;
	int i;

	ASSERT(this);
	ASSERT((nType >= 0) && (nType < MruTypes));

	// Creacion de la seccion
	strSection.Format(_T("MRU%d"), nType);

	// Bucle
	for (i=0; i<9; i++) {
		strKey.Format(_T("File%d"), i);
		::WritePrivateProfileString(strSection, strKey, m_MRUFile[nType][i],
								m_IniFile);
	}
}

//---------------------------------------------------------------------------
//
//	Juego de MRU
//
//---------------------------------------------------------------------------
void FASTCALL CConfig::SetMRUFile(int nType, LPCTSTR lpszFile)
{
	int nMRU;
	int nCpy;
	int nNum;

	ASSERT(this);
	ASSERT((nType >= 0) && (nType < MruTypes));
	ASSERT(lpszFile);

	// ?Ya tienes el mismo?
	nNum = GetMRUNum(nType);
	for (nMRU=0; nMRU<nNum; nMRU++) {
		if (_tcscmp(m_MRUFile[nType][nMRU], lpszFile) == 0) {
			// Estaba en la parte superior y trate de anadir lo mismo de nuevo.
			if (nMRU == 0) {
				return;
			}

			// Copiar
			for (nCpy=nMRU; nCpy>=1; nCpy--) {
				memcpy(m_MRUFile[nType][nCpy], m_MRUFile[nType][nCpy - 1],
						FILEPATH_MAX * sizeof(TCHAR));
			}

			// Colocado en la parte superior
			_tcscpy(m_MRUFile[nType][0], lpszFile);
			return;
		}
	}

	// �ړ�
	for (nMRU=7; nMRU>=0; nMRU--) {
		memcpy(m_MRUFile[nType][nMRU + 1], m_MRUFile[nType][nMRU],
				FILEPATH_MAX * sizeof(TCHAR));
	}

	// �擪�ɃZ�b�g
	ASSERT(_tcslen(lpszFile) < FILEPATH_MAX);
	_tcscpy(m_MRUFile[nType][0], lpszFile);

	// ���X�V
	if (m_MRUNum[nType] < 9) {
		m_MRUNum[nType]++;
	}
}

//---------------------------------------------------------------------------
//
//	Adquisicion de MRU
//
//---------------------------------------------------------------------------
void FASTCALL CConfig::GetMRUFile(int nType, int nIndex, LPTSTR lpszFile) const
{
	ASSERT(this);
	ASSERT((nType >= 0) && (nType < MruTypes));
	ASSERT((nIndex >= 0) && (nIndex < 9));
	ASSERT(lpszFile);

	// Si el numero de piezas es mayor o igual a 0
	if (nIndex >= m_MRUNum[nType]) {
		lpszFile[0] = _T('\0');
		return;
	}

	// Copiar
	ASSERT(_tcslen(m_MRUFile[nType][nIndex]) < FILEPATH_MAX);
	_tcscpy(lpszFile, m_MRUFile[nType][nIndex]);
}

//---------------------------------------------------------------------------
//
//	Adquisicion del numero de MRUs
//
//---------------------------------------------------------------------------
int FASTCALL CConfig::GetMRUNum(int nType) const
{
	ASSERT(this);
	ASSERT((nType >= 0) && (nType < MruTypes));

	return m_MRUNum[nType];
}

//---------------------------------------------------------------------------
//
//	Carga clave
//
//---------------------------------------------------------------------------
void FASTCALL CConfig::LoadKey() const
{
	DWORD dwMap[0x100];
	CInput *pInput;
	CString strName;
	int i;
	int nValue;
	BOOL bFlag;

	ASSERT(this);

	// Adquisicion de entradas
	pInput = m_pFrmWnd->GetInput();
	ASSERT(pInput);

	// �t���OOFF(�L���f�[�^�Ȃ�)�A�N���A
	bFlag = FALSE;
	memset(dwMap, 0, sizeof(dwMap));

	// ���[�v
	for (i=0; i<0x100; i++) {
		strName.Format(_T("Key%d"), i);
		nValue = ::GetPrivateProfileInt(_T("Keyboard"), strName, 0, m_IniFile);

		// Si el valor no esta dentro del rango, se censura aqui (usando el valor por defecto)
		if ((nValue < 0) || (nValue > 0x73)) {
			return;
		}

		// Establece el valor, si lo hay, y lo marca.
		if (nValue != 0) {
			dwMap[i] = nValue;
			bFlag = TRUE;
		}
	}

	// Si se marca, la configuracion de los datos del mapa
	if (bFlag) {
		pInput->SetKeyMap(dwMap);
	}
}

//---------------------------------------------------------------------------
//
//	Ahorro de llaves
//
//---------------------------------------------------------------------------
void FASTCALL CConfig::SaveKey() const
{
	DWORD dwMap[0x100];
	CInput *pInput;
	CString strName;
	CString strKey;
	int i;

	ASSERT(this);

	// Adquisicion de entradas
	pInput = m_pFrmWnd->GetInput();
	ASSERT(pInput);

	// Adquisicion de datos cartograficos
	pInput->GetKeyMap(dwMap);

	// Bucle
	for (i=0; i<0x100; i++) {
		// Escribir todo (256 tipos)
		strName.Format(_T("Key%d"), i);
		strKey.Format(_T("%d"), dwMap[i]);
		::WritePrivateProfileString(_T("Keyboard"), strName,
									strKey, m_IniFile);
	}
}

//---------------------------------------------------------------------------
//
//	Carga de TrueKey
//
//---------------------------------------------------------------------------
void FASTCALL CConfig::LoadTKey() const
{
	CTKey *pTKey;
	int nMap[0x73];
	CString strName;
	CString strKey;
	int i;
	int nValue;
	BOOL bFlag;

	ASSERT(this);

	// Obtener TrueKey
	pTKey = m_pFrmWnd->GetTKey();
	ASSERT(pTKey);

	// Indicador OFF (no hay datos validos), claro
	bFlag = FALSE;
	memset(nMap, 0, sizeof(nMap));

	// Bucle
	for (i=0; i<0x73; i++) {
		strName.Format(_T("Key%d"), i);
		nValue = ::GetPrivateProfileInt(_T("TrueKey"), strName, 0, m_IniFile);

		// Si el valor no esta dentro del rango, se censura aqui (usando el valor por defecto)
		if ((nValue < 0) || (nValue > 0xfe)) {
			return;
		}

		// Establece el valor, si lo hay, y lo marca.
		if (nValue != 0) {
			nMap[i] = nValue;
			bFlag = TRUE;
		}
	}

	// Si se marca, la configuracion de los datos del mapa
	if (bFlag) {
		pTKey->SetKeyMap(nMap);
	}
}

//---------------------------------------------------------------------------
//
//	Guardar TrueKey
//
//---------------------------------------------------------------------------
void FASTCALL CConfig::SaveTKey() const
{
	CTKey *pTKey;
	int nMap[0x73];
	CString strName;
	CString strKey;
	int i;

	ASSERT(this);

	// TrueKey�擾
	pTKey = m_pFrmWnd->GetTKey();
	ASSERT(pTKey);

	// �L�[�}�b�v�擾
	pTKey->GetKeyMap(nMap);

	// ���[�v
	for (i=0; i<0x73; i++) {
		// ���ׂ�(0x73���)����
		strName.Format(_T("Key%d"), i);
		strKey.Format(_T("%d"), nMap[i]);
		::WritePrivateProfileString(_T("TrueKey"), strName,
									strKey, m_IniFile);
	}
}

//---------------------------------------------------------------------------
//
//	Ahorra
//
//---------------------------------------------------------------------------
BOOL FASTCALL CConfig::Save(Fileio *pFio, int /*nVer*/)
{
	size_t sz;

	ASSERT(this);
	ASSERT(pFio);

	// Guardar el tamano
	sz = sizeof(m_Config);
	if (!pFio->Write(&sz, sizeof(sz))) {
		return FALSE;
	}

	// Guardar la unidad
	if (!pFio->Write(&m_Config, (int)sz)) {
		return FALSE;
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	Cargar
//
//---------------------------------------------------------------------------
BOOL FASTCALL CConfig::Load(Fileio *pFio, int nVer)
{
	size_t sz;

	ASSERT(this);
	ASSERT(pFio);

	// Compatibilidad con versiones anteriores
	if (nVer <= 0x0201) {
		return Load200(pFio);
	}
	if (nVer <= 0x0203) {
		return Load202(pFio);
	}

	// Carga y cotejo de tallas
	if (!pFio->Read(&sz, sizeof(sz))) {
		return FALSE;
	}
	if (sz != sizeof(m_Config)) {
		return FALSE;
	}

	// Cargar el cuerpo
	if (!pFio->Read(&m_Config, (int)sz)) {
		return FALSE;
	}

	// Elevar la bandera de solicitud de ApplyCfg
	m_bApply = TRUE;

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	Cargar(version2.00)
//
//---------------------------------------------------------------------------
BOOL FASTCALL CConfig::Load200(Fileio *pFio)
{
	int i;
	size_t sz;
	Config200 *pConfig200;

	ASSERT(this);
	ASSERT(pFio);

	// Fundido para que solo se pueda cargar la parte de la version 2.00
	pConfig200 = (Config200*)&m_Config;

	// Carga y cotejo de tamanos
	if (!pFio->Read(&sz, sizeof(sz))) {
		return FALSE;
	}
	if (sz != sizeof(Config200)) {
		return FALSE;
	}

	// Cargar el cuerpo
	if (!pFio->Read(pConfig200, (int)sz)) {
		return FALSE;
	}

	// Inicializar el nuevo elemento (Config202)
	m_Config.mem_type = 1;
	m_Config.scsi_ilevel = 1;
	m_Config.scsi_drives = 0;
	m_Config.scsi_sramsync = TRUE;
	m_Config.scsi_mofirst = FALSE;
	for (i=0; i<5; i++) {
		m_Config.scsi_file[i][0] = _T('\0');
	}

	// Inicializar el nuevo elemento (Config204)
	m_Config.windrv_enable = 0;
	m_Config.resume_fd = FALSE;
	m_Config.resume_mo = FALSE;
	m_Config.resume_cd = FALSE;
	m_Config.resume_state = FALSE;
	m_Config.resume_screen = FALSE;
	m_Config.resume_dir = FALSE;

	// Elevar la bandera de solicitud de ApplyCfg
	m_bApply = TRUE;

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	Cargar(version2.02)
//
//---------------------------------------------------------------------------
BOOL FASTCALL CConfig::Load202(Fileio *pFio)
{
	size_t sz;
	Config202 *pConfig202;

	ASSERT(this);
	ASSERT(pFio);

	// Castigalo para que solo se pueda cargar la parte de la version 2.02
	pConfig202 = (Config202*)&m_Config;

	// Carga y cotejo de tamanos
	if (!pFio->Read(&sz, sizeof(sz))) {
		return FALSE;
	}
	if (sz != sizeof(Config202)) {
		return FALSE;
	}

	// Cargar el cuerpo
	if (!pFio->Read(pConfig202, (int)sz)) {
		return FALSE;
	}

	// Inicializar el nuevo elemento (Config204)
	m_Config.windrv_enable = 0;
	m_Config.resume_fd = FALSE;
	m_Config.resume_mo = FALSE;
	m_Config.resume_cd = FALSE;
	m_Config.resume_state = FALSE;
	m_Config.resume_screen = FALSE;
	m_Config.resume_dir = FALSE;

	// Elevar la bandera de solicitud de ApplyCfg
	m_bApply = TRUE;

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	Aplicar la comprobacion de la solicitud
//
//---------------------------------------------------------------------------
BOOL FASTCALL CConfig::IsApply()
{
	ASSERT(this);

	// Si quieres, puedo dejarte aqui.
	if (m_bApply) {
		m_bApply = FALSE;
		return TRUE;
	}

	// No solicitado
	return FALSE;
}

//===========================================================================
//
//	Pagina de propiedades de configuracion
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	Constructor
//
//---------------------------------------------------------------------------
CConfigPage::CConfigPage()
{
	// �����o�ϐ��N���A
	m_dwID = 0;
	m_nTemplate = 0;
	m_uHelpID = 0;
	m_uMsgID = 0;
	m_pConfig = NULL;
	m_pSheet = NULL;
}

//---------------------------------------------------------------------------
//
//	Mapa de mensajes
//
//---------------------------------------------------------------------------
BEGIN_MESSAGE_MAP(CConfigPage, CPropertyPage)
	ON_WM_SETCURSOR()
END_MESSAGE_MAP()

//---------------------------------------------------------------------------
//
//	Inicializacion
//
//---------------------------------------------------------------------------
void FASTCALL CConfigPage::Init(CConfigSheet *pSheet)
{
	int nID;

	ASSERT(this);
	ASSERT(m_dwID != 0);

	// �e�V�[�g�L��
	ASSERT(pSheet);
	m_pSheet = pSheet;

	// ID����
	nID = m_nTemplate;
	if (!::IsJapanese()) {
		nID += 50;
	}

	// �\�z
	CommonConstruct(MAKEINTRESOURCE(nID), 0);

	// �e�V�[�g�ɒǉ�
	pSheet->AddPage(this);
}

//---------------------------------------------------------------------------
//
//	Inicializacion
//
//---------------------------------------------------------------------------
BOOL CConfigPage::OnInitDialog()
{
	CConfigSheet *pSheet;

	ASSERT(this);

	// Recibir datos de configuracion de la ventana principal
	pSheet = (CConfigSheet*)GetParent();
	ASSERT(pSheet);
	m_pConfig = pSheet->m_pConfig;

	// Clase basica
	return CPropertyPage::OnInitDialog();
}

//---------------------------------------------------------------------------
//
//	Pagina activa
//
//---------------------------------------------------------------------------
BOOL CConfigPage::OnSetActive()
{
	CStatic *pStatic;
	CString strEmpty;

	ASSERT(this);

	// Clase basica
	if (!CPropertyPage::OnSetActive()) {
		return FALSE;
	}

	// Ayuda a la inicializacion
	ASSERT(m_uHelpID > 0);
	m_uMsgID = 0;
	pStatic = (CStatic*)GetDlgItem(m_uHelpID);
	ASSERT(pStatic);
	strEmpty.Empty();
	pStatic->SetWindowText(strEmpty);

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	Configuracion del cursor del raton
//
//---------------------------------------------------------------------------
BOOL CConfigPage::OnSetCursor(CWnd *pWnd, UINT nHitTest, UINT nMsg)
{
	CWnd *pChildWnd;
	CPoint pt;
	UINT nID;
	CRect rectParent;
	CRect rectChild;
	CString strText;
	CStatic *pStatic;

	// Se debe especificar la ayuda.
	ASSERT(this);
	ASSERT(m_uHelpID > 0);

	// Adquisicion de la posicion del raton
	GetCursorPos(&pt);

	// Recorre la ventana hija y comprueba si se encuentra dentro del rectangulo
	nID = 0;
	rectParent.top = 0;
	pChildWnd = GetTopWindow();

	// ���[�v
	while (pChildWnd) {
		// �w���vID���g�Ȃ�X�L�b�v
		if (pChildWnd->GetDlgCtrlID() == (int)m_uHelpID) {
			pChildWnd = pChildWnd->GetNextWindow();
			continue;
		}

		// ��`���擾
		pChildWnd->GetWindowRect(&rectChild);

		// �����ɂ��邩
		if (rectChild.PtInRect(pt)) {
			// ���Ɏ擾������`������΁A�����������
			if (rectParent.top == 0) {
				// �ŏ��̌��
				rectParent = rectChild;
				nID = pChildWnd->GetDlgCtrlID();
			}
			else {
				if (rectChild.Width() < rectParent.Width()) {
					// �������̌��
					rectParent = rectChild;
					nID = pChildWnd->GetDlgCtrlID();
				}
			}
		}

		// ����
		pChildWnd = pChildWnd->GetNextWindow();
	}

	// nID���r
	if (m_uMsgID == nID) {
		// ��{�N���X
		return CPropertyPage::OnSetCursor(pWnd, nHitTest, nMsg);
	}
	m_uMsgID = nID;

	// ����������[�h�A�ݒ�
	::GetMsg(m_uMsgID, strText);
	pStatic = (CStatic*)GetDlgItem(m_uHelpID);
	ASSERT(pStatic);
	pStatic->SetWindowText(strText);

	// ��{�N���X
	return CPropertyPage::OnSetCursor(pWnd, nHitTest, nMsg);
}

//===========================================================================
//
//	Pagina basica
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	Constructor
//
//---------------------------------------------------------------------------
CBasicPage::CBasicPage()
{
	// ID,Help��K���ݒ�
	m_dwID = MAKEID('B', 'A', 'S', 'C');
	m_nTemplate = IDD_BASICPAGE;
	m_uHelpID = IDC_BASIC_HELP;
}

//---------------------------------------------------------------------------
//
//	Mapa de mensajes
//
//---------------------------------------------------------------------------
BEGIN_MESSAGE_MAP(CBasicPage, CConfigPage)
	ON_BN_CLICKED(IDC_BASIC_CPUFULLB, OnMPUFull)
END_MESSAGE_MAP()

//---------------------------------------------------------------------------
//
//	Inicializacion
//
//---------------------------------------------------------------------------
BOOL CBasicPage::OnInitDialog()
{
	CString string;
	CButton *pButton;
	CComboBox *pComboBox;
	int i;

	// Clase basica
	CConfigPage::OnInitDialog();

	// Reloj del sistema
	pComboBox = (CComboBox*)GetDlgItem(IDC_BASIC_CLOCKC);
	ASSERT(pComboBox);
	for (i=0; i<6; i++) {
		::GetMsg((IDS_BASIC_CLOCK0 + i), string);
		pComboBox->AddString(string);
	}
	pComboBox->SetCurSel(m_pConfig->system_clock);

	// MPU�t���X�s�[�h
	pButton = (CButton*)GetDlgItem(IDC_BASIC_CPUFULLB);
	ASSERT(pButton);
	pButton->SetCheck(m_pConfig->mpu_fullspeed);

	// VM�t���X�s�[�h
	pButton = (CButton*)GetDlgItem(IDC_BASIC_ALLFULLB);
	ASSERT(pButton);
	pButton->SetCheck(m_pConfig->vm_fullspeed);

	// ���C��������
	pComboBox = (CComboBox*)GetDlgItem(IDC_BASIC_MEMORYC);
	ASSERT(pComboBox);
	for (i=0; i<6; i++) {
		::GetMsg((IDS_BASIC_MEMORY0 + i), string);
		pComboBox->AddString(string);
	}
	pComboBox->SetCurSel(m_pConfig->ram_size);

	// SRAM����
	pButton = (CButton*)GetDlgItem(IDC_BASIC_MEMSWB);
	ASSERT(pButton);
	pButton->SetCheck(m_pConfig->ram_sramsync);

	return TRUE;
}





//---------------------------------------------------------------------------
//
//	Decision
//
//---------------------------------------------------------------------------
void CBasicPage::OnOK()
{
	CButton *pButton;
	CComboBox *pComboBox;

	// Reloj del sistema
	pComboBox = (CComboBox*)GetDlgItem(IDC_BASIC_CLOCKC);
	ASSERT(pComboBox);
	m_pConfig->system_clock = pComboBox->GetCurSel();

	// Velocidad maxima de la MPU
	pButton = (CButton*)GetDlgItem(IDC_BASIC_CPUFULLB);
	ASSERT(pButton);
	m_pConfig->mpu_fullspeed = pButton->GetCheck();

	// VM a toda velocidad
	pButton = (CButton*)GetDlgItem(IDC_BASIC_ALLFULLB);
	ASSERT(pButton);
	m_pConfig->vm_fullspeed = pButton->GetCheck();

	// Memoria principal
	pComboBox = (CComboBox*)GetDlgItem(IDC_BASIC_MEMORYC);
	ASSERT(pComboBox);
	m_pConfig->ram_size = pComboBox->GetCurSel();

	// Sincronizacion de la SRAM
	pButton = (CButton*)GetDlgItem(IDC_BASIC_MEMSWB);
	ASSERT(pButton);
	m_pConfig->ram_sramsync = pButton->GetCheck();

	// Clase basica
	CConfigPage::OnOK();
}

//---------------------------------------------------------------------------
//
//	Velocidad maxima de la MPU
//
//---------------------------------------------------------------------------
void CBasicPage::OnMPUFull()
{
	CSxSIPage *pSxSIPage;
	CButton *pButton;
	CString strWarn;

	// Obtener boton
	pButton = (CButton*)GetDlgItem(IDC_BASIC_CPUFULLB);
	ASSERT(pButton);

	// Si esta apagado, no hago nada.
	if (pButton->GetCheck() == 0) {
		return;
	}

	// Si el SxSI esta desactivado, no se hara nada.
	pSxSIPage = (CSxSIPage*)m_pSheet->SearchPage(MAKEID('S', 'X', 'S', 'I'));
	ASSERT(pSxSIPage);
	if (pSxSIPage->GetDrives(m_pConfig) == 0) {
		return;
	}

	// �x��
	::GetMsg(IDS_MPUSXSI, strWarn);
	MessageBox(strWarn, NULL, MB_ICONINFORMATION | MB_OK);
}

//===========================================================================
//
//	Pagina de sonido
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	Constructor
//
//---------------------------------------------------------------------------
CSoundPage::CSoundPage()
{
	// Siempre se debe establecer la identificacion y la ayuda
	m_dwID = MAKEID('S', 'N', 'D', ' ');
	m_nTemplate = IDD_SOUNDPAGE;
	m_uHelpID = IDC_SOUND_HELP;
}

//---------------------------------------------------------------------------
//
//	Mapa de mensajes
//
//---------------------------------------------------------------------------
BEGIN_MESSAGE_MAP(CSoundPage, CConfigPage)
	ON_WM_VSCROLL()
	ON_CBN_SELCHANGE(IDC_SOUND_DEVICEC, OnSelChange)
END_MESSAGE_MAP()

//---------------------------------------------------------------------------
//
//	Inicializacion
//
//---------------------------------------------------------------------------
BOOL CSoundPage::OnInitDialog()
{
	CFrmWnd *pFrmWnd;
	CSound *pSound;
	CComboBox *pComboBox;
	CButton *pButton;
	CEdit *pEdit;
	CSpinButtonCtrl *pSpin;
	CString strName;
	CString strEdit;
	int i;

	// Clase basica
	CConfigPage::OnInitDialog();

	// Obtener el componente de sonido
	pFrmWnd = (CFrmWnd*)AfxGetApp()->m_pMainWnd;
	ASSERT(pFrmWnd);
	pSound = pFrmWnd->GetSound();
	//int msgboxID = MessageBox(pFrmWnd->RutaSaveStates,"rutasave",  2 );	
	ASSERT(pSound);

	// Inicializacion del cuadro combinado del dispositivo
	pComboBox = (CComboBox*)GetDlgItem(IDC_SOUND_DEVICEC);
	ASSERT(pComboBox);
	pComboBox->ResetContent();
	::GetMsg(IDS_SOUND_NOASSIGN, strName);
	pComboBox->AddString(strName);
	for (i=0; i<pSound->m_nDeviceNum; i++) {
		pComboBox->AddString(pSound->m_DeviceDescr[i]);
	}

	// Posicion del cursor del cuadro combinado
	if (m_pConfig->sample_rate == 0) {
		pComboBox->SetCurSel(0);
	}
	else {
		if (pSound->m_nDeviceNum <= m_pConfig->sound_device) {
			pComboBox->SetCurSel(0);
		}
		else {
			pComboBox->SetCurSel(m_pConfig->sound_device + 1);
		}
	}

	// Inicializacion de la frecuencia de muestreo
	for (i=0; i<5; i++) {
		pButton = (CButton*)GetDlgItem(IDC_SOUND_RATE0 + i);
		ASSERT(pButton);
		pButton->SetCheck(0);
	}
	if (m_pConfig->sample_rate > 0) {
		pButton = (CButton*)GetDlgItem(IDC_SOUND_RATE0 + m_pConfig->sample_rate - 1);
		ASSERT(pButton);
		pButton->SetCheck(1);
	}

	// �o�b�t�@�T�C�Y������
	pEdit = (CEdit*)GetDlgItem(IDC_SOUND_BUF1E);
	ASSERT(pEdit);
	strEdit.Format(_T("%d"), m_pConfig->primary_buffer * 10);
	pEdit->SetWindowText(strEdit);
	pSpin = (CSpinButtonCtrl*)GetDlgItem(IDC_SOUND_BUF1S);
	pSpin->SetBase(10);
	pSpin->SetRange(2, 100);
	pSpin->SetPos(m_pConfig->primary_buffer);

	// �|�[�����O�Ԋu������
	pEdit = (CEdit*)GetDlgItem(IDC_SOUND_BUF2E);
	ASSERT(pEdit);
	strEdit.Format(_T("%d"), m_pConfig->polling_buffer);
	pEdit->SetWindowText(strEdit);
	pSpin = (CSpinButtonCtrl*)GetDlgItem(IDC_SOUND_BUF2S);
	pSpin->SetBase(10);
	pSpin->SetRange(1, 100);
	pSpin->SetPos(m_pConfig->polling_buffer);

	// ADPCM���`��ԏ�����
	pButton = (CButton*)GetDlgItem(IDC_SOUND_INTERP);
	ASSERT(pButton);
	pButton->SetCheck(m_pConfig->adpcm_interp);

	// �R���g���[���L���E����
	m_bEnableCtrl = TRUE;
	if (m_pConfig->sample_rate == 0) {
		EnableControls(FALSE);
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	Decision OK
//
//---------------------------------------------------------------------------
void CSoundPage::OnOK()
{
	CComboBox *pComboBox;
	CButton *pButton;
	CSpinButtonCtrl *pSpin;
	int i;

	// Adquisicion de dispositivos
	pComboBox = (CComboBox*)GetDlgItem(IDC_SOUND_DEVICEC);
	ASSERT(pComboBox);
	if (pComboBox->GetCurSel() == 0) {
		// �f�o�C�X�I���Ȃ�
		m_pConfig->sample_rate = 0;
	}
	else {
		// Con la seleccion de dispositivos
		m_pConfig->sound_device = pComboBox->GetCurSel() - 1;

		// Adquisicion de la tasa de muestreo
		for (i=0; i<5; i++) {
			pButton = (CButton*)GetDlgItem(IDC_SOUND_RATE0 + i);
			ASSERT(pButton);
			if (pButton->GetCheck() == 1) {
				m_pConfig->sample_rate = i + 1;
				break;
			}
		}
	}

	// Adquisicion de la memoria intermedia
	pSpin = (CSpinButtonCtrl*)GetDlgItem(IDC_SOUND_BUF1S);
	ASSERT(pSpin);
	m_pConfig->primary_buffer = LOWORD(pSpin->GetPos());
	pSpin = (CSpinButtonCtrl*)GetDlgItem(IDC_SOUND_BUF2S);
	ASSERT(pSpin);
	m_pConfig->polling_buffer = LOWORD(pSpin->GetPos());

	// ADPCM���`��Ԏ擾
	pButton = (CButton*)GetDlgItem(IDC_SOUND_INTERP);
	ASSERT(pButton);
	m_pConfig->adpcm_interp = pButton->GetCheck();

	// ��{�N���X
	CConfigPage::OnOK();
}

//---------------------------------------------------------------------------
//
//	Desplazamiento vertical
//
//---------------------------------------------------------------------------
void CSoundPage::OnVScroll(UINT /*nSBCode*/, UINT nPos, CScrollBar* pBar)
{
	CEdit *pEdit;
	CSpinButtonCtrl *pSpin;
	CString strEdit;

	// ?Consecuente con el control de giro?
	pSpin = (CSpinButtonCtrl*)GetDlgItem(IDC_SOUND_BUF1S);
	if ((CWnd*)pBar == (CWnd*)pSpin) {
		// �G�f�B�b�g�ɔ��f
		pEdit = (CEdit*)GetDlgItem(IDC_SOUND_BUF1E);
		strEdit.Format(_T("%d"), nPos * 10);
		pEdit->SetWindowText(strEdit);
	}

	// ?Consecuente con el control de giro?
	pSpin = (CSpinButtonCtrl*)GetDlgItem(IDC_SOUND_BUF2S);
	if ((CWnd*)pBar == (CWnd*)pSpin) {
		// �G�f�B�b�g�ɔ��f
		pEdit = (CEdit*)GetDlgItem(IDC_SOUND_BUF2E);
		strEdit.Format(_T("%d"), nPos);
		pEdit->SetWindowText(strEdit);
	}
}

//---------------------------------------------------------------------------
//
//	Cambios en el cuadro combinado
//
//---------------------------------------------------------------------------
void CSoundPage::OnSelChange()
{
	int i;
	CComboBox *pComboBox;
	CButton *pButton;

	pComboBox = (CComboBox*)GetDlgItem(IDC_SOUND_DEVICEC);
	ASSERT(pComboBox);
	if (pComboBox->GetCurSel() == 0) {
		EnableControls(FALSE);
	}
	else {
		EnableControls(TRUE);
	}

	// Consideracion de los ajustes de la tasa de sumideros
	if (m_bEnableCtrl) {
		// Si esta activada, debe marcarse cualquiera de estas opciones
		for (i=0; i<5; i++) {
			pButton = (CButton*)GetDlgItem(IDC_SOUND_RATE0 + i);
			ASSERT(pButton);
			if (pButton->GetCheck() != 0) {
				return;
			}
		}

		// Ninguno de ellos esta comprobado, asi que comprueba 62.5kHz
		pButton = (CButton*)GetDlgItem(IDC_SOUND_RATE4);
		ASSERT(pButton);
		pButton->SetCheck(1);
		return;
	}

	// Si esta desactivado, desactiva todas las comprobaciones
	for (i=0; i<5; i++) {
		pButton = (CButton*)GetDlgItem(IDC_SOUND_RATE0 + i);
		ASSERT(pButton);
		pButton->SetCheck(0);
	}
}

//---------------------------------------------------------------------------
//
//	Cambio de estado de control
//
//---------------------------------------------------------------------------
void FASTCALL CSoundPage::EnableControls(BOOL bEnable) 
{
	int i;
	CWnd *pWnd;

	ASSERT(this);

	// �t���O�`�F�b�N
	if (m_bEnableCtrl == bEnable) {
		return;
	}
	m_bEnableCtrl = bEnable;

	// �f�o�C�X�AHelp�ȊO�̑S�R���g���[����ݒ�
	for(i=0; ; i++) {
		// �I���`�F�b�N
		if (ControlTable[i] == NULL) {
			break;
		}

		// �R���g���[���擾
		pWnd = GetDlgItem(ControlTable[i]);
		ASSERT(pWnd);
		pWnd->EnableWindow(bEnable);
	}
}

//---------------------------------------------------------------------------
//
//	Tabla de ID de control
//
//---------------------------------------------------------------------------
const UINT CSoundPage::ControlTable[] = {
	IDC_SOUND_RATEG,
	IDC_SOUND_RATE0,
	IDC_SOUND_RATE1,
	IDC_SOUND_RATE2,
	IDC_SOUND_RATE3,
	IDC_SOUND_RATE4,
	IDC_SOUND_BUFFERG,
	IDC_SOUND_BUF1L,
	IDC_SOUND_BUF1E,
	IDC_SOUND_BUF1S,
	IDC_SOUND_BUF1MS,
	IDC_SOUND_BUF2L,
	IDC_SOUND_BUF2E,
	IDC_SOUND_BUF2S,
	IDC_SOUND_BUF2MS,
	IDC_SOUND_OPTIONG,
	IDC_SOUND_INTERP,
	NULL
};

//===========================================================================
//
//	Pagina del volumen
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	�R���X�g���N�^
//
//---------------------------------------------------------------------------
CVolPage::CVolPage()
{
	// ID,Help��K���ݒ�
	m_dwID = MAKEID('V', 'O', 'L', ' ');
	m_nTemplate = IDD_VOLPAGE;
	m_uHelpID = IDC_VOL_HELP;

	// �I�u�W�F�N�g
	m_pSound = NULL;
	m_pOPMIF = NULL;
	m_pADPCM = NULL;
	m_pMIDI = NULL;

	// �^�C�}�[
	m_nTimerID = NULL;
}

//---------------------------------------------------------------------------
//
//	Mapa de mensajes
//
//---------------------------------------------------------------------------
BEGIN_MESSAGE_MAP(CVolPage, CConfigPage)
	ON_WM_HSCROLL()
	ON_WM_TIMER()
	ON_BN_CLICKED(IDC_VOL_FMC, OnFMCheck)
	ON_BN_CLICKED(IDC_VOL_ADPCMC, OnADPCMCheck)
END_MESSAGE_MAP()

//---------------------------------------------------------------------------
//
//	Inicializacion p�gina de Volumen
//
//---------------------------------------------------------------------------
BOOL CVolPage::OnInitDialog()
{
	CFrmWnd *pFrmWnd;
	CSliderCtrl *pSlider;
	CStatic *pStatic;
	CString strLabel;
	CButton *pButton;
	int nPos;
	int nMax;

	// ��{�N���X
	CConfigPage::OnInitDialog();

	// �T�E���h�R���|�[�l���g���擾
	pFrmWnd = (CFrmWnd*)AfxGetApp()->m_pMainWnd;
	ASSERT(pFrmWnd);
	m_pSound = pFrmWnd->GetSound();
	ASSERT(m_pSound);

	// OPMIF���擾
	m_pOPMIF = (OPMIF*)::GetVM()->SearchDevice(MAKEID('O', 'P', 'M', ' '));
	ASSERT(m_pOPMIF);

	// ADPCM���擾
	m_pADPCM = (ADPCM*)::GetVM()->SearchDevice(MAKEID('A', 'P', 'C', 'M'));
	ASSERT(m_pADPCM);

	// MIDI���擾
	m_pMIDI = pFrmWnd->GetMIDI();
	ASSERT(m_pMIDI);

	// �}�X�^�{�����[��
	pSlider = (CSliderCtrl*)GetDlgItem(IDC_VOL_VOLS);
	ASSERT(pSlider);
	pSlider->SetRange(0, 100);
	nPos = m_pSound->GetMasterVol(nMax);
	if (nPos >= 0) {
		// ���ʒ����ł���
		pSlider->SetRange(0, nMax);
		pSlider->SetPos(nPos);
		pSlider->EnableWindow(TRUE);
		strLabel.Format(_T(" %d"), (nPos * 100) / nMax);
	}
	else {
		// ���ʒ����ł��Ȃ�
		pSlider->SetPos(0);
		pSlider->EnableWindow(FALSE);
		strLabel.Empty();
	}
	pStatic = (CStatic*)GetDlgItem(IDC_VOL_VOLN);
	pStatic->SetWindowText(strLabel);
	m_nMasterVol = nPos;
	m_nMasterOrg = nPos;

	// WAVE���x��
	pSlider = (CSliderCtrl*)GetDlgItem(IDC_VOL_MASTERS);
	ASSERT(pSlider);
	pSlider->SetRange(0, 100);
	::LockVM();
	nPos = m_pSound->GetVolume();
	::UnlockVM();
	pSlider->SetPos(nPos);
	strLabel.Format(_T(" %d"), nPos);
	pStatic = (CStatic*)GetDlgItem(IDC_VOL_MASTERN);
	pStatic->SetWindowText(strLabel);

	// MIDI���x��
	pSlider = (CSliderCtrl*)GetDlgItem(IDC_VOL_SEPS);
	ASSERT(pSlider);
	pSlider->SetRange(0, 0xffff);
	nPos = m_pMIDI->GetOutVolume();
	if (nPos >= 0) {
		// MIDI�o�̓f�o�C�X�̓A�N�e�B�u�����ʒ����ł���
		pSlider->SetPos(nPos);
		pSlider->EnableWindow(TRUE);
		strLabel.Format(_T(" %d"), ((nPos + 1) * 100) >> 16);
	}
	else {
		// MIDI�o�̓f�o�C�X�̓A�N�e�B�u�łȂ��A���͉��ʒ����ł��Ȃ�
		pSlider->SetPos(0);
		pSlider->EnableWindow(FALSE);
		strLabel.Empty();
	}
	pStatic = (CStatic*)GetDlgItem(IDC_VOL_SEPN);
	pStatic->SetWindowText(strLabel);
	m_nMIDIVol = nPos;
	m_nMIDIOrg = nPos;

	// FM����
	pButton = (CButton*)GetDlgItem(IDC_VOL_FMC);
	ASSERT(pButton);
	pButton->SetCheck(m_pConfig->fm_enable);
	pSlider = (CSliderCtrl*)GetDlgItem(IDC_VOL_FMS);
	ASSERT(pSlider);
	pSlider->SetRange(0, 100);
	pSlider->SetPos(m_pConfig->fm_volume);
	strLabel.Format(_T(" %d"), m_pConfig->fm_volume);
	pStatic = (CStatic*)GetDlgItem(IDC_VOL_FMN);
	pStatic->SetWindowText(strLabel);

	// ADPCM����
	pButton = (CButton*)GetDlgItem(IDC_VOL_ADPCMC);
	ASSERT(pButton);
	pButton->SetCheck(m_pConfig->adpcm_enable);
	pSlider = (CSliderCtrl*)GetDlgItem(IDC_VOL_ADPCMS);
	ASSERT(pSlider);
	pSlider->SetRange(0, 100);
	pSlider->SetPos(m_pConfig->adpcm_volume);
	strLabel.Format(_T(" %d"), m_pConfig->adpcm_volume);
	pStatic = (CStatic*)GetDlgItem(IDC_VOL_ADPCMN);
	pStatic->SetWindowText(strLabel);

	// �^�C�}���J�n(100ms�Ńt�@�C��)
	m_nTimerID = SetTimer(IDD_VOLPAGE, 100, NULL);

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	Desplazamiento horizontal
//
//---------------------------------------------------------------------------
void CVolPage::OnHScroll(UINT /*nSBCode*/, UINT nPos, CScrollBar *pBar)
{
	UINT uID;
	CSliderCtrl *pSlider;
	CStatic *pStatic;
	CString strLabel;

	ASSERT(this);
	ASSERT(pBar);

	// �ϊ�
	pSlider = (CSliderCtrl*)pBar;
	ASSERT(pSlider);

	// �`�F�b�N
	switch (pSlider->GetDlgCtrlID()) {
		// �}�X�^�{�����[���ύX
		case IDC_VOL_VOLS:
			nPos = pSlider->GetPos();
			m_pSound->SetMasterVol(nPos);
			// �X�V��OnTimer�ɔC����
			OnTimer(m_nTimerID);
			return;

		// WAVE���x���ύX
		case IDC_VOL_MASTERS:
			// �ύX
			nPos = pSlider->GetPos();
			::LockVM();
			m_pSound->SetVolume(nPos);
			::UnlockVM();

			// �X�V
			uID = IDC_VOL_MASTERN;
			strLabel.Format(_T(" %d"), nPos);
			break;

		// MIDI���x���ύX
		case IDC_VOL_SEPS:
			nPos = pSlider->GetPos();
			m_pMIDI->SetOutVolume(nPos);
			// �X�V��OnTimer�ɔC����
			OnTimer(m_nTimerID);
			return;

		// FM���ʕύX
		case IDC_VOL_FMS:
			// �ύX
			nPos = pSlider->GetPos();
			::LockVM();
			m_pSound->SetFMVol(nPos);
			::UnlockVM();

			// �X�V
			uID = IDC_VOL_FMN;
			strLabel.Format(_T(" %d"), nPos);
			break;

		// ADPCM���ʕύX
		case IDC_VOL_ADPCMS:
			// �ύX
			nPos = pSlider->GetPos();
			::LockVM();
			m_pSound->SetADPCMVol(nPos);
			::UnlockVM();

			// �X�V
			uID = IDC_VOL_ADPCMN;
			strLabel.Format(_T(" %d"), nPos);
			break;

		// ���̑�
		default:
			ASSERT(FALSE);
			return;
	}

	// �ύX
	pStatic = (CStatic*)GetDlgItem(uID);
	ASSERT(pStatic);
	pStatic->SetWindowText(strLabel);
}

//---------------------------------------------------------------------------
//
//	Temporizadores
//
//---------------------------------------------------------------------------
void CVolPage::OnTimer(UINT /*nTimerID*/)
{
	CSliderCtrl *pSlider;
	CStatic *pStatic;
	CString strLabel;
	int nPos;
	int nMax;

	// Adquisicion del volumen principal
	pSlider = (CSliderCtrl*)GetDlgItem(IDC_VOL_VOLS);
	ASSERT(pSlider);
	nPos = m_pSound->GetMasterVol(nMax);

	// �{�����[����r
	if (nPos != m_nMasterVol) {
		m_nMasterVol = nPos;

		// ����
		if (nPos >= 0) {
			// �L����
			pSlider->SetPos(nPos);
			pSlider->EnableWindow(TRUE);
			strLabel.Format(_T(" %d"), (nPos * 100) / nMax);
		}
		else {
			// ������
			pSlider->SetPos(0);
			pSlider->EnableWindow(FALSE);
			strLabel.Empty();
		}

		pStatic = (CStatic*)GetDlgItem(IDC_VOL_VOLN);
		pStatic->SetWindowText(strLabel);
	}

	// MIDI
	pSlider = (CSliderCtrl*)GetDlgItem(IDC_VOL_SEPS);
	nPos = m_pMIDI->GetOutVolume();

	// MIDI��r
	if (nPos != m_nMIDIVol) {
		m_nMIDIVol = nPos;

		// ����
		if (nPos >= 0) {
			// �L����
			pSlider->SetPos(nPos);
			pSlider->EnableWindow(TRUE);
			strLabel.Format(_T(" %d"), ((nPos + 1) * 100) >> 16);
		}
		else {
			// ������
			pSlider->SetPos(0);
			pSlider->EnableWindow(FALSE);
			strLabel.Empty();
		}

		pStatic = (CStatic*)GetDlgItem(IDC_VOL_SEPN);
		pStatic->SetWindowText(strLabel);
	}
}

//---------------------------------------------------------------------------
//
//	Prueba de sonido FM
//
//---------------------------------------------------------------------------
void CVolPage::OnFMCheck()
{
	CButton *pButton;

	pButton = (CButton*)GetDlgItem(IDC_VOL_FMC);
	ASSERT(pButton);
	if (pButton->GetCheck()) {
		m_pOPMIF->EnableFM(TRUE);
	}
	else {
		m_pOPMIF->EnableFM(FALSE);
	}
}

//---------------------------------------------------------------------------
//
//	Prueba de sonido ADPCM
//
//---------------------------------------------------------------------------
void CVolPage::OnADPCMCheck()
{
	CButton *pButton;

	pButton = (CButton*)GetDlgItem(IDC_VOL_ADPCMC);
	ASSERT(pButton);
	if (pButton->GetCheck()) {
		m_pADPCM->EnableADPCM(TRUE);
	}
	else {
		m_pADPCM->EnableADPCM(FALSE);
	}
}

//---------------------------------------------------------------------------
//
//	Decision OK
//
//---------------------------------------------------------------------------
void CVolPage::OnOK()
{
	CSliderCtrl *pSlider;
	CButton *pButton;

	// �^�C�}��~
	if (m_nTimerID) {
		KillTimer(m_nTimerID);
		m_nTimerID = NULL;
	}

	// WAVE���x��
	pSlider = (CSliderCtrl*)GetDlgItem(IDC_VOL_MASTERS);
	ASSERT(pSlider);
	m_pConfig->master_volume = pSlider->GetPos();

	// FM�L��
	pButton = (CButton*)GetDlgItem(IDC_VOL_FMC);
	ASSERT(pButton);
	m_pConfig->fm_enable = pButton->GetCheck();

	// FM����
	pSlider = (CSliderCtrl*)GetDlgItem(IDC_VOL_FMS);
	ASSERT(pSlider);
	m_pConfig->fm_volume = pSlider->GetPos();

	// ADPCM�L��
	pButton = (CButton*)GetDlgItem(IDC_VOL_ADPCMC);
	ASSERT(pButton);
	m_pConfig->adpcm_enable = pButton->GetCheck();

	// ADPCM����
	pSlider = (CSliderCtrl*)GetDlgItem(IDC_VOL_ADPCMS);
	ASSERT(pSlider);
	m_pConfig->adpcm_volume = pSlider->GetPos();

	// ��{�N���X
	CConfigPage::OnOK();
}

//---------------------------------------------------------------------------
//
//	Cancelar
//
//---------------------------------------------------------------------------
void CVolPage::OnCancel()
{
	// �^�C�}��~
	if (m_nTimerID) {
		KillTimer(m_nTimerID);
		m_nTimerID = NULL;
	}

	// ���̒l�ɍĐݒ�(CONFIG�f�[�^)
	::LockVM();
	m_pSound->SetVolume(m_pConfig->master_volume);
	m_pOPMIF->EnableFM(m_pConfig->fm_enable);
	m_pSound->SetFMVol(m_pConfig->fm_volume);
	m_pADPCM->EnableADPCM(m_pConfig->adpcm_enable);
	m_pSound->SetADPCMVol(m_pConfig->adpcm_volume);
	::UnlockVM();

	// ���̒l�ɍĐݒ�(�~�L�T)
	if (m_nMasterOrg >= 0) {
		m_pSound->SetMasterVol(m_nMasterOrg);
	}
	if (m_nMIDIOrg >= 0) {
		m_pMIDI->SetOutVolume(m_nMIDIOrg);
	}

	// ��{�N���X
	CConfigPage::OnCancel();
}

//===========================================================================
//
//	Pagina del teclado
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	�R���X�g���N�^
//
//---------------------------------------------------------------------------
CKbdPage::CKbdPage()
{
	CFrmWnd *pWnd;

	// ID,Help��K���ݒ�
	m_dwID = MAKEID('K', 'E', 'Y', 'B');
	m_nTemplate = IDD_KBDPAGE;
	m_uHelpID = IDC_KBD_HELP;

	// �C���v�b�g�擾
	pWnd = (CFrmWnd*)AfxGetApp()->m_pMainWnd;
	ASSERT(pWnd);
	m_pInput = pWnd->GetInput();
	ASSERT(m_pInput);
}

//---------------------------------------------------------------------------
//
//	Mapa de mensajes
//
//---------------------------------------------------------------------------
BEGIN_MESSAGE_MAP(CKbdPage, CConfigPage)
	ON_COMMAND(IDC_KBD_EDITB, OnEdit)
	ON_COMMAND(IDC_KBD_DEFB, OnDefault)
	ON_BN_CLICKED(IDC_KBD_NOCONB, OnConnect)
	ON_NOTIFY(NM_CLICK, IDC_KBD_MAPL, OnClick)
	ON_NOTIFY(NM_RCLICK, IDC_KBD_MAPL, OnRClick)
END_MESSAGE_MAP()

//---------------------------------------------------------------------------
//
//	Inicializacion
//
//---------------------------------------------------------------------------
BOOL CKbdPage::OnInitDialog()
{
	CButton *pButton;
	CListCtrl *pListCtrl;
	CClientDC *pDC;
	TEXTMETRIC tm;
	CString strText;
	LPCTSTR lpszName;
	int nKey;
	LONG cx;

	// ��{�N���X
	CConfigPage::OnInitDialog();

	// �L�[�}�b�v�̃o�b�N�A�b�v�����
	m_pInput->GetKeyMap(m_dwBackup);
	memcpy(m_dwEdit, m_dwBackup, sizeof(m_dwBackup));

	// �e�L�X�g���g���b�N�𓾂�
	pDC = new CClientDC(this);
	::GetTextMetrics(pDC->m_hDC, &tm);
	delete pDC;
	cx = tm.tmAveCharWidth;

	// ���X�g�R���g���[����
	pListCtrl = (CListCtrl*)GetDlgItem(IDC_KBD_MAPL);
	ASSERT(pListCtrl);
	if (::IsJapanese()) {
		// ���{��
		::GetMsg(IDS_KBD_NO, strText);
		pListCtrl->InsertColumn(0, strText, LVCFMT_LEFT, cx * 4, 0);
		::GetMsg(IDS_KBD_KEYTOP, strText);
		pListCtrl->InsertColumn(1, strText, LVCFMT_LEFT, cx * 10, 1);
		::GetMsg(IDS_KBD_DIRECTX, strText);
		pListCtrl->InsertColumn(2, strText, LVCFMT_LEFT, cx * 22, 2);
	}
	else {
		// �p��
		::GetMsg(IDS_KBD_NO, strText);
		pListCtrl->InsertColumn(0, strText, LVCFMT_LEFT, cx * 5, 0);
		::GetMsg(IDS_KBD_KEYTOP, strText);
		pListCtrl->InsertColumn(1, strText, LVCFMT_LEFT, cx * 12, 1);
		::GetMsg(IDS_KBD_DIRECTX, strText);
		pListCtrl->InsertColumn(2, strText, LVCFMT_LEFT, cx * 18, 2);
	}

	// ���X�g�R���g���[��1�s�S�̃I�v�V����(COMCTL32.DLL v4.71�ȍ~)
	pListCtrl->SendMessage(LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_FULLROWSELECT);

	// �A�C�e��������(X68000�����̓}�b�s���O�ɂ�炸�Œ�)
	pListCtrl->DeleteAllItems();
	cx = 0;
	for (nKey=0; nKey<=0x73; nKey++) {
		// CKeyDispWnd����L�[���̂𓾂�
		lpszName = m_pInput->GetKeyName(nKey);
		if (lpszName) {
			// ���̃L�[�͗L��
			strText.Format(_T("%02X"), nKey);
			pListCtrl->InsertItem(cx, strText);
			pListCtrl->SetItemText(cx, 1, lpszName);
			pListCtrl->SetItemData(cx, (DWORD)nKey);
			cx++;
		}
	}

	// ���|�[�g�X�V
	UpdateReport();

	// �ڑ�
	pButton = (CButton*)GetDlgItem(IDC_KBD_NOCONB);
	ASSERT(pButton);
	pButton->SetCheck(!m_pConfig->kbd_connect);

	// �R���g���[���X�V
	m_bEnableCtrl = TRUE;
	if (!m_pConfig->kbd_connect) {
		EnableControls(FALSE);
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	Decision OK 
//
//---------------------------------------------------------------------------
void CKbdPage::OnOK()
{
	CButton *pButton;

	// �L�[�}�b�v�ݒ�
	m_pInput->SetKeyMap(m_dwEdit);

	// �ڑ�
	pButton = (CButton*)GetDlgItem(IDC_KBD_NOCONB);
	ASSERT(pButton);
	m_pConfig->kbd_connect = !(pButton->GetCheck());

	// ��{�N���X
	CConfigPage::OnOK();
}

//---------------------------------------------------------------------------
//
//	Cancelar
//
//---------------------------------------------------------------------------
void CKbdPage::OnCancel()
{
	// �o�b�N�A�b�v����L�[�}�b�v����
	m_pInput->SetKeyMap(m_dwBackup);

	// ��{�N���X
	CConfigPage::OnCancel();
}

//---------------------------------------------------------------------------
//
//	Actualizacion del informe
//
//---------------------------------------------------------------------------
void FASTCALL CKbdPage::UpdateReport()
{
	CListCtrl *pListCtrl;
	int nX68;
	int nWin;
	int nItem;
	CString strNext;
	CString strPrev;
	LPCTSTR lpszName;

	ASSERT(this);

	// Adquisicion de control
	pListCtrl = (CListCtrl*)GetDlgItem(IDC_KBD_MAPL);
	ASSERT(pListCtrl);

	// Linea de control de la lista
	nItem = 0;
	for (nX68=0; nX68<=0x73; nX68++) {
		// Obtener el nombre de la llave de CKeyDispWnd
		lpszName = m_pInput->GetKeyName(nX68);
		if (lpszName) {
			// Hay una llave valida. Inicializacion
			strNext.Empty();

			// Establecer la asignacion, si la hay
			for (nWin=0; nWin<0x100; nWin++) {
				if (nX68 == (int)m_dwEdit[nWin]) {
					// Obtener un nombre
					lpszName = m_pInput->GetKeyID(nWin);
					strNext = lpszName;
					break;
				}
			}

			// Si es diferente, reescriba
			strPrev = pListCtrl->GetItemText(nItem, 2);
			if (strPrev != strNext) {
				pListCtrl->SetItemText(nItem, 2, strNext);
			}

			// Ir al siguiente punto
			nItem++;
		}
	}
}

//---------------------------------------------------------------------------
//
//	Edicion de
//
//---------------------------------------------------------------------------
void CKbdPage::OnEdit()
{
	CKbdMapDlg dlg(this, m_dwEdit);

	ASSERT(this);

	// �_�C�A���O���s
	dlg.DoModal();

	// �\�����X�V
	UpdateReport();
}

//---------------------------------------------------------------------------
//
//	Restablecer los valores por defecto
//
//---------------------------------------------------------------------------
void CKbdPage::OnDefault()
{
	ASSERT(this);

	// �����̃o�b�t�@��106�}�b�v�����āA������Z�b�g
	m_pInput->SetDefaultKeyMap(m_dwEdit);
	m_pInput->SetKeyMap(m_dwEdit);

	// �\�����X�V
	UpdateReport();
}

//---------------------------------------------------------------------------
//
//	Articulo clic
//
//---------------------------------------------------------------------------
void CKbdPage::OnClick(NMHDR * /*pNMHDR*/, LRESULT * /*pResult*/)
{
	CListCtrl *pListCtrl;
	int nCount;
	int nItem;
	int nKey;
	int nPrev;
	int i;
	CKeyinDlg dlg(this);

	// ���X�g�R���g���[���擾
	pListCtrl = (CListCtrl*)GetDlgItem(IDC_KBD_MAPL);
	ASSERT(pListCtrl);

	// �J�E���g�����擾
	nCount = pListCtrl->GetItemCount();

	// �Z���N�g����Ă���C���f�b�N�X�𓾂�
	for (nItem=0; nItem<nCount; nItem++) {
		if (pListCtrl->GetItemState(nItem, LVIS_SELECTED)) {
			break;
		}
	}
	if (nItem >= nCount) {
		return;
	}

	// ���̃C���f�b�N�X�̎w���f�[�^�𓾂�
	nKey = (int)pListCtrl->GetItemData(nItem);
	ASSERT((nKey >= 0) && (nKey <= 0x73));

	// �ݒ�J�n
	dlg.m_nTarget = nKey;
	dlg.m_nKey = 0;

	// �Y������Windows�L�[������ΐݒ�
	nPrev = -1;
	for (i=0; i<0x100; i++) {
		if (m_dwEdit[i] == (DWORD)nKey) {
			dlg.m_nKey = i;
			nPrev = i;
			break;
		}
	}

	// �_�C�A���O���s
	m_pInput->EnableKey(FALSE);
	if (dlg.DoModal() != IDOK) {
		m_pInput->EnableKey(TRUE);
		return;
	}
	m_pInput->EnableKey(TRUE);

	// �L�[�}�b�v��ݒ�
	if (nPrev >= 0) {
		m_dwEdit[nPrev] = 0;
	}
	m_dwEdit[dlg.m_nKey] = (DWORD)nKey;

	// SHIFT�L�[��O����
	if (nPrev == DIK_LSHIFT) {
		m_dwEdit[DIK_RSHIFT] = 0;
	}
	if (dlg.m_nKey == DIK_LSHIFT) {
		m_dwEdit[DIK_RSHIFT] = (DWORD)nKey;
	}

	// �\�����X�V
	UpdateReport();
}

//---------------------------------------------------------------------------
//
//	Elemento clic derecho
//
//---------------------------------------------------------------------------
void CKbdPage::OnRClick(NMHDR * /*pNMHDR*/, LRESULT * /*pResult*/)
{
	CListCtrl *pListCtrl;
	int nCount;
	int nItem;
	int nKey;
	int nWin;
	int i;
	CString strText;
	CString strMsg;

	// ���X�g�R���g���[���擾
	pListCtrl = (CListCtrl*)GetDlgItem(IDC_KBD_MAPL);
	ASSERT(pListCtrl);

	// �J�E���g�����擾
	nCount = pListCtrl->GetItemCount();

	// �Z���N�g����Ă���C���f�b�N�X�𓾂�
	for (nItem=0; nItem<nCount; nItem++) {
		if (pListCtrl->GetItemState(nItem, LVIS_SELECTED)) {
			break;
		}
	}
	if (nItem >= nCount) {
		return;
	}

	// ���̃C���f�b�N�X�̎w���f�[�^�𓾂�
	nKey = (int)pListCtrl->GetItemData(nItem);
	ASSERT((nKey >= 0) && (nKey <= 0x73));

	// �Y������Windows�L�[�����邩
	nWin = -1;
	for (i=0; i<0x100; i++) {
		if (m_dwEdit[i] == (DWORD)nKey) {
			nWin = i;
			break;
		}
	}
	if (nWin < 0) {
		// �}�b�s���O����Ă��Ȃ�
		return;
	}

	// ���b�Z�[�W�{�b�N�X�ŁA�폜�̗L�����`�F�b�N
	::GetMsg(IDS_KBD_DELMSG, strText);
	strMsg.Format(strText, nKey, m_pInput->GetKeyID(nWin));
	::GetMsg(IDS_KBD_DELTITLE, strText);
	if (MessageBox(strMsg, strText, MB_ICONQUESTION | MB_YESNO) != IDYES) {
		return;
	}

	// �Y������Windows�L�[���폜
	m_dwEdit[nWin] = 0;

	// SHIFT�L�[��O����
	if (nWin == DIK_LSHIFT) {
		m_dwEdit[DIK_RSHIFT] = 0;
	}

	// �\�����X�V
	UpdateReport();
}

//---------------------------------------------------------------------------
//
//	No conectado
//
//---------------------------------------------------------------------------
void CKbdPage::OnConnect()
{
	CButton *pButton;

	pButton = (CButton*)GetDlgItem(IDC_KBD_NOCONB);
	ASSERT(pButton);

	// �R���g���[���L���E����
	if (pButton->GetCheck() == 1) {
		EnableControls(FALSE);
	}
	else {
		EnableControls(TRUE);
	}
}

//---------------------------------------------------------------------------
//
//	Cambio de estado de control
//
//---------------------------------------------------------------------------
void FASTCALL CKbdPage::EnableControls(BOOL bEnable) 
{
	int i;
	CWnd *pWnd;

	ASSERT(this);

	// �t���O�`�F�b�N
	if (m_bEnableCtrl == bEnable) {
		return;
	}
	m_bEnableCtrl = bEnable;

	// �{�[�hID�AHelp�ȊO�̑S�R���g���[����ݒ�
	for(i=0; ; i++) {
		// �I���`�F�b�N
		if (ControlTable[i] == NULL) {
			break;
		}

		// �R���g���[���擾
		pWnd = GetDlgItem(ControlTable[i]);
		ASSERT(pWnd);
		pWnd->EnableWindow(bEnable);
	}
}

//---------------------------------------------------------------------------
//
//	Tabla de ID de control
//
//---------------------------------------------------------------------------
const UINT CKbdPage::ControlTable[] = {
	IDC_KBD_MAPG,
	IDC_KBD_MAPL,
	IDC_KBD_EDITB,
	IDC_KBD_DEFB,
	NULL
};

//===========================================================================
//
//	Dialogo de edicion del mapa del teclado
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	Constructor
//
//---------------------------------------------------------------------------
CKbdMapDlg::CKbdMapDlg(CWnd *pParent, DWORD *pMap) : CDialog(IDD_KBDMAPDLG, pParent)
{
	CFrmWnd *pFrmWnd;

	// �ҏW�f�[�^���L��
	ASSERT(pMap);
	m_pEditMap = pMap;

	// �p����ւ̑Ή�
	if (!::IsJapanese()) {
		m_lpszTemplateName = MAKEINTRESOURCE(IDD_US_KBDMAPDLG);
		m_nIDHelp = IDD_US_KBDMAPDLG;
	}

	// �C���v�b�g�擾
	pFrmWnd = (CFrmWnd*)AfxGetApp()->m_pMainWnd;
	ASSERT(pFrmWnd);
	m_pInput = pFrmWnd->GetInput();
	ASSERT(m_pInput);

	// �X�e�[�^�X���b�Z�[�W�Ȃ�
	m_strStat.Empty();
}

//---------------------------------------------------------------------------
//
//	Mapa de mensajes
//
//---------------------------------------------------------------------------
#if !defined(WM_KICKIDLE)
#define WM_KICKIDLE		0x36a
#endif	// WM_KICKIDLE
BEGIN_MESSAGE_MAP(CKbdMapDlg, CDialog)
	ON_WM_PAINT()
	ON_MESSAGE(WM_KICKIDLE, OnKickIdle)
	ON_MESSAGE(WM_APP, OnApp)
END_MESSAGE_MAP()

//---------------------------------------------------------------------------
//
//	Inicializacion
//
//---------------------------------------------------------------------------
BOOL CKbdMapDlg::OnInitDialog()
{
	LONG cx;
	LONG cy;
	CRect rectClient;
	CRect rectWnd;
	CStatic *pStatic;

	// Clase basica
	CDialog::OnInitDialog();

	// �N���C�A���g�̑傫���𓾂�
	GetClientRect(&rectClient);

	// �X�e�[�^�X�o�[�̍����𓾂�
	pStatic = (CStatic*)GetDlgItem(IDC_KBDMAP_STAT);
	ASSERT(pStatic);
	pStatic->GetWindowRect(&rectWnd);

	// �������o��(>0���O��)
	cx = 616 - rectClient.Width();
	ASSERT(cx > 0);
	cy = (140 + rectWnd.Height()) - rectClient.Height();
	ASSERT(cy > 0);

	// cx,cy�����A�L���Ă���
	GetWindowRect(&rectWnd);
	SetWindowPos(&wndTop, 0, 0, rectWnd.Width() + cx, rectWnd.Height() + cy, SWP_NOMOVE);

	// �X�e�[�^�X�o�[���Ɉړ��A�폜
	pStatic->GetWindowRect(&rectClient);
	ScreenToClient(&rectClient);
	pStatic->SetWindowPos(&wndTop, 0, 140,
					rectClient.Width() + cx, rectClient.Height(), SWP_NOZORDER);
	pStatic->GetWindowRect(&m_rectStat);
	ScreenToClient(&m_rectStat);
	pStatic->DestroyWindow();

	// �f�B�X�v���C�E�B���h�E���ړ�
	pStatic = (CStatic*)GetDlgItem(IDC_KBDMAP_DISP);
	ASSERT(pStatic);
	pStatic->GetWindowRect(&rectWnd);
	ScreenToClient(&rectWnd);
	pStatic->SetWindowPos(&wndTop, 0, 0, 616, 140, SWP_NOZORDER);

	// �f�B�X�v���C�E�B���h�E�̈ʒu�ɁACKeyDispWnd��z�u
	pStatic->GetWindowRect(&rectWnd);
	ScreenToClient(&rectWnd);
	pStatic->DestroyWindow();
	m_pDispWnd = new CKeyDispWnd;
	m_pDispWnd->Create(NULL, NULL, WS_CHILD | WS_VISIBLE,
					rectWnd, this, 0, NULL);

	// �E�B���h�E����
	CenterWindow(GetParent());

	// IME�I�t
	::ImmAssociateContext(m_hWnd, (HIMC)NULL);

	// �L�[���͂��֎~
	ASSERT(m_pInput);
	m_pInput->EnableKey(FALSE);

	// �K�C�h���b�Z�[�W�����[�h
	::GetMsg(IDS_KBDMAP_GUIDE, m_strGuide);

	// �Ō��OnKickIdle���Ă�(���񂩂猻�݂̏�Ԃŕ\��)
	OnKickIdle(0, 0);

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	OK
//
//---------------------------------------------------------------------------
void CKbdMapDlg::OnOK()
{
	// [CR]�ɂ��I����}��
}

//---------------------------------------------------------------------------
//
//	Cancelar
//
//---------------------------------------------------------------------------
void CKbdMapDlg::OnCancel()
{
	// �L�[�L��
	m_pInput->EnableKey(TRUE);

	// ��{�N���X
	CDialog::OnCancel();
}

//---------------------------------------------------------------------------
//
//	Dibujo del dialogo
//
//---------------------------------------------------------------------------
void CKbdMapDlg::OnPaint()
{
	CPaintDC dc(this);

	// OnDraw�ɔC����
	OnDraw(&dc);
}

//---------------------------------------------------------------------------
//
//	Subcontratacion de dibujos
//
//---------------------------------------------------------------------------
void FASTCALL CKbdMapDlg::OnDraw(CDC *pDC)
{
	CFont *pFont;

	ASSERT(this);
	ASSERT(pDC);

	// �F�ݒ�
	pDC->SetTextColor(::GetSysColor(COLOR_BTNTEXT));
	pDC->SetBkColor(::GetSysColor(COLOR_3DFACE));

	// �t�H���g�ݒ�
	pFont = (CFont*)pDC->SelectStockObject(DEFAULT_GUI_FONT);
	ASSERT(pFont);

	pDC->FillSolidRect(m_rectStat, ::GetSysColor(COLOR_3DFACE));
	pDC->DrawText(m_strStat, m_rectStat,
				DT_LEFT | DT_SINGLELINE | DT_VCENTER | DT_NOPREFIX);

	// �t�H���g�߂�
	pDC->SelectObject(pFont);
}

//---------------------------------------------------------------------------
//
//	Procesamiento en vacio
//
//---------------------------------------------------------------------------
LONG CKbdMapDlg::OnKickIdle(UINT /*uParam*/, LONG /*lParam*/)
{
	BOOL bBuf[0x100];
	BOOL bFlg[0x100];
	int nWin;
	DWORD dwCode;
	CKeyDispWnd *pWnd;

	// �L�[��Ԃ𓾂�
	ASSERT(m_pInput);
	m_pInput->GetKeyBuf(bBuf);

	// �L�[�t���O����U�N���A
	memset(bFlg, 0, sizeof(bFlg));

	// ���݂̃}�b�v�ɏ]���āA�ϊ��\�����
	for (nWin=0; nWin<0x100; nWin++) {
		// �L�[��������Ă��邩
		if (bBuf[nWin]) {
			// �R�[�h�擾
			dwCode = m_pEditMap[nWin];
			if (dwCode != 0) {
				// �L�[��������A���蓖�Ă��Ă���̂ŁA�L�[�o�b�t�@�ݒ�
				bFlg[dwCode] = TRUE;
			}
		}
	}

	// SHIFT�L�[��O����(L,R�����킹��)
	bFlg[0x74] = bFlg[0x70];

	// ���t���b�V��(�`��)
	pWnd = (CKeyDispWnd*)m_pDispWnd;
	pWnd->Refresh(bFlg);

	return 0;
}

//---------------------------------------------------------------------------
//
//	Notificaciones de las ventanas del nivel inferior
//
//---------------------------------------------------------------------------
LONG CKbdMapDlg::OnApp(UINT uParam, LONG lParam)
{
	CKeyinDlg dlg(this);
	int nPrev;
	int nWin;
	CString strText;
	CString strName;
	CClientDC *pDC;

	ASSERT(this);
	ASSERT(uParam <= 0x73);

	// �U�蕪��
	switch (lParam) {
		// ���{�^������
		case WM_LBUTTONDOWN:
			// �^�[�Q�b�g�Ɗ��蓖�ăL�[��������
			dlg.m_nTarget = uParam;
			dlg.m_nKey = 0;

			// �Y������Windows�L�[������ΐݒ�
			nPrev = -1;
			for (nWin=0; nWin<0x100; nWin++) {
				if (m_pEditMap[nWin] == uParam) {
					dlg.m_nKey = nWin;
					nPrev = nWin;
					break;
				}
			}

			// �_�C�A���O���s
			if (dlg.DoModal() != IDOK) {
				return 0;
			}

			// �L�[�}�b�v��ݒ�
			m_pEditMap[dlg.m_nKey] = uParam;
			if (nPrev >= 0) {
				m_pEditMap[nPrev] = 0;
			}

			// SHIFT�L�[��O����
			if (nPrev == DIK_LSHIFT) {
				m_pEditMap[DIK_RSHIFT] = 0;
			}
			if (dlg.m_nKey == DIK_LSHIFT) {
				m_pEditMap[DIK_RSHIFT] = uParam;
			}
			break;

		// ���{�^��������
		case WM_LBUTTONUP:
			break;

		// �E�{�^������
		case WM_RBUTTONDOWN:
			// �Y������Windows�L�[������ΐݒ�
			nPrev = -1;
			for (nWin=0; nWin<0x100; nWin++) {
				if (m_pEditMap[nWin] == uParam) {
					dlg.m_nKey = nWin;
					nPrev = nWin;
					break;
				}
			}
			if (nPrev < 0) {
				break;
			}

			// ���b�Z�[�W�{�b�N�X�ŁA�폜�̗L�����`�F�b�N
			::GetMsg(IDS_KBD_DELMSG, strName);
			strText.Format(strName, uParam, m_pInput->GetKeyID(nWin));
			::GetMsg(IDS_KBD_DELTITLE, strName);
			if (MessageBox(strText, strName, MB_ICONQUESTION | MB_YESNO) != IDYES) {
				break;
			}

			// �Y������Windows�L�[���폜
			m_pEditMap[nWin] = 0;

			// SHIFT�L�[��O����
			if (nWin == DIK_LSHIFT) {
				m_pEditMap[DIK_RSHIFT] = 0;
			}
			break;

		// �E�{�^��������
		case WM_RBUTTONUP:
			break;

		// �}�E�X�ړ�
		case WM_MOUSEMOVE:
			// �������b�Z�[�W�ݒ�
			strText = m_strGuide;

			// �L�[�Ƀt�H�[�J�X�����ꍇ
			if (uParam != 0) {
				// �܂�X68000�L�[��\��
				strText.Format(_T("Key%02X  "), uParam);
				strText += m_pInput->GetKeyName(uParam);

				// �Y������Windows�L�[������Βǉ�
				for (nWin=0; nWin<0x100; nWin++) {
					if (m_pEditMap[nWin] == uParam) {
						// Windows�L�[��������
						strName = m_pInput->GetKeyID(nWin);
						strText += _T("    (");
						strText += strName;
						strText += _T(")");
						break;
					}
				}
			}

			// ���b�Z�[�W�ݒ�
			m_strStat = strText;
			pDC = new CClientDC(this);
			OnDraw(pDC);
			delete pDC;
			break;

		// ���̑�(���肦�Ȃ�)
		default:
			ASSERT(FALSE);
			break;
	}

	return 0;
}

//===========================================================================
//
//	Dialogo de introduccion de datos
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	Constructor
//
//---------------------------------------------------------------------------
CKeyinDlg::CKeyinDlg(CWnd *pParent) : CDialog(IDD_KEYINDLG, pParent)
{
	CFrmWnd *pFrmWnd;

	// �p����ւ̑Ή�
	if (!::IsJapanese()) {
		m_lpszTemplateName = MAKEINTRESOURCE(IDD_US_KEYINDLG);
		m_nIDHelp = IDD_US_KEYINDLG;
	}

	// �C���v�b�g�擾
	pFrmWnd = (CFrmWnd*)AfxGetApp()->m_pMainWnd;
	ASSERT(pFrmWnd);
	m_pInput = pFrmWnd->GetInput();
	ASSERT(m_pInput);
}

//---------------------------------------------------------------------------
//
//	Mapa de mensajes
//
//---------------------------------------------------------------------------
#if !defined(WM_KICKIDLE)
#define WM_KICKIDLE		0x36a
#endif	// WM_KICKIDLE
BEGIN_MESSAGE_MAP(CKeyinDlg, CDialog)
	ON_WM_PAINT()
	ON_WM_GETDLGCODE()
	ON_MESSAGE(WM_KICKIDLE, OnKickIdle)
	ON_WM_RBUTTONDOWN()
END_MESSAGE_MAP()

//---------------------------------------------------------------------------
//
//	Inicializacion del dialogo
//
//---------------------------------------------------------------------------
BOOL CKeyinDlg::OnInitDialog()
{
	CStatic *pStatic;
	CString string;

	// ��{�N���X
	CDialog::OnInitDialog();

	// IME�I�t
	::ImmAssociateContext(m_hWnd, (HIMC)NULL);

	// ����L�[���o�b�t�@��
	ASSERT(m_pInput);
	m_pInput->GetKeyBuf(m_bKey);

	// �K�C�h��`�̏���
	pStatic = (CStatic*)GetDlgItem(IDC_KEYIN_LABEL);
	ASSERT(pStatic);
	pStatic->GetWindowText(string);
	m_GuideString.Format(string, m_nTarget);
	pStatic->GetWindowRect(&m_GuideRect);
	ScreenToClient(&m_GuideRect);
	pStatic->DestroyWindow();

	// ���蓖�ċ�`�̏���
	pStatic = (CStatic*)GetDlgItem(IDC_KEYIN_STATIC);
	ASSERT(pStatic);
	pStatic->GetWindowText(m_AssignString);
	pStatic->GetWindowRect(&m_AssignRect);
	ScreenToClient(&m_AssignRect);
	pStatic->DestroyWindow();

	// �L�[��`�̏���
	pStatic = (CStatic*)GetDlgItem(IDC_KEYIN_KEY);
	ASSERT(pStatic);
	pStatic->GetWindowText(m_KeyString);
	if (m_nKey != 0) {
		m_KeyString = m_pInput->GetKeyID(m_nKey);
	}
	pStatic->GetWindowRect(&m_KeyRect);
	ScreenToClient(&m_KeyRect);
	pStatic->DestroyWindow();

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	OK
//
//---------------------------------------------------------------------------
void CKeyinDlg::OnOK()
{
	// [CR]�ɂ��I����}��
}

//---------------------------------------------------------------------------
//
//	Obtenga el codigo de dialogo
//
//---------------------------------------------------------------------------
UINT CKeyinDlg::OnGetDlgCode()
{
	// �L�[���b�Z�[�W���󂯎���悤�ɂ���
	return DLGC_WANTMESSAGE;
}

//---------------------------------------------------------------------------
//
//	Dibujar
//
//---------------------------------------------------------------------------
void CKeyinDlg::OnPaint()
{
	CPaintDC dc(this);
	CDC mDC;
	CRect rect;
	CBitmap Bitmap;
	CBitmap *pBitmap;

	// ������DC�쐬
	VERIFY(mDC.CreateCompatibleDC(&dc));

	// �݊��r�b�g�}�b�v�쐬
	GetClientRect(&rect);
	VERIFY(Bitmap.CreateCompatibleBitmap(&dc, rect.Width(), rect.Height()));
	pBitmap = mDC.SelectObject(&Bitmap);
	ASSERT(pBitmap);

	// �w�i�N���A
	mDC.FillSolidRect(&rect, ::GetSysColor(COLOR_3DFACE));

	// �`��
	OnDraw(&mDC);

	// BitBlt
	VERIFY(dc.BitBlt(0, 0, rect.Width(), rect.Height(), &mDC, 0, 0, SRCCOPY));

	// �r�b�g�}�b�v�I��
	VERIFY(mDC.SelectObject(pBitmap));
	VERIFY(Bitmap.DeleteObject());

	// ������DC�I��
	VERIFY(mDC.DeleteDC());
}

//---------------------------------------------------------------------------
//
//	Subcontratacion de dibujos
//
//---------------------------------------------------------------------------
void FASTCALL CKeyinDlg::OnDraw(CDC *pDC)
{
	CFont *pFont;

	ASSERT(this);
	ASSERT(pDC);

	// �F�ݒ�
	pDC->SetTextColor(::GetSysColor(COLOR_BTNTEXT));
	pDC->SetBkColor(::GetSysColor(COLOR_3DFACE));

	// �t�H���g�ݒ�
	pFont = (CFont*)pDC->SelectStockObject(DEFAULT_GUI_FONT);
	ASSERT(pFont);

	// �\��
	pDC->DrawText(m_GuideString, m_GuideRect,
					DT_LEFT | DT_NOPREFIX | DT_WORDBREAK);
	pDC->DrawText(m_AssignString, m_AssignRect,
					DT_LEFT | DT_NOPREFIX | DT_SINGLELINE | DT_VCENTER);
	pDC->DrawText(m_KeyString, m_KeyRect,
					DT_LEFT | DT_NOPREFIX | DT_SINGLELINE | DT_VCENTER);

	// �t�H���g�߂�(�I�u�W�F�N�g�͍폜���Ȃ��Ă悢)
	pDC->SelectObject(pFont);
}

//---------------------------------------------------------------------------
//
//	Idolos
//
//---------------------------------------------------------------------------
LONG CKeyinDlg::OnKickIdle(UINT /*uParam*/, LONG /*lParam*/)
{
	BOOL bKey[0x100];
	int i;
	UINT nOld;

	// ���L�[�L��
	nOld = m_nKey;

	// DirectInput�o�R�ŁA�L�[���󂯎��
	m_pInput->GetKeyBuf(bKey);

	// �L�[����
	for (i=0; i<0x100; i++) {
		// �����O���葝���Ă���L�[������΁A�����ݒ�
		if (!m_bKey[i] && bKey[i]) {
			m_nKey = (UINT)i;
		}

		// �R�s�[
		m_bKey[i] = bKey[i];
	}

	// SHIFT�L�[��O����
	if (m_nKey == DIK_RSHIFT) {
		m_nKey = DIK_LSHIFT;
	}

	// ��v���Ă���Ες��Ȃ��ėǂ�
	if (m_nKey == nOld) {
		return 0;
	}

	// �L�[���̂�\��
	m_KeyString = m_pInput->GetKeyID(m_nKey);
	Invalidate(FALSE);

	return 0;
}

//---------------------------------------------------------------------------
//
//	Mantener pulsado el boton derecho
//
//---------------------------------------------------------------------------
void CKeyinDlg::OnRButtonDown(UINT /*nFlags*/, CPoint /*point*/)
{
	// �_�C�A���O�I��
	EndDialog(IDOK);
}

//===========================================================================
//
//	Pagina del raton
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	Constructor
//
//---------------------------------------------------------------------------
CMousePage::CMousePage()
{
	// ID,Help��K���ݒ�
	m_dwID = MAKEID('M', 'O', 'U', 'S');
	m_nTemplate = IDD_MOUSEPAGE;
	m_uHelpID = IDC_MOUSE_HELP;
}

//---------------------------------------------------------------------------
//
//	Mapa de mensajes
//
//---------------------------------------------------------------------------
BEGIN_MESSAGE_MAP(CMousePage, CConfigPage)
	ON_WM_HSCROLL()
	ON_BN_CLICKED(IDC_MOUSE_NPORT, OnPort)
	ON_BN_CLICKED(IDC_MOUSE_FPORT, OnPort)
	ON_BN_CLICKED(IDC_MOUSE_KPORT, OnPort)
END_MESSAGE_MAP()

//---------------------------------------------------------------------------
//
//	Inicializacion
//
//---------------------------------------------------------------------------
BOOL CMousePage::OnInitDialog()
{
	CSliderCtrl *pSlider;
	CStatic *pStatic;
	CButton *pButton;
	CString strText;
	UINT nID;

	// ��{�N���X
	CConfigPage::OnInitDialog();

	// ���x
	pSlider = (CSliderCtrl*)GetDlgItem(IDC_MOUSE_SLIDER);
	ASSERT(pSlider);
	pSlider->SetRange(0, 512);
	pSlider->SetPos(m_pConfig->mouse_speed);

	// ���x�e�L�X�g
	strText.Format(_T("%d%%"), (m_pConfig->mouse_speed * 100) >> 8);
	pStatic = (CStatic*)GetDlgItem(IDC_MOUSE_PARS);
	pStatic->SetWindowText(strText);

	// �ڑ���|�[�g
	nID = IDC_MOUSE_NPORT;
	switch (m_pConfig->mouse_port) {
		// �ڑ����Ȃ�
		case 0:
			break;
		// SCC
		case 1:
			nID = IDC_MOUSE_FPORT;
			break;
		// �L�[�{�[�h
		case 2:
			nID = IDC_MOUSE_KPORT;
			break;
		// ���̑�(���肦�Ȃ�)
		default:
			ASSERT(FALSE);
			break;
	}
	pButton = (CButton*)GetDlgItem(nID);
	ASSERT(pButton);
	pButton->SetCheck(1);

	// �I�v�V����
	pButton = (CButton*)GetDlgItem(IDC_MOUSE_SWAPB);
	ASSERT(pButton);
	pButton->SetCheck(m_pConfig->mouse_swap);
	pButton = (CButton*)GetDlgItem(IDC_MOUSE_MIDB);
	ASSERT(pButton);
	pButton->SetCheck(!m_pConfig->mouse_mid);
	pButton = (CButton*)GetDlgItem(IDC_MOUSE_TBG);
	ASSERT(pButton);
	pButton->SetCheck(m_pConfig->mouse_trackb);

	// �R���g���[���L���E����
	m_bEnableCtrl = TRUE;
	if (m_pConfig->mouse_port == 0) {
		EnableControls(FALSE);
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	Ok
//
//---------------------------------------------------------------------------
void CMousePage::OnOK()
{
	CSliderCtrl *pSlider;
	CButton *pButton;

	// ���x
	pSlider = (CSliderCtrl*)GetDlgItem(IDC_MOUSE_SLIDER);
	ASSERT(pSlider);
	m_pConfig->mouse_speed = pSlider->GetPos();

	// �ڑ��|�[�g
	pButton = (CButton*)GetDlgItem(IDC_MOUSE_NPORT);
	ASSERT(pButton);
	if (pButton->GetCheck()) {
		m_pConfig->mouse_port = 0;
	}
	pButton = (CButton*)GetDlgItem(IDC_MOUSE_FPORT);
	ASSERT(pButton);
	if (pButton->GetCheck()) {
		m_pConfig->mouse_port = 1;
	}
	pButton = (CButton*)GetDlgItem(IDC_MOUSE_KPORT);
	ASSERT(pButton);
	if (pButton->GetCheck()) {
		m_pConfig->mouse_port = 2;
	}

	// �I�v�V����
	pButton = (CButton*)GetDlgItem(IDC_MOUSE_SWAPB);
	ASSERT(pButton);
	m_pConfig->mouse_swap = pButton->GetCheck();
	pButton = (CButton*)GetDlgItem(IDC_MOUSE_MIDB);
	ASSERT(pButton);
	m_pConfig->mouse_mid = !(pButton->GetCheck());
	pButton = (CButton*)GetDlgItem(IDC_MOUSE_TBG);
	ASSERT(pButton);
	m_pConfig->mouse_trackb = pButton->GetCheck();

	// ��{�N���X
	CConfigPage::OnOK();
}

//---------------------------------------------------------------------------
//
//	Desplazamiento horizontal
//
//---------------------------------------------------------------------------
void CMousePage::OnHScroll(UINT /*nSBCode*/, UINT /*nPos*/, CScrollBar *pBar)
{
	CSliderCtrl *pSlider;
	CStatic *pStatic;
	CString strText;

	// �ϊ��A�`�F�b�N
	pSlider = (CSliderCtrl*)pBar;
	if (pSlider->GetDlgCtrlID() != IDC_MOUSE_SLIDER) {
		return;
	}

	// �\��
	strText.Format(_T("%d%%"), (pSlider->GetPos() * 100) >> 8);
	pStatic = (CStatic*)GetDlgItem(IDC_MOUSE_PARS);
	pStatic->SetWindowText(strText);
}

//---------------------------------------------------------------------------
//
//	Seleccion de puertos
//
//---------------------------------------------------------------------------
void CMousePage::OnPort()
{
	CButton *pButton;

	// �{�^���擾
	pButton = (CButton*)GetDlgItem(IDC_MOUSE_NPORT);
	ASSERT(pButton);

	// �ڑ����Ȃ� or ���̃|�[�g�Ŕ���
	if (pButton->GetCheck()) {
		EnableControls(FALSE);
	}
	else {
		EnableControls(TRUE);
	}
}

//---------------------------------------------------------------------------
//
//	Cambio de estado de control
//
//---------------------------------------------------------------------------
void FASTCALL CMousePage::EnableControls(BOOL bEnable) 
{
	int i;
	CWnd *pWnd;

	ASSERT(this);

	// �t���O�`�F�b�N
	if (m_bEnableCtrl == bEnable) {
		return;
	}
	m_bEnableCtrl = bEnable;

	// �{�[�hID�AHelp�ȊO�̑S�R���g���[����ݒ�
	for(i=0; ; i++) {
		// �I���`�F�b�N
		if (ControlTable[i] == NULL) {
			break;
		}

		// �R���g���[���擾
		pWnd = GetDlgItem(ControlTable[i]);
		ASSERT(pWnd);
		pWnd->EnableWindow(bEnable);
	}
}

//---------------------------------------------------------------------------
//
//	Tabla de ID de control
//
//---------------------------------------------------------------------------
const UINT CMousePage::ControlTable[] = {
	IDC_MOUSE_SPEEDG,
	IDC_MOUSE_SLIDER,
	IDC_MOUSE_PARS,
	IDC_MOUSE_OPTG,
	IDC_MOUSE_SWAPB,
	IDC_MOUSE_MIDB,
	IDC_MOUSE_TBG,
	NULL
};

//===========================================================================
//
//	Pagina del joystick
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	Constructor
//
//---------------------------------------------------------------------------
CJoyPage::CJoyPage()
{
	CFrmWnd *pFrmWnd;

	// ID,Help��K���ݒ�
	m_dwID = MAKEID('J', 'O', 'Y', ' ');
	m_nTemplate = IDD_JOYPAGE;
	m_uHelpID = IDC_JOY_HELP;

	// �C���v�b�g�擾
	pFrmWnd = (CFrmWnd*)AfxGetApp()->m_pMainWnd;
	ASSERT(pFrmWnd);
	m_pInput = pFrmWnd->GetInput();
	ASSERT(m_pInput);
}

//---------------------------------------------------------------------------
//
//	Inicializar
//
//---------------------------------------------------------------------------
BOOL CJoyPage::OnInitDialog()
{
	int i;
	int nDevice;
	CString strNoA;
	CString strDesc;
	DIDEVCAPS ddc;
	CComboBox *pComboBox;

	ASSERT(this);
	ASSERT(m_pInput);

	// ��{�N���X
	CConfigPage::OnInitDialog();

	// No Assign������擾
	::GetMsg(IDS_JOY_NOASSIGN, strNoA);

	// Cuadro combinado de puertos
	for (i=0; i<2; i++) {
		// Obtener cuadro combinado
		if (i == 0) {
			pComboBox = (CComboBox*)GetDlgItem(IDC_JOY_PORTC1);
		}
		else {
			pComboBox = (CComboBox*)GetDlgItem(IDC_JOY_PORTC2);
		}
		ASSERT(pComboBox);

		// Borrar
		pComboBox->ResetContent();

		// Establecer secuencialmente las cadenas
		pComboBox->AddString(strNoA);
		::GetMsg(IDS_JOY_ATARI, strDesc);
		pComboBox->AddString(strDesc);
		::GetMsg(IDS_JOY_ATARISS, strDesc);
		pComboBox->AddString(strDesc);
		::GetMsg(IDS_JOY_CYBERA, strDesc);
		pComboBox->AddString(strDesc);
		::GetMsg(IDS_JOY_CYBERD, strDesc);
		pComboBox->AddString(strDesc);
		::GetMsg(IDS_JOY_MD3, strDesc);
		pComboBox->AddString(strDesc);
		::GetMsg(IDS_JOY_MD6, strDesc);
		pComboBox->AddString(strDesc);
		::GetMsg(IDS_JOY_CPSF, strDesc);
		pComboBox->AddString(strDesc);
		::GetMsg(IDS_JOY_CPSFMD, strDesc);
		pComboBox->AddString(strDesc);
		::GetMsg(IDS_JOY_MAGICAL, strDesc);
		pComboBox->AddString(strDesc);
		::GetMsg(IDS_JOY_LR, strDesc);
		pComboBox->AddString(strDesc);
		::GetMsg(IDS_JOY_PACLAND, strDesc);
		pComboBox->AddString(strDesc);
		::GetMsg(IDS_JOY_BM, strDesc);
		pComboBox->AddString(strDesc);

		// Cursor
		pComboBox->SetCurSel(m_pConfig->joy_type[i]);

		// Inicializacion de los botones correspondientes
		OnSelChg(pComboBox);
	}

	// Cuadro combinado de dispositivos
	for (i=0; i<2; i++) {
		// Obtener cuadro combinado
		if (i == 0) {
			pComboBox = (CComboBox*)GetDlgItem(IDC_JOY_DEVCA);
		}
		else {
			pComboBox = (CComboBox*)GetDlgItem(IDC_JOY_DEVCB);
		}
		ASSERT(pComboBox);

		// Borrar
		pComboBox->ResetContent();

		// No hay ajuste de asignacion
		pComboBox->AddString(strNoA);

		// Bucle de dispositivos
		for (nDevice=0; ; nDevice++) {
			if (!m_pInput->GetJoyCaps(nDevice, strDesc, &ddc)) {
				// No hay mas dispositivos.
				break;
			}

			// �ǉ�
			pComboBox->AddString(strDesc);
		}

		// Cursor en el elemento de ajuste
		if (m_pConfig->joy_dev[i] < pComboBox->GetCount()) {
			pComboBox->SetCurSel(m_pConfig->joy_dev[i]);
		}
		else {
			// El valor ajustado supera el numero de dispositivos presentes -> Enfocar en No asignar
			pComboBox->SetCurSel(0);
		}

		// Inicializacion de los botones correspondientes
		OnSelChg(pComboBox);
	}

	// Dispositivos de teclado
	pComboBox = (CComboBox*)GetDlgItem(IDC_JOY_DEVCC);
	ASSERT(pComboBox);
	pComboBox->ResetContent();
	pComboBox->AddString(strNoA);
	pComboBox->SetCurSel(0);
	OnSelChg(pComboBox);

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	Decision OK
//
//---------------------------------------------------------------------------
void CJoyPage::OnOK()
{
	int i;
	int nButton;
	CComboBox *pComboBox;
	CInput::JOYCFG cfg;

	ASSERT(this);

	// Cuadro combinado de puertos
	for (i=0; i<2; i++) {
		// �R���{�{�b�N�X�擾
		if (i == 0) {
			pComboBox = (CComboBox*)GetDlgItem(IDC_JOY_PORTC1);
		}
		else {
			pComboBox = (CComboBox*)GetDlgItem(IDC_JOY_PORTC2);
		}

		// �ݒ�l�𓾂�
		m_pConfig->joy_type[i] = pComboBox->GetCurSel();
	}

	// Cuadro combinado de dispositivos
	for (i=0; i<2; i++) {
		// �R���{�{�b�N�X�擾
		if (i == 0) {
			pComboBox = (CComboBox*)GetDlgItem(IDC_JOY_DEVCA);
		}
		else {
			pComboBox = (CComboBox*)GetDlgItem(IDC_JOY_DEVCB);
		}
		ASSERT(pComboBox);

		// Obtener el valor establecido
		m_pConfig->joy_dev[i] = pComboBox->GetCurSel();
	}

	// Los ejes y los botones crean m_pConfig basandose en la configuracion actual
	for (i=0; i<CInput::JoyDevices; i++) {
		// Leer la configuracion de funcionamiento actual
		m_pInput->GetJoyCfg(i, &cfg);

		// Botones
		for (nButton=0; nButton<CInput::JoyButtons; nButton++) {
			// Combinacion de asignacion y fuego continuo
			if (i == 0) {
				// Puerto 1
				m_pConfig->joy_button0[nButton] = 
						cfg.dwButton[nButton] | (cfg.dwRapid[nButton] << 8);
			}
			else {
				// Puerto 2
				m_pConfig->joy_button1[nButton] = 
						cfg.dwButton[nButton] | (cfg.dwRapid[nButton] << 8);
			}
		}
	}

	// Clase basica
	CConfigPage::OnOK();
}

//---------------------------------------------------------------------------
//
//	Cancelar
//
//---------------------------------------------------------------------------
void CJoyPage::OnCancel()
{
	// AplicarCfg a CInput de forma independiente (revertir la configuracion al estado no editado)
	m_pInput->ApplyCfg(m_pConfig);

	// ��{�N���X
	CConfigPage::OnCancel();
}

//---------------------------------------------------------------------------
//
//	Notificacion de comandos
//
//---------------------------------------------------------------------------
BOOL CJoyPage::OnCommand(WPARAM wParam, LPARAM lParam)
{
	CComboBox *pComboBox;
	UINT nID;

	ASSERT(this);

	// ���M��ID�擾
	nID = (UINT)LOWORD(wParam);

	// CBN_SELCHANGE
	if (HIWORD(wParam) == CBN_SELCHANGE) {
		pComboBox = (CComboBox*)GetDlgItem(nID);
		ASSERT(pComboBox);

		// ��p���[�`��
		OnSelChg(pComboBox);
		return TRUE;
	}

	// BN_CLICKED
	if (HIWORD(wParam) == BN_CLICKED) {
		if ((nID == IDC_JOY_PORTD1) || (nID == IDC_JOY_PORTD2)) {
			// �|�[�g���ڍ�
			OnDetail(nID);
		}
		else {
			// �f�o�C�X���ݒ�
			OnSetting(nID);
		}
		return TRUE;
	}

	// ��{�N���X
	return CConfigPage::OnCommand(wParam, lParam);
}

//---------------------------------------------------------------------------
//
//	Cambios en el cuadro combinado
//
//---------------------------------------------------------------------------
void FASTCALL CJoyPage::OnSelChg(CComboBox *pComboBox)
{
	CButton *pButton;

	ASSERT(this);
	ASSERT(pComboBox);

	// Obtener el boton correspondiente
	pButton = GetCorButton(pComboBox->GetDlgCtrlID());
	if (!pButton) {
		return;
	}

	// Decidido por la seleccion del cuadro combinado
	if (pComboBox->GetCurSel() == 0) {
		// (sin asignacion) -> boton desactivado
		pButton->EnableWindow(FALSE);
	}
	else {
		// Boton activado
		pButton->EnableWindow(TRUE);
	}
}

//---------------------------------------------------------------------------
//
//	Detalles del puerto
//
//---------------------------------------------------------------------------
void FASTCALL CJoyPage::OnDetail(UINT nButton)
{
	CComboBox *pComboBox;
	int nPort;
	int nType;
	CString strDesc;
	CJoyDetDlg dlg(this);

	ASSERT(this);
	ASSERT(nButton != 0);

	// �|�[�g�擾
	nPort = 0;
	if (nButton == IDC_JOY_PORTD2) {
		nPort++;
	}

	// �Ή�����R���{�{�b�N�X�𓾂�
	pComboBox = GetCorCombo(nButton);
	if (!pComboBox) {
		return;
	}

	// �I��ԍ��𓾂�
	nType = pComboBox->GetCurSel();
	if (nType == 0) {
		return;
	}

	// �I��ԍ�����A���̂��擾
	pComboBox->GetLBText(nType, strDesc);

	// �p�����[�^��n���A�_�C�A���O���s
	dlg.m_strDesc = strDesc;
	dlg.m_nPort = nPort;
	dlg.m_nType = nType;
	dlg.DoModal();
}

//---------------------------------------------------------------------------
//
//	Ajustes del dispositivo
//
//---------------------------------------------------------------------------
void FASTCALL CJoyPage::OnSetting(UINT nButton)
{
	CComboBox *pComboBox;
	CJoySheet sheet(this);
	CInput::JOYCFG cfg;
	int nJoy;
	int nCombo;
	int nType[PPI::PortMax];

	ASSERT(this);
	ASSERT(nButton != 0);

	// �Ή�����R���{�{�b�N�X�𓾂�
	pComboBox = GetCorCombo(nButton);
	if (!pComboBox) {
		return;
	}

	// �f�o�C�X�C���f�b�N�X�擾
	nJoy = -1;
	switch (pComboBox->GetDlgCtrlID()) {
		// �f�o�C�XA
		case IDC_JOY_DEVCA:
			nJoy = 0;
			break;

		// �f�o�C�XB
		case IDC_JOY_DEVCB:
			nJoy = 1;
			break;

		// ���̑�(�Q�[���R���g���[���ł͂Ȃ��f�o�C�X)
		default:
			return;
	}
	ASSERT((nJoy == 0) || (nJoy == 1));
	ASSERT(nJoy < CInput::JoyDevices);

	// �R���{�{�b�N�X�̑I��ԍ��𓾂�
	nCombo = pComboBox->GetCurSel();
	if (nCombo == 0) {
		// ���蓖�Ė���
		return;
	}

	// �R���{�{�b�N�X�̑I��ԍ��𓾂�B0(���蓖�Ė���)�����e
	pComboBox = (CComboBox*)GetDlgItem(IDC_JOY_PORTC1);
	ASSERT(pComboBox);
	nType[0] = pComboBox->GetCurSel();
	pComboBox = (CComboBox*)GetDlgItem(IDC_JOY_PORTC2);
	ASSERT(pComboBox);
	nType[1] = pComboBox->GetCurSel();

	// ���݂̃W���C�X�e�B�b�N�ݒ��ۑ�
	m_pInput->GetJoyCfg(nJoy, &cfg);

	// �p�����[�^�ݒ�
	sheet.SetParam(nJoy, nCombo, nType);

	// �_�C�A���O���s(�W���C�X�e�B�b�N�؂�ւ������݁A�L�����Z���Ȃ�ݒ�߂�)
	m_pInput->EnableJoy(FALSE);
	if (sheet.DoModal() != IDOK) {
		m_pInput->SetJoyCfg(nJoy, &cfg);
	}
	m_pInput->EnableJoy(TRUE);
}

//---------------------------------------------------------------------------
//
//	Obtener el boton correspondiente
//
//---------------------------------------------------------------------------
CButton* CJoyPage::GetCorButton(UINT nComboBox)
{
	int i;
	CButton *pButton;

	ASSERT(this);
	ASSERT(nComboBox != 0);

	pButton = NULL;

	// �R���g���[���e�[�u��������
	for (i=0; ; i+=2) {
		// �I�[�`�F�b�N
		if (ControlTable[i] == NULL) {
			return NULL;
		}

		// ��v���Ă���΁Aok
		if (ControlTable[i] == nComboBox) {
			// �Ή�����{�^���𓾂�
			pButton = (CButton*)GetDlgItem(ControlTable[i + 1]);
			break;
		}
	}

	ASSERT(pButton);
	return pButton;
}

//---------------------------------------------------------------------------
//
//	Obtener el cuadro combinado correspondiente
//
//---------------------------------------------------------------------------
CComboBox* CJoyPage::GetCorCombo(UINT nButton)
{
	int i;
	CComboBox *pComboBox;

	ASSERT(this);
	ASSERT(nButton != 0);

	pComboBox = NULL;

	// �R���g���[���e�[�u��������
	for (i=1; ; i+=2) {
		// �I�[�`�F�b�N
		if (ControlTable[i] == NULL) {
			return NULL;
		}

		// ��v���Ă���΁Aok
		if (ControlTable[i] == nButton) {
			// �Ή�����R���{�{�b�N�X�𓾂�
			pComboBox = (CComboBox*)GetDlgItem(ControlTable[i - 1]);
			break;
		}
	}

	ASSERT(pComboBox);
	return pComboBox;
}

//---------------------------------------------------------------------------
//
//	Tabla de control
//	Para asignar cuadros combinados y botones entre si
//
//---------------------------------------------------------------------------
UINT CJoyPage::ControlTable[] = {
	IDC_JOY_PORTC1, IDC_JOY_PORTD1,
	IDC_JOY_PORTC2, IDC_JOY_PORTD2,
	IDC_JOY_DEVCA, IDC_JOY_DEVAA,
	IDC_JOY_DEVCB, IDC_JOY_DEVAB,
	IDC_JOY_DEVCC, IDC_JOY_DEVAC,
	NULL, NULL
};

//===========================================================================
//
//	Dialogo de detalles del joystick
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	Constructor
//
//---------------------------------------------------------------------------
CJoyDetDlg::CJoyDetDlg(CWnd *pParent) : CDialog(IDD_JOYDETDLG, pParent)
{
	// Adaptacion a un entorno anglofono
	if (!::IsJapanese()) {
		m_lpszTemplateName = MAKEINTRESOURCE(IDD_US_JOYDETDLG);
		m_nIDHelp = IDD_US_JOYDETDLG;
	}

	m_strDesc.Empty();
	m_nPort = -1;
	m_nType = 0;
}

//---------------------------------------------------------------------------
//
//	Inicializacion del dialogo
//
//---------------------------------------------------------------------------
BOOL CJoyDetDlg::OnInitDialog()
{
	CString strBase;
	CString strText;
	CStatic *pStatic;
	PPI *pPPI;
	JoyDevice *pDevice;

	// ��{�N���X
	CDialog::OnInitDialog();

	ASSERT(m_strDesc.GetLength() > 0);
	ASSERT((m_nPort >= 0) && (m_nPort < PPI::PortMax));
	ASSERT(m_nType >= 1);

	// �|�[�g��
	GetWindowText(strBase);
	strText.Format(strBase, m_nPort + 1);
	SetWindowText(strText);

	// ����
	pStatic = (CStatic*)GetDlgItem(IDC_JOYDET_NAMEL);
	ASSERT(pStatic);
	pStatic->SetWindowText(m_strDesc);

	// �W���C�X�e�B�b�N�쐬
	pPPI = (PPI*)::GetVM()->SearchDevice(MAKEID('P', 'P', 'I', ' '));
	ASSERT(pPPI);
	pDevice = pPPI->CreateJoy(m_nPort, m_nType);
	ASSERT(pDevice);

	// ����
	pStatic = (CStatic*)GetDlgItem(IDC_JOYDET_AXISS);
	ASSERT(pStatic);
	pStatic->GetWindowText(strBase);
	strText.Format(strBase, pDevice->GetAxes());
	pStatic->SetWindowText(strText);

	// �{�^����
	pStatic = (CStatic*)GetDlgItem(IDC_JOYDET_BUTTONS);
	ASSERT(pStatic);
	pStatic->GetWindowText(strBase);
	strText.Format(strBase, pDevice->GetButtons());
	pStatic->SetWindowText(strText);

	// �^�C�v(�A�i���O�E�f�W�^��)
	if (pDevice->IsAnalog()) {
		pStatic = (CStatic*)GetDlgItem(IDC_JOYDET_TYPES);
		::GetMsg(IDS_JOYDET_ANALOG, strText);
		pStatic->SetWindowText(strText);
	}

	// �f�[�^��
	pStatic = (CStatic*)GetDlgItem(IDC_JOYDET_DATASS);
	ASSERT(pStatic);
	pStatic->GetWindowText(strBase);
	strText.Format(strBase, pDevice->GetDatas());
	pStatic->SetWindowText(strText);

	// �W���C�X�e�B�b�N�폜
	delete pDevice;

	return TRUE;
}

//===========================================================================
//
//	Pagina de configuracion de los botones
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	Constructor
//
//---------------------------------------------------------------------------
CBtnSetPage::CBtnSetPage()
{
	CFrmWnd *pFrmWnd;
	int i;

#if defined(_DEBUG)
	ASSERT(CInput::JoyButtons <= (sizeof(m_bButton)/sizeof(BOOL)));
	ASSERT(CInput::JoyButtons <= (sizeof(m_rectLabel)/sizeof(CRect)));
#endif	// _DEBUG

	// �C���v�b�g�擾
	pFrmWnd = (CFrmWnd*)AfxGetApp()->m_pMainWnd;
	ASSERT(pFrmWnd);
	m_pInput = pFrmWnd->GetInput();
	ASSERT(m_pInput);

	// �W���C�X�e�B�b�N�ԍ����N���A
	m_nJoy = -1;

	// �^�C�v�ԍ����N���A
	for (i=0; i<PPI::PortMax; i++) {
		m_nType[i] = -1;
	}
}

//---------------------------------------------------------------------------
//
//	Mapa de mensajes
//
//---------------------------------------------------------------------------
BEGIN_MESSAGE_MAP(CBtnSetPage, CPropertyPage)
	ON_WM_PAINT()
	ON_WM_TIMER()
	ON_WM_HSCROLL()
END_MESSAGE_MAP()

//---------------------------------------------------------------------------
//
//	Crear
//
//---------------------------------------------------------------------------
void FASTCALL CBtnSetPage::Init(CPropertySheet *pSheet)
{
	int nID;

	ASSERT(this);

	// �e�V�[�g�L��
	ASSERT(pSheet);
	m_pSheet = pSheet;

	// ID����
	nID = IDD_BTNSETPAGE;
	if (!::IsJapanese()) {
		nID += 50;
	}

	// �\�z
	CommonConstruct(MAKEINTRESOURCE(nID), 0);

	// �e�V�[�g�ɒǉ�
	pSheet->AddPage(this);
}

//---------------------------------------------------------------------------
//
//	Inicializacion
//
//---------------------------------------------------------------------------
BOOL CBtnSetPage::OnInitDialog()
{
	CJoySheet *pJoySheet;
	int nButton;
	int nButtons;
	int nPort;
	int nCandidate;
	CStatic *pStatic;
	CComboBox *pComboBox;
	CSliderCtrl *pSlider;
	CString strText;
	CString strBase;
	CInput::JOYCFG cfg;
	DWORD dwData;
	PPI *pPPI;
	JoyDevice *pJoyDevice;
	CString strDesc;

	ASSERT(this);
	ASSERT(m_pSheet);

	// Clase basica
	CPropertyPage::OnInitDialog();

	// Inicializar la clase padre (CPropertySheet no tiene OnInitDialog)
	pJoySheet = (CJoySheet*)m_pSheet;
	pJoySheet->InitSheet();
	ASSERT((m_nJoy >= 0) && (m_nJoy < CInput::JoyDevices));

	// Obtener la configuracion actual del joystick
	m_pInput->GetJoyCfg(m_nJoy, &cfg);

	// Obtenga el PPI
	pPPI = (PPI*)::GetVM()->SearchDevice(MAKEID('P', 'P', 'I', ' '));
	ASSERT(pPPI);

	// Obtener el numero de botones
	nButtons = pJoySheet->GetButtons();

	// Obtener el texto base
	::GetMsg(IDS_JOYSET_BTNPORT, strBase);

	// Ajustes de control
	for (nButton=0; nButton<CInput::JoyButtons; nButton++) {
		// Etiquetas
		pStatic = (CStatic*)GetDlgItem(GetControl(nButton, BtnLabel));
		ASSERT(pStatic);
		if (nButton < nButtons) {
			// Activado (borrar ventana)
			pStatic->GetWindowRect(&m_rectLabel[nButton]);
			ScreenToClient(&m_rectLabel[nButton]);
			pStatic->DestroyWindow();
		}
		else {
			// Desactivado (ventana prohibida)
			m_rectLabel[nButton].top = 0;
			m_rectLabel[nButton].left = 0;
			pStatic->EnableWindow(FALSE);
		}

		// Cuadro combinado
		pComboBox = (CComboBox*)GetDlgItem(GetControl(nButton, BtnCombo));
		ASSERT(pComboBox);
		if (nButton < nButtons) {
			// Activado (anadir candidato)
			pComboBox->ResetContent();

			// Fijar No Asignar
			::GetMsg(IDS_JOYSET_NOASSIGN, strText);
			pComboBox->AddString(strText);

			// Puerto, boton de giro
			for (nPort=0; nPort<PPI::PortMax; nPort++) {
				// Consigue un dispositivo de joystick temporal
				pJoyDevice = pPPI->CreateJoy(0, m_nType[nPort]);

				for (nCandidate=0; nCandidate<PPI::ButtonMax; nCandidate++) {
					// Obtener el nombre del boton del dispositivo del joystick
					GetButtonDesc(pJoyDevice->GetButtonDesc(nCandidate), strDesc);

					// Formato
					strText.Format(strBase, nPort + 1, nCandidate + 1, strDesc);
					pComboBox->AddString(strText);
				}

				// Retirar el dispositivo temporal del joystick
				delete pJoyDevice;
			}

			// Configuracion del cursor
			pComboBox->SetCurSel(0);
			if ((LOWORD(cfg.dwButton[nButton]) != 0) && (LOWORD(cfg.dwButton[nButton]) <= PPI::ButtonMax)) {
				if (cfg.dwButton[nButton] & 0x10000) {
					// Puerto 2
					pComboBox->SetCurSel(LOWORD(cfg.dwButton[nButton]) + PPI::ButtonMax);
				}
				else {
					//Puerto 1
					pComboBox->SetCurSel(LOWORD(cfg.dwButton[nButton]));
				}
			}
		}
		else {
			// Desactivado (ventana prohibida)
			pComboBox->EnableWindow(FALSE);
		}

		// Deslizador de fuego continuo
		pSlider = (CSliderCtrl*)GetDlgItem(GetControl(nButton, BtnRapid));
		ASSERT(pSlider);
		if (nButton < nButtons) {
			// Habilitado (rango establecido y valor actual)
			pSlider->SetRange(0, CInput::JoyRapids);
			if (cfg.dwRapid[nButton] <= CInput::JoyRapids) {
				pSlider->SetPos(cfg.dwRapid[nButton]);
			}
		}
		else {
			// Desactivado (ventana prohibida)
			pSlider->EnableWindow(FALSE);
		}

		// Valor de fuego continuo
		pStatic = (CStatic*)GetDlgItem(GetControl(nButton, BtnValue));
		ASSERT(pStatic);
		if (nButton < nButtons) {
			// Activado (se muestra el valor por defecto)
			OnSlider(nButton);
			OnSelChg(nButton);
		}
		else {
			// Desactivado (claro)
			strText.Empty();
			pStatic->SetWindowText(strText);
		}
	}

	// Lectura del valor inicial del boton
	for (nButton=0; nButton<CInput::JoyButtons; nButton++) {
		m_bButton[nButton] = FALSE;
		dwData = m_pInput->GetJoyButton(m_nJoy, nButton);
		if ((dwData < 0x10000) && (dwData & 0x80)) {
			m_bButton[nButton] = TRUE;
		}
	}

	// Temporizador de inicio (se dispara en 100ms)
	m_nTimerID = SetTimer(IDD_BTNSETPAGE, 100, NULL);

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	�`��
//
//---------------------------------------------------------------------------
void CBtnSetPage::OnPaint()
{
	CPaintDC dc(this);

	// �`�惁�C��
	OnDraw(&dc, NULL, TRUE);
}

//---------------------------------------------------------------------------
//
//	�����X�N���[��
//
//---------------------------------------------------------------------------
void CBtnSetPage::OnHScroll(UINT /*nSBCode*/, UINT /*nPos*/, CScrollBar *pBar)
{
	CSliderCtrl *pSlider;
	UINT nID;
	int nButton;

	// �R���g���[��ID���擾
	pSlider = (CSliderCtrl*)pBar;
	nID = pSlider->GetDlgCtrlID();

	// �{�^���C���f�b�N�X������
	for (nButton=0; nButton<CInput::JoyButtons; nButton++) {
		if (GetControl(nButton, BtnRapid) == nID) {
			// ��p���[�`�����Ă�
			OnSlider(nButton);
			break;
		}
	}
}

//---------------------------------------------------------------------------
//
//	�R�}���h�ʒm
//
//---------------------------------------------------------------------------
BOOL CBtnSetPage::OnCommand(WPARAM wParam, LPARAM lParam)
{
	int nButton;
	UINT nID;

	ASSERT(this);

	// ���M��ID�擾
	nID = (UINT)LOWORD(wParam);

	// CBN_SELCHANGE
	if (HIWORD(wParam) == CBN_SELCHANGE) {
		// �{�^���C���f�b�N�X������
		for (nButton=0; nButton<CInput::JoyButtons; nButton++) {
			if (GetControl(nButton, BtnCombo) == nID) {
				OnSelChg(nButton);
				break;
			}
		}
	}

	// ��{�N���X
	return CPropertyPage::OnCommand(wParam, lParam);
}

//---------------------------------------------------------------------------
//
//	�`�惁�C��
//
//---------------------------------------------------------------------------
void FASTCALL CBtnSetPage::OnDraw(CDC *pDC, BOOL *pButton, BOOL bForce)
{
	int nButton;
	CFont *pFont;
	CString strBase;
	CString strText;

	ASSERT(this);
	ASSERT(pDC);

	// �F�ݒ�
	pDC->SetBkColor(::GetSysColor(COLOR_3DFACE));

	// �t�H���g�ݒ�
	pFont = (CFont*)pDC->SelectStockObject(DEFAULT_GUI_FONT);
	ASSERT(pFont);

	// �x�[�X��������擾
	::GetMsg(IDS_JOYSET_BTNLABEL, strBase);

	// �{�^�����[�v
	for (nButton=0; nButton<CInput::JoyButtons; nButton++) {
		// �L��(�\�����ׂ�)�{�^�����ۂ�
		if ((m_rectLabel[nButton].left == 0) && (m_rectLabel[nButton].top == 0)) {
			// �{�^�����Ȃ��̂ŁA�����ɂ��ꂽ�X�^�e�B�b�N�e�L�X�g
			continue;
		}

		// !bForce�Ȃ�A��r���Č���
		if (!bForce) {
			ASSERT(pButton);
			if (m_bButton[nButton] == pButton[nButton]) {
				// ��v���Ă���̂ŕ`�悵�Ȃ�
				continue;
			}
			// ����Ă���̂ŕۑ�
			m_bButton[nButton] = pButton[nButton];
		}

		// �F������
		if (m_bButton[nButton]) {
			// ������Ă���(�ԐF)
			pDC->SetTextColor(RGB(255, 0, 0));
		}
		else {
			// ������Ă��Ȃ�(���F)
			pDC->SetTextColor(::GetSysColor(COLOR_BTNTEXT));
		}

		// �\��
		strText.Format(strBase, nButton + 1);
		pDC->DrawText(strText, m_rectLabel[nButton],
						DT_LEFT | DT_NOPREFIX | DT_SINGLELINE | DT_VCENTER);
	}

	// �t�H���g�߂�(�I�u�W�F�N�g�͍폜���Ȃ��Ă悢)
	pDC->SelectObject(pFont);
}

//---------------------------------------------------------------------------
//
//	�^�C�}
//
//---------------------------------------------------------------------------
void CBtnSetPage::OnTimer(UINT /*nTimerID*/)
{
	int nButton;
	BOOL bButton[CInput::JoyButtons];
	BOOL bFlag;
	DWORD dwData;
	CClientDC *pDC;

	ASSERT(this);

	// �t���O������
	bFlag = FALSE;

	// ���݂̃W���C�X�e�B�b�N�{�^������ǂݎ��A��r
	for (nButton=0; nButton<CInput::JoyButtons; nButton++) {
		bButton[nButton] = FALSE;
		dwData = m_pInput->GetJoyButton(m_nJoy, nButton);
		if ((dwData < 0x10000) && (dwData & 0x80)) {
			bButton[nButton] = TRUE;
		}

		// ����Ă�����A�t���OUp
		if (m_bButton[nButton] != bButton[nButton]) {
			bFlag = TRUE;
		}
	}

	// �t���O���オ���Ă���΁A�ĕ`��
	if (bFlag) {
		pDC = new CClientDC(this);
		OnDraw(pDC, bButton, FALSE);
		delete pDC;
	}
}

//---------------------------------------------------------------------------
//
//	����
//
//---------------------------------------------------------------------------
void CBtnSetPage::OnOK()
{
	CJoySheet *pJoySheet;
	int nButtons;
	int nButton;
	CInput::JOYCFG cfg;
	CComboBox *pComboBox;
	CSliderCtrl *pSlider;
	int nSelect;

	// �^�C�}��~
	if (m_nTimerID) {
		KillTimer(m_nTimerID);
		m_nTimerID = NULL;
	}

	// �e�V�[�g���擾
	pJoySheet = (CJoySheet*)m_pSheet;
	nButtons = pJoySheet->GetButtons();

	// ���݂̐ݒ�f�[�^���擾
	m_pInput->GetJoyCfg(m_nJoy, &cfg);

	// �R���g���[����ǂݎ��A���݂̐ݒ�֔��f
	for (nButton=0; nButton<CInput::JoyButtons; nButton++) {
		// �L���ȃ{�^����
		if (nButton >= nButtons) {
			// �����ȃ{�^���Ȃ̂ŁA���蓖�āE�A�˂Ƃ���0
			cfg.dwButton[nButton] = 0;
			cfg.dwRapid[nButton] = 0;
			continue;
		}

		// ���蓖�ēǂݎ��
		pComboBox = (CComboBox*)GetDlgItem(GetControl(nButton, BtnCombo));
		ASSERT(pComboBox);
		nSelect = pComboBox->GetCurSel();

		// (���蓖�ĂȂ�)�`�F�b�N
		if (nSelect == 0) {
			// �������蓖�ĂȂ�A���蓖�āE�A�˂Ƃ���0
			cfg.dwButton[nButton] = 0;
			cfg.dwRapid[nButton] = 0;
			continue;
		}

		// �ʏ튄�蓖��
		nSelect--;
		if (nSelect >= PPI::ButtonMax) {
			// �|�[�g2
			cfg.dwButton[nButton] = (DWORD)(0x10000 | (nSelect - PPI::ButtonMax + 1));
		}
		else {
			// �|�[�g1
			cfg.dwButton[nButton] = (DWORD)(nSelect + 1);
		}

		// �A��
		pSlider = (CSliderCtrl*)GetDlgItem(GetControl(nButton, BtnRapid));
		ASSERT(pSlider);
		cfg.dwRapid[nButton] = pSlider->GetPos();
	}

	// �ݒ�f�[�^�𔽉f
	m_pInput->SetJoyCfg(m_nJoy, &cfg);

	// ��{�N���X
	CPropertyPage::OnOK();
}

//---------------------------------------------------------------------------
//
//	�L�����Z��
//
//---------------------------------------------------------------------------
void CBtnSetPage::OnCancel()
{
	// �^�C�}��~
	if (m_nTimerID) {
		KillTimer(m_nTimerID);
		m_nTimerID = NULL;
	}

	// ��{�N���X
	CPropertyPage::OnCancel();
}

//---------------------------------------------------------------------------
//
//	�X���C�_�ύX
//
//---------------------------------------------------------------------------
void FASTCALL CBtnSetPage::OnSlider(int nButton)
{
	CSliderCtrl *pSlider;
	CStatic *pStatic;
	CString strText;
	int nPos;

	ASSERT(this);
	ASSERT((nButton >= 0) && (nButton < CInput::JoyButtons));

	// �|�W�V�������擾
	pSlider = (CSliderCtrl*)GetDlgItem(GetControl(nButton, BtnRapid));
	ASSERT(pSlider);
	nPos = pSlider->GetPos();

	// �Ή����郉�x�����擾
	pStatic = (CStatic*)GetDlgItem(GetControl(nButton, BtnValue));
	ASSERT(pStatic);

	// �e�[�u������l���Z�b�g
	if ((nPos >= 0) && (nPos <= CInput::JoyRapids)) {
		// �Œ菬���_����
		if (RapidTable[nPos] & 1) {
			strText.Format(_T("%d.5"), RapidTable[nPos] >> 1);
		}
		else {
			strText.Format(_T("%d"), RapidTable[nPos] >> 1);
		}
	}
	else {
		strText.Empty();
	}
	pStatic->SetWindowText(strText);
}

//---------------------------------------------------------------------------
//
//	�R���{�{�b�N�X�ύX
//
//---------------------------------------------------------------------------
void FASTCALL CBtnSetPage::OnSelChg(int nButton)
{
	CComboBox *pComboBox;
	CSliderCtrl *pSlider;
	CStatic *pStatic;
	int nPos;

	ASSERT(this);
	ASSERT((nButton >= 0) && (nButton < CInput::JoyButtons));

	// �|�W�V�������擾
	pComboBox = (CComboBox*)GetDlgItem(GetControl(nButton, BtnCombo));
	ASSERT(pComboBox);
	nPos = pComboBox->GetCurSel();

	// �Ή�����X���C�_�A���x�����擾
	pSlider = (CSliderCtrl*)GetDlgItem(GetControl(nButton, BtnRapid));
	ASSERT(pSlider);
	pStatic = (CStatic*)GetDlgItem(GetControl(nButton, BtnValue));
	ASSERT(pStatic);

	// �E�B���h�E�̗L���E������ݒ�
	if (nPos == 0) {
		pSlider->EnableWindow(FALSE);
		pStatic->EnableWindow(FALSE);
	}
	else {
		pSlider->EnableWindow(TRUE);
		pStatic->EnableWindow(TRUE);
	}
}

//---------------------------------------------------------------------------
//
//	�{�^�����̎擾
//
//---------------------------------------------------------------------------
void FASTCALL CBtnSetPage::GetButtonDesc(const char *pszDesc, CString& strDesc)
{
	LPCTSTR lpszT;

	ASSERT(this);

	// ������
	strDesc.Empty();

	// NULL�Ȃ烊�^�[��
	if (!pszDesc) {
		return;
	}

	// TC�ɕϊ�
	lpszT = A2CT(pszDesc);

	// (�J�b�R)�����ĕ����񐶐�
	strDesc = _T("(");
	strDesc += lpszT;
	strDesc += _T(")");
}

//---------------------------------------------------------------------------
//
//	�R���g���[���擾
//
//---------------------------------------------------------------------------
UINT FASTCALL CBtnSetPage::GetControl(int nButton, CtrlType ctlType) const
{
	int nType;

	ASSERT(this);
	ASSERT((nButton >= 0) && (nButton < CInput::JoyButtons));

	// �^�C�v�擾
	nType = (int)ctlType;
	ASSERT((nType >= 0) && (nType < 4));

	// ID�擾
	return ControlTable[(nButton << 2) + nType];
}

//---------------------------------------------------------------------------
//
//	�R���g���[���e�[�u��
//
//---------------------------------------------------------------------------
const UINT CBtnSetPage::ControlTable[] = {
	IDC_BTNSET_BTNL1, IDC_BTNSET_ASSIGNC1, IDC_BTNSET_RAPIDC1, IDC_BTNSET_RAPIDL1,
	IDC_BTNSET_BTNL2, IDC_BTNSET_ASSIGNC2, IDC_BTNSET_RAPIDC2, IDC_BTNSET_RAPIDL2,
	IDC_BTNSET_BTNL3, IDC_BTNSET_ASSIGNC3, IDC_BTNSET_RAPIDC3, IDC_BTNSET_RAPIDL3,
	IDC_BTNSET_BTNL4, IDC_BTNSET_ASSIGNC4, IDC_BTNSET_RAPIDC4, IDC_BTNSET_RAPIDL4,
	IDC_BTNSET_BTNL5, IDC_BTNSET_ASSIGNC5, IDC_BTNSET_RAPIDC5, IDC_BTNSET_RAPIDL5,
	IDC_BTNSET_BTNL6, IDC_BTNSET_ASSIGNC6, IDC_BTNSET_RAPIDC6, IDC_BTNSET_RAPIDL6,
	IDC_BTNSET_BTNL7, IDC_BTNSET_ASSIGNC7, IDC_BTNSET_RAPIDC7, IDC_BTNSET_RAPIDL7,
	IDC_BTNSET_BTNL8, IDC_BTNSET_ASSIGNC8, IDC_BTNSET_RAPIDC8, IDC_BTNSET_RAPIDL8,
	IDC_BTNSET_BTNL9, IDC_BTNSET_ASSIGNC9, IDC_BTNSET_RAPIDC9, IDC_BTNSET_RAPIDL9,
	IDC_BTNSET_BTNL10,IDC_BTNSET_ASSIGNC10,IDC_BTNSET_RAPIDC10,IDC_BTNSET_RAPIDL10,
	IDC_BTNSET_BTNL11,IDC_BTNSET_ASSIGNC11,IDC_BTNSET_RAPIDC11,IDC_BTNSET_RAPIDL11,
	IDC_BTNSET_BTNL12,IDC_BTNSET_ASSIGNC12,IDC_BTNSET_RAPIDC12,IDC_BTNSET_RAPIDL12
};

//---------------------------------------------------------------------------
//
//	�A�˃e�[�u��
//	���Œ菬���_�����̂��߁A2�{���Ă���
//
//---------------------------------------------------------------------------
const int CBtnSetPage::RapidTable[CInput::JoyRapids + 1] = {
	0,
	4,
	6,
	8,
	10,
	15,
	20,
	24,
	30,
	40,
	60
};

//===========================================================================
//
//	�W���C�X�e�B�b�N�v���p�e�B�V�[�g
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	�R���X�g���N�^
//
//---------------------------------------------------------------------------
CJoySheet::CJoySheet(CWnd *pParent) : CPropertySheet(IDS_JOYSET, pParent)
{
	CFrmWnd *pFrmWnd;
	int i;

	// �p����ւ̑Ή�
	if (!::IsJapanese()) {
		::GetMsg(IDS_JOYSET, m_strCaption);
	}

	// Apply�{�^�����폜
	m_psh.dwFlags |= PSH_NOAPPLYNOW;

	// CInput�擾
	pFrmWnd = (CFrmWnd*)AfxGetApp()->m_pMainWnd;
	ASSERT(pFrmWnd);
	m_pInput = pFrmWnd->GetInput();
	ASSERT(m_pInput);

	// �p�����[�^������
	m_nJoy = -1;
	m_nCombo = -1;
	for (i=0; i<PPI::PortMax; i++) {
		m_nType[i] = -1;
	}

	// �y�[�W������
	m_BtnSet.Init(this);
}

//---------------------------------------------------------------------------
//
//	�p�����[�^�ݒ�
//
//---------------------------------------------------------------------------
void FASTCALL CJoySheet::SetParam(int nJoy, int nCombo, int nType[])
{
	int i;

	ASSERT(this);
	ASSERT((nJoy == 0) || (nJoy == 1));
	ASSERT(nJoy < CInput::JoyDevices);
	ASSERT(nCombo >= 1);
	ASSERT(nType);

	// �L��(�R���{�{�b�N�X��-1)
	m_nJoy = nJoy;
	m_nCombo = nCombo - 1;
	for (i=0; i<PPI::PortMax; i++) {
		m_nType[i] = nType[i];
	}

	// Caps�N���A
	memset(&m_DevCaps, 0, sizeof(m_DevCaps));
}

//---------------------------------------------------------------------------
//
//	�V�[�g������
//
//---------------------------------------------------------------------------
void FASTCALL CJoySheet::InitSheet()
{
	int i;
	CString strDesc;
	CString strFmt;
	CString strText;

	ASSERT(this);
	ASSERT(m_pInput);
	ASSERT((m_nJoy == 0) || (m_nJoy == 1));
	ASSERT(m_nJoy < CInput::JoyDevices);
	ASSERT(m_nCombo >= 0);

	// �f�o�C�XCaps�擾
	m_pInput->GetJoyCaps(m_nCombo, strDesc, &m_DevCaps);

	// �E�B���h�E�e�L�X�g�ҏW
	GetWindowText(strFmt);
	strText.Format(strFmt, _T('A' + m_nJoy), strDesc);
	SetWindowText(strText);

	// �e�y�[�W�Ƀp�����[�^�z�M
	m_BtnSet.m_nJoy = m_nJoy;
	for (i=0; i<PPI::PortMax; i++) {
		m_BtnSet.m_nType[i] = m_nType[i];
	}
}

//---------------------------------------------------------------------------
//
//	�����擾
//
//---------------------------------------------------------------------------
int FASTCALL CJoySheet::GetAxes() const
{
	ASSERT(this);

	return (int)m_DevCaps.dwAxes;
}

//---------------------------------------------------------------------------
//
//	�{�^�����擾
//
//---------------------------------------------------------------------------
int FASTCALL CJoySheet::GetButtons() const
{
	ASSERT(this);

	return (int)m_DevCaps.dwButtons;
}

//===========================================================================
//
//	SASI�y�[�W
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	�R���X�g���N�^
//
//---------------------------------------------------------------------------
CSASIPage::CSASIPage()
{
	int i;

	// ID,Help��K���ݒ�
	m_dwID = MAKEID('S', 'A', 'S', 'I');
	m_nTemplate = IDD_SASIPAGE;
	m_uHelpID = IDC_SASI_HELP;

	// SASI�f�o�C�X�擾
	m_pSASI = (SASI*)::GetVM()->SearchDevice(MAKEID('S', 'A', 'S', 'I'));
	ASSERT(m_pSASI);

	// ��������
	m_bInit = FALSE;
	m_nDrives = -1;

	ASSERT(SASI::SASIMax <= 16);
	for (i=0; i<SASI::SASIMax; i++) {
		m_szFile[i][0] = _T('\0');
	}
}

//---------------------------------------------------------------------------
//
//	���b�Z�[�W �}�b�v
//
//---------------------------------------------------------------------------
BEGIN_MESSAGE_MAP(CSASIPage, CConfigPage)
	ON_WM_VSCROLL()
	ON_NOTIFY(NM_CLICK, IDC_SASI_LIST, OnClick)
END_MESSAGE_MAP()

//---------------------------------------------------------------------------
//
//	������
//
//---------------------------------------------------------------------------
BOOL CSASIPage::OnInitDialog()
{
	CSpinButtonCtrl *pSpin;
	CButton *pButton;
	CListCtrl *pListCtrl;
	CClientDC *pDC;
	CString strCaption;
	CString strFile;
	TEXTMETRIC tm;
	LONG cx;
	int i;

	// ��{�N���X
	CConfigPage::OnInitDialog();

	// �������t���OUp�A�h���C�u���擾
	m_bInit = TRUE;
	m_nDrives = m_pConfig->sasi_drives;
	ASSERT((m_nDrives >= 0) && (m_nDrives <= SASI::SASIMax));

	// �����񃍁[�h
	::GetMsg(IDS_SASI_DEVERROR, m_strError);

	// �h���C�u��
	pSpin = (CSpinButtonCtrl*)GetDlgItem(IDC_SASI_DRIVES);
	ASSERT(pSpin);
	pSpin->SetBase(10);
	pSpin->SetRange(0, SASI::SASIMax);
	pSpin->SetPos(m_nDrives);

	// �������X�C�b�`�����X�V
	pButton = (CButton*)GetDlgItem(IDC_SASI_MEMSWB);
	ASSERT(pButton);
	if (m_pConfig->sasi_sramsync) {
		pButton->SetCheck(1);
	}
	else {
		pButton->SetCheck(0);
	}

	// �t�@�C�����擾
	for (i=0; i<SASI::SASIMax; i++) {
		_tcscpy(m_szFile[i], m_pConfig->sasi_file[i]);
	}

	// �e�L�X�g���g���b�N�𓾂�
	pDC = new CClientDC(this);
	::GetTextMetrics(pDC->m_hDC, &tm);
	delete pDC;
	cx = tm.tmAveCharWidth;

	// ���X�g�R���g���[���ݒ�
	pListCtrl = (CListCtrl*)GetDlgItem(IDC_SASI_LIST);
	ASSERT(pListCtrl);
	pListCtrl->DeleteAllItems();
	::GetMsg(IDS_SASI_CAPACITY, strCaption);
	::GetMsg(IDS_SASI_FILENAME, strFile);
	if (::IsJapanese()) {
		pListCtrl->InsertColumn(0, _T("No."), LVCFMT_LEFT, cx * 4, 0);
	}
	else {
		pListCtrl->InsertColumn(0, _T("No."), LVCFMT_LEFT, cx * 5, 0);
	}
	pListCtrl->InsertColumn(1, strCaption, LVCFMT_CENTER,  cx * 6, 0);
	pListCtrl->InsertColumn(2, strFile, LVCFMT_LEFT, cx * 28, 0);

	// ���X�g�R���g���[��1�s�S�̃I�v�V����(COMCTL32.DLL v4.71�ȍ~)
	pListCtrl->SendMessage(LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_FULLROWSELECT ); //| LVS_EX_LABELTIP

	// ���X�g�R���g���[���X�V
	UpdateList();

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	�y�[�W�A�N�e�B�u
//
//---------------------------------------------------------------------------
BOOL CSASIPage::OnSetActive()
{
	CSpinButtonCtrl *pSpin;
	CSCSIPage *pSCSIPage;
	BOOL bEnable;

	// ��{�N���X
	if (!CConfigPage::OnSetActive()) {
		return FALSE;
	}

	// SCSI�C���^�t�F�[�X�𓮓I�Ɏ擾
	ASSERT(m_pSheet);
	pSCSIPage = (CSCSIPage*)m_pSheet->SearchPage(MAKEID('S', 'C', 'S', 'I'));
	ASSERT(pSCSIPage);
	if (pSCSIPage->GetInterface(m_pConfig) == 2) {
		// ����SCSI�C���^�t�F�[�X(SASI�͎g�p�ł��Ȃ�)
		bEnable = FALSE;
	}
	else {
		// SASI�܂��͊O�tSCSI�C���^�t�F�[�X
		bEnable = TRUE;
	}

	// �R���g���[���L���E����
	if (bEnable) {
		// �L���̏ꍇ�A�X�s���{�^�����猻�݂̃h���C�u�����擾
		pSpin = (CSpinButtonCtrl*)GetDlgItem(IDC_SASI_DRIVES);
		ASSERT(pSpin);
		if (pSpin->GetPos() > 0 ) {
			// ���X�g�L���E�h���C�u�L��
			EnableControls(TRUE, TRUE);
		}
		else {
			// ���X�g�����E�h���C�u�L��
			EnableControls(FALSE, TRUE);
		}
	}
	else {
		// ���X�g�����E�h���C�u����
		EnableControls(FALSE, FALSE);
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	����
//
//---------------------------------------------------------------------------
void CSASIPage::OnOK()
{
	int i;
	TCHAR szPath[FILEPATH_MAX];
	CButton *pButton;
	CListCtrl *pListCtrl;

	// �h���C�u��
	ASSERT((m_nDrives >= 0) && (m_nDrives <= SASI::SASIMax));
	m_pConfig->sasi_drives = m_nDrives;

	// �t�@�C����
	pListCtrl = (CListCtrl*)GetDlgItem(IDC_SASI_LIST);
	ASSERT(pListCtrl);
	for (i=0; i<m_nDrives; i++) {
		pListCtrl->GetItemText(i, 2, szPath, FILEPATH_MAX);
		_tcscpy(m_pConfig->sasi_file[i], szPath);
	}

	// �`�F�b�N�{�b�N�X(SASI�ESCSI�Ƃ����ʐݒ�)
	pButton = (CButton*)GetDlgItem(IDC_SASI_MEMSWB);
	ASSERT(pButton);
	if (pButton->GetCheck() == 1) {
		m_pConfig->sasi_sramsync = TRUE;
		m_pConfig->scsi_sramsync = TRUE;
	}
	else {
		m_pConfig->sasi_sramsync = FALSE;
		m_pConfig->scsi_sramsync = FALSE;
	}

	// ��{�N���X
	CConfigPage::OnOK();
}

//---------------------------------------------------------------------------
//
//	�c�X�N���[��
//
//---------------------------------------------------------------------------
void CSASIPage::OnVScroll(UINT /*nSBCode*/, UINT nPos, CScrollBar* /*pBar*/)
{
	ASSERT(this);
	ASSERT(nPos <= SASI::SASIMax);

	// �h���C�u���X�V
	m_nDrives = nPos;

	// �R���g���[���L���E����
	if (m_nDrives > 0) {
		EnableControls(TRUE);
	}
	else {
		EnableControls(FALSE);
	}

	// ���X�g�R���g���[���X�V
	UpdateList();
}

//---------------------------------------------------------------------------
//
//	���X�g�R���g���[���N���b�N
//
//---------------------------------------------------------------------------
void CSASIPage::OnClick(NMHDR* /*pNMHDR*/, LRESULT* /*pResult*/)
{
	CListCtrl *pListCtrl;
	int i;
	int nID;
	int nCount;
	TCHAR szPath[FILEPATH_MAX];

	// ���X�g�R���g���[���擾
	pListCtrl = (CListCtrl*)GetDlgItem(IDC_SASI_LIST);
	ASSERT(pListCtrl);

	// �J�E���g�����擾
	nCount = pListCtrl->GetItemCount();

	// �Z���N�g����Ă���ID���擾
	nID = -1;
	for (i=0; i<nCount; i++) {
		if (pListCtrl->GetItemState(i, LVIS_SELECTED)) {
			nID = i;
			break;
		}
	}
	if (nID < 0) {
		return;
	}

	// �I�[�v�������݂�
	_tcscpy(szPath, m_szFile[nID]);
	if (!::FileOpenDlg(this, szPath, IDS_SASIOPEN)) {
		return;
	}

	// �p�X���X�V
	_tcscpy(m_szFile[nID], szPath);

	// ���X�g�R���g���[���X�V
	UpdateList();
}

//---------------------------------------------------------------------------
//
//	���X�g�R���g���[���X�V
//
//---------------------------------------------------------------------------
void FASTCALL CSASIPage::UpdateList()
{
	CListCtrl *pListCtrl;
	int nCount;
	int i;
	CString strID;
	CString strDisk;
	CString strCtrl;
	DWORD dwDisk[SASI::SASIMax];

	// ���X�g�R���g���[���̌��ݐ����擾
	pListCtrl = (CListCtrl*)GetDlgItem(IDC_SASI_LIST);
	ASSERT(pListCtrl);
	nCount = pListCtrl->GetItemCount();

	// ���X�g�R���g���[���̕��������ꍇ�A�㔼�����
	while (nCount > m_nDrives) {
		pListCtrl->DeleteItem(nCount - 1);
		nCount--;
	}

	// ���X�g�R���g���[��������Ȃ������́A�ǉ�����
	while (m_nDrives > nCount) {
		strID.Format(_T("%d"), nCount + 1);
		pListCtrl->InsertItem(nCount, strID);
		nCount++;
	}

	// ���f�B�`�F�b�N(m_nDrive�����܂Ƃ߂čs�Ȃ�)
	CheckSASI(dwDisk);

	// ��r���[�v
	for (i=0; i<nCount; i++) {
		// ���f�B�`�F�b�N�̌��ʂɂ��A������쐬
		if (dwDisk[i] == 0) {
			// �s��
			strDisk = m_strError;
		}
		else {
			// MB�\��
			strDisk.Format(_T("%uMB"), dwDisk[i]);
		}

		// ��r����уZ�b�g
		strCtrl = pListCtrl->GetItemText(i, 1);
		if (strDisk != strCtrl) {
			pListCtrl->SetItemText(i, 1, strDisk);
		}

		// �t�@�C����
		strDisk = m_szFile[i];
		strCtrl = pListCtrl->GetItemText(i, 2);
		if (strDisk != strCtrl) {
			pListCtrl->SetItemText(i, 2, strDisk);
		}
	}
}

//---------------------------------------------------------------------------
//
//	SASI�h���C�u�`�F�b�N
//
//---------------------------------------------------------------------------
void FASTCALL CSASIPage::CheckSASI(DWORD *pDisk)
{
	int i;
	DWORD dwSize;
	Fileio fio;

	ASSERT(this);
	ASSERT(pDisk);

	// VM���b�N
	::LockVM();

	// �h���C�u���[�v
	for (i=0; i<m_nDrives; i++) {
		// �T�C�Y0
		pDisk[i] = 0;

		// �I�[�v�������݂�
		if (!fio.Open(m_szFile[i], Fileio::ReadOnly)) {
			continue;
		}

		// �T�C�Y�擾�A�N���[�Y
		dwSize = fio.GetFileSize();
		fio.Close();

		// �T�C�Y�`�F�b�N
		switch (dwSize) {
			// 10MB
			case 0x9f5400:
				pDisk[i] = 10;
				break;

			// 20MB
			case 0x13c9800:
				pDisk[i] = 20;
				break;

			// 40MB
			case 0x2793000:
				pDisk[i] = 40;
				break;

			default:
				break;
		}
	}

	// �A�����b�N
	::UnlockVM();
}

//---------------------------------------------------------------------------
//
//	SASI�h���C�u���擾
//
//---------------------------------------------------------------------------
int FASTCALL CSASIPage::GetDrives(const Config *pConfig) const
{
	ASSERT(this);
	ASSERT(pConfig);

	// ����������Ă��Ȃ���΁A�^����ꂽConfig����
	if (!m_bInit) {
		return pConfig->sasi_drives;
	}

	// �������ς݂Ȃ�A���݂̒l��
	return m_nDrives;
}

//---------------------------------------------------------------------------
//
//	�R���g���[����ԕύX
//
//---------------------------------------------------------------------------
void FASTCALL CSASIPage::EnableControls(BOOL bEnable, BOOL bDrive)
{
	CListCtrl *pListCtrl;
	CWnd *pWnd;

	ASSERT(this);

	// ���X�g�R���g���[��(bEnable)
	pListCtrl = (CListCtrl*)GetDlgItem(IDC_SASI_LIST);
	ASSERT(pListCtrl);
	pListCtrl->EnableWindow(bEnable);

	// �h���C�u��(bDrive)
	pWnd = GetDlgItem(IDC_SASI_DRIVEL);
	ASSERT(pWnd);
	pWnd->EnableWindow(bDrive);
	pWnd = GetDlgItem(IDC_SASI_DRIVEE);
	ASSERT(pWnd);
	pWnd->EnableWindow(bDrive);
	pWnd = GetDlgItem(IDC_SASI_DRIVES);
	ASSERT(pWnd);
	pWnd->EnableWindow(bDrive);
}

//===========================================================================
//
//	SxSI�y�[�W
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	�R���X�g���N�^
//
//---------------------------------------------------------------------------
CSxSIPage::CSxSIPage()
{
	int i;

	// ID,Help��K���ݒ�
	m_dwID = MAKEID('S', 'X', 'S', 'I');
	m_nTemplate = IDD_SXSIPAGE;
	m_uHelpID = IDC_SXSI_HELP;

	// ������(���̑��f�[�^)
	m_nSASIDrives = 0;
	for (i=0; i<8; i++) {
		m_DevMap[i] = DevNone;
	}
	ASSERT(SASI::SCSIMax == 6);
	for (i=0; i<SASI::SCSIMax; i++) {
		m_szFile[i][0] = _T('\0');
	}

	// ��������
	m_bInit = FALSE;
}

//---------------------------------------------------------------------------
//
//	���b�Z�[�W �}�b�v
//
//---------------------------------------------------------------------------
BEGIN_MESSAGE_MAP(CSxSIPage, CConfigPage)
	ON_WM_VSCROLL()
	ON_NOTIFY(NM_CLICK, IDC_SXSI_LIST, OnClick)
	ON_BN_CLICKED(IDC_SXSI_MOCHECK, OnCheck)
END_MESSAGE_MAP()

//---------------------------------------------------------------------------
//
//	�y�[�W������
//
//---------------------------------------------------------------------------
BOOL CSxSIPage::OnInitDialog()
{
	int i;
	int nMax;
	int nDrives;
	CSASIPage *pSASIPage;
	CSpinButtonCtrl *pSpin;
	CButton *pButton;
	CListCtrl *pListCtrl;
	CDC *pDC;
	TEXTMETRIC tm;
	LONG cx;
	CString strCap;
	CString strFile;

	// ��{�N���X
	CConfigPage::OnInitDialog();

	// �������t���OUp
	m_bInit = TRUE;

	// SASI�y�[�W�擾
	ASSERT(m_pSheet);
	pSASIPage = (CSASIPage*)m_pSheet->SearchPage(MAKEID('S', 'A', 'S', 'I'));
	ASSERT(pSASIPage);

	// SASI�̐ݒ�h���C�u������ASCSI�ɐݒ�ł���ő�h���C�u���𓾂�
	m_nSASIDrives = pSASIPage->GetDrives(m_pConfig);
	nMax = m_nSASIDrives;
	nMax = (nMax + 1) >> 1;
	ASSERT((nMax >= 0) && (nMax <= 8));
	if (nMax >= 7) {
		nMax = 0;
	}
	else {
		nMax = 7 - nMax;
	}

	// SCSI�̍ő�h���C�u���𐧌�
	pSpin = (CSpinButtonCtrl*)GetDlgItem(IDC_SXSI_DRIVES);
	pSpin->SetBase(10);
	nDrives = m_pConfig->sxsi_drives;
	if (nDrives > nMax) {
		nDrives = nMax;
	}
	pSpin->SetRange(0, (short)nMax);
	pSpin->SetPos(nDrives);

	// SCSI�̃t�@�C�������擾
	for (i=0; i<6; i++) {
		_tcscpy(m_szFile[i], m_pConfig->sxsi_file[i]);
	}

	// MO�D��t���O�ݒ�
	pButton = (CButton*)GetDlgItem(IDC_SXSI_MOCHECK);
	if (m_pConfig->sxsi_mofirst) {
		pButton->SetCheck(1);
	}
	else {
		pButton->SetCheck(0);
	}

	// �e�L�X�g���g���b�N�𓾂�
	pDC = new CClientDC(this);
	::GetTextMetrics(pDC->m_hDC, &tm);
	delete pDC;
	cx = tm.tmAveCharWidth;

	// ���X�g�R���g���[���ݒ�
	pListCtrl = (CListCtrl*)GetDlgItem(IDC_SXSI_LIST);
	ASSERT(pListCtrl);
	pListCtrl->DeleteAllItems();
	if (::IsJapanese()) {
		pListCtrl->InsertColumn(0, _T("ID"), LVCFMT_LEFT, cx * 3, 0);
	}
	else {
		pListCtrl->InsertColumn(0, _T("ID"), LVCFMT_LEFT, cx * 4, 0);
	}
	::GetMsg(IDS_SXSI_CAPACITY, strCap);
	pListCtrl->InsertColumn(1, strCap, LVCFMT_CENTER,  cx * 7, 0);
	::GetMsg(IDS_SXSI_FILENAME, strFile);
	pListCtrl->InsertColumn(2, strFile, LVCFMT_LEFT, cx * 26, 0);

	// ���X�g�R���g���[��1�s�S�̃I�v�V����(COMCTL32.DLL v4.71�ȍ~)
	pListCtrl->SendMessage(LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_FULLROWSELECT ); //| LVS_EX_LABELTIP

	// ���X�g�R���g���[���Ŏg����������擾
	::GetMsg(IDS_SXSI_SASI, m_strSASI);
	::GetMsg(IDS_SXSI_MO, m_strMO);
	::GetMsg(IDS_SXSI_INIT, m_strInit);
	::GetMsg(IDS_SXSI_NONE, m_strNone);
	::GetMsg(IDS_SXSI_DEVERROR, m_strError);

	// ���X�g�R���g���[���X�V
	UpdateList();

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	�y�[�W�A�N�e�B�u
//
//---------------------------------------------------------------------------
BOOL CSxSIPage::OnSetActive()
{
	int nMax;
	int nPos;
	CSpinButtonCtrl *pSpin;
	BOOL bEnable;
	CSASIPage *pSASIPage;
	CSCSIPage *pSCSIPage;
	CAlterPage *pAlterPage;

	// ��{�N���X
	if (!CConfigPage::OnSetActive()) {
		return FALSE;
	}

	// �y�[�W�擾
	ASSERT(m_pSheet);
	pSASIPage = (CSASIPage*)m_pSheet->SearchPage(MAKEID('S', 'A', 'S', 'I'));
	ASSERT(pSASIPage);
	pSCSIPage = (CSCSIPage*)m_pSheet->SearchPage(MAKEID('S', 'C', 'S', 'I'));
	ASSERT(pSCSIPage);
	pAlterPage = (CAlterPage*)m_pSheet->SearchPage(MAKEID('A', 'L', 'T', ' '));
	ASSERT(pAlterPage);

	// SxSI�C�l�[�u���t���O�𓮓I�Ɏ擾
	bEnable = TRUE;
	if (!pAlterPage->HasParity(m_pConfig)) {
		// �p���e�B��ݒ肵�Ȃ��BSxSI�͎g�p�ł��Ȃ�
		bEnable = FALSE;
	}
	if (pSCSIPage->GetInterface(m_pConfig) != 0) {
		// �����܂��͊O�tSCSI�C���^�t�F�[�X�BSxSI�͎g�p�ł��Ȃ�
		bEnable = FALSE;
	}

	// SASI�̃h���C�u�����擾���ASCSI�̍ő�h���C�u���𓾂�
	m_nSASIDrives = pSASIPage->GetDrives(m_pConfig);
	nMax = m_nSASIDrives;
	nMax = (nMax + 1) >> 1;
	ASSERT((nMax >= 0) && (nMax <= 8));
	if (nMax >= 7) {
		nMax = 0;
	}
	else {
		nMax = 7 - nMax;
	}

	// SCSI�̍ő�h���C�u���𐧌�
	pSpin = (CSpinButtonCtrl*)GetDlgItem(IDC_SXSI_DRIVES);
	ASSERT(pSpin);
	nPos = LOWORD(pSpin->GetPos());
	if (nPos > nMax) {
		nPos = nMax;
		pSpin->SetPos(nPos);
	}
	pSpin->SetRange(0, (short)nMax);

	// ���X�g�R���g���[���X�V
	UpdateList();

	// �R���g���[���L���E����
	if (bEnable) {
		if (nPos > 0) {
			// ���X�g�L���E�h���C�u�L��
			EnableControls(TRUE, TRUE);
		}
		else {
			// ���X�g�L���E�h���C�u����
			EnableControls(FALSE, TRUE);
		}
	}
	else {
		// ���X�g�����E�h���C�u����
		EnableControls(FALSE, FALSE);
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	�c�X�N���[��
//
//---------------------------------------------------------------------------
void CSxSIPage::OnVScroll(UINT /*nSBCode*/, UINT nPos, CScrollBar* /*pBar*/)
{
	// ���X�g�R���g���[���X�V(������BuildMap���s��)
	UpdateList();

	// �R���g���[���L���E����
	if (nPos > 0) {
		EnableControls(TRUE);
	}
	else {
		EnableControls(FALSE);
	}
}

//---------------------------------------------------------------------------
//
//	���X�g�R���g���[���N���b�N
//
//---------------------------------------------------------------------------
void CSxSIPage::OnClick(NMHDR* /*pNMHDR*/, LRESULT* /*pResult*/)
{
	CListCtrl *pListCtrl;
	int i;
	int nID;
	int nCount;
	int nDrive;
	TCHAR szPath[FILEPATH_MAX];

	// ���X�g�R���g���[���擾
	pListCtrl = (CListCtrl*)GetDlgItem(IDC_SXSI_LIST);
	ASSERT(pListCtrl);

	// �J�E���g�����擾
	nCount = pListCtrl->GetItemCount();

	// �Z���N�g����Ă���ID���擾
	nID = -1;
	for (i=0; i<nCount; i++) {
		if (pListCtrl->GetItemState(i, LVIS_SELECTED)) {
			nID = i;
			break;
		}
	}
	if (nID < 0) {
		return;
	}

	// �}�b�v�����āA�^�C�v�𔻕�
	if (m_DevMap[nID] != DevSCSI) {
		return;
	}

	// ID����h���C�u�C���f�b�N�X�擾(MO�͍l�����Ȃ�)
	nDrive = 0;
	for (i=0; i<8; i++) {
		if (i == nID) {
			break;
		}
		if (m_DevMap[i] == DevSCSI) {
			nDrive++;
		}
	}
	ASSERT((nDrive >= 0) && (nDrive < SASI::SCSIMax));

	// �I�[�v�������݂�
	_tcscpy(szPath, m_szFile[nDrive]);
	if (!::FileOpenDlg(this, szPath, IDS_SCSIOPEN)) {
		return;
	}

	// �p�X���X�V
	_tcscpy(m_szFile[nDrive], szPath);

	// ���X�g�R���g���[���X�V
	UpdateList();
}

//---------------------------------------------------------------------------
//
//	�`�F�b�N�{�b�N�X�ύX
//
//---------------------------------------------------------------------------
void CSxSIPage::OnCheck()
{
	// ���X�g�R���g���[���X�V(������BuildMap���s��)
	UpdateList();
}

//---------------------------------------------------------------------------
//
//	����
//
//---------------------------------------------------------------------------
void CSxSIPage::OnOK()
{
	CSpinButtonCtrl *pSpin;
	CButton *pButton;
	int i;

	// �h���C�u��
	pSpin = (CSpinButtonCtrl*)GetDlgItem(IDC_SXSI_DRIVES);
	ASSERT(pSpin);
	m_pConfig->sxsi_drives = LOWORD(pSpin->GetPos());

	// MO�D��t���O
	pButton = (CButton*)GetDlgItem(IDC_SXSI_MOCHECK);
	ASSERT(pButton);
	if (pButton->GetCheck() == 1) {
		m_pConfig->sxsi_mofirst = TRUE;
	}
	else {
		m_pConfig->sxsi_mofirst = FALSE;
	}

	// �t�@�C����
	for (i=0; i<SASI::SCSIMax; i++) {
		_tcscpy(m_pConfig->sxsi_file[i], m_szFile[i]);
	}

	// ��{�N���X
	CConfigPage::OnOK();
}

//---------------------------------------------------------------------------
//
//	���X�g�R���g���[���X�V
//
//---------------------------------------------------------------------------
void FASTCALL CSxSIPage::UpdateList()
{
	int i;
	int nDrive;
	int nDev;
	int nCount;
	int nCap;
	CListCtrl *pListCtrl;
	CString strCtrl;
	CString strID;
	CString strSize;
	CString strFile;

	ASSERT(this);

	// �}�b�v���r���h
	BuildMap();

	// ���X�g�R���g���[���擾�A�J�E���g�擾
	pListCtrl = (CListCtrl*)GetDlgItem(IDC_SXSI_LIST);
	ASSERT(pListCtrl);
	nCount = pListCtrl->GetItemCount();

	// �}�b�v�̂���None�łȂ����̂̐��𐔂���
	nDev = 0;
	for (i=0; i<8; i++) {
		if (m_DevMap[i] != DevNone) {
			nDev++;
		}
	}

	// nDev�����A�C�e��������
	while (nCount > nDev) {
		pListCtrl->DeleteItem(nCount - 1);
		nCount--;
	}
	while (nDev > nCount) {
		strID.Format(_T("%d"), nCount + 1);
		pListCtrl->InsertItem(nCount, strID);
		nCount++;
	}

	// ��r���[�v
	nDrive = 0;
	nDev = 0;
	for (i=0; i<8; i++) {
		// �^�C�v�ɉ����ĕ���������
		switch (m_DevMap[i]) {
			// SASI �n�[�h�f�B�X�N
			case DevSASI:
				strSize = m_strNone;
				strFile = m_strSASI;
				break;

			// SCSI �n�[�h�f�B�X�N
			case DevSCSI:
				nCap = CheckSCSI(nDrive);
				if (nCap > 0) {
					strSize.Format("%dMB", nCap);
				}
				else {
					strSize = m_strError;
				}
				strFile = m_szFile[nDrive];
				nDrive++;
				break;

			// SCSI MO�f�B�X�N
			case DevMO:
				strSize = m_strNone;
				strFile = m_strMO;
				break;

			// �C�j�V�G�[�^(�z�X�g)
			case DevInit:
				strSize = m_strNone;
				strFile = m_strInit;
				break;

			// �f�o�C�X�Ȃ�
			case DevNone:
				// ���ɐi��
				continue;

			// ���̑�(���蓾�Ȃ�)
			default:
				ASSERT(FALSE);
				return;
		}

		// ID
		strID.Format(_T("%d"), i);
		strCtrl = pListCtrl->GetItemText(nDev, 0);
		if (strID != strCtrl) {
			pListCtrl->SetItemText(nDev, 0, strID);
		}

		// �e��
		strCtrl = pListCtrl->GetItemText(nDev, 1);
		if (strSize != strCtrl) {
			pListCtrl->SetItemText(nDev, 1, strSize);
		}

		// �t�@�C����
		strCtrl = pListCtrl->GetItemText(nDev, 2);
		if (strFile != strCtrl) {
			pListCtrl->SetItemText(nDev, 2, strFile);
		}

		// ����
		nDev++;
	}
}

//---------------------------------------------------------------------------
//
//	�}�b�v�쐬
//
//---------------------------------------------------------------------------
void FASTCALL CSxSIPage::BuildMap()
{
	int nSASI;
	int nMO;
	int nSCSI;
	int nInit;
	int nMax;
	int nID;
	int i;
	BOOL bMOFirst;
	CButton *pButton;
	CSpinButtonCtrl *pSpin;

	ASSERT(this);

	// ������
	nSASI = 0;
	nMO = 0;
	nSCSI = 0;
	nInit = 0;

	// MO�D��t���O���擾
	pButton = (CButton*)GetDlgItem(IDC_SXSI_MOCHECK);
	ASSERT(pButton);
	bMOFirst = FALSE;
	if (pButton->GetCheck() != 0) {
		bMOFirst = TRUE;
	}

	// SASI�h���C�u������ASASI�̐�LID���𓾂�
	ASSERT((m_nSASIDrives >= 0) && (m_nSASIDrives <= 0x10));
	nSASI = m_nSASIDrives;
	nSASI = (nSASI + 1) >> 1;

	// SASI����AMO,SCSI,INIT�̍ő吔�𓾂�
	if (nSASI <= 6) {
		nMO = 1;
		nSCSI = 6 - nSASI;
	}
	if (nSASI <= 7) {
		nInit = 1;
	}

	// SxSI�h���C�u���̐ݒ�����āA�l�𒲐�
	pSpin = (CSpinButtonCtrl*)GetDlgItem(IDC_SXSI_DRIVES);
	ASSERT(pSpin);
	nMax = LOWORD(pSpin->GetPos());
	ASSERT((nMax >= 0) && (nMax <= (nSCSI + nMO)));
	if (nMax == 0) {
		// SxSI�h���C�u����0
		nMO = 0;
		nSCSI = 0;
	}
	else {
		// �Ƃ肠����nSCSI��HD+MO���W�߂�
		nSCSI = nMax;

		// 1�̏ꍇ��MO�̂�
		if (nMax == 1) {
			nMO = 1;
			nSCSI = 0;
		}
		else {
			// 2�ȏ�̏ꍇ�́A1��MO�Ɋ��蓖�Ă�
			nSCSI--;
			nMO = 1;
		}
	}

	// ID�����Z�b�g
	nID = 0;

	// �I�[���N���A
	for (i=0; i<8; i++) {
		m_DevMap[i] = DevNone;
	}

	// SASI���Z�b�g
	for (i=0; i<nSASI; i++) {
		m_DevMap[nID] = DevSASI;
		nID++;
	}

	// SCSI,MO�Z�b�g
	if (bMOFirst) {
		// MO�D��
		for (i=0; i<nMO; i++) {
			m_DevMap[nID] = DevMO;
			nID++;
		}
		for (i=0; i<nSCSI; i++) {
			m_DevMap[nID] = DevSCSI;
			nID++;
		}
	}
	else {
		// HD�D��
		for (i=0; i<nSCSI; i++) {
			m_DevMap[nID] = DevSCSI;
			nID++;
		}
		for (i=0; i<nMO; i++) {
			m_DevMap[nID] = DevMO;
			nID++;
		}
	}

	// �C�j�V�G�[�^�Z�b�g
	for (i=0; i<nInit; i++) {
		ASSERT(nID <= 7);
		m_DevMap[7] = DevInit;
	}
}

//---------------------------------------------------------------------------
//
//	SCSI�n�[�h�f�B�X�N�e�ʃ`�F�b�N
//	���f�o�C�X�G���[��0��Ԃ�
//
//---------------------------------------------------------------------------
int FASTCALL CSxSIPage::CheckSCSI(int nDrive)
{
	Fileio fio;
	DWORD dwSize;

	ASSERT(this);
	ASSERT((nDrive >= 0) && (nDrive <= 5));

	// ���b�N
	::LockVM();

	// �t�@�C���I�[�v��
	if (!fio.Open(m_szFile[nDrive], Fileio::ReadOnly)) {
		// �G���[�Ȃ̂�0��Ԃ�
		fio.Close();
		::UnlockVM();
		return 0;
	}

	// �e�ʎ擾
	dwSize = fio.GetFileSize();

	// �A�����b�N
	fio.Close();
	::UnlockVM();

	// �t�@�C���T�C�Y���`�F�b�N(512�o�C�g�P��)
	if ((dwSize & 0x1ff) != 0) {
		return 0;
	}

	// �t�@�C���T�C�Y���`�F�b�N(10MB�ȏ�)
	if (dwSize < 10 * 0x400 * 0x400) {
		return 0;
	}

	// �t�@�C���T�C�Y���`�F�b�N(1016MB�ȉ�)
	if (dwSize > 1016 * 0x400 * 0x400) {
		return 0;
	}

	// �T�C�Y�������A��
	dwSize >>= 20;
	return dwSize;
}

//---------------------------------------------------------------------------
//
//	�R���g���[����ԕύX
//
//---------------------------------------------------------------------------
void CSxSIPage::EnableControls(BOOL bEnable, BOOL bDrive)
{
	int i;
	CWnd *pWnd;
	CListCtrl *pListCtrl;

	ASSERT(this);

	// ���X�g�R���g���[���EMO�`�F�b�N�ȊO�̑S�R���g���[����ݒ�
	for (i=0; ; i++) {
		// �R���g���[���擾
		if (!ControlTable[i]) {
			break;
		}
		pWnd = GetDlgItem(ControlTable[i]);
		ASSERT(pWnd);

		// �ݒ�
		pWnd->EnableWindow(bDrive);
	}

	// ���X�g�R���g���[����ݒ�
	pListCtrl = (CListCtrl*)GetDlgItem(IDC_SXSI_LIST);
	ASSERT(pListCtrl);
	pListCtrl->EnableWindow(bEnable);

	// MO�`�F�b�N��ݒ�
	pWnd = GetDlgItem(IDC_SXSI_MOCHECK);
	ASSERT(pWnd);
	pWnd->EnableWindow(bEnable);
}

//---------------------------------------------------------------------------
//
//	�h���C�u���擾
//
//---------------------------------------------------------------------------
int FASTCALL CSxSIPage::GetDrives(const Config *pConfig) const
{
	BOOL bEnable;
	CSASIPage *pSASIPage;
	CSCSIPage *pSCSIPage;
	CAlterPage *pAlterPage;
	CSpinButtonCtrl *pSpin;
	int nPos;

	ASSERT(this);
	ASSERT(pConfig);

	// �y�[�W�擾
	ASSERT(m_pSheet);
	pSASIPage = (CSASIPage*)m_pSheet->SearchPage(MAKEID('S', 'A', 'S', 'I'));
	ASSERT(pSASIPage);
	pSCSIPage = (CSCSIPage*)m_pSheet->SearchPage(MAKEID('S', 'C', 'S', 'I'));
	ASSERT(pSCSIPage);
	pAlterPage = (CAlterPage*)m_pSheet->SearchPage(MAKEID('A', 'L', 'T', ' '));
	ASSERT(pAlterPage);

	// SxSI�C�l�[�u���t���O�𓮓I�Ɏ擾
	bEnable = TRUE;
	if (!pAlterPage->HasParity(pConfig)) {
		// �p���e�B��ݒ肵�Ȃ��BSxSI�͎g�p�ł��Ȃ�
		bEnable = FALSE;
	}
	if (pSCSIPage->GetInterface(pConfig) != 0) {
		// �����܂��͊O�tSCSI�C���^�t�F�[�X�BSxSI�͎g�p�ł��Ȃ�
		bEnable = FALSE;
	}
	if (pSASIPage->GetDrives(pConfig) >= 12) {
		// SASI�h���C�u������������BSxSI�͎g�p�ł��Ȃ�
		bEnable = FALSE;
	}

	// �g�p�ł��Ȃ��ꍇ��0
	if (!bEnable) {
		return 0;
	}

	// ���������̏ꍇ�A�ݒ�l��Ԃ�
	if (!m_bInit) {
		return pConfig->sxsi_drives;
	}

	// ���ݕҏW���̒l��Ԃ�
	pSpin = (CSpinButtonCtrl*)GetDlgItem(IDC_SXSI_DRIVES);
	ASSERT(pSpin);
	nPos = LOWORD(pSpin->GetPos());
	return nPos;
}

//---------------------------------------------------------------------------
//
//	�R���g���[���e�[�u��
//
//---------------------------------------------------------------------------
const UINT CSxSIPage::ControlTable[] = {
	IDC_SXSI_GROUP,
	IDC_SXSI_DRIVEL,
	IDC_SXSI_DRIVEE,
	IDC_SXSI_DRIVES,
	NULL
};

//===========================================================================
//
//	SCSI�y�[�W
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	�R���X�g���N�^
//
//---------------------------------------------------------------------------
CSCSIPage::CSCSIPage()
{
	int i;

	// ID,Help��K���ݒ�
	m_dwID = MAKEID('S', 'C', 'S', 'I');
	m_nTemplate = IDD_SCSIPAGE;
	m_uHelpID = IDC_SCSI_HELP;

	// SCSI�擾
	m_pSCSI = (SCSI*)::GetVM()->SearchDevice(MAKEID('S', 'C', 'S', 'I'));
	ASSERT(m_pSCSI);

	// ������(���̑��f�[�^)
	m_bInit = FALSE;
	m_nDrives = 0;
	m_bMOFirst = FALSE;

	// �f�o�C�X�}�b�v
	ASSERT(SCSI::DeviceMax == 8);
	for (i=0; i<SCSI::DeviceMax; i++) {
		m_DevMap[i] = DevNone;
	}

	// �t�@�C���p�X
	ASSERT(SCSI::HDMax == 5);
	for (i=0; i<SCSI::HDMax; i++) {
		m_szFile[i][0] = _T('\0');
	}
}

//---------------------------------------------------------------------------
//
//	���b�Z�[�W �}�b�v
//
//---------------------------------------------------------------------------
BEGIN_MESSAGE_MAP(CSCSIPage, CConfigPage)
	ON_WM_VSCROLL()
	ON_NOTIFY(NM_CLICK, IDC_SCSI_LIST, OnClick)
	ON_BN_CLICKED(IDC_SCSI_NONEB, OnButton)
	ON_BN_CLICKED(IDC_SCSI_INTB, OnButton)
	ON_BN_CLICKED(IDC_SCSI_EXTB, OnButton)
	ON_BN_CLICKED(IDC_SCSI_MOCHECK, OnCheck)
END_MESSAGE_MAP()

//---------------------------------------------------------------------------
//
//	�y�[�W������
//
//---------------------------------------------------------------------------
BOOL CSCSIPage::OnInitDialog()
{
	int i;
	BOOL bAvail;
	BOOL bEnable[2];
	CButton *pButton;
	CSpinButtonCtrl *pSpin;
	CDC *pDC;
	TEXTMETRIC tm;
	LONG cx;
	CListCtrl *pListCtrl;
	CString strCap;
	CString strFile;

	// ��{�N���X
	CConfigPage::OnInitDialog();

	// �������t���OUp
	m_bInit = TRUE;

	// ROM�̗L���ɉ����āA�C���^�t�F�[�X���W�I�{�^�����֎~
	pButton = (CButton*)GetDlgItem(IDC_SCSI_EXTB);
	ASSERT(pButton);
	bEnable[0] = CheckROM(1);
	pButton->EnableWindow(bEnable[0]);
	pButton = (CButton*)GetDlgItem(IDC_SCSI_INTB);
	ASSERT(pButton);
	bEnable[1] = CheckROM(2);
	pButton->EnableWindow(bEnable[1]);

	// �C���^�t�F�[�X���
	pButton = (CButton*)GetDlgItem(IDC_SCSI_NONEB);
	bAvail = FALSE;
	switch (m_pConfig->mem_type) {
		// �������Ȃ�
		case Memory::None:
		case Memory::SASI:
			break;

		// �O�t
		case Memory::SCSIExt:
			// �O�tROM�����݂���ꍇ�̂�
			if (bEnable[0]) {
				pButton = (CButton*)GetDlgItem(IDC_SCSI_EXTB);
				bAvail = TRUE;
			}
			break;

		// ���̑�(����)
		default:
			// ����ROM�����݂���ꍇ�̂�
			if (bEnable[1]) {
				pButton = (CButton*)GetDlgItem(IDC_SCSI_INTB);
				bAvail = TRUE;
			}
			break;
	}
	ASSERT(pButton);
	pButton->SetCheck(1);

	// �h���C�u��
	pSpin = (CSpinButtonCtrl*)GetDlgItem(IDC_SCSI_DRIVES);
	pSpin->SetBase(10);
	pSpin->SetRange(0, 7);
	m_nDrives = m_pConfig->scsi_drives;
	ASSERT((m_nDrives >= 0) && (m_nDrives <= 7));
	pSpin->SetPos(m_nDrives);

	// MO�D��t���O
	pButton = (CButton*)GetDlgItem(IDC_SCSI_MOCHECK);
	ASSERT(pButton);
	if (m_pConfig->scsi_mofirst) {
		pButton->SetCheck(1);
		m_bMOFirst = TRUE;
	}
	else {
		pButton->SetCheck(0);
		m_bMOFirst = FALSE;
	}

	// SCSI-HD�t�@�C���p�X
	for (i=0; i<SCSI::HDMax; i++) {
		_tcscpy(m_szFile[i], m_pConfig->scsi_file[i]);
	}

	// �e�L�X�g���g���b�N�𓾂�
	pDC = new CClientDC(this);
	::GetTextMetrics(pDC->m_hDC, &tm);
	delete pDC;
	cx = tm.tmAveCharWidth;

	// ���X�g�R���g���[���ݒ�
	pListCtrl = (CListCtrl*)GetDlgItem(IDC_SCSI_LIST);
	ASSERT(pListCtrl);
	pListCtrl->DeleteAllItems();
	if (::IsJapanese()) {
		pListCtrl->InsertColumn(0, _T("ID"), LVCFMT_LEFT, cx * 3, 0);
	}
	else {
		pListCtrl->InsertColumn(0, _T("ID"), LVCFMT_LEFT, cx * 4, 0);
	}
	::GetMsg(IDS_SCSI_CAPACITY, strCap);
	pListCtrl->InsertColumn(1, strCap, LVCFMT_CENTER,  cx * 7, 0);
	::GetMsg(IDS_SCSI_FILENAME, strFile);
	pListCtrl->InsertColumn(2, strFile, LVCFMT_LEFT, cx * 26, 0);

	// ���X�g�R���g���[��1�s�S�̃I�v�V����(COMCTL32.DLL v4.71�ȍ~)
	pListCtrl->SendMessage(LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_FULLROWSELECT ); //| LVS_EX_LABELTIP

	// ���X�g�R���g���[���Ŏg����������擾
	::GetMsg(IDS_SCSI_MO, m_strMO);
	::GetMsg(IDS_SCSI_CD, m_strCD);
	::GetMsg(IDS_SCSI_INIT, m_strInit);
	::GetMsg(IDS_SCSI_NONE, m_strNone);
	::GetMsg(IDS_SCSI_DEVERROR, m_strError);

	// ���X�g�R���g���[���X�V(������BuildMap���s��)
	UpdateList();

	// �R���g���[���L���E����
	if (bAvail) {
		if (m_nDrives > 0) {
			// ���X�g�L���E�h���C�u�L��
			EnableControls(TRUE, TRUE);
		}
		else {
			// ���X�g�����E�h���C�u�L��
			EnableControls(FALSE, TRUE);
		}
	}
	else {
		// ���X�g�����E�h���C�u����
		EnableControls(FALSE, FALSE);
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	����
//
//---------------------------------------------------------------------------
void CSCSIPage::OnOK()
{
	int i;

	// �C���^�t�F�[�X��ʂ��烁������ʐݒ�
	switch (GetIfCtrl()) {
		// �������Ȃ�
		case 0:
			m_pConfig->mem_type = Memory::SASI;
			break;

		// �O�t
		case 1:
			m_pConfig->mem_type = Memory::SCSIExt;
			break;

		// ����
		case 2:
			// �^�C�v���Ⴄ�ꍇ�̂݁ASCSIInt�ɕύX
			if ((m_pConfig->mem_type == Memory::SASI) || (m_pConfig->mem_type == Memory::SCSIExt)) {
				m_pConfig->mem_type = Memory::SCSIInt;
			}
			break;

		// ���̑�(���肦�Ȃ�)
		default:
			ASSERT(FALSE);
	}

	// �h���C�u��
	m_pConfig->scsi_drives = m_nDrives;

	// MO�D��t���O
	m_pConfig->scsi_mofirst = m_bMOFirst;

	// SCSI-HD�t�@�C���p�X
	for (i=0; i<SCSI::HDMax; i++) {
		_tcscpy(m_pConfig->scsi_file[i], m_szFile[i]);
	}

	// ��{�N���X
	CConfigPage::OnOK();
}

//---------------------------------------------------------------------------
//
//	�c�X�N���[��
//
//---------------------------------------------------------------------------
void CSCSIPage::OnVScroll(UINT /*nSBCode*/, UINT nPos, CScrollBar* /*pBar*/)
{
	// �h���C�u���擾
	m_nDrives = nPos;

	// ���X�g�R���g���[���X�V(������BuildMap���s��)
	UpdateList();

	// �R���g���[���L���E����
	if (nPos > 0) {
		EnableControls(TRUE);
	}
	else {
		EnableControls(FALSE);
	}
}

//---------------------------------------------------------------------------
//
//	���X�g�R���g���[���N���b�N
//
//---------------------------------------------------------------------------
void CSCSIPage::OnClick(NMHDR* /*pNMHDR*/, LRESULT* /*pResult*/)
{
	CListCtrl *pListCtrl;
	int i;
	int nID;
	int nCount;
	int nDrive;
	TCHAR szPath[FILEPATH_MAX];

	// ���X�g�R���g���[���擾
	pListCtrl = (CListCtrl*)GetDlgItem(IDC_SCSI_LIST);
	ASSERT(pListCtrl);

	// �J�E���g�����擾
	nCount = pListCtrl->GetItemCount();

	// �Z���N�g����Ă���A�C�e�����擾
	nID = -1;
	for (i=0; i<nCount; i++) {
		if (pListCtrl->GetItemState(i, LVIS_SELECTED)) {
			nID = i;
			break;
		}
	}
	if (nID < 0) {
		return;
	}

	// �A�C�e���f�[�^����ID���擾
	nID = (int)pListCtrl->GetItemData(nID);

	// �}�b�v�����āA�^�C�v�𔻕�
	if (m_DevMap[nID] != DevSCSI) {
		return;
	}

	// ID����h���C�u�C���f�b�N�X�擾(MO�͍l�����Ȃ�)
	nDrive = 0;
	for (i=0; i<SCSI::DeviceMax; i++) {
		if (i == nID) {
			break;
		}
		if (m_DevMap[i] == DevSCSI) {
			nDrive++;
		}
	}
	ASSERT((nDrive >= 0) && (nDrive < SCSI::HDMax));

	// �I�[�v�������݂�
	_tcscpy(szPath, m_szFile[nDrive]);
	if (!::FileOpenDlg(this, szPath, IDS_SCSIOPEN)) {
		return;
	}

	// �p�X���X�V
	_tcscpy(m_szFile[nDrive], szPath);

	// ���X�g�R���g���[���X�V
	UpdateList();
}

//---------------------------------------------------------------------------
//
//	���W�I�{�^���ύX
//
//---------------------------------------------------------------------------
void CSCSIPage::OnButton()
{
	CButton *pButton;

	// �C���^�t�F�[�X�����Ƀ`�F�b�N����Ă��邩
	pButton = (CButton*)GetDlgItem(IDC_SCSI_NONEB);
	ASSERT(pButton);
	if (pButton->GetCheck() != 0) {
		// ���X�g�����E�h���C�u����
		EnableControls(FALSE, FALSE);
		return;
	}

	if (m_nDrives > 0) {
		// ���X�g�L���E�h���C�u�L��
		EnableControls(TRUE, TRUE);
	}
	else {
		// ���X�g�����E�h���C�u�L��
		EnableControls(FALSE, TRUE);
	}
}

//---------------------------------------------------------------------------
//
//	�`�F�b�N�{�b�N�X�ύX
//
//---------------------------------------------------------------------------
void CSCSIPage::OnCheck()
{
	CButton *pButton;

	// ���݂̏�Ԃ𓾂�
	pButton = (CButton*)GetDlgItem(IDC_SCSI_MOCHECK);
	ASSERT(pButton);
	if (pButton->GetCheck() != 0) {
		m_bMOFirst = TRUE;
	}
	else {
		m_bMOFirst = FALSE;
	}

	// ���X�g�R���g���[���X�V(������BuildMap���s��)
	UpdateList();
}

//---------------------------------------------------------------------------
//
//	�C���^�t�F�[�X��ʎ擾
//
//---------------------------------------------------------------------------
int FASTCALL CSCSIPage::GetInterface(const Config *pConfig) const
{
	ASSERT(this);
	ASSERT(pConfig);

	// �������t���O
	if (!m_bInit) {
		// ����������Ă��Ȃ��̂ŁAConfig����擾
		switch (pConfig->mem_type) {
			// �������Ȃ�
			case Memory::None:
			case Memory::SASI:
				return 0;

			// �O�t
			case Memory::SCSIExt:
				return 1;

			// ���̑�(����)
			default:
				return 2;
		}
	}

	// ����������Ă���̂ŁA�R���g���[������擾
	return GetIfCtrl();
}

//---------------------------------------------------------------------------
//
//	�C���^�t�F�[�X��ʎ擾(�R���g���[�����)
//
//---------------------------------------------------------------------------
int FASTCALL CSCSIPage::GetIfCtrl() const
{
	CButton *pButton;

	ASSERT(this);

	// �������Ȃ�
	pButton = (CButton*)GetDlgItem(IDC_SCSI_NONEB);
	ASSERT(pButton);
	if (pButton->GetCheck() != 0) {
		return 0;
	}

	// �O�t
	pButton = (CButton*)GetDlgItem(IDC_SCSI_EXTB);
	ASSERT(pButton);
	if (pButton->GetCheck() != 0) {
		return 1;
	}

	// ����
	pButton = (CButton*)GetDlgItem(IDC_SCSI_INTB);
	ASSERT(pButton);
	ASSERT(pButton->GetCheck() != 0);
	return 2;
}

//---------------------------------------------------------------------------
//
//	ROM�`�F�b�N
//
//---------------------------------------------------------------------------
BOOL FASTCALL CSCSIPage::CheckROM(int nType) const
{
	Filepath path;
	Fileio fio;
	DWORD dwSize;

	ASSERT(this);
	ASSERT((nType >= 0) && (nType <= 2));

	// 0:�����̏ꍇ�͖�������OK
	if (nType == 0) {
		return TRUE;
	}

	// �t�@�C���p�X�쐬
	if (nType == 1) {
		// �O�t
		path.SysFile(Filepath::SCSIExt);
	}
	else {
		// ����
		path.SysFile(Filepath::SCSIInt);
	}

	// ���b�N
	::LockVM();

	// �I�[�v�������݂�
	if (!fio.Open(path, Fileio::ReadOnly)) {
		::UnlockVM();
		return FALSE;
	}

	// �t�@�C���T�C�Y�擾
	dwSize = fio.GetFileSize();
	fio.Close();
	::UnlockVM();

	if (nType == 1) {
		// �O�t�́A0x2000�o�C�g�܂���0x1fe0�o�C�g(WinX68k�����łƌ݊����Ƃ�)
		if ((dwSize == 0x2000) || (dwSize == 0x1fe0)) {
			return TRUE;
		}
	}
	else {
		// �����́A0x2000�o�C�g�̂�
		if (dwSize == 0x2000) {
			return TRUE;
		}
	}

	return FALSE;
}

//---------------------------------------------------------------------------
//
//	���X�g�R���g���[���X�V
//
//---------------------------------------------------------------------------
void FASTCALL CSCSIPage::UpdateList()
{
	int i;
	int nDrive;
	int nDev;
	int nCount;
	int nCap;
	CListCtrl *pListCtrl;
	CString strCtrl;
	CString strID;
	CString strSize;
	CString strFile;

	ASSERT(this);

	// �}�b�v���r���h
	BuildMap();

	// ���X�g�R���g���[���擾�A�J�E���g�擾
	pListCtrl = (CListCtrl*)GetDlgItem(IDC_SCSI_LIST);
	ASSERT(pListCtrl);
	nCount = pListCtrl->GetItemCount();

	// �}�b�v�̂���None�łȂ����̂̐��𐔂���
	nDev = 0;
	for (i=0; i<8; i++) {
		if (m_DevMap[i] != DevNone) {
			nDev++;
		}
	}

	// nDev�����A�C�e��������
	while (nCount > nDev) {
		pListCtrl->DeleteItem(nCount - 1);
		nCount--;
	}
	while (nDev > nCount) {
		strID.Format(_T("%d"), nCount + 1);
		pListCtrl->InsertItem(nCount, strID);
		nCount++;
	}

	// ��r���[�v
	nDrive = 0;
	nDev = 0;
	for (i=0; i<SCSI::DeviceMax; i++) {
		// �^�C�v�ɉ����ĕ���������
		switch (m_DevMap[i]) {
			// SCSI �n�[�h�f�B�X�N
			case DevSCSI:
				nCap = CheckSCSI(nDrive);
				if (nCap > 0) {
					strSize.Format("%dMB", nCap);
				}
				else {
					strSize = m_strError;
				}
				strFile = m_szFile[nDrive];
				nDrive++;
				break;

			// SCSI MO�f�B�X�N
			case DevMO:
				strSize = m_strNone;
				strFile = m_strMO;
				break;

			// SCSI CD-ROM
			case DevCD:
				strSize = m_strNone;
				strFile = m_strCD;
				break;

			// �C�j�V�G�[�^(�z�X�g)
			case DevInit:
				strSize = m_strNone;
				strFile = m_strInit;
				break;

			// �f�o�C�X�Ȃ�
			case DevNone:
				// ���ɐi��
				continue;

			// ���̑�(���蓾�Ȃ�)
			default:
				ASSERT(FALSE);
				return;
		}

		// �A�C�e���f�[�^
		if ((int)pListCtrl->GetItemData(nDev) != i) {
			pListCtrl->SetItemData(nDev, (DWORD)i);
		}

		// ID
		strID.Format(_T("%d"), i);
		strCtrl = pListCtrl->GetItemText(nDev, 0);
		if (strID != strCtrl) {
			pListCtrl->SetItemText(nDev, 0, strID);
		}

		// �e��
		strCtrl = pListCtrl->GetItemText(nDev, 1);
		if (strSize != strCtrl) {
			pListCtrl->SetItemText(nDev, 1, strSize);
		}

		// �t�@�C����
		strCtrl = pListCtrl->GetItemText(nDev, 2);
		if (strFile != strCtrl) {
			pListCtrl->SetItemText(nDev, 2, strFile);
		}

		// ����
		nDev++;
	}
}

//---------------------------------------------------------------------------
//
//	�}�b�v�쐬
//
//---------------------------------------------------------------------------
void FASTCALL CSCSIPage::BuildMap()
{
	int i;
	int nID;
	int nInit;
	int nHD;
	BOOL bMO;
	BOOL bCD;

	ASSERT(this);

	// ������
	nHD = 0;
	bMO = FALSE;
	bCD = FALSE;

	// �f�B�X�N��������
	switch (m_nDrives) {
		// 0��
		case 0:
			break;

		// 1��
		case 1:
			// MO�D�悩�AHD�D�悩�ŕ�����
			if (m_bMOFirst) {
				bMO = TRUE;
			}
			else {
				nHD = 1;
			}
			break;

		// 2��
		case 2:
			// HD,MO�Ƃ�1��
			nHD = 1;
			bMO = TRUE;
			break;

		// 3��
		case 3:
			// HD,MO,CD�Ƃ�1��
			nHD = 1;
			bMO = TRUE;
			bCD = TRUE;
			break;

		// 4��ȏ�
		default:
			ASSERT(m_nDrives <= 7);
			nHD= m_nDrives - 2;
			bMO = TRUE;
			bCD = TRUE;
			break;
	}

	// �I�[���N���A
	for (i=0; i<8; i++) {
		m_DevMap[i] = DevNone;
	}

	// �C�j�V�G�[�^���ɐݒ�
	ASSERT(m_pSCSI);
	nInit = m_pSCSI->GetSCSIID();
	ASSERT((nInit >= 0) && (nInit <= 7));
	m_DevMap[nInit] = DevInit;

	// MO�ݒ�(�D��t���O���̂�)
	if (bMO && m_bMOFirst) {
		for (nID=0; nID<SCSI::DeviceMax; nID++) {
			if (m_DevMap[nID] == DevNone) {
				m_DevMap[nID] = DevMO;
				bMO = FALSE;
				break;
			}
		}
	}

	// HD�ݒ�
	for (i=0; i<nHD; i++) {
		for (nID=0; nID<SCSI::DeviceMax; nID++) {
			if (m_DevMap[nID] == DevNone) {
				m_DevMap[nID] = DevSCSI;
				break;
			}
		}
	}

	// MO�ݒ�
	if (bMO) {
		for (nID=0; nID<SCSI::DeviceMax; nID++) {
			if (m_DevMap[nID] == DevNone) {
				m_DevMap[nID] = DevMO;
				break;
			}
		}
	}

	// CD�ݒ�(ID=6�Œ�A�����g���Ă�����7)
	if (bCD) {
		if (m_DevMap[6] == DevNone) {
			m_DevMap[6] = DevCD;
		}
		else {
			ASSERT(m_DevMap[7] == DevNone);
			m_DevMap[7] = DevCD;
		}
	}
}

//---------------------------------------------------------------------------
//
//	SCSI�n�[�h�f�B�X�N�e�ʃ`�F�b�N
//	���f�o�C�X�G���[��0��Ԃ�
//
//---------------------------------------------------------------------------
int FASTCALL CSCSIPage::CheckSCSI(int nDrive)
{
	Fileio fio;
	DWORD dwSize;

	ASSERT(this);
	ASSERT((nDrive >= 0) && (nDrive <= SCSI::HDMax));

	// ���b�N
	::LockVM();

	// �t�@�C���I�[�v��
	if (!fio.Open(m_szFile[nDrive], Fileio::ReadOnly)) {
		// �G���[�Ȃ̂�0��Ԃ�
		fio.Close();
		::UnlockVM();
		return 0;
	}

	// �e�ʎ擾
	dwSize = fio.GetFileSize();

	// �A�����b�N
	fio.Close();
	::UnlockVM();

	// �t�@�C���T�C�Y���`�F�b�N(512�o�C�g�P��)
	if ((dwSize & 0x1ff) != 0) {
		return 0;
	}

	// �t�@�C���T�C�Y���`�F�b�N(10MB�ȏ�)
	if (dwSize < 10 * 0x400 * 0x400) {
		return 0;
	}

	// �t�@�C���T�C�Y���`�F�b�N(4095MB�ȉ�)
	if (dwSize > 0xfff00000) {
		return 0;
	}

	// �T�C�Y�������A��
	dwSize >>= 20;
	return dwSize;
}

//---------------------------------------------------------------------------
//
//	�R���g���[����ԕύX
//
//---------------------------------------------------------------------------
void FASTCALL CSCSIPage::EnableControls(BOOL bEnable, BOOL bDrive)
{
	int i;
	CWnd *pWnd;
	CListCtrl *pListCtrl;

	ASSERT(this);

	// ���X�g�R���g���[���EMO�`�F�b�N�ȊO�̑S�R���g���[����ݒ�
	for (i=0; ; i++) {
		// �R���g���[���擾
		if (!ControlTable[i]) {
			break;
		}
		pWnd = GetDlgItem(ControlTable[i]);
		ASSERT(pWnd);

		// �ݒ�
		pWnd->EnableWindow(bDrive);
	}

	// ���X�g�R���g���[����ݒ�
	pListCtrl = (CListCtrl*)GetDlgItem(IDC_SCSI_LIST);
	ASSERT(pListCtrl);
	pListCtrl->EnableWindow(bEnable);

	// MO�`�F�b�N��ݒ�
	pWnd = GetDlgItem(IDC_SCSI_MOCHECK);
	ASSERT(pWnd);
	pWnd->EnableWindow(bEnable);
}

//---------------------------------------------------------------------------
//
//	�R���g���[���e�[�u��
//
//---------------------------------------------------------------------------
const UINT CSCSIPage::ControlTable[] = {
	IDC_SCSI_GROUP,
	IDC_SCSI_DRIVEL,
	IDC_SCSI_DRIVEE,
	IDC_SCSI_DRIVES,
	NULL
};

//===========================================================================
//
//	�|�[�g�y�[�W
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	�R���X�g���N�^
//
//---------------------------------------------------------------------------
CPortPage::CPortPage()
{
	// ID,Help��K���ݒ�
	m_dwID = MAKEID('P', 'O', 'R', 'T');
	m_nTemplate = IDD_PORTPAGE;
	m_uHelpID = IDC_PORT_HELP;
}

//---------------------------------------------------------------------------
//
//	������
//
//---------------------------------------------------------------------------
BOOL CPortPage::OnInitDialog()
{
	int i;
	CComboBox *pComboBox;
	CString strText;
	CButton *pButton;
	CEdit *pEdit;

	// ��{�N���X
	CConfigPage::OnInitDialog();

	// COM�R���{�{�b�N�X
	pComboBox = (CComboBox*)GetDlgItem(IDC_PORT_COMC);
	ASSERT(pComboBox);
	pComboBox->ResetContent();
	::GetMsg(IDS_PORT_NOASSIGN, strText);
	pComboBox->AddString(strText);
	for (i=1; i<=9; i++) {
		strText.Format(_T("COM%d"), i);
		pComboBox->AddString(strText);
	}
	pComboBox->SetCurSel(m_pConfig->port_com);

	// ��M���O
	pEdit = (CEdit*)GetDlgItem(IDC_PORT_RECVE);
	ASSERT(pEdit);
	pEdit->SetWindowText(m_pConfig->port_recvlog);

	// ����38400bps
	pButton = (CButton*)GetDlgItem(IDC_PORT_BAUDRATE);
	ASSERT(pButton);
	pButton->SetCheck(m_pConfig->port_384);

	// LPT�R���{�{�b�N�X
	pComboBox = (CComboBox*)GetDlgItem(IDC_PORT_LPTC);
	ASSERT(pComboBox);
	pComboBox->ResetContent();
	::GetMsg(IDS_PORT_NOASSIGN, strText);
	pComboBox->AddString(strText);
	for (i=1; i<=9; i++) {
		strText.Format(_T("LPT%d"), i);
		pComboBox->AddString(strText);
	}
	pComboBox->SetCurSel(m_pConfig->port_lpt);

	// ���M���O
	pEdit = (CEdit*)GetDlgItem(IDC_PORT_SENDE);
	ASSERT(pEdit);
	pEdit->SetWindowText(m_pConfig->port_sendlog);

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	����
//
//---------------------------------------------------------------------------
void CPortPage::OnOK()
{
	CComboBox *pComboBox;
	CEdit *pEdit;
	CButton *pButton;

	// COM�R���{�{�b�N�X
	pComboBox = (CComboBox*)GetDlgItem(IDC_PORT_COMC);
	ASSERT(pComboBox);
	m_pConfig->port_com = pComboBox->GetCurSel();

	// ��M���O
	pEdit = (CEdit*)GetDlgItem(IDC_PORT_RECVE);
	ASSERT(pEdit);
	pEdit->GetWindowText(m_pConfig->port_recvlog, sizeof(m_pConfig->port_recvlog));

	// ����38400bps
	pButton = (CButton*)GetDlgItem(IDC_PORT_BAUDRATE);
	ASSERT(pButton);
	m_pConfig->port_384 = pButton->GetCheck();

	// LPT�R���{�{�b�N�X
	pComboBox = (CComboBox*)GetDlgItem(IDC_PORT_LPTC);
	ASSERT(pComboBox);
	m_pConfig->port_lpt = pComboBox->GetCurSel();

	// ���M���O
	pEdit = (CEdit*)GetDlgItem(IDC_PORT_SENDE);
	ASSERT(pEdit);
	pEdit->GetWindowText(m_pConfig->port_sendlog, sizeof(m_pConfig->port_sendlog));

	// ��{�N���X
	CConfigPage::OnOK();
}

//===========================================================================
//
//	MIDI�y�[�W
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	�R���X�g���N�^
//
//---------------------------------------------------------------------------
CMIDIPage::CMIDIPage()
{
	// ID,Help��K���ݒ�
	m_dwID = MAKEID('M', 'I', 'D', 'I');
	m_nTemplate = IDD_MIDIPAGE;
	m_uHelpID = IDC_MIDI_HELP;

	// �I�u�W�F�N�g
	m_pMIDI = NULL;
}

//---------------------------------------------------------------------------
//
//	���b�Z�[�W �}�b�v
//
//---------------------------------------------------------------------------
BEGIN_MESSAGE_MAP(CMIDIPage, CConfigPage)
	ON_WM_VSCROLL()
	ON_BN_CLICKED(IDC_MIDI_BID0, OnBIDClick)
	ON_BN_CLICKED(IDC_MIDI_BID1, OnBIDClick)
	ON_BN_CLICKED(IDC_MIDI_BID2, OnBIDClick)
END_MESSAGE_MAP()

//---------------------------------------------------------------------------
//
//	������
//
//---------------------------------------------------------------------------
BOOL CMIDIPage::OnInitDialog()
{
	CButton *pButton;
	CComboBox *pComboBox;
	CSpinButtonCtrl *pSpin;
	CFrmWnd *pFrmWnd;
	CString strDesc;
	int nNum;
	int i;

	// ��{�N���X
	CConfigPage::OnInitDialog();

	// MIDI�R���|�[�l���g�擾
	pFrmWnd = (CFrmWnd*)AfxGetApp()->m_pMainWnd;
	ASSERT(pFrmWnd);
	m_pMIDI = pFrmWnd->GetMIDI();
	ASSERT(m_pMIDI);

	// �R���g���[���L���E����
	m_bEnableCtrl = TRUE;
	EnableControls(FALSE);
	if (m_pConfig->midi_bid != 0) {
		EnableControls(TRUE);
	}

	// �{�[�hID
	pButton = (CButton*)GetDlgItem(IDC_MIDI_BID0 + m_pConfig->midi_bid);
	ASSERT(pButton);
	pButton->SetCheck(1);

	// ���荞�݃��x��
	pButton = (CButton*)GetDlgItem(IDC_MIDI_ILEVEL4 + m_pConfig->midi_ilevel);
	ASSERT(pButton);
	pButton->SetCheck(1);

	// �������Z�b�g
	pButton = (CButton*)GetDlgItem(IDC_MIDI_RSTGM + m_pConfig->midi_reset);
	ASSERT(pButton);
	pButton->SetCheck(1);

	// �f�o�C�X(IN)
	pComboBox = (CComboBox*)GetDlgItem(IDC_MIDI_INC);
	ASSERT(pComboBox);
	pComboBox->ResetContent();
	::GetMsg(IDS_MIDI_NOASSIGN, strDesc);
	pComboBox->AddString(strDesc);
	nNum = (int)m_pMIDI->GetInDevs();
	for (i=0; i<nNum; i++) {
		m_pMIDI->GetInDevDesc(i, strDesc);
		pComboBox->AddString(strDesc);
	}

	// �R���{�{�b�N�X�̃J�[�\����ݒ�
	if (m_pConfig->midiin_device <= nNum) {
		pComboBox->SetCurSel(m_pConfig->midiin_device);
	}
	else {
		pComboBox->SetCurSel(0);
	}

	// �f�o�C�X(OUT)
	pComboBox = (CComboBox*)GetDlgItem(IDC_MIDI_OUTC);
	ASSERT(pComboBox);
	pComboBox->ResetContent();
	::GetMsg(IDS_MIDI_NOASSIGN, strDesc);
	pComboBox->AddString(strDesc);
	nNum = (int)m_pMIDI->GetOutDevs();
	if (nNum >= 1) {
		::GetMsg(IDS_MIDI_MAPPER, strDesc);
		pComboBox->AddString(strDesc);
		for (i=0; i<nNum; i++) {
			m_pMIDI->GetOutDevDesc(i, strDesc);
			pComboBox->AddString(strDesc);
		}
	}

	// �R���{�{�b�N�X�̃J�[�\����ݒ�
	if (m_pConfig->midiout_device < (nNum + 2)) {
		pComboBox->SetCurSel(m_pConfig->midiout_device);
	}
	else {
		pComboBox->SetCurSel(0);
	}

	// �x��(IN)
	pSpin = (CSpinButtonCtrl*)GetDlgItem(IDC_MIDI_DLYIS);
	ASSERT(pSpin);
	pSpin->SetBase(10);
	pSpin->SetRange(0, 200);
	pSpin->SetPos(m_pConfig->midiin_delay);

	// �x��(OUT)
	pSpin = (CSpinButtonCtrl*)GetDlgItem(IDC_MIDI_DLYOS);
	ASSERT(pSpin);
	pSpin->SetBase(10);
	pSpin->SetRange(20, 200);
	pSpin->SetPos(m_pConfig->midiout_delay);

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	����
//
//---------------------------------------------------------------------------
void CMIDIPage::OnOK()
{
	int i;
	CButton *pButton;
	CComboBox *pComboBox;
	CSpinButtonCtrl *pSpin;

	// �{�[�hID
	for (i=0; i<3; i++) {
		pButton = (CButton*)GetDlgItem(IDC_MIDI_BID0 + i);
		ASSERT(pButton);
		if (pButton->GetCheck() == 1) {
			m_pConfig->midi_bid = i;
			break;
		}
	}

	// ���荞�݃��x��
	for (i=0; i<2; i++) {
		pButton = (CButton*)GetDlgItem(IDC_MIDI_ILEVEL4 + i);
		ASSERT(pButton);
		if (pButton->GetCheck() == 1) {
			m_pConfig->midi_ilevel = i;
			break;
		}
	}

	// �������Z�b�g
	for (i=0; i<4; i++) {
		pButton = (CButton*)GetDlgItem(IDC_MIDI_RSTGM + i);
		ASSERT(pButton);
		if (pButton->GetCheck() == 1) {
			m_pConfig->midi_reset = i;
			break;
		}
	}

	// �f�o�C�X(IN)
	pComboBox = (CComboBox*)GetDlgItem(IDC_MIDI_INC);
	ASSERT(pComboBox);
	m_pConfig->midiin_device = pComboBox->GetCurSel();

	// �f�o�C�X(OUT)
	pComboBox = (CComboBox*)GetDlgItem(IDC_MIDI_OUTC);
	ASSERT(pComboBox);
	m_pConfig->midiout_device = pComboBox->GetCurSel();

	// �x��(IN)
	pSpin = (CSpinButtonCtrl*)GetDlgItem(IDC_MIDI_DLYIS);
	ASSERT(pSpin);
	m_pConfig->midiin_delay = LOWORD(pSpin->GetPos());

	// �x��(OUT)
	pSpin = (CSpinButtonCtrl*)GetDlgItem(IDC_MIDI_DLYOS);
	ASSERT(pSpin);
	m_pConfig->midiout_delay = LOWORD(pSpin->GetPos());

	// ��{�N���X
	CConfigPage::OnOK();
}

//---------------------------------------------------------------------------
//
//	�L�����Z��
//
//---------------------------------------------------------------------------
void CMIDIPage::OnCancel()
{
	// MIDI�f�B���C��߂�(IN)
	m_pMIDI->SetInDelay(m_pConfig->midiin_delay);

	// MIDI�f�B���C��߂�(OUT)
	m_pMIDI->SetOutDelay(m_pConfig->midiout_delay);

	// ��{�N���X
	CConfigPage::OnCancel();
}

//---------------------------------------------------------------------------
//
//	�c�X�N���[��
//
//---------------------------------------------------------------------------
void CMIDIPage::OnVScroll(UINT /*nSBCode*/, UINT nPos, CScrollBar *pBar)
{
	CSpinButtonCtrl *pSpin;

	// IN
	pSpin = (CSpinButtonCtrl*)GetDlgItem(IDC_MIDI_DLYIS);
	ASSERT(pSpin);
	if ((CWnd*)pSpin == (CWnd*)pBar) {
		m_pMIDI->SetInDelay(nPos);
	}

	// OUT
	pSpin = (CSpinButtonCtrl*)GetDlgItem(IDC_MIDI_DLYOS);
	ASSERT(pSpin);
	if ((CWnd*)pSpin == (CWnd*)pBar) {
		m_pMIDI->SetOutDelay(nPos);
	}
}

//---------------------------------------------------------------------------
//
//	�{�[�hID�N���b�N
//
//---------------------------------------------------------------------------
void CMIDIPage::OnBIDClick()
{
	CButton *pButton;

	// �{�[�hID�u�Ȃ��v�̃R���g���[�����擾
	pButton = (CButton*)GetDlgItem(IDC_MIDI_BID0);
	ASSERT(pButton);

	// �`�F�b�N�����Ă��邩�ǂ����Œ��ׂ�
	if (pButton->GetCheck() == 1) {
		EnableControls(FALSE);
	}
	else {
		EnableControls(TRUE);
	}
}

//---------------------------------------------------------------------------
//
//	�R���g���[����ԕύX
//
//---------------------------------------------------------------------------
void FASTCALL CMIDIPage::EnableControls(BOOL bEnable) 
{
	int i;
	CWnd *pWnd;

	ASSERT(this);

	// �t���O�`�F�b�N
	if (m_bEnableCtrl == bEnable) {
		return;
	}
	m_bEnableCtrl = bEnable;

	// �{�[�hID�AHelp�ȊO�̑S�R���g���[����ݒ�
	for(i=0; ; i++) {
		// �I���`�F�b�N
		if (ControlTable[i] == NULL) {
			break;
		}

		// �R���g���[���擾
		pWnd = GetDlgItem(ControlTable[i]);
		ASSERT(pWnd);
		pWnd->EnableWindow(bEnable);
	}
}

//---------------------------------------------------------------------------
//
//	�R���g���[��ID�e�[�u��
//
//---------------------------------------------------------------------------
const UINT CMIDIPage::ControlTable[] = {
	IDC_MIDI_ILEVELG,
	IDC_MIDI_ILEVEL4,
	IDC_MIDI_ILEVEL2,
	IDC_MIDI_RSTG,
	IDC_MIDI_RSTGM,
	IDC_MIDI_RSTGS,
	IDC_MIDI_RSTXG,
	IDC_MIDI_RSTLA,
	IDC_MIDI_DEVG,
	IDC_MIDI_INS,
	IDC_MIDI_INC,
	IDC_MIDI_DLYIL,
	IDC_MIDI_DLYIE,
	IDC_MIDI_DLYIS,
	IDC_MIDI_DLYIG,
	IDC_MIDI_OUTS,
	IDC_MIDI_OUTC,
	IDC_MIDI_DLYOL,
	IDC_MIDI_DLYOE,
	IDC_MIDI_DLYOS,
	IDC_MIDI_DLYOG,
	NULL
};

//===========================================================================
//
//	�����y�[�W
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	�R���X�g���N�^
//
//---------------------------------------------------------------------------
CAlterPage::CAlterPage()
{
	// ID,Help��K���ݒ�
	m_dwID = MAKEID('A', 'L', 'T', ' ');
	m_nTemplate = IDD_ALTERPAGE;
	m_uHelpID = IDC_ALTER_HELP;

	// ������
	m_bInit = FALSE;
	m_bParity = FALSE;
}

//---------------------------------------------------------------------------
//
//	������
//
//---------------------------------------------------------------------------
BOOL CAlterPage::OnInitDialog()
{
	// ��{�N���X
	CConfigPage::OnInitDialog();

	// �������ς݁A�p���e�B�t���O���擾���Ă���
	m_bInit = TRUE;
	m_bParity = m_pConfig->sasi_parity;

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	�y�[�W�ړ�
//
//---------------------------------------------------------------------------
BOOL CAlterPage::OnKillActive()
{
	CButton *pButton;

	ASSERT(this);

	// �`�F�b�N�{�b�N�X���p���e�B�t���O�ɔ��f������
	pButton = (CButton*)GetDlgItem(IDC_ALTER_PARITY);
	ASSERT(pButton);
	if (pButton->GetCheck() == 1) {
		m_bParity = TRUE;
	}
	else {
		m_bParity = FALSE;
	}

	// ���N���X
	return CConfigPage::OnKillActive();
}

//---------------------------------------------------------------------------
//
//	�f�[�^����
//
//---------------------------------------------------------------------------
void CAlterPage::DoDataExchange(CDataExchange *pDX)
{
	ASSERT(this);
	ASSERT(pDX);

	// ��{�N���X
	CConfigPage::DoDataExchange(pDX);

	// �f�[�^����
	DDX_Check(pDX, IDC_ALTER_SRAM, m_pConfig->sram_64k);
	DDX_Check(pDX, IDC_ALTER_SCC, m_pConfig->scc_clkup);
	DDX_Check(pDX, IDC_ALTER_POWERLED, m_pConfig->power_led);
	DDX_Check(pDX, IDC_ALTER_2DD, m_pConfig->dual_fdd);
	DDX_Check(pDX, IDC_ALTER_PARITY, m_pConfig->sasi_parity);
}

//---------------------------------------------------------------------------
//
//	SASI�p���e�B�@�\�`�F�b�N
//
//---------------------------------------------------------------------------
BOOL FASTCALL CAlterPage::HasParity(const Config *pConfig) const
{
	ASSERT(this);
	ASSERT(pConfig);

	// ����������Ă��Ȃ���΁A�^���ꂽConfig�f�[�^����
	if (!m_bInit) {
		return pConfig->sasi_parity;
	}

	// �������ς݂Ȃ�A�ŐV�̕ҏW���ʂ�m�点��
	return m_bParity;
}

//===========================================================================
//
//	���W���[���y�[�W
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	�R���X�g���N�^
//
//---------------------------------------------------------------------------
CResumePage::CResumePage()
{
	// ID,Help��K���ݒ�
	m_dwID = MAKEID('R', 'E', 'S', 'M');
	m_nTemplate = IDD_RESUMEPAGE;
	m_uHelpID = IDC_RESUME_HELP;
}

//---------------------------------------------------------------------------
//
//	������
//
//---------------------------------------------------------------------------
BOOL CResumePage::OnInitDialog()
{
	// ��{�N���X
	CConfigPage::OnInitDialog();

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	�f�[�^����
//
//---------------------------------------------------------------------------
void CResumePage::DoDataExchange(CDataExchange *pDX)
{
	ASSERT(this);
	ASSERT(pDX);

	// ��{�N���X
	CConfigPage::DoDataExchange(pDX);

	// �f�[�^����
	DDX_Check(pDX, IDC_RESUME_FDC, m_pConfig->resume_fd);
	DDX_Check(pDX, IDC_RESUME_MOC, m_pConfig->resume_mo);
	DDX_Check(pDX, IDC_RESUME_CDC, m_pConfig->resume_cd);
	DDX_Check(pDX, IDC_RESUME_XM6C, m_pConfig->resume_state);
	DDX_Check(pDX, IDC_RESUME_SCREENC, m_pConfig->resume_screen);
	DDX_Check(pDX, IDC_RESUME_DIRC, m_pConfig->resume_dir);
}

//===========================================================================
//
//	TrueKey�_�C�A���O
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	�R���X�g���N�^
//
//---------------------------------------------------------------------------
CTKeyDlg::CTKeyDlg(CWnd *pParent) : CDialog(IDD_KEYINDLG, pParent)
{
	CFrmWnd *pFrmWnd;

	// �p����ւ̑Ή�
	if (!::IsJapanese()) {
		m_lpszTemplateName = MAKEINTRESOURCE(IDD_US_KEYINDLG);
		m_nIDHelp = IDD_US_KEYINDLG;
	}

	// TrueKey�擾
	pFrmWnd = (CFrmWnd*)AfxGetApp()->m_pMainWnd;
	ASSERT(pFrmWnd);
	m_pTKey = pFrmWnd->GetTKey();
	ASSERT(m_pTKey);
}

//---------------------------------------------------------------------------
//
//	���b�Z�[�W �}�b�v
//
//---------------------------------------------------------------------------
BEGIN_MESSAGE_MAP(CTKeyDlg, CDialog)
	ON_WM_PAINT()
	ON_WM_GETDLGCODE()
	ON_WM_TIMER()
	ON_WM_RBUTTONDOWN()
END_MESSAGE_MAP()

//---------------------------------------------------------------------------
//
//	�_�C�A���O������
//
//---------------------------------------------------------------------------
BOOL CTKeyDlg::OnInitDialog()
{
	CString strText;
	CStatic *pStatic;
	LPCSTR lpszKey;

	// ��{�N���X
	CDialog::OnInitDialog();

	// IME�I�t
	::ImmAssociateContext(m_hWnd, (HIMC)NULL);

	// �K�C�h��`�̏���
	pStatic = (CStatic*)GetDlgItem(IDC_KEYIN_LABEL);
	ASSERT(pStatic);
	pStatic->GetWindowText(strText);
	m_strGuide.Format(strText, m_nTarget);
	pStatic->GetWindowRect(&m_rectGuide);
	ScreenToClient(&m_rectGuide);
	pStatic->DestroyWindow();

	// ���蓖�ċ�`�̏���
	pStatic = (CStatic*)GetDlgItem(IDC_KEYIN_STATIC);
	ASSERT(pStatic);
	pStatic->GetWindowText(m_strAssign);
	pStatic->GetWindowRect(&m_rectAssign);
	ScreenToClient(&m_rectAssign);
	pStatic->DestroyWindow();

	// �L�[��`�̏���
	pStatic = (CStatic*)GetDlgItem(IDC_KEYIN_KEY);
	ASSERT(pStatic);
	pStatic->GetWindowText(m_strKey);
	if (m_nKey != 0) {
		lpszKey = m_pTKey->GetKeyID(m_nKey);
		if (lpszKey) {
			m_strKey = lpszKey;
		}
	}
	pStatic->GetWindowRect(&m_rectKey);
	ScreenToClient(&m_rectKey);
	pStatic->DestroyWindow();

	// �L�[�{�[�h��Ԃ��擾
	::GetKeyboardState(m_KeyState);

	// �^�C�}���͂�
	m_nTimerID = SetTimer(IDD_KEYINDLG, 100, NULL);

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	OK
//
//---------------------------------------------------------------------------
void CTKeyDlg::OnOK()
{
	// [CR]�ɂ��I����}��
}

//---------------------------------------------------------------------------
//
//	�L�����Z��
//
//---------------------------------------------------------------------------
void CTKeyDlg::OnCancel()
{
	// �^�C�}��~
	if (m_nTimerID) {
		KillTimer(m_nTimerID);
		m_nTimerID = NULL;
	}

	// ��{�N���X
	CDialog::OnCancel();
}

//---------------------------------------------------------------------------
//
//	�`��
//
//---------------------------------------------------------------------------
void CTKeyDlg::OnPaint()
{
	CPaintDC dc(this);
	CDC mDC;
	CRect rect;
	CBitmap Bitmap;
	CBitmap *pBitmap;

	// ������DC�쐬
	VERIFY(mDC.CreateCompatibleDC(&dc));

	// �݊��r�b�g�}�b�v�쐬
	GetClientRect(&rect);
	VERIFY(Bitmap.CreateCompatibleBitmap(&dc, rect.Width(), rect.Height()));
	pBitmap = mDC.SelectObject(&Bitmap);
	ASSERT(pBitmap);

	// �w�i�N���A
	mDC.FillSolidRect(&rect, ::GetSysColor(COLOR_3DFACE));

	// �`��
	OnDraw(&mDC);

	// BitBlt
	VERIFY(dc.BitBlt(0, 0, rect.Width(), rect.Height(), &mDC, 0, 0, SRCCOPY));

	// �r�b�g�}�b�v�I��
	VERIFY(mDC.SelectObject(pBitmap));
	VERIFY(Bitmap.DeleteObject());

	// ������DC�I��
	VERIFY(mDC.DeleteDC());
}

//---------------------------------------------------------------------------
//
//	�`�惁�C��
//
//---------------------------------------------------------------------------
void FASTCALL CTKeyDlg::OnDraw(CDC *pDC)
{
	HFONT hFont;
	CFont *pFont;
	CFont *pDefFont;

	ASSERT(this);
	ASSERT(pDC);

	// �F�ݒ�
	pDC->SetTextColor(::GetSysColor(COLOR_BTNTEXT));
	pDC->SetBkColor(::GetSysColor(COLOR_3DFACE));

	// �t�H���g�ݒ�
	hFont = (HFONT)::GetStockObject(DEFAULT_GUI_FONT);
	ASSERT(hFont);
	pFont = CFont::FromHandle(hFont);
	pDefFont = pDC->SelectObject(pFont);
	ASSERT(pDefFont);

	// �\��
	pDC->DrawText(m_strGuide, m_rectGuide,
					DT_LEFT | DT_NOPREFIX | DT_WORDBREAK);
	pDC->DrawText(m_strAssign, m_rectAssign,
					DT_LEFT | DT_NOPREFIX | DT_SINGLELINE | DT_VCENTER);
	pDC->DrawText(m_strKey, m_rectKey,
					DT_LEFT | DT_NOPREFIX | DT_SINGLELINE | DT_VCENTER);

	// �t�H���g�߂�(�I�u�W�F�N�g�͍폜���Ȃ��Ă悢)
	pDC->SelectObject(pDefFont);
}

//---------------------------------------------------------------------------
//
//	�_�C�A���O�R�[�h�擾
//
//---------------------------------------------------------------------------
UINT CTKeyDlg::OnGetDlgCode()
{
	// �L�[���b�Z�[�W���󂯎���悤�ɂ���
	return DLGC_WANTMESSAGE;
}

//---------------------------------------------------------------------------
//
//	�^�C�}
//
//---------------------------------------------------------------------------
#if _MFC_VER >= 0x700
void CTKeyDlg::OnTimer(UINT_PTR nID)
#else
void CTKeyDlg::OnTimer(UINT nID)
#endif
{
	BYTE state[0x100];
	int nKey;
	int nTarget;
	LPCTSTR lpszKey;

	// ID�`�F�b�N
	if (m_nTimerID != nID) {
		return;
	}

	// �L�[�擾
	GetKeyboardState(state);

	// ��v���Ă���Εω�����
	if (memcmp(state, m_KeyState, sizeof(state)) == 0) {
		return;
	}

	// �^�[�Q�b�g�̃o�b�N�A�b�v�����
	nTarget = m_nKey;

	// �ω��_��T��B������LBUTTON,RBUTTON�͓���Ȃ�
	for (nKey=3; nKey<0x100; nKey++) {
		// �ȑO������Ă��Ȃ���
		if ((m_KeyState[nKey] & 0x80) == 0) {
			// ���񉟂��ꂽ����
			if (state[nKey] & 0x80) {
				// �^�[�Q�b�g�ݒ�
				nTarget = nKey;
				break;
			}
		}
	}

	// �X�e�[�g�X�V
	memcpy(m_KeyState, state, sizeof(state));

	// �^�[�Q�b�g���ω����Ă��Ȃ���΂Ȃɂ����Ȃ�
	if (m_nKey == nTarget) {
		return;
	}

	// ������擾
	lpszKey = m_pTKey->GetKeyID(nTarget);
	if (lpszKey) {
		// �L�[�����񂪂���̂ŁA�V�K�ݒ�
		m_nKey = nTarget;

		// �R���g���[���ɐݒ�A�ĕ`��
		m_strKey = lpszKey;
		Invalidate(FALSE);
	}
}

//---------------------------------------------------------------------------
//
//	�E�N���b�N
//
//---------------------------------------------------------------------------
void CTKeyDlg::OnRButtonDown(UINT /*nFlags*/, CPoint /*point*/)
{
	// �^�C�}��~
	if (m_nTimerID) {
		KillTimer(m_nTimerID);
		m_nTimerID = NULL;
	}

	// �_�C�A���O�I��
	EndDialog(IDOK);
}

//===========================================================================
//
//	TrueKey�y�[�W
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	�R���X�g���N�^
//
//---------------------------------------------------------------------------
CTKeyPage::CTKeyPage()
{
	CFrmWnd *pFrmWnd;

	// ID,Help��K���ݒ�
	m_dwID = MAKEID('T', 'K', 'E', 'Y');
	m_nTemplate = IDD_TKEYPAGE;
	m_uHelpID = IDC_TKEY_HELP;

	// �C���v�b�g�擾
	pFrmWnd = (CFrmWnd*)AfxGetApp()->m_pMainWnd;
	ASSERT(pFrmWnd);
	m_pInput = pFrmWnd->GetInput();
	ASSERT(m_pInput);

	// TrueKey�擾
	m_pTKey = pFrmWnd->GetTKey();
	ASSERT(m_pTKey);
}

//---------------------------------------------------------------------------
//
//	���b�Z�[�W �}�b�v
//
//---------------------------------------------------------------------------
BEGIN_MESSAGE_MAP(CTKeyPage, CConfigPage)
	ON_BN_CLICKED(IDC_TKEY_WINC, OnSelChange)
	ON_CBN_SELCHANGE(IDC_TKEY_COMC, OnSelChange)
	ON_NOTIFY(NM_CLICK, IDC_TKEY_LIST, OnClick)
	ON_NOTIFY(NM_RCLICK, IDC_TKEY_LIST, OnRClick)
END_MESSAGE_MAP()

//---------------------------------------------------------------------------
//
//	������
//
//---------------------------------------------------------------------------
BOOL CTKeyPage::OnInitDialog()
{
	CComboBox *pComboBox;
	CButton *pButton;
	CString strText;
	int i;
	CDC *pDC;
	TEXTMETRIC tm;
	LONG cx;
	CListCtrl *pListCtrl;
	int nItem;
	LPCTSTR lpszKey;

	// ��{�N���X
	CConfigPage::OnInitDialog();

	// �|�[�g�R���{�{�b�N�X
	pComboBox = (CComboBox*)GetDlgItem(IDC_TKEY_COMC);
	ASSERT(pComboBox);
	pComboBox->ResetContent();
	::GetMsg(IDS_TKEY_NOASSIGN, strText);
	pComboBox->AddString(strText);
	for (i=1; i<=9; i++) {
		strText.Format(_T("COM%d"), i);
		pComboBox->AddString(strText);
	}
	pComboBox->SetCurSel(m_pConfig->tkey_com);

	// RTS���]
	pButton = (CButton*)GetDlgItem(IDC_TKEY_RTS);
	ASSERT(pButton);
	pButton->SetCheck(m_pConfig->tkey_rts);

	// ���[�h
	pButton = (CButton*)GetDlgItem(IDC_TKEY_VMC);
	ASSERT(pButton);
	pButton->SetCheck(m_pConfig->tkey_mode & 1);
	pButton = (CButton*)GetDlgItem(IDC_TKEY_WINC);
	ASSERT(pButton);
	pButton->SetCheck(m_pConfig->tkey_mode >> 1);

	// �e�L�X�g���g���b�N�𓾂�
	pDC = new CClientDC(this);
	::GetTextMetrics(pDC->m_hDC, &tm);
	delete pDC;
	cx = tm.tmAveCharWidth;

	// ���X�g�R���g���[����
	pListCtrl = (CListCtrl*)GetDlgItem(IDC_TKEY_LIST);
	ASSERT(pListCtrl);
	if (::IsJapanese()) {
		// ���{��
		::GetMsg(IDS_TKEY_NO, strText);
		pListCtrl->InsertColumn(0, strText, LVCFMT_LEFT, cx * 4, 0);
		::GetMsg(IDS_TKEY_KEYTOP, strText);
		pListCtrl->InsertColumn(1, strText, LVCFMT_LEFT, cx * 10, 1);
		::GetMsg(IDS_TKEY_VK, strText);
		pListCtrl->InsertColumn(2, strText, LVCFMT_LEFT, cx * 22, 2);
	}
	else {
		// �p��
		::GetMsg(IDS_TKEY_NO, strText);
		pListCtrl->InsertColumn(0, strText, LVCFMT_LEFT, cx * 5, 0);
		::GetMsg(IDS_TKEY_KEYTOP, strText);
		pListCtrl->InsertColumn(1, strText, LVCFMT_LEFT, cx * 12, 1);
		::GetMsg(IDS_TKEY_VK, strText);
		pListCtrl->InsertColumn(2, strText, LVCFMT_LEFT, cx * 18, 2);
	}

	// �A�C�e��������(X68000�����̓}�b�s���O�ɂ�炸�Œ�)
	pListCtrl->DeleteAllItems();
	nItem = 0;
	for (i=0; i<=0x73; i++) {
		// CKeyDispWnd����L�[���̂𓾂�
		lpszKey = m_pInput->GetKeyName(i);
		if (lpszKey) {
			// ���̃L�[�͗L��
			strText.Format(_T("%02X"), i);
			pListCtrl->InsertItem(nItem, strText);
			pListCtrl->SetItemText(nItem, 1, lpszKey);
			pListCtrl->SetItemData(nItem, (DWORD)i);
			nItem++;
		}
	}

	// ���X�g�R���g���[��1�s�S�̃I�v�V����(COMCTL32.DLL v4.71�ȍ~)
	pListCtrl->SendMessage(LVM_SETEXTENDEDLISTVIEWSTYLE, 0, LVS_EX_FULLROWSELECT);

	// VK�}�b�s���O�擾
	m_pTKey->GetKeyMap(m_nKey);

	// ���X�g�R���g���[���X�V
	UpdateReport();

	// �R���g���[���L���ݒ�
	if (m_pConfig->tkey_com == 0) {
		m_bEnableCtrl = TRUE;
		EnableControls(FALSE);
	}
	else {
		m_bEnableCtrl = FALSE;
		EnableControls(TRUE);
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	����
//
//---------------------------------------------------------------------------
void CTKeyPage::OnOK()
{
	CComboBox *pComboBox;
	CButton *pButton;

	// �|�[�g�R���{�{�b�N�X
	pComboBox = (CComboBox*)GetDlgItem(IDC_TKEY_COMC);
	ASSERT(pComboBox);
	m_pConfig->tkey_com = pComboBox->GetCurSel();

	// RTS���]
	pButton = (CButton*)GetDlgItem(IDC_TKEY_RTS);
	ASSERT(pButton);
	m_pConfig->tkey_rts = pButton->GetCheck();

	// ���蓖��
	m_pConfig->tkey_mode = 0;
	pButton = (CButton*)GetDlgItem(IDC_TKEY_VMC);
	ASSERT(pButton);
	if (pButton->GetCheck() == 1) {
		m_pConfig->tkey_mode |= 1;
	}
	pButton = (CButton*)GetDlgItem(IDC_TKEY_WINC);
	ASSERT(pButton);
	if (pButton->GetCheck() == 1) {
		m_pConfig->tkey_mode |= 2;
	}

	// �L�[�}�b�v�ݒ�
	m_pTKey->SetKeyMap(m_nKey);

	// ��{�N���X
	CConfigPage::OnOK();
}

//---------------------------------------------------------------------------
//
//	�R���{�{�b�N�X�ύX
//
//---------------------------------------------------------------------------
void CTKeyPage::OnSelChange()
{
	CComboBox *pComboBox;

	pComboBox = (CComboBox*)GetDlgItem(IDC_TKEY_COMC);
	ASSERT(pComboBox);

	// �R���g���[���L���E����
	if (pComboBox->GetCurSel() > 0) {
		EnableControls(TRUE);
	}
	else {
		EnableControls(FALSE);
	}
}

//---------------------------------------------------------------------------
//
//	�A�C�e���N���b�N
//
//---------------------------------------------------------------------------
void CTKeyPage::OnClick(NMHDR* /*pNMHDR*/, LRESULT* /*pResult*/)
{
	CListCtrl *pListCtrl;
	int nItem;
	int nCount;
	int nKey;
	CTKeyDlg dlg(this);

	// ���X�g�R���g���[���擾
	pListCtrl = (CListCtrl*)GetDlgItem(IDC_TKEY_LIST);
	ASSERT(pListCtrl);

	// �J�E���g�����擾
	nCount = pListCtrl->GetItemCount();

	// �Z���N�g����Ă���C���f�b�N�X�𓾂�
	for (nItem=0; nItem<nCount; nItem++) {
		if (pListCtrl->GetItemState(nItem, LVIS_SELECTED)) {
			break;
		}
	}
	if (nItem >= nCount) {
		return;
	}

	// ���̃C���f�b�N�X�̎w���f�[�^�𓾂�(1�`0x73)
	nKey = (int)pListCtrl->GetItemData(nItem);
	ASSERT((nKey >= 1) && (nKey <= 0x73));

	// �ݒ�J�n
	dlg.m_nTarget = nKey;
	dlg.m_nKey = m_nKey[nKey - 1];

	// �_�C�A���O���s
	if (dlg.DoModal() != IDOK) {
		return;
	}

	// �L�[�}�b�v��ݒ�
	m_nKey[nKey - 1] = dlg.m_nKey;

	// �\�����X�V
	UpdateReport();
}

//---------------------------------------------------------------------------
//
//	�A�C�e���E�N���b�N
//
//---------------------------------------------------------------------------
void CTKeyPage::OnRClick(NMHDR* /*pNMHDR*/, LRESULT* /*pResult*/)
{
	CString strText;
	CString strMsg;
	CListCtrl *pListCtrl;
	int nItem;
	int nCount;
	int nKey;

	// ���X�g�R���g���[���擾
	pListCtrl = (CListCtrl*)GetDlgItem(IDC_TKEY_LIST);
	ASSERT(pListCtrl);

	// �J�E���g�����擾
	nCount = pListCtrl->GetItemCount();

	// �Z���N�g����Ă���C���f�b�N�X�𓾂�
	for (nItem=0; nItem<nCount; nItem++) {
		if (pListCtrl->GetItemState(nItem, LVIS_SELECTED)) {
			break;
		}
	}
	if (nItem >= nCount) {
		return;
	}

	// ���̃C���f�b�N�X�̎w���f�[�^�𓾂�(1�`0x73)
	nKey = (int)pListCtrl->GetItemData(nItem);
	ASSERT((nKey >= 1) && (nKey <= 0x73));

	// ���łɍ폜����Ă���Ή������Ȃ�
	if (m_nKey[nKey - 1] == 0) {
		return;
	}

	// ���b�Z�[�W�{�b�N�X�ŁA�폜�̗L�����`�F�b�N
	::GetMsg(IDS_KBD_DELMSG, strText);
	strMsg.Format(strText, nKey, m_pTKey->GetKeyID(m_nKey[nKey - 1]));
	::GetMsg(IDS_KBD_DELTITLE, strText);
	if (MessageBox(strMsg, strText, MB_ICONQUESTION | MB_YESNO) != IDYES) {
		return;
	}

	// ����
	m_nKey[nKey - 1] = 0;

	// �\�����X�V
	UpdateReport();
}

//---------------------------------------------------------------------------
//
//	���|�[�g�X�V
//
//---------------------------------------------------------------------------
void FASTCALL CTKeyPage::UpdateReport()
{
	CListCtrl *pListCtrl;
	LPCTSTR lpszKey;
	int nItem;
	int nKey;
	int nVK;
	CString strNext;
	CString strPrev;

	ASSERT(this);

	// �R���g���[���擾
	pListCtrl = (CListCtrl*)GetDlgItem(IDC_TKEY_LIST);
	ASSERT(pListCtrl);

	// ���X�g�R���g���[���s
	nItem = 0;
	for (nKey=1; nKey<=0x73; nKey++) {
		// CKeyDispWnd����L�[���̂𓾂�
		lpszKey = m_pInput->GetKeyName(nKey);
		if (lpszKey) {
			// �L���ȃL�[������B������
			strNext.Empty();

			// VK���蓖�Ă�����΁A���̂��擾
			nVK = m_nKey[nKey - 1];
			if (nVK != 0) {
				lpszKey = m_pTKey->GetKeyID(nVK);
				strNext = lpszKey;
			}

			// �قȂ��Ă���Ώ�������
			strPrev = pListCtrl->GetItemText(nItem, 2);
			if (strPrev != strNext) {
				pListCtrl->SetItemText(nItem, 2, strNext);
			}

			// ���̃A�C�e����
			nItem++;
		}
	}
}

//---------------------------------------------------------------------------
//
//	�R���g���[����ԕύX
//
//---------------------------------------------------------------------------
void FASTCALL CTKeyPage::EnableControls(BOOL bEnable) 
{
	int i;
	CWnd *pWnd;
	CButton *pButton;
	BOOL bCheck;

	ASSERT(this);

	// Windows�`�F�b�N�{�b�N�X�擾
	pButton = (CButton*)GetDlgItem(IDC_TKEY_WINC);
	ASSERT(pButton);
	bCheck = FALSE;
	if (pButton->GetCheck() != 0) {
		bCheck = TRUE;
	}

	// �t���O�`�F�b�N
	if (m_bEnableCtrl == bEnable) {
		// FALSE��FALSE�̏ꍇ�̂�return
		if (!m_bEnableCtrl) {
			return;
		}
	}
	m_bEnableCtrl = bEnable;

	// �f�o�C�X�AHelp�ȊO�̑S�R���g���[����ݒ�
	for(i=0; ; i++) {
		// �I���`�F�b�N
		if (ControlTable[i] == NULL) {
			break;
		}

		// �R���g���[���擾
		pWnd = GetDlgItem(ControlTable[i]);
		ASSERT(pWnd);

		// ControlTable[i]��IDC_TKEY_MAPG, IDC_TKEY_LIST�͓���
		switch (ControlTable[i]) {
			// Windows�L�[�}�b�s���O�Ɋւ���R���g���[����
			case IDC_TKEY_MAPG:
			case IDC_TKEY_LIST:
				// bEnable�A����Windows�L�����̂�
				if (bEnable && bCheck) {
					pWnd->EnableWindow(TRUE);
				}
				else {
					pWnd->EnableWindow(FALSE);
				}
				break;

			// ���̑���bEnable�ɏ]��
			default:
				pWnd->EnableWindow(bEnable);
		}
	}
}

//---------------------------------------------------------------------------
//
//	�R���g���[��ID�e�[�u��
//
//---------------------------------------------------------------------------
const UINT CTKeyPage::ControlTable[] = {
	IDC_TKEY_RTS,
	IDC_TKEY_ASSIGNG,
	IDC_TKEY_VMC,
	IDC_TKEY_WINC,
	IDC_TKEY_MAPG,
	IDC_TKEY_LIST,
	NULL
};

//===========================================================================
//
//	���̑��y�[�W
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	�R���X�g���N�^
//
//---------------------------------------------------------------------------
CMiscPage::CMiscPage()
{
	// ID,Help��K���ݒ�
	m_dwID = MAKEID('M', 'I', 'S', 'C');
	m_nTemplate = IDD_MISCPAGE;
	m_uHelpID = IDC_MISC_HELP;
	//int msgboxID = MessageBox("","saves", 2 );

	//CEdit *pEdit;	
	//pEdit = (CEdit*)GetDlgItem(IDC_EDIT1);
 	//pEdit->SetWindowTextA("m_pConfig->ruta_savestate");

}



BEGIN_MESSAGE_MAP(CMiscPage, CConfigPage)
	ON_BN_CLICKED(IDC_BUSCAR, OnBuscarFolder)
END_MESSAGE_MAP()



BOOL CMiscPage::OnInitDialog()
{
	CEdit *pEdit;	
	CConfigPage::OnInitDialog();
	CFrmWnd *pFrmWnd;
	pFrmWnd = (CFrmWnd*)AfxGetApp()->m_pMainWnd;
	
	// �I�v�V����
	pEdit = (CEdit*)GetDlgItem(IDC_EDIT1);
	pEdit->SetWindowTextA(pFrmWnd->RutaSaveStates);
		
	/*int msgboxID = MessageBox(
       m_pConfig->ruta_savestate,"saves",
        2 );	*/

	return TRUE;
}

void CMiscPage::OnBuscarFolder()
{ 
	char *hola= "hola";
	CEdit *pEdit;	
	CFolderPickerDialog folderPickerDialog("c:\\", OFN_FILEMUSTEXIST | OFN_ALLOWMULTISELECT | OFN_ENABLESIZING, this,
        sizeof(OPENFILENAME));
    CString folderPath;
			

    if (folderPickerDialog.DoModal() == IDOK)
    {
        POSITION pos = folderPickerDialog.GetStartPosition();
        while (pos)
        {
            folderPath = folderPickerDialog.GetNextPathName(pos);
        }
    }

	pEdit = (CEdit*)GetDlgItem(IDC_EDIT1);
	pEdit->SetWindowTextA(folderPath);
}

void CMiscPage::OnOK()
{
	CEdit *pEdit;	
	CString  folderDestino;
	pEdit = (CEdit*)GetDlgItem(IDC_EDIT1);
	pEdit->GetWindowTextA(folderDestino);
	//int msgboxID = MessageBox(folderDestino,"saves", 2 );
	_tcscpy(m_pConfig->ruta_savestate, folderDestino);
	CConfigPage::OnOK();
}


//---------------------------------------------------------------------------
//
//	�f�[�^����
//
//---------------------------------------------------------------------------
void CMiscPage::DoDataExchange(CDataExchange *pDX)
{
	ASSERT(this);
	ASSERT(pDX);

	// ��{�N���X
	CConfigPage::DoDataExchange(pDX);

	// �f�[�^����
	DDX_Check(pDX, IDC_MISC_FDSPEED, m_pConfig->floppy_speed);
	DDX_Check(pDX, IDC_MISC_FDLED, m_pConfig->floppy_led);
	DDX_Check(pDX, IDC_MISC_POPUP, m_pConfig->popup_swnd);
	DDX_Check(pDX, IDC_MISC_AUTOMOUSE, m_pConfig->auto_mouse);
	DDX_Check(pDX, IDC_MISC_POWEROFF, m_pConfig->power_off);
}

//===========================================================================
//
//	�R���t�B�O�v���p�e�B�V�[�g
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	�R���X�g���N�^
//
//---------------------------------------------------------------------------
CConfigSheet::CConfigSheet(CWnd *pParent) : CPropertySheet(IDS_OPTIONS, pParent)
{
	// ���̎��_�ł͐ݒ�f�[�^��NULL
	m_pConfig = NULL;

	// �p����ւ̑Ή�
	if (!::IsJapanese()) {
		::GetMsg(IDS_OPTIONS, m_strCaption);
	}

	// Apply�{�^�����폜
	m_psh.dwFlags |= PSH_NOAPPLYNOW;

	// �e�E�B���h�E���L��
	m_pFrmWnd = (CFrmWnd*)pParent;

	// �^�C�}�Ȃ�
	m_nTimerID = NULL;

	// �y�[�W������
	m_Basic.Init(this);
	m_Sound.Init(this);
	m_Vol.Init(this);
	m_Kbd.Init(this);
	m_Mouse.Init(this);
	m_Joy.Init(this);
	m_SASI.Init(this);
	m_SxSI.Init(this);
	m_SCSI.Init(this);
	m_Port.Init(this);
	m_MIDI.Init(this);
	m_Alter.Init(this);
	m_Resume.Init(this);
	m_TKey.Init(this);
	m_Misc.Init(this);
}

//---------------------------------------------------------------------------
//
//	���b�Z�[�W �}�b�v
//
//---------------------------------------------------------------------------
BEGIN_MESSAGE_MAP(CConfigSheet, CPropertySheet)
	ON_WM_CREATE()
	ON_WM_DESTROY()
	ON_WM_TIMER()
END_MESSAGE_MAP()

//---------------------------------------------------------------------------
//
//	�y�[�W����
//
//---------------------------------------------------------------------------
CConfigPage* FASTCALL CConfigSheet::SearchPage(DWORD dwID) const
{
	int nPage;
	int nCount;
	CConfigPage *pPage;

	ASSERT(this);
	ASSERT(dwID != 0);

	// �y�[�W���擾
	nCount = GetPageCount();
	ASSERT(nCount >= 0);

	// �y�[�W���[�v
	for (nPage=0; nPage<nCount; nPage++) {
		// �y�[�W�擾
		pPage = (CConfigPage*)GetPage(nPage);
		ASSERT(pPage);

		// ID�`�F�b�N
		if (pPage->GetID() == dwID) {
			return pPage;
		}
	}

	// ������Ȃ�����
	return NULL;
}

//---------------------------------------------------------------------------
//
//	�E�B���h�E�쐬
//
//---------------------------------------------------------------------------
int CConfigSheet::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	// ��{�N���X
	if (CPropertySheet::OnCreate(lpCreateStruct) != 0) {
		return -1;
	}

	// �^�C�}���C���X�g�[��
	m_nTimerID = SetTimer(IDM_OPTIONS, 100, NULL);

	return 0;
}

//---------------------------------------------------------------------------
//
//	�E�B���h�E�폜
//
//---------------------------------------------------------------------------
void CConfigSheet::OnDestroy()
{
	// �^�C�}��~
	if (m_nTimerID) {
		KillTimer(m_nTimerID);
		m_nTimerID = NULL;
	}

	// ��{�N���X
	CPropertySheet::OnDestroy();
}

//---------------------------------------------------------------------------
//
//	�^�C�}
//
//---------------------------------------------------------------------------
#if _MFC_VER >= 0x700
void CConfigSheet::OnTimer(UINT_PTR nID)
#else
void CConfigSheet::OnTimer(UINT nID)
#endif
{
	CInfo *pInfo;

	ASSERT(m_pFrmWnd);

	// ID�`�F�b�N
	if (m_nTimerID != nID) {
		return;
	}

	// �^�C�}��~
	KillTimer(m_nTimerID);
	m_nTimerID = NULL;

	// Info�����݂���΁A�X�V
	pInfo = m_pFrmWnd->GetInfo();
	if (pInfo) {
		pInfo->UpdateStatus();
		pInfo->UpdateCaption();
		pInfo->UpdateInfo();
	}

	// �^�C�}�ĊJ(�\����������100ms������)
	m_nTimerID = SetTimer(IDM_OPTIONS, 100, NULL);
}

#endif	// _WIN32
