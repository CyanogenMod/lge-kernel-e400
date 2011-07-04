/* linux/drivers/usb/gadget/u_lgeusb.h
 *
 * Copyright (C) 2008 Google, Inc.
 * Copyright (C) 2010 LGE.
 * Author : Young Kyoung KIM <yk.kim@lge.com>
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

#ifndef __U_LGEUSB_H__
#define __U_LGEUSB_H__

/* Common Interface */
int lge_get_usb_serial_number(char *serial_number);
int lge_detect_factory_cable(void);

/* BEGIN:0010319 [yk.kim@lge.com] 2010-10-29 */
/* MOD:0010319 Product ID 0x61CF set for Voltmicro USB Driver.
0x618E : LGE Android Platform Driver (CDMA)
0x61CF : Bryce CDMA/LTE Driver */
#define LGE_DEFAULT_PID			0x61CF
/* #define LGE_DEFAULT_PID		0x618E */
/* #define LGE_DEFAULT_PID		0x9018 */
/* END:0010319 [yk.kim@lge.com] 2010-10-29 */

#define LGE_FACTORY_USB_PID 	    0x6000
#define LGE_FACTORY_USB_PID_STRING  "0x6000"

#define LG_UNKNOWN_CABLE			0
#define LG_WALL_CHARGER_CABLE		1
#define LG_NORMAL_USB_CABLE			2
#define LG_FACTORY_CABLE_56K_TYPE	3
#define LG_FACTORY_CABLE_130K_TYPE	4
#define LG_FACTORY_CABLE_910K_TYPE	5
#define LG_RESERVED1_CABLE			6
#define LG_RESERVED2_CABLE			7
#define LG_NONE_CABLE				8

/* DEBUG MSG USB Gadget */
#define USB_GADGET_DEBUG_PRINT 0

#if USB_GADGET_DEBUG_PRINT
#define USB_DBG(fmt, args...) \
             printk(KERN_INFO "USB_DBG[%-7s:%3d]" \
                 fmt, __FUNCTION__, __LINE__, ##args);
#else
#define USB_DBG(fmt, args...)    {};
#endif

#endif /* __U_LGE_USB_H__ */
