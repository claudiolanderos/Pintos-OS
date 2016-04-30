#ifndef FILESYS_CACHE_H
#define FILESYS_CACHE_H

#include "filesys/filesys.h"
#include "devices/disk.h"
#include "lib/debug.h"

#define CACHE_SIZE 64

enum cache_lock_type
{
  READ_ONLY,
  READ_WRITE
};

void cache_init (void);
struct cache_block *cache_acquire (disk_sector_t sector, enum cache_lock_type lock_type);
void cache_release (struct cache_block *cache);
void cache_read (struct cache_block *cache, int offset, int size, uint8_t *buffer);
void cache_write (struct cache_block *cache, int offset, int size, const uint8_t *buffer);
void cache_fillzero (struct cache_block *cache);
void cache_flush (struct cache_block *cache);
void cache_flush_all (void);
void cache_flushd (void *aux UNUSED);
void cache_readd (void *aux UNUSED);
void cache_read_request (disk_sector_t sector);

#endif
