/*
 * disasm.c
 *
 *  Created on: 2021/01/15
 *      Author: tenda_000
 */

#include <stdio.h>
#include <inttypes.h>

#include "disasm.h"

//
// instruction table entry
// mnuemonic
// type:
//   0: 1byte instruction
//   1: 2byte instruction, one for 1byte operand
//   2: 3byte instruction, two byte operand, big endian order
//   3: 1byte instruction, relative branch offset
//
struct pair {
	uint8_t *mnemonic;
	uint8_t type;
};

static struct pair f8inst[];

//uint8_t sram[64];

//
// display one instruction and operand in a line
// @param: sram ... F8 memory array
// @param: addr ... an index on which the instruction starts.
//
int disasm (uint8_t *sram, uint16_t addr) {
	uint8_t op = sram[addr];
	const uint8_t *mn = f8inst[op].mnemonic;
	int len = 1;
	int operand;
	printf ("%04X %02X ", addr, op);
	switch (f8inst[op].type) {
	case 0:		// single address
		printf ("      %s\r\n", mn);
		len = 1;
		break;
	case 1:		// 1byte operand
		printf ("%02X    %s %02X\r\n", sram[addr+1], mn, sram[addr+1]);
		len = 2;
		break;
	case 2:		// 2byte operand
		operand = (uint16_t)sram[addr+1]*256 + sram[addr+2];
		printf ("%02X %02X %s %04X\r\n", (operand>>8), (operand&0xff), mn, operand);
		len = 3;
		break;
	case 4:
	case 3:		// 1 byte branch address
		operand = addr + 1 + (int8_t)sram[addr+1];
		printf ("%02X    %s %04X\r\n", sram[addr+1], mn, operand);
		len = 2;
		break;
	}
	return len;
}

#if 0
int main (int ac, char **av)
{
	int byte, i;
	++av;
	for (i = 0; *av && i < 64; ++i, ++av) {
		if (sscanf (*av, "%x", &byte) == 1) {
			sram[i] = byte;
		}
	}
	int n = i;
	for (int i = 0; i < n; ) {
		printf ("%04X %02X ", i, sram[i]);
		i += disasm (i);
	}
}
#endif

