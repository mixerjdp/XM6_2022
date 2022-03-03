//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2005 ＰＩ．(ytanaka@ipc-tokai.or.jp)
//	[ グラフィックVRAM ]
//
//---------------------------------------------------------------------------

#include "os.h"
#include "xm6.h"
#include "vm.h"
#include "log.h"
#include "schedule.h"
#include "fileio.h"
#include "render.h"
#include "renderin.h"
#include "gvram.h"

//===========================================================================
//
//	グラフィックVRAMハンドラ
//
//===========================================================================
//#define GVRAM_LOG

//---------------------------------------------------------------------------
//
//	コンストラクタ
//
//---------------------------------------------------------------------------
GVRAMHandler::GVRAMHandler(Render *rend, BYTE *mem, CPU *p)
{
	ASSERT(rend);
	ASSERT(mem);
	ASSERT(p);

	render = rend;
	gvram = mem;
	cpu = p;
}

//===========================================================================
//
//	グラフィックVRAMハンドラ(1024×1024)
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	コンストラクタ
//
//---------------------------------------------------------------------------
GVRAM1024::GVRAM1024(Render *rend, BYTE *mem, CPU *cpu) : GVRAMHandler(rend, mem, cpu)
{
}

//---------------------------------------------------------------------------
//
//	バイト読み込み
//
//---------------------------------------------------------------------------
DWORD FASTCALL GVRAM1024::ReadByte(DWORD addr)
{
	DWORD offset;

	ASSERT(this);
	ASSERT(addr <= 0x1fffff);

	// 偶数バイトは読み出しても0
	if ((addr & 1) == 0) {
		return 0x00;
	}

	// 共通項
	offset = addr & 0x3ff;

	// 上下左右で分ける
	if (addr & 0x100000) {
		if (addr & 0x400) {
			// ページ3空間
			addr >>= 1;
			addr &= 0x7fc00;
			addr |= offset;
			return (gvram[addr] >> 4);
		}
		else {
			// ページ2空間
			addr >>= 1;
			addr &= 0x7fc00;
			addr |= offset;
			return (gvram[addr] & 0x0f);
		}
	}
	else {
		if (addr & 0x400) {
			// ページ1空間
			addr >>= 1;
			addr &= 0x7fc00;
			addr |= offset;
			return (gvram[addr ^ 1] >> 4);
		}
		else {
			// ページ0空間
			addr >>= 1;
			addr &= 0x7fc00;
			addr |= offset;
			return (gvram[addr ^ 1] & 0x0f);
		}
	}
}

//---------------------------------------------------------------------------
//
//	ワード読み込み
//
//---------------------------------------------------------------------------
DWORD FASTCALL GVRAM1024::ReadWord(DWORD addr)
{
	DWORD offset;

	ASSERT(this);
	ASSERT(addr <= 0x1fffff);
	ASSERT((addr & 1) == 0);

	// 共通項
	offset = addr & 0x3ff;

	// 上下左右で分ける
	if (addr & 0x100000) {
		if (addr & 0x400) {
			// ページ3空間
			addr >>= 1;
			addr &= 0x7fc00;
			addr |= offset;
			return (gvram[addr ^ 1] >> 4);
		}
		else {
			// ページ2空間
			addr >>= 1;
			addr &= 0x7fc00;
			addr |= offset;
			return (gvram[addr ^ 1] & 0x0f);
		}
	}
	else {
		if (addr & 0x400) {
			// ページ1空間
			addr >>= 1;
			addr &= 0x7fc00;
			addr |= offset;
			return (gvram[addr] >> 4);
		}
		else {
			// ページ0空間
			addr >>= 1;
			addr &= 0x7fc00;
			addr |= offset;
			return (gvram[addr] & 0x0f);
		}
	}
}

//---------------------------------------------------------------------------
//
//	バイト書き込み
//
//---------------------------------------------------------------------------
void FASTCALL GVRAM1024::WriteByte(DWORD addr, DWORD data)
{
	DWORD offset;
	DWORD mem;

	ASSERT(this);
	ASSERT(addr <= 0x1fffff);
	ASSERT(data < 0x100);

	// 偶数バイトは書き込めない
	if ((addr & 1) == 0) {
		return;
	}

	// 共通項
	offset = addr & 0x3ff;

	// 上下左右で分ける
	if (addr & 0x100000) {
		if (addr & 0x400) {
			// ページ3空間
			addr >>= 1;
			addr &= 0x7fc00;
			addr |= offset;

			// 上位ニブルへ
			mem = (gvram[addr] & 0x0f);
			mem |= (data << 4);

			// 書き込み
			if (gvram[addr] != mem) {
				gvram[addr] = (BYTE)mem;
				render->GrpMem(addr, 3);
			}
		}
		else {
			// ページ2空間
			addr >>= 1;
			addr &= 0x7fc00;
			addr |= offset;

			// 下位ニブルへ
			mem = (gvram[addr] & 0xf0);
			mem |= (data & 0x0f);

			// 書き込み
			if (gvram[addr] != mem) {
				gvram[addr] = (BYTE)mem;
				render->GrpMem(addr, 2);
			}
		}
	}
	else {
		if (addr & 0x400) {
			// ページ1空間
			addr >>= 1;
			addr &= 0x7fc00;
			addr |= offset;

			// 上位ニブルへ
			mem = (gvram[addr ^ 1] & 0x0f);
			mem |= (data << 4);

			// 書き込み
			if (gvram[addr ^ 1] != mem) {
				gvram[addr ^ 1] = (BYTE)mem;
				render->GrpMem(addr ^ 1, 1);
			}
		}
		else {
			// ページ0空間
			addr >>= 1;
			addr &= 0x7fc00;
			addr |= offset;

			// 下位ニブルヘ
			mem = (gvram[addr ^ 1] & 0xf0);
			mem |= (data & 0x0f);

			// 書き込み
			if (gvram[addr ^ 1] != mem) {
				gvram[addr ^ 1] = (BYTE)mem;
				render->GrpMem(addr ^ 1, 0);
			}
		}
	}
}

