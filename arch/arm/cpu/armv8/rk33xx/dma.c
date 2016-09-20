/*
 * (C) Copyright 2008-2016 Fuzhou Rockchip Electronics Co., Ltd
 * Peter, Software Engineering, <superpeter.cai@gmail.com>.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#include <common.h>
#include <malloc.h>
#include <ubi_uboot.h>
#include <linux/list.h>
#include <asm/io.h>

#include <asm/arch/rkplat.h>

#define RK_DMA_PL330_VERSION	"1.4"

#define rk_dma_dev_err(...) \
	do { \
		printf(__VA_ARGS__); \
		printf("\n"); \
	} while(0);

#define rk_dma_dev_info(...) \
	do { \
		debug(__VA_ARGS__); \
		debug("\n"); \
	} while(0);


/**
 * struct rk_pl330_dmac - Logical representation of a PL330 DMAC.
 * @busy_chan: Number of channels currently busy.
 * @peri: List of IDs of peripherals this DMAC can work with.
 * @node: To attach to the global list of DMACs.
 * @pi: PL330 configuration info for the DMAC.
 */
struct rk_pl330_dmac {
	unsigned		busy_chan;
	enum dma_ch		*peri;
	struct list_head	node;
	struct pl330_info	*pi;
};

/**
 * struct rk_pl330_xfer - A request submitted by rk DMA clients.
 * @token: Xfer ID provided by the client.
 * @node: To attach to the list of xfers on a channel.
 * @px: Xfer for PL330 core.
 * @chan: Owner channel of this xfer.
 */
struct rk_pl330_xfer {
	void			*token;
	struct list_head	node;
	struct pl330_xfer	px;
	struct rk_pl330_chan	*chan;
};

/**
 * struct rk_pl330_chan - Logical channel to communicate with
 * 	a Physical peripheral.
 * @pl330_chan_id: Token of a hardware channel thread of PL330 DMAC.
 * 	NULL if the channel is available to be acquired.
 * @id: ID of the peripheral that this channel can communicate with.
 * @options: Options specified by the client.
 * @sdaddr: Address provided via rk_dma_devconfig.
 * @node: To attach to the global list of channels.
 * @lrq: Pointer to the last submitted pl330_req to PL330 core.
 * @xfer_list: To manage list of xfers enqueued.
 * @req: Two requests to communicate with the PL330 engine.
 * @callback_fn: Callback function to the client.
 * @rqcfg: Channel configuration for the xfers.
 * @xfer_head: Pointer to the xfer to be next executed.
 * @dmac: Pointer to the DMAC that manages this channel, NULL if the
 * 	channel is available to be acquired.
 * @client: Client of this channel. NULL if the
 * 	channel is available to be acquired.
 */
struct rk_pl330_chan {
	void				*pl330_chan_id;
	enum dma_ch			id;
	unsigned int			options;
	unsigned long			sdaddr;
	struct list_head		node;
	struct pl330_req		*lrq;
	struct list_head		xfer_list;
	struct pl330_req		req[2];
	rk_dma_cbfn_t  			callback_fn;
	struct pl330_reqcfg		rqcfg;
	struct rk_pl330_xfer		*xfer_head;
	struct rk_pl330_dmac		*dmac;
	struct rk_dma_client		*client;
};


/* All DMACs in the platform */
static LIST_HEAD(dmac_list);

/* All channels to peripherals in the platform */
static LIST_HEAD(chan_list);

#if 0
#define spin_lock_init(...)
#define spin_lock_irqsave(...)
#define spin_unlock_irqrestore(...)

/*
 * Since we add resources(DMACs and Channels) to the global pool,
 * we need to guard access to the resources using a global lock
 */
static DEFINE_SPINLOCK(res_lock);
#endif

/* Returns the channel with ID 'id' in the chan_list */
static struct rk_pl330_chan *id_to_chan(const enum dma_ch id)
{
	struct rk_pl330_chan *ch;

	list_for_each_entry(ch, &chan_list, node) {
		if (ch->id == id)
			return ch;
	}

	return NULL;
}

/* Allocate a new channel with ID 'id' and add to chan_list */
static void chan_add(const enum dma_ch id)
{
	struct rk_pl330_chan *ch = id_to_chan(id);

	/* Return if the channel already exists */
	if (ch)
		return;

	ch = malloc(sizeof(*ch));
	/* Return silently to work with other channels */
	if (!ch)
		return;
	ch->id = id;
	ch->dmac = NULL;

	list_add_tail(&ch->node, &chan_list);
}

/* If the channel is not yet acquired by any client */
static bool chan_free(struct rk_pl330_chan *ch)
{
	if (!ch)
		return false;

	/* Channel points to some DMAC only when it's acquired */
	return ch->dmac ? false : true;
}

/*
 * Returns 0 is peripheral i/f is invalid or not present on the dmac.
 * Index + 1, otherwise.
 */
static unsigned iface_of_dmac(struct rk_pl330_dmac *dmac, enum dma_ch ch_id)
{
	enum dma_ch *id = dmac->peri;
	int i;

	/* Discount invalid markers */
	if (ch_id == DMACH_MAX)
		return 0;

	for (i = 0; i < PL330_MAX_PERI; i++)
		if (id[i] == ch_id)
			return i + 1;

	return 0;
}

/* If all channel threads of the DMAC are busy */
static inline bool dmac_busy(struct rk_pl330_dmac *dmac)
{
	struct pl330_info *pi = dmac->pi;

	return (dmac->busy_chan < pi->pcfg.num_chan) ? false : true;
}

/*
 * Returns the number of free channels that
 * can be handled by this dmac only.
 */
static unsigned ch_onlyby_dmac(struct rk_pl330_dmac *dmac)
{
	enum dma_ch *id = dmac->peri;
	struct rk_pl330_dmac *d;
	struct rk_pl330_chan *ch;
	unsigned found, count = 0;
	enum dma_ch p;
	int i;

	for (i = 0; i < PL330_MAX_PERI; i++) {
		p = id[i];
		ch = id_to_chan(p);

		if (p == DMACH_MAX || !chan_free(ch))
			continue;

		found = 0;
		list_for_each_entry(d, &dmac_list, node) {
			if (d != dmac && iface_of_dmac(d, ch->id)) {
				found = 1;
				break;
			}
		}
		if (!found)
			count++;
	}

	return count;
}

