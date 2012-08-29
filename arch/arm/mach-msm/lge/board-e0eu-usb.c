#include <linux/init.h>

#include <mach/board.h>
#include <mach/msm_iomap.h>
#include <mach/msm_hsusb.h>
#include <mach/rpc_hsusb.h>
#include <mach/usbdiag.h>

#include <linux/usb/android_composite.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <mach/vreg.h>

#include "devices.h"
#include "devices-msm7x2xa.h"
#include "pm.h"

#include <mach/board_lge.h>

#include "board-e0eu.h"
#ifdef CONFIG_LGE_USB_GADGET_DRIVER

char *usb_functions_lge_all[] = {

	"acm",
	"diag",
	"rndis",
#ifdef CONFIG_USB_SUPPORT_LGE_ANDROID_AUTORUN
	"usb_cdrom_storage",
	"mtp",
#endif
	"usb_mass_storage",
	"adb",
};

#ifdef LGE_TRACFONE
/* LGE_CHANGE
 * Add PID 61FA for E0 Tracfone model
 * 2012-01-13 junghyun.cha@lge.com
 */
char *usb_functions_lge_android_plat[] = {
	"acm",
	"diag",
	"usb_mass_storage",
};

char *usb_functions_lge_android_plat_adb[] = {
	"acm",
	"diag",
	"usb_mass_storage",
	"adb",
};
#endif

static char *usb_functions_ndis[] = {
	//"acm",
	//"diag",
	"rndis",
	//"usb_mass_storage",
};

static char *usb_functions_ndis_adb[] = {
	//"acm",
	//"diag",
	"rndis",
	//"usb_mass_storage",
	"adb",
};

/* LG Manufacturing mode */
char *usb_functions_lge_manufacturing[] = {
	"acm", "diag",
};

/* Mass storage only mode */
char *usb_functions_lge_mass_storage_only[] = {
	"usb_mass_storage",
};

/* MTP */
char *usb_functions_lge_mtp_only[] = {
	"mtp",
};
char *usb_functions_lge_mtp_adb[] = {
	"mtp",
	"adb",
};

#ifdef CONFIG_USB_SUPPORT_LGE_ANDROID_AUTORUN
/* CDROM storage only mode(Autorun default mode) */
char *usb_functions_lge_cdrom_storage_only[] = {
	"usb_cdrom_storage",
};

char *usb_functions_lge_cdrom_storage_adb[] = {
	"usb_cdrom_storage", "adb",
};

char *usb_functions_lge_charge_only[] = {
	"charge_only",
};
#endif

struct android_usb_product usb_products[] = {
#ifdef LGE_TRACFONE
	/* LGE_CHANGE
	 * Add PID 61FA for E0 Tracfone model
	 * 2012-01-12 junghyun.cha@lge.com
	 */
	{
		.product_id = 0x61FA,
		.num_functions = ARRAY_SIZE(usb_functions_lge_android_plat),
		.functions = usb_functions_lge_android_plat,
	},
	{
		.product_id = 0x61FA,
		.num_functions = ARRAY_SIZE(usb_functions_lge_android_plat_adb),
		.functions = usb_functions_lge_android_plat_adb,
	},
#endif
	{
		.product_id = 0x61DA,
		.num_functions = ARRAY_SIZE(usb_functions_ndis),
		.functions = usb_functions_ndis,
	},
	{
		.product_id = 0x61D9,
		.num_functions = ARRAY_SIZE(usb_functions_ndis_adb),
		.functions = usb_functions_ndis_adb,
	},
	{
		.product_id = 0x6000,
		.num_functions = ARRAY_SIZE(usb_functions_lge_manufacturing),
		.functions = usb_functions_lge_manufacturing,
	},
	{
		.product_id = 0x61F9,
		.num_functions = ARRAY_SIZE(usb_functions_lge_mtp_adb),
		.functions = usb_functions_lge_mass_storage_only,
	},
	{
		.product_id = 0x631C,
		.num_functions = ARRAY_SIZE(usb_functions_lge_mtp_only),
		.functions = usb_functions_lge_mass_storage_only,
	},
	{
		.product_id = 0x61C5,
		.num_functions = ARRAY_SIZE(usb_functions_lge_mass_storage_only),
		.functions = usb_functions_lge_mass_storage_only,
	},
#ifdef CONFIG_USB_SUPPORT_LGE_ANDROID_AUTORUN
	{
		/* FIXME: This pid is just for test */
		.product_id = 0x91C8,
		.num_functions = ARRAY_SIZE(usb_functions_lge_cdrom_storage_only),
		.functions = usb_functions_lge_cdrom_storage_only,
	},
	{
		.product_id = 0x61A6,
		.num_functions = ARRAY_SIZE(usb_functions_lge_cdrom_storage_adb),
		.functions = usb_functions_lge_cdrom_storage_adb,
	},
	{
		/* Charge only doesn't have no specific pid */
		.product_id = 0xFFFF,
		.num_functions = ARRAY_SIZE(usb_functions_lge_charge_only),
		.functions = usb_functions_lge_charge_only,
	},
#endif

};

