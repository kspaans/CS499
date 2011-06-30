#ifndef KERN_PMU_H
#define KERN_PMU_H

#include <coproc.h>

#define pmu_pmnc_read()       read_coproc(p15, 0, c9, c12, 0)
#define pmu_pmnc_write(v)    write_coproc(p15, 0, c9, c12, 0, v)
#define pmu_cntens_read()     read_coproc(p15, 0, c9, c12, 1)
#define pmu_cntens_write(v)  write_coproc(p15, 0, c9, c12, 1, v)
#define pmu_cntenc_read()     read_coproc(p15, 0, c9, c12, 2)
#define pmu_cntenc_write(v)  write_coproc(p15, 0, c9, c12, 2, v)
#define pmu_flag_read()       read_coproc(p15, 0, c9, c12, 3)
#define pmu_flag_write(v)    write_coproc(p15, 0, c9, c12, 3, v)
#define pmu_swincr_read()     read_coproc(p15, 0, c9, c12, 4)
#define pmu_swincr_write(v)  write_coproc(p15, 0, c9, c12, 4, v)
#define pmu_pmnxsel_read()    read_coproc(p15, 0, c9, c12, 5)
#define pmu_pmnxsel_write(v) write_coproc(p15, 0, c9, c12, 5, v)
#define pmu_ccnt_read()       read_coproc(p15, 0, c9, c13, 0)
#define pmu_ccnt_write(v)    write_coproc(p15, 0, c9, c13, 0, v)
#define pmu_evtsel_read()     read_coproc(p15, 0, c9, c13, 1)
#define pmu_evtsel_write(v)  write_coproc(p15, 0, c9, c13, 1, v)
#define pmu_pmcnt_read()      read_coproc(p15, 0, c9, c13, 2)
#define pmu_pmcnt_write(v)   write_coproc(p15, 0, c9, c13, 2, v)
#define pmu_useren_read()     read_coproc(p15, 0, c9, c14, 0)
#define pmu_useren_write(v)  write_coproc(p15, 0, c9, c14, 0, v)
#define pmu_intens_read()     read_coproc(p15, 0, c9, c14, 1)
#define pmu_intens_write(v)  write_coproc(p15, 0, c9, c14, 1, v)
#define pmu_intenc_read()     read_coproc(p15, 0, c9, c14, 2)
#define pmu_intenc_write(v)  write_coproc(p15, 0, c9, c14, 2, v)

/* performance monitor control register */
#define PMU_PMNC_E    0x00000001 /* enable all counters */
#define PMU_PMNC_P    0x00000002 /* performance counter reset */
#define PMU_PMNC_C    0x00000004 /* cycle counter reset */
#define PMU_PMNC_D    0x00000008 /* cycle count divider */
#define PMU_PMNC_X    0x00000010 /* export events */
#define PMU_PMNC_DP   0x00000020 /* disable cycle counter if debug prohibited */
#define PMU_PMNC_MASK 0x0000003f /* writable bits */

/* count enable set register */
#define PMU_CNTENS_P(i) (1 << (i)) /* PMCNTi enable */
#define PMU_CNTENS_C    0x80000000 /* CCNT enable */
#define PMU_CNTENS_MASK 0x8000000f /* writable bits */

/* count enable clear register */
#define PMU_CNTENC_P(i) (1 << (i)) /* PMCNTi disable */
#define PMU_CNTENC_C    0x80000000 /* CCNT disable */
#define PMU_CNTENC_MASK 0x8000000f /* writable bits */

/* overflow flag status register */
#define PMU_FLAG_P(i) (1 << (i)) /* PMCNTi overflow */
#define PMU_FLAG_C    0x80000000 /* CCNT overflow */
#define PMU_FLAG_MASK 0x8000000f /* writable bits */

/* software increment register */
#define PMU_SWINCR_P(i) (1 << (i)) /* PMCNTi increment */
#define PMU_SWINCR_MASK 0x0000000f /* writable bits */

/* performance counter selection register */
#define PMU_PMNXSEL_P(i) (i)        /* PMCNTi select */
#define PMU_PMNXSEL_MASK 0x00000003 /* writable bits */

/* cycle count register */
#define PMU_CCNT_MASK 0xffffffff /* writable bits */

