/*
** Starscream 680x0 emulation library
** Copyright 1997, 1998, 1999 Neill Corlett
** XM6 version - Copyright 2001-2003 PI.
**
** Refer to STARDOC.TXT for terms of use, API reference, and directions on
** how to compile.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <assert.h>
#include "starcpu.h"

void (*cpudebug_get)(char*, int);
void (*cpudebug_put)(const char*);

typedef unsigned char BYTE;
typedef unsigned short WORD;
typedef unsigned int DWORD;

static char out_buffer[0x100];
static int out_count;

static void cpudebug_putc(char c)
{
	if (c >= 0x60) {
		c -= (char)0x20;
	}
	out_buffer[out_count++] = c;

	if (c == '\n') {
		out_buffer[out_count] = '\0';
		cpudebug_put(out_buffer);
		out_count = 0;
	}
}

static void cpudebug_printf(const char *fmt, ...) {
	static char buffer[400];
	char *s = buffer;
	va_list ap;
	va_start(ap, fmt);
	vsprintf(s, fmt, ap);
	va_end(ap);
	while(*s) cpudebug_putc(*s++);
}

#define byte unsigned char
#define word unsigned short int
#define dword unsigned int

#define int08 signed char
#define int16 signed short int
#define int32 signed int

int cpudebug_disabled(void){return 0;}

#define ea eacalc[inst&0x3F]()

/******************************
** SNAGS / EA CONSIDERATIONS **
*******************************

- ADDX is encoded the same way as ADD->ea
- SUBX is encoded the same way as SUB->ea
- ABCD is encoded the same way as AND->ea
- EXG is encoded the same way as AND->ea
- SBCD is encoded the same way as OR->ea
- EOR is encoded the same way as CMPM
- ASR is encoded the same way as LSR
  (so are LSL and ASL, but they do the same thing)
- Bcc does NOT support 32-bit offsets on the 68000.
  (this is a reminder, don't bother implementing 32-bit offsets!)
- Look on p. 3-19 for how to calculate branch conditions (GE, LT, GT, etc.)
- Bit operations are 32-bit for registers, 8-bit for memory locations.
- MOVEM->memory is encoded the same way as EXT
  If the EA is just a register, then it's EXT, otherwise it's MOVEM.
- MOVEP done the same way as the bit operations
- Scc done the same way as DBcc.
  Assume it's Scc, unless the EA is a direct An mode (then it's a DBcc).
- SWAP done the same way as PEA
- TAS done the same way as ILLEGAL

- LINK, NOP, RTR, RTS, TRAP, TRAPV, UNLK are encoded the same way.

******************************/

#define hex08 "%02x"
#define hex16 "%04x"
#define hex32 "%08x"
#define hexlong "%06x"

#define isregister ((inst&0x0030)==0x0000)
#define isaddressr ((inst&0x0038)==0x0008)

static char eabuffer[20],sdebug[80];
dword debugpc,hexaddr;
static int isize;
static word inst;

typedef struct _x68func {
	unsigned short code;
	const char *name;
} x68func_t;

word cpudebug_fetch(dword);

static WORD fetch(void)
{
	debugpc += 2;
	isize += 2;
	return cpudebug_fetch(debugpc - 2);
}

static DWORD fetchl(void)
{
	DWORD t;

	t = cpudebug_fetch(debugpc);
	t <<= 16;
	t |= cpudebug_fetch(debugpc + 2);
	debugpc += 4;
	isize += 4;
	return t;
}

static int08 opsize[1024]={
1,2,4,0,0,0,0,0,1,2,4,0,0,0,0,0,1,2,4,0,0,0,0,0,1,2,4,0,0,0,0,0,
0,0,0,0,0,0,0,0,1,2,4,0,0,0,0,0,1,2,4,0,0,0,0,0,0,0,0,0,0,0,0,0,
1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,4,
2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,
1,2,4,2,0,0,2,4,1,2,4,1,0,0,2,4,1,2,4,1,0,0,2,4,1,2,4,2,0,0,2,4,
1,4,2,4,0,0,2,4,1,2,4,1,0,0,2,4,0,0,2,4,0,0,2,4,0,2,0,0,0,0,2,4,
1,2,4,0,1,2,4,0,1,2,4,0,1,2,4,0,1,2,4,0,1,2,4,0,1,2,4,0,1,2,4,0,
1,2,4,0,1,2,4,0,1,2,4,0,1,2,4,0,1,2,4,0,1,2,4,0,1,2,4,0,1,2,4,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
4,4,4,4,0,0,0,0,4,4,4,4,0,0,0,0,4,4,4,4,0,0,0,0,4,4,4,4,0,0,0,0,
4,4,4,4,0,0,0,0,4,4,4,4,0,0,0,0,4,4,4,4,0,0,0,0,4,4,4,4,0,0,0,0,
1,2,4,2,1,2,4,2,1,2,4,2,1,2,4,2,1,2,4,2,1,2,4,2,1,2,4,2,1,2,4,2,
1,2,4,2,1,2,4,2,1,2,4,2,1,2,4,2,1,2,4,2,1,2,4,2,1,2,4,2,1,2,4,2,
1,2,4,2,1,2,4,4,1,2,4,2,1,2,4,4,1,2,4,2,1,2,4,4,1,2,4,2,1,2,4,4,
1,2,4,2,1,2,4,4,1,2,4,2,1,2,4,4,1,2,4,2,1,2,4,4,1,2,4,2,1,2,4,4,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
1,2,4,2,1,2,4,4,1,2,4,2,1,2,4,4,1,2,4,2,1,2,4,4,1,2,4,2,1,2,4,4,
1,2,4,2,1,2,4,4,1,2,4,2,1,2,4,4,1,2,4,2,1,2,4,4,1,2,4,2,1,2,4,4,
1,2,4,2,1,2,4,2,1,2,4,2,1,2,4,2,1,2,4,2,1,2,4,2,1,2,4,2,1,2,4,2,
1,2,4,2,1,2,4,2,1,2,4,2,1,2,4,2,1,2,4,2,1,2,4,2,1,2,4,2,1,2,4,2,
1,2,4,2,1,2,4,4,1,2,4,2,1,2,4,4,1,2,4,2,1,2,4,4,1,2,4,2,1,2,4,4,
1,2,4,2,1,2,4,4,1,2,4,2,1,2,4,4,1,2,4,2,1,2,4,4,1,2,4,2,1,2,4,4,
1,2,4,2,1,2,4,2,1,2,4,2,1,2,4,2,1,2,4,2,1,2,4,2,1,2,4,2,1,2,4,2,
1,2,4,0,1,2,4,0,1,2,4,0,1,2,4,0,1,2,4,0,1,2,4,0,1,2,4,0,1,2,4,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

/******************** EA GENERATION ********************/

/* These are the functions to generate effective addresses.
   Here is the syntax:

   eacalc[mode]();

   mode   is the EA mode << 3 + register number

   The function sets addr so you can use m68read and m68write.

   These routines screw around with the PC, and might call fetch() a couple
   times to get extension words.  Place the eacalc() calls strategically to
   get the extension words in the right order.
*/

/* These are the jump tables for EA calculation.
   drd     = data reg direct                         Dn
   ard     = address reg direct                      An
   ari     = address reg indirect                   (An)
   ari_inc = address reg indirect, postincrement    (An)+
   ari_dec = address reg indirect, predecrement    -(An)
   ari_dis = address reg indirect, displacement     (d16,An)
   ari_ind = address reg indirect, index            (d8,An,Xn)
   pci_dis = prog. counter indirect, displacement   (d16,PC)
   pci_ind = prog. counter indirect, index          (d8,PC,Xn)
*/

#define eacalc_drd(n,r) static void n(void){sprintf(eabuffer,"d%d",r);}
#define eacalc_ard(n,r) static void n(void){sprintf(eabuffer,"a%d",r);}
#define eacalc_ari(n,r) static void n(void){sprintf(eabuffer,"(a%d)",r);}
#define eacalc_ari_inc(n,r) static void n(void){sprintf(eabuffer,"(a%d)+",r);}
#define eacalc_ari_dec(n,r) static void n(void){sprintf(eabuffer,"-(a%d)",r);}
#define eacalc_ari_dis(n,r) static void n(void){\
	int16 briefext=fetch();\
	if(briefext<0)sprintf(eabuffer,"-$" hex16 "(A%d)",-briefext,r);else \
	sprintf(eabuffer,"$" hex16 "(A%d)",briefext,r);\
}
#define eacalc_ari_ind(n,r) static void n(void){\
	int16 briefext=fetch();\
	if(briefext & 0x80)sprintf(eabuffer,"-$" hex08 "(a%d,%c%d.%c)",\
		(int08)(128-(briefext&0x7f)),r,\
		briefext&0x8000?'a':'d',(briefext>>12)&7,\
		briefext&0x800?'l':'w');\
	else sprintf(eabuffer,"$" hex08 "(a%d,%c%d.%c)",\
		(int08)briefext,r,\
		briefext&0x8000?'a':'d',(briefext>>12)&7,\
		briefext&0x800?'l':'w');\
}
eacalc_drd(ea_0_0,0)
eacalc_drd(ea_0_1,1)
eacalc_drd(ea_0_2,2)
eacalc_drd(ea_0_3,3)
eacalc_drd(ea_0_4,4)
eacalc_drd(ea_0_5,5)
eacalc_drd(ea_0_6,6)
eacalc_drd(ea_0_7,7)
eacalc_ard(ea_1_0,0)
eacalc_ard(ea_1_1,1)
eacalc_ard(ea_1_2,2)
eacalc_ard(ea_1_3,3)
eacalc_ard(ea_1_4,4)
eacalc_ard(ea_1_5,5)
eacalc_ard(ea_1_6,6)
eacalc_ard(ea_1_7,7)
eacalc_ari(ea_2_0,0)
eacalc_ari(ea_2_1,1)
eacalc_ari(ea_2_2,2)
eacalc_ari(ea_2_3,3)
eacalc_ari(ea_2_4,4)
eacalc_ari(ea_2_5,5)
eacalc_ari(ea_2_6,6)
eacalc_ari(ea_2_7,7)
eacalc_ari_inc(ea_3_0,0)
eacalc_ari_inc(ea_3_1,1)
eacalc_ari_inc(ea_3_2,2)
eacalc_ari_inc(ea_3_3,3)
eacalc_ari_inc(ea_3_4,4)
eacalc_ari_inc(ea_3_5,5)
eacalc_ari_inc(ea_3_6,6)
eacalc_ari_inc(ea_3_7,7)
eacalc_ari_dec(ea_4_0,0)
eacalc_ari_dec(ea_4_1,1)
eacalc_ari_dec(ea_4_2,2)
eacalc_ari_dec(ea_4_3,3)
eacalc_ari_dec(ea_4_4,4)
eacalc_ari_dec(ea_4_5,5)
eacalc_ari_dec(ea_4_6,6)
eacalc_ari_dec(ea_4_7,7)
eacalc_ari_dis(ea_5_0,0)
eacalc_ari_dis(ea_5_1,1)
eacalc_ari_dis(ea_5_2,2)
eacalc_ari_dis(ea_5_3,3)
eacalc_ari_dis(ea_5_4,4)
eacalc_ari_dis(ea_5_5,5)
eacalc_ari_dis(ea_5_6,6)
eacalc_ari_dis(ea_5_7,7)
eacalc_ari_ind(ea_6_0,0)
eacalc_ari_ind(ea_6_1,1)
eacalc_ari_ind(ea_6_2,2)
eacalc_ari_ind(ea_6_3,3)
eacalc_ari_ind(ea_6_4,4)
eacalc_ari_ind(ea_6_5,5)
eacalc_ari_ind(ea_6_6,6)
eacalc_ari_ind(ea_6_7,7)

/* These are the "special" addressing modes:
   abshort    absolute short address
   abslong    absolute long address
   immdata    immediate data
*/

static void eacalcspecial_abshort(void){
	word briefext;
	briefext=fetch();
	sprintf(eabuffer,"$" hex16 "",briefext);
}

static void eacalcspecial_abslong(void){
	dword briefext;
	briefext=fetch();
	briefext<<=16;
	briefext|=fetch();
	sprintf(eabuffer,"$"hexlong"",briefext);
}

static void eacalcspecial_immdata(void){
	dword briefext;
	switch(opsize[inst>>6]){
		case 1:
			briefext=fetch()&0xFF;
			sprintf(eabuffer,"#$" hex08,briefext);
			break;
		case 2:
			briefext=fetch();
			sprintf(eabuffer,"#$" hex16,briefext);
			break;
		default:
			briefext=fetch();
			briefext<<=16;
			briefext|=fetch();
			sprintf(eabuffer,"#$" hex32,briefext);
			break;
	}
}

