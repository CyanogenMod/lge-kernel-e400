#include <linux/err.h>
#include <linux/gpio.h>

#include <asm/mach-types.h>

#include <mach/msm_rpcrouter.h>
#include <mach/rpc_pmapp.h>
#include <mach/board.h>

#include "devices.h"
#include "board-msm7x27a_hdk.h"
#include <mach/board_lge.h>
#include <mach/vreg.h>

/* backlight device */
static struct gpio_i2c_pin bl_i2c_pin = {
	.sda_pin = 123,
	.scl_pin = 122,
	.reset_pin = 124,
};

static struct i2c_gpio_platform_data bl_i2c_pdata = {
	.sda_is_open_drain = 0,
	.scl_is_open_drain = 0,
	.udelay = 2,
};

static struct platform_device bl_i2c_device = {
	.name = "i2c-gpio",
	.dev.platform_data = &bl_i2c_pdata,
};

#ifdef CONFIG_BACKLIGHT_LM3530
static struct lge_backlight_platform_data lm3530bl_data = {
	.gpio = 124,
	.version = 3530,
};
#endif

#ifdef CONFIG_BACKLIGHT_AAT2870
/* For 2.8" Display */
static struct lge_backlight_platform_data aat2870bl_data = {
	.gpio = 124,
#ifdef CONFIG_FB_MSM_MIPI_DSI_MAGNACHIP
	.version = 2870,  /* for WVGA panel */
#else
	.version = 2862,  /* for QVGA panel */
#endif
};
#endif

static struct i2c_board_info bl_i2c_bdinfo[] = {
#ifdef CONFIG_BACKLIGHT_LM3530
	{
		I2C_BOARD_INFO("lm3530bl", 0x38),
		.type = "lm3530bl",
		.platform_data = &lm3530bl_data,
	},
#endif
#ifdef CONFIG_BACKLIGHT_AAT2870
	{
		I2C_BOARD_INFO("aat2870bl", 0x60),
		.type = "aat2870bl",
		.platform_data = &aat2870bl_data,
	},
#endif
};

static struct msm_panel_common_pdata mdp_pdata = {
	.gpio = 97,  /* LCD_VSYNC */
	.mdp_rev = MDP_REV_303,
};

#define GPIO_LCD_RESET 125
#define GPIO_LCD_IFMODE1 129

#ifdef CONFIG_FB_MSM_MIPI_DSI
static struct platform_device mipi_dsi_r61529_panel_device = {
#ifdef CONFIG_FB_MSM_MIPI_DSI_MAGNACHIP
	.name = "mipi_magnachip",  /* for WVGA panel */
#else
	.name = "mipi_r61529",  /* for HVGA panel */
#endif
	.id = 0,
};

enum {
	DSI_SINGLE_LANE = 1,
	DSI_TWO_LANES,
};

static int msm_fb_get_lane_config(void)
{
	int rc = DSI_TWO_LANES;
	return rc;
}

static int dsi_gpio_initialized;

static int mipi_dsi_panel_power(int on)
{
	int rc = 0;
	struct vreg *vreg_mipi_dsi_v28, *vreg_mipi_dsi_v18;

	if (!dsi_gpio_initialized) {
		rc = gpio_request(GPIO_LCD_IFMODE1, "MIPI I/F");
		if (rc)
			pr_err("%s: gpio_request failed\n", __func__);


		/* Resetting LCD Panel */
		rc = gpio_request(GPIO_LCD_RESET, "lcd_reset");
		if (rc)
			pr_err("%s: gpio_request CD_RESET failed\n", __func__);


		dsi_gpio_initialized = 1;
	}

	vreg_mipi_dsi_v28 = vreg_get(0, "emmc");
	if (IS_ERR(vreg_mipi_dsi_v28)) {
		pr_err("%s: vreg_get for emmc failed\n", __func__);
		return PTR_ERR(vreg_mipi_dsi_v28);
	}

	vreg_mipi_dsi_v18 = vreg_get(0, "wlan_tcx0");
	if (IS_ERR(vreg_mipi_dsi_v18)) {
		pr_err("%s: vreg_get for wlan_tcx0 failed\n", __func__);
		rc = PTR_ERR(vreg_mipi_dsi_v18);
		goto vreg_put_dsi_v28;
	}

#ifdef CONFIG_FB_MSM_MIPI_R61529_VIDEO_HVGA_PT
	/* IFMODE1=1 setting is DSI Video mode */
	gpio_direction_output(GPIO_LCD_IFMODE1, 1);
#else
	/* IFMODE1=0 setting is DSI CMD mode */
	gpio_direction_output(GPIO_LCD_IFMODE1, 0);
#endif /* CONFIG_FB_MSM_MIPI_R61529_VIDEO_HVGA_PT */

	if (on) {
		rc = vreg_set_level(vreg_mipi_dsi_v28, 2800);
		if (rc) {
			pr_err("%s: vreg_set_level failed v28\n", __func__);
			goto vreg_put_dsi_v18;
		}
		rc = vreg_enable(vreg_mipi_dsi_v28);
		if (rc) {
			pr_err("%s: vreg_enable failed v28\n", __func__);
			goto vreg_put_dsi_v18;
		}

		rc = vreg_set_level(vreg_mipi_dsi_v18, 1800);
		if (rc) {
			pr_err("%s: vreg_set_level failed v18\n", __func__);
			goto vreg_put_dsi_v18;
		}
		rc = vreg_enable(vreg_mipi_dsi_v18);
		if (rc) {
			pr_err("%s: vreg_enable failed v18\n", __func__);
			goto vreg_put_dsi_v18;
		}

		rc = gpio_direction_output(GPIO_LCD_RESET, 1);
		if (rc) {
			pr_err("%s: gpio_direction_output failed\n", __func__);
			goto vreg_put_dsi_v18;
		}

		msleep(20);
		gpio_set_value(GPIO_LCD_RESET, 0);
		msleep(20);
		gpio_set_value(GPIO_LCD_RESET, 1);
		msleep(20);
	} else {
		rc = vreg_disable(vreg_mipi_dsi_v28);
		if (rc) {
			pr_err("%s: vreg_disable failed v28\n", __func__);
			goto vreg_put_dsi_v18;
		}
		rc = vreg_disable(vreg_mipi_dsi_v18);
		if (rc) {
			pr_err("%s: vreg_disable failed v18\n", __func__);
			goto vreg_put_dsi_v18;
		}
	}

vreg_put_dsi_v18:
	vreg_put(vreg_mipi_dsi_v18);
vreg_put_dsi_v28:
	vreg_put(vreg_mipi_dsi_v28);

	return rc;
}

