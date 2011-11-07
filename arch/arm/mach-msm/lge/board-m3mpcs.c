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
#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/io.h>
#include <linux/gpio.h>

#include <asm/mach-types.h>
#include <asm/mach/arch.h>

#include <mach/board.h>
#include <mach/msm_iomap.h>
// BEGIN: eternalblue@lge.com:2009-11-11
// 0001905: [ARM9] Sound related AT CMD & Hidden menu added 
#include <linux/eve_at.h>
// END: eternalblue@lge.com:2009-11-11
#include <mach/socinfo.h>
#include <mach/msm_serial_hs.h>

#include "devices.h"
#include "timer.h"
#include "devices-msm7x2xa.h"

#include <mach/board_lge.h>

#include "board-m3mpcs.h"
// [110906 kkh8318@lge.com M3_ALL]Added TestMode interface [START]
#ifdef CONFIG_LGE_DIAGTEST
#include <../../../lge/include/lg_fw_diag_communication.h>
#endif 
// [110906 kkh8318@lge.com M3_ALL] [END]


#ifdef CONFIG_LGE_SILENCE_RESET
/*2011-11-03, sohyun.nam@lge.com add silence reset*/
#include <linux/proc_fs.h>
#include <linux/slab.h>
#include "smd_private.h"


static uint32_t  *smem_sreset =  NULL;
static char *smem_sreset_log;
static size_t smem_sreset_log_size;

#if 0
static ssize_t smem_sreset_read(struct file *file, char __user *buf,
				    size_t len, loff_t *offset)
{
	loff_t pos = *offset;
	ssize_t count;

	if (pos >= smem_sreset_log_size)
		return 0;

	count = min(len, (size_t)(smem_sreset_log_size - pos));
	if (copy_to_user(buf, smem_sreset_log + pos, count))
		return -EFAULT;

	*offset += count;
	return count;
}


static const struct file_operations smem_sreset_file_ops = {
	.owner = THIS_MODULE,
	.read = smem_sreset_read,
}

#endif

int check_smem_ers_status(void)
{
	struct proc_dir_entry *entry;

	smem_sreset = (uint32_t *)smem_alloc(SMEM_ID_VENDOR0, sizeof(uint64_t)*4);
	
	if((smem_sreset != NULL) && ((*smem_sreset ) != 0))
	{
		printk(KERN_INFO "smem_sreset => addr : 0x%X, value : 0x%X\n", (int)smem_sreset, *smem_sreset);
		smem_sreset_log = kzalloc(100, GFP_KERNEL);
		if (smem_sreset_log == NULL) {
			printk(KERN_ERR "smem_sreset_log allocation failed \n");
			smem_sreset_log_size = 0;
			return 0;
		}

	if(*smem_sreset == 0xDDDEADDD)
			{
				//1 create a file to notify kernel crash
				printk(KERN_INFO "Kernel Crash \n");
	
				entry = create_proc_entry("last_kmsg_kernel_crash", S_IFREG | S_IRUGO, NULL);
				if (!entry) {
					printk(KERN_ERR "%s: failed to create proc entry\n", "last_kmsg_kernel_crash");
					if(smem_sreset_log)
						kfree(smem_sreset_log);
					smem_sreset_log = NULL;
					smem_sreset_log_size = 0;
					return 0;
				}
		
				if(smem_sreset_log)
				{
					sprintf(smem_sreset_log, "%s value : 0x%X\n", "kernel crash!!", *smem_sreset);
					smem_sreset_log_size = strlen(smem_sreset_log);
				}
	
				*smem_sreset = 0;
				//entry->proc_fops = &smem_sreset_file_ops;
				//entry->size = smem_sreset_log_size;
		return 1;
		}
	else if(*smem_sreset == 0xDDEAEADD)
		{
				//1 create a file to notify modem crash
				printk(KERN_INFO "Kernel Crash \n");
	
				entry = create_proc_entry("last_kmsg_modem_crash", S_IFREG | S_IRUGO, NULL);
				if (!entry) 
				{
					printk(KERN_ERR "%s: failed to create proc entry\n", "last_kmsg_modem_crash");
					if(smem_sreset_log)
						kfree(smem_sreset_log);
					smem_sreset_log = NULL;
					smem_sreset_log_size = 0;
					return 0;
				}
		
				if(smem_sreset_log)
				{
					sprintf(smem_sreset_log, "%s value : 0x%X\n", "modem crash!!", *smem_sreset);
					smem_sreset_log_size = strlen(smem_sreset_log);
				}
	
				*smem_sreset = 0;
				//entry->proc_fops = &smem_sreset_file_ops;
				//entry->size = smem_sreset_log_size;
		return 1;
		}
	}
			
	return 0;
}
			
#endif /*CONFIG_LGE_SILENCE_RESET*/




