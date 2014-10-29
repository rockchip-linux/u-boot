/*
 * (C) Copyright 2008-2014 Rockchip Electronics
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */
#include <malloc.h>
#include "rk_hdmi.h"
#include <lcd.h>
#include <common.h>
#include <../board/rockchip/common/config.h>
#include "rockchip_fb.h"
#include "rk3036_tve.h"

#define PARTITION_NAME "baseparamer"
#define DEFAULT_MODE   15 

#ifndef OUT_P888
#define OUT_P888 0 
#endif
short g_hdmi_vic = -1;
static struct baseparamer_pos g_pos_baseparamer = {-1, -1};

//struct hdmi_dev *hdmi = NULL;
extern int g_hdmi_noexit;

//#define HDMIDEBUG
#ifdef CONFIG_RK3036_TVE
#include <linux/fb.h>
extern struct fb_videomode rk3036_cvbs_mode [MAX_TVE_COUNT];
extern int g_tve_pos;
#endif

static const struct hdmi_video_timing hdmi_mode [] = {
		//name			refresh		xres	yres	pixclock	h_bp	h_fp	v_bp	v_fp	h_pw	v_pw	polariry			                            PorI	flag	vic		2ndvic		               pixelrepeat	interface
#ifdef CONFIG_RK3036_HDMI
	{ { "720x480i@60Hz",    60,	    720,    480,    27000000,   57,     19,     15,     4,      62,     3,		0,				                                1,      0   },  6,      HDMI_720x480i_60HZ_16_9,    0,          OUT_P888},
	{ { "720x576i@50Hz",    50,	    720,	576,	27000000,	69,	    12,	    19,	    2,	    63,	    3,		0,				                                1,	    0   },  21,     HDMI_720x576i_50HZ_16_9,    0,          OUT_P888}, 
#endif
	{ {	"720x480p@60Hz",	60,		720,	480,	27000000,	60,	    16,	    30,	    9,	    62,  	6,		0,				                                0,	    0	},	2,  	HDMI_720x480p_60HZ_16_9,	1,		    OUT_P888},
	{ {	"720x576p@50Hz",	50,		720,	576,	27000000,	68,	    12,	    39,	    5,	    64,	    5,		0,				                                0,	    0	},	17,  	HDMI_720x576p_50HZ_16_9,	1,		    OUT_P888},
	{ {	"1280x720p@24Hz",	24,		1280,	720,	59400000,	220,	1760,	20,	    5,	    40,	    5,	    FB_SYNC_HOR_HIGH_ACT | FB_SYNC_VERT_HIGH_ACT,	0,	    0	},	60,	    HDMI_1280x720p_24HZ_4_3,	1,		    OUT_P888},
	{ {	"1280x720p@25Hz",	25,		1280,	720,	74250000,	220,	2420,	20,	    5,	    40,	    5,	    FB_SYNC_HOR_HIGH_ACT | FB_SYNC_VERT_HIGH_ACT,	0,	    0	},	61,	    HDMI_1280x720p_25HZ_4_3,	1,		    OUT_P888},
	{ {	"1280x720p@30Hz",	30,		1280,	720,	74250000,	220,	1760,	20,	    5,	    40,	    5,	    FB_SYNC_HOR_HIGH_ACT | FB_SYNC_VERT_HIGH_ACT,	0,	    0	},	62,	    HDMI_1280x720p_30HZ_4_3,	1,		    OUT_P888},
	{ {	"1280x720p@50Hz",	50,		1280,	720,	74250000,	220,	440,	20,	    5,	    40,	    5,	    FB_SYNC_HOR_HIGH_ACT | FB_SYNC_VERT_HIGH_ACT,	0,	    0	},	19,  	HDMI_1280x720p_50HZ_4_3,	1,		    OUT_P888},
	{ {	"1280x720p@60Hz",	60,		1280,	720,	74250000,	220,	110,	20,	    5,	    40,	    5,	    FB_SYNC_HOR_HIGH_ACT | FB_SYNC_VERT_HIGH_ACT,	0,	    0	},	4,  	HDMI_1280x720p_60HZ_4_3,	1,		    OUT_P888},
#ifdef CONFIG_RK3036_HDMI
	{ { "1920x1080i@50Hz",	50,	    1920,   1080,   74250000,   148,    528,    15,     2,      44,     5,      FB_SYNC_HOR_HIGH_ACT | FB_SYNC_VERT_HIGH_ACT,	1,	    0   },  20,     HDMI_1920x1080i_50HZ,	    1,		    OUT_P888},
	{ { "1920x1080i@60Hz",	60,	    1920,   1080,   74250000,   148,    88,     15,     2,      44,     5,      FB_SYNC_HOR_HIGH_ACT | FB_SYNC_VERT_HIGH_ACT,	1,	    0   },  5,      HDMI_1920x1080i_60HZ,	    1,		    OUT_P888},
#endif
	{ {	"1920x1080p@24Hz",	24,		1920,	1080,	74250000,	148,	638,	36,	    4,	    44,	    5,	    FB_SYNC_HOR_HIGH_ACT | FB_SYNC_VERT_HIGH_ACT,	0,	    0	},	32,	    HDMI_1920x1080p_24HZ_4_3,	1,		    OUT_P888},
	{ {	"1920x1080p@25Hz",	25,		1920,	1080,	74250000,	148,	528,	36,	    4,	    44,	    5,	    FB_SYNC_HOR_HIGH_ACT | FB_SYNC_VERT_HIGH_ACT,	0,	    0	},	33,	    HDMI_1920x1080p_25HZ_4_3,	1,		    OUT_P888},
	{ {	"1920x1080p@30Hz",	30,		1920,	1080,	74250000,	148,	88,	    36,	    4,	    44,	    5,	    FB_SYNC_HOR_HIGH_ACT | FB_SYNC_VERT_HIGH_ACT,	0,	    0	},	34,	    HDMI_1920x1080p_30HZ_4_3,	1,		    OUT_P888},
	{ {	"1920x1080p@50Hz",	50,		1920,	1080,	148500000,	148,	528,	36,	    4,	    44,	    5,	    FB_SYNC_HOR_HIGH_ACT | FB_SYNC_VERT_HIGH_ACT,	0,	    0	},	31,  	HDMI_1920x1080p_50HZ_4_3,	1,		    OUT_P888},
	{ {	"1920x1080p@60Hz",	60,		1920,	1080,	148500000,	148,	88,	    36,	    4,	    44,	    5,	    FB_SYNC_HOR_HIGH_ACT | FB_SYNC_VERT_HIGH_ACT,	0,	    0	},	16,  	HDMI_1920x1080p_60HZ_4_3,	1,		    OUT_P888},		
#ifdef CONFIG_RK32_HDMI
	{ {	"3840x2160p@24Hz",	24,		3840,	2160,	297000000,	296,	1276,	72,	    8,	    88,	    10,	    FB_SYNC_HOR_HIGH_ACT | FB_SYNC_VERT_HIGH_ACT,	0,	    0	},	93,	    HDMI_3840x2160p_24HZ_4_3,	1,		    OUT_P888},
	{ {	"3840x2160p@25Hz",	25,		3840,	2160,	297000000,	296,	1056,	72,	    8,	    88,	    10,	    FB_SYNC_HOR_HIGH_ACT | FB_SYNC_VERT_HIGH_ACT,	0,	    0	},	94,	    HDMI_3840x2160p_25HZ_4_3,	1,		    OUT_P888},
	{ {	"3840x2160p@30Hz", 	30,		3840,	2160,	297000000,	296,	176,	72,	    8,	    88,	    10,	    FB_SYNC_HOR_HIGH_ACT | FB_SYNC_VERT_HIGH_ACT,	0,	    0	},	95,	    HDMI_3840x2160p_30HZ_4_3,	1,		    OUT_P888},
//	{ {	"3840x2160p@50Hz",	50,		3840,	2160,	594000000,	296,	1056,	72,	    8,	    88,	    10,	    FB_SYNC_HOR_HIGH_ACT | FB_SYNC_VERT_HIGH_ACT,	0,	    0	},	96,	    HDMI_3840x2160p_50HZ_4_3,	1,		    OUT_P888},
//	{ {	"3840x2160p@60Hz",	60,		3840,	2160,	594000000,	296,	176,	72,	    8,	    88,	    10,	    FB_SYNC_HOR_HIGH_ACT | FB_SYNC_VERT_HIGH_ACT,	0,	    0	},	97,	    HDMI_3840x2160p_60HZ_4_3,	1,		    OUT_P888},
#endif
};

