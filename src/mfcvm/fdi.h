//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2005 �o�h�D(ytanaka@ipc-tokai.or.jp)
//	[ �t���b�s�[�f�B�X�N�C���[�W ]
//
//---------------------------------------------------------------------------

#if !defined(fdi_h)
#define fdi_h

#include "filepath.h"

//---------------------------------------------------------------------------
//
//	�N���X��s��`
//
//---------------------------------------------------------------------------
class FDI;
class FDIDisk;
class FDITrack;
class FDISector;

class FDIDisk2HD;
class FDITrack2HD;
class FDIDiskDIM;
class FDITrackDIM;
class FDIDiskD68;
class FDITrackD68;
class FDIDiskBAD;
class FDITrackBAD;
class FDIDisk2DD;
class FDITrack2DD;
class FDIDisk2HQ;
class FDITrack2HQ;

//---------------------------------------------------------------------------
//
//	�����t�H�[�}�b�g��`
//
//---------------------------------------------------------------------------
#define FDI_2HD			0x00			// 2HD
#define FDI_2HDA		0x01			// 2HDA
#define FDI_2HS			0x02			// 2HS
#define FDI_2HC			0x03			// 2HC
#define FDI_2HDE		0x04			// 2HDE
#define FDI_2HQ			0x05			// 2HQ
#define FDI_N88B		0x06			// N88-BASIC
#define FDI_OS9			0x07			// OS-9/68000
#define FDI_2DD			0x08			// 2DD

//===========================================================================
//
//	FDI�Z�N�^
//
//===========================================================================
class FDISector
{
public:
	// �����f�[�^��`
	typedef struct {
		DWORD chrn[4];					// CHRN
		BOOL mfm;						// MFM�t���O
		int error;						// �G���[�R�[�h
		int length;						// �f�[�^��
		int gap3;						// GAP3
		BYTE *buffer;					// �f�[�^�o�b�t�@
		DWORD pos;						// �|�W�V����
		BOOL changed;					// �ύX�ς݃t���O
		FDISector *next;				// ���̃Z�N�^
	} sector_t;

public:
	// ��{�t�@���N�V����
	FDISector(BOOL mfm, const DWORD *chrn);
										// �R���X�g���N�^
	virtual ~FDISector();
										// �f�X�g���N�^
	void FASTCALL Load(const BYTE *buf, int len, int gap, int err);
										// �������[�h

	// ���[�h�E���C�g
	BOOL FASTCALL IsMatch(BOOL mfm, const DWORD *chrn) const;
										// �Z�N�^�}�b�`���邩
	void FASTCALL GetCHRN(DWORD *chrn)	const;
										// CHRN���擾
	BOOL FASTCALL IsMFM() const			{ return sec.mfm; }
										// MFM��
	int FASTCALL Read(BYTE *buf) const;
										// ���[�h
	int FASTCALL Write(const BYTE *buf, BOOL deleted);
										// ���C�g
	int FASTCALL Fill(DWORD d);
										// �t�B��

	// �v���p�e�B
	const BYTE* FASTCALL GetSector() const	{ return sec.buffer; }
										// �Z�N�^�f�[�^�擾
	int FASTCALL GetLength() const		{ return sec.length; }
										// �f�[�^���擾
	int FASTCALL GetError() const		{ return sec.error; }
										// �G���[�R�[�h�擾
	int FASTCALL GetGAP3() const		{ return sec.gap3; }
										// GAP3�o�C�g���擾

	// �|�W�V����
	void FASTCALL SetPos(DWORD pos)		{  sec.pos = pos; }
										// �|�W�V�����ݒ�
	DWORD FASTCALL GetPos() const		{ return sec.pos; }
										// �|�W�V�����擾

	// �ύX�t���O
	BOOL FASTCALL IsChanged() const		{ return sec.changed; }
										// �ύX�t���O�`�F�b�N
	void FASTCALL ClrChanged()			{ sec.changed = FALSE; }
										// �ύX�t���O�𗎂Ƃ�
	void FASTCALL ForceChanged()		{ sec.changed = TRUE; }
										// �ύX�t���O���グ��

