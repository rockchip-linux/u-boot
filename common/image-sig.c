/*
 * Copyright (c) 2013, Google Inc.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#ifdef USE_HOSTCC
#include "mkimage.h"
#include <time.h>
#else
#include <common.h>
#include <malloc.h>
#include <optee_include/OpteeClientInterface.h>
DECLARE_GLOBAL_DATA_PTR;
#endif /* !USE_HOSTCC*/
#include <image.h>
#include <u-boot/rsa.h>
#include <u-boot/rsa-checksum.h>

#define IMAGE_MAX_HASHED_NODES		100
#define FIT_MAX_HASH_PATH_BUF		4096

#ifdef USE_HOSTCC
void *host_blob;
void image_set_host_blob(void *blob)
{
	host_blob = blob;
}
void *image_get_host_blob(void)
{
	return host_blob;
}
#endif

static const uint8_t sha1_der_prefix_data[SHA1_DER_LEN] = {
	0x30, 0x21, 0x30, 0x09, 0x06, 0x05, 0x2b, 0x0e,
	0x03, 0x02, 0x1a, 0x05, 0x00, 0x04, 0x14
};

static const uint8_t sha256_der_prefix_data[SHA256_DER_LEN] = {
	0x30, 0x31, 0x30, 0x0d, 0x06, 0x09, 0x60, 0x86,
	0x48, 0x01, 0x65, 0x03, 0x04, 0x02, 0x01, 0x05,
	0x00, 0x04, 0x20
};

struct checksum_algo checksum_algos[] = {
	{
		.name = "sha1",
		.checksum_len = SHA1_SUM_LEN,
		.der_len = SHA1_DER_LEN,
		.der_prefix = sha1_der_prefix_data,
#if IMAGE_ENABLE_SIGN
		.calculate_sign = EVP_sha1,
#endif
		.calculate = hash_calculate,
	},
	{
		.name = "sha256",
		.checksum_len = SHA256_SUM_LEN,
		.der_len = SHA256_DER_LEN,
		.der_prefix = sha256_der_prefix_data,
#if IMAGE_ENABLE_SIGN
		.calculate_sign = EVP_sha256,
#endif
		.calculate = hash_calculate,
	}

};

struct crypto_algo crypto_algos[] = {
	{
		.name = "rsa2048",
		.key_len = RSA2048_BYTES,
		.sign = rsa_sign,
		.add_verify_data = rsa_add_verify_data,
		.verify = rsa_verify,
	},
	{
		.name = "rsa4096",
		.key_len = RSA4096_BYTES,
		.sign = rsa_sign,
		.add_verify_data = rsa_add_verify_data,
		.verify = rsa_verify,
	}

};

struct padding_algo padding_algos[] = {
	{
		.name = "pkcs-1.5",
		.verify = padding_pkcs_15_verify,
	},
#ifdef CONFIG_FIT_ENABLE_RSASSA_PSS_SUPPORT
	{
		.name = "pss",
		.verify = padding_pss_verify,
	}
#endif /* CONFIG_FIT_ENABLE_RSASSA_PSS_SUPPORT */
};

struct checksum_algo *image_get_checksum_algo(const char *full_name)
{
	int i;
	const char *name;

	for (i = 0; i < ARRAY_SIZE(checksum_algos); i++) {
		name = checksum_algos[i].name;
		/* Make sure names match and next char is a comma */
		if (!strncmp(name, full_name, strlen(name)) &&
		    full_name[strlen(name)] == ',')
			return &checksum_algos[i];
	}

	return NULL;
}

struct crypto_algo *image_get_crypto_algo(const char *full_name)
{
	int i;
	const char *name;

	/* Move name to after the comma */
	name = strchr(full_name, ',');
	if (!name)
		return NULL;
	name += 1;

	for (i = 0; i < ARRAY_SIZE(crypto_algos); i++) {
		if (!strcmp(crypto_algos[i].name, name))
			return &crypto_algos[i];
	}

