/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * (C) Copyright 2025 Rockchip Electronics Co., Ltd
 */

#ifndef __SOC_ROCKCHIP_RK3572_GRF_H__
#define __SOC_ROCKCHIP_RK3572_GRF_H__

/* audio_grf register structure define */
struct rk3572_audio_grf_reg {
     uint32_t mem_ con0;                          /* address offset: 0x0000 */
     uint32_t mem_con1;                           /* address offset: 0x0004 */
     uint32_t reserved0008;                       /* address offset: 0x0008 */
     uint32_t mem_con2;                           /* address offset: 0x000c */
     uint32_t audio_con0;                         /* address offset: 0x0010 */
     uint32_t audio_con1;                         /* address offset: 0x0014 */
     uint32_t audio_con2;                         /* address offset: 0x0018 */
     uint32_t audio_con3;                         /* address offset: 0x001c */
     uint32_t audio_con4;                         /* address offset: 0x0020 */
     uint32_t reserved0024[2];                    /* address offset: 0x0024 */
     uint32_t audio_con7;                         /* address offset: 0x002c */
     uint32_t audio_st;                           /* address offset: 0x0030 */
     uint32_t reserved0034[3];                    /* address offset: 0x0034 */
};

check_member(rk3572_audio_grf_reg, reserved0034, 0x0034);

/* bigcore_grf register structure define */
struct rk3572_bigcore_grf_reg {
     uint32_t reserved0000[11];                   /* address offset: 0x0000 */
     uint32_t cpu_status0;                        /* address offset: 0x002c */
     uint32_t reserved0030;                       /* address offset: 0x0030 */
     uint32_t cpu_con0;                           /* address offset: 0x0034 */
     uint32_t cpu_con1;                           /* address offset: 0x0038 */
     uint32_t reserved003c;                       /* address offset: 0x003c */
     uint32_t cpu_mem_cfg_hdsprf;                 /* address offset: 0x0040 */
     uint32_t cpu_mem_cfg_hssprf_low;             /* address offset: 0x0044 */
     uint32_t cpu_mem_cfg_hssprf_high;            /* address offset: 0x0048 */
};

check_member(rk3572_bigcore_grf_reg, cpu_mem_cfg_hssprf_high, 0x0048);

/* cci_grf register structure define */
struct rk3572_cci_grf_reg {
     uint32_t cci_con0;                           /* address offset: 0x0000 */
     uint32_t cci_con1;                           /* address offset: 0x0004 */
     uint32_t reserved0008[15];                   /* address offset: 0x0008 */
     uint32_t cci_status4;                        /* address offset: 0x0044 */
     uint32_t reserved0048[3];                    /* address offset: 0x0048 */
     uint32_t cci_mem_cfg_hdsprf;                 /* address offset: 0x0054 */
};

check_member(rk3572_cci_grf_reg, cci_mem_cfg_hdsprf, 0x0054);

/* center_grf register structure define */
struct rk3572_center_grf_reg {
     uint32_t soc_con0;                           /* address offset: 0x0000 */
     uint32_t soc_con1;                           /* address offset: 0x0004 */
     uint32_t reserved0008[2];                    /* address offset: 0x0008 */
     uint32_t soc_con4;                           /* address offset: 0x0010 */
     uint32_t reserved0014[11];                   /* address offset: 0x0014 */
     uint32_t center2ddr_stat0;                   /* address offset: 0x0040 */
     uint32_t center2ddr_stat1;                   /* address offset: 0x0044 */
     uint32_t center_slv_stat;                    /* address offset: 0x0048 */
     uint32_t gpu_slv_stat;                       /* address offset: 0x004c */
     uint32_t sysmem_stat;                        /* address offset: 0x0050 */
     uint32_t reserved0054;                       /* address offset: 0x0054 */
     uint32_t center2ddrp1_stat0;                 /* address offset: 0x0058 */
     uint32_t center2ddrp1_stat1;                 /* address offset: 0x005c */
};

check_member(rk3572_center_grf_reg, center2ddrp1_stat1, 0x005c);