/*
 * Measure of suitability of 'dmac' handling 'ch'
 *
 * 0 indicates 'dmac' can not handle 'ch' either
 * because it is not supported by the hardware or
 * because all dmac channels are currently busy.
 *
 * >0 vlaue indicates 'dmac' has the capability.
 * The bigger the value the more suitable the dmac.
 */
#define MAX_SUIT	(~0U)
#define MIN_SUIT	0

static unsigned suitablility(struct rk_pl330_dmac *dmac,
		struct rk_pl330_chan *ch)
{
	struct pl330_info *pi = dmac->pi;
	enum dma_ch *id = dmac->peri;
	struct rk_pl330_dmac *d;
	unsigned s;
	int i;

	s = MIN_SUIT;
	/* If all the DMAC channel threads are busy */
	if (dmac_busy(dmac))
		return s;

	for (i = 0; i < PL330_MAX_PERI; i++)
		if (id[i] == ch->id)
			break;

	/* If the 'dmac' can't talk to 'ch' */
	if (i == PL330_MAX_PERI)
		return s;

	s = MAX_SUIT;
	list_for_each_entry(d, &dmac_list, node) {
		/*
		 * If some other dmac can talk to this
		 * peri and has some channel free.
		 */
		if (d != dmac && iface_of_dmac(d, ch->id) && !dmac_busy(d)) {
			s = 0;
			break;
		}
	}
	if (s)
		return s;

	s = 100;

	/* Good if free chans are more, bad otherwise */
	s += (pi->pcfg.num_chan - dmac->busy_chan) - ch_onlyby_dmac(dmac);

	return s;
}

/* More than one DMAC may have capability to transfer data with the
 * peripheral. This function assigns most suitable DMAC to manage the
 * channel and hence communicate with the peripheral.
 */
static struct rk_pl330_dmac *map_chan_to_dmac(struct rk_pl330_chan *ch)
{
	struct rk_pl330_dmac *d, *dmac = NULL;
	unsigned sn, sl = MIN_SUIT;

	list_for_each_entry(d, &dmac_list, node) {
		sn = suitablility(d, ch);

		if (sn == MAX_SUIT)
			return d;

		if (sn > sl)
			dmac = d;
	}

	return dmac;
}

/* Acquire the channel for peripheral 'id' */
static struct rk_pl330_chan *chan_acquire(const enum dma_ch id)
{
	struct rk_pl330_chan *ch = id_to_chan(id);
	struct rk_pl330_dmac *dmac;

	/* If the channel doesn't exist or is already acquired */
	if (!ch || !chan_free(ch)) {
		ch = NULL;
		goto acq_exit;
	}

	dmac = map_chan_to_dmac(ch);
	/* If couldn't map */
	if (!dmac) {
		ch = NULL;
		goto acq_exit;
	}

	dmac->busy_chan++;
	ch->dmac = dmac;

acq_exit:
	return ch;
}

/* Delete xfer from the queue */
static inline void del_from_queue(struct rk_pl330_xfer *xfer)
{
	struct rk_pl330_xfer *t;
	struct rk_pl330_chan *ch;
	int found;

	if (!xfer)
		return;

	ch = xfer->chan;

	/* Make sure xfer is in the queue */
	found = 0;
	list_for_each_entry(t, &ch->xfer_list, node)
		if (t == xfer) {
			found = 1;
			break;
		}

	if (!found)
		return;

	/* If xfer is last entry in the queue */
	if (xfer->node.next == &ch->xfer_list)
		t = list_entry(ch->xfer_list.next,
				struct rk_pl330_xfer, node);
	else
		t = list_entry(xfer->node.next,
				struct rk_pl330_xfer, node);

	/* If there was only one node left */
	if (t == xfer)
		ch->xfer_head = NULL;
	else if (ch->xfer_head == xfer)
		ch->xfer_head = t;

	list_del(&xfer->node);
}

/* Provides pointer to the next xfer in the queue.
 * If CIRCULAR option is set, the list is left intact,
 * otherwise the xfer is removed from the list.
 * Forced delete 'pluck' can be set to override the CIRCULAR option.
 */
static struct rk_pl330_xfer *get_from_queue(struct rk_pl330_chan *ch,
		int pluck)
{
	struct rk_pl330_xfer *xfer = ch->xfer_head;

	if (!xfer)
		return NULL;

	/* If xfer is last entry in the queue */
	if (xfer->node.next == &ch->xfer_list)
		ch->xfer_head = list_entry(ch->xfer_list.next,
					struct rk_pl330_xfer, node);
	else
		ch->xfer_head = list_entry(xfer->node.next,
					struct rk_pl330_xfer, node);

	if (pluck || !(ch->options & RK_DMAF_CIRCULAR))
		del_from_queue(xfer);

	return xfer;
}

static inline void add_to_queue(struct rk_pl330_chan *ch,
		struct rk_pl330_xfer *xfer, int front)
{
	struct pl330_xfer *xt;

	/* If queue empty */
	if (ch->xfer_head == NULL)
		ch->xfer_head = xfer;

	xt = &ch->xfer_head->px;
	/* If the head already submitted (CIRCULAR head) */
	if (ch->options & RK_DMAF_CIRCULAR &&
		(xt == ch->req[0].x || xt == ch->req[1].x))
		ch->xfer_head = xfer;

	/* If this is a resubmission, it should go at the head */
	if (front) {
		ch->xfer_head = xfer;
		list_add(&xfer->node, &ch->xfer_list);
	} else
		list_add_tail(&xfer->node, &ch->xfer_list);
}

static inline void _finish_off(struct rk_pl330_xfer *xfer,
		enum rk_dma_buffresult res, int ffree)
{
	struct rk_pl330_chan *ch;

	if (!xfer)
		return;

