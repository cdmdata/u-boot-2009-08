#include "mx5x_common.h"
#include "mx5x.h"
#include "mx51.h"

#define FAST_INDEX (1 << 10)		//test 4K of memory
#define AGGRESSIVE_INDEX (1 << 18)
/*
  Perform calibration for write and read delay parameters.
  Read: DQS to DQ timing is relative to the i.MX51 internally.
  Write: DQS to DQ timing is relative to the DDR memory device.
  Find center of valid sample window to clock DQS which meets setup and hold time requirements.
   5 DQS lines, 4 for read, 1 for write
ESDCDLY1 - 0x83fd9020 - DQS0 for read data bits[7:0]
ESDCDLY2 - 0x83fd9024 - DQS1 for read data bits[15:8]
ESDCDLY3 - 0x83fd9028 - DQS2 for read data bits[23:16]
ESDCDLY4 - 0x83fd902c - DQS3 for read data bits[31:24]
ESDCDLY5 - 0x83fd9030 - DQS[3:0] for write data bits[31:0]
ESDGPR[7:0] = read only Total_delay
Total_delay = ((measure x DLY_ABS_OFFSET) / 512) + DLY_OFFSET
measure = (Total_delay -  DLY_OFFSET) * 512 / DLY_ABS_OFFSET
 = (24 - -12) * 512 / 128 = 36 * 4  = 144
measure is hardware calculated # of delay units in one DDR clock period,
DLY_ABS_OFFSET : default value of 128 is 1/4 cycle
 DLY_OFFSET: default value of -12 to compensate for the constant delay of 12 units
 */


#define cache_flush(void)	\
{	\
	asm volatile (	\
		"stmfd sp!, {r0-r5, r7, r9-r11};"	\
		"mrc        p15, 1, r0, c0, c0, 1;" /*@ read clidr*/	\
		/* @ extract loc from clidr */       \
		"ands       r3, r0, #0x7000000;"	\
		/* @ left align loc bit field*/      \
		"mov        r3, r3, lsr #23;"	\
		/* @ if loc is 0, then no need to clean*/    \
		"beq        555f;" /* finished;" */	\
		/* @ start clean at cache level 0*/  \
		"mov        r10, #0;"	\
		"111:" /*"loop1: */	\
		/* @ work out 3x current cache level */	\
		"add        r2, r10, r10, lsr #1;"	\
		/* @ extract cache type bits from clidr */    \
		"mov        r1, r0, lsr r2;"	\
		/* @ mask of the bits for current cache only */	\
		"and        r1, r1, #7;"	\
		/* @ see what cache we have at this level*/  \
		"cmp        r1, #2;"	\
		/* @ skip if no cache, or just i-cache*/	\
		"blt        444f;" /* skip;" */	\
		/* @ select current cache level in cssr*/   \
		"mcr        p15, 2, r10, c0, c0, 0;"	\
		/* @ isb to sych the new cssr&csidr */	\
		"mcr        p15, 0, r10, c7, c5, 4;"	\
		/* @ read the new csidr */    \
		"mrc        p15, 1, r1, c0, c0, 0;"	\
		/* @ extract the length of the cache lines */ \
		"and        r2, r1, #7;"	\
		/* @ add 4 (line length offset) */   \
		"add        r2, r2, #4;"	\
		"ldr        r4, =0x3ff;"	\
		/* @ find maximum number on the way size*/   \
		"ands       r4, r4, r1, lsr #3;"	\
		/*"clz  r5, r4;" @ find bit position of way size increment*/ \
		".word 0xE16F5F14;"	\
		"ldr        r7, =0x7fff;"	\
		/* @ extract max number of the index size*/  \
		"ands       r7, r7, r1, lsr #13;"	\
		"222:" /* loop2:"  */	\
		/* @ create working copy of max way size*/   \
		"mov        r9, r4;"	\
		"333:" /* loop3:"  */	\
		/* @ factor way and cache number into r11*/  \
		"orr        r11, r10, r9, lsl r5;"	\
		/* @ factor index number into r11*/  \
		"orr        r11, r11, r7, lsl r2;"	\
		/* @ clean & invalidate by set/way */	\
		"mcr        p15, 0, r11, c7, c14, 2;"	\
		/* @ decrement the way */	\
		"subs       r9, r9, #1;"	\
		"bge        333b;" /* loop3;" */	\
		/* @ decrement the index */	\
		"subs       r7, r7, #1;"	\
		"bge        222b;" /* loop2;" */	\
		"444:" /* skip: */	\
		/*@ increment cache number */	\
		"add        r10, r10, #2;"	\
		"cmp        r3, r10;"	\
		"bgt        111b;" /* loop1; */	\
		"555:" /* "finished:" */	\
		/* @ swith back to cache level 0 */	\
		"mov        r10, #0;"	\
		/* @ select current cache level in cssr */	\
		"mcr        p15, 2, r10, c0, c0, 0;"	\
		/* @ isb to sych the new cssr&csidr */	\
		"mcr        p15, 0, r10, c7, c5, 4;"	\
		"ldmfd	    sp!, {r0-r5, r7, r9-r11};"	\
		"666:" /* iflush:" */	\
		"mov        r0, #0x0;"	\
		/* @ invalidate I+BTB */	\
		"mcr        p15, 0, r0, c7, c5, 0;"	\
		/* @ drain WB */	\
		"mcr        p15, 0, r0, c7, c10, 4;"	\
		:	\
		:	\
		: "r0" /* Clobber list */	\
	);	\
}

