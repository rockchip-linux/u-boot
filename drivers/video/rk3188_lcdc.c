/********************************************************************************
		COPYRIGHT (c)   2013 BY ROCK-CHIP FUZHOU
			--  ALL RIGHTS RESERVED  --
File Name:	
Author:         
Created:        
Modified:
Revision:       1.00
********************************************************************************/


#include <common.h>
#include <asm/arch/rk30_memap.h>
#include <lcd.h>
#include <asm/arch/rk30_drivers.h>


#define LcdReadBit(addr, msk)      ((regbak.addr=preg->addr)&(msk))
#define LcdWrReg(addr, val)        preg->addr=regbak.addr=(val)
#define LcdRdReg(addr)             (preg->addr)
#define LcdSetBit(addr, msk)       preg->addr=((regbak.addr) |= (msk))
#define LcdClrBit(addr, msk)       preg->addr=((regbak.addr) &= ~(msk))
#define LcdSetRegBit(addr, msk)    preg->addr=((preg->addr) |= (msk))
#define LcdMskReg(addr, msk, val)  (regbak.addr)&=~(msk);   preg->addr=(regbak.addr|=(val))
#define LCDC_REG_CFG_DONE()		LcdWrReg(REG_CFG_DONE, 0x01); 


/********************************************************************
**                          结构定义                                *
********************************************************************/
/* LCDC的寄存器结构 */

typedef volatile struct tagLCDC_REG
{
    /* offset 0x00~0xc0 */
	unsigned int SYS_CTRL;               //0x00 system control register 0
	unsigned int DSP_CTRL0;				 //0x04 display control register 0
	unsigned int DSP_CTRL1;				 //0x08 display control register 1
	unsigned int MCU_CTRL ;				 //0x0c MCU mode contol register
	unsigned int INT_STATUS;             //0x10 Interrupt status register
	unsigned int ALPHA_CTRL;             //0x14 Blending control register
	unsigned int WIN0_COLOR_KEY_CTRL;     //0x18 Win0 blending control register
	unsigned int WIN1_COLOR_KEY_CTRL;     //0x1C Win1 blending control register
	unsigned int WIN0_YRGB_MST0;           //0x20 Win0 active YRGB memory start address0
	unsigned int WIN0_CBR_MST0;            //0x24 Win0 active Cbr memory start address0
	unsigned int WIN0_YRGB_MST1;           //0x28 Win0 active YRGB memory start address1
	unsigned int WIN0_CBR_MST1;            //0x2C Win0 active Cbr memory start address1
    unsigned int WIN0_VIR;                //0x30 WIN0 virtual display width/height
	unsigned int WIN0_ACT_INFO;           //0x34 Win0 active window width/height
	unsigned int WIN0_DSP_INFO;           //0x38 Win0 display width/height on panel
	unsigned int WIN0_DSP_ST;             //0x3c Win0 display start point on panel
    unsigned int WIN0_SCL_FACTOR_YRGB;    //0x40 Win0 YRGB scaling  factor setting
	unsigned int WIN0_SCL_FACTOR_CBR;     //0x44 Win0 YRGB scaling factor setting
	unsigned int WIN0_SCL_OFFSET;         //0x48 Win0 Cbr scaling start point offset
	unsigned int WIN1_MST;                //0x4c Win1 active YRGB memory start address
    unsigned int WIN1_DSP_INFO;           //0x50 Win1 display width/height on panel
    unsigned int WIN1_DSP_ST;             //0x54 Win1 display start point on panel
	unsigned int HWC_MST;                 //0x58 HWC memory start address
	unsigned int HWC_DSP_ST;              //0x5C HWC display start point on panel
	unsigned int HWC_COLOR_LUT0;          //0x60 Hardware cursor color 2’b01 look up table 0
	unsigned int HWC_COLOR_LUT1;          //0x64 Hardware cursor color 2’b10 look up table 1
	unsigned int HWC_COLOR_LUT2;          //0x68 Hardware cursor color 2’b11 look up table 2
	unsigned int DSP_HTOTAL_HS_END;       //0x6c Panel scanning horizontal width and hsync pulse end point
	unsigned int DSP_HACT_ST_END;         //0x70 Panel active horizontal scanning start/end point
	unsigned int DSP_VTOTAL_VS_END;       //0x74 Panel scanning vertical height and vsync pulse end point
	unsigned int DSP_VACT_ST_END;         //0x78 Panel active vertical scanning start/end point
	unsigned int DSP_VS_ST_END_F1;        //0x7c Vertical scanning start point and vsync pulse end point of even filed in interlace mode
	unsigned int DSP_VACT_ST_END_F1;      //0x80 Vertical scanning active start/end point of even filed in interlace mode
	unsigned int reserved0[3];
	unsigned int REG_CFG_DONE;            //0xc0 REGISTER CONFIG FINISH
	unsigned int reserved1[(0x100-0x94)/4];
	unsigned int MCU_BYPASS_WPORT;         //0x100 MCU BYPASS MODE, DATA Write Only Port
	unsigned int reserved2[(0x200-0x104)/4];
	unsigned int MCU_BYPASS_RPORT;         //0x200 MCU BYPASS MODE, DATA Read Only Port   
	unsigned int reserved3[(0x400-0x204)/4];
	unsigned int WIN2_LUT_ADDR;
	unsigned int reserved4[(0x800-0x404)/4];
	unsigned int DSP_LUT_ADDR;
  
} LCDC_REG, *pLCDC_REG;

