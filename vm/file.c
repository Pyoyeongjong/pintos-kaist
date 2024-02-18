/* file.c: Implementation of memory backed file object (mmaped object). */

#include "vm/vm.h"
#include "vm/uninit.h"
#include "threads/vaddr.h"
#include "userprog/process.h"

static bool file_backed_swap_in (struct page *page, void *kva);
static bool file_backed_swap_out (struct page *page);
static void file_backed_destroy (struct page *page);

/* DO NOT MODIFY this struct */
static const struct page_operations file_ops = {
	.swap_in = file_backed_swap_in,
	.swap_out = file_backed_swap_out,
	.destroy = file_backed_destroy,
	.type = VM_FILE,
};

/* The initializer of file vm */
void
vm_file_init (void) {
}

/* Initialize the file backed page */
bool
file_backed_initializer (struct page *page, enum vm_type type, void *kva) {
	/* Set up the handler */
	page->operations = &file_ops;
	struct file_page *file_page = &page->file;
    struct lazy_load_aux* aux = page->uninit.aux;

    file_page->ofs = aux->ofs;
    file_page->page_read_bytes = aux->page_read_bytes;
    file_page->page_zero_bytes = aux->page_zero_bytes;
    file_page->file = file_reopen(aux->file);

    //printf(" hi fbi ");
    return true;
}

/* Swap in the page by read contents from the file. */
static bool
file_backed_swap_in (struct page *page, void *kva) {
	struct file_page *file_page UNUSED = &page->file;
}

/* Swap out the page by writeback contents to the file. */
static bool
file_backed_swap_out (struct page *page) {
	struct file_page *file_page UNUSED = &page->file;
}

/* Destory the file backed page. PAGE will be freed by the caller. */
static void
file_backed_destroy (struct page *page) {
	struct file_page *file_page UNUSED = &page->file;
}

/* Do the mmap */
void *
do_mmap (void *addr, size_t length, int writable,
		struct file *file, off_t offset) {

    // The entire file is mapped into consecutive virtual pages!
    
    void *tmp_addr = addr;
    off_t tmp_ofs = offset;
    off_t file_len = file_length(file);
    int page_read_bytes, page_zero_bytes;
    
    int page_count = 0;
    
    while( page_count * PGSIZE < length ){
        struct lazy_load_aux* laux = (struct lazy_load_aux*)malloc(sizeof(struct lazy_load_aux));
        laux->ofs = tmp_ofs;
        if(tmp_ofs < file_len)
            page_read_bytes = (file_len - tmp_ofs < PGSIZE) ? file_len-tmp_ofs : PGSIZE;
        else
            page_read_bytes = 0;
        page_zero_bytes = PGSIZE - page_read_bytes;
        laux->page_read_bytes = page_read_bytes;
        laux->page_zero_bytes = page_zero_bytes;
        laux->file = file_reopen(file);
        if(!vm_alloc_page_with_initializer(VM_FILE, tmp_addr, writable, lazy_load_segment, laux)){
            // TODO: if fail during alloc_page, allcated_pages should be free
            return NULL;
        }

        tmp_ofs += PGSIZE;
        tmp_addr += PGSIZE;
        page_count++;
    }
    return addr;
}

/* Do the munmap */
void
do_munmap (void *addr) {
    
    /*
    // It is for VM_FILE
    struct supplemental_page_table *spt = &thread_current()->spt;
    struct page* spage =spt_find_page(spt, addr);

    // ASSERT( spage->operations->type == VM_FILE )
    if(spage->operations->type != VM_FILE)
        return;

    struct file* file = spage->file.file;
    file_seek();
    
    //struct file* f = 

    printf(" hi do_munmap ");

    file_close(); // here!
    */
}

//_close

















