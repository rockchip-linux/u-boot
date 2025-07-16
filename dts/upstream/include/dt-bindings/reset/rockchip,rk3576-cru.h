/* SPDX-License-Identifier: (GPL-2.0+ OR MIT) */
/*
 * Copyright (c) 2023 Rockchip Electronics Co. Ltd.
 * Author: Elaine Zhang <zhangqing@rock-chips.com>
 */

#ifndef _DT_BINDINGS_RESET_ROCKCHIP_RK3576_H
#define _DT_BINDINGS_RESET_ROCKCHIP_RK3576_H

/********Name=SOFTRST_CON01,Offset=0xA04********/
#define SRST_A_TOP_BIU			19
#define SRST_P_TOP_BIU			21
#define SRST_A_TOP_MID_BIU		22
#define SRST_A_SECURE_HIGH_BIU		23
#define SRST_H_TOP_BIU			30
/********Name=SOFTRST_CON02,Offset=0xA08********/
#define SRST_H_VO0VOP_CHANNEL_BIU	32
#define SRST_A_VO0VOP_CHANNEL_BIU	33
/********Name=SOFTRST_CON06,Offset=0xA18********/
#define SRST_BISRINTF			98
/********Name=SOFTRST_CON07,Offset=0xA1C********/
#define SRST_H_AUDIO_BIU		114
#define SRST_H_ASRC_2CH_0		115
#define SRST_H_ASRC_2CH_1		116
#define SRST_H_ASRC_4CH_0		117
#define SRST_H_ASRC_4CH_1		118
#define SRST_ASRC_2CH_0			119
#define SRST_ASRC_2CH_1			120
#define SRST_ASRC_4CH_0			121
#define SRST_ASRC_4CH_1			122
#define SRST_M_SAI0_8CH			124
#define SRST_H_SAI0_8CH			125
#define SRST_H_SPDIF_RX0		126
#define SRST_M_SPDIF_RX0		127
/********Name=SOFTRST_CON08,Offset=0xA20********/
#define SRST_H_SPDIF_RX1		128
#define SRST_M_SPDIF_RX1		129
#define SRST_M_SAI1_8CH			133
#define SRST_H_SAI1_8CH			134
#define SRST_M_SAI2_2CH			136
#define SRST_H_SAI2_2CH			138
#define SRST_M_SAI3_2CH			140
#define SRST_H_SAI3_2CH			142
/********Name=SOFTRST_CON09,Offset=0xA24********/
#define SRST_M_SAI4_2CH			144
#define SRST_H_SAI4_2CH			146
#define SRST_H_ACDCDIG_DSM		147
#define SRST_M_ACDCDIG_DSM		148
#define SRST_PDM1			149
#define SRST_H_PDM1			151
#define SRST_M_PDM1			152
#define SRST_H_SPDIF_TX0		153
#define SRST_M_SPDIF_TX0		154
#define SRST_H_SPDIF_TX1		155
#define SRST_M_SPDIF_TX1		156
/********Name=SOFTRST_CON11,Offset=0xA2C********/
#define SRST_A_BUS_BIU			179
#define SRST_P_BUS_BIU			180
#define SRST_P_CRU			181
#define SRST_H_CAN0			182
#define SRST_CAN0			183
#define SRST_H_CAN1			184
#define SRST_CAN1			185
#define SRST_P_INTMUX2BUS		188
#define SRST_P_VCCIO_IOC		189
#define SRST_H_BUS_BIU			190
#define SRST_KEY_SHIFT			191
/********Name=SOFTRST_CON12,Offset=0xA30********/
#define SRST_P_I2C1			192
#define SRST_P_I2C2			193
#define SRST_P_I2C3			194
#define SRST_P_I2C4			195
#define SRST_P_I2C5			196
#define SRST_P_I2C6			197
#define SRST_P_I2C7			198
#define SRST_P_I2C8			199
#define SRST_P_I2C9			200
#define SRST_P_WDT_BUSMCU		201
#define SRST_T_WDT_BUSMCU		202
#define SRST_A_GIC			203
#define SRST_I2C1			204
#define SRST_I2C2			205
#define SRST_I2C3			206
#define SRST_I2C4			207
/********Name=SOFTRST_CON13,Offset=0xA34********/
#define SRST_I2C5			208
#define SRST_I2C6			209
#define SRST_I2C7			210
#define SRST_I2C8			211
#define SRST_I2C9			212
#define SRST_P_SARADC			214
#define SRST_SARADC			215
#define SRST_P_TSADC			216
#define SRST_TSADC			217
#define SRST_P_UART0			218
#define SRST_P_UART2			219
#define SRST_P_UART3			220
#define SRST_P_UART4			221
#define SRST_P_UART5			222
#define SRST_P_UART6			223
/********Name=SOFTRST_CON14,Offset=0xA38********/
#define SRST_P_UART7			224
#define SRST_P_UART8			225
#define SRST_P_UART9			226
#define SRST_P_UART10			227
#define SRST_P_UART11			228
#define SRST_S_UART0			229
#define SRST_S_UART2			230
#define SRST_S_UART3			233
#define SRST_S_UART4			236
#define SRST_S_UART5			239
/********Name=SOFTRST_CON15,Offset=0xA3C********/
#define SRST_S_UART6			242
#define SRST_S_UART7			245
#define SRST_S_UART8			248
#define SRST_S_UART9			249
#define SRST_S_UART10			250
#define SRST_S_UART11			251
#define SRST_P_SPI0			253
#define SRST_P_SPI1			254
#define SRST_P_SPI2			255
/********Name=SOFTRST_CON16,Offset=0xA40********/
#define SRST_P_SPI3			256
#define SRST_P_SPI4			257
#define SRST_SPI0			258
#define SRST_SPI1			259
#define SRST_SPI2			260
#define SRST_SPI3			261
#define SRST_SPI4			262
#define SRST_P_WDT0			263
#define SRST_T_WDT0			264
#define SRST_P_SYS_GRF			265
#define SRST_P_PWM1			266
#define SRST_PWM1			267

