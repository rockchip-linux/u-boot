/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * (C) Copyright 2025 Rockchip Electronics Co., Ltd.
 */
#ifndef _ASM_ARCH_IOC_RK3572_H
#define _ASM_ARCH_IOC_RK3572_H

struct rk3572_gpio0_ioc_reg {
     uint32_t gpio0a_iomux_sel_0;                 /* address offset: 0x0000 */
     uint32_t gpio0a_iomux_sel_1;                 /* address offset: 0x0004 */
     uint32_t gpio0b_iomux_sel_0;                 /* address offset: 0x0008 */
     uint32_t reserved000c[61];                   /* address offset: 0x000c */
     uint32_t gpio0a_ds_0;                        /* address offset: 0x0100 */
     uint32_t gpio0a_ds_1;                        /* address offset: 0x0104 */
     uint32_t gpio0b_ds_0;                        /* address offset: 0x0108 */
     uint32_t reserved010c[61];                   /* address offset: 0x010c */
     uint32_t gpio0a_pull;                        /* address offset: 0x0200 */
     uint32_t gpio0b_pull_0;                      /* address offset: 0x0204 */
     uint32_t reserved0208[62];                   /* address offset: 0x0208 */
     uint32_t gpio0a_ie;                          /* address offset: 0x0300 */
     uint32_t gpio0b_ie_0;                        /* address offset: 0x0304 */
     uint32_t reserved0308[62];                   /* address offset: 0x0308 */
     uint32_t gpio0a_smt;                         /* address offset: 0x0400 */
     uint32_t gpio0b_smt_0;                       /* address offset: 0x0404 */
     uint32_t reserved0408[62];                   /* address offset: 0x0408 */
     uint32_t gpio0a_pdis;                        /* address offset: 0x0500 */
     uint32_t gpio0b_pdis_0;                      /* address offset: 0x0504 */
     uint32_t reserved0508[62];                   /* address offset: 0x0508 */
     uint32_t osc_con;                            /* address offset: 0x0600 */
     uint32_t reserved0604[1666];                 /* address offset: 0x0604 */
     uint32_t gpio0b_iomux_sel_1;                 /* address offset: 0x200c */
     uint32_t gpio0c_iomux_sel_0;                 /* address offset: 0x2010 */
     uint32_t gpio0c_iomux_sel_1;                 /* address offset: 0x2014 */
     uint32_t gpio0d_iomux_sel_0;                 /* address offset: 0x2018 */
     uint32_t gpio0d_iomux_sel_1;                 /* address offset: 0x201c */
     uint32_t reserved2020[59];                   /* address offset: 0x2020 */
     uint32_t gpio0b_ds_1;                        /* address offset: 0x210c */
     uint32_t gpio0c_ds_0;                        /* address offset: 0x2110 */
     uint32_t gpio0c_ds_1;                        /* address offset: 0x2114 */
     uint32_t gpio0d_ds_0;                        /* address offset: 0x2118 */
     uint32_t gpio0d_ds_1;                        /* address offset: 0x211c */
     uint32_t reserved2120[57];                   /* address offset: 0x2120 */
     uint32_t gpio0b_pull_1;                      /* address offset: 0x2204 */
     uint32_t gpio0c_pull;                        /* address offset: 0x2208 */
     uint32_t gpio0d_pull;                        /* address offset: 0x220c */
     uint32_t reserved2210[61];                   /* address offset: 0x2210 */
     uint32_t gpio0b_ie_1;                        /* address offset: 0x2304 */
     uint32_t gpio0c_ie;                          /* address offset: 0x2308 */
     uint32_t gpio0d_ie;                          /* address offset: 0x230c */
     uint32_t reserved2310[61];                   /* address offset: 0x2310 */
     uint32_t gpio0b_smt_1;                       /* address offset: 0x2404 */
     uint32_t gpio0c_smt;                         /* address offset: 0x2408 */
     uint32_t gpio0d_smt;                         /* address offset: 0x240c */
     uint32_t reserved2410[61];                   /* address offset: 0x2410 */
     uint32_t gpio0b_pdis_1;                      /* address offset: 0x2504 */
     uint32_t gpio0c_pdis;                        /* address offset: 0x2508 */
     uint32_t gpio0d_pdis;                        /* address offset: 0x250c */
};

check_member(rk3572_gpio0_ioc_reg, gpio0d_pdis, 0x250c);

