#include <linux/err.h>
#include <linux/gpio.h>

#include <asm/mach-types.h>

#include <mach/msm_rpcrouter.h>
#include <mach/rpc_pmapp.h>
#include <mach/board.h>

#include "devices.h"
#include "board-m3.h"

#define LCDC_CONFIG_PROC          21
#define LCDC_UN_CONFIG_PROC       22
#define LCDC_API_PROG             0x30000066
#define LCDC_API_VERS             0x00010001

static struct msm_rpc_endpoint *lcdc_ep;

static int msm_fb_lcdc_config(int on)
{
	int rc = 0;
	struct rpc_request_hdr hdr;

	if (on)
		pr_info("lcdc config\n");
	else
		pr_info("lcdc un-config\n");

	lcdc_ep = msm_rpc_connect_compatible(LCDC_API_PROG, LCDC_API_VERS, 0);

	if (IS_ERR(lcdc_ep)) {
		printk(KERN_ERR "%s: msm_rpc_connect failed! rc = %ld\n",
			__func__, PTR_ERR(lcdc_ep));
		return -EINVAL;
	}

	rc = msm_rpc_call(lcdc_ep,
		(on) ? LCDC_CONFIG_PROC : LCDC_UN_CONFIG_PROC,
		&hdr, sizeof(hdr), 5 * HZ);

	if (rc)
		printk(KERN_ERR
			"%s: msm_rpc_call failed! rc = %d\n", __func__, rc);

	msm_rpc_close(lcdc_ep);
	return rc;
}

static uint32_t lcdc_gpio_initialized;

static void lcdc_toshiba_gpio_init(void)
{
	if (!lcdc_gpio_initialized) {
		if (gpio_request(GPIO_SPI_CLK, "spi_clk")) {
			pr_err("failed to request gpio spi_clk\n");
			goto fail_gpio;
		}
		if (gpio_request(GPIO_SPI_CS0_N, "spi_cs")) {
			pr_err("failed to request gpio spi_cs0_N\n");
			goto fail_gpio;
		}
		if (gpio_request(GPIO_SPI_MOSI, "spi_mosi")) {
			pr_err("failed to request gpio spi_mosi\n");
			goto fail_gpio;
		}
		if (gpio_request(GPIO_SPI_MISO, "spi_miso")) {
			pr_err("failed to request gpio spi_miso\n");
			goto fail_gpio;
		}
		if (gpio_request(GPIO_DISPLAY_PWR_EN, "gpio_disp_pwr")) {
			pr_err("failed to request gpio_disp_pwr\n");
			goto fail_gpio;
		}
		if (gpio_request(GPIO_BACKLIGHT_EN, "gpio_bkl_en")) {
			pr_err("failed to request gpio_bkl_en\n");
			goto fail_gpio;
		}
		lcdc_gpio_initialized = 1;
		return;
	}
fail_gpio:
	lcdc_gpio_initialized = 0;
}

static uint32_t lcdc_gpio_table[] = {
	GPIO_SPI_CLK,
	GPIO_SPI_CS0_N,
	GPIO_SPI_MOSI,
	GPIO_DISPLAY_PWR_EN,
	GPIO_BACKLIGHT_EN,
	GPIO_SPI_MISO,
};

static void config_lcdc_gpio_table(uint32_t *table, int len, unsigned enable)
{
	int n;

	if (lcdc_gpio_initialized) {
		/* All are IO Expander GPIOs */
		for (n = 0; n < (len - 1); n++)
			gpio_direction_output(table[n], 1);
	}
}

static void lcdc_toshiba_config_gpios(int enable)
{
	config_lcdc_gpio_table(lcdc_gpio_table,
		ARRAY_SIZE(lcdc_gpio_table), enable);
}

static int msm_fb_lcdc_power_save(int on)
{
	/* struct vreg *vreg[ARRAY_SIZE(msm_fb_lcdc_vreg)]; */
	int rc = 0;

	/* Doing the init of the LCDC GPIOs very late as they are from
		an I2C-controlled IO Expander */
	lcdc_toshiba_gpio_init();

	if (lcdc_gpio_initialized) {
		gpio_set_value_cansleep(GPIO_DISPLAY_PWR_EN, on);
		gpio_set_value_cansleep(GPIO_BACKLIGHT_EN, on);
	}

	pmapp_disp_backlight_init();
	rc = pmapp_disp_backlight_set_brightness(100);

	return 0;
}

static struct lcdc_platform_data lcdc_pdata = {
	.lcdc_gpio_config = msm_fb_lcdc_config,
	.lcdc_power_save   = msm_fb_lcdc_power_save,
};

static int lcd_panel_spi_gpio_num[] = {
		GPIO_SPI_MOSI,  /* spi_sdi */
		GPIO_SPI_MISO,  /* spi_sdoi */
		GPIO_SPI_CLK,   /* spi_clk */
		GPIO_SPI_CS0_N, /* spi_cs  */
};

static struct msm_panel_common_pdata lcdc_toshiba_panel_data = {
	.panel_config_gpio = lcdc_toshiba_config_gpios,
	.gpio_num	  = lcd_panel_spi_gpio_num,
};

static struct platform_device lcdc_toshiba_panel_device = {
	.name   = "lcdc_toshiba_fwvga_pt",
	.id     = 0,
	.dev    = {
		.platform_data = &lcdc_toshiba_panel_data,
	}
};

#ifdef CONFIG_FB_MSM_MIPI_DSI
static struct platform_device mipi_dsi_renesas_panel_device = {
	.name = "mipi_renesas",
	.id = 0,
};
#endif

/* input platform device */
static struct platform_device *m3_panel_devices[] __initdata = {
	&lcdc_toshiba_panel_device,
#ifdef CONFIG_FB_MSM_MIPI_DSI
	&mipi_dsi_renesas_panel_device,
#endif
};

static struct msm_panel_common_pdata mdp_pdata = {
	.gpio = 97,
};

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

static int dsi_gpio_initialized;

static int mipi_dsi_panel_power(int on)
{
	int rc = 0;

	if (!on)
		return rc;

	/* I2C-controlled GPIO Expander -init of the GPIOs very late */
	if (!dsi_gpio_initialized) {
		pmapp_disp_backlight_init();

		if (pmapp_disp_backlight_set_brightness(100))
			pr_err("display backlight set brightness failed\n");

		if (machine_is_msm7x27a_surf() || machine_is_msm7x27a_m3()) {
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
	}

	return rc;
}

#define MDP_303_VSYNC_GPIO 97

#ifdef CONFIG_FB_MSM_MDP303
static struct mipi_dsi_platform_data mipi_dsi_pdata = {
	.vsync_gpio = MDP_303_VSYNC_GPIO,
	.dsi_power_save   = mipi_dsi_panel_power,
	.dsi_client_reset = msm_fb_dsi_client_reset,
};
#endif

static void __init msm_fb_add_devices(void)
{
	msm_fb_register_device("mdp", &mdp_pdata);
	msm_fb_register_device("lcdc", &lcdc_pdata);
	msm_fb_register_device("mipi_dsi", &mipi_dsi_pdata);
}

/* common functions */
void __init lge_add_lcd_devices(void)
{
	platform_add_devices(m3_panel_devices, ARRAY_SIZE(m3_panel_devices));
	msm_fb_add_devices();
}

