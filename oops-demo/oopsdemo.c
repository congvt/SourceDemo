/*
 * oopsdemo_world.c
 * Test KGDB
 */
#include <linux/fs.h>
#include <linux/types.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/err.h>

static u8 *buffer;
static unsigned bufsize = 128;
module_param(bufsize, uint, S_IRUGO);

static void create_oops(void)
{
	*(int *) 0 = 0;
}

static int oopsdemo_open(struct inode *inode, struct file *filp)
{
	if (!buffer) {
		buffer = kmalloc(bufsize, GFP_KERNEL);
		if (!buffer)
			return -ENOMEM;
	}	
	memset(buffer, 0, bufsize);
	return 0;
}

static int oopsdemo_release(struct inode *inode, struct file *filp)
{
	/*
	 * FIX Me - don't set pointer to NULL after free
	 */
	kfree(buffer);
	return 0;
}

static ssize_t oopsdemo_read(struct file *fp, char __user *buf, size_t len, 
		loff_t *pos)
{
	int i = 0;
	for (i = 0; i < 10; i++)
		printk(KERN_INFO "oopsdemo_read: %d\n", i);
	create_oops();
	return 0;
}
static struct class *oopsdemo_class;
static struct device *oopsdemo_dev;
static struct file_operations oopsdemo_fops = {
	.open		= oopsdemo_open,
	.release	= oopsdemo_release,
	.read		= oopsdemo_read,
};

int major;

static int __init oopsdemo_init(void)
{
	int ret = 0;
	printk(KERN_INFO "Hello world\n");
	major = register_chrdev(0, "oopsdemo", &oopsdemo_fops);
	if (major < 0) {
		printk("Can't register oopsdemo device driver\n");
		return ret;
	}
	oopsdemo_class = class_create(THIS_MODULE, "oopsdemo");
	if (IS_ERR(oopsdemo_class)) {
		ret = PTR_ERR(oopsdemo_class);
		unregister_chrdev(major, "oopsdemo");
		return ret;
	}
	oopsdemo_dev = device_create(oopsdemo_class, NULL, MKDEV(major, 0), NULL, 
			"%s", "oopsdemo");
	if (IS_ERR(oopsdemo_class)) {
		ret = PTR_ERR(oopsdemo_dev);
		class_destroy(oopsdemo_class);
		unregister_chrdev(major, "oopsdemo");
		return ret;
	}
	return 0;
}

static void oopsdemo_exit(void)
{
	device_destroy(oopsdemo_class, MKDEV(major, 0));
	class_destroy(oopsdemo_class);
	unregister_chrdev(major, "oopsdemo");
	printk(KERN_INFO "Hello world: Exit\n");
}
module_init(oopsdemo_init);
module_exit(oopsdemo_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("CongVT");
MODULE_DESCRIPTION("Demo oops");