	return NULL;
}

struct padding_algo *image_get_padding_algo(const char *name)
{
	int i;

	if (!name)
		return NULL;

	for (i = 0; i < ARRAY_SIZE(padding_algos); i++) {
		if (!strcmp(padding_algos[i].name, name))
			return &padding_algos[i];
	}

	return NULL;
}

/**
 * fit_region_make_list() - Make a list of image regions
 *
 * Given a list of fdt_regions, create a list of image_regions. This is a
 * simple conversion routine since the FDT and image code use different
 * structures.
 *
 * @fit: FIT image
 * @fdt_regions: Pointer to FDT regions
 * @count: Number of FDT regions
 * @region: Pointer to image regions, which must hold @count records. If
 * region is NULL, then (except for an SPL build) the array will be
 * allocated.
 * @return: Pointer to image regions
 */
struct image_region *fit_region_make_list(const void *fit,
		struct fdt_region *fdt_regions, int count,
		struct image_region *region)
{
	int i;

	debug("Hash regions:\n");
	debug("%10s %10s\n", "Offset", "Size");

	/*
	 * Use malloc() except in SPL (to save code size). In SPL the caller
	 * must allocate the array.
	 */
#ifndef CONFIG_SPL_BUILD
	if (!region)
		region = calloc(sizeof(*region), count);
#endif
	if (!region)
		return NULL;
	for (i = 0; i < count; i++) {
		debug("%10x %10x\n", fdt_regions[i].offset,
		      fdt_regions[i].size);
		region[i].data = fit + fdt_regions[i].offset;
		region[i].size = fdt_regions[i].size;
	}

	return region;
}

static int fit_image_setup_verify(struct image_sign_info *info,
		const void *fit, int noffset, int required_keynode,
		char **err_msgp)
{
	char *algo_name;
	const char *padding_name;

	if (fdt_totalsize(fit) > CONFIG_FIT_SIGNATURE_MAX_SIZE) {
		*err_msgp = "Total size too large";
		return 1;
	}

	if (fit_image_hash_get_algo(fit, noffset, &algo_name)) {
		*err_msgp = "Can't get hash algo property";
		return -1;
	}

	padding_name = fdt_getprop(fit, noffset, "padding", NULL);
	if (!padding_name)
		padding_name = RSA_DEFAULT_PADDING_NAME;

	memset(info, '\0', sizeof(*info));
	info->keyname = fdt_getprop(fit, noffset, "key-name-hint", NULL);
	info->fit = (void *)fit;
	info->node_offset = noffset;
	info->name = algo_name;
	info->checksum = image_get_checksum_algo(algo_name);
	info->crypto = image_get_crypto_algo(algo_name);
	info->padding = image_get_padding_algo(padding_name);
	info->fdt_blob = gd_fdt_blob();
	info->required_keynode = required_keynode;
	printf("%s:%s", algo_name, info->keyname);

	if (!info->checksum || !info->crypto) {
		*err_msgp = "Unknown signature algorithm";
		return -1;
	}

	return 0;
}

int fit_image_check_sig(const void *fit, int noffset, const void *data,
		size_t size, int required_keynode, char **err_msgp)
{
	struct image_sign_info info;
	struct image_region region;
	uint8_t *fit_value;
	int fit_value_len;

	*err_msgp = NULL;
	if (fit_image_setup_verify(&info, fit, noffset, required_keynode,
				   err_msgp))
		return -1;

	if (fit_image_hash_get_value(fit, noffset, &fit_value,
				     &fit_value_len)) {
		*err_msgp = "Can't get hash value property";
		return -1;
	}

	region.data = data;
	region.size = size;

	if (info.crypto->verify(&info, &region, 1, fit_value, fit_value_len)) {
		*err_msgp = "Verification failed";
		return -1;
	}

	return 0;
}

