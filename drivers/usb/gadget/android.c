/*
 * Gadget Driver for Android
 *
 * Copyright (C) 2008 Google, Inc.
 * Copyright (c) 2009-2010, Code Aurora Forum. All rights reserved.
 * Author: Mike Lockwood <lockwood@android.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 */

/* #define DEBUG */
/* #define VERBOSE_DEBUG */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/fs.h>

#include <linux/delay.h>
#include <linux/kernel.h>
#include <linux/utsname.h>
#include <linux/platform_device.h>
#include <linux/pm_runtime.h>
#include <linux/debugfs.h>

#include <linux/usb/android_composite.h>
#include <linux/usb/ch9.h>
#include <linux/usb/composite.h>
#include <linux/usb/gadget.h>

#include "gadget_chips.h"


#ifdef CONFIG_LGE_USB_GADGET_DRIVER
#include <mach/vreg.h>
#include "gadget_chips.h"
#include "u_lgeusb.h"
#include <mach/rpc_hsusb.h>
#endif


#ifdef CONFIG_LGE_USB_SUPPORT_ANDROID_AUTORUN
#include <linux/time.h>
#endif


/*
 * Kbuild is not very cooperative with respect to linking separately
 * compiled library objects into one module.  So for now we won't use
 * separate compilation ... ensuring init/exit sections work to shrink
 * the runtime footprint, and giving us at least some parts of what
 * a "gcc --combine ... part1.c part2.c part3.c ... " build would.
 */
#include "usbstring.c"
#include "config.c"
#include "epautoconf.c"
#include "composite.c"

MODULE_AUTHOR("Mike Lockwood");
MODULE_DESCRIPTION("Android Composite USB Driver");
MODULE_LICENSE("GPL");
MODULE_VERSION("1.0");

#ifdef CONFIG_LGE_USB_GADGET_DRIVER
/* product id */
u16 product_id;
int android_set_pid(const char *val, struct kernel_param *kp);
static int android_get_pid(char *buffer, struct kernel_param *kp);
module_param_call(product_id, android_set_pid, android_get_pid,
					&product_id, 0664);
MODULE_PARM_DESC(product_id, "USB device product id");
#endif

#ifdef CONFIG_LGE_USB_GADGET_DRIVER
const u16 lg_default_pid	= 0x618E;
#define lg_default_pid_string "618E"

const u16 lg_ums_pid		= 0x6209;
#define lg_ums_pid_string "6209"

#ifdef CONFIG_LGE_USB_GADGET_SUPPORT_FACTORY_USB
static int cable;
int switch_cable;
#endif

/* [yk.kim@lge.com] 2011-01-05, workaround flag, FIX ME */

//hyunjin tmp for compile  static int switch_flag = 0;

int set_pid_flag = 0;
#ifndef CONFIG_LGE_USB_GADGET_NDIS_DRIVER
int hidden_pid = 0;	/* switching UMS pid */
#endif 
#endif /*CONFIG_LGE_USB_GADGET_DRIVER */

#ifdef CONFIG_USB_SUPPORT_LGE_ANDROID_AUTORUN

#ifdef CONFIG_USB_SUPPORT_LGE_ANDROID_AUTORUN_CGO
const u16 lg_charge_only_pid = 0xFFFF;
#define lg_charge_only_pid_string "FFFF"
#endif
	
static char autorun_serial_number[MAX_SERIAL_LEN] = "LGANDROIDVS760";
static u16 autorun_user_mode;
static int android_set_usermode(const char *val, struct kernel_param *kp);
module_param_call(user_mode, android_set_usermode, param_get_string,
					&autorun_user_mode, 0664);
MODULE_PARM_DESC(user_mode, "USB Autorun user mode");
#endif /*CONFIG_USB_SUPPORT_LGE_ANDROID_AUTORUN*/




static const char longname[] = "Gadget Android";

/* Default vendor and product IDs, overridden by platform data */
#define VENDOR_ID		0x18D1
#define PRODUCT_ID		0x0001

struct android_dev {
	struct usb_composite_dev *cdev;
	struct usb_configuration *config;
	int num_products;
	struct android_usb_product *products;
	int num_functions;
	char **functions;

	int product_id;
	int version;

#ifdef CONFIG_LGE_USB_GADGET_DRIVER
	struct mutex lock;
#endif
#ifdef CONFIG_LGE_USB_GADGET_FUNC_BIND_ONLY_INIT
    unique_usb_function unique_function;
#endif
};

static struct android_dev *_android_dev;

#ifdef CONFIG_USB_SUPPORT_LGE_ANDROID_AUTORUN
const u16 lg_autorun_pid	= 0x620B;
#define lg_autorun_pid_string "620B"
/* 2011.05.13 jaeho.cho@lge.com generate ADB USB uevent for gingerbread*/
int adb_disable = 1;
#endif

#define MAX_STR_LEN		16
/* string IDs are assigned dynamically */

#define STRING_MANUFACTURER_IDX		0
#define STRING_PRODUCT_IDX		1
#define STRING_SERIAL_IDX		2
#ifdef CONFIG_LGE_USB_GADGET_SUPPORT_FACTORY_USB
const u16 lg_factory_pid = 0x6000;
extern struct android_usb_platform_data android_usb_pdata_factory;

int lg_manual_test_mode = 0;
u8 manual_test_mode;
static int android_get_manual_test_mode(char *buffer, struct kernel_param *kp);
module_param_call(manual_test_mode, NULL, android_get_manual_test_mode,
					&manual_test_mode, 0444);
MODULE_PARM_DESC(manual_test_mode, "Manual Test Mode");
#endif

#ifdef CONFIG_LGE_USB_GADGET_SUPPORT_FACTORY_USB
char serial_number[MAX_SERIAL_LEN] = "\0";
char user_serial_number[MAX_SERIAL_LEN];
#else
char serial_number[MAX_STR_LEN];
#endif

#ifdef CONFIG_LGE_USB_GADGET_NDIS_DRIVER
const u16 lg_ndis_pid = 0x61A1;
#define lg_ndis_pid_string "61A1"

#elif defined(CONFIG_LGE_USB_GADGET_ECM_DRIVER)

const u16 lg_ecm_pid = 0x6208;
#define lg_ecm_pid_string "6208"

