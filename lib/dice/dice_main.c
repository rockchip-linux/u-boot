// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2026 Rockchip Electronics Co., Ltd
 */

#include <dm.h>
#include <android_image.h>
#include <hexdump.h>
#include <misc.h>
#include <memalign.h>
#include <dice/android.h>
#include <dice/dice.h>
#include <dice/ops.h>
#include <part.h>
#include <spl.h>
#include <linux/stringify.h>
#include <tee/optee.h>

#ifndef DICE_BUF_ADDR
#error "DICE_BUF_ADDR must be defined in platform header"
#endif

#ifndef DICE_BUF_SIZE
#error "DICE_BUF_SIZE must be defined in platform header"
#endif

#if DICE_DEBUG
static void dice_dump_context(struct DiceContext *DiceCtx)
{
	printf("\n==== BCC Context(%s)===\n", DiceCtx->profile_name);
	printf("## UDS pubkey:\n");
	print_hex_dump("    ", DUMP_PREFIX_ADDRESS, 16, 1,
		       DiceCtx->uds_pubkey, DiceCtx->uds_pubkey_size, true);

	printf("## Last subject private key:\n");
	print_hex_dump("    ", DUMP_PREFIX_ADDRESS, 16, 1,
		       DiceCtx->last_subject_privkey,
		       DiceCtx->last_subject_privkey_size, true);

	printf("## Last subject public key:\n");
	print_hex_dump("    ", DUMP_PREFIX_ADDRESS, 16, 1,
		       DiceCtx->last_subject_pubkey,
		       DiceCtx->last_subject_pubkey_size, true);

	printf("\n## BCC: 0x%08lx - 0x%08lx, cert_count=%d\n",
		(ulong)DiceCtx->cert_chain,
		(ulong)DiceCtx->cert_chain + DiceCtx->cert_chain_size,
		DiceCtx->cert_count);
	print_hex_dump("    ", DUMP_PREFIX_ADDRESS, 16, 1,
		       DiceCtx->cert_chain, DiceCtx->cert_chain_size, true);
}

static void dice_dump_bcc_chain(struct DiceContext *DiceCtx)
{
	printf("\n## === BCC Chain(%s): Certificate count=%d, size=0x%x ===\n",
	       DiceCtx->profile_name, DiceCtx->cert_count, DiceCtx->cert_chain_size);
	print_hex_dump("    ", DUMP_PREFIX_ADDRESS, 32, 1,
		       DiceCtx->cert_chain, DiceCtx->cert_chain_size, true);

	printf("## current_cdi_attest[%d]:\n", DiceCtx->cert_count);
	print_hex_dump("    ", DUMP_PREFIX_ADDRESS, 32, 1,
		       DiceCtx->next_cdi_attest, 32, true);
	printf("## current_cdi_seal[%d]:\n", DiceCtx->cert_count);
	print_hex_dump("    ", DUMP_PREFIX_ADDRESS, 32, 1,
		       DiceCtx->next_cdi_seal, 32, true);
}

