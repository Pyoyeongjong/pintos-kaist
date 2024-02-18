#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

#include <stdbool.h>
#include "threads/interrupt.h"
#include "filesys/file.h"
#include "lib/stddef.h"
#define FD_LIMIT 0x200
//thread_create
//process_exit

void syscall_init (void);

void user_memory_access(void*);
void* _mmap(void *addr, size_t length, int writable, int fd, off_t offset);
void _munmap(void *addr);
void _halt(void);
void _exit(int status);
int _fork(const char *thread_name, struct intr_frame *if_);
int _exec(const char *cmd_line);
int _wait(int pid);
bool _create(const char *file, unsigned initial_size);
bool _remove(const char *file);
int _open(const char *file);
int _filesize(int fd);
int _read(int fd, void *buffer, unsigned size);
int _write(int fd, const void *buffer, unsigned size);
void _seek(int fd, unsigned position);
unsigned _tell(int fd);
void _close(int fd);

int thread_fd_insert(struct file *);
void thread_fd_delete(int);
struct file* thread_fd_find(int);

#endif /* userprog/syscall.h */
