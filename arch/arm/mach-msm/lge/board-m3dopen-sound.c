#include <linux/init.h>
#include <linux/platform_device.h>
#include <linux/gpio.h>
#include <linux/input.h>

#include <mach/board.h>
#include <mach/board_lge.h>

#include "board-m3eu.h"

#define SND(desc, num) { .name = #desc, .id = num }
static struct snd_endpoint snd_endpoints_list[] = {
	SND(HANDSET_LOOPBACK,5),
	SND(HANDSET, 6),
	SND(HEADSET_LOOPBACK, 1),
	SND(HEADSET, 2),
	SND(HEADSET_STEREO, 3),
	SND(SPEAKER, 0),
	SND(SPEAKER_IN_CALL, 7),
	SND(SPEAKER_RING, 8),
	SND(HEADSET_AND_SPEAKER, 8),
	SND(FM_HEADSET, 10),
	SND(FM_SPEAKER, 11),
	SND(BT, 13),
	SND(TTY_HEADSET, 15),
	SND(TTY_VCO, 16),
	SND(TTY_HCO, 17),
	SND(TTY_HCO_SPEAKER, 18),
	SND(HANDSET_VR, 20),
	SND(HEADSET_VR, 21),
	SND(BT_VR, 23),
	SND(HEADSET_WITHOUT_MIC, 4),
	SND(HEADSET_WITHOUT_MIC_VR, 22),
	SND(SPEAKER_LOOPBACK, 29),
	SND(CURRENT, 31),
};
#undef SND

static struct msm_snd_endpoints msm_device_snd_endpoints = {
	.endpoints = snd_endpoints_list,
	.num = sizeof(snd_endpoints_list) / sizeof(struct snd_endpoint)
};

static struct platform_device msm_device_snd = {
	.name = "msm_snd",
	.id = -1,
	.dev    = {
		.platform_data = &msm_device_snd_endpoints
	},
};

#define DEC0_FORMAT ((1<<MSM_ADSP_CODEC_MP3)| \
	(1<<MSM_ADSP_CODEC_AAC)|(1<<MSM_ADSP_CODEC_WMA)| \
	(1<<MSM_ADSP_CODEC_WMAPRO)|(1<<MSM_ADSP_CODEC_AMRWB)| \
	(1<<MSM_ADSP_CODEC_AMRNB)|(1<<MSM_ADSP_CODEC_WAV)| \
	(1<<MSM_ADSP_CODEC_ADPCM)|(1<<MSM_ADSP_CODEC_YADPCM)| \
	(1<<MSM_ADSP_CODEC_EVRC)|(1<<MSM_ADSP_CODEC_QCELP))
#define DEC1_FORMAT ((1<<MSM_ADSP_CODEC_MP3)| \
	(1<<MSM_ADSP_CODEC_AAC)|(1<<MSM_ADSP_CODEC_WMA)| \
	(1<<MSM_ADSP_CODEC_WMAPRO)|(1<<MSM_ADSP_CODEC_AMRWB)| \
	(1<<MSM_ADSP_CODEC_AMRNB)|(1<<MSM_ADSP_CODEC_WAV)| \
	(1<<MSM_ADSP_CODEC_ADPCM)|(1<<MSM_ADSP_CODEC_YADPCM)| \
	(1<<MSM_ADSP_CODEC_EVRC)|(1<<MSM_ADSP_CODEC_QCELP))
#define DEC2_FORMAT ((1<<MSM_ADSP_CODEC_MP3)| \
	(1<<MSM_ADSP_CODEC_AAC)|(1<<MSM_ADSP_CODEC_WMA)| \
	(1<<MSM_ADSP_CODEC_WMAPRO)|(1<<MSM_ADSP_CODEC_AMRWB)| \
	(1<<MSM_ADSP_CODEC_AMRNB)|(1<<MSM_ADSP_CODEC_WAV)| \
	(1<<MSM_ADSP_CODEC_ADPCM)|(1<<MSM_ADSP_CODEC_YADPCM)| \
	(1<<MSM_ADSP_CODEC_EVRC)|(1<<MSM_ADSP_CODEC_QCELP))
#define DEC3_FORMAT ((1<<MSM_ADSP_CODEC_MP3)| \
	(1<<MSM_ADSP_CODEC_AAC)|(1<<MSM_ADSP_CODEC_WMA)| \
	(1<<MSM_ADSP_CODEC_WMAPRO)|(1<<MSM_ADSP_CODEC_AMRWB)| \
	(1<<MSM_ADSP_CODEC_AMRNB)|(1<<MSM_ADSP_CODEC_WAV)| \
	(1<<MSM_ADSP_CODEC_ADPCM)|(1<<MSM_ADSP_CODEC_YADPCM)| \
	(1<<MSM_ADSP_CODEC_EVRC)|(1<<MSM_ADSP_CODEC_QCELP))
