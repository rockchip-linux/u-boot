/*
 * (C) Copyright 2016 Rockchip Electronics
 * Shawn Lin <shawn.lin@rock-chips.com>
 * Wenrui Li <wenrui.li@rock-chips.com>
 *
 * Bits taken from linux nvme-core w/o
 * block multi-queue support
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <linux/compat.h>
#include <linux/err.h>
#include <linux/string.h>
#include <linux/types.h>
#include "pcie.h"

#define PCI_COMMAND_MEMORY		0x2
#define PCI_COMMAND_MASTER		0x4	/* Enable bus mastering */
#define PCI_COMMAND_INTX_DISABLE	0x400	/* INTx Emulation Disable */
#define PCI_COMMAND			0x04	/* 16 bits */
#define SQ_SIZE(depth)			(depth * sizeof(struct nvme_command))
#define CQ_SIZE(depth)			(depth * sizeof(struct nvme_completion))
#define BITS_PER_BYTE 8
#define BITS_TO_LONGS(nr) \
	DIV_ROUND_UP(nr, BITS_PER_BYTE * sizeof(long))
#define PAGE_SHIFT			12
#define PAGE_MASK			(~(PAGE_SIZE - 1))
#define NVME_VS(major, minor)		(((major) << 16) | ((minor) << 8))
#define NVME_MAX_TIMEOUT		60000000
#define UINT_MAX			(~0U)
#define MAX_REQ_SECTOR			PAGE_SIZE

struct nvme_queue {
	struct nvme_dev *dev;
	struct nvme_command *sq_cmds;
	struct nvme_completion *cqes;
	dma_addr_t sq_dma_addr;
	dma_addr_t cq_dma_addr;
	u32 __iomem *q_db;
	u16 q_depth;
	u16 sq_tail;
	u16 sq_head;
	u16 cq_head;
	u16 qid;
	u8 cq_phase;
	unsigned long cmdid_data[];
};

struct nvme_dev {
	struct nvme_queue **queues;
	u32 __iomem *dbs;
	unsigned char *prp_page_pool;
	unsigned char *prp_small_pool;
	unsigned queue_count;
	int q_depth;
	u32 db_stride;
	u32 ctrl_config;
	void __iomem *bar;
	void __iomem *config_space;
	int lba_shift;
	u64 nsize;
	u64 ncap;
	bool subsystem;
	u32 max_hw_sectors;
	u32 stripe_size;
};

static struct nvme_dev ssd_dev;
static struct pcie_bus *pbus;

enum {
	NVME_REG_CAP	= 0x0000,	/* Controller Capabilities */
	NVME_REG_VS	= 0x0008,	/* Version */
	NVME_REG_INTMS	= 0x000c,	/* Interrupt Mask Set */
	NVME_REG_INTMC	= 0x0010,	/* Interrupt Mask Clear */
	NVME_REG_CC	= 0x0014,	/* Controller Configuration */
	NVME_REG_CSTS	= 0x001c,	/* Controller Status */
	NVME_REG_NSSR	= 0x0020,	/* NVM Subsystem Reset */
	NVME_REG_AQA	= 0x0024,	/* Admin Queue Attributes */
	NVME_REG_ASQ	= 0x0028,	/* Admin SQ Base Address */
	NVME_REG_ACQ	= 0x0030,	/* Admin CQ Base Address */
	NVME_REG_CMBLOC = 0x0038,	/* Controller Memory Buffer Location */
	NVME_REG_CMBSZ	= 0x003c,	/* Controller Memory Buffer Size */
};

#define NVME_NVM_IOSQES		6
#define NVME_NVM_IOCQES		4
enum {
	NVME_CC_ENABLE		= 1 << 0,
	NVME_CC_CSS_NVM		= 0 << 4,
	NVME_CC_MPS_SHIFT	= 7,
	NVME_CC_ARB_RR		= 0 << 11,
	NVME_CC_ARB_WRRU	= 1 << 11,
	NVME_CC_ARB_VS		= 7 << 11,
	NVME_CC_SHN_NONE	= 0 << 14,
	NVME_CC_SHN_NORMAL	= 1 << 14,
	NVME_CC_SHN_ABRUPT	= 2 << 14,
	NVME_CC_SHN_MASK	= 3 << 14,
	NVME_CC_IOSQES		= NVME_NVM_IOSQES << 16,
	NVME_CC_IOCQES		= NVME_NVM_IOCQES << 20,
	NVME_CSTS_RDY		= 1 << 0,
	NVME_CSTS_CFS		= 1 << 1,
	NVME_CSTS_NSSRO		= 1 << 4,
	NVME_CSTS_SHST_NORMAL	= 0 << 2,
	NVME_CSTS_SHST_OCCUR	= 1 << 2,
	NVME_CSTS_SHST_CMPLT	= 2 << 2,
	NVME_CSTS_SHST_MASK	= 3 << 2,
};

enum nvme_admin_opcode {
	nvme_admin_delete_sq		= 0x00,
	nvme_admin_create_sq		= 0x01,
	nvme_admin_get_log_page		= 0x02,
	nvme_admin_delete_cq		= 0x04,
	nvme_admin_create_cq		= 0x05,
	nvme_admin_identify		= 0x06,
	nvme_admin_abort_cmd		= 0x08,
	nvme_admin_set_features		= 0x09,
	nvme_admin_get_features		= 0x0a,
	nvme_admin_async_event		= 0x0c,
	nvme_admin_activate_fw		= 0x10,
	nvme_admin_download_fw		= 0x11,
	nvme_admin_keep_alive		= 0x18,
	nvme_admin_format_nvm		= 0x80,
	nvme_admin_security_send	= 0x81,
	nvme_admin_security_recv	= 0x82,
};

