/*
 * linux/arch/arm/kernel/etm.c
 *
 * Driver for ARM's Embedded Trace Macrocell and Embedded Trace Buffer.
 *
 * Copyright (C) 2009 Nokia Corporation.
 * Alexander Shishkin
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include <types.h>
#include <drivers/etm.h>
#include <errno.h>
#include <kern/printk.h>
#include <panic.h>

extern char text_start;
extern char text_end;

/*
 * ETM tracer state
 */
struct tracectx {
	unsigned int	etb_bufsz;
	intptr_t	etb_regs;
	intptr_t        etm_regs;
	unsigned long	flags;
	int		ncmppairs;
	int		etm_portsz;
};

static struct tracectx tracer;

static inline bool trace_isrunning(struct tracectx *t)
{
	return !!(t->flags & TRACER_RUNNING);
}

static int etm_setup_address_range(struct tracectx *t, int n,
		unsigned long start, unsigned long end, int exclude, int data)
{
	uint32_t flags = ETMAAT_ARM | ETMAAT_IGNCONTEXTID | ETMAAT_NSONLY | \
		    ETMAAT_NOVALCMP;

	if (n < 1 || n > t->ncmppairs)
		return EINVAL;

	/* comparators and ranges are numbered starting with 1 as opposed
	 * to bits in a word */
	n--;

	if (data)
		flags |= ETMAAT_DLOADSTORE;
	else
		flags |= ETMAAT_IEXEC;

	/* first comparator for the range */
	etm_writel(t, flags, ETMR_COMP_ACC_TYPE(n * 2));
	etm_writel(t, start, ETMR_COMP_VAL(n * 2));

	/* second comparator is right next to it */
	etm_writel(t, flags, ETMR_COMP_ACC_TYPE(n * 2 + 1));
	etm_writel(t, end, ETMR_COMP_VAL(n * 2 + 1));

	flags = exclude ? ETMTE_INCLEXCL : 0;
	etm_writel(t, flags | (1 << n), ETMR_TRACEENCTRL);

	return 0;
}

void trace_start(void)
{
	struct tracectx *t = &tracer;
	uint32_t v;
	unsigned long timeout = TRACER_TIMEOUT;

	etb_unlock(t);

	etb_writel(t, 0, ETBR_FORMATTERCTRL);
	etb_writel(t, 1, ETBR_CTRL);

	etb_lock(t);

	/* configure etm */
	v = ETMCTRL_OPTS | ETMCTRL_PROGRAM | ETMCTRL_PORTSIZE(t->etm_portsz);

	if (t->flags & TRACER_CYCLE_ACC)
		v |= ETMCTRL_CYCLEACCURATE;

	etm_unlock(t);

	etm_writel(t, v, ETMR_CTRL);

	while (!(etm_readl(t, ETMR_CTRL) & ETMCTRL_PROGRAM) && --timeout)
		;
	if (!timeout) {
		panic("Waiting for progbit to assert timed out");
		etm_lock(t);
		return;
	}

	etm_setup_address_range(t, 1, (unsigned long)&text_start,
			(unsigned long)&text_end, 0, 0);
	etm_writel(t, 0, ETMR_TRACEENCTRL2);
	etm_writel(t, 0, ETMR_TRACESSCTRL);
	etm_writel(t, 0x6f, ETMR_TRACEENEVT);

	v &= ~ETMCTRL_PROGRAM;
	v |= ETMCTRL_PORTSEL;

	etm_writel(t, v, ETMR_CTRL);

	timeout = TRACER_TIMEOUT;
	while (etm_readl(t, ETMR_CTRL) & ETMCTRL_PROGRAM && --timeout)
		;
	if (!timeout) {
		panic("Waiting for progbit to assert timed out");
		etm_lock(t);
		return;
	}

	etm_lock(t);

	t->flags |= TRACER_RUNNING;
}