/*******************register definition**********************/

#define m_WIN0_EN		(1<<0)
#define m_WIN1_EN		(1<<1)
#define m_HWC_EN		(1<<2)
#define m_WIN0_FORMAT		(7<<3)
#define m_WIN1_FORMAT		(7<<6)
#define m_HWC_COLOR_MODE	(1<<9)
#define m_HWC_SIZE		(1<<10)
#define m_WIN0_3D_EN		(1<11)
#define m_WIN0_3D_MODE		(7<<12)
#define m_WIN0_RB_SWAP		(1<<15)
#define m_WIN0_ALPHA_SWAP	(1<<16)
#define m_WIN0_Y8_SWAP		(1<<17)
#define m_WIN0_UV_SWAP		(1<<18)
#define m_WIN1_RB_SWAP		(1<<19)
#define m_WIN1_ALPHA_SWAP	(1<<20)
#define m_WIN1_BL_SWAP		(1<<21)
#define m_WIN0_OTSD_DISABLE	(1<<22)
#define m_WIN1_OTSD_DISABLE	(1<<23)
#define m_DMA_BURST_LENGTH	(3<<24)
#define m_HWC_LODAD_EN		(1<<26)
#define m_WIN1_LUT_EN		(1<<27)
#define m_DSP_LUT_EN		(1<<28)
#define m_DMA_STOP		(1<<29)
#define m_LCDC_STANDBY		(1<<30)
#define m_AUTO_GATING_EN	(1<<31)
#define v_WIN0_EN(x)		(((x)&1)<<0)
#define v_WIN1_EN(x)		(((x)&1)<<1)
#define v_HWC_EN(x)		(((x)&1)<<2)
#define v_WIN0_FORMAT(x)	(((x)&7)<<3)
#define v_WIN1_FORMAT(x)	(((x)&7)<<6)
#define v_HWC_COLOR_MODE(x)	(((x)&1)<<9)
#define v_HWC_SIZE(x)		(((x)&1)<<10)
#define v_WIN0_3D_EN(x)		(((x)&1)<11)
#define v_WIN0_3D_MODE(x)	(((x)&7)<<12)
#define v_WIN0_RB_SWAP(x)	(((x)&1)<<15)
#define v_WIN0_ALPHA_SWAP(x)	(((x)&1)<<16)
#define v_WIN0_Y8_SWAP(x)	(((x)&1)<<17)
#define v_WIN0_UV_SWAP(x)	(((x)&1)<<18)
#define v_WIN1_RB_SWAP(x)	(((x)&1)<<19)
#define v_WIN1_ALPHA_SWAP(x)	(((x)&1)<<20)
#define v_WIN1_BL_SWAP(x)	(((x)&1)<<21)
#define v_WIN0_OTSD_DISABLE(x)	(((x)&1)<<22)
#define v_WIN1_OTSD_DISABLE(x)	(((x)&1)<<23)
#define v_DMA_BURST_LENGTH(x)	(((x)&3)<<24)
#define v_HWC_LODAD_EN(x)	(((x)&1)<<26)
#define v_WIN1_LUT_EN(x)	(((x)&1)<<27)
#define v_DSP_LUT_EN(x)		(((x)&1)<<28)
#define v_DMA_STOP(x)		(((x)&1)<<29)
#define v_LCDC_STANDBY(x)	(((x)&1)<<30)
#define v_AUTO_GATING_EN(x)	(((x)&1)<<31)


