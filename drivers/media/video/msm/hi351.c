/* Copyright (c) 2009, Code Aurora Forum. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 *
 */

#include <linux/delay.h>
#include <linux/debugfs.h>
#include <linux/types.h>
#include <linux/i2c.h>
#include <linux/uaccess.h>
#include <linux/miscdevice.h>
#include <linux/slab.h>
#include <linux/gpio.h>
#include <linux/bitops.h>
#include <mach/camera.h>
#include <media/msm_camera.h>
#include "hi351.h"


#define SENSOR_DEBUG 0

#define HI351_REG_MODEL_ID 		 0x04 /*Chip ID read register*/
#define HI351_MODEL_ID     		 0xA4 /*Hynix HI351 Chip ID*/ 


struct hi351_work {
	struct work_struct work;
};

static struct  hi351_work *hi351_sensorw;
static struct  i2c_client *hi351_client;
static bool CONFIG_CSI;
static bool DELAY_START;

struct hi351_ctrl_t {
	const struct msm_camera_sensor_info *sensordata;
};

static struct hi351_ctrl_t *hi351_ctrl;

static DECLARE_WAIT_QUEUE_HEAD(hi351_wait_queue);
DEFINE_MUTEX(hi351_mut);

static int prev_effect_mode;
static int prev_balance_mode;
static int prev_iso_mode;
static int prev_scene_mode;
static int prev_fps_mode;



/*=============================================================*/

static int hi351_i2c_rxdata(u16 saddr,
	u8 *rxdata, int length)
{
	struct i2c_msg msg[] = {
	{
		.addr   = saddr << 1,
		.flags = 0,
		.len   = 1,
		.buf   = rxdata,
	},
	{
		.addr   = saddr << 1,
		.flags = I2C_M_RD,
		.len   = length,
		.buf   = rxdata,
	},
	};

#if SENSOR_DEBUG
	pr_err("hi351_i2c_rxdata(1): addr : 0x%x len : %d buf: 0x%x\n", msg[0].addr, msg[0].len, *(msg[0].buf));
	pr_err("hi351_i2c_rxdata(2): addr : 0x%x len : %d buf: 0x%x\n", msg[1].addr, msg[1].len, *(msg[1].buf));
#endif

	if (i2c_transfer(hi351_client->adapter, msg, 2) < 0) {
		pr_err("hi351_i2c_rxdata failed!\n");
		return -EIO;
	}

	return 0;
}

static int32_t hi351_i2c_read(unsigned short   saddr,
	u8 raddr, u8 *rdata)
{
	int32_t rc = 0;
	u8 buf[1];

	if (!rdata)
		return -EIO;

	memset(buf, 0, sizeof(buf));


	buf[0] = raddr;

	rc = hi351_i2c_rxdata(saddr, buf, 1);
	if (rc < 0)
		return rc;
	*rdata = buf[0];

	if (rc < 0)
		pr_err("hi351_i2c_read failed!\n");

	return rc;
}

static int32_t hi351_i2c_txdata(u16 saddr,
	u8 *txdata, int length)
{
	struct i2c_msg msg[] = {
		{
			.addr = saddr << 1,
			.flags = 0,
			.len = length,
			.buf = txdata,
		},
	};

#if SENSOR_DEBUG
	pr_err("hi351_i2c_txdata: addr : 0x%x len : %d buf[0]: 0x%x\n", msg[0].addr, msg[0].len, *(msg[0].buf));

#endif
	if (i2c_transfer(hi351_client->adapter, msg, 1) < 0) {
		pr_err("hi351_i2c_txdata: addr : 0x%x len : %d failed\n", msg[0].addr, msg[0].len);
		return -EIO;
	}

	return 0;
}