#elif defined(CONFIG_LGE_USB_GADGET_RNDIS_DRIVER)

const u16 lg_rndis_pid = 0x6208;
#define lg_rndis_pid_string "0x6208"
#endif /*CONFIG_LGE_USB_GADGET_NDIS_DRIVER*/

char serial_number[MAX_STR_LEN];
/* String Table */
static struct usb_string strings_dev[] = {
	/* These dummy values should be overridden by platform data */
	[STRING_MANUFACTURER_IDX].s = "Android",
	[STRING_PRODUCT_IDX].s = "Android",
#ifdef CONFIG_LGE_USB_GADGET_SUPPORT_FACTORY_USB
	[STRING_SERIAL_IDX].s = "\0",
#else
	[STRING_SERIAL_IDX].s = "0123456789ABCDEF",
#endif

	{  }			/* end of list */
};

static struct usb_gadget_strings stringtab_dev = {
	.language	= 0x0409,	/* en-us */
	.strings	= strings_dev,
};

static struct usb_gadget_strings *dev_strings[] = {
	&stringtab_dev,
	NULL,
};

static struct usb_device_descriptor device_desc = {
	.bLength              = sizeof(device_desc),
	.bDescriptorType      = USB_DT_DEVICE,
	.bcdUSB               = __constant_cpu_to_le16(0x0200),
	.bDeviceClass         = USB_CLASS_PER_INTERFACE,
	.idVendor             = __constant_cpu_to_le16(VENDOR_ID),
	.idProduct            = __constant_cpu_to_le16(PRODUCT_ID),
	.bcdDevice            = __constant_cpu_to_le16(0xffff),
	.bNumConfigurations   = 1,
};

static struct usb_otg_descriptor otg_descriptor = {
	.bLength =		sizeof otg_descriptor,
	.bDescriptorType =	USB_DT_OTG,
	.bmAttributes =		USB_OTG_SRP | USB_OTG_HNP,
	.bcdOTG               = __constant_cpu_to_le16(0x0200),
};

static const struct usb_descriptor_header *otg_desc[] = {
	(struct usb_descriptor_header *) &otg_descriptor,
	NULL,
};

static struct list_head _functions = LIST_HEAD_INIT(_functions);
static bool _are_functions_bound;

static void android_set_default_product(int pid);

#ifdef CONFIG_LGE_USB_GADGET_DRIVER
extern void diag_set_serial_number(char *serial_number);
#endif
#ifdef CONFIG_LGE_USB_GADGET_FUNC_BIND_ONLY_INIT
void android_set_device_class(u16 pid);
#endif

void android_usb_set_connected(int connected)
{
	if (_android_dev && _android_dev->cdev && _android_dev->cdev->gadget) {
		if (connected)
			usb_gadget_connect(_android_dev->cdev->gadget);
		else
			usb_gadget_disconnect(_android_dev->cdev->gadget);
	}
}

static struct android_usb_function *get_function(const char *name)
{
	struct android_usb_function	*f;
	list_for_each_entry(f, &_functions, list) {
		if (!strcmp(name, f->name))
			return f;
	}
	return 0;
}

static bool are_functions_registered(struct android_dev *dev)
{
	char **functions = dev->functions;
	int i;

	/* Look only for functions required by the board config */
	for (i = 0; i < dev->num_functions; i++) {
		char *name = *functions++;
		bool is_match = false;
		/* Could reuse get_function() here, but a reverse search
		 * should yield less comparisons overall */
		struct android_usb_function *f;
		list_for_each_entry_reverse(f, &_functions, list) {
			if (!strcmp(name, f->name)) {
				is_match = true;
				break;
			}
		}
		if (is_match)
			continue;
		else
			return false;
	}

	return true;
}

static bool should_bind_functions(struct android_dev *dev)
{
	/* Don't waste time if the main driver hasn't bound */
	if (!dev->config)
		return false;

	/* Don't waste time if we've already bound the functions */
	if (_are_functions_bound)
		return false;

	/* This call is the most costly, so call it last */
	if (!are_functions_registered(dev))
		return false;

	return true;
}

static void bind_functions(struct android_dev *dev)
{
	struct android_usb_function	*f;
	char **functions = dev->functions;
	int i;
	
	for (i = 0; i < dev->num_functions; i++) {
		char *name = *functions++;
		f = get_function(name);
		if (f)
			f->bind_config(dev->config);
		else
			pr_err("%s: function %s not found\n", __func__, name);
	}

	_are_functions_bound = true;
	/*
	 * set_alt(), or next config->bind(), sets up
	 * ep->driver_data as needed.
	 */
	usb_ep_autoconfig_reset(dev->cdev->gadget);
}

static int __ref android_bind_config(struct usb_configuration *c)
{
	struct android_dev *dev = _android_dev;

	pr_debug("android_bind_config\n");
	dev->config = c;

	if (should_bind_functions(dev)) {
		bind_functions(dev);
		android_set_default_product(dev->product_id);
	} else {
		/* Defer enumeration until all functions are bounded */
		if (c->cdev && c->cdev->gadget)
			usb_gadget_disconnect(c->cdev->gadget);
	}

	return 0;
}

static int android_setup_config(struct usb_configuration *c,
		const struct usb_ctrlrequest *ctrl);

static struct usb_configuration android_config_driver = {
	.label		= "android",
	.setup		= android_setup_config,
	.bConfigurationValue = 1,
	.bmAttributes	= USB_CONFIG_ATT_ONE | USB_CONFIG_ATT_SELFPOWER,
	.bMaxPower	= 0xFA, /* 500ma */
};

static int android_setup_config(struct usb_configuration *c,
		const struct usb_ctrlrequest *ctrl)
{
	int i;
	int ret = -EOPNOTSUPP;

	for (i = 0; i < android_config_driver.next_interface_id; i++) {
		if (android_config_driver.interface[i]->setup) {
			ret = android_config_driver.interface[i]->setup(
				android_config_driver.interface[i], ctrl);
			if (ret >= 0)
				return ret;
		}
	}
	return ret;
}

static int product_has_function(struct android_usb_product *p,
		struct usb_function *f)
{
	char **functions = p->functions;
	int count = p->num_functions;
	const char *name = f->name;
	int i;

