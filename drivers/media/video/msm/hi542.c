/* Copyright (c) 2011, Code Aurora Forum. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 and
 * only version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
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
#include "hi542.h"

/* 16bit address - 8 bit context register structure */
#define Q8	0x00000100
#define Q10	0x00000400

/* MCLK */
#define HI542_MASTER_CLK_RATE 24000000

/* AF Total steps parameters */
#define HI542_TOTAL_STEPS_NEAR_TO_FAR	32
/*****To Do:  Start *****/ 
#define HI542_REG_PREV_FRAME_LEN_1	31 /*To Do*/
#define HI542_REG_PREV_FRAME_LEN_2	32
#define HI542_REG_PREV_LINE_LEN_1	33
#define HI542_REG_PREV_LINE_LEN_2	34

#define HI542_REG_SNAP_FRAME_LEN_1	15
#define HI542_REG_SNAP_FRAME_LEN_2	16
#define  HI542_REG_SNAP_LINE_LEN_1	17
#define HI542_REG_SNAP_LINE_LEN_2	18
/*****To Do:  End *****/ 

#define HI542_OFFSET                                     5

/* QTR Size */
#define HI542_5M_Bayer_QTR_SIZE_WIDTH                    1288 // in pixels
#define HI542_5M_Bayer_QTR_SIZE_HEIGHT                   968  // in lines
#define HI542_5M_Bayer_QTR_SIZE_BLANKING_PIXELS          1496 // in pixels
#define HI542_5M_Bayer_QTR_SIZE_BLANKING_LINES           15   // in lines
#define HI542_5M_Bayer_QTR_SIZE_X_DECIMATION             2
#define HI542_5M_Bayer_QTR_SIZE_Y_DECIMATION             2
#define HI542_5M_Bayer_QTR_SIZE_FIRST_IMAGE_LINE         0   // Set to 2 if meta-data is output
#define HI542_5M_Bayer_QTR_SIZE_FIRST_IMAGE_PIXEL_CLOCK  0
#define HI542_5M_Bayer_QTR_SIZE_FRAME_RATE               30

/* FULL Size */
#define HI542_5M_Bayer_FULL_SIZE_WIDTH                   2608 // in pixels
#define HI542_5M_Bayer_FULL_SIZE_HEIGHT                  1960 // in lines
#define HI542_5M_Bayer_FULL_SIZE_BLANKING_PIXELS         176  // in pixels
#define HI542_5M_Bayer_FULL_SIZE_BLANKING_LINES          15  // in lines
#define HI542_5M_Bayer_FULL_SIZE_X_DECIMATION            1
#define HI542_5M_Bayer_FULL_SIZE_Y_DECIMATION            1
#define HI542_5M_Bayer_FULL_SIZE_FIRST_IMAGE_LINE        0    // Set to 2 if meta-data is output
#define HI542_5M_Bayer_FULL_SIZE_FIRST_IMAGE_PIXEL_CLOCK 0
#define HI542_5M_Bayer_FULL_SIZE_FRAME_RATE              15

#define HI542_5M_Bayer_MAX_SIZE_WIDTH                    HI542_5M_Bayer_FULL_SIZE_WIDTH
#define HI542_5M_Bayer_MAX_SIZE_HEIGHT                   HI542_5M_Bayer_FULL_SIZE_HEIGHT

/* Grouped Parameter */
#define REG_GROUPED_PARAMETER_HOLD                       0x0104
#define GROUPED_PARAMETER_HOLD_OFF                       0x00
#define GROUPED_PARAMETER_HOLD                           0x01

/* Mode Select */
#define REG_MODE_SELECT                                  0x0001
#define MODE_SELECT_STANDBY_MODE                         0x01
#define MODE_SELECT_STREAM                               0x00


