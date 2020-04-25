#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include <signal.h>

#include "address_map_arm.h"
#include "physical.h"

void *LW_virtual;		//Lightweight bridge base address
volatile int *ADC_addr;  //Pointer defined for setting ADC's Virtual address

volatile sig_atomic_t stop;
void catchSIGINT(int signum){
    stop = 1;
}

int main(int argc, char *argv[]){
    int fd = -1;
    signal(SIGINT,catchSIGINT);

    if ((fd = open_physical (fd)) == -1)
		return (-1);
	
    if (!(LW_virtual = map_physical (fd, LW_BRIDGE_BASE, LW_BRIDGE_SPAN)))
        return (-1);

    ADC_addr = (int *)(LW_virtual + ADC_BASE);        //For setting ADC's Virtual address

    while(!stop){

        *ADC_addr = 1;
        printf("%d\n", *ADC_addr);

    }

    unmap_physical (LW_virtual, LW_BRIDGE_SPAN);
	close_physical(fd);
}
