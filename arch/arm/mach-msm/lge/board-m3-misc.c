#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/power_supply.h>

#include <mach/msm_battery.h>

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

/* LED platform data */
static struct platform_device msm_device_pmic_leds = {
	.name = "pmic-leds",
	.id = -1,
};

/* misc platform devices */
static struct platform_device *m3_misc_devices[] __initdata = {
	&msm_batt_device,
};

/* main interface */
void __init lge_add_misc_devices(void)
{
	platform_add_devices(m3_misc_devices, ARRAY_SIZE(m3_misc_devices));
	platform_device_register(&msm_device_pmic_leds);
}

