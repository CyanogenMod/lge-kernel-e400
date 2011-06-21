/*  Date: 2011/6/16 11:00:00
 *  Revision: 2.0
 */

/*
 * This software program is licensed subject to the GNU General Public License
 * (GPL).Version 2,June 1991, available at http://www.fsf.org/copyleft/gpl.html

 * (C) Copyright 2011 Bosch Sensortec GmbH
 * All Rights Reserved
 */


/* file BMA222.c
   brief This file contains all function implementations for the BMA222 in linux

*/

#include <linux/module.h>
#include <linux/init.h>
#include <linux/i2c.h>
#include <linux/input.h>
#include <linux/workqueue.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <linux/mutex.h>
#include <linux/earlysuspend.h>
#include <linux/delay.h>

/* LGE_CHANGE,
 * to debug basic operation
 * LGE_DEBUG 0,1 : off,on
 * 2011-06-16, jihyun.seong@lge.com
 */
#define LGE_DEBUG 1

#define SENSOR_NAME 			"bma222"
#define GRAVITY_EARTH                   9806550
#define ABSMIN_2G                       (-GRAVITY_EARTH * 2)
#define ABSMAX_2G                       (GRAVITY_EARTH * 2)
#define SLOPE_THRESHOLD_VALUE 		32
#define SLOPE_DURATION_VALUE 		1
#define INTERRUPT_LATCH_MODE 		13
#define INTERRUPT_ENABLE 		1
#define INTERRUPT_DISABLE 		0
#define MAP_SLOPE_INTERRUPT 		2
#define SLOPE_X_INDEX 			5
#define SLOPE_Y_INDEX 			6
#define SLOPE_Z_INDEX 			7
#define BMA222_MAX_DELAY		200
#define BMA222_CHIP_ID			3
#define BMA222_RANGE_SET		0
#define BMA222_BW_SET			4

/* LGE_CHANGE,
 * use LGE board platform data
 * 2011-06-16, jihyun.seong@lge.com
 */
#include <mach/board_lge.h> /* platform data */

/*
 *
 *      register definitions
 *
 */

#define BMA222_CHIP_ID_REG                      0x00
#define BMA222_VERSION_REG                      0x01
#define BMA222_X_AXIS_LSB_REG                   0x02
#define BMA222_X_AXIS_MSB_REG                   0x03
#define BMA222_Y_AXIS_LSB_REG                   0x04
#define BMA222_Y_AXIS_MSB_REG                   0x05
#define BMA222_Z_AXIS_LSB_REG                   0x06
#define BMA222_Z_AXIS_MSB_REG                   0x07
#define BMA222_TEMP_RD_REG                      0x08
#define BMA222_STATUS1_REG                      0x09
#define BMA222_STATUS2_REG                      0x0A
#define BMA222_STATUS_TAP_SLOPE_REG             0x0B
#define BMA222_STATUS_ORIENT_HIGH_REG           0x0C
#define BMA222_RANGE_SEL_REG                    0x0F
#define BMA222_BW_SEL_REG                       0x10
#define BMA222_MODE_CTRL_REG                    0x11
#define BMA222_LOW_NOISE_CTRL_REG               0x12
#define BMA222_DATA_CTRL_REG                    0x13
#define BMA222_RESET_REG                        0x14
#define BMA222_INT_ENABLE1_REG                  0x16
#define BMA222_INT_ENABLE2_REG                  0x17
#define BMA222_INT1_PAD_SEL_REG                 0x19
#define BMA222_INT_DATA_SEL_REG                 0x1A
#define BMA222_INT2_PAD_SEL_REG                 0x1B
#define BMA222_INT_SRC_REG                      0x1E
#define BMA222_INT_SET_REG                      0x20
#define BMA222_INT_CTRL_REG                     0x21
#define BMA222_LOW_DURN_REG                     0x22
#define BMA222_LOW_THRES_REG                    0x23
#define BMA222_LOW_HIGH_HYST_REG                0x24
#define BMA222_HIGH_DURN_REG                    0x25
#define BMA222_HIGH_THRES_REG                   0x26
#define BMA222_SLOPE_DURN_REG                   0x27
#define BMA222_SLOPE_THRES_REG                  0x28
#define BMA222_TAP_PARAM_REG                    0x2A
#define BMA222_TAP_THRES_REG                    0x2B
#define BMA222_ORIENT_PARAM_REG                 0x2C
#define BMA222_THETA_BLOCK_REG                  0x2D
#define BMA222_THETA_FLAT_REG                   0x2E
#define BMA222_FLAT_HOLD_TIME_REG               0x2F
#define BMA222_STATUS_LOW_POWER_REG             0x31
#define BMA222_SELF_TEST_REG                    0x32
#define BMA222_EEPROM_CTRL_REG                  0x33
#define BMA222_SERIAL_CTRL_REG                  0x34
#define BMA222_CTRL_UNLOCK_REG                  0x35
#define BMA222_OFFSET_CTRL_REG                  0x36
#define BMA222_OFFSET_PARAMS_REG                0x37
#define BMA222_OFFSET_FILT_X_REG                0x38
#define BMA222_OFFSET_FILT_Y_REG                0x39
#define BMA222_OFFSET_FILT_Z_REG                0x3A
#define BMA222_OFFSET_UNFILT_X_REG              0x3B
#define BMA222_OFFSET_UNFILT_Y_REG              0x3C
#define BMA222_OFFSET_UNFILT_Z_REG              0x3D
#define BMA222_SPARE_0_REG                      0x3E
#define BMA222_SPARE_1_REG                      0x3F




