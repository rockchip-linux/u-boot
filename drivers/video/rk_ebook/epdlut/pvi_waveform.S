// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2020-2024 Rockchip Electronics Co., Ltd.
 *
 * Author: Zorro Liu <zorro.liu@rock-chips.com>
 */

	.arch armv8-a+nosimd
	.file	"pvi_waveform.c"
	.section	.text.get_wf_temp_index,"ax",@progbits
	.align	2
	.type	get_wf_temp_index, %function
get_wf_temp_index:
	mov	w5, w0
	adrp	x0, .LANCHOR0
	ldr	x4, [x0, #:lo12:.LANCHOR0]
	ldrb	w2, [x4, 38]
	cbz	w2, .L5
	mov	x1, 0
.L4:
	add	x3, x4, x1
	mov	w0, w1
	ldrb	w3, [x3, 48]
	cmp	w3, w5
	bge	.L6
	add	w3, w1, 1
	add	x1, x1, 1
	cmp	w2, w1
	bgt	.L4
	mov	w0, -1
.L3:
	cmp	w2, w3
	bne	.L1
	sub	w0, w2, #1
	ret
.L6:
	mov	w3, w1
	b	.L3
.L5:
	mov	w0, 0
.L1:
	ret
	.size	get_wf_temp_index, .-get_wf_temp_index
	.section	.text.get_wf_frm_num,"ax",@progbits
	.align	2
	.type	get_wf_frm_num, %function
get_wf_frm_num:
	stp	x29, x30, [sp, -64]!
	adrp	x2, .LANCHOR0
	lsl	w0, w0, 2
	add	x29, sp, 0
	ldr	x2, [x2, #:lo12:.LANCHOR0]
	stp	x19, x20, [sp, 16]
	stp	x21, x22, [sp, 32]
	str	x23, [sp, 48]
	ldrb	w3, [x2, 32]
	add	x0, x3, x0, sxtw
	add	x4, x2, x0
	ldrb	w3, [x2, x0]
	ldrb	w6, [x4, 1]
	ldrb	w5, [x4, 3]
	ldrb	w4, [x4, 2]
	add	w3, w3, w6
	add	w3, w3, w4
	cmp	w5, w3, uxtb
	beq	.L9
	mov	w2, 270
.L43:
	adrp	x1, .LANCHOR1
	adrp	x0, .LC0
	add	x1, x1, :lo12:.LANCHOR1
	add	x0, x0, :lo12:.LC0
	bl	printf
.L42:
	mov	w0, -22
.L8:
	ldp	x19, x20, [sp, 16]
	ldp	x21, x22, [sp, 32]
	ldr	x23, [sp, 48]
	ldp	x29, x30, [sp], 64
	ret
.L9:
	ldr	w0, [x2, x0]
	lsl	w1, w1, 2
	and	x0, x0, 16777215
	add	x1, x0, x1, sxtw
	add	x3, x2, x1
	ldrb	w0, [x2, x1]
	ldrb	w5, [x3, 1]
	ldrb	w4, [x3, 3]
	ldrb	w3, [x3, 2]
	add	w0, w0, w5
	add	w0, w0, w3
	cmp	w4, w0, uxtb
	beq	.L11
	mov	w2, 277
	b	.L43
.L11:
	adrp	x20, .LANCHOR2
	ldr	w8, [x2, x1]
	ldr	x10, [x20, #:lo12:.LANCHOR2]
	and	x8, x8, 16777215
	add	x8, x2, x8
	cbnz	x10, .L12
	adrp	x0, .LC1
	add	x0, x0, :lo12:.LC1
	bl	printf
	b	.L42
.L12:
	adrp	x21, .LANCHOR3
	ldr	w12, [x21, #:lo12:.LANCHOR3]
	cmp	w12, 16
	bne	.L30
	adrp	x0, .LANCHOR4
	ldr	w0, [x0, #:lo12:.LANCHOR4]
	cmp	w0, 32
	cset	w13, eq
.L13:
	add	w2, w13, 5
	mov	w0, 0
	mov	w1, 0
	mov	w14, 1
	mov	w11, 0
	mov	w19, 0
.L14:
	ldrb	w3, [x8, w11, uxtw]
	cmp	w3, 255
	beq	.L15
	cmp	w3, 252
	bne	.L16
	eor	w14, w14, 1
	add	w11, w11, 1
.L16:
	uxtw	x17, w11
	add	w15, w11, 1
	lsl	w6, w19, 10
	lsl	w7, w0, w13
	add	w5, w1, 1
	add	w3, w1, 3
	add	w4, w1, 2
	add	w16, w1, 4
	cbz	w14, .L17
	lsl	w1, w1, w2
	add	w1, w1, w6
	add	w1, w1, w7
	add	w6, w6, w7
	ldrb	w23, [x8, x17]
	lsl	w5, w5, w2
	add	w5, w5, w6
	lsl	w4, w4, w2
	add	w4, w4, w6
	and	w17, w23, 3
	lsl	w3, w3, w2
	strb	w17, [x10, x1]
	add	w6, w3, w6
	ubfx	x30, x23, 2, 2
	strb	w30, [x10, x5]
	ubfx	x22, x23, 4, 2
	strb	w22, [x10, x4]
	lsr	w5, w23, 6
	strb	w5, [x10, x6]
	mov	w1, w16
	cmp	w16, w12
	bcc	.L18
	add	w0, w0, 1
	cmp	w12, w0
	bhi	.L31
	add	w19, w19, 1
	mov	w0, 0
.L31:
	mov	w1, 0
.L18:
	ldrb	w6, [x8, w15, uxtw]
.L19:
	cbnz	w6, .L21
	add	w11, w11, 2
	b	.L22
.L30:
	mov	w13, 0
	b	.L13
.L21:
	lsl	w4, w19, 10
	lsl	w7, w1, w2
	add	w7, w7, w4
	lsl	w3, w0, w13
	add	w7, w7, w3
	add	w3, w4, w3
	add	w4, w1, 1
	lsl	w4, w4, w2
	add	w4, w4, w3
	strb	w17, [x10, x7]
	add	w7, w1, 2
	strb	w30, [x10, x4]
	add	w4, w1, 3
	lsl	w7, w7, w2
	add	w7, w7, w3
	lsl	w4, w4, w2
	add	w3, w4, w3
	add	w1, w1, 4
	strb	w22, [x10, x7]
	cmp	w12, w1
	strb	w5, [x10, x3]
	bhi	.L20
	add	w0, w0, 1
	cmp	w12, w0
	bhi	.L32
	add	w19, w19, 1
	mov	w0, 0
.L32:
	mov	w1, 0
.L20:
	sub	w6, w6, #1
	and	w6, w6, 255
	b	.L19
.L17:
	lsl	w1, w1, w2
	add	w1, w1, w6
	add	w1, w1, w7
	add	w6, w6, w7
	ldrb	w11, [x8, x17]
	lsl	w5, w5, w2
	add	w5, w5, w6
	lsl	w4, w4, w2
	add	w4, w4, w6
	and	w17, w11, 3
	lsl	w3, w3, w2
	strb	w17, [x10, x1]
	add	w6, w3, w6
	ubfx	x1, x11, 2, 2
	strb	w1, [x10, x5]
	ubfx	x1, x11, 4, 2
	strb	w1, [x10, x4]
	lsr	w3, w11, 6
	strb	w3, [x10, x6]
	mov	w1, w16
	mov	w11, w15
	cmp	w16, w12
	bcc	.L22
	add	w0, w0, 1
	cmp	w12, w0
	bhi	.L33
	add	w19, w19, 1
	mov	w0, 0
.L33:
	mov	w1, 0
.L22:
	cmp	w19, 254
	bls	.L14
	adrp	x0, .LC2
	add	x0, x0, :lo12:.LC2
	bl	printf
.L15:
	ldr	w0, [x21, #:lo12:.LANCHOR3]
	cmp	w0, 32
	beq	.L23
.L24:
	mov	w0, w19
	b	.L8
.L23:
	adrp	x0, .LANCHOR4
	ldr	w0, [x0, #:lo12:.LANCHOR4]
	cmp	w0, 16
	bne	.L24
	ldr	x4, [x20, #:lo12:.LANCHOR2]
	mov	w0, 0
.L25:
	cmp	w0, w19
	bcs	.L24
	lsl	w5, w0, 10
	mov	w2, 0
.L27:
	lsr	w3, w2, 1
	add	w6, w5, w2, lsl 5
	add	w3, w5, w3, lsl 5
	mov	w1, 0
.L26:
	add	w7, w6, w1
	ldrb	w8, [x4, x7]
	add	w7, w3, w1, lsr 1
	add	w1, w1, 2
	cmp	w1, 32
	strb	w8, [x4, x7]
	bne	.L26
	add	w2, w2, 2
	cmp	w2, 32
	bne	.L27
	add	w0, w0, 1
	and	w0, w0, 255
	b	.L25
	.size	get_wf_frm_num, .-get_wf_frm_num
	.section	.text.get_wf_mode_index.part.1,"ax",@progbits
	.align	2
	.type	get_wf_mode_index.part.1, %function
get_wf_mode_index.part.1:
	cmp	w0, 10
	beq	.L63
	cmp	w0, 12
	beq	.L60
	cmp	w0, 11
	beq	.L61
	cmp	w0, 13
	beq	.L62
	cmp	w0, 14
	beq	.L63
	cmp	w0, 15
	mov	w1, 5
	csel	w0, w0, w1, ne
.L45:
	adrp	x1, .LANCHOR0
	ldr	x1, [x1, #:lo12:.LANCHOR0]
	ldrb	w2, [x1, 16]
	cmp	w2, 32
	beq	.L80
	bhi	.L48
	cmp	w2, 22
	beq	.L49
	bhi	.L50
	cmp	w2, 9
	beq	.L65
	cmp	w2, 18
	beq	.L52
.L46:
	adrp	x1, .LANCHOR6
	add	x1, x1, :lo12:.LANCHOR6
	ldrb	w3, [x1, 9]
	cmp	w3, w2
	beq	.L51
.L80:
	adrp	x1, .LANCHOR5
	add	x1, x1, :lo12:.LANCHOR5
	add	x1, x1, 72
	b	.L51
.L60:
	mov	w0, 6
	b	.L45
.L61:
	mov	w0, 5
	b	.L45
.L62:
	mov	w0, 4
	b	.L45
.L63:
	mov	w0, 3
	b	.L45
.L50:
	cmp	w2, 24
	beq	.L80
	cmp	w2, 25
.L79:
	bne	.L46
	adrp	x1, .LANCHOR5
	add	x1, x1, :lo12:.LANCHOR5
	add	x1, x1, 27
	b	.L51
.L48:
	cmp	w2, 72
	beq	.L54
	bhi	.L55
	cmp	w2, 35
	beq	.L56
	cmp	w2, 67
	b	.L79
.L55:
	cmp	w2, 84
	beq	.L57
	cmp	w2, 114
	bne	.L46
	adrp	x1, .LANCHOR5
	add	x1, x1, :lo12:.LANCHOR5
	add	x1, x1, 63
	b	.L51
.L52:
	adrp	x1, .LANCHOR5
	add	x1, x1, :lo12:.LANCHOR5
	add	x1, x1, 9
.L51:
	ldrb	w0, [x1, w0, sxtw]
	ret
.L49:
	adrp	x1, .LANCHOR5
	add	x1, x1, :lo12:.LANCHOR5
	add	x1, x1, 18
	b	.L51
.L56:
	adrp	x1, .LANCHOR5
	add	x1, x1, :lo12:.LANCHOR5
	add	x1, x1, 36
	b	.L51
.L54:
	adrp	x1, .LANCHOR5
	add	x1, x1, :lo12:.LANCHOR5
	add	x1, x1, 45
	b	.L51
.L57:
	adrp	x1, .LANCHOR5
	add	x1, x1, :lo12:.LANCHOR5
	add	x1, x1, 54
	b	.L51
.L65:
	adrp	x1, .LANCHOR5
	add	x1, x1, :lo12:.LANCHOR5
	b	.L51
	.size	get_wf_mode_index.part.1, .-get_wf_mode_index.part.1
	.section	.text.pvi_wf_add_custom_mode_table,"ax",@progbits
	.align	2
	.global	pvi_wf_add_custom_mode_table
	.type	pvi_wf_add_custom_mode_table, %function
pvi_wf_add_custom_mode_table:
	cmp	w1, 11
	bne	.L85
	adrp	x1, .LANCHOR6
	add	x1, x1, :lo12:.LANCHOR6
	mov	x2, 0
.L83:
	add	x3, x0, x2
	ldrb	w3, [x3, 2]
	strb	w3, [x2, x1]
	add	x2, x2, 1
	cmp	x2, 9
	bne	.L83
	ldrb	w2, [x0]
	strb	w2, [x1, 9]
	ldrb	w0, [x0, 1]
	cmp	w0, 4
	bne	.L84
	mov	w0, 16
.L88:
	str	w0, [x1, 12]
.L86:
	mov	w0, 0
	ret
.L84:
	cmp	w0, 5
	bne	.L86
	mov	w0, 32
	b	.L88
.L85:
	mov	w0, -1
	ret
	.size	pvi_wf_add_custom_mode_table, .-pvi_wf_add_custom_mode_table
	.section	.text.pvi_wf_get_lut,"ax",@progbits
	.align	2
	.global	pvi_wf_get_lut
	.type	pvi_wf_get_lut, %function
pvi_wf_get_lut:
	stp	x29, x30, [sp, -96]!
	add	x29, sp, 0
	stp	x19, x20, [sp, 16]
	stp	x21, x22, [sp, 32]
	stp	x3, x4, [x29, 80]
	adrp	x3, .LANCHOR0
	stp	x23, x24, [sp, 48]
	ldr	x3, [x3, #:lo12:.LANCHOR0]
	stp	x25, x26, [sp, 64]
	cbz	x3, .L105
	mov	w22, w4
	cbnz	x0, .L91
	ldr	x3, [x0, 16]
	cbz	x3, .L106
.L91:
	mov	w3, 3
	sdiv	w3, w2, w3
	cbnz	w3, .L92
	adrp	x3, .LANCHOR7
	ldr	w3, [x3, #:lo12:.LANCHOR7]
	cmp	w3, w1
	bne	.L92
	adrp	x3, .LANCHOR8
	ldr	w3, [x3, #:lo12:.LANCHOR8]
	cmp	w22, w3
	bne	.L92
.L115:
	mov	w0, 0
	b	.L89
.L92:
	adrp	x24, .LANCHOR4
	ldr	w3, [x29, 80]
	cmp	w1, 10
	str	w3, [x24, #:lo12:.LANCHOR4]
	beq	.L115
	adrp	x25, .LANCHOR2
	mov	x19, x0
	ldr	x0, [x0, 16]
	mov	w21, w1
	str	x0, [x25, #:lo12:.LANCHOR2]
	mov	w0, w2
	bl	get_wf_temp_index
	mov	w20, w0
	cmp	w21, 15
	bls	.L94
	adrp	x0, .LC3
	add	x0, x0, :lo12:.LC3
	bl	printf
	mov	w0, 255
.L95:
	mov	w1, w20
	bl	get_wf_frm_num
	mov	w23, w0
	tbz	w0, #31, .L96
	adrp	x0, .LC4
	add	x0, x0, :lo12:.LC4
	bl	printf
	mov	w0, -5
.L89:
	ldp	x19, x20, [sp, 16]
	ldp	x21, x22, [sp, 32]
	ldp	x23, x24, [sp, 48]
	ldp	x25, x26, [sp, 64]
	ldp	x29, x30, [sp], 96
	ret
.L94:
	mov	w0, w21
	bl	get_wf_mode_index.part.1
	and	w0, w0, 255
	b	.L95
.L96:
	ldr	x0, [x19, 8]
	cbz	x0, .L97
	bl	free
	str	xzr, [x19, 8]
.L97:
	add	w20, w23, 15
	mov	w1, 32768
	asr	w20, w20, 4
	lsl	w20, w20, 10
	sxtw	x20, w20
	mov	x0, x20
	bl	kmalloc
	str	x0, [x19, 8]
	mov	x26, x0
	cbnz	x0, .L98
	adrp	x0, .LC5
	add	x0, x0, :lo12:.LC5
	bl	printf
	mov	w0, -12
	b	.L89
.L98:
	ldr	x25, [x25, #:lo12:.LANCHOR2]
	ldr	w1, [x24, #:lo12:.LANCHOR4]
	cmp	w1, 16
	cset	w24, ne
	cbnz	x25, .L99
	adrp	x0, .LC6
	add	x0, x0, :lo12:.LC6
	bl	printf
.L100:
	ldr	w0, [x19]
	and	w0, w0, 65280
	orr	w23, w0, w23
	adrp	x0, .LANCHOR7
	str	w23, [x19]
	str	w21, [x0, #:lo12:.LANCHOR7]
	adrp	x0, .LANCHOR8
	str	w22, [x0, #:lo12:.LANCHOR8]
	b	.L115
.L99:
	mov	x2, x20
	mov	w1, 0
	bl	memset
	mov	w10, 32
	mov	w11, 1
	lsl	w10, w10, w24
	mov	w2, 0
	lsl	w11, w11, w24
	mov	w13, 16
.L101:
	cmp	w23, w2
	beq	.L100
	lsl	w1, w2, 8
	lsl	w8, w2, 10
	sxtw	x8, w8
	mov	w5, 0
	mov	w7, 0
	sdiv	w1, w1, w13
	sxtw	x1, w1
	b	.L104
.L102:
	ldrb	w4, [x3, w12, sxtw]
	and	w14, w6, 30
	add	w6, w6, 2
	add	w12, w12, w11
	and	w4, w4, 3
	cmp	w6, 32
	lsl	w4, w4, w14
	ldr	w14, [x0]
	orr	w4, w14, w4
	str	w4, [x0]
	bne	.L102
	add	w5, w5, 16
	add	w7, w7, w10
	cmp	w5, 256
	beq	.L103
.L104:
	asr	w0, w5, 4
	add	x3, x8, x7, sxtw
	add	x0, x1, x0, sxtw
	mov	w6, 0
	add	x0, x26, x0, lsl 2
	mov	w12, 0
	add	x3, x25, x3
	b	.L102
.L103:
	add	w2, w2, 1
	b	.L101
.L105:
	mov	w0, -19
	b	.L89
.L106:
	mov	w0, -22
	b	.L89
	.size	pvi_wf_get_lut, .-pvi_wf_get_lut
	.section	.text.pvi_wf_get_original_lut,"ax",@progbits
	.align	2
	.global	pvi_wf_get_original_lut
	.type	pvi_wf_get_original_lut, %function
pvi_wf_get_original_lut:
	adrp	x4, .LANCHOR0
	ldr	x4, [x4, #:lo12:.LANCHOR0]
	cbz	x4, .L122
	cbnz	x0, .L118
	ldr	x4, [x0, 16]
	cbz	x4, .L123
.L118:
	stp	x29, x30, [sp, -32]!
	mov	w6, w1
	add	x29, sp, 0
	stp	x19, x20, [sp, 16]
	mov	x19, x0
	adrp	x0, .LANCHOR4
	ldr	x1, [x19, 16]
	str	w3, [x0, #:lo12:.LANCHOR4]
	adrp	x0, .LANCHOR2
	str	x1, [x0, #:lo12:.LANCHOR2]
	mov	w0, w2
	bl	get_wf_temp_index
	mov	w20, w0
	cmp	w6, 15
	bls	.L119
	adrp	x0, .LC3
	add	x0, x0, :lo12:.LC3
	bl	printf
	mov	w0, 255
.L120:
	mov	w1, w20
	bl	get_wf_frm_num
	tbz	w0, #31, .L121
	adrp	x0, .LC4
	add	x0, x0, :lo12:.LC4
	bl	printf
	mov	w0, -5
.L116:
	ldp	x19, x20, [sp, 16]
	ldp	x29, x30, [sp], 32
	ret
.L119:
	mov	w0, w6
	bl	get_wf_mode_index.part.1
	and	w0, w0, 255
	b	.L120
.L121:
	ldr	w1, [x19]
	and	w1, w1, 65280
	orr	w0, w1, w0
	str	w0, [x19]
	mov	w0, 0
	b	.L116
.L122:
	mov	w0, -19
	ret
.L123:
	mov	w0, -22
	ret
	.size	pvi_wf_get_original_lut, .-pvi_wf_get_original_lut
	.section	.text.pvi_wf_input,"ax",@progbits
	.align	2
	.global	pvi_wf_input
	.type	pvi_wf_input, %function
pvi_wf_input:
	adrp	x1, .LANCHOR0
	ldr	x2, [x1, #:lo12:.LANCHOR0]
	cbnz	x2, .L136
	stp	x29, x30, [sp, -32]!
	add	x29, sp, 0
	stp	x19, x20, [sp, 16]
	mov	x20, x1
	mov	x19, x0
	ldrb	w1, [x0, 16]
	cmp	w1, 32
	beq	.L130
	bhi	.L131
	cmp	w1, 22
	beq	.L130
	bhi	.L132
	cmp	w1, 9
	beq	.L133
	cmp	w1, 18
	beq	.L133
.L129:
	cbz	w1, .L137
	adrp	x0, .LANCHOR6
	add	x0, x0, :lo12:.LANCHOR6
	ldrb	w2, [x0, 9]
	cmp	w2, w1
	bne	.L135
	ldr	w2, [x0, 12]
	b	.L171
.L132:
	sub	w0, w1, #24
	cmp	w0, 1
	bhi	.L129
.L130:
	mov	w2, 32
.L171:
	adrp	x0, .LANCHOR3
	str	w2, [x0, #:lo12:.LANCHOR3]
.L133:
	adrp	x0, .LANCHOR3
	ldr	w2, [x0, #:lo12:.LANCHOR3]
	adrp	x0, .LC8
	add	x0, x0, :lo12:.LC8
	bl	printf
	str	x19, [x20, #:lo12:.LANCHOR0]
	mov	w0, 0
.L127:
	ldp	x19, x20, [sp, 16]
	ldp	x29, x30, [sp], 32
	ret
.L131:
	cmp	w1, 72
	beq	.L130
	bhi	.L134
	cmp	w1, 35
	beq	.L133
	cmp	w1, 67
.L170:
	bne	.L129
	b	.L130
.L134:
	cmp	w1, 84
	beq	.L133
	cmp	w1, 114
	b	.L170
.L135:
	ldrb	w2, [x19, 22]
	adrp	x0, .LC7
	add	x0, x0, :lo12:.LC7
	bl	printf
.L137:
	mov	w0, -8
	b	.L127
.L136:
	mov	w0, -16
	ret
	.size	pvi_wf_input, .-pvi_wf_input
	.section	.text.pvi_wf_get_version,"ax",@progbits
	.align	2
	.global	pvi_wf_get_version
	.type	pvi_wf_get_version, %function
pvi_wf_get_version:
	adrp	x0, .LANCHOR0
	ldr	x2, [x0, #:lo12:.LANCHOR0]
	cbz	x2, .L175
	adrp	x0, .LANCHOR9
	add	x0, x0, :lo12:.LANCHOR9
	mov	x1, 0
.L174:
	add	x3, x2, x1
	ldrb	w3, [x3, 65]
	strb	w3, [x1, x0]
	add	x1, x1, 1
	cmp	x1, 31
	bne	.L174
	strb	wzr, [x0, 31]
	ret
.L175:
	mov	x0, 0
	ret
	.size	pvi_wf_get_version, .-pvi_wf_get_version
	.section	.text.pvi_wf_get_wf_bit,"ax",@progbits
	.align	2
	.global	pvi_wf_get_wf_bit
	.type	pvi_wf_get_wf_bit, %function
pvi_wf_get_wf_bit:
	adrp	x0, .LANCHOR0
	ldr	x0, [x0, #:lo12:.LANCHOR0]
	cbz	x0, .L179
	adrp	x0, .LANCHOR3
	ldr	w0, [x0, #:lo12:.LANCHOR3]
	ret
.L179:
	mov	w0, 0
	ret
	.size	pvi_wf_get_wf_bit, .-pvi_wf_get_wf_bit
	.global	custom_mode_table
	.global	pvi_mode_table
	.section	.bss.custom_mode_table,"aw",@nobits
	.align	3
	.set	.LANCHOR6,. + 0
	.type	custom_mode_table, %object
	.size	custom_mode_table, 16
custom_mode_table:
	.zero	16
	.section	.bss.fix.6338,"aw",@nobits
	.align	2
	.set	.LANCHOR8,. + 0
	.type	fix.6338, %object
	.size	fix.6338, 4
fix.6338:
	.zero	4
	.section	.bss.global_waveform,"aw",@nobits
	.align	3
	.set	.LANCHOR0,. + 0
	.type	global_waveform, %object
	.size	global_waveform, 8
global_waveform:
	.zero	8
	.section	.bss.spi_id_buffer.6373,"aw",@nobits
	.align	3
	.set	.LANCHOR9,. + 0
	.type	spi_id_buffer.6373, %object
	.size	spi_id_buffer.6373, 32
spi_id_buffer.6373:
	.zero	32
	.section	.bss.waveformdata,"aw",@nobits
	.align	3
	.set	.LANCHOR2,. + 0
	.type	waveformdata, %object
	.size	waveformdata, 8
waveformdata:
	.zero	8
	.section	.data.maxpic,"aw",@progbits
	.align	2
	.set	.LANCHOR3,. + 0
	.type	maxpic, %object
	.size	maxpic, 4
maxpic:
	.word	16
	.section	.data.need_pic,"aw",@progbits
	.align	2
	.set	.LANCHOR4,. + 0
	.type	need_pic, %object
	.size	need_pic, 4
need_pic:
	.word	16
	.section	.data.stype.6336,"aw",@progbits
	.align	2
	.set	.LANCHOR7,. + 0
	.type	stype.6336, %object
	.size	stype.6336, 4
stype.6336:
	.word	16
	.section	.rodata.__func__.6308,"a",@progbits
	.align	3
	.set	.LANCHOR1,. + 0
	.type	__func__.6308, %object
	.size	__func__.6308, 15
__func__.6308:
	.string	"get_wf_frm_num"
	.section	.rodata.get_wf_frm_num.str1.1,"aMS",@progbits,1
.LC0:
	.string	"pvi: %s %d check error\n"
.LC1:
	.string	"waveformdata is NULL\n"
.LC2:
	.string	"pvi: decodec waveform error, framenum err\n"
	.section	.rodata.pvi_mode_table,"a",@progbits
	.align	3
	.set	.LANCHOR5,. + 0
	.type	pvi_mode_table, %object
	.size	pvi_mode_table, 81
pvi_mode_table:
	.byte	0
	.byte	1
	.byte	1
	.byte	2
	.byte	3
	.byte	3
	.byte	3
	.byte	4
	.byte	3
	.byte	0
	.byte	1
	.byte	7
	.byte	3
	.byte	3
	.byte	5
	.byte	6
	.byte	4
	.byte	5
	.byte	0
	.byte	1
	.byte	1
	.byte	2
	.byte	3
	.byte	4
	.byte	4
	.byte	6
	.byte	5
	.byte	0
	.byte	1
	.byte	7
	.byte	2
	.byte	3
	.byte	4
	.byte	5
	.byte	6
	.byte	4
	.byte	0
	.byte	1
	.byte	5
	.byte	2
	.byte	3
	.byte	3
	.byte	3
	.byte	4
	.byte	3
	.byte	0
	.byte	1
	.byte	1
	.byte	2
	.byte	3
	.byte	4
	.byte	4
	.byte	6
	.byte	4
	.byte	0
	.byte	1
	.byte	1
	.byte	2
	.byte	3
	.byte	4
	.byte	4
	.byte	5
	.byte	4
	.byte	0
	.byte	1
	.byte	1
	.byte	2
	.byte	3
	.byte	4
	.byte	11
	.byte	6
	.byte	10
	.byte	0
	.byte	1
	.byte	1
	.byte	2
	.byte	3
	.byte	4
	.byte	5
	.byte	6
	.byte	4
	.section	.rodata.pvi_wf_get_lut.str1.1,"aMS",@progbits,1
.LC3:
	.string	"pvi: unspport PVI waveform type"
.LC4:
	.string	"pvi waveform get frame number failed\n"
.LC5:
	.string	"kzalloc output->data failed\n"
.LC6:
	.string	"wf_table or lut_data is NULL\n"
	.section	.rodata.pvi_wf_input.str1.1,"aMS",@progbits,1
.LC7:
	.string	"pvi : Unknow waveform version 0x%x, 0x%x, may be wrong waveform file\n"
.LC8:
	.string	"pvi : input waveform version 0x%x, gray level: %d\n"
	.hidden	free
	.ident	"GCC: (Linaro GCC 6.3-2017.05) 6.3.1 20170404"
	.section	.note.GNU-stack,"",@progbits
