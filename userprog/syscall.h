#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

#define EXIT_SUCCESS 0          /* Successful execution. */
#define EXIT_FAILURE 1          /* Unsuccessful execution. */

#define FD_TABLE_SIZE 256

/* Process identifier. */
typedef int pid_t;

struct file_element
{
    struct file *file_ptr;
    int fd;
    pid_t owner;
};

/* Global fd table */
struct fd_table
{
    struct file_element files[FD_TABLE_SIZE];
    int size;
};

void init_fd_table(struct fd_table*);
int add_file(struct fd_table * table, struct file*);
struct file* get_file(struct fd_table * table, int fd);
pid_t get_file_owner(struct fd_table * table, int fd);
struct file* remove_file(struct fd_table * table, int fd);

void syscall_init (void);
void system_exit(int status);

#endif /* userprog/syscall.h */
