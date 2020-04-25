#include <linux/fs.h>               // struct file, struct file_operations
#include <linux/init.h>             // for __init, see code
#include <linux/module.h>           // for module init and exit macros
#include <linux/miscdevice.h>       // for misc_device_register and struct miscdev
#include <linux/uaccess.h>          // for copy_to_user, see code
#include <asm/io.h>                 // for mmap
#include <linux/interrupt.h>
#include "./address_map_arm.h"
#include "./interrupt_ID.h"

/* Kernel character device driver. By default, this driver provides the text "Hello from 
 * chardev" when /dev/chardev is read (for example, cat /dev/chardev). The text can be changed 
 * by writing a new string to /dev/chardev (for example echo "New message" > /dev/chardev).
 * This version of the code uses copy_to_user and copy_from_user, to send data to, and receive
 * date from, user-space */

static int signal_generator_open (struct inode *, struct file *);
static int signal_generator_release (struct inode *, struct file *);
static ssize_t signal_generator_read (struct file *, char *, size_t, loff_t *);
static ssize_t signal_generator_write(struct file *, const char *, size_t, loff_t *);

static int timer_handler(void);
irq_handler_t irq_handler(int irq, void *dev_id, struct pt_regs *regs);
//int hex(int num);

static struct file_operations signal_generator_fops = {
    .owner = THIS_MODULE,
    //.read = signal_generator_read,
    //.write = signal_generator_write,
    .open = signal_generator_open,
    .release = signal_generator_release
};

#define SUCCESS 0
#define DEV_NAME1 "signal_generator"

// Assume that no message longer than this will be used
#define MAX_SIZE 8

int gpio_pin_out = 0;
static int hex[10] = {0x3F,0x06,0x5B,0x4F,0x66,0x6D,0x7D,0x07,0x7F,0x67};

static struct miscdevice signal_generator = { 
    .minor = MISC_DYNAMIC_MINOR, 
    .name = DEV_NAME1,
    .fops = &signal_generator_fops,
    .mode = 0666
};

static int signal_generator_registered = 0;

 // Declare global variables needed to use the pixel buffer
void *LW_virtual; // used to access FPGA light-weight bridge
volatile int *timer_ptr; //used to access the FPGA Timer
volatile int *HEX_3_0_ptr; //used to access the HEX display
volatile int *SW_ptr; //used to access the sw switches
volatile int *LEDR_ptr; //used to access the LEDRs
volatile int *pin_ptr; //Used to access the GPIO pins 


static int __init start_signal_generator(void)
{
    int value;
    int err = misc_register (&signal_generator);
    if (err < 0) {
        printk (KERN_ERR "/dev/%s: misc_register() failed\n", DEV_NAME1);
        return err;
    }
    else {
        printk (KERN_INFO "/dev/%s driver registered\n", DEV_NAME1);
        signal_generator_registered = 1;
    }

    /*LW_virtual = ioremap_nocache(LW_BRIDGE_BASE, LW_BRIDGE_SPAN); //Mapping the FPGA lightweight bridge

    timer_ptr = LW_virtual + TIMER0_BASE; //mapping the timer pointer

    LEDR_ptr = LW_virtual + LEDR_BASE; //mapping the ledr pointer

    SW_ptr = LW_virtual + SW_BASE; //mapping the switch pointer

    pin_ptr = LW_virtual + GPIO0_BASE; //mapping the switch pointer

    *(pin_ptr + 1) = 1;

    *(pin_ptr + 2) = 0x2D00; //setting the lower counter start value register

    *(pin_ptr + 3) =  0x131; //setting the upper counter start value register

    *(pin_ptr + 1) = 0x7; //setting the ITO, CONT, START bits to 1;

    HEX_3_0_ptr = LW_virtual + HEX3_HEX0_BASE;
    
    *(HEX_3_0_ptr) = hex(0) | (hex(1)<<8) | (hex(0) << 16) | (hex(0) << 24);

    value = request_irq (TIMER0_IRQ, (irq_handler_t) irq_handler, IRQF_SHARED,"irq_handler", (void *) (irq_handler));*/

    value = timer_handler();

    return err;
}

static void __exit stop_signal_generator(void)
{
    *HEX_3_0_ptr = 0;
    *LEDR_ptr = 0;
	free_irq (TIMER0_IRQ, (void*) irq_handler);
    // unmap the physical-to-virtual mappings
    iounmap (LW_virtual);

    if (signal_generator_registered) {
        misc_deregister (&signal_generator);
        printk (KERN_INFO "/dev/%s driver de-registered\n", DEV_NAME1);
    }
}

/* Called when a process opens chardev */
static int signal_generator_open(struct inode *inode, struct file *file)
{
    return SUCCESS;
}

/* Called when a process closes chardev */
static int signal_generator_release(struct inode *inode, struct file *file)
{
    return 0;
}

/* Called when a process reads from chardev. Provides character data from stopwatch_msg.
 * Returns, and sets *offset to, the number of bytes read. */
