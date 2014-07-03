#include "rk3288_hdmi.h"
#include <asm/arch-rk32xx/io-rk3288.h>
#include <asm/arch-rk32xx/grf-rk3288.h>
#include <common.h>
#include <malloc.h>
#include <lcd.h>

#include <../board/rockchip/common/config.h>
#include <../board/rockchip/common/storage/storage.h>

#define PARTITION_NAME "baseparamer"
#define DEFAULT_MODE        11 
#define HDMI_SEL_LCDC(x)    ((((x)&1)<<4)|(1<<20))

struct HW_BASE_PARAMETER
{
    int xres;
    int yres;
    int width;
    int height;
    int refresh;
};

struct HW_BASE_PARAMETER base_paramer;

short g_hdmi_vic = -1;
struct hdmi_dev *hdmi = NULL;

static const struct hdmi_video_timing hdmi_mode [] = {
		//name			refresh		xres	yres	pixclock	h_bp	h_fp	v_bp	v_fp	h_pw	v_pw	polariry			                            PorI	flag	vic		2ndvic		               pixelrepeat	interface
	{ {	"720x480p@60Hz",	60,		720,	480,	27000000,	60,	    16,	    30,	    9,	    62,  	6,		0,				                                0,	    0	},	2,  	HDMI_720x480p_60HZ_16_9,	1,		    OUT_P888},
	{ {	"720x576p@50Hz",	50,		720,	576,	27000000,	68,	    12,	    39,	    5,	    64,	    5,		0,				                                0,	    0	},	17,  	HDMI_720x576p_50HZ_16_9,	1,		    OUT_P888},
	{ {	"1280x720p@24Hz",	24,		1280,	720,	59400000,	220,	1760,	20,	    5,	    40,	    5,	    FB_SYNC_HOR_HIGH_ACT | FB_SYNC_VERT_HIGH_ACT,	0,	    0	},	60,	    HDMI_1280x720p_24HZ_4_3,	1,		    OUT_P888},
	{ {	"1280x720p@25Hz",	25,		1280,	720,	74250000,	220,	2420,	20,	    5,	    40,	    5,	    FB_SYNC_HOR_HIGH_ACT | FB_SYNC_VERT_HIGH_ACT,	0,	    0	},	61,	    HDMI_1280x720p_25HZ_4_3,	1,		    OUT_P888},
	{ {	"1280x720p@30Hz",	30,		1280,	720,	74250000,	220,	1760,	20,	    5,	    40,	    5,	    FB_SYNC_HOR_HIGH_ACT | FB_SYNC_VERT_HIGH_ACT,	0,	    0	},	62,	    HDMI_1280x720p_30HZ_4_3,	1,		    OUT_P888},
	{ {	"1280x720p@50Hz",	50,		1280,	720,	74250000,	220,	440,	20,	    5,	    40,	    5,	    FB_SYNC_HOR_HIGH_ACT | FB_SYNC_VERT_HIGH_ACT,	0,	    0	},	19,  	HDMI_1280x720p_50HZ_4_3,	1,		    OUT_P888},
	{ {	"1280x720p@60Hz",	60,		1280,	720,	74250000,	220,	110,	20,	    5,	    40,	    5,	    FB_SYNC_HOR_HIGH_ACT | FB_SYNC_VERT_HIGH_ACT,	0,	    0	},	4,  	HDMI_1280x720p_60HZ_4_3,	1,		    OUT_P888},
	{ {	"1920x1080p@24Hz",	24,		1920,	1080,	74250000,	148,	638,	36,	    4,	    44,	    5,	    FB_SYNC_HOR_HIGH_ACT | FB_SYNC_VERT_HIGH_ACT,	0,	    0	},	32,	    HDMI_1920x1080p_24HZ_4_3,	1,		    OUT_P888},
	{ {	"1920x1080p@25Hz",	25,		1920,	1080,	74250000,	148,	528,	36,	    4,	    44,	    5,	    FB_SYNC_HOR_HIGH_ACT | FB_SYNC_VERT_HIGH_ACT,	0,	    0	},	33,	    HDMI_1920x1080p_25HZ_4_3,	1,		    OUT_P888},
	{ {	"1920x1080p@30Hz",	30,		1920,	1080,	74250000,	148,	88,	    36,	    4,	    44,	    5,	    FB_SYNC_HOR_HIGH_ACT | FB_SYNC_VERT_HIGH_ACT,	0,	    0	},	34,	    HDMI_1920x1080p_30HZ_4_3,	1,		    OUT_P888},
	{ {	"1920x1080p@50Hz",	50,		1920,	1080,	148500000,	148,	528,	36,	    4,	    44,	    5,	    FB_SYNC_HOR_HIGH_ACT | FB_SYNC_VERT_HIGH_ACT,	0,	    0	},	31,  	HDMI_1920x1080p_50HZ_4_3,	1,		    OUT_P888},
	{ {	"1920x1080p@60Hz",	60,		1920,	1080,	148500000,	148,	88,	    36,	    4,	    44,	    5,	    FB_SYNC_HOR_HIGH_ACT | FB_SYNC_VERT_HIGH_ACT,	0,	    0	},	16,  	HDMI_1920x1080p_60HZ_4_3,	1,		    OUT_P888},		
	{ {	"3840x2160p@24Hz",	24,		3840,	2160,	297000000,	296,	1276,	72,	    8,	    88,	    10,	    FB_SYNC_HOR_HIGH_ACT | FB_SYNC_VERT_HIGH_ACT,	0,	    0	},	93,	    HDMI_3840x2160p_24HZ_4_3,	1,		    OUT_P888},
	{ {	"3840x2160p@25Hz",	25,		3840,	2160,	297000000,	296,	1056,	72,	    8,	    88,	    10,	    FB_SYNC_HOR_HIGH_ACT | FB_SYNC_VERT_HIGH_ACT,	0,	    0	},	94,	    HDMI_3840x2160p_25HZ_4_3,	1,		    OUT_P888},
	{ {	"3840x2160p@30Hz", 	30,		3840,	2160,	297000000,	296,	176,	72,	    8,	    88,	    10,	    FB_SYNC_HOR_HIGH_ACT | FB_SYNC_VERT_HIGH_ACT,	0,	    0	},	95,	    HDMI_3840x2160p_30HZ_4_3,	1,		    OUT_P888},
	{ {	"3840x2160p@50Hz",	50,		3840,	2160,	594000000,	296,	1056,	72,	    8,	    88,	    10,	    FB_SYNC_HOR_HIGH_ACT | FB_SYNC_VERT_HIGH_ACT,	0,	    0	},	96,	    HDMI_3840x2160p_50HZ_4_3,	1,		    OUT_P888},
	{ {	"3840x2160p@60Hz",	60,		3840,	2160,	594000000,	296,	176,	72,	    8,	    88,	    10,	    FB_SYNC_HOR_HIGH_ACT | FB_SYNC_VERT_HIGH_ACT,	0,	    0	},	97,	    HDMI_3840x2160p_60HZ_4_3,	1,		    OUT_P888},

};

