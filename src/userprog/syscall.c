#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "lib/user/syscall.h"
#include "devices/shutdown.h"
#include "threads/synch.h"
#include "devices/input.h"
#include <list.h>
#define USER_VALID_START 0x08048000

static void syscall_handler (struct intr_frame *);
 void halt (void);
 void exit (int status);
 pid_t exec (const char *cmd_line);
 int wait (pid_t pid);
 bool create (const char *file, unsigned initial_size);
 bool remove (const char *file);
 int open (const char *file);
 int filesize (int fd);
 int read (int fd, void *buffer, unsigned size);
 int write (int fd, const void *buffer, unsigned size);
 void seek (int fd, unsigned position);
 unsigned tell (int fd);
 void close (int fd);

 int count=2;
 struct list files;

 struct file_descriptor{
 int fd;
 const char *file;
 struct file *file_1;
 struct list_elem elem;
 bool closed;
 int parent;
 };

void
syscall_init (void) 
{
  list_init(&files);
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f UNUSED) 
{ 
  check_valid_pointer((void*)(f->esp));
  int fn = *(int*)f->esp;
  switch (fn){
  case SYS_HALT :
  {
    halt();
    break;
  }
  case SYS_EXIT :{
   check_valid_pointer((void*)(f->esp+sizeof(int)));
   int status=*(int*)(f->esp+sizeof(int));
   exit(status);
   break;
  }
  case SYS_EXEC :
  {
   check_valid_pointer((void*)(f->esp+sizeof(int)));
   char* command=(char*)(*((int*)(f->esp+sizeof(int))));
   f->eax=exec(command);
   break;
  }
  case SYS_WAIT :{
   check_valid_pointer((void*)(f->esp+sizeof(int)));
   pid_t pid=*(int *)(f->esp+sizeof(int));
   f->eax=wait(pid);
   break;
  }
  case SYS_CREATE :
  {
   check_valid_pointer((void*)(f->esp+sizeof(int)));
   char* file=(char*)(*((int*)(f->esp+sizeof(int))));
   check_valid_pointer((void*)(f->esp+sizeof(int)+sizeof(char*)));
   unsigned number=*(unsigned*)(f->esp+sizeof(int)+sizeof(char*));
   f->eax=create(file,number);
   break;
  }
  case SYS_REMOVE :
  {
   check_valid_pointer((void*)(f->esp+sizeof(int)));
   char* file=(char*)(*((int*)(f->esp+sizeof(int))));
   f->eax=remove(file);
   break;
  }
  case SYS_OPEN :
  {
   check_valid_pointer((void*)(f->esp+sizeof(int)));
   char* file=(char*)(*((int*)(f->esp+sizeof(int))));
   f->eax=open(file);
   break;
  }
  case SYS_FILESIZE :
  {
  check_valid_pointer((void*)(f->esp+sizeof(int)));
  int fd=*(int*)(f->esp+sizeof(int));
  f->eax=filesize(fd);
  break;
  }
  case SYS_READ :
  {
   check_valid_pointer((void*)(f->esp+sizeof(int)));
   int fd=*(int*)(f->esp+sizeof(int));
   check_valid_pointer((void*)(f->esp+sizeof(int)+sizeof(int)));
   void* buffer=*(char**)(f->esp+sizeof(int)+sizeof(int));
   check_valid_pointer((void*)(f->esp+sizeof(int)+sizeof(int)+sizeof(void*)));
   unsigned size=*(unsigned*)(f->esp+sizeof(int)+sizeof(int)+sizeof(void*));
   f->eax=read(fd,buffer,size);
   break;
  }
  case SYS_WRITE :
  {
   check_valid_pointer((void*)(f->esp+sizeof(int)));
   int fd=*(int*)(f->esp+sizeof(int));
   check_valid_pointer((void*)(f->esp+sizeof(int)+sizeof(int)));
   void* buffer=*(char**)(f->esp+sizeof(int)+sizeof(int));
   check_valid_pointer((void*)(f->esp+sizeof(int)+sizeof(int)+sizeof(void*)));
   unsigned size=*(unsigned*)(f->esp+sizeof(int)+sizeof(int)+sizeof(void*));
   f->eax=write(fd,buffer,size);
   break;
  }
  case SYS_SEEK :{
  check_valid_pointer((void*)(f->esp+sizeof(int)));
  int fd=*(int*)(f->esp+sizeof(int));
  check_valid_pointer((void*)(f->esp+sizeof(int)+sizeof(int)));
  unsigned position = *(unsigned*)(f->esp+sizeof(int)+sizeof(int));
  seek(fd,position);
  break;
  }
  case SYS_TELL :{
   check_valid_pointer((void*)(f->esp+sizeof(int)));
   int fd=*(int*)(f->esp+sizeof(int));
   f->eax=tell(fd);
   break;
  }
  case SYS_CLOSE :{
   check_valid_pointer((void*)(f->esp+sizeof(int)));
   int fd=*(int *)(f->esp+sizeof(int));
   close(fd);
   break;
  }
  }

}

 void halt (void){
   shutdown_power_off();
 }

 pid_t exec (const char *cmd_line){
    check_bad_ptr(cmd_line);
    return  process_execute(cmd_line);
 }

 bool create (const char *file, unsigned initial_size){
    check_bad_ptr(file);
    return filesys_create(file,initial_size);
 }

 bool remove (const char *file){
    return filesys_remove(file);
 }

 int open (const char *file){
   check_bad_ptr(file);
   int number = -1;
   struct file_descriptor *descriptor=malloc(sizeof(*descriptor)); 
   struct file *opened_file;
   opened_file = filesys_open(file);
   if(opened_file != NULL){
     descriptor->file_1 = opened_file;
     descriptor->file = file;
     descriptor->fd = count;
     descriptor->closed =false;
     descriptor->parent =thread_current()->tid;
     number = count;
     count++;
     list_push_back(&files,&descriptor->elem);

   }
   return number;
  
}


 int filesize (int fd){
   if(!list_empty(&files)){
      size_t size1=list_size(&files);
      while(size1>0){
            struct file_descriptor* index_file=list_entry(list_front(&files),struct file_descriptor,elem);
            if(index_file->fd == fd){
                   return file_length(index_file->file_1);

            }
           list_push_back (&files, list_pop_front(&files)) ;
           size1--;
    }

  }
 return 0;
}


 unsigned tell (int fd){
   if(!list_empty(&files)){
         size_t size1=list_size(&files);
         while(size1>0){
               struct file_descriptor* index_file=list_entry(list_front(&files),struct file_descriptor,elem);
               if(index_file->fd == fd){
                    return file_tell(index_file->file_1);

               }
         list_push_back (&files, list_pop_front(&files)) ;
         size1--;
         }
   }
 }

  void close (int fd){
      if(!list_empty(&files)){
          size_t size1=list_size(&files);
          while(size1>0){
                struct file_descriptor* index_file=list_entry(list_front(&files),struct file_descriptor,elem);
                if(index_file->fd == fd){
                     if(index_file->parent != thread_current()->tid){
                          exit(-1);
                      }
                if(index_file->closed==false){
                    index_file->closed=true;
                    file_close(index_file->file_1);
                    break;
                }
               else{
                    exit(-1);
		   }
         }
         list_push_back (&files, list_pop_front(&files)) ;
         size1--;
         }
     }
  }

  void seek (int fd, unsigned position){

   if(!list_empty(&files)){
       size_t size1=list_size(&files);
       while(size1>0){
           struct file_descriptor* index_file=list_entry(list_front(&files),struct file_descriptor,elem);
           if(index_file->fd == fd){
                 file_seek(index_file->file_1,position);
                 break;
           }
           list_push_back (&files, list_pop_front(&files)) ;
            size1--;
      }
    }
 }

 int read (int fd, void *buffer, unsigned size){
     if(size==0){return 0;}
        check_valid_pointer((void *)buffer);
        if(fd == STDIN_FILENO){
           int input = input_getc();
           return input;
       }
        else{
          if(!list_empty(&files)){
             size_t size1=list_size(&files);
             while(size1 >0){
                     struct file_descriptor* index_file=list_entry(list_front(&files),struct file_descriptor,elem);
                     if(index_file->fd == fd){
                          int read_syn =file_read(index_file->file_1,buffer,size);
                          return read_syn;
                     }
             list_push_back (&files, list_pop_front(&files)) ;
              size1--;
              }
    }
 }

 return -1;
 }



