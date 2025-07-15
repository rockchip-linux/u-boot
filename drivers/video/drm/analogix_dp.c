/*
 * (C) Copyright 2008-2017 Fuzhou Rockchip Electronics Co., Ltd
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <config.h>
#include <common.h>
#include <errno.h>
#include <malloc.h>
#include <asm/unaligned.h>
#include <asm/io.h>
#include <dm/device.h>
#include <dm/of_access.h>
#include <dm/read.h>
#include <linux/bitfield.h>
#include <linux/list.h>
#include <linux/media-bus-format.h>
#include <syscon.h>
#include <asm/arch-rockchip/clock.h>
#include <asm/gpio.h>

#include "rockchip_display.h"
#include "rockchip_crtc.h"
#include "rockchip_connector.h"
#include "analogix_dp.h"

#define RK3588_GRF_VO1_CON0	0x0000
#define EDP_MODE		BIT(0)
#define RK3588_GRF_VO1_CON1	0x0004

/**
 * struct rockchip_dp_chip_data - splite the grf setting of kind of chips
 * @lcdsel_grf_reg: grf register offset of lcdc select
 * @lcdsel_big: reg value of selecting vop big for eDP
 * @lcdsel_lit: reg value of selecting vop little for eDP
 * @chip_type: specific chip type
 * @reg: register base address
 * @max_dclk_khz: the maximum supported dclk rate
 * @ssc: check if SSC is supported by source
 * @max_link_rate: max supported link rate
 * @max_lane_count: max supported lane count
 * @format_yuv: check if yuv color format is supported
 * @support_dp_mode: check if dp mode is supported
 * @max_bpc: max supported bpc which set to 8 by default
 */
struct rockchip_dp_chip_data {
	u32	lcdsel_grf_reg;
	u32	lcdsel_big;
	u32	lcdsel_lit;
	u32	chip_type;
	u32	reg;
	u32	max_dclk_khz;
	bool    ssc;

	u32 max_link_rate;
	u32 max_lane_count;
	bool format_yuv;
	bool support_dp_mode;
	u8 max_bpc;
};

static const struct analogix_dp_output_format possible_output_fmts[] = {
	{ MEDIA_BUS_FMT_RGB101010_1X30, DRM_COLOR_FORMAT_RGB444, 10 },
	{ MEDIA_BUS_FMT_RGB888_1X24, DRM_COLOR_FORMAT_RGB444, 8 },
	{ MEDIA_BUS_FMT_RGB666_1X24_CPADHI, DRM_COLOR_FORMAT_RGB444, 6 },
	{ MEDIA_BUS_FMT_YUV10_1X30, DRM_COLOR_FORMAT_YCRCB444, 10 },
	{ MEDIA_BUS_FMT_YUV8_1X24, DRM_COLOR_FORMAT_YCRCB444, 8},
	{ MEDIA_BUS_FMT_YUYV10_1X20, DRM_COLOR_FORMAT_YCRCB422, 10 },
	{ MEDIA_BUS_FMT_YUYV8_1X16, DRM_COLOR_FORMAT_YCRCB422, 8 },
};

static u8 analogix_dp_get_output_bpp(const struct analogix_dp_output_format *fmt)
{
	switch (fmt->color_format) {
	case DRM_COLOR_FORMAT_YCRCB422:
		return fmt->bpc * 2;
	case DRM_COLOR_FORMAT_RGB444:
	case DRM_COLOR_FORMAT_YCRCB444:
	default:
		return fmt->bpc * 3;
	}
}

static int
analogix_dp_enable_rx_to_enhanced_mode(struct analogix_dp_device *dp,
				       bool enable)
{
	u8 data;
	int ret;

	ret = drm_dp_dpcd_readb(&dp->aux, DP_LANE_COUNT_SET, &data);
	if (ret != 1)
		return ret;

	if (enable)
		ret = drm_dp_dpcd_writeb(&dp->aux, DP_LANE_COUNT_SET,
					 DP_LANE_COUNT_ENHANCED_FRAME_EN |
					 DPCD_LANE_COUNT_SET(data));
	else
		ret = drm_dp_dpcd_writeb(&dp->aux, DP_LANE_COUNT_SET,
					 DPCD_LANE_COUNT_SET(data));

	return ret < 0 ? ret : 0;
}

static int analogix_dp_set_enhanced_mode(struct analogix_dp_device *dp)
{
	bool enhanced_frame_en;
	u8 data;
	int ret;

	enhanced_frame_en = drm_dp_enhanced_frame_cap(dp->dpcd);

	ret = analogix_dp_enable_rx_to_enhanced_mode(dp, enhanced_frame_en);
	if (ret < 0)
		return ret;

	if (!enhanced_frame_en) {
		/*
		 * As the Table 3-4 in eDP v1.2 spec:
		 * DPCD 0000Dh:
		 * Bit 1 = FRAMING_CHANGE_CAPABLE
		 * A setting of 1 indicates that this is an eDP device that
		 * uses only Enhanced Framing, independently of the setting by
		 * the source of ENHANCED_FRAME_EN
		 *
		 * And as the Table 3-3 in eDP v1.4 spec:
		 * DPCD 0000Dh:
		 * Bit 1 = RESERVED for eDP
		 * New to eDP v1.4.(Read all 0s)
		 */
		ret = drm_dp_dpcd_readb(&dp->aux, DP_EDP_CONFIGURATION_CAP,
					&data);
		if (ret < 0)
			return ret;

		enhanced_frame_en = !!(data & DP_FRAMING_CHANGE_CAP);
	}

	analogix_dp_enable_enhanced_mode(dp, enhanced_frame_en);

	return 0;
}

static int analogix_dp_training_pattern_dis(struct analogix_dp_device *dp)
{
	int ret;

	analogix_dp_set_training_pattern(dp, DP_NONE);

	ret = drm_dp_dpcd_writeb(&dp->aux, DP_TRAINING_PATTERN_SET,
				 DP_TRAINING_PATTERN_DISABLE);

	return ret < 0 ? ret : 0;
}

static int analogix_dp_enable_sink_to_assr_mode(struct analogix_dp_device *dp, bool enable)
{
	u8 data;
	int ret;

	ret = drm_dp_dpcd_readb(&dp->aux, DP_EDP_CONFIGURATION_SET, &data);
	if (ret != 1)
		return ret;

	if (enable)
		ret = drm_dp_dpcd_writeb(&dp->aux, DP_EDP_CONFIGURATION_SET,
					 data | DP_ALTERNATE_SCRAMBLER_RESET_ENABLE);
	else
		ret = drm_dp_dpcd_writeb(&dp->aux, DP_EDP_CONFIGURATION_SET,
					 data & ~DP_ALTERNATE_SCRAMBLER_RESET_ENABLE);

	return ret < 0 ? ret : 0;
}

static int analogix_dp_set_assr_mode(struct analogix_dp_device *dp)
{
	bool assr_en;
	int ret;

	assr_en = drm_dp_alternate_scrambler_reset_cap(dp->dpcd);

	ret = analogix_dp_enable_sink_to_assr_mode(dp, assr_en);
	if (ret < 0)
		return ret;

	analogix_dp_enable_assr_mode(dp, assr_en);

	return 0;
}