#else	/* Qualcomm Original*/

static char *usb_functions_default[] = {
	"diag",
	"modem",
	"nmea",
	"rmnet",
	"usb_mass_storage",
};

static char *usb_functions_default_adb[] = {
	"diag",
	"adb",
	"modem",
	"nmea",
	"rmnet",
	"usb_mass_storage",
};

static char *usb_functions_rndis[] = {
	"rndis",
};

static char *usb_functions_rndis_adb[] = {
	"rndis",
	"adb",
};

static char *usb_functions_all[] = {
#ifdef CONFIG_USB_ANDROID_RNDIS
	"rndis",
#endif
#ifdef CONFIG_USB_ANDROID_DIAG
	"diag",
#endif
	"adb",
#ifdef CONFIG_USB_F_SERIAL
	"modem",
	"nmea",
#endif
#ifdef CONFIG_USB_ANDROID_RMNET
	"rmnet",
#endif
	"usb_mass_storage",
};

static struct android_usb_product usb_products[] = {
	{
		.product_id = 0x9025,
		.num_functions	= ARRAY_SIZE(usb_functions_default_adb),
		.functions	= usb_functions_default_adb,
	},
	{
		.product_id 	= 0x9026,
		.num_functions	= ARRAY_SIZE(usb_functions_default),
		.functions		= usb_functions_default,
	},
	
	{
		.product_id	= 0xf00e,
		.num_functions	= ARRAY_SIZE(usb_functions_rndis),
		.functions	= usb_functions_rndis,
	},
	{
		.product_id	= 0x9024,
		.num_functions	= ARRAY_SIZE(usb_functions_rndis_adb),
		.functions	= usb_functions_rndis_adb,
	},
};
#endif	/*CONFIG_LGE_USB_GADGET_DRIVER*/



#ifdef CONFIG_LGE_USB_GADGET_DRIVER

struct usb_mass_storage_platform_data mass_storage_pdata = {
	.nluns      = 2,
	.vendor     = "LGE",
	.product    = "Android Platform",
	.release    = 0x0100,
};

struct platform_device usb_mass_storage_device = {
	.name   = "usb_mass_storage",
	.id 	= -1,
	.dev    = {
		.platform_data = &mass_storage_pdata,
	},
};

#else	/*Qualcomm Original*/
static struct usb_mass_storage_platform_data mass_storage_pdata = {
	.nluns		= 1,
	.vendor		= "Qualcomm Incorporated",
	.product	= "Mass storage",
	.release	= 0x0100,

};

static struct platform_device usb_mass_storage_device = {
	.name	= "usb_mass_storage",
	.id	= -1,
	.dev	= {
		.platform_data = &mass_storage_pdata,
	},
};

#endif	/*Qualcomm Original*/



#ifdef CONFIG_USB_ANDROID_CDC_ECM
struct usb_ether_platform_data ecm_pdata = {
	/* ethaddr is filled by board_serialno_setup */
	.vendorID   	= 0x1004,
	.vendorDescr    = "LG Electronics Inc.",
};

struct platform_device ecm_device = {
	.name   = "ecm",
	.id 	= -1,
	.dev    = {
		.platform_data = &ecm_pdata,
	},
};
#endif

#ifdef CONFIG_USB_ANDROID_ACM
struct acm_platform_data acm_pdata = {
	.num_inst	    = 1,
};

struct platform_device acm_device = {
	.name   = "acm",
	.id 	= -1,
	.dev    = {
		.platform_data = &acm_pdata,
	},
};
#endif

#ifdef CONFIG_USB_SUPPORT_LGE_ANDROID_AUTORUN
struct usb_cdrom_storage_platform_data cdrom_storage_pdata = {
	.nluns      = 1,
	.vendor     = "LGE",
	.product    = "Android Platform",
	.release    = 0x0100,
};

struct platform_device usb_cdrom_storage_device = {
	.name   = "usb_cdrom_storage",
	.id = -1,
	.dev    = {
		.platform_data = &cdrom_storage_pdata,
	},
};
#endif



