/* linux/arch/arm/common/pl330.c
 *
 * Copyright (C) 2010 Samsung Electronics Co Ltd.
 *	Jaswinder Singh <jassi.brar@samsung.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include <linux/list.h>
#include <common.h>
#include <malloc.h>
#include <asm/errno.h>
#include <asm/io.h>
#include "../../board/rockchip/common/armlinux/config.h"
#include <api_pl330.h>

#include "pl330.h"
#define DMA2_USE

#define dev_err(arg1, ...) \
    do\
    {\
        printf(__VA_ARGS__);\
        printf("\n");\
    }while(0);
#define dev_info(arg1, ...) \
    do\
    {\
        printf(__VA_ARGS__);\
        printf("\n");\
    }while(0);

#ifdef DRIVER_ONLY
#define spin_lock_irqsave(a,b)
#define spin_unlock_irqrestore(a,b)
#else
#define spin_lock_irqsave(a,b) 
#define spin_unlock_irqrestore(a,b)
#endif

/* Register and Bit field Definitions */
#define DS		0x0
#define DS_ST_STOP	0x0
#define DS_ST_EXEC	0x1
#define DS_ST_CMISS	0x2
#define DS_ST_UPDTPC	0x3
#define DS_ST_WFE	0x4
#define DS_ST_ATBRR	0x5
#define DS_ST_QBUSY	0x6
#define DS_ST_WFP	0x7
#define DS_ST_KILL	0x8
#define DS_ST_CMPLT	0x9
#define DS_ST_FLTCMP	0xe
#define DS_ST_FAULT	0xf

#define DPC		0x4
#define INTEN		0x20
#define ES		0x24
#define INTSTATUS	0x28
#define INTCLR		0x2c
#define FSM		0x30
#define FSC		0x34
#define FTM		0x38

#define _FTC		0x40
#define FTC(n)		(_FTC + (n)*0x4)

#define _CS		0x100
#define CS(n)		(_CS + (n)*0x8)
#define CS_CNS		(1 << 21)

#define _CPC		0x104
#define CPC(n)		(_CPC + (n)*0x8)

#define _SA		0x400
#define SA(n)		(_SA + (n)*0x20)

#define _DA		0x404
#define DA(n)		(_DA + (n)*0x20)

#define _CC		0x408
#define CC(n)		(_CC + (n)*0x20)

#define CC_SRCINC	(1 << 0)
#define CC_DSTINC	(1 << 14)
#define CC_SRCPRI	(1 << 8)
#define CC_DSTPRI	(1 << 22)
#define CC_SRCNS	(1 << 9)
#define CC_DSTNS	(1 << 23)
#define CC_SRCIA	(1 << 10)
#define CC_DSTIA	(1 << 24)
#define CC_SRCBRSTLEN_SHFT	4
#define CC_DSTBRSTLEN_SHFT	18
#define CC_SRCBRSTSIZE_SHFT	1
#define CC_DSTBRSTSIZE_SHFT	15
#define CC_SRCCCTRL_SHFT	11
#define CC_SRCCCTRL_MASK	0x7
#define CC_DSTCCTRL_SHFT	25
#define CC_DRCCCTRL_MASK	0x7
#define CC_SWAP_SHFT	28

#define _LC0		0x40c
#define LC0(n)		(_LC0 + (n)*0x20)

#define _LC1		0x410
#define LC1(n)		(_LC1 + (n)*0x20)

#define DBGSTATUS	0xd00
#define DBG_BUSY	(1 << 0)

#define DBGCMD		0xd04
#define DBGINST0	0xd08
#define DBGINST1	0xd0c

#define CR0		0xe00
#define CR1		0xe04
#define CR2		0xe08
#define CR3		0xe0c
#define CR4		0xe10
#define CRD		0xe14

#define PERIPH_ID	0xfe0
#define PCELL_ID	0xff0

#define CR0_PERIPH_REQ_SET	(1 << 0)
#define CR0_BOOT_EN_SET		(1 << 1)
#define CR0_BOOT_MAN_NS		(1 << 2)
#define CR0_NUM_CHANS_SHIFT	4
#define CR0_NUM_CHANS_MASK	0x7
#define CR0_NUM_PERIPH_SHIFT	12
#define CR0_NUM_PERIPH_MASK	0x1f
#define CR0_NUM_EVENTS_SHIFT	17
#define CR0_NUM_EVENTS_MASK	0x1f

#define CR1_ICACHE_LEN_SHIFT	0
#define CR1_ICACHE_LEN_MASK	0x7
#define CR1_NUM_ICACHELINES_SHIFT	4
#define CR1_NUM_ICACHELINES_MASK	0xf

#define CRD_DATA_WIDTH_SHIFT	0
#define CRD_DATA_WIDTH_MASK	0x7
#define CRD_WR_CAP_SHIFT	4
#define CRD_WR_CAP_MASK		0x7
#define CRD_WR_Q_DEP_SHIFT	8
#define CRD_WR_Q_DEP_MASK	0xf
#define CRD_RD_CAP_SHIFT	12
#define CRD_RD_CAP_MASK		0x7
#define CRD_RD_Q_DEP_SHIFT	16
#define CRD_RD_Q_DEP_MASK	0xf
#define CRD_DATA_BUFF_SHIFT	20
#define CRD_DATA_BUFF_MASK	0x3ff

#define	PART		0x330
#define DESIGNER	0x41
#define REVISION	0x2
#define INTEG_CFG	0x0
#define PERIPH_ID_VAL	((PART << 0) | (DESIGNER << 12) \
			  | (REVISION << 20) | (INTEG_CFG << 24))

#define PCELL_ID_VAL	0xb105f00d

#define PL330_STATE_STOPPED		(1 << 0)
#define PL330_STATE_EXECUTING		(1 << 1)
#define PL330_STATE_WFE			(1 << 2)
#define PL330_STATE_FAULTING		(1 << 3)
#define PL330_STATE_COMPLETING		(1 << 4)
#define PL330_STATE_WFP			(1 << 5)
#define PL330_STATE_KILLING		(1 << 6)
#define PL330_STATE_FAULT_COMPLETING	(1 << 7)
#define PL330_STATE_CACHEMISS		(1 << 8)
#define PL330_STATE_UPDTPC		(1 << 9)
#define PL330_STATE_ATBARRIER		(1 << 10)
#define PL330_STATE_QUEUEBUSY		(1 << 11)
#define PL330_STATE_INVALID		(1 << 15)

#define PL330_STABLE_STATES (PL330_STATE_STOPPED | PL330_STATE_EXECUTING \
				| PL330_STATE_WFE | PL330_STATE_FAULTING)

#define CMD_DMAADDH	0x54
#define CMD_DMAEND	0x00
#define CMD_DMAFLUSHP	0x35
#define CMD_DMAGO	0xa0
#define CMD_DMALD	0x04
#define CMD_DMALDP	0x25
#define CMD_DMALP	0x20
#define CMD_DMALPEND	0x28
#define CMD_DMAKILL	0x01
#define CMD_DMAMOV	0xbc
#define CMD_DMANOP	0x18
#define CMD_DMARMB	0x12
#define CMD_DMASEV	0x34
#define CMD_DMAST	0x08
#define CMD_DMASTP	0x29
#define CMD_DMASTZ	0x0c
#define CMD_DMAWFE	0x36
#define CMD_DMAWFP	0x30
#define CMD_DMAWMB	0x13

#define SZ_DMAADDH	3
#define SZ_DMAEND	1
#define SZ_DMAFLUSHP	2
#define SZ_DMALD	1
#define SZ_DMALDP	2
#define SZ_DMALP	2
#define SZ_DMALPEND	2
#define SZ_DMAKILL	1
#define SZ_DMAMOV	6
#define SZ_DMANOP	1
#define SZ_DMARMB	1
#define SZ_DMASEV	2
#define SZ_DMAST	1
#define SZ_DMASTP	2
#define SZ_DMASTZ	1
#define SZ_DMAWFE	2
#define SZ_DMAWFP	2
#define SZ_DMAWMB	1
#define SZ_DMAGO	6

#define BRST_LEN(ccr)	((((ccr) >> CC_SRCBRSTLEN_SHFT) & 0xf) + 1)
#define BRST_SIZE(ccr)	(1 << (((ccr) >> CC_SRCBRSTSIZE_SHFT) & 0x7))

#define BYTE_TO_BURST(b, ccr)  ((b) / BRST_SIZE(ccr) / BRST_LEN(ccr))
#define BURST_TO_BYTE(c, ccr)  ((c) * BRST_SIZE(ccr) * BRST_LEN(ccr))

/*
 * With 256 bytes, we can do more than 2.5MB and 5MB xfers per req
 * at 1byte/burst for P<->M and M<->M respectively.
 * For typical scenario, at 1word/burst, 10MB and 20MB xfers per req
 * should be enough for P<->M and M<->M respectively.
 */
#define MCODE_BUFF_PER_REQ	256

/*
 * Mark a _pl330_req as free.
 * We do it by writing DMAEND as the first instruction
 * because no valid request is going to have DMAEND as
 * its first instruction to execute.
 */
#define MARK_FREE(req)	do { \
				_emit_END(0, (req)->mc_cpu); \
				(req)->mc_len = 0; \
			} while (0)

/* If the _pl330_req is available to the client */
#define IS_FREE(req)	(*((uint8 *)((req)->mc_cpu)) == CMD_DMAEND)

/* Use this _only_ to wait on transient states */
#define UNTIL(t, s)	while (!(_state(t) & (s))) udelay(1);

#ifdef PL330_DEBUG_MCGEN
static uint32 cmd_line;
#define PL330_DBGCMD_DUMP(off, x...)	do { \
						printk("%x:", cmd_line); \
						printk(x); \
						cmd_line += off; \
					} while (0)