/* Integration Time */
#define REG_COARSE_INTEGRATION_TIME_LB                   0x0118
#define REG_COARSE_INTEGRATION_TIME_MB1                  0x0117
#define REG_COARSE_INTEGRATION_TIME_MB2                  0x0116
#define REG_COARSE_INTEGRATION_TIME_HB                   0x0115


/* Gain */
#define REG_ANALOGUE_GAIN_CODE_GLOBAL                    0x0129

#define MSB                             1
#define LSB                             0


#define HI542_REG_MODEL_ID 		 0x0004 /*Chip ID read register*/
#define HI542_MODEL_ID     		 0xB1 /*Hynix HI542 Chip ID*/ 


struct hi542_work_t {
	struct work_struct work;
};

static struct hi542_work_t *hi542_sensorw;
static struct i2c_client *hi542_client;

struct hi542_ctrl_t {
	const struct  msm_camera_sensor_info *sensordata;

	uint32_t sensormode;
	uint32_t fps_divider;/* init to 1 * 0x00000400 */
	uint32_t pict_fps_divider;/* init to 1 * 0x00000400 */
	uint16_t fps;

	uint16_t curr_lens_pos;
	uint16_t curr_step_pos;
	uint16_t my_reg_gain;
	uint32_t my_reg_line_count;
	uint16_t total_lines_per_frame;

	enum hi542_resolution_t prev_res;
	enum hi542_resolution_t pict_res;
	enum hi542_resolution_t curr_res;
	enum hi542_test_mode_t  set_test;
};

static bool CSI_CONFIG;
static struct hi542_ctrl_t *hi542_ctrl;

static DECLARE_WAIT_QUEUE_HEAD(hi542_wait_queue);
DEFINE_MUTEX(hi542_mut);

static uint16_t prev_line_length_pck;
static uint16_t prev_frame_length_lines;
static uint16_t snap_line_length_pck;
static uint16_t snap_frame_length_lines;

static int hi542_i2c_rxdata(unsigned short saddr,
		unsigned char *rxdata, int length)
{
	struct i2c_msg msgs[] = {
		{
			.addr  = saddr << 1,
			.flags = 0,
			.len   = 2,
			.buf   = rxdata,
		},
		{
			.addr  = saddr << 1,
			.flags = I2C_M_RD,
			.len   = 1,
			.buf   = rxdata,
		},
	};
	if (i2c_transfer(hi542_client->adapter, msgs, 2) < 0) {
		CDBG("hi542_i2c_rxdata faild 0x%x\n", saddr);
		return -EIO;
	}
	return 0;
}

static int32_t hi542_i2c_txdata(unsigned short saddr,
		unsigned char *txdata, int length)
{
	struct i2c_msg msg[] = {
		{
			.addr = saddr << 1,
			.flags = 0,
			.len = length,
			.buf = txdata,
		},
	};
	if (i2c_transfer(hi542_client->adapter, msg, 1) < 0) {
		CDBG("hi542_i2c_txdata faild 0x%x\n", saddr);
		return -EIO;
	}

	return 0;
}

static int32_t hi542_i2c_read(unsigned short saddr,
	unsigned short raddr, unsigned short *rdata)
{
	int32_t rc = 0;
	unsigned char buf[4];

	if (!rdata)
		return -EIO;

	memset(buf, 0, sizeof(buf));

	buf[0] = (raddr & 0xFF00)>>8;
	buf[1] = (raddr & 0x00FF);

	rc = hi542_i2c_rxdata(saddr, buf, 2);
	if (rc < 0)
		return rc;


	*rdata = buf[0];
	//*rdata = buf[0] << 8 | buf[1];


	if (rc < 0)
		pr_err("hi542_i2c_read failed!\n");

	return rc;
}


