/* SPDX-License-Identifier: GPL-2.0
 *
 * Copyright (c) 2023 BOE Inc.
 */

#ifndef _SC89601D_SW_H_
#define _SC89601D_SW_H_

#include <linux/types.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/platform_device.h>
#include <linux/i2c.h>
#include <linux/slab.h>
#include <linux/delay.h>
#include <linux/interrupt.h>
#include <linux/of.h>
#include <linux/of_irq.h>
#include <linux/of_address.h>
#include <linux/of_device.h>
#include <linux/gpio.h>
#include <linux/of_gpio.h>
#include <linux/power_supply.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/of_regulator.h>
#include "charger_class.h"
#include "adapter_class.h"

//CON0
#define SC89601D_CON0			0x00

#define CON0_EN_HIZ_MASK		0x01
#define CON0_EN_HIZ_SHIFT		7

#define	CON0_STAT_IMON_CTRL_MASK	0x03
#define	CON0_STAT_IMON_CTRL_SHIFT	5

#define CON0_IINLIM_MASK		0x1F
#define CON0_IINLIM_SHIFT		0

//CON1
#define SC89601D_CON1			0x01

#define CON1_OTG_CONFIG_MASK		0x01
#define CON1_OTG_CONFIG_SHIFT		5

#define CON1_CHG_CONFIG_MASK		0x01
#define CON1_CHG_CONFIG_SHIFT		4

//CON2
#define SC89601D_CON2			0x02

#define CON2_BOOST_LIM_MASK		0x01
#define CON2_BOOST_LIM_SHIFT		7

#define CON2_ICHG_MASK			0x3F
#define CON2_ICHG_SHIFT			0

//CON3
#define SC89601D_CON3			0x03

#define CON3_ITERM_MASK			0x0F
#define CON3_ITERM_SHIFT		0

//CON4
#define SC89601D_CON4			0x04

#define CON4_VREG_MASK			0x1F
#define CON4_VREG_SHIFT			3

//CON5
#define SC89601D_CON5			0x05

#define CON5_EN_TIMER_MASK		0x01
#define CON5_EN_TIMER_SHIFT		3

#define CON5_WATCHDOG_MASK		0x03
#define CON5_WATCHDOG_SHIFT		4

//CON6
#define SC89601D_CON6			0x06

#define	CON6_OVP_MASK			0x03
#define	CON6_OVP_SHIFT			6

#define	CON6_VINDPM_MASK		0x0F
#define	CON6_VINDPM_SHIFT		0

//CON7
#define SC89601D_CON7			0x07

#define	CON7_BATFET_RST_EN_MASK		0x01
#define	CON7_BATFET_RST_EN_SHIFT	2

#define CON7_BATFET_DISABLE_MASK	0x01
#define CON7_BATFET_DISABLE_SHIFT	5

#define CON7_BATFET_DLY_MASK            0x01
#define CON7_BATFET_DLY_SHIFT           3

//CON8
#define SC89601D_CON8			0x08

#define CON8_VBUS_STAT_MASK		0x07
#define CON8_VBUS_STAT_SHIFT		5
#define CON8_VBUS_TYPE_NONE		0
#define CON8_VBUS_TYPE_SDP		1
#define CON8_VBUS_TYPE_CDP		2
#define CON8_VBUS_TYPE_DCP		3
#define CON8_VBUS_TYPE_HVDCP		4
#define CON8_VBUS_TYPE_UNKNOWN		5
#define CON8_VBUS_TYPE_NON_STD		6
#define CON8_VBUS_TYPE_OTG		7

#define CON8_CHRG_STAT_MASK		0x03
#define CON8_CHRG_STAT_SHIFT		3
#define CON8_CHRG_STAT_NOT		0
#define CON8_CHRG_STAT_PRE		1
#define CON8_CHRG_STAT_FAST		2
#define CON8_CHRG_STAT_TERM		3

//CON10
#define SC89601D_CON10			0x0A

#define	CON10_VBUS_GD_MASK		0x01
#define	CON10_VBUS_GD_SHIFT		7

#define	CON10_VINDPM_STAT_MASK		0x01
#define	CON10_VINDPM_STAT_SHIFT		6

//CON11
#define	SC89601D_CON11			0x0B

#define CON11_REG_RST_MASK		0x01
#define CON11_REG_RST_SHIFT		7
#define CON11_REG_RST_VALUE		1

#define CON11_PN_MASK			0x0F
#define CON11_PN_SHIFT			3

#define CON11_REV_MASK			0x03
#define CON11_REV_SHIFT			0

//CON14
#define	SC89601D_CON14			0x0E

#define CON14_INPUT_DET_DONE_MASK	0x01
#define CON14_INPUT_DET_DONE_SHIFT	6

//other
#define CHARGING_TERMINATED		0x3
#define SC89601D_REG_NUM		15

struct sc89601d_chip {
	struct device *dev;
	struct power_supply *psy;
	struct power_supply_desc psy_desc;
	struct regulator_dev *rdev;
	struct regulator_desc rdesc;
	struct charger_device *chg_dev;
	struct charger_properties chg_props;
	struct i2c_client *client;
	struct mutex i2c_lock;

	const char *chg_name;
	enum power_supply_usb_type psy_usb_type;
	bool power_good;
	int chg_stat;
	unsigned int gpio_slave_en;
};

enum chg_dev_notifier_events {
	EVENT_FULL,
	EVENT_RECHARGE,
	EVENT_DISCHARGE,
};

#endif // _SC89601D_SW_H_
