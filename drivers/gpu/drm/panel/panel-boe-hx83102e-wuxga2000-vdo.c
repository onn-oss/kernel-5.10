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
#include <linux/of_graph.h>
#include <linux/platform_device.h>

#define CONFIG_MTK_PANEL_EXT
#if defined(CONFIG_MTK_PANEL_EXT)
#include "../mediatek/mediatek_v2/mtk_panel_ext.h"
#include "../mediatek/mediatek_v2/mtk_drm_graphics_base.h"
#endif

#include "../mediatek/mediatek_v2/mtk_corner_pattern/mtk_data_hw_roundedpattern.h"

#ifdef  CONFIG_MTK_MP3314_BACKLIGHT_IC
#include "../../../misc/mediatek/mp3314/mp3314_i2c.h"
#endif

#include"lcd_bias_lp6280s.h"
#include "mp3314g.h"
//static char bl_tb0[] = { 0x51, 0x0f, 0xff };
static int current_fps = 60;
#ifdef HIMAX_WAKE_UP
extern uint8_t wake_flag_drm;
#endif

struct boe {
	struct device *dev;
	struct drm_panel panel;
	struct backlight_device *backlight;
	struct gpio_desc *reset_gpio;
	struct gpio_desc *bias_pos;
	struct gpio_desc *bias_neg;
	struct gpio_desc *backlight_en;
	bool prepared;
	bool enabled;

