//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2004 ＰＩ．(ytanaka@ipc-tokai.or.jp)
//	[ MFC キュー ]
//
//---------------------------------------------------------------------------

#if defined(_WIN32)

#include "os.h"
#include "xm6.h"
#include "mfc_que.h"

//===========================================================================
//
//	キュー
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	コンストラクタ
//
//---------------------------------------------------------------------------
CQueue::CQueue()
{
	// ポインタなし
	m_Queue.pBuf = NULL;

	// サイズ0
	m_Queue.dwSize = 0;
	m_Queue.dwMask = 0;

	// その他
	Clear();
}

//---------------------------------------------------------------------------
//
//	デストラクタ
//
//---------------------------------------------------------------------------
CQueue::~CQueue()
{
	// メモリ解放
	if (m_Queue.pBuf) {
		delete[] m_Queue.pBuf;
		m_Queue.pBuf = NULL;
	}
}

//---------------------------------------------------------------------------
//
//	初期化
//
//---------------------------------------------------------------------------
BOOL FASTCALL CQueue::Init(DWORD dwSize)
{
	ASSERT(this);
	ASSERT(!m_Queue.pBuf);
	ASSERT(m_Queue.dwSize == 0);
	ASSERT(dwSize > 0);

	// メモリ確保
	try {
		m_Queue.pBuf = new BYTE[dwSize];
	}
	catch (...) {
		m_Queue.pBuf = NULL;
		return FALSE;
	}
	if (!m_Queue.pBuf) {
		return FALSE;
	}

	// サイズ設定
	m_Queue.dwSize = dwSize;
	m_Queue.dwMask = m_Queue.dwSize - 1;

	// クリア
	Clear();

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	クリア
//	※同期非対応
//
//---------------------------------------------------------------------------
void FASTCALL CQueue::Clear()
{
	ASSERT(this);

	m_Queue.dwRead = 0;
	m_Queue.dwWrite = 0;
	m_Queue.dwNum = 0;
	m_Queue.dwTotalRead = 0;
	m_Queue.dwTotalWrite = 0;
}

//---------------------------------------------------------------------------
//
//	キュー内のデータをすべて取得
//	※同期非対応
//
//---------------------------------------------------------------------------
DWORD FASTCALL CQueue::Get(BYTE *pDest)
{
	DWORD dwLength;
	DWORD dwRest;

	ASSERT(this);
	ASSERT(m_Queue.dwSize > 0);
	ASSERT(m_Queue.pBuf);
	ASSERT((m_Queue.dwMask + 1) == m_Queue.dwSize);
	ASSERT(m_Queue.dwRead < m_Queue.dwSize);
	ASSERT(m_Queue.dwWrite < m_Queue.dwSize);
	ASSERT(m_Queue.dwNum <= m_Queue.dwSize);

	ASSERT(pDest);

	// データがなければ0
	if (m_Queue.dwNum == 0) {
		return 0;
	}

	// 現在分、オーバーラップ分の個数を決める
	dwLength = m_Queue.dwNum;
	dwRest = 0;
	if ((dwLength + m_Queue.dwRead) > m_Queue.dwSize) {
		dwRest = (dwLength + m_Queue.dwRead) - m_Queue.dwSize;
		dwLength -= dwRest;
	}

	// カレント
	memcpy(pDest, &m_Queue.pBuf[m_Queue.dwRead], dwLength);
	m_Queue.dwRead += dwLength;
	m_Queue.dwRead &= m_Queue.dwMask;
	m_Queue.dwNum -= dwLength;

	// オーバーラップ
	memcpy(&pDest[dwLength], m_Queue.pBuf, dwRest);
	m_Queue.dwRead += dwRest;
	m_Queue.dwRead &= m_Queue.dwMask;
	m_Queue.dwNum -= dwRest;

	// トータルReadを増やす
	m_Queue.dwTotalRead += dwLength;
	m_Queue.dwTotalRead += dwRest;

	// コピーしたサイズを返す
	return (DWORD)(dwLength + dwRest);
}

//---------------------------------------------------------------------------
//
//	キュー内のデータをすべて取得(キュー進めない)
//	※同期非対応
//
//---------------------------------------------------------------------------
DWORD FASTCALL CQueue::Copy(BYTE *pDest) const
{
	DWORD dwLength;
	DWORD dwRest;

	ASSERT(this);
	ASSERT(m_Queue.dwSize > 0);
	ASSERT(m_Queue.pBuf);
	ASSERT((m_Queue.dwMask + 1) == m_Queue.dwSize);
	ASSERT(m_Queue.dwRead < m_Queue.dwSize);
	ASSERT(m_Queue.dwWrite < m_Queue.dwSize);
	ASSERT(m_Queue.dwNum <= m_Queue.dwSize);

	ASSERT(pDest);

	// データがなければ0
	if (m_Queue.dwNum == 0) {
		return 0;
	}

	// 現在分、オーバーラップ分の個数を決める
	dwLength = m_Queue.dwNum;
	dwRest = 0;
	if ((dwLength + m_Queue.dwRead) > m_Queue.dwSize) {
		dwRest = (dwLength + m_Queue.dwRead) - m_Queue.dwSize;
		dwLength -= dwRest;
	}

	// カレント
	memcpy(pDest, &m_Queue.pBuf[m_Queue.dwRead], dwLength);

	// オーバーラップ
	memcpy(&pDest[dwLength], m_Queue.pBuf, dwRest);

	// コピーしたサイズを返す
	return (DWORD)(dwLength + dwRest);
}

//---------------------------------------------------------------------------
//
//	キューを進める
//	※同期非対応。Copy->Diskcard間でGetしないこと
//
//---------------------------------------------------------------------------
void FASTCALL CQueue::Discard(DWORD dwNum)
{
	ASSERT(this);
	ASSERT(m_Queue.dwSize > 0);
	ASSERT(m_Queue.pBuf);
	ASSERT((m_Queue.dwMask + 1) == m_Queue.dwSize);
	ASSERT(m_Queue.dwRead < m_Queue.dwSize);
	ASSERT(m_Queue.dwWrite < m_Queue.dwSize);
	ASSERT(m_Queue.dwNum <= m_Queue.dwSize);

	ASSERT(dwNum <= m_Queue.dwSize);
	ASSERT(dwNum <= m_Queue.dwNum);

	// 進める
	m_Queue.dwRead += dwNum;
	m_Queue.dwRead &= m_Queue.dwMask;

	// 個数
	m_Queue.dwNum -= dwNum;

	// トータル
	m_Queue.dwTotalRead += dwNum;
}

//---------------------------------------------------------------------------
//
//	キューを戻す
//	※同期非対応。Get->Back間でInsertしないこと
//
//---------------------------------------------------------------------------
void FASTCALL CQueue::Back(DWORD dwNum)
{
	DWORD dwRest;

	ASSERT(this);
	ASSERT(m_Queue.dwSize > 0);
	ASSERT(m_Queue.pBuf);
	ASSERT((m_Queue.dwMask + 1) == m_Queue.dwSize);
	ASSERT(m_Queue.dwRead < m_Queue.dwSize);
	ASSERT(m_Queue.dwWrite < m_Queue.dwSize);
	ASSERT(m_Queue.dwNum <= m_Queue.dwSize);

	ASSERT(dwNum <= m_Queue.dwSize);

	// dwRest分を算出
	dwRest = 0;
	if (m_Queue.dwRead < dwNum) {
		dwRest = dwNum - m_Queue.dwRead;
		dwNum = m_Queue.dwRead;
	}

	// カレント
	m_Queue.dwRead -= dwNum;
	m_Queue.dwRead &= m_Queue.dwMask;
	m_Queue.dwNum += dwNum;

	// オーバーラップ
	m_Queue.dwRead -= dwRest;
	m_Queue.dwRead &= m_Queue.dwMask;
	m_Queue.dwNum += dwRest;

	// トータル
	m_Queue.dwTotalRead -= dwNum;
	m_Queue.dwTotalRead -= dwRest;
}

//---------------------------------------------------------------------------
//
//	キューの空き個数を取得
//	※同期非対応。
//
//---------------------------------------------------------------------------
DWORD FASTCALL CQueue::GetFree() const
{
	ASSERT(this);
	ASSERT(m_Queue.dwSize > 0);
	ASSERT(m_Queue.pBuf);
	ASSERT((m_Queue.dwMask + 1) == m_Queue.dwSize);
	ASSERT(m_Queue.dwRead < m_Queue.dwSize);
	ASSERT(m_Queue.dwWrite < m_Queue.dwSize);
	ASSERT(m_Queue.dwNum <= m_Queue.dwSize);

	return (DWORD)(m_Queue.dwSize - m_Queue.dwNum);
}

//---------------------------------------------------------------------------
//
//	キューに挿入
//	※同期非対応。
//
//---------------------------------------------------------------------------
BOOL FASTCALL CQueue::Insert(const BYTE *pSrc, DWORD dwLength)
{
	DWORD dwRest;

	ASSERT(this);
	ASSERT(m_Queue.dwSize > 0);
	ASSERT(m_Queue.pBuf);
	ASSERT((m_Queue.dwMask + 1) == m_Queue.dwSize);
	ASSERT(m_Queue.dwRead < m_Queue.dwSize);
	ASSERT(m_Queue.dwWrite < m_Queue.dwSize);
	ASSERT(m_Queue.dwNum <= m_Queue.dwSize);

	ASSERT(pSrc);

	// 現在分、オーバーラップ分の個数を決める
	dwRest = 0;
	if ((dwLength + m_Queue.dwWrite) > m_Queue.dwSize) {
		dwRest = (dwLength + m_Queue.dwWrite) - m_Queue.dwSize;
		dwLength -= dwRest;
	}

	// カレント
	memcpy(&m_Queue.pBuf[m_Queue.dwWrite], pSrc, dwLength);
	m_Queue.dwWrite += dwLength;
	m_Queue.dwWrite &= m_Queue.dwMask;
	m_Queue.dwNum += dwLength;

	// オーバーラップ
	memcpy(m_Queue.pBuf, &pSrc[dwLength], dwRest);
	m_Queue.dwWrite += dwRest;
	m_Queue.dwWrite &= m_Queue.dwMask;
	m_Queue.dwNum += dwRest;

	// トータルWriteを増やす
	m_Queue.dwTotalWrite += dwLength;
	m_Queue.dwTotalWrite += dwRest;

	// Read位置を超えてしまった場合は補正してFALSE
	if (m_Queue.dwNum > m_Queue.dwSize) {
		// バッファオーバーラン
		m_Queue.dwNum = m_Queue.dwSize;
		m_Queue.dwRead = m_Queue.dwWrite;
		return FALSE;
	}
	return TRUE;
}

//---------------------------------------------------------------------------
//
//	キュー情報取得
//	※同期非対応。
//
//---------------------------------------------------------------------------
void FASTCALL CQueue::GetQueue(QUEUEINFO *pInfo) const
{
	ASSERT(this);
	ASSERT(pInfo);

	*pInfo = m_Queue;
}

#endif	// _WIN32
