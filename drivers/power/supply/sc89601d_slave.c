/* SPDX-License-Identifier: GPL-2.0
 *
 * Copyright (c) 2023 BOE Inc.
 */

#include "sc89601d.h"

//#define MASTER_CHARGER_NAME "sc89601d_master"
#define SLAVE_CHARGER_NAME "sc89601d_slave"

#ifdef MASTER_CHARGER_NAME
#define sc89601d_dbg(x,y...) printk ("sc89601d_master: " x, ## y)
#else
#define sc89601d_dbg(x,y...) printk ("sc89601d_slave: " x, ## y)
#endif

//CON0
static const unsigned int SC89601D_INPUT_LIMIT_TABLE[] = {
	100000, 200000, 300000, 400000, 500000,
	600000, 700000, 800000, 900000, 1000000,
	1100000, 1200000, 1300000, 1400000, 1500000,
	1600000, 1700000, 1800000, 1900000, 2000000,
	2100000, 2200000, 2300000, 2400000, 2500000,
	2600000, 2700000, 2800000, 2900000, 3000000,
	3100000, 3200000
};

//CON2
static const u32 SC89601D_ICHG_TABLE[] = {
	0, 60000, 120000, 180000, 240000,
	300000, 360000, 420000, 480000, 540000,
	600000, 660000, 720000, 780000, 840000,
	900000, 960000, 1020000, 1080000, 1140000,
	1200000, 1260000, 1320000, 1380000, 1440000,
	1500000, 1560000, 1620000, 1680000, 1740000,
	1800000, 1860000, 1920000, 1980000, 2040000,
	2100000, 2160000, 2220000, 2280000, 2340000,
	2400000, 2460000, 2520000, 2580000, 2640000,
	2700000, 2760000, 2820000, 2880000, 2940000,
	3000000
};

//CON3
static const u32 SC89601D_ITERM_TABLE[] = {
	60000, 120000, 180000, 240000, 300000,
	360000, 420000, 480000, 540000, 600000,
	660000, 720000, 780000, 840000, 900000,
	960000
};

//CON4
static const u32 SC89601D_VREG_TABLE[] = {
	3848000, 3880000, 3912000, 3944000, 3976000,
	4008000, 4040000, 4072000, 4104000, 4136000,
	4168000, 4200000, 4232000, 4264000, 4296000,
	4328000, 4360000, 4392000, 4424000, 4456000,
	4488000, 4520000, 4552000, 4584000, 4616000,
	4648000, 4680000, 4712000, 4744000, 4776000,
	4808000, 4840000
};

//CON6
static const unsigned int SC89601D_VINDPM_TABLE[] = {
	3900000, 4000000, 4100000, 4200000, 4300000,
	4400000, 4500000, 4600000, 4700000, 4800000,
	4900000, 5000000, 5100000, 8000000, 8200000,
	8400000
};

static int sc89601d_read_byte(struct i2c_client *client, u8 cmd)
{
	s32 ret;
	struct sc89601d_chip *chip = i2c_get_clientdata(client);

	mutex_lock(&chip->i2c_lock);
	ret = i2c_smbus_read_byte_data(client, cmd);
	mutex_unlock(&chip->i2c_lock);

	return ret;
}

static int sc89601d_write_byte(struct i2c_client *client, u8 cmd, u8 val)
{
	s32 ret;
	struct sc89601d_chip *chip = i2c_get_clientdata(client);

	mutex_lock(&chip->i2c_lock);
	ret = i2c_smbus_write_byte_data(client, cmd, val);
	mutex_unlock(&chip->i2c_lock);
	return ret;
}

static int sc89601d_read_interface(struct i2c_client *client, u8 reg, u8 *val, u8 mask, u8 shift)
{
	int ret = 0;

	ret = sc89601d_read_byte(client, reg);
	if (ret < 0)
		return ret;

	ret &= (mask << shift);
	*val = (ret >> shift);

	return 0;
}

static int sc89601d_write_interface(struct i2c_client *client, u8 reg, u8 val, u8 mask, u8 shift)
{
	int ret = 0;
	u8 read_reg = 0;

	ret = sc89601d_read_byte(client, reg);
	if (ret < 0)
		return -EIO;

	read_reg = ret;
	ret &= ~(mask << shift);
	ret |= (val << shift);

	return sc89601d_write_byte(client, reg, ret);
}

/* CON0---------------------------------------------------- */
static int sc89601d_set_en_hiz(struct sc89601d_chip *chip, u8 val)
{
	int ret = 0;

	sc89601d_dbg("%s, val = 0x%x\n", __func__, val);
	ret = sc89601d_write_interface(chip->client, (u8) (SC89601D_CON0),
			(u8) (val),
			(u8) (CON0_EN_HIZ_MASK),
			(u8) (CON0_EN_HIZ_SHIFT)
			);
	if (ret < 0)
		sc89601d_dbg("%s fail\n", __func__);

	return ret;
}

static int sc89601d_get_en_hiz(struct sc89601d_chip *chip, u8 *val)
{
	return sc89601d_read_interface(chip->client, (u8) (SC89601D_CON0),
			(val),
			(u8) (CON0_EN_HIZ_MASK),
			(u8) (CON0_EN_HIZ_SHIFT)
			);
}

static int sc89601d_set_aicr(struct sc89601d_chip *chip, u8 val)
{
	int ret = 0;

	sc89601d_dbg("%s, val = 0x%x\n", __func__, val);
	ret = sc89601d_write_interface(chip->client, (u8) (SC89601D_CON0),
			(u8) (val),
			(u8) (CON0_IINLIM_MASK),
			(u8) (CON0_IINLIM_SHIFT)
			);
	if (ret < 0)
		sc89601d_dbg("%s fail\n", __func__);

	return ret;
}

static int sc89601d_get_aicr(struct sc89601d_chip *chip, u8 *val)
{
	return sc89601d_read_interface(chip->client, (u8) (SC89601D_CON0),
			(val),
			(u8) (CON0_IINLIM_MASK),
			(u8) (CON0_IINLIM_SHIFT)
			);
}

static int sc89601d_set_stat_ctrl(struct sc89601d_chip *chip, u8 val)
{
	int ret = 0;

	sc89601d_dbg("%s, val = 0x%x\n", __func__, val);
	ret = sc89601d_write_interface(chip->client, (u8) (SC89601D_CON0),
			(u8) (val),
			(u8) (CON0_STAT_IMON_CTRL_MASK),
			(u8) (CON0_STAT_IMON_CTRL_SHIFT)
			);
	if (ret < 0)
		sc89601d_dbg("%s fail\n", __func__);

	return ret;
}

