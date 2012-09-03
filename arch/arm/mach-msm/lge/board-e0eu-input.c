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
#include "board-e0eu.h"

/* handset device */
static struct msm_handset_platform_data hs_platform_data = {
	.hs_name = "7k_handset",
	.pwr_key_delay_ms = 10, /* 0 will disable end key */
};

static struct platform_device hs_pdev = {
	.name   = "msm-handset",
	.id     = -1,
	.dev    = {
		.platform_data = &hs_platform_data,
	},
};



// SIM Change Key
#if defined (CONFIG_MACH_MSM7X25A_E1BR)
/* GPIO key map for E1 EVB */
static unsigned int keypad_row_gpios[] = {36, 37, 38};
static unsigned int keypad_col_gpios[] = {32, 33};

#define KEYMAP_INDEX(col, row) ((col)*ARRAY_SIZE(keypad_row_gpios) + (row))

static const unsigned short keypad_keymap_e0eu[] = {
		[KEYMAP_INDEX(1, 1)] = KEY_VOLUMEUP,
		[KEYMAP_INDEX(1, 0)] = KEY_VOLUMEDOWN,
		[KEYMAP_INDEX(0, 2)] = KEY_HOME,
		[KEYMAP_INDEX(0, 0)] = KEY_SIM_SWITCH,		
};
#else
 /* GPIO key map for M3EU EVB */
 /* LGE_CHANGE_S: E0 wonsang.yoon@lge.com [2011-10-17] : for Rev.B Key MAPl */
static unsigned int keypad_row_gpios[] = {
		36, 37, 38
};
	static unsigned int keypad_col_gpios[] = {32, 33};
/* LGE_CHANGE_N: E0 wonsang.yoon@lge.com [2011-10-17] : for Rev.B Key MAPl */

#define KEYMAP_INDEX(col, row) ((col)*ARRAY_SIZE(keypad_row_gpios) + (row))

/* LGE_CHANGE_S: E0 wonsang.yoon@lge.com [2011-10-17] : for Rev.B Key MAPl */
static const unsigned short keypad_keymap_e0eu[] = {
		[KEYMAP_INDEX(1, 1)] = KEY_VOLUMEUP,
		[KEYMAP_INDEX(1, 0)] = KEY_VOLUMEDOWN,
		[KEYMAP_INDEX(0, 2)] = KEY_HOME,
};
/* LGE_CHANGE_N: E0 wonsang.yoon@lge.com [2011-10-17] : for Rev.B Key MAPl */
#endif
// END , SIM key switch 

int e0eu_matrix_info_wrapper(struct gpio_event_input_devs *input_dev,
							 struct gpio_event_info *info, void **data, int func)
{
        int ret ;
		if (func == GPIO_EVENT_FUNC_RESUME) {
		gpio_tlmm_config(
			GPIO_CFG(keypad_row_gpios[0], 0,
						GPIO_CFG_INPUT, GPIO_CFG_PULL_UP,GPIO_CFG_2MA), GPIO_CFG_ENABLE);
		gpio_tlmm_config(
			GPIO_CFG(keypad_row_gpios[1], 0,
						GPIO_CFG_INPUT, GPIO_CFG_PULL_UP,GPIO_CFG_2MA), GPIO_CFG_ENABLE);

		/* LGE_CHANGE_S: E0 wonsang.yoon@lge.com [2011-10-17] : for Rev.B Key MAPl */
				gpio_tlmm_config(
					GPIO_CFG(keypad_row_gpios[2], 0,
							GPIO_CFG_INPUT, GPIO_CFG_PULL_UP,GPIO_CFG_2MA), GPIO_CFG_ENABLE);
		}
		/* LGE_CHANGE_N: E0 wonsang.yoon@lge.com [2011-10-17] : for Rev.B Key MAPl */

		ret = gpio_event_matrix_func(input_dev,info, data,func);
        return ret ;
}

static int e0eu_gpio_matrix_power(const struct gpio_event_platform_data *pdata, bool on)
{
	/* this is dummy function
	 * to make gpio_event driver register suspend function
	 * 2010-01-29, cleaneye.kim@lge.com
	 * copy from ALOHA code
	 * 2010-04-22 younchan.kim@lge.com
	 */

	return 0;
}

static struct gpio_event_matrix_info e0eu_keypad_matrix_info = {
	.info.func	= e0eu_matrix_info_wrapper,
	.keymap		= keypad_keymap_e0eu,
	.output_gpios	= keypad_col_gpios,
	.input_gpios	= keypad_row_gpios,
	.noutputs	= ARRAY_SIZE(keypad_col_gpios),
	.ninputs	= ARRAY_SIZE(keypad_row_gpios),
	.settle_time.tv.nsec = 40 * NSEC_PER_USEC,
	.poll_time.tv.nsec = 20 * NSEC_PER_MSEC,
	.flags		= GPIOKPF_LEVEL_TRIGGERED_IRQ | GPIOKPF_PRINT_UNMAPPED_KEYS | GPIOKPF_DRIVE_INACTIVE
};

static struct gpio_event_info *e0eu_keypad_info[] = {
	&e0eu_keypad_matrix_info.info
};

