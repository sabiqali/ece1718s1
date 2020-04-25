#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
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
#define video_BYTES 8 // number of characters to read from /dev/video

#define RELEASED 0
#define PRESSED 1

#define KEY_BYTES 2
#define stopwatch_BYTES 8

static const char *const press_type[2] = {"RELEASED", "PRESSED "};

char *Note[] = {"C#", "D#", " ", "F#", "G#", "A#", "C", "D", "E", "F", "G", "A", "B", "C"};
char ASCII[] = {'2',  '3',  '4', '5',  '6',  '7',  'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I'};
double tone_output[14] = {0};

int volume = (int) MAX_VOLUME / 13;

 double freq_notes[14] = {Csharp_freq, Dsharp_freq, empty_freq, Fsharp_freq, Gsharp_freq, Asharp_freq,
                            C_low_freq, D_freq, E_freq, F_freq, G_freq, A_freq, B_freq, C_high_freq};

static pthread_t tid1, tid2;
pthread_mutex_t mutex_tone_output;

void *LW_virtual;					    //Lightweight bridge base address
volatile int *audio_ptr;	            // virtual address for Audio

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
int read_timer(int *stopwatch_fd);

int main(int argc, char *argv[]) {

    int current_time;
    int err;
    int k = 0;
    int j = 0;
    int i = 0;
    int press_count = 0;
    int rel_count = 0;
    int rec_done = 0;

	struct input_event ev;
	int fd_kb, event_size = sizeof (struct input_event), key;
	int key_fd, led_fd, hex_fd, stopwatch_fd;

	char key_buffer[KEY_BYTES];

	int key_val, key_read;

	int key0_toggle = 0;

	int elapsed_min = 0;
	int elapsed_sec = 0;
	int elapsed_ms = 0;
	int time_stamp_p[1024];
    int time_stamp_r[1024];

	char send_time[256];

	int check_press[1024] = {0};
    char keyboard_path[50];

	// catch SIGINT from ctrl+c, instead of having it abruptly close this program
  	signal(SIGINT, catchSIGINT);

    //Creat the pthreads
    if((err = pthread_create(&tid1, NULL, &audio_thread, NULL)) != 0) {
        printf("pthread_create failed:[%s]\n", strerror(err));
    }

    if((err = pthread_create(&tid2, NULL, &video_thread, NULL)) != 0) {
        printf("pthread_create failed:[%s]\n", strerror(err));
    }

	// Input keyboard path from user
    printf("Please input entire keyboard path: \n");
    scanf("%s",keyboard_path);

    // Open keyboard device
	if ((fd_kb = open (keyboard_path, O_RDONLY | O_NONBLOCK)) == -1) {
		printf ("Could not open %s\n", keyboard_path);
		return -1;
	}

    // Open the character device drivers for read/write
	if ((key_fd = open("/dev/IntelFPGAUP/KEY", O_RDONLY)) == -1){
		printf("Error opening /dev/KEY: %s\n", strerror(errno));
		return -1;
	}

	if ((led_fd = open("/dev/IntelFPGAUP/LEDR", O_WRONLY)) == -1){
		printf("Error opening /dev/LEDR: %s\n", strerror(errno));
		return -1;
	}

	if ((hex_fd = open("/dev/IntelFPGAUP/HEX", O_WRONLY)) == -1){
		printf("Error opening /dev/HEX: %s\n", strerror(errno));
		return -1;
	}

    if ((stopwatch_fd = open("/dev/stopwatch", O_RDWR)) == -1){
		printf("Error opening /dev/stopwatch: %s\n", strerror(errno));
		return -1;
	}

	//Assign to CPU0
	set_processor_affinity(0);

    //Send time value to /dev/stopwatch
    write (stopwatch_fd, "01:00:00", 8);
    write (stopwatch_fd, "disp", 4);
    write (stopwatch_fd, "stop", 4);

    while(!stop){
        //Read the driver until EOF
        key_read = 0;
        while ((key_val = read (key_fd, key_buffer, 1)) != 0)
            {key_read += key_val;}

		//if KEY0 is pressed
        if ((*(key_buffer)) & 0x01) {

            key0_toggle = !key0_toggle;

            if (key0_toggle) {

                //Send time value to /dev/stopwatch
                write (stopwatch_fd, "01:00:00", 8);
                write (stopwatch_fd, "disp", 4);
                write (led_fd, "1", 1);
                write (stopwatch_fd, "run", 3);

                //reset previous recording
                j = 0;
                k = 0;
                press_count = 0;
                rel_count = 0;
                rec_done = 0;
                memset(check_press, 0, sizeof(check_press));
                memset(time_stamp_p,0, sizeof(time_stamp_p));
                memset(time_stamp_r,0, sizeof(time_stamp_r));
            }

            else if (key0_toggle == 0){

                write (led_fd, "0", 1);
                write (stopwatch_fd, "stop", 4);

                //setting sound output to zero after key0 press
                pthread_mutex_lock (&mutex_tone_output);
                memset(tone_output, 0, sizeof(tone_output));
                pthread_mutex_unlock (&mutex_tone_output);

                //Fimd cuttent time when KEY0 is pressed
                current_time = read_timer(&stopwatch_fd);

                elapsed_min = 0;
                elapsed_sec = 59 - (current_time %10000)/100;
                elapsed_ms = 99 - (current_time %100);

                sprintf(send_time, "%d:%d:%d", elapsed_min, elapsed_sec, elapsed_ms);
                write (stopwatch_fd, send_time, strlen(send_time));
                rec_done = 1;
            }
        }

        //if KEY1 is pressed
        if (((*(key_buffer)) & 0x02) && rec_done) {

            j = 0;
            k = 0;
            write (led_fd, "2", 1);
            write (stopwatch_fd, send_time, strlen(send_time));
            write(stopwatch_fd, "disp", 4);
            write(stopwatch_fd, "run", 3);

            //Read /dev/stopwatch and loop while time != 0, if time = time_stamp, then output sound
           i = read_timer(&stopwatch_fd);

            while(i){

                if (i == (time_stamp_p[j] - current_time)){

                    press_flag = 1;

                    pthread_mutex_lock (&mutex_tone_output);
                    tone_output[check_press[j]] = volume;
                    pthread_mutex_unlock (&mutex_tone_output);
                    j++;
                }

                else if (i == (time_stamp_r[k] - current_time)){

                    press_flag = 1;
                    k++;
                }

                i = read_timer(&stopwatch_fd);
            }

        }

           // Read keyboard and if pressed set the press_flag to 1 (for video) and tone_output to volume (for audio)
            if (read (fd_kb, &ev, event_size) < event_size)
                continue;
            if (ev.type == EV_KEY && (ev.value == PRESSED)) {

                key = (int) ev.code;

                if (key > 2 && key < 9){

                    press_flag = 1;
                    printf("You %s key %c (note %s)\n", press_type[ev.value], ASCII[key-3],Note[key-3]);

                    //Only do recording while KEY0 has been pressed
                    if (key0_toggle == 1){

                        //Record key press and time of key press
                        check_press[press_count] = key-3;
                        time_stamp_p[press_count] = read_timer(&stopwatch_fd);
                        press_count++;
                    }

                    //Setting tone as volume or 0 for audio thread
                    pthread_mutex_lock (&mutex_tone_output);
                    tone_output[key-3] = volume;
                    pthread_mutex_unlock (&mutex_tone_output);
                }

                else if (key > 15 && key < 24){

                    press_flag = 1;
                    printf("You %s key %c (note %s)\n", press_type[ev.value], ASCII[key-10],Note[key-10]);

                    //Only do recording while KEY0 has been pressed
                    if (key0_toggle == 1){

                        //Record key press and time of key press
                        check_press[press_count] = key-10;
                        time_stamp_p[press_count] = read_timer(&stopwatch_fd);
                        press_count++;
                    }

                    //Setting tone as volume or 0 for audio thread
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
                if (key > 2 && key < 9){

                    press_flag = 1;
                    if (key0_toggle == 1){

                        time_stamp_r[rel_count] = read_timer(&stopwatch_fd);
                        rel_count++;
                     }
                }

                else if (key > 15 && key < 24){

                    press_flag = 1;
                    if (key0_toggle == 1){

                        time_stamp_r[rel_count] = read_timer(&stopwatch_fd);
                        rel_count++;
                     }
                }
            }

    }

    pthread_cancel(tid1);
    pthread_join(tid1, NULL);

    pthread_cancel(tid2);
    pthread_join(tid2, NULL);

    write(led_fd, "0", 1);
    write(stopwatch_fd, "nodisp", 6);

	close_physical(fd_kb);
    close_physical(key_fd);
    close_physical(led_fd);
    close_physical(hex_fd);
    close_physical(stopwatch_fd);

return 0;}

void *audio_thread(){

	//assign the audio thread to CPU1
	set_processor_affinity(1);


    int fd = -1;
    int sample = 0;
    int i;
    int sound;


	// Create virtual memory access to the FPGA light-weight bridge
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

        while ((((*(audio_ptr + FIFOSPACE) >> 16) & 0xFF) < 0x10) && (((*(audio_ptr + FIFOSPACE) >> 24) & 0xFF) < 0x10));

        sound = 0;

            for (i = 0; i < 14; i++){


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

void *video_thread(){

    //assign the video thread to CPU0
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


    // Open the character device driver
    if ((video_FD = open("/dev/video", O_RDWR)) == -1)
    {
        printf("Error opening /dev/video: %s\n", strerror(errno));
        return;
    }

    // Set screen_x and screen_y by reading from the driver
    video_read = 0;
	while ((video_val = read (video_FD, buffer, video_BYTES)) != 0)
		{video_read += video_val; }

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

            if (y_output > screen_y - 1){

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

//Function to read timer, called whenever we want to read current time value from /dev/stopwatch
int read_timer(int *stopwatch_fd){

    int stopwatch_read;
    int stopwatch_val;
    int current_time;

    char stopwatch_buffer[stopwatch_BYTES];
    int min, sec, ms;

    stopwatch_read = 0;
	while ((stopwatch_val = read (*stopwatch_fd, stopwatch_buffer, 8)) !=0)
		    {stopwatch_read += stopwatch_val;}
    stopwatch_buffer[stopwatch_read] = '\0';

    sscanf (stopwatch_buffer, "%2d:%2d:%2d", &min, &sec, &ms);

    current_time = 10000*min + 100*sec + ms;

    return current_time;

}