enum {
	NVME_QUEUE_PHYS_CONTIG	= (1 << 0),
	NVME_CQ_IRQ_ENABLED	= (1 << 1),
	NVME_SQ_PRIO_URGENT	= (0 << 1),
	NVME_SQ_PRIO_HIGH	= (1 << 1),
	NVME_SQ_PRIO_MEDIUM	= (2 << 1),
	NVME_SQ_PRIO_LOW	= (3 << 1),
	NVME_FEAT_ARBITRATION	= 0x01,
	NVME_FEAT_POWER_MGMT	= 0x02,
	NVME_FEAT_LBA_RANGE	= 0x03,
	NVME_FEAT_TEMP_THRESH	= 0x04,
	NVME_FEAT_ERR_RECOVERY	= 0x05,
	NVME_FEAT_VOLATILE_WC	= 0x06,
	NVME_FEAT_NUM_QUEUES	= 0x07,
	NVME_FEAT_IRQ_COALESCE	= 0x08,
	NVME_FEAT_IRQ_CONFIG	= 0x09,
	NVME_FEAT_WRITE_ATOMIC	= 0x0a,
	NVME_FEAT_ASYNC_EVENT	= 0x0b,
	NVME_FEAT_AUTO_PST	= 0x0c,
	NVME_FEAT_KATO		= 0x0f,
	NVME_FEAT_SW_PROGRESS	= 0x80,
	NVME_FEAT_HOST_ID	= 0x81,
	NVME_FEAT_RESV_MASK	= 0x82,
	NVME_FEAT_RESV_PERSIST	= 0x83,
	NVME_LOG_ERROR		= 0x01,
	NVME_LOG_SMART		= 0x02,
	NVME_LOG_FW_SLOT	= 0x03,
	NVME_LOG_DISC		= 0x70,
	NVME_LOG_RESERVATION	= 0x80,
	NVME_FWACT_REPL		= (0 << 3),
	NVME_FWACT_REPL_ACTV	= (1 << 3),
	NVME_FWACT_ACTV		= (2 << 3),
};

struct nvme_sgl_desc {
	__le64	addr;
	__le32	length;
	__u8	rsvd[3];
	__u8	type;
};

struct nvme_keyed_sgl_desc {
	__le64	addr;
	__u8	length[3];
	__u8	key[4];
	__u8	type;
};

struct nvme_common_command {
	__u8			opcode;
	__u8			flags;
	__u16			command_id;
	__le32			nsid;
	__le32			cdw2[2];
	__le64			metadata;
	__le64			prp1;
	__le64			prp2;
	__le32			cdw10[6];
};

struct nvme_rw_command {
	__u8			opcode;
	__u8			flags;
	__u16			command_id;
	__le32			nsid;
	__u64			rsvd2;
	__le64			metadata;
	__le64			prp1;
	__le64			prp2;
	__le64			slba;
	__le16			length;
	__le16			control;
	__le32			dsmgmt;
	__le32			reftag;
	__le16			apptag;
	__le16			appmask;
};

struct nvme_id_power_state {
	__le16			max_power;	/* centiwatts */
	__u8			rsvd2;
	__u8			flags;
	__le32			entry_lat;	/* microseconds */
	__le32			exit_lat;	/* microseconds */
	__u8			read_tput;
	__u8			read_lat;
	__u8			write_tput;
	__u8			write_lat;
	__le16			idle_power;
	__u8			idle_scale;
	__u8			rsvd19;
	__le16			active_power;
	__u8			active_work_scale;
	__u8			rsvd23[9];
};

struct nvme_id_ctrl {
	__le16			vid;
	__le16			ssvid;
	char			sn[20];
	char			mn[40];
	char			fr[8];
	__u8			rab;
	__u8			ieee[3];
	__u8			mic;
	__u8			mdts;
	__le16			cntlid;
	__le32			ver;
	__le32			rtd3r;
	__le32			rtd3e;
	__le32			oaes;
	__le32			ctratt;
	__u8			rsvd100[156];
	__le16			oacs;
	__u8			acl;
	__u8			aerl;
	__u8			frmw;
	__u8			lpa;
	__u8			elpe;
	__u8			npss;
	__u8			avscc;
	__u8			apsta;
	__le16			wctemp;
	__le16			cctemp;
	__u8			rsvd270[50];
	__le16			kas;
	__u8			rsvd322[190];
	__u8			sqes;
	__u8			cqes;
	__le16			maxcmd;
	__le32			nn;
	__le16			oncs;
	__le16			fuses;
	__u8			fna;
	__u8			vwc;
	__le16			awun;
	__le16			awupf;
	__u8			nvscc;
	__u8			rsvd531;
	__le16			acwu;
	__u8			rsvd534[2];
	__le32			sgls;
	__u8			rsvd540[228];
	char			subnqn[256];
	__u8			rsvd1024[768];
	__le32			ioccsz;
	__le32			iorcsz;
	__le16			icdoff;
	__u8			ctrattr;
	__u8			msdbd;
	__u8			rsvd1804[244];
	struct nvme_id_power_state	psd[32];
	__u8			vs[1024];
};

struct nvme_identify {
	__u8			opcode;
	__u8			flags;
	__u16			command_id;
	__le32			nsid;
	__u64			rsvd2[2];
	__le64			prp1;
	__le64			prp2;
	__le32			cns;
	__u32			rsvd11[5];
};

struct nvme_features {
	__u8			opcode;
	__u8			flags;
	__u16			command_id;
	__le32			nsid;
	__u64			rsvd2[2];
	__le64			prp1;
	__le64			prp2;
	__le32			fid;
	__le32			dword11;
	__u32			rsvd12[4];
};

struct nvme_create_cq {
	__u8			opcode;
	__u8			flags;
	__u16			command_id;
	__u32			rsvd1[5];
	__le64			prp1;
	__u64			rsvd8;
	__le16			cqid;
	__le16			qsize;
	__le16			cq_flags;
	__le16			irq_vector;
	__u32			rsvd12[4];
};

struct nvme_create_sq {
	__u8			opcode;
	__u8			flags;
	__u16			command_id;
	__u32			rsvd1[5];
	__le64			prp1;
	__u64			rsvd8;
	__le16			sqid;
	__le16			qsize;
	__le16			sq_flags;
	__le16			cqid;
	__u32			rsvd12[4];
};

struct nvme_delete_queue {
	__u8			opcode;
	__u8			flags;
	__u16			command_id;
	__u32			rsvd1[9];
	__le16			qid;
	__u16			rsvd10;
	__u32			rsvd11[5];
};

struct nvme_command {
	union {
		struct nvme_common_command common;
		struct nvme_rw_command rw;
		struct nvme_identify identify;
		struct nvme_features features;
		struct nvme_create_cq create_cq;
		struct nvme_create_sq create_sq;
		struct nvme_delete_queue delete_queue;
	};
};

