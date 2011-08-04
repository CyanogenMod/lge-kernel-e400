#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/i2c.h>
#include <linux/gpio.h>
#include <linux/regulator/rt8053.h>
#include <linux/regulator/consumer.h>

#include <mach/board_lge.h>
#include "devices-msm7x2xa.h"

/* Sub-PMIC */
#if defined(CONFIG_REGULATOR_RT8053)
static struct regulator_consumer_supply rt8053_vreg_supply[] = {
	REGULATOR_SUPPLY("RT8053_LDO1", NULL),
	REGULATOR_SUPPLY("RT8053_LDO2", NULL),
	REGULATOR_SUPPLY("RT8053_LDO3", NULL),
	REGULATOR_SUPPLY("RT8053_LDO4", NULL),
	REGULATOR_SUPPLY("RT8053_LDO5", NULL),
	REGULATOR_SUPPLY("RT8053_DCDC1", NULL),
};

#define RT8053_VREG_INIT(_id, _min_uV, _max_uV) \
	{ \
		.id = RT8053_##_id, \
		.initdata = { \
			.constraints = { \
				.valid_modes_mask = REGULATOR_MODE_NORMAL, \
				.valid_ops_mask = REGULATOR_CHANGE_VOLTAGE | \
					REGULATOR_CHANGE_STATUS, \
				.min_uV = _min_uV, \
				.max_uV = _max_uV, \
			}, \
			.num_consumer_supplies = 1, \
			.consumer_supplies = \
				&rt8053_vreg_supply[RT8053_##_id], \
		}, \
	}

static struct rt8053_regulator_subdev rt8053_regulators[] = {
	RT8053_VREG_INIT(LDO1, 1200000, 3300000),
	RT8053_VREG_INIT(LDO2, 1200000, 3300000),
	RT8053_VREG_INIT(LDO3, 1200000, 3300000),
	RT8053_VREG_INIT(LDO4,  800000, 2850000),
	RT8053_VREG_INIT(LDO5, 1200000, 3300000),
	RT8053_VREG_INIT(DCDC1, 800000, 2300000),
};

static struct rt8053_platform_data rt8053_data = {
	.num_regulators = 6,
	.enable_gpio = 58,
	.regulators = rt8053_regulators,
};

#elif defined(CONFIG_SUBPMIC_LP8720)
#include <../drivers/video/backlight/lp8720.h>
#define CAM_SUBPMIC_RESET_N		(76)

static struct lp8720_platform_data lp8720ldo_data = {
	.en_gpio_num = CAM_SUBPMIC_RESET_N,
};

#endif

static struct i2c_board_info subpm_i2c_bdinfo[] = {
#if defined(CONFIG_REGULATOR_RT8053)
	{
		I2C_BOARD_INFO("rt8053", 0x7D),
		.platform_data = &rt8053_data,
	},
#elif defined(CONFIG_SUBPMIC_LP8720)
	{
		I2C_BOARD_INFO("lp8720", 0x7D),
		.platform_data = &lp8720ldo_data,
	},
#endif
};

/* main interface */
void __init lge_add_pm_devices(void)
{
	i2c_register_board_info(MSM_GSBI0_QUP_I2C_BUS_ID,
			subpm_i2c_bdinfo,
			ARRAY_SIZE(subpm_i2c_bdinfo));
}
