//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001,2002 �o�h�D(ytanaka@ipc-tokai.or.jp)
//	[ �����_�� ]
//
//---------------------------------------------------------------------------

#if !defined(render_h)
#define render_h

#include "device.h"
#include "vc.h"

//===========================================================================
//
//	�����_��
//
//===========================================================================
class Render : public Device
{
public:
	// �����f�[�^��`
	typedef struct {
		// �S�̐���
		BOOL act;						// �������Ă��邩
		BOOL enable;					// ��������
		int count;						// �X�P�W���[���A�g�J�E���^
		BOOL ready;						// �`�揀���ł��Ă��邩
		int first;						// ���������X�^
		int last;						// �\���I�����X�^

		// CRTC
		BOOL crtc;						// CRTC�ύX�t���O
		int width;						// X�����h�b�g��(256�`)
		int h_mul;						// X�����{��(1,2)
		int height;						// Y�����h�b�g��(256�`)
		int v_mul;						// Y�����{��(0,1,2)
		BOOL lowres;					// 15kHz�t���O

		// VC
		BOOL vc;						// VC�ύX�t���O

		// ����
		BOOL mix[1024];					// �����t���O(���C��)
		DWORD *mixbuf;					// �����o�b�t�@
		DWORD *mixptr[8];				// �����|�C���^
		DWORD mixshift[8];				// �����|�C���^��Y�V�t�g
		DWORD *mixx[8];					// �����|�C���^��X�X�N���[���|�C���^
		DWORD *mixy[8];					// �����|�C���^��Y�X�N���[���|�C���^
		DWORD mixand[8];				// �����|�C���^�̃X�N���[��AND�l
		int mixmap[3];					// �����}�b�v
		int mixtype;					// �����^�C�v
		int mixpage;					// �����O���t�B�b�N�y�[�W��
		int mixwidth;					// �����o�b�t�@��
		int mixheight;					// �����o�b�t�@����
		int mixlen;						// ��������������(x����)

		// �`��
		BOOL draw[1024];				// �`��t���O(���C��)
		BOOL *drawflag;					// �`��t���O(16dot)

		// �R���g���X�g
		BOOL contrast;					// �R���g���X�g�ύX�t���O
		int contlevel;					// �R���g���X�g

		// �p���b�g
		BOOL palette;					// �p���b�g�ύX�t���O
		BOOL palmod[0x200];				// �p���b�g�ύX�t���O
		DWORD *palbuf;					// �p���b�g�o�b�t�@
		DWORD *palptr;					// �p���b�g�|�C���^
		const WORD *palvc;				// �p���b�gVC�|�C���^
		DWORD paldata[0x200];			// �p���b�g�f�[�^
		BYTE pal64k[0x200];				// �p���b�g�f�[�^�ό`

		// �e�L�X�gVRAM
		BOOL texten;					// �e�L�X�g�\���t���O
		BOOL textpal[1024];				// �e�L�X�g�p���b�g�t���O
		BOOL textmod[1024];				// �e�L�X�g�X�V�t���O(���C��)
		BOOL *textflag;					// �e�L�X�g�X�V�t���O(32dot)
		BYTE *textbuf;					// �e�L�X�g�o�b�t�@(�p���b�g�O)
		DWORD *textout;					// �e�L�X�g�o�b�t�@(�p���b�g��)
		const BYTE *texttv;				// �e�L�X�gTVRAM�|�C���^
		DWORD textx;					// �e�L�X�g�X�N���[��X
		DWORD texty;					// �e�L�X�g�X�N���[��Y

		// �O���t�B�b�NVRAM
		int grptype;					// �O���t�B�b�N�^�C�v(0�`4)
		BOOL grpen[4];					// �O���t�B�b�N�u���b�N�\���t���O
		BOOL grppal[2048];				// �O���t�B�b�N�p���b�g�t���O
		BOOL grpmod[2048];				// �O���t�B�b�N�X�V�t���O(���C��)
		BOOL *grpflag;					// �O���t�B�b�N�X�V�t���O(16dot)
		DWORD *grpbuf[4];				// �O���t�B�b�N�u���b�N�o�b�t�@
		const BYTE* grpgv;				// �O���t�B�b�NGVRAM�|�C���^
		DWORD grpx[4];					// �O���t�B�b�N�u���b�N�X�N���[��X
		DWORD grpy[4];					// �O���t�B�b�N�u���b�N�X�N���[��Y

		// PCG
		BOOL pcgready[256 * 16];		// PCG����OK�t���O
		DWORD pcguse[256 * 16];			// PCG�g�p���J�E���g
		DWORD pcgpal[16];				// PCG�p���b�g�g�p�J�E���g
		DWORD *pcgbuf;					// PCG�o�b�t�@
		const BYTE* sprmem;				// �X�v���C�g������

		// �X�v���C�g
		DWORD **spptr;					// �X�v���C�g�|�C���^�o�b�t�@
		DWORD spreg[0x200];				// �X�v���C�g���W�X�^�ۑ�
		BOOL spuse[128];				// �X�v���C�g�g�p���t���O

		// BG
		DWORD bgreg[2][64 * 64];		// BG���W�X�^�{�ύX�t���O($10000)
		BOOL bgall[2][64];				// BG�ύX�t���O(�u���b�N�P��)
		BOOL bgdisp[2];					// BG�\���t���O
		BOOL bgarea[2];					// BG�\���G���A
		BOOL bgsize;					// BG�\���T�C�Y(16dot=TRUE)
		DWORD **bgptr[2];				// BG�|�C���^+�f�[�^
		BOOL bgmod[2][1024];			// BG�X�V�t���O
		DWORD bgx[2];					// BG�X�N���[��(X)
		DWORD bgy[2];					// BG�X�N���[��(Y)

		// BG/�X�v���C�g����
		BOOL bgspflag;					// BG/�X�v���C�g�\���t���O
		BOOL bgspdisp;					// BG/�X�v���C�gCPU/Video�t���O
		BOOL bgspmod[512];				// BG/�X�v���C�g�X�V�t���O
		DWORD *bgspbuf;					// BG/�X�v���C�g�o�b�t�@
		DWORD zero;						// �X�N���[���_�~�[(0)
	} render_t;

public:
	// ��{�t�@���N�V����
	Render(VM *p);
										// �R���X�g���N�^
	BOOL FASTCALL Init();
										// ������
	void FASTCALL Cleanup();
										// �N���[���A�b�v
	void FASTCALL Reset();
										// ���Z�b�g
	BOOL FASTCALL Save(Fileio *fio, int ver);
										// �Z�[�u
	BOOL FASTCALL Load(Fileio *fio, int ver);
										// ���[�h
	void FASTCALL ApplyCfg(const Config *config);
										// �ݒ�K�p

