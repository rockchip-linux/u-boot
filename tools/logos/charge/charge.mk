LOCAL_PATH := logos/charge

#convert inputfile -colors 256 -compress rle output.bmp

define all-images-under
    $(call convert-image, \
        $(shell find $(LOCAL_PATH)/$(1) -name "*.bmp" \
            -or -name "*.png" -and -not -name ".*"|sort))
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
BMP_IMAGES := \
	$(call convert-image, $(FAIL_IMAGE)) \
	$(LEVEL_OPT)$(LEVEL_0) \
	$(call convert-image, $(LEVEL_0_IMAGES)) \
	$(LEVEL_OPT)$(LEVEL_1) \
	$(call convert-image, $(LEVEL_1_IMAGES)) \
	$(LEVEL_OPT)$(LEVEL_2) \
	$(call convert-image, $(LEVEL_2_IMAGES)) \
	$(LEVEL_OPT)$(LEVEL_3) \
	$(call convert-image, $(LEVEL_3_IMAGES)) \
	$(LEVEL_OPT)$(LEVEL_4) \
	$(call convert-image, $(LEVEL_4_IMAGES)) \
	$(LEVEL_OPT)$(LEVEL_5) \
	$(call convert-image, $(LEVEL_5_IMAGES)) \

$(warning $(BMP_IMAGES))

.PHONY : \
$(LEVEL_OPT)$(LEVEL_0) \
$(LEVEL_OPT)$(LEVEL_1) \
$(LEVEL_OPT)$(LEVEL_2) \
$(LEVEL_OPT)$(LEVEL_3) \
$(LEVEL_OPT)$(LEVEL_4) \
$(LEVEL_OPT)$(LEVEL_5) \
