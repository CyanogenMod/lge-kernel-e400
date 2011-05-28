#include <linux/err.h>
#include <linux/gpio.h>

#include <asm/mach-types.h>

#include <mach/msm_rpcrouter.h>
#include <mach/rpc_pmapp.h>
#include <mach/board.h>

#include "devices.h"
#include "board-m3.h"
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

static struct lge_backlight_platform_data lm3530bl_data = {
	.gpio = 124,
	.version = 3530,
};

static struct i2c_board_info bl_i2c_bdinfo[] = {
	[0] = {
		I2C_BOARD_INFO("lm3530bl", 0x38),
		.type = "lm3530bl",
	},
};

static struct platform_device mipi_dsi_r61529_panel_device = {
	.name = "mipi_r61529",
	.id = 0,
};

/* input platform device */
static struct platform_device *m3_panel_devices[] __initdata = {
	&mipi_dsi_r61529_panel_device,
};

static struct msm_panel_common_pdata mdp_pdata = {
	.gpio = 97,						//LCD_VSYNC_O
};

#ifndef CONFIG_MACH_LGE
#define GPIO_LCDC_BRDG_PD	128
#define GPIO_LCDC_BRDG_RESET_N	129
static unsigned mipi_dsi_gpio[] = {
	GPIO_CFG(GPIO_LCDC_BRDG_RESET_N, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL,
		GPIO_CFG_2MA),       /* LCDC_BRDG_RESET_N */
};

static int msm_fb_dsi_client_reset(void)
{
	int rc = 0;

	rc = gpio_request(GPIO_LCDC_BRDG_RESET_N, "lcdc_brdg_reset_n");

	if (rc < 0) {
		pr_err("failed to request lcd brdg reset\n");
		return rc;
	}

	rc = gpio_tlmm_config(mipi_dsi_gpio[0], GPIO_CFG_ENABLE);
	if (rc) {
		pr_err("Failed to enable LCDC Bridge reset enable\n");
		goto gpio_error;
	}

	rc = gpio_direction_output(GPIO_LCDC_BRDG_RESET_N, 1);
	if (!rc) {
		gpio_set_value_cansleep(GPIO_LCDC_BRDG_RESET_N, 0);
		msleep(20);
		gpio_set_value_cansleep(GPIO_LCDC_BRDG_RESET_N, 1);
		return rc;
	} else {
		goto gpio_error;
	}

gpio_error:
	pr_err("Failed GPIO bridge reset\n");
	gpio_free(GPIO_LCDC_BRDG_RESET_N);
	return rc;
}
#endif

static int dsi_gpio_initialized;

static int mipi_dsi_panel_power(int on)
{
	int rc = 0;

	if (!on)
		return rc;

	/* I2C-controlled GPIO Expander -init of the GPIOs very late */
	if (!dsi_gpio_initialized) {
		struct vreg *vreg_mipi_dsi_v28, *vreg_mipi_dsi_v18; 
		printk("seongjae : mipi_dsi_panel_power = on\n");
		pmapp_disp_backlight_init();

		if (pmapp_disp_backlight_set_brightness(100))
			pr_err("display backlight set brightness failed\n");

		vreg_mipi_dsi_v28 = vreg_get(0, "emmc");
		if (IS_ERR(vreg_mipi_dsi_v28))
			return PTR_ERR(vreg_mipi_dsi_v28); 

		rc = vreg_set_level(vreg_mipi_dsi_v28, 2800); 
		if (rc) {
			pr_err("MIPI v28 Set Failed\n");
		}
		rc = vreg_enable(vreg_mipi_dsi_v28); 
		if (rc) {
			pr_err("MIPI v28 Enable Failed\n");
		}
		
		vreg_mipi_dsi_v18 = vreg_get(0, "wlan_tcx0");
		if (IS_ERR(vreg_mipi_dsi_v18))
			return PTR_ERR(vreg_mipi_dsi_v18); 

		vreg_set_level(vreg_mipi_dsi_v18, 1800); 
		if (rc) {
			pr_err("MIPI v18 Set Failed\n");
		}
		vreg_enable(vreg_mipi_dsi_v18); 

		rc = gpio_request(129, "MIPI I/F");
		if (rc) {
			pr_err("MIPI v18 Enable Failed\n");
		}

#if defined (CONFIG_FB_MSM_MIPI_R61529_VIDEO_HVGA_PT)
		gpio_direction_output(129, 1); /* IFMODE1=1 setting is DSI Video mode */
#else
		gpio_direction_output(129, 0); /* IFMODE1=1 setting is DSI CMD mode */
#endif
		printk("seongjae : mipi_dsi_panel_power FINISHED\n");
#ifndef CONFIG_MACH_LGE
		if (machine_is_msm7x27a_surf()) {
			rc = gpio_request(GPIO_DISPLAY_PWR_EN, "gpio_disp_pwr");
			if (rc < 0) {
				pr_err("failed to request gpio_disp_pwr\n");
				return rc;
			}

			if (gpio_request(GPIO_BACKLIGHT_EN, "gpio_bkl_en")) {
				pr_err("failed to request gpio_bkl_en\n");
				goto fail_gpio1;
			}

			if (!gpio_direction_output(GPIO_DISPLAY_PWR_EN, 1)) {
				gpio_set_value_cansleep(GPIO_DISPLAY_PWR_EN,
					on);
			} else {
				pr_err("failed to enable display pwr\n");
				goto fail_gpio2;
			}

			if (!gpio_direction_output(GPIO_BACKLIGHT_EN, 1)) {
				gpio_set_value_cansleep(GPIO_BACKLIGHT_EN, on);
				return rc;
			} else {
				pr_err("failed to enable backlight\n");
				goto fail_gpio2;
			}

fail_gpio2:
		gpio_free(GPIO_BACKLIGHT_EN);
fail_gpio1:
		gpio_free(GPIO_DISPLAY_PWR_EN);

		} else {
			rc = gpio_request(GPIO_FFA_LCD_PWR_EN_N,
				"gpio_disp_pwr");
			if (rc < 0) {
				pr_err("failed to request lcd pwr\n");
				return rc;
			}
			if (!gpio_direction_output(GPIO_FFA_LCD_PWR_EN_N,
				1)) {
				gpio_set_value_cansleep(GPIO_FFA_LCD_PWR_EN_N,
					on);
				return rc;
			} else {
				pr_err("failed to enable lcd pwr\n");
				return rc;
			}

		}
#endif
	}

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
};
#endif

static void __init msm_fb_add_devices(void)
{
	msm_fb_register_device("mdp", &mdp_pdata);
	msm_fb_register_device("lcdc", 0);
	msm_fb_register_device("mipi_dsi", &mipi_dsi_pdata);
}

void __init msm7x27a_m3_init_i2c_backlight(int bus_num)
{
	bl_i2c_device.id = bus_num;
	bl_i2c_bdinfo[0].platform_data = &lm3530bl_data;
	
	/* workaround for HDK rev_a no pullup */
	lge_init_gpio_i2c_pin_pullup(&bl_i2c_pdata, bl_i2c_pin, &bl_i2c_bdinfo[0]);
	i2c_register_board_info(bus_num, &bl_i2c_bdinfo[0], 1);
	platform_device_register(&bl_i2c_device);
}

void __init lge_add_lcd_devices(void)
{
	platform_add_devices(m3_panel_devices, ARRAY_SIZE(m3_panel_devices));
	msm_fb_add_devices();
	lge_add_gpio_i2c_device(msm7x27a_m3_init_i2c_backlight);
}

