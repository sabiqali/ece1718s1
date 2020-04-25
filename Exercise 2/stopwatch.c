#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <asm/io.h>
#include "./address_map_arm.h"
//#include "interrupt_ID.h"
//timer0
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Altera University Program");
MODULE_DESCRIPTION("DE1SoC Pushbutton Interrupt Handler");

int min = 59; int sec = 59; int ms = 99;
int time;

void * LW_virtual;         // Lightweight bridge base address
//volatile int *LEDR_ptr;    // virtual address for the LEDR port
volatile int *timer_ptr;   // virtual address for timer
volatile int *HEX_3_0_ptr; // virtual address for HEX 0 to 3
volatile int *HEX_4_5_ptr; // virtual address for HEX 4 and 5
volatile int *KEY_ptr;	   // virtual address for Key buttons
volatile int *SW_ptr;	   // virtual address for SW buttons


int hex(int num){
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

irq_handler_t timer_irq_handler(int irq, void *dev_id, struct pt_regs *regs)
{

*(timer_ptr) = 0;

int disp0, disp1, disp2, disp3, disp4, disp5;    
int temp;
if (time >0) {time--;}

temp = time % 10000; //get seconds and miliseconds

if  (temp == 9999) { time = time  - 9999 + 5999;}


disp0 = time %10; 		//miliseconds first digit
disp1 = (time %100)/10;		//miliseconds second digit
disp2 = (time %1000)/100;	//seconds first digit
disp3 = (time %10000)/1000;	//seconds second digit
disp4 = (time %100000)/10000;	//minutes first digit
disp5 = (time %1000000)/100000;	//minutes second digit

ms =  disp0 + (disp1*10); 
sec = disp2 + (disp3*10);
min = disp4 + (disp5*10);


*(HEX_3_0_ptr) = hex(disp0)| (hex(disp1)<<8) | (hex(disp2) << 16) | (hex(disp3) << 24); 
*(HEX_4_5_ptr) = hex(disp4) | (hex(disp5) <<8);

   return (irq_handler_t) IRQ_HANDLED;
}

irq_handler_t key_irq_handler(int irq, void *dev_id, struct pt_regs *regs)
{


if (*(KEY_ptr+3) & 0x1){  
if (*(timer_ptr +1) == 0x7){ *(timer_ptr+1) = 0xB;}
else {*(timer_ptr+1) = 0x7;} 
}

if (*(KEY_ptr+3) & 0x2){
ms = *SW_ptr;
if (ms > 99) {ms = 99;}
}

if (*(KEY_ptr+3) & 0x4){
sec = *SW_ptr;
if (sec > 59) {sec = 59;}
}

if (*(KEY_ptr+3) & 0x8){
min = *SW_ptr;
if (min > 59) {min = 59;}
}

time = 10000*min + 100*sec + ms;
*(KEY_ptr + 3) = 0xF;
// *LEDR_ptr = 0xF;
   return (irq_handler_t) IRQ_HANDLED;
}

static int __init initialize_stopwatch(void)
{
   time = 10000*min + 100*sec + ms;
   int value; // value2;
   // generate a virtual address for the FPGA lightweight bridge
   LW_virtual = ioremap_nocache (LW_BRIDGE_BASE, LW_BRIDGE_SPAN); 
   
   timer_ptr = LW_virtual + TIMER0_BASE;
   HEX_3_0_ptr = LW_virtual + HEX3_HEX0_BASE;
   HEX_4_5_ptr = LW_virtual + HEX5_HEX4_BASE;
//   LEDR_ptr = LW_virtual +LEDR_BASE;
   KEY_ptr = LW_virtual + KEY_BASE;
   SW_ptr = LW_virtual +SW_BASE;

//  *LEDR_ptr = 0;
  *HEX_3_0_ptr = 0;
  *HEX_4_5_ptr = 0;
  *(timer_ptr + 2) = 0x4240;
  *(timer_ptr + 3) =0xF;
  *(timer_ptr + 1) = 0x7; 	   
  *(KEY_ptr + 2) = 0xF;
  *(KEY_ptr + 3) = 0xF;
 // Register the interrupt handler.
  value = request_irq (72, (irq_handler_t) timer_irq_handler, IRQF_SHARED, 
      "timer_irq_handler", (void *) (timer_irq_handler));

  value = request_irq (73, (irq_handler_t) key_irq_handler, IRQF_SHARED, 
      "key_irq_handler", (void *) (key_irq_handler));
   return value;

}

static void __exit cleanup_stopwatch(void)
{
   free_irq (72, (void*) timer_irq_handler);
   free_irq (73, (void*) key_irq_handler);
//   iounmap (*LW_virtual);
//   *LEDR_ptr = 0;
   *(HEX_3_0_ptr) = 0;
   *(HEX_4_5_ptr) = 0;	
	
}

module_init(initialize_stopwatch);
module_exit(cleanup_stopwatch);
