/*
 * memcmd.h
 *
 *  Created on: 2021/02/03
 *      Author: tenda_000
 */

#ifndef SRC_MEMCMD_H_
#define SRC_MEMCMD_H_

#include <inttypes.h>

int do_crc16_command (int ac, void *arg);
int do_fill_command (int ac, void *arg);
int do_set_byte (int ac, void *arg);
int do_set_word (int ac, void *arg);
int do_set_long (int ac, void *arg);
int do_dump_byte (int ac, void *arg);
int do_dump_word (int ac, void *arg);
int do_dump_long (int ac, void *arg);

#endif /* SRC_MEMCMD_H_ */