#define PL330_DBGMC_START(addr)		(cmd_line = addr)
#else
#define PL330_DBGCMD_DUMP(off, x...)	do {} while (0)
#define PL330_DBGMC_START(addr)		do {} while (0)
#endif

struct _xfer_spec {
	uint32 ccr;
	struct pl330_req *r;
	struct pl330_xfer *x;
};

enum dmamov_dst {
	SAR = 0,
	CCR,
	DAR,
};

enum pl330_dst {
	SRC = 0,
	DST,
};

enum pl330_cond {
	SINGLE,
	BURST,
	ALWAYS,
};

struct _pl330_req {
	uint32 mc_bus;
	void *mc_cpu;
	/* Number of bytes taken to setup MC for the req */
	uint32 mc_len;
	struct pl330_req *r;
	/* Hook to attach to DMAC's list of reqs with due callback */
	//struct list_head rqd;
};

/* ToBeDone for tasklet */
struct _pl330_tbd {
	bool reset_dmac;
	bool reset_mngr;
	uint8 reset_chan;
};

/* A DMAC Thread */
struct pl330_thread {
	uint8 id;
	int ev;
	/* If the channel is not yet acquired by any client */
	bool free;
	/* Parent DMAC */
	struct pl330_dmac *dmac;
	/* Only two at a time */
	struct _pl330_req req[2];
	/* Index of the last submitted request */
	uint32 lstenq;
};

enum pl330_dmac_state {
	UNINIT,
	INIT,
	DYING,
};

/* A DMAC */
struct pl330_dmac {
	//spinlock_t		lock;
	uint32 		lock;
	/* Holds list of reqs with due callbacks */
	//struct list_head	req_done;
	/* Pointer to platform specific stuff */
	struct pl330_info	*pinfo;
	/* Maximum possible events/irqs */
	int			events[32];
	/* BUS address of MicroCode buffer */
	uint32			mcode_bus;
	/* CPU address of MicroCode buffer */
	void			*mcode_cpu;
	/* List of all Channel threads */
	struct pl330_thread	*channels;
	/* Pointer to the MANAGER thread */
	struct pl330_thread	*manager;
	/* To handle bad news in interrupt */
	//struct tasklet_struct	tasks;
	struct _pl330_tbd	dmac_tbd;
	/* State of DMAC operation */
	enum pl330_dmac_state	state;
};

static inline void _callback(struct pl330_req *r, enum pl330_op_err err)
{
	if (r && r->xfer_cb)
		r->xfer_cb(r->token, err);
}

static inline bool _queue_empty(struct pl330_thread *thrd)
{
	return (IS_FREE(&thrd->req[0]) && IS_FREE(&thrd->req[1]))
		? true : false;
}

static inline bool _queue_full(struct pl330_thread *thrd)
{
	return (IS_FREE(&thrd->req[0]) || IS_FREE(&thrd->req[1]))
		? false : true;
}

static inline bool is_manager(struct pl330_thread *thrd)
{
	struct pl330_dmac *pl330 = thrd->dmac;

	/* MANAGER is indexed at the end */
	if (thrd->id == pl330->pinfo->pcfg.num_chan)
		return true;
	else
		return false;
}

/* If manager of the thread is in Non-Secure mode */
static inline bool _manager_ns(struct pl330_thread *thrd)
{
	struct pl330_dmac *pl330 = thrd->dmac;

	return (pl330->pinfo->pcfg.mode & DMAC_MODE_NS) ? true : false;
}

static inline uint32 get_id(struct pl330_info *pi, uint32 off)
{
	void __iomem *regs = pi->base;
	uint32 id = 0;
#if 0
	id |= (readb((uint32)regs + off + 0x0) << 0);
	id |= (readb((uint32)regs + off + 0x4) << 8);
	id |= (readb((uint32)regs + off + 0x8) << 16);
	id |= (readb((uint32)regs + off + 0xc) << 24);
#else
	id |= (readl((uint32)regs + off + 0x0) << 0);
	id |= (readl((uint32)regs + off + 0x4) << 8);
	id |= (readl((uint32)regs + off + 0x8) << 16);
	id |= (readl((uint32)regs + off + 0xc) << 24);
  //  id =readl((uint32)regs + off );
#endif
	return id;
}

static inline uint32 _emit_ADDH(uint32 dry_run, uint8 buf[],
		enum pl330_dst da, uint16 val)
{
	if (dry_run)
		return SZ_DMAADDH;

	buf[0] = CMD_DMAADDH;
	buf[0] |= (da << 1);
	*((uint16 *)&buf[1]) = val;

	PL330_DBGCMD_DUMP(SZ_DMAADDH, "\tDMAADDH %s %u\n",
		da == 1 ? "DA" : "SA", val);

	return SZ_DMAADDH;
}

static inline uint32 _emit_END(uint32 dry_run, uint8 buf[])
{
	if (dry_run)
		return SZ_DMAEND;

	buf[0] = CMD_DMAEND;

	PL330_DBGCMD_DUMP(SZ_DMAEND, "\tDMAEND\n");

	return SZ_DMAEND;
}

static inline uint32 _emit_FLUSHP(uint32 dry_run, uint8 buf[], uint8 peri)
{
	if (dry_run)
		return SZ_DMAFLUSHP;

	buf[0] = CMD_DMAFLUSHP;

	peri &= 0x1f;
	peri <<= 3;
	buf[1] = peri;

	PL330_DBGCMD_DUMP(SZ_DMAFLUSHP, "\tDMAFLUSHP %u\n", peri >> 3);

	return SZ_DMAFLUSHP;
}

static inline uint32 _emit_LD(uint32 dry_run, uint8 buf[],	enum pl330_cond cond)
{
	if (dry_run)
		return SZ_DMALD;

	buf[0] = CMD_DMALD;

	if (cond == SINGLE)
		buf[0] |= (0 << 1) | (1 << 0);
	else if (cond == BURST)
		buf[0] |= (1 << 1) | (1 << 0);

	PL330_DBGCMD_DUMP(SZ_DMALD, "\tDMALD%c\n",
		cond == SINGLE ? 'S' : (cond == BURST ? 'B' : 'A'));

	return SZ_DMALD;
}

static inline uint32 _emit_LDP(uint32 dry_run, uint8 buf[],
		enum pl330_cond cond, uint8 peri)
{
	if (dry_run)
		return SZ_DMALDP;

	buf[0] = CMD_DMALDP;

	if (cond == BURST)
		buf[0] |= (1 << 1);

	peri &= 0x1f;
	peri <<= 3;
	buf[1] = peri;

	PL330_DBGCMD_DUMP(SZ_DMALDP, "\tDMALDP%c %u\n",
		cond == SINGLE ? 'S' : 'B', peri >> 3);

	return SZ_DMALDP;
}

static inline uint32 _emit_LP(uint32 dry_run, uint8 buf[],
		uint32 loop, uint8 cnt)
{
	if (dry_run)
		return SZ_DMALP;

	buf[0] = CMD_DMALP;

	if (loop)
		buf[0] |= (1 << 1);

	cnt--; /* DMAC increments by 1 internally */
	buf[1] = cnt;

	PL330_DBGCMD_DUMP(SZ_DMALP, "\tDMALP_%c %u\n", loop ? '1' : '0', cnt);

	return SZ_DMALP;
}

struct _arg_LPEND {
	enum pl330_cond cond;
	bool forever;
	uint32 loop;
	uint8 bjump;
};

static inline uint32  _emit_LPEND(uint32 dry_run, uint8 buf[],
		const struct _arg_LPEND *arg)
{
	enum pl330_cond cond = arg->cond;
	bool forever = arg->forever;
	uint32 loop = arg->loop;
	uint8 bjump = arg->bjump;

	if (dry_run)
		return SZ_DMALPEND;

	buf[0] = CMD_DMALPEND;

	if (loop)
		buf[0] |= (1 << 2);

	if (!forever)
		buf[0] |= (1 << 4);

	if (cond == SINGLE)
		buf[0] |= (0 << 1) | (1 << 0);
	else if (cond == BURST)
		buf[0] |= (1 << 1) | (1 << 0);

	buf[1] = bjump;

	PL330_DBGCMD_DUMP(SZ_DMALPEND, "\tDMALP%s%c_%c bjmpto_%x\n",
			forever ? "FE" : "END",
			cond == SINGLE ? 'S' : (cond == BURST ? 'B' : 'A'),
			loop ? '1' : '0',
			bjump);

	return SZ_DMALPEND;
}

static inline uint32 _emit_KILL(uint32 dry_run, uint8 buf[])
{
	if (dry_run)
		return SZ_DMAKILL;

	buf[0] = CMD_DMAKILL;

	return SZ_DMAKILL;
}

static inline uint32 _emit_MOV(uint32 dry_run, uint8 buf[],
		enum dmamov_dst dst, uint32 val)
{
	if (dry_run)
		return SZ_DMAMOV;

	buf[0] = CMD_DMAMOV;
	buf[1] = dst;
	// *((u32 *)&buf[2]) = val;
	buf[2]=(uint8)(val&0xff);
	buf[3]=(uint8)((val>>8)&0xff);
	buf[4]=(uint8)((val>>16)&0xff);
	buf[5]=(uint8)((val>>24)&0xff);

	PL330_DBGCMD_DUMP(SZ_DMAMOV, "\tDMAMOV %s 0x%x\n",
		dst == SAR ? "SAR" : (dst == DAR ? "DAR" : "CCR"), val);

	return SZ_DMAMOV;
}

static inline uint32 _emit_NOP(uint32 dry_run, uint8 buf[])
{
	if (dry_run)
		return SZ_DMANOP;

	buf[0] = CMD_DMANOP;

	PL330_DBGCMD_DUMP(SZ_DMANOP, "\tDMANOP\n");

	return SZ_DMANOP;
}

