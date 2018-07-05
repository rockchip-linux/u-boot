.PHONY: u-boot-package
u-boot-package: $(UBOOT_LOADERS)
	fpm -s dir -t deb -n u-boot-rockchip-$(BOARD_TARGET) -v $(RELEASE_NAME) \
		-p u-boot-rockchip-$(BOARD_TARGET)-$(RELEASE_NAME).deb \
		--deb-priority optional --category admin \
		--force \
		--depends debsums \
		--depends mtd-utils \
		--deb-compression bzip2 \
		--deb-field "Multi-Arch: foreign" \
		--deb-field "Conflicts: u-boot-$(BOARD_TARGET), u-boot-rockchip" \
		--deb-field "Provides: u-boot-virtual" \
		--after-install dev-ayufan/scripts/postinst.deb \
		--before-remove dev-ayufan/scripts/prerm.deb \
		--url https://gitlab.com/ayufan-rock64/linux-build \
		--description "Rock64 U-boot package" \
		-m "Kamil Trzciński <ayufan@ayufan.eu>" \
		--license "MIT" \
		--vendor "Kamil Trzciński" \
		-a all \
		dev-ayufan/root/=/ \
		$(addsuffix =/usr/lib/u-boot-$(BOARD_TARGET)/,$(UBOOT_LOADERS))

all: u-boot-package
