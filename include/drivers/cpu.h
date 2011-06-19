#ifndef CPU_DRIVER_H
#define CPU_DRIVER_H

void init_cache();
void init_mmu();
void deinit_mmu();
void set_ttbr(uint32_t *pagetable);
void set_dacr(uint32_t dacr);

#endif /* CPU_DRIVER_H */
