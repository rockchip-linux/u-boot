/*
 * (C) Copyright 2008-2017 Fuzhou Rockchip Electronics Co., Ltd
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <config.h>
#include <common.h>
#include <errno.h>
#include <malloc.h>
#include <asm/unaligned.h>
#include <linux/list.h>

#include "rockchip_display.h"
#include "rockchip_crtc.h"
#include "rockchip_connector.h"
#include "rockchip_mipi_dsi.h"

void mipi_dphy_timing_get_default(struct mipi_dphy_timing *timing,
				  unsigned long period)
{
	/* Global Operation Timing Parameters */
	timing->clkmiss = 0;
	timing->clkpost = 70 + 52 * period;
	timing->clkpre = 8 * period;
	timing->clkprepare = 65;
	timing->clksettle = 95;
	timing->clktermen = 0;
	timing->clktrail = 80;
	timing->clkzero = 260;
	timing->dtermen = 0;
	timing->eot = 0;
	timing->hsexit = 120;
	timing->hsprepare = 65 + 4 * period;
	timing->hszero = 145 + 6 * period;
	timing->hssettle = 85 + 6 * period;
	timing->hsskip = 40;
	timing->hstrail = max(8 * period, 60 + 4 * period);
	timing->init = 100000;
	timing->lpx = 60;
	timing->taget = 5 * timing->lpx;
	timing->tago = 4 * timing->lpx;
	timing->tasure = 2 * timing->lpx;
	timing->wakeup = 1000000;
}

ssize_t mipi_dsi_generic_write(struct display_state *state,
			       const void *payload, size_t size)
{
	struct connector_state *conn_state = &state->conn_state;
	const struct rockchip_connector *connector = conn_state->connector;
	struct mipi_dsi_msg msg;

	if (!connector || !connector->funcs || !connector->funcs->transfer) {
		printf("%s: failed to find connector transfer funcs\n", __func__);
		return -ENODEV;
	}

	memset(&msg, 0, sizeof(msg));
	msg.channel = 0;
	msg.tx_buf = payload;
	msg.tx_len = size;
	msg.flags |= MIPI_DSI_MSG_USE_LPM;

	switch (size) {
	case 0:
		msg.type = MIPI_DSI_GENERIC_SHORT_WRITE_0_PARAM;
		break;
	case 1:
		msg.type = MIPI_DSI_GENERIC_SHORT_WRITE_1_PARAM;
		break;
	case 2:
		msg.type = MIPI_DSI_GENERIC_SHORT_WRITE_2_PARAM;
		break;
	default:
		msg.type = MIPI_DSI_GENERIC_LONG_WRITE;
		break;
	}

	return connector->funcs->transfer(state, &msg);
}

ssize_t mipi_dsi_dcs_write(struct display_state *state,
			   const void *payload, size_t size)
{
	struct connector_state *conn_state = &state->conn_state;
	const struct rockchip_connector *connector = conn_state->connector;
	struct mipi_dsi_msg msg;

	if (!connector || !connector->funcs || !connector->funcs->transfer) {
		printf("%s: failed to find connector transfer funcs\n", __func__);
		return -ENODEV;
	}

	memset(&msg, 0, sizeof(msg));
	msg.channel = 0;
	msg.tx_buf = payload;
	msg.tx_len = size;
	msg.flags |= MIPI_DSI_MSG_USE_LPM;

	switch (size) {
	case 0:
		return -EINVAL;
	case 1:
		msg.type = MIPI_DSI_DCS_SHORT_WRITE;
		break;
	case 2:
		msg.type = MIPI_DSI_DCS_SHORT_WRITE_PARAM;
		break;
	default:
		msg.type = MIPI_DSI_DCS_LONG_WRITE;
		break;
	}

	return connector->funcs->transfer(state, &msg);
}

ssize_t mipi_dsi_generic_read(struct display_state *state, const void *params,
			      size_t num_params, void *data, size_t size)
{
	struct connector_state *conn_state = &state->conn_state;
	const struct rockchip_connector *connector = conn_state->connector;
	struct mipi_dsi_msg msg;

	if (!connector || !connector->funcs || !connector->funcs->transfer) {
		printf("%s: failed to find connector transfer funcs\n", __func__);
		return -ENODEV;
	}

	memset(&msg, 0, sizeof(msg));
	msg.channel = 0;
	msg.tx_buf = params;
	msg.tx_len = num_params;
	msg.rx_len = size;
	msg.rx_buf = data;
	msg.flags |= MIPI_DSI_MSG_USE_LPM;

	switch (num_params) {
	case 0:
		msg.type = MIPI_DSI_GENERIC_READ_REQUEST_0_PARAM;
		break;
	case 1:
		msg.type = MIPI_DSI_GENERIC_READ_REQUEST_1_PARAM;
		break;
	case 2:
		msg.type = MIPI_DSI_GENERIC_READ_REQUEST_2_PARAM;
		break;
	default:
		return -EINVAL;
	}

	return connector->funcs->transfer(state, &msg);
}