/* CON1---------------------------------------------------- */
static int sc89601d_set_otg_config(struct sc89601d_chip *chip, u8 val)
{
	int ret = 0;

	sc89601d_dbg("%s, val = 0x%x\n", __func__, val);
	ret = sc89601d_write_interface(chip->client, (u8) (SC89601D_CON1),
			(u8) (val),
			(u8) (CON1_OTG_CONFIG_MASK),
			(u8) (CON1_OTG_CONFIG_SHIFT)
			);
	if (ret < 0)
		sc89601d_dbg("%s fail\n", __func__);

	return ret;
}

static int sc89601d_get_otg_config(struct sc89601d_chip *chip, u8 *val)
{
	return sc89601d_read_interface(chip->client, (u8) (SC89601D_CON1),
			(val),
			(u8) (CON1_OTG_CONFIG_MASK),
			(u8) (CON1_OTG_CONFIG_SHIFT)
			);
}

static int sc89601d_set_chg_config(struct sc89601d_chip *chip, u8 val)
{
	int ret = 0;

	sc89601d_dbg("%s, val = 0x%x\n", __func__, val);

	ret = sc89601d_write_interface(chip->client, (u8) (SC89601D_CON1),
			(u8) (val),
			(u8) (CON1_CHG_CONFIG_MASK),
			(u8) (CON1_CHG_CONFIG_SHIFT)
			);
	if (ret < 0)
		sc89601d_dbg("%s fail\n", __func__);

	return ret;
}

static int sc89601d_get_chg_config(struct sc89601d_chip *chip, u8 *val)
{
	return sc89601d_read_interface(chip->client, (u8) (SC89601D_CON1),
			(val),
			(u8) (CON1_CHG_CONFIG_MASK),
			(u8) (CON1_CHG_CONFIG_SHIFT)
			);
}

/* CON2---------------------------------------------------- */
static int sc89601d_set_ichg(struct sc89601d_chip *chip, u8 val)
{
	int ret = 0;

	sc89601d_dbg("%s, val = 0x%x\n", __func__, val);
	ret = sc89601d_write_interface(chip->client, (u8) (SC89601D_CON2),
			(u8) (val),
			(u8) (CON2_ICHG_MASK),
			(u8) (CON2_ICHG_SHIFT)
			);
	if (ret < 0)
		sc89601d_dbg("%s fail\n", __func__);

	return ret;
}

static int sc89601d_get_ichg(struct sc89601d_chip *chip, u8 *val)
{
	return sc89601d_read_interface(chip->client, (u8) (SC89601D_CON2),
			(val),
			(u8) (CON2_ICHG_MASK),
			(u8) (CON2_ICHG_SHIFT)
			);
}

/* CON3---------------------------------------------------- */
static int sc89601d_set_iterm(struct sc89601d_chip *chip, u8 val)
{
	int ret = 0;

	sc89601d_dbg("%s, val = 0x%x\n", __func__, val);
	ret = sc89601d_write_interface(chip->client, (u8) (SC89601D_CON3),
			(u8) (val),
			(u8) (CON3_ITERM_MASK),
			(u8) (CON3_ITERM_SHIFT)
			);
	if (ret < 0)
		sc89601d_dbg("%s fail\n", __func__);

	return ret;
}

/* CON4---------------------------------------------------- */
static int sc89601d_set_vreg(struct sc89601d_chip *chip, u8 val)
{
	int ret = 0;

	sc89601d_dbg("%s, val = 0x%x\n", __func__, val);
	ret = sc89601d_write_interface(chip->client, (u8) (SC89601D_CON4),
			(u8) (val),
			(u8) (CON4_VREG_MASK),
			(u8) (CON4_VREG_SHIFT)
			);
	if (ret < 0)
		sc89601d_dbg("%s fail\n", __func__);

	return ret;
}

/* CON5---------------------------------------------------- */
static int sc89601d_set_watchdog(struct sc89601d_chip *chip, u8 val)
{
	int ret = 0;

	sc89601d_dbg("%s, val = 0x%x\n", __func__, val);
	ret = sc89601d_write_interface(chip->client, (u8) (SC89601D_CON5),
			(u8) (val),
			(u8) (CON5_WATCHDOG_MASK),
			(u8) (CON5_WATCHDOG_SHIFT)
			);
	if (ret < 0)
		sc89601d_dbg("%s fail\n", __func__);

	return ret;
}

static int sc89601d_en_timer(struct sc89601d_chip *chip, u8 val)
{
	int ret = 0;

	sc89601d_dbg("%s, val = 0x%x\n", __func__, val);
	ret = sc89601d_write_interface(chip->client, SC89601D_CON5, val, CON5_EN_TIMER_MASK, CON5_EN_TIMER_SHIFT);
	if (ret < 0)
		sc89601d_dbg("%s fail\n", __func__);

	return ret;
}

/* CON6---------------------------------------------------- */
static int sc89601d_set_vindpm(struct sc89601d_chip *chip, u8 val)
{
	int ret = 0;

	sc89601d_dbg("%s, val = 0x%x\n", __func__, val);
	ret = sc89601d_write_interface(chip->client, (u8) (SC89601D_CON6),
			(u8) (val),
			(u8) (CON6_VINDPM_MASK),
			(u8) (CON6_VINDPM_SHIFT)
			);
	if (ret < 0)
		sc89601d_dbg("%s fail\n", __func__);

	return ret;
}

static int sc89601d_get_vindpm(struct sc89601d_chip *chip, u8 *val)
{
	return sc89601d_read_interface(chip->client, (u8) (SC89601D_CON6),
			(val),
			(u8) (CON6_VINDPM_MASK),
			(u8) (CON6_VINDPM_SHIFT)
			);
}

static int sc89601d_set_ovp(struct sc89601d_chip *chip, u8 val)
{
	int ret = 0;

	sc89601d_dbg("%s, val = 0x%x\n", __func__, val);
	ret = sc89601d_write_interface(chip->client, (u8) (SC89601D_CON6),
			(u8) (val),
			(u8) (CON6_OVP_MASK),
			(u8) (CON6_OVP_SHIFT)
			);
	if (ret < 0)
		sc89601d_dbg("%s fail\n", __func__);

	return ret;
}

