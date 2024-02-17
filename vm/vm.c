/* vm.c: Generic interface for virtual memory objects. */

#include "threads/malloc.h"
#include "threads/vaddr.h"
#include "vm/vm.h"
#include "vm/inspect.h"

// delete this inculde after testing

/* Initializes the virtual memory subsystem by invoking each subsystem's
 * intialize codes. */
struct list frame_table;
void
vm_init (void) {
	vm_anon_init ();
	vm_file_init ();
#ifdef EFILESYS  /* For project 4 */
	pagecache_init ();
#endif
	register_inspect_intr ();
	/* DO NOT MODIFY UPPER LINES. */
	/* TODO: Your code goes here. */
    list_init(&frame_table);
}

/* Get the type of the page. This function is useful if you want to know the
 * type of the page after it will be initialized.
 * This function is fully implemented now. */
enum vm_type
page_get_type (struct page *page) {
	int ty = VM_TYPE (page->operations->type);
	switch (ty) {
		case VM_UNINIT:
			return VM_TYPE (page->uninit.type);
		default:
			return ty;
	}
}

/* Helpers */
static struct frame *vm_get_victim (void);
static bool vm_do_claim_page (struct page *page);
static struct frame *vm_evict_frame (void);
static bool install_page (void *upage, void *kpage, bool writable);

/* Create the pending page object with initializer. If you want to create a
 * page, do not create it directly and make it through this function or
 * `vm_alloc_page`. */
bool
vm_alloc_page_with_initializer (enum vm_type type, void *upage, bool writable,
		vm_initializer *init, void *aux) {

	ASSERT (VM_TYPE(type) != VM_UNINIT)

	struct supplemental_page_table *spt = &thread_current ()->spt;

	/* Check wheter the upage is already occupied or not. */
	if (spt_find_page (spt, upage) == NULL) {
		/* TODO: Create the page, fetch the initialier according to the VM type,
		 * TODO: and then create "uninit" page struct by calling uninit_new. You
		 * TODO: should modify the field after calling the uninit_new.*/
        struct page* page = (struct page*)malloc(sizeof(struct page));
		bool (*initializer)(struct page *, enum vm_type, void *);
        if(type == VM_ANON)
        {
            initializer = anon_initializer;
        }
        else if(type == VM_FILE)
        {
            initializer = file_backed_initializer;
        }
        else
        {
            initializer = NULL;
        }
            
        uninit_new(page, upage, init, type, aux, initializer);
        page->writable = writable;
        page->type = VM_UNINIT;

		/* TODO: Insert the page into the spt. */
        if(!spt_insert_page(spt, page))
            goto err;
	}
    return true;
err:
	return false;
}

/* Find VA from spt and return page. On error, return NULL. */
struct page *
spt_find_page (struct supplemental_page_table *spt UNUSED, void *va UNUSED) {
	struct page *page = NULL;
	/* TODO: Fill this function. */
    struct list* spt_list = &spt->spt_list;
    struct list_elem* e;
    for(e = list_begin(spt_list); e!=list_end(spt_list);e = list_next(e)){
        struct page* p = list_entry(e, struct page, spt_elem);
        if(p->va == va){
            page = p;
            break;
        }
    }

	return page;
}

/* Insert PAGE into spt with validation. */
bool
spt_insert_page (struct supplemental_page_table *spt UNUSED,
		struct page *page UNUSED) {
	int succ = false;
	/* TODO: Fill this function. */
    struct list* spt_list = &spt->spt_list;
    list_push_back(spt_list, &page->spt_elem);
    succ = true;

	return succ;
}

void
spt_remove_page (struct supplemental_page_table *spt, struct page *page) {
    list_remove(&page->spt_elem);
	vm_dealloc_page (page);
	return true;
}

/* Get the struct frame, that will be evicted. */
static struct frame *
vm_get_victim (void) {
	struct frame *victim = NULL;
	 /* TODO: The policy for eviction is up to you. */

    // FIFO..
    if(!list_empty(&frame_table)){
        victim = list_entry(list_pop_front(&frame_table),struct frame, elem);
    }

	return victim;
}

/* Evict one page and return the corresponding frame.
 * Return NULL on error.*/
static struct frame *
vm_evict_frame (void) {
	struct frame *victim UNUSED = vm_get_victim ();
	/* TODO: swap out the victim and return the evicted frame. */

    if(victim->page != NULL)
        swap_out(victim->page);

	return victim;
}

/* palloc() and get frame. If there is no available page, evict the page
 * and return it. This always return valid address. That is, if the user pool
 * memory is full, this function evicts the frame to get the available memory
 * space.*/
