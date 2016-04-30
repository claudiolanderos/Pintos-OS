#ifndef VM_SWAP_H
#define VM_SWAP_H

#include "devices/disk.h"
#include "threads/palloc.h"
#include "userprog/syscall.h"
#include "vm/page.h"
#include "vm/frame.h"
#include <debug.h>

#define MAXDD 1024
#define BYTE__ 8
#define P_SIZE 512

// TODO: include file should not have this.
int d_use[MAXDD];

int find_empty (void);
void swap_d_write (struct disk *d, int sec_num, uint8_t *page);
void swap_d_read (struct disk *d, int sec_num, uint8_t *page);

void swap_in (struct page_table *pte, uint8_t *addr); 
void swap_out (struct page_table *pte, uint8_t *kpage);

#endif