//---------------------------------------------------------------------------
//
//	ワード書き込み
//
//---------------------------------------------------------------------------
void FASTCALL GVRAM1024::WriteWord(DWORD addr, DWORD data)
{
	DWORD offset;
	DWORD mem;

	ASSERT(this);
	ASSERT(addr <= 0x1fffff);
	ASSERT(data < 0x10000);

	// 共通項
	offset = addr & 0x3ff;

	// 上下左右で分ける
	if (addr & 0x100000) {
		if (addr & 0x400) {
			// ページ3空間
			addr >>= 1;
			addr &= 0x7fc00;
			addr |= offset;

			// 上位ニブルへ
			mem = (gvram[addr ^ 1] & 0x0f);
			data &= 0x0f;
			mem |= (data << 4);

			// 書き込み
			if (gvram[addr ^ 1] != mem) {
				gvram[addr ^ 1] = (BYTE)mem;
				render->GrpMem(addr ^ 1, 3);
			}
		}
		else {
			// ページ2空間
			addr >>= 1;
			addr &= 0x7fc00;
			addr |= offset;

			// 下位ニブルへ
			mem = (gvram[addr ^ 1] & 0xf0);
			mem |= (data & 0x0f);

			// 書き込み
			if (gvram[addr ^ 1] != mem) {
				gvram[addr ^ 1] = (BYTE)mem;
				render->GrpMem(addr ^ 1, 2);
			}
		}
	}
	else {
		if (addr & 0x400) {
			// ページ1空間
			addr >>= 1;
			addr &= 0x7fc00;
			addr |= offset;

			// 上位ニブルへ
			mem = (gvram[addr] & 0x0f);
			data &= 0x0f;
			mem |= (data << 4);

			// 書き込み
			if (gvram[addr] != mem) {
				gvram[addr] = (BYTE)mem;
				render->GrpMem(addr, 1);
			}
		}
		else {
			// ページ0空間
			addr >>= 1;
			addr &= 0x7fc00;
			addr |= offset;

			// 下位ニブルへ
			mem = (gvram[addr] & 0xf0);
			mem |= (data & 0x0f);

			// 書き込み
			if (gvram[addr] != mem) {
				gvram[addr] = (BYTE)mem;
				render->GrpMem(addr, 0);
			}
		}
	}
}

//---------------------------------------------------------------------------
//
//	読み込みのみ
//
//---------------------------------------------------------------------------
DWORD FASTCALL GVRAM1024::ReadOnly(DWORD addr) const
{
	DWORD offset;

	ASSERT(this);
	ASSERT(addr <= 0x1fffff);

	// 偶数バイトは読み出しても0
	if ((addr & 1) == 0) {
		return 0x00;
	}

	// 共通項
	offset = addr & 0x3ff;

	// 上下左右で分ける
	if (addr & 0x100000) {
		if (addr & 0x400) {
			// ページ3空間
			addr >>= 1;
			addr &= 0x7fc00;
			addr |= offset;
			return (gvram[addr] >> 4);
		}
		else {
			// ページ2空間
			addr >>= 1;
			addr &= 0x7fc00;
			addr |= offset;
			return (gvram[addr] & 0x0f);
		}
	}
	else {
		if (addr & 0x400) {
			// ページ1空間
			addr >>= 1;
			addr &= 0x7fc00;
			addr |= offset;
			return (gvram[addr ^ 1] >> 4);
		}
		else {
			// ページ0空間
			addr >>= 1;
			addr &= 0x7fc00;
			addr |= offset;
			return (gvram[addr ^ 1] & 0x0f);
		}
	}
}

//===========================================================================
//
//	グラフィックVRAMハンドラ(16色)
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	コンストラクタ
//
//---------------------------------------------------------------------------
GVRAM16::GVRAM16(Render *rend, BYTE *mem, CPU *cpu) : GVRAMHandler(rend, mem, cpu)
{
}

//---------------------------------------------------------------------------
//
//	バイト読み込み
//
//---------------------------------------------------------------------------
DWORD FASTCALL GVRAM16::ReadByte(DWORD addr)
{
	ASSERT(this);
	ASSERT(addr <= 0x1fffff);

	// 奇数アドレスのみ
	if (addr & 1) {
		if (addr < 0x80000) {
			// ページ0:ワードの下位バイトb0-b3
			return (gvram[addr ^ 1] & 0x0f);
		}

		if (addr < 0x100000) {
			// ページ1:ワードの下位バイトb4-b7
			addr &= 0x7ffff;
			return (gvram[addr ^ 1] >> 4);
		}

		if (addr < 0x180000) {
			// ページ2:ワードの上位バイトb0-b3
			addr &= 0x7ffff;
			return (gvram[addr] & 0x0f);
		}

		// ページ3:ワードの上位バイトb4-b7
		addr &= 0x7ffff;
		return (gvram[addr] >> 4);
	}

	// 偶数アドレスは常に0
	return 0;
}

//---------------------------------------------------------------------------
//
//	ワード読み込み
//
//---------------------------------------------------------------------------
DWORD FASTCALL GVRAM16::ReadWord(DWORD addr)
{
	ASSERT(this);
	ASSERT(addr <= 0x1fffff);
	ASSERT((addr & 1) == 0);

	if (addr < 0x80000) {
		// ページ0:ワードの下位バイトb0-b3
		return (gvram[addr] & 0x0f);
	}

	if (addr < 0x100000) {
		// ページ1:ワードの下位バイトb4-b7
		addr &= 0x7ffff;
		return (gvram[addr] >> 4);
	}

	if (addr < 0x180000) {
		// ページ2:ワードの上位バイトb0-b3
		addr &= 0x7ffff;
		return (gvram[addr ^ 1] & 0x0f);
	}

	// ページ3:ワードの上位バイトb4-b7
	addr &= 0x7ffff;
	return (gvram[addr ^ 1] >> 4);
}

