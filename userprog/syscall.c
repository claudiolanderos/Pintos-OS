#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "../devices/shutdown.h"
#include "../filesys/filesys.h"
#include "../filesys/file.h"
#include "threads/vaddr.h"
#include "../threads/synch.h"
#include "threads/malloc.h"
#include "../devices/input.h"
#include <console.h>
#include "process.h"

static struct lock locker;
static int fd_gl = 3;
static struct fd_table fd_list;

static int fetch_argument(const void *esp, unsigned int arg_num);

static void syscall_handler(struct intr_frame *);

/* Read and write data from/to user space */
static int get_user_word(const uint8_t *uaddr);
static int get_user(const uint8_t *uaddr);

/* Local functions */
static void system_halt(void) NO_RETURN;
static pid_t system_exec(const char *file);
static bool system_create(const char *file, unsigned initial_size);
static bool system_remove(const char *file);
static int system_open(const char *file);
static int system_filesize(int fd);
static int system_read(int fd, void *buffer, unsigned length);
static int system_write(int fd, const void *buffer, unsigned length);
static void system_seek(int fd, unsigned position);
static unsigned system_tell(int fd);
static void system_close(int fd);

void
syscall_init(void)
{
    intr_register_int(0x30, 3, INTR_ON, syscall_handler, "syscall");
    lock_init(&locker);
    init_fd_table(&fd_list);
}

/**
 * Handles system calls, esp should point sys call no
 * eax should contain return value of sys call.
 * esp is stack ptr
 * @param f
 */
static void
syscall_handler(struct intr_frame *f UNUSED)
{
    /* System call number is loaded to stack before this function is called.
     * esp now points(should point) to 4 byte sys call number in stack. 
     */
    int system_call_number = fetch_argument(f->esp, 0);

    /*printf("\n-->Call num:%d, 1-> %x , 2-> %x , 3-> %x\n", system_call_number, fetch_argument(f->esp, 1),
           fetch_argument(f->esp, 2),
           fetch_argument(f->esp, 3));*/
    //hex_dump((int)(f->esp),f->esp,100,true);

    switch (system_call_number)
    {/* Select proper system call */

        case SYS_HALT:
            system_halt();
            break; /* Halt the operating system. */

        case SYS_EXIT:
            system_exit((int) fetch_argument(f->esp, 1));
            break; /* Terminate this process. */

        case SYS_EXEC:
            f->eax = system_exec((const char*) fetch_argument(f->esp, 1));
            break; /* Start another process. */

        case SYS_WAIT:
            f->eax = process_wait((tid_t) fetch_argument(f->esp, 1));
            break; /* Wait for a child process to die. */

        case SYS_CREATE:
            f->eax = system_create((const char*) fetch_argument(f->esp, 1),
                                   (unsigned) fetch_argument(f->esp, 2));
            break; /* Create a file. */

        case SYS_REMOVE:
            f->eax = system_remove((const char*) fetch_argument(f->esp, 1));
            break; /* Delete a file. */

        case SYS_OPEN:
            f->eax = system_open((const char*) fetch_argument(f->esp, 1));
            break; /* Open a file. */

        case SYS_FILESIZE:
            f->eax = system_filesize((int) fetch_argument(f->esp, 1));
            break; /* Obtain a file's size. */

        case SYS_READ:
            f->eax = system_read((int) fetch_argument(f->esp, 1),
                                 (void*) fetch_argument(f->esp, 2),
                                 (unsigned) fetch_argument(f->esp, 3));
            break; /* Read from a file. */

        case SYS_WRITE:
            f->eax = system_write((int) fetch_argument(f->esp, 1),
                                  (void*) fetch_argument(f->esp, 2),
                                  (unsigned) fetch_argument(f->esp, 3));
            break; /* Write to a file. */

        case SYS_SEEK:
            system_seek((int) fetch_argument(f->esp, 1), (unsigned) fetch_argument(f->esp, 2));
            break; /* Change position in a file. */

        case SYS_TELL:
            f->eax = system_tell((int) fetch_argument(f->esp, 1));
            break; /* Report current position in a file. */

        case SYS_CLOSE:
            system_close((int) fetch_argument(f->esp, 1));
            break; /* Close a file. */
    }
}

