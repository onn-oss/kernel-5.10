// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#define pr_fmt(fmt) KBUILD_MODNAME ": %s: " fmt, __func__

#include <linux/types.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/platform_device.h>
#include <linux/hrtimer.h>
#include <linux/ktime.h>
#include <linux/workqueue.h>
#include <linux/mutex.h>
#include <linux/of.h>
#include <linux/list.h>
#include <linux/delay.h>
#include <linux/pinctrl/consumer.h>

#include "flashlight-core.h"
#include "flashlight-dt.h"

/* define device tree */
/* TODO: modify temp device tree name */
#ifndef LED_GPIO_DTNAME
#define LED_GPIO_DTNAME "mediatek,flashlights_led_gpio"
#endif

/* TODO: define driver name */
#define LED_NAME "flashlights_led_gpio"

/* define registers */
/* TODO: define register */

/* define mutex and work queue */
static DEFINE_MUTEX(led_mutex);
static struct work_struct led_work;

/* define pinctrl */
/* TODO: define pinctrl */
#define LED_PINCTRL_PIN_HWEN 0
#define LED_PINCTRL_PINSTATE_LOW 0
#define LED_PINCTRL_PINSTATE_HIGH 1
#define LED_PINCTRL_STATE_HWEN_HIGH "hwen_high"
#define LED_PINCTRL_STATE_HWEN_LOW  "hwen_low"
static struct pinctrl *led_pinctrl;
static struct pinctrl_state *led_hwen_high;
static struct pinctrl_state *led_hwen_low;

/* define usage count */
static int use_count;

/* platform data */
struct led_platform_data {
	int channel_num;
	struct flashlight_device_id *dev_id;
};


/******************************************************************************
 * Pinctrl configuration
 *****************************************************************************/
static int led_pinctrl_init(struct platform_device *pdev)
{
	int ret = 0;

	/* get pinctrl */
	led_pinctrl = devm_pinctrl_get(&pdev->dev);
	if (IS_ERR(led_pinctrl)) {
		pr_info("Failed to get flashlight pinctrl.\n");
		ret = PTR_ERR(led_pinctrl);
		return ret;
	}

	/* TODO: Flashlight HWEN pin initialization */
	led_hwen_high = pinctrl_lookup_state(
			led_pinctrl, LED_PINCTRL_STATE_HWEN_HIGH);
	if (IS_ERR(led_hwen_high)) {
		pr_info("Failed to init (%s)\n", LED_PINCTRL_STATE_HWEN_HIGH);
		ret = PTR_ERR(led_hwen_high);
	}
	led_hwen_low = pinctrl_lookup_state(
			led_pinctrl, LED_PINCTRL_STATE_HWEN_LOW);
	if (IS_ERR(led_hwen_low)) {
		pr_info("Failed to init (%s)\n", LED_PINCTRL_STATE_HWEN_LOW);
		ret = PTR_ERR(led_hwen_low);
	}

	return ret;
}

static int led_pinctrl_set(int pin, int state)
{
	int ret = 0;

	if (IS_ERR(led_pinctrl)) {
		pr_info("pinctrl is not available\n");
		return -1;
	}
	pr_info("led_pinctrl_set, pin(%d) state(%d)\n", pin, state);
	switch (pin) {
	case LED_PINCTRL_PIN_HWEN:
		if (state == LED_PINCTRL_PINSTATE_LOW &&
				!IS_ERR(led_hwen_low))
			pinctrl_select_state(led_pinctrl, led_hwen_low);
		else if (state == LED_PINCTRL_PINSTATE_HIGH &&
				!IS_ERR(led_hwen_high))
			pinctrl_select_state(led_pinctrl, led_hwen_high);
		else
			pr_info("set err, pin(%d) state(%d)\n", pin, state);
		break;
	default:
		pr_info("set err, pin(%d) state(%d)\n", pin, state);
		break;
	}
	pr_debug("pin(%d) state(%d)\n", pin, state);

	return ret;
}


/******************************************************************************
 * led operations
 *****************************************************************************/
/* flashlight enable function */
static int led_enable(void)
{
	int pin = 0, state = 1;

	/* TODO: wrap enable function */

	return led_pinctrl_set(pin, state);
}

/* flashlight disable function */
static int led_disable(void)
{
	int pin = 0, state = 0;

	/* TODO: wrap disable function */

	return led_pinctrl_set(pin, state);
}

/* set flashlight level */
static int led_set_level(int level)
{

	return 0;
}

/* flashlight init */
static int led_init(void)
{
	return 0;
}

/* flashlight uninit */
static int led_uninit(void)
{
	int pin = 0, state = 0;

	/* TODO: wrap uninit function */

	return led_pinctrl_set(pin, state);
}