static void dice_dump_inputs(struct DiceContext *DiceCtx, const DiceInputValues *values)
{
    int i;

    if (!values) {
	printf("DiceInputValues: NULL\n");
	return;
    }

    printf("\n## === DiceInputValues(%s: %s) ===\n",
    	   DiceCtx->profile_name, values->code_descriptor);

    /* code_hash */
    printf("    code_hash: ");
    for (i = 0; i < DICE_HASH_SIZE; i++) {
	printf("%02x", values->code_hash[i]);
    }
    printf("\n");

    /* code_descriptor */
    printf("    code_descriptor: %p\n", values->code_descriptor);
    if (values->code_descriptor && values->code_descriptor_size > 0) {
	printf("    code_descriptor_data: ");
	for (i = 0; i < values->code_descriptor_size && i < 32; i++) {
	    printf("%02x", values->code_descriptor[i]);
	}
	if (values->code_descriptor_size > 32) {
	    printf("...");
	}
	printf("\n");
    }
    printf("    code_descriptor_size: %zu\n", values->code_descriptor_size);

    /* config_type */
    printf("    config_type: %d\n", values->config_type);

    /* config_value */
    printf("    config_value: ");
    for (i = 0; i < DICE_INLINE_CONFIG_SIZE; i++) {
	printf("%02x", values->config_value[i]);
    }
    printf("\n");

    /* config_descriptor */
    printf("    config_descriptor: %p\n", values->config_descriptor);
    if (values->config_descriptor && values->config_descriptor_size > 0) {
	printf("    config_descriptor_data: ");
	for (i = 0; i < values->config_descriptor_size && i < 32; i++) {
	    printf("%02x", values->config_descriptor[i]);
	}
	if (values->config_descriptor_size > 32) {
	    printf("...");
	}
	printf("\n");
    }
    printf("    config_descriptor_size: %zu\n", values->config_descriptor_size);

    /* authority_hash */
    printf("    authority_hash: ");
    for (i = 0; i < DICE_HASH_SIZE; i++) {
	printf("%02x", values->authority_hash[i]);
    }
    printf("\n");

    /* authority_descriptor */
    printf("    authority_descriptor: %p\n", values->authority_descriptor);
    if (values->authority_descriptor && values->authority_descriptor_size > 0) {
	printf("    authority_descriptor_data: ");
	for (i = 0; i < values->authority_descriptor_size && i < 32; i++) {
	    printf("%02x", values->authority_descriptor[i]);
	}
	if (values->authority_descriptor_size > 32) {
	    printf("...");
	}
	printf("\n");
    }
    printf("    authority_descriptor_size: %zu\n", values->authority_descriptor_size);

    /* mode */
    printf("    mode: %d\n", values->mode);

    /* hidden */
    printf("    hidden: ");
    for (i = 0; i < DICE_HIDDEN_SIZE; i++) {
	printf("%02x", values->hidden[i]);
    }
    printf("\n");
}
#endif

static int dice_measure_component(struct DiceContext *DiceCtx,
				  struct DiceFlow *flow)
{
	DiceInputValues inputs;
	char authority_descriptor[] = "authority:rockchip-dice";
	DiceResult result;
	size_t actual_size;
	uint8_t bcc_buffer[4096*4];
	DiceAndroidConfigValues config_descriptor;
	uint8_t current_cdi_attest[DICE_CDI_SIZE];
	uint8_t current_cdi_seal[DICE_CDI_SIZE];
	uint8_t next_cdi_attest[DICE_CDI_SIZE];
	uint8_t next_cdi_seal[DICE_CDI_SIZE];

	/* Validate magic number */
	if (DiceCtx->magic != DICE_CTX_MAGIC) {
		printf("Dice: Invalid Magic: 0x%08x\n", DiceCtx->magic);
		return -EINVAL;
	}

	/* Check if we've reached the maximum number of certificates */
	if (DiceCtx->cert_max_count > 0 &&
	    DiceCtx->cert_count >= DiceCtx->cert_max_count) {
		printf("Dice: Maximum number of certificates reached: %d\n",
		       DiceCtx->cert_count);
		return 0;
	}

	/*
	 * 1. Prepare DICE input values
	 */
	memset(&inputs, 0, sizeof(inputs));

	/* Code hash/descriptor */
	inputs.code_descriptor = flow->component_name;
	inputs.code_descriptor_size = strlen(flow->component_name) + 1;
	memcpy(inputs.code_hash, flow->code_hash, flow->code_hash_len);

	/* Config descriptor */
	config_descriptor.configs = 0;
	config_descriptor.configs |= DICE_ANDROID_CONFIG_COMPONENT_NAME;
	config_descriptor.configs |= DICE_ANDROID_CONFIG_COMPONENT_VERSION;
	config_descriptor.configs |= DICE_ANDROID_CONFIG_SECURITY_VERSION;
	config_descriptor.component_name = flow->component_name;
	config_descriptor.component_version = flow->component_version;
	config_descriptor.security_version = 1;

