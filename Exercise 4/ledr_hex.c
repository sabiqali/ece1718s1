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

static int ledr_open (struct inode *, struct file *);
static int ledr_release (struct inode *, struct file *);
//static ssize_t ledr_read (struct file *, char *, size_t, loff_t *);
static ssize_t ledr_write(struct file *, const char *, size_t, loff_t *);

static int hex_open (struct inode *, struct file *);
static int hex_release (struct inode *, struct file *);
//static ssize_t hex_read (struct file *, char *, size_t, loff_t *);
static ssize_t hex_write(struct file *, const char *, size_t, loff_t *);

static struct file_operations ledr_fops = {
    .owner = THIS_MODULE,
    //.read = ledr_read,
    .write = ledr_write,
    .open = ledr_open,
    .release = ledr_release
};

static struct file_operations hex_fops = {
    .owner = THIS_MODULE,
    //.read = hex_read,
    .write = hex_write,
    .open = hex_open,
    .release = hex_release
};

#define SUCCESS 0
#define DEV_NAME1 "ledr"
#define DEV_NAME2 "hex"

static struct miscdevice ledr = { 
    .minor = MISC_DYNAMIC_MINOR, 
    .name = DEV_NAME1,
    .fops = &ledr_fops,
    .mode = 0666
};

static struct miscdevice hex = { 
    .minor = MISC_DYNAMIC_MINOR, 
    .name = DEV_NAME2,
    .fops = &hex_fops,
    .mode = 0666
};

static int ledr_registered = 0;
static int hex_registered = 0;

#define MAX_SIZE_LEDR 5                // we assume that no message will be longer than this
#define MAX_SIZE_HEX 8                // we assume that no message will be longer than this
static char ledr_msg[MAX_SIZE_LEDR];  // the character array that can be read or written
static char hex_msg[MAX_SIZE_HEX];  // the character array that can be read or written

void *LW_virtual;
volatile unsigned *LEDR_ptr, *HEX3_HEX0_ptr, *HEX5_HEX4_ptr;

int hex_num(int num){
	switch(num){
		case 0:
			return 0x3F;
		case 1:
			return 0x06;
		case 2:
			return 0x5B;
		case 3:
			return 0x4F;
		case 4:
			return 0x66;
		case 5:
			return 0x6D;
		case 6:
			return 0x7D;
		case 7:
			return 0x07;
		case 8:
			return 0x7F;
		case 9:
			return 0x67;
	}
}

static int __init start_drivers(void)
{
    
    //LW_virtual = ioremap_nocache(0xFF200000, 0x5000);
    //SW_ptr = LW_virtual + 0x40;
    //KEY_ptr = LW_virtual + 0x50;
    //*(KEY_ptr + 0x3) = 0xF;
    LW_virtual = ioremap_nocache(LW_BRIDGE_BASE, LW_BRIDGE_SPAN);
    LEDR_ptr = LW_virtual + LEDR_BASE;
    HEX3_HEX0_ptr = LW_virtual + HEX3_HEX0_BASE;
    HEX5_HEX4_ptr = LW_virtual + HEX5_HEX4_BASE;
    *(LEDR_ptr) = 0x0;
	//KEY_ptr = (unsigned int *) (LW_virtual + KEY_BASE + 0x0C);
	//*KEY_ptr = 0x0F;
    
    //SW_ptr = (unsigned int *) (LW_virtual + SW_BASE);

    int err = misc_register (&ledr);
    if (err < 0) {
        printk (KERN_ERR "/dev/%s: misc_register() failed\n", DEV_NAME1);
        return err;
    }
    else {
        printk (KERN_INFO "/dev/%s driver registered\n", DEV_NAME1);
        ledr_registered = 1;
    }
    //strcpy (key_msg, "Hello from key\n"); /* initialize the message */

    err = misc_register (&hex);
    if (err < 0) {
        printk (KERN_ERR "/dev/%s: misc_register() failed\n", DEV_NAME2);
        return err;
    }
    else {
        printk (KERN_INFO "/dev/%s driver registered\n", DEV_NAME2);
        hex_registered = 1;
    }
    //strcpy (sw_msg, "Hello from sw\n"); /* initialize the message */

    return err;
}

static void __exit stop_drivers(void)
{
    *(LEDR_ptr) = 0x0;
    *(HEX5_HEX4_ptr) = 0x0;
    *(HEX3_HEX0_ptr) = 0x0;
    
    if (ledr_registered) {
        misc_deregister (&ledr);
        printk (KERN_INFO "/dev/%s driver de-registered\n", DEV_NAME1);
    }
    if (hex_registered) {
        misc_deregister (&hex);
        printk (KERN_INFO "/dev/%s driver de-registered\n", DEV_NAME2);
    }
}

/* Called when a process opens chardev */
static int ledr_open(struct inode *inode, struct file *file)
{
    return SUCCESS;
}

/* Called when a process closes chardev */
static int ledr_release(struct inode *inode, struct file *file)
{
    return 0;
}

/* Called when a process reads from chardev. Provides character data from key_msg.
 * Returns, and sets *offset to, the number of bytes read. */