/* gpio1_ioc register structure define */
struct rk3572_gpio1_ioc_reg {
     uint32_t reserved0000[8];                    /* address offset: 0x0000 */
     uint32_t gpio1a_iomux_sel_0;                 /* address offset: 0x0020 */
     uint32_t gpio1a_iomux_sel_1;                 /* address offset: 0x0024 */
     uint32_t gpio1b_iomux_sel_0;                 /* address offset: 0x0028 */
     uint32_t gpio1b_iomux_sel_1;                 /* address offset: 0x002c */
     uint32_t gpio1c_iomux_sel_0;                 /* address offset: 0x0030 */
     uint32_t gpio1c_iomux_sel_1;                 /* address offset: 0x0034 */
     uint32_t gpio1d_iomux_sel_0;                 /* address offset: 0x0038 */
     uint32_t gpio1d_iomux_sel_1;                 /* address offset: 0x003c */
     uint32_t reserved0040[56];                   /* address offset: 0x0040 */
     uint32_t gpio1a_ds_0;                        /* address offset: 0x0120 */
     uint32_t gpio1a_ds_1;                        /* address offset: 0x0124 */
     uint32_t gpio1b_ds_0;                        /* address offset: 0x0128 */
     uint32_t gpio1b_ds_1;                        /* address offset: 0x012c */
     uint32_t gpio1c_ds_0;                        /* address offset: 0x0130 */
     uint32_t gpio1c_ds_1;                        /* address offset: 0x0134 */
     uint32_t gpio1d_ds_0;                        /* address offset: 0x0138 */
     uint32_t gpio1d_ds_1;                        /* address offset: 0x013c */
     uint32_t reserved0140[52];                   /* address offset: 0x0140 */
     uint32_t gpio1a_pull;                        /* address offset: 0x0210 */
     uint32_t gpio1b_pull;                        /* address offset: 0x0214 */
     uint32_t gpio1c_pull;                        /* address offset: 0x0218 */
     uint32_t gpio1d_pull;                        /* address offset: 0x021c */
     uint32_t reserved0220[60];                   /* address offset: 0x0220 */
     uint32_t gpio1a_ie;                          /* address offset: 0x0310 */
     uint32_t gpio1b_ie;                          /* address offset: 0x0314 */
     uint32_t gpio1c_ie;                          /* address offset: 0x0318 */
     uint32_t gpio1d_ie;                          /* address offset: 0x031c */
     uint32_t reserved0320[60];                   /* address offset: 0x0320 */
     uint32_t gpio1a_smt;                         /* address offset: 0x0410 */
     uint32_t gpio1b_smt;                         /* address offset: 0x0414 */
     uint32_t gpio1c_smt;                         /* address offset: 0x0418 */
     uint32_t gpio1d_smt;                         /* address offset: 0x041c */
     uint32_t reserved0420[60];                   /* address offset: 0x0420 */
     uint32_t gpio1a_pdis;                        /* address offset: 0x0510 */
     uint32_t gpio1b_pdis;                        /* address offset: 0x0514 */
     uint32_t gpio1c_pdis;                        /* address offset: 0x0518 */
     uint32_t gpio1d_pdis;                        /* address offset: 0x051c */
     uint32_t reserved0520[56];                   /* address offset: 0x0520 */
     uint32_t vo_ioc_st;                          /* address offset: 0x0600 */
     uint32_t vo_ioc_con0;                        /* address offset: 0x0604 */
     uint32_t vo_ioc_con1;                        /* address offset: 0x0608 */
     uint32_t vo_ioc_con2;                        /* address offset: 0x060c */
};

check_member(rk3572_gpio1_ioc_reg, vo_ioc_con2, 0x060c);