static int32_t hi542_i2c_write_b_sensor(unsigned short waddr, uint8_t bdata)
{
	int32_t rc = -EFAULT;
	unsigned char buf[3];
	//pr_err("%s: %d  Enter \n",__func__, __LINE__);
	memset(buf, 0, sizeof(buf));
	buf[0] = (waddr & 0xFF00) >> 8;
	buf[1] = (waddr & 0x00FF);
	buf[2] = bdata;
	CDBG("i2c_write_b addr = 0x%x, val = 0x%x\n", waddr, bdata);
//	pr_err("%s: %d  Enter \n",__func__, __LINE__);
//	pr_err("i2c_write_b addr = 0x%x, val = 0x%x\n", waddr, bdata);
	rc = hi542_i2c_txdata(hi542_client->addr, buf, 3);
	if (rc < 0) {
		CDBG("i2c_write_b failed, addr = 0x%x, val = 0x%x!\n",
				waddr, bdata);
//		pr_err("i2c_write_b failed, addr = 0x%x, val = 0x%x!\n",
//				waddr, bdata);
		
	}
	//pr_err("%s: %d  Exit \n",__func__, __LINE__);
	return rc;
}

static int32_t hi542_i2c_write_b_table(struct hi542_i2c_reg_conf const
		*reg_conf_tbl, int num)
{
	int i;
	int32_t rc = -EIO;
	pr_err("%s: %d  Enter num: %d\n",__func__,__LINE__,num);
	for (i = 0; i < num; i++) {
		rc = hi542_i2c_write_b_sensor(reg_conf_tbl->waddr,
				reg_conf_tbl->wdata);
		if (rc < 0)
			break;
		reg_conf_tbl++;
	}
	pr_err("%s: %d  Exit \n",__func__, __LINE__);
	return rc;
}

static void hi542_start_stream(void)
{
	pr_err("%s: %d  Enter \n",__func__, __LINE__);
	hi542_i2c_write_b_sensor(0x0001, 0x00);/* streaming on */
	pr_err("%s: %d  Exit \n",__func__, __LINE__);
}

static void hi542_stop_stream(void)
{
	pr_err("%s: %d  Enter \n",__func__, __LINE__);
	hi542_i2c_write_b_sensor(0x0001, 0x1);
	hi542_i2c_write_b_sensor(0x0001, 0x0);
	hi542_i2c_write_b_sensor(0x0001, 0x1);
	pr_err("%s: %d  Exit \n",__func__, __LINE__);
}

static void hi542_group_hold_on(void)
{
	pr_err("%s: %d  Enter \n",__func__, __LINE__);
	hi542_i2c_write_b_sensor(0x0104, 0x01);
	pr_err("%s: %d  Exit \n",__func__, __LINE__);
}

static void hi542_group_hold_off(void)
{
	pr_err("%s: %d  Enter \n",__func__, __LINE__);
	hi542_i2c_write_b_sensor(0x0104, 0x0);
	pr_err("%s: %d  Exit \n",__func__, __LINE__);
}

static void hi542_get_pict_fps(uint16_t fps, uint16_t *pfps)
{
	/* input fps is preview fps in Q8 format */
	uint32_t divider, d1, d2;
	pr_err("%s: %d  Enter \n",__func__, __LINE__);
	d1 = (prev_frame_length_lines * 0x00000400) / snap_frame_length_lines;
	d2 = (prev_line_length_pck * 0x00000400) / snap_line_length_pck;
	divider = (d1 * d2) / 0x400;

	/*Verify PCLK settings and frame sizes.*/
	*pfps = (uint16_t) (fps * divider / 0x400);
	pr_err("%s: %d  Exit \n",__func__, __LINE__);
}

static uint16_t hi542_get_prev_lines_pf(void)
{
	pr_err("%s: %d  Enter \n",__func__, __LINE__);
	if (hi542_ctrl->prev_res == QTR_SIZE)
		return prev_frame_length_lines;
	else
		return snap_frame_length_lines;
}

static uint16_t hi542_get_prev_pixels_pl(void)
{
	if (hi542_ctrl->prev_res == QTR_SIZE)
		return prev_line_length_pck;
	else
		return snap_line_length_pck;
}

