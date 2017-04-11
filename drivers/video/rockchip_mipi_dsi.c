/*
 * (C) Copyright 2008-2017 Fuzhou Rockchip Electronics Co., Ltd
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <config.h>
#include <common.h>
#include <errno.h>
#include <malloc.h>
#include <fdtdec.h>
#include <fdt_support.h>
#include <resource.h>
#include <asm/arch/rkplat.h>
#include <asm/unaligned.h>
#include <linux/list.h>

#include "rockchip_display.h"
#include "rockchip_crtc.h"
#include "rockchip_connector.h"
#include "rockchip_mipi_dsi.h"

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
