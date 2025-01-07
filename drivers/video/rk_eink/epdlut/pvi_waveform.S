// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2020-2024 Rockchip Electronics Co., Ltd.
 *
 * Author: Zorro Liu <zorro.liu@rock-chips.com>
 */

	.arch armv8-a+nosimd
	.file	"pvi_waveform.c"
	.text
.Ltext0:
	.cfi_sections	.debug_frame
	.section	.text.get_wf_temp_index,"ax",@progbits
	.align	2
	.type	get_wf_temp_index, %function
get_wf_temp_index:
.LFB218:
	.file 1 "/home/hzb/sdk/ebc-dev/rk_eink/../epdlut/pvi_waveform.c"
	.loc 1 243 0
	.cfi_startproc
.LVL0:
	.loc 1 243 0
	mov	w4, w0
	.loc 1 244 0
	adrp	x0, .LANCHOR0
.LVL1:
	.loc 1 248 0
	mov	x1, 0
	.loc 1 244 0
	ldr	x3, [x0, #:lo12:.LANCHOR0]
	ldrb	w2, [x3, 38]
.LVL2:
.L2:
	mov	w0, w1
.LVL3:
	.loc 1 248 0 discriminator 1
	cmp	w1, w2
	blt	.L4
	.loc 1 255 0
	cmp	w1, w2
	bne	.L5
	.loc 1 256 0
	sub	w0, w2, #1
.LVL4:
	ret
.LVL5:
.L4:
	.loc 1 249 0
	add	x5, x3, x1
	add	x1, x1, 1
	ldrb	w5, [x5, 48]
	cmp	w5, w4
	blt	.L2
.LVL6:
.L1:
	.loc 1 259 0
	ret
.LVL7:
.L5:
	.loc 1 245 0
	mov	w0, -1
.LVL8:
	.loc 1 258 0
	b	.L1
	.cfi_endproc
.LFE218:
	.size	get_wf_temp_index, .-get_wf_temp_index
	.section	.text.get_wf_frm_num,"ax",@progbits
	.align	2
	.type	get_wf_frm_num, %function
get_wf_frm_num:
.LFB219:
	.loc 1 262 0
	.cfi_startproc
.LVL9:
	stp	x29, x30, [sp, -64]!
	.cfi_def_cfa_offset 64
	.cfi_offset 29, -64
	.cfi_offset 30, -56
	.loc 1 263 0
	adrp	x2, .LANCHOR0
	.loc 1 267 0
	lsl	w0, w0, 2
.LVL10:
	.loc 1 262 0
	add	x29, sp, 0
	.cfi_def_cfa_register 29
	.loc 1 263 0
	ldr	x2, [x2, #:lo12:.LANCHOR0]
.LVL11:
	.loc 1 262 0
	stp	x19, x20, [sp, 16]
.LVL12:
	stp	x21, x22, [sp, 32]
	str	x23, [sp, 48]
	.cfi_offset 19, -48
	.cfi_offset 20, -40
	.cfi_offset 21, -32
	.cfi_offset 22, -24
	.cfi_offset 23, -16
	.loc 1 267 0
	ldrb	w3, [x2, 32]
	add	x0, x3, x0, sxtw
	add	x4, x2, x0
.LVL13:
	.loc 1 268 0
	ldrb	w3, [x2, x0]
	ldrb	w6, [x4, 1]
	ldrb	w5, [x4, 3]
	ldrb	w4, [x4, 2]
.LVL14:
	add	w3, w3, w6
	add	w3, w3, w4
	cmp	w5, w3, uxtb
	beq	.L7
	.loc 1 269 0
	mov	w2, 269
.LVL15:
.L41:
	.loc 1 276 0
	adrp	x1, .LANCHOR1
	adrp	x0, .LC0
	add	x1, x1, :lo12:.LANCHOR1
	add	x0, x0, :lo12:.LC0
	bl	printf
.LVL16:
.L40:
.LBB8:
.LBB9:
	.loc 1 69 0
	mov	w0, -22
.L6:
.LBE9:
.LBE8:
	.loc 1 283 0
	ldp	x19, x20, [sp, 16]
	ldp	x21, x22, [sp, 32]
	ldr	x23, [sp, 48]
	ldp	x29, x30, [sp], 64
	.cfi_remember_state
	.cfi_restore 30
	.cfi_restore 29
	.cfi_restore 23
	.cfi_restore 21
	.cfi_restore 22
	.cfi_restore 19
	.cfi_restore 20
	.cfi_def_cfa 31, 0
	ret
.LVL17:
.L7:
	.cfi_restore_state
	.loc 1 272 0
	ldr	w0, [x2, x0]
.LVL18:
	.loc 1 274 0
	lsl	w1, w1, 2
.LVL19:
	and	x0, x0, 16777215
.LVL20:
	add	x1, x0, x1, sxtw
	add	x3, x2, x1
.LVL21:
	.loc 1 275 0
	ldrb	w0, [x2, x1]
	ldrb	w5, [x3, 1]
	ldrb	w4, [x3, 3]
	ldrb	w3, [x3, 2]
.LVL22:
	add	w0, w0, w5
	add	w0, w0, w3
	cmp	w4, w0, uxtb
	beq	.L9
	.loc 1 276 0
	mov	w2, 276
.LVL23:
	b	.L41
.LVL24:
.L9:
.LBB13:
.LBB10:
	.loc 1 67 0
	adrp	x20, .LANCHOR2
.LBE10:
.LBE13:
	.loc 1 279 0
	ldr	w8, [x2, x1]
.LBB14:
.LBB11:
	.loc 1 67 0
	ldr	x10, [x20, #:lo12:.LANCHOR2]
.LBE11:
.LBE14:
	.loc 1 281 0
	and	x8, x8, 16777215
	add	x8, x2, x8
.LVL25:
.LBB15:
.LBB12:
	.loc 1 67 0
	cbnz	x10, .L10
	.loc 1 68 0
	adrp	x0, .LC1
	add	x0, x0, :lo12:.LC1
	bl	printf
.LVL26:
	b	.L40
.LVL27:
.L10:
	.loc 1 72 0
	adrp	x21, .LANCHOR3
	ldr	w12, [x21, #:lo12:.LANCHOR3]
	cmp	w12, 16
	bne	.L28
	adrp	x0, .LANCHOR4
	ldr	w0, [x0, #:lo12:.LANCHOR4]
	cmp	w0, 32
	cset	w13, eq
.L11:
.LVL28:
	add	w2, w13, 5
.LVL29:
	.loc 1 133 0
	mov	w0, 0
	mov	w1, 0
.LVL30:
	mov	w14, 1
	mov	w11, 0
	mov	w19, 0
.LVL31:
.L12:
	.loc 1 81 0
	ldrb	w3, [x8, w11, uxtw]
	cmp	w3, 255
	beq	.L13
	.loc 1 86 0
	cmp	w3, 252
	bne	.L14
	.loc 1 87 0
	eor	w14, w14, 1
.LVL32:
	.loc 1 88 0
	add	w11, w11, 1
.LVL33:
.L14:
	uxtw	x17, w11
	add	w15, w11, 1
	lsl	w6, w19, 10
	lsl	w7, w0, w13
	add	w5, w1, 1
	add	w3, w1, 3
	add	w4, w1, 2
	add	w16, w1, 4
	.loc 1 91 0
	cbz	w14, .L15
	.loc 1 93 0
	lsl	w1, w1, w2
	add	w1, w1, w6
	add	w1, w1, w7
	add	w6, w6, w7
	.loc 1 92 0
	ldrb	w23, [x8, x17]
.LVL34:
	.loc 1 94 0
	lsl	w5, w5, w2
	add	w5, w5, w6
	.loc 1 95 0
	lsl	w4, w4, w2
	add	w4, w4, w6
	.loc 1 93 0
	and	w17, w23, 3
	.loc 1 96 0
	lsl	w3, w3, w2
	.loc 1 93 0
	strb	w17, [x10, x1]
	.loc 1 96 0
	add	w6, w3, w6
	.loc 1 94 0
	ubfx	x30, x23, 2, 2
	strb	w30, [x10, x5]
	.loc 1 95 0
	ubfx	x22, x23, 4, 2
	strb	w22, [x10, x4]
	.loc 1 96 0
	lsr	w5, w23, 6
	strb	w5, [x10, x6]
	mov	w1, w16
.LVL35:
	.loc 1 99 0
	cmp	w16, w12
	bcc	.L16
.LVL36:
	.loc 1 101 0
	add	w0, w0, 1
.LVL37:
	cmp	w12, w0
	bhi	.L29
.LVL38:
	.loc 1 103 0
	add	w19, w19, 1
.LVL39:
	.loc 1 102 0
	mov	w0, 0
.LVL40:
.L29:
	.loc 1 100 0
	mov	w1, 0
.LVL41:
.L16:
	.loc 1 107 0
	ldrb	w6, [x8, w15, uxtw]
.LVL42:
.L17:
	.loc 1 108 0
	cbnz	w6, .L19
	.loc 1 122 0
	add	w11, w11, 2
.LVL43:
	b	.L20
.LVL44:
.L28:
	.loc 1 72 0
	mov	w13, 0
	b	.L11
.LVL45:
.L19:
	.loc 1 109 0
	lsl	w4, w19, 10
	lsl	w7, w1, w2
	add	w7, w7, w4
	lsl	w3, w0, w13
	add	w7, w7, w3
	add	w3, w4, w3
	add	w4, w1, 1
	.loc 1 110 0
	lsl	w4, w4, w2
	add	w4, w4, w3
	.loc 1 109 0
	strb	w17, [x10, x7]
	.loc 1 110 0
	add	w7, w1, 2
	strb	w30, [x10, x4]
	.loc 1 111 0
	add	w4, w1, 3
.LVL46:
	lsl	w7, w7, w2
	add	w7, w7, w3
	.loc 1 112 0
	lsl	w4, w4, w2
.LVL47:
	add	w3, w4, w3
	add	w1, w1, 4
.LVL48:
	.loc 1 111 0
	strb	w22, [x10, x7]
.LVL49:
	.loc 1 113 0
	cmp	w12, w1
	.loc 1 112 0
	strb	w5, [x10, x3]
	.loc 1 113 0
	bhi	.L18
.LVL50:
	.loc 1 115 0
	add	w0, w0, 1
.LVL51:
	cmp	w12, w0
	bhi	.L30
.LVL52:
	.loc 1 117 0
	add	w19, w19, 1
.LVL53:
	.loc 1 116 0
	mov	w0, 0
.LVL54:
.L30:
	.loc 1 114 0
	mov	w1, 0
.LVL55:
.L18:
	.loc 1 108 0
	sub	w6, w6, #1
.LVL56:
	and	w6, w6, 255
.LVL57:
	b	.L17
.LVL58:
.L15:
	.loc 1 126 0
	lsl	w1, w1, w2
	add	w1, w1, w6
	add	w1, w1, w7
	add	w6, w6, w7
	.loc 1 125 0
	ldrb	w11, [x8, x17]
.LVL59:
	.loc 1 127 0
	lsl	w5, w5, w2
	add	w5, w5, w6
	.loc 1 128 0
	lsl	w4, w4, w2
	add	w4, w4, w6
	.loc 1 126 0
	and	w17, w11, 3
	.loc 1 129 0
	lsl	w3, w3, w2
	.loc 1 126 0
	strb	w17, [x10, x1]
	.loc 1 129 0
	add	w6, w3, w6
	.loc 1 127 0
	ubfx	x1, x11, 2, 2
	strb	w1, [x10, x5]
	.loc 1 128 0
	ubfx	x1, x11, 4, 2
	strb	w1, [x10, x4]
	.loc 1 129 0
	lsr	w3, w11, 6
	strb	w3, [x10, x6]
	mov	w1, w16
.LVL60:
	.loc 1 130 0
	mov	w11, w15
.LVL61:
	.loc 1 132 0
	cmp	w16, w12
	bcc	.L20
.LVL62:
	.loc 1 134 0
	add	w0, w0, 1
.LVL63:
	cmp	w12, w0
	bhi	.L31
.LVL64:
	.loc 1 136 0
	add	w19, w19, 1
.LVL65:
	.loc 1 135 0
	mov	w0, 0
.LVL66:
.L31:
	.loc 1 133 0
	mov	w1, 0
.LVL67:
.L20:
	.loc 1 76 0
	cmp	w19, 254
	bls	.L12
	.loc 1 77 0
	adrp	x0, .LC2
.LVL68:
	add	x0, x0, :lo12:.LC2
	bl	printf
.LVL69:
.L13:
	.loc 1 142 0
	ldr	w0, [x21, #:lo12:.LANCHOR3]
	cmp	w0, 32
	beq	.L21
.L22:
	.loc 1 154 0
	mov	w0, w19
	b	.L6
.L21:
	.loc 1 142 0
	adrp	x0, .LANCHOR4
	ldr	w0, [x0, #:lo12:.LANCHOR4]
	cmp	w0, 16
	bne	.L22
	.loc 1 147 0
	ldr	x4, [x20, #:lo12:.LANCHOR2]
	mov	w0, 0
.L23:
.LVL70:
	.loc 1 144 0
	cmp	w0, w19
	bcs	.L22
	.loc 1 147 0
	lsl	w5, w0, 10
	mov	w2, 0
.L25:
.LVL71:
	.loc 1 148 0
	lsr	w3, w2, 1
	add	w6, w5, w2, lsl 5
	add	w3, w5, w3, lsl 5
	mov	w1, 0
.LVL72:
.L24:
	.loc 1 147 0
	add	w7, w6, w1
	ldrb	w8, [x4, x7]
.LVL73:
	.loc 1 148 0
	add	w7, w3, w1, lsr 1
	.loc 1 146 0
	add	w1, w1, 2
.LVL74:
	cmp	w1, 32
	.loc 1 148 0
	strb	w8, [x4, x7]
	.loc 1 146 0
	bne	.L24
	.loc 1 145 0
	add	w2, w2, 2
.LVL75:
	cmp	w2, 32
	bne	.L25
	.loc 1 144 0
	add	w0, w0, 1
.LVL76:
	and	w0, w0, 255
.LVL77:
	b	.L23
.LBE12:
.LBE15:
	.cfi_endproc
.LFE219:
	.size	get_wf_frm_num, .-get_wf_frm_num
	.section	.text.get_wf_mode_index,"ax",@progbits
	.align	2
	.type	get_wf_mode_index, %function
get_wf_mode_index:
.LFB217:
	.loc 1 217 0
	.cfi_startproc
.LVL78:
	.loc 1 221 0
	cmp	w0, 14
	bls	.L43
	.loc 1 217 0
	stp	x29, x30, [sp, -16]!
	.cfi_def_cfa_offset 16
	.cfi_offset 29, -16
	.cfi_offset 30, -8
	.loc 1 222 0
	adrp	x0, .LC3
.LVL79:
	add	x0, x0, :lo12:.LC3
	.loc 1 217 0
	add	x29, sp, 0
	.cfi_def_cfa_register 29
	.loc 1 222 0
	bl	printf
.LVL80:
	.loc 1 223 0
	mov	w0, 255
	.loc 1 240 0
	ldp	x29, x30, [sp], 16
	.cfi_restore 30
	.cfi_restore 29
	.cfi_def_cfa 31, 0
	ret
.LVL81:
.L43:
.LBB20:
.LBB21:
	.loc 1 224 0
	cmp	w0, 10
	beq	.L59
	.loc 1 226 0
	cmp	w0, 12
	beq	.L60
	.loc 1 228 0
	cmp	w0, 11
	beq	.L61
	.loc 1 230 0
	cmp	w0, 13
	beq	.L62
	.loc 1 235 0
	cmp	w0, 14
	mov	w1, 3
	csel	w0, w0, w1, ne
.LVL82:
.L45:
	.loc 1 237 0
	adrp	x1, .LANCHOR0
	ldr	x1, [x1, #:lo12:.LANCHOR0]
	ldrb	w2, [x1, 16]
.LVL83:
.LBB22:
.LBB23:
	.loc 1 195 0
	cmp	w2, 32
	beq	.L83
	bhi	.L48
	cmp	w2, 22
	beq	.L49
	bhi	.L50
	cmp	w2, 9
	beq	.L64
	cmp	w2, 18
	beq	.L52
.L46:
	.loc 1 206 0
	adrp	x1, .LANCHOR6
.LVL84:
	add	x1, x1, :lo12:.LANCHOR6
	ldrb	w3, [x1, 9]
	cmp	w3, w2
	beq	.L51
.L83:
	.loc 1 209 0
	adrp	x1, .LANCHOR5
	add	x1, x1, :lo12:.LANCHOR5
	add	x1, x1, 72
	b	.L51
.LVL85:
.L59:
.LBE23:
.LBE22:
	.loc 1 225 0
	mov	w0, 3
.LVL86:
	b	.L45
.LVL87:
.L60:
	.loc 1 227 0
	mov	w0, 6
.LVL88:
	b	.L45
.LVL89:
.L61:
	.loc 1 229 0
	mov	w0, 5
.LVL90:
	b	.L45
.LVL91:
.L62:
	.loc 1 231 0
	mov	w0, 4
.LVL92:
	b	.L45
.LVL93:
.L50:
.LBB26:
.LBB24:
	.loc 1 195 0
	cmp	w2, 24
	beq	.L83
	cmp	w2, 25
.L82:
	bne	.L46
.LVL94:
	.loc 1 199 0
	adrp	x1, .LANCHOR5
.LVL95:
	add	x1, x1, :lo12:.LANCHOR5
	add	x1, x1, 27
	b	.L51
.LVL96:
.L48:
	.loc 1 195 0
	cmp	w2, 72
	beq	.L54
	bhi	.L55
	cmp	w2, 35
	beq	.L56
	cmp	w2, 67
	b	.L82
.L55:
	cmp	w2, 84
	beq	.L57
	cmp	w2, 114
	bne	.L46
.LVL97:
	.loc 1 203 0
	adrp	x1, .LANCHOR5
.LVL98:
	add	x1, x1, :lo12:.LANCHOR5
	add	x1, x1, 63
	b	.L51
.LVL99:
.L52:
	.loc 1 197 0
	adrp	x1, .LANCHOR5
.LVL100:
	add	x1, x1, :lo12:.LANCHOR5
	add	x1, x1, 9
.LVL101:
.L51:
.LBE24:
.LBE26:
	.loc 1 239 0
	ldrb	w0, [x1, w0, sxtw]
.LVL102:
	ret
.LVL103:
.L49:
.LBB27:
.LBB25:
	.loc 1 198 0
	adrp	x1, .LANCHOR5
.LVL104:
	add	x1, x1, :lo12:.LANCHOR5
	add	x1, x1, 18
	b	.L51
.LVL105:
.L56:
	.loc 1 200 0
	adrp	x1, .LANCHOR5
.LVL106:
	add	x1, x1, :lo12:.LANCHOR5
	add	x1, x1, 36
	b	.L51
.LVL107:
.L54:
	.loc 1 201 0
	adrp	x1, .LANCHOR5
.LVL108:
	add	x1, x1, :lo12:.LANCHOR5
	add	x1, x1, 45
	b	.L51
.LVL109:
.L57:
	.loc 1 202 0
	adrp	x1, .LANCHOR5
.LVL110:
	add	x1, x1, :lo12:.LANCHOR5
	add	x1, x1, 54
	b	.L51
.LVL111:
.L64:
	.loc 1 196 0
	adrp	x1, .LANCHOR5
.LVL112:
	add	x1, x1, :lo12:.LANCHOR5
	b	.L51
.LBE25:
.LBE27:
.LBE21:
.LBE20:
	.cfi_endproc
.LFE217:
	.size	get_wf_mode_index, .-get_wf_mode_index
	.section	.text.pvi_wf_add_custom_mode_table,"ax",@progbits
	.align	2
	.global	pvi_wf_add_custom_mode_table
	.type	pvi_wf_add_custom_mode_table, %function
pvi_wf_add_custom_mode_table:
.LFB215:
	.loc 1 177 0
	.cfi_startproc
.LVL113:
	.loc 1 178 0
	cmp	w1, 11
	bne	.L88
.LBB28:
	.loc 1 182 0 discriminator 3
	adrp	x1, .LANCHOR6
.LVL114:
	add	x1, x1, :lo12:.LANCHOR6
.LBE28:
	mov	x2, 0
.L86:
.LVL115:
.LBB29:
	add	x3, x0, x2
	ldrb	w3, [x3, 2]
	strb	w3, [x2, x1]
.LVL116:
	add	x2, x2, 1
.LVL117:
	.loc 1 181 0 discriminator 3
	cmp	x2, 9
	bne	.L86
.LBE29:
	.loc 1 183 0
	ldrb	w2, [x0]
	strb	w2, [x1, 9]
	.loc 1 184 0
	ldrb	w0, [x0, 1]
.LVL118:
	cmp	w0, 4
	bne	.L87
	.loc 1 185 0
	mov	w0, 16
.L91:
	.loc 1 187 0
	str	w0, [x1, 12]
.L89:
	.loc 1 189 0
	mov	w0, 0
	.loc 1 190 0
	ret
.L87:
	.loc 1 186 0
	cmp	w0, 5
	bne	.L89
	.loc 1 187 0
	mov	w0, 32
	b	.L91
.LVL119:
.L88:
	.loc 1 179 0
	mov	w0, -1
.LVL120:
	ret
	.cfi_endproc
.LFE215:
	.size	pvi_wf_add_custom_mode_table, .-pvi_wf_add_custom_mode_table
	.section	.text.pvi_wf_get_lut,"ax",@progbits
	.align	2
	.global	pvi_wf_get_lut
	.type	pvi_wf_get_lut, %function
pvi_wf_get_lut:
.LFB226:
	.loc 1 444 0
	.cfi_startproc
.LVL121:
	.loc 1 453 0
	adrp	x5, .LANCHOR0
	ldr	x5, [x5, #:lo12:.LANCHOR0]
	cbz	x5, .L106
	.loc 1 456 0
	cbnz	x0, .L94
	.loc 1 456 0 is_stmt 0 discriminator 1
	ldr	x5, [x0, 16]
	cbz	x5, .L107
.L94:
	.loc 1 462 0 is_stmt 1
	adrp	x5, .LANCHOR7
	mov	w6, 3
	sdiv	w7, w2, w6
	ldr	w5, [x5, #:lo12:.LANCHOR7]
	sdiv	w5, w5, w6
	cmp	w7, w5
	bne	.L95
	.loc 1 462 0 is_stmt 0 discriminator 1
	adrp	x5, .LANCHOR8
	ldr	w5, [x5, #:lo12:.LANCHOR8]
	cmp	w5, w1
	bne	.L95
	.loc 1 462 0 discriminator 2
	adrp	x5, .LANCHOR9
	ldr	w5, [x5, #:lo12:.LANCHOR9]
	cmp	w5, w4
	bne	.L95
	.loc 1 463 0 is_stmt 1
	mov	w0, 0
.LVL122:
	.loc 1 529 0
	ret
.LVL123:
.L95:
	.loc 1 444 0
	stp	x29, x30, [sp, -80]!
	.cfi_def_cfa_offset 80
	.cfi_offset 29, -80
	.cfi_offset 30, -72
	.loc 1 468 0
	cmp	w1, 10
	.loc 1 444 0
	add	x29, sp, 0
	.cfi_def_cfa_register 29
	stp	x23, x24, [sp, 48]
	.cfi_offset 23, -32
	.cfi_offset 24, -24
	.loc 1 466 0
	adrp	x24, .LANCHOR4
	.loc 1 444 0
	stp	x19, x20, [sp, 16]
	stp	x21, x22, [sp, 32]
	stp	x25, x26, [sp, 64]
	.cfi_offset 19, -64
	.cfi_offset 20, -56
	.cfi_offset 21, -48
	.cfi_offset 22, -40
	.cfi_offset 25, -16
	.cfi_offset 26, -8
	.loc 1 466 0
	str	w3, [x24, #:lo12:.LANCHOR4]
	.loc 1 468 0
	beq	.L119
	.loc 1 470 0
	adrp	x25, .LANCHOR2
	mov	x19, x0
	ldr	x0, [x0, 16]
.LVL124:
	mov	w21, w1
	mov	w22, w4
	str	x0, [x25, #:lo12:.LANCHOR2]
	.loc 1 471 0
	mov	w0, w2
	bl	get_wf_temp_index
.LVL125:
	mov	w20, w0
.LVL126:
	.loc 1 472 0
	mov	w0, w21
.LVL127:
	bl	get_wf_mode_index
.LVL128:
	.loc 1 473 0
	and	w0, w0, 255
	mov	w1, w20
	bl	get_wf_frm_num
.LVL129:
	mov	w23, w0
.LVL130:
	.loc 1 474 0
	tbz	w0, #31, .L97
	.loc 1 475 0
	adrp	x0, .LC4
.LVL131:
	add	x0, x0, :lo12:.LC4
	bl	printf
.LVL132:
	.loc 1 476 0
	mov	w0, -5
.LVL133:
.L92:
	.loc 1 529 0
	ldp	x19, x20, [sp, 16]
	ldp	x21, x22, [sp, 32]
	ldp	x23, x24, [sp, 48]
	ldp	x25, x26, [sp, 64]
	ldp	x29, x30, [sp], 80
	.cfi_remember_state
	.cfi_restore 30
	.cfi_restore 29
	.cfi_restore 25
	.cfi_restore 26
	.cfi_restore 23
	.cfi_restore 24
	.cfi_restore 21
	.cfi_restore 22
	.cfi_restore 19
	.cfi_restore 20
	.cfi_def_cfa 31, 0
	ret
.LVL134:
.L97:
	.cfi_restore_state
	.loc 1 491 0
	ldr	x0, [x19, 8]
.LVL135:
	cbz	x0, .L98
.LVL136:
.LBB49:
.LBB50:
	.file 2 "include/linux/compat.h"
	.loc 2 81 0
	bl	free
.LVL137:
.LBE50:
.LBE49:
	.loc 1 493 0
	str	xzr, [x19, 8]
.L98:
	.loc 1 495 0
	add	w20, w23, 15
.LVL138:
.LBB51:
.LBB52:
	.loc 2 58 0
	mov	w1, 32768
.LBE52:
.LBE51:
	.loc 1 495 0
	asr	w20, w20, 4
	lsl	w20, w20, 10
	sxtw	x20, w20
.LVL139:
.LBB55:
.LBB53:
	.loc 2 58 0
	mov	x0, x20
	bl	kmalloc
.LVL140:
.LBE53:
.LBE55:
	.loc 1 495 0
	str	x0, [x19, 8]
.LBB56:
.LBB54:
	.loc 2 58 0
	mov	x26, x0
.LVL141:
.LBE54:
.LBE56:
	.loc 1 496 0
	cbnz	x0, .L99
	.loc 1 497 0
	adrp	x0, .LC5
	add	x0, x0, :lo12:.LC5
	bl	printf
.LVL142:
	.loc 1 498 0
	mov	w0, -12
	b	.L92
.L99:
	.loc 1 500 0
	ldr	x25, [x25, #:lo12:.LANCHOR2]
.LVL143:
.LBB57:
.LBB58:
	.loc 1 421 0
	ldr	w1, [x24, #:lo12:.LANCHOR4]
	cmp	w1, 16
	cset	w24, ne
.LVL144:
	.loc 1 423 0
	cbnz	x25, .L100
	.loc 1 424 0
	adrp	x0, .LC6
.LVL145:
	add	x0, x0, :lo12:.LC6
	bl	printf
.LVL146:
.L101:
.LBE58:
.LBE57:
	.loc 1 503 0
	ldr	w0, [x19]
	and	w0, w0, 65280
	.loc 1 504 0
	orr	w23, w0, w23
.LVL147:
	.loc 1 505 0
	adrp	x0, .LANCHOR8
	.loc 1 504 0
	str	w23, [x19]
	.loc 1 505 0
	str	w21, [x0, #:lo12:.LANCHOR8]
	.loc 1 506 0
	adrp	x0, .LANCHOR9
	str	w22, [x0, #:lo12:.LANCHOR9]
.LVL148:
.L119:
	.loc 1 510 0
	mov	w0, 0
	b	.L92
.LVL149:
.L100:
.LBB66:
.LBB65:
	.loc 1 428 0
	mov	x2, x20
	mov	w1, 0
	bl	memset
.LVL150:
	mov	w10, 32
	mov	w11, 1
	lsl	w10, w10, w24
.LBB59:
	.loc 1 430 0
	mov	w2, 0
	lsl	w11, w11, w24
.LBB60:
.LBB61:
	.loc 1 434 0
	mov	w13, 16
.LVL151:
.L102:
.LBE61:
.LBE60:
	.loc 1 430 0
	cmp	w23, w2
	beq	.L101
.LBB64:
.LBB62:
	.loc 1 434 0
	lsl	w1, w2, 8
	.loc 1 433 0
	lsl	w8, w2, 10
	sxtw	x8, w8
	.loc 1 434 0
	mov	w5, 0
	mov	w7, 0
	sdiv	w1, w1, w13
	sxtw	x1, w1
	b	.L105
.L103:
.LVL152:
	.loc 1 433 0
	ldrb	w4, [x3, w12, sxtw]
	.loc 1 435 0
	and	w14, w6, 30
	add	w6, w6, 2
	add	w12, w12, w11
.LVL153:
	and	w4, w4, 3
	.loc 1 432 0
	cmp	w6, 32
	.loc 1 435 0
	lsl	w4, w4, w14
	ldr	w14, [x0]
	orr	w4, w14, w4
	str	w4, [x0]
.LVL154:
	.loc 1 432 0
	bne	.L103
.LVL155:
	add	w5, w5, 16
.LVL156:
	add	w7, w7, w10
.LBE62:
	.loc 1 431 0
	cmp	w5, 256
	beq	.L104
.LVL157:
.L105:
.LBB63:
	.loc 1 434 0
	asr	w0, w5, 4
	.loc 1 433 0
	add	x3, x8, x7, sxtw
	.loc 1 434 0
	add	x0, x1, x0, sxtw
	mov	w6, 0
	add	x0, x26, x0, lsl 2
	mov	w12, 0
	.loc 1 433 0
	add	x3, x25, x3
	b	.L103
.LVL158:
.L104:
.LBE63:
.LBE64:
	.loc 1 430 0
	add	w2, w2, 1
.LVL159:
	b	.L102
.LVL160:
.L106:
	.cfi_def_cfa 31, 0
	.cfi_restore 19
	.cfi_restore 20
	.cfi_restore 21
	.cfi_restore 22
	.cfi_restore 23
	.cfi_restore 24
	.cfi_restore 25
	.cfi_restore 26
	.cfi_restore 29
	.cfi_restore 30
.LBE59:
.LBE65:
.LBE66:
	.loc 1 454 0
	mov	w0, -19
.LVL161:
	ret
.LVL162:
.L107:
	.loc 1 457 0
	mov	w0, -22
.LVL163:
	ret
	.cfi_endproc
.LFE226:
	.size	pvi_wf_get_lut, .-pvi_wf_get_lut
	.section	.text.pvi_wf_input,"ax",@progbits
	.align	2
	.global	pvi_wf_input
	.type	pvi_wf_input, %function
pvi_wf_input:
.LFB227:
	.loc 1 538 0
	.cfi_startproc
.LVL164:
	.loc 1 541 0
	adrp	x1, .LANCHOR0
	ldr	x2, [x1, #:lo12:.LANCHOR0]
	cbnz	x2, .L129
	.loc 1 538 0
	stp	x29, x30, [sp, -32]!
	.cfi_def_cfa_offset 32
	.cfi_offset 29, -32
	.cfi_offset 30, -24
	add	x29, sp, 0
	.cfi_def_cfa_register 29
	stp	x19, x20, [sp, 16]
	.cfi_offset 19, -16
	.cfi_offset 20, -8
	mov	x20, x1
	mov	x19, x0
	.loc 1 544 0
	ldrb	w1, [x0, 16]
	adrp	x0, .LC7
.LVL165:
	add	x0, x0, :lo12:.LC7
	bl	printf
.LVL166:
	.loc 1 547 0
	ldrb	w1, [x19, 16]
	cmp	w1, 32
	beq	.L123
	bhi	.L124
	cmp	w1, 22
	beq	.L123
	bhi	.L125
	cmp	w1, 9
	beq	.L126
	cmp	w1, 18
	beq	.L126
.L122:
	.loc 1 563 0
	adrp	x0, .LANCHOR6
	add	x0, x0, :lo12:.LANCHOR6
	ldrb	w2, [x0, 9]
	cmp	w2, w1
	bne	.L128
	.loc 1 564 0
	ldr	w1, [x0, 12]
	b	.L163
.L125:
	.loc 1 547 0
	sub	w0, w1, #24
	cmp	w0, 1
	bhi	.L122
.L123:
	.loc 1 555 0
	mov	w1, 32
.L163:
	.loc 1 564 0
	adrp	x0, .LANCHOR3
	str	w1, [x0, #:lo12:.LANCHOR3]
.L126:
	.loc 1 573 0
	str	x19, [x20, #:lo12:.LANCHOR0]
	.loc 1 575 0
	mov	w0, 0
	b	.L120
.L124:
	.loc 1 547 0
	cmp	w1, 72
	beq	.L123
	bhi	.L127
	cmp	w1, 35
	beq	.L126
	cmp	w1, 67
.L162:
	bne	.L122
	b	.L123
.L127:
	cmp	w1, 84
	beq	.L126
	cmp	w1, 114
	b	.L162
.L128:
	.loc 1 566 0
	ldrb	w2, [x19, 22]
	adrp	x0, .LC8
	add	x0, x0, :lo12:.LC8
	bl	printf
.LVL167:
	.loc 1 568 0
	mov	w0, -8
.L120:
	.loc 1 576 0
	ldp	x19, x20, [sp, 16]
.LVL168:
	ldp	x29, x30, [sp], 32
	.cfi_restore 30
	.cfi_restore 29
	.cfi_restore 19
	.cfi_restore 20
	.cfi_def_cfa 31, 0
	ret
.LVL169:
.L129:
	.loc 1 542 0
	mov	w0, -16
.LVL170:
	ret
	.cfi_endproc
.LFE227:
	.size	pvi_wf_input, .-pvi_wf_input
	.section	.text.pvi_wf_get_version,"ax",@progbits
	.align	2
	.global	pvi_wf_get_version
	.type	pvi_wf_get_version, %function
pvi_wf_get_version:
.LFB228:
	.loc 1 585 0
	.cfi_startproc
	.loc 1 586 0
	adrp	x0, .LANCHOR0
	ldr	x2, [x0, #:lo12:.LANCHOR0]
.LVL171:
	.loc 1 589 0
	cbz	x2, .L167
.LBB67:
	.loc 1 593 0 discriminator 3
	adrp	x0, .LANCHOR10
	add	x0, x0, :lo12:.LANCHOR10
.LBE67:
	mov	x1, 0
.L166:
.LVL172:
.LBB68:
	add	x3, x2, x1
	ldrb	w3, [x3, 65]
	strb	w3, [x1, x0]
.LVL173:
	add	x1, x1, 1
.LVL174:
	.loc 1 592 0 discriminator 3
	cmp	x1, 31
	bne	.L166
.LBE68:
	.loc 1 595 0
	strb	wzr, [x0, 31]
	.loc 1 597 0
	ret
.L167:
	.loc 1 590 0
	mov	x0, 0
	.loc 1 598 0
	ret
	.cfi_endproc
.LFE228:
	.size	pvi_wf_get_version, .-pvi_wf_get_version
	.global	custom_mode_table
	.global	pvi_mode_table
	.section	.bss.custom_mode_table,"aw",@nobits
	.align	3
	.set	.LANCHOR6,. + 0
	.type	custom_mode_table, %object
	.size	custom_mode_table, 16
custom_mode_table:
	.zero	16
	.section	.bss.fix.6288,"aw",@nobits
	.align	2
	.set	.LANCHOR9,. + 0
	.type	fix.6288, %object
	.size	fix.6288, 4
fix.6288:
	.zero	4
	.section	.bss.global_waveform,"aw",@nobits
	.align	3
	.set	.LANCHOR0,. + 0
	.type	global_waveform, %object
	.size	global_waveform, 8
global_waveform:
	.zero	8
	.section	.bss.spi_id_buffer.6314,"aw",@nobits
	.align	3
	.set	.LANCHOR10,. + 0
	.type	spi_id_buffer.6314, %object
	.size	spi_id_buffer.6314, 32
spi_id_buffer.6314:
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
	.section	.data.sftemp.6287,"aw",@progbits
	.align	2
	.set	.LANCHOR7,. + 0
	.type	sftemp.6287, %object
	.size	sftemp.6287, 4
sftemp.6287:
	.word	-1
	.section	.data.stype.6286,"aw",@progbits
	.align	2
	.set	.LANCHOR8,. + 0
	.type	stype.6286, %object
	.size	stype.6286, 4
stype.6286:
	.word	15
	.section	.rodata.__func__.6207,"a",@progbits
	.align	3
	.set	.LANCHOR1,. + 0
	.type	__func__.6207, %object
	.size	__func__.6207, 15
__func__.6207:
	.string	"get_wf_frm_num"
	.section	.rodata.get_wf_frm_num.str1.1,"aMS",@progbits,1
.LC0:
	.string	"pvi: %s %d check error\n"
.LC1:
	.string	"waveformdata is NULL\n"
.LC2:
	.string	"pvi: decodec waveform error, framenum err\n"
	.section	.rodata.get_wf_mode_index.str1.1,"aMS",@progbits,1
.LC3:
	.string	"pvi: unspport PVI waveform type"
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
.LC4:
	.string	"pvi waveform get frame number failed\n"
.LC5:
	.string	"kzalloc output->data failed\n"
.LC6:
	.string	"wf_table or lut_data is NULL\n"
	.section	.rodata.pvi_wf_input.str1.1,"aMS",@progbits,1
.LC7:
	.string	"pvi : input waveform version 0x%x\n"
.LC8:
	.string	"pvi : Unknow waveform version 0x%x, 0x%x, may be wrong waveform file\n"
	.text
.Letext0:
	.file 3 "include/common.h"
	.file 4 "/home/hzb/sdk/u-boot/arch/arm/include/asm/types.h"
	.file 5 "/home/hzb/sdk/u-boot/arch/arm/include/asm/posix_types.h"
	.file 6 "include/linux/types.h"
	.file 7 "include/errno.h"
	.file 8 "include/linux/string.h"
	.file 9 "include/efi.h"
	.file 10 "include/ide.h"
	.file 11 "include/part.h"
	.file 12 "include/flash.h"
	.file 13 "include/lmb.h"
	.file 14 "include/asm-generic/u-boot.h"
	.file 15 "/home/hzb/sdk/u-boot/arch/arm/include/asm/u-boot-arm.h"
	.file 16 "include/linux/libfdt_env.h"
	.file 17 "include/linux/../../scripts/dtc/libfdt/fdt.h"
	.file 18 "include/linux/libfdt.h"
	.file 19 "include/image.h"
	.file 20 "include/net.h"
	.file 21 "include/malloc.h"
	.file 22 "/home/hzb/sdk/ebc-dev/rk_eink/../epdlut/epd_lut.h"
	.file 23 "include/stdio.h"
	.section	.debug_info,"",@progbits
.Ldebug_info0:
	.4byte	0x1792
	.2byte	0x4
	.4byte	.Ldebug_abbrev0
	.byte	0x8
	.uleb128 0x1
	.4byte	.LASF284
	.byte	0xc
	.4byte	.LASF285
	.4byte	.LASF286
	.4byte	.Ldebug_ranges0+0x1d0
	.8byte	0
	.4byte	.Ldebug_line0
	.uleb128 0x2
	.4byte	.LASF4
	.byte	0x3
	.byte	0xd
	.4byte	0x34
	.uleb128 0x3
	.byte	0x1
	.byte	0x8
	.4byte	.LASF0
	.uleb128 0x3
	.byte	0x8
	.byte	0x7
	.4byte	.LASF1
	.uleb128 0x3
	.byte	0x2
	.byte	0x7
	.4byte	.LASF2
	.uleb128 0x4
	.4byte	.LASF23
	.byte	0x7
	.byte	0xc
	.4byte	0x54
	.uleb128 0x5
	.byte	0x4
	.byte	0x5
	.string	"int"
	.uleb128 0x3
	.byte	0x1
	.byte	0x6
	.4byte	.LASF3
	.uleb128 0x2
	.4byte	.LASF5
	.byte	0x4
	.byte	0xc
	.4byte	0x34
	.uleb128 0x3
	.byte	0x2
	.byte	0x5
	.4byte	.LASF6
	.uleb128 0x2
	.4byte	.LASF7
	.byte	0x4
	.byte	0x12
	.4byte	0x7f
	.uleb128 0x3
	.byte	0x4
	.byte	0x7
	.4byte	.LASF8
	.uleb128 0x3
	.byte	0x8
	.byte	0x5
	.4byte	.LASF9
	.uleb128 0x3
	.byte	0x8
	.byte	0x7
	.4byte	.LASF10
	.uleb128 0x6
	.string	"u8"
	.byte	0x4
	.byte	0x1f
	.4byte	0x34
	.uleb128 0x7
	.4byte	0x94
	.uleb128 0x6
	.string	"u16"
	.byte	0x4
	.byte	0x22
	.4byte	0x42
	.uleb128 0x6
	.string	"u32"
	.byte	0x4
	.byte	0x25
	.4byte	0x7f
	.uleb128 0x6
	.string	"u64"
	.byte	0x4
	.byte	0x28
	.4byte	0x8d
	.uleb128 0x2
	.4byte	.LASF11
	.byte	0x4
	.byte	0x31
	.4byte	0x8d
	.uleb128 0x2
	.4byte	.LASF12
	.byte	0x4
	.byte	0x32
	.4byte	0x8d
	.uleb128 0x3
	.byte	0x8
	.byte	0x7
	.4byte	.LASF13
	.uleb128 0x8
	.byte	0x8
	.4byte	0xee
	.uleb128 0x3
	.byte	0x1
	.byte	0x8
	.4byte	.LASF14
	.uleb128 0x7
	.4byte	0xe7
	.uleb128 0x3
	.byte	0x8
	.byte	0x5
	.4byte	.LASF15
	.uleb128 0x2
	.4byte	.LASF16
	.byte	0x5
	.byte	0x21
	.4byte	0x3b
	.uleb128 0x8
	.byte	0x8
	.4byte	0xe7
	.uleb128 0x2
	.4byte	.LASF17
	.byte	0x6
	.byte	0x35
	.4byte	0xfa
	.uleb128 0x2
	.4byte	.LASF18
	.byte	0x6
	.byte	0x59
	.4byte	0x42
	.uleb128 0x2
	.4byte	.LASF19
	.byte	0x6
	.byte	0x5b
	.4byte	0x3b
	.uleb128 0x2
	.4byte	.LASF20
	.byte	0x6
	.byte	0x69
	.4byte	0x62
	.uleb128 0x2
	.4byte	.LASF21
	.byte	0x6
	.byte	0x97
	.4byte	0x74
	.uleb128 0x2
	.4byte	.LASF22
	.byte	0x6
	.byte	0x9f
	.4byte	0x7f
	.uleb128 0x9
	.byte	0x8
	.uleb128 0x4
	.4byte	.LASF24
	.byte	0x8
	.byte	0xb
	.4byte	0x105
	.uleb128 0x3
	.byte	0x1
	.byte	0x2
	.4byte	.LASF25
	.uleb128 0xa
	.4byte	0xe7
	.4byte	0x16c
	.uleb128 0xb
	.byte	0
	.uleb128 0xc
	.4byte	.LASF26
	.byte	0x9
	.2byte	0x140
	.4byte	0x161
	.uleb128 0xc
	.4byte	.LASF27
	.byte	0x9
	.2byte	0x143
	.4byte	0x161
	.uleb128 0xc
	.4byte	.LASF28
	.byte	0x9
	.2byte	0x143
	.4byte	0x161
	.uleb128 0x8
	.byte	0x8
	.4byte	0x196
	.uleb128 0xd
	.uleb128 0xa
	.4byte	0x121
	.4byte	0x1a2
	.uleb128 0xb
	.byte	0
	.uleb128 0x4
	.4byte	.LASF29
	.byte	0xa
	.byte	0x10
	.4byte	0x197
	.uleb128 0xa
	.4byte	0x34
	.4byte	0x1bd
	.uleb128 0xe
	.4byte	0xda
	.byte	0x5
	.byte	0
	.uleb128 0xf
	.4byte	.LASF32
	.byte	0x10
	.byte	0xb
	.byte	0xf
	.4byte	0x1e2
	.uleb128 0x10
	.4byte	.LASF30
	.byte	0xb
	.byte	0x10
	.4byte	0x105
	.byte	0
	.uleb128 0x10
	.4byte	.LASF31
	.byte	0xb
	.byte	0x11
	.4byte	0x1fb
	.byte	0x8
	.byte	0
	.uleb128 0x7
	.4byte	0x1bd
	.uleb128 0x11
	.4byte	0x54
	.4byte	0x1fb
	.uleb128 0x12
	.4byte	0x54
	.uleb128 0x12
	.4byte	0x54
	.byte	0
	.uleb128 0x8
	.byte	0x8
	.4byte	0x1e7
	.uleb128 0xa
	.4byte	0x1e2
	.4byte	0x20c
	.uleb128 0xb
	.byte	0
	.uleb128 0x7
	.4byte	0x201
	.uleb128 0x4
	.4byte	.LASF32
	.byte	0xb
	.byte	0xe1
	.4byte	0x20c
	.uleb128 0x13
	.2byte	0x1220
	.byte	0xc
	.byte	0x13
	.4byte	0x270
	.uleb128 0x10
	.4byte	.LASF33
	.byte	0xc
	.byte	0x14
	.4byte	0x121
	.byte	0
	.uleb128 0x10
	.4byte	.LASF34
	.byte	0xc
	.byte	0x15
	.4byte	0x116
	.byte	0x8
	.uleb128 0x10
	.4byte	.LASF35
	.byte	0xc
	.byte	0x16
	.4byte	0x121
	.byte	0x10
	.uleb128 0x10
	.4byte	.LASF36
	.byte	0xc
	.byte	0x17
	.4byte	0x270
	.byte	0x18
	.uleb128 0x14
	.4byte	.LASF37
	.byte	0xc
	.byte	0x18
	.4byte	0x281
	.2byte	0x1018
	.uleb128 0x15
	.string	"mtd"
	.byte	0xc
	.byte	0x31
	.4byte	0x297
	.2byte	0x1218
	.byte	0
	.uleb128 0xa
	.4byte	0x121
	.4byte	0x281
	.uleb128 0x16
	.4byte	0xda
	.2byte	0x1ff
	.byte	0
	.uleb128 0xa
	.4byte	0x29
	.4byte	0x292
	.uleb128 0x16
	.4byte	0xda
	.2byte	0x1ff
	.byte	0
	.uleb128 0x17
	.4byte	.LASF287
	.uleb128 0x8
	.byte	0x8
	.4byte	0x292
	.uleb128 0x2
	.4byte	.LASF38
	.byte	0xc
	.byte	0x37
	.4byte	0x21c
	.uleb128 0xa
	.4byte	0x29d
	.4byte	0x2b3
	.uleb128 0xb
	.byte	0
	.uleb128 0x4
	.4byte	.LASF39
	.byte	0xc
	.byte	0x39
	.4byte	0x2a8
	.uleb128 0x3
	.byte	0x10
	.byte	0x4
	.4byte	.LASF40
	.uleb128 0xf
	.4byte	.LASF41
	.byte	0x10
	.byte	0xd
	.byte	0x10
	.4byte	0x2ea
	.uleb128 0x10
	.4byte	.LASF42
	.byte	0xd
	.byte	0x11
	.4byte	0xc4
	.byte	0
	.uleb128 0x10
	.4byte	.LASF33
	.byte	0xd
	.byte	0x12
	.4byte	0xcf
	.byte	0x8
	.byte	0
	.uleb128 0x18
	.4byte	.LASF43
	.2byte	0x120
	.byte	0xd
	.byte	0x15
	.4byte	0x31c
	.uleb128 0x19
	.string	"cnt"
	.byte	0xd
	.byte	0x16
	.4byte	0x3b
	.byte	0
	.uleb128 0x10
	.4byte	.LASF33
	.byte	0xd
	.byte	0x17
	.4byte	0xcf
	.byte	0x8
	.uleb128 0x10
	.4byte	.LASF44
	.byte	0xd
	.byte	0x18
	.4byte	0x31c
	.byte	0x10
	.byte	0
	.uleb128 0xa
	.4byte	0x2c5
	.4byte	0x32c
	.uleb128 0xe
	.4byte	0xda
	.byte	0x10
	.byte	0
	.uleb128 0x1a
	.string	"lmb"
	.2byte	0x240
	.byte	0xd
	.byte	0x1b
	.4byte	0x353
	.uleb128 0x10
	.4byte	.LASF45
	.byte	0xd
	.byte	0x1c
	.4byte	0x2ea
	.byte	0
	.uleb128 0x14
	.4byte	.LASF46
	.byte	0xd
	.byte	0x1d
	.4byte	0x2ea
	.2byte	0x120
	.byte	0
	.uleb128 0x1b
	.string	"lmb"
	.byte	0xd
	.byte	0x20
	.4byte	0x32c
	.uleb128 0x1c
	.byte	0x10
	.byte	0xe
	.byte	0x5b
	.4byte	0x37f
	.uleb128 0x10
	.4byte	.LASF36
	.byte	0xe
	.byte	0x5c
	.4byte	0xb9
	.byte	0
	.uleb128 0x10
	.4byte	.LASF33
	.byte	0xe
	.byte	0x5d
	.4byte	0xb9
	.byte	0x8
	.byte	0
	.uleb128 0x18
	.4byte	.LASF47
	.2byte	0x148
	.byte	0xe
	.byte	0x1b
	.4byte	0x471
	.uleb128 0x10
	.4byte	.LASF48
	.byte	0xe
	.byte	0x1c
	.4byte	0x3b
	.byte	0
	.uleb128 0x10
	.4byte	.LASF49
	.byte	0xe
	.byte	0x1d
	.4byte	0xcf
	.byte	0x8
	.uleb128 0x10
	.4byte	.LASF50
	.byte	0xe
	.byte	0x1e
	.4byte	0x3b
	.byte	0x10
	.uleb128 0x10
	.4byte	.LASF51
	.byte	0xe
	.byte	0x1f
	.4byte	0x3b
	.byte	0x18
	.uleb128 0x10
	.4byte	.LASF52
	.byte	0xe
	.byte	0x20
	.4byte	0x3b
	.byte	0x20
	.uleb128 0x10
	.4byte	.LASF53
	.byte	0xe
	.byte	0x21
	.4byte	0x3b
	.byte	0x28
	.uleb128 0x10
	.4byte	.LASF54
	.byte	0xe
	.byte	0x22
	.4byte	0x3b
	.byte	0x30
	.uleb128 0x10
	.4byte	.LASF55
	.byte	0xe
	.byte	0x24
	.4byte	0x3b
	.byte	0x38
	.uleb128 0x10
	.4byte	.LASF56
	.byte	0xe
	.byte	0x25
	.4byte	0x3b
	.byte	0x40
	.uleb128 0x10
	.4byte	.LASF57
	.byte	0xe
	.byte	0x26
	.4byte	0x3b
	.byte	0x48
	.uleb128 0x10
	.4byte	.LASF58
	.byte	0xe
	.byte	0x31
	.4byte	0x3b
	.byte	0x50
	.uleb128 0x10
	.4byte	.LASF59
	.byte	0xe
	.byte	0x32
	.4byte	0x3b
	.byte	0x58
	.uleb128 0x10
	.4byte	.LASF60
	.byte	0xe
	.byte	0x33
	.4byte	0x1ad
	.byte	0x60
	.uleb128 0x10
	.4byte	.LASF61
	.byte	0xe
	.byte	0x34
	.4byte	0x42
	.byte	0x66
	.uleb128 0x10
	.4byte	.LASF62
	.byte	0xe
	.byte	0x35
	.4byte	0x3b
	.byte	0x68
	.uleb128 0x10
	.4byte	.LASF63
	.byte	0xe
	.byte	0x36
	.4byte	0x3b
	.byte	0x70
	.uleb128 0x10
	.4byte	.LASF64
	.byte	0xe
	.byte	0x57
	.4byte	0x121
	.byte	0x78
	.uleb128 0x10
	.4byte	.LASF65
	.byte	0xe
	.byte	0x58
	.4byte	0x121
	.byte	0x80
	.uleb128 0x10
	.4byte	.LASF66
	.byte	0xe
	.byte	0x5e
	.4byte	0x471
	.byte	0x88
	.byte	0
	.uleb128 0xa
	.4byte	0x35e
	.4byte	0x481
	.uleb128 0xe
	.4byte	0xda
	.byte	0xb
	.byte	0
	.uleb128 0x2
	.4byte	.LASF67
	.byte	0xe
	.byte	0x60
	.4byte	0x37f
	.uleb128 0x4
	.4byte	.LASF68
	.byte	0xf
	.byte	0x13
	.4byte	0x121
	.uleb128 0x4
	.4byte	.LASF69
	.byte	0xf
	.byte	0x14
	.4byte	0x121
	.uleb128 0x4
	.4byte	.LASF70
	.byte	0xf
	.byte	0x15
	.4byte	0x121
	.uleb128 0x4
	.4byte	.LASF71
	.byte	0xf
	.byte	0x16
	.4byte	0x121
	.uleb128 0x4
	.4byte	.LASF72
	.byte	0xf
	.byte	0x17
	.4byte	0x121
	.uleb128 0x4
	.4byte	.LASF73
	.byte	0xf
	.byte	0x18
	.4byte	0x121
	.uleb128 0x4
	.4byte	.LASF74
	.byte	0xf
	.byte	0x19
	.4byte	0x121
	.uleb128 0x8
	.byte	0x8
	.4byte	0x34
	.uleb128 0x2
	.4byte	.LASF75
	.byte	0x10
	.byte	0x11
	.4byte	0x137
	.uleb128 0xf
	.4byte	.LASF76
	.byte	0x28
	.byte	0x11
	.byte	0x39
	.4byte	0x56f
	.uleb128 0x10
	.4byte	.LASF77
	.byte	0x11
	.byte	0x3a
	.4byte	0x4df
	.byte	0
	.uleb128 0x10
	.4byte	.LASF78
	.byte	0x11
	.byte	0x3b
	.4byte	0x4df
	.byte	0x4
	.uleb128 0x10
	.4byte	.LASF79
	.byte	0x11
	.byte	0x3c
	.4byte	0x4df
	.byte	0x8
	.uleb128 0x10
	.4byte	.LASF80
	.byte	0x11
	.byte	0x3d
	.4byte	0x4df
	.byte	0xc
	.uleb128 0x10
	.4byte	.LASF81
	.byte	0x11
	.byte	0x3e
	.4byte	0x4df
	.byte	0x10
	.uleb128 0x10
	.4byte	.LASF82
	.byte	0x11
	.byte	0x3f
	.4byte	0x4df
	.byte	0x14
	.uleb128 0x10
	.4byte	.LASF83
	.byte	0x11
	.byte	0x40
	.4byte	0x4df
	.byte	0x18
	.uleb128 0x10
	.4byte	.LASF84
	.byte	0x11
	.byte	0x43
	.4byte	0x4df
	.byte	0x1c
	.uleb128 0x10
	.4byte	.LASF85
	.byte	0x11
	.byte	0x46
	.4byte	0x4df
	.byte	0x20
	.uleb128 0x10
	.4byte	.LASF86
	.byte	0x11
	.byte	0x49
	.4byte	0x4df
	.byte	0x24
	.byte	0
	.uleb128 0xc
	.4byte	.LASF87
	.byte	0x12
	.2byte	0x136
	.4byte	0x57b
	.uleb128 0x8
	.byte	0x8
	.4byte	0x4ea
	.uleb128 0x1d
	.4byte	.LASF88
	.byte	0x40
	.byte	0x13
	.2byte	0x137
	.4byte	0x62b
	.uleb128 0x1e
	.4byte	.LASF89
	.byte	0x13
	.2byte	0x138
	.4byte	0x137
	.byte	0
	.uleb128 0x1e
	.4byte	.LASF90
	.byte	0x13
	.2byte	0x139
	.4byte	0x137
	.byte	0x4
	.uleb128 0x1e
	.4byte	.LASF91
	.byte	0x13
	.2byte	0x13a
	.4byte	0x137
	.byte	0x8
	.uleb128 0x1e
	.4byte	.LASF92
	.byte	0x13
	.2byte	0x13b
	.4byte	0x137
	.byte	0xc
	.uleb128 0x1e
	.4byte	.LASF93
	.byte	0x13
	.2byte	0x13c
	.4byte	0x137
	.byte	0x10
	.uleb128 0x1e
	.4byte	.LASF94
	.byte	0x13
	.2byte	0x13d
	.4byte	0x137
	.byte	0x14
	.uleb128 0x1e
	.4byte	.LASF95
	.byte	0x13
	.2byte	0x13e
	.4byte	0x137
	.byte	0x18
	.uleb128 0x1e
	.4byte	.LASF96
	.byte	0x13
	.2byte	0x13f
	.4byte	0x12c
	.byte	0x1c
	.uleb128 0x1e
	.4byte	.LASF97
	.byte	0x13
	.2byte	0x140
	.4byte	0x12c
	.byte	0x1d
	.uleb128 0x1e
	.4byte	.LASF98
	.byte	0x13
	.2byte	0x141
	.4byte	0x12c
	.byte	0x1e
	.uleb128 0x1e
	.4byte	.LASF99
	.byte	0x13
	.2byte	0x142
	.4byte	0x12c
	.byte	0x1f
	.uleb128 0x1e
	.4byte	.LASF100
	.byte	0x13
	.2byte	0x143
	.4byte	0x62b
	.byte	0x20
	.byte	0
	.uleb128 0xa
	.4byte	0x12c
	.4byte	0x63b
	.uleb128 0xe
	.4byte	0xda
	.byte	0x1f
	.byte	0
	.uleb128 0x1f
	.4byte	.LASF101
	.byte	0x13
	.2byte	0x144
	.4byte	0x581
	.uleb128 0x1d
	.4byte	.LASF102
	.byte	0x30
	.byte	0x13
	.2byte	0x146
	.4byte	0x6c9
	.uleb128 0x1e
	.4byte	.LASF36
	.byte	0x13
	.2byte	0x147
	.4byte	0x121
	.byte	0
	.uleb128 0x20
	.string	"end"
	.byte	0x13
	.2byte	0x147
	.4byte	0x121
	.byte	0x8
	.uleb128 0x1e
	.4byte	.LASF103
	.byte	0x13
	.2byte	0x148
	.4byte	0x121
	.byte	0x10
	.uleb128 0x1e
	.4byte	.LASF104
	.byte	0x13
	.2byte	0x148
	.4byte	0x121
	.byte	0x18
	.uleb128 0x1e
	.4byte	.LASF105
	.byte	0x13
	.2byte	0x149
	.4byte	0x121
	.byte	0x20
	.uleb128 0x1e
	.4byte	.LASF106
	.byte	0x13
	.2byte	0x14a
	.4byte	0x12c
	.byte	0x28
	.uleb128 0x1e
	.4byte	.LASF107
	.byte	0x13
	.2byte	0x14a
	.4byte	0x12c
	.byte	0x29
	.uleb128 0x20
	.string	"os"
	.byte	0x13
	.2byte	0x14a
	.4byte	0x12c
	.byte	0x2a
	.uleb128 0x1e
	.4byte	.LASF108
	.byte	0x13
	.2byte	0x14b
	.4byte	0x12c
	.byte	0x2b
	.byte	0
	.uleb128 0x1f
	.4byte	.LASF109
	.byte	0x13
	.2byte	0x14c
	.4byte	0x647
	.uleb128 0x21
	.4byte	.LASF110
	.2byte	0x380
	.byte	0x13
	.2byte	0x152
	.4byte	0x872
	.uleb128 0x1e
	.4byte	.LASF111
	.byte	0x13
	.2byte	0x158
	.4byte	0x872
	.byte	0
	.uleb128 0x1e
	.4byte	.LASF112
	.byte	0x13
	.2byte	0x159
	.4byte	0x63b
	.byte	0x8
	.uleb128 0x1e
	.4byte	.LASF113
	.byte	0x13
	.2byte	0x15a
	.4byte	0x121
	.byte	0x48
	.uleb128 0x1e
	.4byte	.LASF114
	.byte	0x13
	.2byte	0x15d
	.4byte	0xe1
	.byte	0x50
	.uleb128 0x1e
	.4byte	.LASF115
	.byte	0x13
	.2byte	0x15f
	.4byte	0x14d
	.byte	0x58
	.uleb128 0x1e
	.4byte	.LASF116
	.byte	0x13
	.2byte	0x160
	.4byte	0xe1
	.byte	0x60
	.uleb128 0x1e
	.4byte	.LASF117
	.byte	0x13
	.2byte	0x161
	.4byte	0x54
	.byte	0x68
	.uleb128 0x1e
	.4byte	.LASF118
	.byte	0x13
	.2byte	0x163
	.4byte	0x14d
	.byte	0x70
	.uleb128 0x1e
	.4byte	.LASF119
	.byte	0x13
	.2byte	0x164
	.4byte	0xe1
	.byte	0x78
	.uleb128 0x1e
	.4byte	.LASF120
	.byte	0x13
	.2byte	0x165
	.4byte	0x54
	.byte	0x80
	.uleb128 0x1e
	.4byte	.LASF121
	.byte	0x13
	.2byte	0x167
	.4byte	0x14d
	.byte	0x88
	.uleb128 0x1e
	.4byte	.LASF122
	.byte	0x13
	.2byte	0x168
	.4byte	0xe1
	.byte	0x90
	.uleb128 0x1e
	.4byte	.LASF123
	.byte	0x13
	.2byte	0x169
	.4byte	0x54
	.byte	0x98
	.uleb128 0x1e
	.4byte	.LASF124
	.byte	0x13
	.2byte	0x16b
	.4byte	0x14d
	.byte	0xa0
	.uleb128 0x1e
	.4byte	.LASF125
	.byte	0x13
	.2byte	0x16c
	.4byte	0xe1
	.byte	0xa8
	.uleb128 0x1e
	.4byte	.LASF126
	.byte	0x13
	.2byte	0x16d
	.4byte	0x54
	.byte	0xb0
	.uleb128 0x20
	.string	"os"
	.byte	0x13
	.2byte	0x171
	.4byte	0x6c9
	.byte	0xb8
	.uleb128 0x20
	.string	"ep"
	.byte	0x13
	.2byte	0x172
	.4byte	0x121
	.byte	0xe8
	.uleb128 0x1e
	.4byte	.LASF127
	.byte	0x13
	.2byte	0x174
	.4byte	0x121
	.byte	0xf0
	.uleb128 0x1e
	.4byte	.LASF128
	.byte	0x13
	.2byte	0x174
	.4byte	0x121
	.byte	0xf8
	.uleb128 0x22
	.4byte	.LASF129
	.byte	0x13
	.2byte	0x176
	.4byte	0x105
	.2byte	0x100
	.uleb128 0x22
	.4byte	.LASF130
	.byte	0x13
	.2byte	0x177
	.4byte	0x121
	.2byte	0x108
	.uleb128 0x22
	.4byte	.LASF131
	.byte	0x13
	.2byte	0x179
	.4byte	0x121
	.2byte	0x110
	.uleb128 0x22
	.4byte	.LASF132
	.byte	0x13
	.2byte	0x17a
	.4byte	0x121
	.2byte	0x118
	.uleb128 0x22
	.4byte	.LASF133
	.byte	0x13
	.2byte	0x17b
	.4byte	0x121
	.2byte	0x120
	.uleb128 0x22
	.4byte	.LASF134
	.byte	0x13
	.2byte	0x17c
	.4byte	0x121
	.2byte	0x128
	.uleb128 0x23
	.string	"kbd"
	.byte	0x13
	.2byte	0x17d
	.4byte	0x878
	.2byte	0x130
	.uleb128 0x22
	.4byte	.LASF135
	.byte	0x13
	.2byte	0x180
	.4byte	0x54
	.2byte	0x138
	.uleb128 0x22
	.4byte	.LASF136
	.byte	0x13
	.2byte	0x18d
	.4byte	0x54
	.2byte	0x13c
	.uleb128 0x23
	.string	"lmb"
	.byte	0x13
	.2byte	0x190
	.4byte	0x32c
	.2byte	0x140
	.byte	0
	.uleb128 0x8
	.byte	0x8
	.4byte	0x63b
	.uleb128 0x8
	.byte	0x8
	.4byte	0x481
	.uleb128 0x1f
	.4byte	.LASF137
	.byte	0x13
	.2byte	0x196
	.4byte	0x6d5
	.uleb128 0xc
	.4byte	.LASF138
	.byte	0x13
	.2byte	0x198
	.4byte	0x87e
	.uleb128 0xa
	.4byte	0xe7
	.4byte	0x8a7
	.uleb128 0x16
	.4byte	0xda
	.2byte	0x3ff
	.byte	0
	.uleb128 0xa
	.4byte	0xe7
	.4byte	0x8b7
	.uleb128 0xe
	.4byte	0xda
	.byte	0x1f
	.byte	0
	.uleb128 0x4
	.4byte	.LASF139
	.byte	0x3
	.byte	0xad
	.4byte	0x121
	.uleb128 0xa
	.4byte	0x94
	.4byte	0x8cd
	.uleb128 0xb
	.byte	0
	.uleb128 0x4
	.4byte	.LASF140
	.byte	0x3
	.byte	0xaf
	.4byte	0x8c2
	.uleb128 0x4
	.4byte	.LASF141
	.byte	0x3
	.byte	0xb0
	.4byte	0x8c2
	.uleb128 0x4
	.4byte	.LASF142
	.byte	0x3
	.byte	0xfe
	.4byte	0x121
	.uleb128 0x4
	.4byte	.LASF143
	.byte	0x3
	.byte	0xff
	.4byte	0x121
	.uleb128 0xc
	.4byte	.LASF144
	.byte	0x3
	.2byte	0x100
	.4byte	0x121
	.uleb128 0xf
	.4byte	.LASF145
	.byte	0x4
	.byte	0x14
	.byte	0x2e
	.4byte	0x91e
	.uleb128 0x10
	.4byte	.LASF146
	.byte	0x14
	.byte	0x2f
	.4byte	0x137
	.byte	0
	.byte	0
	.uleb128 0x8
	.byte	0x8
	.4byte	0x29
	.uleb128 0xa
	.4byte	0x94
	.4byte	0x934
	.uleb128 0xe
	.4byte	0xda
	.byte	0x5
	.byte	0
	.uleb128 0xc
	.4byte	.LASF147
	.byte	0x14
	.2byte	0x1fd
	.4byte	0x905
	.uleb128 0xc
	.4byte	.LASF148
	.byte	0x14
	.2byte	0x1fe
	.4byte	0x905
	.uleb128 0xc
	.4byte	.LASF149
	.byte	0x14
	.2byte	0x200
	.4byte	0x905
	.uleb128 0xc
	.4byte	.LASF150
	.byte	0x14
	.2byte	0x205
	.4byte	0x8a7
	.uleb128 0xc
	.4byte	.LASF151
	.byte	0x14
	.2byte	0x206
	.4byte	0x8a7
	.uleb128 0xa
	.4byte	0xe7
	.4byte	0x980
	.uleb128 0xe
	.4byte	0xda
	.byte	0x3f
	.byte	0
	.uleb128 0xc
	.4byte	.LASF152
	.byte	0x14
	.2byte	0x207
	.4byte	0x970
	.uleb128 0xc
	.4byte	.LASF153
	.byte	0x14
	.2byte	0x209
	.4byte	0x924
	.uleb128 0xc
	.4byte	.LASF154
	.byte	0x14
	.2byte	0x20a
	.4byte	0x924
	.uleb128 0xc
	.4byte	.LASF155
	.byte	0x14
	.2byte	0x20b
	.4byte	0x905
	.uleb128 0xc
	.4byte	.LASF156
	.byte	0x14
	.2byte	0x20c
	.4byte	0x905
	.uleb128 0xc
	.4byte	.LASF157
	.byte	0x14
	.2byte	0x20d
	.4byte	0x91e
	.uleb128 0xa
	.4byte	0x91e
	.4byte	0x9d8
	.uleb128 0xe
	.4byte	0xda
	.byte	0x3
	.byte	0
	.uleb128 0xc
	.4byte	.LASF158
	.byte	0x14
	.2byte	0x20e
	.4byte	0x9c8
	.uleb128 0xc
	.4byte	.LASF159
	.byte	0x14
	.2byte	0x20f
	.4byte	0x91e
	.uleb128 0xc
	.4byte	.LASF160
	.byte	0x14
	.2byte	0x210
	.4byte	0x54
	.uleb128 0xa
	.4byte	0x9e
	.4byte	0xa0c
	.uleb128 0xe
	.4byte	0xda
	.byte	0x5
	.byte	0
	.uleb128 0x7
	.4byte	0x9fc
	.uleb128 0xc
	.4byte	.LASF161
	.byte	0x14
	.2byte	0x211
	.4byte	0xa0c
	.uleb128 0xc
	.4byte	.LASF162
	.byte	0x14
	.2byte	0x212
	.4byte	0xa0c
	.uleb128 0xc
	.4byte	.LASF163
	.byte	0x14
	.2byte	0x216
	.4byte	0x116
	.uleb128 0xc
	.4byte	.LASF164
	.byte	0x14
	.2byte	0x217
	.4byte	0x116
	.uleb128 0xc
	.4byte	.LASF165
	.byte	0x14
	.2byte	0x219
	.4byte	0x54
	.uleb128 0xc
	.4byte	.LASF166
	.byte	0x14
	.2byte	0x220
	.4byte	0x896
	.uleb128 0xc
	.4byte	.LASF167
	.byte	0x14
	.2byte	0x222
	.4byte	0xae
	.uleb128 0xc
	.4byte	.LASF168
	.byte	0x14
	.2byte	0x224
	.4byte	0xae
	.uleb128 0xc
	.4byte	.LASF169
	.byte	0x14
	.2byte	0x230
	.4byte	0x905
	.uleb128 0x24
	.4byte	.LASF180
	.byte	0x4
	.4byte	0x7f
	.byte	0x14
	.2byte	0x286
	.4byte	0xaa7
	.uleb128 0x25
	.4byte	.LASF170
	.byte	0
	.uleb128 0x25
	.4byte	.LASF171
	.byte	0x1
	.uleb128 0x25
	.4byte	.LASF172
	.byte	0x2
	.uleb128 0x25
	.4byte	.LASF173
	.byte	0x3
	.byte	0
	.uleb128 0xc
	.4byte	.LASF174
	.byte	0x14
	.2byte	0x28c
	.4byte	0xa7d
	.uleb128 0xc
	.4byte	.LASF175
	.byte	0x15
	.2byte	0x3ba
	.4byte	0x121
	.uleb128 0xc
	.4byte	.LASF176
	.byte	0x15
	.2byte	0x3bb
	.4byte	0x121
	.uleb128 0xc
	.4byte	.LASF177
	.byte	0x15
	.2byte	0x3bc
	.4byte	0x121
	.uleb128 0xf
	.4byte	.LASF178
	.byte	0x4
	.byte	0x2
	.byte	0xd
	.4byte	0xaf0
	.uleb128 0x19
	.string	"pid"
	.byte	0x2
	.byte	0xe
	.4byte	0x54
	.byte	0
	.byte	0
	.uleb128 0x4
	.4byte	.LASF179
	.byte	0x2
	.byte	0x11
	.4byte	0xafb
	.uleb128 0x8
	.byte	0x8
	.4byte	0xad7
	.uleb128 0x26
	.4byte	.LASF181
	.byte	0x4
	.4byte	0x7f
	.byte	0x16
	.byte	0x11
	.4byte	0xb78
	.uleb128 0x25
	.4byte	.LASF182
	.byte	0
	.uleb128 0x25
	.4byte	.LASF183
	.byte	0x1
	.uleb128 0x25
	.4byte	.LASF184
	.byte	0x2
	.uleb128 0x25
	.4byte	.LASF185
	.byte	0x3
	.uleb128 0x25
	.4byte	.LASF186
	.byte	0x4
	.uleb128 0x25
	.4byte	.LASF187
	.byte	0x5
	.uleb128 0x25
	.4byte	.LASF188
	.byte	0x6
	.uleb128 0x25
	.4byte	.LASF189
	.byte	0x7
	.uleb128 0x25
	.4byte	.LASF190
	.byte	0x8
	.uleb128 0x25
	.4byte	.LASF191
	.byte	0x9
	.uleb128 0x25
	.4byte	.LASF192
	.byte	0xa
	.uleb128 0x25
	.4byte	.LASF193
	.byte	0xb
	.uleb128 0x25
	.4byte	.LASF194
	.byte	0xc
	.uleb128 0x25
	.4byte	.LASF195
	.byte	0xd
	.uleb128 0x25
	.4byte	.LASF196
	.byte	0xe
	.uleb128 0x25
	.4byte	.LASF197
	.byte	0xf
	.uleb128 0x25
	.4byte	.LASF198
	.byte	0x10
	.byte	0
	.uleb128 0xf
	.4byte	.LASF199
	.byte	0x20
	.byte	0x16
	.byte	0x26
	.4byte	0xba9
	.uleb128 0x10
	.4byte	.LASF200
	.byte	0x16
	.byte	0x27
	.4byte	0x7f
	.byte	0
	.uleb128 0x10
	.4byte	.LASF201
	.byte	0x16
	.byte	0x28
	.4byte	0xba9
	.byte	0x8
	.uleb128 0x10
	.4byte	.LASF202
	.byte	0x16
	.byte	0x29
	.4byte	0xbaf
	.byte	0x10
	.byte	0
	.uleb128 0x8
	.byte	0x8
	.4byte	0x7f
	.uleb128 0xa
	.4byte	0xbbf
	.4byte	0xbbf
	.uleb128 0xe
	.4byte	0xda
	.byte	0x1
	.byte	0
	.uleb128 0x8
	.byte	0x8
	.4byte	0x94
	.uleb128 0x27
	.4byte	.LASF249
	.byte	0x1
	.byte	0x15
	.4byte	0x15a
	.uleb128 0xf
	.4byte	.LASF203
	.byte	0x34
	.byte	0x1
	.byte	0x1a
	.4byte	0xcf1
	.uleb128 0x10
	.4byte	.LASF204
	.byte	0x1
	.byte	0x1b
	.4byte	0xae
	.byte	0
	.uleb128 0x10
	.4byte	.LASF205
	.byte	0x1
	.byte	0x1c
	.4byte	0xae
	.byte	0x4
	.uleb128 0x10
	.4byte	.LASF206
	.byte	0x1
	.byte	0x1d
	.4byte	0xae
	.byte	0x8
	.uleb128 0x10
	.4byte	.LASF207
	.byte	0x1
	.byte	0x1e
	.4byte	0x94
	.byte	0xc
	.uleb128 0x10
	.4byte	.LASF208
	.byte	0x1
	.byte	0x1f
	.4byte	0x94
	.byte	0xd
	.uleb128 0x10
	.4byte	.LASF209
	.byte	0x1
	.byte	0x20
	.4byte	0xa3
	.byte	0xe
	.uleb128 0x10
	.4byte	.LASF210
	.byte	0x1
	.byte	0x21
	.4byte	0x94
	.byte	0x10
	.uleb128 0x10
	.4byte	.LASF211
	.byte	0x1
	.byte	0x22
	.4byte	0x94
	.byte	0x11
	.uleb128 0x10
	.4byte	.LASF212
	.byte	0x1
	.byte	0x23
	.4byte	0x94
	.byte	0x12
	.uleb128 0x10
	.4byte	.LASF213
	.byte	0x1
	.byte	0x24
	.4byte	0x94
	.byte	0x13
	.uleb128 0x10
	.4byte	.LASF214
	.byte	0x1
	.byte	0x25
	.4byte	0x94
	.byte	0x14
	.uleb128 0x10
	.4byte	.LASF215
	.byte	0x1
	.byte	0x26
	.4byte	0x94
	.byte	0x15
	.uleb128 0x10
	.4byte	.LASF216
	.byte	0x1
	.byte	0x27
	.4byte	0x94
	.byte	0x16
	.uleb128 0x10
	.4byte	.LASF217
	.byte	0x1
	.byte	0x28
	.4byte	0x94
	.byte	0x17
	.uleb128 0x10
	.4byte	.LASF46
	.byte	0x1
	.byte	0x29
	.4byte	0xae
	.byte	0x18
	.uleb128 0x10
	.4byte	.LASF218
	.byte	0x1
	.byte	0x2a
	.4byte	0x94
	.byte	0x1c
	.uleb128 0x10
	.4byte	.LASF219
	.byte	0x1
	.byte	0x2b
	.4byte	0xcf1
	.byte	0x1d
	.uleb128 0x19
	.string	"sc1"
	.byte	0x1
	.byte	0x2c
	.4byte	0x94
	.byte	0x20
	.uleb128 0x10
	.4byte	.LASF220
	.byte	0x1
	.byte	0x2d
	.4byte	0xd01
	.byte	0x21
	.uleb128 0x10
	.4byte	.LASF221
	.byte	0x1
	.byte	0x2e
	.4byte	0x94
	.byte	0x25
	.uleb128 0x10
	.4byte	.LASF222
	.byte	0x1
	.byte	0x2f
	.4byte	0x94
	.byte	0x26
	.uleb128 0x10
	.4byte	.LASF223
	.byte	0x1
	.byte	0x30
	.4byte	0xd11
	.byte	0x27
	.uleb128 0x10
	.4byte	.LASF224
	.byte	0x1
	.byte	0x31
	.4byte	0xd21
	.byte	0x30
	.byte	0
	.uleb128 0xa
	.4byte	0x94
	.4byte	0xd01
	.uleb128 0xe
	.4byte	0xda
	.byte	0x2
	.byte	0
	.uleb128 0xa
	.4byte	0x94
	.4byte	0xd11
	.uleb128 0xe
	.4byte	0xda
	.byte	0x3
	.byte	0
	.uleb128 0xa
	.4byte	0x94
	.4byte	0xd21
	.uleb128 0xe
	.4byte	0xda
	.byte	0x8
	.byte	0
	.uleb128 0xa
	.4byte	0x94
	.4byte	0xd31
	.uleb128 0xe
	.4byte	0xda
	.byte	0
	.byte	0
	.uleb128 0x28
	.4byte	.LASF225
	.byte	0x1
	.byte	0x34
	.4byte	0xbbf
	.uleb128 0x9
	.byte	0x3
	.8byte	waveformdata
	.uleb128 0x28
	.4byte	.LASF226
	.byte	0x1
	.byte	0x35
	.4byte	0x54
	.uleb128 0x9
	.byte	0x3
	.8byte	maxpic
	.uleb128 0x28
	.4byte	.LASF227
	.byte	0x1
	.byte	0x36
	.4byte	0xd70
	.uleb128 0x9
	.byte	0x3
	.8byte	global_waveform
	.uleb128 0x8
	.byte	0x8
	.4byte	0xbd0
	.uleb128 0x28
	.4byte	.LASF228
	.byte	0x1
	.byte	0x37
	.4byte	0x54
	.uleb128 0x9
	.byte	0x3
	.8byte	need_pic
	.uleb128 0xa
	.4byte	0x9e
	.4byte	0xda1
	.uleb128 0xe
	.4byte	0xda
	.byte	0x8
	.uleb128 0xe
	.4byte	0xda
	.byte	0x8
	.byte	0
	.uleb128 0x7
	.4byte	0xd8b
	.uleb128 0x29
	.4byte	.LASF229
	.byte	0x1
	.byte	0x9e
	.4byte	0xda1
	.uleb128 0x9
	.byte	0x3
	.8byte	pvi_mode_table
	.uleb128 0xf
	.4byte	.LASF230
	.byte	0x10
	.byte	0x1
	.byte	0xaa
	.4byte	0xdec
	.uleb128 0x10
	.4byte	.LASF231
	.byte	0x1
	.byte	0xab
	.4byte	0xd11
	.byte	0
	.uleb128 0x10
	.4byte	.LASF82
	.byte	0x1
	.byte	0xac
	.4byte	0x94
	.byte	0x9
	.uleb128 0x10
	.4byte	.LASF226
	.byte	0x1
	.byte	0xad
	.4byte	0x54
	.byte	0xc
	.byte	0
	.uleb128 0x29
	.4byte	.LASF232
	.byte	0x1
	.byte	0xae
	.4byte	0xdbb
	.uleb128 0x9
	.byte	0x3
	.8byte	custom_mode_table
	.uleb128 0x2a
	.4byte	.LASF235
	.byte	0x1
	.2byte	0x248
	.4byte	0xe1
	.8byte	.LFB228
	.8byte	.LFE228-.LFB228
	.uleb128 0x1
	.byte	0x9c
	.4byte	0xe5c
	.uleb128 0x2b
	.4byte	.LASF233
	.byte	0x1
	.2byte	0x24a
	.4byte	0xe1
	.uleb128 0x1
	.byte	0x52
	.uleb128 0x2b
	.4byte	.LASF234
	.byte	0x1
	.2byte	0x24b
	.4byte	0x8a7
	.uleb128 0x9
	.byte	0x3
	.8byte	spi_id_buffer.6314
	.uleb128 0x2c
	.4byte	.Ldebug_ranges0+0x1a0
	.uleb128 0x2d
	.string	"i"
	.byte	0x1
	.2byte	0x250
	.4byte	0x54
	.4byte	.LLST47
	.byte	0
	.byte	0
	.uleb128 0x2a
	.4byte	.LASF236
	.byte	0x1
	.2byte	0x219
	.4byte	0x54
	.8byte	.LFB227
	.8byte	.LFE227-.LFB227
	.uleb128 0x1
	.byte	0x9c
	.4byte	0xed9
	.uleb128 0x2e
	.4byte	.LASF239
	.byte	0x1
	.2byte	0x219
	.4byte	0x14d
	.4byte	.LLST45
	.uleb128 0x2f
	.4byte	.LASF237
	.byte	0x1
	.2byte	0x21b
	.4byte	0xd70
	.4byte	.LLST46
	.uleb128 0x30
	.8byte	.LVL166
	.4byte	0x1767
	.4byte	0xebd
	.uleb128 0x31
	.uleb128 0x1
	.byte	0x50
	.uleb128 0x9
	.byte	0x3
	.8byte	.LC7
	.byte	0
	.uleb128 0x32
	.8byte	.LVL167
	.4byte	0x1767
	.uleb128 0x31
	.uleb128 0x1
	.byte	0x50
	.uleb128 0x9
	.byte	0x3
	.8byte	.LC8
	.byte	0
	.byte	0
	.uleb128 0x2a
	.4byte	.LASF238
	.byte	0x1
	.2byte	0x1bb
	.4byte	0x54
	.8byte	.LFB226
	.8byte	.LFE226-.LFB226
	.uleb128 0x1
	.byte	0x9c
	.4byte	0x1196
	.uleb128 0x2e
	.4byte	.LASF240
	.byte	0x1
	.2byte	0x1bb
	.4byte	0x1196
	.4byte	.LLST25
	.uleb128 0x2e
	.4byte	.LASF241
	.byte	0x1
	.2byte	0x1bb
	.4byte	0xb01
	.4byte	.LLST26
	.uleb128 0x2e
	.4byte	.LASF224
	.byte	0x1
	.2byte	0x1bb
	.4byte	0x54
	.4byte	.LLST27
	.uleb128 0x33
	.string	"pic"
	.byte	0x1
	.2byte	0x1bb
	.4byte	0x54
	.4byte	.LLST28
	.uleb128 0x2e
	.4byte	.LASF242
	.byte	0x1
	.2byte	0x1bb
	.4byte	0x54
	.4byte	.LLST29
	.uleb128 0x2b
	.4byte	.LASF243
	.byte	0x1
	.2byte	0x1bd
	.4byte	0xb01
	.uleb128 0x9
	.byte	0x3
	.8byte	stype.6286
	.uleb128 0x2b
	.4byte	.LASF244
	.byte	0x1
	.2byte	0x1be
	.4byte	0x54
	.uleb128 0x9
	.byte	0x3
	.8byte	sftemp.6287
	.uleb128 0x34
	.string	"fix"
	.byte	0x1
	.2byte	0x1bf
	.4byte	0x54
	.uleb128 0x9
	.byte	0x3
	.8byte	fix.6288
	.uleb128 0x2f
	.4byte	.LASF245
	.byte	0x1
	.2byte	0x1c0
	.4byte	0x54
	.4byte	.LLST30
	.uleb128 0x2f
	.4byte	.LASF246
	.byte	0x1
	.2byte	0x1c1
	.4byte	0x54
	.4byte	.LLST31
	.uleb128 0x2f
	.4byte	.LASF200
	.byte	0x1
	.2byte	0x1c2
	.4byte	0x54
	.4byte	.LLST32
	.uleb128 0x35
	.4byte	.LASF247
	.byte	0x1
	.2byte	0x1c3
	.4byte	0xb01
	.sleb128 -1
	.uleb128 0x36
	.4byte	0x1673
	.8byte	.LBB49
	.8byte	.LBE49-.LBB49
	.byte	0x1
	.2byte	0x1ec
	.4byte	0xffd
	.uleb128 0x37
	.4byte	0x167f
	.4byte	.LLST33
	.uleb128 0x38
	.8byte	.LVL137
	.4byte	0x1772
	.byte	0
	.uleb128 0x39
	.4byte	0x168b
	.8byte	.LBB51
	.4byte	.Ldebug_ranges0+0xc0
	.byte	0x1
	.2byte	0x1ef
	.4byte	0x1043
	.uleb128 0x37
	.4byte	0x16a6
	.4byte	.LLST34
	.uleb128 0x37
	.4byte	0x169b
	.4byte	.LLST35
	.uleb128 0x32
	.8byte	.LVL140
	.4byte	0x177e
	.uleb128 0x31
	.uleb128 0x1
	.byte	0x50
	.uleb128 0x2
	.byte	0x84
	.sleb128 0
	.uleb128 0x31
	.uleb128 0x1
	.byte	0x51
	.uleb128 0x3
	.byte	0xa
	.2byte	0x8000
	.byte	0
	.byte	0
	.uleb128 0x39
	.4byte	0x119c
	.8byte	.LBB57
	.4byte	.Ldebug_ranges0+0x100
	.byte	0x1
	.2byte	0x1f4
	.4byte	0x1112
	.uleb128 0x37
	.4byte	0x11d1
	.4byte	.LLST36
	.uleb128 0x37
	.4byte	0x11c5
	.4byte	.LLST37
	.uleb128 0x37
	.4byte	0x11b9
	.4byte	.LLST38
	.uleb128 0x37
	.4byte	0x11ad
	.4byte	.LLST39
	.uleb128 0x2c
	.4byte	.Ldebug_ranges0+0x100
	.uleb128 0x3a
	.4byte	0x11dd
	.4byte	.LLST40
	.uleb128 0x3a
	.4byte	0x11e9
	.4byte	.LLST41
	.uleb128 0x3a
	.4byte	0x11f5
	.4byte	.LLST42
	.uleb128 0x3b
	.8byte	.LBB59
	.8byte	.LBE59-.LBB59
	.4byte	0x10d8
	.uleb128 0x3a
	.4byte	0x1202
	.4byte	.LLST43
	.uleb128 0x2c
	.4byte	.Ldebug_ranges0+0x130
	.uleb128 0x3c
	.4byte	0x120d
	.uleb128 0x2c
	.4byte	.Ldebug_ranges0+0x160
	.uleb128 0x3a
	.4byte	0x121a
	.4byte	.LLST44
	.byte	0
	.byte	0
	.byte	0
	.uleb128 0x30
	.8byte	.LVL146
	.4byte	0x1767
	.4byte	0x10f7
	.uleb128 0x31
	.uleb128 0x1
	.byte	0x50
	.uleb128 0x9
	.byte	0x3
	.8byte	.LC6
	.byte	0
	.uleb128 0x32
	.8byte	.LVL150
	.4byte	0x1789
	.uleb128 0x31
	.uleb128 0x1
	.byte	0x51
	.uleb128 0x1
	.byte	0x30
	.uleb128 0x31
	.uleb128 0x1
	.byte	0x52
	.uleb128 0x2
	.byte	0x84
	.sleb128 0
	.byte	0
	.byte	0
	.byte	0
	.uleb128 0x30
	.8byte	.LVL125
	.4byte	0x14f5
	.4byte	0x112b
	.uleb128 0x31
	.uleb128 0x1
	.byte	0x50
	.uleb128 0x3
	.byte	0xf3
	.uleb128 0x1
	.byte	0x52
	.byte	0
	.uleb128 0x30
	.8byte	.LVL128
	.4byte	0x154f
	.4byte	0x1143
	.uleb128 0x31
	.uleb128 0x1
	.byte	0x50
	.uleb128 0x2
	.byte	0x85
	.sleb128 0
	.byte	0
	.uleb128 0x30
	.8byte	.LVL129
	.4byte	0x1388
	.4byte	0x115b
	.uleb128 0x31
	.uleb128 0x1
	.byte	0x51
	.uleb128 0x2
	.byte	0x84
	.sleb128 0
	.byte	0
	.uleb128 0x30
	.8byte	.LVL132
	.4byte	0x1767
	.4byte	0x117a
	.uleb128 0x31
	.uleb128 0x1
	.byte	0x50
	.uleb128 0x9
	.byte	0x3
	.8byte	.LC4
	.byte	0
	.uleb128 0x32
	.8byte	.LVL142
	.4byte	0x1767
	.uleb128 0x31
	.uleb128 0x1
	.byte	0x50
	.uleb128 0x9
	.byte	0x3
	.8byte	.LC5
	.byte	0
	.byte	0
	.uleb128 0x8
	.byte	0x8
	.4byte	0xb78
	.uleb128 0x3d
	.4byte	.LASF251
	.byte	0x1
	.2byte	0x1a3
	.4byte	0x54
	.byte	0x1
	.4byte	0x122a
	.uleb128 0x3e
	.4byte	.LASF248
	.byte	0x1
	.2byte	0x1a3
	.4byte	0x122a
	.uleb128 0x3e
	.4byte	.LASF202
	.byte	0x1
	.2byte	0x1a3
	.4byte	0xbbf
	.uleb128 0x3e
	.4byte	.LASF200
	.byte	0x1
	.2byte	0x1a3
	.4byte	0x54
	.uleb128 0x3f
	.string	"pic"
	.byte	0x1
	.2byte	0x1a3
	.4byte	0x54
	.uleb128 0x40
	.string	"lut"
	.byte	0x1
	.2byte	0x1a4
	.4byte	0x122a
	.uleb128 0x41
	.4byte	.LASF201
	.byte	0x1
	.2byte	0x1a5
	.4byte	0x94
	.uleb128 0x41
	.4byte	.LASF250
	.byte	0x1
	.2byte	0x1a5
	.4byte	0x94
	.uleb128 0x42
	.uleb128 0x40
	.string	"f"
	.byte	0x1
	.2byte	0x1ae
	.4byte	0x54
	.uleb128 0x42
	.uleb128 0x40
	.string	"old"
	.byte	0x1
	.2byte	0x1af
	.4byte	0x94
	.uleb128 0x42
	.uleb128 0x40
	.string	"new"
	.byte	0x1
	.2byte	0x1b0
	.4byte	0x94
	.byte	0
	.byte	0
	.byte	0
	.byte	0
	.uleb128 0x8
	.byte	0x8
	.4byte	0xae
	.uleb128 0x3d
	.4byte	.LASF252
	.byte	0x1
	.2byte	0x17c
	.4byte	0x54
	.byte	0x1
	.4byte	0x127c
	.uleb128 0x3e
	.4byte	.LASF202
	.byte	0x1
	.2byte	0x17c
	.4byte	0xbbf
	.uleb128 0x3e
	.4byte	.LASF200
	.byte	0x1
	.2byte	0x17c
	.4byte	0x54
	.uleb128 0x3f
	.string	"fix"
	.byte	0x1
	.2byte	0x17c
	.4byte	0x54
	.uleb128 0x40
	.string	"f"
	.byte	0x1
	.2byte	0x17e
	.4byte	0x54
	.uleb128 0x41
	.4byte	.LASF231
	.byte	0x1
	.2byte	0x17f
	.4byte	0xbbf
	.byte	0
	.uleb128 0x3d
	.4byte	.LASF253
	.byte	0x1
	.2byte	0x165
	.4byte	0x54
	.byte	0x1
	.4byte	0x12c8
	.uleb128 0x3e
	.4byte	.LASF202
	.byte	0x1
	.2byte	0x165
	.4byte	0xbbf
	.uleb128 0x3e
	.4byte	.LASF200
	.byte	0x1
	.2byte	0x165
	.4byte	0x54
	.uleb128 0x3e
	.4byte	.LASF254
	.byte	0x1
	.2byte	0x165
	.4byte	0x54
	.uleb128 0x40
	.string	"f"
	.byte	0x1
	.2byte	0x167
	.4byte	0x54
	.uleb128 0x41
	.4byte	.LASF231
	.byte	0x1
	.2byte	0x168
	.4byte	0xbbf
	.byte	0
	.uleb128 0x3d
	.4byte	.LASF255
	.byte	0x1
	.2byte	0x156
	.4byte	0x54
	.byte	0x1
	.4byte	0x1308
	.uleb128 0x3e
	.4byte	.LASF202
	.byte	0x1
	.2byte	0x156
	.4byte	0xbbf
	.uleb128 0x3e
	.4byte	.LASF200
	.byte	0x1
	.2byte	0x156
	.4byte	0x54
	.uleb128 0x40
	.string	"f"
	.byte	0x1
	.2byte	0x158
	.4byte	0x54
	.uleb128 0x41
	.4byte	.LASF231
	.byte	0x1
	.2byte	0x159
	.4byte	0xbbf
	.byte	0
	.uleb128 0x3d
	.4byte	.LASF256
	.byte	0x1
	.2byte	0x13a
	.4byte	0x54
	.byte	0x1
	.4byte	0x1348
	.uleb128 0x3e
	.4byte	.LASF202
	.byte	0x1
	.2byte	0x13a
	.4byte	0xbbf
	.uleb128 0x3e
	.4byte	.LASF200
	.byte	0x1
	.2byte	0x13a
	.4byte	0x54
	.uleb128 0x40
	.string	"f"
	.byte	0x1
	.2byte	0x13c
	.4byte	0x54
	.uleb128 0x41
	.4byte	.LASF231
	.byte	0x1
	.2byte	0x13d
	.4byte	0xbbf
	.byte	0
	.uleb128 0x3d
	.4byte	.LASF257
	.byte	0x1
	.2byte	0x11d
	.4byte	0x54
	.byte	0x1
	.4byte	0x1388
	.uleb128 0x3e
	.4byte	.LASF202
	.byte	0x1
	.2byte	0x11d
	.4byte	0xbbf
	.uleb128 0x3e
	.4byte	.LASF200
	.byte	0x1
	.2byte	0x11d
	.4byte	0x54
	.uleb128 0x40
	.string	"f"
	.byte	0x1
	.2byte	0x11f
	.4byte	0x54
	.uleb128 0x41
	.4byte	.LASF231
	.byte	0x1
	.2byte	0x120
	.4byte	0xbbf
	.byte	0
	.uleb128 0x43
	.4byte	.LASF262
	.byte	0x1
	.2byte	0x105
	.4byte	0x54
	.8byte	.LFB219
	.8byte	.LFE219-.LFB219
	.uleb128 0x1
	.byte	0x9c
	.4byte	0x14e0
	.uleb128 0x2e
	.4byte	.LASF258
	.byte	0x1
	.2byte	0x105
	.4byte	0x54
	.4byte	.LLST3
	.uleb128 0x2e
	.4byte	.LASF245
	.byte	0x1
	.2byte	0x105
	.4byte	0x54
	.4byte	.LLST4
	.uleb128 0x2f
	.4byte	.LASF233
	.byte	0x1
	.2byte	0x107
	.4byte	0x4d9
	.4byte	.LLST5
	.uleb128 0x2f
	.4byte	.LASF259
	.byte	0x1
	.2byte	0x108
	.4byte	0x4d9
	.4byte	.LLST6
	.uleb128 0x2f
	.4byte	.LASF260
	.byte	0x1
	.2byte	0x109
	.4byte	0x54
	.4byte	.LLST7
	.uleb128 0x44
	.4byte	.LASF261
	.4byte	0x14f0
	.uleb128 0x9
	.byte	0x3
	.8byte	__func__.6207
	.uleb128 0x39
	.4byte	0x1601
	.8byte	.LBB8
	.4byte	.Ldebug_ranges0+0
	.byte	0x1
	.2byte	0x11a
	.4byte	0x14b7
	.uleb128 0x37
	.4byte	0x1611
	.4byte	.LLST8
	.uleb128 0x2c
	.4byte	.Ldebug_ranges0+0
	.uleb128 0x3a
	.4byte	0x161c
	.4byte	.LLST9
	.uleb128 0x3a
	.4byte	0x1627
	.4byte	.LLST10
	.uleb128 0x3a
	.4byte	0x1632
	.4byte	.LLST11
	.uleb128 0x3a
	.4byte	0x163d
	.4byte	.LLST12
	.uleb128 0x3a
	.4byte	0x1648
	.4byte	.LLST13
	.uleb128 0x3a
	.4byte	0x1653
	.4byte	.LLST14
	.uleb128 0x3a
	.4byte	0x165c
	.4byte	.LLST15
	.uleb128 0x3a
	.4byte	0x1667
	.4byte	.LLST16
	.uleb128 0x30
	.8byte	.LVL26
	.4byte	0x1767
	.4byte	0x149a
	.uleb128 0x31
	.uleb128 0x1
	.byte	0x50
	.uleb128 0x9
	.byte	0x3
	.8byte	.LC1
	.byte	0
	.uleb128 0x32
	.8byte	.LVL69
	.4byte	0x1767
	.uleb128 0x31
	.uleb128 0x1
	.byte	0x50
	.uleb128 0x9
	.byte	0x3
	.8byte	.LC2
	.byte	0
	.byte	0
	.byte	0
	.uleb128 0x32
	.8byte	.LVL16
	.4byte	0x1767
	.uleb128 0x31
	.uleb128 0x1
	.byte	0x50
	.uleb128 0x9
	.byte	0x3
	.8byte	.LC0
	.uleb128 0x31
	.uleb128 0x1
	.byte	0x51
	.uleb128 0x9
	.byte	0x3
	.8byte	.LANCHOR1
	.byte	0
	.byte	0
	.uleb128 0xa
	.4byte	0xee
	.4byte	0x14f0
	.uleb128 0xe
	.4byte	0xda
	.byte	0xe
	.byte	0
	.uleb128 0x7
	.4byte	0x14e0
	.uleb128 0x45
	.4byte	.LASF263
	.byte	0x1
	.byte	0xf2
	.4byte	0x54
	.8byte	.LFB218
	.8byte	.LFE218-.LFB218
	.uleb128 0x1
	.byte	0x9c
	.4byte	0x154f
	.uleb128 0x46
	.4byte	.LASF224
	.byte	0x1
	.byte	0xf2
	.4byte	0x54
	.4byte	.LLST0
	.uleb128 0x28
	.4byte	.LASF264
	.byte	0x1
	.byte	0xf4
	.4byte	0x54
	.uleb128 0x1
	.byte	0x52
	.uleb128 0x47
	.4byte	.LASF265
	.byte	0x1
	.byte	0xf5
	.4byte	0x54
	.4byte	.LLST1
	.uleb128 0x48
	.string	"i"
	.byte	0x1
	.byte	0xf6
	.4byte	0x54
	.4byte	.LLST2
	.byte	0
	.uleb128 0x49
	.4byte	.LASF266
	.byte	0x1
	.byte	0xd8
	.4byte	0x94
	.byte	0x1
	.4byte	0x1581
	.uleb128 0x4a
	.4byte	.LASF241
	.byte	0x1
	.byte	0xd8
	.4byte	0xb01
	.uleb128 0x27
	.4byte	.LASF267
	.byte	0x1
	.byte	0xda
	.4byte	0x1581
	.uleb128 0x27
	.4byte	.LASF258
	.byte	0x1
	.byte	0xdb
	.4byte	0x54
	.byte	0
	.uleb128 0x8
	.byte	0x8
	.4byte	0x9e
	.uleb128 0x49
	.4byte	.LASF268
	.byte	0x1
	.byte	0xc0
	.4byte	0x1581
	.byte	0x1
	.4byte	0x15ae
	.uleb128 0x4a
	.4byte	.LASF233
	.byte	0x1
	.byte	0xc0
	.4byte	0xd70
	.uleb128 0x27
	.4byte	.LASF267
	.byte	0x1
	.byte	0xc2
	.4byte	0x1581
	.byte	0
	.uleb128 0x4b
	.4byte	.LASF269
	.byte	0x1
	.byte	0xb0
	.4byte	0x54
	.8byte	.LFB215
	.8byte	.LFE215-.LFB215
	.uleb128 0x1
	.byte	0x9c
	.4byte	0x1601
	.uleb128 0x46
	.4byte	.LASF231
	.byte	0x1
	.byte	0xb0
	.4byte	0xbbf
	.4byte	.LLST22
	.uleb128 0x46
	.4byte	.LASF33
	.byte	0x1
	.byte	0xb0
	.4byte	0x54
	.4byte	.LLST23
	.uleb128 0x2c
	.4byte	.Ldebug_ranges0+0x90
	.uleb128 0x48
	.string	"i"
	.byte	0x1
	.byte	0xb5
	.4byte	0x54
	.4byte	.LLST24
	.byte	0
	.byte	0
	.uleb128 0x49
	.4byte	.LASF270
	.byte	0x1
	.byte	0x39
	.4byte	0x54
	.byte	0x1
	.4byte	0x1673
	.uleb128 0x4a
	.4byte	.LASF201
	.byte	0x1
	.byte	0x39
	.4byte	0x4d9
	.uleb128 0x27
	.4byte	.LASF271
	.byte	0x1
	.byte	0x3b
	.4byte	0x7f
	.uleb128 0x27
	.4byte	.LASF272
	.byte	0x1
	.byte	0x3b
	.4byte	0x7f
	.uleb128 0x27
	.4byte	.LASF273
	.byte	0x1
	.byte	0x3c
	.4byte	0x7f
	.uleb128 0x27
	.4byte	.LASF260
	.byte	0x1
	.byte	0x3d
	.4byte	0x7f
	.uleb128 0x27
	.4byte	.LASF274
	.byte	0x1
	.byte	0x3e
	.4byte	0x34
	.uleb128 0x4c
	.string	"i"
	.byte	0x1
	.byte	0x3f
	.4byte	0x34
	.uleb128 0x27
	.4byte	.LASF275
	.byte	0x1
	.byte	0x40
	.4byte	0x15a
	.uleb128 0x27
	.4byte	.LASF276
	.byte	0x1
	.byte	0x41
	.4byte	0x54
	.byte	0
	.uleb128 0x4d
	.4byte	.LASF288
	.byte	0x2
	.byte	0x4f
	.byte	0x3
	.4byte	0x168b
	.uleb128 0x4a
	.4byte	.LASF277
	.byte	0x2
	.byte	0x4f
	.4byte	0x190
	.byte	0
	.uleb128 0x49
	.4byte	.LASF278
	.byte	0x2
	.byte	0x38
	.4byte	0x14d
	.byte	0x3
	.4byte	0x16b2
	.uleb128 0x4a
	.4byte	.LASF33
	.byte	0x2
	.byte	0x38
	.4byte	0x10b
	.uleb128 0x4a
	.4byte	.LASF279
	.byte	0x2
	.byte	0x38
	.4byte	0x142
	.byte	0
	.uleb128 0x4e
	.4byte	0x154f
	.8byte	.LFB217
	.8byte	.LFE217-.LFB217
	.uleb128 0x1
	.byte	0x9c
	.4byte	0x1767
	.uleb128 0x37
	.4byte	0x155f
	.4byte	.LLST17
	.uleb128 0x3c
	.4byte	0x156a
	.uleb128 0x3c
	.4byte	0x1575
	.uleb128 0x3b
	.8byte	.LBB20
	.8byte	.LBE20-.LBB20
	.4byte	0x174b
	.uleb128 0x37
	.4byte	0x155f
	.4byte	.LLST18
	.uleb128 0x4f
	.8byte	.LBB21
	.8byte	.LBE21-.LBB21
	.uleb128 0x3c
	.4byte	0x16d6
	.uleb128 0x3a
	.4byte	0x16db
	.4byte	.LLST19
	.uleb128 0x50
	.4byte	0x1587
	.8byte	.LBB22
	.4byte	.Ldebug_ranges0+0x50
	.byte	0x1
	.byte	0xed
	.uleb128 0x37
	.4byte	0x1597
	.4byte	.LLST20
	.uleb128 0x2c
	.4byte	.Ldebug_ranges0+0x50
	.uleb128 0x3a
	.4byte	0x15a2
	.4byte	.LLST21
	.byte	0
	.byte	0
	.byte	0
	.byte	0
	.uleb128 0x32
	.8byte	.LVL80
	.4byte	0x1767
	.uleb128 0x31
	.uleb128 0x1
	.byte	0x50
	.uleb128 0x9
	.byte	0x3
	.8byte	.LC3
	.byte	0
	.byte	0
	.uleb128 0x51
	.4byte	.LASF280
	.4byte	.LASF280
	.byte	0x17
	.byte	0x13
	.uleb128 0x52
	.4byte	.LASF281
	.4byte	.LASF281
	.byte	0x15
	.2byte	0x399
	.uleb128 0x51
	.4byte	.LASF282
	.4byte	.LASF282
	.byte	0x2
	.byte	0x36
	.uleb128 0x52
	.4byte	.LASF283
	.4byte	.LASF283
	.byte	0x15
	.2byte	0x16c
	.byte	0
	.section	.debug_abbrev,"",@progbits
.Ldebug_abbrev0:
	.uleb128 0x1
	.uleb128 0x11
	.byte	0x1
	.uleb128 0x25
	.uleb128 0xe
	.uleb128 0x13
	.uleb128 0xb
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x1b
	.uleb128 0xe
	.uleb128 0x55
	.uleb128 0x17
	.uleb128 0x11
	.uleb128 0x1
	.uleb128 0x10
	.uleb128 0x17
	.byte	0
	.byte	0
	.uleb128 0x2
	.uleb128 0x16
	.byte	0
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x49
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0x3
	.uleb128 0x24
	.byte	0
	.uleb128 0xb
	.uleb128 0xb
	.uleb128 0x3e
	.uleb128 0xb
	.uleb128 0x3
	.uleb128 0xe
	.byte	0
	.byte	0
	.uleb128 0x4
	.uleb128 0x34
	.byte	0
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x3f
	.uleb128 0x19
	.uleb128 0x3c
	.uleb128 0x19
	.byte	0
	.byte	0
	.uleb128 0x5
	.uleb128 0x24
	.byte	0
	.uleb128 0xb
	.uleb128 0xb
	.uleb128 0x3e
	.uleb128 0xb
	.uleb128 0x3
	.uleb128 0x8
	.byte	0
	.byte	0
	.uleb128 0x6
	.uleb128 0x16
	.byte	0
	.uleb128 0x3
	.uleb128 0x8
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x49
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0x7
	.uleb128 0x26
	.byte	0
	.uleb128 0x49
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0x8
	.uleb128 0xf
	.byte	0
	.uleb128 0xb
	.uleb128 0xb
	.uleb128 0x49
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0x9
	.uleb128 0xf
	.byte	0
	.uleb128 0xb
	.uleb128 0xb
	.byte	0
	.byte	0
	.uleb128 0xa
	.uleb128 0x1
	.byte	0x1
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x1
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0xb
	.uleb128 0x21
	.byte	0
	.byte	0
	.byte	0
	.uleb128 0xc
	.uleb128 0x34
	.byte	0
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0x5
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x3f
	.uleb128 0x19
	.uleb128 0x3c
	.uleb128 0x19
	.byte	0
	.byte	0
	.uleb128 0xd
	.uleb128 0x26
	.byte	0
	.byte	0
	.byte	0
	.uleb128 0xe
	.uleb128 0x21
	.byte	0
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x2f
	.uleb128 0xb
	.byte	0
	.byte	0
	.uleb128 0xf
	.uleb128 0x13
	.byte	0x1
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0xb
	.uleb128 0xb
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x1
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0x10
	.uleb128 0xd
	.byte	0
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x38
	.uleb128 0xb
	.byte	0
	.byte	0
	.uleb128 0x11
	.uleb128 0x15
	.byte	0x1
	.uleb128 0x27
	.uleb128 0x19
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x1
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0x12
	.uleb128 0x5
	.byte	0
	.uleb128 0x49
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0x13
	.uleb128 0x13
	.byte	0x1
	.uleb128 0xb
	.uleb128 0x5
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x1
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0x14
	.uleb128 0xd
	.byte	0
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x38
	.uleb128 0x5
	.byte	0
	.byte	0
	.uleb128 0x15
	.uleb128 0xd
	.byte	0
	.uleb128 0x3
	.uleb128 0x8
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x38
	.uleb128 0x5
	.byte	0
	.byte	0
	.uleb128 0x16
	.uleb128 0x21
	.byte	0
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x2f
	.uleb128 0x5
	.byte	0
	.byte	0
	.uleb128 0x17
	.uleb128 0x13
	.byte	0
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3c
	.uleb128 0x19
	.byte	0
	.byte	0
	.uleb128 0x18
	.uleb128 0x13
	.byte	0x1
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0xb
	.uleb128 0x5
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x1
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0x19
	.uleb128 0xd
	.byte	0
	.uleb128 0x3
	.uleb128 0x8
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x38
	.uleb128 0xb
	.byte	0
	.byte	0
	.uleb128 0x1a
	.uleb128 0x13
	.byte	0x1
	.uleb128 0x3
	.uleb128 0x8
	.uleb128 0xb
	.uleb128 0x5
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x1
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0x1b
	.uleb128 0x34
	.byte	0
	.uleb128 0x3
	.uleb128 0x8
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x3f
	.uleb128 0x19
	.uleb128 0x3c
	.uleb128 0x19
	.byte	0
	.byte	0
	.uleb128 0x1c
	.uleb128 0x13
	.byte	0x1
	.uleb128 0xb
	.uleb128 0xb
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x1
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0x1d
	.uleb128 0x13
	.byte	0x1
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0xb
	.uleb128 0xb
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0x5
	.uleb128 0x1
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0x1e
	.uleb128 0xd
	.byte	0
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0x5
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x38
	.uleb128 0xb
	.byte	0
	.byte	0
	.uleb128 0x1f
	.uleb128 0x16
	.byte	0
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0x5
	.uleb128 0x49
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0x20
	.uleb128 0xd
	.byte	0
	.uleb128 0x3
	.uleb128 0x8
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0x5
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x38
	.uleb128 0xb
	.byte	0
	.byte	0
	.uleb128 0x21
	.uleb128 0x13
	.byte	0x1
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0xb
	.uleb128 0x5
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0x5
	.uleb128 0x1
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0x22
	.uleb128 0xd
	.byte	0
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0x5
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x38
	.uleb128 0x5
	.byte	0
	.byte	0
	.uleb128 0x23
	.uleb128 0xd
	.byte	0
	.uleb128 0x3
	.uleb128 0x8
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0x5
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x38
	.uleb128 0x5
	.byte	0
	.byte	0
	.uleb128 0x24
	.uleb128 0x4
	.byte	0x1
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0xb
	.uleb128 0xb
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0x5
	.uleb128 0x1
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0x25
	.uleb128 0x28
	.byte	0
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x1c
	.uleb128 0xb
	.byte	0
	.byte	0
	.uleb128 0x26
	.uleb128 0x4
	.byte	0x1
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0xb
	.uleb128 0xb
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x1
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0x27
	.uleb128 0x34
	.byte	0
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x49
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0x28
	.uleb128 0x34
	.byte	0
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x2
	.uleb128 0x18
	.byte	0
	.byte	0
	.uleb128 0x29
	.uleb128 0x34
	.byte	0
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x3f
	.uleb128 0x19
	.uleb128 0x2
	.uleb128 0x18
	.byte	0
	.byte	0
	.uleb128 0x2a
	.uleb128 0x2e
	.byte	0x1
	.uleb128 0x3f
	.uleb128 0x19
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0x5
	.uleb128 0x27
	.uleb128 0x19
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x11
	.uleb128 0x1
	.uleb128 0x12
	.uleb128 0x7
	.uleb128 0x40
	.uleb128 0x18
	.uleb128 0x2117
	.uleb128 0x19
	.uleb128 0x1
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0x2b
	.uleb128 0x34
	.byte	0
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0x5
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x2
	.uleb128 0x18
	.byte	0
	.byte	0
	.uleb128 0x2c
	.uleb128 0xb
	.byte	0x1
	.uleb128 0x55
	.uleb128 0x17
	.byte	0
	.byte	0
	.uleb128 0x2d
	.uleb128 0x34
	.byte	0
	.uleb128 0x3
	.uleb128 0x8
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0x5
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x2
	.uleb128 0x17
	.byte	0
	.byte	0
	.uleb128 0x2e
	.uleb128 0x5
	.byte	0
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0x5
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x2
	.uleb128 0x17
	.byte	0
	.byte	0
	.uleb128 0x2f
	.uleb128 0x34
	.byte	0
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0x5
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x2
	.uleb128 0x17
	.byte	0
	.byte	0
	.uleb128 0x30
	.uleb128 0x4109
	.byte	0x1
	.uleb128 0x11
	.uleb128 0x1
	.uleb128 0x31
	.uleb128 0x13
	.uleb128 0x1
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0x31
	.uleb128 0x410a
	.byte	0
	.uleb128 0x2
	.uleb128 0x18
	.uleb128 0x2111
	.uleb128 0x18
	.byte	0
	.byte	0
	.uleb128 0x32
	.uleb128 0x4109
	.byte	0x1
	.uleb128 0x11
	.uleb128 0x1
	.uleb128 0x31
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0x33
	.uleb128 0x5
	.byte	0
	.uleb128 0x3
	.uleb128 0x8
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0x5
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x2
	.uleb128 0x17
	.byte	0
	.byte	0
	.uleb128 0x34
	.uleb128 0x34
	.byte	0
	.uleb128 0x3
	.uleb128 0x8
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0x5
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x2
	.uleb128 0x18
	.byte	0
	.byte	0
	.uleb128 0x35
	.uleb128 0x34
	.byte	0
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0x5
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x1c
	.uleb128 0xd
	.byte	0
	.byte	0
	.uleb128 0x36
	.uleb128 0x1d
	.byte	0x1
	.uleb128 0x31
	.uleb128 0x13
	.uleb128 0x11
	.uleb128 0x1
	.uleb128 0x12
	.uleb128 0x7
	.uleb128 0x58
	.uleb128 0xb
	.uleb128 0x59
	.uleb128 0x5
	.uleb128 0x1
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0x37
	.uleb128 0x5
	.byte	0
	.uleb128 0x31
	.uleb128 0x13
	.uleb128 0x2
	.uleb128 0x17
	.byte	0
	.byte	0
	.uleb128 0x38
	.uleb128 0x4109
	.byte	0
	.uleb128 0x11
	.uleb128 0x1
	.uleb128 0x31
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0x39
	.uleb128 0x1d
	.byte	0x1
	.uleb128 0x31
	.uleb128 0x13
	.uleb128 0x52
	.uleb128 0x1
	.uleb128 0x55
	.uleb128 0x17
	.uleb128 0x58
	.uleb128 0xb
	.uleb128 0x59
	.uleb128 0x5
	.uleb128 0x1
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0x3a
	.uleb128 0x34
	.byte	0
	.uleb128 0x31
	.uleb128 0x13
	.uleb128 0x2
	.uleb128 0x17
	.byte	0
	.byte	0
	.uleb128 0x3b
	.uleb128 0xb
	.byte	0x1
	.uleb128 0x11
	.uleb128 0x1
	.uleb128 0x12
	.uleb128 0x7
	.uleb128 0x1
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0x3c
	.uleb128 0x34
	.byte	0
	.uleb128 0x31
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0x3d
	.uleb128 0x2e
	.byte	0x1
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0x5
	.uleb128 0x27
	.uleb128 0x19
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x20
	.uleb128 0xb
	.uleb128 0x1
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0x3e
	.uleb128 0x5
	.byte	0
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0x5
	.uleb128 0x49
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0x3f
	.uleb128 0x5
	.byte	0
	.uleb128 0x3
	.uleb128 0x8
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0x5
	.uleb128 0x49
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0x40
	.uleb128 0x34
	.byte	0
	.uleb128 0x3
	.uleb128 0x8
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0x5
	.uleb128 0x49
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0x41
	.uleb128 0x34
	.byte	0
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0x5
	.uleb128 0x49
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0x42
	.uleb128 0xb
	.byte	0x1
	.byte	0
	.byte	0
	.uleb128 0x43
	.uleb128 0x2e
	.byte	0x1
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0x5
	.uleb128 0x27
	.uleb128 0x19
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x11
	.uleb128 0x1
	.uleb128 0x12
	.uleb128 0x7
	.uleb128 0x40
	.uleb128 0x18
	.uleb128 0x2117
	.uleb128 0x19
	.uleb128 0x1
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0x44
	.uleb128 0x34
	.byte	0
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x34
	.uleb128 0x19
	.uleb128 0x2
	.uleb128 0x18
	.byte	0
	.byte	0
	.uleb128 0x45
	.uleb128 0x2e
	.byte	0x1
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x27
	.uleb128 0x19
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x11
	.uleb128 0x1
	.uleb128 0x12
	.uleb128 0x7
	.uleb128 0x40
	.uleb128 0x18
	.uleb128 0x2117
	.uleb128 0x19
	.uleb128 0x1
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0x46
	.uleb128 0x5
	.byte	0
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x2
	.uleb128 0x17
	.byte	0
	.byte	0
	.uleb128 0x47
	.uleb128 0x34
	.byte	0
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x2
	.uleb128 0x17
	.byte	0
	.byte	0
	.uleb128 0x48
	.uleb128 0x34
	.byte	0
	.uleb128 0x3
	.uleb128 0x8
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x2
	.uleb128 0x17
	.byte	0
	.byte	0
	.uleb128 0x49
	.uleb128 0x2e
	.byte	0x1
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x27
	.uleb128 0x19
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x20
	.uleb128 0xb
	.uleb128 0x1
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0x4a
	.uleb128 0x5
	.byte	0
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x49
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0x4b
	.uleb128 0x2e
	.byte	0x1
	.uleb128 0x3f
	.uleb128 0x19
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x27
	.uleb128 0x19
	.uleb128 0x49
	.uleb128 0x13
	.uleb128 0x11
	.uleb128 0x1
	.uleb128 0x12
	.uleb128 0x7
	.uleb128 0x40
	.uleb128 0x18
	.uleb128 0x2117
	.uleb128 0x19
	.uleb128 0x1
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0x4c
	.uleb128 0x34
	.byte	0
	.uleb128 0x3
	.uleb128 0x8
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x49
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0x4d
	.uleb128 0x2e
	.byte	0x1
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.uleb128 0x27
	.uleb128 0x19
	.uleb128 0x20
	.uleb128 0xb
	.uleb128 0x1
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0x4e
	.uleb128 0x2e
	.byte	0x1
	.uleb128 0x31
	.uleb128 0x13
	.uleb128 0x11
	.uleb128 0x1
	.uleb128 0x12
	.uleb128 0x7
	.uleb128 0x40
	.uleb128 0x18
	.uleb128 0x2117
	.uleb128 0x19
	.uleb128 0x1
	.uleb128 0x13
	.byte	0
	.byte	0
	.uleb128 0x4f
	.uleb128 0xb
	.byte	0x1
	.uleb128 0x11
	.uleb128 0x1
	.uleb128 0x12
	.uleb128 0x7
	.byte	0
	.byte	0
	.uleb128 0x50
	.uleb128 0x1d
	.byte	0x1
	.uleb128 0x31
	.uleb128 0x13
	.uleb128 0x52
	.uleb128 0x1
	.uleb128 0x55
	.uleb128 0x17
	.uleb128 0x58
	.uleb128 0xb
	.uleb128 0x59
	.uleb128 0xb
	.byte	0
	.byte	0
	.uleb128 0x51
	.uleb128 0x2e
	.byte	0
	.uleb128 0x3f
	.uleb128 0x19
	.uleb128 0x3c
	.uleb128 0x19
	.uleb128 0x6e
	.uleb128 0xe
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0xb
	.byte	0
	.byte	0
	.uleb128 0x52
	.uleb128 0x2e
	.byte	0
	.uleb128 0x3f
	.uleb128 0x19
	.uleb128 0x3c
	.uleb128 0x19
	.uleb128 0x6e
	.uleb128 0xe
	.uleb128 0x3
	.uleb128 0xe
	.uleb128 0x3a
	.uleb128 0xb
	.uleb128 0x3b
	.uleb128 0x5
	.byte	0
	.byte	0
	.byte	0
	.section	.debug_loc,"",@progbits
.Ldebug_loc0:
.LLST47:
	.8byte	.LVL172
	.8byte	.LVL173
	.2byte	0x1
	.byte	0x51
	.8byte	.LVL173
	.8byte	.LVL174
	.2byte	0x3
	.byte	0x71
	.sleb128 1
	.byte	0x9f
	.8byte	0
	.8byte	0
.LLST45:
	.8byte	.LVL164
	.8byte	.LVL165
	.2byte	0x1
	.byte	0x50
	.8byte	.LVL165
	.8byte	.LVL168
	.2byte	0x1
	.byte	0x63
	.8byte	.LVL168
	.8byte	.LVL169
	.2byte	0x4
	.byte	0xf3
	.uleb128 0x1
	.byte	0x50
	.byte	0x9f
	.8byte	.LVL169
	.8byte	.LVL170
	.2byte	0x1
	.byte	0x50
	.8byte	.LVL170
	.8byte	.LFE227
	.2byte	0x4
	.byte	0xf3
	.uleb128 0x1
	.byte	0x50
	.byte	0x9f
	.8byte	0
	.8byte	0
.LLST46:
	.8byte	.LVL164
	.8byte	.LVL165
	.2byte	0x1
	.byte	0x50
	.8byte	.LVL165
	.8byte	.LVL168
	.2byte	0x1
	.byte	0x63
	.8byte	.LVL168
	.8byte	.LVL169
	.2byte	0x4
	.byte	0xf3
	.uleb128 0x1
	.byte	0x50
	.byte	0x9f
	.8byte	.LVL169
	.8byte	.LVL170
	.2byte	0x1
	.byte	0x50
	.8byte	.LVL170
	.8byte	.LFE227
	.2byte	0x4
	.byte	0xf3
	.uleb128 0x1
	.byte	0x50
	.byte	0x9f
	.8byte	0
	.8byte	0
.LLST25:
	.8byte	.LVL121
	.8byte	.LVL122
	.2byte	0x1
	.byte	0x50
	.8byte	.LVL122
	.8byte	.LVL123
	.2byte	0x4
	.byte	0xf3
	.uleb128 0x1
	.byte	0x50
	.byte	0x9f
	.8byte	.LVL123
	.8byte	.LVL124
	.2byte	0x1
	.byte	0x50
	.8byte	.LVL124
	.8byte	.LVL133
	.2byte	0x1
	.byte	0x63
	.8byte	.LVL133
	.8byte	.LVL134
	.2byte	0x4
	.byte	0xf3
	.uleb128 0x1
	.byte	0x50
	.byte	0x9f
	.8byte	.LVL134
	.8byte	.LVL148
	.2byte	0x1
	.byte	0x63
	.8byte	.LVL148
	.8byte	.LVL149
	.2byte	0x4
	.byte	0xf3
	.uleb128 0x1
	.byte	0x50
	.byte	0x9f
	.8byte	.LVL149
	.8byte	.LVL160
	.2byte	0x1
	.byte	0x63
	.8byte	.LVL160
	.8byte	.LVL161
	.2byte	0x1
	.byte	0x50
	.8byte	.LVL161
	.8byte	.LVL162
	.2byte	0x4
	.byte	0xf3
	.uleb128 0x1
	.byte	0x50
	.byte	0x9f
	.8byte	.LVL162
	.8byte	.LVL163
	.2byte	0x1
	.byte	0x50
	.8byte	.LVL163
	.8byte	.LFE226
	.2byte	0x4
	.byte	0xf3
	.uleb128 0x1
	.byte	0x50
	.byte	0x9f
	.8byte	0
	.8byte	0
.LLST26:
	.8byte	.LVL121
	.8byte	.LVL125-1
	.2byte	0x1
	.byte	0x51
	.8byte	.LVL125-1
	.8byte	.LVL133
	.2byte	0x1
	.byte	0x65
	.8byte	.LVL133
	.8byte	.LVL134
	.2byte	0x4
	.byte	0xf3
	.uleb128 0x1
	.byte	0x51
	.byte	0x9f
	.8byte	.LVL134
	.8byte	.LVL148
	.2byte	0x1
	.byte	0x65
	.8byte	.LVL148
	.8byte	.LVL149
	.2byte	0x4
	.byte	0xf3
	.uleb128 0x1
	.byte	0x51
	.byte	0x9f
	.8byte	.LVL149
	.8byte	.LVL160
	.2byte	0x1
	.byte	0x65
	.8byte	.LVL160
	.8byte	.LFE226
	.2byte	0x1
	.byte	0x51
	.8byte	0
	.8byte	0
.LLST27:
	.8byte	.LVL121
	.8byte	.LVL125-1
	.2byte	0x1
	.byte	0x52
	.8byte	.LVL125-1
	.8byte	.LVL160
	.2byte	0x4
	.byte	0xf3
	.uleb128 0x1
	.byte	0x52
	.byte	0x9f
	.8byte	.LVL160
	.8byte	.LFE226
	.2byte	0x1
	.byte	0x52
	.8byte	0
	.8byte	0
.LLST28:
	.8byte	.LVL121
	.8byte	.LVL125-1
	.2byte	0x1
	.byte	0x53
	.8byte	.LVL125-1
	.8byte	.LVL160
	.2byte	0x4
	.byte	0xf3
	.uleb128 0x1
	.byte	0x53
	.byte	0x9f
	.8byte	.LVL160
	.8byte	.LFE226
	.2byte	0x1
	.byte	0x53
	.8byte	0
	.8byte	0
.LLST29:
	.8byte	.LVL121
	.8byte	.LVL125-1
	.2byte	0x1
	.byte	0x54
	.8byte	.LVL125-1
	.8byte	.LVL133
	.2byte	0x1
	.byte	0x66
	.8byte	.LVL133
	.8byte	.LVL134
	.2byte	0x4
	.byte	0xf3
	.uleb128 0x1
	.byte	0x54
	.byte	0x9f
	.8byte	.LVL134
	.8byte	.LVL148
	.2byte	0x1
	.byte	0x66
	.8byte	.LVL148
	.8byte	.LVL149
	.2byte	0x4
	.byte	0xf3
	.uleb128 0x1
	.byte	0x54
	.byte	0x9f
	.8byte	.LVL149
	.8byte	.LVL160
	.2byte	0x1
	.byte	0x66
	.8byte	.LVL160
	.8byte	.LFE226
	.2byte	0x1
	.byte	0x54
	.8byte	0
	.8byte	0
.LLST30:
	.8byte	.LVL126
	.8byte	.LVL127
	.2byte	0x1
	.byte	0x50
	.8byte	.LVL127
	.8byte	.LVL133
	.2byte	0x1
	.byte	0x64
	.8byte	.LVL134
	.8byte	.LVL138
	.2byte	0x1
	.byte	0x64
	.8byte	0
	.8byte	0
.LLST31:
	.8byte	.LVL128
	.8byte	.LVL129-1
	.2byte	0x6
	.byte	0x70
	.sleb128 0
	.byte	0x8
	.byte	0xff
	.byte	0x1a
	.byte	0x9f
	.8byte	0
	.8byte	0
.LLST32:
	.8byte	.LVL130
	.8byte	.LVL131
	.2byte	0x1
	.byte	0x50
	.8byte	.LVL131
	.8byte	.LVL133
	.2byte	0x1
	.byte	0x67
	.8byte	.LVL134
	.8byte	.LVL135
	.2byte	0x1
	.byte	0x50
	.8byte	.LVL135
	.8byte	.LVL147
	.2byte	0x1
	.byte	0x67
	.8byte	.LVL149
	.8byte	.LVL160
	.2byte	0x1
	.byte	0x67
	.8byte	0
	.8byte	0
.LLST33:
	.8byte	.LVL136
	.8byte	.LVL137-1
	.2byte	0x1
	.byte	0x50
	.8byte	0
	.8byte	0
.LLST34:
	.8byte	.LVL139
	.8byte	.LVL141
	.2byte	0x2
	.byte	0x30
	.byte	0x9f
	.8byte	0
	.8byte	0
.LLST35:
	.8byte	.LVL139
	.8byte	.LVL141
	.2byte	0x1
	.byte	0x64
	.8byte	0
	.8byte	0
.LLST36:
	.8byte	.LVL143
	.8byte	.LVL146-1
	.2byte	0x9
	.byte	0x3
	.8byte	need_pic
	.8byte	.LVL149
	.8byte	.LVL150-1
	.2byte	0x9
	.byte	0x3
	.8byte	need_pic
	.8byte	0
	.8byte	0
.LLST37:
	.8byte	.LVL143
	.8byte	.LVL147
	.2byte	0x1
	.byte	0x67
	.8byte	.LVL149
	.8byte	.LVL160
	.2byte	0x1
	.byte	0x67
	.8byte	0
	.8byte	0
.LLST38:
	.8byte	.LVL143
	.8byte	.LVL148
	.2byte	0x1
	.byte	0x69
	.8byte	.LVL149
	.8byte	.LVL160
	.2byte	0x1
	.byte	0x69
	.8byte	0
	.8byte	0
.LLST39:
	.8byte	.LVL143
	.8byte	.LVL145
	.2byte	0x1
	.byte	0x50
	.8byte	.LVL145
	.8byte	.LVL148
	.2byte	0x1
	.byte	0x6a
	.8byte	.LVL149
	.8byte	.LVL150-1
	.2byte	0x1
	.byte	0x50
	.8byte	.LVL150-1
	.8byte	.LVL160
	.2byte	0x1
	.byte	0x6a
	.8byte	0
	.8byte	0
.LLST40:
	.8byte	.LVL152
	.8byte	.LVL156
	.2byte	0x13
	.byte	0x75
	.sleb128 0
	.byte	0x34
	.byte	0x26
	.byte	0x8
	.byte	0x20
	.byte	0x24
	.byte	0x8
	.byte	0x20
	.byte	0x26
	.byte	0x71
	.sleb128 0
	.byte	0x22
	.byte	0x32
	.byte	0x24
	.byte	0x8a
	.sleb128 0
	.byte	0x22
	.byte	0x9f
	.8byte	.LVL156
	.8byte	.LVL157
	.2byte	0x13
	.byte	0x75
	.sleb128 -16
	.byte	0x34
	.byte	0x26
	.byte	0x8
	.byte	0x20
	.byte	0x24
	.byte	0x8
	.byte	0x20
	.byte	0x26
	.byte	0x71
	.sleb128 0
	.byte	0x22
	.byte	0x32
	.byte	0x24
	.byte	0x8a
	.sleb128 0
	.byte	0x22
	.byte	0x9f
	.8byte	.LVL158
	.8byte	.LVL160
	.2byte	0x13
	.byte	0x75
	.sleb128 -16
	.byte	0x34
	.byte	0x26
	.byte	0x8
	.byte	0x20
	.byte	0x24
	.byte	0x8
	.byte	0x20
	.byte	0x26
	.byte	0x71
	.sleb128 0
	.byte	0x22
	.byte	0x32
	.byte	0x24
	.byte	0x8a
	.sleb128 0
	.byte	0x22
	.byte	0x9f
	.8byte	0
	.8byte	0
.LLST41:
	.8byte	.LVL152
	.8byte	.LVL153
	.2byte	0x1c
	.byte	0x7c
	.sleb128 0
	.byte	0x8
	.byte	0x20
	.byte	0x24
	.byte	0x8
	.byte	0x20
	.byte	0x26
	.byte	0x77
	.sleb128 0
	.byte	0x8
	.byte	0x20
	.byte	0x24
	.byte	0x8
	.byte	0x20
	.byte	0x26
	.byte	0x22
	.byte	0x89
	.sleb128 0
	.byte	0x22
	.byte	0x78
	.sleb128 0
	.byte	0x22
	.byte	0x94
	.byte	0x1
	.byte	0x33
	.byte	0x1a
	.byte	0x9f
	.8byte	0
	.8byte	0
.LLST42:
	.8byte	.LVL144
	.8byte	.LVL148
	.2byte	0x1
	.byte	0x68
	.8byte	.LVL149
	.8byte	.LVL160
	.2byte	0x1
	.byte	0x68
	.8byte	0
	.8byte	0
.LLST43:
	.8byte	.LVL150
	.8byte	.LVL151
	.2byte	0x2
	.byte	0x30
	.byte	0x9f
	.8byte	.LVL151
	.8byte	.LVL160
	.2byte	0x1
	.byte	0x52
	.8byte	0
	.8byte	0
.LLST44:
	.8byte	.LVL157
	.8byte	.LVL158
	.2byte	0x2
	.byte	0x30
	.byte	0x9f
	.8byte	0
	.8byte	0
.LLST3:
	.8byte	.LVL9
	.8byte	.LVL10
	.2byte	0x1
	.byte	0x50
	.8byte	.LVL10
	.8byte	.LFE219
	.2byte	0x4
	.byte	0xf3
	.uleb128 0x1
	.byte	0x50
	.byte	0x9f
	.8byte	0
	.8byte	0
.LLST4:
	.8byte	.LVL9
	.8byte	.LVL15
	.2byte	0x1
	.byte	0x51
	.8byte	.LVL15
	.8byte	.LVL17
	.2byte	0x4
	.byte	0xf3
	.uleb128 0x1
	.byte	0x51
	.byte	0x9f
	.8byte	.LVL17
	.8byte	.LVL19
	.2byte	0x1
	.byte	0x51
	.8byte	.LVL19
	.8byte	.LFE219
	.2byte	0x4
	.byte	0xf3
	.uleb128 0x1
	.byte	0x51
	.byte	0x9f
	.8byte	0
	.8byte	0
.LLST5:
	.8byte	.LVL11
	.8byte	.LVL15
	.2byte	0x1
	.byte	0x52
	.8byte	.LVL15
	.8byte	.LVL16-1
	.2byte	0x9
	.byte	0x3
	.8byte	global_waveform
	.8byte	.LVL17
	.8byte	.LVL23
	.2byte	0x1
	.byte	0x52
	.8byte	.LVL23
	.8byte	.LVL24
	.2byte	0x9
	.byte	0x3
	.8byte	global_waveform
	.8byte	.LVL24
	.8byte	.LVL26-1
	.2byte	0x1
	.byte	0x52
	.8byte	.LVL27
	.8byte	.LVL29
	.2byte	0x1
	.byte	0x52
	.8byte	.LVL29
	.8byte	.LVL44
	.2byte	0x9
	.byte	0x3
	.8byte	global_waveform
	.8byte	.LVL44
	.8byte	.LVL45
	.2byte	0x1
	.byte	0x52
	.8byte	.LVL45
	.8byte	.LVL69-1
	.2byte	0x9
	.byte	0x3
	.8byte	global_waveform
	.8byte	0
	.8byte	0
.LLST6:
	.8byte	.LVL11
	.8byte	.LVL13
	.2byte	0x2
	.byte	0x30
	.byte	0x9f
	.8byte	.LVL13
	.8byte	.LVL14
	.2byte	0x1
	.byte	0x54
	.8byte	.LVL14
	.8byte	.LVL15
	.2byte	0x6
	.byte	0x72
	.sleb128 0
	.byte	0x70
	.sleb128 0
	.byte	0x22
	.byte	0x9f
	.8byte	.LVL17
	.8byte	.LVL18
	.2byte	0x6
	.byte	0x72
	.sleb128 0
	.byte	0x70
	.sleb128 0
	.byte	0x22
	.byte	0x9f
	.8byte	.LVL18
	.8byte	.LVL21
	.2byte	0x17
	.byte	0xf3
	.uleb128 0x1
	.byte	0x50
	.byte	0x32
	.byte	0x24
	.byte	0x8
	.byte	0x20
	.byte	0x24
	.byte	0x8
	.byte	0x20
	.byte	0x26
	.byte	0x72
	.sleb128 32
	.byte	0x94
	.byte	0x1
	.byte	0x8
	.byte	0xff
	.byte	0x1a
	.byte	0x22
	.byte	0x72
	.sleb128 0
	.byte	0x22
	.byte	0x9f
	.8byte	.LVL21
	.8byte	.LVL22
	.2byte	0x1
	.byte	0x53
	.8byte	.LVL22
	.8byte	.LVL23
	.2byte	0x6
	.byte	0x72
	.sleb128 0
	.byte	0x71
	.sleb128 0
	.byte	0x22
	.byte	0x9f
	.8byte	.LVL23
	.8byte	.LVL24
	.2byte	0xe
	.byte	0x3
	.8byte	global_waveform
	.byte	0x6
	.byte	0x71
	.sleb128 0
	.byte	0x22
	.byte	0x9f
	.8byte	.LVL24
	.8byte	.LVL25
	.2byte	0x6
	.byte	0x72
	.sleb128 0
	.byte	0x71
	.sleb128 0
	.byte	0x22
	.byte	0x9f
	.8byte	.LVL25
	.8byte	.LVL26-1
	.2byte	0x1
	.byte	0x58
	.8byte	.LVL27
	.8byte	.LVL69-1
	.2byte	0x1
	.byte	0x58
	.8byte	0
	.8byte	0
.LLST7:
	.8byte	.LVL11
	.8byte	.LVL12
	.2byte	0x8
	.byte	0x72
	.sleb128 32
	.byte	0x94
	.byte	0x1
	.byte	0x8
	.byte	0xff
	.byte	0x1a
	.byte	0x9f
	.8byte	.LVL17
	.8byte	.LVL18
	.2byte	0xe
	.byte	0x72
	.sleb128 0
	.byte	0x70
	.sleb128 0
	.byte	0x22
	.byte	0x94
	.byte	0x4
	.byte	0xc
	.4byte	0xffffff
	.byte	0x1a
	.byte	0x9f
	.8byte	.LVL18
	.8byte	.LVL20
	.2byte	0x9
	.byte	0x70
	.sleb128 0
	.byte	0xc
	.4byte	0xffffff
	.byte	0x1a
	.byte	0x9f
	.8byte	.LVL20
	.8byte	.LVL23
	.2byte	0x1f
	.byte	0xf3
	.uleb128 0x1
	.byte	0x50
	.byte	0x32
	.byte	0x24
	.byte	0x8
	.byte	0x20
	.byte	0x24
	.byte	0x8
	.byte	0x20
	.byte	0x26
	.byte	0x72
	.sleb128 32
	.byte	0x94
	.byte	0x1
	.byte	0x8
	.byte	0xff
	.byte	0x1a
	.byte	0x22
	.byte	0x72
	.sleb128 0
	.byte	0x22
	.byte	0x94
	.byte	0x4
	.byte	0xc
	.4byte	0xffffff
	.byte	0x1a
	.byte	0x9f
	.8byte	.LVL23
	.8byte	.LVL24
	.2byte	0x31
	.byte	0xf3
	.uleb128 0x1
	.byte	0x50
	.byte	0x32
	.byte	0x24
	.byte	0x8
	.byte	0x20
	.byte	0x24
	.byte	0x8
	.byte	0x20
	.byte	0x26
	.byte	0x3
	.8byte	global_waveform
	.byte	0x6
	.byte	0x23
	.uleb128 0x20
	.byte	0x94
	.byte	0x1
	.byte	0x8
	.byte	0xff
	.byte	0x1a
	.byte	0x22
	.byte	0x3
	.8byte	global_waveform
	.byte	0x6
	.byte	0x22
	.byte	0x94
	.byte	0x4
	.byte	0xc
	.4byte	0xffffff
	.byte	0x1a
	.byte	0x9f
	.8byte	.LVL24
	.8byte	.LVL26-1
	.2byte	0xe
	.byte	0x72
	.sleb128 0
	.byte	0x71
	.sleb128 0
	.byte	0x22
	.byte	0x94
	.byte	0x4
	.byte	0xc
	.4byte	0xffffff
	.byte	0x1a
	.byte	0x9f
	.8byte	.LVL27
	.8byte	.LVL29
	.2byte	0xe
	.byte	0x72
	.sleb128 0
	.byte	0x71
	.sleb128 0
	.byte	0x22
	.byte	0x94
	.byte	0x4
	.byte	0xc
	.4byte	0xffffff
	.byte	0x1a
	.byte	0x9f
	.8byte	.LVL29
	.8byte	.LVL30
	.2byte	0x16
	.byte	0x3
	.8byte	global_waveform
	.byte	0x6
	.byte	0x71
	.sleb128 0
	.byte	0x22
	.byte	0x94
	.byte	0x4
	.byte	0xc
	.4byte	0xffffff
	.byte	0x1a
	.byte	0x9f
	.8byte	.LVL30
	.8byte	.LVL31
	.2byte	0x56
	.byte	0xf3
	.uleb128 0x1
	.byte	0x51
	.byte	0x32
	.byte	0x24
	.byte	0x8
	.byte	0x20
	.byte	0x24
	.byte	0x8
	.byte	0x20
	.byte	0x26
	.byte	0xf3
	.uleb128 0x1
	.byte	0x50
	.byte	0x32
	.byte	0x24
	.byte	0x8
	.byte	0x20
	.byte	0x24
	.byte	0x8
	.byte	0x20
	.byte	0x26
	.byte	0x3
	.8byte	global_waveform
	.byte	0x6
	.byte	0x23
	.uleb128 0x20
	.byte	0x94
	.byte	0x1
	.byte	0x8
	.byte	0xff
	.byte	0x1a
	.byte	0x22
	.byte	0x3
	.8byte	global_waveform
	.byte	0x6
	.byte	0x22
	.byte	0x94
	.byte	0x4
	.byte	0xc
	.4byte	0xffffff
	.byte	0x1a
	.byte	0xc
	.4byte	0xffffffff
	.byte	0x1a
	.byte	0x22
	.byte	0x3
	.8byte	global_waveform
	.byte	0x6
	.byte	0x22
	.byte	0x94
	.byte	0x4
	.byte	0xc
	.4byte	0xffffff
	.byte	0x1a
	.byte	0x9f
	.8byte	.LVL44
	.8byte	.LVL45
	.2byte	0xe
	.byte	0x72
	.sleb128 0
	.byte	0x71
	.sleb128 0
	.byte	0x22
	.byte	0x94
	.byte	0x4
	.byte	0xc
	.4byte	0xffffff
	.byte	0x1a
	.byte	0x9f
	.8byte	0
	.8byte	0
.LLST8:
	.8byte	.LVL25
	.8byte	.LVL26-1
	.2byte	0x1
	.byte	0x58
	.8byte	.LVL27
	.8byte	.LVL69-1
	.2byte	0x1
	.byte	0x58
	.8byte	0
	.8byte	0
.LLST9:
	.8byte	.LVL25
	.8byte	.LVL31
	.2byte	0x2
	.byte	0x30
	.byte	0x9f
	.8byte	.LVL35
	.8byte	.LVL36
	.2byte	0x1
	.byte	0x51
	.8byte	.LVL36
	.8byte	.LVL41
	.2byte	0x2
	.byte	0x30
	.byte	0x9f
	.8byte	.LVL44
	.8byte	.LVL45
	.2byte	0x2
	.byte	0x30
	.byte	0x9f
	.8byte	.LVL46
	.8byte	.LVL47
	.2byte	0x1
	.byte	0x54
	.8byte	.LVL47
	.8byte	.LVL48
	.2byte	0x3
	.byte	0x71
	.sleb128 3
	.byte	0x9f
	.8byte	.LVL48
	.8byte	.LVL49
	.2byte	0x3
	.byte	0x71
	.sleb128 -1
	.byte	0x9f
	.8byte	.LVL50
	.8byte	.LVL55
	.2byte	0x2
	.byte	0x30
	.byte	0x9f
	.8byte	.LVL60
	.8byte	.LVL62
	.2byte	0x1
	.byte	0x51
	.8byte	.LVL62
	.8byte	.LVL67
	.2byte	0x2
	.byte	0x30
	.byte	0x9f
	.8byte	.LVL71
	.8byte	.LFE219
	.2byte	0x1
	.byte	0x52
	.8byte	0
	.8byte	0
.LLST10:
	.8byte	.LVL25
	.8byte	.LVL31
	.2byte	0x2
	.byte	0x30
	.byte	0x9f
	.8byte	.LVL31
	.8byte	.LVL38
	.2byte	0x1
	.byte	0x50
	.8byte	.LVL38
	.8byte	.LVL40
	.2byte	0x2
	.byte	0x30
	.byte	0x9f
	.8byte	.LVL41
	.8byte	.LVL44
	.2byte	0x1
	.byte	0x50
	.8byte	.LVL44
	.8byte	.LVL45
	.2byte	0x2
	.byte	0x30
	.byte	0x9f
	.8byte	.LVL45
	.8byte	.LVL52
	.2byte	0x1
	.byte	0x50
	.8byte	.LVL52
	.8byte	.LVL54
	.2byte	0x2
	.byte	0x30
	.byte	0x9f
	.8byte	.LVL55
	.8byte	.LVL64
	.2byte	0x1
	.byte	0x50
	.8byte	.LVL64
	.8byte	.LVL66
	.2byte	0x2
	.byte	0x30
	.byte	0x9f
	.8byte	.LVL67
	.8byte	.LVL68
	.2byte	0x1
	.byte	0x50
	.8byte	.LVL71
	.8byte	.LVL72
	.2byte	0x2
	.byte	0x30
	.byte	0x9f
	.8byte	.LVL72
	.8byte	.LFE219
	.2byte	0x1
	.byte	0x51
	.8byte	0
	.8byte	0
.LLST11:
	.8byte	.LVL25
	.8byte	.LVL31
	.2byte	0x2
	.byte	0x30
	.byte	0x9f
	.8byte	.LVL31
	.8byte	.LVL44
	.2byte	0x1
	.byte	0x63
	.8byte	.LVL44
	.8byte	.LVL45
	.2byte	0x2
	.byte	0x30
	.byte	0x9f
	.8byte	.LVL45
	.8byte	.LFE219
	.2byte	0x1
	.byte	0x63
	.8byte	0
	.8byte	0
.LLST12:
	.8byte	.LVL25
	.8byte	.LVL31
	.2byte	0x2
	.byte	0x30
	.byte	0x9f
	.8byte	.LVL31
	.8byte	.LVL35
	.2byte	0x1
	.byte	0x5b
	.8byte	.LVL35
	.8byte	.LVL43
	.2byte	0x1
	.byte	0x5f
	.8byte	.LVL43
	.8byte	.LVL44
	.2byte	0x1
	.byte	0x5b
	.8byte	.LVL44
	.8byte	.LVL45
	.2byte	0x2
	.byte	0x30
	.byte	0x9f
	.8byte	.LVL45
	.8byte	.LVL58
	.2byte	0x1
	.byte	0x5f
	.8byte	.LVL58
	.8byte	.LVL59
	.2byte	0x1
	.byte	0x5b
	.8byte	.LVL59
	.8byte	.LVL61
	.2byte	0x3
	.byte	0x7f
	.sleb128 -1
	.byte	0x9f
	.8byte	.LVL61
	.8byte	.LVL62
	.2byte	0x1
	.byte	0x5f
	.8byte	.LVL62
	.8byte	.LVL69-1
	.2byte	0x1
	.byte	0x5b
	.8byte	0
	.8byte	0
.LLST13:
	.8byte	.LVL34
	.8byte	.LVL44
	.2byte	0x1
	.byte	0x67
	.8byte	.LVL45
	.8byte	.LVL58
	.2byte	0x1
	.byte	0x67
	.8byte	.LVL59
	.8byte	.LVL61
	.2byte	0x1
	.byte	0x5b
	.8byte	.LVL73
	.8byte	.LFE219
	.2byte	0x1
	.byte	0x58
	.8byte	0
	.8byte	0
.LLST14:
	.8byte	.LVL42
	.8byte	.LVL44
	.2byte	0x1
	.byte	0x56
	.8byte	.LVL45
	.8byte	.LVL56
	.2byte	0x1
	.byte	0x56
	.8byte	.LVL57
	.8byte	.LVL58
	.2byte	0x1
	.byte	0x56
	.8byte	.LVL70
	.8byte	.LVL76
	.2byte	0x1
	.byte	0x50
	.8byte	.LVL77
	.8byte	.LFE219
	.2byte	0x1
	.byte	0x50
	.8byte	0
	.8byte	0
.LLST15:
	.8byte	.LVL25
	.8byte	.LVL31
	.2byte	0x2
	.byte	0x31
	.byte	0x9f
	.8byte	.LVL31
	.8byte	.LVL44
	.2byte	0x1
	.byte	0x5e
	.8byte	.LVL44
	.8byte	.LVL45
	.2byte	0x2
	.byte	0x31
	.byte	0x9f
	.8byte	.LVL45
	.8byte	.LVL69-1
	.2byte	0x1
	.byte	0x5e
	.8byte	0
	.8byte	0
.LLST16:
	.8byte	.LVL25
	.8byte	.LVL28
	.2byte	0x2
	.byte	0x30
	.byte	0x9f
	.8byte	.LVL31
	.8byte	.LVL44
	.2byte	0x1
	.byte	0x5d
	.8byte	.LVL44
	.8byte	.LVL45
	.2byte	0x2
	.byte	0x30
	.byte	0x9f
	.8byte	.LVL45
	.8byte	.LVL69-1
	.2byte	0x1
	.byte	0x5d
	.8byte	0
	.8byte	0
.LLST0:
	.8byte	.LVL0
	.8byte	.LVL1
	.2byte	0x1
	.byte	0x50
	.8byte	.LVL1
	.8byte	.LFE218
	.2byte	0x1
	.byte	0x54
	.8byte	0
	.8byte	0
.LLST1:
	.8byte	.LVL2
	.8byte	.LVL4
	.2byte	0x3
	.byte	0x9
	.byte	0xff
	.byte	0x9f
	.8byte	.LVL4
	.8byte	.LVL5
	.2byte	0x1
	.byte	0x50
	.8byte	.LVL5
	.8byte	.LFE218
	.2byte	0x3
	.byte	0x9
	.byte	0xff
	.byte	0x9f
	.8byte	0
	.8byte	0
.LLST2:
	.8byte	.LVL3
	.8byte	.LVL4
	.2byte	0x1
	.byte	0x50
	.8byte	.LVL4
	.8byte	.LVL5
	.2byte	0x1
	.byte	0x51
	.8byte	.LVL5
	.8byte	.LVL6
	.2byte	0x1
	.byte	0x50
	.8byte	.LVL7
	.8byte	.LVL8
	.2byte	0x1
	.byte	0x50
	.8byte	.LVL8
	.8byte	.LFE218
	.2byte	0x1
	.byte	0x51
	.8byte	0
	.8byte	0
.LLST22:
	.8byte	.LVL113
	.8byte	.LVL118
	.2byte	0x1
	.byte	0x50
	.8byte	.LVL118
	.8byte	.LVL119
	.2byte	0x4
	.byte	0xf3
	.uleb128 0x1
	.byte	0x50
	.byte	0x9f
	.8byte	.LVL119
	.8byte	.LVL120
	.2byte	0x1
	.byte	0x50
	.8byte	.LVL120
	.8byte	.LFE215
	.2byte	0x4
	.byte	0xf3
	.uleb128 0x1
	.byte	0x50
	.byte	0x9f
	.8byte	0
	.8byte	0
.LLST23:
	.8byte	.LVL113
	.8byte	.LVL114
	.2byte	0x1
	.byte	0x51
	.8byte	.LVL114
	.8byte	.LVL119
	.2byte	0x4
	.byte	0xf3
	.uleb128 0x1
	.byte	0x51
	.byte	0x9f
	.8byte	.LVL119
	.8byte	.LFE215
	.2byte	0x1
	.byte	0x51
	.8byte	0
	.8byte	0
.LLST24:
	.8byte	.LVL115
	.8byte	.LVL116
	.2byte	0x1
	.byte	0x52
	.8byte	.LVL116
	.8byte	.LVL117
	.2byte	0x3
	.byte	0x72
	.sleb128 1
	.byte	0x9f
	.8byte	0
	.8byte	0
.LLST17:
	.8byte	.LVL78
	.8byte	.LVL79
	.2byte	0x1
	.byte	0x50
	.8byte	.LVL79
	.8byte	.LVL81
	.2byte	0x4
	.byte	0xf3
	.uleb128 0x1
	.byte	0x50
	.byte	0x9f
	.8byte	.LVL81
	.8byte	.LVL82
	.2byte	0x1
	.byte	0x50
	.8byte	.LVL82
	.8byte	.LVL85
	.2byte	0x4
	.byte	0xf3
	.uleb128 0x1
	.byte	0x50
	.byte	0x9f
	.8byte	.LVL85
	.8byte	.LVL86
	.2byte	0x1
	.byte	0x50
	.8byte	.LVL86
	.8byte	.LVL87
	.2byte	0x4
	.byte	0xf3
	.uleb128 0x1
	.byte	0x50
	.byte	0x9f
	.8byte	.LVL87
	.8byte	.LVL88
	.2byte	0x1
	.byte	0x50
	.8byte	.LVL88
	.8byte	.LVL89
	.2byte	0x4
	.byte	0xf3
	.uleb128 0x1
	.byte	0x50
	.byte	0x9f
	.8byte	.LVL89
	.8byte	.LVL90
	.2byte	0x1
	.byte	0x50
	.8byte	.LVL90
	.8byte	.LVL91
	.2byte	0x4
	.byte	0xf3
	.uleb128 0x1
	.byte	0x50
	.byte	0x9f
	.8byte	.LVL91
	.8byte	.LVL92
	.2byte	0x1
	.byte	0x50
	.8byte	.LVL92
	.8byte	.LFE217
	.2byte	0x4
	.byte	0xf3
	.uleb128 0x1
	.byte	0x50
	.byte	0x9f
	.8byte	0
	.8byte	0
.LLST18:
	.8byte	.LVL81
	.8byte	.LVL82
	.2byte	0x1
	.byte	0x50
	.8byte	.LVL82
	.8byte	.LVL85
	.2byte	0x4
	.byte	0xf3
	.uleb128 0x1
	.byte	0x50
	.byte	0x9f
	.8byte	.LVL85
	.8byte	.LVL86
	.2byte	0x1
	.byte	0x50
	.8byte	.LVL86
	.8byte	.LVL87
	.2byte	0x4
	.byte	0xf3
	.uleb128 0x1
	.byte	0x50
	.byte	0x9f
	.8byte	.LVL87
	.8byte	.LVL88
	.2byte	0x1
	.byte	0x50
	.8byte	.LVL88
	.8byte	.LVL89
	.2byte	0x4
	.byte	0xf3
	.uleb128 0x1
	.byte	0x50
	.byte	0x9f
	.8byte	.LVL89
	.8byte	.LVL90
	.2byte	0x1
	.byte	0x50
	.8byte	.LVL90
	.8byte	.LVL91
	.2byte	0x4
	.byte	0xf3
	.uleb128 0x1
	.byte	0x50
	.byte	0x9f
	.8byte	.LVL91
	.8byte	.LVL92
	.2byte	0x1
	.byte	0x50
	.8byte	.LVL92
	.8byte	.LVL102
	.2byte	0x4
	.byte	0xf3
	.uleb128 0x1
	.byte	0x50
	.byte	0x9f
	.8byte	.LVL103
	.8byte	.LFE217
	.2byte	0x4
	.byte	0xf3
	.uleb128 0x1
	.byte	0x50
	.byte	0x9f
	.8byte	0
	.8byte	0
.LLST19:
	.8byte	.LVL82
	.8byte	.LVL85
	.2byte	0x1
	.byte	0x50
	.8byte	.LVL93
	.8byte	.LVL102
	.2byte	0x1
	.byte	0x50
	.8byte	.LVL103
	.8byte	.LFE217
	.2byte	0x1
	.byte	0x50
	.8byte	0
	.8byte	0
.LLST20:
	.8byte	.LVL83
	.8byte	.LVL84
	.2byte	0x1
	.byte	0x51
	.8byte	.LVL84
	.8byte	.LVL85
	.2byte	0x9
	.byte	0x3
	.8byte	global_waveform
	.8byte	.LVL93
	.8byte	.LVL95
	.2byte	0x1
	.byte	0x51
	.8byte	.LVL95
	.8byte	.LVL96
	.2byte	0x9
	.byte	0x3
	.8byte	global_waveform
	.8byte	.LVL96
	.8byte	.LVL98
	.2byte	0x1
	.byte	0x51
	.8byte	.LVL98
	.8byte	.LVL99
	.2byte	0x9
	.byte	0x3
	.8byte	global_waveform
	.8byte	.LVL99
	.8byte	.LVL100
	.2byte	0x1
	.byte	0x51
	.8byte	.LVL100
	.8byte	.LVL103
	.2byte	0x9
	.byte	0x3
	.8byte	global_waveform
	.8byte	.LVL103
	.8byte	.LVL104
	.2byte	0x1
	.byte	0x51
	.8byte	.LVL104
	.8byte	.LVL105
	.2byte	0x9
	.byte	0x3
	.8byte	global_waveform
	.8byte	.LVL105
	.8byte	.LVL106
	.2byte	0x1
	.byte	0x51
	.8byte	.LVL106
	.8byte	.LVL107
	.2byte	0x9
	.byte	0x3
	.8byte	global_waveform
	.8byte	.LVL107
	.8byte	.LVL108
	.2byte	0x1
	.byte	0x51
	.8byte	.LVL108
	.8byte	.LVL109
	.2byte	0x9
	.byte	0x3
	.8byte	global_waveform
	.8byte	.LVL109
	.8byte	.LVL110
	.2byte	0x1
	.byte	0x51
	.8byte	.LVL110
	.8byte	.LVL111
	.2byte	0x9
	.byte	0x3
	.8byte	global_waveform
	.8byte	.LVL111
	.8byte	.LVL112
	.2byte	0x1
	.byte	0x51
	.8byte	.LVL112
	.8byte	.LFE217
	.2byte	0x9
	.byte	0x3
	.8byte	global_waveform
	.8byte	0
	.8byte	0
.LLST21:
	.8byte	.LVL83
	.8byte	.LVL85
	.2byte	0x2
	.byte	0x30
	.byte	0x9f
	.8byte	.LVL93
	.8byte	.LVL94
	.2byte	0x2
	.byte	0x30
	.byte	0x9f
	.8byte	.LVL94
	.8byte	.LVL96
	.2byte	0xa
	.byte	0x3
	.8byte	pvi_mode_table+27
	.byte	0x9f
	.8byte	.LVL96
	.8byte	.LVL97
	.2byte	0x2
	.byte	0x30
	.byte	0x9f
	.8byte	.LVL97
	.8byte	.LVL99
	.2byte	0xa
	.byte	0x3
	.8byte	pvi_mode_table+63
	.byte	0x9f
	.8byte	.LVL99
	.8byte	.LVL101
	.2byte	0x2
	.byte	0x30
	.byte	0x9f
	.8byte	.LVL103
	.8byte	.LVL105
	.2byte	0xa
	.byte	0x3
	.8byte	pvi_mode_table+18
	.byte	0x9f
	.8byte	.LVL105
	.8byte	.LVL107
	.2byte	0xa
	.byte	0x3
	.8byte	pvi_mode_table+36
	.byte	0x9f
	.8byte	.LVL107
	.8byte	.LVL109
	.2byte	0xa
	.byte	0x3
	.8byte	pvi_mode_table+45
	.byte	0x9f
	.8byte	.LVL109
	.8byte	.LVL111
	.2byte	0xa
	.byte	0x3
	.8byte	pvi_mode_table+54
	.byte	0x9f
	.8byte	.LVL111
	.8byte	.LFE217
	.2byte	0x2
	.byte	0x30
	.byte	0x9f
	.8byte	0
	.8byte	0
	.section	.debug_aranges,"",@progbits
	.4byte	0x8c
	.2byte	0x2
	.4byte	.Ldebug_info0
	.byte	0x8
	.byte	0
	.2byte	0
	.2byte	0
	.8byte	.LFB218
	.8byte	.LFE218-.LFB218
	.8byte	.LFB219
	.8byte	.LFE219-.LFB219
	.8byte	.LFB217
	.8byte	.LFE217-.LFB217
	.8byte	.LFB215
	.8byte	.LFE215-.LFB215
	.8byte	.LFB226
	.8byte	.LFE226-.LFB226
	.8byte	.LFB227
	.8byte	.LFE227-.LFB227
	.8byte	.LFB228
	.8byte	.LFE228-.LFB228
	.8byte	0
	.8byte	0
	.section	.debug_ranges,"",@progbits
.Ldebug_ranges0:
	.8byte	.LBB8
	.8byte	.LBE8
	.8byte	.LBB13
	.8byte	.LBE13
	.8byte	.LBB14
	.8byte	.LBE14
	.8byte	.LBB15
	.8byte	.LBE15
	.8byte	0
	.8byte	0
	.8byte	.LBB22
	.8byte	.LBE22
	.8byte	.LBB26
	.8byte	.LBE26
	.8byte	.LBB27
	.8byte	.LBE27
	.8byte	0
	.8byte	0
	.8byte	.LBB28
	.8byte	.LBE28
	.8byte	.LBB29
	.8byte	.LBE29
	.8byte	0
	.8byte	0
	.8byte	.LBB51
	.8byte	.LBE51
	.8byte	.LBB55
	.8byte	.LBE55
	.8byte	.LBB56
	.8byte	.LBE56
	.8byte	0
	.8byte	0
	.8byte	.LBB57
	.8byte	.LBE57
	.8byte	.LBB66
	.8byte	.LBE66
	.8byte	0
	.8byte	0
	.8byte	.LBB60
	.8byte	.LBE60
	.8byte	.LBB64
	.8byte	.LBE64
	.8byte	0
	.8byte	0
	.8byte	.LBB61
	.8byte	.LBE61
	.8byte	.LBB62
	.8byte	.LBE62
	.8byte	.LBB63
	.8byte	.LBE63
	.8byte	0
	.8byte	0
	.8byte	.LBB67
	.8byte	.LBE67
	.8byte	.LBB68
	.8byte	.LBE68
	.8byte	0
	.8byte	0
	.8byte	.LFB218
	.8byte	.LFE218
	.8byte	.LFB219
	.8byte	.LFE219
	.8byte	.LFB217
	.8byte	.LFE217
	.8byte	.LFB215
	.8byte	.LFE215
	.8byte	.LFB226
	.8byte	.LFE226
	.8byte	.LFB227
	.8byte	.LFE227
	.8byte	.LFB228
	.8byte	.LFE228
	.8byte	0
	.8byte	0
	.section	.debug_line,"",@progbits
.Ldebug_line0:
	.section	.debug_str,"MS",@progbits,1
.LASF265:
	.string	"level"
.LASF81:
	.string	"off_mem_rsvmap"
.LASF17:
	.string	"size_t"
.LASF131:
	.string	"initrd_start"
.LASF13:
	.string	"sizetype"
.LASF151:
	.string	"net_hostname"
.LASF36:
	.string	"start"
.LASF171:
	.string	"NETLOOP_RESTART"
.LASF238:
	.string	"pvi_wf_get_lut"
.LASF186:
	.string	"WF_TYPE_GL16"
.LASF236:
	.string	"pvi_wf_input"
.LASF89:
	.string	"ih_magic"
.LASF62:
	.string	"bi_intfreq"
.LASF261:
	.string	"__func__"
.LASF11:
	.string	"phys_addr_t"
.LASF5:
	.string	"__u8"
.LASF170:
	.string	"NETLOOP_CONTINUE"
.LASF38:
	.string	"flash_info_t"
.LASF106:
	.string	"comp"
.LASF101:
	.string	"image_header_t"
.LASF136:
	.string	"state"
.LASF56:
	.string	"bi_dsp_freq"
.LASF103:
	.string	"image_start"
.LASF47:
	.string	"bd_info"
.LASF93:
	.string	"ih_load"
.LASF141:
	.string	"__dtb_dt_spl_begin"
.LASF205:
	.string	"file_length"
.LASF213:
	.string	"wf_type"
.LASF7:
	.string	"__u32"
.LASF263:
	.string	"get_wf_temp_index"
.LASF199:
	.string	"epd_lut_data"
.LASF157:
	.string	"net_tx_packet"
.LASF156:
	.string	"net_server_ip"
.LASF164:
	.string	"net_native_vlan"
.LASF252:
	.string	"pvi_wf_normal_fix"
.LASF182:
	.string	"WF_TYPE_RESET"
.LASF284:
	.ascii	"GNU C11 6.3.1 20170404 -ms"
	.string	"trict-align -march=armv8-a+nosimd -mlittle-endian -mabi=lp64 -g -Os -fno-builtin -ffreestanding -fshort-wchar -fno-stack-protector -fno-delete-null-pointer-checks -fstack-usage -fno-pic -ffunction-sections -fdata-sections -ffixed-r9 -fno-common -ffixed-x18"
.LASF23:
	.string	"errno"
.LASF15:
	.string	"long int"
.LASF51:
	.string	"bi_flashsize"
.LASF74:
	.string	"IRQ_STACK_START_IN"
.LASF85:
	.string	"size_dt_strings"
.LASF250:
	.string	"shift"
.LASF271:
	.string	"oldpic"
.LASF167:
	.string	"net_boot_file_size"
.LASF95:
	.string	"ih_dcrc"
.LASF61:
	.string	"bi_ethspeed"
.LASF262:
	.string	"get_wf_frm_num"
.LASF29:
	.string	"ide_bus_offset"
.LASF154:
	.string	"net_server_ethaddr"
.LASF64:
	.string	"bi_arch_number"
.LASF3:
	.string	"signed char"
.LASF20:
	.string	"uint8_t"
.LASF225:
	.string	"waveformdata"
.LASF97:
	.string	"ih_arch"
.LASF78:
	.string	"totalsize"
.LASF273:
	.string	"frame_tmp"
.LASF90:
	.string	"ih_hcrc"
.LASF105:
	.string	"load"
.LASF185:
	.string	"WF_TYPE_GC16"
.LASF41:
	.string	"lmb_property"
.LASF83:
	.string	"last_comp_version"
.LASF0:
	.string	"unsigned char"
.LASF138:
	.string	"images"
.LASF256:
	.string	"pvi_wf_gray2_repair"
.LASF57:
	.string	"bi_ddr_freq"
.LASF92:
	.string	"ih_size"
.LASF269:
	.string	"pvi_wf_add_custom_mode_table"
.LASF104:
	.string	"image_len"
.LASF251:
	.string	"wf_table_to_4bit_wf_lut_data"
.LASF25:
	.string	"_Bool"
.LASF231:
	.string	"table"
.LASF234:
	.string	"spi_id_buffer"
.LASF200:
	.string	"frame_num"
.LASF14:
	.string	"char"
.LASF27:
	.string	"_binary_u_boot_bin_start"
.LASF172:
	.string	"NETLOOP_SUCCESS"
.LASF240:
	.string	"output"
.LASF178:
	.string	"p_current"
.LASF180:
	.string	"net_loop_state"
.LASF276:
	.string	"shift_n"
.LASF121:
	.string	"fit_hdr_fdt"
.LASF165:
	.string	"net_restart_wrap"
.LASF153:
	.string	"net_ethaddr"
.LASF279:
	.string	"flags"
.LASF270:
	.string	"decodewaveform"
.LASF98:
	.string	"ih_type"
.LASF79:
	.string	"off_dt_struct"
.LASF52:
	.string	"bi_flashoffset"
.LASF166:
	.string	"net_boot_file_name"
.LASF91:
	.string	"ih_time"
.LASF259:
	.string	"wf_data"
.LASF102:
	.string	"image_info"
.LASF108:
	.string	"arch"
.LASF31:
	.string	"select_hwpart"
.LASF158:
	.string	"net_rx_packets"
.LASF175:
	.string	"mem_malloc_start"
.LASF73:
	.string	"_datarelro_start_ofs"
.LASF19:
	.string	"ulong"
.LASF94:
	.string	"ih_ep"
.LASF43:
	.string	"lmb_region"
.LASF230:
	.string	"pvi_custom_mode_table"
.LASF233:
	.string	"waveform"
.LASF280:
	.string	"printf"
.LASF116:
	.string	"fit_uname_os"
.LASF195:
	.string	"WF_TYPE_RK_GL16"
.LASF58:
	.string	"bi_bootflags"
.LASF155:
	.string	"net_ip"
.LASF149:
	.string	"net_dns_server"
.LASF169:
	.string	"net_ping_ip"
.LASF45:
	.string	"memory"
.LASF76:
	.string	"fdt_header"
.LASF176:
	.string	"mem_malloc_end"
.LASF222:
	.string	"temperaturenumber"
.LASF128:
	.string	"rd_end"
.LASF133:
	.string	"cmdline_start"
.LASF224:
	.string	"temperture"
.LASF278:
	.string	"kzalloc"
.LASF1:
	.string	"long unsigned int"
.LASF150:
	.string	"net_nis_domain"
.LASF184:
	.string	"WF_TYPE_GRAY4"
.LASF123:
	.string	"fit_noffset_fdt"
.LASF173:
	.string	"NETLOOP_FAIL"
.LASF226:
	.string	"maxpic"
.LASF53:
	.string	"bi_sramstart"
.LASF28:
	.string	"_binary_u_boot_bin_end"
.LASF84:
	.string	"boot_cpuid_phys"
.LASF119:
	.string	"fit_uname_rd"
.LASF246:
	.string	"mode_index"
.LASF114:
	.string	"fit_uname_cfg"
.LASF163:
	.string	"net_our_vlan"
.LASF206:
	.string	"serial"
.LASF235:
	.string	"pvi_wf_get_version"
.LASF109:
	.string	"image_info_t"
.LASF191:
	.string	"PVI_WF_MAX"
.LASF218:
	.string	"vcom_offset"
.LASF227:
	.string	"global_waveform"
.LASF219:
	.string	"xwia"
.LASF286:
	.string	"/home/hzb/sdk/u-boot"
.LASF59:
	.string	"bi_ip_addr"
.LASF112:
	.string	"legacy_hdr_os_copy"
.LASF9:
	.string	"long long int"
.LASF187:
	.string	"WF_TYPE_GLR16"
.LASF202:
	.string	"wf_table"
.LASF24:
	.string	"___strtok"
.LASF168:
	.string	"net_boot_file_expected_size_in_blocks"
.LASF16:
	.string	"__kernel_size_t"
.LASF110:
	.string	"bootm_headers"
.LASF37:
	.string	"protect"
.LASF162:
	.string	"net_null_ethaddr"
.LASF221:
	.string	"modenumber"
.LASF70:
	.string	"_datarel_start_ofs"
.LASF142:
	.string	"load_addr"
.LASF115:
	.string	"fit_hdr_os"
.LASF71:
	.string	"_datarelrolocal_start_ofs"
.LASF196:
	.string	"WF_TYPE_RK_GC16"
.LASF207:
	.string	"run_type"
.LASF247:
	.string	"overlay_lut"
.LASF8:
	.string	"unsigned int"
.LASF18:
	.string	"ushort"
.LASF214:
	.string	"panel_size"
.LASF216:
	.string	"wfm_rev"
.LASF174:
	.string	"net_state"
.LASF245:
	.string	"temp_index"
.LASF229:
	.string	"pvi_mode_table"
.LASF111:
	.string	"legacy_hdr_os"
.LASF80:
	.string	"off_dt_strings"
.LASF139:
	.string	"monitor_flash_len"
.LASF49:
	.string	"bi_memsize"
.LASF220:
	.string	"unknown1"
.LASF223:
	.string	"unknown2"
.LASF26:
	.string	"image_base"
.LASF179:
	.string	"current"
.LASF260:
	.string	"offset"
.LASF275:
	.string	"israr"
.LASF277:
	.string	"block"
.LASF137:
	.string	"bootm_headers_t"
.LASF55:
	.string	"bi_arm_freq"
.LASF107:
	.string	"type"
.LASF118:
	.string	"fit_hdr_rd"
.LASF54:
	.string	"bi_sramsize"
.LASF40:
	.string	"long double"
.LASF255:
	.string	"pvi_wf_regal_fix"
.LASF274:
	.string	"tempdata"
.LASF127:
	.string	"rd_start"
.LASF147:
	.string	"net_gateway"
.LASF217:
	.string	"frame_rate"
.LASF144:
	.string	"save_size"
.LASF204:
	.string	"checksum"
.LASF253:
	.string	"pvi_wf_rpart_fix"
.LASF100:
	.string	"ih_name"
.LASF96:
	.string	"ih_os"
.LASF188:
	.string	"WF_TYPE_GLD16"
.LASF134:
	.string	"cmdline_end"
.LASF159:
	.string	"net_rx_packet"
.LASF193:
	.string	"WF_TYPE_RK_GLR16"
.LASF33:
	.string	"size"
.LASF10:
	.string	"long long unsigned int"
.LASF267:
	.string	"pvi_modes"
.LASF88:
	.string	"image_header"
.LASF183:
	.string	"WF_TYPE_GRAY2"
.LASF21:
	.string	"__be32"
.LASF272:
	.string	"newpic"
.LASF50:
	.string	"bi_flashstart"
.LASF117:
	.string	"fit_noffset_os"
.LASF197:
	.string	"WF_TYPE_MAX"
.LASF283:
	.string	"memset"
.LASF39:
	.string	"flash_info"
.LASF82:
	.string	"version"
.LASF44:
	.string	"region"
.LASF86:
	.string	"size_dt_struct"
.LASF75:
	.string	"fdt32_t"
.LASF241:
	.string	"lut_type"
.LASF60:
	.string	"bi_enetaddr"
.LASF208:
	.string	"fpl_platform"
.LASF287:
	.string	"mtd_info"
.LASF145:
	.string	"in_addr"
.LASF160:
	.string	"net_rx_packet_len"
.LASF243:
	.string	"stype"
.LASF215:
	.string	"amepd_part_number"
.LASF140:
	.string	"__dtb_dt_begin"
.LASF264:
	.string	"temp_num"
.LASF120:
	.string	"fit_noffset_rd"
.LASF12:
	.string	"phys_size_t"
.LASF69:
	.string	"FIQ_STACK_START"
.LASF135:
	.string	"verify"
.LASF30:
	.string	"name"
.LASF268:
	.string	"parse_mode_version"
.LASF282:
	.string	"kmalloc"
.LASF211:
	.string	"wf_version"
.LASF237:
	.string	"pvi_wf"
.LASF63:
	.string	"bi_busfreq"
.LASF6:
	.string	"short int"
.LASF258:
	.string	"mode"
.LASF143:
	.string	"save_addr"
.LASF232:
	.string	"custom_mode_table"
.LASF132:
	.string	"initrd_end"
.LASF288:
	.string	"kfree"
.LASF254:
	.string	"regal_fix"
.LASF72:
	.string	"_datarellocal_start_ofs"
.LASF194:
	.string	"WF_TYPE_RK_GLD16"
.LASF242:
	.string	"regal_pix"
.LASF212:
	.string	"wf_subversion"
.LASF32:
	.string	"block_drvr"
.LASF22:
	.string	"gfp_t"
.LASF35:
	.string	"flash_id"
.LASF126:
	.string	"fit_noffset_setup"
.LASF87:
	.string	"working_fdt"
.LASF152:
	.string	"net_root_path"
.LASF67:
	.string	"bd_t"
.LASF146:
	.string	"s_addr"
.LASF161:
	.string	"net_bcast_ethaddr"
.LASF65:
	.string	"bi_boot_params"
.LASF244:
	.string	"sftemp"
.LASF177:
	.string	"mem_malloc_brk"
.LASF48:
	.string	"bi_memstart"
.LASF181:
	.string	"epd_lut_type"
.LASF189:
	.string	"WF_TYPE_A2"
.LASF209:
	.string	"fpl_lot"
.LASF46:
	.string	"reserved"
.LASF281:
	.string	"free"
.LASF203:
	.string	"pvi_waveform"
.LASF66:
	.string	"bi_dram"
.LASF2:
	.string	"short unsigned int"
.LASF77:
	.string	"magic"
.LASF124:
	.string	"fit_hdr_setup"
.LASF190:
	.string	"WF_TYPE_GCC16"
.LASF113:
	.string	"legacy_hdr_valid"
.LASF42:
	.string	"base"
.LASF228:
	.string	"need_pic"
.LASF34:
	.string	"sector_count"
.LASF99:
	.string	"ih_comp"
.LASF198:
	.string	"WF_TYPE_GRAY16"
.LASF248:
	.string	"lut_data"
.LASF129:
	.string	"ft_addr"
.LASF210:
	.string	"mode_version"
.LASF4:
	.string	"uchar"
.LASF130:
	.string	"ft_len"
.LASF285:
	.string	"/home/hzb/sdk/ebc-dev/rk_eink/../epdlut/pvi_waveform.c"
.LASF201:
	.string	"data"
.LASF192:
	.string	"WF_TYPE_AUTO"
.LASF125:
	.string	"fit_uname_setup"
.LASF148:
	.string	"net_netmask"
.LASF68:
	.string	"IRQ_STACK_START"
.LASF257:
	.string	"pvi_wf_a2_repair"
.LASF266:
	.string	"get_wf_mode_index"
.LASF239:
	.string	"waveform_file"
.LASF249:
	.string	"is_uboot"
.LASF122:
	.string	"fit_uname_fdt"
	.hidden	free
	.ident	"GCC: (Linaro GCC 6.3-2017.05) 6.3.1 20170404"
	.section	.note.GNU-stack,"",@progbits