static int analogix_dp_link_start(struct analogix_dp_device *dp)
{
	u8 buf[4];
	int lane, lane_count, retval;

	lane_count = dp->link_train.lane_count;

	dp->link_train.lt_state = CLOCK_RECOVERY;
	dp->link_train.eq_loop = 0;

	for (lane = 0; lane < lane_count; lane++)
		dp->link_train.cr_loop[lane] = 0;

	/* Set link rate and count as you want to establish */
	analogix_dp_set_link_bandwidth(dp, dp->link_train.link_rate);
	analogix_dp_set_lane_count(dp, dp->link_train.lane_count);

	if (dp->nr_link_rate_table) {
		/* Setup DP_LINK_RATE_SET for eDP 1.4 and later */
		drm_dp_dpcd_writeb(&dp->aux, DP_LANE_COUNT_SET, dp->link_train.lane_count);
		drm_dp_dpcd_writeb(&dp->aux, DP_LINK_RATE_SET, dp->link_rate_select);
	} else {
		/* Setup DP_LINK_BW_SET for eDP 1.3 and earlier */
		buf[0] = dp->link_train.link_rate;
		buf[1] = dp->link_train.lane_count;
		retval = drm_dp_dpcd_write(&dp->aux, DP_LINK_BW_SET, buf, 2);
		if (retval < 0)
			return retval;
	}

	/* Spread AMP if required, enable 8b/10b coding */
	buf[0] = analogix_dp_ssc_supported(dp) ? DP_SPREAD_AMP_0_5 : 0;
	buf[1] = DP_SET_ANSI_8B10B;
	retval = drm_dp_dpcd_write(&dp->aux, DP_DOWNSPREAD_CTRL, buf, 2);
	if (retval < 0)
		return retval;

	/* set ASSR if available */
	retval = analogix_dp_set_assr_mode(dp);
	if (retval < 0) {
		dev_err(dp->dev, "failed to set assr mode\n");
		return retval;
	}

	/* set enhanced mode if available */
	retval = analogix_dp_set_enhanced_mode(dp);
	if (retval < 0) {
		dev_err(dp->dev, "failed to set enhance mode\n");
		return retval;
	}

	/* Set TX voltage-swing and pre-emphasis to minimum */
	for (lane = 0; lane < lane_count; lane++)
		dp->link_train.training_lane[lane] =
				DP_TRAIN_VOLTAGE_SWING_LEVEL_0 |
				DP_TRAIN_PRE_EMPH_LEVEL_0;
	analogix_dp_set_lane_link_training(dp);

	/* Set training pattern 1 */
	analogix_dp_set_training_pattern(dp, TRAINING_PTN1);

	/* Set RX training pattern */
	retval = drm_dp_dpcd_writeb(&dp->aux, DP_TRAINING_PATTERN_SET,
				    DP_LINK_SCRAMBLING_DISABLE | DP_TRAINING_PATTERN_1);
	if (retval < 0)
		return retval;

	for (lane = 0; lane < lane_count; lane++)
		buf[lane] = DP_TRAIN_PRE_EMPH_LEVEL_0 |
			    DP_TRAIN_VOLTAGE_SWING_LEVEL_0;

	retval = drm_dp_dpcd_write(&dp->aux, DP_TRAINING_LANE0_SET, buf,
				   lane_count);
	if (retval < 0)
		return retval;

	return 0;
}

static unsigned char analogix_dp_get_lane_status(u8 link_status[2], int lane)
{
	int shift = (lane & 1) * 4;
	u8 link_value = link_status[lane >> 1];

	return (link_value >> shift) & 0xf;
}

static int analogix_dp_clock_recovery_ok(u8 link_status[2], int lane_count)
{
	int lane;
	u8 lane_status;

	for (lane = 0; lane < lane_count; lane++) {
		lane_status = analogix_dp_get_lane_status(link_status, lane);
		if ((lane_status & DP_LANE_CR_DONE) == 0)
			return -EINVAL;
	}
	return 0;
}

static int analogix_dp_channel_eq_ok(u8 link_status[2], u8 link_align,
				     int lane_count)
{
	int lane;
	u8 lane_status;

	if ((link_align & DP_INTERLANE_ALIGN_DONE) == 0)
		return -EINVAL;

	for (lane = 0; lane < lane_count; lane++) {
		lane_status = analogix_dp_get_lane_status(link_status, lane);
		lane_status &= DP_CHANNEL_EQ_BITS;
		if (lane_status != DP_CHANNEL_EQ_BITS)
			return -EINVAL;
	}

	return 0;
}

static unsigned char
analogix_dp_get_adjust_request_voltage(u8 adjust_request[2], int lane)
{
	int shift = (lane & 1) * 4;
	u8 link_value = adjust_request[lane >> 1];

	return (link_value >> shift) & 0x3;
}

static unsigned char analogix_dp_get_adjust_request_pre_emphasis(
					u8 adjust_request[2],
					int lane)
{
	int shift = (lane & 1) * 4;
	u8 link_value = adjust_request[lane >> 1];

	return ((link_value >> shift) & 0xc) >> 2;
}

static void analogix_dp_reduce_link_rate(struct analogix_dp_device *dp)
{
	analogix_dp_training_pattern_dis(dp);
	analogix_dp_set_enhanced_mode(dp);

	dp->link_train.lt_state = FAILED;
}

static void analogix_dp_get_adjust_training_lane(struct analogix_dp_device *dp,
						 u8 adjust_request[2])
{
	int lane, lane_count;
	u8 voltage_swing, pre_emphasis, training_lane;

	lane_count = dp->link_train.lane_count;
	for (lane = 0; lane < lane_count; lane++) {
		voltage_swing = analogix_dp_get_adjust_request_voltage(
						adjust_request, lane);
		pre_emphasis = analogix_dp_get_adjust_request_pre_emphasis(
						adjust_request, lane);
		training_lane = DPCD_VOLTAGE_SWING_SET(voltage_swing) |
				DPCD_PRE_EMPHASIS_SET(pre_emphasis);

		if (voltage_swing == VOLTAGE_LEVEL_3)
			training_lane |= DP_TRAIN_MAX_SWING_REACHED;
		if (pre_emphasis == PRE_EMPHASIS_LEVEL_3)
			training_lane |= DP_TRAIN_MAX_PRE_EMPHASIS_REACHED;

		dp->link_train.training_lane[lane] = training_lane;
	}
}

static bool analogix_dp_tps3_supported(struct analogix_dp_device *dp)
{
	bool source_tps3_supported, sink_tps3_supported;
	u8 dpcd = 0;

	source_tps3_supported =
		dp->video_info.max_link_rate == DP_LINK_BW_5_4;
	drm_dp_dpcd_readb(&dp->aux, DP_MAX_LANE_COUNT, &dpcd);
	sink_tps3_supported = dpcd & DP_TPS3_SUPPORTED;

	return source_tps3_supported && sink_tps3_supported;
}

