#include <kern/printk.h>
#include <types.h>
#include <string.h>
#include <arm.h>

struct regbits {
	uint32_t mask;
	const char *name;
	const char *desc;
	const char **extra;
	void (*describe)(const struct regbits *bits, uint32_t val);
};

struct regdesc {
	const char *name;
	struct regbits bits[32];
};

void describe_enable_bit(const struct regbits *bits, uint32_t val) {
	printk("%s:%d - %s - %s", bits->name, !!val,
		bits->desc, val ? "enabled" : "disabled");
}

void describe_select_bit(const struct regbits *bits, uint32_t val) {
	printk("%s:%d - %s - %s", bits->name, !!val,
		bits->desc, bits->extra[!!val]);
}

void describe_hexa_field(const struct regbits *bits, uint32_t val) {
	printk("%s:0x%x - %s", bits->name, val, bits->desc);
}

void describe_cpsr_mode(const struct regbits *bits, uint32_t val) {
	const char *modename;
	switch (val) {
		case ARM_MODE_USER:
			modename = "usr";
			break;
		case ARM_MODE_FIQ:
			modename = "fiq";
			break;
		case ARM_MODE_IRQ:
			modename = "fiq";
			break;
		case ARM_MODE_ABORT:
			modename = "abt";
			break;
		case ARM_MODE_UNDEF:
			modename = "und";
			break;
		case ARM_MODE_SVC:
			modename = "svc";
			break;
		case ARM_MODE_SYSTEM:
			modename = "sys";
			break;
		default:
			modename = "???";
	}

	printk("%s:0x%x - %s - %s", bits->name, val, bits->desc, modename);
}

#define CUST_FIELD(m, name, desc, extra, func) { m, name, desc, extra, func }
#define HEXA_FIELD(m, name, desc) { m, name, desc, NULL, describe_hexa_field }
#define ENABLE_BIT(i, name, desc) { 1 << i, name, desc, NULL, describe_enable_bit }
#define SELECT_BIT(i, name, desc, d0, d1) { 1 << i, name, desc, (const char*[]) { d0, d1 }, describe_select_bit }
#define UNUSED_BIT(i) { 1 << i }

const struct regdesc psr_desc = {
	"psr",
	{
		CUST_FIELD(ARM_MODE_MASK, "m", "processor mode", NULL, describe_cpsr_mode),
		ENABLE_BIT(5, "t", "thumb mode"),
		ENABLE_BIT(6, "f", "mask fiq"),
		ENABLE_BIT(7, "i", "mask irq"),
		ENABLE_BIT(8, "a", "mask asynchronous abort"),
		SELECT_BIT(9, "e", "endianness execution state", "little", "big"),
		HEXA_FIELD(0x0600fc00, "it", "thumb IT state"),
		HEXA_FIELD(0x000f0000, "ge", "simd flags"),
		UNUSED_BIT(20),
		UNUSED_BIT(21),
		UNUSED_BIT(22),
		UNUSED_BIT(23),
		ENABLE_BIT(24, "j", "jazelle mode"),
		ENABLE_BIT(27, "q", "cumulative saturation flag"),
		ENABLE_BIT(28, "v", "overflow condition flag"),
		ENABLE_BIT(29, "c", "carry condition flag"),
		ENABLE_BIT(30, "z", "zero condition flag"),
		ENABLE_BIT(31, "n", "negative condition flag"),

	},
};

const struct regdesc sctlr_desc = {
	"sctlr",
	{
		ENABLE_BIT(0, "m", "mmu"),
		ENABLE_BIT(1, "a", "alignment fault checking"),
		ENABLE_BIT(2, "c", "data and unified caches"),
		ENABLE_BIT(3, "w", "write buffer"),		/* obsolete SBO */
		UNUSED_BIT(4),
		UNUSED_BIT(5),
		UNUSED_BIT(6),
		SELECT_BIT(7, "b", "memory endianness",		/* obsolete SBZ */
				   "little", "be-32"),
		ENABLE_BIT(8, "s", "system protection"),	/* obsolete SBZ */
		ENABLE_BIT(9, "r", "rom protection"),		/* obsolete SBZ */
		ENABLE_BIT(10, "sw", "swp instructions"),
		ENABLE_BIT(11, "z", "branch prediction"),
		ENABLE_BIT(12, "i", "instruction cache"),
		ENABLE_BIT(13, "v", "high vectors"),
		ENABLE_BIT(14, "rr", "round-robin replacement"),
		ENABLE_BIT(15, "l4", "inhibit thumb interwork"), /* obsolete SBZ */
		UNUSED_BIT(16),
		ENABLE_BIT(17, "ha", "hw manages access flag"),
		UNUSED_BIT(18),
		UNUSED_BIT(19),
		UNUSED_BIT(20),
		ENABLE_BIT(21, "fi", "low latency interrupts"),
		ENABLE_BIT(22, "u", "unaligned access"),	/* obsolete SBO */
		UNUSED_BIT(23),
		ENABLE_BIT(24, "ve", "vectored interrupts"),
		SELECT_BIT(25, "ee", "exception endianness",
				     "little", "big"),
		UNUSED_BIT(26),
		ENABLE_BIT(27, "nmfi", "non-maskable fiq"),
		ENABLE_BIT(28, "tre", "tex remap"),
		ENABLE_BIT(29, "afe", "access flag"),
		ENABLE_BIT(30, "te", "thumb exception handlers"),
		UNUSED_BIT(31),
	},
};

static uint32_t read_sctlr(void) {
	uint32_t sctlr;
	asm("mrc p15, 0, %0, c1, c0, 0" : "=r" (sctlr));
	return sctlr;
}

static uint32_t read_cpsr(void) {
	uint32_t cpsr;
	asm("mrs %0, cpsr" : "=r" (cpsr));
	return cpsr;
}

static void dump_register(const struct regdesc *reg, uint32_t value) {
	printk("register %s value 0x%08x\n", reg->name, value);
	uint32_t testmask = 0;
	for (int i = 0; i < arraysize(reg->bits); ++i) {
		uint32_t bitsval = value & reg->bits[i].mask;
		if (reg->bits[i].mask && reg->bits[i].describe) {
			printk("  ");
			reg->bits[i].describe(&reg->bits[i], bitsval);
			printk("\n");
		}

		if (testmask & bitsval)
			panic("overlapping fields in register description");
		testmask |= reg->bits[i].mask;
	}

	if (testmask != -1)
		panic("missing bits in register description: %08x", testmask);
}

void cpu_info(void) {
	dump_register(&sctlr_desc, read_sctlr());
	dump_register(&psr_desc, read_cpsr());
}