#define m_DSP_OUT_FORMAT	(0x0f<<0)
#define m_HSYNC_POL		(1<<4)
#define m_VSYNC_POL		(1<<5)
#define m_DEN_POL		(1<<6)
#define m_DCLK_POL		(1<<7)
#define m_WIN0_TOP		(1<<8)
#define m_DITHER_UP_EN		(1<<9)
#define m_DITHER_DOWN_MODE	(1<<10)
#define m_DITHER_DOWN_EN	(1<<11)
#define m_INTERLACE_DSP_EN	(1<<12)
#define m_INTERLACE_POL		(1<<13)
#define m_WIN0_INTERLACE_EN	(1<<14)
#define m_WIN1_INTERLACE_EN	(1<<15)
#define m_WIN0_YRGB_DEFLICK_EN	(1<<16)
#define m_WIN0_CBR_DEFLICK_EN	(1<<17)
#define m_WIN0_ALPHA_MODE	(1<<18)
#define m_WIN1_ALPHA_MODE	(1<<19)
#define m_WIN0_CSC_MODE		(3<<20)
#define m_WIN1_CSC_MODE		(1<<22)
#define m_WIN0_YUV_CLIP		(1<<23)
#define m_DSP_CCIR656_AVG	(1<<24)
#define m_DCLK_OUTPUT_MODE	(1<<25)
#define m_DCLK_PHASE_LOCK	(1<<26)
#define m_DITHER_DOWN_SEL	(3<<27)
#define m_ALPHA_MODE_SEL0	(1<<29)
#define m_ALPHA_MODE_SEL1	(1<<30)
#define m_DIFF_DCLK_EN		(1<<31)
#define v_DSP_OUT_FORMAT(x)	(((x)&0x0f)<<0)
#define v_HSYNC_POL(x)		(((x)&1)<<4)
#define v_VSYNC_POL(x)		(((x)&1)<<5)
#define v_DEN_POL(x)		(((x)&1)<<6)
#define v_DCLK_POL(x)		(((x)&1)<<7)
#define v_WIN0_TOP(x)		(((x)&1)<<8)
#define v_DITHER_UP_EN(x)	(((x)&1)<<9)
#define v_DITHER_DOWN_MODE(x)	(((x)&1)<<10)
#define v_DITHER_DOWN_EN(x)	(((x)&1)<<11)
#define v_INTERLACE_DSP_EN(x)	(((x)&1)<<12)
#define v_INTERLACE_POL(x)	(((x)&1)<<13)
#define v_WIN0_INTERLACE_EN(x)	(((x)&1)<<14)
#define v_WIN1_INTERLACE_EN(x)	(((x)&1)<<15)
#define v_WIN0_YRGB_DEFLICK_EN(x)	(((x)&1)<<16)
#define v_WIN0_CBR_DEFLICK_EN(x)	(((x)&1)<<17)
#define v_WIN0_ALPHA_MODE(x)		(((x)&1)<<18)
#define v_WIN1_ALPHA_MODE(x)		(((x)&1)<<19)
#define v_WIN0_CSC_MODE(x)		(((x)&3)<<20)
#define v_WIN1_CSC_MODE(x)		(((x)&1)<<22)
#define v_WIN0_YUV_CLIP(x)		(((x)&1)<<23)
#define v_DSP_CCIR656_AVG(x)		(((x)&1)<<24)
#define v_DCLK_OUTPUT_MODE(x)		(((x)&1)<<25)
#define v_DCLK_PHASE_LOCK(x)		(((x)&1)<<26)
#define v_DITHER_DOWN_SEL(x)		(((x)&1)<<27)
#define v_ALPHA_MODE_SEL0(x)		(((x)&1)<<29)
#define v_ALPHA_MODE_SEL1(x)		(((x)&1)<<30)
#define v_DIFF_DCLK_EN(x)		(((x)&1)<<31)