	inputs.config_descriptor = (void *)&config_descriptor;
	inputs.config_descriptor_size = sizeof(config_descriptor);
	inputs.config_type = kDiceConfigTypeDescriptor;

	/* Authority descriptor and hash */
	inputs.authority_descriptor = authority_descriptor;
	inputs.authority_descriptor_size = sizeof(authority_descriptor);
	result = DiceHash(DiceCtx, authority_descriptor,
			  sizeof(authority_descriptor), inputs.authority_hash);
	if (result != kDiceResultOk) {
		printf("Dice: authority hash failed: %d\n", result);
		return result;
	}

	/* Mode and hidden values */
	inputs.mode = kDiceModeNormal;

#if DICE_DEBUG
	dice_dump_inputs(DiceCtx, &inputs);
#endif
	/*
	 * 2. Generate/Extend BCC using Android DICE flow
	 */
	memcpy(current_cdi_attest, DiceCtx->next_cdi_attest, DICE_CDI_SIZE);
	memcpy(current_cdi_seal, DiceCtx->next_cdi_seal, DICE_CDI_SIZE);

	if (DiceCtx->cert_count == 0) {
		/*
		 * First certificate: Create new BCC with DiceAndroidMainFlowWithNewDiceChain
		 * This creates: [COSE_Key, COSE_Sign1]
		 */
		debug("Dice: Creating first certificate in BCC...\n");

		result = DiceAndroidMainFlowWithNewDiceChain(
			DiceCtx,
			current_cdi_attest,
			current_cdi_seal,
			&inputs,
			sizeof(bcc_buffer),
			bcc_buffer,
			&actual_size,
			next_cdi_attest,
			next_cdi_seal);

		if (result != kDiceResultOk) {
			printf("Dice: First BCC generation failed: %d\n", result);
			return result;
		}

		/* Copy BCC to cert_chain */
		memcpy(DiceCtx->cert_chain, bcc_buffer, actual_size);
		DiceCtx->cert_chain_size = actual_size;
		DiceCtx->cert_count = 1;

	} else {
		/*
		 * Subsequent certificates: Extend BCC with DiceAndroidMainFlow
		 * This extends: [COSE_Key, COSE_Sign1, ..., COSE_Sign1]
		 */
		debug("Dice: Extending BCC with certificate %d...\n",
		      DiceCtx->cert_count + 1);

		result = DiceAndroidMainFlow(
			DiceCtx,
			current_cdi_attest,
			current_cdi_seal,
			DiceCtx->cert_chain,         // Current BCC
			DiceCtx->cert_chain_size,    // Current BCC size
			&inputs,
			sizeof(bcc_buffer),
			bcc_buffer,
			&actual_size,
			next_cdi_attest,
			next_cdi_seal);

		if (result != kDiceResultOk) {
			printf("Dice: BCC extension failed: %d\n", result);
			return result;
		}

		/* Copy updated BCC back to cert_chain */
		memcpy(DiceCtx->cert_chain, bcc_buffer, actual_size);
		DiceCtx->cert_chain_size = actual_size;
		DiceCtx->cert_count++;
	}

	/* Update CDI values for next stage */
	memcpy(DiceCtx->next_cdi_attest, next_cdi_attest, DICE_CDI_SIZE);
	memcpy(DiceCtx->next_cdi_seal, next_cdi_seal, DICE_CDI_SIZE);

	/* Clear sensitive data */
	memset(current_cdi_attest, 0, DICE_CDI_SIZE);
	memset(current_cdi_seal, 0, DICE_CDI_SIZE);
	memset(next_cdi_attest, 0, DICE_CDI_SIZE);
	memset(next_cdi_seal, 0, DICE_CDI_SIZE);

