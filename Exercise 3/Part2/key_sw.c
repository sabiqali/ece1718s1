#include <linux/fs.h>               // struct file, struct file_operations
#include <linux/init.h>             // for __init, see code
#include <linux/module.h>           // for module init and exit macros
#include <linux/miscdevice.h>       // for misc_device_register and struct miscdev
#include <linux/uaccess.h>          // for copy_to_user, see code
#include <asm/io.h>                 // for mmap
#include "./address_map_arm.h"

/* Kernel character device driver. By default, this driver provides the text "Hello from 
 * chardev" when /dev/chardev is read (for example, cat /dev/chardev). The text can be changed 
 * by writing a new string to /dev/chardev (for example echo "New message" > /dev/chardev).
 * This version of the code uses copy_to_user and copy_from_user, to send data to, and receive
 * date from, user-space */

static int key_open (struct inode *, struct file *);
static int key_release (struct inode *, struct file *);
static ssize_t key_read (struct file *, char *, size_t, loff_t *);
//static ssize_t key_write(struct file *, const char *, size_t, loff_t *);

static int sw_open (struct inode *, struct file *);
static int sw_release (struct inode *, struct file *);
static ssize_t sw_read (struct file *, char *, size_t, loff_t *);
//static ssize_t sw_write(struct file *, const char *, size_t, loff_t *);

static struct file_operations key_fops = {
    .owner = THIS_MODULE,
    .read = key_read,
    //.write = key_write,
    .open = key_open,
    .release = key_release
};

static struct file_operations sw_fops = {
    .owner = THIS_MODULE,
    .read = sw_read,
    //.write = sw_write,
    .open = sw_open,
    .release = sw_release
};

#define SUCCESS 0
#define DEV_NAME1 "key"
#define DEV_NAME2 "sw"

static struct miscdevice key = { 
    .minor = MISC_DYNAMIC_MINOR, 
    .name = DEV_NAME1,
    .fops = &key_fops,
    .mode = 0666
};

static struct miscdevice sw = { 
    .minor = MISC_DYNAMIC_MINOR, 
    .name = DEV_NAME2,
    .fops = &sw_fops,
    .mode = 0666
};

static int key_registered = 0;
static int sw_registered = 0;

#define MAX_SIZE_KEY 2                // we assume that no message will be longer than this
#define MAX_SIZE_SW 4                // we assume that no message will be longer than this
static char key_msg[MAX_SIZE_KEY];  // the character array that can be read or written
static char sw_msg[MAX_SIZE_SW];  // the character array that can be read or written

void *LW_virtual;
volatile unsigned *KEY_ptr, *SW_ptr;

static int __init start_drivers(void)
{
    
    //LW_virtual = ioremap_nocache(0xFF200000, 0x5000);
    //SW_ptr = LW_virtual + 0x40;
    //KEY_ptr = LW_virtual + 0x50;
    //*(KEY_ptr + 0x3) = 0xF;
    LW_virtual = ioremap_nocache(LW_BRIDGE_BASE, LW_BRIDGE_SPAN);
	KEY_ptr = (unsigned int *) (LW_virtual + KEY_BASE + 0x0C);
	*KEY_ptr = 0x0F;
    
    SW_ptr = (unsigned int *) (LW_virtual + SW_BASE);

    int err = misc_register (&key);
    if (err < 0) {
        printk (KERN_ERR "/dev/%s: misc_register() failed\n", DEV_NAME1);
        return err;
    }
    else {
        printk (KERN_INFO "/dev/%s driver registered\n", DEV_NAME1);
        key_registered = 1;
    }
    //strcpy (key_msg, "Hello from key\n"); /* initialize the message */

    err = misc_register (&sw);
    if (err < 0) {
        printk (KERN_ERR "/dev/%s: misc_register() failed\n", DEV_NAME2);
        return err;
    }
    else {
        printk (KERN_INFO "/dev/%s driver registered\n", DEV_NAME2);
        sw_registered = 1;
    }
    //strcpy (sw_msg, "Hello from sw\n"); /* initialize the message */

    return err;
}

static void __exit stop_drivers(void)
{
    *(KEY_ptr + 0x3) = 0xF;
    
    if (key_registered) {
        misc_deregister (&key);
        printk (KERN_INFO "/dev/%s driver de-registered\n", DEV_NAME1);
    }
    if (sw_registered) {
        misc_deregister (&sw);
        printk (KERN_INFO "/dev/%s driver de-registered\n", DEV_NAME2);
    }
}

/* Called when a process opens chardev */
static int key_open(struct inode *inode, struct file *file)
{
    return SUCCESS;
}