static void eacalcspecial_pci_dis(void){
	dword dpc = debugpc;
	word briefext = fetch();
	sprintf(eabuffer,"$" hexlong "(pc)",((int16)(briefext))+dpc);
}

static void eacalcspecial_pci_ind(void){
	dword dpc = debugpc;
	word briefext = fetch();

	if (briefext & 0x80) {
		sprintf(eabuffer,"$" hexlong "(pc,%c%d)",
			dpc-(128-(briefext&0x7f)),
			briefext&0x8000?'a':'d',(briefext>>12)&7);
	}
	else {
		sprintf(eabuffer,"$" hexlong "(pc,%c%d)",
			((int08)(briefext))+dpc,
			briefext&0x8000?'a':'d',(briefext>>12)&7);
	}
}

static void eacalcspecial_unknown(void){
	sprintf(eabuffer,"(UNKNOWN EA MODE)");
}

static void (*(eacalc[64]))(void)={
ea_0_0,ea_0_1,ea_0_2,ea_0_3,ea_0_4,ea_0_5,ea_0_6,ea_0_7,
ea_1_0,ea_1_1,ea_1_2,ea_1_3,ea_1_4,ea_1_5,ea_1_6,ea_1_7,
ea_2_0,ea_2_1,ea_2_2,ea_2_3,ea_2_4,ea_2_5,ea_2_6,ea_2_7,
ea_3_0,ea_3_1,ea_3_2,ea_3_3,ea_3_4,ea_3_5,ea_3_6,ea_3_7,
ea_4_0,ea_4_1,ea_4_2,ea_4_3,ea_4_4,ea_4_5,ea_4_6,ea_4_7,
ea_5_0,ea_5_1,ea_5_2,ea_5_3,ea_5_4,ea_5_5,ea_5_6,ea_5_7,
ea_6_0,ea_6_1,ea_6_2,ea_6_3,ea_6_4,ea_6_5,ea_6_6,ea_6_7,
eacalcspecial_abshort,eacalcspecial_abslong,eacalcspecial_pci_dis,eacalcspecial_pci_ind,
eacalcspecial_immdata,eacalcspecial_unknown,eacalcspecial_unknown,eacalcspecial_unknown};

static void m68unsupported(void){
	sprintf(sdebug,"(NOT RECOGNIZED)");
}

static void m68_unrecog_x(void){
	sprintf(sdebug,"(unrecognized)");
}

/******************** BIT TEST-AND-____ ********************/

static void m68_bitopdn_x(void){
	int16 d16;
	if((inst&0x38)==0x08){
		d16=fetch();
		sprintf(eabuffer,"%c$" hex16 "(a%d)",(d16<0)?'-':'+',(d16<0)?-d16:d16,inst&7);
		if(!(inst&0x80)){
			sprintf(sdebug,"movep%s %s,d%d",
				((inst&0x40)==0x40)?".l":"  ",
				eabuffer,inst>>9
			);
		}else{
			sprintf(sdebug,"movep%s d%d,%s",
				((inst&0x40)==0x40)?".l":"  ",
				inst>>9,eabuffer
			);
		}
	}else{
		ea;
		switch((inst>>6)&3){
		case 0:sprintf(sdebug,"btst.l  d%d,%s",inst>>9,eabuffer);break;
		case 1:sprintf(sdebug,"bchg.l  d%d,%s",inst>>9,eabuffer);break;
		case 2:sprintf(sdebug,"bclr.l  d%d,%s",inst>>9,eabuffer);break;
		case 3:sprintf(sdebug,"bset.l  d%d,%s",inst>>9,eabuffer);break;
		default:break;
		}
	}
}

#define bittest_st(name,dump) static void name(void)\
{\
	BYTE s = (BYTE)fetch();\
	ea;\
	if ((inst & 0x3f) < 0x08) sprintf(sdebug, "%s.l  #$%02x,%s", dump, s, eabuffer);\
	else sprintf(sdebug,"%s.b  #$%02x,%s",dump, s, eabuffer);\
}
bittest_st(m68_btst_st_x, "btst")
bittest_st(m68_bclr_st_x, "bclr")
bittest_st(m68_bset_st_x, "bset")
bittest_st(m68_bchg_st_x, "bchg")

/******************** Bcc ********************/

#define conditional_branch(name,dump) static void name(void){\
	int16 disp;\
	int32 currentpc=debugpc;\
	disp=(int08)(inst&0xFF);\
	if(!disp)disp=fetch();\
	sprintf(sdebug,"%s     $"hexlong"",dump,currentpc+disp);\
}

conditional_branch(m68_bra_____x,"bra")
conditional_branch(m68_bhi_____x,"bhi")
conditional_branch(m68_bls_____x,"bls")
conditional_branch(m68_bcc_____x,"bcc")
conditional_branch(m68_bcs_____x,"bcs")
conditional_branch(m68_bne_____x,"bne")
conditional_branch(m68_beq_____x,"beq")
conditional_branch(m68_bvc_____x,"bvc")
conditional_branch(m68_bvs_____x,"bvs")
conditional_branch(m68_bpl_____x,"bpl")
conditional_branch(m68_bmi_____x,"bmi")
conditional_branch(m68_bge_____x,"bge")
conditional_branch(m68_blt_____x,"blt")
conditional_branch(m68_bgt_____x,"bgt")
conditional_branch(m68_ble_____x,"ble")

/******************** Scc, DBcc ********************/

#define scc_dbcc(name,dump1,dump2)\
static void name(void){\
	ea;if(isaddressr){\
		int16 disp;\
		disp=fetch();\
		sprintf(sdebug,"%s  d%d,$"hexlong"",dump2,inst&7,debugpc+disp-2);\
	}else sprintf(sdebug,"%s     %s",dump1,eabuffer);\
}

scc_dbcc(m68_st______x,"st ","dbt.w ")
scc_dbcc(m68_sf______x,"sf ","dbra.w")
scc_dbcc(m68_shi_____x,"shi","dbhi.w")
scc_dbcc(m68_sls_____x,"sls","dbls.w")
scc_dbcc(m68_scc_____x,"scc","dbcc.w")
scc_dbcc(m68_scs_____x,"scs","dbcs.w")
scc_dbcc(m68_sne_____x,"sne","dbne.w")
scc_dbcc(m68_seq_____x,"seq","dbeq.w")
scc_dbcc(m68_svc_____x,"svc","dbvc.w")
scc_dbcc(m68_svs_____x,"svs","dbvs.w")
scc_dbcc(m68_spl_____x,"spl","dbpl.w")
scc_dbcc(m68_smi_____x,"smi","dbmi.w")
scc_dbcc(m68_sge_____x,"sge","dbge.w")
scc_dbcc(m68_slt_____x,"slt","dblt.w")
scc_dbcc(m68_sgt_____x,"sgt","dbgt.w")
scc_dbcc(m68_sle_____x,"sle","dble.w")

/******************** JMP ********************/

static void m68_jmp_____x(void){ea;sprintf(sdebug,"jmp     %s",eabuffer);}

/******************** JSR/BSR ********************/

static void m68_jsr_____x(void){ea;sprintf(sdebug,"jsr     %s",eabuffer);}

static void m68_bsr_____x(void){
	int16 disp;
	int32 currentpc=debugpc;
	disp=(int08)(inst&0xFF);
	if(!disp)disp=fetch();
	sprintf(sdebug,"%s     $"hexlong"","bsr",disp+currentpc);
}

/******************** TAS ********************/

/* Test-and-set / illegal */
static void m68_tas_____b(void){
	if(inst==0x4AFC){
		sprintf(sdebug,"illegal");
	}else{
		ea;
		sprintf(sdebug,"tas     %s",eabuffer);
	}
}

/******************** LEA ********************/

static void m68_lea___n_l(void){
	ea;sprintf(sdebug,"lea.l   %s,a%d",eabuffer,(inst>>9)&7);
}

/******************** PEA ********************/

static void m68_pea_____l(void){
	ea;if(isregister){/* SWAP Dn */
		sprintf(sdebug,"swap.w  d%d",inst&7);
	}else{
		sprintf(sdebug,"pea.l   %s",eabuffer);
	}
}

/******************** CLR, NEG, NEGX, NOT, TST ********************/

#define negate_ea(name,dump) static void name(void){\
	ea;sprintf(sdebug,"%s  %s",dump,eabuffer);\
}

negate_ea(m68_neg_____b,"neg.b ")
negate_ea(m68_neg_____w,"neg.w ")
negate_ea(m68_neg_____l,"neg.l ")
negate_ea(m68_negx____b,"negx.b")
negate_ea(m68_negx____w,"negx.w")
negate_ea(m68_negx____l,"negx.l")
negate_ea(m68_not_____b,"not.b ")
negate_ea(m68_not_____w,"not.w ")
negate_ea(m68_not_____l,"not.l ")
negate_ea(m68_clr_____b,"clr.b ")
negate_ea(m68_clr_____w,"clr.w ")
negate_ea(m68_clr_____l,"clr.l ")
negate_ea(m68_tst_____b,"tst.b ")
negate_ea(m68_tst_____w,"tst.w ")
negate_ea(m68_tst_____l,"tst.l ")

/********************      SOURCE: IMMEDIATE DATA
********************* DESTINATION: EFFECTIVE ADDRESS
********************/

#define im_to_ea(name,type,hextype,fetchtype,dump,r) static void name(void){\
	type src=(type)fetchtype();\
	if((inst&0x3F)==0x3C){\
	sprintf(sdebug,"%s   #$"hextype",%s",dump,src,r);\
	}else{\
	ea;sprintf(sdebug,"%s   #$"hextype",%s",dump,src,eabuffer);\
	}\
}

im_to_ea(m68_ori_____b,byte ,hex08,fetch ,"or.b ","ccr")
im_to_ea(m68_ori_____w,word ,hex16,fetch ,"or.w ","sr" )
im_to_ea(m68_ori_____l,dword,hex32,fetchl,"or.l ",""   )
im_to_ea(m68_andi____b,byte ,hex08,fetch ,"and.b","ccr")
im_to_ea(m68_andi____w,word ,hex16,fetch ,"and.w","sr" )
im_to_ea(m68_andi____l,dword,hex32,fetchl,"and.l",""   )
im_to_ea(m68_eori____b,byte ,hex08,fetch ,"eor.b","ccr")
im_to_ea(m68_eori____w,word ,hex16,fetch ,"eor.w","sr" )
im_to_ea(m68_eori____l,dword,hex32,fetchl,"eor.l",""   )
im_to_ea(m68_addi____b,byte ,hex08,fetch ,"add.b",""   )
im_to_ea(m68_addi____w,word ,hex16,fetch ,"add.w",""   )
im_to_ea(m68_addi____l,dword,hex32,fetchl,"add.l",""   )
im_to_ea(m68_subi____b,byte ,hex08,fetch ,"sub.b",""   )
im_to_ea(m68_subi____w,word ,hex16,fetch ,"sub.w",""   )
im_to_ea(m68_subi____l,dword,hex32,fetchl,"sub.l",""   )
im_to_ea(m68_cmpi____b,byte ,hex08,fetch ,"cmp.b",""   )
im_to_ea(m68_cmpi____w,word ,hex16,fetch ,"cmp.w",""   )
im_to_ea(m68_cmpi____l,dword,hex32,fetchl,"cmp.l",""   )

/********************      SOURCE: EFFECTIVE ADDRESS
********************* DESTINATION: DATA REGISTER
********************/

#define ea_to_dn(name,dump) static void name(void){\
	ea;sprintf(sdebug,"%s   %s,d%d",dump,eabuffer,(inst>>9)&7);\
}

ea_to_dn(m68_or__d_n_b,"or.b ")
ea_to_dn(m68_or__d_n_w,"or.w ")
ea_to_dn(m68_or__d_n_l,"or.l ")
ea_to_dn(m68_and_d_n_b,"and.b")
ea_to_dn(m68_and_d_n_w,"and.w")
ea_to_dn(m68_and_d_n_l,"and.l")
ea_to_dn(m68_add_d_n_b,"add.b")
ea_to_dn(m68_add_d_n_w,"add.w")
ea_to_dn(m68_add_d_n_l,"add.l")
ea_to_dn(m68_sub_d_n_b,"sub.b")
ea_to_dn(m68_sub_d_n_w,"sub.w")
ea_to_dn(m68_sub_d_n_l,"sub.l")
ea_to_dn(m68_cmp_d_n_b,"cmp.b")
ea_to_dn(m68_cmp_d_n_w,"cmp.w")
ea_to_dn(m68_cmp_d_n_l,"cmp.l")

/********************      SOURCE: EFFECTIVE ADDRESS
********************* DESTINATION: ADDRESS REGISTER
********************/