/* CON7---------------------------------------------------- */
static int sc89601d_set_batfet_reset_enable(struct sc89601d_chip *chip, u8 val)
{
	int ret = 0;

	sc89601d_dbg("%s, val = 0x%x\n", __func__, val);
	ret = sc89601d_write_interface(chip->client, (u8) (SC89601D_CON7),
			(u8) (val),
			(u8) (CON7_BATFET_RST_EN_MASK),
			(u8) (CON7_BATFET_RST_EN_SHIFT)
			);
	if (ret < 0)
		sc89601d_dbg("%s fail\n", __func__);

	return ret;
}

static int sc89601d_set_batfet_dly(struct sc89601d_chip *chip, u8 val)
{
	int ret = 0;

	sc89601d_dbg("%s, val = 0x%x\n", __func__, val);
	ret = sc89601d_write_interface(chip->client, (u8) (SC89601D_CON7),
			(u8) (val),
			(u8) (CON7_BATFET_DLY_MASK),
			(u8) (CON7_BATFET_DLY_SHIFT)
			);
	if (ret < 0)
		sc89601d_dbg("%s fail\n", __func__);

	return ret;
}

#ifdef SLAVE_CHARGER_NAME
static int sc89601d_set_batfet_dis(struct sc89601d_chip *chip, u8 val)
{
	int ret = 0;

	sc89601d_dbg("%s, val = 0x%x\n", __func__, val);
	ret = sc89601d_write_interface(chip->client, (u8) (SC89601D_CON7),
			(u8) (val),
			(u8) (CON7_BATFET_DISABLE_MASK),
			(u8) (CON7_BATFET_DISABLE_SHIFT)
			);
	if (ret < 0)
		sc89601d_dbg("%s fail\n", __func__);

	return ret;
}
#endif

/* CON8---------------------------------------------------- */
static int sc89601d_get_chrg_stat(struct sc89601d_chip *chip, u8 *val)
{
	return sc89601d_read_interface(chip->client, (u8) (SC89601D_CON8),
			(val),
			(u8) (CON8_CHRG_STAT_MASK),
			(u8) (CON8_CHRG_STAT_SHIFT)
			);
}

/*CON10----------------------------------------------------------*/
static int sc89601d_get_vindpm_stat(struct sc89601d_chip *chip, u8 *val)
{
	return sc89601d_read_interface(chip->client, (u8) (SC89601D_CON10),
			(val),
			(u8) (CON10_VINDPM_STAT_MASK),
			(u8) (CON10_VINDPM_STAT_SHIFT)
			);
}

/* CON11---------------------------------------------------- */
static bool sc89601d_check_devinfo(struct sc89601d_chip *chip)
{
	int ret;
	u8 chip_id;

	ret = i2c_smbus_read_byte_data(chip->client, SC89601D_CON11);
	if (ret < 0) {
		sc89601d_dbg("%s get devchip fail(%d)\n",
				__func__, ret);
		return false;
	}

	ret &= (CON11_PN_MASK << CON11_PN_SHIFT);
	chip_id = (ret >> CON11_PN_SHIFT);

	sc89601d_dbg("%s reg11:0x%x, chip_id = 0x%x\n", __func__, ret, chip_id);

	return true;
}

static int sc89601d_reset_reg(struct sc89601d_chip *chip)
{
	int ret = 0;

	sc89601d_dbg("%s\n", __func__);
	ret = sc89601d_write_interface(chip->client, (u8) (SC89601D_CON11),
			(u8) (CON11_REG_RST_VALUE),
			(u8) (CON11_REG_RST_MASK),
			(u8) (CON11_REG_RST_SHIFT)
			);
	if (ret < 0)
		sc89601d_dbg("%s fail\n", __func__);

	return ret;
}

/* CON14---------------------------------------------------- */
static void convert_to_reg(const u32 *table, int table_size, u32 raw_val, u8 *reg_val)
{
	int i;

	if(raw_val > table[table_size - 1]) {
		*reg_val = (table_size - 1);
		sc89601d_dbg("raw_val = %d, out of [%d-%d], default use %d\n",
				raw_val, table[0], table[table_size - 1], table[table_size - 1]);
		return;
	}

	if(raw_val < table[0]) {
		*reg_val = 0;
		sc89601d_dbg("raw_val = %d, out of [%d-%d], default use %d\n",
				raw_val, table[0], table[table_size - 1], table[0]);
		return;
	}

	for (i = 0; i < table_size; i++) {
		if (table[i] > raw_val) {
			*reg_val = i-1;
			return;
		}

		if (table[i] == raw_val) {
			*reg_val = i;
			return;
		}
	}

	sc89601d_dbg("%s fail\n", __func__);
}

static int sc89601d_set_charging_current(struct charger_device *chg_dev, u32 ichg)
{
	u8 reg_val;
	struct sc89601d_chip *chip = dev_get_drvdata(&chg_dev->dev);

	ichg = 3000000;
	convert_to_reg(SC89601D_ICHG_TABLE, ARRAY_SIZE(SC89601D_ICHG_TABLE), ichg, &reg_val);

	sc89601d_dbg("%s, ichg:%duA, reg_val:0x%x\n", __func__, ichg, reg_val);

	return sc89601d_set_ichg(chip, reg_val);
}

static int sc89601d_get_min_charging_current(struct charger_device *chg_dev, u32 *min_ichg)
{
	*min_ichg = SC89601D_ICHG_TABLE[0];
	sc89601d_dbg("%s, min_ichg:%duA\n", __func__, *min_ichg);
	return 0;
}

static int sc89601d_get_input_current(struct charger_device *chg_dev, u32 *aicr)
{
	int ret = 0;
	u8 reg_val = 0;
	struct sc89601d_chip *chip = dev_get_drvdata(&chg_dev->dev);

	ret = sc89601d_get_aicr(chip, &reg_val);
	if (ret < 0)
		return ret;

	*aicr = SC89601D_INPUT_LIMIT_TABLE[reg_val];
	sc89601d_dbg("%s, reg_val:0x%x, aicr:%duA\n", __func__, reg_val, *aicr);

	return ret;
}