//---------------------------------------------------------------------------
//
//	バイト書き込み
//
//---------------------------------------------------------------------------
void FASTCALL GVRAM16::WriteByte(DWORD addr, DWORD data)
{
	DWORD mem;

	ASSERT(this);
	ASSERT(addr <= 0x1fffff);
	ASSERT(data < 0x100);

	// 奇数アドレスのみ
	if (addr & 1) {
		if (addr < 0x80000) {
			// ページ0:ワードの下位バイトb0-b3
			mem = (gvram[addr ^ 1] & 0xf0);
			mem |= (data & 0x0f);

			// 書き込み
			if (gvram[addr ^ 1] != mem) {
				gvram[addr ^ 1] = (BYTE)mem;
				render->GrpMem(addr ^ 1, 0);
			}
			return;
		}

		if (addr < 0x100000) {
			// ページ1:ワードの下位バイトb4-b7
			addr &= 0x7ffff;
			mem = (gvram[addr ^ 1] & 0x0f);
			mem |= (data << 4);

			// 書き込み
			if (gvram[addr ^ 1] != mem) {
				gvram[addr ^ 1] = (BYTE)mem;
				render->GrpMem(addr ^ 1, 1);
			}
			return;
		}

		if (addr < 0x180000) {
			// ページ2:ワードの上位バイトb0-b3
			addr &= 0x7ffff;
			mem = (gvram[addr] & 0xf0);
			mem |= (data & 0x0f);

			// 書き込み
			if (gvram[addr] != mem) {
				gvram[addr] = (BYTE)mem;
				render->GrpMem(addr, 2);
			}
			return;
		}

		// ページ3:ワードの上位バイトb4-b7
		addr &= 0x7ffff;
		mem = (gvram[addr] & 0x0f);
		mem |= (data << 4);

		// 書き込み
		if (gvram[addr] != mem) {
			gvram[addr] = (BYTE)mem;
			render->GrpMem(addr, 3);
		}
		return;
	}

	// 偶数アドレスは書き込めない
}

//---------------------------------------------------------------------------
//
//	ワード書き込み
//
//---------------------------------------------------------------------------
void FASTCALL GVRAM16::WriteWord(DWORD addr, DWORD data)
{
	DWORD mem;

	ASSERT(this);
	ASSERT(addr <= 0x1fffff);
	ASSERT((addr & 1) == 0);
	ASSERT(data < 0x10000);

	if (addr < 0x80000) {
		// ページ0:ワードの下位バイトb0-b3
		mem = (gvram[addr] & 0xf0);
		mem |= (data & 0x0f);

		// 書き込み
		if (gvram[addr] != mem) {
			gvram[addr] = (BYTE)mem;
			render->GrpMem(addr, 0);
		}
		return;
	}
	if (addr < 0x100000) {
		// ページ1:ワードの下位バイトb4-b7
		addr &= 0x7ffff;
		mem = (gvram[addr] & 0x0f);
		data &= 0x0f;
		mem |= (data << 4);

		// 書き込み
		if (gvram[addr] != mem) {
			gvram[addr] = (BYTE)mem;
			render->GrpMem(addr, 1);
		}
		return;
	}
	if (addr < 0x180000) {
		// ページ2:ワードの上位バイトb0-b3
		addr &= 0x7ffff;
		mem = (gvram[addr ^ 1] & 0xf0);
		mem |= (data & 0x0f);

		// 書き込み
		if (gvram[addr ^ 1] != mem) {
			gvram[addr ^ 1] = (BYTE)mem;
			render->GrpMem(addr ^ 1, 2);
		}
		return;
	}

	// ページ3:ワードの上位バイトb4-b7
	addr &= 0x7ffff;
	mem = (gvram[addr ^ 1] & 0x0f);
	data &= 0x0f;
	mem |= (data << 4);

	// 書き込み
	if (gvram[addr ^ 1] != mem) {
		gvram[addr ^ 1] = (BYTE)mem;
		render->GrpMem(addr ^ 1, 3);
	}
}

//---------------------------------------------------------------------------
//
//	読み込みのみ
//
//---------------------------------------------------------------------------
DWORD FASTCALL GVRAM16::ReadOnly(DWORD addr) const
{
	ASSERT(this);
	ASSERT(addr <= 0x1fffff);

	// 奇数アドレスのみ
	if (addr & 1) {
		if (addr < 0x80000) {
			// ページ0:ワードの下位バイトb0-b3
			return (gvram[addr ^ 1] & 0x0f);
		}

		if (addr < 0x100000) {
			// ページ1:ワードの下位バイトb4-b7
			addr &= 0x7ffff;
			return (gvram[addr ^ 1] >> 4);
		}

		if (addr < 0x180000) {
			// ページ2:ワードの上位バイトb0-b3
			addr &= 0x7ffff;
			return (gvram[addr] & 0x0f);
		}

		// ページ3:ワードの上位バイトb4-b7
		addr &= 0x7ffff;
		return (gvram[addr] >> 4);
	}

	// 偶数アドレスは常に0
	return 0;
}

//===========================================================================
//
//	グラフィックVRAMハンドラ(256色)
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	コンストラクタ
//
//---------------------------------------------------------------------------
GVRAM256::GVRAM256(Render *rend, BYTE *mem, CPU *p) : GVRAMHandler(rend, mem, p)
{
}

