#include "vm/frame.h"
#include "vm/page.h"
#include "vm/swap.h"
#include <hash.h>

static unsigned
frame_hash_func (const struct hash_elem *a, void *aux UNUSED)
{
  struct frame_table *pa = hash_entry (a, struct frame_table, hash_elem);
  return hash_bytes (&pa->page, sizeof pa->page);
}

static bool
frame_less_func (const struct hash_elem *a,
                      const struct hash_elem *b,
                      void *aux UNUSED)
{
  struct frame_table *pa = hash_entry (a, struct frame_table, hash_elem);
  struct frame_table *pb = hash_entry (b, struct frame_table, hash_elem);
  if (pa->page == pb->page)
  {
    return pa->t->tid < pb->t->tid;
  }
  return pa->page < pb->page;
}

void
init_frame(void)
{
  list_init (&frame_list);
  hash_init (&frame_hash, frame_hash_func, frame_less_func, NULL);
  lock_init (&lock_ft);
}

void
make_frame_table (uint8_t *page, struct thread *t)
{
  struct frame_table *fte = (struct frame_table *)malloc(sizeof(struct frame_table));
  ASSERT (fte != NULL);
  fte->page = page;
  fte->t = t;
  frame_table_insert(fte);
}

void
frame_table_insert (struct frame_table *f)
{
  list_push_back (&frame_list,&f->elem);
  hash_insert (&frame_hash, &f->hash_elem);
}

struct frame_table *
frame_table_find (struct thread *t, uint8_t *page)
{
  struct frame_table a;
  a.page = page;
  a.t = t;
  struct hash_elem *ret = hash_find (&frame_hash, &a.hash_elem);

  if (ret == NULL)
  {
    return NULL;
  }
  return hash_entry (ret, struct frame_table, hash_elem);
}

void
frame_table_free (struct thread *t)
{
  struct list_elem *e;
  struct frame_table *f;

  for(e = list_begin(&frame_list); e != list_end(&frame_list); )
  {
    f = list_entry(e,struct frame_table,elem);  
    if (f->t == t)
    {
      e = list_remove (e);
      hash_delete (&frame_hash, &f->hash_elem);
      free (f);
    }
    else{
      e = list_next (e);
    }
  }
}

uint8_t *
victim (void)
{
  struct frame_table *fte;
  struct page_table *p;
  struct thread *frame_thread;
  uint8_t *upage, *kpage;

  fte = list_entry(list_pop_front(&frame_list), struct frame_table, elem);
  frame_thread = fte -> t;
  upage = fte -> page;
  /*  Free FTE  */
  hash_delete (&frame_hash, &fte->hash_elem);
  free(fte);

  p = page_table_find (frame_thread, upage);
  ASSERT (p != NULL);
  kpage = pagedir_get_page(frame_thread -> pagedir,upage);
  ASSERT (kpage != NULL);

  if (pagedir_is_dirty (frame_thread->pagedir, upage)) 
  {
    p->dirty = true;
  }

  pagedir_clear_page(frame_thread->pagedir, upage);
  swap_out (p, kpage);

  return kpage;
}