static int sc89601d_set_input_current(struct charger_device *chg_dev, u32 aicr)
{
	u8 reg_val;
	struct sc89601d_chip *chip = dev_get_drvdata(&chg_dev->dev);

	convert_to_reg(SC89601D_INPUT_LIMIT_TABLE, ARRAY_SIZE(SC89601D_INPUT_LIMIT_TABLE), aicr, &reg_val);

	sc89601d_dbg("%s, aicr:%duA, reg_val:0x%x\n", __func__, aicr, reg_val);

	return sc89601d_set_aicr(chip, reg_val);
}

static int sc89601d_get_min_input_current(struct charger_device *chg_dev, u32 *min_aicr)
{
	*min_aicr = SC89601D_INPUT_LIMIT_TABLE[0];
	sc89601d_dbg("%s, min_aicr:%duA\n", __func__, *min_aicr);
	return 0;
}

static int sc89601d_set_eoc_current(struct charger_device *chg_dev, u32 ieoc)
{
	u8 reg_val;
	struct sc89601d_chip *chip = dev_get_drvdata(&chg_dev->dev);

	convert_to_reg(SC89601D_ITERM_TABLE, ARRAY_SIZE(SC89601D_ITERM_TABLE), ieoc, &reg_val);

	sc89601d_dbg("%s, ieoc:%duA, reg_val:0x%x\n", __func__, ieoc, reg_val);

	return sc89601d_set_iterm(chip, reg_val);
}

static int sc89601d_event(struct charger_device *chg_dev, u32 event, u32 args)
{
	sc89601d_dbg("%s event = %d\n", __func__, event);

	switch (event) {
		case EVENT_FULL:
			charger_dev_notify(chg_dev, CHARGER_DEV_NOTIFY_EOC);
			break;
		case EVENT_RECHARGE:
			charger_dev_notify(chg_dev, CHARGER_DEV_NOTIFY_RECHG);
			break;
		default:
			break;
	}

	return 0;
}

static int sc89601d_set_constant_voltage(struct charger_device *chg_dev, u32 cv)
{
	u8 reg_val;
	struct sc89601d_chip *chip = dev_get_drvdata(&chg_dev->dev);

	convert_to_reg(SC89601D_VREG_TABLE, ARRAY_SIZE(SC89601D_VREG_TABLE), cv, &reg_val);

	sc89601d_dbg("%s, cv:%duV, reg_val:0x%x\n", __func__, cv, reg_val);

	return sc89601d_set_vreg(chip, reg_val);
}

static int sc89601d_get_mivr(struct charger_device *chg_dev, u32 *mivr)
{
	int ret;
	u8 reg_vindpm;
	struct sc89601d_chip *chip = dev_get_drvdata(&chg_dev->dev);

	ret = sc89601d_get_vindpm(chip, &reg_vindpm);
	if (ret < 0)
		return ret;

	*mivr = SC89601D_VINDPM_TABLE[reg_vindpm];

	sc89601d_dbg("%s, reg_vindpm:0x%x, mivr:%duV\n", __func__, reg_vindpm, *mivr);
	return ret;
}

static int sc89601d_enable(struct charger_device *chg_dev, bool en)
{
	int ret;
	struct sc89601d_chip *chip = dev_get_drvdata(&chg_dev->dev);

	sc89601d_dbg("%s, en = %d\n", __func__, en);

#ifdef SLAVE_CHARGER_NAME
	ret = sc89601d_set_batfet_dis(chip, !en);
#endif
	ret = sc89601d_set_chg_config(chip, en);

	return ret;
}

static int sc89601d_set_mivr(struct charger_device *chg_dev, u32 mivr)
{
	u8 reg_val;
	struct sc89601d_chip *chip = dev_get_drvdata(&chg_dev->dev);

	convert_to_reg(SC89601D_VINDPM_TABLE, ARRAY_SIZE(SC89601D_VINDPM_TABLE), mivr, &reg_val);

	sc89601d_dbg("%s, mivr = %duV, reg_val = 0x%x\n", __func__, mivr, reg_val);
	return sc89601d_set_vindpm(chip, reg_val);
}

static int sc89601d_get_mivr_state(struct charger_device *chg_dev, bool *in_loop)
{
	int ret;
	u8 reg_val;
	struct sc89601d_chip *chip = dev_get_drvdata(&chg_dev->dev);

	ret = sc89601d_get_vindpm_stat(chip, &reg_val);
	if (ret < 0)
		return ret;

	if (reg_val == 1)
		*in_loop = true;
	else
		*in_loop = false;
	sc89601d_dbg("%s, reg_val:0x%x, mivr_state:%d\n", __func__, reg_val, *in_loop);

	return ret;
}

static int sc89601d_is_charging_done(struct charger_device *chg_dev, bool *is_done)
{
	int ret;
	u8 reg_val;
	struct sc89601d_chip *chip = dev_get_drvdata(&chg_dev->dev);

	ret = sc89601d_get_chrg_stat(chip, &reg_val);
	if (ret < 0)
		return ret;

	if (reg_val == CHARGING_TERMINATED)
		*is_done = true;
	else
		*is_done = false;
	sc89601d_dbg("%s, is_done:%d\n", __func__, *is_done);

	return ret;
}

static int sc89601d_enable_discharge(struct charger_device *chg_dev, bool en)
{
	sc89601d_dbg("%s en = %d\n", __func__, en);
	return sc89601d_enable(chg_dev, !en);
}

static int sc89601d_otg_is_enabled(struct regulator_dev *rdev)
{
	struct sc89601d_chip *chip = rdev_get_drvdata(rdev);
	u8 otg_on, hiz_on;

	if (sc89601d_get_en_hiz(chip, &hiz_on) < 0)
		return -EIO;

	if (sc89601d_get_otg_config(chip, &otg_on) < 0)
		return -EIO;
	sc89601d_dbg("%s, hiz_on:%d, otg_on:%d\n", __func__, hiz_on, otg_on);

	if (hiz_on == 0 && otg_on == 1)
		return 1;

	return 0;
}

static int sc89601d_enable_otg(struct regulator_dev *rdev)
{
	struct sc89601d_chip *chip = rdev_get_drvdata(rdev);

	sc89601d_dbg("%s\n", __func__);
	if (sc89601d_set_en_hiz(chip, 0) < 0)
		return -EIO;
	return sc89601d_set_otg_config(chip, 1);
}