	// �C���f�b�N�X�E�����N
	void FASTCALL SetNext(FDISector *next)	{ sec.next = next; }
										// ���̃Z�N�^��ݒ�
	FDISector* FASTCALL GetNext() const	{ return sec.next; }
										// ���̃Z�N�^���擾

private:
	// �����f�[�^
	sector_t sec;
										// �Z�N�^�����f�[�^
};

//===========================================================================
//
//	FDI�g���b�N
//
//===========================================================================
class FDITrack
{
public:
	// �������[�N
	typedef struct {
		FDIDisk *disk;					// �e�f�B�X�N
		int track;						// �g���b�N
		BOOL init;						// ���[�h�ς݂�
		int sectors[3];					// ���L�Z�N�^��(ALL/FM/MFM)
		BOOL hd;						// ���x�t���O
		BOOL mfm;						// �擪�Z�N�^MFM�t���O
		FDISector *first;				// �ŏ��̃Z�N�^
		FDITrack *next;					// ���̃g���b�N
	} track_t;

public:
	// ��{�t�@���N�V����
	FDITrack(FDIDisk *disk, int track, BOOL hd = TRUE);
										// �R���X�g���N�^
	virtual ~FDITrack();
										// �f�X�g���N�^
	virtual BOOL FASTCALL Save(Fileio *fio, DWORD offset);
										// �Z�[�u
	virtual BOOL FASTCALL Save(const Filepath& path, DWORD offset);
										// �Z�[�u
	void FASTCALL Create(DWORD phyfmt);
										// �����t�H�[�}�b�g
	BOOL FASTCALL IsHD() const			{ return trk.hd; }
										// HD�t���O�擾

	// ���[�h�E���C�g
	int FASTCALL ReadID(DWORD *buf, BOOL mfm);
										// ���[�hID
	int FASTCALL ReadSector(BYTE *buf, int *len, BOOL mfm, const DWORD *chrn);
										// ���[�h�Z�N�^
	int FASTCALL WriteSector(const BYTE *buf, int *len, BOOL mfm, const DWORD *chrn, BOOL deleted);
										// ���C�g�Z�N�^
	int FASTCALL ReadDiag(BYTE *buf, int *len, BOOL mfm, const DWORD *chrn);
										// ���[�h�_�C�A�O
	virtual int FASTCALL WriteID(const BYTE *buf, DWORD d, int sc, BOOL mfm, int gpl);
										// ���C�gID

	// �C���f�b�N�X�E�����N
	int FASTCALL GetTrack() const		{ return trk.track; }
										// ���̃g���b�N���擾
	void FASTCALL SetNext(FDITrack *p)	{ trk.next = p; }
										// ���̃g���b�N��ݒ�
	FDITrack* FASTCALL GetNext() const	{ return trk.next; }
										// ���̃g���b�N���擾

	// �Z�N�^
	BOOL FASTCALL IsChanged() const;
										// �ύX�t���O�`�F�b�N
	DWORD FASTCALL GetTotalLength() const;
										// �Z�N�^�����O�X�݌v�Z�o
	void FASTCALL ForceChanged();
										// �����ύX
	FDISector* FASTCALL Search(BOOL mfm, const DWORD *chrn);
										// �����ɍ����Z�N�^���T�[�`
	FDISector* FASTCALL GetFirst() const{ return trk.first; }
										// �ŏ��̃Z�N�^���擾

protected:
	// �����t�H�[�}�b�g
	void FASTCALL Create2HD(int lim);
										// �����t�H�[�}�b�g(2HD)
	void FASTCALL Create2HS();
										// �����t�H�[�}�b�g(2HS)
	void FASTCALL Create2HC();
										// �����t�H�[�}�b�g(2HC)
	void FASTCALL Create2HDE();
										// �����t�H�[�}�b�g(2HDE)
	void FASTCALL Create2HQ();
										// �����t�H�[�}�b�g(2HQ)
	void FASTCALL CreateN88B();
										// �����t�H�[�}�b�g(N88-BASIC)
	void FASTCALL CreateOS9();
										// �����t�H�[�}�b�g(OS-9)
	void FASTCALL Create2DD();
										// �����t�H�[�}�b�g(2DD)

