#include <stdarg.h>
#define DEBUG
#include "imx.h"
#include "mx5x_common.h"
#include "mx5x_ecspi.h"


extern unsigned chain_ivt;
extern unsigned program_end;
extern unsigned prog_start;

int plug_main2(void **pstart, unsigned *pbytes, unsigned *pivt_offset)
{
	my_printf("s2\n");
	return 0;	/* 1 is success, 0 is failure */
}

int plug_main(void **pstart, unsigned *pbytes, unsigned *pivt_offset)
{
	unsigned char *ram_base = (unsigned char *)get_ram_base();
	my_printf("s1: pstart=%x pbytes=%x pivt_offset=%x\n", pstart, pbytes, pivt_offset);
	if (!ram_test((unsigned *)ram_base)) {
		my_printf("ram error\n");
		return ERROR_MEMORY_TEST;
	}
	my_printf("ddr_init ram test passed!!!\n");
#if 1
	{
		struct common_info ci;
		int rtn;
		unsigned bytes;
		unsigned ivt_offset;
		ci.search = &chain_ivt;
		ci.initial_buf = &prog_start;
		ci.buf = &program_end;
		ci.hdr = 0;
		ci.cur_end = ci.end = ci.dest = 0;
		header_search(&ci);
		header_update_end(&ci);
		bytes = ci.end - ci.dest;
		rtn = common_exec_program(&ci);
		ivt_offset = ci.hdr - ci.dest;
		if (pstart)
			*pstart = ci.dest;
		if (pbytes)
			*pbytes = bytes;
		if (pivt_offset)
			*pivt_offset = ivt_offset;
		my_printf("s1: start=%x bytes=%x ivt_offset=%x\n", ci.dest, bytes, ivt_offset);
//		return 0;
		return (rtn) ? 1 : 0;	/* 1 is success, 0 is failure */
	}
#else
	//this works, but then mfgtool won't find chain_ivt for boot command
	return plug_main2(pstart, pbytes, pivt_offset);
#endif
}