static int32_t hi351_i2c_write_b_sensor(u8 baddr, u8 bdata)
{
	int32_t rc = -EIO;
	u8 buf[2];
	memset(buf, 0, sizeof(buf));

	if (DELAY_START == 1) {
		if (baddr == 0xFE) {
			msleep(bdata);
		}
		DELAY_START = 0;
		return 0;
	}
	else {
		if (baddr == 0x03 && bdata == 0xFE) {
			DELAY_START = 1;
			return 0;
		}
		else {

			buf[0] = baddr;
			buf[1] = bdata;
			
			rc = hi351_i2c_txdata(hi351_client->addr, buf, 2);


			if (rc < 0)
				pr_err("i2c_write_w failed, addr = 0x%x, val = 0x%x!\n", baddr, bdata);
		}
	}
	return rc;
}

static int32_t hi351_i2c_write_b_table(struct hi351_i2c_reg_conf const *reg_conf_tbl, int num)
{
	int i;
	int32_t rc = 0;
	for (i = 0; i < num; i++) {
			rc = hi351_i2c_write_b_sensor(reg_conf_tbl->baddr,
					reg_conf_tbl->bdata);
			if (rc < 0) {
				pr_err("hi351_i2c_write_b_table fail\n");
				break;
			}
			reg_conf_tbl++;
		}
	CDBG("%s: %d	Exit \n",__func__, __LINE__);
	return rc;

}

static void hi351_start_stream(void)
{
	//int rc = 0;
	CDBG("%s: %d  Enter \n",__func__, __LINE__);

	hi351_i2c_write_b_sensor(0x03, 0x00);
	hi351_i2c_write_b_sensor(0x01, 0xf0);/* streaming on */
	CDBG("%s: %d  Exit \n",__func__, __LINE__);
}

static void hi351_stop_stream(void)
{	
	//int rc = 0;
	CDBG("%s: %d  Enter \n",__func__, __LINE__);

	hi351_i2c_write_b_sensor(0x03, 0x00);
	hi351_i2c_write_b_sensor(0x01, 0xf1);

	CDBG("%s: %d  Exit \n",__func__, __LINE__);
}


static int hi351_probe_init_done(const struct msm_camera_sensor_info *data)
{
	CDBG("%s : hi351 sensor_probe_init_done which sensor initialize failed 0\n", __func__);
	mutex_lock(&hi351_mut);	

	hi351_stop_stream();

	msleep(2);

	hi351_ctrl->sensordata->pdata->camera_power_off();	

	kfree(hi351_ctrl);

	mutex_unlock(&hi351_mut);	
		
	return 0;
}
static long hi351_reg_init(void)
{
#if 0		//NORMAL MODE
	int32_t rc = 0;
	rc = hi351_i2c_write_b_table(hi351_regs.reg_settings, hi351_regs.reg_setting_size);

	if (rc < 0) {
		pr_err("%s: %d  Exit \n",__func__, __LINE__);
 		return rc;
	}
	
	return rc;

#else			//BURST MODE

	int32_t rc = 0;
	int i;
	u8 buf[301];
	int bufIndex = 0;

	memset(buf, 0, sizeof(buf));

	//for burst mode

	for (i = 0; i < hi351_regs.reg_setting_size; i++) {
		if ( hi351_regs.reg_settings[i].register_type == BURST_TYPE && bufIndex < 301 ) {
			if(bufIndex == 0) {
				buf[bufIndex] = hi351_regs.reg_settings[i].baddr;
				bufIndex++;
				buf[bufIndex] = hi351_regs.reg_settings[i].bdata;
				bufIndex++;
			}
			else {
				buf[bufIndex] = hi351_regs.reg_settings[i].bdata;
				bufIndex++;
			}
		}
		else {
			if (bufIndex > 0) {
				rc = hi351_i2c_txdata(hi351_client->addr, buf, bufIndex);
				//pr_err("%s: BurstMODE write bufIndex = %d \n",__func__, bufIndex);
				bufIndex = 0;
				memset(buf, 0, sizeof(buf));
				if (rc < 0) {
					pr_err("%s: %d  failed Exit \n",__func__, __LINE__);
 					return rc;
				}
			}
			rc = hi351_i2c_write_b_sensor(hi351_regs.reg_settings[i].baddr, hi351_regs.reg_settings[i].bdata);
			if (rc < 0) {
				pr_err("%s: %d  failed Exit \n",__func__, __LINE__);
 				return rc;
			}
		}
	}
	return rc;

#endif
}