static pid_t system_exec(const char *file)
{
    int retVal = -1;

    lock_acquire(&locker);
    
    if (file != NULL && is_user_vaddr(file))
    {
        retVal = process_execute(file);
    }
    lock_release(&locker);

    return retVal;
}

static void system_halt(void)
{
    shutdown_power_off();
}

void system_exit(int status)
{
    printf("%s: exit(%d)\n", thread_current()->name, status);
    thread_current()->return_value = status;
    thread_exit();
}

static bool system_remove(const char *file)
{
    bool ret;

    lock_acquire(&locker);

    if (!is_user_vaddr(file))
    {
        lock_release(&locker);
        system_exit(-1);
    }

    ret = filesys_remove(file);

    lock_release(&locker);

    return ret;
}

static int system_open(const char *file)
{
    struct file * file_ptr = NULL; /* opened file by filesys_open */
    int fd = -1;

    lock_acquire(&locker);

    if (!is_user_vaddr(file))
    {
        lock_release(&locker);
        system_exit(-1);
    }

    filesys_create(file, 500);

    file_ptr = filesys_open(file);

    if (file_ptr != NULL)
    {
        fd = add_file(&fd_list, file_ptr);
    }

    //printf("\nOpen %s, fd:%d, owner :%d , lsize:%d\n", file, fd, get_file_owner(&fd_list, fd), fd_list.size);

    lock_release(&locker);

    return fd;
}

static void system_close(int fd)
{
    struct file *file_ptr = NULL;

    lock_acquire(&locker);

    file_ptr = get_file(&fd_list, fd);

    //printf("\nClose, fd:%d , in list %p, owner:%d , lsize:%d\n", fd, file_ptr, get_file_owner(&fd_list, fd), fd_list.size);

    if (file_ptr != NULL)
    {
        file_close(file_ptr);
        remove_file(&fd_list, fd);
    }

    lock_release(&locker);
}

static int system_read(int fd, void *buffer, unsigned length)
{
    int retVal = -1;
    unsigned int counterForLoop;
    struct file* file_ptr = NULL;

    lock_acquire(&locker);

    if (fd == STDIN_FILENO/*SDTIN_FILENO*/)
    {
        for (counterForLoop = 0;
             counterForLoop != length;
             ++counterForLoop)
        {
            *(uint8_t*) (buffer + counterForLoop) = input_getc();
        }
        retVal = length;
    }
    else if (fd == STDOUT_FILENO)
    {
        retVal = -1;
    }
    else
    {
        if (!is_user_vaddr(buffer + length))
        {
            lock_release(&locker);
            system_exit(-1);
        }
        else
        {

            file_ptr = get_file(&fd_list, fd);

            if (file_ptr != NULL)
            {/* If file is valid and pointers are in user space(means valid ptr) */
                retVal = file_read(file_ptr, buffer, length);
            }
            else
            {
                retVal = -1;
            }

            //printf("\nRead, fd:%d , in list : %p , ret:%d, lsize:%d\n", fd, file_ptr, retVal, fd_list.size);
        }
    }


    lock_release(&locker);

    return retVal;

}

static int system_write(int fd, const void *buffer, unsigned length)
{
    struct file *file_ptr = NULL;
    int retVal = -1;

    lock_acquire(&locker);

    if (fd == STDOUT_FILENO)
    {
        putbuf(buffer, length);
        retVal = length;
    }
    else if (fd == STDIN_FILENO)
    {
        retVal = -1;
    }
    else
    {// If the process has open file,and want to write it.

        if (!is_user_vaddr(buffer + length))
        {
            lock_release(&locker);
            system_exit(-1);
        }
        else
        {
            file_ptr = get_file(&fd_list, fd);

            if (file_ptr != NULL)
            {/* If file is valid and pointers are in user space(means valid ptr) */
                retVal = file_write(file_ptr, buffer, length);
            }
            else
            {// Error 
                retVal = 0;
            }

           // printf("\nWrite, fd:%d , in list :%p , ret:%d , owner:%d ,lsize:%d\n", fd, file_ptr, retVal, thread_current()->tid, fd_list.size);

        }

    }

    lock_release(&locker);
    return retVal;
}

