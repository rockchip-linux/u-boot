/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * (C) Copyright 2025 Rockchip Electronics Co., Ltd.
 */
#ifndef _ASM_ARCH_IOC_RK3538_H
#define _ASM_ARCH_IOC_RK3538_H

struct rk3538_pmuio0_ioc_reg {
     uint32_t gpio0a_iomux_sel_0;                 /* address offset: 0x0000 */
     uint32_t gpio0a_iomux_sel_1;                 /* address offset: 0x0004 */
     uint32_t gpio0b_iomux_sel_0;                 /* address offset: 0x0008 */
     uint32_t gpio0b_iomux_sel_1;                 /* address offset: 0x000c */
     uint32_t gpio0c_iomux_sel_0;                 /* address offset: 0x0010 */
     uint32_t reserved0014[59];                   /* address offset: 0x0014 */
     uint32_t gpio0a_ds_0;                        /* address offset: 0x0100 */
     uint32_t gpio0a_ds_1;                        /* address offset: 0x0104 */
     uint32_t gpio0a_ds_2;                        /* address offset: 0x0108 */
     uint32_t reserved010c[2];                    /* address offset: 0x010c */
     uint32_t gpio0b_ds_1;                        /* address offset: 0x0114 */
     uint32_t gpio0b_ds_2;                        /* address offset: 0x0118 */
     uint32_t gpio0b_ds_3;                        /* address offset: 0x011c */
     uint32_t gpio0c_ds_0;                        /* address offset: 0x0120 */
     uint32_t reserved0124[119];                  /* address offset: 0x0124 */
     uint32_t gpio0a_pull;                        /* address offset: 0x0300 */
     uint32_t gpio0b_pull;                        /* address offset: 0x0304 */
     uint32_t gpio0c_pull;                        /* address offset: 0x0308 */
     uint32_t reserved030c[61];                   /* address offset: 0x030c */
     uint32_t gpio0a_ie;                          /* address offset: 0x0400 */
     uint32_t gpio0b_ie;                          /* address offset: 0x0404 */
     uint32_t gpio0c_ie;                          /* address offset: 0x0408 */
     uint32_t reserved040c[61];                   /* address offset: 0x040c */
     uint32_t gpio0a_smt;                         /* address offset: 0x0500 */
     uint32_t gpio0b_smt;                         /* address offset: 0x0504 */
     uint32_t gpio0c_smt;                         /* address offset: 0x0508 */
     uint32_t reserved050c[61];                   /* address offset: 0x050c */
     uint32_t gpio0a_sus;                         /* address offset: 0x0600 */
     uint32_t gpio0b_sus;                         /* address offset: 0x0604 */
     uint32_t gpio0c_sus;                         /* address offset: 0x0608 */
     uint32_t reserved060c[61];                   /* address offset: 0x060c */
     uint32_t gpio0a_sl;                          /* address offset: 0x0700 */
     uint32_t gpio0b_sl;                          /* address offset: 0x0704 */
     uint32_t gpio0c_sl;                          /* address offset: 0x0708 */
     uint32_t reserved070c[61];                   /* address offset: 0x070c */
     uint32_t gpio0a_od;                          /* address offset: 0x0800 */
     uint32_t gpio0b_od;                          /* address offset: 0x0804 */
     uint32_t gpio0c_od;                          /* address offset: 0x0808 */
     uint32_t reserved080c[61];                   /* address offset: 0x080c */
     uint32_t 5vio_ctrl0;                         /* address offset: 0x0900 */
     uint32_t 5vio_ctrl1;                         /* address offset: 0x0904 */
     uint32_t reserved0908[62];                   /* address offset: 0x0908 */
     uint32_t io_vsel_pmuio0;                     /* address offset: 0x0a00 */
     uint32_t grf_jtag_con0;                      /* address offset: 0x0a04 */
     uint32_t grf_jtag_con1;                      /* address offset: 0x0a08 */
     uint32_t reserved0a0c;                       /* address offset: 0x0a0c */
     uint32_t xin_con;                            /* address offset: 0x0a10 */
};