static int fit_image_verify_sig(const void *fit, int image_noffset,
		const char *data, size_t size, const void *sig_blob,
		int sig_offset)
{
	int noffset;
	char *err_msg = "";
	int verified = 0;
	int ret;

	/* Process all hash subnodes of the component image node */
	fdt_for_each_subnode(noffset, fit, image_noffset) {
		const char *name = fit_get_name(fit, noffset, NULL);

		/*
		 * We don't support this since libfdt considers names with the
		 * name root but different @ suffix to be equal
		 */
		if (strchr(name, '@')) {
			err_msg = "Node name contains @";
			goto error;
		}
		if (!strncmp(name, FIT_SIG_NODENAME,
			     strlen(FIT_SIG_NODENAME))) {
			ret = fit_image_check_sig(fit, noffset, data,
							size, -1, &err_msg);
			if (ret) {
				puts("- ");
			} else {
				puts("+ ");
				verified = 1;
				break;
			}
		}
	}

	if (noffset == -FDT_ERR_TRUNCATED || noffset == -FDT_ERR_BADSTRUCTURE) {
		err_msg = "Corrupted or truncated tree";
		goto error;
	}

	if (verified)
		return 0;

error:
	printf(" error!\n%s for '%s' hash node in '%s' image node\n",
	       err_msg, fit_get_name(fit, noffset, NULL),
	       fit_get_name(fit, image_noffset, NULL));
	return -1;
}

int fit_image_verify_required_sigs(const void *fit, int image_noffset,
		const char *data, size_t size, const void *sig_blob,
		int *no_sigsp)
{
	int verify_count = 0;
	int noffset;
	int sig_node;

	/* Work out what we need to verify */
	*no_sigsp = 1;
	sig_node = fdt_subnode_offset(sig_blob, 0, FIT_SIG_NODENAME);
	if (sig_node < 0) {
		printf("No RSA key found\n");
		return -EINVAL;
	}

	fdt_for_each_subnode(noffset, sig_blob, sig_node) {
		const char *required;
		int ret;

		required = fdt_getprop(sig_blob, noffset, "required", NULL);
		if (!required || strcmp(required, "image"))
			continue;
		ret = fit_image_verify_sig(fit, image_noffset, data, size,
					sig_blob, noffset);
		if (ret) {
			printf("Failed to verify required signature '%s'\n",
			       fit_get_name(sig_blob, noffset, NULL));
			return ret;
		}
		verify_count++;
	}

	if (verify_count)
		*no_sigsp = 0;

	return 0;
}

/**
 * fit_config_add_hash() - Add hash nodes for one image to the node list
 *
 * Adds the image path, all its hash-* subnode paths, and its cipher
 * subnode path (if present) to the packed buffer.
 *
 * @fit:		FIT blob
 * @image_noffset:	Image node offset (e.g. /images/kernel-1)
 * @node_inc:		Array of path pointers to fill
 * @count:		Pointer to current count (updated on return)
 * @max_nodes:		Maximum entries in @node_inc
 * @buf:		Buffer for packed path strings
 * @buf_used:		Pointer to bytes used in @buf (updated on return)
 * @buf_len:		Total size of @buf
 * Return: 0 on success, -ve on error
 */