enum nvme_opcode {
	nvme_cmd_flush		= 0x00,
	nvme_cmd_write		= 0x01,
	nvme_cmd_read		= 0x02,
	nvme_cmd_write_uncor	= 0x04,
	nvme_cmd_compare	= 0x05,
	nvme_cmd_write_zeroes	= 0x08,
	nvme_cmd_dsm		= 0x09,
	nvme_cmd_resv_register	= 0x0d,
	nvme_cmd_resv_report	= 0x0e,
	nvme_cmd_resv_acquire	= 0x11,
	nvme_cmd_resv_release	= 0x15,
};

struct nvme_lbaf {
	__le16			ms;
	__u8			ds;
	__u8			rp;
};

struct nvme_id_ns {
	__le64			nsze;
	__le64			ncap;
	__le64			nuse;
	__u8			nsfeat;
	__u8			nlbaf;
	__u8			flbas;
	__u8			mc;
	__u8			dpc;
	__u8			dps;
	__u8			nmic;
	__u8			rescap;
	__u8			fpi;
	__u8			rsvd33;
	__le16			nawun;
	__le16			nawupf;
	__le16			nacwu;
	__le16			nabsn;
	__le16			nabo;
	__le16			nabspf;
	__u16			rsvd46;
	__le64			nvmcap[2];
	__u8			rsvd64[40];
	__u8			nguid[16];
	__u8			eui64[8];
	struct nvme_lbaf	lbaf[16];
	__u8			rsvd192[192];
	__u8			vs[3712];
};

enum {
	/*
	 * Generic Command Status:
	 */
	NVME_SC_SUCCESS			= 0x0,
	NVME_SC_INVALID_OPCODE		= 0x1,
	NVME_SC_INVALID_FIELD		= 0x2,
	NVME_SC_CMDID_CONFLICT		= 0x3,
	NVME_SC_DATA_XFER_ERROR		= 0x4,
	NVME_SC_POWER_LOSS		= 0x5,
	NVME_SC_INTERNAL		= 0x6,
	NVME_SC_ABORT_REQ		= 0x7,
	NVME_SC_ABORT_QUEUE		= 0x8,
	NVME_SC_FUSED_FAIL		= 0x9,
	NVME_SC_FUSED_MISSING		= 0xa,
	NVME_SC_INVALID_NS		= 0xb,
	NVME_SC_CMD_SEQ_ERROR		= 0xc,
	NVME_SC_SGL_INVALID_LAST	= 0xd,
	NVME_SC_SGL_INVALID_COUNT	= 0xe,
	NVME_SC_SGL_INVALID_DATA	= 0xf,
	NVME_SC_SGL_INVALID_METADATA	= 0x10,
	NVME_SC_SGL_INVALID_TYPE	= 0x11,

	NVME_SC_SGL_INVALID_OFFSET	= 0x16,
	NVME_SC_SGL_INVALID_SUBTYPE	= 0x17,

	NVME_SC_LBA_RANGE		= 0x80,
	NVME_SC_CAP_EXCEEDED		= 0x81,
	NVME_SC_NS_NOT_READY		= 0x82,
	NVME_SC_RESERVATION_CONFLICT	= 0x83,

	/*
	 * Command Specific Status:
	 */
	NVME_SC_CQ_INVALID		= 0x100,
	NVME_SC_QID_INVALID		= 0x101,
	NVME_SC_QUEUE_SIZE		= 0x102,
	NVME_SC_ABORT_LIMIT		= 0x103,
	NVME_SC_ABORT_MISSING		= 0x104,
	NVME_SC_ASYNC_LIMIT		= 0x105,
	NVME_SC_FIRMWARE_SLOT		= 0x106,
	NVME_SC_FIRMWARE_IMAGE		= 0x107,
	NVME_SC_INVALID_VECTOR		= 0x108,
	NVME_SC_INVALID_LOG_PAGE	= 0x109,
	NVME_SC_INVALID_FORMAT		= 0x10a,
	NVME_SC_FIRMWARE_NEEDS_RESET	= 0x10b,
	NVME_SC_INVALID_QUEUE		= 0x10c,
	NVME_SC_FEATURE_NOT_SAVEABLE	= 0x10d,
	NVME_SC_FEATURE_NOT_CHANGEABLE	= 0x10e,
	NVME_SC_FEATURE_NOT_PER_NS	= 0x10f,
	NVME_SC_FW_NEEDS_RESET_SUBSYS	= 0x110,

	/*
	 * I/O Command Set Specific - NVM commands:
	 */
	NVME_SC_BAD_ATTRIBUTES		= 0x180,
	NVME_SC_INVALID_PI		= 0x181,
	NVME_SC_READ_ONLY		= 0x182,

	/*
	 * I/O Command Set Specific - Fabrics commands:
	 */
	NVME_SC_CONNECT_FORMAT		= 0x180,
	NVME_SC_CONNECT_CTRL_BUSY	= 0x181,
	NVME_SC_CONNECT_INVALID_PARAM	= 0x182,
	NVME_SC_CONNECT_RESTART_DISC	= 0x183,
	NVME_SC_CONNECT_INVALID_HOST	= 0x184,

	NVME_SC_DISCOVERY_RESTART	= 0x190,
	NVME_SC_AUTH_REQUIRED		= 0x191,

	/*
	 * Media and Data Integrity Errors:
	 */
	NVME_SC_WRITE_FAULT		= 0x280,
	NVME_SC_READ_ERROR		= 0x281,
	NVME_SC_GUARD_CHECK		= 0x282,
	NVME_SC_APPTAG_CHECK		= 0x283,
	NVME_SC_REFTAG_CHECK		= 0x284,
	NVME_SC_COMPARE_FAILED		= 0x285,
	NVME_SC_ACCESS_DENIED		= 0x286,

	NVME_SC_DNR			= 0x4000,
};

/*
 * Shawn: should align to page(consisten with NVMe cfg), though
 * some NVMe-based SSDs could support sector aligned
 */
static __aligned(4096) unsigned char prp_page_pool[MAX_REQ_SECTOR];
static __aligned(4096) unsigned char prp_small_pool[256];
static __aligned(512) u8 *tbuf;;

#define NVME_CAP_MQES(cap)	((cap) & 0xffff)
#define NVME_CAP_TIMEOUT(cap)	(((cap) >> 24) & 0xff)
#define NVME_CAP_STRIDE(cap)	(((cap) >> 32) & 0xf)
#define NVME_CAP_NSSRC(cap)	(((cap) >> 36) & 0x1)
#define NVME_CAP_MPSMIN(cap)	(((cap) >> 48) & 0xf)
#define NVME_CAP_MPSMAX(cap)	(((cap) >> 52) & 0xf)