#define BMA222_ACC_X_LSB__POS           6
#define BMA222_ACC_X_LSB__LEN           2
#define BMA222_ACC_X_LSB__MSK           0xC0
#define BMA222_ACC_X_LSB__REG           BMA222_X_AXIS_LSB_REG

#define BMA222_ACC_X_MSB__POS           0
#define BMA222_ACC_X_MSB__LEN           8
#define BMA222_ACC_X_MSB__MSK           0xFF
#define BMA222_ACC_X_MSB__REG           BMA222_X_AXIS_MSB_REG

#define BMA222_ACC_Y_LSB__POS           6
#define BMA222_ACC_Y_LSB__LEN           2
#define BMA222_ACC_Y_LSB__MSK           0xC0
#define BMA222_ACC_Y_LSB__REG           BMA222_Y_AXIS_LSB_REG

#define BMA222_ACC_Y_MSB__POS           0
#define BMA222_ACC_Y_MSB__LEN           8
#define BMA222_ACC_Y_MSB__MSK           0xFF
#define BMA222_ACC_Y_MSB__REG           BMA222_Y_AXIS_MSB_REG

#define BMA222_ACC_Z_LSB__POS           6
#define BMA222_ACC_Z_LSB__LEN           2
#define BMA222_ACC_Z_LSB__MSK           0xC0
#define BMA222_ACC_Z_LSB__REG           BMA222_Z_AXIS_LSB_REG

#define BMA222_ACC_Z_MSB__POS           0
#define BMA222_ACC_Z_MSB__LEN           8
#define BMA222_ACC_Z_MSB__MSK           0xFF
#define BMA222_ACC_Z_MSB__REG           BMA222_Z_AXIS_MSB_REG

#define BMA222_RANGE_SEL__POS             0
#define BMA222_RANGE_SEL__LEN             4
#define BMA222_RANGE_SEL__MSK             0x0F
#define BMA222_RANGE_SEL__REG             BMA222_RANGE_SEL_REG

#define BMA222_BANDWIDTH__POS             0
#define BMA222_BANDWIDTH__LEN             5
#define BMA222_BANDWIDTH__MSK             0x1F
#define BMA222_BANDWIDTH__REG             BMA222_BW_SEL_REG

#define BMA222_EN_LOW_POWER__POS          6
#define BMA222_EN_LOW_POWER__LEN          1
#define BMA222_EN_LOW_POWER__MSK          0x40
#define BMA222_EN_LOW_POWER__REG          BMA222_MODE_CTRL_REG

