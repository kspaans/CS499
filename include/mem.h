#ifndef MEM_H
#define MEM_H

#include <inttypes.h>

#define mem8(addr)  (*(volatile uint8_t  *)(addr))
#define mem16(addr) (*(volatile uint16_t *)(addr))
#define mem32(addr) (*(volatile uint32_t *)(addr))

#define write8(addr, val)  ({ (void)(mem8(addr)  = (uint8_t) val); })
#define write16(addr, val) ({ (void)(mem16(addr) = (uint16_t) val); })
#define write32(addr, val) ({ (void)(mem32(addr) = (uint32_t) val); })

#define read8(addr)  ({ uint8_t  val = mem8(addr);  val; })
#define read16(addr) ({ uint16_t val = mem16(addr); val; })
#define read32(addr) ({ uint32_t val = mem32(addr); val; })

#endif
