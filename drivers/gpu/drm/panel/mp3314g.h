/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#ifndef _mp3314g_SW_H_
#define _mp3314g_SW_H_

#ifndef BUILD_LK
struct mp3314g_setting_table {
	unsigned char cmd;
	unsigned char data;
};

extern int mp3314g_read_byte(unsigned char cmd, unsigned char *returnData);
extern int mp3314g_write_byte(unsigned char cmd, unsigned char writeData);
extern int mp3314g_backlight_init(void);
#endif

#endif
