//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 ＰＩ．(ytanaka@ipc-tokai.or.jp)
//	[ PPI(i8255A) ]
//
//---------------------------------------------------------------------------

#if !defined(ppi_h)
#define ppi_h

#include "device.h"

//===========================================================================
//
//	PPI
//
//===========================================================================
class PPI : public MemDevice
{
public:
	// 定数定義
	enum {
		PortMax = 2,					// ポート最大数
		AxisMax = 4,					// 軸最大数
		ButtonMax = 8					// ボタン最大数
	};

	// ジョイスティックデータ定義
	typedef struct {
		DWORD axis[AxisMax];				// 軸情報
		BOOL button[ButtonMax];				// ボタン情報
	} joyinfo_t;

	// 内部データ定義
	typedef struct {
		DWORD portc;					// ポートC
		int type[PortMax];				// ジョイスティックタイプ
		DWORD ctl[PortMax];				// ジョイスティックコントロール
		joyinfo_t info[PortMax];		// ジョイスティック情報
	} ppi_t;

public:
	// 基本ファンクション
	PPI(VM *p);
										// コンストラクタ
	BOOL FASTCALL Init();
										// 初期化
	void FASTCALL Cleanup();
										// クリーンアップ
	void FASTCALL Reset();
										// リセット
	BOOL FASTCALL Save(Fileio *fio, int ver);
										// セーブ
	BOOL FASTCALL Load(Fileio *fio, int ver);
										// ロード
	void FASTCALL ApplyCfg(const Config *config);
										// 設定適用
#if defined(_DEBUG)
	void FASTCALL AssertDiag() const;
										// 診断
#endif	// _DEBUG

	// メモリデバイス
	DWORD FASTCALL ReadByte(DWORD addr);
										// バイト読み込み
	DWORD FASTCALL ReadWord(DWORD addr);
										// ワード読み込み
	void FASTCALL WriteByte(DWORD addr, DWORD data);
										// バイト書き込み
	void FASTCALL WriteWord(DWORD addr, DWORD data);
										// ワード書き込み
	DWORD FASTCALL ReadOnly(DWORD addr) const;
										// 読み込みのみ

	// 外部API
	void FASTCALL GetPPI(ppi_t *buffer);
										// 内部データ取得
	void FASTCALL SetJoyInfo(int port, const joyinfo_t *info);
										// ジョイスティック情報設定
	const joyinfo_t* FASTCALL GetJoyInfo(int port) const;
										// ジョイスティック情報取得
	JoyDevice* FASTCALL CreateJoy(int port, int type);
										// ジョイスティックデバイス作成

private:
	void FASTCALL SetPortC(DWORD data);
										// ポートCセット
	ADPCM *adpcm;
										// ADPCM
	JoyDevice *joy[PortMax];
										// ジョイスティック
	ppi_t ppi;
										// 内部データ
};

//===========================================================================
//
//	ジョイスティックデバイス
//
//===========================================================================
class JoyDevice
{
public:
	// 基本ファンクション
	JoyDevice(PPI *parent, int no);
										// コンストラクタ
	virtual ~JoyDevice();
										// デストラクタ
	DWORD FASTCALL GetID() const		{ return id; }
										// ID取得
	DWORD FASTCALL GetType() const		{ return type; }
										// タイプ取得
	virtual void FASTCALL Reset();
										// リセット
	virtual BOOL FASTCALL Save(Fileio *fio, int ver);
										// セーブ
	virtual BOOL FASTCALL Load(Fileio *fio, int ver);
										// ロード

	// アクセス
	virtual DWORD FASTCALL ReadPort(DWORD ctl);
										// ポート読み取り
	virtual DWORD FASTCALL ReadOnly(DWORD ctl) const;
										// ポート読み取り(Read Only)
	virtual void FASTCALL Control(DWORD ctl);
										// コントロール

	// キャッシュ
	void FASTCALL Notify()				{ changed = TRUE; }
										// 親ポート変更通知
	virtual void FASTCALL MakeData();
										// データ作成

