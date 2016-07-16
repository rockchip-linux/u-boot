/*
 * (C) Copyright 2008-2016 Fuzhou Rockchip Electronics Co., Ltd
 * Peter, Software Engineering, <superpeter.cai@gmail.com>.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <linux/input.h>
#include <asm/io.h>
#include <asm/arch/rkplat.h>


/* PWM0 registers  */
#define PWM_REG_CNTR                    0x00  /* Counter Register */
#define PWM_REG_HPR                     0x04  /* Period Register */
#define PWM_REG_LPR                     0x08  /* Duty Cycle Register */
#define PWM_REG_CTRL                    0x0c  /* Control Register */


/*REG_CTRL bits definitions*/
#define PWM_ENABLE                      (1 << 0)
#define PWM_DISABLE                     (0 << 0)

/*operation mode*/
#define PWM_MODE_ONESHOT                (0x00 << 1)
#define PWM_MODE_CONTINUMOUS            (0x01 << 1)
#define PWM_MODE_CAPTURE                (0x02 << 1)

/*duty cycle output polarity*/
#define PWM_DUTY_POSTIVE                (0x01 << 3)
#define PWM_DUTY_NEGATIVE               (0x00 << 3)

/*incative state output polarity*/
#define PWM_INACTIVE_POSTIVE            (0x01 << 4)
#define PWM_INACTIVE_NEGATIVE           (0x00 << 4)

/*clock source select*/
#define PWM_CLK_SCALE                   (1 << 9)
#define PWM_CLK_NON_SCALE               (0 << 9)

#define PWM_CH0_INT                     (1 << 0)
#define PWM_CH1_INT                     (1 << 1)
#define PWM_CH2_INT                     (1 << 2)
#define PWM_CH3_INT                     (1 << 3)

#define PWM_CH0_POL                     (1 << 8)
#define PWM_CH1_POL                     (1 << 9)
#define PWM_CH2_POL                     (1 << 10)
#define PWM_CH3_POL                     (1 << 11)

#define PWM_CH0_INT_ENABLE              (1 << 0)
#define PWM_CH0_INT_DISABLE             (0 << 0)

#define PWM_CH1_INT_ENABLE              (1 << 0)
#define PWM_CH1_INT_DISABLE             (0 << 1)

#define PWM_CH2_INT_ENABLE              (1 << 2)
#define PWM_CH2_INT_DISABLE             (0 << 2)

#define PWM_CH3_INT_ENABLE              (1 << 3)
#define PWM_CH3_INT_DISABLE             (0 << 3)

/*prescale factor*/
#define PWMCR_MIN_PRESCALE              0x00
#define PWMCR_MAX_PRESCALE              0x07

#define PWMDCR_MIN_DUTY	       	        0x0001
#define PWMDCR_MAX_DUTY                 0xFFFF

#define PWMPCR_MIN_PERIOD               0x0001
#define PWMPCR_MAX_PERIOD               0xFFFF

#define PWMPCR_MIN_PERIOD               0x0001
#define PWMPCR_MAX_PERIOD               0xFFFF


int g_ir_keycode = 0;
int g_ir_flag_signal = 0;


typedef enum _RMC_STATE {
	RMC_IDLE,
	RMC_PRELOAD,
	RMC_USERCODE,
	RMC_GETDATA,
	RMC_SEQUENCE
} eRMC_STATE;


struct rkxx_remote_key_table {
	int scanCode;
	int keyCode;		
};

struct rkxx_remotectl_button {	
	int usercode;
	int nbuttons;
	struct rkxx_remote_key_table *key_table;
};

struct rkxx_remotectl_drvdata {
	int state;
	int nbuttons;
	int pwm_freq_nstime;
	long period;
	int scandata;
	int count;
	int keybdNum;
	int keycode;
	int press;
	unsigned int  base;
};


/********************************************************************
**                            ºê¶¨Òå                                *
********************************************************************/
#define RK_PWM_TIME_PRE_MIN      3000   /*4500*/
#define RK_PWM_TIME_PRE_MAX      5000   /*5500*/           /*PreLoad 4.5+0.56 = 5.06ms*/