static int32_t hi351_reg_preview(void)
{
	int32_t rc = 0;
	rc = hi351_i2c_write_b_table(hi351_regs.reg_prev, hi351_regs.reg_prev_size);

	if (rc < 0) {
		pr_err("%s: %d  Exit \n",__func__, __LINE__);
 		return rc;
	}
	
	return rc;
}

static int32_t hi351_reg_snapshot(void)
{
	int32_t rc = 0;
	rc = hi351_i2c_write_b_table(hi351_regs.reg_snap, hi351_regs.reg_snap_size);
	
	if (rc < 0) {
		pr_err("%s: %d  Exit \n",__func__, __LINE__);
 		return rc;
	}
	
	return rc;
}


static int hi351_set_effect(int effect)
{
	int rc = 0;

	if(prev_effect_mode == -1)
	{
		CDBG("[CHECK]%s: skip this function, Previous Init sets \n", __func__);
		prev_effect_mode = effect;
		return rc;
	}

	if(prev_effect_mode == effect)
	{
		CDBG("[CHECK]%s: skip this function, effect_mode -> %d\n", __func__, effect);
		return rc;
	}

       CDBG("[CHECK]%s: mode -> %d\n", __func__, effect);

	   switch (effect) {
	   case CAMERA_EFFECT_OFF:
		   CDBG("Camera Effect OFF\n");
	
		   rc = hi351_i2c_write_b_table(hi351_regs.effect_off_reg_settings,
								   hi351_regs.effect_off_reg_settings_size);
		   break;
	
	   case CAMERA_EFFECT_MONO:
		   CDBG("Camera Effect MONO\n");
	
	
		   rc = hi351_i2c_write_b_table(hi351_regs.effect_mono_reg_settings,
								   hi351_regs.effect_mono_reg_settings_size);
		   break;
	
	   case CAMERA_EFFECT_NEGATIVE:
		   CDBG("Camera Effect NEGATIVE\n");
	
		   rc = hi351_i2c_write_b_table(hi351_regs.effect_negative_reg_settings,
										hi351_regs.effect_negative_reg_settings_size);
		   break;
	
	   case CAMERA_EFFECT_SEPIA:
		   CDBG("Camera Effect SEPIA\n");
	
		   rc = hi351_i2c_write_b_table(hi351_regs.effect_sepia_reg_settings,
								   hi351_regs.effect_sepia_reg_settings_size);
		   break;
	
	   case CAMERA_EFFECT_AQUA:
		   CDBG("Camera Effect AQUA\n");
	
		   rc = hi351_i2c_write_b_table(hi351_regs.effect_aqua_reg_settings,
								   hi351_regs.effect_aqua_reg_settings_size);
		   break;
	
	   default:
		   return -EINVAL;
	}
	
	prev_effect_mode = effect;
	return rc;
}

static int hi351_set_wb(int mode)
{
	int32_t rc = 0;

	if(prev_balance_mode == -1)
	{
		CDBG("[CHECK]%s: skip this function, Previous Init sets \n", __func__);
		prev_balance_mode = mode;
		return rc;
	}

	if(prev_balance_mode == mode)
	{
		CDBG("[CHECK]%s: skip this function, wb_mode -> %d\n", __func__, mode);
		return rc;
	}
       printk(KERN_INFO "### %s: mode -> %d\n", __func__, mode);

	switch (mode) {
	case CAMERA_WB_AUTO:
		CDBG("Camera WB AUTO\n");
		rc = hi351_i2c_write_b_table(hi351_regs.wb_auto_reg_settings,
								hi351_regs.wb_auto_reg_settings_size);
		break;

	case CAMERA_WB_CUSTOM:	/* Do not support */
		break;

	case CAMERA_WB_INCANDESCENT: 

 		CDBG("Camera WB INCANDESCENT\n");

 	 	rc = hi351_i2c_write_b_table(hi351_regs.wb_incandescent_reg_settings,
								hi351_regs.wb_incandescent_reg_settings_size);
 		break;

	case CAMERA_WB_FLUORESCENT: 

		CDBG("Camera WB FLUORESCENT\n");

	 	rc = hi351_i2c_write_b_table(hi351_regs.wb_fluorescent_reg_settings,
								hi351_regs.wb_fluorescent_reg_settings_size);
		break;


	case CAMERA_WB_DAYLIGHT: 
		CDBG("Camera WB DAYLIGHT\n");

		rc = hi351_i2c_write_b_table(hi351_regs.wb_daylight_reg_settings,
								hi351_regs.wb_daylight_reg_settings_size);
		break;
	
	case CAMERA_WB_CLOUDY_DAYLIGHT: 

		CDBG("Camera WB CLOUDY_DAYLIGHT\n");

		rc = hi351_i2c_write_b_table(hi351_regs.wb_cloudy_reg_settings,
								hi351_regs.wb_cloudy_reg_settings_size);
		break;
	

	default:
		return -EINVAL;
	}

	
	prev_balance_mode = mode;
	return rc;
}

