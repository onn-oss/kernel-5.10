// SPDX-License-Identifier: GPL-2.0
/*
 * Copyright (c) 2019 MediaTek Inc.
 */

#include <linux/backlight.h>
#include <drm/drm_mipi_dsi.h>
#include <drm/drm_panel.h>
#include <drm/drm_modes.h>
#include <linux/delay.h>
#include <drm/drm_connector.h>
#include <drm/drm_device.h>

#include <linux/gpio/consumer.h>
#include <linux/regulator/consumer.h>

#include <video/mipi_display.h>
#include <video/of_videomode.h>
#include <video/videomode.h>

#include <linux/module.h>
#include <linux/of_platform.h>

#include <linux/platform_device.h>
#include <linux/of_gpio.h>

#define CONFIG_MTK_PANEL_EXT
#if defined(CONFIG_MTK_PANEL_EXT)
#include "../mediatek/mediatek_v2/mtk_panel_ext.h"
#include "../mediatek/mediatek_v2/mtk_drm_graphics_base.h"
#endif

#include"lcd_bias_lp6280s.h"
#define tp_reset 442
//static char bl_tb0[] = { 0x51, 0x0f, 0xff };
static int current_fps = 60;
extern uint8_t HX_SMWP_EN;

struct boe {
	struct device *dev;
	struct drm_panel panel;
	struct backlight_device *backlight;
	struct gpio_desc *reset_gpio;
	struct gpio_desc *iovcc_gpio;
	struct gpio_desc *te_gpio;
	struct gpio_desc *te1_gpio;
	struct gpio_desc *bias_pos;
	struct gpio_desc *bias_neg;
	struct gpio_desc *backlight_en;
	bool prepared;
	bool enabled;



	int error;
};

#define boe_dcs_write_seq(ctx, seq...)                                         \
	({                                                                     \
		const u8 d[] = { seq };                                        \
		BUILD_BUG_ON_MSG(ARRAY_SIZE(d) > 64,                           \
				 "DCS sequence too big for stack");            \
		boe_dcs_write(ctx, d, ARRAY_SIZE(d));                          \
	})

#define boe_dcs_write_seq_static(ctx, seq...)                                  \
	({                                                                     \
		static const u8 d[] = { seq };                                 \
		boe_dcs_write(ctx, d, ARRAY_SIZE(d));                          \
	})

static inline struct boe *panel_to_boe(struct drm_panel *panel)
{
	return container_of(panel, struct boe, panel);
}

#ifdef PANEL_SUPPORT_READBACK
static int boe_dcs_read(struct boe *ctx, u8 cmd, void *data, size_t len)
{
	struct mipi_dsi_device *dsi = to_mipi_dsi_device(ctx->dev);
	ssize_t ret;

	if (ctx->error < 0)
		return 0;

	ret = mipi_dsi_dcs_read(dsi, cmd, data, len);
	if (ret < 0) {
		dev_info(ctx->dev, "error %d reading dcs seq:(%#x)\n", ret,
			 cmd);
		ctx->error = ret;
	}

	return ret;
}

static void boe_panel_get_data(struct boe *ctx)
{
	u8 buffer[3] = { 0 };
	static int ret;

	pr_info("%s+\n", __func__);

	if (ret == 0) {
		ret = boe_dcs_read(ctx, 0x0A, buffer, 1);
		pr_info("%s  0x%08x\n", __func__, buffer[0] | (buffer[1] << 8));
		dev_info(ctx->dev, "return %d data(0x%08x) to dsi engine\n",
			ret, buffer[0] | (buffer[1] << 8));
	}
}
#endif

static void boe_dcs_write(struct boe *ctx, const void *data, size_t len)
{
	struct mipi_dsi_device *dsi = to_mipi_dsi_device(ctx->dev);
	ssize_t ret;
	char *addr;

	if (ctx->error < 0)
		return;

	addr = (char *)data;
	if ((int)*addr < 0xB0)
		ret = mipi_dsi_dcs_write_buffer(dsi, data, len);
	else
		ret = mipi_dsi_generic_write(dsi, data, len);
	if (ret < 0) {
		dev_info(ctx->dev, "error %zd writing seq: %ph\n", ret, data);
		ctx->error = ret;
	}
}

