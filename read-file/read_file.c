/*
 * Create a file /tmp/test.txt
 * Write a sting to this file, then read the string
 */

#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/syscalls.h>
#include <linux/fcntl.h>
#include <asm/uaccess.h>

#define dbgprint(fmt, args...) printk(KERN_INFO "%s: " fmt, __func__, ##args)

struct file* file_open(const char* path, int flags, mode_t mode)
{
	struct file* filp = NULL;
	mm_segment_t oldfs;
	int err = 0;

	oldfs = get_fs();
//	set_fs(get_ds());
	set_fs(KERNEL_DS);
	filp = filp_open(path, flags, mode);
	set_fs(oldfs);
	if(IS_ERR(filp)) {
		err = PTR_ERR(filp);
		return NULL;
	}
	return filp;
}

void file_close(struct file* file)
{
	filp_close(file, NULL);
}

int file_read(struct file* file, loff_t *pos,
		unsigned char* data, unsigned int size)
{
	mm_segment_t oldfs;
	int ret;

	oldfs = get_fs();
	set_fs(get_ds());

	ret = vfs_read(file, data, size, pos);

	set_fs(oldfs);
	return ret;
}

int file_write(struct file* file, unsigned long long *pos,
		unsigned char* data, unsigned int size) {
	mm_segment_t oldfs;
	int ret;

	oldfs = get_fs();
	set_fs(get_ds());

	ret = vfs_write(file, data, size, pos);

	set_fs(oldfs);
	return ret;
}

static int file_sync(struct file* file) {
	vfs_fsync(file, 0);
	return 0;
}

static int __init init(void)
{
	struct file	*fd;
	char		buf[1];
	loff_t		pos = 0;
	char		*str = "Hello world\nHello world";

	dbgprint("Enter\n");
	fd = file_open("/tmp/test.txt", O_CREAT | O_WRONLY | O_RDONLY, 0666);
	file_write(fd, &pos, str, strlen(str));
	file_sync(fd);
	dbgprint("pos %llu\n", pos);
	file_close(fd);

	fd = file_open("/tmp/test.txt", O_RDONLY, 0666);
	pos = 0;
	while (file_read(fd, &pos, buf, 1) == 1){
		printk("%c", buf[0]);
	}
	printk("\n");
	dbgprint("pos %llu\n", pos);
	file_close(fd);
	return 0;
}

static void __exit exit(void)
{ }

MODULE_LICENSE("GPL");
module_init(init);
module_exit(exit);