static int analogix_dp_process_clock_recovery(struct analogix_dp_device *dp)
{
	int lane, lane_count, retval;
	u8 voltage_swing, pre_emphasis, training_lane;
	u8 link_status[2], adjust_request[2];
	u8 training_pattern = TRAINING_PTN2;

	drm_dp_link_train_clock_recovery_delay(dp->dpcd);

	lane_count = dp->link_train.lane_count;

	retval =  drm_dp_dpcd_read(&dp->aux, DP_LANE0_1_STATUS, link_status, 2);
	if (retval < 0)
		return retval;

	if (analogix_dp_clock_recovery_ok(link_status, lane_count) == 0) {
		if (analogix_dp_tps3_supported(dp))
			training_pattern = TRAINING_PTN3;

		/* set training pattern for EQ */
		analogix_dp_set_training_pattern(dp, training_pattern);

		retval = drm_dp_dpcd_writeb(&dp->aux, DP_TRAINING_PATTERN_SET,
					    DP_LINK_SCRAMBLING_DISABLE |
					    (training_pattern == TRAINING_PTN3 ?
					     DP_TRAINING_PATTERN_3 : DP_TRAINING_PATTERN_2));
		if (retval < 0)
			return retval;

		dev_info(dp->dev, "Link Training Clock Recovery success\n");
		dp->link_train.lt_state = EQUALIZER_TRAINING;

		return 0;
	} else {
		retval = drm_dp_dpcd_read(&dp->aux, DP_ADJUST_REQUEST_LANE0_1,
					  adjust_request, 2);
		if (retval < 0)
			return retval;

		for (lane = 0; lane < lane_count; lane++) {
			training_lane = analogix_dp_get_lane_link_training(
							dp, lane);
			voltage_swing = analogix_dp_get_adjust_request_voltage(
							adjust_request, lane);
			pre_emphasis = analogix_dp_get_adjust_request_pre_emphasis(
							adjust_request, lane);

			if (DPCD_VOLTAGE_SWING_GET(training_lane) ==
					voltage_swing &&
			    DPCD_PRE_EMPHASIS_GET(training_lane) ==
					pre_emphasis)
				dp->link_train.cr_loop[lane]++;

			/*
			 * In DP spec 1.3, Condition of CR fail are
			 * outlined in section 3.5.1.2.2.1, figure 3-20:
			 *
			 * 1. Maximum Voltage Swing reached
			 * 2. Same Voltage five times
			 */
			if (dp->link_train.cr_loop[lane] == MAX_CR_LOOP ||
			    DPCD_VOLTAGE_SWING_GET(training_lane) == VOLTAGE_LEVEL_3) {
				dev_err(dp->dev, "CR Max reached (%d,%d,%d)\n",
					dp->link_train.cr_loop[lane],
					voltage_swing, pre_emphasis);
				analogix_dp_reduce_link_rate(dp);
				return -EIO;
			}
		}
	}

	analogix_dp_get_adjust_training_lane(dp, adjust_request);
	analogix_dp_set_lane_link_training(dp);

	retval = drm_dp_dpcd_write(&dp->aux, DP_TRAINING_LANE0_SET,
				   dp->link_train.training_lane, lane_count);
	if (retval < 0)
		return retval;

	return 0;
}

static int analogix_dp_process_equalizer_training(struct analogix_dp_device *dp)
{
	int lane_count, retval;
	u32 reg;
	u8 link_align, link_status[2], adjust_request[2];

	drm_dp_link_train_channel_eq_delay(dp->dpcd);

	lane_count = dp->link_train.lane_count;

	retval = drm_dp_dpcd_read(&dp->aux, DP_LANE0_1_STATUS, link_status, 2);
	if (retval < 0)
		return retval;

	if (analogix_dp_clock_recovery_ok(link_status, lane_count)) {
		analogix_dp_reduce_link_rate(dp);
		return -EIO;
	}

	retval = drm_dp_dpcd_readb(&dp->aux, DP_LANE_ALIGN_STATUS_UPDATED, &link_align);
	if (retval < 0)
		return retval;

	if (!analogix_dp_channel_eq_ok(link_status, link_align, lane_count)) {
		/* traing pattern Set to Normal */
		retval = analogix_dp_training_pattern_dis(dp);
		if (retval < 0)
			return retval;

		printf("Link Training success!\n");

		analogix_dp_get_link_bandwidth(dp, &reg);
		dp->link_train.link_rate = reg;
		analogix_dp_get_lane_count(dp, &reg);
		dp->link_train.lane_count = reg;

		printf("final link rate = 0x%.2x, lane count = 0x%.2x\n",
		       dp->link_train.link_rate, dp->link_train.lane_count);

		dp->link_train.lt_state = FINISHED;

		return 0;
	}

	/* not all locked */
	dp->link_train.eq_loop++;

	if (dp->link_train.eq_loop > MAX_EQ_LOOP) {
		dev_dbg(dp->dev, "EQ Max loop\n");
		analogix_dp_reduce_link_rate(dp);
		return -EIO;
	}

	retval = drm_dp_dpcd_read(&dp->aux, DP_ADJUST_REQUEST_LANE0_1, adjust_request, 2);
	if (retval < 0)
		return retval;

	analogix_dp_get_adjust_training_lane(dp, adjust_request);
	analogix_dp_set_lane_link_training(dp);

	retval = drm_dp_dpcd_write(&dp->aux, DP_TRAINING_LANE0_SET,
				   dp->link_train.training_lane, lane_count);
	if (retval < 0)
		return retval;

	return 0;
}

static bool analogix_dp_bandwidth_ok(struct analogix_dp_device *dp,
				     const struct drm_display_mode *mode, u32 bpp,
				     unsigned int rate, unsigned int lanes)
{
	u32 max_bw, req_bw;

	req_bw = mode->clock * bpp / 8;
	max_bw = lanes * rate;
	if (req_bw > max_bw)
		return false;

	return true;
}

static bool analogix_dp_link_config_validate(u8 link_rate, u8 lane_count)
{
	switch (link_rate) {
	case DP_LINK_BW_1_62:
	case DP_LINK_BW_2_7:
	case DP_LINK_BW_5_4:
	/* Supported link rate in eDP 1.4 */
	case EDP_LINK_BW_2_16:
	case EDP_LINK_BW_2_43:
	case EDP_LINK_BW_3_24:
	case EDP_LINK_BW_4_32:
		break;
	default:
		return false;
	}

	switch (lane_count) {
	case LANE_COUNT1:
	case LANE_COUNT2:
	case LANE_COUNT4:
		break;
	default:
		return false;
	}

	return true;
}

static int analogix_dp_select_link_rate_from_table(struct analogix_dp_device *dp)
{
	int i;
	u8 bw_code;
	u32 max_link_rate = drm_dp_bw_code_to_link_rate(dp->video_info.max_link_rate);

	for (i = 0; i < dp->nr_link_rate_table; i++) {
		bw_code =  drm_dp_link_rate_to_bw_code(dp->link_rate_table[i]);

		if (!analogix_dp_bandwidth_ok(dp, &dp->video_info.mode,
					      analogix_dp_get_output_bpp(dp->output_fmt),
					      dp->link_rate_table[i], dp->link_train.lane_count))
			continue;

		if (dp->link_rate_table[i] <= max_link_rate &&
		    analogix_dp_link_config_validate(bw_code, dp->link_train.lane_count)) {
			dp->link_rate_select = i;
			return bw_code;
		}
	}

	return 0;
}

