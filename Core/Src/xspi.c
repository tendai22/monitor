/*
 * xspi.c
 *
 *  Created on: 2021/02/03
 *      Author: tenda_000
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <limits.h>
#include "main.h"

#include "monitor.h"
#include "xspi.h"

//
// low level spi primitives
//
void spi_xfer (byte_t sdata, byte_t *rdata)
{
	byte_t data;
	while ((SPI2->SR & SPI_SR_TXE) == 0)
		;
	SPI2->DR = sdata;
	while ((SPI2->SR & SPI_SR_RXNE) == 0)
		;
	data = (byte_t)(SPI2->DR);
	if (rdata) {
		*rdata = data;
	}
}

//
// /CS on/off primitives
//
void spi_cs_reset (void)
{
	SPI2->CR1 |= SPI_CR1_SPE;
	GPIOB->ODR &= ~CS_Pin;
}

void spi_cs_set (void)
{
	while ((SPI2->SR & SPI_SR_RXNE) == 1)
		;
	while ((SPI2->SR & SPI_SR_TXE) == 1)
		;
	while ((SPI2->SR & SPI_SR_BSY) == 1)
		;
	SPI2->CR1 &= ~SPI_CR1_SPE;
	GPIOB->ODR |= CS_Pin;
}

//
// generic spi command
//
void spi_generic_command (byte_t *sdata, int slen, byte_t *rdata, int rlen)
{
	spi_cs_reset ();
	for (int i = 0; i < slen; ++i) {
		spi_xfer (sdata[i], 0);
	}
	for (int i = 0; rdata && i < rlen; ++i) {
		spi_xfer (0, &rdata[i]);
	}
}

//
// monitor command
//
int do_spi_command (int ac, void *arg)
{
	int i;
	uint32_t *av = (uint32_t *)arg;
	byte_t buf[256];
	long rlen = 0, slen = 0;
	if (ac == 0) {
		return -1;
	}
	if (ac == -3) {
		printf ("spi cc cc... len\r\n");
		return 0;
	}
	// fill send data
	slen = ac - 1;
	for (i = 0; i < slen; ++i) {
		buf[i] = av[i];
	}
	rlen = av[ac - 1];
	printf ("spi: slen = %d, rlen = %d\r\n", (int)slen, (int)rlen);
	// read back?
	if (slen == 0) {
		printf ("no valid send data is specified, abort\r\n");
		return -1;
	}
	if (rlen >= 256) {
		printf ("rlen too long\r\n");
		return -2;
	}
	// do it!
	printf ("Send: ");
	for (i = 0; i < slen; ++i) {
		printf (" %02X", buf[i]);
	}
	printf ("\r\n");
	// perform a sequence
	spi_cs_reset ();
	for (i = 0; i < slen; ++i) {
		spi_xfer (buf[i], 0);
	}
	if (rlen > 0) {
		for (i = 0; i < rlen; ++i) {
			spi_xfer (0, &buf[i]);
		}
	}
	spi_cs_set ();
	// result dump
	if (rlen > 0) {
		printf ("Receive: ");
		if (rlen >= 8) {
			printf ("\r\n");
		}
		for (i = 0; i < rlen; ++i) {
			if (rlen >= 8 && (i % 16) == 0) {
				printf ("%02X:", i);
			}
			printf (" %02X", buf[i]);
			if ((i % 16) == 15) {
				printf ("\r\n");
			}
		}
		if ((i % 16) != 0) {
			printf ("\r\n");
		}
	}
	return 0;
}