/********Name=SOFTRST_CON17,Offset=0xA44********/
#define SRST_P_BUSTIMER0		275
#define SRST_P_BUSTIMER1		276
#define SRST_TIMER0			278
#define SRST_TIMER1			279
#define SRST_TIMER2			280
#define SRST_TIMER3			281
#define SRST_TIMER4			282
#define SRST_TIMER5			283
#define SRST_P_BUSIOC			284
#define SRST_P_MAILBOX0			285
#define SRST_P_GPIO1			287
/********Name=SOFTRST_CON18,Offset=0xA48********/
#define SRST_GPIO1			288
#define SRST_P_GPIO2			289
#define SRST_GPIO2			290
#define SRST_P_GPIO3			291
#define SRST_GPIO3			292
#define SRST_P_GPIO4			293
#define SRST_GPIO4			294
#define SRST_A_DECOM			295
#define SRST_P_DECOM			296
#define SRST_D_DECOM			297
#define SRST_TIMER6			299
#define SRST_TIMER7			300
#define SRST_TIMER8			301
#define SRST_TIMER9			302
#define SRST_TIMER10			303
/********Name=SOFTRST_CON19,Offset=0xA4C********/
#define SRST_TIMER11			304
#define SRST_A_DMAC0			305
#define SRST_A_DMAC1			306
#define SRST_A_DMAC2			307
#define SRST_A_SPINLOCK			308
#define SRST_REF_PVTPLL_BUS		309
#define SRST_H_I3C0			311
#define SRST_H_I3C1			313
#define SRST_H_BUS_CM0_BIU		315
#define SRST_F_BUS_CM0_CORE		316
#define SRST_T_BUS_CM0_JTAG		317
/********Name=SOFTRST_CON20,Offset=0xA50********/
#define SRST_P_INTMUX2PMU		320
#define SRST_P_INTMUX2DDR		321
#define SRST_P_PVTPLL_BUS		323
#define SRST_P_PWM2			324
#define SRST_PWM2			325
#define SRST_FREQ_PWM1			328
#define SRST_COUNTER_PWM1		329
#define SRST_I3C0			332
#define SRST_I3C1			333
/********Name=SOFTRST_CON21,Offset=0xA54********/
#define SRST_P_DDR_MON_CH0		337
#define SRST_P_DDR_BIU			338
#define SRST_P_DDR_UPCTL_CH0		339
#define SRST_TM_DDR_MON_CH0		340
#define SRST_A_DDR_BIU			341
#define SRST_DFI_CH0			342
#define SRST_DDR_MON_CH0		346
#define SRST_P_DDR_HWLP_CH0		349
#define SRST_P_DDR_MON_CH1		350
#define SRST_P_DDR_HWLP_CH1		351
/********Name=SOFTRST_CON22,Offset=0xA58********/
#define SRST_P_DDR_UPCTL_CH1		352
#define SRST_TM_DDR_MON_CH1		353
#define SRST_DFI_CH1			354
#define SRST_A_DDR01_MSCH0		355
#define SRST_A_DDR01_MSCH1		356
#define SRST_DDR_MON_CH1		358
#define SRST_DDR_SCRAMBLE_CH0		361
#define SRST_DDR_SCRAMBLE_CH1		362
#define SRST_P_AHB2APB			364
#define SRST_H_AHB2APB			365
#define SRST_H_DDR_BIU			366
#define SRST_F_DDR_CM0_CORE		367
/********Name=SOFTRST_CON23,Offset=0xA5C********/
#define SRST_P_DDR01_MSCH0		369
#define SRST_P_DDR01_MSCH1		370
#define SRST_DDR_TIMER0			372
#define SRST_DDR_TIMER1			373
#define SRST_T_WDT_DDR			374
#define SRST_P_WDT			375
#define SRST_P_TIMER			376
#define SRST_T_DDR_CM0_JTAG		377
#define SRST_P_DDR_GRF			379
/********Name=SOFTRST_CON25,Offset=0xA64********/
#define SRST_DDR_UPCTL_CH0		401
#define SRST_A_DDR_UPCTL_0_CH0		402
#define SRST_A_DDR_UPCTL_1_CH0		403
#define SRST_A_DDR_UPCTL_2_CH0		404
#define SRST_A_DDR_UPCTL_3_CH0		405
#define SRST_A_DDR_UPCTL_4_CH0		406
/********Name=SOFTRST_CON26,Offset=0xA68********/
#define SRST_DDR_UPCTL_CH1		417
#define SRST_A_DDR_UPCTL_0_CH1		418
#define SRST_A_DDR_UPCTL_1_CH1		419
#define SRST_A_DDR_UPCTL_2_CH1		420
#define SRST_A_DDR_UPCTL_3_CH1		421
#define SRST_A_DDR_UPCTL_4_CH1		422
/********Name=SOFTRST_CON27,Offset=0xA6C********/
#define SRST_REF_PVTPLL_DDR		432
#define SRST_P_PVTPLL_DDR		433