#if defined(CONFIG_RK_FB)
static void hdmi_init_panel(struct hdmi_dev *hdmi_dev, vidinfo_t *panel)
{
	struct hdmi_video_timing *timing = NULL;
	const struct fb_videomode *mode = NULL;
	const struct hdmi_video *vpara = &hdmi_dev->video; 

	timing = (struct hdmi_video_timing *)hdmi_vic2timing(hdmi_dev, vpara->vic);
	if(timing == NULL) {
		printf("[%s] not found vic %d\n", __FUNCTION__, vpara->vic);
		return ;
	}
	mode = &(timing->mode);
	panel->pixelrepeat = !timing->pixelrepeat;
	panel->screen_type = SCREEN_HDMI; 
	panel->vl_freq    = mode->pixclock; 
	panel->vl_col     = mode->xres ;//xres
	panel->vl_row     = mode->yres;//yres
	panel->vl_width   = mode->xres;
	panel->vl_height  = mode->yres;
	//sync polarity
	panel->vl_clkp = 1;
	if(FB_SYNC_HOR_HIGH_ACT & mode->sync)
		panel->vl_hsp  = 1;
	else
		panel->vl_hsp  = 0;
	if(FB_SYNC_VERT_HIGH_ACT & mode->sync)
		panel->vl_vsp  = 1;
	else
		panel->vl_vsp  = 0;

	//h
	panel->vl_hspw = mode->hsync_len;
	panel->vl_hbpd = mode->left_margin;
	panel->vl_hfpd = mode->right_margin;
	//v
	panel->vl_vspw = mode->vsync_len;
	panel->vl_vbpd = mode->upper_margin;
	panel->vl_vfpd = mode->lower_margin;
	panel->vmode   = mode->vmode;

	HDMIDBG("%s:panel->lcd_face=%d\n panel->vl_freq=%d\n panel->vl_col=%d\n panel->vl_row=%d\n panel->vl_width=%d\n panel->vl_height=%d\n panel->vl_clkp=%d\n panel->vl_hsp=%d\n panel->vl_vsp=%d\n panel->vl_bpix=%d\n panel->vl_swap_rb=%d\n panel->vl_hspw=%d\n panel->vl_hbpd=%d\n panel->vl_hfpd=%d\n panel->vl_vspw=%d\n panel->vl_vbpd=%d\n panel->vl_vfpd=%d\n",

	__func__,
	
	panel->lcd_face,
	panel->vl_freq,
	panel->vl_col,
	panel->vl_row,
	panel->vl_width,
	panel->vl_height,
	panel->vl_clkp,
	panel->vl_hsp,
	panel->vl_vsp,
	panel->vl_bpix,
	panel->vl_swap_rb,

	/* Panel infomation */
	panel->vl_hspw,
	panel->vl_hbpd,
	panel->vl_hfpd,

	panel->vl_vspw,
	panel->vl_vbpd,
	panel->vl_vfpd);
}
#endif

DECLARE_GLOBAL_DATA_PTR;
#ifdef CONFIG_RK_DEVICEINFO
static int inline read_deviceinfo_storage(struct hdmi_dev *hdmi_dev) 
{
	int i, ret = 0;
	const disk_partition_t* ptn_deviceinfo;
	char deviceinfo_buf[8 * RK_BLK_SIZE];
	char *p_deviceinfo = CONFIG_RAM_PHY_START + CONFIG_RAM_PHY_SIZE;
	char *p_baseparamer =  CONFIG_RAM_PHY_START + CONFIG_RAM_PHY_SIZE + 0x1000;//4K
	char *p = p_deviceinfo;
	int deviceinfo_reserve_on = 0;
	int size = 0;
	int node = 0;
	int len = 0;
	int reg[2] = {0,0};

	if (!hdmi_dev)
		goto err;

	memset(p_deviceinfo, 0, 4096);
	memset(p_baseparamer, 0, 4096);


	if (gd->fdt_blob)
	{
			node = fdt_node_offset_by_compatible(gd->fdt_blob,
					0, "rockchip,deviceinfo");
			if (node < 0) {
				printf("can't find dts node for deviceinfo\n");
				return -ENODEV;
			}

			if (!fdt_device_is_available(gd->fdt_blob, node)) {
				printf("device deviceinfo is disabled\n");
				return -EPERM;
			}

			deviceinfo_reserve_on = fdtdec_get_int(gd->fdt_blob, node, "rockchip,uboot-deviceinfo-on", 0);
			if(deviceinfo_reserve_on)
			{
				if (fdtdec_get_int_array(gd->fdt_blob, node, "reg", reg, 2)) {
					printf("Cannot decode reg\n");
					return -EINVAL;
				}

				p_deviceinfo = reg[0];
				size = reg[1];
				p_baseparamer = p_deviceinfo + size / 2;

				//printf("%s:deviceinfo_reserve_on=%d,reg[0]=0x%x,reg[1]=0x%x\n",__func__, deviceinfo_reserve_on, reg[0], reg[1]);
				printf("%s:p_deviceinfo=0x%p,p_baseparamer=0x%p\n", __func__, p_deviceinfo, p_baseparamer);
			}

	}

	ptn_deviceinfo = get_disk_partition("deviceinfo");
	if (ptn_deviceinfo)
	{
		if (StorageReadLba(ptn_deviceinfo->start, deviceinfo_buf, 8) < 0)
		{
			printf("%s: Failed Read deviceinfo Partition data\n", __func__);
			goto err;
		}

		memcpy(p_deviceinfo, deviceinfo_buf, sizeof(deviceinfo_buf));
	}

#if 0
		p = p_deviceinfo;
		for(i=0; i<4096; i++)
		{
				printf("%x ",*p);
				if((i > 0) && (i % 32)== 0)
				printf("\nnum=%d\n",i/32);
				p++;
		}
		printf("\n\n");
#endif

err:
	return ret;
}
#endif

/*
 * return preset res position
 */
