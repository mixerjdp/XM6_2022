;
; X68000 EMULATOR "XM6"
;
; Copyright (C) 2001-2004 ＰＩ．(ytanaka@ipc-tokai.or.jp)
; [ レンダラ(アセンブラ) ]
;

;
; 外部宣言
;
	%ifdef	OMF
		section	_TEXT use32 align=16 class=CODE
	%else
		section	.text
	%endif
		bits	32

		global	_RendTextMem
		global	_RendTextPal
		global	_RendTextAll
		global	_RendTextCopy

		global	_Rend1024A
		global	_Rend1024B
		global	_Rend1024C
		global	_Rend1024D
		global	_Rend1024E
		global	_Rend1024F
		global	_Rend16A
		global	_Rend16B
		global	_Rend16C
		global	_Rend16D
		global	_Rend16E
		global	_Rend16F
		global	_Rend16G
		global	_Rend16H
		global	_Rend256A
		global	_Rend256B
		global	_Rend256C
		global	_Rend256D
		global	_Rend64KA
		global	_Rend64KB

		global	_RendClrSprite
		global	_RendSprite
		global	_RendSpriteC
		global	_RendPCGNew
		global	_RendBG8
		global	_RendBG8C
		global	_RendBG8P
		global	_RendBG16
		global	_RendBG16C
		global	_RendBG16P

		global	_RendMix00
		global	_RendMix01
		global	_RendMix02
		global	_RendMix02C
		global	_RendMix03
		global	_RendMix03C
		global	_RendMix04
		global	_RendMix04C

		global	_RendGrp02
		global	_RendGrp02C
		global	_RendGrp03
		global	_RendGrp03C
		global	_RendGrp04

;
; レンダラ
; テキスト(水平垂直変換)
;
; void RendTextMem(const BYTE *tvram, BOOL *flag, BYTE *buf)
;
_RendTextMem:
		push	ebp
		mov	ebp,esp
		push	eax
		push	ebx
		push	ecx
		push	edx
		push	esi
		push	edi
; 初期設定
		mov	ecx,32
		mov	esi,[ebp+8]
		mov	ebx,[ebp+0ch]
		mov	edi,[ebp+10h]
		xor	edx,edx
; ループ
.loop:
		cmp	edx,[ebx]
		jnz	.exec
; この32dotは処理しない
		add	edi,16
		add	esi,4
		add	ebx,4
		dec	ecx
		jnz	.loop
		jmp	.exit
; この32dotは処理
.exec:
		push	ecx
		push	ebx
		mov	eax,[esi]
		mov	ebx,[esi+20000h]
		mov	ecx,[esi+40000h]
		mov	edx,[esi+60000h]
; 初期化とビットシフト
		rol	eax,16
		rol	ebx,16
		rol	ecx,16
		rol	edx,16
; b31
		add	edx,edx
		adc	ebp,ebp
		add	ecx,ecx
		adc	ebp,ebp
		add	ebx,ebx
		adc	ebp,ebp
		add	eax,eax
		adc	ebp,ebp
; b30
		add	edx,edx
		adc	ebp,ebp
		add	ecx,ecx
		adc	ebp,ebp
		add	ebx,ebx
		adc	ebp,ebp
		add	eax,eax
		adc	ebp,ebp
; b29
		add	edx,edx
		adc	ebp,ebp
		add	ecx,ecx
		adc	ebp,ebp
		add	ebx,ebx
		adc	ebp,ebp
		add	eax,eax
		adc	ebp,ebp
; b28
		add	edx,edx
		adc	ebp,ebp
		add	ecx,ecx
		adc	ebp,ebp
		add	ebx,ebx
		adc	ebp,ebp
		add	eax,eax
		adc	ebp,ebp
; b27
		add	edx,edx
		adc	ebp,ebp
		add	ecx,ecx
		adc	ebp,ebp
		add	ebx,ebx
		adc	ebp,ebp
		add	eax,eax
		adc	ebp,ebp
; b26
		add	edx,edx
		adc	ebp,ebp
		add	ecx,ecx
		adc	ebp,ebp
		add	ebx,ebx
		adc	ebp,ebp
		add	eax,eax
		adc	ebp,ebp
; b25
		add	edx,edx
		adc	ebp,ebp
		add	ecx,ecx
		adc	ebp,ebp
		add	ebx,ebx
		adc	ebp,ebp
		add	eax,eax
		adc	ebp,ebp
; b24
		add	edx,edx
		adc	ebp,ebp
		add	ecx,ecx
		adc	ebp,ebp
		add	ebx,ebx
		adc	ebp,ebp
		add	eax,eax
		adc	ebp,ebp
; 書き込み
		mov	[edi],ebp
; b23
		add	edx,edx
		adc	ebp,ebp
		add	ecx,ecx
		adc	ebp,ebp
		add	ebx,ebx
		adc	ebp,ebp
		add	eax,eax
		adc	ebp,ebp
; b22
		add	edx,edx
		adc	ebp,ebp
		add	ecx,ecx
		adc	ebp,ebp
		add	ebx,ebx
		adc	ebp,ebp
		add	eax,eax
		adc	ebp,ebp
; b21
		add	edx,edx
		adc	ebp,ebp
		add	ecx,ecx
		adc	ebp,ebp
		add	ebx,ebx
		adc	ebp,ebp
		add	eax,eax
		adc	ebp,ebp
; b20
		add	edx,edx
		adc	ebp,ebp
		add	ecx,ecx
		adc	ebp,ebp
		add	ebx,ebx
		adc	ebp,ebp
		add	eax,eax
		adc	ebp,ebp
; b19
		add	edx,edx
		adc	ebp,ebp
		add	ecx,ecx
		adc	ebp,ebp
		add	ebx,ebx
		adc	ebp,ebp
		add	eax,eax
		adc	ebp,ebp
; b18
		add	edx,edx
		adc	ebp,ebp
		add	ecx,ecx
		adc	ebp,ebp
		add	ebx,ebx
		adc	ebp,ebp
		add	eax,eax
		adc	ebp,ebp
; b17
		add	edx,edx
		adc	ebp,ebp
		add	ecx,ecx
		adc	ebp,ebp
		add	ebx,ebx
		adc	ebp,ebp
		add	eax,eax
		adc	ebp,ebp
; b16
		add	edx,edx
		adc	ebp,ebp
		add	ecx,ecx
		adc	ebp,ebp
		add	ebx,ebx
		adc	ebp,ebp
		add	eax,eax
		adc	ebp,ebp
; 書き込み
		mov	[edi+4],ebp
; b15
		add	edx,edx
		adc	ebp,ebp
		add	ecx,ecx
		adc	ebp,ebp
		add	ebx,ebx
		adc	ebp,ebp
		add	eax,eax
		adc	ebp,ebp
; b14
		add	edx,edx
		adc	ebp,ebp
		add	ecx,ecx
		adc	ebp,ebp
		add	ebx,ebx
		adc	ebp,ebp
		add	eax,eax
		adc	ebp,ebp
; b13
		add	edx,edx
		adc	ebp,ebp
		add	ecx,ecx
		adc	ebp,ebp
		add	ebx,ebx
		adc	ebp,ebp
		add	eax,eax
		adc	ebp,ebp
; b12
		add	edx,edx
		adc	ebp,ebp
		add	ecx,ecx
		adc	ebp,ebp
		add	ebx,ebx
		adc	ebp,ebp
		add	eax,eax
		adc	ebp,ebp
; b11
		add	edx,edx
		adc	ebp,ebp
		add	ecx,ecx
		adc	ebp,ebp
		add	ebx,ebx
		adc	ebp,ebp
		add	eax,eax
		adc	ebp,ebp
; b10
		add	edx,edx
		adc	ebp,ebp
		add	ecx,ecx
		adc	ebp,ebp
		add	ebx,ebx
		adc	ebp,ebp
		add	eax,eax
		adc	ebp,ebp
; b9
		add	edx,edx
		adc	ebp,ebp
		add	ecx,ecx
		adc	ebp,ebp
		add	ebx,ebx
		adc	ebp,ebp
		add	eax,eax
		adc	ebp,ebp
; b8
		add	edx,edx
		adc	ebp,ebp
		add	ecx,ecx
		adc	ebp,ebp
		add	ebx,ebx
		adc	ebp,ebp
		add	eax,eax
		adc	ebp,ebp
; 書き込み
		mov	[edi+8],ebp
; b7
		add	edx,edx
		adc	ebp,ebp
		add	ecx,ecx
		adc	ebp,ebp
		add	ebx,ebx
		adc	ebp,ebp
		add	eax,eax
		adc	ebp,ebp
; b6
		add	edx,edx
		adc	ebp,ebp
		add	ecx,ecx
		adc	ebp,ebp
		add	ebx,ebx
		adc	ebp,ebp
		add	eax,eax
		adc	ebp,ebp
; b5
		add	edx,edx
		adc	ebp,ebp
		add	ecx,ecx
		adc	ebp,ebp
		add	ebx,ebx
		adc	ebp,ebp
		add	eax,eax
		adc	ebp,ebp
; b4
		add	edx,edx
		adc	ebp,ebp
		add	ecx,ecx
		adc	ebp,ebp
		add	ebx,ebx
		adc	ebp,ebp
		add	eax,eax
		adc	ebp,ebp
; b3
		add	edx,edx
		adc	ebp,ebp
		add	ecx,ecx
		adc	ebp,ebp
		add	ebx,ebx
		adc	ebp,ebp
		add	eax,eax
		adc	ebp,ebp
; b2
		add	edx,edx
		adc	ebp,ebp
		add	ecx,ecx
		adc	ebp,ebp
		add	ebx,ebx
		adc	ebp,ebp
		add	eax,eax
		adc	ebp,ebp
; b1
		add	edx,edx
		adc	ebp,ebp
		add	ecx,ecx
		adc	ebp,ebp
		add	ebx,ebx
		adc	ebp,ebp
		add	eax,eax
		adc	ebp,ebp
; b0
		add	edx,edx
		adc	ebp,ebp
		add	ecx,ecx
		adc	ebp,ebp
		add	ebx,ebx
		adc	ebp,ebp
		add	eax,eax
		adc	ebp,ebp
; 書き込み
		mov	[edi+12],ebp
; 次へ
		add	esi,4
		pop	ebx
		add	edi,16
		pop	ecx
		add	ebx,4
		xor	edx,edx
		dec	ecx
		jnz	near .loop
; 終了
.exit:
		pop	edi
		pop	esi
		pop	edx
		pop	ecx
		pop	ebx
		pop	eax
		pop	ebp
		ret

;
; レンダラ
; テキスト(パレット)
;
; void RendTextPal(BYTE *buf, DWORD *out, BOOL *flag, DWORD *pal)
;
_RendTextPal:
		push	ebp
		mov	ebp,esp
		push	eax
		push	ebx
		push	ecx
		push	edx
		push	esi
		push	edi
; パラメータ取得
		mov	esi,[ebp+8]
		mov	edi,[ebp+12]
		mov	ebx,[ebp+20]
		mov	ebp,[ebp+16]
		mov	ecx,32
		xor	edx,edx
; チェック
.loop:
		cmp	[ebp],edx
		jnz	.exec
		add	ebp,4
		add	esi,16
		add	edi,128
		dec	ecx
		jnz	.loop
		jmp	.exit
; この32dotは処理
.exec:
		mov	[ebp],edx
		push	ebp
		mov	edx,15
		push	ecx
; +0～+7
		mov	eax,[esi]
		mov	ebp,edx
		rol	eax,4
		and	ebp,eax
		mov	ecx,[ebx+ebp*4]
		rol	eax,4
		mov	ebp,edx
		mov	[edi],ecx
		and	ebp,eax
		mov	ecx,[ebx+ebp*4]
		rol	eax,4
		mov	ebp,edx
		mov	[edi+4],ecx
		and	ebp,eax
		mov	ecx,[ebx+ebp*4]
		rol	eax,4
		mov	ebp,edx
		mov	[edi+8],ecx
		and	ebp,eax
		mov	ecx,[ebx+ebp*4]
		rol	eax,4
		mov	ebp,edx
		mov	[edi+12],ecx
		and	ebp,eax
		mov	ecx,[ebx+ebp*4]
		rol	eax,4
		mov	ebp,edx
		mov	[edi+16],ecx
		and	ebp,eax
		mov	ecx,[ebx+ebp*4]
		rol	eax,4
		mov	ebp,edx
		mov	[edi+20],ecx
		and	ebp,eax
		mov	ecx,[ebx+ebp*4]
		rol	eax,4
		mov	ebp,edx
		mov	[edi+24],ecx
		and	ebp,eax
		mov	ecx,[ebx+ebp*4]
; +8～+15
		mov	eax,[esi+4]
		mov	ebp,edx
		rol	eax,4
		mov	[edi+28],ecx
		and	ebp,eax
		mov	ecx,[ebx+ebp*4]
		rol	eax,4
		mov	ebp,edx
		mov	[edi+32],ecx
		and	ebp,eax
		mov	ecx,[ebx+ebp*4]
		rol	eax,4
		mov	ebp,edx
		mov	[edi+36],ecx
		and	ebp,eax
		mov	ecx,[ebx+ebp*4]
		rol	eax,4
		mov	ebp,edx
		mov	[edi+40],ecx
		and	ebp,eax
		mov	ecx,[ebx+ebp*4]
		rol	eax,4
		mov	ebp,edx
		mov	[edi+44],ecx
		and	ebp,eax
		mov	ecx,[ebx+ebp*4]
		rol	eax,4
		mov	ebp,edx
		mov	[edi+48],ecx
		and	ebp,eax
		mov	ecx,[ebx+ebp*4]
		rol	eax,4
		mov	ebp,edx
		mov	[edi+52],ecx
		and	ebp,eax
		mov	ecx,[ebx+ebp*4]
		rol	eax,4
		mov	ebp,edx
		mov	[edi+56],ecx
		and	ebp,eax
		mov	ecx,[ebx+ebp*4]
; +16～+23
		mov	eax,[esi+8]
		mov	ebp,edx
		rol	eax,4
		mov	[edi+60],ecx
		and	ebp,eax
		mov	ecx,[ebx+ebp*4]
		rol	eax,4
		mov	ebp,edx
		mov	[edi+64],ecx
		and	ebp,eax
		mov	ecx,[ebx+ebp*4]
		rol	eax,4
		mov	ebp,edx
		mov	[edi+68],ecx
		and	ebp,eax
		mov	ecx,[ebx+ebp*4]
		rol	eax,4
		mov	ebp,edx
		mov	[edi+72],ecx
		and	ebp,eax
		mov	ecx,[ebx+ebp*4]
		rol	eax,4
		mov	ebp,edx
		mov	[edi+76],ecx
		and	ebp,eax
		mov	ecx,[ebx+ebp*4]
		rol	eax,4
		mov	ebp,edx
		mov	[edi+80],ecx
		and	ebp,eax
		mov	ecx,[ebx+ebp*4]
		rol	eax,4
		mov	ebp,edx
		mov	[edi+84],ecx
		and	ebp,eax
		mov	ecx,[ebx+ebp*4]
		rol	eax,4
		mov	ebp,edx
		mov	[edi+88],ecx
		and	ebp,eax
		mov	ecx,[ebx+ebp*4]
; +24～+31
		mov	eax,[esi+12]
		mov	ebp,edx
		rol	eax,4
		mov	[edi+92],ecx
		and	ebp,eax
		mov	ecx,[ebx+ebp*4]
		rol	eax,4
		mov	ebp,edx
		mov	[edi+96],ecx
		and	ebp,eax
		mov	ecx,[ebx+ebp*4]
		rol	eax,4
		mov	ebp,edx
		mov	[edi+100],ecx
		and	ebp,eax
		mov	ecx,[ebx+ebp*4]
		rol	eax,4
		mov	ebp,edx
		mov	[edi+104],ecx
		and	ebp,eax
		mov	ecx,[ebx+ebp*4]
		rol	eax,4
		mov	ebp,edx
		mov	[edi+108],ecx
		and	ebp,eax
		mov	ecx,[ebx+ebp*4]
		rol	eax,4
		mov	ebp,edx
		mov	[edi+112],ecx
		and	ebp,eax
		mov	ecx,[ebx+ebp*4]
		rol	eax,4
		mov	ebp,edx
		mov	[edi+116],ecx
		and	ebp,eax
		mov	ecx,[ebx+ebp*4]
		rol	eax,4
		mov	ebp,edx
		mov	[edi+120],ecx
		and	ebp,eax
		mov	ecx,[ebx+ebp*4]
		mov	[edi+124],ecx
; 次へ
		pop	ecx
		add	esi,16
		pop	ebp
		add	edi,128
		xor	edx,edx
		add	ebp,4
		dec	ecx
		jnz	near .loop
; 終了
.exit:
		pop	edi
		pop	esi
		pop	edx
		pop	ecx
		pop	ebx
		pop	eax
		pop	ebp
		ret

;
; レンダラ
; テキスト(全パレット)
;
; void RendTextAll(BYTE *buf, DWORD *out, DWORD *pal)
;
_RendTextAll:
		push	ebp
		mov	ebp,esp
		push	eax
		push	ebx
		push	ecx
		push	edx
		push	esi
		push	edi
; パラメータ取得
		mov	esi,[ebp+8]
		mov	edi,[ebp+12]
		mov	ebx,[ebp+16]
		mov	ecx,32
		mov	edx,15
.loop:
		push	ecx
; +0～+7
		mov	eax,[esi]
		mov	ebp,edx
		rol	eax,4
		and	ebp,eax
		mov	ecx,[ebx+ebp*4]
		rol	eax,4
		mov	ebp,edx
		mov	[edi],ecx
		and	ebp,eax
		mov	ecx,[ebx+ebp*4]
		rol	eax,4
		mov	ebp,edx
		mov	[edi+4],ecx
		and	ebp,eax
		mov	ecx,[ebx+ebp*4]
		rol	eax,4
		mov	ebp,edx
		mov	[edi+8],ecx
		and	ebp,eax
		mov	ecx,[ebx+ebp*4]
		rol	eax,4
		mov	ebp,edx
		mov	[edi+12],ecx
		and	ebp,eax
		mov	ecx,[ebx+ebp*4]
		rol	eax,4
		mov	ebp,edx
		mov	[edi+16],ecx
		and	ebp,eax
		mov	ecx,[ebx+ebp*4]
		rol	eax,4
		mov	ebp,edx
		mov	[edi+20],ecx
		and	ebp,eax
		mov	ecx,[ebx+ebp*4]
		rol	eax,4
		mov	ebp,edx
		mov	[edi+24],ecx
		and	ebp,eax
		mov	ecx,[ebx+ebp*4]
; +8～+15
		mov	eax,[esi+4]
		mov	ebp,edx
		rol	eax,4
		mov	[edi+28],ecx
		and	ebp,eax
		mov	ecx,[ebx+ebp*4]
		rol	eax,4
		mov	ebp,edx
		mov	[edi+32],ecx
		and	ebp,eax
		mov	ecx,[ebx+ebp*4]
		rol	eax,4
		mov	ebp,edx
		mov	[edi+36],ecx
		and	ebp,eax
		mov	ecx,[ebx+ebp*4]
		rol	eax,4
		mov	ebp,edx
		mov	[edi+40],ecx
		and	ebp,eax
		mov	ecx,[ebx+ebp*4]
		rol	eax,4
		mov	ebp,edx
		mov	[edi+44],ecx
		and	ebp,eax
		mov	ecx,[ebx+ebp*4]
		rol	eax,4
		mov	ebp,edx
		mov	[edi+48],ecx
		and	ebp,eax
		mov	ecx,[ebx+ebp*4]
		rol	eax,4
		mov	ebp,edx
		mov	[edi+52],ecx
		and	ebp,eax
		mov	ecx,[ebx+ebp*4]
		rol	eax,4
		mov	ebp,edx
		mov	[edi+56],ecx
		and	ebp,eax
		mov	ecx,[ebx+ebp*4]
; +16～+23
		mov	eax,[esi+8]
		mov	ebp,edx
		rol	eax,4
		mov	[edi+60],ecx
		and	ebp,eax
		mov	ecx,[ebx+ebp*4]
		rol	eax,4
		mov	ebp,edx
		mov	[edi+64],ecx
		and	ebp,eax
		mov	ecx,[ebx+ebp*4]
		rol	eax,4
		mov	ebp,edx
		mov	[edi+68],ecx
		and	ebp,eax
		mov	ecx,[ebx+ebp*4]
		rol	eax,4
		mov	ebp,edx
		mov	[edi+72],ecx
		and	ebp,eax
		mov	ecx,[ebx+ebp*4]
		rol	eax,4
		mov	ebp,edx
		mov	[edi+76],ecx
		and	ebp,eax
		mov	ecx,[ebx+ebp*4]
		rol	eax,4
		mov	ebp,edx
		mov	[edi+80],ecx
		and	ebp,eax
		mov	ecx,[ebx+ebp*4]
		rol	eax,4
		mov	ebp,edx
		mov	[edi+84],ecx
		and	ebp,eax
		mov	ecx,[ebx+ebp*4]
		rol	eax,4
		mov	ebp,edx
		mov	[edi+88],ecx
		and	ebp,eax
		mov	ecx,[ebx+ebp*4]
; +24～+31
		mov	eax,[esi+12]
		mov	ebp,edx
		rol	eax,4
		mov	[edi+92],ecx
		and	ebp,eax
		mov	ecx,[ebx+ebp*4]
		rol	eax,4
		mov	ebp,edx
		mov	[edi+96],ecx
		and	ebp,eax
		mov	ecx,[ebx+ebp*4]
		rol	eax,4
		mov	ebp,edx
		mov	[edi+100],ecx
		and	ebp,eax
		mov	ecx,[ebx+ebp*4]
		rol	eax,4
		mov	ebp,edx
		mov	[edi+104],ecx
		and	ebp,eax
		mov	ecx,[ebx+ebp*4]
		rol	eax,4
		mov	ebp,edx
		mov	[edi+108],ecx
		and	ebp,eax
		mov	ecx,[ebx+ebp*4]
		rol	eax,4
		mov	ebp,edx
		mov	[edi+112],ecx
		and	ebp,eax
		mov	ecx,[ebx+ebp*4]
		rol	eax,4
		mov	ebp,edx
		mov	[edi+116],ecx
		and	ebp,eax
		mov	ecx,[ebx+ebp*4]
		rol	eax,4
		mov	ebp,edx
		mov	[edi+120],ecx
		and	ebp,eax
		mov	ecx,[ebx+ebp*4]
		mov	[edi+124],ecx
; 次へ
		add	esi,16
		pop	ecx
		add	edi,128
		dec	ecx
		jnz	near .loop
; 終了
		pop	edi
		pop	esi
		pop	edx
		pop	ecx
		pop	ebx
		pop	eax
		pop	ebp
		ret

;
; レンダラ
; テキスト(ラスタコピー)
;
; void RendTextCopy(const BYTE *src, BYTE *dst, DWORD plane,
;			BOOL *textflag, BOOL *textmod);
;
_RendTextCopy:
		push	ebp
		mov	ebp,esp
		push	eax
		push	ebx
		push	ecx
		push	edx
		push	esi
		push	edi
; 初期設定
		mov	esi,[ebp+8]
		mov	edi,[ebp+12]
		mov	eax,[ebp+16]
		mov	ebx,[ebp+20]
		mov	edx,[ebp+24]
		mov	ecx,4
; プレーンチェック
.loop:
		shr	eax,1
		jnc	near .next
; このプレーンは処理
		push	eax
		push	ecx
		mov	ecx,4
; ラインループ
.line:
		push	ecx
		push	edx
		mov	ebp,4
		xor	edx,edx
; ブロックループ(32バイト)
.block:
; +0
		mov	eax,[esi]
		cmp	eax,[edi]
		setnz	cl
		mov	[edi],eax
		or	dl,cl
		or	[ebx],cl
; +1
		mov	eax,[esi+4]
		cmp	eax,[edi+4]
		setnz	cl
		mov	[edi+4],eax
		or	dl,cl
		or	[ebx+4],cl
; +2
		mov	eax,[esi+8]
		cmp	eax,[edi+8]
		setnz	cl
		mov	[edi+8],eax
		or	dl,cl
		or	[ebx+8],cl
; +3
		mov	eax,[esi+12]
		cmp	eax,[edi+12]
		setnz	cl
		mov	[edi+12],eax
		or	dl,cl
		or	[ebx+12],cl
; +4
		mov	eax,[esi+16]
		cmp	eax,[edi+16]
		setnz	cl
		mov	[edi+16],eax
		or	dl,cl
		or	[ebx+16],cl