static int analogix_dp_select_rx_bandwidth(struct analogix_dp_device *dp)
{
	if (dp->nr_link_rate_table)
		/*
		 * Select the smallest one among link rates which meet
		 * the bandwidth requirement for eDP 1.4 and later.
		 */
		dp->link_train.link_rate = analogix_dp_select_link_rate_from_table(dp);
	else
		/*
		 * Select the smaller one between rx DP_MAX_LINK_RATE
		 * and the max link rate supported by the platform.
		 */
		dp->link_train.link_rate = min_t(u32, dp->link_train.link_rate,
						 dp->video_info.max_link_rate);
	if (!dp->link_train.link_rate)
		return -EINVAL;

	return 0;
}

static int analogix_dp_init_link_rate_table(struct analogix_dp_device *dp)
{
	u8 link_rate_table[DP_MAX_SUPPORTED_RATES * 2];
	int i;
	int ret;

	ret = drm_dp_dpcd_read(&dp->aux, DP_SUPPORTED_LINK_RATES, link_rate_table,
			       sizeof(link_rate_table));
	if (ret < 0)
		return ret;

	for (i = 0; i < ARRAY_SIZE(link_rate_table) / 2; i++) {
		int val = link_rate_table[2 * i] | link_rate_table[2 * i + 1] << 8;

		if (val == 0)
			break;

		/* Convert to the link_rate as drm_dp_bw_code_to_link_rate() */
		dp->link_rate_table[i] = (val * 20);
	}
	dp->nr_link_rate_table = i;

	return 0;
}

static int analogix_dp_get_max_rx_bandwidth(struct analogix_dp_device *dp,
					    u8 *bandwidth)
{
	u32 max_link_rate;
	u8 data;
	int ret;

	/*
	 * For DP rev.1.1, Maximum link rate of Main Link lanes
	 * 0x06 = 1.62 Gbps, 0x0a = 2.7 Gbps
	 * For DP rev.1.2, Maximum link rate of Main Link lanes
	 * 0x06 = 1.62 Gbps, 0x0a = 2.7 Gbps, 0x14 = 5.4Gbps
	 */
	ret = drm_dp_dpcd_readb(&dp->aux, DP_MAX_LINK_RATE, &data);
	if (ret < 0)
		return ret;

	*bandwidth = data;

	/*
	 * As the Table 4-24 in eDP 1.4 spec, the Sink device can only support
	 * Main-Link rate selection via SUPPORTED_LINK_RATES when the value of
	 * DPCD MAX_LINK_RATE is 00h. If MAX_LINK_RATE and SUPPORTED_LINK_RATES
	 * are both non-zero, the Sink device can support both methods.
	 *
	 * In practice, if MAX_LINK_RATE is not 00h and SUPPORTED_LINK_RATES
	 * contains non-zero values, sometimes the sink device can only support
	 * to set link rate via LINK_BW_SET. In such case, there will be errors
	 * if set the link rate read from SUPPORTED_LINK_RATES to LINK_RATE_SET.
	 *
	 * The panel vendor may explain this is to ensure the same Sink firmware
	 * remains compatible across different versions of the eDP spec. Or the
	 * Main-Link rate selection method has not been fully verified.
	 *
	 * In order to avoid these unexpected cases, MAX_LINK_RATE/LINK_BW_SET
	 * method will be selected first if MAX_LINK_RATE is non-zero for eDP
	 * panels that support 1.4 or higher.
	 */
	if (*bandwidth == 0) {
		ret = drm_dp_dpcd_readb(&dp->aux, DP_EDP_DPCD_REV, &data);
		if (ret == 1 && data >= DP_EDP_14) {
			/*
			 * As the Table 4-23 in eDP 1.4 spec, the link rate table is required
			 * for eDP 1.4 Sink devices.
			 */
			if (!dp->nr_link_rate_table) {
				dev_info(dp->dev, "eDP version: 0x%02x supports link rate table\n",
					 data);

				ret = analogix_dp_init_link_rate_table(dp);
				if (ret) {
					dev_err(dp->dev, "failed to read link rate table: %d\n",
						ret);
					return ret;
				}
			}
			max_link_rate = dp->link_rate_table[dp->nr_link_rate_table - 1];
			*bandwidth = drm_dp_link_rate_to_bw_code(max_link_rate);
		} else {
			dev_err(dp->dev, "eDP version: 0x%02x MAX_LINK_RATE should be non-zero\n",
				data);
			return -EINVAL;
		}
	}

	return 0;
}

static int analogix_dp_get_max_rx_lane_count(struct analogix_dp_device *dp,
					      u8 *lane_count)
{
	u8 data;
	int ret;

	/*
	 * For DP rev.1.1, Maximum number of Main Link lanes
	 * 0x01 = 1 lane, 0x02 = 2 lanes, 0x04 = 4 lanes
	 */
	ret = drm_dp_dpcd_readb(&dp->aux, DP_MAX_LANE_COUNT, &data);
	if (ret < 0)
		return ret;

	*lane_count = DPCD_MAX_LANE_COUNT(data);

	return 0;
}

static int analogix_dp_init_training(struct analogix_dp_device *dp,
				     enum link_lane_count_type max_lane,
				     int max_rate)
{
	u8 dpcd;

	/*
	 * MACRO_RST must be applied after the PLL_LOCK to avoid
	 * the DP inter pair skew issue for at least 10 us
	 */
	analogix_dp_reset_macro(dp);

	/* Setup TX lane count */
	dp->link_train.lane_count = min_t(u32, dp->link_train.lane_count, max_lane);

	/* Setup TX lane rate */
	if (analogix_dp_select_rx_bandwidth(dp)) {
		dev_err(dp->dev, "Select rx bandwidth failed\n");
		return -EINVAL;
	}

	drm_dp_dpcd_readb(&dp->aux, DP_MAX_DOWNSPREAD, &dpcd);
	dp->link_train.ssc = !!(dpcd & DP_MAX_DOWNSPREAD_0_5);

	/* All DP analog module power up */
	analogix_dp_set_analog_power_down(dp, POWER_ALL, 0);

	return 0;
}

static int analogix_dp_sw_link_training(struct analogix_dp_device *dp)
{
	int retval = 0, training_finished = 0;

	dp->link_train.lt_state = START;

	/* Process here */
	while (!retval && !training_finished) {
		switch (dp->link_train.lt_state) {
		case START:
			retval = analogix_dp_link_start(dp);
			if (retval)
				dev_err(dp->dev, "LT link start failed!\n");
			break;
		case CLOCK_RECOVERY:
			retval = analogix_dp_process_clock_recovery(dp);
			if (retval)
				dev_err(dp->dev, "LT CR failed!\n");
			break;
		case EQUALIZER_TRAINING:
			retval = analogix_dp_process_equalizer_training(dp);
			if (retval)
				dev_err(dp->dev, "LT EQ failed!\n");
			break;
		case FINISHED:
			training_finished = 1;
			break;
		case FAILED:
			return -EREMOTEIO;
		}
	}

	return retval;
}

static int analogix_dp_set_link_train(struct analogix_dp_device *dp,
				      u32 count, u32 bwtype)
{
	int i, ret;

	for (i = 0; i < 5; i++) {
		ret = analogix_dp_init_training(dp, count, bwtype);
		if (ret < 0) {
			dev_err(dp->dev, "failed to init training\n");
			return ret;
		}

		ret = analogix_dp_sw_link_training(dp);
		if (!ret)
			break;
	}

	return ret;
}