/* combo_pipe_phy_grf register structure define */
struct rk3572_combo_pipe_phy_grf_reg {
     uint32_t pipe_con0;                          /* address offset: 0x0000 */
     uint32_t pipe_con1;                          /* address offset: 0x0004 */
     uint32_t pipe_con2;                          /* address offset: 0x0008 */
     uint32_t pipe_con3;                          /* address offset: 0x000c */
     uint32_t pipe_con4;                          /* address offset: 0x0010 */
     uint32_t reserved0014[8];                    /* address offset: 0x0014 */
     uint32_t pipe_status1;                       /* address offset: 0x0034 */
     uint32_t reserved0038[18];                   /* address offset: 0x0038 */
     uint32_t lfps_det_con;                       /* address offset: 0x0080 */
     uint32_t reserved0084[7];                    /* address offset: 0x0084 */
     uint32_t phy_int_en;                         /* address offset: 0x00a0 */
     uint32_t phy_int_status;                     /* address offset: 0x00a4 */
};

check_member(rk3572_combo_pipe_phy_grf_reg, phy_int_status, 0x00a4);

/* ddr_grf register structure define */
struct rk3572_ddr_grf_reg {
     uint32_t con[23];                            /* address offset: 0x0000 */
     uint32_t reserved005c;                       /* address offset: 0x005c */
     uint32_t cha_con24;                          /* address offset: 0x0060 */
     uint32_t cha_con25;                          /* address offset: 0x0064 */
     uint32_t cha_con26;                          /* address offset: 0x0068 */
     uint32_t cha_con27;                          /* address offset: 0x006c */
     uint32_t cha_con28;                          /* address offset: 0x0070 */
     uint32_t cha_con29;                          /* address offset: 0x0074 */
     uint32_t cha_con30;                          /* address offset: 0x0078 */
     uint32_t cha_con31;                          /* address offset: 0x007c */
     uint32_t cha_con32;                          /* address offset: 0x0080 */
     uint32_t cha_con33;                          /* address offset: 0x0084 */
     uint32_t cha_con34;                          /* address offset: 0x0088 */
     uint32_t cha_con35;                          /* address offset: 0x008c */
     uint32_t cha_con36;                          /* address offset: 0x0090 */
     uint32_t cha_con37;                          /* address offset: 0x0094 */
     uint32_t cha_con38;                          /* address offset: 0x0098 */
     uint32_t cha_con39;                          /* address offset: 0x009c */
     uint32_t cha_con40;                          /* address offset: 0x00a0 */
     uint32_t cha_con41;                          /* address offset: 0x00a4 */
     uint32_t reserved00a8[86];                   /* address offset: 0x00a8 */
     uint32_t cha_status0;                        /* address offset: 0x0200 */
     uint32_t reserved0204[7];                    /* address offset: 0x0204 */
     uint32_t cha_status16;                       /* address offset: 0x0220 */
     uint32_t cha_status17;                       /* address offset: 0x0224 */
     uint32_t cha_status18;                       /* address offset: 0x0228 */
     uint32_t cha_status19;                       /* address offset: 0x022c */
     uint32_t cha_status20;                       /* address offset: 0x0230 */
     uint32_t reserved0234[115];                  /* address offset: 0x0234 */
     uint32_t cha_phy_test_con0;                  /* address offset: 0x0400 */
     uint32_t cha_phy_test_con1;                  /* address offset: 0x0404 */
     uint32_t cha_phy_test_con2;                  /* address offset: 0x0408 */
     uint32_t cha_phy_test_con3;                  /* address offset: 0x040c */
     uint32_t cha_phy_test_con4;                  /* address offset: 0x0410 */
     uint32_t cha_phy_test_con5;                  /* address offset: 0x0414 */
     uint32_t cha_phy_test_con6;                  /* address offset: 0x0418 */
     uint32_t cha_phy_test_con7;                  /* address offset: 0x041c */
     uint32_t cha_phy_test_status0;               /* address offset: 0x0420 */
     uint32_t cha_phy_test_status1;               /* address offset: 0x0424 */
     uint32_t cha_phy_test_status2;               /* address offset: 0x0428 */
     uint32_t reserved042c[65];                   /* address offset: 0x042c */
     uint32_t cha_phy_con0;                       /* address offset: 0x0530 */
     uint32_t reserved0534[3];                    /* address offset: 0x0534 */
     uint32_t common_con0;                        /* address offset: 0x0540 */
     uint32_t common_con1;                        /* address offset: 0x0544 */
     uint32_t common_con2;                        /* address offset: 0x0548 */
     uint32_t common_con3;                        /* address offset: 0x054c */
     uint32_t common_con4;                        /* address offset: 0x0550 */
     uint32_t common_con5;                        /* address offset: 0x0554 */
     uint32_t common_con6;                        /* address offset: 0x0558 */
     uint32_t reserved055c[9];                    /* address offset: 0x055c */
     uint32_t status0;                            /* address offset: 0x0580 */
     uint32_t status1;                            /* address offset: 0x0584 */
};

