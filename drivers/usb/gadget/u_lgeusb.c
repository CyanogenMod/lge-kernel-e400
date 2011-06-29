/* linux/drivers/usb/gadget/u_lgeusb.c
 *
 * Copyright (C) 2008 Google, Inc.
 * Copyright (C) 2010 LGE.
 * Author : Hyeon H. Park <hyunhui.park@lge.com>
 *			Youn Suk Song <younsuk.song@lge.com>
 *
 * This software is licensed under the terms of the GNU General Public
 * License version 2, as published by the Free Software Foundation, and
 * may be copied, distributed, and modified under those terms.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/delay.h>
#include <mach/msm_hsusb.h>
#include <mach/rpc_hsusb.h>
#ifdef CONFIG_USB_ANDROID
#include <linux/usb/android.h>
#endif
#include <mach/board.h>

#include "u_lgeusb.h"

/* FIXME: This length must be same as MAX_SERIAL_LEN in android.c */
#define MAX_SERIAL_NO_LEN 256
extern int msm_chg_LG_cable_type(void);

#if 0 //hyunjin.lim for compile. 아직 bsp 작업 안됨 ?? 
extern void msm_get_MEID_type(char* sMeid);
#endif
static int do_get_usb_serial_number(char *serial_number)
{
	memset(serial_number, 0, MAX_SERIAL_NO_LEN);
#if 0 //hyunjin.lim for compile. 아직 bsp 작업 안됨 ?? 
	msm_get_MEID_type(serial_number);
#endif	
	printk(KERN_ERR "LG_FW :: %s Serail number %s \n",__func__, serial_number);

	if(!strcmp(serial_number,"00000000000000")) 
		serial_number[0] = '\0';
#if 0
	if(msm_chg_LG_cable_type() == LT_ADB_CABLE)
	{
		sprintf(serial_number,"%s","LGE_ANDROID_DE");
	}
#endif		

	return 0;
}

static int do_detect_factory_cable(void)
{
	int cable_type = 0;
	#if 0 //hyunjin.lim not used ??
	int mdm_cable_type =  msm_chg_LG_cable_type();
	#endif

	switch(cable_type)
	{
		case 0: //NOINIT_CABLE
		case 9: //NO_CABLE
			cable_type = LG_NONE_CABLE;
			break;
		case 1: //UNKNOWN_CABLE
			cable_type = LG_UNKNOWN_CABLE;
			break;
		case 2: //TA_CABLE
		case 5: // FORGED_TA_CABLE
		case 8: // C1A_TA_CABLE
			cable_type = LG_WALL_CHARGER_CABLE;
			break;
		case 3: // LT_CABLE
			cable_type = LG_FACTORY_CABLE_56K_TYPE;
			break;
		case 4: // USB_CABLE
		case 6: //ABNORMAL_USB_CABLE
		case 7 : //ABNORMAL_USB_400c_CABLE
			cable_type = LG_NORMAL_USB_CABLE;
			break;
		case 10: // LT_CABLE_130K
			cable_type = LG_FACTORY_CABLE_130K_TYPE;
			break;
		case 11: // LT_CABLE_910K
			cable_type = LG_FACTORY_CABLE_910K_TYPE;
			break;
		default:
			cable_type = LG_NONE_CABLE;
	}

	if((cable_type == LG_FACTORY_CABLE_910K_TYPE) ||
		(cable_type == LG_FACTORY_CABLE_130K_TYPE)||
		(cable_type == LG_FACTORY_CABLE_56K_TYPE ))
		return 1;
	else
		return 0;
}

/* Common Interface */

/*
 * lge_detect_factory_cable
 *
 * If factory cable (PIF or LT) is connected,
 * return 1, otherwise return 0.
 *
 */
int lge_detect_factory_cable(void)
{
	return do_detect_factory_cable();
}
EXPORT_SYMBOL(lge_detect_factory_cable);

/*
 * lge_get_usb_serial_number
 *
 * Get USB serial number from ARM9 using RPC or RAPI.
 * return -1 : If any error
 * return 0 : success
 *
 */
int lge_get_usb_serial_number(char *serial_number)
{
	return do_get_usb_serial_number(serial_number);
}
EXPORT_SYMBOL(lge_get_usb_serial_number);
