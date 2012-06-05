#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/i2c.h>
#include <linux/gpio.h>
#include <linux/regulator/consumer.h>

#include <asm/mach-types.h>

#include <mach/gpio.h>
#include <mach/vreg.h>
#include <mach/board.h>
#include <mach/msm_iomap.h>
#include <mach/board_lge.h>
#include <mach/camera.h>
#include <linux/mutex.h>

#include "devices-msm7x2xa.h"
#include "board-e0eu.h"
//LGE_CHANGE E0_CAMERA_PORTING hong.junki@lge.com 2011-10-23
DEFINE_MUTEX(camera_power_mutex);

#define HI351_I2C_ADDR 	(0x40>>2)

#define HI351_MASTER_CLK_RATE 24000000

/* LGE_CHANGE_S: E0 jiwon.seo@lge.com [2011-10-17] : for new bl */
#if defined(CONFIG_BACKLIGHT_AAT2870)
extern int aat28xx_ldo_enable(struct device *dev, unsigned num, unsigned enable);
extern int aat28xx_ldo_set_level(struct device *dev, unsigned num, unsigned vol);
#elif defined(CONFIG_BACKLIGHT_BU61800)
extern int bu61800_ldo_enable(struct device *dev, unsigned num, unsigned enable);
#else
//default
#endif
/* LGE_CHANGE_E: E0 jiwon.seo@lge.com [2011-10-17] : for new bl */

#ifdef CONFIG_MSM_CAMERA
static uint32_t camera_off_gpio_table[] = {
	GPIO_CFG(15, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),
	GPIO_CFG(42, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA),
};

static uint32_t camera_on_gpio_table[] = {
	GPIO_CFG(15, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),
	GPIO_CFG(42, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA),
};

