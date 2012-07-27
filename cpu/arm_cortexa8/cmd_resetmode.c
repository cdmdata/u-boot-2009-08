/*
 * Copyright (C) 2012 Boundary Devices Inc.
 *
 * See file CREDITS for list of people who contributed to this
 * project.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of
 * the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */
#include <common.h>
#include <asm/errno.h>
#include <asm/io.h>
#include <asm/imx-common/resetmode.h>
#include <malloc.h>

extern int do_reset(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[]);

enum command_ret_t {
	CMD_RET_SUCCESS,	/* 0 = Success */
	CMD_RET_FAILURE,	/* 1 = Failure */
	CMD_RET_USAGE = -1,	/* Failure, please report 'usage' error */
};

const struct reset_mode *modes[2];

const struct reset_mode *search_modes(char *arg)
{
	int i;

	for (i = 0; i < ARRAY_SIZE(modes); i++) {
		const struct reset_mode *p = modes[i];
		if (p) {
			while (p->name) {
				if (!strcmp(p->name, arg))
					return p;
				p++;
			}
		}
	}
	return NULL;
}

int create_usage(char *dest)
{
	int i;
	int size = 0;

	for (i = 0; i < ARRAY_SIZE(modes); i++) {
		const struct reset_mode *p = modes[i];
		if (p) {
			while (p->name) {
				int len = strlen(p->name);
				if (dest) {
					memcpy(dest, p->name, len);
					dest += len;
					*dest++ = '|';
				}
				size += len + 1;
				p++;
			}
		}
	}
	if (dest)
		dest[-1] = 0;
	return size;
}

int do_resetmode(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	const struct reset_mode *p;

	if (argc < 2)
		return CMD_RET_USAGE;
	p = search_modes(argv[1]);
	if (!p)
		return CMD_RET_USAGE;
	reset_mode_apply(p->cfg_val);
	return 0;
}

U_BOOT_CMD(
	resetmode, 2, 0, do_resetmode,
	NULL,
	"");

void add_board_resetmodes(const struct reset_mode *p)
{
	int size;
	char *dest;

	if (__u_boot_cmd_resetmode.usage) {
		free(__u_boot_cmd_resetmode.usage);
		__u_boot_cmd_resetmode.usage = NULL;
	}

	modes[0] = p;
	modes[1] = soc_reset_modes;
	size = create_usage(NULL);
	dest = malloc(size);
	if (dest) {
		create_usage(dest);
		__u_boot_cmd_resetmode.usage = dest;
	}
}

int do_resetmode_cmd(char *arg)
{
	const struct reset_mode *p;

	p = search_modes(arg);
	if (!p) {
		printf("%s not found\n", arg);
		return CMD_RET_FAILURE;
	}
	reset_mode_apply(p->cfg_val);
	do_reset(NULL, 0, 0, NULL);
	return 0;
}

int do_bootmmc0(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	return do_resetmode_cmd("mmc0");
}

int do_bootmmc1(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	return do_resetmode_cmd("mmc1");
}

int do_download_mode(cmd_tbl_t *cmdtp, int flag, int argc, char *argv[])
{
	return do_resetmode_cmd("usb");
}

U_BOOT_CMD(
	bootmmc0, 1, 0, do_bootmmc0,
	"boot to mmc 0",
	"");

U_BOOT_CMD(
	bootmmc1, 1, 0, do_bootmmc1,
	"boot to mmc 1",
	"");

U_BOOT_CMD(
	download_mode, 1, 0, do_download_mode,
	"start rom loader",
	"");