	ch = xfer->chan;

	/* Do callback */

	if (ch->callback_fn)
		ch->callback_fn(xfer->token, xfer->px.bytes, res);

	/* Force Free or if buffer is not needed anymore */
	if (ffree || !(ch->options & RK_DMAF_CIRCULAR))
		free(xfer);
}

static inline int rk_pl330_submit(struct rk_pl330_chan *ch,
		struct pl330_req *r)
{
	struct rk_pl330_xfer *xfer;
	int ret = 0;

	/* If already submitted */
	if (r->x)
		return 0;

	xfer = get_from_queue(ch, 0);

	if (xfer) {
		r->x = &xfer->px;

		/* Use max bandwidth for M<->M xfers */
		if (r->rqtype == MEMTOMEM) {
			struct pl330_info *pi = xfer->chan->dmac->pi;
			int burst = 1 << ch->rqcfg.brst_size;
			u32 bytes = r->x->bytes;
			int bl;

			bl = pi->pcfg.data_bus_width / 8;
			bl *= pi->pcfg.data_buf_dep;
			bl /= burst;

			/* src/dst_burst_len can't be more than 16 */
			if (bl > 16)
				bl = 16;

			while (bl > 1) {
				if (!(bytes % (bl * burst)))
					break;
				bl--;
			}

			ch->rqcfg.brst_len = bl;
		} else {
#ifdef CONFIG_RK_MMC_EDMAC
			/* mmc using pll330 dma */
			if (ch->id == DMACH_EMMC)
				ch->rqcfg.brst_len = 16;
#endif
		}

		ret = pl330_submit_req(ch->pl330_chan_id, r);

		/* If submission was successful */
		if (!ret) {
			ch->lrq = r; /* latest submitted req */
			return 0;
		}

		r->x = NULL;

		/* If both of the PL330 ping-pong buffers filled */
		if (ret == -EAGAIN) {
			rk_dma_dev_err("%s:%d!\n", __func__, __LINE__);
			/* Queue back again */
			add_to_queue(ch, xfer, 1);
			ret = 0;
		} else {
			rk_dma_dev_err("%s:%d!\n", __func__, __LINE__);
			_finish_off(xfer, RK_RES_ERR, 0);
		}
	}

	return ret;
}

static void rk_pl330_rq(struct rk_pl330_chan *ch,
	struct pl330_req *r, enum pl330_op_err err)
{
	unsigned long flags = 0;
	struct rk_pl330_xfer *xfer;
	struct pl330_xfer *xl;
	enum rk_dma_buffresult res;

	spin_lock_irqsave(&res_lock, flags);
	xl = r->x;
	if (!r->infiniteloop) {		
		r->x = NULL;

		rk_pl330_submit(ch, r);
	}

	spin_unlock_irqrestore(&res_lock, flags);

	/* Map result to rk DMA API */
	if (err == PL330_ERR_NONE)
		res = RK_RES_OK;
	else if (err == PL330_ERR_ABORT)
		res = RK_RES_ABORT;
	else
		res = RK_RES_ERR;

	/* If last request had some xfer */
	if (!r->infiniteloop) {
		if (xl) {
			xfer = container_of(xl, struct rk_pl330_xfer, px);
			_finish_off(xfer, res, 0);
		} else {
			rk_dma_dev_info("%s:%d No Xfer?!\n", __func__, __LINE__);
		}
	} else {
		/* Do callback */

		xfer = container_of(xl, struct rk_pl330_xfer, px);
		if (ch->callback_fn)
			ch->callback_fn(xfer->token, xfer->px.bytes, res);
	}
}

static void rk_pl330_rq0(void *token, enum pl330_op_err err)
{
	struct pl330_req *r = token;
	struct rk_pl330_chan *ch = container_of(r,
					struct rk_pl330_chan, req[0]);
	rk_pl330_rq(ch, r, err);
}

static void rk_pl330_rq1(void *token, enum pl330_op_err err)
{
	struct pl330_req *r = token;
	struct rk_pl330_chan *ch = container_of(r,
					struct rk_pl330_chan, req[1]);
	rk_pl330_rq(ch, r, err);
}

/* Release an acquired channel */
static void chan_release(struct rk_pl330_chan *ch)
{
	struct rk_pl330_dmac *dmac;

	if (chan_free(ch))
		return;

	dmac = ch->dmac;
	ch->dmac = NULL;
	dmac->busy_chan--;
}

int rk_dma_ctrl(enum dma_ch id, enum rk_chan_op op)
{
	struct rk_pl330_xfer *xfer;
	enum pl330_chan_op pl330op;
	struct rk_pl330_chan *ch;
	unsigned long flags = 0;
	int idx, ret;

	spin_lock_irqsave(&res_lock, flags);

	ch = id_to_chan(id);

	if (!ch || chan_free(ch)) {
		ret = -EINVAL;
		goto ctrl_exit;
	}

	switch (op) {
	case RK_DMAOP_START:
		/* Make sure both reqs are enqueued */
		idx = (ch->lrq == &ch->req[0]) ? 1 : 0;
		rk_pl330_submit(ch, &ch->req[idx]);
		rk_pl330_submit(ch, &ch->req[1 - idx]);
		pl330op = PL330_OP_START;
		break;

	case RK_DMAOP_STOP:
		pl330op = PL330_OP_ABORT;
		break;

	case RK_DMAOP_FLUSH:
		pl330op = PL330_OP_FLUSH;
		break;

	case RK_DMAOP_PAUSE:
	case RK_DMAOP_RESUME:
	case RK_DMAOP_TIMEOUT:
	case RK_DMAOP_STARTED:
		spin_unlock_irqrestore(&res_lock, flags);
		return 0;

	default:
		spin_unlock_irqrestore(&res_lock, flags);
		return -EINVAL;
	}

	ret = pl330_chan_ctrl(ch->pl330_chan_id, pl330op);

	if (pl330op == PL330_OP_START) {
		spin_unlock_irqrestore(&res_lock, flags);
		return ret;
	}

	idx = (ch->lrq == &ch->req[0]) ? 1 : 0;

	/* Abort the current xfer */
	if (ch->req[idx].x) {
		xfer = container_of(ch->req[idx].x,
				struct rk_pl330_xfer, px);

		/* Drop xfer during FLUSH */
		if (pl330op == PL330_OP_FLUSH)
			del_from_queue(xfer);

		ch->req[idx].x = NULL;

		spin_unlock_irqrestore(&res_lock, flags);
		_finish_off(xfer, RK_RES_ABORT,
				pl330op == PL330_OP_FLUSH ? 1 : 0);
		spin_lock_irqsave(&res_lock, flags);
	}

	/* Flush the whole queue */
	if (pl330op == PL330_OP_FLUSH) {
		if (ch->req[1 - idx].x) {
			xfer = container_of(ch->req[1 - idx].x,
					struct rk_pl330_xfer, px);

			del_from_queue(xfer);

			ch->req[1 - idx].x = NULL;

			spin_unlock_irqrestore(&res_lock, flags);
			_finish_off(xfer, RK_RES_ABORT, 1);
			spin_lock_irqsave(&res_lock, flags);
		}

		/* Finish off the remaining in the queue */
		xfer = ch->xfer_head;
		while (xfer) {
			del_from_queue(xfer);

			spin_unlock_irqrestore(&res_lock, flags);
			_finish_off(xfer, RK_RES_ABORT, 1);
			spin_lock_irqsave(&res_lock, flags);

			xfer = ch->xfer_head;
		}
	}

ctrl_exit:
	spin_unlock_irqrestore(&res_lock, flags);

	return ret;
}