static int sc89601d_disable_otg(struct regulator_dev *rdev)
{
	struct sc89601d_chip *chip = rdev_get_drvdata(rdev);

	sc89601d_dbg("%s\n", __func__);
	if (sc89601d_set_en_hiz(chip, 1) < 0)
		return -EIO;
	return sc89601d_set_otg_config(chip, 0);
}

static int sc89601d_parse_dts(struct sc89601d_chip *chip)
{
	int ret = 0;
	struct device_node *np = chip->dev->of_node;

	if (!np) {
		sc89601d_dbg("%s no device node\n", __func__);
		return -EINVAL;
	}

	if (of_property_read_string(np, "charger_name",
				&chip->chg_name) < 0) {
#ifdef MASTER_CHARGER_NAME
		chip->chg_name = "primary_chg";
#else
		chip->chg_name = "secondary_chg";
#endif
		sc89601d_dbg("%s: no charger name\n", __func__);
	}

	if (of_property_read_string(np, "alias_name",
				&(chip->chg_props.alias_name)) < 0) {
#ifdef MASTER_CHARGER_NAME
		chip->chg_props.alias_name = MASTER_CHARGER_NAME;
#else
		chip->chg_props.alias_name = SLAVE_CHARGER_NAME;
#endif
		sc89601d_dbg("%s: no alias name\n", __func__);
	}

#ifdef SLAVE_CHARGER_NAME
	chip->gpio_slave_en = of_get_named_gpio(np, "sc,gpio_slave_en", 0);
	if (!gpio_is_valid(chip->gpio_slave_en))
		sc89601d_dbg("%s, get gpio failed\n", __func__);

	ret = gpio_request(chip->gpio_slave_en, "gpio_slave_en");
	if (ret)
		sc89601d_dbg("%s, request gpio fail\n", __func__);
#endif

	sc89601d_dbg("charger_name = %s, alias_name = %s\n", chip->chg_name, chip->chg_props.alias_name);

	return ret;
}

static int sc89601d_is_enable(struct charger_device *chg_dev, bool *en)
{
	int ret;
	u8 val;
	struct sc89601d_chip *chip = dev_get_drvdata(&chg_dev->dev);

	ret = sc89601d_get_chg_config(chip, &val);
	if (ret < 0)
		return ret;

	if(val == 1)
		*en = true;
	else
		*en = false;
	sc89601d_dbg("%s, is_enable:%d\n", __func__, *en);

	return ret;
}

static int sc89601d_plug_in(struct charger_device *chg_dev)
{
	sc89601d_dbg("%s\n", __func__);
	return sc89601d_enable(chg_dev, true);
}

static int sc89601d_plug_out(struct charger_device *chg_dev)
{
	sc89601d_dbg("%s\n", __func__);
	return sc89601d_enable(chg_dev, false);
}

static const struct regulator_ops sc89601d_otg_ops = {
	.is_enabled = sc89601d_otg_is_enabled,
	.enable = sc89601d_enable_otg,
	.disable = sc89601d_disable_otg,
};

static const struct regulator_desc sc89601d_otg_rdesc = {
	.of_match = "usb-otg-vbus",
	.name = "usb-otg-vbus",
	.ops = &sc89601d_otg_ops,
	.owner = THIS_MODULE,
	.type = REGULATOR_VOLTAGE,
};

static ssize_t dump_reg_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int i = 0, count = 0, reg = 0;
	struct sc89601d_chip *chip = dev_get_drvdata(dev);

	if (chip == NULL) {
		sc89601d_dbg("%s chip == NULL\n", __func__);
		return -EINVAL;
	}

	for (i = 0; i <SC89601D_REG_NUM; i++) {
		reg = sc89601d_read_byte(chip->client, i);
		if (reg < 0) {
			sc89601d_dbg("%s, sc89601d_read_byte fail\n", __func__);
			return -EIO;
		}
		count += snprintf(buf + count, PAGE_SIZE, "REG%02x %02x\n", i, reg);
	}
	count += snprintf(buf + count, PAGE_SIZE, "\n");

	return count;
}

static ssize_t dump_reg_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t size)
{
	int ret = 0;
	char *pvalue = NULL, *addr, *val;
	u32 reg_value = 0;
	u32 reg_address = 0;
	struct sc89601d_chip *chip = dev_get_drvdata(dev);

	if (chip == NULL) {
		sc89601d_dbg("%s chip == NULL\n", __func__);
		return -EINVAL;
	}

	if (buf != NULL && size != 0) {
		pvalue = (char *)buf;
		addr = strsep(&pvalue, " ");
		ret = kstrtou32(addr, 16, (u32 *)&reg_address);

		val = strsep(&pvalue, " ");
		ret = kstrtou32(val, 16, (u32 *)&reg_value);

		ret = sc89601d_write_byte(chip->client, reg_address, reg_value);
		if (ret < 0) {
			sc89601d_dbg("Failed: reg=%02X, ret=%d\n", (u32)reg_address, ret);
			return -EIO;
		}
	}

	return size;
}

static DEVICE_ATTR_RW(dump_reg);

static int sc89601d_dump_register(struct charger_device *chg_dev)
{
	int i = 0;
	u8 val[SC89601D_REG_NUM] = {0};
	struct sc89601d_chip *chip = dev_get_drvdata(&chg_dev->dev);

	if (chip == NULL) {
		sc89601d_dbg("%s chip == NULL\n");
		return -EINVAL;
	}

	for (i=0; i<SC89601D_REG_NUM; i++) {
		val[i] = sc89601d_read_byte(chip->client, i);
		if (val[i] < 0) {
			sc89601d_dbg("%s, sc89601d_read_byte fail\n", __func__);
			return -EIO;
		}
	}

	sc89601d_dbg("%s, r0:0x%x,r1:0x%x,r2:0x%x,r3:0x%x,r4:0x%x,r5:0x%x,r6:0x%x,r7:0x%x,r8:0x%x,r9:0x%x,ra:0x%x,rb:0x%x,rc:0x%x,rd:0x%x,re:0x%x\n",
			__func__, val[0], val[1], val[2], val[3], val[4], val[5], val[6], val[7], val[8], val[9], val[10], val[11], val[12], val[13], val[14]);

	return 0;
}

static int sc89601d_enable_chip(struct charger_device *chg_dev, bool en)
{
	struct sc89601d_chip *chip = dev_get_drvdata(&chg_dev->dev);

	sc89601d_dbg("%s en = %d\n", __func__, en);
	return sc89601d_set_en_hiz(chip, en ? 0 : 1);
}