static struct gpio_event_platform_data e0eu_keypad_data = {
	.name		= "e0_keypad",
	.info		= e0eu_keypad_info,
	.info_count	= ARRAY_SIZE(e0eu_keypad_info),
	.power          = e0eu_gpio_matrix_power,
};

struct platform_device keypad_device_e0eu = {
	.name	= GPIO_EVENT_DEV_NAME,
	.id	= -1,
	.dev	= {
		.platform_data	= &e0eu_keypad_data,
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
static struct platform_device *e0eu_input_devices[] __initdata = {
	&hs_pdev,
};

static struct platform_device *e0eu_gpio_input_devices[] __initdata = {
	&keypad_device_e0eu,/* the gpio keypad for m3eu EVB */
#ifdef CONFIG_KEYBOARD_PP2106
	&hdk_qwerty_device,
#endif
#ifdef CONFIG_HOLLIC
	&hazel_gpio_slide_device,
#endif
};

/* Melfas MCS8000 Touch (mms-128)*/
#if defined(CONFIG_TOUCHSCREEN_MCS8000_MMS128)
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

/*static*/ int ts_set_vreg(unsigned char onoff) //for touch download
{

	int rc = 0;
	struct vreg *vreg_l1 = NULL;

	vreg_l1 = vreg_get(NULL, "rfrx1");
	if (IS_ERR(vreg_l1)) {
		pr_err("%s: vreg_get failed (%ld)\n", __func__, PTR_ERR(vreg_l1));
		return PTR_ERR(vreg_l1);
	}

	printk(KERN_INFO "ts_set_veg : %d\n", onoff);
	if(onoff){

		/* LGE_CHANGE_S: E0 kevinzone.han@lge.com [2011-12-19] 
		: Changed the touchscreen operating power 3V into 3.05V*/
		rc = vreg_set_level(vreg_l1, 3050);
		/* LGE_CHANGE_E: E0 kevinzone.han@lge.com [2011-12-19] 
		: Changed the touchscreen operating power 3V into 3.05V*/

		if (rc < 0) {
			pr_err("%s: vreg_set_level failed (%d)\n", __func__, rc);
			goto vreg_touch_fail;
		}
		rc = vreg_enable(vreg_l1);
		if (rc < 0) {
			pr_err("%s: vreg_enable failed (%d)\n", __func__, rc);
			goto vreg_touch_fail;
		}
	} else {
		rc = vreg_disable(vreg_l1);
			if (rc < 0) {
			pr_err("%s: vreg_disable failed (%d)\n", __func__, rc);
			goto vreg_touch_fail;
		}
	}

	return rc;

vreg_touch_fail:
	vreg_put(vreg_l1);
	return rc;
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

	//gpio_tlmm_config(GPIO_CFG(gpio_i2c_pin.sda_pin, 0, GPIO_CFG_OUTPUT,
		//		GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	//gpio_tlmm_config(GPIO_CFG(gpio_i2c_pin.scl_pin, 0, GPIO_CFG_OUTPUT,
	//			GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	gpio_tlmm_config(GPIO_CFG(gpio_i2c_pin.sda_pin, 0, GPIO_CFG_OUTPUT,
				GPIO_CFG_PULL_UP, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	gpio_tlmm_config(GPIO_CFG(gpio_i2c_pin.scl_pin, 0, GPIO_CFG_OUTPUT,
				GPIO_CFG_PULL_UP, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
	gpio_set_value(gpio_i2c_pin.sda_pin, 1);
	gpio_set_value(gpio_i2c_pin.scl_pin, 1);

	if (gpio_i2c_pin.reset_pin) {
		gpio_tlmm_config(GPIO_CFG(gpio_i2c_pin.reset_pin, 0, GPIO_CFG_OUTPUT,
					GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
		gpio_set_value(gpio_i2c_pin.reset_pin, 1);
	}

	if (gpio_i2c_pin.irq_pin) {
		//gpio_tlmm_config(GPIO_CFG(gpio_i2c_pin.irq_pin, 0, GPIO_CFG_INPUT,
		//			GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
		gpio_tlmm_config(GPIO_CFG(gpio_i2c_pin.irq_pin, 0, GPIO_CFG_INPUT,
					GPIO_CFG_PULL_UP, GPIO_CFG_2MA), GPIO_CFG_ENABLE);

		i2c_board_info_data->irq =
			MSM_GPIO_TO_INT(gpio_i2c_pin.irq_pin);
	}

	return 0;
}

static void __init e0eu_init_i2c_touch(int bus_num)
{
	ts_i2c_device.id = bus_num;

	init_gpio_i2c_pin_touch(&ts_i2c_pdata, ts_i2c_pin[0], &ts_i2c_bdinfo[0]);
	i2c_register_board_info(bus_num, &ts_i2c_bdinfo[0], 1);
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

	printk("[Touch] %s() onoff:%d\n",__FUNCTION__, onoff);

	vreg_touch = vreg_get(0, "bt");

	if(IS_ERR(vreg_touch)) {
		printk("[Touch] vreg_get fail : touch\n");
		return -1;
	}

	if (onoff) {
		rc = vreg_set_level(vreg_touch, 3000);
		if (rc != 0) {
			printk("[Touch] vreg_set_level failed\n");
			return -1;
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

static void __init e0eu_init_i2c_touch(int bus_num)
{
	ts_i2c_device.id = bus_num;
	/* workaround for HDK rev_a no pullup */
	lge_init_gpio_i2c_pin(&ts_i2c_pdata, ts_i2c_pin, &ts_i2c_bdinfo[0]);
	i2c_register_board_info(bus_num, &ts_i2c_bdinfo[0], 1);
	platform_device_register(&ts_i2c_device);
}
#endif /* CONFIG_TOUCH_mxt_140 */

static struct gpio_i2c_pin accel_i2c_pin[] = {
	[0] = {
		.sda_pin	= SENSOR_GPIO_I2C_SDA,
		.scl_pin	= SENSOR_GPIO_I2C_SCL,
		.reset_pin	= 0,
		.irq_pin	= ACCEL_GPIO_INT,
	},
};

static struct gpio_i2c_pin ecom_i2c_pin[] = {
	[0] = {
		.sda_pin	= SENSOR_GPIO_I2C_SDA,
		.scl_pin	= SENSOR_GPIO_I2C_SCL,
		.reset_pin	= 0,
		.irq_pin	= ECOM_GPIO_INT,
	},
};


static struct i2c_gpio_platform_data sensor_i2c_pdata = {
	.sda_is_open_drain = 0,
	.scl_is_open_drain = 0,
	.udelay = 2,
};

static struct platform_device sensor_i2c_device = {
	.name = "i2c-gpio",
	.dev.platform_data = &sensor_i2c_pdata,
};

static struct i2c_board_info sensor_i2c_bdinfo[] = {
	[0] = {
		I2C_BOARD_INFO("bma250", ACCEL_I2C_ADDRESS),
		.type = "bma250",
	},
	[1] = {
		I2C_BOARD_INFO("bmm050", ECOM_I2C_ADDRESS),
		.type = "bmm050",
	},
};

static void __init e0_init_i2c_sensor(int bus_num)
{
	sensor_i2c_device.id = bus_num;

	lge_init_gpio_i2c_pin(&sensor_i2c_pdata, accel_i2c_pin[0], &sensor_i2c_bdinfo[0]);
	lge_init_gpio_i2c_pin(&sensor_i2c_pdata, ecom_i2c_pin[0], &sensor_i2c_bdinfo[1]);

	i2c_register_board_info(bus_num, sensor_i2c_bdinfo, ARRAY_SIZE(sensor_i2c_bdinfo));

	platform_device_register(&sensor_i2c_device);
}

/* proximity */

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

static int prox_power_set(unsigned char onoff)
{

/* LGE_CHANGE_S: E0 jiwon.seo@lge.com [2011-10-17] : for new bl */
#if defined(CONFIG_BACKLIGHT_AAT2870)
	if(onoff == 1) {
		aat28xx_ldo_set_level(NULL,1,1800);
		aat28xx_ldo_enable(NULL,1,1);
	} else {
		aat28xx_ldo_enable(NULL,1,0);
	}
#elif defined(CONFIG_BACKLIGHT_BU61800)
   if(onoff == 1) {
		bu61800_ldo_enable(NULL,1,1);
	} else {
		bu61800_ldo_enable(NULL,1,0);
	}
#else
	//default
#endif
/* LGE_CHANGE_E: E0 jiwon.seo@lge.com [2011-10-17] : for new bl */

	printk("[Proximity] %s() : Power %s\n",__FUNCTION__, onoff ? "On" : "Off");

	return 0;
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

static void __init e0eu_init_i2c_prox(int bus_num)
{
	proxi_i2c_device.id = bus_num;

	lge_init_gpio_i2c_pin(&proxi_i2c_pdata, proxi_i2c_pin[0], &prox_i2c_bdinfo[0]);

	i2c_register_board_info(bus_num, &prox_i2c_bdinfo[0], 1);
	platform_device_register(&proxi_i2c_device);
}

/* common function */
void __init lge_add_input_devices(void)
{
	platform_add_devices(e0eu_input_devices, ARRAY_SIZE(e0eu_input_devices));
	platform_add_devices(e0eu_gpio_input_devices, ARRAY_SIZE(e0eu_gpio_input_devices));
	lge_add_gpio_i2c_device(e0eu_init_i2c_touch);

        /* LGE_CHANGE_S [seven.kim@lge.com] to support new bosch accel+compass sensor */
	#if defined (CONFIG_SENSORS_BMM050) ||defined(CONFIG_SENSORS_BMA250)
	lge_add_gpio_i2c_device(e0_init_i2c_sensor);
	#else
	lge_add_gpio_i2c_device(e0eu_init_i2c_acceleration);
	lge_add_gpio_i2c_device(e0eu_init_i2c_ecom);
	#endif
	/* LGE_CHANGE_E [seven.kim@lge.com] to support new bosch accel+compass sensor */

	lge_add_gpio_i2c_device(e0eu_init_i2c_prox);
}