//hhb@rock-chips.com 2012-06-14
int rk_dma_enqueue_ring(enum dma_ch id, void *token,
			dma_addr_t addr, int size, int numofblock, bool sev)
{
	struct rk_pl330_chan *ch;
	struct rk_pl330_xfer *xfer;
	unsigned long flags = 0;
	int idx, ret = 0;
	
	spin_lock_irqsave(&res_lock, flags);

	ch = id_to_chan(id);

	/* Error if invalid or free channel */
	if (!ch || chan_free(ch)) {
		ret = -EINVAL;
		goto enq_exit;
	}

	/* Error if size is unaligned */
	if (ch->rqcfg.brst_size && size % (1 << ch->rqcfg.brst_size)) {
		ret = -EINVAL;
		goto enq_exit;
	}

	xfer = malloc(sizeof(struct rk_pl330_xfer));
	if (!xfer) {
		ret = -ENOMEM;
		goto enq_exit;
	}

	xfer->token = token;
	xfer->chan = ch;
	xfer->px.bytes = size;
	xfer->px.next = NULL; /* Single request */

	/* For rk DMA API, direction is always fixed for all xfers */
	if (ch->req[0].rqtype == MEMTODEV) {
		xfer->px.src_addr = addr;
		xfer->px.dst_addr = ch->sdaddr;
	} else {
		xfer->px.src_addr = ch->sdaddr;
		xfer->px.dst_addr = addr;
	}

	add_to_queue(ch, xfer, 0);

	/* Try submitting on either request */
	idx = (ch->lrq == &ch->req[0]) ? 1 : 0;

	if (!ch->req[idx].x) {
		ch->req[idx].infiniteloop = numofblock;
		if (numofblock)
			ch->req[idx].infiniteloop_sev = sev;
		rk_pl330_submit(ch, &ch->req[idx]);
	} else {
		ch->req[1 - idx].infiniteloop = numofblock;
		if (numofblock)
			ch->req[1 - idx].infiniteloop_sev = sev;
		rk_pl330_submit(ch, &ch->req[1 - idx]);
	}
	spin_unlock_irqrestore(&res_lock, flags);

	if (ch->options & RK_DMAF_AUTOSTART)
		rk_dma_ctrl(id, RK_DMAOP_START);

	return 0;

enq_exit:
	spin_unlock_irqrestore(&res_lock, flags);

	return ret;
}


int rk_dma_enqueue(enum dma_ch id, void *token,
			dma_addr_t addr, int size)
{
	return rk_dma_enqueue_ring(id, token, addr, size, 0, false);
}


int rk_dma_request(enum dma_ch id,
			struct rk_dma_client *client,
			void *dev)
{
	struct rk_pl330_dmac *dmac;
	struct rk_pl330_chan *ch;
	unsigned long flags = 0;
	int ret = 0;

	spin_lock_irqsave(&res_lock, flags);

	ch = chan_acquire(id);
	if (!ch) {
		ret = -EBUSY;
		goto req_exit;
	}

	dmac = ch->dmac;

	ch->pl330_chan_id = pl330_request_channel(dmac->pi);
	if (!ch->pl330_chan_id) {
		chan_release(ch);
		ret = -EBUSY;
		goto req_exit;
	}

	ch->client = client;
	ch->options = 0; /* Clear any option */
	ch->callback_fn = NULL; /* Clear any callback */
	ch->lrq = NULL;

	ch->rqcfg.brst_size = 2; /* Default word size */
	ch->rqcfg.swap = SWAP_NO;
	ch->rqcfg.scctl = SCCTRL0; /* Noncacheable and nonbufferable */
	ch->rqcfg.dcctl = DCCTRL0; /* Noncacheable and nonbufferable */
	ch->rqcfg.privileged = 0;
	ch->rqcfg.insnaccess = 0;
	ch->rqcfg.pcfg = &dmac->pi->pcfg;

	/* Set invalid direction */
	ch->req[0].rqtype = DEVTODEV;
	ch->req[1].rqtype = ch->req[0].rqtype;

	ch->req[0].cfg = &ch->rqcfg;
	ch->req[1].cfg = ch->req[0].cfg;

	ch->req[0].peri = iface_of_dmac(dmac, id) - 1; /* Original index */
	ch->req[1].peri = ch->req[0].peri;

	ch->req[0].token = &ch->req[0];
	ch->req[0].xfer_cb = rk_pl330_rq0;
	ch->req[1].token = &ch->req[1];
	ch->req[1].xfer_cb = rk_pl330_rq1;

	ch->req[0].x = NULL;
	ch->req[1].x = NULL;

	/* Reset xfer list */
	INIT_LIST_HEAD(&ch->xfer_list);
	ch->xfer_head = NULL;

req_exit:
	spin_unlock_irqrestore(&res_lock, flags);

	return ret;
}