void enter_config_mode(unsigned esd_base)
{
	unsigned val;
	IO_WRITE(esd_base, ESD_SCR, 0x8000);
	for (;;) {
		val = IO_READ(esd_base, ESD_SCR) & 0xc000;
		if (val == 0xc000)
			break;
	}

}

void exit_config_mode(unsigned esd_base)
{
	IO_WRITE(esd_base, ESD_SCR, 0);
}

#define MISC_RST	(1 << 1)
void esd_reset(unsigned esd_base)
{
	IO_MOD(esd_base, ESD_MISC, 0, MISC_RST);	/* reset ddr fifo pointers */
//	delayMicro(2);
	IO_MOD(esd_base, ESD_MISC, MISC_RST, 0);	/* release reset */
//	delayMicro(2);
	IO_WRITE(esd_base, ESD_SCR, 0x04008008);	//PRECHARGE ALL
//	delayMicro(2);
}

#define DELAY_FIELDS(off, abs) ((off & 0xff) << 16) | ((abs << 8))

unsigned update_and_wait_for_measure(unsigned esd_base, unsigned reg_index, unsigned dly)
{
	unsigned td;
	unsigned cnt;
	enter_config_mode(esd_base);
	IO_WRITE(esd_base, ESD_DLY1 + (reg_index << 2), DELAY_FIELDS(-12, dly));
	IO_READ(esd_base, ESD_GPR);
	for (cnt = 0; cnt < 100; cnt ++) {
		delayMicro(1);
		td = IO_READ(esd_base, ESD_GPR) & 0xff;
		if (td)
			break;
	}
	esd_reset(esd_base);
	exit_config_mode(esd_base);
	return td;
}

/*
 * Let y1 = total delay (ESDGPR[7:0]) when x1 = DLY_ABS_OFFSET
 * Then
 *  y1 = ((measure * x1) / 512) - 12
 *  y2 = ((measure * x2) / 512) - 12
 *
 * or
 * measure = ((y1 + 12) * 512)/x1
 * x2 =  ((y2 + 12) * 512)/measure
 * x2 = ((y2 + 12) * 512)/(((y1 + 12) * 512)/x1)
 * x2 = x1 * ((y2 + 12) * 512) / ((y1 + 12) * 512)
 * x2 = x1 * (y2 + 12) / (y1 + 12)
 *
 * Now to find zero point, set y2 = 0;
 * p0 = 12(x1) / (y1 + 12)
 * Or Find one point set y2 = 1;
 * p1 = 13(x1) / (y1 + 12)
 */