#define RK_PWM_TIME_BIT0_MIN     480  /*Bit0  1.125ms*/
#define RK_PWM_TIME_BIT0_MAX     700

#define RK_PWM_TIME_BIT1_MIN     1200  /*Bit1  2.25ms*/
#define RK_PWM_TIME_BIT1_MAX     2000

#define RK_PWM_TIME_RPT_MIN      2000   /*101000*/
#define RK_PWM_TIME_RPT_MAX      2500   /*103000*/         /*Repeat  105-2.81=102.19ms*/  //110-9-2.25-0.56=98.19ms

#define RK_PWM_TIME_SEQ1_MIN     95000   /*2650*/
#define RK_PWM_TIME_SEQ1_MAX     98000   /*3000*/           /*sequence  2.25+0.56=2.81ms*/ //11.25ms

#define RK_PWM_TIME_SEQ2_MIN     30000   /*101000*/
#define RK_PWM_TIME_SEQ2_MAX     55000   /*103000*/         /*Repeat  105-2.81=102.19ms*/  //110-9-2.25-0.56=98.19ms

#define PWM3_REG_INT_EN                 0x14  /* Interrupt Enable Refister For Pwm3*/
#define PWM2_REG_INT_EN                 0x24  /* Interrupt Enable Refister For Pwm2*/
#define PWM1_REG_INT_EN                 0x34  /* Interrupt Enable Refister For Pwm1*/
#define PWM0_REG_INT_EN                 0x44  /* Interrupt Enable Refister For Pwm0*/

#define BIT(x)                  (1 << (x))
#define PWM_REG_INTSTS(n)       ((4 - (n)) * 0x10)
#define PWM_REG_INT_EN(n)		((4 - (n)) * 0x10)
#define PWM_CH_INT(n)   		BIT(n)
#define PWM_CH_POL(n)   		BIT(n+8)
extern unsigned int rkclk_get_pwm_clk(uint32 pwm_id);



static struct rkxx_remotectl_drvdata data = {0};
static struct rkxx_remotectl_drvdata *ddata = NULL;



static struct rkxx_remote_key_table remote_key_table_meiyu_4040[] = {
	{0xf2, KEY_REPLY},
	{0xba, KEY_BACK},
	{0xf4, KEY_UP},
	{0xf1, KEY_DOWN},
	{0xef, KEY_LEFT},
	{0xee, KEY_RIGHT},
	{0xbd, KEY_HOME},
	{0xea, KEY_VOLUMEUP},
	{0xe3, KEY_VOLUMEDOWN},
	{0xe2, KEY_SEARCH},
	{0xb2, KEY_POWER},
	{0xbc, KEY_MUTE},
	{0xec, KEY_MENU},
/*lay pause*/
	{0xbf, 0x190},
/*pre*/
	{0xe0, 0x191},
/*next*/
	{0xe1, 0x192},
/*pic,rorate left*/
	{0xe9, 183},
/*rorate right*/
	{0xe6, 248},
/*zoom out*/
	{0xe8, 185},
/*zoom in*/
	{0xe7, 186},
/*mouse switch*/
	{0xb8, 388},
/*zoom outdisplay switch*/
	{0xbe, 0x175},
};


static struct rkxx_remote_key_table remote_key_table_sunchip_ff00[] = {
	{0xf9, KEY_HOME},
	{0xbf, KEY_BACK},
	{0xfb, KEY_MENU},
	{0xaa, KEY_REPLY},
	{0xb9, KEY_UP},
	{0xe9, KEY_DOWN},
	{0xb8, KEY_LEFT},
	{0xea, KEY_RIGHT},
	{0xeb, KEY_VOLUMEDOWN},
	{0xef, KEY_VOLUMEUP},
	{0xf7, KEY_MUTE},
	{0xe7, KEY_POWER},
	{0xfc, KEY_POWER},
	{0x63, KEY_POWER}, 
	{0xa9, KEY_VOLUMEDOWN},
	{0xa8, KEY_VOLUMEDOWN},
	{0xe0, KEY_VOLUMEDOWN},
	{0xa5, KEY_VOLUMEDOWN},
	{0xab, 183},
	{0xb7, 388},
	{0xf8, 184},
	{0xaf, 185},
	{0xed, KEY_VOLUMEDOWN},
	{0xee, 186},
	{0xb3, KEY_VOLUMEDOWN},
	{0xf1, KEY_VOLUMEDOWN},
	{0xf2, KEY_VOLUMEDOWN},
	{0xf3, KEY_SEARCH},
	{0xb4, KEY_VOLUMEDOWN},
	{0xbe, KEY_SEARCH},
};