static struct msm_gpio qup_i2c_gpios_io[] = {
	{ GPIO_CFG(60, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA),
		"qup_scl" },
	{ GPIO_CFG(61, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA),
		"qup_sda" },
	{ GPIO_CFG(131, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA),
		"qup_scl" },
	{ GPIO_CFG(132, 0, GPIO_CFG_OUTPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA),
		"qup_sda" },
};

static struct msm_gpio qup_i2c_gpios_hw[] = {
	{ GPIO_CFG(60, 1, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA),
		"qup_scl" },
	{ GPIO_CFG(61, 1, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA),
		"qup_sda" },
	{ GPIO_CFG(131, 2, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA),
		"qup_scl" },
	{ GPIO_CFG(132, 2, GPIO_CFG_INPUT, GPIO_CFG_NO_PULL, GPIO_CFG_8MA),
		"qup_sda" },
};

static void gsbi_qup_i2c_gpio_config(int adap_id, int config_type)
{
	int rc;

	if (lge_bd_rev == LGE_REV_A) {
		if (adap_id < 0 || adap_id > 0)
			return;
	} else if (lge_bd_rev == LGE_REV_B) {
		if (adap_id < 0 || adap_id > 1)
			return;
	} else {
		if (adap_id < 0 || adap_id > 1)
			return;
	}

	/* Each adapter gets 2 lines from the table */
	if (config_type)
		rc = msm_gpios_request_enable(&qup_i2c_gpios_hw[adap_id * 2], 2);
	else
		rc = msm_gpios_request_enable(&qup_i2c_gpios_io[adap_id * 2], 2);
	if (rc < 0)
		pr_err("QUP GPIO request/enable failed: %d\n", rc);
}

static struct msm_i2c_platform_data msm_gsbi0_qup_i2c_pdata = {
	.clk_freq		= 100000,
	.clk			= "gsbi_qup_clk",
	.pclk			= "gsbi_qup_pclk",
	.msm_i2c_config_gpio	= gsbi_qup_i2c_gpio_config,
};

static struct msm_i2c_platform_data msm_gsbi1_qup_i2c_pdata = {
	.clk_freq		= 100000,
	.clk			= "gsbi_qup_clk",
	.pclk			= "gsbi_qup_pclk",
	.msm_i2c_config_gpio	= gsbi_qup_i2c_gpio_config,
};

static struct resource resources_uart3[] = {
	{
		.start	= INT_UART3,
		.end	= INT_UART3,
		.flags	= IORESOURCE_IRQ,
	},
	{
		.start	= MSM_UART3_PHYS,
		.end	= MSM_UART3_PHYS + MSM_UART3_SIZE - 1,
		.flags	= IORESOURCE_MEM,
	},
};

#ifdef CONFIG_SERIAL_MSM_HS
static struct msm_serial_hs_platform_data msm_uart_dm1_pdata = {
	.inject_rx_on_wakeup       = 1,
	.rx_to_inject       = 0xFD,
};
#endif

struct platform_device msm_device_uart3 = {
	.name	= "msm_serial",
	.id	= 2,
	.num_resources	= ARRAY_SIZE(resources_uart3),
	.resource	= resources_uart3,
};

static struct msm_acpu_clock_platform_data msm7x2x_clock_data = {
	.acpu_switch_time_us = 50,
	.max_speed_delta_khz = 400000,
	.vdd_switch_time_us = 62,
	.max_axi_khz = 200000,
};

// BEGIN: eternalblue@lge.com:2009-11-11
// 0001905: [ARM9] Sound related AT CMD & Hidden menu added 
static struct atcmd_platform_data eve_atcmd_pdata = {
	.name = "eve_atcmd",
};

static struct platform_device eve_atcmd_device = {
	.name = "eve_atcmd",
	.id = -1,
	.dev    = {
		.platform_data = &eve_atcmd_pdata
	},
}; 
// END: eternalblue@lge.com:2009-11-11
// [110906 kkh8318@lge.com M3_ALL]Added TestMode interface [START]
#ifdef CONFIG_LGE_DIAGTEST
// MOD 0009214: [DIAG] LG Diag feature added in side of android
static struct diagcmd_platform_data lg_fw_diagcmd_pdata = {
	.name = "lg_fw_diagcmd",
};

static struct platform_device lg_fw_diagcmd_device = {
	.name = "lg_fw_diagcmd",
	.id = -1,
	.dev = {
		.platform_data = &lg_fw_diagcmd_pdata
	},
};

static struct platform_device lg_diag_cmd_device = {
	.name = "lg_diag_cmd",
	.id = -1,
	.dev = {
		.platform_data = 0, //&lg_diag_cmd_pdata
	},
};
#endif
// [110906 kkh8318@lge.com M3_ALL] [END]
static struct platform_device *m3_devices[] __initdata = {
	&msm_device_dmov,
	&msm_device_smd,
	&msm_device_uart_dm1,
	&msm_gsbi0_qup_i2c_device,
	&msm_gsbi1_qup_i2c_device,
	&msm_kgsl_3d0,
// BEGIN: eternalblue@lge.com:2009-11-11
// 0001905: [ARM9] Sound related AT CMD & Hidden menu added 
	&eve_atcmd_device, //vlc	
// END: eternalblue@lge.com:2009-11-11
// [110906 kkh8318@lge.com M3_ALL]Added TestMode interface [START]
#ifdef CONFIG_LGE_DIAGTEST
		&lg_fw_diagcmd_device,	
		&lg_diag_cmd_device,
#endif 
// [110906 kkh8318@lge.com M3_ALL] [END]
};

