$(UBOOT_OUTPUT_DIR)/%/boot.scr: dev-ayufan/blobs/$(BOARD_TARGET)/%.cmd
	mkdir -p $$(dirname $@)
	mkimage -C none -A arm -T script -d $< $@

$(UBOOT_OUTPUT_DIR)/%/boot.img: $(UBOOT_OUTPUT_DIR)/%/boot.scr $(UBOOT_LOADERS)
	dd if=/dev/zero of=$@.tmp bs=1M count=32
	mkfs.vfat -n "u-boot-script" $@.tmp
	mcopy -sm -i $@.tmp $^ ::
	mv $@.tmp $@

u-boot-%-$(BOARD_TARGET).img: $(UBOOT_OUTPUT_DIR)/%/boot.img $(UBOOT_LOADERS)
	dd if=/dev/zero of=$@.tmp bs=1M count=16
	parted -s $@.tmp mklabel gpt
	parted -s $@.tmp unit s mkpart bootloader 64 8127
	parted -s $@.tmp unit s mkpart boot fat16 8192 100%
	parted -s $@.tmp set 2 legacy_boot on
	dd if=$(word 2,$^) of=$@.tmp conv=notrunc seek=64
	dd if=$(word 1,$^) of=$@.tmp conv=notrunc seek=8192
	mv "$@.tmp" $@

u-boot-%-$(BOARD_TARGET).img.xz: u-boot-%-$(BOARD_TARGET).img
	xz -f $<

.PHONY: u-boot-images
u-boot-images: $(addprefix u-boot-, $(addsuffix -$(BOARD_TARGET).img.xz, $(IMAGES)))

all: u-boot-images
