/* SPDX-License-Identifier: GPL-2.0
 *
 * Copyright (c) 2023 BOE Inc.
 */

#include "boe_charger.h"

static int boe_read_byte(struct i2c_client *client, u8 cmd)
{
	s32 ret;
	struct boe_chg *chip = i2c_get_clientdata(client);

	mutex_lock(&chip->i2c_lock);
	ret = i2c_smbus_read_byte_data(client, cmd);
	mutex_unlock(&chip->i2c_lock);

	return ret;
}

static int boe_write_byte(struct i2c_client *client, u8 cmd, u8 val)
{
	s32 ret;
	struct boe_chg *chip = i2c_get_clientdata(client);

	mutex_lock(&chip->i2c_lock);
	ret = i2c_smbus_write_byte_data(client, cmd, val);
	mutex_unlock(&chip->i2c_lock);
	return ret;
}

static int boe_update_bits(struct boe_chg *chip, u8 reg, u8 mask, u8 data)
{
	int ret;
	u8 tmp = 0;

	ret = boe_read_byte(chip->client, reg);
	if (ret < 0)
		return ret;

	tmp &= ~mask;
	tmp |= data & mask;

	ret = boe_write_byte(chip->client, reg, tmp);
	if (ret < 0)
		return ret;

	return ret;
}

static int boe_read_interface(struct i2c_client *client, u8 reg, u8 *val, u8 mask, u8 shift)
{
	int ret = 0;

	ret = boe_read_byte(client, reg);
	if (ret < 0)
		return ret;

	ret &= (mask << shift);
	*val = (ret >> shift);

	return 0;
}

static int boe_write_interface(struct i2c_client *client, u8 reg, u8 val, u8 mask, u8 shift)
{
	int ret = 0;
	u8 read_reg = 0;

	ret = boe_read_byte(client, reg);
	if (ret < 0)
		return -EIO;

	read_reg = ret;
	ret &= ~(mask << shift);
	ret |= (val << shift);

	return boe_write_byte(client, reg, ret);
}

static int boe_set_en_hiz(struct boe_chg *chip, u8 val)
{
	int ret = 0;

	ret = boe_write_interface(chip->client, (u8) (REG0),
			(u8) (val),
			(u8) (REG0_EN_HIZ_MASK),
			(u8) (REG0_EN_HIZ_SHIFT)
			);
	if (ret < 0)
		dev_err(chip->dev, "%s fail\n", __func__);

	return ret;
}

static int boe_set_stat_ctrl(struct boe_chg *chip, u8 val)
{
	int ret = 0;

	ret = boe_write_interface(chip->client, (u8) (REG0),
			(u8) (val),
			(u8) (REG0_STAT_IMON_CTRL_MASK),
			(u8) (REG0_STAT_IMON_CTRL_SHIFT)
			);
	if (ret < 0)
		dev_err(chip->dev, "%s fail\n", __func__);

	return ret;
}

static int boe_set_otg_config(struct boe_chg *chip, u8 val)
{
	int ret = 0;

	ret = boe_write_interface(chip->client, (u8) (REG1),
			(u8) (val),
			(u8) (REG1_OTG_CONFIG_MASK),
			(u8) (REG1_OTG_CONFIG_SHIFT)
			);
	if (ret < 0)
		dev_err(chip->dev, "%s fail\n", __func__);

	return ret;
}

static int boe_get_otg_config(struct boe_chg *chip, u8 *val)
{
	return boe_read_interface(chip->client, (u8) (REG1),
			(val),
			(u8) (REG1_OTG_CONFIG_MASK),
			(u8) (REG1_OTG_CONFIG_SHIFT)
			);
}

static int boe_set_chg_config(struct boe_chg *chip, u8 val)
{
	int ret = 0;

	ret = boe_write_interface(chip->client, (u8) (REG1),
			(u8) (val),
			(u8) (REG1_CHG_CONFIG_MASK),
			(u8) (REG1_CHG_CONFIG_SHIFT)
			);
	if (ret < 0)
		dev_err(chip->dev, "%s fail\n", __func__);

	return ret;
}

static int boe_get_chg_config(struct boe_chg *chip, u8 *val)
{
	return boe_read_interface(chip->client, (u8) (REG1),
			(val),
			(u8) (REG1_CHG_CONFIG_MASK),
			(u8) (REG1_CHG_CONFIG_SHIFT)
			);
}

static int boe_get_ichg(struct boe_chg *chip, u8 *val)
{
	return boe_read_interface(chip->client, (u8) (REG2),
			(val),
			(u8) (REG2_ICHG_MASK),
			(u8) (REG2_ICHG_SHIFT)
			);
}

static int boe_set_ichg(struct boe_chg *chip, u8 val)
{
	int ret = 0;

	ret = boe_write_interface(chip->client, (u8) (REG2),
			(u8) (val),
			(u8) (REG2_ICHG_MASK),
			(u8) (REG2_ICHG_SHIFT)
			);
	if (ret < 0)
		dev_err(chip->dev, "%s fail\n", __func__);

	return ret;
}

static int boe_get_iterm(struct boe_chg *chip, u8 *val)
{
	return boe_read_interface(chip->client, REG3, val, REG3_ITERM_MASK, REG3_ITERM_SHIFT);
}

static int boe_set_iterm(struct boe_chg *chip, u8 val)
{
	int ret = 0;

	ret = boe_write_interface(chip->client, (u8) (REG3),
			(u8) (val),
			(u8) (REG3_ITERM_MASK),
			(u8) (REG3_ITERM_SHIFT)
			);
	if (ret < 0)
		dev_err(chip->dev, "%s fail\n", __func__);

	return ret;
}

static int boe_get_vreg(struct boe_chg *chip, u8 *val)
{
	return boe_read_interface(chip->client, REG4, val, REG4_VREG_MASK, REG4_VREG_SHIFT);
}

static int boe_set_vreg(struct boe_chg *chip, u8 val)
{
	int ret = 0;

	dev_info(chip->dev, "%s, val = 0x%x\n", __func__, val);
	ret = boe_write_interface(chip->client, (u8) (REG4),
			(u8) (val),
			(u8) (REG4_VREG_MASK),
			(u8) (REG4_VREG_SHIFT)
			);
	if (ret < 0)
		dev_err(chip->dev, "%s fail\n", __func__);

	return ret;
}