; +5
		mov	eax,[esi+20]
		cmp	eax,[edi+20]
		setnz	cl
		mov	[edi+20],eax
		or	dl,cl
		or	[ebx+20],cl
; +6
		mov	eax,[esi+24]
		cmp	eax,[edi+24]
		setnz	cl
		mov	[edi+24],eax
		or	dl,cl
		or	[ebx+24],cl
; +7
		mov	eax,[esi+28]
		cmp	eax,[edi+28]
		setnz	cl
		mov	[edi+28],eax
		or	dl,cl
		or	[ebx+28],cl
; 次のブロックへ
		add	esi,32
		add	edi,32
		add	ebx,32
		dec	ebp
		jnz	near .block
; ライン終了
		mov	eax,edx
		pop	edx
		pop	ecx
		or	[edx],eax
; 次のラインへ
		add	edx,4
		dec	ecx
		jnz	near .line
; このプレーンは終了
		sub	esi,512
		sub	edi,512
		sub	ebx,512
		sub	edx,16
		pop	ecx
		pop	eax
; 次のプレーンへ
.next:
		add	esi,20000h
		add	edi,20000h
		dec	ecx
		jnz	near .loop
; 終了
		pop	edi
		pop	esi
		pop	edx
		pop	ecx
		pop	ebx
		pop	eax
		pop	ebp
		ret

;
; レンダラ
; グラフィック(1024色モード、ページ0,1、全て)
;
; int Rend1024A(const BYTE *gvram, DWORD *buf, const DWORD *pal)
;
_Rend1024A:
		push	ebp
		mov	ebp,esp
		push	ebx
		push	ecx
		push	edx
		push	esi
		push	edi
; 初期設定
		mov	esi,[ebp+8]
		mov	edi,[ebp+12]
		mov	ebp,[ebp+16]
		xor	edx,edx
		mov	ecx,128
; ループ
.loop:
		push	ecx
		mov	ecx,15
; +0(Block 0)
		mov	eax,[esi]
		mov	ebx,ecx
		and	ebx,eax
		mov	ebx,[ebp+ebx*4]
		cmp	ebx,[edi]
		setnz	dl
		mov	[edi],ebx
		or	dh,dl
		mov	[edi+4096],ebx
; +0(Block 1)
		shr	eax,4
		mov	ebx,ecx
		and	ebx,eax
		mov	ebx,[ebp+ebx*4]
		cmp	ebx,[edi+2048]
		setnz	dl
		mov	[edi+2048],ebx
		or	dh,dl
		mov	[edi+2048+4096],ebx
; +1(Block 0)
		shr	eax,12
		mov	ebx,ecx
		and	ebx,eax
		mov	ebx,[ebp+ebx*4]
		cmp	ebx,[edi+4]
		setnz	dl
		mov	[edi+4],ebx
		or	dh,dl
		mov	[edi+4+4096],ebx
; +1(Block 1)
		shr	eax,4
		mov	ebx,ecx
		and	ebx,eax
		mov	ebx,[ebp+ebx*4]
		cmp	ebx,[edi+4+2048]
		setnz	dl
		mov	[edi+4+2048],ebx
		or	dh,dl
		mov	[edi+4+2048+4096],ebx
; +2(Block 0)
		mov	eax,[esi+4]
		mov	ebx,ecx
		and	ebx,eax
		mov	ebx,[ebp+ebx*4]
		cmp	ebx,[edi+8]
		setnz	dl
		mov	[edi+8],ebx
		or	dh,dl
		mov	[edi+8+4096],ebx
; +2(Block 1)
		shr	eax,4
		mov	ebx,ecx
		and	ebx,eax
		mov	ebx,[ebp+ebx*4]
		cmp	ebx,[edi+8+2048]
		setnz	dl
		mov	[edi+8+2048],ebx
		or	dh,dl
		mov	[edi+8+2048+4096],ebx
; +3(Block 0)
		shr	eax,12
		mov	ebx,ecx
		and	ebx,eax
		mov	ebx,[ebp+ebx*4]
		cmp	ebx,[edi+12]
		setnz	dl
		mov	[edi+12],ebx
		or	dh,dl
		mov	[edi+12+4096],ebx
; +3(Block 1)
		shr	eax,4
		mov	ebx,ecx
		and	ebx,eax
		mov	ebx,[ebp+ebx*4]
		cmp	ebx,[edi+12+2048]
		setnz	dl
		mov	[edi+12+2048],ebx
		or	dh,dl
		mov	[edi+12+2048+4096],ebx
; 次へ
		add	esi,8
		add	edi,16
		pop	ecx
		dec	ecx
		jnz	near .loop
; 終了
		mov	eax,edx
		shr	eax,8
		pop	edi
		pop	esi
		pop	edx
		pop	ecx
		pop	ebx
		pop	ebp
		ret

;
; レンダラ
; グラフィック(1024色モード、ページ2,3、全て)
;
; int Rend1024B(const BYTE *gvram, DWORD *buf, const DWORD *pal)
;
_Rend1024B:
		push	ebp
		mov	ebp,esp
		push	ebx
		push	ecx
		push	edx
		push	esi
		push	edi
; 初期設定
		mov	esi,[ebp+8]
		mov	edi,[ebp+12]
		mov	ebp,[ebp+16]
		xor	edx,edx
		mov	ecx,128
; ループ
.loop:
		push	ecx
		mov	ecx,15
; +0(Block 2)
		mov	eax,[esi]
		shr	eax,8
		mov	ebx,ecx
		and	ebx,eax
		mov	ebx,[ebp+ebx*4]
		cmp	ebx,[edi]
		setnz	dl
		mov	[edi],ebx
		or	dh,dl
		mov	[edi+4096],ebx
; +0(Block 3)
		shr	eax,4
		mov	ebx,ecx
		and	ebx,eax
		mov	ebx,[ebp+ebx*4]
		cmp	ebx,[edi+2048]
		setnz	dl
		mov	[edi+2048],ebx
		or	dh,dl
		mov	[edi+2048+4096],ebx
; +1(Block 2)
		shr	eax,12
		mov	ebx,ecx
		and	ebx,eax
		mov	ebx,[ebp+ebx*4]
		cmp	ebx,[edi+4]
		setnz	dl
		mov	[edi+4],ebx
		or	dh,dl
		mov	[edi+4+4096],ebx
; +1(Block 3)
		shr	eax,4
		mov	ebx,ecx
		and	ebx,eax
		mov	ebx,[ebp+ebx*4]
		cmp	ebx,[edi+4+2048]
		setnz	dl
		mov	[edi+4+2048],ebx
		or	dh,dl
		mov	[edi+4+2048+4096],ebx
; +2(Block 2)
		mov	eax,[esi+4]
		shr	eax,8
		mov	ebx,ecx
		and	ebx,eax
		mov	ebx,[ebp+ebx*4]
		cmp	ebx,[edi+8]
		setnz	dl
		mov	[edi+8],ebx
		or	dh,dl
		mov	[edi+8+4096],ebx
; +2(Block 3)
		shr	eax,4
		mov	ebx,ecx
		and	ebx,eax
		mov	ebx,[ebp+ebx*4]
		cmp	ebx,[edi+8+2048]
		setnz	dl
		mov	[edi+8+2048],ebx
		or	dh,dl
		mov	[edi+8+2048+4096],ebx
; +3(Block 2)
		shr	eax,12
		mov	ebx,ecx
		and	ebx,eax
		mov	ebx,[ebp+ebx*4]
		cmp	ebx,[edi+12]
		setnz	dl
		mov	[edi+12],ebx
		or	dh,dl
		mov	[edi+12+4096],ebx
; +3(Block 3)
		shr	eax,4
		mov	ebx,ecx
		and	ebx,eax
		mov	ebx,[ebp+ebx*4]
		cmp	ebx,[edi+12+2048]
		setnz	dl
		mov	[edi+12+2048],ebx
		or	dh,dl
		mov	[edi+12+2048+4096],ebx
; 次へ
		add	esi,8
		add	edi,16
		pop	ecx
		dec	ecx
		jnz	near .loop
; 終了
		mov	eax,edx
		shr	eax,8
		pop	edi
		pop	esi
		pop	edx
		pop	ecx
		pop	ebx
		pop	ebp
		ret

;
; レンダラ
; グラフィック(1024色モード、ページ0)
;
; void Rend1024C(const BYTE *gvram, DWORD *buf, BOOL *flag, const DWORD *pal)
;
_Rend1024C:
		push	ebp
		mov	ebp,esp
		push	eax
		push	ebx
		push	ecx
		push	edx
		push	esi
		push	edi
; 初期設定
		mov	esi,[ebp+8]
		mov	edi,[ebp+12]
		mov	ebx,[ebp+16]
		mov	ebp,[ebp+20]
		mov	ecx,32
		xor	edx,edx
; ループ
.loop1:
		cmp	[ebx],edx
		jnz	.exec
		add	esi,32
		add	edi,64
		add	ebx,4
		dec	ecx
		jnz	.loop1
		jmp	.exit
; この16dotは処理
.exec:
		mov	[ebx],edx
		push	ecx
		mov	ecx,2
.loop2:
		push	ecx
		mov	ecx,15
; +0
		mov	eax,[esi]
		mov	edx,ecx
		and	edx,eax
		mov	edx,[ebp+edx*4]
		mov	[edi],edx
		mov	[edi+4096],edx
; +1
		shr	eax,16
		mov	edx,ecx
		and	edx,eax
		mov	edx,[ebp+edx*4]
		mov	[edi+4],edx
		mov	[edi+4+4096],edx
; +2
		mov	eax,[esi+4]
		mov	edx,ecx
		and	edx,eax
		mov	edx,[ebp+edx*4]
		mov	[edi+8],edx
		mov	[edi+8+4096],edx
; +3
		shr	eax,16
		mov	edx,ecx
		and	edx,eax
		mov	edx,[ebp+edx*4]
		mov	[edi+12],edx
		mov	[edi+12+4096],edx
; +4
		mov	eax,[esi+8]
		mov	edx,ecx
		and	edx,eax
		mov	edx,[ebp+edx*4]
		mov	[edi+16],edx
		mov	[edi+16+4096],edx
; +5
		shr	eax,16
		mov	edx,ecx
		and	edx,eax
		mov	edx,[ebp+edx*4]
		mov	[edi+20],edx
		mov	[edi+20+4096],edx
; +6
		mov	eax,[esi+12]
		mov	edx,ecx
		and	edx,eax
		mov	edx,[ebp+edx*4]
		mov	[edi+24],edx
		mov	[edi+24+4096],edx
; +7
		shr	eax,16
		mov	edx,ecx
		and	edx,eax
		mov	edx,[ebp+edx*4]
		mov	[edi+28],edx
		mov	[edi+28+4096],edx
; もう１回
		add	esi,16
		pop	ecx
		add	edi,32
		dec	ecx
		jnz	near .loop2
; 次の16dotへ
		add	ebx,4
		pop	ecx
		xor	edx,edx
		dec	ecx
		jnz	near .loop1
; 終了
.exit:
		pop	edi
		pop	esi
		pop	edx
		pop	ecx
		pop	ebx
		pop	eax
		pop	ebp
		ret

;
; レンダラ
; グラフィック(1024色モード、ページ1)
;
; void Rend1024D(const BYTE *gvram, DWORD *buf, BOOL *flag, const DWORD *pal)
;
_Rend1024D:
		push	ebp
		mov	ebp,esp
		push	eax
		push	ebx
		push	ecx
		push	edx
		push	esi
		push	edi
; 初期設定
		mov	esi,[ebp+8]
		mov	edi,[ebp+12]
		mov	ebx,[ebp+16]
		mov	ebp,[ebp+20]
		mov	ecx,32
		xor	edx,edx
		add	edi,2048
; ループ
.loop1:
		cmp	[ebx],edx
		jnz	.exec
		add	esi,32
		add	edi,64
		add	ebx,4
		dec	ecx
		jnz	.loop1
		jmp	.exit
; この16dotは処理
.exec:
		mov	[ebx],edx
		push	ecx
		mov	ecx,2
.loop2:
		push	ecx
		mov	ecx,15
; +0
		mov	eax,[esi]
		shr	eax,4
		mov	edx,ecx
		and	edx,eax
		mov	edx,[ebp+edx*4]
		mov	[edi],edx
		mov	[edi+4096],edx
; +1
		shr	eax,16
		mov	edx,ecx
		and	edx,eax
		mov	edx,[ebp+edx*4]
		mov	[edi+4],edx
		mov	[edi+4+4096],edx
; +2
		mov	eax,[esi+4]
		shr	eax,4
		mov	edx,ecx
		and	edx,eax
		mov	edx,[ebp+edx*4]
		mov	[edi+8],edx
		mov	[edi+8+4096],edx
; +3
		shr	eax,16
		mov	edx,ecx
		and	edx,eax
		mov	edx,[ebp+edx*4]
		mov	[edi+12],edx
		mov	[edi+12+4096],edx
; +4
		mov	eax,[esi+8]
		shr	eax,4
		mov	edx,ecx
		and	edx,eax
		mov	edx,[ebp+edx*4]
		mov	[edi+16],edx
		mov	[edi+16+4096],edx
; +5
		shr	eax,16
		mov	edx,ecx
		and	edx,eax
		mov	edx,[ebp+edx*4]
		mov	[edi+20],edx
		mov	[edi+20+4096],edx
; +6
		mov	eax,[esi+12]
		shr	eax,4
		mov	edx,ecx
		and	edx,eax
		mov	edx,[ebp+edx*4]
		mov	[edi+24],edx
		mov	[edi+24+4096],edx
; +7
		shr	eax,16
		mov	edx,ecx
		and	edx,eax
		mov	edx,[ebp+edx*4]
		mov	[edi+28],edx
		mov	[edi+28+4096],edx
; もう１回
		add	esi,16
		pop	ecx
		add	edi,32
		dec	ecx
		jnz	near .loop2
; 次の16dotへ
		add	ebx,4
		pop	ecx
		xor	edx,edx
		dec	ecx
		jnz	near .loop1
; 終了
.exit:
		pop	edi
		pop	esi
		pop	edx
		pop	ecx
		pop	ebx
		pop	eax
		pop	ebp
		ret

;
; レンダラ
; グラフィック(1024色モード、ページ2)
;
; void Rend1024E(const BYTE *gvram, DWORD *buf, BOOL *flag, const DWORD *pal)
;
_Rend1024E:
		push	ebp
		mov	ebp,esp
		push	eax
		push	ebx
		push	ecx
		push	edx
		push	esi
		push	edi
; 初期設定
		mov	esi,[ebp+8]
		mov	edi,[ebp+12]
		mov	ebx,[ebp+16]
		mov	ebp,[ebp+20]
		mov	ecx,32
		xor	edx,edx
; ループ
.loop1:
		cmp	[ebx],edx
		jnz	.exec
		add	esi,32
		add	edi,64
		add	ebx,4
		dec	ecx
		jnz	.loop1
		jmp	.exit
; この16dotは処理
.exec:
		mov	[ebx],edx
		push	ecx
		mov	ecx,2
.loop2:
		push	ecx
		mov	ecx,15
; +0
		mov	eax,[esi]
		shr	eax,8
		mov	edx,ecx
		and	edx,eax
		mov	edx,[ebp+edx*4]
		mov	[edi],edx
		mov	[edi+4096],edx
; +1
		shr	eax,16
		mov	edx,ecx
		and	edx,eax
		mov	edx,[ebp+edx*4]
		mov	[edi+4],edx
		mov	[edi+4+4096],edx
; +2
		mov	eax,[esi+4]
		shr	eax,8
		mov	edx,ecx
		and	edx,eax
		mov	edx,[ebp+edx*4]
		mov	[edi+8],edx
		mov	[edi+8+4096],edx
; +3
		shr	eax,16
		mov	edx,ecx
		and	edx,eax
		mov	edx,[ebp+edx*4]
		mov	[edi+12],edx
		mov	[edi+12+4096],edx
; +4
		mov	eax,[esi+8]
		shr	eax,8
		mov	edx,ecx
		and	edx,eax
		mov	edx,[ebp+edx*4]
		mov	[edi+16],edx
		mov	[edi+16+4096],edx
; +5
		shr	eax,16
		mov	edx,ecx
		and	edx,eax
		mov	edx,[ebp+edx*4]
		mov	[edi+20],edx
		mov	[edi+20+4096],edx
; +6
		mov	eax,[esi+12]
		shr	eax,8
		mov	edx,ecx
		and	edx,eax
		mov	edx,[ebp+edx*4]
		mov	[edi+24],edx
		mov	[edi+24+4096],edx
; +7
		shr	eax,16
		mov	edx,ecx
		and	edx,eax
		mov	edx,[ebp+edx*4]
		mov	[edi+28],edx
		mov	[edi+28+4096],edx
; もう１回
		add	esi,16
		pop	ecx
		add	edi,32
		dec	ecx
		jnz	near .loop2
; 次の16dotへ
		add	ebx,4
		pop	ecx
		xor	edx,edx
		dec	ecx
		jnz	near .loop1
; 終了
.exit:
		pop	edi
		pop	esi
		pop	edx
		pop	ecx
		pop	ebx
		pop	eax
		pop	ebp
		ret

;
; レンダラ
; グラフィック(1024色モード、ページ3)
;
; void Rend1024F(const BYTE *gvram, DWORD *buf, BOOL *flag, const DWORD *pal)
;
_Rend1024F:
		push	ebp
		mov	ebp,esp
		push	eax
		push	ebx
		push	ecx
		push	edx
		push	esi
		push	edi
; 初期設定
		mov	esi,[ebp+8]
		mov	edi,[ebp+12]
		mov	ebx,[ebp+16]
		mov	ebp,[ebp+20]
		mov	ecx,32
		xor	edx,edx
		add	edi,2048
; ループ
.loop1:
		cmp	[ebx],edx
		jnz	.exec
		add	esi,32
		add	edi,64
		add	ebx,4
		dec	ecx
		jnz	.loop1
		jmp	.exit
; この16dotは処理
.exec:
		mov	[ebx],edx
		push	ecx
		mov	ecx,2
.loop2:
		push	ecx
		mov	ecx,15
; +0
		mov	eax,[esi]
		shr	eax,12
		mov	edx,ecx
		and	edx,eax
		mov	edx,[ebp+edx*4]
		mov	[edi],edx
		mov	[edi+4096],edx
; +1
		shr	eax,16
		mov	edx,ecx
		and	edx,eax
		mov	edx,[ebp+edx*4]
		mov	[edi+4],edx
		mov	[edi+4+4096],edx
; +2
		mov	eax,[esi+4]
		shr	eax,12
		mov	edx,ecx
		and	edx,eax
		mov	edx,[ebp+edx*4]
		mov	[edi+8],edx
		mov	[edi+8+4096],edx
; +3
		shr	eax,16
		mov	edx,ecx
		and	edx,eax
		mov	edx,[ebp+edx*4]
		mov	[edi+12],edx
		mov	[edi+12+4096],edx
; +4
		mov	eax,[esi+8]
		shr	eax,12
		mov	edx,ecx
		and	edx,eax
		mov	edx,[ebp+edx*4]
		mov	[edi+16],edx
		mov	[edi+16+4096],edx
; +5
		shr	eax,16
		mov	edx,ecx
		and	edx,eax
		mov	edx,[ebp+edx*4]
		mov	[edi+20],edx
		mov	[edi+20+4096],edx
; +6
		mov	eax,[esi+12]
		shr	eax,12
		mov	edx,ecx
		and	edx,eax
		mov	edx,[ebp+edx*4]
		mov	[edi+24],edx
		mov	[edi+24+4096],edx
; +7
		shr	eax,16
		mov	edx,ecx
		and	edx,eax
		mov	edx,[ebp+edx*4]
		mov	[edi+28],edx
		mov	[edi+28+4096],edx
; もう１回
		add	esi,16
		pop	ecx
		add	edi,32
		dec	ecx
		jnz	near .loop2
; 次の16dotへ
		add	ebx,4
		pop	ecx
		xor	edx,edx
		dec	ecx
		jnz	near .loop1
; 終了
.exit:
		pop	edi
		pop	esi
		pop	edx
		pop	ecx
		pop	ebx
		pop	eax
		pop	ebp
		ret

;
; レンダラ
; グラフィック(16色モード、ページ0、All)
;
; int Rend16A(const BYTE *gvram, DWORD *buf, const DWORD *pal)
;
_Rend16A:
		push	ebp
		mov	ebp,esp
		push	ebx
		push	ecx
		push	edx
		push	esi
		push	edi
; 初期設定
		mov	esi,[ebp+8]
		mov	edi,[ebp+12]
		mov	ebp,[ebp+16]
		mov	ecx,64
		xor	edx,edx
; 8dotループ
.loop:
		push	ecx
		mov	ecx,15
; +0, +1
		mov	eax,[esi]
		mov	ebx,ecx
		and	ebx,eax
		mov	ebx,[ebp+ebx*4]
		shr	eax,16
		cmp	ebx,[edi]
		setnz	dl
		mov	[edi],ebx
		mov	[edi+2048],ebx
		and	eax,ecx
		or	dh,dl
		mov	ebx,[ebp+eax*4]
		cmp	ebx,[edi+4]
		setnz	dl
		mov	[edi+4],ebx
		mov	[edi+2048+4],ebx
		or	dh,dl
; +2, +3
		mov	eax,[esi+4]
		mov	ebx,ecx
		and	ebx,eax
		mov	ebx,[ebp+ebx*4]
		shr	eax,16
		cmp	ebx,[edi+8]
		setnz	dl
		mov	[edi+8],ebx
		mov	[edi+2048+8],ebx
		and	eax,ecx
		or	dh,dl
		mov	ebx,[ebp+eax*4]
		cmp	ebx,[edi+12]
		setnz	dl
		mov	[edi+12],ebx
		mov	[edi+2048+12],ebx
		or	dh,dl
; +4, +5
		mov	eax,[esi+8]
		mov	ebx,ecx
		and	ebx,eax
		mov	ebx,[ebp+ebx*4]
		shr	eax,16
		cmp	ebx,[edi+16]
		setnz	dl
		mov	[edi+16],ebx
		mov	[edi+2048+16],ebx
		and	eax,ecx
		or	dh,dl
		mov	ebx,[ebp+eax*4]
		cmp	ebx,[edi+20]
		setnz	dl
		mov	[edi+20],ebx
		mov	[edi+2048+20],ebx
		or	dh,dl
; +6, +7
		mov	eax,[esi+12]
		mov	ebx,ecx
		and	ebx,eax
		mov	ebx,[ebp+ebx*4]
		shr	eax,16
		cmp	ebx,[edi+24]
		setnz	dl
		mov	[edi+24],ebx
		mov	[edi+2048+24],ebx
		and	eax,ecx
		or	dh,dl
		mov	ebx,[ebp+eax*4]
		cmp	ebx,[edi+28]
		setnz	dl
		mov	[edi+28],ebx
		mov	[edi+2048+28],ebx
		or	dh,dl
; 次へ
		pop	ecx
		add	esi,16
		add	edi,32
		dec	ecx
		jnz	near .loop
; 終了
		mov	eax,edx
		shr	eax,8
		pop	edi
		pop	esi
		pop	edx
		pop	ecx
		pop	ebx
		pop	ebp
		ret

;
; レンダラ
; グラフィック(16色モード、ページ0)
;
; void Rend16A(const BYTE *gvram, DWORD *buf, BOOL *flag, const DWORD *pal)
;
_Rend16B:
		push	ebp
		mov	ebp,esp
		push	eax
		push	ebx
		push	ecx
		push	edx
		push	esi
		push	edi
; 初期設定
		mov	esi,[ebp+8]
		mov	edi,[ebp+12]
		mov	ebx,[ebp+16]
		mov	ebp,[ebp+20]
		mov	ecx,32
		xor	edx,edx
; 比較ループ
.loop:
		cmp	edx,[ebx]
		jnz	.exec
; この16dotは処理しない
		add	esi,32
		add	edi,64
		add	ebx,4
		dec	ecx
		jnz	.loop
		jmp	.exit
; この16dotは処理
.exec:
		mov	[ebx],edx
		push	ecx
		mov	ecx,15
; +0, +1
		mov	eax,[esi]
		mov	edx,ecx
		and	edx,eax
		mov	edx,[ebp+edx*4]
		shr	eax,16
		mov	[edi],edx
		mov	[edi+2048],edx
		and	eax,ecx
		mov	edx,[ebp+eax*4]
		mov	[edi+4],edx
		mov	[edi+2048+4],edx