static int hi351_set_iso(int mode)
{
	int32_t rc = 0;

	if(prev_iso_mode == -1)
	{
		CDBG("###  [CHECK]%s: skip this function, Previous Init sets \n", __func__);
		prev_iso_mode = mode;
		return rc;
	}

	if(prev_iso_mode == mode)
	{
		CDBG("###  [CHECK]%s: skip this function, iso_mode -> %d\n", __func__, mode);
		return rc;
	}
       CDBG("%s: mode -> %d\n", __func__, mode);

	switch (mode) {
	case CAMERA_ISO_AUTO:
		CDBG("Camera ISO AUTO\n");

		rc = hi351_i2c_write_b_table(hi351_regs.iso_auto_reg_settings,
								hi351_regs.iso_auto_reg_settings_size);
		break;


	case CAMERA_ISO_100:
		CDBG("Camera ISO 100\n");

		rc = hi351_i2c_write_b_table(hi351_regs.iso_100_reg_settings,
								hi351_regs.iso_100_reg_settings_size);
		break;

	case CAMERA_ISO_200:
		CDBG("Camera ISO 200\n");
		
		rc = hi351_i2c_write_b_table(hi351_regs.iso_200_reg_settings,
								hi351_regs.iso_200_reg_settings_size);

		break;

	case CAMERA_ISO_400:
		CDBG("Camera ISO 400\n");
		
		rc = hi351_i2c_write_b_table(hi351_regs.iso_400_reg_settings,
								hi351_regs.iso_400_reg_settings_size);

		break;

	default:
		return -EINVAL;
	}
	
	prev_iso_mode = mode;
	return rc;
}

static long hi351_set_scene_mode(int8_t mode)
{
	int32_t rc = 0;

	if(prev_scene_mode == -1)
	{
		CDBG("[CHECK]%s: skip this function, Previous Init sets \n", __func__);
		prev_scene_mode = mode;
		return rc;
	}

	if(prev_scene_mode == mode)
	{
		CDBG("###  [CHECK]%s: skip this function, scene_mode -> %d\n", __func__, mode);
		return rc;
	}
       
	   printk(KERN_INFO "### %s: mode -> %d\n", __func__, mode);
	
	switch (mode) {
	case CAMERA_SCENE_AUTO:
		CDBG("Camera Scene AUTO\n");

		rc = hi351_i2c_write_b_table(hi351_regs.scene_normal_reg_settings,
								hi351_regs.scene_normal_reg_settings_size);
		break;

	case CAMERA_SCENE_PORTRAIT:
		CDBG("Camera Scene PORTRAIT\n");
		
		rc = hi351_i2c_write_b_table(hi351_regs.scene_portrait_reg_settings,
								hi351_regs.scene_portrait_reg_settings_size);
		break;
	break;

	case CAMERA_SCENE_LANDSCAPE:
		CDBG("Camera Scene LANDSCAPE\n");
		
		rc = hi351_i2c_write_b_table(hi351_regs.scene_landscape_reg_settings,
								hi351_regs.scene_landscape_reg_settings_size);
		break;

	case CAMERA_SCENE_SPORTS:
		CDBG("Camera Scene SPORTS\n");
		
		rc = hi351_i2c_write_b_table(hi351_regs.scene_sport_reg_settings,
								hi351_regs.scene_sport_reg_settings_size);
		break;

	case CAMERA_SCENE_SUNSET:
		CDBG("Camera Scene SUNSET\n");
		
		rc = hi351_i2c_write_b_table(hi351_regs.scene_sunset_reg_settings,
								hi351_regs.scene_sunset_reg_settings_size);
		break;

	case CAMERA_SCENE_NIGHT:
		CDBG("Camera Scene NIGHT\n");
		
		rc = hi351_i2c_write_b_table(hi351_regs.scene_night_reg_settings,
								hi351_regs.scene_night_reg_settings_size);
		break;

	default:
		CDBG("Camera Scene WRONG\n");
		return -EINVAL;
		
	}
	if (rc < 0)
		return rc;

	prev_scene_mode = mode;
	return rc;
}