//---------------------------------------------------------------------------
//
//	バイト読み込み
//
//---------------------------------------------------------------------------
DWORD FASTCALL GVRAM256::ReadByte(DWORD addr)
{
	ASSERT(this);
	ASSERT(addr <= 0x1fffff);

	// ページ0
	if (addr < 0x80000) {
		if (addr & 1) {
			// ワードの下位バイト
			return gvram[addr ^ 1];
		}
		return 0;
	}

	// ページ1
	if (addr < 0x100000) {
		addr &= 0x7ffff;
		if (addr & 1) {
			// ワードの上位バイト
			return gvram[addr];
		}
		return 0;
	}

	// バスエラー
	cpu->BusErr(addr + 0xc00000, TRUE);
	return 0xff;
}

//---------------------------------------------------------------------------
//
//	ワード読み込み
//
//---------------------------------------------------------------------------
DWORD FASTCALL GVRAM256::ReadWord(DWORD addr)
{
	ASSERT(this);
	ASSERT(addr <= 0x1fffff);
	ASSERT((addr & 1) == 0);

	// ページ0
	if (addr < 0x80000) {
		// ワードの下位バイト
		return gvram[addr];
	}

	// ページ1
	if (addr < 0x100000) {
		addr &= 0x7ffff;
		// ワードの上位バイト
		return gvram[addr ^ 1];
	}

	// バスエラー
	cpu->BusErr(addr + 0xc00000, TRUE);
	return 0xff;
}

//---------------------------------------------------------------------------
//
//	バイト書き込み
//
//---------------------------------------------------------------------------
void FASTCALL GVRAM256::WriteByte(DWORD addr, DWORD data)
{
	ASSERT(this);
	ASSERT(addr <= 0x1fffff);
	ASSERT(data < 0x100);

	// ページ0
	if (addr < 0x80000) {
		if (addr & 1) {
			// ワードの下位バイト
			if (gvram[addr ^ 1] != data) {
				gvram[addr ^ 1] = (BYTE)data;
				render->GrpMem(addr ^ 1, 0);
			}
		}
		return;
	}

	// ページ1(ブロック2)
	if (addr < 0x100000) {
		addr &= 0x7ffff;
		if (addr & 1) {
			// ワードの上位バイト
			if (gvram[addr] != data) {
				gvram[addr] = (BYTE)data;
				render->GrpMem(addr, 2);
			}
		}
		return;
	}

	// バスエラー
	cpu->BusErr(addr + 0xc00000, FALSE);
}

//---------------------------------------------------------------------------
//
//	ワード書き込み
//
//---------------------------------------------------------------------------
void FASTCALL GVRAM256::WriteWord(DWORD addr, DWORD data)
{
	ASSERT(this);
	ASSERT(addr <= 0x1fffff);
	ASSERT((addr & 1) == 0);
	ASSERT(data < 0x10000);

	// ページ0
	if (addr < 0x80000) {
		// ワードの下位バイト
		if (gvram[addr] != data) {
			gvram[addr] = (BYTE)data;
			render->GrpMem(addr, 0);
		}
		return;
	}

	// ページ1(ブロック2)
	if (addr < 0x100000) {
		addr &= 0x7ffff;
		// ワードの上位バイト
		if (gvram[addr ^ 1] != data) {
			gvram[addr ^ 1] = (BYTE)data;
			render->GrpMem(addr ^ 1, 2);
		}
		return;
	}

	// バスエラー
	cpu->BusErr(addr + 0xc00000, FALSE);
}

//---------------------------------------------------------------------------
//
//	読み込みのみ
//
//---------------------------------------------------------------------------
DWORD FASTCALL GVRAM256::ReadOnly(DWORD addr) const
{
	ASSERT(this);
	ASSERT(addr <= 0x1fffff);

	// ページ0
	if (addr < 0x80000) {
		if (addr & 1) {
			// ワードの下位バイト
			return gvram[addr ^ 1];
		}
		return 0;
	}

	// ページ1
	if (addr < 0x100000) {
		addr &= 0x7ffff;
		if (addr & 1) {
			// ワードの上位バイト
			return gvram[addr];
		}
		return 0;
	}

	// バスエラー
	return 0xff;
}

//===========================================================================
//
//	グラフィックVRAMハンドラ(無効)
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	コンストラクタ
//
//---------------------------------------------------------------------------
GVRAMNDef::GVRAMNDef(Render *rend, BYTE *mem, CPU *p) : GVRAMHandler(rend, mem, p)
{
}

//---------------------------------------------------------------------------
//
//	バイト読み込み
//
//---------------------------------------------------------------------------
DWORD FASTCALL GVRAMNDef::ReadByte(DWORD addr)
{
	ASSERT(this);
	ASSERT(addr <= 0x1fffff);

	// 無効ページ
	if (addr & 0x80000) {
		return 0;
	}

	// 下位ページ
	addr &= 0x7ffff;
	if (addr & 1) {
		return gvram[addr ^ 1];
	}
	return 0;
}

//---------------------------------------------------------------------------
//
//	ワード読み込み
//
//---------------------------------------------------------------------------
DWORD FASTCALL GVRAMNDef::ReadWord(DWORD addr)
{
	ASSERT(this);
	ASSERT(addr <= 0x1fffff);
	ASSERT((addr & 1) == 0);

	// 無効ページ
	if (addr & 0x80000) {
		return 0;
	}

	// 下位ページ
	addr &= 0x7ffff;
	return gvram[addr ^ 1];
}

//---------------------------------------------------------------------------
//
//	バイト書き込み
//
//---------------------------------------------------------------------------
void FASTCALL GVRAMNDef::WriteByte(DWORD addr, DWORD data)
{
	ASSERT(this);
	ASSERT(addr <= 0x1fffff);
	ASSERT(data < 0x100);

	// 無効ページ
	if (addr & 0x80000) {
		return;
	}

	// 下位ページ
	addr &= 0x7ffff;
	if (addr & 1) {
		if (gvram[addr ^ 1] != data) {
			gvram[addr ^ 1] = (BYTE)data;
			render->GrpMem(addr ^ 1, 0);
		}
	}
}