static inline uint32 _emit_RMB(uint32 dry_run, uint8 buf[])
{
	if (dry_run)
		return SZ_DMARMB;

	buf[0] = CMD_DMARMB;

	PL330_DBGCMD_DUMP(SZ_DMARMB, "\tDMARMB\n");

	return SZ_DMARMB;
}

static inline uint32 _emit_SEV(uint32 dry_run, uint8 buf[], uint8 ev)
{
	if (dry_run)
		return SZ_DMASEV;

	buf[0] = CMD_DMASEV;

	ev &= 0x1f;
	ev <<= 3;
	buf[1] = ev;

	PL330_DBGCMD_DUMP(SZ_DMASEV, "\tDMASEV %u\n", ev >> 3);

	return SZ_DMASEV;
}

static inline uint32 _emit_ST(uint32 dry_run, uint8 buf[], enum pl330_cond cond)
{
	if (dry_run)
		return SZ_DMAST;

	buf[0] = CMD_DMAST;

	if (cond == SINGLE)
		buf[0] |= (0 << 1) | (1 << 0);
	else if (cond == BURST)
		buf[0] |= (1 << 1) | (1 << 0);

	PL330_DBGCMD_DUMP(SZ_DMAST, "\tDMAST%c\n",
		cond == SINGLE ? 'S' : (cond == BURST ? 'B' : 'A'));

	return SZ_DMAST;
}

static inline uint32 _emit_STP(uint32 dry_run, uint8 buf[],
		enum pl330_cond cond, uint8 peri)
{
	if (dry_run)
		return SZ_DMASTP;

	buf[0] = CMD_DMASTP;

	if (cond == BURST)
		buf[0] |= (1 << 1);

	peri &= 0x1f;
	peri <<= 3;
	buf[1] = peri;

	PL330_DBGCMD_DUMP(SZ_DMASTP, "\tDMASTP%c %u\n",
		cond == SINGLE ? 'S' : 'B', peri >> 3);

	return SZ_DMASTP;
}

static inline uint32 _emit_STZ(uint32 dry_run, uint8 buf[])
{
	if (dry_run)
		return SZ_DMASTZ;

	buf[0] = CMD_DMASTZ;

	PL330_DBGCMD_DUMP(SZ_DMASTZ, "\tDMASTZ\n");

	return SZ_DMASTZ;
}

static inline uint32 _emit_WFE(uint32 dry_run, uint8 buf[], uint8 ev,
		uint32 invalidate)
{
	if (dry_run)
		return SZ_DMAWFE;

	buf[0] = CMD_DMAWFE;

	ev &= 0x1f;
	ev <<= 3;
	buf[1] = ev;

	if (invalidate)
		buf[1] |= (1 << 1);

	PL330_DBGCMD_DUMP(SZ_DMAWFE, "\tDMAWFE %u%s\n",
		ev >> 3, invalidate ? ", I" : "");

	return SZ_DMAWFE;
}

static inline uint32 _emit_WFP(uint32 dry_run, uint8 buf[],
		enum pl330_cond cond, uint8 peri)
{
	if (dry_run)
		return SZ_DMAWFP;

	buf[0] = CMD_DMAWFP;

	if (cond == SINGLE)
		buf[0] |= (0 << 1) | (0 << 0);
	else if (cond == BURST)
		buf[0] |= (1 << 1) | (0 << 0);
	else
		buf[0] |= (0 << 1) | (1 << 0);

	peri &= 0x1f;
	peri <<= 3;
	buf[1] = peri;

	PL330_DBGCMD_DUMP(SZ_DMAWFP, "\tDMAWFP%c %u\n",
		cond == SINGLE ? 'S' : (cond == BURST ? 'B' : 'P'), peri >> 3);

	return SZ_DMAWFP;
}

static inline uint32 _emit_WMB(uint32 dry_run, uint8 buf[])
{
	if (dry_run)
		return SZ_DMAWMB;

	buf[0] = CMD_DMAWMB;

	PL330_DBGCMD_DUMP(SZ_DMAWMB, "\tDMAWMB\n");

	return SZ_DMAWMB;
}

struct _arg_GO {
	uint8 chan;
	uint32 addr;
	uint32 ns;
};

static inline uint32 _emit_GO(uint32 dry_run, uint8 buf[],
		const struct _arg_GO *arg)
{
	uint8 chan = arg->chan;
	uint32 addr = arg->addr;
	uint32 ns = arg->ns;

	if (dry_run)
		return SZ_DMAGO;

	buf[0] = CMD_DMAGO;
	buf[0] |= (ns << 1);

	buf[1] = chan & 0x7;

	// *((u32 *)&buf[2]) = addr;
	    buf[2]=(uint8)(addr&0xff);
		buf[3]=(uint8)((addr>>8)&0xff);
		buf[4]=(uint8)((addr>>16)&0xff);
		buf[5]=(uint8)((addr>>24)&0xff);

	return SZ_DMAGO;
}


/* Returns Time-Out */
static bool _until_dmac_idle(struct pl330_thread *thrd)
{
	void __iomem *regs = thrd->dmac->pinfo->base;
	uint32 loops = 5;

	do {
		/* Until Manager is Idle */
		if (!(readl((uint32)regs + DBGSTATUS) & DBG_BUSY))
			break;

		mdelay(1);
	} while (--loops);

	if (!loops)
		return true;

	return false;
}

static inline void _execute_DBGINSN(struct pl330_thread *thrd,
		uint8 insn[], bool as_manager)
{
	void __iomem *regs = thrd->dmac->pinfo->base;
	uint32 val;
    volatile uint16 tmp[2];

	val = (insn[0] << 16) | (insn[1] << 24);
	if (!as_manager) {
		val |= (1 << 0);
		val |= (thrd->id << 8); /* Channel Number */
	}
	writel(val, (uint32)regs + DBGINST0);

	//val = *((uint32 *)&insn[2]);
	tmp[0] = insn[2]|(insn[3]<<8);
	tmp[1] = insn[4]|(insn[5]<<8);
	val  = tmp[0]+(tmp[1]<<16);
	//val=insn[2]|(insn[3]<<8)|(insn[4]<<16)|(insn[5]<<24);

	writel(val, (uint32)regs + DBGINST1);
	/* If timed out due to halted state-machine */
	if (_until_dmac_idle(thrd)) {
		dev_err(thrd->dmac->pinfo->dev, "DMAC halted!\n");
		return;
	}

	/* Get going */
	writel(0, (uint32)regs + DBGCMD);
}

static inline uint32 _state(struct pl330_thread *thrd)
{
	void __iomem *regs = thrd->dmac->pinfo->base;
	uint32 val;

	if (is_manager(thrd))
		val = readl((uint32)regs + DS) & 0xf;
	else
		val = readl((uint32)regs + CS(thrd->id)) & 0xf;

	switch (val) {
	case DS_ST_STOP:
		return PL330_STATE_STOPPED;
	case DS_ST_EXEC:
		return PL330_STATE_EXECUTING;
	case DS_ST_CMISS:
		return PL330_STATE_CACHEMISS;
	case DS_ST_UPDTPC:
		return PL330_STATE_UPDTPC;
	case DS_ST_WFE:
		return PL330_STATE_WFE;
	case DS_ST_FAULT:
		return PL330_STATE_FAULTING;
	case DS_ST_ATBRR:
		if (is_manager(thrd))
			return PL330_STATE_INVALID;
		else
			return PL330_STATE_ATBARRIER;
	case DS_ST_QBUSY:
		if (is_manager(thrd))
			return PL330_STATE_INVALID;
		else
			return PL330_STATE_QUEUEBUSY;
	case DS_ST_WFP:
		if (is_manager(thrd))
			return PL330_STATE_INVALID;
		else
			return PL330_STATE_WFP;
	case DS_ST_KILL:
		if (is_manager(thrd))
			return PL330_STATE_INVALID;
		else
			return PL330_STATE_KILLING;
	case DS_ST_CMPLT:
		if (is_manager(thrd))
			return PL330_STATE_INVALID;
		else
			return PL330_STATE_COMPLETING;
	case DS_ST_FLTCMP:
		if (is_manager(thrd))
			return PL330_STATE_INVALID;
		else
			return PL330_STATE_FAULT_COMPLETING;
	default:
		return PL330_STATE_INVALID;
	}
}

/* If the request 'req' of thread 'thrd' is currently active */
static inline bool _req_active(struct pl330_thread *thrd,
		struct _pl330_req *req)
{
	void __iomem *regs = thrd->dmac->pinfo->base;
	uint32 buf = req->mc_bus, pc = readl((uint32)regs + CPC(thrd->id));

	if (IS_FREE(req))
		return false;

	return (pc >= buf && pc <= buf + req->mc_len) ? true : false;
}

/* Returns 0 if the thread is inactive, ID of active req + 1 otherwise */
static inline uint32 _thrd_active(struct pl330_thread *thrd)
{
	if (_req_active(thrd, &thrd->req[0]))
		return 1; /* First req active */

	if (_req_active(thrd, &thrd->req[1]))
		return 2; /* Second req active */

	return 0;
}

static void _stop(struct pl330_thread *thrd)
{
	void __iomem *regs = thrd->dmac->pinfo->base;
	uint8 insn[6] = {0, 0, 0, 0, 0, 0};

	if (_state(thrd) == PL330_STATE_FAULT_COMPLETING)
		UNTIL(thrd, PL330_STATE_FAULTING | PL330_STATE_KILLING);

	/* Return if nothing needs to be done */
	if (_state(thrd) == PL330_STATE_COMPLETING
		  || _state(thrd) == PL330_STATE_KILLING
		  || _state(thrd) == PL330_STATE_STOPPED)
		return;

	_emit_KILL(0, insn);

	/* Stop generating interrupts for SEV */
	writel(readl((uint32)regs + INTEN) & ~(1 << thrd->ev), (uint32)regs + INTEN);

	_execute_DBGINSN(thrd, insn, is_manager(thrd));
}