static int inline read_baseparamer_storage(struct hdmi_dev *hdmi_dev, struct baseparamer_pos *id)
{
	int i, ret = 0;
	const disk_partition_t* ptn_baseparamer;
	char baseparamer_buf[8 * RK_BLK_SIZE];
	char *p_baseparamer =  CONFIG_RAM_PHY_START + CONFIG_RAM_PHY_SIZE + 0x1000;//4K
	char *p = p_baseparamer;
	int deviceinfo_reserve_on = 0;
	int size = 0;
	int node = 0;
	int len = 0;
	int reg[2] = {0,0};

	if (!hdmi_dev)
		goto err;

	memset(p_baseparamer, 0, 4096);

	ptn_baseparamer = get_disk_partition("baseparamer");
	if (ptn_baseparamer)
	{
		if (StorageReadLba(ptn_baseparamer->start, baseparamer_buf, 8) < 0)
		{
			printf("%s: Failed Read baseparamer Partition data\n", __func__);
			goto err;
		}

		memcpy(p_baseparamer, baseparamer_buf, sizeof(baseparamer_buf));

		memcpy(&hdmi_dev->base_paramer_hdmi.xres, &baseparamer_buf[0], sizeof(hdmi_dev->base_paramer_hdmi.xres));
		memcpy(&hdmi_dev->base_paramer_hdmi.yres, &baseparamer_buf[4], sizeof(hdmi_dev->base_paramer_hdmi.yres));
		memcpy(&hdmi_dev->base_paramer_hdmi.interlaced, &baseparamer_buf[8], sizeof(hdmi_dev->base_paramer_hdmi.interlaced));
		memcpy(&hdmi_dev->base_paramer_hdmi.type, &baseparamer_buf[12], sizeof(hdmi_dev->base_paramer_hdmi.type));
		memcpy(&hdmi_dev->base_paramer_hdmi.refresh, &baseparamer_buf[16], sizeof(hdmi_dev->base_paramer_hdmi.refresh));

		
		for (i = 0; i < hdmi_dev->mode_len; i++) {
			if (hdmi_dev->base_paramer_hdmi.xres == hdmi_dev->modedb[i].mode.xres &&
					hdmi_dev->base_paramer_hdmi.yres == hdmi_dev->modedb[i].mode.yres &&
					   hdmi_dev->base_paramer_hdmi.refresh == hdmi_dev->modedb[i].mode.refresh &&
					    hdmi_dev->base_paramer_hdmi.interlaced == hdmi_dev->modedb[i].mode.vmode)
				break;
		}

		if (i != hdmi_dev->mode_len) {
			printf("preset hdmi resolution is %dx%d@%d-%d,i=%d\n", hdmi_dev->base_paramer_hdmi.xres, hdmi_dev->base_paramer_hdmi.yres, hdmi_dev->base_paramer_hdmi.refresh, hdmi_dev->base_paramer_hdmi.interlaced, i);
			id->hdmi_pos = i;
		}
		else
		{
			printf("hdmi baseparamer %dx%d@%d-%d\n", hdmi_dev->base_paramer_hdmi.xres, hdmi_dev->base_paramer_hdmi.yres, hdmi_dev->base_paramer_hdmi.refresh, hdmi_dev->base_paramer_hdmi.interlaced);
		}


		memcpy(&hdmi_dev->base_paramer_tve.xres, &baseparamer_buf[24+0], sizeof(hdmi_dev->base_paramer_tve.xres));
                memcpy(&hdmi_dev->base_paramer_tve.yres, &baseparamer_buf[24+4], sizeof(hdmi_dev->base_paramer_tve.yres));
                memcpy(&hdmi_dev->base_paramer_tve.interlaced, &baseparamer_buf[24+8], sizeof(hdmi_dev->base_paramer_tve.interlaced));
                memcpy(&hdmi_dev->base_paramer_tve.type, &baseparamer_buf[24+12], sizeof(hdmi_dev->base_paramer_tve.type));
                memcpy(&hdmi_dev->base_paramer_tve.refresh, &baseparamer_buf[24+16], sizeof(hdmi_dev->base_paramer_tve.refresh));

#ifdef CONFIG_RK3036_TVE
                for (i = 0; i < MAX_TVE_COUNT; i++) {
                        if (hdmi_dev->base_paramer_tve.xres == rk3036_cvbs_mode[i].xres &&
                                        hdmi_dev->base_paramer_tve.yres == rk3036_cvbs_mode[i].yres &&
                                           hdmi_dev->base_paramer_tve.refresh == rk3036_cvbs_mode[i].refresh &&
                                            hdmi_dev->base_paramer_tve.interlaced == rk3036_cvbs_mode[i].vmode)
                                break;
                }

                if (i != MAX_TVE_COUNT) {
                        printf("preset tve resolution is %dx%d@%d-%d,i=%d\n", hdmi_dev->base_paramer_tve.xres, hdmi_dev->base_paramer_tve.yres, hdmi_dev->base_paramer_tve.refresh, hdmi_dev->base_paramer_tve.interlaced, i);
                        id->tve_pos = i;
                }
                else
                {
                        printf("tve baseparamer %dx%d@%d-%d\n", hdmi_dev->base_paramer_tve.xres, hdmi_dev->base_paramer_tve.yres, hdmi_dev->base_paramer_tve.refresh, hdmi_dev->base_paramer_tve.interlaced);
                }
#endif

    }
	
#if 0
	p = p_baseparamer;
	for(i=0; i<4096; i++)
	{
			printf("%x ",*p);
			if((i > 0) && (i % 32)== 0)
			printf("\nnum=%d\n",i/32);
			p++;
	}
	printf("\n");
#endif


err:
	return ret;
}

/*
 * hdmi_vic2timing: transverse vic mode to video timing
 * @vmode: vic to transverse
 * 
 */
const struct hdmi_video_timing* hdmi_vic2timing(struct hdmi_dev *hdmi_dev, int vic)
{
    int i;
    if(vic == 0 || hdmi_dev == NULL)
        return NULL;

	HDMIDBG("%s: modedb len = %d\n", __func__, hdmi_dev->mode_len);

    for(i = 0; i < hdmi_dev->mode_len; i++)
    {
        if(hdmi_dev->modedb[i].vic == vic || hdmi_dev->modedb[i].vic_2nd == vic)
            return &(hdmi_dev->modedb[i]);
    }        
    return NULL;
} 

static int hdmi_videomode_to_vic(struct hdmi_dev *hdmi_dev, struct fb_videomode *vmode)
{
	const struct fb_videomode *mode = NULL;
	unsigned char vic = 0;
	int i = 0;
	
	if (hdmi_dev == NULL || hdmi_dev->modedb == NULL)
		return -1;

	for(i = 0; i < hdmi_dev->mode_len; i++) {
		mode = (const struct fb_videomode *)&(hdmi_dev->modedb[i].mode);
		if (mode == NULL) {
			printf("%s: NULL mode\n", __func__);
			return -1;
		}
		
		if(	vmode->vmode == mode->vmode &&
			vmode->refresh == mode->refresh &&
			vmode->xres == mode->xres && 
			vmode->left_margin == mode->left_margin &&
			vmode->right_margin == mode->right_margin &&
			vmode->upper_margin == mode->upper_margin &&
			vmode->lower_margin == mode->lower_margin && 
			vmode->hsync_len == mode->hsync_len && 
			vmode->vsync_len == mode->vsync_len) {
				vic = hdmi_dev->modedb[i].vic;
				break;
		}
	}
	return vic;
}

static void hdmi_add_vic(struct hdmi_dev *hdmi_dev, int vic)
{
	int i, exist = 0;

	if (hdmi_dev == NULL)
		return;

	for (i = 0; i < hdmi_dev->vic_pos; i++) {
		if (hdmi_dev->vicdb[i] == vic) {
			exist = 1;
			break;
		}
	}

	if (!exist && hdmi_dev->vic_pos < HDMI_VICDB_LEN) {
		hdmi_dev->vicdb[hdmi_dev->vic_pos] = vic;
		hdmi_dev->vic_pos++;
	}
}

static int hdmi_edid_checksum(unsigned char *buf)
{
	int i;
	int checksum = 0;
	
	for(i = 0; i < HDMI_EDID_BLOCK_SIZE; i++)
		checksum += buf[i];	
	
	checksum &= 0xff;
	
	if(checksum == 0)
		return E_HDMI_EDID_SUCCESS;
	else
		return E_HDMI_EDID_CHECKSUM;
}

static void calc_mode_timings(int xres, int yres, int refresh,
			      struct fb_videomode *mode)
{
	struct fb_var_screeninfo *var;

	var = malloc(sizeof(struct fb_var_screeninfo));

