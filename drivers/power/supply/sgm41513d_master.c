/* SPDX-License-Identifier: GPL-2.0
 *
 * Copyright (c) 2023 BOE Inc.
 */

#include "sgm41513d.h"

#define MASTER_CHARGER_NAME "sgm41513d_master"
//#define SLAVE_CHARGER_NAME "sgm41513d_slave"

#ifdef MASTER_CHARGER_NAME
#define sgm41513d_dbg(x,y...) printk ("sgm41513d_master: " x, ## y)
#else
#define sgm41513d_dbg(x,y...) printk ("sgm41513d_slave: " x, ## y)
#endif

//CON2
static const u32 SGM41513D_ICHG_TABLE[] = {
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

//CON3
static const u32 SGM41513D_ITERM_TABLE[] = {
	5000,   10000,  15000,  20000,  30000,
	40000,  50000,  60000,  80000,  100000,
	120000, 140000, 160000, 180000, 200000,
	240000
};

static int sgm41513d_read_byte(struct i2c_client *client, u8 cmd)
{
	s32 ret;
	struct sgm41513d_chip *chip = i2c_get_clientdata(client);

	mutex_lock(&chip->i2c_lock);
	ret = i2c_smbus_read_byte_data(client, cmd);
	mutex_unlock(&chip->i2c_lock);

	return ret;
}

static int sgm41513d_write_byte(struct i2c_client *client, u8 cmd, u8 val)
{
	s32 ret;
	struct sgm41513d_chip *chip = i2c_get_clientdata(client);

	mutex_lock(&chip->i2c_lock);
	ret = i2c_smbus_write_byte_data(client, cmd, val);
	mutex_unlock(&chip->i2c_lock);
	return ret;
}

static int sgm41513d_read_interface(struct i2c_client *client, u8 reg, u8 *val, u8 mask, u8 shift)
{
	int ret = 0;

	ret = sgm41513d_read_byte(client, reg);
	if (ret < 0)
		return ret;

	ret &= (mask << shift);
	*val = (ret >> shift);

	return 0;
}

static int sgm41513d_write_interface(struct i2c_client *client, u8 reg, u8 val, u8 mask, u8 shift)
{
	int ret = 0;
	u8 read_reg = 0;

	ret = sgm41513d_read_byte(client, reg);
	if (ret < 0)
		return -EIO;

	read_reg = ret;
	ret &= ~(mask << shift);
	ret |= (val << shift);

	return sgm41513d_write_byte(client, reg, ret);
}

/* CON0---------------------------------------------------- */
static int sgm41513d_set_en_hiz(struct sgm41513d_chip *chip, u8 val)
{
	int ret = 0;

	sgm41513d_dbg("%s, val = 0x%x\n", __func__, val);
	ret = sgm41513d_write_interface(chip->client, (u8) (SGM41513D_CON0),
			(u8) (val),
			(u8) (CON0_EN_HIZ_MASK),
			(u8) (CON0_EN_HIZ_SHIFT)
			);
	if (ret < 0)
		sgm41513d_dbg("%s fail\n", __func__);

	return ret;
}

static int sgm41513d_get_en_hiz(struct sgm41513d_chip *chip, u8 *val)
{
	return sgm41513d_read_interface(chip->client, (u8) (SGM41513D_CON0),
			(val),
			(u8) (CON0_EN_HIZ_MASK),
			(u8) (CON0_EN_HIZ_SHIFT)
			);
}

static int sgm41513d_set_aicr(struct sgm41513d_chip *chip, u8 val)
{
	int ret = 0;

	sgm41513d_dbg("%s, val = 0x%x\n", __func__, val);
	ret = sgm41513d_write_interface(chip->client, (u8) (SGM41513D_CON0),
			(u8) (val),
			(u8) (CON0_IINLIM_MASK),
			(u8) (CON0_IINLIM_SHIFT)
			);
	if (ret < 0)
		sgm41513d_dbg("%s fail\n", __func__);

	return ret;
}

static int sgm41513d_get_aicr(struct sgm41513d_chip *chip, u8 *val)
{
	return sgm41513d_read_interface(chip->client, (u8) (SGM41513D_CON0),
			(val),
			(u8) (CON0_IINLIM_MASK),
			(u8) (CON0_IINLIM_SHIFT)
			);
}

static int sgm41513d_set_stat_ctrl(struct sgm41513d_chip *chip, u8 val)
{
	int ret = 0;

	sgm41513d_dbg("%s, val = 0x%x\n", __func__, val);
	ret = sgm41513d_write_interface(chip->client, (u8) (SGM41513D_CON0),
			(u8) (val),
			(u8) (CON0_STAT_IMON_CTRL_MASK),
			(u8) (CON0_STAT_IMON_CTRL_SHIFT)
			);
	if (ret < 0)
		sgm41513d_dbg("%s fail\n", __func__);

	return ret;
}

/* CON1---------------------------------------------------- */
static int sgm41513d_set_otg_config(struct sgm41513d_chip *chip, u8 val)
{
	int ret = 0;

	sgm41513d_dbg("%s, val = 0x%x\n", __func__, val);
	ret = sgm41513d_write_interface(chip->client, (u8) (SGM41513D_CON1),
			(u8) (val),
			(u8) (CON1_OTG_CONFIG_MASK),
			(u8) (CON1_OTG_CONFIG_SHIFT)
			);
	if (ret < 0)
		sgm41513d_dbg("%s fail\n", __func__);

	return ret;
}

static int sgm41513d_get_otg_config(struct sgm41513d_chip *chip, u8 *val)
{
	return sgm41513d_read_interface(chip->client, (u8) (SGM41513D_CON1),
			(val),
			(u8) (CON1_OTG_CONFIG_MASK),
			(u8) (CON1_OTG_CONFIG_SHIFT)
			);
}

static int sgm41513d_set_chg_config(struct sgm41513d_chip *chip, u8 val)
{
	int ret = 0;

	sgm41513d_dbg("%s, val = 0x%x\n", __func__, val);

	ret = sgm41513d_write_interface(chip->client, (u8) (SGM41513D_CON1),
			(u8) (val),
			(u8) (CON1_CHG_CONFIG_MASK),
			(u8) (CON1_CHG_CONFIG_SHIFT)
			);
	if (ret < 0)
		sgm41513d_dbg("%s fail\n", __func__);

	return ret;
}

static int sgm41513d_get_chg_config(struct sgm41513d_chip *chip, u8 *val)
{
	return sgm41513d_read_interface(chip->client, (u8) (SGM41513D_CON1),
			(val),
			(u8) (CON1_CHG_CONFIG_MASK),
			(u8) (CON1_CHG_CONFIG_SHIFT)
			);
}

/* CON2---------------------------------------------------- */
static int sgm41513d_set_ichg(struct sgm41513d_chip *chip, u8 val)
{
	int ret = 0;

	sgm41513d_dbg("%s, val = 0x%x\n", __func__, val);
	ret = sgm41513d_write_interface(chip->client, (u8) (SGM41513D_CON2),
			(u8) (val),
			(u8) (CON2_ICHG_MASK),
			(u8) (CON2_ICHG_SHIFT)
			);
	if (ret < 0)
		sgm41513d_dbg("%s fail\n", __func__);

	return ret;
}

static int sgm41513d_get_ichg(struct sgm41513d_chip *chip, u8 *val)
{
	return sgm41513d_read_interface(chip->client, (u8) (SGM41513D_CON2),
			(val),
			(u8) (CON2_ICHG_MASK),
			(u8) (CON2_ICHG_SHIFT)
			);
}

/* CON3---------------------------------------------------- */
static int sgm41513d_set_iterm(struct sgm41513d_chip *chip, u8 val)
{
	int ret = 0;

	sgm41513d_dbg("%s, val = 0x%x\n", __func__, val);
	ret = sgm41513d_write_interface(chip->client, (u8) (SGM41513D_CON3),
			(u8) (val),
			(u8) (CON3_ITERM_MASK),
			(u8) (CON3_ITERM_SHIFT)
			);
	if (ret < 0)
		sgm41513d_dbg("%s fail\n", __func__);

	return ret;
}

/* CON4---------------------------------------------------- */
static int sgm41513d_set_vreg(struct sgm41513d_chip *chip, u8 val)
{
	int ret = 0;

	sgm41513d_dbg("%s, val = 0x%x\n", __func__, val);
	ret = sgm41513d_write_interface(chip->client, (u8) (SGM41513D_CON4),
			(u8) (val),
			(u8) (CON4_VREG_MASK),
			(u8) (CON4_VREG_SHIFT)
			);
	if (ret < 0)
		sgm41513d_dbg("%s fail\n", __func__);

	return ret;
}

/* CON5---------------------------------------------------- */
static int sgm41513d_set_watchdog(struct sgm41513d_chip *chip, u8 val)
{
	int ret = 0;

	sgm41513d_dbg("%s, val = 0x%x\n", __func__, val);
	ret = sgm41513d_write_interface(chip->client, (u8) (SGM41513D_CON5),
			(u8) (val),
			(u8) (CON5_WATCHDOG_MASK),
			(u8) (CON5_WATCHDOG_SHIFT)
			);
	if (ret < 0)
		sgm41513d_dbg("%s fail\n", __func__);

	return ret;
}

static int sgm41513d_en_timer(struct sgm41513d_chip *chip, u8 val)
{
	int ret = 0;

	sgm41513d_dbg("%s, val = 0x%x\n", __func__, val);
	ret = sgm41513d_write_interface(chip->client, SGM41513D_CON5, val, CON5_EN_TIMER_MASK, CON5_EN_TIMER_SHIFT);
	if (ret < 0)
		sgm41513d_dbg("%s fail\n", __func__);

	return ret;
}

/* CON6---------------------------------------------------- */
static int sgm41513d_set_vindpm(struct sgm41513d_chip *chip, u8 val)
{
	int ret = 0;

	sgm41513d_dbg("%s, val = 0x%x\n", __func__, val);
	ret = sgm41513d_write_interface(chip->client, (u8) (SGM41513D_CON6),
			(u8) (val),
			(u8) (CON6_VINDPM_MASK),
			(u8) (CON6_VINDPM_SHIFT)
			);
	if (ret < 0)
		sgm41513d_dbg("%s fail\n", __func__);

	return ret;
}

static int sgm41513d_get_vindpm(struct sgm41513d_chip *chip, u8 *val)
{
	return sgm41513d_read_interface(chip->client, (u8) (SGM41513D_CON6),
			(val),
			(u8) (CON6_VINDPM_MASK),
			(u8) (CON6_VINDPM_SHIFT)
			);
}

static int sgm41513d_set_ovp(struct sgm41513d_chip *chip, u8 val)
{
	int ret = 0;

	sgm41513d_dbg("%s, val = 0x%x\n", __func__, val);
	ret = sgm41513d_write_interface(chip->client, (u8) (SGM41513D_CON6),
			(u8) (val),
			(u8) (CON6_OVP_MASK),
			(u8) (CON6_OVP_SHIFT)
			);
	if (ret < 0)
		sgm41513d_dbg("%s fail\n", __func__);

	return ret;
}

static int sgm41513d_set_vindpm_os(struct sgm41513d_chip *chip, u8 val)
{
	int ret = 0;

	sgm41513d_dbg("%s, val = 0x%x\n", __func__, val);
	ret = sgm41513d_write_interface(chip->client, (u8) (SGM41513D_CON15),
			(u8) (val),
			(u8) (CON15_VINDPM_OS_MASK),
			(u8) (CON15_VINDPM_OS_SHIFT)
			);
	if (ret < 0)
		sgm41513d_dbg("%s fail\n", __func__);

	return ret;
}

static int sgm41513d_get_vindpm_os(struct sgm41513d_chip *chip, u8 *val)
{
	return sgm41513d_read_interface(chip->client, SGM41513D_CON15,
			val, CON15_VINDPM_OS_MASK, CON15_VINDPM_OS_SHIFT);
}

/* CON7---------------------------------------------------- */
static int sgm41513d_set_batfet_reset_enable(struct sgm41513d_chip *chip, u8 val)
{
	int ret = 0;

	sgm41513d_dbg("%s, val = 0x%x\n", __func__, val);
	ret = sgm41513d_write_interface(chip->client, (u8) (SGM41513D_CON7),
			(u8) (val),
			(u8) (CON7_BATFET_RST_EN_MASK),
			(u8) (CON7_BATFET_RST_EN_SHIFT)
			);
	if (ret < 0)
		sgm41513d_dbg("%s fail\n", __func__);

	return ret;
}

static int sgm41513d_set_batfet_dly(struct sgm41513d_chip *chip, u8 val)
{
	int ret = 0;

	sgm41513d_dbg("%s, val = 0x%x\n", __func__, val);
	ret = sgm41513d_write_interface(chip->client, (u8) (SGM41513D_CON7),
			(u8) (val),
			(u8) (CON7_BATFET_DLY_MASK),
			(u8) (CON7_BATFET_DLY_SHIFT)
			);
	if (ret < 0)
		sgm41513d_dbg("%s fail\n", __func__);

	return ret;
}

#ifdef SLAVE_CHARGER_NAME
static int sgm41513d_set_batfet_dis(struct sgm41513d_chip *chip, u8 val)
{
	int ret = 0;

	sgm41513d_dbg("%s, val = 0x%x\n", __func__, val);
	ret = sgm41513d_write_interface(chip->client, (u8) (SGM41513D_CON7),
			(u8) (val),
			(u8) (CON7_BATFET_DISABLE_MASK),
			(u8) (CON7_BATFET_DISABLE_SHIFT)
			);
	if (ret < 0)
		sgm41513d_dbg("%s fail\n", __func__);

	return ret;
}
#endif

/* CON8---------------------------------------------------- */
static int sgm41513d_get_chrg_stat(struct sgm41513d_chip *chip, u8 *val)
{
	return sgm41513d_read_interface(chip->client, (u8) (SGM41513D_CON8),
			(val),
			(u8) (CON8_CHRG_STAT_MASK),
			(u8) (CON8_CHRG_STAT_SHIFT)
			);
}

/*CON10----------------------------------------------------------*/
static int sgm41513d_get_vindpm_stat(struct sgm41513d_chip *chip, u8 *val)
{
	return sgm41513d_read_interface(chip->client, (u8) (SGM41513D_CON10),
			(val),
			(u8) (CON10_VINDPM_STAT_MASK),
			(u8) (CON10_VINDPM_STAT_SHIFT)
			);
}

/* CON11---------------------------------------------------- */
static bool sgm41513d_check_devinfo(struct sgm41513d_chip *chip)
{
	int ret;
	u8 chip_id;

	ret = i2c_smbus_read_byte_data(chip->client, SGM41513D_CON11);
	if (ret < 0) {
		sgm41513d_dbg("%s get devchip fail(%d)\n",
				__func__, ret);
		return false;
	}

	ret &= (CON11_PN_MASK << CON11_PN_SHIFT);
	chip_id = (ret >> CON11_PN_SHIFT);

	sgm41513d_dbg("%s reg11:0x%x, chip_id = 0x%x\n", __func__, ret, chip_id);

	return true;
}

static int sgm41513d_reset_reg(struct sgm41513d_chip *chip)
{
	int ret = 0;

	sgm41513d_dbg("%s\n", __func__);
	ret = sgm41513d_write_interface(chip->client, (u8) (SGM41513D_CON11),
			(u8) (CON11_REG_RST_VALUE),
			(u8) (CON11_REG_RST_MASK),
			(u8) (CON11_REG_RST_SHIFT)
			);
	if (ret < 0)
		sgm41513d_dbg("%s fail\n", __func__);

	return ret;
}

/* CON14---------------------------------------------------- */
static void convert_to_reg(const u32 *table, int table_size, u32 raw_val, u8 *reg_val)
{
	int i;

	if(raw_val > table[table_size - 1]) {
		*reg_val = (table_size - 1);
		sgm41513d_dbg("raw_val = %d, out of [%d-%d], default use %d\n",
				raw_val, table[0], table[table_size - 1], table[table_size - 1]);
		return;
	}

	if(raw_val < table[0]) {
		*reg_val = 0;
		sgm41513d_dbg("raw_val = %d, out of [%d-%d], default use %d\n",
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

	sgm41513d_dbg("%s fail\n", __func__);
}

static int sgm41513d_set_charging_current(struct charger_device *chg_dev, u32 ichg)
{
	u8 reg_val;
	struct sgm41513d_chip *chip = dev_get_drvdata(&chg_dev->dev);

	ichg = 3000000;
	convert_to_reg(SGM41513D_ICHG_TABLE, ARRAY_SIZE(SGM41513D_ICHG_TABLE), ichg, &reg_val);

	sgm41513d_dbg("%s, ichg:%duA, reg_val:0x%x\n", __func__, ichg, reg_val);

	return sgm41513d_set_ichg(chip, reg_val);
}

static int sgm41513d_get_min_charging_current(struct charger_device *chg_dev, u32 *min_ichg)
{
	*min_ichg = SGM41513D_ICHG_TABLE[0];
	sgm41513d_dbg("%s, min_ichg:%duA\n", __func__, *min_ichg);
	return 0;
}

static int sgm41513d_get_input_current(struct charger_device *chg_dev, u32 *aicr)
{
	int ret = 0;
	u8 reg_val = 0;
	struct sgm41513d_chip *chip = dev_get_drvdata(&chg_dev->dev);

	ret = sgm41513d_get_aicr(chip, &reg_val);
	if (ret < 0)
		return ret;

	if (CON0_IINLIM_MASK == (reg_val & CON0_IINLIM_MASK))
		*aicr = 3200000;
	else
		*aicr = (reg_val & CON0_IINLIM_MASK) * 100000 + 100000;

	sgm41513d_dbg("%s, reg_val:0x%x, aicr:%duA\n", __func__, reg_val, *aicr);

	return ret;
}

static int sgm41513d_set_input_current(struct charger_device *chg_dev, u32 aicr)
{
	u8 reg_val;
	struct sgm41513d_chip *chip = dev_get_drvdata(&chg_dev->dev);

	reg_val = (aicr - 100000) / 100000;

	sgm41513d_dbg("%s, aicr:%duA, reg_val:0x%x\n", __func__, aicr, reg_val);

	return sgm41513d_set_aicr(chip, reg_val);
}

static int sgm41513d_get_min_input_current(struct charger_device *chg_dev, u32 *min_aicr)
{
	*min_aicr = 100000;
	sgm41513d_dbg("%s, min_aicr:%duA\n", __func__, *min_aicr);
	return 0;
}

static int sgm41513d_set_eoc_current(struct charger_device *chg_dev, u32 ieoc)
{
	u8 reg_val;
	struct sgm41513d_chip *chip = dev_get_drvdata(&chg_dev->dev);

	convert_to_reg(SGM41513D_ITERM_TABLE, ARRAY_SIZE(SGM41513D_ITERM_TABLE), ieoc, &reg_val);

	sgm41513d_dbg("%s, ieoc:%duA, reg_val:0x%x\n", __func__, ieoc, reg_val);

	return sgm41513d_set_iterm(chip, reg_val);
}

static int sgm41513d_event(struct charger_device *chg_dev, u32 event, u32 args)
{
	sgm41513d_dbg("%s event = %d\n", __func__, event);

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

static int sgm41513d_set_constant_voltage(struct charger_device *chg_dev, u32 cv_uv)
{
	u8 reg_val;
	struct sgm41513d_chip *chip = dev_get_drvdata(&chg_dev->dev);

	reg_val = (cv_uv - 3856000) / 1000 / 32;

	sgm41513d_dbg("%s, cv_uv:%duV, reg_val:0x%x\n", __func__, cv_uv, reg_val);

	return sgm41513d_set_vreg(chip, reg_val);
}

static int sgm41513d_enable(struct charger_device *chg_dev, bool en)
{
	int ret;
	struct sgm41513d_chip *chip = dev_get_drvdata(&chg_dev->dev);

	if (chip->disable_charge)
		en = false;

	sgm41513d_dbg("%s, en = %d\n", __func__, en);

#ifdef SLAVE_CHARGER_NAME
	ret = sgm41513d_set_batfet_dis(chip, !en);
#endif
	ret = sgm41513d_set_chg_config(chip, en);

	return ret;
}

static int sgm41513d_set_mivr(struct charger_device *chg_dev, u32 vindpm_uv)
{
	int ret;
	u8 reg_val, os_val;
	u32 offset;
	struct sgm41513d_chip *chip = dev_get_drvdata(&chg_dev->dev);

	if (vindpm_uv < 3900000 || vindpm_uv > 12000000)
		return -EINVAL;

	if (vindpm_uv < 5900000) {
		os_val = 0;
		offset = 3900000;
	} else if (vindpm_uv >= 5900000 && vindpm_uv < 7500000) {
		os_val = 1;
		offset = 5900000;
	} else if (vindpm_uv >= 7500000 && vindpm_uv < 10500000) {
		os_val = 2;
		offset = 7500000;
	} else {
		os_val = 3;
		offset = 10500000;
	}

	ret = sgm41513d_set_vindpm_os(chip, os_val);
	if (ret < 0)
		return ret;

	reg_val = (vindpm_uv - offset) / 100000;

	sgm41513d_dbg("%s, vindpm_uv = %duV, reg_val = 0x%x\n", __func__, vindpm_uv, reg_val);
	return sgm41513d_set_vindpm(chip, reg_val);
}

static int sgm41513d_get_mivr(struct charger_device *chg_dev, u32 *mivr)
{
	int ret;
	u32 offset = 0;
	u8 reg_vindpm, reg_vindpm_os;
	struct sgm41513d_chip *chip = dev_get_drvdata(&chg_dev->dev);

	ret = sgm41513d_get_vindpm(chip, &reg_vindpm);
	if (ret < 0)
		return ret;

	ret = sgm41513d_get_vindpm_os(chip, &reg_vindpm_os);
	if (ret < 0)
		return ret;

	if (reg_vindpm_os == 0)
		offset = 3900000;
	else if (reg_vindpm_os == 1)
		offset = 5900000;
	else if (reg_vindpm_os == 2)
		offset = 7500000;
	else if (reg_vindpm_os == 3)
		offset = 10500000;

	*mivr = reg_vindpm * 100000 + offset;

	sgm41513d_dbg("%s, reg_vindpm:0x%x, reg_vindpm_os:0x%x, mivr:%duV\n", __func__, reg_vindpm, *mivr, reg_vindpm_os);
	return ret;
}

static int sgm41513d_get_mivr_state(struct charger_device *chg_dev, bool *in_loop)
{
	int ret;
	u8 reg_val;
	struct sgm41513d_chip *chip = dev_get_drvdata(&chg_dev->dev);

	ret = sgm41513d_get_vindpm_stat(chip, &reg_val);
	if (ret < 0)
		return ret;

	if (reg_val == 1)
		*in_loop = true;
	else
		*in_loop = false;
	sgm41513d_dbg("%s, reg_val:0x%x, mivr_state:%d\n", __func__, reg_val, *in_loop);

	return ret;
}

static int sgm41513d_is_charging_done(struct charger_device *chg_dev, bool *is_done)
{
	int ret;
	u8 reg_val;
	struct sgm41513d_chip *chip = dev_get_drvdata(&chg_dev->dev);

	ret = sgm41513d_get_chrg_stat(chip, &reg_val);
	if (ret < 0)
		return ret;

	if (reg_val == CHARGING_TERMINATED)
		*is_done = true;
	else
		*is_done = false;
	sgm41513d_dbg("%s, is_done:%d\n", __func__, *is_done);

	return ret;
}

static int sgm41513d_enable_discharge(struct charger_device *chg_dev, bool en)
{
	sgm41513d_dbg("%s en = %d\n", __func__, en);
	return sgm41513d_enable(chg_dev, !en);
}

static int sgm41513d_otg_is_enabled(struct regulator_dev *rdev)
{
	struct sgm41513d_chip *chip = rdev_get_drvdata(rdev);
	u8 otg_on, hiz_on;

	if (sgm41513d_get_en_hiz(chip, &hiz_on) < 0)
		return -EIO;

	if (sgm41513d_get_otg_config(chip, &otg_on) < 0)
		return -EIO;
	sgm41513d_dbg("%s, hiz_on:%d, otg_on:%d\n", __func__, hiz_on, otg_on);

	if (hiz_on == 0 && otg_on == 1)
		return 1;

	return 0;
}

static int sgm41513d_enable_otg(struct regulator_dev *rdev)
{
	struct sgm41513d_chip *chip = rdev_get_drvdata(rdev);

	sgm41513d_dbg("%s\n", __func__);
	if (sgm41513d_set_en_hiz(chip, 0) < 0)
		return -EIO;
	return sgm41513d_set_otg_config(chip, 1);
}

static int sgm41513d_disable_otg(struct regulator_dev *rdev)
{
	struct sgm41513d_chip *chip = rdev_get_drvdata(rdev);

	sgm41513d_dbg("%s\n", __func__);
	if (sgm41513d_set_en_hiz(chip, 1) < 0)
		return -EIO;
	return sgm41513d_set_otg_config(chip, 0);
}

static int sgm41513d_parse_dts(struct sgm41513d_chip *chip)
{
	int ret = 0;
	struct device_node *np = chip->dev->of_node;

	if (!np) {
		sgm41513d_dbg("%s no device node\n", __func__);
		return -EINVAL;
	}

	if (of_property_read_string(np, "charger_name",
				&chip->chg_name) < 0) {
#ifdef MASTER_CHARGER_NAME
		chip->chg_name = "primary_chg";
#else
		chip->chg_name = "secondary_chg";
#endif
		sgm41513d_dbg("%s: no charger name\n", __func__);
	}

	if (of_property_read_string(np, "alias_name",
				&(chip->chg_props.alias_name)) < 0) {
#ifdef MASTER_CHARGER_NAME
		chip->chg_props.alias_name = MASTER_CHARGER_NAME;
#else
		chip->chg_props.alias_name = SLAVE_CHARGER_NAME;
#endif
		sgm41513d_dbg("%s: no alias name\n", __func__);
	}

#ifdef SLAVE_CHARGER_NAME
	chip->gpio_slave_en = of_get_named_gpio(np, "sc,gpio_slave_en", 0);
	if (!gpio_is_valid(chip->gpio_slave_en))
		sgm41513d_dbg("%s, get gpio failed\n", __func__);

	ret = gpio_request(chip->gpio_slave_en, "gpio_slave_en");
	if (ret)
		sgm41513d_dbg("%s, request gpio fail\n", __func__);
#endif

	sgm41513d_dbg("charger_name = %s, alias_name = %s\n", chip->chg_name, chip->chg_props.alias_name);

	return ret;
}

static int sgm41513d_is_enable(struct charger_device *chg_dev, bool *en)
{
	int ret;
	u8 val;
	struct sgm41513d_chip *chip = dev_get_drvdata(&chg_dev->dev);

	ret = sgm41513d_get_chg_config(chip, &val);
	if (ret < 0)
		return ret;

	if(val == 1)
		*en = true;
	else
		*en = false;
	sgm41513d_dbg("%s, is_enable:%d\n", __func__, *en);

	return ret;
}

static int sgm41513d_plug_in(struct charger_device *chg_dev)
{
	sgm41513d_dbg("%s\n", __func__);
	return sgm41513d_enable(chg_dev, true);
}

static int sgm41513d_plug_out(struct charger_device *chg_dev)
{
	sgm41513d_dbg("%s\n", __func__);
	return sgm41513d_enable(chg_dev, false);
}

static const struct regulator_ops sgm41513d_otg_ops = {
	.is_enabled = sgm41513d_otg_is_enabled,
	.enable = sgm41513d_enable_otg,
	.disable = sgm41513d_disable_otg,
};

static const struct regulator_desc sgm41513d_otg_rdesc = {
	.of_match = "usb-otg-vbus",
	.name = "usb-otg-vbus",
	.ops = &sgm41513d_otg_ops,
	.owner = THIS_MODULE,
	.type = REGULATOR_VOLTAGE,
};

static ssize_t dump_reg_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int i = 0, count = 0, reg = 0;
	struct sgm41513d_chip *chip = dev_get_drvdata(dev);

	if (chip == NULL) {
		sgm41513d_dbg("%s chip == NULL\n", __func__);
		return -EINVAL;
	}

	for (i = 0; i <SGM41513D_REG_NUM; i++) {
		reg = sgm41513d_read_byte(chip->client, i);
		if (reg < 0) {
			sgm41513d_dbg("%s, sgm41513d_read_byte fail\n", __func__);
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
	struct sgm41513d_chip *chip = dev_get_drvdata(dev);

	if (chip == NULL) {
		sgm41513d_dbg("%s chip == NULL\n", __func__);
		return -EINVAL;
	}

	if (buf != NULL && size != 0) {
		pvalue = (char *)buf;
		addr = strsep(&pvalue, " ");
		ret = kstrtou32(addr, 16, (u32 *)&reg_address);

		val = strsep(&pvalue, " ");
		ret = kstrtou32(val, 16, (u32 *)&reg_value);

		ret = sgm41513d_write_byte(chip->client, reg_address, reg_value);
		if (ret < 0) {
			sgm41513d_dbg("Failed: reg=%02X, ret=%d\n", (u32)reg_address, ret);
			return -EIO;
		}
	}

	return size;
}

static DEVICE_ATTR_RW(dump_reg);

static int sgm41513d_dump_register(struct charger_device *chg_dev)
{
	int i = 0;
	u8 val[SGM41513D_REG_NUM] = {0};
	struct sgm41513d_chip *chip = dev_get_drvdata(&chg_dev->dev);

	if (chip == NULL) {
		sgm41513d_dbg("%s chip == NULL\n");
		return -EINVAL;
	}

	for (i=0; i<SGM41513D_REG_NUM; i++) {
		val[i] = sgm41513d_read_byte(chip->client, i);
		if (val[i] < 0) {
			sgm41513d_dbg("%s, sgm41513d_read_byte fail\n", __func__);
			return -EIO;
		}
	}

	sgm41513d_dbg("%s, r0:0x%x,r1:0x%x,r2:0x%x,r3:0x%x,r4:0x%x,r5:0x%x,r6:0x%x,r7:0x%x,r8:0x%x,r9:0x%x,ra:0x%x,rb:0x%x,rc:0x%x,rd:0x%x,re:0x%x\n",
			__func__, val[0], val[1], val[2], val[3], val[4], val[5], val[6], val[7], val[8], val[9], val[10], val[11], val[12], val[13], val[14]);

	return 0;
}

static int sgm41513d_enable_chip(struct charger_device *chg_dev, bool en)
{
	struct sgm41513d_chip *chip = dev_get_drvdata(&chg_dev->dev);

	sgm41513d_dbg("%s en = %d\n", __func__, en);
	return sgm41513d_set_en_hiz(chip, en ? 0 : 1);
}

static int sgm41513d_is_chip_enabled(struct charger_device *chg_dev, bool *en)
{
	int ret = 0;
	u8 val;
	struct sgm41513d_chip *chip = dev_get_drvdata(&chg_dev->dev);

	ret = sgm41513d_get_en_hiz(chip, &val);
	if (ret < 0)
		return ret;

	if (val == 0)
		*en = true;
	else
		*en = false;
	sgm41513d_dbg("%s, is_chip_enable:%d\n", __func__, *en);

	return ret;
}

static int sgm41513d_get_charging_current(struct charger_device *chg_dev, u32 *ichg)
{
	int ret = 0;
	u8 reg_val = 0;
	struct sgm41513d_chip *chip = dev_get_drvdata(&chg_dev->dev);

	ret = sgm41513d_get_ichg(chip, &reg_val);
	if (ret < 0)
		return ret;

	*ichg = SGM41513D_ICHG_TABLE[reg_val];
	sgm41513d_dbg("%s, reg_val:0x%x, ichg:%duA\n", __func__, reg_val, *ichg);

	return ret;
}

static int sgm41513d_disable_charge(struct charger_device *chg_dev, bool dis)
{
	int ret;
	struct sgm41513d_chip *chip = dev_get_drvdata(&chg_dev->dev);

	sgm41513d_dbg("%s, dis = %d\n", __func__, dis);

	if (IS_ERR_OR_NULL(chip)) {
		sgm41513d_dbg("%s, chip is NULL\n", __func__);
		return -ENODEV;
	}

	chip->disable_charge = dis;

	ret = sgm41513d_enable(chg_dev, !dis);
	if (ret < 0) {
		sgm41513d_dbg("%s, sgm41513d_enable failed\n", __func__);
		return ret;
	}

#ifdef MASTER_CHARGER_NAME
	if (chip->psy)
		power_supply_changed(chip->psy);
#endif

	return 0;
}

static struct charger_ops sgm41513d_chg_ops = {
	/* cable plug in/out */
	.plug_in = sgm41513d_plug_in,
	.plug_out = sgm41513d_plug_out,

	/* enable/disable charger */
	.enable = sgm41513d_enable,
	.is_enabled = sgm41513d_is_enable,

	/* enable/disable chip */
	.enable_chip = sgm41513d_enable_chip,
	.is_chip_enabled = sgm41513d_is_chip_enabled,

	.disable_charge = sgm41513d_disable_charge,

	/* get/set charging current*/
	.get_charging_current = sgm41513d_get_charging_current,
	.set_charging_current = sgm41513d_set_charging_current,
	.get_min_charging_current = sgm41513d_get_min_charging_current,

	/* set cv */
	.set_constant_voltage = sgm41513d_set_constant_voltage,

	/* set input_current */
	.get_input_current = sgm41513d_get_input_current,
	.set_input_current = sgm41513d_set_input_current,
	.get_min_input_current = sgm41513d_get_min_input_current,

	/* set termination current */
	.set_eoc_current = sgm41513d_set_eoc_current,

	.event = sgm41513d_event,

	/* set mivr */
	.get_mivr = sgm41513d_get_mivr,
	.set_mivr = sgm41513d_set_mivr,
	.get_mivr_state = sgm41513d_get_mivr_state,

	/* PD */
	.enable_discharge = sgm41513d_enable_discharge,

	/* run AICL */
	//.run_aicl =

	.is_charging_done = sgm41513d_is_charging_done,

	.dump_registers = sgm41513d_dump_register,
};

static int sgm41513d_init_setting(struct sgm41513d_chip *chip)
{
	int ret = 0;

	ret = sgm41513d_set_watchdog(chip, 0x0);			/* WDT disable */
	ret |= sgm41513d_en_timer(chip, 0x0);			/* Disable Charging Safety Timer */
	ret |= sgm41513d_set_stat_ctrl(chip, 0x3);		/* Disable STAT Pin Function */
	ret |= sgm41513d_set_ovp(chip, 0x2);			/* vbus overvoltage protection, 11V */
	ret |= sgm41513d_set_iterm(chip, 0x1);			/* Termination current 120mA */
	ret |= sgm41513d_set_batfet_reset_enable(chip, 0x0);	/* Disable batfet reset */
	ret |= sgm41513d_set_batfet_dly(chip, 0x0);              /* Turn off BATFET immediately */

#ifdef SLAVE_CHARGER_NAME
	/* default disable slave charger */
	ret |= sgm41513d_set_batfet_dis(chip, 0x1);              /* Turn off BATFET */
	ret |= sgm41513d_set_chg_config(chip, 0x0);
	ret |= gpio_direction_output(chip->gpio_slave_en, 1);
#endif
	if (ret < 0) {
		sgm41513d_dbg("%s fail\n", __func__);
		return -EIO;
	}

	sgm41513d_dbg("%s: down!\n", __func__);
	return ret;
}

#ifdef MASTER_CHARGER_NAME
static void sgm41513d_parse_vbus_stat(struct sgm41513d_chip *chip, u8 vbus_stat)
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

static int sgm41513d_get_vbus_stat(struct sgm41513d_chip *chip, u8 *val)
{
	return sgm41513d_read_interface(chip->client, SGM41513D_CON8,
			val, CON8_VBUS_STAT_MASK, CON8_VBUS_STAT_SHIFT);
}

static int get_bc12(struct sgm41513d_chip *chip)
{
	int ret, i;
	u8 vbus_stat;

	sgm41513d_dbg("start %s\n", __func__);

	if (!chip || !chip->psy) {
		sgm41513d_dbg("%s: chip is NULL or chip->psy is NULL\n", __func__);
		return -EINVAL;
	}

	for (i=1; i<4; i++) {
		ret = sgm41513d_get_vbus_stat(chip, &vbus_stat);
		if (ret < 0) {
			sgm41513d_dbg("%s sgm41513d_get_vbus_stat err\n", __func__);
			goto out;
		}

		sgm41513d_dbg("%s, vbus_stat = 0x%x, retry:%d\n", __func__, vbus_stat, i);

		if (vbus_stat != 0) {
			sgm41513d_dbg("%s, adapter plug in\n", __func__);
			sgm41513d_parse_vbus_stat(chip, vbus_stat);
			goto out;
		}
		msleep(100);
	}

out:
	power_supply_changed(chip->psy);
	return ret;
}

static int sgm41513d_get_vbus(void)
{
	union power_supply_propval prop;
	static struct power_supply *psy;
	int ret;

	psy = power_supply_get_by_name("mtk-gauge");
	if (psy == NULL || IS_ERR(psy)) {
		sgm41513d_dbg("%s, Couldn't get psy\n", __func__);
		return -1;
	}

	ret = power_supply_get_property(psy, POWER_SUPPLY_PROP_VOLTAGE_NOW, &prop);
	if (ret < 0) {
		sgm41513d_dbg("%s, get vbus fail\n", __func__);
		return -1;
	}

	sgm41513d_dbg("%s, vbus:%d\n", __func__, prop.intval);
	return prop.intval;
}

static char *sgm41513d_psy_supplied_to[] = {
	"battery",
	"mtk-master-charger"
};

static enum power_supply_usb_type sgm41513d_psy_usb_types[] = {
	POWER_SUPPLY_USB_TYPE_UNKNOWN,
	POWER_SUPPLY_USB_TYPE_SDP,
	POWER_SUPPLY_USB_TYPE_CDP,
	POWER_SUPPLY_USB_TYPE_DCP,
};

static enum power_supply_property sgm41513d_psy_properties[] = {
	POWER_SUPPLY_PROP_ONLINE,
	POWER_SUPPLY_PROP_STATUS,
	POWER_SUPPLY_PROP_TYPE,
	POWER_SUPPLY_PROP_USB_TYPE,
	POWER_SUPPLY_PROP_VOLTAGE_NOW,
};

static int sgm41513d_get_vbus_gd(struct sgm41513d_chip *chip, u8 *val)
{
	return sgm41513d_read_interface(chip->client, SGM41513D_CON10,
			val, CON10_VBUS_GD_MASK, CON10_VBUS_GD_SHIFT);
}

static void sgm41513d_parse_chg_stat(struct sgm41513d_chip *chip, u8 chg_stat)
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

static int get_chg_stat(struct sgm41513d_chip *chip)
{
	int ret;
	u8 chg_stat;

	ret = sgm41513d_get_chrg_stat(chip, &chg_stat);
	if (ret < 0) {
		sgm41513d_dbg("%s sgm41513d_get_chrg_stat err\n", __func__);
		return -EINVAL;
	}

	sgm41513d_parse_chg_stat(chip, chg_stat);

	return 0;
}

static int sgm41513d_get_property(struct power_supply *psy,
		enum power_supply_property psp, union power_supply_propval *val)
{
	int ret;
	struct sgm41513d_chip *chip = power_supply_get_drvdata(psy);

	switch (psp) {
		case POWER_SUPPLY_PROP_ONLINE:
			ret = sgm41513d_get_vbus_gd(chip, &chip->power_good);
			if (ret < 0) {
				sgm41513d_dbg("%s sgm41513d_get_vbus_gd err\n", __func__);
				return -EINVAL;
			}
			val->intval = chip->power_good;
			break;
		case POWER_SUPPLY_PROP_STATUS:
			ret = get_chg_stat(chip);
			if (ret < 0) {
				sgm41513d_dbg("%s sgm41513d_get_chrg_stat err\n", __func__);
				return -EINVAL;
			}
			val->intval = chip->chg_stat;
			break;
		case POWER_SUPPLY_PROP_TYPE:
			val->intval = chip->psy_desc.type;
			break;
		case POWER_SUPPLY_PROP_USB_TYPE:
			val->intval = chip->psy_usb_type;
			break;
		case POWER_SUPPLY_PROP_VOLTAGE_NOW:
			val->intval = sgm41513d_get_vbus();
			break;
		default:
			return -EINVAL;
	}

	return 0;
}

static int sgm41513d_set_property(struct power_supply *psy,
		enum power_supply_property psp,
		const union power_supply_propval *val)
{
	int ret;
	struct sgm41513d_chip *chip = power_supply_get_drvdata(psy);

	switch (psp) {
		case POWER_SUPPLY_PROP_ONLINE:
			ret = get_bc12(chip);
			if (ret < 0) {
				sgm41513d_dbg("%s: get_bc12 err\n", __func__);
				return -EINVAL;
			}
			break;
		default:
			return -EINVAL;
	}

	return 0;
}

static int sgm41513d_property_is_writeable(struct power_supply *psy,
		enum power_supply_property psp)
{
	switch (psp) {
		case POWER_SUPPLY_PROP_ONLINE:
			return 1;
		default:
			return 0;
	}
}

static const struct power_supply_desc sgm41513d_psy_desc = {
	.name = "mtk_charger_type",
	.type = POWER_SUPPLY_TYPE_USB,
	.usb_types = sgm41513d_psy_usb_types,
	.num_usb_types = ARRAY_SIZE(sgm41513d_psy_usb_types),
	.properties = sgm41513d_psy_properties,
	.num_properties = ARRAY_SIZE(sgm41513d_psy_properties),
	.get_property = sgm41513d_get_property,
	.set_property = sgm41513d_set_property,
	.property_is_writeable = sgm41513d_property_is_writeable,
};

static int sgm41513d_init_psy(struct sgm41513d_chip *chip)
{
	struct power_supply_config cfg = {
		.drv_data = chip,
		.of_node = chip->dev->of_node,
		.supplied_to = sgm41513d_psy_supplied_to,
		.num_supplicants = ARRAY_SIZE(sgm41513d_psy_supplied_to),
	};

	memcpy(&chip->psy_desc, &sgm41513d_psy_desc, sizeof(chip->psy_desc));
	chip->psy = devm_power_supply_register(chip->dev, &chip->psy_desc, &cfg);

	return IS_ERR(chip->psy) ? PTR_ERR(chip->psy) : 0;
}
#endif

static int sgm41513d_init_regulator(struct sgm41513d_chip *chip)
{
	struct regulator_config cfg = {
		.dev = chip->dev,
		.driver_data = chip,
	};

	memcpy(&chip->rdesc, &sgm41513d_otg_rdesc, sizeof(chip->rdesc));
	chip->rdesc.name = dev_name(chip->dev);
	chip->rdev = devm_regulator_register(chip->dev, &chip->rdesc, &cfg);
	return IS_ERR(chip->rdev) ? PTR_ERR(chip->rdev) : 0;
}

static int sgm41513d_init_chg_dev(struct sgm41513d_chip *chip)
{
	chip->chg_dev = charger_device_register(chip->chg_name, chip->dev,
			chip, &sgm41513d_chg_ops,
			&chip->chg_props);
	return IS_ERR(chip->chg_dev) ? PTR_ERR(chip->chg_dev) : 0;
}

static int sgm41513d_probe(struct i2c_client *client,
		const struct i2c_device_id *id)
{
	int ret;
	struct sgm41513d_chip *chip;
	struct device *dev = &client->dev;

	sgm41513d_dbg("[%s]\n", __func__);

	chip = devm_kzalloc(dev, sizeof(*chip), GFP_KERNEL);
	if (!chip)
		return -ENOMEM;

	chip->client = client;
	chip->dev = dev;
	i2c_set_clientdata(client, chip);

	if (!sgm41513d_check_devinfo(chip)) {
		return -ENODEV;
	}

	ret = sgm41513d_parse_dts(chip);
	if (ret < 0) {
		sgm41513d_dbg("%s parse dt fail(%d)\n", __func__, ret);
		return ret;
	}

	mutex_init(&chip->i2c_lock);

	ret = sgm41513d_reset_reg(chip);
	if (ret < 0)
		goto out;

	ret = sgm41513d_init_setting(chip);
	if (ret < 0)
		goto out;

#ifdef MASTER_CHARGER_NAME
	ret = sgm41513d_init_psy(chip);
	if (ret < 0) {
		sgm41513d_dbg("%s failed to init power supply\n", __func__);
		goto out;
	}
#endif

	ret = sgm41513d_init_regulator(chip);
	if (ret < 0) {
		sgm41513d_dbg("%s, failed to init regulator\n", __func__);
		goto out;
	}

	ret = sgm41513d_init_chg_dev(chip);
	if (ret < 0) {
		sgm41513d_dbg("%s failed to init chg_dev\n", __func__);
		goto out;
	}

	ret = device_create_file(dev, &dev_attr_dump_reg);
	if (ret < 0) {
		sgm41513d_dbg("%s create file fail(%d)\n",
				__func__, ret);
		goto create_file_err;
	}

	chip->disable_charge = false;

	sgm41513d_dump_register(chip->chg_dev);
	sgm41513d_dbg("%s successfully\n", __func__);

	return ret;

create_file_err:
	charger_device_unregister(chip->chg_dev);
out:
	mutex_destroy(&chip->i2c_lock);
	return ret;
}

static int sgm41513d_remove(struct i2c_client *client)
{
	struct sgm41513d_chip *chip = i2c_get_clientdata(client);

	sgm41513d_dbg("%s\n", __func__);

	if (chip) {
		device_remove_file(chip->dev, &dev_attr_dump_reg);
		charger_device_unregister(chip->chg_dev);
		mutex_destroy(&chip->i2c_lock);
	}

	return 0;
}

static const struct of_device_id sgm41513d_of_match[] = {
#ifdef MASTER_CHARGER_NAME
	{.compatible = "sc,sgm41513d_master"},
#else
	{.compatible = "sc,sgm41513d_slave"},
#endif
	{},
};

static const struct i2c_device_id sgm41513d_i2c_id[] = {
#ifdef MASTER_CHARGER_NAME
	{MASTER_CHARGER_NAME, 0},
#else
	{SLAVE_CHARGER_NAME, 0},
#endif
	{}
};
MODULE_DEVICE_TABLE(i2c, sgm41513d_i2c_id);

static struct i2c_driver sgm41513d_i2c_driver = {
	.driver = {
#ifdef MASTER_CHARGER_NAME
		.name = MASTER_CHARGER_NAME,
#else
		.name = SLAVE_CHARGER_NAME,
#endif
		.owner = THIS_MODULE,
		.of_match_table = sgm41513d_of_match,
	},
	.probe = sgm41513d_probe,
	.remove = sgm41513d_remove,
	.id_table = sgm41513d_i2c_id,
};
module_i2c_driver(sgm41513d_i2c_driver);

MODULE_AUTHOR("JiangWei Huang <huangjiangwei@boe.com.cn>");
MODULE_DESCRIPTION("SGM41513D Charger Driver");
MODULE_LICENSE("GPL");
