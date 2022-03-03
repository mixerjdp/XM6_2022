//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2005 �o�h�D(ytanaka@ipc-tokai.or.jp)
//	[ �����_�� ]
//
//---------------------------------------------------------------------------

#include "os.h"
#include "xm6.h"
#include "vm.h"
#include "crtc.h"
#include "vc.h"
#include "tvram.h"
#include "gvram.h"
#include "sprite.h"
#include "rend_asm.h"
#include "render.h"

//===========================================================================
//
//	�����_��
//
//===========================================================================
//#define REND_LOG

//---------------------------------------------------------------------------
//
//	�萔��`
//
//---------------------------------------------------------------------------
#define REND_COLOR0		0x80000000		// �J���[0�t���O(rend_asm.asm�Ŏg�p)

//---------------------------------------------------------------------------
//
//	�R���X�g���N�^
//
//---------------------------------------------------------------------------
Render::Render(VM *p) : Device(p)
{
	// �f�o�C�XID��������
	dev.id = MAKEID('R', 'E', 'N', 'D');
	dev.desc = "Renderer";

	// �f�o�C�X�|�C���^
	crtc = NULL;
	vc = NULL;
	sprite = NULL;

	// ���[�N�G���A������(CRTC)
	render.crtc = FALSE;
	render.width = 768;
	render.h_mul = 1;
	render.height = 512;
	render.v_mul = 1;

	// ���[�N�G���A������(�p���b�g)
	render.palbuf = NULL;
	render.palptr = NULL;
	render.palvc = NULL;

	// ���[�N�G���A������(�e�L�X�g)
	render.textflag = NULL;
	render.texttv = NULL;
	render.textbuf = NULL;
	render.textout = NULL;

	// ���[�N�G���A������(�O���t�B�b�N)
	render.grpflag = NULL;
	render.grpgv = NULL;
	render.grpbuf[0] = NULL;
	render.grpbuf[1] = NULL;
	render.grpbuf[2] = NULL;
	render.grpbuf[3] = NULL;

	// ���[�N�G���A������(PCG,�X�v���C�g,BG)
	render.pcgbuf = NULL;
	render.spptr = NULL;
	render.bgspbuf = NULL;
	render.zero = NULL;
	render.bgptr[0] = NULL;
	render.bgptr[1] = NULL;

	// ���[�N�G���A������(����)
	render.mixbuf = NULL;
	render.mixwidth = 0;
	render.mixheight = 0;
	render.mixlen = 0;
	render.mixtype = 0;
	memset(render.mixptr, 0, sizeof(render.mixptr));
	memset(render.mixshift, 0, sizeof(render.mixshift));
	memset(render.mixx, 0, sizeof(render.mixx));
	memset(render.mixy, 0, sizeof(render.mixy));
	memset(render.mixand, 0, sizeof(render.mixand));
	memset(render.mixmap, 0, sizeof(render.mixmap));

	// ���[�N�G���A������(�`��)
	render.drawflag = NULL;

	// ���̑�
	cmov = FALSE;
}