	if (var) {
		var->xres = xres;
		var->yres = yres;
		//fb_get_mode(FB_VSYNCTIMINGS | FB_IGNOREMON,
		//	    refresh, var, NULL);
		mode->xres = xres;
		mode->yres = yres;
		mode->pixclock = var->pixclock;
		mode->refresh = refresh;
		mode->left_margin = var->left_margin;
		mode->right_margin = var->right_margin;
		mode->upper_margin = var->upper_margin;
		mode->lower_margin = var->lower_margin;
		mode->hsync_len = var->hsync_len;
		mode->vsync_len = var->vsync_len;
		mode->vmode = 0;
		mode->sync = 0;
		free(var);
	}
}
const struct fb_videomode vesa_modes[] = {
	/* 0 640x350-85 VESA */
	{ NULL, 85, 640, 350, 31746,  96, 32, 60, 32, 64, 3,
	  FB_SYNC_HOR_HIGH_ACT, FB_VMODE_NONINTERLACED, FB_MODE_IS_VESA},
	/* 1 640x400-85 VESA */
	{ NULL, 85, 640, 400, 31746,  96, 32, 41, 01, 64, 3,
	  FB_SYNC_VERT_HIGH_ACT, FB_VMODE_NONINTERLACED, FB_MODE_IS_VESA },
	/* 2 720x400-85 VESA */
	{ NULL, 85, 721, 400, 28169, 108, 36, 42, 01, 72, 3,
	  FB_SYNC_VERT_HIGH_ACT, FB_VMODE_NONINTERLACED, FB_MODE_IS_VESA },
	/* 3 640x480-60 VESA */
	{ NULL, 60, 640, 480, 39682,  48, 16, 33, 10, 96, 2,
	  0, FB_VMODE_NONINTERLACED, FB_MODE_IS_VESA },
	/* 4 640x480-72 VESA */
	{ NULL, 72, 640, 480, 31746, 128, 24, 29, 9, 40, 2,
	  0, FB_VMODE_NONINTERLACED, FB_MODE_IS_VESA },
	/* 5 640x480-75 VESA */
	{ NULL, 75, 640, 480, 31746, 120, 16, 16, 01, 64, 3,
	  0, FB_VMODE_NONINTERLACED, FB_MODE_IS_VESA },
	/* 6 640x480-85 VESA */
	{ NULL, 85, 640, 480, 27777, 80, 56, 25, 01, 56, 3,
	  0, FB_VMODE_NONINTERLACED, FB_MODE_IS_VESA },
	/* 7 800x600-56 VESA */
	{ NULL, 56, 800, 600, 27777, 128, 24, 22, 01, 72, 2,
	  FB_SYNC_HOR_HIGH_ACT | FB_SYNC_VERT_HIGH_ACT,
	  FB_VMODE_NONINTERLACED, FB_MODE_IS_VESA },
	/* 8 800x600-60 VESA */
	{ NULL, 60, 800, 600, 25000, 88, 40, 23, 01, 128, 4,
	  FB_SYNC_HOR_HIGH_ACT | FB_SYNC_VERT_HIGH_ACT,
	  FB_VMODE_NONINTERLACED, FB_MODE_IS_VESA },
	/* 9 800x600-72 VESA */
	{ NULL, 72, 800, 600, 20000, 64, 56, 23, 37, 120, 6,
	  FB_SYNC_HOR_HIGH_ACT | FB_SYNC_VERT_HIGH_ACT,
	  FB_VMODE_NONINTERLACED, FB_MODE_IS_VESA },
	/* 10 800x600-75 VESA */
	{ NULL, 75, 800, 600, 20202, 160, 16, 21, 01, 80, 3,
	  FB_SYNC_HOR_HIGH_ACT | FB_SYNC_VERT_HIGH_ACT,
	  FB_VMODE_NONINTERLACED, FB_MODE_IS_VESA },
	/* 11 800x600-85 VESA */
	{ NULL, 85, 800, 600, 17761, 152, 32, 27, 01, 64, 3,
	  FB_SYNC_HOR_HIGH_ACT | FB_SYNC_VERT_HIGH_ACT,
	  FB_VMODE_NONINTERLACED, FB_MODE_IS_VESA },
        /* 12 1024x768i-43 VESA */
	{ NULL, 43, 1024, 768, 22271, 56, 8, 41, 0, 176, 8,
	  FB_SYNC_HOR_HIGH_ACT | FB_SYNC_VERT_HIGH_ACT,
	  FB_VMODE_INTERLACED, FB_MODE_IS_VESA },
	/* 13 1024x768-60 VESA */
	{ NULL, 60, 1024, 768, 15384, 160, 24, 29, 3, 136, 6,
	  0, FB_VMODE_NONINTERLACED, FB_MODE_IS_VESA },
	/* 14 1024x768-70 VESA */
	{ NULL, 70, 1024, 768, 13333, 144, 24, 29, 3, 136, 6,
	  0, FB_VMODE_NONINTERLACED, FB_MODE_IS_VESA },
	/* 15 1024x768-75 VESA */
	{ NULL, 75, 1024, 768, 12690, 176, 16, 28, 1, 96, 3,
	  FB_SYNC_HOR_HIGH_ACT | FB_SYNC_VERT_HIGH_ACT,
	  FB_VMODE_NONINTERLACED, FB_MODE_IS_VESA },
	/* 16 1024x768-85 VESA */
	{ NULL, 85, 1024, 768, 10582, 208, 48, 36, 1, 96, 3,
	  FB_SYNC_HOR_HIGH_ACT | FB_SYNC_VERT_HIGH_ACT,
	  FB_VMODE_NONINTERLACED, FB_MODE_IS_VESA },
	/* 17 1152x864-75 VESA */
	{ NULL, 75, 1152, 864, 9259, 256, 64, 32, 1, 128, 3,
	  FB_SYNC_HOR_HIGH_ACT | FB_SYNC_VERT_HIGH_ACT,
	  FB_VMODE_NONINTERLACED, FB_MODE_IS_VESA },
	/* 18 1280x960-60 VESA */
	{ NULL, 60, 1280, 960, 9259, 312, 96, 36, 1, 112, 3,
	  FB_SYNC_HOR_HIGH_ACT | FB_SYNC_VERT_HIGH_ACT,
	  FB_VMODE_NONINTERLACED, FB_MODE_IS_VESA },
	/* 19 1280x960-85 VESA */
	{ NULL, 85, 1280, 960, 6734, 224, 64, 47, 1, 160, 3,
	  FB_SYNC_HOR_HIGH_ACT | FB_SYNC_VERT_HIGH_ACT,
	  FB_VMODE_NONINTERLACED, FB_MODE_IS_VESA },
	/* 20 1280x1024-60 VESA */
	{ NULL, 60, 1280, 1024, 9259, 248, 48, 38, 1, 112, 3,
	  FB_SYNC_HOR_HIGH_ACT | FB_SYNC_VERT_HIGH_ACT,
	  FB_VMODE_NONINTERLACED, FB_MODE_IS_VESA },
	/* 21 1280x1024-75 VESA */
	{ NULL, 75, 1280, 1024, 7407, 248, 16, 38, 1, 144, 3,
	  FB_SYNC_HOR_HIGH_ACT | FB_SYNC_VERT_HIGH_ACT,
	  FB_VMODE_NONINTERLACED, FB_MODE_IS_VESA },
	/* 22 1280x1024-85 VESA */
	{ NULL, 85, 1280, 1024, 6349, 224, 64, 44, 1, 160, 3,
	  FB_SYNC_HOR_HIGH_ACT | FB_SYNC_VERT_HIGH_ACT,
	  FB_VMODE_NONINTERLACED, FB_MODE_IS_VESA },
	/* 23 1600x1200-60 VESA */
	{ NULL, 60, 1600, 1200, 6172, 304, 64, 46, 1, 192, 3,
	  FB_SYNC_HOR_HIGH_ACT | FB_SYNC_VERT_HIGH_ACT,
	  FB_VMODE_NONINTERLACED, FB_MODE_IS_VESA },
	/* 24 1600x1200-65 VESA */
	{ NULL, 65, 1600, 1200, 5698, 304,  64, 46, 1, 192, 3,
	  FB_SYNC_HOR_HIGH_ACT | FB_SYNC_VERT_HIGH_ACT,
	  FB_VMODE_NONINTERLACED, FB_MODE_IS_VESA },
	/* 25 1600x1200-70 VESA */
	{ NULL, 70, 1600, 1200, 5291, 304, 64, 46, 1, 192, 3,
	  FB_SYNC_HOR_HIGH_ACT | FB_SYNC_VERT_HIGH_ACT,
	  FB_VMODE_NONINTERLACED, FB_MODE_IS_VESA },
	/* 26 1600x1200-75 VESA */
	{ NULL, 75, 1600, 1200, 4938, 304, 64, 46, 1, 192, 3,
	  FB_SYNC_HOR_HIGH_ACT | FB_SYNC_VERT_HIGH_ACT,
	  FB_VMODE_NONINTERLACED, FB_MODE_IS_VESA },
	/* 27 1600x1200-85 VESA */
	{ NULL, 85, 1600, 1200, 4357, 304, 64, 46, 1, 192, 3,
	  FB_SYNC_HOR_HIGH_ACT | FB_SYNC_VERT_HIGH_ACT,
	  FB_VMODE_NONINTERLACED, FB_MODE_IS_VESA },
	/* 28 1792x1344-60 VESA */
	{ NULL, 60, 1792, 1344, 4882, 328, 128, 46, 1, 200, 3,
	  FB_SYNC_VERT_HIGH_ACT, FB_VMODE_NONINTERLACED, FB_MODE_IS_VESA },
	/* 29 1792x1344-75 VESA */
	{ NULL, 75, 1792, 1344, 3831, 352, 96, 69, 1, 216, 3,
	  FB_SYNC_VERT_HIGH_ACT, FB_VMODE_NONINTERLACED, FB_MODE_IS_VESA },
	/* 30 1856x1392-60 VESA */
	{ NULL, 60, 1856, 1392, 4580, 352, 96, 43, 1, 224, 3,
	  FB_SYNC_VERT_HIGH_ACT, FB_VMODE_NONINTERLACED, FB_MODE_IS_VESA },
	/* 31 1856x1392-75 VESA */
	{ NULL, 75, 1856, 1392, 3472, 352, 128, 104, 1, 224, 3,
	  FB_SYNC_VERT_HIGH_ACT, FB_VMODE_NONINTERLACED, FB_MODE_IS_VESA },
	/* 32 1920x1440-60 VESA */
	{ NULL, 60, 1920, 1440, 4273, 344, 128, 56, 1, 200, 3,
	  FB_SYNC_VERT_HIGH_ACT, FB_VMODE_NONINTERLACED, FB_MODE_IS_VESA },
	/* 33 1920x1440-75 VESA */
	{ NULL, 75, 1920, 1440, 3367, 352, 144, 56, 1, 224, 3,
	  FB_SYNC_VERT_HIGH_ACT, FB_VMODE_NONINTERLACED, FB_MODE_IS_VESA },
};
static int get_est_timing(unsigned char *block, struct fb_videomode *mode)
{
	int num = 0;
	unsigned char c;

	c = block[0];
	if (c&0x80) {
		calc_mode_timings(720, 400, 70, &mode[num]);
		mode[num++].flag = FB_MODE_IS_CALCULATED;
		HDMIDBG("      720x400@70Hz\n");
	}
	if (c&0x40) {
		calc_mode_timings(720, 400, 88, &mode[num]);
		mode[num++].flag = FB_MODE_IS_CALCULATED;
		HDMIDBG("      720x400@88Hz\n");
	}
	if (c&0x20) {
		mode[num++] = vesa_modes[3];
		HDMIDBG("      640x480@60Hz\n");
	}
	if (c&0x10) {
		calc_mode_timings(640, 480, 67, &mode[num]);
		mode[num++].flag = FB_MODE_IS_CALCULATED;
		HDMIDBG("      640x480@67Hz\n");
	}
	if (c&0x08) {
		mode[num++] = vesa_modes[4];
		HDMIDBG("      640x480@72Hz\n");
	}
	if (c&0x04) {
		mode[num++] = vesa_modes[5];
		HDMIDBG("      640x480@75Hz\n");
	}
	if (c&0x02) {
		mode[num++] = vesa_modes[7];
		HDMIDBG("      800x600@56Hz\n");
	}
	if (c&0x01) {
		mode[num++] = vesa_modes[8];
		HDMIDBG("      800x600@60Hz\n");
	}

	c = block[1];
	if (c&0x80) {
		mode[num++] = vesa_modes[9];
		HDMIDBG("      800x600@72Hz\n");
	}
	if (c&0x40) {
		mode[num++] = vesa_modes[10];
		HDMIDBG("      800x600@75Hz\n");
	}
	if (c&0x20) {
		calc_mode_timings(832, 624, 75, &mode[num]);
		mode[num++].flag = FB_MODE_IS_CALCULATED;
		HDMIDBG("      832x624@75Hz\n");
	}
	if (c&0x10) {
		mode[num++] = vesa_modes[12];
		HDMIDBG("      1024x768@87Hz Interlaced\n");
	}
	if (c&0x08) {
		mode[num++] = vesa_modes[13];
		HDMIDBG("      1024x768@60Hz\n");
	}
	if (c&0x04) {
		mode[num++] = vesa_modes[14];
		HDMIDBG("      1024x768@70Hz\n");
	}
	if (c&0x02) {
		mode[num++] = vesa_modes[15];
		HDMIDBG("      1024x768@75Hz\n");
	}
	if (c&0x01) {
		mode[num++] = vesa_modes[21];
		HDMIDBG("      1280x1024@75Hz\n");
	}
	c = block[2];
	if (c&0x80) {
		mode[num++] = vesa_modes[17];
		HDMIDBG("      1152x870@75Hz\n");
	}
	HDMIDBG("      Manufacturer's mask: %x\n",c&0x7F);
	return num;
}