static int sc89601d_is_chip_enabled(struct charger_device *chg_dev, bool *en)
{
	int ret = 0;
	u8 val;
	struct sc89601d_chip *chip = dev_get_drvdata(&chg_dev->dev);

	ret = sc89601d_get_en_hiz(chip, &val);
	if (ret < 0)
		return ret;

	if (val == 0)
		*en = true;
	else
		*en = false;
	sc89601d_dbg("%s, is_chip_enable:%d\n", __func__, *en);

	return ret;
}

static int sc89601d_get_charging_current(struct charger_device *chg_dev, u32 *ichg)
{
	int ret = 0;
	u8 reg_val = 0;
	struct sc89601d_chip *chip = dev_get_drvdata(&chg_dev->dev);

	ret = sc89601d_get_ichg(chip, &reg_val);
	if (ret < 0)
		return ret;

	*ichg = SC89601D_ICHG_TABLE[reg_val];
	sc89601d_dbg("%s, reg_val:0x%x, ichg:%duA\n", __func__, reg_val, *ichg);

	return ret;
}

static struct charger_ops sc89601d_chg_ops = {
	/* cable plug in/out */
	.plug_in = sc89601d_plug_in,
	.plug_out = sc89601d_plug_out,

	/* enable/disable charger */
	.enable = sc89601d_enable,
	.is_enabled = sc89601d_is_enable,

	/* enable/disable chip */
	.enable_chip = sc89601d_enable_chip,
	.is_chip_enabled = sc89601d_is_chip_enabled,

	/* get/set charging current*/
	.get_charging_current = sc89601d_get_charging_current,
	.set_charging_current = sc89601d_set_charging_current,
	.get_min_charging_current = sc89601d_get_min_charging_current,

	/* set cv */
	.set_constant_voltage = sc89601d_set_constant_voltage,

	/* set input_current */
	.get_input_current = sc89601d_get_input_current,
	.set_input_current = sc89601d_set_input_current,
	.get_min_input_current = sc89601d_get_min_input_current,

	/* set termination current */
	.set_eoc_current = sc89601d_set_eoc_current,

	.event = sc89601d_event,

	/* set mivr */
	.get_mivr = sc89601d_get_mivr,
	.set_mivr = sc89601d_set_mivr,
	.get_mivr_state = sc89601d_get_mivr_state,

	/* PD */
	.enable_discharge = sc89601d_enable_discharge,

	/* run AICL */
	//.run_aicl =

	.is_charging_done = sc89601d_is_charging_done,

	.dump_registers = sc89601d_dump_register,
};

static int sc89601d_init_setting(struct sc89601d_chip *chip)
{
	int ret = 0;

	ret = sc89601d_set_watchdog(chip, 0x0);			/* WDT disable */
	ret |= sc89601d_en_timer(chip, 0x0);			/* Disable Charging Safety Timer */
	ret |= sc89601d_set_stat_ctrl(chip, 0x3);		/* Disable STAT Pin Function */
	ret |= sc89601d_set_ovp(chip, 0x2);			/* vbus overvoltage protection, 11V */
	ret |= sc89601d_set_iterm(chip, 0x1);			/* Termination current 120mA */
	ret |= sc89601d_set_batfet_reset_enable(chip, 0x0);	/* Disable batfet reset */
	ret |= sc89601d_set_batfet_dly(chip, 0x0);              /* Turn off BATFET immediately */

#ifdef SLAVE_CHARGER_NAME
	/* default disable slave charger */
	ret |= sc89601d_set_batfet_dis(chip, 0x1);              /* Turn off BATFET */
	ret |= sc89601d_set_chg_config(chip, 0x0);
	ret |= gpio_direction_output(chip->gpio_slave_en, 1);
#endif
	if (ret < 0) {
		sc89601d_dbg("%s fail\n", __func__);
		return -EIO;
	}

	sc89601d_dbg("%s: down!\n", __func__);
	return ret;
}

#ifdef MASTER_CHARGER_NAME
static int sc89601d_get_vbus(void)
{
	union power_supply_propval prop;
	static struct power_supply *psy;
	int ret;

	psy = power_supply_get_by_name("mtk-gauge");
	if (psy == NULL || IS_ERR(psy)) {
		sc89601d_dbg("%s, Couldn't get psy\n", __func__);
		return -1;
	}

	ret = power_supply_get_property(psy, POWER_SUPPLY_PROP_VOLTAGE_NOW, &prop);
	if (ret < 0) {
		sc89601d_dbg("%s, get vbus fail\n", __func__);
		return -1;
	}

	sc89601d_dbg("%s, vbus:%d\n", __func__, prop.intval);
	return prop.intval;
}

static char *sc89601d_psy_supplied_to[] = {
	"battery",
	"mtk-master-charger"
};

static enum power_supply_usb_type sc89601d_psy_usb_types[] = {
	POWER_SUPPLY_USB_TYPE_UNKNOWN,
	POWER_SUPPLY_USB_TYPE_SDP,
	POWER_SUPPLY_USB_TYPE_CDP,
	POWER_SUPPLY_USB_TYPE_DCP,
};

static enum power_supply_property sc89601d_psy_properties[] = {
	POWER_SUPPLY_PROP_ONLINE,
	POWER_SUPPLY_PROP_STATUS,
	POWER_SUPPLY_PROP_TYPE,
	POWER_SUPPLY_PROP_USB_TYPE,
	POWER_SUPPLY_PROP_VOLTAGE_NOW,
};

static int sc89601d_get_property(struct power_supply *psy,
		enum power_supply_property psp, union power_supply_propval *val)
{
	struct sc89601d_chip *chip = power_supply_get_drvdata(psy);

	switch (psp) {
		case POWER_SUPPLY_PROP_ONLINE:
			val->intval = chip->power_good;
			break;
		case POWER_SUPPLY_PROP_STATUS:
			val->intval = chip->chg_stat;
			break;
		case POWER_SUPPLY_PROP_TYPE:
			val->intval = chip->psy_desc.type;
			break;
		case POWER_SUPPLY_PROP_USB_TYPE:
			val->intval = chip->psy_usb_type;
			break;
		case POWER_SUPPLY_PROP_VOLTAGE_NOW:
			val->intval = sc89601d_get_vbus();
			break;
		default:
			return -EINVAL;
	}