	/* Validate BCC size */
	if (DiceCtx->cert_chain_size > DICE_BUF_SIZE) {
		printf("Dice: BCC size overflow: %u > 0x%x\n",
		       DiceCtx->cert_chain_size, DICE_BUF_SIZE);
		return -EINVAL;
	}
#if DICE_DEBUG
	dice_dump_bcc_chain(DiceCtx);
#endif
	return 0;
}

static int dice_mask_uds(void)
{
#if !DICE_STATIC_BROM_UDS
	struct otp_param param;
	struct udevice *dev;
	int ret;

	dev = misc_otp_get_device(OTP_S);
	if (!dev) {
		printf("DICE: No secure otp\n");
		return -ENODEV;
	}

	param.offset = OTP_DICE_UDS_ADDR;
	param.size = OTP_DICE_UDS_SIZE;
	param.flags = OTP_FLG_READ_MASK | OTP_FLG_PROG_MASK;
	ret = misc_otp_ioctl(dev, IOCTL_REQ_MASK, &param);
	if (ret) {
		printf("DICE: Can't mask otp UDS, ret=%d\n", ret);
		return ret;
	}
#endif
	return 0;
}

static int dice_read_uds(u8 *buffer)
{
#if DICE_STATIC_BROM_UDS
	const uint8_t static_brom_uds[DICE_CDI_SIZE] = {
	    0x00, 0x01, 0x02, 0x03,
	    0x04, 0x05, 0x06, 0x07,
	    0x08, 0x09, 0x0A, 0x0B,
	    0x0C, 0x0D, 0x0E, 0x0F,
	    0x10, 0x11, 0x12, 0x13,
	    0x14, 0x15, 0x16, 0x17,
	    0x18, 0x19, 0x1A, 0x1B,
	    0x1C, 0x1D, 0x1E, 0x1F
	};

	memcpy(buffer, static_brom_uds, DICE_CDI_SIZE);
#else
	struct udevice *dev;
	int ret;

	dev = misc_otp_get_device(OTP_S);
	if (!dev) {
		printf("DICE: No secure otp\n");
		return -ENODEV;
	}

	ret = misc_otp_read(dev, OTP_DICE_UDS_ADDR, buffer, DICE_CDI_SIZE);
	if (ret) {
		printf("DICE: Can't read otp UDS, ret=%d\n", ret);
		return -EIO;
	}
#if DICE_DEBUG
	print_hex_dump("\nOTP UDS: ", DUMP_PREFIX_ADDRESS, 32, 1,
		       buffer, DICE_CDI_SIZE, true);
#endif
#endif
	return 0;
}

int dice_start(void)
{
	printf("DICE: 0x%08lx - 0x%08lx\n",
	       (ulong)DICE_BUF_ADDR, (ulong)DICE_BUF_ADDR + DICE_BUF_SIZE);
	memset((void *)DICE_BUF_ADDR, 0, DICE_BUF_SIZE);

	return 0;
}