check_member(rk3572_ddr_grf_reg, status1, 0x0584);

/* dsmc_grf register structure define */
struct rk3572_dsmc_grf_reg {
     uint32_t reserved0000;                       /* address offset: 0x0000 */
     uint32_t mem_con1;                           /* address offset: 0x0004 */
     uint32_t reserved0008;                       /* address offset: 0x0008 */
     uint32_t mem_gate_con;                       /* address offset: 0x000c */
     uint32_t reserved0010[3];                    /* address offset: 0x0010 */
};

check_member(rk3572_dsmc_grf_reg, reserved0010, 0x0010);

/* gpu_grf register structure define */
struct rk3572_gpu_grf_reg {
     uint32_t mem_ con0;                          /* address offset: 0x0000 */
     uint32_t mem_con1;                           /* address offset: 0x0004 */
     uint32_t mem_con2;                           /* address offset: 0x0008 */
     uint32_t gpu_con0;                           /* address offset: 0x000c */
     uint32_t gpu_st0;                            /* address offset: 0x0010 */
     uint32_t reserved0014[6];                    /* address offset: 0x0014 */
};

check_member(rk3572_gpu_grf_reg, reserved0014, 0x0014);

/* csiphy_grf register structure define */
struct rk3572_csiphy_grf_reg {
     uint32_t con0;                               /* address offset: 0x0000 */
     uint32_t reserved0004[31];                   /* address offset: 0x0004 */
     uint32_t status0;                            /* address offset: 0x0080 */
};

check_member(rk3572_csiphy_grf_reg, status0, 0x0080);

/* hdptxphy_grf register structure define */
struct rk3572_hdptxphy_grf_reg {
     uint32_t con0;                               /* address offset: 0x0000 */
     uint32_t con1;                               /* address offset: 0x0004 */
     uint32_t reserved0008[30];                   /* address offset: 0x0008 */
     uint32_t status;                             /* address offset: 0x0080 */
};

check_member(rk3572_hdptxphy_grf_reg, status, 0x0080);

/* img_grf register structure define */
struct rk3572_img_grf_reg {
     uint32_t con0;                               /* address offset: 0x0000 */
     uint32_t con1;                               /* address offset: 0x0004 */
     uint32_t con2;                               /* address offset: 0x0008 */
     uint32_t con3;                               /* address offset: 0x000c */
     uint32_t reserved0010[4];                    /* address offset: 0x0010 */
     uint32_t st0;                                /* address offset: 0x0020 */
};

check_member(rk3572_img_grf_reg, st0, 0x0020);

/* trng_grf register structure define */
struct rk3572_trng_grf_reg {
     uint32_t reserved0000[60];                   /* address offset: 0x0000 */
     uint32_t con0;                               /* address offset: 0x00f0 */
     uint32_t reserved00f4;                       /* address offset: 0x00f4 */
     uint32_t st_con0;                            /* address offset: 0x00f8 */
};

check_member(rk3572_trng_grf_reg, st_con0, 0x00f8);

/* vdec_grf register structure define */
struct rk3572_vdec_grf_reg {
     uint32_t con0;                               /* address offset: 0x0000 */
     uint32_t con1;                               /* address offset: 0x0004 */
     uint32_t con2;                               /* address offset: 0x0008 */
     uint32_t con3;                               /* address offset: 0x000c */
     uint32_t con4;                               /* address offset: 0x0010 */
     uint32_t reserved0014[3];                    /* address offset: 0x0014 */
     uint32_t st0;                                /* address offset: 0x0020 */
};

check_member(rk3572_vdec_grf_reg, st0, 0x0020);

/* vi_grf register structure define */
struct rk3572_vi_grf_reg {
     uint32_t con0;                               /* address offset: 0x0000 */
     uint32_t con1;                               /* address offset: 0x0004 */
     uint32_t con2;                               /* address offset: 0x0008 */
     uint32_t con3;                               /* address offset: 0x000c */
     uint32_t con4;                               /* address offset: 0x0010 */
     uint32_t reserved0014[27];                   /* address offset: 0x0014 */
     uint32_t con32;                              /* address offset: 0x0080 */
     uint32_t con33;                              /* address offset: 0x0084 */
     uint32_t con34;                              /* address offset: 0x0088 */
     uint32_t con35;                              /* address offset: 0x008c */
     uint32_t reserved0090[24];                   /* address offset: 0x0090 */
     uint32_t st0;                                /* address offset: 0x00f0 */
};