#define VESA_MODEDB_SIZE 34
static int get_std_timing(unsigned char *block, struct fb_videomode *mode,
		int ver, int rev)
{
	int xres, yres = 0, refresh, ratio, i;

	xres = (block[0] + 31) * 8;
	if (xres <= 256)
		return 0;

	ratio = (block[1] & 0xc0) >> 6;
	switch (ratio) {
	case 0:
		/* in EDID 1.3 the meaning of 0 changed to 16:10 (prior 1:1) */
		if (ver < 1 || (ver == 1 && rev < 3))
			yres = xres;
		else
			yres = (xres * 10)/16;
		break;
	case 1:
		yres = (xres * 3)/4;
		break;
	case 2:
		yres = (xres * 4)/5;
		break;
	case 3:
		yres = (xres * 9)/16;
		break;
	}
	refresh = (block[1] & 0x3f) + 60;

	HDMIDBG("      %dx%d@%dHz\n", xres, yres, refresh);
	for (i = 0; i < VESA_MODEDB_SIZE; i++) {
		if (vesa_modes[i].xres == xres &&
		    vesa_modes[i].yres == yres &&
		    vesa_modes[i].refresh == refresh) {
			*mode = vesa_modes[i];
			mode->flag |= FB_MODE_IS_STANDARD;
			return 1;
		}
	}
	calc_mode_timings(xres, yres, refresh, mode);
	return 1;
}

static int get_dst_timing(unsigned char *block,
			  struct fb_videomode *mode, int ver, int rev)
{
	int j, num = 0;

	for (j = 0; j < 6; j++, block += STD_TIMING_DESCRIPTION_SIZE)
		num += get_std_timing(block, &mode[num], ver, rev);

	return num;
}

static void get_detailed_timing(unsigned char *block,
				struct fb_videomode *mode)
{
	mode->xres = H_ACTIVE;
	mode->yres = V_ACTIVE;
	mode->pixclock = PIXEL_CLOCK;
	mode->pixclock /= 1000;
	mode->pixclock = KHZ2PICOS(mode->pixclock);
	mode->right_margin = H_SYNC_OFFSET;
	mode->left_margin = (H_ACTIVE + H_BLANKING) -
		(H_ACTIVE + H_SYNC_OFFSET + H_SYNC_WIDTH);
	mode->upper_margin = V_BLANKING - V_SYNC_OFFSET -
		V_SYNC_WIDTH;
	mode->lower_margin = V_SYNC_OFFSET;
	mode->hsync_len = H_SYNC_WIDTH;
	mode->vsync_len = V_SYNC_WIDTH;
	if (HSYNC_POSITIVE)
		mode->sync |= FB_SYNC_HOR_HIGH_ACT;
	if (VSYNC_POSITIVE)
		mode->sync |= FB_SYNC_VERT_HIGH_ACT;
	mode->refresh = PIXEL_CLOCK/((H_ACTIVE + H_BLANKING) *
				     (V_ACTIVE + V_BLANKING));
	if (INTERLACED) {
		mode->yres *= 2;
		mode->upper_margin *= 2;
		mode->lower_margin *= 2;
		mode->vsync_len *= 2;
		mode->vmode |= FB_VMODE_INTERLACED;
	}
	mode->flag = FB_MODE_IS_DETAILED;

	HDMIDBG("      %d MHz ",  PIXEL_CLOCK/1000000);
	HDMIDBG("%d %d %d %d ", H_ACTIVE, H_ACTIVE + H_SYNC_OFFSET,
	       H_ACTIVE + H_SYNC_OFFSET + H_SYNC_WIDTH, H_ACTIVE + H_BLANKING);
	HDMIDBG("%d %d %d %d ", V_ACTIVE, V_ACTIVE + V_SYNC_OFFSET,
	       V_ACTIVE + V_SYNC_OFFSET + V_SYNC_WIDTH, V_ACTIVE + V_BLANKING);
	HDMIDBG("%sHSync %sVSync\n\n", (HSYNC_POSITIVE) ? "+" : "-",
	       (VSYNC_POSITIVE) ? "+" : "-");
}
/**
 * fb_create_modedb - create video mode database
 * @edid: EDID data
 * @dbsize: database size
 *
 * RETURNS: struct fb_videomode, @dbsize contains length of database
 *
 * DESCRIPTION:
 * This function builds a mode database using the contents of the EDID
 * data
 */
static struct fb_videomode *fb_create_modedb(unsigned char *edid, int *dbsize)
{
	struct fb_videomode *mode, *m;
	unsigned char *block;
	int num = 0, i, first = 1;
	int ver, rev;

	ver = edid[EDID_STRUCT_VERSION];
	rev = edid[EDID_STRUCT_REVISION];

	mode = malloc(50 * sizeof(struct fb_videomode));
	if (mode == NULL)
		return NULL;

	*dbsize = 0;

	HDMIDBG("   Detailed Timings\n");
	block = edid + DETAILED_TIMING_DESCRIPTIONS_START;
	for (i = 0; i < 4; i++, block+= DETAILED_TIMING_DESCRIPTION_SIZE) {
		if (!(block[0] == 0x00 && block[1] == 0x00)) {
			get_detailed_timing(block, &mode[num]);
			if (first) {
			        mode[num].flag |= FB_MODE_IS_FIRST;
				first = 0;
			}
			num++;
		}
	}