int dice_finish(void)
{
	struct DiceContext *DiceCtx_km = (void *)DICE_BUF_ADDR;
	uint32_t ret;

	printf("DICE(%s): 0x%08lx - 0x%08lx, cert_count=%d\n",
		DiceCtx_km->profile_name,
		(ulong)DiceCtx_km->cert_chain,
		(ulong)DiceCtx_km->cert_chain + DiceCtx_km->cert_chain_size,
		DiceCtx_km->cert_count);
#if DICE_DEBUG
	dice_dump_context(DiceCtx_km);
#endif
	ret = optee_set_dice_data(RK_DICE_UDS_PUB,
				  DiceCtx_km->uds_pubkey,
				  DiceCtx_km->uds_pubkey_size);
	if (ret) {
		printf("DICE: Send uds pubkey failed: %d\n", ret);
		return ret;
	}

	ret = optee_set_dice_data(RK_DICE_KM_ED25519_PRI,
				  DiceCtx_km->last_subject_privkey,
				  DiceCtx_km->last_subject_privkey_size);
	if (ret) {
		printf("DICE: Send KM last subject private key failed: %d\n", ret);
		return ret;
	}

	ret = optee_set_dice_data(RK_DICE_KM_ED25519_PUB,
				  DiceCtx_km->last_subject_pubkey,
				  DiceCtx_km->last_subject_pubkey_size);
	if (ret) {
		printf("DICE: Send KM last subject public key failed: %d\n", ret);
		return ret;
	}

	ret = optee_set_dice_data(RK_DICE_KM_CERTCHAIN,
				  DiceCtx_km->cert_chain,
				  DiceCtx_km->cert_chain_size);
	if (ret) {
		printf("DICE: Send KM BCC failed: %d\n", ret);
		return ret;
	}

#ifdef CONFIG_DICE_WIDEVINE
	struct DiceContext *DiceCtx_wv =
			(void *)DICE_BUF_ADDR + DICE_BUF_SIZE / DICE_CNT;

	printf("DICE(%s): 0x%08lx - 0x%08lx, cert_count=%d\n",
		DiceCtx_wv->profile_name,
		(ulong)DiceCtx_wv->cert_chain,
		(ulong)DiceCtx_wv->cert_chain + DiceCtx_wv->cert_chain_size,
		DiceCtx_wv->cert_count);
#if DICE_DEBUG
	dice_dump_context(DiceCtx_wv);
#endif
	ret = optee_set_dice_data(RK_DICE_WV_ED25519_PRI,
				  DiceCtx_wv->last_subject_privkey,
				  DiceCtx_wv->last_subject_privkey_size);
	if (ret) {
		printf("DICE: Send WV last subject private key failed: %d\n", ret);
		return ret;
	}

	ret = optee_set_dice_data(RK_DICE_WV_ED25519_PUB,
				  DiceCtx_wv->last_subject_pubkey,
				  DiceCtx_wv->last_subject_pubkey_size);
	if (ret) {
		printf("DICE: Send WV last subject public key failed: %d\n", ret);
		return ret;
	}

	ret = optee_set_dice_data(RK_DICE_WV_CERTCHAIN,
				  DiceCtx_wv->cert_chain,
				  DiceCtx_wv->cert_chain_size);
	if (ret) {
		printf("DICE: Send WV BCC failed: %d\n", ret);
		return ret;
	}

#endif
#ifndef CONFIG_SPL_BUILD
	uint8_t written = 0;
	char buf[32];

	ret = optee_oem_dice_uds_is_written(&written);
	if (ret)
		printf("DICE: Can't get UDS written status, ret=%d\n", ret);

	/* cmdline to kernel */
	snprintf(buf, sizeof(buf), "androidboot.uds_written=%d", written);
	env_update("bootargs", buf);
	printf("DICE: UDS is%s written\n", written ? "" : " not");
#endif
	return 0;
}

static void dice_parse_profile_versions(const char *cmdline,
					const char *needle,
					uint8_t *android_version,
					uint8_t *widevine_version)
{
	const char *profile;
	char *endp;
	ulong version;

	*android_version = 0;
	*widevine_version = 0;

	profile = strstr(cmdline, needle);
	if (!profile)
		return;

	profile += strlen(needle);
	version = simple_strtoul(profile, &endp, 10);
	if (endp == profile || version > U8_MAX)
		return;

	*android_version = version;
#ifdef CONFIG_DICE_WIDEVINE
	if (*endp != ',')
		return;

	profile = endp + 1;
	version = simple_strtoul(profile, &endp, 10);
	if (endp == profile || version > U8_MAX)
		return;

	*widevine_version = version;
#endif
}

