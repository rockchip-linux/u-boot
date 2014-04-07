/*
 * Copyright (C) 2014
 * zyw <zyw@rock-chips.com>
 *
 */
#include <common.h>
#include <asm/gpio.h>
#include <asm/io.h>

/*
 * It's how we calculate the full port address
 * We have to get the group number from gpio
 * So we set the base addr to high 16bit of gpio
 * eg:gpio=0xFF780008, it mean gpio1_B0, 0xFF780000 is base addr of GPIO1, 0x8 is gpio_b0 
 */

int gpio_set_value(unsigned gpio, int value)
{
	int reg_addr = gpio&0xffff0000;
    int index = gpio&0xffff;
    if(reg_addr==0)
    {
        printf("no gpio group \n");
        return -1;
    }
    if(value)writel(readl(reg_addr+RK_GPIO_WRITE_REG)|(1ul<<index),reg_addr+RK_GPIO_WRITE_REG);
    else writel(readl(reg_addr+RK_GPIO_WRITE_REG)&(~(1ul<<index)), reg_addr+RK_GPIO_WRITE_REG);
    return 0;
}

int gpio_get_value(unsigned gpio)
{
	int reg_addr = gpio&0xffff0000;
    int index = gpio&0xffff;
    if(reg_addr==0)
    {
        printf("no gpio group \n");
    }
    return (readl(reg_addr+RK_GPIO_READ_REG) >> index)&0x1;
}

int gpio_request(unsigned gpio, const char *label)
{
	return 0;
}

int gpio_free(unsigned gpio)
{
	return 0;
}

int gpio_irq_state(unsigned gpio)
{
    int reg_addr = gpio&0xffff0000;
    int index = gpio&0xffff;
    if(reg_addr==0)
    {
        printf("no gpio group \n");
    }
    return (readl(reg_addr+RK_GPIO_INT_STATUS) >> index)&0x1;
}

int gpio_irq_clr(unsigned gpio)
{
    int reg_addr = gpio&0xffff0000;
    int index = gpio&0xffff;
    if(reg_addr==0)
    {
        printf("no gpio group \n");
        return -1;
    }
    writel(readl(reg_addr+RK_GPIO_INT_EOI)|(1ul<<index), reg_addr+RK_GPIO_INT_EOI);

    return 0;
}

int gpio_irq_request(unsigned gpio, int type)
{
    int reg_addr = gpio&0xffff0000;
    int index = gpio&0xffff;
    if(reg_addr==0)
    {
        printf("no gpio group 0x%x\n",gpio);
        return -1;
    }
    //printf("%s,reg_addr=0x%x,index=%d,type=%d,addr = %x\n",__func__,reg_addr,index,type,reg_addr+RK_GPIO_DEBOUNCE_REG);
    writel(readl(reg_addr+RK_GPIO_DIR_REG)&(~(1ul<<index)), reg_addr+RK_GPIO_DIR_REG);   //input
	writel(readl(reg_addr+RK_GPIO_INT_MASK)&(~(1ul<<index)), reg_addr+RK_GPIO_INT_MASK); //int mask
    writel(readl(reg_addr+RK_GPIO_DEBOUNCE_REG)|(1ul<<index), reg_addr+RK_GPIO_DEBOUNCE_REG);  //debounce
 
    if((type & IRQ_TYPE_EDGE_RISING) || (type&IRQ_TYPE_EDGE_FALLING))
        writel(readl(reg_addr+RK_GPIO_INT_LEVEL)|(1ul<<index), reg_addr+RK_GPIO_INT_LEVEL);   //use edge sensitive
    else if((type & IRQ_TYPE_LEVEL_HIGH) || (type&IRQ_TYPE_LEVEL_LOW))
        writel(readl(reg_addr+RK_GPIO_INT_LEVEL)&(~(1ul<<index)), reg_addr+RK_GPIO_INT_LEVEL); //use level 

    if((type & IRQ_TYPE_EDGE_RISING) || (type&IRQ_TYPE_LEVEL_HIGH))
        writel(readl(reg_addr+RK_GPIO_INT_POLARITY)|(1ul<<index), reg_addr+RK_GPIO_INT_POLARITY);		
	else if((type & IRQ_TYPE_EDGE_FALLING) || (type&IRQ_TYPE_LEVEL_LOW))
		writel(readl(reg_addr+RK_GPIO_INT_POLARITY)&(~(1ul<<index)), reg_addr+RK_GPIO_INT_POLARITY);

    if(type)
	    writel(readl(reg_addr+RK_GPIO_INT_EN)|(1ul<<index), reg_addr+RK_GPIO_INT_EN);          //enable int
    else writel(readl(reg_addr+RK_GPIO_INT_EN)&(~(1ul<<index)), reg_addr+RK_GPIO_INT_EN);
    return 0;
}

int gpio_direction_input(unsigned gpio)
{
	int reg_addr = gpio&0xffff0000;
    int index = gpio&0xffff;
    if(reg_addr==0)
    {
        printf("no gpio group 0x%x\n",gpio);
        return -1;
    }
    writel(readl(reg_addr+RK_GPIO_DIR_REG)&(~(1ul<<index)), reg_addr+RK_GPIO_DIR_REG);
   // writel(reg_addr+RK_GPIO_DEBOUNCE_REG,  readl(reg_addr+RK_GPIO_DEBOUNCE_REG)|(1ul<<index));
    return 0;
}

int gpio_direction_output(unsigned gpio, int value)
{
    int reg_addr = gpio&0xffff0000;
    int index = gpio&0xffff;
    if(reg_addr==0)
    {
        printf("no gpio group 0x%x\n",gpio);
        return -1;
    }
    writel(readl(reg_addr+RK_GPIO_DIR_REG)|(1ul<<index), reg_addr+RK_GPIO_DIR_REG);
	return gpio_set_value(gpio, value);
}