	unsigned int gate_ic;

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
	ctx->reset_gpio = devm_gpiod_get(ctx->dev, "reset", GPIOD_OUT_HIGH);
	gpiod_set_value(ctx->reset_gpio, 1);
	usleep_range(5 * 1000, 10 * 1000);
	gpiod_set_value(ctx->reset_gpio, 0);
	usleep_range(10 * 1000, 15 * 1000);
	gpiod_set_value(ctx->reset_gpio, 1);
	usleep_range(55 * 1000, 65 * 1000);
	devm_gpiod_put(ctx->dev, ctx->reset_gpio);
	boe_dcs_write_seq_static(ctx,0xB9,0x83,0x10,0x2E);
	boe_dcs_write_seq_static(ctx,0xE9,0xCD);
	boe_dcs_write_seq_static(ctx,0xBB,0x01);
	boe_dcs_write_seq_static(ctx,0xE9,0x00);
	boe_dcs_write_seq_static(ctx,0xD1,0x67,0x0C,0xFF,0x05);
	boe_dcs_write_seq_static(ctx,0xB1,0x10,0xFA,0xAF,0xAF,0x2B,0x2B,0xC1,0x75,0x39,0x36,0x36,0x36,0x36,0x22,0x21,0x15,0x00);
	boe_dcs_write_seq_static(ctx,0xB2,0x00,0xB0,0x47,0xD0,0x00,0x2C,0x50,0x2C,0x00,0x00,0x00,0x00,0x15,0x20,0xD7,0x00);
	boe_dcs_write_seq_static(ctx,0xB4,0x38,0x47,0x38,0x47,0x66,0x4E,0x00,0x00,0x01,0x72,0x01,0x58,0x00,0xFF,0x00,0xFF);
	boe_dcs_write_seq_static(ctx,0xBF,0xFC,0x85,0x80);
	boe_dcs_write_seq_static(ctx,0xD2,0x2B,0x2B);
	boe_dcs_write_seq_static(ctx,0xD3,0x00,0x00,0x00,0x00,0x78,0x04,0x00,0x14,0x00,0x27,0x00,0x44,0x4F,0x29,0x29,0x00,0x00,0x32,0x10,0x25,0x00,0x25,0x32,0x10,0x1F,0x00,0x1F,0x32,0x18,0x10,0x08,0x10,0x00,0x00,0x20,0x30,0x01,0x55,0x21,0x2E,0x01,0x55,0x0F);
	boe_dcs_write_seq_static(ctx,0xE0,0x00,0x04,0x0B,0x11,0x17,0x26,0x3D,0x45,0x4D,0x4A,0x65,0x6D,0x75,0x87,0x86,0x92,0x9D,0xB0,0xAF,0x56,0x5E,0x68,0x70,0x00,0x04,0x0B,0x11,0x17,0x26,0x3D,0x45,0x4D,0x4A,0x65,0x6D,0x75,0x87,0x86,0x92,0x9D,0xB0,0xAF,0x56,0x5E,0x68,0x70);
	boe_dcs_write_seq_static(ctx,0xBD,0x01);
	boe_dcs_write_seq_static(ctx,0xB1,0x01,0x9B,0x01,0x31);
	boe_dcs_write_seq_static(ctx,0xCB,0xF4,0x36,0x12,0x16,0xC0,0x28,0x6C,0x85,0x3F,0x04);
	boe_dcs_write_seq_static(ctx,0xD3,0x01,0x00,0x3C,0x00,0x00,0x11,0x10,0x00,0x0E,0x00,0x01);
	boe_dcs_write_seq_static(ctx,0xBD,0x02);
	boe_dcs_write_seq_static(ctx,0xB4,0x4E,0x00,0x33,0x11,0x33,0x88);
	boe_dcs_write_seq_static(ctx,0xBF,0xF2,0x00,0x02);
	boe_dcs_write_seq_static(ctx,0xBD,0x00);
	boe_dcs_write_seq_static(ctx,0xC0,0x23,0x23,0x22,0x11,0xA2,0x17,0x00,0x80,0x00,0x00,0x08,0x00,0x63,0x63);
	boe_dcs_write_seq_static(ctx,0xC6,0xF9);
	boe_dcs_write_seq_static(ctx,0xC7,0x30);
	boe_dcs_write_seq_static(ctx,0xC8,0x00,0x04,0x04,0x00,0x00,0x85,0x43,0xFF);
	boe_dcs_write_seq_static(ctx,0xD0,0x07,0x04,0x05);
	boe_dcs_write_seq_static(ctx,0xD5,0x21,0x20,0x21,0x20,0x25,0x24,0x25,0x24,0x18,0x18,0x18,0x18,0x1A,0x1A,0x1A,0x1A,0x1B,0x1B,0x1B,0x1B,0x03,0x02,0x03,0x02,0x01,0x00,0x01,0x00,0x07,0x06,0x07,0x06,0x05,0x04,0x05,0x04,0x18,0x18,0x18,0x18,0x18,0x18,0x18,0x18);
	boe_dcs_write_seq_static(ctx,0xE7,0x12,0x13,0x02,0x02,0x56,0x56,0x0E,0x0E,0x1B,0x27,0x29,0x73,0x25,0x74,0x01,0x07,0x00,0x00,0x00,0x00,0x17,0x00,0x68);
	boe_dcs_write_seq_static(ctx,0xBD,0x01);
	boe_dcs_write_seq_static(ctx,0xE7,0x02,0x38,0x01,0x93,0x0D,0xDD,0x0E);
	boe_dcs_write_seq_static(ctx,0xBD,0x02);
	boe_dcs_write_seq_static(ctx,0xE7,0xFF,0x01,0xFF,0x01,0x00,0x00,0x22,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x81,0x00,0x02,0x40);
	boe_dcs_write_seq_static(ctx,0xBD,0x00);
	boe_dcs_write_seq_static(ctx,0xBA,0x70,0x03,0xA8,0x83,0xF2,0x00,0xC0,0x0D);
	boe_dcs_write_seq_static(ctx,0xBD,0x02);
	boe_dcs_write_seq_static(ctx,0xD8,0xAF,0xFF,0xFF,0xFF,0xF0,0x00,0xAF,0xFF,0xFF,0xFF,0xF0,0x00);
	boe_dcs_write_seq_static(ctx,0xBD,0x03);
	boe_dcs_write_seq_static(ctx,0xD8,0xAA,0xAA,0xAA,0xAA,0xA0,0x00,0xAA,0xAA,0xAA,0xAA,0xA0,0x00,0x55,0x55,0x55,0x55,0x50,0x00,0x55,0x55,0x55,0x55,0x50,0x00);
	boe_dcs_write_seq_static(ctx,0xBD,0x00);
	boe_dcs_write_seq_static(ctx,0xE1,0x01,0x04);
	boe_dcs_write_seq_static(ctx,0xCC,0x02);
	boe_dcs_write_seq_static(ctx,0xBD,0x03);
	boe_dcs_write_seq_static(ctx,0xB2,0x80);
	boe_dcs_write_seq_static(ctx,0xBD,0x00);
	boe_dcs_write_seq_static(ctx,0x35,0x00);
	boe_dcs_write_seq_static(ctx, 0x11);
	msleep(120);
	boe_dcs_write_seq_static(ctx, 0x29);
	msleep(10);
	//boe_dcs_write_seq(ctx, bl_tb0[0], bl_tb0[1]);
	pr_info("%s-\n", __func__);
}

static int boe_disable(struct drm_panel *panel)
{
	struct boe *ctx = panel_to_boe(panel);

	if (!ctx->enabled)
		return 0;

	if (ctx->backlight) {
		ctx->backlight->props.power = FB_BLANK_POWERDOWN;
		backlight_update_status(ctx->backlight);
	}

	ctx->enabled = false;

	return 0;
}