static int dice_set_profile_name(struct DiceContext *DiceCtx, int i)
{
	const char *needle = "dice_profile=";
	const char *dice_profile_hdr[] = {
		"android.",
#ifdef CONFIG_DICE_WIDEVINE
		"widevine.",
#endif
	};
	struct disk_partition part;
	struct blk_desc *desc;
	struct vendor_boot_img_hdr_v34 *vboot_hdr = NULL;
	struct andr_img_hdr *hdr = NULL;
	char *boot_cmdline = NULL;
	const char *cmdline = NULL;
	uint8_t android_version;
	uint8_t widevine_version;
	lbaint_t blkcnt;
	int ret = 0;

	if (i >= ARRAY_SIZE(dice_profile_hdr))
		return -EINVAL;

#ifdef CONFIG_SPL_BUILD
	struct spl_load_info *info;

	info = glb_spl_load_info();
	desc = info ? info->priv : NULL;
#else
	desc = plat_bootdev();
#endif
	if (!desc) {
		printf("DICE: no dev desc found\n");
		return -ENODEV;
	}

	/*
	 * Relation: DICE=y base on GKI=y (while GKI=y not require DICE=y)
	 *
	 * Compatible:
	 *	Try to get cmdline from vendor boot partition(GKI=y), fallback
	 *	to get from boot partition(GKI=n) if failed.
	 */
	if (part_get_info_by_name(desc, PART_VENDOR_BOOT, &part) > 0) {
		blkcnt = DIV_ROUND_UP(sizeof(*vboot_hdr), desc->blksz);
		vboot_hdr = memalign(ARCH_DMA_MINALIGN, blkcnt * desc->blksz);
		if (!vboot_hdr)
			return -ENOMEM;
		if (blk_dread(desc, part.start, blkcnt, vboot_hdr) == blkcnt &&
		    !memcmp(vboot_hdr->magic, VENDOR_BOOT_MAGIC,
			    VENDOR_BOOT_MAGIC_SIZE)) {
			cmdline = (const char *)vboot_hdr->cmdline;
		}
	}

	/* fallback to boot.img when GKI=n (header v2) */
	if (!cmdline && part_get_info_by_name(desc, PART_BOOT, &part) > 0) {
		blkcnt = DIV_ROUND_UP(sizeof(*hdr), desc->blksz);
		hdr = memalign(ARCH_DMA_MINALIGN, blkcnt * desc->blksz);
		if (!hdr) {
			ret = -ENOMEM;
			goto out;
		}
		if (blk_dread(desc, part.start, blkcnt, hdr) == blkcnt &&
		    !memcmp(hdr->magic, ANDR_BOOT_MAGIC, ANDR_BOOT_MAGIC_SIZE)) {
			size_t cmdline_len = strlen(hdr->cmdline);
			size_t extra_len = strlen(hdr->extra_cmdline);

			boot_cmdline = malloc(cmdline_len + extra_len + 1);
			if (!boot_cmdline) {
				ret = -ENOMEM;
				goto out;
			}

			strcpy(boot_cmdline, hdr->cmdline);
			strcpy(boot_cmdline + cmdline_len, hdr->extra_cmdline);
			cmdline = boot_cmdline;
		}
	}
	if (!cmdline) {
		printf("Dice: Can't find boot and vendor_boot cmdline\n");
		ret = -EINVAL;
		goto out;
	}

#ifdef DICE_STATIC_PROFILE
	cmdline = DICE_STATIC_PROFILE;
#endif
	debug("Dice: %s cmdline: %s\n", part.name, cmdline);
	if (!strstr(cmdline, needle)) {
		printf("Dice: Can't find '%s' in %s cmdline !\n", needle, part.name);
		printf("Dice: %s cmdline: %s\n", part.name, cmdline);
		ret = -EINVAL;
		goto out;
	}

	dice_parse_profile_versions(cmdline, needle,
				    &android_version, &widevine_version);
	if (i == 0) {
		if (android_version == 0) {
			printf("Dice: invalid android profile version(%d)\n",
			       android_version);
			ret = -EINVAL;
			goto out;
		}
		DiceCtx->profile_version = android_version;
	}
#ifdef CONFIG_DICE_WIDEVINE
	else {
		if (widevine_version == 0) {
			printf("Dice: invalid widevine profile version(%d)\n",
			       widevine_version);
			ret = -EINVAL;
			goto out;
		}
		DiceCtx->profile_version = widevine_version;
	}
#endif
	snprintf(DiceCtx->profile_name, sizeof(DiceCtx->profile_name), "%s%d",
		 dice_profile_hdr[i], DiceCtx->profile_version);
	printf("DICE: %s\n", DiceCtx->profile_name);

out:
	free(boot_cmdline);
	free(vboot_hdr);
	free(hdr);
	return ret;
}