#define m_BG_COLOR		(0xffffff<<0)
#define m_BG_B			(0xff<<0)
#define m_BG_G			(0xff<<8)
#define m_BG_R			(0xff<<16)
#define m_BLANK_EN		(1<<24)
#define m_BLACK_EN		(1<<25)
#define m_DSP_BG_SWAP		(1<<26)
#define m_DSP_RB_SWAP		(1<<27)
#define m_DSP_RG_SWAP		(1<<28)
#define m_DSP_DELTA_SWAP	(1<<29)
#define m_DSP_DUMMY_SWAP	(1<<30)
#define m_DSP_OUT_ZERO		(1<<31)
#define v_BG_COLOR(x)		(((x)&0xffffff)<<0)
#define v_BG_B(x)		(((x)&0xff)<<0)
#define v_BG_G(x)		(((x)&0xff)<<8)
#define v_BG_R(x)		(((x)&0xff)<<16)
#define v_BLANK_EN(x)		(((x)&1)<<24)
#define v_BLACK_EN(x)		(((x)&1)<<25)
#define v_DSP_BG_SWAP(x)	(((x)&1)<<26)
#define v_DSP_RB_SWAP(x)	(((x)&1)<<27)
#define v_DSP_RG_SWAP(x)	(((x)&1)<<28)
#define v_DSP_DELTA_SWAP(x)	(((x)&1)<<29)
#define v_DSP_DUMMY_SWAP(x)	(((x)&1)<<30)
#define v_DSP_OUT_ZERO(x)	(((x)&1)<<31)


#define m_MCU_PIX_TOTAL		(0x3f<<0)
#define m_MCU_CS_ST		(0x0f<<6)
#define m_MCU_CS_END		(0x3f<<10)
#define m_MCU_RW_ST		(0x0f<<16)
#define m_MCU_RW_END		(0x3f<<20)
#define m_MCU_CLK_SEL		(1<<26)
#define m_MCU_HOLD_MODE		(1<<27)
#define m_MCU_FS_HOLD_STA	(1<<28)
#define m_MCU_RS_SELECT		(1<<29)
#define m_MCU_BYPASS 		(1<<30)
#define m_MCU_TYPE		(1<<31)

#define v_MCU_PIX_TOTAL(x)		(((x)&0x3f)<<0)
#define v_MCU_CS_ST(x)			(((x)&0x0f)<<6)
#define v_MCU_CS_END(x)			(((x)&0x3f)<<10)
#define v_MCU_RW_ST(x)			(((x)&0x0f)<<16)
#define v_MCU_RW_END(x)			(((x)&0x3f)<<20)
#define v_MCU_CLK_SEL(x)		(((x)&1)<<26)
#define v_MCU_HOLD_MODE(x)		(((x)&1)<<27)
#define v_MCU_FS_HOLD_STA(x)		(((x)&1)<<28)
#define v_MCU_RS_SELECT(x)		(((x)&1)<<29)
#define v_MCU_BYPASS(x) 		(((x)&1)<<30)
#define v_MCU_TYPE(x)			(((x)&1)<<31)

#define m_HS_INT_STA		(1<<0)  //status
#define m_FS_INT_STA		(1<<1)
#define m_LF_INT_STA		(1<<2)
#define m_BUS_ERR_INT_STA	(1<<3)
#define m_HS_INT_EN		(1<<4)  //enable
#define m_FS_INT_EN          	(1<<5)
#define m_LF_INT_EN         	(1<<6)
#define m_BUS_ERR_INT_EN	(1<<7)
#define m_HS_INT_CLEAR		(1<<8) //auto clear
#define m_FS_INT_CLEAR		(1<<9)
#define m_LF_INT_CLEAR		(1<<10)
#define m_BUS_ERR_INT_CLEAR	(1<<11)
#define m_LINE_FLAG_NUM		(0xfff<<12)
#define v_HS_INT_EN(x)		(((x)&1)<<4)
#define v_FS_INT_EN(x)		(((x)&1)<<5)
#define v_LF_INT_EN(x)		(((x)&1)<<6)
#define v_BUS_ERR_INT_EN(x)	(((x)&1)<<7)
#define v_HS_INT_CLEAR(x)	(((x)&1)<<8)
#define v_FS_INT_CLEAR(x)	(((x)&1)<<9)
#define v_LF_INT_CLEAR(x)	(((x)&1)<<10)
#define v_BUS_ERR_INT_CLEAR(x)	(((x)&1)<<11)
#define v_LINE_FLAG_NUM(x)	(((x)&0xfff)<<12)


