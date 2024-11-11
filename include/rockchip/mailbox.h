/* SPDX-License-Identifier:     GPL-2.0+ */
/*
 * (C) Copyright 2021 Rockchip Electronics Co., Ltd
 */
#ifndef _ROCKCHIP_MAILBOX_H_
#define _ROCKCHIP_MAILBOX_H_

struct rockchip_mbox_msg {
	u32 cmd;
	u32 data;
};
#endif
