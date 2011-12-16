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

#ifndef HI351_H
#define HI351_H
#include <linux/types.h>
#include <mach/board.h>
extern struct hi351_reg hi351_regs;

enum hi351_type {
	BYTE_LEN,
	WORD_LEN,
	BURST_TYPE,
	DELAY_TYPE,
};

struct hi351_i2c_reg_conf {
	uint8_t baddr;
	uint8_t bdata;
	enum hi351_type register_type;
};

enum hi351_test_mode_t {
	TEST_OFF,
	TEST_1,
	TEST_2,
	TEST_3
};

enum hi351_resolution_t {
	QTR_SIZE,
	FULL_SIZE,
	INVALID_SIZE
};
enum hi351_setting {
	RES_PREVIEW,
	RES_CAPTURE
};
enum hi351_reg_update {
	/* Sensor egisters that need to be updated during initialization */
	REG_INIT,
	/* Sensor egisters that needs periodic I2C writes */
	UPDATE_PERIODIC,
	/* All the sensor Registers will be updated */
	UPDATE_ALL,
	/* Not valid update */
	UPDATE_INVALID
};

enum hi351_reg_pll {
	E013_VT_PIX_CLK_DIV,
	E013_VT_SYS_CLK_DIV,
	E013_PRE_PLL_CLK_DIV,
	E013_PLL_MULTIPLIER,
	E013_OP_PIX_CLK_DIV,
	E013_OP_SYS_CLK_DIV
};

enum hi351_reg_mode {
	E013_X_ADDR_START,
	E013_X_ADDR_END,
	E013_Y_ADDR_START,
	E013_Y_ADDR_END,
	E013_X_OUTPUT_SIZE,
	E013_Y_OUTPUT_SIZE,
	E013_DATAPATH_SELECT,
	E013_READ_MODE,
	E013_ANALOG_CONTROL5,
	E013_DAC_LD_4_5,
	E013_SCALING_MODE,
	E013_SCALE_M,
	E013_LINE_LENGTH_PCK,
	E013_FRAME_LENGTH_LINES,
	E013_COARSE_INTEGRATION_TIME,
	E013_FINE_INTEGRATION_TIME,
	E013_FINE_CORRECTION
};

/* this value is defined in Android native camera */
enum hi351_wb_type {
	CAMERA_WB_MIN_MINUS_1,
	CAMERA_WB_AUTO = 1,  /* This list must match aeecamera.h */
	CAMERA_WB_CUSTOM,
	CAMERA_WB_INCANDESCENT,
	CAMERA_WB_FLUORESCENT,
	CAMERA_WB_DAYLIGHT,
	CAMERA_WB_CLOUDY_DAYLIGHT,
	CAMERA_WB_TWILIGHT,
	CAMERA_WB_SHADE,
	CAMERA_WB_MAX_PLUS_1
};

/* Enum Type for different ISO Mode supported */
enum hi351_iso_value {
	CAMERA_ISO_AUTO = 0,
	CAMERA_ISO_DEBLUR,
	CAMERA_ISO_100,
	CAMERA_ISO_200,
	CAMERA_ISO_400,
	CAMERA_ISO_800,
	CAMERA_ISO_MAX
};

// change the framerate mode between capture(preview)(auto) and video(fixed)
enum hi351_fps_mode {
 	FRAME_RATE_AUTO = 5,
	FRAME_RATE_ATTACHED = 15,
	FRAME_RATE_FIXED = 30
};

enum hi351_antibanding_type {
	CAMERA_ANTIBANDING_OFF,
	CAMERA_ANTIBANDING_60HZ,
	CAMERA_ANTIBANDING_50HZ,
	CAMERA_ANTIBANDING_AUTO,
	CAMERA_MAX_ANTIBANDING,
};

enum {
	CAMERA_SCENE_AUTO = 0,
	CAMERA_SCENE_LANDSCAPE = 1,
	CAMERA_SCENE_SUNSET = 4,
	CAMERA_SCENE_NIGHT = 5,
	CAMERA_SCENE_PORTRAIT = 6,
	CAMERA_SCENE_SPORTS = 8,
};

