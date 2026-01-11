// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2017 exceet electronics GmbH
 *
 * Authors:
 *	Frieder Schrempf <frieder.schrempf@exceet.de>
 *	Boris Brezillon <boris.brezillon@bootlin.com>
 */

#ifndef __UBOOT__
#include <linux/device.h>
#include <linux/kernel.h>
#endif
#include <linux/mtd/spinand.h>

#define SPINAND_MFR_WINBOND		0xEF

#define WINBOND_CFG_BRC_READ		BIT(1)
#define WINBOND_CFG_BUF_READ		BIT(3)

static SPINAND_OP_VARIANTS(read_cache_variants,
		SPINAND_PAGE_READ_FROM_CACHE_QUADIO_OP(0, 2, NULL, 0),
		SPINAND_PAGE_READ_FROM_CACHE_X4_OP(0, 1, NULL, 0),
		SPINAND_PAGE_READ_FROM_CACHE_DUALIO_OP(0, 1, NULL, 0),
		SPINAND_PAGE_READ_FROM_CACHE_X2_OP(0, 1, NULL, 0),
		SPINAND_PAGE_READ_FROM_CACHE_OP(true, 0, 1, NULL, 0),
		SPINAND_PAGE_READ_FROM_CACHE_OP(false, 0, 1, NULL, 0));

#if defined(CONFIG_SPL_BUILD) && defined(CONFIG_SPI_NAND_CONT_READ)
static SPINAND_OP_VARIANTS(read_cache_variants_cont,
		SPINAND_PAGE_READ_FROM_CACHE_X4_OP_3A(0, 1, NULL, 0));
#endif

static SPINAND_OP_VARIANTS(write_cache_variants,
		SPINAND_PROG_LOAD_X4(true, 0, NULL, 0),
		SPINAND_PROG_LOAD(true, 0, NULL, 0));

static SPINAND_OP_VARIANTS(update_cache_variants,
		SPINAND_PROG_LOAD_X4(false, 0, NULL, 0),
		SPINAND_PROG_LOAD(false, 0, NULL, 0));

static int w25m02gv_ooblayout_ecc(struct mtd_info *mtd, int section,
				  struct mtd_oob_region *region)
{
	if (section > 3)
		return -ERANGE;

	region->offset = (16 * section) + 8;
	region->length = 8;

	return 0;
}

static int w25m02gv_ooblayout_free(struct mtd_info *mtd, int section,
				   struct mtd_oob_region *region)
{
	if (section > 3)
		return -ERANGE;

	region->offset = (16 * section) + 2;
	region->length = 6;

	return 0;
}

static const struct mtd_ooblayout_ops w25m02gv_ooblayout = {
	.ecc = w25m02gv_ooblayout_ecc,
	.rfree = w25m02gv_ooblayout_free,
};

static int w25m02gv_select_target(struct spinand_device *spinand,
				  unsigned int target)
{
	struct spi_mem_op op = SPI_MEM_OP(SPI_MEM_OP_CMD(0xc2, 1),
					  SPI_MEM_OP_NO_ADDR,
					  SPI_MEM_OP_NO_DUMMY,
					  SPI_MEM_OP_DATA_OUT(1,
							spinand->scratchbuf,
							1));

	*spinand->scratchbuf = target;
	return spi_mem_exec_op(spinand->slave, &op);
}

static int w25n02kv_ooblayout_ecc(struct mtd_info *mtd, int section,
				  struct mtd_oob_region *region)
{
	if (section)
		return -ERANGE;

	region->offset = 64;
	region->length = 64;

	return 0;
}

static int w25n02kv_ooblayout_free(struct mtd_info *mtd, int section,
				   struct mtd_oob_region *region)
{
	if (section)
		return -ERANGE;

	/* Reserve 2 bytes for the BBM. */
	region->offset = 2;
	region->length = 62;

	return 0;
}

static const struct mtd_ooblayout_ops w25n02kv_ooblayout = {
	.ecc = w25n02kv_ooblayout_ecc,
	.rfree = w25n02kv_ooblayout_free,
};