unsigned find_zero_point(unsigned esd_base)
{
	unsigned x1 = 255;
	unsigned prev = 0;
//	my_printf("esd_dly5(%x)=%x  esd_gpr(%x)=%x\n", esd_base+ESD_DLY5, IO_READ(esd_base, ESD_DLY5), esd_base+ESD_GPR, IO_READ(esd_base, ESD_GPR));
	while (x1) {
		unsigned p0, p1;
		unsigned y1 = update_and_wait_for_measure(esd_base, 4, x1);
		p0 = (12 * x1 + y1 + 11) / (y1 + 12);	//round up
		p1 = (13 * x1 + y1 + 11) / (y1 + 12);	//round up
		my_printf("x1=%x y1=%x  p0=%x p1=%x\n", x1, y1, p0, p1);
		if ((!y1) && (prev == (x1 + 1)))
			break;
		prev = x1;
		if (p1 > 255)
			p1 = 255;
		x1 = ((y1 > 3) && (p1 != x1))? p1 :  (y1) ? (x1 - 1) : (x1 + 1);
	}
	my_printf("zero_point = %x\n", x1);
	return x1;
}

struct check_params;
typedef unsigned (*check_rtn)(struct check_params *p);

struct check_params
{
	unsigned esd_base;
	unsigned esdc_dly_index;
	unsigned mask;
	unsigned *ram_start;
	unsigned *ram_end;
	unsigned start;
	unsigned max;
	unsigned inc;
	unsigned best_start;
	unsigned find_any;
	check_rtn ck_rtn;
	struct check_params *ck_parms;
};

#if 0
void write_memory_pattern(unsigned *p, unsigned *p_end)
{
//	my_printf("a");
//	my_printf("write_memory_pattern from %x to %x\n", p, p_end);
	while (p < p_end) {
		p[0] = ((unsigned)p);
		p[1] = ~((unsigned)p);
		p[2] = ((unsigned)p) ^ 0xaaaaaaaa;
		p[3] = ((unsigned)p) ^ 0x55555555;
		p += 4;
	}
}
#else
void write_memory_pattern(unsigned *p, unsigned *p_end)
{
	asm volatile (
	"	mov	r0, %0;"
	"	ldr	r9, =0x55555555;"
	"	b	2f;"
	"1:	mvn	r2,r0;"
	"	eor	r3,r2,r9;"
	"	eor	r4,r0,r9;"
	"	add	r5,r0,#16;"
	"	mvn	r6,r5;"
	"	eor	r7,r6,r9;"
	"	eor	r8,r5,r9;"
	"	stm	r0!,{r0, r2-r8};"
	"2:	cmp	r0,%1;"
	"	blo	1b;"
		: "+r" (p), "+r" (p_end)
		:
		: "r0", "r2", "r3", "r4", "r5", "r6", "r7", "r8", "r9" /* Clobber list */
	);
}
#endif