#ifdef CONFIG_LGE_USB_GADGET_DRIVER
struct android_usb_platform_data android_usb_pdata = {
#ifdef LGE_TRACFONE
	/* LGE_CHANGE
	 * Add PID 61FA for E0 Tracfone model
	 * 2012-01-12 junghyun.cha@lge.com
	 */
	.vendor_id  = 0x1004,
	.product_id = 0x61FA,
	.version    = 0x0100,
	.product_name       = "LGE Android Phone",
	.manufacturer_name  = "LG Electronics Inc.",
	.num_products = ARRAY_SIZE(usb_products),
	.products = usb_products,
	.num_functions = ARRAY_SIZE(usb_functions_lge_all),
	.functions = usb_functions_lge_all,
	.serial_number = "LG_ANDROID_E0TRF_US_",
#else
	.vendor_id  = 0x1004,
	.product_id = 0x61F9,
	.version    = 0x0100,
	.product_name       = "LGE Android Phone",
	.manufacturer_name  = "LG Electronics Inc.",
	.num_products = ARRAY_SIZE(usb_products),
	.products = usb_products,
	.num_functions = ARRAY_SIZE(usb_functions_lge_all),
	.functions = usb_functions_lge_all,
	.serial_number = "LG_ANDROID_E0OPEN_GB_",
#endif
};
#else
static struct android_usb_platform_data android_usb_pdata = {
	.vendor_id	= 0x05C6,
	.product_id	= 0x9026,
	.version	= 0x0100,
	.product_name	= "Qualcomm HSUSB Device",
	.manufacturer_name = "Qualcomm Incorporated",
	.num_products = ARRAY_SIZE(usb_products),
	.products = usb_products,
	.num_functions = ARRAY_SIZE(usb_functions_all),
	.functions = usb_functions_all,
	.serial_number = "1234567890ABCDEF",
};
#endif

static struct platform_device android_usb_device = {
	.name	= "android_usb",
	.id	= -1,
	.dev	= {
		.platform_data = &android_usb_pdata,
	},
};

//#ifndef CONFIG_LGE_USB_GADGET_DRIVER
static struct usb_ether_platform_data rndis_pdata = {
#ifdef CONFIG_LGE_USB_GADGET_DRIVER
	.vendorID	= 0x1004,
	.vendorDescr	= "LG Electronics Inc.",
#else
	.vendorID	= 0x05C6,
	.vendorDescr	= "Qualcomm Incorporated",
#endif
};

static struct platform_device rndis_device = {
	.name	= "rndis",
	.id	= -1,
	.dev	= {
		.platform_data = &rndis_pdata,
	},
};

static int __init board_serialno_setup(char *serialno)
{
	int i;
	char *src = serialno;

	/* create a fake MAC address from our serial number.
	 * first byte is 0x02 to signify locally administered.
	 */
	rndis_pdata.ethaddr[0] = 0x02;
	for (i = 0; *src; i++) {
		/* XOR the USB serial across the remaining bytes */
		rndis_pdata.ethaddr[i % (ETH_ALEN - 1) + 1] ^= *src++;
	}

	android_usb_pdata.serial_number = serialno;
	return 1;
}
__setup("androidboot.serialno=", board_serialno_setup);

//#endif

#if 0
static int __init board_serialno_setup(char *serialno)
{
	int i;
	char *src = serialno;

	/* create a fake MAC address from our serial number.
	 * first byte is 0x02 to signify locally administered.
	 */
	ecm_pdata.ethaddr[0] = 0x02;
	for (i = 0; *src; i++) {
		/* XOR the USB serial across the remaining bytes */
		ecm_pdata.ethaddr[i % (ETH_ALEN - 1) + 1] ^= *src++;
	}

	android_usb_pdata.serial_number = serialno;
	return 1;
}
__setup("androidboot.serialno=", board_serialno_setup);

#endif


#ifdef CONFIG_USB_EHCI_MSM_72K
static void msm_hsusb_vbus_power(unsigned phy_info, int on)
{
	int rc = 0;
	unsigned gpio;

	gpio = GPIO_HOST_VBUS_EN;

	rc = gpio_request(gpio, "i2c_host_vbus_en");
	if (rc < 0) {
		pr_err("failed to request %d GPIO\n", gpio);
		return;
	}
	gpio_direction_output(gpio, !!on);
	gpio_set_value_cansleep(gpio, !!on);
	gpio_free(gpio);
}

static struct msm_usb_host_platform_data msm_usb_host_pdata = {
	.phy_info       = (USB_PHY_INTEGRATED | USB_PHY_MODEL_45NM),
};

static void __init msm7x2x_init_host(void)
{
	msm_add_host(0, &msm_usb_host_pdata);
}
#endif

#ifdef CONFIG_USB_MSM_OTG_72K
static int hsusb_rpc_connect(int connect)
{
	if (connect)
		return msm_hsusb_rpc_connect();
	else
		return msm_hsusb_rpc_close();
}

