/*
 * (C) Copyright 2008-2017 Fuzhou Rockchip Electronics Co., Ltd
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifndef __ROCKCHIP_MIPI_DSI_H__
#define __ROCKCHIP_MIPI_DSI_H__

#define MSEC_PER_SEC	1000L
#define USEC_PER_MSEC	1000L
#define NSEC_PER_USEC	1000L
#define NSEC_PER_MSEC	1000000L
#define USEC_PER_SEC	1000000L
#define NSEC_PER_SEC	1000000000L
#define FSEC_PER_SEC	1000000000000000LL

#define BIT(x)		(1 << (x))
#define GENMASK(h, l)	(((~0UL) << (l)) & (~0UL >> (BITS_PER_LONG - 1 - (h))))
#define UPDATE(v, h, l)	(((v) << (l)) & GENMASK((h), (l)))
#define HIWORD_UPDATE(v, h, l)	(((v) << (l)) | (GENMASK(h, l) << 16))

/* MIPI DSI Processor-to-Peripheral transaction types */
enum {
	MIPI_DSI_V_SYNC_START				= 0x01,
	MIPI_DSI_V_SYNC_END				= 0x11,
	MIPI_DSI_H_SYNC_START				= 0x21,
	MIPI_DSI_H_SYNC_END				= 0x31,

	MIPI_DSI_COLOR_MODE_OFF				= 0x02,
	MIPI_DSI_COLOR_MODE_ON				= 0x12,
	MIPI_DSI_SHUTDOWN_PERIPHERAL			= 0x22,
	MIPI_DSI_TURN_ON_PERIPHERAL			= 0x32,

	MIPI_DSI_GENERIC_SHORT_WRITE_0_PARAM		= 0x03,
	MIPI_DSI_GENERIC_SHORT_WRITE_1_PARAM		= 0x13,
	MIPI_DSI_GENERIC_SHORT_WRITE_2_PARAM		= 0x23,

	MIPI_DSI_GENERIC_READ_REQUEST_0_PARAM		= 0x04,
	MIPI_DSI_GENERIC_READ_REQUEST_1_PARAM		= 0x14,
	MIPI_DSI_GENERIC_READ_REQUEST_2_PARAM		= 0x24,

	MIPI_DSI_DCS_SHORT_WRITE			= 0x05,
	MIPI_DSI_DCS_SHORT_WRITE_PARAM			= 0x15,

	MIPI_DSI_DCS_READ				= 0x06,

	MIPI_DSI_SET_MAXIMUM_RETURN_PACKET_SIZE		= 0x37,

	MIPI_DSI_END_OF_TRANSMISSION			= 0x08,

	MIPI_DSI_NULL_PACKET				= 0x09,
	MIPI_DSI_BLANKING_PACKET			= 0x19,
	MIPI_DSI_GENERIC_LONG_WRITE			= 0x29,
	MIPI_DSI_DCS_LONG_WRITE				= 0x39,

	MIPI_DSI_LOOSELY_PACKED_PIXEL_STREAM_YCBCR20	= 0x0c,
	MIPI_DSI_PACKED_PIXEL_STREAM_YCBCR24		= 0x1c,
	MIPI_DSI_PACKED_PIXEL_STREAM_YCBCR16		= 0x2c,

	MIPI_DSI_PACKED_PIXEL_STREAM_30			= 0x0d,
	MIPI_DSI_PACKED_PIXEL_STREAM_36			= 0x1d,
	MIPI_DSI_PACKED_PIXEL_STREAM_YCBCR12		= 0x3d,

	MIPI_DSI_PACKED_PIXEL_STREAM_16			= 0x0e,
	MIPI_DSI_PACKED_PIXEL_STREAM_18			= 0x1e,
	MIPI_DSI_PIXEL_STREAM_3BYTE_18			= 0x2e,
	MIPI_DSI_PACKED_PIXEL_STREAM_24			= 0x3e,
};

/* request ACK from peripheral */
#define MIPI_DSI_MSG_REQ_ACK	BIT(0)
/* use Low Power Mode to transmit message */
#define MIPI_DSI_MSG_USE_LPM	BIT(1)

/**
 * struct mipi_dsi_msg - read/write DSI buffer
 * @channel: virtual channel id
 * @type: payload data type
 * @flags: flags controlling this message transmission
 * @tx_len: length of @tx_buf
 * @tx_buf: data to be written
 * @rx_len: length of @rx_buf
 * @rx_buf: data to be read, or NULL
 */
struct mipi_dsi_msg {
	u8 channel;
	u8 type;
	u16 flags;

	size_t tx_len;
	const void *tx_buf;

	size_t rx_len;
	void *rx_buf;
};

/**
 * struct mipi_dsi_packet - represents a MIPI DSI packet in protocol format
 * @size: size (in bytes) of the packet
 * @header: the four bytes that make up the header (Data ID, Word Count or
 *     Packet Data, and ECC)
 * @payload_length: number of bytes in the payload
 * @payload: a pointer to a buffer containing the payload, if any
 */
struct mipi_dsi_packet {
	size_t size;
	u8 header[4];
	size_t payload_length;
	const u8 *payload;
};


