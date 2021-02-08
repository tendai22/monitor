/*
 * monitor.c
 *
 *  Created on: 2020/11/18
 *      Author: tenda_000
 */

#include <stdio.h>
#include <inttypes.h>
#include <stdarg.h>
#include <stdlib.h>
#include <limits.h>
#include <ctype.h>
#include <string.h>

//#include "z80.h"
//#include "z80onSTM32.h"
#include "monitor.h"
//#include "F8onSTM32.h"
#include "spi.h"
#include "flash.h"
#include "memcmd.h"
#include "intelhex.h"


//
// external function declarations
//
//int xmodemReceive (uint8_t *dest, int destsz);
unsigned short crc16_ccitt(const void *buf, int len);
void do_xmodem (uint32_t saddr, int maxlen);
int getchar_pol (void);
int _inbyte(int timeout); // msec timeout
//int do_hex_format (byte_t *sram, int endram, uint32_t *beginp, uint32_t *endp);

int do_xmodem_command (int ac, void *arg);

//int spi_xfer (uint8_t sdata, uint8_t *rdata);
//void spi_cs_set (void);
//void spi_cs_reset (void);

extern byte_t *sram;
//
// intel hex format reader
//
int unget_buf = -1;

int getch (void)
{
	int c;
	if (unget_buf >= 0) {
		c = unget_buf;
		unget_buf = -1;
		return c;
	}
	c = getchar ();
	return c;
}

int getch_timeout (int millis)
{
	int c;
	if (unget_buf >= 0) {
		c = unget_buf;
		unget_buf = -1;
		return c;
	}
	c = _inbyte (millis);
	return c;
}

int ungetch (byte_t c)
{
  unget_buf = c;
  return c;
}

void crlf (void)
{
	printf ("\r\n");
}

#if 0
static int readhex (void)
{
  int c, d;
  d = 0;
  if ((c = c2h (getch())) < 0)
    return -1;
  d = c << 4;
  if ((c = c2h (getch())) < 0)
    return -1;
  d += c;
  //printf ("readhex: %02x\r\n", d);
  return d;
}

static uint16_t hex_start = 0xffff;
static uint16_t hex_end = 0;
#endif

int readline (char *buf, int len)
{
	char *p = buf;
	int c;
	while (p < (buf + len - 1) && (c = getch ()) != '\r' && c != 'n') {
		if (c == 0x08 || c == 0x7f) {
			if (p > buf) {
				printf ("\x08 \x08");
				--p;
			}
		} else {
			putchar (c);
			*p++ = c;
		}
	}
	*p = '\0';
	return p - buf;
}

#if 0
/*
 * dump ... hex dump in sram buffer
 */
static void dump (const uint8_t *sram, uint16_t start, uint16_t end)
{
	int i = 0;
	uint16_t ptr = start;
	int flag = 0;
	for (i = 0; i < 16; ++i)
		if (sram[ptr + i])
			flag = 1;
	if (flag == 0)
		return;
	printf ("%04X ", start);
	for (i = 0; ptr < end && i < 16 ; ++i, ++ptr)
		printf ("%02X ", sram[ptr]);
	while (i++ < 16)
		printf ("   ");
	printf (" ");
	ptr = start;
	for (i = 0; ptr < end && i < 16 ; ++i, ++ptr)
		printf ("%c", isprint(sram[ptr]) ? sram[ptr] : '.');
	printf ("\r\n");
}
#endif

