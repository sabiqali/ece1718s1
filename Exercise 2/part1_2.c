//#include "physical.h"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/mman.h>
#include  "./address_map_arm.h"

int hex(char a){

	switch (a){
		case '0': return 0x3F;
		case '1': return 0x06;
		case '2': return 0x5B;
		case '3': return 0x4F;
		case '4': return 0x66;
		case '5': return 0x6D;
		case '6': return 0x7D;
		case '7': return 0x07;
		case '8': return 0x7F;
		case '9': return 0x6F;
		case ' ': return 0x00;
		case 'A': return 0x77;
		case 'a': return 0x5F;
		case 'B': return 0x7F;
		case 'b': return 0x7C;
		case 'C': return 0x39;
		case 'c': return 0x58;
		case 'D': return 0x3F;
		case 'd': return 0x5E;
		case 'E': return 0x79;
		case 'e': return 0x6f;//Put in intitial value
		case 'F': return 0x71;
		case 'f': return 0x71;
		case 'G': return 0x3D;
		case 'g': return 0x3D;
		case 'H': return 0x76;
		case 'h': return 0x74;
		case 'I': return 0x30;
		case 'i': return 0x10;
		case 'J': return 0x1E;
		case 'j': return 0x1E;
		case 'K': return 0x00;
		case 'k': return 0x00;
		case 'L': return 0x38;
		case 'l': return 0x30;
		case 'M': return 0x00;
		case 'm': return 0x00;
		case 'N': return 0x00;
		case 'n': return 0x54;
		case 'O': return 0x3F;
		case 'o': return 0x5C;
		case 'P': return 0x73;
		case 'p': return 0x73;
		case 'Q': return 0x67;
		case 'q': return 0x67;
		case 'R': return 0x50;
		case 'r': return 0x50;
		case 'S': return 0x6D;
		case 's': return 0x6D;
		case 'T': return 0x78;
		case 't': return 0x78;
		case 'U': return 0x3E;
		case 'u': return 0x1C;
		case 'V': return 0x00;
		case 'v': return 0x00;
		case 'W': return 0x00;
		case 'w': return 0x00;
		case 'X': return 0x00;
		case 'x': return 0x00;
		case 'Y': return 0x6E;
		case 'y': return 0x6E;
		case 'Z': return 0x00;
		case 'z': return 0x00;
	}
}

int main() {

	
	//char msg[] = "intEL SoC FPGA ";
	char msg[20];
	printf("Enter A String");
	scanf("%19[^\n]", msg);
	int fd = -1;
	void *LW_virtual;					//Lightweight bridge base address
	volatile int *HEX_3_0_ptr,*HEX_4_5_ptr, *KEY_ptr;	//Virtual addresses of HEX and KEY
	volatile int *LED_ptr;

    	struct timespec ts;
        	ts.tv_sec = 1;
        	ts.tv_nsec = 0L;



	// Create virtual memory access to the FPGA light-weight bridge
	if ((fd = open( "/dev/mem", (O_RDWR | O_SYNC))) == -1)
		return (-1);
	void *virtual_base;

	// Get a mapping from physical addresses to virtual addresses
	LW_virtual = mmap (NULL,LW_BRIDGE_SPAN , (PROT_READ | PROT_WRITE), MAP_SHARED, fd, LW_BRIDGE_BASE);
	if (LW_virtual == MAP_FAILED)
 	return (-1);

	HEX_3_0_ptr = (int *)(LW_virtual+HEX3_HEX0_BASE);	 //Set virtual address of HEX 0 to 3
	HEX_4_5_ptr = (int *)(LW_virtual+HEX5_HEX4_BASE);	 //Set virtual address of HEX 4 and 5	
	KEY_ptr = (int *)(LW_virtual+KEY_BASE);			 //Set virtual address of Keys
	*(KEY_ptr+3) = 0xF;
	
	*HEX_3_0_ptr = 0;
	*HEX_4_5_ptr = 0;
	
	int i = 0;	
	int j = 0;
	int a = 1;
	
	while(1) {
	
	

	if (*(KEY_ptr + 3) > 0){
	a  = !a;
	 *(KEY_ptr+3) = 0xF;
	}
	

	if (a){
	int alph = hex(msg[i]);
	*HEX_3_0_ptr = alph | *HEX_3_0_ptr;
	nanosleep(&ts , NULL);
	
	i++;
	j++;
	*HEX_3_0_ptr = *HEX_3_0_ptr << 8;
	*HEX_4_5_ptr = *HEX_4_5_ptr << 8;

	if (j > 3){
		
		if (j > (strlen(msg) + 3)){j = 4;}
		int alph2 = hex(msg[j-4]);
		*HEX_4_5_ptr = alph2 | *HEX_4_5_ptr;
		
	}

	if(i==strlen(msg)){
		i = 0;

	}
	
	}
	
	}
	//unmap_physical (LW_virtual, LW_BRIDGE_SPAN);
	munmap (LW_virtual, LW_BRIDGE_SPAN);
	close(fd);
	//close_physical(fd);


return 0;}
