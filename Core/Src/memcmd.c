/*
 * memcmd.c
 *
 *  Created on: 2021/02/03
 *      Author: tenda_000
 */

//
// fill, src commands
//

#include <stdio.h>
#include <inttypes.h>
#include <stdlib.h>
#include "monitor.h"
#include "memcmd.h"

static uint32_t do_setcmd (arg_t saddr, int mod);
static void do_dump (int ac, void *arg, int mod);
//
// sumcheck/crc16
//
/* CRC16 implementation acording to CCITT standards */

static const unsigned short crc16tab[256]= {
	0x0000,0x1021,0x2042,0x3063,0x4084,0x50a5,0x60c6,0x70e7,
	0x8108,0x9129,0xa14a,0xb16b,0xc18c,0xd1ad,0xe1ce,0xf1ef,
	0x1231,0x0210,0x3273,0x2252,0x52b5,0x4294,0x72f7,0x62d6,
	0x9339,0x8318,0xb37b,0xa35a,0xd3bd,0xc39c,0xf3ff,0xe3de,
	0x2462,0x3443,0x0420,0x1401,0x64e6,0x74c7,0x44a4,0x5485,
	0xa56a,0xb54b,0x8528,0x9509,0xe5ee,0xf5cf,0xc5ac,0xd58d,
	0x3653,0x2672,0x1611,0x0630,0x76d7,0x66f6,0x5695,0x46b4,
	0xb75b,0xa77a,0x9719,0x8738,0xf7df,0xe7fe,0xd79d,0xc7bc,
	0x48c4,0x58e5,0x6886,0x78a7,0x0840,0x1861,0x2802,0x3823,
	0xc9cc,0xd9ed,0xe98e,0xf9af,0x8948,0x9969,0xa90a,0xb92b,
	0x5af5,0x4ad4,0x7ab7,0x6a96,0x1a71,0x0a50,0x3a33,0x2a12,
	0xdbfd,0xcbdc,0xfbbf,0xeb9e,0x9b79,0x8b58,0xbb3b,0xab1a,
	0x6ca6,0x7c87,0x4ce4,0x5cc5,0x2c22,0x3c03,0x0c60,0x1c41,
	0xedae,0xfd8f,0xcdec,0xddcd,0xad2a,0xbd0b,0x8d68,0x9d49,
	0x7e97,0x6eb6,0x5ed5,0x4ef4,0x3e13,0x2e32,0x1e51,0x0e70,
	0xff9f,0xefbe,0xdfdd,0xcffc,0xbf1b,0xaf3a,0x9f59,0x8f78,
	0x9188,0x81a9,0xb1ca,0xa1eb,0xd10c,0xc12d,0xf14e,0xe16f,
	0x1080,0x00a1,0x30c2,0x20e3,0x5004,0x4025,0x7046,0x6067,
	0x83b9,0x9398,0xa3fb,0xb3da,0xc33d,0xd31c,0xe37f,0xf35e,
	0x02b1,0x1290,0x22f3,0x32d2,0x4235,0x5214,0x6277,0x7256,
	0xb5ea,0xa5cb,0x95a8,0x8589,0xf56e,0xe54f,0xd52c,0xc50d,
	0x34e2,0x24c3,0x14a0,0x0481,0x7466,0x6447,0x5424,0x4405,
	0xa7db,0xb7fa,0x8799,0x97b8,0xe75f,0xf77e,0xc71d,0xd73c,
	0x26d3,0x36f2,0x0691,0x16b0,0x6657,0x7676,0x4615,0x5634,
	0xd94c,0xc96d,0xf90e,0xe92f,0x99c8,0x89e9,0xb98a,0xa9ab,
	0x5844,0x4865,0x7806,0x6827,0x18c0,0x08e1,0x3882,0x28a3,
	0xcb7d,0xdb5c,0xeb3f,0xfb1e,0x8bf9,0x9bd8,0xabbb,0xbb9a,
	0x4a75,0x5a54,0x6a37,0x7a16,0x0af1,0x1ad0,0x2ab3,0x3a92,
	0xfd2e,0xed0f,0xdd6c,0xcd4d,0xbdaa,0xad8b,0x9de8,0x8dc9,
	0x7c26,0x6c07,0x5c64,0x4c45,0x3ca2,0x2c83,0x1ce0,0x0cc1,
	0xef1f,0xff3e,0xcf5d,0xdf7c,0xaf9b,0xbfba,0x8fd9,0x9ff8,
	0x6e17,0x7e36,0x4e55,0x5e74,0x2e93,0x3eb2,0x0ed1,0x1ef0
};

int do_crc16_command (int ac, void *arg)
{
	uint32_t *av = (uint32_t *)arg;
	byte_t *top, *p;
	int c, size, sum, len;
	unsigned short crc;
	size = 0;
	crc = 0;
	sum = 0;
	if (ac == -3) {
		printf ("crc addr len\r\n");
		return 0;
	}
	if (ac < 2) {
		printf ("do_crc16: arg too short\r\n");
		return -1;
	}
	p = top = PHYADDR(av[0]);
	len = av[1];
	while (len-- > 0) {
		c = *p++;
		crc = (crc<<8) ^ crc16tab[((crc>>8) ^ c)&0x00FF];
		sum += (unsigned char)c;
		size++;
	}
	printf ("crc: %04X, sum:%X, len:%d\r\n", crc, sum, size);
	return 0;
}