int write (int fd, const void *buffer, unsigned size){
     if(size==0)
     {
      return 0;
      }
     check_bad_ptr((char *)buffer);

     if(fd == STDOUT_FILENO){
         putbuf (buffer, size);
     return size;
     }
     else{
       if(!list_empty(&files)){
             size_t size1=list_size(&files);
             while(size1>0){
                     struct file_descriptor* index_file=list_entry(list_front(&files),struct file_descriptor,elem);
                     if(index_file->fd == fd){
                         int write_syn = file_write(index_file->file_1,buffer,size);
                         return write_syn;
                     }
                     list_push_back (&files, list_pop_front(&files)) ;
                     size1--;
              }

       }
    }
 return -1;
 }

 void exit(int status){
   struct thread *current_thread = thread_current();
   printf ("%s: exit(%d)\n", current_thread->name, status);
   struct thread *parent = get_thread_by_tid(current_thread->parent_tid);
   struct child *child;
   struct list_elem *e;
   // check if there is parent waiting for it and return the status to the kernal
     for (e = list_begin (&parent->children); e != list_end (&parent->children); e = list_next (e)){
         child = list_entry(e, struct child, child_element);
         if(child->child_tid == current_thread->tid){
	   break;
         }
     }
   if(child->is_waited_by_parent == true){
      struct list_elem *e;
      struct child * child;
      for (e = list_begin (&parent->children); e != list_end (&parent->children);
       e = list_next (e))
      {
        child = list_entry(e, struct child, child_element);
        if(child->child_tid == current_thread->tid){
          child->final_status = status;
          child->is_exit = true;
          child->is_waited_by_parent=false;
        }
      }
   }
   thread_exit();
}

 int wait (pid_t pid){
  return process_wait(pid);
 }

 void check_valid_pointer(const void *address){
   if(!is_user_vaddr(address)){
        exit(-1);
   }
   if(address < USER_VALID_START){
        exit(-1);
   }
 }

 void check_bad_ptr(const char *address){
   if(pagedir_get_page(thread_current()->pagedir, address) == NULL){
        exit(-1);
   }
 }



