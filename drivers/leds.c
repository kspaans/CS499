/* This file cobbled together from U-Boot sources. It is specific to the Overo OMAP 35xx boards. */
#include <machine.h>
#include <drivers/leds.h>
#include <drivers/eth.h> // for ethernet defines
#include <drivers/gpio.h>
#include <types.h>
#include <kern/printk.h>

/*
 * Generic virtual read/write.  Note that we don't support half-word
 * read/writes.  We define __arch_*[bl] here, and leave __arch_*w
 * to the architecture specific code.
 */
#define __arch_getb(a)			(*(volatile unsigned char *)(a))
#define __arch_getw(a)			(*(volatile unsigned short *)(a))
#define __arch_getl(a)			(*(volatile unsigned int *)(a))

#define __arch_putb(v,a)		(*(volatile unsigned char *)(a) = (v))
#define __arch_putw(v,a)		(*(volatile unsigned short *)(a) = (v))
#define __arch_putl(v,a)		(*(volatile unsigned int *)(a) = (v))

/*
 * TODO: The kernel offers some more advanced versions of barriers, it might
 * have some advantages to use them instead of the simple one here.
 */
#define dmb()		__asm__ __volatile__ ("" : : : "memory")
#define __iormb()	dmb()
#define __iowmb()	dmb()

#define writeb(v,c)	({ uint8_t  __v = v; __iowmb(); __arch_putb(__v,c); __v; })
#define writew(v,c)	({ uint16_t __v = v; __iowmb(); __arch_putw(__v,c); __v; })
#define writel(v,c)	({ uint32_t __v = v; __iowmb(); __arch_putl(__v,c); __v; })

#define readb(c)	({ uint8_t  __v = __arch_getb(c); __iormb(); __v; })
#define readw(c)	({ uint16_t __v = __arch_getw(c); __iormb(); __v; })
#define readl(c)	({ uint32_t __v = __arch_getl(c); __iormb(); __v; })


/* I2C masks */

/* I2C Status Register (I2C_STAT): */

#define I2C_STAT_SBD	(1 << 15) /* Single byte data */
#define I2C_STAT_BB	(1 << 12) /* Bus busy */
#define I2C_STAT_ROVR	(1 << 11) /* Receive overrun */
#define I2C_STAT_XUDF	(1 << 10) /* Transmit underflow */
#define I2C_STAT_AAS	(1 << 9)  /* Address as slave */
#define I2C_STAT_GC	(1 << 5)
#define I2C_STAT_XRDY	(1 << 4)  /* Transmit data ready */
#define I2C_STAT_RRDY	(1 << 3)  /* Receive data ready */
#define I2C_STAT_ARDY	(1 << 2)  /* Register access ready */
#define I2C_STAT_NACK	(1 << 1)  /* No acknowledgment interrupt enable */
#define I2C_STAT_AL	(1 << 0)  /* Arbitration lost interrupt enable */

/* I2C Configuration Register (I2C_CON): */

#define I2C_CON_EN	(1 << 15)  /* I2C module enable */
#define I2C_CON_BE	(1 << 14)  /* Big endian mode */
#define I2C_CON_STB	(1 << 11)  /* Start byte mode (master mode only) */
#define I2C_CON_MST	(1 << 10)  /* Master/slave mode */
#define I2C_CON_TRX	(1 << 9)   /* Transmitter/receiver mode */
				   /* (master mode only) */
#define I2C_CON_XA	(1 << 8)   /* Expand address */
#define I2C_CON_STP	(1 << 1)   /* Stop condition (master mode only) */
#define I2C_CON_STT	(1 << 0)   /* Start condition (master mode only) */

/* I2C base */
#define OMAP34XX_CORE_L4_IO_BASE	0x48000000
#define I2C_BASE1		(OMAP34XX_CORE_L4_IO_BASE + 0x70000)
#define I2C_BASE2		(OMAP34XX_CORE_L4_IO_BASE + 0x72000)
#define I2C_BASE3		(OMAP34XX_CORE_L4_IO_BASE + 0x60000)

#define I2C_BUS_MAX	3
#define I2C_DEFAULT_BASE	I2C_BASE1

#define I2C_TIMEOUT  1000

static void flush_fifo(void);
static void wait_for_bb (void);
static uint16_t wait_for_pin (void);

static struct i2c {
	unsigned short rev;	/* 0x00 */
	unsigned short res1;
	unsigned short ie;	/* 0x04 */
	unsigned short res2;
	unsigned short stat;	/* 0x08 */
	unsigned short res3;
	unsigned short iv;	/* 0x0C */
	unsigned short res4;
	unsigned short syss;	/* 0x10 */
	unsigned short res4a;
	unsigned short buf;	/* 0x14 */
	unsigned short res5;
	unsigned short cnt;	/* 0x18 */
	unsigned short res6;
	unsigned short data;	/* 0x1C */
	unsigned short res7;
	unsigned short sysc;	/* 0x20 */
	unsigned short res8;
	unsigned short con;	/* 0x24 */
	unsigned short res9;
	unsigned short oa;	/* 0x28 */
	unsigned short res10;
	unsigned short sa;	/* 0x2C */
	unsigned short res11;
	unsigned short psc;	/* 0x30 */
	unsigned short res12;
	unsigned short scll;	/* 0x34 */
	unsigned short res13;
	unsigned short sclh;	/* 0x38 */
	unsigned short res14;
	unsigned short systest;	/* 0x3c */
	unsigned short res15;
} *i2c_base = (struct i2c *)I2C_DEFAULT_BASE;

static void udelay(unsigned long us) {
	volatile int i = us*3;
	while(i-->0)
		;
}

