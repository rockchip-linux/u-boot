/*
 * (C) Copyright 2008 - 2009
 * Windriver, <www.windriver.com>
 * Tom Rix <Tom.Rix@windriver.com>
 *
 * Copyright 2011 Sebastian Andrzej Siewior <bigeasy@linutronix.de>
 *
 * Copyright 2014 Linaro, Ltd.
 * Rob Herring <robh@kernel.org>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#ifndef _FASTBOOT_H_
#define _FASTBOOT_H_

/* The 64 defined bytes plus \0 */
#define FASTBOOT_RESPONSE_LEN	(64 + 1)

void fastboot_info(const char *fmt, ...)
                __attribute__ ((format (__printf__, 1, 2)));
void fastboot_fail(char *response, const char *reason);
void fastboot_okay(char *response, const char *reason);

int fb_locked(void);
int fb_set_reboot_flag(void);
int fb_getvar(char *cmd, char* response, size_t chars_left);
int fb_unknown_command(char *cmd, char* response, size_t chars_left);

#endif /* _FASTBOOT_H_ */