/* gpio2_ioc register structure define */
struct rk3572_gpio2_ioc_reg {
     uint32_t reserved0000[16];                   /* address offset: 0x0000 */
     uint32_t gpio2a_iomux_sel_0;                 /* address offset: 0x0040 */
     uint32_t gpio2a_iomux_sel_1;                 /* address offset: 0x0044 */
     uint32_t gpio2b_iomux_sel_0;                 /* address offset: 0x0048 */
     uint32_t gpio2b_iomux_sel_1;                 /* address offset: 0x004c */
     uint32_t gpio2c_iomux_sel_0;                 /* address offset: 0x0050 */
     uint32_t gpio2c_iomux_sel_1;                 /* address offset: 0x0054 */
     uint32_t gpio2d_iomux_sel_0;                 /* address offset: 0x0058 */
     uint32_t gpio2d_iomux_sel_1;                 /* address offset: 0x005c */
     uint32_t reserved0060[56];                   /* address offset: 0x0060 */
     uint32_t gpio2a_ds_0;                        /* address offset: 0x0140 */
     uint32_t gpio2a_ds_1;                        /* address offset: 0x0144 */
     uint32_t gpio2b_ds_0;                        /* address offset: 0x0148 */
     uint32_t gpio2b_ds_1;                        /* address offset: 0x014c */
     uint32_t gpio2c_ds_0;                        /* address offset: 0x0150 */
     uint32_t gpio2c_ds_1;                        /* address offset: 0x0154 */
     uint32_t gpio2d_ds_0;                        /* address offset: 0x0158 */
     uint32_t gpio2d_ds_1;                        /* address offset: 0x015c */
     uint32_t reserved0160[48];                   /* address offset: 0x0160 */
     uint32_t gpio2a_pull;                        /* address offset: 0x0220 */
     uint32_t gpio2b_pull;                        /* address offset: 0x0224 */
     uint32_t gpio2c_pull;                        /* address offset: 0x0228 */
     uint32_t gpio2d_pull;                        /* address offset: 0x022c */
     uint32_t reserved0230[60];                   /* address offset: 0x0230 */
     uint32_t gpio2a_ie;                          /* address offset: 0x0320 */
     uint32_t gpio2b_ie;                          /* address offset: 0x0324 */
     uint32_t gpio2c_ie;                          /* address offset: 0x0328 */
     uint32_t gpio2d_ie;                          /* address offset: 0x032c */
     uint32_t reserved0330[60];                   /* address offset: 0x0330 */
     uint32_t gpio2a_smt;                         /* address offset: 0x0420 */
     uint32_t gpio2b_smt;                         /* address offset: 0x0424 */
     uint32_t gpio2c_smt;                         /* address offset: 0x0428 */
     uint32_t gpio2d_smt;                         /* address offset: 0x042c */
     uint32_t reserved0430[60];                   /* address offset: 0x0430 */
     uint32_t gpio2a_pdis;                        /* address offset: 0x0520 */
     uint32_t gpio2b_pdis;                        /* address offset: 0x0524 */
     uint32_t gpio2c_pdis;                        /* address offset: 0x0528 */
     uint32_t gpio2d_pdis;                        /* address offset: 0x052c */
     uint32_t reserved0530[52];                   /* address offset: 0x0530 */
     uint32_t vi_ioc_con0;                        /* address offset: 0x0600 */
     uint32_t vi_ioc_con1;                        /* address offset: 0x0604 */
     uint32_t vi_ioc_con2;                        /* address offset: 0x0608 */
     uint32_t vi_ioc_con3;                        /* address offset: 0x060c */
};

check_member(rk3572_gpio2_ioc_reg, vi_ioc_con3, 0x060c);

