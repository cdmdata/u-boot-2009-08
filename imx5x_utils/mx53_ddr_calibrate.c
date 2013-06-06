#include "mx5x_common.h"
#include "mx5x.h"
#include "mx53.h"

#define FAST_COUNT (1 << 10)		//test 4K of low memory, 4K high mem
#define AGGRESSIVE_COUNT (1 << 19)	//test 2M of low memory, 2M high mem
#define SLOW_COUNT (32 << 20)	//test 128M of low memory, 128M of high mem

#define MAX_DELAY 1024
#if 0
void write_memory_pattern(unsigned *p, unsigned * p_middle, unsigned *p_end)
{
//	my_printf("a");
//	my_printf("write_memory_pattern from %x to %x\n", p, p_end);
	while (p < p_middle) {
		p[0] = ((unsigned)p);
		p[1] = ~((unsigned)p);
		p[2] = ((unsigned)p) ^ 0xaaaaaaaa;
		p[3] = ((unsigned)p) ^ 0x55555555;
		p += 4;
		p_end -= 4;
		p_end[0] = ((unsigned)p_end);
		p_end[1] = ~((unsigned)p_end);
		p_end[2] = ((unsigned)p_end) ^ 0xaaaaaaaa;
		p_end[3] = ((unsigned)p_end) ^ 0x55555555;
	}
}
#else
void write_memory_pattern(unsigned *p, unsigned * p_middle, unsigned *p_end)
{
	asm volatile (
	"	ldr	r8, =0xaaaaaaaa;"
	"	b	2f;"
	"1:	mov	r0,%0;"
	"	mvn	r1,%0;"
	"	eor	r2,%0,r8;"
	"	eor	r3,r1,r8;"
	"	add	r4,%0,#16;"
	"	mvn	r5,r4;"
	"	eor	r6,r4,r8;"
	"	eor	r7,r5,r8;"
	"	stm	%0!,{r0-r7};"

	"	sub	%2,%2,#32;"
	"	mov	r0,%2;"
	"	mvn	r1,%2;"
	"	eor	r2,%2,r8;"
	"	eor	r3,r1,r8;"
	"	add	r4,%2,#16;"
	"	mvn	r5,r4;"
	"	eor	r6,r4,r8;"
	"	eor	r7,r5,r8;"
	"	stm	%2,{r0-r7};"
	"2:	cmp	%0,%1;"
	"	blo	1b;"
		: "+r" (p), "+r" (p_middle), "+r" (p_end)
		:
		: "r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7", "r8" /* Clobber list */
	);
}
#endif

/*
Check function should
  a. Perform write bursts, single writes
  b. Read bursts, single reads
  c. 8/16/32 bit accesses
  d. Use wide address range

Out: 0 - success, otherwise bitmask of errors
 */
void cache_flush(void);
#if 0
unsigned check_memory_pattern(unsigned *p, unsigned * p_middle, unsigned *p_end, unsigned sum)
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
		if (p >= p_middle)
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

		p_end -= 4;
		val[0] = p_end[0];
		val[1] = p_end[1];
		val[2] = p_end[2];
		val[3] = p_end[3];
		expected = (unsigned)p_end;
		sum |= (val[0] ^ expected);
		expected = ~((unsigned)p_end);
		sum |= (val[1] ^ expected);
		expected = ((unsigned)p_end) ^ 0xaaaaaaaa;
		sum |= (val[2] ^ expected);
		expected = ((unsigned)p_end) ^ 0x55555555;
		sum |= (val[3] ^ expected);
		if (sum == 0xffffffff)
			break;
		p += 4;
	}
//	my_printf("d");
//	my_printf("check_memory_pattern returns %x\n", sum);
	return sum;
}
#else
unsigned check_memory_pattern(unsigned *p, unsigned * p_middle, unsigned *p_end, unsigned sum)
{
	cache_flush();
	asm volatile (
	"	ldr	r8, =0xaaaaaaaa;"
	"	b	3f;"
	"1:	ldm	%0,{r4-r7};"
	"	mvn	r1,%0;"
	"	eor	r2,%0,r8;"
	"	eor	r3,r1,r8;"
	"	eor	r4,%0,r4;"
	"	eor	r5,r1,r5;"
	"	orr	%3,%3,r4;"
	"	eor	r6,r2,r6;"
	"	orr	%3,%3,r5;"
	"	eor	r7,r3,r7;"
	"	orr	%3,%3,r6;"
	"	orr	%3,%3,r7;"

	"	sub	%2, %2, #16;"
	"	ldm	%2,{r4-r7};"
	"	mvn	r1,%2;"
	"	eor	r2,%2,r8;"
	"	eor	r3,r1,r8;"
	"	eor	r4,%2,r4;"
	"	eor	r5,r1,r5;"
	"	orr	%3,%3,r4;"
	"	eor	r6,r2,r6;"
	"	orr	%3,%3,r5;"
	"	eor	r7,r3,r7;"
	"	orr	%3,%3,r6;"
	"	orr	%3,%3,r7;"

	"	mvns	r2,%3;"
	"	beq	4f;"
	"	add	%0, %0, #16;"
	"3:	cmp	%0,%1;"
	"	blo	1b;"
	"4:		;"
		: "+r" (p), "+r" (p_middle), "+r" (p_end), "+r" (sum)
		:
		: "r0", "r1", "r2", "r3", "r4", "r5", "r6", "r7", "r8" /* Clobber list */
	);
	return sum;
}
#endif

