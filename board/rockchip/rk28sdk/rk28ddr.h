/*
 * (C) Copyright 2009 rockchip
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */
#ifndef _RK28DDR_H_
#define _RK28DDR_H_

#define SDRAM_REG_BASE     (SDRAMC_BASE_ADDR)
#define DDR_REG_BASE       (SDRAMC_BASE_ADDR)

/* CPU_APB_REG4 */
#define MSDR_1_8V_ENABLE  (0x1 << 24)
#define READ_PIPE_ENABLE  (0x1 << 22)
#define EXIT_SELF_REFRESH (0x1 << 21)

/* CPU_APB_REG5 */
#define MEMTYPEMASK   (0x3 << 11) 
#define SDRAM         (0x0 << 11)
#define Mobile_SDRAM  (0x1 << 11)
#define DDRII         (0x2 << 11)
#define Mobile_DDR    (0x3 << 11)

/* SDRAM Config Register */
#define DATA_WIDTH_16     (0x0 << 13)
#define DATA_WIDTH_32     (0x1 << 13)
#define DATA_WIDTH_64     (0x2 << 13)
#define DATA_WIDTH_128    (0x3 << 13)

#define COL(n)            ((n-1) << 9)
#define ROW(n)            ((n-1) << 5)

#define BANK_2            (0 << 3)
#define BANK_4            (1 << 3)
#define BANK_8            (2 << 3)
#define BANK_16           (3 << 3) 

/* SDRAM Timing Register0 */
#define T_RC_SHIFT        (22)
#define T_RC_MAX         (0xF)
#define T_XSR_MSB_SHIFT   (27)
#define T_XSR_MSB_MASK    (0x1F)
#define T_XSR_LSB_SHIFT   (18)
#define T_XSR_LSB_MASK    (0xF)
#define T_RCAR_SHIFT      (14)
#define T_RCAR_MAX       (0xF)
#define T_WR_SHIFT        (12)
#define T_WR_MAX         (0x3)
#define T_RP_SHIFT        (9)
#define T_RP_MAX         (0x7)
#define T_RCD_SHIFT       (6)
#define T_RCD_MAX        (0x7)
#define T_RAS_SHIFT       (2)
#define T_RAS_MASK        (0xF)

#define CL_1              (0)
#define CL_2              (1)
#define CL_3              (2)
#define CL_4              (3)

/* SDRAM Timing Register1 */
#define AR_COUNT_SHIFT    (16)

/* SDRAM Control Regitster */
#define MSD_DEEP_POWERDOWN (1 << 20)
#define UPDATE_EMRS    (1 << 18)
#define OPEN_BANK_COUNT_SHIFT  (12)
#define SR_MODE            (1 << 11)
#define UPDATE_MRS         (1 << 9)
#define READ_PIPE_SHIFT    (6)
#define REFRESH_ALL_ROW_A  (1 << 5)
#define REFRESH_ALL_ROW_B  (1 << 4)
#define DELAY_PRECHARGE    (1 << 3)
#define SDR_POWERDOWN      (1 << 2)
#define ENTER_SELF_REFRESH (1 << 1)
#define SDR_INIT           (1 << 0)

/* Extended Mode Register */
#define DS_FULL            (0 << 5)
#define DS_1_2             (1 << 5)
#define DS_1_4             (2 << 5)
#define DS_1_8             (3 << 5)

#define TCSR_70            (0 << 3)
#define TCSR_45            (1 << 3)
#define TCSR_15            (2 << 3)
#define TCSR_85            (3 << 3)

#define PASR_4_BANK        (0)
#define PASR_2_BANK        (1)
#define PASR_1_BANK        (2)
#define PASR_1_2_BANK      (5)
#define PASR_1_4_BANK      (6)