#if 0
int parse_args (uint8_t *line, int *argc, uint8_t **argv, int maxargs)
{
	//printf ("parse_args: %s\r\n", line);
	int nargs;
	if (argc == 0)
		return 0;
	if (argv == 0) {
		*argc = 0;
		return 0;
	}
	if (line == 0 || *line == 0) {
		*argc = 0;
		return 0;
	}
	uint8_t *p = line, *p0;
	nargs = 0;
	while (*p && nargs < maxargs - 1) {
		while (*p && isspace (*p))
			p++;
		if (*p == '\0') {
			break;
		}
		p0 = p;
		while (*p && !isspace (*p))
			p++;
		if (*p)
			*p++ = '\0';
		argv[nargs] = p0;
		//printf ("argv[%d] %s\r\n", nargs, argv[nargs]);
		nargs++;
	}
	if (nargs < maxargs)
		argv[nargs] = NULL;
	*argc = nargs;
	//printf ("argc = %d\r\n", *argc);
	//for (int i = 0; i < *argc; ++i) {
	//	printf ("<%s>", argv[i]);
	//}
	//printf ("\r\n");
	return nargs;
}
#endif

static void strtoupper (char *dest, const char *src, int len)
{
	char *end = dest + len - 1;
	for (; *src; src++, dest++) {
		if (dest >= end) {
			*end = '\0';
			return;
		}
		*dest = toupper (*src);
	}
	*dest = '\0';
	return;
}

int strnccmp (const char *s1, const char *s2)
{
	char buf1[80], buf2[80];
	strtoupper (buf1, s1, sizeof buf1);
	strtoupper (buf2, s2, sizeof buf2);
	return strcmp ((const char *)buf1, (const char *)buf2);
}

/*
 * monitor commands
 * D
 * D <src>
 * D <src> <end>
 * dump memory, from <s> to <e>, or 128bytes
 *
 * S
 * S <addr>
 * set a byte to memory, a period('.') stops to input.
 *
 * G
 * G <addr>
 * jump to <addr>
 *
 * L ... load code segment
 * L <num>
 * load code segment, if no argument, last number of the code is selected
 */


static int modifier = 1;	// 1: byte, 2: short, 4: long
int get_modifier (const uint8_t *s)
{
	if (*s == '.')
		s++;
	switch (toupper(*s)) {
	case 'B': modifier = 1; return modifier;
	case 'W': modifier = 2; return modifier;
	case 'L': modifier = 4; return modifier;
	default: return modifier;
	}
}

#if 0
static void print_cmd (uint8_t cmd, uint8_t *start, int len)
{
	printf ("%02x:", cmd);
	while (len-- > 0) {
		printf (" %02X", *start++);
	}
	printf ("\r\n");
}
#endif

//
// do command
//
#include "xspi.h"
#include "memcmd.h"
#include "embedflash.h"
#include "flash.h"
//#include "xmodem.h"

//
// cmd table
//

//static uint32_t nextaddr = 0;
//static uint32_t prevaddr = 0;

uint32_t argtoaddr (uint8_t *s, uint32_t alteraddr)
{
	uint32_t addr;
	if ((addr = strtoul ((char *)s, NULL, 16)) == (uint32_t)BADADDR)
		addr = alteraddr;
	return addr;
}

