;
; X68000 EMULATOR "XM6"
;
; Copyright (C) 2001-2005 �o�h�D(ytanaka@ipc-tokai.or.jp)
; [ ���z�}�V���R�A �A�Z���u���T�u ]
;

;
; Visual C++�̊O���A�Z���u�����[�`���ł�
; ebx, esi, edi, ebp��ۑ����邱��
;

;
; �O���錾
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
; �������f�R�[�_
; �W�����v�e�[�u���쐬
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
; Memory�N���X�L��
		mov	eax,[ebp+8]
		mov	[Memory],eax
; �f�o�C�X�����ɐݒ�
		mov	ebx,_MemDecodeTable
		mov	ecx,384
		mov	edx,MemDecodeData
		mov	esi,[ebp+12]
; ���[�v
.Loop:
; �f�o�C�X�ԍ�(0:Memory 1:GVRAM .... SRAM)���擾
		mov	eax,[edx]
		add	edx,byte 4
; list[]���g���āA���̃C���X�^���X�̃|�C���^���X�g�A
		mov	edi,[esi+eax*4]
		mov	[ebx],edi
		add	ebx,byte 4
; ����
		dec	ecx
		jnz	.Loop
; �I��
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
; �o�C�g�ǂݍ���
;
; EAX	DWORD�߂�l
; ECX	this
; EDX	�A�h���X
;
_ReadByteC:
; �W�����v�e�[�u���ɏ]���ăW�����v(8KB����)
		mov	eax,edx
		sub	eax,0C00000h
		jc	.Memory
		shr	eax,13
		mov	ecx,[_MemDecodeTable+eax*4]
		mov	eax,[ecx]
		jmp	[eax+32]
; ���������Ă�
.Memory:
		mov	ecx,[Memory]
		mov	eax,[ecx]
		jmp	[eax+32]

		align	16

;
; ���[�h�ǂݍ���
;
; EAX	DWORD�߂�l
; ECX	this
; EDX	�A�h���X
;
_ReadWordC:
; �W�����v�e�[�u���ɏ]���ăW�����v(8KB����)
		mov	eax,edx
		sub	eax,0C00000h
		jc	.Memory
		shr	eax,13
		mov	ecx,[_MemDecodeTable+eax*4]
		mov	eax,[ecx]
		jmp	[eax+36]
; ���������Ă�
.Memory:
		mov	ecx,[Memory]
		mov	eax,[ecx]
		jmp	[eax+36]

		align	16

;
; �o�C�g��������
;
; EBX	�f�[�^
; ECX	this
; EDX	�A�h���X
;
_WriteByteC:
; �W�����v�e�[�u���ɏ]���ăW�����v(8KB����)
		mov	eax,edx
		sub	eax,0C00000h
		jc	.Memory
		shr	eax,13
		mov	ecx,[_MemDecodeTable+eax*4]
		mov	eax,[ecx]
		push	ebx
		call	[eax+40]
		ret
; ���������Ă�
.Memory:
		mov	ecx,[Memory]
		mov	eax,[ecx]
		push	ebx
		call	[eax+40]
		ret

		align	16

;
; ���[�h��������
;
; EBX	�f�[�^
; ECX	this
; EDX	�A�h���X
;
_WriteWordC:
; �W�����v�e�[�u���ɏ]���ăW�����v(8KB����)
		mov	eax,edx
		sub	eax,0C00000h
		jc	.Memory
		shr	eax,13
		mov	ecx,[_MemDecodeTable+eax*4]
		mov	eax,[ecx]
		push	ebx
		call	[eax+44]
		ret
; ���������Ă�
.Memory:
		mov	ecx,[Memory]
		mov	eax,[ecx]
		push	ebx
		call	[eax+44]
		ret

		align	16

;
; �o�X�G���[�ǂݍ���
;
; EDX	�A�h���X
;
_ReadErrC:
		push	edx
		call	_ReadBusErr
		add	esp,4
		ret

		align	16

;
; �o�X�G���[��������
;
; EDX	�A�h���X
; EBX	�f�[�^
;
_WriteErrC:
		push	edx
		call	_WriteBusErr
		add	esp,4
		ret

		align	16

;
; �C�x���g�ύX�ʒm
; ���ő�C�x���g����31�܂�(SubExecEvent�̍Ō�Ń_�~�[���[�h���邽��)
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
; �p�����[�^�󂯎��ƃJ�E���^�N���A
		mov	esi,[ebp+8]
		xor	ecx,ecx
		mov	edi,EventTable
; ���[�v
.loop:
		or	esi,esi
		jz	.exit
; �Z�b�g
		mov	[edi],esi
		inc	ecx
; �����擾
		mov	esi,[esi+24]
		add	edi,byte 4
		jmp	.loop
; �����L�^����
.exit:
		mov	[EventNum],ecx
; �I��
		pop	edi
		pop	esi
		pop	ecx
		pop	eax
		pop	ebp
		ret

		align	16

;
; �ŏ��̃C�x���g��T��
; ��CMOV�g�p
;
; void GetMinEvent(DWORD hus)
;
_GetMinEvent:
		push	ebp
		mov	ebp,esp
		push	esi
		push	edi
; �p�����[�^���󂯎��
		mov	eax,[ebp+8]
		mov	esi,EventTable
		mov	ecx,[EventNum]
; ���[�v
.loop:
		mov	edi,[esi]
		add	esi,byte 4
; GetRemain()�Ăяo��
		mov	edx,[edi+4]
		or	edx,edx
; �[���Ȃ炱�̃C�x���g�͍l�����Ȃ�
		cmovz	edx,eax
; ��r
		cmp	edx,eax
; ������ŏ��Ƃ���
		cmovc	eax,edx
; ����
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
; �C�x���g���Ԍ����Ǝ��s
;
; DWORD SubExecEvent(DWORD hus)
;
_SubExecEvent:
		push	ebp
		mov	ebp,esp
		push	esi
		push	edi
; �p�����[�^���󂯎��
		mov	edi,[ebp+8]
		mov	esi,EventTable
		mov	ecx,[EventNum]
; Event�I�u�W�F�N�g���擾
		mov	ebp,[esi]
; �C�x���g���[�v
.loop:
; ���Ԗ��ݒ�Ȃ�A�������Ȃ�
		mov	eax,[ebp+8]
		add	esi,byte 4
		or	eax,eax
		jz	.next
; ���Ԃ������B0���}�C�i�X�Ȃ�C�x���g���s
		sub	[ebp+4],edi
		jna	.exec
; ����
.next:
		dec	ecx
		mov	ebp,[esi]
		jnz	.loop
; �I��
		pop	edi
		pop	esi
		pop	ebp
		ret
; ���s
.exec:
; ���Ԃ����Z�b�g
		mov	[ebp+4],eax
; �C�x���g���s(Device::Callback�Ăяo��)
		push	ecx
		mov	ecx,[ebp+16]
		mov	eax,[ecx]
		mov	edx,ebp
		call	[eax+28]
		pop	ecx
; ���ʂ�0�Ȃ�AFALSE�̂��ߖ�����
		or	eax,eax
		jz	.disable
; ���̃C�x���g��
		dec	ecx
		mov	ebp,[esi]
		jnz	.loop
; �I��
		pop	edi
		pop	esi
		pop	ebp
		ret
; ������(time�����remain��0�N���A)
.disable:
		mov	[ebp+8],eax
		mov	[ebp+4],eax
; ���̃C�x���g��
		dec	ecx
		mov	ebp,[esi]
		jnz	.loop
; �I��
		pop	edi
		pop	esi
		pop	ebp
		ret

;
; �f�[�^�G���A (8KB�P��)
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
; BSS�G���A
;
		section	.bss align=16
		bits	32
Memory:
		resb	4
EventNum:
		resb	4
; �_�~�[(align 16)
		resb	8

_MemDecodeTable:
		resb	384*4
EventTable:
		resb	32*4

;
; �v���O�����I��
;
		end
