#ifndef __ASM_ARCH_MSM_BOARD_LGE_H
#define __ASM_ARCH_MSM_BOARD_LGE_H

#include <linux/i2c.h>
#include <linux/i2c-gpio.h>

#if __GNUC__
#define __WEAK __attribute__((weak))
#endif

#ifdef CONFIG_ANDROID_RAM_CONSOLE
/* allocate 128K * 2 instead of ram_console's original size 128K
 * this is for storing kernel panic log which is used by lk loader
 * 2010-03-03, cleaneye.kim@lge.com
 */
#define MSM7X27_EBI1_CS0_BASE	PHYS_OFFSET
#define LGE_RAM_CONSOLE_SIZE    (128 * SZ_1K * 2)
#endif

#define PMEM_KERNEL_EBI1_SIZE	0x3A000
#define MSM_PMEM_AUDIO_SIZE	0x5B000

#ifdef CONFIG_ARCH_MSM7X27A
#define MSM_PMEM_MDP_SIZE       0x1DD1000
#define MSM_PMEM_ADSP_SIZE      0x1000000
#define MSM_FB_SIZE             0x195000
#endif

/* define gpio pin number of i2c-gpio */
struct gpio_i2c_pin {
	unsigned int sda_pin;
	unsigned int scl_pin;
	unsigned int reset_pin;
	unsigned int irq_pin;
};

/* touch screen platform data */
#if defined(CONFIG_TOUCHSCREEN_MCS8000)
struct touch_platform_data {
	int ts_x_min;
	int ts_x_max;
	int ts_y_min;
	int ts_y_max;
	int (*power)(unsigned char onoff);
	int irq;
	int scl;
	int sda;
};
#endif

#if defined(CONFIG_TOUCHSCREEN_MXT140)
struct touch_platform_data {
	int ts_x_min;
	int ts_x_max;
	int ts_y_min;
	int ts_y_max;
	int ts_y_start;
	int ts_y_scrn_max;
	int (*power)(unsigned char onoff);
	int irq;
	int gpio_int;
	int hw_i2c;
	int scl;
	int sda;
	int ce;
};
#endif

/* acceleration platform data */
struct acceleration_platform_data {
	int irq_num;
	int (*power)(unsigned char onoff);
};

#if defined(CONFIG_SENSOR_K3DH)
/* k3dh acceleration platform data */
struct k3dh_platform_data {
	int poll_interval;
	int min_interval;

	u8 g_range;

	u8 axis_map_x;
	u8 axis_map_y;
	u8 axis_map_z;

	u8 negate_x;
	u8 negate_y;
	u8 negate_z;

	int (*kr_init)(void);
	void (*kr_exit)(void);
	int (*power_on)(void);
	int (*power_off)(void);
	int sda_pin;
	int scl_pin;
	int pin_int;
	int (*gpio_config)(int);
	//int gpio_int;
	//int irq;
};
#endif

/* ecompass platform data */
struct ecom_platform_data {
	int pin_int;
	int pin_rst;
	int (*power)(unsigned char onoff);
	char accelerator_name[20];
	int fdata_sign_x;
        int fdata_sign_y;
        int fdata_sign_z;
	int fdata_order0;
	int fdata_order1;
	int fdata_order2;
	int sensitivity1g;
	s16 *h_layout;
	s16 *a_layout;
	int drdy;
};

/* proximity platform data */
struct proximity_platform_data {
	int irq_num;
	int (*power)(unsigned char onoff);
	int methods;
	int operation_mode;
	int debounce;
	u8 cycle;
};

/* backlight platform data*/
struct lge_backlight_platform_data {
	void (*platform_init)(void);
	int gpio;
	unsigned int mode;		     /* initial mode */
	int max_current;			 /* led max current(0-7F) */
	int initialized;			 /* flag which initialize on system boot */
	int version;				 /* Chip version number */
};

/* android vibrator platform data */
struct android_vibrator_platform_data {
	int enable_status;
	int (*power_set)(int enable); 		/* LDO Power Set Function */
	int (*pwm_set)(int enable, int gain); 		/* PWM Set Function */
	int (*ic_enable_set)(int enable); 	/* Motor IC Set Function */
	int (*gpio_request)(void);	/* gpio request */
	int amp_value;				/* PWM tuning value */
};

struct gpio_h2w_platform_data {
	int gpio_detect;
	int gpio_button_detect;
};

/* gpio switch platform data */
struct lge_gpio_switch_platform_data {
	const char *name;
	unsigned *gpios;
	size_t num_gpios;
	unsigned long irqflags;
	unsigned int wakeup_flag;
	int (*work_func)(void);
	char *(*print_name)(void);
	char *(*print_state)(int state);
	int (*sysfs_store)(const char *buf, size_t size);
	int (*additional_init)(void);

	/* especially to address gpio key */
	unsigned *key_gpios;
	size_t num_key_gpios;
	int (*key_work_func)(int *value);
};

void __init msm_msm7x2x_allocate_memory_regions(void);
void __init msm7x27a_reserve(void);

/* lge API functions to register i2c devices */
typedef void (gpio_i2c_init_func_t)(int bus_num);

void __init lge_add_gpio_i2c_device(gpio_i2c_init_func_t *init_func);
void __init lge_add_gpio_i2c_devices(void);
int __init lge_init_gpio_i2c_pin(struct i2c_gpio_platform_data *i2c_adap_pdata,
		struct gpio_i2c_pin gpio_i2c_pin,
		struct i2c_board_info *i2c_board_info_data);
int __init lge_init_gpio_i2c_pin_pullup(struct i2c_gpio_platform_data *i2c_adap_pdata,
		struct gpio_i2c_pin gpio_i2c_pin,
		struct i2c_board_info *i2c_board_info_data);

void __init msm_add_fb_device(void);
void __init msm_add_pmem_devices(void);

/* lge common functions to add devices */
void __init lge_add_input_devices(void);
void __init lge_add_misc_devices(void);
void __init lge_add_mmc_devices(void);
void __init lge_add_sound_devices(void);
void __init lge_add_lcd_devices(void);
void __init lge_add_camera_devices(void);
void __init lge_add_pm_devices(void);
void __init lge_add_usb_devices(void);
void __init lge_add_connectivity_devices(void);

void __init lge_add_gpio_i2c_device(gpio_i2c_init_func_t *init_func);

void __init lge_add_ramconsole_devices(void);

#endif
