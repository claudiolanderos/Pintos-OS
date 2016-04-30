#include <stdio.h>
#include <stdbool.h>
#include "userprog/fd.h"
#include "filesys/directory.h"
#include "filesys/file.h"
#include "filesys/filesys.h"
#include "filesys/inode.h"

struct file_descriptor
{
  struct file *file;
  struct dir *dir;
  struct thread *owner;
  bool status;
};

struct file_descriptor fd_list[FDSIZE];
int fd_cnt, empty_fd_cnt, empty_fd_list[FDSIZE];

bool
fd_is_valid (int fd_id)
{
  if (fd_id < 3 || fd_id >= fd_cnt)
  {
    return false;
  }
  if (fd_list[fd_id].status == false)
  {
    return false;
  }
  if (fd_list[fd_id].owner->tid != thread_current()->tid)
  {
    return false;
  }
  return true;
}

void
fd_init (void)
{
  fd_cnt = 3;
  empty_fd_cnt = 0;
}

int
fd_acquire (struct file *file)
{
  int fd_id;
  if (empty_fd_cnt > 0)
  {
    fd_id = empty_fd_list[--empty_fd_cnt];
  }
  else
  {
    fd_id = fd_cnt++;
  }

  if (inode_isdir (file_get_inode (file)))
  {
    fd_list[fd_id].dir = dir_open (file_get_inode (file));
  }
  else
  {
    fd_list[fd_id].dir = NULL;
  }
  
  fd_list[fd_id].file = file;
  fd_list[fd_id].owner = thread_current ();
  fd_list[fd_id].status = true;
  return fd_id;
}

void fd_release (int fd_id)
{
  file_close (fd_list[fd_id].file);
  fd_list[fd_id].status = false;
  empty_fd_list[empty_fd_cnt++] = fd_id;
}

struct file *
fd_get (int fd_id)
{
  return fd_list[fd_id].file;
}

struct dir *
fd_get_dir (int fd_id)
{
  return fd_list[fd_id].dir;
}

void
fd_process_exit (struct thread *t)
{
  int i;
  for (i = 3; i < fd_cnt; ++i)
  {
    if (fd_list[i].owner->tid == t->tid &&
        fd_list[i].status)
    {
      fd_release (i);
    }
  }
}
