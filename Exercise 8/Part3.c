#define _GNU_SOURCE

#include <stdio.h>
#include <signal.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <math.h>
#include <string.h>
#include <linux/input.h>
#include <pthread.h>
#include <sched.h>

#include "audio.h"
#include "physical.h"
#include "address_map_arm.h"

//#define VOLUME 0x7fffffff/13

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
#define empty_freq 0

#define RELEASED 0
#define PRESSED 1
static const char *const press_type[2] = {"RELEASED", "PRESSED "};

char *Note[] = {"C#", "D#", " ", "F#", "G#", "A#", "C", "D", "E", "F", "G", "A", "B", "C"};
char ASCII[] = {'2',  '3',  '4', '5',  '6',  '7',  'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I'};
int tone_volumes[14] = {0};
double tone_fade_factor[14] = {0};
int volume = (int) MAX_VOLUME / 13;

 double freq_notes[14] = {Csharp_freq, Dsharp_freq, empty_freq, Fsharp_freq, Gsharp_freq, Asharp_freq,
                            C_low_freq, D_freq, E_freq, F_freq, G_freq, A_freq, B_freq, C_high_freq};

static pthread_t tid;
pthread_mutex_t mutex_tone_volume;

void *LW_virtual;					    //Lightweight bridge base address
volatile int *audio_ptr;	            // virtual address for Audio


volatile sig_atomic_t stop;
void catchSIGINT(int signum){
    stop = 1;
    pthread_cancel(tid);
}



// Set the current thread's affinity to the core specified
int set_processor_affinity(unsigned int core) {
    cpu_set_t cpuset;
    pthread_t current_thread = pthread_self();

    if (core >= sysconf(_SC_NPROCESSORS_ONLN)){
        printf("CPU Core %d does not exist!\n", core);
        return -1;
    }
    // Zero out the cpuset mask
    CPU_ZERO(&cpuset);
    // Set the mask bit for specified core
    CPU_SET(core, &cpuset);

    return pthread_setaffinity_np(current_thread, sizeof(cpu_set_t), &cpuset);
}

void *audio_thread();

int main(int argc, char *argv[]) {

    int err;

	struct input_event ev;
	int fd_kb, event_size = sizeof (struct input_event), key;

	char keyboard_path[50];

    //Input keyboard path from user
    printf("Please input entire keyboard path: \n");
    scanf("%s",keyboard_path);

	// Open keyboard device
	if ((fd_kb = open (keyboard_path, O_RDONLY | O_NONBLOCK)) == -1) {
		printf ("Could not open %s\n", keyboard_path);
		return -1;
	}

    if((err = pthread_create(&tid, NULL, &audio_thread, NULL)) != 0) {
        printf("pthread_create failed:[%s]\n", strerror(err));
    }

	//assign the main thread to CPU0
	set_processor_affinity(0);

    while(!stop){
        // Read keyboard
        if (read (fd_kb, &ev, event_size) < event_size)
			continue;
		if (ev.type == EV_KEY && (ev.value == PRESSED)) {

			key = (int) ev.code;
			if (key > 2 && key < 9) {
				printf("You %s key %c (note %s)\n", press_type[ev.value], ASCII[key-3],Note[key-3]);
                pthread_mutex_lock (&mutex_tone_volume);
				tone_volumes[key-3] = volume;
				pthread_mutex_unlock (&mutex_tone_volume);
			}

			else if (key > 15 && key < 24){
				printf("You %s key %c (note %s)\n", press_type[ev.value], ASCII[key-10],Note[key-10]);

                pthread_mutex_lock (&mutex_tone_volume);
				tone_volumes[key-10] = volume;
				pthread_mutex_unlock (&mutex_tone_volume);
			}
			else
				printf("You %s key code 0x%04x\n", press_type[ev.value], key);
 		}
    }

    pthread_cancel(tid);
    pthread_join(tid, NULL);
	close_physical(fd_kb);

    return 0;
}

void *audio_thread(){

	//assign the audio thread to CPU1
	set_processor_affinity(1);

    int fd = -1;
    int nth_sample = 0;
    int i, j;
    int sound;
    int low_volume = 0;

    low_volume = (int) volume * 0.5; //pow(0.25,4);

	// Create virtual memory access to the FPGA light-weight bridge
	if ((fd = open_physical (fd)) == -1)
		return;
	if (!(LW_virtual = map_physical (fd, LW_BRIDGE_BASE, LW_BRIDGE_SPAN)))
        return;

	audio_ptr = (int *)(LW_virtual+AUDIO_BASE);	 //Set virtual address of Audio port

    *audio_ptr |= 0x8;
    *audio_ptr &= ~0x3;
    *audio_ptr &= ~0x8;

    int sample = 0;

    while(1) {
        pthread_testcancel();

        while ((((*(audio_ptr + FIFOSPACE) >> 16) & 0xFF) < 0x10) && (((*(audio_ptr + FIFOSPACE) >> 24) & 0xFF) < 0x10));
        sound = 0;

        for (i = 0; i < 14; i++){
            
            sound += (int) (tone_volumes[i] * sin(sample * PI2 *freq_notes[i]/ 8000));

            pthread_mutex_lock(&mutex_tone_volume);
            tone_volumes[i] *= 0.9997;
            pthread_mutex_unlock(&mutex_tone_volume);
        }

        *(audio_ptr + RDATA) = sound;
        *(audio_ptr + LDATA) = sound;
        sample++;
    }

    unmap_physical (LW_virtual, LW_BRIDGE_SPAN);
    close_physical(fd);

}
