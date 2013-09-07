LOCAL_PATH := logos/charge

#convert inputfile -colors 256 -compress rle output.bmp

define all-images-under
    $(shell find $(LOCAL_PATH)/$(1) -name "*.bmp" -and -not -name ".*"|sort)
endef

FAIL_IMAGE := \
	$(LOCAL_PATH)/images/battery_fail.bmp

LEVEL_0_IMAGES := \
	$(LOCAL_PATH)/images/battery_0.bmp

LEVEL_1_IMAGES := \
	$(LOCAL_PATH)/images/battery_1.bmp

LEVEL_2_IMAGES := \
	$(LOCAL_PATH)/images/battery_2.bmp

LEVEL_3_IMAGES := \
	$(LOCAL_PATH)/images/battery_3.bmp

LEVEL_4_IMAGES := \
	$(LOCAL_PATH)/images/battery_4.bmp

LEVEL_4_IMAGES := \
	$(LOCAL_PATH)/images/battery_5.bmp

LEVEL_0 := 19 #max value to match this level.
LEVEL_1 := 39
LEVEL_2 := 59
LEVEL_3 := 79
LEVEL_4 := 99
LEVEL_5 := 100

LEVEL_OPT := -level=
BMP_IMAGES := \
	$(FAIL_IMAGE) \
	$(LEVEL_OPT)$(LEVEL_0) \
	$(LEVEL_0_IMAGES) \
	$(LEVEL_OPT)$(LEVEL_1) \
	$(LEVEL_1_IMAGES) \
	$(LEVEL_OPT)$(LEVEL_2) \
	$(LEVEL_2_IMAGES) \
	$(LEVEL_OPT)$(LEVEL_3) \
	$(LEVEL_3_IMAGES) \
	$(LEVEL_OPT)$(LEVEL_4) \
	$(LEVEL_4_IMAGES) \
	$(LEVEL_OPT)$(LEVEL_5) \
	$(LEVEL_5_IMAGES) \

.PHONY : \
$(LEVEL_OPT)$(LEVEL_0) \
$(LEVEL_OPT)$(LEVEL_1) \
$(LEVEL_OPT)$(LEVEL_2) \
$(LEVEL_OPT)$(LEVEL_3) \
$(LEVEL_OPT)$(LEVEL_4) \
$(LEVEL_OPT)$(LEVEL_5) \
