//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 ＰＩ．(ytanaka@ipc-tokai.or.jp)
//	[ PPI(i8255A) ]
//
//---------------------------------------------------------------------------

#include "os.h"
#include "xm6.h"
#include "vm.h"
#include "log.h"
#include "adpcm.h"
#include "schedule.h"
#include "config.h"
#include "fileio.h"
#include "ppi.h"

//===========================================================================
//
//	PPI
//
//===========================================================================
//#define PPI_LOG

//---------------------------------------------------------------------------
//
//	コンストラクタ
//
//---------------------------------------------------------------------------
PPI::PPI(VM *p) : MemDevice(p)
{
	int i;

	// デバイスIDを初期化
	dev.id = MAKEID('P', 'P', 'I', ' ');
	dev.desc = "PPI (i8255A)";

	// 開始アドレス、終了アドレス
	memdev.first = 0xe9a000;
	memdev.last = 0xe9bfff;

	// 内部ワーク
	memset(&ppi, 0, sizeof(ppi));

	// オブジェクト
	adpcm = NULL;
	for (i=0; i<PortMax; i++) {
		joy[i] = NULL;
	}
}

//---------------------------------------------------------------------------
//
//	初期化
//
//---------------------------------------------------------------------------
BOOL FASTCALL PPI::Init()
{
	int i;

	ASSERT(this);

	// 基本クラス
	if (!MemDevice::Init()) {
		return FALSE;
	}

	// ADPCM取得
	ASSERT(!adpcm);
	adpcm = (ADPCM*)vm->SearchDevice(MAKEID('A', 'P', 'C', 'M'));
	ASSERT(adpcm);

	// ジョイスティックタイプ
	for (i=0; i<PortMax; i++) {
		ppi.type[i] = 0;
		ASSERT(!joy[i]);
		joy[i] = new JoyDevice(this, i);
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	クリーンアップ
//
//---------------------------------------------------------------------------
void FASTCALL PPI::Cleanup()
{
	int i;

	ASSERT(this);

	// ジョイスティックデバイスを解放
	for (i=0; i<PortMax; i++) {
		ASSERT(joy[i]);
		delete joy[i];
		joy[i] = NULL;
	}

	// 基本クラスへ
	MemDevice::Cleanup();
}

//---------------------------------------------------------------------------
//
//	リセット
//
//---------------------------------------------------------------------------
void FASTCALL PPI::Reset()
{
	int i;

	ASSERT(this);
	ASSERT_DIAG();

	LOG0(Log::Normal, "リセット");

	// ポートC
	ppi.portc = 0;

	// コントロール
	for (i=0; i<PortMax; i++) {
		ppi.ctl[i] = 0;
	}

	// ジョイスティックデバイスに対して、リセットを通知
	for (i=0; i<PortMax; i++) {
		joy[i]->Reset();
	}
}

//---------------------------------------------------------------------------
//
//	セーブ
//
//---------------------------------------------------------------------------
BOOL FASTCALL PPI::Save(Fileio *fio, int ver)
{
	size_t sz;
	int i;
	DWORD type;

	ASSERT(this);
	ASSERT(fio);
	ASSERT(ver >= 0x0200);
	ASSERT_DIAG();

	LOG0(Log::Normal, "セーブ");

	// サイズをセーブ
	sz = sizeof(ppi_t);
	if (!fio->Write(&sz, sizeof(sz))) {
		return FALSE;
	}

	// 実体をセーブ
	if (!fio->Write(&ppi, (int)sz)) {
		return FALSE;
	}

	// デバイスをセーブ
	for (i=0; i<PortMax; i++) {
		// デバイスタイプ
		type = joy[i]->GetType();
		if (!fio->Write(&type, sizeof(type))) {
			return FALSE;
		}

		// デバイス実体
		if (!joy[i]->Save(fio, ver)) {
			return FALSE;
		}
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	ロード
//
//---------------------------------------------------------------------------
BOOL FASTCALL PPI::Load(Fileio *fio, int ver)
{
	size_t sz;
	int i;
	DWORD type;

	ASSERT(this);
	ASSERT(fio);
	ASSERT(ver >= 0x0200);
	ASSERT_DIAG();

	LOG0(Log::Normal, "ロード");

	// サイズをロード、照合
	if (!fio->Read(&sz, sizeof(sz))) {
		return FALSE;
	}
	if (sz != sizeof(ppi_t)) {
		return FALSE;
	}

	// 実体をロード
	if (!fio->Read(&ppi, (int)sz)) {
		return FALSE;
	}

	// version2.00であればここまで
	if (ver <= 0x200) {
		return TRUE;
	}

	// デバイスをロード
	for (i=0; i<PortMax; i++) {
		// デバイスタイプを得る
		if (!fio->Read(&type, sizeof(type))) {
			return FALSE;
		}

		// 現在のデバイスと一致していなければ、作り直す
		if (joy[i]->GetType() != type) {
			delete joy[i];
			joy[i] = NULL;

			// PPIに記憶しているタイプもあわせて変更
			ppi.type[i] = (int)type;

			// 再作成
			joy[i] = CreateJoy(i, ppi.type[i]);
			ASSERT(joy[i]->GetType() == type);
		}

		// デバイス固有
		if (!joy[i]->Load(fio, ver)) {
			return FALSE;
		}
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	設定適用
//
//---------------------------------------------------------------------------
void FASTCALL PPI::ApplyCfg(const Config *config)
{
	int i;

	ASSERT(this);
	ASSERT(config);
	ASSERT_DIAG();

	LOG0(Log::Normal, "設定適用");

	// ポートループ
	for (i=0; i<PortMax; i++) {
		// タイプ一致なら次へ
		if (config->joy_type[i] == ppi.type[i]) {
			continue;
		}

		// 現在のデバイスを削除
		ASSERT(joy[i]);
		delete joy[i];
		joy[i] = NULL;

		// タイプ記憶、ジョイスティック作成
		ppi.type[i] = config->joy_type[i];
		joy[i] = CreateJoy(i, config->joy_type[i]);
	}
}

#if defined(_DEBUG)
//---------------------------------------------------------------------------
//
//	診断
//
//---------------------------------------------------------------------------
void FASTCALL PPI::AssertDiag() const
{
	ASSERT(this);
	ASSERT(GetID() == MAKEID('P', 'P', 'I', ' '));
	ASSERT(adpcm);
	ASSERT(adpcm->GetID() == MAKEID('A', 'P', 'C', 'M'));
	ASSERT(joy[0]);
	ASSERT(joy[1]);
}
#endif	// _DEBUG

//---------------------------------------------------------------------------
//
//	バイト読み込み
//
//---------------------------------------------------------------------------
DWORD FASTCALL PPI::ReadByte(DWORD addr)
{
	DWORD data;

	ASSERT(this);
	ASSERT((addr >= memdev.first) && (addr <= memdev.last));
	ASSERT(PortMax >= 2);
	ASSERT_DIAG();

	// 奇数アドレスのみデコードされている
	if ((addr & 1) == 0) {
		return 0xff;
	}

	// 8バイト単位でループ
	addr &= 7;

	// ウェイト
	scheduler->Wait(1);

	// デコード
	addr >>= 1;
	switch (addr) {
		// ポートA
		case 0:
#if defined(PPI_LOG)
			data = joy[0]->ReadPort(ppi.ctl[0]);
			LOG2(Log::Normal, "ポート1読み出し コントロール$%02X データ$%02X",
								ppi.ctl[0], data);
#else
			data = joy[0]->ReadPort(ppi.ctl[0]);
#endif	// PPI_LOG

			// PC7,PC6を考慮
			if (ppi.ctl[0] & 0x80) {
				data &= ~0x40;
			}
			if (ppi.ctl[0] & 0x40) {
				data &= ~0x20;
			}
			return data;

		// ポートB
		case 1:
#if defined(PPI_LOG)
			data = joy[1]->ReadPort(ppi.ctl[1]);
			LOG2(Log::Normal, "ポート2読み出し コントロール$%02X データ$%02X",
								ppi.ctl[1], data);
			return data;
#else
			return joy[1]->ReadPort(ppi.ctl[1]);
#endif	// PPI_LOG

		// ポートC
		case 2:
			return ppi.portc;
	}

	LOG1(Log::Warning, "未実装レジスタ読み込み R%02d", addr);
	return 0xff;
}

//---------------------------------------------------------------------------
//
//	ワード読み込み
//
//---------------------------------------------------------------------------
DWORD FASTCALL PPI::ReadWord(DWORD addr)
{
	ASSERT(this);
	ASSERT((addr >= memdev.first) && (addr <= memdev.last));
	ASSERT((addr & 1) == 0);
	ASSERT_DIAG();

	return (0xff00 | ReadByte(addr + 1));
}

//---------------------------------------------------------------------------
//
//	バイト書き込み
//
//---------------------------------------------------------------------------
void FASTCALL PPI::WriteByte(DWORD addr, DWORD data)
{
	DWORD bit;
	int i;

	ASSERT(this);
	ASSERT((addr >= memdev.first) && (addr <= memdev.last));
	ASSERT(data < 0x100);
	ASSERT_DIAG();

	// 奇数アドレスのみデコードされている
	if ((addr & 1) == 0) {
		return;
	}

	// 8バイト単位でループ
	addr &= 7;

	// ウェイト
	scheduler->Wait(1);

	// ポートCへのWrite
	if (addr == 5) {
		// ジョイスティック・ADPCMコントロール
		SetPortC(data);
		return;
	}

	// モードコントロール
	if (addr == 7) {
		if (data < 0x80) {
			// ビットセット・リセットモード
			i = ((data >> 1) & 0x07);
			bit = (DWORD)(1 << i);
			if (data & 1) {
				SetPortC(DWORD(ppi.portc | bit));
			}
			else {
				SetPortC(DWORD(ppi.portc & ~bit));
			}
			return;
		}

		// モードコントロール
		if (data != 0x92) {
			LOG0(Log::Warning, "サポート外モード指定 $%02X");
		}
		return;
	}

	LOG2(Log::Warning, "未実装レジスタ書き込み R%02d <- $%02X",
							addr, data);
}

//---------------------------------------------------------------------------
//
//	ワード書き込み
//
//---------------------------------------------------------------------------
void FASTCALL PPI::WriteWord(DWORD addr, DWORD data)
{
	ASSERT(this);
	ASSERT((addr >= memdev.first) && (addr <= memdev.last));
	ASSERT((addr & 1) == 0);
	ASSERT(data < 0x10000);
	ASSERT_DIAG();

	WriteByte(addr + 1, (BYTE)data);
}

//---------------------------------------------------------------------------
//
//	読み込みのみ
//
//---------------------------------------------------------------------------
DWORD FASTCALL PPI::ReadOnly(DWORD addr) const
{
	DWORD data;

	ASSERT(this);
	ASSERT((addr >= memdev.first) && (addr <= memdev.last));
	ASSERT(PortMax >= 2);
	ASSERT_DIAG();

	// 奇数アドレスのみデコードされている
	if ((addr & 1) == 0) {
		return 0xff;
	}

	// 8バイト単位でループ
	addr &= 7;

	// デコード
	addr >>= 1;
	switch (addr) {
		// ポートA
		case 0:
			// データ取得
			data = joy[0]->ReadOnly(ppi.ctl[0]);

			// PC7,PC6を考慮
			if (ppi.ctl[0] & 0x80) {
				data &= ~0x40;
			}
			if (ppi.ctl[0] & 0x40) {
				data &= ~0x20;
			}
			return data;

		// ポートB
		case 1:
			return joy[1]->ReadOnly(ppi.ctl[1]);

		// ポートC
		case 2:
			return ppi.portc;
	}

	return 0xff;
}

//---------------------------------------------------------------------------
//
//	ポートCセット
//
//---------------------------------------------------------------------------
void FASTCALL PPI::SetPortC(DWORD data)
{
	ASSERT(this);
	ASSERT(data < 0x100);
	ASSERT(PortMax >= 2);
	ASSERT_DIAG();

	// データを記憶
	ppi.portc = data;

	// コントロールデータ組み立て(ポートA)...bit0がPC4、bit6,7がPC6,7を示す
	ppi.ctl[0] = ppi.portc & 0xc0;
	if (ppi.portc & 0x10) {
		ppi.ctl[0] |= 0x01;
	}
#if defined(PPI_LOG)
	LOG1(Log::Normal, "ポート1 コントロール $%02X", ppi.ctl[0]);
#endif	// PPI_LOG
	joy[0]->Control(ppi.ctl[0]);

	// コントロールデータ組み立て(ポートB)...bit0がPC5を示す
	if (ppi.portc & 0x20) {
		ppi.ctl[1] = 0x01;
	}
	else {
		ppi.ctl[1] = 0x00;
	}
#if defined(PPI_LOG)
	LOG1(Log::Normal, "ポート2 コントロール $%02X", ppi.ctl[1]);
#endif	// PPI_LOG
	joy[1]->Control(ppi.ctl[1]);

	// ADPCMパンポット
	adpcm->SetPanpot(data & 3);

	// ADPCM速度比率
	adpcm->SetRatio((data >> 2) & 3);
}

//---------------------------------------------------------------------------
//
//	内部データ取得
//
//---------------------------------------------------------------------------
void FASTCALL PPI::GetPPI(ppi_t *buffer)
{
	ASSERT(this);
	ASSERT(buffer);
	ASSERT_DIAG();

	// 内部ワークをコピー
	*buffer = ppi;
}

//---------------------------------------------------------------------------
//
//	ジョイスティック情報設定
//
//---------------------------------------------------------------------------
void FASTCALL PPI::SetJoyInfo(int port, const joyinfo_t *info)
{
	ASSERT(this);
	ASSERT((port >= 0) && (port < PortMax));
	ASSERT(info);
	ASSERT(PortMax >= 2);
	ASSERT_DIAG();

	// 比較して、一致していれば何もしない
	if (memcmp(&ppi.info[port], info, sizeof(joyinfo_t)) == 0) {
		return;
	}

	// 保存
	memcpy(&ppi.info[port], info, sizeof(joyinfo_t));

	// そのポートに対応するジョイスティックデバイスへ、通知
	ASSERT(joy[port]);
	joy[port]->Notify();
}

//---------------------------------------------------------------------------
//
//	ジョイスティック情報取得
//
//---------------------------------------------------------------------------
const PPI::joyinfo_t* FASTCALL PPI::GetJoyInfo(int port) const
{
	ASSERT(this);
	ASSERT((port >= 0) && (port < PortMax));
	ASSERT(PortMax >= 2);
	ASSERT_DIAG();

	return &(ppi.info[port]);
}

//---------------------------------------------------------------------------
//
//	ジョイスティックデバイス作成
//
//---------------------------------------------------------------------------
JoyDevice* FASTCALL PPI::CreateJoy(int port, int type)
{
	ASSERT(this);
	ASSERT(type >= 0);
	ASSERT((port >= 0) && (port < PortMax));
	ASSERT(PortMax >= 2);

	// タイプ別
	switch (type) {
		// 接続なし
		case 0:
			return new JoyDevice(this, port);

		// ATARI標準
		case 1:
			return new JoyAtari(this, port);

		// ATARI標準+START/SELCT
		case 2:
			return new JoyASS(this, port);

		// サイバースティック(アナログ)
		case 3:
			return new JoyCyberA(this, port);

		// サイバースティック(デジタル)
		case 4:
			return new JoyCyberD(this, port);

		// MD3ボタン
		case 5:
			return new JoyMd3(this, port);

		// MD6ボタン
		case 6:
			return new JoyMd6(this, port);

		// CPSF-SFC
		case 7:
			return new JoyCpsf(this, port);

		// CPSF-MD
		case 8:
			return new JoyCpsfMd(this, port);

		// マジカルパッド
		case 9:
			return new JoyMagical(this, port);

		// XPD-1LR
		case 10:
			return new JoyLR(this, port);

		// パックランド専用パッド
		case 11:
			return new JoyPacl(this, port);

		// BM68専用コントローラ
		case 12:
			return new JoyBM(this, port);

		// その他
		default:
			ASSERT(FALSE);
			break;
	}

	// 通常、ここにはこない
	return new JoyDevice(this, port);
}

//===========================================================================
//
//	ジョイスティックデバイス
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	コンストラクタ
//
//---------------------------------------------------------------------------
JoyDevice::JoyDevice(PPI *parent, int no)
{
	ASSERT((no >= 0) || (no < PPI::PortMax));

	// タイプNULL
	id = MAKEID('N', 'U', 'L', 'L');
	type = 0;

	// 親デバイス(PPI)を記憶、ポート番号設定
	ppi = parent;
	port = no;

	// 軸・ボタンなし、デジタル、データ数0
	axes = 0;
	buttons = 0;
	analog = FALSE;
	datas = 0;

	// 表示
	axis_desc = NULL;
	button_desc = NULL;

	// データバッファはNULL
	data = NULL;

	// 更新チェック要
	changed = TRUE;
}

//---------------------------------------------------------------------------
//
//	デストラクタ
//
//---------------------------------------------------------------------------
JoyDevice::~JoyDevice()
{
	// データバッファがあれば解放
	if (data) {
		delete[] data;
		data = NULL;
	}
}

//---------------------------------------------------------------------------
//
//	リセット
//
//---------------------------------------------------------------------------
void FASTCALL JoyDevice::Reset()
{
	ASSERT(this);
}

//---------------------------------------------------------------------------
//
//	セーブ
//
//---------------------------------------------------------------------------
BOOL FASTCALL JoyDevice::Save(Fileio *fio, int /*ver*/)
{
	ASSERT(this);
	ASSERT(fio);

	// データ数が0ならセーブしない
	if (datas <= 0) {
		ASSERT(datas == 0);
		return TRUE;
	}

	// データ数だけ保存
	if (!fio->Write(data, sizeof(DWORD) * datas)) {
		return FALSE;
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	ロード
//
//---------------------------------------------------------------------------
BOOL FASTCALL JoyDevice::Load(Fileio *fio, int /*ver*/)
{
	ASSERT(this);
	ASSERT(fio);

	// データ数が0ならロードしない
	if (datas <= 0) {
		ASSERT(datas == 0);
		return TRUE;
	}

	// 更新あり
	changed = TRUE;

	// データ実体をロード
	if (!fio->Read(data, sizeof(DWORD) * datas)) {
		return FALSE;
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	ポート読み取り
//
//---------------------------------------------------------------------------
DWORD FASTCALL JoyDevice::ReadPort(DWORD ctl)
{
	ASSERT(this);
	ASSERT((port >= 0) && (port < PPI::PortMax));
	ASSERT(ppi);
	ASSERT(ctl < 0x100);

	// 変更フラグをチェック
	if (changed) {
		// フラグ落とす
		changed = FALSE;

		// データ作成
		MakeData();
	}

	// ReadOnlyと同じデータを返す
	return ReadOnly(ctl);
}

//---------------------------------------------------------------------------
//
//	ポート読み取り(Read Only)
//
//---------------------------------------------------------------------------
DWORD FASTCALL JoyDevice::ReadOnly(DWORD /*ctl*/) const
{
	ASSERT(this);

	// 未接続
	return 0xff;
}

//---------------------------------------------------------------------------
//
//	コントロール
//
//---------------------------------------------------------------------------
void FASTCALL JoyDevice::Control(DWORD /*ctl*/)
{
	ASSERT(this);
}

//---------------------------------------------------------------------------
//
//	データ作成
//
//---------------------------------------------------------------------------
void FASTCALL JoyDevice::MakeData()
{
	ASSERT(this);
}

//---------------------------------------------------------------------------
//
//	軸表示
//
//---------------------------------------------------------------------------
const char* FASTCALL JoyDevice::GetAxisDesc(int axis) const
{
	ASSERT(this);
	ASSERT(axis >= 0);

	// 軸数を超えていればNULL
	if (axis >= axes) {
		return NULL;
	}

	// 軸表示テーブルがなければNULL
	if (!axis_desc) {
		return NULL;
	}

	// 軸表示テーブルから返す
	return axis_desc[axis];
}

//---------------------------------------------------------------------------
//
//	ボタン表示
//
//---------------------------------------------------------------------------
const char* FASTCALL JoyDevice::GetButtonDesc(int button) const
{
	ASSERT(this);
	ASSERT(button >= 0);

	// ボタン数を超えていればNULL
	if (button >= buttons) {
		return NULL;
	}

	// ボタン表示テーブルがなければNULL
	if (!button_desc) {
		return NULL;
	}

	// ボタン表示テーブルから返す
	return button_desc[button];
}

//===========================================================================
//
//	ジョイスティック(ATARI標準)
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	コンストラクタ
//
//---------------------------------------------------------------------------
JoyAtari::JoyAtari(PPI *parent, int no) : JoyDevice(parent, no)
{
	// タイプATAR
	id = MAKEID('A', 'T', 'A', 'R');
	type = 1;

	// 2軸2ボタン、データ数1
	axes = 2;
	buttons = 2;
	datas = 1;

	// 表示テーブル
	axis_desc = AxisDescTable;
	button_desc = ButtonDescTable;

	// データバッファ確保
	data = new DWORD[datas];

	// 初期データ設定
	data[0] = 0xff;
}

//---------------------------------------------------------------------------
//
//	ポート読み取り(Read Only)
//
//---------------------------------------------------------------------------
DWORD FASTCALL JoyAtari::ReadOnly(DWORD ctl) const
{
	ASSERT(this);
	ASSERT(ctl < 0x100);

	// PC4が1なら、0xff
	if (ctl & 1) {
		return 0xff;
	}

	// 作成済みのデータを返す
	return data[0];
}

//---------------------------------------------------------------------------
//
//	データ作成
//
//---------------------------------------------------------------------------
void FASTCALL JoyAtari::MakeData()
{
	const PPI::joyinfo_t *info;
	DWORD axis;

	ASSERT(this);
	ASSERT(ppi);

	// データ初期化
	info = ppi->GetJoyInfo(port);
	data[0] = 0xff;

	// Up
	axis = info->axis[1];
	if ((axis >= 0xfffff800) && (axis < 0xfffffc00)) {
		data[0] &= ~0x01;
	}
	// Down
	if ((axis >= 0x00000400) && (axis < 0x00000800)) {
		data[0] &= ~0x02;
	}

	// Left
	axis = info->axis[0];
	if ((axis >= 0xfffff800) && (axis < 0xfffffc00)) {
		data[0] &= ~0x04;
	}
	// Right
	if ((axis >= 0x00000400) && (axis < 0x00000800)) {
		data[0] &= ~0x08;
	}

	// ボタンA
	if (info->button[0]) {
		data[0] &= ~0x40;
	}

	// ボタンB
	if (info->button[1]) {
		data[0] &= ~0x20;
	}
}

//---------------------------------------------------------------------------
//
//	軸表示テーブル
//
//---------------------------------------------------------------------------
const char* JoyAtari::AxisDescTable[] = {
	"X",
	"Y"
};

//---------------------------------------------------------------------------
//
//	ボタン表示テーブル
//
//---------------------------------------------------------------------------
const char* JoyAtari::ButtonDescTable[] = {
	"A",
	"B"
};

//===========================================================================
//
//	ジョイスティック(ATARI標準+START/SELECT)
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	コンストラクタ
//
//---------------------------------------------------------------------------
JoyASS::JoyASS(PPI *parent, int no) : JoyDevice(parent, no)
{
	// タイプATSS
	id = MAKEID('A', 'T', 'S', 'S');
	type = 2;

	// 2軸4ボタン、データ数1
	axes = 2;
	buttons = 4;
	datas = 1;

	// 表示テーブル
	axis_desc = AxisDescTable;
	button_desc = ButtonDescTable;

	// データバッファ確保
	data = new DWORD[datas];

	// 初期データ設定
	data[0] = 0xff;
}

//---------------------------------------------------------------------------
//
//	ポート読み取り(Read Only)
//
//---------------------------------------------------------------------------
DWORD FASTCALL JoyASS::ReadOnly(DWORD ctl) const
{
	ASSERT(this);
	ASSERT(ctl < 0x100);

	// PC4が1なら、0xff
	if (ctl & 1) {
		return 0xff;
	}

	// 作成済みのデータを返す
	return data[0];
}

//---------------------------------------------------------------------------
//
//	データ作成
//
//---------------------------------------------------------------------------
void FASTCALL JoyASS::MakeData()
{
	const PPI::joyinfo_t *info;
	DWORD axis;

	ASSERT(this);
	ASSERT(ppi);

	// データ初期化
	info = ppi->GetJoyInfo(port);
	data[0] = 0xff;

	// Up
	axis = info->axis[1];
	if ((axis >= 0xfffff800) && (axis < 0xfffffc00)) {
		data[0] &= ~0x01;
	}
	// Down
	if ((axis >= 0x00000400) && (axis < 0x00000800)) {
		data[0] &= ~0x02;
	}

	// Left
	axis = info->axis[0];
	if ((axis >= 0xfffff800) && (axis < 0xfffffc00)) {
		data[0] &= ~0x04;
	}
	// Right
	if ((axis >= 0x00000400) && (axis < 0x00000800)) {
		data[0] &= ~0x08;
	}

	// ボタンA
	if (info->button[0]) {
		data[0] &= ~0x40;
	}

	// ボタンB
	if (info->button[1]) {
		data[0] &= ~0x20;
	}

	// START(左右同時押しとして表現)
	if (info->button[2]) {
		data[0] &= ~0x0c;
	}

	// SELECT(上下同時押しとして表現)
	if (info->button[3]) {
		data[0] &= ~0x03;
	}
}

//---------------------------------------------------------------------------
//
//	軸表示テーブル
//
//---------------------------------------------------------------------------
const char* JoyASS::AxisDescTable[] = {
	"X",
	"Y"
};

//---------------------------------------------------------------------------
//
//	ボタン表示テーブル
//
//---------------------------------------------------------------------------
const char* JoyASS::ButtonDescTable[] = {
	"A",
	"B",
	"START",
	"SELECT"
};

//===========================================================================
//
//	ジョイスティック(サイバースティック・アナログ)
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	コンストラクタ
//
//---------------------------------------------------------------------------
JoyCyberA::JoyCyberA(PPI *parent, int no) : JoyDevice(parent, no)
{
	int i;

	// タイプCYBA
	id = MAKEID('C', 'Y', 'B', 'A');
	type = 3;

	// 3軸8ボタン、アナログ、データ数11
	axes = 3;
	buttons = 8;
	analog = TRUE;
	datas = 12;

	// 表示テーブル
	axis_desc = AxisDescTable;
	button_desc = ButtonDescTable;

	// データバッファ確保
	data = new DWORD[datas];

	// 初期データ設定
	for (i=0; i<12; i++) {
		// ACK,L/H,ボタン
		if (i & 1) {
			data[i] = 0xbf;
		}
		else {
			data[i] = 0x9f;
		}

		// センタ値は0x7fとする
		if ((i >= 2) && (i <= 5)) {
			// アナログデータHを7にする
			data[i] &= 0xf7;
		}
	}

	// スケジューラ取得
	scheduler = (Scheduler*)ppi->GetVM()->SearchDevice(MAKEID('S', 'C', 'H', 'E'));
	ASSERT(scheduler);

	// 自動リセット(コントローラを差し替えた場合に備える)
	Reset();
}

//---------------------------------------------------------------------------
//
//	リセット
//
//---------------------------------------------------------------------------
void FASTCALL JoyCyberA::Reset()
{
	ASSERT(this);
	ASSERT(scheduler);

	// 基本クラス
	JoyDevice::Reset();

	// シーケンス初期化
	seq = 0;

	// コントロール0
	ctrl = 0;

	// 時間記憶
	hus = scheduler->GetTotalTime();
}

//---------------------------------------------------------------------------
//
//	セーブ
//
//---------------------------------------------------------------------------
BOOL FASTCALL JoyCyberA::Save(Fileio *fio, int ver)
{
	ASSERT(this);
	ASSERT(fio);

	// 基本クラス
	if (!JoyDevice::Save(fio, ver)) {
		return FALSE;
	}

	// シーケンスをセーブ
	if (!fio->Write(&seq, sizeof(seq))) {
		return FALSE;
	}

	// コントロールをセーブ
	if (!fio->Write(&ctrl, sizeof(ctrl))) {
		return FALSE;
	}

	// 時間をセーブ
	if (!fio->Write(&hus, sizeof(hus))) {
		return FALSE;
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	ロード
//
//---------------------------------------------------------------------------
BOOL FASTCALL JoyCyberA::Load(Fileio *fio, int ver)
{
	ASSERT(this);
	ASSERT(fio);

	// 基本クラス
	if (!JoyDevice::Load(fio, ver)) {
		return FALSE;
	}

	// シーケンスをロード
	if (!fio->Read(&seq, sizeof(seq))) {
		return FALSE;
	}

	// コントロールをロード
	if (!fio->Read(&ctrl, sizeof(ctrl))) {
		return FALSE;
	}

	// 時間をロード
	if (!fio->Read(&hus, sizeof(hus))) {
		return FALSE;
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	応答速度設定(実機との比較に基づく)
//	※正確にはPC4を最初に立ててから、PA4のb5→b6が落ちるまでに100us以上
//	かかっている雰囲気があるが、そこまではエミュレーションしない
//
//---------------------------------------------------------------------------
#define JOYCYBERA_CYCLE		108

//---------------------------------------------------------------------------
//
//	ポート読み取り
//
//---------------------------------------------------------------------------
DWORD FASTCALL JoyCyberA::ReadPort(DWORD ctl)
{
	DWORD diff;
	DWORD n;

	// シーケンス0は無効
	if (seq == 0) {
		return 0xff;
	}

	// シーケンス12以上も無効
	if (seq >= 13) {
		// シーケンス0
		seq = 0;
		return 0xff;
	}

	// 変更フラグをチェック
	if (changed) {
		// フラグ落とす
		changed = FALSE;

		// データ作成
		MakeData();
	}

	ASSERT((seq >= 1) && (seq <= 12));

	// 差分取得
	diff = scheduler->GetTotalTime();
	diff -= hus;

	// 差分から計算
	if (diff >= JOYCYBERA_CYCLE) {
		n = diff / JOYCYBERA_CYCLE;
		diff %= JOYCYBERA_CYCLE;

		// シーケンスをリセット
		if ((seq & 1) == 0) {
			seq--;
		}
		// 2単位で進める
		seq += (2 * n);

		// 時間を補正
		hus += (JOYCYBERA_CYCLE * n);

		// +1
		if (diff >= (JOYCYBERA_CYCLE / 2)) {
			diff -= (JOYCYBERA_CYCLE / 2);
			seq++;
		}

		// シーケンスオーバー対策
		if (seq >= 13) {
			seq = 0;
			return 0xff;
		}
	}
	if (diff >= (JOYCYBERA_CYCLE / 2)) {
		// 後半に設定
		if (seq & 1) {
			seq++;
		}
	}

	// データ取得
	return ReadOnly(ctl);
}

//---------------------------------------------------------------------------
//
//	ポート読み取り(Read Only)
//
//---------------------------------------------------------------------------
DWORD FASTCALL JoyCyberA::ReadOnly(DWORD /*ctl*/) const
{
	ASSERT(this);

	// シーケンス0は無効
	if (seq == 0) {
		return 0xff;
	}

	// シーケンス12以上も無効
	if (seq >= 13) {
		return 0xff;
	}

	// シーケンスに従ったデータを返す
	ASSERT((seq >= 1) && (seq <= 12));
	return data[seq - 1];
}

//---------------------------------------------------------------------------
//
//	コントロール
//
//---------------------------------------------------------------------------
void FASTCALL JoyCyberA::Control(DWORD ctl)
{
	ASSERT(this);
	ASSERT(ctl < 0x100);

	// シーケンス0(無効)時とシーケンス11以降
	if ((seq == 0) || (seq >= 11)) {
		// 1→0で、シーケンス開始
		if (ctl) {
			// 今回1にした
			ctrl = 1;
		}
		else {
			// 今回0にした
			if (ctrl) {
				// 1→0への立ち下げ
				seq = 1;
				hus = scheduler->GetTotalTime();
			}
			ctrl = 0;
		}
		return;
	}

	// シーケンス1以降では、ACKのみ有効
	ctrl = ctl;
	if (ctl) {
		return;
	}

	// シーケンスを2単位で進める効果をもつ
	if ((seq & 1) == 0) {
		seq--;
	}
	seq += 2;

	// 時間を記憶
	hus = scheduler->GetTotalTime();
}

//---------------------------------------------------------------------------
//
//	データ作成
//
//---------------------------------------------------------------------------
void FASTCALL JoyCyberA::MakeData()
{
	const PPI::joyinfo_t *info;
	DWORD axis;

	ASSERT(this);
	ASSERT(ppi);

	// ジョイスティック情報取得
	info = ppi->GetJoyInfo(port);

	// data[0]:ボタンA,ボタンB,ボタンC,ボタンD
	data[0] |= 0x0f;
	if (info->button[0]) {
		data[0] &= ~0x08;
	}
	if (info->button[1]) {
		data[0] &= ~0x04;
	}
	if (info->button[2]) {
		data[0] &= ~0x02;
	}
	if (info->button[3]) {
		data[0] &= ~0x01;
	}

	// data[1]:ボタンE1,ボタンE2,ボタンF,ボタンG
	data[1] |= 0x0f;
	if (info->button[4]) {
		data[1] &= ~0x08;
	}
	if (info->button[5]) {
		data[1] &= ~0x04;
	}
	if (info->button[6]) {
		data[1] &= ~0x02;
	}
	if (info->button[7]) {
		data[1] &= ~0x01;
	}

	// data[2]:1H
	axis = info->axis[1];
	axis = (axis + 0x800) >> 4;
	data[2] &= 0xf0;
	data[2] |= (axis >> 4);

	// data[3]:2H
	axis = info->axis[0];
	axis = (axis + 0x800) >> 4;
	data[3] &= 0xf0;
	data[3] |= (axis >> 4);

	// data[4]:3H
	axis = info->axis[3];
	axis = (axis + 0x800) >> 4;
	data[4] &= 0xf0;
	data[4] |= (axis >> 4);

	// data[5]:4H(予約となっている。実機では0)
	data[5] &= 0xf0;

	// data[6]:1L
	axis = info->axis[1];
	axis = (axis + 0x800) >> 4;
	data[6] &= 0xf0;
	data[6] |= (axis & 0x0f);

	// data[7]:2L
	axis = info->axis[0];
	axis = (axis + 0x800) >> 4;
	data[7] &= 0xf0;
	data[7] |= (axis & 0x0f);

	// data[8]:3L
	axis = info->axis[3];
	axis = (axis + 0x800) >> 4;
	data[8] &= 0xf0;
	data[8] |= (axis & 0x0f);

	// data[9]:4L(予約となっている。実機では0)
	data[9] &= 0xf0;

	// data[10]:A,B,A',B'
	// A,Bはレバー一体のミニボタン、A'B'は通常の押しボタン
	// レバー一体のミニボタンとして扱う(アフターバーナーII)
	data[10] &= 0xf0;
	data[10] |= 0x0f;
	if (info->button[0]) {
		data[10] &= ~0x08;
	}
	if (info->button[1]) {
		data[10] &= ~0x04;
	}
}

//---------------------------------------------------------------------------
//
//	軸表示テーブル
//
//---------------------------------------------------------------------------
const char* JoyCyberA::AxisDescTable[] = {
	"Stick X",
	"Stick Y",
	"Throttle"
};

//---------------------------------------------------------------------------
//
//	ボタン表示テーブル
//
//---------------------------------------------------------------------------
const char* JoyCyberA::ButtonDescTable[] = {
	"A",
	"B",
	"C",
	"D",
	"E1",
	"E2",
	"START",
	"SELECT"
};

//===========================================================================
//
//	ジョイスティック(サイバースティック・デジタル)
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	コンストラクタ
//
//---------------------------------------------------------------------------
JoyCyberD::JoyCyberD(PPI *parent, int no) : JoyDevice(parent, no)
{
	// タイプCYBD
	id = MAKEID('C', 'Y', 'B', 'D');
	type = 4;

	// 3軸6ボタン、データ数2
	axes = 3;
	buttons = 6;
	datas = 2;

	// 表示テーブル
	axis_desc = AxisDescTable;
	button_desc = ButtonDescTable;

	// データバッファ確保
	data = new DWORD[datas];

	// 初期データ設定
	data[0] = 0xff;
	data[1] = 0xff;
}

//---------------------------------------------------------------------------
//
//	ポート読み取り(Read Only)
//
//---------------------------------------------------------------------------
DWORD FASTCALL JoyCyberD::ReadOnly(DWORD ctl) const
{
	ASSERT(this);
	ASSERT(ctl < 0x100);
	ASSERT(data[0] < 0x100);
	ASSERT(data[1] < 0x100);

	// PC4によって分ける
	if (ctl & 1) {
		return data[1];
	}
	else {
		return data[0];
	}
}

//---------------------------------------------------------------------------
//
//	データ作成
//
//---------------------------------------------------------------------------
void FASTCALL JoyCyberD::MakeData()
{
	const PPI::joyinfo_t *info;
	DWORD axis;

	ASSERT(this);
	ASSERT(ppi);

	// データ初期化
	info = ppi->GetJoyInfo(port);
	data[0] = 0xff;
	data[1] = 0xff;

	// Up
	axis = info->axis[1];
	if ((axis >= 0xfffff800) && (axis < 0xfffffc00)) {
		data[0] &= ~0x01;
	}
	// Down
	if ((axis >= 0x00000400) && (axis < 0x00000800)) {
		data[0] &= ~0x02;
	}

	// Left
	axis = info->axis[0];
	if ((axis >= 0xfffff800) && (axis < 0xfffffc00)) {
		data[0] &= ~0x04;
	}
	// Right
	if ((axis >= 0x00000400) && (axis < 0x00000800)) {
		data[0] &= ~0x08;
	}

	// ボタンA
	if (info->button[0]) {
		data[0] &= ~0x20;
	}

	// ボタンB
	if (info->button[1]) {
		data[0] &= ~0x40;
	}

	// スロットルUp
	axis = info->axis[2];
	if ((axis >= 0xfffff800) && (axis < 0xfffffc00)) {
		data[1] &= ~0x01;
	}
	// スロットルDown
	if ((axis >= 0x00000400) && (axis < 0x00000800)) {
		data[1] &= ~0x02;
	}

	// ボタンC
	if (info->button[2]) {
		data[1] &= ~0x04;
	}

	// ボタンD
	if (info->button[3]) {
		data[1] &= ~0x08;
	}

	// ボタンE1
	if (info->button[4]) {
		data[1] &= ~0x20;
	}

	// ボタンE2
	if (info->button[5]) {
		data[1] &= ~0x40;
	}
}

//---------------------------------------------------------------------------
//
//	軸表示テーブル
//
//---------------------------------------------------------------------------
const char* JoyCyberD::AxisDescTable[] = {
	"X",
	"Y",
	"Throttle"
};

//---------------------------------------------------------------------------
//
//	ボタン表示テーブル
//
//---------------------------------------------------------------------------
const char* JoyCyberD::ButtonDescTable[] = {
	"A",
	"B",
	"C",
	"D",
	"E1",
	"E2"
};

//===========================================================================
//
//	ジョイスティック(MD3ボタン)
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	コンストラクタ
//
//---------------------------------------------------------------------------
JoyMd3::JoyMd3(PPI *parent, int no) : JoyDevice(parent, no)
{
	// タイプMD3B
	id = MAKEID('M', 'D', '3', 'B');
	type = 5;

	// 2軸4ボタン、データ数2
	axes = 2;
	buttons = 4;
	datas = 2;

	// 表示テーブル
	axis_desc = AxisDescTable;
	button_desc = ButtonDescTable;

	// データバッファ確保
	data = new DWORD[datas];

	// 初期データ設定
	data[0] = 0xf3;
	data[1] = 0xff;
}

//---------------------------------------------------------------------------
//
//	ポート読み取り(Read Only)
//
//---------------------------------------------------------------------------
DWORD FASTCALL JoyMd3::ReadOnly(DWORD ctl) const
{
	ASSERT(this);
	ASSERT(ctl < 0x100);
	ASSERT(data[0] < 0x100);
	ASSERT(data[1] < 0x100);

	// PC4によって分ける
	if (ctl & 1) {
		return data[1];
	}
	else {
		return data[0];
	}
}

//---------------------------------------------------------------------------
//
//	データ作成
//
//---------------------------------------------------------------------------
void FASTCALL JoyMd3::MakeData()
{
	const PPI::joyinfo_t *info;
	DWORD axis;

	ASSERT(this);
	ASSERT(ppi);

	// データ初期化
	info = ppi->GetJoyInfo(port);
	data[0] = 0xf3;
	data[1] = 0xff;

	// Up
	axis = info->axis[1];
	if ((axis >= 0xfffff800) && (axis < 0xfffffc00)) {
		data[1] &= ~0x01;
		data[0] &= ~0x01;
	}
	// Down
	if ((axis >= 0x00000400) && (axis < 0x00000800)) {
		data[1] &= ~0x02;
		data[0] &= ~0x02;
	}

	// Left
	axis = info->axis[0];
	if ((axis >= 0xfffff800) && (axis < 0xfffffc00)) {
		data[1] &= ~0x04;
	}
	// Right
	if ((axis >= 0x00000400) && (axis < 0x00000800)) {
		data[1] &= ~0x08;
	}

	// ボタンB
	if (info->button[1]) {
		data[1] &= ~0x20;
	}

	// ボタンC
	if (info->button[2]) {
		data[1] &= ~0x40;
	}

	// ボタンA
	if (info->button[0]) {
		data[0] &= ~0x20;
	}

	// スタートボタン
	if (info->button[3]) {
		data[0] &= ~0x40;
	}
}

//---------------------------------------------------------------------------
//
//	軸表示テーブル
//
//---------------------------------------------------------------------------
const char* JoyMd3::AxisDescTable[] = {
	"X",
	"Y"
};

//---------------------------------------------------------------------------
//
//	ボタン表示テーブル
//
//---------------------------------------------------------------------------
const char* JoyMd3::ButtonDescTable[] = {
	"A",
	"B",
	"C",
	"START"
};

//===========================================================================
//
//	ジョイスティック(MD6ボタン)
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	コンストラクタ
//
//---------------------------------------------------------------------------
JoyMd6::JoyMd6(PPI *parent, int no) : JoyDevice(parent, no)
{
	// タイプMD6B
	id = MAKEID('M', 'D', '6', 'B');
	type = 6;

	// 2軸8ボタン、データ数3
	axes = 2;
	buttons = 8;
	datas = 5;

	// 表示テーブル
	axis_desc = AxisDescTable;
	button_desc = ButtonDescTable;

	// データバッファ確保
	data = new DWORD[datas];

	// 初期データ設定
	data[0] = 0xf3;
	data[1] = 0xff;
	data[2] = 0xf0;
	data[3] = 0xff;
	data[4] = 0xff;

	// スケジューラ取得
	scheduler = (Scheduler*)ppi->GetVM()->SearchDevice(MAKEID('S', 'C', 'H', 'E'));
	ASSERT(scheduler);

	// 自動リセット(コントローラを差し替えた場合に備える)
	Reset();
}

//---------------------------------------------------------------------------
//
//	リセット
//
//---------------------------------------------------------------------------
void FASTCALL JoyMd6::Reset()
{
	ASSERT(this);
	ASSERT(scheduler);

	// 基本クラス
	JoyDevice::Reset();

	// シーケンス、コントロール、時間を初期化
	seq = 0;
	ctrl = 0;
	hus = scheduler->GetTotalTime();
}

//---------------------------------------------------------------------------
//
//	セーブ
//
//---------------------------------------------------------------------------
BOOL FASTCALL JoyMd6::Save(Fileio *fio, int ver)
{
	ASSERT(this);
	ASSERT(fio);

	// 基本クラス
	if (!JoyDevice::Save(fio, ver)) {
		return FALSE;
	}

	// シーケンスをセーブ
	if (!fio->Write(&seq, sizeof(seq))) {
		return FALSE;
	}

	// コントロールをセーブ
	if (!fio->Write(&ctrl, sizeof(ctrl))) {
		return FALSE;
	}

	// 時間をセーブ
	if (!fio->Write(&hus, sizeof(hus))) {
		return FALSE;
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	ロード
//
//---------------------------------------------------------------------------
BOOL FASTCALL JoyMd6::Load(Fileio *fio, int ver)
{
	ASSERT(this);
	ASSERT(fio);

	// 基本クラス
	if (!JoyDevice::Load(fio, ver)) {
		return FALSE;
	}

	// シーケンスをロード
	if (!fio->Read(&seq, sizeof(seq))) {
		return FALSE;
	}

	// コントロールをロード
	if (!fio->Read(&ctrl, sizeof(ctrl))) {
		return FALSE;
	}

	// 時間をロード
	if (!fio->Read(&hus, sizeof(hus))) {
		return FALSE;
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	ポート読み取り(Read Only)
//
//---------------------------------------------------------------------------
DWORD FASTCALL JoyMd6::ReadOnly(DWORD /*ctl*/) const
{
	ASSERT(this);
	ASSERT(data[0] < 0x100);
	ASSERT(data[1] < 0x100);
	ASSERT(data[2] < 0x100);
	ASSERT(data[3] < 0x100);
	ASSERT(data[4] < 0x100);

	// シーケンス別
	switch (seq) {
		// 初期状態 CTL=0
		case 0:
			return data[0];

		// 1回目 CTL=1
		case 1:
			return data[1];

		// 1回目 CTL=0
		case 2:
			return data[0];

		// 2回目 CTL=1
		case 3:
			return data[1];

		// 6B確定後 CTL=0
		case 4:
			return data[2];

		// 6B確定後 CTL=1
		case 5:
			return data[3];

		// 6B確定後 CTL=0
		case 6:
			return data[4];

		// 6B確定後 CTL=1
		case 7:
			return data[1];

		// 6B確定後 CTL=0
		case 8:
			return data[0];

		// その他(あり得ない)
		default:
			ASSERT(FALSE);
			break;
	}

	return 0xff;
}

//---------------------------------------------------------------------------
//
//	コントロール
//
//---------------------------------------------------------------------------
void FASTCALL JoyMd6::Control(DWORD ctl)
{
	DWORD diff;

	ASSERT(this);
	ASSERT(ctl < 0x100);

	// bit0のみ必要
	ctl &= 0x01;

	// 必ず更新
	ctrl = ctl;

	// seq >= 3なら、前回の起動から1.8ms(3600hus)経過したかチェック
	// 経過していれば、seq=0 or seq=1にリセット(女帝戦記V4)
	if (seq >= 3) {
		diff = scheduler->GetTotalTime();
		diff -= hus;
		if (diff >= 3600) {
			// リセット
			if (ctl) {
				seq = 1;
				hus = scheduler->GetTotalTime();
			}
			else {
				seq = 0;
			}
			return;
		}
	}

	switch (seq) {
		// シーケンス外 CTL=0
		case 0:
			// 1なら、シーケンス1と時間記憶
			if (ctl) {
				seq = 1;
				hus = scheduler->GetTotalTime();
			}
			break;

		// 最初の1の後 CTL=1
		case 1:
			// 0なら、シーケンス2
			if (!ctl) {
				seq = 2;
			}
			break;

		// 1→0の後 CTL=0
		case 2:
			// 1なら、時間チェック
			if (ctl) {
				diff = scheduler->GetTotalTime();
				diff -= hus;
				if (diff <= 2200) {
					// 1.1ms(2200hus)以下なら次のシーケンスへ(6B読み取り)
					seq = 3;
				}
				else {
					// 十分時間が空いているので、シーケンス1と同じとみなす(3B読み取り)
					seq = 1;
					hus = scheduler->GetTotalTime();
				}
			}
			break;

		// 6B確定後 CTL=1
		case 3:
			if (!ctl) {
				seq = 4;
			}
			break;

		// 6B確定後 CTL=0
		case 4:
			if (ctl) {
				seq = 5;
			}
			break;

		// 6B確定後 CTL=1
		case 5:
			if (!ctl) {
				seq = 6;
			}
			break;

		// 6B確定後 CTL=0
		case 6:
			if (ctl) {
				seq = 7;
			}
			break;

		// 1.8msの待ち
		case 7:
			if (!ctl) {
				seq = 8;
			}
			break;

		// 1.8msの待ち
		case 8:
			if (ctl) {
				seq = 7;
			}
			break;

		// その他(あり得ない)
		default:
			ASSERT(FALSE);
			break;
	}
}

//---------------------------------------------------------------------------
//
//	データ作成
//
//---------------------------------------------------------------------------
void FASTCALL JoyMd6::MakeData()
{
	const PPI::joyinfo_t *info;
	DWORD axis;

	ASSERT(this);
	ASSERT(ppi);

	// データ初期化
	info = ppi->GetJoyInfo(port);
	data[0] = 0xf3;
	data[1] = 0xff;
	data[2] = 0xf0;
	data[3] = 0xff;
	data[4] = 0xff;

	// Up(data[0], data[1], data[4])
	axis = info->axis[1];
	if ((axis >= 0xfffff800) && (axis < 0xfffffc00)) {
		data[0] &= ~0x01;
		data[1] &= ~0x01;
		data[4] &= ~0x01;
	}
	// Down(data[0], data[1], data[4])
	if ((axis >= 0x00000400) && (axis < 0x00000800)) {
		data[0] &= ~0x02;
		data[1] &= ~0x02;
		data[4] &= ~0x02;
	}

	// Left(data[1], data[4])
	axis = info->axis[0];
	if ((axis >= 0xfffff800) && (axis < 0xfffffc00)) {
		data[1] &= ~0x04;
		data[4] &= ~0x04;
	}
	// Right(data[1], data[4])
	if ((axis >= 0x00000400) && (axis < 0x00000800)) {
		data[1] &= ~0x08;
		data[4] &= ~0x08;
	}

	// ボタンB(data[1], data[3], data[4])
	if (info->button[1]) {
		// 3B互換
		data[1] &= ~0x20;

		// (女帝戦記V4)
		data[3] &= ~0x20;

		// (SFII'patch)
		data[4] &= ~0x40;
	}

	// ボタンC(data[1], data[3])
	if (info->button[2]) {
		// 3B互換
		data[1] &= ~0x40;

		// (SFII'patch)
		data[3] &= ~0x20;

		// (女帝戦記V4)
		data[3] &= ~0x40;
	}

	// ボタンA(data[0], data[2], data[4])
	if (info->button[0]) {
		// 3B互換
		data[0] &= ~0x20;

		// 6Bマーカ
		data[2] &= ~0x20;

		// (SFII'patch)
		data[4] &= ~0x20;
	}

	// スタートボタン(data[0], data[2])
	if (info->button[6]) {
		// 3B互換
		data[0] &= ~0x40;

		// 6Bマーカ
		data[2] &= ~0x40;
	}

	// ボタンX(data[3])
	if (info->button[3]) {
		data[3] &= ~0x04;
	}

	// ボタンY(data[3])
	if (info->button[4]) {
		data[3] &= ~0x02;
	}

	// ボタンZ(data[3])
	if (info->button[5]) {
		data[3] &= ~0x01;
	}

	// MODEボタン(data[3])
	if (info->button[7]) {
		data[3] &= ~0x08;
	}
}

//---------------------------------------------------------------------------
//
//	軸表示テーブル
//
//---------------------------------------------------------------------------
const char* JoyMd6::AxisDescTable[] = {
	"X",
	"Y"
};

//---------------------------------------------------------------------------
//
//	ボタン表示テーブル
//
//---------------------------------------------------------------------------
const char* JoyMd6::ButtonDescTable[] = {
	"A",
	"B",
	"C",
	"X",
	"Y",
	"Z",
	"START",
	"MODE"
};

//===========================================================================
//
//	ジョイスティック(CPSF-SFC)
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	コンストラクタ
//
//---------------------------------------------------------------------------
JoyCpsf::JoyCpsf(PPI *parent, int no) : JoyDevice(parent, no)
{
	// タイプCPSF
	id = MAKEID('C', 'P', 'S', 'F');
	type = 7;

	// 2軸8ボタン、データ数2
	axes = 2;
	buttons = 8;
	datas = 2;

	// 表示テーブル
	axis_desc = AxisDescTable;
	button_desc = ButtonDescTable;

	// データバッファ確保
	data = new DWORD[datas];

	// 初期データ設定
	data[0] = 0xff;
	data[1] = 0xff;
}

//---------------------------------------------------------------------------
//
//	ポート読み取り(Read Only)
//
//---------------------------------------------------------------------------
DWORD FASTCALL JoyCpsf::ReadOnly(DWORD ctl) const
{
	ASSERT(this);
	ASSERT(ctl < 0x100);
	ASSERT(data[0] < 0x100);
	ASSERT(data[1] < 0x100);

	// PC4によって分ける
	if (ctl & 1) {
		return data[1];
	}
	else {
		return data[0];
	}
}

//---------------------------------------------------------------------------
//
//	データ作成
//
//---------------------------------------------------------------------------
void FASTCALL JoyCpsf::MakeData()
{
	const PPI::joyinfo_t *info;
	DWORD axis;

	ASSERT(this);
	ASSERT(ppi);

	// データ初期化
	info = ppi->GetJoyInfo(port);
	data[0] = 0xff;
	data[1] = 0xff;

	// Up
	axis = info->axis[1];
	if ((axis >= 0xfffff800) && (axis < 0xfffffc00)) {
		data[0] &= ~0x01;
	}
	// Down
	if ((axis >= 0x00000400) && (axis < 0x00000800)) {
		data[0] &= ~0x02;
	}

	// Left
	axis = info->axis[0];
	if ((axis >= 0xfffff800) && (axis < 0xfffffc00)) {
		data[0] &= ~0x04;
	}
	// Right
	if ((axis >= 0x00000400) && (axis < 0x00000800)) {
		data[0] &= ~0x08;
	}

	// ボタンY
	if (info->button[0]) {
		data[1] &= ~0x02;
	}

	// ボタンX
	if (info->button[1]) {
		data[1] &= ~0x04;
	}

	// ボタンB
	if (info->button[2]) {
		data[0] &= ~0x40;
	}

	// ボタンA
	if (info->button[3]) {
		data[0] &= ~0x20;
	}

	// ボタンL
	if (info->button[4]) {
		data[1] &= ~0x20;
	}

	// ボタンR
	if (info->button[5]) {
		data[1] &= ~0x01;
	}

	// スタートボタン
	if (info->button[6]) {
		data[1] &= ~0x40;
	}

	// セレクトボタン
	if (info->button[7]) {
		data[1] &= ~0x08;
	}
}

//---------------------------------------------------------------------------
//
//	軸表示テーブル
//
//---------------------------------------------------------------------------
const char* JoyCpsf::AxisDescTable[] = {
	"X",
	"Y"
};

//---------------------------------------------------------------------------
//
//	ボタン表示テーブル
//
//---------------------------------------------------------------------------
const char* JoyCpsf::ButtonDescTable[] = {
	"Y",
	"X",
	"B",
	"A",
	"L",
	"R",
	"START",
	"SELECT"
};

//===========================================================================
//
//	ジョイスティック(CPSF-MD)
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	コンストラクタ
//
//---------------------------------------------------------------------------
JoyCpsfMd::JoyCpsfMd(PPI *parent, int no) : JoyDevice(parent, no)
{
	// タイプCPSM
	id = MAKEID('C', 'P', 'S', 'M');
	type = 8;

	// 2軸8ボタン、データ数2
	axes = 2;
	buttons = 8;
	datas = 2;

	// 表示テーブル
	axis_desc = AxisDescTable;
	button_desc = ButtonDescTable;

	// データバッファ確保
	data = new DWORD[datas];

	// 初期データ設定
	data[0] = 0xff;
	data[1] = 0xff;
}

//---------------------------------------------------------------------------
//
//	ポート読み取り(Read Only)
//
//---------------------------------------------------------------------------
DWORD FASTCALL JoyCpsfMd::ReadOnly(DWORD ctl) const
{
	ASSERT(this);
	ASSERT(ctl < 0x100);
	ASSERT(data[0] < 0x100);
	ASSERT(data[1] < 0x100);

	// PC4によって分ける
	if (ctl & 1) {
		return data[1];
	}
	else {
		return data[0];
	}
}

//---------------------------------------------------------------------------
//
//	データ作成
//
//---------------------------------------------------------------------------
void FASTCALL JoyCpsfMd::MakeData()
{
	const PPI::joyinfo_t *info;
	DWORD axis;

	ASSERT(this);
	ASSERT(ppi);

	// データ初期化
	info = ppi->GetJoyInfo(port);
	data[0] = 0xff;
	data[1] = 0xff;

	// Up
	axis = info->axis[1];
	if ((axis >= 0xfffff800) && (axis < 0xfffffc00)) {
		data[0] &= ~0x01;
	}
	// Down
	if ((axis >= 0x00000400) && (axis < 0x00000800)) {
		data[0] &= ~0x02;
	}

	// Left
	axis = info->axis[0];
	if ((axis >= 0xfffff800) && (axis < 0xfffffc00)) {
		data[0] &= ~0x04;
	}
	// Right
	if ((axis >= 0x00000400) && (axis < 0x00000800)) {
		data[0] &= ~0x08;
	}

	// ボタンA
	if (info->button[0]) {
		data[0] &= ~0x20;
	}

	// ボタンB
	if (info->button[1]) {
		data[0] &= ~0x40;
	}

	// ボタンC
	if (info->button[2]) {
		data[1] &= ~0x20;
	}

	// ボタンX
	if (info->button[3]) {
		data[1] &= ~0x04;
	}

	// ボタンY
	if (info->button[4]) {
		data[1] &= ~0x02;
	}

	// ボタンZ
	if (info->button[5]) {
		data[1] &= ~0x01;
	}

	// スタートボタン
	if (info->button[6]) {
		data[1] &= ~0x40;
	}

	// MODEボタン
	if (info->button[7]) {
		data[1] &= ~0x08;
	}
}

//---------------------------------------------------------------------------
//
//	軸表示テーブル
//
//---------------------------------------------------------------------------
const char* JoyCpsfMd::AxisDescTable[] = {
	"X",
	"Y"
};

//---------------------------------------------------------------------------
//
//	ボタン表示テーブル
//
//---------------------------------------------------------------------------
const char* JoyCpsfMd::ButtonDescTable[] = {
	"A",
	"B",
	"C",
	"X",
	"Y",
	"Z",
	"START",
	"MODE"
};

//===========================================================================
//
//	ジョイスティック(マジカルパッド)
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	コンストラクタ
//
//---------------------------------------------------------------------------
JoyMagical::JoyMagical(PPI *parent, int no) : JoyDevice(parent, no)
{
	// タイプMAGI
	id = MAKEID('M', 'A', 'G', 'I');
	type = 9;

	// 2軸6ボタン、データ数2
	axes = 2;
	buttons = 6;
	datas = 2;

	// 表示テーブル
	axis_desc = AxisDescTable;
	button_desc = ButtonDescTable;

	// データバッファ確保
	data = new DWORD[datas];

	// 初期データ設定
	data[0] = 0xff;
	data[1] = 0xfc;
}

//---------------------------------------------------------------------------
//
//	ポート読み取り(Read Only)
//
//---------------------------------------------------------------------------
DWORD FASTCALL JoyMagical::ReadOnly(DWORD ctl) const
{
	ASSERT(this);
	ASSERT(ctl < 0x100);
	ASSERT(data[0] < 0x100);
	ASSERT(data[1] < 0x100);

	// PC4によって分ける
	if (ctl & 1) {
		return data[1];
	}
	else {
		return data[0];
	}
}

//---------------------------------------------------------------------------
//
//	データ作成
//
//---------------------------------------------------------------------------
void FASTCALL JoyMagical::MakeData()
{
	const PPI::joyinfo_t *info;
	DWORD axis;

	ASSERT(this);
	ASSERT(ppi);

	// データ初期化
	info = ppi->GetJoyInfo(port);
	data[0] = 0xff;
	data[1] = 0xfc;

	// Up
	axis = info->axis[1];
	if ((axis >= 0xfffff800) && (axis < 0xfffffc00)) {
		data[0] &= ~0x01;
	}
	// Down
	if ((axis >= 0x00000400) && (axis < 0x00000800)) {
		data[0] &= ~0x02;
	}

	// Left
	axis = info->axis[0];
	if ((axis >= 0xfffff800) && (axis < 0xfffffc00)) {
		data[0] &= ~0x04;
	}
	// Right
	if ((axis >= 0x00000400) && (axis < 0x00000800)) {
		data[0] &= ~0x08;
	}

	// ボタンA
	if (info->button[0]) {
		data[0] &= ~0x40;
	}

	// ボタンB
	if (info->button[1]) {
		data[1] &= ~0x40;
	}

	// ボタンC
	if (info->button[2]) {
		data[0] &= ~0x20;
	}

	// ボタンD
	if (info->button[3]) {
		data[1] &= ~0x40;
	}

	// ボタンR
	if (info->button[4]) {
		data[1] &= ~0x08;
	}

	// ボタンL
	if (info->button[5]) {
		data[1] &= ~0x04;
	}
}

//---------------------------------------------------------------------------
//
//	軸表示テーブル
//
//---------------------------------------------------------------------------
const char* JoyMagical::AxisDescTable[] = {
	"X",
	"Y"
};

//---------------------------------------------------------------------------
//
//	ボタン表示テーブル
//
//---------------------------------------------------------------------------
const char* JoyMagical::ButtonDescTable[] = {
	"A",
	"B",
	"C",
	"D",
	"R",
	"L"
};

//===========================================================================
//
//	ジョイスティック(XPD-1LR)
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	コンストラクタ
//
//---------------------------------------------------------------------------
JoyLR::JoyLR(PPI *parent, int no) : JoyDevice(parent, no)
{
	// タイプXPLR
	id = MAKEID('X', 'P', 'L', 'R');
	type = 10;

	// 4軸2ボタン、データ数2
	axes = 4;
	buttons = 2;
	datas = 2;

	// 表示テーブル
	axis_desc = AxisDescTable;
	button_desc = ButtonDescTable;

	// データバッファ確保
	data = new DWORD[datas];

	// 初期データ設定
	data[0] = 0xff;
	data[1] = 0xff;
}

//---------------------------------------------------------------------------
//
//	ポート読み取り(Read Only)
//
//---------------------------------------------------------------------------
DWORD FASTCALL JoyLR::ReadOnly(DWORD ctl) const
{
	ASSERT(this);
	ASSERT(ctl < 0x100);
	ASSERT(data[0] < 0x100);
	ASSERT(data[1] < 0x100);

	// PC4によって分ける
	if (ctl & 1) {
		return data[1];
	}
	else {
		return data[0];
	}
}

//---------------------------------------------------------------------------
//
//	データ作成
//
//---------------------------------------------------------------------------
void FASTCALL JoyLR::MakeData()
{
	const PPI::joyinfo_t *info;
	DWORD axis;

	ASSERT(this);
	ASSERT(ppi);

	// データ初期化
	info = ppi->GetJoyInfo(port);
	data[0] = 0xff;
	data[1] = 0xff;

	// 右側Up
	axis = info->axis[3];
	if ((axis >= 0xfffff800) && (axis < 0xfffffc00)) {
		data[1] &= ~0x01;
	}
	// 右側Down
	if ((axis >= 0x00000400) && (axis < 0x00000800)) {
		data[1] &= ~0x02;
	}

	// 右側Left
	axis = info->axis[2];
	if ((axis >= 0xfffff800) && (axis < 0xfffffc00)) {
		data[1] &= ~0x04;
	}
	// 右側Right
	if ((axis >= 0x00000400) && (axis < 0x00000800)) {
		data[1] &= ~0x08;
	}

	// ボタンA
	if (info->button[0]) {
		data[1] &= ~0x40;
	}

	// ボタンB
	if (info->button[1]) {
		data[1] &= ~0x20;
	}

	// 左側Up
	axis = info->axis[1];
	if ((axis >= 0xfffff800) && (axis < 0xfffffc00)) {
		data[0] &= ~0x01;
	}
	// 左側Down
	if ((axis >= 0x00000400) && (axis < 0x00000800)) {
		data[0] &= ~0x02;
	}

	// 右側Left
	axis = info->axis[0];
	if ((axis >= 0xfffff800) && (axis < 0xfffffc00)) {
		data[0] &= ~0x04;
	}
	// 右側Right
	if ((axis >= 0x00000400) && (axis < 0x00000800)) {
		data[0] &= ~0x08;
	}

	// ボタンA
	if (info->button[0]) {
		data[0] &= ~0x40;
	}

	// ボタンB
	if (info->button[1]) {
		data[0] &= ~0x20;
	}
}

//---------------------------------------------------------------------------
//
//	軸表示テーブル
//
//---------------------------------------------------------------------------
const char* JoyLR::AxisDescTable[] = {
	"Left-X",
	"Left-Y",
	"Right-X",
	"Right-Y"
};

//---------------------------------------------------------------------------
//
//	ボタン表示テーブル
//
//---------------------------------------------------------------------------
const char* JoyLR::ButtonDescTable[] = {
	"A",
	"B"
};

//===========================================================================
//
//	ジョイスティック(パックランド専用パッド)
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	コンストラクタ
//
//---------------------------------------------------------------------------
JoyPacl::JoyPacl(PPI *parent, int no) : JoyDevice(parent, no)
{
	// タイプPACL
	id = MAKEID('P', 'A', 'C', 'L');
	type = 11;

	// 0軸3ボタン、データ数1
	axes = 0;
	buttons = 3;
	datas = 1;

	// 表示テーブル
	button_desc = ButtonDescTable;

	// データバッファ確保
	data = new DWORD[datas];

	// 初期データ設定
	data[0] = 0xff;
}

//---------------------------------------------------------------------------
//
//	ポート読み取り(Read Only)
//
//---------------------------------------------------------------------------
DWORD FASTCALL JoyPacl::ReadOnly(DWORD ctl) const
{
	ASSERT(this);
	ASSERT(ctl < 0x100);
	ASSERT(data[0] < 0x100);

	// PC4が1なら、0xff
	if (ctl & 1) {
		return 0xff;
	}

	// 作成済みのデータを返す
	return data[0];
}

//---------------------------------------------------------------------------
//
//	データ作成
//
//---------------------------------------------------------------------------
void FASTCALL JoyPacl::MakeData()
{
	const PPI::joyinfo_t *info;

	ASSERT(this);
	ASSERT(ppi);

	// データ初期化
	info = ppi->GetJoyInfo(port);
	data[0] = 0xff;

	// ボタンA(Left)
	if (info->button[0]) {
		data[0] &= ~0x04;
	}

	// ボタンB(Jump)
	if (info->button[1]) {
		data[0] &= ~0x20;
	}

	// ボタンC(Right)
	if (info->button[2]) {
		data[0] &= ~0x08;
	}
}

//---------------------------------------------------------------------------
//
//	ボタン表示テーブル
//
//---------------------------------------------------------------------------
const char* JoyPacl::ButtonDescTable[] = {
	"Left",
	"Jump",
	"Right",
};

//===========================================================================
//
//	ジョイスティック(BM68専用コントローラ)
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	コンストラクタ
//
//---------------------------------------------------------------------------
JoyBM::JoyBM(PPI *parent, int no) : JoyDevice(parent, no)
{
	// タイプBM68
	id = MAKEID('B', 'M', '6', '8');
	type = 12;

	// 0軸6ボタン、データ数1
	axes = 0;
	buttons = 6;
	datas = 1;

	// 表示テーブル
	button_desc = ButtonDescTable;

	// データバッファ確保
	data = new DWORD[datas];

	// 初期データ設定
	data[0] = 0xff;
}

//---------------------------------------------------------------------------
//
//	ポート読み取り(Read Only)
//
//---------------------------------------------------------------------------
DWORD FASTCALL JoyBM::ReadOnly(DWORD ctl) const
{
	ASSERT(this);
	ASSERT(ctl < 0x100);
	ASSERT(data[0] < 0x100);

	// PC4が1なら、0xff
	if (ctl & 1) {
		return 0xff;
	}

	// 作成済みのデータを返す
	return data[0];
}

//---------------------------------------------------------------------------
//
//	データ作成
//
//---------------------------------------------------------------------------
void FASTCALL JoyBM::MakeData()
{
	const PPI::joyinfo_t *info;

	ASSERT(this);
	ASSERT(ppi);

	// データ初期化
	info = ppi->GetJoyInfo(port);
	data[0] = 0xff;

	// ボタン1(C)
	if (info->button[0]) {
		data[0] &= ~0x08;
	}

	// ボタン2(C+,D-)
	if (info->button[1]) {
		data[0] &= ~0x04;
	}

	// ボタン3(D)
	if (info->button[2]) {
		data[0] &= ~0x40;
	}

	// ボタン4(D+,E-)
	if (info->button[3]) {
		data[0] &= ~0x20;
	}

	// ボタン5(E)
	if (info->button[4]) {
		data[0] &= ~0x02;
	}

	// ボタンF(Hat)
	if (info->button[5]) {
		data[0] &= ~0x01;
	}
}

//---------------------------------------------------------------------------
//
//	ボタン表示テーブル
//
//---------------------------------------------------------------------------
const char* JoyBM::ButtonDescTable[] = {
	"C",
	"C#",
	"D",
	"D#",
	"E",
	"HiHat"
};