#if 0
unsigned check_memory_pattern(unsigned *p, unsigned *p_end, unsigned mask)
{
	unsigned sum = 0;
	unsigned expected;
	unsigned val[4];
//	my_printf("b");
//	my_printf("cache_flush\n");
//	delayMicro(2);
	cache_flush();
//	delayMicro(2);
//	my_printf("c");
//	my_printf("check_memory_pattern from %x to %x\n", p, p_end);
	for (;;) {
		if (p >= p_end)
			break;
		val[0] = p[0];
		val[1] = p[1];
		val[2] = p[2];
		val[3] = p[3];
		expected = (unsigned)p;
		sum |= (val[0] ^ expected);
		expected = ~((unsigned)p);
		sum |= (val[1] ^ expected);
		expected = ((unsigned)p) ^ 0xaaaaaaaa;
		sum |= (val[2] ^ expected);
		expected = ((unsigned)p) ^ 0x55555555;
		sum |= (val[3] ^ expected);
		if (sum & mask)
			break;
		if (sum && (sum & 0x00ff) && (sum & 0xff00) && (sum & 0x00ff0000) && (sum & 0xff000000))
			break;
		p += 4;
	}
//	my_printf("d");
//	my_printf("check_memory_pattern returns %x\n", sum);
	return sum;
}
#else
unsigned check_memory_pattern(unsigned *p, unsigned *p_end, unsigned mask)
{
	unsigned sum = 0;
	cache_flush();
	asm volatile (
	"	ldr	r9, =0x55555555;"
	"	b	3f;"
	"1:	ldm	%0,{r5-r8};"
	"	mvn	r2,%0;"
	"	eor	r3,r2,r9;"
	"	eor	r4,%0,r9;"

	"	eor	r5,%0,r5;"
	"	eor	r6,r2,r6;"
	"	orr	%2,%2,r5;"
	"	eor	r7,r3,r7;"
	"	orr	%2,%2,r6;"
	"	eor	r8,r4,r8;"
	"	orr	%2,%2,r7;"
	"	orrs	%2,%2,r8;"
	"	beq	2f;"
	"	tst	%2, %3;"
	"	bne	4f;"
	"	tst	%2,#0x000000ff;"
	"	tstne	%2,#0x0000ff00;"
	"	tstne	%2,#0x00ff0000;"
	"	tstne	%2,#0xff000000;"
	"	bne	4f;"
	"2:	add	%0, %0, #16;"
	"3:	cmp	%0,%1;"
	"	blo	1b;"
	"4:		;"
		: "+r" (p), "+r" (p_end), "+r" (sum), "+r" (mask)
		:
		: "r2", "r3", "r4", "r5", "r6", "r7", "r8", "r9" /* Clobber list */
	);
	return sum;
}
#endif

/*
Check function should
  a. Perform write bursts, single writes
  b. Read bursts, single reads
  c. 8/16/32 bit accesses
  d. Use wide address range

Out: 0 - failure, 1 - success
 */
unsigned check_function(struct check_params *p)
{
	unsigned sum;
	write_memory_pattern(p->ram_start, p->ram_end);
	sum = check_memory_pattern(p->ram_start, p->ram_end, p->mask);
	return (sum & p->mask)? 0 : 1;
}

unsigned check_function_read(struct check_params *p)
{
	unsigned sum;
	sum = check_memory_pattern(p->ram_start, p->ram_end, p->mask);
	return (sum & p->mask)? 0 : 1;
}


unsigned scan_delay(struct check_params *p)
{
	unsigned best_start = 0;
	unsigned best_len = 0;
	unsigned start = 0;
	unsigned len = 0;
	unsigned dly = p->start;
	unsigned ret;
	for (;;) {
		if (dly < p->max) {
			update_and_wait_for_measure(p->esd_base, p->esdc_dly_index, dly);
			ret = p->ck_rtn(p->ck_parms);
		} else
			ret = 0;
		if (ret) {
			if (!len) {
				if (!p->find_any)
					my_printf("index(%x) found dly=%x -", p->esdc_dly_index, dly);
				start = dly;
			}
			len += p->inc;
			if (best_len < len) {
				best_len = len;
				best_start = start;
				if (p->find_any)
					break;
			}
		} else {
			if (len) {
				if (dly >= p->max)
					dly = p->max;
				if (!p->find_any)
					my_printf("%x\n", dly - 1);
			}
			len = 0;
			if ((dly + best_len) >= p->max)
				break;	/* bigger group not possible */
		}
		dly += p->inc;
	}
	p->best_start = best_start;
	if (best_len > (p->max - best_start))
		best_len = p->max - best_start;
	return best_len;
}

void set_range(struct check_params *p, unsigned len, unsigned zero_point)
{
	p->start = p->best_start;
	p->max = p->best_start + len;
	p->inc--;
	if (p->start >= p->inc)
		p->start -= p->inc;
	if (p->start < zero_point)
		p->start = zero_point;
	if (p->max > 256)
		p->max = 256;
	p->inc = 1;
}