int rk_dma_free(enum dma_ch id, struct rk_dma_client *client)
{
	struct rk_pl330_chan *ch;
	struct rk_pl330_xfer *xfer;
	unsigned long flags = 0;
	int ret = 0;
	unsigned idx;

	spin_lock_irqsave(&res_lock, flags);

	ch = id_to_chan(id);

	if (!ch || chan_free(ch))
		goto free_exit;

	/* Refuse if someone else wanted to free the channel */
	if (ch->client != client) {
		ret = -EBUSY;
		goto free_exit;
	}

	/* Stop any active xfer, Flushe the queue and do callbacks */
	pl330_chan_ctrl(ch->pl330_chan_id, PL330_OP_FLUSH);

	/* Abort the submitted requests */
	idx = (ch->lrq == &ch->req[0]) ? 1 : 0;

	if (ch->req[idx].x) {
		xfer = container_of(ch->req[idx].x,
				struct rk_pl330_xfer, px);

		ch->req[idx].x = NULL;
		del_from_queue(xfer);

		spin_unlock_irqrestore(&res_lock, flags);
		_finish_off(xfer, RK_RES_ABORT, 1);
		spin_lock_irqsave(&res_lock, flags);
	}

	if (ch->req[1 - idx].x) {
		xfer = container_of(ch->req[1 - idx].x,
				struct rk_pl330_xfer, px);

		ch->req[1 - idx].x = NULL;
		del_from_queue(xfer);

		spin_unlock_irqrestore(&res_lock, flags);
		_finish_off(xfer, RK_RES_ABORT, 1);
		spin_lock_irqsave(&res_lock, flags);
	}

	/* Pluck and Abort the queued requests in order */
	do {
		xfer = get_from_queue(ch, 1);

		spin_unlock_irqrestore(&res_lock, flags);
		_finish_off(xfer, RK_RES_ABORT, 1);
		spin_lock_irqsave(&res_lock, flags);
	} while (xfer);

	ch->client = NULL;

	pl330_release_channel(ch->pl330_chan_id);

	ch->pl330_chan_id = NULL;

	chan_release(ch);

free_exit:
	spin_unlock_irqrestore(&res_lock, flags);

	return ret;
}


/**
*   config the burst length when dma init or brst_len change
*   every peripher has to determine burst width and length by its FIFO
*
*   param:
*           id: dma request id
*           xferunit: burst width in byte
*           brst_len: burst length every transfer
*/
int rk_dma_config(enum dma_ch id, int xferunit, int brst_len)
{
	struct rk_pl330_chan *ch;
	unsigned long flags = 0;
	int i, ret = 0;

	spin_lock_irqsave(&res_lock, flags);

	ch = id_to_chan(id);

	if (!ch || chan_free(ch)) {
		ret = -EINVAL;
		goto cfg_exit;
	}

	i = 0;
	while (xferunit != (1 << i))
		i++;

	if (xferunit > 8)
		goto cfg_exit;
	else
		ch->rqcfg.brst_size = i;

	if (brst_len > 16)
		goto cfg_exit;
	else
		ch->rqcfg.brst_len = brst_len;

cfg_exit:
	spin_unlock_irqrestore(&res_lock, flags);

	return ret;
}


/* Options that are supported by this driver */
#define RK_PL330_FLAGS (RK_DMAF_CIRCULAR | RK_DMAF_AUTOSTART)

int rk_dma_setflags(enum dma_ch id, unsigned int options)
{
	struct rk_pl330_chan *ch;
	unsigned long flags = 0;
	int ret = 0;

	spin_lock_irqsave(&res_lock, flags);

	ch = id_to_chan(id);

	if (!ch || chan_free(ch) || options & ~(RK_PL330_FLAGS))
		ret = -EINVAL;
	else
		ch->options = options;

	spin_unlock_irqrestore(&res_lock, flags);

	return ret;
}


int rk_dma_set_buffdone_fn(enum dma_ch id, rk_dma_cbfn_t rtn)
{
	struct rk_pl330_chan *ch;
	unsigned long flags = 0;
	int ret = 0;

	spin_lock_irqsave(&res_lock, flags);

	ch = id_to_chan(id);

	if (!ch || chan_free(ch))
		ret = -EINVAL;
	else
		ch->callback_fn = rtn;

	spin_unlock_irqrestore(&res_lock, flags);

	return ret;
}


int rk_dma_devconfig(enum dma_ch id, enum rk_dmasrc source,
			  unsigned long address)
{
	struct rk_pl330_chan *ch;
	unsigned long flags = 0;
	int ret = 0;

	spin_lock_irqsave(&res_lock, flags);

	ch = id_to_chan(id);

	if (!ch || chan_free(ch)) {
		ret = -EINVAL;
		goto devcfg_exit;
	}

	switch (source) {
	case RK_DMASRC_HW: /* P->M */
		ch->req[0].rqtype = DEVTOMEM;
		ch->req[1].rqtype = DEVTOMEM;
		ch->rqcfg.src_inc = 0;
		ch->rqcfg.dst_inc = 1;
		break;
	case RK_DMASRC_MEM: /* M->P */
		ch->req[0].rqtype = MEMTODEV;
		ch->req[1].rqtype = MEMTODEV;
		ch->rqcfg.src_inc = 1;
		ch->rqcfg.dst_inc = 0;
		break;
        case RK_DMASRC_MEMTOMEM:
                ch->req[0].rqtype = MEMTOMEM;
		ch->req[1].rqtype = MEMTOMEM;
		ch->rqcfg.src_inc = 1;
		ch->rqcfg.dst_inc = 1;
                break;
	default:
		ret = -EINVAL;
		goto devcfg_exit;
	}

	ch->sdaddr = address;

devcfg_exit:
	spin_unlock_irqrestore(&res_lock, flags);

	return ret;
}