//---------------------------------------------------------------------------
//
//	ワード書き込み
//
//---------------------------------------------------------------------------
void FASTCALL GVRAMNDef::WriteWord(DWORD addr, DWORD data)
{
	ASSERT(this);
	ASSERT(addr <= 0x1fffff);
	ASSERT((addr & 1) == 0);
	ASSERT(data < 0x10000);

	// 無効ページ
	if (addr & 0x80000) {
		return;
	}

	// 下位ページ
	addr &= 0x7ffff;
	if (gvram[addr ^ 1] != data) {
		gvram[addr ^ 1] = (BYTE)data;
		render->GrpMem(addr ^ 1, 0);
	}
}

//---------------------------------------------------------------------------
//
//	読み込みのみ
//
//---------------------------------------------------------------------------
DWORD FASTCALL GVRAMNDef::ReadOnly(DWORD addr) const
{
	ASSERT(this);
	ASSERT(addr <= 0x1fffff);

	// 無効ページ
	if (addr & 0x80000) {
		return 0;
	}

	// 下位ページ
	addr &= 0x7ffff;
	if (addr & 1) {
		return gvram[addr ^ 1];
	}
	return 0;
}

//===========================================================================
//
//	グラフィックVRAMハンドラ(65536色)
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	コンストラクタ
//
//---------------------------------------------------------------------------
GVRAM64K::GVRAM64K(Render *rend, BYTE *mem, CPU *p) : GVRAMHandler(rend, mem, p)
{
}

//---------------------------------------------------------------------------
//
//	バイト読み込み
//
//---------------------------------------------------------------------------
DWORD FASTCALL GVRAM64K::ReadByte(DWORD addr)
{
	ASSERT(this);
	ASSERT(addr <= 0x1fffff);

	if (addr < 0x80000) {
		return gvram[addr ^ 1];
	}

	// バスエラー
	cpu->BusErr(addr + 0xc00000, TRUE);
	return 0xff;
}

//---------------------------------------------------------------------------
//
//	ワード読み込み
//
//---------------------------------------------------------------------------
DWORD FASTCALL GVRAM64K::ReadWord(DWORD addr)
{
	ASSERT(this);
	ASSERT(addr <= 0x1fffff);
	ASSERT((addr & 1) == 0);

	if (addr < 0x80000) {
		return *(WORD*)(&gvram[addr]);
	}

	// バスエラー
	cpu->BusErr(addr + 0xc00000, TRUE);
	return 0xffff;
}

//---------------------------------------------------------------------------
//
//	バイト書き込み
//
//---------------------------------------------------------------------------
void FASTCALL GVRAM64K::WriteByte(DWORD addr, DWORD data)
{
	ASSERT(this);
	ASSERT(addr <= 0x1fffff);
	ASSERT(data < 0x100);

	if (addr < 0x80000) {
		if (gvram[addr ^ 1] != data) {
			gvram[addr ^ 1] = (BYTE)data;
			render->GrpMem(addr ^ 1, 0);
		}
		return;
	}

	// バスエラー
	cpu->BusErr(addr + 0xc00000, FALSE);
}

//---------------------------------------------------------------------------
//
//	ワード書き込み
//
//---------------------------------------------------------------------------
void FASTCALL GVRAM64K::WriteWord(DWORD addr, DWORD data)
{
	ASSERT(this);
	ASSERT(addr <= 0x1fffff);
	ASSERT((addr & 1) == 0);
	ASSERT(data < 0x10000);

	if (addr < 0x80000) {
		if (*(WORD*)(&gvram[addr]) != data) {
			*(WORD*)(&gvram[addr]) = (WORD)data;
			render->GrpMem(addr, 0);
		}
		return;
	}

	// バスエラー
	cpu->BusErr(addr + 0xc00000, FALSE);
}

//---------------------------------------------------------------------------
//
//	読み込みのみ
//
//---------------------------------------------------------------------------
DWORD FASTCALL GVRAM64K::ReadOnly(DWORD addr) const
{
	ASSERT(this);
	ASSERT(addr <= 0x1fffff);

	if (addr < 0x80000) {
		return gvram[addr ^ 1];
	}

	// バスエラー
	return 0xff;
}

//===========================================================================
//
//	グラフィックVRAM
//
//===========================================================================

//---------------------------------------------------------------------------
//
//	コンストラクタ
//
//---------------------------------------------------------------------------
GVRAM::GVRAM(VM *p) : MemDevice(p)
{
	// デバイスIDを初期化
	dev.id = MAKEID('G', 'V', 'R', 'M');
	dev.desc = "Graphic VRAM";

	// 開始アドレス、終了アドレス
	memdev.first = 0xc00000;
	memdev.last = 0xdfffff;

	// ワークエリア
	gvram = NULL;
	render = NULL;

	// ハンドラ
	handler = NULL;
	hand1024 = NULL;
	hand16 = NULL;
	hand256 = NULL;
	handNDef = NULL;
	hand64K = NULL;
}

