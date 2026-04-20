/*
 * (C) Copyright 2020 Rockchip Electronics Co., Ltd
 *
 * SPDX-License-Identifier:     GPL-2.0+
 * Author: Wenping Zhang <wenping.zhang@rock-chips.com>
 */
#include <common.h>
#include <dm.h>
#include <stdio.h>
#include <errno.h>
#include <mapmem.h>
#include <stdlib.h>
#include <asm/arch-rockchip/common.h>
#include <asm/arch-rockchip/vendor.h>
#include <asm/cache.h>
#include <cpu_func.h>
#include <dm/device_compat.h>
#include <dm/device-internal.h>
#include <dm/of_access.h>
#include <dm/uclass.h>
#include <dm/uclass-id.h>
#include <env.h>
#include <linux/compat.h>
#include <rk_ebook.h>
#include <backlight.h>
#include <part.h>
#include <power/regulator.h>
#include <thermal.h>
#include "rk_ebc.h"
#include "epdlut/epd_lut.h"

#if !CONFIG_IS_ENABLED(DM_THERMAL)
inline int thermal_get_temp(struct udevice *dev, int *temp)
{
	return 0;
}
#endif

#define PART_WAVEFORM		"waveform"
#define EBOOK_LOGO_PART_MAGIC	"RKEL"
#define EBOOK_LOGO_IMAGE_MAGIC	"GR04"
/*
 * grayscale logo partition format:
 * block 0:
 * struct logo_part_header part_header;
 * struct grayscale_header logo1_header;
 * struct grayscale_header logo2_header;
 * struct grayscale_header logo3_header;
 * struct grayscale_header logo4_header;
 * ....
 *
 * block (align(hdr_size, blk_size) / blk_size):
 * logo1_image
 *
 * .....
 * block m:
 * logo2_image
 *
 * ........
 * block n:
 * logo3_image
 *
 * ........
 * block i:
 * logoi_image
 */

//logo partition Header, 64byte
struct logo_part_header {
	char magic[4]; /* must be "RKEL" */
	u32  total_size;
	u32  screen_width;
	u32  screen_height;
	u32  logo_count;
	u8   version[4];
	u32  rsv[10];
} __packed;

// logo image header,32 byte
struct grayscale_header {
	char magic[4]; /* must be "GR04" */
	u16 x;
	u16 y;
	u16 w;
	u16 h;
	u32 logo_type;
	u32 data_offset; /* image offset in byte */
	u32 data_size; /* image size in byte */
	u32 rsv[2];
} __packed;

/*
 * The start address of logo image in logo.img must be aligned in 512 bytes.
 */
struct logo_info {
	struct logo_part_header *part_hdr;
	struct grayscale_header *img_hdr;
};

struct rockchip_ebook_display_priv {
	struct udevice *dev;
	struct udevice *ebc_tcon_dev;
	struct udevice *ebc_pwr_dev;
	struct udevice *regulator_dev;
	struct udevice *thermal_dev;
	int vcom;
	struct udevice *backlight0;
	struct udevice *backlight1;
};

enum {
	EBC_PWR_DOWN = 0,
	EBC_PWR_ON = 1,
};

#define EBOOK_VCOM_ID		17
#define EBOOK_VCOM_MAX		64
#define VCOM_DEFAULT_VALUE	1650
#define LOGO_BUF_MAX		3 // last buf for tmp logo, other for other logo

static struct logo_info ebook_logo_info;
static void *logo_buf_addrs[LOGO_BUF_MAX];
static int pre_logo_buf_indx = 0;
static int cur_logo_buf_indx = 0;
static int tmp_logo_buf_indx = LOGO_BUF_MAX - 1;
static void *logo_part_hdr;
static struct udevice *ebook_dev;
static volatile int last_logo_type = -1;
static int read_vcom_from_vendor(void)
{
	int ret = 0;
	char vcom_str[EBOOK_VCOM_MAX] = {0};
	char vcom_args[EBOOK_VCOM_MAX] = {0};

	/* Read vcom value from vendor storage part */
	ret = vendor_storage_read(EBOOK_VCOM_ID, vcom_str, (EBOOK_VCOM_MAX - 1));
	if (ret > 0) {
		snprintf(vcom_args, strlen(vcom_str) + 15, "ebc_pmic.vcom=%s", vcom_str);
		printf("ebook update bootargs: %s\n", vcom_args);
		env_update("bootargs", vcom_args);
	} else {
		return ret;
	}

	return atoi(vcom_str);
}

