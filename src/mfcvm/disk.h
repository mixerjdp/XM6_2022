//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 �o�h�D(ytanaka@ipc-tokai.or.jp)
//	[ �f�B�X�N ]
//
//---------------------------------------------------------------------------

#if !defined(disk_h)
#define disk_h

//---------------------------------------------------------------------------
//
//	�N���X��s��`
//
//---------------------------------------------------------------------------
class DiskTrack;
class DiskCache;
class Disk;
class SASIHD;
class SCSIHD;
class SCSIMO;
class SCSICDTrack;
class SCSICD;

//---------------------------------------------------------------------------
//
//	�G���[��`(REQUEST SENSE�ŕԂ����Z���X�R�[�h)
//
//	MSB		�\��(0x00)
//			�Z���X�L�[
//			�g���Z���X�R�[�h(ASC)
//	LSB		�g���Z���X�R�[�h�N�H���t�@�C�A(ASCQ)
//
//---------------------------------------------------------------------------
#define DISK_NOERROR		0x00000000	// NO ADDITIONAL SENSE INFO.
#define DISK_DEVRESET		0x00062900	// POWER ON OR RESET OCCURED
#define DISK_NOTREADY		0x00023a00	// MEDIUM NOT PRESENT
#define DISK_ATTENTION		0x00062800	// MEDIUIM MAY HAVE CHANGED
#define DISK_PREVENT		0x00045302	// MEDIUM REMOVAL PREVENTED
#define DISK_READFAULT		0x00031100	// UNRECOVERED READ ERROR
#define DISK_WRITEFAULT		0x00030300	// PERIPHERAL DEVICE WRITE FAULT
#define DISK_WRITEPROTECT	0x00042700	// WRITE PROTECTED
#define DISK_MISCOMPARE		0x000e1d00	// MISCOMPARE DURING VERIFY
#define DISK_INVALIDCMD		0x00052000	// INVALID COMMAND OPERATION CODE
#define DISK_INVALIDLBA		0x00052100	// LOGICAL BLOCK ADDR. OUT OF RANGE
#define DISK_INVALIDCDB		0x00052400	// INVALID FIELD IN CDB
#define DISK_INVALIDLUN		0x00052500	// LOGICAL UNIT NOT SUPPORTED
#define DISK_INVALIDPRM		0x00052600	// INVALID FIELD IN PARAMETER LIST
#define DISK_INVALIDMSG		0x00054900	// INVALID MESSAGE ERROR
#define DISK_PARAMLEN		0x00051a00	// PARAMETERS LIST LENGTH ERROR
#define DISK_PARAMNOT		0x00052601	// PARAMETERS NOT SUPPORTED
#define DISK_PARAMVALUE		0x00052602	// PARAMETERS VALUE INVALID
#define DISK_PARAMSAVE		0x00053900	// SAVING PARAMETERS NOT SUPPORTED

#if 0
#define DISK_AUDIOPROGRESS	0x00??0011	// AUDIO PLAY IN PROGRESS
#define DISK_AUDIOPAUSED	0x00??0012	// AUDIO PLAY PAUSED
#define DISK_AUDIOSTOPPED	0x00??0014	// AUDIO PLAY STOPPED DUE TO ERROR
#define DISK_AUDIOCOMPLETE	0x00??0013	// AUDIO PLAY SUCCESSFULLY COMPLETED
#endif

//===========================================================================
//
//	�f�B�X�N�g���b�N
//
//===========================================================================
class DiskTrack
{
public:
	// �����f�[�^��`
	typedef struct {
		int track;						// �g���b�N�i���o�[
		int size;						// �Z�N�^�T�C�Y(8 or 9)
		int sectors;					// �Z�N�^��(<=0x100)
		BYTE *buffer;					// �f�[�^�o�b�t�@
		BOOL init;						// ���[�h�ς݂�
		BOOL changed;					// �ύX�ς݃t���O
		BOOL *changemap;				// �ύX�ς݃}�b�v
		BOOL raw;						// RAW���[�h
	} disktrk_t;

public:
	// ��{�t�@���N�V����
	DiskTrack(int track, int size, int sectors, BOOL raw = FALSE);
										// �R���X�g���N�^
	virtual ~DiskTrack();
										// �f�X�g���N�^
	BOOL FASTCALL Load(const Filepath& path);
										// ���[�h
	BOOL FASTCALL Save(const Filepath& path);
										// �Z�[�u