static int fit_config_add_hash(const void *fit, int image_noffset,
			       char **node_inc, int *count, int max_nodes,
			       char *buf, int *buf_used, int buf_len)
{
	int noffset, hash_count, ret, len;

	if (*count >= max_nodes)
		return -ENOSPC;

	ret = fdt_get_path(fit, image_noffset, buf + *buf_used,
			   buf_len - *buf_used);
	if (ret < 0)
		return -ENOENT;
	len = strlen(buf + *buf_used) + 1;
	node_inc[(*count)++] = buf + *buf_used;
	*buf_used += len;

	/* Add all this image's hash subnodes */
	hash_count = 0;
	for (noffset = fdt_first_subnode(fit, image_noffset);
	     noffset >= 0;
	     noffset = fdt_next_subnode(fit, noffset)) {
		const char *name = fit_get_name(fit, noffset, NULL);

		if (strncmp(name, FIT_HASH_NODENAME,
			    strlen(FIT_HASH_NODENAME)))
			continue;
		if (*count >= max_nodes)
			return -ENOSPC;
		ret = fdt_get_path(fit, noffset, buf + *buf_used,
				   buf_len - *buf_used);
		if (ret < 0)
			return -ENOENT;
		len = strlen(buf + *buf_used) + 1;
		node_inc[(*count)++] = buf + *buf_used;
		*buf_used += len;
		hash_count++;
	}

	if (!hash_count) {
		printf("No hash nodes in image '%s'\n",
		       fdt_get_name(fit, image_noffset, NULL));
		return -ENOMSG;
	}

	/* Add this image's cipher node if present */
	noffset = fdt_subnode_offset(fit, image_noffset, FIT_CIPHER_NODENAME);
	if (noffset != -FDT_ERR_NOTFOUND) {
		if (noffset < 0)
			return -EIO;
		if (*count >= max_nodes)
			return -ENOSPC;
		ret = fdt_get_path(fit, noffset, buf + *buf_used,
				   buf_len - *buf_used);
		if (ret < 0)
			return -ENOENT;
		len = strlen(buf + *buf_used) + 1;
		node_inc[(*count)++] = buf + *buf_used;
		*buf_used += len;
	}

	return 0;
}

/**
 * fit_config_get_hash_list() - Build the list of nodes to hash
 *
 * Works through every image referenced by the configuration and collects the
 * node paths: root + config + all referenced images with their hash and
 * cipher subnodes.
 *
 * Properties known not to be image references (description, compatible,
 * default, load-only) are skipped, so any new image type is covered by default.
 *
 * @fit:	FIT blob
 * @conf_noffset: Configuration node offset
 * @node_inc:	Array to fill with path string pointers
 * @max_nodes:	Size of @node_inc array
 * @buf:	Buffer for packed null-terminated path strings
 * @buf_len:	Size of @buf
 * Return: number of entries in @node_inc, or -ve on error
 */
static int fit_config_get_hash_list(const void *fit, int conf_noffset,
				    char **node_inc, int max_nodes,
				    char *buf, int buf_len)
{
	const char *conf_name;
	int image_count;
	int prop_offset;
	int used = 0;
	int count = 0;
	int ret, len;

	conf_name = fit_get_name(fit, conf_noffset, NULL);

	/* Always include the root, configurations and configuration nodes */
	if (max_nodes < 3)
		return -ENOSPC;

	len = 2;  /* "/" + nul */
	if (len > buf_len)
		return -ENOSPC;
	strcpy(buf, "/");
	node_inc[count++] = buf;
	used += len;

	len = strlen(FIT_CONFS_PATH) + 1;
	if (used + len > buf_len)
		return -ENOSPC;
	strcpy(buf + used, FIT_CONFS_PATH);
	node_inc[count++] = buf + used;
	used += len;

	len = snprintf(buf + used, buf_len - used, "%s/%s", FIT_CONFS_PATH,
		       conf_name) + 1;
	if (used + len > buf_len)
		return -ENOSPC;
	node_inc[count++] = buf + used;
	used += len;

	/* Process each image referenced by the config */
	image_count = 0;
	fdt_for_each_property_offset(prop_offset, fit, conf_noffset) {
		const char *prop_name;
		int img_count, i;

		fdt_getprop_by_offset(fit, prop_offset, &prop_name, NULL);
		if (!prop_name)
			continue;

		/* Skip properties that are not image references */
		if (!strcmp(prop_name, FIT_DESC_PROP) ||
		    !strcmp(prop_name, FIT_COMPAT_PROP) ||
		    !strcmp(prop_name, FIT_DEFAULT_PROP))
			continue;

		img_count = fdt_stringlist_count(fit, conf_noffset, prop_name);
		for (i = 0; i < img_count; i++) {
			int noffset;

			noffset = fit_conf_get_prop_node_index(fit,
						       conf_noffset,
						       prop_name, i);
			if (noffset < 0)
				continue;

			ret = fit_config_add_hash(fit, noffset, node_inc,
						  &count, max_nodes, buf, &used,
						  buf_len);
			if (ret < 0)
				return ret;

			image_count++;
		}
	}

	if (!image_count) {
		printf("No images in config '%s'\n", conf_name);
		return -ENOMSG;
	}

	return count;
}

