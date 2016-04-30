#ifndef VM_FRAME_H
#define VM_FRAME_H

#include "threads/thread.h"
#include "threads/synch.h"
#include "threads/malloc.h"
#include "threads/palloc.h"
#include "threads/pte.h"
#include "devices/disk.h"
#include "userprog/pagedir.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <list.h>
#include <hash.h>

struct frame_table {
	uint8_t *page;
	struct thread *t;
	struct list_elem elem;
	struct hash_elem hash_elem;
};

// TODO: include file should not have these.
struct list frame_list;
struct hash frame_hash;
struct lock lock_ft;

void make_frame_table (uint8_t *, struct thread *);
void init_frame (void);
uint8_t * victim (void);
void frame_table_insert (struct frame_table *f);
struct frame_table * frame_table_find (struct thread *t, uint8_t *page_num);
void frame_table_free (struct thread *t);
#endif