static void __init msm_device_i2c_init(void)
{
	msm_gsbi0_qup_i2c_device.dev.platform_data = &msm_gsbi0_qup_i2c_pdata;
	msm_gsbi1_qup_i2c_device.dev.platform_data = &msm_gsbi1_qup_i2c_pdata;
}

#define MSM_EBI2_PHYS			0xa0d00000
#define MSM_EBI2_XMEM_CS2_CFG1		0xa0d10030

static void __init msm7x27a_init_ebi2(void)
{
	uint32_t ebi2_cfg;
	void __iomem *ebi2_cfg_ptr;

	ebi2_cfg_ptr = ioremap_nocache(MSM_EBI2_PHYS, sizeof(uint32_t));
	if (!ebi2_cfg_ptr)
		return;

	ebi2_cfg = readl(ebi2_cfg_ptr);
	ebi2_cfg |= (1 << 4); /* CS2 */

	writel(ebi2_cfg, ebi2_cfg_ptr);
	iounmap(ebi2_cfg_ptr);

	/* Enable A/D MUX[bit 31] from EBI2_XMEM_CS2_CFG1 */
	ebi2_cfg_ptr = ioremap_nocache(MSM_EBI2_XMEM_CS2_CFG1,
							 sizeof(uint32_t));
	if (!ebi2_cfg_ptr)
		return;

	ebi2_cfg = readl(ebi2_cfg_ptr);
	ebi2_cfg |= (1 << 31);

	writel(ebi2_cfg, ebi2_cfg_ptr);
	iounmap(ebi2_cfg_ptr);
}

#define UART1DM_RX_GPIO         45
static void __init msm7x2x_init(void)
{
	if (socinfo_init() < 0)
		printk(KERN_ERR "%s: socinfo_init() failed!\n",
		       __func__);

	msm_clock_init(msm_clocks_7x27a, msm_num_clocks_7x27a);
	msm_acpu_clock_init(&msm7x2x_clock_data);

	/* Common functions for SURF/FFA/RUMI3 */
	msm_device_i2c_init();

	msm7x27a_init_ebi2();
#ifdef CONFIG_SERIAL_MSM_HS
	msm_uart_dm1_pdata.wakeup_irq = gpio_to_irq(UART1DM_RX_GPIO);
	msm_device_uart_dm1.dev.platform_data = &msm_uart_dm1_pdata;
#endif

	msm_add_pmem_devices();
	msm_add_fb_device();

	platform_add_devices(m3_devices,
		ARRAY_SIZE(m3_devices));

	/*7x25a kgsl initializations*/
	msm7x25a_kgsl_3d0_init();

	if (lge_get_uart_mode()) {
		if (lge_bd_rev == LGE_REV_A)
			platform_device_register(&msm_device_uart3);
		else
			platform_device_register(&msm_device_uart1);
	}

	lge_add_input_devices();
	lge_add_misc_devices();
	lge_add_mmc_devices();
	lge_add_sound_devices();
	lge_add_lcd_devices();
	lge_add_camera_devices();
#ifndef CONFIG_MACH_MSM7X27A_M3MPCS_REV_A
	lge_add_pm_devices();
#endif
	lge_add_usb_devices();
	lge_add_connectivity_devices(); /* suhui.kim@lge.com, for Bluetooth/FM */

	/* gpio i2c devices should be registered at latest point */
	lge_add_gpio_i2c_devices();

	lge_add_ramconsole_devices();
#if defined(CONFIG_ANDROID_RAM_CONSOLE) && defined(CONFIG_LGE_HANDLE_PANIC)
	lge_add_panic_handler_devices();
#endif

/* murali.ramaiah@lge.com [2011-09-22] - Read power on status and update boot reason */
#ifdef CONFIG_LGE_POWER_ON_STATUS_PATCH
	lge_board_pwr_on_status();
#endif

#ifdef CONFIG_LGE_SILENCE_RESET
	check_smem_ers_status();
#endif
}

static void __init msm7x2x_init_early(void)
{
	msm_msm7x2x_allocate_memory_regions();
}

MACHINE_START(MSM7X27A_M3MPCS, "LGE MSM7x27a M3MPCS")
	.boot_params	= PHYS_OFFSET + 0x100,
	.map_io		= msm_common_io_init,
	.reserve	= msm7x27a_reserve,
	.init_irq	= msm_init_irq,
	.init_machine	= msm7x2x_init,
	.timer		= &msm_timer,
	.init_early     = msm7x2x_init_early,
MACHINE_END
