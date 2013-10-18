/*
 * hello.c
 * Testing wait_queue
 * Step 1:
 *		# echo "y" > /pro/waitcon
 *		# cat /dev/hello	; waiting
 *		# dmesg
 * Step 2:
 *		# echo "n" > /proc/waitcon ; wakeup
 *		# dmesg
 */

#include <linux/fs.h>
#include <linux/types.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/uaccess.h>
#include <linux/wait.h>
#include <linux/sched.h>
#include <linux/proc_fs.h>
#include <linux/mutex.h>

#define dbgprint(fmt, args...)	printk(KERN_INFO "%s: " fmt, __func__, ##args);

#define YES		'y'
#define NO		'n'

#define HELLO_PROCFS_NAME	"waitcon"
#define HELLO_PROCFS_MODE	0666
#define HELLO_PROCFS_SIZE	128

DECLARE_WAIT_QUEUE_HEAD(wqueue);
DEFINE_MUTEX(hello_mutex);

struct proc_dir_entry	*hello_procfs;
static char		wait_flag = 'y';

static int hello_procfs_write(struct file *file, const char __user *buffer,
			unsigned long count, void *data)
{
	char read_buf[128];
	dbgprint("Enter\n");

	if (count > sizeof(read_buf))
		return -EINVAL;

	memset(read_buf, 0, sizeof(read_buf));
	if (copy_from_user (read_buf, (char __user *)buffer, count)) {
		dbgprint("Can't copy from user\n");
		return -EFAULT;
	}

	if (strncmp(read_buf, "y", 1))
		wait_flag = NO;
	else
		wait_flag = YES;
	/* wake up the wait queue */
	wake_up_interruptible(&wqueue);
	return count;
}

static ssize_t hello_read(struct file *fp, char __user *buf, size_t len,
		loff_t *pos)
{
	int i = 0;
	for (i = 0; i < 5; i++)
		dbgprint("%d\n", i);
	dbgprint("waiting ...\n");

	/* waiting for wake up */
	wait_event_interruptible(wqueue, wait_flag != YES);
	dbgprint("woken up ...\n");

	for (; i < 10; i++)
		dbgprint("%d\n", i);

	return 0;
}

static int hello_open(struct inode *inode, struct file *filp)
{
	mutex_lock(&hello_mutex);
	return 0;
}

static int hello_release(struct inode *inode, struct file *filp)
{
	mutex_unlock(&hello_mutex);
	return 0;
}
static struct file_operations hello_fops = {
	.open = hello_open,
	.release = hello_release,
	.read = hello_read,
};

static struct class *hello_class;
static struct device *hello_dev;
static int major;
static int __init hello_init(void)
{
	int ret = 0;

	dbgprint("Enter\n");

	major = register_chrdev(0, "hello", &hello_fops);
	if (major < 0) {
		dbgprint("Can't register hello device driver\n");
		return ret;
	}
	hello_class = class_create(THIS_MODULE, "hello");
	if (IS_ERR(hello_class)) {
		ret = PTR_ERR(hello_class);
		unregister_chrdev(major, "hello");
		return ret;
	}
	hello_dev = device_create(hello_class, NULL, MKDEV(major, 0), NULL, 
			"%s", "hello");
	if (IS_ERR(hello_class)) {
		ret = PTR_ERR(hello_dev);
		class_destroy(hello_class);
		unregister_chrdev(major, "hello");
		return ret;
	}
	/* Create procfs */
	hello_procfs = create_proc_entry(HELLO_PROCFS_NAME, HELLO_PROCFS_MODE, NULL);
	if (!hello_procfs) {
		remove_proc_entry(HELLO_PROCFS_NAME, NULL);
		goto out;
	}

	hello_procfs->write_proc	= hello_procfs_write;
//	hello_procfs->mode		= S_IFREG | S_IRUGO;
//	hello_procfs->uid		= 0;
//	hello_procfs->gid		= 0;
//	hello_procfs->size		= HELLO_PROCFS_SIZE;
	return 0;
out:
	device_destroy(hello_class, MKDEV(major, 0));
	class_destroy(hello_class);
	unregister_chrdev(major, "hello");
	return ret;
}

static void hello_exit(void)
{
	dbgprint("Enter\n");
	device_destroy(hello_class, MKDEV(major, 0));
	class_destroy(hello_class);
	unregister_chrdev(major, "hello");
	remove_proc_entry(HELLO_PROCFS_NAME, NULL);
}
module_init(hello_init);
module_exit(hello_exit);

MODULE_AUTHOR("CongVT");
MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("wait queue sample");
