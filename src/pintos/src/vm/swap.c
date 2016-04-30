#include "vm/page.h"
#include "vm/swap.h"

void swap_d_write (struct disk *d, int sec_num, uint8_t *page) {
	int i;
	for (i=0;i<BYTE__;i++) {
		disk_write(d,sec_num,page);
		sec_num++;
		page += P_SIZE;
	}
}

void swap_d_read (struct disk *d, int sec_num, uint8_t *page) {
	int i;
	for(i=0;i<BYTE__;i++) {
		disk_read(d,sec_num,page);
		sec_num++;
		page += P_SIZE;
	}
}
int find_empty (void) {
	int i;
	for(i=0; i<MAXDD;i++) {
		if(d_use[i]==false) {
			d_use[i]=true;
			return i*BYTE__;
		}
	}
	ASSERT(1 == 0 );
	return -1;
}

void swap_out (struct page_table *pte, uint8_t *kpage) 
{
	int sec_num = find_empty();
	struct disk *d;

	ASSERT(sec_num != -1);
	pte->swp = sec_num;
	pte->swap = true;
	pte->kpage = NULL;
	d = disk_get(1,1);
	swap_d_write(d, sec_num, kpage);
}

void swap_in (struct page_table *pte, uint8_t *addr)
{
	struct thread *t = thread_current();
	struct disk *d;
	uint8_t *kpage;

	kpage = palloc_get_page(PAL_USER | PAL_ZERO);
	if(kpage == NULL) 
		kpage = victim();
	memset (kpage, 0,4096);
	d = disk_get(1,1);
	swap_d_read(d, pte->swp,kpage);
	d_use[pte->swp/8] = false;
	pte->swap = false;
	pte->kpage = kpage;
	make_frame_table (addr, t);
	pagedir_set_page(t->pagedir, addr, kpage, true);
}