static int boe_set_vrechg(struct boe_chg *chip, u8 val)
{
	int ret = 0;

	ret = boe_write_interface(chip->client, REG4, val, REG4_VRECHG_MASK, REG4_VRECHG_SHIFT);
	if (ret < 0)
		dev_err(chip->dev, "%s fail\n", __func__);

	return ret;
}

static int boe_set_watchdog(struct boe_chg *chip, u8 val)
{
	int ret = 0;

	ret = boe_write_interface(chip->client, (u8) (REG5),
			(u8) (val),
			(u8) (REG5_WATCHDOG_MASK),
			(u8) (REG5_WATCHDOG_SHIFT)
			);
	if (ret < 0)
		dev_err(chip->dev, "%s fail\n", __func__);

	return ret;
}

static int boe_en_timer(struct boe_chg *chip, u8 val)
{
	int ret = 0;

	ret = boe_write_interface(chip->client, REG5, val, REG5_EN_TIMER_MASK, REG5_EN_TIMER_SHIFT);
	if (ret < 0)
		dev_err(chip->dev, "%s fail\n", __func__);

	return ret;
}

static int boe_set_vindpm(struct boe_chg *chip, u8 val)
{
	int ret = 0;

	dev_info(chip->dev, "%s, val = 0x%x\n", __func__, val);
	ret = boe_write_interface(chip->client, (u8) (REG6),
			(u8) (val),
			(u8) (REG6_VINDPM_MASK),
			(u8) (REG6_VINDPM_SHIFT)
			);
	if (ret < 0)
		dev_err(chip->dev, "%s fail\n", __func__);

	return ret;
}

static int boe_get_vindpm(struct boe_chg *chip, u8 *val)
{
	return boe_read_interface(chip->client, (u8) (REG6),
			(val),
			(u8) (REG6_VINDPM_MASK),
			(u8) (REG6_VINDPM_SHIFT)
			);
}

static int boe_set_ovp(struct boe_chg *chip, u8 val)
{
	int ret = 0;

	ret = boe_write_interface(chip->client, (u8) (REG6),
			(u8) (val),
			(u8) (REG6_OVP_MASK),
			(u8) (REG6_OVP_SHIFT)
			);
	if (ret < 0)
		dev_err(chip->dev, "%s fail\n", __func__);

	return ret;
}

static int boe_set_vindpm_os(struct boe_chg *chip, u8 val)
{
	int ret = 0;

	ret = boe_write_interface(chip->client, (u8) (REG15),
			(u8) (val),
			(u8) (REG15_VINDPM_OS_MASK),
			(u8) (REG15_VINDPM_OS_SHIFT)
			);
	if (ret < 0)
		dev_err(chip->dev, "%s fail\n", __func__);

	return ret;
}

static int boe_get_vindpm_os(struct boe_chg *chip, u8 *val)
{
	return boe_read_interface(chip->client, REG15,
			val, REG15_VINDPM_OS_MASK, REG15_VINDPM_OS_SHIFT);
}

static int boe_set_batfet_reset_enable(struct boe_chg *chip, u8 val)
{
	int ret = 0;

	dev_info(chip->dev, "%s, val = 0x%x\n", __func__, val);
	ret = boe_write_interface(chip->client, (u8) (REG7),
			(u8) (val),
			(u8) (REG7_BATFET_RST_EN_MASK),
			(u8) (REG7_BATFET_RST_EN_SHIFT)
			);
	if (ret < 0)
		dev_err(chip->dev, "%s fail\n", __func__);

	return ret;
}

static int boe_set_batfet_dly(struct boe_chg *chip, u8 val)
{
	int ret = 0;

	ret = boe_write_interface(chip->client, (u8) (REG7),
			(u8) (val),
			(u8) (REG7_BATFET_DLY_MASK),
			(u8) (REG7_BATFET_DLY_SHIFT)
			);
	if (ret < 0)
		dev_err(chip->dev, "%s fail\n", __func__);

	return ret;
}

static int boe_set_batfet_dis(struct boe_chg *chip, u8 val)
{
	int ret = 0;

	dev_info(chip->dev, "%s, val = 0x%x\n", __func__, val);
	ret = boe_write_interface(chip->client, (u8) (REG7),
			(u8) (val),
			(u8) (REG7_BATFET_DISABLE_MASK),
			(u8) (REG7_BATFET_DISABLE_SHIFT)
			);
	if (ret < 0)
		dev_err(chip->dev, "%s fail\n", __func__);

	return ret;
}

static int boe_get_vindpm_stat(struct boe_chg *chip, u8 *val)
{
	return boe_read_interface(chip->client, (u8) (REG10),
			(val),
			(u8) (REG10_VINDPM_STAT_MASK),
			(u8) (REG10_VINDPM_STAT_SHIFT)
			);
}

static bool boe_check_devinfo(struct boe_chg *chip)
{
	int ret;
	u8 chip_id;

	ret = i2c_smbus_read_byte_data(chip->client, REG11);
	if (ret < 0) {
		dev_err(chip->dev, "%s get devchip fail(%d)\n",
				__func__, ret);
		return false;
	}

	ret &= (REG11_PN_MASK << REG11_PN_SHIFT);
	chip_id = (ret >> REG11_PN_SHIFT);

	dev_info(chip->dev, "%s reg11:0x%x, chip_id = 0x%x\n", __func__, ret, chip_id);

	return true;
}