/*
 * return preset res position
 */
static int inline read_baseparamer_storage(void) 
{
	int i, ret = -1;
    const disk_partition_t* ptn;                                       
	char baseparamer_buf[8 * RK_BLK_SIZE];

#ifdef CONFIG_CMD_FASTBOOT                                             
    ptn = get_disk_partition(PARTITION_NAME);                            
#else                                                                  
    //TODO: find disk_partition_t in other way.                        
    ptn = NULL;                                                        
#endif                                                                 
    if (!ptn) {                                                        
        //printf("%s ptn not found.\n", PARTITION_NAME);                  
		goto err;
    }                                                                  

    if (StorageReadLba(ptn->start, baseparamer_buf, 8) < 0) {
		printf("%s: Failed Read baseparamer Partition data\n", __func__);
		goto err;
	}


    memcpy(&base_paramer.xres, &baseparamer_buf[0], sizeof(base_paramer.xres));
    memcpy(&base_paramer.yres, &baseparamer_buf[4], sizeof(base_paramer.yres));
    memcpy(&base_paramer.width, &baseparamer_buf[8], sizeof(base_paramer.width));
    memcpy(&base_paramer.height, &baseparamer_buf[12], sizeof(base_paramer.height));
    memcpy(&base_paramer.refresh, &baseparamer_buf[16], sizeof(base_paramer.refresh));

	for (i = 0; i < (sizeof(hdmi_mode) / sizeof(hdmi_mode[0])); i++) {
		if (base_paramer.xres == hdmi_mode[i].mode.xres &&
				base_paramer.yres == hdmi_mode[i].mode.yres &&
				   base_paramer.refresh == hdmi_mode[i].mode.refresh)
			break;
	}

	if (i != sizeof(hdmi_mode) / sizeof(hdmi_mode[0])) {
		printf("preset display resolution is %dx%d@%d\n", base_paramer.xres, base_paramer.yres, base_paramer.refresh);
		ret = i;
	}

err:
	return ret;
}

static const struct phy_mpll_config_tab PHY_MPLL_TABLE[] = {	
	//tmdsclk = (pixclk / ref_cntrl ) * (fbdiv2 * fbdiv1) / nctrl / tmdsmhl
	//opmode: 0:HDMI1.4 	1:HDMI2.0
//	|pixclock|pixrepet|colordepth|prepdiv|tmdsmhl|opmode|fbdiv2|fbdiv1|ref_cntrl|nctrl|propctrl|intctrl|gmpctrl|	
	{27000000,	0,	8,	0,	0,	0,	2,	3,	0,	3,	3,	0,	0},
	{27000000,	0,	10,	1,	0,	0,	5,	1,	0,	3,	3,	0,	0},
	{27000000,	0,	12,	2,	0,	0,	3,	3,	0,	3,	3,	0,	0},
	{27000000,	0,	16,	3,	0,	0,	2,	3,	0,	2,	5,	0,	1},
	{74250000, 	0,	8, 	0,	0,	0, 	1, 	3,	0,	2,	5,	0, 	1},
	{74250000,	0,	10,	1,	0,	0,	5,	0,	1,	1,	7,	0,	2},
	{74250000,	0,	12,	2,	0,	0,	1,	2,	0,	1,	7,	0,	2},
	{74250000,	0,	16,	3,	0,	0,	1,	3,	0,	1,	7,	0,	2},
	{148500000, 	0, 	8,  	0, 	0,	0,	1,	1,	0,	1,	0,	0,	3},
	{148500000,	0,	10,	1,	0,	0,	5,	0,	3,	0,	7,	0,	3},
	{148500000,	0,	12,	2,	0,	0,	1,	2,	1,	0,	7,	0,	3},
	{148500000,	0,	16,	3,	0,	0,	1,	1,	0,	0,	7,	0,	3},
	{297000000,	0, 	8,	0, 	0, 	0, 	1, 	0, 	0, 	0, 	0, 	0, 	3},
	{297000000,	0, 	10,	1, 	3, 	1, 	5, 	0, 	3, 	0, 	7, 	0, 	3},
	{297000000,	0, 	12,	2, 	3, 	1, 	1, 	2, 	2, 	0, 	7, 	0, 	3},
	{297000000, 	0, 	16,  	3, 	3, 	1, 	1, 	1, 	0, 	0, 	5, 	0, 	3},
	{594000000,	0, 	8, 	0, 	3, 	1, 	1, 	3, 	3, 	0, 	0, 	0, 	3},
};

static const struct phy_mpll_config_tab* get_phy_mpll_tab(unsigned int pixClock, char pixRepet, char colorDepth)
{
	int i;

	if(pixClock == 0)
		return NULL;
	HDMIDBG("%s pixClock %u pixRepet %d colorDepth %d\n", __FUNCTION__, pixClock, pixRepet, colorDepth);
	for(i = 0; i < ARRAY_SIZE(PHY_MPLL_TABLE); i++)
	{
		if((PHY_MPLL_TABLE[i].pix_clock == pixClock) && (PHY_MPLL_TABLE[i].pix_repet == pixRepet)
			&& (PHY_MPLL_TABLE[i].color_depth == colorDepth))
			return &PHY_MPLL_TABLE[i];
	}
	return NULL;
}

/**
 * hdmi_vic2timing: transverse vic mode to video timing
 * @vmode: vic to transverse
 * 
 */
const struct hdmi_video_timing* hdmi_vic2timing(int vic)
{
    int i;

    if(vic == 0)
        return NULL;

    for(i = 0; i < ARRAY_SIZE(hdmi_mode); i++)
    {
        if(hdmi_mode[i].vic == vic || hdmi_mode[i].vic_2nd == vic)
            return &(hdmi_mode[i]);
    }        
    return NULL;
} 

