#ifndef COPROC_H
#define COPROC_H

#define read_coproc(coproc, opc1, crn, crm, opc2) \
	({ uint32_t val; asm volatile("mrc " #coproc ", " #opc1 ", %0, " #crn ", " #crm ", " #opc2 : "=r" (val)); val; })

#define write_coproc(coproc, opc1, crn, crm, opc2, val) \
	asm volatile("mcr " #coproc ", " #opc1 ", %0, " #crn ", " #crm ", " #opc2 :: "r" (val))

#endif

