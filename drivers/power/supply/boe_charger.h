/* SPDX-License-Identifier: GPL-2.0
 *
 * Copyright (c) 2023 BOE Inc.
 */

#ifndef _BOE_SW_H_
#define _BOE_SW_H_

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
#include <linux/workqueue.h>
#include <linux/jiffies.h>
#include <linux/regulator/driver.h>
#include <linux/regulator/of_regulator.h>
#include <linux/phy/phy.h>
#include "charger_class.h"
#include "adapter_class.h"

#define BOE_CHARGER_VER            "boe_chg_v1.1"

#define REG_NUM				16

#define REG0				0x00
#define REG0_EN_HIZ_MASK		0x01
#define REG0_EN_HIZ_SHIFT		7
#define	REG0_STAT_IMON_CTRL_MASK	0x03
#define	REG0_STAT_IMON_CTRL_SHIFT	5
#define REG0_IINLIM_MASK		GENMASK(4, 0)
#define REG0_IINLIM_SHIFT		0

#define REG1				0x01
#define REG1_OTG_CONFIG_MASK		0x01
#define REG1_OTG_CONFIG_SHIFT		5
#define REG1_CHG_CONFIG_MASK		0x01
#define REG1_CHG_CONFIG_SHIFT		4

#define REG2				0x02
#define REG2_BOOST_LIM_MASK		0x01
#define REG2_BOOST_LIM_SHIFT		7
#define REG2_ICHG_MASK			0x3F
#define REG2_ICHG_SHIFT			0

#define REG3				0x03
#define REG3_ITERM_MASK			0x0F
#define REG3_ITERM_SHIFT		0

#define REG4				0x04
#define REG4_VREG_MASK			0x1F
#define REG4_VREG_SHIFT			3
#define REG4_VRECHG_MASK		0x01
#define REG4_VRECHG_SHIFT		0

#define REG5				0x05
#define REG5_EN_TIMER_MASK		0x01
#define REG5_EN_TIMER_SHIFT		3
#define REG5_WATCHDOG_MASK		0x03
#define REG5_WATCHDOG_SHIFT		4

#define REG6				0x06
#define	REG6_OVP_MASK			0x03
#define	REG6_OVP_SHIFT			6
#define	REG6_VINDPM_MASK		0x0F
#define	REG6_VINDPM_SHIFT		0

#define REG7				0x07
#define	REG7_BATFET_RST_EN_MASK		0x01
#define	REG7_BATFET_RST_EN_SHIFT	2
#define REG7_BATFET_DISABLE_MASK	0x01
#define REG7_BATFET_DISABLE_SHIFT	5
#define REG7_BATFET_DLY_MASK            0x01
#define REG7_BATFET_DLY_SHIFT           3
#define REG7_IINDET_EN_MASK            	0x01
#define REG7_IINDET_EN_SHIFT           	7

#define REG8				0x08
#define REG8_PG_STAT_MASK		BIT(2)
#define REG8_PG_STAT_SHIFT		2
#define REG8_VBUS_STAT_MASK		GENMASK(7, 5)
#define REG8_VBUS_STAT_SHIFT		5
#define REG8_VBUS_TYPE_NONE		0
#define REG8_VBUS_TYPE_SDP		1
#define REG8_VBUS_TYPE_CDP		2
#define REG8_VBUS_TYPE_DCP		3
#define REG8_VBUS_TYPE_HVDCP		4
#define REG8_VBUS_TYPE_UNKNOWN		5
#define REG8_VBUS_TYPE_NON_STD		6
#define REG8_VBUS_TYPE_OTG		7
#define REG8_CHRG_STAT_MASK		GENMASK(4, 3)
#define REG8_CHRG_STAT_SHIFT		3
#define REG8_CHRG_STAT_NOT		0
#define REG8_CHRG_STAT_PRE		1
#define REG8_CHRG_STAT_FAST		2
#define REG8_CHRG_STAT_TERM		3
#define REG8_CHARGING_TERMINATED	0x3