/* Start doing req 'idx' of thread 'thrd' */
static bool _trigger(struct pl330_thread *thrd)
{
	void __iomem *regs = thrd->dmac->pinfo->base;
	struct _pl330_req *req;
	struct pl330_req *r;
	struct _arg_GO go;
	uint32 ns;
	uint8 insn[6] = {0, 0, 0, 0, 0, 0};

	/* Return if already ACTIVE */
	if (_state(thrd) != PL330_STATE_STOPPED)
		return true;

	if (!IS_FREE(&thrd->req[1 - thrd->lstenq]))
		req = &thrd->req[1 - thrd->lstenq];
	else if (!IS_FREE(&thrd->req[thrd->lstenq]))
		req = &thrd->req[thrd->lstenq];
	else
		req = NULL;

	/* Return if no request */
	if (!req || !req->r)
		return false;//true;

	r = req->r;

	if (r->cfg)
		ns = r->cfg->nonsecure ? 1 : 0;
	else if (readl((uint32)regs + CS(thrd->id)) & CS_CNS)
		ns = 1;
	else
		ns = 0;

	/* See 'Abort Sources' point-4 at Page 2-25 */
	if (_manager_ns(thrd) && !ns)
		dev_info(thrd->dmac->pinfo->dev, "%s:%d Recipe for ABORT!\n",
			__func__, __LINE__);

	go.chan = thrd->id;
	go.addr = req->mc_bus;
	go.ns = ns;
	_emit_GO(0, insn, &go);

	/* Set to generate interrupts for SEV */
	writel(readl((uint32)regs + INTEN) | (1 << thrd->ev), (uint32)regs + INTEN);

	/* Only manager can execute GO */
	_execute_DBGINSN(thrd, insn, true);

	return true;
}

static bool _start(struct pl330_thread *thrd)
{
	switch (_state(thrd)) {
	case PL330_STATE_FAULT_COMPLETING:
		UNTIL(thrd, PL330_STATE_FAULTING | PL330_STATE_KILLING);

		if (_state(thrd) == PL330_STATE_KILLING)
			UNTIL(thrd, PL330_STATE_STOPPED)

	case PL330_STATE_FAULTING:
		_stop(thrd);

	case PL330_STATE_KILLING:
	case PL330_STATE_COMPLETING:
		UNTIL(thrd, PL330_STATE_STOPPED)

	case PL330_STATE_STOPPED:
		return _trigger(thrd);

	case PL330_STATE_WFP:
	case PL330_STATE_QUEUEBUSY:
	case PL330_STATE_ATBARRIER:
	case PL330_STATE_UPDTPC:
	case PL330_STATE_CACHEMISS:
	case PL330_STATE_EXECUTING:
		return true;

	case PL330_STATE_WFE: /* For RESUME, nothing yet */
	default:
		return false;
	}
}

static inline int _ldst_memtomem(uint32 dry_run, uint8 buf[],
		const struct _xfer_spec *pxs, int cyc)
{
	int off = 0;

	while (cyc--) {
		off += _emit_LD(dry_run, &buf[off], ALWAYS);
		off += _emit_RMB(dry_run, &buf[off]);
		off += _emit_ST(dry_run, &buf[off], ALWAYS);
		off += _emit_WMB(dry_run, &buf[off]);
	}

	return off;
}

static inline int _ldst_devtomem(uint32 dry_run, uint8 buf[],
		const struct _xfer_spec *pxs, int cyc)
{
	int off = 0;
    

	while (cyc--) {
		off += _emit_WFP(dry_run, &buf[off], BURST, pxs->r->peri);//SINGLE
		off += _emit_LDP(dry_run, &buf[off], BURST, pxs->r->peri);//SINGLE
		off += _emit_ST(dry_run, &buf[off], BURST);//ALWAYS
//      off += _emit_FLUSHP(dry_run, &buf[off], pxs->r->peri);
	}

	return off;
}

static inline int _ldst_memtodev(uint32 dry_run, uint8 buf[],
		const struct _xfer_spec *pxs, int cyc)
{
	int off = 0;
#if 0
	while (cyc--) {
		off += _emit_WFP(dry_run, &buf[off], SINGLE, pxs->r->peri);//SINGLE
		off += _emit_LD(dry_run, &buf[off], ALWAYS);
		off += _emit_STP(dry_run, &buf[off], SINGLE, pxs->r->peri);
		off += _emit_FLUSHP(dry_run, &buf[off], pxs->r->peri);
	}
#else
	while (cyc--) {
		off += _emit_WFP(dry_run, &buf[off], BURST, pxs->r->peri);//SINGLE
		off += _emit_LD(dry_run, &buf[off], BURST);//ALWAYS
		off += _emit_STP(dry_run, &buf[off], BURST, pxs->r->peri);
//		off += _emit_FLUSHP(dry_run, &buf[off], pxs->r->peri);
	}
#endif
	return off;
}

static int _bursts(uint32 dry_run, uint8 buf[],
		const struct _xfer_spec *pxs, int cyc)
{
	int off = 0;

	switch (pxs->r->rqtype) {
	case MEMTODEV:
		off += _ldst_memtodev(dry_run, &buf[off], pxs, cyc);
		break;
	case DEVTOMEM:
		off += _ldst_devtomem(dry_run, &buf[off], pxs, cyc);
		break;
	case MEMTOMEM:
		off += _ldst_memtomem(dry_run, &buf[off], pxs, cyc);
		break;
	default:
		off += 0x40000000; /* Scare off the Client */
		break;
	}

	return off;
}

/* Returns bytes consumed and updates bursts */
static inline int _loop(uint32 dry_run, uint8 buf[],
		uint32 *bursts, const struct _xfer_spec *pxs)
{
	int cyc, cycmax, szlp, szlpend, szbrst, off;
	uint32 lcnt0, lcnt1, ljmp0, ljmp1;
	struct _arg_LPEND lpend;

	/* Max iterations possibile in DMALP is 256 */
	if (*bursts >= 256*256) {
		lcnt1 = 256;
		lcnt0 = 256;
		cyc = *bursts / lcnt1 / lcnt0;
	} else if (*bursts > 256) {
		lcnt1 = 256;
		lcnt0 = *bursts / lcnt1;
		cyc = 1;
	} else {
		lcnt1 = *bursts;
		lcnt0 = 0;
		cyc = 1;
	}

	szlp = _emit_LP(1, buf, 0, 0);
	szbrst = _bursts(1, buf, pxs, 1);

	lpend.cond = ALWAYS;
	lpend.forever = false;
	lpend.loop = 0;
	lpend.bjump = 0;
	szlpend = _emit_LPEND(1, buf, &lpend);

	if (lcnt0) {
		szlp *= 2;
		szlpend *= 2;
	}

	/*
	 * Max bursts that we can unroll due to limit on the
	 * size of backward jump that can be encoded in DMALPEND
	 * which is 8-bits and hence 255
	 */
	cycmax = (255 - (szlp + szlpend)) / szbrst;

	cyc = (cycmax < cyc) ? cycmax : cyc;

	off = 0;

	if (lcnt0) {
		off += _emit_LP(dry_run, &buf[off], 0, lcnt0);
		ljmp0 = off;
	}

	off += _emit_LP(dry_run, &buf[off], 1, lcnt1);
	ljmp1 = off;

	off += _bursts(dry_run, &buf[off], pxs, cyc);

	lpend.cond = ALWAYS;
	lpend.forever = false;
	lpend.loop = 1;
	lpend.bjump = off - ljmp1;
	off += _emit_LPEND(dry_run, &buf[off], &lpend);

	if (lcnt0) {
		lpend.cond = ALWAYS;
		lpend.forever = false;
		lpend.loop = 0;
		lpend.bjump = off - ljmp0;
		off += _emit_LPEND(dry_run, &buf[off], &lpend);
	}

	*bursts = lcnt1 * cyc;
	if (lcnt0)
		*bursts *= lcnt0;

	return off;
}

static inline int _setup_loops(uint32 dry_run, uint8 buf[],
		const struct _xfer_spec *pxs)
{
	struct pl330_xfer *x = pxs->x;
	uint32 ccr = pxs->ccr;
	uint32 c, bursts = BYTE_TO_BURST(x->bytes, ccr);
	int off = 0;

	while (bursts) {
		c = bursts;
		off += _loop(dry_run, &buf[off], &c, pxs);
		bursts -= c;
	}

	return off;
}

static inline int _setup_xfer(uint32 dry_run, uint8 buf[],
		const struct _xfer_spec *pxs)
{
	struct pl330_xfer *x = pxs->x;
	int off = 0;

	/* DMAMOV SAR, x->src_addr */
	off += _emit_MOV(dry_run, &buf[off], SAR, x->src_addr);
	/* DMAMOV DAR, x->dst_addr */
	off += _emit_MOV(dry_run, &buf[off], DAR, x->dst_addr);

	/* Setup Loop(s) */
	off += _setup_loops(dry_run, &buf[off], pxs);

	return off;
}

/*
 * A req is a sequence of one or more xfer units.
 * Returns the number of bytes taken to setup the MC for the req.
 */
