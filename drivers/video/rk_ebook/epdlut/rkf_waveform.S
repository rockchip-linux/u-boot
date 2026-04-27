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
	ldr	w2, [x1, #:lo12:.LANCHOR1]
	mul	w1, w19, w2
	mul	w1, w1, w2
	mov	w2, 4
	sdiv	w1, w1, w2
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
	stp	x29, x30, [sp, -112]!
	adrp	x4, .LANCHOR0
	add	x29, sp, 0
	ldr	x4, [x4, #:lo12:.LANCHOR0]
	stp	x27, x28, [sp, 80]
	stp	x19, x20, [sp, 16]
	stp	x21, x22, [sp, 32]
	stp	x23, x24, [sp, 48]
	stp	x25, x26, [sp, 64]
	ldr	w28, [x3, 8]
	cbz	x4, .L104
	cbz	x0, .L105
	adrp	x24, .LANCHOR3
	ldr	w3, [x3]
	cmp	w1, 10
	str	w3, [x24, #:lo12:.LANCHOR3]
	beq	.L109
	adrp	x25, .LANCHOR4
	and	w2, w2, 65535
	and	w22, w2, 255
	mov	w3, 5
	ldrb	w2, [x25, #:lo12:.LANCHOR4]
	sdiv	w4, w22, w3
	sdiv	w2, w2, w3
	cmp	w4, w2
	bne	.L109
	adrp	x26, .LANCHOR5
	ldr	w2, [x26, #:lo12:.LANCHOR5]
	cmp	w1, w2
	bne	.L109
	adrp	x27, .LANCHOR6
	ldr	w2, [x27, #:lo12:.LANCHOR6]
	cmp	w28, w2
	beq	.L109
	mov	x19, x0
	ldr	x0, [x0, 16]
	mov	w21, w1
	cbz	x0, .L57
	bl	free
	str	xzr, [x19, 16]
.L57:
	cmp	w21, 8
	beq	.L58
	cmp	w21, 12
	beq	.L59
	cmp	w21, 11
	beq	.L60
	cmp	w21, 13
	beq	.L61
	cmp	w21, 14
	beq	.L58
	cmp	w21, 15
	beq	.L60
	cmp	w21, 7
	bhi	.L62
	adrp	x0, .L64
	add	x0, x0, :lo12:.L64
	ldrh	w0, [x0,w21,uxtw #1]
	adr	x1, .Lrtx64
	add	x0, x1, w0, sxth #2
	br	x0
.Lrtx64:
	.section	.rodata.rkf_wf_get_lut,"a",@progbits
	.align	0
	.align	2
.L64:
	.2byte	(.L63 - .Lrtx64) / 4
	.2byte	(.L65 - .Lrtx64) / 4
	.2byte	(.L66 - .Lrtx64) / 4
	.2byte	(.L58 - .Lrtx64) / 4
	.2byte	(.L61 - .Lrtx64) / 4
	.2byte	(.L60 - .Lrtx64) / 4
	.2byte	(.L59 - .Lrtx64) / 4
	.2byte	(.L67 - .Lrtx64) / 4
	.section	.text.rkf_wf_get_lut
.L63:
	mov	w1, 0
	mov	w0, w22
	bl	get_wf_buf
	ldrb	w20, [x0]
	add	w23, w20, 15
	asr	w23, w23, 4
	lsl	w1, w23, 2
	bl	decode_wf_data
	str	x0, [x29, 104]
	cbz	x0, .L62
	mov	w1, 32768
	ubfiz	x0, x23, 10, 5
	bl	kmalloc
	ldr	x4, [x29, 104]
	cbz	x0, .L110
	mov	x2, 0
.L70:
	cmp	w23, w2
	bgt	.L74
.L69:
	str	x0, [x19, 16]
	cbnz	x0, .L75
	mov	x0, x4
	bl	free
.L62:
	adrp	x0, .LC11
	add	x0, x0, :lo12:.LC11
	bl	printf
	mov	w0, -5
	b	.L55
.L74:
	ldr	w7, [x4, x2, lsl 2]
	lsl	x8, x2, 8
	mov	x3, 0
.L73:
	lsl	w1, w3, 1
	mov	w5, 0
	asr	w1, w7, w1
	and	w1, w1, 3
	mov	w6, w1
.L71:
	lsl	w10, w1, w5
	add	w5, w5, 2
	orr	w6, w6, w10
	cmp	w5, 32
	bne	.L71
	add	x5, x8, x3, lsl 4
	mov	x1, 0
	add	x5, x0, x5, lsl 2
.L72:
	str	w6, [x5, x1, lsl 2]
	add	x1, x1, 1
	cmp	x1, 16
	bne	.L72
	add	x3, x3, 1
	cmp	x3, 16
	bne	.L73
	add	x2, x2, 1
	b	.L70
.L110:
	mov	x0, 0
	b	.L69
.L75:
	mov	x0, x4
	bl	free
.L76:
	ldr	x5, [x19, 24]
	mov	x0, 0
.L80:
	cmp	w20, w0
	bgt	.L84
.L85:
	ldr	x0, [x19, 16]
	cbz	x0, .L95
	bl	free
	str	xzr, [x19, 16]
.L95:
	add	w23, w20, 15
	mov	w1, 32768
	asr	w23, w23, 4
	lsl	w23, w23, 10
	sxtw	x23, w23
	mov	x0, x23
	bl	kmalloc
	str	x0, [x19, 16]
	cbnz	x0, .L96
	adrp	x0, .LC9
	add	x0, x0, :lo12:.LC9
	bl	printf
	mov	w0, -12
.L55:
	ldp	x19, x20, [sp, 16]
	ldp	x21, x22, [sp, 32]
	ldp	x23, x24, [sp, 48]
	ldp	x25, x26, [sp, 64]
	ldp	x27, x28, [sp, 80]
	ldp	x29, x30, [sp], 112
	ret
.L58:
	mov	w2, 2
.L162:
	mov	w1, w22
	add	x0, x19, 16
	bl	parse_wf_gray16.isra.3
	mov	w20, w0
	tbnz	w0, #31, .L62
	cbz	w21, .L76
	b	.L103
.L61:
	mov	w2, 3
	b	.L162
.L60:
	mov	w2, 4
	b	.L162
.L59:
	mov	w2, 5
	b	.L162
.L66:
	mov	w2, 2
	mov	w1, w22
	add	x0, x19, 16
	bl	parse_wf_gray16.isra.3
	mov	w20, w0
	tbnz	w0, #31, .L62
.L103:
	adrp	x0, .LANCHOR1
	mov	w5, 16
	ldr	x7, [x19, 24]
	mov	x8, 0
	ldr	w0, [x0, #:lo12:.LANCHOR1]
	mov	w4, 0
	sdiv	w5, w0, w5
	mul	w11, w0, w5
.L86:
	cmp	w20, w4
	bgt	.L90
	cmp	w0, 32
	bne	.L85
	ldr	w0, [x24, #:lo12:.LANCHOR3]
	cmp	w0, 16
	bne	.L85
	mov	w2, 0
.L91:
	cmp	w20, w2
	ble	.L85
	lsl	w4, w2, 10
	mov	w0, 0
	b	.L94
.L65:
	mov	w1, 1
.L156:
	mov	w0, w22
	bl	get_wf_buf
	adrp	x1, .LANCHOR1
	ldrb	w20, [x0]
	ldr	w2, [x1, #:lo12:.LANCHOR1]
	mul	w1, w20, w2
	mul	w1, w1, w2
	mov	w2, 4
	sdiv	w1, w1, w2
	bl	decode_wf_data
	str	x0, [x19, 16]
	cbnz	x0, .L103
	b	.L62
.L67:
	mov	w1, 6
	b	.L156
.L84:
	ldr	x4, [x19, 16]
	lsl	w7, w0, 10
	mov	x2, 0
	add	x4, x4, x0, lsl 6
.L81:
	add	w6, w7, w2, lsl 5
	mov	w1, 0
.L82:
	add	w8, w6, w1
	ldr	w3, [x4, x2, lsl 2]
	lsl	w10, w1, 1
	add	w1, w1, 1
	cmp	w1, 16
	lsr	w3, w3, w10
	and	w3, w3, 3
	strb	w3, [x5, x8]
	bne	.L82
	add	x2, x2, 1
	cmp	x2, 16
	bne	.L81
	add	x0, x0, 1
	b	.L80
.L90:
	ldr	x15, [x19, 16]
	lsl	w13, w4, 10
	mov	w10, 0
	mov	w6, 0
.L87:
	cmp	w6, w0
	beq	.L89
	uxtw	x14, w10
	add	w12, w13, w6, lsl 5
	mov	w1, 0
.L88:
	lsl	w17, w1, 1
	add	w16, w12, w1
	lsr	w3, w17, 5
	add	w1, w1, 1
	add	x3, x3, x14
	cmp	w1, w0
	add	x3, x15, x3, lsl 2
	ldr	w2, [x3, x8]
	lsr	w2, w2, w17
	and	w2, w2, 3
	strb	w2, [x7, x16]
	bne	.L88
	add	w6, w6, 1
	add	w10, w10, w5
	b	.L87
.L89:
	add	w4, w4, 1
	add	x8, x8, x11, sxtw 2
	b	.L86
.L92:
	add	w6, w5, w1
	ldrb	w8, [x7, x6]
	add	w6, w3, w1, lsr 1
	add	w1, w1, 2
	cmp	w1, 32
	strb	w8, [x7, x6]
	bne	.L92
	add	w0, w0, 2
	cmp	w0, 32
	beq	.L93
.L94:
	lsr	w3, w0, 1
	add	w5, w4, w0, lsl 5
	add	w3, w4, w3, lsl 5
	mov	w1, 0
	b	.L92
.L93:
	add	w2, w2, 1
	b	.L91
.L96:
	ldr	x5, [x19, 24]
	ldr	w1, [x24, #:lo12:.LANCHOR3]
	cmp	w1, 16
	cset	w24, ne
	cbnz	x5, .L97
	adrp	x0, .LC10
	add	x0, x0, :lo12:.LC10
	bl	printf
.L98:
	ldr	w2, [x25, #:lo12:.LANCHOR4]
	mov	w0, 1
	str	w21, [x26, #:lo12:.LANCHOR5]
	and	w2, w2, 65280
	str	w28, [x27, #:lo12:.LANCHOR6]
	orr	w22, w2, w22
	str	w20, [x19]
	str	w22, [x25, #:lo12:.LANCHOR4]
	str	wzr, [x19, 8]
	b	.L55
.L97:
	stp	x0, x5, [x29, 96]
	mov	x2, x23
	mov	w1, 0
	bl	memset
	ldp	x4, x5, [x29, 96]
	mov	w11, 32
	mov	w12, 1
	lsl	w11, w11, w24
	lsl	w12, w12, w24
	mov	w8, 0
	mov	w13, 16
.L99:
	cmp	w8, w20
	bge	.L98
	lsl	w0, w8, 8
	lsl	w10, w8, 10
	sxtw	x10, w10
	mov	w14, 0
	mov	w15, 0
	sdiv	w0, w0, w13
	sxtw	x7, w0
	b	.L102
.L100:
	ldrb	w2, [x1, w6, sxtw]
	and	w16, w3, 30
	add	w3, w3, 2
	add	w6, w6, w12
	and	w2, w2, 3
	cmp	w3, 32
	lsl	w2, w2, w16
	ldr	w16, [x0]
	orr	w2, w16, w2
	str	w2, [x0]
	bne	.L100
	add	w14, w14, 16
	add	w15, w15, w11
	cmp	w14, 256
	beq	.L101
.L102:
	asr	w0, w14, 4
	add	x1, x10, x15, sxtw
	add	x0, x7, x0, sxtw
	mov	w3, 0
	add	x0, x4, x0, lsl 2
	mov	w6, 0
	add	x1, x5, x1
	b	.L100
.L101:
	add	w8, w8, 1
	b	.L99
.L104:
	mov	w0, -19
	b	.L55
.L105:
	mov	w0, -22
	b	.L55
.L109:
	mov	w0, 0
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
	.section	.bss.normal_repair.3319,"aw",@nobits
	.align	2
	.set	.LANCHOR6,. + 0
	.type	normal_repair.3319, %object
	.size	normal_repair.3319, 4
normal_repair.3319:
	.zero	4
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
	.section	.data.sftemp.3318,"aw",@progbits
	.align	2
	.set	.LANCHOR4,. + 0
	.type	sftemp.3318, %object
	.size	sftemp.3318, 4
sftemp.3318:
	.word	-1
	.section	.data.stype.3317,"aw",@progbits
	.align	2
	.set	.LANCHOR5,. + 0
	.type	stype.3317, %object
	.size	stype.3317, 4
stype.3317:
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
