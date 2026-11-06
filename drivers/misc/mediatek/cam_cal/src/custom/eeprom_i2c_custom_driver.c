// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#define PFX "CAM_CAL"
#define pr_fmt(fmt) PFX "[%s] " fmt, __func__


#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/i2c.h>
#include <linux/platform_device.h>
#include <linux/delay.h>
#include <linux/cdev.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/of.h>
#include "cam_cal.h"
#include "cam_cal_define.h"
#include <linux/dma-mapping.h>
#ifdef CONFIG_COMPAT
/* 64 bit */
#include <linux/fs.h>
#include <linux/compat.h>
#endif

#include "eeprom_utils.h"
#include "kd_camera_typedef.h"

#define EEPROM_I2C_MSG_SIZE_READ 2

/************************************************************
 * I2C read function (Custom)
 * Customer's driver can put on here
 * Below is an example
 ************************************************************/
 #define PAGE_SIZE_ 256

#define INFO_FLAG_ADDR              0x15A0
#define AWBINFO_FLAG_ADDR           0x19C8
#define AFINFO_FLAG_ADDR            0x1B68
#define LSCINFO_FLAG_ADDR           0x1BE8

extern struct gc08a8_otp_struct gc08a8_otp;

struct gc08a8_otp_struct {
	kal_uint8 af_group_flag;
	kal_uint8 af_info[8];
	kal_uint8 af_checksum;
	kal_uint8 awb_group_flag;
	kal_uint8 awb_info[12];
	kal_uint8 awb_checksum;
	kal_uint8 lsc_group_flag;
	kal_uint8 lsc_info[1868];
	kal_uint8 lsc_checksum;
	kal_uint8 info_flag;
};

unsigned int gc08a8_read_region(struct i2c_client *client, unsigned int addr,
			unsigned char *data, unsigned int size)
{
	if(addr == 0 && size == 0x1500) //Avoiding preloading, hal print error log "Preload data failed"
		return 0;
	else if(addr == INFO_FLAG_ADDR && size == 1)
		memcpy(data, &(gc08a8_otp.info_flag), size * sizeof(kal_uint8));
	else if(addr == AFINFO_FLAG_ADDR && size == 4)
		memcpy(data, gc08a8_otp.af_info, size * sizeof(kal_uint8));
	else if(addr == LSCINFO_FLAG_ADDR && size == 1868)
		memcpy(data, gc08a8_otp.lsc_info, size * sizeof(kal_uint8));
	else if(addr == AWBINFO_FLAG_ADDR && size == 8)
		memcpy(data, gc08a8_otp.awb_info, size * sizeof(kal_uint8));

	return size;
}

static int iReadRegI2C(struct i2c_client *client,
		u8 *a_pSendData, u16 a_sizeSendData,
		u8 *a_pRecvData, u16 a_sizeRecvData, u16 i2cId)
{
	int i4RetValue = 0;
	struct i2c_msg msg[EEPROM_I2C_MSG_SIZE_READ];

	client->addr = (i2cId >> 1);

	msg[0].addr = client->addr;
	msg[0].flags = client->flags & I2C_M_TEN;
	msg[0].len = a_sizeSendData;
	msg[0].buf = a_pSendData;

	msg[1].addr = client->addr;
	msg[1].flags = client->flags & I2C_M_TEN;
	msg[1].flags |= I2C_M_RD;
	msg[1].len = a_sizeRecvData;
	msg[1].buf = a_pRecvData;

	i4RetValue = i2c_transfer(client->adapter, msg,
				EEPROM_I2C_MSG_SIZE_READ);

	if (i4RetValue != EEPROM_I2C_MSG_SIZE_READ) {
		must_log("I2C read failed!!\n");
		return -1;
	}
	return 0;
}

static int custom_read_region(struct i2c_client *client,
			      u32 addr, u8 *data, u16 i2c_id, u32 size)
{
	u8 *buff = data;
	u32 size_to_read = size;

	int ret = 0;

	while (size_to_read > 0) {
		u8 page = addr / PAGE_SIZE_;
		u8 offset = addr % PAGE_SIZE_;
		char *Buff = data;

		if (iReadRegI2C(client, &offset, 1, (u8 *)Buff, 1,
			i2c_id + (page << 1)) < 0) {
			must_log("fail addr=0x%x 0x%x, P=%d, offset=0x%x",
				addr, *Buff, page, offset);
			break;
		}
		addr++;
		buff++;
		size_to_read--;
		ret++;
	}
	must_log("addr =%x size %d data read = %d\n", addr, size, ret);
	return ret;
}



unsigned int Custom_read_region(struct i2c_client *client, unsigned int addr,
				unsigned char *data, unsigned int size)
{
	if (custom_read_region(client, addr, data, client->addr, size) == 0)
		return size;
	else
		return 0;
}