	// �f�B�X�N�A��]�Ǘ�
	FDIDisk* FASTCALL GetDisk() const	{ return trk.disk; }
										// �f�B�X�N�擾
	BOOL FASTCALL IsMFM() const			{ return trk.mfm; }
										// �擪�Z�N�^��MFM��
	int FASTCALL GetGAP1() const;
										// GAP1�����擾
	int FASTCALL GetTotal() const;
										// �g�[�^�������擾
	void FASTCALL CalcPos();
										// �Z�N�^�擪�̈ʒu���Z�o
	DWORD FASTCALL GetSize(FDISector *sector) const;
										// �Z�N�^�̃T�C�Y(ID,GAP3�܂�)���擾
	FDISector* FASTCALL GetCurSector() const;
										// �J�����g�ʒu�ȍ~�̍ŏ��̃Z�N�^���擾

	// �g���b�N
	BOOL IsInit() const					{ return trk.init; }

	// �Z�N�^
	void FASTCALL AddSector(FDISector *sector);
										// �Z�N�^�ǉ�
	void FASTCALL ClrSector();
										// �Z�N�^�S�폜
	int FASTCALL GetAllSectors() const	{ return trk. sectors[0]; }
										// �Z�N�^���擾(All)
	int FASTCALL GetMFMSectors() const	{ return trk.sectors[1]; }
										// �Z�N�^���擾(MFM)
	int FASTCALL GetFMSectors() const	{ return trk.sectors[2]; }
										// �Z�N�^���擾(FM)

	// �_�C�A�O
	int FASTCALL MakeGAP1(BYTE *buf, int offset) const;
										// GAP1�쐬
	int FASTCALL MakeSector(BYTE *buf, int offset, FDISector *sector) const;
										// �Z�N�^�f�[�^�쐬
	int FASTCALL MakeGAP4(BYTE *buf, int offset) const;
										// GAP4�쐬
	int FASTCALL MakeData(BYTE *buf, int offset, BYTE data, int length) const;
										// Diag�f�[�^�쐬
	WORD FASTCALL CalcCRC(BYTE *buf, int length) const;
										// CRC�Z�o
	static const WORD CRCTable[0x100];
										// CRC�Z�o�e�[�u��

	// �����f�[�^
	track_t trk;
										// �g���b�N�����f�[�^
};

//===========================================================================
//
//	FDI�f�B�X�N
//
//===========================================================================
class FDIDisk
{
public:
	// �V�K�I�v�V������`
	typedef struct {
		DWORD phyfmt;					// �����t�H�[�}�b�g���
		BOOL logfmt;					// �_���t�H�[�}�b�g�L��
		char name[60];					// �R�����g(DIM)/�f�B�X�N��(D68)
	} option_t;

	// �����f�[�^��`
	typedef struct {
		int index;						// �C���f�b�N�X
		FDI *fdi;						// �eFDI
		DWORD id;						// ID
		BOOL writep;					// �������݋֎~
		BOOL readonly;					// �ǂݍ��ݐ�p
		char name[60];					// �f�B�X�N��
		Filepath path;					// �p�X
		DWORD offset;					// �t�@�C���̃I�t�Z�b�g
		FDITrack *first;				// �ŏ��̃g���b�N
		FDITrack *head[2];				// �w�b�h�ɑΉ������g���b�N
		int search;						// ��������(�P��=0x10000)
		FDIDisk *next;					// ���̃f�B�X�N
	} disk_t;

public:
	FDIDisk(int index, FDI *fdi);
										// �R���X�g���N�^
	virtual ~FDIDisk();
										// �f�X�g���N�^
	DWORD FASTCALL GetID() const		{ return disk.id; }
										// ID�擾

	// ���f�B�A����
	virtual BOOL FASTCALL Create(const Filepath& path, const option_t *opt);
										// �V�K�f�B�X�N�쐬
	virtual BOOL FASTCALL Open(const Filepath& path, DWORD offset = 0);
										// �I�[�v��
	void FASTCALL GetName(char *buf) const;
										// �f�B�X�N���擾
	void FASTCALL GetPath(Filepath& path) const;
										// �p�X�擾
	BOOL FASTCALL IsWriteP() const		{ return disk.writep; }
										// ���C�g�v���e�N�g��
	BOOL FASTCALL IsReadOnly() const	{ return disk.readonly; }
										// Read Only�f�B�X�N�C���[�W��
	void FASTCALL WriteP(BOOL flag);
										// �������݋֎~���Z�b�g
	virtual BOOL FASTCALL Flush();
										// �o�b�t�@���t���b�V��

