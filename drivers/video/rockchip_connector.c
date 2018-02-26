/*
 * (C) Copyright 2008-2016 Fuzhou Rockchip Electronics Co., Ltd
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

static const struct rockchip_connector g_connector[] = {
#ifdef CONFIG_ROCKCHIP_DRM_TVE
	{
         .compatible = "rockchip,rk3328-tve",
         .funcs = &rockchip_drm_tve_funcs,
	},
#endif
#ifdef CONFIG_ROCKCHIP_DW_MIPI_DSI
	{
	 .compatible = "rockchip,rk3128-mipi-dsi",
	 .funcs = &rockchip_dw_mipi_dsi_funcs,
	 .data = &rk3128_mipi_dsi_drv_data,
	},
	{
	 .compatible = "rockchip,rk3288-mipi-dsi",
	 .funcs = &rockchip_dw_mipi_dsi_funcs,
	 .data = &rk3288_mipi_dsi_drv_data,
	},{
	 .compatible = "rockchip,rk3366-mipi-dsi",
	 .funcs = &rockchip_dw_mipi_dsi_funcs,
	 .data = &rk3366_mipi_dsi_drv_data,
	},{
	 .compatible = "rockchip,rk3368-mipi-dsi",
	 .funcs = &rockchip_dw_mipi_dsi_funcs,
	 .data = &rk3368_mipi_dsi_drv_data,
	},{
	 .compatible = "rockchip,rk3399-mipi-dsi",
	 .funcs = &rockchip_dw_mipi_dsi_funcs,
	 .data = &rk3399_mipi_dsi_drv_data,
	},
#endif
#ifdef CONFIG_ROCKCHIP_ANALOGIX_DP
	{
	 .compatible = "rockchip,rk3288-dp",
	 .funcs = &rockchip_analogix_dp_funcs,
	 .data = &rk3288_analogix_dp_drv_data,
	},{
	 .compatible = "rockchip,rk3399-edp",
	 .funcs = &rockchip_analogix_dp_funcs,
	 .data = &rk3399_analogix_edp_drv_data,
	},{
	 .compatible = "rockchip,rk3368-edp",
	 .funcs = &rockchip_analogix_dp_funcs,
	 .data = &rk3368_analogix_edp_drv_data,
	},
#endif
#ifdef CONFIG_ROCKCHIP_LVDS
	{
	 .compatible = "rockchip,rk3366-lvds",
	 .funcs = &rockchip_lvds_funcs,
	 .data = &rk3366_lvds_drv_data,
	},
	{
	 .compatible = "rockchip,rk3368-lvds",
	 .funcs = &rockchip_lvds_funcs,
	 .data = &rk3368_lvds_drv_data,
	},{
	 .compatible = "rockchip,rk3288-lvds",
	 .funcs = &rockchip_lvds_funcs,
	 .data = &rk3288_lvds_drv_data,
	},
	{
	 .compatible = "rockchip,rk3126-lvds",
	 .funcs = &rockchip_lvds_funcs,
	 .data = &rk3126_lvds_drv_data,
	},
#endif
#ifdef CONFIG_DRM_ROCKCHIP_DW_HDMI
	{
	 .compatible = "rockchip,rk3288-dw-hdmi",
	 .funcs = &rockchip_dw_hdmi_funcs,
	 .data = &rk3288_hdmi_drv_data,
	}, {
	 .compatible = "rockchip,rk3368-dw-hdmi",
	 .funcs = &rockchip_dw_hdmi_funcs,
	 .data = &rk3368_hdmi_drv_data,
	}, {
	 .compatible = "rockchip,rk3399-dw-hdmi",
	 .funcs = &rockchip_dw_hdmi_funcs,
	 .data = &rk3399_hdmi_drv_data,
	}, {
	 .compatible = "rockchip,rk3328-dw-hdmi",
	 .funcs = &rockchip_dw_hdmi_funcs,
	 .data = &rk3328_hdmi_drv_data,
	}, {
	 .compatible = "rockchip,rk3228-dw-hdmi",
	 .funcs = &rockchip_dw_hdmi_funcs,
	 .data = &rk3228_hdmi_drv_data,
	}

#endif
};

const struct rockchip_connector *
rockchip_get_connector(const void *blob, int connector_node)
{
	const char *name;
	int i;

	fdt_get_string(blob, connector_node, "compatible", &name);

	for (i = 0; i < ARRAY_SIZE(g_connector); i++) {
		if (!strcmp(name, g_connector[i].compatible))
			break;
	}
	if (i >= ARRAY_SIZE(g_connector))
		return NULL;

	return &g_connector[i];
}
