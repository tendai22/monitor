/*
 * monitor.h
 *
 *  Created on: Oct 19, 2020
 *      Author: tenda_000
 */

#ifndef SRC_MONITOR_H_
#define SRC_MONITOR_H_

#include <limits.h>
#include <inttypes.h>

//
// pointer-in-monitor
//
// Its storage size is enough (or just as) to accommodate a generic pointer,
// If its value is within 0x0000 - 0xffff, it represents an offset in sram[].
//
// address within 0000 - FFFF regards as in sram[offset]
//

// a type, compatible with 'char' as well as it it used in 'sram[]'
// Strict signedness discrimination is needed where in alithmetic operations,
// including sumcheck, crc16 calucuration.
typedef char byte_t;

// generil pointer/integer type,
typedef byte_t *arg_t;

// sram address/offset
typedef unsigned int s_off;

// SRAM area
extern byte_t *sram;
#define SRAM_SIZE 0x10000

#ifdef LINUX
//#define BADADDR ULONG_MAX
//#define PHYADDR(p) (sram + p)
#else
extern byte_t __not_used;
#define BADADDR ((arg_t)0xffffffff)
#define PHYADDR(p) ((byte_t *)((s_off)(p) < SRAM_SIZE ? (sram + (s_off)(p)) : (byte_t *)(p)))
#endif

// serial flash address type
// usually 24bit-width (less than 16MB), or some more for extended address
//
typedef int32_t faddr_t;
typedef int32_t foff_t;

//
// target CPU execution context
//
struct codeinfo {
	byte_t *code;
	uint16_t start;
	int length;
};

extern struct codeinfo codes[];

//
// command table and its entry
//
struct cmd_entry {
	char *format;
	int (*func)(int ac, void *av);
};

//
// global function declarations
//
int monitor (byte_t *sram, struct codeinfo *codes);
int getch (void);
int ungetch (byte_t c);
int getch_timeout (int millisec);
void crlf (void);
int readline (char *buf, int len);
int strnccmp (const char *a, const char *b);
int do_command (const char *cmdstr, const struct cmd_entry *cmdtable, int ncmds, struct codeinfo *codes);

#endif /* SRC_MONITOR_H_ */