static uint16_t hi542_get_pict_lines_pf(void)
{
	if (hi542_ctrl->pict_res == QTR_SIZE)
		return prev_frame_length_lines;
	else
		return snap_frame_length_lines;
}

static uint16_t hi542_get_pict_pixels_pl(void)
{
	if (hi542_ctrl->pict_res == QTR_SIZE)
		return prev_line_length_pck;
	else
		return snap_line_length_pck;
}

static uint32_t hi542_get_pict_max_exp_lc(void)
{
	return snap_frame_length_lines * 24;
}

static int32_t hi542_set_fps(struct fps_cfg   *fps)
{
	uint32_t total_lines_per_frame;
	int32_t rc = 0;

	hi542_ctrl->fps_divider = fps->fps_div;
	hi542_ctrl->pict_fps_divider = fps->pict_fps_div;

	if (hi542_ctrl->sensormode == SENSOR_PREVIEW_MODE) {
		total_lines_per_frame = (uint32_t)
		((prev_frame_length_lines * prev_line_length_pck * hi542_ctrl->fps_divider) / 0x400);
	} else {
		total_lines_per_frame = (uint32_t)
		((snap_frame_length_lines * snap_line_length_pck * hi542_ctrl->fps_divider) / 0x400);
	}

	hi542_group_hold_on();
	rc = hi542_i2c_write_b_sensor(0x0120,
			((total_lines_per_frame & 0xFF000000) >> 24));
	rc = hi542_i2c_write_b_sensor(0x0121,
			((total_lines_per_frame & 0x00FF0000) >> 16));
	rc = hi542_i2c_write_b_sensor(0x0122,
			((total_lines_per_frame & 0x0000FF00) >> 8));
	rc = hi542_i2c_write_b_sensor(0x0123,
			((total_lines_per_frame & 0x000000FF)));
	hi542_group_hold_off();

	return rc;
}

static inline uint8_t hi542_byte(uint16_t word, uint8_t offset)
{
	return word >> (offset * BITS_PER_BYTE);
}

static int32_t hi542_write_exp_gain(uint16_t gain, uint32_t line)
{
	uint32_t pixels_line = 0;
	uint8_t i = 0, mask = 0xFF;
	uint8_t values[] = { 0, 0, 0, 0, 0 };
	int rc;
	pr_err("%s: %d  Enter \n",__func__, __LINE__);
	pixels_line = line *
	   (HI542_5M_Bayer_FULL_SIZE_WIDTH + HI542_5M_Bayer_FULL_SIZE_BLANKING_PIXELS);

	for ( i = 1 ; i < 5; i++ ) {
	   values[i]  = ( mask & pixels_line );
	   pixels_line >>= 8;
	}
	values[0] = gain;

	rc = hi542_i2c_write_b_sensor(REG_ANALOGUE_GAIN_CODE_GLOBAL, values[0]);
	rc = hi542_i2c_write_b_sensor(REG_COARSE_INTEGRATION_TIME_LB, values[1]);
	rc = hi542_i2c_write_b_sensor(REG_COARSE_INTEGRATION_TIME_MB1, values[2]);
	rc = hi542_i2c_write_b_sensor(REG_COARSE_INTEGRATION_TIME_MB2, values[3]);
	rc = hi542_i2c_write_b_sensor(REG_COARSE_INTEGRATION_TIME_HB, values[4]);

	pr_err("%s: %d  Exit \n",__func__, __LINE__);
	return rc;
}

static int32_t hi542_set_pict_exp_gain(uint16_t gain, uint32_t line)
{
	return hi542_write_exp_gain(gain, line);
}

