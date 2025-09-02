#include <linux/debugfs.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/uaccess.h>
#include <linux/slab.h>
#include <linux/pgtable.h>
#include <asm/pgtable.h>

#define SUCCESS 0
#define FAIL 1

struct packet
{
        pid_t pid;
        unsigned long vaddr;
        unsigned long paddr;
};

static struct dentry *dir, *outputf; /* Debug file system entries. */

static void page_walk(struct packet *pckt)
{
        struct task_struct *task;
        struct mm_struct *mm;
        pgd_t *pgd;
        p4d_t *p4d;
        pud_t *pud;
        pmd_t *pmd;
        pte_t pte;
        unsigned long paddr;

        task = pid_task(find_vpid(pckt->pid), PIDTYPE_PID);
        mm = task->mm;

        /* Get page table entry. */
        pgd = pgd_offset(mm, pckt->vaddr);
        p4d = p4d_offset(pgd, pckt->vaddr);
        pud = pud_offset(p4d, pckt->vaddr);
        pmd = pmd_offset(pud, pckt->vaddr);
        pte = *pte_offset_map(pmd, pckt->vaddr);

        if (pte_present(pte))
        {
                paddr = (pte_val(pte) & PAGE_MASK);  /* Page number */
                paddr |= (pckt->vaddr & ~PAGE_MASK); /* Page offset */
                paddr <<= 1;                         /* Remove valid bit */
                paddr >>= 1;
        }
        else
                paddr = 0UL;

        pckt->paddr = paddr;
}

static ssize_t read_output(struct file *fp,
                           char __user *user_buffer,
                           size_t length,
                           loff_t *position)
{
        struct packet *pckt = (struct packet *)kmalloc(sizeof(struct packet), GFP_KERNEL);

        /* Retrieve packet from the user buffer. */
        if (copy_from_user(pckt, user_buffer, sizeof(struct packet)))
        {
                printk("Copy from user failed\n");
                return 0;
        }

        page_walk(pckt);

        /* Send the packet to the user buffer. */
        if (copy_to_user(user_buffer, pckt, sizeof(struct packet)))
        {
                printk("Copy to user failed\n");
                return 0;
        }

        kfree(pckt);

        return length;
}

static const struct file_operations dbfs_fops = {
    .read = read_output,
};

static int __init dbfs_module_init(void)
{
        dir = debugfs_create_dir("paddr", NULL);
        if (!dir)
        {
                printk("Cannot create paddr dir\n");
                return FAIL;
        }

        outputf = debugfs_create_file("output", 0777, dir, NULL, &dbfs_fops);
        if (!outputf)
        {
                printk("Cannot create output file\n");
                return FAIL;
        }

        printk("dbfs_paddr module initialize done\n");
        return SUCCESS;
}

static void __exit dbfs_module_exit(void)
{
        /* Remove every files under 'dir' and iteslf. */
        debugfs_remove_recursive(dir);

        printk("dbfs_paddr module exit\n");
}

module_init(dbfs_module_init);
module_exit(dbfs_module_exit);

MODULE_LICENSE("GPL");