; +2, +3
		mov	eax,[esi+4]
		mov	edx,ecx
		and	edx,eax
		mov	edx,[ebp+edx*4]
		shr	eax,16
		mov	[edi+8],edx
		mov	[edi+2048+8],edx
		and	eax,ecx
		mov	edx,[ebp+eax*4]
		mov	[edi+12],edx
		mov	[edi+2048+12],edx
; +4, +5
		mov	eax,[esi+8]
		mov	edx,ecx
		and	edx,eax
		mov	edx,[ebp+edx*4]
		shr	eax,16
		mov	[edi+16],edx
		mov	[edi+2048+16],edx
		and	eax,ecx
		mov	edx,[ebp+eax*4]
		mov	[edi+20],edx
		mov	[edi+2048+20],edx
; +6, +7
		mov	eax,[esi+12]
		mov	edx,ecx
		and	edx,eax
		mov	edx,[ebp+edx*4]
		shr	eax,16
		mov	[edi+24],edx
		mov	[edi+2048+24],edx
		and	eax,ecx
		mov	edx,[ebp+eax*4]
		mov	[edi+28],edx
		mov	[edi+2048+28],edx
; +8, +9
		mov	eax,[esi+16]
		mov	edx,ecx
		and	edx,eax
		mov	edx,[ebp+edx*4]
		shr	eax,16
		mov	[edi+32],edx
		mov	[edi+2048+32],edx
		and	eax,ecx
		mov	edx,[ebp+eax*4]
		mov	[edi+36],edx
		mov	[edi+2048+36],edx
; +A, +B
		mov	eax,[esi+20]
		mov	edx,ecx
		and	edx,eax
		mov	edx,[ebp+edx*4]
		shr	eax,16
		mov	[edi+40],edx
		mov	[edi+2048+40],edx
		and	eax,ecx
		mov	edx,[ebp+eax*4]
		mov	[edi+44],edx
		mov	[edi+2048+44],edx
; +C, +D
		mov	eax,[esi+24]
		mov	edx,ecx
		and	edx,eax
		mov	edx,[ebp+edx*4]
		shr	eax,16
		mov	[edi+48],edx
		mov	[edi+2048+48],edx
		and	eax,ecx
		mov	edx,[ebp+eax*4]
		mov	[edi+52],edx
		mov	[edi+2048+52],edx
; +E, +F
		mov	eax,[esi+28]
		mov	edx,ecx
		and	edx,eax
		mov	edx,[ebp+edx*4]
		shr	eax,16
		mov	[edi+56],edx
		mov	[edi+2048+56],edx
		and	eax,ecx
		mov	edx,[ebp+eax*4]
		mov	[edi+60],edx
		mov	[edi+2048+60],edx
; 次へ
		pop	ecx
		add	esi,32
		add	edi,64
		add	ebx,4
		xor	edx,edx
		dec	ecx
		jnz	near .loop
; 終了
.exit:
		pop	edi
		pop	esi
		pop	edx
		pop	ecx
		pop	ebx
		pop	eax
		pop	ebp
		ret

;
; レンダラ
; グラフィック(16色モード、ページ1、All)
;
; int Rend16C(const BYTE *gvram, DWORD *buf, const DWORD *pal)
;
_Rend16C:
		push	ebp
		mov	ebp,esp
		push	ebx
		push	ecx
		push	edx
		push	esi
		push	edi
; 初期設定
		mov	esi,[ebp+8]
		mov	edi,[ebp+12]
		mov	ebp,[ebp+16]
		mov	ecx,64
		xor	edx,edx
; 8dotループ
.loop:
		push	ecx
		mov	ecx,15
; +0, +1
		mov	eax,[esi]
		mov	ebx,ecx
		shr	eax,4
		and	ebx,eax
		mov	ebx,[ebp+ebx*4]
		shr	eax,16
		cmp	ebx,[edi]
		setnz	dl
		mov	[edi],ebx
		mov	[edi+2048],ebx
		and	eax,ecx
		or	dh,dl
		mov	ebx,[ebp+eax*4]
		cmp	ebx,[edi+4]
		setnz	dl
		mov	[edi+4],ebx
		mov	[edi+2048+4],ebx
		or	dh,dl
; +2, +3
		mov	eax,[esi+4]
		mov	ebx,ecx
		shr	eax,4
		and	ebx,eax
		mov	ebx,[ebp+ebx*4]
		shr	eax,16
		cmp	ebx,[edi+8]
		setnz	dl
		mov	[edi+8],ebx
		mov	[edi+2048+8],ebx
		and	eax,ecx
		or	dh,dl
		mov	ebx,[ebp+eax*4]
		cmp	ebx,[edi+12]
		setnz	dl
		mov	[edi+12],ebx
		mov	[edi+2048+12],ebx
		or	dh,dl
; +4, +5
		mov	eax,[esi+8]
		mov	ebx,ecx
		shr	eax,4
		and	ebx,eax
		mov	ebx,[ebp+ebx*4]
		shr	eax,16
		cmp	ebx,[edi+16]
		setnz	dl
		mov	[edi+16],ebx
		mov	[edi+2048+16],ebx
		and	eax,ecx
		or	dh,dl
		mov	ebx,[ebp+eax*4]
		cmp	ebx,[edi+20]
		setnz	dl
		mov	[edi+20],ebx
		mov	[edi+2048+20],ebx
		or	dh,dl
; +6, +7
		mov	eax,[esi+12]
		mov	ebx,ecx
		shr	eax,4
		and	ebx,eax
		mov	ebx,[ebp+ebx*4]
		shr	eax,16
		cmp	ebx,[edi+24]
		setnz	dl
		mov	[edi+24],ebx
		mov	[edi+2048+24],ebx
		and	eax,ecx
		or	dh,dl
		mov	ebx,[ebp+eax*4]
		cmp	ebx,[edi+28]
		setnz	dl
		mov	[edi+28],ebx
		mov	[edi+2048+28],ebx
		or	dh,dl
; 次へ
		pop	ecx
		add	esi,16
		add	edi,32
		dec	ecx
		jnz	near .loop
; 終了
		mov	eax,edx
		shr	eax,8
		pop	edi
		pop	esi
		pop	edx
		pop	ecx
		pop	ebx
		pop	ebp
		ret

;
; レンダラ
; グラフィック(16色モード、ページ1)
;
; void Rend16D(const BYTE *gvram, DWORD *buf, BOOL *flag, const DWORD *pal)
;
_Rend16D:
		push	ebp
		mov	ebp,esp
		push	eax
		push	ebx
		push	ecx
		push	edx
		push	esi
		push	edi
; 初期設定
		mov	esi,[ebp+8]
		mov	edi,[ebp+12]
		mov	ebx,[ebp+16]
		mov	ebp,[ebp+20]
		mov	ecx,32
		xor	edx,edx
; 比較ループ
.loop:
		cmp	edx,[ebx]
		jnz	.exec
; この16dotは処理しない
		add	esi,32
		add	edi,64
		add	ebx,4
		dec	ecx
		jnz	.loop
		jmp	.exit
; この16dotは処理
.exec:
		mov	[ebx],edx
		push	ecx
		mov	ecx,15
; +0, +1
		mov	eax,[esi]
		mov	edx,ecx
		shr	eax,4
		and	edx,eax
		mov	edx,[ebp+edx*4]
		shr	eax,16
		mov	[edi],edx
		mov	[edi+2048],edx
		and	eax,ecx
		mov	edx,[ebp+eax*4]
		mov	[edi+4],edx
		mov	[edi+2048+4],edx
; +2, +3
		mov	eax,[esi+4]
		mov	edx,ecx
		shr	eax,4
		and	edx,eax
		mov	edx,[ebp+edx*4]
		shr	eax,16
		mov	[edi+8],edx
		mov	[edi+2048+8],edx
		and	eax,ecx
		mov	edx,[ebp+eax*4]
		mov	[edi+12],edx
		mov	[edi+2048+12],edx
; +4, +5
		mov	eax,[esi+8]
		mov	edx,ecx
		shr	eax,4
		and	edx,eax
		mov	edx,[ebp+edx*4]
		shr	eax,16
		mov	[edi+16],edx
		mov	[edi+2048+16],edx
		and	eax,ecx
		mov	edx,[ebp+eax*4]
		mov	[edi+20],edx
		mov	[edi+2048+20],edx
; +6, +7
		mov	eax,[esi+12]
		mov	edx,ecx
		shr	eax,4
		and	edx,eax
		mov	edx,[ebp+edx*4]
		shr	eax,16
		mov	[edi+24],edx
		mov	[edi+2048+24],edx
		and	eax,ecx
		mov	edx,[ebp+eax*4]
		mov	[edi+28],edx
		mov	[edi+2048+28],edx
; +8, +9
		mov	eax,[esi+16]
		mov	edx,ecx
		shr	eax,4
		and	edx,eax
		mov	edx,[ebp+edx*4]
		shr	eax,16
		mov	[edi+32],edx
		mov	[edi+2048+32],edx
		and	eax,ecx
		mov	edx,[ebp+eax*4]
		mov	[edi+36],edx
		mov	[edi+2048+36],edx
; +A, +B
		mov	eax,[esi+20]
		mov	edx,ecx
		shr	eax,4
		and	edx,eax
		mov	edx,[ebp+edx*4]
		shr	eax,16
		mov	[edi+40],edx
		mov	[edi+2048+40],edx
		and	eax,ecx
		mov	edx,[ebp+eax*4]
		mov	[edi+44],edx
		mov	[edi+2048+44],edx
; +C, +D
		mov	eax,[esi+24]
		mov	edx,ecx
		shr	eax,4
		and	edx,eax
		mov	edx,[ebp+edx*4]
		shr	eax,16
		mov	[edi+48],edx
		mov	[edi+2048+48],edx
		and	eax,ecx
		mov	edx,[ebp+eax*4]
		mov	[edi+52],edx
		mov	[edi+2048+52],edx
; +E, +F
		mov	eax,[esi+28]
		mov	edx,ecx
		shr	eax,4
		and	edx,eax
		mov	edx,[ebp+edx*4]
		shr	eax,16
		mov	[edi+56],edx
		mov	[edi+2048+56],edx
		and	eax,ecx
		mov	edx,[ebp+eax*4]
		mov	[edi+60],edx
		mov	[edi+2048+60],edx
; 次へ
		pop	ecx
		add	esi,32
		add	edi,64
		add	ebx,4
		xor	edx,edx
		dec	ecx
		jnz	near .loop
; 終了
.exit:
		pop	edi
		pop	esi
		pop	edx
		pop	ecx
		pop	ebx
		pop	eax
		pop	ebp
		ret

;
; レンダラ
; グラフィック(16色モード、ページ2、All)
;
; int Rend16E(const BYTE *gvram, DWORD *buf, const DWORD *pal)
;
_Rend16E:
		push	ebp
		mov	ebp,esp
		push	ebx
		push	ecx
		push	edx
		push	esi
		push	edi
; 初期設定
		mov	esi,[ebp+8]
		mov	edi,[ebp+12]
		mov	ebp,[ebp+16]
		mov	ecx,64
		xor	edx,edx
; 8dotループ
.loop:
		push	ecx
		mov	ecx,15
; +0, +1
		mov	eax,[esi]
		mov	ebx,ecx
		shr	eax,8
		and	ebx,eax
		mov	ebx,[ebp+ebx*4]
		shr	eax,16
		cmp	ebx,[edi]
		setnz	dl
		mov	[edi],ebx
		mov	[edi+2048],ebx
		and	eax,ecx
		or	dh,dl
		mov	ebx,[ebp+eax*4]
		cmp	ebx,[edi+4]
		setnz	dl
		mov	[edi+4],ebx
		mov	[edi+2048+4],ebx
		or	dh,dl
; +2, +3
		mov	eax,[esi+4]
		mov	ebx,ecx
		shr	eax,8
		and	ebx,eax
		mov	ebx,[ebp+ebx*4]
		shr	eax,16
		cmp	ebx,[edi+8]
		setnz	dl
		mov	[edi+8],ebx
		mov	[edi+2048+8],ebx
		and	eax,ecx
		or	dh,dl
		mov	ebx,[ebp+eax*4]
		cmp	ebx,[edi+12]
		setnz	dl
		mov	[edi+12],ebx
		mov	[edi+2048+12],ebx
		or	dh,dl
; +4, +5
		mov	eax,[esi+8]
		mov	ebx,ecx
		shr	eax,8
		and	ebx,eax
		mov	ebx,[ebp+ebx*4]
		shr	eax,16
		cmp	ebx,[edi+16]
		setnz	dl
		mov	[edi+16],ebx
		mov	[edi+2048+16],ebx
		and	eax,ecx
		or	dh,dl
		mov	ebx,[ebp+eax*4]
		cmp	ebx,[edi+20]
		setnz	dl
		mov	[edi+20],ebx
		mov	[edi+2048+20],ebx
		or	dh,dl
; +6, +7
		mov	eax,[esi+12]
		mov	ebx,ecx
		shr	eax,8
		and	ebx,eax
		mov	ebx,[ebp+ebx*4]
		shr	eax,16
		cmp	ebx,[edi+24]
		setnz	dl
		mov	[edi+24],ebx
		mov	[edi+2048+24],ebx
		and	eax,ecx
		or	dh,dl
		mov	ebx,[ebp+eax*4]
		cmp	ebx,[edi+28]
		setnz	dl
		mov	[edi+28],ebx
		mov	[edi+2048+28],ebx
		or	dh,dl
; 次へ
		pop	ecx
		add	esi,16
		add	edi,32
		dec	ecx
		jnz	near .loop
; 終了
		mov	eax,edx
		shr	eax,8
		pop	edi
		pop	esi
		pop	edx
		pop	ecx
		pop	ebx
		pop	ebp
		ret

;
; レンダラ
; グラフィック(16色モード、ページ2)
;
; void Rend16F(const BYTE *gvram, DWORD *buf, BOOL *flag, const DWORD *pal)
;
_Rend16F:
		push	ebp
		mov	ebp,esp
		push	eax
		push	ebx
		push	ecx
		push	edx
		push	esi
		push	edi
; 初期設定
		mov	esi,[ebp+8]
		mov	edi,[ebp+12]
		mov	ebx,[ebp+16]
		mov	ebp,[ebp+20]
		mov	ecx,32
		xor	edx,edx
; 比較ループ
.loop:
		cmp	edx,[ebx]
		jnz	.exec
; この16dotは処理しない
		add	esi,32
		add	edi,64
		add	ebx,4
		dec	ecx
		jnz	.loop
		jmp	.exit
; この16dotは処理
.exec:
		mov	[ebx],edx
		push	ecx
		mov	ecx,15
; +0, +1
		mov	eax,[esi]
		mov	edx,ecx
		shr	eax,8
		and	edx,eax
		mov	edx,[ebp+edx*4]
		shr	eax,16
		mov	[edi],edx
		mov	[edi+2048],edx
		and	eax,ecx
		mov	edx,[ebp+eax*4]
		mov	[edi+4],edx
		mov	[edi+2048+4],edx
; +2, +3
		mov	eax,[esi+4]
		mov	edx,ecx
		shr	eax,8
		and	edx,eax
		mov	edx,[ebp+edx*4]
		shr	eax,16
		mov	[edi+8],edx
		mov	[edi+2048+8],edx
		and	eax,ecx
		mov	edx,[ebp+eax*4]
		mov	[edi+12],edx
		mov	[edi+2048+12],edx
; +4, +5
		mov	eax,[esi+8]
		mov	edx,ecx
		shr	eax,8
		and	edx,eax
		mov	edx,[ebp+edx*4]
		shr	eax,16
		mov	[edi+16],edx
		mov	[edi+2048+16],edx
		and	eax,ecx
		mov	edx,[ebp+eax*4]
		mov	[edi+20],edx
		mov	[edi+2048+20],edx
; +6, +7
		mov	eax,[esi+12]
		mov	edx,ecx
		shr	eax,8
		and	edx,eax
		mov	edx,[ebp+edx*4]
		shr	eax,16
		mov	[edi+24],edx
		mov	[edi+2048+24],edx
		and	eax,ecx
		mov	edx,[ebp+eax*4]
		mov	[edi+28],edx
		mov	[edi+2048+28],edx
; +8, +9
		mov	eax,[esi+16]
		mov	edx,ecx
		shr	eax,8
		and	edx,eax
		mov	edx,[ebp+edx*4]
		shr	eax,16
		mov	[edi+32],edx
		mov	[edi+2048+32],edx
		and	eax,ecx
		mov	edx,[ebp+eax*4]
		mov	[edi+36],edx
		mov	[edi+2048+36],edx
; +A, +B
		mov	eax,[esi+20]
		mov	edx,ecx
		shr	eax,8
		and	edx,eax
		mov	edx,[ebp+edx*4]
		shr	eax,16
		mov	[edi+40],edx
		mov	[edi+2048+40],edx
		and	eax,ecx
		mov	edx,[ebp+eax*4]
		mov	[edi+44],edx
		mov	[edi+2048+44],edx
; +C, +D
		mov	eax,[esi+24]
		mov	edx,ecx
		shr	eax,8
		and	edx,eax
		mov	edx,[ebp+edx*4]
		shr	eax,16
		mov	[edi+48],edx
		mov	[edi+2048+48],edx
		and	eax,ecx
		mov	edx,[ebp+eax*4]
		mov	[edi+52],edx
		mov	[edi+2048+52],edx
; +E, +F
		mov	eax,[esi+28]
		mov	edx,ecx
		shr	eax,8
		and	edx,eax
		mov	edx,[ebp+edx*4]
		shr	eax,16
		mov	[edi+56],edx
		mov	[edi+2048+56],edx
		and	eax,ecx
		mov	edx,[ebp+eax*4]
		mov	[edi+60],edx
		mov	[edi+2048+60],edx
; 次へ
		pop	ecx
		add	esi,32
		add	edi,64
		add	ebx,4
		xor	edx,edx
		dec	ecx
		jnz	near .loop
; 終了
.exit:
		pop	edi
		pop	esi
		pop	edx
		pop	ecx
		pop	ebx
		pop	eax
		pop	ebp
		ret

;
; レンダラ
; グラフィック(16色モード、ページ3、All)
;
; int Rend16G(const BYTE *gvram, DWORD *buf, const DWORD *pal)
;
_Rend16G:
		push	ebp
		mov	ebp,esp
		push	ebx
		push	ecx
		push	edx
		push	esi
		push	edi
; 初期設定
		mov	esi,[ebp+8]
		mov	edi,[ebp+12]
		mov	ebp,[ebp+16]
		mov	ecx,64
		xor	edx,edx
; 8dotループ
.loop:
		push	ecx
		mov	ecx,15
; +0, +1
		mov	eax,[esi]
		mov	ebx,ecx
		shr	eax,12
		and	ebx,eax
		mov	ebx,[ebp+ebx*4]
		shr	eax,16
		cmp	ebx,[edi]
		setnz	dl
		mov	[edi],ebx
		mov	[edi+2048],ebx
		and	eax,ecx
		or	dh,dl
		mov	ebx,[ebp+eax*4]
		cmp	ebx,[edi+4]
		setnz	dl
		mov	[edi+4],ebx
		mov	[edi+2048+4],ebx
		or	dh,dl
; +2, +3
		mov	eax,[esi+4]
		mov	ebx,ecx
		shr	eax,12
		and	ebx,eax
		mov	ebx,[ebp+ebx*4]
		shr	eax,16
		cmp	ebx,[edi+8]
		setnz	dl
		mov	[edi+8],ebx
		mov	[edi+2048+8],ebx
		and	eax,ecx
		or	dh,dl
		mov	ebx,[ebp+eax*4]
		cmp	ebx,[edi+12]
		setnz	dl
		mov	[edi+12],ebx
		mov	[edi+2048+12],ebx
		or	dh,dl
; +4, +5
		mov	eax,[esi+8]
		mov	ebx,ecx
		shr	eax,12
		and	ebx,eax
		mov	ebx,[ebp+ebx*4]
		shr	eax,16
		cmp	ebx,[edi+16]
		setnz	dl
		mov	[edi+16],ebx
		mov	[edi+2048+16],ebx
		and	eax,ecx
		or	dh,dl
		mov	ebx,[ebp+eax*4]
		cmp	ebx,[edi+20]
		setnz	dl
		mov	[edi+20],ebx
		mov	[edi+2048+20],ebx
		or	dh,dl
; +6, +7
		mov	eax,[esi+12]
		mov	ebx,ecx
		shr	eax,12
		and	ebx,eax
		mov	ebx,[ebp+ebx*4]
		shr	eax,16
		cmp	ebx,[edi+24]
		setnz	dl
		mov	[edi+24],ebx
		mov	[edi+2048+24],ebx
		and	eax,ecx
		or	dh,dl
		mov	ebx,[ebp+eax*4]
		cmp	ebx,[edi+28]
		setnz	dl
		mov	[edi+28],ebx
		mov	[edi+2048+28],ebx
		or	dh,dl
; 次へ
		pop	ecx
		add	esi,16
		add	edi,32
		dec	ecx
		jnz	near .loop
; 終了
		mov	eax,edx
		shr	eax,8
		pop	edi
		pop	esi
		pop	edx
		pop	ecx
		pop	ebx
		pop	ebp
		ret

;
; レンダラ
; グラフィック(16色モード、ページ3)
;
; void Rend16H(const BYTE *gvram, DWORD *buf, BOOL *flag, const DWORD *pal)
;
_Rend16H:
		push	ebp
		mov	ebp,esp
		push	eax
		push	ebx
		push	ecx
		push	edx
		push	esi
		push	edi
; 初期設定
		mov	esi,[ebp+8]
		mov	edi,[ebp+12]
		mov	ebx,[ebp+16]
		mov	ebp,[ebp+20]
		mov	ecx,32
		xor	edx,edx
; 比較ループ
.loop:
		cmp	edx,[ebx]
		jnz	.exec
; この16dotは処理しない
		add	esi,32
		add	edi,64
		add	ebx,4
		dec	ecx
		jnz	.loop
		jmp	.exit
; この16dotは処理
.exec:
		mov	[ebx],edx
		push	ecx
		mov	ecx,15
; +0, +1
		mov	eax,[esi]
		mov	edx,ecx
		shr	eax,12
		and	edx,eax
		mov	edx,[ebp+edx*4]
		shr	eax,16
		mov	[edi],edx
		mov	[edi+2048],edx
		and	eax,ecx
		mov	edx,[ebp+eax*4]
		mov	[edi+4],edx
		mov	[edi+2048+4],edx
; +2, +3
		mov	eax,[esi+4]
		mov	edx,ecx
		shr	eax,12
		and	edx,eax
		mov	edx,[ebp+edx*4]
		shr	eax,16
		mov	[edi+8],edx
		mov	[edi+2048+8],edx
		and	eax,ecx
		mov	edx,[ebp+eax*4]
		mov	[edi+12],edx
		mov	[edi+2048+12],edx
; +4, +5
		mov	eax,[esi+8]
		mov	edx,ecx
		shr	eax,12
		and	edx,eax
		mov	edx,[ebp+edx*4]
		shr	eax,16
		mov	[edi+16],edx
		mov	[edi+2048+16],edx
		and	eax,ecx
		mov	edx,[ebp+eax*4]
		mov	[edi+20],edx
		mov	[edi+2048+20],edx
; +6, +7
		mov	eax,[esi+12]
		mov	edx,ecx
		shr	eax,12
		and	edx,eax
		mov	edx,[ebp+edx*4]
		shr	eax,16
		mov	[edi+24],edx
		mov	[edi+2048+24],edx
		and	eax,ecx
		mov	edx,[ebp+eax*4]
		mov	[edi+28],edx
		mov	[edi+2048+28],edx
; +8, +9
		mov	eax,[esi+16]
		mov	edx,ecx
		shr	eax,12
		and	edx,eax
		mov	edx,[ebp+edx*4]
		shr	eax,16
		mov	[edi+32],edx
		mov	[edi+2048+32],edx
		and	eax,ecx
		mov	edx,[ebp+eax*4]
		mov	[edi+36],edx
		mov	[edi+2048+36],edx
; +A, +B
		mov	eax,[esi+20]
		mov	edx,ecx
		shr	eax,12
		and	edx,eax
		mov	edx,[ebp+edx*4]
		shr	eax,16
		mov	[edi+40],edx
		mov	[edi+2048+40],edx
		and	eax,ecx
		mov	edx,[ebp+eax*4]
		mov	[edi+44],edx
		mov	[edi+2048+44],edx
; +C, +D
		mov	eax,[esi+24]
		mov	edx,ecx
		shr	eax,12
		and	edx,eax
		mov	edx,[ebp+edx*4]
		shr	eax,16
		mov	[edi+48],edx
		mov	[edi+2048+48],edx
		and	eax,ecx
		mov	edx,[ebp+eax*4]
		mov	[edi+52],edx
		mov	[edi+2048+52],edx
