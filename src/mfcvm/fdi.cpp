//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 �o�h�D(ytanaka@ipc-tokai.or.jp)
//	[ �t���b�s�[�f�B�X�N�C���[�W ]
//
//---------------------------------------------------------------------------

#include "os.h"
#include "xm6.h"
#include "filepath.h"
#include "fileio.h"
#include "fdd.h"
#include "fdi.h"

//===========================================================================
//
//	FDI�Z�N�^
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	�R���X�g���N�^
//
//---------------------------------------------------------------------------
FDISector::FDISector(BOOL mfm, const DWORD *chrn)
{
	int i;

	ASSERT(chrn);

	// CHRN,MFM�L��
	for (i=0; i<4; i++) {
		sec.chrn[i] = chrn[i];
	}
	sec.mfm = mfm;

	// ���̑�������
	sec.error = FDD_NODATA;
	sec.length = 0;
	sec.gap3 = 0;
	sec.buffer = NULL;
	sec.pos = 0;
	sec.changed = FALSE;
	sec.next = NULL;
}

//---------------------------------------------------------------------------
//
//	�f�X�g���N�^
//
//---------------------------------------------------------------------------
FDISector::~FDISector()
{
	// ���������
	if (sec.buffer) {
		delete[] sec.buffer;
		sec.buffer = NULL;
	}
}

//---------------------------------------------------------------------------
//
//	�������[�h
//
//---------------------------------------------------------------------------
void FASTCALL FDISector::Load(const BYTE *buf, int len, int gap, int err)
{
	ASSERT(this);
	ASSERT(!sec.buffer);
	ASSERT(sec.length == 0);
	ASSERT(buf);
	ASSERT(len > 0);
	ASSERT(gap > 0);

	// �����O�X������ɐݒ�
	sec.length = len;

	// �o�b�t�@�m��
	try {
		sec.buffer = new BYTE[len];
	}
	catch (...) {
		sec.buffer = NULL;
		sec.length = 0;
	}
	if (!sec.buffer) {
		sec.length = 0;
	}

	// �]��
	memcpy(sec.buffer, buf, sec.length);

	// ���[�N�ݒ�
	sec.gap3 = gap;
	sec.error = err;
	sec.changed = FALSE;
}