int do_fill_command (int ac, void *arg)
{
	uint32_t *av = (uint32_t *)arg;
	if (ac == -3) {
		printf ("fill cc addr len\r\n");
		return 0;
	}

	if (ac < 3) {
		printf ("do_fill: arg too short\r\n");
		return -1;
	}
	uint8_t c = av[0];
	byte_t *top = PHYADDR (av[1]);
	int len = av[2];
	while (len-- > 0)
		*top++ = c;
	return 0;
}

int do_set_byte (int ac, void *arg)
{
	if (ac == -3) {
		printf ("s addr\r\n");
		return 0;
	}
	arg_t *av = arg;
	if (ac < 1) {
		return -1;
	}
	return do_setcmd (av[0], 1);
}

int do_set_word (int ac, void *arg)
{	if (ac == -3) {
	printf ("dw addr len\r\n");
	return 0;
}

	arg_t *av = (arg_t *)arg;
	if (ac < 1) {
		return -1;
	}
	return do_setcmd (av[0], 2);
}

int do_set_long (int ac, void *arg)
{
	if (ac == -3) {
		printf ("dl addr len\r\n");
		return 0;
	}

	arg_t *av = (arg_t *)arg;
	if (ac < 1) {
		return -1;
	}
	return do_setcmd (av[0], 4);
}


static uint32_t do_setcmd (arg_t arg, int mod)
{
	uint32_t saddr = (uint32_t)arg;
	char buf[16];
	int c;

	printf ("do_setcmd: %X, %d\r\n", (unsigned int)saddr, mod);
	saddr &= ~(mod - 1);
	while (1) {
		printf ("%04lX ", saddr);
		uint32_t val;
		switch (mod) {
		case 1: printf ("%02X ", *((uint8_t *)PHYADDR(saddr))); break;
		case 2: printf ("%04X ", *((uint16_t *)PHYADDR(saddr))); break;
		case 4: printf ("%08lX ", *((uint32_t *)PHYADDR(saddr))); break;
		}
		if ((c = getch ()) == '.') {
			putchar (c);
			crlf ();
			break;
		} else if (c == '\r' || c == '\n') {
			printf ("\r\n");
			saddr += mod;
			continue;
		}
		ungetch (c);
		readline (buf, sizeof buf - 1);
		val = strtoul (buf, NULL, 16);
		switch (mod) {
		case 1: *((uint8_t *)PHYADDR(saddr)) = val; break;
		case 2: *((uint16_t *)PHYADDR(saddr)) = val; break;
		case 4: *((uint32_t *)PHYADDR(saddr)) = val; break;
		}
		saddr += mod;
		crlf ();
	}
	return saddr;
}

int do_dump_byte (int ac, void *arg)
{
	if (ac == -3) {
		printf ("d addr len\r\n");
		return 0;
	}
	do_dump (ac, arg, 1);
	return 0;
}

int do_dump_word (int ac, void *arg)
{
	if (ac == -3) {
		printf ("dw addr len\r\n");
		return 0;
	}
	do_dump (ac, arg, 2);
	return 0;
}

int do_dump_long (int ac, void *arg)
{
	if (ac == -3) {
		printf ("dl addr len\r\n");
		return 0;
	}
	do_dump (ac, arg, 4);
	return 0;
}

static s_off prevaddr = 0;

static void do_dump (int ac, void *arg, int mod)
{
	arg_t *av = (arg_t *)arg;
	s_off  saddr;
	byte_t *pptr;
	s_off  p, e, p0;
	int len;
	//
	// saddr, eaddr in 0000-FFFF, refer sram[], otherwise refer STM32 address space
	//
	saddr = (ac < 1 || av[0] == BADADDR) ? prevaddr : (s_off)av[0];
	len = (ac < 2 || av[1] == BADADDR) ? 128 : (int)av[1];
	printf ("do_dump: %X, %d, %d\r\n", (s_off)saddr, len, mod);

	p = saddr;
	e = saddr + len;
	//
	// hexdump
	//
	p0 = p;
	while (p < e) {
		if (p == p0 || ((int)p & 0xf) == 0) {
			p0 = p;
			if (p < SRAM_SIZE) {
				printf ("%04X ", p);
			} else {
				printf ("%X ", p);
			}
		}
		switch (mod) {
		case 1:
			printf ("%02X ", *((uint8_t *)PHYADDR(p))); p++; break;
		case 2:
			printf ("%04X ", *((uint16_t *)PHYADDR(p))); p += 2; break;
		case 4:
			printf ("%08lX ", *((uint32_t *)PHYADDR(p))); p += 4; break;
		}
		if (p & 0xf)
			continue;
		p = p0;
		for (int i = 0; i < 16; ++i, ++p) {
			pptr = (byte_t *)PHYADDR(p);
			printf ("%c", ((' ' <= *pptr && *pptr < 0x7f) ? *pptr : '.'));
		}
		crlf ();
	}
	if (p & 0xf) {
		for (int i = 0; i < 16; ++i, ++p) {
			pptr = (byte_t *)PHYADDR(p);
			printf ("%c", ((' ' <= *pptr && *pptr < 0x7f) ? *pptr : '.'));
		}
		crlf ();
	}
	prevaddr = p;
}
