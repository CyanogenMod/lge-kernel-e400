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

#include "devices-msm7x2xa.h"
#include "board-m3mpcs.h"

#ifdef CONFIG_MSM_CAMERA
static uint32_t camera_off_gpio_table[] = {
	GPIO_CFG(15, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),
};

static uint32_t camera_on_gpio_table[] = {
	GPIO_CFG(15, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),
};

static void msm_camera_vreg_config(int vreg_en)
{
	static int gpio_initialzed;
	static struct regulator *ldo2;
	static struct regulator *ldo3;
	static struct regulator *ldo4;
	int rc;

	if (!gpio_initialzed) {
		gpio_request(GPIO_CAM_RESET, "cam_reset");
		gpio_direction_output(GPIO_CAM_RESET, 0);
		gpio_initialzed = 1;
	}

	if (vreg_en) {
		gpio_set_value(GPIO_CAM_RESET, 1);
		mdelay(1);

		/* TODO : error checking */
		ldo4 = regulator_get(NULL, "RT8053_LDO4");
		if (ldo4 == NULL)
			pr_err("%s: regulator_get(ldo4) failed\n", __func__);

		ldo2 = regulator_get(NULL, "RT8053_LDO2");
		if (ldo2 == NULL)
			pr_err("%s: regulator_get(ldo2) failed\n", __func__);

		ldo3 = regulator_get(NULL, "RT8053_LDO3");
		if (ldo3 == NULL)
			pr_err("%s: regulator_get(ldo3) failed\n", __func__);

		rc = regulator_set_voltage(ldo4, 1800000, 1800000);
		if (rc < 0)
			pr_err("%s: regulator_set_voltage(ldo4) failed\n",
				__func__);
		rc = regulator_enable(ldo4);
		if (rc < 0)
			pr_err("%s: regulator_enable(ldo4) failed\n",
				__func__);

		rc = regulator_set_voltage(ldo2, 2800000, 2800000);
		if (rc < 0)
			pr_err("%s: regulator_set_voltage(ldo2) failed\n",
				__func__);
		rc = regulator_enable(ldo2);
		if (rc < 0)
			pr_err("%s: regulator_enable(ldo2) failed\n",
				__func__);

		rc = regulator_set_voltage(ldo3, 2800000, 2800000);
		if (rc < 0)
			pr_err("%s: regulator_set_voltage(ldo3) failed\n",
				__func__);
		rc = regulator_enable(ldo3);
		if (rc < 0)
			pr_err("%s: regulator_enable(ldo3) failed\n",
				__func__);
	} else {
		gpio_set_value(GPIO_CAM_RESET, 0);

		rc = regulator_disable(ldo3);
		if (rc < 0)
			pr_err("%s: regulator_disable(ldo3) failed\n",
				__func__);
		regulator_put(ldo3);

		rc = regulator_disable(ldo2);
		if (rc < 0)
			pr_err("%s: regulator_disble(ldo2) failed\n",
				__func__);
		regulator_put(ldo2);

		rc = regulator_disable(ldo4);
		if (rc < 0)
			pr_err("%s: regulator_disable(ldo4) failed\n",
				__func__);
		regulator_put(ldo4);
	}

	return;
}

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
	msm_camera_vreg_config(0);

	config_gpio_table(camera_off_gpio_table,
			ARRAY_SIZE(camera_off_gpio_table));
}

static int camera_power_on_rear(void)
{
	/* mt9p017 power on sequence: cam reset after enabling MCLK */
	gpio_set_value(GPIO_CAM_RESET, 0);
	mdelay(2);
	gpio_set_value(GPIO_CAM_RESET, 1);
	mdelay(2);
	return 0;
}