/******************************************************************************
 * Timer and work queue
 *****************************************************************************/
static struct hrtimer led_timer;
static unsigned int led_timeout_ms;

static void led_work_disable(struct work_struct *data)
{
	pr_debug("work queue callback\n");
	led_disable();
}

static enum hrtimer_restart led_timer_func(struct hrtimer *timer)
{
	schedule_work(&led_work);
	return HRTIMER_NORESTART;
}


/******************************************************************************
 * Flashlight operations
 *****************************************************************************/
static int led_ioctl(unsigned int cmd, unsigned long arg)
{
	struct flashlight_dev_arg *fl_arg;
	int channel;
	ktime_t ktime;
	unsigned int s;
	unsigned int ns;

	fl_arg = (struct flashlight_dev_arg *)arg;
	channel = fl_arg->channel;

	switch (cmd) {
	case FLASH_IOC_SET_TIME_OUT_TIME_MS:
		pr_debug("FLASH_IOC_SET_TIME_OUT_TIME_MS(%d): %d\n",
				channel, (int)fl_arg->arg);
		led_timeout_ms = fl_arg->arg;
		break;

	case FLASH_IOC_SET_DUTY:
		pr_debug("FLASH_IOC_SET_DUTY(%d): %d\n",
				channel, (int)fl_arg->arg);
		led_set_level(fl_arg->arg);
		break;

	case FLASH_IOC_SET_ONOFF:
		pr_debug("FLASH_IOC_SET_ONOFF(%d): %d\n",
				channel, (int)fl_arg->arg);
		if (fl_arg->arg == 1) {
			if (led_timeout_ms) {
				s = led_timeout_ms / 1000;
				ns = led_timeout_ms % 1000 * 1000000;
				ktime = ktime_set(s, ns);
				hrtimer_start(&led_timer, ktime,
						HRTIMER_MODE_REL);
			}
			led_enable();
		} else {
			led_disable();
			hrtimer_cancel(&led_timer);
		}
		break;
	default:
		pr_info("No such command and arg(%d): (%d, %d)\n",
				channel, _IOC_NR(cmd), (int)fl_arg->arg);
		return -ENOTTY;
	}

	return 0;
}

static int led_open(void)
{
	return 0;
}

static int led_release(void)
{
	return 0;
}

static int led_set_driver(int set)
{
	int ret = 0;

	/* set chip and usage count */
	mutex_lock(&led_mutex);
	if (set) {
		if (!use_count)
			ret = led_init();
		use_count++;
		pr_debug("Set driver: %d\n", use_count);
	} else {
		use_count--;
		if (!use_count)
			ret = led_uninit();
		if (use_count < 0)
			use_count = 0;
		pr_debug("Unset driver: %d\n", use_count);
	}
	mutex_unlock(&led_mutex);

	return ret;
}

static ssize_t led_strobe_store(struct flashlight_arg arg)
{
	led_set_driver(1);
	led_set_level(arg.level);
	led_timeout_ms = 0;
	led_enable();
	msleep(arg.dur);
	led_disable();
	led_set_driver(0);

	return 0;
}

static struct flashlight_operations led_ops = {
	led_open,
	led_release,
	led_ioctl,
	led_strobe_store,
	led_set_driver
};


/******************************************************************************
 * Platform device and driver
 *****************************************************************************/
static int led_chip_init(void)
{
	/* NOTE: Chip initialication move to "set driver" for power saving.
	 * led_init();
	 */

	return 0;
}

static int led_parse_dt(struct device *dev,
		struct led_platform_data *pdata)
{
	struct device_node *np, *cnp;
	u32 decouple = 0;
	int i = 0;

	if (!dev || !dev->of_node || !pdata)
		return -ENODEV;

	np = dev->of_node;

	pdata->channel_num = of_get_child_count(np);
	if (!pdata->channel_num) {
		pr_info("Parse no dt, node.\n");
		return 0;
	}
	pr_info("Channel number(%d).\n", pdata->channel_num);

	if (of_property_read_u32(np, "decouple", &decouple))
		pr_info("Parse no dt, decouple.\n");

	pdata->dev_id = devm_kzalloc(dev,
			pdata->channel_num *
			sizeof(struct flashlight_device_id),
			GFP_KERNEL);
	if (!pdata->dev_id)
		return -ENOMEM;