static int analogix_dp_config_video(struct analogix_dp_device *dp)
{
	int timeout_loop = 0;
	int done_count = 0;

	analogix_dp_config_video_slave_mode(dp);

	analogix_dp_set_video_color_format(dp);

	if (analogix_dp_get_pll_lock_status(dp) == PLL_UNLOCKED) {
		dev_err(dp->dev, "PLL is not locked yet.\n");
		return -EINVAL;
	}

	for (;;) {
		timeout_loop++;
		if (analogix_dp_is_slave_video_stream_clock_on(dp) == 0)
			break;
		if (timeout_loop > DP_TIMEOUT_LOOP_COUNT) {
			dev_err(dp->dev, "Timeout of video streamclk ok\n");
			return -ETIMEDOUT;
		}

		udelay(2);
	}

	/* Set to use the register calculated M/N video */
	analogix_dp_set_video_cr_mn(dp, CALCULATED_M, 0, 0);

	/* For video bist, Video timing must be generated by register */
	analogix_dp_set_video_timing_mode(dp, VIDEO_TIMING_FROM_REGISTER);

	/* Disable video mute */
	analogix_dp_enable_video_mute(dp, 0);

	/* Configure video slave mode */
	analogix_dp_enable_video_master(dp, 0);

	/* Enable video input */
	analogix_dp_start_video(dp);

	timeout_loop = 0;

	for (;;) {
		timeout_loop++;
		if (analogix_dp_is_video_stream_on(dp) == 0) {
			done_count++;
			if (done_count > 10)
				break;
		} else if (done_count) {
			done_count = 0;
		}
		if (timeout_loop > DP_TIMEOUT_LOOP_COUNT) {
			dev_err(dp->dev, "Timeout of video streamclk ok\n");
			return -ETIMEDOUT;
		}

		udelay(1001);
	}

	return 0;
}

static int analogix_dp_enable_scramble(struct analogix_dp_device *dp,
					bool enable)
{
	u8 data;
	int ret;

	if (enable) {
		analogix_dp_enable_scrambling(dp);

		ret = drm_dp_dpcd_readb(&dp->aux, DP_TRAINING_PATTERN_SET,
					&data);
		if (ret != 1)
			return ret;
		ret = drm_dp_dpcd_writeb(&dp->aux, DP_TRAINING_PATTERN_SET,
					 (u8)(data & ~DP_LINK_SCRAMBLING_DISABLE));
	} else {
		analogix_dp_disable_scrambling(dp);

		ret = drm_dp_dpcd_readb(&dp->aux, DP_TRAINING_PATTERN_SET,
					&data);
		if (ret != 1)
			return ret;
		ret = drm_dp_dpcd_writeb(&dp->aux, DP_TRAINING_PATTERN_SET,
					 (u8)(data | DP_LINK_SCRAMBLING_DISABLE));
	}
	return ret < 0 ? ret : 0;
}

static void analogix_dp_init_dp(struct analogix_dp_device *dp)
{
	analogix_dp_reset(dp);

	analogix_dp_swreset(dp);

	analogix_dp_init_analog_param(dp);
	analogix_dp_init_interrupt(dp);

	/* SW defined function Normal operation */
	analogix_dp_enable_sw_function(dp);

	analogix_dp_config_interrupt(dp);
	analogix_dp_init_analog_func(dp);

	analogix_dp_init_hpd(dp);
	analogix_dp_init_aux(dp);
}

static int analogix_dp_connector_init(struct rockchip_connector *conn, struct display_state *state)
{
	struct connector_state *conn_state = &state->conn_state;
	struct analogix_dp_device *dp = dev_get_priv(conn->dev);
	int submode = PHY_SUBMODE_EDP;

	if (!conn->panel)
		dp->dp_mode = true;

	if (dev_read_bool(conn->dev, "dp-mode"))
		dp->dp_mode = true;
	else if (dev_read_bool(conn->dev, "edp-mode"))
		dp->dp_mode = false;

	conn_state->output_if |= dp->id ? VOP_OUTPUT_IF_eDP1 : VOP_OUTPUT_IF_eDP0;
	conn_state->output_mode = ROCKCHIP_OUT_MODE_AAAA;
	conn_state->color_encoding = DRM_COLOR_YCBCR_BT709;
	conn_state->color_range = DRM_COLOR_YCBCR_FULL_RANGE;

	reset_assert_bulk(&dp->resets);
	udelay(1);
	reset_deassert_bulk(&dp->resets);

	rockchip_baseparameter_disp_info_init((uintptr_t)conn_state, conn_state->type, dp->id);
	if (dp->plat_data.support_dp_mode && dp->dp_mode)
		submode = PHY_SUBMODE_DP;
	generic_phy_set_mode(&dp->phy, PHY_MODE_DP, submode);
	generic_phy_power_on(&dp->phy);
	analogix_dp_init_dp(dp);

	return 0;
}

static int analogix_dp_link_power_up(struct analogix_dp_device *dp)
{
	u8 value;
	int ret;

	if (dp->dpcd[DP_DPCD_REV] < 0x11)
		return 0;

	ret = drm_dp_dpcd_readb(&dp->aux, DP_SET_POWER, &value);
	if (ret < 0)
		return ret;

	value &= ~DP_SET_POWER_MASK;
	value |= DP_SET_POWER_D0;

	ret = drm_dp_dpcd_writeb(&dp->aux, DP_SET_POWER, value);
	if (ret < 0)
		return ret;

	mdelay(1);

	return 0;
}

static int analogix_dp_link_power_down(struct analogix_dp_device *dp)
{
	u8 value;
	int ret;

	if (dp->dpcd[DP_DPCD_REV] < 0x11)
		return 0;

	ret = drm_dp_dpcd_readb(&dp->aux, DP_SET_POWER, &value);
	if (ret < 0)
		return ret;

	value &= ~DP_SET_POWER_MASK;
	value |= DP_SET_POWER_D3;

	ret = drm_dp_dpcd_writeb(&dp->aux, DP_SET_POWER, value);
	if (ret < 0)
		return ret;

	return 0;
}

static u32 analogix_dp_get_output_format(struct analogix_dp_device *dp, u32 bus_format)
{
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(possible_output_fmts); i++) {
		const struct analogix_dp_output_format *fmt = &possible_output_fmts[i];

		if (fmt->bus_format == bus_format)
			break;
	}

	if (i == ARRAY_SIZE(possible_output_fmts))
		return 1;

	return i;
}

static u32 analogix_dp_get_output_format_by_edid(struct analogix_dp_device *dp,
						 struct hdmi_edid_data *edid_data)
{
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE(possible_output_fmts); i++) {
		const struct analogix_dp_output_format *fmt = &possible_output_fmts[i];

		if (fmt->bpc > edid_data->display_info.bpc || fmt->bpc > dp->plat_data.max_bpc)
			continue;

		if (!(edid_data->display_info.color_formats & fmt->color_format))
			continue;

		if (!analogix_dp_bandwidth_ok(dp, edid_data->preferred_mode,
					      analogix_dp_get_output_bpp(fmt),
					      drm_dp_bw_code_to_link_rate(dp->link_train.link_rate),
					      dp->link_train.lane_count))
			continue;

		break;
	}

	if (i == ARRAY_SIZE(possible_output_fmts))
		return 1;

	return i;
}