	// �A�N�Z�X
	virtual void FASTCALL Seek(int c);
										// �V�[�N
	int FASTCALL ReadID(DWORD *buf, BOOL mfm, int hd);
										// ���[�hID
	int FASTCALL ReadSector(BYTE *buf, int *len, BOOL mfm, const DWORD *chrn, int hd);
										// ���[�h�Z�N�^
	int FASTCALL WriteSector(const BYTE *buf, int *len, BOOL mfm, const DWORD *chrn, int hd, BOOL deleted);
										// ���C�g�Z�N�^
	int FASTCALL ReadDiag(BYTE *buf, int *len, BOOL mfm, const DWORD *chrn, int hd);
										// ���[�h�_�C�A�O
	int FASTCALL WriteID(const BYTE *buf, DWORD d, int sc, BOOL mfm, int hd, int gpl);
										// ���C�gID

	// ��]�Ǘ�
	DWORD FASTCALL GetRotationPos() const;
										// ��]�ʒu�擾
	DWORD FASTCALL GetRotationTime() const;
										// ��]���Ԏ擾
	DWORD FASTCALL GetSearch() const	{ return disk.search; }
										// ���������擾
	void FASTCALL SetSearch(DWORD len)	{ disk.search = len; }
										// ���������ݒ�
	void FASTCALL CalcSearch(DWORD pos);
										// ���������Z�o
	BOOL FASTCALL IsHD() const;
										// �h���C�uHD��Ԏ擾
	FDITrack* FASTCALL Search(int track) const;
										// �g���b�N�T�[�`

	// �C���f�b�N�X�E�����N
	int FASTCALL GetIndex() const		{ return disk.index; }
										// �C���f�b�N�X�擾
	void FASTCALL SetNext(FDIDisk *p)	{ disk.next = p; }
										// ���̃g���b�N��ݒ�
	FDIDisk* FASTCALL GetNext() const	{ return disk.next; }
										// ���̃g���b�N���擾

protected:
	// �_���t�H�[�}�b�g
	void FASTCALL Create2HD(BOOL flag2hd);
										// �_���t�H�[�}�b�g(2HD, 2HDA)
	static const BYTE IPL2HD[0x200];
										// IPL(2HD, 2HDA)
	void FASTCALL Create2HS();
										// �_���t�H�[�}�b�g(2HS)
	static const BYTE IPL2HS[0x800];
										// IPL(2HS)
	void FASTCALL Create2HC();
										// �_���t�H�[�}�b�g(2HC)
	static const BYTE IPL2HC[0x200];
										// IPL(2HC)
	void FASTCALL Create2HDE();
										// �_���t�H�[�}�b�g(2HDE)
	static const BYTE IPL2HDE[0x800];
										// IPL(2HDE)
	void FASTCALL Create2HQ();
										// �_���t�H�[�}�b�g(2HQ)
	static const BYTE IPL2HQ[0x200];
										// IPL(2HQ)
	void FASTCALL Create2DD();
										// �_���t�H�[�}�b�g(2DD)

	// �C���[�W
	FDI* FASTCALL GetFDI() const		{ return disk.fdi; }
										// �e�C���[�W�擾

	// �g���b�N
	void FASTCALL AddTrack(FDITrack *track);
										// �g���b�N�ǉ�
	void FASTCALL ClrTrack();
										// �g���b�N�S�폜
	FDITrack* FASTCALL GetFirst() const	{ return disk.first; }
										// �ŏ��̃g���b�N���擾
	FDITrack* FASTCALL GetHead(int idx) { ASSERT((idx==0)||(idx==1)); return disk.head[idx]; }
										// �w�b�h�ɑΉ�����g���b�N���擾

	// �����f�[�^
	disk_t disk;
										// �f�B�X�N�����f�[�^
};

//===========================================================================
//
//	FDI
//
//===========================================================================
class FDI
{
public:
	// �����f�[�^��`
	typedef struct {
		FDD *fdd;						// FDD
		int disks;						// �f�B�X�N��
		FDIDisk *first;					// �ŏ��̃f�B�X�N
		FDIDisk *disk;					// ���݂̃f�B�X�N
	} fdi_t;

public:
	FDI(FDD *fdd);
										// �R���X�g���N�^
	virtual ~FDI();
										// �f�X�g���N�^

