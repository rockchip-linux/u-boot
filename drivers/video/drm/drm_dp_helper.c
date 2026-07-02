// SPDX-License-Identifier: GPL-2.0+
/*
 * Copyright © 2009 Keith Packard
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that copyright
 * notice and this permission notice appear in supporting documentation, and
 * that the name of the copyright holders not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  The copyright holders make no representations
 * about the suitability of this software for any purpose.  It is provided "as
 * is" without express or implied warranty.
 *
 * THE COPYRIGHT HOLDERS DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
 * OF THIS SOFTWARE.
 */

#include <backlight.h>
#include <common.h>
#include <dm/device.h>
#include <dm/device-internal.h>
#include <dm/lists.h>
#include <drm/drm_dp_helper.h>

#include "rockchip_display.h"
#include "rockchip_panel.h"

struct dp_aux_backlight {
	struct drm_dp_aux *aux;
	struct drm_edp_backlight_info info;
	u16 default_level;
	bool enabled;
};

/**
 * DOC: dp helpers
 *
 * These functions contain some common logic and helpers at various abstraction
 * levels to deal with Display Port sink devices and related things like DP aux
 * channel transfers, EDID reading over DP aux channels, decoding certain DPCD
 * blocks, ...
 */

/* Helpers for DP link training */
static u8 dp_link_status(const u8 link_status[DP_LINK_STATUS_SIZE], int r)
{
	return link_status[r - DP_LANE0_1_STATUS];
}

static u8 dp_get_lane_status(const u8 link_status[DP_LINK_STATUS_SIZE],
			     int lane)
{
	int i = DP_LANE0_1_STATUS + (lane >> 1);
	int s = (lane & 1) * 4;
	u8 l = dp_link_status(link_status, i);

	return (l >> s) & 0xf;
}

bool drm_dp_channel_eq_ok(const u8 link_status[DP_LINK_STATUS_SIZE],
			  int lane_count)
{
	u8 lane_align;
	u8 lane_status;
	int lane;

	lane_align = dp_link_status(link_status,
				    DP_LANE_ALIGN_STATUS_UPDATED);
	if ((lane_align & DP_INTERLANE_ALIGN_DONE) == 0)
		return false;
	for (lane = 0; lane < lane_count; lane++) {
		lane_status = dp_get_lane_status(link_status, lane);
		if ((lane_status & DP_CHANNEL_EQ_BITS) != DP_CHANNEL_EQ_BITS)
			return false;
	}
	return true;
}

bool drm_dp_clock_recovery_ok(const u8 link_status[DP_LINK_STATUS_SIZE],
			      int lane_count)
{
	int lane;
	u8 lane_status;

	for (lane = 0; lane < lane_count; lane++) {
		lane_status = dp_get_lane_status(link_status, lane);
		if ((lane_status & DP_LANE_CR_DONE) == 0)
			return false;
	}
	return true;
}

u8 drm_dp_get_adjust_request_voltage(const u8 link_status[DP_LINK_STATUS_SIZE],
				     int lane)
{
	int i = DP_ADJUST_REQUEST_LANE0_1 + (lane >> 1);
	int s = ((lane & 1) ?
		 DP_ADJUST_VOLTAGE_SWING_LANE1_SHIFT :
		 DP_ADJUST_VOLTAGE_SWING_LANE0_SHIFT);
	u8 l = dp_link_status(link_status, i);

	return ((l >> s) & 0x3) << DP_TRAIN_VOLTAGE_SWING_SHIFT;
}

u8 drm_dp_get_adjust_request_pre_emphasis(const u8 link_status[DP_LINK_STATUS_SIZE],
					  int lane)
{
	int i = DP_ADJUST_REQUEST_LANE0_1 + (lane >> 1);
	int s = ((lane & 1) ?
		 DP_ADJUST_PRE_EMPHASIS_LANE1_SHIFT :
		 DP_ADJUST_PRE_EMPHASIS_LANE0_SHIFT);
	u8 l = dp_link_status(link_status, i);

	return ((l >> s) & 0x3) << DP_TRAIN_PRE_EMPHASIS_SHIFT;
}

void drm_dp_link_train_clock_recovery_delay(const u8 dpcd[DP_RECEIVER_CAP_SIZE])
{
	int rd_interval = dpcd[DP_TRAINING_AUX_RD_INTERVAL] &
			  DP_TRAINING_AUX_RD_MASK;

	if (rd_interval > 4)
		printf("AUX interval %d, out of range (max 4)\n", rd_interval);

	if (rd_interval == 0 || dpcd[DP_DPCD_REV] >= DP_DPCD_REV_14)
		udelay(100);
	else
		mdelay(rd_interval * 4);
}

void drm_dp_link_train_channel_eq_delay(const u8 dpcd[DP_RECEIVER_CAP_SIZE])
{
	int rd_interval = dpcd[DP_TRAINING_AUX_RD_INTERVAL] &
			  DP_TRAINING_AUX_RD_MASK;

	if (rd_interval > 4)
		printf("AUX interval %d, out of range (max 4)\n", rd_interval);

	if (rd_interval == 0)
		udelay(400);
	else
		mdelay(rd_interval * 4);
}

u8 drm_dp_link_rate_to_bw_code(int link_rate)
{
	/* Spec says link_bw = link_rate / 0.27Gbps */
	return link_rate / 27000;
}

int drm_dp_bw_code_to_link_rate(u8 link_bw)
{
	/* Spec says link_rate = link_bw * 0.27Gbps */
	return link_bw * 27000;
}

#define AUX_RETRY_INTERVAL 500 /* us */