int dice_measure(const char *name, uint8_t *code_hash, int code_hash_len)
{
	struct DiceContext *DiceCtx[DICE_CNT];
	struct DiceFlow DiceFlow[DICE_CNT];
	uint8_t brom_uds[DICE_CDI_SIZE];
	int clear_uds = 0;
	int valid_otp_uds = 0;
	int i, err = 0;

#if DICE_DEBUG
	printf("dice_measure: %s\n", name);
#endif
	DiceCtx[0] = (void *)DICE_BUF_ADDR;
#ifdef CONFIG_DICE_WIDEVINE
	DiceCtx[1] = (void *)DICE_BUF_ADDR + DICE_BUF_SIZE / DICE_CNT;
#endif
	/* Read UDS only once ! */
	if (DiceCtx[0]->cert_chain_size == 0) {
		err = dice_read_uds(brom_uds);
		if (err)
			return err;

		/* non-zero ? */
		for (i = 0; i < DICE_CDI_SIZE; i++) {
			if (brom_uds[i] != 0) {
				valid_otp_uds = 1;
				break;
			}
		}
		printf("DICE: UDS is%s written\n", valid_otp_uds ? "" : " not");

		clear_uds = 1;
		if (valid_otp_uds) {
			err = dice_mask_uds();
			if (err) {
				printf("DICE: mask uds failed, ret=%d\n", err);
				goto out;
			}
		}
	}

	for (i = 0; i < ARRAY_SIZE(DiceCtx); i++) {
		/*
		 * 1. DICE initialization (first time only)
		 */
		if (DiceCtx[i]->cert_chain_size == 0) {
			debug("DICE: Initializing BCC...\n");

			DiceCtx[i]->magic = DICE_CTX_MAGIC;
			DiceCtx[i]->cert_chain = (void *)DiceCtx[i] + DICE_CTX_HDR_SIZE;
			DiceCtx[i]->cert_max_count = DICE_CERT_MAX;
			err = dice_set_profile_name(DiceCtx[i], i);
			if (err) {
				printf("DICE: set profile name failed, ret=%d\n", err);
				goto out;
			}
			memcpy(DiceCtx[i]->next_cdi_attest, brom_uds, DICE_CDI_SIZE);
			memcpy(DiceCtx[i]->next_cdi_seal, brom_uds, DICE_CDI_SIZE);
		}

		/*
		 * 2. DICE main flow
		 */
		DiceFlow[i].component_name = name;
		DiceFlow[i].component_version = 1;
		DiceFlow[i].code_hash = code_hash;
		DiceFlow[i].code_hash_len = code_hash_len;
#ifdef CONFIG_DICE_WIDEVINE
		/*
		 * FIXUP: Last stage component must use a fixed info.
		 *
		 * it leads a different 'DiceCtx->last_subject_{private,public}_key'
		 * from android flow.
		 */
		if (i == 1 && !strcmp(name, "kernel")) {
			DiceFlow[i].component_name = "Widevine";
			DiceFlow[i].component_version = DiceCtx[i]->profile_version;
		}
#endif
		err = dice_measure_component(DiceCtx[i], &DiceFlow[i]);
		if (err) {
			printf("DICE: measure failed, ret=%d\n", err);
			goto out;
		}
	}
out:
	if (clear_uds)
		memset(brom_uds, 0, DICE_CDI_SIZE);

	return err;
}
