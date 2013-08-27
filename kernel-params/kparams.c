/*
 * kparams.c
 * Create a module and a kernel parameter, you can adjust this kernel parameter
 * by echoing
 * For ex: echo 128 > /sys/module/kparams/paramters/bufsize
 */
#include <linux/fs.h>
#include <linux/types.h>
#include <linux/module.h>
#include <linux/device.h>
#include <linux/slab.h>
#include <linux/err.h>
#include <linux/uaccess.h>

#define MAX_CONFIG_LEN		80

static char config[MAX_CONFIG_LEN];
static unsigned bufsize = 128;

static struct kparam_string kps = {
	.string			= config,
	.maxlen			= MAX_CONFIG_LEN,
};

static int param_set_kparams_var(const char *kmessage, struct kernel_param *kp)
{
	strcpy(config, kmessage);
	kstrtouint(kmessage, 0, &bufsize);
	printk(KERN_INFO "%s kmessage: %s, bufsize: %u\n",
			__func__, kmessage, bufsize);
	return 0;
}

module_param_call(bufsize, param_set_kparams_var, param_get_string, &kps, 0644);

static int kparams_open(struct inode *inode, struct file *filp)
{
	return 0;
}

static int kparams_release(struct inode *inode, struct file *filp)
{
	return 0;
}

static ssize_t kparams_read(struct file *fp, char __user *buf, size_t len, 
		loff_t *pos)
{
	printk(KERN_INFO "kparams_read: %d\n", bufsize);
	return 0;
}

static struct class *kparams_class;
static struct device *kparams_dev;
static struct file_operations kparams_fops = {
	.open		= kparams_open,
	.release	= kparams_release,
	.read		= kparams_read,
};

int major;

static int __init kparams_init(void)
{
	int ret = 0;
	major = register_chrdev(0, "kparams", &kparams_fops);
	if (major < 0) {
		printk("Can't register kparams device driver\n");
		return ret;
	}
	kparams_class = class_create(THIS_MODULE, "kparams");
	if (IS_ERR(kparams_class)) {
		ret = PTR_ERR(kparams_class);
		unregister_chrdev(major, "kparams");
		return ret;
	}
	kparams_dev = device_create(kparams_class, NULL, MKDEV(major, 0), NULL, 
			"%s", "kparams");
	if (IS_ERR(kparams_class)) {
		ret = PTR_ERR(kparams_dev);
		class_destroy(kparams_class);
		unregister_chrdev(major, "kparams");
		return ret;
	}
	return 0;
}

static void kparams_exit(void)
{
	device_destroy(kparams_class, MKDEV(major, 0));
	class_destroy(kparams_class);
	unregister_chrdev(major, "kparams");
}

module_init(kparams_init);
module_exit(kparams_exit);
MODULE_LICENSE("GPL");
MODULE_AUTHOR("CongVT");
MODULE_DESCRIPTION("Demo kernel parameter");
