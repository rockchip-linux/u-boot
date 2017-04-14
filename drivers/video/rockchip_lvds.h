#ifndef _ROCKCHIP_LVDS_H_
#define _ROCKCHIP_LVDS_H_

#define LVDS_FMT_MASK				(0x07 << 16)
#define LVDS_MSB				BIT(3)
#define LVDS_DUAL				BIT(4)
#define LVDS_FMT_1				BIT(5)
#define LVDS_TTL_EN				BIT(6)
#define LVDS_START_PHASE_RST_1			BIT(7)
#define LVDS_DCLK_INV				BIT(8)
#define LVDS_CH0_EN				BIT(11)
#define LVDS_CH1_EN				BIT(12)
#define LVDS_PWRDN				BIT(15)

#define LVDS_24BIT				(0 << 1)
#define LVDS_18BIT				(1 << 1)
#define LVDS_FORMAT_VESA			(0 << 0)
#define LVDS_FORMAT_JEIDA			(1 << 0)

#define DISPLAY_OUTPUT_RGB		0
#define DISPLAY_OUTPUT_LVDS		1
#define DISPLAY_OUTPUT_DUAL_LVDS	2

#define BITS(x, bit)            ((x) << (bit))
#define BITS_MASK(x, mask, bit)  BITS((x) & (mask), bit)
#define BITS_EN(mask, bit)       BITS(mask, bit + 16)

/* RK3368_GRF_SOC_CON7 */
#define v_RK3368_LVDS_OUTPUT_FORMAT(x) (BITS_MASK(x, 3, 13) | BITS_EN(3, 13))
#define v_RK3368_LVDS_MSBSEL(x)        (BITS_MASK(x, 1, 11) | BITS_EN(1, 11))
#define v_RK3368_LVDSMODE_EN(x)        (BITS_MASK(x, 1, 12) | BITS_EN(1, 12))
#define v_RK3368_MIPIPHY_TTL_EN(x)     (BITS_MASK(x, 1, 15) | BITS_EN(1, 15))
#define v_RK3368_MIPIPHY_LANE0_EN(x)   (BITS_MASK(x, 1, 5) | BITS_EN(1, 5))
#define v_RK3368_MIPIDPI_FORCEX_EN(x)  (BITS_MASK(x, 1, 6) | BITS_EN(1, 6))
enum {
	LVDS_DATA_FROM_LCDC = 0,
	LVDS_DATA_FORM_EBC,
};

enum {
	LVDS_MSB_D0 = 0,
	LVDS_MSB_D7,
};

#define MIPIPHY_REG0            0x0000
#define m_LANE_EN_0             BITS(1, 2)
#define m_LANE_EN_1             BITS(1, 3)
#define m_LANE_EN_2             BITS(1, 4)
#define m_LANE_EN_3             BITS(1, 5)
#define m_LANE_EN_CLK           BITS(1, 5)
#define v_LANE_EN_0(x)          BITS(1, 2)
#define v_LANE_EN_1(x)          BITS(1, 3)
#define v_LANE_EN_2(x)          BITS(1, 4)
#define v_LANE_EN_3(x)          BITS(1, 5)
#define v_LANE_EN_CLK(x)        BITS(1, 5)

#define MIPIPHY_REG1            0x0004
#define m_SYNC_RST              BITS(1, 0)
#define m_LDO_PWR_DOWN          BITS(1, 1)
#define m_PLL_PWR_DOWN          BITS(1, 2)
#define v_SYNC_RST(x)           BITS_MASK(x, 1, 0)
#define v_LDO_PWR_DOWN(x)       BITS_MASK(x, 1, 1)
#define v_PLL_PWR_DOWN(x)       BITS_MASK(x, 1, 2)

#define MIPIPHY_REG3		0x000c
#define m_PREDIV                BITS(0x1f, 0)
#define m_FBDIV_MSB             BITS(1, 5)
#define v_PREDIV(x)             BITS_MASK(x, 0x1f, 0)
#define v_FBDIV_MSB(x)          BITS_MASK(x, 1, 5)

#define MIPIPHY_REG4		0x0010
#define v_FBDIV_LSB(x)          BITS_MASK(x, 0xff, 0)

#define MIPIPHY_REGE0		0x0380
#define m_MSB_SEL               BITS(1, 0)
#define m_DIG_INTER_RST         BITS(1, 2)
#define m_LVDS_MODE_EN          BITS(1, 5)
#define m_TTL_MODE_EN           BITS(1, 6)
#define m_MIPI_MODE_EN          BITS(1, 7)
#define v_MSB_SEL(x)            BITS_MASK(x, 1, 0)
#define v_DIG_INTER_RST(x)      BITS_MASK(x, 1, 2)
#define v_LVDS_MODE_EN(x)       BITS_MASK(x, 1, 5)
#define v_TTL_MODE_EN(x)        BITS_MASK(x, 1, 6)
#define v_MIPI_MODE_EN(x)       BITS_MASK(x, 1, 7)

#define MIPIPHY_REGE1           0x0384
#define m_DIG_INTER_EN          BITS(1, 7)
#define v_DIG_INTER_EN(x)       BITS_MASK(x, 1, 7)

#define MIPIPHY_REGE3           0x038c
#define m_MIPI_EN               BITS(1, 0)
#define m_LVDS_EN               BITS(1, 1)
#define m_TTL_EN                BITS(1, 2)
#define v_MIPI_EN(x)            BITS_MASK(x, 1, 0)
#define v_LVDS_EN(x)            BITS_MASK(x, 1, 1)
#define v_TTL_EN(x)             BITS_MASK(x, 1, 2)

#define MIPIPHY_REGE4		0x0390
#define m_VOCM			BITS(3, 4)
#define m_DIFF_V		BITS(3, 6)

#define v_VOCM(x)		BITS_MASK(x, 3, 4)
#define v_DIFF_V(x)		BITS_MASK(x, 3, 6)

#define MIPIPHY_REGE8           0x03a0

#define MIPIPHY_REGEB           0x03ac
#define v_PLL_PWR_OFF(x)        BITS_MASK(x, 1, 2)
#define v_LANECLK_EN(x)         BITS_MASK(x, 1, 3)
#define v_LANE3_EN(x)           BITS_MASK(x, 1, 4)
#define v_LANE2_EN(x)           BITS_MASK(x, 1, 5)
#define v_LANE1_EN(x)           BITS_MASK(x, 1, 6)
#define v_LANE0_EN(x)           BITS_MASK(x, 1, 7)

#define LVDS_PMUGRF_BASE         0xff738000
#define v_RK3368_FORCE_JETAG(x) (BITS_MASK(x, 1, 13) | BITS_EN(1, 13))

#endif