#define MDP_303_VSYNC_GPIO 97

#ifdef CONFIG_FB_MSM_MDP303
static struct mipi_dsi_platform_data mipi_dsi_pdata = {
	.vsync_gpio = MDP_303_VSYNC_GPIO,
	.dsi_power_save   = mipi_dsi_panel_power,
#ifndef CONFIG_MACH_LGE
	.dsi_client_reset = msm_fb_dsi_client_reset,
#endif
	.get_lane_config = msm_fb_get_lane_config,
};
#endif
#endif /* CONFIG_FB_MSM_MIPI_DSI */

#ifdef CONFIG_FB_MSM_EBI2
#define MSM_FB_LCDC_VREG_OP(name, op, level) \
	do { \
		vreg = vreg_get(0, name); \
		vreg_set_level(vreg, level); \
		if (vreg_##op(vreg)) \
			printk(KERN_ERR "%s: %s vreg operation failed \n", \
				(vreg_##op == vreg_enable) ? "vreg_enable" \
					: "vreg_disable", name); \
	} while (0)

static char *msm_fb_vreg[] = {
	"wlan_tcx0",
	"emmc",
};

static int ebi2_power_save_on;

static int ebi2_hdk_power_save(int on)
{
	struct vreg *vreg;
	int flag_on = !!on;

	printk(KERN_INFO"%s: on=%d\n", __func__, flag_on);

	if (ebi2_power_save_on == flag_on)
		return 0;

	ebi2_power_save_on = flag_on;

	if (on) {
		gpio_direction_output(GPIO_LCD_RESET, 1);

		MSM_FB_LCDC_VREG_OP(msm_fb_vreg[0], enable, 1800);
		MSM_FB_LCDC_VREG_OP(msm_fb_vreg[1], enable, 2800);
	} else{
		/* LGE_CHANGE, [hyuncheol0.kim@lge.com]
		 * 2011-02-10, for current consumption */
		/* MSM_FB_LCDC_VREG_OP(msm_fb_vreg[0], disable, 0); */
		MSM_FB_LCDC_VREG_OP(msm_fb_vreg[1], disable, 0);
	}

	return 0;
}

static struct msm_panel_ilitek_pdata ebi2_hdk_panel_data = {
	.gpio = GPIO_LCD_RESET,
	.lcd_power_save = ebi2_hdk_power_save,
	.maker_id = PANEL_ID_LGDISPLAY,
	.initialized = 0,
};

static struct platform_device ebi2_hdk_panel_device = {
	.name = "ebi2_tovis_qvga",
	.id = 0,
	.dev = {
		.platform_data = &ebi2_hdk_panel_data,
	}
};
#endif /* CONFIG_FB_MSM_EBI2 */

static void __init msm_fb_add_devices(void)
{
	msm_fb_register_device("mdp", &mdp_pdata);
	msm_fb_register_device("lcdc", 0);
#ifdef CONFIG_FB_MSM_MIPI_DSI
	msm_fb_register_device("mipi_dsi", &mipi_dsi_pdata);
#endif
#ifdef CONFIG_FB_MSM_EBI2
	msm_fb_register_device("ebi2", 0);
#endif
}

/* hdk panel platform device */
static struct platform_device *hdk_panel_devices[] __initdata = {
#ifdef CONFIG_FB_MSM_MIPI_DSI
	&mipi_dsi_r61529_panel_device,
#endif
#ifdef CONFIG_FB_MSM_EBI2
	&ebi2_hdk_panel_device,
#endif
};

void __init hdk_init_i2c_backlight(int bus_num)
{
	bl_i2c_device.id = bus_num;

	/* workaround for HDK rev_a no pullup */
	lge_init_gpio_i2c_pin_pullup(&bl_i2c_pdata, bl_i2c_pin,
		&bl_i2c_bdinfo[0]);
	i2c_register_board_info(bus_num, bl_i2c_bdinfo,
		ARRAY_SIZE(bl_i2c_bdinfo));
	platform_device_register(&bl_i2c_device);
}

void __init lge_add_lcd_devices(void)
{
	platform_add_devices(hdk_panel_devices, ARRAY_SIZE(hdk_panel_devices));
	msm_fb_add_devices();
	lge_add_gpio_i2c_device(hdk_init_i2c_backlight);
}