static void boe_panel_init(struct boe *ctx)
{
	int error=0;
	ctx->reset_gpio = devm_gpiod_get(ctx->dev, "reset", GPIOD_OUT_HIGH);
	gpiod_set_value(ctx->reset_gpio, 1);
	usleep_range(10 * 1000, 15 * 1000);
	gpiod_set_value(ctx->reset_gpio, 0);
	usleep_range(10 * 1000, 15 * 1000);
	gpiod_set_value(ctx->reset_gpio, 1);
	if (gpio_is_valid(tp_reset)) {
			error = gpio_direction_output(tp_reset, 1);
			if (error) {
				pr_info("hx83122a %s HXTP \n", __func__);
			}
		}
	usleep_range(60 * 1000, 65 * 1000);
	devm_gpiod_put(ctx->dev, ctx->reset_gpio);
	boe_dcs_write_seq_static(ctx,0xB9,0x83,0x12,0x2A,0x55,0x00);
	boe_dcs_write_seq_static(ctx,0xE9,0xC4);
	boe_dcs_write_seq_static(ctx,0xD9,0xD0);
	boe_dcs_write_seq_static(ctx,0xE9,0x3F);
	boe_dcs_write_seq_static(ctx,0xB1,0x20,0x31,0x31,0x2B,0xEB,0x63,0x6B,0x43,0x38,0x38,0x00,0x00,0x15,0x33,0x61,0x40,0xCD,0xB7,0x0A,0x0A,0x15,0x98,0x00,0x88,0xC4,0xCC,0xCC,0xCC,0x0F,0x88,0x52,0x00,0x00,0xD0);
	boe_dcs_write_seq_static(ctx,0xE9,0xE5);
	boe_dcs_write_seq_static(ctx,0xB1,0xFF,0xA0);
	boe_dcs_write_seq_static(ctx,0xE9,0x3F);
	boe_dcs_write_seq_static(ctx,0xBD,0x01);
	boe_dcs_write_seq_static(ctx,0xB1,0x01,0xBF,0x00);
	boe_dcs_write_seq_static(ctx,0xBD,0x00);
	boe_dcs_write_seq_static(ctx,0xB2,0x00,0x6A,0x40,0x00,0x00,0x0C,0xE2,0x00,0x6F,0x00,0x00,0x31,0x00,0x00,0x00,0x15,0x2A);
	boe_dcs_write_seq_static(ctx,0xB4,0x59,0x55,0x59,0x55,0x59,0x55,0x59,0x55,0x59,0x55,0x59,0x55,0x02,0x65,0x00,0x00,0x00,0x13,0x00,0x27,0x07,0x07,0x07,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xFF,0x00,0xFF,0x10,0x00,0x02,0x2B,0x2B,0x2B,0x2B);
	boe_dcs_write_seq_static(ctx,0xBD,0x02);
	boe_dcs_write_seq_static(ctx,0xBA,0x70,0x03,0xA8,0xAF);
	boe_dcs_write_seq_static(ctx,0xBD,0x03);
	boe_dcs_write_seq_static(ctx,0xBA,0x00,0x00,0x20,0xC0,0x6F,0xEF);
	boe_dcs_write_seq_static(ctx,0xBD,0x00);
	boe_dcs_write_seq_static(ctx,0xBC,0x34,0x02,0x00);
	boe_dcs_write_seq_static(ctx,0xBF,0x00,0x41,0x80,0x0C,0x36,0x00,0x85);
	boe_dcs_write_seq_static(ctx,0xBD,0x02);
	boe_dcs_write_seq_static(ctx,0xBF,0x70);
	boe_dcs_write_seq_static(ctx,0xBD,0x00);
	boe_dcs_write_seq_static(ctx,0xC0,0x09,0x09,0x44,0x22,0x08,0xD8);
	boe_dcs_write_seq_static(ctx,0xE9,0xD2);
	boe_dcs_write_seq_static(ctx,0xC0,0xFF);
	boe_dcs_write_seq_static(ctx,0xE9,0x3F);
	boe_dcs_write_seq_static(ctx,0xCB,0x00,0x13,0x38,0x00,0x07,0x10);
	boe_dcs_write_seq_static(ctx,0xBD,0x01);
	boe_dcs_write_seq_static(ctx,0xCB,0x1F,0x55,0x02,0x28,0x1C,0x08,0x1B);
	boe_dcs_write_seq_static(ctx,0xBD,0x00);
	boe_dcs_write_seq_static(ctx,0xCC,0x0A,0x03,0x4C);
	boe_dcs_write_seq_static(ctx,0xCE,0x00,0x8A);
	boe_dcs_write_seq_static(ctx,0xD0,0x07,0xC0,0x18,0x48,0x11,0x08);
	boe_dcs_write_seq_static(ctx,0xD1,0x57,0x03,0x0C,0xFD);
	boe_dcs_write_seq_static(ctx,0xD2,0x1F,0x11,0x1F);
	boe_dcs_write_seq_static(ctx,0xE9,0xC9);
	boe_dcs_write_seq_static(ctx,0xD1,0x02);
	boe_dcs_write_seq_static(ctx,0xC7,0x80);
	boe_dcs_write_seq_static(ctx,0xE9,0x3F);
	boe_dcs_write_seq_static(ctx,0xBD,0x01);
	boe_dcs_write_seq_static(ctx,0xD2,0x00,0x00,0x00,0xB4);
	boe_dcs_write_seq_static(ctx,0xBD,0x00);
	boe_dcs_write_seq_static(ctx,0xD3,0x00,0xC0,0x04,0x04,0x04,0x04,0x04,0x04,0x14,0x02,0x37,0x07,0x07,0x53,0x3F,0x0B,0x0B,0x03,0x03,0x03,0x03,0x32,0x10,0x07,0x00,0x07,0x32,0x10,0x03,0x00,0x03,0x32,0x1A,0x12,0x0A,0x12,0x00,0x00,0xFF,0x00);
	boe_dcs_write_seq_static(ctx,0xBD,0x01);
	boe_dcs_write_seq_static(ctx,0xE9,0xC8);
	boe_dcs_write_seq_static(ctx,0xD3,0x84,0x04);
	boe_dcs_write_seq_static(ctx,0xE9,0xC4);
	boe_dcs_write_seq_static(ctx,0xD3,0x00);
	boe_dcs_write_seq_static(ctx,0xE9,0x3F);
	boe_dcs_write_seq_static(ctx,0xBD,0x00);
	boe_dcs_write_seq_static(ctx,0xD5,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x07,0x06,0x07,0x06,0x05,0x04,0x05,0x04,0x03,0x02,0x03,0x02,0x01,0x00,0x01,0x00,0x24,0x24,0x24,0x24,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x21,0x20,0x21,0x20,0x1E,0x1E,0x1E,0x1E,0x1F,0x1F,0x1F,0x1F);
	boe_dcs_write_seq_static(ctx,0xD6,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x07,0x06,0x07,0x06,0x05,0x04,0x05,0x04,0x03,0x02,0x03,0x02,0x01,0x00,0x01,0x00,0x24,0x24,0x24,0x24,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x21,0x20,0x21,0x20,0x1E,0x1E,0x1E,0x1E,0x1F,0x1F,0x1F,0x1F);
	boe_dcs_write_seq_static(ctx,0xD8,0xAA,0xAA,0xAA,0xAA,0xAA,0xAF,0xAA,0xAA,0xAA,0xAA,0xAA,0xAF,0xAA,0xAA,0xAA,0xAA,0xAA,0xAF,0xAA,0xAA,0xAA,0xAA,0xAA,0xAF,0xAA,0xAA,0xAA,0xAA,0xAA,0xAF,0xAA,0xAA,0xAA,0xAA,0xAA,0xAF);
	boe_dcs_write_seq_static(ctx,0xBD,0x01);
	boe_dcs_write_seq_static(ctx,0xD8,0xAA,0xFF,0xFF,0xFA,0xFA,0xFF,0xAA,0xFF,0xFF,0xFA,0xFA,0xFF,0xAA,0xFF,0xFF,0xFA,0xFA,0xFF,0xAA,0xFF,0xFF,0xFA,0xFA,0xFF,0xAA,0xFF,0xFF,0xFA,0xFA,0xFF,0xAA,0xFF,0xFF,0xFA,0xFA,0xFF);
	boe_dcs_write_seq_static(ctx,0xBD,0x02);
	boe_dcs_write_seq_static(ctx,0xD8,0xAA,0xFF,0xFF,0xFA,0xFA,0xFF,0xAA,0xFF,0xFF,0xFA,0xFA,0xFF);
	boe_dcs_write_seq_static(ctx,0xBD,0x03);
	boe_dcs_write_seq_static(ctx,0xD8,0xAA,0xAA,0xAA,0xFA,0xAA,0xAA,0xAA,0xAA,0xAA,0xFA,0xAA,0xAA,0xAA,0xAA,0xAA,0xFA,0xAA,0xAA,0xAA,0xAA,0xAA,0xFA,0xAA,0xAA,0xAA,0xAA,0xAA,0xFA,0xAA,0xAA,0xAA,0xAA,0xAA,0xFA,0xAA,0xAA);
	boe_dcs_write_seq_static(ctx,0xBD,0x00);
	boe_dcs_write_seq_static(ctx,0xE0,0x00,0x0C,0x24,0x28,0x2F,0x69,0x7F,0x87,0x8E,0x90,0x95,0x95,0x94,0x94,0x91,0x8D,0x8E,0x8F,0x8F,0xA6,0xB7,0x4C,0x68,0x00,0x0C,0x24,0x28,0x2F,0x69,0x7F,0x87,0x8E,0x90,0x95,0x95,0x94,0x94,0x91,0x8D,0x8E,0x8F,0x8F,0xA6,0xB7,0x4C,0x68);
	boe_dcs_write_seq_static(ctx,0xE1,0x11,0x00,0x00,0x89,0x30,0x80,0x0A,0x00,0x03,0x20,0x00,0x14,0x03,0x20,0x03,0x20,0x02,0x00,0x02,0x91,0x00,0x20,0x02,0x47,0x00,0x0B,0x00,0x0C,0x05,0x0E,0x03,0x68,0x18,0x00,0x10,0xE0,0x03,0x0C,0x20,0x00,0x06,0x0B,0x0B,0x33,0x0E,0x1C,0x2A,0x38,0x46,0x54,0x62,0x69,0x70,0x77,0x79,0x7B,0x7D,0x7E,0x01,0x02,0x01,0x00,0x09);
	boe_dcs_write_seq_static(ctx,0xBD,0x01);
	boe_dcs_write_seq_static(ctx,0xE1,0x40,0x09,0xBE,0x19,0xFC,0x19,0xFA,0x19,0xF8,0x1A,0x38,0x1A,0x78,0x1A,0xB6,0x2A,0xF6,0x2B,0x34,0x2B,0x74,0x3B,0x74,0x6B,0x74);
	boe_dcs_write_seq_static(ctx,0xBD,0x03);
	boe_dcs_write_seq_static(ctx,0xE1,0x01,0x3F);
	boe_dcs_write_seq_static(ctx,0xBD,0x00);
	boe_dcs_write_seq_static(ctx,0xE7,0x17,0x0E,0x0E,0x24,0x38,0x29,0x00,0xAC,0x29,0x02,0x02);
	boe_dcs_write_seq_static(ctx,0xE9,0xD6);
	boe_dcs_write_seq_static(ctx,0xE7,0x03,0x88);
	boe_dcs_write_seq_static(ctx,0xE9,0x3F);
	boe_dcs_write_seq_static(ctx,0xBD,0x01);
	boe_dcs_write_seq_static(ctx,0xE7,0x02,0x00,0x50,0x01,0xCA,0x0D,0x3D,0x0E,0x00,0x00);
	boe_dcs_write_seq_static(ctx,0xBD,0x02);
	boe_dcs_write_seq_static(ctx,0xE7,0xFD,0x04,0xFD,0x04,0xFD,0x04,0x03,0x03,0x03,0x17,0x00,0x17,0x81,0x02,0x40,0x00,0x20,0x65,0x06,0x05,0x04,0x03,0x02,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00);
	boe_dcs_write_seq_static(ctx,0xBD,0x01);
	boe_dcs_write_seq_static(ctx,0xE2,0x59,0x55,0x59,0x55,0x59,0x55,0x59,0x55,0x59,0x55,0x02,0x65,0x00,0x00,0x09,0x09,0x00,0x13,0x27,0x07,0x07,0x2B,0x2B,0x2B,0x2B,0x00,0x00,0x00,0x17,0x14,0x14,0x24,0x38,0x29,0x00,0xAC,0x29,0x02,0x02,0x00,0x00,0x50,0x01,0xCA,0x13,0x40,0x54,0x00,0x20,0x65,0x00,0x6F,0x00,0x07,0x10,0x00);
	boe_dcs_write_seq_static(ctx,0xBD,0x00);
	boe_dcs_write_seq_static(ctx,0xD9,0x0C);
	boe_dcs_write_seq_static(ctx,0xE9,0xC6);
	boe_dcs_write_seq_static(ctx,0xC8,0x83);
	boe_dcs_write_seq_static(ctx,0xE9,0x3F);
	boe_dcs_write_seq_static(ctx,0xB9,0x00,0x00,0x00,0x00);
	boe_dcs_write_seq_static(ctx,0x11);
	msleep(120);
	boe_dcs_write_seq_static(ctx,0x29);
	msleep(50);
	pr_info("%s-\n", __func__);
}

