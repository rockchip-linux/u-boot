$(dst_image) : $(src_image)
	$(warning $(call convert-one-image,$<,$@))
