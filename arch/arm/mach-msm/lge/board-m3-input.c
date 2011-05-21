#include <linux/init.h>
#include <linux/platform_device.h>
#include <mach/rpc_server_handset.h>

/* handset device */
static struct msm_handset_platform_data hs_platform_data = {
	.hs_name = "7k_handset",
	.pwr_key_delay_ms = 500, /* 0 will disable end key */
};

static struct platform_device hs_pdev = {
	.name   = "msm-handset",
	.id     = -1,
	.dev    = {
		.platform_data = &hs_platform_data,
	},
};

/* input platform device */
static struct platform_device *m3_input_devices[] __initdata = {
	&hs_pdev,
};

/* common function */
void __init lge_add_input_devices(void)
{
	platform_add_devices(m3_input_devices, ARRAY_SIZE(m3_input_devices));
}