/********Name=SOFTRST_CON28,Offset=0xA70********/
#define SRST_A_RKNN0			457
#define SRST_A_RKNN0_BIU		459
#define SRST_L_RKNN0_BIU		460
/********Name=SOFTRST_CON29,Offset=0xA74********/
#define SRST_A_RKNN1			464
#define SRST_A_RKNN1_BIU		466
#define SRST_L_RKNN1_BIU		467
/********Name=SOFTRST_CON31,Offset=0xA7C********/
#define SRST_NPU_DAP			496
#define SRST_L_NPUSUBSYS_BIU		497
#define SRST_P_NPUTOP_BIU		505
#define SRST_P_NPU_TIMER		506
#define SRST_NPUTIMER0			508
#define SRST_NPUTIMER1			509
#define SRST_P_NPU_WDT			510
#define SRST_T_NPU_WDT			511
/********Name=SOFTRST_CON32,Offset=0xA80********/
#define SRST_A_RKNN_CBUF		512
#define SRST_A_RVCORE0			513
#define SRST_P_NPU_GRF			514
#define SRST_P_PVTPLL_NPU		515
#define SRST_NPU_PVTPLL			516
#define SRST_H_NPU_CM0_BIU		518
#define SRST_F_NPU_CM0_CORE		519
#define SRST_T_NPU_CM0_JTAG		520
#define SRST_A_RKNNTOP_BIU		523
#define SRST_H_RKNN_CBUF		524
#define SRST_H_RKNNTOP_BIU		525
/********Name=SOFTRST_CON33,Offset=0xA84********/
#define SRST_H_NVM_BIU			530
#define SRST_A_NVM_BIU			531
#define SRST_S_FSPI			534
#define SRST_H_FSPI			535
#define SRST_C_EMMC			536
#define SRST_H_EMMC			537
#define SRST_A_EMMC			538
#define SRST_B_EMMC			539
#define SRST_T_EMMC			540
/********Name=SOFTRST_CON34,Offset=0xA88********/
#define SRST_P_GRF			545
#define SRST_P_PHP_BIU			549
#define SRST_A_PHP_BIU			553
#define SRST_P_PCIE0			557
#define SRST_PCIE0_POWER_UP		559
/********Name=SOFTRST_CON35,Offset=0xA8C********/
#define SRST_A_USB3OTG1			563
#define SRST_A_MMU0			571
#define SRST_A_SLV_MMU0			573
#define SRST_A_MMU1			574
/********Name=SOFTRST_CON36,Offset=0xA90********/
#define SRST_A_SLV_MMU1			576
#define SRST_P_PCIE1			583
#define SRST_PCIE1_POWER_UP		585
/********Name=SOFTRST_CON37,Offset=0xA94********/
#define SRST_RXOOB0			592
#define SRST_RXOOB1			593
#define SRST_PMALIVE0			594
#define SRST_PMALIVE1			595
#define SRST_A_SATA0			596
#define SRST_A_SATA1			597
#define SRST_ASIC1			598
#define SRST_ASIC0			599
/********Name=SOFTRST_CON40,Offset=0xAA0********/
#define SRST_P_CSIDPHY1			642
#define SRST_SCAN_CSIDPHY1		643
/********Name=SOFTRST_CON42,Offset=0xAA8********/
#define SRST_P_SDGMAC_GRF		675
#define SRST_P_SDGMAC_BIU		676
#define SRST_A_SDGMAC_BIU		677
#define SRST_H_SDGMAC_BIU		678
#define SRST_A_GMAC0			679
#define SRST_A_GMAC1			680
#define SRST_P_GMAC0			681
#define SRST_P_GMAC1			682
#define SRST_H_SDIO			684
/********Name=SOFTRST_CON43,Offset=0xAAC********/
#define SRST_H_SDMMC0			690
#define SRST_S_FSPI1			691
#define SRST_H_FSPI1			692
#define SRST_A_DSMC_BIU			694
#define SRST_A_DSMC			695
#define SRST_P_DSMC			696
#define SRST_H_HSGPIO			698
#define SRST_HSGPIO			699
#define SRST_A_HSGPIO			701
/********Name=SOFTRST_CON45,Offset=0xAB4********/
#define SRST_H_RKVDEC			723
#define SRST_H_RKVDEC_BIU		725
#define SRST_A_RKVDEC_BIU		726
#define SRST_RKVDEC_HEVC_CA		728
#define SRST_RKVDEC_CORE		729
/********Name=SOFTRST_CON47,Offset=0xABC********/
#define SRST_A_USB_BIU			755
#define SRST_P_USBUFS_BIU		756
#define SRST_A_USB3OTG0			757
#define SRST_A_UFS_BIU			762
#define SRST_A_MMU2			764
#define SRST_A_SLV_MMU2			765
#define SRST_A_UFS_SYS			767
/********Name=SOFTRST_CON48,Offset=0xAC0********/
#define SRST_A_UFS			768
#define SRST_P_USBUFS_GRF		769
#define SRST_P_UFS_GRF			770
/********Name=SOFTRST_CON49,Offset=0xAC4********/
#define SRST_H_VPU_BIU			790
#define SRST_A_JPEG_BIU			791
#define SRST_A_RGA_BIU			794
#define SRST_A_VDPP_BIU			795
#define SRST_A_EBC_BIU			796
#define SRST_H_RGA2E_0			797
#define SRST_A_RGA2E_0			798
#define SRST_CORE_RGA2E_0		799
/********Name=SOFTRST_CON50,Offset=0xAC8********/
#define SRST_A_JPEG			800
#define SRST_H_JPEG			801
#define SRST_H_VDPP			802
#define SRST_A_VDPP			803
#define SRST_CORE_VDPP			804
#define SRST_H_RGA2E_1			805
#define SRST_A_RGA2E_1			806
#define SRST_CORE_RGA2E_1		807
#define SRST_H_EBC			810
#define SRST_A_EBC			811
#define SRST_D_EBC			812
/********Name=SOFTRST_CON51,Offset=0xACC********/
#define SRST_H_VEPU0_BIU		818
#define SRST_A_VEPU0_BIU		819
#define SRST_H_VEPU0			820
#define SRST_A_VEPU0			821
#define SRST_VEPU0_CORE			822
/********Name=SOFTRST_CON53,Offset=0xAD4********/
#define SRST_A_VI_BIU			851
#define SRST_H_VI_BIU			852
#define SRST_P_VI_BIU			853
#define SRST_D_VICAP			854
#define SRST_A_VICAP			855
#define SRST_H_VICAP			856
#define SRST_ISP0			858
#define SRST_ISP0_VICAP			859
/********Name=SOFTRST_CON54,Offset=0xAD8********/
#define SRST_CORE_VPSS			865
#define SRST_P_CSI_HOST_0		868
#define SRST_P_CSI_HOST_1		869
#define SRST_P_CSI_HOST_2		870
#define SRST_P_CSI_HOST_3		871
#define SRST_P_CSI_HOST_4		872
/********Name=SOFTRST_CON59,Offset=0xAEC********/
#define SRST_CIFIN			944
#define SRST_VICAP_I0CLK		945
#define SRST_VICAP_I1CLK		946
#define SRST_VICAP_I2CLK		947
#define SRST_VICAP_I3CLK		948
#define SRST_VICAP_I4CLK		949
/********Name=SOFTRST_CON61,Offset=0xAF4********/
#define SRST_A_VOP_BIU			980
#define SRST_A_VOP2_BIU			981
#define SRST_H_VOP_BIU			982
#define SRST_P_VOP_BIU			983
#define SRST_H_VOP			984
#define SRST_A_VOP			985
#define SRST_D_VP0			989
/********Name=SOFTRST_CON62,Offset=0xAF8********/
#define SRST_D_VP1			992
#define SRST_D_VP2			993
#define SRST_P_VOP2_BIU			994
#define SRST_P_VOPGRF			995
/********Name=SOFTRST_CON63,Offset=0xAFC********/
#define SRST_H_VO0_BIU			1013
#define SRST_P_VO0_BIU			1015
#define SRST_A_HDCP0_BIU		1017
#define SRST_P_VO0_GRF			1018
#define SRST_A_HDCP0			1020
#define SRST_H_HDCP0			1021
#define SRST_HDCP0			1022
/********Name=SOFTRST_CON64,Offset=0xB00********/
#define SRST_P_DSIHOST0			1029
#define SRST_DSIHOST0			1030
#define SRST_P_HDMITX0			1031
#define SRST_HDMITX0_REF		1033
#define SRST_P_EDP0			1037
#define SRST_EDP0_24M			1038
/********Name=SOFTRST_CON65,Offset=0xB04********/
#define SRST_M_SAI5_8CH			1044
#define SRST_H_SAI5_8CH			1045
#define SRST_M_SAI6_8CH			1048
#define SRST_H_SAI6_8CH			1049
#define SRST_H_SPDIF_TX2		1050
#define SRST_M_SPDIF_TX2		1053
#define SRST_H_SPDIF_RX2		1054
#define SRST_M_SPDIF_RX2		1055
/********Name=SOFTRST_CON66,Offset=0xB08********/
#define SRST_H_SAI8_8CH			1056
#define SRST_M_SAI8_8CH			1058
/********Name=SOFTRST_CON67,Offset=0xB0C********/
#define SRST_H_VO1_BIU			1077
#define SRST_P_VO1_BIU			1078
#define SRST_M_SAI7_8CH			1081
#define SRST_H_SAI7_8CH			1082
#define SRST_H_SPDIF_TX3		1083
#define SRST_H_SPDIF_TX4		1084
#define SRST_H_SPDIF_TX5		1085
#define SRST_M_SPDIF_TX3		1086
/********Name=SOFTRST_CON68,Offset=0xB10********/
#define SRST_DP0			1088
#define SRST_P_VO1_GRF			1090
#define SRST_A_HDCP1_BIU		1091
#define SRST_A_HDCP1			1092
#define SRST_H_HDCP1			1093
#define SRST_HDCP1			1094
#define SRST_H_SAI9_8CH			1097
#define SRST_M_SAI9_8CH			1099
#define SRST_M_SPDIF_TX4		1100
#define SRST_M_SPDIF_TX5		1101
/********Name=SOFTRST_CON69,Offset=0xB14********/
#define SRST_GPU			1107
#define SRST_A_S_GPU_BIU		1110
#define SRST_A_M0_GPU_BIU		1111
#define SRST_P_GPU_BIU			1113
#define SRST_P_GPU_GRF			1117
#define SRST_GPU_PVTPLL			1118
#define SRST_P_PVTPLL_GPU		1119
/********Name=SOFTRST_CON72,Offset=0xB20********/
#define SRST_A_CENTER_BIU		1156
#define SRST_A_DMA2DDR			1157
#define SRST_A_DDR_SHAREMEM		1158
#define SRST_A_DDR_SHAREMEM_BIU		1159
#define SRST_H_CENTER_BIU		1160
#define SRST_P_CENTER_GRF		1161
#define SRST_P_DMA2DDR			1162
#define SRST_P_SHAREMEM			1163
#define SRST_P_CENTER_BIU		1164
/********Name=SOFTRST_CON75,Offset=0xB2C********/
#define SRST_LINKSYM_HDMITXPHY0		1201
/********Name=SOFTRST_CON78,Offset=0xB38********/
#define SRST_DP0_PIXELCLK		1249
#define SRST_PHY_DP0_TX			1250
#define SRST_DP1_PIXELCLK		1251
#define SRST_DP2_PIXELCLK		1252
/********Name=SOFTRST_CON79,Offset=0xB3C********/
#define SRST_H_VEPU1_BIU		1265
#define SRST_A_VEPU1_BIU		1266
#define SRST_H_VEPU1			1267
#define SRST_A_VEPU1			1268
#define SRST_VEPU1_CORE			1269

