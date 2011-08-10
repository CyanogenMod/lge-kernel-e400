/* Copyright (c) 2011, Code Aurora Forum. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include "msm_fb.h"
#include "mipi_dsi.h"
#include "mipi_magnachip.h"
#include <linux/gpio.h>
#include <mach/vreg.h>
#include <mach/board_lge.h>

#define MAGNACHIP_CMD_DELAY 0 /* 50 */

static void mipi_ldp_lcd_panel_poweroff(void);

static struct msm_panel_common_pdata *mipi_magnachip_pdata;

static struct dsi_buf magnachip_tx_buf;
static struct dsi_buf magnachip_rx_buf;

static char config_mipi[] = { 0xBC, 0x12, 0x8A, 0x02, 0x04, 0xFF, 0xFF, 0xFF,
	0x10, 0xFF, 0xFF, 0x00, 0xA6, 0x14, 0x0A, 0x19, 0x00, 0x00, 0xFF };
static char config_set_addr[2] = { 0x36, 0x0A };
static char config_setvgmpm[] = { 0xB4, 0xAA };
static char config_set_ddvdhp[] = { 0xB7, 0x1A, 0x33, 0x03, 0x03, 0x03, 0x00,
	0x00, 0x01, 0x02, 0x00, 0x00, 0x00, 0x00, 0x01, 0x01, 0x01 };
static char config_set_ddvdhm[] = { 0xB8, 0x1C, 0x53, 0x03, 0x03, 0x00, 0x01,
	0x02, 0x00, 0x00, 0x04, 0x00, 0x01, 0x01 };
static char config_set_vgh[] = { 0xB9, 0x0A, 0x01, 0x01, 0x00, 0x00, 0x00,
	0x02, 0x00, 0x02, 0x01 };
static char config_set_vgl[] = { 0xBA, 0x0F, 0x01, 0x01, 0x00, 0x00, 0x00,
	0x02, 0x00, 0x02, 0x01 };
static char config_number_of_lines[] = { 0xC1, 0x01 };
/* Remove config_1h_period for strage center lines by bongkyu.kim */
/* static char config_1h_period[] = { 0xC4, 0x04 }; */
static char config_src_precharge[] = { 0xC5, 0x07 };
static char config_src_precharge_timing[] = { 0xC6, 0xC4, 0x04 };
static char config_gateset_3[] = { 0xCA, 0x04, 0x04 };
static char config_dotinv[] = { 0xD6, 0x01 };
static char config_ponseqa[] = { 0xD8, 0x01, 0x05, 0x06, 0x0D, 0x18, 0x09, 0x22,
	0x23, 0x00 };
static char config_ponseqc[] = { 0xDE, 0x09, 0x0F, 0x21, 0x12, 0x04 };
static char config_bl_control[] = { 0x53, 0x40 };
static char config_high_speed_ram[] = { 0xEA, 0x01 };
static char config_gamma_r_pos[] = { 0xEB, 0x00, 0x33, 0x12, 0x10, 0x98, 0x88,
	0x87, 0x0B };
static char config_gamma_r_neg[] = { 0xEC, 0x00, 0x33, 0x12, 0x10, 0x98, 0x88,
	0x87, 0x0B };
static char config_gamma_g_pos[] = { 0xED, 0x00, 0x33, 0x12, 0x10, 0x98, 0x88,
	0x87, 0x0B };
static char config_gamma_g_neg[] = { 0xEE, 0x00, 0x33, 0x12, 0x10, 0x98, 0x88,
	0x87, 0x0B };
static char config_gamma_b_pos[] = { 0xEF, 0x00, 0x33, 0x12, 0x10, 0x98, 0x88,
	0x87, 0x0B };
static char config_gamma_b_neg[] = { 0xF0, 0x00, 0x33, 0x12, 0x10, 0x98, 0x88,
	0x87, 0x0B };
static char config_set_tear_scanline[] = { 0x44, 0x00, 0x00 };

/*---------------------- display_on ----------------------------*/
static char disp_sleep_out[1] = {0x11};
static char disp_display_on[1] = {0x29};

/*---------------------- sleep_mode_on ----------------------------*/
static char sleep_display_off[1] = {0x28};
static char sleep_mode_on[1] = {0x10};

