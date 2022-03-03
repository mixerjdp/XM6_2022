//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2004 �o�h�D(ytanaka@ipc-tokai.or.jp)
//	[ MFC �T�u�E�B���h�E(�����_��) ]
//
//---------------------------------------------------------------------------

#if defined(_WIN32)

#include "os.h"
#include "xm6.h"
#include "vm.h"
#include "render.h"
#include "mfc_sub.h"
#include "mfc_res.h"
#include "mfc_rend.h"

//===========================================================================
//
//	�����_���o�b�t�@�E�B���h�E
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	�R���X�g���N�^
//
//---------------------------------------------------------------------------
CRendBufWnd::CRendBufWnd(int nType)
{
	Render *render;

	// �����_���擾
	render = (Render*)::GetVM()->SearchDevice(MAKEID('R', 'E', 'N', 'D'));
	ASSERT(render);

	// �^�C�v�L��
	m_nType = nType;

	// �X�N���[���T�C�Y�A�E�B���h�E�p�����[�^�A�o�b�t�@�A�h���X
	switch (nType) {
		// TEXT
		case 0:
			m_nScrlWidth = 1024;
			m_nScrlHeight = 1024;
			m_dwID = MAKEID('T', 'E', 'X', 'B');
			::GetMsg(IDS_SWND_REND_TEXT, m_strCaption);
			m_pRendBuf = render->GetTextBuf();
			break;
		// GRP0
		case 1:
			m_nScrlWidth = 512;
			m_nScrlHeight = 1024;
			m_dwID = MAKEID('G', 'P', '0', 'B');
			::GetMsg(IDS_SWND_REND_GP0, m_strCaption);
			m_pRendBuf = render->GetGrpBuf(0);
			break;
		// GRP1
		case 2:
			m_nScrlWidth = 512;
			m_nScrlHeight = 1024;
			m_dwID = MAKEID('G', 'P', '1', 'B');
			::GetMsg(IDS_SWND_REND_GP1, m_strCaption);
			m_pRendBuf = render->GetGrpBuf(1);
			break;
		// GRP2
		case 3:
			m_nScrlWidth = 512;
			m_nScrlHeight = 1024;
			m_dwID = MAKEID('G', 'P', '2', 'B');
			::GetMsg(IDS_SWND_REND_GP2, m_strCaption);
			m_pRendBuf = render->GetGrpBuf(2);
			break;
		// GRP3
		case 4:
			m_nScrlWidth = 512;
			m_nScrlHeight = 1024;
			m_dwID = MAKEID('G', 'P', '3', 'B');
			::GetMsg(IDS_SWND_REND_GP3, m_strCaption);
			m_pRendBuf = render->GetGrpBuf(3);
			break;
		// BG/SP
		case 5:
			m_nScrlWidth = 512;
			m_nScrlHeight = 512;
			m_dwID = MAKEID('B', 'G', 'S', 'P');
			::GetMsg(IDS_SWND_REND_BGSPRITE, m_strCaption);
			m_pRendBuf = render->GetBGSpBuf();
			break;
		default:
			ASSERT(FALSE);
			m_nScrlWidth = 0;
			m_nScrlHeight = 0;
	}
}

//---------------------------------------------------------------------------
//
//	�Z�b�g�A�b�v
//
//---------------------------------------------------------------------------
void FASTCALL CRendBufWnd::Setup(int x, int y, int width, int height, BYTE *ptr)
{
	int i;
	int delta;
	int next;
	const DWORD *p;

	// �|�C���^�Z�o
	p = m_pRendBuf;
	switch (m_nType) {
		case 0:
			// �e�L�X�g
			p += (y << 10);
			next = 1024;
			break;
		// �O���t�B�b�N
		case 1:
		case 2:
		case 3:
		case 4:
		// BG/�X�v���C�g
		case 5:
			p += (y << 9);
			next = 512;
			break;
		default:
			ASSERT(FALSE);
			break;
	}
	p += x;

	// �I�[�o�[�΍�
	if ((y + height) > m_nScrlHeight) {
		height = m_nScrlHeight - y;
	}
	delta = 0;
	if ((x + width) > m_nScrlWidth) {
		delta = width - m_nScrlWidth + x;
		width = m_nScrlWidth - x;
	}

	// ���[�v
	for (i=0; i<height; i++) {
		// x, width�����Ă��ăR�s�[
		memcpy(ptr, p, (width << 2));
		p += next;
		ptr += (width << 2);
		ptr += (delta << 2);
	}
}