ssize_t mipi_dsi_dcs_read(struct display_state *state, u8 cmd, void *data,
			  size_t len)
{
	struct connector_state *conn_state = &state->conn_state;
	const struct rockchip_connector *connector = conn_state->connector;
	struct mipi_dsi_msg msg;

	if (!connector || !connector->funcs || !connector->funcs->transfer) {
		printf("%s: failed to find connector transfer funcs\n", __func__);
		return -ENODEV;
	}

	memset(&msg, 0, sizeof(msg));
	msg.channel = 0;
	msg.type = MIPI_DSI_DCS_READ;
	msg.tx_buf = &cmd;
	msg.tx_len = 1;
	msg.rx_len = len;
	msg.rx_buf = data;
	msg.flags |= MIPI_DSI_MSG_USE_LPM;

	return connector->funcs->transfer(state, &msg);
}

static bool mipi_dsi_packet_format_is_short(u8 type)
{
	switch (type) {
	case MIPI_DSI_V_SYNC_START:
	case MIPI_DSI_V_SYNC_END:
	case MIPI_DSI_H_SYNC_START:
	case MIPI_DSI_H_SYNC_END:
	case MIPI_DSI_END_OF_TRANSMISSION:
	case MIPI_DSI_COLOR_MODE_OFF:
	case MIPI_DSI_COLOR_MODE_ON:
	case MIPI_DSI_SHUTDOWN_PERIPHERAL:
	case MIPI_DSI_TURN_ON_PERIPHERAL:
	case MIPI_DSI_GENERIC_SHORT_WRITE_0_PARAM:
	case MIPI_DSI_GENERIC_SHORT_WRITE_1_PARAM:
	case MIPI_DSI_GENERIC_SHORT_WRITE_2_PARAM:
	case MIPI_DSI_GENERIC_READ_REQUEST_0_PARAM:
	case MIPI_DSI_GENERIC_READ_REQUEST_1_PARAM:
	case MIPI_DSI_GENERIC_READ_REQUEST_2_PARAM:
	case MIPI_DSI_DCS_SHORT_WRITE:
	case MIPI_DSI_DCS_SHORT_WRITE_PARAM:
	case MIPI_DSI_DCS_READ:
	case MIPI_DSI_SET_MAXIMUM_RETURN_PACKET_SIZE:
		return true;
	}

	return false;
}

static bool mipi_dsi_packet_format_is_long(u8 type)
{
	switch (type) {
	case MIPI_DSI_NULL_PACKET:
	case MIPI_DSI_BLANKING_PACKET:
	case MIPI_DSI_GENERIC_LONG_WRITE:
	case MIPI_DSI_DCS_LONG_WRITE:
	case MIPI_DSI_LOOSELY_PACKED_PIXEL_STREAM_YCBCR20:
	case MIPI_DSI_PACKED_PIXEL_STREAM_YCBCR24:
	case MIPI_DSI_PACKED_PIXEL_STREAM_YCBCR16:
	case MIPI_DSI_PACKED_PIXEL_STREAM_30:
	case MIPI_DSI_PACKED_PIXEL_STREAM_36:
	case MIPI_DSI_PACKED_PIXEL_STREAM_YCBCR12:
	case MIPI_DSI_PACKED_PIXEL_STREAM_16:
	case MIPI_DSI_PACKED_PIXEL_STREAM_18:
	case MIPI_DSI_PIXEL_STREAM_3BYTE_18:
	case MIPI_DSI_PACKED_PIXEL_STREAM_24:
		return true;
	}

	return false;
}

int mipi_dsi_create_packet(struct mipi_dsi_packet *packet,
			   const struct mipi_dsi_msg *msg)
{
	if (!packet || !msg)
		return -EINVAL;

	/* do some minimum sanity checking */
	if (!mipi_dsi_packet_format_is_short(msg->type) &&
	    !mipi_dsi_packet_format_is_long(msg->type))
		return -EINVAL;

	if (msg->channel > 3)
		return -EINVAL;

	memset(packet, 0, sizeof(*packet));
	packet->header[0] = ((msg->channel & 0x3) << 6) | (msg->type & 0x3f);
	if (mipi_dsi_packet_format_is_long(msg->type)) {
		packet->header[1] = (msg->tx_len >> 0) & 0xff;
		packet->header[2] = (msg->tx_len >> 8) & 0xff;
		packet->payload_length = msg->tx_len;
		packet->payload = msg->tx_buf;
	} else {
		const u8 *tx = msg->tx_buf;
		packet->header[1] = (msg->tx_len > 0) ? tx[0] : 0;
		packet->header[2] = (msg->tx_len > 1) ? tx[1] : 0;
	}

	packet->size = sizeof(packet->header) + packet->payload_length;

	return 0;
}