static void drm_dp_dump_access(const struct drm_dp_aux *aux,
			       u8 request, unsigned int offset,
			       void *buffer, int ret)
{
	const char *arrow = (request == DP_AUX_NATIVE_READ) ? "->" : "<-";

	if (ret > 0) {
		char hex[64];
		int i;
		int len = min_t(int, ret, 20);

		for (i = 0; i < len; i++)
			sprintf(hex + i * 3, "%02x ", ((u8 *)buffer)[i]);
		hex[len * 3] = '\0';
		debug("%s: 0x%05x AUX %s (ret=%3d) %s\n",
		      aux->name, offset, arrow, ret, hex);
	} else {
		debug("%s: 0x%05x AUX %s (ret=%3d)\n",
		      aux->name, offset, arrow, ret);
	}
}

static int drm_dp_dpcd_access(struct drm_dp_aux *aux, u8 request,
			      unsigned int offset, void *buffer, size_t size)
{
	struct drm_dp_aux_msg msg;
	unsigned int retry, native_reply;
	int err = 0, ret = 0;

	memset(&msg, 0, sizeof(msg));
	msg.address = offset;
	msg.request = request;
	msg.buffer = buffer;
	msg.size = size;

	/*
	 * The specification doesn't give any recommendation on how often to
	 * retry native transactions. We used to retry 7 times like for
	 * aux i2c transactions but real world devices this wasn't
	 * sufficient, bump to 32 which makes Dell 4k monitors happier.
	 */
	for (retry = 0; retry < 32; retry++) {
		if (ret != 0 && ret != -ETIMEDOUT)
			udelay(AUX_RETRY_INTERVAL);

		ret = aux->transfer(aux, &msg);
		if (ret >= 0) {
			native_reply = msg.reply & DP_AUX_NATIVE_REPLY_MASK;
			if (native_reply == DP_AUX_NATIVE_REPLY_ACK) {
				if (ret == size)
					goto out;

				ret = -EPROTO;
			} else {
				ret = -EIO;
			}
		}

		/*
		 * We want the error we return to be the error we received on
		 * the first transaction, since we may get a different error the
		 * next time we retry
		 */
		if (!err)
			err = ret;
	}

	printf("%s: Too many retries, giving up. First error: %d\n",
	       aux->name, err);
	ret = err;

out:
	return ret;
}

ssize_t drm_dp_dpcd_read(struct drm_dp_aux *aux, unsigned int offset,
			 void *buffer, size_t size)
{
	int ret;

	ret = drm_dp_dpcd_access(aux, DP_AUX_NATIVE_READ, DP_DPCD_REV,
				 buffer, 1);
	if (ret != 1)
		goto out;

	ret = drm_dp_dpcd_access(aux, DP_AUX_NATIVE_READ, offset,
				 buffer, size);

out:
	drm_dp_dump_access(aux, DP_AUX_NATIVE_READ, offset, buffer, ret);
	return ret;
}

ssize_t drm_dp_dpcd_write(struct drm_dp_aux *aux, unsigned int offset,
			  void *buffer, size_t size)
{
	int ret;

	ret = drm_dp_dpcd_access(aux, DP_AUX_NATIVE_WRITE, offset,
				 buffer, size);

	drm_dp_dump_access(aux, DP_AUX_NATIVE_WRITE, offset, buffer, ret);
	return ret;
}

int drm_dp_dpcd_read_link_status(struct drm_dp_aux *aux,
				 u8 status[DP_LINK_STATUS_SIZE])
{
	return drm_dp_dpcd_read(aux, DP_LANE0_1_STATUS, status,
				DP_LINK_STATUS_SIZE);
}

static u8 drm_dp_downstream_port_count(const u8 dpcd[DP_RECEIVER_CAP_SIZE])
{
	u8 port_count = dpcd[DP_DOWN_STREAM_PORT_COUNT] & DP_PORT_COUNT_MASK;

	if (dpcd[DP_DOWNSTREAMPORT_PRESENT] & DP_DETAILED_CAP_INFO_AVAILABLE && port_count > 4)
		port_count = 4;

	return port_count;
}

static int drm_dp_read_extended_dpcd_caps(struct drm_dp_aux *aux,
					  u8 dpcd[DP_RECEIVER_CAP_SIZE])
{
	u8 dpcd_ext[6];
	int ret;

	/*
	 * Prior to DP1.3 the bit represented by
	 * DP_EXTENDED_RECEIVER_CAP_FIELD_PRESENT was reserved.
	 * If it is set DP_DPCD_REV at 0000h could be at a value less than
	 * the true capability of the panel. The only way to check is to
	 * then compare 0000h and 2200h.
	 */
	if (!(dpcd[DP_TRAINING_AUX_RD_INTERVAL] &
	      DP_EXTENDED_RECEIVER_CAP_FIELD_PRESENT))
		return 0;

	ret = drm_dp_dpcd_read(aux, DP_DP13_DPCD_REV, &dpcd_ext,
			       sizeof(dpcd_ext));
	if (ret < 0)
		return ret;
	if (ret != sizeof(dpcd_ext))
		return -EIO;

	if (dpcd[DP_DPCD_REV] > dpcd_ext[DP_DPCD_REV]) {
		printf("%s: Extended DPCD rev less than base DPCD rev (%d > %d)\n",
		       aux->name, dpcd[DP_DPCD_REV], dpcd_ext[DP_DPCD_REV]);
		return 0;
	}

	if (!memcmp(dpcd, dpcd_ext, sizeof(dpcd_ext)))
		return 0;

	debug("%s: Base DPCD: %*ph\n",
	       aux->name, DP_RECEIVER_CAP_SIZE, dpcd);

	memcpy(dpcd, dpcd_ext, sizeof(dpcd_ext));

	return 0;
}