check_member(rk3572_vi_grf_reg, st0, 0x00f0);

/* vo_grf register structure define */
struct rk3572_vo_grf_reg {
     uint32_t con0;                               /* address offset: 0x0000 */
     uint32_t con1;                               /* address offset: 0x0004 */
     uint32_t con2;                               /* address offset: 0x0008 */
     uint32_t con3;                               /* address offset: 0x000c */
     uint32_t con4;                               /* address offset: 0x0010 */
     uint32_t con5;                               /* address offset: 0x0014 */
     uint32_t con6;                               /* address offset: 0x0018 */
     uint32_t con7;                               /* address offset: 0x001c */
     uint32_t con8;                               /* address offset: 0x0020 */
     uint32_t usbphy_con[3];                      /* address offset: 0x0024 */
     uint32_t usb2host_con[2];                    /* address offset: 0x0030 */
     uint32_t usbotg_con[2];                      /* address offset: 0x0038 */
     uint32_t con16;                              /* address offset: 0x0040 */
     uint32_t reserved0044[17];                   /* address offset: 0x0044 */
     uint32_t con34;                              /* address offset: 0x0088 */
     uint32_t con35;                              /* address offset: 0x008c */
     uint32_t con36;                              /* address offset: 0x0090 */
     uint32_t con37;                              /* address offset: 0x0094 */
     uint32_t reserved0098[2];                    /* address offset: 0x0098 */
     uint32_t con40;                              /* address offset: 0x00a0 */
     uint32_t reserved00a4[7];                    /* address offset: 0x00a4 */
     uint32_t st_con0;                            /* address offset: 0x00c0 */
     uint32_t st_con1;                            /* address offset: 0x00c4 */
     uint32_t st_con2;                            /* address offset: 0x00c8 */
     uint32_t st_con3;                            /* address offset: 0x00cc */
     uint32_t st_con4;                            /* address offset: 0x00d0 */
     uint32_t st_con5;                            /* address offset: 0x00d4 */
     uint32_t st_con6;                            /* address offset: 0x00d8 */
};

check_member(rk3572_vo_grf_reg, st_con6, 0x00d8);

/* vop_grf register structure define */
struct rk3572_vop_grf_reg {
     uint32_t con0;                               /* address offset: 0x0000 */
     uint32_t con1;                               /* address offset: 0x0004 */
     uint32_t con2;                               /* address offset: 0x0008 */
     uint32_t con3;                               /* address offset: 0x000c */
     uint32_t reserved0010[6];                    /* address offset: 0x0010 */
     uint32_t con10;                              /* address offset: 0x0028 */
     uint32_t con11;                              /* address offset: 0x002c */
     uint32_t st_con0;                            /* address offset: 0x0030 */
     uint32_t st_con1;                            /* address offset: 0x0034 */
     uint32_t reserved0038;                       /* address offset: 0x0038 */
     uint32_t st_con3;                            /* address offset: 0x003c */
};

check_member(rk3572_vop_grf_reg, st_con3, 0x003c);

/* usb2phy_grf register structure define */
struct rk3572_usb2phy_grf_reg {
     uint32_t con0;                               /* address offset: 0x0000 */
     uint32_t con1;                               /* address offset: 0x0004 */
     uint32_t con2;                               /* address offset: 0x0008 */
     uint32_t con3;                               /* address offset: 0x000c */
     uint32_t con4;                               /* address offset: 0x0010 */
     uint32_t con5;                               /* address offset: 0x0014 */
     uint32_t reserved0018[2];                    /* address offset: 0x0018 */
     uint32_t ls_con;                             /* address offset: 0x0020 */
     uint32_t dis_con;                            /* address offset: 0x0024 */
     uint32_t bvalid_con;                         /* address offset: 0x0028 */
     uint32_t id_con;                             /* address offset: 0x002c */
     uint32_t vbusvalid_con;                      /* address offset: 0x0030 */
     uint32_t reserved0034[3];                    /* address offset: 0x0034 */
     uint32_t dbg_con0;                           /* address offset: 0x0040 */
     uint32_t linest_timeout;                     /* address offset: 0x0044 */
     uint32_t linest_deb;                         /* address offset: 0x0048 */
     uint32_t rx_timeout;                         /* address offset: 0x004c */
     uint32_t seq_limt;                           /* address offset: 0x0050 */
     uint32_t linest_cnt_st;                      /* address offset: 0x0054 */
     uint32_t dbg_st;                             /* address offset: 0x0058 */
     uint32_t rx_cnt_st;                          /* address offset: 0x005c */
     uint32_t reserved0060[8];                    /* address offset: 0x0060 */
     uint32_t st0;                                /* address offset: 0x0080 */
     uint32_t reserved0084[15];                   /* address offset: 0x0084 */
     uint32_t int_en;                             /* address offset: 0x00c0 */
     uint32_t int_st;                             /* address offset: 0x00c4 */
     uint32_t int_st_clr;                         /* address offset: 0x00c8 */
     uint32_t reserved00cc;                       /* address offset: 0x00cc */
     uint32_t detclk_sel;                         /* address offset: 0x00d0 */
};