//---------------------------------------------------------------------------
//
//	������
//
//---------------------------------------------------------------------------
BOOL FASTCALL Render::Init()
{
	int i;

	ASSERT(this);

	// ��{�N���X
	if (!Device::Init()) {
		return FALSE;
	}

	// CRTC�擾
	crtc = (CRTC*)vm->SearchDevice(MAKEID('C', 'R', 'T', 'C'));
	ASSERT(crtc);

	// VC�擾
	vc = (VC*)vm->SearchDevice(MAKEID('V', 'C', ' ', ' '));
	ASSERT(vc);

	// �p���b�g�o�b�t�@�m��(4MB)
	try {
		render.palbuf = new DWORD[0x10000 * 16];
	}
	catch (...) {
		return FALSE;
	}
	if (!render.palbuf) {
		return FALSE;
	}

	// �e�L�X�gVRAM�o�b�t�@�m��(4.7MB)
	try {
		render.textflag = new BOOL[1024 * 32];
		render.textbuf = new BYTE[1024 * 512];
		render.textout = new DWORD[1024 * (1024 + 1)];
	}
	catch (...) {
		return FALSE;
	}
	if (!render.textflag) {
		return FALSE;
	}
	if (!render.textbuf) {
		return FALSE;
	}
	if (!render.textout) {
		return FALSE;
	}
	for (i=0; i<1024*32; i++) {
		render.textflag[i] = TRUE;
	}
	for (i=0; i<1024; i++) {
		render.textmod[i] = TRUE;
	}

	// �O���t�B�b�NVRAM�o�b�t�@�m��(8.2MB)
	try {
		render.grpflag = new BOOL[512 * 32 * 4];
		render.grpbuf[0] = new DWORD[512 * 1024 * 4];
	}
	catch (...) {
		return FALSE;
	}
	if (!render.grpflag) {
		return FALSE;
	}
	if (!render.grpbuf[0]) {
		return FALSE;
	}
	render.grpbuf[1] = render.grpbuf[0] + 512 * 1024;
	render.grpbuf[2] = render.grpbuf[1] + 512 * 1024;
	render.grpbuf[3] = render.grpbuf[2] + 512 * 1024;
	memset(render.grpflag, 0, sizeof(BOOL) * 32 * 512 * 4);
	for (i=0; i<512*4; i++) {
		render.grpmod[i] = FALSE;
		render.grppal[i] = TRUE;
	}

	// PCG�o�b�t�@�m��(4MB)
	try {
		render.pcgbuf = new DWORD[ 16 * 256 * 16 * 16 ];
	}
	catch (...) {
		return FALSE;
	}
	if (!render.pcgbuf) {
		return FALSE;
	}

	// �X�v���C�g�|�C���^�m��(256KB)
	try {
		render.spptr = new DWORD*[ 128 * 512 ];
	}
	catch (...) {
		return FALSE;
	}
	if (!render.spptr) {
		return FALSE;
	}

	// BG�|�C���^�m��(768KB)
	try {
		render.bgptr[0] = new DWORD*[ (64 * 2) * 1024 ];
		memset(render.bgptr[0], 0, sizeof(DWORD*) * (64 * 2 * 1024));
		render.bgptr[1] = new DWORD*[ (64 * 2) * 1024 ];	// from 512 to 1024 since version2.04
		memset(render.bgptr[1], 0, sizeof(DWORD*) * (64 * 2 * 1024));
	}
	catch (...) {
		return FALSE;
	}
	if (!render.bgptr[0]) {
		return FALSE;
	}
	if (!render.bgptr[1]) {
		return FALSE;
	}
	memset(render.bgall, 0, sizeof(render.bgall));
	memset(render.bgmod, 0, sizeof(render.bgmod));

	// BG/�X�v���C�g�o�b�t�@�m��(1MB)
	try {
		render.bgspbuf = new DWORD[ 512 * 512 + 16];	// +16�͎b��[�u
	}
	catch (...) {
		return FALSE;
	}
	if (!render.bgspbuf) {
		return FALSE;
	}

	// �`��t���O�o�b�t�@�m��(256KB)
	try {
		render.drawflag = new BOOL[64 * 1024];
	}
	catch (...) {
		return FALSE;
	}
	if (!render.drawflag) {
		return FALSE;
	}
	memset(render.drawflag, 0, sizeof(BOOL) * 64 * 1024);

	// �p���b�g�쐬
	MakePalette();

	// ���̑����[�N�G���A
	render.contlevel = 0;
	cmov = ::IsCMOV();

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	�N���[���A�b�v
//
//---------------------------------------------------------------------------
void FASTCALL Render::Cleanup()
{
	int i;

	ASSERT(this);

	// �`��t���O
	if (render.drawflag) {
		delete[] render.drawflag;
		render.drawflag = NULL;
	}

	// BG/�X�v���C�g�o�b�t�@
	if (render.bgspbuf) {
		delete[] render.bgspbuf;
		render.bgspbuf = NULL;
	}

	// BG�|�C���^
	if (render.bgptr[0]) {
		delete[] render.bgptr[0];
		render.bgptr[0] = NULL;
	}
	if (render.bgptr[1]) {
		delete[] render.bgptr[1];
		render.bgptr[1] = NULL;
	}

	// �X�v���C�g�|�C���^
	if (render.spptr) {
		delete[] render.spptr;
		render.spptr = NULL;
	}

	// PCG�o�b�t�@
	if (render.pcgbuf) {
		delete[] render.pcgbuf;
		render.pcgbuf = NULL;
	}

	// �O���t�B�b�NVRAM�o�b�t�@
	if (render.grpflag) {
		delete[] render.grpflag;
		render.grpflag = NULL;
	}
	if (render.grpbuf[0]) {
		delete[] render.grpbuf[0];
		for (i=0; i<4; i++) {
			render.grpbuf[i] = NULL;
		}
	}

	// �e�L�X�gVRAM�o�b�t�@
	if (render.textflag) {
		delete[] render.textflag;
		render.textflag = NULL;
	}
	if (render.textbuf) {
		delete[] render.textbuf;
		render.textbuf = NULL;
	}
	if (render.textout) {
		delete[] render.textout;
		render.textout = NULL;
	}

	// �p���b�g�o�b�t�@
	if (render.palbuf) {
		delete[] render.palbuf;
		render.palbuf = NULL;
	}

	// ��{�N���X��
	Device::Cleanup();
}

//---------------------------------------------------------------------------
//
//	���Z�b�g
//
//---------------------------------------------------------------------------
void FASTCALL Render::Reset()
{
	TVRAM *tvram;
	GVRAM *gvram;
	int i;
	int j;
	int k;
	DWORD **ptr;

	ASSERT(this);
	LOG0(Log::Normal, "���Z�b�g");

	// �r�f�I�R���g���[�����|�C���^�擾
	ASSERT(vc);
	render.palvc = (const WORD*)vc->GetPalette();

	// �e�L�X�gVRAM���|�C���^�擾
	tvram = (TVRAM*)vm->SearchDevice(MAKEID('T', 'V', 'R', 'M'));
	ASSERT(tvram);
	render.texttv = tvram->GetTVRAM();

	// �O���t�B�b�NVRAM���|�C���^�擾
	gvram = (GVRAM*)vm->SearchDevice(MAKEID('G', 'V', 'R', 'M'));
	ASSERT(gvram);
	render.grpgv = gvram->GetGVRAM();

	// �X�v���C�g�R���g���[�����|�C���^�擾
	sprite = (Sprite*)vm->SearchDevice(MAKEID('S', 'P', 'R', ' '));
	ASSERT(sprite);
	render.sprmem = sprite->GetPCG() - 0x8000;

	// ���[�N�G���A������
	render.first = 0;
	render.last = 0;
	render.enable = TRUE;
	render.act = TRUE;
	render.count = 2;

	// ���[�N�G���A������(crtc, vc)
	render.crtc = FALSE;
	render.vc = FALSE;

	// ���[�N�G���A������(�R���g���X�g)
	render.contrast = FALSE;

	// ���[�N�G���A������(�p���b�g)
	render.palette = FALSE;
	render.palptr = render.palbuf;

	// ���[�N�G���A������(�e�L�X�g)
	render.texten = FALSE;
	render.textx = 0;
	render.texty = 0;

	// ���[�N�G���A������(�O���t�B�b�N)
	for (i=0; i<4; i++) {
		render.grpen[i] = FALSE;
		render.grpx[i] = 0;
		render.grpy[i] = 0;
	}
	render.grptype = 4;

	// ���[�N�G���A������(PCG)
	// ���Z�b�g�����BG,Sprite�Ƃ����ׂĕ\�����Ȃ���PCG�͖��g�p
	memset(render.pcgready, 0, sizeof(render.pcgready));
	memset(render.pcguse, 0, sizeof(render.pcguse));
	memset(render.pcgpal, 0, sizeof(render.pcgpal));

	// ���[�N�G���A������(�X�v���C�g)
	memset(render.spptr, 0, sizeof(DWORD*) * 128 * 512);
	memset(render.spreg, 0, sizeof(render.spreg));
	memset(render.spuse, 0, sizeof(render.spuse));

	// ���[�N�G���A������(BG)
	memset(render.bgreg, 0, sizeof(render.bgreg));
	render.bgdisp[0] = FALSE;
	render.bgdisp[1] = FALSE;
	render.bgarea[0] = FALSE;
	render.bgarea[1] = TRUE;
	render.bgsize = FALSE;
	render.bgx[0] = 0;
	render.bgx[1] = 0;
	render.bgy[0] = 0;
	render.bgy[1] = 0;

	// ���[�N�G���A������(BG/�X�v���C�g)
	render.bgspflag = FALSE;
	render.bgspdisp = FALSE;
	memset(render.bgspmod, 0, sizeof(render.bgspmod));

	// BG�̏�������Ԃ�����(���ׂ�0000)
	for (i=0; i<(64*64); i++) {
		render.bgreg[0][i] = 0x10000;
		render.bgreg[1][i] = 0x10000;
	}
	render.pcgready[0] = TRUE;
	render.pcguse[0] = (64 * 64) * 2;
	render.pcgpal[0] = (64 * 64) * 2;
	memset(render.pcgbuf, 0, (16 * 16) * sizeof(DWORD));
	for (i=0; i<64; i++) {
		ptr = &render.bgptr[0][i << 3];
		for (j=0; j<64; j++) {
			for (k=0; k<8; k++) {
				ptr[(k << 7) + 0] = &render.pcgbuf[k << 4];
				ptr[(k << 7) + 1] = (DWORD*)0x10000;
			}
			ptr += 2;
		}
		ptr = &render.bgptr[0][(512 + (i << 3)) << 7];
		for (j=0; j<64; j++) {
			for (k=0; k<8; k++) {
				ptr[(k << 7) + 0] = &render.pcgbuf[k << 4];
				ptr[(k << 7) + 1] = (DWORD*)0x10000;
			}
			ptr += 2;
		}
		ptr = &render.bgptr[1][i << 10];
		for (j=0; j<64; j++) {
			for (k=0; k<8; k++) {
				ptr[(k << 7) + 0] = &render.pcgbuf[k << 4];
				ptr[(k << 7) + 1] = (DWORD*)0x10000;
			}
			ptr += 2;
		}
	}

	// ���[�N�G���A������(����)
	render.mixtype = 0;
}

//---------------------------------------------------------------------------
//
//	�Z�[�u
//
//---------------------------------------------------------------------------
BOOL FASTCALL Render::Save(Fileio *fio, int ver)
{
	ASSERT(this);
	LOG0(Log::Normal, "�Z�[�u");

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	���[�h
//
//---------------------------------------------------------------------------
BOOL FASTCALL Render::Load(Fileio *fio, int ver)
{
	ASSERT(this);
	LOG0(Log::Normal, "���[�h");

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	�ݒ�K�p
//
//---------------------------------------------------------------------------
void FASTCALL Render::ApplyCfg(const Config *config)
{
	ASSERT(config);
	LOG0(Log::Normal, "�ݒ�K�p");
}

//---------------------------------------------------------------------------
//
//	�t���[���J�n
//
//---------------------------------------------------------------------------
void FASTCALL Render::StartFrame()
{
	CRTC::crtc_t crtcdata;
	int i;

	ASSERT(this);

	// ���̃t���[���̓X�L�b�v���邩
	if ((render.count != 0) || !render.enable) {
		render.act = FALSE;
		return;
	}

	// ���̃t���[���̓����_�����O����
	render.act = TRUE;

	// ���X�^���N���A
	render.first = 0;
	render.last = -1;

	// CRTC�t���O������
	if (render.crtc) {
#if defined(REND_LOG)
		LOG0(Log::Normal, "CRTC����");
#endif	// REND_LOG

		// �f�[�^�擾
		crtc->GetCRTC(&crtcdata);

		// h_dots�Av_dots��0�Ȃ�ۗ�
		if ((crtcdata.h_dots == 0) || (crtcdata.v_dots == 0)) {
			return;
		}

		// �����R�s�[
		render.width = crtcdata.h_dots;
		render.h_mul = crtcdata.h_mul;
		render.height = crtcdata.v_dots;
		render.v_mul = crtcdata.v_mul;
		render.lowres = crtcdata.lowres;
		if ((render.v_mul == 2) && !render.lowres) {
			render.height >>= 1;
		}

		// �����o�b�t�@�̏������𒲐�
		render.mixlen = render.width;
		if (render.mixwidth < render.width) {
			render.mixlen = render.mixwidth;
		}

		// �X�v���C�g���Z�b�g(mixlen�Ɉˑ����邽��)
		SpriteReset();

		// �S���C������
		for (i=0; i<1024; i++) {
			render.mix[i] = TRUE;
		}

		// �I�t
		render.crtc = FALSE;
	}
}

//---------------------------------------------------------------------------
//
//	�t���[���I��
//
//---------------------------------------------------------------------------
void FASTCALL Render::EndFrame()
{
	ASSERT(this);

	// �����Ȃ牽�����Ȃ�
	if (!render.act) {
		return;
	}

	// �����܂ł̃��X�^������
	if (render.last > 0) {
		render.last = render.height;
		Process();
	}

	// �J�E���gUp
	render.count++;

	// ������
	render.act = FALSE;
}

//---------------------------------------------------------------------------
//
//	�����o�b�t�@�Z�b�g
//
//---------------------------------------------------------------------------
void FASTCALL Render::SetMixBuf(DWORD *buf, int width, int height)
{
	int i;

	ASSERT(this);
	ASSERT(width >= 0);
	ASSERT(height >= 0);

	// �ݒ�
	render.mixbuf = buf;
	render.mixwidth = width;
	render.mixheight = height;

	// �����o�b�t�@�̏������𒲐�
	render.mixlen = render.width;
	if (render.mixwidth < render.width) {
		render.mixlen = render.mixwidth;
	}

	// ���ׂĂ̍������w��
	for (i=0; i<1024; i++) {
		render.mix[i] = TRUE;
	}
}

//---------------------------------------------------------------------------
//
//	CRTC�Z�b�g
//
//---------------------------------------------------------------------------
void FASTCALL Render::SetCRTC()
{
	ASSERT(this);

	// �t���OON�̂�
	render.crtc = TRUE;
	render.vc = TRUE;
}

//---------------------------------------------------------------------------
//
//	VC�Z�b�g
//
//---------------------------------------------------------------------------
void FASTCALL Render::SetVC()
{
	ASSERT(this);

	// �t���OON�̂�
	render.vc = TRUE;
}

//---------------------------------------------------------------------------
//
//	VC����
//
//---------------------------------------------------------------------------
void FASTCALL Render::Video()
{
	const VC::vc_t *p;
	const CRTC::crtc_t *q;
	int type;
	int i;
	int j;
	int sp;
	int gr;
	int tx;
	int map[4];
	DWORD *ptr[4];
	DWORD shift[4];
	DWORD an[4];

	// VC�t���O���~�낷
	render.vc = FALSE;

	// �t���OON
	for (i=0; i<1024; i++) {
		render.mix[i] = TRUE;
	}

	// VC�f�[�^�ACRTC�f�[�^���擾
	p = vc->GetWorkAddr();
	q = crtc->GetWorkAddr();

	// �e�L�X�g�C�l�[�u��
	if (p->ton && !q->tmem) {
		render.texten = TRUE;
	}
	else {
		render.texten = FALSE;
	}

	// �O���t�B�b�N�^�C�v
	type = 0;
	if (!p->siz) {
		type = (int)(p->col + 1);
	}
	if (type != render.grptype) {
		render.grptype = type;
		for (i=0; i<512*4; i++) {
			render.grppal[i] = TRUE;
		}
	}

	// �O���t�B�b�N�C�l�[�u���A�D��x�}�b�v
	render.mixpage = 0;
	for (i=0; i<4; i++) {
		render.grpen[i] = FALSE;
		map[i] = -1;
		an[i] = 512 - 1;
	}
	render.mixpage = 0;
	if (!q->gmem) {
		switch (render.grptype) {
			// 1024x1024x1
			case 0:
				render.grpen[0] = p->gon;
				if (render.grpen[0]) {
					map[0] = 0;
					render.mixpage = 1;
					an[0] = 1024 - 1;
				}
				break;
			// 512x512x4
			case 1:
				for (i=0; i<4; i++) {
					if (p->gs[i]) {
						ASSERT((p->gp[i] >= 0) && (p->gp[i] < 4));
						render.grpen[ p->gp[i] ] = TRUE;
						map[i] = p->gp[i];
						render.mixpage++;
					}
				}
				break;
			// 512x512x2
			case 2:
				for (i=0; i<2; i++) {
					// �y�[�W0�̃`�F�b�N
					if ((p->gp[i * 2 + 0] == 0) && (p->gp[i * 2 + 1] == 1)) {
						if (p->gs[i * 2 + 0] && p->gs[i * 2 + 1]) {
							map[i] = 0;
							render.grpen[0] = TRUE;
							render.mixpage++;
						}
					}
					// �y�[�W1�̃`�F�b�N
					if ((p->gp[i * 2 + 0] == 2) && (p->gp[i * 2 + 1] == 3)) {
						if (p->gs[i * 2 + 0] && p->gs[i * 2 + 1]) {
							map[i] = 2;
							render.grpen[2] = TRUE;
							render.mixpage++;
						}
					}
				}
				break;
			// 512x512x1
			case 3:
			case 4:
				render.grpen[0] = TRUE;
				render.mixpage = 1;
				map[0] = 0;
				for (i=0; i<4; i++) {
					if (!p->gs[i]) {
						render.grpen[0] = FALSE;
						render.mixpage = 0;
						map[0] = -1;
						break;
					}
				}
				break;
			default:
				ASSERT(FALSE);
				break;
		}
	}

	// �O���t�B�b�N�o�b�t�@���Z�b�g
	j = 0;
	for (i=0; i<4; i++) {
		if (map[i] >= 0) {
			ASSERT((map[i] >= 0) && (map[i] <= 3));
			ptr[j] = render.grpbuf[ map[i] ];
			if (render.grptype == 0) {
				shift[j] = 11;
			}
			else {
				shift[j] = 10;
			}
			ASSERT(j <= i);
			map[j] = map[i];
			j++;
		}
	}

	// �D�揇�ʂ��擾
	tx = p->tx;
	sp = p->sp;
	gr = p->gr;

	// �^�C�v������
	render.mixtype = 0;

	// BG/�X�v���C�g�\���ؑւ�
	if ((q->hd >= 2) || (!p->son)) {
		if (render.bgspflag) {
			// BG/�X�v���C�g�\��ON->OFF
			render.bgspflag = FALSE;
			for (i=0; i<512; i++) {
				render.bgspmod[i] = TRUE;
			}
			render.bgspdisp = sprite->IsDisplay();
		}
	}
	else {
		if (!render.bgspflag) {
			// BG/�X�v���C�g�\��OFF->ON
			render.bgspflag = TRUE;
			for (i=0; i<512; i++) {
				render.bgspmod[i] = TRUE;
			}
			render.bgspdisp = sprite->IsDisplay();
		}
	}

	// �ݒ�(q->hd >= 2�̏ꍇ�̓X�v���C�g�ʂȂ�)
	if ((q->hd >= 2) || (!p->son)) {
		// �X�v���C�g�Ȃ�
		if (!render.texten) {
			// �e�L�X�g�Ȃ�
			if (render.mixpage == 0) {
				// �O���t�B�b�N�Ȃ�(type=0)
				render.mixtype = 0;
				return;
			}
			if (render.mixpage == 1) {
				// �O���t�B�b�N1�ʂ̂�(type=1)
				render.mixptr[0] = ptr[0];
				render.mixshift[0] = shift[0];
				render.mixx[0] = &render.grpx[ map[0] ];
				render.mixy[0] = &render.grpy[ map[0] ];
				render.mixand[0] = an[0];
				render.mixtype = 1;
				return;
			}
			if (render.mixpage == 2) {
				// �O���t�B�b�N2�ʂ̂�(type=2)
				for (i=0; i<2; i++) {
					render.mixptr[i] = ptr[i];
					render.mixshift[i] = shift[i];
					render.mixx[i] = &render.grpx[ map[i] ];
					render.mixy[i] = &render.grpy[ map[i] ];
					render.mixand[i] = an[i];
				}
				render.mixtype = 2;
				return;
			}
			ASSERT((render.mixpage == 3) || (render.mixpage == 4));
			// �O���t�B�b�N3�ʈȏ�̂�(type=4)
			for (i=0; i<render.mixpage; i++) {
				render.mixptr[i + 4] = ptr[i];
				render.mixshift[i + 4] = shift[i];
				render.mixx[i + 4] = &render.grpx[ map[i] ];
				render.mixy[i + 4] = &render.grpy[ map[i] ];
				render.mixand[i + 4] = an[i];
			}
			render.mixtype = 4;
			return;
		}
		// �e�L�X�g����
		if (render.mixpage == 0) {
			// �O���t�B�b�N�Ȃ��B�e�L�X�g�̂�(type=1)
			render.mixptr[0] = render.textout;
			render.mixshift[0] = 10;
			render.mixx[0] = &render.textx;
			render.mixy[0] = &render.texty;
			render.mixand[0] = 1024 - 1;
			render.mixtype = 1;
				return;
		}
		if (render.mixpage == 1) {
			// �e�L�X�g+�O���t�B�b�N1��
			if (tx < gr) {
				// �e�L�X�g�O��(type=3)
				render.mixptr[0] = render.textout;
				render.mixshift[0] = 10;
				render.mixx[0] = &render.textx;
				render.mixy[0] = &render.texty;
				render.mixand[0] = 1024 - 1;
				render.mixptr[1] = ptr[0];
				render.mixshift[1] = shift[0];
				render.mixx[1] = &render.grpx[ map[0] ];
				render.mixy[1] = &render.grpy[ map[0] ];
				render.mixand[1] = an[0];
				render.mixtype = 3;
				return;
			}
			// �O���t�B�b�N�O��(type=3,tx=gr�̓O���t�B�b�N�O�ʁA��헪II)
			render.mixptr[1] = render.textout;
			render.mixshift[1] = 10;
			render.mixx[1] = &render.textx;
			render.mixy[1] = &render.texty;
			render.mixand[1] = 1024 - 1;
			render.mixptr[0] = ptr[0];
			render.mixshift[0] = shift[0];
			render.mixx[0] = &render.grpx[ map[0] ];
			render.mixy[0] = &render.grpy[ map[0] ];
			render.mixand[0] = an[0];
			render.mixtype = 3;
			return;
		}
		// �e�L�X�g+�O���t�B�b�N2�ʈȏ�(type=5, type=6)
		ASSERT((render.mixpage >= 2) && (render.mixpage <= 6));
		render.mixptr[0] = render.textout;
		render.mixshift[0] = 10;
		render.mixx[0] = &render.textx;
		render.mixy[0] = &render.texty;
		render.mixand[0] = 1024 - 1;
		for (i=0; i<render.mixpage; i++) {
			render.mixptr[i + 4] = ptr[i];
			render.mixshift[i + 4] = shift[i];
			render.mixx[i + 4] = &render.grpx[ map[i] ];
			render.mixy[i + 4] = &render.grpy[ map[i] ];
			render.mixand[i + 4] = an[i];
		}
		if (tx < gr) {
			render.mixtype = 5;
		}
		else {
			render.mixtype = 6;
		}
		return;
	}

	// �X�v���C�g����
	if (!render.texten) {
		// �e�L�X�g�Ȃ�
		if (render.mixpage == 0) {
			// �O���t�B�b�N�Ȃ��A�X�v���C�g�̂�(type=1)
			render.mixptr[0] = render.bgspbuf;
			render.mixshift[0] = 9;
			render.mixx[0] = &render.zero;
			render.mixy[0] = &render.zero;
			render.mixand[0] = 512 - 1;
			render.mixtype = 1;
			return;
		}
		if (render.mixpage == 1) {
			// �X�v���C�g+�O���t�B�b�N1��(type=3)
			if (sp < gr) {
				// �X�v���C�g�O��
				render.mixptr[0] = render.bgspbuf;
				render.mixshift[0] = 9;
				render.mixx[0] = &render.zero;
				render.mixy[0] = &render.zero;
				render.mixand[0] = 512 - 1;
				render.mixptr[1] = ptr[0];
				render.mixshift[1] = shift[0];
				render.mixx[1] = &render.grpx[ map[0] ];
				render.mixy[1] = &render.grpy[ map[0] ];
				render.mixand[1] = an[0];
				render.mixtype = 3;
				return;
			}
			// �O���t�B�b�N�O��(sp=gr�͕s��)
			render.mixptr[1] = render.bgspbuf;
			render.mixshift[1] = 9;
			render.mixx[1] = &render.zero;
			render.mixy[1] = &render.zero;
			render.mixand[1] = 512 - 1;
			render.mixptr[0] = ptr[0];
			render.mixshift[0] = shift[0];
			render.mixx[0] = &render.grpx[ map[0] ];
			render.mixy[0] = &render.grpy[ map[0] ];
			render.mixand[0] = an[0];
			render.mixtype = 3;
			return;
		}
		// �X�v���C�g+�O���t�B�b�N2�ʈȏ�(type=5, type=6)
		ASSERT((render.mixpage >= 2) && (render.mixpage <= 4));
		render.mixptr[0] = render.bgspbuf;
		render.mixshift[0] = 9;
		render.mixx[0] = &render.zero;
		render.mixy[0] = &render.zero;
		render.mixand[0] = 512 - 1;
		for (i=0; i<render.mixpage; i++) {
			render.mixptr[i + 4] = ptr[i];
			render.mixshift[i + 4] = shift[i];
			render.mixx[i + 4] = &render.grpx[ map[i] ];
			render.mixy[i + 4] = &render.grpy[ map[i] ];
			render.mixand[i + 4] = an[i];
		}
		if (sp < gr) {
			render.mixtype = 5;
		}
		else {
			render.mixtype = 6;
		}
		return;
	}

	// �e�L�X�g����
	if (render.mixpage == 0) {
		// �O���t�B�b�N�Ȃ��B�e�L�X�g�{�X�v���C�g(type=3)
		if (tx <= sp) {
			// tx=sp�̓e�L�X�g�O��(LMZ2)
			render.mixptr[0] = render.textout;
			render.mixshift[0] = 10;
			render.mixx[0] = &render.textx;
			render.mixy[0] = &render.texty;
			render.mixand[0] = 1024 - 1;
			render.mixptr[1] = render.bgspbuf;
			render.mixshift[1] = 9;
			render.mixx[1] = &render.zero;
			render.mixy[1] = &render.zero;
			render.mixand[1] = 512 - 1;
			render.mixtype = 3;
			return;
		}
		// �X�v���C�g�O��
		render.mixptr[1] = render.textout;
		render.mixshift[1] = 10;
		render.mixx[1] = &render.textx;
		render.mixy[1] = &render.texty;
		render.mixand[1] = 1024 - 1;
		render.mixptr[0] = render.bgspbuf;
		render.mixshift[0] = 9;
		render.mixx[0] = &render.zero;
		render.mixy[0] = &render.zero;
		render.mixand[0] = 512 - 1;
		render.mixtype = 3;
		return;
	}

	// �D�揇�ʌ���
	if (tx == 3) tx--;
	if (sp == 3) sp--;
	if (gr == 3) gr--;
	if (tx == sp) {
		// �K���Ɍ��߂Ă���
		if (tx < gr) {
			tx = 0;
			sp = 1;
			gr = 2;
		}
		else {
			gr = 0;
			tx = 1;
			sp = 2;
		}
	}
	if (tx == gr) {
		// �K���Ɍ��߂Ă���
		if (tx < sp) {
			tx = 0;
			gr = 1;
			sp = 2;
		}
		else {
			sp = 0;
			tx = 1;
			gr = 2;
		}
	}
	if (sp == gr) {
		// �K���Ɍ��߂Ă���
		if (sp < tx) {
			sp = 0;
			gr = 1;
			tx = 2;
		}
		else {
			tx = 0;
			sp = 1;
			gr = 2;
		}
	}
	ASSERT((tx != gr) && (gr != sp) && (tx != sp));
	ASSERT((tx >= 0) && (tx < 3));
	ASSERT((sp >= 0) && (sp < 3));
	ASSERT((gr >= 0) && (gr < 3));
	render.mixmap[tx] = 0;
	render.mixmap[sp] = 1;
	render.mixmap[gr] = 2;

	if (render.mixpage == 1) {
		// �e�L�X�g�{�X�v���C�g�{�O���t�B�b�N1��(type=7)
		render.mixptr[0] = render.textout;
		render.mixshift[0] = 10;
		render.mixx[0] = &render.textx;
		render.mixy[0] = &render.texty;
		render.mixand[0] = 1024 - 1;
		render.mixptr[1] = render.bgspbuf;
		render.mixshift[1] = 9;
		render.mixx[1] = &render.zero;
		render.mixy[1] = &render.zero;
		render.mixand[1] = 512 - 1;
		render.mixptr[2] = ptr[0];
		render.mixshift[2] = shift[0];
		render.mixx[2] = &render.grpx[ map[0] ];
		render.mixy[2] = &render.grpy[ map[0] ];
		render.mixand[2] = an[0];
		render.mixtype = 7;
		return;
	}

	// �e�L�X�g�{�X�v���C�g�{�O���t�B�b�N�Q�ʈȏ�(type=8)
	render.mixptr[0] = render.textout;
	render.mixshift[0] = 10;
	render.mixx[0] = &render.textx;
	render.mixy[0] = &render.texty;
	render.mixand[0] = 1024 - 1;
	render.mixptr[1] = render.bgspbuf;
	render.mixshift[1] = 9;
	render.mixx[1] = &render.zero;
	render.mixy[1] = &render.zero;
	render.mixand[1] = 512 - 1;
	for (i=0; i<render.mixpage; i++) {
		render.mixptr[i + 4] = ptr[i];
		render.mixshift[i + 4] = shift[i];
		render.mixx[i + 4] = &render.grpx[ map[i] ];
		render.mixy[i + 4] = &render.grpy[ map[i] ];
		render.mixand[i + 4] = an[i];
	}
	render.mixtype = 8;
}

//---------------------------------------------------------------------------
//
//	�R���g���X�g�ݒ�
//
//---------------------------------------------------------------------------
void FASTCALL Render::SetContrast(int cont)
{
	// �V�X�e���|�[�g�̎��_�ň�v�`�F�b�N���s���̂ŁA�قȂ��Ă���ꍇ�̂�
	ASSERT(this);
	ASSERT((cont >= 0) && (cont <= 15));

	// �ύX�ƃt���OON
	render.contlevel = cont;
	render.contrast = TRUE;
}

//---------------------------------------------------------------------------
//
//	�R���g���X�g�擾
//
//---------------------------------------------------------------------------
int FASTCALL Render::GetContrast() const
{
	ASSERT(this);
	ASSERT((render.contlevel >= 0) && (render.contlevel <= 15));

	return render.contlevel;
}

//---------------------------------------------------------------------------
//
//	�R���g���X�g����
//
//---------------------------------------------------------------------------
void FASTCALL Render::Contrast()
{
	int i;

	// �|�C���g�ʒu��ύX�A�t���ODown
	render.palptr = render.palbuf;
	render.palptr += (render.contlevel << 16);
	render.contrast = FALSE;

	// �p���b�g�t���O��S��Up
	for (i=0; i<0x200; i++) {
		render.palmod[i] = TRUE;
	}
	render.palette = TRUE;
}

//---------------------------------------------------------------------------
//
//	�p���b�g�쐬
//
//---------------------------------------------------------------------------
void FASTCALL Render::MakePalette()
{
	DWORD *p;
	int ratio;
	int i;
	int j;

	ASSERT(render.palbuf);

	// ������
	p = render.palbuf;

	// �R���g���X�g���[�v
	for (i=0; i<16; i++) {
		// �䗦���Z�o
		ratio = 256 - ((15 - i) << 4);

		// �쐬���[�v
		for (j=0; j<0x10000; j++) {
			*p++ = ConvPalette(j, ratio);
		}
	}
}

//---------------------------------------------------------------------------
//
//	�p���b�g�ϊ�
//
//---------------------------------------------------------------------------
DWORD FASTCALL Render::ConvPalette(int color, int ratio)
{
	DWORD r;
	DWORD g;
	DWORD b;

	// assert
	ASSERT((color >= 0) && (color < 0x10000));
	ASSERT((ratio >= 0) && (ratio <= 0x100));

	// �S�ăR�s�[
	r = (DWORD)color;
	g = (DWORD)color;
	b = (DWORD)color;

	// MSB����G:5�AR:5�AB:5�AI:1�̏��ɂȂ��Ă���
	// ����� R:8 G:8 B:8��DWORD�ɕϊ��Bb31-b24�͎g��Ȃ�
	r <<= 13;
	r &= 0xf80000;
	g &= 0x00f800;
	b <<= 2;
	b &= 0x0000f8;

	// �P�x�r�b�g�͈ꗥUp(���f�[�^��0�̏ꍇ���A!=0�ɂ�����ʂ���)
	if (color & 1) {
		r |= 0x070000;
		g |= 0x000700;
		b |= 0x000007;
	}

	// �R���g���X�g���e��������
	b *= ratio;
	b >>= 8;
	g *= ratio;
	g >>= 8;
	g &= 0xff00;
	r *= ratio;
	r >>= 8;
	r &= 0xff0000;

	return (DWORD)(r | g | b);
}

//---------------------------------------------------------------------------
//
//	�p���b�g�擾
//
//---------------------------------------------------------------------------
const DWORD* FASTCALL Render::GetPalette() const
{
	ASSERT(this);
	ASSERT(render.paldata);

	return render.paldata;
}

//---------------------------------------------------------------------------
//
//	�p���b�g����
//
//---------------------------------------------------------------------------
void FASTCALL Render::Palette()
{
	DWORD data;
	BOOL tx;
	BOOL gr;
	BOOL sp;
	int i;
	int j;

	// �t���OOFF
	tx = FALSE;
	gr = FALSE;
	sp = FALSE;

	// �O���t�B�b�N
	for (i=0; i<0x100; i++) {
		if (render.palmod[i]) {
			data = (DWORD)render.palvc[i];
			render.paldata[i] = render.palptr[data];

			// �O���t�B�b�N�ɉe���A�t���OOFF
			gr = TRUE;
			render.palmod[i] = FALSE;

			// �����F�̏���
			if (i == 0) {
				render.paldata[i] |= REND_COLOR0;
			}

			// 65536�F�̂��߂̃p���b�g�f�[�^�ݒ�
			j = i >> 1;
			if (i & 1) {
				j += 128;
			}
			render.pal64k[j * 2 + 0] = (BYTE)(data >> 8);
			render.pal64k[j * 2 + 1] = (BYTE)data;
		}
	}

	// �e�L�X�g���X�v���C�g
	for (i=0x100; i<0x110; i++) {
		if (render.palmod[i]) {
			data = (DWORD)render.palvc[i];
			render.paldata[i] = render.palptr[data];

			// �e�L�X�g�ɉe���A�t���OOFF
			tx = TRUE;
			render.palmod[i] = FALSE;

			// �����F�̏���
			if (i == 0x100) {
				render.paldata[i] |= REND_COLOR0;
				// 0x100��BG�E�X�v���C�g�ɂ��K���e��
				sp = TRUE;
			}

			// PCG����
			memset(&render.pcgready[0], 0, sizeof(BOOL) * 256);
			if (render.pcgpal[0] > 0) {
				sp = TRUE;
			}
		}
	}

	// �X�v���C�g
	for (i=0x110; i<0x200; i++) {
		if (render.palmod[i]) {
			// �X�v���C�g�ɉe���A�t���OOFF
			data = (DWORD)render.palvc[i];
			render.paldata[i] = render.palptr[data];
			render.palmod[i] = FALSE;

			// �����F�̏���
			if ((i & 0x00f) == 0) {
				render.paldata[i] |= REND_COLOR0;
			}

			// PCG����
			memset(&render.pcgready[(i & 0xf0) << 4], 0, sizeof(BOOL) * 256);
			if (render.pcgpal[(i & 0xf0) >> 4] > 0) {
				sp = TRUE;
			}
		}
	}

	// �O���t�B�b�N�t���O
	if (gr) {
		// �t���OON
		for (i=0; i<512*4; i++) {
			render.grppal[i] = TRUE;
		}
	}

	// �e�L�X�g�t���O
	if (tx) {
		for (i=0; i<1024; i++) {
			render.textpal[i] = TRUE;
		}
	}

	// �X�v���C�g�t���O
	if (sp) {
		for (i=0; i<512; i++) {
			render.bgspmod[i] = TRUE;
		}
	}

	// �p���b�g�t���OOFF
	render.palette = FALSE;
}

//---------------------------------------------------------------------------
//
//	�e�L�X�g�X�N���[��
//
//---------------------------------------------------------------------------
void FASTCALL Render::TextScrl(DWORD x, DWORD y)
{
	int i;

	ASSERT(this);
	ASSERT(x < 1024);
	ASSERT(y < 1024);

	// ��r�`�F�b�N
	if ((render.textx == x) && (render.texty == y)) {
		return;
	}

	// ���[�N�X�V
	render.textx = x;
	render.texty = y;

	// �t���OON
	if (render.texten) {
#if defined(REND_LOG)
		LOG2(Log::Normal, "�e�L�X�g�X�N���[�� x=%d y=%d", x, y);
#endif	// REND_LOG

		for (i=0; i<1024; i++) {
			render.mix[i] = TRUE;
		}
	}
}

//---------------------------------------------------------------------------
//
//	�e�L�X�g�R�s�[
//
//---------------------------------------------------------------------------
void FASTCALL Render::TextCopy(DWORD src, DWORD dst, DWORD plane)
{
	ASSERT(this);
	ASSERT((src >= 0) && (src < 256));
	ASSERT((dst >= 0) && (dst < 256));
	ASSERT(plane < 16);

	// �A�Z���u���T�u
	RendTextCopy(&render.texttv[src << 9],
				 &render.texttv[dst << 9],
				 plane,
				 &render.textflag[dst << 7],
				 &render.textmod[dst << 2]);
}

//---------------------------------------------------------------------------
//
//	�e�L�X�g�o�b�t�@�擾
//
//---------------------------------------------------------------------------
const DWORD* FASTCALL Render::GetTextBuf() const
{
	ASSERT(this);
	ASSERT(render.textout);

	return render.textout;
}

//---------------------------------------------------------------------------
//
//	�e�L�X�g����
//
//---------------------------------------------------------------------------
void FASTCALL Render::Text(int raster)
{
	int y;

	// assert
	ASSERT((raster >= 0) && (raster < 1024));
	ASSERT(render.texttv);
	ASSERT(render.textflag);
	ASSERT(render.textbuf);
	ASSERT(render.palbuf);

	// �f�B�Z�[�u���Ȃ牽�����Ȃ�
	if (!render.texten) {
		return;
	}

	// �����Y�Z�o
	y = (raster + render.texty) & 0x3ff;

	// �ύX�t���O(�����^)
	if (render.textmod[y]) {
		// �t���O����
		render.textmod[y] = FALSE;
		render.mix[raster] = TRUE;

		// ���������ϊ�
		RendTextMem(render.texttv + (y << 7),
					render.textflag + (y << 5),
					render.textbuf + (y << 9));

		// �����p���b�g�ϊ�
		RendTextPal(render.textbuf + (y << 9),
					render.textout + (y << 10),
					render.textflag + (y << 5),
					render.paldata + 0x100);
	}

	// �p���b�g(�ꊇ�^)
	if (render.textpal[y]) {
		// �t���O����
		render.textpal[y] = FALSE;

		// �����p���b�g�ϊ�
		RendTextAll(render.textbuf + (y << 9),
					render.textout + (y << 10),
					render.paldata + 0x100);
		render.mix[raster] = TRUE;

		// y == 1023�Ȃ�R�s�[����
		if (y == 1023) {
			memcpy(render.textout + (1024 << 10), render.textout + (1023 << 10), sizeof(DWORD) * 1024);
		}
	}
}

//---------------------------------------------------------------------------
//
//	�O���t�B�b�N�o�b�t�@�擾
//
//---------------------------------------------------------------------------
const DWORD* FASTCALL Render::GetGrpBuf(int index) const
{
	ASSERT(this);
	ASSERT((index >= 0) && (index <= 3));

	ASSERT(render.grpbuf[index]);
	return render.grpbuf[index];
}

//---------------------------------------------------------------------------
//
//	�O���t�B�b�N�X�N���[��
//
//---------------------------------------------------------------------------
void FASTCALL Render::GrpScrl(int block, DWORD x, DWORD y)
{
	BOOL flag;
	int i;

	ASSERT(this);
	ASSERT((block >= 0) && (block <= 3));
	ASSERT(x < 1024);
	ASSERT(y < 1024);

	// ��r�`�F�b�N�B��\���Ȃ�X�V�Ȃ�
	flag = FALSE;
	if ((render.grpx[block] != x) || (render.grpy[block] != y)) {
		render.grpx[block] = x;
		render.grpy[block] = y;
		flag = render.grpen[block];
	}

	// �t���O����
	if (!flag) {
		return;
	}

#if defined(REND_LOG)
	LOG3(Log::Normal, "�O���t�B�b�N�X�N���[�� block=%d x=%d y=%d", block, x, y);
#endif	// REND_LOG

	for (i=0; i<1024; i++) {
		render.mix[i] = TRUE;
	}
}

//---------------------------------------------------------------------------
//
//	�O���t�B�b�N����
//
//---------------------------------------------------------------------------
void FASTCALL Render::Grp(int block, int raster)
{
	int i;
	int y;
	int offset;

	ASSERT((block >= 0) && (block <= 3));
	ASSERT((raster >= 0) && (raster < 1024));
	ASSERT(render.grpbuf[block]);
	ASSERT(render.grpgv);

	if (render.grptype == 0) {
		// 1024���[�h�̓y�[�W0���݂�
		if (!render.grpen[0]) {
			return;
		}
	}
	else {
		// ����ȊO
		if (!render.grpen[block]) {
			return;
		}
	}

	// �^�C�v��
	switch (render.grptype) {
		// �^�C�v0:1024�~1024 16Color
		case 0:
			// �I�t�Z�b�g�Z�o
			offset = (raster + render.grpy[0]) & 0x3ff;
			y = offset & 0x1ff;

			// �\���Ώۃ`�F�b�N
			if ((offset < 512) && (block >= 2)) {
				return;
			}
			if ((offset >= 512) && (block < 2)) {
				return;
			}

			// �p���b�g�̏ꍇ�͑S�̈揈��
			if (render.grppal[y + (block << 9)]) {
				render.grppal[y + (block << 9)] = FALSE;
				render.grpmod[y + (block << 9)] = FALSE;
				for (i=0; i<32; i++) {
					render.grpflag[(y << 5) + (block << 14) + i] = FALSE;
				}
				switch (block) {
					// �㔼���̓u���b�N0�ő�
					case 0:
						if (Rend1024A(render.grpgv + (y << 10),
									render.grpbuf[0] + (offset << 11),
									render.paldata) != 0) {
							render.mix[raster] = TRUE;
						}
					case 1:
						break;
					// �������̓u���b�N2�ő�
					case 2:
						if (Rend1024B(render.grpgv + (y << 10),
									render.grpbuf[0] + (offset << 11),
									render.paldata) != 0) {
							render.mix[raster] = TRUE;
						}
					case 3:
						break;
				}
				return;
			}

			// ����ȊO��grpmod�����ď���
			if (!render.grpmod[y + (block << 9)]) {
				return;
			}
			render.grpmod[y + (block << 9)] = FALSE;
			render.mix[raster] = TRUE;
			switch (block) {
				// �u���b�N0-����
				case 0:
					Rend1024C(render.grpgv + (y << 10),
								render.grpbuf[0] + (offset << 11),
								render.grpflag + (y << 5),
								render.paldata);
					break;
				// �u���b�N1-�E��
				case 1:
					Rend1024D(render.grpgv + (y << 10),
								render.grpbuf[0] + (offset << 11),
								render.grpflag + (y << 5) + 0x4000,
								render.paldata);
					break;
				// �u���b�N2-����
				case 2:
					Rend1024E(render.grpgv + (y << 10),
								render.grpbuf[0] + (offset << 11),
								render.grpflag + (y << 5) + 0x8000,
								render.paldata);
					break;
				// �u���b�N3-�E��
				case 3:
					Rend1024F(render.grpgv + (y << 10),
								render.grpbuf[0] + (offset << 11),
								render.grpflag + (y << 5) + 0xc000,
								render.paldata);
					break;
			}
			return;

		// �^�C�v1:512�~512 16Color
		case 1:
			switch (block) {
				// �y�[�W0
				case 0:
					y = (raster + render.grpy[0]) & 0x1ff;
					// �p���b�g
					if (render.grppal[y]) {
						render.grppal[y] = FALSE;
						render.grpmod[y] = FALSE;
						for (i=0; i<32; i++) {
							render.grpflag[(y << 5) + i] = FALSE;
						}
						if (Rend16A(render.grpgv + (y << 10),
										render.grpbuf[0] + (y << 10),
										render.paldata) != 0) {
							render.mix[raster] = TRUE;
						}
						return;
					}
					// �ʏ�
					if (render.grpmod[y]) {
						render.grpmod[y] = FALSE;
						render.mix[raster] = TRUE;
						Rend16B(render.grpgv + (y << 10),
								render.grpbuf[0] + (y << 10),
								render.grpflag + (y << 5),
								render.paldata);
					}
					return;
				// �y�[�W1
				case 1:
					y = (raster + render.grpy[1]) & 0x1ff;
					// �p���b�g
					if (render.grppal[y + 512]) {
						render.grppal[y + 512] = FALSE;
						render.grpmod[y + 512] = FALSE;
						for (i=0; i<32; i++) {
							render.grpflag[(y << 5) + i + 0x4000] = FALSE;
						}
						if (Rend16C(render.grpgv + (y << 10),
										render.grpbuf[1] + (y << 10),
										render.paldata) != 0) {
							render.mix[raster] = TRUE;
						}
						return;
					}
					// �ʏ�
					if (render.grpmod[y + 512]) {
						render.grpmod[y + 512] = FALSE;
						render.mix[raster] = TRUE;
						Rend16D(render.grpgv + (y << 10),
								render.grpbuf[1] + (y << 10),
								render.grpflag + (y << 5) + 0x4000,
								render.paldata);
					}
					return;
				// �y�[�W2
				case 2:
					y = (raster + render.grpy[2]) & 0x1ff;
					// �p���b�g
					if (render.grppal[y + 1024]) {
						render.grppal[y + 1024] = FALSE;
						render.grpmod[y + 1024] = FALSE;
						for (i=0; i<32; i++) {
							render.grpflag[(y << 5) + i + 0x8000] = FALSE;
						}
						if (Rend16E(render.grpgv + (y << 10),
										render.grpbuf[2] + (y << 10),
										render.paldata) != 0) {
							render.mix[raster] = TRUE;
						}
						return;
					}
					// �ʏ�
					if (render.grpmod[y + 1024]) {
						render.grpmod[y + 1024] = FALSE;
						render.mix[raster] = TRUE;
						Rend16F(render.grpgv + (y << 10),
								render.grpbuf[2] + (y << 10),
								render.grpflag + (y << 5) + 0x8000,
								render.paldata);
					}
					return;
				// �y�[�W3
				case 3:
					y = (raster + render.grpy[3]) & 0x1ff;
					// �p���b�g
					if (render.grppal[y + 1536]) {
						render.grppal[y + 1536] = FALSE;
						render.grpmod[y + 1536] = FALSE;
						for (i=0; i<32; i++) {
							render.grpflag[(y << 5) + i + 0xC000] = FALSE;
						}
						if (Rend16G(render.grpgv + (y << 10),
										render.grpbuf[3] + (y << 10),
										render.paldata) != 0) {
							render.mix[raster] = TRUE;
						}
						return;
					}
					// �ʏ�
					if (render.grpmod[y + 1536]) {
						render.grpmod[y + 1536] = FALSE;
						render.mix[raster] = TRUE;
						Rend16H(render.grpgv + (y << 10),
								render.grpbuf[3] + (y << 10),
								render.grpflag + (y << 5) + 0xC000,
								render.paldata);
					}
					return;
			}
			return;

		// �^�C�v2:512�~512 256Color
		case 2:
			ASSERT((block == 0) || (block == 2));
			if (block == 0) {
				// �I�t�Z�b�g�Z�o
				y = (raster + render.grpy[0]) & 0x1ff;

				// �p���b�g�̏ꍇ�͑S�̈揈��
				if (render.grppal[y]) {
					render.grppal[y] = FALSE;
					render.grpmod[y] = FALSE;
					for (i=0; i<32; i++) {
						render.grpflag[(y << 5) + i] = FALSE;
					}
					if (Rend256C(render.grpgv + (y << 10),
									render.grpbuf[0] + (y << 10),
									render.paldata) != 0) {
						render.mix[raster] = TRUE;
					}
					return;
				}

				// ����ȊO��grpmod�����ď���
				if (!render.grpmod[y]) {
					return;
				}

				render.grpmod[y] = FALSE;
				render.mix[raster] = TRUE;
				Rend256A(render.grpgv + (y << 10),
							render.grpbuf[0] + (y << 10),
							render.grpflag + (y << 5),
							render.paldata);
			}
			else {
				// �I�t�Z�b�g�Z�o
				y = (raster + render.grpy[2]) & 0x1ff;

				// �p���b�g�̏ꍇ�͑S�̈揈��
				if (render.grppal[0x400 + y]) {
					render.grppal[0x400 + y] = FALSE;
					render.grpmod[0x400 + y] = FALSE;
					for (i=0; i<32; i++) {
						render.grpflag[(y << 5) + i + 0x8000] = FALSE;
					}
					if (Rend256D(render.grpgv + (y << 10),
									render.grpbuf[2] + (y << 10),
									render.paldata) != 0) {
						render.mix[raster] = TRUE;
					}
					return;
				}

				// ����ȊO��grpmod�����ď���
				if (!render.grpmod[0x400 + y]) {
					return;
				}

				render.grpmod[0x400 + y] = FALSE;
				render.mix[raster] = TRUE;

				// �����_�����O
				Rend256B(render.grpgv + (y << 10),
							render.grpbuf[2] + (y << 10),
							render.grpflag + 0x8000 + (y << 5),
							render.paldata);
			}
			return;

		// �^�C�v3:512x512 ����`
		case 3:
		// �^�C�v4:512x512 65536Color
		case 4:
			ASSERT(block == 0);
			// �I�t�Z�b�g�Z�o
			y = (raster + render.grpy[0]) & 0x1ff;

			// �p���b�g�̏ꍇ�͑S�̈揈��
			if (render.grppal[y]) {
				render.grppal[y] = FALSE;
				render.grpmod[y] = FALSE;
				for (i=0; i<32; i++) {
					render.grpflag[(y << 5) + i] = FALSE;
				}
				if (Rend64KB(render.grpgv + (y << 10),
								render.grpbuf[0] + (y << 10),
								render.pal64k,
								render.palptr) != 0) {
					render.mix[raster] = TRUE;
				}
				return;
			}

			// ����ȊO��grpmod�����ď���
			if (!render.grpmod[y]) {
				return;
			}
			render.grpmod[y] = FALSE;
			render.mix[raster] = TRUE;
			Rend64KA(render.grpgv + (y << 10),
						render.grpbuf[0] + (y << 10),
						render.grpflag + (y << 5),
						render.pal64k,
						render.palptr);
			return;
	}
}

//===========================================================================
//
//	�����_��(BG�E�X�v���C�g��)
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	�X�v���C�g���W�X�^���Z�b�g
//
//---------------------------------------------------------------------------
void FASTCALL Render::SpriteReset()
{
	DWORD addr;
	WORD data;

	// �X�v���C�g���W�X�^�ݒ�
	for (addr=0; addr<0x400; addr+=2) {
		data = *(WORD*)(&render.sprmem[addr]);
		SpriteReg(addr, data);
	}
}

//---------------------------------------------------------------------------
//
//	�X�v���C�g���W�X�^�ύX
//
//---------------------------------------------------------------------------
void FASTCALL Render::SpriteReg(DWORD addr, DWORD data)
{
	BOOL use;
	DWORD reg[4];
	DWORD *next;
	DWORD **ptr;
	int index;
	int i;
	int j;
	int offset;
	DWORD pcgno;

	ASSERT(this);
	ASSERT(addr < 0x400);
	ASSERT((addr & 1) == 0);

	// �C���f�N�V���O�ƃf�[�^����
	index = (int)(addr >> 3);
	switch ((addr & 7) >> 1) {
		// X,Y(0�`1023)
		case 0:
		case 1:
			data &= 0x3ff;
			break;
		// V,H,PAL,PCG
		case 2:
			data &= 0xcfff;
			break;
		// PRW(0,1,2,3)
		case 3:
			data &= 0x0003;
			break;
	}

	// ptr�ݒ�(&spptr[index << 9])
	ptr = &render.spptr[index << 9];

	// ���W�X�^�̃o�b�N�A�b�v
	next = &render.spreg[index << 2];
	reg[0] = next[0];
	reg[1] = next[1];
	reg[2] = next[2];
	reg[3] = next[3];

	// ���W�X�^�֏�������
	render.spreg[addr >> 1] = data;

	// ����L���ɂȂ邩�`�F�b�N
	use = TRUE;
	if (next[0] == 0) {
		use = FALSE;
	}
	if (next[0] >= (DWORD)(render.mixlen + 16)) {
		use = FALSE;
	}
	if (next[1] == 0) {
		use = FALSE;
	}
	if (next[1] >= (512 + 16)) {
		use = FALSE;
	}
	if (next[3] == 0) {
		use = FALSE;
	}

	// ���܂܂Ŗ����ŁA���ꂩ��������Ȃ牽�����Ȃ�
	if (!render.spuse[index]) {
		if (!use) {
			return;
		}
	}

	// ���܂܂ŗL���Ȃ̂ŁA��x�Ƃ߂�
	if (render.spuse[index]) {
		// ��������(PCG)
		pcgno = reg[2] & 0xfff;
		ASSERT(render.pcguse[ pcgno ] > 0);
		render.pcguse[ pcgno ]--;
		pcgno >>= 8;
		ASSERT(render.pcgpal[ pcgno ] > 0);
		render.pcgpal[ pcgno ]--;

		// ��������(�|�C���^)
		for (i=0; i<16; i++) {
			j = (int)(reg[1] - 16 + i);
			if ((j >= 0) && (j < 512)) {
				ptr[j] = NULL;
				render.bgspmod[j] = TRUE;
			}
		}

		// ���㖳���Ȃ�A�����ŏI��
		if (!use) {
			render.spuse[index] = FALSE;
			return;
		}
	}

	// �o�^����(�g�p�t���O)
	render.spuse[index] = TRUE;

	// �o�^����(PCG)
	pcgno = next[2] & 0xfff;
	render.pcguse[ pcgno ]++;
	offset = pcgno << 8;
	pcgno >>= 8;
	render.pcgpal[ pcgno ]++;

	// PCG�A�h���X���v�Z�A�|�C���^�Z�b�g
	if (next[2] & 0x8000) {
		// V���]
		offset += 0xf0;
		for (i=0; i<16; i++) {
			j = (int)(next[1] - 16 + i);
			if ((j >= 0) && (j < 512)) {
				ptr[j] = &render.pcgbuf[offset];
				render.bgspmod[j] = TRUE;
			}
			offset -= 16;
		}
	}
	else {
		// �m�[�}��
		for (i=0; i<16; i++) {
			j = (int)(next[1] - 16 + i);
			if ((j >= 0) && (j < 512)) {
				ptr[j] = &render.pcgbuf[offset];
				render.bgspmod[j] = TRUE;
			}
			offset += 16;
		}
	}
}

//---------------------------------------------------------------------------
//
//	BG�X�N���[���ύX
//
//---------------------------------------------------------------------------
void FASTCALL Render::BGScrl(int page, DWORD x, DWORD y)
{
	BOOL flag;
	int i;

	ASSERT((page == 0) || (page == 1));
	ASSERT(x < 1024);
	ASSERT(y < 1024);

	// ��r�A��v���Ă�Ή������Ȃ�
	if ((render.bgx[page] == x) && (render.bgy[page] == y)) {
		return;
	}

	// �X�V
	render.bgx[page] = x;
	render.bgy[page] = y;

	// 768�~512�Ȃ疳�Ӗ�
	if (!render.bgspflag) {
		return;
	}

	// �\�����Ȃ�ABGSPMOD���グ��
	flag = FALSE;
	if (render.bgdisp[0]) {
		flag = TRUE;
	}
	if (render.bgdisp[1] && !render.bgsize) {
		flag = TRUE;
	}
	if (flag) {
		for (i=0; i<512; i++) {
			render.bgspmod[i] = TRUE;
		}
	}
}

//---------------------------------------------------------------------------
//
//	BG�R���g���[���ύX
//
//---------------------------------------------------------------------------
void FASTCALL Render::BGCtrl(int index, BOOL flag)
{
	int i;
	int j;
	BOOL areaflag[2];
	DWORD *reg;
	WORD *area;
	DWORD pcgno;
	DWORD low;
	DWORD mid;
	DWORD high;

	// �t���OOFF
	areaflag[0] = FALSE;
	areaflag[1] = FALSE;

	// �^�C�v��
	switch (index) {
		// BG0 �\���t���O
		case 0:
			if (render.bgdisp[0] == flag) {
				return;
			}
			render.bgdisp[0] = flag;
			break;

		// BG1 �\���t���O
		case 1:
			if (render.bgdisp[1] == flag) {
				return;
			}
			render.bgdisp[1] = flag;
			break;

		// BG0 �G���A�ύX
		case 2:
			if (render.bgarea[0] == flag) {
				return;
			}
			render.bgarea[0] = flag;
			areaflag[0] = TRUE;
			break;

		// BG1 �G���A�ύX
		case 3:
			if (render.bgarea[1] == flag) {
				return;
			}
			render.bgarea[1] = flag;
			areaflag[1] = TRUE;
			break;

		// BG�T�C�Y�ύX
		case 4:
			if (render.bgsize == flag) {
				return;
			}
			render.bgsize = flag;
			areaflag[0] = TRUE;
			areaflag[1] = TRUE;
			break;

		// ���̑�(���肦�Ȃ�)
		default:
			ASSERT(FALSE);
			return;
	}

	// �t���O����
	for (i=0; i<2; i++) {
		if (areaflag[i]) {
			// ����Ŏg���Ă���render.pcguse���J�b�g
			reg = render.bgreg[i];
			for (j=0; j<(64 * 64); j++) {
				pcgno = reg[j];
				if (pcgno & 0x10000) {
					pcgno &= 0xfff;
					ASSERT(render.pcguse[ pcgno ] > 0);
					render.pcguse[ pcgno ]--;
					pcgno = (pcgno >> 8) & 0x0f;
					ASSERT(render.pcgpal[ pcgno ] > 0);
					render.pcgpal[ pcgno ]--;
				}
			}

			// �f�[�^�A�h���X���Z�o($EBE000,$EBC000)
			area = (WORD*)render.sprmem;
			area += 0x6000;
			if (render.bgarea[i]) {
				area += 0x1000;
			}

			// 64�~64���[�h�R�s�[�B$10000�̃r�b�g�͏��0
			if (render.bgsize) {
				// 16x16�͂��̂܂�
				for (j=0; j<(64*64); j++) {
					render.bgreg[i][j] = (DWORD)area[j];
				}
			}
			else {
				// 8x8�͍H�v���K�v�BPCG(0-255)��>>2���A������bit0,1��bit17,18��
				for (j=0; j<(64*64); j++) {
					low = (DWORD)area[j];
					mid = low;
					high = low;
					low >>= 2;
					low &= (64 - 1);
					mid &= 0xff00;
					high <<= 17;
					high &= 0x60000;
					render.bgreg[i][j] = (DWORD)(low | mid | high);
				}
			}

			// bgall�̃Z�b�g
			for (j=0; j<64; j++) {
				render.bgall[i][j] = TRUE;
			}
		}
	}

	// �ǂ̕ύX�ł��A768�~512�ȊO�Ȃ�bgspmod���グ��
	if (render.bgspflag) {
		for (i=0; i<512; i++) {
			render.bgspmod[i] = TRUE;
		}
	}
}

//---------------------------------------------------------------------------
//
//	BG�������ύX
//
//---------------------------------------------------------------------------
void FASTCALL Render::BGMem(DWORD addr, WORD data)
{
	BOOL flag;
	int i;
	int j;
	int index;
	int raster;
	DWORD pcgno;
	DWORD low;
	DWORD mid;
	DWORD high;

	ASSERT((addr >= 0xc000) && (addr < 0x10000));

	// �y�[�W���[�v
	for (i=0; i<2; i++) {
		// �Y���y�[�W�̃f�[�^�G���A�ƈ�v���Ă��邩
		flag = FALSE;
		if ((render.bgarea[i] == FALSE) && (addr < 0xe000)) {
			flag = TRUE;
		}
		if ((render.bgarea[i] == TRUE) && (addr >= 0xe000)) {
			flag = TRUE;
		}
		if (!flag) {
			continue;
		}

		// �C���f�b�N�X(<64x64)�A���W�X�^�|�C���^�擾
		index = (int)(addr & 0x1fff);
		index >>= 1;
		ASSERT((index >= 0) && (index < 64*64));
		pcgno = render.bgreg[i][index];

		// �ȑO��pcguse������
		if (pcgno & 0x10000) {
			pcgno &= 0xfff;
			ASSERT(render.pcguse[ pcgno ] > 0);
			render.pcguse[ pcgno ]--;
			pcgno = (pcgno >> 8) & 0x0f;
			ASSERT(render.pcgpal[ pcgno ] > 0);
			render.pcgpal[ pcgno ]--;
		}

		// �R�s�[
		if (render.bgsize) {
			// 16x16�͂��̂܂�
			render.bgreg[i][index] = (DWORD)data;
		}
		else {
			// 8x8�͍H�v���K�v�BPCG(0-255)��>>2���A������bit0,1��bit17,18��
			low = (DWORD)data;
			mid = low;
			high = low;
			low >>= 2;
			low &= (64 - 1);
			mid &= 0xff00;
			high <<= 17;
			high &= 0x60000;
			render.bgreg[i][index] = (DWORD)(low | mid | high);
		}

		// bgall���グ��
		render.bgall[i][index >> 6] = TRUE;

		// �\�����łȂ���ΏI���Bbgsize=1�Ńy�[�W1�̏ꍇ���I��
		if (!render.bgspflag || !render.bgdisp[i]) {
			continue;
		}
		if (render.bgsize && (i == 1)) {
			continue;
		}

		// �X�N���[���ʒu����v�Z���Abgspmod���グ��
		index >>= 6;
		if (render.bgsize) {
			// 16x16
			raster = render.bgy[i] + (index << 4);
			for (j=0; j<16; j++) {
				raster &= (1024 - 1);
				if ((raster >= 0) && (raster < 512)) {
					render.bgspmod[raster] = TRUE;
				}
				raster++;
			}
		}
		else {
			// 8x8
			raster = render.bgy[i] + (index << 3);
			for (j=0; j<16; j++) {
				raster &= (512 - 1);
				render.bgspmod[raster] = TRUE;
				raster++;
			}
		}
	}
}

//---------------------------------------------------------------------------
//
//	PCG�������ύX
//
//---------------------------------------------------------------------------
void FASTCALL Render::PCGMem(DWORD addr)
{
	int index;
	DWORD count;
	int i;

	ASSERT(this);
	ASSERT(addr >= 0x8000);
	ASSERT(addr < 0x10000);
	ASSERT((addr & 1) == 0);

	// �C���f�b�N�X���o��
	addr &= 0x7fff;
	index = (int)(addr >> 7);
	ASSERT((index >= 0) && (index < 256));

	// render.pcgready������
	for (i=0; i<16; i++) {
		render.pcgready[index + (i << 8)] = FALSE;
	}

	// render.pcguse��>0�Ȃ�
	count = 0;
	for (i=0; i<16; i++) {
		count += render.pcguse[index + (i << 8)];
	}
	if (count > 0) {
		// �d���Ȃ��̂ŁABG/�X�v���C�g�č���������
		for (i=0; i<512; i++) {
			render.bgspmod[i] = TRUE;
		}
	}
}

//---------------------------------------------------------------------------
//
//	PCG�o�b�t�@�擾
//
//---------------------------------------------------------------------------
const DWORD* FASTCALL Render::GetPCGBuf() const
{
	ASSERT(this);
	ASSERT(render.pcgbuf);

	return render.pcgbuf;
}

//---------------------------------------------------------------------------
//
//	BG/�X�v���C�g�o�b�t�@�擾
//
//---------------------------------------------------------------------------
const DWORD* FASTCALL Render::GetBGSpBuf() const
{
	ASSERT(this);
	ASSERT(render.bgspbuf);

	return render.bgspbuf;
}

//---------------------------------------------------------------------------
//
//	BG/�X�v���C�g
//
//---------------------------------------------------------------------------
void FASTCALL Render::BGSprite(int raster)
{
	int i;
	DWORD *reg;
	DWORD **ptr;
	DWORD *buf;
	DWORD pcgno;

	// BG�X�v���C�g��Mix���Ȃ��t���O�̃`�F�b�N���K�v���B����ASSERT�B

	// BG,�X�v���C�g�Ƃ�512�܂ł����l���Ă��Ȃ�
	if (raster >= 512) return;
//	ASSERT((raster >= 0) && (raster < 512));

	// ������512�܂ŁB�������O��
	if (render.mixlen > 512) return;
//	ASSERT(render.mixlen <= 512);

	// �t���O�`�F�b�N�A�I�t�A�����w��
	if (!render.bgspmod[raster]) {
		return;
	}
	render.bgspmod[raster] = FALSE;
	render.mix[raster] = TRUE;

	// �o�b�t�@�N���A
	// �����Ńp���b�g$100�Ŗ��߂�(�o���c�CLoading)
	buf = &render.bgspbuf[raster << 9];
	RendClrSprite(buf, render.paldata[0x100], render.mixlen);
	if (!sprite->IsDisplay()) {
		// ��\���Ȃ�$80000000�̃r�b�g�͗��Ƃ�(�o���c�CF3)
		RendClrSprite(buf, render.paldata[0x100] & 0x00ffffff, render.mixlen);
		return;
	}

	// ��Ԍ��ɂ���(PRW=1)�X�v���C�g
	reg = &render.spreg[127 << 2];
	ptr = &render.spptr[127 << 9];
	ptr += raster;
	for (i=127; i>=0; i--) {
		if (render.spuse[i]) {
			// �g�p��
			if (reg[3] == 1) {
				// PRW=1
				if (*ptr) {
					// �\��
					pcgno = reg[2] & 0xfff;
					if (!render.pcgready[pcgno]) {
						ASSERT(render.pcguse[pcgno] > 0);
						render.pcgready[pcgno] = TRUE;
						RendPCGNew(pcgno, render.sprmem, render.pcgbuf, render.paldata);
					}
					if (cmov) {
						RendSpriteC(*ptr, buf, reg[0], reg[2] & 0x4000);
					}
					else {
						RendSprite(*ptr, buf, reg[0], reg[2] & 0x4000);
					}
				}
			}
		}
		// ���̃X�v���C�g(SP0�������Ƃ���O)
		reg -= 4;
		ptr -= 512;
	}

	// BG1��\��
	if (render.bgdisp[1] && !render.bgsize) {
		BG(1, raster, buf);
	}

	// ���Ԃɂ���(PRW=2)�X�v���C�g
	reg = &render.spreg[127 << 2];
	ptr = &render.spptr[127 << 9];
	ptr += raster;
	for (i=127; i>=0; i--) {
		if (render.spuse[i]) {
			// �g�p��
			if (reg[3] == 2) {
				// PRW=2
				if (*ptr) {
					// �\��
					pcgno = reg[2] & 0xfff;
					if (!render.pcgready[pcgno]) {
						ASSERT(render.pcguse[pcgno] > 0);
						render.pcgready[pcgno] = TRUE;
						RendPCGNew(pcgno, render.sprmem, render.pcgbuf, render.paldata);
					}
					if (cmov) {
						RendSpriteC(*ptr, buf, reg[0], reg[2] & 0x4000);
					}
					else {
						RendSprite(*ptr, buf, reg[0], reg[2] & 0x4000);
					}
				}
			}
		}
		// ���̃X�v���C�g(SP0�������Ƃ���O)
		reg -= 4;
		ptr -= 512;
	}

	// BG0��\��
	if (render.bgdisp[0]) {
		BG(0, raster, buf);
	}

	// ��O�ɂ���(PRW=3)�X�v���C�g
	reg = &render.spreg[127 << 2];
	ptr = &render.spptr[127 << 9];
	ptr += raster;
	for (i=127; i>=0; i--) {
		if (render.spuse[i]) {
			// �g�p��
			if (reg[3] == 3) {
				// PRW=3
				if (*ptr) {
					// �\��
					pcgno = reg[2] & 0xfff;
					if (!render.pcgready[pcgno]) {
						ASSERT(render.pcguse[pcgno] > 0);
						render.pcgready[pcgno] = TRUE;
						RendPCGNew(pcgno, render.sprmem, render.pcgbuf, render.paldata);
					}
					if (cmov) {
						RendSpriteC(*ptr, buf, reg[0], reg[2] & 0x4000);
					}
					else {
						RendSprite(*ptr, buf, reg[0], reg[2] & 0x4000);
					}
				}
			}
		}
		// ���̃X�v���C�g(SP0�������Ƃ���O)
		reg -= 4;
		ptr -= 512;
	}
}

//---------------------------------------------------------------------------
//
//	BG
//
//---------------------------------------------------------------------------
void FASTCALL Render::BG(int page, int raster, DWORD *buf)
{
	int x;
	int y;
	DWORD **ptr;
	int len;
	int rest;

	ASSERT((page == 0) || (page == 1));
	ASSERT((raster >= 0) && (raster < 512));
	ASSERT(buf);

	// y�u���b�N������o��
	y = render.bgy[page] + raster;
	if (render.bgsize) {
		// 16x16���[�h
		y &= (1024 - 1);
		y >>= 4;
	}
	else {
		// 8x8���[�h
		y &= (512 - 1);
		y >>= 3;
	}
	ASSERT((y >= 0) && (y < 64));

	// bgall��TRUE�Ȃ�A����y�u���b�N�ŕύX�f�[�^����
	if (render.bgall[page][y]) {
		render.bgall[page][y] = FALSE;
		BGBlock(page, y);
	}

	// �\��
	ptr = render.bgptr[page];
	if (!render.bgsize) {
		// 8x8�̕\��
		x = render.bgx[page] & (512 - 1);
		ptr += (((render.bgy[page] + raster) & (512 - 1)) << 7);

		// ����؂�邩�`�F�b�N
		if ((x & 7) == 0) {
			// 8x8�A����؂��
			x >>= 3;
			if (cmov) {
				RendBG8C(ptr, buf, x, render.mixlen, render.pcgready,
					render.sprmem, render.pcgbuf, render.paldata);
			}
			else {
				RendBG8(ptr, buf, x, render.mixlen, render.pcgready,
					render.sprmem, render.pcgbuf, render.paldata);
			}
			return;
		}

		// �ŏ��̔��[�u���b�N�����s
		rest = 8 - (x & 7);
		ASSERT((rest > 0) && (rest < 8));
		RendBG8P(&ptr[(x & 0xfff8) >> 2], buf, (x & 7), rest, render.pcgready,
				render.sprmem, render.pcgbuf, render.paldata);

		// �]��𒲂ׂ�8dot�P�ʕ�������
		len = render.mixlen - rest;
		x += rest;
		x &= (512 - 1);
		ASSERT((x & 7) == 0);
		if (cmov) {
			RendBG8C(ptr, &buf[rest], (x >> 3), (len & 0xfff8), render.pcgready,
				render.sprmem, render.pcgbuf, render.paldata);
		}
		else {
			RendBG8(ptr, &buf[rest], (x >> 3), (len & 0xfff8), render.pcgready,
				render.sprmem, render.pcgbuf, render.paldata);
		}

		// �Ō�
		if (len & 7) {
			x += (len & 0xfff8);
			x &= (512 - 1);
			RendBG8P(&ptr[x >> 2], &buf[rest + (len & 0xfff8)], 0, (len & 7),
				render.pcgready, render.sprmem, render.pcgbuf, render.paldata);
		}
		return;
	}

	// 16x16�̕\��
	x = render.bgx[page] & (1024 - 1);
	ptr += (((render.bgy[page] + raster) & (1024 - 1)) << 7);

	// ����؂�邩�`�F�b�N
	if ((x & 15) == 0) {
		// 16x16�A����؂��
		x >>= 4;
		if (cmov) {
			RendBG16C(ptr, buf, x, render.mixlen, render.pcgready,
				render.sprmem, render.pcgbuf, render.paldata);
		}
		else {
			RendBG16(ptr, buf, x, render.mixlen, render.pcgready,
				render.sprmem, render.pcgbuf, render.paldata);
		}
		return;
	}

	// �ŏ��̔��[�u���b�N�����s
	rest = 16 - (x & 15);
	ASSERT((rest > 0) && (rest < 16));
	RendBG16P(&ptr[(x & 0xfff0) >> 3], buf, (x & 15), rest, render.pcgready,
			render.sprmem, render.pcgbuf, render.paldata);

	// �]��𒲂ׂ�16dot�P�ʕ�������
	len = render.mixlen - rest;
	x += rest;
	x &= (1024 - 1);
	ASSERT((x & 15) == 0);
	if (cmov) {
		RendBG16C(ptr, &buf[rest], (x >> 4), (len & 0xfff0), render.pcgready,
			render.sprmem, render.pcgbuf, render.paldata);
	}
	else {
		RendBG16(ptr, &buf[rest], (x >> 4), (len & 0xfff0), render.pcgready,
			render.sprmem, render.pcgbuf, render.paldata);
	}

	// �Ō�
	if (len & 15) {
		x += (len & 0xfff0);
		x &= (1024 - 1);
		x >>= 4;
		RendBG16P(&ptr[x << 1], &buf[rest + (len & 0xfff0)], 0, (len & 15),
			render.pcgready, render.sprmem, render.pcgbuf, render.paldata);
	}
}

//---------------------------------------------------------------------------
//
//	BG(�u���b�N����)
//
//---------------------------------------------------------------------------
void FASTCALL Render::BGBlock(int page, int y)
{
	int i;
	int j;
	DWORD *reg;
	DWORD **ptr;
	DWORD *pcgbuf;
	DWORD bgdata;
	DWORD pcgno;

	ASSERT((page == 0) || (page == 1));
	ASSERT((y >= 0) && (y < 64));

	// ���W�X�^�|�C���^�𓾂�
	reg = &render.bgreg[page][y << 6];

	// BG�|�C���^�𓾂�
	ptr = render.bgptr[page];
	if (render.bgsize) {
		ptr += (y << 11);
	}
	else {
		ptr += (y << 10);
	}

	// ���[�v
	for (i=0; i<64; i++) {
		// �擾
		bgdata = reg[i];

		// $10000�������Ă����OK
		if (bgdata & 0x10000) {
			ptr += 2;
			continue;
		}

		// $10000��OR
		reg[i] |= 0x10000;

		// pcgno�𓾂�
		pcgno = bgdata & 0xfff;

		// �T�C�Y��
		if (render.bgsize) {
			// 16x16
			pcgbuf = &render.pcgbuf[ (pcgno << 8) ];
			if (bgdata & 0x8000) {
				// �㉺���]
				pcgbuf += 0xf0;
				for (j=0; j<16; j++) {
					ptr[0] = pcgbuf;
					ptr[1] = (DWORD*)bgdata;
					pcgbuf -= 0x10;
					ptr += 128;
				}
			}
			else {
				// �ʏ�
				for (j=0; j<16; j++) {
					ptr[0] = pcgbuf;
					ptr[1] = (DWORD*)bgdata;
					pcgbuf += 0x10;
					ptr += 128;
				}
			}
			ptr -= 2048;
		}
		else {
			// 8x8�Bbit17,bit18���l������
			pcgbuf = &render.pcgbuf[ (pcgno << 8) ];
			if (bgdata & 0x20000) {
				pcgbuf += 0x80;
			}
			if (bgdata & 0x40000) {
				pcgbuf += 8;
			}

			if (bgdata & 0x8000) {
				// �㉺���]
				pcgbuf += 0x70;
				for (j=0; j<8; j++) {
					ptr[0] = pcgbuf;
					ptr[1] = (DWORD*)bgdata;
					pcgbuf -= 0x10;
					ptr += 128;
				}
			}
			else {
				// �ʏ�
				for (j=0; j<8; j++) {
					ptr[0] = pcgbuf;
					ptr[1] = (DWORD*)bgdata;
					pcgbuf += 0x10;
					ptr += 128;
				}
			}
			ptr -= 1024;
		}

		// �o�^����(PCG)
		render.pcguse[ pcgno ]++;
		pcgno = (pcgno >> 8) & 0x0f;
		render.pcgpal[ pcgno ]++;

		// �|�C���^��i�߂�
		ptr += 2;
	}
}

//===========================================================================
//
//	�����_��(������)
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	����
//
//---------------------------------------------------------------------------
void FASTCALL Render::Mix(int y)
{
	DWORD *p;
	DWORD *q;
	DWORD *r;
	DWORD *ptr[3];
	int offset;
	DWORD buf[1024];

	// �����w���������ꍇ�A�����o�b�t�@�������ꍇ�Ay�I�[�o�[�̏ꍇreturn
	if ((!render.mix[y]) || (!render.mixbuf)) {
		return;
	}
	if (render.mixheight <= y) {
		return;
	}
	ASSERT(render.mixlen > 0);

#if defined(REND_LOG)
	LOG1(Log::Normal, "���� y=%d", y);
#endif	// REND_LOG

	// �t���OOFF�A�����o�b�t�@�A�h���X������
	render.mix[y] = FALSE;
	q = &render.mixbuf[render.mixwidth * y];

	switch (render.mixtype) {
		// �^�C�v0(�\�����Ȃ�)
		case 0:
			RendMix00(q, render.drawflag + (y << 6), render.mixlen);
			return;

		// �^�C�v1(1�ʂ̂�)
		case 1:
			offset = (*render.mixy[0] + y) & render.mixand[0];
			p = render.mixptr[0];
			ASSERT(p);
			p += (offset << render.mixshift[0]);
			p += (*render.mixx[0] & render.mixand[0]);
			RendMix01(q, p, render.drawflag + (y << 6), render.mixlen);
			return;

		// �^�C�v2(2�ʁA�J���[0�d�ˍ��킹)
		case 2:
			offset = (*render.mixy[0] + y) & render.mixand[0];
			p = render.mixptr[0];
			ASSERT(p);
			p += (offset << render.mixshift[0]);
			p += (*render.mixx[0] & render.mixand[0]);
			offset = (*render.mixy[1] + y) & render.mixand[1];
			r = render.mixptr[1];
			ASSERT(r);
			r += (offset << render.mixshift[1]);
			r += (*render.mixx[1] & render.mixand[1]);
			if (cmov) {
				RendMix02C(q, p, r, render.drawflag + (y << 6), render.mixlen);
			}
			else {
				RendMix02(q, p, r, render.drawflag + (y << 6), render.mixlen);
			}
			return;

		// �^�C�v3(2�ʁA�ʏ�d�ˍ��킹)
		case 3:
			offset = (*render.mixy[0] + y) & render.mixand[0];
			p = render.mixptr[0];
			ASSERT(p);
			p += (offset << render.mixshift[0]);
			p += (*render.mixx[0] & render.mixand[0]);
			offset = (*render.mixy[1] + y) & render.mixand[1];
			r = render.mixptr[1];
			ASSERT(r);
			r += (offset << render.mixshift[1]);
			r += (*render.mixx[1] & render.mixand[1]);
			if (cmov) {
				RendMix03C(q, p, r, render.drawflag + (y << 6), render.mixlen);
			}
			else {
				RendMix03(q, p, r, render.drawflag + (y << 6), render.mixlen);
			}
			return;

		// �^�C�v4(�O���t�B�b�N�̂�3�� or 4��)
		case 4:
			MixGrp(y, buf);
			RendMix01(q, buf, render.drawflag + (y << 6), render.mixlen);
			break;

		// �^�C�v5(�O���t�B�b�N�{�e�L�X�g�A�e�L�X�g�D��ʏ�d�ˍ��킹)
		case 5:
			MixGrp(y, buf);
			offset = (*render.mixy[0] + y) & render.mixand[0];
			p = render.mixptr[0];
			ASSERT(p);
			p += (offset << render.mixshift[0]);
			p += (*render.mixx[0] & render.mixand[0]);
			if (cmov) {
				RendMix03C(q, p, buf, render.drawflag + (y << 6), render.mixlen);
			}
			else {
				RendMix03(q, p, buf, render.drawflag + (y << 6), render.mixlen);
			}
			return;

		// �^�C�v6(�O���t�B�b�N�{�e�L�X�g�A�O���t�B�b�N�D��ʏ�d�ˍ��킹)
		case 6:
			MixGrp(y, buf);
			offset = (*render.mixy[0] + y) & render.mixand[0];
			p = render.mixptr[0];
			p += (offset << render.mixshift[0]);
			p += (*render.mixx[0] & render.mixand[0]);
			if (cmov) {
				RendMix03C(q, buf, p, render.drawflag + (y << 6), render.mixlen);
			}
			else {
				RendMix03(q, buf, p, render.drawflag + (y << 6), render.mixlen);
			}
			return;

		// �^�C�v7(�e�L�X�g�{�X�v���C�g�{�O���t�B�b�N1��)
		case 7:
			offset = (*render.mixy[0] + y) & render.mixand[0];
			ptr[0] = render.mixptr[0];
			ptr[0] += (offset << render.mixshift[0]);
			ptr[0] += (*render.mixx[0] & render.mixand[0]);
			offset = (*render.mixy[1] + y) & render.mixand[1];
			ptr[1] = render.mixptr[1];
			ptr[1] += (offset << render.mixshift[1]);
			ptr[1] += (*render.mixx[1] & render.mixand[1]);
			offset = (*render.mixy[2] + y) & render.mixand[2];
			ptr[2] = render.mixptr[2];
			ptr[2] += (offset << render.mixshift[2]);
			ptr[2] += (*render.mixx[2] & render.mixand[2]);
			if (cmov) {
				RendMix04C(q, ptr[render.mixmap[0]], ptr[render.mixmap[1]],
					ptr[render.mixmap[2]], render.drawflag + (y << 6), render.mixlen);
			}
			else {
				RendMix04(q, ptr[render.mixmap[0]], ptr[render.mixmap[1]],
					ptr[render.mixmap[2]], render.drawflag + (y << 6), render.mixlen);
			}
			return;

		// �^�C�v8(�e�L�X�g+�X�v���C�g+�O���t�B�b�N�Q�ʈȏ�)
		case 8:
			MixGrp(y, buf);
			offset = (*render.mixy[0] + y) & render.mixand[0];
			ptr[0] = render.mixptr[0];
			ptr[0] += (offset << render.mixshift[0]);
			ptr[0] += (*render.mixx[0] & render.mixand[0]);
			offset = (*render.mixy[1] + y) & render.mixand[1];
			ptr[1] = render.mixptr[1];
			ptr[1] += (offset << render.mixshift[1]);
			ptr[1] += (*render.mixx[1] & render.mixand[1]);
			ptr[2] = buf;
			if (cmov) {
				RendMix04C(q, ptr[render.mixmap[0]], ptr[render.mixmap[1]],
					ptr[render.mixmap[2]], render.drawflag + (y << 6), render.mixlen);
			}
			else {
				RendMix04(q, ptr[render.mixmap[0]], ptr[render.mixmap[1]],                                    	
					ptr[render.mixmap[2]], render.drawflag + (y << 6), render.mixlen);
			}
			return;

		// ���̑�
		default:
			ASSERT(FALSE);
			break;
	}
}

//---------------------------------------------------------------------------
//
//	�O���t�B�b�N����
//
//---------------------------------------------------------------------------
void FASTCALL Render::MixGrp(int y, DWORD *buf)
{
	DWORD *p;
	DWORD *q;
	DWORD *r;
	DWORD *s;
	int offset;

	ASSERT(buf);
	ASSERT((y >= 0) && (y < render.mixheight));

	switch (render.mixpage) {
		// �Ȃ�(����render.mixpage�ŋz��)
		case 0:
			ASSERT(FALSE);
			return;

		// 1��(����render.mixpage�ŋz��)
		case 1:
			ASSERT(FALSE);
			return;

		// 2��
		case 2:
			offset = (*render.mixy[4] + y) & render.mixand[4];
			p = render.mixptr[4];
			ASSERT(p);
			p += (offset << render.mixshift[4]);
			p += (*render.mixx[4] & render.mixand[4]);
			offset = (*render.mixy[5] + y) & render.mixand[5];
			q = render.mixptr[5];
			ASSERT(q);
			q += (offset << render.mixshift[5]);
			q += (*render.mixx[5] & render.mixand[5]);

			if (cmov) {
				RendGrp02C(buf, p, q, render.mixlen);
			}
			else {
				RendGrp02(buf, p, q, render.mixlen);
			}
			break;

		// 3��
		case 3:
			offset = (*render.mixy[4] + y) & render.mixand[4];
			p = render.mixptr[4];
			ASSERT(p);
			p += (offset << render.mixshift[4]);
			p += (*render.mixx[4] & render.mixand[4]);

			offset = (*render.mixy[5] + y) & render.mixand[5];
			q = render.mixptr[5];
			ASSERT(q);
			q += (offset << render.mixshift[5]);
			q += (*render.mixx[5] & render.mixand[5]);

			offset = (*render.mixy[6] + y) & render.mixand[6];
			r = render.mixptr[6];
			ASSERT(r);
			r += (offset << render.mixshift[6]);
			r += (*render.mixx[6] & render.mixand[6]);

			if (cmov) {
				RendGrp03C(buf, p, q, r, render.mixlen);
			}
			else {
				RendGrp03(buf, p, q, r, render.mixlen);
			}
			break;

		// 4��
		case 4:
			offset = (*render.mixy[4] + y) & render.mixand[4];
			p = render.mixptr[4];
			ASSERT(p);
			p += (offset << render.mixshift[4]);
			p += (*render.mixx[4] & render.mixand[4]);

			offset = (*render.mixy[5] + y) & render.mixand[5];
			q = render.mixptr[5];
			ASSERT(q);
			q += (offset << render.mixshift[5]);
			q += (*render.mixx[5] & render.mixand[5]);

			offset = (*render.mixy[6] + y) & render.mixand[6];
			r = render.mixptr[6];
			ASSERT(r);
			r += (offset << render.mixshift[6]);
			r += (*render.mixx[6] & render.mixand[6]);

			offset = (*render.mixy[7] + y) & render.mixand[7];
			s = render.mixptr[7];
			ASSERT(s);
			s += (offset << render.mixshift[7]);
			s += (*render.mixx[7] & render.mixand[7]);

			RendGrp04(buf, p, q, r, s, render.mixlen);
			return;

		// ���̑�
		default:
			ASSERT(FALSE);
			break;
	}
}

//---------------------------------------------------------------------------
//
//	�����o�b�t�@�擾
//
//---------------------------------------------------------------------------
const DWORD* FASTCALL Render::GetMixBuf() const
{
	ASSERT(this);

	// NULL�̏ꍇ������
	return render.mixbuf;
}

//---------------------------------------------------------------------------
//
//	�����_�����O
//
//---------------------------------------------------------------------------
void FASTCALL Render::Process()
{
	int i;

	// �s�������Ă���ꍇ�͕s�v
	if (render.first >= render.last) {
		return;
	}

	// VC
	if (render.vc) {
#if defined(REND_LOG)
		LOG0(Log::Normal, "�r�f�I����");
#endif	// RENDER_LOG
		Video();
	}

	// �R���g���X�g
	if (render.contrast) {
#if defined(REND_LOG)
		LOG0(Log::Normal, "�R���g���X�g����");
#endif	// RENDER_LOG
		Contrast();
	}

	// �p���b�g
	if (render.palette) {
#if defined(REND_LOG)
		LOG0(Log::Normal, "�p���b�g����");
#endif	// RENDER_LOG
		Palette();
	}

	// first==0�́A�X�v���C�g�̕\��ON/OFF������
	if (render.first == 0) {
		if (sprite->IsDisplay()) {
			if (!render.bgspdisp) {
				// �X�v���C�gCPU��Video
				for (i=0; i<512; i++) {
					render.bgspmod[i] = TRUE;
				}
				render.bgspdisp = TRUE;
			}
		}
		else {
			if (render.bgspdisp) {
				// �X�v���C�gVideo��CPU
				for (i=0; i<512; i++) {
					render.bgspmod[i] = TRUE;
				}
				render.bgspdisp = FALSE;
			}
		}
	}

	// ����x2�̏ꍇ
	if ((render.v_mul == 2) && !render.lowres) {
		// I/O���Ŋg�傷�邽�߁A�c�����͔����������Ȃ�
		for (i=render.first; i<render.last; i++) {
			if ((i & 1) == 0) {
				Text(i >> 1);
				Grp(0, i >> 1);
				Grp(1, i >> 1);
				Grp(2, i >> 1);
				Grp(3, i >> 1);
				BGSprite(i >> 1);
				Mix(i >> 1);
			}
		}
		// �X�V
		render.first = render.last;
		return;
	}

	// �C���^���[�X�̏ꍇ
	if ((render.v_mul == 0) && render.lowres) {
		// �����E��𓯎��ɍ��(���@�Ƃ͈قȂ�)
		for (i=render.first; i<render.last; i++) {
			// �e�L�X�g�E�O���t�B�b�N
			Text((i << 1) + 0);
			Text((i << 1) + 1);
			Grp(0, (i << 1) + 0);
			Grp(0, (i << 1) + 1);
			Grp(1, (i << 1) + 0);
			Grp(1, (i << 1) + 1);
			Grp(2, (i << 1) + 0);
			Grp(2, (i << 1) + 1);
			Grp(3, (i << 1) + 0);
			Grp(3, (i << 1) + 1);
			BGSprite((i << 1) + 0);
			BGSprite((i << 1) + 1);
			Mix((i << 1) + 0);
			Mix((i << 1) + 1);
		}
		// �X�V
		render.first = render.last;
		return;
	}

	// �ʏ탋�[�v
	for (i=render.first; i<render.last; i++) {
		Text(i);
		Grp(0, i);
		Grp(1, i);
		Grp(2, i);
		Grp(3, i);
		BGSprite(i);
		Mix(i);
	}
	// �X�V
	render.first = render.last;
}

