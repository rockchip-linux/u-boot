

#include <common.h>
#include <asm/arch/grf.h>
#include <asm/arch/iomux.h>
#include <asm/arch/drivers.h>

static void rk_pwm_iomux_config(int pwm_id)
{
    switch (pwm_id){
    case RK_PWM0_IOMUX:
        grf_writel((3<<16)|1, RK3288_GRF_GPIO7A_IOMUX);     
        break;
    case RK_PWM1_IOMUX:
        grf_writel((1<<18)|(1<<2), RK3288_GRF_GPIO7A_IOMUX); 
        break;
    case RK_PWM2_IOMUX:
        grf_writel((1<<24)|(1<<8), RK3288_GRF_GPIO7CH_IOMUX); 
        break;
    case RK_PWM3_IOMUX:
        grf_writel((1<<16)|1, RK3288_GRF_GPIO7CH_IOMUX); 
        break;
    default :
        printf("RK have not this pwm iomux id!\n");
        break;
    }
}
static void rk_i2c_iomux_config(int i2c_id)
{
    switch (i2c_id){
    case RK_I2C0_IOMUX: 
        pmu_writel(pmu_readl(0x88)|(1<<14), 0x88);
        pmu_writel(pmu_readl(0x8c)|1, 0x8c);
        break;
    case RK_I2C1_IOMUX:
        grf_writel((1<<26)|(1<<24)|(1<<10)|(1<<8), RK3288_GRF_GPIO8A_IOMUX);
        break;
    case RK_I2C2_IOMUX:
        grf_writel((1<<20)|(1<<18)|(1<<4)|(1<<2), RK3288_GRF_GPIO6B_IOMUX);
        break;
    case RK_I2C3_IOMUX:
        grf_writel((1<<20)|(1<<18)|(1<<2)|1, RK3288_GRF_GPIO2C_IOMUX);
        break;
    case RK_I2C4_IOMUX:
        grf_writel((1<<28)|(1<<26)|(1<<12)|(1<<10), RK3288_GRF_GPIO1D_IOMUX);
        break;
    default :
        printf("RK have not this i2c iomux id!\n");
        break;        
    }
}

static void rk_lcdc_iomux_config()
{
    grf_writel(0x00550055, RK3288_GRF_GPIO1D_IOMUX);  //lcdc iomux
}

static void rk_spi_iomux_config(int spi_id)
{
    switch (spi_id){
    case RK_SPI0_CS0_IOMUX:
        grf_writel((((0x3<<14)|(0x3<<12)|(0x3<<10)|(0x3<<8))<<16)|(0x1<<14)|(0x1<<12)|(0x1<<10)|(0x1<<8), RK3288_GRF_GPIO5B_IOMUX); 
        break;
    case RK_SPI0_CS1_IOMUX:
        grf_writel((((0x3<<14)|(0x3<<12)|(0x3<<8))<<16)|(0x1<<14)|(0x1<<12)|(0x1<<8), RK3288_GRF_GPIO5B_IOMUX); 
        grf_writel(((0x3)<<16)|(0x1), RK3288_GRF_GPIO5C_IOMUX); 
        break;
    case RK_SPI1_CS0_IOMUX:
        grf_writel((((0x3<<14)|(0x3<<12)|((0x3<<10))|(0x3<<8))<<16)|(0x2<<14)|(0x2<<12)|((0x2<<10))|(0x2<<8), RK3288_GRF_GPIO7B_IOMUX);    
        break;
    case RK_SPI1_CS1_IOMUX:
        printf("rkspi: bus=1 cs=1 not support");
        break;
    case RK_SPI2_CS0_IOMUX:  
        grf_writel(((0xf<<12)<<16) | (0x5<<12), RK3288_GRF_GPIO8A_IOMUX); 
        grf_writel((((0x3<<2)|(0x3))<<16)|(0x1<<2)|(0x1), RK3288_GRF_GPIO8B_IOMUX); 
        break;
    case RK_SPI2_CS1_IOMUX:
        grf_writel((((0x3<<12)|(0x3<<6))<<16)|(0x1<<12)|(0x1<<6), RK3288_GRF_GPIO8A_IOMUX); 
        grf_writel((((0x3<<2)|(0x3))<<16)|(0x1<<2)|(0x1), RK3288_GRF_GPIO8B_IOMUX); 
        break;    
    default :
        printf("RK have not this spi iomux id!\n");
        break;
    }
}

static void rk_uart_iomux_config(int uart_id)
{
    switch (uart_id){
    case RK_UART_BT_IOMUX:
        grf_writel((0x55<<16)|0x55, RK3288_GRF_GPIO4C_IOMUX); 
        break;
    case RK_UART_BB_IOMUX:
        grf_writel((0xff<<16)|0x55, RK3288_GRF_GPIO5B_IOMUX); 
        break;
    case RK_UART_DBG_IOMUX:
        grf_writel((3<<28)|(3<<24)|(1<<12)|(1<<8), RK3288_GRF_GPIO7CH_IOMUX); 
        break;
    case RK_UART_GPS_IOMUX:
        grf_writel((0xff<<16)|0x55, RK3288_GRF_GPIO7B_IOMUX); 
        break;
    case RK_UART_EXP_IOMUX:
        grf_writel((0xff<<24)|(0xff<<8), RK3288_GRF_GPIO5B_IOMUX); 
        break;
    default:
        printf("RK have not this uart iomux id!\n");
        break;       
    }

}

void rk_iomux_config(int iomux_id)
{
    switch (iomux_id){
    case RK_PWM0_IOMUX:
    case RK_PWM1_IOMUX:
    case RK_PWM2_IOMUX:
    case RK_PWM3_IOMUX:
    case RK_PWM4_IOMUX:
        rk_pwm_iomux_config(iomux_id);
        break;
    case RK_I2C0_IOMUX:        
    case RK_I2C1_IOMUX:
    case RK_I2C2_IOMUX:
    case RK_I2C3_IOMUX:
    case RK_I2C4_IOMUX:
        rk_i2c_iomux_config(iomux_id);
        break;
    case RK_UART_BT_IOMUX:
    case RK_UART_BB_IOMUX:
    case RK_UART_DBG_IOMUX:
    case RK_UART_GPS_IOMUX:
    case RK_UART_EXP_IOMUX:
        rk_uart_iomux_config(iomux_id);
        break;    
    case RK_LCDC0_IOMUX:
        rk_lcdc_iomux_config();
        break;
    case RK_SPI0_CS0_IOMUX:
    case RK_SPI0_CS1_IOMUX:
    case RK_SPI1_CS0_IOMUX:
    case RK_SPI1_CS1_IOMUX:
    case RK_SPI2_CS0_IOMUX:
    case RK_SPI2_CS1_IOMUX:
        rk_pwm_iomux_config(iomux_id);
        break;
    default :
        printf("RK have not this iomux id!\n");
        break;
    }     

}


