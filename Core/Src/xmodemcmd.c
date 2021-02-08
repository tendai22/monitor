/*
 * xmodemcmd.c
 *
 *  Created on: 2021/02/07
 *      Author: tenda_000
 */

#include <stdio.h>
#include "monitor.h"

int xmodemReceive(unsigned char *dest, int destsz);

//
// do_xmodem
//
int do_xmodem_command (int ac, void *arg)
{
	arg_t *av = (arg_t *)arg;
	byte_t *top;
	s_off addr;
	int len;

	if (ac == -3) {
		printf ("xmodem addr len\r\n");
		return 0;
	}
	addr = (ac == 0) ? 0 : (s_off)av[0];
	top = PHYADDR(addr);
	len = (ac == 1) ? SRAM_SIZE : (int)av[1];
	if (len > SRAM_SIZE) {
		len = SRAM_SIZE;
	}
	if (addr < SRAM_SIZE) {
		printf ("download on sram[%04X], start XMODEM\r\n", addr);
	} else {
		printf ("download on %04X, start XMODEM\r\n", addr);
	}
	len = xmodemReceive ((unsigned char *)top, len);
	printf ("download %d(0x%x) bytes\r\n", len, (unsigned int)len);
	return 0;
}

#if 0
void do_xmodem (uint32_t saddr, int maxlen)
{
	int len;
	uint8_t *top;
	top = PHYADDR(saddr);
	printf ("downLoad on %04lX, start XMODEM\r\n", saddr);
	len = xmodemReceive (top, maxlen);
	printf ("download %d(0x%x) bytes\r\n", len, len);
}
#endif


#ifdef TEST_XMODEM_RECEIVE
int main(void)
{
	int st;

	printf ("Send data using the xmodem protocol from your terminal emulator now...\n");
	/* the following should be changed for your environment:
	   0x30000 is the download address,
	   65536 is the maximum size to be written at this address
	 */
	st = xmodemReceive((char *)0x30000, 65536);
	if (st < 0) {
		printf ("Xmodem receive error: status: %d\n", st);
	}
	else  {
		printf ("Xmodem successfully received %d bytes\n", st);
	}

	return 0;
}
#endif
#ifdef TEST_XMODEM_SEND
int main(void)
{
	int st;

	printf ("Prepare your terminal emulator to receive data now...\n");
	/* the following should be changed for your environment:
	   0x30000 is the download address,
	   12000 is the maximum size to be send from this address
	 */
	st = xmodemTransmit((char *)0x30000, 12000);
	if (st < 0) {
		printf ("Xmodem transmit error: status: %d\n", st);
	}
	else  {
		printf ("Xmodem successfully transmitted %d bytes\n", st);
	}

	return 0;
}
#endif
