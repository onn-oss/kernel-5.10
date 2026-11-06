// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#ifndef BUILD_LK
#include <linux/interrupt.h>
#include <linux/i2c.h>
#include <linux/i2c-dev.h>
#include <linux/slab.h>
#include <linux/irq.h>
#include <linux/miscdevice.h>
#include <linux/uaccess.h>
#include <linux/delay.h>
#include <linux/input.h>
#include <linux/workqueue.h>
#include <linux/kobject.h>
#include <linux/platform_device.h>
#include <linux/atomic.h>
#include <linux/of.h>
#include <linux/module.h>

#include "mp3314g.h"

/* I2C Slave Setting */
#define mp3314g_SLAVE_ADDR_WRITE	0x28
bool MP3314G_IC=false;

static struct i2c_client *mp3314g_client;
static const struct i2c_device_id mp3314g_i2c_id[] = { {"mp3314g", 0}, {} };

static int mp3314g_driver_probe(struct i2c_client *client,
	const struct i2c_device_id *id);
static int mp3314g_driver_remove(struct i2c_client *client);

#ifdef CONFIG_OF
static const struct of_device_id mp3314g_id[] = {
	{.compatible = "mediatek,mp3314g"},
	{},
};

MODULE_DEVICE_TABLE(of, mp3314g_id);
#endif

static struct i2c_driver mp3314g_driver = {
	.driver = {
		.owner = THIS_MODULE,
		.name = "mp3314g",
#ifdef CONFIG_OF
		.of_match_table = of_match_ptr(mp3314g_id),
#endif
	},
	.probe = mp3314g_driver_probe,
	.remove = mp3314g_driver_remove,
	.id_table = mp3314g_i2c_id,
};

static DEFINE_MUTEX(mp3314g_i2c_access);

/* I2C Function For Read/Write */
int mp3314g_read_byte(unsigned char cmd, unsigned char *returnData)
{
	char cmd_buf[2] = { 0x00, 0x00 };
	char readData = 0;
	int ret = 0;

	mutex_lock(&mp3314g_i2c_access);

	cmd_buf[0] = cmd;
	ret = i2c_master_send(mp3314g_client, &cmd_buf[0], 1);
	ret = i2c_master_recv(mp3314g_client, &cmd_buf[1], 1);
	if (ret < 0) {
		mutex_unlock(&mp3314g_i2c_access);
		return 0;
	}

	readData = cmd_buf[1];
	*returnData = readData;

	mutex_unlock(&mp3314g_i2c_access);

	return 1;
}

int mp3314g_write_byte(unsigned char addr, unsigned char value)
{
	#if 0
	char write_data[2] = { 0 };
	int ret = 0;

	pr_notice("[KE/mp3314g] cmd: %02x, data: %02x,%s\n", cmd, writeData, __func__);

	mutex_lock(&mp3314g_i2c_access);

	write_data[0] = cmd;
	write_data[1] = writeData;

	ret = i2c_master_send(mp3314g_client, write_data, 2);
	if (ret < 0) {
		mutex_unlock(&mp3314g_i2c_access);
		pr_notice("[mp3314g] I2C write fail!!!\n");

		return 0;
	}

	mutex_unlock(&mp3314g_i2c_access);

	return 1;
	#endif 
	
	int ret = 0;
	struct i2c_client *client = mp3314g_client;
	char write_data[2] = { 0 };
	if (client == NULL) {
		pr_info("ERROR!! mp3314g_client is null\n");
		return 0;
	}
	write_data[0] = addr;
	write_data[1] = value;
	ret = i2c_master_send(client, write_data, 2);
	if (ret < 0)
		pr_info("ERROR %d!! i2c write data fail 0x%0x, 0x%0x !!\n",
				ret, addr, value);
	return ret;
}

EXPORT_SYMBOL(mp3314g_write_byte);

int mp3314g_backlight_init(void)
{
	mp3314g_write_byte(0x00,0x00);
	mp3314g_write_byte(0x01,0x00);
	mp3314g_write_byte(0x02,0x9d);
	mp3314g_write_byte(0x03,0x7f);
	mp3314g_write_byte(0x04,0xb8);
	mp3314g_write_byte(0x05,0x00);
	mp3314g_write_byte(0x06,0x5b);
	mp3314g_write_byte(0x07,0x5f);
	mp3314g_write_byte(0x08,0xa0);
	mp3314g_write_byte(0x09,0x3e);
	mp3314g_write_byte(0x0a,0x90);
	mp3314g_write_byte(0x0c,0x00);
	mp3314g_write_byte(0x0d,0x00);
	mp3314g_write_byte(0x0f,0x3c);
	mp3314g_write_byte(0x10,0x80);

	return 1;
}

EXPORT_SYMBOL(mp3314g_backlight_init);

static int mp3314g_driver_probe(struct i2c_client *client,
	const struct i2c_device_id *id)
{
	int err = 0;
	unsigned char data = 0;

	pr_notice("[KE/mp3314g] name=%s addr=0x%x\n",
		client->name, client->addr);
	mp3314g_client = kmalloc(sizeof(struct i2c_client), GFP_KERNEL);
	if (!mp3314g_client) {
		err = -ENOMEM;
		goto exit;
	}

	memset(mp3314g_client, 0, sizeof(struct i2c_client));
	mp3314g_client = client;

	mp3314g_read_byte(0x1D, &data);
	pr_notice("[KE/mp3314g] devices id =0x%x\n", data);
	if (data == 0x11)
		MP3314G_IC = true;
	else
		MP3314G_IC = false;

	return 0;

 exit:
	return err;

}

static int mp3314g_driver_remove(struct i2c_client *client)
{
	pr_notice("[KE/mp3314g] %s\n", __func__);

	mp3314g_client = NULL;
	i2c_unregister_device(client);

	return 0;
}

#define mp3314g_BUSNUM 5

static int __init mp3314g_init(void)
{
	pr_notice("[KE/mp3314g] %s\n", __func__);

	if (i2c_add_driver(&mp3314g_driver) != 0)
		pr_notice("[KE/mp3314g] failed to register mp3314g i2c driver.\n");
	else
		pr_notice("[KE/mp3314g] Success to register mp3314g i2c driver.\n");

	return 0;
}

static void __exit mp3314g_exit(void)
{
	i2c_del_driver(&mp3314g_driver);
}

module_init(mp3314g_init);
module_exit(mp3314g_exit);
MODULE_LICENSE("GPL");
#endif