static int camera_power_off_rear(void)
{
	/* TODO: dummy function */
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

#ifdef CONFIG_MSM_CAMERA_FLASH_LM3559
static struct msm_camera_sensor_flash_src led_flash_src = {
	.flash_sr_type = MSM_CAMERA_FLASH_SRC_CURRENT_DRIVER,
};

static struct msm_camera_sensor_flash_data led_flash_data = {
	.flash_type = MSM_CAMERA_FLASH_LED,
	.flash_src  = &led_flash_src,
};
#else
static struct msm_camera_sensor_flash_data led_flash_data = {
	.flash_type = MSM_CAMERA_FLASH_NONE,
	.flash_src  = NULL,
};
#endif

#ifdef CONFIG_MT9P017
static struct msm_camera_sensor_platform_info mt9p017_sensor_info = {
	.mount_angle = 0
};

static struct msm_camera_sensor_info msm_camera_sensor_mt9p017_data = {
	.sensor_name    = "mt9p017",
	.sensor_reset_enable = 1,
	.sensor_reset   = GPIO_CAM_RESET,
	.sensor_pwd     = 0,
	.vcm_pwd        = 0,
	.vcm_enable     = 0,
	.pdata          = &msm_camera_device_data_rear,
	.flash_data     = &led_flash_data,
	.csi_if         = 1,
	.sensor_platform_info = &mt9p017_sensor_info,
};

static struct platform_device msm_camera_sensor_mt9p017 = {
	.name = "msm_camera_mt9p017",
	.dev  = {
		.platform_data = &msm_camera_sensor_mt9p017_data,
	},
};
#endif

static struct i2c_board_info i2c_camera_devices[] = {
#ifdef CONFIG_MT9P017
	{
		I2C_BOARD_INFO("mt9p017", 0x1B),
	},
#endif
};

#ifdef CONFIG_MSM_CAMERA_FLASH_LM3559
/* LM3559 flash led driver */
static struct gpio_i2c_pin flash_i2c_pin[] = {
	{
		.sda_pin	= GPIO_FLASH_I2C_SDA,
		.scl_pin	= GPIO_FLASH_I2C_SCL,
		.reset_pin	= 0,
		.irq_pin	= 0,
	},
};

static struct i2c_gpio_platform_data flash_i2c_pdata = {
	.sda_is_open_drain	= 0,
	.scl_is_open_drain	= 0,
	.udelay			    = 2,
};

static struct platform_device flash_i2c_device = {
	.name	= "i2c-gpio",
	.dev.platform_data = &flash_i2c_pdata,
};

static struct led_flash_platform_data lm3559_flash_pdata = {
	.gpio_flen = GPIO_FLASH_EN,
};

static struct i2c_board_info i2c_camera_flash_devices[] = {
	{
		I2C_BOARD_INFO("lm3559", FLASH_I2C_ADDRESS),
		.platform_data = &lm3559_flash_pdata,
	},
};
#endif

#endif /* CONFIG_MSM_CAMERA */

static struct platform_device *m3mpcs_camera_devices[] __initdata = {
#ifdef CONFIG_MT9P017
	&msm_camera_sensor_mt9p017,
#endif
};

#ifdef CONFIG_MSM_CAMERA_FLASH_LM3559
static void __init m3mpcs_init_i2c_camera(int bus_num)
{
	flash_i2c_device.id = bus_num;

	lge_init_gpio_i2c_pin(&flash_i2c_pdata, flash_i2c_pin[0],
		&i2c_camera_flash_devices[0]);
	i2c_register_board_info(bus_num, i2c_camera_flash_devices,
		ARRAY_SIZE(i2c_camera_flash_devices));
	platform_device_register(&flash_i2c_device);
}
#endif

void __init lge_add_camera_devices(void)
{
#ifdef CONFIG_MSM_CAMERA
	i2c_register_board_info(MSM_GSBI0_QUP_I2C_BUS_ID,
		i2c_camera_devices,
		ARRAY_SIZE(i2c_camera_devices));
#endif
	platform_add_devices(m3mpcs_camera_devices,
		ARRAY_SIZE(m3mpcs_camera_devices));
#ifdef CONFIG_MSM_CAMERA_FLASH_LM3559
	lge_add_gpio_i2c_device(m3mpcs_init_i2c_camera);
#endif
}