static int boe_disable(struct drm_panel *panel)
{
	struct boe *ctx = panel_to_boe(panel);

	if (!ctx->enabled)
		return 0;
	pr_info("hx83122a %s +\n", __func__);
	if (ctx->backlight) {
		ctx->backlight->props.power = FB_BLANK_POWERDOWN;
		backlight_update_status(ctx->backlight);
		pr_info("%s backlight disable \n", __func__);
	}

	ctx->enabled = false;
	pr_info("hx83122a %s -\n", __func__);
	return 0;
}

static int boe_unprepare(struct drm_panel *panel)
{

	struct boe *ctx = panel_to_boe(panel);
	int error=0;
	if (!ctx->prepared)
		return 0;

	boe_dcs_write_seq_static(ctx, MIPI_DCS_ENTER_SLEEP_MODE);
	msleep(120);
	boe_dcs_write_seq_static(ctx, MIPI_DCS_SET_DISPLAY_OFF);	
	msleep(120);
	pr_info("hx83122a %s, HX_SMWP_EN: %d\n", __func__, HX_SMWP_EN);
	if (HX_SMWP_EN == 1) {
		ctx->error = 0;
		ctx->prepared = false;
		pr_info("hx83122a %s tp is gesture mode\n", __func__);
		return 0;
	}
	pr_info("hx83122a %s +\n", __func__);
	ctx->reset_gpio = devm_gpiod_get(ctx->dev, "reset", GPIOD_OUT_HIGH);
	gpiod_set_value(ctx->reset_gpio, 0);
	devm_gpiod_put(ctx->dev, ctx->reset_gpio);
	if (gpio_is_valid(tp_reset)) {
			error = gpio_direction_output(tp_reset, 0);
			if (error) {
				pr_info("hx83122a %s HXTP \n", __func__);
			}
		}
	lp6280s_write_byte(0x00, 0x00);
	lp6280s_write_byte(0x01, 0x00);
   
        usleep_range(2000, 2001);
   
	ctx->bias_neg =
		devm_gpiod_get_index(ctx->dev, "bias", 1, GPIOD_OUT_HIGH);
	gpiod_set_value(ctx->bias_neg, 0);
	devm_gpiod_put(ctx->dev, ctx->bias_neg);

	usleep_range(2000, 2001);

	ctx->bias_pos =
	devm_gpiod_get_index(ctx->dev, "bias", 0, GPIOD_OUT_HIGH);
	gpiod_set_value(ctx->bias_pos, 0);
	devm_gpiod_put(ctx->dev, ctx->bias_pos);
	
	usleep_range(2000, 2001);
//	ctx->backlight_en = devm_gpiod_get(ctx->dev, "pm-enable", GPIOD_OUT_HIGH);
//	gpiod_set_value(ctx->backlight_en, 0);
//	devm_gpiod_put(ctx->dev, ctx->backlight_en);
	ctx->iovcc_gpio = devm_gpiod_get(ctx->dev, "iovcc", GPIOD_OUT_HIGH);
	gpiod_set_value(ctx->iovcc_gpio, 0);
	devm_gpiod_put(ctx->dev, ctx->iovcc_gpio);
        pr_info("hx83122a %s -\n", __func__);
	msleep(40);
	ctx->error = 0;
	ctx->prepared = false;
	return 0;
}

