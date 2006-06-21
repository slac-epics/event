/* $Id$ */

/* PLX9080 serial EEPROM access (93CS46 device) */
#include <stdio.h>
#include <errno.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include <rtems.h>
#include <bsp/pci.h>
#include <libcpu/io.h>

#define BSP_pciFindDevice pci_find_device

#ifndef PCI_DEVICE_ID_PLX_9030
#define PCI_DEVICE_ID_PLX_9030 0x9030
#endif

#ifdef __PPC__
#include <bsp.h>
#define TBNS (1000000/(BSP_bus_frequency/BSP_time_base_divisor))
#if 0
#define TBNS 60 			/* 1 timebase tick is 60ns (66.6 MHz bus) */
#endif
#endif

/* timing for the 93CS46, 93C66 (in ns) */

#define EE_SETUP_TIME 400	/* exceeds CS setup */
#define EE_CLKHI_TIME 1000
#define EE_CLKLO_TIME 1000
#define EE_CS_LO_TIME 1000

/* EEPROM device parameters (different for the larger CS56) */
#define C46 0
#define C56 1
#define C66 2
typedef struct {
	char *name;
	int abits;
	int size_bits;
} Eeprom93CxxDesc;

static Eeprom93CxxDesc eeDesc[] = {
	{
		"93C46",
		6,
		1024,
	},
	{
		"93C56",
		8,	/* needs the extra clock cycle */
		2048,
	},
	{
		"93C66",
		8,
		4096,
	},
	{0}
};

#define ABITS			(eeDesc[plx9080_ee_type].abits)
#define EE_SIZE_BITS	(eeDesc[plx9080_ee_type].size_bits)

#define EE_WORDS		(EE_SIZE_BITS/D_LEN)	/* size in words */

#define D_LEN			16			/* data bits */
#define CMD_LEN			(ABITS+3)

/* EEPROM operations */
#define EE_CMD(op,addr)	((1<<(ABITS + 2))			| \
			   			 ((op) << ABITS) 			| \
						 ((addr) & ((1<<ABITS)-1)))

/* read (+ read sequential) command */
#define CMD_READ(addr)	EE_CMD(2,addr)
/* write enable */
#define CMD_WEN()		EE_CMD(0, (3<<(ABITS-2)))
/* write */
#define CMD_WRITE(addr)	EE_CMD(1,addr)
/* write all */
#define CMD_WRALL()		EE_CMD(0, (1<<(ABITS-2)))
/* write disable */
#define CMD_WDS()		EE_CMD(0, (0<<(ABITS-2)))


/* EEPROM access through the PLX9080 serial eeprom control register */

#define EE_REG_OFF_9080 0x6c	/* offset */

/* EEPROM access through the PLX9030 serial eeprom control register */
#define EE_REG_OFF_9030	0x50	/* offset */

#define EE_CLK  (1<<24)		/* clock bit   */
#define EE_CS	(1<<25)		/* chip select */
#define EE_WD	(1<<26)		/* write data  */
#define EE_RD	(1<<27)		/* read data   */
#define EE_PRES (1<<28)		/* rom present */


#ifdef __PPC__

#define MFTB(r) do {__asm__ __volatile__("mftb %0":"=r"(r)); } while (0)

/* delay for 'ns' nanoseconds */
static inline void
nsdely(unsigned ns)
{
register unsigned long now,then;
	MFTB(then);
	ns /= TBNS;
	ns++;
	do {
		MFTB(now);
	} while ( (now - then) < ns );
}

#else

#error Need to implement nsdely() for this architecture

#endif


volatile unsigned *plx9080_ee_reg = 0;
int	plx9080_ee_type               = -1;

long plx9080_ee_read(long address, FILE *f);
long plx9080_ee_write(int address, int value);

/* send/receive a bit */
static inline int
wb(int bit, volatile unsigned *rp)
{
unsigned long v;
	/* apply bit */
	v=in_le32(rp);
	if (bit)
		v |= EE_WD;
	else
		v &= ~EE_WD;
	out_le32(rp, v);
	/* observe setup time */
	nsdely(EE_SETUP_TIME);
	/* raise clock */
	out_le32(rp, v | EE_CLK);
	/* min clock hi time (exceeds data hold) */
	nsdely(EE_CLKHI_TIME);
	/* lower data line and clock */
	v &= ~EE_WD;
	out_le32(rp, v);
	nsdely(EE_CLKLO_TIME);
	/* min clock lo time */
	return (in_le32(rp) & EE_RD) != 0;
}

/* set/reset chip select */
static inline void
cs(int on, volatile unsigned *rp)
{
unsigned long v;
	/* apply bit */
	v=in_le32(rp);
	if (on)
		v |= EE_CS;
	else
		v &= ~EE_CS;
	v &= ~EE_CLK;
	out_le32(rp, v);
	nsdely( on ? EE_SETUP_TIME : EE_CS_LO_TIME);
}

/* write a word */
static void
ww(unsigned long w, unsigned len, volatile unsigned *rp)
{
	/* length to mask */
	len = (1<<(len-1));
	do {
		wb(w & len, rp);
		len>>=1;
	} while (len);
}

