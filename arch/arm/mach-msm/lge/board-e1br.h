#ifndef __ARCH_MSM_BOARD_E1BR_H
#define __ARCH_MSM_BOARD_E1BR_H

enum {
	GPIO_EXPANDER_IRQ_BASE	= NR_MSM_IRQS + NR_GPIO_IRQS,
	GPIO_EXPANDER_GPIO_BASE	= NR_MSM_GPIOS,
	/* SURF expander */
	GPIO_CORE_EXPANDER_BASE	= GPIO_EXPANDER_GPIO_BASE,
	GPIO_BT_SYS_REST_EN	= GPIO_CORE_EXPANDER_BASE,
	GPIO_WLAN_EXT_POR_N,
	GPIO_DISPLAY_PWR_EN,
	GPIO_BACKLIGHT_EN,
	GPIO_PRESSURE_XCLR,
	GPIO_VREG_S3_EXP,
	GPIO_UBM2M_PWRDWN,
	GPIO_ETM_MODE_CS_N,
	GPIO_HOST_VBUS_EN,
	GPIO_SPI_MOSI,
	GPIO_SPI_MISO,
	GPIO_SPI_CLK,
	GPIO_SPI_CS0_N,
	GPIO_CORE_EXPANDER_IO13,
	GPIO_CORE_EXPANDER_IO14,
	GPIO_CORE_EXPANDER_IO15,
	/* Camera expander */
	GPIO_CAM_EXPANDER_BASE	= GPIO_CORE_EXPANDER_BASE + 16,
	GPIO_CAM_GP_STROBE_READY	= GPIO_CAM_EXPANDER_BASE,
	GPIO_CAM_GP_AFBUSY,
	GPIO_CAM_GP_CAM_PWDN,
	GPIO_CAM_GP_CAM1MP_XCLR,
	GPIO_CAM_GP_CAMIF_RESET_N,
	GPIO_CAM_GP_STROBE_CE,
	GPIO_CAM_GP_LED_EN1,
	GPIO_CAM_GP_LED_EN2,
};

/* touch-screen macros */
#define TS_X_MIN		0
#define TS_X_MAX		240
#define TS_Y_MIN		0
#define TS_Y_MAX		320
#define TS_GPIO_I2C_SDA		10
#define TS_GPIO_I2C_SCL		9	
#define TS_GPIO_IRQ		39
#define TS_I2C_SLAVE_ADDR	0x48	/* MELFAS Mcs8000(mms-128) addr is 0x48 */

#if 1  /* LGE_CHANGE_E [yoonsoo.kim@lge.com] 20110902: New Porting BMC050*/
#define SENSOR_GPIO_I2C_SCL	49
#define SENSOR_GPIO_I2C_SDA	48
/* accelerometer */
#define ACCEL_GPIO_INT	 		94	/* motion interrupt 1*/ 
#define ACCEL_GPIO_I2C_SCL  	SENSOR_GPIO_I2C_SCL
#define ACCEL_GPIO_I2C_SDA  	SENSOR_GPIO_I2C_SDA
#define ACCEL_I2C_ADDRESS		0x18 /* slave address 7bit - U0 BMA250 accelerometer sensor */

/* BOSCH Ecompass : Bosch compass+accelerometer are internally use two sensor */
#define ECOM_GPIO_I2C_SCL		SENSOR_GPIO_I2C_SCL
#define ECOM_GPIO_I2C_SDA		SENSOR_GPIO_I2C_SDA
#define ECOM_GPIO_INT			35 /* motion interrupt 2*/
#define ECOM_I2C_ADDRESS		0x10 /* slave address 7bit - U0 bmm050 bosch compass sensor */
#else
#define ACCEL_GPIO_INT	 		94
#define ACCEL_GPIO_I2C_SCL  	49
#define ACCEL_GPIO_I2C_SDA  	48
#define ACCEL_I2C_ADDRESS		0x08 /* slave address 7bit - BMA222 */

/* Ecompass */
#define ECOM_GPIO_I2C_SCL		38
#define ECOM_GPIO_I2C_SDA		35
#define ECOM_GPIO_INT			130 /* DRDY */
#define ECOM_I2C_ADDRESS		0x0C /* slave address 7bit - AK8975C */
#endif  /* LGE_CHANGE_E [yoonsoo.kim@lge.com] 20110902: New Porting BMC050*/

/* proximity sensor */
#define PROXI_GPIO_I2C_SCL	16   
#define PROXI_GPIO_I2C_SDA 	30   
#define PROXI_GPIO_DOUT		17
#define PROXI_I2C_ADDRESS	0x44 /* slave address 7bit - GP2AP002 */
#define PROXI_LDO_NO_VCC	1

/* sdcard related macros */
#ifdef CONFIG_MMC_MSM_CARD_HW_DETECTION
#define GPIO_SD_DETECT_N    40
//#define VREG_SD_LEVEL       3000
#define VREG_SD_LEVEL       2850

#define GPIO_SD_DATA_3      51
#define GPIO_SD_DATA_2      52
#define GPIO_SD_DATA_1      53
#define GPIO_SD_DATA_0      54
#define GPIO_SD_CMD         55
#define GPIO_SD_CLK         56
#endif

/* ear sense gpio */
#define GPIO_EAR_SENSE		41
#define GPIO_BUTTON_DETECT  28
#define GPIO_MIC_MODE		127

/* connectivity gpio */
#define BT_SYS_REST_EN		6
/* suhui.kim@lge.com, added bluetooth gpio using I2C1 */
#define BT_GPIO_I2C_SCL  	131
#define BT_GPIO_I2C_SDA  	132

/* camera gpio */
//LGE_CHANGE_S E0_CAMERA [2011-10-23] hong.junki@lge.com
#define GPIO_CAM_RESET      34
#define GPIO_CAM_PWDN		42
//LGE_CHANGE_E

/* hollic gpio */
#ifdef CONFIG_HOLLIC
#define GPIO_HALLIC_IRQ     83
#endif

extern void snd_fm_vol_mute(void);
#endif