//---------------------------------------------------------------------------
//
//	���b�Z�[�W�X���b�h����̍X�V
//
//---------------------------------------------------------------------------
void FASTCALL CRendBufWnd::Update()
{
	int x;
	int y;
	DWORD rgb;
	CString string;

	// BMP�E�B���h�E�`�F�b�N
	if (!m_pBMPWnd) {
		return;
	}

	// �}�E�X�J�[�\���`�F�b�N
	if ((m_pBMPWnd->m_nCursorX < 0) || (m_pBMPWnd->m_nCursorY < 0)) {
		m_StatusBar.SetPaneText(0, "");
		return;
	}

	// ���W�v�Z�A�I�[�o�[�`�F�b�N
	x = m_pBMPWnd->m_nCursorX + m_pBMPWnd->m_nScrlX;
	y = m_pBMPWnd->m_nCursorY + m_pBMPWnd->m_nScrlY;
	if (x >= m_nScrlWidth) {
		return;
	}
	if (y >= m_nScrlHeight) {
		return;
	}

	// �\���f�[�^�쐬
	switch (m_nType) {
		case 0:
			rgb = m_pRendBuf[(y << 10) + x];
			break;
		case 1:
		case 2:
		case 3:
		case 4:
		case 5:
			rgb = m_pRendBuf[(y << 9) + x];
			break;
		default:
			ASSERT(FALSE);
			break;
	}
	string.Format("( %d, %d) R%d G%d B%d",
				x, y, (rgb >> 16) & 0xff, (rgb >> 8) & 0xff, (rgb & 0xff));
	m_StatusBar.SetPaneText(0, string);
}

//===========================================================================
//
//	�����o�b�t�@�E�B���h�E
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	�R���X�g���N�^
//
//---------------------------------------------------------------------------
CMixBufWnd::CMixBufWnd()
{
	Render *render;

	// ��{�p�����[�^
	m_nScrlWidth = 1024;
	m_nScrlHeight = 1024;
	m_dwID = MAKEID('M', 'I', 'X', 'B');
	::GetMsg(IDS_SWND_REND_MIX, m_strCaption);

	// �����_���擾
	render = (Render*)::GetVM()->SearchDevice(MAKEID('R', 'E', 'N', 'D'));
	ASSERT(render);

	// �A�h���X�擾
	m_pRendWork = render->GetWorkAddr();
	ASSERT(m_pRendWork);
}

//---------------------------------------------------------------------------
//
//	�Z�b�g�A�b�v
//
//---------------------------------------------------------------------------
void FASTCALL CMixBufWnd::Setup(int x, int y, int width, int height, BYTE *ptr)
{
	int i;
	int delta;
	int below;
	const DWORD *p;

	// x, y�`�F�b�N
	if (x >= m_pRendWork->mixwidth) {
		// �\���̈�Ȃ��B���ׂč�
		for (i=0; i<height; i++) {
			memset(ptr, 0, (width << 2));
			ptr += (width << 2);
		}
		return;
	}
	if (y >= m_pRendWork->mixheight) {
		// �\���̈�Ȃ��B���ׂč�
		for (i=0; i<height; i++) {
			memset(ptr, 0, (width << 2));
			ptr += (width << 2);
		}
		return;
	}

	// �|�C���^�Z�o
	p = m_pRendWork->mixbuf;
	ASSERT(p);
	p += (y * m_pRendWork->mixwidth);
	p += x;

	// �I�[�o�[�΍�
	below = 0;
	if ((y + height) > m_pRendWork->mixheight) {
		below = height - m_pRendWork->mixheight + y;
		height = m_pRendWork->mixheight - y;
	}
	delta = 0;
	if ((x + width) > m_pRendWork->mixwidth) {
		delta = width - m_pRendWork->mixwidth + x;
		width = m_pRendWork->mixwidth - x;
	}

	// ���[�v
	for (i=0; i<height; i++) {
		// x, width�����Ă��ăR�s�[
		memcpy(ptr, p, (width << 2));
		p += m_pRendWork->mixwidth;
		ptr += (width << 2);
		memset(ptr, 0, (delta << 2));
		ptr += (delta << 2);
	}

	// �]�v�ȉ�����������
	for (i=0; i<below; i++) {
		memset(ptr, 0, (width << 2));
		ptr += (width << 2);
		memset(ptr, 0, (delta << 2));
		ptr += (delta << 2);
	}
}