static int analogix_dp_connector_enable(struct rockchip_connector *conn,
					struct display_state *state)
{
	struct connector_state *conn_state = &state->conn_state;
	struct crtc_state *crtc_state = &state->crtc_state;
	const struct rockchip_dp_chip_data *pdata =
		(const struct rockchip_dp_chip_data *)dev_get_driver_data(conn->dev);
	struct analogix_dp_device *dp = dev_get_priv(conn->dev);
	struct video_info *video = &dp->video_info;
	struct drm_display_mode mode;
	u32 fmt_id;
	u32 val;
	int ret;

	drm_mode_copy(&video->mode, &conn_state->mode);

	if (pdata->lcdsel_grf_reg) {
		if (crtc_state->crtc_id)
			val = pdata->lcdsel_lit;
		else
			val = pdata->lcdsel_big;

		regmap_write(dp->grf, pdata->lcdsel_grf_reg, val);
	}

	if (pdata->chip_type == RK3588_EDP)
		regmap_write(dp->grf, dp->id ? RK3588_GRF_VO1_CON1 : RK3588_GRF_VO1_CON0,
			     EDP_MODE << 16 | FIELD_PREP(EDP_MODE, 1));

	if (!dp->output_fmt) {
		fmt_id = analogix_dp_get_output_format(dp, conn_state->bus_format);
		dp->output_fmt = &possible_output_fmts[fmt_id];
	}

	switch (dp->output_fmt->bpc) {
	case 12:
		video->color_depth = COLOR_12;
		break;
	case 10:
		video->color_depth = COLOR_10;
		break;
	case 6:
		video->color_depth = COLOR_6;
		break;
	case 8:
	default:
		video->color_depth = COLOR_8;
		break;
	}
	if (dp->output_fmt->color_format == DRM_COLOR_FORMAT_YCRCB444) {
		video->color_space = COLOR_YCBCR444;
		video->ycbcr_coeff = COLOR_YCBCR709;
	} else if (dp->output_fmt->color_format == DRM_COLOR_FORMAT_YCRCB422) {
		video->color_space = COLOR_YCBCR422;
		video->ycbcr_coeff = COLOR_YCBCR709;
	} else {
		video->color_space = COLOR_RGB;
		video->ycbcr_coeff = COLOR_YCBCR601;
	}

	ret = drm_dp_dpcd_read(&dp->aux, DP_DPCD_REV, dp->dpcd, DP_RECEIVER_CAP_SIZE);
	if (ret < 0) {
		dev_err(dp->dev, "failed to read dpcd caps: %d\n", ret);
		return ret;
	}

	ret = analogix_dp_link_power_up(dp);
	if (ret) {
		dev_err(dp->dev, "failed to power up link: %d\n", ret);
		return ret;
	}

	ret = analogix_dp_set_link_train(dp, dp->video_info.max_lane_count,
					 dp->video_info.max_link_rate);
	if (ret) {
		dev_err(dp->dev, "unable to do link train\n");
		return ret;
	}

	ret = analogix_dp_enable_scramble(dp, 1);
	if (ret < 0) {
		dev_err(dp->dev, "can not enable scramble\n");
		return ret;
	}

	analogix_dp_init_video(dp);

	drm_mode_copy(&mode, &conn_state->mode);
	if (conn->dual_channel_mode)
		drm_mode_convert_to_origin_mode(&mode);
	analogix_dp_set_video_format(dp, &mode);

	if (dp->video_bist_enable)
		analogix_dp_video_bist_enable(dp);

	ret = analogix_dp_config_video(dp);
	if (ret) {
		dev_err(dp->dev, "unable to config video\n");
		return ret;
	}

	return 0;
}

static int analogix_dp_connector_disable(struct rockchip_connector *conn,
					 struct display_state *state)
{
	const struct rockchip_dp_chip_data *pdata =
		(const struct rockchip_dp_chip_data *)dev_get_driver_data(conn->dev);
	struct analogix_dp_device *dp = dev_get_priv(conn->dev);

	if (!analogix_dp_get_plug_in_status(dp))
		analogix_dp_link_power_down(dp);

	if (pdata->chip_type == RK3588_EDP)
		regmap_write(dp->grf, dp->id ? RK3588_GRF_VO1_CON1 : RK3588_GRF_VO1_CON0,
			     EDP_MODE << 16 | FIELD_PREP(EDP_MODE, 0));

	return 0;
}

static int analogix_dp_connector_detect(struct rockchip_connector *conn,
					struct display_state *state)
{
	struct analogix_dp_device *dp = dev_get_priv(conn->dev);
	int ret;

	if (analogix_dp_detect(dp)) {
		/* Initialize by reading RX's DPCD */
		ret = analogix_dp_get_max_rx_bandwidth(dp, &dp->link_train.link_rate);
		if (ret) {
			dev_err(dp->dev, "failed to read max link rate\n");
			return 0;
		}

		ret = analogix_dp_get_max_rx_lane_count(dp, &dp->link_train.lane_count);
		if (ret) {
			dev_err(dp->dev, "failed to read max lane count\n");
			return 0;
		}

		return 1;
	} else {
		return 0;
	}
}

static int analogix_dp_connector_mode_valid(struct rockchip_connector *conn,
					    struct display_state *state)
{
	struct analogix_dp_device *dp = dev_get_priv(conn->dev);
	struct connector_state *conn_state = &state->conn_state;
	struct drm_display_mode *mode = &conn_state->mode;
	const struct rockchip_dp_chip_data *pdata =
		(const struct rockchip_dp_chip_data *)dev_get_driver_data(conn->dev);
	struct videomode vm;

	drm_display_mode_to_videomode(mode, &vm);

	if (!vm.hfront_porch || !vm.hback_porch || !vm.vfront_porch || !vm.vback_porch) {
		dev_err(dp->dev, "front porch or back porch can not be 0\n");
		return MODE_BAD;
	}

	if (mode->clock > pdata->max_dclk_khz) {
		dev_err(dp->dev, "clock[%dkHz] exceeds limit[%dkHz]\n",
			mode->clock, pdata->max_dclk_khz);
		return MODE_CLOCK_HIGH;
	}

	if (mode->vtotal > 4095) {
		dev_err(dp->dev, "vtotal[%d] exceeds limit[4095]\n", mode->vtotal);
		return MODE_BAD_VVALUE;
	}

	return MODE_OK;
}

static int analogix_dp_mode_valid(struct analogix_dp_device *dp, struct hdmi_edid_data *edid_data)
{
	struct drm_display_info *di = &edid_data->display_info;
	u32 max_link_rate, max_lane_count;
	u32 min_bpp;
	int i;

	if (di->color_formats & DRM_COLOR_FORMAT_YCRCB422)
		min_bpp = 16;
	else if (di->color_formats & DRM_COLOR_FORMAT_RGB444)
		min_bpp = 18;
	else
		min_bpp = 24;

	max_link_rate = min_t(u32, dp->video_info.max_link_rate, dp->link_train.link_rate);
	max_lane_count = min_t(u32, dp->video_info.max_lane_count, dp->link_train.lane_count);
	for (i = 0; i < edid_data->modes; i++) {
		if (!analogix_dp_bandwidth_ok(dp, &edid_data->mode_buf[i], min_bpp,
					      drm_dp_bw_code_to_link_rate(max_link_rate),
					      max_lane_count))
			edid_data->mode_buf[i].invalid = true;
	}

	return 0;
}