static int boe_prepare(struct drm_panel *panel)
{
	struct boe *ctx = panel_to_boe(panel);
	int ret;

	pr_info(" hx83122a %s+\n", __func__);
	if (ctx->prepared)
		return 0;

	ctx->reset_gpio = devm_gpiod_get(ctx->dev, "reset", GPIOD_OUT_HIGH);
	gpiod_set_value(ctx->reset_gpio, 0);
	devm_gpiod_put(ctx->dev, ctx->reset_gpio);
	usleep_range(1000, 1001);
	ctx->iovcc_gpio = devm_gpiod_get(ctx->dev, "iovcc", GPIOD_OUT_HIGH);
	gpiod_set_value(ctx->iovcc_gpio, 1);
	devm_gpiod_put(ctx->dev, ctx->iovcc_gpio);
	usleep_range(5000, 5001);
	ctx->bias_pos =
		devm_gpiod_get_index(ctx->dev, "bias", 0, GPIOD_OUT_HIGH);
	gpiod_set_value(ctx->bias_pos, 1);
	devm_gpiod_put(ctx->dev, ctx->bias_pos);

	usleep_range(5000, 5001);
	ctx->bias_neg =
		devm_gpiod_get_index(ctx->dev, "bias", 1, GPIOD_OUT_HIGH);
	gpiod_set_value(ctx->bias_neg, 1);
	devm_gpiod_put(ctx->dev, ctx->bias_neg);
	usleep_range(5000, 5001);
	lp6280s_write_byte(0x00, 0x14);
	lp6280s_write_byte(0x01, 0x14);
	lp6280s_write_byte(0x03, 0x73);
	usleep_range(5000, 5001);
 //     ctx->backlight_en = devm_gpiod_get(ctx->dev, "pm-enable", GPIOD_OUT_HIGH);
 //  	gpiod_set_value(ctx->backlight_en, 1);
 //	devm_gpiod_put(ctx->dev, ctx->backlight_en);

#if 0	
 #ifdef   CONFIG_MTK_MP3314_BACKLIGHT_IC
	mp3314_ic_power_on();
	mp3314_ic_i2c_panel_backlight_enable(1);
#endif
#endif 
	pr_info(" hx83122a %s+\n", __func__);
	boe_panel_init(ctx);
	ret = ctx->error;
	if (ret < 0) { 
		boe_unprepare(panel);
		pr_info("%s11111-\n", __func__);
	}
	ctx->prepared = true;
#ifdef PANEL_SUPPORT_READBACK
	boe_panel_get_data(ctx);
#endif

#ifdef VENDOR_EDIT
	// shifan@bsp.tp 20191226 add for loading tp fw when screen lighting on
	lcd_queue_load_tp_fw();
#endif

	pr_info("hx83122a %s-\n", __func__);
	return ret;
}

static int boe_enable(struct drm_panel *panel)
{
	struct boe *ctx = panel_to_boe(panel);
	pr_info("hx83122a %s+\n", __func__);
	if (ctx->enabled)
		return 0;
    //mp3314g_backlight_init();	
	if (ctx->backlight) {
		ctx->backlight->props.power = FB_BLANK_UNBLANK;
		backlight_update_status(ctx->backlight);
		pr_info("%s backlight enable\n", __func__);
	}
    
	ctx->enabled = true;
	pr_info("hx83122a %s -\n", __func__);
	return 0;
}

static const struct drm_display_mode default_mode = {
	.clock = 477125,
	.hdisplay = 1600,
	.hsync_start = 1600 + 232,//HFP
	.hsync_end = 1600 + 232+ 20,//HSA
	.htotal = 1600 + 232+ 20 + 40,//HBP
	.vdisplay = 2560,
	.vsync_start = 2560 + 228,//VFP
	.vsync_end = 2560 + 228+ 4,//VSA
	.vtotal = 2560 + 228+ 4 + 10,//VBP
};


static const struct drm_display_mode performance_mode_60hz = {
	.clock = 477125,
	.hdisplay = 1600,
	.hsync_start = 1600 + 232,//HFP
	.hsync_end = 1600 + 232+ 20,//HSA
	.htotal = 1600 + 232+ 20 + 40,//HBP
	.vdisplay = 2560,
	.vsync_start = 2560 + 1629,//VFP
	.vsync_end = 2560 + 1629+ 4,//VSA
	.vtotal = 2560 + 1629 + 4 + 10,//VBP
};


