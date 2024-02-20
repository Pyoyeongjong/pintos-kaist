#ifndef USERPROG_PROCESS_H
#define USERPROG_PROCESS_H

#include "threads/thread.h"

struct lazy_load_aux{
    int ofs;
    int page_read_bytes;
    int page_zero_bytes;
    struct file* file;
    //mmap
    bool mmap_head;
    int mmap_len;
};
tid_t process_create_initd (const char *file_name);
tid_t process_fork (const char *name, struct intr_frame *if_);
void put_args_into_stack(int argc, char **argv, void **rsp,struct intr_frame *if_ );
int process_exec (void *f_name);
int process_wait (tid_t);
void process_exit (void);
void process_activate (struct thread *next);
bool lazy_load_segment(struct page*, void *aux);
struct thread* get_child(tid_t tid);
#endif /* userprog/process.h */
