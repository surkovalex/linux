#include <linux/amlogic/instaboot/instaboot.h>
#include <linux/export.h>
#include <linux/atomic.h>
#include <linux/init.h>
#include <linux/printk.h>

atomic_t snapshot_device_available = ATOMIC_INIT(1);

extern int snapshot_read_next(struct snapshot_handle *handle);
int aml_snapshot_read_next(struct snapshot_handle *handle)
{
	return snapshot_read_next(handle);
}
EXPORT_SYMBOL(aml_snapshot_read_next);

extern int snapshot_write_next(struct snapshot_handle *handle);
int aml_snapshot_write_next(struct snapshot_handle *handle)
{
	return snapshot_write_next(handle);
}
EXPORT_SYMBOL(aml_snapshot_write_next);

extern void snapshot_write_finalize(struct snapshot_handle *handle);
void aml_snapshot_write_finalize(struct snapshot_handle *handle)
{
	snapshot_write_finalize(handle);
}
EXPORT_SYMBOL(aml_snapshot_write_finalize);

extern unsigned long snapshot_get_image_size(void);
unsigned long aml_snapshot_get_image_size(void)
{
	return snapshot_get_image_size();
}
EXPORT_SYMBOL(aml_snapshot_get_image_size);

extern int snapshot_image_loaded(struct snapshot_handle *handle);
int aml_snapshot_image_loaded(struct snapshot_handle *handle)
{
	return snapshot_image_loaded(handle);
}
EXPORT_SYMBOL(aml_snapshot_image_loaded);

extern dev_t swsusp_resume_device;
dev_t aml_get_swsusp_resume_device(void)
{
	printk("swsusp_resume_device: %u\n", (unsigned int)swsusp_resume_device);
	return swsusp_resume_device;
}
EXPORT_SYMBOL(aml_get_swsusp_resume_device);

extern void end_swap_bio_read(struct bio *bio, int err);
end_swap_bio_read_p_t aml_get_end_swap_bio_read(void)
{
	end_swap_bio_read_p_t fun_p;
	fun_p = end_swap_bio_read;
	return end_swap_bio_read;
}
EXPORT_SYMBOL(aml_get_end_swap_bio_read);

extern unsigned int nr_free_highpages (void);
unsigned int aml_nr_free_highpages (void)
{
	return nr_free_highpages();
}
EXPORT_SYMBOL(aml_nr_free_highpages);

extern void bio_set_pages_dirty(struct bio *bio);
void aml_bio_set_pages_dirty(struct bio *bio)
{
	bio_set_pages_dirty(bio);
}
EXPORT_SYMBOL(aml_bio_set_pages_dirty);

/*
   in kernel booting process, acquire some memory for device probe,
   which will not be crush when recovery the instaboot image.
*/
static int alloc_permission = 1;
struct reserve_mem{
	unsigned long long startaddr;
	unsigned long long size;
	unsigned int flag;		/* 0: high memory  1:low memory */
	char name[16];			/* limit: device name must < 14; */
};

#define MAX_RESERVE_BLOCK  32

struct reserve_mgr{
	int count;
	unsigned long long start_memory_addr;
	unsigned long long total_memory;
	unsigned long long current_addr_from_low;
	unsigned long long current_addr_from_high;
	struct reserve_mem reserve[MAX_RESERVE_BLOCK];
};

extern struct reserve_mgr *get_reserve_mgr(void);
extern unsigned long long get_reserve_base(void);

void* aml_boot_alloc_mem(size_t size)
{
	phys_addr_t buf;
	struct reserve_mgr* rsv_mgr;
	struct reserve_mem* prm;
	unsigned long addr = 0;
	int i;
	unsigned long long base_addr;

	if (!alloc_permission) {
		pr_err("%s can only be useful in booting time\n", __FUNCTION__);
		return NULL;
	}

	base_addr = get_reserve_base();

	rsv_mgr = get_reserve_mgr();
	if (!rsv_mgr)
		return NULL;

	for (i = 0; i < rsv_mgr->count; i++) {
		prm = &rsv_mgr->reserve[i];
		if ((!(prm->flag & 2)) && prm->size > size) {
			addr = (unsigned long)(prm->startaddr + base_addr);
			break;
		}
	}

	buf = (phys_addr_t)phys_to_virt(addr);
	/* pr_info("alloc nosave buffer: 0x%x\n", buf); */

	return (void*)buf;
}
EXPORT_SYMBOL(aml_boot_alloc_mem);

static int __init aml_boot_complete(void)
{
	alloc_permission = 0;
	return 0;
}
late_initcall(aml_boot_complete);