//########################################

static struct rkxx_remote_key_table remote_key_table_meiyu_202[] = {
	{0xf2, KEY_REPLY},//ok = DPAD CENTER
	{0xba, KEY_BACK}, 
	{0xf4, KEY_UP},
	{0xf1, KEY_DOWN},
	{0xef, KEY_LEFT},
	{0xee, KEY_RIGHT}, ////////
	{0xbd, KEY_HOME}, //home
	{0xea, KEY_VOLUMEUP},
	{0xe3, KEY_VOLUMEDOWN},
	{0xb8, KEY_SEARCH}, //search
	{0xb2, KEY_POWER}, //power off
	{0xbc, KEY_MUTE}, //mute
	{0xec, KEY_MENU},

//media ctrl
	{0xe1, 0x190}, //play pause
	{0xe0, 0x191}, //pre
	{0xbf, 0x192}, //next

//pic
	{0xe2, 183}, //rorate left
	{0xe5, 184}, //rorate right
	{0xe9, 185}, //zoom out
	{0xe6, 186}, //zoom in
//mouse switch
	{0xf0,388},
//display switch
	{0xbe, 0x175},
};

static struct rkxx_remote_key_table remote_key_table_df[] = {
	{0xe0, KEY_REPLY},
	{0xfc, KEY_BACK}, 
	{0xf0, KEY_UP},
	{0xe4, KEY_DOWN},
	{0xf4, KEY_LEFT},
	{0xe8,KEY_RIGHT}, ////////
	{0xf6, KEY_VOLUMEDOWN},
	{0xf9, KEY_VOLUMEUP},
	{0xfe, KEY_HOME}, //home
	{0xf8, 183}, //rorate left
	{0xf7, 184}, //rorate right
	{0xfb, 185}, //zoom out
	{0xfa, 186}, //zoom in
	{0xf1, KEY_MUTE}, //mute
	{0xf5, KEY_POWER}, //power off
	{0xfd, KEY_SEARCH}, //search
};

static struct rkxx_remote_key_table remote_key_table_4db2[] = {
	{0x23, KEY_POWER},
	{0x77, KEY_HOME},
	{0x7d, KEY_SEARCH},

	{0x35, KEY_UP},
	{0x2d, KEY_DOWN},
	{0x66, KEY_LEFT},
	{0x3e, KEY_RIGHT},
	{0x31, KEY_REPLY},
	{0x3a, KEY_BACK},
	{0x65, KEY_MENU},

	{0x7e, KEY_VOLUMEDOWN},
	{0x7f, KEY_VOLUMEUP},

	{0x6d, KEY_1},
	{0x6c, KEY_2},
	{0x33, KEY_3},
	{0x71, KEY_4},
	{0x70, KEY_5},

	{0x37, KEY_6},
	{0x75, KEY_7},
	{0x74, KEY_8},
	{0x3b, KEY_9},
	{0x78, KEY_0},

	{0x25, KEY_MINUS}, //KEY_NUMERIC_STAR
	{0x2f, KEY_EQUAL}, //KEY_NUMERIC_POUND
};

static struct rkxx_remote_key_table remote_key_table_ff[] = {
	{0xe7, KEY_POWER},
	{0xf9, KEY_HOME},
	{0xa4, KEY_SEARCH},

	{0xb9, KEY_UP},
	{0xe9, KEY_DOWN},
	{0xb8, KEY_LEFT},
	{0xea, KEY_RIGHT},
	{0xaa, KEY_REPLY},
	{0xbf, KEY_BACK},
	{0xfb, KEY_MENU},

	{0xeb, KEY_VOLUMEDOWN},
	{0xf7, KEY_MUTE},
	{0xef, KEY_VOLUMEUP},