/********Name=PHPPHYSOFTRST_CON00,Offset=0x8A00********/
#define SRST_P_PHPPHY_CRU		131073
#define SRST_P_APB2ASB_SLV_CHIP_TOP	131075
#define SRST_P_PCIE2_COMBOPHY0		131077
#define SRST_P_PCIE2_COMBOPHY0_GRF	131078
#define SRST_P_PCIE2_COMBOPHY1		131079
#define SRST_P_PCIE2_COMBOPHY1_GRF	131080
/********Name=PHPPHYSOFTRST_CON01,Offset=0x8A04********/
#define SRST_PCIE0_PIPE_PHY		131093
#define SRST_PCIE1_PIPE_PHY		131096

/********Name=SECURENSSOFTRST_CON00,Offset=0x10A00********/
#define SRST_H_CRYPTO_NS		262147
#define SRST_H_TRNG_NS			262148
#define SRST_P_OTPC_NS			262152
#define SRST_OTPC_NS			262153

/********Name=PMU1SOFTRST_CON00,Offset=0x20A00********/
#define SRST_P_HDPTX_GRF		524288
#define SRST_P_HDPTX_APB		524289
#define SRST_P_MIPI_DCPHY		524290
#define SRST_P_DCPHY_GRF		524291
#define SRST_P_BOT0_APB2ASB		524292
#define SRST_P_BOT1_APB2ASB		524293
#define SRST_USB2DEBUG			524294
#define SRST_P_CSIPHY_GRF		524295
#define SRST_P_CSIPHY			524296
#define SRST_P_USBPHY_GRF_0		524297
#define SRST_P_USBPHY_GRF_1		524298
#define SRST_P_USBDP_GRF		524299
#define SRST_P_USBDPPHY			524300
#define SRST_USBDP_COMBO_PHY_INIT 524303
/********Name=PMU1SOFTRST_CON01,Offset=0x20A04********/
#define SRST_USBDP_COMBO_PHY_CMN	524304
#define SRST_USBDP_COMBO_PHY_LANE	524305
#define SRST_USBDP_COMBO_PHY_PCS	524306
#define SRST_M_MIPI_DCPHY		524307
#define SRST_S_MIPI_DCPHY		524308
#define SRST_SCAN_CSIPHY		524309
#define SRST_P_VCCIO6_IOC		524310
#define SRST_OTGPHY_0			524311
#define SRST_OTGPHY_1			524312
#define SRST_HDPTX_INIT			524313
#define SRST_HDPTX_CMN			524314
#define SRST_HDPTX_LANE			524315
#define SRST_HDMITXHPD			524317
/********Name=PMU1SOFTRST_CON02,Offset=0x20A08********/
#define SRST_MPHY_INIT			524320
#define SRST_P_MPHY_GRF			524321
#define SRST_P_VCCIO7_IOC		524323
/********Name=PMU1SOFTRST_CON03,Offset=0x20A0C********/
#define SRST_H_PMU1_BIU			524345
#define SRST_P_PMU1_NIU			524346
#define SRST_H_PMU_CM0_BIU		524347
#define SRST_PMU_CM0_CORE		524348
#define SRST_PMU_CM0_JTAG		524349
/********Name=PMU1SOFTRST_CON04,Offset=0x20A10********/
#define SRST_P_CRU_PMU1			524353
#define SRST_P_PMU1_GRF			524355
#define SRST_P_PMU1_IOC			524356
#define SRST_P_PMU1WDT			524357
#define SRST_T_PMU1WDT			524358
#define SRST_P_PMUTIMER			524359
#define SRST_PMUTIMER0			524361
#define SRST_PMUTIMER1			524362
#define SRST_P_PMU1PWM			524363
#define SRST_PMU1PWM			524364
/********Name=PMU1SOFTRST_CON05,Offset=0x20A14********/
#define SRST_P_I2C0			524369
#define SRST_I2C0			524371
#define SRST_S_UART1			525373
#define SRST_P_UART1			525374
#define SRST_PDM0			524381
#define SRST_H_PDM0			524383
/********Name=PMU1SOFTRST_CON06,Offset=0xA18********/
#define SRST_M_PDM0			524384
#define SRST_H_VAD			524385
/********Name=PMU1SOFTRST_CON07,Offset=0x20A1C********/
#define SRST_P_PMU0GRF			524404
#define SRST_P_PMU0IOC			524405
#define SRST_P_GPIO0			524406
#define SRST_DB_GPIO0			524407

#define SRST_NR_RSTS			(SRST_DB_GPIO0 + 1)
#endif
