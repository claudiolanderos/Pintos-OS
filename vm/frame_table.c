/* 
 * File:   frame_table.c
 * Author: Emre Ardıç
 *
 * Created on May 19, 2013, 6:01 PM
 */

#include "frame_table.h"
#include "filesys/file.h"
#include "threads/thread.h"
#include "threads/synch.h"
#include "threads/malloc.h"
#include "threads/palloc.h"
#include "userprog/pagedir.h"
#include "page_table.h"

#include "lib/typedefs.h"

// Frame table, hashing
static struct hash frame_table;
static struct lock locker;

void initialize_frame_table(void)
{
    hash_init(&frame_table, frame_hash, frame_hash_less,NULL);    
    lock_init(&locker);
}

/* Computes and returns the hash value for hash element E, given
   auxiliary data AUX. */
unsigned frame_hash ( const_hash_elem_ptr_t e, void_ptr_t aux UNUSED)
{
   frame_entry_ptr_t frame = hash_entry(e,struct frame_entry,elem);
   return hash_int((int)frame->order);   
}

/* Compares the value of two hash elements A and B, given
   auxiliary data AUX.  Returns true if A is less than B, or
   false if A is greater than or equal to B. */
bool frame_hash_less (  const_hash_elem_ptr_t a,
                        const_hash_elem_ptr_t b,
                        void_ptr_t aux UNUSED)
{
    frame_entry_ptr_t frame_a = hash_entry(a,struct frame_entry,elem),
                      frame_b = hash_entry(b,struct frame_entry,elem);

    return frame_a->order < frame_b->order;
}

/**
 * Creates a frame and initializes it using given page
 * @param page
 * @return frame address
 */
frame_entry_ptr_t create_frame(page_entry_ptr_t page)
{
    frame_entry_ptr_t frame_entry = NULL;    
    void_ptr_t new_frame = NULL;
    
    lock_acquire(&locker);
    
    while(new_frame == NULL)
    {
        frame_entry = (frame_entry_ptr_t)malloc(sizeof(struct frame_entry));    
        new_frame = palloc_get_page(PAL_USER);

        if(new_frame != NULL)
        {    
            frame_entry->frame_ptr = new_frame;
            frame_entry->thread_ptr = thread_current();
            frame_entry->page = page;
            frame_entry->order = hash_size(&frame_table);
            hash_insert(&frame_table,&frame_entry->elem);
        }
        else
        {// No Mem Space,must remove a page to get space
            frame_eviction();
        }
    }
    
    lock_release(&locker);
    
    return frame_entry;
}

/**
 * Frees given frame from memory
 * @param frame
 */
void destroy_frame(frame_entry_ptr_t frame)
{
    hash_elem_ptr_t frame_elem = NULL;
   
    lock_acquire(&locker);
    
    if(frame != NULL)
    {
        frame_elem = hash_find(&frame_table, &frame->elem);

        if(frame_elem != NULL && frame->frame_ptr != NULL)
        {
            hash_delete(&frame_table, &frame->elem);
            palloc_free_page(frame->frame_ptr);
            free(frame);        
        }
    }
    lock_release(&locker);
}

frame_entry_ptr_t retrieve_frame(page_entry_ptr_t page)
{
    struct hash_iterator i;
    
    lock_acquire(&locker);
    
    hash_first (&i, &frame_table);
    while (hash_next (&i))
    {
        frame_entry_ptr_t frame = hash_entry (hash_cur (&i), struct frame_entry, elem);
        if(frame->page == page){
            lock_release(&locker);
            return frame;
        }      
    }    
    
    lock_release(&locker);
    
    return NULL;
}

// Creates empty space for a frame in frame table
void frame_eviction(void)
{
    struct hash_iterator i;
    thread_ptr_t thread=NULL;
    page_entry_ptr_t page = NULL;
    
    lock_acquire(&locker);
    
    if(!hash_empty(&frame_table))
    {// If table is not empty
        
        // Get first element
        hash_first (&i, &frame_table);
        hash_next (&i);    
        frame_entry_ptr_t frame = hash_entry (hash_cur (&i),struct frame_entry,elem);
        
        // Check if the page is dirty
        thread = frame->thread_ptr;
        page = frame->page; 
        if(pagedir_is_dirty(thread->pagedir, page))
        {            
            file_write_at(page->file,frame->frame_ptr,page->read_bytes,page->ofs);
        }        
        
        //Free resources
        hash_delete(&frame_table, &frame->elem);
        pagedir_clear_page(thread->pagedir, frame->page->user_page);
        palloc_free_page(frame->frame_ptr);
        free(frame);        
    }      
    
     lock_release(&locker);
}