#if defined(CONFIG_MTK_PANEL_EXT)
static struct mtk_panel_params ext_params = {
	.pll_clk = 364,
	.vdo_per_frame_lp_enable = 0,
	.change_fps_by_vfp_send_cmd = 1,
	//.vfp_low_power = 228,
	.cust_esd_check = 0,
	.esd_check_enable = 1,
	.esd_te_gpio_check_enable = 1,
	.lcm_esd_check_table[0] = {
		.cmd = 0x0A, .count = 1, .para_list[0] = 0x9D,
	},
	.phy_timcon={
		.lpx=0x0A,
		.da_hs_exit=0x0F,
		.ta_get=0x32,
		.ta_sure=0x0F,
		.ta_go=0x28,
	},
	//.rotate = 1,
	.is_cphy = 1,
	.output_mode = MTK_PANEL_DSC_SINGLE_PORT,
	.dsc_params = {
		.enable = 1,
		.ver = 17,
		.slice_mode = 1,
		.rgb_swap = 0,
		.dsc_cfg = 34,
		.rct_on = 1,
		.bit_per_channel = 8,
		.dsc_line_buf_depth = 9,
		.bp_enable = 1,
		.bit_per_pixel = 128,
		.pic_height = 2560,
		.pic_width = 1600,
		.slice_height = 20,
		.slice_width = 800,
		.chunk_size = 800,
		.xmit_delay = 512,
		.dec_delay = 657,
		.scale_value = 32,
		.increment_interval = 583,
		.decrement_interval = 11,
		.line_bpg_offset = 12,
		.nfl_bpg_offset = 1294,
		.slice_bpg_offset = 872,
		.initial_offset = 6144,
		.final_offset = 4320,
		.flatness_minqp = 3,
		.flatness_maxqp = 12,
		.rc_model_size = 8192,
		.rc_edge_factor = 6,
		.rc_quant_incr_limit0 = 11,
		.rc_quant_incr_limit1 = 11,
		.rc_tgt_offset_hi = 3,
		.rc_tgt_offset_lo = 3,

		.rc_buf_thresh[0] = 14,
		.rc_buf_thresh[1] = 28,
		.rc_buf_thresh[2] = 42,
		.rc_buf_thresh[3] = 56,
		.rc_buf_thresh[4] = 70,
		.rc_buf_thresh[5] = 84,
		.rc_buf_thresh[6] = 98,
		.rc_buf_thresh[7] = 105,
		.rc_buf_thresh[8] = 112,
		.rc_buf_thresh[9] = 119,
		.rc_buf_thresh[10] = 121,
		.rc_buf_thresh[11] = 123,
		.rc_buf_thresh[12] = 125,
		.rc_buf_thresh[13] = 126,
		.rc_range_parameters[0].range_min_qp = 0,
		.rc_range_parameters[0].range_max_qp = 4,
		.rc_range_parameters[0].range_bpg_offset = 2,
		.rc_range_parameters[1].range_min_qp = 0,
		.rc_range_parameters[1].range_max_qp = 4,
		.rc_range_parameters[1].range_bpg_offset = 0,
		.rc_range_parameters[2].range_min_qp = 1,
		.rc_range_parameters[2].range_max_qp = 5,
		.rc_range_parameters[2].range_bpg_offset = 0,
		.rc_range_parameters[3].range_min_qp = 1,
		.rc_range_parameters[3].range_max_qp = 6,
		.rc_range_parameters[3].range_bpg_offset = -2,
		.rc_range_parameters[4].range_min_qp = 3,
		.rc_range_parameters[4].range_max_qp = 7,
		.rc_range_parameters[4].range_bpg_offset = -4,
		.rc_range_parameters[5].range_min_qp = 3,
		.rc_range_parameters[5].range_max_qp = 7,
		.rc_range_parameters[5].range_bpg_offset = -6,
		.rc_range_parameters[6].range_min_qp = 3,
		.rc_range_parameters[6].range_max_qp = 7,
		.rc_range_parameters[6].range_bpg_offset = -8,
		.rc_range_parameters[7].range_min_qp = 3,
		.rc_range_parameters[7].range_max_qp = 8,
		.rc_range_parameters[7].range_bpg_offset = -8,
		.rc_range_parameters[8].range_min_qp = 3,
		.rc_range_parameters[8].range_max_qp = 9,
		.rc_range_parameters[8].range_bpg_offset = -8,
		.rc_range_parameters[9].range_min_qp = 3,
		.rc_range_parameters[9].range_max_qp = 10,
		.rc_range_parameters[9].range_bpg_offset = -10,
		.rc_range_parameters[10].range_min_qp = 5,
		.rc_range_parameters[10].range_max_qp = 11,
		.rc_range_parameters[10].range_bpg_offset = -10,
		.rc_range_parameters[11].range_min_qp = 5,
		.rc_range_parameters[11].range_max_qp = 12,
		.rc_range_parameters[11].range_bpg_offset = -12,
		.rc_range_parameters[12].range_min_qp = 5,
		.rc_range_parameters[12].range_max_qp = 13,
		.rc_range_parameters[12].range_bpg_offset = -12,
		.rc_range_parameters[13].range_min_qp = 7,
		.rc_range_parameters[13].range_max_qp = 13,
		.rc_range_parameters[13].range_bpg_offset = -12,
		.rc_range_parameters[14].range_min_qp = 13,
		.rc_range_parameters[14].range_max_qp = 13,
		.rc_range_parameters[14].range_bpg_offset = -12,
	},
	.data_rate = 728,
	.data_rate_khz = 727300,//x1000/540400/62/727300
	.lfr_enable = 0,
	.lfr_minimum_fps = 90,
	.dyn_fps = {
		.switch_en = 1,
		.vact_timing_fps = 90,
		.dfps_cmd_table[0] = {0, 4, {0xB9, 0x83, 0x12, 0x2A} },
		.dfps_cmd_table[1] = {0, 2, {0xE2, 0x00} },
		.dfps_cmd_table[2] = {0, 2, {0xBD, 0x01} },
		.dfps_cmd_table[3] = {0, 5, {0xD2, 0x00, 0x00, 0x00, 0xB4} },
		.dfps_cmd_table[4] = {0, 2, {0xBD, 0x00} },
		.dfps_cmd_table[5] = {0, 4, {0xB9, 0x00, 0x00, 0x00} },
	},
	/* following MIPI hopping parameter might cause screen mess */
	.dyn = {
		.switch_en = 1,
		//.vfp = 228,
	},
};