static struct frame *
vm_get_frame (void) {
	struct frame *frame = NULL;
	/* TODO: Fill this function. */

    void *kva = palloc_get_page(PAL_USER | PAL_ZERO);
    if(kva == NULL)
        frame = vm_evict_frame();
    else{
        frame = (struct frame*)malloc(sizeof(struct frame));
        frame->kva = kva;
    }
    list_push_back(&frame_table, &frame->elem); 
    frame->page = NULL;

	ASSERT (frame != NULL);
    ASSERT (frame->page == NULL);
	return frame;
}

/* Growing the stack. */
static void
vm_stack_growth (void *addr UNUSED) {
}

/* Handle the fault on write_protected page */
static bool
vm_handle_wp (struct page *page UNUSED) {
}

/* Return true on success */
bool
vm_try_handle_fault (struct intr_frame *f UNUSED, void *addr UNUSED,
		bool user UNUSED, bool write UNUSED, bool not_present UNUSED) {
	struct supplemental_page_table *spt UNUSED = &thread_current ()->spt;
	struct page *page = NULL;
	/* TODO: Validate the fault */
	/* TODO: Your code goes here */
    page = spt_find_page(spt, pg_round_down(addr));
    if(page == NULL)
        return false;
    if(page->frame != NULL)
        return false;

	return vm_do_claim_page (page);
}

/* Free the page.
 * DO NOT MODIFY THIS FUNCTION. */
void
vm_dealloc_page (struct page *page) {
	destroy (page);
	free (page);
}

/* Claim the page that allocate on VA. */
bool
vm_claim_page (void *va UNUSED) {
	struct page *page = NULL;
	/* TODO: Fill this function */
    struct supplemental_page_table* spt = &thread_current()->spt;
    page = spt_find_page(spt, va);
    if(page == NULL){
        return false;
    }
	return vm_do_claim_page (page);
}

/* Claim the PAGE and set up the mmu. */
static bool
vm_do_claim_page (struct page *page) {
	struct frame *frame = vm_get_frame ();

    /* Set links */
	frame->page = page;
	page->frame = frame;

	/* TODO: Insert page table entry to map page's VA to frame's PA. */
    struct thread* curr = thread_current ();
    //malloc
    
    if(!install_page(page->va, frame->kva, page->writable)){
        // TODO: destroy page? or do nothing?
        printf(" 4/ ");
        return false;
    }
	return swap_in (page, frame->kva);
}
//uninit_initialize

/* Initialize new supplemental page table */
void
supplemental_page_table_init (struct supplemental_page_table *spt UNUSED) {
   list_init(&spt->spt_list); 
   return;
}

/* Copy supplemental page table from src to dst */
bool
supplemental_page_table_copy (struct supplemental_page_table *dst UNUSED,
		struct supplemental_page_table *src UNUSED) {
    if(dst == NULL || src == NULL)
        return false;
    struct list* dst_list = &dst->spt_list;
    struct list* src_list = &src->spt_list;

    // Should do Deep copy
    struct list_elem *e;
    for(e = list_begin(src_list); e != list_end(src_list); e = list_next(e)){
        struct page* src_page = list_entry(e, struct page, spt_elem);
        struct page* dst_page;
        if(src_page == NULL)
            continue;
        //struct page* dst_page = (struct page*)malloc(sizeof(struct page));
        vm_alloc_page(src_page->type, src_page->va, src_page->writable);
        dst_page = spt_find_page(dst, src_page->va);
        memcpy(dst_page, src_page, sizeof(struct page));    
        if(src_page->type == VM_UNINIT){
            int a = NULL; // Nothing~
        }
        else if(src_page->type == VM_ANON || src_page->type == VM_FILE){
            if(!vm_claim_page(dst_page->va))
                return false;
            memcpy(dst_page->frame->kva, src_page->frame->kva, PGSIZE);
        }
        //memcpy(dst_page, src_page, sizeof(struct page));

        list_push_back(dst_list, &dst_page->spt_elem);
    }
    return true;
}

/* Free the resource hold by the supplemental page table */
void
supplemental_page_table_kill (struct supplemental_page_table *spt UNUSED) {
	/* TODO: Destroy all the supplemental_page_table hold by thread and
	 * TODO: writeback all the modified contents to the storage. */
    if(spt == NULL)
        return;
    struct list* spt_list = &spt->spt_list;
    struct list_elem* e;

    // we don't have to kill page->frame memory here because pml4_destroy do this! 
    for(e = list_begin(spt_list); e != list_end(spt_list); e = list_next(e)){
        struct page* p = list_entry(e, struct page, spt_elem);
        spt_remove_page(spt, p); 
    }
    return;
}

static bool
install_page (void *upage, void *kpage, bool writable) {
	struct thread *t = thread_current ();

	/* Verify that there's not already a page at that virtual
	 * address, then map our page there. */
	return (pml4_get_page (t->pml4, upage) == NULL
			&& pml4_set_page (t->pml4, upage, kpage, writable));
}

































