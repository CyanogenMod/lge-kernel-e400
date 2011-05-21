#ifndef __ASM_ARCH_MSM_BOARD_LGE_H
#define __ASM_ARCH_MSM_BOARD_LGE_H

#include <linux/i2c.h>
#include <linux/i2c-gpio.h>

#if __GNUC__
#define __WEAK __attribute__((weak))
#endif

#define PMEM_KERNEL_EBI1_SIZE	0x1C000
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

void __init msm_msm7x2x_allocate_memory_regions(void);
void __init msm7x27a_reserve(void);

/* lge API functions to register i2c devices */
typedef void (gpio_i2c_init_func_t)(int bus_num);

void __init lge_add_gpio_i2c_device(gpio_i2c_init_func_t *init_func);
void __init lge_add_gpio_i2c_devices(void);
int __init lge_init_gpio_i2c_pin(struct i2c_gpio_platform_data *i2c_adap_pdata,
		struct gpio_i2c_pin gpio_i2c_pin,
		struct i2c_board_info *i2c_board_info_data);

/* lge common functions to add devices */
void __init lge_add_input_devices(void);
void __init lge_add_misc_devices(void);
void __init lge_add_mmc_devices(void);
void __init lge_add_sound_devices(void);

#endif