int rk_dma_getposition(enum dma_ch id, dma_addr_t *src, dma_addr_t *dst)
{
	struct rk_pl330_chan *ch = id_to_chan(id);
	struct pl330_chanstatus status;
	int ret;

	if (!ch || chan_free(ch))
		return -EINVAL;

	ret = pl330_chan_status(ch->pl330_chan_id, &status);
	if (ret < 0)
		return ret;

	*src = status.src_addr;
	*dst = status.dst_addr;

	return 0;
}


static inline void *rk_pl330_dmac_get_base(int dmac_id)
{
#ifdef CONFIG_RK_DMAC_0
	if (dmac_id == 0)
		return (void *)RK_DMAC0_BASE;
#endif

#ifdef CONFIG_RK_DMAC_1
	if (dmac_id == 1)
		return (void *)RK_DMAC1_BASE;
#endif

	return NULL;
}


/* dmac pl330 info */
#ifdef CONFIG_RK_DMAC_0
static struct pl330_info	*g_pl330_info_0 = NULL;
#endif
#ifdef CONFIG_RK_DMAC_1
static struct pl330_info	*g_pl330_info_1 = NULL;
#endif

static inline struct pl330_info *rk_pl330_dmac_get_info(int dmac_id)
{
#ifdef CONFIG_RK_DMAC_0
	if (dmac_id == 0)
		return g_pl330_info_0;
#endif

#ifdef CONFIG_RK_DMAC_1
	if (dmac_id == 1)
		return g_pl330_info_1;
#endif

	return NULL;
}


#ifdef CONFIG_RK_DMAC_0

#if defined(CONFIG_RKCHIP_RK3368) || defined(CONFIG_RKCHIP_RK3366)
static struct rk_pl330_platdata g_dmac0_pdata = {
	.peri = {
		[0] = DMACH_I2S_8CH_TX,
		[1] = DMACH_I2S_8CH_RX,
		[2] = DMACH_PWM,
		[3] = DMACH_SPDIF_8CH_TX,
		[4] = DMACH_UART2_DBG_TX,
		[5] = DMACH_UART2_DBG_RX,
		[6] = DMACH_I2S_2CH_TX,
		[7] = DMACH_I2S_2CH_RX,
		[8] = DMACH_MAX,
		[9] = DMACH_MAX,
		[10] = DMACH_DMAC0_MEMTOMEM,
		[11] = DMACH_MAX,
		[12] = DMACH_MAX,
		[13] = DMACH_MAX,
		[14] = DMACH_MAX,
		[15] = DMACH_MAX,
		[16] = DMACH_MAX,
		[17] = DMACH_MAX,
		[18] = DMACH_MAX,
		[19] = DMACH_MAX,
		[20] = DMACH_MAX,
		[21] = DMACH_MAX,
		[22] = DMACH_MAX,
		[23] = DMACH_MAX,
		[24] = DMACH_MAX,
		[25] = DMACH_MAX,
		[26] = DMACH_MAX,
		[27] = DMACH_MAX,
		[28] = DMACH_MAX,
		[29] = DMACH_MAX,
		[30] = DMACH_MAX,
		[31] = DMACH_MAX,
	},
};

#elif defined(CONFIG_RKCHIP_RK322XH)
static struct rk_pl330_platdata g_dmac0_pdata = {
	.peri = {
		[0] = DMACH_I2S2_2CH_TX,
		[1] = DMACH_I2S2_2CH_RX,
		[2] = DMACH_UART0_TX,
		[3] = DMACH_UART0_RX,
		[4] = DMACH_UART1_TX,
		[5] = DMACH_UART1_RX,
		[6] = DMACH_UART2_TX,
		[7] = DMACH_UART2_RX,
		[8] = DMACH_SPI_TX,
		[9] = DMACH_SPI_RX,
		[10] = DMACH_SPDIF_TX,
		[11] = DMACH_I2S0_8CH_TX,
		[12] = DMACH_I2S0_8CH_RX,
		[13] = DMACH_PWM_TX,
		[14] = DMACH_I2S1_8CH_TX,
		[15] = DMACH_I2S1_8CH_TX,
		[16] = DMACH_PDM_TX,
		[17] = DMACH_MAX,
		[18] = DMACH_MAX,
		[19] = DMACH_DMAC0_MEMTOMEM,
		[20] = DMACH_MAX,
		[21] = DMACH_MAX,
		[22] = DMACH_MAX,
		[23] = DMACH_MAX,
		[24] = DMACH_MAX,
		[25] = DMACH_MAX,
		[26] = DMACH_MAX,
		[27] = DMACH_MAX,
		[28] = DMACH_MAX,
		[29] = DMACH_MAX,
		[30] = DMACH_MAX,
		[31] = DMACH_MAX,
	},
};
#else
	#error "Please config rk chip for dmac0."
#endif

#endif /* CONFIG_RK_DMAC_0 */


#ifdef CONFIG_RK_DMAC_1

#if defined(CONFIG_RKCHIP_RK3368)
static struct rk_pl330_platdata g_dmac1_pdata = {
	.peri = {
		[0] = DMACH_HSADC,
		[1] = DMACH_UART0_BT_TX,
		[2] = DMACH_UART0_BT_RX,
		[3] = DMACH_UART1_BB_TX,
		[4] = DMACH_UART1_BB_RX,
		[5] = DMACH_MAX,
		[6] = DMACH_MAX,
		[7] = DMACH_UART3_GPS_TX,
		[8] = DMACH_UART3_GPS_RX,
		[9] = DMACH_UART4_EXP_TX,
		[10] = DMACH_UART4_EXP_RX,
		[11] = DMACH_SPI0_TX,
		[12] = DMACH_SPI0_RX,
		[13] = DMACH_SPI1_TX,
		[14] = DMACH_SPI1_RX,
		[15] = DMACH_SPI2_TX,
		[16] = DMACH_SPI2_RX,
		[17] = DMACH_MAX,
		[18] = DMACH_MAX,
		[19] = DMACH_MAX,
		[20] = DMACH_MAX,
		[21] = DMACH_MAX,
		[22] = DMACH_MAX,
		[23] = DMACH_MAX,
		[24] = DMACH_MAX,
		[25] = DMACH_MAX,
		[26] = DMACH_MAX,
		[27] = DMACH_MAX,
		[28] = DMACH_MAX,
		[29] = DMACH_MAX,
		[30] = DMACH_MAX,
		[31] = DMACH_MAX,
	},
};

