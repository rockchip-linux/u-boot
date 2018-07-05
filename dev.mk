export RELEASE ?= 1

all:

include dev-ayufan/boards.mk
include dev-ayufan/build.mk
include dev-ayufan/images.mk
include dev-ayufan/package.mk
include dev-ayufan/rockchip.mk

RELEASE_NAME ?= $(shell $(UBOOT_MAKE) -s ubootrelease)

.PHONY: .scmversion
.scmversion:
	@echo "-rockchip-ayufan-$(RELEASE)-g$$(git rev-parse --short HEAD)" > .scmversion

version: .scmversion
	@echo $(RELEASE_NAME)

$(filter tmp/rkbin/%, $(BL31) $(UBOOT_TPL) $(UBOOT_SPL) $(LOADER_BIN)):
	mkdir -p $$(dirname "$@")
	curl --fail -L https://github.com/ayufan-rock64/rkbin/raw/master/$(subst tmp/rkbin/,,$@) > $@.tmp
	mv $@.tmp $@
