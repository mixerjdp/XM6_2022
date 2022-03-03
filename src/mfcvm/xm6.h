//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2005 �o�h�D(ytanaka@ipc-tokai.or.jp)
//	[ ���ʒ�` ]
//
//---------------------------------------------------------------------------

#if !defined(xm6_h)
#define xm6_h

//---------------------------------------------------------------------------
//
//	��{�萔
//
//---------------------------------------------------------------------------
#if !defined(FALSE)
#define FALSE		0
#define TRUE		(!FALSE)
#endif	// FALSE
#if !defined(NULL)
#define NULL		0
#endif	// NULL

//---------------------------------------------------------------------------
//
//	��{�}�N��
//
//---------------------------------------------------------------------------
#if !defined(ASSERT)
#if !defined(NDEBUG)
#define ASSERT(cond)	assert(cond)
#else
#define ASSERT(cond)	((void)0)
#endif	// NDEBUG
#endif	// ASSERT

#if !defined(ASSERT_DIAG)
#if !defined(NDEBUG)
#define ASSERT_DIAG()	AssertDiag()
#else
#define ASSERT_DIAG()	((void)0)
#endif	// NDEBUG
#endif	// ASSERT_DIAG

//---------------------------------------------------------------------------
//
//	��{�^��`
//
//---------------------------------------------------------------------------
typedef unsigned char BYTE;
typedef unsigned short WORD;
typedef unsigned long DWORD;
typedef int BOOL;

//---------------------------------------------------------------------------
//
//	ID�}�N��
//
//---------------------------------------------------------------------------
#define MAKEID(a, b, c, d)	((DWORD)((a<<24) | (b<<16) | (c<<8) | d))

//---------------------------------------------------------------------------
//
//	�N���X�錾
//
//---------------------------------------------------------------------------
class VM;
										// ���z�}�V��
class Config;
										// �R���t�B�M�����[�V����
class Device;
										// �f�o�C�X�ėp
class MemDevice;
										// �������}�b�v�h�f�o�C�X�ėp
class Log;
										// ���O
class Event;
										// �C�x���g
class Scheduler;
										// �X�P�W���[��
class CPU;
										// CPU MC68000
class Memory;
										// �A�h���X��� OHM2
class Fileio;
										// �t�@�C�����o��
class SRAM;
										// �X�^�e�B�b�NRAM
class SysPort;
										// �V�X�e���|�[�g MESSIAH
class TVRAM;
										// �e�L�X�gVRAM
class VC;
										// �r�f�I�R���g���[�� VIPS&CATHY
class CRTC;
										// CRTC VICON
class RTC;
										// RTC RP5C15
class PPI;
										// PPI i8255A
class DMAC;
										// DMAC HD63450
class MFP;
										// MFP MC68901
class FDC;
										// FDC uPD72065
class FDD;
										// FDD FD55GFR
class IOSC;
										// I/O�R���g���[�� IOSC-2
class SASI;
										// SASI
class Sync;
										// �����I�u�W�F�N�g
class OPMIF;
										// OPM YM2151
class Keyboard;
										// �L�[�{�[�h
class ADPCM;
										// ADPCM MSM6258V
class GVRAM;
										// �O���t�B�b�NVRAM
class Sprite;
										// �X�v���C�gRAM
class SCC;
										// SCC Z8530
class Mouse;
										// �}�E�X
class Printer;
										// �v�����^
class AreaSet;
										// �G���A�Z�b�g
class Render;
										// �����_��
class Windrv;
										// Windrv
class FDI;
										// �t���b�s�[�f�B�X�N�C���[�W
class Disk;
										// SASI/SCSI�f�B�X�N
class MIDI;
										// MIDI YM3802
class Filepath;
										// �t�@�C���p�X
class JoyDevice;
										// �W���C�X�e�B�b�N�f�o�C�X
class FileSys;
										// �t�@�C���V�X�e��
class SCSI;
										// SCSI MB89352
class Mercury;
										// Mercury-Unit

#endif	// xm6_h