static int analogix_dp_connector_get_timing(struct rockchip_connector *conn,
					    struct display_state *state)
{
	struct connector_state *conn_state = &state->conn_state;
	const struct rockchip_dp_chip_data *pdata =
		(const struct rockchip_dp_chip_data *)dev_get_driver_data(conn->dev);
	struct analogix_dp_device *dp = dev_get_priv(conn->dev);
	struct drm_display_mode *mode = &conn_state->mode;
	struct hdmi_edid_data edid_data;
	struct drm_display_mode *mode_buf;
	struct vop_rect rect;
	u32 yuv_fmts_mask = DRM_COLOR_FORMAT_YCRCB444 | DRM_COLOR_FORMAT_YCRCB422;
	u32 fmt_id;
	int ret = 0, i;

	mode_buf = malloc(MODE_LEN * sizeof(struct drm_display_mode));
	if (!mode_buf)
		return -ENOMEM;

	memset(mode_buf, 0, MODE_LEN * sizeof(struct drm_display_mode));
	memset(&edid_data, 0, sizeof(struct hdmi_edid_data));
	edid_data.mode_buf = mode_buf;

	conn_state->edid = drm_do_get_edid(&dp->aux.ddc);
	if (conn_state->edid)
		ret = drm_add_edid_modes(&edid_data, conn_state->edid);

	if (ret <= 0) {
		printf("failed to get edid\n");
		goto err;
	}

	if (!pdata->format_yuv) {
		if (edid_data.display_info.color_formats & yuv_fmts_mask) {
			printf("Swapping display color format from YUV to RGB\n");
			edid_data.display_info.color_formats &= ~yuv_fmts_mask;
			edid_data.display_info.color_formats |= DRM_COLOR_FORMAT_RGB444;
		}
	}

	if (state->conn_state.secondary) {
		rect.width = state->crtc_state.max_output.width / 2;
		rect.height = state->crtc_state.max_output.height / 2;
	} else {
		rect.width = state->crtc_state.max_output.width;
		rect.height = state->crtc_state.max_output.height;
	}

	drm_mode_max_resolution_filter(&edid_data, &rect);
	analogix_dp_mode_valid(dp, &edid_data);

	if (!drm_mode_prune_invalid(&edid_data)) {
		printf("can't find valid dp mode\n");
		ret = -EINVAL;
		goto err;
	}

	for (i = 0; i < edid_data.modes; i++)
		edid_data.mode_buf[i].vrefresh = drm_mode_vrefresh(&edid_data.mode_buf[i]);

	drm_mode_sort(&edid_data);
	memcpy(mode, edid_data.preferred_mode, sizeof(struct drm_display_mode));

	fmt_id = analogix_dp_get_output_format_by_edid(dp, &edid_data);
	dp->output_fmt = &possible_output_fmts[fmt_id];

	switch (dp->output_fmt->color_format) {
	case DRM_COLOR_FORMAT_YCRCB422:
		conn_state->output_mode = ROCKCHIP_OUT_MODE_YUV422;
		break;
	case DRM_COLOR_FORMAT_RGB444:
	case DRM_COLOR_FORMAT_YCRCB444:
	default:
		conn_state->output_mode = ROCKCHIP_OUT_MODE_AAAA;
		break;
	}

	conn_state->bus_format = dp->output_fmt->bus_format;
	conn_state->bpc = dp->output_fmt->bpc;
	conn_state->color_encoding = DRM_COLOR_YCBCR_BT709;
	if (dp->output_fmt->color_format == DRM_COLOR_FORMAT_RGB444)
		conn_state->color_range = DRM_COLOR_YCBCR_FULL_RANGE;
	else
		conn_state->color_range = DRM_COLOR_YCBCR_LIMITED_RANGE;

err:
	free(mode_buf);

	return 0;
}

static const struct rockchip_connector_funcs analogix_dp_connector_funcs = {
	.init = analogix_dp_connector_init,
	.enable = analogix_dp_connector_enable,
	.disable = analogix_dp_connector_disable,
	.detect = analogix_dp_connector_detect,
	.mode_valid = analogix_dp_connector_mode_valid,
	.get_timing = analogix_dp_connector_get_timing,
};

static u32 analogix_dp_parse_link_frequencies(struct analogix_dp_device *dp)
{
	struct udevice *dev = dp->dev;
	const struct device_node *endpoint;
	u64 frequency = 0;

	endpoint = rockchip_of_graph_get_endpoint_by_regs(dev_ofnode(dev), 1, 0);
	if (!endpoint)
		return 0;

	if (of_read_u64(endpoint, "link-frequencies", &frequency) < 0)
		return 0;

	if (!frequency)
		return 0;

	do_div(frequency, 10 * 1000);	/* symbol rate kbytes */

	switch (frequency) {
	case 162000:
	case 270000:
	case 540000:
		break;
	default:
		dev_err(dev, "invalid link frequency value: %llu\n", frequency);
		return 0;
	}

	return frequency;
}

static int analogix_dp_parse_dt(struct analogix_dp_device *dp)
{
	struct udevice *dev = dp->dev;
	int len;
	u32 num_lanes;
	u32 max_link_rate;
	int ret;

	dp->force_hpd = dev_read_bool(dev, "force-hpd");
	dp->video_bist_enable = dev_read_bool(dev, "analogix,video-bist-enable");
	dp->video_info.force_stream_valid =
		dev_read_bool(dev, "analogix,force-stream-valid");

	max_link_rate = analogix_dp_parse_link_frequencies(dp);
	if (max_link_rate && max_link_rate < drm_dp_bw_code_to_link_rate(dp->video_info.max_link_rate))
		dp->video_info.max_link_rate = drm_dp_link_rate_to_bw_code(max_link_rate);

	if (dev_read_prop(dev, "data-lanes", &len)) {
		num_lanes = len / sizeof(u32);
		if (num_lanes < 1 || num_lanes > 4 || num_lanes == 3) {
			dev_err(dev, "bad number of data lanes\n");
			return -EINVAL;
		}

		ret = dev_read_u32_array(dev, "data-lanes", dp->lane_map,
					 num_lanes);
		if (ret)
			return ret;

		dp->video_info.max_lane_count = num_lanes;
	} else {
		dp->lane_map[0] = 0;
		dp->lane_map[1] = 1;
		dp->lane_map[2] = 2;
		dp->lane_map[3] = 3;
	}

	return 0;
}

static int analogix_dp_ddc_init(struct analogix_dp_device *dp)
{
	dp->aux.name = "analogix-dp";
	dp->aux.dev = dp->dev;
	dp->aux.transfer = analogix_dp_aux_transfer;
	dp->aux.ddc.ddc_xfer = drm_dp_i2c_xfer;

	return 0;
}