#define DEC4_FORMAT (1<<MSM_ADSP_CODEC_MIDI)

static unsigned int dec_concurrency_table[] = {
	/* Audio LP */
	(DEC0_FORMAT|(1<<MSM_ADSP_MODE_TUNNEL)|(1<<MSM_ADSP_OP_DMA)), 0,
	0, 0, 0,

	/* Concurrency 1 */
	(DEC0_FORMAT|(1<<MSM_ADSP_MODE_TUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC1_FORMAT|(1<<MSM_ADSP_MODE_TUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC2_FORMAT|(1<<MSM_ADSP_MODE_TUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC3_FORMAT|(1<<MSM_ADSP_MODE_TUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC4_FORMAT),

	 /* Concurrency 2 */
	(DEC0_FORMAT|(1<<MSM_ADSP_MODE_TUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC1_FORMAT|(1<<MSM_ADSP_MODE_TUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC2_FORMAT|(1<<MSM_ADSP_MODE_TUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC3_FORMAT|(1<<MSM_ADSP_MODE_TUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC4_FORMAT),

	/* Concurrency 3 */
	(DEC0_FORMAT|(1<<MSM_ADSP_MODE_TUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC1_FORMAT|(1<<MSM_ADSP_MODE_TUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC2_FORMAT|(1<<MSM_ADSP_MODE_TUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC3_FORMAT|(1<<MSM_ADSP_MODE_NONTUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC4_FORMAT),

	/* Concurrency 4 */
	(DEC0_FORMAT|(1<<MSM_ADSP_MODE_TUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC1_FORMAT|(1<<MSM_ADSP_MODE_TUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC2_FORMAT|(1<<MSM_ADSP_MODE_NONTUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC3_FORMAT|(1<<MSM_ADSP_MODE_NONTUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC4_FORMAT),

	/* Concurrency 5 */
	(DEC0_FORMAT|(1<<MSM_ADSP_MODE_TUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC1_FORMAT|(1<<MSM_ADSP_MODE_NONTUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC2_FORMAT|(1<<MSM_ADSP_MODE_NONTUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC3_FORMAT|(1<<MSM_ADSP_MODE_NONTUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC4_FORMAT),

	/* Concurrency 6 */
	(DEC0_FORMAT|(1<<MSM_ADSP_MODE_NONTUNNEL)|(1<<MSM_ADSP_OP_DM)),
	0, 0, 0, 0,

	/* Concurrency 7 */
	(DEC0_FORMAT|(1<<MSM_ADSP_MODE_NONTUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC1_FORMAT|(1<<MSM_ADSP_MODE_NONTUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC2_FORMAT|(1<<MSM_ADSP_MODE_NONTUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC3_FORMAT|(1<<MSM_ADSP_MODE_NONTUNNEL)|(1<<MSM_ADSP_OP_DM)),
	(DEC4_FORMAT),
};

#define DEC_INFO(name, queueid, decid, nr_codec) { .module_name = name, \
	.module_queueid = queueid, .module_decid = decid, \
	.nr_codec_support = nr_codec}

static struct msm_adspdec_info dec_info_list[] = {
	DEC_INFO("AUDPLAY0TASK", 13, 0, 11), /* AudPlay0BitStreamCtrlQueue */
	DEC_INFO("AUDPLAY1TASK", 14, 1, 11),  /* AudPlay1BitStreamCtrlQueue */
	DEC_INFO("AUDPLAY2TASK", 15, 2, 11),  /* AudPlay2BitStreamCtrlQueue */
	DEC_INFO("AUDPLAY3TASK", 16, 3, 11),  /* AudPlay3BitStreamCtrlQueue */
	DEC_INFO("AUDPLAY4TASK", 17, 4, 1),  /* AudPlay4BitStreamCtrlQueue */
};

static struct msm_adspdec_database msm_device_adspdec_database = {
	.num_dec = ARRAY_SIZE(dec_info_list),
	.num_concurrency_support = (ARRAY_SIZE(dec_concurrency_table) / \
					ARRAY_SIZE(dec_info_list)),
	.dec_concurrency_table = dec_concurrency_table,
	.dec_info_list = dec_info_list,
};

static struct platform_device msm_device_adspdec = {
	.name = "msm_adspdec",
	.id = -1,
	.dev    = {
		.platform_data = &msm_device_adspdec_database
	},
};

/* ear sense driver */
static char *ear_state_string[] = {
	"0",
	"1",
	"2",
};

enum {
	EAR_STATE_EJECT = 0,
	EAR_STATE_INJECT = 1, 
};

enum {
	EAR_EJECT = 0,
	EAR_INJECT = 1,
};

static int m3eu_gpio_earsense_work_func(int *value)
{
	int state;
	int gpio_value;

	msleep(100);
	
	gpio_value = !gpio_get_value(GPIO_EAR_SENSE);
	printk(KERN_INFO "%s: ear sense detected : %s\n", __func__,
		gpio_value ? "injected" : "ejected");

	if (gpio_value == EAR_EJECT) {
		state = EAR_STATE_EJECT;
		*value = 0;
		gpio_set_value(GPIO_MIC_MODE, 0);

	} else {
		state = EAR_STATE_INJECT;
		msleep(50);
		gpio_value = !gpio_get_value(GPIO_BUTTON_DETECT);
		if (gpio_value) {
			printk(KERN_INFO "headphone was inserted!\n");
			*value = SW_HEADPHONE_INSERT;
		} else {
			printk(KERN_INFO "micorphone was inserted!\n");
			*value = SW_MICROPHONE_INSERT;
			gpio_set_value(GPIO_MIC_MODE, 1);
		}
	}

	return state;
}

static char *m3eu_gpio_earsense_print_name(int state)
{
	if (state)
		return "Headset";
	else
		return "No Device";
}

static char *m3eu_gpio_earsense_print_state(int state)
{
	if (state == SW_HEADPHONE_INSERT)
		return ear_state_string[2];
	else if (state == SW_MICROPHONE_INSERT)
		return ear_state_string[1];
	else
		return ear_state_string[0];
}

static int m3eu_gpio_earsense_sysfs_store(const char *buf, size_t size)
{
	int state;

	if (!strncmp(buf, "eject", size - 1))
		state = EAR_STATE_EJECT;
	else if (!strncmp(buf, "inject", size - 1))
		state = EAR_STATE_INJECT;
	else
		return -EINVAL;

	return state;
}

static unsigned m3eu_earsense_gpios[] = {
	GPIO_EAR_SENSE,
};

/* especially to address gpio key */
static unsigned m3eu_hook_key_gpios[] = {
	GPIO_BUTTON_DETECT,
};

static int m3eu_gpio_hook_key_work_func(int *value)
{
	int gpio_value;

	*value = KEY_MEDIA;
	gpio_value = !gpio_get_value(GPIO_BUTTON_DETECT);
	printk(KERN_INFO "%s: hook key detected : %s\n", __func__,
		gpio_value ? "pressed" : "released");

	msleep(150);
	if (gpio_get_value(GPIO_EAR_SENSE)) {
		printk(KERN_INFO "Ignore hook key event\n");
		*value = 0;
	}

	return gpio_value;
}

static struct lge_gpio_switch_platform_data m3eu_earsense_data = {
	.name = "h2w_headset",
	.gpios = m3eu_earsense_gpios,
	.num_gpios = ARRAY_SIZE(m3eu_earsense_gpios),
	.irqflags = IRQF_TRIGGER_RISING | IRQF_TRIGGER_FALLING,
	.wakeup_flag = 1,
	.work_func = m3eu_gpio_earsense_work_func,
	.print_name = m3eu_gpio_earsense_print_name,
	.print_state = m3eu_gpio_earsense_print_state,
	.sysfs_store = m3eu_gpio_earsense_sysfs_store,

	/* especially to address gpio key */
	.key_gpios = m3eu_hook_key_gpios,
	.num_key_gpios = ARRAY_SIZE(m3eu_hook_key_gpios),
	.key_work_func = m3eu_gpio_hook_key_work_func,
};

static struct platform_device m3eu_earsense_device = {
	.name	= "lge-switch-gpio",
	.id		= -1,
	.dev	= {
		.platform_data = &m3eu_earsense_data,
	},
};

/* input platform device */
static struct platform_device *m3eu_sound_devices[] __initdata = {
	&msm_device_snd,
	&msm_device_adspdec,
	&m3eu_earsense_device,
};

/* common function */
void __init lge_add_sound_devices(void)
{
	int rc;

	rc = gpio_request(GPIO_MIC_MODE, "mic_en");
	if (rc) {
		printk(KERN_ERR "%d gpio request is failed\n", GPIO_MIC_MODE);
	} else {
		rc = gpio_tlmm_config(GPIO_CFG(GPIO_MIC_MODE, 0, GPIO_CFG_OUTPUT,
				GPIO_CFG_NO_PULL, GPIO_CFG_2MA), GPIO_CFG_ENABLE);
		if (rc)
			printk(KERN_ERR "%d gpio tlmm config is failed\n", GPIO_MIC_MODE);
	}

	platform_add_devices(m3eu_sound_devices, ARRAY_SIZE(m3eu_sound_devices));
}