check_member(rk3572_usb2phy_grf_reg, detclk_sel, 0x00d0);

/* litcore register structure define */
struct rk3572_litcore_reg {
     uint32_t reserved0000[11];                   /* address offset: 0x0000 */
     uint32_t cpu_status0;                        /* address offset: 0x002c */
     uint32_t reserved0030;                       /* address offset: 0x0030 */
     uint32_t cpu_con0;                           /* address offset: 0x0034 */
     uint32_t cpu_con1;                           /* address offset: 0x0038 */
     uint32_t reserved003c;                       /* address offset: 0x003c */
     uint32_t cpu_mem_cfg_hdsprf;                 /* address offset: 0x0040 */
     uint32_t cpu_mem_cfg_hssprf_low;             /* address offset: 0x0044 */
     uint32_t cpu_mem_cfg_hssprf_high;            /* address offset: 0x0048 */
};

check_member(rk3572_litcore_reg, cpu_mem_cfg_hssprf_high, 0x0048);

/* mphy_grf register structure define */
struct rk3572_mphy_grf_reg {
     uint32_t con0;                               /* address offset: 0x0000 */
     uint32_t con1;                               /* address offset: 0x0004 */
     uint32_t status0;                            /* address offset: 0x0008 */
};

check_member(rk3572_mphy_grf_reg, status0, 0x0008);

/* npu_grf register structure define */
struct rk3572_npu_grf_reg {
     uint32_t rknn_con0;                          /* address offset: 0x0000 */
     uint32_t rknn_con1;                          /* address offset: 0x0004 */
     uint32_t cbuf_soft_gate0;                    /* address offset: 0x0008 */
     uint32_t cbuf_soft_gate1;                    /* address offset: 0x000c */
     uint32_t rknn_status0;                       /* address offset: 0x0010 */
     uint32_t reserved0014;                       /* address offset: 0x0014 */
     uint32_t fw_st;                              /* address offset: 0x0018 */
     uint32_t reserved001c;                       /* address offset: 0x001c */
     uint32_t mem_con0;                           /* address offset: 0x0020 */
     uint32_t mem_con1;                           /* address offset: 0x0024 */
     uint32_t mem_con2;                           /* address offset: 0x0028 */
     uint32_t mem_con3;                           /* address offset: 0x002c */
     uint32_t reserved0030[3];                    /* address offset: 0x0030 */
};

check_member(rk3572_npu_grf_reg, reserved0030, 0x0030);

/* nvm0_grf register structure define */
struct rk3572_nvm0_grf_reg {
     uint32_t mem_ con0;                          /* address offset: 0x0000 */
     uint32_t mem_con1;                           /* address offset: 0x0004 */
     uint32_t mem_con2;                           /* address offset: 0x0008 */
     uint32_t reserved000c;                       /* address offset: 0x000c */
     uint32_t nvm0_st;                            /* address offset: 0x0010 */
     uint32_t ahb_con;                            /* address offset: 0x0014 */
     uint32_t reserved0018[2];                    /* address offset: 0x0018 */
     uint32_t gmac1_con;                          /* address offset: 0x0020 */
     uint32_t gmac1_tp0;                          /* address offset: 0x0024 */
     uint32_t gmac1_tp1;                          /* address offset: 0x0028 */
     uint32_t gmac1_cmd;                          /* address offset: 0x002c */
     uint32_t gmac1_st;                           /* address offset: 0x0030 */
     uint32_t reserved0034[6];                    /* address offset: 0x0034 */
};

check_member(rk3572_nvm0_grf_reg, reserved0034, 0x0034);