check_member(rk3538_pmuio0_ioc_reg, xin_con, 0x0a10);

struct rk3538_pmuio1_ioc_reg {
     uint32_t reserved0000[6];                    /* address offset: 0x0000 */
     uint32_t gpio0d_iomux_sel_0;                 /* address offset: 0x0018 */
     uint32_t gpio0d_iomux_sel_1;                 /* address offset: 0x001c */
     uint32_t reserved0020[68];                   /* address offset: 0x0020 */
     uint32_t gpio0d_ds_0;                        /* address offset: 0x0130 */
     uint32_t gpio0d_ds_1;                        /* address offset: 0x0134 */
     uint32_t gpio0d_ds_2;                        /* address offset: 0x0138 */
     uint32_t reserved013c[116];                  /* address offset: 0x013c */
     uint32_t gpio0d_pull;                        /* address offset: 0x030c */
     uint32_t reserved0310[63];                   /* address offset: 0x0310 */
     uint32_t gpio0d_ie;                          /* address offset: 0x040c */
     uint32_t reserved0410[63];                   /* address offset: 0x0410 */
     uint32_t gpio0d_smt;                         /* address offset: 0x050c */
     uint32_t reserved0510[63];                   /* address offset: 0x0510 */
     uint32_t gpio0d_sus;                         /* address offset: 0x060c */
     uint32_t reserved0610[63];                   /* address offset: 0x0610 */
     uint32_t gpio0d_sl;                          /* address offset: 0x070c */
     uint32_t reserved0710[63];                   /* address offset: 0x0710 */
     uint32_t gpio0d_od;                          /* address offset: 0x080c */
     uint32_t reserved0810[124];                  /* address offset: 0x0810 */
     uint32_t io_vsel_pmuio1;                     /* address offset: 0x0a00 */
};

check_member(rk3538_pmuio1_ioc_reg, io_vsel_pmuio1, 0x0a00);

struct rk3538_vccio1_ioc_reg {
     uint32_t reserved0000[8];                    /* address offset: 0x0000 */
     uint32_t gpio1a_iomux_sel_0;                 /* address offset: 0x0020 */
     uint32_t gpio1a_iomux_sel_1;                 /* address offset: 0x0024 */
     uint32_t gpio1b_iomux_sel_0;                 /* address offset: 0x0028 */
     uint32_t gpio1b_iomux_sel_1;                 /* address offset: 0x002c */
     uint32_t gpio1c_iomux_sel_0;                 /* address offset: 0x0030 */
     uint32_t reserved0034[67];                   /* address offset: 0x0034 */
     uint32_t gpio1a_ds_0;                        /* address offset: 0x0140 */
     uint32_t gpio1a_ds_1;                        /* address offset: 0x0144 */
     uint32_t gpio1a_ds_2;                        /* address offset: 0x0148 */
     uint32_t gpio1a_ds_3;                        /* address offset: 0x014c */
     uint32_t gpio1b_ds_0;                        /* address offset: 0x0150 */
     uint32_t gpio1b_ds_1;                        /* address offset: 0x0154 */
     uint32_t gpio1b_ds_2;                        /* address offset: 0x0158 */
     uint32_t gpio1b_ds_3;                        /* address offset: 0x015c */
     uint32_t gpio1c_ds_0;                        /* address offset: 0x0160 */
     uint32_t reserved0164[107];                  /* address offset: 0x0164 */
     uint32_t gpio1a_pull;                        /* address offset: 0x0310 */
     uint32_t gpio1b_pull;                        /* address offset: 0x0314 */
     uint32_t gpio1c_pull;                        /* address offset: 0x0318 */
     uint32_t reserved031c[61];                   /* address offset: 0x031c */
     uint32_t gpio1a_ie;                          /* address offset: 0x0410 */
     uint32_t gpio1b_ie;                          /* address offset: 0x0414 */
     uint32_t gpio1c_ie;                          /* address offset: 0x0418 */
     uint32_t reserved041c[61];                   /* address offset: 0x041c */
     uint32_t gpio1a_smt;                         /* address offset: 0x0510 */
     uint32_t gpio1b_smt;                         /* address offset: 0x0514 */
     uint32_t gpio1c_smt;                         /* address offset: 0x0518 */
     uint32_t reserved051c[61];                   /* address offset: 0x051c */
     uint32_t gpio1a_sus;                         /* address offset: 0x0610 */
     uint32_t gpio1b_sus;                         /* address offset: 0x0614 */
     uint32_t gpio1c_sus;                         /* address offset: 0x0618 */
     uint32_t reserved061c[61];                   /* address offset: 0x061c */
     uint32_t gpio1a_sl;                          /* address offset: 0x0710 */
     uint32_t gpio1b_sl;                          /* address offset: 0x0714 */
     uint32_t gpio1c_sl;                          /* address offset: 0x0718 */
     uint32_t reserved071c[61];                   /* address offset: 0x071c */
     uint32_t gpio1a_od;                          /* address offset: 0x0810 */
     uint32_t gpio1b_od;                          /* address offset: 0x0814 */
     uint32_t gpio1c_od;                          /* address offset: 0x0818 */
     uint32_t reserved081c[61];                   /* address offset: 0x081c */
     uint32_t gpio1_iddq;                         /* address offset: 0x0910 */
};