static struct dsi_cmd_desc magnachip_init_on_cmds[] = {
	{DTYPE_DCS_LWRITE, 1, 0, 0, MAGNACHIP_CMD_DELAY,
		sizeof(config_mipi), config_mipi},
	{DTYPE_DCS_WRITE1, 1, 0, 0, MAGNACHIP_CMD_DELAY,
		sizeof(config_set_addr), config_set_addr},
	{DTYPE_GEN_WRITE2, 1, 0, 0, MAGNACHIP_CMD_DELAY,
		sizeof(config_setvgmpm), config_setvgmpm},
	{DTYPE_GEN_LWRITE, 1, 0, 0, MAGNACHIP_CMD_DELAY,
		sizeof(config_set_ddvdhp), config_set_ddvdhp},
	{DTYPE_GEN_LWRITE, 1, 0, 0, MAGNACHIP_CMD_DELAY,
		sizeof(config_set_ddvdhm), config_set_ddvdhm},
	{DTYPE_GEN_LWRITE, 1, 0, 0, MAGNACHIP_CMD_DELAY,
		sizeof(config_set_vgh), config_set_vgh},
	{DTYPE_GEN_LWRITE, 1, 0, 0, MAGNACHIP_CMD_DELAY,
		sizeof(config_set_vgl), config_set_vgl},
	{DTYPE_GEN_WRITE2, 1, 0, 0, MAGNACHIP_CMD_DELAY,
		sizeof(config_number_of_lines), config_number_of_lines},
/* Remove config_1h_period for strage center lines by bongkyu.kim */
/*	{DTYPE_GEN_WRITE2, 1, 0, 0, MAGNACHIP_CMD_DELAY,
		sizeof(config_1h_period), config_1h_period}, */
	{DTYPE_GEN_WRITE2, 1, 0, 0, MAGNACHIP_CMD_DELAY,
		sizeof(config_src_precharge), config_src_precharge},
	{DTYPE_GEN_LWRITE, 1, 0, 0, MAGNACHIP_CMD_DELAY,
		sizeof(config_src_precharge_timing),
		config_src_precharge_timing},
	{DTYPE_GEN_LWRITE, 1, 0, 0, MAGNACHIP_CMD_DELAY,
		sizeof(config_gateset_3), config_gateset_3},
	{DTYPE_GEN_WRITE2, 1, 0, 0, MAGNACHIP_CMD_DELAY,
		sizeof(config_dotinv), config_dotinv},
	{DTYPE_GEN_LWRITE, 1, 0, 0, MAGNACHIP_CMD_DELAY,
		sizeof(config_ponseqa), config_ponseqa},
	{DTYPE_GEN_LWRITE, 1, 0, 0, MAGNACHIP_CMD_DELAY,
		sizeof(config_ponseqc), config_ponseqc},
	{DTYPE_GEN_WRITE2, 1, 0, 0, MAGNACHIP_CMD_DELAY,
		sizeof(config_bl_control), config_bl_control},
	{DTYPE_GEN_WRITE2, 1, 0, 0, MAGNACHIP_CMD_DELAY,
		sizeof(config_high_speed_ram), config_high_speed_ram},
	{DTYPE_GEN_LWRITE, 1, 0, 0, MAGNACHIP_CMD_DELAY,
		sizeof(config_gamma_r_pos), config_gamma_r_pos},
	{DTYPE_GEN_LWRITE, 1, 0, 0, MAGNACHIP_CMD_DELAY,
		sizeof(config_gamma_r_neg), config_gamma_r_neg},
	{DTYPE_GEN_LWRITE, 1, 0, 0, MAGNACHIP_CMD_DELAY,
		sizeof(config_gamma_g_pos), config_gamma_g_pos},
	{DTYPE_GEN_LWRITE, 1, 0, 0, MAGNACHIP_CMD_DELAY,
		sizeof(config_gamma_g_neg), config_gamma_g_neg},
	{DTYPE_GEN_LWRITE, 1, 0, 0, MAGNACHIP_CMD_DELAY,
		sizeof(config_gamma_b_pos), config_gamma_b_pos},
	{DTYPE_GEN_LWRITE, 1, 0, 0, MAGNACHIP_CMD_DELAY,
		sizeof(config_gamma_b_neg), config_gamma_b_neg},
	{DTYPE_DCS_LWRITE, 1, 0, 0, MAGNACHIP_CMD_DELAY,
		sizeof(config_set_tear_scanline), config_set_tear_scanline},
/*	{DTYPE_DCS_LWRITE, 1, 0, 0, MAGNACHIP_CMD_DELAY,
		sizeof(config_write_memory), config_write_memory} */
};

static struct dsi_cmd_desc magnachip_disp_on_cmds[] = {
	{DTYPE_DCS_WRITE, 1, 0, 0, 40,
		sizeof(disp_display_on), disp_display_on}
};

static struct dsi_cmd_desc magnachip_disp_off_cmds[] = {
	{DTYPE_DCS_WRITE, 1, 0, 0, 40,
		sizeof(sleep_display_off), sleep_display_off},
	{DTYPE_DCS_WRITE, 1, 0, 0, MAGNACHIP_CMD_DELAY,
		sizeof(sleep_mode_on), sleep_mode_on},
};

static struct dsi_cmd_desc magnachip_sleep_out_cmds[] = {
	{DTYPE_DCS_WRITE, 1, 0, 0, 120,
		sizeof(disp_sleep_out), disp_sleep_out},
};