/* gpio3_ioc register structure define */
struct rk3572_gpio3_ioc_reg {
     uint32_t reserved0000[25];                   /* address offset: 0x0000 */
     uint32_t gpio3a_iomux_sel_1;                 /* address offset: 0x0064 */
     uint32_t gpio3b_iomux_sel_0;                 /* address offset: 0x0068 */
     uint32_t gpio3b_iomux_sel_1;                 /* address offset: 0x006c */
     uint32_t gpio3c_iomux_sel_0;                 /* address offset: 0x0070 */
     uint32_t gpio3c_iomux_sel_1;                 /* address offset: 0x0074 */
     uint32_t gpio3d_iomux_sel_0;                 /* address offset: 0x0078 */
     uint32_t gpio3d_iomux_sel_1;                 /* address offset: 0x007c */
     uint32_t reserved0080[57];                   /* address offset: 0x0080 */
     uint32_t gpio3a_ds_1;                        /* address offset: 0x0164 */
     uint32_t gpio3b_ds_0;                        /* address offset: 0x0168 */
     uint32_t gpio3b_ds_1;                        /* address offset: 0x016c */
     uint32_t gpio3c_ds_0;                        /* address offset: 0x0170 */
     uint32_t gpio3c_ds_1;                        /* address offset: 0x0174 */
     uint32_t gpio3d_ds_0;                        /* address offset: 0x0178 */
     uint32_t gpio3d_ds_1;                        /* address offset: 0x017c */
     uint32_t gpio4a_ds_0;                        /* address offset: 0x0180 */
     uint32_t reserved0184[43];                   /* address offset: 0x0184 */
     uint32_t gpio3a_pull;                        /* address offset: 0x0230 */
     uint32_t gpio3b_pull;                        /* address offset: 0x0234 */
     uint32_t gpio3c_pull;                        /* address offset: 0x0238 */
     uint32_t gpio3d_pull;                        /* address offset: 0x023c */
     uint32_t reserved0240[60];                   /* address offset: 0x0240 */
     uint32_t gpio3a_ie;                          /* address offset: 0x0330 */
     uint32_t gpio3b_ie;                          /* address offset: 0x0334 */
     uint32_t gpio3c_ie;                          /* address offset: 0x0338 */
     uint32_t gpio3d_ie;                          /* address offset: 0x033c */
     uint32_t reserved0340[60];                   /* address offset: 0x0340 */
     uint32_t gpio3a_smt;                         /* address offset: 0x0430 */
     uint32_t gpio3b_smt;                         /* address offset: 0x0434 */
     uint32_t gpio3c_smt;                         /* address offset: 0x0438 */
     uint32_t gpio3d_smt;                         /* address offset: 0x043c */
     uint32_t reserved0440[60];                   /* address offset: 0x0440 */
     uint32_t gpio3a_pdis;                        /* address offset: 0x0530 */
     uint32_t gpio3b_pdis;                        /* address offset: 0x0534 */
     uint32_t gpio3c_pdis;                        /* address offset: 0x0538 */
     uint32_t gpio3d_pdis;                        /* address offset: 0x053c */
     uint32_t reserved0540[60];                   /* address offset: 0x0540 */
     uint32_t misc0;                              /* address offset: 0x0630 */
     uint32_t misc1;                              /* address offset: 0x0634 */
     uint32_t misc2;                              /* address offset: 0x0638 */
     uint32_t misc3;                              /* address offset: 0x063c */
};

check_member(rk3572_gpio3_ioc_reg, misc3, 0x063c);

/* gpio4_ioc register structure define */
struct rk3572_gpio4_ioc_reg {
     uint32_t reserved0000[36];                   /* address offset: 0x0000 */
     uint32_t gpio4c_iomux_sel_0;                 /* address offset: 0x0090 */
     uint32_t reserved0094[63];                   /* address offset: 0x0094 */
     uint32_t gpio4c_ds_0;                        /* address offset: 0x0190 */
     uint32_t reserved0194[45];                   /* address offset: 0x0194 */
     uint32_t gpio4c_pull;                        /* address offset: 0x0248 */
     uint32_t reserved024c[63];                   /* address offset: 0x024c */
     uint32_t gpio4c_ie;                          /* address offset: 0x0348 */
     uint32_t reserved034c[63];                   /* address offset: 0x034c */
     uint32_t gpio4c_smt;                         /* address offset: 0x0448 */
     uint32_t reserved044c[63];                   /* address offset: 0x044c */
     uint32_t gpio4c_pdis;                        /* address offset: 0x0548 */
     uint32_t reserved054c[16077];                /* address offset: 0x054c */
     uint32_t gpio4a_iomux_sel_0;                 /* address offset: 0x10080 */
     uint32_t reserved10084;                      /* address offset: 0x10084 */
     uint32_t gpio4b_iomux_sel_0;                 /* address offset: 0x10088 */
     uint32_t gpio4b_iomux_sel_1;                 /* address offset: 0x1008c */
     uint32_t reserved10090[62];                  /* address offset: 0x10090 */
     uint32_t gpio4b_ds_0;                        /* address offset: 0x10188 */
     uint32_t gpio4b_ds_1;                        /* address offset: 0x1018c */
     uint32_t reserved10190[44];                  /* address offset: 0x10190 */
     uint32_t gpio4a_pull;                        /* address offset: 0x10240 */
     uint32_t gpio4b_pull;                        /* address offset: 0x10244 */
     uint32_t reserved10248[62];                  /* address offset: 0x10248 */
     uint32_t gpio4a_ie;                          /* address offset: 0x10340 */
     uint32_t gpio4b_ie;                          /* address offset: 0x10344 */
     uint32_t reserved10348[62];                  /* address offset: 0x10348 */
     uint32_t gpio4a_smt;                         /* address offset: 0x10440 */
     uint32_t gpio4b_smt;                         /* address offset: 0x10444 */
     uint32_t reserved10448[62];                  /* address offset: 0x10448 */
     uint32_t gpio4a_pdis;                        /* address offset: 0x10540 */
     uint32_t gpio4b_pdis;                        /* address offset: 0x10544 */
};

