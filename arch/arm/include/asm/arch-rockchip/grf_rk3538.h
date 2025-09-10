/* SPDX-License-Identifier: GPL-2.0+ */
/*
 * (C) Copyright 2025 Rockchip Electronics Co., Ltd
 */

#ifndef __SOC_ROCKCHIP_RK3538_GRF_H__
#define __SOC_ROCKCHIP_RK3538_GRF_H__

struct rk3538_cpu_grf_reg {
     uint32_t cpu_con0;                           /* address offset: 0x0000 */
     uint32_t peri_addr;                          /* address offset: 0x0004 */
     uint32_t a55_mem_cfg_uhdspra;                /* address offset: 0x0008 */
     uint32_t a55_mem_cfg_dprf;                   /* address offset: 0x000c */
     uint32_t dsu_mem_cfg_uhdspra;                /* address offset: 0x0010 */
     uint32_t dsu_mem_cfg_dprf;                   /* address offset: 0x0014 */
     uint32_t autocs_hdmi2p1_con[5];              /* address offset: 0x0018 */
     uint32_t cpu_status0;                        /* address offset: 0x002c */
     uint32_t autocs_hdmi1p1_status;              /* address offset: 0x0030 */
};

check_member(rk3538_cpu_grf_reg, autocs_hdmi1p1_status, 0x0030);

struct rk3538_ddr_grf_reg {
     uint32_t con[23];                            /* address offset: 0x0000 */
     uint32_t reserved005c[9];                    /* address offset: 0x005c */
     uint32_t probe_ctrl;                         /* address offset: 0x0080 */
     uint32_t reserved0084[31];                   /* address offset: 0x0084 */
     uint32_t status[31];                         /* address offset: 0x0100 */
};

check_member(rk3538_ddr_grf_reg, status, 0x0100);

struct rk3538_gpu_grf_reg {
     uint32_t gpu_mem_grf_spra;                   /* address offset: 0x0000 */
     uint32_t gpu_mem_grf_dpra;                   /* address offset: 0x0004 */
     uint32_t gpu_grf_con[4];                     /* address offset: 0x0008 */
     uint32_t reserved0018[58];                   /* address offset: 0x0018 */
     uint32_t gpu_grf_status0;                    /* address offset: 0x0100 */
};

check_member(rk3538_gpu_grf_reg, gpu_grf_status0, 0x0100);

struct rk3538_pmu_grf_reg {
     uint32_t soc_con[7];                         /* address offset: 0x0000 */
     uint32_t reserved001c[73];                   /* address offset: 0x001c */
     uint32_t men_con0;                           /* address offset: 0x0140 */
     uint32_t reserved0144[3];                    /* address offset: 0x0144 */
     uint32_t soc_special0;                       /* address offset: 0x0150 */
     uint32_t reserved0154[7];                    /* address offset: 0x0154 */
     uint32_t soc_status0;                        /* address offset: 0x0170 */
     uint32_t reserved0174[47];                   /* address offset: 0x0174 */
     uint32_t reset_function_status;              /* address offset: 0x0230 */
     uint32_t reset_function_clr;                 /* address offset: 0x0234 */
     uint32_t reserved0238[82];                   /* address offset: 0x0238 */
     uint32_t sig_detect_con;                     /* address offset: 0x0380 */
     uint32_t reserved0384[3];                    /* address offset: 0x0384 */
     uint32_t sig_detect_status;                  /* address offset: 0x0390 */
     uint32_t reserved0394[3];                    /* address offset: 0x0394 */
     uint32_t sig_detect_status_clear;            /* address offset: 0x03a0 */
     uint32_t reserved03a4[23];                   /* address offset: 0x03a4 */
     uint32_t hdmi_hpd_int_con;                   /* address offset: 0x0400 */
     uint32_t hdmi_hpd_con;                       /* address offset: 0x0404 */
     uint32_t hdmi_hpd_st;                        /* address offset: 0x0408 */
     uint32_t reserved040c[765];                  /* address offset: 0x040c */
     uint32_t os_reg[24];                         /* address offset: 0x1000 */
     uint32_t reserved1060[103];                  /* address offset: 0x1060 */
     uint32_t os_tag_reg;                         /* address offset: 0x11fc */
};

check_member(rk3538_pmu_grf_reg, os_tag_reg, 0x11fc);

