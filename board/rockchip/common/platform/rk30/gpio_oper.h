#ifndef __GPIO_OPER_H__
#define __GPIO_OPER_H__

typedef enum{
    KEY_GPIO = 0,   // IO按键
    KEY_AD,      // AD按键
}KEY_TYPE;

typedef struct
{
    int     group;
    int     index;
    int     valid;

    // IO操作地址
	uint32	io_read;
	uint32  io_write;
	uint32	io_dir_conf;
	uint32	io_debounce;
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

typedef struct {
    KEY_TYPE type;
    union{
        gpio_conf   gpio;   // IO按键
        adc_conf    adc;    // AD按键
    }key;
}key_config;

void boot_gpio_set(void);
int GetPortState(key_config* gpio);
#endif

