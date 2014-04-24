#ifndef __GPIO_OPER_H__
#define __GPIO_OPER_H__
#define LONE_PRESS_TIME		1500//ms
#define KEY_LONG_PRESS			-1
#define KEY_SHORT_PRESS			1
typedef enum{
    KEY_AD,      // AD°´¼ü
    KEY_INT,
}KEY_TYPE;

typedef struct
{
    const char *name;   /* name of the fdt property defining this */
    uint gpio;      /* GPIO number, or FDT_GPIO_NONE if none */
    u8 flags;       /* FDT_GPIO_... flags */
}gpio_conf;


typedef struct
{
    uint32  index;
    uint32  keyValueLow;
    uint32  keyValueHigh;
	uint32	data;
	uint32  stas;
	uint32	ctrl;
}adc_conf;

typedef struct
{
    const char *name;   /* name of the fdt property defining this */
    uint gpio;      /* GPIO number, or FDT_GPIO_NONE if none */
    u8 flags;       /* FDT_GPIO_... flags */
	volatile uint32  pressed_state;
	volatile uint32  press_time;
}int_conf;

typedef struct {
    KEY_TYPE type;
    union{ 
        adc_conf    adc;    
        int_conf    ioint;    
    }key;
}key_config;

int checkKey(uint32* boot_rockusb, uint32* boot_recovery, uint32* boot_fastboot);
int power_hold();
void key_init();
int is_charging();
void powerOn(void);
void powerOff(void);

#endif

