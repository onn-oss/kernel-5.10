// SPDX-License-Identifier: GPL-2.0-only
/* Copyright (c) 2018-2020, The Linux Foundation. All rights reserved.
 */
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/power_supply.h>
#include <linux/regmap.h>
#include <linux/i2c.h>
#include <linux/mutex.h>
#include "fsa4480-i2c.h"
#include <linux/gpio.h>
#include <linux/of.h>
#include <linux/of_gpio.h>
#define FSA4480_I2C_NAME	"fsa4480-driver"
#define FSA4480_SWITCH_SETTINGS 0x04
#define FSA4480_SWITCH_CONTROL  0x05
#define FSA4480_SWITCH_STATUS1  0x07
#define FSA4480_SLOW_L          0x08
#define FSA4480_SLOW_R          0x09
#define FSA4480_SLOW_MIC        0x0A
#define FSA4480_SLOW_SENSE      0x0B
#define FSA4480_SLOW_GND        0x0C
#define FSA4480_DELAY_L_R       0x0D
#define FSA4480_DELAY_L_MIC     0x0E
#define FSA4480_DELAY_L_SENSE   0x0F
#define FSA4480_DELAY_L_AGND    0x10
#define FSA4480_RESET           0x1E

#define POWER_SUPPLY_TYPEC_SINK_AUDIO_ADAPTER	1
#define POWER_SUPPLY_TYPEC_NONE			0

struct fsa4480_priv {
	struct regmap *regmap;
	struct device *dev;
	struct power_supply *usb_psy;
	struct notifier_block psy_nb;
	atomic_t usbc_mode;
	struct work_struct usbc_analog_work;
	struct blocking_notifier_head fsa4480_notifier;
	struct mutex notification_lock;
	struct pinctrl *fsa4480_pinctrl;
	struct pinctrl_state *gpio_state_active;
	struct pinctrl_state *gpio_state_suspend;
};
struct fsa4480_reg_val {
	u16 reg;
	u8 val;
};
static const struct regmap_config fsa4480_regmap_config = {
	.reg_bits = 8,
	.val_bits = 8,
	.max_register = FSA4480_RESET,
};
static const struct fsa4480_reg_val fsa_reg_i2c_defaults[] = {
	{FSA4480_SLOW_L, 0x00},
	{FSA4480_SLOW_R, 0x00},
	{FSA4480_SLOW_MIC, 0x00},
	{FSA4480_SLOW_SENSE, 0x00},
	{FSA4480_SLOW_GND, 0x00},
	{FSA4480_DELAY_L_R, 0x00},
	{FSA4480_DELAY_L_MIC, 0x00},
	{FSA4480_DELAY_L_SENSE, 0x00},
	{FSA4480_DELAY_L_AGND, 0x09},
	{FSA4480_SWITCH_SETTINGS, 0x98},
};
static 	struct fsa4480_priv *fsa_priv_cc;
static void fsa4480_usbc_update_settings(struct fsa4480_priv *fsa_priv,
		u32 switch_control, u32 switch_enable)
{
	if (!fsa_priv->regmap) {
		dev_err(fsa_priv->dev, "%s: regmap invalid\n", __func__);
		return;
	}
	regmap_write(fsa_priv->regmap, FSA4480_SWITCH_SETTINGS, 0x80);
	regmap_write(fsa_priv->regmap, FSA4480_SWITCH_CONTROL, switch_control);
	/* FSA4480 chip hardware requirement */
	usleep_range(50, 55);
	regmap_write(fsa_priv->regmap, FSA4480_SWITCH_SETTINGS, switch_enable);
}

static int fsa4480_validate_display_port_settings(struct fsa4480_priv *fsa_priv)
{
	u32 switch_status = 0;
	regmap_read(fsa_priv->regmap, FSA4480_SWITCH_STATUS1, &switch_status);
	if ((switch_status != 0x23) && (switch_status != 0x1C)) {
		printk("AUX SBU1/2 switch status is invalid = %u\n",
				switch_status);
		return -EIO;
	}
	return 0;
}
/*
 * fsa4480_switch_event - configure FSA switch position based on event
 *
 * @node - phandle node to fsa4480 device
 * @event - fsa_function enum
 *
 * Returns int on whether the switch happened or not
 */
int fsa4480_switch_event(struct device_node *node,
			 enum fsa_function event)
{
	int switch_control = 0;
	struct i2c_client *client = of_find_i2c_device_by_node(node);
	struct fsa4480_priv *fsa_priv;
	if (!client)
		return -EINVAL;
	fsa_priv = (struct fsa4480_priv *)i2c_get_clientdata(client);
	if (!fsa_priv)
		return -EINVAL;
	if (!fsa_priv->regmap)
		return -EINVAL;
	switch (event) {
	case FSA_MIC_GND_SWAP:
		regmap_read(fsa_priv->regmap, FSA4480_SWITCH_CONTROL,
				&switch_control);
		if ((switch_control & 0x07) == 0x07)
			switch_control = 0x0;
		else
			switch_control = 0x7;
		fsa4480_usbc_update_settings(fsa_priv, switch_control, 0x9F);
		break;
	case FSA_USBC_ORIENTATION_CC1:
		fsa4480_usbc_update_settings(fsa_priv, 0x18, 0xF8);
		return fsa4480_validate_display_port_settings(fsa_priv);
	case FSA_USBC_ORIENTATION_CC2:
		fsa4480_usbc_update_settings(fsa_priv, 0x78, 0xF8);
		return fsa4480_validate_display_port_settings(fsa_priv);
	case FSA_USBC_DISPLAYPORT_DISCONNECTED:
		fsa4480_usbc_update_settings(fsa_priv, 0x18, 0x98);
		break;
	default:
		break;
	}
	return 0;
}
EXPORT_SYMBOL(fsa4480_switch_event);

