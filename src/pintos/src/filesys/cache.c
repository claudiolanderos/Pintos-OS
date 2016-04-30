#include "filesys/cache.h"
#include <debug.h>
#include <list.h>
#include <string.h>
#include "filesys/filesys.h"
#include "threads/malloc.h"
#include "threads/synch.h"
#include "threads/thread.h"
#include "devices/disk.h"
#include "devices/timer.h"

struct cache_block 
{
  bool valid;
  bool loaded;
  bool dirty;
  disk_sector_t sector;
  uint8_t data[DISK_SECTOR_SIZE];
  int readers, writers, read_waiters, write_waiters;
  struct lock data_lock;
  struct lock rw_lock;
  struct condition read, write;
};

struct read_info
{
  disk_sector_t sector;
  struct list_elem elem;
};

static struct cache_block cache_table[CACHE_SIZE];
static int victim_idx;
static struct lock cache_lock;
static struct lock read_lock;
static struct condition read_list_nonempty;
static struct list read_list;

static struct cache_block *cache_victim (void);

void
cache_init (void) 
{
  int i;
  for (i = 0; i < CACHE_SIZE; ++i) 
  {
    cache_table[i].valid = false;
    lock_init (&cache_table[i].data_lock);
    lock_init (&cache_table[i].rw_lock);
    cond_init (&cache_table[i].read);
    cond_init (&cache_table[i].write);
    cache_table[i].readers = cache_table[i].writers = cache_table[i].read_waiters = cache_table[i].write_waiters = 0;
  }

  victim_idx = 0;
  lock_init (&cache_lock);
  lock_init (&read_lock);
  cond_init (&read_list_nonempty);
  list_init (&read_list);
}

void
cache_flush (struct cache_block *cache)
{
  lock_acquire (&cache->data_lock);

  if (cache->loaded && cache->dirty)
  {
    disk_write (filesys_disk, cache->sector, cache->data);
    cache->dirty = false;
  }

  lock_release (&cache->data_lock);
}

void cache_flush_all (void)
{
  int i;

  for (i = 0; i < CACHE_SIZE; ++i)
  {
    lock_acquire (&cache_lock);
    struct cache_block *cache = &cache_table[i];
    if (cache->valid)
    {
      cache_flush (cache);
    }
    lock_release (&cache_lock);  
  }
}

struct cache_block *
cache_acquire (disk_sector_t sector, enum cache_lock_type lock_type)
{
  int i;
  bool cache_lock_acquired = true;
  struct cache_block *cache = NULL;

  lock_acquire (&cache_lock);

  for (i = 0; i < CACHE_SIZE; ++i)
  {
    if (cache_table[i].valid && cache_table[i].sector == sector)
    {
      cache = &cache_table[i];
      break;
    }
  }

  if (cache == NULL)
  {
    cache = cache_victim ();
    lock_acquire (&cache->rw_lock);
    cache->sector = sector;
    cache->valid = true;
  }
  else
  {
    lock_acquire (&cache->rw_lock);
    lock_release (&cache_lock);
    cache_lock_acquired = false;
  }

  if (lock_type == READ_ONLY)
  {
    if (cache->writers || cache->write_waiters)
    {
      ++cache->read_waiters;
      do
      {
        cond_wait (&cache->read, &cache->rw_lock);
      } while (cache->writers || cache->write_waiters);
      --cache->read_waiters;
    }
    ++cache->readers;
  }
  else
  {
    if (cache->readers || cache->writers)
    {
      ++cache->write_waiters;
      do
      {
        cond_wait (&cache->write, &cache->rw_lock);
      } while (cache->readers || cache->writers);
      --cache->write_waiters;
    }
    cache->writers = 1;
  }

  lock_release (&cache->rw_lock);
  if (cache_lock_acquired)
  {
    lock_release (&cache_lock);
  }

  return cache;
}

void
cache_release (struct cache_block *cache)
{
  lock_acquire (&cache_lock);
  lock_acquire (&cache->rw_lock);

  if (cache->readers)
  {
    --cache->readers;
    if (cache->write_waiters)
    {
      cond_signal (&cache->write, &cache->rw_lock);
    }
  }
  else
  {
    cache->writers = 0;
    if (cache->write_waiters)
    {
      cond_signal (&cache->write, &cache->rw_lock);
    }
    else
    {
      cond_broadcast (&cache->read, &cache->rw_lock);
    }
  }

  lock_release (&cache->rw_lock);
  lock_release (&cache_lock);  
}

static void
cache_load (struct cache_block *cache)
{
  if (!cache->loaded)
  {
    disk_read (filesys_disk, cache->sector, cache->data);
    cache->loaded = true;
    cache->dirty = false;
  }
}

void
cache_read (struct cache_block *cache, int offset, int size, uint8_t *buffer)
{
  lock_acquire (&cache->data_lock);
  cache_load (cache);
  memcpy (buffer, cache->data + offset, size);
  lock_release (&cache->data_lock);
}

void
cache_write (struct cache_block *cache, int offset, int size, const uint8_t *buffer)
{
  lock_acquire (&cache->data_lock);
  if (offset == 0 && size == DISK_SECTOR_SIZE)
  {
    cache->loaded = true;
  }
  else
  {
    cache_load (cache);
  }
  memcpy (cache->data + offset, buffer, size);
  cache->dirty = true;
  lock_release (&cache->data_lock);
}

void
cache_fillzero (struct cache_block *cache)
{
  cache_load (cache);
  memset (cache->data, 0, sizeof (cache->data));
  cache->dirty = true;
}

static struct cache_block *
cache_victim (void)
{
  struct cache_block *cache;

  int count;
  for (count = 0; true; victim_idx = (victim_idx + 1) % CACHE_SIZE, ++count)
  {
    cache = &cache_table[victim_idx];

    if (!cache->valid)
    {
      break;
    }

    if (count >= CACHE_SIZE && cache->readers == 0 && cache->writers == 0 &&
        cache->read_waiters == 0 && cache->write_waiters == 0)
    {
      cache_flush (cache);
      break;
    }
  }

  cache->valid = false;
  cache->loaded = false;
  cache->dirty = false;

  return cache;
}

void
cache_flushd (void *aux UNUSED)
{
  while (true)
  {
    timer_sleep (5);
    cache_flush_all ();
  }
}

void
cache_readd (void *aux UNUSED)
{
  while (true)
  {
    lock_acquire (&read_lock);
    while (list_empty (&read_list))
    {
      cond_wait (&read_list_nonempty, &read_lock);
    }
    struct read_info *read = list_entry (list_pop_front (&read_list),
                                         struct read_info, elem);
    lock_release (&read_lock);

    struct cache_block *cache = cache_acquire (read->sector, READ_ONLY);
    cache_load (cache);
    cache_release (cache);
    free (read);
  }
}

void
cache_read_request (disk_sector_t sector)
{
  struct read_info *read = (struct read_info *) malloc (sizeof (struct read_info));
  if (read == NULL)
  {
    return;
  }

  read->sector = sector;
  lock_acquire (&read_lock);
  list_push_back (&read_list, &read->elem);
  cond_signal (&read_list_nonempty, &read_lock);
  lock_release (&read_lock);
}