#ifdef CONFIG_HI351
static void msm_camera_vreg_config(int vreg_en)
{
	int rc;
	static int gpio_initialzed;

	if (!gpio_initialzed) {
		rc = gpio_request(GPIO_CAM_PWDN, "hi351_pwdn");
		if (rc < 0) {
			pr_err("%s: gpio_request(GPIO_CAM_PWDN) failed\n", __func__);
		}
		
		rc = gpio_request(GPIO_CAM_RESET, "hi351_reset");
		if (rc < 0) {
			pr_err("%s: gpio_request(GPIO_CAM_RESET) failed\n", __func__);
		}
		
		rc = gpio_direction_output(GPIO_CAM_PWDN, 0);
		if (rc < 0) {
			pr_err("%s: gpio_direction_output(GPIO_CAM_PWDN, 0) failed(1)\n", __func__);
		}

		rc = gpio_direction_output(GPIO_CAM_RESET, 0);
		if (rc < 0) {
			pr_err("%s: gpio_direction_output(GPIO_CAM_RESET, 0) failed(2)\n", __func__);
		}
		gpio_initialzed = 1;
	}
	
#if defined(CONFIG_BACKLIGHT_AAT2870)
	if (vreg_en) {
		pr_err("%s: msm_camera_vreg_config power on vreg_en enable\n", __func__);

		//IOVDD: 1.8V START
		rc = aat28xx_ldo_set_level(NULL, 4, 1800);
		if (rc < 0) {
			pr_err("%s: aat28xx_ldo_set_level(ldo4) failed\n", __func__);
		}
		rc = aat28xx_ldo_enable(NULL, 4, vreg_en);
		if (rc < 0) {
			pr_err("%s: aat28xx_ldo_enable(ldo4) failed\n", __func__);
		}
		//IOVDD: 1.8V END

		//AVDD: 2.8V START
		rc = aat28xx_ldo_set_level(NULL, 2, 2800);
		if (rc < 0) {
			pr_err("%s: aat28xx_ldo_set_level(ldo2) failed\n", __func__);
		}
		rc = aat28xx_ldo_enable(NULL, 2, vreg_en);
		if (rc < 0) {
			pr_err("%s: aat28xx_ldo_enable(ldo2) failed\n", __func__);
		}
		//AVDD: 2.8V END

		//DVDD: 1.2V START
		rc = aat28xx_ldo_set_level(NULL, 3, 1200); //DVDD: 1.2V E0
		if (rc < 0) {
			pr_err("%s: aat28xx_ldo_set_level(ldo3) failed\n", __func__);
		}
		rc = aat28xx_ldo_enable(NULL, 3, vreg_en);
		if (rc < 0) {
			pr_err("%s: aat28xx_ldo_enable(ldo3) failed\n", __func__);
		}
		//DVDD: 1.2V END

	} 
	else {
	 	pr_err("%s: msm_camera_vreg_config power on vreg_en disable start\n", __func__);

		rc = aat28xx_ldo_enable(NULL, 3, 0);
		if (rc < 0) {
			pr_err("%s: aat28xx_ldo_enable(ldo3) OFF failed\n", __func__);
		}

		rc = aat28xx_ldo_enable(NULL, 2, 0);
		if (rc < 0) {
			pr_err("%s: aat28xx_ldo_enable(ldo2) OFF failed\n", __func__);
		}

		rc = aat28xx_ldo_enable(NULL, 4, 0);
		if (rc < 0) {
			pr_err("%s: aat28xx_ldo_enable(ldo4) OFF failed\n", __func__);
		}
		pr_err("%s: msm_camera_vreg_config power on vreg_en disable end\n", __func__);

	}
#elif defined(CONFIG_BACKLIGHT_BU61800)
	if (vreg_en) {
		pr_err("%s: msm_camera_vreg_config power on vreg_en enable\n", __func__);

		//IOVDD: 1.8V START

		rc = bu61800_ldo_enable(NULL,4,vreg_en); 
		if (rc < 0) {
			pr_err("%s: aat28xx_ldo_enable(ldo4) failed\n", __func__);
		}
		//IOVDD: 1.8V END

		//AVDD: 2.8V START

		rc = bu61800_ldo_enable(NULL,2,vreg_en);
		if (rc < 0) {
			pr_err("%s: aat28xx_ldo_enable(ldo2) failed\n", __func__);
		}
		//AVDD: 2.8V END

		//DVDD: 1.2V START

		rc = bu61800_ldo_enable(NULL,3,vreg_en);
		if (rc < 0) {
			pr_err("%s: aat28xx_ldo_enable(ldo3) failed\n", __func__);
		}
		//DVDD: 1.2V END

	} 
	else {
		
	 	pr_err("%s: msm_camera_vreg_config power on vreg_en disable start\n", __func__);

		rc = bu61800_ldo_enable(NULL,3,vreg_en);
		if (rc < 0) {
			pr_err("%s: aat28xx_ldo_enable(ldo3) OFF failed\n", __func__);
		}

		rc = bu61800_ldo_enable(NULL,2,vreg_en);
		if (rc < 0) {
			pr_err("%s: aat28xx_ldo_enable(ldo2) OFF failed\n", __func__);
		}

		rc = bu61800_ldo_enable(NULL,4,vreg_en);
		if (rc < 0) {
			pr_err("%s: aat28xx_ldo_enable(ldo4) OFF failed\n", __func__);
		}
		pr_err("%s: msm_camera_vreg_config power on vreg_en disable end\n", __func__);

	}
#else
	//default
#endif
	return;
}
#endif

static int config_gpio_table(uint32_t *table, int len)
{
	int rc = 0, i = 0;

	for (i = 0; i < len; i++) {
		rc = gpio_tlmm_config(table[i], GPIO_CFG_ENABLE);
		if (rc) {
			pr_err("%s not able to get gpio\n", __func__);
			for (i--; i >= 0; i--)
				gpio_tlmm_config(camera_off_gpio_table[i],
							GPIO_CFG_ENABLE);
			break;
		}
	}
	return rc;
}

static int config_camera_on_gpios_rear(void)
{
	int rc = 0;

	msm_camera_vreg_config(1);

	mdelay(2);

	rc = config_gpio_table(camera_on_gpio_table,
			ARRAY_SIZE(camera_on_gpio_table));
	if (rc < 0) {
		pr_err("%s: CAMSENSOR gpio table request"
		"failed\n", __func__);
		return rc;
	}

	return rc;
}