/* brightness register setting */
static int32_t hi351_set_brightness(int8_t brightness)
{
	int rc = 0;

	CDBG(" %s: mode -> %d\n", __func__, brightness);

	brightness = brightness + 6;		//Brightness value come from -6 to 6

  	rc = hi351_i2c_write_b_sensor(hi351_regs.brightness_reg_settings[0].baddr,
				 			  hi351_regs.brightness_reg_settings[0].bdata);
	rc = hi351_i2c_write_b_sensor(hi351_regs.brightness_reg_settings[1].baddr,
				  			  hi351_regs.brightness_reg_settings[1].bdata);
  	rc = hi351_i2c_write_b_sensor(hi351_regs.brightness_reg_settings[brightness+2].baddr,
							  hi351_regs.brightness_reg_settings[brightness+2].bdata);

 	if (rc < 0) {
		pr_err("%s: %d Error Exit \n",__func__, __LINE__);
 		return rc;
	}
 	return rc;
}

static int32_t hi351_set_csi(void) {
	int32_t rc = 0;
	struct msm_camera_csi_params hi351_csi_params;
	hi351_stop_stream();
	msleep(20);
	/* config mipi csi controller */
	printk(KERN_INFO "### %s: config mipi csi controller\n", __func__);
	if (CONFIG_CSI == 0) {
		msm_camio_vfe_clk_rate_set(192000000);
		hi351_csi_params.lane_cnt = 1;
		hi351_csi_params.data_format = CSI_8BIT;
		hi351_csi_params.lane_assign = 0xe4;
		hi351_csi_params.dpcm_scheme = 0;
		hi351_csi_params.settle_cnt = 0x19;

	    printk(KERN_INFO "%s: config mipi enter \n", __func__);
		rc = msm_camio_csi_config(&hi351_csi_params);
		if (rc < 0)
			printk(KERN_ERR "config csi controller failed \n");
		msleep(50);
		CONFIG_CSI = 1;
	}
	hi351_start_stream();
	return rc;
}
static long hi351_set_sensor_mode(int mode)
{
	int32_t rc = 0;
	int retry = 0;

	switch (mode) {
	case SENSOR_PREVIEW_MODE:
				
		CDBG("%s: %d CAMERA_Preview_MODE Enter \n",__func__, __LINE__);

		for (retry = 0; retry < 3; ++retry) {
				printk(KERN_INFO "%s:Sensor Preview Mode In\n", __func__);
			rc = hi351_reg_preview();
			if (rc < 0)
				printk(KERN_ERR "[ERROR]%s:Sensor Preview Mode Fail\n", __func__);
			else
				break;
		}

		break;
	case SENSOR_SNAPSHOT_MODE:
	case SENSOR_RAW_SNAPSHOT_MODE:
		for (retry = 0; retry < 3; ++retry) {
			printk(KERN_INFO "%s:Sensor Snapshot Mode In\n", __func__);
			rc = hi351_reg_snapshot();
			if (rc < 0)
				printk(KERN_ERR "[ERROR]%s:Sensor Snapshot Mode Fail\n", __func__);
			else
				break;
		}
		
		break;		
	default:
		rc = -EINVAL;
		break;
	}

	return rc;
}