struct sync_cmd_info {
	u32 result;
	int status;
};

struct nvme_completion {
	/*
	 * Used by Admin and Fabrics commands to return data:
	 */
	union {
		__le16	result16;
		__le32	result;
		__le64	result64;
	};
	__le16	sq_head;	/* how much of this queue may be reclaimed */
	__le16	sq_id;		/* submission queue that generated this entry */
	__u16	command_id;	/* of the command which completed */
	__le16	status;		/* did the command fail, and if so, why? */
};

static __aligned(4096) struct nvme_command admin_cmd[2];
static __aligned(4096) struct nvme_completion admin_cmp[2];
static __aligned(4096) struct nvme_command io_cmd[512];
static __aligned(4096) struct nvme_completion io_cmp[512];

int nvme_pci_enable_device(struct nvme_dev *dev)
{
	u32 cmd;

	pbus->ops->rd_ep_conf(pbus->priv, PCI_COMMAND, 2, &cmd);
	cmd = (cmd | PCI_COMMAND_MEMORY | PCI_COMMAND_MASTER);
	pbus->ops->wr_ep_conf(pbus->priv, PCI_COMMAND, 2, cmd);

	return 0;
}

static int nvme_wait_ready(struct nvme_dev *dev, u64 cap, bool enabled)
{
	unsigned long timeout =	NVME_CAP_TIMEOUT(cap);
	u32 bit = enabled ? NVME_CSTS_RDY : 0;

	while ((readl(dev->bar + NVME_REG_CSTS) & NVME_CSTS_RDY) != bit) {
		mdelay(500);
		timeout--;
		if (!timeout)
			return -1;
	}
	return 0;
}

typedef void (*nvme_completion_fn)(struct nvme_dev *, void *,
				   struct nvme_completion *);

struct nvme_cmd_info {
	nvme_completion_fn fn;
	void *ctx;
	unsigned long timeout;
};

static void special_completion(struct nvme_dev *dev, void *ctx,
			       struct nvme_completion *cqe)
{
}

static struct nvme_cmd_info *nvme_cmd_info(struct nvme_queue *nvmeq)
{
	return (void *)&nvmeq->cmdid_data[BITS_TO_LONGS(nvmeq->q_depth)];
}

static void *cancel_cmdid(struct nvme_queue *nvmeq, int cmdid,
			  nvme_completion_fn *fn)
{
	void *ctx;
	struct nvme_cmd_info *info = nvme_cmd_info(nvmeq);

	if (fn)
		*fn = info[cmdid].fn;
	ctx = info[cmdid].ctx;
	info[cmdid].fn = special_completion;
	info[cmdid].ctx = 0;
	return ctx;
}

static void nvme_abort_command(struct nvme_queue *nvmeq, int cmdid)
{
	cancel_cmdid(nvmeq, cmdid, NULL);
}

static unsigned nvme_queue_extra(int depth)
{
	return DIV_ROUND_UP(depth, 8) + (depth * sizeof(struct nvme_cmd_info));
}

static int nvme_disable_ctrl(struct nvme_dev *dev, u64 cap)
{
	u32 cc = readl(dev->bar + NVME_REG_CC);

	dev->ctrl_config &= ~NVME_CC_SHN_MASK;
	dev->ctrl_config &= ~NVME_CC_ENABLE;

	if (cc & NVME_CC_ENABLE)
		writel(cc & ~NVME_CC_ENABLE, (dev->bar + NVME_REG_CC));
	return nvme_wait_ready(dev, cap, false);
}

static int nvme_enable_ctrl(struct nvme_dev *dev, u64 cap)
{
	return nvme_wait_ready(dev, cap, true);
}

static int alloc_cmdid(struct nvme_queue *nvmeq, void *ctx,
		       nvme_completion_fn handler, unsigned timeout)
{
	int depth = nvmeq->q_depth - 1;
	struct nvme_cmd_info *info = nvme_cmd_info(nvmeq);
	int cmdid;

	do {
		cmdid = ffz(nvmeq->cmdid_data[0]);
		if (cmdid >= depth)
			return -EBUSY;
	} while (test_and_set_bit(cmdid, nvmeq->cmdid_data));

	info[cmdid].fn = handler;
	info[cmdid].ctx = ctx;
	info[cmdid].timeout = timeout;
	return cmdid;
}

static void sync_completion(struct nvme_dev *dev, void *ctx,
			    struct nvme_completion *cqe)
{
	struct sync_cmd_info *cmdinfo = ctx;

	cmdinfo->result = le32_to_cpup(&cqe->result);
	cmdinfo->status = le16_to_cpup(&cqe->status) >> 1;
}

static int nvme_submit_cmd(struct nvme_queue *nvmeq, struct nvme_command *cmd)
{
	u16 tail;

	tail = nvmeq->sq_tail;
	memcpy(&nvmeq->sq_cmds[tail], cmd, sizeof(*cmd));
	CacheFlushDRegion((u32)(unsigned long)&nvmeq->sq_cmds[tail],
			  sizeof(*cmd));
	if (++tail == nvmeq->q_depth)
		tail = 0;

	__raw_writel(tail, nvmeq->q_db);
	mb(); /* drain buffer */
	CacheFlushDRegion((u32)(unsigned long)nvmeq->q_db, 4);
	nvmeq->sq_tail = tail;

	return 0;
}

static void *free_cmdid(struct nvme_queue *nvmeq, int cmdid,
			nvme_completion_fn *fn)
{
	void *ctx;
	struct nvme_cmd_info *info = nvme_cmd_info(nvmeq);

	if (cmdid >= nvmeq->q_depth) {
		*fn = special_completion;
		return NULL;
	}

	if (fn)
		*fn = info[cmdid].fn;

	ctx = info[cmdid].ctx;
	info[cmdid].fn = special_completion;
	info[cmdid].ctx = NULL;
	__clear_bit(cmdid, nvmeq->cmdid_data);
	return ctx;
}