#elif defined(CONFIG_RKCHIP_RK3366)
static struct rk_pl330_platdata g_dmac1_pdata = {
	.peri = {
		[0] = DMACH_MAX,
		[1] = DMACH_UART0_BT_TX,
		[2] = DMACH_UART0_BT_RX,
		[3] = DMACH_MAX,
		[4] = DMACH_MAX,
		[5] = DMACH_MAX,
		[6] = DMACH_MAX,
		[7] = DMACH_UART3_GPS_TX,
		[8] = DMACH_UART3_GPS_RX,
		[9] = DMACH_MAX,
		[10] = DMACH_MAX,
		[11] = DMACH_SPI0_TX,
		[12] = DMACH_SPI0_RX,
		[13] = DMACH_SPI1_TX,
		[14] = DMACH_SPI1_RX,
		[15] = DMACH_MAX,
		[16] = DMACH_MAX,
		[17] = DMACH_MAX,
		[18] = DMACH_MAX,
		[19] = DMACH_MAX,
		[20] = DMACH_MAX,
		[21] = DMACH_MAX,
		[22] = DMACH_MAX,
		[23] = DMACH_MAX,
		[24] = DMACH_MAX,
		[25] = DMACH_MAX,
		[26] = DMACH_MAX,
		[27] = DMACH_MAX,
		[28] = DMACH_MAX,
		[29] = DMACH_MAX,
		[30] = DMACH_MAX,
		[31] = DMACH_MAX,
	},
};

#else
	#error "Please config rk chip for dmac1."
#endif

#endif /* CONFIG_RK_DMAC_1 */


static inline struct rk_pl330_platdata *rk_pl330_dmac_get_pd(int dmac_id)
{
#ifdef CONFIG_RK_DMAC_0
	if (dmac_id == 0)
		return &g_dmac0_pdata;
#endif

#ifdef CONFIG_RK_DMAC_1
	if (dmac_id == 1)
		return &g_dmac1_pdata;
#endif

	return NULL;
}


static void rk_pl330_dmac_isr(void *data)
{
	uint32 isr_src = (uint32)(unsigned long) data;

	switch (isr_src) {
#ifdef CONFIG_RK_DMAC_0
	case RK_DMAC0_IRQ0:
	case RK_DMAC0_IRQ1:
		pl330_update(g_pl330_info_0);
		break;
#endif /* CONFIG_RK_DMAC_0 */

#ifdef CONFIG_RK_DMAC_1
	case RK_DMAC1_IRQ0:
	case RK_DMAC1_IRQ1:
		pl330_update(g_pl330_info_1);
		break;
#endif /* CONFIG_RK_DMAC_1 */
	}
}


int rk_pl330_dmac_init(int dmac_id)
{
	struct rk_pl330_dmac *rk_pl330_dmac = NULL;
	struct rk_pl330_platdata *pl330pd = NULL;
	struct pl330_info *pl330_info = NULL;
	int i, ret = 0;

	printf("rk dma pl330 version: %s\n", RK_DMA_PL330_VERSION);

	if (dmac_id >= RK_PL330_DMAC_MAX) {
		rk_dma_dev_err("dmac id error!\n");
		return -EIO;
	}

	pl330pd = rk_pl330_dmac_get_pd(dmac_id);

	/* Can't do without the list of _32_ peripherals */
	if (!pl330pd || !pl330pd->peri) {
		rk_dma_dev_err("platform data missing!\n");
		return -ENODEV;
	}

	pl330_info = malloc(sizeof(struct pl330_info));
	if (!pl330_info) {
		rk_dma_dev_err("malloc error!\n");
		return -ENOMEM;
	}
	memset(pl330_info, 0, sizeof(struct pl330_info));

	pl330_info->pl330_data = NULL;
	pl330_info->base = rk_pl330_dmac_get_base(dmac_id);
	if (pl330_info->base == NULL) {
		ret = -EIO;
		rk_dma_dev_err("pl330 base error!\n");
		goto probe_err1;
	}

	ret = pl330_add(pl330_info);
	if (ret) {
		rk_dma_dev_err("pl330 info add error!\n");
		goto probe_err1;
	}

	/* Allocate a new DMAC */
	rk_pl330_dmac = malloc(sizeof(struct rk_pl330_dmac));
	if (!rk_pl330_dmac) {
		ret = -ENOMEM;
		goto probe_err2;
	}

	/* Hook the info */
	rk_pl330_dmac->pi = pl330_info;

	/* No busy channels */
	rk_pl330_dmac->busy_chan = 0;

	/* Get the list of peripherals */
	rk_pl330_dmac->peri = pl330pd->peri;

	/* Attach to the list of DMACs */
	list_add_tail(&rk_pl330_dmac->node, &dmac_list);

	/* Create a channel for each peripheral in the DMAC
	 * that is, if it doesn't already exist
	 */
	for (i = 0; i < PL330_MAX_PERI; i++)
		if (rk_pl330_dmac->peri[i] != DMACH_MAX)
			chan_add(rk_pl330_dmac->peri[i]);

	/* global info */
	if (dmac_id == 0) {
#ifdef CONFIG_RK_DMAC_0
		g_pl330_info_0 = pl330_info;

		irq_install_handler(RK_DMAC0_IRQ0, rk_pl330_dmac_isr, NULL);
		irq_install_handler(RK_DMAC0_IRQ1, rk_pl330_dmac_isr, NULL);

		irq_handler_enable(RK_DMAC0_IRQ0);
		irq_handler_enable(RK_DMAC0_IRQ1);
#endif
	} else {
#ifdef CONFIG_RK_DMAC_1
		g_pl330_info_1 = pl330_info;

		irq_install_handler(RK_DMAC1_IRQ0, rk_pl330_dmac_isr, NULL);
		irq_install_handler(RK_DMAC1_IRQ1, rk_pl330_dmac_isr, NULL);

		irq_handler_enable(RK_DMAC1_IRQ0);
		irq_handler_enable(RK_DMAC1_IRQ1);
#endif
	}

	debug("Loaded driver for PL330 DMAC-%d\n", dmac_id);
	debug("\tDBUFF-%ux%ubytes Num_Chans-%u Num_Peri-%u Num_Events-%u\n",
		pl330_info->pcfg.data_buf_dep,
		pl330_info->pcfg.data_bus_width / 8, pl330_info->pcfg.num_chan,
		pl330_info->pcfg.num_peri, pl330_info->pcfg.num_events);

	return 0;

probe_err2:
	pl330_del(pl330_info);
probe_err1:
	free(pl330_info);

	return ret;
}