static int hi351_set_Fps(int mode)
{
	int32_t rc = 0;

	if(prev_fps_mode == -1)
	{
		CDBG("[CHECK]%s: skip this function, Previous Init sets \n", __func__);
		prev_fps_mode = mode;
		return rc;
	}
	
	if(prev_fps_mode == mode)
	{
		CDBG("[CHECK]%s: skip this function, prev_fps_mode -> %d\n", __func__, mode);
		return rc;
	}

	printk(KERN_INFO "### %s: hi351_set_Fps mode -> %d\n", __func__, mode);
	
	switch (mode) {
	case FRAME_RATE_AUTO:
		printk(KERN_INFO "%s: %d  Frame_rate_AUTO \n",__func__, __LINE__);
		rc = hi351_i2c_write_b_table(hi351_regs.auto_framerate_reg_settings,
								hi351_regs.auto_framerate_reg_settings_size);
		
		break;
	case FRAME_RATE_ATTACHED:
		
		printk(KERN_INFO "%s: %d  Frame_rate_ATTACHED \n",__func__, __LINE__);
	
		rc = hi351_i2c_write_b_table(hi351_regs.attached_framerate_reg_settings,
								hi351_regs.attached_framerate_reg_settings_size);

		break;
	case FRAME_RATE_FIXED:
		
		printk(KERN_INFO "%s: %d  Frame_rate_FIXED \n",__func__, __LINE__);
	
		rc = hi351_i2c_write_b_table(hi351_regs.fixed_framerate_reg_settings,
								hi351_regs.fixed_framerate_reg_settings_size);

		break;
	
	   default:
		   return -EINVAL;
	   }

	prev_fps_mode = mode;
	return rc;
}
//LGE_CHANGE_S QM TD#28886 Wrong EXIF data for Auto ISO hong.junki@lge.com [2011-12-19]
int hi351_get_iso_speed( void )
{
	u8 analogGain = 0;
	u8 digitalGain = 128;
	int result = 0;

	hi351_i2c_write_b_sensor(0x03, 0x20);
	hi351_i2c_read(hi351_client->addr, 0x50, &analogGain);

	if( analogGain <= 0x28 )
	{
		CDBG("[CHECK]%s : iso speed - analogGain = 0x%x ",  __func__, analogGain);
		analogGain = 0x28;  		//analogGain cannot move down than 0x28
	}

	CDBG("[CHECK]%s : iso speed - analogGain = 0x%x ",  __func__, analogGain);

	result = ((analogGain / 32) * (digitalGain /128) * 100);
	
	return result;
}
//LGE_CHANGE_E
int hi351_sensor_config(void __user *argp)
{
	struct sensor_cfg_data cfg_data;
	long   rc = 0;
	if (CONFIG_CSI == 0) {
		rc = hi351_set_csi();
		if (rc < 0) {
			CDBG("init CSI setting failed\n");
			return rc;
		}
	}
	if (copy_from_user(&cfg_data,
			(void *)argp,
			sizeof(struct sensor_cfg_data)))
		return -EFAULT;

	mutex_lock(&hi351_mut);
	//printk(KERN_ERR "### %s: cfgtype = %d, mode -> %d\n", __func__, cfg_data.cfgtype, cfg_data.mode);

	CDBG("hi351_ioctl, cfgtype = %d, mode = %d\n",
		cfg_data.cfgtype, cfg_data.mode);

		switch (cfg_data.cfgtype) {
		case CFG_SET_MODE:
			CDBG("CFG_SET_MODE\n");
			printk(KERN_INFO "### %s: CFG_SET_MODE: mode -> %d\n", __func__, cfg_data.mode);
			rc = hi351_set_sensor_mode(cfg_data.mode);
			break;

		case CFG_SET_EFFECT:

			printk(KERN_INFO "### %s: CFG_SET_EFFECT: mode -> %d\n", __func__, cfg_data.mode);
			rc = hi351_set_effect(cfg_data.mode);
			break;

		case CFG_SET_WB:

			printk(KERN_INFO "### %s: CFG_SET_WB: mode -> %d\n", __func__, cfg_data.mode);
			rc = hi351_set_wb(cfg_data.mode);
			break;

		case CFG_SET_ISO:

			printk(KERN_INFO "### %s: CFG_SET_ISO: mode -> %d\n", __func__, cfg_data.mode);
			rc = hi351_set_iso(cfg_data.mode);
			break;

		case CFG_SET_SCENE:

			printk(KERN_INFO "### %s: CFG_SET_SCENE: mode -> %d\n", __func__, cfg_data.mode);
			rc = hi351_set_scene_mode(cfg_data.mode);
			break;
			
		case CFG_SET_BRIGHTNESS:

			printk(KERN_INFO "### %s: CFG_SET_BRIGHTNESS: mode -> %d\n", __func__, cfg_data.mode);
			rc = hi351_set_brightness(cfg_data.mode);
			break;			
	
		case CFG_SET_PICT_FPS:
		case CFG_SET_FPS:

			printk(KERN_INFO "### %s: CFG_SET_FPS OR PICT_FPS: mode -> %d\n", __func__, cfg_data.mode);
			rc = hi351_set_Fps(cfg_data.mode);
			break;
//LGE_CHANGE_S QM TD#28886 Wrong EXIF data for Auto ISO hong.junki@lge.com [2011-12-19]
		case CFG_GET_ISO_SPEED: 
			cfg_data.iso_speed = hi351_get_iso_speed();

			if(cfg_data.iso_speed <= 0 )
				return -EFAULT;
			
			if (copy_to_user((void *)argp,
					&cfg_data,
				sizeof(struct sensor_cfg_data)))
				rc = -EFAULT;
			printk(KERN_INFO "### %s: CFG_GET_ISO_SPEED : iso_speed -> %d\n", __func__, cfg_data.iso_speed );
			break;
//LGE_CHANGE_E			
		default:
			pr_err("hi351_sensor_config wrong case entered!!! case: %d", cfg_data.cfgtype);
			rc = -EINVAL;
			break;
		}

	mutex_unlock(&hi351_mut);

	return rc;
}