static struct vreg *vreg_3p3;
static int msm_hsusb_ldo_init(int init)
{
	if (init) {
		vreg_3p3 = vreg_get(NULL, "usb");
		if (IS_ERR(vreg_3p3))
			return PTR_ERR(vreg_3p3);
	} else
		vreg_put(vreg_3p3);

	return 0;
}

static int msm_hsusb_ldo_enable(int enable)
{
	static int ldo_status;

	if (!vreg_3p3 || IS_ERR(vreg_3p3))
		return -ENODEV;

	if (ldo_status == enable)
		return 0;

	ldo_status = enable;

	if (enable)
		return vreg_enable(vreg_3p3);

	return vreg_disable(vreg_3p3);
}

#ifndef CONFIG_USB_EHCI_MSM_72K
static int msm_hsusb_pmic_notif_init(void (*callback)(int online), int init)
{
	int ret = 0;

	if (init)
		ret = msm_pm_app_rpc_init(callback);
	else
		msm_pm_app_rpc_deinit(callback);

	return ret;
}
#endif

static struct msm_otg_platform_data msm_otg_pdata = { 
#ifndef CONFIG_USB_EHCI_MSM_72K
	.pmic_vbus_notif_init	 = msm_hsusb_pmic_notif_init,
#else
	.vbus_power		 = msm_hsusb_vbus_power,
#endif
	.rpc_connect		 = hsusb_rpc_connect,
	.core_clk		 = 1,
	.pemp_level		 = PRE_EMPHASIS_WITH_20_PERCENT,
	.cdr_autoreset		 = CDR_AUTO_RESET_DISABLE,
	.drv_ampl		 = HS_DRV_AMPLITUDE_DEFAULT,
	.se1_gating		 = SE1_GATING_DISABLE,
	.ldo_init		 = msm_hsusb_ldo_init,
	.ldo_enable		 = msm_hsusb_ldo_enable,
	.chg_init		 = hsusb_chg_init,
	.chg_connected		 = hsusb_chg_connected,
	.chg_vbus_draw		 = hsusb_chg_vbus_draw,
};
#endif

static struct msm_pm_platform_data msm7x27a_pm_data[MSM_PM_SLEEP_MODE_NR] = {
	[MSM_PM_SLEEP_MODE_POWER_COLLAPSE] = {
					.idle_supported = 1,
					.suspend_supported = 1,
					.idle_enabled = 1,
					.suspend_enabled = 1,
					.latency = 16000,
					.residency = 20000,
	},
	[MSM_PM_SLEEP_MODE_POWER_COLLAPSE_NO_XO_SHUTDOWN] = {
					.idle_supported = 1,
					.suspend_supported = 1,
					.idle_enabled = 1,
					.suspend_enabled = 1,
					.latency = 12000,
					.residency = 20000,
	},
	[MSM_PM_SLEEP_MODE_RAMP_DOWN_AND_WAIT_FOR_INTERRUPT] = {
					.idle_supported = 1,
					.suspend_supported = 1,
					.idle_enabled = 0,
					.suspend_enabled = 1,
					.latency = 2000,
					.residency = 0,
	},
	[MSM_PM_SLEEP_MODE_WAIT_FOR_INTERRUPT] = {
					.idle_supported = 1,
					.suspend_supported = 1,
					.idle_enabled = 1,
					.suspend_enabled = 1,
					.latency = 2,
					.residency = 0,
	},
};


static struct platform_device *e0eu_usb_devices[] __initdata = {
	&msm_device_otg,
	&msm_device_gadget_peripheral,
	&usb_mass_storage_device,
//#ifndef CONFIG_LGE_USB_GADGET_DRIVER
	&rndis_device,
//#endif	
	&usb_diag_device,
#ifdef CONFIG_USB_ANDROID_CDC_ECM
	&ecm_device,
#endif
#ifdef CONFIG_USB_ANDROID_ACM
	&acm_device,
#endif
#ifdef CONFIG_USB_SUPPORT_LGE_ANDROID_AUTORUN
	&usb_cdrom_storage_device,
#endif
	&android_usb_device,
};

void __init lge_add_usb_devices(void)
{

#ifdef CONFIG_USB_MSM_OTG_72K
	msm_otg_pdata.swfi_latency =
		msm7x27a_pm_data
		[MSM_PM_SLEEP_MODE_RAMP_DOWN_AND_WAIT_FOR_INTERRUPT].latency;
	msm_device_otg.dev.platform_data = &msm_otg_pdata;
#endif
	platform_add_devices(e0eu_usb_devices, ARRAY_SIZE(e0eu_usb_devices));
#ifdef CONFIG_USB_EHCI_MSM_72K
	msm7x2x_init_host();
#endif
	msm_pm_set_platform_data(msm7x27a_pm_data,
			ARRAY_SIZE(msm7x27a_pm_data));
}

