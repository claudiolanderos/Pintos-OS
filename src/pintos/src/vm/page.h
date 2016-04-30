#ifndef VM_PAGE_H
#define VM_PAGE_H

#include "vm/frame.h"
#include "threads/thread.h"
#include "threads/synch.h"
#include "threads/interrupt.h"
#include "threads/vaddr.h"
#include "threads/pte.h"
#include "threads/palloc.h"
#include "threads/malloc.h"
#include "userprog/syscall.h"
#include "userprog/pagedir.h"
#include "userprog/process.h"
#include "filesys/file.h"
#include "filesys/off_t.h"
#include <string.h>
#include <list.h>
#include <hash.h>
#include "devices/disk.h"

#define DENY_PAGE_NUM 1024
#define DENY_USER_MEM 512*4096 
#define DENY_KERNEL_MEM 8192

struct page_table {
	bool swap;
	bool writable;
	uint8_t *page;
	uint8_t *kpage;
	int swp;
	bool dirty;
	bool is_loaded;
	struct hash_elem hash_elem;
};

bool check_fault (struct intr_frame *f, void *fault_addr, bool not_present, bool user);
void page_table_init (struct hash *pagetable);
struct page_table * page_table_find (struct thread *t, uint8_t *page_num);
void page_table_entry_free (struct page_table *a);
void page_table_free (struct thread *t);
void page_set_entry (void *upage, bool writable, void *kpage);
void page_table_print (void);

#endif
