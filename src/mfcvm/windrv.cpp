//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2004 ＰＩ．(ytanaka@ipc-tokai.or.jp)
//	Modified (C) 2006 co (cogood＠gmail.com)
//	[ Windrv ]
//
//---------------------------------------------------------------------------

#include "os.h"
#include "xm6.h"
#include "vm.h"
#include "log.h"
#include "schedule.h"
#include "memory.h"
#include "cpu.h"
#include "config.h"
#include "windrv.h"
#include "mfc_com.h"
#include "mfc_cfg.h"

//===========================================================================
//
//	Human68k
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	パス取得
//
//---------------------------------------------------------------------------
void FASTCALL Human68k::namests_t::GetCopyPath(BYTE* szPath) const
{
	ASSERT(this);
	ASSERT(szPath);
	// WARNING: Unicode対応時要修正

	BYTE* p = szPath;
	for (int i = 0; i < 65; i++) {
		BYTE c = path[i];
		if (c == '\0') break;
		if (c == 0x09) {
			c = '/';
		}
		*p++ = c;
	}

	*p = '\0';
}

//---------------------------------------------------------------------------
//
//	ファイル名取得
//
//	TwentyOneで生成したエントリと同等のファイル名再現能力を持たせる。
//
//	仮想ディレクトリエントリ機能が無効な場合:
//	Human68k内部でワイルドカード文字がすべて'?'に展開されてしまうた
//	め、18 + 3文字より長いファイル名の検索が不可能となる。このような
//	場合は、ワイルドカード文字変換を行なうことで、FILESによる全ファ
//	イルの検索を可能となる。なお、検索が可能になるだけでオープンはで
//	きないので注意。
//
//	すべてのファイルをオープンしたい場合はディレクトリエントリキャッ
//	シュを有効にすること。ファイル名変換が不要になり素直に動作する。
//
//---------------------------------------------------------------------------
void FASTCALL Human68k::namests_t::GetCopyFilename(BYTE* szFilename) const
{
	ASSERT(this);
	ASSERT(szFilename);
	// WARNING: Unicode対応時要修正

	int i;
	BYTE* p = szFilename;
#ifdef XM6_WINDRV_WILDCARD
	BYTE* pWild = NULL;
#endif // XM6_WINDRV_WILDCARD

	// ファイル名本体転送
	for (i = 0; i < 8; i++) {
		BYTE c = name[i];
		if (c == ' ') {
			BOOL bTerminate = TRUE;
			// ファイル名中にスペースが出現した場合、以降のエントリが続いているかどうか確認
			// add[0] が有効な文字なら続ける
			if (add[0] != '\0') {
				bTerminate = FALSE;
			} else {
				// name[i] より後に空白以外の文字が存在するなら続ける
				for (int j = i + 1; j < 8; j++) {
					if (name[j] != ' ') {
						bTerminate = FALSE;
						break;
					}
				}
			}
			// ファイル名終端なら転送終了
			if (bTerminate) break;
		}
#ifdef XM6_WINDRV_WILDCARD
		if (c == '?') {
			if (pWild == NULL) pWild = p;
		} else {
			pWild = NULL;
		}
#endif // XM6_WINDRV_WILDCARD
		*p++ = c;
	}
	// 全ての文字を読み込むと、ここで i >= 8 となる

	// ファイル名本体が8文字以上なら追加部分も加える
	if (i >= 8) {
		// ファイル名追加部分転送
		for (i = 0; i < 10; i++) {
			BYTE c = add[i];
			if (c == '\0') break;
#ifdef XM6_WINDRV_WILDCARD
			if (c == '?') {
				if (pWild == NULL) pWild = p;
			} else {
				pWild = NULL;
			}
#endif // XM6_WINDRV_WILDCARD
			*p++ = c;
		}
		// 全ての文字を読み込むと、ここで i >= 10 となる
	}

#ifdef XM6_WINDRV_WILDCARD
	// ファイル名末尾がワイルドカードで、かつ後半10文字のすべてを使用しているか？
	if (pWild && i >= 10) {
		p = pWild;
		*p++ = '*';
	}
#endif // XM6_WINDRV_WILDCARD

	// ファイル名本体は終了、拡張子の処理開始
#ifdef XM6_WINDRV_WILDCARD
	pWild = NULL;
#endif // XM6_WINDRV_WILDCARD
	i = 0;

	// 拡張子が存在するか？
	if (ext[0] == ' ' && ext[1] == ' ' && ext[2] == ' ') {
		// 拡張子なし
	} else {
		// 拡張子転送
		*p++ = '.';
		for (i = 0; i < 3; i++) {
			BYTE c = ext[i];
			if (c == ' ') {
				BOOL bTerminate = TRUE;
				// 拡張子中にスペースが出現した場合、以降のエントリが続いているかどうか確認
				// ext[i] より後に空白以外の文字が存在するなら続ける
				for (int j = i + 1; j < 3; j++) {
					if (ext[j] != ' ') {
						bTerminate = FALSE;
						break;
					}
				}
				// 拡張子終端なら転送終了
				if (bTerminate) break;
			}
#ifdef XM6_WINDRV_WILDCARD
			if (c == '?') {
				if (pWild == NULL) pWild = p;
			} else {
				pWild = NULL;
			}
#endif // XM6_WINDRV_WILDCARD
			*p++ = c;
		}
		// 全ての文字を読み込むと、ここで i >= 3 となる
	}

#ifdef XM6_WINDRV_WILDCARD
	// 拡張子末尾がワイルドカードで、かつ拡張子3文字すべてを使用しているか？
	if (pWild && i >= 3) {
		p = pWild;
		*p++ = '*';
	}
#endif // XM6_WINDRV_WILDCARD

	// 番兵追加
	*p = '\0';
}

//===========================================================================
//
//	コマンドハンドラ
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	コンストラクタ
//
//---------------------------------------------------------------------------
CWindrv::CWindrv(Windrv* pWindrv, Memory* pMemory, DWORD nHandle)
{
	windrv = pWindrv;
	memory = pMemory;
	a5 = 0;
	unit = 0;
	command = 0;

	m_nHandle = nHandle;
	m_bAlloc = FALSE;
	m_bExecute = FALSE;
	m_bReady = FALSE;
	m_bTerminate = FALSE;
	m_hThread = NULL;
	m_hEventStart = NULL;
	m_hEventReady = NULL;
}

//---------------------------------------------------------------------------
//
//	デストラクタ
//
//---------------------------------------------------------------------------
CWindrv::~CWindrv()
{
	Terminate();
}