static int w25n02kv_ecc_get_status(struct spinand_device *spinand,
				   u8 status)
{
	struct nand_device *nand = spinand_to_nand(spinand);

	switch (status & STATUS_ECC_MASK) {
	case STATUS_ECC_NO_BITFLIPS:
		return 0;

	case STATUS_ECC_UNCOR_ERROR:
		return -EBADMSG;

	case STATUS_ECC_HAS_BITFLIPS:
		return 1;

	default:
		return nand->eccreq.strength;
	}

	return -EINVAL;
}

static int w25n04lw_ooblayout_ecc(struct mtd_info *mtd, int section,
				    struct mtd_oob_region *region)
{
	if (section)
		return -ERANGE;

	region->offset = mtd->oobsize / 2;
	region->length = mtd->oobsize / 2;

	return 0;
}

static int w25n04lw_ooblayout_free(struct mtd_info *mtd, int section,
				     struct mtd_oob_region *region)
{
	if (section)
		return -ERANGE;

	region->offset = 2;
	region->length = mtd->oobsize / 2 - 2;

	return 0;
}

static const struct mtd_ooblayout_ops w25n04lw_ooblayout = {
	.ecc = w25n04lw_ooblayout_ecc,
	.rfree = w25n04lw_ooblayout_free,
};