static void fsa4480_update_reg_defaults(struct regmap *regmap)
{
	u8 i;
	for (i = 0; i < ARRAY_SIZE(fsa_reg_i2c_defaults); i++)
		regmap_write(regmap, fsa_reg_i2c_defaults[i].reg,
				   fsa_reg_i2c_defaults[i].val);
}
void set_usb_audio_sel_pin(int mode)
{
	int temp = 0;
	printk("====>boe,%s: mode = %d \n", __func__, mode);
	 switch (mode) {
		case 1:
				regmap_write(fsa_priv_cc->regmap, FSA4480_SWITCH_CONTROL, 0x00);
				/* FSA4480 chip hardware requirement */
				usleep_range(50, 55);
				regmap_write(fsa_priv_cc->regmap, FSA4480_SWITCH_SETTINGS, 0x9F);  
				usleep_range(50, 55);
				temp = 0;
			    regmap_write(fsa_priv_cc->regmap, 0x12, 0x1101);   // 1101 popo yin
			    usleep_range(50, 55);
				regmap_read(fsa_priv_cc->regmap, 0x12,&temp);
				printk("%s:  0x12  regmap = %d \n",__func__, temp);
				temp = 0;
				regmap_read(fsa_priv_cc->regmap, 0x17,&temp);
				dev_err(fsa_priv_cc->dev, "%s:  0x17 regmap = %d\n",__func__, temp);
				usleep_range(50, 55);
			break;
		case 0:
	
				printk("%s:  audio switch to usb dn dr \n",__func__);
			 	regmap_write(fsa_priv_cc->regmap, FSA4480_SWITCH_CONTROL, 0x18);
				/* FSA4480 chip hardware requirement */
				usleep_range(50, 55);
				regmap_write(fsa_priv_cc->regmap, FSA4480_SWITCH_SETTINGS, 0x98);
				usleep_range(50, 55);
			break;
			
		default:
			break;
		}
}
EXPORT_SYMBOL(set_usb_audio_sel_pin);

static int fsa4480_probe(struct i2c_client *i2c,
			 const struct i2c_device_id *id)
{
	struct fsa4480_priv *fsa_priv;
	int rc = 0;
	printk("fsa4480_probe --- enter\n");
	fsa_priv = devm_kzalloc(&i2c->dev, sizeof(*fsa_priv),
				GFP_KERNEL);
	if (!fsa_priv)
		return -ENOMEM;
	fsa_priv->dev = &i2c->dev;

	fsa_priv->regmap = devm_regmap_init_i2c(i2c, &fsa4480_regmap_config);
	if (IS_ERR_OR_NULL(fsa_priv->regmap)) {
		printk("%s: Failed to initialize regmap: %d\n",__func__, rc);
		if (!fsa_priv->regmap) {
			return -EINVAL;
		}
		return PTR_ERR(fsa_priv->regmap);
	}
	fsa4480_update_reg_defaults(fsa_priv->regmap);

	mutex_init(&fsa_priv->notification_lock);
	i2c_set_clientdata(i2c, fsa_priv);
    	fsa_priv_cc = fsa_priv;

	fsa_priv->fsa4480_notifier.rwsem =
		(struct rw_semaphore)__RWSEM_INITIALIZER
		((fsa_priv->fsa4480_notifier).rwsem);
	fsa_priv->fsa4480_notifier.head = NULL;

	pr_info("%s success\n", __func__);
	return rc;
}
static int fsa4480_remove(struct i2c_client *i2c)
{
	struct fsa4480_priv *fsa_priv =
			(struct fsa4480_priv *)i2c_get_clientdata(i2c);
	if (!fsa_priv)
		return -EINVAL;
	fsa4480_usbc_update_settings(fsa_priv, 0x18, 0x98);
	cancel_work_sync(&fsa_priv->usbc_analog_work);
	pm_relax(fsa_priv->dev);
	/* deregister from PMI */
	mutex_destroy(&fsa_priv->notification_lock);
	dev_set_drvdata(&i2c->dev, NULL);
	return 0;
}
static const struct of_device_id fsa4480_i2c_dt_match[] = {
	{
		.compatible = "qcom,fsa4480-i2c",
	},
	{}
};
static struct i2c_driver fsa4480_i2c_driver = {
	.driver = {
		.name = FSA4480_I2C_NAME,
		.of_match_table = fsa4480_i2c_dt_match,
	},
	.probe = fsa4480_probe,
	.remove = fsa4480_remove,
};
static int __init fsa4480_init(void)
{
	int rc;
	rc = i2c_add_driver(&fsa4480_i2c_driver);
	if (rc)
		printk("fsa4480: Failed to register I2C driver: %d\n", rc);
	return rc;
}
module_init(fsa4480_init);
static void __exit fsa4480_exit(void)
{
	i2c_del_driver(&fsa4480_i2c_driver);
}
module_exit(fsa4480_exit);
MODULE_DESCRIPTION("FSA4480 I2C driver");
MODULE_LICENSE("GPL v2");
