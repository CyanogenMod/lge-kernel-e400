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
#include <linux/delay.h>
#ifdef CONFIG_TOUCHSCREEN_ATMEL_MAXTOUCH
#include <linux/atmel_maxtouch.h>
#endif
#ifdef CONFIG_TOUCHSCREEN_SYNAPTICS_I2C_RMI4
#include <linux/synaptics_i2c_rmi4.h>
#endif

#include "devices-msm7x2xa.h"
#include "board-msm7x27a_hdk.h"

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

/* GPIO key map for hdk */
static unsigned int keypad_row_gpios[] = {31, 32, 33};
static unsigned int keypad_col_gpios[] = {36, 37, 38};

#define KEYMAP_INDEX(row, col) ((row)*ARRAY_SIZE(keypad_col_gpios) + (col))

static const unsigned short keypad_keymap_hdk[] = {
	[KEYMAP_INDEX(0, 0)] = KEY_VOLUMEUP,
	[KEYMAP_INDEX(0, 1)] = KEY_HP, /*KEY_AF*/
	[KEYMAP_INDEX(0, 2)] = KEY_CAMERA,

	[KEYMAP_INDEX(1, 0)] = KEY_VOLUMEDOWN,
	[KEYMAP_INDEX(1, 1)] = KEY_BACK,
	[KEYMAP_INDEX(1, 2)] = KEY_HOME,

	[KEYMAP_INDEX(2, 0)] = KEY_F2, /*KEY_EXTRA*/
	[KEYMAP_INDEX(2, 1)] = KEY_SEARCH,
	[KEYMAP_INDEX(2, 2)] = KEY_MENU, 
};

int hdk_matrix_info_wrapper(struct gpio_event_input_devs *input_dev,struct gpio_event_info *info, void **data, int func)
{
        int ret ;
		if (func == GPIO_EVENT_FUNC_RESUME) {
			gpio_tlmm_config(GPIO_CFG(keypad_col_gpios[0], 0,
						GPIO_CFG_INPUT, GPIO_CFG_PULL_UP,GPIO_CFG_2MA), GPIO_CFG_ENABLE);
			gpio_tlmm_config(GPIO_CFG(keypad_col_gpios[1], 0,
						GPIO_CFG_INPUT, GPIO_CFG_PULL_UP,GPIO_CFG_2MA), GPIO_CFG_ENABLE);
			gpio_tlmm_config(GPIO_CFG(keypad_col_gpios[2], 0,
						GPIO_CFG_INPUT, GPIO_CFG_PULL_UP,GPIO_CFG_2MA), GPIO_CFG_ENABLE);						
		}

		ret = gpio_event_matrix_func(input_dev,info, data,func);
        return ret ;
}

static int hdk_gpio_matrix_power(
                const struct gpio_event_platform_data *pdata, bool on)
{
	/* this is dummy function to make gpio_event driver register suspend function
	 * 2010-01-29, cleaneye.kim@lge.com
	 * copy from ALOHA code
	 * 2010-04-22 younchan.kim@lge.com
	 */

	return 0;
}

static struct gpio_event_matrix_info hdk_keypad_matrix_info = {
	.info.func	= hdk_matrix_info_wrapper,
	.keymap		= keypad_keymap_hdk,
	.output_gpios	= keypad_row_gpios,
	.input_gpios	= keypad_col_gpios,
	.noutputs	= ARRAY_SIZE(keypad_row_gpios),
	.ninputs	= ARRAY_SIZE(keypad_col_gpios),
	.settle_time.tv.nsec = 40 * NSEC_PER_USEC,
	.poll_time.tv.nsec = 20 * NSEC_PER_MSEC,
	.flags		= GPIOKPF_LEVEL_TRIGGERED_IRQ | GPIOKPF_PRINT_UNMAPPED_KEYS | GPIOKPF_DRIVE_INACTIVE
};

static struct gpio_event_info *hdk_keypad_info[] = {
	&hdk_keypad_matrix_info.info
};

