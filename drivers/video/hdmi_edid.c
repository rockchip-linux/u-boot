#include "rk_hdmi.h"
#include <malloc.h>

typedef enum HDMI_EDID_ERRORCODE
{
	E_HDMI_EDID_SUCCESS = 0,
	E_HDMI_EDID_PARAM,
	E_HDMI_EDID_HEAD,
	E_HDMI_EDID_CHECKSUM,
	E_HDMI_EDID_VERSION,
	E_HDMI_EDID_UNKOWNDATA,
	E_HDMI_EDID_NOMEMORY
}HDMI_EDID_ErrorCode;

int hdmi_videomode_to_vic(struct fb_videomode *vmode)
{
	const struct fb_videomode *mode = NULL;
	unsigned char vic = 0;
	int i = 0;
	
	if (!hdmi || !hdmi->modedb)
		return -1;

	for(i = 0; i < hdmi->mode_len; i++) {
		mode = (const struct fb_videomode *)&(hdmi->modedb[i].mode);
		if (!mode) {
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
			//if( (vmode->vmode == FB_VMODE_NONINTERLACED && vmode->yres == mode->yres) || 
			//	(vmode->vmode == FB_VMODE_INTERLACED && vmode->yres == mode->yres/2))
			{								
				vic = hdmi->modedb[i].vic;
				break;
			}
		}
	}
	return vic;
}

void hdmi_add_vic(int vic)
{
	int i, exist = 0;

	if (!hdmi)
		return;

	for (i = 0; i < hdmi->vic_pos; i++) {
		if (hdmi->vicdb[i] == vic) {
			exist = 1;
			break;
		}
	}

	if (!exist && hdmi->vic_pos < HDMI_VICDB_LEN) {
		hdmi->vicdb[hdmi->vic_pos] = vic;
		hdmi->vic_pos++;
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

int hdmi_edid_parse_base(unsigned char *buf, int *extend_num, struct hdmi_edid *pedid)
{
	int rc, len = -1;
	
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

	printf("%s: create len %d\n", __func__, len);
	
    return E_HDMI_EDID_SUCCESS;
}

// Parse CEA Short Video Descriptor
static int hdmi_edid_get_cea_svd(unsigned char *buf, struct hdmi_edid *pedid)
{
	int count, i, vic;
	
	count = buf[0] & 0x1F;
	for(i = 0; i < count; i++)
	{
		HDMIDBG("[EDID-CEA] %02x VID %d native %d\n", buf[1 + i], buf[1 + i] & 0x7f, buf[1 + i] >> 7);
		vic = buf[1 + i] & 0x7f;
		hdmi_add_vic(vic);
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

static int hdmi_edid_parse_3dinfo(unsigned char *buf/*, struct list_head *head*/)
{
	int i, j, len = 0, format_3d, vic_mask;
	unsigned char offset = 2, vic_2d, structure_3d;
	//struct list_head *pos;
	
	if(buf[1] & 0xF0) {
		len = (buf[1] & 0xF0) >> 4;
		for(i = 0; i < len; i++) {
			hdmi_add_vic(96 - buf[offset++]);
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

		for(i = 0; i < 16; i++) {
			if(vic_mask & (1 << i)) {
				j = 0;
				//for (pos = (head)->next; pos != (head); pos = pos->next) {
					//j++;
					//if(j == i) {
						//modelist = list_entry(pos, struct display_modelist, list);
						//modelist->format_3d = format_3d;
						//break;
					//}
				//}
			}
		}
		while(offset < len) {
			vic_2d = (buf[offset] & 0xF0) >> 4;
			structure_3d = (buf[offset++] & 0x0F);
			j = 0;
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
		}
	}
	
	return 0;
}

// Parse CEA 861 Serial Extension.
static int hdmi_edid_parse_extensions_cea(unsigned char *buf, struct hdmi_edid *pedid)
{
	unsigned int ddc_offset, native_dtd_num, cur_offset = 4, buf_offset;
//	unsigned int underscan_support, baseaudio_support;
	unsigned int tag, IEEEOUI = 0, count;
	
	if(buf == NULL)
		return E_HDMI_EDID_PARAM;
		
	// Check ces extension version
	if(buf[1] != 3)
	{
		printf("[EDID-CEA] error version.\n");
		return E_HDMI_EDID_VERSION;
	}
	
	ddc_offset = buf[2];
//	underscan_support = (buf[3] >> 7) & 0x01;
//	baseaudio_support = (buf[3] >> 6) & 0x01;
	pedid->ycbcr444 = (buf[3] >> 5) & 0x01;
	pedid->ycbcr422 = (buf[3] >> 4) & 0x01;
	native_dtd_num = buf[3] & 0x0F;
	
	// Parse data block
	while(cur_offset < ddc_offset)
	{
		tag = buf[cur_offset] >> 5;
		count = buf[cur_offset] & 0x1F;
		switch(tag)
		{
			case 0x02:	// Video Data Block
				HDMIDBG("[EDID-CEA] It is a Video Data Block.\n");
				hdmi_edid_get_cea_svd(buf + cur_offset, pedid);
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
					hdmi_edid_parse_3dinfo(buf + buf_offset);
				}
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
		hdmi_add_vic(hdmi_videomode_to_vic(vmode));
		ddc_offset += 18;
	}
	free(vmode);


	return E_HDMI_EDID_SUCCESS;
}

int hdmi_edid_parse_extensions(unsigned char *buf, struct hdmi_edid *pedid)
{
	int rc;
	
	if(buf == NULL || pedid == NULL)
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
    		hdmi_edid_parse_extensions_cea(buf, pedid);
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
