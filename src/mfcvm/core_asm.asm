;
; X68000 EMULATOR "XM6"
;
; Copyright (C) 2001-2005 ＰＩ．(ytanaka@ipc-tokai.or.jp)
; [ 仮想マシンコア アセンブラサブ ]
;

;
; Visual C++の外部アセンブラルーチンでは
; ebx, esi, edi, ebpを保存すること
;

;
; 外部宣言
;
		section	.text align=16
		bits	32

		global	_MemInitDecode
		global	_MemDecodeTable
		global	_ReadByteC
		global	_ReadWordC
		global	_WriteByteC
		global	_WriteWordC
		global	_ReadErrC
		global	_WriteErrC

		extern	_ReadBusErr
		extern	_WriteBusErr

		global	_NotifyEvent
		global	_GetMinEvent
		global	_SubExecEvent

		align	16

;
; メモリデコーダ
; ジャンプテーブル作成
;
; void MemInitDecode(DWORD *mem, MemDevice *list[])
;
_MemInitDecode:
		push	ebp
		mov	ebp,esp
		push	eax
		push	ebx
		push	ecx
		push	edx
		push	esi
		push	edi
; Memoryクラス記憶
		mov	eax,[ebp+8]
		mov	[Memory],eax
; デバイスを順に設定
		mov	ebx,_MemDecodeTable
		mov	ecx,384
		mov	edx,MemDecodeData
		mov	esi,[ebp+12]
; ループ
.Loop:
; デバイス番号(0:Memory 1:GVRAM .... SRAM)を取得
		mov	eax,[edx]
		add	edx,byte 4
; list[]を使って、そのインスタンスのポインタをストア
		mov	edi,[esi+eax*4]
		mov	[ebx],edi
		add	ebx,byte 4
; 次へ
		dec	ecx
		jnz	.Loop
; 終了
		pop	edi
		pop	esi
		pop	edx
		pop	ecx
		pop	ebx
		pop	eax
		pop	ebp
		ret

		align	16

;
; バイト読み込み
;
; EAX	DWORD戻り値
; ECX	this
; EDX	アドレス
;
_ReadByteC:
; ジャンプテーブルに従ってジャンプ(8KBごと)
		mov	eax,edx
		sub	eax,0C00000h
		jc	.Memory
		shr	eax,13
		mov	ecx,[_MemDecodeTable+eax*4]
		mov	eax,[ecx]
		jmp	[eax+32]
; メモリを呼ぶ
.Memory:
		mov	ecx,[Memory]
		mov	eax,[ecx]
		jmp	[eax+32]

		align	16

;
; ワード読み込み
;
; EAX	DWORD戻り値
; ECX	this
; EDX	アドレス
;
_ReadWordC:
; ジャンプテーブルに従ってジャンプ(8KBごと)
		mov	eax,edx
		sub	eax,0C00000h
		jc	.Memory
		shr	eax,13
		mov	ecx,[_MemDecodeTable+eax*4]
		mov	eax,[ecx]
		jmp	[eax+36]
; メモリを呼ぶ
.Memory:
		mov	ecx,[Memory]
		mov	eax,[ecx]
		jmp	[eax+36]

		align	16

;
; バイト書き込み
;
; EBX	データ
; ECX	this
; EDX	アドレス
;
_WriteByteC:
; ジャンプテーブルに従ってジャンプ(8KBごと)
		mov	eax,edx
		sub	eax,0C00000h
		jc	.Memory
		shr	eax,13
		mov	ecx,[_MemDecodeTable+eax*4]
		mov	eax,[ecx]
		push	ebx
		call	[eax+40]
		ret
; メモリを呼ぶ
.Memory:
		mov	ecx,[Memory]
		mov	eax,[ecx]
		push	ebx
		call	[eax+40]
		ret

		align	16

;
; ワード書き込み
;
; EBX	データ
; ECX	this
; EDX	アドレス
;
_WriteWordC:
; ジャンプテーブルに従ってジャンプ(8KBごと)
		mov	eax,edx
		sub	eax,0C00000h
		jc	.Memory
		shr	eax,13
		mov	ecx,[_MemDecodeTable+eax*4]
		mov	eax,[ecx]
		push	ebx
		call	[eax+44]
		ret
; メモリを呼ぶ
.Memory:
		mov	ecx,[Memory]
		mov	eax,[ecx]
		push	ebx
		call	[eax+44]
		ret

		align	16

;
; バスエラー読み込み
;
; EDX	アドレス
;
_ReadErrC:
		push	edx
		call	_ReadBusErr
		add	esp,4
		ret

		align	16

;
; バスエラー書き込み
;
; EDX	アドレス
; EBX	データ
;
_WriteErrC:
		push	edx
		call	_WriteBusErr
		add	esp,4
		ret

		align	16

;
; イベント変更通知
; ※最大イベント数は31まで(SubExecEventの最後でダミーリードするため)
;
; void NotifyEvent(Event *first)
;
_NotifyEvent:
		push	ebp
		mov	ebp,esp
		push	eax
		push	ecx
		push	esi
		push	edi
; パラメータ受け取りとカウンタクリア
		mov	esi,[ebp+8]
		xor	ecx,ecx
		mov	edi,EventTable