/* event selection register */
#define PMU_EVTSEL_SW_INCREMENT        0x00 /* software increment */
#define PMU_EVTSEL_PREFETCH_L1_MISS    0x01 /* instruction fetch refills L1 */
#define PMU_EVTSEL_PREFETCH_TLB_MISS   0x02 /* instruction fetch refills TLB */
#define PMU_EVTSEL_DATA_L1_MISS        0x03 /* data read or write refills L1 */
#define PMU_EVTSEL_DATA_L1_HIT         0x04 /* data read or write accesses L1 */
#define PMU_EVTSEL_DATA_TLB_MISS       0x05 /* data read or write refills TLB */
#define PMU_EVTSEL_DATA_READ           0x06 /* data read executed */
#define PMU_EVTSEL_DATA_WRITE          0x07 /* data write executed */
#define PMU_EVTSEL_INSN_EXECUTE        0x08 /* instruction executed */
#define PMU_EVTSEL_EXCEPTION           0x09 /* exception taken */
#define PMU_EVTSEL_EXCEPTION_RETURN    0x0a /* exception return executed */
#define PMU_EVTSEL_CONTEXTID_WRITE     0x0b /* context id register written */
#define PMU_EVTSEL_SW_PC_CHANGE        0x0c /* software change to the program counter */
#define PMU_EVTSEL_IMMED_BRANCH        0x0d /* immediate branch executed, taken or not */
#define PMU_EVTSEL_PRECEDURE_RETURN    0x0e /* procedure return other than exception */
#define PMU_EVTSEL_UNALIGNED_ACCESS    0x0f /* unaligned access executed */
#define PMU_EVTSEL_BRANCH_MISPREDICT   0x10 /* branch mispredicted or not predicted */
#define PMU_EVTSEL_CYCLE_COUNT         0x11 /* every cycle */
#define PMU_EVTSEL_BRANCH_PREDICT      0x12 /* branch that could have been predicted */
#define PMU_EVTSEL_WRITE_BUFFER_CYCLE  0x40 /* any write buffer full cycle */
#define PMI_EVTSEL_L2_STORE_MERGED     0x41 /* any store that is merged in L2 */
#define PMI_EVTSEL_L2_BUFFERABLE_STORE 0x42 /* any bufferable store from load/store to L2 */
#define PMU_EVTSEL_L2_ACCESS           0x43 /* any access to L2 */
#define PMU_EVTSEL_L2_MISS             0x44 /* any cacheable miss in L2 */
#define PMU_EVTSEL_AXI_READ_TRANSFER   0x45 /* number of AXI read transfers */
#define PMU_EVTSEL_AXI_WRITE_TRANSFER  0x46 /* number of AXI write transfers */
#define PMU_EVTSEL_MEMORY_REPLAY       0x47 /* replay event in memory system */
#define PMU_EVTSEL_UNALIGNED_REPLAY    0x48 /* unaligned access causes replay */
#define PMU_EVTSEL_L1_DATA_HASH_MISS   0x49 /* L1 instruction misses due to hashing */
#define PMU_EVTSEL_L1_INSN_HASH_MISS   0x4a /* L1 data access misses due to hashing */
#define PMU_EVTSEL_L1_DATA_PAGE_ALIAS  0x4b /* L1 data access page coloring alias occurs */
#define PMU_EVTSEL_L1_NEON_HIT         0x4c /* NEON access hits L1 */
#define PMU_EVTSEL_L1_NEON_ACCESS      0x4d /* NEON cacheable accesses L1 */
#define PMU_EVTSEL_L2_NEON_ACCESS      0x4e /* NEON cacheable accesses L2 */
#define PMU_EVTSEL_L2_NEON_HIT         0x4f /* NEON access hits L2 */
#define PMU_EVTSEL_L1_ICACHE_ACCESS    0x50 /* L1 instruction cache access */
#define PMU_EVTSEL_RETURN_MISPREDICT   0x51 /* return stack pop mispredict */
#define PMU_EVTSEL_BRANCH_WRONGPREDICT 0x52 /* branch predicted but wrong */
#define PMU_EVTSEL_PREDICT_TAKEN       0x53 /* predictable branch predicted to be taken */
#define PMU_EVTSEL_PREDICT_EXEC        0x54 /* predictable branch executed and taken */
#define PMU_EVTSEL_OPERATION           0x55 /* micro-operations issued */
#define PMU_EVTSEL_INSN_UNAVAIL        0x56 /* cycles waiting for instructions */
#define PMU_EVTSEL_INSN_ISSUED         0x57 /* instruction executed in a cycle */
#define PMU_EVTSEL_NEON_MRC_STALL      0x58 /* cycles stalled waiting on NEON mrc */
#define PMU_EVTSEL_NEON_QUEUE_STALL    0x59 /* cycles stalled waiting on NEON queues */
#define PMU_EVTSEL_CPU_OR_NEON_BUSY    0x5a /* cycles CPU and NEON both not idle */
#define PMU_EVTSEL_EXTERNAL_0          0x70 /* external input source 0 */
#define PMU_EVTSEL_EXTERNAL_1          0x71 /* external input source 1 */
#define PMU_EVTSEL_EXTERNAL_2          0x72 /* external input source 2 */
#define PMU_EVTSEL_MASK                0xff /* writable bits */

/* performance monitor count register */
#define PMU_PMCNT_MASK  0xffffffff

/* user enable register */
#define PMU_USEREN_EN   0x00000001 /* user mode enable */
#define PMU_USEREN_MASK 0x00000001 /* writable bits */

/* interrupt enable set register */
#define PMU_INTENS_P(i) (1 << (i)) /* PMCNTi interrupt enable */
#define PMU_INTENS_C    0x80000000 /* CCNT interrupt enable */
#define PMU_INTENS_MASK 0x8000000f /* writable bits */

/* interrupt disable set register */
#define PMU_INTENC_P(i) (1 << (i)) /* PMCNTi interrupt disable */
#define PMU_INTENC_C    0x80000000 /* CCNT interrupt disable */
#define PMU_INTENC_MASK 0x8000000f /* writable bits */

void pmu_enable(void);
void pmu_disable(void);
void pmu_cycle_counter_enable(void);
void pmu_cycle_counter_disable(void);
uint32_t pmu_cycle_counter_value(void);
void pmu_counter_event(int counter, int event);
void pmu_counter_reset(int counter);
void pmu_counter_disable(int counter);
void pmu_counter_enable(int counter);
uint32_t pmu_counter_value(int counter);

uint32_t pmu_counter_setup(int counter, int event);

#endif