check_member(rk3572_gpio4_ioc_reg, gpio4b_pdis, 0x10544);

/* pmuio0_ioc register structure define */
struct rk3572_pmuio0_ioc_reg {
     uint32_t gpio0a_iomux_sel_0;                 /* address offset: 0x0000 */
     uint32_t gpio0a_iomux_sel_1;                 /* address offset: 0x0004 */
     uint32_t gpio0b_iomux_sel_0;                 /* address offset: 0x0008 */
     uint32_t reserved000c[61];                   /* address offset: 0x000c */
     uint32_t gpio0a_ds_0;                        /* address offset: 0x0100 */
     uint32_t gpio0a_ds_1;                        /* address offset: 0x0104 */
     uint32_t gpio0b_ds_0;                        /* address offset: 0x0108 */
     uint32_t reserved010c[61];                   /* address offset: 0x010c */
     uint32_t gpio0a_pull;                        /* address offset: 0x0200 */
     uint32_t gpio0b_pull;                        /* address offset: 0x0204 */
     uint32_t reserved0208[62];                   /* address offset: 0x0208 */
     uint32_t gpio0a_ie;                          /* address offset: 0x0300 */
     uint32_t gpio0b_ie;                          /* address offset: 0x0304 */
     uint32_t reserved0308[62];                   /* address offset: 0x0308 */
     uint32_t gpio0a_smt;                         /* address offset: 0x0400 */
     uint32_t gpio0b_smt;                         /* address offset: 0x0404 */
     uint32_t reserved0408[62];                   /* address offset: 0x0408 */
     uint32_t gpio0a_pdis;                        /* address offset: 0x0500 */
     uint32_t gpio0b_pdis;                        /* address offset: 0x0504 */
     uint32_t reserved0508[62];                   /* address offset: 0x0508 */
     uint32_t osc_con;                            /* address offset: 0x0600 */
};

check_member(rk3572_pmuio0_ioc_reg, osc_con, 0x0600);

/* pmuio1_ioc register structure define */
struct rk3572_pmuio1_ioc_reg {
     uint32_t reserved0000[3];                    /* address offset: 0x0000 */
     uint32_t gpio0b_iomux_sel_1;                 /* address offset: 0x000c */
     uint32_t gpio0c_iomux_sel_0;                 /* address offset: 0x0010 */
     uint32_t gpio0c_iomux_sel_1;                 /* address offset: 0x0014 */
     uint32_t gpio0d_iomux_sel_0;                 /* address offset: 0x0018 */
     uint32_t gpio0d_iomux_sel_1;                 /* address offset: 0x001c */
     uint32_t reserved0020[59];                   /* address offset: 0x0020 */
     uint32_t gpio0b_ds_1;                        /* address offset: 0x010c */
     uint32_t gpio0c_ds_0;                        /* address offset: 0x0110 */
     uint32_t gpio0c_ds_1;                        /* address offset: 0x0114 */
     uint32_t gpio0d_ds_0;                        /* address offset: 0x0118 */
     uint32_t gpio0d_ds_1;                        /* address offset: 0x011c */
     uint32_t reserved0120[57];                   /* address offset: 0x0120 */
     uint32_t gpio0b_pull;                        /* address offset: 0x0204 */
     uint32_t gpio0c_pull;                        /* address offset: 0x0208 */
     uint32_t gpio0d_pull;                        /* address offset: 0x020c */
     uint32_t reserved0210[61];                   /* address offset: 0x0210 */
     uint32_t gpio0b_ie;                          /* address offset: 0x0304 */
     uint32_t gpio0c_ie;                          /* address offset: 0x0308 */
     uint32_t gpio0d_ie;                          /* address offset: 0x030c */
     uint32_t reserved0310[61];                   /* address offset: 0x0310 */
     uint32_t gpio0b_smt;                         /* address offset: 0x0404 */
     uint32_t gpio0c_smt;                         /* address offset: 0x0408 */
     uint32_t gpio0d_smt;                         /* address offset: 0x040c */
     uint32_t reserved0410[61];                   /* address offset: 0x0410 */
     uint32_t gpio0b_pdis;                        /* address offset: 0x0504 */
     uint32_t gpio0c_pdis;                        /* address offset: 0x0508 */
     uint32_t gpio0d_pdis;                        /* address offset: 0x050c */
};

check_member(rk3572_pmuio1_ioc_reg, gpio0d_pdis, 0x050c);