static int _setup_req(uint32 dry_run, struct pl330_thread *thrd,
		uint32 index, struct _xfer_spec *pxs)
{
	struct _pl330_req *req = &thrd->req[index];
	struct pl330_xfer *x;
	uint8 *buf = req->mc_cpu;
	int off = 0;

	PL330_DBGMC_START(req->mc_bus);

	/* DMAMOV CCR, ccr */
	off += _emit_MOV(dry_run, &buf[off], CCR, pxs->ccr);

	x = pxs->r->x;
	do {
		/* Error if xfer length is not aligned at burst size */
		if (x->bytes % (BRST_SIZE(pxs->ccr) * BRST_LEN(pxs->ccr)))
		{
		    printf("error xfer length is not aligned at burst size\n");
			return -EINVAL;
        }

		pxs->x = x;
		off += _setup_xfer(dry_run, &buf[off], pxs);

		x = x->next;
	} while (x);

	/* DMASEV peripheral/event */
	off += _emit_SEV(dry_run, &buf[off], thrd->ev);
	/* DMAEND */
	off += _emit_END(dry_run, &buf[off]);

	return off;
}

static inline uint32 _prepare_ccr(const struct pl330_reqcfg *rqc)
{
	uint32 ccr = 0;

	if (rqc->src_inc)
		ccr |= CC_SRCINC;

	if (rqc->dst_inc)
		ccr |= CC_DSTINC;

	/* We set same protection levels for Src and DST for now */
	if (rqc->privileged)
		ccr |= CC_SRCPRI | CC_DSTPRI;
	if (rqc->nonsecure)
		ccr |= CC_SRCNS | CC_DSTNS;
	if (rqc->insnaccess)
		ccr |= CC_SRCIA | CC_DSTIA;

	ccr |= (((rqc->brst_len - 1) & 0xf) << CC_SRCBRSTLEN_SHFT);
	ccr |= (((rqc->brst_len - 1) & 0xf) << CC_DSTBRSTLEN_SHFT);

	ccr |= (rqc->brst_size << CC_SRCBRSTSIZE_SHFT);
	ccr |= (rqc->brst_size << CC_DSTBRSTSIZE_SHFT);

	ccr |= (rqc->dcctl << CC_SRCCCTRL_SHFT);
	ccr |= (rqc->scctl << CC_DSTCCTRL_SHFT);

	ccr |= (rqc->swap << CC_SWAP_SHFT);

	return ccr;
}

static inline bool _is_valid(uint32 ccr)
{
	enum pl330_dstcachectrl dcctl;
	enum pl330_srccachectrl scctl;

	dcctl = (ccr >> CC_DSTCCTRL_SHFT) & CC_DRCCCTRL_MASK;
	scctl = (ccr >> CC_SRCCCTRL_SHFT) & CC_SRCCCTRL_MASK;

	if (dcctl == DINVALID1 || dcctl == DINVALID2
			|| scctl == SINVALID1 || scctl == SINVALID2)
		return false;
	else
		return true;
}

/*
 * Submit a list of xfers after which the client wants notification.
 * Client is not notified after each xfer unit, just once after all
 * xfer units are done or some error occurs.
 */
int pl330_submit_req(void *ch_id, struct pl330_req *r)
{
	struct pl330_thread *thrd = ch_id;
	struct pl330_dmac *pl330;
	struct pl330_info *pi;
	struct _xfer_spec xs;
	uint32 flags;
	void __iomem *regs;
	uint32 idx;
	uint32 ccr;
	int ret = 0;

	/* No Req or Unacquired Channel or DMAC */
	if (!r || !thrd || thrd->free)
		return -EINVAL;

	pl330 = thrd->dmac;
	pi = pl330->pinfo;
	regs = pi->base;

	if (pl330->state == DYING
		|| pl330->dmac_tbd.reset_chan & (1 << thrd->id)) {
		dev_info(thrd->dmac->pinfo->dev, "%s:%d\n",
			__func__, __LINE__);
		return -EAGAIN;
	}

	/* If request for non-existing peripheral */
	if (r->rqtype != MEMTOMEM && r->peri >= pi->pcfg.num_peri) {
		dev_info(thrd->dmac->pinfo->dev,
				"%s:%d Invalid peripheral(%u)!\n",
				__func__, __LINE__, r->peri);
		return -EINVAL;
	}

	spin_lock_irqsave(&pl330->lock, flags);

	if (_queue_full(thrd)) {
		ret = -EAGAIN;
		goto xfer_exit;
	}

	/* Prefer Secure Channel */
	if (!_manager_ns(thrd))
		r->cfg->nonsecure = 0;
	else
		r->cfg->nonsecure = 1;

	/* Use last settings, if not provided */
	if (r->cfg)
		ccr = _prepare_ccr(r->cfg);
	else
		ccr = readl((uint32)regs + CC(thrd->id));

	/* If this req doesn't have valid xfer settings */
	if (!_is_valid(ccr)) {
		ret = -EINVAL;
		dev_info(thrd->dmac->pinfo->dev, "%s:%d Invalid CCR(%x)!\n",
			__func__, __LINE__, ccr);
		goto xfer_exit;
	}

	idx = IS_FREE(&thrd->req[0]) ? 0 : 1;

	xs.ccr = ccr;
	xs.r = r;

	/* First dry run to check if req is acceptable */
	ret = _setup_req(1, thrd, idx, &xs);
	if (ret < 0)
	{
		printf("setup_req fail!");
		goto xfer_exit;
	}

	if (ret > pi->mcbufsz / 2) {
		dev_info(thrd->dmac->pinfo->dev,
			"%s:%d Trying increasing mcbufsz\n",
				__func__, __LINE__);
		ret = -ENOMEM;
		goto xfer_exit;
	}

	/* Hook the request */
	thrd->lstenq = idx;
	thrd->req[idx].mc_len = _setup_req(0, thrd, idx, &xs);
	thrd->req[idx].r = r;

	ret = 0;

xfer_exit:
	spin_unlock_irqrestore(&pl330->lock, flags);

	return ret;
}

static void pl330_dotask(uint32 data)
{
	struct pl330_dmac *pl330 = (struct pl330_dmac *) data;
	struct pl330_info *pi = pl330->pinfo;
	uint32 flags;
	int i;

	spin_lock_irqsave(&pl330->lock, flags);

	/* The DMAC itself gone nuts */
	if (pl330->dmac_tbd.reset_dmac) {
		pl330->state = DYING;
		/* Reset the manager too */
		pl330->dmac_tbd.reset_mngr = true;
		/* Clear the reset flag */
		pl330->dmac_tbd.reset_dmac = false;
	}

	if (pl330->dmac_tbd.reset_mngr) {
		_stop(pl330->manager);
		/* Reset all channels */
		pl330->dmac_tbd.reset_chan = (1 << pi->pcfg.num_chan) - 1;
		/* Clear the reset flag */
		pl330->dmac_tbd.reset_mngr = false;
	}

	for (i = 0; i < pi->pcfg.num_chan; i++) {

		if (pl330->dmac_tbd.reset_chan & (1 << i)) {
			struct pl330_thread *thrd = &pl330->channels[i];
			void __iomem *regs = pi->base;
			enum pl330_op_err err;

			_stop(thrd);

			if (readl((uint32)regs + FSC) & (1 << thrd->id))
				err = PL330_ERR_FAIL;
			else
				err = PL330_ERR_ABORT;

			spin_unlock_irqrestore(&pl330->lock, flags);

			_callback(thrd->req[1 - thrd->lstenq].r, err);
			_callback(thrd->req[thrd->lstenq].r, err);

			spin_lock_irqsave(&pl330->lock, flags);

			thrd->req[0].r = NULL;
			thrd->req[1].r = NULL;
			MARK_FREE(&thrd->req[0]);
			MARK_FREE(&thrd->req[1]);

			/* Clear the reset flag */
			pl330->dmac_tbd.reset_chan &= ~(1 << i);
		}
	}

	spin_unlock_irqrestore(&pl330->lock, flags);

	return;
}