//
// F8 instraction table
//
static struct pair f8inst[] = {
 { (uint8_t *)"LR  A,KU", 0 },
 { (uint8_t *)"LR  A,KL", 0 },
 { (uint8_t *)"LR  A,QU", 0 },
 { (uint8_t *)"LR  A,UL", 0 },
 { (uint8_t *)"LR  KU,A", 0 },
 { (uint8_t *)"LR  KL,A", 0 },
 { (uint8_t *)"LR  QU,A", 0 },
 { (uint8_t *)"LR  QL,A", 0 },
 { (uint8_t *)"LR  K,P", 0 },
 { (uint8_t *)"LR  P,K", 0 },
 { (uint8_t *)"LR  A,IS", 0 },
 { (uint8_t *)"LR  IS,A", 0 },
 { (uint8_t *)"PK  ", 0 },
 { (uint8_t *)"LR  P0,Q", 0 },
 { (uint8_t *)"LR  Q,DC", 0 },
 { (uint8_t *)"LR  DC,Q", 0 },
 { (uint8_t *)"LR  DC,H", 0 },
 { (uint8_t *)"LR H,DC", 0 },
 { (uint8_t *)"SR  1", 0 },
 { (uint8_t *)"SL  1", 0 },
 { (uint8_t *)"SR  4", 0 },
 { (uint8_t *)"SL  4", 0 },
 { (uint8_t *)"LM  ", 0 },
 { (uint8_t *)"ST  ", 0 },
 { (uint8_t *)"COM", 0 },
 { (uint8_t *)"LNK", 0 },
 { (uint8_t *)"DI", 0 },
 { (uint8_t *)"EI", 0 },
 { (uint8_t *)"POP", 0 },
 { (uint8_t *)"LR  W,J", 0 },
 { (uint8_t *)"LR  J,W", 0 },
 { (uint8_t *)"INC", 0 },
 { (uint8_t *)"LI  ", 1 },
 { (uint8_t *)"NI  ", 1 },
 { (uint8_t *)"OI  ", 1 },
 { (uint8_t *)"XI  ", 1 },
 { (uint8_t *)"AI  ", 1 },
 { (uint8_t *)"CI  ", 1 },
 { (uint8_t *)"IN  ", 1 },
 { (uint8_t *)"OUT ", 1 },
 { (uint8_t *)"PI  ", 2 },
 { (uint8_t *)"JMP ", 2 },
 { (uint8_t *)"DCI ", 2 },
 { (uint8_t *)"NOP", 0 },
 { (uint8_t *)"XDC", 0 },
 { (uint8_t *)"undef", 0 },
 { (uint8_t *)"undef", 0 },
 { (uint8_t *)"undef", 0 },
 { (uint8_t *)"DS  r0", 0 },
 { (uint8_t *)"DS  r1", 0 },
 { (uint8_t *)"DS  r2", 0 },
 { (uint8_t *)"DS  r3", 0 },
 { (uint8_t *)"DS  r4", 0 },
 { (uint8_t *)"DS  r5", 0 },
 { (uint8_t *)"DS  r6", 0 },
 { (uint8_t *)"DS  r7", 0 },
 { (uint8_t *)"DS  r8", 0 },
 { (uint8_t *)"DS  r9", 0 },
 { (uint8_t *)"DS  r10", 0 },
 { (uint8_t *)"DS  r11", 0 },
 { (uint8_t *)"DS  r12", 0 },
 { (uint8_t *)"DS  r13", 0 },
 { (uint8_t *)"DS  r14", 0 },
 { (uint8_t *)"DS  r15", 0 },
 { (uint8_t *)"LR  A,r0", 0 },
 { (uint8_t *)"LR  A,r1", 0 },
 { (uint8_t *)"LR  A,r2", 0 },
 { (uint8_t *)"LR  A,r3", 0 },
 { (uint8_t *)"LR  A,r4", 0 },
 { (uint8_t *)"LR  A,r5", 0 },
 { (uint8_t *)"LR  A,r6", 0 },
 { (uint8_t *)"LR  A,r7", 0 },
 { (uint8_t *)"LR  A,r8", 0 },
 { (uint8_t *)"LR  A,r9", 0 },
 { (uint8_t *)"LR  A,r10", 0 },
 { (uint8_t *)"LR  A,r11", 0 },
 { (uint8_t *)"LR  A,r12", 0 },
 { (uint8_t *)"LR  A,r13", 0 },
 { (uint8_t *)"LR  A,r14", 0 },
 { (uint8_t *)"LR  A,r15", 0 },
 { (uint8_t *)"LR  r0,A", 0 },
 { (uint8_t *)"LR  r1,A", 0 },
 { (uint8_t *)"LR  r2,A", 0 },
 { (uint8_t *)"LR  r3,A", 0 },
 { (uint8_t *)"LR  r4,A", 0 },
 { (uint8_t *)"LR  r5,A", 0 },
 { (uint8_t *)"LR  r6,A", 0 },
 { (uint8_t *)"LR  r7,A", 0 },
 { (uint8_t *)"LR  r8,A", 0 },
 { (uint8_t *)"LR  r9,A", 0 },
 { (uint8_t *)"LR  r10,A", 0 },
 { (uint8_t *)"LR  r11,A", 0 },
 { (uint8_t *)"LR  r12,A", 0 },
 { (uint8_t *)"LR  r13,A", 0 },
 { (uint8_t *)"LR  r14,A", 0 },
 { (uint8_t *)"LR  r15,A", 0 },
 { (uint8_t *)"LISU 0", 0 },
 { (uint8_t *)"LISU 1", 0 },
 { (uint8_t *)"LISU 2", 0 },
 { (uint8_t *)"LISU 3", 0 },
 { (uint8_t *)"LISU 4", 0 },
 { (uint8_t *)"LISU 5", 0 },
 { (uint8_t *)"LISU 6", 0 },
 { (uint8_t *)"LISU 7", 0 },
 { (uint8_t *)"LISL 0", 0 },
 { (uint8_t *)"LISL 1", 0 },
 { (uint8_t *)"LISL 2", 0 },
 { (uint8_t *)"LISL 3", 0 },
 { (uint8_t *)"LISL 4", 0 },
 { (uint8_t *)"LISL 5", 0 },
 { (uint8_t *)"LISL 6", 0 },
 { (uint8_t *)"LISL 7", 0 },
 { (uint8_t *)"LIS  $0", 0 },
 { (uint8_t *)"LIS  $1", 0 },
 { (uint8_t *)"LIS  $2", 0 },
 { (uint8_t *)"LIS  $3", 0 },
 { (uint8_t *)"LIS  $4", 0 },
 { (uint8_t *)"LIS  $5", 0 },
 { (uint8_t *)"LIS  $6", 0 },
 { (uint8_t *)"LIS  $7", 0 },
 { (uint8_t *)"LIS  $8", 0 },
 { (uint8_t *)"LIS  $9", 0 },
 { (uint8_t *)"LIS  $a", 0 },
 { (uint8_t *)"LIS  $b", 0 },
 { (uint8_t *)"LIS  $c", 0 },
 { (uint8_t *)"LIS  $d", 0 },
 { (uint8_t *)"LIS  $e", 0 },
 { (uint8_t *)"LIS  $f", 0 },
 { (uint8_t *)"NOP3 ", 3 },
 { (uint8_t *)"BP  ", 3 },
 { (uint8_t *)"BC  ", 3 },
 { (uint8_t *)"BF3 ", 3 },
 { (uint8_t *)"BZ  ", 3 },
 { (uint8_t *)"BP  ", 3 },
 { (uint8_t *)"BF6 ", 3 },
 { (uint8_t *)"BF7 ", 3 },
 { (uint8_t *)"AM  ", 0 },
 { (uint8_t *)"AMD ", 0 },
 { (uint8_t *)"NM  ", 0 },
 { (uint8_t *)"OM  ", 0 },
 { (uint8_t *)"XM  ", 0 },
 { (uint8_t *)"CM  ", 0 },
 { (uint8_t *)"ADC ", 0 },
 { (uint8_t *)"BR7 ", 4 },
 { (uint8_t *)"B   ", 4 },
 { (uint8_t *)"BM  ", 4 },
 { (uint8_t *)"BNC ", 4 },
 { (uint8_t *)"BF3 ", 4 },
 { (uint8_t *)"BNZ ", 4 },
 { (uint8_t *)"BP  ", 4 },
 { (uint8_t *)"BF6 ", 4 },
 { (uint8_t *)"BF7 ", 4 },
 { (uint8_t *)"BNO ", 4 },
 { (uint8_t *)"BF9 ", 4 },
 { (uint8_t *)"BFA ", 4 },
 { (uint8_t *)"BFB ", 4 },
 { (uint8_t *)"BFC ", 4 },
 { (uint8_t *)"BFD ", 4 },
 { (uint8_t *)"BFE ", 4 },
 { (uint8_t *)"BFF ", 4 },
 { (uint8_t *)"INS $0", 0 },
 { (uint8_t *)"INS $1", 0 },
 { (uint8_t *)"INS $2", 0 },
 { (uint8_t *)"INS $3", 0 },
 { (uint8_t *)"INS $4", 0 },
 { (uint8_t *)"INS $5", 0 },
 { (uint8_t *)"INS $6", 0 },
 { (uint8_t *)"INS $7", 0 },
 { (uint8_t *)"INS $8", 0 },
 { (uint8_t *)"INS $9", 0 },
 { (uint8_t *)"INS $A", 0 },
 { (uint8_t *)"INS $B", 0 },
 { (uint8_t *)"INS $C", 0 },
 { (uint8_t *)"INS $D", 0 },
 { (uint8_t *)"INS $E", 0 },
 { (uint8_t *)"INS $F", 0 },
 { (uint8_t *)"OUTS $0", 0 },
 { (uint8_t *)"OUTS $1", 0 },
 { (uint8_t *)"OUTS $2", 0 },
 { (uint8_t *)"OUTS $3", 0 },
 { (uint8_t *)"OUTS $4", 0 },
 { (uint8_t *)"OUTS $5", 0 },
 { (uint8_t *)"OUTS $6", 0 },
 { (uint8_t *)"OUTS $7", 0 },
 { (uint8_t *)"OUTS $8", 0 },
 { (uint8_t *)"OUTS $9", 0 },
 { (uint8_t *)"OUTS $A", 0 },
 { (uint8_t *)"OUTS $B", 0 },
 { (uint8_t *)"OUTS $C", 0 },
 { (uint8_t *)"OUTS $D", 0 },
 { (uint8_t *)"OUTS $E", 0 },
 { (uint8_t *)"OUTS $F", 0 },
 { (uint8_t *)"AS  0", 0 },
 { (uint8_t *)"AS  1", 0 },
 { (uint8_t *)"AS  2", 0 },
 { (uint8_t *)"AS  3", 0 },
 { (uint8_t *)"AS  4", 0 },
 { (uint8_t *)"AS  5", 0 },
 { (uint8_t *)"AS  6", 0 },
 { (uint8_t *)"AS  7", 0 },
 { (uint8_t *)"AS  8", 0 },
 { (uint8_t *)"AS  9", 0 },
 { (uint8_t *)"AS  A", 0 },
 { (uint8_t *)"AS  B", 0 },
 { (uint8_t *)"AS  C", 0 },
 { (uint8_t *)"AS  D", 0 },
 { (uint8_t *)"AS  E", 0 },
 { (uint8_t *)"AS  F", 0 },
 { (uint8_t *)"ASD 0", 0 },
 { (uint8_t *)"ASD 1", 0 },
 { (uint8_t *)"ASD 2", 0 },
 { (uint8_t *)"ASD 3", 0 },
 { (uint8_t *)"ASD 4", 0 },
 { (uint8_t *)"ASD 5", 0 },
 { (uint8_t *)"ASD 6", 0 },
 { (uint8_t *)"ASD 7", 0 },
 { (uint8_t *)"ASD 8", 0 },
 { (uint8_t *)"ASD 9", 0 },
 { (uint8_t *)"ASD A", 0 },
 { (uint8_t *)"ASD B", 0 },
 { (uint8_t *)"ASD C", 0 },
 { (uint8_t *)"ASD D", 0 },
 { (uint8_t *)"ASD E", 0 },
 { (uint8_t *)"ASD F", 0 },
 { (uint8_t *)"XS  0", 0 },
 { (uint8_t *)"XS  1", 0 },
 { (uint8_t *)"XS  2", 0 },
 { (uint8_t *)"XS  3", 0 },
 { (uint8_t *)"XS  4", 0 },
 { (uint8_t *)"XS  5", 0 },
 { (uint8_t *)"XS  6", 0 },
 { (uint8_t *)"XS  7", 0 },
 { (uint8_t *)"XS  8", 0 },
 { (uint8_t *)"XS  9", 0 },
 { (uint8_t *)"XS  A", 0 },
 { (uint8_t *)"XS  B", 0 },
 { (uint8_t *)"XS  C", 0 },
 { (uint8_t *)"XS  D", 0 },
 { (uint8_t *)"XS  E", 0 },
 { (uint8_t *)"XS  F", 0 },
 { (uint8_t *)"NS  0", 0 },
 { (uint8_t *)"NS  1", 0 },
 { (uint8_t *)"NS  2", 0 },
 { (uint8_t *)"NS  3", 0 },
 { (uint8_t *)"NS  4", 0 },
 { (uint8_t *)"NS  5", 0 },
 { (uint8_t *)"NS  6", 0 },
 { (uint8_t *)"NS  7", 0 },
 { (uint8_t *)"NS  8", 0 },
 { (uint8_t *)"NS  9", 0 },
 { (uint8_t *)"NS  A", 0 },
 { (uint8_t *)"NS  B", 0 },
 { (uint8_t *)"NS  C", 0 },
 { (uint8_t *)"NS  D", 0 },
 { (uint8_t *)"NS  E", 0 },
 { (uint8_t *)"NS  F", 0 },
 { 0, 0 }
 };