#define BMA222_EN_SUSPEND__POS            7
#define BMA222_EN_SUSPEND__LEN            1
#define BMA222_EN_SUSPEND__MSK            0x80
#define BMA222_EN_SUSPEND__REG            BMA222_MODE_CTRL_REG

#define BMA222_GET_BITSLICE(regvar, bitname)\
			((regvar & bitname##__MSK) >> bitname##__POS)


#define BMA222_SET_BITSLICE(regvar, bitname, val)\
	((regvar & ~bitname##__MSK) | ((val<<bitname##__POS)&bitname##__MSK))


/* range and bandwidth */

#define BMA222_RANGE_2G                 0
#define BMA222_RANGE_4G                 1
#define BMA222_RANGE_8G                 2
#define BMA222_RANGE_16G                3


#define BMA222_BW_7_81HZ        0x08
#define BMA222_BW_15_63HZ       0x09
#define BMA222_BW_31_25HZ       0x0A
#define BMA222_BW_62_50HZ       0x0B
#define BMA222_BW_125HZ         0x0C
#define BMA222_BW_250HZ         0x0D
#define BMA222_BW_500HZ         0x0E
#define BMA222_BW_1000HZ        0x0F

/* mode settings */

#define BMA222_MODE_NORMAL      0
#define BMA222_MODE_LOWPOWER    1
#define BMA222_MODE_SUSPEND     2


/* LGE_CHANGE,
 * use LGE board platform data
 * 2011-06-16, jihyun.seong@lge.com
 */
struct acceleration_platform_data *bma222_pdata;


struct bma222acc{
	s16	x,
		y,
		z;
} ;

struct bma222_data {
	struct i2c_client *bma222_client;
	atomic_t delay;
	atomic_t enable;
	unsigned char mode;
	struct input_dev *input;
	struct bma222acc value;
	struct mutex value_mutex;
	struct mutex enable_mutex;
	struct mutex mode_mutex;
	struct delayed_work work;
	struct work_struct irq_work;
	struct early_suspend early_suspend;
};

static void bma222_early_suspend(struct early_suspend *h);
static void bma222_late_resume(struct early_suspend *h);

static int bma222_smbus_read_byte(struct i2c_client *client,
		unsigned char reg_addr, unsigned char *data)
{
	s32 dummy;
	dummy = i2c_smbus_read_byte_data(client, reg_addr);
	if (dummy < 0)
		return -1;
	*data = dummy & 0x000000ff;

	return 0;
}

static int bma222_smbus_write_byte(struct i2c_client *client,
		unsigned char reg_addr, unsigned char *data)
{
	s32 dummy;
	dummy = i2c_smbus_write_byte_data(client, reg_addr, *data);
	if (dummy < 0)
		return -1;
	return 0;
}

static int bma222_smbus_read_byte_block(struct i2c_client *client,
		unsigned char reg_addr, unsigned char *data, unsigned char len)
{
	s32 dummy;
	dummy = i2c_smbus_read_i2c_block_data(client, reg_addr, len, data);
	if (dummy < 0)
		return -1;
	return 0;
}

static int bma222_set_mode(struct i2c_client *client, unsigned char Mode)
{
	int comres = 0;
	unsigned char data1;

	if (client == NULL) {
		comres = -1;
	} else{
		if (Mode < 3) {
			comres = bma222_smbus_read_byte(client,
					BMA222_EN_LOW_POWER__REG, &data1);
			switch (Mode) {
			case BMA222_MODE_NORMAL:
				data1  = BMA222_SET_BITSLICE(data1,
					BMA222_EN_LOW_POWER, 0);
				data1  = BMA222_SET_BITSLICE(data1,
					BMA222_EN_SUSPEND, 0);
				break;
			case BMA222_MODE_LOWPOWER:
				data1  = BMA222_SET_BITSLICE(data1,
					BMA222_EN_LOW_POWER, 1);
				data1  = BMA222_SET_BITSLICE(data1,
					BMA222_EN_SUSPEND, 0);
				break;
			case BMA222_MODE_SUSPEND:
				data1  = BMA222_SET_BITSLICE(data1,
					BMA222_EN_LOW_POWER, 0);
				data1  = BMA222_SET_BITSLICE(data1,
					BMA222_EN_SUSPEND, 1);
				break;
			default:
				break;
			}

			comres += bma222_smbus_write_byte(client,
					BMA222_EN_LOW_POWER__REG, &data1);
		} else{
			comres = -1;
		}
	}

	return comres;
}

static int bma222_get_mode(struct i2c_client *client, unsigned char *Mode)
{
	int comres = 0;

	if (client == NULL) {
		comres = -1;
	} else{
		comres = bma222_smbus_read_byte(client,
				BMA222_EN_LOW_POWER__REG, Mode);
		*Mode  = (*Mode) >> 6;
	}

	return comres;
}

static int bma222_set_range(struct i2c_client *client, unsigned char Range)
{
	int comres = 0;
	unsigned char data1;

	if (client == NULL) {
		comres = -1;
	} else{
		if (Range < 4) {
			comres = bma222_smbus_read_byte(client,
					BMA222_RANGE_SEL_REG, &data1);
			switch (Range) {
			case 0:
				data1  = BMA222_SET_BITSLICE(data1,
						BMA222_RANGE_SEL, 0);
				break;
			case 1:
				data1  = BMA222_SET_BITSLICE(data1,
						BMA222_RANGE_SEL, 5);
				break;
			case 2:
				data1  = BMA222_SET_BITSLICE(data1,
						BMA222_RANGE_SEL, 8);
				break;
			case 3:
				data1  = BMA222_SET_BITSLICE(data1,
						BMA222_RANGE_SEL, 12);
				break;
			default:
					break;
			}
			comres += bma222_smbus_write_byte(client,
					BMA222_RANGE_SEL_REG, &data1);
		} else{
			comres = -1;
		}
	}

	return comres;
}

static int bma222_get_range(struct i2c_client *client, unsigned char *Range)
{
	int comres = 0;
	unsigned char data;

	if (client == NULL) {
		comres = -1;
	} else{
		comres = bma222_smbus_read_byte(client, BMA222_RANGE_SEL__REG,
				&data);
		data = BMA222_GET_BITSLICE(data, BMA222_RANGE_SEL);
		*Range = data;
	}

	return comres;
}


static int bma222_set_bandwidth(struct i2c_client *client, unsigned char BW)
{
	int comres = 0;
	unsigned char data;
	int Bandwidth = 0;

	if (client == NULL) {
		comres = -1;
	} else{
		if (BW < 8) {
			switch (BW) {
			case 0:
				Bandwidth = BMA222_BW_7_81HZ;
				break;
			case 1:
				Bandwidth = BMA222_BW_15_63HZ;
				break;
			case 2:
				Bandwidth = BMA222_BW_31_25HZ;
				break;
			case 3:
				Bandwidth = BMA222_BW_62_50HZ;
				break;
			case 4:
				Bandwidth = BMA222_BW_125HZ;
				break;
			case 5:
				Bandwidth = BMA222_BW_250HZ;
				break;
			case 6:
				Bandwidth = BMA222_BW_500HZ;
				break;
			case 7:
				Bandwidth = BMA222_BW_1000HZ;
				break;
			default:
					break;
			}
			comres = bma222_smbus_read_byte(client,
					BMA222_BANDWIDTH__REG, &data);
			data = BMA222_SET_BITSLICE(data, BMA222_BANDWIDTH,
					Bandwidth);
			comres += bma222_smbus_write_byte(client,
					BMA222_BANDWIDTH__REG, &data);
		} else{
			comres = -1;
		}
	}

	return comres;
}

static int bma222_get_bandwidth(struct i2c_client *client, unsigned char *BW)
{
	int comres = 0;
	unsigned char data;

	if (client == NULL) {
		comres = -1;
	} else{
		comres = bma222_smbus_read_byte(client, BMA222_BANDWIDTH__REG,
				&data);
		data = BMA222_GET_BITSLICE(data, BMA222_BANDWIDTH);
		if (data <= 8) {
			*BW = 0;
		} else{
			if (data >= 0x0F)
				*BW = 7;
			else
				*BW = data - 8;

		}
	}

	return comres;
}

static int bma222_read_accel_xyz(struct i2c_client *client,
							struct bma222acc *acc)
{
	int comres;
	unsigned char data[6];
	if (client == NULL) {
		comres = -1;
	} else{
		comres = bma222_smbus_read_byte_block(client,
				BMA222_ACC_X_LSB__REG, data, 6);

		acc->x = BMA222_GET_BITSLICE(data[0], BMA222_ACC_X_LSB)
			|(BMA222_GET_BITSLICE(data[1],
				BMA222_ACC_X_MSB)<<BMA222_ACC_X_LSB__LEN);
		acc->x = acc->x << (sizeof(short)*8-(BMA222_ACC_X_LSB__LEN
					+ BMA222_ACC_X_MSB__LEN));
		acc->x = acc->x >> (sizeof(short)*8-(BMA222_ACC_X_LSB__LEN
					+ BMA222_ACC_X_MSB__LEN));
		acc->y = BMA222_GET_BITSLICE(data[2], BMA222_ACC_Y_LSB)
			| (BMA222_GET_BITSLICE(data[3],
				BMA222_ACC_Y_MSB)<<BMA222_ACC_Y_LSB__LEN);
		acc->y = acc->y << (sizeof(short)*8-(BMA222_ACC_Y_LSB__LEN
					+ BMA222_ACC_Y_MSB__LEN));
		acc->y = acc->y >> (sizeof(short)*8-(BMA222_ACC_Y_LSB__LEN
					+ BMA222_ACC_Y_MSB__LEN));

		acc->z = BMA222_GET_BITSLICE(data[4], BMA222_ACC_Z_LSB)
			| (BMA222_GET_BITSLICE(data[5],
				BMA222_ACC_Z_MSB)<<BMA222_ACC_Z_LSB__LEN);
		acc->z = acc->z << (sizeof(short)*8-(BMA222_ACC_Z_LSB__LEN
					+ BMA222_ACC_Z_MSB__LEN));
		acc->z = acc->z >> (sizeof(short)*8-(BMA222_ACC_Z_LSB__LEN
					+ BMA222_ACC_Z_MSB__LEN));
	}

	return comres;
}

static void bma222_work_func(struct work_struct *work)
{
	struct bma222_data *bma222 = container_of((struct delayed_work *)work,
			struct bma222_data, work);
	static struct bma222acc acc;
	unsigned long delay = msecs_to_jiffies(atomic_read(&bma222->delay));

	bma222_read_accel_xyz(bma222->bma222_client, &acc);
	input_report_abs(bma222->input, ABS_X, acc.x);
	input_report_abs(bma222->input, ABS_Y, acc.y);
	input_report_abs(bma222->input, ABS_Z, acc.z);
	input_sync(bma222->input);
	mutex_lock(&bma222->value_mutex);
	bma222->value = acc;
	mutex_unlock(&bma222->value_mutex);
	schedule_delayed_work(&bma222->work, delay);
}

static ssize_t bma222_range_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	unsigned char data;
	struct i2c_client *client = to_i2c_client(dev);
	struct bma222_data *bma222 = i2c_get_clientdata(client);

	if (bma222_get_range(bma222->bma222_client, &data) < 0)
		return sprintf(buf, "Read error\n");

	return sprintf(buf, "%d\n", data);
}

static ssize_t bma222_range_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	unsigned long data;
	int error;
	struct i2c_client *client = to_i2c_client(dev);
	struct bma222_data *bma222 = i2c_get_clientdata(client);

	error = strict_strtoul(buf, 10, &data);
	if (error)
		return error;
	if (bma222_set_range(bma222->bma222_client, (unsigned char) data) < 0)
		return -EINVAL;

	return count;
}

static ssize_t bma222_bandwidth_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	unsigned char data;
	struct i2c_client *client = to_i2c_client(dev);
	struct bma222_data *bma222 = i2c_get_clientdata(client);

	if (bma222_get_bandwidth(bma222->bma222_client, &data) < 0)
		return sprintf(buf, "Read error\n");

	return sprintf(buf, "%d\n", data);

}

static ssize_t bma222_bandwidth_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	unsigned long data;
	int error;
	struct i2c_client *client = to_i2c_client(dev);
	struct bma222_data *bma222 = i2c_get_clientdata(client);

	error = strict_strtoul(buf, 10, &data);
	if (error)
		return error;
	if (bma222_set_bandwidth(bma222->bma222_client,
						 (unsigned char) data) < 0)
		return -EINVAL;

	return count;
}