#define ea_to_an(name,dump) static void name(void){\
	ea;sprintf(sdebug,"%s   %s,a%d",dump,eabuffer,((inst>>9)&7));\
}

ea_to_an(m68_adda__n_w,"add.w")
ea_to_an(m68_adda__n_l,"add.l")
ea_to_an(m68_suba__n_w,"sub.w")
ea_to_an(m68_suba__n_l,"sub.l")
ea_to_an(m68_cmpa__n_w,"cmp.w")
ea_to_an(m68_cmpa__n_l,"cmp.l")

/******************** SUPPORT ROUTINE:  ADDX AND SUBX
********************/

#define support_addsubx(name,dump)\
static void name(void){\
	word nregx=(word)((inst>>9)&7),nregy=(word)(inst&7);\
	if(inst&0x0008)sprintf(sdebug,"%s  -(a%d),-(a%d)",dump,nregy,nregx);\
	else sprintf(sdebug,"%s  d%d,d%d",dump,nregy,nregx);\
}

support_addsubx(m68support_addx_b,"addx.b")
support_addsubx(m68support_addx_w,"addx.w")
support_addsubx(m68support_addx_l,"addx.l")
support_addsubx(m68support_subx_b,"subx.b")
support_addsubx(m68support_subx_w,"subx.w")
support_addsubx(m68support_subx_l,"subx.l")

#define support_bcd(name,dump)\
static void name(void){\
	word nregx=(word)((inst>>9)&7),nregy=(word)(inst&7);\
	if(inst&0x0008)sprintf(sdebug,"%s    -(a%d),-(a%d)",dump,nregy,nregx);\
	else sprintf(sdebug,"%s    d%d,d%d",dump,nregy,nregx);\
}

support_bcd(m68support_abcd,"abcd")
support_bcd(m68support_sbcd,"sbcd")

/******************** SUPPORT ROUTINE:  CMPM
********************/

#define support_cmpm(name,dump) static void name(void){\
	sprintf(sdebug,"%s   (a%d)+,(a%d)+",dump,inst&7,(inst>>9)&7);\
}

support_cmpm(m68support_cmpm_b,"cmp.b")
support_cmpm(m68support_cmpm_w,"cmp.w")
support_cmpm(m68support_cmpm_l,"cmp.l")

/******************** SUPPORT ROUTINE:  EXG
********************/

static void m68support_exg_same(void){
	dword rx;
	rx=(inst&8)|((inst>>9)&7);
	sprintf(sdebug,"exg.l   %c%d,%c%d",
		rx&8?'a':'d',rx&7,inst&8?'a':'d',inst&7);
}

static void m68support_exg_diff(void){
	sprintf(sdebug,"exg.l   d%d,a%d",(inst>>9)&7,inst&7);
}

/********************      SOURCE: DATA REGISTER
********************* DESTINATION: EFFECTIVE ADDRESS
*********************
********************* calls a support routine if EA is a register
********************/

#define dn_to_ea(name,dump,s_cond,s_routine)\
static void name(void){\
	ea;if(s_cond)s_routine();\
	else sprintf(sdebug,"%s   d%d,%s",dump,(inst>>9)&7,eabuffer);\
}

dn_to_ea(m68_add_e_n_b,"add.b",isregister,m68support_addx_b)
dn_to_ea(m68_add_e_n_w,"add.w",isregister,m68support_addx_w)
dn_to_ea(m68_add_e_n_l,"add.l",isregister,m68support_addx_l)
dn_to_ea(m68_sub_e_n_b,"sub.b",isregister,m68support_subx_b)
dn_to_ea(m68_sub_e_n_w,"sub.w",isregister,m68support_subx_w)
dn_to_ea(m68_sub_e_n_l,"sub.l",isregister,m68support_subx_l)
dn_to_ea(m68_eor_e_n_b,"eor.b",isaddressr,m68support_cmpm_b)
dn_to_ea(m68_eor_e_n_w,"eor.w",isaddressr,m68support_cmpm_w)
dn_to_ea(m68_eor_e_n_l,"eor.l",isaddressr,m68support_cmpm_l)
dn_to_ea(m68_or__e_n_b,"or.b ",isregister,m68support_sbcd)
dn_to_ea(m68_or__e_n_w,"or.w ",isregister,m68unsupported)
dn_to_ea(m68_or__e_n_l,"or.l ",isregister,m68unsupported)
dn_to_ea(m68_and_e_n_b,"and.b",isregister,m68support_abcd)
dn_to_ea(m68_and_e_n_w,"and.w",isregister,m68support_exg_same)
dn_to_ea(m68_and_e_n_l,"and.l",isaddressr,m68support_exg_diff)

/********************      SOURCE: QUICK DATA
********************* DESTINATION: EFFECTIVE ADDRESS
********************/

#define qn_to_ea(name,dump1) static void name(void){\
	ea;sprintf(sdebug,"%s   #%d,%s",dump1,(((inst>>9)&7)==0)?8:(inst>>9)&7,eabuffer);\
}

qn_to_ea(m68_addq__n_b,"add.b")
qn_to_ea(m68_addq__n_w,"add.w")
qn_to_ea(m68_addq__n_l,"add.l")
qn_to_ea(m68_subq__n_b,"sub.b")
qn_to_ea(m68_subq__n_w,"sub.w")
qn_to_ea(m68_subq__n_l,"sub.l")

static const x68func_t iocs_table[] = {
	{ 0x7000, "_B_KEYINP" },
	{ 0x7001, "_B_KEYSNS" },
	{ 0x7002, "_B_SFTSNS" },
	{ 0x7003, "_KEY_INIT" },
	{ 0x7004, "_BITSNS" },
	{ 0x7005, "_SKEYSET" },
	{ 0x7006, "_LEDCTRL" },
	{ 0x7007, "_LEDSET" },
	{ 0x7008, "_KEYDLY" },
	{ 0x7009, "_KEYREP" },
	{ 0x700c, "_TVCTRL" },
	{ 0x700d, "_LEDMOD" },
	{ 0x700e, "_TGUSEMD" },
	{ 0x700f, "_DEFCHR" },
	{ 0x7010, "_CRTMOD" },
	{ 0x7011, "_CONTRAST" },
	{ 0x7012, "_HSVTORGB" },
	{ 0x7013, "_TPALET" },
	{ 0x7014, "_TPALET2" },
	{ 0x7015, "_TCOLOR" },
	{ 0x7016, "_FNTADR" },
	{ 0x7017, "_VRAMGET" },
	{ 0x7018, "_VRAMPUT" },
	{ 0x7019, "_FNTGET" },
	{ 0x701a, "_TEXTGET" },
	{ 0x701b, "_TEXTPUT" },
	{ 0x701c, "_CLIPPUT" },
	{ 0x701d, "_SCROLL" },
	{ 0x701e, "_B_CURON" },
	{ 0x701f, "_B_CUROFF" },
	{ 0x7020, "_B_PUTC" },
	{ 0x7021, "_B_PRINT" },
	{ 0x7022, "_B_COLOR" },
	{ 0x7023, "_B_LOCATE" },
	{ 0x7024, "_B_DOWN_S" },
	{ 0x7025, "_B_UP_S" },
	{ 0x7026, "_B_UP" },
	{ 0x7027, "_B_DOWN" },
	{ 0x7028, "_B_RIGHT" },
	{ 0x7029, "_B_LEFT" },
	{ 0x702a, "_B_CLR_ST" },
	{ 0x702b, "_B_ERA_ST" },
	{ 0x702c, "_B_INS" },
	{ 0x702d, "_B_DEL" },
	{ 0x702e, "_B_CONSOL" },
	{ 0x702f, "_B_PUTMES" },
	{ 0x7030, "_SET232C" },
	{ 0x7031, "_LOF232C" },
	{ 0x7032, "_INP232C" },
	{ 0x7033, "_ISNS232C" },
	{ 0x7034, "_OSNS232C" },
	{ 0x7035, "_OUT232C" },
	{ 0x7038, "_SETFNTADR" },
	{ 0x703b, "_JOYGET" },
	{ 0x703c, "_INIT_PRN" },
	{ 0x703d, "_SNSPRN" },
	{ 0x703e, "_OUTLPT" },
	{ 0x703f, "_OUTPRN" },
	{ 0x7040, "_B_SEEK" },
	{ 0x7041, "_B_VERIFY" },
	{ 0x7042, "_B_READDI" },
	{ 0x7043, "_B_DSKINI" },
	{ 0x7044, "_B_DRVSNS" },
	{ 0x7045, "_B_WRITE" },
	{ 0x7046, "_B_READ" },
	{ 0x7047, "_B_RECALI" },
	{ 0x7048, "_B_ASSIGN" },
	{ 0x7049, "_B_WRITED" },
	{ 0x704a, "_B_READID" },
	{ 0x704b, "_B_BADFMT" },
	{ 0x704c, "_B_READDL" },
	{ 0x704d, "_B_FORMAT" },
	{ 0x704e, "_B_DRVCHK" },
	{ 0x704f, "_B_EJECT" },
	{ 0x7050, "_DATEBCD" },
	{ 0x7051, "_DATESET" },
	{ 0x7052, "_TIMEBCD" },
	{ 0x7053, "_TIMESET" },
	{ 0x7054, "_DATEGET" },
	{ 0x7055, "_DATEBIN" },
	{ 0x7056, "_TIMEGET" },
	{ 0x7057, "_TIMEBIN" },
	{ 0x7058, "_DATECNV" },
	{ 0x7059, "_TIMECNV" },
	{ 0x705a, "_DATEASC" },
	{ 0x705b, "_TIMEASC" },
	{ 0x705c, "_DAYASC" },
	{ 0x705d, "_ALARMMOD" },
	{ 0x705e, "_ALARMSET" },
	{ 0x705f, "_ALARMGET" },
	{ 0x7060, "_ADPCMOUT" },
	{ 0x7061, "_ADPCMINP" },
	{ 0x7062, "_ADPCMAOT" },
	{ 0x7063, "_ADPCMAIN" },
	{ 0x7064, "_ADPCMLOT" },
	{ 0x7065, "_ADPCMLIN" },
	{ 0x7066, "_ADPCMSNS" },
	{ 0x7067, "_ADPCMMOD" },
	{ 0x7068, "_OPMSET" },
	{ 0x7069, "_OPMSNS" },
	{ 0x706a, "_OPMINTST" },
	{ 0x706b, "_TIMERDST" },
	{ 0x706c, "_VDISPST" },
	{ 0x706d, "_CRTCRAS" },
	{ 0x706e, "_HSYNCST" },
	{ 0x706f, "_PRNINTST" },
	{ 0x7070, "_MS_INIT" },
	{ 0x7071, "_MS_CURON" },
	{ 0x7072, "_MS_CUROF" },
	{ 0x7073, "_MS_STAT" },
	{ 0x7074, "_MS_GETDT" },
	{ 0x7075, "_MS_CURGT" },
	{ 0x7076, "_MS_CURST" },
	{ 0x7077, "_MS_LIMIT" },
	{ 0x7078, "_MS_OFFTM" },
	{ 0x7079, "_MS_ONTM" },
	{ 0x707a, "_MS_PATST" },
	{ 0x707b, "_MS_SEL" },
	{ 0x707c, "_MS_SEL2" },
	{ 0x707d, "_SKEY_MOD" },
	{ 0x707e, "_DENSNS" },
	{ 0x707f, "_ONTIME" },
	{ 0x7080, "_B_INTVCS" },
	{ 0x7081, "_B_SUPER" },
	{ 0x7082, "_B_BPEEK" },
	{ 0x7083, "_B_WPEEK" },
	{ 0x7084, "_B_LPEEK" },
	{ 0x7085, "_B_MEMSTR" },
	{ 0x7086, "_B_BPOKE" },
	{ 0x7087, "_B_WPOKE" },
	{ 0x7088, "_B_LPOKE" },
	{ 0x7089, "_B_MEMSET" },
	{ 0x708a, "_DMAMOVE" },
	{ 0x708b, "_DMAMOV_A" },
	{ 0x708c, "_DMAMOV_L" },
	{ 0x708d, "_DMAMODE" },
	{ 0x708e, "_BOOTINF" },
	{ 0x708f, "_ROMVER" },
	{ 0x7090, "_G_CLR_ON" },
	{ 0x7094, "_GPALET" },
	{ 0x70a0, "_SFTJIS" },
	{ 0x70a1, "_JISSFT" },
	{ 0x70a2, "_AKCONV" },
	{ 0x70a3, "_RMACNV" },
	{ 0x70a4, "_DAKJOB" },
	{ 0x70a5, "_HANJOB" },
	{ 0x70ac, "_SYS_STAT" },
	{ 0x70ad, "_B_CONMOD" },
	{ 0x70ae, "_OS_CURON" },
	{ 0x70af, "_OS_CUROF" },
	{ 0x70b0, "_DRAWMODE" },
	{ 0x70b1, "_APAGE" },
	{ 0x70b2, "_VPAGE" },
	{ 0x70b3, "_HOME" },
	{ 0x70b4, "_WINDOW" },
	{ 0x70b5, "_WIPE" },
	{ 0x70b6, "_PSET" },
	{ 0x70b7, "_POINT" },
	{ 0x70b8, "_LINE" },
	{ 0x70b9, "_BOX" },
	{ 0x70ba, "_FILL" },
	{ 0x70bb, "_CIRCLE" },
	{ 0x70bc, "_PAINT" },
	{ 0x70bd, "_SYMBOL" },
	{ 0x70be, "_GETGRM" },
	{ 0x70bf, "_PUTGRM" },
	{ 0x70c0, "_SP_INIT" },
	{ 0x70c1, "_SP_ON" },
	{ 0x70c2, "_SP_OFF" },
	{ 0x70c3, "_SP_CGCLR" },
	{ 0x70c4, "_SP_DEFCG" },
	{ 0x70c5, "_SP_GTPCG" },
	{ 0x70c6, "_SP_REGST" },
	{ 0x70c7, "_SP_REGGT" },
	{ 0x70c8, "_BGSCRLST" },
	{ 0x70c9, "_BGSCRLGT" },
	{ 0x70ca, "_BGCTRLST" },
	{ 0x70cb, "_BGCTRLGT" },
	{ 0x70cc, "_BGTEXTCL" },
	{ 0x70cd, "_BGTEXTST" },
	{ 0x70ce, "_BGTEXTGT" },
	{ 0x70cf, "_SPALET" },
	{ 0x70d3, "_TXXLINE" },
	{ 0x70d4, "_TXYLINE" },
	{ 0x70d5, "_TXLINE" },
	{ 0x70d6, "_TXBOX" },
	{ 0x70d7, "_TXFILL" },
	{ 0x70d8, "_TXREV" },
	{ 0x70df, "_TXRASCPY" },
	{ 0x70f0, "_OPMDRV" },
	{ 0x70f1, "_RSDRV" },
	{ 0x70f2, "_A_JOYGET" },
	{ 0x70f3, "_MUSICDRV" },
	{ 0x70f5, "_SCSIDRV" },
	{ 0x70fd, "_ABORTRST" },
	{ 0x70fe, "_IPLERR" },
	{ 0x70ff, "_ABORTJOB" },
	{ 0, NULL }
};