static void hdmi_init_panel(struct hdmi_video *vpara, vidinfo_t *panel)
{
	struct hdmi_video_timing *timing = NULL;
	const struct fb_videomode *mode = NULL;


	timing = (struct hdmi_video_timing *)hdmi_vic2timing(vpara->vic);
	if(timing == NULL) {
		printf("[%s] not found vic %d\n", __FUNCTION__, vpara->vic);
		return ;
	}
	mode = &(timing->mode);

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

//i2c master reset
static void rk3288_hdmi_i2cm_reset(struct hdmi_dev *hdmi_dev)
{
	hdmi_msk_reg(hdmi_dev, I2CM_SOFTRSTZ, m_I2CM_SOFTRST, v_I2CM_SOFTRST(0));
	udelay(100);
}

static int hdmi_dev_read_edid(struct hdmi_dev *hdmi_dev, int block, unsigned char *buff)
{
	int i = 0, n = 0, index = 0, ret = -1, trytime = 2;
	int offset = (block % 2) * 0x80;
	int interrupt = 0;

	HDMIDBG("[%s] block %d\n", __FUNCTION__, block);

	rk3288_hdmi_i2cm_reset(hdmi_dev);
	
	//Set DDC I2C CLK which devided from DDC_CLK to 100KHz.
	hdmi_writel(hdmi_dev, I2CM_SS_SCL_HCNT_0_ADDR, 0x7a);
	hdmi_writel(hdmi_dev, I2CM_SS_SCL_LCNT_0_ADDR, 0x8d);
	hdmi_msk_reg(hdmi_dev, I2CM_DIV, m_I2CM_FAST_STD_MODE, v_I2CM_FAST_STD_MODE(STANDARD_MODE));	//Set Standard Mode

	//Enable I2C interrupt for reading edid
	hdmi_writel(hdmi_dev, IH_MUTE_I2CM_STAT0, v_SCDC_READREQ_MUTE(0) | v_I2CM_DONE_MUTE(0) | v_I2CM_ERR_MUTE(0));
	hdmi_msk_reg(hdmi_dev, I2CM_INT, m_I2CM_DONE_MASK, v_I2CM_DONE_MASK(0));
	hdmi_msk_reg(hdmi_dev, I2CM_CTLINT, m_I2CM_NACK_MASK | m_I2CM_ARB_MASK, v_I2CM_NACK_MASK(0) | v_I2CM_ARB_MASK(0));

	hdmi_writel(hdmi_dev, I2CM_SLAVE, DDC_I2C_EDID_ADDR);
	hdmi_writel(hdmi_dev, I2CM_SEGADDR, DDC_I2C_SEG_ADDR);
	hdmi_writel(hdmi_dev, I2CM_SEGPTR, block / 2);
	while(trytime--) {
		for(n = 0; n < HDMI_EDID_BLOCK_SIZE / 8; n++) {
			hdmi_writel(hdmi_dev, I2CM_ADDRESS, offset + 8 * n);
			//enable extend sequential read operation
			if(block == 0)
				hdmi_msk_reg(hdmi_dev, I2CM_OPERATION, m_I2CM_RD8, v_I2CM_RD8(1));
			else
				hdmi_msk_reg(hdmi_dev, I2CM_OPERATION, m_I2CM_RD8_EXT, v_I2CM_RD8_EXT(1));

			i = 20;
			while(i--)
			{
				mdelay(1);
				interrupt = hdmi_readl(hdmi_dev, IH_I2CM_STAT0);
				if(interrupt)
					hdmi_writel(hdmi_dev, IH_I2CM_STAT0, interrupt);

				if(interrupt & (m_SCDC_READREQ | m_I2CM_DONE | m_I2CM_ERROR))
					break;
				mdelay(4);
			}

			if(interrupt & m_I2CM_DONE) {
				for(index = 0; index < 8; index++) {
					buff[8 * n + index] = hdmi_readl(hdmi_dev, I2CM_READ_BUFF0 + index);
				}

				if(n == HDMI_EDID_BLOCK_SIZE / 8 - 1) {
					ret = 0;
					HDMIDBG("[%s] edid read sucess\n", __FUNCTION__);

					#ifdef HDMIDEBUG
					for(index = 0; index < 128; index++) {
						printf("0x%02x ,", buff[index]);
						if( (index + 1) % 16 == 0)
							printf("\n");
					}
					#endif
					goto exit;
				}
				continue;
			} else if((interrupt & m_I2CM_ERROR) || (i == -1)) {
				printf("[%s] edid read error\n", __FUNCTION__);
				rk3288_hdmi_i2cm_reset(hdmi_dev);
				break;
			}
		}

		printf("[%s] edid try times %d\n", __FUNCTION__, trytime);
		mdelay(100);
	}

exit:
	//Disable I2C interrupt
	hdmi_msk_reg(hdmi_dev, IH_MUTE_I2CM_STAT0, m_I2CM_DONE_MUTE | m_I2CM_ERR_MUTE, v_I2CM_DONE_MUTE(1) | v_I2CM_ERR_MUTE(1));
	hdmi_msk_reg(hdmi_dev, I2CM_INT, m_I2CM_DONE_MASK, v_I2CM_DONE_MASK(1));
	hdmi_msk_reg(hdmi_dev, I2CM_CTLINT, m_I2CM_NACK_MASK | m_I2CM_ARB_MASK, v_I2CM_NACK_MASK(1) | v_I2CM_ARB_MASK(1));
	return ret;
}

static int hdmi_parse_edid(struct hdmi_dev *hdmi_dev)
{
	struct hdmi_edid pedid;
	unsigned char buf[HDMI_EDID_BLOCK_SIZE];
	int rc = HDMI_ERROR_SUCESS, extendblock = 0, i, trytimes;
	for(trytimes = 0; trytimes < 3; trytimes++) {
		memset(buf, 0 , HDMI_EDID_BLOCK_SIZE);
		if (hdmi_dev_read_edid(hdmi_dev, 0, buf) == 0) {
			rc = hdmi_edid_parse_base(buf, &extendblock, &pedid);
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
		if (hdmi_dev_read_edid(hdmi_dev, i, buf) == 0) {
			rc = hdmi_edid_parse_extensions(buf, &pedid);
			if (rc) {
				printf("[HDMI] parse edid block %d error-%d\n", i, rc);
				continue;
			}
		}else {
			printf("[HDMI] read edid block %d error\n", i);
			goto err;
		}
	}
#if 0
	for (i = 0; i < hdmi_dev->vic_pos; i++)
		printf("%s\n",  hdmi_vic2timing(hdmi_dev->vicdb[i])->mode.name);
#endif
	return 0;

err:
	return -1;
}

static void hdmi_find_best_mode(struct hdmi_dev *hdmi_dev)
{
	int i, pos;

	pos = read_baseparamer_storage();
	if (pos < 0)
		pos = hdmi_dev->mode_len;
	else
		pos += 1;

	while (pos--) {
		for (i = 0; i < hdmi_dev->vic_pos; i++) {
			if (hdmi_dev->vicdb[i] == hdmi->modedb[pos].vic) {
				break;
			}
		}

		if (i != hdmi_dev->vic_pos) {
			hdmi_dev->video.vic = hdmi_dev->modedb[pos].vic;
			break;
		}
	}

	printf("%s[%d]: pos = %d vic = %d\n", __func__, hdmi_dev->mode_len, pos, hdmi_dev->video.vic);
}

static void rk3288_hdmi_powerdown(struct hdmi_dev* hdmi_dev)
{
    hdmi_msk_reg(hdmi_dev, PHY_CONF0, m_PDDQ_SIG | m_TXPWRON_SIG | m_ENHPD_RXSENSE_SIG,
        v_PDDQ_SIG(1) | v_TXPWRON_SIG(0) | v_ENHPD_RXSENSE_SIG(1));
    hdmi_writel(hdmi_dev, MC_CLKDIS, 0x7f);
}

static int rk3288_hdmi_write_phy(struct hdmi_dev *hdmi_dev, int reg_addr, int val)
{
	int trytime = 2, i = 0, op_status = 0;

	while(trytime--) {
		hdmi_writel(hdmi_dev, PHY_I2CM_ADDRESS, reg_addr);
		hdmi_writel(hdmi_dev, PHY_I2CM_DATAO_1, (val >> 8) & 0xff);
		hdmi_writel(hdmi_dev, PHY_I2CM_DATAO_0, val & 0xff);
		hdmi_writel(hdmi_dev, PHY_I2CM_OPERATION, m_PHY_I2CM_WRITE);

		i = 20;
		while(i--) {
			mdelay(1);
			op_status = hdmi_readl(hdmi_dev, IH_I2CMPHY_STAT0);
			if(op_status)
				hdmi_writel(hdmi_dev, IH_I2CMPHY_STAT0, op_status);

			if(op_status & (m_I2CMPHY_DONE | m_I2CMPHY_ERR)) {
				break;
			}
			mdelay(4);
		}

		if(op_status & m_I2CMPHY_DONE) {
			return 0;
		}
		else {
			printf("[%s] operation error,trytime=%d\n", __FUNCTION__, trytime);
		}
		mdelay(100);
	}

	return -1;
}

static int hdmi_dev_detect_hotplug(struct hdmi_dev *hdmi_dev)
{
	u32 value = hdmi_readl(hdmi_dev, PHY_STAT0);

	if(value & m_PHY_HPD)
		return HDMI_HPD_ACTIVED;
	else
		return HDMI_HPD_REMOVED;
}

static void hdmi_dev_init(struct hdmi_dev *hdmi_dev)
{
	int val;

	//i2c_hdmi_sda GRF_GPIO7CL_IOMUX[13:12]=10
	val = (0x2 << 12) | ((0x3 << 12) << 16);
	grf_writel(val, GRF_GPIO7CL_IOMUX);
	//i2c_hdmi_scl GRF_GPIO7CH_IOMUX[1:0]=10
	val = (0x2 << 0) | ((0x3 << 0) << 16);
	grf_writel(val, GRF_GPIO7CH_IOMUX); 

	//lcdc source select
	//grf_writel(HDMI_SEL_LCDC(0), GRF_SOC_CON6);	

	// reset hdmi
	writel((1 << 9) | (1 << 25), RKIO_CRU_PHYS + 0x01d4);
	udelay(1);
	writel((0 << 9) | (1 << 25), RKIO_CRU_PHYS + 0x01d4);

	rk3288_hdmi_powerdown(hdmi_dev);

	//mute unnecessary interrrupt, only enable hpd
	hdmi_writel(hdmi_dev, IH_MUTE_FC_STAT0, 0xff);
	hdmi_writel(hdmi_dev, IH_MUTE_FC_STAT1, 0xff);
	hdmi_writel(hdmi_dev, IH_MUTE_FC_STAT2, 0xff);
	hdmi_writel(hdmi_dev, IH_MUTE_AS_STAT0, 0xff);
	hdmi_writel(hdmi_dev, IH_MUTE_PHY_STAT0, 0xfe);
	hdmi_writel(hdmi_dev, IH_MUTE_I2CM_STAT0, 0xff);
	hdmi_writel(hdmi_dev, IH_MUTE_CEC_STAT0, 0xff);
	hdmi_writel(hdmi_dev, IH_MUTE_VP_STAT0, 0xff);
	hdmi_writel(hdmi_dev, IH_MUTE_I2CMPHY_STAT0, 0xff);
	hdmi_writel(hdmi_dev, IH_MUTE_AHBDMAAUD_STAT0, 0xff);

	hdmi_writel(hdmi_dev, PHY_MASK, 0xf1);

	//Force output black
	hdmi_writel(hdmi_dev, FC_DBGTMDS2, 0x00);   /*R*/
	hdmi_writel(hdmi_dev, FC_DBGTMDS1, 0x00);   /*G*/
	hdmi_writel(hdmi_dev, FC_DBGTMDS0, 0x00);   /*B*/
}
static int rk3288_hdmi_video_frameComposer(struct hdmi_dev *hdmi_dev, struct hdmi_video *vpara)	//TODO Daisen wait to add support 3D
{
	int value, vsync_pol, hsync_pol, de_pol;
	struct hdmi_video_timing *timing = NULL;
	struct fb_videomode *mode = NULL;
	
	vsync_pol = 1;
	hsync_pol = 1;
	de_pol = 1;
	
	timing = (struct hdmi_video_timing *)hdmi_vic2timing(vpara->vic);
	if(timing == NULL) {
		printf("[%s] not found vic %d\n", __FUNCTION__, vpara->vic);
		return -1;
	}
	mode = &(timing->mode);
	switch(vpara->color_output_depth) {
		case 8:
			hdmi_dev->tmdsclk = mode->pixclock;
			break;
		case 10:
			hdmi_dev->tmdsclk = mode->pixclock * 10 / 8;
			break;
		case 12:
			hdmi_dev->tmdsclk = mode->pixclock * 12 / 8;
			break;
		case 16:
			hdmi_dev->tmdsclk = mode->pixclock * 2;
			break;
		default:
			hdmi_dev->tmdsclk = mode->pixclock;
			break;
	}
	// Now we limit to hdmi 1.4b standard.
	if(mode->pixclock <= 340000000 && hdmi_dev->tmdsclk > 340000000)
	{
		vpara->color_output_depth = 8;
		hdmi_dev->tmdsclk = mode->pixclock;
	}
	hdmi_dev->pixelclk = mode->pixclock;
	hdmi_dev->pixelrepeat = timing->pixelrepeat;
	hdmi_dev->colordepth = vpara->color_output_depth;

	hdmi_msk_reg(hdmi_dev, A_HDCPCFG0, m_ENCRYPT_BYPASS | m_HDMI_DVI,
		v_ENCRYPT_BYPASS(1) | v_HDMI_DVI(vpara->sink_hdmi));	//cfg to bypass hdcp data encrypt
	hdmi_msk_reg(hdmi_dev, FC_INVIDCONF, m_FC_VSYNC_POL | m_FC_HSYNC_POL | m_FC_DE_POL | m_FC_HDMI_DVI | m_FC_INTERLACE_MODE,
		v_FC_VSYNC_POL(vsync_pol) | v_FC_HSYNC_POL(hsync_pol) | v_FC_DE_POL(de_pol) | v_FC_HDMI_DVI(vpara->sink_hdmi) | v_FC_INTERLACE_MODE(mode->vmode));

	hdmi_msk_reg(hdmi_dev, FC_INVIDCONF, m_FC_VBLANK, v_FC_VBLANK(0));

	value = mode->xres;
	hdmi_writel(hdmi_dev, FC_INHACTIV1, v_FC_HACTIVE1(value >> 8));
	hdmi_writel(hdmi_dev, FC_INHACTIV0, (value & 0xff));

	value = mode->yres;
	hdmi_writel(hdmi_dev, FC_INVACTIV1, v_FC_VACTIVE1(value >> 8));
	hdmi_writel(hdmi_dev, FC_INVACTIV0, (value & 0xff));

	value = mode->hsync_len + mode->left_margin + mode->right_margin;
	hdmi_writel(hdmi_dev, FC_INHBLANK1, v_FC_HBLANK1(value >> 8));
	hdmi_writel(hdmi_dev, FC_INHBLANK0, (value & 0xff));

	value = mode->vsync_len + mode->upper_margin + mode->lower_margin;
	hdmi_writel(hdmi_dev, FC_INVBLANK, (value & 0xff));

	value = mode->right_margin;
	hdmi_writel(hdmi_dev, FC_HSYNCINDELAY1, v_FC_HSYNCINDEAY1(value >> 8));
	hdmi_writel(hdmi_dev, FC_HSYNCINDELAY0, (value & 0xff));

	value = mode->lower_margin;
	hdmi_writel(hdmi_dev, FC_VSYNCINDELAY, (value & 0xff));

	value = mode->hsync_len;
	hdmi_writel(hdmi_dev, FC_HSYNCINWIDTH1, v_FC_HSYNCWIDTH1(value >> 8));
	hdmi_writel(hdmi_dev, FC_HSYNCINWIDTH0, (value & 0xff));

	value = mode->vsync_len;
	hdmi_writel(hdmi_dev, FC_VSYNCINWIDTH, (value & 0xff));

	/*Set the control period minimum duration(min. of 12 pixel clock cycles, refer to HDMI 1.4b specification)*/
	hdmi_writel(hdmi_dev, FC_CTRLDUR, 12);
	hdmi_writel(hdmi_dev, FC_EXCTRLDUR, 32);
	

	hdmi_writel(hdmi_dev, FC_PRCONF, v_FC_PR_FACTOR(timing->pixelrepeat));
	
	return 0;
}

static int rk3288_hdmi_video_packetizer(struct hdmi_dev *hdmi_dev, struct hdmi_video *vpara)
{
	unsigned char color_depth = 0;
	unsigned char output_select = 0;
	unsigned char remap_size = 0;

	if(vpara->color_output == HDMI_COLOR_YCbCr422) {
		switch (vpara->color_output_depth) {
			case 8:
				remap_size = YCC422_16BIT;
				break;
			case 10:
				remap_size = YCC422_20BIT;
				break;
			case 12:
				remap_size = YCC422_24BIT;
				break;
			default:
				remap_size = YCC422_16BIT;
				break;
		}

		output_select = OUT_FROM_YCC422_REMAP;
		/*Config remap size for the different color Depth*/
		hdmi_msk_reg(hdmi_dev, VP_REMAP, m_YCC422_SIZE, v_YCC422_SIZE(remap_size));
	} else {
		switch (vpara->color_output_depth) {
			case 10:
				color_depth = COLOR_DEPTH_30BIT;
				output_select = OUT_FROM_PIXEL_PACKING;
				break;
			case 12:
				color_depth = COLOR_DEPTH_36BIT;
				output_select = OUT_FROM_PIXEL_PACKING;
				break;
			case 16:
				color_depth = COLOR_DEPTH_48BIT;
				output_select = OUT_FROM_PIXEL_PACKING;
				break;
			case 8:
			default:
				color_depth = COLOR_DEPTH_24BIT;
				output_select = OUT_FROM_8BIT_BYPASS;
				break;
		}

		/*Config Color Depth*/
		hdmi_msk_reg(hdmi_dev, VP_PR_CD, m_COLOR_DEPTH, v_COLOR_DEPTH(color_depth));			
	}
	
	/*Config pixel repettion*/
	hdmi_msk_reg(hdmi_dev, VP_PR_CD, m_DESIRED_PR_FACTOR, v_DESIRED_PR_FACTOR(hdmi_dev->pixelrepeat - 1));
	if (hdmi_dev->pixelrepeat > 1) {
		hdmi_msk_reg(hdmi_dev, VP_CONF, m_PIXEL_REPET_EN | m_BYPASS_SEL, v_PIXEL_REPET_EN(1) | v_BYPASS_SEL(0));
	}
	else
		hdmi_msk_reg(hdmi_dev, VP_CONF, m_PIXEL_REPET_EN | m_BYPASS_SEL, v_PIXEL_REPET_EN(0) | v_BYPASS_SEL(1));
	
	/*config output select*/
	if (output_select == OUT_FROM_PIXEL_PACKING) { /* pixel packing */
		hdmi_msk_reg(hdmi_dev, VP_CONF, m_BYPASS_EN | m_PIXEL_PACK_EN | m_YCC422_EN | m_OUTPUT_SEL,
			v_BYPASS_EN(0) | v_PIXEL_PACK_EN(1) | v_YCC422_EN(0) | v_OUTPUT_SEL(output_select));
	} else if (output_select == OUT_FROM_YCC422_REMAP) { /* YCC422 */
		hdmi_msk_reg(hdmi_dev, VP_CONF, m_BYPASS_EN | m_PIXEL_PACK_EN | m_YCC422_EN | m_OUTPUT_SEL,
			v_BYPASS_EN(0) | v_PIXEL_PACK_EN(0) | v_YCC422_EN(1) | v_OUTPUT_SEL(output_select));
	} else if (output_select == OUT_FROM_8BIT_BYPASS || output_select == 3) { /* bypass */
		hdmi_msk_reg(hdmi_dev, VP_CONF, m_BYPASS_EN | m_PIXEL_PACK_EN | m_YCC422_EN | m_OUTPUT_SEL,
			v_BYPASS_EN(1) | v_PIXEL_PACK_EN(0) | v_YCC422_EN(0) | v_OUTPUT_SEL(output_select));
	}

#if defined(HDMI_VIDEO_STUFFING)
	/* YCC422 and pixel packing stuffing*/
	hdmi_msk_reg(hdmi_dev, VP_STUFF, m_PR_STUFFING, v_PR_STUFFING(1));
	hdmi_msk_reg(hdmi_dev, VP_STUFF, m_YCC422_STUFFING | m_PP_STUFFING, v_YCC422_STUFFING(1) | v_PP_STUFFING(1));
#endif
	return 0;
}

static int rk3288_hdmi_video_sampler(struct hdmi_dev *hdmi_dev, struct hdmi_video *vpara)
{
	int map_code = 0;
	
	if (vpara->color_input == HDMI_COLOR_YCbCr422) {
		/* YCC422 mapping is discontinued - only map 1 is supported */
		switch (vpara->color_output_depth) {
		case 8:
			map_code = VIDEO_YCBCR422_8BIT;
			break;
		case 10:
			map_code = VIDEO_YCBCR422_10BIT;
			break;
		case 12:
			map_code = VIDEO_YCBCR422_12BIT;
			break;
		default:
			map_code = VIDEO_YCBCR422_8BIT;
			break;
		}
	} else {
		switch (vpara->color_output_depth) {
		case 10:
			map_code = VIDEO_RGB444_10BIT;
			break;
		case 12:
			map_code = VIDEO_RGB444_12BIT;
			break;
		case 16:
			map_code = VIDEO_RGB444_16BIT;
			break;
		case 8:
		default:
			map_code = VIDEO_RGB444_8BIT;
			break;
		}
		map_code += (vpara->color_input == HDMI_COLOR_YCbCr444) ? 8 : 0;
	}

	//Set Data enable signal from external and set video sample input mapping
	hdmi_msk_reg(hdmi_dev, TX_INVID0, m_INTERNAL_DE_GEN | m_VIDEO_MAPPING, v_INTERNAL_DE_GEN(0) | v_VIDEO_MAPPING(map_code));

#if defined(HDMI_VIDEO_STUFFING)
	hdmi_writel(hdmi_dev, TX_GYDATA0, 0x00);
	hdmi_writel(hdmi_dev, TX_GYDATA1, 0x00);
	hdmi_msk_reg(hdmi_dev, TX_INSTUFFING, m_GYDATA_STUFF, v_GYDATA_STUFF(1));
	hdmi_writel(hdmi_dev, TX_RCRDATA0, 0x00);
	hdmi_writel(hdmi_dev, TX_RCRDATA1, 0x00);
	hdmi_msk_reg(hdmi_dev, TX_INSTUFFING, m_RCRDATA_STUFF, v_RCRDATA_STUFF(1));
	hdmi_writel(hdmi_dev, TX_BCBDATA0, 0x00);
	hdmi_writel(hdmi_dev, TX_BCBDATA1, 0x00);
	hdmi_msk_reg(hdmi_dev, TX_INSTUFFING, m_BCBDATA_STUFF, v_BCBDATA_STUFF(1));
#endif
	return 0;
}

static void hdmi_dev_config_avi(struct hdmi_dev *hdmi_dev, struct hdmi_video *vpara)
{
	unsigned char colorimetry, ext_colorimetry = 0, aspect_ratio, y1y0;
	unsigned char rgb_quan_range = AVI_QUANTIZATION_RANGE_DEFAULT;

	//Set AVI infoFrame Data byte1
	if(vpara->color_output == HDMI_COLOR_YCbCr444)
		y1y0 = AVI_COLOR_MODE_YCBCR444;
	else if(vpara->color_output == HDMI_COLOR_YCbCr422)
		y1y0 = AVI_COLOR_MODE_YCBCR422;
	else if(vpara->color_output == HDMI_COLOR_YCbCr420)
		y1y0 = AVI_COLOR_MODE_YCBCR420;
	else
		y1y0 = AVI_COLOR_MODE_RGB;

	hdmi_msk_reg(hdmi_dev, FC_AVICONF0, m_FC_ACTIV_FORMAT | m_FC_RGC_YCC, v_FC_RGC_YCC(y1y0) | v_FC_ACTIV_FORMAT(1));

	//Set AVI infoFrame Data byte2
	switch(vpara->vic)
	{
		case HDMI_720x480i_60HZ_4_3:
		case HDMI_720x576i_50HZ_4_3:
		case HDMI_720x480p_60HZ_4_3:
		case HDMI_720x576p_50HZ_4_3:
			aspect_ratio = AVI_CODED_FRAME_ASPECT_4_3;
			colorimetry = AVI_COLORIMETRY_SMPTE_170M;
			break;
		case HDMI_720x480i_60HZ_16_9:
		case HDMI_720x576i_50HZ_16_9:
		case HDMI_720x480p_60HZ_16_9:
		case HDMI_720x576p_50HZ_16_9:
			aspect_ratio = AVI_CODED_FRAME_ASPECT_16_9;
			colorimetry = AVI_COLORIMETRY_SMPTE_170M;
			break;
		default:
			aspect_ratio = AVI_CODED_FRAME_ASPECT_16_9;
			colorimetry = AVI_COLORIMETRY_ITU709;
	}

	if(vpara->color_output_depth > 8) {
		colorimetry = AVI_COLORIMETRY_EXTENDED;
		ext_colorimetry = 6;
	}
	else if(vpara->color_output == HDMI_COLOR_RGB_16_235 || vpara->color_output == HDMI_COLOR_RGB_0_255) {
		colorimetry = AVI_COLORIMETRY_NO_DATA;
		ext_colorimetry = 0;
	}

	hdmi_writel(hdmi_dev, FC_AVICONF1, v_FC_COLORIMETRY(colorimetry) | v_FC_PIC_ASPEC_RATIO(aspect_ratio) | v_FC_ACT_ASPEC_RATIO(ACTIVE_ASPECT_RATE_SAME_AS_CODED_FRAME));

	//Set AVI infoFrame Data byte3
	hdmi_msk_reg(hdmi_dev, FC_AVICONF2, m_FC_EXT_COLORIMETRY | m_FC_QUAN_RANGE,
		v_FC_EXT_COLORIMETRY(ext_colorimetry) | v_FC_QUAN_RANGE(rgb_quan_range));

	//Set AVI infoFrame Data byte4
	hdmi_writel(hdmi_dev, FC_AVIVID, vpara->vic & 0xff);

	//Set AVI infoFrame Data byte5
	hdmi_msk_reg(hdmi_dev, FC_AVICONF3, m_FC_YQ | m_FC_CN, v_FC_YQ(YQ_LIMITED_RANGE) | v_FC_CN(CN_GRAPHICS));
}

static int hdmi_dev_config_vsi(struct hdmi_dev *hdmi_dev, unsigned char vic_3d, unsigned char format)
{
	int i = 0, id = 0x000c03;
	unsigned char data[3] = {0};


	HDMIDBG("[%s] vic %d format %d.\n", __FUNCTION__, vic_3d, format);
        
	hdmi_msk_reg(hdmi_dev, FC_DATAUTO0, m_VSD_AUTO, v_VSD_AUTO(0));
	hdmi_writel(hdmi_dev, FC_VSDIEEEID0, id & 0xff);
	hdmi_writel(hdmi_dev, FC_VSDIEEEID1, (id >> 8) & 0xff);
	hdmi_writel(hdmi_dev, FC_VSDIEEEID2, (id >> 16) & 0xff);

	data[0] = format << 5;	//PB4 --HDMI_Video_Format
	switch(format)
        {
                case HDMI_VIDEO_FORMAT_4Kx2K:
                        data[1] = vic_3d;	//PB5--HDMI_VIC
                        data[2] = 0;
                        break;
                case HDMI_VIDEO_FORMAT_3D:
			data[1] = vic_3d << 4;	//PB5--3D_Structure field
			data[2] = 0;		//PB6--3D_Ext_Data field
			break;
		default:
			data[1] = 0;
			data[2] = 0;
			break;
        }

	for (i = 0; i < 3; i++) {
		hdmi_writel(hdmi_dev, FC_VSDPAYLOAD0 + i, data[i]);
	}

//	if (auto_send) {
		hdmi_msk_reg(hdmi_dev, FC_DATAUTO0, m_VSD_AUTO, v_VSD_AUTO(1));
//	}
//	else {
//		hdmi_msk_reg(hdmi_dev, FC_DATMAN, m_VSD_MAN, v_VSD_MAN(1));
//	}

	return 0;
}

static int rk3288_hdmi_config_phy(struct hdmi_dev *hdmi_dev)
{
	int stat = 0, i = 0;
	const struct phy_mpll_config_tab *phy_mpll = NULL;
	hdmi_msk_reg(hdmi_dev, PHY_I2CM_DIV, m_PHY_I2CM_FAST_STD, v_PHY_I2CM_FAST_STD(0));

	//power on PHY
	//hdmi_writel(hdmi_dev, PHY_CONF0, 0x1e);
	hdmi_msk_reg(hdmi_dev, PHY_CONF0, m_PDDQ_SIG | m_TXPWRON_SIG, v_PDDQ_SIG(1) | v_TXPWRON_SIG(0));

	//reset PHY
	hdmi_writel(hdmi_dev, MC_PHYRSTZ, v_PHY_RSTZ(1));
	mdelay(5);
	hdmi_writel(hdmi_dev, MC_PHYRSTZ, v_PHY_RSTZ(0));

	//Set slave address as PHY GEN2 address
	hdmi_writel(hdmi_dev, PHY_I2CM_SLAVE, PHY_GEN2_ADDR);

	//config the required PHY I2C register
	phy_mpll = get_phy_mpll_tab(hdmi_dev->pixelclk, hdmi_dev->pixelrepeat - 1, hdmi_dev->colordepth);
	if(phy_mpll) {
		rk3288_hdmi_write_phy(hdmi_dev, PHYTX_OPMODE_PLLCFG, v_PREP_DIV(phy_mpll->prep_div) | v_TMDS_CNTRL(phy_mpll->tmdsmhl_cntrl) | v_OPMODE(phy_mpll->opmode) |
			v_FBDIV2_CNTRL(phy_mpll->fbdiv2_cntrl) | v_FBDIV1_CNTRL(phy_mpll->fbdiv1_cntrl) | v_REF_CNTRL(phy_mpll->ref_cntrl) | v_MPLL_N_CNTRL(phy_mpll->n_cntrl));
		rk3288_hdmi_write_phy(hdmi_dev, PHYTX_PLLCURRCTRL, v_MPLL_PROP_CNTRL(phy_mpll->prop_cntrl) | v_MPLL_INT_CNTRL(phy_mpll->int_cntrl));
		rk3288_hdmi_write_phy(hdmi_dev, PHYTX_PLLGMPCTRL, v_MPLL_GMP_CNTRL(phy_mpll->gmp_cntrl));
	}
	if(hdmi_dev->pixelclk <= 74250000) {
		rk3288_hdmi_write_phy(hdmi_dev, PHYTX_CLKSYMCTRL, v_OVERRIDE(1) | v_SLOPEBOOST(0)
			| v_TX_SYMON(1) | v_TX_TRAON(0) | v_TX_TRBON(0) | v_CLK_SYMON(1));
		rk3288_hdmi_write_phy(hdmi_dev, PHYTX_TERM_RESIS, v_TX_TERM(R100_Ohms));
	}
	else if(hdmi_dev->pixelclk <= 148500000) {
		rk3288_hdmi_write_phy(hdmi_dev, PHYTX_CLKSYMCTRL, v_OVERRIDE(1) | v_SLOPEBOOST(2)
			| v_TX_SYMON(1) | v_TX_TRAON(0) | v_TX_TRBON(0) | v_CLK_SYMON(1));
		rk3288_hdmi_write_phy(hdmi_dev, PHYTX_TERM_RESIS, v_TX_TERM(R100_Ohms));
	}
	else if(hdmi_dev->pixelclk <= 297000000) {
		rk3288_hdmi_write_phy(hdmi_dev, PHYTX_CLKSYMCTRL, v_OVERRIDE(1) | v_SLOPEBOOST(2)
			| v_TX_SYMON(1) | v_TX_TRAON(0) | v_TX_TRBON(0) | v_CLK_SYMON(1));
		rk3288_hdmi_write_phy(hdmi_dev, PHYTX_TERM_RESIS, v_TX_TERM(R100_Ohms));
	}
	else if(hdmi_dev->pixelclk > 297000000) {
		//TODO Daisen wait to add and modify
		rk3288_hdmi_write_phy(hdmi_dev, PHYTX_CLKSYMCTRL, v_OVERRIDE(1) | v_SLOPEBOOST(3)
			| v_TX_SYMON(1) | v_TX_TRAON(0) | v_TX_TRBON(1) | v_CLK_SYMON(1));
		rk3288_hdmi_write_phy(hdmi_dev, PHYTX_TERM_RESIS, v_TX_TERM(R100_Ohms));
	}
	if(hdmi_dev->pixelclk < 297000000)
		rk3288_hdmi_write_phy(hdmi_dev, PHYTX_VLEVCTRL, v_SUP_TXLVL(18) | v_SUP_CLKLVL(17));
	else
		rk3288_hdmi_write_phy(hdmi_dev, PHYTX_VLEVCTRL, v_SUP_TXLVL(14) | v_SUP_CLKLVL(13));

	rk3288_hdmi_write_phy(hdmi_dev, 0x05, 0x8000);

	//power on PHY
	hdmi_writel(hdmi_dev, PHY_CONF0, 0x2e);
	//hdmi_msk_reg(hdmi_dev, PHY_CONF0, m_PDDQ_SIG | m_TXPWRON_SIG | m_ENHPD_RXSENSE_SIG,
		//v_PDDQ_SIG(0) | v_TXPWRON_SIG(1) | v_ENHPD_RXSENSE_SIG(1));

	//check if the PHY PLL is locked
	#define PHY_TIMEOUT	10000
	while(i++ < PHY_TIMEOUT) {
		if ((i % 100) == 0) {
			stat = hdmi_readl(hdmi_dev, PHY_STAT0);
			if(stat & m_PHY_LOCK) {
				//hdmi_writel(hdmi_dev, PHY_STAT0, v_PHY_LOCK(1));
				break;
			}
		}
	}
	if((stat & m_PHY_LOCK) == 0) {
		stat = hdmi_readl(hdmi_dev, MC_LOCKONCLOCK);
		printf("PHY PLL not locked: PCLK_ON=%d,TMDSCLK_ON=%d\n", (stat & m_PCLK_ON) >> 6, (stat & m_TMDSCLK_ON) >> 5);
		return -1;
	}

	return 0;
}

static int hdmi_dev_config_video(struct hdmi_dev *hdmi_dev, struct hdmi_video *vpara)
{
	HDMIDBG("%s vic %d 3dformat %d\n", __FUNCTION__, vpara->vic, vpara->format_3d);
		
	// force output blue
	hdmi_msk_reg(hdmi_dev, FC_DBGFORCE, m_FC_FORCEVIDEO, v_FC_FORCEVIDEO(1));
	
	if (rk3288_hdmi_video_frameComposer(hdmi_dev, vpara) < 0)
		return -1;
	if (rk3288_hdmi_video_packetizer(hdmi_dev, vpara) < 0)
		return -1;
	//Color space convert
	//if (rk3288_hdmi_video_csc(hdmi_dev, vpara) < 0)
	//	return -1;
	if (rk3288_hdmi_video_sampler(hdmi_dev, vpara) < 0)
		return -1;

	if (vpara->sink_hdmi == OUTPUT_HDMI) {
		hdmi_dev_config_avi(hdmi_dev, vpara);
		if ( vpara->format_3d != HDMI_3D_NONE)
			hdmi_dev_config_vsi(hdmi_dev, vpara->format_3d, HDMI_VIDEO_FORMAT_3D);
		#ifndef HDMI_VERSION_2
		else if ((vpara->vic > 92 && vpara->vic < 96) || (vpara->vic == 98)) {
			vpara->vic = (vpara->vic == 98) ? 4 : (96 - vpara->vic);
			hdmi_dev_config_vsi(hdmi_dev, vpara->vic, HDMI_VIDEO_FORMAT_4Kx2K);
		}
		#endif
		else
			hdmi_dev_config_vsi(hdmi_dev, vpara->vic, HDMI_VIDEO_FORMAT_NORMAL);
		printf("[HDMI] sucess output HDMI.\n");
	}else {
		printf("[HDMI] sucess output DVI.\n");
	}
	
	rk3288_hdmi_config_phy(hdmi_dev);
	return 0;
}

static int hdmi_dev_control_output(struct hdmi_dev *hdmi_dev, int enable)
{
	if(enable == HDMI_AV_UNMUTE) {
		hdmi_msk_reg(hdmi_dev, AUD_CONF0, m_SW_AUD_FIFO_RST, v_SW_AUD_FIFO_RST(1));
		hdmi_writel(hdmi_dev, MC_SWRSTZREQ, 0xF7);
		//unmute audio
		hdmi_msk_reg(hdmi_dev, FC_AUDSCONF, m_AUD_PACK_SAMPFIT, v_AUD_PACK_SAMPFIT(0));
		hdmi_writel(hdmi_dev, FC_DBGFORCE, 0x00);
	} else {
		if(enable & HDMI_VIDEO_MUTE) {
			hdmi_msk_reg(hdmi_dev, FC_DBGFORCE, m_FC_FORCEVIDEO, v_FC_FORCEVIDEO(1));
		}
		if(enable & HDMI_AUDIO_MUTE) {
			hdmi_msk_reg(hdmi_dev, FC_AUDSCONF, m_AUD_PACK_SAMPFIT, v_AUD_PACK_SAMPFIT(0x0F));
		}
	}

	return 0;
}

static int hdmi_dev_insert(struct hdmi_dev *hdmi_dev)
{
	HDMIDBG("%s\n", __FUNCTION__);
    hdmi_writel(hdmi_dev, MC_CLKDIS, 0x0);
    //mute audio
	hdmi_msk_reg(hdmi_dev, FC_AUDSCONF, m_AUD_PACK_SAMPFIT, v_AUD_PACK_SAMPFIT(0x0F));

    return HDMI_ERROR_SUCESS;
}


void rk3288_hdmi_init(vidinfo_t *panel)
{
	hdmi = malloc(sizeof (struct hdmi_dev));
	if (hdmi != NULL) {
		memset(hdmi, 0, sizeof (hdmi));
		//init vicdb
		hdmi->regbase = (void *)RKIO_HDMI_PHYS;
		hdmi->vic_pos = 0;
		//init modedb
		hdmi->modedb = hdmi_mode;
		hdmi->mode_len = sizeof(hdmi_mode) / sizeof(hdmi_mode[0]);

		hdmi->video.vic = hdmi->modedb[DEFAULT_MODE].vic;

		hdmi->video.color_input = HDMI_COLOR_RGB_0_255;
		hdmi->video.sink_hdmi = OUTPUT_HDMI;
		hdmi->video.format_3d = HDMI_3D_NONE;

		hdmi->video.color_output = HDMI_COLOR_RGB_16_235;
		hdmi->video.color_output_depth = 10;
		
		hdmi_dev_init(hdmi);	
		if (hdmi_dev_detect_hotplug(hdmi)) {
			hdmi_dev_insert(hdmi);
			hdmi_parse_edid(hdmi);
			hdmi_find_best_mode(hdmi);
			g_hdmi_vic = hdmi->video.vic;
			//config lcdc panel
			hdmi_init_panel(&hdmi->video, panel);
			hdmi_dev_config_video(hdmi, &hdmi->video);
			hdmi_dev_control_output(hdmi, HDMI_AV_UNMUTE);
		}else {
			printf("Hdmi Devices Not Exist.\n");
		}
	}

	free(hdmi);
}