int calibrate_write_delay(unsigned esd_base, unsigned *ram_base)
{
	unsigned byte_no;
	unsigned len;
	unsigned val;
	int ret = -1;
	unsigned zero_point;
	struct check_params rd;
	struct check_params wd;
	/* Set ESDMISC[11] to 1. FRC_MSR, force remeasurement */
	IO_MOD(esd_base, ESD_MISC, 0, (1<<11));
	zero_point = find_zero_point(esd_base);
	rd.esd_base = esd_base;
	rd.ram_start = ram_base;
	rd.ram_end = &ram_base[FAST_INDEX];
	rd.start = zero_point;
	rd.max = 256;
	rd.inc = 5;
	rd.best_start = 0;
	rd.find_any = 0;
	rd.ck_rtn = scan_delay;
	rd.ck_parms = &wd;

	wd.esd_base = esd_base;
	wd.ram_start = ram_base;
	wd.ram_end = &ram_base[FAST_INDEX];	//test 4K of memory
	wd.start = zero_point;
	wd.max = 256;
	wd.inc = 5;
	wd.best_start = 0;
	wd.find_any = 1;
	wd.ck_rtn = check_function;
	wd.ck_parms = &wd;
	wd.esdc_dly_index = 4;

	for (byte_no = 0; byte_no < 4; byte_no++) {
		rd.esdc_dly_index = byte_no;
		wd.mask = rd.mask = 0xff << (byte_no << 3);
		len = scan_delay(&rd);
		if (!len) {
			my_printf("error byte %x len=0\n", byte_no);
			goto exit;
		}
		val = rd.best_start + (len >> 1);
		my_printf("%x = %x, best_start=%x, len=%x\n", byte_no, val, rd.best_start, len);
		update_and_wait_for_measure(esd_base, byte_no, val);
	}
	wd.mask = 0xffffffff;
	wd.find_any = 0;
	len = scan_delay(&wd);
	if (!len) {
		my_printf("write len=0\n");
		goto exit;
	}
	rd.ram_end = wd.ram_end = &ram_base[AGGRESSIVE_INDEX];	//test 4M of memory
	set_range(&wd, len, zero_point);
	len = scan_delay(&wd);
	if (!len) {
		my_printf("aggressive write len=0\n");
		goto exit;
	}
	val = wd.best_start + (len >> 1);
	my_printf("write_delay=%x, best_start=%x, len=%x\n", val, wd.best_start, len);
	update_and_wait_for_measure(esd_base, 4, val);

	write_memory_pattern(rd.ram_start, rd.ram_end);
	rd.ck_rtn = check_function_read;
	rd.ck_parms = &rd;
	for (byte_no = 0; byte_no < 4; byte_no++) {
		rd.start = zero_point;
		rd.max = 256;
		rd.inc = 5;
		rd.esdc_dly_index = byte_no;
		rd.mask = 0xff << (byte_no << 3);
		rd.ram_end = &ram_base[FAST_INDEX];	//test 4k of memory
		len = scan_delay(&rd);
		if (!len) {
			my_printf("error byte %x len=0\n", byte_no);
			goto exit;
		}
		rd.ram_end = &ram_base[AGGRESSIVE_INDEX];	//test 4M of memory
		set_range(&rd, len, zero_point);
		len = scan_delay(&rd);
		if (!len) {
			my_printf("aggressive read error byte %x len=0\n", byte_no);
			goto exit;
		}
		val = rd.best_start + (len >> 1);
		my_printf("%x = %x, best_start=%x, len=%x\n", byte_no, val, rd.best_start, len);
		update_and_wait_for_measure(esd_base, byte_no, val);
	}
	ret = 0;
exit:
	/* Set ESDMISC[11] to 0. FRC_MSR, turn off force remeasurement */
	IO_MOD(esd_base, ESD_MISC, (1<<11), 0);
	return ret;
}