static struct mtk_panel_params ext_params_60hz = {
	.pll_clk = 364,
	.vdo_per_frame_lp_enable = 0,
	.change_fps_by_vfp_send_cmd = 1,
	//.vfp_low_power = 1629,
	.cust_esd_check = 0,
	.esd_check_enable = 1,
	.esd_te_gpio_check_enable = 1,
	.lcm_esd_check_table[0] = {
		.cmd = 0x0A, .count = 1, .para_list[0] = 0x9D,
	},
	.phy_timcon={
		.lpx=0x0A,
		.da_hs_exit=0x0F,
		.ta_get=0x32,
		.ta_sure=0x0F,
		.ta_go=0x28,
	},
	//.rotate = 1,
	.is_cphy = 1,
	.output_mode = MTK_PANEL_DSC_SINGLE_PORT,
	.dsc_params = {
		.enable = 1,
		.ver = 17,
		.slice_mode = 1,
		.rgb_swap = 0,
		.dsc_cfg = 34,
		.rct_on = 1,
		.bit_per_channel = 8,
		.dsc_line_buf_depth = 9,
		.bp_enable = 1,
		.bit_per_pixel = 128,
		.pic_height = 2560,
		.pic_width = 1600,
		.slice_height = 20,
		.slice_width = 800,
		.chunk_size = 800,
		.xmit_delay = 512,
		.dec_delay = 657,
		.scale_value = 32,
		.increment_interval = 583,
		.decrement_interval = 11,
		.line_bpg_offset = 12,
		.nfl_bpg_offset = 1294,
		.slice_bpg_offset = 872,
		.initial_offset = 6144,
		.final_offset = 4320,
		.flatness_minqp = 3,
		.flatness_maxqp = 12,
		.rc_model_size = 8192,
		.rc_edge_factor = 6,
		.rc_quant_incr_limit0 = 11,
		.rc_quant_incr_limit1 = 11,
		.rc_tgt_offset_hi = 3,
		.rc_tgt_offset_lo = 3,

		.rc_buf_thresh[0] = 14,
		.rc_buf_thresh[1] = 28,
		.rc_buf_thresh[2] = 42,
		.rc_buf_thresh[3] = 56,
		.rc_buf_thresh[4] = 70,
		.rc_buf_thresh[5] = 84,
		.rc_buf_thresh[6] = 98,
		.rc_buf_thresh[7] = 105,
		.rc_buf_thresh[8] = 112,
		.rc_buf_thresh[9] = 119,
		.rc_buf_thresh[10] = 121,
		.rc_buf_thresh[11] = 123,
		.rc_buf_thresh[12] = 125,
		.rc_buf_thresh[13] = 126,
		.rc_range_parameters[0].range_min_qp = 0,
		.rc_range_parameters[0].range_max_qp = 4,
		.rc_range_parameters[0].range_bpg_offset = 2,
		.rc_range_parameters[1].range_min_qp = 0,
		.rc_range_parameters[1].range_max_qp = 4,
		.rc_range_parameters[1].range_bpg_offset = 0,
		.rc_range_parameters[2].range_min_qp = 1,
		.rc_range_parameters[2].range_max_qp = 5,
		.rc_range_parameters[2].range_bpg_offset = 0,
		.rc_range_parameters[3].range_min_qp = 1,
		.rc_range_parameters[3].range_max_qp = 6,
		.rc_range_parameters[3].range_bpg_offset = -2,
		.rc_range_parameters[4].range_min_qp = 3,
		.rc_range_parameters[4].range_max_qp = 7,
		.rc_range_parameters[4].range_bpg_offset = -4,
		.rc_range_parameters[5].range_min_qp = 3,
		.rc_range_parameters[5].range_max_qp = 7,
		.rc_range_parameters[5].range_bpg_offset = -6,
		.rc_range_parameters[6].range_min_qp = 3,
		.rc_range_parameters[6].range_max_qp = 7,
		.rc_range_parameters[6].range_bpg_offset = -8,
		.rc_range_parameters[7].range_min_qp = 3,
		.rc_range_parameters[7].range_max_qp = 8,
		.rc_range_parameters[7].range_bpg_offset = -8,
		.rc_range_parameters[8].range_min_qp = 3,
		.rc_range_parameters[8].range_max_qp = 9,
		.rc_range_parameters[8].range_bpg_offset = -8,
		.rc_range_parameters[9].range_min_qp = 3,
		.rc_range_parameters[9].range_max_qp = 10,
		.rc_range_parameters[9].range_bpg_offset = -10,
		.rc_range_parameters[10].range_min_qp = 5,
		.rc_range_parameters[10].range_max_qp = 11,
		.rc_range_parameters[10].range_bpg_offset = -10,
		.rc_range_parameters[11].range_min_qp = 5,
		.rc_range_parameters[11].range_max_qp = 12,
		.rc_range_parameters[11].range_bpg_offset = -12,
		.rc_range_parameters[12].range_min_qp = 5,
		.rc_range_parameters[12].range_max_qp = 13,
		.rc_range_parameters[12].range_bpg_offset = -12,
		.rc_range_parameters[13].range_min_qp = 7,
		.rc_range_parameters[13].range_max_qp = 13,
		.rc_range_parameters[13].range_bpg_offset = -12,
		.rc_range_parameters[14].range_min_qp = 13,
		.rc_range_parameters[14].range_max_qp = 13,
		.rc_range_parameters[14].range_bpg_offset = -12,
	},
	.data_rate = 728,
	.data_rate_khz = 727300,
	.lfr_enable = 0,
	.lfr_minimum_fps = 60,
	.dyn_fps = {
		.switch_en = 1,
		.vact_timing_fps = 60,
		.dfps_cmd_table[0] = {0, 4, {0xB9, 0x83, 0x12, 0x2A} },
		.dfps_cmd_table[1] = {0, 2, {0xE2, 0x10} },
		.dfps_cmd_table[2] = {0, 2, {0xBD, 0x01} },
		.dfps_cmd_table[3] = {0, 5, {0xD2, 0x00, 0x00, 0x00, 0x78} },
		.dfps_cmd_table[4] = {0, 2, {0xBD, 0x00} },
		.dfps_cmd_table[5] = {0, 4, {0xB9, 0x00, 0x00, 0x00} },

	},
	/* following MIPI hopping parameter might cause screen mess */
	.dyn = {
		.switch_en = 1,
		//.vfp = 1629,
	},
};

static void mode_switch_to_90(struct drm_panel *panel)
{
	struct boe *ctx = panel_to_boe(panel);

	pr_err("%s\n", __func__);

	boe_dcs_write_seq_static(ctx, 0xB9, 0x83, 0x12, 0x2A);
	boe_dcs_write_seq_static(ctx, 0xE2, 0x00);
	boe_dcs_write_seq_static(ctx, 0xBD, 0x01);
	boe_dcs_write_seq_static(ctx, 0xD2, 0x00, 0x00, 0x00, 0xB4);//90hz
	boe_dcs_write_seq_static(ctx, 0xBD, 0x00);
	boe_dcs_write_seq_static(ctx, 0xB9, 0x00, 0x00, 0x00);
}

static void mode_switch_to_60(struct drm_panel *panel)
{
	struct boe *ctx = panel_to_boe(panel);

	pr_err("%s\n", __func__);

	boe_dcs_write_seq_static(ctx, 0xB9, 0x83, 0x12, 0x2A);
	boe_dcs_write_seq_static(ctx, 0xE2, 0x00);
	boe_dcs_write_seq_static(ctx, 0xBD, 0x01);
	boe_dcs_write_seq_static(ctx, 0xD2, 0x00, 0x00, 0x00, 0x78);//60hz
	boe_dcs_write_seq_static(ctx, 0xBD, 0x00);
	boe_dcs_write_seq_static(ctx, 0xB9, 0x00, 0x00, 0x00);
}

static int panel_ata_check(struct drm_panel *panel)
{
	/* Customer test by own ATA tool */
	return 1;
}

static int boe_setbacklight_cmdq(void *dsi, dcs_write_gce cb, void *handle,
				 unsigned int level)
{
	int  bl_level = 0;
	int bl_msb=0;
	int  bl_lsb=0;	
	if(level>2047)
		level=2047;
	bl_level=level*0xFFFF/2047;   
	bl_msb=(bl_level>>8)&0xFF;
	bl_lsb=bl_level&0xFF;
 	if (!cb)
	{
		printk("cb is  0\n");
 		return -1;
	}
       	if (level < 0) {
		pr_debug("%d %s --ahx invalid level=%d\n", __LINE__, __func__, level);
		return 0;
	}
 	return 0;
}