static int nvme_process_cq(struct nvme_queue *nvmeq)
{
	u16 head, phase;
	int timeout = NVME_MAX_TIMEOUT;
	void *ctx;
	struct nvme_completion cqe;
	nvme_completion_fn fn;

	head = nvmeq->cq_head;
	phase = nvmeq->cq_phase;
	cqe = nvmeq->cqes[head];

	for (;;) {
		CacheInvalidateDRegion((u32)(unsigned long)&nvmeq->cqes[head],
				       sizeof(struct nvme_completion));
		memcpy(&cqe, &nvmeq->cqes[head],
		       sizeof(struct nvme_completion));
		mb(); /* drain buffer */
		if ((le16_to_cpu(cqe.status) & 1) != phase) {
			timeout--;
			udelay(1);
			if (!timeout)
				break;
		} else {
			break;
		}
	}

	if (!timeout) {
		printf("NVMe: process complete queue timeout\n");
		return -1;
	}

	nvmeq->sq_head = le16_to_cpu(cqe.sq_head);
	if (++head == nvmeq->q_depth) {
		head = 0;
		phase = !phase;
	}

	ctx = free_cmdid(nvmeq, cqe.command_id, &fn);
	fn(nvmeq->dev, ctx, &cqe);

	if (head == nvmeq->cq_head && phase == nvmeq->cq_phase)
		return 0;

	__raw_writel(head, nvmeq->q_db + (1 << nvmeq->dev->db_stride));
	nvmeq->cq_head = head;
	nvmeq->cq_phase = phase;
	return 1;
}

int nvme_submit_sync_cmd(struct nvme_queue *nvmeq, struct nvme_command *cmd,
			 u32 *result, unsigned timeout)
{
	int cmdid;
	struct sync_cmd_info cmdinfo;
	int ret;

	cmdinfo.status = -EINTR;

	cmdid = alloc_cmdid(nvmeq, &cmdinfo, sync_completion, timeout);
	if (cmdid < 0)
		return cmdid;

	cmd->common.command_id = cmdid;

	nvme_submit_cmd(nvmeq, cmd);

	ret = nvme_process_cq(nvmeq);
	if (ret < 0)
		return ret;

	if (cmdinfo.status == -EINTR) {
		nvme_abort_command(nvmeq, cmdid);
		return -EINTR;
	}

	if (result)
		*result = cmdinfo.result;

	return cmdinfo.status;
}

int nvme_submit_admin_cmd(struct nvme_dev *dev, struct nvme_command *cmd,
			  u32 *result)
{
	return nvme_submit_sync_cmd(dev->queues[0], cmd,
				    result, NVME_MAX_TIMEOUT);
}

static void nvme_init_queue(struct nvme_queue *nvmeq, u16 qid)
{
	unsigned extra = nvme_queue_extra(nvmeq->q_depth);

	nvmeq->sq_tail = 0;
	nvmeq->cq_head = 0;
	nvmeq->cq_phase = 1;
	memset(nvmeq->cmdid_data, 0, extra);
	memset((void *)nvmeq->cqes, 0, CQ_SIZE(nvmeq->q_depth));
}

static struct nvme_queue *nvme_alloc_queue(struct nvme_dev *dev, int qid,
					   int depth, int vector)
{
	unsigned extra = nvme_queue_extra(depth);
	struct nvme_queue *nvmeq = malloc(sizeof(*nvmeq) + extra);

	if (!nvmeq)
		return NULL;

	memset(nvmeq, 0x0, sizeof(*nvmeq) + extra);
	if (qid == 0) {
		nvmeq->cqes = admin_cmp;
		nvmeq->sq_cmds = admin_cmd;
	} else {
		nvmeq->cqes = io_cmp;
		nvmeq->sq_cmds = io_cmd;
	}

	memset((void *)nvmeq->cqes, 0, CQ_SIZE(depth));
	memset((void *)nvmeq->sq_cmds, 0, SQ_SIZE(depth));

	nvmeq->dev = dev;
	nvmeq->qid = qid;
	nvmeq->cq_head = 0;
	nvmeq->cq_phase = 1;
	nvmeq->q_db = &dev->dbs[qid << (dev->db_stride + 1)];
	nvmeq->q_depth = depth;
	nvmeq->sq_dma_addr = (dma_addr_t)nvmeq->sq_cmds;
	nvmeq->cq_dma_addr = (dma_addr_t)nvmeq->cqes;
	dev->queue_count++;
	return nvmeq;
}

static inline __u64 lo_hi_readq(const void __iomem *addr)
{
	const u32 __iomem *p = addr;
	u32 low, high;

	low = readl(p);
	high = readl(p + 1);

	return low + ((u64)high << 32);
}

static int nvme_configure_admin_queue(struct nvme_dev *dev)
{
	int result;
	u32 aqa;
	u64 cap = __raw_readq(dev->bar + NVME_REG_CAP);
	struct nvme_queue *nvmeq;

	dev->subsystem = readl(dev->bar + NVME_REG_VS) >= NVME_VS(1, 1) ?
			       NVME_CAP_NSSRC(cap) : 0;

	if (dev->subsystem &&
	    (readl(dev->bar + NVME_REG_CSTS) & NVME_CSTS_NSSRO))
		writel(NVME_CSTS_NSSRO, dev->bar + NVME_REG_CSTS);

	result = nvme_disable_ctrl(dev, cap);
	if (result < 0) {
		printf("NVMe: Disalbe nvme ctrl failed\n");
		return result;
	}

	nvmeq = dev->queues[0];
	if (!nvmeq) {
		/* alloc admin queue */
		nvmeq = nvme_alloc_queue(dev, 0, 2, 0);
		if (!nvmeq) {
			printf("NVMe: Alloc nvme admin queue failed\n");
			return -ENOMEM;
		}
		dev->queues[0] = nvmeq;
	}

	aqa = nvmeq->q_depth - 1;
	aqa |= aqa << 16;
	dev->ctrl_config = NVME_CC_ENABLE | NVME_CC_CSS_NVM;
	dev->ctrl_config |= (PAGE_SHIFT - 12) << NVME_CC_MPS_SHIFT;
	dev->ctrl_config |= NVME_CC_ARB_RR | NVME_CC_SHN_NONE;
	dev->ctrl_config |= NVME_CC_IOSQES | NVME_CC_IOCQES;

	__raw_writel(aqa, dev->bar + NVME_REG_AQA);
	__raw_writeq(nvmeq->sq_dma_addr, dev->bar + NVME_REG_ASQ);
	__raw_writeq(nvmeq->cq_dma_addr, dev->bar + NVME_REG_ACQ);
	nvmeq->sq_dma_addr = 0;
	nvmeq->sq_dma_addr = __raw_readq(dev->bar + NVME_REG_ASQ);
	nvmeq->cq_dma_addr = __raw_readq(dev->bar + NVME_REG_ACQ);

	__raw_writel(dev->ctrl_config, dev->bar + NVME_REG_CC);
	dev->ctrl_config = 0;
	dev->ctrl_config = __raw_readl(dev->bar + NVME_REG_CC);

	result = nvme_enable_ctrl(dev, cap);
	if (result) {
		dev->queues[0] = NULL;
		free(nvmeq);
		printf("NVMe: Enable nvme ctrl failed\n");
		return result;
	}
	nvme_init_queue(nvmeq, 0);
	return result;
}

