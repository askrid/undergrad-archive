#include <linux/debugfs.h>
#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/string.h>
#include <linux/uaccess.h>

#define SUCCESS 0
#define FAIL -1
#define MAX_BUF_SIZE 999

static struct dentry *dir, *inputf, *ptreef; /* Debug file system entries. */
static struct task_struct *task;             /* Leaf task. */
static char content[MAX_BUF_SIZE];           /* Content to read from ptree file. */

static ssize_t input_write(struct file *fp,
                           const char __user *user_buffer,
                           size_t length,
                           loff_t *position)
{
        pid_t input_pid;
        char tmp[MAX_BUF_SIZE];

        /* User input is PID. */
        sscanf(user_buffer, "%u", &input_pid);

        /* Get task_struct from input PID. */
        task = pid_task(find_vpid(input_pid), PIDTYPE_PID);
        if (!task)
        {
                printk("Cannot retreive task structure from the given PID\n");
                return -EINVAL;
        }

        /* Clear the content buffer. */
        memset(content, 0, sizeof(content));

        /* Append ancestors' information to the buffer until curr meets init process. */
        while (task->pid != 1)
        {
                strcpy(tmp, content);
                sprintf(content, "%s (%u)\n", task->comm, task->pid);
                strcat(content, tmp);
                task = task->parent;
        }

        /* Append init process to the buffer. */
        strcpy(tmp, content);
        sprintf(content, "%s (%u)\n", task->comm, task->pid);
        strcat(content, tmp);

        return length;
}

static ssize_t ptree_read(struct file *fp,
                          char __user *user_buffer,
                          size_t length,
                          loff_t *position)
{
        int bytes_read = 0;
        const char *c_ptr = content;

        /* EOF - reset the position. */
        if (!*(c_ptr + *position))
        {
                *position = 0;
                return 0;
        }

        /* Move content pointer to the position. */
        c_ptr += *position;

        /* Put the content data to the user buffer. */
        while (length && *c_ptr)
        {
                put_user(*(c_ptr++), user_buffer++);
                length--;
                bytes_read++;
        }

        *position += bytes_read;

        return bytes_read;
}

/* File operations for input file - write only. */
static const struct file_operations input_fops = {
    .write = input_write,
};

/* File operations for ptree file - read only. */
static const struct file_operations ptree_fops = {
    .read = ptree_read,
};

static int __init dbfs_module_init(void)
{
        dir = debugfs_create_dir("ptree", NULL);
        if (!dir)
        {
                printk("Cannot create ptree directory\n");
                return FAIL;
        }

        inputf = debugfs_create_file("input", 0222, dir, NULL, &input_fops);
        ptreef = debugfs_create_file("ptree", 0555, dir, NULL, &ptree_fops);
        if (!inputf || !ptreef)
        {
                printk("Cannot create ptree input/ptree file\n");
                return FAIL;
        }

        printk("dbfs_ptree module initialize done\n");

        return SUCCESS;
}

static void __exit dbfs_module_exit(void)
{
        /* Remove every files under 'dir' and itself. */
        debugfs_remove_recursive(dir);

        printk("dbfs_ptree module exit\n");
}

module_init(dbfs_module_init);
module_exit(dbfs_module_exit);

MODULE_LICENSE("GPL");