//---------------------------------------------------------------------------
//
//	スレッド起動
//
//---------------------------------------------------------------------------
BOOL FASTCALL CWindrv::Start()
{
	ASSERT(this);
	ASSERT(m_bAlloc == FALSE);
	ASSERT(m_bExecute == FALSE);
	ASSERT(m_bTerminate == FALSE);
	ASSERT(m_hThread == NULL);
	ASSERT(m_hEventStart == NULL);
	ASSERT(m_hEventReady == NULL);

	// ハンドル確保
	m_hEventStart = ::CreateEvent(NULL, FALSE, FALSE, NULL);	// 自動リセット
	ASSERT(m_hEventStart);

	m_hEventReady = ::CreateEvent(NULL, TRUE, FALSE, NULL);	// 手動リセット 注意
	ASSERT(m_hEventReady);

	// スレッド生成
	DWORD nThread;
	m_hThread = ::CreateThread(NULL, 0, Run, this, 0, &nThread);
	ASSERT(m_hThread);

	// エラーチェック
	if (m_hEventStart == NULL ||
		m_hEventReady == NULL ||
		m_hThread == NULL) return FALSE;

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	スレッド停止
//
//---------------------------------------------------------------------------
BOOL FASTCALL CWindrv::Terminate()
{
	ASSERT(this);

	BOOL bResult = TRUE;

	if (m_hThread) {
		ASSERT(m_bTerminate == FALSE);
		ASSERT(m_hEventStart);

		// スレッドへ停止要求
		m_bTerminate = TRUE;
		::SetEvent(m_hEventStart);

		// スレッド終了待ち
		DWORD nResult;
		nResult = ::WaitForSingleObject(m_hThread, 30 * 1000);	// 猶予期間 30秒
		if (nResult != WAIT_OBJECT_0) {
			// 強制停止
			ASSERT(FALSE);	// 念のため
			::TerminateThread(m_hThread, -1);
			nResult = ::WaitForSingleObject(m_hThread, 100);
			bResult = FALSE;
		}

		// スレッドハンドル開放
		::CloseHandle(m_hThread);
		m_hThread = NULL;

		// 停止要求を取り下げる
		m_bTerminate = FALSE;
	}

	// イベントハンドル開放
	if (m_hEventStart) {
		::CloseHandle(m_hEventStart);
		m_hEventStart = NULL;
	}

	if (m_hEventReady) {
		::CloseHandle(m_hEventReady);
		m_hEventReady = NULL;
	}

	// そのまま同じオブジェクトでスレッド起動できるよう初期化
	m_bAlloc = FALSE;
	m_bExecute = FALSE;
	m_bReady = FALSE;
	m_bTerminate = FALSE;

	return bResult;
}

//---------------------------------------------------------------------------
//
//	コマンド実行
//
//---------------------------------------------------------------------------
void FASTCALL CWindrv::Execute(DWORD nA5)
{
	ASSERT(this);
	ASSERT(m_bExecute == FALSE);
	ASSERT(m_bTerminate == FALSE);
	ASSERT(m_hThread);
	ASSERT(m_hEventStart);
	ASSERT(m_hEventReady);
	ASSERT(nA5 <= 0xFFFFFF);

	// 実行開始フラグセット
	m_bExecute = TRUE;

	// リクエストヘッダのアドレス
	a5 = nA5;

	// エラー情報をクリア
	SetByte(a5 + 3, 0);
	SetByte(a5 + 4, 0);

	// ユニット番号獲得
	unit = GetByte(a5 + 1);

	// コマンド獲得
	command = GetByte(a5 + 2);

	// スレッド待機準備
	m_bReady = FALSE;
	::ResetEvent(m_hEventReady);

	// 実行開始イベントセット
	::SetEvent(m_hEventStart);

	// マルチスレッド開始要求が来るまでは実質シングルスレッドとして振る舞う
	DWORD nResult = ::WaitForSingleObject(m_hEventReady, INFINITE);
	ASSERT(nResult == WAIT_OBJECT_0);	// 念のため
}

#ifdef WINDRV_SUPPORT_COMPATIBLE
//---------------------------------------------------------------------------
//
//	コマンド実行 (WINDRV互換)
//
//---------------------------------------------------------------------------
DWORD FASTCALL CWindrv::ExecuteCompatible(DWORD nA5)
{
	ASSERT(this);
	ASSERT(nA5 <= 0xFFFFFF);

	// リクエストヘッダのアドレス
	a5 = nA5;

	// エラー情報をクリア
	SetByte(a5 + 3, 0);
	SetByte(a5 + 4, 0);

	// ユニット番号獲得
	unit = GetByte(a5 + 1);

	// コマンド獲得
	command = GetByte(a5 + 2);

	// コマンド実行
	ExecuteCommand();

	return GetByte(a5 + 3) | (GetByte(a5 + 4) << 8);
}
#endif // WINDRV_SUPPORT_COMPATIBLE

//---------------------------------------------------------------------------
//
//	コマンド完了を待たずにVMスレッド実行再開
//
//---------------------------------------------------------------------------
void FASTCALL CWindrv::Ready()
{
	ASSERT(this);
	ASSERT(m_hEventReady);

	m_bReady = TRUE;
	::SetEvent(m_hEventReady);
}

//---------------------------------------------------------------------------
//
//	スレッド実行開始ポイント
//
//---------------------------------------------------------------------------
DWORD WINAPI CWindrv::Run(VOID* pThis)
{
	ASSERT(pThis);

	((CWindrv*)pThis)->Runner();

	::ExitThread(0);
	return 0;
}

//---------------------------------------------------------------------------
//
//	スレッド実体
//
//---------------------------------------------------------------------------
void FASTCALL CWindrv::Runner()
{
	ASSERT(this);
	ASSERT(m_hEventStart);
	ASSERT(m_hEventReady);
	//ASSERT(m_bTerminate == FALSE);
	// スレッド開始直後にいきなりTerminateすると、この時点で m_bTerminate が
	// TRUEとなる可能性が高い。時間経過によって終了し、終了の瞬間に同期して
	// いるためASSERTなしでも問題ない。

	for (;;) {
		DWORD nResult = ::WaitForSingleObject(m_hEventStart, INFINITE);
		if (nResult != WAIT_OBJECT_0) {
			ASSERT(FALSE);	// 念のため
			break;
		}
		if (m_bTerminate) break;

		// 実行
		ExecuteCommand();

		// 実行終了前に通知
		Ready();

		// 実行完了
		m_bExecute = FALSE;
	}

	Ready();	// 念のため
}

//===========================================================================
//
//	コマンドハンドラ管理
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	初期化
//
//	全スレッド確保し、待機状態にする。
//
//---------------------------------------------------------------------------
void FASTCALL CWindrvManager::Init(Windrv* pWindrv, Memory* pMemory)
{
	ASSERT(this);
	ASSERT(pWindrv);
	ASSERT(pMemory);

	for (int i = 0; i < WINDRV_THREAD_MAX; i++) {
		m_pThread[i] = new CWindrv(pWindrv, pMemory, i);
		m_pThread[i]->Start();
	}
}

//---------------------------------------------------------------------------
//
//	終了
//
//	全てのスレッドを停止させる。
//
//---------------------------------------------------------------------------
void FASTCALL CWindrvManager::Clean()
{
	ASSERT(this);

	for (int i = 0; i < WINDRV_THREAD_MAX; i++) {
		ASSERT(m_pThread[i]);	// 念のため
		delete m_pThread[i];
		m_pThread[i] = NULL;
	}
}

//---------------------------------------------------------------------------
//
//	リセット
//
//	実行中のスレッドのみ再起動させる。
//
//---------------------------------------------------------------------------
void FASTCALL CWindrvManager::Reset()
{
	ASSERT(this);

	for (int i = 0; i < WINDRV_THREAD_MAX; i++) {
		ASSERT(m_pThread[i]);	// 念のため
		if (m_pThread[i]->isAlloc()) {
			m_pThread[i]->Terminate();
			m_pThread[i]->Start();
		}
	}
}

//---------------------------------------------------------------------------
//
//	空きスレッド確保
//
//---------------------------------------------------------------------------
CWindrv* FASTCALL CWindrvManager::Alloc()
{
	ASSERT(this);

	for (int i = 0; i < WINDRV_THREAD_MAX; i++) {
		if (m_pThread[i]->isAlloc() == FALSE) {
			m_pThread[i]->SetAlloc(TRUE);
			return m_pThread[i];
		}
	}

	return NULL;
}

//---------------------------------------------------------------------------
//
//	スレッド検索
//
//---------------------------------------------------------------------------
CWindrv* FASTCALL CWindrvManager::Search(DWORD nHandle)
{
	ASSERT(this);

	if (nHandle >= WINDRV_THREAD_MAX) return NULL;
	return m_pThread[nHandle];
}

//---------------------------------------------------------------------------
//
//	スレッド開放
//
//---------------------------------------------------------------------------
void FASTCALL CWindrvManager::Free(CWindrv* p)
{
	ASSERT(this);
	ASSERT(p);

	p->SetAlloc(FALSE);
}

//===========================================================================
//
//	Windrv
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	コンストラクタ
//
//---------------------------------------------------------------------------
Windrv::Windrv(VM *p) : MemDevice(p)
{
	// デバイスIDを初期化
	dev.id = MAKEID('W', 'D', 'R', 'V');
	dev.desc = "Windrv";

	// 開始アドレス、終了アドレス
	memdev.first = 0xe9e000;
	memdev.last = 0xe9ffff;
}

//---------------------------------------------------------------------------
//
//	初期化
//
//---------------------------------------------------------------------------
BOOL FASTCALL Windrv::Init()
{
	ASSERT(this);

	// 基本クラス
	if (!MemDevice::Init()) {
		return FALSE;
	}

	// グローバルフラグを初期化
	windrv.enable = WINDRV_MODE_NONE;

	// ドライブ0、ファイルシステム無し
	windrv.drives = 0;
	windrv.fs = NULL;

	// スレッド初期化
	Memory* memory = (Memory*)vm->SearchDevice(MAKEID('M', 'E', 'M', ' '));
	ASSERT(memory);
	m_cThread.Init(this, memory);

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	クリーンアップ
//
//---------------------------------------------------------------------------
void FASTCALL Windrv::Cleanup()
{
	ASSERT(this);

	// スレッド全開放
	m_cThread.Clean();

	// ファイルシステムがあれば、リセット
	if (windrv.fs) {
		windrv.fs->Reset();
	}

	// 基本クラスへ
	MemDevice::Cleanup();
}

//---------------------------------------------------------------------------
//
//	リセット
//
//---------------------------------------------------------------------------
void FASTCALL Windrv::Reset()
{
	ASSERT(this);

	LOG(Log::Normal, "リセット");

	// スレッドが実行中であれば、リセット
	m_cThread.Reset();

	// ファイルシステムがあれば、リセット
	if (windrv.fs) {
		windrv.fs->Reset();
	}

	// ドライブ数初期化
	windrv.drives = 0;
}

//---------------------------------------------------------------------------
//
//	セーブ
//
//---------------------------------------------------------------------------
BOOL FASTCALL Windrv::Save(Fileio *fio, int ver)
{
	ASSERT(this);
	LOG(Log::Normal, "セーブ");

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	ロード
//
//---------------------------------------------------------------------------
BOOL FASTCALL Windrv::Load(Fileio *fio, int ver)
{
	ASSERT(this);
	LOG(Log::Normal, "ロード");

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	設定適用
//
//---------------------------------------------------------------------------
void FASTCALL Windrv::ApplyCfg(const Config* pConfig)
{
	ASSERT(this);
	ASSERT(pConfig);
	LOG(Log::Normal, "設定適用");

	// 動作フラグを設定
	windrv.enable = pConfig->windrv_enable;
}

//---------------------------------------------------------------------------
//
//	バイト読み込み
//
//---------------------------------------------------------------------------
DWORD FASTCALL Windrv::ReadByte(DWORD addr)
{
	ASSERT(this);
	ASSERT((addr >= memdev.first) && (addr <= memdev.last));

	// ポート1
	if (addr == 0xE9F001) {
		return StatusAsynchronous();
	}

	DWORD result = ReadOnly(addr);

#ifdef WINDRV_LOG
	switch (result) {
	case 'X':
		LOG(Log::Normal, "Windrvポート 未サポート");
		break;
	case 'Y':
		LOG(Log::Normal, "Windrv");
		break;
#ifdef WINDRV_SUPPORT_COMPATIBLE
	case 'W':
		LOG(Log::Normal, "Windrvポート 互換動作");
		break;
#endif // WINDRV_SUPPORT_COMPATIBLE
	}
#endif // WINDRV_LOG

	if (result == 0xFF) {
		cpu->BusErr(addr, TRUE);
	}

	return result;
}

//---------------------------------------------------------------------------
//
//	読み込みのみ
//
//---------------------------------------------------------------------------
DWORD FASTCALL Windrv::ReadOnly(DWORD addr) const
{
	ASSERT(this);
	ASSERT((addr >= memdev.first) && (addr <= memdev.last));

	// 識別ポート以外は-1
	if (addr != 0xE9F000) {
		return 0xFF;
	}

	switch (windrv.enable) {
	case WINDRV_MODE_DUAL:
	case WINDRV_MODE_ENABLE:
		return 'Y';			// WindrvXM

	case WINDRV_MODE_COMPATIBLE:
#ifdef WINDRV_SUPPORT_COMPATIBLE
		return 'W';			// WINDRV互換
#endif // WINDRV_SUPPORT_COMPATIBLE

	case WINDRV_MODE_NONE:
		return 'X';			// 使用不可 (I/O空間は存在する)
	}

	// I/O空間が存在しない場合は-1
	return 0xFF;
}

//---------------------------------------------------------------------------
//
//	バイト書き込み
//
//	windrvポートへの書き込み後、d0.wでリザルトを通知する。
//
//	上位8bitは次の通り。
//		$1x	エラー(中止のみ)
//		$2x	エラー(再実行のみ)
//		$3x	エラー(再実行＆中止)
//		$4x	エラー(無視のみ)
//		$5x	エラー(無視＆中止)
//		$6x	エラー(無視＆再実行)
//		$7x	エラー(無視＆再実行＆中止)
//	下位8bitは次の通り。
//		$01	無効なユニット番号を指定した.
//		$02	ディスクが入っていない.
//		$03	デバイスドライバに無効なコマンドを指定した.
//		$04	CRC エラー.
//		$05	ディスクの管理領域が破壊されている.
//		$06	シークエラー.
//		$07	無効なメディア.
//		$08	セクタが見つからない.
//		$09	プリンタが繋がっていない.
//		$0a	書き込みエラー.
//		$0b	読み込みエラー.
//		$0c	その他のエラー.
//		$0d	ライトプロテクト(プロテクトを外して同じディスクを入れる).
//		$0e	書き込み不可能.
//		$0f	ファイル共有違反.
//
//---------------------------------------------------------------------------
void FASTCALL Windrv::WriteByte(DWORD addr, DWORD data)
{
	ASSERT(this);
	ASSERT((addr >= memdev.first) && (addr <= memdev.last));

	// サポート時のみポート判定
	switch (windrv.enable) {
	case WINDRV_MODE_DUAL:
	case WINDRV_MODE_ENABLE:
		switch (addr) {
		case 0xE9F000:
#ifdef WINDRV_SUPPORT_COMPATIBLE
			if (windrv.enable == WINDRV_MODE_DUAL) {
				Execute();	// 可能ならここだけでも残したい
			}
#endif // WINDRV_SUPPORT_COMPATIBLE
			return;		// ポート未実装でもバスエラーにしない(重要)
		case 0xE9F001:
			switch (data) {
			case 0x00: ExecuteAsynchronous(); return;
			case 0xFF: ReleaseAsynchronous(); return;
			}
			return;		// 不正な値の書き込みでもバスエラーにしない(拡張用)
		}
		break;

	case WINDRV_MODE_COMPATIBLE:
#ifdef WINDRV_SUPPORT_COMPATIBLE
		switch (addr) {
		case 0xE9F000:
			Execute();
			return;
		}
		break;
#endif // WINDRV_SUPPORT_COMPATIBLE

	case WINDRV_MODE_NONE:
		switch (addr) {
		case 0xE9F000:
		case 0xE9F001:
			return;		// ポートアドレスのみ実装 (旧バージョンのXM6との互換性のため)
		}
		break;
	}

	// 書き込み先がなければバスエラー
	cpu->BusErr(addr, FALSE);
	return;
}

#ifdef WINDRV_SUPPORT_COMPATIBLE
//---------------------------------------------------------------------------
//
//	コマンドハンドラ
//
//---------------------------------------------------------------------------
void FASTCALL Windrv::Execute()
{
	ASSERT(this);

	// リクエストヘッダ受け取り
	CPU::cpu_t reg;
	cpu->GetCPU(&reg);
	DWORD a5 = reg.areg[5];
	ASSERT(a5 <= 0xFFFFFF);

	// 実行
	CWindrv* ps = m_cThread.Alloc();
	ASSERT(ps);
	DWORD result = ps->ExecuteCompatible(a5);
	m_cThread.Free(ps);

	// WINDRV互換動作のためD0.L設定
	reg.dreg[0] = result;
	cpu->SetCPU(&reg);

	// 同期型は全てのコマンドで128usを消費
	scheduler->Wait(128);	// TODO: 正確な値を算出する
}
#endif // WINDRV_SUPPORT_COMPATIBLE

//---------------------------------------------------------------------------
//
//	コマンド実行開始 非同期
//
//---------------------------------------------------------------------------
void FASTCALL Windrv::ExecuteAsynchronous()
{
	ASSERT(this);

	// リクエストヘッダ受け取り
	CPU::cpu_t reg;
	cpu->GetCPU(&reg);
	DWORD a5 = reg.areg[5];
	ASSERT(a5 <= 0xFFFFFF);

	// 空きスレッドを捜す
	CWindrv* thread = m_cThread.Alloc();
	if (thread) {
		// スレッド実行開始
		thread->Execute(a5);
		reg.dreg[0] = thread->GetHandle();
	} else {
		// スレッドが見つからなければ-1
		reg.dreg[0] = (DWORD)-1;
		::Sleep(0);		// スレッド実行権を一瞬移す
	}

	// ハンドルをセットして即終了
	cpu->SetCPU(&reg);
}

//---------------------------------------------------------------------------
//
//	ステータス獲得 非同期
//
//---------------------------------------------------------------------------
DWORD FASTCALL Windrv::StatusAsynchronous()
{
	ASSERT(this);

	// ハンドル受け取り
	CPU::cpu_t reg;
	cpu->GetCPU(&reg);
	DWORD handle = reg.dreg[0];

	// 該当するハンドルのステータスを獲得
	CWindrv* thread = m_cThread.Search(handle);
	if (thread) {
		if (thread->isExecute()) {
			::Sleep(0);		// スレッド実行権を一瞬移す
			return 0;		// まだ実行中なら0
		}

		return 1;			// 実行完了なら1
	}

	return 0xFF;			// 不正なハンドルなら-1
}

//---------------------------------------------------------------------------
//
//	ハンドル開放 非同期
//
//---------------------------------------------------------------------------
void FASTCALL Windrv::ReleaseAsynchronous()
{
	ASSERT(this);

	// D0.L(ハンドル)受け取り
	CPU::cpu_t reg;
	cpu->GetCPU(&reg);
	DWORD handle = reg.dreg[0];

	// 該当するハンドルを開放
	CWindrv* thread = m_cThread.Search(handle);
	if (thread) {
		m_cThread.Free(thread);
	}
}

//---------------------------------------------------------------------------
//
//	コマンド実行
//
//---------------------------------------------------------------------------
void FASTCALL CWindrv::ExecuteCommand()
{
	ASSERT(this);
	ASSERT(a5 <= 0xFFFFFF);
	ASSERT(windrv);

	if (command == 0x40) {
		// $40 - 初期化
		InitDrive();
		return;
	}

	if (windrv->isInvalidUnit(unit)) {
		LockXM();

		// 無効なユニット番号を指定した
		SetResult(FS_FATAL_INVALIDUNIT);

		UnlockXM();
		return;
	}

	// コマンド分岐
	switch (command & 0x7F) {
		case 0x41: CheckDir(); return;		// $41 - ディレクトリチェック
		case 0x42: MakeDir(); return;		// $42 - ディレクトリ作成
		case 0x43: RemoveDir(); return;		// $43 - ディレクトリ削除
		case 0x44: Rename(); return;		// $44 - ファイル名変更
		case 0x45: Delete(); return;		// $45 - ファイル削除
		case 0x46: Attribute(); return;		// $46 - ファイル属性取得/設定
		case 0x47: Files(); return;			// $47 - ファイル検索(First)
		case 0x48: NFiles(); return;		// $48 - ファイル検索(Next);
		case 0x49: Create(); return;		// $49 - ファイル作成
		case 0x4A: Open(); return;			// $4A - ファイルオープン
		case 0x4B: Close(); return;			// $4B - ファイルクローズ
		case 0x4C: Read(); return;			// $4C - ファイル読み込み
		case 0x4D: Write(); return;			// $4D - ファイル書き込み
		case 0x4E: Seek(); return;			// $4E - ファイルシーク
		case 0x4F: TimeStamp(); return;		// $4F - ファイル更新時刻の取得/設定
		case 0x50: GetCapacity(); return;	// $50 - 容量取得
		case 0x51: CtrlDrive(); return;		// $51 - ドライブ制御/状態検査
		case 0x52: GetDPB(); return;		// $52 - DPB取得
		case 0x53: DiskRead(); return;		// $53 - セクタ読み込み
		case 0x54: DiskWrite(); return;		// $54 - セクタ書き込み
		case 0x55: IoControl(); return;		// $55 - IOCTRL
		case 0x56: Flush(); return;			// $56 - フラッシュ
		case 0x57: CheckMedia(); return;	// $57 - メディア交換チェック
		case 0x58: Lock(); return;			// $58 - 排他制御
	}


	LockXM();

	// デバイスドライバに無効なコマンドを指定した
	SetResult(FS_FATAL_INVALIDCOMMAND);

	UnlockXM();
}

//---------------------------------------------------------------------------
//
//	ファイルシステム設定
//
//---------------------------------------------------------------------------
void FASTCALL Windrv::SetFileSys(FileSys *fs)
{
	ASSERT(this);

	// 現在存在した場合、一致しなければ一旦閉じる
	if (windrv.fs) {
		if (windrv.fs != fs) {
			// 閉じる
			Reset();
		}
	}

	// NULLまたは実体
	windrv.fs = fs;
}

//---------------------------------------------------------------------------
//
//	ファイルシステム設定
//
//---------------------------------------------------------------------------
BOOL FASTCALL Windrv::isInvalidUnit(DWORD unit) const
{
	ASSERT(this);

	if (unit >= windrv.drives) return TRUE;	// 境界より大きければ不正なユニット番号

	ASSERT(windrv.fs);
	// 厳密にやるなら、対象となるFileSysの動作モードを問い合わせるべきだが、
	// アクセス不可能なファイルシステムはそもそも登録されないのでチェック不要
	return FALSE;
}

//---------------------------------------------------------------------------
//
//	$40 - 初期化
//
//	in	(offset	size)
//		 0	1.b	定数(22)
//		 2	1.b	コマンド($40/$c0)
//		18	1.l	パラメータアドレス
//		22	1.b	ドライブ番号
//	out	(offset	size)
//		 3	1.b	エラーコード(下位)
//		 4	1.b	〃	    (上位)
//		13	1.b	ユニット数
//		14	1.l	デバイスドライバの終了アドレス + 1
//
//	　ローカルドライブのコマンド 0 と同様に組み込み時に呼ばれるが、BPB 及
//	びそのポインタの配列を用意する必要はない.
//
//---------------------------------------------------------------------------
void FASTCALL CWindrv::InitDrive()
{
	ASSERT(this);
	ASSERT(a5 <= 0xFFFFFF);
	ASSERT(windrv);
	ASSERT(windrv->GetFilesystem());

#ifdef WINDRV_LOG
	Log(Log::Normal, "$40 - 初期化");
#endif // WINDRV_LOG

	LockXM();

	// ベースドライブ名を獲得
	DWORD base = GetByte(a5 + 22);
	ASSERT(base < 26);
	// DRIVEコマンドで順序が変わると値が無効になるため、windrv.baseは廃止

	// オプション内容を獲得
	BYTE chOption[256];
	GetParameter(GetAddr(a5 + 18), chOption, sizeof(chOption));

	UnlockXM();

	// Human68k側で利用可能なドライブ数の範囲で、ファイルシステムを構築
	DWORD max_unit = windrv->GetFilesystem()->Init(this, 26 - base, chOption);

	// 実際に確保されたユニット数を保存
	windrv->SetUnitMax(max_unit);

#ifdef WINDRV_LOG
	Log(Log::Detail, "ベースドライブ:%c サポートドライブ数:%d",
					'A' + base, max_unit);
#endif // WINDRV_LOG

	LockXM();

	// ドライブ数を返信
	SetByte(a5 + 13, max_unit);

	// 終了値
	SetResult(0);

	UnlockXM();
}

//---------------------------------------------------------------------------
//
//	$41 - ディレクトリチェック
//
//	in
//		14	1.L	NAMESTS構造体アドレス
//
//---------------------------------------------------------------------------
void FASTCALL CWindrv::CheckDir()
{
	ASSERT(this);
	ASSERT(a5 <= 0xFFFFFF);
	ASSERT(windrv);
	ASSERT(windrv->isValidUnit(unit));
	ASSERT(windrv->GetFilesystem());

	LockXM();

	// 検索対象ファイル名獲得
	Human68k::namests_t ns;
	GetNameStsPath(GetAddr(a5 + 14), &ns);

	UnlockXM();

#ifdef WINDRV_LOG
	Log(Log::Normal, "$41 %d ディレクトリチェック", unit);
#endif // WINDRV_LOG

	// ファイルシステム呼び出し
	DWORD nResult = windrv->GetFilesystem()->CheckDir(this, &ns);

	LockXM();

	// 終了値
	SetResult(nResult);

	UnlockXM();
}

//---------------------------------------------------------------------------
//
//	$42 - ディレクトリ作成
//
//	in
//		14	1.L	NAMESTS構造体アドレス
//
//---------------------------------------------------------------------------
void FASTCALL CWindrv::MakeDir()
{
	ASSERT(this);
	ASSERT(a5 <= 0xFFFFFF);
	ASSERT(windrv);
	ASSERT(windrv->isValidUnit(unit));
	ASSERT(windrv->GetFilesystem());

	LockXM();

	// 検索対象ファイル名獲得
	Human68k::namests_t ns;
	GetNameSts(GetAddr(a5 + 14), &ns);

	UnlockXM();

#ifdef WINDRV_LOG
	Log(Log::Normal, "$42 %d ディレクトリ作成", unit);
#endif // WINDRV_LOG

	// ファイルシステム呼び出し
	DWORD nResult = windrv->GetFilesystem()->MakeDir(this, &ns);

	LockXM();

	// 終了値
	SetResult(nResult);

	UnlockXM();
}

//---------------------------------------------------------------------------
//
//	$43 - ディレクトリ削除
//
//	in
//		14	1.L	NAMESTS構造体アドレス
//
//---------------------------------------------------------------------------
void FASTCALL CWindrv::RemoveDir()
{
	ASSERT(this);
	ASSERT(a5 <= 0xFFFFFF);
	ASSERT(windrv);
	ASSERT(windrv->isValidUnit(unit));
	ASSERT(windrv->GetFilesystem());

	LockXM();

	// 検索対象ファイル名獲得
	Human68k::namests_t ns;
	GetNameSts(GetAddr(a5 + 14), &ns);

	UnlockXM();

#ifdef WINDRV_LOG
	Log(Log::Normal, "$43 %d ディレクトリ削除", unit);
#endif // WINDRV_LOG

	// ファイルシステム呼び出し
	DWORD nResult = windrv->GetFilesystem()->RemoveDir(this, &ns);

	LockXM();

	// 終了値
	SetResult(nResult);

	UnlockXM();
}

//---------------------------------------------------------------------------
//
//	$44 - ファイル名変更
//
//	in
//		14	1.L	NAMESTS構造体アドレス 旧ファイル名
//		18	1.L	NAMESTS構造体アドレス 新ファイル名
//
//---------------------------------------------------------------------------
void FASTCALL CWindrv::Rename()
{
	ASSERT(this);
	ASSERT(a5 <= 0xFFFFFF);
	ASSERT(windrv);
	ASSERT(windrv->isValidUnit(unit));
	ASSERT(windrv->GetFilesystem());

	LockXM();

	// 検索対象ファイル名獲得
	Human68k::namests_t ns;
	GetNameSts(GetAddr(a5 + 14), &ns);
	Human68k::namests_t ns_new;
	GetNameSts(GetAddr(a5 + 18), &ns_new);

	UnlockXM();

#ifdef WINDRV_LOG
	Log(Log::Normal, "$44 %d ファイル名変更", unit);
#endif // WINDRV_LOG

	// ファイルシステム呼び出し
	DWORD nResult = windrv->GetFilesystem()->Rename(this, &ns, &ns_new);

	LockXM();

	// 終了値
	SetResult(nResult);

	UnlockXM();
}

//---------------------------------------------------------------------------
//
//	$45 - ファイル削除
//
//	in
//		14	1.L	NAMESTS構造体アドレス
//
//---------------------------------------------------------------------------
void FASTCALL CWindrv::Delete()
{
	ASSERT(this);
	ASSERT(a5 <= 0xFFFFFF);
	ASSERT(windrv);
	ASSERT(windrv->isValidUnit(unit));
	ASSERT(windrv->GetFilesystem());

	LockXM();

	// 検索対象ファイル名獲得
	Human68k::namests_t ns;
	GetNameSts(GetAddr(a5 + 14), &ns);

	UnlockXM();

#ifdef WINDRV_LOG
	Log(Log::Normal, "$45 %d ファイル削除", unit);
#endif // WINDRV_LOG

	// ファイルシステム呼び出し
	DWORD nResult = windrv->GetFilesystem()->Delete(this, &ns);

	LockXM();

	// 終了値
	SetResult(nResult);

	UnlockXM();
}

//---------------------------------------------------------------------------
//
//	$46 - ファイル属性取得/設定
//
//	in
//		12	1.B	読み出し時に0x01になるので注意
//		13	1.B	属性 $FFだと読み出し
//		14	1.L	NAMESTS構造体アドレス
//
//---------------------------------------------------------------------------
void FASTCALL CWindrv::Attribute()
{
	ASSERT(this);
	ASSERT(a5 <= 0xFFFFFF);
	ASSERT(windrv);
	ASSERT(windrv->isValidUnit(unit));
	ASSERT(windrv->GetFilesystem());

	LockXM();

	// 検索対象ファイル名獲得
	Human68k::namests_t ns;
	GetNameSts(GetAddr(a5 + 14), &ns);

	// 対象属性
	DWORD attr = GetByte(a5 + 13);

	UnlockXM();

#ifdef WINDRV_LOG
	Log(Log::Normal, "$46 %d ファイル属性取得/設定 属性:%02X", unit, attr);
#endif // WINDRV_LOG

	// ファイルシステム呼び出し
	DWORD nResult = windrv->GetFilesystem()->Attribute(this, &ns, attr);

	LockXM();

	// 終了値
	SetResult(nResult);

	UnlockXM();
}

//---------------------------------------------------------------------------
//
//	$47 - ファイル検索(First)
//
//	in	(offset	size)
//		 0	1.b	定数(26)
//		 1	1.b	ユニット番号
//		 2	1.b	コマンド($47/$c7)
//		13	1.b	検索属性
//		14	1.l	ファイル名バッファ(namests 形式)
//		18	1.l	検索バッファ(files 形式) このバッファに検索途中情報と検索結果を書き込む
//	out	(offset	size)
//		 3	1.b	エラーコード(下位)
//		 4	1.b	〃	    (上位)
//		18	1.l	リザルトステータス
//
//	　ディレクトリから指定ファイルを検索する. DOS _FILES から呼び出される.
//	　検索に失敗した場合、若しくは検索に成功してもワイルドカードが使われて
//	いない場合は、次回検索時に必ず失敗させる為に検索バッファのオフセットに
//	-1 を書き込む. 検索が成功した場合は見つかったファイルの情報を設定する
//	と共に、次検索用の情報のセクタ番号、オフセット、ルートディレクトリの場
//	合は更に残りセクタ数を設定する. 検索ドライブ・属性、パス名は DOS コー
//	ル処理内で設定されるので書き込む必要はない.
//
//	<NAMESTS構造体>
//	offset	size
//	0	1.b	NAMWLD	0:ワイルドカードなし -1:ファイル指定なし
//				(ワイルドカードの文字数)
//	1	1.b	NAMDRV	ドライブ番号(A=0,B=1,…,Z=25)
//	2	65.b	NAMPTH	パス('\'＋あればサブディレクトリ名＋'\')
//	67	8.b	NAMNM1	ファイル名(先頭 8 文字)
//	75	3.b	NAMEXT	拡張子
//	78	10.b	NAMNM2	ファイル名(残りの 10 文字)
//
// パス区切り文字は0x2F(/)や0x5C(\)ではなく0x09(TAB)を使っているので注意
//
//---------------------------------------------------------------------------
void FASTCALL CWindrv::Files()
{
	ASSERT(this);
	ASSERT(a5 <= 0xFFFFFF);
	ASSERT(windrv);
	ASSERT(windrv->isValidUnit(unit));
	ASSERT(windrv->GetFilesystem());

	LockXM();

	// 検索途中経過格納領域
	DWORD nFiles = GetAddr(a5 + 18);
	Human68k::files_t info;
	GetFiles(nFiles, &info);
	//info.fatr =  (BYTE)GetByte(a5 + 13);	// 検索属性保存(既に書き込まれている)

	// 検索対象ファイル名獲得
	Human68k::namests_t ns;
	GetNameSts(GetAddr(a5 + 14), &ns);

	UnlockXM();

#ifdef WINDRV_LOG
	Log(Log::Normal, "$47 %d ファイル検索(First) Info:%X 属性:%02X", unit, nFiles, info.fatr);
#endif // WINDRV_LOG

	// ファイルシステム呼び出し
	int nResult = windrv->GetFilesystem()->Files(this, &ns, nFiles, &info);

	LockXM();

	// 検索結果の反映
	if (nResult >= 0) {
		SetFiles(nFiles, &info);
	}

	// 終了値
	SetResult(nResult);

	UnlockXM();
}

//---------------------------------------------------------------------------
//
//	$48 - ファイル検索(Next)
//
//	in
//		18	1.L	FILES構造体アドレス
//
//---------------------------------------------------------------------------
void FASTCALL CWindrv::NFiles()
{
	ASSERT(this);
	ASSERT(a5 <= 0xFFFFFF);
	ASSERT(windrv);
	ASSERT(windrv->GetFilesystem());

	LockXM();

	// ワーク領域の読み込み
	DWORD nFiles = GetAddr(a5 + 18);
	Human68k::files_t info;
	GetFiles(nFiles, &info);

	UnlockXM();

#ifdef WINDRV_LOG
	Log(Log::Normal, "$48 - ファイル検索(Next) Info:%X", nFiles);
#endif // WINDRV_LOG

	// ファイルシステム呼び出し
	int nResult = windrv->GetFilesystem()->NFiles(this, nFiles, &info);

	LockXM();

	// 検索結果の反映
	if (nResult >= 0) {
		SetFiles(nFiles, &info);
	}

	// 終了値
	SetResult(nResult);

	UnlockXM();
}

//---------------------------------------------------------------------------
//
//	$49 - ファイル作成(Create)
//
//	in
//		 1	1.B	ユニット番号
//		13	1.B	属性
//		14	1.L	NAMESTS構造体アドレス
//		18	1.L	モード (0:_NEWFILE 1:_CREATE)
//		22	1.L	FCB構造体アドレス
//
//---------------------------------------------------------------------------
void FASTCALL CWindrv::Create()
{
	ASSERT(this);
	ASSERT(a5 <= 0xFFFFFF);
	ASSERT(windrv);
	ASSERT(windrv->isValidUnit(unit));
	ASSERT(windrv->GetFilesystem());

	LockXM();

	// 対象ファイル名獲得
	Human68k::namests_t ns;
	GetNameSts(GetAddr(a5 + 14), &ns);

	// FCB獲得
	DWORD nFcb = GetAddr(a5 + 22);	// FCBアドレス保存
	Human68k::fcb_t fcb;
	GetFcb(nFcb, &fcb);

	// 属性
	DWORD attr = GetByte(a5 + 13);

	// 強制上書きモード
	BOOL force = (BOOL)GetLong(a5 + 18);

	UnlockXM();

#ifdef WINDRV_LOG
	Log(Log::Normal, "$49 %d ファイル作成 FCB:%X 属性:%02X Mode:%d", unit, nFcb, attr, force);
#endif // WINDRV_LOG

	// ファイルシステム呼び出し
	int nResult = windrv->GetFilesystem()->Create(this, &ns, nFcb, &fcb, attr, force);

	LockXM();

	// 結果の反映
	if (nResult >= 0) {
		SetFcb(nFcb, &fcb);
	}

	// 終了値
	SetResult(nResult);

	UnlockXM();
}

//---------------------------------------------------------------------------
//
//	$4A - ファイルオープン
//
//	in
//		 1	1.B	ユニット番号
//		14	1.L	NAMESTS構造体アドレス
//		22	1.L	FCB構造体アドレス
//				既にFCBにはほとんどのパラメータが設定済み
//				時刻・日付はオープンした瞬間のものになってるので上書き
//				サイズは0になっているので上書き
//
//---------------------------------------------------------------------------
void FASTCALL CWindrv::Open()
{
	ASSERT(this);
	ASSERT(a5 <= 0xFFFFFF);
	ASSERT(windrv);
	ASSERT(windrv->isValidUnit(unit));
	ASSERT(windrv->GetFilesystem());

	LockXM();

	// 対象ファイル名獲得
	Human68k::namests_t ns;
	GetNameSts(GetAddr(a5 + 14), &ns);

	// FCB獲得
	DWORD nFcb = GetAddr(a5 + 22);	// FCBアドレス保存
	Human68k::fcb_t fcb;
	GetFcb(nFcb, &fcb);

	UnlockXM();

#ifdef WINDRV_LOG
	Log(Log::Normal, "$4A %d ファイルオープン FCB:%X Mode:%X", unit, nFcb, fcb.mode);
#endif // WINDRV_LOG

	// ファイルシステム呼び出し
	int nResult = windrv->GetFilesystem()->Open(this, &ns, nFcb, &fcb);

	LockXM();

	// 結果の反映
	if (nResult >= 0) {
		SetFcb(nFcb, &fcb);
	}

	// 終了値
	SetResult(nResult);

	UnlockXM();
}

//---------------------------------------------------------------------------
//
//	$4B - ファイルクローズ
//
//	in
//		22	1.L	FCB構造体アドレス
//
//---------------------------------------------------------------------------
void FASTCALL CWindrv::Close()
{
	ASSERT(this);
	ASSERT(a5 <= 0xFFFFFF);
	ASSERT(windrv);
	ASSERT(windrv->GetFilesystem());

	LockXM();

	// FCB獲得
	DWORD nFcb = GetAddr(a5 + 22);	// FCBアドレス保存
	Human68k::fcb_t fcb;
	GetFcb(nFcb, &fcb);

	UnlockXM();

#ifdef WINDRV_LOG
	Log(Log::Normal, "$4B - ファイルクローズ FCB:%X", nFcb);
#endif // WINDRV_LOG

	// ファイルシステム呼び出し
	DWORD nResult = windrv->GetFilesystem()->Close(this, nFcb, &fcb);

	LockXM();

	// 終了値
	SetResult(nResult);

	UnlockXM();
}

//---------------------------------------------------------------------------
//
//	$4C - ファイル読み込み
//
//	in
//		14	1.L	読み込みバッファ ここにファイル内容を読み込む
//		18	1.L	サイズ 負の数ならファイルサイズを指定したのと同じ
//		22	1.L	FCB構造体アドレス
//
//---------------------------------------------------------------------------
void FASTCALL CWindrv::Read()
{
	ASSERT(this);
	ASSERT(a5 <= 0xFFFFFF);
	ASSERT(windrv);
	ASSERT(windrv->GetFilesystem());

	LockXM();

	// FCB獲得
	DWORD nFcb = GetAddr(a5 + 22);	// FCBアドレス保存
	Human68k::fcb_t fcb;
	GetFcb(nFcb, &fcb);

	// 読み込みバッファ
	DWORD nAddress = GetAddr(a5 + 14);

	// 読み込みサイズ
	DWORD nSize = GetLong(a5 + 18);

	UnlockXM();

#ifdef WINDRV_LOG
	Log(Log::Normal, "$4C - ファイル読み込み FCB:%X アドレス:%X サイズ:%d", nFcb, nAddress, nSize);
#endif // WINDRV_LOG

	// ファイルシステム呼び出し
	int nResult = windrv->GetFilesystem()->Read(this, nFcb, &fcb, nAddress, nSize);

	LockXM();

	// 結果の反映
	if (nResult >= 0) {
		SetFcb(nFcb, &fcb);
	}

	// 終了値
	SetResult(nResult);

	UnlockXM();
}

//---------------------------------------------------------------------------
//
//	$4D - ファイル書き込み
//
//	in
//		14	1.L	書き込みバッファ ここにファイル内容を書き込む
//		18	1.L	サイズ 負の数ならファイルサイズを指定したのと同じ
//		22	1.L	FCB構造体アドレス
//
//---------------------------------------------------------------------------
void FASTCALL CWindrv::Write()
{
	ASSERT(this);
	ASSERT(a5 <= 0xFFFFFF);
	ASSERT(windrv);
	ASSERT(windrv->GetFilesystem());

	LockXM();

	// FCB獲得
	DWORD nFcb = GetAddr(a5 + 22);	// FCBアドレス保存
	Human68k::fcb_t fcb;
	GetFcb(nFcb, &fcb);

	// 書き込みバッファ
	DWORD nAddress = GetAddr(a5 + 14);

	// 書き込みサイズ
	DWORD nSize = GetLong(a5 + 18);

	UnlockXM();

#ifdef WINDRV_LOG
	Log(Log::Normal, "$4D - ファイル書き込み FCB:%X アドレス:%X サイズ:%d", nFcb, nAddress, nSize);
#endif // WINDRV_LOG

	// ファイルシステム呼び出し
	int nResult = windrv->GetFilesystem()->Write(this, nFcb, &fcb, nAddress, nSize);

	LockXM();

	// 結果の反映
	if (nResult >= 0) {
		SetFcb(nFcb, &fcb);
	}

	// 終了値
	SetResult(nResult);

	UnlockXM();
}

//---------------------------------------------------------------------------
//
//	$4E - ファイルシーク
//
//	in
//		12	1.B	0x2B になってるときがある 0のときもある
//		13	1.B	モード
//		18	1.L	オフセット
//		22	1.L	FCB構造体アドレス
//
//---------------------------------------------------------------------------
void FASTCALL CWindrv::Seek()
{
	ASSERT(this);
	ASSERT(a5 <= 0xFFFFFF);
	ASSERT(windrv);
	ASSERT(windrv->GetFilesystem());

	LockXM();

	// FCB獲得
	DWORD nFcb = GetAddr(a5 + 22);	// FCBアドレス保存
	Human68k::fcb_t fcb;
	GetFcb(nFcb, &fcb);

	// シークモード
	DWORD nMode = GetByte(a5 + 13);

	// シークオフセット
	DWORD nOffset = GetLong(a5 + 18);

	UnlockXM();

#ifdef WINDRV_LOG
	Log(Log::Normal, "$4E - ファイルシーク FCB:%X モード:%d オフセット:%d", nFcb, nMode, nOffset);
#endif // WINDRV_LOG

	// ファイルシステム呼び出し
	int nResult = windrv->GetFilesystem()->Seek(this, nFcb, &fcb, nMode, nOffset);

	LockXM();

	// 結果の反映
	if (nResult >= 0) {
		SetFcb(nFcb, &fcb);
	}

	// 終了値
	SetResult(nResult);

	UnlockXM();
}

//---------------------------------------------------------------------------
//
//	$4F - ファイル時刻取得/設定
//
//	in
//		18	1.W	DATE
//		20	1.W	TIME
//		22	1.L	FCB構造体アドレス
//
//---------------------------------------------------------------------------
void FASTCALL CWindrv::TimeStamp()
{
	ASSERT(this);
	ASSERT(a5 <= 0xFFFFFF);
	ASSERT(windrv);
	ASSERT(windrv->GetFilesystem());

	LockXM();

	// FCB獲得
	DWORD nFcb = GetAddr(a5 + 22);	// FCBアドレス保存
	Human68k::fcb_t fcb;
	GetFcb(nFcb, &fcb);

	// 時刻獲得
	WORD nFatDate = (WORD)GetWord(a5 + 18);
	WORD nFatTime = (WORD)GetWord(a5 + 20);

	UnlockXM();

#ifdef WINDRV_LOG
	Log(Log::Normal, "$4F - ファイル時刻取得/設定 FCB:%X 時刻:%04X%04X", nFcb, nFatDate, nFatTime);
#endif // WINDRV_LOG

	// ファイルシステム呼び出し
	DWORD nResult = windrv->GetFilesystem()->TimeStamp(this, nFcb, &fcb, nFatDate, nFatTime);

	LockXM();

	// 結果の反映
	if ((nResult >> 16) != 0xFFFF) {
		SetFcb(nFcb, &fcb);
	}

	// 終了値
	SetResult(nResult);

	UnlockXM();
}

//---------------------------------------------------------------------------
//
//	$50 - 容量取得
//
//	in	(offset	size)
//		 0	1.b	定数(26)
//		 1	1.b	ユニット番号
//		 2	1.b	コマンド($50/$d0)
//		14	1.l	バッファアドレス
//	out	(offset	size)
//		 3	1.b	エラーコード(下位)
//		 4	1.b	〃	    (上位)
//		18	1.l	リザルトステータス
//
//	　メディアの総容量/空き容量、クラスタ/セクタサイズを収得する. バッファ
//	に書き込む内容は以下の通り. リザルトステータスとして使用可能なバイト数
//	を返すこと.
//
//	offset	size
//	0	1.w	使用可能なクラスタ数
//	2	1.w	総クラスタ数
//	4	1.w	1 クラスタ当りのセクタ数
//	6	1.w	1 セクタ当りのバイト数
//
//---------------------------------------------------------------------------
void FASTCALL CWindrv::GetCapacity()
{
	ASSERT(this);
	ASSERT(a5 <= 0xFFFFFF);
	ASSERT(windrv);
	ASSERT(windrv->isValidUnit(unit));
	ASSERT(windrv->GetFilesystem());

#ifdef WINDRV_LOG
	Log(Log::Normal, "$50 %d 容量取得", unit);
#endif // WINDRV_LOG

	LockXM();

	// バッファ取得
	DWORD nCapacity = GetAddr(a5 + 14);
	Human68k::capacity_t cap;
	//GetCapacity(nCapacity, &cap);	// 取得は不要 初期化も不要 出前迅速 落書き無用

	UnlockXM();

	// ファイルシステム呼び出し
	int nResult = windrv->GetFilesystem()->GetCapacity(this, &cap);

	LockXM();

	// 結果の反映
	if (nResult >= 0) {
		SetCapacity(nCapacity, &cap);
	}

	// 終了値
	SetResult(nResult);

	UnlockXM();

#ifdef WINDRV_LOG
	Log(Log::Detail, "ユニット%d 空き容量 %d", unit, nResult);
#endif // WINDRV_LOG
}

//---------------------------------------------------------------------------
//
//	$51 - ドライブ状態検査/制御
//
//	in	(offset	size)
//		 1	1.B	ユニット番号
//		13	1.B	状態	0: 状態検査 1: イジェクト
//
//---------------------------------------------------------------------------
void FASTCALL CWindrv::CtrlDrive()
{
	ASSERT(this);
	ASSERT(a5 <= 0xFFFFFF);
	ASSERT(windrv);
	ASSERT(windrv->isValidUnit(unit));
	ASSERT(windrv->GetFilesystem());

	LockXM();

	// ドライブ状態取得
	Human68k::ctrldrive_t ctrl;
	ctrl.status = (BYTE)GetByte(a5 + 13);

	UnlockXM();

#ifdef WINDRV_LOG
	Log(Log::Normal, "$51 %d ドライブ状態検査/制御 Mode:%d", unit, ctrl.status);
#endif // WINDRV_LOG

	// ファイルシステム呼び出し
	int nResult = windrv->GetFilesystem()->CtrlDrive(this, &ctrl);

	LockXM();

	// 結果の反映
	if (nResult >= 0) {
		SetByte(a5 + 13, ctrl.status);
	}

	// 終了値
	SetResult(nResult);

	UnlockXM();
}

//---------------------------------------------------------------------------
//
//	$52 - DPB取得
//
//	in	(offset	size)
//		 0	1.b	定数(26)
//		 1	1.b	ユニット番号
//		 2	1.b	コマンド($52/$d2)
//		14	1.l	バッファアドレス(先頭アドレス + 2 を指す)
//	out	(offset	size)
//		 3	1.b	エラーコード(下位)
//		 4	1.b	〃	    (上位)
//		18	1.l	リザルトステータス
//
//	　指定メディアの情報を v1 形式 DPB で返す. このコマンドで設定する必要
//	がある情報は以下の通り(括弧内は DOS コールが設定する). ただし、バッフ
//	ァアドレスはオフセット 2 を指したアドレスが渡されるので注意すること.
//
//	offset	size
//	 0	1.b	(ドライブ番号)
//	 1	1.b	(ユニット番号)
//	 2	1.w	1 セクタ当りのバイト数
//	 4	1.b	1 クラスタ当りのセクタ数 - 1
//	 5	1.b	クラスタ→セクタのシフト数
//			bit 7 = 1 で MS-DOS 形式 FAT(16bit Intel 配列)
//	 6	1.w	FAT の先頭セクタ番号
//	 8	1.b	FAT 領域の個数
//	 9	1.b	FAT の占めるセクタ数(複写分を除く)
//	10	1.w	ルートディレクトリに入るファイルの個数
//	12	1.w	データ領域の先頭セクタ番号
//	14	1.w	総クラスタ数 + 1
//	16	1.w	ルートディレクトリの先頭セクタ番号
//	18	1.l	(ドライバヘッダのアドレス)
//	22	1.b	(小文字の物理ドライブ名)
//	23	1.b	(DPB 使用フラグ:常に 0)
//	24	1.l	(次の DPB のアドレス)
//	28	1.w	(カレントディレクトリのクラスタ番号:常に 0)
//	30	64.b	(カレントディレクトリ名)
//
//---------------------------------------------------------------------------
void FASTCALL CWindrv::GetDPB()
{
	ASSERT(this);
	ASSERT(a5 <= 0xFFFFFF);
	ASSERT(windrv);
	ASSERT(windrv->isValidUnit(unit));
	ASSERT(windrv->GetFilesystem());

	LockXM();

	// DPB取得
	DWORD nDpb = GetAddr(a5 + 14);
	Human68k::dpb_t dpb;
	//GetDpb(nDpb, &dpb);	// 取得は不要 初期化も不要 出前迅速 落書き無用

	UnlockXM();

#ifdef WINDRV_LOG
	Log(Log::Normal, "$52 %d DPB取得 DPB:%X", unit, nDpb);
#endif // WINDRV_LOG

	// ファイルシステム呼び出し
	int nResult = windrv->GetFilesystem()->GetDPB(this, &dpb);

	LockXM();

	// 結果の反映
	if (nResult >= 0) {
		SetDpb(nDpb, &dpb);
	}

	// 終了値
	SetResult(nResult);

	UnlockXM();
}

//---------------------------------------------------------------------------
//
//	$53 - セクタ読み込み
//
//	in	(offset	size)
//		 1	1.B	ユニット番号
//		14	1.L	バッファアドレス
//		18	1.L	セクタ数
//		22	1.L	セクタ番号
//
//---------------------------------------------------------------------------
void FASTCALL CWindrv::DiskRead()
{
	ASSERT(this);
	ASSERT(a5 <= 0xFFFFFF);
	ASSERT(windrv);
	ASSERT(windrv->isValidUnit(unit));
	ASSERT(windrv->GetFilesystem());

	LockXM();

	DWORD nAddress = GetAddr(a5 + 14);	// アドレス(上位ビットが拡張フラグ)
	DWORD nSize = GetLong(a5 + 18);		// セクタ数
	DWORD nSector = GetLong(a5 + 22);	// セクタ番号

	UnlockXM();

#ifdef WINDRV_LOG
	Log(Log::Normal, "$53 %d セクタ読み込み アドレス:%X セクタ:%X 個数:%X", unit, nAddress, nSector, nSize);
#endif // WINDRV_LOG

	// ファイルシステム呼び出し
	DWORD nResult = windrv->GetFilesystem()->DiskRead(this, nAddress, nSector, nSize);

	LockXM();

	// 終了値
	SetResult(nResult);

	UnlockXM();
}

//---------------------------------------------------------------------------
//
//	$54 - セクタ書き込み
//
//---------------------------------------------------------------------------
void FASTCALL CWindrv::DiskWrite()
{
	ASSERT(this);
	ASSERT(a5 <= 0xFFFFFF);
	ASSERT(windrv);
	ASSERT(windrv->isValidUnit(unit));
	ASSERT(windrv->GetFilesystem());

	LockXM();

	DWORD nAddress = GetAddr(a5 + 14);	// アドレス(上位ビットが拡張フラグ)
	DWORD nSize = GetLong(a5 + 18);		// セクタ数
	DWORD nSector = GetLong(a5 + 22);	// セクタ番号

	UnlockXM();

#ifdef WINDRV_LOG
	Log(Log::Normal, "$53 %d セクタ書き込み アドレス:%X セクタ:%X 個数:%X", unit, nAddress, nSector, nSize);
#endif // WINDRV_LOG

	// ファイルシステム呼び出し
	DWORD nResult = windrv->GetFilesystem()->DiskWrite(this, nAddress, nSector, nSize);

	LockXM();

	// 終了値
	SetResult(nResult);

	UnlockXM();
}

//---------------------------------------------------------------------------
//
//	$55 - IOCTRL
//
//---------------------------------------------------------------------------
void FASTCALL CWindrv::IoControl()
{
	ASSERT(this);
	ASSERT(a5 <= 0xFFFFFF);
	ASSERT(windrv);
	ASSERT(windrv->isValidUnit(unit));
	ASSERT(windrv->GetFilesystem());

	LockXM();

	// IOCTRL取得
	DWORD nParameter = GetLong(a5 + 14);	// パラメータ
	DWORD nFunction = GetWord(a5 + 18);		// 機能番号
	Human68k::ioctrl_t ioctrl;
	GetIoctrl(nParameter, nFunction, &ioctrl);

	UnlockXM();

#ifdef WINDRV_LOG
	Log(Log::Normal, "$55 %d IOCTRL %X %X", unit, nParameter, nFunction);
#endif // WINDRV_LOG

	// ファイルシステム呼び出し
	int nResult = windrv->GetFilesystem()->IoControl(this, &ioctrl, nFunction);

	LockXM();

	// 結果の反映
	if (nResult >= 0) {
		SetIoctrl(nParameter, nFunction, &ioctrl);
	}

	// 終了値
	SetResult(nResult);

	UnlockXM();
}

//---------------------------------------------------------------------------
//
//	$56 - フラッシュ
//
//---------------------------------------------------------------------------
void FASTCALL CWindrv::Flush()
{
	ASSERT(this);
	ASSERT(a5 <= 0xFFFFFF);
	ASSERT(windrv);
	ASSERT(windrv->isValidUnit(unit));
	ASSERT(windrv->GetFilesystem());

#ifdef WINDRV_LOG
	Log(Log::Normal, "$56 %d フラッシュ", unit);
#endif // WINDRV_LOG

	// ファイルシステム呼び出し
	DWORD nResult = windrv->GetFilesystem()->Flush(this);

	LockXM();

	// 終了値
	SetResult(nResult);

	UnlockXM();
}

//---------------------------------------------------------------------------
//
//	$57 - メディア交換チェック
//
//	in	(offset	size)
//		 0	1.b	定数(26)
//		 1	1.b	ユニット番号
//		 2	1.b	コマンド($57/$d7)
//	out	(offset	size)
//		 3	1.b	エラーコード(下位)
//		 4	1.b	〃	    (上位)
//		18	1.l	リザルトステータス
//
//	　メディアが交換されたか否かを調べる. 交換されていた場合のフォーマット
//	確認はこのコマンド内で行うこと.
//
//---------------------------------------------------------------------------
void FASTCALL CWindrv::CheckMedia()
{
	ASSERT(this);
	ASSERT(a5 <= 0xFFFFFF);
	ASSERT(windrv);
	ASSERT(windrv->isValidUnit(unit));
	ASSERT(windrv->GetFilesystem());

#ifdef WINDRV_LOG
	Log(Log::Normal, "$57 %d メディア交換チェック", unit);
#endif // WINDRV_LOG

	// ファイルシステム呼び出し
	DWORD nResult = windrv->GetFilesystem()->CheckMedia(this);

	LockXM();

	// 終了値
	SetResult(nResult);

	UnlockXM();
}

//---------------------------------------------------------------------------
//
//	$58 - 排他制御
//
//---------------------------------------------------------------------------
void FASTCALL CWindrv::Lock()
{
	ASSERT(this);
	ASSERT(a5 <= 0xFFFFFF);
	ASSERT(windrv);
	ASSERT(windrv->isValidUnit(unit));
	ASSERT(windrv->GetFilesystem());

#ifdef WINDRV_LOG
	Log(Log::Normal, "$58 %d 排他制御", unit);
#endif // WINDRV_LOG

	// ファイルシステム呼び出し
	DWORD nResult = windrv->GetFilesystem()->Lock(this);

	LockXM();

	// 終了値
	SetResult(nResult);

	UnlockXM();
}

//---------------------------------------------------------------------------
//
//	終了値書き込み
//
//---------------------------------------------------------------------------
void FASTCALL CWindrv::SetResult(DWORD result)
{
	ASSERT(this);
	ASSERT(a5 <= 0xFFFFFF);

	// 致命的エラー判定
	DWORD fatal = 0;
	switch (result) {
	case FS_FATAL_INVALIDUNIT: fatal = 0x5001; break;
	case FS_FATAL_INVALIDCOMMAND: fatal = 0x5003; break;
	case FS_FATAL_WRITEPROTECT: fatal = 0x700D; break;
	case FS_FATAL_MEDIAOFFLINE: fatal = 0x7002; break;
	}

	// 結果を格納
	if (fatal) {
		// リトライ可能を返すときは、(a5 + 18)を書き換えてはいけない。
		// その後白帯で Retry を選択した場合、書き換えた値を読み込んで誤動作してしまう。
		result = FS_INVALIDFUNC;
		SetByte(a5 + 3, fatal & 255);
		SetByte(a5 + 4, fatal >> 8);
		if ((fatal & 0x2000) == 0) {
			SetLong(a5 + 18, result);
		}
	} else {
		SetLong(a5 + 18, result);
	}

#if defined(WINDRV_LOG)
	// エラーがなければ終了
	if (result < 0xFFFFFF80) return;

	// エラーメッセージ
	TCHAR* szError;
	switch (result) {
	case FS_INVALIDFUNC: szError = "無効なファンクションコードを実行しました"; break;
	case FS_FILENOTFND: szError = "指定したファイルが見つかりません"; break;
	case FS_DIRNOTFND: szError = "指定したディレクトリが見つかりません"; break;
	case FS_OVEROPENED: szError = "オープンしているファイルが多すぎます"; break;
	case FS_CANTACCESS: szError = "ディレクトリやボリュームラベルはアクセス不可です"; break;
	case FS_NOTOPENED: szError = "指定したハンドルはオープンされていません"; break;
	case FS_INVALIDMEM: szError = "メモリ管理領域が破壊されました"; break;
	case FS_OUTOFMEM: szError = "実行に必要なメモリがありません"; break;
	case FS_INVALIDPTR: szError = "無効なメモリ管理ポインタを指定しました"; break;
	case FS_INVALIDENV: szError = "不正な環境を指定しました"; break;
	case FS_ILLEGALFMT: szError = "実行ファイルのフォーマットが異常です"; break;
	case FS_ILLEGALMOD: szError = "オープンのアクセスモードが異常です"; break;
	case FS_INVALIDPATH: szError = "ファイル名の指定に誤りがあります"; break;
	case FS_INVALIDPRM: szError = "無効なパラメータでコールしました"; break;
	case FS_INVALIDDRV: szError = "ドライブ指定に誤りがあります"; break;
	case FS_DELCURDIR: szError = "カレントディレクトリは削除できません"; break;
	case FS_NOTIOCTRL: szError = "IOCTRLできないデバイスです"; break;
	case FS_LASTFILE: szError = "これ以上ファイルが見つかりません"; break;
	case FS_CANTWRITE: szError = "指定のファイルは書き込みできません"; break;
	case FS_DIRALREADY: szError = "指定のディレクトリは既に登録されています"; break;
	case FS_CANTDELETE: szError = "ファイルがあるので削除できません"; break;
	case FS_CANTRENAME: szError = "ファイルがあるのでリネームできません"; break;
	case FS_DISKFULL: szError = "ディスクが一杯でファイルが作れません"; break;
	case FS_DIRFULL: szError = "ディレクトリが一杯でファイルが作れません"; break;
	case FS_CANTSEEK: szError = "指定の位置にはシークできません"; break;
	case FS_SUPERVISOR: szError = "スーパーバイザ状態でスーパバイザ指定しました"; break;
	case FS_THREADNAME: szError = "同じスレッド名が存在します"; break;
	case FS_BUFWRITE: szError = "プロセス間通信のバッファが書込み禁止です"; break;
	case FS_BACKGROUND: szError = "バックグラウンドプロセスを起動できません"; break;
	case FS_OUTOFLOCK: szError = "ロック領域が足りません"; break;
	case FS_LOCKED: szError = "ロックされていてアクセスできません"; break;
	case FS_DRIVEOPENED: szError = "指定のドライブはハンドラがオープンされています"; break;
	case FS_LINKOVER: szError = "シンボリックリンクネストが16回を超えました"; break;
	case FS_FILEEXIST: szError = "ファイルが存在します"; break;
	default: szError = "未定義のエラーです"; break;
	}
	switch (fatal & 255) {
	case 0: break;
	case 1: szError = "無効なユニット番号を指定しました"; break;
	case 2: szError = "ディスクが入っていません"; break;
	case 3: szError = "無効なコマンド番号を指定しました"; break;
	case 4: szError = "CRCエラーが発生しました"; break;
	case 5: szError = "ディスクの管理領域が破壊されています"; break;
	case 6: szError = "シークに失敗しました"; break;
	case 7: szError = "無効なメディアを使用しました"; break;
	case 8: szError = "セクタが見つかりません"; break;
	case 9: szError = "プリンタがつながっていません"; break;
	case 10: szError = "書き込みエラーが発生しました"; break;
	case 11: szError = "読み込みエラーが発生しました"; break;
	case 12: szError = "その他のエラーが発生しました"; break;
	case 13: szError = "書き込み禁止のため実行できません"; break;
	case 14: szError = "書き込みできません"; break;
	case 15: szError = "ファイル共有違反が発生しました"; break;
	default: szError = "未定義の致命的エラーです"; break;
	}

	Log(Log::Warning, "$%X %d %s(%d)", command, unit, szError, result);
#endif // WINDRV_LOG
}

//---------------------------------------------------------------------------
//
//	バイト読み込み
//
//---------------------------------------------------------------------------
DWORD FASTCALL CWindrv::GetByte(DWORD addr) const
{
	ASSERT(this);
	ASSERT(memory);
	ASSERT(addr <= 0xffffff);

	// 8bit
	return memory->ReadOnly(addr);
}

//---------------------------------------------------------------------------
//
//	バイト書き込み
//
//---------------------------------------------------------------------------
void FASTCALL CWindrv::SetByte(DWORD addr, DWORD data)
{
	ASSERT(this);
	ASSERT(memory);
	ASSERT(addr <= 0xffffff);
	ASSERT(data < 0x100);

	// 8bit
	memory->WriteByte(addr, data);
}

//---------------------------------------------------------------------------
//
//	ワード読み込み
//
//---------------------------------------------------------------------------
DWORD FASTCALL CWindrv::GetWord(DWORD addr) const
{
	DWORD data;

	ASSERT(this);
	ASSERT(memory);
	ASSERT(addr <= 0xffffff);

	// 16bit
	data = memory->ReadOnly(addr);
	data <<= 8;
	data |= memory->ReadOnly(addr + 1);

	return data;
}

//---------------------------------------------------------------------------
//
//	ワード書き込み
//
//---------------------------------------------------------------------------
void FASTCALL CWindrv::SetWord(DWORD addr, DWORD data)
{
	ASSERT(this);
	ASSERT(memory);
	ASSERT(addr <= 0xffffff);
	ASSERT(data < 0x10000);

	// 16bit
	memory->WriteWord(addr, data);
}

//---------------------------------------------------------------------------
//
//	ロング読み込み
//
//---------------------------------------------------------------------------
DWORD FASTCALL CWindrv::GetLong(DWORD addr) const
{
	DWORD data;

	ASSERT(this);
	ASSERT(memory);
	ASSERT(addr <= 0xffffff);

	// 32bit
	data = memory->ReadOnly(addr);
	data <<= 8;
	data |= memory->ReadOnly(addr + 1);
	data <<= 8;
	data |= memory->ReadOnly(addr + 2);
	data <<= 8;
	data |= memory->ReadOnly(addr + 3);

	return data;
}

//---------------------------------------------------------------------------
//
//	ロング書き込み
//
//---------------------------------------------------------------------------
void FASTCALL CWindrv::SetLong(DWORD addr, DWORD data)
{
	ASSERT(this);
	ASSERT(memory);
	ASSERT(addr <= 0xffffff);

	// 32bit
	memory->WriteWord(addr, (WORD)(data >> 16));
	memory->WriteWord(addr + 2, (WORD)data);
}

//---------------------------------------------------------------------------
//
//	アドレス読み込み
//
//---------------------------------------------------------------------------
DWORD FASTCALL CWindrv::GetAddr(DWORD addr) const
{
	DWORD data;

	ASSERT(this);
	ASSERT(memory);
	ASSERT(addr <= 0xffffff);

	// 24bit(最初の1バイトは無視)
	data = memory->ReadOnly(addr + 1);
	data <<= 8;
	data |= memory->ReadOnly(addr + 2);
	data <<= 8;
	data |= memory->ReadOnly(addr + 3);

	return data;
}

//---------------------------------------------------------------------------
//
//	アドレス書き込み
//
//---------------------------------------------------------------------------
void FASTCALL CWindrv::SetAddr(DWORD addr, DWORD data)
{
	ASSERT(this);
	ASSERT(memory);
	ASSERT(addr <= 0xffffff);
	ASSERT(data <= 0xffffff);

	// 24bit(最初の1バイトは必ず0)
	data &= 0xffffff;
	memory->WriteWord(addr, (WORD)(data >> 16));
	memory->WriteWord(addr + 2, (WORD)data);
}

//---------------------------------------------------------------------------
//
//	NAMESTSパス名読み込み
//
//---------------------------------------------------------------------------
void FASTCALL CWindrv::GetNameStsPath(DWORD addr, Human68k::namests_t *pNamests) const
{
	DWORD offset;

	ASSERT(this);
	ASSERT(pNamests);
	ASSERT(addr <= 0xFFFFFF);

	// ワイルドカード情報
	pNamests->wildcard = (BYTE)GetByte(addr + 0);

	// ドライブ番号
	pNamests->drive = (BYTE)GetByte(addr + 1);

	// パス名
	for (offset=0; offset<sizeof(pNamests->path); offset++) {
		pNamests->path[offset] = (BYTE)GetByte(addr + 2 + offset);
	}

	// ファイル名1
	memset(pNamests->name, 0x20, sizeof(pNamests->name));

	// 拡張子
	memset(pNamests->ext, 0x20, sizeof(pNamests->ext));

	// ファイル名2
	memset(pNamests->add, 0, sizeof(pNamests->add));

#if defined(WINDRV_LOG)
{
	TCHAR szPath[_MAX_PATH];
	pNamests->GetCopyPath((BYTE*)szPath);	// WARNING: Unicode時要修正

	Log(Log::Detail, "Path %s", szPath);
}
#endif // WINDRV_LOG
}

//---------------------------------------------------------------------------
//
//	NAMESTS読み込み
//
//---------------------------------------------------------------------------
void FASTCALL CWindrv::GetNameSts(DWORD addr, Human68k::namests_t *pNamests) const
{
	DWORD offset;

	ASSERT(this);
	ASSERT(pNamests);
	ASSERT(addr <= 0xFFFFFF);

	// ワイルドカード情報
	pNamests->wildcard = (BYTE)GetByte(addr + 0);

	// ドライブ番号
	pNamests->drive = (BYTE)GetByte(addr + 1);

	// パス名
	for (offset=0; offset<sizeof(pNamests->path); offset++) {
		pNamests->path[offset] = (BYTE)GetByte(addr + 2 + offset);
	}

	// ファイル名1
	for (offset=0; offset<sizeof(pNamests->name); offset++) {
		pNamests->name[offset] = (BYTE)GetByte(addr + 67 + offset);
	}

	// 拡張子
	for (offset=0; offset<sizeof(pNamests->ext); offset++) {
		pNamests->ext[offset] = (BYTE)GetByte(addr + 75 + offset);
	}

	// ファイル名2
	for (offset=0; offset<sizeof(pNamests->add); offset++) {
		pNamests->add[offset] = (BYTE)GetByte(addr + 78 + offset);
	}

#if defined(WINDRV_LOG)
{
	TCHAR szPath[_MAX_PATH];
	pNamests->GetCopyPath((BYTE*)szPath);	// WARNING: Unicode時要修正

	TCHAR szFilename[_MAX_PATH];
	pNamests->GetCopyFilename((BYTE*)szFilename);	// WARNING: Unicode時要修正

	if (pNamests->wildcard == 0xFF) {
		Log(Log::Detail, "Path %s", szPath);
	} else {
		Log(Log::Detail, "Filename %s%s", szPath, szFilename);
	}
}
#endif // WINDRV_LOG
}

//---------------------------------------------------------------------------
//
//	FILES読み込み
//
//---------------------------------------------------------------------------
void FASTCALL CWindrv::GetFiles(DWORD addr, Human68k::files_t* pFiles) const
{
	ASSERT(this);
	ASSERT(pFiles);
	ASSERT(addr <= 0xFFFFFF);

	// 検索情報
	pFiles->fatr = (BYTE)GetByte(addr + 0);		// Read only
	pFiles->drive = (BYTE)GetByte(addr + 1);	// Read only
	pFiles->sector = GetLong(addr + 2);
	//pFiles->sector2 = (WORD)GetWord(addr + 6);	// Read only
	pFiles->offset = (WORD)GetWord(addr + 8);

#if 0
	DWORD offset;

	// ファイル名
	for (offset=0; offset<sizeof(pFiles->name); offset++) {
		pFiles->name[offset] = (BYTE)GetByte(addr + 10 + offset);
	}

	// 拡張子
	for (offset=0; offset<sizeof(pFiles->ext); offset++) {
		pFiles->ext[offset] = (BYTE)GetByte(addr + 18 + offset);
	}
#endif

#if 0
	// ファイル情報
	pFiles->attr = (BYTE)GetByte(addr + 21);
	pFiles->time = (WORD)GetWord(addr + 22);
	pFiles->date = (WORD)GetWord(addr + 24);
	pFiles->size = GetLong(addr + 26);

	// フルファイル名
	for (offset=0; offset<sizeof(pFiles->full); offset++) {
		pFiles->full[offset] = (BYTE)GetByte(addr + 30 + offset);
	}
#else
	pFiles->attr = 0;
	pFiles->time = 0;
	pFiles->date = 0;
	pFiles->size = 0;
	memset(pFiles->full, 0, sizeof(pFiles->full));
#endif
}

//---------------------------------------------------------------------------
//
//	FILES書き込み
//
//---------------------------------------------------------------------------
void FASTCALL CWindrv::SetFiles(DWORD addr, const Human68k::files_t* pFiles)
{
	DWORD offset;

	ASSERT(this);
	ASSERT(pFiles);
	ASSERT(addr <= 0xFFFFFF);

	SetLong(addr + 2, pFiles->sector);
	SetWord(addr + 8, pFiles->offset);

#if 0
	// 検索情報
	SetByte(addr + 0, pFiles->fatr);	// Read only
	SetByte(addr + 1, pFiles->drive);	// Read only
	SetWord(addr + 6, pFiles->sector2);	// Read only

	// ファイル名
	for (offset=0; offset<sizeof(pFiles->name); offset++) {
		SetByte(addr + 10 + offset, pFiles->name[offset]);
	}

	// 拡張子
	for (offset=0; offset<sizeof(pFiles->ext); offset++) {
		SetByte(addr + 18 + offset, pFiles->ext[offset]);
	}
#endif

	// ファイル情報
	SetByte(addr + 21, pFiles->attr);
	SetWord(addr + 22, pFiles->time);
	SetWord(addr + 24, pFiles->date);
	SetLong(addr + 26, pFiles->size);

	// フルファイル名
	for (offset=0; offset<sizeof(pFiles->full); offset++) {
		SetByte(addr + 30 + offset, pFiles->full[offset]);
	}

#if defined(WINDRV_LOG)
{
	TCHAR szAttribute[16];
	BYTE nAttribute = pFiles->attr;
	for (int i = 0; i < 8; i++) {
		TCHAR c = '-';
		if ((nAttribute & 0x80) != 0)
			c = "XLADVSHR"[i];
		szAttribute[i] = c;
		nAttribute <<= 1;
	}
	szAttribute[8] = '\0';

	Log(Log::Detail, "%s %s %d", szAttribute, pFiles->full, pFiles->size);
}
#endif // WINDRV_LOG
}

//---------------------------------------------------------------------------
//
//	FCB読み込み
//
//---------------------------------------------------------------------------
void FASTCALL CWindrv::GetFcb(DWORD addr, Human68k::fcb_t* pFcb) const
{
	ASSERT(this);
	ASSERT(pFcb);
	ASSERT(addr <= 0xFFFFFF);

	// FCB情報
	pFcb->fileptr = GetLong(addr + 6);
	pFcb->mode = (WORD)GetWord(addr + 14);
	//pFcb->zero = GetLong(addr + 32);

#if 0
	DWORD i;

	// ファイル名
	for (i = 0; i < sizeof(pFcb->name); i++) {
		pFcb->name[i] = (BYTE)GetByte(addr + 36 + i);
	}

	// 拡張子
	for (i = 0; i < sizeof(pFcb->ext); i++) {
		pFcb->ext[i] = (BYTE)GetByte(addr + 44 + i);
	}

	// ファイル名2
	for (i = 0; i < sizeof(pFcb->add); i++) {
		pFcb->add[i] = (BYTE)GetByte(addr + 48 + i);
	}
#endif

	// 属性
	pFcb->attr = (BYTE)GetByte(addr + 47);

	// FCB情報
	pFcb->time = (WORD)GetWord(addr + 58);
	pFcb->date = (WORD)GetWord(addr + 60);
	//pFcb->cluster = (WORD)GetWord(addr + 62);
	pFcb->size = GetLong(addr + 64);
}

//---------------------------------------------------------------------------
//
//	FCB書き込み
//
//---------------------------------------------------------------------------
void FASTCALL CWindrv::SetFcb(DWORD addr, const Human68k::fcb_t* pFcb)
{
	ASSERT(this);
	ASSERT(pFcb);
	ASSERT(addr <= 0xFFFFFF);

	// FCB情報
	SetLong(addr + 6, pFcb->fileptr);
	SetWord(addr + 14, pFcb->mode);
	// SetLong(addr + 32, pFcb->zero);

#if 0
	DWORD i;

	// ファイル名
	for (i = 0; i < sizeof(pFcb->name); i++) {
		SetByte(addr + 36 + i, pFcb->name[i]);
	}

	// 拡張子
	for (i = 0; i < sizeof(pFcb->ext); i++) {
		SetByte(addr + 44 + i, pFcb->ext[i]);
	}

	// ファイル名2
	for (i = 0; i < sizeof(pFcb->add); i++) {
		SetByte(addr + 48 + i, pFcb->add[i]);
	}
#endif

	// 属性
	SetByte(addr + 47, pFcb->attr);

	// FCB情報
	SetWord(addr + 58, pFcb->time);
	SetWord(addr + 60, pFcb->date);
	//SetWord(addr + 62, pFcb->cluster);
	SetLong(addr + 64, pFcb->size);
}

//---------------------------------------------------------------------------
//
//	CAPACITY書き込み
//
//---------------------------------------------------------------------------
void FASTCALL CWindrv::SetCapacity(DWORD addr, const Human68k::capacity_t* pCapacity)
{
	ASSERT(this);
	ASSERT(pCapacity);
	ASSERT(addr <= 0xffffff);

#ifdef WINDRV_LOG
	Log(Log::Detail, "Free:%d Clusters:%d Sectors:%d Bytes:%d",
		pCapacity->free, pCapacity->clusters, pCapacity->sectors, pCapacity->bytes);
#endif // WINDRV_LOG

	SetWord(addr + 0, pCapacity->free);
	SetWord(addr + 2, pCapacity->clusters);
	SetWord(addr + 4, pCapacity->sectors);
	SetWord(addr + 6, pCapacity->bytes);
}

//---------------------------------------------------------------------------
//
//	DPB書き込み
//
//---------------------------------------------------------------------------
void FASTCALL CWindrv::SetDpb(DWORD addr, const Human68k::dpb_t* pDpb)
{
	ASSERT(this);
	ASSERT(pDpb);
	ASSERT(addr <= 0xFFFFFF);

	// DPB情報
	SetWord(addr + 0, pDpb->sector_size);
	SetByte(addr + 2, pDpb->cluster_size);
	SetByte(addr + 3, pDpb->shift);
	SetWord(addr + 4, pDpb->fat_sector);
	SetByte(addr + 6, pDpb->fat_max);
	SetByte(addr + 7, pDpb->fat_size);
	SetWord(addr + 8, pDpb->file_max);
	SetWord(addr + 10, pDpb->data_sector);
	SetWord(addr + 12, pDpb->cluster_max);
	SetWord(addr + 14, pDpb->root_sector);
	SetByte(addr + 20, pDpb->media);
}

//---------------------------------------------------------------------------
//
//	IOCTRL読み込み
//
//---------------------------------------------------------------------------
void FASTCALL CWindrv::GetIoctrl(DWORD param, DWORD func, Human68k::ioctrl_t* pIoctrl)
{
	ASSERT(this);
	ASSERT(pIoctrl);

	switch (func) {
	case 2:
		// メディア再認識
		pIoctrl->param = param;
		return;

	case -2:
		// オプション設定
		ASSERT(param <= 0xFFFFFF);
		pIoctrl->param = GetLong(param);
		return;

#if 0
	case 0:
		// メディアIDの獲得
		return;

	case 1:
		// Human68k互換のためのダミー
		return;

	case -1:
		// 常駐判定
		return;

	case -3:
		// オプション獲得
		return;
#endif
	}
}

//---------------------------------------------------------------------------
//
//	IOCTRL書き込み
//
//---------------------------------------------------------------------------
void FASTCALL CWindrv::SetIoctrl(DWORD param, DWORD func, const Human68k::ioctrl_t* pIoctrl)
{
	ASSERT(this);
	ASSERT(pIoctrl);

	switch (func) {
	case 0:
		// メディアIDの獲得
		ASSERT(param <= 0xFFFFFF);
		SetWord(param, pIoctrl->media);
		return;

	case 1:
		// Human68k互換のためのダミー
		ASSERT(param <= 0xFFFFFF);
		SetLong(param, pIoctrl->param);
		return;

	case -1:
		// 常駐判定
		ASSERT(param <= 0xFFFFFF);
		{
			for (int i = 0; i < 8; i++) SetByte(param + i, pIoctrl->buffer[i]);
		}
		return;

	case -3:
		// オプション獲得
		ASSERT(param <= 0xFFFFFF);
		SetLong(param, pIoctrl->param);
		return;

#if 0
	case 2:
		// メディア再認識
		return;

	case -2:
		// オプション設定
		return;
#endif
	}
}

//---------------------------------------------------------------------------
//
//	パラメータ読み込み
//
//---------------------------------------------------------------------------
void FASTCALL CWindrv::GetParameter(DWORD addr, BYTE* pOption, DWORD nSize)
{
	ASSERT(this);
	ASSERT(addr <= 0xFFFFFF);
	ASSERT(pOption);

	DWORD nMode = 0;
	BYTE* p = pOption;
	for (DWORD i = 0; i < nSize - 2; i++) {
		BYTE c = (BYTE)GetByte(addr + i);
		*p++ = c;
		switch (nMode) {
		case 0:
			if (c == '\0') {
				return;
			}
			nMode = 1;
			break;
		case 1:
			if (c == '\0') nMode = 0;
			break;
		}
	}

	*p++ = '\0';
	*p++ = '\0';
}

#ifdef WINDRV_LOG
//---------------------------------------------------------------------------
//
//	ログ出力
//
//---------------------------------------------------------------------------
void FASTCALL CWindrv::Log(DWORD level, char* format, ...) const
{
	ASSERT(this);
	ASSERT(format);
	ASSERT(windrv);

	va_list args;
	va_start(args, format);
	char message[512];
	vsprintf(message, format, args);
	va_end(args);

	windrv->Log(level, message);
}

//---------------------------------------------------------------------------
//
//	ログ出力
//
//---------------------------------------------------------------------------
void FASTCALL Windrv::Log(DWORD level, char* message) const
{
	ASSERT(this);
	ASSERT(message);

	LOG((enum Log::loglevel)level, "%s", message);
}
#endif // WINDRV_LOG