/* SDRAM Controller register struct */
typedef volatile struct TagSDRAMC_REG
{
    volatile uint32 MSDR_SCONR;         //SDRAM configuration register
    volatile uint32 MSDR_STMG0R;        //SDRAM timing register0
    volatile uint32 MSDR_STMG1R;        //SDRAM timing register1
    volatile uint32 MSDR_SCTLR;         //SDRAM control register
    volatile uint32 MSDR_SREFR;         //SDRAM refresh register
    volatile uint32 MSDR_SCSLR0_LOW;    //Chip select register0(lower 32bits)
    volatile uint32 MSDR_SCSLR1_LOW;    //Chip select register1(lower 32bits)
    volatile uint32 MSDR_SCSLR2_LOW;    //Chip select register2(lower 32bits)
    uint32 reserved0[(0x54-0x1c)/4 - 1];
    volatile uint32 MSDR_SMSKR0;        //Mask register 0
    volatile uint32 MSDR_SMSKR1;        //Mask register 1
    volatile uint32 MSDR_SMSKR2;        //Mask register 2
    uint32 reserved1[(0x84-0x5c)/4 - 1];
    volatile uint32 MSDR_CSREMAP0_LOW;  //Remap register for chip select0(lower 32 bits)
    uint32 reserved2[(0x94-0x84)/4 - 1];
    volatile uint32 MSDR_SMTMGR_SET0;   //Static memory timing register Set0
    volatile uint32 MSDR_SMTMGR_SET1;   //Static memory timing register Set1
    volatile uint32 MSDR_SMTMGR_SET2;   //Static memory timing register Set2
    volatile uint32 MSDR_FLASH_TRPDR;   //FLASH memory tRPD timing register
    volatile uint32 MSDR_SMCTLR;        //Static memory control register
    uint32 reserved4;
    volatile uint32 MSDR_EXN_MODE_REG;  //Extended Mode Register
}SDRAMC_REG_T,*pSDRAMC_REG_T;


#define pSDR_Reg       ((pSDRAMC_REG_T)SDRAM_REG_BASE)
#define pDDR_Reg       ((pDDRC_REG_T)DDR_REG_BASE)
#define pSCU_Reg       ((pSCU_REG)SCU_BASE_ADDR)
#define pGRF_Reg       ((pGRF_REG)REG_FILE_BASE_ADDR)


#define DDR_MEM_TYPE()	        (pGRF_Reg->CPU_APB_REG0 & MEMTYPEMASK)
#define DDR_ENABLE_SLEEP()     do{pDDR_Reg->CTRL_REG_36 = 0x1F1F;}while(0)

#define ODT_DIS          (0x0)
#define ODT_75           (0x8)
#define ODT_150          (0x40)
#define ODT_50           (0x48)

/* CTRL_REG_01 */
#define EXP_BDW_OVFLOW   (0x1 << 24)  //port 3
#define LCDC_BDW_OVFLOW  (0x1 << 16)  //port 2

/* CTRL_REG_02 */
#define VIDEO_BDW_OVFLOW (0x1 << 24) //port 7
#define CEVA_BDW_OVFLOW  (0x1 << 16) //port 6
#define ARMI_BDW_OVFLOW  (0x1 << 8)  //port 5
#define ARMD_BDW_OVFLOW  (0x1 << 0)  //port 4

/* CTR_REG_03 */
#define CONCURRENTAP     (0x1 << 16)

/* CTRL_REG_04 */
#define SINGLE_ENDED_DQS  (0x0 << 24)
#define DIFFERENTIAL_DQS  (0x1 << 24)
#define DLL_BYPASS_EN     (0x1 << 16)

/* CTR_REG_05 */
#define CS1_BANK_4        (0x0 << 16)
#define CS1_BANK_8        (0x1 << 16)
#define CS0_BANK_4        (0x0 << 8)
#define CS0_BANK_8        (0x1 << 8)

/* CTR_REG_07 */
#define ODT_CL_3          (0x1 << 8)

/* CTR_REG_08 */
#define BUS_16BIT         (0x1 << 16)
#define BUS_32BIT         (0x0 << 16)

/* CTR_REG_10 */
#define EN_TRAS_LOCKOUT   (0x1 << 24)
#define DIS_TRAS_LOCKOUT  (0x0 << 24)

/* CTR_REG_12 */
#define AXI_MC_ASYNC      (0x0)
#define AXI_MC_2_1        (0x1)
#define AXI_MC_1_2        (0x2)
#define AXI_MC_SYNC       (0x3)

/* CTR_REG_13 */
#define LCDC_WR_PRIO(n)    ((n) << 24)
#define LCDC_RD_PRIO(n)    ((n) << 16)

/* CTR_REG_14 */
#define EXP_WR_PRIO(n)     ((n) << 16)
#define EXP_RD_PRIO(n)     ((n) << 8)

/* CTR_REG_15 */
#define ARMD_WR_PRIO(n)     ((n) << 8)
#define ARMD_RD_PRIO(n)     (n)
#define ARMI_RD_PRIO(n)     ((n) << 24)

/* CTR_REG_16 */
#define ARMI_WR_PRIO(n)     (n)
#define CEVA_WR_PRIO(n)     ((n) << 24)
#define CEVA_RD_PRIO(n)     ((n) << 16)

/* CTR_REG_17 */
#define VIDEO_WR_PRIO(n)     ((n) << 16)
#define VIDEO_RD_PRIO(n)     ((n) << 8)
#define CS_MAP(n)            ((n) << 24)