; +E, +F
		mov	eax,[esi+28]
		mov	edx,ecx
		shr	eax,12
		and	edx,eax
		mov	edx,[ebp+edx*4]
		shr	eax,16
		mov	[edi+56],edx
		mov	[edi+2048+56],edx
		and	eax,ecx
		mov	edx,[ebp+eax*4]
		mov	[edi+60],edx
		mov	[edi+2048+60],edx
; 次へ
		pop	ecx
		add	esi,32
		add	edi,64
		add	ebx,4
		xor	edx,edx
		dec	ecx
		jnz	near .loop
; 終了
.exit:
		pop	edi
		pop	esi
		pop	edx
		pop	ecx
		pop	ebx
		pop	eax
		pop	ebp
		ret

;
; レンダラ
; グラフィック(256色モード、ページ0)
;
; void Rend256A(const BYTE *gvram, DWORD *buf, BOOL *flag, const DWORD *pal)
;
_Rend256A:
		push	ebp
		mov	ebp,esp
		push	eax
		push	ebx
		push	ecx
		push	edx
		push	esi
		push	edi
; 初期設定
		mov	esi,[ebp+8]
		mov	edi,[ebp+12]
		mov	ebx,[ebp+16]
		mov	ebp,[ebp+20]
		mov	ecx,32
		xor	edx,edx
; ループ
.loop:
		cmp	edx,[ebx]
		jnz	.exec
; この16dotは処理しない
		add	esi,32
		add	edi,64
		add	ebx,4
		dec	ecx
		jnz	.loop
		jmp	.exit
; この16bitは処理
.exec:
		mov	[ebx],edx
		push	ecx
		push	ebx
		mov	edx,255
; +0, +1
		mov	eax,[esi]
		mov	ebx,edx
		and	ebx,eax
		mov	ecx,[ebp+ebx*4]
		shr	eax,16
		mov	[edi],ecx
		and	eax,edx
		mov	[edi+2048],ecx
		mov	ecx,[ebp+eax*4]
		mov	[edi+4],ecx
		mov	[edi+4+2048],ecx
; +2, +3
		mov	eax,[esi+4]
		mov	ebx,edx
		and	ebx,eax
		mov	ecx,[ebp+ebx*4]
		shr	eax,16
		mov	[edi+8],ecx
		and	eax,edx
		mov	[edi+8+2048],ecx
		mov	ecx,[ebp+eax*4]
		mov	[edi+12],ecx
		mov	[edi+12+2048],ecx
; +4, +5
		mov	eax,[esi+8]
		mov	ebx,edx
		and	ebx,eax
		mov	ecx,[ebp+ebx*4]
		shr	eax,16
		mov	[edi+16],ecx
		and	eax,edx
		mov	[edi+16+2048],ecx
		mov	ecx,[ebp+eax*4]
		mov	[edi+20],ecx
		mov	[edi+20+2048],ecx
; +6, +7
		mov	eax,[esi+12]
		mov	ebx,edx
		and	ebx,eax
		mov	ecx,[ebp+ebx*4]
		shr	eax,16
		mov	[edi+24],ecx
		and	eax,edx
		mov	[edi+24+2048],ecx
		mov	ecx,[ebp+eax*4]
		mov	[edi+28],ecx
		mov	[edi+28+2048],ecx
; +8, +9
		mov	eax,[esi+16]
		mov	ebx,edx
		and	ebx,eax
		mov	ecx,[ebp+ebx*4]
		shr	eax,16
		mov	[edi+32],ecx
		and	eax,edx
		mov	[edi+32+2048],ecx
		mov	ecx,[ebp+eax*4]
		mov	[edi+36],ecx
		mov	[edi+36+2048],ecx
; +A, +B
		mov	eax,[esi+20]
		mov	ebx,edx
		and	ebx,eax
		mov	ecx,[ebp+ebx*4]
		shr	eax,16
		mov	[edi+40],ecx
		and	eax,edx
		mov	[edi+40+2048],ecx
		mov	ecx,[ebp+eax*4]
		mov	[edi+44],ecx
		mov	[edi+44+2048],ecx
; +C, +D
		mov	eax,[esi+24]
		mov	ebx,edx
		and	ebx,eax
		mov	ecx,[ebp+ebx*4]
		shr	eax,16
		mov	[edi+48],ecx
		and	eax,edx
		mov	[edi+48+2048],ecx
		mov	ecx,[ebp+eax*4]
		mov	[edi+52],ecx
		mov	[edi+52+2048],ecx
; +E, +F
		mov	eax,[esi+28]
		mov	ebx,edx
		and	ebx,eax
		mov	ecx,[ebp+ebx*4]
		shr	eax,16
		mov	[edi+56],ecx
		and	eax,edx
		mov	[edi+56+2048],ecx
		mov	ecx,[ebp+eax*4]
		mov	[edi+60],ecx
		mov	[edi+60+2048],ecx
; 次へ
		add	esi,32
		pop	ebx
		add	edi,64
		pop	ecx
		xor	edx,edx
		add	ebx,4
		dec	ecx
		jnz	near .loop
; 終了
.exit:
		pop	edi
		pop	esi
		pop	edx
		pop	ecx
		pop	ebx
		pop	eax
		pop	ebp
		ret

;
; レンダラ
; グラフィック(256色モード、ページ0)
;
; void Rend256B(const BYTE *gvram, DWORD *buf, BOOL *flag, const DWORD *pal)
;
_Rend256B:
		push	ebp
		mov	ebp,esp
		push	eax
		push	ebx
		push	ecx
		push	edx
		push	esi
		push	edi
; 初期設定
		mov	esi,[ebp+8]
		mov	edi,[ebp+12]
		mov	ebx,[ebp+16]
		mov	ebp,[ebp+20]
		mov	ecx,32
		xor	edx,edx
; ループ
.loop:
		cmp	edx,[ebx]
		jnz	.exec
; この16dotは処理しない
		add	esi,32
		add	edi,64
		add	ebx,4
		dec	ecx
		jnz	.loop
		jmp	.exit
; この16bitは処理
.exec:
		mov	[ebx],edx
		push	ecx
		push	ebx
		mov	edx,255
; +0, +1
		mov	eax,[esi]
		shr	eax,8
		mov	ebx,edx
		and	ebx,eax
		mov	ecx,[ebp+ebx*4]
		shr	eax,16
		mov	[edi],ecx
		mov	[edi+2048],ecx
		mov	ecx,[ebp+eax*4]
		mov	[edi+4],ecx
		mov	[edi+4+2048],ecx
; +2, +3
		mov	eax,[esi+4]
		shr	eax,8
		mov	ebx,edx
		and	ebx,eax
		mov	ecx,[ebp+ebx*4]
		shr	eax,16
		mov	[edi+8],ecx
		mov	[edi+8+2048],ecx
		mov	ecx,[ebp+eax*4]
		mov	[edi+12],ecx
		mov	[edi+12+2048],ecx
; +4, +5
		mov	eax,[esi+8]
		shr	eax,8
		mov	ebx,edx
		and	ebx,eax
		mov	ecx,[ebp+ebx*4]
		shr	eax,16
		mov	[edi+16],ecx
		mov	[edi+16+2048],ecx
		mov	ecx,[ebp+eax*4]
		mov	[edi+20],ecx
		mov	[edi+20+2048],ecx
; +6, +7
		mov	eax,[esi+12]
		shr	eax,8
		mov	ebx,edx
		and	ebx,eax
		mov	ecx,[ebp+ebx*4]
		shr	eax,16
		mov	[edi+24],ecx
		mov	[edi+24+2048],ecx
		mov	ecx,[ebp+eax*4]
		mov	[edi+28],ecx
		mov	[edi+28+2048],ecx
; +8, +9
		mov	eax,[esi+16]
		shr	eax,8
		mov	ebx,edx
		and	ebx,eax
		mov	ecx,[ebp+ebx*4]
		shr	eax,16
		mov	[edi+32],ecx
		mov	[edi+32+2048],ecx
		mov	ecx,[ebp+eax*4]
		mov	[edi+36],ecx
		mov	[edi+36+2048],ecx
; +A, +B
		mov	eax,[esi+20]
		shr	eax,8
		mov	ebx,edx
		and	ebx,eax
		mov	ecx,[ebp+ebx*4]
		shr	eax,16
		mov	[edi+40],ecx
		mov	[edi+40+2048],ecx
		mov	ecx,[ebp+eax*4]
		mov	[edi+44],ecx
		mov	[edi+44+2048],ecx
; +C, +D
		mov	eax,[esi+24]
		shr	eax,8
		mov	ebx,edx
		and	ebx,eax
		mov	ecx,[ebp+ebx*4]
		shr	eax,16
		mov	[edi+48],ecx
		mov	[edi+48+2048],ecx
		mov	ecx,[ebp+eax*4]
		mov	[edi+52],ecx
		mov	[edi+52+2048],ecx
; +E, +F
		mov	eax,[esi+28]
		shr	eax,8
		mov	ebx,edx
		and	ebx,eax
		mov	ecx,[ebp+ebx*4]
		shr	eax,16
		mov	[edi+56],ecx
		mov	[edi+56+2048],ecx
		mov	ecx,[ebp+eax*4]
		mov	[edi+60],ecx
		mov	[edi+60+2048],ecx
; 次へ
		add	esi,32
		pop	ebx
		add	edi,64
		pop	ecx
		xor	edx,edx
		add	ebx,4
		dec	ecx
		jnz	near .loop
; 終了
.exit:
		pop	edi
		pop	esi
		pop	edx
		pop	ecx
		pop	ebx
		pop	eax
		pop	ebp
		ret
;
; レンダラ
; グラフィック(256色モード、ページ0、All)
;
; int Rend256C(const BYTE *gvram, DWORD *buf, const DWORD *pal)
;
_Rend256C:
		push	ebp
		mov	ebp,esp
		push	ebx
		push	ecx
		push	edx
		push	esi
		push	edi
; 初期設定
		mov	esi,[ebp+8]
		mov	edi,[ebp+12]
		mov	ebp,[ebp+16]
		xor	ebx,ebx
		mov	ecx,64
		mov	edx,255
; ループ
.loop:
		push	ecx
; +0, +1
		mov	eax,[esi]
		mov	ecx,edx
		and	ecx,eax
		mov	ecx,[ebp+ecx*4]
		shr	eax,16
		cmp	ecx,[edi]
		setnz	bl
		mov	[edi],ecx
		or	bh,bl
		and	eax,edx
		mov	[edi+2048],ecx
		mov	ecx,[ebp+eax*4]
		cmp	ecx,[edi+4]
		setnz	bl
		mov	[edi+4],ecx
		or	bh,bl
		mov	[edi+4+2048],ecx
; +2, +3
		mov	eax,[esi+4]
		mov	ecx,edx
		and	ecx,eax
		mov	ecx,[ebp+ecx*4]
		shr	eax,16
		cmp	ecx,[edi+8]
		setnz	bl
		mov	[edi+8],ecx
		or	bh,bl
		and	eax,edx
		mov	[edi+8+2048],ecx
		mov	ecx,[ebp+eax*4]
		cmp	ecx,[edi+12]
		setnz	bl
		mov	[edi+12],ecx
		or	bh,bl
		mov	[edi+12+2048],ecx
; +4, +5
		mov	eax,[esi+8]
		mov	ecx,edx
		and	ecx,eax
		mov	ecx,[ebp+ecx*4]
		shr	eax,16
		cmp	ecx,[edi+16]
		setnz	bl
		mov	[edi+16],ecx
		or	bh,bl
		and	eax,edx
		mov	[edi+16+2048],ecx
		mov	ecx,[ebp+eax*4]
		cmp	ecx,[edi+20]
		setnz	bl
		mov	[edi+20],ecx
		or	bh,bl
		mov	[edi+20+2048],ecx
; +6, +7
		mov	eax,[esi+12]
		mov	ecx,edx
		and	ecx,eax
		mov	ecx,[ebp+ecx*4]
		shr	eax,16
		cmp	ecx,[edi+24]
		setnz	bl
		mov	[edi+24],ecx
		or	bh,bl
		and	eax,edx
		mov	[edi+24+2048],ecx
		mov	ecx,[ebp+eax*4]
		cmp	ecx,[edi+28]
		setnz	bl
		mov	[edi+28],ecx
		or	bh,bl
		mov	[edi+28+2048],ecx
; 次へ
		add	esi,16
		pop	ecx
		add	edi,32
		dec	ecx
		jnz	near .loop
; 終了
		mov	eax,ebx
		shr	eax,8
		pop	edi
		pop	esi
		pop	edx
		pop	ecx
		pop	ebx
		pop	ebp
		ret

;
; レンダラ
; グラフィック(256色モード、ページ0、All)
;
; int Rend256D(const BYTE *gvram, DWORD *buf, BOOL *flag, const DWORD *pal)
;
_Rend256D:
		push	ebp
		mov	ebp,esp
		push	ebx
		push	ecx
		push	edx
		push	esi
		push	edi
; 初期設定
		mov	esi,[ebp+8]
		mov	edi,[ebp+12]
		mov	ebp,[ebp+16]
		xor	ebx,ebx
		mov	ecx,64
		mov	edx,255
; ループ
.loop:
		push	ecx
; +0, +1
		mov	eax,[esi]
		shr	eax,8
		mov	ecx,edx
		and	ecx,eax
		mov	ecx,[ebp+ecx*4]
		shr	eax,16
		cmp	ecx,[edi]
		setnz	bl
		mov	[edi],ecx
		or	bh,bl
		mov	[edi+2048],ecx
		mov	ecx,[ebp+eax*4]
		cmp	ecx,[edi+4]
		setnz	bl
		mov	[edi+4],ecx
		or	bh,bl
		mov	[edi+4+2048],ecx
; +2, +3
		mov	eax,[esi+4]
		shr	eax,8
		mov	ecx,edx
		and	ecx,eax
		mov	ecx,[ebp+ecx*4]
		shr	eax,16
		cmp	ecx,[edi+8]
		setnz	bl
		mov	[edi+8],ecx
		or	bh,bl
		mov	[edi+8+2048],ecx
		mov	ecx,[ebp+eax*4]
		cmp	ecx,[edi+12]
		setnz	bl
		mov	[edi+12],ecx
		or	bh,bl
		mov	[edi+12+2048],ecx
; +4, +5
		mov	eax,[esi+8]
		shr	eax,8
		mov	ecx,edx
		and	ecx,eax
		mov	ecx,[ebp+ecx*4]
		shr	eax,16
		cmp	ecx,[edi+16]
		setnz	bl
		mov	[edi+16],ecx
		or	bh,bl
		mov	[edi+16+2048],ecx
		mov	ecx,[ebp+eax*4]
		cmp	ecx,[edi+20]
		setnz	bl
		mov	[edi+20],ecx
		or	bh,bl
		mov	[edi+20+2048],ecx
; +6, +7
		mov	eax,[esi+12]
		shr	eax,8
		mov	ecx,edx
		and	ecx,eax
		mov	ecx,[ebp+ecx*4]
		shr	eax,16
		cmp	ecx,[edi+24]
		setnz	bl
		mov	[edi+24],ecx
		or	bh,bl
		mov	[edi+24+2048],ecx
		mov	ecx,[ebp+eax*4]
		cmp	ecx,[edi+28]
		setnz	bl
		mov	[edi+28],ecx
		or	bh,bl
		mov	[edi+28+2048],ecx
; 次へ
		add	esi,16
		pop	ecx
		add	edi,32
		dec	ecx
		jnz	near .loop
; 終了
		mov	eax,ebx
		shr	eax,8
		pop	edi
		pop	esi
		pop	edx
		pop	ecx
		pop	ebx
		pop	ebp
		ret

;
; レンダラ
; グラフィック(65536色モード)
;
; void Rend64KA(BYTE *gvrm, DWORD *buf, BOOL *flag, BYTE *plt, DWORD *color)
;
_Rend64KA:
		push	ebp
		mov	ebp,esp
		push	eax
		push	ebx
		push	ecx
		push	edx
		push	esi
		push	edi
; 初期設定
		mov	esi,[ebp+8]
		mov	edi,[ebp+12]
		mov	ebx,[ebp+16]
		mov	edx,[ebp+20]
		mov	ebp,[ebp+24]
		mov	ecx,32
		xor	eax,eax
; ループ
.loop1:
		cmp	eax,[ebx]
		jnz	.exec
; この16dotは処理しない
		add	esi,32
		add	edi,64
		add	ebx,4
		dec	ecx
		jnz	.loop1
		jmp	.exit
; この16bitは処理
.exec:
		mov	[ebx],eax
		push	ebx
		push	ecx
		mov	ecx,2
.loop2:
		push	ecx
; +0
		mov	ax,word[esi]
		or	ax,ax
		movzx	ebx,ah
		movzx	ecx,al
		mov	bh,[edx+ebx+256]
		setz	al
		mov	bl,[edx+ecx]
		movzx	eax,al
		mov	ecx,[ebp+ebx*4]
		ror	eax,1
		or	eax,ecx
		mov	[edi],eax
		mov	[edi+2048],eax
; +1
		mov	ax,word[esi+2]
		or	ax,ax
		movzx	ebx,ah
		movzx	ecx,al
		mov	bh,[edx+ebx+256]
		setz	al
		mov	bl,[edx+ecx]
		movzx	eax,al
		mov	ecx,[ebp+ebx*4]
		ror	eax,1
		or	eax,ecx
		mov	[edi+4],eax
		mov	[edi+4+2048],eax
; +2
		mov	ax,word[esi+4]
		or	ax,ax
		movzx	ebx,ah
		movzx	ecx,al
		mov	bh,[edx+ebx+256]
		setz	al
		mov	bl,[edx+ecx]
		movzx	eax,al
		mov	ecx,[ebp+ebx*4]
		ror	eax,1
		or	eax,ecx
		mov	[edi+8],eax
		mov	[edi+8+2048],eax
; +3
		mov	ax,word[esi+6]
		or	ax,ax
		movzx	ebx,ah
		movzx	ecx,al
		mov	bh,[edx+ebx+256]
		setz	al
		mov	bl,[edx+ecx]
		movzx	eax,al
		mov	ecx,[ebp+ebx*4]
		ror	eax,1
		or	eax,ecx
		mov	[edi+12],eax
		mov	[edi+12+2048],eax
; +4
		mov	ax,word[esi+8]
		or	ax,ax
		movzx	ebx,ah
		movzx	ecx,al
		mov	bh,[edx+ebx+256]
		setz	al
		mov	bl,[edx+ecx]
		movzx	eax,al
		mov	ecx,[ebp+ebx*4]
		ror	eax,1
		or	eax,ecx
		mov	[edi+16],eax
		mov	[edi+16+2048],eax
; +5
		mov	ax,word[esi+10]
		or	ax,ax
		movzx	ebx,ah
		movzx	ecx,al
		mov	bh,[edx+ebx+256]
		setz	al
		mov	bl,[edx+ecx]
		movzx	eax,al
		mov	ecx,[ebp+ebx*4]
		ror	eax,1
		or	eax,ecx
		mov	[edi+20],eax
		mov	[edi+20+2048],eax
; +6
		mov	ax,word[esi+12]
		or	ax,ax
		movzx	ebx,ah
		movzx	ecx,al
		mov	bh,[edx+ebx+256]
		setz	al
		mov	bl,[edx+ecx]
		movzx	eax,al
		mov	ecx,[ebp+ebx*4]
		ror	eax,1
		or	eax,ecx
		mov	[edi+24],eax
		mov	[edi+24+2048],eax
; +7
		mov	ax,word[esi+14]
		or	ax,ax
		movzx	ebx,ah
		movzx	ecx,al
		mov	bh,[edx+ebx+256]
		setz	al
		mov	bl,[edx+ecx]
		movzx	eax,al
		mov	ecx,[ebp+ebx*4]
		ror	eax,1
		or	eax,ecx
		mov	[edi+28],eax
		mov	[edi+28+2048],eax
; 続き
		add	esi,16
		pop	ecx
		add	edi,32
		dec	ecx
		jnz	near .loop2
; 次の16dotへ
		pop	ecx
		xor	eax,eax
		pop	ebx
		add	ebx,4
		dec	ecx
		jnz	near .loop1
; 終了
.exit:
		pop	edi
		pop	esi
		pop	edx
		pop	ecx
		pop	ebx
		pop	eax
		pop	ebp
		ret

;
; レンダラ
; グラフィック(65536色モード、全て)
;
; void Rend64KB(BYTE *gvrm, DWORD *buf, BYTE *plt, DWORD *color)
;
_Rend64KB:
		push	ebp
		mov	ebp,esp
		xor	eax,eax
		push	eax
		push	ebx
		push	ecx
		push	edx
		push	esi
		push	edi
; 初期設定
		mov	esi,[ebp+8]
		mov	edi,[ebp+12]
		mov	edx,[ebp+16]
		mov	ebp,[ebp+20]
		mov	ecx,64
; ループ
.loop:
		push	ecx
; +0
		mov	ax,word[esi]
		or	ax,ax
		movzx	ebx,ah
		movzx	ecx,al
		mov	bh,[edx+ebx+256]
		setz	al
		mov	bl,[edx+ecx]
		movzx	eax,al
		mov	ecx,[ebp+ebx*4]
		ror	eax,1
		or	eax,ecx
		cmp	eax,[edi]
		setnz	bl
		or	[esp+24],bl
		mov	[edi],eax
		mov	[edi+2048],eax
; +1
		mov	ax,word[esi+2]
		or	ax,ax
		movzx	ebx,ah
		movzx	ecx,al
		mov	bh,[edx+ebx+256]
		setz	al
		mov	bl,[edx+ecx]
		movzx	eax,al
		mov	ecx,[ebp+ebx*4]
		ror	eax,1
		or	eax,ecx
		cmp	eax,[edi+4]
		setnz	bl
		or	[esp+24],bl
		mov	[edi+4],eax
		mov	[edi+4+2048],eax
; +2
		mov	ax,word[esi+4]
		or	ax,ax
		movzx	ebx,ah
		movzx	ecx,al
		mov	bh,[edx+ebx+256]
		setz	al
		mov	bl,[edx+ecx]
		movzx	eax,al
		mov	ecx,[ebp+ebx*4]
		ror	eax,1
		or	eax,ecx
		cmp	eax,[edi+8]
		setnz	bl
		or	[esp+24],bl
		mov	[edi+8],eax
		mov	[edi+8+2048],eax
; +3
		mov	ax,word[esi+6]
		or	ax,ax
		movzx	ebx,ah
		movzx	ecx,al
		mov	bh,[edx+ebx+256]
		setz	al
		mov	bl,[edx+ecx]
		movzx	eax,al
		mov	ecx,[ebp+ebx*4]
		ror	eax,1
		or	eax,ecx
		cmp	eax,[edi+12]
		setnz	bl
		or	[esp+24],bl
		mov	[edi+12],eax
		mov	[edi+12+2048],eax
; +4
		mov	ax,word[esi+8]
		or	ax,ax
		movzx	ebx,ah
		movzx	ecx,al
		mov	bh,[edx+ebx+256]
		setz	al
		mov	bl,[edx+ecx]
		movzx	eax,al
		mov	ecx,[ebp+ebx*4]
		ror	eax,1
		or	eax,ecx
		cmp	eax,[edi+16]
		setnz	bl
		or	[esp+24],bl
		mov	[edi+16],eax
		mov	[edi+16+2048],eax
; +5
		mov	ax,word[esi+10]
		or	ax,ax
		movzx	ebx,ah
		movzx	ecx,al
		mov	bh,[edx+ebx+256]
		setz	al
		mov	bl,[edx+ecx]
		movzx	eax,al
		mov	ecx,[ebp+ebx*4]
		ror	eax,1
		or	eax,ecx
		cmp	eax,[edi+20]
		setnz	bl
		or	[esp+24],bl
		mov	[edi+20],eax
		mov	[edi+20+2048],eax
; +6
		mov	ax,word[esi+12]
		or	ax,ax
		movzx	ebx,ah
		movzx	ecx,al
		mov	bh,[edx+ebx+256]
		setz	al
		mov	bl,[edx+ecx]
		movzx	eax,al
		mov	ecx,[ebp+ebx*4]
		ror	eax,1
		or	eax,ecx
		cmp	eax,[edi+24]
		setnz	bl
		or	[esp+24],bl
		mov	[edi+24],eax
		mov	[edi+24+2048],eax