/* Returns 1 if state was updated, 0 otherwise */
int pl330_update(const struct pl330_info *pi)
{
	struct _pl330_req *rqdone;
	struct pl330_dmac *pl330;
	uint32 flags;
	void __iomem *regs;
	uint32 val;
	int id, ev, ret = 0;

	if (!pi || !pi->pl330_data)
		return 0;

	regs = pi->base;
	pl330 = pi->pl330_data;

	spin_lock_irqsave(&pl330->lock, flags);

	val = readl((uint32)regs + FSM) & 0x1;
	if (val)
		pl330->dmac_tbd.reset_mngr = true;
	else
		pl330->dmac_tbd.reset_mngr = false;

	val = readl((uint32)regs + FSC) & ((1 << pi->pcfg.num_chan) - 1);
	pl330->dmac_tbd.reset_chan |= val;
	if (val) {
		int i = 0;
		while (i < pi->pcfg.num_chan) {
			if (val & (1 << i)) {
				dev_info(pi->dev,
					"Reset Channel-%d\t CS-%x FTC-%x\n",
						i, readl((uint32)regs + CS(i)),
						readl((uint32)regs + FTC(i)));
				_stop(&pl330->channels[i]);
			}
			i++;
		}
	}

	/* Check which event happened i.e, thread notified */
	val = readl((uint32)regs + ES);
	if (pi->pcfg.num_events < 32
			&& val & ~((1 << pi->pcfg.num_events) - 1)) {
		pl330->dmac_tbd.reset_dmac = true;
		dev_err(pi->dev, "%s:%d Unexpected!\n", __func__, __LINE__);
		ret = 1;
		goto updt_exit;
	}
	#if 0
	for (ev = 0; ev < pi->pcfg.num_events; ev++) {
		uint32 inten = readl((uint32)regs + INTEN);

		/* Clear the event */
		if (inten & val & (1 << ev))
			writel(1 << ev, (uint32)regs + INTCLR);
	}
	for (id = 0; id < pi->pcfg.num_chan; id++) {
		 /* Event occured */
			struct pl330_thread *thrd;
			int active;

			ret = 1;

			thrd = &pl330->channels[id];

            if(thrd->free) /*no request at this channel*/
            continue;
            
			active = _thrd_active(thrd);
			if (!active) /* Aborted */
				continue;

			active -= 1;

			rqdone = &thrd->req[active];
			MARK_FREE(rqdone);

			/* Get going again ASAP */
			if(!_start(thrd))//no request any more
				pl330_release_channel(thrd);

			/* For now, just make a list of callbacks to be done */
			//list_add_tail(&rqdone->rqd, &pl330->req_done);
			//if(rqdone->r->xfer_cb)
			{
			//    _callback(rqdone->r, PL330_ERR_NONE);
			}
		
	}
       #else
	for (ev = 0; ev < pi->pcfg.num_events; ev++) {
            if(val & (1 << ev)){//event occurred
                struct pl330_thread *thrd;
                uint32 inten = readl((uint32)regs + INTEN);
                int active;

                /* Clear the event */
                if (inten  & (1 << ev))
			writel(1 << ev, (uint32)regs + INTCLR);
                
                ret = 1;

                id = pl330->events[ev];
                thrd = &pl330->channels[id];
            
                active = _thrd_active(thrd);
                if (!active) /* Aborted */
                    continue;

                active -= 1;

                rqdone = &thrd->req[active];
                MARK_FREE(rqdone);
			/* Get going again ASAP */
		if(!_start(thrd))//no request any more //测试用，非STOP状态不release channel
			pl330_release_channel(thrd);
	    }
	}
       #endif
	/* Now that we are in no hurry, do the callbacks */
	/*
	while (!list_empty(&pl330->req_done)) {
		rqdone = container_of(pl330->req_done.next,
					struct _pl330_req, rqd);

		list_del_init(&rqdone->rqd);

		spin_unlock_irqrestore(&pl330->lock, flags);
		_callback(rqdone->r, PL330_ERR_NONE);
		spin_lock_irqsave(&pl330->lock, flags);
	}
    */

updt_exit:
	spin_unlock_irqrestore(&pl330->lock, flags);

	if (pl330->dmac_tbd.reset_dmac
			|| pl330->dmac_tbd.reset_mngr
			|| pl330->dmac_tbd.reset_chan) {
		ret = 1;
		pl330_dotask((uint32)pl330);
		//tasklet_schedule(&pl330->tasks);
	}

	return ret;
}

int pl330_chan_ctrl(void *ch_id, enum pl330_chan_op op)
{
	struct pl330_thread *thrd = ch_id;
	struct pl330_dmac *pl330;
	uint32 flags;
	int ret = 0, active;

	if (!thrd || thrd->free || thrd->dmac->state == DYING)
		return -EINVAL;

	pl330 = thrd->dmac;

	spin_lock_irqsave(&pl330->lock, flags);

	switch (op) {
	case PL330_OP_FLUSH:
		/* Make sure the channel is stopped */
		_stop(thrd);

		thrd->req[0].r = NULL;
		thrd->req[1].r = NULL;
		MARK_FREE(&thrd->req[0]);
		MARK_FREE(&thrd->req[1]);
		break;

	case PL330_OP_ABORT:
		active = _thrd_active(thrd);

		/* Make sure the channel is stopped */
		_stop(thrd);

		/* ABORT is only for the active req */
		if (!active)
			break;

		active--;

		thrd->req[active].r = NULL;
		MARK_FREE(&thrd->req[active]);

		/* Start the next */
	case PL330_OP_START:
		if (!_start(thrd))
			ret = -EIO;
		break;

	default:
		ret = -EINVAL;
	}

	spin_unlock_irqrestore(&pl330->lock, flags);
	return ret;
}

int pl330_chan_status(void *ch_id, struct pl330_chanstatus *pstatus)
{
	struct pl330_thread *thrd = ch_id;
	struct pl330_dmac *pl330;
	struct pl330_info *pi;
	void __iomem *regs;
	int active;
	uint32 val;

	if (!pstatus || !thrd || thrd->free)
		return -EINVAL;

	pl330 = thrd->dmac;
	pi = pl330->pinfo;
	regs = pi->base;

	/* The client should remove the DMAC and add again */
	if (pl330->state == DYING)
		pstatus->dmac_halted = true;
	else
		pstatus->dmac_halted = false;

	val = readl((uint32)regs + FSC);
	if (val & (1 << thrd->id))
		pstatus->faulting = true;
	else
		pstatus->faulting = false;

	active = _thrd_active(thrd);

	if (!active) {
		/* Indicate that the thread is not running */
		pstatus->top_req = NULL;
		pstatus->wait_req = NULL;
	} else {
		active--;
		pstatus->top_req = thrd->req[active].r;
		pstatus->wait_req = !IS_FREE(&thrd->req[1 - active])
					? thrd->req[1 - active].r : NULL;
	}

	pstatus->src_addr = readl((uint32)regs + SA(thrd->id));
	pstatus->dst_addr = readl((uint32)regs + DA(thrd->id));

	return 0;
}

/* Reserve an event */
static inline int _alloc_event(struct pl330_thread *thrd)
{
	struct pl330_dmac *pl330 = thrd->dmac;
	struct pl330_info *pi = pl330->pinfo;
	int ev;

	for (ev = 0; ev < pi->pcfg.num_events; ev++)
		if (pl330->events[ev] == -1) {
			pl330->events[ev] = thrd->id;
			return ev;
		}

	return -1;
}

/* Upon success, returns IdentityToken for the
 * allocated channel, NULL otherwise.
 */
void *pl330_request_channel(const struct pl330_info *pi)
{
	struct pl330_thread *thrd = NULL;
	struct pl330_dmac *pl330;
	uint32 flags;
	int chans, i;

	if (!pi || !pi->pl330_data)
		return NULL;

	pl330 = pi->pl330_data;

	if (pl330->state == DYING)
		return NULL;

	chans = pi->pcfg.num_chan;

	spin_lock_irqsave(&pl330->lock, flags);

	for (i = 0; i < chans; i++) {
		thrd = &pl330->channels[i];
		if (thrd->free) {
		 //   //yk@101029 all channel use event 0;
			thrd->ev = _alloc_event(thrd);
			//thrd->ev = 0;
			if (thrd->ev >= 0) {
				thrd->free = false;
				thrd->lstenq = 1;
				thrd->req[0].r = NULL;
				MARK_FREE(&thrd->req[0]);
				thrd->req[1].r = NULL;
				MARK_FREE(&thrd->req[1]);
				break;
			}
		}
		thrd = NULL;
	}

	spin_unlock_irqrestore(&pl330->lock, flags);

	return thrd;
}

/* Release an event */
static inline void _free_event(struct pl330_thread *thrd, int ev)
{
	struct pl330_dmac *pl330 = thrd->dmac;
	struct pl330_info *pi = pl330->pinfo;

	/* If the event is valid and was held by the thread */
	if (ev >= 0 && ev < pi->pcfg.num_events
			&& pl330->events[ev] == thrd->id)
		pl330->events[ev] = -1;
}

void pl330_release_channel(void *ch_id)
{
	struct pl330_thread *thrd = ch_id;
	struct pl330_dmac *pl330;
	uint32 flags;

	if (!thrd || thrd->free)
		return;
//    		printf("pl330_release_channel DMA id = %d!\n", thrd->id);

	_stop(thrd);

	_callback(thrd->req[1 - thrd->lstenq].r, PL330_ERR_ABORT);
	_callback(thrd->req[thrd->lstenq].r, PL330_ERR_ABORT);

	pl330 = thrd->dmac;

	spin_lock_irqsave(&pl330->lock, flags);
	_free_event(thrd, thrd->ev);
	thrd->free = true;
	spin_unlock_irqrestore(&pl330->lock, flags);
}

/* Initialize the structure for PL330 configuration, that can be used
 * by the client driver the make best use of the DMAC
 */
static void read_dmac_config(struct pl330_info *pi)
{
	void __iomem *regs = pi->base;
	uint32 val;

	val = readl((uint32)regs + CRD) >> CRD_DATA_WIDTH_SHIFT;
	val &= CRD_DATA_WIDTH_MASK;
	pi->pcfg.data_bus_width = 8 * (1 << val);

	val = readl((uint32)regs + CRD) >> CRD_DATA_BUFF_SHIFT;
	val &= CRD_DATA_BUFF_MASK;
	pi->pcfg.data_buf_dep = val + 1;

	val = readl((uint32)regs + CR0) >> CR0_NUM_CHANS_SHIFT;
	val &= CR0_NUM_CHANS_MASK;
	val += 1;
	pi->pcfg.num_chan = val;

	val = readl((uint32)regs + CR0);
	if (val & CR0_PERIPH_REQ_SET) {
		val = (val >> CR0_NUM_PERIPH_SHIFT) & CR0_NUM_PERIPH_MASK;
		val += 1;
		pi->pcfg.num_peri = val;
		pi->pcfg.peri_ns = readl((uint32)regs + CR4);
	} else {
		pi->pcfg.num_peri = 0;
	}

	val = readl((uint32)regs + CR0);
	if (val & CR0_BOOT_MAN_NS)
		pi->pcfg.mode |= DMAC_MODE_NS;
	else
		pi->pcfg.mode &= ~DMAC_MODE_NS;

	val = readl((uint32)regs + CR0) >> CR0_NUM_EVENTS_SHIFT;
	val &= CR0_NUM_EVENTS_MASK;
	val += 1;
	pi->pcfg.num_events = val;

	pi->pcfg.irq_ns = readl((uint32)regs + CR3);

	pi->pcfg.periph_id = get_id(pi, PERIPH_ID);
	pi->pcfg.pcell_id = get_id(pi, PCELL_ID);
}