/* vccio0_3_ioc register structure define */
struct rk3572_vccio0_3_ioc_reg {
     uint32_t reserved0000[8];                    /* address offset: 0x0000 */
     uint32_t gpio1a_iomux_sel_0;                 /* address offset: 0x0020 */
     uint32_t gpio1a_iomux_sel_1;                 /* address offset: 0x0024 */
     uint32_t gpio1b_iomux_sel_0;                 /* address offset: 0x0028 */
     uint32_t gpio1b_iomux_sel_1;                 /* address offset: 0x002c */
     uint32_t gpio1c_iomux_sel_0;                 /* address offset: 0x0030 */
     uint32_t gpio1c_iomux_sel_1;                 /* address offset: 0x0034 */
     uint32_t gpio1d_iomux_sel_0;                 /* address offset: 0x0038 */
     uint32_t gpio1d_iomux_sel_1;                 /* address offset: 0x003c */
     uint32_t reserved0040[56];                   /* address offset: 0x0040 */
     uint32_t gpio1a_ds_0;                        /* address offset: 0x0120 */
     uint32_t gpio1a_ds_1;                        /* address offset: 0x0124 */
     uint32_t gpio1b_ds_0;                        /* address offset: 0x0128 */
     uint32_t gpio1b_ds_1;                        /* address offset: 0x012c */
     uint32_t gpio1c_ds_0;                        /* address offset: 0x0130 */
     uint32_t gpio1c_ds_1;                        /* address offset: 0x0134 */
     uint32_t gpio1d_ds_0;                        /* address offset: 0x0138 */
     uint32_t gpio1d_ds_1;                        /* address offset: 0x013c */
     uint32_t reserved0140[52];                   /* address offset: 0x0140 */
     uint32_t gpio1a_pull;                        /* address offset: 0x0210 */
     uint32_t gpio1b_pull;                        /* address offset: 0x0214 */
     uint32_t gpio1c_pull;                        /* address offset: 0x0218 */
     uint32_t gpio1d_pull;                        /* address offset: 0x021c */
     uint32_t reserved0220[60];                   /* address offset: 0x0220 */
     uint32_t gpio1a_ie;                          /* address offset: 0x0310 */
     uint32_t gpio1b_ie;                          /* address offset: 0x0314 */
     uint32_t gpio1c_ie;                          /* address offset: 0x0318 */
     uint32_t gpio1d_ie;                          /* address offset: 0x031c */
     uint32_t reserved0320[60];                   /* address offset: 0x0320 */
     uint32_t gpio1a_smt;                         /* address offset: 0x0410 */
     uint32_t gpio1b_smt;                         /* address offset: 0x0414 */
     uint32_t gpio1c_smt;                         /* address offset: 0x0418 */
     uint32_t gpio1d_smt;                         /* address offset: 0x041c */
     uint32_t reserved0420[60];                   /* address offset: 0x0420 */
     uint32_t gpio1a_pdis;                        /* address offset: 0x0510 */
     uint32_t gpio1b_pdis;                        /* address offset: 0x0514 */
     uint32_t gpio1c_pdis;                        /* address offset: 0x0518 */
     uint32_t gpio1d_pdis;                        /* address offset: 0x051c */
     uint32_t reserved0520[56];                   /* address offset: 0x0520 */
     uint32_t vo_ioc_st;                          /* address offset: 0x0600 */
     uint32_t vo_ioc_con0;                        /* address offset: 0x0604 */
     uint32_t vo_ioc_con1;                        /* address offset: 0x0608 */
     uint32_t vo_ioc_con2;                        /* address offset: 0x060c */
};

check_member(rk3572_vccio0_3_ioc_reg, vo_ioc_con2, 0x060c);