static int32_t hi542_sensor_setting(int update_type, int rt)
{

	int32_t rc = 0;
	struct msm_camera_csi_params hi542_csi_params;
	pr_err("%s: %d  Enter \n",__func__, __LINE__);
	hi542_stop_stream();
	/* TODO change msleep API */
	msleep(10);

	if (update_type == REG_INIT) {
		hi542_i2c_write_b_table(hi542_regs.rec_settings,
				hi542_regs.rec_size);
		CSI_CONFIG = 0;
	} else if (update_type == UPDATE_PERIODIC) {
		if (rt == RES_PREVIEW)
			hi542_i2c_write_b_table(hi542_regs.reg_prev,
					hi542_regs.reg_prev_size);
		else
			hi542_i2c_write_b_table(hi542_regs.reg_snap,
					hi542_regs.reg_snap_size);
		msleep(20);
		if (!CSI_CONFIG) {
			msm_camio_vfe_clk_rate_set(192000000);
			hi542_csi_params.data_format = CSI_10BIT;
			hi542_csi_params.lane_cnt = 2;
			hi542_csi_params.lane_assign = 0xe4;
			hi542_csi_params.dpcm_scheme = 0;
			hi542_csi_params.settle_cnt = 0x14;
			rc = msm_camio_csi_config(&hi542_csi_params);
			msleep(20);
			CSI_CONFIG = 1;
		}
		hi542_start_stream();
		msleep(10);
	}
	pr_err("%s: %d  Exit \n",__func__, __LINE__);
	return rc;
}

static int32_t hi542_video_config(int mode)
{

	int32_t rc = 0;
	int rt;
	CDBG("video config\n");
	/* change sensor resolution if needed */
	if (hi542_ctrl->prev_res == QTR_SIZE)
		rt = RES_PREVIEW;
	else
		rt = RES_CAPTURE;
	if (hi542_sensor_setting(UPDATE_PERIODIC, rt) < 0)
		return rc;

	hi542_ctrl->curr_res = hi542_ctrl->prev_res;
	hi542_ctrl->sensormode = mode;
	return rc;
}

static int32_t hi542_snapshot_config(int mode)
{
	int32_t rc = 0;
	int rt;

	/*change sensor resolution if needed */
	if (hi542_ctrl->curr_res != hi542_ctrl->pict_res) {
		if (hi542_ctrl->pict_res == QTR_SIZE)
			rt = RES_PREVIEW;
		else
			rt = RES_CAPTURE;
		if (hi542_sensor_setting(UPDATE_PERIODIC, rt) < 0)
			return rc;
	}

	hi542_ctrl->curr_res = hi542_ctrl->pict_res;
	hi542_ctrl->sensormode = mode;
	return rc;
}

static int32_t hi542_raw_snapshot_config(int mode)
{
	int32_t rc = 0;
	int rt;

	/* change sensor resolution if needed */
	if (hi542_ctrl->curr_res != hi542_ctrl->pict_res) {
		if (hi542_ctrl->pict_res == QTR_SIZE)
			rt = RES_PREVIEW;
		else
			rt = RES_CAPTURE;
		if (hi542_sensor_setting(UPDATE_PERIODIC, rt) < 0)
			return rc;
	}

	hi542_ctrl->curr_res = hi542_ctrl->pict_res;
	hi542_ctrl->sensormode = mode;
	return rc;
}

static int32_t hi542_set_sensor_mode(int mode,
		int res)
{
	int32_t rc = 0;
	pr_err("%s: %d  Enter \n",__func__, __LINE__);
	switch (mode) {
	case SENSOR_PREVIEW_MODE:
		rc = hi542_video_config(mode);
		break;
	case SENSOR_SNAPSHOT_MODE:
		rc = hi542_snapshot_config(mode);
		break;
	case SENSOR_RAW_SNAPSHOT_MODE:
		rc = hi542_raw_snapshot_config(mode);
		break;
	default:
		rc = -EINVAL;
		break;
	}
	pr_err("%s: %d  Exit \n",__func__, __LINE__);
	return rc;
}