struct drm_display_mode *get_mode_by_id_hfp(struct drm_connector *connector,
	unsigned int mode)
{
	struct drm_display_mode *m;
	unsigned int i = 0;

	list_for_each_entry(m, &connector->modes, head) {
		if (i == mode)
			return m;
		i++;
	}
	return NULL;
}
static int mtk_panel_ext_param_set(struct drm_panel *panel,
			struct drm_connector *connector, unsigned int mode)
{
	struct mtk_panel_ext *ext = find_panel_ext(panel);
	int ret = 0;
	struct drm_display_mode *m = get_mode_by_id_hfp(connector, mode);

	pr_err("drm_mode_vrefresh(m) =%d", drm_mode_vrefresh(m));
		if (!m) {
		pr_err("%s:%d invalid display_mode\n", __func__, __LINE__);
		return ret;
	}
	if (drm_mode_vrefresh(m) == 90)
		ext->params = &ext_params;
	else if (drm_mode_vrefresh(m) == 60)
		ext->params = &ext_params_60hz;
	else
		ret = 1;
	if (!ret)
		current_fps = drm_mode_vrefresh(m);
	return ret;
}

static int panel_ext_reset(struct drm_panel *panel, int on)
{
	struct boe *ctx = panel_to_boe(panel);

	ctx->reset_gpio =
		devm_gpiod_get(ctx->dev, "reset", GPIOD_OUT_HIGH);
	gpiod_set_value(ctx->reset_gpio, on);
	devm_gpiod_put(ctx->dev, ctx->reset_gpio);

	return 0;
}

static int mode_switch(struct drm_panel *panel,
		struct drm_connector *connector, unsigned int cur_mode,
		unsigned int dst_mode, enum MTK_PANEL_MODE_SWITCH_STAGE stage)
{
	int ret = 0;
	struct drm_display_mode *m = get_mode_by_id_hfp(connector, dst_mode);
	pr_err("boe %s cur_mode = %d dst_mode %d\n", __func__, cur_mode, dst_mode);
	if (!m)
		return ret;
	pr_err("%s cur_mode = %d dst_mode %d\n", __func__, cur_mode, dst_mode);
	if (m == NULL) {
		pr_err("%s:%d invalid display_mode\n", __func__, __LINE__);
		return -1;
	}

	if (drm_mode_vrefresh(m) == 90) { /* 90 switch to 60 */
		mode_switch_to_90(panel);
	} else if (drm_mode_vrefresh(m) == 60) { /* 60 switch to 90 */
		mode_switch_to_60(panel);
	} else
		ret = 1;
	return ret;
}

static struct mtk_panel_funcs ext_funcs = {
	.reset = panel_ext_reset,
	.set_backlight_cmdq = boe_setbacklight_cmdq,
	.ext_param_set = mtk_panel_ext_param_set,
	.mode_switch = mode_switch,
	.ata_check = panel_ata_check,
};
#endif

struct panel_desc {
	const struct drm_display_mode *modes;
	unsigned int num_modes;

	unsigned int bpc;

	struct {
		unsigned int width;
		unsigned int height;
	} size;

	/**
	 * @prepare: the time (in milliseconds) that it takes for the panel to
	 *	   become ready and start receiving video data
	 * @enable: the time (in milliseconds) that it takes for the panel to
	 *	  display the first valid frame after starting to receive
	 *	  video data
	 * @disable: the time (in milliseconds) that it takes for the panel to
	 *	   turn the display off (no content is visible)
	 * @unprepare: the time (in milliseconds) that it takes for the panel
	 *		 to power itself down completely
	 */
	struct {
		unsigned int prepare;
		unsigned int enable;
		unsigned int disable;
		unsigned int unprepare;
	} delay;
};

static int boe_get_modes(struct drm_panel *panel,
					struct drm_connector *connector)
{
	struct drm_display_mode *mode;
	struct drm_display_mode *mode2;

	mode = drm_mode_duplicate(connector->dev, &default_mode);
	if (!mode) {
		dev_info(connector->dev->dev, "failed to add mode %ux%ux@%u\n",
			 default_mode.hdisplay, default_mode.vdisplay,
			 drm_mode_vrefresh(&default_mode));
		return -ENOMEM;
	}
	drm_mode_set_name(mode);
	mode->type = DRM_MODE_TYPE_DRIVER | DRM_MODE_TYPE_PREFERRED;
	drm_mode_probed_add(connector, mode);

	mode2 = drm_mode_duplicate(connector->dev, &performance_mode_60hz);
	if (!mode2) {
		dev_info(connector->dev->dev, "failed to add mode %ux%ux@%u\n",
			 performance_mode_60hz.hdisplay, performance_mode_60hz.vdisplay,
			 drm_mode_vrefresh(&performance_mode_60hz));
		return -ENOMEM;
	}

	drm_mode_set_name(mode2);
	mode2->type = DRM_MODE_TYPE_DRIVER;
	drm_mode_probed_add(connector, mode2);
	connector->display_info.width_mm = 163;
	connector->display_info.height_mm = 261;

	return 1;
}

static const struct drm_panel_funcs boe_drm_funcs = {
	.disable = boe_disable,
	.unprepare = boe_unprepare,
	.prepare = boe_prepare,
	.enable = boe_enable,
	.get_modes = boe_get_modes,
};

