/* anon.c: Implementation of page for non-disk image (a.k.a. anonymous page). */

#include "vm/vm.h"
#include "devices/disk.h"
#include "threads/malloc.h"
#include "threads/vaddr.h"
#include "lib/kernel/bitmap.h"

/* DO NOT MODIFY BELOW LINE */
static struct disk *swap_disk;
static bool anon_swap_in (struct page *page, void *kva);
static bool anon_swap_out (struct page *page);
static void anon_destroy (struct page *page);

/* DO NOT MODIFY this struct */
static const struct page_operations anon_ops = {
	.swap_in = anon_swap_in,
	.swap_out = anon_swap_out,
	.destroy = anon_destroy,
	.type = VM_ANON,
};
/* Initialize the data for anonymous pages */


struct bitmap *swap_table;
const size_t SECTORS_PER_PAGE = PGSIZE / DISK_SECTOR_SIZE;
void
vm_anon_init (void) {
	/* TODO: Set up the swap_disk. */
	swap_disk = disk_get(1, 1); 
    size_t swap_size = disk_size(swap_disk) / SECTORS_PER_PAGE;
    swap_table = bitmap_create(swap_size);
    return;
}
//vm_alloc_page_with_initializer
/* Initialize the file mapping */
bool
anon_initializer (struct page *page, enum vm_type type, void *kva) {
	/* Set up the handler */
	page->operations = &anon_ops;
	struct anon_page *anon_page = &page->anon;
    anon_page->disk_index = -1;
    
    //printf(" hi anon ");
    return true;    
}

/* Swap in the page by read contents from the swap disk. */
static bool
anon_swap_in (struct page *page, void *kva) {
	struct anon_page *anon_page = &page->anon;

    int disk_no = anon_page->disk_index;

    if(bitmap_test(swap_table, disk_no) == false)
        return false;

    for(int i=0; i<SECTORS_PER_PAGE; i++){
        disk_read(swap_disk, disk_no * SECTORS_PER_PAGE + i, page->frame->kva + i * DISK_SECTOR_SIZE);
    }
    bitmap_set(swap_table, disk_no, false);
    return true;
}

/* Swap out the page by writing contents to the swap disk. */
static bool
anon_swap_out (struct page *page) {
	struct anon_page *anon_page = &page->anon;


    size_t disk_no = bitmap_scan(swap_table, 0, 1, false);
    if(disk_no == BITMAP_ERROR)
        return false;

    for(int i=0; i<SECTORS_PER_PAGE; i++){
        disk_write(swap_disk, disk_no * SECTORS_PER_PAGE + i, page->va + DISK_SECTOR_SIZE * i);
    }

    bitmap_set(swap_table, disk_no, true);
    pml4_clear_page(thread_current()->pml4, page->va);
    anon_page->disk_index = disk_no;
    page->frame->page = NULL;
    page->frame = NULL;
    return true;
}

/* Destroy the anonymous page. PAGE will be freed by the caller. */
static void
anon_destroy (struct page *page) {
    uint64_t* pml4 = thread_current()->pml4;
	struct anon_page *anon_page = &page->anon;
    struct frame* frame = page->frame;

    pml4_clear_page(pml4, page->va);
    

    return;
}
