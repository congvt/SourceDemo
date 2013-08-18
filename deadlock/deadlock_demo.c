/*
 * deadlock_world.c
 * Test KGDB
 */
#include <linux/fs.h>
#include <linux/types.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/err.h>

static DEFINE_MUTEX(mtxlock);
static int deadlock_open(struct inode *inode, struct file *filp)
{
	mutex_lock(&mtxlock);
	printk(KERN_INFO "Hello world\n");
	return 0;
}

static int deadlock_release(struct inode *inode, struct file *filp)
{
	/*
	 * FIX Me - Don't unlock mutex
	 */
//	mutex_unlock(&mtxlock);
	return 0;
}

static struct class *deadlock_class;
static struct device *deadlock_dev;
static struct file_operations deadlock_fops = {
	.open		= deadlock_open,
	.release	= deadlock_release,
};

int major;

static int __init deadlock_init(void)
{
	int ret = 0;
	printk(KERN_INFO "%s\n", __func__);
	major = register_chrdev(0, "deadlock", &deadlock_fops);
	if (major < 0) {
		printk("Can't register deadlock device driver\n");
		return ret;
	}
	deadlock_class = class_create(THIS_MODULE, "deadlock");
	if (IS_ERR(deadlock_class)) {
		ret = PTR_ERR(deadlock_class);
		unregister_chrdev(major, "deadlock");
		return ret;
	}
	deadlock_dev = device_create(deadlock_class, NULL, MKDEV(major, 0), NULL, 
			"%s", "deadlock");
	if (IS_ERR(deadlock_class)) {
		ret = PTR_ERR(deadlock_dev);
		class_destroy(deadlock_class);
		unregister_chrdev(major, "deadlock");
		return ret;
	}
	return 0;
}

static void deadlock_exit(void)
{
	printk(KERN_INFO "%s\n", __func__);
	device_destroy(deadlock_class, MKDEV(major, 0));
	class_destroy(deadlock_class);
	unregister_chrdev(major, "deadlock");
}
module_init(deadlock_init);
module_exit(deadlock_exit);
MODULE_LICENSE("GPL");
