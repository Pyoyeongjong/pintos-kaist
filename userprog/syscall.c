#include "userprog/syscall.h"
#include "userprog/process.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/loader.h"
#include "threads/vaddr.h"
#include "lib/string.h"
#include "threads/init.h"
#include "threads/palloc.h"
#include "userprog/gdt.h"
#include "threads/flags.h"
#include "filesys/filesys.h"
#include "filesys/file.h"
#include "threads/thread.h"
#include "intrinsic.h"
#include "vm/vm.h"
void syscall_entry (void);
void syscall_handler (struct intr_frame *);
typedef int pid_t;
/* System call.
 *
 * Previously system call services was handled by the interrupt handler
 * (e.g. int 0x80 in linux). However, in x86-64, the manufacturer supplies
 * efficient path for requesting the system call, the `syscall` instruction.
 *
 * The syscall instruction works by reading the values from the the Model
 * Specific Register (MSR). For the details, see the manual. */

#define MSR_STAR 0xc0000081         /* Segment selector msr */
#define MSR_LSTAR 0xc0000082        /* Long mode SYSCALL target */
#define MSR_SYSCALL_MASK 0xc0000084 /* Mask for the eflags */

void
user_memory_access(void* vaddr){
    struct thread *curr = thread_current(); 
    /* Project 2
    printf(" hi user_memory_access, vaddr=%x ",vaddr);
    if(vaddr == NULL || is_kernel_vaddr((uint64_t)vaddr) || pml4_get_page(curr->pml4, vaddr)==NULL){
        printf(" err2, vaddr=%x ",vaddr);
        _exit(-1);
    }
    */
    // Project 3
    if(vaddr == NULL || is_kernel_vaddr((uint64_t)vaddr)){
    //    printf(" err3, vaddr=%x ",vaddr);
        //printf(" here? ");
        _exit(-1);
    }
    //
    return;
}

void
syscall_init (void) {
	write_msr(MSR_STAR, ((uint64_t)SEL_UCSEG - 0x10) << 48  |
			((uint64_t)SEL_KCSEG) << 32);
	write_msr(MSR_LSTAR, (uint64_t) syscall_entry);

	/* The interrupt service rountine should not serve any interrupts
	 * until the syscall_entry swaps the userland stack to the kernel
	 * mode stack. Therefore, we masked the FLAG_FL. */
	write_msr(MSR_SYSCALL_MASK,
			FLAG_IF | FLAG_TF | FLAG_DF | FLAG_IOPL | FLAG_AC | FLAG_NT);
}

/* The main system call interface */
void
syscall_handler (struct intr_frame *f UNUSED) {
	
    // TODO: Your implementation goes here.
    // rdi, rsi, rdx, r10, r8, r9 order
    uint64_t rax = f->R.rax;

    switch(rax){
        case SYS_HALT:
            _halt();
            break;
        case SYS_EXIT:
            _exit(f->R.rdi);
            break;
        case SYS_FORK:
            f->R.rax = _fork(f->R.rdi, f);
            break;
        case SYS_EXEC:
            f->R.rax = _exec(f->R.rdi);
            break;
        case SYS_WAIT:
            f->R.rax = _wait(f->R.rdi);
            break;
        case SYS_CREATE:
            f->R.rax = _create(f->R.rdi, f->R.rsi);
            break;
        case SYS_REMOVE:
            f->R.rax = _remove(f->R.rdi);
            break;
        case SYS_OPEN:
            f->R.rax = _open(f->R.rdi);
            break;
        case SYS_FILESIZE:
            f->R.rax = _filesize(f->R.rdi);
            break;
        case SYS_READ:
            f->R.rax = _read(f->R.rdi, f->R.rsi, f->R.rdx);
            break;
        case SYS_WRITE:
            f->R.rax = _write(f->R.rdi, f->R.rsi, f->R.rdx);
            break;
        case SYS_SEEK:
            _seek(f->R.rdi, f->R.rsi);            
            break;
        case SYS_TELL:
            f->R.rax = _tell(f->R.rdi);
            break;
        case SYS_CLOSE:
            _close(f->R.rdi);
            break;
        case SYS_DUP2:
            f->R.rax = _dup2(f->R.rdi, f->R.rsi);
            break;
        case SYS_MMAP:
            f->R.rax = _mmap(f->R.rdi, f->R.rsi, f->R.rdx, f->R.r10, f->R.r8); 
            break;
        case SYS_MUNMAP:
            _munmap(f->R.rdi);
            break;
        default:
            _exit(-1);
            break;
    }
}

void*
_mmap(void *addr, size_t length, int writable, int fd, off_t offset){


    //printf(" hi_mmap ");
    if(is_kernel_vaddr(addr) || addr == NULL || addr == 0 || ((int)addr % PGSIZE) != 0 || (long long)length <= 0 || ((offset % PGSIZE) != 0))
        return NULL;

    struct file* file = thread_fd_find(fd);
    if(file == NULL)
        return NULL;
    
    return do_mmap(addr, length, writable, file, offset);
}

void 
_munmap(void *addr){
        
    //file_write_at(file, buffer, size, file_ofs);
    if(addr == NULL || addr == 0)
        return;
    do_munmap(pg_round_down(addr));
}

void
_halt(void){
    power_off();
}

void
_exit(int status){
    struct thread *curr = thread_current();
    curr->exit_status = status;
    printf("%s: exit(%d)\n",curr->name, curr->exit_status);
    thread_exit();
}

pid_t
_fork (const char *thread_name, struct intr_frame *f){
    user_memory_access(thread_name);
    return process_fork(thread_name, f);
}

