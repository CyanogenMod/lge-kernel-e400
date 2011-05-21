#ifndef __ASM_ARCH_MSM_BOARD_LGE_H
#define __ASM_ARCH_MSM_BOARD_LGE_H

#if __GNUC__
#define __WEAK __attribute__((weak))
#endif

#define PMEM_KERNEL_EBI1_SIZE	0x1C000
#define MSM_PMEM_AUDIO_SIZE	0x5B000

#ifdef CONFIG_ARCH_MSM7X27A
#define MSM_PMEM_MDP_SIZE       0x1DD1000
#define MSM_PMEM_ADSP_SIZE      0x1000000
#define MSM_FB_SIZE             0x195000
#endif

void __init msm_msm7x2x_allocate_memory_regions(void);
void __init msm7x27a_reserve(void);

#endif