int drm_dp_read_dpcd_caps(struct drm_dp_aux *aux,
			  u8 dpcd[DP_RECEIVER_CAP_SIZE])
{
	int ret;

	ret = drm_dp_dpcd_read(aux, DP_DPCD_REV, dpcd, DP_RECEIVER_CAP_SIZE);
	if (ret < 0)
		return ret;
	if (ret != DP_RECEIVER_CAP_SIZE || dpcd[DP_DPCD_REV] == 0)
		return -EIO;

	ret = drm_dp_read_extended_dpcd_caps(aux, dpcd);
	if (ret < 0)
		return ret;

	debug("%s: DPCD: %*ph\n",
	       aux->name, DP_RECEIVER_CAP_SIZE, dpcd);

	return ret;
}

int drm_dp_read_downstream_info(struct drm_dp_aux *aux,
				const u8 dpcd[DP_RECEIVER_CAP_SIZE],
				u8 downstream_ports[DP_MAX_DOWNSTREAM_PORTS])
{
	int ret;
	u8 len;

	memset(downstream_ports, 0, DP_MAX_DOWNSTREAM_PORTS);

	/* No downstream info to read */
	if (!drm_dp_is_branch(dpcd) ||
	    dpcd[DP_DPCD_REV] < DP_DPCD_REV_10 ||
	    !(dpcd[DP_DOWNSTREAMPORT_PRESENT] & DP_DWN_STRM_PORT_PRESENT))
		return 0;

	/* Some branches advertise having 0 downstream ports, despite also advertising they have a
	 * downstream port present. The DP spec isn't clear on if this is allowed or not, but since
	 * some branches do it we need to handle it regardless.
	 */
	len = drm_dp_downstream_port_count(dpcd);
	if (!len)
		return 0;

	if (dpcd[DP_DOWNSTREAMPORT_PRESENT] & DP_DETAILED_CAP_INFO_AVAILABLE)
		len *= 4;

	ret = drm_dp_dpcd_read(aux, DP_DOWNSTREAM_PORT_0, downstream_ports, len);
	if (ret < 0)
		return ret;
	if (ret != len)
		return -EIO;

	printf("%s: DPCD DFP: %*ph\n",
		      aux->name, len, downstream_ports);

	return 0;
}
int drm_dp_downstream_max_dotclock(const u8 dpcd[DP_RECEIVER_CAP_SIZE],
				   const u8 port_cap[4])
{
	if (!drm_dp_is_branch(dpcd))
		return 0;

	if (dpcd[DP_DPCD_REV] < 0x11)
		return 0;

	switch (port_cap[0] & DP_DS_PORT_TYPE_MASK) {
	case DP_DS_PORT_TYPE_VGA:
		if ((dpcd[DP_DOWNSTREAMPORT_PRESENT] & DP_DETAILED_CAP_INFO_AVAILABLE) == 0)
			return 0;
		return port_cap[1] * 8000;
	default:
		return 0;
	}
}

int drm_dp_downstream_max_tmds_clock(const u8 dpcd[DP_RECEIVER_CAP_SIZE],
				     const u8 port_cap[4])
{
	if (!drm_dp_is_branch(dpcd))
		return 0;

	if (dpcd[DP_DPCD_REV] < 0x11) {
		switch (dpcd[DP_DOWNSTREAMPORT_PRESENT] & DP_DWN_STRM_PORT_TYPE_MASK) {
		case DP_DWN_STRM_PORT_TYPE_TMDS:
			return 165000;
		default:
			return 0;
		}
	}

	switch (port_cap[0] & DP_DS_PORT_TYPE_MASK) {
	case DP_DS_PORT_TYPE_DP_DUALMODE:
			return 0;
		/*
		 * It's left up to the driver to check the
		 * DP dual mode adapter's max TMDS clock.
		 *
		 * Unfortunatley it looks like branch devices
		 * may not fordward that the DP dual mode i2c
		 * access so we just usually get i2c nak :(
		 */
	case DP_DS_PORT_TYPE_HDMI:
		 /*
		  * We should perhaps assume 165 MHz when detailed cap
		  * info is not available. But looks like many typical
		  * branch devices fall into that category and so we'd
		  * probably end up with users complaining that they can't
		  * get high resolution modes with their favorite dongle.
		  *
		  * So let's limit to 300 MHz instead since DPCD 1.4
		  * HDMI 2.0 DFPs are required to have the detailed cap
		  * info. So it's more likely we're dealing with a HDMI 1.4
		  * compatible* device here.
		  */
		if ((dpcd[DP_DOWNSTREAMPORT_PRESENT] & DP_DETAILED_CAP_INFO_AVAILABLE) == 0)
			return 300000;
		return port_cap[1] * 2500;
	case DP_DS_PORT_TYPE_DVI:
		if ((dpcd[DP_DOWNSTREAMPORT_PRESENT] & DP_DETAILED_CAP_INFO_AVAILABLE) == 0)
			return 165000;
		/* FIXME what to do about DVI dual link? */
		return port_cap[1] * 2500;
	default:
		return 0;
	}
}