#define m_WIN0_ALPHA_EN		(1<<0)
#define m_WIN1_ALPHA_EN		(1<<1)
#define m_HWC_ALPAH_EN		(1<<2)
#define m_WIN0_ALPHA_VAL	(0xff<<4)
#define m_WIN1_ALPHA_VAL	(0xff<<12)
#define m_HWC_ALPAH_VAL		(0x0f<<20)
#define v_WIN0_ALPHA_EN(x)	(((x)&1)<<0)
#define v_WIN1_ALPHA_EN(x)	(((x)&1)<<1)
#define v_HWC_ALPAH_EN(x)	(((x)&1)<<2)
#define v_WIN0_ALPHA_VAL(x)	(((x)&0xff)<<4)
#define v_WIN1_ALPHA_VAL(x)	(((x)&0xff)<<12)
#define v_HWC_ALPAH_VAL(x)	(((x)&0x0f)<<20)

#define m_COLOR_KEY_VAL		(0xffffff<<0)
#define m_COLOR_KEY_EN		(1<<24)
#define v_COLOR_KEY_VAL(x)	(((x)&0xffffff)<<0)
#define v_COLOR_KEY_EN(x)	(((x)&1)<<24)

#define m_WIN0_VIR   		(0x1fff << 0)
#define m_WIN1_VIR   		(0x1fff << 16)
#define v_ARGB888_VIRWIDTH(x) 	(((x)&0x1fff)<<0)
#define v_RGB888_VIRWIDTH(x) 	(((((x*3)>>2)+((x)%3))&0x1fff)<<0)
#define v_RGB565_VIRWIDTH(x) 	 ((DIV_ROUND_UP(x,2)&0x1fff)<<0)
#define v_YUV_VIRWIDTH(x)    	 ((DIV_ROUND_UP(x,4)&0x1fff)<<0)
#define v_WIN1_ARGB888_VIRWIDTH(x) 	(((x)&0x1fff)<<16)
#define v_WIN1_RGB888_VIRWIDTH(x) 	(((((x*3)>>2)+((x)%3))&0x1fff)<<16)
#define v_WIN1_RGB565_VIRWIDTH(x) 	 ((DIV_ROUND_UP(x,2)&0x1fff)<<16)

#define m_ACT_WIDTH       	(0x1fff<<0)
#define m_ACT_HEIGHT      	(0x1fff<<16)
#define v_ACT_WIDTH(x)       	(((x-1)&0x1fff)<<0)
#define v_ACT_HEIGHT(x)      	(((x-1)&0x1fff)<<16)

#define v_DSP_WIDTH(x)     	(((x-1)&0x7ff)<<0)
#define v_DSP_HEIGHT(x)    	(((x-1)&0x7ff)<<16)

#define v_DSP_STX(x)      	(((x)&0xfff)<<0)
#define v_DSP_STY(x)      	(((x)&0xfff)<<16)

#define v_X_SCL_FACTOR(x)  (((x)&0xffff)<<0)
#define v_Y_SCL_FACTOR(x)  (((x)&0xffff)<<16)

#define v_HSYNC(x)  		(((x)&0xfff)<<0)   //hsync pulse width
#define v_HORPRD(x) 		(((x)&0xfff)<<16)   //horizontal period

#define v_HAEP(x) 		(((x)&0xfff)<<0)  //horizontal active end point
#define v_HASP(x) 		(((x)&0xfff)<<16) //horizontal active start point

#define v_VSYNC(x) 		(((x)&0xfff)<<0)
#define v_VERPRD(x) 		(((x)&0xfff)<<16)

#define v_VAEP(x) 		(((x)&0xfff)<<0)
#define v_VASP(x) 		(((x)&0xfff)<<16)

#define preg ((pLCDC_REG)LCDC0_BASE)

//LCDC_REG *preg = LCDC0_BASE;  
LCDC_REG regbak;