	// ���[�h�E���C�g
	BOOL FASTCALL Read(BYTE *buf, int sec) const;
										// �Z�N�^���[�h
	BOOL FASTCALL Write(const BYTE *buf, int sec);
										// �Z�N�^���C�g

	// ���̑�
	int FASTCALL GetTrack() const		{ return dt.track; }
										// �g���b�N�擾
	BOOL FASTCALL IsChanged() const		{ return dt.changed; }
										// �ύX�t���O�`�F�b�N

private:
	// �����f�[�^
	disktrk_t dt;
										// �����f�[�^
};

//===========================================================================
//
//	�f�B�X�N�L���b�V��
//
//===========================================================================
class DiskCache
{
public:
	// �����f�[�^��`
	typedef struct {
		DiskTrack *disktrk;				// ���蓖�ăg���b�N
		DWORD serial;					// �ŏI�V���A��
	} cache_t;

	// �L���b�V����
	enum {
		CacheMax = 16					// �L���b�V������g���b�N��
	};

public:
	// ��{�t�@���N�V����
	DiskCache(const Filepath& path, int size, int blocks);
										// �R���X�g���N�^
	virtual ~DiskCache();
										// �f�X�g���N�^
	void FASTCALL SetRawMode(BOOL raw);
										// CD-ROM raw���[�h�ݒ�

	// �A�N�Z�X
	BOOL FASTCALL Save();
										// �S�Z�[�u�����
	BOOL FASTCALL Read(BYTE *buf, int block);
										// �Z�N�^���[�h
	BOOL FASTCALL Write(const BYTE *buf, int block);
										// �Z�N�^���C�g
	BOOL FASTCALL GetCache(int index, int& track, DWORD& serial) const;
										// �L���b�V�����擾

private:
	// �����Ǘ�
	void FASTCALL Clear();
										// �g���b�N�����ׂăN���A
	DiskTrack* FASTCALL Assign(int track);
										// �g���b�N�̃��[�h
	BOOL FASTCALL Load(int index, int track);
										// �g���b�N�̃��[�h
	void FASTCALL Update();
										// �V���A���ԍ��X�V

	// �����f�[�^
	cache_t cache[CacheMax];
										// �L���b�V���Ǘ�
	DWORD serial;
										// �ŏI�A�N�Z�X�V���A���i���o
	Filepath sec_path;
										// �p�X
	int sec_size;
										// �Z�N�^�T�C�Y(8 or 9 or 11)
	int sec_blocks;
										// �Z�N�^�u���b�N��
	BOOL cd_raw;
										// CD-ROM RAW���[�h
};

//===========================================================================
//
//	�f�B�X�N
//
//===========================================================================
class Disk
{
public:
	// �������[�N
	typedef struct {
		DWORD id;						// ���f�B�AID
		BOOL ready;						// �L���ȃf�B�X�N
		BOOL writep;					// �������݋֎~
		BOOL readonly;					// �ǂݍ��ݐ�p
		BOOL removable;					// ���O��
		BOOL lock;						// ���b�N
		BOOL attn;						// �A�e���V����
		BOOL reset;						// ���Z�b�g
		int size;						// �Z�N�^�T�C�Y
		int blocks;						// ���Z�N�^��
		DWORD lun;						// LUN
		DWORD code;						// �X�e�[�^�X�R�[�h
		DiskCache *dcache;				// �f�B�X�N�L���b�V��
	} disk_t;

public:
	// ��{�t�@���N�V����
	Disk(Device *dev);
										// �R���X�g���N�^
	virtual ~Disk();
										// �f�X�g���N�^
	virtual void FASTCALL Reset();
										// �f�o�C�X���Z�b�g
	virtual BOOL FASTCALL Save(Fileio *fio, int ver);
										// �Z�[�u
	virtual BOOL FASTCALL Load(Fileio *fio, int ver);
										// ���[�h

	// ID
	DWORD FASTCALL GetID() const		{ return disk.id; }
										// ���f�B�AID�擾
	BOOL FASTCALL IsNULL() const;
										// NULL�`�F�b�N
	BOOL FASTCALL IsSASI() const;
										// SASI�`�F�b�N