int drm_dp_downstream_min_tmds_clock(const u8 dpcd[DP_RECEIVER_CAP_SIZE],
				     const u8 port_cap[4])
{
	if (!drm_dp_is_branch(dpcd))
		return 0;

	if (dpcd[DP_DPCD_REV] < 0x11) {
		switch (dpcd[DP_DOWNSTREAMPORT_PRESENT] & DP_DWN_STRM_PORT_TYPE_MASK) {
		case DP_DWN_STRM_PORT_TYPE_TMDS:
			return 25000;
		default:
			return 0;
		}
	}

	switch (port_cap[0] & DP_DS_PORT_TYPE_MASK) {
	case DP_DS_PORT_TYPE_DP_DUALMODE:
			return 0;
	case DP_DS_PORT_TYPE_DVI:
	case DP_DS_PORT_TYPE_HDMI:
		/*
		 * Unclear whether the protocol converter could
		 * utilize pixel replication. Assume it won't.
		 */
		return 25000;
	default:
		return 0;
	}
}

int drm_dp_downstream_max_bpc(const u8 dpcd[DP_RECEIVER_CAP_SIZE],
			      const u8 port_cap[4])
{
	if (!drm_dp_is_branch(dpcd))
		return 0;

	if (dpcd[DP_DPCD_REV] < 0x11) {
		switch (dpcd[DP_DOWNSTREAMPORT_PRESENT] & DP_DWN_STRM_PORT_TYPE_MASK) {
		case DP_DWN_STRM_PORT_TYPE_DP:
			return 0;
		default:
			return 8;
		}
	}

	switch (port_cap[0] & DP_DS_PORT_TYPE_MASK) {
	case DP_DS_PORT_TYPE_DP:
		return 0;
	case DP_DS_PORT_TYPE_DP_DUALMODE:
			return 0;
	case DP_DS_PORT_TYPE_HDMI:
	case DP_DS_PORT_TYPE_DVI:
	case DP_DS_PORT_TYPE_VGA:
		if ((dpcd[DP_DOWNSTREAMPORT_PRESENT] & DP_DETAILED_CAP_INFO_AVAILABLE) == 0)
			return 8;

		switch (port_cap[2] & DP_DS_MAX_BPC_MASK) {
		case DP_DS_8BPC:
			return 8;
		case DP_DS_10BPC:
			return 10;
		case DP_DS_12BPC:
			return 12;
		case DP_DS_16BPC:
			return 16;
		default:
			return 8;
		}
		break;
	default:
		return 8;
	}
}

bool drm_dp_downstream_420_passthrough(const u8 dpcd[DP_RECEIVER_CAP_SIZE],
				       const u8 port_cap[4])
{
	if (!drm_dp_is_branch(dpcd))
		return false;

	if (dpcd[DP_DPCD_REV] < 0x13)
		return false;

	switch (port_cap[0] & DP_DS_PORT_TYPE_MASK) {
	case DP_DS_PORT_TYPE_DP:
		return true;
	case DP_DS_PORT_TYPE_HDMI:
		if ((dpcd[DP_DOWNSTREAMPORT_PRESENT] & DP_DETAILED_CAP_INFO_AVAILABLE) == 0)
			return false;

		return port_cap[3] & DP_DS_HDMI_YCBCR420_PASS_THROUGH;
	default:
		return false;
	}
}

bool drm_dp_downstream_444_to_420_conversion(const u8 dpcd[DP_RECEIVER_CAP_SIZE],
					     const u8 port_cap[4])
{
	if (!drm_dp_is_branch(dpcd))
		return false;

	if (dpcd[DP_DPCD_REV] < 0x13)
		return false;

	switch (port_cap[0] & DP_DS_PORT_TYPE_MASK) {
	case DP_DS_PORT_TYPE_HDMI:
		if ((dpcd[DP_DOWNSTREAMPORT_PRESENT] & DP_DETAILED_CAP_INFO_AVAILABLE) == 0)
			return false;

		return port_cap[3] & DP_DS_HDMI_YCBCR444_TO_420_CONV;
	default:
		return false;
	}
}

static void drm_dp_i2c_msg_write_status_update(struct drm_dp_aux_msg *msg)
{
	/*
	 * In case of i2c defer or short i2c ack reply to a write,
	 * we need to switch to WRITE_STATUS_UPDATE to drain the
	 * rest of the message
	 */
	if ((msg->request & ~DP_AUX_I2C_MOT) == DP_AUX_I2C_WRITE) {
		msg->request &= DP_AUX_I2C_MOT;
		msg->request |= DP_AUX_I2C_WRITE_STATUS_UPDATE;
	}
}

