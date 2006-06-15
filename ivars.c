// pmap

// TODO:
//
// + nazwa/snprintf do fmapstat


#include "fmap_e.h"

#include <linux/config.h>
#include <linux/module.h>

#include <linux/errno.h>
#include <linux/fs.h>
#include <linux/major.h>
#include <linux/kernel.h>
#include <linux/signal.h>
#include <linux/sched.h>
#include <linux/mm.h>
#include <linux/proc_fs.h>

#include <asm/uaccess.h>
#include <asm/pgtable.h>



#ifndef AZT_KERNEL_PRIOR_2_1 
#define  memcpy_fromfs copy_from_user
#define  memcpy_tofs   copy_to_user
#endif


extern void* vmalloc ();
extern void* vfree ();

extern int printk (const char* fmt, ...);


#define STATE__CLOSED 0
#define STATE__OPENED 1
#define STATE__SIZE_SET 2
#define STATE__OFFSET_SET 3
#define STATE__READY 4


#define COPY_TO_USER 0
#define COPY_FROM_USER 1
#define INC_COUNT 2


typedef struct {
    int state;
    unsigned long offset;
    int size;
    struct mm_struct *mm;
    pid_t owner;
    int bytes_read, bytes_written;
} devinfo;


void *phonybuf;

#define DEVS 256
devinfo devs[DEVS];


#define D_MAJOR 123


static size_t hw_action (int direct, char* buf, struct mm_struct *mm, unsigned long addr, int count)
{
	int to_copy, passed = 0;

	unsigned long page;
	int mapnr;
	pgd_t *pgdir;
	pmd_t *pgmiddle;
	pte_t *pgtable;

	while (passed < count) {

	    if (direct == INC_COUNT)	// force physical memory allocation
		copy_from_user (phonybuf, (void *)addr, 4);

	    pgdir = pgd_offset (mm, addr);
	    if (pgd_none(*pgdir)) {
		printk ("fmap: ERROR -- page directory missing.\n");
		return passed;
	    }
	    if (pgd_bad(*pgdir)) {
		printk ("fmap: ERROR -- bad page directory.\n");
		return passed;
	    }
	    pgmiddle = pmd_offset(pgdir, addr);
	    if (pmd_none(*pgmiddle)) {
		printk ("fmap: ERROR -- middle page missing.\n");
		return passed;
	    }
	    if (pmd_bad(*pgmiddle)) {
		printk ("fmap: ERROR -- bad middle page.\n");
		return passed;
	    }
	    pgtable = pte_offset(pgmiddle, addr);
	    if (!pte_present(*pgtable)) {
		printk ("fmap: ERROR -- page not present.\n");
		return passed;
	    }

	    page = pte_page(*pgtable);

	    to_copy = PAGE_SIZE - (addr & ~PAGE_MASK);
	    if (to_copy > count - passed)
		to_copy = count - passed;

	    if (direct == COPY_TO_USER) {		// to user
		copy_to_user (buf, (void *)(page + (addr & ~PAGE_MASK)), to_copy);
		buf += to_copy;
	    }
	    else if (direct == COPY_FROM_USER) {	// from user
		copy_from_user ((void *)(page + (addr & ~PAGE_MASK)), buf, to_copy);
		buf += to_copy;
	    }
	    else if (direct == INC_COUNT) {
		mapnr = MAP_NR(page);
		if (mapnr<max_mapnr) {
		    atomic_inc (&mem_map[mapnr].count);
		    atomic_inc (&mem_map[mapnr].count);
		}
	    }

	    passed += to_copy;
	    addr += to_copy;

	}

	return passed;
}


static ssize_t hw_write (struct file *file, const char *buf, size_t count, loff_t *filep)
{
	int min = MINOR(file->f_dentry->d_inode->i_rdev), left, howmuch;
	size_t passed;

//printk ("fmap: write\n");

	if (devs[min].state != STATE__READY)
	    return -E_SETUP;

	left = devs[min].size - file->f_pos, howmuch = 0;
	if (left < 0)
	    left = 0;

	if ((howmuch = count > left ? left : count)) {
	    passed = hw_action (COPY_FROM_USER, (char *)buf, devs[min].mm, devs[min].offset + file->f_pos, howmuch);
	    file->f_pos += passed;
	    devs[min].bytes_written += passed;
	    return passed;
	}

	return 0;
}


static ssize_t hw_read (struct file *file, char *buf, size_t count, loff_t *filep)
{	
	int min = MINOR(file->f_dentry->d_inode->i_rdev), left, howmuch;
	size_t passed;

//printk ("fmap: read\n");

	if (devs[min].state != STATE__READY)
	    return -E_SETUP;

	left = devs[min].size - file->f_pos, howmuch = 0;
	if (left < 0)
	    left = 0;

	if ((howmuch = count > left ? left : count)) {
	    passed = hw_action (COPY_TO_USER, buf, devs[min].mm, devs[min].offset + file->f_pos, howmuch);
	    file->f_pos += passed;
	    devs[min].bytes_read += passed;
	    return passed;
	}

	return 0;
}


static loff_t hw_lseek (struct file *file, loff_t offset, int orig)
{
	int min = MINOR(file->f_dentry->d_inode->i_rdev);

//printk ("fmap: seek\n");

	if (devs[min].state != STATE__READY)
	    return -E_SETUP;

	switch (orig) {
	    case 0: // set
		if (offset>devs[min].size)
		    return -E_EXPORTRANGE;
		file->f_pos = offset;
		break;
	    case 1: // current
		if (file->f_pos+offset>devs[min].size)
		    return -E_EXPORTRANGE;
		file->f_pos += offset;
		break;
	    case 2: // end
		if (offset>devs[min].size)
		    return -E_EXPORTRANGE;
		file->f_pos = devs[min].size - offset;
		break;
	}

	return file->f_pos;
}