void trace_stop(void)
{
	struct tracectx *t = &tracer;
	unsigned long timeout = TRACER_TIMEOUT;

	etm_unlock(t);

	etm_writel(t, 0x440, ETMR_CTRL);
	while (!(etm_readl(t, ETMR_CTRL) & ETMCTRL_PROGRAM) && --timeout)
		;
	if (!timeout) {
		printk("Waiting for progbit to assert timed out\n");
		etm_lock(t);
		return;
	}

	etm_lock(t);

	etb_unlock(t);
	etb_writel(t, ETBFF_MANUAL_FLUSH, ETBR_FORMATTERCTRL);

	timeout = TRACER_TIMEOUT;
	while (etb_readl(t, ETBR_FORMATTERCTRL) &
			ETBFF_MANUAL_FLUSH && --timeout)
		;
	if (!timeout) {
		printk("Waiting for formatter flush to commence "
				"timed out\n");
		etb_lock(t);
		return;
	}

	etb_writel(t, 0, ETBR_CTRL);

	etb_lock(t);

	t->flags &= ~TRACER_RUNNING;
}

static int etb_getdatalen(struct tracectx *t)
{
	uint32_t v;
	int rp, wp;

	v = etb_readl(t, ETBR_STATUS);

	if (v & 1)
		return t->etb_bufsz;

	rp = etb_readl(t, ETBR_READADDR);
	wp = etb_readl(t, ETBR_WRITEADDR);

	if (rp > wp) {
		etb_writel(t, 0, ETBR_READADDR);
		etb_writel(t, 0, ETBR_WRITEADDR);

		return 0;
	}

	return wp - rp;
}

void trace_dump(void)
{
	struct tracectx *t = &tracer;
	uint32_t first = 0;
	uint32_t length;

	if (trace_isrunning(t))
		trace_stop();

	etb_unlock(t);

	length = etb_getdatalen(t);

	if (length == t->etb_bufsz)
		first = etb_readl(t, ETBR_WRITEADDR);

	etb_writel(t, first, ETBR_READADDR);

	printk("Trace buffer contents length: %d\n", length);
	printk("--- ETB buffer begin ---\n");
	for (; length; length--)
		printk("%08x", (uint32_t)__builtin_bswap32(etb_readl(t, ETBR_READMEM)));
	printk("\n--- ETB buffer end ---\n");

	/* deassert the overflow bit */
	etb_writel(t, 1, ETBR_CTRL);
	etb_writel(t, 0, ETBR_CTRL);

	etb_writel(t, 0, ETBR_TRIGGERCOUNT);
	etb_writel(t, 0, ETBR_READADDR);
	etb_writel(t, 0, ETBR_WRITEADDR);

	etb_lock(t);
}


static void etb_init(void)
{
	struct tracectx *t = &tracer;

	t->etb_regs = 0x5401b000;

	etb_unlock(t);
	t->etb_bufsz = etb_readl(t, ETBR_DEPTH);
	printk("Size: %x\n", t->etb_bufsz);

	/* make sure trace capture is disabled */
	etb_writel(t, 0, ETBR_CTRL);
	etb_writel(t, 0x1000, ETBR_FORMATTERCTRL);
	etb_lock(t);

}

static void etm_init(void)
{
	struct tracectx *t = &tracer;

	t->etm_regs = 0x54010000;
	t->flags = TRACER_CYCLE_ACC;
	t->etm_portsz = 1;

	etm_unlock(t);
	(void)etm_readl(t, ETMMR_PDSR);
	/* dummy first read */
	(void)etm_readl(&tracer, ETMMR_OSSRR);

	t->ncmppairs = etm_readl(t, ETMR_CONFCODE) & 0xf;
	etm_writel(t, 0x440, ETMR_CTRL);
	etm_lock(t);

	printk("ETM AMBA driver initialized.\n");
}

void trace_init(void) {
	etb_init();
	etm_init();
}
