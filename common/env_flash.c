/*
 * (C) Copyright 2000-2002
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 *
 * (C) Copyright 2001 Sysgo Real-Time Solutions, GmbH <www.elinos.com>
 * Andreas Heppel <aheppel@sysgo.de>

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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */

/* #define DEBUG */

#include <common.h>
#include <command.h>
#include <environment.h>
#include <linux/stddef.h>
#include <malloc.h>

#define TEMPLATE_NAME "flash"
int flash_sect_bounds(ulong *pstart, ulong *pend);
int flash_read(void *dest, ulong offset, unsigned len);

#define TEMPLATE_sect_protect(protect, start, end)	flash_sect_protect(protect, start, end)
#define TEMPLATE_perror(rc)			flash_perror(rc)
#define TEMPLATE_sect_bounds(pstart, pend)	flash_sect_bounds(pstart, pend)
#define TEMPLATE_read(dest, offset, len)	flash_read(dest, offset, len)
#define TEMPLATE_write(src, offset, len)	flash_write(src, offset, len)
#define TEMPLATE_sect_erase(start, end)		flash_sect_erase(start, end)
#include "template_saveenv.h"

DECLARE_GLOBAL_DATA_PTR;

#if defined(CONFIG_CMD_SAVEENV) && defined(CONFIG_CMD_FLASH)
#define CMD_SAVEENV
#elif defined(CONFIG_ENV_ADDR_REDUND)
#error Cannot use CONFIG_ENV_ADDR_REDUND without CONFIG_CMD_SAVEENV & CONFIG_CMD_FLASH
#endif

#if defined(CONFIG_ENV_SIZE_REDUND) && (CONFIG_ENV_SIZE_REDUND < CONFIG_ENV_SIZE)
#error CONFIG_ENV_SIZE_REDUND should not be less then CONFIG_ENV_SIZE
#endif

#ifdef CONFIG_INFERNO
# ifdef CONFIG_ENV_ADDR_REDUND
#error CONFIG_ENV_ADDR_REDUND is not implemented for CONFIG_INFERNO
# endif
#endif

char * env_name_spec = "Flash";

#ifdef ENV_IS_EMBEDDED

extern uchar environment[];
env_t *env_ptr = (env_t *)(&environment[0]);

#ifdef CMD_SAVEENV
/* static env_t *flash_addr = (env_t *)(&environment[0]);-broken on ARM-wd-*/
static env_t *flash_addr = (env_t *)CONFIG_ENV_ADDR;
#endif

#else /* ! ENV_IS_EMBEDDED */

env_t *env_ptr = (env_t *)CONFIG_ENV_ADDR;
#ifdef CMD_SAVEENV
static env_t *flash_addr = (env_t *)CONFIG_ENV_ADDR;
#endif

#endif /* ENV_IS_EMBEDDED */

#ifdef CONFIG_ENV_ADDR_REDUND
static env_t *flash_addr_new = (env_t *)CONFIG_ENV_ADDR_REDUND;

#define ACTIVE_FLAG   1
#define OBSOLETE_FLAG 0
#endif /* CONFIG_ENV_ADDR_REDUND */

extern uchar default_environment[];


uchar env_get_char_spec (int index)
{
	return ( *((uchar *)(gd->env_addr + index)) );
}

#ifdef CONFIG_ENV_ADDR_REDUND