/* Configure VENC for a given Mode (NTSC / PAL) */
void rk30_lcdc_set_par(struct fb_dsp_info *fb_info, vidinfo_t *vid)
{
	struct layer_par *par = &vid->par[fb_info->layer_id];
	if(par == NULL){
		printf("%s lay_par==NULL,id=%d\n",fb_info->layer_id);
	}
	if(fb_info != &par->fb_info)
		memcpy(&par->fb_info,fb_info,sizeof(struct fb_dsp_info *));

	switch(fb_info->layer_id){
		case WIN0:
			LcdWrReg(WIN0_SCL_FACTOR_YRGB, v_X_SCL_FACTOR(0x1000) | v_Y_SCL_FACTOR(0x1000));
			LcdWrReg(WIN0_SCL_FACTOR_CBR,v_X_SCL_FACTOR(0x1000)| v_Y_SCL_FACTOR(0x1000));
			LcdMskReg(SYS_CTRL, m_WIN0_FORMAT | m_WIN0_EN, v_WIN0_FORMAT(vid->logo_rgb_mode) | v_WIN0_EN(1) );	      //zyw
			LcdWrReg(WIN0_ACT_INFO,v_ACT_WIDTH(fb_info->xact) | v_ACT_HEIGHT(fb_info->yact));
			LcdWrReg(WIN0_DSP_ST, v_DSP_STX(fb_info->xpos + vid->vl_hspw + vid->vl_hbpd) | v_DSP_STY(fb_info->ypos + vid->vl_vspw + vid->vl_vbpd));
			LcdWrReg(WIN0_DSP_INFO, v_DSP_WIDTH(fb_info->xsize)| v_DSP_HEIGHT(fb_info->ysize));
			LcdMskReg(WIN0_COLOR_KEY_CTRL, m_COLOR_KEY_EN | m_COLOR_KEY_VAL,
					v_COLOR_KEY_EN(1) | v_COLOR_KEY_VAL(0));
			switch(vid->logo_rgb_mode) 
			{
				case ARGB888:
					LcdWrReg(WIN0_VIR,v_ARGB888_VIRWIDTH(fb_info->xvir));  //zyw
					break;
				case RGB888:  //rgb888
					LcdWrReg(WIN0_VIR,v_RGB888_VIRWIDTH(fb_info->xvir));
					break;
				case RGB565:  //rgb565
					LcdWrReg(WIN0_VIR,v_RGB565_VIRWIDTH(fb_info->xvir));
					break;
				case YUV422:
				case YUV420:   
					LcdWrReg(WIN0_VIR,v_YUV_VIRWIDTH(fb_info->xvir));
					break;
				default:
					LcdWrReg(WIN0_VIR,v_RGB888_VIRWIDTH(fb_info->xvir));
					break;
			}
			LcdWrReg(WIN0_YRGB_MST0, fb_info->yaddr);
			break;
		case WIN1:
			printf("%s --->WIN1 not support \n");
			break;
		default:
			printf("%s --->unknow lay_id \n");
			break;
	}

    LCDC_REG_CFG_DONE();
}

