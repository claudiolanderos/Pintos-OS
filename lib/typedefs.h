/* 
 * File:   typedefs.h
 * Author: zangetsu
 *
 * Created on May 19, 2013, 6:39 PM
 */
#include "threads/thread.h"
#include <hash.h>
#include "filesys/file.h"

#ifndef TYPEDEFS_H
#define	TYPEDEFS_H

typedef struct thread* thread_ptr_t;
typedef void* void_ptr_t;
typedef struct hash* hash_ptr_t;
typedef struct hash_elem* hash_elem_ptr_t;
typedef const struct hash_elem* const_hash_elem_ptr_t;
typedef struct frame_entry* frame_entry_ptr_t;
typedef struct file* file_ptr_t;
typedef uint8_t* uint8_ptr_t;
typedef struct page_entry* page_entry_ptr_t;
typedef void** void_db_ptr_t;

#endif	/* TYPEDEFS_H */

