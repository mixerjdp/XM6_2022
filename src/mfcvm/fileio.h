//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2005 ＰＩ．(ytanaka@ipc-tokai.or.jp)
//	[ ファイルI/O ]
//
//---------------------------------------------------------------------------

#if !defined(fileio_h)
#define fileio_h

//===========================================================================
//
//	ファイルI/O
//
//===========================================================================
class Fileio
{
public:
	enum OpenMode {
		ReadOnly,						// 読み込みのみ
		WriteOnly,						// 書き込みのみ
		ReadWrite,						// 読み書き両方
		Append							// アペンド
	};

public:
	Fileio();
										// コンストラクタ
	virtual ~Fileio();
										// デストラクタ
	BOOL FASTCALL Load(const Filepath& path, void *buffer, int size);
										// ROM,RAMロード
	BOOL FASTCALL Save(const Filepath& path, void *buffer, int size);
										// RAMセーブ

#if defined(_WIN32)
	BOOL FASTCALL Open(LPCTSTR fname, OpenMode mode);
										// オープン
#endif	// _WIN32
	BOOL FASTCALL Open(const Filepath& path, OpenMode mode);
										// オープン
	BOOL FASTCALL Seek(long offset);
										// シーク
	BOOL FASTCALL Read(void *buffer, int size);
										// 読み込み
	BOOL FASTCALL Write(const void *buffer, int size);
										// 書き込み
	DWORD FASTCALL GetFileSize() const;
										// ファイルサイズ取得
	DWORD FASTCALL GetFilePos() const;
										// ファイル位置取得
	void FASTCALL Close();
										// クローズ
	BOOL FASTCALL IsValid() const		{ return (BOOL)(handle != -1); }
										// 有効チェック
	int FASTCALL GetHandle() const		{ return handle; }
										// ハンドル取得

private:
	int handle;							// ファイルハンドル
};

#endif	// fileio_h
