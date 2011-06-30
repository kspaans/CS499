#include <types.h>
#include <coproc.h>
#include <drivers/pmu.h>
#include <kern/printk.h>

void pmu_enable(void) {
	uint32_t bits = PMU_PMNC_E | PMU_PMNC_P | PMU_PMNC_C;
	uint32_t mask = ~(PMU_PMNC_D | PMU_PMNC_X | PMU_PMNC_DP);
	pmu_pmnc_write((pmu_pmnc_read() | bits) & mask);
	pmu_useren_write(PMU_USEREN_EN);
}

void pmu_disable(void) {
	pmu_pmnc_write(pmu_pmnc_read() & ~PMU_PMNC_E);
}

void pmu_cycle_counter_enable(void) {
	pmu_cntens_write(PMU_CNTENS_C);
}

void pmu_cycle_counter_disable(void) {
	pmu_cntenc_write(PMU_CNTENC_C);
}

uint32_t pmu_cycle_counter_value(void) {
	return pmu_ccnt_read();
}

void pmu_counter_event(int counter, int event) {
	pmu_pmnxsel_write(PMU_PMNXSEL_P(counter) & PMU_PMNXSEL_MASK);
	pmu_evtsel_write(event & PMU_EVTSEL_MASK);
}

void pmu_counter_reset(int counter) {
	pmu_pmnxsel_write(PMU_PMNXSEL_P(counter) & PMU_PMNXSEL_MASK);
	pmu_pmcnt_write(0);
}

void pmu_counter_disable(int counter) {
	pmu_cntenc_write(PMU_CNTENC_P(counter) & PMU_CNTENC_MASK);
}

void pmu_counter_enable(int counter) {
	pmu_cntens_write(PMU_CNTENC_P(counter) & PMU_CNTENC_MASK);
}

uint32_t pmu_counter_value(int counter) {
	pmu_pmnxsel_write(PMU_PMNXSEL_P(counter) & PMU_PMNXSEL_MASK);
	return pmu_pmcnt_read();
}

uint32_t pmu_counter_setup(int counter, int event) {
	pmu_counter_disable(counter);
	pmu_counter_reset(counter);
	pmu_counter_event(counter, event);
	uint32_t ret = pmu_counter_value(counter);
	pmu_counter_enable(counter);
	return ret;
}
