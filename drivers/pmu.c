#include <types.h>
#include <coproc.h>
#include <drivers/pmu.h>
#include <kern/printk.h>

void pmu_enable(void) {
	uint32_t bits = PMU_PMNC_E | PMU_PMNC_P | PMU_PMNC_C;
	uint32_t mask = ~(PMU_PMNC_D | PMU_PMNC_X | PMU_PMNC_DP);
	pmu_pmnc_write((pmu_pmnc_read() | bits) & mask);
	pmu_useren_write(PMU_USEREN_EN);
	printk("pmu init\n");
}

void pmu_disable(void) {
	pmu_pmnc_write(pmu_pmnc_read() & ~PMU_PMNC_E);
}