static int drm_dp_i2c_do_msg(struct drm_dp_aux *aux, struct drm_dp_aux_msg *msg)
{
	unsigned int retry, defer_i2c;
	int ret;
	/*
	 * DP1.2 sections 2.7.7.1.5.6.1 and 2.7.7.1.6.6.1: A DP Source device
	 * is required to retry at least seven times upon receiving AUX_DEFER
	 * before giving up the AUX transaction.
	 *
	 * We also try to account for the i2c bus speed.
	 */
	int max_retries = 7;

	for (retry = 0, defer_i2c = 0; retry < (max_retries + defer_i2c);
	     retry++) {
		ret = aux->transfer(aux, msg);
		if (ret < 0) {
			if (ret == -EBUSY)
				continue;

			/*
			 * While timeouts can be errors, they're usually normal
			 * behavior (for instance, when a driver tries to
			 * communicate with a non-existent DisplayPort device).
			 * Avoid spamming the kernel log with timeout errors.
			 */
			if (ret == -ETIMEDOUT)
				printf("%s: transaction timed out\n",
				       aux->name);
			else
				printf("%s: transaction failed: %d\n",
				       aux->name, ret);
			return ret;
		}

		switch (msg->reply & DP_AUX_NATIVE_REPLY_MASK) {
		case DP_AUX_NATIVE_REPLY_ACK:
			/*
			 * For I2C-over-AUX transactions this isn't enough, we
			 * need to check for the I2C ACK reply.
			 */
			break;

		case DP_AUX_NATIVE_REPLY_NACK:
			printf("%s: native nack (result=%d, size=%zu)\n",
			       aux->name, ret, msg->size);
			return -EREMOTEIO;

		case DP_AUX_NATIVE_REPLY_DEFER:
			printf("%s: native defer\n", aux->name);
			/*
			 * We could check for I2C bit rate capabilities and if
			 * available adjust this interval. We could also be
			 * more careful with DP-to-legacy adapters where a
			 * long legacy cable may force very low I2C bit rates.
			 *
			 * For now just defer for long enough to hopefully be
			 * safe for all use-cases.
			 */
			udelay(AUX_RETRY_INTERVAL);
			continue;

		default:
			printf("%s: invalid native reply %#04x\n",
			       aux->name, msg->reply);
			return -EREMOTEIO;
		}

		switch (msg->reply & DP_AUX_I2C_REPLY_MASK) {
		case DP_AUX_I2C_REPLY_ACK:
			/*
			 * Both native ACK and I2C ACK replies received. We
			 * can assume the transfer was successful.
			 */
			if (ret != msg->size)
				drm_dp_i2c_msg_write_status_update(msg);
			return ret;

		case DP_AUX_I2C_REPLY_NACK:
			printf("%s: I2C nack (result=%d, size=%zu)\n",
			       aux->name, ret, msg->size);
			aux->i2c_nack_count++;
			return -EREMOTEIO;

		case DP_AUX_I2C_REPLY_DEFER:
			printf("%s: I2C defer\n", aux->name);
			/* DP Compliance Test 4.2.2.5 Requirement:
			 * Must have at least 7 retries for I2C defers on the
			 * transaction to pass this test
			 */
			aux->i2c_defer_count++;
			if (defer_i2c < 7)
				defer_i2c++;
			udelay(AUX_RETRY_INTERVAL);
			drm_dp_i2c_msg_write_status_update(msg);

			continue;

		default:
			printf("%s: invalid I2C reply %#04x\n",
			       aux->name, msg->reply);
			return -EREMOTEIO;
		}
	}

	printf("%s: Too many retries, giving up\n", aux->name);
	return -EREMOTEIO;
}

static void drm_dp_i2c_msg_set_request(struct drm_dp_aux_msg *msg,
				       const struct i2c_msg *i2c_msg)
{
	msg->request = (i2c_msg->flags & I2C_M_RD) ?
		DP_AUX_I2C_READ : DP_AUX_I2C_WRITE;
	if (!(i2c_msg->flags & I2C_M_STOP))
		msg->request |= DP_AUX_I2C_MOT;
}

/*
 * Keep retrying drm_dp_i2c_do_msg until all data has been transferred.
 *
 * Returns an error code on failure, or a recommended transfer size on success.
 */
static int drm_dp_i2c_drain_msg(struct drm_dp_aux *aux,
				struct drm_dp_aux_msg *orig_msg)
{
	int err, ret = orig_msg->size;
	struct drm_dp_aux_msg msg = *orig_msg;

	while (msg.size > 0) {
		err = drm_dp_i2c_do_msg(aux, &msg);
		if (err <= 0)
			return err == 0 ? -EPROTO : err;

		if (err < msg.size && err < ret) {
			printf("%s: Reply: requested %zu bytes got %d bytes\n",
			       aux->name, msg.size, err);
			ret = err;
		}

		msg.size -= err;
		msg.buffer += err;
	}

	return ret;
}

int drm_dp_i2c_xfer(struct ddc_adapter *adapter, struct i2c_msg *msgs,
		    int num)
{
	struct drm_dp_aux *aux = container_of(adapter, struct drm_dp_aux, ddc);
	unsigned int i, j;
	unsigned int transfer_size;
	struct drm_dp_aux_msg msg;
	int err = 0;

	memset(&msg, 0, sizeof(msg));

	for (i = 0; i < num; i++) {
		msg.address = msgs[i].addr;
		drm_dp_i2c_msg_set_request(&msg, &msgs[i]);
		/* Send a bare address packet to start the transaction.
		 * Zero sized messages specify an address only (bare
		 * address) transaction.
		 */
		msg.buffer = NULL;
		msg.size = 0;
		err = drm_dp_i2c_do_msg(aux, &msg);

		/*
		 * Reset msg.request in case in case it got
		 * changed into a WRITE_STATUS_UPDATE.
		 */
		drm_dp_i2c_msg_set_request(&msg, &msgs[i]);

		if (err < 0)
			break;
		/* We want each transaction to be as large as possible, but
		 * we'll go to smaller sizes if the hardware gives us a
		 * short reply.
		 */
		transfer_size = DP_AUX_MAX_PAYLOAD_BYTES;
		for (j = 0; j < msgs[i].len; j += msg.size) {
			msg.buffer = msgs[i].buf + j;
			msg.size = min(transfer_size, msgs[i].len - j);

			err = drm_dp_i2c_drain_msg(aux, &msg);

			/*
			 * Reset msg.request in case in case it got
			 * changed into a WRITE_STATUS_UPDATE.
			 */
			drm_dp_i2c_msg_set_request(&msg, &msgs[i]);

			if (err < 0)
				break;
			transfer_size = err;
		}
		if (err < 0)
			break;
	}
	if (err >= 0)
		err = num;
	/* Send a bare address packet to close out the transaction.
	 * Zero sized messages specify an address only (bare
	 * address) transaction.
	 */
	msg.request &= ~DP_AUX_I2C_MOT;
	msg.buffer = NULL;
	msg.size = 0;
	(void)drm_dp_i2c_do_msg(aux, &msg);

	return err;
}