	// ���f�B�A����
	BOOL FASTCALL Open(const Filepath& path, int media);
										// �I�[�v��
	DWORD FASTCALL GetID() const;
										// ID�擾
	BOOL FASTCALL IsMulti() const;
										// �}���`�f�B�X�N�C���[�W��
	FDIDisk* GetDisk() const			{ return fdi.disk; }
										// ���݂̃f�B�X�N���擾
	int FASTCALL GetDisks() const		{ return fdi.disks; }
										// �f�B�X�N�����擾
	int FASTCALL GetMedia() const;
										// �}���`�f�B�X�N�C���f�b�N�X�擾
	void FASTCALL GetName(char *buf, int index = -1) const;
										// �f�B�X�N���擾
	void FASTCALL GetPath(Filepath& path) const;
										// �p�X�擾
	BOOL FASTCALL Save(Fileio *fio, int ver);
										// �Z�[�u
	BOOL FASTCALL Load(Fileio *fio, int ver, BOOL *ready, int *media, Filepath& path);
										// ���[�h
	void FASTCALL Adjust();
										// ����(����)

	// ���f�B�A���
	BOOL FASTCALL IsReady() const;
										// ���f�B�A���Z�b�g����Ă��邩
	BOOL FASTCALL IsWriteP() const;
										// ���C�g�v���e�N�g��
	BOOL FASTCALL IsReadOnly() const;
										// Read Only�f�B�X�N�C���[�W��
	void FASTCALL WriteP(BOOL flag);
										// �������݋֎~���Z�b�g

	// �A�N�Z�X
	void FASTCALL Seek(int c);
										// �V�[�N
	int FASTCALL ReadID(DWORD *buf, BOOL mfm, int hd);
										// ���[�hID
	int FASTCALL ReadSector(BYTE *buf, int *len, BOOL mfm, const DWORD *chrn, int hd);
										// ���[�h�Z�N�^
	int FASTCALL WriteSector(const BYTE *buf, int *len, BOOL mfm, const DWORD *chrn, int hd, BOOL deleted);
										// ���C�g�Z�N�^
	int FASTCALL ReadDiag(BYTE *buf, int *len, BOOL mfm, const DWORD *chrn, int hd);
										// ���[�h�_�C�A�O
	int FASTCALL WriteID(const BYTE *buf, DWORD d, int sc, BOOL mfm, int hd, int gpl);
										// ���C�gID

	// ��]�Ǘ�
	DWORD FASTCALL GetRotationPos() const;
										// ��]�ʒu�擾
	DWORD FASTCALL GetRotationTime() const;
										// ��]���Ԏ擾
	DWORD FASTCALL GetSearch() const;
										// �������Ԏ擾
	BOOL FASTCALL IsHD() const;
										// �h���C�uHD��Ԏ擾

private:
	// �h���C�u
	FDD* FASTCALL GetFDD() const		{ return fdi.fdd; }

	// �f�B�X�N
	void FASTCALL AddDisk(FDIDisk *disk);
										// �f�B�X�N�ǉ�
	void FASTCALL ClrDisk();
										// �f�B�X�N�S�폜
	FDIDisk* GetFirst() const			{ return fdi.first; }
										// �ŏ��̃f�B�X�N���擾
	FDIDisk* FASTCALL Search(int index) const;
										// �f�B�X�N�T�[�`

	// �����f�[�^
	fdi_t fdi;
										// FDI�����f�[�^
};

//===========================================================================
//
//	FDI�g���b�N(2HD)
//
//===========================================================================
class FDITrack2HD : public FDITrack
{
public:
	// ��{�t�@���N�V����
	FDITrack2HD(FDIDisk *disk, int track);
										// �R���X�g���N�^
	BOOL FASTCALL Load(const Filepath& path, DWORD offset);
										// ���[�h
};