static int nvme_dev_map(struct nvme_dev *dev)
{
	int result = -ENOMEM, i;
	u64 cap;
	bool found = false;

	result = nvme_pci_enable_device(dev);
	if (result < 0) {
		printf("NVMe: pcie enable failed\n");
		return result;
	}

	dev->bar = 0x0;
	for (i = 0; i < pbus->region_count; i++) {
		if (pbus->regions[i].flags == PCI_REGION_MEM) {
			found = true;
			break;
		}
	}

	if (!found)
		return result;

	dev->bar = (void __iomem *)((u64)pbus->regions[i].bus_start);

	/* read 64bit cap */
	cap = __raw_readq(dev->bar + NVME_REG_CAP);

	dev->db_stride = NVME_CAP_STRIDE(cap);
	dev->dbs = dev->bar + 4096;
	return 0;
}

int nvme_get_features(struct nvme_dev *dev, unsigned fid, unsigned nsid,
		      dma_addr_t dma_addr, u32 *result)
{
	struct nvme_command c;

	memset(&c, 0, sizeof(c));
	c.features.opcode = nvme_admin_get_features;
	c.features.nsid = cpu_to_le32(nsid);
	c.features.prp1 = cpu_to_le64(dma_addr);
	c.features.fid = cpu_to_le32(fid);

	return nvme_submit_admin_cmd(dev, &c, result);
}

int nvme_set_features(struct nvme_dev *dev, unsigned fid, unsigned dword11,
		      dma_addr_t dma_addr, u32 *result)
{
	struct nvme_command c;

	memset(&c, 0, sizeof(c));
	c.features.opcode = nvme_admin_set_features;
	c.features.prp1 = cpu_to_le64(dma_addr);
	c.features.fid = cpu_to_le32(fid);
	c.features.dword11 = cpu_to_le32(dword11);

	return nvme_submit_admin_cmd(dev, &c, result);
}

static int set_queue_count(struct nvme_dev *dev, int count)
{
	int status;
	u32 result;
	u32 q_count = (count - 1) | ((count - 1) << 16);

	status = nvme_set_features(dev, NVME_FEAT_NUM_QUEUES,
				   q_count, 0, &result);
	if (status)
		return status < 0 ? -EIO : -EBUSY;

	return min(result & 0xffff, result >> 16) + 1;
}

static int adapter_alloc_cq(struct nvme_dev *dev, u16 qid,
			    struct nvme_queue *nvmeq)
{
	struct nvme_command c;
	int flags = NVME_QUEUE_PHYS_CONTIG | NVME_CQ_IRQ_ENABLED;

	/*
	 * Note: we (ab)use the fact the the prp fields survive if no data
	 * is attached to the request.
	 */
	memset(&c, 0, sizeof(c));
	c.create_cq.opcode = nvme_admin_create_cq;
	c.create_cq.prp1 = cpu_to_le64(nvmeq->cq_dma_addr);
	c.create_cq.cqid = cpu_to_le16(qid);
	c.create_cq.qsize = cpu_to_le16(nvmeq->q_depth - 1);
	c.create_cq.cq_flags = cpu_to_le16(flags);

	return nvme_submit_admin_cmd(dev, &c, NULL);
}

static int adapter_alloc_sq(struct nvme_dev *dev, u16 qid,
			    struct nvme_queue *nvmeq)
{
	struct nvme_command c;
	int flags = NVME_QUEUE_PHYS_CONTIG | NVME_SQ_PRIO_MEDIUM;

	/*
	 * Note: we (ab)use the fact the the prp fields survive if no data
	 * is attached to the request.
	 */
	memset(&c, 0, sizeof(c));
	c.create_sq.opcode = nvme_admin_create_sq;
	c.create_sq.prp1 = cpu_to_le64(nvmeq->sq_dma_addr);
	c.create_sq.sqid = cpu_to_le16(qid);
	c.create_sq.qsize = cpu_to_le16(nvmeq->q_depth - 1);
	c.create_sq.sq_flags = cpu_to_le16(flags);
	c.create_sq.cqid = cpu_to_le16(qid);

	return nvme_submit_admin_cmd(dev, &c, NULL);
}

static int adapter_delete_queue(struct nvme_dev *dev, u8 opcode, u16 id)
{
	int status;
	struct nvme_command c;

	memset(&c, 0, sizeof(c));
	c.delete_queue.opcode = opcode;
	c.delete_queue.qid = cpu_to_le16(id);

	status = nvme_submit_admin_cmd(dev, &c, NULL);
	if (status)
		return -EIO;
	return 0;
}

static int adapter_delete_cq(struct nvme_dev *dev, u16 cqid)
{
	return adapter_delete_queue(dev, nvme_admin_delete_cq, cqid);
}

static int nvme_create_queue(struct nvme_queue *nvmeq, int qid)
{
	struct nvme_dev *dev = nvmeq->dev;
	int result;

	result = adapter_alloc_cq(dev, qid, nvmeq);
	if (result < 0)
		return result;

	result = adapter_alloc_sq(dev, qid, nvmeq);
	if (result < 0)
		goto release_cq;

	nvme_init_queue(nvmeq, qid);

	return result;

 release_cq:
	adapter_delete_cq(dev, qid);
	return result;
}

static int nvme_setup_io_queues(struct nvme_dev *dev)
{
	int result;

	result = set_queue_count(dev, 1);
	if (result < 0) {
		printf("NVMe: Set nvme IO queue count failed\n");
		return result;
	}

	dev->queues[1] = nvme_alloc_queue(dev, 1, 2, 0);
	result = nvme_create_queue(dev->queues[1], 1);

	if (result) {
		printf("NVMe: Creat nvme IO queue failed\n");
		free(dev->queues[1]);
	}
	return result;
}

