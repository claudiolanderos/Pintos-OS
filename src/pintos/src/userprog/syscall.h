#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

#include <stdbool.h>

void syscall_init (void);

int handle_exit (int exit_code);
bool is_valid_memory_filecheck (const void *p);
struct lock file_lock;

#endif /* userprog/syscall.h  */