/*static ssize_t signal_generator_read(struct file *filp, char *buffer, size_t length, loff_t *offset)
{
    size_t bytes;

	get_screen_specs(pixel_ctrl_ptr);
	sprintf(video_msg, "%d %d\n", resolution_x, resolution_y);


	bytes = strlen (video_msg) - (*offset);	// how many bytes not yet sent?
	bytes = bytes > length ? length : bytes;	// too much to send all at once?

	if (bytes)
		if (copy_to_user (buffer, &video_msg[*offset], bytes) != 0)
			printk (KERN_ERR "Error: copy_to_user unsuccessful");
	*offset = bytes;	// keep track of number of bytes sent to the user
	return bytes;
}*/

/* Called when a process writes to chardev. Stores the data received into stopwatch_msg, and 
 * returns the number of bytes stored. */
/*static ssize_t signal_generator_write(struct file *filp, const char *buffer, size_t length, loff_t *offset)
{
    size_t bytes;
	bytes = length;
    char command[bytes];
    char text_[bytes];
    int x1, y1, x2, y2;
    short int color;
    int text_length;

	if (bytes > MAX_SIZE - 1)	// can copy all at once, or not?
		bytes = MAX_SIZE - 1;
	if (copy_from_user (video_msg2, buffer, bytes) != 0)
		printk (KERN_ERR "Error: copy_from_user unsuccessful");

    video_msg2[bytes] = '\0';
    sscanf (video_msg2, "%s", command);

    //Check user input and perform actions
    if (strcmp(command, "clear") == 0){ clear_screen(); }

    else if (strcmp(command, "sync") == 0){wait_for_vsync(pixel_ctrl_ptr);}

    else if (sscanf(video_msg2, "pixel %d,%d %x", &x1, &y1, &color) == 3){
        plot_pixel(x1,y1,color); }

    else if (sscanf(video_msg2, "line %d,%d %d,%d %x", &x1, &y1, &x2, &y2,  &color) == 5){
        draw_line(x1,x2,y1,y2,color); }

    else if (sscanf(video_msg2, "box %d,%d %d,%d %x", &x1, &y1, &x2, &y2,  &color) == 5){
        draw_box(x1,x2,y1,y2,color); }

    else if (sscanf(video_msg2, "text %d,%d %s", &x1, &y1, text_) == 3){

       text_length = strlen(text_);
       plot_chararcter(x1,y1,text_,text_length);}

    else if (strcmp(command, "erase") == 0){erase_text();}



	return bytes;
}*/

static int timer_handler(void) {
    int value;
    LW_virtual = ioremap_nocache (LW_BRIDGE_BASE, LW_BRIDGE_SPAN); //mapping the fpga lightweight bridge

    SW_ptr = LW_virtual + SW_BASE; //mapping the switch pointer

    LEDR_ptr = LW_virtual + LEDR_BASE; //map the ledr pointer

    HEX_3_0_ptr = LW_virtual + HEX3_HEX0_BASE; //map the hex display pointer

    pin_ptr = LW_virtual + GPIO0_BASE; //map the gpio pin pointer
    *(pin_ptr + 1) = 1;

    timer_ptr = LW_virtual + TIMER0_BASE; //map the timer pointer
    *(timer_ptr + 2) = 0x2D00; //loading a value into the lower start counter reg
    *(timer_ptr + 3) = 0x131; //loading a value onto the higher start counter reg
    *(timer_ptr + 1) = 0x7; //setting ITO, CONT, START bits to 1
    *(HEX_3_0_ptr) = 
    *(HEX_3_0_ptr) = hex[0] | (hex[1]<<8) | (hex[0] << 16) | (hex[0] << 24);

    value = request_irq (TIMER0_IRQ, (irq_handler_t) irq_handler, IRQF_SHARED,"irq_handler", (void *) (irq_handler)); //registering the interrupt
    
    return value;

}

irq_handler_t irq_handler(int irq, void *dev_id, struct pt_regs *regs) {

    int wave_frequency; //variable to store the frequency of the wave based on the value received on the switches
    int hex0, hex1, hex2, hex3; //store the value to be displayed on the respective hex displays
    int sw_val; //to store the value entered through the swithc

    sw_val = ((*(SW_ptr) >> 6) * 10) + 10;
    
    hex0 = sw_val %10; 		          
    hex1 = (sw_val %100)/10;		       
    hex2 = (sw_val %1000)/100;	        
    hex3 = (sw_val %10000)/1000;	        
    *(HEX_3_0_ptr) = hex[hex0]| (hex[hex1]<<8) | (hex[hex2] << 16) | (hex[hex3] << 24);

    gpio_pin_out = !gpio_pin_out;
    *pin_ptr = gpio_pin_out;

    *LEDR_ptr = *(SW_ptr);

    wave_frequency = 100000000 / (2*sw_val);
    *(timer_ptr + 2) = (wave_frequency & 0xFFFF);
    *(timer_ptr + 3) = ((wave_frequency >> 16 ) & 0xFFFF);
    *(timer_ptr + 1) = 0x7;

    *(timer_ptr) = 0; //clear the interrupt

    return (irq_handler_t) IRQ_HANDLED;
}

MODULE_LICENSE("GPL");
module_init (start_signal_generator);
module_exit (stop_signal_generator);