	for_each_child_of_node(np, cnp) {
		if (of_property_read_u32(cnp, "type", &pdata->dev_id[i].type))
			goto err_node_put;
		if (of_property_read_u32(cnp, "ct", &pdata->dev_id[i].ct))
			goto err_node_put;
		if (of_property_read_u32(cnp, "part", &pdata->dev_id[i].part))
			goto err_node_put;
		snprintf(pdata->dev_id[i].name, FLASHLIGHT_NAME_SIZE,
				LED_NAME);
		pdata->dev_id[i].channel = i;
		pdata->dev_id[i].decouple = decouple;

		pr_info("Parse dt (type,ct,part,name,channel,decouple)=(%d,%d,%d,%s,%d,%d).\n",
				pdata->dev_id[i].type, pdata->dev_id[i].ct,
				pdata->dev_id[i].part, pdata->dev_id[i].name,
				pdata->dev_id[i].channel,
				pdata->dev_id[i].decouple);
		i++;
	}

	return 0;

err_node_put:
	of_node_put(cnp);
	return -EINVAL;
}

static int led_probe(struct platform_device *pdev)
{
	struct led_platform_data *pdata = dev_get_platdata(&pdev->dev);
	int err;
	int i;

	pr_debug("Probe start.\n");

	/* init pinctrl */
	if (led_pinctrl_init(pdev)) {
		pr_debug("Failed to init pinctrl.\n");
		err = -EFAULT;
		goto err;
	}

	/* init platform data */
	if (!pdata) {
		pdata = devm_kzalloc(&pdev->dev, sizeof(*pdata), GFP_KERNEL);
		if (!pdata) {
			err = -ENOMEM;
			goto err;
		}
		pdev->dev.platform_data = pdata;
		err = led_parse_dt(&pdev->dev, pdata);
		if (err)
			goto err;
	}

	/* init work queue */
	INIT_WORK(&led_work, led_work_disable);

	/* init timer */
	hrtimer_init(&led_timer, CLOCK_MONOTONIC, HRTIMER_MODE_REL);
	led_timer.function = led_timer_func;
	led_timeout_ms = 100;

	/* init chip hw */
	led_chip_init();

	/* clear usage count */
	use_count = 0;

	/* register flashlight device */
	if (pdata->channel_num) {
		for (i = 0; i < pdata->channel_num; i++)
			if (flashlight_dev_register_by_device_id(
						&pdata->dev_id[i],
						&led_ops)) {
				err = -EFAULT;
				goto err;
			}
	} else {
		if (flashlight_dev_register(LED_NAME, &led_ops)) {
			err = -EFAULT;
			goto err;
		}
	}

	pr_debug("Probe done.\n");

	return 0;
err:
	return err;
}

static int led_remove(struct platform_device *pdev)
{
	struct led_platform_data *pdata = dev_get_platdata(&pdev->dev);
	int i;

	pr_debug("Remove start.\n");

	pdev->dev.platform_data = NULL;

	/* unregister flashlight device */
	if (pdata && pdata->channel_num)
		for (i = 0; i < pdata->channel_num; i++)
			flashlight_dev_unregister_by_device_id(
					&pdata->dev_id[i]);
	else
		flashlight_dev_unregister(LED_NAME);

	/* flush work queue */
	flush_work(&led_work);

	pr_debug("Remove done.\n");

	return 0;
}

#ifdef CONFIG_OF
static const struct of_device_id led_gpio_of_match[] = {
	{.compatible = LED_GPIO_DTNAME},
	{},
};
MODULE_DEVICE_TABLE(of, led_gpio_of_match);
#else
static struct platform_device led_gpio_platform_device[] = {
	{
		.name = LED_NAME,
		.id = 0,
		.dev = {}
	},
	{}
};
MODULE_DEVICE_TABLE(platform, led_gpio_platform_device);
#endif

static struct platform_driver led_platform_driver = {
	.probe = led_probe,
	.remove = led_remove,
	.driver = {
		.name = LED_NAME,
		.owner = THIS_MODULE,
#ifdef CONFIG_OF
		.of_match_table = led_gpio_of_match,
#endif
	},
};

static int __init flashlight_led_init(void)
{
	int ret;

	pr_debug("Init start.\n");

#ifndef CONFIG_OF
	ret = platform_device_register(&led_gpio_platform_device);
	if (ret) {
		pr_info("Failed to register platform device\n");
		return ret;
	}
#endif

	ret = platform_driver_register(&led_platform_driver);
	if (ret) {
		pr_info("Failed to register platform driver\n");
		return ret;
	}

	pr_debug("Init done.\n");

	return 0;
}

static void __exit flashlight_led_exit(void)
{
	pr_debug("Exit start.\n");

	platform_driver_unregister(&led_platform_driver);

	pr_debug("Exit done.\n");
}

module_init(flashlight_led_init);
module_exit(flashlight_led_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Simon Wang <Simon-TCH.Wang@mediatek.com>");
MODULE_DESCRIPTION("MTK Flashlight LED GPIO Driver");

