/*
 * flash.h
 *
 *  Created on: 2021/01/30
 *      Author: tenda_000
 */

#ifndef SRC_FLASH_H_
#define SRC_FLASH_H_

#define PAGE_SIZE 256
#define SECTOR_SIZE 4096

void flash_read (byte_t *dest, faddr_t src, foff_t len);	// read: ram <- flash
void flash_write (faddr_t dest, byte_t *src, foff_t len);	// write: flash <- ram
void flash_erase (faddr_t flashaddr, foff_t len);
void flash_erase_write (faddr_t dest, byte_t *src, foff_t len);	// ew: flash <- ram

int do_flash_read_command (int ac, void *arg);
int do_flash_erase_write_command (int ac, void *arg);
int do_flash_erase_command (int ac, void *arg);
int do_flash_command (int ac, void *arg);

#endif /* SRC_FLASH_H_ */
