//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 �o�h�D(ytanaka@ipc-tokai.or.jp)
//	[ MFC Subconjunto del ensamblador ]
//
//---------------------------------------------------------------------------

#if defined(_WIN32)

#if !defined (mfc_asm_h)
#define mfc_asm_h

#if defined(__cplusplus)
extern "C" {
#endif	//__cplusplus

//---------------------------------------------------------------------------
//
//	Declaracion del prototipo
//
//---------------------------------------------------------------------------
BOOL IsMMXSupport(void);
										// Comprobacion de la compatibilidad con MMX
BOOL IsCMOVSupport(void);
										// Comprobacion del apoyo de CMOV

void SoundMMX(DWORD *pSrc, WORD *pDst, int nBytes);
										// Dimensionamiento de la muestra de sonido(MMX)
void SoundEMMS();
										// Dimensionamiento de la muestra de sonido(EMMS)

void VideoText(const BYTE *pTVRAM, DWORD *pBits, int nLen, DWORD *pPalette);
										// �e�L�X�gVRAM
void VideoG1024A(const BYTE *src, DWORD *dst, DWORD *plt);
										// �O���t�B�b�NVRAM 1024�~1024(�y�[�W0,1)
void VideoG1024B(const BYTE *src, DWORD *dst, DWORD *plt);
										// �O���t�B�b�NVRAM 1024�~1024(�y�[�W2,3)
void VideoG16A(const BYTE *src, DWORD *dst, int len, DWORD *plt);
										// �O���t�B�b�NVRAM 16�F(�y�[�W0)
void VideoG16B(const BYTE *src, DWORD *dst, int len, DWORD *plt);
										// �O���t�B�b�NVRAM 16�F(�y�[�W1)
void VideoG16C(const BYTE *src, DWORD *dst, int len, DWORD *plt);
										// �O���t�B�b�NVRAM 16�F(�y�[�W2)
void VideoG16D(const BYTE *src, DWORD *dst, int len, DWORD *plt);
										// �O���t�B�b�NVRAM 16�F(�y�[�W3)
void VideoG256A(const BYTE *src, DWORD *dst, int len, DWORD *plt);
										// �O���t�B�b�NVRAM 256�F(�y�[�W0)
void VideoG256B(const BYTE *src, DWORD *dst, int len, DWORD *plt);
										// �O���t�B�b�NVRAM 256�F(�y�[�W1)
void VideoG64K(const BYTE *src, DWORD *dst, int len, DWORD *plt);
										// �O���t�B�b�NVRAM 65536�F
void VideoPCG(BYTE *src, DWORD *dst, DWORD *plt);
										// PCG
void VideoBG16(BYTE *pcg, DWORD *dst, DWORD bg, int y, DWORD *plt);
										// BG 16x16
void VideoBG8(BYTE *pcg, DWORD *dst, DWORD bg, int y, DWORD *plt);
										// BG 8x8

#if defined(__cplusplus)
}
#endif	//__cplusplus

#endif	// mfc_asm_h
#endif	// _WIN32