	// プロパティ
	int FASTCALL GetAxes() const		{ return axes; }
										// 軸数取得
	const char* FASTCALL GetAxisDesc(int axis) const;
										// 軸表示取得
	int FASTCALL GetButtons() const		{ return buttons; }
										// ボタン数取得
	const char* FASTCALL GetButtonDesc(int button) const;
										// ボタン表示取得
	BOOL FASTCALL IsAnalog() const		{ return analog; }
										// アナログ・デジタル取得
	int FASTCALL GetDatas() const		{ return datas; }
										// データ数取得

protected:
	DWORD type;
										// タイプ
	DWORD id;
										// ID
	PPI *ppi;
										// PPI
	int port;
										// ポート番号
	int axes;
										// 軸数
	const char **axis_desc;
										// 軸表示
	int buttons;
										// ボタン数
	const char **button_desc;
										// ボタン表示
	BOOL analog;
										// 種別(アナログ・デジタル)
	DWORD *data;
										// データ実体
	int datas;
										// データ数
	BOOL changed;
										// ジョイスティック変更通知
};

//===========================================================================
//
//	ジョイスティック(ATARI標準)
//
//===========================================================================
class JoyAtari : public JoyDevice
{
public:
	JoyAtari(PPI *parent, int no);
										// コンストラクタ

protected:
	DWORD FASTCALL ReadOnly(DWORD ctl) const;
										// ポート読み取り(Read Only)
	void FASTCALL MakeData();
										// データ作成

private:
	static const char* AxisDescTable[];
										// 軸表示テーブル
	static const char* ButtonDescTable[];
										// ボタン表示テーブル
};

//===========================================================================
//
//	ジョイスティック(ATARI標準+START/SELECT)
//
//===========================================================================
class JoyASS : public JoyDevice
{
public:
	JoyASS(PPI *parent, int no);
										// コンストラクタ

protected:
	DWORD FASTCALL ReadOnly(DWORD ctl) const;
										// ポート読み取り(Read Only)
	void FASTCALL MakeData();
										// データ作成

private:
	static const char* AxisDescTable[];
										// 軸表示テーブル
	static const char* ButtonDescTable[];
										// ボタン表示テーブル
};

//===========================================================================
//
//	ジョイスティック(サイバースティック・アナログ)
//
//===========================================================================
class JoyCyberA : public JoyDevice
{
public:
	JoyCyberA(PPI *parent, int no);
										// コンストラクタ

protected:
	void FASTCALL Reset();
										// リセット
	DWORD FASTCALL ReadPort(DWORD ctl);
										// ポート読み取り
	DWORD FASTCALL ReadOnly(DWORD ctl) const;
										// ポート読み取り(Read Only)
	void FASTCALL Control(DWORD ctl);
										// コントロール
	void FASTCALL MakeData();
										// データ作成
	BOOL FASTCALL Save(Fileio *fio, int ver);
										// セーブ
	BOOL FASTCALL Load(Fileio *fio, int ver);
										// ロード

private:
	DWORD seq;
										// シーケンス
	DWORD ctrl;
										// 前回のコントロール(0 or 1)
	DWORD hus;
										// 判定用時間
	Scheduler *scheduler;
										// スケジューラ
	static const char* AxisDescTable[];
										// 軸表示テーブル
	static const char* ButtonDescTable[];
										// ボタン表示テーブル
};

//===========================================================================
//
//	ジョイスティック(サイバースティック・デジタル)
//
//===========================================================================
class JoyCyberD : public JoyDevice
{
public:
	JoyCyberD(PPI *parent, int no);
										// コンストラクタ

protected:
	DWORD FASTCALL ReadOnly(DWORD ctl) const;
										// ポート読み取り(Read Only)
	void FASTCALL MakeData();
										// データ作成

private:
	static const char* AxisDescTable[];
										// 軸表示テーブル
	static const char* ButtonDescTable[];
										// ボタン表示テーブル
};

//===========================================================================
//
//	ジョイスティック(MD3ボタン)
//
//===========================================================================
class JoyMd3 : public JoyDevice
{
public:
	JoyMd3(PPI *parent, int no);
										// コンストラクタ

protected:
	DWORD FASTCALL ReadOnly(DWORD ctl) const;
										// ポート読み取り(Read Only)
	void FASTCALL MakeData();
										// データ作成

private:
	static const char* AxisDescTable[];
										// 軸表示テーブル
	static const char* ButtonDescTable[];
										// ボタン表示テーブル
};