static long do_write(int address, int *pval, int n)
{
volatile unsigned	*rp = plx9080_ee_reg;

	/* write enable */
	cs(1, rp);

	ww(CMD_WEN(), CMD_LEN, rp);

	cs(0, rp);

	/* write a word of data */
	cs(1, rp);

	while (n--) {

		/* write address */
		ww(CMD_WRITE(address), CMD_LEN, rp);
		/* write data */
		ww(*pval, D_LEN, rp);

		/* lower CS; this starts the programming cycle */
		cs(0,rp);

		cs(1,rp);
		do {
			rtems_task_wake_after(1);
		} while ( ! (in_le32(rp) & EE_RD) );

		pval++;
		address++;
	}

	/* write disable */
	ww(CMD_WDS(), CMD_LEN, rp);

	cs(0, rp);

	return 0;
}

long plx9080_ee_write(int address, int value)
{
	if ( plx9080_ee_type < 0 ) {
		fprintf(stderr,"plx9080_ee_init() not executed\n");
		return -1;
	}

	if ( (address & ~(EE_WORDS-1)) ) {
		fprintf(stderr,"Invalid address: 0x%08x\n",address);
		return -1;
	}

	if ( (value & ~((1<<D_LEN)-1)) ) {
		fprintf(stderr,"Invalid data: 0x%08x\n",value);
		return -1;
	}

	do_write(address, &value, 1);

	return plx9080_ee_read(address, 0);
}

long plx9080_ee_write_from_file(char *name)
{
FILE *fp   = 0;
int	 rval  = -1;
int  *vals = 0;
int  i,ch;

	if ( plx9080_ee_type < 0 ) {
		fprintf(stderr,"plx9080_ee_init() not executed\n");
		return -1;
	}

	if ( ! (fp=fopen(name,"r")) ) {
		fprintf(stderr,"Error opening '%s': %s\n", name, strerror(errno));
		goto cleanup;
	}

	if ( !(vals=malloc( sizeof(*vals) * (EE_WORDS + 1) )) ) {
		fprintf(stderr,"No memory\n");
		goto cleanup;
	}

	for ( i = 0, ch = 0; EOF !=ch; i++ ) {
		if ( 1!= (ch = fscanf(fp,"%i",vals+i)) ) {
			if ( EOF == ch ) 
				break;
			fprintf(stderr,"Unable to read word %i\n", i);
			goto cleanup;
		}
		if (i>=EE_WORDS) {
			fprintf(stderr,"Too many values in file\n");
			goto cleanup;
		}
		/* skip to EOL or EOF */
		do {
			ch = getc(fp);
		} while ( EOF !=ch && '\n' != ch );
	}
	assert( i <= EE_WORDS );
	rval = do_write(0, vals, i);

cleanup:
	if (fp)
		fclose(fp);
	free(vals);
	return rval;
}


long plx9080_ee_read(long address, FILE *f)
{
unsigned 			rval=0xdeadbeef;
int					i;
volatile unsigned	*rp = plx9080_ee_reg;
int					n = address < 0 ? EE_WORDS : 1;

	if ( plx9080_ee_type < 0 ) {
		fprintf(stderr,"plx9080_ee_init() not executed\n");
		return -1;
	}

	if ( !f )
		f = stdout;

	cs(1, rp);

	if (address < 0) {
		address = 0;
	}

	ww(CMD_READ(address), CMD_LEN, rp);
	while ( n-- > 0 ) {
		rval = 0;
		for ( i = 0; i < D_LEN; i++) {
			rval<<=1;
			rval |= wb(0, rp) ? 1 : 0;
		}
		fprintf(f,"0x%04x\n",rval);
	}

	cs(0, rp);

	return rval;
}

static void usage()
{
	fprintf(stderr,"Usage: plx9080_ee_init(int instance, int eeType)\n");
	fprintf(stderr,"       known eeTypes are 46 56 66 (atmel 93Cxx)\n");
}

long plx9080_ee_init(int instance, int eeType)
{
int b,d,f;
unsigned int base;

	switch (eeType) {
		default:
			usage();
			return -1;
		case 46:	plx9080_ee_type = C46; break;
		case 56:	plx9080_ee_type = C56; break;
		case 66:	plx9080_ee_type = C66; break;
	}

	if (0==BSP_pciFindDevice(PCI_VENDOR_ID_PLX,
							 PCI_DEVICE_ID_PLX_9080,
							 instance,
							 &b,&d,&f)) {
		pci_read_config_dword(b,d,f,PCI_BASE_ADDRESS_0,&base);
		printf("PLX 9080 found at 0x%08x",base);
		plx9080_ee_reg = (volatile unsigned *)(base + EE_REG_OFF_9080);
	} else if (0==BSP_pciFindDevice(PCI_VENDOR_ID_PLX,
							 PCI_DEVICE_ID_PLX_9030,
							 instance,
							 &b,&d,&f)) {
		pci_read_config_dword(b,d,f,PCI_BASE_ADDRESS_0,&base);
		printf("PLX 9030 found at 0x%08x",base);
		plx9080_ee_reg = (volatile unsigned *)(base + EE_REG_OFF_9030);
	} else {
		fprintf(stderr,"NO PLX 9080 or 9030 found\n");
		return -1;
	}
	if ( in_le32(plx9080_ee_reg) & EE_PRES ) {
		printf(", EEPROM present\n");
	} else {
		printf(", NO EEPROM present\n");
		return -1;
	}
	return 0;
}

