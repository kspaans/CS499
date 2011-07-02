#ifndef KERN_CPU_H
#define KERN_CPU_H

#define DEVICEID_SKUID_OFFSET 0xc
#define DEVICEID_SKUID_CPUSPEED_MASK 0x00000008
#define DEVICEID_SKUID_CPUSPEED_720  0x00000008
#define DEVICEID_SKUID_CPUSPEED_600  0x00000000

#define PRM_CLKSEL1_PLL_MPU_OFFSET 0x040

void init_cpu_clock(void);
void init_cache(void);
void init_mmu(void);
void deinit_mmu(void);
void set_ttbr(uint32_t *pagetable);
void set_dacr(uint32_t dacr);

#endif /* KERN_CPU_H */