int rk_pl330_dmac_deinit(int dmac_id)
{
	struct rk_pl330_dmac *dmac, *d;
	struct rk_pl330_chan *ch, *ch_bak;
	struct pl330_info *pl330_info = NULL;
	unsigned long flags = 0;
	int del, found;

	debug("%s, dmac_id = %d\n", __func__, dmac_id);
	if (dmac_id >= RK_PL330_DMAC_MAX) {
		rk_dma_dev_err("dmac id error!\n");
		return -EIO;
	}

	pl330_info = rk_pl330_dmac_get_info(dmac_id);

	spin_lock_irqsave(&res_lock, flags);

	found = 0;
	list_for_each_entry(d, &dmac_list, node) {
		if (pl330_info != NULL && (d->pi == pl330_info)) {
			found = 1;
			break;
		}
	}

	if (!found) {
		spin_unlock_irqrestore(&res_lock, flags);
		return 0;
	}

	dmac = d;
	/* Remove all Channels that are managed only by this DMAC */
	list_for_each_entry_safe(ch, ch_bak, &chan_list, node) {
		/* Only channels that are handled by this DMAC */
		if (iface_of_dmac(dmac, ch->id))
			del = 1;
		else
			continue;

		/* Don't remove if some other DMAC has it too */
		list_for_each_entry(d, &dmac_list, node) {
			if (d != dmac && iface_of_dmac(d, ch->id)) {
				del = 0;
				break;
			}
		}
		if (del) {
			spin_unlock_irqrestore(&res_lock, flags);
			rk_dma_free(ch->id, ch->client);
			spin_lock_irqsave(&res_lock, flags);
			list_del(&ch->node);
			free(ch);
		}
	}

	/* Remove the DMAC */
	list_del(&dmac->node);
	free(dmac);

	if (dmac_id == 0) {
#ifdef CONFIG_RK_DMAC_0
		irq_handler_disable(RK_DMAC0_IRQ0);
		irq_handler_disable(RK_DMAC0_IRQ1);
		irq_uninstall_handler(RK_DMAC0_IRQ0);
		irq_uninstall_handler(RK_DMAC0_IRQ1);
		g_pl330_info_0 = NULL;
#endif
	} else {
#ifdef CONFIG_RK_DMAC_1
		irq_handler_disable(RK_DMAC1_IRQ0);
		irq_handler_disable(RK_DMAC1_IRQ1);
		irq_uninstall_handler(RK_DMAC1_IRQ0);
		irq_uninstall_handler(RK_DMAC1_IRQ1);
		g_pl330_info_1 = NULL;
#endif
	}
	spin_unlock_irqrestore(&res_lock, flags);

	// soft reset dmac0 and dmac1
#if defined(CONFIG_RKCHIP_RK3368) || defined(CONFIG_RKCHIP_RK3366)
#ifdef CONFIG_RK_DMAC_0
	writel(0x1<<2 | 0x1<<(2+16), RKIO_CRU_PHYS + CRU_SOFTRSTS_CON(1));
	mdelay(1);
	writel(0x0<<0 | 0x1<<(2+16), RKIO_CRU_PHYS + CRU_SOFTRSTS_CON(1));
#endif

#ifdef CONFIG_RK_DMAC_1
	writel(0x1<<0 | 0x1<<(0+16), RKIO_CRU_PHYS + CRU_SOFTRSTS_CON(4));
	mdelay(1);
	writel(0x0<<0 | 0x1<<(0+16), RKIO_CRU_PHYS + CRU_SOFTRSTS_CON(4));
#endif

#elif defined(CONFIG_RKCHIP_RK322XH)

#ifdef CONFIG_RK_DMAC_0
	writel(0x1<<8 | 0x1<<(8+16), RKIO_CRU_PHYS + CRU_SOFTRSTS_CON(3));
	mdelay(1);
	writel(0x0<<8 | 0x1<<(8+16), RKIO_CRU_PHYS + CRU_SOFTRSTS_CON(3));
#endif

#else
	#error "PLS config platform for dmac deinit."
#endif
	return 0;
}


void rk_pl330_dmac_init_all(void)
{
#ifdef CONFIG_RK_DMAC_0
	if (rk_pl330_dmac_init(0) != 0)
		rk_dma_dev_err("rk pl330 dmac0 init fail!\n");
#endif
#ifdef CONFIG_RK_DMAC_1
	if (rk_pl330_dmac_init(1) != 0)
		rk_dma_dev_err("rk pl330 dmac1 init fail!\n");
#endif
}


void rk_pl330_dmac_deinit_all(void)
{
#ifdef CONFIG_RK_DMAC_0
	if (rk_pl330_dmac_deinit(0) != 0)
		rk_dma_dev_err("rk pl330 dmac0 deinit fail!\n");
#endif
#ifdef CONFIG_RK_DMAC_1
	if (rk_pl330_dmac_deinit(1) != 0)
		rk_dma_dev_err("rk pl330 dmac1 deinit fail!\n");
#endif
}