static void config_camera_off_gpios_rear(void)
{
	gpio_direction_output(GPIO_CAM_RESET, 0);

	mdelay(1);

	gpio_direction_output(GPIO_CAM_PWDN, 0);

	mdelay(1);

	config_gpio_table(camera_off_gpio_table,
			ARRAY_SIZE(camera_off_gpio_table));

	msm_camera_vreg_config(0);

}

#ifdef CONFIG_HI351
static int camera_power_on_rear(void)
{
	int rc = 0;
	mutex_lock(&camera_power_mutex);

	msm_camio_clk_rate_set(HI351_MASTER_CLK_RATE);
	pr_err("%s: msm_camio_clk_rate_set\n", __func__);

	udelay(10);	
		
	rc = gpio_direction_output(GPIO_CAM_PWDN, 1);
	if (rc < 0) {
		pr_err("%s: gpio_direction_output(GPIO_CAM_PWDN, 1) failed(2)\n", __func__);
	}	

	mdelay(30);

	gpio_set_value(GPIO_CAM_RESET, 1);
	pr_err("%s: gpio_set_value(GPIO_CAM_RESET, 1) final\n", __func__);

	mdelay(1);

	mutex_unlock(&camera_power_mutex);


	return rc;
}
#endif

static int camera_power_off_rear(void)
{
	//Dummy function
	return 0;
}

struct msm_camera_device_platform_data msm_camera_device_data_rear = {
	.camera_gpio_on  = config_camera_on_gpios_rear,
	.camera_gpio_off = config_camera_off_gpios_rear,
	.ioext.csiphy = 0xA1000000,
	.ioext.csisz  = 0x00100000,
	.ioext.csiirq = INT_CSI_IRQ_1,
	.ioclk.mclk_clk_rate = 24000000,
	.ioclk.vfe_clk_rate  = 192000000,
	.ioext.appphy = MSM_CLK_CTL_PHYS,
	.ioext.appsz  = MSM_CLK_CTL_SIZE,
	.camera_power_on   = camera_power_on_rear,
	.camera_power_off  = camera_power_off_rear,	
};

static struct msm_camera_sensor_flash_data led_flash_data = {
	.flash_type = MSM_CAMERA_FLASH_NONE,
	.flash_src  = NULL,
};

#ifdef CONFIG_HI351
static struct msm_camera_sensor_platform_info hi351_sensor_info = {
	.mount_angle = 0
};

static struct msm_camera_sensor_info msm_camera_sensor_hi351_data = {
	.sensor_name    = "hi351",
	.sensor_reset_enable = 1,
	.sensor_reset   = GPIO_CAM_RESET,
	.sensor_pwd     = GPIO_CAM_PWDN,
	.vcm_pwd        = 0,
	.vcm_enable     = 0,
	.pdata          = &msm_camera_device_data_rear,
	.flash_data     = &led_flash_data,
	.csi_if         = 1,
	.sensor_platform_info = &hi351_sensor_info,
};

static struct platform_device msm_camera_sensor_hi351 = {
        .name      = "msm_camera_hi351",
        .dev       = {
                .platform_data = &msm_camera_sensor_hi351_data,
        },
};
#endif//CONFIG_HI351

static struct i2c_board_info i2c_camera_devices[] = {

#ifdef CONFIG_HI351
	{
		I2C_BOARD_INFO("hi351", HI351_I2C_ADDR),
	},
#endif

};

#endif /* CONFIG_MSM_CAMERA */

static struct platform_device *e0eu_camera_devices[] __initdata = {
#ifdef CONFIG_HI351
    &msm_camera_sensor_hi351,
#endif

};

void __init lge_add_camera_devices(void)
{
#ifdef CONFIG_MSM_CAMERA
	i2c_register_board_info(MSM_GSBI0_QUP_I2C_BUS_ID,
		i2c_camera_devices,
		ARRAY_SIZE(i2c_camera_devices));
#endif
	platform_add_devices(e0eu_camera_devices, ARRAY_SIZE(e0eu_camera_devices));

}
