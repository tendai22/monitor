/*
 * xspi.h
 *
 *  Created on: 2021/02/03
 *      Author: tenda_000
 */

#ifndef SRC_XSPI_H_
#define SRC_XSPI_H_

#include <inttypes.h>

//
// global function declarations
//
void spi_xfer (byte_t sdata, byte_t *rdata);
void spi_cs_reset (void);
void spi_cs_set (void);
void spi_generic_command (byte_t *sdata, int slen, byte_t *rdata, int rlen);
int do_spi_command (int ac, void *av);

#endif /* SRC_XSPI_H_ */