	HDMIDBG("   Supported VESA Modes\n");
	block = edid + ESTABLISHED_TIMING_1;
	num += get_est_timing(block, &mode[num]);

	HDMIDBG("   Standard Timings\n");
	block = edid + STD_TIMING_DESCRIPTIONS_START;
	for (i = 0; i < STD_TIMING; i++, block += STD_TIMING_DESCRIPTION_SIZE)
		num += get_std_timing(block, &mode[num], ver, rev);

	block = edid + DETAILED_TIMING_DESCRIPTIONS_START;
	for (i = 0; i < 4; i++, block+= DETAILED_TIMING_DESCRIPTION_SIZE) {
		if (block[0] == 0x00 && block[1] == 0x00 && block[3] == 0xfa)
			num += get_dst_timing(block + 5, &mode[num], ver, rev);
	}

	/* Yikes, EDID data is totally useless */
	if (!num) {
		free(mode);
		return NULL;
	}

	*dbsize = num;
	m = malloc(num * sizeof(struct fb_videomode));
	if (!m)
		return mode;
	memmove(m, mode, num * sizeof(struct fb_videomode));
	free(mode);
	return m;
}

/*
	@Des	Parse Detail Timing Descriptor.
	@Param	buf	:	pointer to DTD data.
	@Param	pvic:	VIC of DTD descripted.
 */
static int hdmi_edid_parse_dtd(unsigned char *block, struct fb_videomode *mode)
{
	mode->xres = H_ACTIVE;
	mode->yres = V_ACTIVE;
	mode->pixclock = PIXEL_CLOCK;
//	mode->pixclock /= 1000;
//	mode->pixclock = KHZ2PICOS(mode->pixclock);
	mode->right_margin = H_SYNC_OFFSET;
	mode->left_margin = (H_ACTIVE + H_BLANKING) -
		(H_ACTIVE + H_SYNC_OFFSET + H_SYNC_WIDTH);
	mode->upper_margin = V_BLANKING - V_SYNC_OFFSET -
		V_SYNC_WIDTH;
	mode->lower_margin = V_SYNC_OFFSET;
	mode->hsync_len = H_SYNC_WIDTH;
	mode->vsync_len = V_SYNC_WIDTH;
	if (HSYNC_POSITIVE)
		mode->sync |= FB_SYNC_HOR_HIGH_ACT;
	if (VSYNC_POSITIVE)
		mode->sync |= FB_SYNC_VERT_HIGH_ACT;
	mode->refresh = PIXEL_CLOCK/((H_ACTIVE + H_BLANKING) *
				     (V_ACTIVE + V_BLANKING));
	if (INTERLACED) {
		mode->yres *= 2;
		mode->upper_margin *= 2;
		mode->lower_margin *= 2;
		mode->vsync_len *= 2;
		mode->vmode |= FB_VMODE_INTERLACED;
	}
	mode->flag = FB_MODE_IS_DETAILED;
   
	HDMIDBG("<<<<<<<<Detailed Time>>>>>>>>>\n");
	HDMIDBG("%d KHz Refresh %d Hz",  PIXEL_CLOCK/1000, mode->refresh);
	HDMIDBG("%d %d %d %d ", H_ACTIVE, H_ACTIVE + H_SYNC_OFFSET,
	       H_ACTIVE + H_SYNC_OFFSET + H_SYNC_WIDTH, H_ACTIVE + H_BLANKING);
	HDMIDBG("%d %d %d %d ", V_ACTIVE, V_ACTIVE + V_SYNC_OFFSET,
	       V_ACTIVE + V_SYNC_OFFSET + V_SYNC_WIDTH, V_ACTIVE + V_BLANKING);
	HDMIDBG("%sHSync %sVSync\n", (HSYNC_POSITIVE) ? "+" : "-",
	       (VSYNC_POSITIVE) ? "+" : "-");

	return E_HDMI_EDID_SUCCESS;
}

static int hdmi_edid_parse_base(struct hdmi_dev *hdmi_dev, unsigned char *buf, int *extend_num)
{
	int rc, len = -1;
	
	if (hdmi_dev == NULL)
		return -1;

	if(buf == NULL || extend_num == NULL)
		return E_HDMI_EDID_PARAM;
		
	// Check first 8 byte to ensure it is an edid base block.
	if( buf[0] != 0x00 ||
	    buf[1] != 0xFF ||
	    buf[2] != 0xFF ||
	    buf[3] != 0xFF ||
	    buf[4] != 0xFF ||
	    buf[5] != 0xFF ||
	    buf[6] != 0xFF ||
	    buf[7] != 0x00)
    {
        printf("[EDID] check header error\n");
        return E_HDMI_EDID_HEAD;
    }
    
    *extend_num = buf[0x7e];
    #ifdef HDMIDEBUG
    printf("[EDID] extend block num is %d\n", buf[0x7e]);
    #endif
    
    // Checksum
    rc = hdmi_edid_checksum(buf);
    if( rc != E_HDMI_EDID_SUCCESS)
    {
    	printf("[EDID] base block checksum error\n");
    	return E_HDMI_EDID_CHECKSUM;
    }

	//pedid->specs = malloc(sizeof(struct fb_monspecs));
	//if(pedid->specs == NULL)
	//	return E_HDMI_EDID_NOMEMORY;
		
	//fb_edid_to_monspecs(buf, pedid->specs);
	fb_create_modedb(buf, &len);

	
    return E_HDMI_EDID_SUCCESS;
}

// Parse CEA Short Video Descriptor
static int hdmi_edid_get_cea_svd(struct hdmi_dev *hdmi_dev, unsigned char *buf)
{
	int count, i, vic;
	
	count = buf[0] & 0x1F;
	for(i = 0; i < count; i++)
	{
		HDMIDBG("[EDID-CEA] %02x VID %d native %d\n", buf[1 + i], buf[1 + i] & 0x7f, buf[1 + i] >> 7);
		vic = buf[1 + i] & 0x7f;
		hdmi_add_vic(hdmi_dev, vic);
	}
	
	return 0;
}

// Parse CEA Short Audio Descriptor
static int hdmi_edid_parse_cea_sad(unsigned char *buf, struct hdmi_edid *pedid)
{
	int i, count;
	
	count = buf[0] & 0x1F;
	pedid->audio = malloc((count/3)*sizeof(struct hdmi_audio));
	if(pedid->audio == NULL)
		return E_HDMI_EDID_NOMEMORY;

	pedid->audio_num = count/3;
	for(i = 0; i < pedid->audio_num; i++)
	{
		pedid->audio[i].type = (buf[1 + i*3] >> 3) & 0x0F;
		pedid->audio[i].channel = (buf[1 + i*3] & 0x07) + 1;
		pedid->audio[i].rate = buf[1 + i*3 + 1];
		if(pedid->audio[i].type == HDMI_AUDIO_LPCM)//LPCM 
		{
			pedid->audio[i].word_length = buf[1 + i*3 + 2];
		}
//		printk("[EDID-CEA] type %d channel %d rate %d word length %d\n", 
//			pedid->audio[i].type, pedid->audio[i].channel, pedid->audio[i].rate, pedid->audio[i].word_length);
	}
	return E_HDMI_EDID_SUCCESS;
}