static void m68_iocs____x(word inst)
{
	const x68func_t *table;

	fetch();
	table = iocs_table;
	while (table->code != 0) {
		if (inst == table->code) {
			sprintf(sdebug, "IOCS    %s", table->name);
			return;
		}
		table++;
	}
	strcpy(sdebug, "IOCS    (UNDEFINED)");
}

/********************      SOURCE: QUICK DATA
********************* DESTINATION: DATA REGISTER
********************/
/* MOVEQ is the only instruction that uses this form */

static void m68_moveq_n_l(void){
	if (((inst>>9)&7)==0){
		if (cpudebug_fetch(debugpc)==0x4e4f) {
			m68_iocs____x(inst);
			return;
		}
	}
	sprintf(sdebug,"moveq.l #$"hex08",d%d",inst&0xFF,(inst>>9)&7);
}

/********************      SOURCE: EFFECTIVE ADDRESS
********************* DESTINATION: EFFECTIVE ADDRESS
********************/
/* MOVE is the only instruction that uses this form */

#define ea_to_ea(name,dump) static void name(void){\
	char tmpbuf[40];\
	ea;strcpy(tmpbuf,eabuffer);\
	eacalc[((((inst>>3)&(7<<3)))|((inst>>9)&7))]();\
	sprintf(sdebug,"%s  %s,%s",dump,tmpbuf,eabuffer);\
}

ea_to_ea(m68_move____b,"move.b")
ea_to_ea(m68_move____w,"move.w")
ea_to_ea(m68_move____l,"move.l")

/******************** MOVEM, EXT
********************/

static void getreglistf(word mask,char*rl){
int now;
int i;
int first;

	now = -1;
	first = 0;
	for (i=0; i<16; i++) {
		if (mask & 0x0001) {
			/* next D7 ? */
			if ((i == 8) && (now != -1)) {
				/* next D7, process D */
				if (first > 0) {
					*(rl++) = '/';
				}
				*(rl++) = 'd';
				*(rl++) = (char)('0' + now);
				if (i != 7) {
					*(rl++) = '-';
					*(rl++) = 'd';
					*(rl++) = '7';
				}
				now = -1;
				first++;
			}

			/* Start? */
			if (now == -1) {
				/* Start from this register */
				now = i;
			}
		}
		else {
			if (now != -1) {
				/* DorA End ? */
				if (first > 0) {
					*(rl++) = '/';
				}
				if (now < 8) {
					*(rl++) = 'd';
				}
				else {
					*(rl++) = 'a';
				}
				*(rl++) = (char)('0' + (now & 0x07));
				if ((i - 1) != now) {
					*(rl++) = '-';
					if (now < 8) {
						*(rl++) = 'd';
					}
					else {
						*(rl++) = 'a';
					}
					*(rl++) = (char)('0' + ((i - 1) & 0x07));
				}
				now = -1;
				first++;
			}
		}
		mask >>= 1;
	}

	/* A End ? */
	if (now != -1) {
		if (first > 0) {
			*(rl++) = '/';
		}
		*(rl++) = 'a';
		*(rl++) = (char)('0' + (now & 0x07));
		if (now != 15) {
			*(rl++) = '-';
			*(rl++) = 'a';
			*(rl++) = '7';
		}
	}

	*rl=0;
}

static void getreglistb(word mask,char*rl){
int now;
int i;
int first;

	now = -1;
	first = 0;
	for (i=0; i<16; i++) {
		if (mask & 0x8000) {
			/* D End ? */
			if ((i == 8) && (now != -1)) {
				if (first > 0) {
					*(rl++) = '/';
				}
				*(rl++) = 'd';
				*(rl++) = (char)('0' + now);
				if (now != 7) {
					*(rl++) = '-';
					*(rl++) = 'd';
					*(rl++) = '7';
				}
				now = -1;
				first++;
			}

			/* Start */
			if (now == -1) {
				now = i;
			}
		}
		else {
			if (now != -1) {
				/* DorA End ? */
				if (first > 0) {
					*(rl++) = '/';
				}
				if (now < 8) {
					*(rl++) = 'd';
				}
				else {
					*(rl++) = 'a';
				}
				*(rl++) = (char)('0' + (now & 0x07));
				if ((i - 1) != now) {
					*(rl++) = '-';
					if (now < 8) {
						*(rl++) = 'd';
					}
					else {
						*(rl++) = 'a';
					}
					*(rl++) = (char)('0' + ((i - 1) & 0x07));
				}
				now = -1;
				first++;
			}
		}
		mask <<= 1;
	}

	/* A End ? */
	if (now != -1) {
		if (first > 0) {
			*(rl++) = '/';
		}
		*(rl++) = 'a';
		*(rl++) = (char)('0' + (now & 0x07));
		if (now != 15) {
			*(rl++) = '-';
			*(rl++) = 'a';
			*(rl++) = '7';
		}
	}

	*rl=0;
}

#define movem_mem(name,dumpm,dumpx)\
static void name(void){\
	word regmask;\
	char reglist[50];\
	if((inst&0x38)==0x0000){ /* ext */\
		sprintf(sdebug,dumpx"d%d",inst&7);\
	}else if((inst&0x38)==0x0020){ /* predecrement addressing mode */\
		regmask=fetch();\
		getreglistb(regmask,reglist);\
		sprintf(sdebug,dumpm"%s,-(a%d)",reglist,inst&7);\
	}else{\
		regmask=fetch();\
		ea;getreglistf(regmask,reglist);\
		sprintf(sdebug,dumpm "%s,%s",reglist,eabuffer);\
	}\
}

movem_mem(m68_movem___w,"movem.w ","ext.w   ")
movem_mem(m68_movem___l,"movem.l ","ext.l   ")

#define movem_reg(name,dump) static void name(void){\
	word regmask;\
	char reglist[50];\
	regmask=fetch();\
	ea;getreglistf(regmask,reglist);\
	sprintf(sdebug,dump "%s,%s",eabuffer,reglist);\
}

movem_reg(m68_movem_r_w,"movem.w ")
movem_reg(m68_movem_r_l,"movem.l ")

/******************** INSTRUCTIONS THAT INVOLVE SR/CCR ********************/

static void m68_move2sr_w(void){ea;sprintf(sdebug,"move.w  %s,sr",eabuffer);}
static void m68_movefsr_w(void){ea;sprintf(sdebug,"move.w  sr,%s",eabuffer);}
static void m68_move2cc_w(void){ea;sprintf(sdebug,"move.b  %s,ccr",eabuffer);}
static void m68_movefcc_w(void){ea;sprintf(sdebug,"move.b  ccr,%s",eabuffer);}

static void m68_rts_____x(void){sprintf(sdebug,"rts");}

/******************** SHIFTS AND ROTATES ********************/

#define regshift(name,sizedump) \
static void name(void){\
	char tmpbuf[10];\
	if((inst&0x20)==0)sprintf(tmpbuf,"#$"hex08",d%d",(((inst>>9)&7)==0)?8:(inst>>9)&7,inst&7);\
	else sprintf(tmpbuf,"d%d,d%d",(inst>>9)&7,inst&7);\
	switch(inst&0x18){\
		case 0x00:sprintf(sdebug,"as%s   %s",sizedump,tmpbuf);break;\
		case 0x08:sprintf(sdebug,"ls%s   %s",sizedump,tmpbuf);break;\
		case 0x10:sprintf(sdebug,"rox%s  %s",sizedump,tmpbuf);break;\
		case 0x18:sprintf(sdebug,"ro%s   %s",sizedump,tmpbuf);break;\
	}\
}

regshift(m68_shl_r_n_b,"l.b")
regshift(m68_shl_r_n_w,"l.w")
regshift(m68_shl_r_n_l,"l.l")
regshift(m68_shr_r_n_b,"r.b")
regshift(m68_shr_r_n_w,"r.w")
regshift(m68_shr_r_n_l,"r.l")

/******************** NOP ********************/

static void m68_nop_____x(void){sprintf(sdebug,"nop");}

/******************** LINK / UNLINK ********************/

static void m68_link_an_w(void){
	int16 briefext=fetch();
	sprintf(sdebug,"link    a%d,%c#$"hex16,inst&7,briefext<0?'-':'+',
		briefext<0?-briefext:briefext
	);
}

static void m68_unlk_an_x(void){
	sprintf(sdebug,"unlk    a%d",inst&7);
}

static void m68_stop____x(void){sprintf(sdebug,"stop    #$%04X",fetch());}
static void m68_rte_____x(void){sprintf(sdebug,"rte");}
static void m68_rtr_____x(void){sprintf(sdebug,"rtr");}
static void m68_reset___x(void){sprintf(sdebug,"reset");}
static void m68_rtd_____x(void){
	int16 briefext=fetch();
	sprintf(sdebug,"rtd     %c#$"hex16,briefext<0?'-':'+',
		briefext<0?-briefext:briefext);
}
static void m68_divu__n_w(void){ea;sprintf(sdebug,"divu.w  %s,d%d",eabuffer,(inst>>9)&7);}
static void m68_divs__n_w(void){ea;sprintf(sdebug,"divs.w  %s,d%d",eabuffer,(inst>>9)&7);}
static void m68_mulu__n_w(void){ea;sprintf(sdebug,"mulu.w  %s,d%d",eabuffer,(inst>>9)&7);}
static void m68_muls__n_w(void){ea;sprintf(sdebug,"muls.w  %s,d%d",eabuffer,(inst>>9)&7);}

static void m68_asr_m___w(void){ea;sprintf(sdebug,"asr.w   %s",eabuffer);}
static void m68_asl_m___w(void){ea;sprintf(sdebug,"asl.w   %s",eabuffer);}
static void m68_lsr_m___w(void){ea;sprintf(sdebug,"lsr.w   %s",eabuffer);}
static void m68_lsl_m___w(void){ea;sprintf(sdebug,"lsl.w   %s",eabuffer);}
static void m68_roxr_m__w(void){ea;sprintf(sdebug,"roxr.w  %s",eabuffer);}
static void m68_roxl_m__w(void){ea;sprintf(sdebug,"roxl.w  %s",eabuffer);}
static void m68_ror_m___w(void){ea;sprintf(sdebug,"ror.w   %s",eabuffer);}
static void m68_rol_m___w(void){ea;sprintf(sdebug,"rol.w   %s",eabuffer);}