int drm_dp_read_desc(struct drm_dp_aux *aux, struct drm_dp_desc *desc,
		     bool is_branch)
{
	struct drm_dp_dpcd_ident *ident = &desc->ident;
	unsigned int offset = is_branch ? DP_BRANCH_OUI : DP_SINK_OUI;
	int ret, dev_id_len;

	ret = drm_dp_dpcd_read(aux, offset, ident, sizeof(*ident));
	if (ret < 0)
		return ret;


	dev_id_len = strnlen((const char *)ident->device_id, sizeof(ident->device_id));

	printk("%s: DP %s: OUI %*phD dev-ID %*pE HW-rev %d.%d SW-rev %d.%d quirks 0x%04x\n",
		      aux->name, is_branch ? "branch" : "sink",
		      (int)sizeof(ident->oui), ident->oui,
		      dev_id_len, ident->device_id,
		      ident->hw_rev >> 4, ident->hw_rev & 0xf,
		      ident->sw_major_rev, ident->sw_minor_rev,
		      desc->quirks);

	return 0;
}

int drm_edp_backlight_set_level(struct drm_dp_aux *aux, const struct drm_edp_backlight_info *bl,
				u16 level)
{
	int ret;
	u8 buf[2] = { 0 };

	/* The panel uses the PWM for controlling brightness levels */
	if (!bl->aux_set)
		return 0;

	if (bl->lsb_reg_used) {
		buf[0] = (level & 0xff00) >> 8;
		buf[1] = (level & 0x00ff);
	} else {
		buf[0] = level;
	}

	ret = drm_dp_dpcd_write(aux, DP_EDP_BACKLIGHT_BRIGHTNESS_MSB, buf, sizeof(buf));
	if (ret != sizeof(buf)) {
		pr_err("%s: Failed to write aux backlight level: %d\n", aux->name, ret);
		return ret < 0 ? ret : -EIO;
	}

	return 0;
}

static int
drm_edp_backlight_set_enable(struct drm_dp_aux *aux, const struct drm_edp_backlight_info *bl,
			     bool enable)
{
	int ret;
	u8 buf;

	/* This panel uses the EDP_BL_PWR GPIO for enablement */
	if (!bl->aux_enable)
		return 0;

	ret = drm_dp_dpcd_readb(aux, DP_EDP_DISPLAY_CONTROL_REGISTER, &buf);
	if (ret != 1) {
		pr_err("%s: Failed to read eDP display control register: %d\n", aux->name, ret);
		return ret < 0 ? ret : -EIO;
	}
	if (enable)
		buf |= DP_EDP_BACKLIGHT_ENABLE;
	else
		buf &= ~DP_EDP_BACKLIGHT_ENABLE;

	ret = drm_dp_dpcd_writeb(aux, DP_EDP_DISPLAY_CONTROL_REGISTER, buf);
	if (ret != 1) {
		pr_err("%s: Failed to write eDP display control register: %d\n", aux->name, ret);
		return ret < 0 ? ret : -EIO;
	}

	return 0;
}

int drm_edp_backlight_enable(struct drm_dp_aux *aux, const struct drm_edp_backlight_info *bl,
			     const u16 level)
{
	int ret;
	u8 dpcd_buf;

	if (bl->aux_set)
		dpcd_buf = DP_EDP_BACKLIGHT_CONTROL_MODE_DPCD;
	else
		dpcd_buf = DP_EDP_BACKLIGHT_CONTROL_MODE_PWM;

	if (bl->pwmgen_bit_count) {
		ret = drm_dp_dpcd_writeb(aux, DP_EDP_PWMGEN_BIT_COUNT, bl->pwmgen_bit_count);
		if (ret != 1)
			pr_debug("%s: Failed to write aux pwmgen bit count: %d\n", aux->name, ret);
	}

	if (bl->pwm_freq_pre_divider) {
		ret = drm_dp_dpcd_writeb(aux, DP_EDP_BACKLIGHT_FREQ_SET, bl->pwm_freq_pre_divider);
		if (ret != 1)
			pr_debug("%s: Failed to write aux backlight frequency: %d\n",
				 aux->name, ret);
		else
			dpcd_buf |= DP_EDP_BACKLIGHT_FREQ_AUX_SET_ENABLE;
	}

	ret = drm_dp_dpcd_writeb(aux, DP_EDP_BACKLIGHT_MODE_SET_REGISTER, dpcd_buf);
	if (ret != 1) {
		pr_debug("%s: Failed to write aux backlight mode: %d\n", aux->name, ret);
		return ret < 0 ? ret : -EIO;
	}

	ret = drm_edp_backlight_set_level(aux, bl, level);
	if (ret < 0)
		return ret;
	ret = drm_edp_backlight_set_enable(aux, bl, true);
	if (ret < 0)
		return ret;

	return 0;
}

int drm_edp_backlight_disable(struct drm_dp_aux *aux, const struct drm_edp_backlight_info *bl)
{
	int ret;

	ret = drm_edp_backlight_set_enable(aux, bl, false);
	if (ret < 0)
		return ret;

	return 0;
}

