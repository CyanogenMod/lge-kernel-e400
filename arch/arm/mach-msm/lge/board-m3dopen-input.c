#include <linux/init.h>
#include <linux/platform_device.h>
#include <mach/rpc_server_handset.h>
#include <linux/i2c.h>
#include <linux/i2c-gpio.h>
#include <linux/gpio_event.h>
#include <mach/gpio.h>
#include <mach/vreg.h>
#include <mach/pmic.h>
#include <mach/board_lge.h>
#include <linux/regulator/consumer.h>

#include "devices-msm7x2xa.h"
#include "board-m3eu.h"

/* handset device */
static struct msm_handset_platform_data hs_platform_data = {
	.hs_name = "7k_handset",
	.pwr_key_delay_ms = 500, /* 0 will disable end key */
};

static struct platform_device hs_pdev = {
	.name   = "msm-handset",
	.id     = -1,
	.dev    = {
		.platform_data = &hs_platform_data,
	},
};
 /* GPIO key map for M3EU EVB */
static unsigned int keypad_row_gpios[] = {
	36, 37
};

static unsigned int keypad_col_gpios[] = {33};

#define KEYMAP_INDEX(col, row) ((col)*ARRAY_SIZE(keypad_row_gpios) + (row))

static const unsigned short keypad_keymap_m3eu[] = {
	[KEYMAP_INDEX(0, 0)] = KEY_VOLUMEUP,
	[KEYMAP_INDEX(0, 1)] = KEY_VOLUMEDOWN,
};