/**
 * fit_config_check_sig() - Check the signature of a config
 *
 * @fit: FIT to check
 * @noffset: Offset of configuration node (e.g. /configurations/conf-1)
 * @required_keynode:	Offset in the control FDT of the required key node,
 *			if any. If this is given, then the configuration wil not
 *			pass verification unless that key is used. If this is
 *			-1 then any signature will do.
 * @conf_noffset: Offset of the configuration subnode being checked (e.g.
 *	 /configurations/conf-1/kernel)
 * @err_msgp:		In the event of an error, this will be pointed to a
 *			help error string to display to the user.
 * @return 0 if all verified ok, <0 on error
 */
static int fit_config_check_sig(const void *fit, int noffset,
				int required_keynode, int conf_noffset,
				char **err_msgp)
{
	char * const exc_prop[] = {"data"};
	char *node_inc[IMAGE_MAX_HASHED_NODES];
	char hash_buf[FIT_MAX_HASH_PATH_BUF];
	struct image_sign_info info;
	const uint32_t *strings;
	uint8_t *fit_value;
	int fit_value_len;
	int max_regions;
	char path[200];
	int count;

	debug("%s: fdt=%p, conf='%s', sig='%s'\n", __func__, gd_fdt_blob(),
	      fit_get_name(fit, noffset, NULL),
	      fit_get_name(gd_fdt_blob(), required_keynode, NULL));
	*err_msgp = NULL;
	if (fit_image_setup_verify(&info, fit, noffset, required_keynode,
				   err_msgp))
		return -1;

	if (fit_image_hash_get_value(fit, noffset, &fit_value,
				     &fit_value_len)) {
		*err_msgp = "Can't get hash value property";
		return -1;
	}

	/* Build the node list from the config, ignoring hashed-nodes */
	count = fit_config_get_hash_list(fit, conf_noffset,
					 node_inc, IMAGE_MAX_HASHED_NODES,
					 hash_buf, sizeof(hash_buf));
	if (count < 0) {
		*err_msgp = "Failed to build hash node list";
		return -1;
	}

	/*
	 * Each node can generate one region for each sub-node. Allow for
	 * 7 sub-nodes (hash@1, signature@1, etc.) and some extra.
	 */
	max_regions = 20 + count * 7;
	struct fdt_region fdt_regions[max_regions];

	/* Get a list of regions to hash */
	count = fdt_find_regions(fit, node_inc, count,
			exc_prop, ARRAY_SIZE(exc_prop),
			fdt_regions, max_regions - 1,
			path, sizeof(path), 0);
	if (count < 0) {
		*err_msgp = "Failed to hash configuration";
		return -1;
	}
	if (count == 0) {
		*err_msgp = "No data to hash";
		return -1;
	}
	if (count >= max_regions - 1) {
		*err_msgp = "Too many hash regions";
		return -1;
	}

	/* Add the strings */
	strings = fdt_getprop(fit, noffset, "hashed-strings", NULL);
	if (strings) {
		/*
		 * The strings region offset must be a static 0x0.
		 * This is set in tool/image-host.c
		 */
		fdt_regions[count].offset = fdt_off_dt_strings(fit);
		fdt_regions[count].size = fdt32_to_cpu(strings[1]);
		count++;
	}

	/* Allocate the region list on the stack */
	struct image_region region[count];

	fit_region_make_list(fit, fdt_regions, count, region);
	if (info.crypto->verify(&info, region, count, fit_value,
				fit_value_len)) {
		*err_msgp = "Verification failed";
		return -1;
	}
	/* Get the secure flag here and write the secure data and the secure flag */
#if !defined(USE_HOSTCC)
#if defined(CONFIG_SPL_BUILD) && defined(CONFIG_SPL_FIT_HW_CRYPTO) && \
    defined(CONFIG_SPL_ROCKCHIP_SECURE_OTP)
	if (rsa_burn_key_hash(&info))
		return -1;
#if defined(CONFIG_SPL_OTP_DISABLE_SD) || defined(CONFIG_SPL_OTP_DISABLE_USB) || \
    defined(CONFIG_SPL_OTP_DISABLE_UART) || defined(CONFIG_SPL_OTP_DISABLE_SPI2APB)
	if (rsa_burn_disable_upgrade())
		return -1;
#endif
#endif
#endif

	return 0;
}