static void m68_nbcd____b(void){ea;sprintf(sdebug,"nbcd.b  %s",eabuffer);}
static void m68_chk___n_w(void){ea;sprintf(sdebug,"chk     %s,d%d",eabuffer,(inst>>9)&7);}

static void m68_trap_nn_x(void){sprintf(sdebug,"trap    #%d",inst&0xF);}
static void m68_move_2u_l(void){sprintf(sdebug,"move.l  a%d,usp",inst&7);}
static void m68_move_fu_l(void){sprintf(sdebug,"move.l  usp,a%d",inst&7);}

static void m68_trapv___x(void){sprintf(sdebug,"trapv");}

static char*specialregister(unsigned short int code){
	switch(code&0xFFF){
	case 0x000:return("sfc");
	case 0x001:return("dfc");
	case 0x800:return("usp");
	case 0x801:return("vbr");
	}
	return("???");
}

static void m68_movec_r_x(void){
	unsigned short int f=fetch();
	sprintf(sdebug,"movec   %s,%c%d",
		specialregister(f),
		(f&0x8000)?'a':'d',(f>>12)&7
	);
}

static void m68_movec_c_x(void){
	unsigned short int f=fetch();
	sprintf(sdebug,"movec   %c%d,%s",
		(f&0x8000)?'a':'d',(f>>12)&7,
		specialregister(f)
	);
}

static const x68func_t fe_table[] = {
	{ 0xfe00, "__LMUL" },
	{ 0xfe01, "__LDIV" },
	{ 0xfe02, "__LMOD" },
	{ 0xfe04, "__UMUL" },
	{ 0xfe05, "__UDIV" },
	{ 0xfe06, "__UMOD" },
	{ 0xfe08, "__IMUL" },
	{ 0xfe09, "__IDIV" },
	{ 0xfe0c, "__RANDOMIZE" },
	{ 0xfe0d, "__SRAND" },
	{ 0xfe0e, "__RAND" },
	{ 0xfe10, "__STOL" },
	{ 0xfe11, "__LTOS" },
	{ 0xfe12, "__STOH" },
	{ 0xfe13, "__HTOS" },
	{ 0xfe14, "__STOO" },
	{ 0xfe15, "__OTOS" },
	{ 0xfe16, "__STOB" },
	{ 0xfe17, "__BTOS" },
	{ 0xfe18, "__IUSING" },
	{ 0xfe1a, "__LTOD" },
	{ 0xfe1b, "__DTOL" },
	{ 0xfe1c, "__LTOF" },
	{ 0xfe1d, "__FTOL" },
	{ 0xfe1e, "__FTOD" },
	{ 0xfe1f, "__DTOF" },
	{ 0xfe20, "__VAL" },
	{ 0xfe21, "__USING" },
	{ 0xfe22, "__STOD" },
	{ 0xfe23, "__DTOS" },
	{ 0xfe24, "__ECVT" },
	{ 0xfe25, "__FCVT" },
	{ 0xfe26, "__GCVT" },
	{ 0xfe28, "__DTST" },
	{ 0xfe29, "__DCMP" },
	{ 0xfe2a, "__DNEG" },
	{ 0xfe2b, "__DADD" },
	{ 0xfe2c, "__DSUB" },
	{ 0xfe2d, "__DMUL" },
	{ 0xfe2e, "__DDIV" },
	{ 0xfe2f, "__DMOD" },
	{ 0xfe30, "__DABS" },
	{ 0xfe31, "__DCEIL" },
	{ 0xfe32, "__DFIX" },
	{ 0xfe33, "__DFLOOR" },
	{ 0xfe34, "__DFRAC" },
	{ 0xfe35, "__DSGN" },
	{ 0xfe36, "__SIN" },
	{ 0xfe37, "__COS" },
	{ 0xfe38, "__TAN" },
	{ 0xfe39, "__ATAN" },
	{ 0xfe3a, "__LOG" },
	{ 0xfe3b, "__EXP" },
	{ 0xfe3c, "__SQR" },
	{ 0xfe3d, "__PI" },
	{ 0xfe3e, "__NPI" },
	{ 0xfe3f, "__POWER" },
	{ 0xfe40, "__RND" },
	{ 0xfe41, "__SINH" },
	{ 0xfe42, "__COSH" },
	{ 0xfe43, "__TANH" },
	{ 0xfe44, "__ATANH" },
	{ 0xfe45, "__ASIN" },
	{ 0xfe46, "__ACOS" },
	{ 0xfe47, "__LOG10" },
	{ 0xfe48, "__LOG2" },
	{ 0xfe49, "__DFREXP" },
	{ 0xfe4a, "__DLDEXP" },
	{ 0xfe4b, "__DADDONE" },
	{ 0xfe4c, "__DSUBONE" },
	{ 0xfe4d, "__DDIVTWO" },
	{ 0xfe4e, "__DIEECNV" },
	{ 0xfe4f, "__IEEDCNV" },
	{ 0xfe50, "__FVAL" },
	{ 0xfe51, "__FUSING" },
	{ 0xfe52, "__STOF" },
	{ 0xfe53, "__FTOS" },
	{ 0xfe54, "__FECVT" },
	{ 0xfe55, "__FFCVT" },
	{ 0xfe56, "__FGCVT" },
	{ 0xfe58, "__FTST" },
	{ 0xfe59, "__FCMP" },
	{ 0xfe5a, "__FNEG" },
	{ 0xfe5b, "__FADD" },
	{ 0xfe5c, "__FSUB" },
	{ 0xfe5d, "__FMUL" },
	{ 0xfe5e, "__FDIV" },
	{ 0xfe5f, "__FMOD" },
	{ 0xfe60, "__FABS" },
	{ 0xfe61, "__FCEIL" },
	{ 0xfe62, "__FFIX" },
	{ 0xfe63, "__FFLOOR" },
	{ 0xfe64, "__FFRAC" },
	{ 0xfe65, "__FSGN" },
	{ 0xfe66, "__FSIN" },
	{ 0xfe67, "__FCOS" },
	{ 0xfe68, "__FTAN" },
	{ 0xfe69, "__FATAN" },
	{ 0xfe6a, "__FLOG" },
	{ 0xfe6b, "__FEXP" },
	{ 0xfe6c, "__FSQR" },
	{ 0xfe6d, "__FPI" },
	{ 0xfe6e, "__FNPI" },
	{ 0xfe6f, "__FPOWER" },
	{ 0xfe70, "__FRND" },
	{ 0xfe71, "__FSINH" },
	{ 0xfe72, "__FCOSH" },
	{ 0xfe73, "__FTANH" },
	{ 0xfe74, "__FATANH" },
	{ 0xfe75, "__FASIN" },
	{ 0xfe76, "__FACOS" },
	{ 0xfe77, "__FLOG10" },
	{ 0xfe78, "__FLOG2" },
	{ 0xfe79, "__FFREXP" },
	{ 0xfe7a, "__FLDEXP" },
	{ 0xfe7b, "__FADDONE" },
	{ 0xfe7c, "__FSUBONE" },
	{ 0xfe7d, "__FDIVTWO" },
	{ 0xfe7e, "__FIEECNV" },
	{ 0xfe7f, "__IEEFCNV" },
	{ 0xfee0, "__CLMUL" },
	{ 0xfee1, "__CLDIV" },
	{ 0xfee2, "__CLMOD" },
	{ 0xfee3, "__CUMUL" },
	{ 0xfee4, "__CUDIV" },
	{ 0xfee5, "__CUMOD" },
	{ 0xfee6, "__CLTOD" },
	{ 0xfee7, "__CDTOL" },
	{ 0xfee8, "__CLTOF" },
	{ 0xfee9, "__CFTOL" },
	{ 0xfeea, "__CFTOD" },
	{ 0xfeeb, "__CDTOF" },
	{ 0xfeec, "__CDCMP" },
	{ 0xfeed, "__CDADD" },
	{ 0xfeee, "__CDSUB" },
	{ 0xfeef, "__CDMUL" },
	{ 0xfef0, "__CDDIV" },
	{ 0xfef1, "__CDMOD" },
	{ 0xfef2, "__CFCMP" },
	{ 0xfef3, "__CFADD" },
	{ 0xfef4, "__CFSUB" },
	{ 0xfef5, "__CFMUL" },
	{ 0xfef6, "__CFDIV" },
	{ 0xfef7, "__CFMOD" },
	{ 0xfef8, "__CDTST" },
	{ 0xfef9, "__CFTST" },
	{ 0xfefa, "__CDINC" },
	{ 0xfefb, "__CFINC" },
	{ 0xfefc, "__CDDEC" },
	{ 0xfefd, "__CFDEC" },
	{ 0xfefe, "__FEVARG" },
	{ 0xfeff, "__FEVECS" },
	{ 0, NULL }
};

static void m68_fecall__x(void)
{
	const x68func_t *table;

	table = fe_table;
	while (table->code != 0) {
		if (inst == table->code) {
			sprintf(sdebug, "FPACK   %s", table->name);
			return;
		}
		table++;
	}
	strcpy(sdebug, "FPACK   (UNDEFINED)");
}

static const x68func_t dos_table[] = {
	{ 0xff00, "_EXIT" },
	{ 0xff01, "_GETCHAR" },
	{ 0xff02, "_PUTCHAR" },
	{ 0xff03, "_COMINP" },
	{ 0xff04, "_COMOUT" },
	{ 0xff05, "_PRNOUT" },
	{ 0xff06, "_INPOUT" },
	{ 0xff07, "_INKEY" },
	{ 0xff08, "_GETC" },
	{ 0xff09, "_PRINT" },
	{ 0xff0a, "_GETS" },
	{ 0xff0b, "_KEYSNS" },
	{ 0xff0c, "_KFLUSH" },
	{ 0xff0d, "_FFLUSH" },
	{ 0xff0e, "_CHGDRV" },
	{ 0xff0f, "_DRVCTRL" },
	{ 0xff10, "_CONSNS" },
	{ 0xff11, "_PRNSNS" },
	{ 0xff12, "_CINSNS" },
	{ 0xff13, "_COUTSNS" },
	{ 0xff17, "_FATCHK" },
	{ 0xff18, "_HENDSP" },
	{ 0xff19, "_CURDRV" },
	{ 0xff1a, "_GETSS" },
	{ 0xff1b, "_FGETC" },
	{ 0xff1c, "_FGETS" },
	{ 0xff1d, "_FPUTC" },
	{ 0xff1e, "_FPUTS" },
	{ 0xff1f, "_ALLCLOSE" },
	{ 0xff20, "_SUPER" },
	{ 0xff21, "_FNCKEY" },
	{ 0xff22, "_KNJCTRL" },
	{ 0xff23, "_CONCTRL" },
	{ 0xff24, "_KEYCTRL" },
	{ 0xff25, "_INTVCS" },
	{ 0xff26, "_PSPSET" },
	{ 0xff27, "_GETTIM2" },
	{ 0xff28, "_SETTIM2" },
	{ 0xff29, "_NAMESTS" },
	{ 0xff2a, "_GETDATE" },
	{ 0xff2b, "_SETDATE" },
	{ 0xff2c, "_GETTIME" },
	{ 0xff2d, "_SETTIME" },
	{ 0xff2e, "_VERIFY" },
	{ 0xff2f, "_DUP0" },
	{ 0xff30, "_VERNUM" },
	{ 0xff31, "_KEEPPR" },
	{ 0xff32, "_GETDPB" },
	{ 0xff33, "_BREAKCK" },
	{ 0xff34, "_DRVXCHG" },
	{ 0xff35, "_INTVCG" },
	{ 0xff36, "_DSKFRE" },
	{ 0xff37, "_NAMECK" },
	{ 0xff39, "_MKDIR" },
	{ 0xff3a, "_RMDIR" },
	{ 0xff3b, "_CHDIR" },
	{ 0xff3c, "_CREATE" },
	{ 0xff3d, "_OPEN" },
	{ 0xff3e, "_CLOSE" },
	{ 0xff3f, "_READ" },
	{ 0xff40, "_WRITE" },
	{ 0xff41, "_DELETE" },
	{ 0xff42, "_SEEK" },
	{ 0xff43, "_CHMOD" },
	{ 0xff44, "_IOCTRL" },
	{ 0xff45, "_DUP" },
	{ 0xff46, "_DUP2" },
	{ 0xff47, "_CURDIR" },
	{ 0xff48, "_MALLOC" },
	{ 0xff49, "_MFREE" },
	{ 0xff4a, "_SETBLOCK" },
	{ 0xff4b, "_EXEC" },
	{ 0xff4c, "_EXIT2" },
	{ 0xff4d, "_WAIT" },
	{ 0xff4e, "_FILES" },
	{ 0xff4f, "_NFILES" },
	{ 0xff50, "_SETPDB" },
	{ 0xff51, "_GETPDB" },
	{ 0xff52, "_SETENV" },
	{ 0xff53, "_GETENV" },
	{ 0xff54, "_VERIFYG" },
	{ 0xff55, "_COMMON" },
	{ 0xff56, "_RENAME" },
	{ 0xff57, "_FILEDATE" },
	{ 0xff58, "_MALLOC2" },
	{ 0xff5a, "_MAKETMP" },
	{ 0xff5b, "_NEWFILE" },
	{ 0xff5c, "_LOCK" },
	{ 0xff5f, "_ASSIGN" },
	{ 0xff7a, "_FFLUSH_SET" },
	{ 0xff7b, "_OS_PATCH" },
	{ 0xff7c, "_GETFCB" },
	{ 0xff7d, "_S_MALLOC" },
	{ 0xff7e, "_S_MFREE" },
	{ 0xff7f, "_S_PROCESS" },
	{ 0xff80, "_SETPDB" },
	{ 0xff81, "_GETPDB" },
	{ 0xff82, "_SETENV" },
	{ 0xff83, "_GETENV" },
	{ 0xff84, "_VERIFYG" },
	{ 0xff85, "_COMMON" },
	{ 0xff86, "_RENAME" },
	{ 0xff87, "_FILEDATE" },
	{ 0xff88, "_MALLOC2" },
	{ 0xff8a, "_MAKETMP" },
	{ 0xff8b, "_NEWFILE" },
	{ 0xff8c, "_LOCK" },
	{ 0xff8f, "_ASSIGN" },
	{ 0xffaa, "_FFLUSH_SET" },
	{ 0xffab, "_OS_PATCH" },
	{ 0xffac, "_GETFCB" },
	{ 0xffad, "_S_MALLOC" },
	{ 0xffae, "_S_MFREE" },
	{ 0xffaf, "_S_PROCESS" },
	{ 0xfff0, "_EXITVC" },
	{ 0xfff1, "_CTRLVC" },
	{ 0xfff2, "_ERRJVC" },
	{ 0xfff3, "_DISKRED" },
	{ 0xfff4, "_DISKWRT" },
	{ 0xfff5, "_INDOSFLG" },
	{ 0xfff6, "_SUPER_JSR" },
	{ 0xfff7, "_BUS_ERR" },
	{ 0xfff8, "_OPEN_PR" },
	{ 0xfff9, "_KILL_PR" },
	{ 0xfffa, "_GET_PR" },
	{ 0xfffb, "_SUSPEND_PR" },
	{ 0xfffc, "_SLEEP_PR" },
	{ 0xfffd, "_SEND_PR" },
	{ 0xfffe, "_TIME_PR" },
	{ 0xffff, "_CHANGE_PR" },
	{ 0, NULL }
};