/* nvm1_grf register structure define */
struct rk3572_nvm1_grf_reg {
     uint32_t mem_con0;                           /* address offset: 0x0000 */
     uint32_t mem_con1;                           /* address offset: 0x0004 */
     uint32_t mem_con2;                           /* address offset: 0x0008 */
     uint32_t nvm1_st;                            /* address offset: 0x000c */
     uint32_t ahb_con;                            /* address offset: 0x0010 */
     uint32_t reserved0014[10];                   /* address offset: 0x0014 */
};

check_member(rk3572_nvm1_grf_reg, reserved0014, 0x0014);

/* php0_grf register structure define */
struct rk3572_php0_grf_reg {
     uint32_t usb3drd0_status_lat0;               /* address offset: 0x0000 */
     uint32_t usb3drd0_status_lat1;               /* address offset: 0x0004 */
     uint32_t usb3drd0_status_cb;                 /* address offset: 0x0008 */
     uint32_t usb3drd0_status;                    /* address offset: 0x000c */
     uint32_t usb3drd0_con0;                      /* address offset: 0x0010 */
     uint32_t usb3drd0_con1;                      /* address offset: 0x0014 */
     uint32_t reserved0018;                       /* address offset: 0x0018 */
     uint32_t usb3drd0_phm;                       /* address offset: 0x001c */
     uint32_t usb3drd1_status_lat0;               /* address offset: 0x0020 */
     uint32_t usb3drd1_status_lat1;               /* address offset: 0x0024 */
     uint32_t usb3drd1_status_cb;                 /* address offset: 0x0028 */
     uint32_t usb3drd1_status;                    /* address offset: 0x002c */
     uint32_t usb3drd1_con0;                      /* address offset: 0x0030 */
     uint32_t usb3drd1_con1;                      /* address offset: 0x0034 */
     uint32_t reserved0038;                       /* address offset: 0x0038 */
     uint32_t usb3drd1_phm;                       /* address offset: 0x003c */
     uint32_t mem_ con0;                          /* address offset: 0x0040 */
     uint32_t mem_con1;                           /* address offset: 0x0044 */
     uint32_t mem_con2;                           /* address offset: 0x0048 */
     uint32_t reserved004c;                       /* address offset: 0x004c */
     uint32_t usb3_0_addr_h;                      /* address offset: 0x0050 */
     uint32_t usb3_1_addr_h ;                     /* address offset: 0x0054 */
     uint32_t sata0_con;                          /* address offset: 0x0058 */
     uint32_t sata1_con;                          /* address offset: 0x005c */
     uint32_t reserved0060[2];                    /* address offset: 0x0060 */
     uint32_t pipe_con0;                          /* address offset: 0x0068 */
     uint32_t pcie_clk_req_st;                    /* address offset: 0x006c */
     uint32_t gmac0_con;                          /* address offset: 0x0070 */
     uint32_t gmac0_tp0;                          /* address offset: 0x0074 */
     uint32_t gmac0_tp1;                          /* address offset: 0x0078 */
     uint32_t gmac0_cmd;                          /* address offset: 0x007c */
     uint32_t gmac0_st;                           /* address offset: 0x0080 */
     uint32_t xpcs_st;                            /* address offset: 0x0084 */
     uint32_t xpcs_con0;                          /* address offset: 0x0088 */
     uint32_t reserved008c[9];                    /* address offset: 0x008c */
     uint32_t fw_st0;                             /* address offset: 0x00b0 */
     uint32_t fw_st1;                             /* address offset: 0x00b4 */
     uint32_t fw_st2;                             /* address offset: 0x00b8 */
     uint32_t reserved00bc;                       /* address offset: 0x00bc */
     uint32_t php0top_cru_test_con0;              /* address offset: 0x00c0 */
     uint32_t php0top_cru_test_con1;              /* address offset: 0x00c4 */
     uint32_t cru_test_st0;                       /* address offset: 0x00c8 */
};

check_member(rk3572_php0_grf_reg, cru_test_st0, 0x00c8);

