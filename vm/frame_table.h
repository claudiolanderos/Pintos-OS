/* 
 * File:   frame_table.h
 * Author: Emre Ardıç
 *
 * Created on May 19, 2013, 6:01 PM
 */

#ifndef FRAME_TABLE_H
#define	FRAME_TABLE_H

#include "lib/typedefs.h"
#include <hash.h>

struct frame_entry
{
    page_entry_ptr_t page;
    thread_ptr_t thread_ptr;
    void_ptr_t frame_ptr;
    uint32_t order;
    struct hash_elem elem;
};

// Initializes frame table as an empty table
void initialize_frame_table(void);

/* Computes and returns the hash value for hash element E, given
   auxiliary data AUX. */
unsigned frame_hash ( const_hash_elem_ptr_t e, void_ptr_t aux);

/* Compares the value of two hash elements A and B, given
   auxiliary data AUX.  Returns true if A is less than B, or
   false if A is greater than or equal to B. */
bool frame_hash_less (  const_hash_elem_ptr_t a,
                        const_hash_elem_ptr_t b,
                        void_ptr_t aux );

frame_entry_ptr_t create_frame(page_entry_ptr_t page);
void destroy_frame(frame_entry_ptr_t frame);
frame_entry_ptr_t retrieve_frame(page_entry_ptr_t page);

void frame_eviction(void);

#endif	/* FRAME_TABLE_H */