static int32_t hi542_power_down(void)
{
	pr_err("%s: %d  Enter \n",__func__, __LINE__);
	hi542_stop_stream();
	pr_err("%s: %d  Exit \n",__func__, __LINE__);

    if(hi542_ctrl)
        hi542_ctrl->sensordata->pdata->camera_power_off();

	return 0;
}

static int hi542_probe_init_done(const struct msm_camera_sensor_info *data)
{
	CDBG("probe done\n");
	pr_err("%s: %d  Enter \n",__func__, __LINE__);
	gpio_free(data->sensor_reset);
	pr_err("%s: %d  Exit \n",__func__, __LINE__);
	return 0;
}

static int hi542_probe_init_sensor(const struct msm_camera_sensor_info *data)
{
	int32_t rc = 0;
	unsigned short chipid1 = 0;

	pr_err("%s: %d\n", __func__, __LINE__);
	pr_err(" hi542_probe_init_sensor is called: Address - 0x%x\n",hi542_client->addr);
	
	rc = hi542_i2c_read(hi542_client->addr,HI542_REG_MODEL_ID, &chipid1);
	if (rc < 0)
	{
		printk(KERN_ERR "hi542_probe_init_sensor: 0x%x\n",chipid1);
		goto init_probe_fail;
	}

	if (chipid1 != HI542_MODEL_ID) {
		rc = -ENODEV;
		printk(KERN_ERR "hi542_probe_init_sensor Chip ID not equal: 0x%x\n",chipid1);
		goto init_probe_fail;
	}
	pr_err("ID: %d\n", chipid1);
	pr_err("%s: %d  Exit Success \n",__func__, __LINE__);
	return rc;

init_probe_fail:
	pr_err(" hi542_probe_init_sensor fails\n");
	gpio_set_value_cansleep(data->sensor_reset, 0);
	hi542_probe_init_done(data);

	return rc;
}

int hi542_sensor_open_init(const struct msm_camera_sensor_info *data)
{
	int32_t rc = 0;

	hi542_ctrl = kzalloc(sizeof(struct hi542_ctrl_t), GFP_KERNEL);
	if (!hi542_ctrl) {
		CDBG("hi542_init failed!\n");
		rc = -ENOMEM;
		goto init_done;
	}
	hi542_ctrl->fps_divider = 1 * 0x00000400;
	hi542_ctrl->pict_fps_divider = 1 * 0x00000400;
	hi542_ctrl->set_test = TEST_OFF;
	hi542_ctrl->prev_res = QTR_SIZE;
	hi542_ctrl->pict_res = FULL_SIZE;

	if (data)
		hi542_ctrl->sensordata = data;

	prev_frame_length_lines = HI542_5M_Bayer_QTR_SIZE_HEIGHT + HI542_5M_Bayer_QTR_SIZE_BLANKING_LINES - HI542_OFFSET;
	prev_line_length_pck = HI542_5M_Bayer_QTR_SIZE_WIDTH + HI542_5M_Bayer_QTR_SIZE_BLANKING_PIXELS;

	snap_frame_length_lines = HI542_5M_Bayer_FULL_SIZE_HEIGHT + HI542_5M_Bayer_FULL_SIZE_BLANKING_LINES - HI542_OFFSET;
	snap_line_length_pck = HI542_5M_Bayer_FULL_SIZE_WIDTH + HI542_5M_Bayer_FULL_SIZE_BLANKING_PIXELS;

	/* enable mclk first */
	msm_camio_clk_rate_set(HI542_MASTER_CLK_RATE);

	// power on
	data->pdata->camera_power_on();

	rc = hi542_probe_init_sensor(data);
	if (rc < 0)
		goto init_fail;

	pr_err("init settings\n");
	if (hi542_ctrl->prev_res == QTR_SIZE)
		rc = hi542_sensor_setting(REG_INIT, RES_PREVIEW);
	else
		rc = hi542_sensor_setting(REG_INIT, RES_CAPTURE);
	hi542_ctrl->fps = 30 * Q8;

	if (rc < 0)
		goto init_fail;
	else
		goto init_done;
init_fail:
	pr_err("%s: %d Exit init_fail\n",__func__,__LINE__);
	hi542_probe_init_done(data);
init_done:
	pr_err("%s: %d Exit init_done\n", __func__,__LINE__);
	return rc;
}