#if 0
int do_command (const char *cmdstr, struct codeinfo *codes)
{
#define LINESIZE 80
	char buf[LINESIZE];
	uint32_t arg0, arg1, arg2;
	int c, m, n, len;
	char *p, *q;
	static int last_index = 0;
	struct cmdentry *ep;

	//
	// parse imput line
	//
	strlcpy (buf, cmdstr, LINESIZE);
	len = strlen (buf);
	if ((p = strchr (buf, ' ')) != 0) {
		*p = '\0';
	}
	n = strlen (buf);	// length of cmd string
	m = sizeof cmdtable / sizeof (struct cmdentry);
	for (int i = 0, ep = &cmdtable[0]; i < m; i++, ep++) {
		if (ep=>format == 0 || strncmp (buf, ep->format, n) == 0) {
			break;
		}
	}
	if (ep->format == 0) {
		printf ("cmd not found: %s", buf);
		return -1;
	}
	// found it, so parse it
	if (n == len) {		// no arguments
		arg1 = arg2 = arg3 = BADADDR;
		res = ep->func (arg1, arg2, arg3);
		return res;
	}
	// one or more arguments, parse it;
	p = &buf[n+1];
	q = &(ep->format[n+1]);
	n = sscanf (p, q, &arg1, &arg2, &arg3);

	// multicharacter commands
	if (strnccmp (av[0], (uint8_t *)"xmodem") == 0) {
		if (ac == 0) {
			saddr = prevaddr;
		}
		if (ac > 1) {
			saddr = argtoaddr (av[1], nextaddr);
		}
		if (saddr == BADADDR) {
			printf ("bad addr %s\r\n", av[1]);
			return -1;
		}
		do_xmodem (saddr, 0x10000);
		return 0;
	}
	if (strnccmp (av[0], (uint8_t *)"spi") == 0) {
		// SPI command
		do_spi_command (ac, av);
		return 0;
	}
	if (strnccmp (av[0], (uint8_t *)"flash") == 0) {
		// FLASH command
		do_flash_command (ac, av);
		return 0;
	}
	if (strnccmp (av[0], (uint8_t *)"embed") == 0) {
		// FLASH command
		do_embed_command (ac, av);
		return 0;
	}
	if (strnccmp (av[0], (uint8_t *)"crc") == 0) {
		// crc/checksum command
		if (ac < 3) {
			printf ("crc saddr len\r\n");
			return -1;
		}
		saddr = argtoaddr (av[1], BADADDR);
		len = strtol ((char *)av[2], 0, 10);
		if (saddr == BADADDR || len == BADADDR) {
			printf ("bad arg\r\n");
			return -1;
		}
		do_crc16 (saddr, len);
		return 0;
	}
	if (strnccmp (av[0], (uint8_t *)"fill") == 0) {
		// crc/checksum command
		if (ac < 4) {
			printf ("fill c saddr len\r\n");
			return -1;
		}
		c = strtoul ((char *)av[1], 0, 16);
		saddr = argtoaddr (av[2], BADADDR);
		len = strtol ((char *)av[3], 0, 16);
		if (saddr == BADADDR || len == BADADDR) {
			printf ("bad arg\r\n");
			return -1;
		}
		do_fill (c, saddr, len);
		return 0;
	}
	// singlecharacter commands
	switch (toupper (c)) {
	case 'D':	// dump data
		mod = get_modifier (p + 1);
		if (ac == 1) {
			saddr = nextaddr;
		}
		if (ac > 1) {
			if ((saddr = strtoul ((char *)av[1], NULL, 16)) == BADADDR)
				saddr = nextaddr;
		}
		if (saddr == BADADDR) {
			printf ("bad addr %s\r\n", av[1]);
			return -1;
		}
		// round saddr
		saddr = saddr / mod * mod;
		// eaddr
		eaddr = ac > 2 ? strtoul ((char *)av[2], NULL, 16) : BADADDR;
		if (eaddr == BADADDR)
			eaddr = 128;
		nextaddr = saddr + eaddr;
		do_dump (saddr, saddr + eaddr, mod);
		break;
	case 'E':		// set data
		mod = get_modifier (p + 1);
		if (ac == 1) {
			saddr = nextaddr;
		}
		if (ac > 1) {
			if ((saddr = strtoul ((char *)av[1], NULL, 16)) == BADADDR)
				saddr = nextaddr;
		}
		if (saddr == BADADDR) {
			printf ("bad addr %s\r\n", av[1]);
			return -1;
		}
		nextaddr = do_setcmd (saddr, mod);
		printf ("\r\n");
		break;
	case 'L':		// load data
		if (ac == 1) {
			index = last_index;
		}
		if (ac > 1) {
			if ((index = strtoul ((char *)av[1], NULL, 16)) == BADADDR) {
				index = last_index;
			}
		}
		int i, max_index;
		struct codeinfo *cp = codes;
		if (codes == 0) {
			printf ("no code specified, not set\r\n");
			return -1;
		}
		for (i = 0; cp && cp->code ; ++i, ++cp)
			;
		max_index = i;
		if (!(0 <= index && index < max_index)) {
			printf ("bad index %s\r\n", av[1]);
			return -1;
		}
		// copy code
		cp = &codes[index];
		uint8_t *src, *dst, *dend;
		src = cp->code;
		dst = &sram[cp->start];
		dend = dst + cp->length;
		if (dend >= sram + 0x10000) {
			dend = sram + 0x10000;
		}
		while (dst < dend) {
			*dst++ = *src++;
		}
		break;
	}
	return 0;
}
#endif

