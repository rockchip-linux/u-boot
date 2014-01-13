






#define CONFIG_RK3066           1
#define CONFIG_RK3066B          2
#define CONFIG_RK3168           3
#define CONFIG_RK3188           4
#define CONFIG_RK3188B          5
#define CONFIG_RK3188T          6
#define CONFIG_RK3026           7
#define CONFIG_RK2928           8


#define CONFIG_RKCHIPTYPE           CONFIG_RK3188

#if (CONFIG_RKCHIPTYPE == CONFIG_RK3026)
#define CONFIG_RK_3026_CHIP
#else
#define CONFIG_RK_30XX_CHIP
#endif