static inline void _reset_thread(struct pl330_thread *thrd)
{
	struct pl330_dmac *pl330 = thrd->dmac;
	struct pl330_info *pi = pl330->pinfo;

	thrd->req[0].mc_cpu = (void *)((uint32)pl330->mcode_cpu
				+ (thrd->id * pi->mcbufsz));
	thrd->req[0].mc_bus = pl330->mcode_bus
				+ (thrd->id * pi->mcbufsz);
	thrd->req[0].r = NULL;
	MARK_FREE(&thrd->req[0]);

	thrd->req[1].mc_cpu = (void *)((uint32)thrd->req[0].mc_cpu
				+ pi->mcbufsz / 2);
	thrd->req[1].mc_bus = thrd->req[0].mc_bus
				+ pi->mcbufsz / 2;
	thrd->req[1].r = NULL;
	MARK_FREE(&thrd->req[1]);
}

static int dmac_alloc_threads(struct pl330_dmac *pl330)
{
	struct pl330_info *pi = pl330->pinfo;
	int chans = pi->pcfg.num_chan;
	struct pl330_thread *thrd;
	int i;

	/* Allocate 1 Manager and 'chans' Channel threads */
	pl330->channels = (struct pl330_thread *)malloc((1 + chans) * sizeof(*thrd));
	if (!pl330->channels)
		return -ENOMEM;
	memset(pl330->channels,0,sizeof(*thrd));

	/* Init Channel threads */
	for (i = 0; i < chans; i++) {
		thrd = &pl330->channels[i];
		thrd->id = i;
		thrd->dmac = pl330;
		_reset_thread(thrd);
		thrd->free = true;
	}

	/* MANAGER is indexed at the end */
	thrd = &pl330->channels[chans];
	thrd->id = chans;
	thrd->dmac = pl330;
	thrd->free = false;
	pl330->manager = thrd;

	return 0;
}

static int dmac_alloc_resources(struct pl330_dmac *pl330)
{
	struct pl330_info *pi = pl330->pinfo;
	int chans = pi->pcfg.num_chan;
	int ret;

	/*
	 * Alloc MicroCode buffer for 'chans' Channel threads.
	 * A channel's buffer offset is (Channel_Id * MCODE_BUFF_PERCHAN)
	 */
	/*
	pl330->mcode_cpu = dma_alloc_coherent(pi->dev,
				chans * pi->mcbufsz,
				&pl330->mcode_bus, GFP_KERNEL);
	*/
	pl330->mcode_cpu = (void *)malloc(chans * pi->mcbufsz);
	if (!pl330->mcode_cpu) {
		dev_err(pi->dev, "%s:%d Can't allocate memory!\n",
			__func__, __LINE__);
		return -ENOMEM;
	}
	pl330->mcode_bus = (uint32)pl330->mcode_cpu;

	ret = dmac_alloc_threads(pl330);
	if (ret) {
		dev_err(pi->dev, "%s:%d Can't to create channels for DMAC!\n",
			__func__, __LINE__);
		/*
		dma_free_coherent(pi->dev,
				chans * pi->mcbufsz,
				pl330->mcode_cpu, pl330->mcode_bus);
        */
        free(pl330->mcode_cpu);
		return ret;
	}

	return 0;
}

int pl330_add(struct pl330_info *pi)
{
	struct pl330_dmac *pl330;
	void __iomem *regs;
	int i, ret;

	if (!pi)
		return -EINVAL;

	/* If already added */
	if (pi->pl330_data)
		return -EINVAL;

	/*
	 * If the SoC can perform reset on the DMAC, then do it
	 * before reading its configuration.
	 */
	if (pi->dmac_reset)
		pi->dmac_reset(pi);

	regs = pi->base;

	/* Check if we can handle this DMAC */
	if (get_id(pi, PERIPH_ID) != PERIPH_ID_VAL
	   || get_id(pi, PCELL_ID) != PCELL_ID_VAL) {
		dev_err(pi->dev, "PERIPH_ID 0x%x, PCELL_ID 0x%x !\n",
			readl((uint32)regs + PERIPH_ID), readl((uint32)regs + PCELL_ID));
		return -EINVAL;
	}

	/* Read the configuration of the DMAC */
	read_dmac_config(pi);

	if (pi->pcfg.num_events == 0) {
		dev_err(pi->dev, "%s:%d Can't work without events!\n",
			__func__, __LINE__);
		return -EINVAL;
	}

	pl330 = (struct pl330_dmac *)malloc(sizeof(*pl330));
	if (!pl330) {
		dev_err(pi->dev, "%s:%d Can't allocate memory!\n",
			__func__, __LINE__);
		return -ENOMEM;
	}
	memset(pl330,0,sizeof(*pl330));

	/* Assign the info structure and private data */
	pl330->pinfo = pi;
	pi->pl330_data = pl330;

	//spin_lock_init(&pl330->lock);

	//INIT_LIST_HEAD(&pl330->req_done);

	/* Use default MC buffer size if not provided */
	if (!pi->mcbufsz)
		pi->mcbufsz = MCODE_BUFF_PER_REQ * 2;

	/* Mark all events as free */
	for (i = 0; i < pi->pcfg.num_events; i++)
		pl330->events[i] = -1;

	/* Allocate resources needed by the DMAC */
	ret = dmac_alloc_resources(pl330);
	if (ret) {
		dev_err(pi->dev, "Unable to create channels for DMAC\n");
		free(pl330);
		return ret;
	}

	//tasklet_init(&pl330->tasks, pl330_dotask, (uint32) pl330);

	pl330->state = INIT;

	return 0;
}

static int dmac_free_threads(struct pl330_dmac *pl330)
{
	struct pl330_info *pi = pl330->pinfo;
	int chans = pi->pcfg.num_chan;
	struct pl330_thread *thrd;
	int i;

	/* Release Channel threads */
	for (i = 0; i < chans; i++) {
		thrd = &pl330->channels[i];
		pl330_release_channel((void *)thrd);
	}

	/* Free memory */
	free(pl330->channels);

	return 0;
}

static void dmac_free_resources(struct pl330_dmac *pl330)
{
	struct pl330_info *pi = pl330->pinfo;
	int chans = pi->pcfg.num_chan;

	dmac_free_threads(pl330);

    free(pl330->mcode_cpu);
    /* yk
	dma_free_coherent(pi->dev, chans * pi->mcbufsz,
				pl330->mcode_cpu, pl330->mcode_bus);
	*/
}

void pl330_del(struct pl330_info *pi)
{
	struct pl330_dmac *pl330;

	if (!pi || !pi->pl330_data)
		return;

	pl330 = pi->pl330_data;

	pl330->state = UNINIT;

	//tasklet_kill(&pl330->tasks);

	/* Free DMAC resources */
	dmac_free_resources(pl330);

	free(pl330);
	pi->pl330_data = NULL;
}

///////////////////////////////////////////////////////////////

#define IRQ_DMAC1_0                     32
#define IRQ_DMAC1_1                     33
#define IRQ_DMAC2_0                     34
#define IRQ_DMAC2_1                     35
struct pl330_info *pl330_info_0,*pl330_info_2;
void pl330_irq_handler(uint32 intSrc)
{
	int i;
	switch(intSrc){
		case IRQ_DMAC1_0:
		case IRQ_DMAC1_1:
			pl330_update(pl330_info_0);
			break;
#ifdef DMA2_USE
		case IRQ_DMAC2_0:
		case IRQ_DMAC2_1:
			pl330_update(pl330_info_2);
			break;
#endif
		default:
			serial_printf("Dma not support irqhandler intSrc=%d\n",intSrc);
			break;
	}
}

int DMAInit(void)
{
	//    PRINTF("DMAInit: malloc pl330_info_0 !\n");
	pl330_info_0 = (struct pl330_info *)malloc(sizeof(struct pl330_info));

	if (!pl330_info_0)
	{
	    printf("DMAInit: malloc pl330_info_0 fail!\n");
		return -ENOMEM;
	}

	pl330_info_0->pl330_data = NULL;
	pl330_info_0->dev = (void*)0xffffffff;
	pl330_info_0->dmac_reset=NULL;
	pl330_info_0->mcbufsz=0;
	pl330_info_0->base=(void*)0x20018000;
	//pl330_info_0->pcfg.num_chan = 0;//zy,2011-02-24
	pl330_add(pl330_info_0);
#ifdef DMA2_USE
	pl330_info_2 = (struct pl330_info *)malloc(sizeof(struct pl330_info));

	if (!pl330_info_2)
	{
		printf("DMAInit: malloc pl330_info_2 fail!\n");
		return -ENOMEM;
	}
	pl330_info_2->pl330_data = NULL;
	pl330_info_2->dev = (void*)0xffffffff;
	pl330_info_2->dmac_reset=NULL;
	pl330_info_2->mcbufsz=0;
	pl330_info_2->base=(void*)0x20078000;
	pl330_add(pl330_info_2);
#endif	
	if(IRQEnable(IRQ_DMAC1_0)<0){
		printf("DMAInit: fail enable irq IRQ_DMAC1_0\n");
		return -1;
	};
#if 1
	if(IRQEnable(IRQ_DMAC1_1)<0){
		printf("DMAInit: fail enable irq IRQ_DMAC1_1\n");
		return -1;
	};
#endif
#ifdef DMA2_USE
	if(IRQEnable(IRQ_DMAC2_0)<0){
		printf("DMAInit: fail enable irq IRQ_DMAC2_0\n");
		return -1;
	};
	if(IRQEnable(IRQ_DMAC2_1)<0){
		printf("DMAInit: fail enable irq IRQ_DMAC2_1\n");
		return -1;
	};
#endif

	printf("DMAInit: OK\n");

	return 0;
}
void DMADeinit(void)
{
	        pl330_del(pl330_info_0);  
			free(pl330_info_0); 
#ifdef DMA2_USE		
			pl330_del(pl330_info_2);
			free(pl330_info_2);
#endif
}
struct pl330_req DMA_pl330_req1[14];//一个EVENT对应一个DMA通道，只有3个EVENT可用，所以最多只会打开3个DMA通道
struct pl330_xfer DMA_pl330_xfer1[14];
struct pl330_reqcfg DMA_pl330_reqcfg1[14]; 
struct pl330_req DMA_pl330_req0[12];//一个EVENT对应一个DMA通道，只有3个EVENT可用，所以最多只会打开3个DMA通道
struct pl330_xfer DMA_pl330_xfer0[12];
struct pl330_reqcfg DMA_pl330_reqcfg0[12]; 