static ssize_t bma222_mode_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	unsigned char data;
	struct i2c_client *client = to_i2c_client(dev);
	struct bma222_data *bma222 = i2c_get_clientdata(client);

	if (bma222_get_mode(bma222->bma222_client, &data) < 0)
		return sprintf(buf, "Read error\n");

	return sprintf(buf, "%d\n", data);
}

static ssize_t bma222_mode_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	unsigned long data;
	int error;
	struct i2c_client *client = to_i2c_client(dev);
	struct bma222_data *bma222 = i2c_get_clientdata(client);

	error = strict_strtoul(buf, 10, &data);
	if (error)
		return error;
	if (bma222_set_mode(bma222->bma222_client, (unsigned char) data) < 0)
		return -EINVAL;

	return count;
}


static ssize_t bma222_value_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct input_dev *input = to_input_dev(dev);
	struct bma222_data *bma222 = input_get_drvdata(input);
	struct bma222acc acc_value;

	mutex_lock(&bma222->value_mutex);
	acc_value = bma222->value;
	mutex_unlock(&bma222->value_mutex);

	return sprintf(buf, "%d %d %d\n", acc_value.x, acc_value.y,
			acc_value.z);
}

static ssize_t bma222_delay_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct bma222_data *bma222 = i2c_get_clientdata(client);

	return sprintf(buf, "%d\n", atomic_read(&bma222->delay));

}

