/*
 * (C) Copyright 2008-2015 Rockchip Electronics
 * Peter, Software Engineering, <superpeter.cai@gmail.com>.
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */
#ifndef __RKPM_H
#define __RKPM_H


/* void pm callback function type */
typedef void (* v_pm_cb_f)(int flag);


/*
 * rkpm wakeup gpio init
 */
void rk_pm_wakeup_gpio_init(void);


/*
 * rkpm wakeup gpio deinit
 */
void rk_pm_wakeup_gpio_deinit(void);


/*
 * rkpm enter
 * module_pm_conf: callback function that control such as backlight/lcd on or off
 */
void rk_pm_enter(v_pm_cb_f module_pm_conf);


#endif /* __RKPM_H */