static struct gpio_event_platform_data hdk_keypad_data = {
	.name		= "hdk_keypad",
	.info		= hdk_keypad_info,
	.info_count	= ARRAY_SIZE(hdk_keypad_info),
	.power          = hdk_gpio_matrix_power,
};

struct platform_device keypad_device_hdk = {
	.name	= GPIO_EVENT_DEV_NAME,
	.id	= -1,
	.dev	= {
		.platform_data	= &hdk_keypad_data,
	},
};

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
static struct platform_device *hdk_input_devices[] __initdata = {
	&hs_pdev,
};

static struct platform_device *hdk_gpio_input_devices[] __initdata = {
	&keypad_device_hdk,/* the gpio keypad for hdk */
#ifdef CONFIG_KEYBOARD_PP2106
	&hdk_qwerty_device,
#endif
};

/* Atmel Touch for hdk */
#if defined(CONFIG_TOUCHSCREEN_ATMEL_MAXTOUCH)

#define ATMEL_TS_I2C_NAME "maXTouch"
#define ATMEL_TS_GPIO_IRQ TS_GPIO_IRQ

static int atmel_ts_power_on(bool onoff)
{
	int ret = 0;

	ret = gpio_request(TS_GPIO_LDO_EN, "maxtouch_LDO_gpio");
	if (ret) {
		pr_err("Requesting TOUCH_LDO_EN GPIO failed!\n");
		goto err;
	}

	ret = gpio_direction_output(TS_GPIO_LDO_EN, onoff);
	if (ret) {
		pr_err("Setting TOUCH_LDO_EN GPIO direction failed!\n");
		goto err2;
	}

	/* 	LGE -murali.ramaiah@lge.com
		power on reset delay (40-70ms  --> maXT140 spec)
	*/
	if(onoff) {
		usleep_range(40000, 70000);
	}

err2:
	gpio_free(TS_GPIO_LDO_EN);
err:
	return 0;
}

static int atmel_ts_platform_init(struct i2c_client *client)
{
	int rc;

	rc = gpio_tlmm_config(GPIO_CFG(ATMEL_TS_GPIO_IRQ, 0,
				GPIO_CFG_INPUT, GPIO_CFG_PULL_UP,
				GPIO_CFG_8MA), GPIO_CFG_ENABLE);
	if (rc) {
		pr_err("%s: gpio_tlmm_config for %d failed\n",
			__func__, ATMEL_TS_GPIO_IRQ);
		goto err_end;
	}

	/* configure touchscreen interrupt gpio */
	rc = gpio_request(ATMEL_TS_GPIO_IRQ, "maxtouch_IRQ_gpio");
	if (rc) {
		pr_err("%s: unable to request gpio %d\n",
			__func__, ATMEL_TS_GPIO_IRQ);
		goto ts_gpio_tlmm_unconfig;
	}

	rc = gpio_direction_input(ATMEL_TS_GPIO_IRQ);
	if (rc < 0) {
		pr_err("%s: unable to set the direction of gpio %d\n",
			__func__, ATMEL_TS_GPIO_IRQ);
		goto free_ts_gpio;
	}
	return 0;

free_ts_gpio:
	gpio_free(ATMEL_TS_GPIO_IRQ);
ts_gpio_tlmm_unconfig:
	gpio_tlmm_config(GPIO_CFG(ATMEL_TS_GPIO_IRQ, 0,
				GPIO_CFG_INPUT, GPIO_CFG_NO_PULL,
				GPIO_CFG_2MA), GPIO_CFG_DISABLE);
err_end:
	return rc;
}

static int atmel_ts_platform_exit(struct i2c_client *client)
{
	gpio_free(ATMEL_TS_GPIO_IRQ);
	gpio_tlmm_config(GPIO_CFG(ATMEL_TS_GPIO_IRQ, 0,
				GPIO_CFG_INPUT, GPIO_CFG_NO_PULL,
				GPIO_CFG_2MA), GPIO_CFG_DISABLE);
	atmel_ts_power_on(0);
	return 0;
}

