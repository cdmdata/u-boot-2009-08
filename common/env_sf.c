/*
 * (C) Copyright 2000-2002
 * Wolfgang Denk, DENX Software Engineering, wd@denx.de.
 *
 * (C) Copyright 2001 Sysgo Real-Time Solutions, GmbH <www.elinos.com>
 * Andreas Heppel <aheppel@sysgo.de>
 *
 * (C) Copyright 2008 Atmel Corporation
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
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston,
 * MA 02111-1307 USA
 */
#include <common.h>
#include <environment.h>
#include <malloc.h>
#include <spi_flash.h>
#define TEMPLATE_NAME "spi_flash"
int Xspi_flash_sect_protect(int protect, ulong start, ulong end);
void Xspi_flash_perror(int rc);
static struct spi_flash *env_flash;

#define TEMPLATE_sect_protect(protect, start, end)	Xspi_flash_sect_protect(protect, start, end)
#define TEMPLATE_perror(rc)			Xspi_flash_perror(rc)
#define TEMPLATE_sect_bounds(pstart, pend)	spi_flash_sect_bounds(env_flash, (u32 *)pstart, (u32 *)pend)
#define TEMPLATE_read(dest, offset, len)	spi_flash_read(env_flash, offset, len, dest)
#define TEMPLATE_write(src, offset, len)	spi_flash_write(env_flash, offset, len, src)
#define TEMPLATE_sect_erase(start, end)		spi_flash_erase(env_flash, start, (end) + 1 - (start))
#include "template_saveenv.h"

#ifndef CONFIG_ENV_SPI_BUS
# define CONFIG_ENV_SPI_BUS	0
#endif
#ifndef CONFIG_ENV_SPI_CS
# define CONFIG_ENV_SPI_CS		0
#endif
#ifndef CONFIG_ENV_SPI_MAX_HZ
# define CONFIG_ENV_SPI_MAX_HZ	1000000
#endif
#ifndef CONFIG_ENV_SPI_MODE
# define CONFIG_ENV_SPI_MODE	SPI_MODE_3
#endif

DECLARE_GLOBAL_DATA_PTR;

/* references to names in env_common.c */
extern uchar default_environment[];

static char * env_name_spec = "SPI Flash";

#ifdef CONFIG_FSL_ENV_IN_SF_FIRST
extern env_t *env_ptr;
#else
env_t *env_ptr;
#endif


#ifndef CONFIG_FSL_ENV_IN_SF_FIRST
uchar env_get_char_spec(int index)
{
	return *((uchar *)(gd->env_addr + index));
}
#endif

int Xspi_flash_sect_protect(int protect, ulong start, ulong end)
{
	return 0;
}
void Xspi_flash_perror(int rc)
{
	printf ("%s: rc=%d\n", __func__, rc);	
}

#ifdef CONFIG_FSL_ENV_IN_SF_FIRST
int saveenv_sf(void)
#else
int saveenv(void)
#endif
{
	if (!env_flash) {
#ifndef CONFIG_FSL_ENV_IN_SF_FIRST
		puts("Environment SPI flash not initialized\n");
#endif
		return 1;
	}
	printf ("Saving Environment to %s...\n", env_name_spec);
#ifdef CONFIG_ENV_SF_OFFSET2
	return template_saveenv(CONFIG_ENV_SF_SIZE, CONFIG_ENV_SF_OFFSET, CONFIG_ENV_SF_OFFSET2, env_ptr);
#else
	return template_saveenv(CONFIG_ENV_SF_SIZE, CONFIG_ENV_SF_OFFSET, 0, env_ptr);
#endif
}
extern unsigned env_size;

#ifdef CONFIG_FSL_ENV_IN_SF_FIRST
void env_sf_relocate_spec(void)
#else
void env_relocate_spec(void)
#endif
{
	int ret;

	env_flash = spi_flash_probe(CONFIG_ENV_SPI_BUS, CONFIG_ENV_SPI_CS,
			CONFIG_ENV_SPI_MAX_HZ, CONFIG_ENV_SPI_MODE);
	if (!env_flash)
		goto err_probe;

	ret = spi_flash_read(env_flash, CONFIG_ENV_SF_OFFSET, CONFIG_ENV_SF_SIZE, env_ptr);
	if (ret)
		goto err_read;
	env_size = (CONFIG_ENV_SF_SIZE - ENV_HEADER_SIZE);

	if (crc32(0, env_ptr->data, env_size) != env_ptr->crc)
		goto err_crc;

	gd->env_valid = 1;

	return;

err_read:
	spi_flash_free(env_flash);
	env_flash = NULL;
err_probe:
#ifdef CONFIG_FSL_ENV_IN_SF_FIRST
	return;
#endif
err_crc:
	puts("*** Warning - bad CRC, using default environment\n\n");
	set_default_env();
}

#ifndef CONFIG_FSL_ENV_IN_SF_FIRST
int env_init(void)
{
	/* SPI flash isn't usable before relocation */
	gd->env_addr = (ulong)&default_environment[0];
	gd->env_valid = 1;

	return 0;
}
#endif