/* Another set for the same id[2] devices in one series */
static const struct spinand_info winbond_spinand_table[] = {
	SPINAND_INFO("W25M02GV",
		     SPINAND_ID(SPINAND_READID_METHOD_OPCODE_DUMMY, 0xAB),
		     NAND_MEMORG(1, 2048, 64, 64, 1024, 1, 1, 2),
		     NAND_ECCREQ(1, 512),
		     SPINAND_INFO_OP_VARIANTS(&read_cache_variants,
					      &write_cache_variants,
					      &update_cache_variants),
		     0,
		     SPINAND_ECCINFO(&w25m02gv_ooblayout, NULL),
		     SPINAND_SELECT_TARGET(w25m02gv_select_target)),
	SPINAND_INFO("W25N512GV",
		     SPINAND_ID(SPINAND_READID_METHOD_OPCODE_DUMMY, 0xAA, 0x20),
		     NAND_MEMORG(1, 2048, 64, 64, 512, 1, 1, 1),
		     NAND_ECCREQ(1, 512),
		     SPINAND_INFO_OP_VARIANTS(&read_cache_variants,
					      &write_cache_variants,
					      &update_cache_variants),
		     0,
		     SPINAND_ECCINFO(&w25m02gv_ooblayout, NULL),
		     SPINAND_SELECT_TARGET(w25m02gv_select_target)),
	SPINAND_INFO("W25N01GV",
		     SPINAND_ID(SPINAND_READID_METHOD_OPCODE_DUMMY, 0xAA, 0x21),
		     NAND_MEMORG(1, 2048, 64, 64, 1024, 1, 1, 1),
		     NAND_ECCREQ(1, 512),
		     SPINAND_INFO_OP_VARIANTS(&read_cache_variants,
					      &write_cache_variants,
					      &update_cache_variants),
		     0,
		     SPINAND_ECCINFO(&w25m02gv_ooblayout, NULL),
		     SPINAND_SELECT_TARGET(w25m02gv_select_target)),
	SPINAND_INFO("W25N02KV",
		     SPINAND_ID(SPINAND_READID_METHOD_OPCODE_DUMMY, 0xAA, 0x22),
		     NAND_MEMORG(1, 2048, 128, 64, 2048, 1, 1, 1),
		     NAND_ECCREQ(8, 512),
		     SPINAND_INFO_OP_VARIANTS(&read_cache_variants,
					      &write_cache_variants,
					      &update_cache_variants),
		     0,
		     SPINAND_ECCINFO(&w25n02kv_ooblayout,
				     w25n02kv_ecc_get_status)),
	SPINAND_INFO("W25N04KV",
		     SPINAND_ID(SPINAND_READID_METHOD_OPCODE_DUMMY, 0xAA, 0x23),
		     NAND_MEMORG(1, 2048, 128, 64, 4096, 1, 1, 1),
		     NAND_ECCREQ(8, 512),
		     SPINAND_INFO_OP_VARIANTS(&read_cache_variants,
					      &write_cache_variants,
					      &update_cache_variants),
		     0,
		     SPINAND_ECCINFO(&w25n02kv_ooblayout,
				     w25n02kv_ecc_get_status)),
	SPINAND_INFO("W25N01GW",
		     SPINAND_ID(SPINAND_READID_METHOD_OPCODE_DUMMY, 0xBA, 0x21),
		     NAND_MEMORG(1, 2048, 64, 64, 1024, 1, 1, 1),
		     NAND_ECCREQ(1, 512),
		     SPINAND_INFO_OP_VARIANTS(&read_cache_variants,
					      &write_cache_variants,
					      &update_cache_variants),
		     0,
		     SPINAND_ECCINFO(&w25m02gv_ooblayout, NULL),
		     SPINAND_SELECT_TARGET(w25m02gv_select_target)),
	SPINAND_INFO("W25N02KW",
		     SPINAND_ID(SPINAND_READID_METHOD_OPCODE_DUMMY, 0xBA, 0x22),
		     NAND_MEMORG(1, 2048, 128, 64, 2048, 1, 1, 1),
		     NAND_ECCREQ(8, 512),
		     SPINAND_INFO_OP_VARIANTS(&read_cache_variants,
					      &write_cache_variants,
					      &update_cache_variants),
		     0,
		     SPINAND_ECCINFO(&w25n02kv_ooblayout,
				     w25n02kv_ecc_get_status)),
	SPINAND_INFO("W25N01KV",
		     SPINAND_ID(SPINAND_READID_METHOD_OPCODE_DUMMY, 0xAE, 0x21),
		     NAND_MEMORG(1, 2048, 128, 64, 1024, 1, 1, 1),
		     NAND_ECCREQ(4, 512),
		     SPINAND_INFO_OP_VARIANTS(&read_cache_variants,
					      &write_cache_variants,
					      &update_cache_variants),
		     0,
		     SPINAND_ECCINFO(&w25n02kv_ooblayout,
				     w25n02kv_ecc_get_status)),
	SPINAND_INFO("W25N01JWZEIG",
		     SPINAND_ID(SPINAND_READID_METHOD_OPCODE_DUMMY, 0xBC, 0x21),
		     NAND_MEMORG(1, 2048, 64, 64, 1024, 1, 1, 1),
		     NAND_ECCREQ(1, 512),
		     SPINAND_INFO_OP_VARIANTS(&read_cache_variants,
					      &write_cache_variants,
					      &update_cache_variants),
		     SPINAND_HAS_QE_BIT,
		     SPINAND_ECCINFO(&w25m02gv_ooblayout, NULL)),
	SPINAND_INFO("W25N01KWZPIG",
		     SPINAND_ID(SPINAND_READID_METHOD_OPCODE_DUMMY, 0xBE, 0x21),
		     NAND_MEMORG(1, 2048, 128, 64, 1024, 1, 1, 1),
		     NAND_ECCREQ(4, 512),
		     SPINAND_INFO_OP_VARIANTS(&read_cache_variants,
					      &write_cache_variants,
					      &update_cache_variants),
		     0,
		     SPINAND_ECCINFO(&w25n02kv_ooblayout, w25n02kv_ecc_get_status)),
	SPINAND_INFO("W25N04LW2EIG",
		     SPINAND_ID(SPINAND_READID_METHOD_OPCODE_DUMMY, 0xB2, 0x23),
		     NAND_MEMORG(1, 4096, 256, 64, 2048, 1, 1, 1),
		     NAND_ECCREQ(8, 512),
		     SPINAND_INFO_OP_VARIANTS(&read_cache_variants,
					      &write_cache_variants,
					      &update_cache_variants),
		     0,
		     SPINAND_ECCINFO(&w25n04lw_ooblayout, w25n02kv_ecc_get_status)),
	SPINAND_INFO("W25N08LW2EIG",
		     SPINAND_ID(SPINAND_READID_METHOD_OPCODE_DUMMY, 0xB3, 0x24),
		     NAND_MEMORG(1, 4096, 128, 64, 4096, 1, 1, 1),
		     NAND_ECCREQ(8, 512),
		     SPINAND_INFO_OP_VARIANTS(&read_cache_variants,
					      &write_cache_variants,
					      &update_cache_variants),
		     0,
		     SPINAND_ECCINFO(&w25n02kv_ooblayout, w25n02kv_ecc_get_status)),
	SPINAND_INFO("W25N02JW2EIF",
		     SPINAND_ID(SPINAND_READID_METHOD_OPCODE_DUMMY, 0xBF, 0x22),
		     NAND_MEMORG(1, 2048, 64, 64, 2048, 1, 1, 1),
		     NAND_ECCREQ(1, 512),
		     SPINAND_INFO_OP_VARIANTS(&read_cache_variants,
					      &write_cache_variants,
					      &update_cache_variants),
		     SPINAND_HAS_QE_BIT,
		     SPINAND_ECCINFO(&w25m02gv_ooblayout, NULL)),
	SPINAND_INFO("W25N02LVCEIG",
		     SPINAND_ID(SPINAND_READID_METHOD_OPCODE_DUMMY, 0x8A, 0x22),
		     NAND_MEMORG(1, 2048, 128, 64, 2048, 1, 1, 1),
		     NAND_ECCREQ(8, 512),
		     SPINAND_INFO_OP_VARIANTS(&read_cache_variants,
					      &write_cache_variants,
					      &update_cache_variants),
		     0,
		     SPINAND_ECCINFO(&w25n02kv_ooblayout, w25n02kv_ecc_get_status)),
};