	// ���f�B�A����
	virtual BOOL FASTCALL Open(const Filepath& path);
										// �I�[�v��
	void FASTCALL GetPath(Filepath& path) const;
										// �p�X�擾
	void FASTCALL Eject(BOOL force);
										// �C�W�F�N�g
	BOOL FASTCALL IsReady() const		{ return disk.ready; }
										// Ready�`�F�b�N
	void FASTCALL WriteP(BOOL flag);
										// �������݋֎~
	BOOL FASTCALL IsWriteP() const		{ return disk.writep; }
										// �������݋֎~�`�F�b�N
	BOOL FASTCALL IsReadOnly() const	{ return disk.readonly; }
										// Read Only�`�F�b�N
	BOOL FASTCALL IsRemovable() const	{ return disk.removable; }
										// �����[�o�u���`�F�b�N
	BOOL FASTCALL IsLocked() const		{ return disk.lock; }
										// ���b�N�`�F�b�N
	BOOL FASTCALL IsAttn() const		{ return disk.attn; }
										// �����`�F�b�N
	BOOL FASTCALL Flush();
										// �L���b�V���t���b�V��
	void FASTCALL GetDisk(disk_t *buffer) const;
										// �������[�N�擾

	// �v���p�e�B
	void FASTCALL SetLUN(DWORD lun)		{ disk.lun = lun; }
										// LUN�Z�b�g
	DWORD FASTCALL GetLUN()				{ return disk.lun; }
										// LUN�擾

	// �R�}���h
	virtual int FASTCALL Inquiry(const DWORD *cdb, BYTE *buf);
										// INQUIRY�R�}���h
	virtual int FASTCALL RequestSense(const DWORD *cdb, BYTE *buf);
										// REQUEST SENSE�R�}���h
	int FASTCALL SelectCheck(const DWORD *cdb);
										// SELECT�`�F�b�N
	BOOL FASTCALL ModeSelect(const BYTE *buf, int size);
										// MODE SELECT�R�}���h
	int FASTCALL ModeSense(const DWORD *cdb, BYTE *buf);
										// MODE SENSE�R�}���h
	BOOL FASTCALL TestUnitReady(const DWORD *cdb);
										// TEST UNIT READY�R�}���h
	BOOL FASTCALL Rezero(const DWORD *cdb);
										// REZERO�R�}���h
	BOOL FASTCALL Format(const DWORD *cdb);
										// FORMAT UNIT�R�}���h
	BOOL FASTCALL Reassign(const DWORD *cdb);
										// REASSIGN UNIT�R�}���h
	virtual int FASTCALL Read(BYTE *buf, int block);
										// READ�R�}���h
	int FASTCALL WriteCheck(int block);
										// WRITE�`�F�b�N
	BOOL FASTCALL Write(const BYTE *buf, int block);
										// WRITE�R�}���h
	BOOL FASTCALL Seek(const DWORD *cdb);
										// SEEK�R�}���h
	BOOL FASTCALL StartStop(const DWORD *cdb);
										// START STOP UNIT�R�}���h
	BOOL FASTCALL SendDiag(const DWORD *cdb);
										// SEND DIAGNOSTIC�R�}���h
	BOOL FASTCALL Removal(const DWORD *cdb);
										// PREVENT/ALLOW MEDIUM REMOVAL�R�}���h
	int FASTCALL ReadCapacity(const DWORD *cdb, BYTE *buf);
										// READ CAPACITY�R�}���h
	BOOL FASTCALL Verify(const DWORD *cdb);
										// VERIFY�R�}���h
	virtual int FASTCALL ReadToc(const DWORD *cdb, BYTE *buf);
										// READ TOC�R�}���h
	virtual BOOL FASTCALL PlayAudio(const DWORD *cdb);
										// PLAY AUDIO�R�}���h
	virtual BOOL FASTCALL PlayAudioMSF(const DWORD *cdb);
										// PLAY AUDIO MSF�R�}���h
	virtual BOOL FASTCALL PlayAudioTrack(const DWORD *cdb);
										// PLAY AUDIO TRACK�R�}���h
	void FASTCALL InvalidCmd()			{ disk.code = DISK_INVALIDCMD; }
										// �T�|�[�g���Ă��Ȃ��R�}���h

protected:
	// �T�u����
	int FASTCALL AddError(BOOL change, BYTE *buf);
										// �G���[�y�[�W�ǉ�
	int FASTCALL AddFormat(BOOL change, BYTE *buf);
										// �t�H�[�}�b�g�y�[�W�ǉ�
	int FASTCALL AddOpt(BOOL change, BYTE *buf);
										// �I�v�e�B�J���y�[�W�ǉ�
	int FASTCALL AddCache(BOOL change, BYTE *buf);
										// �L���b�V���y�[�W�ǉ�
	int FASTCALL AddCDROM(BOOL change, BYTE *buf);
										// CD-ROM�y�[�W�ǉ�
	int FASTCALL AddCDDA(BOOL change, BYTE *buf);
										// CD-DA�y�[�W�ǉ�
	BOOL FASTCALL CheckReady();
										// ���f�B�`�F�b�N