static int i2c_write_byte (uint8_t devaddr, uint8_t regoffset, uint8_t value)
{
	int i2c_error = 0;
	uint16_t status;

	/* wait until bus not busy */
	wait_for_bb ();

	/* two bytes */
	writew (2, &i2c_base->cnt);
	/* set slave address */
	writew (devaddr, &i2c_base->sa);
	/* stop bit needed here */
	writew (I2C_CON_EN | I2C_CON_MST | I2C_CON_STT | I2C_CON_TRX |
		I2C_CON_STP, &i2c_base->con);

	while (1) {
		status = wait_for_pin();
		if (status == 0 || status & I2C_STAT_NACK) {
			i2c_error = 1;
			goto write_exit;
		}
		if (status & I2C_STAT_XRDY) {
			/* send register offset */
			writeb(regoffset, &i2c_base->data);
			writew(I2C_STAT_XRDY, &i2c_base->stat);

			while (1) {
				status = wait_for_pin();
				if (status == 0 || status & I2C_STAT_NACK) {
					i2c_error = 1;
					goto write_exit;
				}
				if (status & I2C_STAT_XRDY) {
					/* send data */
					writeb(value, &i2c_base->data);
					writew(I2C_STAT_XRDY, &i2c_base->stat);
				}
				if (status & I2C_STAT_ARDY) {
					writew(I2C_STAT_ARDY, &i2c_base->stat);
					break;
				}
			}
			break;
		}
		if (status & I2C_STAT_ARDY) {
			writew(I2C_STAT_ARDY, &i2c_base->stat);
			break;
		}
	}

	wait_for_bb();

	status = readw(&i2c_base->stat);
	if (status & I2C_STAT_NACK)
		i2c_error = 1;

write_exit:
	flush_fifo();
	writew (0xFFFF, &i2c_base->stat);
	writew (0, &i2c_base->cnt);
	return i2c_error;
}

static void flush_fifo(void)
{	uint16_t stat;

	/* note: if you try and read data when its not there or ready
	 * you get a bus error
	 */
	while(1){
		stat = readw(&i2c_base->stat);
		if(stat == I2C_STAT_RRDY){
			readb(&i2c_base->data);
			writew(I2C_STAT_RRDY,&i2c_base->stat);
			udelay(1000);
		}else
			break;
	}
}

static void wait_for_bb (void)
{
	int timeout = I2C_TIMEOUT;
	uint16_t stat;

	writew(0xFFFF, &i2c_base->stat);	 /* clear current interruts...*/
	while ((stat = readw (&i2c_base->stat) & I2C_STAT_BB) && timeout--) {
		writew (stat, &i2c_base->stat);
		udelay(1000);
	}

	if (timeout <= 0) {
		printk ("timed out in wait_for_bb: I2C_STAT=%x\n",
			readw (&i2c_base->stat));
	}
	writew(0xFFFF, &i2c_base->stat);	 /* clear delayed stuff*/
}

static uint16_t wait_for_pin (void)
{
	uint16_t status;
	int timeout = I2C_TIMEOUT;

	do {
		udelay (1000);
		status = readw (&i2c_base->stat);
	} while (  !(status &
		   (I2C_STAT_ROVR | I2C_STAT_XUDF | I2C_STAT_XRDY |
		    I2C_STAT_RRDY | I2C_STAT_ARDY | I2C_STAT_NACK |
		    I2C_STAT_AL)) && timeout--);

	if (timeout <= 0) {
		printk ("timed out in wait_for_pin: I2C_STAT=%x\n",
			readw (&i2c_base->stat));
		writew(0xFFFF, &i2c_base->stat);
		status = 0;
	}

	return status;
}

#define TWL4030_CHIP_LED				0x4a
#define TWL4030_LED_LEDEN				0xEE
#define TWL4030_LED_LEDEN_LEDAON			(1 << 0)
#define TWL4030_LED_LEDEN_LEDBON			(1 << 1)
#define TWL4030_LED_LEDEN_LEDAPWM			(1 << 4)
#define TWL4030_LED_LEDEN_LEDBPWM			(1 << 5)

static void twl4030_led_init(unsigned char ledon_mask) {
	/* LEDs need to have corresponding PWMs enabled */
	if (ledon_mask & TWL4030_LED_LEDEN_LEDAON)
		ledon_mask |= TWL4030_LED_LEDEN_LEDAPWM;
	if (ledon_mask & TWL4030_LED_LEDEN_LEDBON)
		ledon_mask |= TWL4030_LED_LEDEN_LEDBPWM;

	if(i2c_write_byte(TWL4030_CHIP_LED, TWL4030_LED_LEDEN, ledon_mask)) {
		printk("i2c_write failed?\n");
	}
}

static void eth0_led_set(int pin, int status) {
	volatile int *flags = (int *)(ETH1_BASE + ETH_GPIO_CFG_OFFSET);
	int cur = *flags;
	cur &= ~(0x10000000 << pin); // switch LED control to GPIO
	cur |= (0x100 << pin); // set GPIO output direction
	if(status)
		cur |= (0x1 << pin);
	else
		cur &= ~(0x1 << pin);
	*flags = cur;
}

void led_set(enum leds led, int status) {
	switch(led) {
		case LED1:
			gpio_set(GPIO_LED1, status);
			break;
		case LED2:
			gpio_set(GPIO_LED2, status);
			break;
		case LED3:
			eth0_led_set(0, status);
			break;
		case LED4:
			eth0_led_set(1, status);
			break;
		case LED5:
			twl4030_led_init((TWL4030_LED_LEDEN_LEDAON | TWL4030_LED_LEDEN_LEDBON) * status);
			break;
	}
}