//===========================================================================
//
//	FDI�f�B�X�N(2HD)
//
//===========================================================================
class FDIDisk2HD : public FDIDisk
{
public:
	FDIDisk2HD(int index, FDI *fdi);
										// �R���X�g���N�^
	virtual ~FDIDisk2HD();
										// �f�X�g���N�^
	BOOL FASTCALL Open(const Filepath& path, DWORD offset = 0);
										// �I�[�v��
	void FASTCALL Seek(int c);
										// �V�[�N
	BOOL FASTCALL Create(const Filepath& path, const option_t *opt);
										// �V�K�f�B�X�N�쐬
	BOOL FASTCALL Flush();
										// �o�b�t�@���t���b�V��
};

//===========================================================================
//
//	FDI�g���b�N(DIM)
//
//===========================================================================
class FDITrackDIM : public FDITrack
{
public:
	// ��{�t�@���N�V����
	FDITrackDIM(FDIDisk *disk, int track, int type);
										// �R���X�g���N�^
	BOOL FASTCALL Load(const Filepath& path, DWORD offset, BOOL load);
										// ���[�h
	BOOL FASTCALL IsDIMMFM() const		{ return dim_mfm; }
										// DIM MFM�t���O�擾
	int FASTCALL GetDIMSectors() const	{ return dim_secs; }
										// DIM �Z�N�^���擾
	int FASTCALL GetDIMN() const		{ return dim_n; }
										// DIM �����O�X�擾

private:
	BOOL dim_mfm;
										// DIM MFM�t���O
	int dim_secs;
										// DIM �Z�N�^��
	int dim_n;
										// DIM �����O�X
	int dim_type;
										// DIM �^�C�v
};

//===========================================================================
//
//	FDI�f�B�X�N(DIM)
//
//===========================================================================
class FDIDiskDIM : public FDIDisk
{
public:
	FDIDiskDIM(int index, FDI *fdi);
										// �R���X�g���N�^
	virtual ~FDIDiskDIM();
										// �f�X�g���N�^
	BOOL FASTCALL Open(const Filepath& path, DWORD offset = 0);
										// �I�[�v��
	void FASTCALL Seek(int c);
										// �V�[�N
	BOOL FASTCALL Create(const Filepath& path, const option_t *opt);
										// �V�K�f�B�X�N�쐬
	BOOL FASTCALL Flush();
										// �o�b�t�@���t���b�V��

private:
	BOOL FASTCALL GetDIMMap(int track) const;
										// �g���b�N�}�b�v�擾
	DWORD FASTCALL GetDIMOffset(int track) const;
										// �g���b�N�I�t�Z�b�g�擾
	BOOL FASTCALL Save();
										// �폜�O�̕ۑ�
	BYTE dim_hdr[0x100];
										// DIM�w�b�_
	BOOL dim_load;
										// �w�b�_�m�F�t���O
};

//===========================================================================
//
//	FDI�g���b�N(D68)
//
//===========================================================================
class FDITrackD68 : public FDITrack
{
public:
	// ��{�t�@���N�V����
	FDITrackD68(FDIDisk *disk, int track, BOOL hd);
										// �R���X�g���N�^
	BOOL FASTCALL Load(const Filepath& path, DWORD offset);
										// ���[�h
	BOOL FASTCALL Save(const Filepath& path, DWORD offset);
										// �Z�[�u
	BOOL FASTCALL Save(Fileio *fio, DWORD offset);
										// �Z�[�u
	int FASTCALL WriteID(const BYTE *buf, DWORD d, int sc, BOOL mfm, int gpl);
										// ���C�gID
	void FASTCALL ForceFormat()			{ d68_format = TRUE; }
										// �����t�H�[�}�b�g
	BOOL FASTCALL IsFormated() const	{ return d68_format; }
										// �t�H�[�}�b�g�ύX����Ă��邩
	DWORD FASTCALL GetD68Length() const;
										// D68�`���ł̒������擾

private:
	BOOL d68_format;
										// �t�H�[�}�b�g�ύX�t���O
	static const int Gap3Table[];
										// GAP3�e�[�u��
};