static int read_waveform(struct udevice *dev)
{
	int cnt, start, ret;
	struct disk_partition part;
	struct blk_desc *dev_desc;
	struct ebc_panel *plat = dev_get_plat(dev);

	dev_desc = plat_bootdev();
	if (!dev_desc) {
		printf("%s: Could not find device\n", __func__);
		return -EIO;
	}
	if (part_get_info_by_name(dev_desc, PART_WAVEFORM, &part) < 0) {
		printf("Get waveform partition failed\n");
		return -ENODEV;
	}
	cnt = plat->lut_pbuf_size / RK_BLK_SIZE;
	start = part.start;
	ret = blk_dread(dev_desc, start, cnt, (void *)plat->lut_pbuf);
	if (ret != cnt)
		printf("Try to read %d blocks failed, only read %d\n",
		       cnt, ret);

	flush_dcache_range((ulong)plat->lut_pbuf,
			   ALIGN((ulong)plat->lut_pbuf + cnt,
				 CONFIG_SYS_CACHELINE_SIZE));
	ret = epd_lut_from_mem_init(plat->lut_pbuf);
	if (ret < 0) {
		printf("lut init failed\n");
		return -EINVAL;
	}

	return 0;
}

static u32 aligned_image_size_4k(struct udevice *dev)
{
	struct ebc_panel *plat = dev_get_plat(dev);
	u32 w = plat->width;
	u32 h = plat->height;

	return ALIGN((w * h) >> 1, 4096);
}

/*
 * This driver load the grayscale image from flash,
 * and put it in the reserve memory which define in dts:
 * display_reserved: framebuffer@10000000 {
 *	reg = <0x0 0x10000000 0x0 0x2000000>;
 *	no-map;
 * };
 * Every image logo size must be aligned in 4K.
 */
static void *get_addr_by_index(struct udevice *dev, int index)
{
	u32 img_size;
	struct ebc_panel *plat = dev_get_plat(dev);

	if (plat->disp_pbuf_size == 0 || !plat->disp_pbuf) {
		printf("invalid display buffer, please check dts\n");
		return NULL;
	}

	if (index >= LOGO_BUF_MAX || index < 0) {
		printf("invalid index %d\n", index);
		return NULL;
	}

	if (logo_buf_addrs[index] != 0)
		return logo_buf_addrs[index];

	img_size = aligned_image_size_4k(dev);
	if ((index + 1) * img_size > plat->disp_pbuf_size) {
		printf("reserve display memory size is not enough\n");
		return NULL;
	}

	logo_buf_addrs[index] = (plat->disp_pbuf + index * img_size);

	return logo_buf_addrs[index];
}

static int update_logo_buf_indx(void)
{
	pre_logo_buf_indx = cur_logo_buf_indx;
	cur_logo_buf_indx = (cur_logo_buf_indx + 1) % (LOGO_BUF_MAX - 1);
	return cur_logo_buf_indx;
}

static int read_header(struct blk_desc *dev_desc,
		       struct disk_partition *part)
{
	int i, ret;
	size_t part_hdr_size = sizeof(struct logo_part_header);
	u32 blk_count = 1;
	struct logo_part_header *part_hdr;
	struct grayscale_header *img_hdr;

	if (ebook_logo_info.part_hdr && ebook_logo_info.img_hdr)
		return 0;

	logo_part_hdr = kzalloc(dev_desc->blksz, GFP_KERNEL);
	if (!logo_part_hdr)
		return -ENOMEM;
	part_hdr = (struct logo_part_header *)logo_part_hdr;

	if (blk_dread(dev_desc, part->start, blk_count, logo_part_hdr) != 1) {
		ret = -EIO;
		goto err;
	}

	if (memcmp(part_hdr->magic, EBOOK_LOGO_PART_MAGIC, 4)) {
		printf("partition header is invalid\n");
		ret = -EINVAL;
		goto err;
	}
	if (part_hdr->logo_count == 0) {
		printf("the count of logo image is 0\n");
		ret = -EINVAL;
		goto err;
	}
	debug("found %d logo, w=%d, h=%d\n", part_hdr->logo_count,
	      part_hdr->screen_width, part_hdr->screen_height);

	part_hdr_size += part_hdr->logo_count * sizeof(struct grayscale_header);
	blk_count = DIV_ROUND_UP(part_hdr_size, dev_desc->blksz);
	kfree(logo_part_hdr);
	logo_part_hdr = kzalloc(dev_desc->blksz * blk_count, GFP_KERNEL);
	if (!logo_part_hdr)
		return -ENOMEM;
	part_hdr = (struct logo_part_header *)logo_part_hdr;

	if (blk_dread(dev_desc, part->start, blk_count, logo_part_hdr) != blk_count) {
		ret = -EIO;
		goto err;
	}