static u8 atmel_ts_read_chg(void)
{
	return gpio_get_value(ATMEL_TS_GPIO_IRQ);
}

static u8 atmel_ts_valid_interrupt(void)
{
	return !atmel_ts_read_chg();
}

#define ATMEL_X_OFFSET 13
#define ATMEL_Y_OFFSET 0

static struct mxt_platform_data atmel_ts_pdata = {
	.numtouch = 4,
	.init_platform_hw = atmel_ts_platform_init,
	.exit_platform_hw = atmel_ts_platform_exit,
	.power_on = atmel_ts_power_on,
	.display_res_x = 320,
	.display_res_y = 480,
	.min_x = ATMEL_X_OFFSET,
	.max_x = (345 - ATMEL_X_OFFSET),
	.min_y = ATMEL_Y_OFFSET,
	.max_y = (480 - ATMEL_Y_OFFSET),
	.valid_interrupt = atmel_ts_valid_interrupt,
	.read_chg = atmel_ts_read_chg,
};

#elif defined(CONFIG_TOUCHSCREEN_MXT140)

static int ts_set_vreg(unsigned char onoff)
{
	int ret = 0;
	
	ret = gpio_request(TS_GPIO_LDO_EN, "TOUCH_LDO_EN");
	if (ret) {
		pr_err("Requesting TOUCH_LDO_EN GPIO failed!\n");
		goto err;
	}

	ret = gpio_direction_output(TS_GPIO_LDO_EN, onoff);
	if (ret) {
		pr_err("Setting TOUCH_LDO_EN GPIO direction failed!\n");
		goto err2;
	}

err2:
	gpio_free(TS_GPIO_LDO_EN);
err:
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

#elif defined(CONFIG_TOUCHSCREEN_SYNAPTICS_I2C_RMI4)

static int synaptics_t1320_power_on(int onoff, bool log_en)
{
	int ret = 0;

	ret = gpio_request(TS_GPIO_LDO_EN, "TOUCH_LDO_EN");
	if (ret) {
		pr_err("Requesting TOUCH_LDO_EN GPIO failed!\n");
		goto err;
	}

	ret = gpio_direction_output(TS_GPIO_LDO_EN, onoff);
	if (ret) {
		pr_err("Setting TOUCH_LDO_EN GPIO direction failed!\n");
		gpio_free(TS_GPIO_LDO_EN);
	}

err:
	return 0;
}

static struct synaptics_ts_platform_data synaptics_t1320_ts_platform_data = {
	.use_irq             = 1,
	.irqflags            = IRQF_TRIGGER_FALLING,
	.i2c_sda_gpio        = TS_GPIO_I2C_SDA,
	.i2c_scl_gpio        = TS_GPIO_I2C_SCL,
	.i2c_int_gpio        = TS_GPIO_IRQ,
	.power               = synaptics_t1320_power_on,
	.ic_booting_delay    = 400,       /* ms */
	.report_period       = 12500000,  /* 12.5 msec */
	.num_of_finger       = 10,
	.num_of_button       = 4,
	.button[0]           = KEY_MENU,
	.button[1]           = KEY_HOME,
	.button[2]           = KEY_BACK,
	.button[3]           = KEY_SEARCH,
	.x_max               = 1036,  /* for 4.0" */
	.y_max               = 1728,  /* for 4.0" */
	.fw_ver              = 1,
	.palm_threshold      = 0,
	.delta_pos_threshold = 0,
};
#endif

static struct i2c_board_info ts_i2c_bdinfo[] = {
#if defined(CONFIG_TOUCHSCREEN_ATMEL_MAXTOUCH)
	{
		I2C_BOARD_INFO(ATMEL_TS_I2C_NAME, 0x4A),
		.platform_data = &atmel_ts_pdata,
		.irq = MSM_GPIO_TO_INT(ATMEL_TS_GPIO_IRQ),
	},
#elif defined(CONFIG_TOUCHSCREEN_MXT140)
	{
		I2C_BOARD_INFO("qt602240_ts", 0x4A),
		.type = "qt602240_ts",
		.platform_data = &ts_pdata,
	},
#elif defined(CONFIG_TOUCHSCREEN_SYNAPTICS_I2C_RMI4)
	{
		I2C_BOARD_INFO("synaptics_ts", 0x20),
		.platform_data = &synaptics_t1320_ts_platform_data,
		.irq = MSM_GPIO_TO_INT(TS_GPIO_IRQ),
	},
#endif
};

/** accelerometer **/
#if defined(CONFIG_SENSOR_K3DH)
static int accel_power_on(void)
{
	int ret = 0;
	struct vreg *gp2_vreg = vreg_get(0, ACCEL_VREG_2_8V);

	printk("[Accelerometer] %s() : Power On\n",__FUNCTION__);

	vreg_set_level(gp2_vreg, 3000);
	vreg_enable(gp2_vreg);

	return ret;
}

static int accel_power_off(void)
{
	int ret = 0;
	struct vreg *gp2_vreg = vreg_get(0, PROXI_VREG_2_8V);

	printk("[Accelerometer] %s() : Power Off\n",__FUNCTION__);

	vreg_disable(gp2_vreg);

	return ret;
}

static struct k3dh_platform_data accel_pdata = {
	.poll_interval = 100,
	.min_interval = 0,
	.g_range = 0x00,
	.axis_map_x = 0,
	.axis_map_y = 1,
	.axis_map_z = 2,

	.negate_x = 0,
	.negate_y = 0,
	.negate_z = 0,

	.power_on      = accel_power_on,
	.power_off	= accel_power_off,
	.sda_pin		= ACCEL_GPIO_I2C_SDA,
	.scl_pin		= ACCEL_GPIO_I2C_SCL,
	.pin_int 		= ACCEL_GPIO_INT,
	//.gpio_int   = ACCEL_GPIO_INT,
	//.irq        = MSM_GPIO_TO_INT(ACCEL_GPIO_INT),
};

#endif

static struct i2c_board_info accel_i2c_bdinfo[] = {
#if defined(CONFIG_SENSOR_K3DH)
	[0] = {
		I2C_BOARD_INFO("K3DH", ACCEL_I2C_ADDRESS),
		.platform_data = &accel_pdata,
	},
#endif
};

#ifdef CONFIG_MSM_CAMERA_FLASH_LM2759
#define LM2759_FLASH_I2C_SLAVE_ADDR 0x53
static struct led_flash_platform_data lm2759_flash_pdata = {
	.gpio_flen		= 0,
	.gpio_en_set	= -1,
	.gpio_inh		= -1,
};

static struct i2c_board_info lm2759_i2c_bdinfo[] = {
	{
		I2C_BOARD_INFO("lm2759", LM2759_FLASH_I2C_SLAVE_ADDR/*0x53*/),
		.platform_data = &lm2759_flash_pdata,
	},

};
#endif

/* ecompass */
static int ecom_power_set(unsigned char onoff)
{
	int ret = 0;
	struct vreg *gp2_vreg = vreg_get(0, ECOM_VREG_2_8V);

	printk("[Ecompass] %s() : Power %s\n",__FUNCTION__, onoff ? "On" : "Off");

	if (onoff) {
		vreg_set_level(gp2_vreg, 3000);
		vreg_enable(gp2_vreg);
	} else
		vreg_disable(gp2_vreg);

	return ret;
}

static struct ecom_platform_data ecom_pdata = {
	.power          = ecom_power_set,
	.pin_int        = 0,
	.pin_rst        = 0,
	.drdy           = ECOM_GPIO_INT,
};

static struct i2c_board_info ecom_i2c_bdinfo[] = {
	[0] = {
		I2C_BOARD_INFO("ami306_sensor", ECOM_I2C_ADDRESS),
		.platform_data = &ecom_pdata,
	},
};

/* proximity */
static int prox_power_set(unsigned char onoff)
{
	int ret = 0;
	struct vreg *gp2_vreg;
	gp2_vreg = vreg_get(0, PROXI_VREG_2_8V);
	if (onoff) {
		vreg_set_level(gp2_vreg, 2800);
		vreg_enable(gp2_vreg);
		msleep(10);
	} else
		vreg_disable(gp2_vreg);

	return ret;
}

static struct proximity_platform_data proxi_pdata = {
	.irq_num	= PROXI_GPIO_DOUT,
	.power		= prox_power_set,
	.methods		= 0,
	.operation_mode		= 2,
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

/* light ambient sensor */
#if defined(CONFIG_SENSORS_BH1721)

#define AMBIENT_VREG_2_8V		"gp2"
#define AMBIENT_I2C_ADDRESS		0x23
#define AMBIENT_DVI_GPIO		17

static int ambient_power_set(unsigned char onoff)
{
	int ret = 0;
	struct vreg *gp2_vreg;
	gp2_vreg = vreg_get(0, AMBIENT_VREG_2_8V);
	if (onoff) {
		vreg_set_level(gp2_vreg, 2800);
		vreg_enable(gp2_vreg);
		msleep(10);
	} else
		vreg_disable(gp2_vreg);

	return ret;
}

static int ambient_dvi_reset_ctrl(unsigned char onoff)
{
	int rc = 0;
	rc = gpio_request(AMBIENT_DVI_GPIO, "ambient_dvi_gpio");
	if (rc) {
		pr_err("%s: unable to request gpio %d\n",
			__func__, AMBIENT_DVI_GPIO);
		goto ambient_err_end;
	}
	rc = gpio_direction_output(AMBIENT_DVI_GPIO, onoff);
	if (rc) {
		pr_err("dvi_gpio direction failed!\n");
		goto ambient_err_end;
	}

ambient_err_end:
	gpio_free(AMBIENT_DVI_GPIO);
	return rc;
}
static struct light_ambient_platform_data ambient_pdata = {
	.power_state  = 0,
	.dvi_gpio = AMBIENT_DVI_GPIO,
	.power_on	= ambient_power_set,
	.dvi_reset_ctrl = ambient_dvi_reset_ctrl,
};

static struct i2c_board_info ambient_i2c_bdinfo[] = {
	[0] = {
		I2C_BOARD_INFO("ambient_bh1721", AMBIENT_I2C_ADDRESS),
		.platform_data = &ambient_pdata,
	},
};

#endif

/* common function */
void __init lge_add_input_devices(void)
{
	platform_add_devices(hdk_input_devices, ARRAY_SIZE(hdk_input_devices));
	platform_add_devices(hdk_gpio_input_devices, ARRAY_SIZE(hdk_gpio_input_devices));
	i2c_register_board_info(MSM_GSBI1_QUP_I2C_BUS_ID,
			ts_i2c_bdinfo,
			ARRAY_SIZE(ts_i2c_bdinfo));
	i2c_register_board_info(MSM_GSBI0_QUP_I2C_BUS_ID,
			accel_i2c_bdinfo,
			ARRAY_SIZE(accel_i2c_bdinfo));
	i2c_register_board_info(MSM_GSBI0_QUP_I2C_BUS_ID,
			ecom_i2c_bdinfo,
			ARRAY_SIZE(ecom_i2c_bdinfo));			
	i2c_register_board_info(MSM_GSBI0_QUP_I2C_BUS_ID,
			prox_i2c_bdinfo,
			ARRAY_SIZE(prox_i2c_bdinfo));

#if defined(CONFIG_SENSORS_BH1721)
	i2c_register_board_info(MSM_GSBI0_QUP_I2C_BUS_ID,
			ambient_i2c_bdinfo,
			ARRAY_SIZE(ambient_i2c_bdinfo));
#endif

#ifdef CONFIG_MSM_CAMERA_FLASH_LM2759
	i2c_register_board_info(MSM_GSBI1_QUP_I2C_BUS_ID,
			lm2759_i2c_bdinfo,
			ARRAY_SIZE(lm2759_i2c_bdinfo));
#endif

}