//=========================================
#ifdef LINUX
int do_exit (int ac, uint32_t a1, uint32_t a2, uint32_t a3)
{
	printf ("exit the loop\r\n");
	return -1;
}
#endif
//
// parse args
//
#define MAXARGS 20
#define LINESIZE 80

int strxcmp (const char *p, const char *pat)
{
	return strncmp (p, pat, strlen (pat));
}

static int parse_args (char *src, char *pat, void *av, int maxargs)
{
	char sbuf[LINESIZE], pbuf[LINESIZE];
	char *s, *p, *ss, *pp, *se, *pe;
	int i = 0, valid_num = 0;
	uint32_t val;

	strlcpy (sbuf, src, LINESIZE);
	strlcpy (pbuf, pat, LINESIZE);
	s = sbuf; p = pbuf;
	printf ("parse_args: %s/%s\r\n", s, p);
	se = s + strlen (s);
	pe = p + strlen (p);
	for (i = 0; i < maxargs && p < pe && s < se; ++i) {
		if ((ss = strchr (s, ' ')) != NULL) {
			*ss = '\0';
		}
		if ((pp = strchr (p, ' ')) != NULL) {
			*pp = '\0';
		}
		printf ("%s %s\r\n", s, p);
		if (strxcmp (p, "%ARGS") == 0) {
			// indetermined number of args
			while (i < maxargs && s < se && p < pe) {
				if ((ss = strchr (s, ' ')) != NULL) {
					*ss = '\0';
				}
				if (sscanf (s, "%x", (unsigned int *)&val) == 1) {
					*((uint32_t *)av + i) = val;
					valid_num++;
					++i;
				}
				s += strlen (s) + 1;
			}
		} else if (strxcmp (p, "%s") == 0) {
			return -2;	// parse subcommand in ep->func
		} else if (sscanf (s, p, &val) == 1) {
			*((uint32_t *)av + i) = val;
			s += strlen (s) + 1;
			valid_num++;
		} else {
			s += strlen (s) + 1;
		}
		p += strlen (p) + 1;
	}
	return valid_num;
}

//
// cmd table
//

static struct cmd_entry cmdtable[] = {
#ifdef LINUX
	{ "exit", do_exit },
#endif
	{ "d %X %X", 	do_dump_byte },
	{ "dw %X %X", 	do_dump_word },
	{ "dl %X %X",	do_dump_long },
	{ "s %X", 		do_set_byte },
	{ "sw %X",		do_set_word },
	{ "sl %X",		do_set_long },
	{ "xmodem %X %X",	do_xmodem_command },
	{ "spi %ARGS",	do_spi_command },
	{ "flash %s",	do_flash_command },
	{ "embed %s",	do_embed_command },
	{ "crc %X %d",	do_crc16_command },
	{ "fill %X %X %X",	do_fill_command },
	{ 0, 0 }
};

#define NCMDS(table) (sizeof table / sizeof (struct cmd_entry))