/* vccio1_2_4_ioc register structure define */
struct rk3572_vccio1_2_4_ioc_reg {
     uint32_t reserved0000[16];                   /* address offset: 0x0000 */
     uint32_t gpio2a_iomux_sel_0;                 /* address offset: 0x0040 */
     uint32_t gpio2a_iomux_sel_1;                 /* address offset: 0x0044 */
     uint32_t gpio2b_iomux_sel_0;                 /* address offset: 0x0048 */
     uint32_t gpio2b_iomux_sel_1;                 /* address offset: 0x004c */
     uint32_t gpio2c_iomux_sel_0;                 /* address offset: 0x0050 */
     uint32_t gpio2c_iomux_sel_1;                 /* address offset: 0x0054 */
     uint32_t gpio2d_iomux_sel_0;                 /* address offset: 0x0058 */
     uint32_t gpio2d_iomux_sel_1;                 /* address offset: 0x005c */
     uint32_t reserved0060[56];                   /* address offset: 0x0060 */
     uint32_t gpio2a_ds_0;                        /* address offset: 0x0140 */
     uint32_t gpio2a_ds_1;                        /* address offset: 0x0144 */
     uint32_t gpio2b_ds_0;                        /* address offset: 0x0148 */
     uint32_t gpio2b_ds_1;                        /* address offset: 0x014c */
     uint32_t gpio2c_ds_0;                        /* address offset: 0x0150 */
     uint32_t gpio2c_ds_1;                        /* address offset: 0x0154 */
     uint32_t gpio2d_ds_0;                        /* address offset: 0x0158 */
     uint32_t gpio2d_ds_1;                        /* address offset: 0x015c */
     uint32_t reserved0160[48];                   /* address offset: 0x0160 */
     uint32_t gpio2a_pull;                        /* address offset: 0x0220 */
     uint32_t gpio2b_pull;                        /* address offset: 0x0224 */
     uint32_t gpio2c_pull;                        /* address offset: 0x0228 */
     uint32_t gpio2d_pull;                        /* address offset: 0x022c */
     uint32_t reserved0230[60];                   /* address offset: 0x0230 */
     uint32_t gpio2a_ie;                          /* address offset: 0x0320 */
     uint32_t gpio2b_ie;                          /* address offset: 0x0324 */
     uint32_t gpio2c_ie;                          /* address offset: 0x0328 */
     uint32_t gpio2d_ie;                          /* address offset: 0x032c */
     uint32_t reserved0330[60];                   /* address offset: 0x0330 */
     uint32_t gpio2a_smt;                         /* address offset: 0x0420 */
     uint32_t gpio2b_smt;                         /* address offset: 0x0424 */
     uint32_t gpio2c_smt;                         /* address offset: 0x0428 */
     uint32_t gpio2d_smt;                         /* address offset: 0x042c */
     uint32_t reserved0430[60];                   /* address offset: 0x0430 */
     uint32_t gpio2a_pdis;                        /* address offset: 0x0520 */
     uint32_t gpio2b_pdis;                        /* address offset: 0x0524 */
     uint32_t gpio2c_pdis;                        /* address offset: 0x0528 */
     uint32_t gpio2d_pdis;                        /* address offset: 0x052c */
     uint32_t reserved0530[52];                   /* address offset: 0x0530 */
     uint32_t vi_ioc_con0;                        /* address offset: 0x0600 */
     uint32_t vi_ioc_con1;                        /* address offset: 0x0604 */
     uint32_t vi_ioc_con2;                        /* address offset: 0x0608 */
     uint32_t vi_ioc_con3;                        /* address offset: 0x060c */
};

check_member(rk3572_vccio1_2_4_ioc_reg, vi_ioc_con3, 0x060c);

