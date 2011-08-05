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
#include <linux/mutex.h>
#include <mach/camera.h>
#include <mach/board_lge.h>

#include "devices-msm7x2xa.h"
#include "board-msm7x27a_hdk.h"

#ifdef CONFIG_SUBPMIC_LP8720
#include <../drivers/video/backlight/lp8720.h>
#endif

#ifdef CONFIG_MT9P017
#define CAM_MAIN_APTINA_I2C_SLAVE_ADDR  (0x6C >> 2)  /* Aptina(5M) */
#endif
#ifdef CONFIG_IMX072
#define CAM_MAIN_IMX072_I2C_SLAVE_ADDR  (0x34)       /* Sony (5M) */
#endif

#define CAM_MAIN_GPIO_RESET_N		(34)
#define CAM_VCM_GPIO_RESET_N		(35)


DEFINE_MUTEX(camera_power_mutex);

#ifdef CONFIG_SUBPMIC_LP8720
extern void subpm_gpio_output(int onoff);
extern void subpm_output_enable(void);
extern void subpm_set_output(subpm_output_enum outnum, int onoff);
extern void lp8720_write_reg(u8 reg, u8 data);
#endif

/* Flash */
#ifdef CONFIG_MSM_CAMERA_FLASH
static struct msm_camera_sensor_flash_src msm_flash_src = {
	.flash_sr_type = MSM_CAMERA_FLASH_SRC_CURRENT_DRIVER,
};

static struct msm_camera_sensor_flash_data led_flash_data = {
	.flash_type = MSM_CAMERA_FLASH_LED,
	.flash_src  = &msm_flash_src,
};
#else
static struct msm_camera_sensor_flash_data flash_none = {
	.flash_type = MSM_CAMERA_FLASH_NONE,
	.flash_src  = NULL,
};
#endif

/* Camera Sensor  ( Sony - IMX072 , Aptina - MT9P017) */
#ifdef CONFIG_MSM_CAMERA
static uint32_t camera_off_gpio_table[] = {
	GPIO_CFG(15, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),
	GPIO_CFG(35,  0, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),
};

static uint32_t camera_on_gpio_table[] = {
	GPIO_CFG(15, 1, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),
	GPIO_CFG(35,  1, GPIO_CFG_INPUT, GPIO_CFG_PULL_DOWN, GPIO_CFG_2MA),
};

static void config_gpio_table(uint32_t *table, int len)
{
	int n, rc;
	for (n = 0; n < len; n++) {
		rc = gpio_tlmm_config(table[n], GPIO_CFG_ENABLE);
		if (rc) {
			printk(KERN_ERR "%s: gpio_tlmm_config(%#x)=%d\n",
				__func__, table[n], rc);
			break;
		}
	}
}

int config_camera_on_gpios(void)
{
	config_gpio_table(camera_on_gpio_table,
		ARRAY_SIZE(camera_on_gpio_table));

	return 0;
}

void config_camera_off_gpios(void)
{
	config_gpio_table(camera_off_gpio_table,
		ARRAY_SIZE(camera_off_gpio_table));
}

void camera_power_mutex_lock(void)
{
	mutex_lock(&camera_power_mutex);
}

void camera_power_mutex_unlock(void)
{
	mutex_unlock(&camera_power_mutex);
}

#ifdef CONFIG_SUBPMIC_LP8720
static void subpmic_lp8720_off(void)
{
#ifdef CONFIG_IMX072
	subpm_set_output(SWREG, 0);
	subpm_set_output(LDO5, 0);
	subpm_set_output(LDO2, 0);
	subpm_set_output(LDO1, 0);
	mdelay(1);
	subpm_set_output(LDO3, 0);
	subpm_output_enable();
	subpm_gpio_output(0);
#endif

#ifdef CONFIG_MT9P017
	subpm_set_output(LDO5, 0);
	subpm_set_output(LDO2, 0);
	subpm_set_output(LDO1, 0);
	mdelay(1);
	subpm_output_enable();
	subpm_gpio_output(0);
#endif
}

void subpmic_lp8720_on(void)
{
#ifdef CONFIG_IMX072
	/* 2.8v - CAM_IOVDD_1.8V */
	lp8720_write_reg(LP8720_LDO1_SETTING, LP8720_STARTUP_DELAY_3TS | 0x0C);
	/* 2.8v - CAM_AVDD_2.8V */
	lp8720_write_reg(LP8720_LDO2_SETTING, LP8720_STARTUP_DELAY_3TS | 0x19);
	/* 1.8v - CAM_DVDD_1.8V */
	lp8720_write_reg(LP8720_LDO3_SETTING, LP8720_STARTUP_DELAY_3TS | 0x0C);
	/* 2.8v - CAM_DVDD_1.8V */
	lp8720_write_reg(LP8720_LDO5_SETTING, LP8720_STARTUP_DELAY_3TS | 0x19);
	/* BUCK 2 ==>1.8v */
	lp8720_write_reg(LP8720_BUCK_SETTING2, 0x00 | 0x09);

	mdelay(5);
	subpm_gpio_output(1);

	subpm_set_output(LDO3, 1);
	subpm_set_output(LDO1, 1);
	subpm_set_output(LDO2, 1);
	subpm_set_output(LDO5, 1);
	subpm_set_output(SWREG, 1);

	subpm_output_enable();
	mdelay(5);
#endif

#ifdef CONFIG_MT9P017
	/* 1.8v - CAM_IOVDD_1.8V */
	lp8720_write_reg(LP8720_LDO1_SETTING, LP8720_STARTUP_DELAY_3TS | 0x0C);
	/* 2.8v - CAM_AVDD_2.8V */
	lp8720_write_reg(LP8720_LDO2_SETTING, LP8720_STARTUP_DELAY_3TS | 0x19);
	/* 2.8v - CAM_DVDD_1.8V */
	lp8720_write_reg(LP8720_LDO5_SETTING, LP8720_STARTUP_DELAY_3TS | 0x19);

	mdelay(5);
	subpm_gpio_output(1);

	subpm_set_output(LDO1, 1);
	subpm_set_output(LDO2, 1);
	subpm_set_output(LDO5, 1);
	subpm_output_enable();
	mdelay(5);
#endif
}
#endif