static ssize_t bma222_delay_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	unsigned long data;
	int error;
	struct i2c_client *client = to_i2c_client(dev);
	struct bma222_data *bma222 = i2c_get_clientdata(client);

	error = strict_strtoul(buf, 10, &data);
	if (error)
		return error;
	if (data > BMA222_MAX_DELAY)
		data = BMA222_MAX_DELAY;
	atomic_set(&bma222->delay, (unsigned int) data);

	return count;
}


static ssize_t bma222_enable_show(struct device *dev,
		struct device_attribute *attr, char *buf)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct bma222_data *bma222 = i2c_get_clientdata(client);

	return sprintf(buf, "%d\n", atomic_read(&bma222->enable));

}

static void bma222_set_enable(struct device *dev, int enable)
{
	struct i2c_client *client = to_i2c_client(dev);
	struct bma222_data *bma222 = i2c_get_clientdata(client);
	int pre_enable = atomic_read(&bma222->enable);

	mutex_lock(&bma222->enable_mutex);
	if (enable) {
		if (pre_enable ==0) {
			bma222_set_mode(bma222->bma222_client, 
							BMA222_MODE_NORMAL);
			schedule_delayed_work(&bma222->work,
				msecs_to_jiffies(atomic_read(&bma222->delay)));
			atomic_set(&bma222->enable, 1);
		}
		
	} else {
		if (pre_enable ==1) {
			bma222_set_mode(bma222->bma222_client, 
							BMA222_MODE_SUSPEND);
			cancel_delayed_work_sync(&bma222->work);
			atomic_set(&bma222->enable, 0);
		} 
	}
	mutex_unlock(&bma222->enable_mutex);
	
}