	img_hdr = (struct grayscale_header *)(logo_part_hdr + sizeof(struct logo_part_header));
	for (i = 0; i < part_hdr->logo_count; i++) {
		if (memcmp(img_hdr[i].magic, EBOOK_LOGO_IMAGE_MAGIC, 4)) {
			char img_magic[5] = {0};
			memcpy(img_magic, img_hdr[i].magic, 4);
			printf("image[%d] header '%s' is invalid\n", i,
			       img_magic);
			ret = -EINVAL;
			goto err;
		}
		debug("found logo[%d]\n", img_hdr[i].logo_type);
	}

	ebook_logo_info.part_hdr = part_hdr;
	ebook_logo_info.img_hdr = img_hdr;

	return 0;

err:
	kfree(logo_part_hdr);
	return ret;
}

static int read_grayscale(struct blk_desc *dev_desc,
			  struct disk_partition *part, u32 offset,
			  u32 size, void *buf)
{
	u32 blk_start, blk_offset, blk_count;

	blk_offset = DIV_ROUND_UP(offset, dev_desc->blksz);
	blk_start = part->start + blk_offset;
	blk_count = DIV_ROUND_UP(size, dev_desc->blksz);

	debug("blk_offset=%d, blk_start=%d,blk_count=%d,out buf=%p\n",
	      blk_offset, blk_start, blk_count, buf);
	if (blk_dread(dev_desc, blk_start, blk_count, buf) != blk_count) {
		printf("read grayscale data failed\n");
		return -EIO;
	}

	return 0;
}

static int image_rearrange(u8 *in_buf, u8 *out_buf, u16 w, u16 h)
{
	int i, j;
	u8 in_data;
	u8 *out_buf_tmp;

	if (!in_buf || !out_buf) {
		printf("rearrange in buffer or out buffer is NULL\n");
		return -EINVAL;
	}

	for (i = 0; i < h; i += 2) {
		out_buf_tmp = out_buf + (i * w / 2);
		for (j = 0; j < w / 2; j++) {
			in_data = *in_buf++;
			*(out_buf_tmp + j * 2) = in_data & 0x0f;
			*(out_buf_tmp + j * 2 + 1) = (in_data >> 4) & 0x0f;
		}
		for (j = 0; j < w / 2; j++) {
			in_data = *in_buf++;
			*(out_buf_tmp + j * 2) |= (in_data << 4) & 0xf0;
			*(out_buf_tmp + j * 2 + 1) |= in_data & 0xf0;
		}
	}

	return 0;
}

static int image_mirror(u8 *in_buf, u8 *out_buf, u16 w, u16 h)
{
	int i;

	if (!in_buf || !out_buf) {
		printf("mirror in buffer or out buffer is NULL\n");
		return -EINVAL;
	}

	for (i = 0; i < h; i++) {
		u16 column_len = w / 2;
		u8 *column_in = in_buf + i * column_len;
		u8 *column_out = out_buf + (h - i - 1) * column_len;

		memcpy(column_out, column_in, column_len);
	}

	return 0;
}

/*
 * The ebook kernel driver need last frame to do part refresh,
 * so we need to transfer two images to kernel, which is kernel
 * logo and the logo displayed in uboot.
 *
 * this function use logo type bitmap to indicate several logo.
 * u32 needed_logo: we only load needed logo image into ram, such as
 *                   uboot logo + kernel logo or charger logo + kernel
 *                   logo
 * u32 *loaded_logo: because the needed logo may not exist in logo.img,
 *                  store the really loaded logo in para loaded_logo.
 */
