#include <stdio.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>
#include <math.h>
#include <unistd.h>
#include <sys/mman.h>
#include <string.h>

#include "address_map_arm.h"
#include "audio.h"
#include "physical.h"

//Defining required audio frequencies

#define C_low_freq 261.626
#define Csharp_freq 277.183
#define D_freq 293.665
#define Dsharp_freq 311.127
#define E_freq 329.628
#define F_freq 349.228
#define Fsharp_freq 369.994
#define G_freq 391.995
#define Gsharp_freq 415.305
#define A_freq 440.000
#define Asharp_freq 466.164
#define B_freq 493.883
#define C_high_freq 523.251

volatile sig_atomic_t stop;
void catchSIGINT(int signum){
    stop = 1;
}


void *LW_virtual;					    //Lightweight bridge base address
volatile int *audio_ptr;	            // virtual address for Audio

int main(int argc, char *argv[]) {

    double notes[13] = {C_low_freq, Csharp_freq, 
                        D_freq, Dsharp_freq, 
                        E_freq, 
                        F_freq, Fsharp_freq, 
                        G_freq, Gsharp_freq,
                        A_freq, Asharp_freq, 
                        B_freq, 
                        C_high_freq};
    
    int fd = -1;
    int volume = (int) MAX_VOLUME / 13;
    int nth_sample, i, sound;
    char * input;
    int volume_of_tones[13];
    
    nth_sample = 0;      //Initializing value to 0

	// Create virtual memory access to the FPGA light-weight bridge
	if ((fd = open_physical (fd)) == -1)
		return (-1);
	if (!(LW_virtual = map_physical (fd, LW_BRIDGE_BASE, LW_BRIDGE_SPAN)))
 	return (-1);

	audio_ptr = (int *)(LW_virtual+AUDIO_BASE);	 //Set virtual address of Audio port

    *audio_ptr |= 0x8;
    *audio_ptr &= ~0x3;
    *audio_ptr &= ~0x8;

    input = argv[1];

    for (i = 0; i < strlen(input); i++){
        if (input[i] == '1'){
            volume_of_tones[i] = volume;
        }
        else{
            volume_of_tones[i] = 0;
        }
    }

    for (nth_sample = 0; nth_sample < 8000; nth_sample++){

        while ((((*(audio_ptr + FIFOSPACE) >> 16) & 0xFF) < 0x40) && (((*(audio_ptr + FIFOSPACE) >> 24) & 0xFF) < 0x40));
        sound = 0;

        for (i = 0; i < 14; i++){
            sound += (int) volume_of_tones[i] * sin(nth_sample* PI2 *notes[i]/ 8000);
        }

        *(audio_ptr + RDATA) = sound;
        *(audio_ptr + LDATA) = sound;
    }

    unmap_physical (LW_virtual, LW_BRIDGE_SPAN);
	close_physical(fd);

return 0;

}