; +7
		mov	ax,word[esi+14]
		or	ax,ax
		movzx	ebx,ah
		movzx	ecx,al
		mov	bh,[edx+ebx+256]
		setz	al
		mov	bl,[edx+ecx]
		movzx	eax,al
		mov	ecx,[ebp+ebx*4]
		ror	eax,1
		or	eax,ecx
		cmp	eax,[edi+28]
		setnz	bl
		or	[esp+24],bl
		mov	[edi+28],eax
		mov	[edi+28+2048],eax
; 続き
		add	esi,16
		pop	ecx
		add	edi,32
		dec	ecx
		jnz	near .loop
; 終了
.exit:
		pop	edi
		pop	esi
		pop	edx
		pop	ecx
		pop	ebx
		pop	eax
		pop	ebp
		ret

;
; レンダラ
; スプライトクリア
;
; void RendClrSprite(DWORD *buf, DWORD color, int len)
;
_RendClrSprite:
		push	ebp
		mov	ebp,esp
		push	eax
		push	ebx
		push	ecx
		push	esi
		push	edi
; パラメータ受け取り
		mov	esi,[ebp+8]
		mov	eax,[ebp+12]
		mov	ecx,[ebp+16];
; 準備
		mov	edi,esi
		mov	ebx,eax
; サイズあわせ
		mov	edx,ecx
		shr	ecx,4
		and	edx,15
		setnz	dl
		add	cl,dl
; 16ダブルワードクリア
.loop:
		mov	[esi],eax
		mov	[edi+4],ebx
		mov	[esi+8],eax
		mov	[edi+12],ebx
		mov	[esi+16],eax
		mov	[edi+20],ebx
		mov	[esi+24],eax
		mov	[edi+28],ebx
		mov	[esi+32],eax
		mov	[edi+36],ebx
		mov	[esi+40],eax
		mov	[edi+44],ebx
		mov	[esi+48],eax
		mov	[edi+52],ebx
		mov	[esi+56],eax
		mov	[edi+60],ebx
; 次へ
		add	esi,64
		add	edi,64
		dec	ecx
		jnz	.loop
; 終了
		pop	edi
		pop	esi
		pop	ecx
		pop	ebx
		pop	eax
		pop	ebp
		ret

;
; レンダラ
; スプライト(単体)
;
; void RendSprite(DWORD *line, DWORD *buf, DWORD x, BOOL hflag)
;
_RendSprite:
		push	ebp
		mov	ebp,esp
		push	eax
		push	ebx
		push	ecx
		push	edx
		push	esi
		push	edi
; パラメータ受け取り
		mov	esi,[ebp+8]
		mov	edi,[ebp+12]
		mov	ecx,[ebp+16]
		mov	edx,[ebp+20]
; 水平反転か
		or	edx,edx
		jnz	near .reverse
; <16か
		cmp	ecx,16
		jc	near .left
; バッファ加算(16を引くこと)
		sub	ecx,16
		shl	ecx,2
		add	edi,ecx
; 16dot重ね合わせ(カラー0は透明なので、書き込まない)
		mov	edx,80000000h
; +0
		mov	eax,[esi]
		test	eax,edx
		jnz	.dot0
		mov	[edi],eax
.dot0:
; +1
		mov	eax,[esi+4]
		test	eax,edx
		jnz	.dot1
		mov	[edi+4],eax
.dot1:
; +2
		mov	eax,[esi+8]
		test	eax,edx
		jnz	.dot2
		mov	[edi+8],eax
.dot2:
; +3
		mov	eax,[esi+12]
		test	eax,edx
		jnz	.dot3
		mov	[edi+12],eax
.dot3:
; +4
		mov	eax,[esi+16]
		test	eax,edx
		jnz	.dot4
		mov	[edi+16],eax
.dot4:
; +5
		mov	eax,[esi+20]
		test	eax,edx
		jnz	.dot5
		mov	[edi+20],eax
.dot5:
; +6
		mov	eax,[esi+24]
		test	eax,edx
		jnz	.dot6
		mov	[edi+24],eax
.dot6:
; +7
		mov	eax,[esi+28]
		test	eax,edx
		jnz	.dot7
		mov	[edi+28],eax
.dot7:
; +8
		mov	eax,[esi+32]
		test	eax,edx
		jnz	.dot8
		mov	[edi+32],eax
.dot8:
; +9
		mov	eax,[esi+36]
		test	eax,edx
		jnz	.dot9
		mov	[edi+36],eax
.dot9:
; +A
		mov	eax,[esi+40]
		test	eax,edx
		jnz	.dota
		mov	[edi+40],eax
.dota:
; +B
		mov	eax,[esi+44]
		test	eax,edx
		jnz	.dotb
		mov	[edi+44],eax
.dotb:
; +C
		mov	eax,[esi+48]
		test	eax,edx
		jnz	.dotc
		mov	[edi+48],eax
.dotc:
; +D
		mov	eax,[esi+52]
		test	eax,edx
		jnz	.dotd
		mov	[edi+52],eax
.dotd:
; +E
		mov	eax,[esi+56]
		test	eax,edx
		jnz	.dote
		mov	[edi+56],eax
.dote:
; +F
		mov	eax,[esi+60]
		test	eax,edx
		jnz	.dotf
		mov	[edi+60],eax
.dotf:
		pop	edi
		pop	esi
		pop	edx
		pop	ecx
		pop	ebx
		pop	eax
		pop	ebp
		ret
; 水平反転
.reverse:
		cmp	ecx,16
		jc	near .leftreverse
; バッファ加算(16を引くこと)
		sub	ecx,16
		shl	ecx,2
		add	edi,ecx
; 16dot重ね合わせ(カラー0は透明なので、書き込まない)
		mov	edx,80000000h
; +0
		mov	eax,[esi]
		test	eax,edx
		jnz	.dotr0
		mov	[edi+60],eax
.dotr0:
; +1
		mov	eax,[esi+4]
		test	eax,edx
		jnz	.dotr1
		mov	[edi+56],eax
.dotr1:
; +2
		mov	eax,[esi+8]
		test	eax,edx
		jnz	.dotr2
		mov	[edi+52],eax
.dotr2:
; +3
		mov	eax,[esi+12]
		test	eax,edx
		jnz	.dotr3
		mov	[edi+48],eax
.dotr3:
; +4
		mov	eax,[esi+16]
		test	eax,edx
		jnz	.dotr4
		mov	[edi+44],eax
.dotr4:
; +5
		mov	eax,[esi+20]
		test	eax,edx
		jnz	.dotr5
		mov	[edi+40],eax
.dotr5:
; +6
		mov	eax,[esi+24]
		test	eax,edx
		jnz	.dotr6
		mov	[edi+36],eax
.dotr6:
; +7
		mov	eax,[esi+28]
		test	eax,edx
		jnz	.dotr7
		mov	[edi+32],eax
.dotr7:
; +8
		mov	eax,[esi+32]
		test	eax,edx
		jnz	.dotr8
		mov	[edi+28],eax
.dotr8:
; +9
		mov	eax,[esi+36]
		test	eax,edx
		jnz	.dotr9
		mov	[edi+24],eax
.dotr9:
; +A
		mov	eax,[esi+40]
		test	eax,edx
		jnz	.dotra
		mov	[edi+20],eax
.dotra:
; +B
		mov	eax,[esi+44]
		test	eax,edx
		jnz	.dotrb
		mov	[edi+16],eax
.dotrb:
; +C
		mov	eax,[esi+48]
		test	eax,edx
		jnz	.dotrc
		mov	[edi+12],eax
.dotrc:
; +D
		mov	eax,[esi+52]
		test	eax,edx
		jnz	.dotrd
		mov	[edi+8],eax
.dotrd:
; +E
		mov	eax,[esi+56]
		test	eax,edx
		jnz	.dotre
		mov	[edi+4],eax
.dotre:
; +F
		mov	eax,[esi+60]
		test	eax,edx
		jnz	.dotrf
		mov	[edi],eax
.dotrf:
		pop	edi
		pop	esi
		pop	edx
		pop	ecx
		pop	ebx
		pop	eax
		pop	ebp
		ret
; 左
.left:
		mov	ebx,16
		sub	ebx,ecx
		shl	ebx,2
		add	esi,ebx
		mov	edx,80000000h
; 左ループ
.leftloop:
		mov	eax,[esi]
		test	eax,edx
		jnz	.leftok
		mov	[edi],eax
.leftok:
		add	esi,4
		add	edi,4
		dec	ecx
		jnz	.leftloop
; 終了
		pop	edi
		pop	esi
		pop	edx
		pop	ecx
		pop	ebx
		pop	eax
		pop	ebp
		ret
; 左＋左右反転
.leftreverse:
		mov	ebx,ecx
		shl	ebx,2
		add	esi,ebx
		sub	esi,4
		mov	edx,80000000h
; 左ループ
.leftrloop:
		mov	eax,[esi]
		test	eax,edx
		jnz	.leftrok
		mov	[edi],eax
.leftrok:
		sub	esi,4
		add	edi,4
		dec	ecx
		jnz	.leftrloop
; 終了
		pop	edi
		pop	esi
		pop	edx
		pop	ecx
		pop	ebx
		pop	eax
		pop	ebp
		ret

;
; レンダラ
; スプライト(単体、CMOV)
;
; void RendSpriteC(DWORD *line, DWORD *buf, DWORD x, BOOL hflag)
;
_RendSpriteC:
		push	ebp
		mov	ebp,esp
		push	eax
		push	ebx
		push	ecx
		push	edx
		push	esi
		push	edi
; パラメータ受け取り
		mov	esi,[ebp+8]
		mov	edi,[ebp+12]
		mov	ecx,[ebp+16]
		mov	edx,[ebp+20]
; 水平反転か
		or	edx,edx
		jnz	near .reverse
; <16か
		cmp	ecx,16
		jc	near .left
; バッファ加算(16を引くこと)
		sub	ecx,16
		shl	ecx,2
		add	edi,ecx
; 16dot重ね合わせ(カラー0は透明なので、書き込まない)
		mov	edx,80000000h
		mov	ecx,2
.loop:
; +0
		mov	eax,[esi]
		test	eax,edx
		cmovnz	eax,[edi]
		mov	[edi],eax
; +1
		mov	eax,[esi+4]
		test	eax,edx
		cmovnz	eax,[edi+4]
		mov	[edi+4],eax
; +2
		mov	eax,[esi+8]
		test	eax,edx
		cmovnz	eax,[edi+8]
		mov	[edi+8],eax
; +3
		mov	eax,[esi+12]
		test	eax,edx
		cmovnz	eax,[edi+12]
		mov	[edi+12],eax
; +4
		mov	eax,[esi+16]
		test	eax,edx
		cmovnz	eax,[edi+16]
		mov	[edi+16],eax
; +5
		mov	eax,[esi+20]
		test	eax,edx
		cmovnz	eax,[edi+20]
		mov	[edi+20],eax
; +6
		mov	eax,[esi+24]
		test	eax,edx
		cmovnz	eax,[edi+24]
		mov	[edi+24],eax
; +7
		mov	eax,[esi+28]
		test	eax,edx
		cmovnz	eax,[edi+28]
		mov	[edi+28],eax
; 次へ
		add	esi,32
		add	edi,32
		dec	ecx
		jnz	.loop
; 終了
		pop	edi
		pop	esi
		pop	edx
		pop	ecx
		pop	ebx
		pop	eax
		pop	ebp
		ret
; 水平反転
.reverse:
		cmp	ecx,16
		jc	near .leftreverse
; バッファ加算(16を引くこと)
		sub	ecx,16
		shl	ecx,2
		add	edi,ecx
; 16dot重ね合わせ(カラー0は透明なので、書き込まない)
		mov	edx,80000000h
		mov	ecx,2
.reverseloop:
; +0
		mov	eax,[esi]
		test	eax,edx
		cmovnz	eax,[edi+60]
		mov	[edi+60],eax
; +1
		mov	eax,[esi+4]
		test	eax,edx
		cmovnz	eax,[edi+56]
		mov	[edi+56],eax
; +2
		mov	eax,[esi+8]
		test	eax,edx
		cmovnz	eax,[edi+52]
		mov	[edi+52],eax
; +3
		mov	eax,[esi+12]
		test	eax,edx
		cmovnz	eax,[edi+48]
		mov	[edi+48],eax
; +4
		mov	eax,[esi+16]
		test	eax,edx
		cmovnz	eax,[edi+44]
		mov	[edi+44],eax
; +5
		mov	eax,[esi+20]
		test	eax,edx
		cmovnz	eax,[edi+40]
		mov	[edi+40],eax
; +6
		mov	eax,[esi+24]
		test	eax,edx
		cmovnz	eax,[edi+36]
		mov	[edi+36],eax
; +7
		mov	eax,[esi+28]
		test	eax,edx
		cmovnz	eax,[edi+32]
		mov	[edi+32],eax
; 次へ
		add	esi,32
		sub	edi,32
		dec	ecx
		jnz	.reverseloop
; 終了
		pop	edi
		pop	esi
		pop	edx
		pop	ecx
		pop	ebx
		pop	eax
		pop	ebp
		ret
; 左
.left:
		mov	ebx,16
		sub	ebx,ecx
		shl	ebx,2
		add	esi,ebx
		mov	edx,80000000h
; 左ループ
.leftloop:
		mov	eax,[esi]
		test	eax,edx
		jnz	.leftok
		mov	[edi],eax
.leftok:
		add	esi,4
		add	edi,4
		dec	ecx
		jnz	.leftloop
; 終了
		pop	edi
		pop	esi
		pop	edx
		pop	ecx
		pop	ebx
		pop	eax
		pop	ebp
		ret
; 左＋左右反転
.leftreverse:
		mov	ebx,ecx
		shl	ebx,2
		add	esi,ebx
		sub	esi,4
		mov	edx,80000000h
; 左ループ
.leftrloop:
		mov	eax,[esi]
		test	eax,edx
		jnz	.leftrok
		mov	[edi],eax
.leftrok:
		sub	esi,4
		add	edi,4
		dec	ecx
		jnz	.leftrloop
; 終了
		pop	edi
		pop	esi
		pop	edx
		pop	ecx
		pop	ebx
		pop	eax
		pop	ebp
		ret

;
; レンダラ
; PCG(NewVer)
;
; void RendPCGNew(DWORD index, BYTE *mem, DWORD *buf, DWORD *pal)
;
_RendPCGNew:
		push	ebp
		mov	ebp,esp
		push	eax
		push	ebx
		push	ecx
		push	edx
		push	esi
		push	edi
; パラメータ受け取り
		mov	eax,[ebp+8]
		mov	esi,[ebp+12]
		mov	edi,[ebp+16]
		mov	ebp,[ebp+20]
; スプライトポインタ ((index & 0xff) << 7) + 0x8000
		movzx	ebx,al
		shl	ebx,7
		add	ebx,8000h
		add	esi,ebx
; バッファポインタ (DWORD*)(index << 8)
		shl	eax,10
		add	edi,eax
; パレットポインタ (DWORD*)((index >> 8) << 4) + 0x100)
		shr	eax,12
		and	eax,3c0h
		add	eax,400h
		add	ebp,eax
; パラメータ準備
		mov	ebx,15
		mov	ecx,16
; ループ
.loop:
		mov	eax,[esi]
		rol	eax,16
; +0
		rol	eax,4
		mov	edx,eax
		and	edx,ebx
		mov	edx,[ebp+edx*4]
		mov	[edi],edx
; +1
		rol	eax,4
		mov	edx,eax
		and	edx,ebx
		mov	edx,[ebp+edx*4]
		mov	[edi+4],edx
; +2
		rol	eax,4
		mov	edx,eax
		and	edx,ebx
		mov	edx,[ebp+edx*4]
		mov	[edi+8],edx
; +3
		rol	eax,4
		mov	edx,eax
		and	edx,ebx
		mov	edx,[ebp+edx*4]
		mov	[edi+12],edx
; +4
		rol	eax,4
		mov	edx,eax
		and	edx,ebx
		mov	edx,[ebp+edx*4]
		mov	[edi+16],edx
; +5
		rol	eax,4
		mov	edx,eax
		and	edx,ebx
		mov	edx,[ebp+edx*4]
		mov	[edi+20],edx
; +6
		rol	eax,4
		mov	edx,eax
		and	edx,ebx
		mov	edx,[ebp+edx*4]
		mov	[edi+24],edx
; +7
		rol	eax,4
		mov	edx,eax
		and	edx,ebx
		mov	edx,[ebp+edx*4]
		mov	[edi+28],edx
; 次のフェッチ
		mov	eax,[esi+64]
		rol	eax,16
; +8
		rol	eax,4
		mov	edx,eax
		and	edx,ebx
		mov	edx,[ebp+edx*4]
		mov	[edi+32],edx
; +9
		rol	eax,4
		mov	edx,eax
		and	edx,ebx
		mov	edx,[ebp+edx*4]
		mov	[edi+36],edx
; +10
		rol	eax,4
		mov	edx,eax
		and	edx,ebx
		mov	edx,[ebp+edx*4]
		mov	[edi+40],edx
; +11
		rol	eax,4
		mov	edx,eax
		and	edx,ebx
		mov	edx,[ebp+edx*4]
		mov	[edi+44],edx
; +12
		rol	eax,4
		mov	edx,eax
		and	edx,ebx
		mov	edx,[ebp+edx*4]
		mov	[edi+48],edx
; +13
		rol	eax,4
		mov	edx,eax
		and	edx,ebx
		mov	edx,[ebp+edx*4]
		mov	[edi+52],edx
; +14
		rol	eax,4
		mov	edx,eax
		and	edx,ebx
		mov	edx,[ebp+edx*4]
		mov	[edi+56],edx
; +15
		rol	eax,4
		mov	edx,eax
		and	edx,ebx
		mov	edx,[ebp+edx*4]
		mov	[edi+60],edx
; 16dot終了、次へ
		add	esi,4
		add	edi,64
		dec	ecx
		jnz	near .loop
; 終了
.exit:
		pop	edi
		pop	esi
		pop	edx
		pop	ecx
		pop	ebx
		pop	eax
		pop	ebp
		ret

;
; レンダラ
; BG(8x8、割り切れる)
;
; void RendBG8(DWORD **ptr, DWORD *buf, int x, int len,
;		BOOL *ready, BYTE *mem, DWORD *pcgbuf, DWORD *pal);
;
; ptr		[ebp+8]
; buf		[ebp+12]
; x		[ebp+16]
; len		[ebp+20]
; ready		[ebp+24]
; mem		[ebp+28]
; pcgbuf	[ebp+32]
; pal		[ebp+36]
;
_RendBG8:
		push	ebp
		mov	ebp,esp
		push	eax
		push	ebx
		push	ecx
		push	edx
		push	esi
		push	edi
; ptrを一度保存して、x<<3を加算
		mov	esi,[ebp+8]
		push	esi
		mov	ebx,[ebp+16]
		shl	ebx,3
		add	esi,ebx
; ecx=len>>3。lenが8で割り切れなければ、+1
		mov	ecx,[ebp+20]
		mov	edx,ecx
		shr	ecx,3
		and	edx,7
		setnz	dl
		movzx	edx,dl
		add	ecx,edx
		push	ecx
; xをlenと足して、65以上ならループ
		add	ecx,[ebp+16]
		cmp	ecx,65
		jc	.normal
; ループする。1回目をecx,2回目を[ebp+16]に設定
		pop	edx
		mov	ecx,64
		sub	ecx,[ebp+16]
		sub	edx,ecx
		mov	[ebp+16],edx
		jmp	short .param
; ループしない。1回目ecx、[ebp+16]は0
.normal;
		pop	ecx
		xor	eax,eax
		mov	[ebp+16],eax
; パラメータ受け取り。esi=ptr+(x*8) edi=buf ebx=ready ecx=chars
.param:
		mov	edi,[ebp+12]
		mov	ebx,[ebp+24]
; ループ
.loop:
		push	esi
		mov	edx,[esi+4]
		and	edx,0fffh
		mov	eax,[ebx+edx*4]
		or	eax,eax
		jz	near .notready
; 左右反転チェック
.check:
		mov	edx,[esi+4]
		and	edx,4000h
		jnz	near .reverse
; 左右反転なし
		mov	esi,[esi]
		mov	edx,00ffffffh
; dot0
.dot0:
		mov	eax,[esi]
		test	eax,edx
		jz	.dot1
		mov	[edi],eax
; dot1
.dot1:
		mov	eax,[esi+4]
		test	eax,edx
		jz	.dot2
		mov	[edi+4],eax
; dot2
.dot2:
		mov	eax,[esi+8]
		test	eax,edx
		jz	.dot3
		mov	[edi+8],eax
; dot3
.dot3:
		mov	eax,[esi+12]
		test	eax,edx
		jz	.dot4
		mov	[edi+12],eax
; dot4
.dot4:
		mov	eax,[esi+16]
		test	eax,edx
		jz	.dot5
		mov	[edi+16],eax
; dot5
.dot5:
		mov	eax,[esi+20]
		test	eax,edx
		jz	.dot6
		mov	[edi+20],eax
; dot6
.dot6:
		mov	eax,[esi+24]
		test	eax,edx
		jz	.dot7
		mov	[edi+24],eax
; dot7
.dot7:
		mov	eax,[esi+28]
		test	eax,edx
		jz	.next
		mov	[edi+28],eax
; 次へ
.next:
		pop	esi
		add	edi,32
		add	esi,8
		dec	ecx
		jnz	near .loop
; 2回目へ移行
		pop	esi
		push	esi
		xor	eax,eax
		mov	ecx,[ebp+16]
		mov	[ebp+16],eax
		or	ecx,ecx
		jnz	near .loop
; 終了
		pop	esi
;
		pop	edi
		pop	esi
		pop	edx
		pop	ecx
		pop	ebx
		pop	eax
		pop	ebp
		ret
; 反転パターン
.reverse:
		mov	esi,[esi]
		mov	edx,00ffffffh
; dot0
.rdot0:
		mov	eax,[esi]
		test	eax,edx
		jz	.rdot1
		mov	[edi+28],eax
; dot1
.rdot1:
		mov	eax,[esi+4]
		test	eax,edx
		jz	.rdot2
		mov	[edi+24],eax
; dot2
.rdot2:
		mov	eax,[esi+8]
		test	eax,edx
		jz	.rdot3
		mov	[edi+20],eax
; dot3
.rdot3:
		mov	eax,[esi+12]
		test	eax,edx
		jz	.rdot4
		mov	[edi+16],eax
; dot4
.rdot4:
		mov	eax,[esi+16]
		test	eax,edx
		jz	.rdot5
		mov	[edi+12],eax
; dot5
.rdot5:
		mov	eax,[esi+20]
		test	eax,edx
		jz	.rdot6
		mov	[edi+8],eax
; dot6
.rdot6:
		mov	eax,[esi+24]
		test	eax,edx
		jz	.rdot7
		mov	[edi+4],eax
; dot7
.rdot7:
		mov	eax,[esi+28]
		test	eax,edx
		jz	near .next
		mov	[edi],eax
		jmp	.next
; PCG未登録
.notready:
		inc	eax
		mov	[ebx+edx*4],eax
; RendPCGNewの呼び出し
		mov	eax,[ebp+36]
		push	eax
		mov	eax,[ebp+32]
		push	eax
		mov	eax,[ebp+28]
		push	eax
		push	edx
		call	_RendPCGNew
		add	esp,16
		jmp	.check

;
; レンダラ
; BG(8x8、割り切れる、CMOV)
;
; void RendBG8C(DWORD **ptr, DWORD *buf, int x, int len,
;		BOOL *ready, BYTE *mem, DWORD *pcgbuf, DWORD *pal);
;
; ptr		[ebp+8]
; buf		[ebp+12]
; x		[ebp+16]
; len		[ebp+20]
; ready		[ebp+24]
; mem		[ebp+28]
; pcgbuf	[ebp+32]
; pal		[ebp+36]
;
_RendBG8C:
		push	ebp
		mov	ebp,esp
		push	eax
		push	ebx
		push	ecx
		push	edx
		push	esi
		push	edi
; ptrを一度保存して、x<<3を加算
		mov	esi,[ebp+8]
		push	esi
		mov	ebx,[ebp+16]
		shl	ebx,3
		add	esi,ebx
; ecx=len>>3。lenが8で割り切れなければ、+1
		mov	ecx,[ebp+20]
		mov	edx,ecx
		shr	ecx,3
		and	edx,7
		setnz	dl
		movzx	edx,dl
		add	ecx,edx
		push	ecx
; xをlenと足して、65以上ならループ
		add	ecx,[ebp+16]
		cmp	ecx,65
		jc	.normal