/* CTR_REG_18 */
#define CS0_RD_ODT_MASK      (0x3 << 24)
#define CS0_RD_ODT(n)        (0x1 << (24+(n)))
#define CS0_LOW_POWER_REF_EN (0x0 << 8)
#define CS0_LOW_POWER_REF_DIS (0x1 << 8)
#define CS1_LOW_POWER_REF_EN (0x0 << 9)
#define CS1_LOW_POWER_REF_DIS (0x1 << 9)

/* CTR_REG_19 */
#define CS0_ROW(n)           ((n) << 24)
#define CS1_WR_ODT_MASK      (0x3 << 16)
#define CS1_WR_ODT(n)        (0x1 << (16+(n)))
#define CS0_WR_ODT_MASK      (0x3 << 8)
#define CS0_WR_ODT(n)        (0x1 << (8+(n)))
#define CS1_RD_ODT_MASK      (0x3)
#define CS1_RD_ODT(n)        (0x1 << (n))

/* CTR_REG_20 */
#define CS1_ROW(n)           (n)
#define CL(n)                (((n)&0x7) << 16)

/* CTR_REG_21 */
#define CS0_COL(n)           (n)
#define CS1_COL(n)           ((n) << 8)

/* CTR_REG_23 */
#define TRRD(n)              (((n)&0x7) << 24)
#define TCKE(n)              (((n)&0x7) << 8)

/* CTR_REG_24 */
#define TWTR_CK(n)           (((n)&0x7) << 16)
#define TRTP(n)              ((n)&0x7)

/* CTR_REG_29 */
//CAS latency linear value
#define CL_L_1_0             (2)
#define CL_L_1_5             (3)
#define CL_L_2_0             (4)
#define CL_L_2_5             (5)
#define CL_L_3_0             (6)
#define CL_L_3_5             (7)
#define CL_L_4_0             (8)
#define CL_L_4_5             (9)
#define CL_L_5_0             (0xA)
#define CL_L_5_5             (0xB)
#define CL_L_6_0             (0xC)
#define CL_L_6_5             (0xD)
#define CL_L_7_0             (0xE)
#define CL_L_7_5             (0xF)

/* CTR_REG_34 */
#define CS0_TRP_ALL(n)       (((n)&0xF) << 24)
#define TRP(n)               (((n)&0xF) << 16)

/* CTR_REG_35 */
#define WL(n)                ((((n)&0xF) << 16) | (((n)&0xF) << 24))
#define TWTR(n)              (((n)&0xF) << 8)
#define CS1_TRP_ALL(n)       ((n)&0xF)

/* CTR_REG_37 */
#define TMRD(n)              ((n) << 24)
#define TDAL(n)              ((n) << 16)
#define TCKESR(n)            ((n) << 8)
#define TCCD(n)              (n)

/* CTR_REG_38 */
#define TRC(n)               ((n) << 24)
#define TFAW(n)              ((n) << 16)
#define TWR(n)               (n)

/* CTR_REG_40 */
#define EXP_BW_PER(n)        ((n) << 16)
#define LCDC_BW_PER(n)       (n)

/* CTR_REG_41 */
#define ARMI_BW_PER(n)       ((n) << 16)
#define ARMD_BW_PER(n)       (n)

/* CTR_REG_42 */
#define VIDEO_BW_PER(n)      ((n) << 16)
#define CEVA_BW_PER(n)       (n)

/* CTR_REG_43 */
#define TMOD(n)              (((n)&0xFF) << 24)

/* CTR_REG_44 */
#define TRFC(n)              ((n) << 16)
#define TRCD(n)              ((n) << 8)
#define TRAS_MIN(n)          (n)

/* CTR_REG_48 */
#define TPHYUPD_RESP(n)      (((n)&0x3FFF) << 16)
#define TCTRLUPD_MAX(n)      ((n)&0x3FFF)

/* CTR_REG_51 */
#define CS0_MR(n)            ((((n) & 0xFEF0) | 0x2) << 16)
#define TREF(n)              (n)

/* CTR_REG_52 */
#define CS0_EMRS_1(n)        (((n)&0xFFC7) << 16)
#define CS1_MR(n)            (((n) & 0xFEF0) | 0x2)

/* CTR_REG_53 */
#define CS0_EMRS_2(n)        ((n) << 16)
#define CS0_EMR(n)           ((n) << 16)
#define CS1_EMRS_1(n)        ((n)&0xFFC7)

/* CTR_REG_54 */
#define CS0_EMRS_3(n)        ((n) << 16)
#define CS1_EMRS_2(n)        (n)
#define CS1_EMR(n)           (n)