static inline int
drm_edp_backlight_probe_max(struct drm_dp_aux *aux, struct drm_edp_backlight_info *bl,
			    u16 driver_pwm_freq_hz, const u8 edp_dpcd[EDP_DISPLAY_CTL_CAP_SIZE])
{
	int fxp, fxp_min, fxp_max, fxp_actual, f = 1;
	int ret;
	u8 pn, pn_min, pn_max;

	if (!bl->aux_set)
		return 0;

	ret = drm_dp_dpcd_readb(aux, DP_EDP_PWMGEN_BIT_COUNT, &pn);
	if (ret != 1) {
		pr_debug("%s: Failed to read pwmgen bit count cap: %d\n", aux->name, ret);
		return -ENODEV;
	}

	pn &= DP_EDP_PWMGEN_BIT_COUNT_MASK;
	bl->max = (1 << pn) - 1;
	if (!driver_pwm_freq_hz)
		return 0;

	/*
	 * Set PWM Frequency divider to match desired frequency provided by the driver.
	 * The PWM Frequency is calculated as 27Mhz / (F x P).
	 * - Where F = PWM Frequency Pre-Divider value programmed by field 7:0 of the
	 *             EDP_BACKLIGHT_FREQ_SET register (DPCD Address 00728h)
	 * - Where P = 2^Pn, where Pn is the value programmed by field 4:0 of the
	 *             EDP_PWMGEN_BIT_COUNT register (DPCD Address 00724h)
	 */

	/* Find desired value of (F x P)
	 * Note that, if F x P is out of supported range, the maximum value or minimum value will
	 * applied automatically. So no need to check that.
	 */
	fxp = DIV_ROUND_CLOSEST(1000 * DP_EDP_BACKLIGHT_FREQ_BASE_KHZ, driver_pwm_freq_hz);

	/* Use highest possible value of Pn for more granularity of brightness adjustment while
	 * satisfying the conditions below.
	 * - Pn is in the range of Pn_min and Pn_max
	 * - F is in the range of 1 and 255
	 * - FxP is within 25% of desired value.
	 *   Note: 25% is arbitrary value and may need some tweak.
	 */
	ret = drm_dp_dpcd_readb(aux, DP_EDP_PWMGEN_BIT_COUNT_CAP_MIN, &pn_min);
	if (ret != 1) {
		pr_debug("%s: Failed to read pwmgen bit count cap min: %d\n", aux->name, ret);
		return 0;
	}
	ret = drm_dp_dpcd_readb(aux, DP_EDP_PWMGEN_BIT_COUNT_CAP_MAX, &pn_max);
	if (ret != 1) {
		pr_debug("%s: Failed to read pwmgen bit count cap max: %d\n", aux->name, ret);
		return 0;
	}
	pn_min &= DP_EDP_PWMGEN_BIT_COUNT_MASK;
	pn_max &= DP_EDP_PWMGEN_BIT_COUNT_MASK;

	/* Ensure frequency is within 25% of desired value */
	fxp_min = DIV_ROUND_CLOSEST(fxp * 3, 4);
	fxp_max = DIV_ROUND_CLOSEST(fxp * 5, 4);
	if (fxp_min < (1 << pn_min) || (255 << pn_max) < fxp_max) {
		pr_debug("%s: Driver defined backlight frequency (%d) out of range\n",
			 aux->name, driver_pwm_freq_hz);
		return 0;
	}

	for (pn = pn_max; pn >= pn_min; pn--) {
		f = clamp(DIV_ROUND_CLOSEST(fxp, 1 << pn), 1, 255);
		fxp_actual = f << pn;
		if (fxp_min <= fxp_actual && fxp_actual <= fxp_max)
			break;
	}

	ret = drm_dp_dpcd_writeb(aux, DP_EDP_PWMGEN_BIT_COUNT, pn);
	if (ret != 1) {
		pr_debug("%s: Failed to write aux pwmgen bit count: %d\n", aux->name, ret);
		return 0;
	}
	bl->pwmgen_bit_count = pn;
	bl->max = (1 << pn) - 1;

	if (edp_dpcd[2] & DP_EDP_BACKLIGHT_FREQ_AUX_SET_CAP) {
		bl->pwm_freq_pre_divider = f;
		pr_debug("%s: Using backlight frequency from driver (%dHz)\n",
			 aux->name, driver_pwm_freq_hz);
	}

	return 0;
}

static inline int
drm_edp_backlight_probe_state(struct drm_dp_aux *aux, struct drm_edp_backlight_info *bl,
			      u8 *current_mode)
{
	int ret;
	u8 buf[2];
	u8 mode_reg;

	ret = drm_dp_dpcd_readb(aux, DP_EDP_BACKLIGHT_MODE_SET_REGISTER, &mode_reg);
	if (ret != 1) {
		pr_debug("%s: Failed to read backlight mode: %d\n", aux->name, ret);
		return ret < 0 ? ret : -EIO;
	}

	*current_mode = (mode_reg & DP_EDP_BACKLIGHT_CONTROL_MODE_MASK);
	if (!bl->aux_set)
		return 0;

	if (*current_mode == DP_EDP_BACKLIGHT_CONTROL_MODE_DPCD) {
		int size = 1 + bl->lsb_reg_used;

		ret = drm_dp_dpcd_read(aux, DP_EDP_BACKLIGHT_BRIGHTNESS_MSB, buf, size);
		if (ret != size) {
			pr_debug("%s: Failed to read backlight level: %d\n", aux->name, ret);
			return ret < 0 ? ret : -EIO;
		}

		if (bl->lsb_reg_used)
			return (buf[0] << 8) | buf[1];
		else
			return buf[0];
	}

	/*
	 * If we're not in DPCD control mode yet, the programmed brightness value is meaningless and
	 * the driver should assume max brightness
	 */
	return bl->max;
}