#define REG9				0x09

#define REG10				0x0A
#define	REG10_VBUS_GD_MASK		0x01
#define	REG10_VBUS_GD_SHIFT		7
#define	REG10_VINDPM_STAT_MASK		0x01
#define	REG10_VINDPM_STAT_SHIFT		6
#define	REG10_VINDPM_INT_MASK_MASK	0x01
#define	REG10_VINDPM_INT_MASK_SHIFT	1
#define	REG10_IINDPM_INT_MASK_MASK	0x01
#define	REG10_IINDPM_INT_MASK_SHIFT	0

#define	REG11				0x0B
#define REG11_REG_RST_MASK		0x01
#define REG11_REG_RST_SHIFT		7
#define REG11_REG_RST_VALUE		1
#define REG11_PN_MASK			0x0F
#define REG11_PN_SHIFT			3
#define REG11_REV_MASK			0x03
#define REG11_REV_SHIFT			0

#define	REG14				0x0E
#define REG14_INPUT_DET_DONE_MASK	0x01
#define REG14_INPUT_DET_DONE_SHIFT	7

#define	REG15				0x0F
#define	REG15_VINDPM_OS_MASK		0x03
#define	REG15_VINDPM_OS_SHIFT		0

struct state_reg {
        bool vsys_stat;
        bool therm_stat;
        bool pg_stat;
        u8 chrg_stat;
        u8 vbus_stat;

        bool chrg_en;
        bool hiz_en;
        bool term_en;
        bool input_det_done;
        u8 chrg_type;
        u8 health;
};

struct boe_chg {
	struct device *dev;
	struct power_supply *psy;
	struct power_supply_desc psy_desc;
	struct regulator_dev *rdev;
	struct regulator_desc rdesc;
	struct charger_device *chg_dev;
	struct charger_properties chg_props;
	struct i2c_client *client;
	struct mutex i2c_lock;
	struct delayed_work bc12_work;
	struct delayed_work charge_detect_work;
	struct state_reg state;

	const char *chg_name;
	enum power_supply_usb_type psy_usb_type;
	unsigned int gpio_secondary_en;
	bool disable_charge;
	bool is_primary_chg;
	u32 last_iin;
	int attach_type;
        bool vbus_gd;
        bool last_vbus_gd;
};

enum chg_dev_notifier_events {
	EVENT_FULL,
	EVENT_RECHARGE,
	EVENT_DISCHARGE,
};

/* map with mtk_chg_type_det.c */
enum attach_type {
        ATTACH_TYPE_NONE,
        ATTACH_TYPE_PWR_RDY,
        ATTACH_TYPE_TYPEC,
        ATTACH_TYPE_PD,
        ATTACH_TYPE_PD_SDP,
        ATTACH_TYPE_PD_DCP,
        ATTACH_TYPE_PD_NONSTD,
};

static const u32 SGM_ICHG_TABLE[] = {
        0,       5000,    10000,   15000,   20000,
        25000,   30000,   35000,   40000,   50000,
        60000,   70000,   80000,   90000,   100000,
        110000,  130000,  150000,  170000,  190000,
        210000,  230000,  250000,  270000,  300000,
        330000,  360000,  390000,  420000,  450000,
        480000,  510000,  540000,  600000,  660000,
        720000,  780000,  840000,  900000,  960000,
        1020000, 1080000, 1140000, 1200000, 1260000,
        1320000, 1380000, 1440000, 1500000, 1620000,
        1740000, 1860000, 1980000, 2100000, 2220000,
        2340000, 2460000, 2580000, 2700000, 2820000,
        2940000, 3000000
};

static const u32 SGM_ITERM_TABLE[] = {
        5000,   10000,  15000,  20000,  30000,
        40000,  50000,  60000,  80000,  100000,
        120000, 140000, 160000, 180000, 200000,
        240000
};
#endif // _BOE_SW_H_