static int boe_unprepare(struct drm_panel *panel)
{

	struct boe *ctx = panel_to_boe(panel);

	if (!ctx->prepared)
		return 0;

	boe_dcs_write_seq_static(ctx, MIPI_DCS_SET_DISPLAY_OFF);
	msleep(50);
	boe_dcs_write_seq_static(ctx, MIPI_DCS_ENTER_SLEEP_MODE);
	msleep(150);

#ifdef HIMAX_WAKE_UP
	if (wake_flag_drm == 0) {
		ctx->reset_gpio = devm_gpiod_get(ctx->dev, "reset", GPIOD_OUT_HIGH);
		gpiod_set_value(ctx->reset_gpio, 0);
		devm_gpiod_put(ctx->dev, ctx->reset_gpio);
	}
#else
	ctx->reset_gpio = devm_gpiod_get(ctx->dev, "reset", GPIOD_OUT_HIGH);
	gpiod_set_value(ctx->reset_gpio, 0);
	devm_gpiod_put(ctx->dev, ctx->reset_gpio);
#endif

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
	ctx->backlight_en = devm_gpiod_get(ctx->dev, "pm-enable", GPIOD_OUT_HIGH);
        gpiod_set_value(ctx->backlight_en, 0);
        devm_gpiod_put(ctx->dev, ctx->backlight_en);
	
#if 0	
#ifdef   CONFIG_MTK_MP3314_BACKLIGHT_IC
	mp3314_ic_i2c_panel_backlight_enable(0);
	mp3314_ic_power_off();
#endif
#endif
	ctx->error = 0;
	ctx->prepared = false;
	return 0;
}

static int boe_prepare(struct drm_panel *panel)
{
	struct boe *ctx = panel_to_boe(panel);
	int ret;

	pr_info("%s+\n", __func__);
	if (ctx->prepared)
		return 0;

	//lcd reset H -> L -> L
	ctx->reset_gpio = devm_gpiod_get(ctx->dev, "reset", GPIOD_OUT_HIGH);
	gpiod_set_value(ctx->reset_gpio, 1);
	usleep_range(10000, 10001);
	gpiod_set_value(ctx->reset_gpio, 0);
	msleep(20);
	gpiod_set_value(ctx->reset_gpio, 1);
	devm_gpiod_put(ctx->dev, ctx->reset_gpio);
	//end
	ctx->bias_pos =
		devm_gpiod_get_index(ctx->dev, "bias", 0, GPIOD_OUT_HIGH);
	gpiod_set_value(ctx->bias_pos, 1);
	devm_gpiod_put(ctx->dev, ctx->bias_pos);

	usleep_range(2000, 2001);
	ctx->bias_neg =
		devm_gpiod_get_index(ctx->dev, "bias", 1, GPIOD_OUT_HIGH);
	gpiod_set_value(ctx->bias_neg, 1);
	devm_gpiod_put(ctx->dev, ctx->bias_neg);
	lp6280s_write_byte(0x00, 0x14);
	lp6280s_write_byte(0x01, 0x14);
	lp6280s_write_byte(0x03, 0x43);
	usleep_range(2000, 2001);
        ctx->backlight_en = devm_gpiod_get(ctx->dev, "pm-enable", GPIOD_OUT_HIGH);
   	gpiod_set_value(ctx->backlight_en, 1);
 	devm_gpiod_put(ctx->dev, ctx->backlight_en);

#if 0	
 #ifdef   CONFIG_MTK_MP3314_BACKLIGHT_IC
	mp3314_ic_power_on();
	mp3314_ic_i2c_panel_backlight_enable(1);
#endif
#endif 
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

	pr_info("%s-\n", __func__);
	return ret;
}

static int boe_enable(struct drm_panel *panel)
{
	struct boe *ctx = panel_to_boe(panel);

	if (ctx->enabled)
		return 0;
    mp3314g_backlight_init();	
	if (ctx->backlight) {
		ctx->backlight->props.power = FB_BLANK_UNBLANK;
		backlight_update_status(ctx->backlight);
	}
    
	ctx->enabled = true;

	return 0;
}

static const struct drm_display_mode default_mode = {
	.clock = 162843,
	.hdisplay = 1200,
	.hsync_start = 1200 + 40,//HFP
	.hsync_end = 1200 + 40+ 8,//HSA
	.htotal = 1200 + 40+ 8 + 28,//HBP
	.vdisplay = 2000,
	.vsync_start = 2000 + 80,//VFP
	.vsync_end = 2000 + 80+ 8,//VSA
	.vtotal = 2000 + 80+ 8 + 38,//VBP
};