; ループ
.loop:
		or	esi,esi
		jz	.exit
; セット
		mov	[edi],esi
		inc	ecx
; 次を取得
		mov	esi,[esi+24]
		add	edi,byte 4
		jmp	.loop
; 個数を記録して
.exit:
		mov	[EventNum],ecx
; 終了
		pop	edi
		pop	esi
		pop	ecx
		pop	eax
		pop	ebp
		ret

		align	16

;
; 最小のイベントを探す
; ※CMOV使用
;
; void GetMinEvent(DWORD hus)
;
_GetMinEvent:
		push	ebp
		mov	ebp,esp
		push	esi
		push	edi
; パラメータを受け取る
		mov	eax,[ebp+8]
		mov	esi,EventTable
		mov	ecx,[EventNum]
; ループ
.loop:
		mov	edi,[esi]
		add	esi,byte 4
; GetRemain()呼び出し
		mov	edx,[edi+4]
		or	edx,edx
; ゼロならこのイベントは考慮しない
		cmovz	edx,eax
; 比較
		cmp	edx,eax
; これを最小とする
		cmovc	eax,edx
; 次へ
.next:
		dec	ecx
		jnz	.loop
;
		pop	edi
		pop	esi
		pop	ebp
		ret

		align	16

;
; イベント時間減少と実行
;
; DWORD SubExecEvent(DWORD hus)
;
_SubExecEvent:
		push	ebp
		mov	ebp,esp
		push	esi
		push	edi
; パラメータを受け取る
		mov	edi,[ebp+8]
		mov	esi,EventTable
		mov	ecx,[EventNum]
; Eventオブジェクトを取得
		mov	ebp,[esi]
; イベントループ
.loop:
; 時間未設定なら、何もしない
		mov	eax,[ebp+8]
		add	esi,byte 4
		or	eax,eax
		jz	.next
; 時間を引く。0かマイナスならイベント実行
		sub	[ebp+4],edi
		jna	.exec
; 次へ
.next:
		dec	ecx
		mov	ebp,[esi]
		jnz	.loop
; 終了
		pop	edi
		pop	esi
		pop	ebp
		ret
; 実行
.exec:
; 時間をリセット
		mov	[ebp+4],eax
; イベント実行(Device::Callback呼び出し)
		push	ecx
		mov	ecx,[ebp+16]
		mov	eax,[ecx]
		mov	edx,ebp
		call	[eax+28]
		pop	ecx
; 結果が0なら、FALSEのため無効化
		or	eax,eax
		jz	.disable
; 次のイベントへ
		dec	ecx
		mov	ebp,[esi]
		jnz	.loop
; 終了
		pop	edi
		pop	esi
		pop	ebp
		ret
; 無効化(timeおよびremainを0クリア)
.disable:
		mov	[ebp+8],eax
		mov	[ebp+4],eax
; 次のイベントへ
		dec	ecx
		mov	ebp,[esi]
		jnz	.loop
; 終了
		pop	edi
		pop	esi
		pop	ebp
		ret

;
; データエリア (8KB単位)
;
; 0	MEMORY
; 1	GVRAM
; 2	TVRAM
; 3	CRTC
; 4	VC
; 5	DMAC
; 6	AREA
; 7	MFP
; 8	RTC
; 9	PRN
; 10	SYSPORT
; 11	OPM
; 12	ADPCM
; 13	FDC
; 14	SASI
; 15	SCC
; 16	PPI
; 17	IOSC
; 18	WINDRV
; 19	SCSI
; 20	MIDI
; 21	SPR
; 22	MERCURY
; 23	NEPTUNE
; 24	SRAM
;
		section	.data align=16
		bits	32
MemDecodeData:
; $C00000 (GVRAM)
		dd	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1
		dd	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1
		dd	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1
		dd	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1
		dd	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1
		dd	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1
		dd	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1
		dd	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1
		dd	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1
		dd	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1
		dd	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1
		dd	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1
		dd	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1
		dd	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1
		dd	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1
		dd	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1
; $E00000 (TVRAM)
		dd	2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2
		dd	2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2
		dd	2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2
		dd	2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2
; $E80000 (CRTC - IOSC)
		dd	3,4,5,6,7,8,9,10,11,12,13,14,15,16,17
; $E9E000 (WINDRV)
		dd	18
; $EA0000 (SCSI)
		dd	19
; $EA2000 (RESERVE)
		dd	0,0,0,0,0,0
; $EAE000 (MIDI)
		dd	20
; $EB0000 (SPRITE)
		dd	21,21,21,21,21,21,21,21
; $EC0000 (USER)
		dd	0,0,0,0,0,0
; $ECC000 (MERCURY)
		dd	22
; $ECE000 (NEPTUNE)
		dd	23
; $ED0000 (SRAM)
		dd	24,24,24,24,24,24,24,24
; $EE0000 (RESERVE)
		dd	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0

;
; BSSエリア
;
		section	.bss align=16
		bits	32
Memory:
		resb	4
EventNum:
		resb	4
; ダミー(align 16)
		resb	8

_MemDecodeTable:
		resb	384*4
EventTable:
		resb	32*4

;
; プログラム終了
;
		end