static void m68_doscall_x(void)
{
	const x68func_t *table;

	table = dos_table;
	while (table->code != 0) {
		if (inst == table->code) {
			sprintf(sdebug, "DOS     %s", table->name);
			return;
		}
		table++;
	}
	strcpy(sdebug, "DOS     (UNDEFINED)");
}

/******************** SPECIAL INSTRUCTION TABLE ********************/

/* This table is used for 0100111001xxxxxx instructions (4E4x-4E7x) */
static void(*(debugspecialmap[64]))(void)={
/* 0000xx */ m68_trap_nn_x,m68_trap_nn_x,m68_trap_nn_x,m68_trap_nn_x,
/* 0001xx */ m68_trap_nn_x,m68_trap_nn_x,m68_trap_nn_x,m68_trap_nn_x,
/* 0010xx */ m68_trap_nn_x,m68_trap_nn_x,m68_trap_nn_x,m68_trap_nn_x,
/* 0011xx */ m68_trap_nn_x,m68_trap_nn_x,m68_trap_nn_x,m68_trap_nn_x,
/* 0100xx */ m68_link_an_w,m68_link_an_w,m68_link_an_w,m68_link_an_w,
/* 0101xx */ m68_link_an_w,m68_link_an_w,m68_link_an_w,m68_link_an_w,
/* 0110xx */ m68_unlk_an_x,m68_unlk_an_x,m68_unlk_an_x,m68_unlk_an_x,
/* 0111xx */ m68_unlk_an_x,m68_unlk_an_x,m68_unlk_an_x,m68_unlk_an_x,
/* 1000xx */ m68_move_2u_l,m68_move_2u_l,m68_move_2u_l,m68_move_2u_l,
/* 1001xx */ m68_move_2u_l,m68_move_2u_l,m68_move_2u_l,m68_move_2u_l,
/* 1010xx */ m68_move_fu_l,m68_move_fu_l,m68_move_fu_l,m68_move_fu_l,
/* 1011xx */ m68_move_fu_l,m68_move_fu_l,m68_move_fu_l,m68_move_fu_l,
/* 1100xx */ m68_reset___x,m68_nop_____x,m68_stop____x,m68_rte_____x,
/* 1101xx */ m68_rtd_____x,m68_rts_____x,m68_trapv___x,m68_rtr_____x,
/* 1110xx */ m68_unrecog_x,m68_unrecog_x,m68_movec_r_x,m68_movec_c_x,
/* 1111xx */ m68_unrecog_x,m68_unrecog_x,m68_unrecog_x,m68_unrecog_x};

static void m68_special_x(void){
	debugspecialmap[inst&0x3F]();
}

/******************** INSTRUCTION TABLE ********************/

/* The big 1024-element jump table that handles all possible opcodes
   is in the following header file.
   Use this syntax to execute an instruction:

   cpumap[i>>6]();

   ("i" is the instruction word)
*/

