#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include <user/syscall.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "userprog/process.h"
#include "userprog/fd.h"
#include "filesys/file.h"
#include "filesys/filesys.h"
#include "filesys/directory.h"
#include "threads/vaddr.h"
#include "userprog/pagedir.h"
#include "userprog/exception.h"
#include "devices/input.h"

#define SYSCALL(esp) (get_user_4 (esp))
#define ARG0(esp) (get_user_4 ((esp) + 4))
#define ARG1(esp) (get_user_4 ((esp) + 8))
#define ARG2(esp) (get_user_4 ((esp) + 12))

/* Reads a byte at user virtual address UADDR.
   UADDR must be below PHYS_BASE.
   Returns the byte value if successful, -1 if a segfault
   occurred. */

struct lock file_lock;

static int
get_user (const uint8_t *uaddr)
{
  if ( uaddr == NULL || !is_user_vaddr (uaddr))
  {
    handle_exit (-1);
  }
  int result;
  asm ("movl $1f, %0; movzbl %1, %0; 1:"
       : "=&a" (result) : "m" (*uaddr));  
  return result;
}

static int
get_user_4 (const uint8_t *uaddr)
{
  return get_user (uaddr) +
      (get_user (uaddr + 1) << 8) +
      (get_user (uaddr + 2) << 16) +
      (get_user (uaddr + 3) << 24);
}

bool
is_valid_memory_filecheck (const void *p)
{
  if (p != NULL && is_user_vaddr (p))
  {
    return true;
  }
  return false;
}

static int
handle_halt (void)
{
  thread_exit ();
}

int
handle_exit (int exit_code)
{
  thread_current ()->exit_code = exit_code;
  printf ("%s: exit(%d)\n", thread_current ()->name, exit_code);
  thread_exit ();
}

static int
handle_wait (pid_t pid)
{
  int val = process_wait (pid);
  return val;
}

static int
handle_exec (const char *cmd)
{
  lock_acquire (&file_lock);
  int result = process_execute (cmd);
  lock_release (&file_lock);
  return result;
}

static int
handle_create (const char *path, off_t file_size)
{
  if (!is_valid_memory_filecheck (path))
  {
    handle_exit (-1);
  }
  lock_acquire (&file_lock);
  int result = filesys_create (path, file_size);
  lock_release (&file_lock);
  return result;
}

static int
handle_remove (const char *path)
{
  if (!is_valid_memory_filecheck (path))
  {
    handle_exit (-1);
  }
  lock_acquire (&file_lock);
  int result = filesys_remove (path);
  lock_release (&file_lock);
  return result;
}

static int
handle_open (const char *path)
{
  if (!is_valid_memory_filecheck (path))
  {
    handle_exit (-1);
  }

  lock_acquire (&file_lock);
  struct file *file_opened = filesys_open (path);

  if (file_opened == NULL)
  {
    lock_release (&file_lock);
    return -1;
  }

  int result = fd_acquire (file_opened);
  lock_release (&file_lock);
  return result;
}

static int
handle_filesize (const int fd_id) 
{
  if (!fd_is_valid (fd_id))
  {
    handle_exit (-1);
  }
   
  lock_acquire (&file_lock);
  int result = file_length (fd_get (fd_id));
  lock_release (&file_lock);
  return result;
}

static int 
handle_read (const int fd_id, char *buf, const int size) 
{
  if (!is_valid_memory_filecheck (buf) || !is_user_vaddr (buf + size))
  {
    handle_exit (-1);
  }

  lock_acquire (&file_lock);
  if (fd_id == STDIN_FILENO)
  {
    int i;
    for (i = 0; i < size; ++i)
    {
      buf[i] = input_getc ();
    }
    lock_release (&file_lock);
    return size;
  }

  if (!fd_is_valid (fd_id))
  {
    lock_release (&file_lock);
    return -1;
  }

  struct file *fp = fd_get (fd_id);
  int result = file_read (fp, buf, size);

  lock_release (&file_lock);
  return result;
}