static int hi542_init_client(struct i2c_client *client)
{
	/* Initialize the MSM_CAMI2C Chip */
	init_waitqueue_head(&hi542_wait_queue);
	return 0;
}

static const struct i2c_device_id hi542_i2c_id[] = {
	{"hi542", 0},
	{ }
};

static int hi542_i2c_probe(struct i2c_client *client,
		const struct i2c_device_id *id)
{
	int rc = 0;
	CDBG("hi542_probe called!\n");

	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		CDBG("i2c_check_functionality failed\n");
		goto probe_failure;
	}

	hi542_sensorw = kzalloc(sizeof(struct hi542_work_t), GFP_KERNEL);
	if (!hi542_sensorw) {
		CDBG("kzalloc failed.\n");
		rc = -ENOMEM;
		goto probe_failure;
	}

	i2c_set_clientdata(client, hi542_sensorw);
	hi542_init_client(client);
	hi542_client = client;

	msleep(50);

	CDBG("hi542_probe successed! rc = %d\n", rc);
	return 0;

probe_failure:
	CDBG("hi542_probe failed! rc = %d\n", rc);
	return rc;
}

static int __devexit hi542_remove(struct i2c_client *client)
{
	struct hi542_work_t *sensorw = i2c_get_clientdata(client);
	free_irq(client->irq, sensorw);
	hi542_client = NULL;
	kfree(sensorw);
	return 0;
}

static struct i2c_driver hi542_i2c_driver = {
	.id_table = hi542_i2c_id,
	.probe  = hi542_i2c_probe,
	.remove = __exit_p(hi542_i2c_remove),
	.driver = {
		.name = "hi542",
	},
};

int hi542_sensor_config(void __user *argp)
{
	struct sensor_cfg_data cdata;
	long   rc = 0;
	if (copy_from_user(&cdata,
				(void *)argp,
				sizeof(struct sensor_cfg_data)))
		return -EFAULT;
	mutex_lock(&hi542_mut);
	CDBG("hi542_sensor_config: cfgtype = %d\n",
			cdata.cfgtype);
	switch (cdata.cfgtype) {
	case CFG_GET_PICT_FPS:
		hi542_get_pict_fps(
			cdata.cfg.gfps.prevfps,
			&(cdata.cfg.gfps.pictfps));

		if (copy_to_user((void *)argp,
			&cdata,
			sizeof(struct sensor_cfg_data)))
			rc = -EFAULT;
		break;
	case CFG_GET_PREV_L_PF:
		cdata.cfg.prevl_pf =
			hi542_get_prev_lines_pf();

		if (copy_to_user((void *)argp,
				&cdata,
				sizeof(struct sensor_cfg_data)))
			rc = -EFAULT;
		break;
	case CFG_GET_PREV_P_PL:
		cdata.cfg.prevp_pl =
			hi542_get_prev_pixels_pl();

		if (copy_to_user((void *)argp,
				&cdata,
				sizeof(struct sensor_cfg_data)))
			rc = -EFAULT;
		break;
	case CFG_GET_PICT_L_PF:
		cdata.cfg.pictl_pf =
			hi542_get_pict_lines_pf();

		if (copy_to_user((void *)argp,
				&cdata,
				sizeof(struct sensor_cfg_data)))
			rc = -EFAULT;
		break;
	case CFG_GET_PICT_P_PL:
		cdata.cfg.pictp_pl =
			hi542_get_pict_pixels_pl();
		if (copy_to_user((void *)argp,
				&cdata,
				sizeof(struct sensor_cfg_data)))
			rc = -EFAULT;
		break;
	case CFG_GET_PICT_MAX_EXP_LC:
		cdata.cfg.pict_max_exp_lc =
			hi542_get_pict_max_exp_lc();

		if (copy_to_user((void *)argp,
				&cdata,
				sizeof(struct sensor_cfg_data)))
			rc = -EFAULT;
		break;
	case CFG_SET_FPS:
	case CFG_SET_PICT_FPS:
		rc = hi542_set_fps(&(cdata.cfg.fps));
		break;
	case CFG_SET_EXP_GAIN:
		rc = hi542_write_exp_gain(cdata.cfg.exp_gain.gain,
				cdata.cfg.exp_gain.line);
		break;
	case CFG_SET_PICT_EXP_GAIN:
		rc = hi542_set_pict_exp_gain(cdata.cfg.exp_gain.gain,
				cdata.cfg.exp_gain.line);
		break;
	case CFG_SET_MODE:
		rc = hi542_set_sensor_mode(cdata.mode, cdata.rs);
		break;
	case CFG_PWR_DOWN:
		rc = hi542_power_down();
		break;
	case CFG_GET_AF_MAX_STEPS:
		cdata.max_steps = HI542_TOTAL_STEPS_NEAR_TO_FAR;
		if (copy_to_user((void *)argp,
					&cdata,
				sizeof(struct sensor_cfg_data)))
			rc = -EFAULT;
		break;
	default:
		rc = -EFAULT;
		break;
	}
	mutex_unlock(&hi542_mut);

	return rc;
}

