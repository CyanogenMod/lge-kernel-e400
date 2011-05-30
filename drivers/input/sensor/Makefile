#
# Makefile for the sensor drivers.
#

# Each configuration option enables a list of files.

obj-$(CONFIG_SENSOR_BMA222)	+= bma222_driver.o bma222.o 

ifeq ($(CONFIG_SENSOR_BMA222_USE_SMBUS),y)
EXTRA_CFLAGS += -DBMA222_SMBUS
endif
ifeq ($(CONFIG_SENSOR_BMA222_WITH_DEBUG),y)
EXTRA_CFLAGS += -DBMA222_DEBUG
endif
ifeq ($(CONFIG_SENSOR_BMA222_HAS_EARLYSUSPEND),y)
EXTRA_CFLAGS += -DBMA222_HAS_EARLYSUSPEND
endif