int DMAStart(uint32 dstAddr, uint32 srcAddr, uint32 size, eDMA_MODE mode,pFunc CallBack)
{
    struct pl330_req *preq;//,req;//,req1;
    struct pl330_xfer *pxfer;//,xfer;//,xfer1; 
    struct pl330_reqcfg *preqcfg;//,reqcfg;//,reqcfg1;
    struct pl330_dmac *pl330;
    struct pl330_thread *thrd;
    void *chan_id;
    uint32 req_type;
    uint32 iperi;
    uint32 ret = 0;

//    printf("------>DMAStart mode=%d \n",mode);
    chan_id = NULL;
#if 0
       preq = &req;
    pxfer = &xfer;
    preqcfg = &reqcfg;
       #endif
    // use which dma controller
    if((mode < DMA_PERI_DAMC0MAX)||(mode == DMA_M2M_DMAC0))
    {
        chan_id = pl330_request_channel(pl330_info_0);
        pl330 = pl330_info_0->pl330_data;
    }
#ifdef DMA2_USE
    else if((mode < 64)||(mode == DMA_M2M_DMAC2))
    {
        chan_id = pl330_request_channel(pl330_info_2);
        pl330 = pl330_info_2->pl330_data;
    }
#endif
    else
    {
        ret = -1;
        printf("DMAStart: parameter mode err %x", mode);
        goto done;
    }

    if(chan_id == NULL)
    {
        printf("dma request channel fail!");
    	ret = -2;
    	goto done;
    }

     // transfer peripher id
    switch(mode)
    {
        case    DMA_PERI_UART0_TX:
        case    DMA_PERI_UART0_RX:
        case    DMA_PERI_UART1_TX:
        case    DMA_PERI_UART1_RX:
        //case    DMA_PERI_I2S8ch_TX:
        //case    DMA_PERI_I2S8ch_RX:
        case    DMA_PERI_I2S_TX:
        case    DMA_PERI_I2S_RX:
        case    DMA_PERI_SPDIF_TX: 
            iperi = mode;
            break;
        //case    DMA_PERI_I2S2ch_TX:
        //case    DMA_PERI_I2S2ch_RX:
        //    iperi = mode-1;
        //    break;
        case	DMA_PERI_TSI_TX:
        case	DMA_PERI_TSI_RX:
            iperi = DMA2_PERI_TSI;
            break;
        case	DMA_PERI_SDMMC_TX:
        case	DMA_PERI_SDMMC_RX:
            iperi = DMA2_PERI_SDMMC;
            break;
        case	DMA_PERI_SDIO_TX:
        case	DMA_PERI_SDIO_RX:
            iperi = DMA2_PERI_SDIO;
            break;
        case	DMA_PERI_EMMC_TX:
        case	DMA_PERI_EMMC_RX:
            iperi = DMA2_PERI_EMMC;
            break;
        case	DMA_PERI_UART2_TX:
        case	DMA_PERI_UART2_RX:
        case	DMA_PERI_UART3_TX:
        case	DMA_PERI_UART3_RX:
        case	DMA_PERI_SPI0_TX:
        case	DMA_PERI_SPI0_RX:
        case	DMA_PERI_SPI1_TX:
        case	DMA_PERI_SPI1_RX:
            iperi = mode-32;
            break;
        case	DMA_PERI_PIDFILTER_TX:
        case	DMA_PERI_PIDFILTER_RX:
            iperi = DMA2_PERI_PIDFILTER;
            break;
        case	DMA_M2M_DMAC0:
        case	DMA_M2M_DMAC2:
            iperi = 0;
            break;
        default:
            break;
    }
    
    // transfer type
    if(mode > DMA_PERI_DMAC2MAX)
        req_type = MEMTOMEM;
    else if(mode&0x01)
        req_type = DEVTOMEM;
    else 
        req_type = MEMTODEV;

    thrd = chan_id;
    // use which dma controller
    if((mode < DMA_PERI_DAMC0MAX)||(mode == DMA_M2M_DMAC0))
    {
           preq = &DMA_pl330_req0[thrd->id];
           pxfer = &DMA_pl330_xfer0[thrd->id];
           preqcfg = &DMA_pl330_reqcfg0[thrd->id];
    }
    else// if((mode < 64)||(mode == DMA_M2M_DMAC2))
    {
           preq = &DMA_pl330_req1[thrd->id];
           pxfer = &DMA_pl330_xfer1[thrd->id];
           preqcfg = &DMA_pl330_reqcfg1[thrd->id];
    }
       
    pxfer->src_addr = srcAddr;
    pxfer->dst_addr = dstAddr;
    pxfer->bytes = size;
    pxfer->next = 0;

    preqcfg->dst_inc = 1;
    preqcfg->src_inc = 1;
    if(req_type == DEVTOMEM)
    	preqcfg->src_inc = 0;
    else if (req_type == MEMTODEV)
    	preqcfg->dst_inc = 0;
    	
    preqcfg->nonsecure = 1;
    preqcfg->privileged = 1;
    preqcfg->insnaccess = 0;
    preqcfg->brst_size = 0x3;
    preqcfg->brst_len = 0x4;
    preqcfg->dcctl = 0x0;
    preqcfg->scctl = 0;
    preqcfg->swap = 0;

    // transfer to periph, default 1 byte
    if(mode < DMA_PERI_DAMC0MAX)
    {
    	    preqcfg->nonsecure = 0;
    	    preqcfg->privileged = 0;
        preqcfg->brst_size = 0x2;
    }
    else if(req_type != 0)
        preqcfg->brst_size = 0x2;
    
    preq->rqtype = req_type;
    preq->peri = iperi;
    preq->token = (void*)0;
    preq->xfer_cb = CallBack;
    preq->x = pxfer;
    preq->cfg = preqcfg;
    ret = pl330_submit_req(chan_id,preq);
    if(ret < 0)
    {
        printf("DMAStart: pl330_submit_req fail! err:0x%x", ret);
       #if 0
           free(preq);
           free(pxfer);
           free(preqcfg);
        #endif
        goto done;
    }
    //thrd = chan_id;
    				
    _start(&pl330->channels[thrd->id]);
done:
    return ret;
}

int DMAStartM2M(uint32 dstAddr, uint32 srcAddr, uint32 size, uint32 bDMA2,pFunc CallBack)
{
    struct pl330_req *preq;//,req;
    struct pl330_xfer *pxfer;//,xfer; 
    struct pl330_reqcfg *preqcfg;//,reqcfg;
    struct pl330_dmac *pl330;
    struct pl330_thread *thrd;
    void *ptemp0;

    ptemp0=NULL;

    //preq=&req;
    //pxfer=&xfer;
    //preqcfg=&reqcfg;
#ifdef DMA2_USE
    if(bDMA2)
        ptemp0=pl330_request_channel(pl330_info_2);
    else
#endif	    
        ptemp0=pl330_request_channel(pl330_info_0);
    if(ptemp0==NULL)
    {
        printf("dma request channel fail!");
    	return -1;
    }
    	
    thrd=ptemp0;
    if(bDMA2)
    {
           preq = &DMA_pl330_req1[thrd->id];
           pxfer = &DMA_pl330_xfer1[thrd->id];
           preqcfg = &DMA_pl330_reqcfg1[thrd->id];
    }
    else
    {
           preq = &DMA_pl330_req0[thrd->id];
           pxfer = &DMA_pl330_xfer0[thrd->id];
           preqcfg = &DMA_pl330_reqcfg0[thrd->id];
    }
    pxfer->src_addr=srcAddr;
    pxfer->dst_addr=dstAddr;
    pxfer->bytes=size;
    pxfer->next=0;

    preqcfg->dst_inc=1;
    preqcfg->src_inc=1;
    preqcfg->nonsecure=0;
    preqcfg->privileged=0;
    preqcfg->insnaccess=0;
    preqcfg->brst_size=0x3;
    preqcfg->brst_len=0x4;
    preqcfg->dcctl=0x0;
    preqcfg->scctl=0;
    preqcfg->swap=0;

    preq->rqtype=MEMTOMEM;
    preq->peri=0;
    preq->token=(void*)0;
    preq->xfer_cb=CallBack;
    preq->x=pxfer;
    preq->cfg=preqcfg;
    pl330_submit_req(ptemp0,preq);


#ifdef DMA2_USE
    if(bDMA2)
    	pl330 = pl330_info_2->pl330_data;	
    else
#endif	    
        pl330 = pl330_info_0->pl330_data;
    			
    _start(&pl330->channels[thrd->id]);

    return 0;
}
#if 0
int DMAStop(eDMA_MODE mode)
{
    struct pl330_req *preq,req;
	struct pl330_xfer *pxfer,xfer; 
	struct pl330_reqcfg *preqcfg,reqcfg;
	struct pl330_dmac *pl330;
	struct pl330_thread *thrd;
	void *chan_id;
	uint32 req_type;
	uint32 iperi;
	uint32 ret = 0;

	chan_id = NULL;
	
	preq = &req;
	pxfer = &xfer;
	preqcfg = &reqcfg;

	dmac_free_threads(pl330);
}
#endif