static int read_needed_logo_from_partition(struct udevice *dev,
					   u32 needed_logo,
					   u32 *loaded_logo)
{
	int ret, i;
	struct disk_partition part;
	struct blk_desc *dev_desc;
	struct logo_part_header *part_hdr;
	struct grayscale_header *img_hdr;
	struct ebc_panel *panel = dev_get_plat(dev);
	u32 logo = needed_logo;

	dev_desc = plat_bootdev();
	if (!dev_desc) {
		printf("%s: Could not find device\n", __func__);
		return -EIO;
	}

	if (part_get_info_by_name(dev_desc, PART_LOGO, &part) < 0)
		return -ENODEV;

	ret = read_header(dev_desc, &part);
	if (ret < 0) {
		printf("ebook logo read header failed,ret = %d\n", ret);
		return -EINVAL;
	}
	part_hdr = ebook_logo_info.part_hdr;
	if (part_hdr->screen_width != panel->width ||
	    part_hdr->screen_height != panel->height){
		printf("logo size(%dx%d) is not same as screen size(%dx%d)\n",
		       part_hdr->screen_width, part_hdr->screen_height,
			panel->width, panel->height);
		return -EINVAL;
	}

	memset(logo_buf_addrs, 0, sizeof(logo_buf_addrs));
	img_hdr = ebook_logo_info.img_hdr;
	for (i = 0; i < part_hdr->logo_count; i++) {
		void *pic_buf;
		u32 offset = img_hdr[i].data_offset;
		u32 size = img_hdr[i].data_size;
		u32 logo_type = img_hdr[i].logo_type;

		debug("offset=0x%x, size=%d,logo_type=%d,w=%d,h=%d\n",
		      offset, size, logo_type, img_hdr[i].w, img_hdr[i].h);

		if (logo & logo_type) {
			pic_buf = get_addr_by_index(dev, cur_logo_buf_indx);

			if (pic_buf == NULL) {
				printf("Get buffer failed for image %d\n",
				       img_hdr[i].logo_type);
				return -EIO;
			}
			if (!IS_ALIGNED((ulong)pic_buf, ARCH_DMA_MINALIGN)) {
				printf("disp buffer is not dma aligned\n");
				return -EINVAL;
			}
			/*
			 * kernel logo is transmitted to kernel to display, and
			 * kernel will do the mirror operation, so skip kernel
			 * logo here.
			 */
			if (panel->mirror && logo_type != EBOOK_LOGO_KERNEL) {
				u32 w = panel->width;
				u32 h = panel->height;
				void *mirror_buf = NULL;

				mirror_buf = get_addr_by_index(dev, tmp_logo_buf_indx);
				if (mirror_buf == NULL) {
					printf("get mirror buffer failed\n");
					return -EIO;
				}
				read_grayscale(dev_desc, &part, offset, size, mirror_buf);
				image_mirror((u8 *)mirror_buf, (u8 *)pic_buf, w, h);
			} else if (panel->rearrange && logo_type != EBOOK_LOGO_KERNEL) {
				u32 w = panel->width;
				u32 h = panel->height;
				void *rearrange_buf = NULL;

				rearrange_buf = get_addr_by_index(dev, tmp_logo_buf_indx);
				if (rearrange_buf == NULL) {
					printf("get mirror buffer failed\n");
					return -EIO;
				}
				read_grayscale(dev_desc, &part, offset, size, rearrange_buf);
				image_rearrange((u8 *)rearrange_buf, (u8 *)pic_buf, w, h);
			} else {
				read_grayscale(dev_desc, &part, offset, size, pic_buf);
			}
			flush_dcache_range((ulong)pic_buf,
					   ALIGN((ulong)pic_buf + size,
						 CONFIG_SYS_CACHELINE_SIZE));
			*loaded_logo = logo_type;

			logo &= ~logo_type;
			if (!logo)
				break;
		}
	}

	return 0;
}

static int ebc_power_set(struct udevice *dev, int is_on)
{
	int ret;
	struct rockchip_ebook_display_priv *priv = dev_get_priv(dev);
	struct ebc_panel *panel = dev_get_plat(dev);
	struct udevice *ebc_tcon_dev = priv->ebc_tcon_dev;
	struct rk_ebc_tcon_ops *ebc_tcon_ops = ebc_tcon_get_ops(ebc_tcon_dev);
	struct udevice *ebc_pwr_dev = priv->ebc_pwr_dev;
	struct rk_ebc_pwr_ops *pwr_ops = NULL;

	if (ebc_pwr_dev)
		pwr_ops = ebc_pwr_get_ops(ebc_pwr_dev);

	if (is_on) {
		if (pwr_ops)
			ret = pwr_ops->power_on(ebc_pwr_dev);
		else
			ret  = regulator_set_enable(priv->regulator_dev, true);
		if (ret) {
			printf("%s, power on failed\n", __func__);
			return -1;
		}
		ret = ebc_tcon_ops->enable(ebc_tcon_dev, panel);
		if (ret) {
			printf("%s, ebc tcon enabled failed\n", __func__);
			return -1;
		}
	} else {
		ret = ebc_tcon_ops->disable(ebc_tcon_dev);
		if (ret) {
			printf("%s, ebc tcon disable failed\n", __func__);
			return -1;
		}

		if (pwr_ops)
			ret = pwr_ops->power_down(ebc_pwr_dev);
		else
			ret  = regulator_set_enable(priv->regulator_dev, false);
		if (ret) {
			printf("%s, power_down failed\n", __func__);
			return -1;
		}
	}
	return 0;
}