	for (i = 0; i < count; i++) {
		/* For functions with multiple instances, usb_function.name
		 * will have an index appended to the core name (ex: acm0),
		 * while android_usb_product.functions[i] will only have the
		 * core name (ex: acm). So, only compare up to the length of
		 * android_usb_product.functions[i].
		 */
		if (!strncmp(name, functions[i], strlen(functions[i])))
			return 1;
	}
	return 0;
}

static int product_matches_functions(struct android_usb_product *p)
{
	struct usb_function		*f;
	list_for_each_entry(f, &android_config_driver.functions, list) {
		if (product_has_function(p, f) == !!f->disabled)
			return 0;
	}
	return 1;
}

#ifdef CONFIG_LGE_USB_GADGET_FUNC_BIND_ONLY_INIT
static int product_matches_unique_functions(struct android_dev *dev, struct android_usb_product *p)
{
    if(p->unique_function == dev->unique_function)
		return 1;
	else
		return 0;
}

static int get_init_product_id(struct android_dev *dev)
{
	struct android_usb_product *p = dev->products;
	int count = dev->num_products;
	int i;
	
    if (p) {
		for (i = 0; i < count; i++, p++) {  
			if (product_matches_unique_functions(dev,p))
			{
				USB_DBG("dev->product_id 0x%x\n", p->product_id);
				return p->product_id;
			}
		}
    }

	USB_DBG("dev->product_id 0x%x\n", dev->product_id);
	/* use default product ID */
	return dev->product_id;
}

#endif