	// �����f�[�^
	disk_t disk;
										// �f�B�X�N�����f�[�^
	Device *ctrl;
										// �R���g���[���f�o�C�X
	Filepath diskpath;
										// �p�X(GetPath�p)
};

//===========================================================================
//
//	SASI �n�[�h�f�B�X�N
//
//===========================================================================
class SASIHD : public Disk
{
public:
	// ��{�t�@���N�V����
	SASIHD(Device *dev);
										// �R���X�g���N�^
	BOOL FASTCALL Open(const Filepath& path);
										// �I�[�v��

	// ���f�B�A����
	void FASTCALL Reset();
										// �f�o�C�X���Z�b�g

	// �R�}���h
	int FASTCALL RequestSense(const DWORD *cdb, BYTE *buf);
										// REQUEST SENSE�R�}���h
};

//===========================================================================
//
//	SCSI �n�[�h�f�B�X�N
//
//===========================================================================
class SCSIHD : public Disk
{
public:
	// ��{�t�@���N�V����
	SCSIHD(Device *dev);
										// �R���X�g���N�^
	BOOL FASTCALL Open(const Filepath& path);
										// �I�[�v��

	// �R�}���h
	int FASTCALL Inquiry(const DWORD *cdb, BYTE *buf);
										// INQUIRY�R�}���h
};

//===========================================================================
//
//	SCSI �����C�f�B�X�N
//
//===========================================================================
class SCSIMO : public Disk
{
public:
	// ��{�t�@���N�V����
	SCSIMO(Device *dev);
										// �R���X�g���N�^
	BOOL FASTCALL Open(const Filepath& path, BOOL attn = TRUE);
										// �I�[�v��
	BOOL FASTCALL Load(Fileio *fio, int ver);
										// ���[�h

	// �R�}���h
	int FASTCALL Inquiry(const DWORD *cdb, BYTE *buf);
										// INQUIRY�R�}���h
};

//===========================================================================
//
//	CD-ROM �g���b�N
//
//===========================================================================
class CDTrack
{
public:
	// ��{�t�@���N�V����
	CDTrack(SCSICD *scsicd);
										// �R���X�g���N�^
	virtual ~CDTrack();
										// �f�X�g���N�^
	BOOL FASTCALL Init(int track, DWORD first, DWORD last);
										// ������

	// �v���p�e�B
	void FASTCALL SetPath(BOOL cdda, const Filepath& path);
										// �p�X�ݒ�
	void FASTCALL GetPath(Filepath& path) const;
										// �p�X�擾
	void FASTCALL AddIndex(int index, DWORD lba);
										// �C���f�b�N�X�ǉ�
	DWORD FASTCALL GetFirst() const;
										// �J�nLBA�擾
	DWORD FASTCALL GetLast() const;
										// �I�[LBA�擾
	DWORD FASTCALL GetBlocks() const;
										// �u���b�N���擾
	int FASTCALL GetTrackNo() const;
										// �g���b�N�ԍ��擾
	BOOL FASTCALL IsValid(DWORD lba) const;
										// �L����LBA��
	BOOL FASTCALL IsAudio() const;
										// �I�[�f�B�I�g���b�N��

private:
	SCSICD *cdrom;
										// �e�f�o�C�X
	BOOL valid;
										// �L���ȃg���b�N
	int track_no;
										// �g���b�N�ԍ�
	DWORD first_lba;
										// �J�nLBA
	DWORD last_lba;
										// �I��LBA
	BOOL audio;
										// �I�[�f�B�I�g���b�N�t���O
	BOOL raw;
										// RAW�f�[�^�t���O
	Filepath imgpath;
										// �C���[�W�t�@�C���p�X
};

