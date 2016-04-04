/* 
 * File:   page_table.c
 * Author: Emre Ardıç
 *
 * Created on May 19, 2013, 6:02 PM
 */

#include "page_table.h"
#include "threads/malloc.h"
#include "frame_table.h"
#include "threads/synch.h"
#include <string.h>
#include "userprog/pagedir.h"
#include "threads/vaddr.h"

// Install page, from process.c
static bool setup_page(void *upage, void *kpage, bool writable);

void init_sup_page_table(hash_ptr_t page_table)
{
    hash_init(page_table, page_hash, page_hash_less, NULL);
}

void destroy_page_table(hash_ptr_t table)
{
    hash_destroy(table, page_action_delete);
}

/* Computes and returns the hash value for hash element E, given
   auxiliary data AUX. */
unsigned page_hash(const_hash_elem_ptr_t e, void_ptr_t aux UNUSED)
{
    page_entry_ptr_t page = hash_entry(e, struct page_entry, elem);
    return hash_int((int) page->user_page);
}

/* Compares the value of two hash elements A and B, given
   auxiliary data AUX.  Returns true if A is less than B, or
   false if A is greater than or equal to B. */
bool page_hash_less(const_hash_elem_ptr_t a,
                    const_hash_elem_ptr_t b,
                    void_ptr_t aux UNUSED)
{
    page_entry_ptr_t page_a = hash_entry(a, struct page_entry, elem),
            page_b = hash_entry(b, struct page_entry, elem);

    return page_a->user_page < page_b->user_page;
}

void page_action_delete(hash_elem_ptr_t hash_elem, void_ptr_t aux UNUSED)
{
    page_entry_ptr_t page_entry = NULL;
    page_entry = hash_entry(hash_elem, struct page_entry, elem);

    if (page_entry != NULL)
    {
        // Remove page
        free(page_entry);
    }
}

/**
 * Creates an supplement page table entry for given parameters
 * @param file
 * @param ofs
 * @param upage
 * @param read_bytes
 * @param zero_bytes
 * @param writable
 * @return True if successful otherwise false.
 */
page_entry_ptr_t create_page_entry(file_ptr_t file, off_t ofs, uint8_ptr_t upage,
                                   uint32_t read_bytes, uint32_t zero_bytes,
                                   bool writable, page_t type)
{
    // Create an entry
    page_entry_ptr_t page = (page_entry_ptr_t) malloc(sizeof (struct page_entry));
    thread_ptr_t curr_thr = thread_current();

    if (page != NULL)
    {// If malloc is successfull

        // Initialize the page entry
        page->file = file;
        page->ofs = ofs;
        page->read_bytes = read_bytes;
        page->zero_bytes = zero_bytes;
        page->user_page = upage;
        page->writable = writable;
        page->page_type = type;

        // Add the new page to the supplement page table
        if (hash_insert(&curr_thr->sup_page_table, &page->elem) == NULL)
        {
            return page;
        }
    }

    // If there is already this page, then free resources.
    free(page);
    return NULL;
}

page_entry_ptr_t retrieve_entry(uint8_ptr_t upage)
{
    struct page_entry temp_page_entry;
    hash_elem_ptr_t hash_elem = NULL;
    page_entry_ptr_t page_entry = NULL;
    thread_ptr_t curr_thr = thread_current();

    temp_page_entry.user_page = upage;
    hash_elem = hash_find(&curr_thr->sup_page_table, &temp_page_entry.elem);

    if (hash_elem != NULL)
    {
        page_entry = hash_entry(hash_elem, struct page_entry, elem);
    }

    return page_entry;
}

bool load_page_from_file(page_entry_ptr_t page)
{
    frame_entry_ptr_t new_frame = NULL;
    uint32_t read_bytes = 0;

    lock_acquire(&gl_file_lock);

    if (page != NULL)
    {
        new_frame = create_frame(page);

        if (new_frame != NULL)
        {
            if (page->read_bytes != 0)
            {
                read_bytes = file_read_at(page->file, new_frame->frame_ptr, page->read_bytes, page->ofs);
                if (read_bytes != page->read_bytes)
                {// If all bytes are not read, then free resources.
                    destroy_frame(new_frame);
                    lock_release(&gl_file_lock);
                    return false;
                }
            }

            // Make zero the zero part
            memset(new_frame->frame_ptr + page->read_bytes, 0, page->zero_bytes);

            if (!setup_page(page->user_page, new_frame->frame_ptr, page->writable))
            {
                destroy_frame(new_frame);
                lock_release(&gl_file_lock);
                return false;
            }

        }

    }

    lock_release(&gl_file_lock);
    return true;
}

void destroy_page_entry(page_entry_ptr_t page)
{
    thread_ptr_t curr_thr = thread_current();
    hash_delete(&curr_thr->sup_page_table, &page->elem);
}

void_ptr_t expand_stack(void_ptr_t uaddr)
{
    page_entry_ptr_t page = NULL;
    frame_entry_ptr_t frame = NULL;

    if (CALC_SIZE(uaddr) < MAX_STACK_SIZE)
    {// If max stack size is not exceeded.     

        page = create_page_entry(NULL, 0, uaddr, 0, 0, true, STACK);
        if (page != NULL)
        {// If page is valid

            frame = create_frame(page);
            if (frame != NULL)
            {// If frame is good

                if (setup_page(page->user_page, frame->frame_ptr, page->writable))
                {// If setup is successful, return kpage
                    return frame->frame_ptr;
                }
                else
                {
                    destroy_frame(frame);
                    destroy_page_entry(page);
                }
            }
            else
            {
                destroy_page_entry(page);
            }
        }
    }

    return NULL;
}

static bool
setup_page(void *upage, void *kpage, bool writable)
{
    struct thread *t = thread_current();

    /* Verify that there's not already a page at that virtual
       address, then map our page there. */
    return (pagedir_get_page(t->pagedir, upage) == NULL
            && pagedir_set_page(t->pagedir, upage, kpage, writable));
}