static int winbond_spinand_init(struct spinand_device *spinand)
{
	struct nand_device *nand = spinand_to_nand(spinand);
	unsigned int i;

	/*
	 * Make sure all dies are in buffer read mode and not continuous read
	 * mode.
	 */
	for (i = 0; i < nand->memorg.ntargets; i++) {
		spinand_select_target(spinand, i);
		spinand_upd_cfg(spinand, WINBOND_CFG_BUF_READ,
				WINBOND_CFG_BUF_READ);
	}

	/* W25N01JWZEIG enable continuous read */
#ifdef CONFIG_SPI_NAND_CONT_READ
	if (((spinand->id.data[1] == 0xaa || spinand->id.data[1] == 0xba ||
	     spinand->id.data[1] == 0xbc || spinand->id.data[1] == 0xbe) &&
	     spinand->id.data[2] == 0x21) ||
	     (spinand->id.data[1] == 0xbf && spinand->id.data[2] == 0x22)) {
#ifdef CONFIG_SPL_BUILD
		spinand->support_cont_read = true;
		spinand_upd_cfg(spinand, CFG_BUF_ENABLE, 0);
		spinand->op_templates.read_cache = &read_cache_variants_cont.ops[0];
		printf("Support cont_read\n");
#else
		spinand_upd_cfg(spinand, CFG_BUF_ENABLE, CFG_BUF_ENABLE);
#endif
	}
#endif

	/* W25N0xLV disable BRC in default */
	if ((spinand->id.data[1] == 0x8b && spinand->id.data[2] == 0x23) ||
	    (spinand->id.data[1] == 0x8a && spinand->id.data[2] == 0x22)) {
		spinand_upd_cfg(spinand, WINBOND_CFG_BRC_READ, 0);
		dev_info(&spinand->spimem->spi->dev, "Disable BRC in default\n");
	}

	return 0;
}

static const struct spinand_manufacturer_ops winbond_spinand_manuf_ops = {
	.init = winbond_spinand_init,
};

const struct spinand_manufacturer winbond_spinand_manufacturer = {
	.id = SPINAND_MFR_WINBOND,
	.name = "Winbond",
	.chips = winbond_spinand_table,
	.nchips = ARRAY_SIZE(winbond_spinand_table),
	.ops = &winbond_spinand_manuf_ops,
};