/* DSI mode flags */

/* video mode */
#define MIPI_DSI_MODE_VIDEO		BIT(0)
/* video burst mode */
#define MIPI_DSI_MODE_VIDEO_BURST	BIT(1)
/* video pulse mode */
#define MIPI_DSI_MODE_VIDEO_SYNC_PULSE	BIT(2)
/* enable auto vertical count mode */
#define MIPI_DSI_MODE_VIDEO_AUTO_VERT	BIT(3)
/* enable hsync-end packets in vsync-pulse and v-porch area */
#define MIPI_DSI_MODE_VIDEO_HSE		BIT(4)
/* disable hfront-porch area */
#define MIPI_DSI_MODE_VIDEO_HFP		BIT(5)
/* disable hback-porch area */
#define MIPI_DSI_MODE_VIDEO_HBP		BIT(6)
/* disable hsync-active area */
#define MIPI_DSI_MODE_VIDEO_HSA		BIT(7)
/* flush display FIFO on vsync pulse */
#define MIPI_DSI_MODE_VSYNC_FLUSH	BIT(8)
/* disable EoT packets in HS mode */
#define MIPI_DSI_MODE_EOT_PACKET	BIT(9)
/* device supports non-continuous clock behavior (DSI spec 5.6.1) */
#define MIPI_DSI_CLOCK_NON_CONTINUOUS	BIT(10)
/* transmit data in low power */
#define MIPI_DSI_MODE_LPM		BIT(11)

enum mipi_dsi_pixel_format {
	MIPI_DSI_FMT_RGB888,
	MIPI_DSI_FMT_RGB666,
	MIPI_DSI_FMT_RGB666_PACKED,
	MIPI_DSI_FMT_RGB565,
};

/**
 * mipi_dsi_pixel_format_to_bpp - obtain the number of bits per pixel for any
 *                                given pixel format defined by the MIPI DSI
 *                                specification
 * @fmt: MIPI DSI pixel format
 *
 * Returns: The number of bits per pixel of the given pixel format.
 */
static inline int mipi_dsi_pixel_format_to_bpp(enum mipi_dsi_pixel_format fmt)
{
	switch (fmt) {
	case MIPI_DSI_FMT_RGB888:
	case MIPI_DSI_FMT_RGB666:
		return 24;

	case MIPI_DSI_FMT_RGB666_PACKED:
		return 18;

	case MIPI_DSI_FMT_RGB565:
		return 16;
	}

	return -EINVAL;
}

/**
 * enum mipi_dsi_dcs_tear_mode - Tearing Effect Output Line mode
 * @MIPI_DSI_DCS_TEAR_MODE_VBLANK: the TE output line consists of V-Blanking
 *    information only
 * @MIPI_DSI_DCS_TEAR_MODE_VHBLANK : the TE output line consists of both
 *    V-Blanking and H-Blanking information
 */
enum mipi_dsi_dcs_tear_mode {
	MIPI_DSI_DCS_TEAR_MODE_VBLANK,
	MIPI_DSI_DCS_TEAR_MODE_VHBLANK,
};

#define MIPI_DSI_DCS_POWER_MODE_DISPLAY (1 << 2)
#define MIPI_DSI_DCS_POWER_MODE_NORMAL  (1 << 3)
#define MIPI_DSI_DCS_POWER_MODE_SLEEP   (1 << 4)
#define MIPI_DSI_DCS_POWER_MODE_PARTIAL (1 << 5)
#define MIPI_DSI_DCS_POWER_MODE_IDLE    (1 << 6)

struct mipi_dphy_timing {
	unsigned int clkmiss;
	unsigned int clkpost;
	unsigned int clkpre;
	unsigned int clkprepare;
	unsigned int clksettle;
	unsigned int clktermen;
	unsigned int clktrail;
	unsigned int clkzero;
	unsigned int dtermen;
	unsigned int eot;
	unsigned int hsexit;
	unsigned int hsprepare;
	unsigned int hszero;
	unsigned int hssettle;
	unsigned int hsskip;
	unsigned int hstrail;
	unsigned int init;
	unsigned int lpx;
	unsigned int taget;
	unsigned int tago;
	unsigned int tasure;
	unsigned int wakeup;
};

void mipi_dphy_timing_get_default(struct mipi_dphy_timing *timing,
				  unsigned long period);
ssize_t mipi_dsi_dcs_write(struct display_state *state,
			   const void *payload, size_t size);
ssize_t mipi_dsi_generic_write(struct display_state *state,
			       const void *payload, size_t size);
int mipi_dsi_create_packet(struct mipi_dsi_packet *packet,
			   const struct mipi_dsi_msg *msg);
ssize_t mipi_dsi_dcs_read(struct display_state *state, u8 cmd, void *data,
			  size_t len);
ssize_t mipi_dsi_generic_read(struct display_state *state, const void *params,
			      size_t num_params, void *data, size_t size);
ssize_t mipi_dsi_shutdown_peripheral(struct display_state *state);
ssize_t mipi_dsi_turn_on_peripheral(struct display_state *state);

#endif /* __ROCKCHIP_MIPI_DSI__ */