/* pmu0_grf register structure define */
struct rk3572_pmu0_grf_reg {
     uint32_t soc_con0;                           /* address offset: 0x0000 */
     uint32_t soc_con1;                           /* address offset: 0x0004 */
     uint32_t reserved0008[2];                    /* address offset: 0x0008 */
     uint32_t soc_con4;                           /* address offset: 0x0010 */
     uint32_t soc_con5;                           /* address offset: 0x0014 */
     uint32_t soc_con6;                           /* address offset: 0x0018 */
     uint32_t reserved001c;                       /* address offset: 0x001c */
     uint32_t io_ret_con0;                        /* address offset: 0x0020 */
     uint32_t io_ret_con1;                        /* address offset: 0x0024 */
     uint32_t reserved0028[2];                    /* address offset: 0x0028 */
     uint32_t mem_con;                            /* address offset: 0x0030 */
     uint32_t reserved0034[1027];                 /* address offset: 0x0034 */
     uint32_t os_reg16;                           /* address offset: 0x1040 */
     uint32_t os_reg17;                           /* address offset: 0x1044 */
     uint32_t os_reg18;                           /* address offset: 0x1048 */
     uint32_t os_reg19;                           /* address offset: 0x104c */
     uint32_t os_reg20;                           /* address offset: 0x1050 */
     uint32_t os_reg21;                           /* address offset: 0x1054 */
     uint32_t os_reg22;                           /* address offset: 0x1058 */
     uint32_t os_reg23;                           /* address offset: 0x105c */
     uint32_t reserved1060[103];                  /* address offset: 0x1060 */
     uint32_t os_tag;                             /* address offset: 0x11fc */
};

check_member(rk3572_pmu0_grf_reg, os_tag, 0x11fc);

/* pmu1_grf register structure define */
struct rk3572_pmu1_grf_reg {
     uint32_t soc_con0;                           /* address offset: 0x0000 */
     uint32_t soc_con1;                           /* address offset: 0x0004 */
     uint32_t soc_con2;                           /* address offset: 0x0008 */
     uint32_t reserved000c[3];                    /* address offset: 0x000c */
     uint32_t soc_con6;                           /* address offset: 0x0018 */
     uint32_t soc_con7;                           /* address offset: 0x001c */
     uint32_t reserved0020[12];                   /* address offset: 0x0020 */
     uint32_t biu_con;                            /* address offset: 0x0050 */
     uint32_t biu_status;                         /* address offset: 0x0054 */
     uint32_t reserved0058[2];                    /* address offset: 0x0058 */
     uint32_t soc_status;                         /* address offset: 0x0060 */
     uint32_t reserved0064[7];                    /* address offset: 0x0064 */
     uint32_t mem_con0;                           /* address offset: 0x0080 */
     uint32_t mem_con1;                           /* address offset: 0x0084 */
     uint32_t reserved0088[30];                   /* address offset: 0x0088 */
     uint32_t func_rst_status;                    /* address offset: 0x0100 */
    __o  uint32_t func_rst_clr;                       /* address offset: 0x0104 */
     uint32_t reserved0108[2];                    /* address offset: 0x0108 */
     uint32_t sd_detect_con;                      /* address offset: 0x0110 */
     uint32_t sd_detect_sts;                      /* address offset: 0x0114 */
    __o  uint32_t sd_detect_clr;                      /* address offset: 0x0118 */
     uint32_t sd_detect_cnt;                      /* address offset: 0x011c */
     uint32_t reserved0120[952];                  /* address offset: 0x0120 */
     uint32_t os_reg0;                            /* address offset: 0x1000 */
     uint32_t os_reg1;                            /* address offset: 0x1004 */
     uint32_t os_reg2;                            /* address offset: 0x1008 */
     uint32_t os_reg3;                            /* address offset: 0x100c */
     uint32_t os_reg4;                            /* address offset: 0x1010 */
     uint32_t os_reg5;                            /* address offset: 0x1014 */
     uint32_t os_reg6;                            /* address offset: 0x1018 */
     uint32_t os_reg7;                            /* address offset: 0x101c */
     uint32_t os_reg8;                            /* address offset: 0x1020 */
     uint32_t os_reg9;                            /* address offset: 0x1024 */
     uint32_t os_reg10;                           /* address offset: 0x1028 */
     uint32_t os_reg11;                           /* address offset: 0x102c */
     uint32_t os_reg12;                           /* address offset: 0x1030 */
     uint32_t os_reg13;                           /* address offset: 0x1034 */
     uint32_t os_reg14;                           /* address offset: 0x1038 */
     uint32_t os_reg15;                           /* address offset: 0x103c */
};

check_member(rk3572_pmu1_grf_reg, os_reg15, 0x103c);