static int ebook_display(struct udevice *dev, void *pre_img_buf,
			 void *cur_img_buf, u32 lut_type, int update_mode)
{
	int temperature;
	u32 frame_num;
	struct rockchip_ebook_display_priv *priv = dev_get_priv(dev);
	struct ebc_panel *plat = dev_get_plat(dev);
	struct udevice *ebc_pwr_dev = priv->ebc_pwr_dev;
	struct rk_ebc_pwr_ops *pwr_ops = NULL;
	struct udevice *ebc_tcon_dev = priv->ebc_tcon_dev;
	struct rk_ebc_tcon_ops *ebc_tcon_ops = ebc_tcon_get_ops(ebc_tcon_dev);

	if (ebc_pwr_dev)
		pwr_ops = ebc_pwr_get_ops(ebc_pwr_dev);

	if (pwr_ops)
		pwr_ops->temp_get(ebc_pwr_dev, (u32 *)(&temperature));
	else
		thermal_get_temp(priv->thermal_dev, &temperature);
	if (temperature <= 0 || temperature > 50) {
		printf("temperature = %d, out of range0~50 ,use 25\n",
		       temperature);
		temperature = 25;
	}

	if(!plat->lut_data.wf_table[0])
		plat->lut_data.wf_table[0] = kzalloc(MAXFRAME * 32 * 32, GFP_KERNEL);
	epd_lut_get(&plat->lut_data, lut_type, temperature, WF_4BIT, 0, 0);
	kfree(plat->lut_data.wf_table[0]);
	plat->lut_data.wf_table[0] = NULL;

	frame_num = plat->lut_data.frame_num & 0xff;
	printf("lut_type=%d, frame num=%d, temp=%d\n", lut_type,
	       frame_num, temperature);

	ebc_tcon_ops->lut_data_set(ebc_tcon_dev, plat->lut_data.data,
				   frame_num, 0);
	ebc_tcon_ops->dsp_mode_set(ebc_tcon_dev, update_mode,
				   LUT_MODE, !THREE_WIN_MODE, !EINK_MODE);
	ebc_tcon_ops->image_addr_set(ebc_tcon_dev, (u32)((ulong)pre_img_buf),
				     (u32)((ulong)cur_img_buf));
	ebc_tcon_ops->frame_start(ebc_tcon_dev, frame_num);

	ebc_tcon_ops->wait_for_last_frame_complete(ebc_tcon_dev);

	update_logo_buf_indx();

	return 0;
}

static int rk_ebook_display_init(void)
{
	int ret;
	struct uclass *uc;
	struct udevice *dev;

	if (ebook_dev) {
		printf("ebc-dev is already initialized!\n");
		return 0;
	}

	ret = uclass_get(UCLASS_EBOOK_DISPLAY, &uc);
	if (ret) {
		printf("can't find uclass ebook\n");
		return -ENODEV;
	}
	for (uclass_first_device(UCLASS_EBOOK_DISPLAY, &dev);
	     dev; uclass_next_device(&dev))
		;

	if (ebook_dev) {
		printf("ebc-dev is probed success!\n");
		return 0;
	}
	printf("Can't find ebc-dev\n");
	return -ENODEV;
}

static int rockchip_ebook_transmit_kernel_logo(struct udevice *dev)
{
	char logo_args[64] = {0};
	static u32 loaded_logo = 0;
	int ret;

	ret = read_needed_logo_from_partition(dev, EBOOK_LOGO_KERNEL,
						&loaded_logo);
	if (ret || !(loaded_logo & EBOOK_LOGO_KERNEL)) {
		printf("No invalid kernel logo in logo.img\n");
		return -EIO;
	} else {
		void *klogo_addr = get_addr_by_index(dev, cur_logo_buf_indx);

		if (klogo_addr == NULL) {
			printf("get kernel logo buffer failed\n");
			return -EIO;
		}
		printf("Transmit kernel logo addr(0x%x) to kernel\n",
			(u32)(ulong)klogo_addr);
		sprintf(logo_args, "klogo_addr=0x%x", (u32)(ulong)klogo_addr);
		env_update("bootargs", logo_args);
	}

	return 0;
}

/*
 * Ebook display need current and previous image buffer, We assume
 * every type of logo has only one image, so just tell this function
 * last logo type and current logo type, it will find the right images.
 * last_logo_type: -1 means it's first displaying.
 */