/* Parse CEA Vendor Specific Data Block */
static int hdmi_edid_parse_cea_sdb(struct hdmi_dev *hdmi_dev, unsigned char *buf, struct hdmi_edid *pedid)
{
	unsigned int count = 0, cur_offset = 0, i = 0;
	unsigned int IEEEOUI = 0;
	unsigned int supports_ai, dc_48bit, dc_36bit, dc_30bit, dc_y444;
	unsigned int len_3d, len_4k;
	unsigned char vic = 0;
	const struct fb_videomode *mode;

	count = buf[0] & 0x1F;
	IEEEOUI = buf[3];
	IEEEOUI <<= 8;
	IEEEOUI += buf[2];
	IEEEOUI <<= 8;
	IEEEOUI += buf[1];
	HDMIDBG("[EDID-CEA] IEEEOUI is 0x%08x.\n", IEEEOUI);
	if (IEEEOUI == 0x0c03)
		pedid->sink_hdmi = 1;

	if (count > 5) {
		pedid->deepcolor = (buf[6] >> 3) & 0x0F;
		supports_ai = buf[6] >> 7;
		dc_48bit = (buf[6] >> 6) & 0x1;
		dc_36bit = (buf[6] >> 5) & 0x1;
		dc_30bit = (buf[6] >> 4) & 0x1;
		dc_y444 = (buf[6] >> 3) & 0x1;
		HDMIDBG("[EDID-CEA] supports_ai %d\n"
			"dc_48bit %d dc_36bit %d dc_30bit %d dc_y444 %d\n",
			supports_ai,
			dc_48bit, dc_36bit, dc_30bit, dc_y444);
	}
	if (count > 6)
		pedid->maxtmdsclock = buf[7] * 5000000;

	if (count > 7) {
		pedid->latency_fields_present = (buf[8] & 0x80) ? 1 : 0;
		pedid->i_latency_fields_present = (buf[8] & 0x40) ? 1 : 0;
		pedid->video_present = (buf[8] & 0x20) ? 1 : 0;
	}

	cur_offset = 9;
	if (count >= cur_offset) {
		if (pedid->latency_fields_present == 1) {
			pedid->video_latency = buf[cur_offset++];
			pedid->audio_latency = buf[cur_offset++];
		}
		if (count >= cur_offset && pedid->i_latency_fields_present) {
			pedid->interlaced_video_latency = buf[cur_offset++];
			pedid->interlaced_audio_latency = buf[cur_offset++];
		}
	}

	if (pedid->video_present == 0)
		return E_HDMI_EDID_SUCCESS;

	if (count >= cur_offset) {
		pedid->support_3d = (buf[cur_offset++] & 0x80) ? 1 : 0;

		len_4k = (buf[cur_offset] >> 5) & 0x07;
		len_3d = buf[cur_offset] & 0x1F;
		cur_offset++;
	}
	if (count >= cur_offset && len_4k > 0) {
		for (i = 0; i < len_4k; i++) {
		#ifndef HDMI_VERSION_2
			vic = buf[cur_offset + i] & 0x7f;
			if (vic > 0 && vic < 5)
				vic = (vic == 4) ? 98 : (96 - vic);
			HDMIDBG("[EDID-CEA] %02x VID %d native %d\n",
					buf[cur_offset + i],
					vic,
					buf[cur_offset + i] >> 7);
		#else
			vic = buf[cur_offset + i] & 0xff;
			HDMIDBG("[EDID-CEA] %02x VID %d native %d\n",
					buf[cur_offset + i], vic);
		#endif
			if (vic) {
				hdmi_add_vic(hdmi_dev, vic);
				//mode = hdmi_vic_to_videomode(vic);
				//if (mode)
				//	hdmi_add_videomode(mode,
				//			   &pedid->modelist);
			}
		}
		cur_offset += i;
	}

/* TODO Daisen wait to add
	if (count >= cur_offset && pedid->support_3d && len_3d > 0) {

	}
*/
	return E_HDMI_EDID_SUCCESS;
}

static int hdmi_edid_parse_3dinfo(struct hdmi_dev *hdmi_dev, unsigned char *buf/*, struct list_head *head*/)
{
	int i, len = 0, format_3d, vic_mask;
	unsigned char offset = 2/*, vic_2d, structure_3d*/;
	//struct list_head *pos;
	
	if(buf[1] & 0xF0) {
		len = (buf[1] & 0xF0) >> 4;
		for(i = 0; i < len; i++) {
			if (buf[offset])
				hdmi_add_vic(hdmi_dev, 96 - buf[offset++]);
		}
	}
	
	if(buf[0] & 0x80) {
		//3d supported
		len += (buf[0] & 0x0F) + 2;
		if( ( (buf[0] & 0x60) == 0x40) || ( (buf[0] & 0x60) == 0x20) ) {
			format_3d = buf[offset++] << 8;
			format_3d |= buf[offset++];
		}
		if( (buf[0] & 0x60) == 0x40)
			vic_mask = 0xFFFF;
		else {
			vic_mask  = buf[offset++] << 8;
			vic_mask |= buf[offset++];
		}

		//for(i = 0; i < 16; i++) {
		//	if(vic_mask & (1 << i)) {
				//j = 0;
				//for (pos = (head)->next; pos != (head); pos = pos->next) {
					//j++;
					//if(j == i) {
						//modelist = list_entry(pos, struct display_modelist, list);
						//modelist->format_3d = format_3d;
						//break;
					//}
				//}
		//	}
		//}
		//while(offset < len) {
			//vic_2d = (buf[offset] & 0xF0) >> 4;
			//structure_3d = (buf[offset++] & 0x0F);
			//j = 0;
			//for (pos = (head)->next; pos != (head); pos = pos->next) {
				//j++;
				//if(j == vic_2d) {
					//modelist = list_entry(pos, struct display_modelist, list);
					//modelist->format_3d = format_3d;
					//if(structure_3d & 0x80)
					//modelist->detail_3d = (buf[offset++] & 0xF0) >> 4;
					//break;
				//}
			//}
		//}
	}
	
	return 0;
}

// Parse CEA 861 Serial Extension.
static int hdmi_edid_parse_extensions_cea(struct hdmi_dev *hdmi_dev, unsigned char *buf)
{
	unsigned int ddc_offset, /*native_dtd_num,*/ cur_offset = 4, buf_offset;
	unsigned int /*underscan_support, */baseaudio_support;
	unsigned int tag, IEEEOUI = 0, count;
	struct hdmi_edid *pedid = NULL;
	
	if(buf == NULL || hdmi_dev == NULL)
		return E_HDMI_EDID_PARAM;
		
	pedid = &hdmi_dev->driver.edid;
	// Check ces extension version
	if(buf[1] != 3)
	{
		printf("[EDID-CEA] error version.\n");
		return E_HDMI_EDID_VERSION;
	}
	
	ddc_offset = buf[2];
//	underscan_support = (buf[3] >> 7) & 0x01;
	baseaudio_support = (buf[3] >> 6) & 0x01;
	pedid->ycbcr444 = (buf[3] >> 5) & 0x01;
	pedid->ycbcr422 = (buf[3] >> 4) & 0x01;
	//native_dtd_num = buf[3] & 0x0F;
	pedid->base_audio_support = baseaudio_support;
	
	// Parse data block
	while(cur_offset < ddc_offset)
	{
		tag = buf[cur_offset] >> 5;
		count = buf[cur_offset] & 0x1F;
		switch(tag)
		{
			case 0x02:	// Video Data Block
				HDMIDBG("[EDID-CEA] It is a Video Data Block.\n");
				hdmi_edid_get_cea_svd(hdmi_dev, buf + cur_offset);
				break;
			case 0x01:	// Audio Data Block
				HDMIDBG("[EDID-CEA] It is a Audio Data Block.\n");
				hdmi_edid_parse_cea_sad(buf + cur_offset, pedid);
				break;
			case 0x04:	// Speaker Allocation Data Block
				HDMIDBG("[EDID-CEA] It is a Speaker Allocatio Data Block.\n");
				break;
			case 0x03:	// Vendor Specific Data Block
				HDMIDBG("[EDID-CEA] It is a Vendor Specific Data Block.\n");
				hdmi_edid_parse_cea_sdb(hdmi_dev, buf + cur_offset, pedid);
#if 0
				IEEEOUI = buf[cur_offset + 3];
				IEEEOUI <<= 8;
				IEEEOUI += buf[cur_offset + 2];
				IEEEOUI <<= 8;
				IEEEOUI += buf[cur_offset + 1];
				HDMIDBG("[EDID-CEA] IEEEOUI is 0x%08x.\n", IEEEOUI);
				if(IEEEOUI == 0x0c03)
					pedid->sink_hdmi = 1;
				pedid->cecaddress = buf[cur_offset + 5];
				pedid->cecaddress |= buf[cur_offset + 4] << 8;

				if(count > 6)
					pedid->deepcolor = (buf[cur_offset + 6] >> 3) & 0x0F;					
				if(count > 7) {
					pedid->maxtmdsclock = buf[cur_offset + 7] * 5000000;
					HDMIDBG("[EDID-CEA] maxtmdsclock is %d.\n", pedid->maxtmdsclock);
				}
				if(count > 8) {
					pedid->fields_present = buf[cur_offset + 8];
					HDMIDBG("[EDID-CEA] fields_present is 0x%02x.\n", pedid->fields_present);
				}
				buf_offset = cur_offset + 9;		
				if(pedid->fields_present & 0x80)
				{
					pedid->video_latency = buf[buf_offset++];
					pedid->audio_latency = buf[buf_offset++];
				}
				if(pedid->fields_present & 0x40)
				{
					pedid->interlaced_video_latency = buf[buf_offset++];
					pedid->interlaced_audio_latency = buf[buf_offset++];
				}
				if(pedid->fields_present & 0x20) {
					hdmi_edid_parse_3dinfo(hdmi_dev, buf + buf_offset);
				}
#endif
				break;		
			case 0x05:	// VESA DTC Data Block
				HDMIDBG("[EDID-CEA] It is a VESA DTC Data Block.\n");
				break;
			case 0x07:	// Use Extended Tag
				HDMIDBG("[EDID-CEA] It is a Use Extended Tag Data Block.\n");
				break;
			default:
				HDMIDBG("[EDID-CEA] unkowned data block tag.\n");
				break;
		}
		cur_offset += (buf[cur_offset] & 0x1F) + 1;
	}
	

	// Parse DTD
	struct fb_videomode *vmode = malloc(sizeof(struct fb_videomode));
	if(vmode == NULL)
		return E_HDMI_EDID_SUCCESS; 
	while(ddc_offset < HDMI_EDID_BLOCK_SIZE - 2)	//buf[126] = 0 and buf[127] = checksum
	{
		if(!buf[ddc_offset] && !buf[ddc_offset + 1])
			break;
		memset(vmode, 0, sizeof(struct fb_videomode));
		hdmi_edid_parse_dtd(buf + ddc_offset, vmode);		
		hdmi_add_vic(hdmi_dev, hdmi_videomode_to_vic(hdmi_dev, vmode));
		ddc_offset += 18;
	}
	free(vmode);


	return E_HDMI_EDID_SUCCESS;
}