int  env_init(void)
{
	int crc1_ok = 0, crc2_ok = 0;

	uchar flag1 = flash_addr->flags;
	uchar flag2 = flash_addr_new->flags;

	ulong addr_default = (ulong)&default_environment[0];
	ulong addr1 = (ulong)&(flash_addr->data);
	ulong addr2 = (ulong)&(flash_addr_new->data);

	crc1_ok = (crc32(0, flash_addr->data, ENV_SIZE) == flash_addr->crc);
	crc2_ok = (crc32(0, flash_addr_new->data, ENV_SIZE) == flash_addr_new->crc);

	if (crc1_ok && ! crc2_ok) {
		gd->env_addr  = addr1;
		gd->env_valid = 1;
	} else if (! crc1_ok && crc2_ok) {
		gd->env_addr  = addr2;
		gd->env_valid = 1;
	} else if (! crc1_ok && ! crc2_ok) {
		gd->env_addr  = addr_default;
		gd->env_valid = 0;
	} else if (flag1 == ACTIVE_FLAG && flag2 == OBSOLETE_FLAG) {
		gd->env_addr  = addr1;
		gd->env_valid = 1;
	} else if (flag1 == OBSOLETE_FLAG && flag2 == ACTIVE_FLAG) {
		gd->env_addr  = addr2;
		gd->env_valid = 1;
	} else if (flag1 == flag2) {
		gd->env_addr  = addr1;
		gd->env_valid = 2;
	} else if (flag1 == 0xFF) {
		gd->env_addr  = addr1;
		gd->env_valid = 2;
	} else if (flag2 == 0xFF) {
		gd->env_addr  = addr2;
		gd->env_valid = 2;
	}

	return (0);
}
#else /* ! CONFIG_ENV_ADDR_REDUND */

int  env_init(void)
{
	if (crc32(0, env_ptr->data, ENV_SIZE) == env_ptr->crc) {
		gd->env_addr  = (ulong)&(env_ptr->data);
		gd->env_valid = 1;
		return(0);
	}

	gd->env_addr  = (ulong)&default_environment[0];
	gd->env_valid = 0;
	return (0);
}
#endif /* CONFIG_ENV_ADDR_REDUND */


void env_relocate_spec (void)
{
#if !defined(ENV_IS_EMBEDDED) || defined(CONFIG_ENV_ADDR_REDUND)
#ifdef CONFIG_ENV_ADDR_REDUND
	if (gd->env_addr != (ulong)&(flash_addr->data)) {
		env_t * etmp = flash_addr;
		ulong ltmp = end_addr;

		flash_addr = flash_addr_new;
		flash_addr_new = etmp;

		end_addr = end_addr_new;
		end_addr_new = ltmp;
	}

	if (flash_addr_new->flags != OBSOLETE_FLAG &&
	    crc32(0, flash_addr_new->data, ENV_SIZE) ==
	    flash_addr_new->crc) {
		char flag = OBSOLETE_FLAG;

		gd->env_valid = 2;
		flash_sect_protect (0, (ulong)flash_addr_new, end_addr_new);
		flash_write(&flag,
			    (ulong)&(flash_addr_new->flags),
			    sizeof(flash_addr_new->flags));
		flash_sect_protect (1, (ulong)flash_addr_new, end_addr_new);
	}

	if (flash_addr->flags != ACTIVE_FLAG &&
	    (flash_addr->flags & ACTIVE_FLAG) == ACTIVE_FLAG) {
		char flag = ACTIVE_FLAG;

		gd->env_valid = 2;
		flash_sect_protect (0, (ulong)flash_addr, end_addr);
		flash_write(&flag,
			    (ulong)&(flash_addr->flags),
			    sizeof(flash_addr->flags));
		flash_sect_protect (1, (ulong)flash_addr, end_addr);
	}

	if (gd->env_valid == 2)
		puts ("*** Warning - some problems detected "
		      "reading environment; recovered successfully\n\n");
#endif /* CONFIG_ENV_ADDR_REDUND */
#ifdef CMD_SAVEENV
	memcpy (env_ptr, (void*)flash_addr, CONFIG_ENV_SIZE);
#endif
#endif /* ! ENV_IS_EMBEDDED || CONFIG_ENV_ADDR_REDUND */
}

#ifdef CMD_SAVEENV
int flash_read(void *dest, ulong offset, unsigned len)
{
	memcpy(dest, (void *)offset, len);
	return 0;
}

int saveenv(void)
{
#ifdef CONFIG_ENV_ADDR_REDUND
	return template_saveenv(CONFIG_ENV_SIZE, (ulong)flash_addr, (ulong)flash_addr_new, env_ptr);
#else
	return template_saveenv(CONFIG_ENV_SIZE, (ulong)flash_addr, 0, env_ptr);
#endif
}
#endif