static int analogix_dp_probe(struct udevice *dev)
{
	struct analogix_dp_device *dp = dev_get_priv(dev);
	const struct rockchip_dp_chip_data *dp_data;
	const struct rockchip_dp_chip_data *pdata = NULL;
	struct udevice *syscon;
	int i;
	int ret;

	dp->reg_base = dev_read_addr_ptr(dev);
	dp_data = (const struct rockchip_dp_chip_data *)dev_get_driver_data(dev);

	i = 0;
	while (dp_data[i].reg) {
		if (dp_data[i].reg == (uintptr_t)dp->reg_base) {
			pdata = &dp_data[i];
			break;
		}

		i++;
	}

	if (!pdata) {
		dev_err(dev, "no chip-data for %s\n", dev->name);
		return -EINVAL;
	}
	dp->id = i;

	ret = uclass_get_device_by_phandle(UCLASS_SYSCON, dev, "rockchip,grf",
					   &syscon);
	if (!ret) {
		dp->grf = syscon_get_regmap(syscon);
		if (!dp->grf)
			return -ENODEV;
	}

#if defined(CONFIG_MOS_SUPPORT) && !defined(CONFIG_SPL_BUILD)
	ret = power_domain_get(dev, &dp->pwrdom);
	if (ret) {
		dev_err(dev, "failed to get pwrdom: %d\n", ret);
		return ret;
	}
	ret = power_domain_on(&dp->pwrdom);
	if (ret) {
		dev_err(dev, "failed to power on pd: %d\n", ret);
		return ret;
	}
	ret = clk_get_bulk(dev, &dp->clks);
	if (ret) {
		dev_err(dev, "failed to get clk: %d\n", ret);
		return ret;
	}
	ret = clk_enable_bulk(&dp->clks);
	if (ret) {
		dev_err(dev, "failed to enable clk: %d\n", ret);
		return ret;
	}
#endif

	ret = reset_get_bulk(dev, &dp->resets);
	if (ret) {
		dev_err(dev, "failed to get reset control: %d\n", ret);
		return ret;
	}

	ret = gpio_request_by_name(dev, "hpd-gpios", 0, &dp->hpd_gpio,
				   GPIOD_IS_IN);
	if (ret && ret != -ENOENT) {
		dev_err(dev, "failed to get hpd GPIO: %d\n", ret);
		return ret;
	}

	generic_phy_get_by_name(dev, "dp", &dp->phy);

	dp->plat_data.dev_type = ROCKCHIP_DP;
	dp->plat_data.subdev_type = pdata->chip_type;
	dp->plat_data.ssc = pdata->ssc;
	dp->plat_data.support_dp_mode = pdata->support_dp_mode;
	dp->plat_data.max_bpc = pdata->max_bpc ? pdata->max_bpc : 8;

	dp->video_info.max_link_rate = pdata->max_link_rate;
	dp->video_info.max_lane_count = pdata->max_lane_count;

	dp->dev = dev;

	ret = analogix_dp_parse_dt(dp);
	if (ret) {
		dev_err(dev, "failed to parse DT: %d\n", ret);
		return ret;
	}

	analogix_dp_ddc_init(dp);

	rockchip_connector_bind(&dp->connector, dev, dp->id, &analogix_dp_connector_funcs,
				NULL, DRM_MODE_CONNECTOR_eDP);

	return 0;
}

static const struct rockchip_dp_chip_data rk3288_edp_platform_data[] = {
	{
		.lcdsel_grf_reg = 0x025c,
		.lcdsel_big = 0 | BIT(21),
		.lcdsel_lit = BIT(5) | BIT(21),
		.chip_type = RK3288_DP,
		.reg = 0xff970000,

		.max_link_rate = DP_LINK_BW_2_7,
		.max_lane_count = 4,
		.max_dclk_khz = 350000,
	},
	{ /* sentinel */ }
};

static const struct rockchip_dp_chip_data rk3368_edp_platform_data[] = {
	{
		.chip_type = RK3368_EDP,
		.reg = 0xff970000,

		.max_link_rate = DP_LINK_BW_2_7,
		.max_lane_count = 4,
		.max_dclk_khz = 350000,
	},
	{ /* sentinel */ }
};

static const struct rockchip_dp_chip_data rk3399_edp_platform_data[] = {
	{
		.lcdsel_grf_reg = 0x6250,
		.lcdsel_big = 0 | BIT(21),
		.lcdsel_lit = BIT(5) | BIT(21),
		.chip_type = RK3399_EDP,
		.reg = 0xff970000,
		.ssc = true,

		.max_link_rate = DP_LINK_BW_5_4,
		.max_lane_count = 4,
		.max_dclk_khz = 350000,
	},
	{ /* sentinel */ }
};

static const struct rockchip_dp_chip_data rk3568_edp_platform_data[] = {
	{
		.chip_type = RK3568_EDP,
		.reg = 0xfe0c0000,
		.ssc = true,

		.max_link_rate = DP_LINK_BW_2_7,
		.max_lane_count = 4,
		.max_dclk_khz = 350000,
	},
	{ /* sentinel */ }
};

static const struct rockchip_dp_chip_data rk3572_edp_platform_data[] = {
	{
		.chip_type = RK3576_EDP,
		.reg = 0x276b0000,
		.ssc = true,

		.max_link_rate = DP_LINK_BW_5_4,
		.max_lane_count = 4,
		.format_yuv = true,
		.support_dp_mode = true,
		.max_bpc = 10,
		.max_dclk_khz = 600000,
	},
	{ /* sentinel */ }
};

static const struct rockchip_dp_chip_data rk3576_edp_platform_data[] = {
	{
		.chip_type = RK3576_EDP,
		.reg = 0x27dc0000,
		.ssc = true,

		.max_link_rate = DP_LINK_BW_5_4,
		.max_lane_count = 4,
		.format_yuv = true,
		.support_dp_mode = true,
		.max_bpc = 10,
		.max_dclk_khz = 600000,
	},
	{ /* sentinel */ }
};

static const struct rockchip_dp_chip_data rk3588_edp_platform_data[] = {
	{
		.chip_type = RK3588_EDP,
		.reg = 0xfdec0000,
		.ssc = true,

		.max_link_rate = DP_LINK_BW_5_4,
		.max_lane_count = 4,
		.format_yuv = true,
		.support_dp_mode = true,
		.max_bpc = 10,
		.max_dclk_khz = 600000,
	},
	{
		.chip_type = RK3588_EDP,
		.reg = 0xfded0000,
		.ssc = true,

		.max_link_rate = DP_LINK_BW_5_4,
		.max_lane_count = 4,
		.format_yuv = true,
		.support_dp_mode = true,
		.max_bpc = 10,
		.max_dclk_khz = 600000,
	},
	{ /* sentinel */ }
};

static const struct udevice_id analogix_dp_ids[] = {
	{
		.compatible = "rockchip,rk3288-dp",
		.data = (ulong)&rk3288_edp_platform_data,
	}, {
		.compatible = "rockchip,rk3368-edp",
		.data = (ulong)&rk3368_edp_platform_data,
	}, {
		.compatible = "rockchip,rk3399-edp",
		.data = (ulong)&rk3399_edp_platform_data,
	}, {
		.compatible = "rockchip,rk3568-edp",
		.data = (ulong)&rk3568_edp_platform_data,
	}, {
		.compatible = "rockchip,rk3572-edp",
		.data = (ulong)&rk3572_edp_platform_data,
	}, {
		.compatible = "rockchip,rk3576-edp",
		.data = (ulong)&rk3576_edp_platform_data,
	}, {
		.compatible = "rockchip,rk3588-edp",
		.data = (ulong)&rk3588_edp_platform_data,
	},
	{}
};

U_BOOT_DRIVER(analogix_dp) = {
	.name = "analogix_dp",
	.id = UCLASS_DISPLAY,
	.of_match = analogix_dp_ids,
	.probe = analogix_dp_probe,
	.priv_auto = sizeof(struct analogix_dp_device),
};