static int hi351_sensor_init_probe(const struct msm_camera_sensor_info *data)
{
	int rc = 0;
	unsigned char chipid1 = 0;
	int retry = 0;

	CDBG("%s in :%d\n",__func__, __LINE__);

	data->pdata->camera_power_on();

	if (rc < 0) {
		CDBG("reset failed!\n");
		goto init_probe_fail;
	}

	if (data == 0) {
		pr_err("[ERROR]%s: data is null!\n", __func__);
		return -1;
	}

	//checking sensor ID / software reset

	CDBG(" hi351_probe_init_sensor is called: Address - 0x%x\n",hi351_client->addr);
	
	rc = hi351_i2c_read(hi351_client->addr, HI351_REG_MODEL_ID, &chipid1);
	if (rc < 0)
	{
		pr_err("hi351_probe_init_sensor: 0x%x failed\n",chipid1);
		goto init_probe_fail;
	}

	CDBG("hi351_probe_init_sensor: 0x%x\n",chipid1);

	if (chipid1 != HI351_MODEL_ID) {
		rc = -ENODEV;
		pr_err("hi351_probe_init_sensor Chip ID not equal: 0x%x\n",chipid1);
		goto init_probe_fail;
	}
	pr_err("ID: %d\n", chipid1);

	CDBG("init entry \n");

	for (retry = 0; retry < 3; ++retry) {
		printk(KERN_ERR "%s:Sensor Init Setting In\n", __func__);
		rc = hi351_reg_init();
		if (rc < 0)
			printk(KERN_ERR "[ERROR]%s:Sensor Init Setting Fail\n", __func__);
		else
			break;
	}

	CDBG("init entry END \n");
	if (rc < 0) {
		CDBG("init setting failed\n");
	goto init_probe_fail;
	}

	CDBG("hi351_sensor_init_probe done\n");
	return rc;

init_probe_fail:
	hi351_probe_init_done(data);
	return rc;
}

