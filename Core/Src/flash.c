/*
 * flash.c
 *
 *  Created on: 2021/01/30
 *      Author: tenda_000
 */

#include <stdio.h>
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include "monitor.h"
#include "xspi.h"
#include "flash.h"

//
// SPI FLASH primitives
//
static void spi_send_cmd (byte_t cmd)
{
	spi_xfer (cmd, 0);
}

static void spi_send_addr (long addr)
{
	spi_xfer ((addr>>16)&0xff, 0);
	spi_xfer ((addr>>8)&0xff, 0);
	spi_xfer ((addr>>0)&0xff, 0);
}

static void spi_send_dummy (int count)
{
	while (count-- > 0)
		spi_xfer (0, 0);
}

static void spi_receive_data (byte_t *p, int count)
{
	while (count-- > 0) {
		spi_xfer (0, p++);
	}
}

static void spi_send_data (byte_t *p, int count)
{
	while (count-- > 0) {
		spi_xfer (*p++, 0);
	}
}

//
// SPI FLASH commands
//
#define READ_STATUS 5
#define WRITE_STATUS 1
#define WRITE_ENABLE 6
#define READ_DATA 3
#define PAGE_PROGRAM 2
#define SECTOR_ERASE 0x20
#define READ_UNIQUEID 0x7B
#define READ_MANUID 0x90
#define READ_JEDECID 0x9F


#define DELAY(n) do { for (int volatile i = 0; i < n; ++i); } while (0)


static void flash_cmd (byte_t cmd, uint32_t addr, int delay, byte_t *data, long count)
{
	long n;
	switch (cmd) {
	case READ_STATUS:	// 05: Read Status Register-1
		// length is 2.
		spi_cs_reset ();
		spi_send_cmd (cmd);
		spi_receive_data (data, 2);
		spi_cs_set ();
		break;
	case WRITE_STATUS:	// 01: Write Status Register-1
		// length is 2
		spi_cs_reset ();
		spi_send_cmd (cmd);
		spi_send_data (data, 2);
		spi_cs_set ();
		break;
	case WRITE_ENABLE:			// 06: Write Enable
		spi_cs_reset ();
		spi_send_cmd (cmd);
		spi_cs_set ();
		break;
	case READ_DATA:				// 03: Read Data
		spi_cs_reset ();
		spi_send_cmd (cmd);
		spi_send_addr (addr);
		spi_receive_data (data, count);
		spi_cs_set ();
		break;
	case PAGE_PROGRAM:			// 02: Page Program (write data)
		// date length must be <256
		// lower 8bit of address circulate, so specified address should be 00, and count be 256
		n = (count > PAGE_SIZE) ? PAGE_SIZE : count;
		spi_cs_reset ();
		spi_send_cmd (cmd);
		spi_send_addr (addr);
		spi_send_data (data, n);
		spi_cs_set ();
		break;
	case SECTOR_ERASE:			// 20: Sector Erase (4k erase)
		spi_cs_reset ();
		spi_send_cmd (cmd);
		spi_send_addr (addr);
		spi_cs_set ();		// Sector Erase Command Execution Start
		// need to wait for its completion with Read Status Register command
		break;
	case READ_MANUID:			// 90: Read Manufacturer/Device ID
		spi_cs_reset ();
		spi_send_cmd (cmd);
		spi_send_addr (0);
		spi_receive_data (data, 2);		// return 2 byte
		spi_cs_set ();
		break;
	case READ_UNIQUEID:			// 4B: Read Unique ID Numer
		spi_cs_reset ();
		spi_send_cmd (cmd);
		spi_send_dummy (4);	// dummy 4 byte
		spi_receive_data (data, 8);
		spi_cs_set ();
		break;
	case READ_JEDECID:		// 9F: Read JEDEC ID
		spi_cs_reset ();
		spi_send_cmd (cmd);
		spi_receive_data (data, 3);
		spi_cs_set ();
		break;
	}
}

//
// commands
//

static void flash_write_enable (void)
{
	flash_cmd (WRITE_ENABLE, 0, 0, 0, 0);
}

static uint16_t flash_read_status (void)
{
	byte_t buf[2];
	flash_cmd (READ_STATUS, 0, 0, &buf[0], 2);
	return (((uint16_t)buf[0])<<8)|buf[1];
}

static void flash_read_data (byte_t *data, faddr_t addr, long len)
{
	flash_cmd (READ_DATA, addr, 0, data, len);
}

static void flash_page_program (faddr_t addr, byte_t *data, long len)
{
//	printf ("PP: %06lX %ld\r\n", addr, len);
	flash_write_enable ();
	flash_cmd (PAGE_PROGRAM, addr, 0, data, len);
	while ((flash_read_status () & (1<<0)) != 0)
		DELAY(10);
	// wait for the commmand completion
}