	{0xab, KEY_1},
	{0xb7, KEY_2},
	{0xf8, KEY_3},
	{0xaf, KEY_4},
	{0xed, KEY_5},

	{0xee, KEY_6},
	{0xb3, KEY_7},
	{0xf1, KEY_8},
	{0xf2, KEY_9},
	{0xf3, KEY_0},

	{0xbe, KEY_MINUS}, //KEY_NUMERIC_STAR
	{0xb4, KEY_EQUAL}, //KEY_NUMERIC_POUND
};



static struct rkxx_remotectl_button remotectl_button[] = {
	{
		.usercode = 0xff00,
		.nbuttons =  29,
		.key_table = &remote_key_table_sunchip_ff00[0],
	},
	{
		.usercode = 0x4040,
		.nbuttons =  22,
		.key_table = &remote_key_table_meiyu_4040[0],
	},	
	{
		.usercode = 0x202,
		.nbuttons =  22,
		.key_table = &remote_key_table_meiyu_202[0],
	},
	{
		.usercode = 0xdf,
		.nbuttons =  16,
		.key_table = &remote_key_table_df[0],
	},
	{
		.usercode = 0x4db2,
		.nbuttons =  24,
		.key_table = &remote_key_table_4db2[0],
	},
	{
		.usercode = 0xff,
		.nbuttons =  25,
		.key_table = &remote_key_table_ff[0],
	},
};

static int remotectl_keybd_num_lookup(struct rkxx_remotectl_drvdata *ddata)
{	
	int i;	

	for (i = 0; i < sizeof(remotectl_button)/sizeof(struct rkxx_remotectl_button); i++){		
		if (remotectl_button[i].usercode == (ddata->scandata&0xFFFF)){			
			ddata->keybdNum = i;
			return 1;
		}
	}

	return 0;
}

static int remotectl_keycode_lookup(struct rkxx_remotectl_drvdata *ddata)
{	
	int i;	
	unsigned char keyData = (unsigned char)((ddata->scandata >> 8) & 0xff);

	for (i = 0; i < remotectl_button[ddata->keybdNum].nbuttons; i++){
		if (remotectl_button[ddata->keybdNum].key_table[i].scanCode == keyData){			
			ddata->keycode = remotectl_button[ddata->keybdNum].key_table[i].keyCode;
			return 1;
		}
	}

	return 0;
}

int remotectl_do_something_readtime(void)
{
	int val;
	int temp_hpr;
	unsigned int id = RK_PWM_REMOTE_ID;
	int ret = 0;
	
	if (id > 3)
		return 0;
	val = readl(ddata->base + PWM_REG_INTSTS(id));
	if ((val & PWM_CH_INT(id)) == 0)
		return ret;
	if ((val & PWM_CH_POL(id))) {
		temp_hpr = readl(ddata->base + PWM_REG_HPR);
		//printf("lpr=%d\n", temp_hpr);
		ddata->period = ddata->pwm_freq_nstime * temp_hpr / 1000;
		//printf("period=%d\n", ddata->period);
		ret = 1;
	}
	writel(PWM_CH_INT(id), ddata->base + PWM_REG_INTSTS(id));
	return ret;
}