struct rk3538_phpl_grf_reg {
     uint32_t con0;                               /* address offset: 0x0000 */
     uint32_t gmac_io_buf_con;                    /* address offset: 0x0004 */
     uint32_t gmac_con;                           /* address offset: 0x0008 */
     uint32_t memcfg_gmac;                        /* address offset: 0x000c */
     uint32_t saradc_con[3];                      /* address offset: 0x0010 */
     uint32_t tsadc_con[7];                       /* address offset: 0x001c */
     uint32_t otp_con;                            /* address offset: 0x0038 */
     uint32_t reserved003c[5];                    /* address offset: 0x003c */
     uint32_t sdmmc1_detect_con;                  /* address offset: 0x0050 */
     uint32_t sdmmc1_detect_status_clear;         /* address offset: 0x0054 */
     uint32_t sdmmc1_det_counter;                 /* address offset: 0x0058 */
     uint32_t sdio_detect_con;                    /* address offset: 0x005c */
     uint32_t sdio_detect_status_clear;           /* address offset: 0x0060 */
     uint32_t sdio_det_counter;                   /* address offset: 0x0064 */
     uint32_t reserved0068[2];                    /* address offset: 0x0068 */
     uint32_t tsadc_ota_con[2];                   /* address offset: 0x0070 */
     uint32_t tsadc_ota_clr;                      /* address offset: 0x0078 */
     uint32_t reserved007c;                       /* address offset: 0x007c */
     uint32_t gmac_ack;                           /* address offset: 0x0080 */
     uint32_t reserved0084[31];                   /* address offset: 0x0084 */
     uint32_t status0;                            /* address offset: 0x0100 */
     uint32_t gmac_status[3];                     /* address offset: 0x0104 */
     uint32_t tsadc_status[2];                    /* address offset: 0x0110 */
     uint32_t tsadc_ota_status;                   /* address offset: 0x0118 */
};

check_member(rk3538_phpl_grf_reg, tsadc_ota_status, 0x0118);

struct rk3538_phpr_grf_reg {
     uint32_t con0;                               /* address offset: 0x0000 */
     uint32_t sdmmc0_detect_con;                  /* address offset: 0x0004 */
     uint32_t sdmmc0_detect_status_clear;         /* address offset: 0x0008 */
     uint32_t sdmmc0_det_counter;                 /* address offset: 0x000c */
     uint32_t memcfg_emmc;                        /* address offset: 0x0010 */
     uint32_t memcfg_nandc;                       /* address offset: 0x0014 */
     uint32_t sdcard_dectn_dly;                   /* address offset: 0x0018 */
     uint32_t reserved001c[57];                   /* address offset: 0x001c */
     uint32_t status0;                            /* address offset: 0x0100 */
};

check_member(rk3538_phpr_grf_reg, status0, 0x0100);

struct rk3538_rkvdec_grf_reg {
     uint32_t rkvdec_mem_grf_spra;                /* address offset: 0x0000 */
     uint32_t rkvdec_mem_grf_dpra;                /* address offset: 0x0004 */
};

check_member(rk3538_rkvdec_grf_reg, rkvdec_mem_grf_dpra, 0x0004);

struct rk3538_sys_grf_reg {
     uint32_t mem_grf_spra;                       /* address offset: 0x0000 */
     uint32_t mem_grf_dpra;                       /* address offset: 0x0004 */
     uint32_t con[3];                             /* address offset: 0x0008 */
     uint32_t gpio6_filter_con[6];                /* address offset: 0x0014 */
     uint32_t reserved002c;                       /* address offset: 0x002c */
     uint32_t biu_con[2];                         /* address offset: 0x0030 */
     uint32_t uart_grf_rts_cts;                   /* address offset: 0x0038 */
     uint32_t uart_grf_dma_bypass;                /* address offset: 0x003c */
     uint32_t audio_con0;                         /* address offset: 0x0040 */
     uint32_t reserved0044[47];                   /* address offset: 0x0044 */
     uint32_t biu_status[2];                      /* address offset: 0x0100 */
     uint32_t sys_status;                         /* address offset: 0x0108 */
     uint32_t reserved010c[445];                  /* address offset: 0x010c */
     uint32_t chip_id;                            /* address offset: 0x0800 */
     uint32_t chip_version;                       /* address offset: 0x0804 */
};

check_member(rk3538_sys_grf_reg, chip_version, 0x0804);