check_member(rk3538_vccio1_ioc_reg, gpio1_iddq, 0x0910);

struct rk3538_vccio2_ioc_reg {
     uint32_t reserved0000[16];                   /* address offset: 0x0000 */
     uint32_t gpio2a_iomux_sel_0;                 /* address offset: 0x0040 */
     uint32_t gpio2a_iomux_sel_1;                 /* address offset: 0x0044 */
     uint32_t reserved0048[78];                   /* address offset: 0x0048 */
     uint32_t gpio2a_ds_0;                        /* address offset: 0x0180 */
     uint32_t gpio2a_ds_1;                        /* address offset: 0x0184 */
     uint32_t gpio2a_ds_2;                        /* address offset: 0x0188 */
     uint32_t gpio2a_ds_3;                        /* address offset: 0x018c */
     uint32_t reserved0190[100];                  /* address offset: 0x0190 */
     uint32_t gpio2a_pull;                        /* address offset: 0x0320 */
     uint32_t reserved0324[63];                   /* address offset: 0x0324 */
     uint32_t gpio2a_ie;                          /* address offset: 0x0420 */
     uint32_t reserved0424[63];                   /* address offset: 0x0424 */
     uint32_t gpio2a_smt;                         /* address offset: 0x0520 */
     uint32_t reserved0524[63];                   /* address offset: 0x0524 */
     uint32_t gpio2a_sus;                         /* address offset: 0x0620 */
     uint32_t reserved0624[63];                   /* address offset: 0x0624 */
     uint32_t gpio2a_sl;                          /* address offset: 0x0720 */
     uint32_t reserved0724[63];                   /* address offset: 0x0724 */
     uint32_t gpio2a_od;                          /* address offset: 0x0820 */
     uint32_t reserved0824[59];                   /* address offset: 0x0824 */
     uint32_t gpio2_iddq;                         /* address offset: 0x0910 */
};

check_member(rk3538_vccio2_ioc_reg, gpio2_iddq, 0x0910);