//===========================================================================
//
//	FDI�f�B�X�N(D68)
//
//===========================================================================
class FDIDiskD68 : public FDIDisk
{
public:
	FDIDiskD68(int index, FDI *fdi);
										// �R���X�g���N�^
	virtual ~FDIDiskD68();
										// �f�X�g���N�^
	BOOL FASTCALL Open(const Filepath& path, DWORD offset = 0);
										// �I�[�v��
	void FASTCALL Seek(int c);
										// �V�[�N
	void FASTCALL AdjustOffset();
										// �I�t�Z�b�g�X�V
	static int FASTCALL CheckDisks(const Filepath& path, DWORD *offbuf);
										// D68�w�b�_�`�F�b�N
	BOOL FASTCALL Create(const Filepath& path, const option_t *opt);
										// �V�K�f�B�X�N�쐬
	BOOL FASTCALL Flush();
										// �o�b�t�@���t���b�V��

private:
	DWORD FASTCALL GetD68Offset(int track) const;
										// �g���b�N�I�t�Z�b�g�擾
	BOOL FASTCALL Save();
										// �폜�O�̕ۑ�
	BYTE d68_hdr[0x2b0];
										// D68�w�b�_
	BOOL d68_load;
										// �w�b�_�m�F�t���O
};

//===========================================================================
//
//	FDI�g���b�N(BAD)
//
//===========================================================================
class FDITrackBAD : public FDITrack
{
public:
	// ��{�t�@���N�V����
	FDITrackBAD(FDIDisk *disk, int track);
										// �R���X�g���N�^
	BOOL FASTCALL Load(const Filepath& path, DWORD offset);
										// ���[�h
	BOOL FASTCALL Save(const Filepath& path, DWORD offset);
										// �Z�[�u

private:
	int bad_secs;
										// �Z�N�^��
};

//===========================================================================
//
//	FDI�f�B�X�N(BAD)
//
//===========================================================================
class FDIDiskBAD : public FDIDisk
{
public:
	FDIDiskBAD(int index, FDI *fdi);
										// �R���X�g���N�^
	virtual ~FDIDiskBAD();
										// �f�X�g���N�^
	BOOL FASTCALL Open(const Filepath& path, DWORD offset = 0);
										// �I�[�v��
	void FASTCALL Seek(int c);
										// �V�[�N
	BOOL FASTCALL Flush();
										// �o�b�t�@���t���b�V��
};

//===========================================================================
//
//	FDI�g���b�N(2DD)
//
//===========================================================================
class FDITrack2DD : public FDITrack
{
public:
	// ��{�t�@���N�V����
	FDITrack2DD(FDIDisk *disk, int track);
										// �R���X�g���N�^
	BOOL FASTCALL Load(const Filepath& path, DWORD offset);
										// ���[�h
};

//===========================================================================
//
//	FDI�f�B�X�N(2DD)
//
//===========================================================================
class FDIDisk2DD : public FDIDisk
{
public:
	FDIDisk2DD(int index, FDI *fdi);
										// �R���X�g���N�^
	virtual ~FDIDisk2DD();
										// �f�X�g���N�^
	BOOL FASTCALL Open(const Filepath& path, DWORD offset = 0);
										// �I�[�v��
	void FASTCALL Seek(int c);
										// �V�[�N
	BOOL FASTCALL Create(const Filepath& path, const option_t *opt);
										// �V�K�f�B�X�N�쐬
	BOOL FASTCALL Flush();
										// �o�b�t�@���t���b�V��
};

//===========================================================================
//
//	FDI�g���b�N(2HQ)
//
//===========================================================================
class FDITrack2HQ : public FDITrack
{
public:
	// ��{�t�@���N�V����
	FDITrack2HQ(FDIDisk *disk, int track);
										// �R���X�g���N�^
	BOOL FASTCALL Load(const Filepath& path, DWORD offset);
										// ���[�h
};

//===========================================================================
//
//	FDI�f�B�X�N(2HQ)
//
//===========================================================================
class FDIDisk2HQ : public FDIDisk
{
public:
	FDIDisk2HQ(int index, FDI *fdi);
										// �R���X�g���N�^
	virtual ~FDIDisk2HQ();
										// �f�X�g���N�^
	BOOL FASTCALL Open(const Filepath& path, DWORD offset = 0);
										// �I�[�v��
	void FASTCALL Seek(int c);
										// �V�[�N
	BOOL FASTCALL Create(const Filepath& path, const option_t *opt);
										// �V�K�f�B�X�N�쐬
	BOOL FASTCALL Flush();
										// �o�b�t�@���t���b�V��
};

#endif	// fdi_h
