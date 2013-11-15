#ifndef __GPIO_OPER_H__
#define __GPIO_OPER_H__
#define LONE_PRESS_TIME		1500//ms
#define KEY_LONG_PRESS			-1
#define KEY_SHORT_PRESS			1
typedef enum{
    KEY_GPIO = 0,   // IO按键
    KEY_AD,      // AD按键
    KEY_INT,
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

typedef struct
{
    int     group;
    int     index;
    int     valid;
	uint32  int_en;
	uint32  int_mask;
	uint32  int_level;
	uint32  int_polarity;
	uint32  int_status;
	uint32  int_eoi;
	uint32	io_read;
	uint32	io_dir_conf;
	uint32  io_debounce;
	volatile uint32  pressed_state;
	volatile uint32  press_time;
}int_conf;

typedef struct {
    KEY_TYPE type;
    union{
        gpio_conf   gpio;   
        adc_conf    adc;    
        int_conf    ioint;    
    }key;
}key_config;

void boot_gpio_set(void);
int GetPortState(key_config* gpio);
#endif