struct rk3538_vccio3_ioc_reg {
     uint32_t reserved0000[24];                   /* address offset: 0x0000 */
     uint32_t gpio3a_iomux_sel_0;                 /* address offset: 0x0060 */
     uint32_t gpio3a_iomux_sel_1;                 /* address offset: 0x0064 */
     uint32_t gpio3b_iomux_sel_0;                 /* address offset: 0x0068 */
     uint32_t gpio3b_iomux_sel_1;                 /* address offset: 0x006c */
     uint32_t gpio3c_iomux_sel_0;                 /* address offset: 0x0070 */
     uint32_t reserved0074[83];                   /* address offset: 0x0074 */
     uint32_t gpio3a_ds_0;                        /* address offset: 0x01c0 */
     uint32_t gpio3a_ds_1;                        /* address offset: 0x01c4 */
     uint32_t gpio3a_ds_2;                        /* address offset: 0x01c8 */
     uint32_t gpio3a_ds_3;                        /* address offset: 0x01cc */
     uint32_t gpio3b_ds_0;                        /* address offset: 0x01d0 */
     uint32_t gpio3b_ds_1;                        /* address offset: 0x01d4 */
     uint32_t gpio3b_ds_2;                        /* address offset: 0x01d8 */
     uint32_t gpio3b_ds_3;                        /* address offset: 0x01dc */
     uint32_t gpio3c_ds_0;                        /* address offset: 0x01e0 */
     uint32_t gpio3c_ds_1;                        /* address offset: 0x01e4 */
     uint32_t reserved01e8[82];                   /* address offset: 0x01e8 */
     uint32_t gpio3a_pull;                        /* address offset: 0x0330 */
     uint32_t gpio3b_pull;                        /* address offset: 0x0334 */
     uint32_t gpio3c_pull;                        /* address offset: 0x0338 */
     uint32_t reserved033c[61];                   /* address offset: 0x033c */
     uint32_t gpio3a_ie;                          /* address offset: 0x0430 */
     uint32_t gpio3b_ie;                          /* address offset: 0x0434 */
     uint32_t gpio3c_ie;                          /* address offset: 0x0438 */
     uint32_t reserved043c[61];                   /* address offset: 0x043c */
     uint32_t gpio3a_smt;                         /* address offset: 0x0530 */
     uint32_t gpio3b_smt;                         /* address offset: 0x0534 */
     uint32_t gpio3c_smt;                         /* address offset: 0x0538 */
     uint32_t reserved053c[61];                   /* address offset: 0x053c */
     uint32_t gpio3a_sus;                         /* address offset: 0x0630 */
     uint32_t gpio3b_sus;                         /* address offset: 0x0634 */
     uint32_t gpio3c_sus;                         /* address offset: 0x0638 */
     uint32_t reserved063c[61];                   /* address offset: 0x063c */
     uint32_t gpio3a_sl;                          /* address offset: 0x0730 */
     uint32_t gpio3b_sl;                          /* address offset: 0x0734 */
     uint32_t gpio3c_sl;                          /* address offset: 0x0738 */
     uint32_t reserved073c[61];                   /* address offset: 0x073c */
     uint32_t gpio3a_od;                          /* address offset: 0x0830 */
     uint32_t gpio3b_od;                          /* address offset: 0x0834 */
     uint32_t gpio3c_od;                          /* address offset: 0x0838 */
     uint32_t reserved083c[53];                   /* address offset: 0x083c */
     uint32_t gpio3_iddq;                         /* address offset: 0x0910 */
};

check_member(rk3538_vccio3_ioc_reg, gpio3_iddq, 0x0910);

struct rk3538_vccio4_ioc_reg {
     uint32_t reserved0000[32];                   /* address offset: 0x0000 */
     uint32_t gpio4a_iomux_sel_0;                 /* address offset: 0x0080 */
     uint32_t gpio4a_iomux_sel_1;                 /* address offset: 0x0084 */
     uint32_t reserved0088[94];                   /* address offset: 0x0088 */
     uint32_t gpio4a_ds_0;                        /* address offset: 0x0200 */
     uint32_t gpio4a_ds_1;                        /* address offset: 0x0204 */
     uint32_t gpio4a_ds_2;                        /* address offset: 0x0208 */
     uint32_t gpio4a_ds_3;                        /* address offset: 0x020c */
     uint32_t reserved0210[76];                   /* address offset: 0x0210 */
     uint32_t gpio4a_pull;                        /* address offset: 0x0340 */
     uint32_t reserved0344[63];                   /* address offset: 0x0344 */
     uint32_t gpio4a_ie;                          /* address offset: 0x0440 */
     uint32_t reserved0444[63];                   /* address offset: 0x0444 */
     uint32_t gpio4a_smt;                         /* address offset: 0x0540 */
     uint32_t reserved0544[63];                   /* address offset: 0x0544 */
     uint32_t gpio4a_sus;                         /* address offset: 0x0640 */
     uint32_t reserved0644[63];                   /* address offset: 0x0644 */
     uint32_t gpio4a_sl;                          /* address offset: 0x0740 */
     uint32_t reserved0744[63];                   /* address offset: 0x0744 */
     uint32_t gpio4a_od;                          /* address offset: 0x0840 */
     uint32_t reserved0844[51];                   /* address offset: 0x0844 */
     uint32_t gpio4_iddq;                         /* address offset: 0x0910 */
};