static int mipi_magnachip_lcd_on(struct platform_device *pdev)
{
	struct msm_fb_data_type *mfd;
	struct mipi_panel_info *mipi;

	mfd = platform_get_drvdata(pdev);
	mipi  = &mfd->panel_info.mipi;

	if (!mfd)
		return -ENODEV;
	if (mfd->key != MFD_KEY)
		return -EINVAL;

	printk(KERN_INFO "mipi_magnachip_lcd_on START\n");

	mipi_dsi_cmds_tx(mfd, &magnachip_tx_buf, magnachip_sleep_out_cmds,
			ARRAY_SIZE(magnachip_sleep_out_cmds));
	mipi_set_tx_power_mode(1);
	mipi_dsi_cmds_tx(mfd, &magnachip_tx_buf, magnachip_init_on_cmds,
			ARRAY_SIZE(magnachip_init_on_cmds));

	mipi_dsi_cmds_tx(mfd, &magnachip_tx_buf, magnachip_disp_on_cmds,
			ARRAY_SIZE(magnachip_disp_on_cmds));

	mipi_set_tx_power_mode(0);

	printk(KERN_INFO "mipi_magnachip_lcd_on FINISH\n");
	return 0;
}

static int mipi_magnachip_lcd_off(struct platform_device *pdev)
{
	struct msm_fb_data_type *mfd;

	mfd = platform_get_drvdata(pdev);

	if (!mfd)
		return -ENODEV;
	if (mfd->key != MFD_KEY)
		return -EINVAL;

	printk(KERN_INFO "mipi_magnachip_lcd_off START\n");

	mipi_dsi_cmds_tx(mfd, &magnachip_tx_buf, magnachip_disp_off_cmds,
			ARRAY_SIZE(magnachip_disp_off_cmds));

	mipi_ldp_lcd_panel_poweroff();

	return 0;
}

ssize_t mipi_magnachip_lcd_show_onoff(struct device *dev,
	struct device_attribute *attr, char *buf)
{
	printk(KERN_INFO "%s : start\n", __func__);
	return 0;
}

ssize_t mipi_magnachip_lcd_store_onoff(struct device *dev,
	struct device_attribute *attr, const char *buf, size_t count)
{
	/*struct platform_device dummy_pdev;*/
	int onoff;

	sscanf(buf, "%d", &onoff);
	printk(KERN_INFO "%s: onoff : %d\n", __func__, onoff);
	if (onoff)
		mipi_magnachip_lcd_on(NULL);
	else
		mipi_magnachip_lcd_off(NULL);

	return count;
}

DEVICE_ATTR(lcd_onoff, 0664, mipi_magnachip_lcd_show_onoff,
	mipi_magnachip_lcd_store_onoff);

static int __devinit mipi_magnachip_lcd_probe(struct platform_device *pdev)
{
	int rc = 0;

	if (pdev->id == 0) {
		mipi_magnachip_pdata = pdev->dev.platform_data;
		return 0;
	}

	msm_fb_add_device(pdev);
	/*this for AT Command*/
	rc = device_create_file(&pdev->dev, &dev_attr_lcd_onoff);

	return 0;
}

static struct platform_driver this_driver = {
	.probe  = mipi_magnachip_lcd_probe,
	.driver = {
		.name   = "mipi_magnachip",
	},
};

static struct msm_fb_panel_data magnachip_panel_data = {
	.on		= mipi_magnachip_lcd_on,
	.off	= mipi_magnachip_lcd_off,
};

static int ch_used[3];

int mipi_magnachip_device_register(struct msm_panel_info *pinfo,
					u32 channel, u32 panel)
{
	struct platform_device *pdev = NULL;
	int ret;
	if ((channel >= 3) || ch_used[channel])
		return -ENODEV;

	ch_used[channel] = TRUE;

	pdev = platform_device_alloc("mipi_magnachip", (panel << 8)|channel);
	if (!pdev)
		return -ENOMEM;

	magnachip_panel_data.panel_info = *pinfo;

	ret = platform_device_add_data(pdev, &magnachip_panel_data,
		sizeof(magnachip_panel_data));
	if (ret) {
		pr_err("%s: platform_device_add_data failed!\n", __func__);
		goto err_device_put;
	}

	ret = platform_device_add(pdev);
	if (ret) {
		pr_err("%s: platform_device_register failed!\n", __func__);
		goto err_device_put;
	}

	return 0;

err_device_put:
	platform_device_put(pdev);
	return ret;
}

static int __init mipi_magnachip_lcd_init(void)
{
	mipi_dsi_buf_alloc(&magnachip_tx_buf, DSI_BUF_SIZE);
	mipi_dsi_buf_alloc(&magnachip_rx_buf, DSI_BUF_SIZE);

	return platform_driver_register(&this_driver);
}

static void mipi_ldp_lcd_panel_poweroff(void)
{
	gpio_set_value(GPIO_HDK_LCD_RESET, 0);
	msleep(20);
}

module_init(mipi_magnachip_lcd_init);