static int 
handle_write (const int fd_id, const char *buf, const int size)
{
  if (!is_valid_memory_filecheck (buf) || !is_user_vaddr (buf + size))
  {
    handle_exit (-1);
  }

  lock_acquire (&file_lock);
  if (fd_id == STDOUT_FILENO)
  {
    putbuf (buf, size);
    lock_release (&file_lock);
    return size;
  }
  
  if (!fd_is_valid (fd_id))
  {
    lock_release (&file_lock);
    return -1;
  }

  int result = file_write (fd_get (fd_id), buf, size);
  lock_release (&file_lock);
  return result;
}

static int
handle_seek (const int fd_id, const int position) 
{
  if (!fd_is_valid (fd_id))
  {
    return -1;
  }

  lock_acquire (&file_lock);
  file_seek (fd_get (fd_id), position);
  lock_release (&file_lock);
  return 0;
}

static int
handle_tell (const int fd_id) 
{
  if (!fd_is_valid (fd_id))
  {
    return -1;
  }

  lock_acquire (&file_lock);
  int result = file_tell (fd_get (fd_id));
  lock_release (&file_lock);
  return result;
}

static int
handle_close (const int fd_id)
{
  if (!fd_is_valid (fd_id))
  {
    handle_exit (-1);
  }
  
  lock_acquire (&file_lock);
  fd_release (fd_id);
  lock_release (&file_lock);
  return 0;
}

static bool
handle_chdir (const char *path)
{
  if (!is_valid_memory_filecheck (path))
  {
    handle_exit (-1);
  }

  struct file *file = filesys_open (path);
  if (file == NULL)
  {
    return false;
  }

  thread_set_pwd (thread_current (), dir_open (file_get_inode (file)));
  file_close (file);
  return true;
}

static bool
handle_mkdir (const char *path)
{
  if (!is_valid_memory_filecheck (path))
  {
    handle_exit (-1);
  }

  return filesys_mkdir (path);
}

static bool
handle_readdir (int fd_id, char *name)
{
  if (!fd_is_valid (fd_id))
  {
    handle_exit (-1);
  }
  
  if (!is_valid_memory_filecheck (name))
  {
    handle_exit (-1);
  }
  
  struct dir *dir = fd_get_dir (fd_id);
  if (dir == NULL)
  {
    return false;
  }
  return dir_readdir (dir, name);
}


static bool
handle_isdir (int fd)
{
  return fd_get_dir (fd) != NULL;
}

static int
handle_inumber (int fd)
{
  return inode_get_inumber (file_get_inode (fd_get (fd)));
}

static void
syscall_handler (struct intr_frame *f)
{
  int result;
  
  switch (SYSCALL (f->esp))
    {
    case SYS_HALT: result = handle_halt (); break;
    case SYS_EXIT: result = handle_exit (ARG0(f->esp)); break;
    case SYS_EXEC: result = handle_exec (ARG0(f->esp)); break;
    case SYS_WAIT: result = handle_wait (ARG0(f->esp)); break;

    case SYS_CREATE: result = handle_create (ARG0(f->esp), ARG1(f->esp)); break;
    case SYS_REMOVE: result = handle_remove (ARG0(f->esp)); break;
    case SYS_OPEN: result = handle_open (ARG0(f->esp)); break;
    case SYS_FILESIZE: result = handle_filesize (ARG0(f->esp)); break;
    case SYS_READ: result = handle_read (ARG0(f->esp), ARG1(f->esp), ARG2(f->esp)); break;
    case SYS_WRITE: result = handle_write (ARG0(f->esp), ARG1(f->esp), ARG2(f->esp)); break;
    case SYS_SEEK: result = handle_seek (ARG0(f->esp), ARG1(f->esp)); break;
    case SYS_TELL: result = handle_tell (ARG0(f->esp)); break;
    case SYS_CLOSE: result = handle_close (ARG0(f->esp)); break;

    case SYS_CHDIR: result = handle_chdir (ARG0(f->esp)); break;
    case SYS_MKDIR: result = handle_mkdir (ARG0(f->esp)); break;
    case SYS_READDIR: result = handle_readdir (ARG0(f->esp), ARG1(f->esp)); break;
    case SYS_ISDIR: result = handle_isdir (ARG0(f->esp)); break;
    case SYS_INUMBER: result = handle_inumber (ARG0(f->esp)); break;
        
    default: printf ("unknown system call!\n"); handle_halt ();
    }

  f->eax = result;
}

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
  lock_init (&file_lock);
}