#define MR 0
#define EMR 1
#define EMR2 2
#define EMR3 3
#define MAKE_CMD(reg, addr, cs) (((reg) << 0) | ((addr) << 16) | (3 << 3) | ((cs) << 2) | 0x8000)

unsigned write_gpr(unsigned esd_base, unsigned *pdelay)
{
	int i;
	unsigned min = 0xf;
	unsigned max = 0;
	unsigned gpr = 0x80000000;
	for (i = 0; i < 4; i++) {
		if (min > pdelay[i])
			min = pdelay[i];
		if (max < pdelay[i])
			max = pdelay[i];
	}
	if ((max - min) > 3) {
		my_printf("range to big: %x to %x\n", min, max);
		min = max - 3;
		if (min > 0xf)
			min = 0xf;
	}
	for (i = 0; i < 4; i++) {
		int diff = pdelay[i] - min;
		if (diff < 0)
			diff = 0;
		if (diff > 3) {
			diff = 3;
		}
		gpr |= diff << (25 - (i << 1));
	}
	gpr |= (min << 27);
	enter_config_mode(esd_base);
	IO_WRITE(esd_base, ESD_GPR, gpr);
	my_printf("test enable delays: %x %x %x %x gpr=%x %x\n",
			pdelay[0], pdelay[1], pdelay[2], pdelay[3], gpr, IO_READ(esd_base, ESD_GPR));
	esd_reset(esd_base);
	exit_config_mode(esd_base);
	return gpr;
}

int calibrate_dqs_enable(unsigned esd_base, unsigned *ram_base, unsigned * pgpr)
{
	int i;
	unsigned gpr;
	unsigned mask;
	unsigned delay[4];
	unsigned *ram_end = &ram_base[AGGRESSIVE_INDEX];	//test 4M of memory
	delay[0] = 0;
	delay[1] = 0;
	delay[2] = 0;
	delay[3] = 0;
	write_memory_pattern(ram_base, ram_end);

	for (;;) {
		write_gpr(esd_base, delay);
		mask = check_memory_pattern(ram_base, ram_end, 0);
		if (!mask)
			break;
		for (i = 0; i < 4; i++) {
			if (mask & 0xff) {
				delay[i]++;
				if (delay[i] > 18)
					break;
			}
			mask >>= 8;
		}
	}
	for (i = 0; i < 4; i++) {
		delay[i]++;
	}
	gpr = write_gpr(esd_base, delay);
	if (pgpr)
		*pgpr = gpr;
	mask = check_memory_pattern(ram_base, ram_end, 0);
	if (mask) {
		my_printf("mask error: %x to %x\n", mask);
		return -1;
	}
	my_printf("enable delays: %x %x %x %x gpr=%x\n",
			delay[0], delay[1], delay[2], delay[3], gpr);
	return 0;
}

#define MODE_SINGLE 1
#define MODE_DIFFERENTIAL 0
#define DIFF_DQS_EN (1 << 15)

void iomuxc_ddr_differential(void);
void iomuxc_ddr_single(void);

void switch_to_mode(unsigned esd_base, unsigned mode)
{
	unsigned ctl1;
	unsigned addr =  EMRS1_DRIVE_STRENGTH | EMRS1_ODT_TERM | (mode << EMRS1_DQS_SINGLE_BIT);
	enter_config_mode(esd_base);
	ctl1 = IO_READ(esd_base, ESD_CTL1);
	//DQS# disable (single-ended)/ enable (differential), 50 ohms
	IO_WRITE(esd_base, ESD_SCR, MAKE_CMD(EMR, addr, 0));	//cs0
	if (ctl1 & (1 << 31))
		IO_WRITE(esd_base, ESD_SCR, MAKE_CMD(EMR, addr, 1));	//cs1

	IO_MOD(esd_base, ESD_MISC, DIFF_DQS_EN, (mode == MODE_DIFFERENTIAL) ? DIFF_DQS_EN : 0);

	if (mode == MODE_DIFFERENTIAL) {
		IO_MOD(esd_base, ESD_MISC, 0, (0xff << 20));
		iomuxc_ddr_differential();
	} else {
		iomuxc_ddr_single();
		IO_MOD(esd_base, ESD_MISC, (0xff << 20), 0);
	}
	exit_config_mode(esd_base);
	delayMicro(2);
}