static ssize_t bma222_enable_store(struct device *dev,
		struct device_attribute *attr,
		const char *buf, size_t count)
{
	unsigned long data;
	int error;

	error = strict_strtoul(buf, 10, &data);
	if (error)
		return error;
	if ((data == 0)||(data==1)) {
		bma222_set_enable(dev,data);
	}

	return count;
}

static DEVICE_ATTR(range, S_IRUGO|S_IWUSR|S_IWGRP|S_IWOTH,
		bma222_range_show, bma222_range_store);
static DEVICE_ATTR(bandwidth, S_IRUGO|S_IWUSR|S_IWGRP|S_IWOTH,
		bma222_bandwidth_show, bma222_bandwidth_store);
static DEVICE_ATTR(mode, S_IRUGO|S_IWUSR|S_IWGRP|S_IWOTH,
		bma222_mode_show, bma222_mode_store);
static DEVICE_ATTR(value, S_IRUGO,
		bma222_value_show, NULL);
static DEVICE_ATTR(delay, S_IRUGO|S_IWUSR|S_IWGRP|S_IWOTH,
		bma222_delay_show, bma222_delay_store);
static DEVICE_ATTR(enable, S_IRUGO|S_IWUSR|S_IWGRP|S_IWOTH,
		bma222_enable_show, bma222_enable_store);