static void convert_to_reg(const u32 *table, int table_size, u32 raw_val, u8 *reg_val)
{
	int i;

	if(raw_val > table[table_size - 1]) {
		*reg_val = (table_size - 1);
		pr_info("raw_val = %d, out of [%d-%d], default use %d\n",
				raw_val, table[0], table[table_size - 1], table[table_size - 1]);
		return;
	}

	if(raw_val < table[0]) {
		*reg_val = 0;
		pr_info("raw_val = %d, out of [%d-%d], default use %d\n",
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

	pr_err("%s fail\n", __func__);
}

static int boe_get_charging_current(struct charger_device *chg_dev, u32 *ichg)
{
	int ret = 0;
	u8 reg_val = 0;
	struct boe_chg *chip = dev_get_drvdata(&chg_dev->dev);

	ret = boe_get_ichg(chip, &reg_val);
	if (ret < 0)
		return ret;

	*ichg = SGM_ICHG_TABLE[reg_val];
	dev_info(chip->dev, "%s, reg_val:0x%x, ichg:%duA\n", __func__, reg_val, *ichg);

	return ret;
}

static int boe_set_charging_current(struct charger_device *chg_dev, u32 ichg)
{
	u8 reg_val;
	struct boe_chg *chip = dev_get_drvdata(&chg_dev->dev);

	convert_to_reg(SGM_ICHG_TABLE, ARRAY_SIZE(SGM_ICHG_TABLE), ichg, &reg_val);

	dev_info(chip->dev, "%s, ichg:%duA, reg_val:0x%x\n", __func__, ichg, reg_val);

	return boe_set_ichg(chip, reg_val);
}

static int boe_get_min_charging_current(struct charger_device *chg_dev, u32 *min_ichg)
{
	*min_ichg = SGM_ICHG_TABLE[0];
	return 0;
}

static int boe_get_input_current(struct charger_device *chg_dev, u32 *aicr)
{
	int ret = 0;
	u8 reg_val = 0;
	struct boe_chg *chip = dev_get_drvdata(&chg_dev->dev);

	ret = boe_read_byte(chip->client, REG0);
	if (ret < 0)
		return ret;

	reg_val = (ret & REG0_IINLIM_MASK) >> REG0_IINLIM_SHIFT;

	*aicr = reg_val * 100000 + 100000;

	dev_info(chip->dev, "%s, reg_val:0x%x, aicr:%duA\n", __func__, reg_val, *aicr);

	return ret;
}

static int boe_set_input_current(struct charger_device *chg_dev, u32 aicr)
{
	int ret;
	u8 reg_val;
	struct boe_chg *chip = dev_get_drvdata(&chg_dev->dev);

	chip->last_iin = aicr;

	if (chip->disable_charge)
		aicr = 100000;

	reg_val = (aicr - 100000) / 100000;

	dev_info(chip->dev, "%s, aicr:%duA, reg_val:0x%x\n", __func__, aicr, reg_val);

	ret = boe_update_bits(chip, REG0, REG0_IINLIM_MASK, reg_val);
	if (ret < 0)
		dev_err(chip->dev, "%s: boe_update_bits err\n", __func__);

	return ret;
}

static int boe_get_min_input_current(struct charger_device *chg_dev, u32 *min_aicr)
{
	*min_aicr = 100000;
	return 0;
}

static int boe_get_eoc_current(struct charger_device *chg_dev, u32 *ieoc_ua)
{
        int ret = 0;
        u8 reg_val = 0;
        struct boe_chg *chip = dev_get_drvdata(&chg_dev->dev);

        ret = boe_get_iterm(chip, &reg_val);
        if (ret < 0)
                return ret;

	*ieoc_ua = SGM_ITERM_TABLE[reg_val];

	dev_info(chip->dev, "%s, reg_val:0x%x, ieoc:%duA\n", __func__, reg_val, *ieoc_ua);

        return ret;
}

static int boe_set_eoc_current(struct charger_device *chg_dev, u32 ieoc)
{
	u8 reg_val;
	struct boe_chg *chip = dev_get_drvdata(&chg_dev->dev);

	convert_to_reg(SGM_ITERM_TABLE, ARRAY_SIZE(SGM_ITERM_TABLE), ieoc, &reg_val);

	return boe_set_iterm(chip, reg_val);
}

static int boe_event(struct charger_device *chg_dev, u32 event, u32 args)
{
	struct boe_chg *chip = dev_get_drvdata(&chg_dev->dev);

	switch (event) {
		case EVENT_FULL:
		case EVENT_RECHARGE:
		case EVENT_DISCHARGE:
			power_supply_changed(chip->psy);
			break;
		default:
			break;
	}

	return 0;
}

static int boe_get_constant_voltage(struct charger_device *chg_dev, u32 *cv_uv)
{
        int ret = 0;
        u8 reg_val = 0;
        struct boe_chg *chip = dev_get_drvdata(&chg_dev->dev);

	ret = boe_get_vreg(chip, &reg_val);
	if (ret < 0)
		return ret;

	if (reg_val == 15)
		*cv_uv = 4350000;
	else if (reg_val > 24)
		*cv_uv = 4624000;
	else
		*cv_uv = 3856000 + 32 * reg_val * 1000;

        dev_info(chip->dev, "%s, reg_val:0x%x, cv:%duV\n", __func__, reg_val, *cv_uv);

        return ret;
}

static int boe_set_constant_voltage(struct charger_device *chg_dev, u32 cv_uv)
{
	u8 reg_val;
	struct boe_chg *chip = dev_get_drvdata(&chg_dev->dev);

	reg_val = (cv_uv - 3856000) / 1000 / 32;

	dev_info(chip->dev, "%s, cv_uv:%duV, reg_val:0x%x\n", __func__, cv_uv, reg_val);

	return boe_set_vreg(chip, reg_val);
}

static int boe_enable(struct charger_device *chg_dev, bool en)
{
	int ret;
	struct boe_chg *chip = dev_get_drvdata(&chg_dev->dev);

	if (chip->disable_charge)
		en = false;

	dev_info(chip->dev, "%s, en = %d\n", __func__, en);

	if (!chip->is_primary_chg)
		ret = boe_set_batfet_dis(chip, !en);
	ret = boe_set_chg_config(chip, en);
	if (ret < 0)
		dev_err(chip->dev, "%s: boe_set_chg_config fail\n", __func__);

	return ret;
}

static int boe_set_mivr(struct charger_device *chg_dev, u32 vindpm_uv)
{
	int ret;
	u8 reg_val, os_val;
	u32 offset;
	struct boe_chg *chip = dev_get_drvdata(&chg_dev->dev);

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

	ret = boe_set_vindpm_os(chip, os_val);
	if (ret < 0)
		return ret;

	reg_val = (vindpm_uv - offset) / 100000;

	dev_info(chip->dev, "%s, vindpm_uv = %duV, reg_val = 0x%x\n", __func__, vindpm_uv, reg_val);
	return boe_set_vindpm(chip, reg_val);
}

static int boe_get_mivr(struct charger_device *chg_dev, u32 *mivr)
{
	int ret;
	u32 offset = 0;
	u8 reg_vindpm, reg_vindpm_os;
	struct boe_chg *chip = dev_get_drvdata(&chg_dev->dev);

	ret = boe_get_vindpm(chip, &reg_vindpm);
	if (ret < 0)
		return ret;

	ret = boe_get_vindpm_os(chip, &reg_vindpm_os);
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

	dev_info(chip->dev, "%s, reg_vindpm:0x%x, reg_vindpm_os:0x%x, mivr:%duV\n", __func__, reg_vindpm, *mivr, reg_vindpm_os);
	return ret;
}

static int boe_get_mivr_state(struct charger_device *chg_dev, bool *in_loop)
{
	int ret;
	u8 reg_val;
	struct boe_chg *chip = dev_get_drvdata(&chg_dev->dev);

	ret = boe_get_vindpm_stat(chip, &reg_val);
	if (ret < 0)
		return ret;

	if (reg_val == 1)
		*in_loop = true;
	else
		*in_loop = false;

	return ret;
}

static int boe_otg_is_enabled(struct regulator_dev *rdev)
{
	struct boe_chg *chip = rdev_get_drvdata(rdev);
	u8 otg_on;

	if (boe_get_otg_config(chip, &otg_on) < 0)
		return -EIO;
	return otg_on;
}

static int boe_enable_otg(struct regulator_dev *rdev)
{
	struct boe_chg *chip = rdev_get_drvdata(rdev);

	return boe_set_otg_config(chip, 1);
}

static int boe_disable_otg(struct regulator_dev *rdev)
{
	struct boe_chg *chip = rdev_get_drvdata(rdev);

	return boe_set_otg_config(chip, 0);
}

static int boe_parse_dts(struct boe_chg *chip)
{
	int ret = 0, irq_gpio = 0, irqn = 0;
	struct device_node *np = chip->dev->of_node;

	if (!np) {
		dev_err(chip->dev, "%s no device node\n", __func__);
		return -EINVAL;
	}

	if (of_property_read_string(np, "charger_name",
				&chip->chg_name) < 0)
		dev_err(chip->dev, "%s: no charger name\n", __func__);

	if (!strcmp(chip->chg_name, "primary_chg"))
		chip->is_primary_chg = true;
	else if (!strcmp(chip->chg_name, "secondary_chg"))
		chip->is_primary_chg = false;
	else {
		dev_err(chip->dev, "%s: dts charger_name match failed\n", __func__);
		chip->is_primary_chg = true;
	}

	if (of_property_read_string(np, "alias_name",
				&(chip->chg_props.alias_name)) < 0)
		dev_err(chip->dev, "%s: no alias name\n", __func__);

	if (!chip->is_primary_chg) {
		chip->gpio_secondary_en = of_get_named_gpio(np, "gpio_secondary_en", 0);
		if (!gpio_is_valid(chip->gpio_secondary_en))
			dev_err(chip->dev, "%s, get gpio failed\n", __func__);

		ret = gpio_request(chip->gpio_secondary_en, "gpio_secondary_en");
		if (ret)
			dev_err(chip->dev, "%s, request gpio fail\n", __func__);
	} else {
		irq_gpio = of_get_named_gpio(chip->dev->of_node, "irq-gpio", 0);
		if (!gpio_is_valid(irq_gpio)) {
			dev_err(chip->dev, "%s: %d gpio get failed\n", __func__, irq_gpio);
			return -EINVAL;
		}

		ret = gpio_request(irq_gpio, "boe_chg_irq");
		if (ret) {
			dev_err(chip->dev, "%s: %d gpio request failed\n", __func__, irq_gpio);
			return ret;
		}

		irqn = gpio_to_irq(irq_gpio);
		if (irqn < 0) {
			dev_err(chip->dev, "%s:%d gpio_to_irq failed\n", __func__, irqn);
			return irqn;
		}
		chip->client->irq = irqn;
	}

	dev_info(chip->dev, "%s, charger_name=%s alias_name=%s\n", __func__, chip->chg_name, chip->chg_props.alias_name);

	return ret;
}

static int boe_is_enable(struct charger_device *chg_dev, bool *en)
{
	int ret;
	u8 val;
	struct boe_chg *chip = dev_get_drvdata(&chg_dev->dev);

	ret = boe_get_chg_config(chip, &val);
	if (ret < 0)
		return ret;

	if(val == 1)
		*en = true;
	else
		*en = false;

	return ret;
}

static const struct regulator_ops boe_otg_ops = {
	.is_enabled = boe_otg_is_enabled,
	.enable = boe_enable_otg,
	.disable = boe_disable_otg,
};

static const struct regulator_desc boe_otg_rdesc = {
	.of_match = "usb-otg-vbus",
	.name = "usb-otg-vbus",
	.ops = &boe_otg_ops,
	.owner = THIS_MODULE,
	.type = REGULATOR_VOLTAGE,
};

static ssize_t dump_reg_show(struct device *dev, struct device_attribute *attr, char *buf)
{
	int i = 0, count = 0, reg = 0;
	struct boe_chg *chip = dev_get_drvdata(dev);

	if (chip == NULL) {
		pr_err("%s chip == NULL\n", __func__);
		return -EINVAL;
	}

	for (i = 0; i < REG_NUM; i++) {
		reg = boe_read_byte(chip->client, i);
		if (reg < 0) {
			dev_err(chip->dev, "%s, boe_read_byte fail\n", __func__);
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
	struct boe_chg *chip = dev_get_drvdata(dev);

	if (chip == NULL) {
		pr_err("%s chip == NULL\n", __func__);
		return -EINVAL;
	}

	if (buf != NULL && size != 0) {
		pvalue = (char *)buf;
		addr = strsep(&pvalue, " ");
		ret = kstrtou32(addr, 16, (u32 *)&reg_address);

		val = strsep(&pvalue, " ");
		ret = kstrtou32(val, 16, (u32 *)&reg_value);

		ret = boe_write_byte(chip->client, reg_address, reg_value);
		if (ret < 0) {
			dev_err(chip->dev, "Failed: reg=%02X, ret=%d\n", (u32)reg_address, ret);
			return -EIO;
		}
	}

	return size;
}

static DEVICE_ATTR_RW(dump_reg);

static int boe_dump_register(struct charger_device *chg_dev)
{
	int i = 0;
	u8 val[REG_NUM] = {0};
	struct boe_chg *chip = dev_get_drvdata(&chg_dev->dev);

	if (chip == NULL) {
		pr_err("%s chip == NULL\n", __func__);
		return -EINVAL;
	}

	for (i=0; i<REG_NUM; i++) {
		val[i] = boe_read_byte(chip->client, i);
		if (val[i] < 0) {
			dev_err(chip->dev, "%s, boe_read_byte fail\n", __func__);
			return -EIO;
		}
	}

	dev_info(chip->dev, "%s, r0:0x%x,r1:0x%x,r2:0x%x,r3:0x%x,r4:0x%x,r5:0x%x,r6:0x%x,r7:0x%x,r8:0x%x,r9:0x%x,r10:0x%x,r11:0x%x,r12:0x%x,r13:0x%x,r14:0x%x,r15:0x%x\n",
			__func__, val[0], val[1], val[2], val[3], val[4], val[5], val[6], val[7], val[8], val[9], val[10], val[11], val[12], val[13], val[14], val[15]);

	return 0;
}

static int boe_disable_charge(struct charger_device *chg_dev, bool dis)
{
	int ret;
	struct boe_chg *chip = dev_get_drvdata(&chg_dev->dev);

	dev_info(chip->dev, "%s, dis=%d, last_iin=%d\n", __func__, dis, chip->last_iin);

	if (IS_ERR_OR_NULL(chip)) {
		pr_err("%s, chip is NULL\n", __func__);
		return -ENODEV;
	}

	chip->disable_charge = dis;

	if (dis)
		ret = boe_set_input_current(chg_dev, 100000);
	else
		ret = boe_set_input_current(chg_dev, chip->last_iin);
	ret |= boe_enable(chg_dev, !dis);
	ret |= boe_set_en_hiz(chip, dis); //not effective
	if (ret < 0)
		return ret;

	if (chip->is_primary_chg && chip->psy)
		power_supply_changed(chip->psy);

	return 0;
}

static struct charger_ops boe_chg_ops = {
	/* enable/disable charger */
	.enable = boe_enable,
	.is_enabled = boe_is_enable,

	/* factory */
	.disable_charge = boe_disable_charge,

	/* get/set charging current*/
	.get_charging_current = boe_get_charging_current,
	.set_charging_current = boe_set_charging_current,
	.get_min_charging_current = boe_get_min_charging_current,

	/* get/set cv */
	.get_constant_voltage = boe_get_constant_voltage,
	.set_constant_voltage = boe_set_constant_voltage,

	/* set input_current */
	.get_input_current = boe_get_input_current,
	.set_input_current = boe_set_input_current,
	.get_min_input_current = boe_get_min_input_current,

	/* get/set termination current */
	.get_eoc_current = boe_get_eoc_current,
	.set_eoc_current = boe_set_eoc_current,

	.event = boe_event,

	/* set mivr */
	.get_mivr = boe_get_mivr,
	.set_mivr = boe_set_mivr,
	.get_mivr_state = boe_get_mivr_state,

	/* run AICL */
	//.run_aicl =

	.dump_registers = boe_dump_register,
};

static int boe_init_setting(struct boe_chg *chip)
{
	int ret = 0;

	ret = boe_set_watchdog(chip, 0x0);		/* WDT disable */
	ret |= boe_en_timer(chip, 0x0);			/* Disable Charging Safety Timer */
	ret |= boe_set_stat_ctrl(chip, 0x3);		/* Disable STAT Pin Function */
	ret |= boe_set_ovp(chip, 0x2);			/* vbus overvoltage protection, 11V */
	ret |= boe_set_iterm(chip, 0xc);		/* Termination current 160mA */
	ret |= boe_set_vrechg(chip, 0x1);
	ret |= boe_set_batfet_reset_enable(chip, 0x0);	/* Disable batfet reset */
	ret |= boe_set_batfet_dly(chip, 0x0);		/* Turn off BATFET immediately */
	ret |= boe_set_chg_config(chip, 0);
	ret |= boe_write_interface(chip->client, REG10, 1, REG10_VINDPM_INT_MASK_MASK, REG10_VINDPM_INT_MASK_SHIFT); /* disable vindpm int */
	ret |= boe_write_interface(chip->client, REG10, 1, REG10_IINDPM_INT_MASK_MASK, REG10_IINDPM_INT_MASK_SHIFT); /* disable iindpm int */

	if (!chip->is_primary_chg) {
		ret |= boe_set_batfet_dis(chip, 1);
		ret |= gpio_direction_output(chip->gpio_secondary_en, 1);
	}

	if (ret < 0) {
		dev_err(chip->dev, "%s fail\n", __func__);
		return -EIO;
	}

	dev_info(chip->dev, "%s: down!\n", __func__);
	return ret;
}

static int boe_get_vbus(struct boe_chg *chip)
{
	union power_supply_propval prop;
	static struct power_supply *psy;
	int ret;

	psy = power_supply_get_by_name("mtk-gauge");
	if (psy == NULL || IS_ERR(psy)) {
		dev_err(chip->dev, "%s, couldn't get psy\n", __func__);
		return -1;
	}

	ret = power_supply_get_property(psy, POWER_SUPPLY_PROP_VOLTAGE_NOW, &prop);
	if (ret < 0) {
		dev_err(chip->dev, "%s, get vbus fail\n", __func__);
		return -1;
	}

	dev_info(chip->dev, "%s, vbus:%d\n", __func__, prop.intval);
	return prop.intval;
}

static char *boe_psy_supplied_to[] = {
	"battery",
	"mtk-master-charger",
};

static enum power_supply_usb_type boe_psy_usb_types[] = {
	POWER_SUPPLY_USB_TYPE_UNKNOWN,
	POWER_SUPPLY_USB_TYPE_SDP,
	POWER_SUPPLY_USB_TYPE_CDP,
	POWER_SUPPLY_USB_TYPE_DCP,
	POWER_SUPPLY_USB_TYPE_PD,
};

static enum power_supply_property boe_psy_properties[] = {
	POWER_SUPPLY_PROP_MANUFACTURER,
	POWER_SUPPLY_PROP_ONLINE,
	POWER_SUPPLY_PROP_STATUS,
	POWER_SUPPLY_PROP_CONSTANT_CHARGE_CURRENT,
	POWER_SUPPLY_PROP_CONSTANT_CHARGE_VOLTAGE,
	POWER_SUPPLY_PROP_INPUT_CURRENT_LIMIT,
	POWER_SUPPLY_PROP_INPUT_VOLTAGE_LIMIT,
	POWER_SUPPLY_PROP_CHARGE_TERM_CURRENT,
	POWER_SUPPLY_PROP_TYPE,
	POWER_SUPPLY_PROP_USB_TYPE,
	POWER_SUPPLY_PROP_CURRENT_MAX,
	POWER_SUPPLY_PROP_VOLTAGE_MAX,
	POWER_SUPPLY_PROP_VOLTAGE_NOW,
};

static int boe_get_property(struct power_supply *psy,
		enum power_supply_property psp, union power_supply_propval *val)
{
	int ret = 0;
	struct boe_chg *chip = power_supply_get_drvdata(psy);

	switch (psp) {
		case POWER_SUPPLY_PROP_MANUFACTURER:
			val->strval = "Sgmicro";
			break;
		case POWER_SUPPLY_PROP_ONLINE:
			val->intval = chip->state.pg_stat;
			break;
		case POWER_SUPPLY_PROP_STATUS:
			if (chip->disable_charge) {
				val->intval = POWER_SUPPLY_STATUS_NOT_CHARGING;
				break;
			}

			if (!chip->state.vbus_stat || (chip->state.vbus_stat == REG8_VBUS_TYPE_OTG))
				val->intval = POWER_SUPPLY_STATUS_DISCHARGING;
			else if (chip->state.chrg_stat == REG8_CHRG_STAT_NOT)
				val->intval = POWER_SUPPLY_STATUS_NOT_CHARGING;
			else if (chip->state.chrg_stat == REG8_CHRG_STAT_TERM)
				val->intval = POWER_SUPPLY_STATUS_FULL;
			else
				val->intval = POWER_SUPPLY_STATUS_CHARGING;
			break;
		case POWER_SUPPLY_PROP_CONSTANT_CHARGE_CURRENT:
			ret = boe_get_charging_current(chip->chg_dev, &val->intval);
			if (ret < 0)
				dev_err(chip->dev, "%s: boe_get_charging_current fail\n", __func__);
			break;
		case POWER_SUPPLY_PROP_CONSTANT_CHARGE_VOLTAGE:
			ret = boe_get_constant_voltage(chip->chg_dev, &val->intval);
			if (ret < 0)
				dev_err(chip->dev, "%s: boe_get_constant_voltage fail\n", __func__);
			break;
		case POWER_SUPPLY_PROP_INPUT_CURRENT_LIMIT:
			ret = boe_get_input_current(chip->chg_dev, &val->intval);
			if (ret < 0)
				dev_err(chip->dev, "%s: boe_get_input_current fail\n", __func__);
			break;
		case POWER_SUPPLY_PROP_INPUT_VOLTAGE_LIMIT:
			ret = boe_get_mivr(chip->chg_dev, &val->intval);
			if (ret < 0)
				dev_err(chip->dev, "%s: boe_get_mivr fail\n", __func__);
			break;
		case POWER_SUPPLY_PROP_CHARGE_TERM_CURRENT:
			ret = boe_get_eoc_current(chip->chg_dev, &val->intval);
			if (ret < 0)
				dev_err(chip->dev, "%s: boe_get_eoc_current fail\n", __func__);
			break;
		case POWER_SUPPLY_PROP_TYPE:
			val->intval = chip->psy_desc.type;
			break;
		case POWER_SUPPLY_PROP_USB_TYPE:
			val->intval = chip->psy_usb_type;
			break;
		case POWER_SUPPLY_PROP_VOLTAGE_NOW:
			val->intval = boe_get_vbus(chip);
			break;
		case POWER_SUPPLY_PROP_CURRENT_MAX:
			if (chip->psy_desc.type == POWER_SUPPLY_TYPE_USB)
				val->intval = 500000;
			else if (chip->psy_desc.type == POWER_SUPPLY_TYPE_USB_DCP)
				val->intval = 2200000;
			break;
		case POWER_SUPPLY_PROP_VOLTAGE_MAX:
			if (chip->psy_desc.type == POWER_SUPPLY_TYPE_USB)
				val->intval = 5000000;
			else if (chip->psy_desc.type == POWER_SUPPLY_TYPE_USB_DCP)
				val->intval = 9000000;
			break;
		default:
			return -EINVAL;
	}

	return ret;
}

static int boe_set_property(struct power_supply *psy,
		enum power_supply_property psp,
		const union power_supply_propval *val)
{
	int ret = 0;
	struct boe_chg *chip = power_supply_get_drvdata(psy);

	if (!chip) {
		pr_err("%s, chip is NULL\n", __func__);
		return -EINVAL;
	}

	switch (psp) {
		case POWER_SUPPLY_PROP_ONLINE:
			dev_info(chip->dev, "%s, attach=%d\n", __func__, val->intval);
			chip->attach_type = val->intval;
			schedule_delayed_work(&chip->charge_detect_work, msecs_to_jiffies(900));
			break;
		case POWER_SUPPLY_PROP_STATUS:
			ret = boe_enable(chip->chg_dev, val->intval);
			break;
		case POWER_SUPPLY_PROP_CONSTANT_CHARGE_CURRENT:
			ret = boe_set_charging_current(chip->chg_dev, val->intval);
			break;
		case POWER_SUPPLY_PROP_CONSTANT_CHARGE_VOLTAGE:
			ret = boe_set_constant_voltage(chip->chg_dev, val->intval);
			break;
		case POWER_SUPPLY_PROP_INPUT_CURRENT_LIMIT:
			ret = boe_set_input_current(chip->chg_dev, val->intval);
			break;
		case POWER_SUPPLY_PROP_INPUT_VOLTAGE_LIMIT:
			ret = boe_set_mivr(chip->chg_dev, val->intval);
			break;
		case POWER_SUPPLY_PROP_CHARGE_TERM_CURRENT:
			ret = boe_set_eoc_current(chip->chg_dev, val->intval);
			break;
		default:
			return -EINVAL;
	}

	return ret;
}

static int boe_property_is_writeable(struct power_supply *psy,
		enum power_supply_property psp)
{
	switch (psp) {
		case POWER_SUPPLY_PROP_CONSTANT_CHARGE_CURRENT:
		case POWER_SUPPLY_PROP_CONSTANT_CHARGE_VOLTAGE:
		case POWER_SUPPLY_PROP_INPUT_CURRENT_LIMIT:
		case POWER_SUPPLY_PROP_INPUT_VOLTAGE_LIMIT:
		case POWER_SUPPLY_PROP_CHARGE_TERM_CURRENT:
		case POWER_SUPPLY_PROP_STATUS:
		case POWER_SUPPLY_PROP_ONLINE:
			return 1;
		default:
			return 0;
	}
}

static const struct power_supply_desc boe_psy_desc = {
	.name = "ext_charger_type",
	.type = POWER_SUPPLY_TYPE_USB,
	.usb_types = boe_psy_usb_types,
	.num_usb_types = ARRAY_SIZE(boe_psy_usb_types),
	.properties = boe_psy_properties,
	.num_properties = ARRAY_SIZE(boe_psy_properties),
	.property_is_writeable = boe_property_is_writeable,
	.get_property = boe_get_property,
	.set_property = boe_set_property,
};

static int boe_init_psy(struct boe_chg *chip)
{
	struct power_supply_config cfg = {
		.drv_data = chip,
		.of_node = chip->dev->of_node,
		.supplied_to = boe_psy_supplied_to,
		.num_supplicants = ARRAY_SIZE(boe_psy_supplied_to),
	};

	memcpy(&chip->psy_desc, &boe_psy_desc, sizeof(chip->psy_desc));
	chip->psy = devm_power_supply_register(chip->dev, &chip->psy_desc, &cfg);

	return IS_ERR(chip->psy) ? PTR_ERR(chip->psy) : 0;
}

static int boe_init_regulator(struct boe_chg *chip)
{
	struct regulator_config cfg = {
		.dev = chip->dev,
		.driver_data = chip,
	};

	memcpy(&chip->rdesc, &boe_otg_rdesc, sizeof(chip->rdesc));
	chip->rdesc.name = dev_name(chip->dev);
	chip->rdev = devm_regulator_register(chip->dev, &chip->rdesc, &cfg);
	return IS_ERR(chip->rdev) ? PTR_ERR(chip->rdev) : 0;
}

static int boe_init_chg_dev(struct boe_chg *chip)
{
	chip->chg_dev = charger_device_register(chip->chg_name, chip->dev,
			chip, &boe_chg_ops,
			&chip->chg_props);
	return IS_ERR(chip->chg_dev) ? PTR_ERR(chip->chg_dev) : 0;
}

static int get_state_reg(struct boe_chg *chip)
{
	u8 reg_val;
	struct state_reg state;

	reg_val = boe_read_byte(chip->client, REG8);
	if (reg_val < 0) {
		dev_err(chip->dev, "%s read state reg fail\n", __func__);
		return reg_val;
	}

	state.vbus_stat = (reg_val & REG8_VBUS_STAT_MASK) >> REG8_VBUS_STAT_SHIFT;
	state.chrg_stat = (reg_val & REG8_CHRG_STAT_MASK) >> REG8_CHRG_STAT_SHIFT;
	state.pg_stat = (reg_val & REG8_PG_STAT_MASK) >> REG8_PG_STAT_SHIFT;

	switch (state.vbus_stat) {
		case REG8_VBUS_TYPE_NONE:
			chip->psy_desc.type = POWER_SUPPLY_TYPE_UNKNOWN;
			chip->psy_usb_type = POWER_SUPPLY_USB_TYPE_UNKNOWN;
			break;
		case REG8_VBUS_TYPE_UNKNOWN:
			chip->psy_desc.type = POWER_SUPPLY_TYPE_USB;
			chip->psy_usb_type = POWER_SUPPLY_USB_TYPE_UNKNOWN;
			break;
		case REG8_VBUS_TYPE_SDP:
			chip->psy_desc.type = POWER_SUPPLY_TYPE_USB;
			chip->psy_usb_type = POWER_SUPPLY_USB_TYPE_SDP;
			break;
		case REG8_VBUS_TYPE_CDP:
			chip->psy_desc.type = POWER_SUPPLY_TYPE_USB_CDP;
			chip->psy_usb_type = POWER_SUPPLY_USB_TYPE_CDP;
			break;
		case REG8_VBUS_TYPE_DCP:
		case REG8_VBUS_TYPE_NON_STD:
			chip->psy_desc.type = POWER_SUPPLY_TYPE_USB_DCP;
			chip->psy_usb_type = POWER_SUPPLY_USB_TYPE_DCP;
			break;
		default:
			chip->psy_desc.type = POWER_SUPPLY_TYPE_UNKNOWN;
			chip->psy_usb_type = POWER_SUPPLY_USB_TYPE_UNKNOWN;
			break;
	}

	if (chip->attach_type == ATTACH_TYPE_PD || chip->attach_type == ATTACH_TYPE_PD_DCP ||
			chip->attach_type == ATTACH_TYPE_PD_NONSTD) {
		chip->psy_desc.type = POWER_SUPPLY_TYPE_USB_DCP;
		chip->psy_usb_type = POWER_SUPPLY_USB_TYPE_PD;
	} else if (chip->attach_type == ATTACH_TYPE_PD_SDP) {
		chip->psy_desc.type = POWER_SUPPLY_TYPE_USB;
		chip->psy_usb_type = POWER_SUPPLY_USB_TYPE_PD;
	}

	dev_info(chip->dev, "%s: reg8:0x%x online:%d chg_type:0x%x chg_stat:0x%x\n",
			__func__, reg_val, state.pg_stat, state.vbus_stat, state.chrg_stat);

	chip->state = state;

	return 0;
}

static void charger_detect_work_func(struct work_struct *work)
{
	int ret;
	u8 vbus_gd;
	struct boe_chg *chip = container_of(work, struct boe_chg, charge_detect_work.work);

	if (!chip) {
		pr_err("%s: chip is NULL\n", __func__);
		return;
	}

	dev_info(chip->dev, "%s enter\n", __func__);

	chip->last_vbus_gd = chip->vbus_gd;
	ret = boe_read_interface(chip->client, REG10, &vbus_gd, REG10_VBUS_GD_MASK, REG10_VBUS_GD_SHIFT);
	if (ret < 0) {
		dev_err(chip->dev, "%s: read vbus_gd fail\n", __func__);
		return;
	}
	chip->vbus_gd = vbus_gd;

	dev_info(chip->dev, "%s: vbus_gd:%d last_vbus_gd:%d\n",
			__func__, chip->vbus_gd, chip->last_vbus_gd);

	if (!chip->last_vbus_gd && chip->vbus_gd) {
		dev_info(chip->dev, "%s: adapter/usb plug in\n", __func__);
		boe_enable(chip->chg_dev, true);
		msleep(10);
	} else if (chip->last_vbus_gd && !chip->vbus_gd) {
		dev_info(chip->dev, "%s: adapter/usb plug out\n", __func__);
		boe_enable(chip->chg_dev, false);
	}

	get_state_reg(chip);

	power_supply_changed(chip->psy);
	boe_dump_register(chip->chg_dev);
}

static irqreturn_t boe_irq_handler_thread(int irq, void *private)
{
	struct  boe_chg *chip = private;

	dev_info(chip->dev, "%s entry\n", __func__);

	schedule_delayed_work(&chip->charge_detect_work, msecs_to_jiffies(900));

	return IRQ_HANDLED;
}

static int boe_probe(struct i2c_client *client,
		const struct i2c_device_id *id)
{
	int ret;
	struct boe_chg *chip;
	struct device *dev = &client->dev;

	dev_info(dev, "%s: driver version: %s\n", __func__, BOE_CHARGER_VER);

	chip = devm_kzalloc(dev, sizeof(*chip), GFP_KERNEL);
	if (!chip)
		return -ENOMEM;

	chip->client = client;
	chip->dev = dev;
	i2c_set_clientdata(client, chip);

	mutex_init(&chip->i2c_lock);

	if (!boe_check_devinfo(chip))
		return -ENODEV;

	ret = boe_parse_dts(chip);
	if (ret < 0) {
		dev_err(chip->dev, "%s parse dt fail(%d)\n", __func__, ret);
		return ret;
	}

	ret = boe_init_setting(chip);
	if (ret < 0)
		goto out;

	if (chip->is_primary_chg) {
		ret = boe_init_psy(chip);
		if (ret < 0) {
			dev_err(chip->dev, "%s failed to init power supply\n", __func__);
			goto out;
		}

		ret = boe_init_regulator(chip);
		if (ret < 0) {
			dev_err(chip->dev, "%s, failed to init regulator\n", __func__);
			goto out;
		}

		INIT_DELAYED_WORK(&chip->charge_detect_work, charger_detect_work_func);

		if (client->irq) {
			ret = devm_request_threaded_irq(dev, client->irq, NULL,
					boe_irq_handler_thread,
					IRQF_TRIGGER_FALLING | IRQF_NO_SUSPEND |
					IRQF_ONESHOT,
					dev_name(&client->dev), chip);
			if (ret) {
				dev_err(chip->dev, "devm_request_threaded_irq fail\n", __func__);
				goto out;
			} else
				dev_info(chip->dev, "%s: request thread irq ok, irq = %d\n", __func__, client->irq);

			enable_irq_wake(client->irq);
		}
	}

	ret = boe_init_chg_dev(chip);
	if (ret < 0) {
		dev_err(chip->dev, "%s failed to init chg_dev\n", __func__);
		goto out;
	}

	ret = device_create_file(dev, &dev_attr_dump_reg);
	if (ret < 0) {
		dev_err(chip->dev, "%s create file fail(%d)\n",
				__func__, ret);
		goto create_file_err;
	}

	chip->disable_charge = false;

	boe_dump_register(chip->chg_dev);

	dev_info(chip->dev, "%s successfully\n", __func__);

	return ret;

create_file_err:
	charger_device_unregister(chip->chg_dev);
out:
	mutex_destroy(&chip->i2c_lock);
	return ret;
}

static int boe_remove(struct i2c_client *client)
{
	struct boe_chg *chip = i2c_get_clientdata(client);

	if (chip) {
		device_remove_file(chip->dev, &dev_attr_dump_reg);
		charger_device_unregister(chip->chg_dev);
		mutex_destroy(&chip->i2c_lock);
	}

	return 0;
}

static const struct of_device_id boe_of_match[] = {
	{.compatible = "sgm,sgm41513d"},
	{.compatible = "sgm,sgm41513"},
	{},
};

static struct i2c_driver boe_i2c_driver = {
	.probe = boe_probe,
	.remove = boe_remove,
	.driver = {
		.name = "boe_chg",
		.of_match_table = boe_of_match,
	},
};
module_i2c_driver(boe_i2c_driver);

MODULE_AUTHOR("JiangWei Huang <huangjiangwei@boe.com.cn>");
MODULE_DESCRIPTION("BOE general charger driver");
MODULE_LICENSE("GPL");