unsigned check_mem(unsigned esd_base, unsigned *ram_base)
{
	unsigned *ram_end = &ram_base[AGGRESSIVE_INDEX];
	unsigned mask;
	write_memory_pattern(ram_base, ram_end);
	mask = check_memory_pattern(ram_base, ram_end, 0);
	my_printf("check mask %x\n", mask);
	my_printf("DLYs: %x %x %x %x %x\n", IO_READ(esd_base, ESD_DLY1), IO_READ(esd_base, ESD_DLY2),
			IO_READ(esd_base, ESD_DLY3), IO_READ(esd_base, ESD_DLY4),
			IO_READ(esd_base, ESD_DLY5));
	return mask;
}

static void up_and_measure(unsigned esd_base, unsigned reg_index, unsigned val)
{
	/* Set ESDMISC[11] to 1. FRC_MSR, force remeasurement */
	IO_MOD(esd_base, ESD_MISC, 0, (1<<11));
	update_and_wait_for_measure(esd_base, reg_index, val);
	/* Set ESDMISC[11] to 0. FRC_MSR, turn off force remeasurement */
	IO_MOD(esd_base, ESD_MISC, (1<<11), 0);
}

int plug_main(void **pstart, unsigned *pbytes, unsigned *pivt_offset)
{
	int ret = 0;
	unsigned *ram_base = (unsigned *)get_ram_base();
	unsigned esd_base = ESD_BASE;
	unsigned mask = 0;
	unsigned gpr = 0;

	if (0) {
		int i;
		check_mem(esd_base, ram_base);
		up_and_measure(esd_base, 4, 0x80);
		check_mem(esd_base, ram_base);
		for (i = 0; i <= 4; i++) {
			up_and_measure(esd_base, i, 0x80);
		}
		/* Set ESDMISC[11] to 1. FRC_MSR, force remeasurement */
		IO_MOD(esd_base, ESD_MISC, 0, (1<<11));
		find_zero_point(esd_base);
		/* Set ESDMISC[11] to 0. FRC_MSR, turn off force remeasurement */
		IO_MOD(esd_base, ESD_MISC, (1<<11), 0);
		check_mem(esd_base, ram_base);
		up_and_measure(esd_base, 4, 0x80);
		check_mem(esd_base, ram_base);
		up_and_measure(esd_base, 4, 0xff);
		check_mem(esd_base, ram_base);
		up_and_measure(esd_base, 4, 0x80);
		mask = check_mem(esd_base, ram_base);
	}

	if (!mask) {
//		switch_to_mode(esd_base, MODE_SINGLE);
//		ret = calibrate_write_delay(esd_base, ram_base);
//		switch_to_mode(esd_base, MODE_DIFFERENTIAL);
		if (!ret) {
			ret = calibrate_dqs_enable(esd_base, ram_base, &gpr);
			if (!ret)
				ret = calibrate_write_delay(esd_base, ram_base);
		} else {
			int i;
			for (i = 0; i <= 4; i++) {
				up_and_measure(esd_base, i, 0x80);
			}
		}
	}
	mask = check_mem(esd_base, ram_base);
	my_printf("Final DLYs: %x %x %x %x %x, gpr=%x mask=%x\n",
			IO_READ(esd_base, ESD_DLY1), IO_READ(esd_base, ESD_DLY2),
			IO_READ(esd_base, ESD_DLY3), IO_READ(esd_base, ESD_DLY4),
			IO_READ(esd_base, ESD_DLY5), gpr, mask);
	my_printf("done\n");
	for (;;) {
	}
	return ret;
}