static int hi542_sensor_release(void)
{
	int rc = -EBADF;

	printk(KERN_ERR "%s: 1", __func__);
	mutex_lock(&hi542_mut);
	hi542_power_down();
	msleep(20);
	gpio_set_value_cansleep(hi542_ctrl->sensordata->sensor_reset, 0);
	usleep_range(5000, 5100);
	gpio_free(hi542_ctrl->sensordata->sensor_reset);
	if (hi542_ctrl->sensordata->vcm_enable) {
		printk(KERN_ERR "%s: 2", __func__);
		gpio_set_value_cansleep(hi542_ctrl->sensordata->vcm_pwd, 0);
		gpio_free(hi542_ctrl->sensordata->vcm_pwd);
	}
	kfree(hi542_ctrl);
	hi542_ctrl = NULL;
	CDBG("hi542_release completed\n");
	mutex_unlock(&hi542_mut);

	return rc;
}

static int hi542_sensor_probe(const struct msm_camera_sensor_info *info,
		struct msm_sensor_ctrl *s)
{
	int rc = 0;

	rc = i2c_add_driver(&hi542_i2c_driver);
	if (rc < 0 || hi542_client == NULL) {
		rc = -ENOTSUPP;
		pr_err("I2C add driver failed");
		goto probe_fail_1;
	}

	s->s_init = hi542_sensor_open_init;
	s->s_release = hi542_sensor_release;
	s->s_config  = hi542_sensor_config;
	s->s_camera_type = BACK_CAMERA_2D;
	s->s_mount_angle = info->sensor_platform_info->mount_angle;

	return rc;

probe_fail_1:
	CDBG("hi542_sensor_probe: SENSOR PROBE FAILS!\n");
	return rc;
}

static int __devinit hi542_probe(struct platform_device *pdev)
{
	return msm_camera_drv_start(pdev, hi542_sensor_probe);
}

static struct platform_driver msm_camera_driver = {
	.probe = hi542_probe,
	.driver = {
		.name = "msm_camera_hi542",
		.owner = THIS_MODULE,
	},
};

static int __init hi542_init(void)
{
	return platform_driver_register(&msm_camera_driver);
}

module_init(hi542_init);
MODULE_DESCRIPTION("Hynix 5 MP Bayer sensor driver");
MODULE_LICENSE("GPL v2");