static int rockchip_ebook_show_logo(int cur_logo_type, int update_mode)
{
	int ret = 0;
	void *logo_addr, *last_logo_addr;
	struct ebc_panel *plat;
	struct udevice *dev;
	static u32 loaded_logo = 0;
	struct rockchip_ebook_display_priv *priv;

	if (!ebook_dev) {
		static bool first_init = true;

		if (first_init) {
			first_init = false;
			ret = rk_ebook_display_init();
			if (ret) {
				printf("Get ebc dev failed, check dts\n");
				return -ENODEV;
			}
		} else {
			return -ENODEV;
		}
	}
	dev = ebook_dev;

	/*Don't need to update display*/
	if (last_logo_type == cur_logo_type) {
		debug("Same as last picture, Don't need to display\n");
		return 0;
	}

	plat = dev_get_plat(dev);
	priv = dev_get_priv(dev);

	/*
	 * The last_logo_type is -1 means it's first displaying
	 */
	if (last_logo_type == -1) {
		ret = ebc_power_set(dev, EBC_PWR_ON);
		if (ret) {
			printf("Ebook power on failed\n");
			ret = -EIO;
			goto out;
		}

		int size = (plat->width * plat->height) >> 1;

		logo_addr = get_addr_by_index(dev, cur_logo_buf_indx);
		memset(logo_addr, 0xff, size);
		flush_dcache_range((ulong)logo_addr,
				   ALIGN((ulong)logo_addr + size,
					 CONFIG_SYS_CACHELINE_SIZE));
		debug("show reset logo, addr=0x%x\n", (u32)(ulong)logo_addr);
		ebook_display(dev, logo_addr, logo_addr, WF_TYPE_RESET, EBOOK_LOGO_RESET);
		last_logo_type = 0;
		last_logo_addr = logo_addr;
	} else {
		last_logo_addr = get_addr_by_index(dev, pre_logo_buf_indx);
		if (last_logo_addr == NULL) {
			printf("Invalid last logo addr, exit!\n");
			ret = -EIO;
			goto out;
		}
	}
	ret = read_needed_logo_from_partition(dev, cur_logo_type,
					      &loaded_logo);
	if (ret || !(loaded_logo & cur_logo_type)) {
		printf("read logo[0x%x] failed, loaded_logo=0x%x\n",
		       cur_logo_type, loaded_logo);
		ret = -EIO;
		goto out;
	}
	logo_addr = get_addr_by_index(dev, cur_logo_buf_indx);
	debug("logo_addr=%p, logo_type=%d\n", logo_addr, cur_logo_type);
	if (logo_addr == NULL) {
		printf("get logo buffer failed\n");
		ret = -EIO;
		goto out;
	}

	debug("show logo, pre addr: 0x%x, cur addr: 0x%x, type: %d\n",
	      (u32)((ulong)last_logo_addr), (u32)((ulong)logo_addr), ffs(cur_logo_type));
	ebook_display(dev, last_logo_addr, logo_addr, WF_TYPE_GC16, update_mode);

	if (priv->backlight0)
		backlight_enable(priv->backlight0);
	if (priv->backlight1)
		backlight_enable(priv->backlight1);

	last_logo_type = cur_logo_type;

	if (cur_logo_type == EBOOK_LOGO_POWEROFF) {
		struct udevice *ebc_tcon_dev = priv->ebc_tcon_dev;
		struct rk_ebc_tcon_ops *ebc_tcon_ops;

		last_logo_type = -1;
		/*
		 * For normal logo display, waiting for the last frame
		 * completion before start a new frame, except one
		 * situation which charging logo display finished,
		 * because device will rebooting or shutdown after
		 * charging logo is competed.
		 *
		 * We should take care of the power sequence,
		 * because ebc can't power off if last frame
		 * data is still sending, so keep the ebc power
		 * during u-boot phase and shutdown the
		 * power only if uboot charging is finished.
		 */
		ebc_tcon_ops = ebc_tcon_get_ops(ebc_tcon_dev);
		ebc_tcon_ops->wait_for_last_frame_complete(ebc_tcon_dev);
		debug("charging logo displaying is complete\n");
		/*
		 *shutdown ebc after charging logo display is complete
		 */
		ret = ebc_power_set(dev, EBC_PWR_DOWN);
		if (ret)
			printf("Ebook power down failed\n");
		goto out;
	}

	/*
	 * System will boot up to kernel only when the
	 * logo is uboot logo
	 */
	if (cur_logo_type == EBOOK_LOGO_UBOOT) {
		char logo_args[64] = {0};
		void *uboot_logo_buf;

		if (plat->mirror || plat->rearrange)
			uboot_logo_buf = get_addr_by_index(dev, tmp_logo_buf_indx);
		else
			uboot_logo_buf = logo_addr;
		printf("Transmit uboot logo addr(0x%x) to kernel\n", (u32)(ulong)uboot_logo_buf);
		sprintf(logo_args, "ulogo_addr=0x%x", (u32)(ulong)uboot_logo_buf);
		env_update("bootargs", logo_args);
	}

out:
	if (cur_logo_type == EBOOK_LOGO_UBOOT) {
		rockchip_ebook_transmit_kernel_logo(dev);
	}
	return ret;
}