/* vccio5_6_ioc register structure define */
struct rk3572_vccio5_6_ioc_reg {
     uint32_t reserved0000[25];                   /* address offset: 0x0000 */
     uint32_t gpio3a_iomux_sel_1;                 /* address offset: 0x0064 */
     uint32_t gpio3b_iomux_sel_0;                 /* address offset: 0x0068 */
     uint32_t gpio3b_iomux_sel_1;                 /* address offset: 0x006c */
     uint32_t gpio3c_iomux_sel_0;                 /* address offset: 0x0070 */
     uint32_t gpio3c_iomux_sel_1;                 /* address offset: 0x0074 */
     uint32_t gpio3d_iomux_sel_0;                 /* address offset: 0x0078 */
     uint32_t gpio3d_iomux_sel_1;                 /* address offset: 0x007c */
     uint32_t gpio4a_iomux_sel_0;                 /* address offset: 0x0080 */
     uint32_t reserved0084;                       /* address offset: 0x0084 */
     uint32_t gpio4b_iomux_sel_0;                 /* address offset: 0x0088 */
     uint32_t gpio4b_iomux_sel_1;                 /* address offset: 0x008c */
     uint32_t reserved0090[53];                   /* address offset: 0x0090 */
     uint32_t gpio3a_ds_1;                        /* address offset: 0x0164 */
     uint32_t gpio3b_ds_0;                        /* address offset: 0x0168 */
     uint32_t gpio3b_ds_1;                        /* address offset: 0x016c */
     uint32_t gpio3c_ds_0;                        /* address offset: 0x0170 */
     uint32_t gpio3c_ds_1;                        /* address offset: 0x0174 */
     uint32_t gpio3d_ds_0;                        /* address offset: 0x0178 */
     uint32_t gpio3d_ds_1;                        /* address offset: 0x017c */
     uint32_t gpio4a_ds_0;                        /* address offset: 0x0180 */
     uint32_t reserved0184;                       /* address offset: 0x0184 */
     uint32_t gpio4b_ds_0;                        /* address offset: 0x0188 */
     uint32_t gpio4b_ds_1;                        /* address offset: 0x018c */
     uint32_t reserved0190[40];                   /* address offset: 0x0190 */
     uint32_t gpio3a_pull;                        /* address offset: 0x0230 */
     uint32_t gpio3b_pull;                        /* address offset: 0x0234 */
     uint32_t gpio3c_pull;                        /* address offset: 0x0238 */
     uint32_t gpio3d_pull;                        /* address offset: 0x023c */
     uint32_t gpio4a_pull;                        /* address offset: 0x0240 */
     uint32_t gpio4b_pull;                        /* address offset: 0x0244 */
     uint32_t reserved0248[58];                   /* address offset: 0x0248 */
     uint32_t gpio3a_ie;                          /* address offset: 0x0330 */
     uint32_t gpio3b_ie;                          /* address offset: 0x0334 */
     uint32_t gpio3c_ie;                          /* address offset: 0x0338 */
     uint32_t gpio3d_ie;                          /* address offset: 0x033c */
     uint32_t gpio4a_ie;                          /* address offset: 0x0340 */
     uint32_t gpio4b_ie;                          /* address offset: 0x0344 */
     uint32_t reserved0348[58];                   /* address offset: 0x0348 */
     uint32_t gpio3a_smt;                         /* address offset: 0x0430 */
     uint32_t gpio3b_smt;                         /* address offset: 0x0434 */
     uint32_t gpio3c_smt;                         /* address offset: 0x0438 */
     uint32_t gpio3d_smt;                         /* address offset: 0x043c */
     uint32_t gpio4a_smt;                         /* address offset: 0x0440 */
     uint32_t gpio4b_smt;                         /* address offset: 0x0444 */
     uint32_t reserved0448[58];                   /* address offset: 0x0448 */
     uint32_t gpio3a_pdis;                        /* address offset: 0x0530 */
     uint32_t gpio3b_pdis;                        /* address offset: 0x0534 */
     uint32_t gpio3c_pdis;                        /* address offset: 0x0538 */
     uint32_t gpio3d_pdis;                        /* address offset: 0x053c */
     uint32_t gpio4a_pdis;                        /* address offset: 0x0540 */
     uint32_t gpio4b_pdis;                        /* address offset: 0x0544 */
     uint32_t reserved0548[58];                   /* address offset: 0x0548 */
     uint32_t misc0;                              /* address offset: 0x0630 */
     uint32_t misc1;                              /* address offset: 0x0634 */
     uint32_t misc2;                              /* address offset: 0x0638 */
     uint32_t misc3;                              /* address offset: 0x063c */
};

check_member(rk3572_vccio5_6_ioc_reg, misc3, 0x063c);

/* vccio7_ioc register structure define */
struct rk3572_vccio7_ioc_reg {
     uint32_t reserved0000[36];                   /* address offset: 0x0000 */
     uint32_t gpio4c_iomux_sel_0;                 /* address offset: 0x0090 */
     uint32_t reserved0094[63];                   /* address offset: 0x0094 */
     uint32_t gpio4c_ds_0;                        /* address offset: 0x0190 */
     uint32_t reserved0194[45];                   /* address offset: 0x0194 */
     uint32_t gpio4c_pull;                        /* address offset: 0x0248 */
     uint32_t reserved024c[63];                   /* address offset: 0x024c */
     uint32_t gpio4c_ie;                          /* address offset: 0x0348 */
     uint32_t reserved034c[63];                   /* address offset: 0x034c */
     uint32_t gpio4c_smt;                         /* address offset: 0x0448 */
     uint32_t reserved044c[63];                   /* address offset: 0x044c */
     uint32_t gpio4c_pdis;                        /* address offset: 0x0548 */
};

check_member(rk3572_vccio7_ioc_reg, gpio4c_pdis, 0x0548);

#endif /* _ASM_ARCH_IOC_RK3572_H */