/*static ssize_t ledr_read(struct file *filp, char *buffer, size_t length, loff_t *offset)
{
    /*size_t bytes;
    bytes = strlen (key_msg) - (*offset);    // how many bytes not yet sent?
    bytes = bytes > length ? length : bytes;     // too much to send all at once?
    
    if (bytes)
        if (copy_to_user (buffer, &key_msg[*offset], bytes) != 0)
            printk (KERN_ERR "Error: copy_to_user unsuccessful");
    *offset = bytes;    // keep track of number of bytes sent to the user
    return bytes;

    /*char themessage[MAX_SIZE_KEY + 1];
	size_t bytes;

	bytes = MAX_SIZE_KEY - (*offset);

	if (bytes){
        sprintf(themessage, "%03X\n", *(KEY_ptr + 0x3));

        copy_to_user(buffer, themessage, bytes);

	}

	(*offset) = bytes;

    *(KEY_ptr + 0x3) = 0xF;
	return bytes;

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
}*/

/* Called when a process writes to chardev. Stores the data received into key_msg, and 
 * returns the number of bytes stored. */
static ssize_t ledr_write(struct file *filp, const char *buffer, size_t length, loff_t *offset)
{
    /*size_t bytes;
    bytes = length;

    if (bytes > MAX_SIZE - 1)    // can copy all at once, or not?
        bytes = MAX_SIZE - 1;
    if (copy_from_user (key_msg, buffer, bytes) != 0)
        printk (KERN_ERR "Error: copy_from_user unsuccessful");
    key_msg[bytes] = '\0';    // NULL terminate
    // Note: we do NOT update *offset; we just copy the data into key_msg
    return bytes;*/

    size_t bytes; int num;
	bytes = length;

	if (bytes > MAX_SIZE_LEDR - 1)	// can copy all at once, or not?
		bytes = MAX_SIZE_LEDR - 1;
	if (copy_from_user (ledr_msg, buffer, bytes) != 0)
		printk (KERN_ERR "Error: copy_from_user unsuccessful");

	//LEDR_msg[bytes] = '\0';	// NULL terminate
	sscanf(ledr_msg, "%x", &num);

	*LEDR_ptr = num;
	// Note: we do NOT update *offset; we just copy the data into chardev_msg
	return bytes;
}

static int hex_open(struct inode *inode, struct file *file)
{
    return SUCCESS;
}

/* Called when a process closes chardev */
static int hex_release(struct inode *inode, struct file *file)
{
    return 0;
}

/* Called when a process reads from chardev. Provides character data from sw_msg.
 * Returns, and sets *offset to, the number of bytes read. */
/*static ssize_t hex_read(struct file *filp, char *buffer, size_t length, loff_t *offset)
{
    /*size_t bytes;
    bytes = strlen (sw_msg) - (*offset);    // how many bytes not yet sent?
    bytes = bytes > length ? length : bytes;     // too much to send all at once?
    
    if (bytes)
        if (copy_to_user (buffer, &sw_msg[*offset], bytes) != 0)
            printk (KERN_ERR "Error: copy_to_user unsuccessful");
    *offset = bytes;    // keep track of number of bytes sent to the user
    return bytes;*

    char themessage[MAX_SIZE_SW + 1] = "";
	size_t bytes;

	bytes = MAX_SIZE_SW - (*offset);

	if (bytes){
        sprintf(themessage, "%X\n", *(SW_ptr));

        copy_to_user(buffer, themessage, bytes);

	}

	(*offset) = bytes;

	return bytes;
}*/

/* Called when a process writes to chardev. Stores the data received into sw_msg, and 
 * returns the number of bytes stored. */
static ssize_t hex_write(struct file *filp, const char *buffer, size_t length, loff_t *offset)
{
    /*size_t bytes;
    bytes = length;

    if (bytes > MAX_SIZE - 1)    // can copy all at once, or not?
        bytes = MAX_SIZE - 1;
    if (copy_from_user (sw_msg, buffer, bytes) != 0)
        printk (KERN_ERR "Error: copy_from_user unsuccessful");
    sw_msg[bytes] = '\0';    // NULL terminate
    // Note: we do NOT update *offset; we just copy the data into sw_msg
    return bytes;*/

    size_t bytes;
	bytes = length;

    int disp0, disp1, disp2, disp3, disp4, disp5;
    int num;

	if (bytes > MAX_SIZE_HEX - 1)	// can copy all at once, or not?
		bytes = MAX_SIZE_HEX - 1;
	if (copy_from_user (hex_msg, buffer, bytes) != 0)
		printk (KERN_ERR "Error: copy_from_user unsuccessful");

    sscanf(hex_msg, "%d", &num);

    disp0 = num %10; 		        //first digit
    disp1 = (num %100)/10;		    //second digit
    disp2 = (num %1000)/100;	    //third digit
    disp3 = (num %10000)/1000;	    //fourth digit
    disp4 = (num %100000)/10000;	//fifth digit
    disp5 = (num %1000000)/100000;	//sixth digit


    *(HEX3_HEX0_ptr) = hex_num(disp0)| (hex_num(disp1)<<8) | (hex_num(disp2) << 16) | (hex_num(disp3) << 24);
    *(HEX5_HEX4_ptr) = hex_num(disp4) | (hex_num(disp5) <<8);

	return bytes;
}

MODULE_LICENSE("GPL");
module_init (start_drivers);
module_exit (stop_drivers);