check_member(rk3538_vccio4_ioc_reg, gpio4_iddq, 0x0910);

struct rk3538_vccio5_ioc_reg {
     uint32_t reserved0000[40];                   /* address offset: 0x0000 */
     uint32_t gpio5a_iomux_sel_0;                 /* address offset: 0x00a0 */
     uint32_t gpio5a_iomux_sel_1;                 /* address offset: 0x00a4 */
     uint32_t gpio5b_iomux_sel_0;                 /* address offset: 0x00a8 */
     uint32_t gpio5b_iomux_sel_1;                 /* address offset: 0x00ac */
     uint32_t gpio5c_iomux_sel_0;                 /* address offset: 0x00b0 */
     uint32_t reserved00b4[99];                   /* address offset: 0x00b4 */
     uint32_t gpio5a_ds_0;                        /* address offset: 0x0240 */
     uint32_t gpio5a_ds_1;                        /* address offset: 0x0244 */
     uint32_t gpio5a_ds_2;                        /* address offset: 0x0248 */
     uint32_t gpio5a_ds_3;                        /* address offset: 0x024c */
     uint32_t gpio5b_ds_0;                        /* address offset: 0x0250 */
     uint32_t gpio5b_ds_1;                        /* address offset: 0x0254 */
     uint32_t gpio5b_ds_2;                        /* address offset: 0x0258 */
     uint32_t gpio5b_ds_3;                        /* address offset: 0x025c */
     uint32_t gpio5c_ds_0;                        /* address offset: 0x0260 */
     uint32_t gpio5c_ds_1;                        /* address offset: 0x0264 */
     uint32_t reserved0268[58];                   /* address offset: 0x0268 */
     uint32_t gpio5a_pull;                        /* address offset: 0x0350 */
     uint32_t gpio5b_pull;                        /* address offset: 0x0354 */
     uint32_t gpio5c_pull;                        /* address offset: 0x0358 */
     uint32_t reserved035c[61];                   /* address offset: 0x035c */
     uint32_t gpio5a_ie;                          /* address offset: 0x0450 */
     uint32_t gpio5b_ie;                          /* address offset: 0x0454 */
     uint32_t gpio5c_ie;                          /* address offset: 0x0458 */
     uint32_t reserved045c[61];                   /* address offset: 0x045c */
     uint32_t gpio5a_smt;                         /* address offset: 0x0550 */
     uint32_t gpio5b_smt;                         /* address offset: 0x0554 */
     uint32_t gpio5c_smt;                         /* address offset: 0x0558 */
     uint32_t reserved055c[61];                   /* address offset: 0x055c */
     uint32_t gpio5a_sus;                         /* address offset: 0x0650 */
     uint32_t gpio5b_sus;                         /* address offset: 0x0654 */
     uint32_t gpio5c_sus;                         /* address offset: 0x0658 */
     uint32_t reserved065c[61];                   /* address offset: 0x065c */
     uint32_t gpio5a_sl;                          /* address offset: 0x0750 */
     uint32_t gpio5b_sl;                          /* address offset: 0x0754 */
     uint32_t gpio5c_sl;                          /* address offset: 0x0758 */
     uint32_t reserved075c[61];                   /* address offset: 0x075c */
     uint32_t gpio5a_od;                          /* address offset: 0x0850 */
     uint32_t gpio5b_od;                          /* address offset: 0x0854 */
     uint32_t gpio5c_od;                          /* address offset: 0x0858 */
     uint32_t reserved085c[45];                   /* address offset: 0x085c */
     uint32_t gpio5_iddq;                         /* address offset: 0x0910 */
};

