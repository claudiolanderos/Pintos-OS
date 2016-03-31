#ifndef USERPROG_PROCESS_H
#define USERPROG_PROCESS_H

#include "threads/thread.h"

#define ARGLEN 30
#define MAX_ARG 128 

struct argument
{
    char *arg; /* Argument(which is a string) pointer */
    size_t len; /* Length of the argument */
    void *esp; /* Address of the argument in stack */
    struct list_elem elem; /* for listing */
};



tid_t process_execute (const char *file_name);
int process_wait (tid_t);
void process_exit (void);
void process_activate (void);

#endif /* userprog/process.h */
