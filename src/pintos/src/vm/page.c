#include "vm/page.h"
#include "vm/swap.h"

bool 
check_fault (struct intr_frame *f, void *fault_addr, bool not_present, bool user UNUSED)
{
  uint8_t *fault_pg = pg_round_down (fault_addr);
  uint8_t *kpage;
  struct thread *t = thread_current ();
  struct page_table *pt;
  bool get_lock_here = false;

  // Check the vaild address
  if (fault_addr >= PHYS_BASE || fault_addr == 0)
  {
    return true;
  }

  if (not_present)
  {
    if (pt_no(fault_pg) >= DENY_PAGE_NUM)
    {
      return true;
    }

    pt = page_table_find (t,fault_pg);

    if (pt != NULL) {
      // When Page exists
      if (!lock_held_by_current_thread (&lock_ft)) 
      {
        get_lock_here = true;
        lock_acquire (&lock_ft);
      }
      
      if(pt->swap) {
        swap_in (pt, fault_pg);
        if (get_lock_here) 
          lock_release (&lock_ft);
        return false;
      }
      NOT_REACHED();
    }
    else
    {
      // When Page does not exists.
      // Stack Growth
      
      if (fault_addr < PHYS_BASE - DENY_USER_MEM)
      {
        return true;
      }
      if (fault_addr <f->esp-32) {
        return true; 
      } 

      void *upage = fault_pg; 

      if (!lock_held_by_current_thread (&lock_ft))
      {
        get_lock_here = true;
        lock_acquire (&lock_ft);
      }
      kpage = palloc_get_page (PAL_USER | PAL_ZERO);
      if(kpage == NULL) 
        kpage = victim();

      pagedir_set_page(t->pagedir,upage,kpage,true);
      make_frame_table (upage, t);
      page_set_entry (upage, true, kpage);
      if (get_lock_here)
      {
        lock_release (&lock_ft);
      }

      return false;
    }
  }
  return true;
}

static unsigned
page_hash_func (const struct hash_elem *a, void *aux UNUSED)
{
  struct page_table *pa = hash_entry (a, struct page_table, hash_elem);
  return hash_bytes (&pa->page, sizeof pa->page);
}

static bool
page_less_func (const struct hash_elem *a,
                const struct hash_elem *b,
                void *aux UNUSED)
{
  struct page_table *pa = hash_entry (a, struct page_table, hash_elem);
  struct page_table *pb = hash_entry (b, struct page_table, hash_elem);
  return pa->page < pb->page;
}

void
page_table_init (struct hash *pagetable) {
  hash_init (pagetable, page_hash_func, page_less_func, NULL);
}

static void
page_table_insert (struct hash *pagetable, struct page_table *p)
{
  hash_insert (pagetable, &p->hash_elem);
}

void
page_set_entry (void *upage, bool writable, void *kpage)
{
  struct page_table *p = (struct page_table *)malloc(sizeof(struct page_table));
  ASSERT (p != NULL);
  p->swap = false;
  p->page = upage;
  p->writable = writable;
  p->kpage = kpage;

  p->is_loaded = false;

  p->dirty = false;

  page_table_insert (&(thread_current()->pagetable), p);
}

struct page_table *
page_table_find (struct thread *t, uint8_t *page)
{
  struct page_table a;
  a.page = page;
  struct hash_elem *ret = hash_find (&t->pagetable, &a.hash_elem);
  if (ret == NULL)
  {
    return NULL;
  }
  return hash_entry (ret, struct page_table, hash_elem); 
}

static void
page_destructor (struct hash_elem *a, void * aux UNUSED) {
  struct page_table *pa = hash_entry (a, struct page_table, hash_elem);
  if (pa->swap)
  {
    d_use[pa->swp/8] = false;
  }
  // palloc_free_page() is in process_exit ();
  free (pa);
}

void page_table_free (struct thread *t)
{
  hash_destroy (&t->pagetable, page_destructor);
}

void page_table_entry_free (struct page_table *a)
{
  struct thread *t = thread_current ();
  hash_delete (&t->pagetable, &a->hash_elem);
  if (a->swap)
  {
    d_use[a->swp/8] = false;
  } 
  else if (a->is_loaded) 
  {
    pagedir_clear_page(t->pagedir, a->page);
  }
  free (a);
}