int remotectl_do_something(void)
{
	if(remotectl_do_something_readtime() == 0)
	{
		return 0;
	}
	
	switch (ddata->state) {
	case RMC_IDLE: {
		;
		break;
	}
	case RMC_PRELOAD: {
		g_ir_flag_signal = 1;
		if ((RK_PWM_TIME_PRE_MIN < ddata->period) &&
		    (ddata->period < RK_PWM_TIME_PRE_MAX)) {
			ddata->scandata = 0;
			ddata->count = 0;
			ddata->state = RMC_USERCODE;
		} else {
			ddata->state = RMC_PRELOAD;
		}
		break;
	}
	case RMC_USERCODE: {
		if ((RK_PWM_TIME_BIT1_MIN < ddata->period) &&
		    (ddata->period < RK_PWM_TIME_BIT1_MAX))
			ddata->scandata |= (0x01 << ddata->count);
		ddata->count++;
		if (ddata->count == 0x10) {
			//printf("USERCODE=0x%x\n", ddata->scandata);
			if (remotectl_keybd_num_lookup(ddata)) {
				ddata->state = RMC_GETDATA;
				ddata->scandata = 0;
				ddata->count = 0;
			} else {
				ddata->state = RMC_PRELOAD;
			}
		}
	}
	break;
	case RMC_GETDATA: {
		if ((RK_PWM_TIME_BIT1_MIN < ddata->period) &&
		    (ddata->period < RK_PWM_TIME_BIT1_MAX))
			ddata->scandata |= (0x01<<ddata->count);
		ddata->count++;
		if (ddata->count < 0x10)
			return 0;
		//printf("RMC_GETDATA=%x\n", (ddata->scandata>>8));
		if ((ddata->scandata&0x0ff) ==
		    ((~ddata->scandata >> 8) & 0x0ff)) {
			if (remotectl_keycode_lookup(ddata)) {
				ddata->press = 1;
				ddata->state = RMC_PRELOAD;
				g_ir_keycode = ddata->keycode;
					//printf("g_ir_keycode = [%d] \n", g_ir_keycode);
			} else {
				ddata->state = RMC_PRELOAD;
			}
		} else {
			ddata->state = RMC_PRELOAD;
		}
	}
	break;
	
	default:
	break;
	}
	return 0;
}


static int rk_pwm_remotectl_hw_init(struct rkxx_remotectl_drvdata *ddata)
{
	int val;
	
	val = readl(ddata->base + PWM_REG_CTRL);
	val = (val & 0xFFFFFFFE) | PWM_DISABLE;
	writel(val, ddata->base + PWM_REG_CTRL);
	val = readl(ddata->base + PWM_REG_CTRL);
	val = (val & 0xFFFFFFF9) | PWM_MODE_CAPTURE;
	writel(val, ddata->base + PWM_REG_CTRL);
	val = readl(ddata->base + PWM_REG_CTRL);
	val = (val & 0xFF008DFF) | 0x0006000;
	writel(val, ddata->base + PWM_REG_CTRL);
	switch (RK_PWM_REMOTE_ID) {
	case 0: {
		val = readl(ddata->base + PWM0_REG_INT_EN);
		val = (val & 0xFFFFFFFE) | PWM_CH0_INT_ENABLE;
		writel(val, ddata->base + PWM0_REG_INT_EN);
	}
	break;
	case 1:	{
		val = readl(ddata->base + PWM1_REG_INT_EN);
		val = (val & 0xFFFFFFFD) | PWM_CH1_INT_ENABLE;
		writel(val, ddata->base + PWM1_REG_INT_EN);
	}
	break;
	case 2:	{
		val = readl(ddata->base + PWM2_REG_INT_EN);
		val = (val & 0xFFFFFFFB) | PWM_CH2_INT_ENABLE;
		writel(val, ddata->base + PWM2_REG_INT_EN);
	}
	break;
	case 3:	{
		val = readl(ddata->base + PWM3_REG_INT_EN);
		val = (val & 0xFFFFFFF7) | PWM_CH3_INT_ENABLE;
		writel(val, ddata->base + PWM3_REG_INT_EN);
	}
	break;
	default:
	break;
	}
	val = readl(ddata->base + PWM_REG_CTRL);
	val = (val & 0xFFFFFFFE) | PWM_ENABLE;
	writel(val, ddata->base + PWM_REG_CTRL);
	return 0;
}


void remotectlInitInDriver(void)
{
	int pwm_freq;

	printf("remotectl v0.1\n");
	ddata = &data;
	ddata->state = RMC_PRELOAD;
	ddata->base = RK_PWM_REMOTE_IOBASE;

	pwm_freq = rkclk_get_pwm_clk(RK_PWM_REMOTE_ID)/64;
	printf("pwm freq=0x%x\n",pwm_freq);
	ddata->pwm_freq_nstime = 1000000000 / pwm_freq;
	printf("pwm_freq_nstime=0x%x\n",ddata->pwm_freq_nstime);
	rk_iomux_config(RK_PWM0_IOMUX + RK_PWM_REMOTE_ID);
	rk_pwm_remotectl_hw_init(ddata);
	
}