struct rk3538_vo_grf_reg {
     uint32_t mem_grf_spra;                       /* address offset: 0x0000 */
     uint32_t mem_grf_dpra;                       /* address offset: 0x0004 */
     uint32_t sai2acodec;                         /* address offset: 0x0008 */
     uint32_t reserved000c;                       /* address offset: 0x000c */
     uint32_t earc_dbg;                           /* address offset: 0x0010 */
     uint32_t con5;                               /* address offset: 0x0014 */
     uint32_t con_hdcp;                           /* address offset: 0x0018 */
     uint32_t usbphy_host_con0;                   /* address offset: 0x001c */
     uint32_t usb2phy_con0;                       /* address offset: 0x0020 */
     uint32_t usbphy_con[3];                      /* address offset: 0x0024 */
     uint32_t usb2host_con[2];                    /* address offset: 0x0030 */
     uint32_t usbotg_con[2];                      /* address offset: 0x0038 */
     uint32_t mac_dma_ack;                        /* address offset: 0x0040 */
     uint32_t mac_con;                            /* address offset: 0x0044 */
     uint32_t rkmacphy_calib_con;                 /* address offset: 0x0048 */
     uint32_t rkmacphy_grf_con[3];                /* address offset: 0x004c */
     uint32_t reserved0058[3];                    /* address offset: 0x0058 */
     uint32_t hdcp_misc;                          /* address offset: 0x0064 */
     uint32_t hdmi_misc;                          /* address offset: 0x0068 */
     uint32_t hdmi_switch;                        /* address offset: 0x006c */
     uint32_t hdmi_autocs_con[2];                 /* address offset: 0x0070 */
     uint32_t rkearcrx_con[4];                    /* address offset: 0x0078 */
     uint32_t reserved0088;                       /* address offset: 0x0088 */
     uint32_t usbphy_line_state_con;              /* address offset: 0x008c */
     uint32_t reserved0090[4];                    /* address offset: 0x0090 */
     uint32_t cvbs_disable;                       /* address offset: 0x00a0 */
     uint32_t cvbs_ctrl;                          /* address offset: 0x00a4 */
     uint32_t reserved00a8[6];                    /* address offset: 0x00a8 */
     uint32_t usbphy_int_en;                      /* address offset: 0x00c0 */
     uint32_t usbphy_int_st;                      /* address offset: 0x00c4 */
     uint32_t usbphy_int_st_clr;                  /* address offset: 0x00c8 */
     uint32_t usbphy_ls_con;                      /* address offset: 0x00cc */
     uint32_t usbphy_dis_con;                     /* address offset: 0x00d0 */
     uint32_t usbphy_bvalid_con;                  /* address offset: 0x00d4 */
     uint32_t usbphy_id_con;                      /* address offset: 0x00d8 */
     uint32_t usbphy_vbusvalid_con;               /* address offset: 0x00dc */
     uint32_t usb2hostphy_int_en;                 /* address offset: 0x00e0 */
     uint32_t usb2hostphy_int_st;                 /* address offset: 0x00e4 */
     uint32_t usb2hostphy_int_st_clr;             /* address offset: 0x00e8 */
     uint32_t usb2hostphy_ls_con;                 /* address offset: 0x00ec */
     uint32_t usb2hostphy_dis_con;                /* address offset: 0x00f0 */
     uint32_t usb2hostphy_bvalid_con;             /* address offset: 0x00f4 */
     uint32_t usb2hostphy_id_con;                 /* address offset: 0x00f8 */
     uint32_t reserved00fc;                       /* address offset: 0x00fc */
     uint32_t usbotg_status_lat[2];               /* address offset: 0x0100 */
     uint32_t usbotg_status_cb;                   /* address offset: 0x0108 */
     uint32_t reserved010c;                       /* address offset: 0x010c */
     uint32_t usbphy_st;                          /* address offset: 0x0110 */
     uint32_t usb2host_st;                        /* address offset: 0x0114 */
     uint32_t usbphy_utmi_st;                     /* address offset: 0x0118 */
     uint32_t hdmi_hdcp_misc_status;              /* address offset: 0x011c */
     uint32_t hdcp_diag_status;                   /* address offset: 0x0120 */
     uint32_t hdmitx_autocs_st;                   /* address offset: 0x0124 */
     uint32_t earcrx_term_status[2];              /* address offset: 0x0128 */
     uint32_t reserved0130[2];                    /* address offset: 0x0130 */
     uint32_t mac_grf_status[3];                  /* address offset: 0x0138 */
     uint32_t rkmacphy_grf_status;                /* address offset: 0x0144 */
};

check_member(rk3538_vo_grf_reg, rkmacphy_grf_status, 0x0144);

struct rk3538_vpu_grf_reg {
     uint32_t vpu_mem_grf_spra;                   /* address offset: 0x0000 */
     uint32_t vpu_mem_grf_dpra;                   /* address offset: 0x0004 */
     uint32_t vpu_mem_grf_rom;                    /* address offset: 0x0008 */
     uint32_t reserved000c[61];                   /* address offset: 0x000c */
     uint32_t vpu_grf_status0;                    /* address offset: 0x0100 */
};

check_member(rk3538_vpu_grf_reg, vpu_grf_status0, 0x0100);

#endif /*__SOC_ROCKCHIP_RK3538_GRF_H__ */
