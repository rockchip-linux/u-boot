// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2020-2024 Rockchip Electronics Co., Ltd.
 *
 * Author: Zorro Liu <zorro.liu@rock-chips.com>
 */

	.arch armv8-a+nosimd
	.file	"rkf_waveform.c"
	.section	.text.get_wf_buf,"ax",@progbits
	.align	2
	.type	get_wf_buf, %function
get_wf_buf:
	adrp	x2, .LANCHOR0
	cmp	w1, 6
	ldr	x2, [x2, #:lo12:.LANCHOR0]
	bhi	.L17
	adrp	x3, .L4
	add	x3, x3, :lo12:.L4
	ldrb	w1, [x3,w1,uxtw]
	adr	x3, .Lrtx4
	add	x1, x3, w1, sxtb #2
	br	x1
.Lrtx4:
	.section	.rodata.get_wf_buf,"a",@progbits
	.align	0
	.align	2
.L4:
	.byte	(.L3 - .Lrtx4) / 4
	.byte	(.L5 - .Lrtx4) / 4
	.byte	(.L6 - .Lrtx4) / 4
	.byte	(.L7 - .Lrtx4) / 4
	.byte	(.L8 - .Lrtx4) / 4
	.byte	(.L9 - .Lrtx4) / 4
	.byte	(.L10 - .Lrtx4) / 4
	.section	.text.get_wf_buf
.L3:
	add	x1, x2, 148
	add	x3, x2, 596
.L11:
	cmp	w0, 0
	mov	w4, 50
	csel	w0, w0, wzr, ge
	cmp	w0, 50
	csel	w0, w0, w4, le
	mov	w4, 0
.L12:
	ldrb	w5, [x1]
	cmp	w0, w5
	bge	.L14
.L13:
	sub	w4, w4, #1
.L15:
	ldr	w0, [x3, w4, uxtw 2]
	add	x0, x2, x0
	ret
.L5:
	add	x1, x2, 468
	add	x3, x2, 1876
	b	.L11
.L6:
	add	x1, x2, 212
	add	x3, x2, 852
	b	.L11
.L7:
	add	x1, x2, 276
	add	x3, x2, 1108
	b	.L11
.L8:
	add	x1, x2, 340
	add	x3, x2, 1364
	b	.L11
.L9:
	add	x1, x2, 404
	add	x3, x2, 1620
	b	.L11
.L10:
	add	x1, x2, 532
	add	x3, x2, 2132
	b	.L11
.L16:
	ldrb	w6, [x1, 1]
	cmp	w6, w5
	bcc	.L13
	add	x1, x1, 1
	b	.L12
.L14:
	add	w4, w4, 1
	cmp	w4, 51
	bne	.L16
	mov	w4, 0
	b	.L15
.L17:
	mov	x0, 0
	ret
	.size	get_wf_buf, .-get_wf_buf
	.section	.text.decode_wf_data,"ax",@progbits
	.align	2
	.type	decode_wf_data, %function
decode_wf_data:
	stp	x29, x30, [sp, -48]!
	add	x29, sp, 0
	stp	x19, x20, [sp, 16]
	uxtw	x20, w1
	str	x21, [sp, 32]
	mov	w1, 32768
	mov	x21, x0
	mov	x0, x20
	bl	kmalloc
	mov	x19, x0
	cbz	x0, .L19
	mov	x2, x20
	add	x1, x21, 4
	bl	memcpy
.L19:
	mov	x0, x19
	ldr	x21, [sp, 32]
	ldp	x19, x20, [sp, 16]
	ldp	x29, x30, [sp], 48
	ret
	.size	decode_wf_data, .-decode_wf_data
	.section	.text.parse_wf_gray16.isra.3,"ax",@progbits
	.align	2
	.type	parse_wf_gray16.isra.3, %function
parse_wf_gray16.isra.3:
	stp	x29, x30, [sp, -32]!
	add	x29, sp, 0
	stp	x19, x20, [sp, 16]
	mov	x20, x0
	mov	w0, w1
	mov	w1, w2
	bl	get_wf_buf
	ldrb	w19, [x0]
	adrp	x1, .LANCHOR1
	ldr	w1, [x1, #:lo12:.LANCHOR1]
	lsl	w2, w19, 1
	mul	w1, w1, w1
	mul	w1, w1, w2
	lsr	w1, w1, 3
	bl	decode_wf_data
	str	x0, [x20]
	cmp	x0, 0
	mov	w0, -22
	csel	w0, w19, w0, ne
	ldp	x19, x20, [sp, 16]
	ldp	x29, x30, [sp], 32
	ret
	.size	parse_wf_gray16.isra.3, .-parse_wf_gray16.isra.3
	.section	.text.rkf_wf_input,"ax",@progbits
	.align	2
	.global	rkf_wf_input
	.type	rkf_wf_input, %function
rkf_wf_input:
	stp	x29, x30, [sp, -80]!
	add	x29, sp, 0
	stp	x21, x22, [sp, 32]
	adrp	x22, .LANCHOR0
	stp	x19, x20, [sp, 16]
	ldr	x1, [x22, #:lo12:.LANCHOR0]
	stp	x23, x24, [sp, 48]
	str	x25, [sp, 64]
	cbnz	x1, .L43
	adrp	x20, .LC0
	add	x20, x20, :lo12:.LC0
	add	x21, x0, 20
	mov	x19, x0
	mov	x0, x20
	bl	strlen
	mov	x1, x20
	mov	x2, x0
	mov	x0, x21
	bl	strncmp
	cbnz	w0, .L30
	mov	w1, 16
.L47:
	adrp	x0, .LANCHOR1
	str	w1, [x0, #:lo12:.LANCHOR1]
	adrp	x0, .LANCHOR1
	mov	x1, x21
	ldr	w2, [x0, #:lo12:.LANCHOR1]
	adrp	x0, .LC2
	add	x0, x0, :lo12:.LC2
	bl	printf
	mov	x2, 12
	adrp	x1, .LC3
	add	x0, x19, 4
	add	x1, x1, :lo12:.LC3
	bl	strncmp
	mov	w20, w0
	cbz	w0, .L33
	adrp	x0, .LC4
	add	x0, x0, :lo12:.LC4
	bl	printf
	adrp	x0, .LC5
	add	x0, x0, :lo12:.LC5
.L48:
	bl	printf
	mov	w20, -1
	b	.L28
.L30:
	adrp	x20, .LC1
	add	x20, x20, :lo12:.LC1
	mov	x0, x20
	bl	strlen
	mov	x1, x20
	mov	x2, x0
	mov	x0, x21
	bl	strncmp
	cbnz	w0, .L32
	mov	w1, 32
	b	.L47
.L39:
	ldrb	w0, [x19, x1]
	add	x1, x1, 1
	eor	w0, w0, w2, lsr 24
	ldr	w0, [x21, x0, lsl 2]
	eor	w2, w0, w2, lsl 8
	b	.L38
.L43:
	mov	w20, -16
.L28:
	mov	w0, w20
	ldr	x25, [sp, 64]
	ldp	x19, x20, [sp, 16]
	ldp	x21, x22, [sp, 32]
	ldp	x23, x24, [sp, 48]
	ldp	x29, x30, [sp], 80
	ret
.L32:
	adrp	x0, .LC7
	add	x0, x0, :lo12:.LC7
	b	.L48
.L33:
	ldr	w24, [x19]
	cmp	w24, 1048576
	bgt	.L41
	adrp	x21, .LANCHOR2
	add	x23, x21, :lo12:.LANCHOR2
	ldr	w25, [x19, w24, sxtw]
	mov	x2, 1024
	mov	w1, 0
	mov	x0, x23
	bl	memset
	mov	w4, 3511
	mov	x3, 0
	movk	w4, 0x4c1, lsl 16
.L37:
	lsl	w0, w3, 22
	mov	w2, 8
	mov	w1, 0
.L36:
	eor	w6, w0, w1
	lsl	w1, w1, 1
	mov	w5, w1
	cmp	w6, 0
	eor	w1, w1, w4
	lsl	w0, w0, 1
	csel	w1, w1, w5, lt
	subs	w2, w2, #1
	bne	.L36
	str	w1, [x3, x23]
	add	x3, x3, 4
	cmp	x3, 1024
	bne	.L37
	add	x21, x21, :lo12:.LANCHOR2
	mov	x1, 0
.L38:
	cmp	w24, w1
	bhi	.L39
	cmp	w25, w2
	beq	.L40
	adrp	x0, .LC6
	mov	w1, w25
	add	x0, x0, :lo12:.LC6
	bl	printf
.L41:
	adrp	x0, .LC8
	add	x0, x0, :lo12:.LC8
	b	.L48
.L40:
	str	x19, [x22, #:lo12:.LANCHOR0]
	b	.L28
	.size	rkf_wf_input, .-rkf_wf_input
	.section	.text.rkf_wf_get_version,"ax",@progbits
	.align	2
	.global	rkf_wf_get_version
	.type	rkf_wf_get_version, %function
rkf_wf_get_version:
	adrp	x0, .LANCHOR0
	ldr	x1, [x0, #:lo12:.LANCHOR0]
	add	x0, x1, 84
	cmp	x1, 0
	csel	x0, x0, xzr, ne
	ret
	.size	rkf_wf_get_version, .-rkf_wf_get_version
	.section	.text.rkf_wf_get_wf_bit,"ax",@progbits
	.align	2
	.global	rkf_wf_get_wf_bit
	.type	rkf_wf_get_wf_bit, %function
rkf_wf_get_wf_bit:
	adrp	x0, .LANCHOR0
	ldr	x0, [x0, #:lo12:.LANCHOR0]
	cbz	x0, .L54
	adrp	x0, .LANCHOR1
	ldr	w0, [x0, #:lo12:.LANCHOR1]
	ret
.L54:
	mov	w0, 0
	ret
	.size	rkf_wf_get_wf_bit, .-rkf_wf_get_wf_bit
	.section	.text.rkf_wf_get_lut,"ax",@progbits
	.align	2
	.global	rkf_wf_get_lut
	.type	rkf_wf_get_lut, %function
rkf_wf_get_lut:
	stp	x29, x30, [sp, -96]!
	add	x29, sp, 0
	stp	x19, x20, [sp, 16]
	stp	x21, x22, [sp, 32]
	stp	x3, x4, [x29, 80]
	adrp	x3, .LANCHOR0
	stp	x23, x24, [sp, 48]
	ldr	x3, [x3, #:lo12:.LANCHOR0]
	str	x25, [sp, 64]
	cbz	x3, .L105
	cbz	x0, .L106
	ldr	w3, [x29, 80]
	adrp	x23, .LANCHOR3
	str	w3, [x23, #:lo12:.LANCHOR3]
	mov	w3, 5
	sdiv	w3, w2, w3
	cbnz	w3, .L57
	adrp	x4, .LANCHOR4
	ldr	w4, [x4, #:lo12:.LANCHOR4]
	cmp	w4, w1
	beq	.L55
.L57:
	cmp	w1, 10
	beq	.L107
	mov	x20, x0
	ldr	x0, [x0, 8]
	mov	w19, w2
	mov	w21, w1
	cbz	x0, .L58
	bl	free
	str	xzr, [x20, 8]
.L58:
	cmp	w21, 8
	beq	.L59
	cmp	w21, 12
	beq	.L60
	cmp	w21, 11
	beq	.L61
	cmp	w21, 13
	beq	.L62
	cmp	w21, 14
	beq	.L59
	cmp	w21, 15
	beq	.L61
	cmp	w21, 7
	bhi	.L63
	adrp	x0, .L65
	add	x0, x0, :lo12:.L65
	ldrh	w0, [x0,w21,uxtw #1]
	adr	x1, .Lrtx65
	add	x0, x1, w0, sxth #2
	br	x0
.Lrtx65:
	.section	.rodata.rkf_wf_get_lut,"a",@progbits
	.align	0
	.align	2
.L65:
	.2byte	(.L64 - .Lrtx65) / 4
	.2byte	(.L66 - .Lrtx65) / 4
	.2byte	(.L67 - .Lrtx65) / 4
	.2byte	(.L59 - .Lrtx65) / 4
	.2byte	(.L62 - .Lrtx65) / 4
	.2byte	(.L61 - .Lrtx65) / 4
	.2byte	(.L60 - .Lrtx65) / 4
	.2byte	(.L68 - .Lrtx65) / 4
	.section	.text.rkf_wf_get_lut
.L64:
	mov	w1, 0
	mov	w0, w19
	bl	get_wf_buf
	ldrb	w19, [x0]
	add	w22, w19, 15
	lsr	w22, w22, 4
	lsl	w1, w22, 2
	bl	decode_wf_data
	mov	x24, x0
	cbz	x0, .L63
	mov	w1, 32768
	ubfiz	x0, x22, 10, 5
	bl	kmalloc
	cbz	x0, .L108
	mov	x2, 0
.L71:
	cmp	w22, w2
	bgt	.L75
.L70:
	str	x0, [x20, 8]
	cbnz	x0, .L76
	mov	x0, x24
	bl	free
.L63:
	adrp	x0, .LC11
	add	x0, x0, :lo12:.LC11
	bl	printf
	mov	w3, -5
	b	.L55
.L75:
	ldr	w6, [x24, x2, lsl 2]
	lsl	x7, x2, 8
	mov	x3, 0
.L74:
	lsl	w1, w3, 1
	mov	w4, 0
	asr	w1, w6, w1
	and	w1, w1, 3
	mov	w5, w1
.L72:
	lsl	w8, w1, w4
	add	w4, w4, 2
	orr	w5, w5, w8
	cmp	w4, 32
	bne	.L72
	add	x4, x7, x3, lsl 4
	mov	x1, 0
	add	x4, x0, x4, lsl 2
.L73:
	str	w5, [x4, x1, lsl 2]
	add	x1, x1, 1
	cmp	x1, 16
	bne	.L73
	add	x3, x3, 1
	cmp	x3, 16
	bne	.L74
	add	x2, x2, 1
	b	.L71
.L108:
	mov	x0, 0
	b	.L70
.L76:
	mov	x0, x24
	bl	free
.L77:
	ldr	x5, [x20, 16]
	mov	x0, 0
.L81:
	cmp	w19, w0
	bgt	.L85
.L86:
	ldr	x0, [x20, 8]
	cbz	x0, .L96
	bl	free
	str	xzr, [x20, 8]
.L96:
	add	w22, w19, 15
	mov	w1, 32768
	asr	w22, w22, 4
	lsl	w22, w22, 10
	sxtw	x22, w22
	mov	x0, x22
	bl	kmalloc
	str	x0, [x20, 8]
	mov	x24, x0
	cbnz	x0, .L97
	adrp	x0, .LC9
	add	x0, x0, :lo12:.LC9
	bl	printf
	mov	w3, -12
.L55:
	ldp	x19, x20, [sp, 16]
	mov	w0, w3
	ldp	x21, x22, [sp, 32]
	ldp	x23, x24, [sp, 48]
	ldr	x25, [sp, 64]
	ldp	x29, x30, [sp], 96
	ret
.L59:
	mov	w2, 2
.L160:
	mov	w1, w19
	add	x0, x20, 8
	bl	parse_wf_gray16.isra.3
	mov	w19, w0
	tbnz	w0, #31, .L63
	cbz	w21, .L77
	b	.L104
.L62:
	mov	w2, 3
	b	.L160
.L61:
	mov	w2, 4
	b	.L160
.L60:
	mov	w2, 5
	b	.L160
.L67:
	mov	w1, w19
	mov	w2, 2
	add	x0, x20, 8
	bl	parse_wf_gray16.isra.3
	mov	w19, w0
	tbnz	w0, #31, .L63
.L104:
	adrp	x0, .LANCHOR1
	mov	w5, 16
	ldr	x7, [x20, 16]
	mov	x8, 0
	ldr	w0, [x0, #:lo12:.LANCHOR1]
	mov	w4, 0
	sdiv	w5, w0, w5
	mul	w12, w0, w5
.L87:
	cmp	w4, w19
	blt	.L91
	cmp	w0, 32
	bne	.L86
	ldr	w0, [x23, #:lo12:.LANCHOR3]
	cmp	w0, 16
	bne	.L86
	mov	w2, 0
.L92:
	cmp	w2, w19
	bge	.L86
	lsl	w4, w2, 10
	mov	w0, 0
	b	.L95
.L66:
	mov	w1, 1
.L154:
	mov	w0, w19
	bl	get_wf_buf
	adrp	x1, .LANCHOR1
	ldrb	w19, [x0]
	ldr	w1, [x1, #:lo12:.LANCHOR1]
	lsl	w2, w19, 1
	mul	w1, w1, w1
	mul	w1, w2, w1
	lsr	w1, w1, 3
	bl	decode_wf_data
	str	x0, [x20, 8]
	cbnz	x0, .L104
	b	.L63
.L68:
	mov	w1, 6
	b	.L154
.L85:
	ldr	x4, [x20, 8]
	lsl	w7, w0, 10
	mov	x2, 0
	add	x4, x4, x0, lsl 6
.L82:
	add	w6, w7, w2, lsl 5
	mov	w1, 0
.L83:
	add	w8, w6, w1
	ldr	w3, [x4, x2, lsl 2]
	lsl	w10, w1, 1
	add	w1, w1, 1
	cmp	w1, 16
	lsr	w3, w3, w10
	and	w3, w3, 3
	strb	w3, [x5, x8]
	bne	.L83
	add	x2, x2, 1
	cmp	x2, 16
	bne	.L82
	add	x0, x0, 1
	b	.L81
.L91:
	ldr	x16, [x20, 8]
	lsl	w14, w4, 10
	mov	w10, 0
	mov	w6, 0
.L88:
	cmp	w6, w0
	beq	.L90
	uxtw	x15, w10
	add	w13, w14, w6, lsl 5
	mov	w1, 0
.L89:
	lsl	w11, w1, 1
	add	w17, w13, w1
	lsr	w3, w11, 5
	add	w1, w1, 1
	add	x3, x3, x15
	cmp	w1, w0
	add	x3, x16, x3, lsl 2
	ldr	w2, [x3, x8]
	lsr	w2, w2, w11
	and	w2, w2, 3
	strb	w2, [x7, x17]
	bne	.L89
	add	w6, w6, 1
	add	w10, w10, w5
	b	.L88
.L90:
	add	w4, w4, 1
	add	x8, x8, x12, sxtw 2
	b	.L87
.L93:
	add	w6, w5, w1
	ldrb	w8, [x7, x6]
	add	w6, w3, w1, lsr 1
	add	w1, w1, 2
	cmp	w1, 32
	strb	w8, [x7, x6]
	bne	.L93
	add	w0, w0, 2
	cmp	w0, 32
	beq	.L94
.L95:
	lsr	w3, w0, 1
	add	w5, w4, w0, lsl 5
	add	w3, w4, w3, lsl 5
	mov	w1, 0
	b	.L93
.L94:
	add	w2, w2, 1
	b	.L92
.L97:
	ldr	x25, [x20, 16]
	ldr	w1, [x23, #:lo12:.LANCHOR3]
	cmp	w1, 16
	cset	w23, ne
	cbnz	x25, .L98
	adrp	x0, .LC10
	add	x0, x0, :lo12:.LC10
	bl	printf
.L99:
	ldr	w0, [x20]
	and	w0, w0, 65280
	orr	w19, w0, w19
	adrp	x0, .LANCHOR4
	str	w19, [x20]
	str	w21, [x0, #:lo12:.LANCHOR4]
.L107:
	mov	w3, 0
	b	.L55
.L98:
	mov	x2, x22
	mov	w1, 0
	bl	memset
	mov	w10, 32
	mov	w11, 1
	lsl	w10, w10, w23
	mov	w2, 0
	lsl	w11, w11, w23
	mov	w14, 16
.L100:
	cmp	w2, w19
	bge	.L99
	lsl	w1, w2, 8
	lsl	w8, w2, 10
	sxtw	x8, w8
	mov	w5, 0
	mov	w7, 0
	sdiv	w1, w1, w14
	sxtw	x1, w1
	b	.L103
.L101:
	ldrb	w4, [x3, w12, sxtw]
	and	w13, w6, 30
	add	w6, w6, 2
	add	w12, w12, w11
	and	w4, w4, 3
	cmp	w6, 32
	lsl	w4, w4, w13
	ldr	w13, [x0]
	orr	w4, w13, w4
	str	w4, [x0]
	bne	.L101
	add	w5, w5, 16
	add	w7, w7, w10
	cmp	w5, 256
	beq	.L102
.L103:
	asr	w0, w5, 4
	add	x3, x8, x7, sxtw
	add	x0, x1, x0, sxtw
	mov	w6, 0
	add	x0, x24, x0, lsl 2
	mov	w12, 0
	add	x3, x25, x3
	b	.L101
.L102:
	add	w2, w2, 1
	b	.L100
.L105:
	mov	w3, -19
	b	.L55
.L106:
	mov	w3, -22
	b	.L55
	.size	rkf_wf_get_lut, .-rkf_wf_get_lut
	.section	.bss.crc32_table,"aw",@nobits
	.align	3
	.set	.LANCHOR2,. + 0
	.type	crc32_table, %object
	.size	crc32_table, 1024
crc32_table:
	.zero	1024
	.section	.bss.global_waveform,"aw",@nobits
	.align	3
	.set	.LANCHOR0,. + 0
	.type	global_waveform, %object
	.size	global_waveform, 8
global_waveform:
	.zero	8
	.section	.data.maxpic,"aw",@progbits
	.align	2
	.set	.LANCHOR1,. + 0
	.type	maxpic, %object
	.size	maxpic, 4
maxpic:
	.word	16
	.section	.data.need_pic,"aw",@progbits
	.align	2
	.set	.LANCHOR3,. + 0
	.type	need_pic, %object
	.size	need_pic, 4
need_pic:
	.word	16
	.section	.data.stype.6472,"aw",@progbits
	.align	2
	.set	.LANCHOR4,. + 0
	.type	stype.6472, %object
	.size	stype.6472, 4
stype.6472:
	.word	16
	.section	.rodata.rkf_wf_get_lut.str1.1,"aMS",@progbits,1
.LC9:
	.string	"kzalloc output->data failed\n"
.LC10:
	.string	"wf_table or lut_data is NULL\n"
.LC11:
	.string	"rkf waveform get frame number failed\n"
	.section	.rodata.rkf_wf_input.str1.1,"aMS",@progbits,1
.LC0:
	.string	"rkf:v1.0"
.LC1:
	.string	"rkf:v2.0"
.LC2:
	.string	"rkf: input waveform version: %s, gray level: %d\n"
.LC3:
	.string	"rkf waveform"
.LC4:
	.string	"rkf: check format failed\n"
.LC5:
	.string	"rkf: failed to check RKF file format\n"
.LC6:
	.string	"[EBOOK]: waveform crc err readcrc = %x crccheck = %x\n"
.LC7:
	.string	"rkf: failed to check RKF file version\n"
.LC8:
	.string	"rkf: failed to check crc RKF waveform\n"
	.hidden	free
	.ident	"GCC: (Linaro GCC 6.3-2017.05) 6.3.1 20170404"
	.section	.note.GNU-stack,"",@progbits