static int outofrange (int min, unsigned long endofs)
{
	struct vm_area_struct *vma = find_vma (devs[min].mm, (endofs));

	if (vma == NULL)
	    return 1;
	if (endofs < vma->vm_start || endofs >= vma->vm_end)
	    return 1;

	return 0;
}


static int hw_ioctl (struct inode* ino, struct file *filep, unsigned int req, unsigned long val)
{
	int min = MINOR(ino->i_rdev);

//printk ("fmap: ioctl\n");

	if (devs[min].owner != current->pid)
	    return -E_NOEXPORT;

	if (devs[min].state == STATE__READY)
	    return -E_ALREADYSET;

	if (req == SIZE) {
//printk ("fmap: ioctl/SIZE\n");
	    if (devs[min].state == STATE__SIZE_SET)
		return -E_ALREADYSET;
	    else if (devs[min].state == STATE__OFFSET_SET) {
		if (outofrange (min, val+devs[min].offset))
		    return -E_RANGE;
	    }
	    devs[min].size = (int)val;
	    devs[min].state = (devs[min].state == STATE__OPENED ? STATE__SIZE_SET : STATE__READY);
	}
	else if (req == OFFSET) {
//printk ("fmap: ioctl/OFFSET\n");
	    if (devs[min].state == STATE__OFFSET_SET)
		return -E_ALREADYSET;
	    else if (devs[min].state == STATE__SIZE_SET) {
		if (outofrange (min, val+devs[min].size))
		    return -E_RANGE;
	    }
	    devs[min].offset = val;
	    devs[min].state = (devs[min].state == STATE__OPENED ? STATE__OFFSET_SET : STATE__READY);
	}

	if (devs[min].state == STATE__READY) {
	    hw_action (INC_COUNT, NULL, devs[min].mm, devs[min].offset, devs[min].size);
	    devs[min].bytes_read = 0;
	    devs[min].bytes_written = 0;
	}

	return 0;
}


static int hw_open (struct inode *ino, struct file *filep)
{
	int min = MINOR(ino->i_rdev);

//printk ("fmap: open\n");

	if ((filep->f_flags & O_INIT) == O_INIT) {
//printk ("fmap: O_INIT open.\n");
	    if (devs[min].state == STATE__CLOSED) {
		MOD_INC_USE_COUNT;
		devs[min].state = STATE__OPENED;
		devs[min].owner = current->pid;
		devs[min].mm = current->mm;
		return 0;
	    }
	    else 
		return -E_INIT;
	}
	else if (filep->f_flags == 0) {
//printk ("fmap: plain open.\n");
	    if (devs[min].state == STATE__READY) {
		return 0;
	    }
	    else
		return -E_SETUP;
	}

	return 0;
}


static int hw_close (struct inode *ino, struct file *filep)
{
	int min = MINOR(ino->i_rdev);

//printk ("fmap: close\n");

	if (devs[min].state == STATE__CLOSED)
	    return -1;

	if (devs[min].owner != current->pid)
	    return 0;

	devs[min].state = STATE__CLOSED;
	MOD_DEC_USE_COUNT;

	return 0;

}


static struct file_operations fops = {
	hw_lseek,
	hw_read,
	hw_write,
	NULL,		/* hw_readdir */
	NULL,		/* hw_select */
	hw_ioctl,	/* hw_ioctl */
	NULL,
        hw_open, 
        NULL,
	hw_close,
	NULL		/* fsync */
};


int procfile_read (char *buf, char **bufloc, off_t offset, int bufsize, int zero)
{
	int i, of = 0;

	if (offset>0)
	    return 0;

	for (i=0;i<DEVS;i++)
	    if (devs[i].state == STATE__READY)
		of += sprintf (&buf[of], "?name? minor=%d pid=%d offset=%ld size=%d bytes_read=%d bytes_written=%d\n", i, devs[i].owner, devs[i].offset, devs[i].size, devs[i].bytes_read, devs[i].bytes_written);

	return of;
}


static struct proc_dir_entry proc_file = {
	0,
	8,
	"fmapstat",
	S_IFREG | S_IRUGO,
	1,
	0, 0,
	0,
	NULL,
	procfile_read,
	NULL
};


// MODUL ---------------------------------------------------------------------


int init_module (void)
{
	int i;

	printk ("fmap: init_module.\n");

	if (proc_register (&proc_root, &proc_file)) {
	    printk ("fmap: proc_register failed.\n");
	    return -EIO;
	}

	if (!(phonybuf = vmalloc (4))) {
	    printk ("fmap: not enough kernel memory. (?)\n");
	    return -ENOMEM;
	}

	if (register_chrdev (D_MAJOR, "hw", &fops)) {
	    printk ("fmap: register_chrdev failed.\n");
	    return -EIO;
	}

	for (i=0;i<DEVS;i++)
	    devs[i].state = STATE__CLOSED;

	printk ("fmap: init_module succeeded.\n");
	return 0;
}


void cleanup_module (void)
{
	printk ("fmap: cleanup_module.\n");

	if (phonybuf)
	    vfree (phonybuf);

	if (unregister_chrdev (D_MAJOR, "hw") != 0 || proc_unregister (&proc_root, proc_file.low_ino) != 0)
	    printk ("fmap: cleanup_module failed.\n");
	else
	    printk ("fmap: cleanup_module succeeded.\n");
}