	return 0;
}

static int sc89601d_get_vbus_stat(struct sc89601d_chip *chip, u8 *val)
{
	return sc89601d_read_interface(chip->client, SC89601D_CON8,
			val, CON8_VBUS_STAT_MASK, CON8_VBUS_STAT_SHIFT);
}

static int sc89601d_get_vbus_gd(struct sc89601d_chip *chip, u8 *val)
{
	return sc89601d_read_interface(chip->client, SC89601D_CON10,
			val, CON10_VBUS_GD_MASK, CON10_VBUS_GD_SHIFT);
}

static int sc89601d_get_input_det_done(struct sc89601d_chip *chip, u8 *val)
{
	return sc89601d_read_interface(chip->client, SC89601D_CON14,
			val, CON14_INPUT_DET_DONE_MASK, CON14_INPUT_DET_DONE_SHIFT);
}

static void sc89601d_parse_vbus_stat(struct sc89601d_chip *chip, u8 vbus_stat)
{
	switch (vbus_stat) {
		case CON8_VBUS_TYPE_NONE:
		case CON8_VBUS_TYPE_UNKNOWN:
			chip->psy_desc.type = POWER_SUPPLY_TYPE_UNKNOWN;
			chip->psy_usb_type = POWER_SUPPLY_USB_TYPE_UNKNOWN;
			break;
		case CON8_VBUS_TYPE_SDP:
			chip->psy_desc.type = POWER_SUPPLY_TYPE_USB;
			chip->psy_usb_type = POWER_SUPPLY_USB_TYPE_SDP;
			break;
		case CON8_VBUS_TYPE_CDP:
			chip->psy_desc.type = POWER_SUPPLY_TYPE_USB_CDP;
			chip->psy_usb_type = POWER_SUPPLY_USB_TYPE_CDP;
			break;
		case CON8_VBUS_TYPE_DCP:
		case CON8_VBUS_TYPE_HVDCP:
		case CON8_VBUS_TYPE_NON_STD:
			chip->psy_desc.type = POWER_SUPPLY_TYPE_USB_DCP;
			chip->psy_usb_type = POWER_SUPPLY_USB_TYPE_DCP;
			break;
		default:
			chip->psy_desc.type = POWER_SUPPLY_TYPE_UNKNOWN;
			chip->psy_usb_type = POWER_SUPPLY_USB_TYPE_UNKNOWN;
			break;
	}
}

static void sc89601d_parse_chg_stat(struct sc89601d_chip *chip, u8 chg_stat)
{
	switch(chg_stat) {
		case CON8_CHRG_STAT_NOT:
			chip->chg_stat = POWER_SUPPLY_STATUS_NOT_CHARGING;
			break;
		case CON8_CHRG_STAT_PRE:
		case CON8_CHRG_STAT_FAST:
			chip->chg_stat = POWER_SUPPLY_STATUS_CHARGING;
			break;
		case CON8_CHRG_STAT_TERM:
			chip->chg_stat = POWER_SUPPLY_STATUS_FULL;
			break;
		default:
			chip->chg_stat = POWER_SUPPLY_STATUS_UNKNOWN;
			break;
	}
}

static void do_bc12(struct sc89601d_chip *chip)
{
	int ret, i;
	u8 vbus_gd, vbus_stat, chg_stat, input_det_done = 0;

	sc89601d_dbg("start %s\n", __func__);

	for (i=1; i<=30; i++) {
		ret = sc89601d_get_input_det_done(chip, &input_det_done);
		if (ret < 0) {
			sc89601d_dbg("%s sc89601d_get_input_det_done err\n", __func__);
			return;
		}
		sc89601d_dbg("%s, input_det_done:%d, retry:%d\n", __func__, input_det_done, i);
		if (input_det_done == 1)
			break;
		msleep(100);
	}

	if (input_det_done == 0)
		goto err;

	for (i=1; i<30; i++) {
		ret = sc89601d_get_vbus_gd(chip, &vbus_gd);
		if (ret < 0) {
			sc89601d_dbg("%s sc89601d_get_vbus_gd err\n", __func__);
			return;
		}

		ret = sc89601d_get_vbus_stat(chip, &vbus_stat);
		if (ret < 0) {
			sc89601d_dbg("%s sc89601d_get_vbus_stat err\n", __func__);
			return;
		}

		ret = sc89601d_get_chrg_stat(chip, &chg_stat);
		if (ret < 0) {
			sc89601d_dbg("%s sc89601d_get_chrg_stat err\n", __func__);
			return;
		}

		sc89601d_dbg("%s, vbus_gd = %d, vbus_stat = 0x%x, chg_stat = 0x%x, retry:%d\n",
				__func__, vbus_gd, vbus_stat, chg_stat, i);

		if ((vbus_gd == 1) && (vbus_stat != 0) && (chg_stat != 0)) {
			sc89601d_dbg("%s, adapter plug in\n", __func__);
			chip->power_good = vbus_gd;
			sc89601d_parse_vbus_stat(chip, vbus_stat);
			sc89601d_parse_chg_stat(chip, chg_stat);
			power_supply_changed(chip->psy);
			break;
		}
		msleep(100);
	}

	if (chip->power_good == 0)
		goto err;

	return;

err:
	sc89601d_dbg("%s, power is not good\n", __func__);
	chip->power_good = 0;
	chip->psy_desc.type = POWER_SUPPLY_TYPE_UNKNOWN;
	chip->psy_usb_type = POWER_SUPPLY_USB_TYPE_UNKNOWN;
	chip->chg_stat = POWER_SUPPLY_STATUS_DISCHARGING;
	power_supply_changed(chip->psy);
}

static int sc89601d_set_property(struct power_supply *psy,
		enum power_supply_property psp,
		const union power_supply_propval *val)
{
	struct sc89601d_chip *chip = power_supply_get_drvdata(psy);

	switch (psp) {
		case POWER_SUPPLY_PROP_ONLINE:
			if (val->intval == 1) {
				do_bc12(chip);
			} else if (val->intval == 0) {
				chip->power_good = 0;
				chip->psy_desc.type = POWER_SUPPLY_TYPE_UNKNOWN;
				chip->psy_usb_type = POWER_SUPPLY_USB_TYPE_UNKNOWN;
				chip->chg_stat = POWER_SUPPLY_STATUS_DISCHARGING;
				power_supply_changed(chip->psy);
			} else
				return -EINVAL;
			break;
		default:
			return -EINVAL;
	}

	return 0;
}