int
drm_edp_backlight_init(struct drm_dp_aux *aux, struct drm_edp_backlight_info *bl,
		       u16 driver_pwm_freq_hz, const u8 edp_dpcd[EDP_DISPLAY_CTL_CAP_SIZE],
		       u16 *current_level, u8 *current_mode)
{
	int ret;

	if (edp_dpcd[1] & DP_EDP_BACKLIGHT_AUX_ENABLE_CAP)
		bl->aux_enable = true;
	if (edp_dpcd[2] & DP_EDP_BACKLIGHT_BRIGHTNESS_AUX_SET_CAP)
		bl->aux_set = true;
	if (edp_dpcd[2] & DP_EDP_BACKLIGHT_BRIGHTNESS_BYTE_COUNT)
		bl->lsb_reg_used = true;

	/* Sanity check caps */
	if (!bl->aux_set && !(edp_dpcd[2] & DP_EDP_BACKLIGHT_BRIGHTNESS_PWM_PIN_CAP)) {
		pr_debug("%s: Panel supports neither AUX or PWM brightness control? Aborting\n",
			 aux->name);
		return -EINVAL;
	}

	ret = drm_edp_backlight_probe_max(aux, bl, driver_pwm_freq_hz, edp_dpcd);
	if (ret < 0)
		return ret;

	ret = drm_edp_backlight_probe_state(aux, bl, current_mode);
	if (ret < 0)
		return ret;
	*current_level = ret;

	pr_debug("%s: Found backlight: aux_set=%d aux_enable=%d mode=%d\n",
		 aux->name, bl->aux_set, bl->aux_enable, *current_mode);
	if (bl->aux_set) {
		pr_debug("%s: Backlight caps: level=%d/%d pwm_freq_pre_divider=%d lsb_reg_used=%d\n",
			 aux->name, *current_level, bl->max, bl->pwm_freq_pre_divider,
			 bl->lsb_reg_used);
	}

	return 0;
}

static int dp_aux_backlight_enable(struct udevice *dev)
{
	struct drm_dp_aux *aux = (struct drm_dp_aux *)dev_get_driver_data(dev);
	struct dp_aux_backlight *bl = dev_get_priv(dev);
	u8 edp_dpcd[EDP_DISPLAY_CTL_CAP_SIZE];
	u8 current_mode;
	int ret;

	if (!bl->enabled) {
		ret = drm_dp_dpcd_read(aux, DP_EDP_DPCD_REV, edp_dpcd,
				       EDP_DISPLAY_CTL_CAP_SIZE);
		if (ret < 0)
			return ret;

		ret = drm_edp_backlight_init(aux, &bl->info, 0, edp_dpcd,
					     &bl->default_level, &current_mode);
		if (ret < 0)
			return ret;

		ret = drm_edp_backlight_enable(bl->aux, &bl->info, bl->default_level);
		if (ret < 0)
			return ret;

		bl->enabled = true;
	}

	return 0;
}

static int dp_aux_backlight_disable(struct udevice *dev)
{
	struct dp_aux_backlight *bl = dev_get_priv(dev);
	int ret;

	if (bl->enabled) {
		ret = drm_edp_backlight_disable(bl->aux, &bl->info);
		if (ret < 0) {
			pr_err("%s: Failed to disable AUX backlight\n", bl->aux->name);
			return ret;
		}

		bl->enabled = false;
	}

	return 0;
}

static const struct backlight_ops dp_aux_bl_ops = {
	.enable = dp_aux_backlight_enable,
	.disable = dp_aux_backlight_disable,
};

static int dp_aux_bl_probe(struct udevice *dev)
{
	struct drm_dp_aux *aux = (struct drm_dp_aux *)dev_get_driver_data(dev);
	struct dp_aux_backlight *bl = dev_get_priv(dev);

	bl->aux = aux;

	return 0;
}

U_BOOT_DRIVER(dp_aux_backlight) = {
	.name = "dp_aux_backlight",
	.id = UCLASS_PANEL_BACKLIGHT,
	.ops = &dp_aux_bl_ops,
	.probe = dp_aux_bl_probe,
	.priv_auto_alloc_size = sizeof(struct dp_aux_backlight),
};

int drm_panel_dp_aux_backlight(struct rockchip_panel *panel, struct drm_dp_aux *aux)
{
	struct driver *drv;
	u8 edp_dpcd[EDP_DISPLAY_CTL_CAP_SIZE];
	int ret;

	if (!panel || !panel->dev || !aux)
		return -EINVAL;

	ret = drm_dp_dpcd_read(aux, DP_EDP_DPCD_REV, edp_dpcd,
			       EDP_DISPLAY_CTL_CAP_SIZE);
	if (ret < 0)
		return ret;

	if (!drm_edp_backlight_supported(edp_dpcd)) {
		dev_info(panel->dev, "DP AUX backlight is not supported\n");
		return 0;
	}

	drv = lists_driver_lookup_name("dp_aux_backlight");
	if (!drv) {
		dev_err(panel->dev, "Failed to find DP AUX backlight driver\n");
		return -ENOENT;
	}

	ret = device_bind_with_driver_data(panel->dev, drv, "dp_aux_backlight", (ulong)aux,
					   panel->dev->node, &panel->backlight);
	if (ret)
		return ret;

	ret = device_probe(panel->backlight);
	if (ret) {
		device_unbind(panel->backlight);
		panel->backlight = NULL;
		return ret;
	}

	return 0;
}