int rk30_load_screen(vidinfo_t *vid)
{
	int ret = -1;
    int face = 0;
	switch (vid->lcd_face)
	{
    	case OUT_P565:
        		face = OUT_P565;
        		LcdMskReg(DSP_CTRL0, m_DITHER_DOWN_EN | m_DITHER_DOWN_MODE, v_DITHER_DOWN_EN(1) | v_DITHER_DOWN_MODE(0));
        		break;
    	case OUT_P666:
        		face = OUT_P666;
        		LcdMskReg(DSP_CTRL0, m_DITHER_DOWN_EN | m_DITHER_DOWN_MODE, v_DITHER_DOWN_EN(1) | v_DITHER_DOWN_MODE(1));
        		break;
    	case OUT_D888_P565:
        		face = OUT_P888;
        		LcdMskReg(DSP_CTRL0, m_DITHER_DOWN_EN | m_DITHER_DOWN_MODE, v_DITHER_DOWN_EN(1) | v_DITHER_DOWN_MODE(0));
        		break;
    	case OUT_D888_P666:
        		face = OUT_P888;
        		LcdMskReg(DSP_CTRL0, m_DITHER_DOWN_EN | m_DITHER_DOWN_MODE, v_DITHER_DOWN_EN(1) | v_DITHER_DOWN_MODE(1));
        		break;
    	case OUT_P888:
        		face = OUT_P888;
        		LcdMskReg(DSP_CTRL0, m_DITHER_UP_EN, v_DITHER_UP_EN(1));
        		LcdMskReg(DSP_CTRL0, m_DITHER_DOWN_EN | m_DITHER_DOWN_MODE, v_DITHER_DOWN_EN(0) | v_DITHER_DOWN_MODE(0));
        		break;
    	default:
        		LcdMskReg(DSP_CTRL0, m_DITHER_UP_EN, v_DITHER_UP_EN(0));
        		LcdMskReg(DSP_CTRL0, m_DITHER_DOWN_EN | m_DITHER_DOWN_MODE, v_DITHER_DOWN_EN(0) | v_DITHER_DOWN_MODE(0));
        		face = vid->lcd_face;
        		break;
	}

	//use default overlay,set vsyn hsync den dclk polarity
	LcdMskReg(DSP_CTRL0,m_DSP_OUT_FORMAT | m_HSYNC_POL | m_VSYNC_POL |
     		m_DEN_POL |m_DCLK_POL,v_DSP_OUT_FORMAT(face) | 
     		v_HSYNC_POL(vid->vl_hsp) | v_VSYNC_POL(vid->vl_vsp) |
        	v_DEN_POL(vid->vl_oep) | v_DCLK_POL(vid->vl_clkp));

	//set background color to black,set swap according to the screen panel,disable blank mode
	LcdMskReg(DSP_CTRL1, m_BG_COLOR | m_DSP_RB_SWAP | m_DSP_RG_SWAP | m_DSP_DELTA_SWAP | 
	 	m_DSP_DUMMY_SWAP | m_BLANK_EN | m_BG_R,v_BG_COLOR(0x000000) | v_DSP_RB_SWAP(vid->vl_swap_rb) | 
	 	v_DSP_RG_SWAP(0) | v_DSP_DELTA_SWAP(0) | v_DSP_DUMMY_SWAP(0) | v_BLANK_EN(0) | v_BG_R(0));
	
	LcdWrReg(DSP_HTOTAL_HS_END,v_HSYNC(vid->vl_hspw) |
             v_HORPRD(vid->vl_hspw + vid->vl_hbpd + vid->vl_col + vid->vl_hfpd));
	LcdWrReg(DSP_HACT_ST_END, v_HAEP(vid->vl_hspw + vid->vl_hbpd + vid->vl_col) |
             v_HASP(vid->vl_hspw + vid->vl_hbpd));

	LcdWrReg(DSP_VTOTAL_VS_END, v_VSYNC(vid->vl_vspw) |
              v_VERPRD(vid->vl_vspw + vid->vl_vbpd + vid->vl_row + vid->vl_vfpd));
	LcdWrReg(DSP_VACT_ST_END,  v_VAEP(vid->vl_vspw + vid->vl_vbpd + vid->vl_row)|
              v_VASP(vid->vl_vspw + vid->vl_vbpd));

    // let above to take effect
	LCDC_REG_CFG_DONE();

    set_lcdc_dclk(vid->vl_freq);
    
	printf("%s for lcdc ok!\n",__func__);
	return 0;
}


/* Enable LCD and DIGITAL OUT in DSS */
void rk30_lcdc_enable(void)
{
    //LcdMskReg(DSP_CTRL1,m_BLANK_MODE ,v_BLANK_MODE(1));

}

/* Enable LCD and DIGITAL OUT in DSS */
void rk30_lcdc_standby(enable)
{
    LcdMskReg(SYS_CTRL, m_WIN0_EN, v_WIN0_EN(enable?0:1));
    LCDC_REG_CFG_DONE();  
}


int rk30_lcdc_init()
{
    lcdc_clk_enable();
    #ifdef CONFIG_VCC_LCDC_1_8
        g_grfReg->GRF_IO_CON[4] = 0x40004000;
    #endif	
	LcdMskReg(SYS_CTRL, m_AUTO_GATING_EN|m_LCDC_STANDBY | m_WIN0_EN, v_AUTO_GATING_EN(1)|v_LCDC_STANDBY(0)|v_WIN0_EN(1));	      //zyw
//	LcdMskReg(INT_STATUS, m_FS_INT_EN, v_FS_INT_EN(1));  
	LCDC_REG_CFG_DONE();  // write any value to  REG_CFG_DONE let config become effective
	return 0;
}

