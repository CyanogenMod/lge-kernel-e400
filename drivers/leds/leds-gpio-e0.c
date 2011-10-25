/*
 * Phoenix Keypad LED Driver for the OMAP4430 SDP
 *
 * Copyright (C) 2010 Texas Instruments
 *
 * Author: Dan Murphy <DMurphy@ti.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published by
 * the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#include <linux/gpio.h>
#include <linux/leds.h>
#include <linux/platform_device.h>
#include <linux/slab.h>

#define KP_LEDS_GPIO 96

struct keypad_led_data {
	struct led_classdev keypad_led_class_dev;
};

static void e0_keypad_led_store(struct led_classdev *led_cdev,
				enum led_brightness value)
{
	if (value > LED_OFF) {
		gpio_set_value(KP_LEDS_GPIO, 1);
	} else {
		gpio_set_value(KP_LEDS_GPIO, 0);
	}
}

static int __devinit e0_keypad_led_probe(struct platform_device *pdev)
{
	int ret = 0;
	struct keypad_led_data *info;
	
	ret = gpio_request(KP_LEDS_GPIO, "kp_leds_gpio"); 
	if(ret){
		printk(KERN_ERR "[kp_led]: request gpio %d failed!\n", KP_LEDS_GPIO);
		return ret;
	}
		
	ret = gpio_direction_output(KP_LEDS_GPIO, 0); 
	if(ret){
		printk(KERN_ERR"[kp_led]: set gpio %d direction out error!\n", KP_LEDS_GPIO);
		return ret;
	}
	
	info = kzalloc(sizeof(struct keypad_led_data), GFP_KERNEL);
	if (info == NULL) {
		ret = -ENOMEM;
		return ret;
	}

	platform_set_drvdata(pdev, info);

	info->keypad_led_class_dev.name = "button-backlight";
	info->keypad_led_class_dev.brightness_set =
			e0_keypad_led_store;
	info->keypad_led_class_dev.max_brightness = LED_FULL;

	ret = led_classdev_register(&pdev->dev,
				    &info->keypad_led_class_dev);
	if (ret < 0) {
		pr_err("[kp_led]: %s: Register led class failed\n", __func__);
		kfree(info);
		return ret;
	}
	return ret;
}

static int e0_keypad_led_remove(struct platform_device *pdev)
{
	struct keypad_led_data *info = platform_get_drvdata(pdev);
	led_classdev_unregister(&info->keypad_led_class_dev);
	return 0;
}

static struct platform_driver e0_keypad_led_driver = {
	.probe = e0_keypad_led_probe,
	.remove = e0_keypad_led_remove,
	.driver = {
		.name = "keypad_led",
		.owner = THIS_MODULE,
	},
};

static int __init e0_keypad_led_init(void)
{
	return platform_driver_register(&e0_keypad_led_driver);
}

static void __exit e0_keypad_led_exit(void)
{
	platform_driver_unregister(&e0_keypad_led_driver);
}

module_init(e0_keypad_led_init);
module_exit(e0_keypad_led_exit);

MODULE_DESCRIPTION("E0 Keypad Lighting driver");
MODULE_AUTHOR("xuming <xuming.chen@agreeya.com");
MODULE_LICENSE("GPL");