static struct attribute *bma222_attributes[] = {
	&dev_attr_range.attr,
	&dev_attr_bandwidth.attr,
	&dev_attr_mode.attr,
	&dev_attr_value.attr,
	&dev_attr_delay.attr,
	&dev_attr_enable.attr,
	NULL
};

static struct attribute_group bma222_attribute_group = {
	.attrs = bma222_attributes
};

static int bma222_input_init(struct bma222_data *bma222)
{
	struct input_dev *dev;
	int err;

	dev = input_allocate_device();
	if (!dev)
		return -ENOMEM;
	dev->name = SENSOR_NAME;
	dev->id.bustype = BUS_I2C;

	input_set_capability(dev, EV_ABS, ABS_MISC);
	input_set_abs_params(dev, ABS_X, ABSMIN_2G, ABSMAX_2G, 0, 0);
	input_set_abs_params(dev, ABS_Y, ABSMIN_2G, ABSMAX_2G, 0, 0);
	input_set_abs_params(dev, ABS_Z, ABSMIN_2G, ABSMAX_2G, 0, 0);
	input_set_drvdata(dev, bma222);

	err = input_register_device(dev);
	if (err < 0) {
		input_free_device(dev);
		return err;
	}
	bma222->input = dev;

	return 0;
}

static void bma222_input_delete(struct bma222_data *bma222)
{
	struct input_dev *dev = bma222->input;

	input_unregister_device(dev);
	input_free_device(dev);
}