static int boe_probe(struct mipi_dsi_device *dsi)
{
	struct device *dev = &dsi->dev;
//	struct device_node *dsi_node, *remote_node = NULL, *endpoint = NULL;
	struct boe *ctx;
	struct device_node *backlight;
	//unsigned int value;
	int ret;

	pr_info("%s+ boe,hx83122a,wqxga,vdo\n", __func__);

#if 0
	dsi_node = of_get_parent(dev->of_node);
	if (dsi_node) {
		endpoint = of_graph_get_next_endpoint(dsi_node, NULL);
		if (endpoint) {
			remote_node = of_graph_get_remote_port_parent(endpoint);
			if (!remote_node) {
				pr_info("No panel connected,skip probe lcm\n");
				return -ENODEV;
			}
			pr_info("device node name:%s\n", remote_node->name);
		}
	}
	if (remote_node != dev->of_node) {
		pr_info("%s+ skip probe due to not current lcm\n", __func__);
		return -ENODEV;
	}
#endif

	ctx = devm_kzalloc(dev, sizeof(struct boe), GFP_KERNEL);
	if (!ctx)
		return -ENOMEM;

	mipi_dsi_set_drvdata(dsi, ctx);

	ctx->dev = dev;
	dsi->lanes = 3;
	dsi->format = MIPI_DSI_FMT_RGB888;
	dsi->mode_flags =  MIPI_DSI_MODE_VIDEO | MIPI_DSI_MODE_VIDEO_SYNC_PULSE |
			  MIPI_DSI_MODE_LPM | MIPI_DSI_MODE_EOT_PACKET |
			  MIPI_DSI_CLOCK_NON_CONTINUOUS;
			 
#if 0
	ret = of_property_read_u32(dev->of_node, "gate-ic", &value);
	if (ret < 0)
		value = 0;
	ret = of_property_read_u32(dev->of_node, "rc-enable", &value);
	if (ret < 0)
		value = 0;
	else {
		ext_params.round_corner_en = value;
	}
#endif
	backlight = of_parse_phandle(dev->of_node, "backlight", 0);
	if (backlight) {
		ctx->backlight = of_find_backlight_by_node(backlight);
		of_node_put(backlight);

		if (!ctx->backlight)
			return -EPROBE_DEFER;
	}

	ctx->iovcc_gpio = devm_gpiod_get(dev, "iovcc", GPIOD_OUT_HIGH);
	if (IS_ERR(ctx->iovcc_gpio)) {
		dev_info(dev, "cannot get iovcc_gpio %ld\n",
			 PTR_ERR(ctx->iovcc_gpio));
		return PTR_ERR(ctx->iovcc_gpio);
	}
	devm_gpiod_put(dev, ctx->iovcc_gpio);
	//add for te 
	ctx->te_gpio = devm_gpiod_get(dev, "te", GPIOD_IN);
	if (IS_ERR(ctx->te_gpio)) {
		dev_info(dev, "esd cannot get te_gpio %ld\n",
			 PTR_ERR(ctx->te_gpio));
		return PTR_ERR(ctx->te_gpio);
	}
	ext_params_60hz.esd_te_gpio = desc_to_gpio(ctx->te_gpio) ;
	ext_params.esd_te_gpio = desc_to_gpio(ctx->te_gpio) ;
	devm_gpiod_put(dev, ctx->te_gpio);
	
	ctx->te1_gpio = devm_gpiod_get(dev, "te1", GPIOD_IN);
	if (IS_ERR(ctx->te1_gpio)) {
		dev_info(dev, "esd cannot get te1_gpio %ld\n",
			 PTR_ERR(ctx->te1_gpio));
		return PTR_ERR(ctx->te1_gpio);
	}
	ext_params_60hz.esd_te1_gpio = desc_to_gpio(ctx->te1_gpio) ;
	ext_params.esd_te1_gpio = desc_to_gpio(ctx->te1_gpio) ;
	devm_gpiod_put(dev, ctx->te1_gpio);

	ctx->reset_gpio = devm_gpiod_get(dev, "reset", GPIOD_OUT_HIGH);
	if (IS_ERR(ctx->reset_gpio)) {
		dev_info(dev, "cannot get reset-gpios %ld\n",
			 PTR_ERR(ctx->reset_gpio));
		return PTR_ERR(ctx->reset_gpio);
	}
	devm_gpiod_put(dev, ctx->reset_gpio);

		ctx->bias_pos = devm_gpiod_get_index(dev, "bias", 0, GPIOD_OUT_HIGH);
		if (IS_ERR(ctx->bias_pos)) {
			dev_info(dev, "cannot get bias-gpios 0 %ld\n",
				 PTR_ERR(ctx->bias_pos));
			return PTR_ERR(ctx->bias_pos);
		}
		devm_gpiod_put(dev, ctx->bias_pos);

		ctx->bias_neg = devm_gpiod_get_index(dev, "bias", 1, GPIOD_OUT_HIGH);
		if (IS_ERR(ctx->bias_neg)) {
			dev_info(dev, "cannot get bias-gpios 1 %ld\n",
				 PTR_ERR(ctx->bias_neg));
			return PTR_ERR(ctx->bias_neg);
		}
		devm_gpiod_put(dev, ctx->bias_neg);
	
	
	//ctx->backlight_en = devm_gpiod_get(dev, "pm-enable", GPIOD_OUT_HIGH);
	//if (IS_ERR(ctx->backlight_en)) {
	//	dev_info(dev, "cannot get gpio_backlight -gpios %ld\n",
	//		 PTR_ERR(ctx->backlight_en));
	//	return PTR_ERR(ctx->backlight_en);
	//}
	//devm_gpiod_put(dev, ctx->backlight_en);
	//mp3314g_backlight_init();

	ctx->prepared = true;
	ctx->enabled = true;
	
	drm_panel_init(&ctx->panel, dev, &boe_drm_funcs, DRM_MODE_CONNECTOR_DSI);

	drm_panel_add(&ctx->panel);

	ret = mipi_dsi_attach(dsi);
	if (ret < 0)
		drm_panel_remove(&ctx->panel);

#if defined(CONFIG_MTK_PANEL_EXT)
	mtk_panel_tch_handle_reg(&ctx->panel);
	ret = mtk_panel_ext_create(dev, &ext_params, &ext_funcs, &ctx->panel);
	pr_info(" %d  %s\n", __LINE__, __func__);
	if (ret < 0)
		return ret;

#endif

	pr_info("%s- boe,hx83122a,wqxga,vdo\n", __func__);

	return ret;
}

static int boe_remove(struct mipi_dsi_device *dsi)
{
	struct boe *ctx = mipi_dsi_get_drvdata(dsi);
#if defined(CONFIG_MTK_PANEL_EXT)
	struct mtk_panel_ctx *ext_ctx = find_panel_ctx(&ctx->panel);
#endif

	mipi_dsi_detach(dsi);
	drm_panel_remove(&ctx->panel);
#if defined(CONFIG_MTK_PANEL_EXT)
	mtk_panel_detach(ext_ctx);
	mtk_panel_remove(ext_ctx);
#endif

	return 0;
}

static const struct of_device_id boe_of_match[] = {
	{
	    .compatible = "boe,hx83122a,wqxga,vdo,cphy",
	},
	{}
};

MODULE_DEVICE_TABLE(of, boe_of_match);

static struct mipi_dsi_driver boe_driver = {
	.probe = boe_probe,
	.remove = boe_remove,
	.driver = {
		.name = "panel-boe-hx83122a-wqxga-vdo-cphy",
		.owner = THIS_MODULE,
		.of_match_table = boe_of_match,
	},
};

module_mipi_dsi_driver(boe_driver);

MODULE_AUTHOR("wulongchao <wulongchao@huaqin.com>");
MODULE_DESCRIPTION("BOE hx83122aJ wqxga VDO  LCD Panel Driver");
MODULE_LICENSE("GPL v2");