static int nvme_dev_start(struct nvme_dev *dev)
{
	int result;

	result = nvme_dev_map(dev);
	if (result) {
		printf("NVMe: MAP Resource failed!\n");
		return result;
	}

	result = nvme_configure_admin_queue(dev);
	if (result) {
		printf("NMVe: Configure nvme admin queue failed\n");
		return result;
	}

	result = nvme_setup_io_queues(dev);
	if (result && result != -EBUSY) {
		printf("NVMe: Setup nvme IO queue failed\n");
		goto disable;
	}

	return result;

disable:
	free(dev->queues[0]);
	return result;
}

static int nvme_setup_prp_pools(struct nvme_dev *dev)
{
	dev->prp_small_pool = prp_small_pool;
	dev->prp_page_pool = prp_page_pool;
	return 0;
}

static int nvme_setup_prps(struct nvme_rw_command *cmd,
			   int total_len, uint8 *buf)
{
	int offset, i = 0;
	int length = total_len;
	__le64 *prp_list;

	offset = ((unsigned long)(buf) & ~PAGE_MASK);
	cmd->prp1 = cpu_to_le64(buf);

	length -= (PAGE_SIZE - offset);
	if (length  <= 0)
		return total_len;

	if (length)
		buf += (PAGE_SIZE - offset);

	if (length <= PAGE_SIZE) {
		cmd->prp2 = cpu_to_le64(buf);
		return total_len;
	}

	prp_list = (__le64 *)prp_page_pool;
	cmd->prp2 = (__le64)prp_list;
	for (;;) {
		if (i == (PAGE_SIZE >> 3)) {
			__le64 *old_prp_list = prp_list;

			prp_list = (__le64 *)(prp_page_pool + PAGE_SIZE);
			prp_list[0] = old_prp_list[i - 1];
			old_prp_list[i - 1] = cpu_to_le64 (prp_list);
			i = 1;
		}
		prp_list[i++] = cpu_to_le64(buf);
		buf += PAGE_SIZE;
		length -= PAGE_SIZE;
		if (length <= 0)
			break;
	}
	CacheFlushDRegion((u32)(unsigned long)prp_list, PAGE_SIZE);
	return total_len;
}

u32 nvme_get_capacity(u8 chip)
{
	if (ssd_dev.ncap <= 0)
		return -EINVAL;
	else
		return ssd_dev.nsize;
}

int nvme_flush(void)
{
	struct nvme_queue *nvmeq;
	int err;
	struct nvme_command cmnd;

	/* queue 0: Admin, queue 1: IO */
	nvmeq = ssd_dev.queues[1];
	memset(&cmnd, 0, sizeof(cmnd));
	cmnd.common.opcode = nvme_cmd_flush;
	cmnd.common.nsid = cpu_to_le32(1);

	err = nvme_submit_sync_cmd(nvmeq, &cmnd,
				   NULL, NVME_MAX_TIMEOUT);
	if (err)
		printf("fail to flush nvme\n");

	return err;
}

static int __nvme_write(u8 *buf, u32 lba, u32 nsec)
{
	int length;
	int status;
	struct nvme_command cmd;
	unsigned total_len;
	struct nvme_queue *nvmeq;

	memset(&cmd, 0, sizeof(cmd));
	nvmeq = ssd_dev.queues[1];
	length = nsec << ssd_dev.lba_shift;
	total_len = length;
	cmd.rw.opcode = nvme_cmd_write;
	cmd.rw.flags = 0;	/* no fuse */
	cmd.rw.nsid = 1;
	cmd.rw.slba = cpu_to_le64(lba);
	cmd.rw.length = nsec - 1;
	cmd.rw.control = 0;
	cmd.rw.dsmgmt = 0;
	length = nvme_setup_prps(&cmd.rw, total_len, buf);

	CacheFlushDRegion((u32)(unsigned long)buf,
			  (nsec << ssd_dev.lba_shift));
	if ((length != (nsec) << ssd_dev.lba_shift) ||
	    ((lba + nsec) >= ssd_dev.nsize))
		status = -ENOMEM;
	else
		status = nvme_submit_sync_cmd(nvmeq, &cmd, NULL,
					      NVME_MAX_TIMEOUT);
	return status;
}

u32 nvme_write(u8 chip, u32 lba, void *src, u32 nsec, u32 mode)
{
	struct nvme_queue *nvmeq = ssd_dev.queues[1];
	int err;
	int req_size;
	u8 *buf = (u8 *)src;
	bool is_aligned = true;

	if ((u32)(unsigned long)buf & 0x1ff) {
		printf("nvme_write: buf no aligned to sector\n");
		tbuf = (u8 *)(gd->arch.ddr_end - CONFIG_RK_LCD_SIZE - SZ_16M);
		buf = tbuf;
		is_aligned = false;
		memcpy((u32 *)tbuf, (u32 *)src, nsec * 512);
	}

	req_size = min(MAX_REQ_SECTOR, nvmeq->dev->max_hw_sectors);

	for (;;) {
		if (nsec > req_size) {
			err = __nvme_write(buf, lba, req_size);
			if (err)
				return err;

			nsec -=  req_size;
			buf += 512 *  req_size;
			lba +=  req_size;
			continue;
		}

		err = __nvme_write(buf, lba, nsec);
		break;
	}

	if (!is_aligned)
		memset((u32 *)tbuf, 0, nsec * 512);

	if (err)
		return err;
	else
		return nvme_flush();
}

static int __nvme_read(u8 *buf, u32 lba, u32 nsec)
{
	int length;
	int status;
	struct nvme_command cmd;
	unsigned total_len;
	struct nvme_queue *nvmeq;

	CacheInvalidateDRegion((u32)(unsigned long)buf,
			       (nsec << ssd_dev.lba_shift));
	nvmeq = ssd_dev.queues[1];
	memset(&cmd, 0, sizeof(cmd));
	length = nsec << ssd_dev.lba_shift;
	total_len = length;
	cmd.rw.opcode = nvme_cmd_read;
	cmd.rw.flags = 0;
	cmd.rw.nsid = 1;
	cmd.rw.slba = cpu_to_le64(lba);
	cmd.rw.length = nsec - 1;
	cmd.rw.control = 0;
	cmd.rw.dsmgmt = 0;
	length = nvme_setup_prps(&cmd.rw, total_len, buf);

	if ((length != (nsec) << ssd_dev.lba_shift) ||
	    ((lba + nsec) >= ssd_dev.nsize))
		status = -ENOMEM;
	else
		status = nvme_submit_sync_cmd(nvmeq, &cmd,
					      NULL, NVME_MAX_TIMEOUT);

	CacheInvalidateDRegion((u32)(unsigned long)buf,
			       (nsec << ssd_dev.lba_shift));
	return status;
}