; ループする。1回目をecx,2回目を[ebp+16]に設定
		pop	edx
		mov	ecx,64
		sub	ecx,[ebp+16]
		sub	edx,ecx
		mov	[ebp+16],edx
		jmp	short .param
; ループしない。1回目ecx、[ebp+16]は0
.normal;
		pop	ecx
		xor	eax,eax
		mov	[ebp+16],eax
; パラメータ受け取り。esi=ptr+(x*8) edi=buf ebx=ready ecx=chars
.param:
		mov	edi,[ebp+12]
		mov	ebx,[ebp+24]
; ループ
.loop:
		push	esi
		mov	edx,[esi+4]
		and	edx,0fffh
		mov	eax,[ebx+edx*4]
		or	eax,eax
		jz	near .notready
; 左右反転チェック
.check:
		mov	edx,[esi+4]
		and	edx,4000h
		jnz	near .reverse
; 左右反転なし
		mov	esi,[esi]
		mov	edx,00ffffffh
; +0
		mov	eax,[esi]
		test	eax,edx
		cmovz	eax,[edi]
		mov	[edi],eax
; +1
		mov	eax,[esi+4]
		test	eax,edx
		cmovz	eax,[edi+4]
		mov	[edi+4],eax
; +2
		mov	eax,[esi+8]
		test	eax,edx
		cmovz	eax,[edi+8]
		mov	[edi+8],eax
; +3
		mov	eax,[esi+12]
		test	eax,edx
		cmovz	eax,[edi+12]
		mov	[edi+12],eax
; +4
		mov	eax,[esi+16]
		test	eax,edx
		cmovz	eax,[edi+16]
		mov	[edi+16],eax
; +5
		mov	eax,[esi+20]
		test	eax,edx
		cmovz	eax,[edi+20]
		mov	[edi+20],eax
; +6
		mov	eax,[esi+24]
		test	eax,edx
		cmovz	eax,[edi+24]
		mov	[edi+24],eax
; +7
		mov	eax,[esi+28]
		test	eax,edx
		cmovz	eax,[edi+28]
		mov	[edi+28],eax
; 次へ
.next:
		pop	esi
		add	edi,32
		add	esi,8
		dec	ecx
		jnz	near .loop
; 2回目へ移行
		pop	esi
		push	esi
		xor	eax,eax
		mov	ecx,[ebp+16]
		mov	[ebp+16],eax
		or	ecx,ecx
		jnz	near .loop
; 終了
		pop	esi
;
		pop	edi
		pop	esi
		pop	edx
		pop	ecx
		pop	ebx
		pop	eax
		pop	ebp
		ret
; 反転パターン
.reverse:
		mov	esi,[esi]
		mov	edx,00ffffffh
; +0
		mov	eax,[esi]
		test	eax,edx
		cmovz	eax,[edi+28]
		mov	[edi+28],eax
; +1
		mov	eax,[esi+4]
		test	eax,edx
		cmovz	eax,[edi+24]
		mov	[edi+24],eax
; +2
		mov	eax,[esi+8]
		test	eax,edx
		cmovz	eax,[edi+20]
		mov	[edi+20],eax
; +3
		mov	eax,[esi+12]
		test	eax,edx
		cmovz	eax,[edi+16]
		mov	[edi+16],eax
; +4
		mov	eax,[esi+16]
		test	eax,edx
		cmovz	eax,[edi+12]
		mov	[edi+12],eax
; +5
		mov	eax,[esi+20]
		test	eax,edx
		cmovz	eax,[edi+8]
		mov	[edi+8],eax
; +6
		mov	eax,[esi+24]
		test	eax,edx
		cmovz	eax,[edi+4]
		mov	[edi+4],eax
; +7
		mov	eax,[esi+28]
		test	eax,edx
		cmovz	eax,[edi]
		mov	[edi],eax
		jmp	.next
; PCG未登録
.notready:
		inc	eax
		mov	[ebx+edx*4],eax
; RendPCGNewの呼び出し
		mov	eax,[ebp+36]
		push	eax
		mov	eax,[ebp+32]
		push	eax
		mov	eax,[ebp+28]
		push	eax
		push	edx
		call	_RendPCGNew
		add	esp,16
		jmp	.check

; レンダラ
; BG(8x8、割り切れる、部分のみ)
;
; void RendBG8P(DWORD **ptr, DWORD *buf, int offset, int length,
;		BOOL *ready, BYTE *mem, DWORD *pcgbuf, DWORD *pal);
;
; ptr		[ebp+8]
; buf		[ebp+12]
; offset	[ebp+16]
; length	[ebp+20]
; ready		[ebp+24]
; mem		[ebp+28]
; pcgbuf	[ebp+32]
; pal		[ebp+36]
;
_RendBG8P:
		push	ebp
		mov	ebp,esp
		push	eax
		push	ebx
		push	ecx
		push	edx
		push	esi
		push	edi
; パラメータ受け取り。esi=ptr edi=buf ebx=ready
		mov	esi,[ebp+8]
		mov	edi,[ebp+12]
		mov	ebx,[ebp+24]
; 登録検査
		mov	edx,[esi+4]
		and	edx,0fffh
		mov	eax,[ebx+edx*4]
		or	eax,eax
		jnz	.ready
; 登録
		inc	eax
		mov	[ebx+edx*4],eax
; RendPCGNewの呼び出し
		mov	eax,[ebp+36]
		push	eax
		mov	eax,[ebp+32]
		push	eax
		mov	eax,[ebp+28]
		push	eax
		push	edx
		call	_RendPCGNew
		add	esp,16
; レディ。残りを受け取る
.ready:
		mov	edx,[ebp+16]
		mov	ecx,[ebp+20]
; 左右反転チェック
		mov	eax,[esi+4]
		and	eax,4000h
		jnz	.reverse
; 正常。esi += (offset * 4)
		mov	esi,[esi]
		shl	edx,2
		add	esi,edx
; 処理
		mov	edx,00ffffffh
.loop:
		mov	eax,[esi]
		test	eax,edx
		jz	.next
		mov	[edi],eax
.next:
		add	esi,4
		add	edi,4
		dec	ecx
		jnz	.loop
; 終了
		pop	edi
		pop	esi
		pop	edx
		pop	ecx
		pop	ebx
		pop	eax
		pop	ebp
		ret
; 左右反転。esi += (7 - offset) * 4
.reverse:
		mov	esi,[esi]
		mov	eax,7
		sub	eax,edx
		shl	eax,2
		add	esi,eax
; 処理
		mov	edx,00ffffffh
.loopr:
		mov	eax,[esi]
		test	eax,edx
		jz	.nextr
		mov	[edi],eax
.nextr:
		sub	esi,4
		add	edi,4
		dec	ecx
		jnz	.loopr
; 終了
		pop	edi
		pop	esi
		pop	edx
		pop	ecx
		pop	ebx
		pop	eax
		pop	ebp
		ret

;
; レンダラ
; BG(16x16、割り切れる)
;
; void RendBG16(DWORD **ptr, DWORD *buf, int x, int len,
;		BOOL *ready, BYTE *mem, DWORD *pcgbuf, DWORD *pal);
;
; ptr		[ebp+8]
; buf		[ebp+12]
; x		[ebp+16]
; len		[ebp+20]
; ready		[ebp+24]
; mem		[ebp+28]
; pcgbuf	[ebp+32]
; pal		[ebp+36]
;
_RendBG16:
		push	ebp
		mov	ebp,esp
		push	eax
		push	ebx
		push	ecx
		push	edx
		push	esi
		push	edi
; ptrを一度保存して、x<<3を加算
		mov	esi,[ebp+8]
		push	esi
		mov	ebx,[ebp+16]
		shl	ebx,3
		add	esi,ebx
; ecx=len>>4。lenが16で割り切れなければ、+1
		mov	ecx,[ebp+20]
		mov	edx,ecx
		shr	ecx,4
		and	edx,15
		setnz	dl
		movzx	edx,dl
		add	ecx,edx
		push	ecx
; xをlenと足して、65以上ならループ
		add	ecx,[ebp+16]
		cmp	ecx,65
		jc	.normal
; ループする。1回目をecx,2回目を[ebp+16]に設定
		pop	edx
		mov	ecx,64
		sub	ecx,[ebp+16]
		sub	edx,ecx
		mov	[ebp+16],edx
		jmp	short .param
; ループしない。1回目ecx、[ebp+16]は0
.normal;
		pop	ecx
		xor	eax,eax
		mov	[ebp+16],eax
; パラメータ受け取り。esi=ptr+(x*8) edi=buf ebx=ready ecx=chars
.param:
		mov	edi,[ebp+12]
		mov	ebx,[ebp+24]
; ループ
.loop:
		push	esi
		mov	edx,[esi+4]
		and	edx,0fffh
		mov	eax,[ebx+edx*4]
		or	eax,eax
		jz	near .notready
; 左右反転チェック
.check:
		mov	edx,[esi+4]
		and	edx,4000h
		jnz	near .reverse
; 左右反転なし
		mov	esi,[esi]
		mov	edx,00ffffffh
		push	ecx
		mov	ecx,2
; dot0
.dot0:
		mov	eax,[esi]
		test	eax,edx
		jz	.dot1
		mov	[edi],eax
; dot1
.dot1:
		mov	eax,[esi+4]
		test	eax,edx
		jz	.dot2
		mov	[edi+4],eax
; dot2
.dot2:
		mov	eax,[esi+8]
		test	eax,edx
		jz	.dot3
		mov	[edi+8],eax
; dot3
.dot3:
		mov	eax,[esi+12]
		test	eax,edx
		jz	.dot4
		mov	[edi+12],eax
; dot4
.dot4:
		mov	eax,[esi+16]
		test	eax,edx
		jz	.dot5
		mov	[edi+16],eax
; dot5
.dot5:
		mov	eax,[esi+20]
		test	eax,edx
		jz	.dot6
		mov	[edi+20],eax
; dot6
.dot6:
		mov	eax,[esi+24]
		test	eax,edx
		jz	.dot7
		mov	[edi+24],eax
; dot7
.dot7:
		mov	eax,[esi+28]
		test	eax,edx
		jz	.next
		mov	[edi+28],eax
; 次へ
.next:
		add	esi,32
		add	edi,32
		dec	ecx
		jnz	near .dot0
; 次へ共通
.nextcmn:
		pop	ecx
		pop	esi
		add	esi,8
		dec	ecx
		jnz	near .loop
; 2回目へ移行
		pop	esi
		push	esi
		xor	eax,eax
		mov	ecx,[ebp+16]
		mov	[ebp+16],eax
		or	ecx,ecx
		jnz	near .loop
; 終了
		pop	esi
;
		pop	edi
		pop	esi
		pop	edx
		pop	ecx
		pop	ebx
		pop	eax
		pop	ebp
		ret
; 反転パターン
.reverse:
		mov	esi,[esi]
		mov	edx,00ffffffh
		push	ecx
		mov	ecx,2
		add	edi,32
; dot0
.rdot0:
		mov	eax,[esi]
		test	eax,edx
		jz	.rdot1
		mov	[edi+28],eax
; dot1
.rdot1:
		mov	eax,[esi+4]
		test	eax,edx
		jz	.rdot2
		mov	[edi+24],eax
; dot2
.rdot2:
		mov	eax,[esi+8]
		test	eax,edx
		jz	.rdot3
		mov	[edi+20],eax
; dot3
.rdot3:
		mov	eax,[esi+12]
		test	eax,edx
		jz	.rdot4
		mov	[edi+16],eax
; dot4
.rdot4:
		mov	eax,[esi+16]
		test	eax,edx
		jz	.rdot5
		mov	[edi+12],eax
; dot5
.rdot5:
		mov	eax,[esi+20]
		test	eax,edx
		jz	.rdot6
		mov	[edi+8],eax
; dot6
.rdot6:
		mov	eax,[esi+24]
		test	eax,edx
		jz	.rdot7
		mov	[edi+4],eax
; dot7
.rdot7:
		mov	eax,[esi+28]
		test	eax,edx
		jz	near .rdot8
		mov	[edi],eax
; 次へ
.rdot8:
		add	esi,32
		sub	edi,32
		dec	ecx
		jnz	near .rdot0
		add	edi,96
		jmp	.nextcmn
; PCG未登録
.notready:
		inc	eax
		mov	[ebx+edx*4],eax
; RendPCGNewの呼び出し
		mov	eax,[ebp+36]
		push	eax
		mov	eax,[ebp+32]
		push	eax
		mov	eax,[ebp+28]
		push	eax
		push	edx
		call	_RendPCGNew
		add	esp,16
		jmp	.check

;
; レンダラ
; BG(16x16、割り切れる、CMOV)
;
; void RendBG16C(DWORD **ptr, DWORD *buf, int x, int len,
;		BOOL *ready, BYTE *mem, DWORD *pcgbuf, DWORD *pal);
;
; ptr		[ebp+8]
; buf		[ebp+12]
; x		[ebp+16]
; len		[ebp+20]
; ready		[ebp+24]
; mem		[ebp+28]
; pcgbuf	[ebp+32]
; pal		[ebp+36]
;
_RendBG16C:
		push	ebp
		mov	ebp,esp
		push	eax
		push	ebx
		push	ecx
		push	edx
		push	esi
		push	edi
; ptrを一度保存して、x<<3を加算
		mov	esi,[ebp+8]
		push	esi
		mov	ebx,[ebp+16]
		shl	ebx,3
		add	esi,ebx
; ecx=len>>4。lenが16で割り切れなければ、+1
		mov	ecx,[ebp+20]
		mov	edx,ecx
		shr	ecx,4
		and	edx,15
		setnz	dl
		movzx	edx,dl
		add	ecx,edx
		push	ecx
; xをlenと足して、65以上ならループ
		add	ecx,[ebp+16]
		cmp	ecx,65
		jc	.normal
; ループする。1回目をecx,2回目を[ebp+16]に設定
		pop	edx
		mov	ecx,64
		sub	ecx,[ebp+16]
		sub	edx,ecx
		mov	[ebp+16],edx
		jmp	short .param
; ループしない。1回目ecx、[ebp+16]は0
.normal;
		pop	ecx
		xor	eax,eax
		mov	[ebp+16],eax
; パラメータ受け取り。esi=ptr+(x*8) edi=buf ebx=ready ecx=chars
.param:
		mov	edi,[ebp+12]
		mov	ebx,[ebp+24]
; ループ
.loop:
		push	esi
		mov	edx,[esi+4]
		and	edx,0fffh
		mov	eax,[ebx+edx*4]
		or	eax,eax
		jz	near .notready
; 左右反転チェック
.check:
		mov	edx,[esi+4]
		and	edx,4000h
		jnz	near .reverse
; 左右反転なし
		mov	esi,[esi]
		mov	edx,00ffffffh
		push	ecx
		mov	ecx,2
; +0
.dot0:
		mov	eax,[esi]
		test	eax,edx
		cmovz	eax,[edi]
		mov	[edi],eax
; +1
		mov	eax,[esi+4]
		test	eax,edx
		cmovz	eax,[edi+4]
		mov	[edi+4],eax
; +2
		mov	eax,[esi+8]
		test	eax,edx
		cmovz	eax,[edi+8]
		mov	[edi+8],eax
; +3
		mov	eax,[esi+12]
		test	eax,edx
		cmovz	eax,[edi+12]
		mov	[edi+12],eax
; +4
		mov	eax,[esi+16]
		test	eax,edx
		cmovz	eax,[edi+16]
		mov	[edi+16],eax
; +5
		mov	eax,[esi+20]
		test	eax,edx
		cmovz	eax,[edi+20]
		mov	[edi+20],eax
; +6
		mov	eax,[esi+24]
		test	eax,edx
		cmovz	eax,[edi+24]
		mov	[edi+24],eax
; +7
		mov	eax,[esi+28]
		test	eax,edx
		cmovz	eax,[edi+28]
		mov	[edi+28],eax
; 次へ
		add	esi,32
		add	edi,32
		dec	ecx
		jnz	near .dot0
.next:
		pop	ecx
		pop	esi
		add	esi,8
		dec	ecx
		jnz	near .loop
; 2回目へ移行
		pop	esi
		push	esi
		xor	eax,eax
		mov	ecx,[ebp+16]
		mov	[ebp+16],eax
		or	ecx,ecx
		jnz	near .loop
; 終了
		pop	esi
;
		pop	edi
		pop	esi
		pop	edx
		pop	ecx
		pop	ebx
		pop	eax
		pop	ebp
		ret
; 反転パターン
.reverse:
		mov	esi,[esi]
		mov	edx,00ffffffh
		push	ecx
		mov	ecx,2
		add	edi,32
; +0
.rdot0:
		mov	eax,[esi]
		test	eax,edx
		cmovz	eax,[edi+28]
		mov	[edi+28],eax
; +1
		mov	eax,[esi+4]
		test	eax,edx
		cmovz	eax,[edi+24]
		mov	[edi+24],eax
; +2
		mov	eax,[esi+8]
		test	eax,edx
		cmovz	eax,[edi+20]
		mov	[edi+20],eax
; +3
		mov	eax,[esi+12]
		test	eax,edx
		cmovz	eax,[edi+16]
		mov	[edi+16],eax
; +4
		mov	eax,[esi+16]
		test	eax,edx
		cmovz	eax,[edi+12]
		mov	[edi+12],eax
; +5
		mov	eax,[esi+20]
		test	eax,edx
		cmovz	eax,[edi+8]
		mov	[edi+8],eax
; +6
		mov	eax,[esi+24]
		test	eax,edx
		cmovz	eax,[edi+4]
		mov	[edi+4],eax
; +7
		mov	eax,[esi+28]
		test	eax,edx
		cmovz	eax,[edi]
		mov	[edi],eax
; 次へ
		add	esi,32
		sub	edi,32
		dec	ecx
		jnz	near .rdot0
		add	edi,96
		jmp	.next
; PCG未登録
.notready:
		inc	eax
		mov	[ebx+edx*4],eax
; RendPCGNewの呼び出し
		mov	eax,[ebp+36]
		push	eax
		mov	eax,[ebp+32]
		push	eax
		mov	eax,[ebp+28]
		push	eax
		push	edx
		call	_RendPCGNew
		add	esp,16
		jmp	.check

; レンダラ
; BG(16x16、割り切れる、部分のみ)
;
; void RendBG16P(DWORD **ptr, DWORD *buf, int offset, int length,
;		BOOL *ready, BYTE *mem, DWORD *pcgbuf, DWORD *pal);
;
; ptr		[ebp+8]
; buf		[ebp+12]
; offset	[ebp+16]
; length	[ebp+20]
; ready		[ebp+24]
; mem		[ebp+28]
; pcgbuf	[ebp+32]
; pal		[ebp+36]
;
_RendBG16P:
		push	ebp
		mov	ebp,esp
		push	eax
		push	ebx
		push	ecx
		push	edx
		push	esi
		push	edi
; パラメータ受け取り。esi=ptr edi=buf ebx=ready
		mov	esi,[ebp+8]
		mov	edi,[ebp+12]
		mov	ebx,[ebp+24]
; 登録検査
		mov	edx,[esi+4]
		and	edx,0fffh
		mov	eax,[ebx+edx*4]
		or	eax,eax
		jnz	.ready
; 登録
		inc	eax
		mov	[ebx+edx*4],eax
; RendPCGNewの呼び出し
		mov	eax,[ebp+36]
		push	eax
		mov	eax,[ebp+32]
		push	eax
		mov	eax,[ebp+28]
		push	eax
		push	edx
		call	_RendPCGNew
		add	esp,16
; レディ。残りを受け取る
.ready:
		mov	edx,[ebp+16]
		mov	ecx,[ebp+20]
; 左右反転チェック
		mov	eax,[esi+4]
		and	eax,4000h
		jnz	.reverse
; 正常。esi += (offset * 4)
		mov	esi,[esi]
		shl	edx,2
		add	esi,edx
; 処理
		mov	edx,00ffffffh
.loop:
		mov	eax,[esi]
		test	eax,edx
		jz	.next
		mov	[edi],eax
.next:
		add	esi,4
		add	edi,4
		dec	ecx
		jnz	.loop
; 終了
		pop	edi
		pop	esi
		pop	edx
		pop	ecx
		pop	ebx
		pop	eax
		pop	ebp
		ret
; 左右反転。esi += (15 - offset) * 4
.reverse:
		mov	esi,[esi]
		mov	eax,15
		sub	eax,edx
		shl	eax,2
		add	esi,eax
; 処理
		mov	edx,00ffffffh
.loopr:
		mov	eax,[esi]
		test	eax,edx
		jz	.nextr
		mov	[edi],eax
.nextr:
		sub	esi,4
		add	edi,4
		dec	ecx
		jnz	.loopr
; 終了
		pop	edi
		pop	esi
		pop	edx
		pop	ecx
		pop	ebx
		pop	eax
		pop	ebp
		ret

;
; レンダラ
; 合成(0面)
;
; void RendMix00(DWORD *buf, BOOL *flag, int len)
;
_RendMix00:
		push	ebp
		mov	ebp,esp
		push	eax
		push	ebx
		push	ecx
		push	edx
		push	esi
		push	edi
; パラメータ受け取り
		mov	edi,[ebp+8]
		mov	ebx,[ebp+12]
		mov	ecx,[ebp+16]
		xor	eax,eax
; 16dot単位で処理
		push	ecx
		shr	ecx,4
		jz	near .rest
; 16dotループ
.loop1:
		xor	edx,edx
; +0
		cmp	eax,[edi]
		setnz	dl
		mov	[edi],eax
		or	dh,dl
; +1
		cmp	eax,[edi+4]
		setnz	dl
		mov	[edi+4],eax
		or	dh,dl
; +2
		cmp	eax,[edi+8]
		setnz	dl
		mov	[edi+8],eax
		or	dh,dl
; +3
		cmp	eax,[edi+12]
		setnz	dl
		mov	[edi+12],eax
		or	dh,dl
; +4
		cmp	eax,[edi+16]
		setnz	dl
		mov	[edi+16],eax
		or	dh,dl
; +5
		cmp	eax,[edi+20]
		setnz	dl
		mov	[edi+20],eax
		or	dh,dl
; +6
		cmp	eax,[edi+24]
		setnz	dl
		mov	[edi+24],eax
		or	dh,dl
; +7
		cmp	eax,[edi+28]
		setnz	dl
		mov	[edi+28],eax
		or	dh,dl
; +8
		cmp	eax,[edi+32]
		setnz	dl
		mov	[edi+32],eax
		or	dh,dl
; +9
		cmp	eax,[edi+36]
		setnz	dl
		mov	[edi+36],eax
		or	dh,dl
; +A
		cmp	eax,[edi+40]
		setnz	dl
		mov	[edi+40],eax
		or	dh,dl
; +B
		cmp	eax,[edi+44]
		setnz	dl
		mov	[edi+44],eax
		or	dh,dl
; +C
		cmp	eax,[edi+48]
		setnz	dl
		mov	[edi+48],eax
		or	dh,dl
; +D
		cmp	eax,[edi+52]
		setnz	dl
		mov	[edi+52],eax
		or	dh,dl
; +E
		cmp	eax,[edi+56]
		setnz	dl
		mov	[edi+56],eax
		or	dh,dl
; +F
		cmp	eax,[edi+60]
		setnz	dl
		mov	[edi+60],eax
		or	dh,dl
; アドレスとフラグ処理
		add	esi,64
		shr	edx,8
		add	edi,64
		or	[ebx],edx
		add	ebx,4
; 次の16dotへ
		dec	ecx
		jnz	near .loop1
; 余りの16dotを処理
.rest:
		pop	ecx
		and	ecx,15
		jz	.exit
		xor	edx,edx
; ループ2
.loop2:
		cmp	eax,[edi]
		setnz	dl
		mov	[edi],eax
		or	dh,dl
		add	esi,4
		add	edi,4
		dec	ecx
		jnz	.loop2
; 格納
		shr	edx,8
		mov	[ebx],edx
; 終了
.exit:
		pop	edi
		pop	esi
		pop	edx
		pop	ecx
		pop	ebx
		pop	eax
		pop	ebp
		ret

;
; レンダラ
; 合成(1面)
;
; void RendMix01(DWORD *buf, DWORD *src, BOOL *flag, int len)
;
_RendMix01:
		push	ebp
		mov	ebp,esp
		push	eax
		push	ebx
		push	ecx
		push	edx
		push	esi
		push	edi
; パラメータ受け取り
		mov	edi,[ebp+8]
		mov	esi,[ebp+12]
		mov	ebx,[ebp+16]
		mov	ecx,[ebp+20]
; 16dot単位で処理
		push	ecx
		shr	ecx,4
		jz	near .rest