static int fit_config_verify_sig(const void *fit, int conf_noffset,
		const void *sig_blob, int sig_offset)
{
	int noffset;
	char *err_msg = "";
	int verified = 0;
	int ret;

	/* Process all hash subnodes of the component conf node */
	fdt_for_each_subnode(noffset, fit, conf_noffset) {
		const char *name = fit_get_name(fit, noffset, NULL);

		if (!strncmp(name, FIT_SIG_NODENAME,
			     strlen(FIT_SIG_NODENAME))) {
			ret = fit_config_check_sig(fit, noffset, sig_offset,
						   conf_noffset, &err_msg);
			if (ret) {
				puts("- ");
			} else {
				puts("+ ");
				verified = 1;
				break;
			}
		}
	}

	if (noffset == -FDT_ERR_TRUNCATED || noffset == -FDT_ERR_BADSTRUCTURE) {
		err_msg = "Corrupted or truncated tree";
		goto error;
	}

	if (verified)
		return 0;

error:
	printf(" error!\n%s for '%s' hash node in '%s' config node\n",
	       err_msg, fit_get_name(fit, noffset, NULL),
	       fit_get_name(fit, conf_noffset, NULL));
	return -1;
}

int fit_config_verify_required_sigs(const void *fit, int conf_noffset,
		const void *sig_blob)
{
	const char *name = fit_get_name(fit, conf_noffset, NULL);
	int noffset;
	int sig_node;

	/*
	 * We don't support this since libfdt considers names with the
	 * name root but different @ suffix to be equal
	 */
	if (strchr(name, '@')) {
		printf("Configuration node '%s' contains '@'\n", name);
		return -EPERM;
	}

	/* Work out what we need to verify */
	sig_node = fdt_subnode_offset(sig_blob, 0, FIT_SIG_NODENAME);
	if (sig_node < 0) {
		printf("No RSA key found\n");
		return -EINVAL;
	}

	fdt_for_each_subnode(noffset, sig_blob, sig_node) {
		const char *required;
		int ret;

		required = fdt_getprop(sig_blob, noffset, "required", NULL);
		if (!required || strcmp(required, "conf"))
			continue;
		ret = fit_config_verify_sig(fit, conf_noffset, sig_blob,
					    noffset);
		if (ret) {
			printf("Failed to verify required signature '%s'\n",
			       fit_get_name(sig_blob, noffset, NULL));
			return ret;
		}
	}

	return 0;
}

int fit_config_verify(const void *fit, int conf_noffset)
{
	return fit_config_verify_required_sigs(fit, conf_noffset,
					       gd_fdt_blob());
}

#ifndef USE_HOSTCC
#if CONFIG_IS_ENABLED(FIT_ROLLBACK_PROTECT)
__weak int fit_read_dev_rollback_index(uint32_t fit_index, uint32_t *dev_index)
{
	*dev_index = 0;

	return 0;
}
__weak int fit_rollback_index_verify(const void *fit, uint32_t rollback_fd,
				     uint32_t *this_index, uint32_t *min_index)
{
	*this_index = 0;
	*min_index = 0;

	return 0;
}
#endif
#endif