/* CTR_REG_55 */
#define CS1_EMRS_3(n)        (n)

/* CTR_REG_59 */
#define CS_MSK_0(n)          (((n)&0xFFFF) << 16)

/* CTR_REG_60 */
#define CS_VAL_0(n)          (((n)&0xFFFF) << 16)
#define CS_MSK_1(n)          ((n)&0xFFFF)

/* CTR_REG_61 */
#define CS_VAL_1(n)          ((n)&0xFFFF)

/* CTR_REG_62 */
#define MODE5_CNT(n)         (((n)&0xFFFF) << 16)
#define MODE4_CNT(n)         ((n)&0xFFFF)

/* CTR_REG_63 */
#define MODE1_2_CNT(n)        ((n)&0xFFFF)

/* CTR_REG_64 */
#define TCPD(n)              ((n) << 16)
#define MODE3_CNT(n)         (n)

/* CTR_REG_65 */
#define TPDEX(n)             ((n) << 16)
#define TDLL(n)              (n)

/* CTR_REG_66 */
#define TXSNR(n)             ((n) << 16)
#define TRAS_MAX(n)          (n)

/* CTR_REG_67 */
#define TXSR(n)              (n)

/* CTR_REG_68 */
#define TINIT(n)             (n)


/* DDR Controller register struct */
typedef volatile struct TagDDRC_REG
{
    volatile uint32 CTRL_REG_00;
    volatile uint32 CTRL_REG_01;
    volatile uint32 CTRL_REG_02;
    volatile uint32 CTRL_REG_03;
    volatile uint32 CTRL_REG_04;
    volatile uint32 CTRL_REG_05;
    volatile uint32 CTRL_REG_06;
    volatile uint32 CTRL_REG_07;
    volatile uint32 CTRL_REG_08;
    volatile uint32 CTRL_REG_09;
    volatile uint32 CTRL_REG_10;
    volatile uint32 CTRL_REG_11;
    volatile uint32 CTRL_REG_12;
    volatile uint32 CTRL_REG_13;
    volatile uint32 CTRL_REG_14;
    volatile uint32 CTRL_REG_15;
    volatile uint32 CTRL_REG_16;
    volatile uint32 CTRL_REG_17;
    volatile uint32 CTRL_REG_18;
    volatile uint32 CTRL_REG_19;
    volatile uint32 CTRL_REG_20;
    volatile uint32 CTRL_REG_21;
    volatile uint32 CTRL_REG_22;
    volatile uint32 CTRL_REG_23;
    volatile uint32 CTRL_REG_24;
    volatile uint32 CTRL_REG_25;
    volatile uint32 CTRL_REG_26;
    volatile uint32 CTRL_REG_27;
    volatile uint32 CTRL_REG_28;
    volatile uint32 CTRL_REG_29;
    volatile uint32 CTRL_REG_30;
    volatile uint32 CTRL_REG_31;
    volatile uint32 CTRL_REG_32;
    volatile uint32 CTRL_REG_33;
    volatile uint32 CTRL_REG_34;
    volatile uint32 CTRL_REG_35;
    volatile uint32 CTRL_REG_36;
    volatile uint32 CTRL_REG_37;
    volatile uint32 CTRL_REG_38;
    volatile uint32 CTRL_REG_39;
    volatile uint32 CTRL_REG_40;
    volatile uint32 CTRL_REG_41;
    volatile uint32 CTRL_REG_42;
    volatile uint32 CTRL_REG_43;
    volatile uint32 CTRL_REG_44;
    volatile uint32 CTRL_REG_45;
    volatile uint32 CTRL_REG_46;
    volatile uint32 CTRL_REG_47;
    volatile uint32 CTRL_REG_48;
    volatile uint32 CTRL_REG_49;
    volatile uint32 CTRL_REG_50;
    volatile uint32 CTRL_REG_51;
    volatile uint32 CTRL_REG_52;
    volatile uint32 CTRL_REG_53;
    volatile uint32 CTRL_REG_54;
    volatile uint32 CTRL_REG_55;
    volatile uint32 CTRL_REG_56;
    volatile uint32 CTRL_REG_57;
    volatile uint32 CTRL_REG_58;
    volatile uint32 CTRL_REG_59;
    volatile uint32 CTRL_REG_60;
    volatile uint32 CTRL_REG_61;
    volatile uint32 CTRL_REG_62;
    volatile uint32 CTRL_REG_63;
    volatile uint32 CTRL_REG_64;
    volatile uint32 CTRL_REG_65;
    volatile uint32 CTRL_REG_66;
    volatile uint32 CTRL_REG_67;
    volatile uint32 CTRL_REG_68;
    volatile uint32 CTRL_REG_69;
    volatile uint32 CTRL_REG_70;
    volatile uint32 CTRL_REG_71;
    volatile uint32 CTRL_REG_72;
    volatile uint32 CTRL_REG_73;
    volatile uint32 CTRL_REG_74;
    volatile uint32 CTRL_REG_75;
    volatile uint32 CTRL_REG_76;
    volatile uint32 CTRL_REG_77;
    volatile uint32 CTRL_REG_78;
    volatile uint32 CTRL_REG_79;
    volatile uint32 CTRL_REG_80;
    volatile uint32 CTRL_REG_81;
    volatile uint32 CTRL_REG_82;
    volatile uint32 CTRL_REG_83;
    volatile uint32 CTRL_REG_84;
    volatile uint32 CTRL_REG_85;
    volatile uint32 CTRL_REG_86;
    volatile uint32 CTRL_REG_87;
    volatile uint32 CTRL_REG_88;
    volatile uint32 CTRL_REG_89;
    volatile uint32 CTRL_REG_90;
    volatile uint32 CTRL_REG_91;
    volatile uint32 CTRL_REG_92;
    volatile uint32 CTRL_REG_93;
    volatile uint32 CTRL_REG_94;
    volatile uint32 CTRL_REG_95;
    volatile uint32 CTRL_REG_96;
    volatile uint32 CTRL_REG_97;
    volatile uint32 CTRL_REG_98;
    volatile uint32 CTRL_REG_99;
    volatile uint32 CTRL_REG_100;
    volatile uint32 CTRL_REG_101;
    volatile uint32 CTRL_REG_102;
    volatile uint32 CTRL_REG_103;
    volatile uint32 CTRL_REG_104;
    volatile uint32 CTRL_REG_105;
    volatile uint32 CTRL_REG_106;
    volatile uint32 CTRL_REG_107;
    volatile uint32 CTRL_REG_108;
    volatile uint32 CTRL_REG_109;
    volatile uint32 CTRL_REG_110;
    volatile uint32 CTRL_REG_111;
    volatile uint32 CTRL_REG_112;
    volatile uint32 CTRL_REG_113;
    volatile uint32 CTRL_REG_114;
    volatile uint32 CTRL_REG_115;
    volatile uint32 CTRL_REG_116;
    volatile uint32 CTRL_REG_117;
    volatile uint32 CTRL_REG_118;
    volatile uint32 CTRL_REG_119;
    volatile uint32 CTRL_REG_120;
    volatile uint32 CTRL_REG_121;
    volatile uint32 CTRL_REG_122;
    volatile uint32 CTRL_REG_123;
    volatile uint32 CTRL_REG_124;
    volatile uint32 CTRL_REG_125;
    volatile uint32 CTRL_REG_126;
    volatile uint32 CTRL_REG_127;
    volatile uint32 CTRL_REG_128;
    volatile uint32 CTRL_REG_129;
    volatile uint32 CTRL_REG_130;
    volatile uint32 CTRL_REG_131;
    volatile uint32 CTRL_REG_132;
    volatile uint32 CTRL_REG_133;
    volatile uint32 CTRL_REG_134;
    volatile uint32 CTRL_REG_135;
    volatile uint32 CTRL_REG_136;
    volatile uint32 CTRL_REG_137;
    volatile uint32 CTRL_REG_138;
    volatile uint32 CTRL_REG_139;
    volatile uint32 CTRL_REG_140;
    volatile uint32 CTRL_REG_141;
    volatile uint32 CTRL_REG_142;
    volatile uint32 CTRL_REG_143;
    volatile uint32 CTRL_REG_144;
    volatile uint32 CTRL_REG_145;
    volatile uint32 CTRL_REG_146;
    volatile uint32 CTRL_REG_147;
    volatile uint32 CTRL_REG_148;
    volatile uint32 CTRL_REG_149;
    volatile uint32 CTRL_REG_150;
    volatile uint32 CTRL_REG_151;
    volatile uint32 CTRL_REG_152;
    volatile uint32 CTRL_REG_153;
    volatile uint32 CTRL_REG_154;
    volatile uint32 CTRL_REG_155;
    volatile uint32 CTRL_REG_156;
    volatile uint32 CTRL_REG_157;
    volatile uint32 CTRL_REG_158;
    volatile uint32 CTRL_REG_159;
}DDRC_REG_T,*pDDRC_REG_T;


int getddrzise(void);

#endif /*_RK28DDR_H_*/