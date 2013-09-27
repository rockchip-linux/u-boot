LOCAL_PATH := logos/charge

define all-images-under
$(shell find $(LOCAL_PATH)/$(1) -name "*.bmp" \
	-or -name "*.png" -and -not -name ".*"|sort)
endef

FAIL_IMAGE := \
	$(LOCAL_PATH)/images/battery_fail.png

LEVEL_0_IMAGES := \
	$(LOCAL_PATH)/images/battery_0.png

LEVEL_1_IMAGES := \
	$(LOCAL_PATH)/images/battery_1.png

LEVEL_2_IMAGES := \
	$(LOCAL_PATH)/images/battery_2.png

LEVEL_3_IMAGES := \
	$(LOCAL_PATH)/images/battery_3.png

LEVEL_4_IMAGES := \
	$(LOCAL_PATH)/images/battery_4.png

LEVEL_5_IMAGES := \
	$(LOCAL_PATH)/images/battery_5.png

LEVEL_0 := 19 #max battery cap percent of this level.
LEVEL_1 := 39
LEVEL_2 := 59
LEVEL_3 := 79
LEVEL_4 := 99
LEVEL_5 := 100

LEVEL_OPT := -level=
BMP_IMAGE_OPT := \
	$(call convert-image-path, $(FAIL_IMAGE)) \
	$(LEVEL_OPT)$(LEVEL_0) \
	$(call convert-image-path, $(LEVEL_0_IMAGES)) \
	$(LEVEL_OPT)$(LEVEL_1) \
	$(call convert-image-path, $(LEVEL_1_IMAGES)) \
	$(LEVEL_OPT)$(LEVEL_2) \
	$(call convert-image-path, $(LEVEL_2_IMAGES)) \
	$(LEVEL_OPT)$(LEVEL_3) \
	$(call convert-image-path, $(LEVEL_3_IMAGES)) \
	$(LEVEL_OPT)$(LEVEL_4) \
	$(call convert-image-path, $(LEVEL_4_IMAGES)) \
	$(LEVEL_OPT)$(LEVEL_5) \
	$(call convert-image-path, $(LEVEL_5_IMAGES)) \

BMP_IMAGES := \
    $(FAIL_IMAGE) \
    $(LEVEL_0_IMAGES) \
    $(LEVEL_1_IMAGES) \
    $(LEVEL_2_IMAGES) \
    $(LEVEL_3_IMAGES) \
    $(LEVEL_4_IMAGES) \
    $(LEVEL_5_IMAGES) \

# Generated bmp image
BMP_IMAGE_DATA_H = $(OBJTREE)/include/bmp_image_data.h
LOGO-$(CONFIG_CMD_CHARGE_ANIM) += $(BMP_IMAGE_DATA_H)

$(obj)bmp_image$(SFX):   $(obj)bmp_image.o
	$(HOSTCC) $(HOSTCFLAGS) $(HOSTLDFLAGS) -o $@ $^
	$(HOSTSTRIP) $@

BMP_IMAGE_IMG= $(OBJTREE)/charge.img
ALL-$(CONFIG_CMD_CHARGE_ANIM) += $(BMP_IMAGE_IMG)
$(BMP_IMAGE_IMG): $(BMP_IMAGE_DATA_H)

$(foreach v, $(BMP_IMAGES), \
	$(eval src_image := $(v)) \
	$(eval dst_image := $(call convert-one-image-path, $(v))) \
	$(eval include $(SRCTREE)/tools/logos/charge/build_image.mk) \
)

$(BMP_IMAGE_DATA_H): \
	$(obj)bmp_image \
	$(SRCTREE)/tools/logos/charge/charge.mk \
	$(call convert-image-path, $(BMP_IMAGES))
	$(obj)./bmp_image $(BMP_IMAGE_IMG) $(BMP_IMAGE_OPT) > $@

