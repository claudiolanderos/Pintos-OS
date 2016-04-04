/* 
 * File:   page_table.h
 * Author: Emre Ardıç
 *
 * Created on May 19, 2013, 6:02 PM
 */

#ifndef PAGE_TABLE_H
#define	PAGE_TABLE_H

#include <hash.h>
#include "lib/typedefs.h"

#define CALC_SIZE(uaddr) ((uint32_t)(((uint8_ptr_t)PHYS_BASE) - (uint8_ptr_t)uaddr))

// 1MB = 1.048.576 BYTE
#define MAX_STACK_SIZE 1048576

typedef enum
{
    STACK, FILE, SWAP
}page_t;

struct page_entry
{
    uint8_ptr_t user_page; // Virtual address of a physical memory
    
    file_ptr_t file; // The file which the page is read from
    off_t ofs; // Last position which page data is read in file    
    uint32_t read_bytes; // The number of bytes which are read and valid
    uint32_t zero_bytes; // The number of bytes which are zero and invalid
    bool writable; // True if the page is writable or false 
    
    page_t page_type; // Type of the page,stack,file or swap
    
    struct hash_elem elem;
};

void init_sup_page_table(hash_ptr_t page_table);

void destroy_page_table(hash_ptr_t table);

/* Computes and returns the hash value for hash element E, given
   auxiliary data AUX. */
unsigned page_hash ( const_hash_elem_ptr_t e, void_ptr_t aux);

/* Compares the value of two hash elements A and B, given
   auxiliary data AUX.  Returns true if A is less than B, or
   false if A is greater than or equal to B. */
bool page_hash_less (  const_hash_elem_ptr_t a,
                       const_hash_elem_ptr_t b,
                       void_ptr_t aux );

void page_action_delete(hash_elem_ptr_t hash_elem, void_ptr_t aux UNUSED);

page_entry_ptr_t create_page_entry(file_ptr_t file, off_t ofs, uint8_ptr_t upage,
                                   uint32_t read_bytes, uint32_t zero_bytes, 
                                   bool writable, page_t type);


void destroy_page_entry(page_entry_ptr_t page);

page_entry_ptr_t retrieve_entry(uint8_ptr_t upage);

bool load_page_from_file(page_entry_ptr_t page_entry);

void_ptr_t expand_stack(void_ptr_t uaddr);



#endif	/* PAGE_TABLE_H */