struct hi351_reg {
	const struct hi351_i2c_reg_conf *reg_mipi;
	const unsigned short reg_mipi_size;
	const struct hi351_i2c_reg_conf *reg_pll_p;
	const unsigned short reg_pll_p_size;
	const struct hi351_i2c_reg_conf *reg_pll_s;
	const unsigned short reg_pll_s_size;
	const struct hi351_i2c_reg_conf *reg_settings;
	const unsigned short reg_setting_size;
	const struct hi351_i2c_reg_conf *reg_prev;
	const unsigned short reg_prev_size;
	const struct hi351_i2c_reg_conf *reg_snap;
	const unsigned short reg_snap_size;
	// LG_DEV PORTING - hong.junki@lge.com 
	// scene mode
	const struct hi351_i2c_reg_conf *scene_normal_reg_settings;
	const unsigned short scene_normal_reg_settings_size;

	const struct hi351_i2c_reg_conf *scene_portrait_reg_settings;
	const unsigned short scene_portrait_reg_settings_size;

	const struct hi351_i2c_reg_conf *scene_landscape_reg_settings;
	const unsigned short scene_landscape_reg_settings_size;

	const struct hi351_i2c_reg_conf *scene_sport_reg_settings;
	const unsigned short scene_sport_reg_settings_size;

	const struct hi351_i2c_reg_conf *scene_sunset_reg_settings;
	const unsigned short scene_sunset_reg_settings_size;

	const struct hi351_i2c_reg_conf *scene_night_reg_settings;
	const unsigned short scene_night_reg_settings_size;
	
	// framerate mode
	const struct hi351_i2c_reg_conf *auto_framerate_reg_settings;
	const unsigned short auto_framerate_reg_settings_size;
	
	const struct hi351_i2c_reg_conf *fixed_framerate_reg_settings;
	const unsigned short fixed_framerate_reg_settings_size;
		
	const struct hi351_i2c_reg_conf *attached_framerate_reg_settings;
	const unsigned short attached_framerate_reg_settings_size;

	// effect mode 
	const struct hi351_i2c_reg_conf *effect_off_reg_settings;
	const unsigned short effect_off_reg_settings_size;
	
	const struct hi351_i2c_reg_conf *effect_mono_reg_settings;
	const unsigned short effect_mono_reg_settings_size;

	const struct hi351_i2c_reg_conf *effect_negative_reg_settings;
	const unsigned short effect_negative_reg_settings_size;

	const struct hi351_i2c_reg_conf *effect_aqua_reg_settings;
	const unsigned short effect_aqua_reg_settings_size;

	const struct hi351_i2c_reg_conf *effect_sepia_reg_settings;
	const unsigned short effect_sepia_reg_settings_size;

	// white balance mode
	const struct hi351_i2c_reg_conf *wb_auto_reg_settings;
	const unsigned short wb_auto_reg_settings_size;

	const struct hi351_i2c_reg_conf *wb_incandescent_reg_settings;
	const unsigned short wb_incandescent_reg_settings_size;

	const struct hi351_i2c_reg_conf *wb_fluorescent_reg_settings;
	const unsigned short wb_fluorescent_reg_settings_size;

	const struct hi351_i2c_reg_conf *wb_daylight_reg_settings;
	const unsigned short wb_daylight_reg_settings_size;

	const struct hi351_i2c_reg_conf *wb_cloudy_reg_settings;
	const unsigned short wb_cloudy_reg_settings_size;

	// iso mode
	const struct hi351_i2c_reg_conf *iso_auto_reg_settings;
	const unsigned short iso_auto_reg_settings_size;

	const struct hi351_i2c_reg_conf *iso_100_reg_settings;
	const unsigned short iso_100_reg_settings_size;

	const struct hi351_i2c_reg_conf *iso_200_reg_settings;
	const unsigned short iso_200_reg_settings_size;

	const struct hi351_i2c_reg_conf *iso_400_reg_settings;
	const unsigned short iso_400_reg_settings_size;

	// brightness
	const struct hi351_i2c_reg_conf *brightness_reg_settings;
	const unsigned short brightness_reg_settings_size;
	
};
#endif /* HI351_H */
