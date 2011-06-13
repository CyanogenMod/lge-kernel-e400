#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/power_supply.h>
#include <linux/gpio.h>
#include <mach/msm_battery.h>
#include <mach/board_lge.h>

#include "devices-msm7x2xa.h"

static u32 msm_calculate_batt_capacity(u32 current_voltage);

static struct msm_psy_batt_pdata msm_psy_batt_data = {
	.voltage_min_design     = 2800,
	.voltage_max_design     = 4300,
	.avail_chg_sources      = AC_CHG | USB_CHG ,
	.batt_technology        = POWER_SUPPLY_TECHNOLOGY_LION,
	.calculate_capacity     = &msm_calculate_batt_capacity,
};

static u32 msm_calculate_batt_capacity(u32 current_voltage)
{
	u32 low_voltage	 = msm_psy_batt_data.voltage_min_design;
	u32 high_voltage = msm_psy_batt_data.voltage_max_design;

	return (current_voltage - low_voltage) * 100
			/ (high_voltage - low_voltage);
}

static struct platform_device msm_batt_device = {
	.name               = "msm-battery",
	.id                 = -1,
	.dev.platform_data  = &msm_psy_batt_data,
};

#if 0 //FIXME
/* LED platform data */
static struct platform_device msm_device_pmic_leds = {
	.name = "pmic-leds",
	.id = -1,
};
#endif

/* msm7x2xa_hdk Board Vibrator Functions for Android Vibrator Driver */
#define VIBE_IC_VOLTAGE				3000
#define PMIC_LIN_MOTOR_PWM_GPIO		1
#define MOTOR_LDO_EN_GPIO			96
#define GPIO_EAR_SENSE		41
#define GPIO_BUTTON_DETECT  128
//#define GPIO_HS_MIC_BIAS_EN	26
int hdk_vibrator_power_set(int onoff)
{
	int ret = 0;
	
	ret = gpio_request(MOTOR_LDO_EN_GPIO, "MOTOR_LDO_EN_GPIO");
	if (ret) {
		pr_err("Requesting MOTOR_LDO_EN_GPIO GPIO failed!\n");
		goto err;
	}
	
	gpio_tlmm_config(GPIO_CFG(MOTOR_LDO_EN_GPIO, 0, GPIO_CFG_OUTPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	ret = gpio_direction_output(MOTOR_LDO_EN_GPIO, onoff);
	if (ret) {
		pr_err("Setting MOTOR_LDO_EN_GPIO GPIO direction failed!\n");
		goto err2;
	}

err2:
	gpio_free(MOTOR_LDO_EN_GPIO);
err:
	return 0;
}

int hdk_vibrator_pwm_set(int enable, int amp)
{
	/* TODO : murali.ramaiah@lge.com - to be implement */
	return 0;
}

int hdk_vibrator_ic_enable_set(int enable)
{
	/* msm7x27_hdk Board does not use Motor IC Enable GPIO */
	return 0;
}

static struct android_vibrator_platform_data hdk_vibrator_data = {
	.enable_status = 0,
	.power_set = hdk_vibrator_power_set,
	.pwm_set = hdk_vibrator_pwm_set,
	.ic_enable_set = hdk_vibrator_ic_enable_set,
	.amp_value = 110,
};

static struct platform_device android_vibrator_device = {
	.name   = "android-vibrator",
	.id = -1,
	.dev = {
		.platform_data = &hdk_vibrator_data,
	},
};

/* misc platform devices */
static struct platform_device *hdk_misc_devices[] __initdata = {
	&msm_batt_device,
	&android_vibrator_device,	
//	&msm_device_pmic_leds,  //FIXME
};

/* main interface */
void __init lge_add_misc_devices(void)
{
	platform_add_devices(hdk_misc_devices, ARRAY_SIZE(hdk_misc_devices));
}