/* sys_grf register structure define */
struct rk3572_sys_grf_reg {
     uint32_t soc_con0;                           /* address offset: 0x0000 */
     uint32_t soc_con1;                           /* address offset: 0x0004 */
     uint32_t con[3];                             /* address offset: 0x0008 */
     uint32_t gpio6_filter_con[6];                /* address offset: 0x0014 */
     uint32_t soc_con11;                          /* address offset: 0x002c */
     uint32_t biu_con[2];                         /* address offset: 0x0030 */
     uint32_t soc_con14;                          /* address offset: 0x0038 */
     uint32_t soc_con15;                          /* address offset: 0x003c */
     uint32_t soc_con16;                          /* address offset: 0x0040 */
     uint32_t soc_con17;                          /* address offset: 0x0044 */
     uint32_t soc_con18;                          /* address offset: 0x0048 */
     uint32_t soc_con19;                          /* address offset: 0x004c */
     uint32_t soc_con20;                          /* address offset: 0x0050 */
     uint32_t soc_con21;                          /* address offset: 0x0054 */
     uint32_t soc_con22;                          /* address offset: 0x0058 */
     uint32_t soc_con23;                          /* address offset: 0x005c */
     uint32_t soc_con24;                          /* address offset: 0x0060 */
     uint32_t soc_con25;                          /* address offset: 0x0064 */
     uint32_t soc_con26;                          /* address offset: 0x0068 */
     uint32_t soc_con27;                          /* address offset: 0x006c */
     uint32_t soc_con28;                          /* address offset: 0x0070 */
     uint32_t soc_con29;                          /* address offset: 0x0074 */
     uint32_t biu_con0;                           /* address offset: 0x0078 */
     uint32_t biu_con1;                           /* address offset: 0x007c */
     uint32_t biu_con2;                           /* address offset: 0x0080 */
     uint32_t biu_con3;                           /* address offset: 0x0084 */
     uint32_t reserved0088[6];                    /* address offset: 0x0088 */
     uint32_t mem_con0;                           /* address offset: 0x00a0 */
     uint32_t mem_con1;                           /* address offset: 0x00a4 */
     uint32_t mem_con2;                           /* address offset: 0x00a8 */
     uint32_t reserved00ac[21];                   /* address offset: 0x00ac */
     uint32_t biu_status[2];                      /* address offset: 0x0100 */
     uint32_t gic_con2;                           /* address offset: 0x0108 */
     uint32_t gic_con3;                           /* address offset: 0x010c */
     uint32_t gic_con4;                           /* address offset: 0x0110 */
     uint32_t reserved0114[11];                   /* address offset: 0x0114 */
     uint32_t soc_status0;                        /* address offset: 0x0140 */
     uint32_t soc_status1;                        /* address offset: 0x0144 */
     uint32_t soc_status2;                        /* address offset: 0x0148 */
     uint32_t reserved014c;                       /* address offset: 0x014c */
     uint32_t fw_bus_slv_mainstat;                /* address offset: 0x0150 */
     uint32_t fw_top_slv_mainstat;                /* address offset: 0x0154 */
     uint32_t reserved0158[6];                    /* address offset: 0x0158 */
     uint32_t pswitch_irq_det_en;                 /* address offset: 0x0170 */
     uint32_t pswitch_irq_status;                 /* address offset: 0x0174 */
     uint32_t reserved0178[2];                    /* address offset: 0x0178 */
     uint32_t soc_code;                           /* address offset: 0x0180 */
     uint32_t reserved0184[3];                    /* address offset: 0x0184 */
     uint32_t soc_version;                        /* address offset: 0x0190 */
     uint32_t reserved0194[3];                    /* address offset: 0x0194 */
     uint32_t chip_id;                            /* address offset: 0x01a0 */
     uint32_t reserved01a4[3];                    /* address offset: 0x01a4 */
     uint32_t chip_version;                       /* address offset: 0x01b0 */
};

check_member(rk3572_sys_grf_reg, chip_version, 0x01b0);

/* ufs_grf register structure define */
struct rk3572_ufs_grf_reg {
     uint32_t clk_ctrl;                           /* address offset: 0x0000 */
     uint32_t uic_src_sel;                        /* address offset: 0x0004 */
     uint32_t ufs_state_ie;                       /* address offset: 0x0008 */
     uint32_t ufs_state_is;                       /* address offset: 0x000c */
     uint32_t ufs_state;                          /* address offset: 0x0010 */
     uint32_t reserved0014[13];                   /* address offset: 0x0014 */
};

check_member(rk3572_ufs_grf_reg, reserved0014, 0x0014);

#endif /*__SOC_ROCKCHIP_RK3572_GRF_H__ */