//---------------------------------------------------------------------------
//
//	���b�Z�[�W�X���b�h����̍X�V
//
//---------------------------------------------------------------------------
void FASTCALL CMixBufWnd::Update()
{
	int x;
	int y;
	DWORD rgb;
	CString string;

	// BMP�E�B���h�E�`�F�b�N
	if (!m_pBMPWnd) {
		return;
	}

	// �}�E�X�J�[�\���`�F�b�N
	if ((m_pBMPWnd->m_nCursorX < 0) || (m_pBMPWnd->m_nCursorY < 0)) {
		m_StatusBar.SetPaneText(0, "");
		return;
	}

	// ���W�v�Z�A�I�[�o�[�`�F�b�N
	x = m_pBMPWnd->m_nCursorX + m_pBMPWnd->m_nScrlX;
	y = m_pBMPWnd->m_nCursorY + m_pBMPWnd->m_nScrlY;
	if (x >= m_nScrlWidth) {
		return;
	}
	if (y >= m_nScrlHeight) {
		return;
	}

	// �\���f�[�^�쐬
	if (x >= m_pRendWork->mixwidth) {
		return;
	}
	if (y >= m_pRendWork->mixheight) {
		return;
	}
	rgb = m_pRendWork->mixbuf[(y * m_pRendWork->mixwidth) + x];

	// �\��
	string.Format("( %d, %d) R%d G%d B%d",
				x, y, (rgb >> 16) & 0xff, (rgb >> 8) & 0xff, (rgb & 0xff));
	m_StatusBar.SetPaneText(0, string);
}

//===========================================================================
//
//	PCG�o�b�t�@�E�B���h�E
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	�R���X�g���N�^
//
//---------------------------------------------------------------------------
CPCGBufWnd::CPCGBufWnd()
{
	Render *render;
	const Render::render_t *p;

	// ��{�p�����[�^
	m_nWidth = 28;
	m_nHeight = 16;
	m_nScrlWidth = 256;
	m_nScrlHeight = 4096;
	m_dwID = MAKEID('P', 'C', 'G', 'B');
	::GetMsg(IDS_SWND_REND_PCG, m_strCaption);

	// �����_���擾
	render = (Render*)::GetVM()->SearchDevice(MAKEID('R', 'E', 'N', 'D'));
	ASSERT(render);

	// �A�h���X�擾
	m_pPCGBuf = render->GetPCGBuf();
	ASSERT(m_pPCGBuf);
	p = render->GetWorkAddr();
	ASSERT(p);
	m_dwPCGBuf = p->pcguse; 
	ASSERT(m_dwPCGBuf);
}

//---------------------------------------------------------------------------
//
//	�Z�b�g�A�b�v
//
//---------------------------------------------------------------------------
void FASTCALL CPCGBufWnd::Setup(int x, int y, int width, int height, BYTE *ptr)
{
	int i;
	int j;
	int delta;
	const DWORD *p;
	DWORD buf[256];

	// x�`�F�b�N
	if (x >= 256) {
		// �\���̈�Ȃ��B���ׂč�
		for (i=0; i<height; i++) {
			memset(ptr, 0, (width << 2));
			ptr += (width << 2);
		}
		return;
	}

	// �I�[�o�[�΍�
	delta = 0;
	if ((x + width) > 256) {
		delta = width - 256 + x;
		width = 256 - x;
	}

	// ���[�v
	for (i=0; i<height; i++) {
		// �o�b�t�@�|�C���^�Z�o
		p = m_pPCGBuf;
		ASSERT((y >> 4) < 256);
		p += ((y >> 4) << 8);
		p += ((y & 0x0f) << 4);

		// �f�[�^�쐬
		memset(buf, 0, sizeof(buf));
		for (j=0; j<16; j++) {
			memcpy(&buf[j << 4], p, sizeof(DWORD) * 16);

			// �o�b�t�@��256x16x16�����A��֐i�߂�
			p += 0x10000;
		}

		// x, width�����Ă��ăR�s�[
		memcpy(ptr, buf, (width << 2));
		ptr += (width << 2);
		memset(ptr, 0, (delta << 2));
		ptr += (delta << 2);

		y++;
	}
}

//---------------------------------------------------------------------------
//
//	���b�Z�[�W�X���b�h����̍X�V
//
//---------------------------------------------------------------------------
void FASTCALL CPCGBufWnd::Update()
{
	int x;
	int y;
	CString string;
	int index;

	// BMP�E�B���h�E�`�F�b�N
	if (!m_pBMPWnd) {
		return;
	}

	// �}�E�X�J�[�\���`�F�b�N
	if ((m_pBMPWnd->m_nCursorX < 0) || (m_pBMPWnd->m_nCursorY < 0)) {
		m_StatusBar.SetPaneText(0, "");
		return;
	}

	// ���W�v�Z�A�I�[�o�[�`�F�b�N
	x = m_pBMPWnd->m_nCursorX + m_pBMPWnd->m_nScrlX;
	y = m_pBMPWnd->m_nCursorY + m_pBMPWnd->m_nScrlY;
	if (x >= m_nScrlWidth) {
		return;
	}
	if (y >= m_nScrlHeight) {
		return;
	}

	// �C���f�b�N�X�쐬
	index = y >> 4;

	// �\��
	string.Format("( %d, %d) Pal%1X [$%02X +%d +%d]",
				x, y, (x >> 4), index, (x & 0x0f), (y & 0x0f));
	m_StatusBar.SetPaneText(0, string);
}

#endif	// _WIN32