int m3eu_matrix_info_wrapper(struct gpio_event_input_devs *input_dev,
							 struct gpio_event_info *info, void **data, int func)
{
	int ret;
	if (func == GPIO_EVENT_FUNC_RESUME) {
		gpio_tlmm_config(
			GPIO_CFG(keypad_row_gpios[0], 0,
					 GPIO_CFG_INPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
		gpio_tlmm_config(
			GPIO_CFG(keypad_row_gpios[1], 0,
					 GPIO_CFG_INPUT, GPIO_CFG_PULL_UP, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	}
	ret = gpio_event_matrix_func(input_dev, info, data, func);
	return ret ;
}

static int m3eu_gpio_matrix_power(const struct gpio_event_platform_data *pdata, bool on)
{
	/* this is dummy function
	 * to make gpio_event driver register suspend function
	 * 2010-01-29, cleaneye.kim@lge.com
	 * copy from ALOHA code
	 * 2010-04-22 younchan.kim@lge.com
	 */

	return 0;
}

static struct gpio_event_matrix_info m3eu_keypad_matrix_info = {
	.info.func	= m3eu_matrix_info_wrapper,
	.keymap		= keypad_keymap_m3eu,
	.output_gpios	= keypad_col_gpios,
	.input_gpios	= keypad_row_gpios,
	.noutputs	= ARRAY_SIZE(keypad_col_gpios),
	.ninputs	= ARRAY_SIZE(keypad_row_gpios),
	.settle_time.tv.nsec = 40 * NSEC_PER_USEC,
	.poll_time.tv.nsec = 20 * NSEC_PER_MSEC,
	.flags		= GPIOKPF_LEVEL_TRIGGERED_IRQ | GPIOKPF_PRINT_UNMAPPED_KEYS | GPIOKPF_DRIVE_INACTIVE
};

static struct gpio_event_info *m3eu_keypad_info[] = {
	&m3eu_keypad_matrix_info.info
};

static struct gpio_event_platform_data m3eu_keypad_data = {
	.name		= "m3_keypad",
	.info		= m3eu_keypad_info,
	.info_count	= ARRAY_SIZE(m3eu_keypad_info),
	.power          = m3eu_gpio_matrix_power,
};

struct platform_device keypad_device_m3eu = {
	.name	= GPIO_EVENT_DEV_NAME,
	.id	= -1,
	.dev	= {
		.platform_data	= &m3eu_keypad_data,
	},
};

#ifdef CONFIG_HOLLIC
static struct gpio_event_direct_entry hazel_slide_switch_map[] = {
	{ GPIO_HALLIC_IRQ,       	SW_LID   	    },
};

static int hazel_gpio_slide_input_func(struct input_dev *input_dev,
			struct gpio_event_info *info, void **data, int func)
{
#if defined (CONFIG_LGE_SUPPORT_AT_CMD)  
	at_cmd_hall_ic = input_dev;
#endif /* add val for C710 AT_CMD */
	
	if (func == GPIO_EVENT_FUNC_INIT)
	{
		gpio_tlmm_config(GPIO_CFG(GPIO_HALLIC_IRQ, 0, GPIO_CFG_INPUT, GPIO_CFG_PULL_UP,
					GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	}

	return gpio_event_input_func(input_dev, info, data, func);
}

static struct gpio_event_input_info hazel_slide_switch_info = {
	.info.func = hazel_gpio_slide_input_func,
	.debounce_time.tv64 = 0,
	.flags = 0,
	.type = EV_SW,
	.keymap = hazel_slide_switch_map,
	.keymap_size = ARRAY_SIZE(hazel_slide_switch_map)
};

static struct gpio_event_info *hazel_gpio_slide_info[] = {
	&hazel_slide_switch_info.info,
};

static struct gpio_event_platform_data hazel_gpio_slide_data = {
	.name = "gpio-slide-detect",
	.info = hazel_gpio_slide_info,
	.info_count = ARRAY_SIZE(hazel_gpio_slide_info)
};

static struct platform_device hazel_gpio_slide_device = {
	.name = GPIO_EVENT_DEV_NAME,
	.id = 0,
	.dev        = {
		.platform_data  = &hazel_gpio_slide_data,
	},
};
#endif

#ifdef CONFIG_KEYBOARD_PP2106
/* pp2106 qwerty keyboard device */
#define PP2106_KEYPAD_ROW 7
#define PP2106_KEYPAD_COL 7

static unsigned short pp2106_keycode[PP2106_KEYPAD_ROW][PP2106_KEYPAD_COL] = {
	{KEY_I, KEY_K, KEY_SPACE, KEY_BACK, KEY_D, KEY_S, KEY_LEFTSHIFT },
	{KEY_SEARCH, KEY_COMPOSE, KEY_SPACE, KEY_R, KEY_E, KEY_W, KEY_A },
	{KEY_ENTER, KEY_SLASH, KEY_HANGEUL, KEY_T, KEY_MENU, KEY_Z, KEY_Q },
	{KEY_BACKSPACE, KEY_L, KEY_DOT, KEY_G, KEY_X, KEY_HOME, KEY_N },
	{KEY_P, KEY_O, KEY_M, KEY_F, KEY_C, KEY_LEFTALT, KEY_J },
	{KEY_V, KEY_B, KEY_U, KEY_Y, KEY_H, KEY_UNKNOWN, KEY_UNKNOWN },
	{KEY_UNKNOWN, KEY_UNKNOWN, KEY_UNKNOWN, KEY_UNKNOWN, KEY_UNKNOWN, KEY_UNKNOWN, KEY_UNKNOWN },
};
static int pp2106_vreg_set(unsigned char onoff)
{
	/*
	int rc = 0;
	struct vreg *vreg_l12 = NULL;

	vreg_l12 = vreg_get(NULL, "gp2");
	if (IS_ERR(vreg_l12)) {
		pr_err("%s: vreg_get failed (%ld)\n", __func__, PTR_ERR(vreg_l12));
		return PTR_ERR(vreg_l12);
	}

	if (onoff) {
		rc = vreg_set_level(vreg_l12, 2850);
		if (rc < 0) {
			pr_err("%s: vreg_set_level failed (%d)\n", __func__, rc);
			goto vreg_fail;
		}
		rc = vreg_enable(vreg_l12);
		if (rc < 0) {
			pr_err("%s: vreg_enable failed (%d)\n", __func__, rc);
			goto vreg_fail;
		}
	} else {
		rc = vreg_disable(vreg_l12);
		if (rc < 0) {
			pr_err("%s: vreg_disable failed (%d)\n", __func__, rc);
			goto vreg_fail;
		}
	}
	
	return rc;

vreg_fail:
	vreg_put(vreg_l12);
	return rc;
	*/
	return 0;
}

static struct pp2106_platform_data pp2106_pdata = {
	.keypad_row = PP2106_KEYPAD_ROW,
	.keypad_col = PP2106_KEYPAD_COL,
	.keycode = (unsigned char *)pp2106_keycode,
	.reset_pin = 4,
	.irq_pin = 114,
	.sda_pin = 115,
	.scl_pin = 116,
	.power = pp2106_vreg_set,
};

static struct platform_device hdk_qwerty_device = {
	.name = "pp2106-keypad",
	.id = 0,
	.dev = {
		.platform_data = &pp2106_pdata,
	},
};
#endif /* CONFIG_KEYBOARD_PP2106 */

/* input platform device */
static struct platform_device *m3eu_input_devices[] __initdata = {
	&hs_pdev,
};

static struct platform_device *m3eu_gpio_input_devices[] __initdata = {
	&keypad_device_m3eu,/* the gpio keypad for m3eu EVB */
#ifdef CONFIG_KEYBOARD_PP2106
	&hdk_qwerty_device,
#endif
#ifdef CONFIG_HOLLIC
	&hazel_gpio_slide_device,
#endif	
};

/* Melfas MCS8000 Touch (mms-128)*/
#if defined(CONFIG_TOUCHSCREEN_MELFAS_TS)
static struct gpio_i2c_pin ts_i2c_pin[] = {
	[0] = {
		.sda_pin	= TS_GPIO_I2C_SDA,
		.scl_pin	= TS_GPIO_I2C_SCL,
		.reset_pin	= 0,
		.irq_pin	= TS_GPIO_IRQ,
	},
};

static struct i2c_gpio_platform_data ts_i2c_pdata = {
	.sda_is_open_drain	= 0,
	.scl_is_open_drain	= 0,
	.udelay			= 1,
};

static struct platform_device ts_i2c_device = {
	.name	= "i2c-gpio",
	.dev.platform_data = &ts_i2c_pdata,
};

int ts_set_vreg(unsigned char onoff)
{
	static struct regulator *ldo1 = NULL;
	int rc;
	static int init = 0;
	ldo1 = regulator_get(NULL, "RT8053_LDO1");
	if (ldo1 == NULL)
		pr_err(
			"%s: regulator_get(ldo1) failed\n",
			__func__);

	if (onoff) {
		rc = regulator_set_voltage(ldo1, 3000000, 3000000);
		if (rc < 0)
			pr_err(
				"%s: regulator_set_voltage(ldo1) failed\n",
				__func__);

		rc = regulator_enable(ldo1);
		if (rc < 0)
			pr_err(
				"%s: regulator_enable(ldo1) failed\n",
				__func__);

		init = 1;
	} else {
		if (init > 0) {
			rc = regulator_disable(ldo1);
			if (rc < 0)
				pr_err(
					"%s: regulator_disble(ldo1) failed\n",
					__func__);

			regulator_put(ldo1);
		}
	}

	return 0;
}

static struct touch_platform_data ts_pdata = {
	.ts_x_min = TS_X_MIN,
	.ts_x_max = TS_X_MAX,
	.ts_y_min = TS_Y_MIN,
	.ts_y_max = TS_Y_MAX,
	.power 	  = ts_set_vreg,
	.irq 	  = TS_GPIO_IRQ,
	.scl      = TS_GPIO_I2C_SCL,
	.sda      = TS_GPIO_I2C_SDA,
};

static struct i2c_board_info ts_i2c_bdinfo[] = {
	[0] = {
		I2C_BOARD_INFO("touch_mcs8000", TS_I2C_SLAVE_ADDR),
		.type = "touch_mcs8000",
		.platform_data = &ts_pdata,
	},
};

/* this routine should be checked for nessarry */
static int init_gpio_i2c_pin_touch(
	struct i2c_gpio_platform_data *i2c_adap_pdata,
	struct gpio_i2c_pin gpio_i2c_pin,
	struct i2c_board_info *i2c_board_info_data)
{
	i2c_adap_pdata->sda_pin = gpio_i2c_pin.sda_pin;
	i2c_adap_pdata->scl_pin = gpio_i2c_pin.scl_pin;

	gpio_tlmm_config(
		GPIO_CFG(gpio_i2c_pin.sda_pin, 0, GPIO_CFG_OUTPUT,
				GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	gpio_tlmm_config(
		GPIO_CFG(gpio_i2c_pin.scl_pin, 0, GPIO_CFG_OUTPUT,
				GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	gpio_set_value(gpio_i2c_pin.sda_pin, 1);
	gpio_set_value(gpio_i2c_pin.scl_pin, 1);

	if (gpio_i2c_pin.reset_pin) {
		gpio_tlmm_config(
			GPIO_CFG(gpio_i2c_pin.reset_pin, 0, GPIO_CFG_OUTPUT,
					GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
		gpio_set_value(gpio_i2c_pin.reset_pin, 1);
	}

	if (gpio_i2c_pin.irq_pin) {
		gpio_tlmm_config(
			GPIO_CFG(gpio_i2c_pin.irq_pin, 0, GPIO_CFG_INPUT,
					GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
		i2c_board_info_data->irq =
			MSM_GPIO_TO_INT(gpio_i2c_pin.irq_pin);
	}

	return 0;
}

static void __init m3eu_init_i2c_touch(int bus_num)
{
	ts_i2c_device.id = bus_num;

	init_gpio_i2c_pin_touch(
		&ts_i2c_pdata, ts_i2c_pin[0], &ts_i2c_bdinfo[0]);
	i2c_register_board_info(
		bus_num, &ts_i2c_bdinfo[0], 1);
	platform_device_register(&ts_i2c_device);
}
#endif /* CONFIG_TOUCH_MCS8000 */

/* Atmel Touch for M3EU EVB */
#if defined(CONFIG_TOUCHSCREEN_MXT140)

static struct gpio_i2c_pin ts_i2c_pin = {
	.sda_pin = TS_GPIO_I2C_SDA,
	.scl_pin = TS_GPIO_I2C_SCL,
};

static struct i2c_gpio_platform_data ts_i2c_pdata = {
	.sda_is_open_drain	= 0,
	.scl_is_open_drain	= 0,
	.udelay			= 1,
};

static struct platform_device ts_i2c_device = {
	.name	= "i2c-gpio",
	.dev.platform_data = &ts_i2c_pdata,
};

static int ts_set_vreg(unsigned char onoff)
{
	struct vreg *vreg_touch;
	int rc;

	printk(KERN_INFO "[Touch] %s() onoff:%d\n",
		   __func__, onoff);

	vreg_touch = vreg_get(0, "bt");

	if (IS_ERR(vreg_touch)) {
		printk(KERN_INFO "[Touch] vreg_get fail : touch\n");
		return -EBUSY;
	}

	if (onoff) {
		rc = vreg_set_level(vreg_touch, 3000);
		if (rc != 0) {
			printk(KERN_INFO "[Touch] vreg_set_level failed\n");
			return -EBUSY;
		}
		vreg_enable(vreg_touch);
	} else {
		vreg_disable(vreg_touch);
	}

	return 0;
}

static struct touch_platform_data ts_pdata = {
	.ts_x_min   = TS_X_MIN,
	.ts_x_max   = TS_X_MAX,
	.ts_y_min   = TS_Y_MIN,
	.ts_y_max   = TS_Y_MAX,
	.ts_y_start = 0,
	.ts_y_scrn_max = 480,
	.power      = ts_set_vreg,
	.gpio_int   = TS_GPIO_IRQ,
	.irq 	  = MSM_GPIO_TO_INT(TS_GPIO_IRQ),
	.scl      = TS_GPIO_I2C_SCL,
	.sda      = TS_GPIO_I2C_SDA,
	.hw_i2c     = 0,
};

static struct i2c_board_info ts_i2c_bdinfo[] = {
	[0] = {
		I2C_BOARD_INFO("qt602240_ts", 0x4A),
		.type = "qt602240_ts",
		.platform_data = &ts_pdata,
	},
};

static void __init m3eu_init_i2c_touch(int bus_num)
{
	ts_i2c_device.id = bus_num;
	/* workaround for HDK rev_a no pullup */
	lge_init_gpio_i2c_pin(
		&ts_i2c_pdata, ts_i2c_pin, &ts_i2c_bdinfo[0]);
	i2c_register_board_info(
		bus_num, &ts_i2c_bdinfo[0], 1);
	platform_device_register(&ts_i2c_device);
}
#endif /* CONFIG_TOUCH_mxt_140 */

/** accelerometer **/
int accel_power(unsigned char onoff)
{
	int ret = 0;
	struct vreg *rfrx1_vreg = vreg_get(0, "rfrx1");

	if (onoff) {
		printk(KERN_INFO "accel_power_on\n");

		ret = vreg_set_level(rfrx1_vreg, 3000);
		if (ret != 0) {
			printk(KERN_INFO "[Accel] vreg_set_level failed\n");
			return ret;
		}
		vreg_enable(rfrx1_vreg);
	} else {
		printk(KERN_INFO "accel_power_off\n");
		vreg_disable(rfrx1_vreg);
	}

	return ret;
}
EXPORT_SYMBOL(accel_power);
struct acceleration_platform_data bma222 = {
	.power = accel_power,
};

static struct gpio_i2c_pin accel_i2c_pin[] = {
	[0] = {
		.sda_pin	= ACCEL_GPIO_I2C_SDA,
		.scl_pin	= ACCEL_GPIO_I2C_SCL,
		.reset_pin	= 0,
		.irq_pin	= ACCEL_GPIO_INT,
	},
};

static struct i2c_gpio_platform_data accel_i2c_pdata = {
	.sda_is_open_drain = 0,
	.scl_is_open_drain = 0,
	.udelay = 2,
};

static struct platform_device accel_i2c_device = {
	.name = "i2c-gpio",
	.dev.platform_data = &accel_i2c_pdata,
};

static struct i2c_board_info accel_i2c_bdinfo[] = {
	[0] = {
		I2C_BOARD_INFO("bma222", ACCEL_I2C_ADDRESS),
		.type = "bma222",
		.platform_data = &bma222,
	},
};

static void __init m3eu_init_i2c_acceleration(int bus_num)
{
	accel_i2c_device.id = bus_num;

	lge_init_gpio_i2c_pin(
		&accel_i2c_pdata, accel_i2c_pin[0], &accel_i2c_bdinfo[0]);

	i2c_register_board_info(
		bus_num, &accel_i2c_bdinfo[0], 1);

	platform_device_register(&accel_i2c_device);
}

/* ecompass */
static int ecom_power_set(unsigned char onoff)
{
	int ret = 0;
	struct vreg *rfrx1_vreg = vreg_get(0, "rfrx1");

	if (onoff) {
		printk(KERN_INFO "ecom_power_on\n");
		vreg_set_level(rfrx1_vreg, 3000);
		vreg_enable(rfrx1_vreg);
	} else {
		printk(KERN_INFO "ecom_power_off\n");
		vreg_disable(rfrx1_vreg);
	}

	return ret;
}
#if defined(CONFIG_SENSOR_AK8975)
static struct ecom_platform_data ecom_pdata = {
	.pin_int = ECOM_GPIO_INT,
	.pin_rst = 0,
	.power = ecom_power_set,
	.accelerator_name = "bma222",
/* LGE_CHANGE,
 * add accel tuning data for H/W accerleration sensor direction,
 * based on [hyesung.shin@lge.com] for <Sensor driver structure>
 *
 * 2011-07-05
 */
	.fdata_sign_x = -1,
    .fdata_sign_y = 1,
    .fdata_sign_z = -1,
    .fdata_order0 = 0,
    .fdata_order1 = 1,
    .fdata_order2 = 2,
    .sensitivity1g = 64,

};
#else
static struct ecom_platform_data ecom_pdata = {
	.pin_int = ECOM_GPIO_INT,
	.pin_rst = 0,
	.power = ecom_power_set,
	.accelerator_name = "bma222",

};
#endif
#if defined(CONFIG_SENSOR_HSCDTD004A)
static struct i2c_board_info ecom_i2c_bdinfo[] = {
	[0] = {
		I2C_BOARD_INFO("hscd_i2c", ECOM_I2C_ADDRESS),
		.type = "hscd_i2c",
		.platform_data = &ecom_pdata,
	},
};
#endif
#if defined(CONFIG_SENSOR_AK8975)
static struct i2c_board_info ecom_i2c_bdinfo[] = {
	[0] = {
		I2C_BOARD_INFO("akm8975", ECOM_I2C_ADDRESS),
		.type = "akm8975",
		.platform_data = &ecom_pdata,
	},
};
#endif

static struct gpio_i2c_pin ecom_i2c_pin[] = {
	[0] = {
		.sda_pin	= ECOM_GPIO_I2C_SDA,
		.scl_pin	= ECOM_GPIO_I2C_SCL,
		.reset_pin	= 0,
		.irq_pin	= ECOM_GPIO_INT,
	},
};

static struct i2c_gpio_platform_data ecom_i2c_pdata = {
	.sda_is_open_drain = 0,
	.scl_is_open_drain = 0,
	.udelay = 2,
};

static struct platform_device ecom_i2c_device = {
	.name = "i2c-gpio",
	.dev.platform_data = &ecom_i2c_pdata,
};


static void __init m3eu_init_i2c_ecom(int bus_num)
{
	ecom_i2c_device.id = bus_num;

	lge_init_gpio_i2c_pin(&ecom_i2c_pdata, ecom_i2c_pin[0], &ecom_i2c_bdinfo[0]);

	i2c_register_board_info(bus_num, &ecom_i2c_bdinfo[0], 1);
	platform_device_register(&ecom_i2c_device);
}

/* proximity */
static int prox_power_set(unsigned char onoff)
{
/* just return 0, later I'll fix it */
	static bool init_done = 0;
	
	int ret = 0;
/* need to be fixed  - for vreg using SUB PMIC */

	struct regulator *ldo5 = NULL;

	ldo5 = regulator_get(NULL, "RT8053_LDO5");
	if (ldo5 == NULL)
		pr_err(
			"%s: regulator_get(ldo5) failed\n",
			__func__);

	printk(KERN_INFO "[Proximity] %s() : Power %s\n",
		   __func__, onoff ? "On" : "Off");

	if (init_done == 0 && onoff) {
		if (onoff) {
			printk(KERN_INFO "LDO5 vreg set.\n");
			ret = regulator_set_voltage(ldo5, 2800000, 2800000);
			if (ret < 0)
				pr_err(
					"%s: regulator_set_voltage(ldo5) failed\n",
					__func__);

			ret = regulator_enable(ldo5);
			if (ret < 0)
				pr_err(
					"%s: regulator_enable(ldo5) failed\n",
					__func__);

			init_done = 1;
		} else {
			ret = regulator_disable(ldo5);
			if (ret < 0)
				pr_err(
					"%s: regulator_disable(ldo5) failed\n",
					__func__);

		}
	}
	return ret;

/*	struct vreg *temp_vreg = vreg_get(0, "");

	printk(
	"[Proximity] %s() : Power %s\n",__FUNCTION__, onoff ? "On" : "Off");

	if (init_done == 0 && onoff) {
		if (onoff) {
			vreg_set_level(temp_vreg, 2800);
			vreg_enable(temp_vreg);

			init_done = 1;
		} else {
			vreg_disable(temp_vreg);
		}
	}
	return ret;
*/
	return ret;
}

static struct proximity_platform_data proxi_pdata = {
	.irq_num	= PROXI_GPIO_DOUT,
	.power		= prox_power_set,
	.methods		= 0,
	.operation_mode		= 0,
	.debounce	 = 0,
	.cycle = 2,
};

static struct i2c_board_info prox_i2c_bdinfo[] = {
	[0] = {
		I2C_BOARD_INFO("proximity_gp2ap", PROXI_I2C_ADDRESS),
		.type = "proximity_gp2ap",
		.platform_data = &proxi_pdata,
	},
};

static struct gpio_i2c_pin proxi_i2c_pin[] = {
	[0] = {
		.sda_pin	= PROXI_GPIO_I2C_SDA,
		.scl_pin	= PROXI_GPIO_I2C_SCL,
		.reset_pin	= 0,
		.irq_pin	= PROXI_GPIO_DOUT,
	},
};

static struct i2c_gpio_platform_data proxi_i2c_pdata = {
	.sda_is_open_drain = 0,
	.scl_is_open_drain = 0,
	.udelay = 2,
};

static struct platform_device proxi_i2c_device = {
	.name = "i2c-gpio",
	.dev.platform_data = &proxi_i2c_pdata,
};

static void __init m3eu_init_i2c_prox(int bus_num)
{
	proxi_i2c_device.id = bus_num;

	lge_init_gpio_i2c_pin(&proxi_i2c_pdata, proxi_i2c_pin[0], &prox_i2c_bdinfo[0]);

	i2c_register_board_info(bus_num, &prox_i2c_bdinfo[0], 1);
	platform_device_register(&proxi_i2c_device);
}

/* common function */
void __init lge_add_input_devices(void)
{
	platform_add_devices(
		m3eu_input_devices, ARRAY_SIZE(m3eu_input_devices));
	platform_add_devices(
		m3eu_gpio_input_devices, ARRAY_SIZE(m3eu_gpio_input_devices));

	lge_add_gpio_i2c_device(m3eu_init_i2c_touch);
	lge_add_gpio_i2c_device(m3eu_init_i2c_acceleration);
	lge_add_gpio_i2c_device(m3eu_init_i2c_ecom);
	lge_add_gpio_i2c_device(m3eu_init_i2c_prox);
}