//===========================================================================
//
//	CD-DA �o�b�t�@
//
//===========================================================================
class CDDABuf
{
public:
	// ��{�t�@���N�V����
	CDDABuf();
										// �R���X�g���N�^
	virtual ~CDDABuf();
										// �f�X�g���N�^
#if 0
	BOOL Init();
										// ������
	BOOL FASTCALL Load(const Filepath& path);
										// ���[�h
	BOOL FASTCALL Save(const Filepath& path);
										// �Z�[�u

	// API
	void FASTCALL Clear();
										// �o�b�t�@�N���A
	BOOL FASTCALL Open(Filepath& path);
										// �t�@�C���w��
	BOOL FASTCALL GetBuf(DWORD *buffer, int frames);
										// �o�b�t�@�擾
	BOOL FASTCALL IsValid();
										// �L���`�F�b�N
	BOOL FASTCALL ReadReq();
										// �ǂݍ��ݗv��
	BOOL FASTCALL IsEnd() const;
										// �I���`�F�b�N

private:
	Filepath wavepath;
										// Wave�p�X
	BOOL valid;
										// �I�[�v������
	DWORD *buf;
										// �f�[�^�o�b�t�@
	DWORD read;
										// Read�|�C���^
	DWORD write;
										// Write�|�C���^
	DWORD num;
										// �f�[�^�L����
	DWORD rest;
										// �t�@�C���c��T�C�Y
#endif
};

//===========================================================================
//
//	SCSI CD-ROM
//
//===========================================================================
class SCSICD : public Disk
{
public:
	// �g���b�N��
	enum {
		TrackMax = 96					// �g���b�N�ő吔
	};

public:
	// ��{�t�@���N�V����
	SCSICD(Device *dev);
										// �R���X�g���N�^
	virtual ~SCSICD();
										// �f�X�g���N�^
	BOOL FASTCALL Open(const Filepath& path, BOOL attn = TRUE);
										// �I�[�v��
	BOOL FASTCALL Load(Fileio *fio, int ver);
										// ���[�h

	// �R�}���h
	int FASTCALL Inquiry(const DWORD *cdb, BYTE *buf);
										// INQUIRY�R�}���h
	int FASTCALL Read(BYTE *buf, int block);
										// READ�R�}���h
	int FASTCALL ReadToc(const DWORD *cdb, BYTE *buf);
										// READ TOC�R�}���h
	BOOL FASTCALL PlayAudio(const DWORD *cdb);
										// PLAY AUDIO�R�}���h
	BOOL FASTCALL PlayAudioMSF(const DWORD *cdb);
										// PLAY AUDIO MSF�R�}���h
	BOOL FASTCALL PlayAudioTrack(const DWORD *cdb);
										// PLAY AUDIO TRACK�R�}���h

	// CD-DA
	BOOL FASTCALL NextFrame();
										// �t���[���ʒm
	void FASTCALL GetBuf(DWORD *buffer, int samples, DWORD rate);
										// CD-DA�o�b�t�@�擾

	// LBA-MSF�ϊ�
	void FASTCALL LBAtoMSF(DWORD lba, BYTE *msf) const;
										// LBA��MSF�ϊ�
	DWORD FASTCALL MSFtoLBA(const BYTE *msf) const;
										// MSF��LBA�ϊ�

private:
	// �I�[�v��
	BOOL FASTCALL OpenCue(const Filepath& path);
										// �I�[�v��(CUE)
	BOOL FASTCALL OpenIso(const Filepath& path);
										// �I�[�v��(ISO)
	BOOL rawfile;
										// RAW�t���O

	// �g���b�N�Ǘ�
	void FASTCALL ClearTrack();
										// �g���b�N�N���A
	int FASTCALL SearchTrack(DWORD lba) const;
										// �g���b�N����
	CDTrack* track[TrackMax];
										// �g���b�N�I�u�W�F�N�g
	int tracks;
										// �g���b�N�I�u�W�F�N�g�L����
	int dataindex;
										// ���݂̃f�[�^�g���b�N
	int audioindex;
										// ���݂̃I�[�f�B�I�g���b�N

	int frame;
										// �t���[���ԍ�

#if 0
	CDDABuf da_buf;
										// CD-DA�o�b�t�@
	int da_num;
										// CD-DA�g���b�N��
	int da_cur;
										// CD-DA�J�����g�g���b�N
	int da_next;
										// CD-DA�l�N�X�g�g���b�N
	BOOL da_req;
										// CD-DA�f�[�^�v��
#endif
};

#endif	// disk_h
