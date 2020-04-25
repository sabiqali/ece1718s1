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

//Set frequency values of the notes
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
#define video_BYTES 8 // number of characters to read from /dev/video

#define RELEASED 0
#define PRESSED 1

static const char *const press_type[2] = {"RELEASED", "PRESSED "};

char *Note[] = {"C#", "D#", " ", "F#", "G#", "A#", "C", "D", "E", "F", "G", "A", "B", "C"};
char ASCII[] = {'2',  '3',  '4', '5',  '6',  '7',  'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I'};

double tone_output[14] = {0};

int volume = (int) MAX_VOLUME / 13;

 double freq_notes[14] = {Csharp_freq, Dsharp_freq, empty_freq, Fsharp_freq, Gsharp_freq, Asharp_freq,
                            C_low_freq, D_freq, E_freq, F_freq, G_freq, A_freq, B_freq, C_high_freq};

static pthread_t tid1, tid2;
pthread_mutex_t mutex_tone_output;

void *LW_virtual;					    
volatile int *audio_ptr; //Virtual Address for audio           

//Flag set to 1 when key pressed on keyboard
int press_flag = 0;

volatile sig_atomic_t stop;
void catchSIGINT(int signum){
    stop = 1;
    pthread_cancel(tid1);
    pthread_cancel(tid2);
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

void *video_thread();

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

    if((err = pthread_create(&tid1, NULL, &audio_thread, NULL)) != 0) {
        printf("pthread_create failed:[%s]\n", strerror(err));
    }

    if((err = pthread_create(&tid2, NULL, &video_thread, NULL)) != 0) {
        printf("pthread_create failed:[%s]\n", strerror(err));
    }

	//Assign to CPU0
	set_processor_affinity(0);

    while(!stop){
        // Read keyboard and if pressed set the press_flag to 1 (for video) and tone_output to volume (for audio)
        if (read (fd_kb, &ev, event_size) < event_size)
			continue;
		
        if (ev.type == EV_KEY && (ev.value == PRESSED)) {
			
            key = (int) ev.code;

			if (key > 2 && key < 9) {
                press_flag = 1;
				printf("You %s key %c (note %s)\n", press_type[ev.value], ASCII[key-3],Note[key-3]);

                pthread_mutex_lock (&mutex_tone_output);
				tone_output[key-3] = volume;
				pthread_mutex_unlock (&mutex_tone_output);
			}
			else if (key > 15 && key < 24) {
                press_flag = 1;
				printf("You %s key %c (note %s)\n", press_type[ev.value], ASCII[key-10],Note[key-10]);

                pthread_mutex_lock (&mutex_tone_output);
				tone_output[key-10] = volume;
				pthread_mutex_unlock (&mutex_tone_output);
			}
			else
				printf("You %s key code 0x%04x\n", press_type[ev.value], key);
		}

        //Check keyboard releases and output the sine wave to video
		if (ev.type == EV_KEY && (ev.value == RELEASED)) {
			key = (int) ev.code;

			if (key > 2 && key < 9) {
                press_flag = 1;
            }

			else if (key > 15 && key < 24) {
                press_flag = 1;
			}
		}
    }

    pthread_cancel(tid1);
    pthread_join(tid1, NULL);

    pthread_cancel(tid2);
    pthread_join(tid2, NULL);

	close_physical(fd_kb);

    return 0;
}


void *audio_thread() {

	//Assign to CPU1
	set_processor_affinity(1);

    int fd = -1;
    int sample = 0;
    int i;
    int sound;

	if ((fd = open_physical (fd)) == -1)
		return;
	if (!(LW_virtual = map_physical (fd, LW_BRIDGE_BASE, LW_BRIDGE_SPAN)))
        return;

	audio_ptr = (int *)(LW_virtual+AUDIO_BASE);	 //Set virtual address of Audio port

    *audio_ptr |= 0x8;
    *audio_ptr &= ~0x3;
    *audio_ptr &= ~0x8;

    //Produce the sine wave, combine multiple if multiple keypresses and output to audio port + fade in every loop
    while(1){

        pthread_testcancel();

        //Wait till the FIFO emptoes out
        while ((((*(audio_ptr + FIFOSPACE) >> 16) & 0xFF) < 0x10) && (((*(audio_ptr + FIFOSPACE) >> 24) & 0xFF) < 0x10));

        sound = 0;

        for (i = 0; i < 14; i++) {
            sound += (int) (tone_output[i] * sin(sample * PI2 *freq_notes[i]/ 8000));

            pthread_mutex_lock(&mutex_tone_output);
            tone_output[i] *= 0.9997;
            pthread_mutex_unlock(&mutex_tone_output);
        }

        *(audio_ptr + RDATA) = sound;
        *(audio_ptr + LDATA) = sound;
        sample++;
    }

    unmap_physical (LW_virtual, LW_BRIDGE_SPAN);
    close_physical(fd);
}

void *video_thread() {

    //Assign to CPU0
	set_processor_affinity(0);

    int screen_x, screen_y;
    int video_FD; // file descriptor
    char buffer[video_BYTES]; // buffer for data read from /dev/video
    char command[64]; // buffer for commands written to /dev/video
    int video_read;
    int video_val = 0;
    int x_output = 1;
    int y_output = 0;
    int color = 0x07E0;
    int i;
    int x_prev = 0;
    int y_prev;

    //Open the video driver
    if ((video_FD = open("/dev/video", O_RDWR)) == -1)
    {
        printf("Error opening /dev/video: %s\n", strerror(errno));
        return;
    }

    // Set screen_x and screen_y by reading from the driver
    video_read = 0;
	while ((video_val = read (video_FD, buffer, video_BYTES)) != 0) {
        video_read += video_val; 
    }

    buffer[video_read] = '\0';
    sscanf(buffer,"%d %d", &screen_x, &screen_y);

    y_prev = screen_y/2;

    write (video_FD, "sync", 4);
    write (video_FD, "clear", 5);

    //Output sine wave to screen, ensure output is within screen max limits
    while(1){
        pthread_testcancel();

        if(press_flag){
            write (video_FD, "clear", 5);

        while (x_output < screen_x){

            for (i = 0; i < 14; i++){
                y_output +=  (int) (((screen_y/2) - 40) * (tone_output[i] / volume) * sin(x_output * PI2 *freq_notes[i]/ 8000));
            }

            y_output += screen_y/2;

            if (y_output > screen_y - 1) {
                y_output = screen_y-1;
            }

            if (y_output < 1){
                y_output = 1;
            }

            sprintf (command, "line %d,%d %d,%d %x\n", x_prev, y_prev, x_output, y_output, color);
            write (video_FD, command, strlen(command));

            x_prev = x_output;
            y_prev = y_output;

            x_output++;
            y_output = 0;
        }

        x_prev = 0;
        y_prev = screen_y/2;
        x_output = 1;
        press_flag = 0;
        }

    }

    close (video_FD);

}