/*
 * embedflash.c
 *
 *  Created on: 2021/02/02
 *      Author: tenda_000
 */

#include <stdio.h>
#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include "main.h"
#include "monitor.h"
#include "embedflash.h"

//
// Sector7
//
#define SECTOR_NO 7UL
#define SECTOR_BASE 0x08060000
#define SECTOR_SIZE 0x00020000
#define PAGE_SIZE 256

void embed_erase (void)
{
	// unlock CR/SR
	FLASH->KEYR = 0x45670123;
	FLASH->KEYR = 0xcdef89ab;
	if (FLASH->CR & FLASH_CR_LOCK) {
		printf ("flash unlock failure\r\n");
		return;
	}
	while ((FLASH->SR & FLASH_SR_BSY) != 0)
		;			// wait for not-BSY

	FLASH->CR &= ~0x78;			// clear SNB field
	FLASH->CR |= (SECTOR_NO<<3);	// SNB = SECTOR_NO
	FLASH->CR &= ~(3UL<<8);		// clear PSIZE field
	FLASH->CR |= 2;				// PSIZE = 10
	//
	FLASH->CR |= FLASH_CR_SER;	// set SER bit
	FLASH->CR |= FLASH_CR_STRT;	// Erase start
	while ((FLASH->SR & FLASH_SR_BSY) != 0)
		;
	// lock CR
	FLASH->CR |= FLASH_CR_LOCK;
	return;
}

void embed_page_program (faddr_t dest, const byte_t* src, foff_t len)
{
	// len is 'byte' size, so actual copy number is 1/4 of len
	//uint32_t val;
	// unlock CR/SR
	FLASH->KEYR = 0x45670123;
	FLASH->KEYR = 0xcdef89ab;
	if (FLASH->CR & FLASH_CR_LOCK) {
		printf ("flash unlock failure\r\n");
		return;
	}
	while ((FLASH->SR & FLASH_SR_BSY) != 0)
		;			// wait for not -BSY
	FLASH->CR = (2UL<<8)|(SECTOR_NO<<3);
	//
	FLASH->CR |= FLASH_CR_PG;	// set PG bit (program start)

	uint32_t *dptr = (uint32_t *)((SECTOR_BASE + dest)&~3UL);
	uint32_t *sptr = (uint32_t *)(src);
	if (dest + len <= SECTOR_SIZE) {
		while (len > 0) {
			// wait for its competion
			while ((FLASH->SR & FLASH_SR_BSY) != 0)
				;
			// actual copying
			*dptr++ = *sptr++;
			len -= 4;
		}
		if (FLASH->SR & (FLASH_SR_PGAERR|FLASH_SR_PGPERR|FLASH_SR_PGSERR)) {
			printf ("error: %lX\r\n", FLASH->SR);
		}
	}
	// wait for its completion
	while ((FLASH->SR & FLASH_SR_BSY) != 0)
		;
	// lock CR
	FLASH->CR |= FLASH_CR_LOCK;
	return;
}

void embed_erase_write (faddr_t dest, const byte_t *src, foff_t len)
{
	embed_erase ();
	embed_page_program (dest, src, len);
}

void embed_read (byte_t *dest, faddr_t src, foff_t len)
{
	byte_t *sptr = (byte_t *)((SECTOR_BASE + src)&~3UL);
	memcpy (dest, sptr, len);
}
//
//
//
int do_embed_read_command (int ac, void *arg)
{
	arg_t *av = (arg_t *)arg;
	byte_t *ramaddr;
	faddr_t flashaddr;			// 24-bit length
	long len;

	if (ac == -3) {
		printf ("embed r ramaddr flashaddr len\r\n");
		return 0;
	}
	if (ac < 2) {
		printf ("args too few\r\n");
		return -1;
	}
	len = (ac == 2) ? 0x1000 : (long)av[2];
	if (av[0] == BADADDR || av[1] == BADADDR) {
		printf ("bad args\r\n");
		return -1;
	}
	ramaddr = PHYADDR (av[0]);
	flashaddr = (faddr_t)av[1];
	embed_read (ramaddr, flashaddr, len);
	return 0;
}

int do_embed_erase_write_command (int ac, void *arg)
{
	arg_t *av = (arg_t *)arg;
	byte_t *ramaddr;
	faddr_t flashaddr;		// 24-bit length
	long len;

	if (ac == -3) {
		printf ("embed ew ramaddr flashaddr len\r\n");
		return 0;
	}
	if (ac < 2) {
		printf ("args too few\r\n");
		return -1;
	}
	len = (ac == 2) ? 0x1000L : (long)av[2];
	if (av[0] == BADADDR || av[1] == BADADDR) {
		printf ("bad args\r\n");
		return -1;
	}
	ramaddr = PHYADDR (av[0]);
	flashaddr = (faddr_t)av[1];
	embed_erase_write (flashaddr, ramaddr, len);
	return 0;
}

int do_embed_erase_command (int ac, void *arg)
{
	if (ac == -3) {
		printf ("embed e\r\n");
		return 0;
	}
	embed_erase ();
	return 0;
}

//
// monitor command
//
int do_embed_command (int ac, void *ptr)
{
	int ncmds;
	struct cmd_entry *ep;
	static struct cmd_entry subcmd[] = {
			{ "r %x %x %x", 	do_embed_read_command },
			{ "ew %x %x %x",	do_embed_erase_write_command },
			{ "e %x %x",		do_embed_erase_command },
			{ 0, 0 }
	};
	if (ac == -3) {
		for (ep = subcmd; ep->format != 0; ep++)
			ep->func (-3, 0);
		return 0;
	}
	ncmds = sizeof subcmd / sizeof (struct cmd_entry);
	return do_command ((const char *)ptr, subcmd, ncmds, NULL);
}