int hi351_sensor_init(const struct msm_camera_sensor_info *data)
{
	int rc = 0;

	hi351_ctrl = kzalloc(sizeof(struct hi351_ctrl_t), GFP_KERNEL);
	if (!hi351_ctrl) {
		CDBG("hi351_init failed!\n");
		rc = -ENOMEM;
		goto init_done;
	}

	CONFIG_CSI = 0;
	DELAY_START = 0;
	
	if (data)
		hi351_ctrl->sensordata = data;

	mutex_lock(&hi351_mut);

	rc = hi351_sensor_init_probe(data);

	mutex_unlock(&hi351_mut);

	if (rc < 0) {
		CDBG("hi351_sensor_init failed!\n");
		goto init_fail;
	}

	prev_effect_mode = -1;	
	prev_balance_mode = -1;
	prev_iso_mode = -1;	
	prev_scene_mode = -1;
	prev_fps_mode = -1;


init_done:
	return rc;

init_fail:
	kfree(hi351_ctrl);
	return rc;
}

static int hi351_init_client(struct i2c_client *client)
{
	/* Initialize the MSM_CAMI2C Chip */
	init_waitqueue_head(&hi351_wait_queue);
	return 0;
}

int hi351_sensor_release(void)
{
	int rc = 0;

	mutex_lock(&hi351_mut);

	hi351_stop_stream();

	msleep(2);

	hi351_ctrl->sensordata->pdata->camera_power_off();	
	
	//gpio_free(hi351_ctrl->sensordata->sensor_reset);

	//gpio_free(hi351_ctrl->sensordata->sensor_pwd);

	kfree(hi351_ctrl);

	mutex_unlock(&hi351_mut);

	return rc;
}

static int hi351_i2c_probe(struct i2c_client *client,
	const struct i2c_device_id *id)
{
	int rc = 0;
	CDBG("hi351_i2c_probe called!\n");
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		rc = -ENOTSUPP;
		CDBG("i2c_check_functionality failed\n");
		goto probe_failure;
	}

	hi351_sensorw =
		kzalloc(sizeof(struct hi351_work), GFP_KERNEL);

	if (!hi351_sensorw) {
		rc = -ENOMEM;
		CDBG("kzalloc failed.\n");
		goto probe_failure;
	}

	i2c_set_clientdata(client, hi351_sensorw);
	hi351_init_client(client);
	hi351_client = client;

	CDBG("hi351_probe succeeded!\n");

	return 0;

probe_failure:
	kfree(hi351_sensorw);
	hi351_sensorw = NULL;
	CDBG("hi351_probe failed!\n");
	return rc;
}

static const struct i2c_device_id hi351_i2c_id[] = {
	{ "hi351", 0},
	{ },
};

static struct i2c_driver hi351_i2c_driver = {
	.id_table = hi351_i2c_id,
	.probe  = hi351_i2c_probe,
	.remove = __exit_p(hi351_i2c_remove),
	.driver = {
		.name = "hi351",
	},
};

static int hi351_sensor_probe(const struct msm_camera_sensor_info *info,
				struct msm_sensor_ctrl *s)
{
	int rc = i2c_add_driver(&hi351_i2c_driver);
	if (rc < 0 || hi351_client == NULL) {
		CDBG("%s: ret =%d\n",__func__,rc);
		rc = -ENOTSUPP;
		goto probe_done;
	}
	s->s_init = hi351_sensor_init;
	s->s_release = hi351_sensor_release;
	s->s_config  = hi351_sensor_config;
	s->s_camera_type = BACK_CAMERA_2D;
	s->s_mount_angle  = info->sensor_platform_info->mount_angle;

probe_done:
	CDBG("%s %s:%d\n", __FILE__, __func__, __LINE__);
	return rc;
}

static int __hi351_probe(struct platform_device *pdev)
{
	return msm_camera_drv_start(pdev, hi351_sensor_probe);
}

static struct platform_driver msm_camera_driver = {
	.probe = __hi351_probe,
	.driver = {
		.name = "msm_camera_hi351",
		.owner = THIS_MODULE,
	},
};

static int __init hi351_init(void)
{
	return platform_driver_register(&msm_camera_driver);
}

module_init(hi351_init);
