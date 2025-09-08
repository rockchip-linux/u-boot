/*
 * (C) Copyright 2024 Rockchip Electronics Co., Ltd
 *
 * SPDX-License-Identifier:     GPL-2.0+
 */

#ifndef __HWID_DTB_H_
#define __HWID_DTB_H_

#define KEY_WORDS_ADC_CTRL	"#_"
#define KEY_WORDS_ADC_CH	"_ch"
#define KEY_WORDS_GPIO		"#gpio"

/*
 * hwid_init_data() - init data about hwid.
 *
 */
void hwid_init_data(void);

/*
 * hwid_dtb_is_available() -  find dtb file by HW(adc or gpio).
 *
 * @file_name: dtb name include with HW info.
 *
 * return found or not.
 */
bool hwid_dtb_is_available(const char *file_name);

#endif /* __RK_HWID_H_ */