#if defined(CONFIG_MTK_PANEL_EXT)
static struct mtk_panel_params ext_params = {
	.pll_clk = 522,
	//.vfp_low_power = 208,
	.cust_esd_check = 0,
	.esd_check_enable = 0,
	.lcm_esd_check_table[0] = {
		.cmd = 0x0A, .count = 1, .para_list[0] = 0x9C,
	},
	.data_rate = 1044,
	.data_rate_khz = 1044207,
	.rotate = 1,

};

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
	if (level > 0) {
		mp3314g_write_byte(0x00,  bl_msb);// msb
		mp3314g_write_byte(0x01, bl_lsb);// lsb
		pr_debug("%d %s,level=%d,bl_msb=%d,bl_lsb=%d\n", __LINE__, __func__, level,bl_msb,bl_lsb);
	} else {
		mp3314g_write_byte(0x00, 0x00);// msb
		mp3314g_write_byte(0x01, 0x00);// lsb
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

	pr_info("drm_mode_vrefresh(m) =%d", drm_mode_vrefresh(m));
		if (!m) {
		pr_err("%s:%d invalid display_mode\n", __func__, __LINE__);
		return ret;
	}
	if (drm_mode_vrefresh(m) == 60)
		ext->params = &ext_params;
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

static struct mtk_panel_funcs ext_funcs = {
	.reset = panel_ext_reset,
	.set_backlight_cmdq = boe_setbacklight_cmdq,
	.ext_param_set = mtk_panel_ext_param_set,
	//.mode_switch = mode_switch,
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
	connector->display_info.width_mm = 156;
	connector->display_info.height_mm = 260;

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
	struct device_node *dsi_node, *remote_node = NULL, *endpoint = NULL;
	struct boe *ctx;
	struct device_node *backlight;
	unsigned int value;
	int ret;

	pr_info("%s+ boe,hx83102,wuxga2000,vdo\n", __func__);

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

	ctx = devm_kzalloc(dev, sizeof(struct boe), GFP_KERNEL);
	if (!ctx)
		return -ENOMEM;

	mipi_dsi_set_drvdata(dsi, ctx);

	ctx->dev = dev;
	dsi->lanes = 4;
	dsi->format = MIPI_DSI_FMT_RGB888;
	dsi->mode_flags =  MIPI_DSI_MODE_VIDEO | MIPI_DSI_MODE_VIDEO_BURST
 			 | MIPI_DSI_MODE_LPM | MIPI_DSI_MODE_EOT_PACKET
  			 | MIPI_DSI_CLOCK_NON_CONTINUOUS;
		
	ret = of_property_read_u32(dev->of_node, "gate-ic", &value);
	if (ret < 0)
		value = 0;
	else
		ctx->gate_ic = value;

	value = 0;
	ret = of_property_read_u32(dev->of_node, "rc-enable", &value);
	if (ret < 0)
		value = 0;
	else {
		ext_params.round_corner_en = value;
	}

	backlight = of_parse_phandle(dev->of_node, "backlight", 0);
	if (backlight) {
		ctx->backlight = of_find_backlight_by_node(backlight);
		of_node_put(backlight);

		if (!ctx->backlight)
			return -EPROBE_DEFER;
	}

	ctx->reset_gpio = devm_gpiod_get(dev, "reset", GPIOD_OUT_HIGH);
	if (IS_ERR(ctx->reset_gpio)) {
		dev_info(dev, "cannot get reset-gpios %ld\n",
			 PTR_ERR(ctx->reset_gpio));
		return PTR_ERR(ctx->reset_gpio);
	}
	devm_gpiod_put(dev, ctx->reset_gpio);
	
	if (ctx->gate_ic == 0) {
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
	}
	
	ctx->backlight_en = devm_gpiod_get(dev, "pm-enable", GPIOD_OUT_HIGH);
	if (IS_ERR(ctx->backlight_en)) {
		dev_info(dev, "cannot get gpio_backlight -gpios %ld\n",
			 PTR_ERR(ctx->backlight_en));
		return PTR_ERR(ctx->backlight_en);
	}
	devm_gpiod_put(dev, ctx->backlight_en);
	mp3314g_backlight_init();

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

	pr_info("%s- boe,hx83102,wuxga2000,vdo\n", __func__);

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
	    .compatible = "boe,hx83102,wuxga2000,vdo",
	},
	{}
};

MODULE_DEVICE_TABLE(of, boe_of_match);

static struct mipi_dsi_driver boe_driver = {
	.probe = boe_probe,
	.remove = boe_remove,
	.driver = {
		.name = "panel-boe-hx83102-wuxga2000-vdo",
		.owner = THIS_MODULE,
		.of_match_table = boe_of_match,
	},
};

module_mipi_dsi_driver(boe_driver);

MODULE_AUTHOR("wulongchao <wulongchao@huaqin.com>");
MODULE_DESCRIPTION("BOE HX83102J WUXGA2000 VDO  LCD Panel Driver");
MODULE_LICENSE("GPL v2");