u32 nvme_read(u8 chip, u32 lba, void *dst, u32 nsec)
{
	struct nvme_queue *nvmeq = ssd_dev.queues[1];
	int err;
	int req_size;
	u8 *buf = (u8 *)dst;
	bool is_aligned = true;

	if ((u32)(unsigned long)buf & 0x1ff) {
		printf("nvme_read: buf no aligned to sector\n");
		tbuf = (u8 *)(gd->arch.ddr_end - CONFIG_RK_LCD_SIZE - SZ_16M);
		buf = tbuf;
		is_aligned = false;
	}

	req_size = min(MAX_REQ_SECTOR, nvmeq->dev->max_hw_sectors);

	for (;;) {
		if (nsec > req_size) {
			err = __nvme_read(buf, lba, req_size);
			if (err)
				return err;

			nsec -=  req_size;
			buf += 512 *  req_size;
			lba +=  req_size;
			continue;
		}

		err = __nvme_read(buf, lba, nsec);
		break;
	}
	if (!is_aligned) {
		memcpy((u32 *)dst, (u32 *)tbuf, nsec * 512);
		memset((u32 *)tbuf, 0, nsec * 512);
	}

	return err;
}

static int nvme_identify(struct nvme_dev *dev, unsigned nsid,
			 unsigned cns, uchar *dma_addr)
{
	struct nvme_command c;

	memset(&c, 0, sizeof(c));
	c.identify.opcode = nvme_admin_identify;
	c.identify.nsid = cpu_to_le32(nsid);
	c.identify.prp1 = cpu_to_le64(dma_addr);
	c.identify.cns = cpu_to_le32(cns);

	return nvme_submit_admin_cmd(dev, &c, NULL);
}

static int nvme_dev_add(struct nvme_dev *dev)
{
	int res;
	unsigned nn, i;
	struct nvme_id_ctrl *ctrl;
	struct nvme_id_ns *id_ns;

	unsigned char __aligned(4096) dma_addr[8192];
	int lbaf;
	int shift = NVME_CAP_MPSMIN(__raw_readq(dev->bar)) + 12;

	if (!pbus) {
		printf("%s invalid pbus\n", __func__);
		res = -EINVAL;
		goto out;
	}

	res = nvme_identify(dev, 0, 1, dma_addr);
	if (res) {
		res = -EIO;
		goto out;
	}

	CacheInvalidateDRegion((u32)(unsigned long)dma_addr, 8192);
	ctrl = (struct nvme_id_ctrl *)dma_addr;
	nn = ctrl->nn;

	if (ctrl->mdts)
		dev->max_hw_sectors = 1 << (ctrl->mdts + shift - 9);
	else
		dev->max_hw_sectors = UINT_MAX;

	if ((pbus->pdev.vendor == PCI_VENDOR_ID_INTEL) &&
	    (pbus->pdev.device == 0x0953) && ctrl->vs[3]) {
		unsigned int max_hw_sectors;

		dev->stripe_size = 1 << (ctrl->vs[3] + shift);
		max_hw_sectors = dev->stripe_size >> (shift - 9);
		if (dev->max_hw_sectors)
			dev->max_hw_sectors = min(max_hw_sectors,
						  dev->max_hw_sectors);
		else
			dev->max_hw_sectors = max_hw_sectors;
	}

	id_ns = (struct nvme_id_ns *)dma_addr;
	for (i = 1; i <= nn; i++) {
		res = nvme_identify(dev, i, 0, dma_addr);
		CacheInvalidateDRegion((u32)(unsigned long)dma_addr, 8192);
		if (res)
			continue;

		if (id_ns->ncap == 0)
			continue;

		CacheInvalidateDRegion((u32)(unsigned long)dma_addr, 8192);
		id_ns = (struct nvme_id_ns *)dma_addr;
		lbaf = id_ns->flbas & 0xf;
		dev->lba_shift = id_ns->lbaf[lbaf].ds;
		dev->ncap = id_ns->ncap;
		dev->nsize = id_ns->nsze;
	}
out:
	return res;
}

void nvme_read_flash_info(void *buf)
{
	pFLASH_INFO pInfo = (pFLASH_INFO)buf;

	pInfo->BlockSize = 1024;
	pInfo->ECCBits = 0;
	pInfo->FlashSize = 536870912;
	pInfo->PageSize = 4;
	pInfo->AccessTime = 40;
	pInfo->ManufacturerName = 0;
	pInfo->FlashMask = 0;
	if (pInfo->FlashSize)
		pInfo->FlashMask = 1;
}

u32 nvme_init(u32 base)
{
	int result;
	struct nvme_dev *dev;

	printf("%s\n", __func__);

	dev = &ssd_dev;
	memset(dev, 0, sizeof(struct nvme_dev));

	result = pcie_scan_bus();
	if (result) {
		printf("NVMe: fail to scan bus\n");
		return -EINVAL;
	}

	pbus = pcie_claim_bus();
	if (!pbus) {
		printf("NVMe: fail to claim pcie bus!\n");
		return -ENOMEM;
	}

	dev->queues = malloc(2 * sizeof(void *));
	if (!dev->queues) {
		pcie_release_bus(pbus);
		return -ENOMEM;
	}

	memset(dev->queues, 0x0, 2 * sizeof(void *));

	nvme_setup_prp_pools(dev);

	result = nvme_dev_start(dev);
	if (result) {
		printf("NVMe: Nvme device start failed\n");
		goto free_queues;
	}

	result = nvme_dev_add(dev);
	if (result) {
		printf("NVMe: Add nvme device failed\n");
		goto free_queues;
	}

	return 0;

free_queues:
	pcie_release_bus(pbus);
	free(dev->queues);
	return -ENOMEM;
}