static bool system_create(const char *file, unsigned initial_size)
{
    bool status;

    lock_acquire(&locker);
    
    if (!is_user_vaddr(file))
    {
        lock_release(&locker);
        system_exit(-1);
    }
    
    status = filesys_create(file, initial_size);

    //printf("\n[ '%s' , init_size: %d create stat:%d]\n",file,initial_size,status);

    lock_release(&locker);
    return status;
}

static int system_filesize(int fd)
{
    int size = 0;
    struct file *file_ptr = NULL;

    lock_acquire(&locker);

    file_ptr = get_file(&fd_list, fd);

    if (file_ptr != NULL)
    {
        size = file_length(file_ptr);
    }

    lock_release(&locker);
    return size;
}

static void system_seek(int fd, unsigned position)
{
    struct file *file_ptr = NULL;

    lock_acquire(&locker);
    
    file_ptr = get_file(&fd_list, fd);

    if (file_ptr != NULL)
    {
        file_seek(file_ptr, position);
    }

    lock_release(&locker);
}

static unsigned system_tell(int fd)
{
    struct file *file_ptr = NULL;
    unsigned int tell = 0;

    lock_acquire(&locker);

    file_ptr = get_file(&fd_list, fd);

    if (file_ptr != NULL)
    {
        tell = file_tell(file_ptr);
    }

    lock_release(&locker);
    return tell;
}

/**
 * Returns an integer from stack.
 * @param esp is stack pointer
 * @param arg_num is the argument number
 * @return integer
 */
static int fetch_argument(const void *esp, unsigned int arg_num)
{
    const void *ptr = esp + arg_num * 4;
    if (!is_user_vaddr(ptr))
    {
        system_exit(EXIT_FAILURE);
    }

    return get_user_word(ptr);
}

/**
 * Gets 4byte from user memory space begining adress uaddr
 * @param uaddr
 * @return 
 */
static int
get_user_word(const uint8_t *uaddr)
{
    int lsb, msb, sec, thrd;

    lsb = get_user(uaddr);
    sec = get_user(uaddr + 1) << 8;
    thrd = get_user(uaddr + 2) << 16;
    msb = get_user(uaddr + 3) << 24;

    return msb + thrd + sec + lsb;
}

/* Reads a byte at user virtual address UADDR.
UADDR must be below PHYS_BASE.
Returns the byte value if successful, -1 if a segfault
occurred. */
static int
get_user(const uint8_t *uaddr)
{
    int result;

    if (!is_user_vaddr(uaddr))
    {
        system_exit(EXIT_FAILURE);
    }

    asm("movl $1f, %0; movzbl %1, %0; 1:"
        : "=&a" (result) : "m" (*uaddr));
    return result;
}

void init_fd_table(struct fd_table* table)
{
    table->size = 0;
}

int add_file(struct fd_table *table, struct file* file_ptr)
{
    int size = table->size;
    struct file_element file_el;
    struct thread * thr = thread_current();

    /* Allocate fd and set datas */
    fd_gl++;
    file_el.fd = fd_gl;
    file_el.file_ptr = file_ptr;
    file_el.owner = thr->tid;

    /* Add to table */
    table->files[size] = file_el;
    table->size++;

    return file_el.fd;
}

struct file* get_file(struct fd_table * table, int fd)
{
    int i = 0;
    int size = table->size;

    for (i = 0; i < size; ++i)
    {
        if (table->files[i].fd == fd)
        {
            return table->files[i].file_ptr;
        }
    }
    return NULL;
}

pid_t get_file_owner(struct fd_table * table, int fd)
{
    int i = 0;
    int size = table->size;

    for (i = 0; i < size; ++i)
    {
        if (table->files[i].fd == fd)
        {
            return table->files[i].owner;
        }
    }
    return -1;
}

struct file* remove_file(struct fd_table * table, int fd)
{
    int i = 0;
    int j = 0;
    int size = table->size;
    struct file * file_ptr;

    for (i = 0; i < size; ++i)
    {
        if (table->files[i].fd == fd)
        {/* If found, shift elements and return file * */
            file_ptr = table->files[i].file_ptr;
            for (j = i; j < size - 1; ++j)
            {
                table->files[j] = table->files[j + 1];
            }
            table->size--;
            return file_ptr;
        }
    }
    return NULL;
}