int do_command (const char *cmdstr, const struct cmd_entry *cmdtable, int ncmds, struct codeinfo *codes)
{
	char buf[LINESIZE];
	arg_t av[MAXARGS];
	int m, n, num, len, res, i;
	char *p, *q;
	const struct cmd_entry *ep;

	printf ("do_command: %s\r\n", cmdstr);
	//
	// parse imput line
	//
	if (*cmdstr == '\0') {
		return 0;			// null string, no command execution
	}
	strlcpy (buf, cmdstr, LINESIZE);
	len = strlen (buf);
	if ((p = strchr (buf, ' ')) != 0) {
		*p = '\0';
	}
	n = strlen (buf);	// length of cmd string
	m = ncmds;
	// help command
	if (strxcmp (buf, "?") == 0) {
		// print command list
		for (ep = cmdtable; ep->format != 0; ++ep) {
			ep->func (-3, NULL);
		}
		return 0;
	}
	for (i = 0, ep = cmdtable; i < m; i++, ep++) {
		if (ep->format == 0 || strncmp (buf, ep->format, n) == 0) {
			break;
		}
		// subcommand?
		if ((p = strstr (ep->format, "%s")) == 0) {
			ep->func (-3, ep->format);
		}
	}
	if (ep->format == 0) {
		printf ("cmd not found: %s\r\n", buf);
		return -1;
	}
	// found it, so parse it
	for (i = 0; i < MAXARGS; ++i) {
		av[i] = BADADDR;
	}
	if (n == len) {		// no arguments
		res = ep->func (0, &av[0]);
		return res;
	}
	// one or more arguments, parse it;
	p = &buf[n+1];
	q = &(ep->format[n+1]);
	if ((num = parse_args (p, q, av, MAXARGS)) == -2) {
		// subcommand parse and go in ep->func.
		printf ("p = %s\r\n", p);
		return ep->func (-2, (void *)&cmdstr[n+1]);
	} else if (num >= 0) {
		return ep->func (num, av);
	} else {
		printf ("bad args\r\n");
		return -1;
	}
}

//=========================================

/*
 * monitor ... read chars to parse and execute corresponting command.
 */
int monitor (byte_t *sram_arg, struct codeinfo *codes)
{
	static char linbuf[80];
	//int continue_flag = 0;
	int c;
	int run_mode = 0;
	//struct f8_context context;
	sram = sram_arg;
	printf ("DIP40 on STM32 monitor\r\n");
	do {
		if (run_mode == 0) {
			printf ("> ");
		}
		c = getchar ();
		if (c != ' ') {
			if (run_mode != 0) {
				printf ("> ");
			}
			run_mode = 0;
		}
		if (c == 'i') {
			putchar (c);
			// generate INT (i8251 RxRdy emulation)
			//generate_i8251int ();		// GPIOA->ODR |= INT_Pin;
		} else if (c == 'g' || c == ' ') {
			if (run_mode == 0) {
				putchar (c);
				printf ("\r\n");
				run_mode = 1;
			}
			// start CPU, nonstop mode
			// SPC ... singlestep, g ... run (nonstop)
			while (getchar_pol() > 0);	// read out unread chars
			//runF3850_EXTI ((c == ' ') ? 1 : 0, continue_flag, &context);
			//continue_flag = 1;
		} else if (c == ':') {
			// intel hex format read
			uint32_t total;
			uint32_t begin = ULONG_MAX, end = 0;
			uint16_t crc_val;
			uint32_t csum_val;
			int i;
			int n;
			ungetch (c);
			total = 0;
			if ((n = do_hex_format (sram, 0x10000 - 1, &begin, &end)) > 0) {
				total += n;
				printf ("begin: %04lX, end: %04lX, len = %ld\r\n", begin, end, total);
				crc_val = crc16_ccitt (PHYADDR(begin), total);
				csum_val = 0;
				for (i = begin; i < end; ++i) {
					csum_val += sram[i];
				}
				printf ("crc16; %X, csum: %X\r\n", (unsigned int)crc_val, (unsigned int)csum_val);
			}
		} else {
			// gather chars in a line
			ungetch (c);
			readline (linbuf, sizeof linbuf);
			printf ("\r\n");
			printf ("lin: %s\r\n", linbuf);
			//parse_args (linbuf, &ac, av, 256);
			do_command (linbuf, cmdtable, NCMDS(cmdtable), codes);
		}
	} while (1);
	return c;
}

#ifdef TEST_MONITOR
int main (int ac, char **av)
{
	static uint8_t sram[65536];
	setbuf (stdout, NULL);
	setbuf (stdin, NULL);
	system ("stty raw -echo");
	monitor(sram);
	system ("stty sane");
}
#endif