int rockchip_ebook_show_uboot_logo(void)
{
	return rockchip_ebook_show_logo(EBOOK_LOGO_UBOOT, EBOOK_UPDATE_DIFF);
}

int rockchip_ebook_show_charge_logo(int logo_type)
{
	return rockchip_ebook_show_logo(logo_type, EBOOK_UPDATE_DIFF);
}

static int rockchip_ebook_display_probe(struct udevice *dev)
{
	struct rockchip_ebook_display_priv *priv = dev_get_priv(dev);
	struct dm_regulator_uclass_plat *uc_pdata;
	struct rk_ebc_pwr_ops *pwr_ops = NULL;
	struct udevice *child, *pmic_dev;
	int ret, vcom, size, i, uclass_id;
	bool find_pmic = false;
	const fdt32_t *list;
	uint32_t phandle;

	/* Before relocation we don't need to do anything */
	if (!(gd->flags & GD_FLG_RELOC))
		return 0;

	vcom = read_vcom_from_vendor();
	if (vcom <= 0) {
		printf("read vcom from vendor failed, use default vcom\n");
		priv->vcom = VCOM_DEFAULT_VALUE;
	} else {
		priv->vcom = vcom;
	}

	ret = uclass_get_device_by_phandle(UCLASS_EBC, dev,
					   "ebc_tcon",
					   &priv->ebc_tcon_dev);
	if (ret) {
		dev_err(dev, "Cannot get ebc_tcon: %d\n", ret);
		goto out;
	}

	list = dev_read_prop(dev, "pmic", &size);
	if (!list) {
		dev_err(dev, "Cannot get pmic prop\n");
		ret = -EINVAL;
		goto out;
	}

	size /= sizeof(*list);
	for (i = 0; i < size; i++) {
		phandle = fdt32_to_cpu(*list++);
		/* TODO: migrate to pmic */
		ret = uclass_get_device_by_phandle_id(UCLASS_I2C_GENERIC,
						      phandle,
						      &priv->ebc_pwr_dev);
		if (!ret) {
			find_pmic = true;
			break;
		}

		ret = uclass_get_device_by_phandle_id(UCLASS_PMIC, phandle, &pmic_dev);
		if (!ret) {
			for (device_find_first_child(pmic_dev, &child); child;
			     device_find_next_child(&child)) {
				uclass_id = device_get_uclass_id(child);
				ret = device_probe(child);
				if (ret) {
					dev_warn(dev, "Failed to probe pmic %s\n", child->name);
					continue;
				}

				if (uclass_id == UCLASS_REGULATOR) {
					uc_pdata = dev_get_uclass_plat(child);
					if (!strcmp(uc_pdata->name, "vcom"))
						priv->regulator_dev = child;
				} else if (uclass_id == UCLASS_THERMAL) {
					priv->thermal_dev = child;
				}
			}

			find_pmic = true;
			break;
		}
	}

	if (!find_pmic) {
		dev_err(dev, "Cannot get pmic: %d\n", ret);
		goto out;
	}

	list = dev_read_prop(dev, "backlight", &size);
	if (!list) {
		dev_warn(dev, "No backlight\n");
	} else {
		size /= sizeof(*list);
		phandle = fdt32_to_cpu(*list++);
		ret = uclass_get_device_by_phandle_id(UCLASS_PANEL_BACKLIGHT, phandle, &priv->backlight0);
		if (size > 1) {
			phandle = fdt32_to_cpu(*list);
			ret = uclass_get_device_by_phandle_id(UCLASS_PANEL_BACKLIGHT, phandle, &priv->backlight1);
		}
	}

	if (priv->ebc_pwr_dev)
		pwr_ops = ebc_pwr_get_ops(priv->ebc_pwr_dev);

	if (priv->ebc_pwr_dev)
		ret = pwr_ops->vcom_set(priv->ebc_pwr_dev, priv->vcom);
	else
		ret = regulator_set_value(priv->regulator_dev, priv->vcom * 1000);
	if (ret) {
		printf("%s, vcom_set failed\n", __func__);
		ret = -EIO;
		goto out;
	}

	ebook_dev = dev;

out:
	// read lut to ram, and get lut ops
	if (read_waveform(dev) < 0) {
		printf("read wavform failed\n");
		ret = -EIO;
	}

	// Even if the probe fails, ensure the kernel logo is transmited to the kernel
	if (ret) {
		if (rockchip_ebook_transmit_kernel_logo(dev) < 0) {
			printf("transmit kernel logo failed\n");
			ret = -EIO;
		}
	}

	return ret;
}

