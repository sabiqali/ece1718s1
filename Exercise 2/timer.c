#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/init.h>
#include <linux/interrupt.h>
#include <asm/io.h>
#include "./address_map_arm.h"
//#include "interrupt_ID.h"

int time = 0;
void * LW_virtual;         // Lightweight bridge base address
//volatile int *LEDR_ptr;    // virtual address for the LEDR port
volatile int *timer_ptr;   // virtual address for timer
volatile int *HEX_3_0_ptr; // virtual address for HEX 0 to 3
volatile int *HEX_4_5_ptr; // virtual address for HEX 4 and 5

int hex(int num){
	switch(num){
		case 0: return 0x3F;
		case 1: return 0x06;
		case 2: return 0x5B;
		case 3: return 0x4F;
		case 4: return 0x66;
		case 5: return 0x6D;
		case 6: return 0x7D;
		case 7: return 0x07;
		case 8: return 0x7F;
		case 9: return 0x67;
	}
}

irq_handler_t irq_handler(int irq, void *dev_id, struct pt_regs *regs) {

	*(timer_ptr) = 0;

	int disp0, disp1, disp2, disp3, disp4, disp5;    
	int temp, temp2;
	time++;

	temp = time % 10000; //get seconds and miliseconds

	if  (temp == 6000) { time += 10000 - 6000;}
	if  (time == 600000) {time = 0;}// *LEDR_ptr = 0xFF; }

	disp0 = time %10; 		//miliseconds first digit
	disp1 = (time %100)/10;		//miliseconds second digit
	disp2 = (time %1000)/100;	//seconds first digit
	disp3 = (time %10000)/1000;	//seconds second digit
	disp4 = (time %100000)/10000;	//minutes first digit
	disp5 = (time %1000000)/100000;	//minutes second digit


	*(HEX_3_0_ptr) = hex(disp0)| (hex(disp1)<<8) | (hex(disp2) << 16) | (hex(disp3) << 24); 
	*(HEX_4_5_ptr) = hex(disp4) | (hex(disp5) <<8);

	return (irq_handler_t) IRQ_HANDLED;
}

static int __init initialize_timer_handler(void) {
   
	int value;
	// generate a virtual address for the FPGA lightweight bridge
   	LW_virtual = ioremap_nocache (LW_BRIDGE_BASE, LW_BRIDGE_SPAN); 
   
   	timer_ptr = LW_virtual + TIMER0_BASE;
   	HEX_3_0_ptr = LW_virtual + HEX3_HEX0_BASE;
   	HEX_4_5_ptr = LW_virtual + HEX5_HEX4_BASE;
	//   LEDR_ptr = LW_virtual +LEDR_BASE;
	//*LEDR_ptr = 0;
	*HEX_3_0_ptr = 0;
	*HEX_4_5_ptr = 0;
	*(timer_ptr + 2) = 0x4240;
	*(timer_ptr + 3) =0xF;
	*(timer_ptr + 1) = 0x7; 	   

 	// Register the interrupt handler.
   	value = request_irq (72, (irq_handler_t) irq_handler, IRQF_SHARED, 
      	"irq_handler", (void *) (irq_handler));
   	return value;

}

static void __exit cleanup_timer_handler(void) {
   
	free_irq (72, (void*) irq_handler);
	*(HEX_3_0_ptr) = 0;
	*(HEX_4_5_ptr) = 0;	
	
}

module_init(initialize_timer_handler);
module_exit(cleanup_timer_handler);