static void flash_sector_erase (faddr_t addr)
{
//	printf ("SE: %06lX\r\n", addr);
	flash_write_enable ();
	flash_cmd (SECTOR_ERASE, addr, 0, 0, 0);
	while ((flash_read_status () & (1<<0)) != 0)
		DELAY(100);
	// wait for the commmand completion
}

//
// public functions
//
void flash_read (byte_t *data, faddr_t addr, foff_t len)
{
//	printf ("RD: %06lX %ld\r\n", addr, len);
	flash_read_data (data, addr, len);
}

void flash_erase (faddr_t addr, long len)
{
	len = (len + SECTOR_SIZE - 1) / SECTOR_SIZE * SECTOR_SIZE;
	while (len > 0) {
		flash_sector_erase (addr);
		addr += SECTOR_SIZE;
		len -= SECTOR_SIZE;
	}
}

void flash_write (faddr_t dest, byte_t *src, foff_t len)
{
	long size;
	len = (len + PAGE_SIZE - 1) / PAGE_SIZE * PAGE_SIZE;
	// erase 4096 byte
	while (len > 0) {
		size = (len > PAGE_SIZE) ? PAGE_SIZE : len;
		flash_page_program (dest, src, size);
		src += size;
		len -= size;
	}
}

void flash_erase_write (faddr_t dest, byte_t *src, foff_t len)
{
	// Sector Enable
	faddr_t addr = (dest + SECTOR_SIZE - 1) / SECTOR_SIZE * SECTOR_SIZE;
	long size;
	// erase 4096 byte
	while (len > 0) {
		flash_sector_erase (addr);
		for (int i = 0; len > 0 && i < (SECTOR_SIZE / PAGE_SIZE); ++i) {
			size = len > PAGE_SIZE ? PAGE_SIZE : len;
			flash_page_program (addr, src, size);
			addr += size;
			src += size;
			len -= size;
		}
	}
}

//
// monitor command
//
int do_flash_read_command (int ac, void *arg)
{
	arg_t *av = (arg_t *)arg;
	byte_t *ramaddr;
	faddr_t flashaddr;			// 24-bit (or some more) length
	long len;

	if (ac == -3) {
		printf ("flash r ramaddr flashaddr len\r\n");
		return 0;
	}
	if (ac < 2) {
		printf ("args too few\r\n");
		return -1;
	}
	len = (ac == 2) ? SECTOR_SIZE : (foff_t)av[2];	// default length is SECTOR_SIZE, or 4KB
	if (av[0] == BADADDR || av[1] == BADADDR) {
		printf ("bad args\r\n");
		return -1;
	}
	ramaddr = PHYADDR (av[0]);
	flashaddr = (faddr_t)av[1];
	flash_read (ramaddr, flashaddr, len);
	return 0;
}

int do_flash_erase_write_command (int ac, void *arg)
{
	arg_t *av = (arg_t *)arg;
	char *ramaddr;
	long flashaddr;		// 24-bit length
	long len;

	if (ac == -3) {
		printf ("flash ew ramaddr flashaddr len\r\n");
		return 0;
	}
	if (ac < 2) {
		printf ("args too few\r\n");
		return -1;
	}
	len = (ac == 2) ? SECTOR_SIZE : (long)av[2];
	if (av[0] == BADADDR || av[1] == BADADDR) {
		printf ("bad args\r\n");
		return -1;
	}
	ramaddr = PHYADDR (av[0]);
	flashaddr = (faddr_t)av[1];
	flash_erase_write (flashaddr, ramaddr, len);
	return 0;
}

int do_flash_erase_command (int ac, void *arg)
{
	arg_t *av = (arg_t *)arg;
	long flashaddr;		// 24-bit length
	long len = -1;

	if (ac == -3) {
		printf ("flash e addr len\r\n");
		return 0;
	}
	if (ac < 1) {
		printf ("args too few\r\n");
	}
	len = (ac == 1) ? SECTOR_SIZE : (long)av[2];
	if (av[0] == BADADDR) {
		printf ("bad args\r\n");
		return -1;
	}
	flashaddr = (faddr_t)av[0];
	flash_erase (flashaddr, len);
	return 0;
}

int do_flash_command (int n, void *ptr)
{
	int ncmds;
	struct cmd_entry *ep;
	static struct cmd_entry subcmd[] = {
			{ "r %x %x %x", 	do_flash_read_command },
			{ "ew %x %x %x",	do_flash_erase_write_command },
			{ "e %x %x",		do_flash_erase_command },
			{ 0, 0 }
	};
	// help command
	if (n == -3) {
		for (ep = subcmd; ep->format != 0; ++ep) {
			ep->func (-3, 0);
		}
		return 0;
	}
	// normal command
	ncmds = sizeof (subcmd) / sizeof (struct cmd_entry);
	return do_command ((const char *)ptr, subcmd, ncmds, NULL);
}