static int hdmi_edid_parse_extensions(struct hdmi_dev *hdmi_dev, unsigned char *buf)
{
	int rc;
	
	if(buf == NULL || hdmi_dev == NULL)
		return E_HDMI_EDID_PARAM;
		
	// Checksum
    rc = hdmi_edid_checksum(buf);
    if( rc != E_HDMI_EDID_SUCCESS)
    {
    	printf("[EDID] extensions block checksum error\n");
    	return E_HDMI_EDID_CHECKSUM;
    }
    
    switch(buf[0])
    {
    	case 0xF0:
    		printf("[EDID-EXTEND] It is a extensions block map.\n");
    		break;
    	case 0x02:
    		printf("[EDID-EXTEND] It is a  CEA 861 Series Extension.\n");
    		hdmi_edid_parse_extensions_cea(hdmi_dev, buf);
    		break;
    	case 0x10:
    		printf("[EDID-EXTEND] It is a Video Timing Block Extension.\n");
    		break;
    	case 0x40:
    		printf("[EDID-EXTEND] It is a Display Information Extension.\n");
    		break;
    	case 0x50:
    		printf("[EDID-EXTEND] It is a Localized String Extension.\n");
    		break;
    	case 0x60:
    		printf("[EDID-EXTEND] It is a Digital Packet Video Link Extension.\n");
    		break;
    	default:
    		printf("[EDID-EXTEND] Unkowned extension.\n");
    		return E_HDMI_EDID_UNKOWNDATA;
    }
    
    return E_HDMI_EDID_SUCCESS;
}

int hdmi_parse_edid(struct hdmi_dev *hdmi_dev)
{
	unsigned char buf[HDMI_EDID_BLOCK_SIZE];
	int rc = HDMI_ERROR_SUCESS, extendblock = 0, i, trytimes;

	if (!hdmi_dev || !hdmi_dev->read_edid)
		goto err;

	for(trytimes = 0; trytimes < 3; trytimes++) {
		memset(buf, 0 , HDMI_EDID_BLOCK_SIZE);
		if (hdmi_dev->read_edid(hdmi_dev, 0, buf) == 0) {
			rc = hdmi_edid_parse_base(hdmi_dev, buf, &extendblock);
			if (rc) 
				printf("[HDMI] parse edid base block error-%d\n", rc);
			else
				break;
		}else{
			printf("[HDMI] read edid base block error\n");
		}
	}

	if (rc)
		goto err;

	for(i = 1; i < extendblock + 1; i++) {
		memset(buf, 0 , HDMI_EDID_BLOCK_SIZE);
		if (hdmi_dev->read_edid(hdmi_dev, i, buf) == 0) {
			rc = hdmi_edid_parse_extensions(hdmi_dev, buf);
			if (rc) {
				printf("[HDMI] parse edid block %d error-%d\n", i, rc);
				continue;
			}
		}else {
			printf("[HDMI] read edid block %d error\n", i);
			goto err;
		}
	}

	return 0;

err:
	return -1;
}

void hdmi_find_best_edid_mode(struct hdmi_dev *hdmi_dev)
{
	int i = 0, pos = 0,pos_edid = 0;
	pos = hdmi_dev->mode_len;
	while (pos--) {
		for (i = 0; i < hdmi_dev->vic_pos; i++) {
			if (hdmi_dev->vicdb[i] == hdmi_dev->modedb[pos].vic) {
				pos_edid = pos;
				break;
			}
		}
		if (i != hdmi_dev->vic_pos) {
			hdmi_dev->video.vic = hdmi_dev->modedb[pos].vic;
			break;
		}
	}
}

void hdmi_find_best_mode(struct hdmi_dev *hdmi_dev)
{
	int i = 0, pos_baseparamer = 0,pos_default = 0;

	pos_baseparamer = g_pos_baseparamer.hdmi_pos;

#ifdef HDMIDEBUG
	for (i = 0; i < hdmi_dev->vic_pos; i++) {
		printf("%d ", hdmi_dev->vicdb[i]);
	}
	printf("\n");
#endif
	/*if read edid error,use default vic mode, or not check pos_baseparamer and selete best video mode*/
	if (hdmi_dev->vic_pos > 0) {
		if (pos_baseparamer >= 0) {
			for(i = 0; i < hdmi_dev->vic_pos; i++) {
				if (hdmi_dev->vicdb[i] == hdmi_dev->modedb[pos_baseparamer].vic) {
					break;
				}
			}
			if (i < hdmi_dev->vic_pos) {
				hdmi_dev->video.vic = hdmi_dev->modedb[pos_baseparamer].vic;
				printf("use baseparamer config,pos_baseparamer=%d\n",pos_baseparamer);
			} else {
				hdmi_find_best_edid_mode(hdmi_dev);
				printf("pos_baseparamer=%d,but edid not support,find best edid vic=%d\n",
					pos_baseparamer,hdmi_dev->video.vic);
			}
		} else {
			hdmi_find_best_edid_mode(hdmi_dev);
			printf("no baseparametr,find best edid mode,vic=%d\n",hdmi_dev->video.vic);
		}
	} else {
		hdmi_dev->video.vic = HDMI_VIDEO_DEFAULT_MODE;
		printf("no edid message:use default vic config:%d\n",hdmi_dev->video.vic);
	}
}

void rk_hdmi_register(struct hdmi_dev *hdmi_dev, vidinfo_t *panel)
{
	int ret = 0;
	//hdmi iomux
	rk_iomux_config(RK_HDMI_IOMUX);
	hdmi_dev->pname = PARTITION_NAME;
	//init vicdb
	hdmi_dev->vic_pos = 0;
	//init video modedb
	hdmi_dev->modedb = hdmi_mode;
	hdmi_dev->mode_len = sizeof(hdmi_mode) / sizeof(hdmi_mode[0]);
	//default out res
	hdmi_dev->video.vic = hdmi_dev->modedb[DEFAULT_MODE].vic;

#ifdef CONFIG_RK_DEVICEINFO
	ret = read_deviceinfo_storage(hdmi_dev);
	if(ret)
	printf("%s:fail to read deviceinfo\n",__func__);
#endif
	ret = read_baseparamer_storage(hdmi_dev, &g_pos_baseparamer);

	if (hdmi_dev->hd_init && !hdmi_dev->hd_init(hdmi_dev)) {
		g_hdmi_vic = hdmi_dev->video.vic;
		//config lcdc panel
#if defined(CONFIG_RK_FB)
	hdmi_init_panel(hdmi_dev, panel);
#endif
	}

#ifdef CONFIG_RK3036_TVE
	if(g_hdmi_noexit == 1)
	g_tve_pos = g_pos_baseparamer.tve_pos;
#endif

}


void rk_hdmi_probe(vidinfo_t *panel)
{
#ifdef CONFIG_RK32_HDMI
	rk32_hdmi_probe(panel);
#endif
#ifdef CONFIG_RK3036_HDMI
	rk3036_hdmi_probe(panel);
#endif
}