int
_exec (const char *cmd_line){

    //printf("exec line=%s ",cmd_line);

    user_memory_access(cmd_line);
    char *fn_copy;
    fn_copy = palloc_get_page(0);
    if(fn_copy == NULL)
        _exit(-1);

    strlcpy (fn_copy, cmd_line, PGSIZE);

    if(process_exec(fn_copy)==-1){
        _exit(-1);
    }
    NOT_REACHED();
    return 0;

}

int
_wait (pid_t pid){
    return process_wait(pid);

}
//write();

bool
_create (const char *file, unsigned initial_size){ 
    user_memory_access(file);
    if(file == NULL)
        _exit(-1);
    return filesys_create(file, initial_size);
}

bool
_remove (const char *file){
    user_memory_access(file);
    return filesys_remove(file);
}

int
_open (const char *file){

    user_memory_access(file);
    struct file *f = filesys_open(file);
    if(f == NULL){
        return -1;

    }
    int fdi = thread_fd_insert(f);
    if(fdi == -1){
        file_close(f);

    }
    return fdi;
}
//process_exit


int
_filesize (int fd){
    struct file *f = thread_fd_find(fd);    
    if(f == NULL || f == STDIN || f == STDOUT)
        return -1;
    return file_length(f);
}

int 
_read(int fd, void *buffer, unsigned size){
    //printf(" LOG B/ ");

    struct thread *curr = thread_current();
    user_memory_access(buffer);
    user_memory_access(buffer + size-1);
    
    // TODO: when writing in unpaged stack.. what should i do?

    //check buffer is writable - Project 3 Stack Growth
    struct supplemental_page_table *spt = &thread_current()->spt;
    struct page* ps = spt_find_page(spt, buffer);
    struct page* pe = spt_find_page(spt, buffer+size-1);
    if(spt_find_page(spt, buffer)->writable == false || 
            spt_find_page(spt, buffer + size-1)->writable == false){
        //printf(" LOG A/ ");
        _exit(-1);
    }
    //printf(" LOG C/ ");
    //printf(" buffer=%x ",buffer);

     
    unsigned int read_size;
    if(fd < 0 || fd >=FD_LIMIT)
        return -1;

    struct file *f = thread_fd_find(fd);
    if (f == STDIN){
        char c;
        char* buff = buffer;
        for (int read_size = 0; read_size < size; read_size++){
            c = input_getc();
            buff[read_size]=c;
            if(c='\0')
                break;
        }   
        return read_size;
    }
    else if (f == STDOUT){
        return -1;
    }
    else{
         if(f == NULL)
             return -1;
         read_size = file_read(f, buffer, size); 
         return read_size;
    }
}

int
_write(int fd, const void *buffer, unsigned size){

    //printf(" @@write@@ ");
    user_memory_access(buffer);
    user_memory_access(buffer + size-1);
    if(fd < 0 || fd >=FD_LIMIT)
        return -1;
    struct file *f = thread_fd_find(fd);

    if(f == STDIN){
        return -1;
    }
    else if(f == STDOUT){
        putbuf(buffer, size);
        return size;
    }
    else{
        unsigned int write_size;
        if(f == NULL){
            return -1;
        }
        write_size = file_write(f, buffer, size);
        return write_size;
    }
}

void
_seek (int fd, unsigned position){
    if(fd < 0 || fd >=FD_LIMIT)
        return;
    struct file *f = thread_fd_find(fd);
    if (f == NULL || f == STDIN || f == STDOUT)
        return;
    file_seek(f, position); 
}

unsigned
_tell (int fd){
    if(fd < 0 || fd >=FD_LIMIT)
        return -1;
    struct file *f = thread_fd_find(fd);
    if(f == NULL || f == STDIN || f == STDOUT)
        return -1;
    return file_tell(f);
}

void
_close (int fd){
    if(fd < 0 || fd >=FD_LIMIT)
        return;
    struct file *f = thread_fd_find(fd);
    if(f == NULL)
        return;
    thread_fd_delete(fd);
    if(f == STDIN || f == STDOUT){
        return;
    }

    file_close(f);
    return;
}
//close


int
thread_fd_insert(struct file *f){
    struct thread *curr = thread_current();
    int fdi = 0;
    while(fdi < FD_LIMIT && curr->fdTable[fdi]!=NULL){
        fdi++;
    }
    if(fdi >= FD_LIMIT)
        return -1;
    
    curr->fdTable[fdi] = f;
    return fdi;
}

void
thread_fd_delete(int fd){
    struct thread *curr = thread_current();
    if (fd < 0 || fd >= FD_LIMIT)
        return;
    curr->fdTable[fd] = NULL;
    return;

}

struct file *
thread_fd_find(int fd){
    struct thread *curr = thread_current();
    if(fd < 0 || fd>=FD_LIMIT)
        return NULL;
    return curr->fdTable[fd];
}


int
_dup2(int oldfd, int newfd){
    
    struct thread *curr = thread_current();
    if(oldfd < 0 || oldfd >= FD_LIMIT || newfd < 0 || newfd >= FD_LIMIT)
        return -1;

    struct file *oldf = thread_fd_find(oldfd);

    if(oldf == NULL)
        return -1;
    if(oldfd == newfd){
        return newfd;
    }
    
    if(oldf != STDIN && oldf != STDOUT)
        oldf->dup2_count += 1;
     
    _close(newfd);
    curr->fdTable[newfd] = oldf;
    return newfd;
    
    return 0;
}




