static int sc89601d_property_is_writeable(struct power_supply *psy,
		enum power_supply_property psp)
{
	switch (psp) {
		case POWER_SUPPLY_PROP_ONLINE:
			return 1;
		default:
			return 0;
	}
}

static const struct power_supply_desc sc89601d_psy_desc = {
	.name = "mtk_charger_type",
	.type = POWER_SUPPLY_TYPE_USB,
	.usb_types = sc89601d_psy_usb_types,
	.num_usb_types = ARRAY_SIZE(sc89601d_psy_usb_types),
	.properties = sc89601d_psy_properties,
	.num_properties = ARRAY_SIZE(sc89601d_psy_properties),
	.get_property = sc89601d_get_property,
	.set_property = sc89601d_set_property,
	.property_is_writeable = sc89601d_property_is_writeable,
};

static int sc89601d_init_psy(struct sc89601d_chip *chip)
{
	struct power_supply_config cfg = {
		.drv_data = chip,
		.of_node = chip->dev->of_node,
		.supplied_to = sc89601d_psy_supplied_to,
		.num_supplicants = ARRAY_SIZE(sc89601d_psy_supplied_to),
	};

	memcpy(&chip->psy_desc, &sc89601d_psy_desc, sizeof(chip->psy_desc));
	chip->psy = devm_power_supply_register(chip->dev, &chip->psy_desc, &cfg);

	return IS_ERR(chip->psy) ? PTR_ERR(chip->psy) : 0;
}
#endif

static int sc89601d_init_regulator(struct sc89601d_chip *chip)
{
	struct regulator_config cfg = {
		.dev = chip->dev,
		.driver_data = chip,
	};

	memcpy(&chip->rdesc, &sc89601d_otg_rdesc, sizeof(chip->rdesc));
	chip->rdesc.name = dev_name(chip->dev);
	chip->rdev = devm_regulator_register(chip->dev, &chip->rdesc, &cfg);
	return IS_ERR(chip->rdev) ? PTR_ERR(chip->rdev) : 0;
}

static int sc89601d_init_chg_dev(struct sc89601d_chip *chip)
{
	chip->chg_dev = charger_device_register(chip->chg_name, chip->dev,
			chip, &sc89601d_chg_ops,
			&chip->chg_props);
	return IS_ERR(chip->chg_dev) ? PTR_ERR(chip->chg_dev) : 0;
}

static int sc89601d_probe(struct i2c_client *client,
		const struct i2c_device_id *id)
{
	int ret;
	struct sc89601d_chip *chip;
	struct device *dev = &client->dev;

	sc89601d_dbg("[%s]\n", __func__);

	chip = devm_kzalloc(dev, sizeof(*chip), GFP_KERNEL);
	if (!chip)
		return -ENOMEM;

	chip->client = client;
	chip->dev = dev;
	i2c_set_clientdata(client, chip);

	if (!sc89601d_check_devinfo(chip)) {
		return -ENODEV;
	}

	ret = sc89601d_parse_dts(chip);
	if (ret < 0) {
		sc89601d_dbg("%s parse dt fail(%d)\n", __func__, ret);
		return ret;
	}

	mutex_init(&chip->i2c_lock);

	ret = sc89601d_reset_reg(chip);
	if (ret < 0)
		goto out;

	ret = sc89601d_init_setting(chip);
	if (ret < 0)
		goto out;

#ifdef MASTER_CHARGER_NAME
	ret = sc89601d_init_psy(chip);
	if (ret < 0) {
		sc89601d_dbg("%s failed to init power supply\n", __func__);
		goto out;
	}
#endif

	ret = sc89601d_init_regulator(chip);
	if (ret < 0) {
		sc89601d_dbg("%s, failed to init regulator\n", __func__);
		goto out;
	}

	ret = sc89601d_init_chg_dev(chip);
	if (ret < 0) {
		sc89601d_dbg("%s failed to init chg_dev\n", __func__);
		goto out;
	}

	ret = device_create_file(dev, &dev_attr_dump_reg);
	if (ret < 0) {
		sc89601d_dbg("%s create file fail(%d)\n",
				__func__, ret);
		goto create_file_err;
	}

	sc89601d_dump_register(chip->chg_dev);
	sc89601d_dbg("%s successfully\n", __func__);

	return ret;

create_file_err:
	charger_device_unregister(chip->chg_dev);
out:
	mutex_destroy(&chip->i2c_lock);
	return ret;
}

static int sc89601d_remove(struct i2c_client *client)
{
	struct sc89601d_chip *chip = i2c_get_clientdata(client);

	sc89601d_dbg("%s\n", __func__);

	if (chip) {
		device_remove_file(chip->dev, &dev_attr_dump_reg);
		charger_device_unregister(chip->chg_dev);
		mutex_destroy(&chip->i2c_lock);
	}

	return 0;
}

static const struct of_device_id sc89601d_of_match[] = {
#ifdef MASTER_CHARGER_NAME
	{.compatible = "sc,sc89601d_master"},
#else
	{.compatible = "sc,sc89601d_slave"},
#endif
	{},
};

static const struct i2c_device_id sc89601d_i2c_id[] = {
#ifdef MASTER_CHARGER_NAME
	{MASTER_CHARGER_NAME, 0},
#else
	{SLAVE_CHARGER_NAME, 0},
#endif
	{}
};
MODULE_DEVICE_TABLE(i2c, sc89601d_i2c_id);

static struct i2c_driver sc89601d_i2c_driver = {
	.driver = {
#ifdef MASTER_CHARGER_NAME
		.name = MASTER_CHARGER_NAME,
#else
		.name = SLAVE_CHARGER_NAME,
#endif
		.owner = THIS_MODULE,
		.of_match_table = sc89601d_of_match,
	},
	.probe = sc89601d_probe,
	.remove = sc89601d_remove,
	.id_table = sc89601d_i2c_id,
};
module_i2c_driver(sc89601d_i2c_driver);

MODULE_AUTHOR("JiangWei Huang <huangjiangwei@boe.com.cn>");
MODULE_DESCRIPTION("SC89601D Charger Driver");
MODULE_LICENSE("GPL");