//===========================================================================
//
//	ジョイスティック(MD6ボタン)
//
//===========================================================================
class JoyMd6 : public JoyDevice
{
public:
	JoyMd6(PPI *parent, int no);
										// コンストラクタ

protected:
	void FASTCALL Reset();
										// リセット
	DWORD FASTCALL ReadOnly(DWORD ctl) const;
										// ポート読み取り(Read Only)
	void FASTCALL Control(DWORD ctl);
										// コントロール
	void FASTCALL MakeData();
										// データ作成
	BOOL FASTCALL Save(Fileio *fio, int ver);
										// セーブ
	BOOL FASTCALL Load(Fileio *fio, int ver);
										// ロード

private:
	DWORD seq;
										// シーケンス
	DWORD ctrl;
										// 前回のコントロール(0 or 1)
	DWORD hus;
										// 判定用時間
	Scheduler *scheduler;
										// スケジューラ
	static const char* AxisDescTable[];
										// 軸表示テーブル
	static const char* ButtonDescTable[];
										// ボタン表示テーブル
};

//===========================================================================
//
//	ジョイスティック(CPSF-SFC)
//
//===========================================================================
class JoyCpsf : public JoyDevice
{
public:
	JoyCpsf(PPI *parent, int no);
										// コンストラクタ

protected:
	DWORD FASTCALL ReadOnly(DWORD ctl) const;
										// ポート読み取り(Read Only)
	void FASTCALL MakeData();
										// データ作成

private:
	static const char* AxisDescTable[];
										// 軸表示テーブル
	static const char* ButtonDescTable[];
										// ボタン表示テーブル
};

//===========================================================================
//
//	ジョイスティック(CPSF-MD)
//
//===========================================================================
class JoyCpsfMd : public JoyDevice
{
public:
	JoyCpsfMd(PPI *parent, int no);
										// コンストラクタ

protected:
	DWORD FASTCALL ReadOnly(DWORD ctl) const;
										// ポート読み取り(Read Only)
	void FASTCALL MakeData();
										// データ作成

private:
	static const char* AxisDescTable[];
										// 軸表示テーブル
	static const char* ButtonDescTable[];
										// ボタン表示テーブル
};

//===========================================================================
//
//	ジョイスティック(マジカルパッド)
//
//===========================================================================
class JoyMagical : public JoyDevice
{
public:
	JoyMagical(PPI *parent, int no);
										// コンストラクタ

protected:
	DWORD FASTCALL ReadOnly(DWORD ctl) const;
										// ポート読み取り(Read Only)
	void FASTCALL MakeData();
										// データ作成

private:
	static const char* AxisDescTable[];
										// 軸表示テーブル
	static const char* ButtonDescTable[];
										// ボタン表示テーブル
};

//===========================================================================
//
//	ジョイスティック(XPD-1LR)
//
//===========================================================================
class JoyLR : public JoyDevice
{
public:
	JoyLR(PPI *parent, int no);
										// コンストラクタ

protected:
	DWORD FASTCALL ReadOnly(DWORD ctl) const;
										// ポート読み取り(Read Only)
	void FASTCALL MakeData();
										// データ作成

private:
	static const char* AxisDescTable[];
										// 軸表示テーブル
	static const char* ButtonDescTable[];
										// ボタン表示テーブル
};

//===========================================================================
//
//	ジョイスティック(パックランド専用パッド)
//
//===========================================================================
class JoyPacl : public JoyDevice
{
public:
	JoyPacl(PPI *parent, int no);
										// コンストラクタ

protected:
	DWORD FASTCALL ReadOnly(DWORD ctl) const;
										// ポート読み取り(Read Only)
	void FASTCALL MakeData();
										// データ作成

private:
	static const char* ButtonDescTable[];
										// ボタン表示テーブル
};

//===========================================================================
//
//	ジョイスティック(BM68専用コントローラ)
//
//===========================================================================
class JoyBM : public JoyDevice
{
public:
	JoyBM(PPI *parent, int no);
										// コンストラクタ

protected:
	DWORD FASTCALL ReadOnly(DWORD ctl) const;
										// ポート読み取り(Read Only)
	void FASTCALL MakeData();
										// データ作成

private:
	static const char* ButtonDescTable[];
										// ボタン表示テーブル
};

#endif	// ppi_h