check_member(rk3538_vccio5_ioc_reg, gpio5_iddq, 0x0910);

struct rk3538_vccio6_ioc_reg {
     uint32_t reserved0000[48];                   /* address offset: 0x0000 */
     uint32_t gpio6a_iomux_sel_0;                 /* address offset: 0x00c0 */
     uint32_t gpio6a_iomux_sel_1;                 /* address offset: 0x00c4 */
     uint32_t gpio6b_iomux_sel_0;                 /* address offset: 0x00c8 */
     uint32_t gpio6b_iomux_sel_1;                 /* address offset: 0x00cc */
     uint32_t gpio6c_iomux_sel_0;                 /* address offset: 0x00d0 */
     uint32_t gpio6c_iomux_sel_1;                 /* address offset: 0x00d4 */
     uint32_t reserved00d8[106];                  /* address offset: 0x00d8 */
     uint32_t gpio6a_ds_0;                        /* address offset: 0x0280 */
     uint32_t gpio6a_ds_1;                        /* address offset: 0x0284 */
     uint32_t gpio6a_ds_2;                        /* address offset: 0x0288 */
     uint32_t gpio6a_ds_3;                        /* address offset: 0x028c */
     uint32_t gpio6b_ds_0;                        /* address offset: 0x0290 */
     uint32_t gpio6b_ds_1;                        /* address offset: 0x0294 */
     uint32_t gpio6b_ds_2;                        /* address offset: 0x0298 */
     uint32_t gpio6b_ds_3;                        /* address offset: 0x029c */
     uint32_t gpio6c_ds_0;                        /* address offset: 0x02a0 */
     uint32_t gpio6c_ds_1;                        /* address offset: 0x02a4 */
     uint32_t gpio6c_ds_2;                        /* address offset: 0x02a8 */
     uint32_t gpio6c_ds_3;                        /* address offset: 0x02ac */
     uint32_t reserved02b0[44];                   /* address offset: 0x02b0 */
     uint32_t gpio6a_pull;                        /* address offset: 0x0360 */
     uint32_t gpio6b_pull;                        /* address offset: 0x0364 */
     uint32_t gpio6c_pull;                        /* address offset: 0x0368 */
     uint32_t reserved036c[61];                   /* address offset: 0x036c */
     uint32_t gpio6a_ie;                          /* address offset: 0x0460 */
     uint32_t gpio6b_ie;                          /* address offset: 0x0464 */
     uint32_t gpio6c_ie;                          /* address offset: 0x0468 */
     uint32_t reserved046c[61];                   /* address offset: 0x046c */
     uint32_t gpio6a_smt;                         /* address offset: 0x0560 */
     uint32_t gpio6b_smt;                         /* address offset: 0x0564 */
     uint32_t gpio6c_smt;                         /* address offset: 0x0568 */
     uint32_t reserved056c[61];                   /* address offset: 0x056c */
     uint32_t gpio6a_sus;                         /* address offset: 0x0660 */
     uint32_t gpio6b_sus;                         /* address offset: 0x0664 */
     uint32_t gpio6c_sus;                         /* address offset: 0x0668 */
     uint32_t reserved066c[61];                   /* address offset: 0x066c */
     uint32_t gpio6a_sl;                          /* address offset: 0x0760 */
     uint32_t gpio6b_sl;                          /* address offset: 0x0764 */
     uint32_t gpio6c_sl;                          /* address offset: 0x0768 */
     uint32_t reserved076c[61];                   /* address offset: 0x076c */
     uint32_t gpio6a_od;                          /* address offset: 0x0860 */
     uint32_t gpio6b_od;                          /* address offset: 0x0864 */
     uint32_t gpio6c_od;                          /* address offset: 0x0868 */
     uint32_t reserved086c[44];                   /* address offset: 0x086c */
     uint32_t gpio6_iddq;                         /* address offset: 0x091c */
};

check_member(rk3538_vccio6_ioc_reg, gpio6_iddq, 0x091c);

#endif /* _ASM_ARCH_IOC_RK3538_H */