//---------------------------------------------------------------------------
//
//	初期化
//
//---------------------------------------------------------------------------
BOOL FASTCALL GVRAM::Init()
{
	ASSERT(this);

	// 基本クラス
	if (!MemDevice::Init()) {
		return FALSE;
	}

	// メモリ確保、クリア
	try {
		gvram = new BYTE[ 0x80000 ];
	}
	catch (...) {
		return FALSE;
	}
	if (!gvram) {
		return FALSE;
	}
	memset(gvram, 0, 0x80000);

	// レンダラ取得
	render = (Render*)vm->SearchDevice(MAKEID('R', 'E', 'N', 'D'));
	ASSERT(render);

	// ハンドラ作成
	hand1024 = new GVRAM1024(render, gvram, cpu);
	hand16 = new GVRAM16(render, gvram, cpu);
	hand256 = new GVRAM256(render, gvram, cpu);
	handNDef = new GVRAMNDef(render, gvram, cpu);
	hand64K = new GVRAM64K(render, gvram, cpu);

	// データ初期化
	gvdata.mem = TRUE;
	gvdata.siz = 0;
	gvdata.col = 3;
	gvcount = 0;

	// ハンドラ初期化(64K)
	gvdata.type = 4;
	handler = hand64K;

	// 高速クリアマスク
	gvdata.mask[0] = 0xfff0;
	gvdata.mask[1] = 0xff0f;
	gvdata.mask[2] = 0xf0ff;
	gvdata.mask[3] = 0x0fff;

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	クリーンアップ
//
//---------------------------------------------------------------------------
void FASTCALL GVRAM::Cleanup()
{
	ASSERT(this);

	// ハンドラ解放
	if (hand64K) {
		delete hand64K;
		hand64K = NULL;
	}
	if (handNDef) {
		delete handNDef;
		handNDef = NULL;
	}
	if (hand256) {
		delete hand256;
		hand256 = NULL;
	}
	if (hand16) {
		delete hand16;
		hand16 = NULL;
	}
	if (hand1024) {
		delete hand1024;
		hand1024 = NULL;
	}
	handler = NULL;

	// メモリ解放
	if (gvram) {
		delete[] gvram;
		gvram = NULL;
	}

	// 基本クラスへ
	MemDevice::Cleanup();
}

//---------------------------------------------------------------------------
//
//	リセット
//
//---------------------------------------------------------------------------
void FASTCALL GVRAM::Reset()
{
	ASSERT(this);
	ASSERT_DIAG();

	LOG0(Log::Normal, "リセット");

	// 高速クリアなし
	gvdata.plane[0] = FALSE;
	gvdata.plane[1] = FALSE;
	gvdata.plane[2] = FALSE;
	gvdata.plane[3] = FALSE;

	// ハンドラ初期化(64K)
	gvdata.mem = TRUE;
	gvdata.siz = 0;
	gvdata.col = 3;
	gvdata.type = 4;
	handler = hand64K;

	// アクセスカウント0
	gvcount = 0;
}

//---------------------------------------------------------------------------
//
//	セーブ
//
//---------------------------------------------------------------------------
BOOL FASTCALL GVRAM::Save(Fileio *fio, int /*ver*/)
{
	size_t sz;

	ASSERT(this);
	ASSERT(fio);
	ASSERT_DIAG();

	LOG0(Log::Normal, "セーブ");

	// メモリをセーブ
	if (!fio->Write(gvram, 0x80000)) {
		return FALSE;
	}

	// サイズをセーブ
	sz = sizeof(gvram_t);
	if (!fio->Write(&sz, sizeof(sz))) {
		return FALSE;
	}

	// 実体をセーブ
	if (!fio->Write(&gvdata, (int)sz)) {
		return FALSE;
	}

	// gvcount(version2.04で追加)
	if (!fio->Write(&gvcount, sizeof(gvcount))) {
		return FALSE;
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	ロード
//
//---------------------------------------------------------------------------
BOOL FASTCALL GVRAM::Load(Fileio *fio, int ver)
{
	size_t sz;
	int line;

	ASSERT(this);
	ASSERT(fio);
	ASSERT(ver >= 0x0200);
	ASSERT_DIAG();

	LOG0(Log::Normal, "ロード");

	// メモリをロード
	if (!fio->Read(gvram, 0x80000)) {
		return FALSE;
	}

	// サイズをロード、照合
	if (!fio->Read(&sz, sizeof(sz))) {
		return FALSE;
	}
	if (sz != sizeof(gvram_t)) {
		return FALSE;
	}

	// 実体をロード
	if (!fio->Read(&gvdata, (int)sz)) {
		return FALSE;
	}

	// gvcount(version2.04で追加)
	gvcount = 0;
	if (ver >= 0x0204) {
		if (!fio->Read(&gvcount, sizeof(gvcount))) {
			return FALSE;
		}
	}

	// レンダラへ通知
	for (line=0; line<0x200; line++) {
		render->GrpAll(line, 0);
		render->GrpAll(line, 1);
		render->GrpAll(line, 2);
		render->GrpAll(line, 3);
	}

	// ハンドラを選択
	switch (gvdata.type) {
		case 0:
			ASSERT(hand1024);
			handler = hand1024;
			break;
		// 16色タイプ
		case 1:
			ASSERT(hand16);
			handler = hand16;
			break;
		// 256色タイプ
		case 2:
			ASSERT(hand256);
			handler = hand256;
			break;
		// 未定義タイプ
		case 3:
			ASSERT(handNDef);
			handler = handNDef;
			break;
		// 64K色タイプ
		case 4:
			ASSERT(hand64K);
			handler = hand64K;
			break;
		// その他
		default:
			ASSERT(FALSE);
	}

	return TRUE;
}

//---------------------------------------------------------------------------
//
//	設定適用
//
//---------------------------------------------------------------------------
void FASTCALL GVRAM::ApplyCfg(const Config* /*config*/)
{
	ASSERT(this);
	ASSERT_DIAG();

	LOG0(Log::Normal, "設定適用");
}

#if !defined(NDEBUG)
//---------------------------------------------------------------------------
//
//	診断
//
//---------------------------------------------------------------------------
void FASTCALL GVRAM::AssertDiag() const
{
	// 基本クラス
	MemDevice::AssertDiag();

	ASSERT(this);
	ASSERT(GetID() == MAKEID('G', 'V', 'R', 'M'));
	ASSERT(memdev.first == 0xc00000);
	ASSERT(memdev.last == 0xdfffff);
	ASSERT(gvram);
	ASSERT(hand1024);
	ASSERT(hand16);
	ASSERT(hand256);
	ASSERT(handNDef);
	ASSERT(hand64K);
	ASSERT(handler);
	ASSERT((gvdata.mem == TRUE) || (gvdata.mem == FALSE));
	ASSERT((gvdata.siz == 0) || (gvdata.siz == 1));
	ASSERT((gvdata.col >= 0) && (gvdata.col <= 3));
	ASSERT((gvdata.type >= 0) && (gvdata.type <= 4));
	ASSERT((gvcount == 0) || (gvcount == 1));
}
#endif	// NDEBUG

//---------------------------------------------------------------------------
//
//	バイト読み込み
//
//---------------------------------------------------------------------------
DWORD FASTCALL GVRAM::ReadByte(DWORD addr)
{
	ASSERT(this);
	ASSERT((addr >= memdev.first) && (addr <= memdev.last));
	ASSERT_DIAG();

	// ウェイト(0.5ウェイト)
	scheduler->Wait(gvcount);
	gvcount ^= 1;

	// ハンドラに任せる
	return handler->ReadByte(addr & 0x1fffff);
}

//---------------------------------------------------------------------------
//
//	ワード読み込み
//
//---------------------------------------------------------------------------
DWORD FASTCALL GVRAM::ReadWord(DWORD addr)
{
	ASSERT(this);
	ASSERT((addr >= memdev.first) && (addr <= memdev.last));
	ASSERT((addr & 1) == 0);
	ASSERT_DIAG();

	// ウェイト(0.5ウェイト)
	scheduler->Wait(gvcount);
	gvcount ^= 1;

	// ハンドラに任せる
	return handler->ReadWord(addr & 0x1fffff);
}

//---------------------------------------------------------------------------
//
//	バイト書き込み
//
//---------------------------------------------------------------------------
void FASTCALL GVRAM::WriteByte(DWORD addr, DWORD data)
{
	ASSERT(this);
	ASSERT((addr >= memdev.first) && (addr <= memdev.last));
	ASSERT(data < 0x100);
	ASSERT_DIAG();

	// ウェイト(0.5ウェイト)
	scheduler->Wait(gvcount);
	gvcount ^= 1;

	// ハンドラに任せる
	handler->WriteByte(addr & 0x1fffff, data);
}

//---------------------------------------------------------------------------
//
//	ワード書き込み
//
//---------------------------------------------------------------------------
void FASTCALL GVRAM::WriteWord(DWORD addr, DWORD data)
{
	ASSERT(this);
	ASSERT((addr >= memdev.first) && (addr <= memdev.last));
	ASSERT((addr & 1) == 0);
	ASSERT(data < 0x10000);
	ASSERT_DIAG();

	// ウェイト(0.5ウェイト)
	scheduler->Wait(gvcount);
	gvcount ^= 1;

	// ハンドラに任せる
	handler->WriteWord(addr & 0x1fffff, data);
}

//---------------------------------------------------------------------------
//
//	読み込みのみ
//
//---------------------------------------------------------------------------
DWORD FASTCALL GVRAM::ReadOnly(DWORD addr) const
{
	ASSERT(this);
	ASSERT((addr >= memdev.first) && (addr <= memdev.last));
	ASSERT_DIAG();

	// ハンドラに任せる
	return handler->ReadOnly(addr & 0x1fffff);
}

//---------------------------------------------------------------------------
//
//	GVRAM取得
//
//---------------------------------------------------------------------------
const BYTE* FASTCALL GVRAM::GetGVRAM() const
{
	ASSERT(this);
	ASSERT_DIAG();

	return gvram;
}

//---------------------------------------------------------------------------
//
//	タイプ設定
//
//---------------------------------------------------------------------------
void FASTCALL GVRAM::SetType(DWORD type)
{
	int next;
	int prev;
	BOOL mem;
	DWORD siz;

	ASSERT(this);
	ASSERT_DIAG();

	// 現在の値を保存
	mem = gvdata.mem;
	siz = gvdata.siz;

	// 設定
	if (type & 8) {
		gvdata.mem = TRUE;
	}
	else {
		gvdata.mem = FALSE;
	}
	if (type & 4) {
		gvdata.siz = 1;
	}
	else {
		gvdata.siz = 0;
	}
	gvdata.col = type & 3;

	// 現在のgvdata.typeを記憶
	prev = gvdata.type;

	// 新しいtypeを見る
	if (gvdata.mem) {
		next = 4;
	}
	else {
		if (gvdata.siz) {
			next = 0;
		}
		else {
			next = gvdata.col + 1;
		}
	}

	// 違っていたら作りなおす
	if (prev != next) {
		switch (next) {
			// 1024色タイプ
			case 0:
				ASSERT(hand1024);
				handler = hand1024;
				break;
			// 16色タイプ
			case 1:
				ASSERT(hand16);
				handler = hand16;
				break;
			// 256色タイプ
			case 2:
				ASSERT(hand256);
				handler = hand256;
				break;
			// 未定義タイプ
			case 3:
				LOG0(Log::Warning, "グラフィックVRAM 未定義タイプ");
				ASSERT(handNDef);
				handler = handNDef;
				break;
			// 64K色タイプ
			case 4:
				ASSERT(hand64K);
				handler = hand64K;
				break;
			// その他
			default:
				ASSERT(FALSE);
		}
		gvdata.type = next;
	}

	// メモリタイプ又は実画面サイズが異なっていれば、レンダラへ通知
	if ((gvdata.mem != mem) || (gvdata.siz != siz)) {
		render->SetVC();
	}
}

//---------------------------------------------------------------------------
//
//	高速クリア設定
//
//---------------------------------------------------------------------------
void FASTCALL GVRAM::FastSet(DWORD mask)
{
	ASSERT(this);
	ASSERT_DIAG();

#if defined(GVRAM_LOG)
	LOG1(Log::Normal, "高速クリアプレーン指定 %02X", mask);
#endif	// GVRAM_LOG

	if (mask & 0x08) {
		gvdata.plane[3] = TRUE;
	}
	else {
		gvdata.plane[3] = FALSE;
	}

	if (mask & 0x04) {
		gvdata.plane[2] = TRUE;
	}
	else {
		gvdata.plane[2] = FALSE;
	}

	if (mask & 0x02) {
		gvdata.plane[1] = TRUE;
	}
	else {
		gvdata.plane[1] = FALSE;
	}

	if (mask & 0x01) {
		gvdata.plane[0] = TRUE;
	}
	else {
		gvdata.plane[0] = FALSE;
	}
}

//---------------------------------------------------------------------------
//
//	高速クリア
//
//---------------------------------------------------------------------------
void FASTCALL GVRAM::FastClr(const CRTC::crtc_t *p)
{
	ASSERT(this);
	ASSERT_DIAG();

	if (gvdata.siz) {
		// 1024x1024
		if (p->hd >= 1) {
			// 1024×1024、512 or 768
			FastClr768(p);
		}
		else {
			// 1024×1024、256
			FastClr256(p);
		}
	}
	else {
		// 512×512
		FastClr512(p);
	}
}

//---------------------------------------------------------------------------
//
//	高速クリア 1024×1024 512/768
//
//---------------------------------------------------------------------------
void FASTCALL GVRAM::FastClr768(const CRTC::crtc_t *p)
{
	int y;
	int n;
	int j;
	int k;
	DWORD offset;
	WORD *q;

	ASSERT(this);
	ASSERT_DIAG();

#if defined(GVRAM_LOG)
	LOG0(Log::Normal, "高速クリア 1024x1024 (512/768幅)");
#endif	// GVRAM_LOG

	// オフセットy、ライン数nを得る
	y = p->v_scan;
	n = 1;
	if ((p->v_mul == 2) && !(p->lowres)) {
		if (y & 1) {
			return;
		}
		y >>= 1;
	}
	if (p->v_mul == 0) {
		y <<= 1;
		n = 2;
	}

	// ラインループ
	for (j=0; j<n; j++) {
		// スクロールレジスタからオフセットを得る
		offset = (y + p->grp_scrly[0]) & 0x3ff;

		// 上半分、下半分で分ける
		if (offset < 512) {
			// ポインタ作成
			q = (WORD*)&gvram[offset << 10];

			// 下位バイトクリア
			for (k=0; k<512; k++) {
				*q++ &= 0xff00;
			}

			// フラグUp
			render->GrpAll(offset, 0);
			render->GrpAll(offset, 1);
		}
		else {
			// ポインタ作成
			offset &= 0x1ff;
			q = (WORD*)&gvram[offset << 10];

			// 上位バイトクリア
			for (k=0; k<512; k++) {
				*q++ &= 0x00ff;
			}

			// フラグUp
			render->GrpAll(offset, 2);
			render->GrpAll(offset, 3);
		}

		// 次のラインへ
		y++;
	}
}

//---------------------------------------------------------------------------
//
//	高速クリア 1024×1024 256
//
//---------------------------------------------------------------------------
void FASTCALL GVRAM::FastClr256(const CRTC::crtc_t *p)
{
#if defined(GVRAM_LOG)
	LOG0(Log::Normal, "高速クリア 1024x1024 (256幅)");
#endif	// GVRAM_LOG

	// 暫定措置
	FastClr768(p);
}

//---------------------------------------------------------------------------
//
//	高速クリア 512x512
//
//---------------------------------------------------------------------------
void FASTCALL GVRAM::FastClr512(const CRTC::crtc_t *p)
{
	int y;
	int n;
	int i;
	int j;
	int k;
	int w[2];
	DWORD offset;
	WORD *q;

	ASSERT(this);
	ASSERT_DIAG();

#if defined(GVRAM_LOG)
	LOG1(Log::Normal, "高速クリア 512x512 Scan=%d", p->v_scan);
#endif	// GVRAM_LOG

	// オフセットy、ライン数nを得る
	y = p->v_scan;
	n = 1;
	if ((p->v_mul == 2) && !(p->lowres)) {
		if (y & 1) {
			return;
		}
		y >>= 1;
	}
	if (p->v_mul == 0) {
		y <<= 1;
		n = 2;
	}

	// プレーンループ
	for (i=0; i<4; i++) {
		if (!gvdata.plane[i]) {
			continue;
		}

		// 幅を算出
		w[0] = p->h_dots;
		w[1] = 0;
		if (((p->grp_scrlx[i] & 0x1ff) + w[0]) > 512) {
			w[1] = (p->grp_scrlx[i] & 0x1ff) + w[0] - 512;
			w[0] = 512 - (p->grp_scrlx[i] & 0x1ff);
		}

		// ラインループ
		for (j=0; j<n; j++) {
			// スクロールレジスタからオフセットを得る
			offset = ((y + p->grp_scrly[i]) & 0x1ff) << 10;
			q = (WORD*)&gvram[offset + ((p->grp_scrlx[i] & 0x1ff) << 1)];

			// クリア(1)
			for (k=0; k<w[0]; k++) {
				*q++ &= gvdata.mask[i];
			}
			if (w[1] > 0) {
				// クリア(2)
				q = (WORD*)&gvram[offset];
				for (k=0; k<w[1]; k++) {
					*q++ &= gvdata.mask[i];
				}
			}

			// フラグUp
			render->GrpAll(offset >> 10, i);

			// 次のラインへ
			y++;
		}

		// ライン戻す
		y -= n;
	}
}