; 16dotループ
.loop1:
		xor	edx,edx
; +0
		mov	eax,[esi]
		cmp	eax,[edi]
		setnz	dl
		mov	[edi],eax
		or	dh,dl
; +1
		mov	eax,[esi+4]
		cmp	eax,[edi+4]
		setnz	dl
		mov	[edi+4],eax
		or	dh,dl
; +2
		mov	eax,[esi+8]
		cmp	eax,[edi+8]
		setnz	dl
		mov	[edi+8],eax
		or	dh,dl
; +3
		mov	eax,[esi+12]
		cmp	eax,[edi+12]
		setnz	dl
		mov	[edi+12],eax
		or	dh,dl
; +4
		mov	eax,[esi+16]
		cmp	eax,[edi+16]
		setnz	dl
		mov	[edi+16],eax
		or	dh,dl
; +5
		mov	eax,[esi+20]
		cmp	eax,[edi+20]
		setnz	dl
		mov	[edi+20],eax
		or	dh,dl
; +6
		mov	eax,[esi+24]
		cmp	eax,[edi+24]
		setnz	dl
		mov	[edi+24],eax
		or	dh,dl
; +7
		mov	eax,[esi+28]
		cmp	eax,[edi+28]
		setnz	dl
		mov	[edi+28],eax
		or	dh,dl
; +8
		mov	eax,[esi+32]
		cmp	eax,[edi+32]
		setnz	dl
		mov	[edi+32],eax
		or	dh,dl
; +9
		mov	eax,[esi+36]
		cmp	eax,[edi+36]
		setnz	dl
		mov	[edi+36],eax
		or	dh,dl
; +A
		mov	eax,[esi+40]
		cmp	eax,[edi+40]
		setnz	dl
		mov	[edi+40],eax
		or	dh,dl
; +B
		mov	eax,[esi+44]
		cmp	eax,[edi+44]
		setnz	dl
		mov	[edi+44],eax
		or	dh,dl
; +C
		mov	eax,[esi+48]
		cmp	eax,[edi+48]
		setnz	dl
		mov	[edi+48],eax
		or	dh,dl
; +D
		mov	eax,[esi+52]
		cmp	eax,[edi+52]
		setnz	dl
		mov	[edi+52],eax
		or	dh,dl
; +E
		mov	eax,[esi+56]
		cmp	eax,[edi+56]
		setnz	dl
		mov	[edi+56],eax
		or	dh,dl
; +F
		mov	eax,[esi+60]
		cmp	eax,[edi+60]
		setnz	dl
		mov	[edi+60],eax
		or	dh,dl
; アドレスとフラグ処理
		add	esi,64
		shr	edx,8
		add	edi,64
		or	[ebx],edx
		add	ebx,4
; 次の16dotへ
		dec	ecx
		jnz	near .loop1
; 余りの16dotを処理
.rest:
		pop	ecx
		and	ecx,15
		jz	.exit
		xor	edx,edx
; ループ2
.loop2:
		mov	eax,[esi]
		cmp	eax,[edi]
		setnz	dl
		mov	[edi],eax
		or	dh,dl
		add	esi,4
		add	edi,4
		dec	ecx
		jnz	.loop2
; 格納
		shr	edx,8
		mov	[ebx],edx
; 終了
.exit:
		pop	edi
		pop	esi
		pop	edx
		pop	ecx
		pop	ebx
		pop	eax
		pop	ebp
		ret

;
; レンダラ
; 合成(2面、カラー0重ね合わせ)
;
; void RendMix02(DWORD *buf, DWORD *f, DWORD *s, BOOL *flag, int len)
;
_RendMix02:
		push	ebp
		mov	ebp,esp
		push	eax
		push	ebx
		push	ecx
		push	edx
		push	esi
		push	edi
; パラメータ受け取り
		mov	edi,[ebp+8]
		mov	esi,[ebp+12]
		mov	ebx,[ebp+20]
		mov	ecx,[ebp+24]
		mov	ebp,[ebp+16]
; 分割
		push	ecx
		shr	ecx,4
		jz	near .rest
; 16dot処理
.loop1:
		push	ecx
		xor	edx,edx
		mov	ecx,2
; +0
.loop2:
		mov	eax,[esi]
		bt	eax,31
		jnc	.dot0
		mov	eax,[ebp]
.dot0:
		cmp	eax,[edi]
		setnz	dl
		mov	[edi],eax
		or	dh,dl
; +1
		mov	eax,[esi+4]
		bt	eax,31
		jnc	.dot1
		mov	eax,[ebp+4]
.dot1:
		cmp	eax,[edi+4]
		setnz	dl
		mov	[edi+4],eax
		or	dh,dl
; +2
		mov	eax,[esi+8]
		bt	eax,31
		jnc	.dot2
		mov	eax,[ebp+8]
.dot2:
		cmp	eax,[edi+8]
		setnz	dl
		mov	[edi+8],eax
		or	dh,dl
; +3
		mov	eax,[esi+12]
		bt	eax,31
		jnc	.dot3
		mov	eax,[ebp+12]
.dot3:
		cmp	eax,[edi+12]
		setnz	dl
		mov	[edi+12],eax
		or	dh,dl
; +4
		mov	eax,[esi+16]
		bt	eax,31
		jnc	.dot4
		mov	eax,[ebp+16]
.dot4:
		cmp	eax,[edi+16]
		setnz	dl
		mov	[edi+16],eax
		or	dh,dl
; +5
		mov	eax,[esi+20]
		bt	eax,31
		jnc	.dot5
		mov	eax,[ebp+20]
.dot5:
		cmp	eax,[edi+20]
		setnz	dl
		mov	[edi+20],eax
		or	dh,dl
; +6
		mov	eax,[esi+24]
		bt	eax,31
		jnc	.dot6
		mov	eax,[ebp+24]
.dot6:
		cmp	eax,[edi+24]
		setnz	dl
		mov	[edi+24],eax
		or	dh,dl
; +7
		mov	eax,[esi+28]
		bt	eax,31
		jnc	.dot7
		mov	eax,[ebp+28]
.dot7:
		cmp	eax,[edi+28]
		setnz	dl
		mov	[edi+28],eax
		or	dh,dl
; もう１回
		add	esi,32
		add	ebp,32
		add	edi,32
		dec	ecx
		jnz	near .loop2
; この16dotは終了
		shr	edx,8
		mov	[ebx],edx
		add	ebx,4
		pop	ecx
		dec	ecx
		jnz	near .loop1
; 余り
.rest:
		pop	ecx
		and	ecx,15
		jz	.exit
		xor	edx,edx
; ループ
.loop3:
		mov	eax,[esi]
		bt	eax,31
		jnc	.dotr
		mov	eax,[ebp]
.dotr:
		cmp	eax,[edi]
		setnz	dl
		mov	[edi],eax
		or	dh,dl
; 次へ
		add	esi,4
		add	edi,4
		add	ebp,4
		dec	ecx
		jnz	.loop3
; 格納
		shr	edx,8
		mov	[ebx],edx
; 終了
.exit:
		pop	edi
		pop	esi
		pop	edx
		pop	ecx
		pop	ebx
		pop	eax
		pop	ebp
		ret

;
; レンダラ
; 合成(2面、カラー0重ね合わせ、CMOV)
;
; void RendMix02C(DWORD *buf, DWORD *f, DWORD *s, BOOL *flag, int len)
;
_RendMix02C:
		push	ebp
		mov	ebp,esp
		push	eax
		push	ebx
		push	ecx
		push	edx
		push	esi
		push	edi
; パラメータ受け取り
		mov	edi,[ebp+8]
		mov	esi,[ebp+12]
		mov	ebx,[ebp+20]
		mov	ecx,[ebp+24]
		mov	ebp,[ebp+16]
; 分割
		push	ecx
		shr	ecx,4
		jz	near .rest
; 16dot処理
.loop1:
		push	ecx
		xor	edx,edx
		mov	ecx,80000000h
; +0
		mov	eax,[esi]
		test	eax,ecx
		cmovnz	eax,[ebp]
		cmp	eax,[edi]
		setnz	dl
		mov	[edi],eax
		or	dh,dl
; +1
		mov	eax,[esi+4]
		test	eax,ecx
		cmovnz	eax,[ebp+4]
		cmp	eax,[edi+4]
		setnz	dl
		mov	[edi+4],eax
		or	dh,dl
; +2
		mov	eax,[esi+8]
		test	eax,ecx
		cmovnz	eax,[ebp+8]
		cmp	eax,[edi+8]
		setnz	dl
		mov	[edi+8],eax
		or	dh,dl
; +3
		mov	eax,[esi+12]
		test	eax,ecx
		cmovnz	eax,[ebp+12]
		cmp	eax,[edi+12]
		setnz	dl
		mov	[edi+12],eax
		or	dh,dl
; +4
		mov	eax,[esi+16]
		test	eax,ecx
		cmovnz	eax,[ebp+16]
		cmp	eax,[edi+16]
		setnz	dl
		mov	[edi+16],eax
		or	dh,dl
; +5
		mov	eax,[esi+20]
		test	eax,ecx
		cmovnz	eax,[ebp+20]
		cmp	eax,[edi+20]
		setnz	dl
		mov	[edi+20],eax
		or	dh,dl
; +6
		mov	eax,[esi+24]
		test	eax,ecx
		cmovnz	eax,[ebp+24]
		cmp	eax,[edi+24]
		setnz	dl
		mov	[edi+24],eax
		or	dh,dl
; +7
		mov	eax,[esi+28]
		test	eax,ecx
		cmovnz	eax,[ebp+28]
		cmp	eax,[edi+28]
		setnz	dl
		mov	[edi+28],eax
		or	dh,dl
; +8
		mov	eax,[esi+32]
		test	eax,ecx
		cmovnz	eax,[ebp+32]
		cmp	eax,[edi+32]
		setnz	dl
		mov	[edi+32],eax
		or	dh,dl
; +9
		mov	eax,[esi+36]
		test	eax,ecx
		cmovnz	eax,[ebp+36]
		cmp	eax,[edi+36]
		setnz	dl
		mov	[edi+36],eax
		or	dh,dl
; +A
		mov	eax,[esi+40]
		test	eax,ecx
		cmovnz	eax,[ebp+40]
		cmp	eax,[edi+40]
		setnz	dl
		mov	[edi+40],eax
		or	dh,dl
; +B
		mov	eax,[esi+44]
		test	eax,ecx
		cmovnz	eax,[ebp+44]
		cmp	eax,[edi+44]
		setnz	dl
		mov	[edi+44],eax
		or	dh,dl
; +C
		mov	eax,[esi+48]
		test	eax,ecx
		cmovnz	eax,[ebp+48]
		cmp	eax,[edi+48]
		setnz	dl
		mov	[edi+48],eax
		or	dh,dl
; +D
		mov	eax,[esi+52]
		test	eax,ecx
		cmovnz	eax,[ebp+52]
		cmp	eax,[edi+52]
		setnz	dl
		mov	[edi+52],eax
		or	dh,dl
; +E
		mov	eax,[esi+56]
		test	eax,ecx
		cmovnz	eax,[ebp+56]
		cmp	eax,[edi+56]
		setnz	dl
		mov	[edi+56],eax
		or	dh,dl
; +F
		mov	eax,[esi+60]
		test	eax,ecx
		cmovnz	eax,[ebp+60]
		cmp	eax,[edi+60]
		setnz	dl
		mov	[edi+60],eax
		or	dh,dl
; この16dotは終了
		add	esi,64
		add	ebp,64
		add	edi,64
		shr	edx,8
		mov	[ebx],edx
		add	ebx,4
		pop	ecx
		dec	ecx
		jnz	near .loop1
; 余り
.rest:
		pop	ecx
		and	ecx,15
		jz	.exit
		xor	edx,edx
; ループ
.loop3:
		mov	eax,[esi]
		bt	eax,31
		cmovc	eax,[ebp]
		cmp	eax,[edi]
		setnz	dl
		mov	[edi],eax
		or	dh,dl
; 次へ
		add	esi,4
		add	edi,4
		add	ebp,4
		dec	ecx
		jnz	.loop3
; 格納
		shr	edx,8
		mov	[ebx],edx
; 終了
.exit:
		pop	edi
		pop	esi
		pop	edx
		pop	ecx
		pop	ebx
		pop	eax
		pop	ebp
		ret

;
; レンダラ
; 合成(2面、通常重ね合わせ)
;
; void RendMix03(DWORD *buf, DWORD *f, DWORD *s, BOOL *flag, int len)
;
_RendMix03:
		push	ebp
		mov	ebp,esp
		push	eax
		push	ebx
		push	ecx
		push	edx
		push	esi
		push	edi
; パラメータ受け取り
		mov	edi,[ebp+8]
		mov	esi,[ebp+12]
		mov	ebx,[ebp+20]
		mov	ecx,[ebp+24]
		mov	ebp,[ebp+16]
; 分割
		push	ecx
		shr	ecx,4
		jz	near .rest
; 16dot処理
.loop1:
		push	ecx
		push	ebx
		xor	edx,edx
		mov	ecx,2
		mov	ebx,00ffffffh
; +0
.loop2:
		mov	eax,[esi]
		bt	eax,31
		jc	.clr0
		test	eax,ebx
		jnz	.dot0
.clr0:
		mov	eax,[ebp]
.dot0:
		cmp	eax,[edi]
		setnz	dl
		mov	[edi],eax
		or	dh,dl
; +1
		mov	eax,[esi+4]
		bt	eax,31
		jc	.clr1
		test	eax,ebx
		jnz	.dot1
.clr1:
		mov	eax,[ebp+4]
.dot1:
		cmp	eax,[edi+4]
		setnz	dl
		mov	[edi+4],eax
		or	dh,dl
; +2
		mov	eax,[esi+8]
		bt	eax,31
		jc	.clr2
		test	eax,ebx
		jnz	.dot2
.clr2:
		mov	eax,[ebp+8]
.dot2:
		cmp	eax,[edi+8]
		setnz	dl
		mov	[edi+8],eax
		or	dh,dl
; +3
		mov	eax,[esi+12]
		bt	eax,31
		jc	.clr3
		test	eax,ebx
		jnz	.dot3
.clr3:
		mov	eax,[ebp+12]
.dot3:
		cmp	eax,[edi+12]
		setnz	dl
		mov	[edi+12],eax
		or	dh,dl
; +4
		mov	eax,[esi+16]
		bt	eax,31
		jc	.clr4
		test	eax,ebx
		jnz	.dot4
.clr4:
		mov	eax,[ebp+16]
.dot4:
		cmp	eax,[edi+16]
		setnz	dl
		mov	[edi+16],eax
		or	dh,dl
; +5
		mov	eax,[esi+20]
		bt	eax,31
		jc	.clr5
		test	eax,ebx
		jnz	.dot5
.clr5:
		mov	eax,[ebp+20]
.dot5:
		cmp	eax,[edi+20]
		setnz	dl
		mov	[edi+20],eax
		or	dh,dl
; +6
		mov	eax,[esi+24]
		bt	eax,31
		jc	.clr6
		test	eax,ebx
		jnz	.dot6
.clr6:
		mov	eax,[ebp+24]
.dot6:
		cmp	eax,[edi+24]
		setnz	dl
		mov	[edi+24],eax
		or	dh,dl
; +7
		mov	eax,[esi+28]
		bt	eax,31
		jc	.clr7
		test	eax,ebx
		jnz	.dot7
.clr7:
		mov	eax,[ebp+28]
.dot7:
		cmp	eax,[edi+28]
		setnz	dl
		mov	[edi+28],eax
		or	dh,dl
; もう１回
		add	esi,32
		add	ebp,32
		add	edi,32
		dec	ecx
		jnz	near .loop2
; この16dotは終了
		pop	ebx
		shr	edx,8
		mov	[ebx],edx
		pop	ecx
		add	ebx,4
		dec	ecx
		jnz	near .loop1
; 余り
.rest:
		pop	ecx
		and	ecx,15
		jz	.exit
		xor	edx,edx
; ループ
.loop3:
		mov	eax,[esi]
		bt	eax,31
		jc	.clrr
		test	eax,00ffffffh
		jnz	.dotr
.clrr:
		mov	eax,[ebp]
.dotr:
		cmp	eax,[edi]
		setnz	dl
		mov	[edi],eax
		or	dh,dl
; 次へ
		add	esi,4
		add	edi,4
		add	ebp,4
		dec	ecx
		jnz	.loop3
; 格納
		shr	edx,8
		mov	[ebx],edx
; 終了
.exit:
		pop	edi
		pop	esi
		pop	edx
		pop	ecx
		pop	ebx
		pop	eax
		pop	ebp
		ret

;
; レンダラ
; 合成(2面、通常重ね合わせ、CMOV)
;
; void RendMix03C(DWORD *buf, DWORD *f, DWORD *s, BOOL *flag, int len)
;
_RendMix03C:
		push	ebp
		mov	ebp,esp
		push	eax
		push	ebx
		push	ecx
		push	edx
		push	esi
		push	edi
; パラメータ受け取り
		mov	edi,[ebp+8]
		mov	esi,[ebp+12]
		mov	ebx,[ebp+20]
		mov	ecx,[ebp+24]
		mov	ebp,[ebp+16]
; 分割
		push	ecx
		shr	ecx,4
		jz	near .rest
; 16dot処理
.loop1:
		push	ecx
		push	ebx
		xor	edx,edx
		mov	ebx,00ffffffh
		mov	ecx,2
.loop2:
; +0
		mov	eax,[esi]
		test	eax,ebx
		cmovz	eax,[ebp]
		bt	eax,31
		cmovc	eax,[ebp]
		cmp	eax,[edi]
		setnz	dl
		mov	[edi],eax
		or	dh,dl
; +1
		mov	eax,[esi+4]
		test	eax,ebx
		cmovz	eax,[ebp+4]
		bt	eax,31
		cmovc	eax,[ebp+4]
		cmp	eax,[edi+4]
		setnz	dl
		mov	[edi+4],eax
		or	dh,dl
; +2
		mov	eax,[esi+8]
		test	eax,ebx
		cmovz	eax,[ebp+8]
		bt	eax,31
		cmovc	eax,[ebp+8]
		cmp	eax,[edi+8]
		setnz	dl
		mov	[edi+8],eax
		or	dh,dl
; +3
		mov	eax,[esi+12]
		test	eax,ebx
		cmovz	eax,[ebp+12]
		bt	eax,31
		cmovc	eax,[ebp+12]
		cmp	eax,[edi+12]
		setnz	dl
		mov	[edi+12],eax
		or	dh,dl
; +4
		mov	eax,[esi+16]
		test	eax,ebx
		cmovz	eax,[ebp+16]
		bt	eax,31
		cmovc	eax,[ebp+16]
		cmp	eax,[edi+16]
		setnz	dl
		mov	[edi+16],eax
		or	dh,dl
; +5
		mov	eax,[esi+20]
		test	eax,ebx
		cmovz	eax,[ebp+20]
		bt	eax,31
		cmovc	eax,[ebp+20]
		cmp	eax,[edi+20]
		setnz	dl
		mov	[edi+20],eax
		or	dh,dl
; +6
		mov	eax,[esi+24]
		test	eax,ebx
		cmovz	eax,[ebp+24]
		bt	eax,31
		cmovc	eax,[ebp+24]
		cmp	eax,[edi+24]
		setnz	dl
		mov	[edi+24],eax
		or	dh,dl
; +7
		mov	eax,[esi+28]
		test	eax,ebx
		cmovz	eax,[ebp+28]
		bt	eax,31
		cmovc	eax,[ebp+28]
		cmp	eax,[edi+28]
		setnz	dl
		mov	[edi+28],eax
		or	dh,dl
; もう１回
		add	esi,32
		add	ebp,32
		add	edi,32
		dec	ecx
		jnz	near .loop2
; この16dotは終了
		pop	ebx
		shr	edx,8
		mov	[ebx],edx
		pop	ecx
		add	ebx,4
		dec	ecx
		jnz	near .loop1
; 余り
.rest:
		pop	ecx
		and	ecx,15
		jz	.exit
		xor	edx,edx
		push	ebx
		mov	ebx,00ffffffh
; ループ
.loop3:
		mov	eax,[esi]
		test	eax,ebx
		cmovz	eax,[ebp]
		bt	eax,31
		cmovc	eax,[ebp]
		cmp	eax,[edi]
		setnz	dl
		mov	[edi],eax
		or	dh,dl
; 次へ
		add	esi,4
		add	edi,4
		add	ebp,4
		dec	ecx
		jnz	.loop3
; 格納
		pop	ebx
		shr	edx,8
		mov	[ebx],edx
; 終了
.exit:
		pop	edi
		pop	esi
		pop	edx
		pop	ecx
		pop	ebx
		pop	eax
		pop	ebp
		ret

;
; レンダラ
; 合成(3面、通常重ね合わせ)
;
; void RendMix04(DWORD *buf, DWORD *f, DWORD *s, DWORD *t, BOOL *flag, int len)
;
_RendMix04:
		push	ebp
		mov	ebp,esp
		push	eax
		push	ebx
		push	ecx
		push	edx
		push	esi
		push	edi
; パラメータ受け取り
		mov	edi,[ebp+8]
		mov	esi,[ebp+12]
		mov	edx,[ebp+20]
		mov	ebx,[ebp+24]
		mov	ecx,[ebp+28]
		mov	ebp,[ebp+16]
; 分割
		push	ecx
		shr	ecx,4
		jz	near .rest
; 16dot処理
.loop1:
		push	ecx
		push	ebx
		xor	ebx,ebx
		mov	ecx,2
.loop2:
		push	ecx
		mov	ecx,00ffffffh
; +0
		mov	eax,[esi]
		bt	eax,31
		jc	.dot01
		test	eax,ecx
		jnz	.dot03
.dot01:
		mov	eax,[ebp]
		bt	eax,31
		jc	.dot02
		test	eax,ecx
		jnz	.dot03
.dot02:
		mov	eax,[edx]
.dot03:
		cmp	eax,[edi]
		setnz	bl
		mov	[edi],eax
		or	bh,bl
; +1
		mov	eax,[esi+4]
		bt	eax,31
		jc	.dot11
		test	eax,ecx
		jnz	.dot13
.dot11:
		mov	eax,[ebp+4]
		bt	eax,31
		jc	.dot12
		test	eax,ecx
		jnz	.dot13
.dot12:
		mov	eax,[edx+4]
.dot13:
		cmp	eax,[edi+4]
		setnz	bl
		mov	[edi+4],eax
		or	bh,bl
; +2
		mov	eax,[esi+8]
		bt	eax,31
		jc	.dot21
		test	eax,ecx
		jnz	.dot23
.dot21:
		mov	eax,[ebp+8]
		bt	eax,31
		jc	.dot22
		test	eax,ecx
		jnz	.dot23
.dot22:
		mov	eax,[edx+8]
.dot23:
		cmp	eax,[edi+8]
		setnz	bl
		mov	[edi+8],eax
		or	bh,bl
; +3
		mov	eax,[esi+12]
		bt	eax,31
		jc	.dot31
		test	eax,ecx
		jnz	.dot33
.dot31:
		mov	eax,[ebp+12]
		bt	eax,31
		jc	.dot32
		test	eax,ecx
		jnz	.dot33
.dot32:
		mov	eax,[edx+12]
.dot33:
		cmp	eax,[edi+12]
		setnz	bl
		mov	[edi+12],eax
		or	bh,bl
; +4
		mov	eax,[esi+16]
		bt	eax,31
		jc	.dot41
		test	eax,ecx
		jnz	.dot43
.dot41:
		mov	eax,[ebp+16]
		bt	eax,31
		jc	.dot42
		test	eax,ecx
		jnz	.dot43
.dot42:
		mov	eax,[edx+16]
.dot43:
		cmp	eax,[edi+16]
		setnz	bl
		mov	[edi+16],eax
		or	bh,bl
; +5
		mov	eax,[esi+20]
		bt	eax,31
		jc	.dot51
		test	eax,ecx
		jnz	.dot53
.dot51:
		mov	eax,[ebp+20]
		bt	eax,31
		jc	.dot52
		test	eax,ecx
		jnz	.dot53
.dot52:
		mov	eax,[edx+20]
.dot53:
		cmp	eax,[edi+20]
		setnz	bl
		mov	[edi+20],eax
		or	bh,bl
; +6
		mov	eax,[esi+24]
		bt	eax,31
		jc	.dot61
		test	eax,ecx
		jnz	.dot63
.dot61:
		mov	eax,[ebp+24]
		bt	eax,31
		jc	.dot62
		test	eax,ecx
		jnz	.dot63
.dot62:
		mov	eax,[edx+24]