static void(*(debugmap[1024]))(void)={
/* 00000000ss */ m68_ori_____b,m68_ori_____w,m68_ori_____l,m68_unrecog_x,
/* 00000001ss */ m68_bitopdn_x,m68_bitopdn_x,m68_bitopdn_x,m68_bitopdn_x,
/* 00000010ss */ m68_andi____b,m68_andi____w,m68_andi____l,m68_unrecog_x,
/* 00000011ss */ m68_bitopdn_x,m68_bitopdn_x,m68_bitopdn_x,m68_bitopdn_x,
/* 00000100ss */ m68_subi____b,m68_subi____w,m68_subi____l,m68_unrecog_x,
/* 00000101ss */ m68_bitopdn_x,m68_bitopdn_x,m68_bitopdn_x,m68_bitopdn_x,
/* 00000110ss */ m68_addi____b,m68_addi____w,m68_addi____l,m68_unrecog_x,
/* 00000111ss */ m68_bitopdn_x,m68_bitopdn_x,m68_bitopdn_x,m68_bitopdn_x,
/* 00001000ss */ m68_btst_st_x,m68_bchg_st_x,m68_bclr_st_x,m68_bset_st_x,
/* 00001001ss */ m68_bitopdn_x,m68_bitopdn_x,m68_bitopdn_x,m68_bitopdn_x,
/* 00001010ss */ m68_eori____b,m68_eori____w,m68_eori____l,m68_unrecog_x,
/* 00001011ss */ m68_bitopdn_x,m68_bitopdn_x,m68_bitopdn_x,m68_bitopdn_x,
/* 00001100ss */ m68_cmpi____b,m68_cmpi____w,m68_cmpi____l,m68_unrecog_x,
/* 00001101ss */ m68_bitopdn_x,m68_bitopdn_x,m68_bitopdn_x,m68_bitopdn_x,
/* 00001110ss */ m68_unrecog_x,m68_unrecog_x,m68_unrecog_x,m68_unrecog_x,
/* 00001111ss */ m68_bitopdn_x,m68_bitopdn_x,m68_bitopdn_x,m68_bitopdn_x,
/* 00010000ss */ m68_move____b,m68_move____b,m68_move____b,m68_move____b,
/* 00010001ss */ m68_move____b,m68_move____b,m68_move____b,m68_move____b,
/* 00010010ss */ m68_move____b,m68_move____b,m68_move____b,m68_move____b,
/* 00010011ss */ m68_move____b,m68_move____b,m68_move____b,m68_move____b,
/* 00010100ss */ m68_move____b,m68_move____b,m68_move____b,m68_move____b,
/* 00010101ss */ m68_move____b,m68_move____b,m68_move____b,m68_move____b,
/* 00010110ss */ m68_move____b,m68_move____b,m68_move____b,m68_move____b,
/* 00010111ss */ m68_move____b,m68_move____b,m68_move____b,m68_move____b,
/* 00011000ss */ m68_move____b,m68_move____b,m68_move____b,m68_move____b,
/* 00011001ss */ m68_move____b,m68_move____b,m68_move____b,m68_move____b,
/* 00011010ss */ m68_move____b,m68_move____b,m68_move____b,m68_move____b,
/* 00011011ss */ m68_move____b,m68_move____b,m68_move____b,m68_move____b,
/* 00011100ss */ m68_move____b,m68_move____b,m68_move____b,m68_move____b,
/* 00011101ss */ m68_move____b,m68_move____b,m68_move____b,m68_move____b,
/* 00011110ss */ m68_move____b,m68_move____b,m68_move____b,m68_move____b,
/* 00011111ss */ m68_move____b,m68_move____b,m68_move____b,m68_move____b,
/* 00100000ss */ m68_move____l,m68_move____l,m68_move____l,m68_move____l,
/* 00100001ss */ m68_move____l,m68_move____l,m68_move____l,m68_move____l,
/* 00100010ss */ m68_move____l,m68_move____l,m68_move____l,m68_move____l,
/* 00100011ss */ m68_move____l,m68_move____l,m68_move____l,m68_move____l,
/* 00100100ss */ m68_move____l,m68_move____l,m68_move____l,m68_move____l,
/* 00100101ss */ m68_move____l,m68_move____l,m68_move____l,m68_move____l,
/* 00100110ss */ m68_move____l,m68_move____l,m68_move____l,m68_move____l,
/* 00100111ss */ m68_move____l,m68_move____l,m68_move____l,m68_move____l,
/* 00101000ss */ m68_move____l,m68_move____l,m68_move____l,m68_move____l,
/* 00101001ss */ m68_move____l,m68_move____l,m68_move____l,m68_move____l,
/* 00101010ss */ m68_move____l,m68_move____l,m68_move____l,m68_move____l,
/* 00101011ss */ m68_move____l,m68_move____l,m68_move____l,m68_move____l,
/* 00101100ss */ m68_move____l,m68_move____l,m68_move____l,m68_move____l,
/* 00101101ss */ m68_move____l,m68_move____l,m68_move____l,m68_move____l,
/* 00101110ss */ m68_move____l,m68_move____l,m68_move____l,m68_move____l,
/* 00101111ss */ m68_move____l,m68_move____l,m68_move____l,m68_move____l,
/* 00110000ss */ m68_move____w,m68_move____w,m68_move____w,m68_move____w,
/* 00110001ss */ m68_move____w,m68_move____w,m68_move____w,m68_move____w,
/* 00110010ss */ m68_move____w,m68_move____w,m68_move____w,m68_move____w,
/* 00110011ss */ m68_move____w,m68_move____w,m68_move____w,m68_move____w,
/* 00110100ss */ m68_move____w,m68_move____w,m68_move____w,m68_move____w,
/* 00110101ss */ m68_move____w,m68_move____w,m68_move____w,m68_move____w,
/* 00110110ss */ m68_move____w,m68_move____w,m68_move____w,m68_move____w,
/* 00110111ss */ m68_move____w,m68_move____w,m68_move____w,m68_move____w,
/* 00111000ss */ m68_move____w,m68_move____w,m68_move____w,m68_move____w,
/* 00111001ss */ m68_move____w,m68_move____w,m68_move____w,m68_move____w,
/* 00111010ss */ m68_move____w,m68_move____w,m68_move____w,m68_move____w,
/* 00111011ss */ m68_move____w,m68_move____w,m68_move____w,m68_move____w,
/* 00111100ss */ m68_move____w,m68_move____w,m68_move____w,m68_move____w,
/* 00111101ss */ m68_move____w,m68_move____w,m68_move____w,m68_move____w,
/* 00111110ss */ m68_move____w,m68_move____w,m68_move____w,m68_move____w,
/* 00111111ss */ m68_move____w,m68_move____w,m68_move____w,m68_move____w,
/* 01000000ss */ m68_negx____b,m68_negx____w,m68_negx____l,m68_movefsr_w,
/* 01000001ss */ m68_unrecog_x,m68_unrecog_x,m68_chk___n_w,m68_lea___n_l,
/* 01000010ss */ m68_clr_____b,m68_clr_____w,m68_clr_____l,m68_movefcc_w,
/* 01000011ss */ m68_unrecog_x,m68_unrecog_x,m68_chk___n_w,m68_lea___n_l,
/* 01000100ss */ m68_neg_____b,m68_neg_____w,m68_neg_____l,m68_move2cc_w,
/* 01000101ss */ m68_unrecog_x,m68_unrecog_x,m68_chk___n_w,m68_lea___n_l,
/* 01000110ss */ m68_not_____b,m68_not_____w,m68_not_____l,m68_move2sr_w,
/* 01000111ss */ m68_unrecog_x,m68_unrecog_x,m68_chk___n_w,m68_lea___n_l,
/* 01001000ss */ m68_nbcd____b,m68_pea_____l,m68_movem___w,m68_movem___l,
/* 01001001ss */ m68_unrecog_x,m68_unrecog_x,m68_chk___n_w,m68_lea___n_l,
/* 01001010ss */ m68_tst_____b,m68_tst_____w,m68_tst_____l,m68_tas_____b,
/* 01001011ss */ m68_unrecog_x,m68_unrecog_x,m68_chk___n_w,m68_lea___n_l,
/* 01001100ss */ m68_unrecog_x,m68_unrecog_x,m68_movem_r_w,m68_movem_r_l,
/* 01001101ss */ m68_unrecog_x,m68_unrecog_x,m68_chk___n_w,m68_lea___n_l,
/* 01001110ss */ m68_unrecog_x,m68_special_x,m68_jsr_____x,m68_jmp_____x,
/* 01001111ss */ m68_unrecog_x,m68_unrecog_x,m68_chk___n_w,m68_lea___n_l,
/* 01010000ss */ m68_addq__n_b,m68_addq__n_w,m68_addq__n_l,m68_st______x,
/* 01010001ss */ m68_subq__n_b,m68_subq__n_w,m68_subq__n_l,m68_sf______x,
/* 01010010ss */ m68_addq__n_b,m68_addq__n_w,m68_addq__n_l,m68_shi_____x,
/* 01010011ss */ m68_subq__n_b,m68_subq__n_w,m68_subq__n_l,m68_sls_____x,
/* 01010100ss */ m68_addq__n_b,m68_addq__n_w,m68_addq__n_l,m68_scc_____x,
/* 01010101ss */ m68_subq__n_b,m68_subq__n_w,m68_subq__n_l,m68_scs_____x,
/* 01010110ss */ m68_addq__n_b,m68_addq__n_w,m68_addq__n_l,m68_sne_____x,
/* 01010111ss */ m68_subq__n_b,m68_subq__n_w,m68_subq__n_l,m68_seq_____x,
/* 01011000ss */ m68_addq__n_b,m68_addq__n_w,m68_addq__n_l,m68_svc_____x,
/* 01011001ss */ m68_subq__n_b,m68_subq__n_w,m68_subq__n_l,m68_svs_____x,
/* 01011010ss */ m68_addq__n_b,m68_addq__n_w,m68_addq__n_l,m68_spl_____x,
/* 01011011ss */ m68_subq__n_b,m68_subq__n_w,m68_subq__n_l,m68_smi_____x,
/* 01011100ss */ m68_addq__n_b,m68_addq__n_w,m68_addq__n_l,m68_sge_____x,
/* 01011101ss */ m68_subq__n_b,m68_subq__n_w,m68_subq__n_l,m68_slt_____x,
/* 01011110ss */ m68_addq__n_b,m68_addq__n_w,m68_addq__n_l,m68_sgt_____x,
/* 01011111ss */ m68_subq__n_b,m68_subq__n_w,m68_subq__n_l,m68_sle_____x,
/* 01100000ss */ m68_bra_____x,m68_bra_____x,m68_bra_____x,m68_bra_____x,
/* 01100001ss */ m68_bsr_____x,m68_bsr_____x,m68_bsr_____x,m68_bsr_____x,
/* 01100010ss */ m68_bhi_____x,m68_bhi_____x,m68_bhi_____x,m68_bhi_____x,
/* 01100011ss */ m68_bls_____x,m68_bls_____x,m68_bls_____x,m68_bls_____x,
/* 01100100ss */ m68_bcc_____x,m68_bcc_____x,m68_bcc_____x,m68_bcc_____x,
/* 01100101ss */ m68_bcs_____x,m68_bcs_____x,m68_bcs_____x,m68_bcs_____x,
/* 01100110ss */ m68_bne_____x,m68_bne_____x,m68_bne_____x,m68_bne_____x,
/* 01100111ss */ m68_beq_____x,m68_beq_____x,m68_beq_____x,m68_beq_____x,
/* 01101000ss */ m68_bvc_____x,m68_bvc_____x,m68_bvc_____x,m68_bvc_____x,
/* 01101001ss */ m68_bvs_____x,m68_bvs_____x,m68_bvs_____x,m68_bvs_____x,
/* 01101010ss */ m68_bpl_____x,m68_bpl_____x,m68_bpl_____x,m68_bpl_____x,
/* 01101011ss */ m68_bmi_____x,m68_bmi_____x,m68_bmi_____x,m68_bmi_____x,
/* 01101100ss */ m68_bge_____x,m68_bge_____x,m68_bge_____x,m68_bge_____x,
/* 01101101ss */ m68_blt_____x,m68_blt_____x,m68_blt_____x,m68_blt_____x,
/* 01101110ss */ m68_bgt_____x,m68_bgt_____x,m68_bgt_____x,m68_bgt_____x,
/* 01101111ss */ m68_ble_____x,m68_ble_____x,m68_ble_____x,m68_ble_____x,
/* 01110000ss */ m68_moveq_n_l,m68_moveq_n_l,m68_moveq_n_l,m68_moveq_n_l,
/* 01110001ss */ m68_unrecog_x,m68_unrecog_x,m68_unrecog_x,m68_unrecog_x,
/* 01110010ss */ m68_moveq_n_l,m68_moveq_n_l,m68_moveq_n_l,m68_moveq_n_l,
/* 01110011ss */ m68_unrecog_x,m68_unrecog_x,m68_unrecog_x,m68_unrecog_x,
/* 01110100ss */ m68_moveq_n_l,m68_moveq_n_l,m68_moveq_n_l,m68_moveq_n_l,
/* 01110101ss */ m68_unrecog_x,m68_unrecog_x,m68_unrecog_x,m68_unrecog_x,
/* 01110110ss */ m68_moveq_n_l,m68_moveq_n_l,m68_moveq_n_l,m68_moveq_n_l,
/* 01110111ss */ m68_unrecog_x,m68_unrecog_x,m68_unrecog_x,m68_unrecog_x,
/* 01111000ss */ m68_moveq_n_l,m68_moveq_n_l,m68_moveq_n_l,m68_moveq_n_l,
/* 01111001ss */ m68_unrecog_x,m68_unrecog_x,m68_unrecog_x,m68_unrecog_x,
/* 01111010ss */ m68_moveq_n_l,m68_moveq_n_l,m68_moveq_n_l,m68_moveq_n_l,
/* 01111011ss */ m68_unrecog_x,m68_unrecog_x,m68_unrecog_x,m68_unrecog_x,
/* 01111100ss */ m68_moveq_n_l,m68_moveq_n_l,m68_moveq_n_l,m68_moveq_n_l,
/* 01111101ss */ m68_unrecog_x,m68_unrecog_x,m68_unrecog_x,m68_unrecog_x,
/* 01111110ss */ m68_moveq_n_l,m68_moveq_n_l,m68_moveq_n_l,m68_moveq_n_l,
/* 01111111ss */ m68_unrecog_x,m68_unrecog_x,m68_unrecog_x,m68_unrecog_x,
/* 10000000ss */ m68_or__d_n_b,m68_or__d_n_w,m68_or__d_n_l,m68_divu__n_w,
/* 10000001ss */ m68_or__e_n_b,m68_or__e_n_w,m68_or__e_n_l,m68_divs__n_w,
/* 10000010ss */ m68_or__d_n_b,m68_or__d_n_w,m68_or__d_n_l,m68_divu__n_w,
/* 10000011ss */ m68_or__e_n_b,m68_or__e_n_w,m68_or__e_n_l,m68_divs__n_w,
/* 10000100ss */ m68_or__d_n_b,m68_or__d_n_w,m68_or__d_n_l,m68_divu__n_w,
/* 10000101ss */ m68_or__e_n_b,m68_or__e_n_w,m68_or__e_n_l,m68_divs__n_w,
/* 10000110ss */ m68_or__d_n_b,m68_or__d_n_w,m68_or__d_n_l,m68_divu__n_w,
/* 10000111ss */ m68_or__e_n_b,m68_or__e_n_w,m68_or__e_n_l,m68_divs__n_w,
/* 10001000ss */ m68_or__d_n_b,m68_or__d_n_w,m68_or__d_n_l,m68_divu__n_w,
/* 10001001ss */ m68_or__e_n_b,m68_or__e_n_w,m68_or__e_n_l,m68_divs__n_w,
/* 10001010ss */ m68_or__d_n_b,m68_or__d_n_w,m68_or__d_n_l,m68_divu__n_w,
/* 10001011ss */ m68_or__e_n_b,m68_or__e_n_w,m68_or__e_n_l,m68_divs__n_w,
/* 10001100ss */ m68_or__d_n_b,m68_or__d_n_w,m68_or__d_n_l,m68_divu__n_w,
/* 10001101ss */ m68_or__e_n_b,m68_or__e_n_w,m68_or__e_n_l,m68_divs__n_w,
/* 10001110ss */ m68_or__d_n_b,m68_or__d_n_w,m68_or__d_n_l,m68_divu__n_w,
/* 10001111ss */ m68_or__e_n_b,m68_or__e_n_w,m68_or__e_n_l,m68_divs__n_w,
/* 10010000ss */ m68_sub_d_n_b,m68_sub_d_n_w,m68_sub_d_n_l,m68_suba__n_w,
/* 10010001ss */ m68_sub_e_n_b,m68_sub_e_n_w,m68_sub_e_n_l,m68_suba__n_l,
/* 10010010ss */ m68_sub_d_n_b,m68_sub_d_n_w,m68_sub_d_n_l,m68_suba__n_w,
/* 10010011ss */ m68_sub_e_n_b,m68_sub_e_n_w,m68_sub_e_n_l,m68_suba__n_l,
/* 10010100ss */ m68_sub_d_n_b,m68_sub_d_n_w,m68_sub_d_n_l,m68_suba__n_w,
/* 10010101ss */ m68_sub_e_n_b,m68_sub_e_n_w,m68_sub_e_n_l,m68_suba__n_l,
/* 10010110ss */ m68_sub_d_n_b,m68_sub_d_n_w,m68_sub_d_n_l,m68_suba__n_w,
/* 10010111ss */ m68_sub_e_n_b,m68_sub_e_n_w,m68_sub_e_n_l,m68_suba__n_l,
/* 10011000ss */ m68_sub_d_n_b,m68_sub_d_n_w,m68_sub_d_n_l,m68_suba__n_w,
/* 10011001ss */ m68_sub_e_n_b,m68_sub_e_n_w,m68_sub_e_n_l,m68_suba__n_l,
/* 10011010ss */ m68_sub_d_n_b,m68_sub_d_n_w,m68_sub_d_n_l,m68_suba__n_w,
/* 10011011ss */ m68_sub_e_n_b,m68_sub_e_n_w,m68_sub_e_n_l,m68_suba__n_l,
/* 10011100ss */ m68_sub_d_n_b,m68_sub_d_n_w,m68_sub_d_n_l,m68_suba__n_w,
/* 10011101ss */ m68_sub_e_n_b,m68_sub_e_n_w,m68_sub_e_n_l,m68_suba__n_l,
/* 10011110ss */ m68_sub_d_n_b,m68_sub_d_n_w,m68_sub_d_n_l,m68_suba__n_w,
/* 10011111ss */ m68_sub_e_n_b,m68_sub_e_n_w,m68_sub_e_n_l,m68_suba__n_l,
/* 10100000ss */ m68_unrecog_x,m68_unrecog_x,m68_unrecog_x,m68_unrecog_x,
/* 10100001ss */ m68_unrecog_x,m68_unrecog_x,m68_unrecog_x,m68_unrecog_x,
/* 10100010ss */ m68_unrecog_x,m68_unrecog_x,m68_unrecog_x,m68_unrecog_x,
/* 10100011ss */ m68_unrecog_x,m68_unrecog_x,m68_unrecog_x,m68_unrecog_x,
/* 10100100ss */ m68_unrecog_x,m68_unrecog_x,m68_unrecog_x,m68_unrecog_x,
/* 10100101ss */ m68_unrecog_x,m68_unrecog_x,m68_unrecog_x,m68_unrecog_x,
/* 10100110ss */ m68_unrecog_x,m68_unrecog_x,m68_unrecog_x,m68_unrecog_x,
/* 10100111ss */ m68_unrecog_x,m68_unrecog_x,m68_unrecog_x,m68_unrecog_x,
/* 10101000ss */ m68_unrecog_x,m68_unrecog_x,m68_unrecog_x,m68_unrecog_x,
/* 10101001ss */ m68_unrecog_x,m68_unrecog_x,m68_unrecog_x,m68_unrecog_x,
/* 10101010ss */ m68_unrecog_x,m68_unrecog_x,m68_unrecog_x,m68_unrecog_x,
/* 10101011ss */ m68_unrecog_x,m68_unrecog_x,m68_unrecog_x,m68_unrecog_x,
/* 10101100ss */ m68_unrecog_x,m68_unrecog_x,m68_unrecog_x,m68_unrecog_x,
/* 10101101ss */ m68_unrecog_x,m68_unrecog_x,m68_unrecog_x,m68_unrecog_x,
/* 10101110ss */ m68_unrecog_x,m68_unrecog_x,m68_unrecog_x,m68_unrecog_x,
/* 10101111ss */ m68_unrecog_x,m68_unrecog_x,m68_unrecog_x,m68_unrecog_x,
/* 10110000ss */ m68_cmp_d_n_b,m68_cmp_d_n_w,m68_cmp_d_n_l,m68_cmpa__n_w,
/* 10110001ss */ m68_eor_e_n_b,m68_eor_e_n_w,m68_eor_e_n_l,m68_cmpa__n_l,
/* 10110010ss */ m68_cmp_d_n_b,m68_cmp_d_n_w,m68_cmp_d_n_l,m68_cmpa__n_w,
/* 10110011ss */ m68_eor_e_n_b,m68_eor_e_n_w,m68_eor_e_n_l,m68_cmpa__n_l,
/* 10110100ss */ m68_cmp_d_n_b,m68_cmp_d_n_w,m68_cmp_d_n_l,m68_cmpa__n_w,
/* 10110101ss */ m68_eor_e_n_b,m68_eor_e_n_w,m68_eor_e_n_l,m68_cmpa__n_l,
/* 10110110ss */ m68_cmp_d_n_b,m68_cmp_d_n_w,m68_cmp_d_n_l,m68_cmpa__n_w,
/* 10110111ss */ m68_eor_e_n_b,m68_eor_e_n_w,m68_eor_e_n_l,m68_cmpa__n_l,
/* 10111000ss */ m68_cmp_d_n_b,m68_cmp_d_n_w,m68_cmp_d_n_l,m68_cmpa__n_w,
/* 10111001ss */ m68_eor_e_n_b,m68_eor_e_n_w,m68_eor_e_n_l,m68_cmpa__n_l,
/* 10111010ss */ m68_cmp_d_n_b,m68_cmp_d_n_w,m68_cmp_d_n_l,m68_cmpa__n_w,
/* 10111011ss */ m68_eor_e_n_b,m68_eor_e_n_w,m68_eor_e_n_l,m68_cmpa__n_l,
/* 10111100ss */ m68_cmp_d_n_b,m68_cmp_d_n_w,m68_cmp_d_n_l,m68_cmpa__n_w,
/* 10111101ss */ m68_eor_e_n_b,m68_eor_e_n_w,m68_eor_e_n_l,m68_cmpa__n_l,
/* 10111110ss */ m68_cmp_d_n_b,m68_cmp_d_n_w,m68_cmp_d_n_l,m68_cmpa__n_w,
/* 10111111ss */ m68_eor_e_n_b,m68_eor_e_n_w,m68_eor_e_n_l,m68_cmpa__n_l,
/* 11000000ss */ m68_and_d_n_b,m68_and_d_n_w,m68_and_d_n_l,m68_mulu__n_w,
/* 11000001ss */ m68_and_e_n_b,m68_and_e_n_w,m68_and_e_n_l,m68_muls__n_w,
/* 11000010ss */ m68_and_d_n_b,m68_and_d_n_w,m68_and_d_n_l,m68_mulu__n_w,
/* 11000011ss */ m68_and_e_n_b,m68_and_e_n_w,m68_and_e_n_l,m68_muls__n_w,
/* 11000100ss */ m68_and_d_n_b,m68_and_d_n_w,m68_and_d_n_l,m68_mulu__n_w,
/* 11000101ss */ m68_and_e_n_b,m68_and_e_n_w,m68_and_e_n_l,m68_muls__n_w,
/* 11000110ss */ m68_and_d_n_b,m68_and_d_n_w,m68_and_d_n_l,m68_mulu__n_w,
/* 11000111ss */ m68_and_e_n_b,m68_and_e_n_w,m68_and_e_n_l,m68_muls__n_w,
/* 11001000ss */ m68_and_d_n_b,m68_and_d_n_w,m68_and_d_n_l,m68_mulu__n_w,
/* 11001001ss */ m68_and_e_n_b,m68_and_e_n_w,m68_and_e_n_l,m68_muls__n_w,
/* 11001010ss */ m68_and_d_n_b,m68_and_d_n_w,m68_and_d_n_l,m68_mulu__n_w,
/* 11001011ss */ m68_and_e_n_b,m68_and_e_n_w,m68_and_e_n_l,m68_muls__n_w,
/* 11001100ss */ m68_and_d_n_b,m68_and_d_n_w,m68_and_d_n_l,m68_mulu__n_w,
/* 11001101ss */ m68_and_e_n_b,m68_and_e_n_w,m68_and_e_n_l,m68_muls__n_w,
/* 11001110ss */ m68_and_d_n_b,m68_and_d_n_w,m68_and_d_n_l,m68_mulu__n_w,
/* 11001111ss */ m68_and_e_n_b,m68_and_e_n_w,m68_and_e_n_l,m68_muls__n_w,
/* 11010000ss */ m68_add_d_n_b,m68_add_d_n_w,m68_add_d_n_l,m68_adda__n_w,
/* 11010001ss */ m68_add_e_n_b,m68_add_e_n_w,m68_add_e_n_l,m68_adda__n_l,
/* 11010010ss */ m68_add_d_n_b,m68_add_d_n_w,m68_add_d_n_l,m68_adda__n_w,
/* 11010011ss */ m68_add_e_n_b,m68_add_e_n_w,m68_add_e_n_l,m68_adda__n_l,
/* 11010100ss */ m68_add_d_n_b,m68_add_d_n_w,m68_add_d_n_l,m68_adda__n_w,
/* 11010101ss */ m68_add_e_n_b,m68_add_e_n_w,m68_add_e_n_l,m68_adda__n_l,
/* 11010110ss */ m68_add_d_n_b,m68_add_d_n_w,m68_add_d_n_l,m68_adda__n_w,
/* 11010111ss */ m68_add_e_n_b,m68_add_e_n_w,m68_add_e_n_l,m68_adda__n_l,
/* 11011000ss */ m68_add_d_n_b,m68_add_d_n_w,m68_add_d_n_l,m68_adda__n_w,
/* 11011001ss */ m68_add_e_n_b,m68_add_e_n_w,m68_add_e_n_l,m68_adda__n_l,
/* 11011010ss */ m68_add_d_n_b,m68_add_d_n_w,m68_add_d_n_l,m68_adda__n_w,
/* 11011011ss */ m68_add_e_n_b,m68_add_e_n_w,m68_add_e_n_l,m68_adda__n_l,
/* 11011100ss */ m68_add_d_n_b,m68_add_d_n_w,m68_add_d_n_l,m68_adda__n_w,
/* 11011101ss */ m68_add_e_n_b,m68_add_e_n_w,m68_add_e_n_l,m68_adda__n_l,
/* 11011110ss */ m68_add_d_n_b,m68_add_d_n_w,m68_add_d_n_l,m68_adda__n_w,
/* 11011111ss */ m68_add_e_n_b,m68_add_e_n_w,m68_add_e_n_l,m68_adda__n_l,
/* 11100000ss */ m68_shr_r_n_b,m68_shr_r_n_w,m68_shr_r_n_l,m68_asr_m___w,
/* 11100001ss */ m68_shl_r_n_b,m68_shl_r_n_w,m68_shl_r_n_l,m68_asl_m___w,
/* 11100010ss */ m68_shr_r_n_b,m68_shr_r_n_w,m68_shr_r_n_l,m68_lsr_m___w,
/* 11100011ss */ m68_shl_r_n_b,m68_shl_r_n_w,m68_shl_r_n_l,m68_lsl_m___w,
/* 11100100ss */ m68_shr_r_n_b,m68_shr_r_n_w,m68_shr_r_n_l,m68_roxr_m__w,
/* 11100101ss */ m68_shl_r_n_b,m68_shl_r_n_w,m68_shl_r_n_l,m68_roxl_m__w,
/* 11100110ss */ m68_shr_r_n_b,m68_shr_r_n_w,m68_shr_r_n_l,m68_ror_m___w,
/* 11100111ss */ m68_shl_r_n_b,m68_shl_r_n_w,m68_shl_r_n_l,m68_rol_m___w,
/* 11101000ss */ m68_shr_r_n_b,m68_shr_r_n_w,m68_shr_r_n_l,m68_unrecog_x,
/* 11101001ss */ m68_shl_r_n_b,m68_shl_r_n_w,m68_shl_r_n_l,m68_unrecog_x,
/* 11101010ss */ m68_shr_r_n_b,m68_shr_r_n_w,m68_shr_r_n_l,m68_unrecog_x,
/* 11101011ss */ m68_shl_r_n_b,m68_shl_r_n_w,m68_shl_r_n_l,m68_unrecog_x,
/* 11101100ss */ m68_shr_r_n_b,m68_shr_r_n_w,m68_shr_r_n_l,m68_unrecog_x,
/* 11101101ss */ m68_shl_r_n_b,m68_shl_r_n_w,m68_shl_r_n_l,m68_unrecog_x,
/* 11101110ss */ m68_shr_r_n_b,m68_shr_r_n_w,m68_shr_r_n_l,m68_unrecog_x,
/* 11101111ss */ m68_shl_r_n_b,m68_shl_r_n_w,m68_shl_r_n_l,m68_unrecog_x,
/* 11110000ss */ m68_unrecog_x,m68_unrecog_x,m68_unrecog_x,m68_unrecog_x,
/* 11110001ss */ m68_unrecog_x,m68_unrecog_x,m68_unrecog_x,m68_unrecog_x,
/* 11110010ss */ m68_unrecog_x,m68_unrecog_x,m68_unrecog_x,m68_unrecog_x,
/* 11110011ss */ m68_unrecog_x,m68_unrecog_x,m68_unrecog_x,m68_unrecog_x,
/* 11110100ss */ m68_unrecog_x,m68_unrecog_x,m68_unrecog_x,m68_unrecog_x,
/* 11110101ss */ m68_unrecog_x,m68_unrecog_x,m68_unrecog_x,m68_unrecog_x,
/* 11110110ss */ m68_unrecog_x,m68_unrecog_x,m68_unrecog_x,m68_unrecog_x,
/* 11110111ss */ m68_unrecog_x,m68_unrecog_x,m68_unrecog_x,m68_unrecog_x,
/* 11111000ss */ m68_unrecog_x,m68_unrecog_x,m68_unrecog_x,m68_unrecog_x,
/* 11111001ss */ m68_unrecog_x,m68_unrecog_x,m68_unrecog_x,m68_unrecog_x,
/* 11111010ss */ m68_unrecog_x,m68_unrecog_x,m68_unrecog_x,m68_unrecog_x,
/* 11111011ss */ m68_unrecog_x,m68_unrecog_x,m68_unrecog_x,m68_unrecog_x,
/* 11111100ss */ m68_unrecog_x,m68_unrecog_x,m68_unrecog_x,m68_unrecog_x,
/* 11111101ss */ m68_unrecog_x,m68_unrecog_x,m68_unrecog_x,m68_unrecog_x,
/* 11111110ss */ m68_fecall__x,m68_fecall__x,m68_fecall__x,m68_fecall__x,
/* 11111111ss */ m68_doscall_x,m68_doscall_x,m68_doscall_x,m68_doscall_x};

void cpudebug_disassemble(int n) {
	while(n--) {
		dword addr = debugpc;
		cpudebug_printf("%08x: ", addr & 0xffffff);
		isize = 0;
		inst = fetch();
		debugmap[inst >> 6]();
		while(addr < debugpc) {
			cpudebug_printf("%04x ", cpudebug_fetch(addr) & 0xffff);
			addr += 2;
		}
		while(isize < 10) {
			cpudebug_printf("     ");
			isize += 2;
		}
		cpudebug_printf("%s\n", sdebug);
	}
}
