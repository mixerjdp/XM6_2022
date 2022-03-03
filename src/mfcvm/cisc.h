//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2003 ＰＩ．(ytanaka@ipc-tokai.or.jp)
//	[ FM音源ユニット接続 ]
//
//---------------------------------------------------------------------------

#if !defined(cisc_h)
#define cisc_h

#include <assert.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>

#if defined(__BORLANDC__)
#pragma warn -8004					// 代入した値は使われていない
#pragma warn -8012					// 符号つきと符号なしの比較
#pragma warn -8071					// 変換によって有効桁が失われる 
#endif	// __BORLANDC__

typedef unsigned char uchar;
typedef unsigned short ushort;
typedef unsigned int uint;
typedef unsigned long ulong;

typedef unsigned char uint8;
typedef unsigned short uint16;
typedef unsigned int  uint32;

typedef signed char sint8;
typedef signed short sint16;
typedef signed int sint32;

typedef signed char int8;
typedef signed short int16;
typedef signed int int32;

inline int Max(int x, int y) { return (x > y) ? x : y; }
inline int Min(int x, int y) { return (x < y) ? x : y; }
inline int Abs(int x) { return x >= 0 ? x : -x; }

inline int Limit(int v, int max, int min) 
{ 
	return v > max ? max : (v < min ? min : v); 
}

inline unsigned int BSwap(unsigned int a)
{
	return (a >> 24) | ((a >> 8) & 0xff00) | ((a << 8) & 0xff0000) | (a << 24);
}

inline unsigned int NtoBCD(unsigned int a)
{
	return ((a / 10) << 4) + (a % 10);
}

inline unsigned int BCDtoN(unsigned int v)
{
	return (v >> 4) * 10 + (v & 15);
}


template<class T>
inline T gcd(T x, T y)
{
	T t;
	while (y)
	{
		t = x % y;
		x = y;
		y = t;
	}
	return x;
}


template<class T>
T bessel0(T x)
{
	T p, r, s;

	r = 1.0;
	s = 1.0;
	p = (x / 2.0) / s;

	while (p > 1.0E-10)
	{
		r += p * p;
		s += 1.0;
		p *= (x / 2.0) / s;
	}
	return r;
}

#endif	// cisc_h