.dot63:
		cmp	eax,[edi+24]
		setnz	bl
		mov	[edi+24],eax
		or	bh,bl
; +7
		mov	eax,[esi+28]
		bt	eax,31
		jc	.dot71
		test	eax,ecx
		jnz	.dot73
.dot71:
		mov	eax,[ebp+28]
		bt	eax,31
		jc	.dot72
		test	eax,ecx
		jnz	.dot73
.dot72
		mov	eax,[edx+28]
.dot73:
		cmp	eax,[edi+28]
		setnz	bl
		mov	[edi+28],eax
		or	bh,bl
; もう１回
		add	esi,32
		add	ebp,32
		pop	ecx
		add	edi,32
		add	edx,32
		dec	ecx
		jnz	near .loop2
; この16dotは終了
		shr	ebx,8
		mov	eax,ebx
		pop	ebx
		mov	[ebx],eax
		pop	ecx
		add	ebx,4
		dec	ecx
		jnz	near .loop1
; 余り
.rest:
		pop	ecx
		and	ecx,15
		jz	.exit
		push	ebx
		xor	ebx,ebx
; ループ
.loop3:
		mov	eax,[esi]
		bt	eax,31
		jc	.next1
		test	eax,00ffffffh
		jnz	.next3
.next1:
		mov	eax,[ebp]
		bt	eax,31
		jc	.next2
		test	eax,00ffffffh
		jnz	.next3
.next2:
		mov	eax,[edx]
.next3:
		cmp	eax,[edi]
		setnz	bl
		mov	[edi],eax
		or	bh,bl
; 次へ
		add	esi,4
		add	edi,4
		add	ebp,4
		add	edx,4
		dec	ecx
		jnz	.loop3
; 格納
		shr	ebx,8
		mov	eax,ebx
		pop	ebx
		mov	[ebx],eax
; 終了
.exit:
		pop	edi
		pop	esi
		pop	edx
		pop	ecx
		pop	ebx
		pop	eax
		pop	ebp
		ret

;
; レンダラ
; 合成(3面、通常重ね合わせ、CMOV)
;
; void RendMix04C(DWORD *buf, DWORD *f, DWORD *s, DWORD *t, BOOL *flag, int len)
;
_RendMix04C:
		push	ebp
		mov	ebp,esp
		push	eax
		push	ebx
		push	ecx
		push	edx
		push	esi
		push	edi
; パラメータ受け取り
		mov	edi,[ebp+8]
		mov	esi,[ebp+12]
		mov	edx,[ebp+20]
		mov	ebx,[ebp+24]
		mov	ecx,[ebp+28]
		mov	ebp,[ebp+16]
; 分割
		push	ecx
		shr	ecx,4
		jz	near .rest
; 16dot処理
.loop1:
		push	ecx
		push	ebx
		xor	ebx,ebx
		mov	ecx,2
.loop2:
		push	ecx
		mov	ecx,00ffffffh
; +0
		mov	eax,[esi]
		test	eax,ecx
		cmovz	eax,[ebp]
		bt	eax,31
		cmovc	eax,[ebp]
		test	eax,ecx
		cmovz	eax,[edx]
		bt	eax,31
		cmovc	eax,[edx]
		cmp	eax,[edi]
		setnz	bl
		mov	[edi],eax
		or	bh,bl
; +1
		mov	eax,[esi+4]
		test	eax,ecx
		cmovz	eax,[ebp+4]
		bt	eax,31
		cmovc	eax,[ebp+4]
		test	eax,ecx
		cmovz	eax,[edx+4]
		bt	eax,31
		cmovc	eax,[edx+4]
		cmp	eax,[edi+4]
		setnz	bl
		mov	[edi+4],eax
		or	bh,bl
; +2
		mov	eax,[esi+8]
		test	eax,ecx
		cmovz	eax,[ebp+8]
		bt	eax,31
		cmovc	eax,[ebp+8]
		test	eax,ecx
		cmovz	eax,[edx+8]
		bt	eax,31
		cmovc	eax,[edx+8]
		cmp	eax,[edi+8]
		setnz	bl
		mov	[edi+8],eax
		or	bh,bl
; +3
		mov	eax,[esi+12]
		test	eax,ecx
		cmovz	eax,[ebp+12]
		bt	eax,31
		cmovc	eax,[ebp+12]
		test	eax,ecx
		cmovz	eax,[edx+12]
		bt	eax,31
		cmovc	eax,[edx+12]
		cmp	eax,[edi+12]
		setnz	bl
		mov	[edi+12],eax
		or	bh,bl
; +4
		mov	eax,[esi+16]
		test	eax,ecx
		cmovz	eax,[ebp+16]
		bt	eax,31
		cmovc	eax,[ebp+16]
		test	eax,ecx
		cmovz	eax,[edx+16]
		bt	eax,31
		cmovc	eax,[edx+16]
		cmp	eax,[edi+16]
		setnz	bl
		mov	[edi+16],eax
		or	bh,bl
; +5
		mov	eax,[esi+20]
		test	eax,ecx
		cmovz	eax,[ebp+20]
		bt	eax,31
		cmovc	eax,[ebp+20]
		test	eax,ecx
		cmovz	eax,[edx+20]
		bt	eax,31
		cmovc	eax,[edx+20]
		cmp	eax,[edi+20]
		setnz	bl
		mov	[edi+20],eax
		or	bh,bl
; +6
		mov	eax,[esi+24]
		test	eax,ecx
		cmovz	eax,[ebp+24]
		bt	eax,31
		cmovc	eax,[ebp+24]
		test	eax,ecx
		cmovz	eax,[edx+24]
		bt	eax,31
		cmovc	eax,[edx+24]
		cmp	eax,[edi+24]
		setnz	bl
		mov	[edi+24],eax
		or	bh,bl
; +7
		mov	eax,[esi+28]
		test	eax,ecx
		cmovz	eax,[ebp+28]
		bt	eax,31
		cmovc	eax,[ebp+28]
		test	eax,ecx
		cmovz	eax,[edx+28]
		bt	eax,31
		cmovc	eax,[edx+28]
		cmp	eax,[edi+28]
		setnz	bl
		mov	[edi+28],eax
		or	bh,bl
; もう１回
		add	esi,32
		add	ebp,32
		pop	ecx
		add	edi,32
		add	edx,32
		dec	ecx
		jnz	near .loop2
; この16dotは終了
		shr	ebx,8
		mov	eax,ebx
		pop	ebx
		mov	[ebx],eax
		pop	ecx
		add	ebx,4
		dec	ecx
		jnz	near .loop1
; 余り
.rest:
		pop	ecx
		and	ecx,15
		jz	.exit
		push	ebx
		xor	ebx,ebx
; ループ
.loop3:
		mov	eax,[esi]
		test	eax,00ffffffh
		cmovz	eax,[ebp]
		bt	eax,31
		cmovc	eax,[ebp]
		test	eax,00ffffffh
		cmovz	eax,[edx]
		bt	eax,31
		cmovc	eax,[edx]
		cmp	eax,[edi]
		setnz	bl
		mov	[edi],eax
		or	bh,bl
; 次へ
		add	esi,4
		add	edi,4
		add	ebp,4
		add	edx,4
		dec	ecx
		jnz	.loop3
; 格納
		shr	ebx,8
		mov	eax,ebx
		pop	ebx
		mov	[ebx],eax
; 終了
.exit:
		pop	edi
		pop	esi
		pop	edx
		pop	ecx
		pop	ebx
		pop	eax
		pop	ebp
		ret

;
; レンダラ
; 合成(グラフィック2面、カラー0重ね合わせ)
;
; void RendGrp02(DWORD *buf, DWORD *f, DWORD *s, int len)
;
_RendGrp02:
		push	ebp
		mov	ebp,esp
		push	eax
		push	ebx
		push	ecx
		push	edx
		push	esi
		push	edi
; パラメータ受け取り
		mov	edi,[ebp+8]
		mov	esi,[ebp+12]
		mov	ecx,[ebp+20]
		mov	ebp,[ebp+16]
; 分割
		push	ecx
		shr	ecx,3
		jz	near .rest
; 8ot処理
.loop1:
; +0
		mov	eax,[esi]
		bt	eax,31
		jnc	.dot0
		mov	eax,[ebp]
.dot0:
		mov	[edi],eax
; +1
		mov	eax,[esi+4]
		bt	eax,31
		jnc	.dot1
		mov	eax,[ebp+4]
.dot1:
		mov	[edi+4],eax
; +2
		mov	eax,[esi+8]
		bt	eax,31
		jnc	.dot2
		mov	eax,[ebp+8]
.dot2:
		mov	[edi+8],eax
; +3
		mov	eax,[esi+12]
		bt	eax,31
		jnc	.dot3
		mov	eax,[ebp+12]
.dot3:
		mov	[edi+12],eax
; +4
		mov	eax,[esi+16]
		bt	eax,31
		jnc	.dot4
		mov	eax,[ebp+16]
.dot4:
		mov	[edi+16],eax
; +5
		mov	eax,[esi+20]
		bt	eax,31
		jnc	.dot5
		mov	eax,[ebp+20]
.dot5:
		mov	[edi+20],eax
; +6
		mov	eax,[esi+24]
		bt	eax,31
		jnc	.dot6
		mov	eax,[ebp+24]
.dot6:
		mov	[edi+24],eax
; +7
		mov	eax,[esi+28]
		bt	eax,31
		jnc	.dot7
		mov	eax,[ebp+28]
.dot7:
		mov	[edi+28],eax
; 次へ
		add	esi,32
		add	ebp,32
		add	edi,32
		dec	ecx
		jnz	near .loop1
; 余り
.rest:
		pop	ecx
		and	ecx,7
		jz	.exit
; ループ
.loop3:
		mov	eax,[esi]
		bt	eax,31
		jnc	.dotr
		mov	eax,[ebp]
.dotr:
		mov	[edi],eax
; 次へ
		add	esi,4
		add	edi,4
		add	ebp,4
		dec	ecx
		jnz	.loop3
; 終了
.exit:
		pop	edi
		pop	esi
		pop	edx
		pop	ecx
		pop	ebx
		pop	eax
		pop	ebp
		ret

;
; レンダラ
; 合成(グラフィック2面、カラー0重ね合わせ、CMOV)
;
; void RendGrp02C(DWORD *buf, DWORD *f, DWORD *s, int len)
;
_RendGrp02C:
		push	ebp
		mov	ebp,esp
		push	eax
		push	ebx
		push	ecx
		push	edx
		push	esi
		push	edi
; パラメータ受け取り
		mov	edi,[ebp+8]
		mov	esi,[ebp+12]
		mov	ecx,[ebp+20]
		mov	ebp,[ebp+16]
; 分割
		mov	edx,80000000h
		push	ecx
		shr	ecx,4
		jz	near .rest
; 16dot処理
.loop1:
; +0,+1
		mov	eax,[esi]
		test	eax,edx
		mov	ebx,[esi+4]
		cmovnz	eax,[ebp]
		test	ebx,edx
		cmovnz	ebx,[ebp+4]
		mov	[edi],eax
		mov	[edi+4],ebx
; +2,+3
		mov	eax,[esi+8]
		test	eax,edx
		mov	ebx,[esi+12]
		cmovnz	eax,[ebp+8]
		test	ebx,edx
		cmovnz	ebx,[ebp+12]
		mov	[edi+8],eax
		mov	[edi+12],ebx
; +4,+5
		mov	eax,[esi+16]
		test	eax,edx
		mov	ebx,[esi+20]
		cmovnz	eax,[ebp+16]
		test	ebx,edx
		cmovnz	ebx,[ebp+20]
		mov	[edi+16],eax
		mov	[edi+20],ebx
; +6,+7
		mov	eax,[esi+24]
		test	eax,edx
		mov	ebx,[esi+28]
		cmovnz	eax,[ebp+24]
		test	ebx,edx
		cmovnz	ebx,[ebp+28]
		mov	[edi+24],eax
		mov	[edi+28],ebx
; +8,+9
		mov	eax,[esi+32]
		test	eax,edx
		mov	ebx,[esi+36]
		cmovnz	eax,[ebp+32]
		test	ebx,edx
		cmovnz	ebx,[ebp+36]
		mov	[edi+32],eax
		mov	[edi+36],ebx
; +A,+B
		mov	eax,[esi+40]
		test	eax,edx
		mov	ebx,[esi+44]
		cmovnz	eax,[ebp+40]
		test	ebx,edx
		cmovnz	ebx,[ebp+44]
		mov	[edi+40],eax
		mov	[edi+44],ebx
; +C,+D
		mov	eax,[esi+48]
		test	eax,edx
		mov	ebx,[esi+52]
		cmovnz	eax,[ebp+48]
		test	ebx,edx
		cmovnz	ebx,[ebp+52]
		mov	[edi+48],eax
		mov	[edi+52],ebx
; +E,+F
		mov	eax,[esi+56]
		test	eax,edx
		mov	ebx,[esi+60]
		cmovnz	eax,[ebp+56]
		test	ebx,edx
		cmovnz	ebx,[ebp+60]
		mov	[edi+56],eax
		mov	[edi+60],ebx
; 次へ
		add	esi,64
		add	ebp,64
		add	edi,64
		dec	ecx
		jnz	near .loop1
; 余り
.rest:
		pop	ecx
		and	ecx,15
		jz	.exit
; ループ
.loop3:
		mov	eax,[esi]
		test	eax,edx
		cmovnz	eax,[ebp]
		mov	[edi],eax
; 次へ
		add	esi,4
		add	edi,4
		add	ebp,4
		dec	ecx
		jnz	.loop3
; 終了
.exit:
		pop	edi
		pop	esi
		pop	edx
		pop	ecx
		pop	ebx
		pop	eax
		pop	ebp
		ret

;
; レンダラ
; 合成(グラフィック3面、カラー0重ね合わせ)
;
; void RendGrp03(DWORD *buf, DWORD *f, DWORD *s, DWORD *t, int len)
;
_RendGrp03:
		push	ebp
		mov	ebp,esp
		push	eax
		push	ebx
		push	ecx
		push	edx
		push	esi
		push	edi
; パラメータ受け取り
		mov	edi,[ebp+8]
		mov	esi,[ebp+12]
		mov	ecx,[ebp+24]
		mov	ebx,[ebp+20]
		mov	ebp,[ebp+16]
; 分割
		push	ecx
		shr	ecx,3
		jz	near .rest
; 8ot処理
.loop1:
; +0
		mov	eax,[esi]
		bt	eax,31
		jnc	.dot0
		mov	eax,[ebp]
		bt	eax,31
		jnc	.dot0
		mov	eax,[ebx]
.dot0:
		mov	[edi],eax
; +1
		mov	eax,[esi+4]
		bt	eax,31
		jnc	.dot1
		mov	eax,[ebp+4]
		bt	eax,31
		jnc	.dot1
		mov	eax,[ebx+4]
.dot1:
		mov	[edi+4],eax
; +2
		mov	eax,[esi+8]
		bt	eax,31
		jnc	.dot2
		mov	eax,[ebp+8]
		bt	eax,31
		jnc	.dot2
		mov	eax,[ebx+8]
.dot2:
		mov	[edi+8],eax
; +3
		mov	eax,[esi+12]
		bt	eax,31
		jnc	.dot3
		mov	eax,[ebp+12]
		bt	eax,31
		jnc	.dot3
		mov	eax,[ebx+12]
.dot3:
		mov	[edi+12],eax
; +4
		mov	eax,[esi+16]
		bt	eax,31
		jnc	.dot4
		mov	eax,[ebp+16]
		bt	eax,31
		jnc	.dot4
		mov	eax,[ebx+16]
.dot4:
		mov	[edi+16],eax
; +5
		mov	eax,[esi+20]
		bt	eax,31
		jnc	.dot5
		mov	eax,[ebp+20]
		bt	eax,31
		jnc	.dot5
		mov	eax,[ebx+20]
.dot5:
		mov	[edi+20],eax
; +6
		mov	eax,[esi+24]
		bt	eax,31
		jnc	.dot6
		mov	eax,[ebp+24]
		bt	eax,31
		jnc	.dot6
		mov	eax,[ebx+24]
.dot6:
		mov	[edi+24],eax
; +7
		mov	eax,[esi+28]
		bt	eax,31
		jnc	.dot7
		mov	eax,[ebp+28]
		bt	eax,31
		jnc	.dot7
		mov	eax,[ebx+28]
.dot7:
		mov	[edi+28],eax
; 次へ
		add	esi,32
		add	ebp,32
		add	ebx,32
		add	edi,32
		dec	ecx
		jnz	near .loop1
; 余り
.rest:
		pop	ecx
		and	ecx,7
		jz	.exit
; ループ
.loop3:
		mov	eax,[esi]
		bt	eax,31
		jnc	.dotr
		mov	eax,[ebp]
		bt	eax,31
		jnc	.dotr
		mov	eax,[ebx]
.dotr:
		mov	[edi],eax
; 次へ
		add	esi,4
		add	edi,4
		add	ebp,4
		add	ebx,4
		dec	ecx
		jnz	.loop3
; 終了
.exit:
		pop	edi
		pop	esi
		pop	edx
		pop	ecx
		pop	ebx
		pop	eax
		pop	ebp
		ret

;
; レンダラ
; 合成(グラフィック3面、カラー0重ね合わせ、CMOV)
;
; void RendGrp03C(DWORD *buf, DWORD *f, DWORD *s, DWORD *t, int len)
;
_RendGrp03C:
		push	ebp
		mov	ebp,esp
		push	eax
		push	ebx
		push	ecx
		push	edx
		push	esi
		push	edi
; パラメータ受け取り
		mov	edi,[ebp+8]
		mov	esi,[ebp+12]
		mov	ebx,[ebp+16]
		mov	ecx,[ebp+24]
		mov	ebp,[ebp+20]
; 分割
		mov	edx,80000000h
		push	ecx
		shr	ecx,3
		jz	near .rest
; 8dot処理
.loop1:
; +0
		mov	eax,[esi]
		test	eax,edx
		cmovnz	eax,[ebp]
		test	eax,edx
		cmovnz	eax,[ebx]
		mov	[edi],eax
; +1
		mov	eax,[esi+4]
		test	eax,edx
		cmovnz	eax,[ebp+4]
		test	eax,edx
		cmovnz	eax,[ebx+4]
		mov	[edi+4],eax
; +2
		mov	eax,[esi+8]
		test	eax,edx
		cmovnz	eax,[ebp+8]
		test	eax,edx
		cmovnz	eax,[ebx+8]
		mov	[edi+8],eax
; +3
		mov	eax,[esi+12]
		test	eax,edx
		cmovnz	eax,[ebp+12]
		test	eax,edx
		cmovnz	eax,[ebx+12]
		mov	[edi+12],eax
; +4
		mov	eax,[esi+16]
		test	eax,edx
		cmovnz	eax,[ebp+16]
		test	eax,edx
		cmovnz	eax,[ebx+16]
		mov	[edi+16],eax
; +5
		mov	eax,[esi+20]
		test	eax,edx
		cmovnz	eax,[ebp+20]
		test	eax,edx
		cmovnz	eax,[ebx+20]
		mov	[edi+20],eax
; +6
		mov	eax,[esi+24]
		test	eax,edx
		cmovnz	eax,[ebp+24]
		test	eax,edx
		cmovnz	eax,[ebx+24]
		mov	[edi+24],eax
; +7
		mov	eax,[esi+28]
		test	eax,edx
		cmovnz	eax,[ebp+28]
		test	eax,edx
		cmovnz	eax,[ebx+28]
		mov	[edi+28],eax
; 次へ
		add	esi,32
		add	ebp,32
		add	edi,32
		add	ebx,32
		dec	ecx
		jnz	near .loop1
; 余り
.rest:
		pop	ecx
		and	ecx,7
		jz	.exit
; ループ
.loop3:
		mov	eax,[esi]
		test	eax,edx
		cmovnz	eax,[ebp]
		test	eax,edx
		cmovnz	eax,[ebx]
		mov	[edi],eax
; 次へ
		add	esi,4
		add	edi,4
		add	ebp,4
		add	ebx,4
		dec	ecx
		jnz	.loop3
; 終了
.exit:
		pop	edi
		pop	esi
		pop	edx
		pop	ecx
		pop	ebx
		pop	eax
		pop	ebp
		ret

;
; レンダラ
; 合成(グラフィック4面、カラー0重ね合わせ)
;
; void RendGrp04(DWORD *buf, DWORD *f, DWORD *s, DWORD *t, DWORD *e, int len)
;
_RendGrp04:
		push	ebp
		mov	ebp,esp
		push	eax
		push	ebx
		push	ecx
		push	edx
		push	esi
		push	edi
; パラメータ受け取り
		mov	edi,[ebp+8]
		mov	esi,[ebp+12]
		mov	ebx,[ebp+20]
		mov	edx,[ebp+24]
		mov	ecx,[ebp+28]
		mov	ebp,[ebp+16]
; 分割
		push	ecx
		shr	ecx,3
		jz	near .rest
; 8ot処理
.loop1:
; +0
		mov	eax,[esi]
		bt	eax,31
		jnc	.dot0
		mov	eax,[ebp]
		bt	eax,31
		jnc	.dot0
		mov	eax,[ebx]
		bt	eax,31
		jnc	.dot0
		mov	eax,[edx]
.dot0:
		mov	[edi],eax
; +1
		mov	eax,[esi+4]
		bt	eax,31
		jnc	.dot1
		mov	eax,[ebp+4]
		bt	eax,31
		jnc	.dot1
		mov	eax,[ebx+4]
		bt	eax,31
		jnc	.dot1
		mov	eax,[edx+4]
.dot1:
		mov	[edi+4],eax
; +2
		mov	eax,[esi+8]
		bt	eax,31
		jnc	.dot2
		mov	eax,[ebp+8]
		bt	eax,31
		jnc	.dot2
		mov	eax,[ebx+8]
		bt	eax,31
		jnc	.dot2
		mov	eax,[edx+8]
.dot2:
		mov	[edi+8],eax
; +3
		mov	eax,[esi+12]
		bt	eax,31
		jnc	.dot3
		mov	eax,[ebp+12]
		bt	eax,31
		jnc	.dot3
		mov	eax,[ebx+12]
		bt	eax,31
		jnc	.dot3
		mov	eax,[edx+12]
.dot3:
		mov	[edi+12],eax
; +4
		mov	eax,[esi+16]
		bt	eax,31
		jnc	.dot4
		mov	eax,[ebp+16]
		bt	eax,31
		jnc	.dot4
		mov	eax,[ebx+16]
		bt	eax,31
		jnc	.dot4
		mov	eax,[edx+16]
.dot4:
		mov	[edi+16],eax
; +5
		mov	eax,[esi+20]
		bt	eax,31
		jnc	.dot5
		mov	eax,[ebp+20]
		bt	eax,31
		jnc	.dot5
		mov	eax,[ebx+20]
		bt	eax,31
		jnc	.dot5
		mov	eax,[edx+20]
.dot5:
		mov	[edi+20],eax
; +6
		mov	eax,[esi+24]
		bt	eax,31
		jnc	.dot6
		mov	eax,[ebp+24]
		bt	eax,31
		jnc	.dot6
		mov	eax,[ebx+24]
		bt	eax,31
		jnc	.dot6
		mov	eax,[edx+24]
.dot6:
		mov	[edi+24],eax
; +7
		mov	eax,[esi+28]
		bt	eax,31
		jnc	.dot7
		mov	eax,[ebp+28]
		bt	eax,31
		jnc	.dot7
		mov	eax,[ebx+28]
		bt	eax,31
		jnc	.dot7
		mov	eax,[edx+28]
.dot7:
		mov	[edi+28],eax
; 次へ
		add	esi,32
		add	ebp,32
		add	ebx,32
		add	edx,32
		add	edi,32
		dec	ecx
		jnz	near .loop1
; 余り
.rest:
		pop	ecx
		and	ecx,7
		jz	.exit
; ループ
.loop3:
		mov	eax,[esi]
		bt	eax,31
		jnc	.dotr
		mov	eax,[ebp]
		bt	eax,31
		jnc	.dotr
		mov	eax,[ebx]
		bt	eax,31
		jnc	.dotr
		mov	eax,[edx]
.dotr:
		mov	[edi],eax
; 次へ
		add	esi,4
		add	edi,4
		add	ebp,4
		add	ebx,4
		add	edx,4
		dec	ecx
		jnz	.loop3
; 終了
.exit:
		pop	edi
		pop	esi
		pop	edx
		pop	ecx
		pop	ebx
		pop	eax
		pop	ebp
		ret

;
; プログラム終了
;
		end