/* Called when a process closes chardev */
static int key_release(struct inode *inode, struct file *file)
{
    return 0;
}

/* Called when a process reads from chardev. Provides character data from key_msg.
 * Returns, and sets *offset to, the number of bytes read. */
static ssize_t key_read(struct file *filp, char *buffer, size_t length, loff_t *offset)
{
    /*size_t bytes;
    bytes = strlen (key_msg) - (*offset);    // how many bytes not yet sent?
    bytes = bytes > length ? length : bytes;     // too much to send all at once?
    
    if (bytes)
        if (copy_to_user (buffer, &key_msg[*offset], bytes) != 0)
            printk (KERN_ERR "Error: copy_to_user unsuccessful");
    *offset = bytes;    // keep track of number of bytes sent to the user
    return bytes;*/

    /*char themessage[MAX_SIZE_KEY + 1];
	size_t bytes;

	bytes = MAX_SIZE_KEY - (*offset);

	if (bytes){
        sprintf(themessage, "%03X\n", *(KEY_ptr + 0x3));

        copy_to_user(buffer, themessage, bytes);

	}

	(*offset) = bytes;

    *(KEY_ptr + 0x3) = 0xF;
	return bytes;*/

    size_t bytes;
	printk("#Reading: %p = %i\n", KEY_ptr, *KEY_ptr);
	sprintf(key_msg, "%01x\n", *KEY_ptr);
	bytes = strlen (key_msg) - (*offset);	// how many bytes not yet sent?
	bytes = bytes > length ? length : bytes;	// too much to send all at once?
	
	if (bytes)
		if (copy_to_user (buffer, &key_msg[*offset], bytes) != 0)
			printk (KERN_ERR "Error: copy_to_user unsuccessful");
	*offset = bytes;	// keep track of number of bytes sent to the user
	*KEY_ptr = 0x0F;
	
	return bytes;
}

/* Called when a process writes to chardev. Stores the data received into key_msg, and 
 * returns the number of bytes stored. */
/*static ssize_t key_write(struct file *filp, const char *buffer, size_t length, loff_t *offset)
{
    size_t bytes;
    bytes = length;

    if (bytes > MAX_SIZE - 1)    // can copy all at once, or not?
        bytes = MAX_SIZE - 1;
    if (copy_from_user (key_msg, buffer, bytes) != 0)
        printk (KERN_ERR "Error: copy_from_user unsuccessful");
    key_msg[bytes] = '\0';    // NULL terminate
    // Note: we do NOT update *offset; we just copy the data into key_msg
    return bytes;
}*/

static int sw_open(struct inode *inode, struct file *file)
{
    return SUCCESS;
}

/* Called when a process closes chardev */
static int sw_release(struct inode *inode, struct file *file)
{
    return 0;
}

/* Called when a process reads from chardev. Provides character data from sw_msg.
 * Returns, and sets *offset to, the number of bytes read. */
static ssize_t sw_read(struct file *filp, char *buffer, size_t length, loff_t *offset)
{
    /*size_t bytes;
    bytes = strlen (sw_msg) - (*offset);    // how many bytes not yet sent?
    bytes = bytes > length ? length : bytes;     // too much to send all at once?
    
    if (bytes)
        if (copy_to_user (buffer, &sw_msg[*offset], bytes) != 0)
            printk (KERN_ERR "Error: copy_to_user unsuccessful");
    *offset = bytes;    // keep track of number of bytes sent to the user
    return bytes;*/

    char themessage[MAX_SIZE_SW + 1] = "";
	size_t bytes;

	bytes = MAX_SIZE_SW - (*offset);

	if (bytes){
        sprintf(themessage, "%X\n", *(SW_ptr));

        copy_to_user(buffer, themessage, bytes);

	}

	(*offset) = bytes;

	return bytes;
}

/* Called when a process writes to chardev. Stores the data received into sw_msg, and 
 * returns the number of bytes stored. */
/*static ssize_t sw_write(struct file *filp, const char *buffer, size_t length, loff_t *offset)
{
    size_t bytes;
    bytes = length;

    if (bytes > MAX_SIZE - 1)    // can copy all at once, or not?
        bytes = MAX_SIZE - 1;
    if (copy_from_user (sw_msg, buffer, bytes) != 0)
        printk (KERN_ERR "Error: copy_from_user unsuccessful");
    sw_msg[bytes] = '\0';    // NULL terminate
    // Note: we do NOT update *offset; we just copy the data into sw_msg
    return bytes;
}*/

MODULE_LICENSE("GPL");
module_init (start_drivers);
module_exit (stop_drivers);