	// �O��API(�R���g���[��)
	void FASTCALL EnableAct(BOOL enable){ render.enable = enable; }
										// ��������
	BOOL FASTCALL IsActive() const		{ return render.act; }
										// �A�N�e�B�u��
	BOOL FASTCALL IsReady() const		{ return (BOOL)(render.count > 0); }
										// �`�惌�f�B�󋵎擾
	void FASTCALL Complete()			{ render.count = 0; }
										// �`�抮��
	void FASTCALL StartFrame();
										// �t���[���J�n(V-DISP)
	void FASTCALL EndFrame();
										// �t���[���I��(V-BLANK)
	void FASTCALL HSync(int raster)		{ render.last = raster; if (render.act) Process(); }
										// ��������(raster�܂ŏI���)
	void FASTCALL SetMixBuf(DWORD *buf, int width, int height);
										// �����o�b�t�@�w��
	render_t* FASTCALL GetWorkAddr() 	{ return &render; }
										// ���[�N�A�h���X�擾

	// �O��API(���)
	void FASTCALL SetCRTC();
										// CRTC�Z�b�g
	void FASTCALL SetVC();
										// VC�Z�b�g
	void FASTCALL SetContrast(int cont);
										// �R���g���X�g�ݒ�
	int FASTCALL GetContrast() const;
										// �R���g���X�g�擾
	void FASTCALL SetPalette(int index);
										// �p���b�g�ݒ�
	const DWORD* FASTCALL GetPalette() const;
										// �p���b�g�o�b�t�@�擾
	void FASTCALL TextMem(DWORD addr);
										// �e�L�X�gVRAM�ύX
	void FASTCALL TextScrl(DWORD x, DWORD y);
										// �e�L�X�g�X�N���[���ύX
	void FASTCALL TextCopy(DWORD src, DWORD dst, DWORD plane);
										// ���X�^�R�s�[
	void FASTCALL GrpMem(DWORD addr, DWORD block);
										// �O���t�B�b�NVRAM�ύX
	void FASTCALL GrpAll(DWORD line, DWORD block);
										// �O���t�B�b�NVRAM�ύX
	void FASTCALL GrpScrl(int block, DWORD x, DWORD y);
										// �O���t�B�b�N�X�N���[���ύX
	void FASTCALL SpriteReg(DWORD addr, DWORD data);
										// �X�v���C�g���W�X�^�ύX
	void FASTCALL BGScrl(int page, DWORD x, DWORD y);
										// BG�X�N���[���ύX
	void FASTCALL BGCtrl(int index, BOOL flag);
										// BG�R���g���[���ύX
	void FASTCALL BGMem(DWORD addr, WORD data);
										// BG�ύX
	void FASTCALL PCGMem(DWORD addr);
										// PCG�ύX

	const DWORD* FASTCALL GetTextBuf() const;
										// �e�L�X�g�o�b�t�@�擾
	const DWORD* FASTCALL GetGrpBuf(int index) const;
										// �O���t�B�b�N�o�b�t�@�擾
	const DWORD* FASTCALL GetPCGBuf() const;
										// PCG�o�b�t�@�擾
	const DWORD* FASTCALL GetBGSpBuf() const;
										// BG/�X�v���C�g�o�b�t�@�擾
	const DWORD* FASTCALL GetMixBuf() const;
										// �����o�b�t�@�擾

private:
	void FASTCALL Process();
										// �����_�����O
	void FASTCALL Video();
										// VC����
	void FASTCALL SetupGrp(int first);
										// �O���t�B�b�N�Z�b�g�A�b�v
	void FASTCALL Contrast();
										// �R���g���X�g����
	void FASTCALL Palette();
										// �p���b�g����
	void FASTCALL MakePalette();
										// �p���b�g�쐬
	DWORD FASTCALL ConvPalette(int color, int ratio);
										// �F�ϊ�
	void FASTCALL Text(int raster);
										// �e�L�X�g
	void FASTCALL Grp(int block, int raster);
										// �O���t�B�b�N
	void FASTCALL SpriteReset();
										// �X�v���C�g���Z�b�g
	void FASTCALL BGSprite(int raster);
										// BG/�X�v���C�g
	void FASTCALL BG(int page, int raster, DWORD *buf);
										// BG
	void FASTCALL BGBlock(int page, int y);
										// BG(���u���b�N)
	void FASTCALL Mix(int offset);
										// ����
	void FASTCALL MixGrp(int y, DWORD *buf);
										// ����(�O���t�B�b�N)
	CRTC *crtc;
										// CRTC
	VC *vc;
										// VC
	Sprite *sprite;
										// �X�v���C�g
	render_t render;
										// �����f�[�^
	BOOL cmov;
										// CMOV�L���b�V��
};

#endif	// render_h
