.PHONY: loader-download-mode
loader-download-mode:
	rkdeveloptool db $(LOADER_BIN)
	sleep 1s

.PHONY: loader-boot		# boot loader over USB
loader-boot: $(UBOOT_LOADERS)
	make loader-download-mode
	rkdeveloptool rid
	dd if=/dev/zero of=$(UBOOT_OUTPUT_DIR)/clear.img count=1
	rkdeveloptool wl 64 $(UBOOT_OUTPUT_DIR)/clear.img
	rkdeveloptool wl 512 $(UBOOT_OUTPUT_DIR)/u-boot.itb

ifneq (rk3399,$(BOARD_CHIP))
	@echo Restart device and press ENTER
	@read XX
	sleep 3s
else
	rkdeveloptool rd
	sleep 1s
endif

ifneq (,$(USE_UBOOT_SPL))
	cat $(UBOOT_OUTPUT_DIR)/spl/u-boot-spl.bin | openssl rc4 -K 7c4e0304550509072d2c7b38170d1711 | rkflashtool l
else
ifneq (,$(USE_UBOOT_TPL))
	cat $(UBOOT_OUTPUT_DIR)/tpl/u-boot-tpl.bin | openssl rc4 -K 7c4e0304550509072d2c7b38170d1711 | rkflashtool l
else
	cat $(DDR) | openssl rc4 -K 7c4e0304550509072d2c7b38170d1711 | rkflashtool l
endif
ifneq (,$(USE_MINILOADER))
	cat $(MINILOADER_BIN) | openssl rc4 -K 7c4e0304550509072d2c7b38170d1711 | rkflashtool L
else
	cat $(UBOOT_OUTPUT_DIR)/spl/u-boot-spl.bin | openssl rc4 -K 7c4e0304550509072d2c7b38170d1711 | rkflashtool L
endif
endif

.PHONY: loader-flash		# flash loader to the device
loader-flash: $(UBOOT_OUTPUT_DIR)/idbloader.img
	make loader-download-mode
	sleep 1s
	rkdeveloptool rid
	rkdeveloptool wl 64 $<
	rkdeveloptool rd

.PHONY: loader-wipe		# clear loader
loader-wipe:
	dd if=/dev/zero of=$(UBOOT_OUTPUT_DIR)/clear.img count=1
	make loader-download-mode
	sleep 1s
	rkdeveloptool rid
	rkdeveloptool wl 64 $(UBOOT_OUTPUT_DIR)/clear.img
	rkdeveloptool rd
