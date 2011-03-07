/*
 * Command for timing other commands
 *
 * Copyright (C) 2011 Boundary Devices
 */
#include <common.h>
#include <command.h>

static int do_time(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	int rval ;
	unsigned long long start ;
	unsigned long elapsed ;

	/* need at least two arguments */
	if (argc < 2)
		goto usage;

	/* once more to measure things */
        reset_timer();
        start = get_timer(0);

	cmdtp = find_cmd(argv[1]);
	if (0 == cmdtp)
		goto usage ;
	cmdtp->cmd(cmdtp,flag,argc-1,argv+1);

	elapsed = (unsigned long)(get_timer(0)-start);
	printf( "%lu ticks: %lu Hz\n", elapsed, CONFIG_SYS_HZ/(elapsed?elapsed:1));
	return rval ;
usage:
	cmd_usage(cmdtp);
	return 1;
}

U_BOOT_CMD(
	time,	255,	1,	do_time,
	"Time a command",
	"time command [args]		- time the execution of command" );