static int get_product_id(struct android_dev *dev)
{
	struct android_usb_product *p = dev->products;
	int count = dev->num_products;
	int i;

#ifdef CONFIG_LGE_USB_GADGET_SUPPORT_FACTORY_USB
	if(product_id == lg_factory_pid)
		return product_id;
	else if(p) {
#else
	if (p) {
#endif
		for (i = 0; i < count; i++, p++) {
			if (product_matches_functions(p))
				return p->product_id;
		}
	}
	/* use default product ID */
	return dev->product_id;
}

#ifdef CONFIG_LGE_USB_GADGET_SUPPORT_FACTORY_USB
static int lge_bind_config(u16 pid)
{
	int ret = 0;

	if (pid == lg_factory_pid) 
	{
		serial_number[0] = '\0';
		msm_hsusb_is_serial_num_null(1); 
		device_desc.iSerialNumber = 0; 
	}
	else
	{
	    sprintf(serial_number, "%s", user_serial_number);
		ret = lge_get_usb_serial_number(serial_number);

		/* Send Serial number to A9 for software download */
		if (serial_number[0] != '\0') {
			msm_hsusb_is_serial_num_null(0);
			msm_hsusb_send_serial_number(serial_number);
		} else {
			/* If error to get serial number, we check the
			 * pdata's serial number. If pdata's serial number
			 * (e.g. default serial number) is set, we use the 
			 * serial number.
			 */
			/* TEMP : [yk.kim@lge.com] 2010-12-27, for user cable multi-download */
			//if (user_serial_number == NULL) {
				serial_number[0] = '\0';
				msm_hsusb_is_serial_num_null(1); 
				device_desc.iSerialNumber = 0; 
			//} else {
			//	msm_hsusb_is_serial_num_null(0);
			//	msm_hsusb_send_serial_number(serial_number);
			//}
		}
	}

	if (pid)
		msm_hsusb_send_productID(pid);

	android_config_driver.bmAttributes |= USB_CONFIG_ATT_SELFPOWER;
	android_config_driver.bMaxPower = 0xFA; /* 500 mA */

    return ret;
	
}
#endif

#ifdef CONFIG_USB_SUPPORT_LGE_ANDROID_AUTORUN
static char random_serial_number[14];
static void __init set_random_serial_number()
{
	struct timeval tv;
	do_gettimeofday(&tv);
	sprintf(random_serial_number,"123456789%5d", tv.tv_usec % 100000);
	pr_info("random_serial_number[%s]\n", random_serial_number);
	return;
}
#endif

static int is_msc_only_comp(int pid)
{
	struct android_dev *dev = _android_dev;
	struct android_usb_product *up = dev->products;
	int count;
	char **functions;
	int index;

	for (index = 0; index < dev->num_products; index++, up++) {
		if (pid == up->product_id)
			break;
	}

	count = up->num_functions;
	functions = up->functions;

	if (count == 1 && !strncmp(*functions, "usb_mass_storage", 32))
		return true;
	else
		return false;
}

static int __devinit android_bind(struct usb_composite_dev *cdev)
{
	struct android_dev *dev = _android_dev;
	struct usb_gadget	*gadget = cdev->gadget;
	int			gcnum, id, product_id, ret;

	pr_debug("android_bind\n");

	/* Allocate string descriptor numbers ... note that string
	 * contents can be overridden by the composite_dev glue.
	 */
	id = usb_string_id(cdev);
	if (id < 0)
		return id;
	strings_dev[STRING_MANUFACTURER_IDX].id = id;
	device_desc.iManufacturer = id;

	id = usb_string_id(cdev);
	if (id < 0)
		return id;
	strings_dev[STRING_PRODUCT_IDX].id = id;
	device_desc.iProduct = id;

	id = usb_string_id(cdev);
	if (id < 0)
		return id;
	strings_dev[STRING_SERIAL_IDX].id = id;
	device_desc.iSerialNumber = id;

	if (gadget_is_otg(cdev->gadget))
		android_config_driver.descriptors = otg_desc;

	if (!usb_gadget_set_selfpowered(gadget))
		android_config_driver.bmAttributes |= USB_CONFIG_ATT_SELFPOWER;
	/*
	 * Supporting remote wakeup for mass storage only function
	 * doesn't make sense, since there is no notifications that
	 * that can be sent from mass storage during suspend.
	 */
	if (gadget->ops->wakeup && !is_msc_only_comp((dev->product_id)))
		android_config_driver.bmAttributes |= USB_CONFIG_ATT_WAKEUP;
	else
		android_config_driver.bmAttributes &= ~USB_CONFIG_ATT_WAKEUP;


	/* register our configuration */
	ret = usb_add_config(cdev, &android_config_driver, android_bind_config);
	if (ret) {
		pr_err("%s: usb_add_config failed\n", __func__);
		return ret;
	}

	gcnum = usb_gadget_controller_number(gadget);
	if (gcnum >= 0)
		device_desc.bcdDevice = cpu_to_le16(0x0200 + gcnum);
	else {
		/* gadget zero is so simple (for now, no altsettings) that
		 * it SHOULD NOT have problems with bulk-capable hardware.
		 * so just warn about unrcognized controllers -- don't panic.
		 *
		 * things like configuration and altsetting numbering
		 * can need hardware-specific attention though.
		 */
		pr_warning("%s: controller '%s' not recognized\n",
			longname, gadget->name);
		device_desc.bcdDevice = __constant_cpu_to_le16(0x9999);
	}

	usb_gadget_set_selfpowered(gadget);
	dev->cdev = cdev;
	
#ifdef CONFIG_LGE_USB_GADGET_FUNC_BIND_ONLY_INIT
	product_id = get_init_product_id(dev);
#else
	product_id = get_product_id(dev);
#endif
#ifdef CONFIG_LGE_USB_GADGET_SUPPORT_FACTORY_USB
	ret = lge_bind_config(product_id);
#endif

	device_desc.idProduct = __constant_cpu_to_le16(product_id);
	cdev->desc.idProduct = device_desc.idProduct;

#ifdef CONFIG_USB_SUPPORT_LGE_ANDROID_AUTORUN
	if(product_id == lg_autorun_pid)
	{	   
	  strings_dev[STRING_SERIAL_IDX].s = autorun_serial_number;
	}
	else
	{
	  strings_dev[STRING_SERIAL_IDX].s = serial_number;
	}
#endif

#ifdef CONFIG_LGE_USB_GADGET_NDIS_DRIVER
	if(product_id == lg_ndis_pid)
	{
		device_desc.bDeviceClass		 = USB_CLASS_MISC;
		device_desc.bDeviceSubClass 	 = 0x02;
		device_desc.bDeviceProtocol 	 = 0x01;
	}
	else
	{
		device_desc.bDeviceClass		 = USB_CLASS_COMM;
		device_desc.bDeviceSubClass 	 = 0x00;
		device_desc.bDeviceProtocol 	 = 0x00;
	}
#else
	/* BEGIN:0010739 [yk.kim@lge.com] 2010-11-11 */
	/* ADD:0010739 set device desc. */
	device_desc.bDeviceClass		 = USB_CLASS_COMM;
	device_desc.bDeviceSubClass 	 = 0x00;
	device_desc.bDeviceProtocol 	 = 0x00;
	/* END:0010739 [yk.kim@lge.com] 2010-11-11 */
#endif

#ifdef CONFIG_LGE_USB_GADGET_DRIVER
#ifdef CONFIG_LGE_USB_GADGET_NDIS_DRIVER
#else
	/* workaround pid flag */
	hidden_pid = product_id;
#endif
#endif

#ifdef CONFIG_USB_SUPPORT_LGE_ANDROID_AUTORUN
	if (serial_number[0] == '\0')
		set_random_serial_number();
#endif

	return 0;
}

static struct usb_composite_driver android_usb_driver = {
	.name		= "android_usb",
	.dev		= &device_desc,
	.strings	= dev_strings,
	.enable_function = android_enable_function,
};

static bool is_func_supported(struct android_usb_function *f)
{
	char **functions = _android_dev->functions;
	int count = _android_dev->num_functions;
	const char *name = f->name;
	int i;

	for (i = 0; i < count; i++) {
		if (!strcmp(*functions++, name))
			return true;
	}
	return false;
}

void android_register_function(struct android_usb_function *f)
{
	struct android_dev *dev = _android_dev;

	pr_debug("%s: %s\n", __func__, f->name);

	if (!is_func_supported(f))
		return;

	list_add_tail(&f->list, &_functions);

	if (dev && should_bind_functions(dev)) {
		bind_functions(dev);
		android_set_default_product(dev->product_id);
		/* All function are bounded. Enable enumeration */
		if (dev->cdev && dev->cdev->gadget)
			usb_gadget_connect(dev->cdev->gadget);
	}

}

/**
 * android_set_function_mask() - enables functions based on selected pid.
 * @up: selected product id pointer
 *
 * This function enables functions related with selected product id.
 */
static void android_set_function_mask(struct android_usb_product *up)
{
	int index, found = 0;
	struct usb_function *func;
#ifdef CONFIG_USB_SUPPORT_LGE_ANDROID_AUTORUN
	static int autorun_started = 0;
#endif

	list_for_each_entry(func, &android_config_driver.functions, list) {
		/* adb function enable/disable handled separetely */
		if (!strcmp(func->name, "adb") && !func->disabled)
#ifdef CONFIG_USB_SUPPORT_LGE_ANDROID_AUTORUN
		{
			if(autorun_started)
			{
				if(up->product_id == lg_ndis_pid)
				{
					usb_function_set_enabled(func,!adb_disable);
				}
				else
				{
					usb_function_set_enabled(func,0);
				}

				continue;
			}
			else
			{
				if(up->product_id != lg_ndis_pid && up->product_id != lg_factory_pid)
				{
					autorun_started = 1;
				}
				else
				{
					continue;
				}
			}
		}
#else
			continue;
#endif

		for (index = 0; index < up->num_functions; index++) {
			if (!strcmp(up->functions[index], func->name)) {
				found = 1;
				break;
			}
		}

		if (found) { /* func is part of product. */
			/* if func is disabled, enable the same. */
			if (func->disabled)
				usb_function_set_enabled(func, 1);
			found = 0;
		} else { /* func is not part if product. */
			/* if func is enabled, disable the same. */
			if (!func->disabled)
				usb_function_set_enabled(func, 0);
		}
	}
}

/**
 * android_set_defaut_product() - selects default product id and enables
 * required functions
 * @product_id: default product id
 *
 * This function selects default product id using pdata information and
 * enables functions for same.
*/
static void android_set_default_product(int pid)
{
	struct android_dev *dev = _android_dev;
	struct android_usb_product *up = dev->products;
	int index;
	
	for (index = 0; index < dev->num_products; index++, up++) {
		if (pid == up->product_id)
			break;
	}
	android_set_function_mask(up);
	device_desc.idProduct = __constant_cpu_to_le16(pid);
	if (dev->cdev)
		dev->cdev->desc.idProduct = device_desc.idProduct;
}

/**
 * android_config_functions() - selects product id based on function need
 * to be enabled / disabled.
 * @f: usb function
 * @enable : function needs to be enable or disable
 *
 * This function selects first product id having required function.
 * RNDIS/MTP function enable/disable uses this.
*/
#if defined (CONFIG_USB_ANDROID_RNDIS) || defined (CONFIG_LGE_USB_GADGET_NDIS_DRIVER)
static void android_config_functions(struct usb_function *f, int enable)
{
	struct android_dev *dev = _android_dev;
	struct android_usb_product *up = dev->products;
	int index;

	/* Searches for product id having function */
	if (enable) {
		for (index = 0; index < dev->num_products; index++, up++) {
			if (product_has_function(up, f))
				break;
		}
		android_set_function_mask(up);
	} else
		android_set_default_product(dev->product_id);
}
#endif

#ifdef CONFIG_USB_SUPPORT_LGE_ANDROID_AUTORUN_CGO
extern void usb_charge_only_softconnect(void);
#endif

#ifdef CONFIG_LGE_USB_GADGET_DRIVER
int android_switch_composition(u16 pid)
{
	
	struct android_dev *dev = _android_dev;
	int ret = -EINVAL;
	
#ifdef CONFIG_LGE_USB_GADGET_FUNC_BIND_ONLY_INIT
    android_set_device_class(pid);
#endif

#ifdef CONFIG_USB_SUPPORT_LGE_ANDROID_AUTORUN_CGO
    usb_charge_only_softconnect();
#endif
#ifdef CONFIG_USB_SUPPORT_LGE_ANDROID_AUTORUN_CGO
	if (pid == lg_charge_only_pid) {
		product_id = pid;		
		device_desc.idProduct = __constant_cpu_to_le16(pid);
		if (dev->cdev)
			dev->cdev->desc.idProduct = device_desc.idProduct;
		/* If we are in charge only pid, disconnect android gadget */
		usb_gadget_disconnect(dev->cdev->gadget);
		return 0;
	}
#endif

	device_desc.idProduct = __constant_cpu_to_le16(pid);
	android_set_default_product(pid);
#ifdef CONFIG_LGE_USB_GADGET_DRIVER
	product_id = pid;
#endif

	if (dev->cdev)
		dev->cdev->desc.idProduct = device_desc.idProduct;

/* [yk.kim@lge.com] 2011-01-18, two UMS mode (eMMC/SDcard), need meid */
#ifdef CONFIG_USB_SUPPORT_LGE_ANDROID_AUTORUN 
#ifdef CONFIG_LGE_USB_GADGET_NDIS_DRIVER
	if (pid == lg_ums_pid) {
		if (serial_number[0] == '\0') {
			strings_dev[STRING_SERIAL_IDX].s = random_serial_number;
			device_desc.iSerialNumber = 3;

			if (dev->cdev)
				dev->cdev->desc.iSerialNumber = device_desc.iSerialNumber;		
		}
	}
#else
	if (pid == lg_ums_pid) {

		if (serial_number[0] == '\0') {
			strings_dev[STRING_SERIAL_IDX].s = random_serial_number;
			device_desc.iSerialNumber = 3;

			if (dev->cdev)
				dev->cdev->desc.iSerialNumber = device_desc.iSerialNumber;		
		}

		hidden_pid = lg_ums_pid;
		goto OUT;
	}
	else {
		hidden_pid = pid;
	}
#endif
#endif


#ifdef CONFIG_LGE_USB_GADGET_SUPPORT_FACTORY_USB
	ret = lge_bind_config(product_id);
#endif

	/* LGE_CHANGES_S [jaeho.cho@lge.com] 2010-08-16, Autorun Serial Number */
#ifdef CONFIG_USB_SUPPORT_LGE_ANDROID_AUTORUN
	if(product_id == lg_autorun_pid) {	   
		strings_dev[STRING_SERIAL_IDX].s = autorun_serial_number;
		device_desc.iSerialNumber = 3;
	}
	else {
		strings_dev[STRING_SERIAL_IDX].s = serial_number;
	}

	/* [yk.kim@lge.com] 2010-12-31, update serial number id */
	if (dev->cdev)
		dev->cdev->desc.iSerialNumber = device_desc.iSerialNumber;
#endif

#ifdef CONFIG_LGE_USB_GADGET_NDIS_DRIVER
	/* force reenumeration */
	if (dev->cdev && dev->cdev->gadget &&
			dev->cdev->gadget->speed != USB_SPEED_UNKNOWN) {
		usb_gadget_disconnect(dev->cdev->gadget);
		msleep(10);
		ret = usb_gadget_connect(dev->cdev->gadget);
	}

	return ret;
#else
OUT:
	/* force reenumeration */
	if (dev->cdev && dev->cdev->gadget &&
            dev->cdev->gadget->speed != USB_SPEED_UNKNOWN) {
		usb_gadget_disconnect(dev->cdev->gadget);
		msleep(10);
		ret = usb_gadget_connect(dev->cdev->gadget);
	}

    return ret;
#endif
}
#endif

#ifdef CONFIG_LGE_USB_GADGET_FUNC_BIND_ONLY_INIT
void android_set_device_class(u16 pid)
{
	struct android_dev *dev = _android_dev;
    int deviceclass = -1;
#ifdef CONFIG_LGE_USB_GADGET_NDIS_DRIVER
#else
#ifdef CONFIG_LGE_USB_GADGET_DRIVER
	if(pid == lg_android_pid) 
#else
    if(pid == lg_rmnet_pid) 
#endif
	{
	  deviceclass = USB_CLASS_COMM;
	  goto SetClass;
	}
#endif
#ifdef CONFIG_LGE_USB_GADGET_SUPPORT_FACTORY_USB
    if(pid == lg_factory_pid) 
    {
      deviceclass = USB_CLASS_COMM;
	  goto SetClass;
    }
#endif
#if defined(CONFIG_LGE_USB_GADGET_NDIS_DRIVER)
    if(pid == lg_ndis_pid) 
    {
  	  deviceclass = USB_CLASS_MISC;
  	  goto SetClass;
    }
#elif defined(CONFIG_LGE_USB_GADGET_ECM_DRIVER)
    if(pid == lg_ecm_pid) 
    {
  	  deviceclass = USB_CLASS_MISC;
  	  goto SetClass;
    }
#elif defined(CONFIG_LGE_USB_GADGET_RNDIS_DRIVER)
    if(pid == lg_rndis_pid) 
    {
  	  deviceclass = USB_CLASS_MISC;
  	  goto SetClass;
    }
#endif
#ifdef CONFIG_LGE_USB_GADGET_DRIVER
    if(pid == lg_ums_pid) 
    {
  	  deviceclass = USB_CLASS_PER_INTERFACE;
  	  goto SetClass;
    }
#endif
#ifdef CONFIG_USB_SUPPORT_LGE_ANDROID_AUTORUN
    if(pid == lg_autorun_pid) 
    {
  	  deviceclass = USB_CLASS_PER_INTERFACE;
  	  goto SetClass;
    }
#endif
#ifdef CONFIG_USB_SUPPORT_LGE_ANDROID_AUTORUN_CGO
    if(pid == lg_charge_only_pid) 
    {
  	  deviceclass = USB_CLASS_PER_INTERFACE;
  	  goto SetClass;
    }
#endif

SetClass:
	if(deviceclass == USB_CLASS_COMM)
	{
  		dev->cdev->desc.bDeviceClass = USB_CLASS_COMM;
		dev->cdev->desc.bDeviceSubClass      = 0x00;
		dev->cdev->desc.bDeviceProtocol      = 0x00;
	}
	else if(deviceclass == USB_CLASS_MISC)
	{
	  	dev->cdev->desc.bDeviceClass = USB_CLASS_MISC;
		dev->cdev->desc.bDeviceSubClass      = 0x02;
		dev->cdev->desc.bDeviceProtocol      = 0x01;
	}
	else if(deviceclass == USB_CLASS_PER_INTERFACE)
	{
		dev->cdev->desc.bDeviceClass = USB_CLASS_PER_INTERFACE;
		dev->cdev->desc.bDeviceSubClass      = 0x00;
		dev->cdev->desc.bDeviceProtocol      = 0x00;
	}
	else
	{
		dev->cdev->desc.bDeviceClass = USB_CLASS_PER_INTERFACE;
		dev->cdev->desc.bDeviceSubClass      = 0x00;
		dev->cdev->desc.bDeviceProtocol      = 0x00;
	}
}
#endif

void update_dev_desc(struct android_dev *dev)
{
	struct usb_function *f;
	struct usb_function *last_enabled_f = NULL;
	int num_enabled = 0;
	int has_iad = 0;

	dev->cdev->desc.bDeviceClass = USB_CLASS_PER_INTERFACE;
	dev->cdev->desc.bDeviceSubClass = 0x00;
	dev->cdev->desc.bDeviceProtocol = 0x00;

	list_for_each_entry(f, &android_config_driver.functions, list) {
		if (!f->disabled) {
			num_enabled++;
			last_enabled_f = f;
			if (f->descriptors[0]->bDescriptorType ==
					USB_DT_INTERFACE_ASSOCIATION)
				has_iad = 1;
		}
		if (num_enabled > 1 && has_iad) {
			dev->cdev->desc.bDeviceClass = USB_CLASS_MISC;
			dev->cdev->desc.bDeviceSubClass = 0x02;
			dev->cdev->desc.bDeviceProtocol = 0x01;
			break;
		}
	}

	if (num_enabled == 1) {
#ifdef CONFIG_USB_ANDROID_RNDIS
		if (!strcmp(last_enabled_f->name, "rndis")) {
#ifdef CONFIG_USB_ANDROID_RNDIS_WCEIS
			dev->cdev->desc.bDeviceClass =
					USB_CLASS_WIRELESS_CONTROLLER;
#else
			dev->cdev->desc.bDeviceClass = USB_CLASS_COMM;
#endif
		}
#endif
	}
}


static char *sysfs_allowed[] = {
	"rndis",
	"mtp",
	"adb",
};

static int is_sysfschange_allowed(struct usb_function *f)
{
	char **functions = sysfs_allowed;
	int count = ARRAY_SIZE(sysfs_allowed);
	int i;

	for (i = 0; i < count; i++) {
		if (!strncmp(f->name, functions[i], 32))
			return 1;
	}
	return 0;
}

int android_enable_function(struct usb_function *f, int enable)
{
	struct android_dev *dev = _android_dev;
	int disable = !enable;
	struct usb_gadget	*gadget = dev->cdev->gadget;
	int product_id;

	pr_info_ratelimited("%s: %s %s\n",
		__func__, enable ? "enable" : "disable", f->name);

	if (!is_sysfschange_allowed(f))
		return -EINVAL;
	if (!!f->disabled != disable) {
		usb_function_set_enabled(f, !disable);

#ifdef CONFIG_LGE_USB_GADGET_NDIS_DRIVER
	if (!strcmp(f->name, "ecm")) {

		/* We need to specify the COMM class in the device descriptor
		 * if we are using RNDIS.
		 */
		if (enable) {
			dev->cdev->desc.bDeviceClass = USB_CLASS_MISC;
			dev->cdev->desc.bDeviceSubClass 	 = 0x02;
			dev->cdev->desc.bDeviceProtocol 	 = 0x01;
		} else {
			dev->cdev->desc.bDeviceClass = USB_CLASS_COMM;
			dev->cdev->desc.bDeviceSubClass 	 = 0;
			dev->cdev->desc.bDeviceProtocol 	 = 0;
		}

		android_config_functions(f, enable);
	}
#else /* !CONFIG_LGE_USB_GADGET_NDIS_DRIVER */
#ifdef CONFIG_USB_ANDROID_RNDIS
		if (!strcmp(f->name, "rndis")) {

			/* We need to specify the COMM class in the device descriptor
			 * if we are using RNDIS.
			 */
			if (enable) {
#ifdef CONFIG_USB_ANDROID_RNDIS_WCEIS
				dev->cdev->desc.bDeviceClass = USB_CLASS_MISC;
				dev->cdev->desc.bDeviceSubClass      = 0x02;
				dev->cdev->desc.bDeviceProtocol      = 0x01;
#else
				dev->cdev->desc.bDeviceClass = USB_CLASS_COMM;
#endif
			} else {
				dev->cdev->desc.bDeviceClass = USB_CLASS_PER_INTERFACE;
				dev->cdev->desc.bDeviceSubClass      = 0;
				dev->cdev->desc.bDeviceProtocol      = 0;
			}

			android_config_functions(f, enable);
		}
#endif
#endif /* CONFIG_LGE_USB_GADGET_NDIS_DRIVER */

#ifdef CONFIG_USB_ANDROID_MTP
		if (!strcmp(f->name, "mtp"))
			android_config_functions(f, enable);
#endif

		product_id = get_product_id(dev);

		if (gadget && gadget->ops->wakeup &&
				!is_msc_only_comp((product_id)))
			android_config_driver.bmAttributes |=
				USB_CONFIG_ATT_WAKEUP;
		else
			android_config_driver.bmAttributes &=
				~USB_CONFIG_ATT_WAKEUP;

		device_desc.idProduct = __constant_cpu_to_le16(product_id);
		
		if (dev->cdev)
			dev->cdev->desc.idProduct = device_desc.idProduct;
		usb_composite_force_reset(dev->cdev);
	}
	return 0;
}

#ifdef CONFIG_USB_SUPPORT_LGE_ANDROID_AUTORUN
static int android_set_usermode(const char *val, struct kernel_param *kp)
{
	int ret = 0;
	unsigned long tmp;

	ret = strict_strtoul(val, 16, &tmp);
	if (ret)
		return ret;

	autorun_user_mode = (unsigned int)tmp;
	pr_info("autorun user mode : %d\n", autorun_user_mode);

	return ret;
}

int get_autorun_user_mode(void)
{
	return autorun_user_mode;
}
EXPORT_SYMBOL(get_autorun_user_mode);
#endif

#ifdef CONFIG_LGE_USB_GADGET_DRIVER
int android_set_pid(const char *val, struct kernel_param *kp)
{
	int ret = 0;
	unsigned long tmp;
	
	ret = strict_strtoul(val, 16, &tmp);
	if (ret)
		goto out;

	/* We come here even before android_probe, when product id
	 * is passed via kernel command line.
	 */
	if (!_android_dev) {
		device_desc.idProduct = tmp;
		goto out;
	}

#ifdef CONFIG_LGE_USB_GADGET_NDIS_DRIVER
	if (device_desc.idProduct == tmp) {
		pr_info("[%s] Requested product id is same(%lx), ignore it\n", __func__, tmp);
		goto out;
	}
#endif

#ifdef CONFIG_LGE_USB_GADGET_SUPPORT_FACTORY_USB
	/* If cable is factory cable, we ignore request from user space */
	if (device_desc.idProduct == LGE_FACTORY_USB_PID && lg_manual_test_mode) {
		pr_info("[%s] Factory USB cable is connected, ignore it\n", __func__);
		goto out;
	}
#endif
	set_pid_flag = 1;

	mutex_lock(&_android_dev->lock);
	pr_info("[%s] user set product id - %lx begin\n", __func__, tmp);
	ret = android_switch_composition(tmp);
	pr_info("[%s] user set product id - %lx complete\n", __func__, tmp);
	mutex_unlock(&_android_dev->lock);
out:
	return ret;
}

static int android_get_pid(char *buffer, struct kernel_param *kp)
{
	int ret;
    pr_debug("[%s] get product id - %x\n", __func__, device_desc.idProduct);
	mutex_lock(&_android_dev->lock);
	ret = sprintf(buffer, "%x", device_desc.idProduct);
	mutex_unlock(&_android_dev->lock);
	return ret;
}

#ifdef CONFIG_USB_SUPPORT_LGE_ANDROID_AUTORUN
int android_set_usb_mode(const char *val, struct kernel_param *kp)
{
	int ret = 0;
	memset(usb_mode, 0, MAX_USB_MODE_LEN);
	pr_info("[%s] request connection mode : [%s]\n", __func__,val);

	if (strstr(val, "charge_only")) {
		strcpy(usb_mode, "charge_only");
		ret = android_set_pid(lg_charge_only_pid_string, NULL);
		return ret;
	}
	else if (strstr(val, "mass_storage")) {
		strcpy(usb_mode, "mass_storage");
		ret = android_set_pid(lg_ums_pid_string, NULL);
		return ret;
	}
	else if (strstr(val, "windows_media_sync")) {
		strcpy(usb_mode, "windows_media_sync");
		ret = android_set_pid(lg_mtp_pid_string, NULL);
		return ret;
	}
	else if (strstr(val, "internet_connection")) {
		strcpy(usb_mode, "internet_connection");
		ret = android_set_pid(lg_default_pid_string, NULL);
		return ret;
	}
	else if (strstr(val, "auto_run")) {
		strcpy(usb_mode, "auto_run");
		ret = android_set_pid(lg_autorun_pid_string, NULL);
		return ret;
	}
	else {
		pr_info("[%s] undefined connection mode, ignore it : [%s]\n", __func__,val);
		return -EINVAL;
	}
}

static int android_get_usb_mode(char *buffer, struct kernel_param *kp)
{
	int ret;
	pr_info("[%s] get usb connection mode\n", __func__, usb_mode);
	mutex_lock(&_android_dev->lock);
	ret = sprintf(buffer, "%s", usb_mode);
	mutex_unlock(&_android_dev->lock);
	return ret;
}
#endif

u16 android_get_product_id(void)
{
    if(device_desc.idProduct != 0x0000 && device_desc.idProduct != 0x0001)
    {
      return device_desc.idProduct;
    }
	else
	{
	  USB_DBG("LG_FW : product_id is not initialized : device_desc.idProduct = 0x%x\n", device_desc.idProduct);
	  return lg_default_pid;
	}
}
	  
#endif /* CONFIG_LGE_USB_GADGET_DRIVER */



#ifdef CONFIG_DEBUG_FS
static int android_debugfs_open(struct inode *inode, struct file *file)
{
	file->private_data = inode->i_private;
	return 0;
}

static ssize_t android_debugfs_serialno_write(struct file *file, const char
				__user *buf,	size_t count, loff_t *ppos)
{
	char str_buf[MAX_STR_LEN];

	if (count > MAX_STR_LEN)
		return -EFAULT;

	if (copy_from_user(str_buf, buf, count))
		return -EFAULT;

	memcpy(serial_number, str_buf, count);

	if (serial_number[count - 1] == '\n')
		serial_number[count - 1] = '\0';

	strings_dev[STRING_SERIAL_IDX].s = serial_number;

	return count;
}
const struct file_operations android_fops = {
	.open	= android_debugfs_open,
	.write	= android_debugfs_serialno_write,
};

struct dentry *android_debug_root;
struct dentry *android_debug_serialno;

static int android_debugfs_init(struct android_dev *dev)
{
	android_debug_root = debugfs_create_dir("android", NULL);
	if (!android_debug_root)
		return -ENOENT;

	android_debug_serialno = debugfs_create_file("serial_number", 0222,
						android_debug_root, dev,
						&android_fops);
	if (!android_debug_serialno) {
		debugfs_remove(android_debug_root);
		android_debug_root = NULL;
		return -ENOENT;
	}
	return 0;
}

static void android_debugfs_cleanup(void)
{
       debugfs_remove(android_debug_serialno);
       debugfs_remove(android_debug_root);
}
#endif


#ifdef CONFIG_LGE_USB_GADGET_SUPPORT_FACTORY_USB
extern int msm_get_manual_test_mode(void);

static int android_get_manual_test_mode(char *buffer, struct kernel_param *kp)
{
	int ret;
    pr_debug("[%s] get manual test mode - %d\n", __func__, lg_manual_test_mode);
	mutex_lock(&_android_dev->lock);
	ret = sprintf(buffer, "%d", lg_manual_test_mode);
	mutex_unlock(&_android_dev->lock);
	return ret;
}
#endif


static int __devinit android_probe(struct platform_device *pdev)
{
	struct android_usb_platform_data *pdata = pdev->dev.platform_data;
	struct android_dev *dev = _android_dev;
	int result;
#ifdef CONFIG_LGE_USB_GADGET_SUPPORT_FACTORY_USB
	/* BEGIN:0010739 [yk.kim@lge.com] 2010-11-11 */
	/* ADD:0010739 init fatory usb switch composition */
	extern int get_msm_cable_type(void);

#ifdef CONFIG_LGE_USB_GADGET_SUPPORT_FACTORY_USB
		/* read NV manual_test_mode for set force Factory USB */
		lg_manual_test_mode = msm_get_manual_test_mode();
#endif
	cable = get_msm_cable_type();
	if (cable==LG_FACTORY_CABLE_56K_TYPE || cable==LG_FACTORY_CABLE_130K_TYPE || cable==LG_FACTORY_CABLE_910K_TYPE)
	{
		pdev->dev.platform_data = &android_usb_pdata_factory;
		pdata = pdev->dev.platform_data;
		switch_cable = 1;
	}
	/* END:0010739 [yk.kim@lge.com] 2010-11-11 */
#endif

	dev_dbg(&pdev->dev, "%s: pdata: %p\n", __func__, pdata);

	pm_runtime_set_active(&pdev->dev);
	pm_runtime_enable(&pdev->dev);

	result = pm_runtime_get(&pdev->dev);
	if (result < 0) {
		dev_err(&pdev->dev,
			"Runtime PM: Unable to wake up the device, rc = %d\n",
			result);
		return result;
	}

	if (pdata) {
		dev->products = pdata->products;
		dev->num_products = pdata->num_products;
		dev->functions = pdata->functions;
		dev->num_functions = pdata->num_functions;
#ifdef CONFIG_LGE_USB_GADGET_FUNC_BIND_ONLY_INIT
		dev->unique_function = pdata->unique_function;
#endif
		if (pdata->vendor_id)
			device_desc.idVendor =
				__constant_cpu_to_le16(pdata->vendor_id);
		if (pdata->product_id) {
			dev->product_id = pdata->product_id;
#ifdef CONFIG_LGE_USB_GADGET_DRIVER
			product_id = pdata->product_id;
#endif
			device_desc.idProduct =
				__constant_cpu_to_le16(pdata->product_id);
		}
		if (pdata->version)
			dev->version = pdata->version;

		if (pdata->product_name)
			strings_dev[STRING_PRODUCT_IDX].s = pdata->product_name;
		if (pdata->manufacturer_name)
			strings_dev[STRING_MANUFACTURER_IDX].s =
					pdata->manufacturer_name;
#ifdef CONFIG_LGE_USB_GADGET_SUPPORT_FACTORY_USB
		strings_dev[STRING_SERIAL_IDX].s = serial_number;
		
		if (pdata->serial_number)
			sprintf(user_serial_number, "%s", pdata->serial_number);
#else
		if (pdata->serial_number)
			strings_dev[STRING_SERIAL_IDX].s = pdata->serial_number;
#endif
	}
#ifdef CONFIG_DEBUG_FS
	result = android_debugfs_init(dev);
	if (result)
		pr_debug("%s: android_debugfs_init failed\n", __func__);
#endif
	return usb_composite_probe(&android_usb_driver, android_bind);
}

static int andr_runtime_suspend(struct device *dev)
{
	dev_dbg(dev, "pm_runtime: suspending...\n");
	return 0;
}

static int andr_runtime_resume(struct device *dev)
{
	dev_dbg(dev, "pm_runtime: resuming...\n");
	return 0;
}

static struct dev_pm_ops andr_dev_pm_ops = {
	.runtime_suspend = andr_runtime_suspend,
	.runtime_resume = andr_runtime_resume,
};

static struct platform_driver android_platform_driver = {
	.driver = { .name = "android_usb", .pm = &andr_dev_pm_ops},
};

static int __init init(void)
{
	struct android_dev *dev;

	pr_debug("android init\n");

	dev = kzalloc(sizeof(*dev), GFP_KERNEL);
	if (!dev)
		return -ENOMEM;

	/* set default values, which should be overridden by platform data */
	dev->product_id = PRODUCT_ID;
	_android_dev = dev;

	return platform_driver_probe(&android_platform_driver, android_probe);
}
module_init(init);

static void __exit cleanup(void)
{
#ifdef CONFIG_DEBUG_FS
	android_debugfs_cleanup();
#endif
	usb_composite_unregister(&android_usb_driver);
	platform_driver_unregister(&android_platform_driver);
	kfree(_android_dev);
	_android_dev = NULL;
}
module_exit(cleanup);