//---------------------------------------------------------------------------
//
//	�Z�N�^�}�b�`���邩
//
//---------------------------------------------------------------------------
BOOL FASTCALL FDISector::IsMatch(BOOL mfm, const DWORD *chrn) const
{
	int i;

	ASSERT(this);
	ASSERT(chrn);

	// MFM���r
	if (sec.mfm != mfm) {
		return FALSE;
	}

	// CHR���r
	for (i=0; i<3; i++) {
		if (chrn[i] != sec.chrn[i]) {
			return FALSE;
		}
	}

	// ������N��!=0�̏ꍇ�̂݁AN��r
	if (chrn[3] != 0) {
		if (chrn[3] != sec.chrn[3]) {
			return FALSE;
		}
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	CHRN���擾
//
//---------------------------------------------------------------------------
void FASTCALL FDISector::GetCHRN(DWORD *chrn) const
{
	int i;

	ASSERT(this);
	ASSERT(chrn);

	for (i=0; i<4; i++) {
		chrn[i] = sec.chrn[i];
	}
}

//---------------------------------------------------------------------------
//
//	���[�h
//
//---------------------------------------------------------------------------
int FASTCALL FDISector::Read(BYTE *buf) const
{
	ASSERT(this);
	ASSERT(buf);

	// �Z�N�^�o�b�t�@���Ȃ���Ή������Ȃ�
	if (!sec.buffer) {
		return sec.error;
	}

	// �]���{�G���[��Ԃ�
	memcpy(buf, sec.buffer, sec.length);
	return sec.error;
}

//---------------------------------------------------------------------------
//
//	���C�g
//
//---------------------------------------------------------------------------
int FASTCALL FDISector::Write(const BYTE *buf, BOOL deleted)
{
	ASSERT(this);
	ASSERT(buf);

	// �Z�N�^�o�b�t�@���Ȃ���Ή������Ȃ�
	if (!sec.buffer) {
		return sec.error;
	}

	// �G���[�������ɍs��
	sec.error &= ~FDD_DATACRC;
	sec.error &= ~FDD_DDAM;
	if (deleted) {
		sec.error |= FDD_DDAM;
	}

	// ��v����Ή������Ȃ�
	if (memcmp(sec.buffer, buf, sec.length) == 0) {
		return sec.error;
	}

	// �]��
	memcpy(sec.buffer, buf, sec.length);

	// �X�V�t���O�����āA�G���[��Ԃ�
	sec.changed = TRUE;
	return sec.error;
}

//---------------------------------------------------------------------------
//
//	�t�B��
//
//---------------------------------------------------------------------------
int FASTCALL FDISector::Fill(DWORD d)
{
	int i;
	BOOL changed;

	ASSERT(this);

	// �Z�N�^�o�b�t�@���Ȃ���Ή������Ȃ�
	if (!sec.buffer) {
		return sec.error;
	}

	// ��r���Ȃ��珑������
	changed = FALSE;
	for (i=0; i<sec.length; i++) {
		if (sec.buffer[i] != (BYTE)d) {
			// 1��ł��������A�t�B������break
			memset(sec.buffer, d, sec.length);
			changed = TRUE;
			break;
		}
	}

	// �������݂ł̓f�[�^CRC�͔������Ȃ����̂Ƃ���
	sec.error &= ~FDD_DATACRC;

	// �X�V�t���O�����āA�G���[��Ԃ�
	if (changed) {
		sec.changed = TRUE;
	}
	return sec.error;
}

//===========================================================================
//
//	FDI�g���b�N
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	�R���X�g���N�^
//
//---------------------------------------------------------------------------
FDITrack::FDITrack(FDIDisk *disk, int track, BOOL hd)
{
	ASSERT(disk);
	ASSERT((track >= 0) && (track <= 163));

	// �f�B�X�N�A�g���b�N�L��
	trk.disk = disk;
	trk.track = track;

	// ���̑����[�N�G���A
	trk.init = FALSE;
	trk.sectors[0] = 0;
	trk.sectors[1] = 0;
	trk.sectors[2] = 0;
	trk.hd = hd;
	trk.mfm = TRUE;
	trk.first = NULL;
	trk.next = NULL;
}

//---------------------------------------------------------------------------
//
//	�f�X�g���N�^
//
//---------------------------------------------------------------------------
FDITrack::~FDITrack()
{
	// �N���A
	ClrSector();
}

//---------------------------------------------------------------------------
//
//	�Z�[�u
//	��2HD,DIM�Ȃǂ̃Z�N�^�A���������݃^�C�v����
//
//---------------------------------------------------------------------------
BOOL FASTCALL FDITrack::Save(const Filepath& path, DWORD offset)
{
	Fileio fio;
	FDISector *sector;
	BOOL changed;

	ASSERT(this);

	// ����������Ă��Ȃ���Ώ������ޕK�v�Ȃ�
	if (!IsInit()) {
		return TRUE;
	}

	// �Z�N�^���܂���āA�������܂�Ă���Z�N�^�����邩
	sector = GetFirst();
	changed = FALSE;
	while (sector) {
		if (sector->IsChanged()) {
			changed = TRUE;
		}
		sector = sector->GetNext();
	}

	// �ǂ���������܂�Ă��Ȃ���Ή������Ȃ�
	if (!changed) {
		return TRUE;
	}

	// �t�@�C���I�[�v��
	if (!fio.Open(path, Fileio::ReadWrite)) {
		return FALSE;
	}

	// ���[�v
	sector = GetFirst();
	while (sector) {
		// �ύX����Ă��Ȃ���΁A����
		if (!sector->IsChanged()) {
			offset += sector->GetLength();
			sector = sector->GetNext();
			continue;
		}

		// �V�[�N
		if (!fio.Seek(offset)) {
			fio.Close();
			return FALSE;
		}

		// ��������
		if (!fio.Write(sector->GetSector(), sector->GetLength())) {
			fio.Close();
			return FALSE;
		}

		// �t���O�𗎂Ƃ�
		sector->ClrChanged();

		// ����
		offset += sector->GetLength();
		sector = sector->GetNext();
	}

	// �I��
	fio.Close();
	return TRUE;
}

//---------------------------------------------------------------------------
//
//	�Z�[�u
//	��2HD,DIM�Ȃǂ̃Z�N�^�A���������݃^�C�v����
//
//---------------------------------------------------------------------------
BOOL FASTCALL FDITrack::Save(Fileio *fio, DWORD offset)
{
	FDISector *sector;
	BOOL changed;

	ASSERT(this);
	ASSERT(fio);

	// ����������Ă��Ȃ���Ώ������ޕK�v�Ȃ�
	if (!IsInit()) {
		return TRUE;
	}

	// �Z�N�^���܂���āA�������܂�Ă���Z�N�^�����邩
	sector = GetFirst();
	changed = FALSE;
	while (sector) {
		if (sector->IsChanged()) {
			changed = TRUE;
		}
		sector = sector->GetNext();
	}

	// �ǂ���������܂�Ă��Ȃ���Ή������Ȃ�
	if (!changed) {
		return TRUE;
	}

	// ���[�v
	sector = GetFirst();
	while (sector) {
		// �ύX����Ă��Ȃ���΁A����
		if (!sector->IsChanged()) {
			offset += sector->GetLength();
			sector = sector->GetNext();
			continue;
		}

		// �V�[�N
		if (!fio->Seek(offset)) {
			return FALSE;
		}

		// ��������
		if (!fio->Write(sector->GetSector(), sector->GetLength())) {
			return FALSE;
		}

		// �t���O�𗎂Ƃ�
		sector->ClrChanged();

		// ����
		offset += sector->GetLength();
		sector = sector->GetNext();
	}

	// �I��
	return TRUE;
}

//---------------------------------------------------------------------------
//
//	�����t�H�[�}�b�g
//	���Z�[�u�͕ʂɍs������
//
//---------------------------------------------------------------------------
void FASTCALL FDITrack::Create(DWORD phyfmt)
{
	ASSERT(this);

	// ���ׂẴZ�N�^���폜
	ClrSector();

	// �����t�H�[�}�b�g��
	switch (phyfmt) {
		// �W��2HD
		case FDI_2HD:
			Create2HD(153);
			break;

		// �I�[�o�g���b�N2HD
		case FDI_2HDA:
			Create2HD(159);
			break;

		// 2HS
		case FDI_2HS:
			Create2HS();
			break;

		// 2HC
		case FDI_2HC:
			Create2HC();
			break;

		// 2HDE
		case FDI_2HDE:
			Create2HDE();
			break;

		// 2HQ
		case FDI_2HQ:
			Create2HQ();
			break;

		// N88-BASIC
		case FDI_N88B:
			CreateN88B();
			break;

		// OS-9/68000
		case FDI_OS9:
			CreateOS9();
			break;

		// 2DD
		case FDI_2DD:
			Create2DD();
			break;

		// ���̑�
		default:
			ASSERT(FALSE);
			return;
	}

	// �Z�N�^������Ȃ�A��ŕK���Z�[�u�����邽�߂ɁA���������S�ύX��ԂƂ���
	if (GetAllSectors() > 0) {
		trk.init = TRUE;
		ForceChanged();
	}
}

//---------------------------------------------------------------------------
//
//	�����t�H�[�}�b�g(2HD, 2HDA)
//
//---------------------------------------------------------------------------
void FASTCALL FDITrack::Create2HD(int lim)
{
	int i;
	FDISector *sector;
	DWORD chrn[4];
	BYTE buf[0x400];

	ASSERT(this);
	ASSERT(trk.hd);

	// �g���b�N�͎w�萔�܂�
	if (trk.track > lim) {
		return;
	}

	// C,H,N�쐬
	chrn[0] = trk.track >> 1;
	chrn[1] = trk.track & 0x01;
	chrn[3] = 0x03;

	// �o�b�t�@������
	memset(buf, 0xe5, sizeof(buf));

	// �����O�X3�~8�Z�N�^(MFM)
	for (i=0; i<8; i++) {
		// R�쐬
		chrn[2] = i + 1;

		// �Z�N�^�쐬
		sector = new FDISector(TRUE, chrn);

		// �f�[�^���[�h
		sector->Load(buf, 0x400, 0x74, FDD_NOERROR);

		// �ǉ�
		AddSector(sector);
	}
}

//---------------------------------------------------------------------------
//
//	�����t�H�[�}�b�g(2HS)
//
//---------------------------------------------------------------------------
void FASTCALL FDITrack::Create2HS()
{
	int i;
	FDISector *sector;
	DWORD chrn[4];
	BYTE buf[0x400];

	ASSERT(this);
	ASSERT(trk.hd);

	// �g���b�N��159�܂�
	if (trk.track > 159) {
		return;
	}

	// C,H,N�쐬
	chrn[0] = trk.track >> 1;
	chrn[1] = trk.track & 0x01;
	chrn[3] = 0x03;

	// �o�b�t�@������
	memset(buf, 0xe5, sizeof(buf));

	// �����O�X3�~9�Z�N�^(MFM)
	for (i=0; i<9; i++) {
		// R�쐬(���Ⴀ��)
		if ((trk.track == 0) && (i == 0)) {
			chrn[2] = 0x01;
		}
		else {
			chrn[2] = 10 + i;
		}

		// �Z�N�^�쐬
		sector = new FDISector(TRUE, chrn);

		// �f�[�^���[�h
		sector->Load(buf, 0x400, 0x39, FDD_NOERROR);

		// �ǉ�
		AddSector(sector);
	}
}

//---------------------------------------------------------------------------
//
//	�����t�H�[�}�b�g(2HC)
//
//---------------------------------------------------------------------------
void FASTCALL FDITrack::Create2HC()
{
	int i;
	FDISector *sector;
	DWORD chrn[4];
	BYTE buf[0x200];

	ASSERT(this);
	ASSERT(trk.hd);

	// �g���b�N��159�܂�
	if (trk.track > 159) {
		return;
	}

	// C,H,N�쐬
	chrn[0] = trk.track >> 1;
	chrn[1] = trk.track & 0x01;
	chrn[3] = 0x02;

	// �o�b�t�@������
	memset(buf, 0xe5, sizeof(buf));

	// �����O�X2�~15�Z�N�^(MFM)
	for (i=0; i<15; i++) {
		// R�쐬
		chrn[2] = i + 1;

		// �Z�N�^�쐬
		sector = new FDISector(TRUE, chrn);

		// �f�[�^���[�h
		sector->Load(buf, 0x200, 0x54, FDD_NOERROR);

		// �ǉ�
		AddSector(sector);
	}
}

//---------------------------------------------------------------------------
//
//	�����t�H�[�}�b�g(2HDE)
//
//---------------------------------------------------------------------------
void FASTCALL FDITrack::Create2HDE()
{
	int i;
	FDISector *sector;
	DWORD chrn[4];
	BYTE buf[0x400];

	ASSERT(this);
	ASSERT(trk.hd);

	// �g���b�N��159�܂�
	if (trk.track > 159) {
		return;
	}

	// C,N�쐬
	chrn[0] = trk.track >> 1;
	chrn[3] = 0x03;

	// �o�b�t�@������
	memset(buf, 0xe5, sizeof(buf));

	// �����O�X3�~9�Z�N�^(MFM)
	for (i=0; i<9; i++) {
		// H�쐬(���Ⴀ��)
		chrn[1] = 0x80 + (trk.track & 1);
		if ((trk.track == 0) && (i == 0)) {
			chrn[1] = 0x00;
		}

		// R�쐬
		chrn[2] = i + 1;

		// �Z�N�^�쐬
		sector = new FDISector(TRUE, chrn);

		// �f�[�^���[�h
		sector->Load(buf, 0x400, 0x39, FDD_NOERROR);

		// �ǉ�
		AddSector(sector);
	}
}

//---------------------------------------------------------------------------
//
//	�����t�H�[�}�b�g(2HQ)
//
//---------------------------------------------------------------------------
void FASTCALL FDITrack::Create2HQ()
{
	int i;
	FDISector *sector;
	DWORD chrn[4];
	BYTE buf[0x200];

	ASSERT(this);
	ASSERT(trk.hd);

	// �g���b�N��159�܂�
	if (trk.track > 159) {
		return;
	}

	// C,H,N�쐬
	chrn[0] = trk.track >> 1;
	chrn[1] = trk.track & 0x01;
	chrn[3] = 0x02;

	// �o�b�t�@������
	memset(buf, 0xe5, sizeof(buf));

	// �����O�X2�~18�Z�N�^(MFM)
	for (i=0; i<18; i++) {
		// R�쐬
		chrn[2] = i + 1;

		// �Z�N�^�쐬
		sector = new FDISector(TRUE, chrn);

		// �f�[�^���[�h
		sector->Load(buf, 0x200, 0x54, FDD_NOERROR);

		// �ǉ�
		AddSector(sector);
	}
}

//---------------------------------------------------------------------------
//
//	�����t�H�[�}�b�g(N88-BASIC)
//
//---------------------------------------------------------------------------
void FASTCALL FDITrack::CreateN88B()
{
	int i;
	FDISector *sector;
	DWORD chrn[4];
	BYTE buf[0x100];

	ASSERT(this);
	ASSERT(trk.hd);

	// �g���b�N��153�܂�
	if (trk.track > 153) {
		return;
	}

	// C,H�쐬
	chrn[0] = trk.track >> 1;
	chrn[1] = trk.track & 0x01;

	// �o�b�t�@������
	memset(buf, 0xe5, sizeof(buf));

	// �g���b�N0�͗�O
	if (trk.track == 0) {
		// �����O�X0�~26�Z�N�^(FM)
		chrn[3] = 0;
		for (i=0; i<26; i++) {
			// R�쐬
			chrn[2] = i + 1;

			// �Z�N�^�쐬
			sector = new FDISector(FALSE, chrn);

			// �f�[�^���[�h
			sector->Load(buf, 0x80, 0x1a, FDD_NOERROR);

			// �ǉ�
			AddSector(sector);
		}
		return;
	}

	// �����O�X1�~26�Z�N�^(MFM)
	chrn[3] = 1;
	for (i=0; i<26; i++) {
		// R�쐬
		chrn[2] = i + 1;

		// �Z�N�^�쐬
		sector = new FDISector(TRUE, chrn);

		// �f�[�^���[�h
		sector->Load(buf, 0x100, 0x33, FDD_NOERROR);

		// �ǉ�
		AddSector(sector);
	}
}

//---------------------------------------------------------------------------
//
//	�����t�H�[�}�b�g(OS-9/68000)
//
//---------------------------------------------------------------------------
void FASTCALL FDITrack::CreateOS9()
{
	int i;
	FDISector *sector;
	DWORD chrn[4];
	BYTE buf[0x100];

	ASSERT(this);
	ASSERT(trk.hd);

	// �g���b�N��153�܂�
	if (trk.track > 153) {
		return;
	}

	// C,H,N�쐬
	chrn[0] = trk.track >> 1;
	chrn[1] = trk.track & 0x01;
	chrn[3] = 1;

	// �o�b�t�@������
	memset(buf, 0xe5, sizeof(buf));

	// �����O�X1�~26�Z�N�^(MFM)
	for (i=0; i<26; i++) {
		// R�쐬
		chrn[2] = i + 1;

		// �Z�N�^�쐬
		sector = new FDISector(TRUE, chrn);

		// �f�[�^���[�h
		sector->Load(buf, 0x100, 0x33, FDD_NOERROR);

		// �ǉ�
		AddSector(sector);
	}
}

//---------------------------------------------------------------------------
//
//	�����t�H�[�}�b�g(2DD)
//
//---------------------------------------------------------------------------
void FASTCALL FDITrack::Create2DD()
{
	int i;
	FDISector *sector;
	DWORD chrn[4];
	BYTE buf[0x200];

	ASSERT(this);
	ASSERT(!trk.hd);

	// �g���b�N��159�܂�
	if (trk.track > 159) {
		return;
	}

	// C,H,N�쐬
	chrn[0] = trk.track >> 1;
	chrn[1] = trk.track & 0x01;
	chrn[3] = 0x02;

	// �o�b�t�@������
	memset(buf, 0xe5, sizeof(buf));

	// �����O�X2�~9�Z�N�^(MFM)
	for (i=0; i<9; i++) {
		// R�쐬
		chrn[2] = i + 1;

		// �Z�N�^�쐬
		sector = new FDISector(TRUE, chrn);

		// �f�[�^���[�h
		sector->Load(buf, 0x200, 0x54, FDD_NOERROR);

		// �ǉ�
		AddSector(sector);
	}
}

//---------------------------------------------------------------------------
//
//	���[�hID
//	�����̃X�e�[�^�X��Ԃ�(�G���[��OR�����)
//		FDD_NOERROR		�G���[�Ȃ�
//		FDD_MAM			�w�薧�x�ł̓A���t�H�[�}�b�g
//		FDD_NODATA		�w�薧�x�ł̓A���t�H�[�}�b�g���A
//						�܂��͗L���ȃZ�N�^���ׂ�ID CRC
//
//---------------------------------------------------------------------------
int FASTCALL FDITrack::ReadID(DWORD *buf, BOOL mfm)
{
	FDISector *sector;
	DWORD pos;
	int status;
	int num;
	int match;

	ASSERT(this);
	ASSERT(buf);
	ASSERT(trk.disk);

	// �X�e�[�^�X������
	status = FDD_NOERROR;

	// HD�t���O������Ȃ���΁A�G���[��������
	if (GetDisk()->IsHD() != trk.hd) {
		status |= FDD_MAM;
		status |= FDD_NODATA;
		pos = GetDisk()->GetRotationTime();
		pos = (pos * 2) - GetDisk()->GetRotationPos();
		GetDisk()->SetSearch(pos);
		return status;
	}

	// ���x�������AID CRC�̂Ȃ��Z�N�^�̌��𐔂���
	num = 0;
	match = 0;
	sector = GetFirst();
	while (sector) {
		// ���x���}�b�`���邩
		if (sector->IsMFM() == mfm) {
			match++;

			// ID CRC�G���[���Ȃ���
			if (!(sector->GetError() & FDD_IDCRC)) {
				num++;
			}
		}
		sector = sector->GetNext();
	}

	// ���x���}�b�`����f�[�^���Ȃ�
	if (match == 0) {
		status |= FDD_MAM;
	}

	// ID CRC�̂Ȃ��Z�N�^���Ȃ�
	if (num == 0) {
		status |= FDD_NODATA;
	}

	// �ǂ���ł����s�B�����ɂ����鎞�Ԃ̓C���f�b�N�X�z�[���Q�񌟏o
	if (status != FDD_NOERROR) {
		pos = GetDisk()->GetRotationTime();
		pos = (pos * 2) - GetDisk()->GetRotationPos();
		GetDisk()->SetSearch(pos);
		return status;
	}

	// �J�����g�|�W�V�����ȍ~�̍ŏ��̃Z�N�^���擾�B���������x����������
	sector = GetCurSector();
	ASSERT(sector);
	for (;;) {
		if (!sector) {
			sector = GetFirst();
		}
		ASSERT(sector);

		// ���x����v���Ȃ���΃X�L�b�v
		if (sector->IsMFM() != mfm) {
			sector = sector->GetNext();
			continue;
		}

		// ID CRC�Ȃ�X�L�b�v
		if (sector->GetError() & FDD_IDCRC) {
			sector = sector->GetNext();
			continue;
		}

		// �I��
		break;
	}

	// CHRN���擾
	sector->GetCHRN(buf);

	// �����ɂ����鎞�Ԃ�ݒ�
	pos = sector->GetPos();
	GetDisk()->CalcSearch(pos);

	// �G���[����
	return status;
}

//---------------------------------------------------------------------------
//
//	���[�h�Z�N�^
//	�����̃X�e�[�^�X��Ԃ�(�G���[��OR�����)
//		FDD_NOERROR		�G���[�Ȃ�
//		FDD_MAM			�w�薧�x�ł̓A���t�H�[�}�b�g
//		FDD_NODATA		�w�薧�x�ł̓A���t�H�[�}�b�g���A
//						�܂��͗L���ȃZ�N�^��S��������R����v���Ȃ�
//		FDD_NOCYL		��������ID��C����v�����AFF�łȂ��Z�N�^��������
//		FDD_BADCYL		��������ID��C����v�����AFF�ƂȂ��Ă���Z�N�^��������
//		FDD_IDCRC		ID�t�B�[���h��CRC�G���[������
//		FDD_DATACRC		DATA�t�B�[���h��CRC�G���[������
//		FDD_DDAM		�f���[�e�b�h�Z�N�^�ł���
//
//---------------------------------------------------------------------------
int FASTCALL FDITrack::ReadSector(BYTE *buf, int *len, BOOL mfm, const DWORD *chrn)
{
	FDISector *sector;
	DWORD work[4];
	DWORD pos;
	int status;
	int i;
	int num;

	ASSERT(this);
	ASSERT(len);
	ASSERT(chrn);

	// ���x�������Z�N�^�����擾
	if (mfm) {
		num = GetMFMSectors();
	}
	else {
		num = GetFMSectors();
	}

	// HD�t���O������Ȃ���΁A�����I�ɃZ�N�^��0�Ƃ���
	if (GetDisk()->IsHD() != trk.hd) {
		num = 0;
	}

	// 0�Ȃ�MAM,NODATA(�Z�N�^�͂P���Ȃ�)
	if (num == 0) {
		// �����ɂ����鎞�Ԃ̓C���f�b�N�X�z�[���Q�񌟏o
		pos = GetDisk()->GetRotationTime();
		pos = (pos * 2) - GetDisk()->GetRotationPos();
		GetDisk()->SetSearch(pos);
		return FDD_NODATA | FDD_MAM;
	}

	// �J�����g�|�W�V�����ȍ~�̍ŏ��̃Z�N�^���擾
	sector = GetCurSector();

	// Number�������[�v�A�Z�N�^����(R�������Ȃ�)
	status = FDD_NOERROR;
	num = GetAllSectors();
	for (i=0; i<num; i++) {
		if (!sector) {
			sector = GetFirst();
		}
		ASSERT(sector);

		// ���x���}�b�`���Ȃ���ΌJ��Ԃ�
		if (sector->IsMFM() != mfm) {
			sector = sector->GetNext();
			continue;
		}

		// CHRN���擾�AC���`�F�b�N
		sector->GetCHRN(work);
		if (work[0] != chrn[0]) {
			if (work[0] == 0xff) {
				status |= FDD_BADCYL;
			}
			else {
				status |= FDD_NOCYL;
			}
		}

		// R��v�Ŕ�����
		if (work[2] == chrn[2]) {
			break;
		}

		// ���̃Z�N�^��
		sector = sector->GetNext();
	}

	// R��v���Ȃ���΁ANODATA�ŕԂ�
	if (work[2] != chrn[2]) {
		status |= FDD_NODATA;

		// �����ɂ����鎞�Ԃ̓C���f�b�N�X�z�[���Q�񌟏o
		pos = GetDisk()->GetRotationTime();
		pos = (pos * 2) - GetDisk()->GetRotationPos();
		GetDisk()->SetSearch(pos);
		return status;
	}

	// �����ɂ����鎞�Ԃ�ݒ�
	pos = sector->GetPos();
	GetDisk()->CalcSearch(pos);

	// buf���w�肳��Ă���ꍇ�̂݁A�o�b�t�@�փf�[�^������BNULL�Ȃ�X�e�[�^�X�̂�
	*len = sector->GetLength();
	if (buf) {
		status = sector->Read(buf);
	}
	else {
		status = sector->GetError();
	}

	// �X�e�[�^�X���}�X�N
	status &= (FDD_IDCRC | FDD_DATACRC | FDD_DDAM);
	return status;
}

//---------------------------------------------------------------------------
//
//	���C�g�Z�N�^
//	�����̃X�e�[�^�X��Ԃ�(�G���[��OR�����)
//		FDD_NOERROR		�G���[�Ȃ�
//		FDD_MAM			�w�薧�x�ł̓A���t�H�[�}�b�g
//		FDD_NODATA		�w�薧�x�ł̓A���t�H�[�}�b�g���A
//						�܂��͗L���ȃZ�N�^��S��������CHRN����v���Ȃ�
//		FDD_NOCYL		��������ID��C����v�����AFF�łȂ��Z�N�^��������
//		FDD_BADCYL		��������ID��C����v�����AFF�ƂȂ��Ă���Z�N�^��������
//		FDD_IDCRC		ID�t�B�[���h��CRC�G���[������
//		FDD_DDAM		�f���[�e�b�h�Z�N�^�ł���
//
//---------------------------------------------------------------------------
int FASTCALL FDITrack::WriteSector(const BYTE *buf, int *len, BOOL mfm, const DWORD *chrn, BOOL deleted)
{
	FDISector *sector;
	DWORD work[4];
	DWORD pos;
	int status;
	int i;
	int num;

	ASSERT(this);
	ASSERT(len);
	ASSERT(chrn);

	// ���x�������Z�N�^�����擾
	if (mfm) {
		num = GetMFMSectors();
	}
	else {
		num = GetFMSectors();
	}

	// HD�t���O������Ȃ���΁A�����I�ɃZ�N�^��0�Ƃ���
	if (GetDisk()->IsHD() != trk.hd) {
		num = 0;
	}

	// 0�Ȃ�MAM,NODATA(�Z�N�^�͂P���Ȃ�)
	if (num == 0) {
		// �����ɂ����鎞�Ԃ̓C���f�b�N�X�z�[���Q�񌟏o
		pos = GetDisk()->GetRotationTime();
		pos = (pos * 2) - GetDisk()->GetRotationPos();
		GetDisk()->SetSearch(pos);
		return FDD_NODATA | FDD_MAM;
	}

	// �J�����g�|�W�V�����ȍ~�̍ŏ��̃Z�N�^���擾
	sector = GetCurSector();

	// Number�������[�v�A�Z�N�^����(CHRN�`�F�b�N)
	status = FDD_NOERROR;
	num = GetAllSectors();
	for (i=0; i<num; i++) {
		if (!sector) {
			sector = GetFirst();
		}
		ASSERT(sector);

		// ���x���}�b�`���Ȃ���ΌJ��Ԃ�
		if (sector->IsMFM() != mfm) {
			sector = sector->GetNext();
			continue;
		}

		// CHRN���擾�AC���`�F�b�N
		sector->GetCHRN(work);
		if (work[0] != chrn[0]) {
			if (work[0] == 0xff) {
				status |= FDD_BADCYL;
			}
			else {
				status |= FDD_NOCYL;
			}
		}

		// CHRN��v�Ŕ�����
		if (sector->IsMatch(mfm, chrn)) {
			break;
		}

		// ���̃Z�N�^��
		sector = sector->GetNext();
	}

	// �Z�N�^��������Ȃ���΁ANODATA
	if (i >= num) {
		status |= FDD_NODATA;

		// �����ɂ����鎞�Ԃ̓C���f�b�N�X�z�[���Q�񌟏o
		pos = GetDisk()->GetRotationTime();
		pos = (pos * 2) - GetDisk()->GetRotationPos();
		GetDisk()->SetSearch(pos);
		return status;
	}

	// �����ɂ����鎞�Ԃ�ݒ�
	pos = sector->GetPos();
	GetDisk()->CalcSearch(pos);

	// buf���w�肳��Ă���ꍇ�̂݁A�������ށBNULL�Ȃ�X�e�[�^�X�̂�
	*len = sector->GetLength();
	if (buf) {
		status = sector->Write(buf, deleted);
	}
	else {
		status = sector->GetError();
	}

	// �X�e�[�^�X���}�X�N
	status &= (FDD_IDCRC | FDD_DDAM);
	return status;
}

//---------------------------------------------------------------------------
//
//	���[�h�_�C�A�O
//	�����̃X�e�[�^�X��Ԃ�(�G���[��OR�����)
//		FDD_NOERROR		�G���[�Ȃ�
//		FDD_MAM			�w�薧�x�ł̓A���t�H�[�}�b�g
//		FDD_NODATA		�w�薧�x�ł̓A���t�H�[�}�b�g���A
//						�܂��͗L���ȃZ�N�^��S��������R����v���Ȃ�
//		FDD_IDCRC		ID�t�B�[���h��CRC�G���[������
//		FDD_DATACRC		�f�[�^�t�B�[���h��CRC�G���[������
//		FDD_DDAM		�f���[�e�b�h�Z�N�^�ł���
//
//---------------------------------------------------------------------------
int FASTCALL FDITrack::ReadDiag(BYTE *buf, int *len, BOOL mfm, const DWORD *chrn)
{
	FDISector *sector;
	DWORD work[4];
	int num;
	int total;
	int start;
	int length;
	int status;
	int error;
	BYTE *ptr;
	DWORD pos;
	BOOL found;

	ASSERT(this);
	ASSERT(len);
	ASSERT(chrn);

	// ���x�������Z�N�^�����擾
	if (mfm) {
		num = GetMFMSectors();
	}
	else {
		num = GetFMSectors();
	}

	// HD�t���O������Ȃ���΁A�����I�ɃZ�N�^��0�Ƃ���
	if (GetDisk()->IsHD() != trk.hd) {
		num = 0;
	}

	// 0�Ȃ�MAM,NODATA(�Z�N�^�͂P���Ȃ�)
	if (num == 0) {
		// �����ɂ����鎞�Ԃ̓C���f�b�N�X�z�[��2�񌟏o
		pos = GetDisk()->GetRotationTime();
		pos = (pos * 2) - GetDisk()->GetRotationPos();
		GetDisk()->SetSearch(pos);
		return FDD_NODATA | FDD_MAM;
	}

	// ���[�N�m��
	try {
		ptr = new BYTE[0x4000];
	}
	catch (...) {
		return FDD_NODATA | FDD_MAM;
	}
	if (!ptr) {
		return FDD_NODATA | FDD_MAM;
	}

	// �����O�X�����߂�B�ő�N=7(4000h)
	length = chrn[3];
	if (length > 7) {
		length = 7;
	}
	length = 1 << (length + 7);

	// �����ɂ����鎞�Ԃ͐擪�Z�N�^�擾����
	sector = GetFirst();
	pos = sector->GetPos();
	GetDisk()->SetSearch(pos);

	// �X�e�[�^�X������
	status = FDD_NOERROR;

	// GAP1�쐬
	total = MakeGAP1(ptr, 0);

	// ���[�v
	found = FALSE;
	while (sector) {
		// �Z�N�^��MFM����v���Ȃ���΁Acontinue
		if (sector->IsMFM() != mfm) {
			sector = sector->GetNext();
			continue;
		}

		// �Z�N�^�f�[�^���쐬
		total = MakeSector(ptr, total, sector);

		// CHRN�擾
		sector->GetCHRN(work);

		// R, N�Ƃ��Ɉ�v�����ꍇ�̂݁Afound
		if (work[2] == chrn[2]) {
			if (work[3] == chrn[3]) {
				found = TRUE;
			}
		}

		// IDCRC, DATACRC, DDAM
		error = sector->GetError();
		error &= (FDD_IDCRC | FDD_DATACRC | FDD_DDAM);
		status |= error;

		// ����
		sector = sector->GetNext();
	}

	// ���ʂ�����
	if (!found) {
		// (�O���[�f�B�A�n)
		status |= (FDD_NODATA | FDD_DATAERR);
	}

	// GAP4�쐬
	total = MakeGAP4(ptr, total);

	// �o�b�t�@���^�����Ă��Ȃ���΁A�����ŏI��
	if (!buf) {
		*len = 0;
		delete[] ptr;
		return status;
	}

	// �X�^�[�g�ʒu�����߂�(�ŏ��̃Z�N�^�̃f�[�^���O�܂ŃX�L�b�v)
	if (mfm) {
		start = 60 + GetGAP1();
	}
	else {
		start = 31 + GetGAP1();
	}

	// �P��ŏI���ꍇ
	if (length <= (total - start)) {
		memcpy(buf, &ptr[start], length);
		*len = length;
		delete[] ptr;
		return status;
	}

	// �ŏ���1�������
	memcpy(buf, &ptr[start], (total - start));
	*len = (total - start);
	length -= (total - start);
	buf += (total - start);

	// ���̃��[�v
	while (length > 0) {
		if (length <= total) {
			// ���܂�
			memcpy(buf, ptr, length);
			*len += length;
			break;
		}
		// �S�ē����
		memcpy(buf, ptr, total);
		*len += total;
		length -= total;
		buf += total;
	}

	// ptr���
	delete[] ptr;

	// �I��
	return status;
}

//---------------------------------------------------------------------------
//
//	���C�gID
//	��2HD,DIM�Ȃǂ̃Z�N�^�Œ�^�C�v����
//	�����̃X�e�[�^�X��Ԃ�(�G���[��OR�����)
//		FDD_NOERROR		�G���[�Ȃ�
//		FDD_NOTWRITE	�������݋֎~
//
//---------------------------------------------------------------------------
int FASTCALL FDITrack::WriteID(const BYTE *buf, DWORD d, int sc, BOOL mfm, int /*gpl*/)
{
	FDISector *sector;
	DWORD chrn[4];
	int num;
	int i;
	DWORD pos;

	ASSERT(this);
	ASSERT(sc > 0);

	// ���݂̃Z�N�^���擾
	if (IsMFM()) {
		num = GetMFMSectors();
	}
	else {
		num = GetFMSectors();
	}

	// HD�t���O������Ȃ���΁A�����I�ɃZ�N�^��0�Ƃ���
	if (GetDisk()->IsHD() != trk.hd) {
		num = 0;
	}

	// �Z�N�^������v���Ă��邱�Ƃ��K�v
	if (num != sc) {
		return FDD_NOTWRITE;
	}

	// �P�{���݂͕s��
	if (GetAllSectors() != num) {
		return FDD_NOTWRITE;
	}

	// ���Ԃ�ݒ�(index�܂�)
	pos = GetDisk()->GetRotationTime();
	pos -= GetDisk()->GetRotationPos();
	GetDisk()->SetSearch(pos);

	// buf���^�����Ă��Ȃ���΂����܂�
	if (!buf) {
		return FDD_NOERROR;
	}

	// CHRN���d���Ȃ��A��v���Ă��邱��
	sector = GetFirst();
	while (sector) {
		// buf�̒����璲�ׂ�
		for (i=0; i<sc; i++) {
			chrn[0] = (DWORD)buf[i * 4 + 0];
			chrn[1] = (DWORD)buf[i * 4 + 1];
			chrn[2] = (DWORD)buf[i * 4 + 2];
			chrn[3] = (DWORD)buf[i * 4 + 3];
			if (sector->IsMatch(mfm, chrn)) {
				break;
			}
		}

		// ��v������̂��Ȃ�����
		if (i >= sc) {
			ASSERT(i == sc);
			return FDD_NOTWRITE;
		}

		// ����
		sector = sector->GetNext();
	}

	// �������݃��[�v(�S�Z�N�^�𖄂߂�)
	sector = GetFirst();
	while (sector) {
		sector->Fill(d);
		sector = sector->GetNext();
	}

	return FDD_NOERROR;
}

//---------------------------------------------------------------------------
//
//	�ύX�`�F�b�N
//
//---------------------------------------------------------------------------
BOOL FASTCALL FDITrack::IsChanged() const
{
	BOOL changed;
	FDISector *sector;

	ASSERT(this);

	// ������
	changed = FALSE;
	sector = GetFirst();

	// OR��
	while (sector) {
		if (sector->IsChanged()) {
			changed = TRUE;
		}
		sector = sector->GetNext();
	}

	return changed;
}

//---------------------------------------------------------------------------
//
//	�Z�N�^�����O�X�݌v�Z�o
//
//---------------------------------------------------------------------------
DWORD FASTCALL FDITrack::GetTotalLength() const
{
	DWORD total;
	FDISector *sector;

	ASSERT(this);

	// ������
	total = 0;
	sector = GetFirst();

	// ���[�v
	while (sector) {
		total += sector->GetLength();
		sector = sector->GetNext();
	}

	return total;
}

//---------------------------------------------------------------------------
//
//	�����ύX
//
//---------------------------------------------------------------------------
void FASTCALL FDITrack::ForceChanged()
{
	FDISector *sector;

	ASSERT(this);

	// ������
	sector = GetFirst();

	// ���[�v
	while (sector) {
		sector->ForceChanged();
		sector = sector->GetNext();
	}
}

//---------------------------------------------------------------------------
//
//	�Z�N�^�ǉ�
//
//---------------------------------------------------------------------------
void FASTCALL FDITrack::AddSector(FDISector *sector)
{
	FDISector *ptr;

	ASSERT(this);
	ASSERT(sector);

	// �Z�N�^�������Ă��Ȃ���΁A���̂܂ܒǉ�
	if (!trk.first) {
		trk.first = sector;
		sector->SetNext(NULL);

		// MFM����
		trk.mfm = sector->IsMFM();
	}
	else {
		// �ŏI�Z�N�^�𓾂�
		ptr = trk.first;
			while (ptr->GetNext()) {
			ptr = ptr->GetNext();
		}

		// �ŏI�Z�N�^�ɒǉ�
		ptr->SetNext(sector);
		sector->SetNext(NULL);
	}

	// �����Z
	trk.sectors[0]++;
	if (sector->IsMFM()) {
		trk.sectors[1]++;
	}
	else {
		trk.sectors[2]++;
	}
}

//---------------------------------------------------------------------------
//
//	�Z�N�^�S�폜
//
//---------------------------------------------------------------------------
void FASTCALL FDITrack::ClrSector()
{
	FDISector *sector;

	ASSERT(this);

	// �Z�N�^�����ׂč폜
	while (trk.first) {
		sector = trk.first->GetNext();
		delete trk.first;
		trk.first = sector;
	}

	// ��0
	trk.sectors[0] = 0;
	trk.sectors[1] = 0;
	trk.sectors[2] = 0;
}

//---------------------------------------------------------------------------
//
//	�Z�N�^����
//
//---------------------------------------------------------------------------
FDISector* FASTCALL FDITrack::Search(BOOL mfm, const DWORD *chrn)
{
	FDISector *sector;

	ASSERT(this);
	ASSERT(chrn);

	// �ŏ��̃Z�N�^���擾
	sector = GetFirst();

	// ���[�v
	while (sector) {
		// �}�b�`����΂��̃Z�N�^��Ԃ�
		if (sector->IsMatch(mfm, chrn)) {
			return sector;
		}

		sector = sector->GetNext();
	}

	// �}�b�`���Ȃ�
	return NULL;
}

//---------------------------------------------------------------------------
//
//	GAP1�̒������擾
//
//---------------------------------------------------------------------------
int FASTCALL FDITrack::GetGAP1() const
{
	ASSERT(this);

	if (IsMFM()) {
		// GAP4a 80�o�C�g�ASYNC12�o�C�g�AIAM4�o�C�g�AGAP1 50�o�C�g
		return 146;
	}
	else {
		// GAP4a 40�o�C�g�ASYNC6�o�C�g�AIAM1�o�C�g�AGAP1 26�o�C�g
		return 73;
	}
}

//---------------------------------------------------------------------------
//
//	�g�[�^���̒������擾
//
//---------------------------------------------------------------------------
int FASTCALL FDITrack::GetTotal() const
{
	ASSERT(this);

	// 2DD�͕ʈ���
	if (!trk.hd) {
		// PC-9801BX4�ł̎����l
		if (IsMFM()) {
			return 6034 + GetGAP1() + 60;
		}
		else {
			return 3016 + GetGAP1() + 31;
		}
	}

	// XVI�ł̎����l
	if (IsMFM()) {
		return 10193 + GetGAP1() + 60;
	}
	else {
		return 5095 + GetGAP1() + 31;
	}
}

//---------------------------------------------------------------------------
//
//	�e�Z�N�^�擪�̈ʒu���Z�o
//
//---------------------------------------------------------------------------
void FASTCALL FDITrack::CalcPos()
{
	FDISector *sector;
	DWORD total;
	DWORD prev;
	DWORD hus;
	FDISector *p;

	ASSERT(this);

	// �ŏ��̃Z�N�^���Z�b�g
	sector = GetFirst();

	// ���[�v
	while (sector) {
		// GAP1
		prev = GetGAP1();

		// �S�ẴZ�N�^���܂���āA�T�C�Y�𓾂�
		p = GetFirst();
		while (p) {
			if (p == sector) {
				break;
			}

			// �܂�����Ă��Ȃ���΁Aprev�����Z
			prev += GetSize(p);
			p = p->GetNext();
		}

		// ID�t�B�[���h��SYNC�̕�����������
		if (sector->IsMFM()) {
			prev += 60;
		}
		else {
			prev += 31;
		}

		// GAP4���������g�[�^���̒l�𓾂�
		total = GetTotal();

		// prev��total�̔���Z�o�B�P����GetDisk()->GetRotationTime()�ɂȂ�悤��
		if (prev >= total) {
			prev = total;
		}
		ASSERT(total <= 0x5000);
		hus = GetDisk()->GetRotationTime();
		prev >>= 1;
		total >>= 1;
		prev *= hus;
		prev /= total;
		if (prev >= hus) {
			prev = hus - 1;
		}

		// �i�[
		sector->SetPos(prev);

		// ����
		sector = sector->GetNext();
	}
}

//---------------------------------------------------------------------------
//
//	�Z�N�^�̃T�C�Y�𓾂�
//
//---------------------------------------------------------------------------
DWORD FASTCALL FDITrack::GetSize(FDISector *sector) const
{
	DWORD len;

	ASSERT(this);
	ASSERT(sector);

	// �܂��Z�N�^�̎��f�[�^�̈�+CRC+GAP3
	len = sector->GetLength();
	len += 2;
	len += sector->GetGAP3();

	if (sector->IsMFM()) {
		// SYNC12�o�C�g
		len += 12;

		// ID�A�h���X�}�[�N�ACHRN�ACRC2�o�C�g
		len += 10;

		// GAP2�ASYNC�A�f�[�^�}�[�N
		len += 38;
	}
	else {
		// SYNC6�o�C�g
		len += 6;

		// ID�A�h���X�}�[�N�ACHRN�ACRC2�o�C�g
		len += 7;

		// GAP2�ASYNC�A�f�[�^�}�[�N
		len += 18;
	}

	return len;
}

//---------------------------------------------------------------------------
//
//	�J�����g�|�W�V�����ȍ~�̃Z�N�^���擾
//
//---------------------------------------------------------------------------
FDISector* FASTCALL FDITrack::GetCurSector() const
{
	DWORD cur;
	DWORD pos;
	FDISector *sector;

	ASSERT(this);

	// �J�����g�|�W�V�����𓾂�
	cur = GetDisk()->GetRotationPos();

	// �擪�Z�N�^�𓾂�
	sector = GetFirst();
	if (!sector) {
		return NULL;
	}

	// �Z�N�^�𓪂��猩�āA�ȏ�ł����ok
	while (sector) {
		pos = sector->GetPos();
		if (pos >= cur) {
			return sector;
		}
		sector = sector->GetNext();
	}

	// �ŏI�Z�N�^�̈ʒu�𒴂���Ƃ��낪�w�肳��Ă���̂ŁA�擪�ɖ߂�
	return GetFirst();
}

//---------------------------------------------------------------------------
//
//	GAP1�쐬
//
//---------------------------------------------------------------------------
int FASTCALL FDITrack::MakeGAP1(BYTE *buf, int offset) const
{
	ASSERT(this);
	ASSERT(buf);
	ASSERT(offset >= 0);

	// MFM��
	if (IsMFM()) {
		ASSERT(GetMFMSectors() > 0);

		// GAP1�쐬
		offset = MakeData(buf, offset, 0x4e, 80);
		offset = MakeData(buf, offset, 0x00, 12);
		offset = MakeData(buf, offset, 0xc2, 3);
		offset = MakeData(buf, offset, 0xfc, 1);
		offset = MakeData(buf, offset, 0x4e, 50);
		return offset;
	}

	// FM
	ASSERT(GetFMSectors() > 0);

	// GAP1�쐬
	offset = MakeData(buf, offset, 0xff, 40);
	offset = MakeData(buf, offset, 0x00, 6);
	offset = MakeData(buf, offset, 0xfc, 1);
	offset = MakeData(buf, offset, 0xff, 26);
	return offset;
}

//---------------------------------------------------------------------------
//
//	�Z�N�^�f�[�^�쐬
//
//---------------------------------------------------------------------------
int FASTCALL FDITrack::MakeSector(BYTE *buf, int offset, FDISector *sector) const
{
	DWORD chrn[4];
	int i;
	WORD crc;
	const BYTE *ptr;
	int len;

	ASSERT(this);
	ASSERT(buf);
	ASSERT(offset >= 0);
	ASSERT(sector);

	// CHRN�A�Z�N�^�f�[�^�A�����O�X�擾
	sector->GetCHRN(chrn);
	ptr = sector->GetSector();
	len = sector->GetLength();

	// MFM��
	if (sector->IsMFM()) {
		// MFM(ID��)
		offset = MakeData(buf, offset, 0x00, 12);
		offset = MakeData(buf, offset, 0xa1, 3);
		offset = MakeData(buf, offset, 0xfe, 1);
		for (i=0; i<4; i++) {
			buf[offset + i] = (BYTE)chrn[i];
		}
		offset += 4;
		crc = CalcCRC(&buf[offset - 8], 8);
		buf[offset + 0] = (BYTE)(crc >> 8);
		buf[offset + 1] = (BYTE)crc;
		offset += 2;
		offset = MakeData(buf, offset, 0x4e, 22);

		// MFM(�f�[�^��)
		offset = MakeData(buf, offset, 0x00, 12);
		offset = MakeData(buf, offset, 0xa1, 3);
		if (sector->GetError() & FDD_DDAM) {
			offset = MakeData(buf, offset, 0xf8, 1);
		}
		else {
			offset = MakeData(buf, offset, 0xfb, 1);
		}
		memcpy(&buf[offset], ptr, len);
		crc = CalcCRC(&buf[offset - 4], len + 4);
		offset += len;
		buf[offset + 0] = (BYTE)(crc >> 8);
		buf[offset + 1] = (BYTE)crc;
		offset += 2;
		offset = MakeData(buf, offset, 0x4e, sector->GetGAP3());
		return offset;
	}

	// FM(ID��)
	offset = MakeData(buf, offset, 0x00, 6);
	offset = MakeData(buf, offset, 0xfe, 1);
	for (i=0; i<4; i++) {
		buf[offset + i] = (BYTE)chrn[i];
	}
	offset += 4;
	crc = CalcCRC(&buf[offset - 5], 5);
	buf[offset + 0] = (BYTE)(crc >> 8);
	buf[offset + 1] = (BYTE)crc;
	offset += 2;
	offset = MakeData(buf, offset, 0xff, 11);

	// FM(�f�[�^��)
	offset = MakeData(buf, offset, 0x00, 6);
	if (sector->GetError() & FDD_DDAM) {
		offset = MakeData(buf, offset, 0xf8, 1);
	}
	else {
		offset = MakeData(buf, offset, 0xfb, 1);
	}
	memcpy(&buf[offset], ptr, len);
	crc = CalcCRC(&buf[offset - 1], len + 1);
	offset += len;
	buf[offset + 0] = (BYTE)(crc >> 8);
	buf[offset + 1] = (BYTE)crc;
	offset += 2;
	offset = MakeData(buf, offset, 0xff, sector->GetGAP3());

	return offset;
}

//---------------------------------------------------------------------------
//
//	GAP4�쐬
//
//---------------------------------------------------------------------------
int FASTCALL FDITrack::MakeGAP4(BYTE *buf, int offset) const
{
	ASSERT(this);
	ASSERT(buf);
	ASSERT(offset >= 0);

	if (IsMFM()) {
		return MakeData(buf, offset, 0x4e, GetTotal() - offset);
	}
	else {
		return MakeData(buf, offset, 0xff, GetTotal() - offset);
	}
}

//---------------------------------------------------------------------------
//
//	CRC�Z�o
//
//---------------------------------------------------------------------------
WORD FASTCALL FDITrack::CalcCRC(BYTE *ptr, int len) const
{
	WORD crc;
	int i;

	ASSERT(this);
	ASSERT(ptr);
	ASSERT(len >= 0);

	// ������
	crc = 0xffff;

	// ���[�v
	for (i=0; i<len; i++) {
		crc = (WORD)((crc << 8) ^ CRCTable[ (BYTE)(crc >> 8) ^ (BYTE)*ptr++ ]);
	}

	return crc;
}

//---------------------------------------------------------------------------
//
//	CRC�Z�o�e�[�u��
//
//---------------------------------------------------------------------------
const WORD FDITrack::CRCTable[0x100] = {
	0x0000,  0x1021,  0x2042,  0x3063,  0x4084,  0x50a5,  0x60c6,  0x70e7,
	0x8108,  0x9129,  0xa14a,  0xb16b,  0xc18c,  0xd1ad,  0xe1ce,  0xf1ef,
	0x1231,  0x0210,  0x3273,  0x2252,  0x52b5,  0x4294,  0x72f7,  0x62d6,
	0x9339,  0x8318,  0xb37b,  0xa35a,  0xd3bd,  0xc39c,  0xf3ff,  0xe3de,
	0x2462,  0x3443,  0x0420,  0x1401,  0x64e6,  0x74c7,  0x44a4,  0x5485,
	0xa56a,  0xb54b,  0x8528,  0x9509,  0xe5ee,  0xf5cf,  0xc5ac,  0xd58d,
	0x3653,  0x2672,  0x1611,  0x0630,  0x76d7,  0x66f6,  0x5695,  0x46b4,
	0xb75b,  0xa77a,  0x9719,  0x8738,  0xf7df,  0xe7fe,  0xd79d,  0xc7bc,
	0x48c4,  0x58e5,  0x6886,  0x78a7,  0x0840,  0x1861,  0x2802,  0x3823,
	0xc9cc,  0xd9ed,  0xe98e,  0xf9af,  0x8948,  0x9969,  0xa90a,  0xb92b,
	0x5af5,  0x4ad4,  0x7ab7,  0x6a96,  0x1a71,  0x0a50,  0x3a33,  0x2a12,
	0xdbfd,  0xcbdc,  0xfbbf,  0xeb9e,  0x9b79,  0x8b58,  0xbb3b,  0xab1a,
	0x6ca6,  0x7c87,  0x4ce4,  0x5cc5,  0x2c22,  0x3c03,  0x0c60,  0x1c41,
	0xedae,  0xfd8f,  0xcdec,  0xddcd,  0xad2a,  0xbd0b,  0x8d68,  0x9d49,
	0x7e97,  0x6eb6,  0x5ed5,  0x4ef4,  0x3e13,  0x2e32,  0x1e51,  0x0e70,
	0xff9f,  0xefbe,  0xdfdd,  0xcffc,  0xbf1b,  0xaf3a,  0x9f59,  0x8f78,
	0x9188,  0x81a9,  0xb1ca,  0xa1eb,  0xd10c,  0xc12d,  0xf14e,  0xe16f,
	0x1080,  0x00a1,  0x30c2,  0x20e3,  0x5004,  0x4025,  0x7046,  0x6067,
	0x83b9,  0x9398,  0xa3fb,  0xb3da,  0xc33d,  0xd31c,  0xe37f,  0xf35e,
	0x02b1,  0x1290,  0x22f3,  0x32d2,  0x4235,  0x5214,  0x6277,  0x7256,
	0xb5ea,  0xa5cb,  0x95a8,  0x8589,  0xf56e,  0xe54f,  0xd52c,  0xc50d,
	0x34e2,  0x24c3,  0x14a0,  0x0481,  0x7466,  0x6447,  0x5424,  0x4405,
	0xa7db,  0xb7fa,  0x8799,  0x97b8,  0xe75f,  0xf77e,  0xc71d,  0xd73c,
	0x26d3,  0x36f2,  0x0691,  0x16b0,  0x6657,  0x7676,  0x4615,  0x5634,
	0xd94c,  0xc96d,  0xf90e,  0xe92f,  0x99c8,  0x89e9,  0xb98a,  0xa9ab,
	0x5844,  0x4865,  0x7806,  0x6827,  0x18c0,  0x08e1,  0x3882,  0x28a3,
	0xcb7d,  0xdb5c,  0xeb3f,  0xfb1e,  0x8bf9,  0x9bd8,  0xabbb,  0xbb9a,
	0x4a75,  0x5a54,  0x6a37,  0x7a16,  0x0af1,  0x1ad0,  0x2ab3,  0x3a92,
	0xfd2e,  0xed0f,  0xdd6c,  0xcd4d,  0xbdaa,  0xad8b,  0x9de8,  0x8dc9,
	0x7c26,  0x6c07,  0x5c64,  0x4c45,  0x3ca2,  0x2c83,  0x1ce0,  0x0cc1,
	0xef1f,  0xff3e,  0xcf5d,  0xdf7c,  0xaf9b,  0xbfba,  0x8fd9,  0x9ff8,
	0x6e17,  0x7e36,  0x4e55,  0x5e74,  0x2e93,  0x3eb2,  0x0ed1,  0x1ef0
};

//---------------------------------------------------------------------------
//
//	Diag�f�[�^�쐬
//
//---------------------------------------------------------------------------
int FASTCALL FDITrack::MakeData(BYTE *buf, int offset, BYTE data, int length) const
{
	int i;

	ASSERT(this);
	ASSERT(buf);
	ASSERT(offset >= 0);
	ASSERT((length > 0) && (length < 0x400));

	for (i=0; i<length; i++) {
		buf[offset + i] = data;
	}

	return (offset + length);
}

//===========================================================================
//
//	FDI�f�B�X�N
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	�R���X�g���N�^
//
//---------------------------------------------------------------------------
FDIDisk::FDIDisk(int index, FDI *fdi)
{
	ASSERT((index >= 0) && (index < 0x10));

	// �C���f�b�N�X�AID�w��
	disk.index = index;
	disk.fdi = fdi;
	disk.id = MAKEID('I', 'N', 'I', 'T');

	// ���
	disk.writep = FALSE;
	disk.readonly = FALSE;

	// ���̂Ȃ�
	disk.name[0] = '\0';
	disk.offset = 0;

	// �ێ��g���b�N�Ȃ�
	disk.first = NULL;
	disk.head[0] = NULL;
	disk.head[1] = NULL;

	// �|�W�V����
	disk.search = 0;

	// �����N�Ȃ�
	disk.next = NULL;
}

//---------------------------------------------------------------------------
//
//	�f�X�g���N�^
//	���h���N���X�̒��ӓ_�F
//		���݂�head[]���t�@�C���ɏ����߂�
//
//---------------------------------------------------------------------------
FDIDisk::~FDIDisk()
{
	// �N���A
	ClrTrack();
}

//---------------------------------------------------------------------------
//
//	�쐬
//	���h���N���X�̒��ӓ_�F
//		�����t�H�[�}�b�g���s��(���z�֐��̍Ō�ŁA�������ĂԂ���)
//
//---------------------------------------------------------------------------
BOOL FASTCALL FDIDisk::Create(const Filepath& /*path*/, const option_t *opt)
{
	ASSERT(this);
	ASSERT(opt);

	// �_���t�H�[�}�b�g���K�v�Ȃ���ΏI��
	if (!opt->logfmt) {
		return TRUE;
	}

	// �����t�H�[�}�b�g�ʂɁA�_���t�H�[�}�b�g���s��
	switch (opt->phyfmt) {
		// 2HD
		case FDI_2HD:
			Create2HD(TRUE);
			break;

		// 2HDA
		case FDI_2HDA:
			Create2HD(FALSE);
			break;

		// 2HS
		case FDI_2HS:
			Create2HS();
			break;

		// 2HC
		case FDI_2HC:
			Create2HC();
			break;

		// 2HDE
		case FDI_2HDE:
			Create2HDE();
			break;

		// 2HQ
		case FDI_2HQ:
			Create2HQ();
			break;

		// 2DD
		case FDI_2DD:
			Create2DD();
			break;

		// ���̑�
		default:
			return FALSE;
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	�_���t�H�[�}�b�g(2HD,2HDA)
//
//---------------------------------------------------------------------------
void FASTCALL FDIDisk::Create2HD(BOOL flag2hd)
{
	FDITrack *track;
	FDISector *sector;
	BYTE buf[0x400];
	DWORD chrn[4];
	int i;

	ASSERT(this);

	// �ʏ평����
	memset(buf, 0, sizeof(buf));

	// �g���b�N0�Z�N�^1�`8�ւ��ׂď�������
	track = Search(0);
	ASSERT(track);
	chrn[0] = 0;
	chrn[1] = 0;
	chrn[3] = 3;
	for (i=1; i<=8; i++) {
		chrn[2] = i;
		sector = track->Search(TRUE, chrn);
		ASSERT(sector);
		sector->Write(buf, FALSE);
	}

	// �g���b�N1�Z�N�^1�`3�ւ��ׂď�������
	track = Search(1);
	ASSERT(track);
	chrn[0] = 0;
	chrn[1] = 1;
	chrn[3] = 3;
	for (i=1; i<=3; i++) {
		chrn[2] = i;
		sector = track->Search(TRUE, chrn);
		ASSERT(sector);
		sector->Write(buf, FALSE);
	}

	// �g���b�N0�փV�[�N
	track = Search(0);
	ASSERT(track);

	// IPL��������
	memcpy(buf, IPL2HD, 0x200);
	if (!flag2hd) {
		// 2HDA�͘_���Z�N�^��=1280�Z�N�^
		buf[0x13] = 0x00;
		buf[0x14] = 0x05;
	}
	chrn[0] = 0;
	chrn[1] = 0;
	chrn[2] = 1;
	chrn[3] = 3;
	sector = track->Search(TRUE, chrn);
	ASSERT(sector);
	sector->Write(buf, FALSE);

	// FAT�擪�Z�N�^������
	memset(buf, 0, sizeof(buf));
	buf[0] = 0xfe;
	buf[1] = 0xff;
	buf[2] = 0xff;

	// ��1FAT��������
	chrn[0] = 0;
	chrn[1] = 0;
	chrn[2] = 2;
	chrn[3] = 3;
	sector = track->Search(TRUE, chrn);
	ASSERT(sector);
	sector->Write(buf, FALSE);

	// ��2FAT��������
	chrn[0] = 0;
	chrn[1] = 0;
	chrn[2] = 4;
	chrn[3] = 3;
	sector = track->Search(TRUE, chrn);
	ASSERT(sector);
	sector->Write(buf, FALSE);
}

//---------------------------------------------------------------------------
//
//	IPL(2HD,2HDA)
//	��FORMAT.x v2.31���擾��������
//
//---------------------------------------------------------------------------
const BYTE FDIDisk::IPL2HD[0x200] = {
	0x60,0x3c,0x90,0x58,0x36,0x38,0x49,0x50,
	0x4c,0x33,0x30,0x00,0x04,0x01,0x01,0x00,
	0x02,0xc0,0x00,0xd0,0x04,0xfe,0x02,0x00,
	0x08,0x00,0x02,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x20,0x20,0x20,0x20,0x20,
	0x20,0x20,0x20,0x20,0x20,0x20,0x46,0x41,
	0x54,0x31,0x32,0x20,0x20,0x20,0x4f,0xfa,
	0xff,0xc0,0x4d,0xfa,0x01,0xb8,0x4b,0xfa,
	0x00,0xe0,0x49,0xfa,0x00,0xea,0x43,0xfa,
	0x01,0x20,0x4e,0x94,0x70,0x8e,0x4e,0x4f,
	0x7e,0x70,0xe1,0x48,0x8e,0x40,0x26,0x3a,
	0x01,0x02,0x22,0x4e,0x24,0x3a,0x01,0x00,
	0x32,0x07,0x4e,0x95,0x66,0x28,0x22,0x4e,
	0x32,0x3a,0x00,0xfa,0x20,0x49,0x45,0xfa,
	0x01,0x78,0x70,0x0a,0x00,0x10,0x00,0x20,
	0xb1,0x0a,0x56,0xc8,0xff,0xf8,0x67,0x38,
	0xd2,0xfc,0x00,0x20,0x51,0xc9,0xff,0xe6,
	0x45,0xfa,0x00,0xe0,0x60,0x10,0x45,0xfa,
	0x00,0xfa,0x60,0x0a,0x45,0xfa,0x01,0x10,
	0x60,0x04,0x45,0xfa,0x01,0x28,0x61,0x00,
	0x00,0x94,0x22,0x4a,0x4c,0x99,0x00,0x06,
	0x70,0x23,0x4e,0x4f,0x4e,0x94,0x32,0x07,
	0x70,0x4f,0x4e,0x4f,0x70,0xfe,0x4e,0x4f,
	0x74,0x00,0x34,0x29,0x00,0x1a,0xe1,0x5a,
	0xd4,0x7a,0x00,0xa4,0x84,0xfa,0x00,0x9c,
	0x84,0x7a,0x00,0x94,0xe2,0x0a,0x64,0x04,
	0x08,0xc2,0x00,0x18,0x48,0x42,0x52,0x02,
	0x22,0x4e,0x26,0x3a,0x00,0x7e,0x32,0x07,
	0x4e,0x95,0x34,0x7c,0x68,0x00,0x22,0x4e,
	0x0c,0x59,0x48,0x55,0x66,0xa6,0x54,0x89,
	0xb5,0xd9,0x66,0xa6,0x2f,0x19,0x20,0x59,
	0xd1,0xd9,0x2f,0x08,0x2f,0x11,0x32,0x7c,
	0x67,0xc0,0x76,0x40,0xd6,0x88,0x4e,0x95,
	0x22,0x1f,0x24,0x1f,0x22,0x5f,0x4a,0x80,
	0x66,0x00,0xff,0x7c,0xd5,0xc2,0x53,0x81,
	0x65,0x04,0x42,0x1a,0x60,0xf8,0x4e,0xd1,
	0x70,0x46,0x4e,0x4f,0x08,0x00,0x00,0x1e,
	0x66,0x02,0x70,0x00,0x4e,0x75,0x70,0x21,
	0x4e,0x4f,0x4e,0x75,0x72,0x0f,0x70,0x22,
	0x4e,0x4f,0x72,0x19,0x74,0x0c,0x70,0x23,
	0x4e,0x4f,0x61,0x08,0x72,0x19,0x74,0x0d,
	0x70,0x23,0x4e,0x4f,0x76,0x2c,0x72,0x20,
	0x70,0x20,0x4e,0x4f,0x51,0xcb,0xff,0xf8,
	0x4e,0x75,0x00,0x00,0x04,0x00,0x03,0x00,
	0x00,0x06,0x00,0x08,0x00,0x1f,0x00,0x09,
	0x1a,0x00,0x00,0x22,0x00,0x0d,0x48,0x75,
	0x6d,0x61,0x6e,0x2e,0x73,0x79,0x73,0x20,
	0x82,0xaa,0x20,0x8c,0xa9,0x82,0xc2,0x82,
	0xa9,0x82,0xe8,0x82,0xdc,0x82,0xb9,0x82,
	0xf1,0x00,0x00,0x25,0x00,0x0d,0x83,0x66,
	0x83,0x42,0x83,0x58,0x83,0x4e,0x82,0xaa,
	0x81,0x40,0x93,0xc7,0x82,0xdf,0x82,0xdc,
	0x82,0xb9,0x82,0xf1,0x00,0x00,0x00,0x23,
	0x00,0x0d,0x48,0x75,0x6d,0x61,0x6e,0x2e,
	0x73,0x79,0x73,0x20,0x82,0xaa,0x20,0x89,
	0xf3,0x82,0xea,0x82,0xc4,0x82,0xa2,0x82,
	0xdc,0x82,0xb7,0x00,0x00,0x20,0x00,0x0d,
	0x48,0x75,0x6d,0x61,0x6e,0x2e,0x73,0x79,
	0x73,0x20,0x82,0xcc,0x20,0x83,0x41,0x83,
	0x68,0x83,0x8c,0x83,0x58,0x82,0xaa,0x88,
	0xd9,0x8f,0xed,0x82,0xc5,0x82,0xb7,0x00,
	0x68,0x75,0x6d,0x61,0x6e,0x20,0x20,0x20,
	0x73,0x79,0x73,0x00,0x00,0x00,0x00,0x00
};

//---------------------------------------------------------------------------
//
//	�_���t�H�[�}�b�g(2HS)
//
//---------------------------------------------------------------------------
void FASTCALL FDIDisk::Create2HS()
{
	FDITrack *track;
	FDISector *sector;
	BYTE buf[0x400];
	DWORD chrn[4];
	int i;

	ASSERT(this);

	// �ʏ평����
	memset(buf, 0, sizeof(buf));

	// ���v10�Z�N�^�֏�������(1�g���b�N������9�Z�N�^)
	track = Search(0);
	ASSERT(track);
	chrn[0] = 0;
	chrn[1] = 0;
	chrn[3] = 3;
	for (i=11; i<=18; i++) {
		chrn[2] = i;
		sector = track->Search(TRUE, chrn);
		ASSERT(sector);
		sector->Write(buf, FALSE);
	}
	track = Search(1);
	ASSERT(track);
	chrn[0] = 0;
	chrn[1] = 1;
	chrn[2] = 10;
	chrn[3] = 3;
	sector = track->Search(TRUE, chrn);
	ASSERT(sector);
	sector->Write(buf, FALSE);

	// �g���b�N0�փV�[�N
	track = Search(0);
	ASSERT(track);

	// IPL��������(1)
	chrn[0] = 0;
	chrn[1] = 0;
	chrn[2] = 1;
	chrn[3] = 3;
	sector = track->Search(TRUE, chrn);
	ASSERT(sector);
	sector->Write(&IPL2HS[0x000], FALSE);

	// IPL��������(2)
	chrn[0] = 0;
	chrn[1] = 0;
	chrn[2] = 13;
	chrn[3] = 3;
	sector = track->Search(TRUE, chrn);
	ASSERT(sector);
	sector->Write(&IPL2HS[0x400], FALSE);

	// FAT�擪�Z�N�^������
	buf[0] = 0xfb;
	buf[1] = 0xff;
	buf[2] = 0xff;

	// FAT��������
	chrn[0] = 0;
	chrn[1] = 0;
	chrn[2] = 11;
	chrn[3] = 3;
	sector = track->Search(TRUE, chrn);
	ASSERT(sector);
	sector->Write(buf, FALSE);
}

//---------------------------------------------------------------------------
//
//	IPL(2HS)
//	��9scdrv.x v3.00���擾��������
//
//---------------------------------------------------------------------------
const BYTE FDIDisk::IPL2HS[0x800] = {
	0x60,0x1e,0x39,0x53,0x43,0x46,0x4d,0x54,
	0x20,0x49,0x50,0x4c,0x20,0x76,0x31,0x2e,
	0x30,0x32,0x04,0x00,0x01,0x03,0x00,0x01,
	0x00,0xc0,0x05,0xa0,0xfb,0x01,0x90,0x70,
	0x60,0x00,0x03,0x5a,0x08,0x01,0x00,0x0c,
	0x66,0x08,0x4d,0xfa,0xff,0xd4,0x2c,0x56,
	0x4e,0xd6,0x61,0x00,0x00,0xba,0x48,0xe7,
	0x4f,0x00,0x61,0x00,0x02,0xf0,0x61,0x00,
	0x00,0xc4,0x08,0x00,0x00,0x1b,0x66,0x4e,
	0xc2,0x3c,0x00,0xc0,0x82,0x3c,0x00,0x06,
	0x61,0x00,0x00,0xd0,0xe1,0x9a,0x54,0x88,
	0x20,0xc2,0xe0,0x9a,0x10,0xc2,0x10,0xc7,
	0x10,0x86,0x61,0x00,0x00,0xf0,0x41,0xf8,
	0x09,0xee,0x70,0x08,0x61,0x00,0x01,0x0c,
	0x61,0x00,0x01,0x42,0x61,0x00,0x01,0x60,
	0x61,0x00,0x01,0x7a,0x08,0x00,0x00,0x0e,
	0x66,0x0c,0x08,0x00,0x00,0x1e,0x67,0x26,
	0x08,0x00,0x00,0x1b,0x66,0x08,0x61,0x00,
	0x01,0x7a,0x51,0xcc,0xff,0xbc,0x4c,0xdf,
	0x00,0xf2,0x4a,0x38,0x09,0xe1,0x67,0x0c,
	0x31,0xf8,0x09,0xc2,0x09,0xc4,0x11,0xfc,
	0x00,0x40,0x09,0xe1,0x4e,0x75,0x08,0x00,
	0x00,0x1f,0x66,0xe2,0xd3,0xc5,0x96,0x85,
	0x63,0xdc,0x20,0x04,0x48,0x40,0x38,0x00,
	0x30,0x3c,0x00,0x12,0x52,0x02,0xb0,0x02,
	0x64,0x86,0x14,0x3c,0x00,0x0a,0x0a,0x42,
	0x01,0x00,0x08,0x02,0x00,0x08,0x66,0x00,
	0xff,0x78,0xd4,0xbc,0x00,0x01,0x00,0x00,
	0x61,0x00,0x01,0xb8,0x08,0x00,0x00,0x1b,
	0x66,0xac,0x60,0x00,0xff,0x64,0x08,0x38,
	0x00,0x07,0x09,0xe1,0x66,0x0c,0x48,0xe7,
	0xc0,0x00,0x61,0x00,0x01,0x46,0x4c,0xdf,
	0x00,0x03,0x4e,0x75,0x70,0x00,0x78,0x00,
	0x08,0x01,0x00,0x05,0x67,0x08,0x78,0x09,
	0x48,0x44,0x38,0x3c,0x00,0x09,0x08,0x01,
	0x00,0x04,0x67,0x04,0x61,0x00,0x01,0x7c,
	0x4e,0x75,0x2f,0x01,0x41,0xf8,0x09,0xee,
	0x10,0x81,0xe0,0x99,0xc2,0x3c,0x00,0x03,
	0x08,0x02,0x00,0x08,0x67,0x04,0x08,0xc1,
	0x00,0x02,0x11,0x41,0x00,0x01,0x22,0x1f,
	0x4e,0x75,0x13,0xfc,0x00,0xff,0x00,0xe8,
	0x40,0x00,0x13,0xfc,0x00,0x32,0x00,0xe8,
	0x40,0x05,0x60,0x10,0x13,0xfc,0x00,0xff,
	0x00,0xe8,0x40,0x00,0x13,0xfc,0x00,0xb2,
	0x00,0xe8,0x40,0x05,0x23,0xc9,0x00,0xe8,
	0x40,0x0c,0x33,0xc5,0x00,0xe8,0x40,0x0a,
	0x13,0xfc,0x00,0x80,0x00,0xe8,0x40,0x07,
	0x4e,0x75,0x48,0xe7,0x40,0x60,0x43,0xf9,
	0x00,0xe9,0x40,0x01,0x45,0xf9,0x00,0xe9,
	0x40,0x03,0x40,0xe7,0x00,0x7c,0x07,0x00,
	0x12,0x11,0x08,0x01,0x00,0x04,0x66,0xf8,
	0x12,0x11,0x08,0x01,0x00,0x07,0x67,0xf8,
	0x08,0x01,0x00,0x06,0x66,0xf2,0x14,0x98,
	0x51,0xc8,0xff,0xee,0x46,0xdf,0x4c,0xdf,
	0x06,0x02,0x4e,0x75,0x10,0x39,0x00,0xe8,
	0x40,0x00,0x08,0x00,0x00,0x04,0x66,0x0e,
	0x10,0x39,0x00,0xe9,0x40,0x01,0xc0,0x3c,
	0x00,0x1f,0x66,0xf4,0x4e,0x75,0x10,0x39,
	0x00,0xe8,0x40,0x01,0x4e,0x75,0x10,0x39,
	0x00,0xe8,0x40,0x00,0x08,0x00,0x00,0x07,
	0x66,0x08,0x13,0xfc,0x00,0x10,0x00,0xe8,
	0x40,0x07,0x13,0xfc,0x00,0xff,0x00,0xe8,
	0x40,0x00,0x4e,0x75,0x30,0x01,0xe0,0x48,
	0xc0,0xbc,0x00,0x00,0x00,0x03,0xe7,0x40,
	0x41,0xf8,0x0c,0x90,0xd1,0xc0,0x20,0x10,
	0x4e,0x75,0x2f,0x00,0xc0,0xbc,0x00,0x35,
	0xff,0x00,0x67,0x2a,0xb8,0x3c,0x00,0x05,
	0x64,0x24,0x2f,0x38,0x09,0xee,0x2f,0x38,
	0x09,0xf2,0x3f,0x38,0x09,0xf6,0x61,0x00,
	0x00,0xc4,0x70,0x64,0x51,0xc8,0xff,0xfe,
	0x61,0x68,0x31,0xdf,0x09,0xf6,0x21,0xdf,
	0x09,0xf2,0x21,0xdf,0x09,0xee,0x20,0x1f,
	0x4e,0x75,0x30,0x01,0xe0,0x48,0x4a,0x00,
	0x67,0x3c,0xc0,0x3c,0x00,0x03,0x80,0x3c,
	0x00,0x80,0x08,0xf8,0x00,0x07,0x09,0xe1,
	0x13,0xc0,0x00,0xe9,0x40,0x07,0x08,0xf8,
	0x00,0x06,0x09,0xe1,0x66,0x18,0x31,0xf8,
	0x09,0xc2,0x09,0xc4,0x61,0x00,0x00,0x90,
	0x08,0x00,0x00,0x1d,0x66,0x08,0x0c,0x78,
	0x00,0x64,0x09,0xc4,0x64,0xee,0x08,0xb8,
	0x00,0x06,0x09,0xe1,0x4e,0x75,0x4a,0x38,
	0x09,0xe1,0x67,0x0c,0x31,0xf8,0x09,0xc2,
	0x09,0xc4,0x11,0xfc,0x00,0x40,0x09,0xe1,
	0x4e,0x75,0x61,0x12,0x08,0x00,0x00,0x1b,
	0x66,0x26,0x48,0x40,0x48,0x42,0xb4,0x00,
	0x67,0x1a,0x48,0x42,0x61,0x3e,0x2f,0x01,
	0x12,0x3c,0x00,0x0f,0x61,0x00,0xfe,0x6c,
	0x48,0x42,0x11,0x42,0x00,0x02,0x48,0x42,
	0x70,0x02,0x60,0x08,0x48,0x42,0x48,0x40,
	0x4e,0x75,0x2f,0x01,0x61,0x00,0xfe,0xac,
	0x61,0x00,0xfe,0xee,0x22,0x1f,0x30,0x01,
	0xe0,0x48,0xc0,0xbc,0x00,0x00,0x00,0x03,
	0xe7,0x40,0x41,0xf8,0x0c,0x90,0xd1,0xc0,
	0x20,0x10,0x4e,0x75,0x2f,0x01,0x12,0x3c,
	0x00,0x07,0x61,0x00,0xfe,0x2e,0x70,0x01,
	0x61,0xd0,0x22,0x1f,0x4e,0x75,0x2f,0x01,
	0x12,0x3c,0x00,0x04,0x61,0x00,0xfe,0x1c,
	0x22,0x1f,0x70,0x01,0x61,0x00,0xfe,0x6c,
	0x10,0x39,0x00,0xe9,0x40,0x01,0xc0,0x3c,
	0x00,0xd0,0xb0,0x3c,0x00,0xd0,0x66,0xf0,
	0x70,0x00,0x10,0x39,0x00,0xe9,0x40,0x03,
	0xe0,0x98,0x4e,0x75,0x53,0x02,0x7e,0x00,
	0x3a,0x02,0xe0,0x5d,0x4a,0x05,0x67,0x04,
	0x06,0x45,0x08,0x00,0xe0,0x4d,0x48,0x42,
	0x02,0x82,0x00,0x00,0x00,0xff,0xe9,0x8a,
	0xd4,0x45,0x0c,0x42,0x00,0x04,0x65,0x02,
	0x53,0x42,0x84,0xfc,0x00,0x12,0x48,0x42,
	0x3e,0x02,0x8e,0xfc,0x00,0x09,0x48,0x47,
	0xe1,0x4f,0xe0,0x8f,0x34,0x07,0x06,0x82,
	0x03,0x00,0x00,0x0a,0x2a,0x3c,0x00,0x00,
	0x04,0x00,0x3c,0x3c,0x00,0xff,0x3e,0x3c,
	0x09,0x28,0x4e,0x75,0x4f,0xfa,0xfc,0x82,
	0x43,0xfa,0xfc,0xa2,0x4d,0xfa,0xfc,0x7a,
	0x2c,0xb9,0x00,0x00,0x05,0x18,0x23,0xc9,
	0x00,0x00,0x05,0x18,0x43,0xfa,0x00,0xda,
	0x4d,0xfa,0xfc,0x6a,0x2c,0xb9,0x00,0x00,
	0x05,0x14,0x23,0xc9,0x00,0x00,0x05,0x14,
	0x43,0xfa,0x01,0x6e,0x4d,0xfa,0xfc,0x5a,
	0x2c,0xb9,0x00,0x00,0x05,0x04,0x23,0xc9,
	0x00,0x00,0x05,0x04,0x24,0x3c,0x03,0x00,
	0x00,0x04,0x20,0x3c,0x00,0x00,0x00,0x8e,
	0x4e,0x4f,0x12,0x00,0xe1,0x41,0x12,0x3c,
	0x00,0x70,0x33,0xc1,0x00,0x00,0x00,0x64,
	0x26,0x3c,0x00,0x00,0x04,0x00,0x43,0xfa,
	0x00,0x20,0x61,0x04,0x60,0x00,0x01,0xf0,
	0x48,0xe7,0x78,0x40,0x70,0x46,0x4e,0x4f,
	0x08,0x00,0x00,0x1e,0x66,0x02,0x70,0x00,
	0x4c,0xdf,0x02,0x1e,0x4e,0x75,0x4e,0x75,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x08,0x01,0x00,0x0c,0x66,0x08,0x4d,0xfa,
	0xfb,0x8c,0x2c,0x56,0x4e,0xd6,0x61,0x00,
	0xfc,0x6e,0x48,0xe7,0x4f,0x00,0x61,0x00,
	0xfe,0xa4,0x61,0x00,0xfc,0x78,0x08,0x00,
	0x00,0x1b,0x66,0x30,0xc2,0x3c,0x00,0xc0,
	0x82,0x3c,0x00,0x05,0x60,0x08,0x30,0x3c,
	0x01,0xac,0x51,0xc8,0xff,0xfe,0x61,0x00,
	0x01,0x00,0x08,0x00,0x00,0x1e,0x67,0x2c,
	0x08,0x00,0x00,0x1b,0x66,0x0e,0x08,0x00,
	0x00,0x11,0x66,0x08,0x61,0x00,0xfd,0x4c,
	0x51,0xcc,0xff,0xe4,0x4c,0xdf,0x00,0xf2,
	0x4a,0x38,0x09,0xe1,0x67,0x0c,0x31,0xf8,
	0x09,0xc2,0x09,0xc4,0x11,0xfc,0x00,0x40,
	0x09,0xe1,0x4e,0x75,0x08,0x00,0x00,0x1f,
	0x66,0xe2,0xd3,0xc5,0x96,0x85,0x63,0xdc,
	0x20,0x04,0x48,0x40,0x38,0x00,0x30,0x3c,
	0x00,0x12,0x52,0x02,0xb0,0x02,0x64,0xae,
	0x14,0x3c,0x00,0x0a,0x0a,0x42,0x01,0x00,
	0x08,0x02,0x00,0x08,0x66,0x98,0xd4,0xbc,
	0x00,0x01,0x00,0x00,0x61,0x00,0xfd,0x8c,
	0x08,0x00,0x00,0x1b,0x66,0xae,0x60,0x8e,
	0x08,0x01,0x00,0x0c,0x66,0x08,0x4d,0xfa,
	0xfa,0xe0,0x2c,0x56,0x4e,0xd6,0x61,0x00,
	0xfb,0xc6,0x48,0xe7,0x4f,0x00,0x61,0x00,
	0xfd,0xfc,0x61,0x00,0xfb,0xd0,0x08,0x00,
	0x00,0x1b,0x66,0x24,0xc2,0x3c,0x00,0xc0,
	0x82,0x3c,0x00,0x11,0x61,0x62,0x08,0x00,
	0x00,0x0a,0x66,0x14,0x08,0x00,0x00,0x1e,
	0x67,0x16,0x08,0x00,0x00,0x1b,0x66,0x08,
	0x61,0x00,0xfc,0xb0,0x51,0xcc,0xff,0xe6,
	0x4c,0xdf,0x00,0xf2,0x60,0x00,0xfb,0x34,
	0x08,0x00,0x00,0x1f,0x66,0xf2,0xd3,0xc5,
	0x96,0x85,0x63,0xec,0x20,0x04,0x48,0x40,
	0x38,0x00,0x30,0x3c,0x00,0x12,0x30,0x3c,
	0x00,0x12,0x52,0x02,0xb0,0x02,0x64,0xbc,
	0x14,0x3c,0x00,0x0a,0x0a,0x42,0x01,0x00,
	0x08,0x02,0x00,0x08,0x66,0xae,0xd4,0xbc,
	0x00,0x01,0x00,0x00,0x61,0x00,0xfc,0xfc,
	0x08,0x00,0x00,0x1b,0x66,0xba,0x60,0x9c,
	0x61,0x00,0xfb,0x78,0xe1,0x9a,0x54,0x88,
	0x20,0xc2,0xe0,0x9a,0x10,0xc2,0x10,0xc7,
	0x10,0x86,0x61,0x00,0xfb,0x86,0x41,0xf8,
	0x09,0xee,0x70,0x08,0x61,0x00,0xfb,0xb4,
	0x61,0x00,0xfb,0xea,0x61,0x00,0xfc,0x08,
	0x61,0x00,0xfc,0x22,0x4e,0x75,0x43,0xfa,
	0x01,0x8c,0x61,0x00,0x01,0x76,0x24,0x3c,
	0x03,0x00,0x00,0x06,0x32,0x39,0x00,0x00,
	0x00,0x64,0x26,0x3c,0x00,0x00,0x04,0x00,
	0x43,0xf8,0x28,0x00,0x61,0x00,0xfd,0xf2,
	0x4a,0x80,0x66,0x00,0x01,0x20,0x43,0xf8,
	0x28,0x00,0x49,0xfa,0x01,0x54,0x78,0x1f,
	0x24,0x49,0x26,0x4c,0x7a,0x0a,0x10,0x1a,
	0x80,0x3c,0x00,0x20,0xb0,0x1b,0x66,0x06,
	0x51,0xcd,0xff,0xf4,0x60,0x0c,0x43,0xe9,
	0x00,0x20,0x51,0xcc,0xff,0xe4,0x66,0x00,
	0x00,0xf4,0x30,0x29,0x00,0x1a,0xe1,0x58,
	0x55,0x40,0xd0,0x7c,0x00,0x0b,0x34,0x00,
	0xc4,0x7c,0x00,0x07,0x52,0x02,0xe8,0x48,
	0x64,0x04,0x84,0x7c,0x01,0x00,0x48,0x42,
	0x34,0x3c,0x03,0x00,0x14,0x00,0x48,0x42,
	0x26,0x29,0x00,0x1c,0xe1,0x5b,0x48,0x43,
	0xe1,0x5b,0x43,0xf8,0x67,0xc0,0x61,0x00,
	0xfd,0x88,0x0c,0x51,0x48,0x55,0x66,0x00,
	0x00,0xb4,0x4b,0xf8,0x68,0x00,0x49,0xfa,
	0x00,0x4c,0x22,0x4d,0x43,0xf1,0x38,0xc0,
	0x2c,0x3c,0x00,0x04,0x00,0x00,0x0c,0x69,
	0x4e,0xd4,0xff,0xd2,0x66,0x36,0x0c,0xad,
	0x4c,0x5a,0x58,0x20,0x00,0x04,0x66,0x16,
	0x2b,0x46,0x00,0x04,0x2b,0x4d,0x00,0x08,
	0x42,0xad,0x00,0x20,0x51,0xf9,0x00,0x00,
	0x07,0x9e,0x4e,0xed,0x00,0x02,0x0c,0x6d,
	0x4e,0xec,0x00,0x1a,0x66,0x0e,0x0c,0x6d,
	0x4e,0xea,0x00,0x2a,0x66,0x06,0x43,0xfa,
	0x01,0x1f,0x60,0x64,0x10,0x3c,0x00,0xc0,
	0x41,0xf8,0x68,0x00,0x36,0x3c,0xff,0xff,
	0xb0,0x18,0x67,0x26,0x51,0xcb,0xff,0xfa,
	0x43,0xf8,0x68,0x00,0x4a,0x39,0x00,0x00,
	0x07,0x9e,0x67,0x14,0x41,0xf8,0x67,0xcc,
	0x24,0x18,0xd4,0x98,0x22,0x10,0xd1,0xc2,
	0x53,0x81,0x65,0x04,0x42,0x18,0x60,0xf8,
	0x4e,0xd1,0x0c,0x10,0x00,0x04,0x66,0xd0,
	0x52,0x88,0x0c,0x10,0x00,0xd0,0x66,0xc8,
	0x52,0x88,0x0c,0x10,0x00,0xfe,0x66,0xc0,
	0x52,0x88,0x0c,0x10,0x00,0x02,0x66,0xb8,
	0x57,0x88,0x30,0xfc,0x05,0xa1,0x10,0xbc,
	0x00,0xfb,0x60,0xac,0x43,0xfa,0x00,0x92,
	0x2f,0x09,0x43,0xfa,0x00,0x47,0x61,0x2a,
	0x43,0xfa,0x00,0x46,0x61,0x24,0x43,0xfa,
	0x00,0x52,0x61,0x1e,0x43,0xfa,0x00,0x43,
	0x61,0x18,0x43,0xfa,0x00,0x46,0x61,0x12,
	0x22,0x5f,0x61,0x0e,0x32,0x39,0x00,0x00,
	0x00,0x64,0x70,0x4f,0x4e,0x4f,0x70,0xfe,
	0x4e,0x4f,0x70,0x21,0x4e,0x4f,0x4e,0x75,
	0x68,0x75,0x6d,0x61,0x6e,0x20,0x20,0x20,
	0x73,0x79,0x73,0x00,0x39,0x53,0x43,0x49,
	0x50,0x4c,0x00,0x1b,0x5b,0x34,0x37,0x6d,
	0x1b,0x5b,0x31,0x33,0x3b,0x32,0x36,0x48,
	0x00,0x1b,0x5b,0x31,0x34,0x3b,0x32,0x36,
	0x48,0x00,0x20,0x20,0x20,0x20,0x20,0x20,
	0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
	0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
	0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
	0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
	0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x00,
	0x1b,0x5b,0x31,0x34,0x3b,0x33,0x34,0x48,
	0x48,0x75,0x6d,0x61,0x6e,0x2e,0x73,0x79,
	0x73,0x20,0x82,0xcc,0x93,0xc7,0x82,0xdd,
	0x8d,0x9e,0x82,0xdd,0x83,0x47,0x83,0x89,
	0x81,0x5b,0x82,0xc5,0x82,0xb7,0x00,0x1b,
	0x5b,0x31,0x34,0x3b,0x33,0x34,0x48,0x4c,
	0x5a,0x58,0x2e,0x58,0x20,0x82,0xcc,0x83,
	0x6f,0x81,0x5b,0x83,0x57,0x83,0x87,0x83,
	0x93,0x82,0xaa,0x8c,0xc3,0x82,0xb7,0x82,
	0xac,0x82,0xdc,0x82,0xb7,0x00,0x00,0x00
};

//---------------------------------------------------------------------------
//
//	�_���t�H�[�}�b�g(2HC)
//
//---------------------------------------------------------------------------
void FASTCALL FDIDisk::Create2HC()
{
	FDITrack *track;
	FDISector *sector;
	BYTE buf[0x200];
	DWORD chrn[4];
	int i;

	ASSERT(this);

	// �ʏ평����
	memset(buf, 0, sizeof(buf));

	// ���v29�Z�N�^�֏�������(1�g���b�N������15�Z�N�^)
	track = Search(0);
	ASSERT(track);
	chrn[0] = 0;
	chrn[1] = 0;
	chrn[3] = 2;
	for (i=1; i<=15; i++) {
		chrn[2] = (BYTE)i;
		sector = track->Search(TRUE, chrn);
		ASSERT(sector);
		sector->Write(buf, FALSE);
	}
	track = Search(1);
	ASSERT(track);
	chrn[0] = 0;
	chrn[1] = 1;
	chrn[3] = 2;
	for (i=1; i<=14; i++) {
		chrn[2] = (BYTE)i;
		sector = track->Search(TRUE, chrn);
		ASSERT(sector);
		sector->Write(buf, FALSE);
	}

	// �g���b�N0�փV�[�N
	track = Search(0);
	ASSERT(track);

	// IPL��������
	chrn[0] = 0;
	chrn[1] = 0;
	chrn[2] = 1;
	chrn[3] = 2;
	sector = track->Search(TRUE, chrn);
	ASSERT(sector);
	sector->Write(IPL2HC, FALSE);

	// FAT�擪�Z�N�^������
	buf[0] = 0xf9;
	buf[1] = 0xff;
	buf[2] = 0xff;

	// ��1FAT��������
	chrn[0] = 0;
	chrn[1] = 0;
	chrn[2] = 2;
	chrn[3] = 2;
	sector = track->Search(TRUE, chrn);
	ASSERT(sector);
	sector->Write(buf, FALSE);

	// ��2FAT��������
	chrn[0] = 0;
	chrn[1] = 0;
	chrn[2] = 9;
	chrn[3] = 2;
	sector = track->Search(TRUE, chrn);
	ASSERT(sector);
	sector->Write(buf, FALSE);
}

//---------------------------------------------------------------------------
//
//	IPL(2HC)
//	��FORMAT.x v2.31���擾��������
//
//---------------------------------------------------------------------------
const BYTE FDIDisk::IPL2HC[0x200] = {
	0x60,0x3c,0x90,0x58,0x36,0x38,0x49,0x50,
	0x4c,0x33,0x30,0x00,0x02,0x01,0x01,0x00,
	0x02,0xe0,0x00,0x60,0x09,0xf9,0x07,0x00,
	0x0f,0x00,0x02,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x20,0x20,0x20,0x20,0x20,
	0x20,0x20,0x20,0x20,0x20,0x20,0x46,0x41,
	0x54,0x31,0x32,0x20,0x20,0x20,0x4f,0xfa,
	0xff,0xc0,0x4d,0xfa,0x01,0xb8,0x4b,0xfa,
	0x00,0xe0,0x49,0xfa,0x00,0xea,0x43,0xfa,
	0x01,0x20,0x4e,0x94,0x70,0x8e,0x4e,0x4f,
	0x7e,0x70,0xe1,0x48,0x8e,0x40,0x26,0x3a,
	0x01,0x02,0x22,0x4e,0x24,0x3a,0x01,0x00,
	0x32,0x07,0x4e,0x95,0x66,0x28,0x22,0x4e,
	0x32,0x3a,0x00,0xfa,0x20,0x49,0x45,0xfa,
	0x01,0x78,0x70,0x0a,0x00,0x10,0x00,0x20,
	0xb1,0x0a,0x56,0xc8,0xff,0xf8,0x67,0x38,
	0xd2,0xfc,0x00,0x20,0x51,0xc9,0xff,0xe6,
	0x45,0xfa,0x00,0xe0,0x60,0x10,0x45,0xfa,
	0x00,0xfa,0x60,0x0a,0x45,0xfa,0x01,0x10,
	0x60,0x04,0x45,0xfa,0x01,0x28,0x61,0x00,
	0x00,0x94,0x22,0x4a,0x4c,0x99,0x00,0x06,
	0x70,0x23,0x4e,0x4f,0x4e,0x94,0x32,0x07,
	0x70,0x4f,0x4e,0x4f,0x70,0xfe,0x4e,0x4f,
	0x74,0x00,0x34,0x29,0x00,0x1a,0xe1,0x5a,
	0xd4,0x7a,0x00,0xa4,0x84,0xfa,0x00,0x9c,
	0x84,0x7a,0x00,0x94,0xe2,0x0a,0x64,0x04,
	0x08,0xc2,0x00,0x18,0x48,0x42,0x52,0x02,
	0x22,0x4e,0x26,0x3a,0x00,0x7e,0x32,0x07,
	0x4e,0x95,0x34,0x7c,0x68,0x00,0x22,0x4e,
	0x0c,0x59,0x48,0x55,0x66,0xa6,0x54,0x89,
	0xb5,0xd9,0x66,0xa6,0x2f,0x19,0x20,0x59,
	0xd1,0xd9,0x2f,0x08,0x2f,0x11,0x32,0x7c,
	0x67,0xc0,0x76,0x40,0xd6,0x88,0x4e,0x95,
	0x22,0x1f,0x24,0x1f,0x22,0x5f,0x4a,0x80,
	0x66,0x00,0xff,0x7c,0xd5,0xc2,0x53,0x81,
	0x65,0x04,0x42,0x1a,0x60,0xf8,0x4e,0xd1,
	0x70,0x46,0x4e,0x4f,0x08,0x00,0x00,0x1e,
	0x66,0x02,0x70,0x00,0x4e,0x75,0x70,0x21,
	0x4e,0x4f,0x4e,0x75,0x72,0x0f,0x70,0x22,
	0x4e,0x4f,0x72,0x19,0x74,0x0c,0x70,0x23,
	0x4e,0x4f,0x61,0x08,0x72,0x19,0x74,0x0d,
	0x70,0x23,0x4e,0x4f,0x76,0x2c,0x72,0x20,
	0x70,0x20,0x4e,0x4f,0x51,0xcb,0xff,0xf8,
	0x4e,0x75,0x00,0x00,0x02,0x00,0x02,0x00,
	0x01,0x01,0x00,0x0f,0x00,0x0f,0x00,0x1b,
	0x1a,0x00,0x00,0x22,0x00,0x0d,0x48,0x75,
	0x6d,0x61,0x6e,0x2e,0x73,0x79,0x73,0x20,
	0x82,0xaa,0x20,0x8c,0xa9,0x82,0xc2,0x82,
	0xa9,0x82,0xe8,0x82,0xdc,0x82,0xb9,0x82,
	0xf1,0x00,0x00,0x25,0x00,0x0d,0x83,0x66,
	0x83,0x42,0x83,0x58,0x83,0x4e,0x82,0xaa,
	0x81,0x40,0x93,0xc7,0x82,0xdf,0x82,0xdc,
	0x82,0xb9,0x82,0xf1,0x00,0x00,0x00,0x23,
	0x00,0x0d,0x48,0x75,0x6d,0x61,0x6e,0x2e,
	0x73,0x79,0x73,0x20,0x82,0xaa,0x20,0x89,
	0xf3,0x82,0xea,0x82,0xc4,0x82,0xa2,0x82,
	0xdc,0x82,0xb7,0x00,0x00,0x20,0x00,0x0d,
	0x48,0x75,0x6d,0x61,0x6e,0x2e,0x73,0x79,
	0x73,0x20,0x82,0xcc,0x20,0x83,0x41,0x83,
	0x68,0x83,0x8c,0x83,0x58,0x82,0xaa,0x88,
	0xd9,0x8f,0xed,0x82,0xc5,0x82,0xb7,0x00,
	0x68,0x75,0x6d,0x61,0x6e,0x20,0x20,0x20,
	0x73,0x79,0x73,0x00,0x00,0x00,0x00,0x00
};

//---------------------------------------------------------------------------
//
//	�_���t�H�[�}�b�g(2HDE)
//
//---------------------------------------------------------------------------
void FASTCALL FDIDisk::Create2HDE()
{
	FDITrack *track;
	FDISector *sector;
	BYTE buf[0x400];
	DWORD chrn[4];
	int i;

	ASSERT(this);

	// �ʏ평����
	memset(buf, 0, sizeof(buf));

	// ���v13�Z�N�^�֏�������(1�g���b�N������9�Z�N�^)
	track = Search(0);
	ASSERT(track);
	chrn[0] = 0;
	chrn[1] = 0x80;
	chrn[3] = 3;
	for (i=2; i<=9; i++) {
		chrn[2] = i;
		sector = track->Search(TRUE, chrn);
		ASSERT(sector);
		sector->Write(buf, FALSE);
	}
	track = Search(1);
	ASSERT(track);
	chrn[0] = 0;
	chrn[1] = 0x81;
	chrn[3] = 3;
	for (i=1; i<=4; i++) {
		chrn[2] = i;
		sector = track->Search(TRUE, chrn);
		ASSERT(sector);
		sector->Write(buf, FALSE);
	}

	// �g���b�N0�փV�[�N
	track = Search(0);
	ASSERT(track);

	// IPL��������(1)
	chrn[0] = 0;
	chrn[1] = 0;
	chrn[2] = 1;
	chrn[3] = 3;
	sector = track->Search(TRUE, chrn);
	ASSERT(sector);
	sector->Write(&IPL2HDE[0x000], FALSE);

	// IPL��������(2)
	chrn[0] = 0;
	chrn[1] = 0x80;
	chrn[2] = 4;
	chrn[3] = 3;
	sector = track->Search(TRUE, chrn);
	ASSERT(sector);
	sector->Write(&IPL2HDE[0x400], FALSE);

	// FAT�擪�Z�N�^������
	buf[0] = 0xf8;
	buf[1] = 0xff;
	buf[2] = 0xff;

	// ��1FAT��������
	chrn[0] = 0;
	chrn[1] = 0x80;
	chrn[2] = 2;
	chrn[3] = 3;
	sector = track->Search(TRUE, chrn);
	ASSERT(sector);
	sector->Write(buf, FALSE);

	// ��2FAT��������
	chrn[0] = 0;
	chrn[1] = 0x80;
	chrn[2] = 5;
	chrn[3] = 3;
	sector = track->Search(TRUE, chrn);
	ASSERT(sector);
	sector->Write(buf, FALSE);
}

//---------------------------------------------------------------------------
//
//	IPL(2HDE)
//	��9scdrv.x v3.00���擾��������
//
//---------------------------------------------------------------------------
const BYTE FDIDisk::IPL2HDE[0x800] = {
	0x60,0x20,0x32,0x48,0x44,0x45,0x20,0x76,
	0x31,0x2e,0x31,0x00,0x00,0x04,0x01,0x01,
	0x00,0x02,0xc0,0x00,0xa0,0x05,0x03,0x03,
	0x00,0x09,0x00,0x02,0x00,0x00,0x00,0x00,
	0x90,0x70,0x60,0x00,0x03,0x5a,0x08,0x01,
	0x00,0x0c,0x66,0x08,0x4d,0xfa,0xff,0xd2,
	0x2c,0x56,0x4e,0xd6,0x61,0x00,0x00,0xba,
	0x48,0xe7,0x4f,0x00,0x61,0x00,0x02,0xf0,
	0x61,0x00,0x00,0xc4,0x08,0x00,0x00,0x1b,
	0x66,0x4e,0xc2,0x3c,0x00,0xc0,0x82,0x3c,
	0x00,0x06,0x61,0x00,0x00,0xd0,0xe1,0x9a,
	0x54,0x88,0x20,0xc2,0xe0,0x9a,0x10,0xc2,
	0x10,0xc7,0x10,0x86,0x61,0x00,0x00,0xf0,
	0x41,0xf8,0x09,0xee,0x70,0x08,0x61,0x00,
	0x01,0x0c,0x61,0x00,0x01,0x42,0x61,0x00,
	0x01,0x60,0x61,0x00,0x01,0x7a,0x08,0x00,
	0x00,0x0e,0x66,0x0c,0x08,0x00,0x00,0x1e,
	0x67,0x26,0x08,0x00,0x00,0x1b,0x66,0x08,
	0x61,0x00,0x01,0x7a,0x51,0xcc,0xff,0xbc,
	0x4c,0xdf,0x00,0xf2,0x4a,0x38,0x09,0xe1,
	0x67,0x0c,0x31,0xf8,0x09,0xc2,0x09,0xc4,
	0x11,0xfc,0x00,0x40,0x09,0xe1,0x4e,0x75,
	0x08,0x00,0x00,0x1f,0x66,0xe2,0xd3,0xc5,
	0x96,0x85,0x63,0xdc,0x20,0x04,0x48,0x40,
	0x38,0x00,0x30,0x3c,0x00,0x09,0x52,0x02,
	0xb0,0x02,0x64,0x86,0x14,0x3c,0x00,0x01,
	0x0a,0x42,0x01,0x00,0x08,0x02,0x00,0x08,
	0x66,0x00,0xff,0x78,0xd4,0xbc,0x00,0x01,
	0x00,0x00,0x61,0x00,0x01,0xb8,0x08,0x00,
	0x00,0x1b,0x66,0xac,0x60,0x00,0xff,0x64,
	0x08,0x38,0x00,0x07,0x09,0xe1,0x66,0x0c,
	0x48,0xe7,0xc0,0x00,0x61,0x00,0x01,0x46,
	0x4c,0xdf,0x00,0x03,0x4e,0x75,0x70,0x00,
	0x78,0x00,0x08,0x01,0x00,0x05,0x67,0x08,
	0x78,0x09,0x48,0x44,0x38,0x3c,0x00,0x09,
	0x08,0x01,0x00,0x04,0x67,0x04,0x61,0x00,
	0x01,0x7c,0x4e,0x75,0x2f,0x01,0x41,0xf8,
	0x09,0xee,0x10,0x81,0xe0,0x99,0xc2,0x3c,
	0x00,0x03,0x08,0x02,0x00,0x08,0x67,0x04,
	0x08,0xc1,0x00,0x02,0x11,0x41,0x00,0x01,
	0x22,0x1f,0x4e,0x75,0x13,0xfc,0x00,0xff,
	0x00,0xe8,0x40,0x00,0x13,0xfc,0x00,0x32,
	0x00,0xe8,0x40,0x05,0x60,0x10,0x13,0xfc,
	0x00,0xff,0x00,0xe8,0x40,0x00,0x13,0xfc,
	0x00,0xb2,0x00,0xe8,0x40,0x05,0x23,0xc9,
	0x00,0xe8,0x40,0x0c,0x33,0xc5,0x00,0xe8,
	0x40,0x0a,0x13,0xfc,0x00,0x80,0x00,0xe8,
	0x40,0x07,0x4e,0x75,0x48,0xe7,0x40,0x60,
	0x43,0xf9,0x00,0xe9,0x40,0x01,0x45,0xf9,
	0x00,0xe9,0x40,0x03,0x40,0xe7,0x00,0x7c,
	0x07,0x00,0x12,0x11,0x08,0x01,0x00,0x04,
	0x66,0xf8,0x12,0x11,0x08,0x01,0x00,0x07,
	0x67,0xf8,0x08,0x01,0x00,0x06,0x66,0xf2,
	0x14,0x98,0x51,0xc8,0xff,0xee,0x46,0xdf,
	0x4c,0xdf,0x06,0x02,0x4e,0x75,0x10,0x39,
	0x00,0xe8,0x40,0x00,0x08,0x00,0x00,0x04,
	0x66,0x0e,0x10,0x39,0x00,0xe9,0x40,0x01,
	0xc0,0x3c,0x00,0x1f,0x66,0xf4,0x4e,0x75,
	0x10,0x39,0x00,0xe8,0x40,0x01,0x4e,0x75,
	0x10,0x39,0x00,0xe8,0x40,0x00,0x08,0x00,
	0x00,0x07,0x66,0x08,0x13,0xfc,0x00,0x10,
	0x00,0xe8,0x40,0x07,0x13,0xfc,0x00,0xff,
	0x00,0xe8,0x40,0x00,0x4e,0x75,0x30,0x01,
	0xe0,0x48,0xc0,0xbc,0x00,0x00,0x00,0x03,
	0xe7,0x40,0x41,0xf8,0x0c,0x90,0xd1,0xc0,
	0x20,0x10,0x4e,0x75,0x2f,0x00,0xc0,0xbc,
	0x00,0x35,0xff,0x00,0x67,0x2a,0xb8,0x3c,
	0x00,0x05,0x64,0x24,0x2f,0x38,0x09,0xee,
	0x2f,0x38,0x09,0xf2,0x3f,0x38,0x09,0xf6,
	0x61,0x00,0x00,0xc4,0x70,0x64,0x51,0xc8,
	0xff,0xfe,0x61,0x68,0x31,0xdf,0x09,0xf6,
	0x21,0xdf,0x09,0xf2,0x21,0xdf,0x09,0xee,
	0x20,0x1f,0x4e,0x75,0x30,0x01,0xe0,0x48,
	0x4a,0x00,0x67,0x3c,0xc0,0x3c,0x00,0x03,
	0x80,0x3c,0x00,0x80,0x08,0xf8,0x00,0x07,
	0x09,0xe1,0x13,0xc0,0x00,0xe9,0x40,0x07,
	0x08,0xf8,0x00,0x06,0x09,0xe1,0x66,0x18,
	0x31,0xf8,0x09,0xc2,0x09,0xc4,0x61,0x00,
	0x00,0x90,0x08,0x00,0x00,0x1d,0x66,0x08,
	0x0c,0x78,0x00,0x64,0x09,0xc4,0x64,0xee,
	0x08,0xb8,0x00,0x06,0x09,0xe1,0x4e,0x75,
	0x4a,0x38,0x09,0xe1,0x67,0x0c,0x31,0xf8,
	0x09,0xc2,0x09,0xc4,0x11,0xfc,0x00,0x40,
	0x09,0xe1,0x4e,0x75,0x61,0x12,0x08,0x00,
	0x00,0x1b,0x66,0x26,0x48,0x40,0x48,0x42,
	0xb4,0x00,0x67,0x1a,0x48,0x42,0x61,0x3e,
	0x2f,0x01,0x12,0x3c,0x00,0x0f,0x61,0x00,
	0xfe,0x6c,0x48,0x42,0x11,0x42,0x00,0x02,
	0x48,0x42,0x70,0x02,0x60,0x08,0x48,0x42,
	0x48,0x40,0x4e,0x75,0x2f,0x01,0x61,0x00,
	0xfe,0xac,0x61,0x00,0xfe,0xee,0x22,0x1f,
	0x30,0x01,0xe0,0x48,0xc0,0xbc,0x00,0x00,
	0x00,0x03,0xe7,0x40,0x41,0xf8,0x0c,0x90,
	0xd1,0xc0,0x20,0x10,0x4e,0x75,0x2f,0x01,
	0x12,0x3c,0x00,0x07,0x61,0x00,0xfe,0x2e,
	0x70,0x01,0x61,0xd0,0x22,0x1f,0x4e,0x75,
	0x2f,0x01,0x12,0x3c,0x00,0x04,0x61,0x00,
	0xfe,0x1c,0x22,0x1f,0x70,0x01,0x61,0x00,
	0xfe,0x6c,0x10,0x39,0x00,0xe9,0x40,0x01,
	0xc0,0x3c,0x00,0xd0,0xb0,0x3c,0x00,0xd0,
	0x66,0xf0,0x70,0x00,0x10,0x39,0x00,0xe9,
	0x40,0x03,0xe0,0x98,0x4e,0x75,0x53,0x02,
	0x7e,0x00,0x3a,0x02,0xe0,0x5d,0x4a,0x05,
	0x67,0x04,0x06,0x45,0x08,0x00,0xe0,0x4d,
	0x48,0x42,0x02,0x82,0x00,0x00,0x00,0xff,
	0xe9,0x8a,0xd4,0x45,0x0c,0x42,0x00,0x04,
	0x65,0x02,0x54,0x42,0x84,0xfc,0x00,0x12,
	0x48,0x42,0x3e,0x02,0x8e,0xfc,0x00,0x09,
	0x48,0x47,0xe1,0x4f,0xe0,0x8f,0x34,0x07,
	0x06,0x82,0x03,0x00,0x80,0x01,0x2a,0x3c,
	0x00,0x00,0x04,0x00,0x3c,0x3c,0x00,0xff,
	0x3e,0x3c,0x09,0x28,0x4e,0x75,0x4f,0xfa,
	0xfc,0x80,0x43,0xfa,0xfc,0xa2,0x4d,0xfa,
	0xfc,0x78,0x2c,0xb9,0x00,0x00,0x05,0x18,
	0x23,0xc9,0x00,0x00,0x05,0x18,0x43,0xfa,
	0x00,0xda,0x4d,0xfa,0xfc,0x68,0x2c,0xb9,
	0x00,0x00,0x05,0x14,0x23,0xc9,0x00,0x00,
	0x05,0x14,0x43,0xfa,0x01,0x6e,0x4d,0xfa,
	0xfc,0x58,0x2c,0xb9,0x00,0x00,0x05,0x04,
	0x23,0xc9,0x00,0x00,0x05,0x04,0x24,0x3c,
	0x03,0x00,0x00,0x04,0x20,0x3c,0x00,0x00,
	0x00,0x8e,0x4e,0x4f,0x12,0x00,0xe1,0x41,
	0x12,0x3c,0x00,0x70,0x33,0xc1,0x00,0x00,
	0x00,0x66,0x26,0x3c,0x00,0x00,0x04,0x00,
	0x43,0xfa,0x00,0x20,0x61,0x04,0x60,0x00,
	0x01,0xec,0x48,0xe7,0x78,0x40,0x70,0x46,
	0x4e,0x4f,0x08,0x00,0x00,0x1e,0x66,0x02,
	0x70,0x00,0x4c,0xdf,0x02,0x1e,0x4e,0x75,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x08,0x01,0x00,0x0c,0x66,0x08,0x4d,0xfa,
	0xfb,0x8a,0x2c,0x56,0x4e,0xd6,0x61,0x00,
	0xfc,0x6e,0x48,0xe7,0x4f,0x00,0x61,0x00,
	0xfe,0xa4,0x61,0x00,0xfc,0x78,0x08,0x00,
	0x00,0x1b,0x66,0x30,0xc2,0x3c,0x00,0xc0,
	0x82,0x3c,0x00,0x05,0x60,0x08,0x30,0x3c,
	0x01,0xac,0x51,0xc8,0xff,0xfe,0x61,0x00,
	0x00,0xfc,0x08,0x00,0x00,0x1e,0x67,0x2c,
	0x08,0x00,0x00,0x1b,0x66,0x0e,0x08,0x00,
	0x00,0x11,0x66,0x08,0x61,0x00,0xfd,0x4c,
	0x51,0xcc,0xff,0xe4,0x4c,0xdf,0x00,0xf2,
	0x4a,0x38,0x09,0xe1,0x67,0x0c,0x31,0xf8,
	0x09,0xc2,0x09,0xc4,0x11,0xfc,0x00,0x40,
	0x09,0xe1,0x4e,0x75,0x08,0x00,0x00,0x1f,
	0x66,0xe2,0xd3,0xc5,0x96,0x85,0x63,0xdc,
	0x20,0x04,0x48,0x40,0x38,0x00,0x30,0x3c,
	0x00,0x09,0x52,0x02,0xb0,0x02,0x64,0xae,
	0x14,0x3c,0x00,0x01,0x0a,0x42,0x01,0x00,
	0x08,0x02,0x00,0x08,0x66,0x98,0xd4,0xbc,
	0x00,0x01,0x00,0x00,0x61,0x00,0xfd,0x8c,
	0x08,0x00,0x00,0x1b,0x66,0xae,0x60,0x8e,
	0x08,0x01,0x00,0x0c,0x66,0x08,0x4d,0xfa,
	0xfa,0xde,0x2c,0x56,0x4e,0xd6,0x61,0x00,
	0xfb,0xc6,0x48,0xe7,0x4f,0x00,0x61,0x00,
	0xfd,0xfc,0x61,0x00,0xfb,0xd0,0x08,0x00,
	0x00,0x1b,0x66,0x24,0xc2,0x3c,0x00,0xc0,
	0x82,0x3c,0x00,0x11,0x61,0x5e,0x08,0x00,
	0x00,0x0a,0x66,0x14,0x08,0x00,0x00,0x1e,
	0x67,0x16,0x08,0x00,0x00,0x1b,0x66,0x08,
	0x61,0x00,0xfc,0xb0,0x51,0xcc,0xff,0xe6,
	0x4c,0xdf,0x00,0xf2,0x60,0x00,0xfb,0x34,
	0x08,0x00,0x00,0x1f,0x66,0xf2,0xd3,0xc5,
	0x96,0x85,0x63,0xec,0x20,0x04,0x48,0x40,
	0x38,0x00,0x30,0x3c,0x00,0x09,0x52,0x02,
	0xb0,0x02,0x64,0xc0,0x14,0x3c,0x00,0x01,
	0x0a,0x42,0x01,0x00,0x08,0x02,0x00,0x08,
	0x66,0xb2,0xd4,0xbc,0x00,0x01,0x00,0x00,
	0x61,0x00,0xfd,0x00,0x08,0x00,0x00,0x1b,
	0x66,0xbe,0x60,0xa0,0x61,0x00,0xfb,0x7c,
	0xe1,0x9a,0x54,0x88,0x20,0xc2,0xe0,0x9a,
	0x10,0xc2,0x10,0xc7,0x10,0x86,0x61,0x00,
	0xfb,0x8a,0x41,0xf8,0x09,0xee,0x70,0x08,
	0x61,0x00,0xfb,0xb8,0x61,0x00,0xfb,0xee,
	0x61,0x00,0xfc,0x0c,0x61,0x00,0xfc,0x26,
	0x4e,0x75,0x43,0xfa,0x01,0x8c,0x61,0x00,
	0x01,0x76,0x24,0x3c,0x03,0x00,0x00,0x06,
	0x32,0x39,0x00,0x00,0x00,0x66,0x26,0x3c,
	0x00,0x00,0x04,0x00,0x43,0xf8,0x28,0x00,
	0x61,0x00,0xfd,0xf6,0x4a,0x80,0x66,0x00,
	0x01,0x20,0x43,0xf8,0x28,0x00,0x49,0xfa,
	0x01,0x54,0x78,0x1f,0x24,0x49,0x26,0x4c,
	0x7a,0x0a,0x10,0x1a,0x80,0x3c,0x00,0x20,
	0xb0,0x1b,0x66,0x06,0x51,0xcd,0xff,0xf4,
	0x60,0x0c,0x43,0xe9,0x00,0x20,0x51,0xcc,
	0xff,0xe4,0x66,0x00,0x00,0xf4,0x30,0x29,
	0x00,0x1a,0xe1,0x58,0x55,0x40,0xd0,0x7c,
	0x00,0x0b,0x34,0x00,0xc4,0x7c,0x00,0x07,
	0x52,0x02,0xe8,0x48,0x64,0x04,0x84,0x7c,
	0x01,0x00,0x48,0x42,0x34,0x3c,0x03,0x00,
	0x14,0x00,0x48,0x42,0x26,0x29,0x00,0x1c,
	0xe1,0x5b,0x48,0x43,0xe1,0x5b,0x43,0xf8,
	0x67,0xc0,0x61,0x00,0xfd,0x8c,0x0c,0x51,
	0x48,0x55,0x66,0x00,0x00,0xb4,0x4b,0xf8,
	0x68,0x00,0x49,0xfa,0x00,0x4c,0x22,0x4d,
	0x43,0xf1,0x38,0xc0,0x2c,0x3c,0x00,0x04,
	0x00,0x00,0x0c,0x69,0x4e,0xd4,0xff,0xd2,
	0x66,0x36,0x0c,0xad,0x4c,0x5a,0x58,0x20,
	0x00,0x04,0x66,0x16,0x2b,0x46,0x00,0x04,
	0x2b,0x4d,0x00,0x08,0x42,0xad,0x00,0x20,
	0x51,0xf9,0x00,0x00,0x07,0x9c,0x4e,0xed,
	0x00,0x02,0x0c,0x6d,0x4e,0xec,0x00,0x1a,
	0x66,0x0e,0x0c,0x6d,0x4e,0xea,0x00,0x2a,
	0x66,0x06,0x43,0xfa,0x01,0x20,0x60,0x64,
	0x10,0x3c,0x00,0xc0,0x41,0xf8,0x68,0x00,
	0x36,0x3c,0xff,0xff,0xb0,0x18,0x67,0x26,
	0x51,0xcb,0xff,0xfa,0x43,0xf8,0x68,0x00,
	0x4a,0x39,0x00,0x00,0x07,0x9c,0x67,0x14,
	0x41,0xf8,0x67,0xcc,0x24,0x18,0xd4,0x98,
	0x22,0x10,0xd1,0xc2,0x53,0x81,0x65,0x04,
	0x42,0x18,0x60,0xf8,0x4e,0xd1,0x0c,0x10,
	0x00,0x04,0x66,0xd0,0x52,0x88,0x0c,0x10,
	0x00,0xd0,0x66,0xc8,0x52,0x88,0x0c,0x10,
	0x00,0xfe,0x66,0xc0,0x52,0x88,0x0c,0x10,
	0x00,0x02,0x66,0xb8,0x57,0x88,0x30,0xfc,
	0x05,0x9e,0x10,0xbc,0x00,0xfb,0x60,0xac,
	0x43,0xfa,0x00,0x93,0x2f,0x09,0x43,0xfa,
	0x00,0x48,0x61,0x2a,0x43,0xfa,0x00,0x47,
	0x61,0x24,0x43,0xfa,0x00,0x53,0x61,0x1e,
	0x43,0xfa,0x00,0x44,0x61,0x18,0x43,0xfa,
	0x00,0x47,0x61,0x12,0x22,0x5f,0x61,0x0e,
	0x32,0x39,0x00,0x00,0x00,0x66,0x70,0x4f,
	0x4e,0x4f,0x70,0xfe,0x4e,0x4f,0x70,0x21,
	0x4e,0x4f,0x4e,0x75,0x68,0x75,0x6d,0x61,
	0x6e,0x20,0x20,0x20,0x73,0x79,0x73,0x00,
	0x32,0x48,0x44,0x45,0x49,0x50,0x4c,0x00,
	0x1b,0x5b,0x34,0x37,0x6d,0x1b,0x5b,0x31,
	0x33,0x3b,0x32,0x36,0x48,0x00,0x1b,0x5b,
	0x31,0x34,0x3b,0x32,0x36,0x48,0x00,0x20,
	0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
	0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
	0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
	0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
	0x20,0x20,0x20,0x20,0x20,0x20,0x20,0x20,
	0x20,0x20,0x20,0x20,0x00,0x1b,0x5b,0x31,
	0x34,0x3b,0x33,0x34,0x48,0x48,0x75,0x6d,
	0x61,0x6e,0x2e,0x73,0x79,0x73,0x20,0x82,
	0xcc,0x93,0xc7,0x82,0xdd,0x8d,0x9e,0x82,
	0xdd,0x83,0x47,0x83,0x89,0x81,0x5b,0x82,
	0xc5,0x82,0xb7,0x00,0x1b,0x5b,0x31,0x34,
	0x3b,0x33,0x34,0x48,0x4c,0x5a,0x58,0x2e,
	0x58,0x20,0x82,0xcc,0x83,0x6f,0x81,0x5b,
	0x83,0x57,0x83,0x87,0x83,0x93,0x82,0xaa,
	0x8c,0xc3,0x82,0xb7,0x82,0xac,0x82,0xdc,
	0x82,0xb7,0x00,0x00,0x00,0x00,0x00,0x00
};

//---------------------------------------------------------------------------
//
//	�_���t�H�[�}�b�g(2HQ)
//
//---------------------------------------------------------------------------
void FASTCALL FDIDisk::Create2HQ()
{
	FDITrack *track;
	FDISector *sector;
	BYTE buf[0x200];
	DWORD chrn[4];
	int i;

	ASSERT(this);

	// �ʏ평����
	memset(buf, 0, sizeof(buf));

	// ���v33�Z�N�^�֏�������(1�g���b�N������18�Z�N�^)
	track = Search(0);
	ASSERT(track);
	chrn[0] = 0;
	chrn[1] = 0;
	chrn[3] = 2;
	for (i=1; i<=18; i++) {
		chrn[2] = i;
		sector = track->Search(TRUE, chrn);
		ASSERT(sector);
		sector->Write(buf, FALSE);
	}
	track = Search(1);
	ASSERT(track);
	chrn[0] = 0;
	chrn[1] = 1;
	chrn[3] = 2;
	for (i=1; i<=15; i++) {
		chrn[2] = i;
		sector = track->Search(TRUE, chrn);
		ASSERT(sector);
		sector->Write(buf, FALSE);
	}

	// �g���b�N0�փV�[�N
	track = Search(0);
	ASSERT(track);

	// IPL��������
	chrn[0] = 0;
	chrn[1] = 0;
	chrn[2] = 1;
	chrn[3] = 2;
	sector = track->Search(TRUE, chrn);
	ASSERT(sector);
	sector->Write(IPL2HQ, FALSE);

	// FAT�擪�Z�N�^������
	buf[0] = 0xf0;
	buf[1] = 0xff;
	buf[2] = 0xff;

	// ��1FAT��������
	chrn[0] = 0;
	chrn[1] = 0;
	chrn[2] = 2;
	chrn[3] = 2;
	sector = track->Search(TRUE, chrn);
	ASSERT(sector);
	sector->Write(buf, FALSE);

	// ��2FAT��������
	chrn[0] = 0;
	chrn[1] = 0;
	chrn[2] = 11;
	chrn[3] = 2;
	sector = track->Search(TRUE, chrn);
	ASSERT(sector);
	sector->Write(buf, FALSE);
}

//---------------------------------------------------------------------------
//
//	IPL(2HQ)
//	��FORMAT.x v2.31���擾��������
//
//---------------------------------------------------------------------------
const BYTE FDIDisk::IPL2HQ[0x200] = {
	0xeb,0xfe,0x90,0x58,0x36,0x38,0x49,0x50,
	0x4c,0x33,0x30,0x00,0x02,0x01,0x01,0x00,
	0x02,0xe0,0x00,0x40,0x0b,0xf0,0x09,0x00,
	0x12,0x00,0x02,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
	0x00,0x00,0x00,0x20,0x20,0x20,0x20,0x20,
	0x20,0x20,0x20,0x20,0x20,0x20,0x46,0x41,
	0x54,0x31,0x32,0x20,0x20,0x20,0x4f,0xfa,
	0xff,0xc0,0x4d,0xfa,0x01,0xb8,0x4b,0xfa,
	0x00,0xe0,0x49,0xfa,0x00,0xea,0x43,0xfa,
	0x01,0x20,0x4e,0x94,0x70,0x8e,0x4e,0x4f,
	0x7e,0x70,0xe1,0x48,0x8e,0x40,0x26,0x3a,
	0x01,0x02,0x22,0x4e,0x24,0x3a,0x01,0x00,
	0x32,0x07,0x4e,0x95,0x66,0x28,0x22,0x4e,
	0x32,0x3a,0x00,0xfa,0x20,0x49,0x45,0xfa,
	0x01,0x78,0x70,0x0a,0x00,0x10,0x00,0x20,
	0xb1,0x0a,0x56,0xc8,0xff,0xf8,0x67,0x38,
	0xd2,0xfc,0x00,0x20,0x51,0xc9,0xff,0xe6,
	0x45,0xfa,0x00,0xe0,0x60,0x10,0x45,0xfa,
	0x00,0xfa,0x60,0x0a,0x45,0xfa,0x01,0x10,
	0x60,0x04,0x45,0xfa,0x01,0x28,0x61,0x00,
	0x00,0x94,0x22,0x4a,0x4c,0x99,0x00,0x06,
	0x70,0x23,0x4e,0x4f,0x4e,0x94,0x32,0x07,
	0x70,0x4f,0x4e,0x4f,0x70,0xfe,0x4e,0x4f,
	0x74,0x00,0x34,0x29,0x00,0x1a,0xe1,0x5a,
	0xd4,0x7a,0x00,0xa4,0x84,0xfa,0x00,0x9c,
	0x84,0x7a,0x00,0x94,0xe2,0x0a,0x64,0x04,
	0x08,0xc2,0x00,0x18,0x48,0x42,0x52,0x02,
	0x22,0x4e,0x26,0x3a,0x00,0x7e,0x32,0x07,
	0x4e,0x95,0x34,0x7c,0x68,0x00,0x22,0x4e,
	0x0c,0x59,0x48,0x55,0x66,0xa6,0x54,0x89,
	0xb5,0xd9,0x66,0xa6,0x2f,0x19,0x20,0x59,
	0xd1,0xd9,0x2f,0x08,0x2f,0x11,0x32,0x7c,
	0x67,0xc0,0x76,0x40,0xd6,0x88,0x4e,0x95,
	0x22,0x1f,0x24,0x1f,0x22,0x5f,0x4a,0x80,
	0x66,0x00,0xff,0x7c,0xd5,0xc2,0x53,0x81,
	0x65,0x04,0x42,0x1a,0x60,0xf8,0x4e,0xd1,
	0x70,0x46,0x4e,0x4f,0x08,0x00,0x00,0x1e,
	0x66,0x02,0x70,0x00,0x4e,0x75,0x70,0x21,
	0x4e,0x4f,0x4e,0x75,0x72,0x0f,0x70,0x22,
	0x4e,0x4f,0x72,0x19,0x74,0x0c,0x70,0x23,
	0x4e,0x4f,0x61,0x08,0x72,0x19,0x74,0x0d,
	0x70,0x23,0x4e,0x4f,0x76,0x2c,0x72,0x20,
	0x70,0x20,0x4e,0x4f,0x51,0xcb,0xff,0xf8,
	0x4e,0x75,0x00,0x00,0x02,0x00,0x02,0x00,
	0x01,0x02,0x00,0x12,0x00,0x0f,0x00,0x1f,
	0x1a,0x00,0x00,0x22,0x00,0x0d,0x48,0x75,
	0x6d,0x61,0x6e,0x2e,0x73,0x79,0x73,0x20,
	0x82,0xaa,0x20,0x8c,0xa9,0x82,0xc2,0x82,
	0xa9,0x82,0xe8,0x82,0xdc,0x82,0xb9,0x82,
	0xf1,0x00,0x00,0x25,0x00,0x0d,0x83,0x66,
	0x83,0x42,0x83,0x58,0x83,0x4e,0x82,0xaa,
	0x81,0x40,0x93,0xc7,0x82,0xdf,0x82,0xdc,
	0x82,0xb9,0x82,0xf1,0x00,0x00,0x00,0x23,
	0x00,0x0d,0x48,0x75,0x6d,0x61,0x6e,0x2e,
	0x73,0x79,0x73,0x20,0x82,0xaa,0x20,0x89,
	0xf3,0x82,0xea,0x82,0xc4,0x82,0xa2,0x82,
	0xdc,0x82,0xb7,0x00,0x00,0x20,0x00,0x0d,
	0x48,0x75,0x6d,0x61,0x6e,0x2e,0x73,0x79,
	0x73,0x20,0x82,0xcc,0x20,0x83,0x41,0x83,
	0x68,0x83,0x8c,0x83,0x58,0x82,0xaa,0x88,
	0xd9,0x8f,0xed,0x82,0xc5,0x82,0xb7,0x00,
	0x68,0x75,0x6d,0x61,0x6e,0x20,0x20,0x20,
	0x73,0x79,0x73,0x00,0x00,0x00,0x00,0x00
};

//---------------------------------------------------------------------------
//
//	�_���t�H�[�}�b�g(2DD)
//
//---------------------------------------------------------------------------
void FASTCALL FDIDisk::Create2DD()
{
	FDITrack *track;
	FDISector *sector;
	BYTE buf[0x200];
	DWORD chrn[4];
	int i;

	ASSERT(this);

	// �ʏ평����
	memset(buf, 0, sizeof(buf));

	// ���v14�Z�N�^�֏�������(1�g���b�N������18�Z�N�^)
	track = Search(0);
	ASSERT(track);
	chrn[0] = 0;
	chrn[1] = 0;
	chrn[3] = 2;
	for (i=1; i<=9; i++) {
		chrn[2] = i;
		sector = track->Search(TRUE, chrn);
		ASSERT(sector);
		sector->Write(buf, FALSE);
	}
	track = Search(1);
	ASSERT(track);
	chrn[0] = 0;
	chrn[1] = 1;
	chrn[3] = 2;
	for (i=1; i<=5; i++) {
		chrn[2] = i;
		sector = track->Search(TRUE, chrn);
		ASSERT(sector);
		sector->Write(buf, FALSE);
	}

	// �g���b�N0�փV�[�N
	track = Search(0);
	ASSERT(track);

	// IPL���H(2HQ�p���x�[�X�ɍ쐬)
	memcpy(buf, IPL2HQ, sizeof(buf));
	buf[0] = 0x60;
	buf[1] = 0x3c;
	buf[13] = 0x02;
	buf[17] = 0x70;
	buf[19] = 0xa0;
	buf[20] = 0x05;
	buf[21] = 0xf9;
	buf[22] = 0x03;
	buf[24] = 0x09;
	buf[0x168] = 0x00;
	buf[0x169] = 0x08;
	buf[0x16b] = 0x09;
	buf[0x16f] = 0x0c;

	// IPL��������
	chrn[0] = 0;
	chrn[1] = 0;
	chrn[2] = 1;
	chrn[3] = 2;
	sector = track->Search(TRUE, chrn);
	ASSERT(sector);
	sector->Write(buf, FALSE);
	memset(buf, 0, sizeof(buf));

	// FAT�擪�Z�N�^������
	buf[0] = 0xf9;
	buf[1] = 0xff;
	buf[2] = 0xff;

	// ��1FAT��������
	chrn[0] = 0;
	chrn[1] = 0;
	chrn[2] = 2;
	chrn[3] = 2;
	sector = track->Search(TRUE, chrn);
	ASSERT(sector);
	sector->Write(buf, FALSE);

	// ��2FAT��������
	chrn[0] = 0;
	chrn[1] = 0;
	chrn[2] = 5;
	chrn[3] = 2;
	sector = track->Search(TRUE, chrn);
	ASSERT(sector);
	sector->Write(buf, FALSE);
}

//---------------------------------------------------------------------------
//
//	�I�[�v��
//	���h���N���X�̒��ӓ_�F
//		writep, readonly��K�؂ɐݒ肷��
//		path, name��K�؂ɐݒ肷��
//	����ʃN���X�̒��ӓ_�F
//		�Ăяo������ŁA���݂̃V�����_�փV�[�N����
//
//---------------------------------------------------------------------------
BOOL FASTCALL FDIDisk::Open(const Filepath& /*path*/, DWORD /*offset*/)
{
	// �������z�N���X�I
	ASSERT(FALSE);
	return FALSE;
}

//---------------------------------------------------------------------------
//
//	�������݋֎~�ݒ�
//
//---------------------------------------------------------------------------
void FASTCALL FDIDisk::WriteP(BOOL flag)
{
	ASSERT(this);

	// ReadOnly�Ȃ�A��ɏ������݋֎~
	if (IsReadOnly()) {
		disk.writep = TRUE;
		return;
	}

	// �ݒ�
	disk.writep = flag;
}

//---------------------------------------------------------------------------
//
//	�t���b�V��
//
//---------------------------------------------------------------------------
BOOL FASTCALL FDIDisk::Flush()
{
	ASSERT(this);

	// �������Ȃ�
	return TRUE;
}

//---------------------------------------------------------------------------
//
//	�f�B�X�N���擾
//
//---------------------------------------------------------------------------
void FASTCALL FDIDisk::GetName(char *buf) const
{
	ASSERT(this);
	ASSERT(buf);

	strcpy(buf, disk.name);
}

//---------------------------------------------------------------------------
//
//	�p�X���擾
//
//---------------------------------------------------------------------------
void FASTCALL FDIDisk::GetPath(Filepath& path) const
{
	ASSERT(this);

	// ���
	path = disk.path;
}

//---------------------------------------------------------------------------
//
//	�V�[�N
//	���h���N���X�̒��ӓ_�F
//		���݂�head[]���t�@�C���ɏ����߂��A�V�����g���b�N�����[�h�Ahead[]�փZ�b�g
//
//---------------------------------------------------------------------------
void FASTCALL FDIDisk::Seek(int /*c*/)
{
	ASSERT(this);
}

//---------------------------------------------------------------------------
//
//	���[�hID
//	�����̃X�e�[�^�X��Ԃ�(�G���[��OR�����)
//		FDD_NOERROR		�G���[�Ȃ�
//		FDD_MAM			�w�薧�x�ł̓A���t�H�[�}�b�g
//		FDD_NODATA		�w�薧�x�ł̓A���t�H�[�}�b�g���A
//						�܂��͗L���ȃZ�N�^���ׂ�ID CRC
//
//---------------------------------------------------------------------------
int FASTCALL FDIDisk::ReadID(DWORD *buf, BOOL mfm, int hd)
{
	FDITrack *track;
	DWORD pos;

	ASSERT(this);
	ASSERT(buf);
	ASSERT((hd == 0) || (hd == 4));

	// �g���b�N�擾
	if (hd == 0) {
		track = GetHead(0);
	}
	else {
		track = GetHead(1);
	}

	// NULL�Ȃ�NODATA
	if (!track) {
		// �����ɂ����鎞�Ԃ�ݒ�
		pos = GetRotationTime();
		pos = (pos * 2) - GetRotationPos();
		SetSearch(pos);
		return FDD_MAM | FDD_NODATA;
	}

	// �g���b�N�ɔC����
	return track->ReadID(buf, mfm);
}

//---------------------------------------------------------------------------
//
//	���[�h�Z�N�^
//	�����̃X�e�[�^�X��Ԃ�(�G���[��OR�����)
//		FDD_NOERROR		�G���[�Ȃ�
//		FDD_MAM			�w�薧�x�ł̓A���t�H�[�}�b�g
//		FDD_NODATA		�w�薧�x�ł̓A���t�H�[�}�b�g���A
//						�܂��͗L���ȃZ�N�^��S��������R����v���Ȃ�
//		FDD_NOCYL		��������ID��C����v�ł��AFF�łȂ��Z�N�^��������
//		FDD_BADCYL		��������ID��C����v�����AFF�ƂȂ��Ă���Z�N�^��������
//		FDD_IDCRC		ID�t�B�[���h��CRC�G���[������
//		FDD_DATACRC		DATA�t�B�[���h��CRC�G���[������
//		FDD_DDAM		�f���[�e�b�h�Z�N�^�ł���
//
//---------------------------------------------------------------------------
int FASTCALL FDIDisk::ReadSector(BYTE *buf, int *len, BOOL mfm, const DWORD *chrn, int hd)
{
	FDITrack *track;
	DWORD pos;

	ASSERT(this);
	ASSERT(len);
	ASSERT(chrn);
	ASSERT((hd == 0) || (hd == 4));

	// �g���b�N�擾
	if (hd == 0) {
		track = GetHead(0);
	}
	else {
		track = GetHead(1);
	}

	// NULL�Ȃ�NODATA
	if (!track) {
		// �����ɂ����鎞�Ԃ�ݒ�
		pos = GetRotationTime();
		pos = (pos * 2) - GetRotationPos();
		SetSearch(pos);
		return FDD_MAM | FDD_NODATA;
	}

	// �g���b�N�ɔC����
	return track->ReadSector(buf, len, mfm, chrn);
}

//---------------------------------------------------------------------------
//
//	���C�g�Z�N�^
//	�����̃X�e�[�^�X��Ԃ�(�G���[��OR�����)
//		FDD_NOERROR		�G���[�Ȃ�
//		FDD_NOTWRITE	���f�B�A�͏������݋֎~
//		FDD_MAM			�w�薧�x�ł̓A���t�H�[�}�b�g
//		FDD_NODATA		�w�薧�x�ł̓A���t�H�[�}�b�g���A
//						�܂��͗L���ȃZ�N�^��S��������R����v���Ȃ�
//		FDD_NOCYL		��������ID��C����v�ł��AFF�łȂ��Z�N�^��������
//		FDD_BADCYL		��������ID��C����v�����AFF�ƂȂ��Ă���Z�N�^��������
//		FDD_IDCRC		ID�t�B�[���h��CRC�G���[������
//		FDD_DDAM		�f���[�e�b�h�Z�N�^�ł���
//
//---------------------------------------------------------------------------
int FASTCALL FDIDisk::WriteSector(const BYTE *buf, int *len, BOOL mfm, const DWORD *chrn, int hd, BOOL deleted)
{
	FDITrack *track;
	DWORD pos;

	ASSERT(this);
	ASSERT(len);
	ASSERT(chrn);
	ASSERT((hd == 0) || (hd == 4));

	// �������݃`�F�b�N
	if (IsWriteP()) {
		return FDD_NOTWRITE;
	}

	// �g���b�N�擾
	if (hd == 0) {
		track = GetHead(0);
	}
	else {
		track = GetHead(1);
	}

	// NULL�Ȃ�NODATA
	if (!track) {
		// �����ɂ����鎞�Ԃ�ݒ�
		pos = GetRotationTime();
		pos = (pos * 2) - GetRotationPos();
		SetSearch(pos);
		return FDD_MAM | FDD_NODATA;
	}

	// �g���b�N�ɔC����
	return track->WriteSector(buf, len, mfm, chrn, deleted);
}

//---------------------------------------------------------------------------
//
//	���[�h�_�C�A�O
//	�����̃X�e�[�^�X��Ԃ�(�G���[��OR�����)
//		FDD_NOERROR		�G���[�Ȃ�
//		FDD_MAM			�w�薧�x�ł̓A���t�H�[�}�b�g
//		FDD_NODATA		�w�薧�x�ł̓A���t�H�[�}�b�g���A
//						�܂��͗L���ȃZ�N�^��S��������R����v���Ȃ�
//		FDD_IDCRC		ID�t�B�[���h��CRC�G���[������
//		FDD_DATACRC		�f�[�^�t�B�[���h��CRC�G���[������
//		FDD_DDAM		�f���[�e�b�h�Z�N�^�ł���
//
//---------------------------------------------------------------------------
int FASTCALL FDIDisk::ReadDiag(BYTE *buf, int *len, BOOL mfm, const DWORD *chrn, int hd)
{
	FDITrack *track;
	DWORD pos;

	ASSERT(this);
	ASSERT(len);
	ASSERT(chrn);
	ASSERT((hd == 0) || (hd == 4));

	// �g���b�N�擾
	if (hd == 0) {
		track = GetHead(0);
	}
	else {
		track = GetHead(1);
	}

	// NULL�Ȃ�NODATA
	if (!track) {
		// �����ɂ����鎞�Ԃ�ݒ�
		pos = GetRotationTime();
		pos = (pos * 2) - GetRotationPos();
		SetSearch(pos);
		return FDD_MAM | FDD_NODATA;
	}

	// �g���b�N�ɔC����
	return track->ReadDiag(buf, len, mfm, chrn);
}

//---------------------------------------------------------------------------
//
//	���C�gID
//	�����̃X�e�[�^�X��Ԃ�(�G���[��OR�����)
//		FDD_NOERROR		�G���[�Ȃ�
//		FDD_NOTWRITE	���f�B�A�͏������݋֎~
//
//---------------------------------------------------------------------------
int FASTCALL FDIDisk::WriteID(const BYTE *buf, DWORD d, int sc, BOOL mfm, int hd, int gpl)
{
	FDITrack *track;

	ASSERT(this);
	ASSERT(sc > 0);

	// �������݃`�F�b�N
	if (IsWriteP()) {
		return FDD_NOTWRITE;
	}

	// �g���b�N�擾
	if (hd == 0) {
		track = GetHead(0);
	}
	else {
		track = GetHead(1);
	}

	// NULL�Ȃ�No Error�Ƃ���(format.x v2.20��154�g���b�N��Write ID)
	if (!track) {
		return FDD_NOERROR;
	}

	// �g���b�N�ɔC����
	return track->WriteID(buf, d, sc, mfm, gpl);
}

//---------------------------------------------------------------------------
//
//	��]�ʒu�擾
//
//---------------------------------------------------------------------------
DWORD FASTCALL FDIDisk::GetRotationPos() const
{
	ASSERT(this);
	ASSERT(GetFDI());

	// �e�ɕ���
	return GetFDI()->GetRotationPos();
}

//---------------------------------------------------------------------------
//
//	��]���Ԏ擾
//
//---------------------------------------------------------------------------
DWORD FASTCALL FDIDisk::GetRotationTime() const
{
	ASSERT(this);

	ASSERT(GetFDI());

	// �e�ɕ���
	return GetFDI()->GetRotationTime();
}

//---------------------------------------------------------------------------
//
//	���������Z�o
//
//---------------------------------------------------------------------------
void FASTCALL FDIDisk::CalcSearch(DWORD pos)
{
	DWORD cur;
	DWORD hus;

	ASSERT(this);

	// �擾
	cur = GetRotationPos();
	hus = GetRotationTime();

	// �J�����g<�|�W�V�����Ȃ�A�����o���̂�
	if (cur < pos) {
		SetSearch(pos - cur);
		return;
	}

	// �|�W�V���� < �J�����g�́A�P���𒴂��Ă���
	ASSERT(cur <= hus);
	pos += (hus - cur);
	SetSearch(pos);
}

//---------------------------------------------------------------------------
//
//	HD�t���O�擾
//
//---------------------------------------------------------------------------
BOOL FASTCALL FDIDisk::IsHD() const
{
	ASSERT(this);
	ASSERT(GetFDI());

	// �e�ɕ���
	return GetFDI()->IsHD();
}

//---------------------------------------------------------------------------
//
//	�g���b�N�ǉ�
//
//---------------------------------------------------------------------------
void FASTCALL FDIDisk::AddTrack(FDITrack *track)
{
	FDITrack *ptr;

	ASSERT(this);
	ASSERT(track);

	// �g���b�N�������Ă��Ȃ���΁A���̂܂ܒǉ�
	if (!disk.first) {
		disk.first = track;
		disk.first->SetNext(NULL);
		return;
	}

	// �ŏI�g���b�N�𓾂�
	ptr = disk.first;
	while (ptr->GetNext()) {
		ptr = ptr->GetNext();
	}

	// �ŏI�g���b�N�ɒǉ�
	ptr->SetNext(track);
	track->SetNext(NULL);
}

//---------------------------------------------------------------------------
//
//	�g���b�N�S�폜
//
//---------------------------------------------------------------------------
void FASTCALL FDIDisk::ClrTrack()
{
	FDITrack *track;

	ASSERT(this);

	// �g���b�N�����ׂč폜
	while (disk.first) {
		track = disk.first->GetNext();
		delete disk.first;
		disk.first = track;
	}
}

//---------------------------------------------------------------------------
//
//	�g���b�N�T�[�`
//
//---------------------------------------------------------------------------
FDITrack* FASTCALL FDIDisk::Search(int track) const
{
	FDITrack *p;

	ASSERT(this);
	ASSERT((track >= 0) && (track <= 163));

	// �ŏ��̃g���b�N���擾
	p = GetFirst();

	// ���[�v
	while (p) {
		if (p->GetTrack() == track) {
			return p;
		}
		p = p->GetNext();
	}

	// ������Ȃ�
	return NULL;
}

//===========================================================================
//
//	FDI
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	�R���X�g���N�^
//
//---------------------------------------------------------------------------
FDI::FDI(FDD *fdd)
{
	ASSERT(fdd);

	// ���[�N������
	fdi.fdd = fdd;
	fdi.disks = 0;
	fdi.first = NULL;
	fdi.disk = NULL;
}

//---------------------------------------------------------------------------
//
//	�f�X�g���N�^
//
//---------------------------------------------------------------------------
FDI::~FDI()
{
	ClrDisk();
}

//---------------------------------------------------------------------------
//
//	�I�[�v��
//	����ʃN���X�̒��ӓ_�F
//		�Ăяo������ŁA���݂̃V�����_�փV�[�N����
//
//---------------------------------------------------------------------------
BOOL FASTCALL FDI::Open(const Filepath& path, int media = 0)
{
	FDIDisk2HD *disk2hd;
	FDIDiskDIM *diskdim;
	FDIDiskD68 *diskd68;
	FDIDisk2DD *disk2dd;
	FDIDisk2HQ *disk2hq;
	FDIDiskBAD *diskbad;
	int i;
	int num;
	DWORD offset[0x10];

	ASSERT(this);
	ASSERT((media >= 0) && (media < 0x10));

	// ���ɃI�[�v������Ă���Ȃ炨������
	ASSERT(!GetDisk());
	ASSERT(!GetFirst());
	ASSERT(fdi.disks == 0);

	// DIM�t�@�C���Ƃ��ăg���C
	diskdim = new FDIDiskDIM(0, this);
	if (diskdim->Open(path, 0)) {
		AddDisk(diskdim);
		fdi.disk = diskdim;
		return TRUE;
	}
	// ���s
	delete diskdim;

	// D68�t�@�C���Ƃ��ăg���C(�����𐔂���)
	num = FDIDiskD68::CheckDisks(path, offset);
	if (num > 0) {
		// D68�f�B�X�N�쐬���[�v(�����������ǉ�)
		for (i=0; i<num; i++) {
			diskd68 = new FDIDiskD68(i, this);
			if (!diskd68->Open(path, offset[i])) {
				// ���s
				delete diskd68;
				ClrDisk();
				return FALSE;
			}
			AddDisk(diskd68);
		}
		// ���f�B�A�Z���N�g
		fdi.disk = Search(media);
		if (!fdi.disk) {
			ClrDisk();
			return FALSE;
		}
		return TRUE;
	}

	// 2HD�t�@�C���Ƃ��ăg���C
	disk2hd = new FDIDisk2HD(0, this);
	if (disk2hd->Open(path, 0)) {
		AddDisk(disk2hd);
		fdi.disk = disk2hd;
		return TRUE;
	}
	// ���s
	delete disk2hd;

	// 2DD�t�@�C���Ƃ��ăg���C
	disk2dd = new FDIDisk2DD(0, this);
	if (disk2dd->Open(path, 0)) {
		AddDisk(disk2dd);
		fdi.disk = disk2dd;
		return TRUE;
	}
	// ���s
	delete disk2dd;

	// 2HQ�t�@�C���Ƃ��ăg���C
	disk2hq = new FDIDisk2HQ(0, this);
	if (disk2hq->Open(path, 0)) {
		AddDisk(disk2hq);
		fdi.disk = disk2hq;
		return TRUE;
	}
	// ���s
	delete disk2hq;

	// BAD�t�@�C���Ƃ��ăg���C
	diskbad = new FDIDiskBAD(0, this);
	if (diskbad->Open(path, 0)) {
		AddDisk(diskbad);
		fdi.disk = diskbad;
		return TRUE;
	}
	// ���s
	delete diskbad;

	// �G���[
	return FALSE;
}

//---------------------------------------------------------------------------
//
//	ID�擾
//
//---------------------------------------------------------------------------
DWORD FASTCALL FDI::GetID() const
{
	ASSERT(this);

	// �m�b�g���f�B�Ȃ�NULL
	if (!IsReady()) {
		return MAKEID('N', 'U', 'L', 'L');
	}

	// �f�B�X�N�ɕ���
	return GetDisk()->GetID();
}

//---------------------------------------------------------------------------
//
//	�}���`�f�B�X�N��
//
//---------------------------------------------------------------------------
BOOL FASTCALL FDI::IsMulti() const
{
	ASSERT(this);

	// �f�B�X�N��2���ȏ�Ȃ�TRUE
	if (GetDisks() >= 2) {
		return TRUE;
	}
	return FALSE;
}

//---------------------------------------------------------------------------
//
//	���f�B�A�ԍ����擾
//
//---------------------------------------------------------------------------
int FASTCALL FDI::GetMedia() const
{
	ASSERT(this);

	// �f�B�X�N���Ȃ����0
	if (!GetDisk()) {
		return 0;
	}

	// �f�B�X�N�ɕ�������
	return GetDisk()->GetIndex();
}

//---------------------------------------------------------------------------
//
//	�f�B�X�N���擾
//
//---------------------------------------------------------------------------
void FASTCALL FDI::GetName(char *buf, int index) const
{
	FDIDisk *disk;

	ASSERT(this);
	ASSERT(buf);
	ASSERT(index >= -1);
	ASSERT(index < GetDisks());

	// -1�̎��́A�J�����g���Ӗ�����
	if (index < 0) {
		// �m�b�g���f�B�Ȃ�A�󕶎���
		if (!IsReady()) {
			buf[0] = '\0';
			return;
		}

		// �J�����g�ɕ���
		GetDisk()->GetName(buf);
		return;
	}

	// �C���f�b�N�X���Ȃ̂ŁA����
	disk = Search(index);
	if (!disk) {
		buf[0] = '\0';
		return;
	}
	disk->GetName(buf);
}

//---------------------------------------------------------------------------
//
//	�p�X�擾
//
//---------------------------------------------------------------------------
void FASTCALL FDI::GetPath(Filepath& path) const
{
	ASSERT(this);

	// �m�b�g���f�B�Ȃ�A�󕶎���
	if (!IsReady()) {
		path.Clear();
		return;
	}

	// �f�B�X�N�ɕ���
	GetDisk()->GetPath(path);
}

//---------------------------------------------------------------------------
//
//	���f�B�`�F�b�N
//
//---------------------------------------------------------------------------
BOOL FASTCALL FDI::IsReady() const
{
	ASSERT(this);

	// �J�����g���f�B�A�������TRUE�A�Ȃ����FALSE
	if (GetDisk()) {
		return TRUE;
	}
	return FALSE;
}

//---------------------------------------------------------------------------
//
//	�������݋֎~��
//
//---------------------------------------------------------------------------
BOOL FASTCALL FDI::IsWriteP() const
{
	// �m�b�g���f�B�Ȃ�FALSE
	if (!IsReady()) {
		return FALSE;
	}

	// �f�B�X�N�ɕ���
	return GetDisk()->IsWriteP();
}

//---------------------------------------------------------------------------
//
//	Read Only�f�B�X�N�C���[�W��
//
//---------------------------------------------------------------------------
BOOL FASTCALL FDI::IsReadOnly() const
{
	// �m�b�g���f�B�Ȃ�FALSE
	if (!IsReady()) {
		return FALSE;
	}

	// �f�B�X�N�ɕ���
	return GetDisk()->IsReadOnly();
}

//---------------------------------------------------------------------------
//
//	�������݋֎~�Z�b�g
//
//---------------------------------------------------------------------------
void FASTCALL FDI::WriteP(BOOL flag)
{
	ASSERT(this);

	// ���f�B�Ȃ�A�w��
	if (IsReady()) {
		GetDisk()->WriteP(flag);
	}
}

//---------------------------------------------------------------------------
//
//	�V�[�N
//
//---------------------------------------------------------------------------
void FASTCALL FDI::Seek(int c)
{
	ASSERT(this);
	ASSERT((c >= 0) && (c < 82));

	// �m�b�g���f�B�Ȃ牽�����Ȃ�
	if (!IsReady()) {
		return;
	}

	// �f�B�X�N�ɒʒm
	GetDisk()->Seek(c);
}

//---------------------------------------------------------------------------
//
//	���[�hID
//	�����̃X�e�[�^�X��Ԃ�(�G���[��OR�����)
//		FDD_NOERROR		�G���[�Ȃ�
//		FDD_NOTREADY	�m�b�g���f�B
//		FDD_MAM			�w�薧�x�ł̓A���t�H�[�}�b�g
//		FDD_NODATA		�w�薧�x�ł̓A���t�H�[�}�b�g���A
//						�܂��͗L���ȃZ�N�^���ׂ�ID CRC
//
//---------------------------------------------------------------------------
int FASTCALL FDI::ReadID(DWORD *buf, BOOL mfm, int hd)
{
	ASSERT(this);
	ASSERT(buf);
	ASSERT((hd == 0) || (hd == 4));

	// �m�b�g���f�B����
	if (!IsReady()) {
		return FDD_NOTREADY;
	}

	// �f�B�X�N�ɔC����
	return GetDisk()->ReadID(buf, mfm, hd);
}

//---------------------------------------------------------------------------
//
//	���[�h�Z�N�^
//	�����̃X�e�[�^�X��Ԃ�(�G���[��OR�����)
//		FDD_NOERROR		�G���[�Ȃ�
//		FDD_NOTREADY	�m�b�g���f�B
//		FDD_MAM			�w�薧�x�ł̓A���t�H�[�}�b�g
//		FDD_NODATA		�w�薧�x�ł̓A���t�H�[�}�b�g���A
//						�܂��͗L���ȃZ�N�^��S��������R����v���Ȃ�
//		FDD_NOCYL		��������ID��C����v�ł��AFF�łȂ��Z�N�^��������
//		FDD_BADCYL		��������ID��C����v�����AFF�ƂȂ��Ă���Z�N�^��������
//		FDD_IDCRC		ID�t�B�[���h��CRC�G���[������
//		FDD_DATACRC		DATA�t�B�[���h��CRC�G���[������
//		FDD_DDAM		�f���[�e�b�h�Z�N�^�ł���
//
//---------------------------------------------------------------------------
int FASTCALL FDI::ReadSector(BYTE *buf, int *len, BOOL mfm, const DWORD *chrn, int hd)
{
	ASSERT(this);
	ASSERT(len);
	ASSERT(chrn);
	ASSERT((hd == 0) || (hd == 4));

	// �m�b�g���f�B����
	if (!IsReady()) {
		return FDD_NOTREADY;
	}

	// �f�B�X�N�ɔC����
	return GetDisk()->ReadSector(buf, len, mfm, chrn, hd);
}

//---------------------------------------------------------------------------
//
//	���C�g�Z�N�^
//	�����̃X�e�[�^�X��Ԃ�(�G���[��OR�����)
//		FDD_NOERROR		�G���[�Ȃ�
//		FDD_NOTREADY	�m�b�g���f�B
//		FDD_NOTWRITE	���f�B�A�͏������݋֎~
//		FDD_MAM			�w�薧�x�ł̓A���t�H�[�}�b�g
//		FDD_NODATA		�w�薧�x�ł̓A���t�H�[�}�b�g���A
//						�܂��͗L���ȃZ�N�^��S��������R����v���Ȃ�
//		FDD_NOCYL		��������ID��C����v�ł��AFF�łȂ��Z�N�^��������
//		FDD_BADCYL		��������ID��C����v�����AFF�ƂȂ��Ă���Z�N�^��������
//		FDD_IDCRC		ID�t�B�[���h��CRC�G���[������
//		FDD_DDAM		�f���[�e�b�h�Z�N�^�ł���
//
//---------------------------------------------------------------------------
int FASTCALL FDI::WriteSector(const BYTE *buf, int *len, BOOL mfm, const DWORD *chrn, int hd, BOOL deleted)
{
	ASSERT(this);
	ASSERT(len);
	ASSERT(chrn);
	ASSERT((hd == 0) || (hd == 4));

	// �m�b�g���f�B����
	if (!IsReady()) {
		return FDD_NOTREADY;
	}

	// �f�B�X�N�ɔC����
	return GetDisk()->WriteSector(buf, len, mfm, chrn, hd, deleted);
}

//---------------------------------------------------------------------------
//
//	���[�h�_�C�A�O
//	�����̃X�e�[�^�X��Ԃ�(�G���[��OR�����)
//		FDD_NOERROR		�G���[�Ȃ�
//		FDD_MAM			�w�薧�x�ł̓A���t�H�[�}�b�g
//		FDD_NODATA		�w�薧�x�ł̓A���t�H�[�}�b�g���A
//						�܂��͗L���ȃZ�N�^��S��������R����v���Ȃ�
//		FDD_IDCRC		ID�t�B�[���h��CRC�G���[������
//		FDD_DATACRC		�f�[�^�t�B�[���h��CRC�G���[������
//		FDD_DDAM		�f���[�e�b�h�Z�N�^�ł���
//
//---------------------------------------------------------------------------
int FASTCALL FDI::ReadDiag(BYTE *buf, int *len, BOOL mfm, const DWORD *chrn, int hd)
{
	ASSERT(this);
	ASSERT(len);
	ASSERT(chrn);
	ASSERT((hd == 0) || (hd == 4));

	// �m�b�g���f�B����
	if (!IsReady()) {
		return FDD_NOTREADY;
	}

	// �f�B�X�N�ɔC����
	return GetDisk()->ReadDiag(buf, len, mfm, chrn, hd);
}

//---------------------------------------------------------------------------
//
//	���C�gID
//	�����̃X�e�[�^�X��Ԃ�(�G���[��OR�����)
//		FDD_NOERROR		�G���[�Ȃ�
//		FDD_NOTREADY	�m�b�g���f�B
//		FDD_NOTWRITE	���f�B�A�͏������݋֎~
//
//---------------------------------------------------------------------------
int FASTCALL FDI::WriteID(const BYTE *buf, DWORD d, int sc, BOOL mfm, int hd, int gpl)
{
	ASSERT(this);
	ASSERT(sc > 0);
	ASSERT((hd == 0) || (hd == 4));

	// �m�b�g���f�B����
	if (!IsReady()) {
		return FDD_NOTREADY;
	}

	// �f�B�X�N�ɔC����
	return GetDisk()->WriteID(buf, d, sc, mfm, hd, gpl);
}

//---------------------------------------------------------------------------
//
//	��]�ʒu�擾
//
//---------------------------------------------------------------------------
DWORD FASTCALL FDI::GetRotationPos() const
{
	ASSERT(this);
	ASSERT(GetFDD());

	// �e�ɕ���
	return GetFDD()->GetRotationPos();
}

//---------------------------------------------------------------------------
//
//	��]���Ԏ擾
//
//---------------------------------------------------------------------------
DWORD FASTCALL FDI::GetRotationTime() const
{
	ASSERT(this);
	ASSERT(GetFDD());

	// �e�ɕ���
	return GetFDD()->GetRotationTime();
}

//---------------------------------------------------------------------------
//
//	�������Ԏ擾
//
//---------------------------------------------------------------------------
DWORD FASTCALL FDI::GetSearch() const
{
	FDIDisk *disk;

	ASSERT(this);

	// �m�b�g���f�B�Ȃ���0
	disk = GetDisk();
	if (!disk) {
		return 0;
	}

	// �f�B�X�N�ɕ���
	return disk->GetSearch();
}

//---------------------------------------------------------------------------
//
//	HD�t���O�擾
//
//---------------------------------------------------------------------------
BOOL FASTCALL FDI::IsHD() const
{
	ASSERT(this);
	ASSERT(GetFDD());

	// �e�ɕ���
	return GetFDD()->IsHD();
}

//---------------------------------------------------------------------------
//
//	�f�B�X�N�ǉ�
//
//---------------------------------------------------------------------------
void FASTCALL FDI::AddDisk(FDIDisk *disk)
{
	FDIDisk *ptr;

	ASSERT(this);
	ASSERT(disk);

	// �f�B�X�N�������Ă��Ȃ���΁A���̂܂ܒǉ�
	if (!fdi.first) {
		fdi.first = disk;
		fdi.first->SetNext(NULL);

		// �J�E���^Up
		fdi.disks++;
		return;
	}

	// �ŏI�f�B�X�N�𓾂�
	ptr = fdi.first;
	while (ptr->GetNext()) {
		ptr = ptr->GetNext();
	}

	// �ŏI�f�B�X�N�ɒǉ�
	ptr->SetNext(disk);
	disk->SetNext(NULL);

	// �J�E���^Up
	fdi.disks++;
}

//---------------------------------------------------------------------------
//
//	�f�B�X�N�S�폜
//
//---------------------------------------------------------------------------
void FASTCALL FDI::ClrDisk()
{
	FDIDisk *disk;

	ASSERT(this);

	// �f�B�X�N�����ׂč폜
	while (fdi.first) {
		disk = fdi.first->GetNext();
		delete fdi.first;
		fdi.first = disk;
	}

	// �J�E���^0
	fdi.disks = 0;
}

//---------------------------------------------------------------------------
//
//	�f�B�X�N����
//
//---------------------------------------------------------------------------
FDIDisk* FASTCALL FDI::Search(int index) const
{
	FDIDisk *disk;

	ASSERT(this);
	ASSERT(index >= 0);
	ASSERT(index < GetDisks());

	// �ŏ��̃f�B�X�N���擾
	disk = GetFirst();

	// ��r���[�v
	while (disk) {
		if (disk->GetIndex() == index) {
			return disk;
		}

		// ����
		disk = disk->GetNext();
	}

	// ������Ȃ�
	return NULL;
}

//---------------------------------------------------------------------------
//
//	�Z�[�u
//
//---------------------------------------------------------------------------
BOOL FASTCALL FDI::Save(Fileio *fio, int ver)
{
	BOOL ready;
	FDIDisk *disk;
	Filepath path;
	int i;
	int disks;
	int media;

	ASSERT(this);
	ASSERT(fio);

	// ���f�B�t���O����������
	ready = IsReady();
	if (!fio->Write(&ready, sizeof(ready))) {
		return FALSE;
	}

	// ���f�B�łȂ���ΏI��
	if (!ready) {
		return TRUE;
	}

	// �S���f�B�A���t���b�V��
	disks = GetDisks();
	for (i=0; i<disks; i++) {
		disk = Search(i);
		ASSERT(disk);
		if (!disk->Flush()) {
			return FALSE;
		}
	}

	// ���f�B�A����������
	media = GetMedia();
	if (!fio->Write(&media, sizeof(media))) {
		return FALSE;
	}

	// �p�X����������
	GetPath(path);
	if (!path.Save(fio, ver)) {
		return FALSE;
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	���[�h
//
//---------------------------------------------------------------------------
BOOL FASTCALL FDI::Load(Fileio *fio, int ver, BOOL *ready, int *media, Filepath& path)
{
	ASSERT(this);
	ASSERT(fio);
	ASSERT(ready);
	ASSERT(media);
	ASSERT(!IsReady());
	ASSERT(GetDisks() == 0);

	// ���f�B�t���O��ǂݍ���
	if (!fio->Read(ready, sizeof(BOOL))) {
		return FALSE;
	}

	// ���f�B�łȂ���ΏI��
	if (!(*ready)) {
		return TRUE;
	}

	// ���f�B�A��ǂݍ���
	if (!fio->Read(media, sizeof(int))) {
		return FALSE;
	}

	// �p�X��ǂݍ���
	if (!path.Load(fio, ver)) {
		return FALSE;
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	����(����)
//
//---------------------------------------------------------------------------
void FASTCALL FDI::Adjust()
{
	FDIDisk *disk;
	FDIDiskD68 *disk68;

	ASSERT(this);

	// �����C���[�W�̏ꍇ�̂�
	if (!IsMulti()) {
		return;
	}

	disk = GetFirst();
	while (disk) {
		// D68�̏ꍇ�̂�
		if (disk->GetID() == MAKEID('D', '6', '8', ' ')) {
			disk68 = (FDIDiskD68*)disk;
			disk68->AdjustOffset();
		}

		disk = disk->GetNext();
	}
}

//===========================================================================
//
//	FDI�g���b�N(2HD)
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	�R���X�g���N�^
//
//---------------------------------------------------------------------------
FDITrack2HD::FDITrack2HD(FDIDisk *disk, int track) : FDITrack(disk, track)
{
	ASSERT(disk);
}

//---------------------------------------------------------------------------
//
//	���[�h
//
//---------------------------------------------------------------------------
BOOL FASTCALL FDITrack2HD::Load(const Filepath& path, DWORD offset)
{
	Fileio fio;
	BYTE buf[0x400];
	DWORD chrn[4];
	int i;
	FDISector *sector;

	ASSERT(this);
	ASSERT((offset & 0x1fff) == 0);
	ASSERT(offset < 0x134000);

	// �������ς݂Ȃ�s�v(�V�[�N���ɌĂ΂��̂ŁA�P�x�����ǂ�ŃL���b�V������)
	if (IsInit()) {
		return TRUE;
	}

	// �Z�N�^�����݂��Ȃ�����
	ASSERT(!GetFirst());
	ASSERT(GetAllSectors() == 0);
	ASSERT(GetMFMSectors() == 0);
	ASSERT(GetFMSectors() == 0);

	// C�EH�EN����
	chrn[0] = GetTrack() >> 1;
	chrn[1] = GetTrack() & 1;
	chrn[3] = 3;

	// �ǂݍ��݃I�[�v��
	if (!fio.Open(path, Fileio::ReadOnly)) {
		return FALSE;
	}

	// �V�[�N
	if (!fio.Seek(offset)) {
		fio.Close();
		return FALSE;
	}

	// ���[�v
	for (i=0; i<8; i++) {
		// �f�[�^�ǂݍ���
		if (!fio.Read(buf, sizeof(buf))) {
			// �r���܂Œǉ����������폜����
			ClrSector();
			fio.Close();
			return FALSE;
		}

		// �Z�N�^�쐬
		chrn[2] = i + 1;
		sector = new FDISector(TRUE, chrn);
		sector->Load(buf, sizeof(buf), 0x74, FDD_NOERROR);

		// �Z�N�^�ǉ�
		AddSector(sector);
	}

	// �N���[�Y
	fio.Close();

	// �|�W�V�����v�Z
	CalcPos();

	// ������ok
	trk.init = TRUE;
	return TRUE;
}

//===========================================================================
//
//	FDI�f�B�X�N(2HD)
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	�R���X�g���N�^
//
//---------------------------------------------------------------------------
FDIDisk2HD::FDIDisk2HD(int index, FDI *fdi) : FDIDisk(index, fdi)
{
	// ID�ݒ�
	disk.id = MAKEID('2', 'H', 'D', ' ');
}

//---------------------------------------------------------------------------
//
//	�f�X�g���N�^
//
//---------------------------------------------------------------------------
FDIDisk2HD::~FDIDisk2HD()
{
	int i;
	DWORD offset;
	FDITrack *track;

	// �Ō�̃g���b�N�f�[�^����������
	for (i=0; i<2; i++) {
		// �g���b�N�����邩
		track = GetHead(i);
		if (!track) {
			continue;
		}

		// �g���b�N�i���o����A�I�t�Z�b�g���Z�o(x8KB)
		offset = track->GetTrack();
		offset <<= 13;

		// ��������
		track->Save(disk.path, offset);
		disk.head[i] = NULL;
	}
}

//---------------------------------------------------------------------------
//
//	�I�[�v��
//
//---------------------------------------------------------------------------
BOOL FASTCALL FDIDisk2HD::Open(const Filepath& path, DWORD offset)
{
	Fileio fio;
	DWORD size;
	FDITrack2HD *track;
	int i;

	ASSERT(this);
	ASSERT(offset == 0);
	ASSERT(!GetFirst());
	ASSERT(!GetHead(0));
	ASSERT(!GetHead(1));

	// �������݉\�Ƃ��ď�����
	disk.writep = FALSE;
	disk.readonly = FALSE;

	// �I�[�v���ł��邱�Ƃ��m���߂�
	if (!fio.Open(path, Fileio::ReadWrite)) {
		// �ǂݍ��݃I�[�v�������݂�
		if (!fio.Open(path, Fileio::ReadOnly)) {
			return FALSE;
		}

		// �ǂݍ��݂͉
		disk.writep = TRUE;
		disk.readonly = TRUE;
	}

	// �t�@�C���T�C�Y��1261568�ł��邱�Ƃ��m���߂�
	size = fio.GetFileSize();
	if (size != 1261568) {
		fio.Close();
		return FALSE;
	}
	fio.Close();

	// �p�X�A�I�t�Z�b�g���L��
	disk.path = path;
	disk.offset = offset;

	// �f�B�X�N���̓t�@�C�����{�g���q�Ƃ���
	strcpy(disk.name, path.GetShort());

	// �g���b�N���쐬(0�`76�V�����_�܂ŁA77*2�g���b�N)
	for (i=0; i<154; i++) {
		track = new FDITrack2HD(this, i);
		AddTrack(track);
	}

	// �I��
	return TRUE;
}

//---------------------------------------------------------------------------
//
//	�V�[�N
//
//---------------------------------------------------------------------------
void FASTCALL FDIDisk2HD::Seek(int c)
{
	int i;
	FDITrack2HD *track;
	DWORD offset;

	ASSERT(this);
	ASSERT((c >= 0) && (c < 82));

	// �g���b�N�f�[�^����������
	for (i=0; i<2; i++) {
		// �g���b�N�����邩
		track = (FDITrack2HD*)GetHead(i);
		if (!track) {
			continue;
		}

		// �g���b�N�i���o����A�I�t�Z�b�g���Z�o(x8KB)
		offset = track->GetTrack();
		offset <<= 13;

		// ��������
		track->Save(disk.path, offset);
	}

	// c��75�܂ŋ��B�͈͊O�ł����head[i]=NULL�Ƃ���
	if ((c < 0) || (c > 76)) {
		disk.head[0] = NULL;
		disk.head[1] = NULL;
		return;
	}

	// �Y������g���b�N���������A���[�h
	for (i=0; i<2; i++) {
		// �g���b�N������
		track = (FDITrack2HD*)Search(c * 2 + i);
		ASSERT(track);
		disk.head[i] = track;

		// �g���b�N�i���o����A�I�t�Z�b�g���Z�o(x8KB)
		offset = track->GetTrack();
		offset <<= 13;

		// ���[�h
		track->Load(disk.path, offset);
	}
}

//---------------------------------------------------------------------------
//
//	�V�K�f�B�X�N�쐬
//
//---------------------------------------------------------------------------
BOOL FASTCALL FDIDisk2HD::Create(const Filepath& path, const option_t *opt)
{
	int i;
	FDITrack2HD *track;
	DWORD offset;
	Fileio fio;

	ASSERT(this);
	ASSERT(opt);

	// �����t�H�[�}�b�g��2HD�̂݋���
	if (opt->phyfmt != FDI_2HD) {
		return FALSE;
	}

	// �t�@�C���쐬�����݂�
	if (!fio.Open(path, Fileio::WriteOnly)) {
		return FALSE;
	}

	// �������݉\�Ƃ��ď�����
	disk.writep = FALSE;
	disk.readonly = FALSE;

	// �p�X���A�I�t�Z�b�g���L�^
	disk.path = path;
	disk.offset = 0;

	// �f�B�X�N���̓t�@�C�����{�g���q�Ƃ���
	strcpy(disk.name, path.GetShort());

	// 0�`153�Ɍ���A�g���b�N���쐬���ĕ����t�H�[�}�b�g
	for (i=0; i<154; i++) {
		track = new FDITrack2HD(this, i);
		track->Create(opt->phyfmt);
		AddTrack(track);
	}

	// �_���t�H�[�}�b�g
	FDIDisk::Create(path, opt);

	// �������݃��[�v
	offset = 0;
	for (i=0; i<154; i++) {
		// �g���b�N�擾
		track = (FDITrack2HD*)Search(i);
		ASSERT(track);
		ASSERT(track->IsChanged());

		// ��������
		if (!track->Save(&fio, offset)) {
			fio.Close();
			return FALSE;
		}

		// ����
		offset += (0x400 * 8);
	}

	// ����
	fio.Close();
	return TRUE;
}

//---------------------------------------------------------------------------
//
//	�t���b�V��
//
//---------------------------------------------------------------------------
BOOL FASTCALL FDIDisk2HD::Flush()
{
	int i;
	DWORD offset;
	FDITrack *track;

	ASSERT(this);

	// �Ō�̃g���b�N�f�[�^����������
	for (i=0; i<2; i++) {
		// �g���b�N�����邩
		track = GetHead(i);
		if (!track) {
			continue;
		}

		// �g���b�N�i���o����A�I�t�Z�b�g���Z�o(x8KB)
		offset = track->GetTrack();
		offset <<= 13;

		// ��������
		if (!track->Save(disk.path, offset)) {
			return FALSE;
		}
	}

	return TRUE;
}

//===========================================================================
//
//	FDI�g���b�N(DIM)
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	�R���X�g���N�^
//
//---------------------------------------------------------------------------
FDITrackDIM::FDITrackDIM(FDIDisk *disk, int track, int type) : FDITrack(disk, track)
{
	// �^�C�v�ɉ�����dim_mfm, dim_secs, dim_n�����߂�
	switch (type) {
		// 2HD (N=3,8�Z�N�^)
		case 0:
			dim_mfm = TRUE;
			dim_secs = 8;
			dim_n = 3;
			break;

		// 2HS (N=3,9�Z�N�^)
		case 1:
			dim_mfm = TRUE;
			dim_secs = 9;
			dim_n = 3;
			break;

		// 2HC (N=2,15�Z�N�^)
		case 2:
			dim_mfm = TRUE;
			dim_secs = 15;
			dim_n = 2;
			break;

		// 2HDE (N=3,9�Z�N�^)
		case 3:
			dim_mfm = TRUE;
			dim_secs = 9;
			dim_n = 3;
			break;

		// 2HQ (N=2,18�Z�N�^)
		case 9:
			dim_mfm = TRUE;
			dim_secs = 18;
			dim_n = 2;
			break;

		// N88-BASIC (26�Z�N�^�A�g���b�N0�̂ݒP��)
		case 17:
			dim_secs = 26;
			if (track == 0) {
				dim_mfm = FALSE;
				dim_n = 0;
			}
			else {
				dim_mfm = TRUE;
				dim_n = 1;
			}
			break;

		// ���̑�
		default:
			ASSERT(FALSE);
			break;
	}

	dim_type = type;
}

//---------------------------------------------------------------------------
//
//	���[�h
//
//---------------------------------------------------------------------------
BOOL FASTCALL FDITrackDIM::Load(const Filepath& path, DWORD offset, BOOL load)
{
	Fileio fio;
	BYTE buf[0x400];
	DWORD chrn[4];
	int i;
	int num;
	int len;
	int gap;
	FDISector *sector;

	ASSERT(this);

	// �������ς݂Ȃ�s�v(�V�[�N���ɌĂ΂��̂ŁA�P�x�����ǂ�ŃL���b�V������)
	if (IsInit()) {
		return TRUE;
	}

	// �Z�N�^�����݂��Ȃ�����
	ASSERT(!GetFirst());
	ASSERT(GetAllSectors() == 0);
	ASSERT(GetMFMSectors() == 0);
	ASSERT(GetFMSectors() == 0);

	// C�EH�EN����
	chrn[0] = GetTrack() >> 1;
	chrn[3] = GetDIMN();

	// �ǂݍ��݃I�[�v��
	if (load) {
		if (!fio.Open(path, Fileio::ReadOnly)) {
			return FALSE;
		}

		// �V�[�N
		if (!fio.Seek(offset)) {
			fio.Close();
			return FALSE;
		}
	}

	// ���[�h����
	num = GetDIMSectors();
	len = 1 << (GetDIMN() + 7);
	ASSERT(len <= sizeof(buf));
	switch (GetDIMN()) {
		case 0:
			gap = 0x14;
			break;
		case 1:
			gap = 0x33;
			break;
		case 2:
			gap = 0x54;
			break;
		case 3:
			if (GetDIMSectors() == 8) {
				gap = 0x74;
			}
			else {
				gap = 0x39;
			}
			break;
		default:
			ASSERT(FALSE);
			fio.Close();
			return FALSE;
	}

	// �����f�[�^�쐬(load==FALSE�̏ꍇ)
	memset(buf, 0xe5, len);

	// ���[�v
	for (i=0; i<num; i++) {
		// �f�[�^�ǂݍ���
		if (load) {
			if (!fio.Read(buf, len)) {
				// �r���܂Œǉ����������폜����
				ClrSector();
				fio.Close();
				return FALSE;
			}
		}

		// H��R�����߂� (2HS, 2HDE�œ��Ⴀ��)
		chrn[1] = GetTrack() & 1;
		chrn[2] = i + 1;
		if (dim_type == 1) {
			chrn[2] = i + 10;
			if ((GetTrack() == 0) && (i == 0)) {
				chrn[2] = 1;
			}
		}
		if (dim_type == 3) {
			chrn[1] = chrn[1] + 0x80;
			if ((GetTrack() == 0) && (i == 0)) {
				chrn[1] = 0;
			}
		}

		// �Z�N�^�쐬
		sector = new FDISector(IsDIMMFM(), chrn);
		sector->Load(buf, len, gap, FDD_NOERROR);

		// �Z�N�^�ǉ�
		AddSector(sector);
	}

	// �N���[�Y
	if (load) {
		fio.Close();
	}

	// �|�W�V�����v�Z
	CalcPos();

	// ������ok
	trk.init = TRUE;
	return TRUE;
}

//===========================================================================
//
//	FDI�f�B�X�N(DIM)
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	�R���X�g���N�^
//
//---------------------------------------------------------------------------
FDIDiskDIM::FDIDiskDIM(int index, FDI *fdi) : FDIDisk(index, fdi)
{
	// ID�ݒ�
	disk.id = MAKEID('D', 'I', 'M', ' ');

	// �w�b�_���N���A
	memset(dim_hdr, 0, sizeof(dim_hdr));

	// ���[�h�Ȃ�
	dim_load = FALSE;
}

//---------------------------------------------------------------------------
//
//	�f�X�g���N�^
//
//---------------------------------------------------------------------------
FDIDiskDIM::~FDIDiskDIM()
{
	// ���[�h����Ă���Ώ�������
	if (dim_load) {
		Save();
	}
}

//---------------------------------------------------------------------------
//
//	�I�[�v��
//
//---------------------------------------------------------------------------
BOOL FASTCALL FDIDiskDIM::Open(const Filepath& path, DWORD offset)
{
	Fileio fio;
	DWORD size;
	FDITrackDIM *track;
	int i;

	ASSERT(this);
	ASSERT(offset == 0);
	ASSERT(!GetFirst());
	ASSERT(!GetHead(0));
	ASSERT(!GetHead(1));

	// �������݉\�Ƃ��ď�����
	disk.writep = FALSE;
	disk.readonly = FALSE;

	// �I�[�v���ł��邱�Ƃ��m���߂�
	if (!fio.Open(path, Fileio::ReadWrite)) {
		// �ǂݍ��݃I�[�v�������݂�
		if (!fio.Open(path, Fileio::ReadOnly)) {
			return FALSE;
		}

		// �ǂݍ��݂͉
		disk.writep = TRUE;
		disk.readonly = TRUE;
	}

	// �t�@�C���T�C�Y��256�ȏ゠�邱�Ƃ��m���߂�
	size = fio.GetFileSize();
	if (size < 0x100) {
		fio.Close();
		return FALSE;
	}

	// �w�b�_�ǂݍ��݁A�F��������`�F�b�N
	fio.Read(dim_hdr, sizeof(dim_hdr));
	fio.Close();
	if (strcmp((char*)&dim_hdr[171], "DIFC HEADER  ") != 0) {
		return FALSE;
	}

	// �p�X���{�I�t�Z�b�g���L�^
	disk.path = path;
	disk.offset = offset;

	// �R�����g�����邩
	if (dim_hdr[0xc2] != '\0') {
		// �f�B�X�N���̓R�����g�Ƃ���(�K��60�����Ő؂�)
		dim_hdr[0xc2 + 60] = '\0';
		strcpy(disk.name, (char*)&dim_hdr[0xc2]);
	}
	else {
		// �f�B�X�N���̓t�@�C�����{�g���q�Ƃ���
		strcpy(disk.name, path.GetShort());
	}

	// �g���b�N���쐬(0�`81�V�����_�܂ŁA82*2�g���b�N)
	for (i=0; i<164; i++) {
		track = new FDITrackDIM(this, i, dim_hdr[0]);
		AddTrack(track);
	}

	// �t���OUp�A�I��
	dim_load = TRUE;
	return TRUE;
}

//---------------------------------------------------------------------------
//
//	�V�[�N
//
//---------------------------------------------------------------------------
void FASTCALL FDIDiskDIM::Seek(int c)
{
	int i;
	FDITrackDIM *track;
	DWORD offset;
	BOOL flag;

	ASSERT(this);
	ASSERT((c >= 0) && (c < 82));
	ASSERT(dim_load);

	// �Y������g���b�N���������A���[�h
	for (i=0; i<2; i++) {
		// �g���b�N������
		track = (FDITrackDIM*)Search(c * 2 + i);
		ASSERT(track);
		disk.head[i] = track;

		// �}�b�v�����āA�L���g���b�N�Ȃ烍�[�h
		flag = FALSE;
		offset = 0;
		if (GetDIMMap(c * 2 + i)) {
			// �I�t�Z�b�g�v�Z
			offset = GetDIMOffset(c * 2 + i);
			flag = TRUE;
		}

		// ���[�h�܂��͍쐬
		track->Load(disk.path, offset, flag);
	}
}

//---------------------------------------------------------------------------
//
//	DIM�g���b�N�}�b�v���擾
//
//---------------------------------------------------------------------------
BOOL FASTCALL FDIDiskDIM::GetDIMMap(int track) const
{
	ASSERT(this);
	ASSERT((track >= 0) && (track <= 163));
	ASSERT(dim_load);

	if (dim_hdr[track + 1] != 0) {
		return TRUE;
	}
	return FALSE;
}

//---------------------------------------------------------------------------
//
//	DIM�g���b�N�I�t�Z�b�g���擾
//
//---------------------------------------------------------------------------
DWORD FASTCALL FDIDiskDIM::GetDIMOffset(int track) const
{
	int i;
	DWORD offset;
	int length;
	FDITrackDIM *dim;

	ASSERT(this);
	ASSERT((track >= 0) && (track <= 163));
	ASSERT(dim_load);

	// �x�[�X��256
	offset = 0x100;

	// �O�̃Z�N�^�܂ł̍��Z�Ƃ���
	for (i=0; i<track; i++) {
		// �L���g���b�N�Ȃ獇�Z
		if (GetDIMMap(i)) {
			dim = (FDITrackDIM*)Search(track);
			ASSERT(dim);
			length = 1 << (dim->GetDIMN() + 7);
			length *= dim->GetDIMSectors();
			offset += length;
		}
	}

	return offset;
}

//---------------------------------------------------------------------------
//
//	�Z�[�u
//
//---------------------------------------------------------------------------
BOOL FASTCALL FDIDiskDIM::Save()
{
	BOOL changed;
	int i;
	FDITrackDIM *track;
	DWORD offset;
	Fileio fio;
	DWORD total;
	BYTE *ptr;

	ASSERT(this);
	ASSERT(dim_load);

	// �}�b�v�ȊO�̃g���b�N�ŕύX���ꂽ���̂����邩�A�܂����ׂ�
	changed = FALSE;
	for (i=0; i<164; i++) {
		if (!GetDIMMap(i)) {
			// �}�b�v�ɂȂ��g���b�N�B�ύX���邩
			track = (FDITrackDIM*)Search(i);
			ASSERT(track);
			if (track->IsChanged()) {
				changed = TRUE;
			}
		}
	}

	// �ύX���}�b�v�݂̂Ȃ�A�ʑΉ�
	if (!changed) {
		for (i=0; i<164; i++) {
			track = (FDITrackDIM*)Search(i);
			ASSERT(track);
			if (track->IsChanged()) {
				// �}�b�v�ς݂̃g���b�N
				ASSERT(GetDIMMap(i));
				offset = GetDIMOffset(i);
				if (!track->Save(disk.path, offset)) {
					return FALSE;
				}
			}
		}
		// �S�g���b�N�I��
		return TRUE;
	}

	// �}�b�v�ς݃g���b�N�����ׂă��[�h
	for (i=0; i<164; i++) {
		if (GetDIMMap(i)) {
			track = (FDITrackDIM*)Search(i);
			ASSERT(track);
			offset = GetDIMOffset(i);
			if (!track->Load(disk.path, offset, TRUE)) {
				return FALSE;
			}
		}
	}

	// �}�b�v����Ă��Ȃ��č���ǉ����ꂽ�Ƃ�����}�b�v�B�����Ƀg�[�^���T�C�Y�𓾂�
	total = 0;
	for (i=0; i<164; i++) {
		track = (FDITrackDIM*)Search(i);
		ASSERT(track);
		if (!GetDIMMap(i)) {
			if (track->IsChanged()) {
				// �V�����}�b�v�ɒǉ�
				dim_hdr[i + 1] = 0x01;
				total += track->GetTotalLength();
			}
		}
		else {
			// ���Ƀ}�b�v����Ă���
			total += track->GetTotalLength();
		}
	}

	// 2HD��154�g���b�N�ȍ~���g���Ă���΁AOverTrack�t���O�𗧂Ă�
	dim_hdr[0xff] = 0x00;
	if (dim_hdr[0] == 0x00) {
		for (i=154; i<164; i++) {
			if (GetDIMMap(i)) {
				dim_hdr[0xff] = 0x01;
			}
		}
	}

	// �V�K�Z�[�u(�T�C�Y������)
	if (!fio.Open(disk.path, Fileio::WriteOnly)) {
		return FALSE;
	}
	if (!fio.Write(dim_hdr, sizeof(dim_hdr))) {
		fio.Close();
		return FALSE;
	}

	// 256�o�C�g�̃w�b�_�ȍ~�́AE5�f�[�^���ŏ��ɃZ�[�u
	try {
		ptr = new BYTE[total];
	}
	catch (...) {
		fio.Close();
		return FALSE;
	}
	if (!ptr) {
		fio.Close();
		return FALSE;
	}
	memset(ptr, 0xe5, total);
	if (!fio.Write(ptr, total)) {
		fio.Close();
		delete[] ptr;
		return FALSE;
	}
	delete[] ptr;

	// �S�ď�������(�t�@�C���͊J�����܂�)
	for (i=0; i<164; i++) {
		if (GetDIMMap(i)) {
			track = (FDITrackDIM*)Search(i);
			ASSERT(track);
			offset = GetDIMOffset(i);
			track->ForceChanged();
			if (!track->Save(&fio, offset)) {
				return FALSE;
			}
		}
	}

	fio.Close();
	return TRUE;
}

//---------------------------------------------------------------------------
//
//	�V�K�f�B�X�N�쐬
//
//---------------------------------------------------------------------------
BOOL FASTCALL FDIDiskDIM::Create(const Filepath& path, const option_t *opt)
{
	int i;
	FDITrackDIM *track;
	Fileio fio;
	static BYTE iocsdata[] = {
		0x04, 0x21, 0x03, 0x22, 0x01, 0x00, 0x00, 0x00
	};

	ASSERT(this);
	ASSERT(opt);

	// �w�b�_���N���A
	memset(dim_hdr, 0, sizeof(dim_hdr));

	// �t�H�[�}�b�g�̃`�F�b�N�ƃ^�C�v��������
	switch (opt->phyfmt) {
		// 2HD(�I�[�o�[�g���b�N�g�p���܂�)
		case FDI_2HD:
		case FDI_2HDA:
			dim_hdr[0] = 0x00;
			break;

		// 2HS
		case FDI_2HS:
			dim_hdr[0] = 0x01;
			break;

		// 2HC
		case FDI_2HC:
			dim_hdr[0] = 0x02;
			break;

		// 2HDE(68)
		case FDI_2HDE:
			dim_hdr[0] = 0x03;
			break;

		// 2HQ
		case FDI_2HQ:
			dim_hdr[0] = 0x09;
			break;

		// N88-BASIC
		case FDI_N88B:
			dim_hdr[0] = 0x11;
			break;

		// �T�|�[�g���Ă��Ȃ������t�H�[�}�b�g
		default:
			return FALSE;
	}

	// �w�b�_�c��(���t��2001-03-22 00:00:00�Ƃ���; XM6�J���J�n��)
	strcpy((char*)&dim_hdr[0xab], "DIFC HEADER  ");
	dim_hdr[0xfe] = 0x19;
	if (opt->phyfmt == FDI_2HDA) {
		dim_hdr[0xff] = 0x01;
	}
	memcpy(&dim_hdr[0xba], iocsdata, 8);
	ASSERT(strlen(opt->name) < 60);
	strcpy((char*)&dim_hdr[0xc2], opt->name);

	// �w�b�_��������
	if (!fio.Open(path, Fileio::WriteOnly)) {
		return FALSE;
	}
	if (!fio.Write(&dim_hdr[0], sizeof(dim_hdr))) {
		return FALSE;
	}
	fio.Close();

	// �t���O�ݒ�
	disk.writep = FALSE;
	disk.readonly = FALSE;
	dim_load = TRUE;

	// �p�X���{�I�t�Z�b�g���L�^
	disk.path = path;
	disk.offset = 0;

	// �f�B�X�N���̓t�@�C�����{�g���q�Ƃ���
	strcpy(disk.name, path.GetShort());

	// �g���b�N���쐬���ĕ����t�H�[�}�b�g
	for (i=0; i<164; i++) {
		track = new FDITrackDIM(this, i, dim_hdr[0x00]);
		track->Create(opt->phyfmt);
		AddTrack(track);
	}

	// �_���t�H�[�}�b�g
	FDIDisk::Create(path, opt);

	// �ۑ�
	if (!Save()) {
		return FALSE;
	}

	// ����
	return TRUE;
}

//---------------------------------------------------------------------------
//
//	�t���b�V��
//
//---------------------------------------------------------------------------
BOOL FASTCALL FDIDiskDIM::Flush()
{
	ASSERT(this);

	// ���[�h����Ă���Ώ�������
	if (dim_load) {
		return Save();
	}

	// ���[�h����Ă��Ȃ�
	return TRUE;
}

//===========================================================================
//
//	FDI�g���b�N(D68)
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	�R���X�g���N�^
//
//---------------------------------------------------------------------------
FDITrackD68::FDITrackD68(FDIDisk *disk, int track, BOOL hd) : FDITrack(disk, track, hd)
{
	// �t�H�[�}�b�g�ύX�Ȃ�
	d68_format = FALSE;
}

//---------------------------------------------------------------------------
//
//	���[�h
//
//---------------------------------------------------------------------------
BOOL FASTCALL FDITrackD68::Load(const Filepath& path, DWORD offset)
{
	Fileio fio;
	BYTE header[0x10];
	DWORD chrn[4];
	BYTE buf[0x2000];
	BOOL mfm;
	int len;
	int i;
	int num;
	int gap;
	int stat;
	FDISector *sector;
	BYTE *ptr;
	const int *table;

	ASSERT(this);
	ASSERT(offset > 0);

	// �������ς݂Ȃ�s�v(�V�[�N���ɌĂ΂��̂ŁA�P�x�����ǂ�ŃL���b�V������)
	if (IsInit()) {
		return TRUE;
	}

	// �Z�N�^�����݂��Ȃ�����
	ASSERT(!GetFirst());
	ASSERT(GetAllSectors() == 0);
	ASSERT(GetMFMSectors() == 0);
	ASSERT(GetFMSectors() == 0);

	// �I�[�v���ƃV�[�N
	if (!fio.Open(path, Fileio::ReadOnly)) {
		return FALSE;
	}
	if (!fio.Seek(offset)) {
		return FALSE;
	}

	// �Œ�1�̓Z�N�^������Ɖ���(�I�t�Z�b�g!=0�̂���)
	i = 0;
	num = 1;
	while (i < num) {
		// �w�b�_�ǂ�
		if (!fio.Read(header, sizeof(header))) {
			break;
		}

		// ����̓Z�N�^�����擾
		if (i == 0) {
			ptr = &header[0x04];
			num = (int)ptr[1];
			num <<= 8;
			num |= (int)ptr[0];
		}

		// MFM
		mfm = TRUE;
		if (header[0x06] != 0) {
			mfm = FALSE;
		}

		// �����O�X
		ptr = &header[0x0e];
		len = (int)ptr[1];
		len <<= 8;
		len |= (int)ptr[0];

		// GAP3(D68�t�@�C���ɂ͏�񂪂Ȃ��̂ŁA�悭����p�^�[����p��)
		gap = 0x12;
		table = &Gap3Table[0];
		while (table[0] != 0) {
			// GAP�e�[�u������
			if ((table[0] == num) && (table[1] == (int)header[3])) {
				gap = table[2];
				break;
			}
			table += 3;
		}

		// DELETED SECTOR���܂ރX�e�[�^�X
		stat = FDD_NOERROR;
		if (header[0x07] != 0) {
			stat |= FDD_DDAM;
		}
		if ((header[0x08] & 0xf0) == 0xa0) {
			stat |= FDD_DATACRC;
		}

		// �o�b�t�@�փf�[�^�����[�h
		if (sizeof(buf) < len) {
			break;
		}
		if (!fio.Read(buf, len)) {
			break;
		}

		// �Z�N�^�쐬
		chrn[0] = (DWORD)header[0];
		chrn[1] = (DWORD)header[1];
		chrn[2] = (DWORD)header[2];
		chrn[3] = (DWORD)header[3];
		sector = new FDISector(mfm, chrn);
		sector->Load(buf, len, gap, stat);

		// ����
		AddSector(sector);
		i++;
	}

	fio.Close();

	// �|�W�V�����v�Z
	CalcPos();

	// ������ok
	trk.init = TRUE;
	return TRUE;
}

//---------------------------------------------------------------------------
//
//	�Z�[�u
//
//---------------------------------------------------------------------------
BOOL FASTCALL FDITrackD68::Save(const Filepath& path, DWORD offset)
{
	FDISector *sector;
	BYTE header[0x10];
	DWORD chrn[4];
	Fileio fio;
	int secs;
	int len;
	BYTE *ptr;

	ASSERT(this);
	ASSERT(offset > 0);

	// �Z�N�^������
	sector = GetFirst();

	while (sector) {
		if (!sector->IsChanged()) {
			// �ύX����Ă��Ȃ��̂ŃX�L�b�v
			offset += 0x10;
			offset += sector->GetLength();
			sector = sector->GetNext();
			continue;
		}

		// �ύX����Ă���B�w�b�_�����
		memset(header, 0, sizeof(header));
		sector->GetCHRN(chrn);
		header[0] = (BYTE)chrn[0];
		header[1] = (BYTE)chrn[1];
		header[2] = (BYTE)chrn[2];
		header[3] = (BYTE)chrn[3];
		secs = GetAllSectors();
		ptr = &header[0x04];
		ptr[1] = (BYTE)(secs >> 8);
		ptr[0] = (BYTE)secs;
		if (!sector->IsMFM()) {
			header[0x06] = 0x40;
		}
		if (sector->GetError() & FDD_DDAM) {
			header[0x07] = 0x10;
		}
		if (sector->GetError() & FDD_IDCRC) {
			header[0x08] = 0xa0;
		}
		if (sector->GetError() & FDD_DATACRC) {
			header[0x08] = 0xa0;
		}
		ptr = &header[0x0e];
		len = sector->GetLength();
		ptr[1] = (BYTE)(len >> 8);
		ptr[0] = (BYTE)len;

		// ��������
		if (!fio.IsValid()) {
			// ����Ȃ�t�@�C���I�[�v��
			if (!fio.Open(path, Fileio::ReadWrite)) {
				return FALSE;
			}
		}
		if (!fio.Seek(offset)) {
			fio.Close();
			return FALSE;
		}
		if (!fio.Write(header, sizeof(header))) {
			fio.Close();
			return FALSE;
		}
		if (!fio.Write(sector->GetSector(), sector->GetLength())) {
			fio.Close();
			return FALSE;
		}

		// �������݊���
		sector->ClrChanged();

		// ����
		offset += 0x10;
		offset += sector->GetLength();
		sector = sector->GetNext();
	}

	// �L���Ȃ�t�@�C���N���[�Y
	if (fio.IsValid()) {
		fio.Close();
	}

	// �t�H�[�}�b�g�t���O���~�낵�Ă���
	d68_format = FALSE;
	return TRUE;
}

//---------------------------------------------------------------------------
//
//	�Z�[�u
//
//---------------------------------------------------------------------------
BOOL FASTCALL FDITrackD68::Save(Fileio *fio, DWORD offset)
{
	FDISector *sector;
	BYTE header[0x10];
	DWORD chrn[4];
	int secs;
	int len;
	BYTE *ptr;

	ASSERT(this);
	ASSERT(fio);
	ASSERT(offset > 0);

	// �Z�N�^������
	sector = GetFirst();

	while (sector) {
		if (!sector->IsChanged()) {
			// �ύX����Ă��Ȃ��̂ŃX�L�b�v
			offset += 0x10;
			offset += sector->GetLength();
			sector = sector->GetNext();
			continue;
		}

		// �ύX����Ă���B�w�b�_�����
		memset(header, 0, sizeof(header));
		sector->GetCHRN(chrn);
		header[0] = (BYTE)chrn[0];
		header[1] = (BYTE)chrn[1];
		header[2] = (BYTE)chrn[2];
		header[3] = (BYTE)chrn[3];
		secs = GetAllSectors();
		ptr = &header[0x04];
		ptr[1] = (BYTE)(secs >> 8);
		ptr[0] = (BYTE)secs;
		if (!sector->IsMFM()) {
			header[0x06] = 0x40;
		}
		if (sector->GetError() & FDD_DDAM) {
			header[0x07] = 0x10;
		}
		if (sector->GetError() & FDD_IDCRC) {
			header[0x08] = 0xa0;
		}
		if (sector->GetError() & FDD_DATACRC) {
			header[0x08] = 0xa0;
		}
		ptr = &header[0x0e];
		len = sector->GetLength();
		ptr[1] = (BYTE)(len >> 8);
		ptr[0] = (BYTE)len;

		// ��������
		fio->Seek(offset);
		if (!fio->Write(header, sizeof(header))) {
			return FALSE;
		}
		if (!fio->Write(sector->GetSector(), sector->GetLength())) {
			return FALSE;
		}

		// �������݊���
		sector->ClrChanged();

		// ����
		offset += 0x10;
		offset += sector->GetLength();
		sector = sector->GetNext();
	}

	// �t�H�[�}�b�g�t���O���~�낵�Ă���
	d68_format = FALSE;
	return TRUE;
}

//---------------------------------------------------------------------------
//
//	���C�gID
//	�����̃X�e�[�^�X��Ԃ�(�G���[��OR�����)
//		FDD_NOERROR		�G���[�Ȃ�
//		FDD_NOTWRITE	�������݋֎~
//
//---------------------------------------------------------------------------
int FASTCALL FDITrackD68::WriteID(const BYTE *buf, DWORD d, int sc, BOOL mfm, int gpl)
{
	int stat;
	FDISector *sector;
	DWORD pos;
	int i;
	BYTE fillbuf[0x2000];
	DWORD chrn[4];

	ASSERT(this);
	ASSERT(sc > 0);

	// �I���W�i�����Ă�(���C�g�v���e�N�g�̃`�F�b�N��FDIDisk�Ŋ��ɍs���Ă���)
	stat = FDITrack::WriteID(buf, d, sc, mfm, gpl);
	if (stat == FDD_NOERROR) {
		// �t�H�[�}�b�g����(�ȑO�Ɠ���̕����t�H�[�}�b�g)
		return stat;
	}

	// �قȂ�t�H�[�}�b�g
	d68_format = TRUE;

	// ���Ԃ�ݒ�(index�܂�)
	pos = GetDisk()->GetRotationTime();
	pos -= GetDisk()->GetRotationPos();
	GetDisk()->SetSearch(pos);

	// buf���^�����Ă��Ȃ���΂����܂�
	if (!buf) {
		return FDD_NOERROR;
	}

	// �Z�N�^���N���A
	ClrSector();
	memset(fillbuf, d, sizeof(fillbuf));

	// ���ɃZ�N�^���쐬
	for (i=0; i<sc; i++) {
		// �����O�X>=7�̓A���t�H�[�}�b�g
		if (buf[i * 4 + 3] >= 0x07) {
			ClrSector();
			return FDD_NOERROR;
		}

		// �Z�N�^���쐬
		chrn[0] = (DWORD)buf[i * 4 + 0];
		chrn[1] = (DWORD)buf[i * 4 + 1];
		chrn[2] = (DWORD)buf[i * 4 + 2];
		chrn[3] = (DWORD)buf[i * 4 + 3];
		sector = new FDISector(mfm, chrn);
		sector->Load(fillbuf, 1 << (buf[i * 4 + 3] + 7), gpl, FDD_NOERROR);

		// �Z�N�^��ǉ�
		AddSector(sector);
	}

	// �|�W�V�����v�Z
	CalcPos();

	return FDD_NOERROR;
}

//---------------------------------------------------------------------------
//
//	D68�ł̒����擾
//
//---------------------------------------------------------------------------
DWORD FASTCALL FDITrackD68::GetD68Length() const
{
	DWORD length;
	FDISector *sector;

	ASSERT(this);

	// ������
	length = 0;
	sector = GetFirst();

	// ���[�v
	while (sector) {
		length += 0x10;
		length += sector->GetLength();
		sector = sector->GetNext();
	}

	return length;
}

//---------------------------------------------------------------------------
//
//	GAP3�e�[�u��
//
//---------------------------------------------------------------------------
const int FDITrackD68::Gap3Table[] = {
	// SEC, N, GAP3
	 8, 3, 0x74,						// 2HD
	 9, 3, 0x39,						// 2HS, 2HDE
	15, 2, 0x54,						// 2HC
	18, 2, 0x54,						// 2HQ
	26, 1, 0x33,						// OS-9/68000, N88-BASIC
	26, 0, 0x1a,						// N88-BASIC
	 9, 2, 0x54,						// 2DD(720KB)
	 8, 2, 0x54,						// 2DD(640KB)
	 5, 3, 0x74,						// 2D(Falcom)
	16, 1, 0x33,						// 2D(BASIC)
	 0, 0, 0
};

//===========================================================================
//
//	FDI�f�B�X�N(D68)
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	�R���X�g���N�^
//
//---------------------------------------------------------------------------
FDIDiskD68::FDIDiskD68(int index, FDI *fdi) : FDIDisk(index, fdi)
{
	// ID�ݒ�
	disk.id = MAKEID('D', '6', '8', ' ');

	// �w�b�_���N���A
	memset(d68_hdr, 0, sizeof(d68_hdr));

	// ���[�h�Ȃ�
	d68_load = FALSE;
}

//---------------------------------------------------------------------------
//
//	�f�X�g���N�^
//
//---------------------------------------------------------------------------
FDIDiskD68::~FDIDiskD68()
{
	// ���[�h����Ă���Ώ�������
	if (d68_load) {
		Save();
	}
}

//---------------------------------------------------------------------------
//
//	�I�[�v��
//
//---------------------------------------------------------------------------
BOOL FASTCALL FDIDiskD68::Open(const Filepath& path, DWORD offset)
{
	Fileio fio;
	int i;
	FDITrackD68 *track;
	BOOL hd;

	ASSERT(this);
	ASSERT(!GetFirst());
	ASSERT(!GetHead(0));
	ASSERT(!GetHead(1));

	// �������݉\�Ƃ��ď�����
	disk.writep = FALSE;
	disk.readonly = FALSE;

	// �I�[�v���ł��邱�Ƃ��m���߂�
	if (!fio.Open(path, Fileio::ReadWrite)) {
		// �ǂݍ��݃I�[�v�������݂�
		if (!fio.Open(path, Fileio::ReadOnly)) {
			return FALSE;
		}

		// �ǂݍ��݂͉
		disk.writep = TRUE;
		disk.readonly = TRUE;
	}

	// �V�[�N�A�w�b�_�ǂݍ���
	if (!fio.Seek(offset)) {
		fio.Close();
		return FALSE;
	}
	if (!fio.Read(d68_hdr, sizeof(d68_hdr))) {
		fio.Close();
		return FALSE;
	}
	fio.Close();

	// �p�X���{�I�t�Z�b�g���L�^
	disk.path = path;
	disk.offset = offset;

	// �f�B�X�N��(�K��16�����Ő؂�)
	d68_hdr[0x10] = 0;
	strcpy(disk.name, (char*)d68_hdr);
	// �������V���O���f�B�X�N�ŁANULL��Default�Ȃ�t�@�C����+�g���q
	if (!GetFDI()->IsMulti()) {
		if (strcmp(disk.name, "Default") == 0) {
			strcpy(disk.name, path.GetShort());
		}
		if (strlen(disk.name) == 0) {
			strcpy(disk.name, path.GetShort());
		}
	}

	// ���C�g�v���e�N�g
	if (d68_hdr[0x1a] != 0) {
		disk.writep = TRUE;
	}

	// HD�t���O
	switch (d68_hdr[0x1b]) {
		// 2D,2DD
		case 0x00:
		case 0x10:
			hd = FALSE;
			break;
		// 2HD
		case 0x20:
			hd = TRUE;
			break;
		default:
			return FALSE;
	}

	// �g���b�N���쐬(0�`81�V�����_�܂ŁA82*2�g���b�N)
	for (i=0; i<164; i++) {
		track = new FDITrackD68(this, i, hd);
		AddTrack(track);
	}

	// �t���OUp�A�I��
	d68_load = TRUE;
	return TRUE;
}

//---------------------------------------------------------------------------
//
//	�V�[�N
//
//---------------------------------------------------------------------------
void FASTCALL FDIDiskD68::Seek(int c)
{
	int i;
	FDITrackD68 *track;
	DWORD offset;

	ASSERT(this);
	ASSERT((c >= 0) && (c < 82));
	ASSERT(d68_load);

	// �Y������g���b�N���������A���[�h
	for (i=0; i<2; i++) {
		// �g���b�N������
		track = (FDITrackD68*)Search(c * 2 + i);
		ASSERT(track);
		disk.head[i] = track;

		// �I�t�Z�b�g�擾�A�L���g���b�N�Ȃ烍�[�h
		if (d68_hdr[0x1b] == 0x00) {
			// 2D
			if (c == 0) {
				offset = GetD68Offset(i);
			}
			else {
				if (c & 1) {
					// 1,3,5...��V�����_�͂��ꂼ��1,2,3�V�����_
					offset = GetD68Offset(c + 1 + i);
				}
				else {
					// 2,4,6...�����V�����_��Unformat
					offset = 0;
				}
			}
		}
		else {
			// 2DD,2HD
			offset = GetD68Offset(c * 2 + i);
		}
		if (offset > 0) {
			track->Load(disk.path, disk.offset + offset);
		}
	}
}

//---------------------------------------------------------------------------
//
//	D68�g���b�N�I�t�Z�b�g���擾
//	�������g���b�N��0
//
//---------------------------------------------------------------------------
DWORD FASTCALL FDIDiskD68::GetD68Offset(int track) const
{
	DWORD offset;
	const BYTE *ptr;

	ASSERT(this);
	ASSERT((track >= 0) && (track <= 163));
	ASSERT(d68_load);

	// �|�C���^�擾
	ptr = &d68_hdr[0x20 + (track << 2)];

	// �I�t�Z�b�g�擾(���g���G���f�B�A��)
	offset = (DWORD)ptr[2];
	offset <<= 8;
	offset |= (DWORD)ptr[1];
	offset <<= 8;
	offset |= (DWORD)ptr[0];

	return offset;
}

//---------------------------------------------------------------------------
//
//	�Z�[�u
//
//---------------------------------------------------------------------------
BOOL FASTCALL FDIDiskD68::Save()
{
	DWORD diskoff[16 + 1];
	BOOL format;
	int i;
	FDITrackD68 *track;
	DWORD offset;
	DWORD length;
	BYTE *fileptr;
	DWORD filelen;
	Fileio fio;
	BYTE *ptr;

	ASSERT(this);

	// ���[�h����Ă��Ȃ���Ή������Ȃ�
	if (!d68_load) {
		return TRUE;
	}

	// �I�t�Z�b�g���Ď擾����
	memset(diskoff, 0, sizeof(diskoff));
	CheckDisks(disk.path, diskoff);
	disk.offset = diskoff[disk.index];

	// �t�H�[�}�b�g�̕ύX�������Ă��邩�ǂ������ׂ�
	format = FALSE;
	for (i=0; i<164; i++) {
		track = (FDITrackD68*)Search(i);
		ASSERT(track);
		if (track->IsFormated()) {
			format = TRUE;
		}
	}

	// �����Ă��Ȃ���΁A�g���b�N�P�ʂŕۑ�
	if (!format) {
		for (i=0; i<164; i++) {
			track = (FDITrackD68*)Search(i);
			ASSERT(track);
			offset = GetD68Offset(i);
			if (offset > 0) {
				if (!track->Save(disk.path, disk.offset + offset)) {
					return FALSE;
				}
			}
		}

		// ���C�g�v���e�N�g�̐H���Ⴂ������΁A���������ۑ�
		i = 0;
		if (IsWriteP()) {
			i = 0x10;
		}
		if (d68_hdr[0x1a] != (BYTE)i) {
			d68_hdr[0x1a] = (BYTE)i;
			if (!fio.Open(disk.path, Fileio::ReadWrite)) {
				return FALSE;
			}
			if (!fio.Seek(diskoff[disk.index])) {
				fio.Close();
				return FALSE;
			}
			if (!fio.Write(d68_hdr, sizeof(d68_hdr))) {
				fio.Close();
				return FALSE;
			}
			fio.Close();
		}
		return TRUE;
	}

	// �t�@�C����S�ă������ɗ��Ƃ�
	if (!fio.Open(disk.path, Fileio::ReadOnly)) {
		return FALSE;
	}
	filelen = fio.GetFileSize();
	try {
		fileptr = new BYTE[filelen];
	}
	catch (...) {
		fio.Close();
		return FALSE;
	}
	if (!fileptr) {
		fio.Close();
		return FALSE;
	}
	if (!fio.Read(fileptr, filelen)) {
		delete[] fileptr;
		fio.Close();
		return FALSE;
	}
	fio.Close();

	// �w�b�_���č\�z
	offset = sizeof(d68_hdr);
	for (i=0; i<164; i++) {
		track = (FDITrackD68*)Search(i);
		ASSERT(track);
		length = track->GetD68Length();
		ptr = &d68_hdr[0x20 + (i << 2)];
		if (length == 0) {
			memset(ptr, 0, 4);
		}
		else {
			ptr[3] = (BYTE)(offset >> 24);
			ptr[2] = (BYTE)(offset >> 16);
			ptr[1] = (BYTE)(offset >> 8);
			ptr[0] = (BYTE)offset;
			offset += length;
		}
	}
	d68_hdr[0x1a] = 0;
	if (IsWriteP()) {
		d68_hdr[0x1a] = 0x10;
	}
	ptr = &d68_hdr[0x1c];
	ptr[3] = (BYTE)(offset >> 24);
	ptr[2] = (BYTE)(offset >> 16);
	ptr[1] = (BYTE)(offset >> 8);
	ptr[0] = (BYTE)offset;

	// �t�@�C���̑O��ۑ�
	if (!fio.Open(disk.path, Fileio::WriteOnly)) {
		delete[] fileptr;
		return FALSE;
	}
	if (diskoff[disk.index] != 0) {
		if (!fio.Write(fileptr, diskoff[disk.index])) {
			delete[] fileptr;
			fio.Close();
			return FALSE;
		}
	}

	// �w�b�_��ۑ�
	if (!fio.Write(d68_hdr, sizeof(d68_hdr))) {
		delete[] fileptr;
		fio.Close();
		return FALSE;
	}
	offset -= sizeof(d68_hdr);

	// �T�C�Y��������
	while (offset > 0) {
		// 1��ŏ������݂��ςޏꍇ
		if (offset < filelen) {
			if (!fio.Write(fileptr, offset)) {
				delete[] fileptr;
				fio.Close();
				return FALSE;
			}
			break;
		}

		// ������ɂ킽��ꍇ
		if (!fio.Write(fileptr, filelen)) {
			delete[] fileptr;
			fio.Close();
			return FALSE;
		}
		offset -= filelen;
	}

	// �t�@�C���̌��ۑ�
	if (diskoff[disk.index + 1] != 0) {
		ASSERT(filelen >= diskoff[disk.index + 1]);
		if (!fio.Write(&fileptr[ diskoff[disk.index + 1] ],
				filelen - diskoff[disk.index + 1])) {
			delete[] fileptr;
			fio.Close();
			return FALSE;
		}
	}
	delete[] fileptr;

	// �����O�X!=0�ɂ��đS�ăZ�[�u(�����ύX�A�t�@�C���͊J�����܂�)
	for (i=0; i<164; i++) {
		track = (FDITrackD68*)Search(i);
		ASSERT(track);
		length = track->GetD68Length();
		if (length != 0) {
			offset = GetD68Offset(i);
			ASSERT(offset != 0);
			track->ForceChanged();
			if (!track->Save(&fio, disk.offset + offset)) {
				return FALSE;
			}
		}
	}

	fio.Close();
	return TRUE;
}

//---------------------------------------------------------------------------
//
//	�V�K�f�B�X�N�쐬
//
//---------------------------------------------------------------------------
BOOL FASTCALL FDIDiskD68::Create(const Filepath& path, const option_t *opt)
{
	Fileio fio;
	int i;
	FDITrackD68 *track;
	BOOL hd;

	ASSERT(this);
	ASSERT(opt);
	ASSERT(!GetFirst());
	ASSERT(!GetHead(0));
	ASSERT(!GetHead(1));

	// �w�b�_�����
	memset(&d68_hdr, 0, sizeof(d68_hdr));
	ASSERT(strlen(opt->name) <= 16);
	strcpy((char*)d68_hdr, opt->name);
	if (opt->phyfmt == FDI_2DD) {
		hd = FALSE;
		d68_hdr[0x1b] = 0x10;
	}
	else {
		hd = TRUE;
		d68_hdr[0x1b] = 0x20;
	}

	// �w�b�_����������
	if (!fio.Open(path, Fileio::WriteOnly)) {
		return FALSE;
	}
	if (!fio.Write(d68_hdr, sizeof(d68_hdr))) {
		fio.Close();
		return FALSE;
	}
	fio.Close();

	// �p�X�A�f�B�X�N���A�I�t�Z�b�g
	disk.path = path;
	strcpy(disk.name, opt->name);
	disk.offset = 0;

	// �g���b�N���쐬(0�`81�V�����_�܂ŁA82*2�g���b�N)
	for (i=0; i<164; i++) {
		track = new FDITrackD68(this, i, hd);
		track->Create(opt->phyfmt);
		track->ForceFormat();
		AddTrack(track);
	}

	// �t���O�ݒ�
	disk.writep = FALSE;
	disk.readonly = FALSE;
	d68_load = TRUE;

	// �_���t�H�[�}�b�g
	FDIDisk::Create(path, opt);

	// �ۑ�
	if (!Save()) {
		return FALSE;
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	D68�t�@�C���̌���
//
//---------------------------------------------------------------------------
int FASTCALL FDIDiskD68::CheckDisks(const Filepath& path, DWORD *offbuf)
{
	Fileio fio;
	DWORD fsize;
	DWORD dsize;
	DWORD base;
	DWORD prev;
	DWORD offset;
	int disks;
	BYTE header[0x2b0];
	BYTE *ptr;
	int i;

	ASSERT(offbuf);

	// ������
	disks = 0;
	base = 0;

	// �t�@�C���T�C�Y�擾
	if (!fio.Open(path, Fileio::ReadOnly)) {
		return 0;
	}
	fsize = fio.GetFileSize();
	fio.Close();
	if (fsize < sizeof(header)) {
		return 0;
	}

	// �f�B�X�N���[�v
	while (disks < 16) {
		// �T�C�Y�I�[�o�Ȃ�I��
		if (base >= fsize) {
			break;
		}

		// �w�b�_��ǂ�
		if (!fio.Open(path, Fileio::ReadOnly)) {
			return 0;
		}
		if (!fio.Seek(base)) {
			fio.Close();
			break;
		}
		if (!fio.Read(header, sizeof(header))) {
			fio.Close();
			break;
		}
		fio.Close();

		// ���x���`�F�b�N
		switch (header[0x1b]) {
			case 0x00:
			case 0x10:
			case 0x20:
				break;
			default:
				return 0;
		}

		// ���̃f�B�X�N�T�C�Y���擾(0x200�ȏ�A1.92MB�ȉ��ƌ���)
		ptr = &header[0x1c];
		dsize = (DWORD)ptr[3];
		dsize <<= 8;
		dsize |= (DWORD)ptr[2];
		dsize <<= 8;
		dsize |= (DWORD)ptr[1];
		dsize <<= 8;
		dsize |= (DWORD)ptr[0];

		if ((dsize + base) > fsize) {
			return 0;
		}
		if (dsize < 0x200) {
			return 0;
		}
		if (dsize > 1920 * 1024) {
			return 0;
		}

		// �I�t�Z�b�g�������Bdsize�𒴂��Ă��Ȃ��ĒP�������ł��邱��
		prev = 0;
		for (i=0; i<164; i++) {
			// 2D�C���[�W�ł����84�g���b�N�ȏ�͌������Ȃ�(�ςȒl���������܂�Ă���ꍇ������)
			if (header[0x1b] == 0x00) {
				if (i >= 84) {
					break;
				}
			}

			// ���̃g���b�N�̃I�t�Z�b�g�𓾂�
			ptr = &header[0x20 + (i << 2)];
			offset = (DWORD)ptr[3];
			offset <<= 8;
			offset |= (DWORD)ptr[2];
			offset <<= 8;
			offset |= (DWORD)ptr[1];
			offset <<= 8;
			offset |= (DWORD)ptr[0];

			// �I�t�Z�b�g��0x10�Ŋ���؂�Ȃ���΃G���[
			if (offset & 0x0f) {
				return 0;
			}

			// 0�́A���̃g���b�N���A���t�H�[�}�b�g�ł��邱�Ƃ�����
			if (offset != 0) {
				// 0�łȂ����
				if (prev == 0) {
					// �ŏ��̃g���b�N��2X0����n�܂邱��
					if ((offset & 0xffffff0f) != 0x200) {
						return 0;
					}
				}
				else {
					// �P��������
					if (offset <= prev) {
						return 0;
					}
					// �f�B�X�N�T�C�Y�𒴂��Ă��Ȃ�����
					if (offset > dsize) {
						return 0;
					}
				}
				prev = offset;
			}
		}

		// ����Up�A�o�b�t�@�ɓo�^�A����
		offbuf[disks] = base;
		disks++;
		base += dsize;
	}

	return disks;
}

//---------------------------------------------------------------------------
//
//	�I�t�Z�b�g�X�V
//
//---------------------------------------------------------------------------
void FASTCALL FDIDiskD68::AdjustOffset()
{
	DWORD offset[0x10];

	ASSERT(this);

	memset(offset, 0, sizeof(offset));
	CheckDisks(disk.path, offset);
	disk.offset = offset[disk.index];
}

//---------------------------------------------------------------------------
//
//	�t���b�V��
//
//---------------------------------------------------------------------------
BOOL FASTCALL FDIDiskD68::Flush()
{
	ASSERT(this);

	// ���[�h����Ă���Ώ�������
	if (d68_load) {
		return Save();
	}

	// ���[�h����Ă��Ȃ�
	return TRUE;
}

//===========================================================================
//
//	FDI�g���b�N(BAD)
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	�R���X�g���N�^
//
//---------------------------------------------------------------------------
FDITrackBAD::FDITrackBAD(FDIDisk *disk, int track) : FDITrack(disk, track)
{
	ASSERT(disk);

	// �t�@�C���L���Z�N�^��0
	bad_secs = 0;
}

//---------------------------------------------------------------------------
//
//	���[�h
//
//---------------------------------------------------------------------------
BOOL FASTCALL FDITrackBAD::Load(const Filepath& path, DWORD offset)
{
	Fileio fio;
	BYTE buf[0x400];
	DWORD chrn[4];
	int i;
	FDISector *sector;

	ASSERT(this);

	// �������ς݂Ȃ�s�v(�V�[�N���ɌĂ΂��̂ŁA�P�x�����ǂ�ŃL���b�V������)
	if (IsInit()) {
		return TRUE;
	}

	// �Z�N�^�����݂��Ȃ�����
	ASSERT(!GetFirst());
	ASSERT(GetAllSectors() == 0);
	ASSERT(GetMFMSectors() == 0);
	ASSERT(GetFMSectors() == 0);

	// �t�@�C���L���Z�N�^��0
	bad_secs = 0;

	// C�EH�EN����
	chrn[0] = GetTrack() >> 1;
	chrn[1] = GetTrack() & 1;
	chrn[3] = 3;

	// �ǂݍ��݃I�[�v��
	if (!fio.Open(path, Fileio::ReadOnly)) {
		return FALSE;
	}

	// �V�[�N(���s�ł��悢)
	if (!fio.Seek(offset)) {
		// �V�[�N���s�Ȃ̂ŁAE5�Ŗ��߂�
		memset(buf, 0xe5, sizeof(buf));

		// �Z�N�^���[�v
		for (i=0; i<8; i++) {
			chrn[2] = i + 1;
			sector = new FDISector(TRUE, chrn);
			sector->Load(buf, sizeof(buf), 0x74, FDD_NOERROR);
			AddSector(sector);
		}

		// �N���[�Y�A������OK
		fio.Close();
		trk.init = TRUE;
		return TRUE;
	}

	// ���[�v
	for (i=0; i<8; i++) {
		// �o�b�t�@�𖈉�E5�Ŗ��߂�
		memset(buf, 0xe5, sizeof(buf));

		// �f�[�^�ǂݍ���(���s���Ă��悢)
		if (fio.Read(buf, sizeof(buf))) {
			// �t�@�C���L���Z�N�^�𑝂₷(0�`8)
			bad_secs++;
		}

		// �Z�N�^�쐬
		chrn[2] = i + 1;
		sector = new FDISector(TRUE, chrn);
		sector->Load(buf, sizeof(buf), 0x74, FDD_NOERROR);

		// �Z�N�^�ǉ�
		AddSector(sector);
	}

	// �N���[�Y
	fio.Close();

	// �|�W�V�����v�Z
	CalcPos();

	// ������ok
	trk.init = TRUE;
	return TRUE;
}

//---------------------------------------------------------------------------
//
//	�Z�[�u
//
//---------------------------------------------------------------------------
BOOL FASTCALL FDITrackBAD::Save(const Filepath& path, DWORD offset)
{
	Fileio fio;
	FDISector *sector;
	BOOL changed;
	int index;

	ASSERT(this);

	// ����������Ă��Ȃ���Ώ������ޕK�v�Ȃ�
	if (!IsInit()) {
		return TRUE;
	}

	// �Z�N�^���܂���āA�������܂�Ă���Z�N�^�����邩
	sector = GetFirst();
	changed = FALSE;
	while (sector) {
		if (sector->IsChanged()) {
			changed = TRUE;
		}
		sector = sector->GetNext();
	}

	// �ǂ���������܂�Ă��Ȃ���Ή������Ȃ�
	if (!changed) {
		return TRUE;
	}

	// �t�@�C���I�[�v��
	if (!fio.Open(path, Fileio::ReadWrite)) {
		return FALSE;
	}

	// ���[�v
	sector = GetFirst();
	index = 1;
	while (sector) {
		// �ύX����Ă��Ȃ���΁A����
		if (!sector->IsChanged()) {
			offset += sector->GetLength();
			sector = sector->GetNext();
			index++;
			continue;
		}

		// �L���͈͓���
		if (index > bad_secs) {
			// �t�@�C���𒴂��Ă���̂ŁA�_�~�[����
			sector->ClrChanged();
			offset += sector->GetLength();
			sector = sector->GetNext();
			index++;
			continue;
		}

		// �V�[�N
		if (!fio.Seek(offset)) {
			fio.Close();
			return FALSE;
		}

		// ��������
		if (!fio.Write(sector->GetSector(), sector->GetLength())) {
			fio.Close();
			return FALSE;
		}

		// �t���O�𗎂Ƃ�
		sector->ClrChanged();

		// ����
		offset += sector->GetLength();
		sector = sector->GetNext();
		index++;
	}

	// �I��
	fio.Close();
	return TRUE;
}

//===========================================================================
//
//	FDI�f�B�X�N(BAD)
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	�R���X�g���N�^
//
//---------------------------------------------------------------------------
FDIDiskBAD::FDIDiskBAD(int index, FDI *fdi) : FDIDisk(index, fdi)
{
	// ID�ݒ�
	disk.id = MAKEID('B', 'A', 'D', ' ');
}

//---------------------------------------------------------------------------
//
//	�f�X�g���N�^
//
//---------------------------------------------------------------------------
FDIDiskBAD::~FDIDiskBAD()
{
	int i;
	DWORD offset;
	FDITrack *track;

	// �Ō�̃g���b�N�f�[�^����������
	for (i=0; i<2; i++) {
		// �g���b�N�����邩
		track = GetHead(i);
		if (!track) {
			continue;
		}

		// �g���b�N�i���o����A�I�t�Z�b�g���Z�o(x8KB)
		offset = track->GetTrack();
		offset <<= 13;

		// ��������
		track->Save(disk.path, offset);
		disk.head[i] = NULL;
	}
}

//---------------------------------------------------------------------------
//
//	�I�[�v��
//
//---------------------------------------------------------------------------
BOOL FASTCALL FDIDiskBAD::Open(const Filepath& path, DWORD offset)
{
	Fileio fio;
	DWORD size;
	FDITrackBAD *track;
	int i;

	ASSERT(this);
	ASSERT(offset == 0);
	ASSERT(!GetFirst());
	ASSERT(!GetHead(0));
	ASSERT(!GetHead(1));

	// �������݉\�Ƃ��ď�����
	disk.writep = FALSE;
	disk.readonly = FALSE;

	// �I�[�v���ł��邱�Ƃ��m���߂�
	if (!fio.Open(path, Fileio::ReadWrite)) {
		// �ǂݍ��݃I�[�v�������݂�
		if (!fio.Open(path, Fileio::ReadOnly)) {
			return FALSE;
		}

		// �ǂݍ��݂͉
		disk.writep = TRUE;
		disk.readonly = TRUE;
	}

	// �t�@�C���T�C�Y��1024�Ŋ���؂�邱�ƁA1280KB�ȉ��ł��邱�Ƃ��m���߂�
	size = fio.GetFileSize();
	if (size & 0x3ff) {
		fio.Close();
		return FALSE;
	}
	if (size > 1310720) {
		fio.Close();
		return FALSE;
	}
	fio.Close();

	// �p�X�A�I�t�Z�b�g���L��
	disk.path = path;
	disk.offset = offset;

	// �f�B�X�N���̓t�@�C�����{�g���q�Ƃ���
	strcpy(disk.name, path.GetShort());

	// �g���b�N���쐬(0�`76�V�����_�܂ŁA77*2�g���b�N)
	for (i=0; i<154; i++) {
		track = new FDITrackBAD(this, i);
		AddTrack(track);
	}

	// �I��
	return TRUE;
}

//---------------------------------------------------------------------------
//
//	�V�[�N
//
//---------------------------------------------------------------------------
void FASTCALL FDIDiskBAD::Seek(int c)
{
	int i;
	FDITrackBAD *track;
	DWORD offset;

	ASSERT(this);
	ASSERT((c >= 0) && (c < 82));

	// �g���b�N�f�[�^����������
	for (i=0; i<2; i++) {
		// �g���b�N�����邩
		track = (FDITrackBAD*)GetHead(i);
		if (!track) {
			continue;
		}

		// �g���b�N�i���o����A�I�t�Z�b�g���Z�o(x8KB)
		offset = track->GetTrack();
		offset <<= 13;

		// ��������
		track->Save(disk.path, offset);
	}

	// c��76�܂ŋ��B�͈͊O�ł����head[i]=NULL�Ƃ���
	if ((c < 0) || (c > 76)) {
		disk.head[0] = NULL;
		disk.head[1] = NULL;
		return;
	}

	// �Y������g���b�N���������A���[�h
	for (i=0; i<2; i++) {
		// �g���b�N������
		track = (FDITrackBAD*)Search(c * 2 + i);
		ASSERT(track);
		disk.head[i] = track;

		// �g���b�N�i���o����A�I�t�Z�b�g���Z�o(x8KB)
		offset = track->GetTrack();
		offset <<= 13;

		// ���[�h
		track->Load(disk.path, offset);
	}
}

//---------------------------------------------------------------------------
//
//	�t���b�V��
//
//---------------------------------------------------------------------------
BOOL FASTCALL FDIDiskBAD::Flush()
{
	ASSERT(this);

	int i;
	DWORD offset;
	FDITrack *track;

	// �Ō�̃g���b�N�f�[�^����������
	for (i=0; i<2; i++) {
		// �g���b�N�����邩
		track = GetHead(i);
		if (!track) {
			continue;
		}

		// �g���b�N�i���o����A�I�t�Z�b�g���Z�o(x8KB)
		offset = track->GetTrack();
		offset <<= 13;

		// ��������
		if(!track->Save(disk.path, offset)) {
			return FALSE;
		}
	}

	return TRUE;
}

//===========================================================================
//
//	FDI�g���b�N(2DD)
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	�R���X�g���N�^
//
//---------------------------------------------------------------------------
FDITrack2DD::FDITrack2DD(FDIDisk *disk, int track) : FDITrack(disk, track, FALSE)
{
	ASSERT(disk);
}

//---------------------------------------------------------------------------
//
//	���[�h
//
//---------------------------------------------------------------------------
BOOL FASTCALL FDITrack2DD::Load(const Filepath& path, DWORD offset)
{
	Fileio fio;
	BYTE buf[0x200];
	DWORD chrn[4];
	int i;
	FDISector *sector;

	ASSERT(this);
	ASSERT((offset % 0x1200) == 0);
	ASSERT(offset < 0xb4000);

	// �������ς݂Ȃ�s�v(�V�[�N���ɌĂ΂��̂ŁA�P�x�����ǂ�ŃL���b�V������)
	if (IsInit()) {
		return TRUE;
	}

	// �Z�N�^�����݂��Ȃ�����
	ASSERT(!GetFirst());
	ASSERT(GetAllSectors() == 0);
	ASSERT(GetMFMSectors() == 0);
	ASSERT(GetFMSectors() == 0);

	// C�EH�EN����
	chrn[0] = GetTrack() >> 1;
	chrn[1] = GetTrack() & 1;
	chrn[3] = 2;

	// �ǂݍ��݃I�[�v��
	if (!fio.Open(path, Fileio::ReadOnly)) {
		return FALSE;
	}

	// �V�[�N
	if (!fio.Seek(offset)) {
		fio.Close();
		return FALSE;
	}

	// ���[�v
	for (i=0; i<9; i++) {
		// �f�[�^�ǂݍ���
		if (!fio.Read(buf, sizeof(buf))) {
			// �r���܂Œǉ����������폜����
			ClrSector();
			fio.Close();
			return FALSE;
		}

		// �Z�N�^�쐬
		chrn[2] = i + 1;
		sector = new FDISector(TRUE, chrn);
		sector->Load(buf, sizeof(buf), 0x54, FDD_NOERROR);

		// �Z�N�^�ǉ�
		AddSector(sector);
	}

	// �N���[�Y
	fio.Close();

	// �|�W�V�����v�Z
	CalcPos();

	// ������ok
	trk.init = TRUE;
	return TRUE;
}

//===========================================================================
//
//	FDI�f�B�X�N(2DD)
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	�R���X�g���N�^
//
//---------------------------------------------------------------------------
FDIDisk2DD::FDIDisk2DD(int index, FDI *fdi) : FDIDisk(index, fdi)
{
	// ID�ݒ�
	disk.id = MAKEID('2', 'D', 'D', ' ');
}

//---------------------------------------------------------------------------
//
//	�f�X�g���N�^
//
//---------------------------------------------------------------------------
FDIDisk2DD::~FDIDisk2DD()
{
	int i;
	DWORD offset;
	FDITrack *track;

	// �Ō�̃g���b�N�f�[�^����������
	for (i=0; i<2; i++) {
		// �g���b�N�����邩
		track = GetHead(i);
		if (!track) {
			continue;
		}

		// �g���b�N�i���o����A�I�t�Z�b�g���Z�o(x0x1200)
		offset = track->GetTrack();
		offset *= 0x1200;

		// ��������
		track->Save(disk.path, offset);
		disk.head[i] = NULL;
	}
}

//---------------------------------------------------------------------------
//
//	�I�[�v��
//
//---------------------------------------------------------------------------
BOOL FASTCALL FDIDisk2DD::Open(const Filepath& path, DWORD offset)
{
	Fileio fio;
	DWORD size;
	FDITrack2DD *track;
	int i;

	ASSERT(this);
	ASSERT(offset == 0);
	ASSERT(!GetFirst());
	ASSERT(!GetHead(0));
	ASSERT(!GetHead(1));

	// �������݉\�Ƃ��ď�����
	disk.writep = FALSE;
	disk.readonly = FALSE;

	// �I�[�v���ł��邱�Ƃ��m���߂�
	if (!fio.Open(path, Fileio::ReadWrite)) {
		// �ǂݍ��݃I�[�v�������݂�
		if (!fio.Open(path, Fileio::ReadOnly)) {
			return FALSE;
		}

		// �ǂݍ��݂͉
		disk.writep = TRUE;
		disk.readonly = TRUE;
	}

	// �t�@�C���T�C�Y��737280�ł��邱�Ƃ��m���߂�
	size = fio.GetFileSize();
	if (size != 0xb4000) {
		fio.Close();
		return FALSE;
	}
	fio.Close();

	// �p�X�A�I�t�Z�b�g���L��
	disk.path = path;
	disk.offset = offset;

	// �f�B�X�N���̓t�@�C�����{�g���q�Ƃ���
	strcpy(disk.name, path.GetShort());

	// �g���b�N���쐬(0�`79�V�����_�܂ŁA80*2�g���b�N)
	for (i=0; i<160; i++) {
		track = new FDITrack2DD(this, i);
		AddTrack(track);
	}

	// �I��
	return TRUE;
}

//---------------------------------------------------------------------------
//
//	�V�[�N
//
//---------------------------------------------------------------------------
void FASTCALL FDIDisk2DD::Seek(int c)
{
	int i;
	FDITrack2DD *track;
	DWORD offset;

	ASSERT(this);
	ASSERT((c >= 0) && (c < 82));

	// �g���b�N�f�[�^����������
	for (i=0; i<2; i++) {
		// �g���b�N�����邩
		track = (FDITrack2DD*)GetHead(i);
		if (!track) {
			continue;
		}

		// �g���b�N�i���o����A�I�t�Z�b�g���Z�o(x0x1200)
		offset = track->GetTrack();
		offset *= 0x1200;

		// ��������
		track->Save(disk.path, offset);
	}

	// c��79�܂ŋ��B�͈͊O�ł����head[i]=NULL�Ƃ���
	if ((c < 0) || (c > 79)) {
		disk.head[0] = NULL;
		disk.head[1] = NULL;
		return;
	}

	// �Y������g���b�N���������A���[�h
	for (i=0; i<2; i++) {
		// �g���b�N������
		track = (FDITrack2DD*)Search(c * 2 + i);
		ASSERT(track);
		disk.head[i] = track;

		// �g���b�N�i���o����A�I�t�Z�b�g���Z�o(x0x1200)
		offset = track->GetTrack();
		offset *= 0x1200;

		// ���[�h
		track->Load(disk.path, offset);
	}
}

//---------------------------------------------------------------------------
//
//	�V�K�f�B�X�N�쐬
//
//---------------------------------------------------------------------------
BOOL FASTCALL FDIDisk2DD::Create(const Filepath& path, const option_t *opt)
{
	int i;
	FDITrack2DD *track;
	DWORD offset;
	Fileio fio;

	ASSERT(this);
	ASSERT(opt);

	// �����t�H�[�}�b�g��2DD�̂݋���
	if (opt->phyfmt != FDI_2DD) {
		return FALSE;
	}

	// �t�@�C���쐬�����݂�
	if (!fio.Open(path, Fileio::WriteOnly)) {
		return FALSE;
	}

	// �������݉\�Ƃ��ď�����
	disk.writep = FALSE;
	disk.readonly = FALSE;

	// �p�X���A�I�t�Z�b�g���L�^
	disk.path = path;
	disk.offset = 0;

	// �f�B�X�N���̓t�@�C�����{�g���q�Ƃ���
	strcpy(disk.name, path.GetShort());

	// 0�`159�Ɍ���A�g���b�N���쐬���ĕ����t�H�[�}�b�g
	for (i=0; i<160; i++) {
		track = new FDITrack2DD(this, i);
		track->Create(opt->phyfmt);
		AddTrack(track);
	}

	// �_���t�H�[�}�b�g
	FDIDisk::Create(path, opt);

	// �������݃��[�v
	offset = 0;
	for (i=0; i<160; i++) {
		// �g���b�N�擾
		track = (FDITrack2DD*)Search(i);
		ASSERT(track);
		ASSERT(track->IsChanged());

		// ��������
		if (!track->Save(&fio, offset)) {
			fio.Close();
			return FALSE;
		}

		// ����
		offset += (0x200 * 9);
	}

	// ����
	fio.Close();
	return TRUE;
}

//---------------------------------------------------------------------------
//
//	�t���b�V��
//
//---------------------------------------------------------------------------
BOOL FASTCALL FDIDisk2DD::Flush()
{
	int i;
	DWORD offset;
	FDITrack *track;

	ASSERT(this);

	// �Ō�̃g���b�N�f�[�^����������
	for (i=0; i<2; i++) {
		// �g���b�N�����邩
		track = GetHead(i);
		if (!track) {
			continue;
		}

		// �g���b�N�i���o����A�I�t�Z�b�g���Z�o(x0x1200)
		offset = track->GetTrack();
		offset *= 0x1200;

		// ��������
		if (!track->Save(disk.path, offset)) {
			return FALSE;
		}
	}

	return TRUE;
}

//===========================================================================
//
//	FDI�g���b�N(2HQ)
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	�R���X�g���N�^
//
//---------------------------------------------------------------------------
FDITrack2HQ::FDITrack2HQ(FDIDisk *disk, int track) : FDITrack(disk, track)
{
	ASSERT(disk);
}

//---------------------------------------------------------------------------
//
//	���[�h
//
//---------------------------------------------------------------------------
BOOL FASTCALL FDITrack2HQ::Load(const Filepath& path, DWORD offset)
{
	Fileio fio;
	BYTE buf[0x200];
	DWORD chrn[4];
	int i;
	FDISector *sector;

	ASSERT(this);
	ASSERT((offset % 0x2400) == 0);
	ASSERT(offset < 0x168000);

	// �������ς݂Ȃ�s�v(�V�[�N���ɌĂ΂��̂ŁA�P�x�����ǂ�ŃL���b�V������)
	if (IsInit()) {
		return TRUE;
	}

	// �Z�N�^�����݂��Ȃ�����
	ASSERT(!GetFirst());
	ASSERT(GetAllSectors() == 0);
	ASSERT(GetMFMSectors() == 0);
	ASSERT(GetFMSectors() == 0);

	// C�EH�EN����
	chrn[0] = GetTrack() >> 1;
	chrn[1] = GetTrack() & 1;
	chrn[3] = 2;

	// �ǂݍ��݃I�[�v��
	if (!fio.Open(path, Fileio::ReadOnly)) {
		return FALSE;
	}

	// �V�[�N
	if (!fio.Seek(offset)) {
		fio.Close();
		return FALSE;
	}

	// ���[�v
	for (i=0; i<18; i++) {
		// �f�[�^�ǂݍ���
		if (!fio.Read(buf, sizeof(buf))) {
			// �r���܂Œǉ����������폜����
			ClrSector();
			fio.Close();
			return FALSE;
		}

		// �Z�N�^�쐬
		chrn[2] = i + 1;
		sector = new FDISector(TRUE, chrn);
		sector->Load(buf, sizeof(buf), 0x54, FDD_NOERROR);

		// �Z�N�^�ǉ�
		AddSector(sector);
	}

	// �N���[�Y
	fio.Close();

	// �|�W�V�����v�Z
	CalcPos();

	// ������ok
	trk.init = TRUE;
	return TRUE;
}

//===========================================================================
//
//	FDI�f�B�X�N(2HQ)
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	�R���X�g���N�^
//
//---------------------------------------------------------------------------
FDIDisk2HQ::FDIDisk2HQ(int index, FDI *fdi) : FDIDisk(index, fdi)
{
	// ID�ݒ�
	disk.id = MAKEID('2', 'H', 'Q', ' ');
}

//---------------------------------------------------------------------------
//
//	�f�X�g���N�^
//
//---------------------------------------------------------------------------
FDIDisk2HQ::~FDIDisk2HQ()
{
	int i;
	DWORD offset;
	FDITrack *track;

	// �Ō�̃g���b�N�f�[�^����������
	for (i=0; i<2; i++) {
		// �g���b�N�����邩
		track = GetHead(i);
		if (!track) {
			continue;
		}

		// �g���b�N�i���o����A�I�t�Z�b�g���Z�o(x0x2400)
		offset = track->GetTrack();
		offset *= 0x2400;

		// ��������
		track->Save(disk.path, offset);
		disk.head[i] = NULL;
	}
}

//---------------------------------------------------------------------------
//
//	�I�[�v��
//
//---------------------------------------------------------------------------
BOOL FASTCALL FDIDisk2HQ::Open(const Filepath& path, DWORD offset)
{
	Fileio fio;
	DWORD size;
	FDITrack2HQ *track;
	int i;

	ASSERT(this);
	ASSERT(offset == 0);
	ASSERT(!GetFirst());
	ASSERT(!GetHead(0));
	ASSERT(!GetHead(1));

	// �������݉\�Ƃ��ď�����
	disk.writep = FALSE;
	disk.readonly = FALSE;

	// �I�[�v���ł��邱�Ƃ��m���߂�
	if (!fio.Open(path, Fileio::ReadWrite)) {
		// �ǂݍ��݃I�[�v�������݂�
		if (!fio.Open(path, Fileio::ReadOnly)) {
			return FALSE;
		}

		// �ǂݍ��݂͉
		disk.writep = TRUE;
		disk.readonly = TRUE;
	}

	// �t�@�C���T�C�Y��1474560�ł��邱�Ƃ��m���߂�
	size = fio.GetFileSize();
	if (size != 0x168000) {
		fio.Close();
		return FALSE;
	}
	fio.Close();

	// �p�X�A�I�t�Z�b�g���L��
	disk.path = path;
	disk.offset = offset;

	// �f�B�X�N���̓t�@�C�����{�g���q�Ƃ���
	strcpy(disk.name, path.GetShort());

	// �g���b�N���쐬(0�`79�V�����_�܂ŁA80*2�g���b�N)
	for (i=0; i<160; i++) {
		track = new FDITrack2HQ(this, i);
		AddTrack(track);
	}

	// �I��
	return TRUE;
}

//---------------------------------------------------------------------------
//
//	�V�[�N
//
//---------------------------------------------------------------------------
void FASTCALL FDIDisk2HQ::Seek(int c)
{
	int i;
	FDITrack2HQ *track;
	DWORD offset;

	ASSERT(this);
	ASSERT((c >= 0) && (c < 82));

	// �g���b�N�f�[�^����������
	for (i=0; i<2; i++) {
		// �g���b�N�����邩
		track = (FDITrack2HQ*)GetHead(i);
		if (!track) {
			continue;
		}

		// �g���b�N�i���o����A�I�t�Z�b�g���Z�o(x0x2400)
		offset = track->GetTrack();
		offset *= 0x2400;

		// ��������
		track->Save(disk.path, offset);
	}

	// c��79�܂ŋ��B�͈͊O�ł����head[i]=NULL�Ƃ���
	if ((c < 0) || (c > 79)) {
		disk.head[0] = NULL;
		disk.head[1] = NULL;
		return;
	}

	// �Y������g���b�N���������A���[�h
	for (i=0; i<2; i++) {
		// �g���b�N������
		track = (FDITrack2HQ*)Search(c * 2 + i);
		ASSERT(track);
		disk.head[i] = track;

		// �g���b�N�i���o����A�I�t�Z�b�g���Z�o(x0x2400)
		offset = track->GetTrack();
		offset *= 0x2400;

		// ���[�h
		track->Load(disk.path, offset);
	}
}

//---------------------------------------------------------------------------
//
//	�V�K�f�B�X�N�쐬
//
//---------------------------------------------------------------------------
BOOL FASTCALL FDIDisk2HQ::Create(const Filepath& path, const option_t *opt)
{
	int i;
	FDITrack2HQ *track;
	DWORD offset;
	Fileio fio;

	ASSERT(this);
	ASSERT(opt);

	// �����t�H�[�}�b�g��2HQ�̂݋���
	if (opt->phyfmt != FDI_2HQ) {
		return FALSE;
	}

	// �t�@�C���쐬�����݂�
	if (!fio.Open(path, Fileio::WriteOnly)) {
		return FALSE;
	}

	// �������݉\�Ƃ��ď�����
	disk.writep = FALSE;
	disk.readonly = FALSE;

	// �p�X���A�I�t�Z�b�g���L�^
	disk.path = path;
	disk.offset = 0;

	// �f�B�X�N���̓t�@�C�����{�g���q�Ƃ���
	strcpy(disk.name, path.GetShort());

	// 0�`159�Ɍ���A�g���b�N���쐬���ĕ����t�H�[�}�b�g
	for (i=0; i<160; i++) {
		track = new FDITrack2HQ(this, i);
		track->Create(opt->phyfmt);
		AddTrack(track);
	}

	// �_���t�H�[�}�b�g
	FDIDisk::Create(path, opt);

	// �������݃��[�v
	offset = 0;
	for (i=0; i<160; i++) {
		// �g���b�N�擾
		track = (FDITrack2HQ*)Search(i);
		ASSERT(track);
		ASSERT(track->IsChanged());

		// ��������
		if (!track->Save(&fio, offset)) {
			fio.Close();
			return FALSE;
		}

		// ����
		offset += (0x200 * 18);
	}

	// ����
	fio.Close();
	return TRUE;
}

//---------------------------------------------------------------------------
//
//	�t���b�V��
//
//---------------------------------------------------------------------------
BOOL FASTCALL FDIDisk2HQ::Flush()
{
	ASSERT(this);

	int i;
	DWORD offset;
	FDITrack *track;

	// �Ō�̃g���b�N�f�[�^����������
	for (i=0; i<2; i++) {
		// �g���b�N�����邩
		track = GetHead(i);
		if (!track) {
			continue;
		}

		// �g���b�N�i���o����A�I�t�Z�b�g���Z�o(x0x2400)
		offset = track->GetTrack();
		offset *= 0x2400;

		// ��������
		if (!track->Save(disk.path, offset)) {
			return FALSE;
		}
	}

	return TRUE;
}