void cache_flush(void)
{
	asm volatile (
		"stmfd sp!, {r0-r5, r7, r9-r11};"
		"mrc        p15, 1, r0, c0, c0, 1;" /*@ read clidr*/
		/* @ extract loc from clidr */
		"ands       r3, r0, #0x7000000;"
		/* @ left align loc bit field*/
		"mov        r3, r3, lsr #23;"
		/* @ if loc is 0, then no need to clean*/
		"beq        555f;" /* finished;" */
		/* @ start clean at cache level 0*/
		"mov        r10, #0;"
		"111:" /*"loop1: */
		/* @ work out 3x current cache level */
		"add        r2, r10, r10, lsr #1;"
		/* @ extract cache type bits from clidr */
		"mov        r1, r0, lsr r2;"
		/* @ mask of the bits for current cache only */
		"and        r1, r1, #7;"
		/* @ see what cache we have at this level*/
		"cmp        r1, #2;"
		/* @ skip if no cache, or just i-cache*/
		"blt        444f;" /* skip;" */
		/* @ select current cache level in cssr*/
		"mcr        p15, 2, r10, c0, c0, 0;"
		/* @ isb to sych the new cssr&csidr */
		"mcr        p15, 0, r10, c7, c5, 4;"
		/* @ read the new csidr */
		"mrc        p15, 1, r1, c0, c0, 0;"
		/* @ extract the length of the cache lines */
		"and        r2, r1, #7;"
		/* @ add 4 (line length offset) */
		"add        r2, r2, #4;"
		"ldr        r4, =0x3ff;"
		/* @ find maximum number on the way size*/
		"ands       r4, r4, r1, lsr #3;"
		/*"clz  r5, r4;" @ find bit position of way size increment*/
		".word 0xE16F5F14;"
		"ldr        r7, =0x7fff;"
		/* @ extract max number of the index size*/
		"ands       r7, r7, r1, lsr #13;"
		"222:" /* loop2:"  */
		/* @ create working copy of max way size*/
		"mov        r9, r4;"
		"333:" /* loop3:"  */
		/* @ factor way and cache number into r11*/
		"orr        r11, r10, r9, lsl r5;"
		/* @ factor index number into r11*/
		"orr        r11, r11, r7, lsl r2;"
		/* @ clean & invalidate by set/way */
		"mcr        p15, 0, r11, c7, c14, 2;"
		/* @ decrement the way */
		"subs       r9, r9, #1;"
		"bge        333b;" /* loop3;" */
		/* @ decrement the index */
		"subs       r7, r7, #1;"
		"bge        222b;" /* loop2;" */
		"444:" /* skip: */
		/*@ increment cache number */
		"add        r10, r10, #2;"
		"cmp        r3, r10;"
		"bgt        111b;" /* loop1; */
		"555:" /* "finished:" */
		/* @ swith back to cache level 0 */
		"mov        r10, #0;"
		/* @ select current cache level in cssr */
		"mcr        p15, 2, r10, c0, c0, 0;"
		/* @ isb to sych the new cssr&csidr */
		"mcr        p15, 0, r10, c7, c5, 4;"
		"ldmfd	    sp!, {r0-r5, r7, r9-r11};"
		"666:" /* iflush:" */
		"mov        r0, #0x0;"
		/* @ invalidate I+BTB */
		"mcr        p15, 0, r0, c7, c5, 0;"
		/* @ drain WB */
		"mcr        p15, 0, r0, c7, c10, 4;"
		:
		:
		: "r0" /* Clobber list */
	);
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

void reset_fifo(unsigned esd_base)
{
	int i;
	IO_MOD(esd_base, ESD_DGCTRL0, 0, 1 << 31);	//reset fifo
	for (i = 0; i < 1000; i++) {
		unsigned dgctrl0;
		delayMicro(10);
		dgctrl0 = IO_READ(esd_base, ESD_DGCTRL0);
		if (!(dgctrl0 & (1 << 31)))
			break;
		my_printf("dgctrl0: %x\n", dgctrl0);
	}
}

unsigned force_measure(unsigned esd_base)
{
	unsigned mur;
	int i;
	IO_WRITE(esd_base, ESD_MUR, 0x800);	//start write delay calibration
	for (i = 0; i < 1000; i++) {
		delayMicro(10);
		mur = IO_READ(esd_base, ESD_MUR);
		my_printf("mur: %x\n", mur);
		if (!(mur & 0x800)) {
			break;
		}
	}
	return (mur >> 16);
}

#define MISC_RST	(1 << 1)
void esd_reset(unsigned esd_base)
{
	unsigned misc;
	int i;
	IO_MOD(esd_base, ESD_MISC, 0, MISC_RST);	/* reset ddr fifo pointers */
	for (i = 0; i < 1000; i++) {
		delayMicro(10);
		misc = IO_READ(esd_base, ESD_MISC);
		if (!(misc & MISC_RST)) {
			break;
		}
		my_printf("reset misc: %x\n", misc);
	}
//	delayMicro(2);
//	IO_MOD(esd_base, ESD_MISC, MISC_RST, 0);	/* release reset */
	delayMicro(2);
	IO_WRITE(esd_base, ESD_SCR, 0x04008010);	//PRECHARGE ALL
	delayMicro(2);
}

#if 0
unsigned cycle_to_delay(unsigned cycle, unsigned short *map, unsigned cdelay)
{
	unsigned a = 0;
	unsigned b = MAX_DELAY;
	unsigned c;
	for (;;) {
		c = (a + b) >> 1;
		if (cycle > map[c]) {
			a = c + 1;
			if (a > b) {
				my_printf("cycle=%x map[%x]=%x map[%x]=%x map[%x]=%x\n", cycle, a, map[a], b, map[b], c, map[c]);
				return c;
			}
		} else if (cycle < map[c]) {
			b = c - 1;
			if (a > b) {
				my_printf("cycle=%x map[%x]=%x map[%x]=%x map[%x]=%x\n", cycle, a, map[a], b, map[b], c, map[c]);
				return (c) ? b : c;
			}
		} else {
			return c;
		}
	}
}
#else
unsigned cycle_to_delay(unsigned cycle, unsigned short *map, unsigned cdelay)
{
	return (cdelay * cycle) >> 8;
}
#endif

#if 1
unsigned delay_to_cycle(unsigned delay, unsigned short *map, unsigned cdelay)
{
	return map[delay];
}
#else
unsigned delay_to_cycle(unsigned delay, unsigned short *map, unsigned cdelay)
{
	unsigned cdelay = (unsigned)delay_ctl;
	return (delay << 8) / cdelay;
}
#endif

void store_reg(unsigned esd_base, unsigned *vals, unsigned reg, unsigned short *map, unsigned cdelay)
{
	int i;
	if (reg == ESD_DGCTRL0) {
		unsigned t[2];
		t[0] = 0;
		t[1] = 0;
		for (i = 0; i < 4; i++) {
			unsigned val = delay_to_cycle(vals[i], map, cdelay);
			unsigned field = ((val & 0x7f) | ((val & 0x0780) << 1));
			if (i & 1)
				field <<= 16;
			t[i >> 1] |= field;
		}
		my_printf("DGCTRL0 = %x DGCTRL1 = %x\n", t[0], t[1]);

		enter_config_mode(esd_base);

		IO_WRITE(esd_base, ESD_DGCTRL0, t[0] | (1 << 31));	//reset fifo
		IO_WRITE(esd_base, ESD_DGCTRL1, t[1]);
		for (i = 0; i < 1000; i++) {
			unsigned dgctrl0;
			delayMicro(10);
			dgctrl0 = IO_READ(esd_base, ESD_DGCTRL0);
			if (!(dgctrl0 & (1 << 31)))
				break;
			my_printf("dgctrl0: %x\n", dgctrl0);
		}
		esd_reset(esd_base);
		exit_config_mode(esd_base);
	} else {
		unsigned t = 0;
		for (i = 0; i < 4; i++) {
			unsigned val = delay_to_cycle(vals[i], map, cdelay);
			t |= (val & 0xff) << (i << 3);
		}
		if (reg == ESD_WRDLCTL)
			my_printf("%s = %x\n", (reg == ESD_WRDLCTL) ? "WRDLCTL" : "RDDLCTL", t);
		enter_config_mode(esd_base);
		IO_WRITE(esd_base, reg, t);
		reset_fifo(esd_base);
		esd_reset(esd_base);
		exit_config_mode(esd_base);
	}
}

void load_reg(unsigned esd_base, unsigned *vals, unsigned reg, unsigned short *map, unsigned cdelay)
{
	int i;
	if (reg == ESD_DGCTRL0) {
		unsigned t[2];
		t[0] = IO_READ(esd_base, ESD_DGCTRL0);
		t[1] = IO_READ(esd_base, ESD_DGCTRL1);
		for (i = 0; i < 4; i++) {
			unsigned field = t[i >> 1];
			unsigned cycle;
			if (i & 1)
				field >>= 16;
			cycle = (field & 0x7f) | ((field & 0xf00) >> 1);
			vals[i] = cycle_to_delay(cycle, map, cdelay);
		}
		my_printf("load DGCTRL0 = %x DGCTRL1 = %x\n", t[0], t[1]);
	} else {
		unsigned t = 0;
		t = IO_READ(esd_base, reg);
		my_printf("load %s = %x\n", (reg == ESD_WRDLCTL) ? "WRDLCTL" : "RDDLCTL", t);
		for (i = 0; i < 4; i++) {
			vals[i] = cycle_to_delay(t & 0xff, map, cdelay);
			t >>= 8;
		}
	}
}

unsigned find_zero_point(unsigned esd_base)
{
	return 0;
}

#if 0
void add_mappings(unsigned short *map, unsigned prev_cycle, unsigned prev_delay, unsigned next_delay)
{
	for (;;) {
		if (prev_delay >= next_delay)
			break;
		map[prev_delay++] = prev_cycle;
	}
	if (prev_delay != next_delay)
		my_printf("map_delays error %x %x\n", prev_delay, next_delay);
}

void map_delays(unsigned esd_base, unsigned short *map, unsigned cdelay)
{
	unsigned cycle = 0;
	unsigned save = IO_READ(esd_base, ESD_DGCTRL0);
	int prev_c = 0;
	unsigned prev_delay = 0;
	unsigned v, t;
	int i;
	unsigned extra;
	for (;;) {
		unsigned val;
		val = cycle | ((cycle + 1) << 16);
		IO_WRITE(esd_base, ESD_DGCTRL0, val);
		v = IO_READ(esd_base, ESD_DGDLST);
		for (i = 0; i < 100; i++) {
			delayMicro(10);
			v = IO_READ(esd_base, ESD_DGDLST);
			if ((v & 0xff) && (v & 0xff00))
				break;
		}
		my_printf("DGCTRL0 = %x, DGDLST = %x\n", val, v);
		t = v & 0xff;
		add_mappings(map, prev_c, prev_delay, t);
		if (t != prev_delay) {
			prev_c = cycle;
			prev_delay = t;
		}
		t = (v >> 8) & 0xff;
		add_mappings(map, prev_c, prev_delay, t);
		if (t != prev_delay) {
			prev_c = cycle + 1;
			prev_delay = t;
		}
		cycle += 2;
		if (cycle >= 0x80)
			break;

	}
	add_mappings(map, prev_c, prev_delay, (cdelay >> 1));
	t = (cdelay >> 1);
	extra = 0x80;
	i = 0;
	for (;;) {
		if (t >= MAX_DELAY)
			break;
		map[t++] = map[i++] + extra;
		if (t == cdelay) {
			i = 0;
			extra = 0x100;
		}
	}
	IO_WRITE(esd_base, ESD_DGCTRL0, save);
}
#else
void map_delays(unsigned esd_base, unsigned short *map, unsigned cdelay)
{
	unsigned i;
	for (i = 0; i < MAX_DELAY; i++) {
		map[i] = (i << 8) / cdelay;
	}
}
#endif


struct check_outside_in;
struct check_inside_out;
typedef unsigned (*update_outside_in_rtn_t)(struct check_outside_in *p, unsigned cur, unsigned mask);
typedef unsigned (*update_inside_out_rtn_t)(struct check_inside_out *p, unsigned mask);


struct check_inside_out
{
	unsigned esd_base;
	unsigned reg;
	unsigned low_limit;
	unsigned high_limit;
	unsigned short *delay_to_cycle;
	unsigned cdelay;
	update_inside_out_rtn_t upd_rtn;
	unsigned cur[4];
	unsigned orig[4];
	struct check_inside_out *pio;
};

struct check_outside_in
{
	unsigned esd_base;
	unsigned reg;
	unsigned low_limit;
	unsigned high_limit;
	unsigned short *delay_to_cycle;
	unsigned cdelay;
	update_outside_in_rtn_t upd_rtn;
	unsigned *ram_start;
	unsigned *ram_stop;
	unsigned *ram_end;
	unsigned low_bound[32];
	unsigned high_bound[32];
	struct check_inside_out *pio;
};


//This routine starts with the current value, and
//spreads alternately to either side.
//Out: -1 means all checked, 0 new value loaded
int update_invalid(struct check_inside_out *p, unsigned mask)
{
	int ret = 0;
	int i;
	for (i = 0; i < 4; i++) {
		if (mask & 0xff) {
			int cur = p->cur[i];
			int diff = p->orig[i] - cur;
			int next =  p->orig[i] + diff;
			if (diff >= 0)
				next++;
			if (next > (int)p->high_limit)
				next = cur - 1;
			if (next < (int)p->low_limit) {
				next = cur;
				if (cur <= (int)p->low_limit)
					ret = -1;
				else
					next++;
			}
			if (next > (int)p->high_limit) {
				ret = -1;
				next = p->high_limit;
			}
			p->cur[i] = next;
		}
		mask >>= 8;
	}
	if (ret) {
		for (i = 0; i < 4; i++) {
			p->cur[i] = p->orig[i];
		}
	}
	return ret;
}


void load_current(struct check_inside_out *p, unsigned low, unsigned high)
{
	int i;
	load_reg(p->esd_base, p->cur, p->reg, p->delay_to_cycle, p->cdelay);
	p->low_limit = low;
	p->high_limit = high;
	for (i = 0; i < 4; i++) {
		unsigned cur = p->cur[i];
		if (cur > high)
			cur = high;
		if (cur < low)
			cur = low;
		p->orig[i] = p->cur[i] = cur;
	}
}

unsigned update_inside_out_rtn(struct check_inside_out *p, unsigned mask)
{
	unsigned ret = update_invalid(p, mask);
	store_reg(p->esd_base, p->cur, p->reg, p->delay_to_cycle, p->cdelay);
	if (ret) {
		struct check_inside_out *pio = p->pio;
		if (!pio)
			return mask;
		if (!pio->upd_rtn)
			return mask;
		if (pio->upd_rtn(pio, mask))
			return mask;
	}
	return 0;
}

unsigned _find_window(struct check_outside_in *p, unsigned low, unsigned high)
{
	int i;
	unsigned sum = 0;
	unsigned s;
	unsigned bound = low;
	unsigned down = 0;
	for (i = 0; i < 32; i++) {
		p->low_bound[i] = high;	//mark as invalid, high should be > low
		p->high_bound[i] = low;
	}
	for (;;) {
		unsigned test;
		p->upd_rtn(p, bound, ~sum);
		test = ~check_memory_pattern(p->ram_start, p->ram_stop, p->ram_end, sum);
		sum |= test;
		for (i = 0; test; i++, test >>= 1) {
			if (test & 1) {
				if (p->low_bound[i] > bound)
					p->low_bound[i] = bound;
				if (p->high_bound[i] < bound)
					p->high_bound[i] = bound;
			}
		}
		hw_watchdog_reset();
		if (sum == 0xffffffff) {
			//all boundaries found in this direction
			if (down)
				break;
			down = 1;
			sum = 0;
			bound = high;
		} else if (down) {
			if (bound <= low) {
				my_printf("!!!valid range not found valid mask = %x\n", sum);
				break;
			}
			bound--;
		} else {
			if (bound >= high) {
				down = 1;
				sum = 0;
				bound = high;
			} else
				bound++;
		}
	}
	s = p->upd_rtn(p, 0, 0);
	my_printf("Range checked: %x - %x, valid mask=%x, update mask=%x\n", low, high, sum, s);
	if (sum) for (i = 0; i < 32; i++) {
		int width = p->high_bound[i] - p->low_bound[i] + 1;
		if (width <= 0) {
			my_printf("DQ%2x invalid\n", i);
		} else {
			my_printf("DQ%2x range is %x to %x width=%x\n", i,
					p->low_bound[i], p->high_bound[i], width);
		}
	}
	return s;
}

unsigned find_window(struct check_outside_in *p, unsigned prev_bad_mask)
{
	unsigned sum;
	for (;;) {
		struct check_inside_out *pio;
		my_printf("testing memory range %x - %x  %x\n", p->ram_start, p->ram_stop, p->ram_end);
		write_memory_pattern(p->ram_start, p->ram_stop, p->ram_end);
		sum = _find_window(p, p->low_limit, p->high_limit);
		if (!sum)
			break;
		if ((~sum) & prev_bad_mask)
			break;
		pio = p->pio;
		if (!pio)
			break;
		if (!pio->upd_rtn)
			break;
		if (pio->upd_rtn(pio, sum))
			break;
		my_printf("error rewriting memory range %x - %x  %x sum=%x\n", p->ram_start, p->ram_stop, p->ram_end, sum);
	}
	return sum;
}

unsigned update_rtn(struct check_outside_in *p, unsigned cur, unsigned cur_mask)
{
	int i;
	unsigned ret = 0;
	unsigned vals[4];

	for (i = 0; i < 32; i += 8) {
		unsigned ave;
		if (cur_mask & 0xff) {
			ave = cur;
		} else {
			int j;
			int max = i + 8;
			unsigned low = p->low_limit;
			unsigned high = p->high_limit;
			//find smallest range for group
			for (j = i; j < max; j++) {
				if (p->low_bound[j] <= p->high_bound[j]) {
					if (low < p->low_bound[j])
						low = p->low_bound[j];
					if (high > p->high_bound[j])
						high = p->high_bound[j];
				} else {
					ret |= 1 << j;
				}
			}
			ave = (low + high) >> 1;
			if (low > high) {
				ret |= 0xff << i;
				if (cur)
					ave = cur;
			} else if (p->reg == ESD_DGCTRL0) {
				unsigned cycle_div2 = p->cdelay >> 1;
				if (high > cycle_div2) {
					unsigned g = high - cycle_div2;
					if (ave < g) {
						my_printf("high=%x low=%x ave=%x good=%x cdelay=%x\n",
							high, low, ave, g, p->cdelay);
						ave = g;
					}
				}
			}
		}
		vals[i >> 3] = ave;
		cur_mask >>= 8;
	}
	store_reg(p->esd_base, vals, p->reg, p->delay_to_cycle, p->cdelay);
	return ret;
}


unsigned update_rtn_rewrite(struct check_outside_in *p, unsigned cur, unsigned skip_mask)
{
	unsigned ret = update_rtn(p, cur, skip_mask);
	write_memory_pattern(p->ram_start, p->ram_stop, p->ram_end);
	return ret;
}

void get_limits(struct check_outside_in *p, unsigned *limits)
{
	int i;
	unsigned limit_high = 0;
	unsigned limit_low = 0xffffffff;
	for (i = 0; i < 32; i += 8) {
		int j;
		int max = i + 8;
		unsigned low = p->low_limit;
		unsigned llow = 0xffffffff;
		unsigned high = p->high_limit;
		unsigned hhigh = 0;
		//find smallest range for group
		for (j = i; j < max; j++) {
			unsigned l = p->low_bound[j];
			unsigned h = p->high_bound[j];
			if (l > h) {
				unsigned tmp = l;
				l = h;
				h = tmp;
			}
			if (llow > l)
				llow = l;
			if (low < l)
				low = l;
			if (hhigh < h)
				hhigh = h;
			if (high > h)
				high = h;
		}
		if (low > high) {
			unsigned tmp = low;
			low = high;
			high = tmp;
		}
		if (llow > hhigh) {
			unsigned tmp = llow;
			llow = hhigh;
			hhigh = tmp;
		}
		low = ((llow + 8) < low) ? (low - 8) : llow;
		high = (hhigh > (high + 8)) ? (high + 8) : hhigh;
		if (limit_low > low)
			limit_low = low;
		if (limit_high < high)
			limit_high = high;
	}
	limits[0] = limit_low;
	limits[1] = limit_high;
}

int calibrate_delay(unsigned esd_base, unsigned *ram_base, unsigned *ram_end, int differential)
{
	int ret = 0;
	unsigned zero_point;
	unsigned rd_limits[2];
	unsigned wd_limits[2];
	unsigned dg_limits[2];
	unsigned i;
	struct check_outside_in p;
	struct check_inside_out wd;
	struct check_inside_out dg;
	unsigned short d[MAX_DELAY];	//index is delay units, val is cycle*256
	unsigned cdelay;
	unsigned prev_bad_mask_rd = 0xffffffff;
	unsigned prev_bad_mask_wd = 0xffffffff;
	unsigned prev_bad_mask_dg = 0xffffffff;
	/* Set ESDMISC[11] to 1. FRC_MSR, force remeasurement */
//	IO_MOD(esd_base, ESD_MISC, 0, (1<<11));
	zero_point = find_zero_point(esd_base);
	cdelay = force_measure(esd_base);

	p.esd_base = wd.esd_base = dg.esd_base = esd_base;
	p.ram_start = ram_base;
	p.ram_stop = &ram_base[FAST_COUNT];
	p.ram_end = ram_end;
	wd.reg = ESD_WRDLCTL;
	dg.reg = ESD_DGCTRL0;
	wd.pio = (differential) ? &dg : NULL;
	dg.pio = 0;
	p.delay_to_cycle = d;
	wd.delay_to_cycle = d;
	dg.delay_to_cycle = d;
	wd.upd_rtn = update_inside_out_rtn;
	dg.upd_rtn = update_inside_out_rtn;
	p.cdelay = cdelay;
	wd.cdelay = cdelay;
	dg.cdelay = cdelay;
	map_delays(esd_base, d, cdelay);
	if (0) for (i = 0; i < MAX_DELAY; i += 8) {
		my_printf("%x: %x %x %x %x %x %x %x %x\n", i,
			d[i],d[i+1],d[i+2],d[i+3],d[i+4],d[i+5],d[i+6],d[i+7]);
	}
	rd_limits[0] = zero_point;
	rd_limits[1] = cycle_to_delay(0x7f, d, cdelay);
	wd_limits[0] = zero_point;
	wd_limits[1] = rd_limits[1];
	dg_limits[0] = 0;
	dg_limits[1] = cycle_to_delay(0x1ff, d, cdelay);
	my_printf("cycle value 0x7f has delay of %x, 0x1ff has delay of %x\n", rd_limits[1], dg_limits[1]);
	for (;;) {
		load_current(&wd, wd_limits[0], wd_limits[1]);
		load_current(&dg, dg_limits[0], dg_limits[1]);
		ret = 0;
		if (differential) {
			p.reg = ESD_DGCTRL0;
			p.pio = NULL;
			p.upd_rtn = update_rtn_rewrite;
			p.low_limit = dg_limits[0];
			p.high_limit = dg_limits[1];
			ret = find_window(&p, prev_bad_mask_dg);
			if (!ret)
				if (p.ram_stop == &ram_base[AGGRESSIVE_COUNT])
					get_limits(&p, dg_limits);
		}
		prev_bad_mask_dg &= ret;
		p.reg = ESD_RDDLCTL;
		p.pio = &wd;
		p.upd_rtn = update_rtn;
		p.low_limit = rd_limits[0];
		p.high_limit = rd_limits[1];
		ret = find_window(&p, prev_bad_mask_rd);
		if (ret && ((prev_bad_mask_rd & ret) == prev_bad_mask_rd))
			break;
		prev_bad_mask_rd &= ret;
		if (!ret)
			if (p.ram_stop == &ram_base[AGGRESSIVE_COUNT])
				get_limits(&p, rd_limits);

		p.upd_rtn = update_rtn_rewrite;
		p.low_limit = wd_limits[0];
		p.high_limit = wd_limits[1];
		p.reg = ESD_WRDLCTL;
		p.pio = (differential) ? &dg : NULL;
		ret = find_window(&p, prev_bad_mask_wd);
		prev_bad_mask_wd &= ret;
		if (!ret)
			if (p.ram_stop == &ram_base[AGGRESSIVE_COUNT])
				get_limits(&p, wd_limits);
		if (!(prev_bad_mask_rd | prev_bad_mask_wd | prev_bad_mask_dg)) {
			unsigned i;
			if (p.ram_stop == &ram_base[SLOW_COUNT])
				break;
			i = AGGRESSIVE_COUNT;
			if (p.ram_stop == &ram_base[i])
				i = SLOW_COUNT;
			p.ram_stop = &ram_base[i];
		}
		my_printf("prev_bad_mask_rd=%x, prev_bad_mask_wd=%x, prev_bad_mask_dg=%x\n", prev_bad_mask_rd, prev_bad_mask_wd, prev_bad_mask_dg);
	}
	return ret;
}


#define MR 0
#define EMR 1
#define EMR2 2
#define EMR3 3
#define MAKE_CMD(reg, addr, cs) (((reg) << 0) | ((addr) << 16) | (3 << 4) | ((cs) << 3) | 0x8000)

#define MODE_SINGLE 1
#define MODE_DIFFERENTIAL 0
#define DIFF_DQS_EN (1 << 15)

void iomuxc_ddr_differential(void);
void iomuxc_ddr_single(void);

void switch_to_mode(unsigned esd_base, unsigned mode)
{
	unsigned ctl;
	unsigned addr =  EMRS1_DRIVE_STRENGTH | EMRS1_ODT_TERM | (mode << EMRS1_DQS_SINGLE_BIT);
	ctl = IO_READ(esd_base, ESD_CTL);

	enter_config_mode(esd_base);
	//DQS# disable (single-ended)/ enable (differential), 50 ohms
	IO_WRITE(esd_base, ESD_SCR, MAKE_CMD(EMR, addr , 0));	//cs0
	if (ctl & (1 << 30))
		IO_WRITE(esd_base, ESD_SCR, MAKE_CMD(EMR, addr, 1));	//cs1

	IO_MOD(esd_base, ESD_DGCTRL0, (1 << 29), (mode << 29));
	if (mode == MODE_DIFFERENTIAL) {
		my_printf("Enabling differential DQS\n");
		iomuxc_ddr_differential();
	} else {
		my_printf("Disabling differential DQS\n");
		iomuxc_ddr_single();
	}
	exit_config_mode(esd_base);
	delayMicro(2);
}

unsigned check_mem(unsigned esd_base, unsigned *ram_base, unsigned *ram_end)
{
	unsigned *ram_stop = &ram_base[AGGRESSIVE_COUNT];
	unsigned mask;
	write_memory_pattern(ram_base, ram_stop, ram_end);
	mask = check_memory_pattern(ram_base, ram_stop, ram_end, 0);
	my_printf("check mask %x, ram_base=%x, ram_stop=%x, ram_end=%x\n", mask, ram_base, ram_stop, ram_end);
//	my_printf("DLYs: %x %x %x %x %x\n", IO_READ(esd_base, ESD_DLY1), IO_READ(esd_base, ESD_DLY2),
//			IO_READ(esd_base, ESD_DLY3), IO_READ(esd_base, ESD_DLY4),
//			IO_READ(esd_base, ESD_DLY5));
	return mask;
}


void set_compare_val(unsigned esd_base, unsigned val)
{
	enter_config_mode(esd_base);
	IO_MOD(esd_base, ESD_PDCMPR2, 3, 0);	//clear MPR_CMP, MPR_FULL_CMP
	esd_reset(esd_base);
	IO_WRITE(esd_base, ESD_PDCMPR1, val);
	exit_config_mode(esd_base);
}

#define DL(r) ((((r) & 0xf00) >> 1) | ((r) & 0x7f))
#define AVE(r) ((((r) & 0xffff) + ((r) >> 16)) >> 1)
void print_dg(unsigned esd_base)
{
	unsigned t0,t1,t2,t3;
	t0 = IO_READ(esd_base, ESD_DGCTRL0);
	t1 = IO_READ(esd_base, ESD_DGCTRL1);
	my_printf("DGCTRL0-1: %x %x  dl0=%x dl1=%x dl2=%x dl3=%x\n",
		t0, t1, DL(t0), DL(t0 >> 16), DL(t1), DL(t1 >> 16));
	t0 = IO_READ(esd_base, ESD_DGHWST0);
	t1 = IO_READ(esd_base, ESD_DGHWST1);
	t2 = IO_READ(esd_base, ESD_DGHWST2);
	t3 = IO_READ(esd_base, ESD_DGHWST3);
	my_printf("DGHWST0-3: %x %x %x %x average = %x %x %x %x\n",
		t0, t1,	t2, t3, AVE(t0), AVE(t1), AVE(t2), AVE(t3));
}

void print_settings(unsigned esd_base, unsigned ram_size)
{
	my_printf("RDDLCTL: %x  WRDLCTL: %x\n",
		IO_READ(esd_base, ESD_RDDLCTL), IO_READ(esd_base, ESD_WRDLCTL));
	my_printf("DGCTRL0: %x  DGCTRL1: %x\n",
		IO_READ(esd_base, ESD_DGCTRL0), IO_READ(esd_base, ESD_DGCTRL1));
	my_printf("ram_size: %x, tapeout=%x\n", ram_size, get_tapeout_version());
}

void hw_calibrate(unsigned esd_base, unsigned *ram_base)
{
	int i;
	unsigned *ram0_base = (unsigned *)0x80000000;	//The address does not have 0x70000000 subtracted 1st
//1. DQS gating calibration
	for (i = 0; i < 16; i++)
		ram0_base[i] = (i & 2)? 0x55555555 : 0xaaaaaaaa;
	cache_flush();
	set_compare_val(esd_base, 0x5555aaaa);
	reset_fifo(esd_base);
	IO_MOD(esd_base, ESD_DGCTRL0, 0, 1 << 28);	//start calibration
	for (i = 0; i < 1000; i++) {
		unsigned dgctrl0;
		delayMicro(10);
		dgctrl0 = IO_READ(esd_base, ESD_DGCTRL0);
		if (!(dgctrl0 & (1 << 28)))
			break;
		my_printf("dgctrl0: %x\n", dgctrl0);
	}
	reset_fifo(esd_base);

	print_dg(esd_base);
//2. DQS write level calibration (ddr3)
//3. DQS delay line calibration
	for (i = 0; i < 16; i++)
		ram0_base[i] = (i & 2)? 0x55555555 : 0xaaaaaaaa;
	if (0) my_printf("mem: %x %x %x %x   %x %x %x %x\n",
			ram0_base[0], ram0_base[1], ram0_base[2], ram0_base[3],
			ram0_base[4], ram0_base[5], ram0_base[6], ram0_base[7]);
	cache_flush();

	set_compare_val(esd_base, 0x5555aaaa);
	IO_WRITE(esd_base, ESD_RDDLHWCTL, 0x10);	//start read delay calibration
	for (i = 0; i < 1000; i++) {
		unsigned rddlhwctl;
		delayMicro(10);
		rddlhwctl = IO_READ(esd_base, ESD_RDDLHWCTL);
		if (!(rddlhwctl & 0x10))
			break;
		my_printf("RDDLHWCTL: %x\n", rddlhwctl);
	}

	my_printf("RDDLCTL: %x\n", IO_READ(esd_base, ESD_RDDLCTL));
	my_printf("RDDLST: %x\n", IO_READ(esd_base, ESD_RDDLST));
	my_printf("RDDLHWCTL: %x\n", IO_READ(esd_base, ESD_RDDLHWCTL));
	my_printf("RDDLHWST0-1: %x %x\n",
			IO_READ(esd_base, ESD_RDDLHWST0), IO_READ(esd_base, ESD_RDDLHWST1));

	for (i = 0; i < 16; i++)
		ram0_base[i] = 0;
	cache_flush();

	enter_config_mode(esd_base);
	IO_MOD(esd_base, ESD_PDCMPR2, 3, 0);	//clear MPR_CMP, MPR_FULL_CMP
	esd_reset(esd_base);
	IO_WRITE(esd_base, ESD_PDCMPR1, 0x5555aaaa);
	exit_config_mode(esd_base);

	IO_WRITE(esd_base, ESD_WRDLHWCTL, 0x10);	//start write delay calibration
	for (i = 0; i < 1000; i++) {
		unsigned wrdlhwctl;
		delayMicro(10);
		wrdlhwctl = IO_READ(esd_base, ESD_WRDLHWCTL);
		if (!(wrdlhwctl & 0x10))
			break;
		my_printf("WRDLHWCTL: %x\n", wrdlhwctl);
	}
	my_printf("WRDLCTL: %x\n", IO_READ(esd_base, ESD_WRDLCTL));
	my_printf("WRDLST: %x\n", IO_READ(esd_base, ESD_WRDLST));
	my_printf("WRDLHWCTL: %x\n", IO_READ(esd_base, ESD_WRDLHWCTL));
	my_printf("WRDLHWST0-1: %x %x\n",
			IO_READ(esd_base, ESD_WRDLHWST0), IO_READ(esd_base, ESD_WRDLHWST1));

	my_printf("mem: %x %x %x %x   %x %x %x %x\n",
			ram0_base[0], ram0_base[1], ram0_base[2], ram0_base[3],
			ram0_base[4], ram0_base[5], ram0_base[6], ram0_base[7]);
}

int plug_main(void **pstart, unsigned *pbytes, unsigned *pivt_offset)
{
	int ret = 0;
	unsigned mask = 0;
	unsigned *ram_base = (unsigned *)get_ram_base();
	unsigned ram_size = get_ram_size();
	unsigned *ram_end = (unsigned *)(((unsigned char *)ram_base) + ram_size);
	unsigned esd_base = ESD_BASE;
#if 1
	unsigned val;
	my_printf("start\n");
	enter_config_mode(esd_base);
//	val = IO_READ(esd_base, ESD_ZQHWCTRL);
//	my_printf("ZQHWCTRL: %x, ZQSWCTRL: %x\n", val, IO_READ(esd_base, ESD_ZQSWCTRL));
	IO_WRITE(esd_base, ESD_ZQHWCTRL, 0xa1380000 | (1 << 16));
	do {
		val = IO_READ(esd_base, ESD_ZQHWCTRL);
	} while (val & (1 << 16));
//	my_printf("ZQHWCTRL: %x, ZQSWCTRL: %x\n", val, IO_READ(esd_base, ESD_ZQSWCTRL));
//	IO_WRITE(esd_base, ESD_ZQHWCTRL, 0xa13800c0);
//	IO_WRITE(esd_base, ESD_ZQSWCTRL, (1<<13)|(0<<7)|(3<<2));
	exit_config_mode(esd_base);
	my_printf("ZQHWCTRL: %x, ZQSWCTRL: %x\n", IO_READ(esd_base, ESD_ZQHWCTRL), IO_READ(esd_base, ESD_ZQSWCTRL));
#endif
	if (1) {
		mask = check_mem(esd_base, ram_base, ram_end);
	}
//	hw_calibrate(esd_base, ram_base);
	if (0 && mask) {
		switch_to_mode(esd_base, MODE_SINGLE);
		ret = calibrate_delay(esd_base, ram_base, ram_end, 0);
		switch_to_mode(esd_base, MODE_DIFFERENTIAL);
	}
	ret = calibrate_delay(esd_base, ram_base, ram_end, 1);

	mask = check_mem(esd_base, ram_base, ram_end);
//	my_printf("Final DLYs: %x %x %x %x %x, gpr=%x mask=%x\n",
//			IO_READ(esd_base, ESD_DLY1), IO_READ(esd_base, ESD_DLY2),
//			IO_READ(esd_base, ESD_DLY3), IO_READ(esd_base, ESD_DLY4),
//			IO_READ(esd_base, ESD_DLY5), gpr, mask);
	print_settings(esd_base, ram_size);
	my_printf("done\n");
	for (;;) {
	}
	return ret;
}