int main_camera_power_off(void)
{
	camera_power_mutex_lock();

#ifdef CONFIG_SUBPMIC_LP8720
	subpmic_lp8720_off();
#endif

	camera_power_mutex_unlock();

	gpio_request(CAM_VCM_GPIO_RESET_N, "vcm_drive");
	gpio_direction_output(CAM_VCM_GPIO_RESET_N, 0);
	msleep(20);
	gpio_free(CAM_VCM_GPIO_RESET_N);

	return 0;
}

int main_camera_power_on(void)
{
	camera_power_mutex_lock();

	gpio_request(CAM_VCM_GPIO_RESET_N, "vcm_drive");
	gpio_direction_output(CAM_VCM_GPIO_RESET_N, 0);
	msleep(20);
	gpio_direction_output(CAM_VCM_GPIO_RESET_N, 1);
	msleep(20);

#ifdef CONFIG_SUBPMIC_LP8720
	subpmic_lp8720_on();
#endif

	camera_power_mutex_unlock();

	mdelay(1);
	msm_camio_clk_enable(CAMIO_CAM_MCLK_CLK);
	mdelay(1);

	gpio_set_value(CAM_MAIN_GPIO_RESET_N, 1);
	mdelay(1);

	return 0;
}

struct msm_camera_device_platform_data msm_main_camera_device_data = {
	.camera_gpio_on  = config_camera_on_gpios,
	.camera_gpio_off = config_camera_off_gpios,
	.ioext.csiphy = 0xA1000000,
	.ioext.csisz  = 0x00100000,
	.ioext.csiirq = INT_CSI_IRQ_1,
	.ioclk.mclk_clk_rate = 24000000,
	.ioclk.vfe_clk_rate  = 192000000,
	.ioext.appphy = MSM_CLK_CTL_PHYS,
	.ioext.appsz  = MSM_CLK_CTL_SIZE,
	.camera_power_on   = main_camera_power_on,
	.camera_power_off  = main_camera_power_off,
};

#ifdef CONFIG_IMX072
static struct msm_camera_sensor_platform_info imx072_sensor_7627a_info = {
	.mount_angle = 90
};

static struct msm_camera_sensor_info msm_camera_sensor_imx072_data = {
	.sensor_name          = "imx072",
	.sensor_reset_enable  = 1,
	.sensor_reset         = CAM_MAIN_GPIO_RESET_N,
	.sensor_pwd           = 0,
	.vcm_pwd              = 1,
	.vcm_enable           = 1,
	.pdata                = &msm_main_camera_device_data,
#ifdef CONFIG_MSM_CAMERA_FLASH
	.flash_data           = &led_flash_data,
#else
	.flash_data           = &flash_none,
#endif
	.sensor_platform_info = &imx072_sensor_7627a_info,
	.csi_if               = 1
};

static struct platform_device msm_camera_sensor_imx072 = {
	.name   = "msm_camera_imx072",
	.dev    = {
		.platform_data = &msm_camera_sensor_imx072_data,
	},
};
#endif

#ifdef CONFIG_MT9P017
static struct msm_camera_sensor_platform_info mt9p017_sensor_info = {
	.mount_angle = 90
};

static struct msm_camera_sensor_info msm_camera_sensor_mt9p017_data = {
	.sensor_name          = "mt9p017",
	.sensor_reset         = CAM_MAIN_GPIO_RESET_N,
	.sensor_pwd           = 0,
	.vcm_pwd              = 1,
	.vcm_enable           = 1,
	.pdata                = &msm_main_camera_device_data,
#ifdef CONFIG_MSM_CAMERA_FLASH
	.flash_data           = &led_flash_data,
#else
	.flash_data           = &flash_none,
#endif

	.csi_if               = 1,
	.sensor_platform_info = &mt9p017_sensor_info,
};

static struct platform_device msm_camera_sensor_mt9p017 = {
	.name      = "msm_camera_mt9p017",
	.dev       = {
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
	#ifdef CONFIG_IMX072
	{
		I2C_BOARD_INFO("imx072", 0x34),
	},
	#endif
};
#endif

static struct platform_device *hdk_camera_devices[] __initdata = {
#ifdef CONFIG_MT9P017
	&msm_camera_sensor_mt9p017,
#endif
#ifdef CONFIG_IMX072
	&msm_camera_sensor_imx072,
#endif
};

void __init lge_add_camera_devices(void)
{
#ifdef CONFIG_MSM_CAMERA
	i2c_register_board_info(MSM_GSBI0_QUP_I2C_BUS_ID,
		i2c_camera_devices,
		ARRAY_SIZE(i2c_camera_devices));
#endif
	platform_add_devices(hdk_camera_devices,
		ARRAY_SIZE(hdk_camera_devices));
}