static int bma222_probe(struct i2c_client *client,
		const struct i2c_device_id *id)
{
	int err = 0;
	int tempvalue;
	struct bma222_data *data;

/* LGE_CHANGE,
 * use LGE board platform data
 * 2011-06-16, jihyun.seong@lge.com
 */
	struct acceleration_platform_data *pdata;

#ifdef LGE_DEBUG
	printk(KERN_INFO "%s\n",__FUNCTION__);
#endif
	if (!i2c_check_functionality(client->adapter, I2C_FUNC_I2C)) {
		printk(KERN_INFO "i2c_check_functionality error\n");
		goto exit;
	}
	data = kzalloc(sizeof(struct bma222_data), GFP_KERNEL);
	if (!data) {
		err = -ENOMEM;
		goto exit;
	}
	
	pdata = client->dev.platform_data;
    bma222_pdata = pdata;
    pdata->power(1);
    mdelay(1);
/* LGE_CHANGE_E */

	/* read chip id */
	tempvalue = 0;
	tempvalue = i2c_smbus_read_word_data(client, BMA222_CHIP_ID_REG);

	if ((tempvalue&0x00FF) == BMA222_CHIP_ID) {
		printk(KERN_INFO "Bosch Sensortec Device detected!\n" \
				"BMA222 registered I2C driver!\n");
	} else{
		printk(KERN_INFO "Bosch Sensortec Device not found, \
				i2c error %d \n", tempvalue);
		err = -1;
		goto kfree_exit;
	}
	i2c_set_clientdata(client, data);
	data->bma222_client = client;
	mutex_init(&data->value_mutex);
	mutex_init(&data->mode_mutex);
	mutex_init(&data->enable_mutex);
	bma222_set_bandwidth(client, BMA222_BW_SET);
	bma222_set_range(client, BMA222_RANGE_SET);

	INIT_DELAYED_WORK(&data->work, bma222_work_func);
	atomic_set(&data->delay, BMA222_MAX_DELAY);
	atomic_set(&data->enable, 0);
	err = bma222_input_init(data);
	if (err < 0)
		goto kfree_exit;

	err = sysfs_create_group(&data->input->dev.kobj,
						 &bma222_attribute_group);
	if (err < 0)
		goto error_sysfs;

	data->early_suspend.level = EARLY_SUSPEND_LEVEL_BLANK_SCREEN + 1;
	data->early_suspend.suspend = bma222_early_suspend;
	data->early_suspend.resume = bma222_late_resume;
	register_early_suspend(&data->early_suspend);

	return 0;

error_sysfs:
	bma222_input_delete(data);

kfree_exit:
	kfree(data);
exit:
	return err;
}


static void bma222_early_suspend(struct early_suspend *h)
{
	struct bma222_data *data =
		container_of(h, struct bma222_data, early_suspend);	

	mutex_lock(&data->enable_mutex);
	if (atomic_read(&data->enable)==1) {
		bma222_set_mode(data->bma222_client, BMA222_MODE_SUSPEND);
		cancel_delayed_work_sync(&data->work);
	}
	mutex_unlock(&data->enable_mutex);
}


static void bma222_late_resume(struct early_suspend *h)
{
	struct bma222_data *data =
		container_of(h, struct bma222_data, early_suspend);

	mutex_lock(&data->enable_mutex);
	if (atomic_read(&data->enable)==1) {
		bma222_set_mode(data->bma222_client, BMA222_MODE_NORMAL);
		schedule_delayed_work(&data->work,
			msecs_to_jiffies(atomic_read(&data->delay)));
	}
	mutex_unlock(&data->enable_mutex);
}

static int bma222_remove(struct i2c_client *client)
{
	struct bma222_data *data = i2c_get_clientdata(client);
#ifdef LGE_DEBUG
	printk(KERN_INFO "%s\n",__FUNCTION__);
#endif
	bma222_set_enable(&client->dev, 0);
	unregister_early_suspend(&data->early_suspend);
	sysfs_remove_group(&data->input->dev.kobj, &bma222_attribute_group);
	bma222_input_delete(data);
	kfree(data);
	return 0;
}


static const struct i2c_device_id bma222_id[] = {
	{ SENSOR_NAME, 0 },
	{ }
};

MODULE_DEVICE_TABLE(i2c, bma222_id);

static struct i2c_driver bma222_driver = {
	.driver = {
		.owner	= THIS_MODULE,
		.name	= SENSOR_NAME,
	},
	.id_table	= bma222_id,
	.probe		= bma222_probe,
	.remove		= bma222_remove,

};

static int __init BMA222_init(void)
{
#ifdef LGE_DEBUG
	printk(KERN_INFO "%s\n",__FUNCTION__);
#endif
	return i2c_add_driver(&bma222_driver);
}

static void __exit BMA222_exit(void)
{
	i2c_del_driver(&bma222_driver);
}

MODULE_AUTHOR("Albert Zhang <xu.zhang@bosch-sensortec.com>");
MODULE_DESCRIPTION("BMA222 driver");
MODULE_LICENSE("GPL");

module_init(BMA222_init);
module_exit(BMA222_exit);