static int rockchip_ebook_display_ofdata_to_platdata(struct udevice *dev)
{
	fdt_size_t size;
	fdt_addr_t tmp_addr;
	struct device_node *disp_mem;
	struct device_node *waveform_mem;
	struct ebc_panel *plat = dev_get_plat(dev);
	void * data;
	int len;

	data = (void *)dev_read_prop(dev, "wf,mode_table", &len);
	if (len > 0 && pvi_wf_add_custom_mode_table(data, len))
		return -ENODEV;

	plat->width = dev_read_u32_default(dev, "panel,width", 0);
	plat->height = dev_read_u32_default(dev, "panel,height", 0);
	plat->vir_width = dev_read_u32_default(dev, "panel,vir_width", plat->width);
	plat->vir_height = dev_read_u32_default(dev, "panel,vir_height", plat->height);
	plat->sdck = dev_read_u32_default(dev, "panel,sdck", 0);
	plat->lsl = dev_read_u32_default(dev, "panel,lsl", 0);
	plat->lbl = dev_read_u32_default(dev, "panel,lbl", 0);
	plat->ldl = dev_read_u32_default(dev, "panel,ldl", 0);
	plat->lel = dev_read_u32_default(dev, "panel,lel", 0);
	plat->gdck_sta = dev_read_u32_default(dev, "panel,gdck-sta", 0);
	plat->lgonl = dev_read_u32_default(dev, "panel,lgonl", 0);
	plat->fsl = dev_read_u32_default(dev, "panel,fsl", 0);
	plat->fbl = dev_read_u32_default(dev, "panel,fbl", 0);
	plat->fdl = dev_read_u32_default(dev, "panel,fdl", 0);
	plat->fel = dev_read_u32_default(dev, "panel,fel", 0);
	plat->panel_16bit = dev_read_u32_default(dev, "panel,panel_16bit", 0);
	plat->panel_color = dev_read_u32_default(dev, "panel,panel_color", 0);
	plat->mirror = dev_read_u32_default(dev, "panel,mirror", 0);
	plat->rearrange = dev_read_u32_default(dev, "panel,rearrange", 0);
	plat->width_mm = dev_read_u32_default(dev, "panel,width-mm", 0);
	plat->height_mm = dev_read_u32_default(dev, "panel,height-mm", 0);
	plat->sdce_width = dev_read_u32_default(dev, "panel,sdce_width", 0);
	plat->sdoe_mode = dev_read_u32_default(dev, "panel,sdoe_mode", 0);
	plat->lel_keep_clk = dev_read_u32_default(dev, "panel,lel-keep-clk", 1);

	disp_mem = of_parse_phandle(ofnode_to_np(dev_ofnode(dev)),
				    "memory-region", 0);
	if (!disp_mem) {
		dev_err(dev, "Cannot get memory-region from dts\n");
		return -ENODEV;
	}
	tmp_addr = ofnode_get_addr_size(np_to_ofnode(disp_mem), "reg", &size);
	if (tmp_addr == FDT_ADDR_T_NONE) {
		printf("get display memory address failed\n");
		return -ENODEV;
	}

	plat->disp_pbuf = map_sysmem(tmp_addr, 0);
	plat->disp_pbuf_size = size;
	debug("display mem=0x%p, size=%x\n", plat->disp_pbuf,
	      plat->disp_pbuf_size);
	waveform_mem = of_parse_phandle(ofnode_to_np(dev_ofnode(dev)),
					"waveform-region", 0);
	if (!waveform_mem) {
		printf("Cannot get waveform-region from dts\n");
		return -ENODEV;
	}
	tmp_addr = ofnode_get_addr_size(np_to_ofnode(waveform_mem),
					"reg", &size);
	if (tmp_addr == FDT_ADDR_T_NONE) {
		printf("get waveform memory address failed\n");
		return -ENODEV;
	}

	plat->lut_pbuf = map_sysmem(tmp_addr, 0);
	plat->lut_pbuf_size = size;
	debug("lut mem=0x%p, size=%x\n", plat->lut_pbuf, plat->lut_pbuf_size);
	return 0;
}

static const struct udevice_id rockchip_ebook_display_ids[] = {
	{ .compatible = "rockchip,ebc-dev", },
	{}
};

U_BOOT_DRIVER(rk_ebook_display) = {
	.name = "rockchip_ebook_display",
	.id = UCLASS_EBOOK_DISPLAY,
	.of_match = rockchip_ebook_display_ids,
	.of_to_plat = rockchip_ebook_display_ofdata_to_platdata,
	.probe = rockchip_ebook_display_probe,
	.priv_auto = sizeof(struct rockchip_ebook_display_priv),
	.plat_auto = sizeof(struct ebc_panel),
};

UCLASS_DRIVER(rk_ebook) = {
	.id	= UCLASS_EBOOK_DISPLAY,
	.name	= "rk_ebook",
};
