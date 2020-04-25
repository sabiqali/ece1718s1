#define _GNU_SOURCE

#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <signal.h>
#include <linux/input.h>
#include <sys/mman.h>
#include <string.h>

#include <time.h>
#include <sched.h>
#include "address_map_arm.h"
#include "physical.h"

// Defines
#define VIDEO_BYTES 8          //Bytes to read from /dev/video
#define VIDEO_COLOUR 0xFFFF    //Setting colour of waveforms
#define COMMAND_STR_SIZE 40    //Defining size of command to write
#define KEY_BYTES 2            //Total characters to read from /dev/KEY

//Function declarations
void timeout_handler (int );  //Declaring function of timeout handler
void clear_screen(void);      //Declaring function to clear screen
void free_resources(void);    //Declaring function to free resources when error is encountered

int screen_x, screen_y, x_output, y_output;
char commands_to_video[64];     //For sending commands to /dev/video
int video_FD;                   //File descriptor
int key_fd;
int count = 0;
int x_prev = 0;
int y_prev = 0;
int value_ADC[500] = {0};    //Variable to hold value received from ADC
int fd = -1;

void *LW_virtual;		     //Lightweight bridge base address
volatile int *ADC_addr;      //Pointer for ADC's virtual address
volatile int *SW_addr;	     //Pointer for virtual address of switches

volatile sig_atomic_t stop;
void catchSIGINT(int signum){
    stop = 1;
}

struct itimerspec interval_timer_start = {
    .it_interval = {.tv_sec=0,.tv_nsec=312500},
    .it_value = {.tv_sec=0,.tv_nsec=312500}};

struct itimerspec interval_timer_stop = {
    .it_interval = {.tv_sec=0,.tv_nsec=0},
    .it_value = {.tv_sec=0,.tv_nsec=0}};
timer_t interval_timer_id;

int main(int argc, char *argv[]){

    int ADC_value, ADC_value_prev;
    int init_trigger = 1;
    int i, j, edge;

    int read_video;
    
    int video_val = 0;

    char read_data[VIDEO_BYTES];      //For reading data from /dev/video
    
    int time_new;

    int key_val, key_read;
	char read_key[KEY_BYTES];        //For reading data from /dev/KEY
	
    signal(SIGINT,catchSIGINT);

    if ((fd = open_physical (fd)) == -1)
		return (-1);
	if (!(LW_virtual = map_physical (fd, LW_BRIDGE_BASE, LW_BRIDGE_SPAN)))
        return (-1);

    // Open the character device driver
    if ((video_FD = open("/dev/IntelFPGAUP/video", O_RDWR)) == -1)
    {
        printf("Error opening /dev/video: %s\n", strerror(errno));
        free_resources();
        return -1;
    }

    // Open the character device drivers for read/write
	if ((key_fd = open("/dev/IntelFPGAUP/KEY", O_RDONLY)) == -1){
		printf("Error opening /dev/KEY: %s\n", strerror(errno));
		free_resources();
        return -1;
	}

    clear_screen();

    // Set screen_x and screen_y by reading from the driver
    read_video = 0;
	
    while ((video_val = read (video_FD, read_data, VIDEO_BYTES)) != 0)
		{read_video += video_val; }

    read_data[read_video] = '\0';
    
    sscanf(read_data,"%d %d", &screen_x, &screen_y);
    clear_screen();

    // Set up the signal handling
    struct sigaction act;
    sigset_t set;
    sigemptyset (&set);
    sigaddset (&set, SIGALRM);
    act.sa_flags = 0;
    act.sa_mask = set;
    act.sa_handler = &timeout_handler;
    sigaction (SIGALRM, &act, NULL);

    // Create a monotonically increasing timer
    timer_create (CLOCK_MONOTONIC, NULL, &interval_timer_id);

    //SW_addr is given the address for Switch
    SW_addr = (int *)(LW_virtual + SW_BASE);

    ADC_addr = (int *)(LW_virtual + ADC_BASE);    //Set virtual address of ADC
    *(ADC_addr + 1) = 1;
    
    ADC_value_prev = *ADC_addr & 0xFFF;

    while(!stop){

        ADC_value = *ADC_addr & 0xFFF;
        
        edge = *SW_addr & 0x1;

        //Read the driver until EOF
        key_read = 0;
        while ((key_val = read (key_fd, read_key, 1)) != 0)
            {key_read += key_val;}

		//KEY0 pressed
        if ((*(read_key)) & 0x01) {
            j++;
            if (j > 4)
            { 
                j = 4;
            }

            time_new = (int)(((100.0 + 100.0*j)/320.0) * 1000000);   //Calculating new time to increase sweep time

            timer_settime (interval_timer_id, 0, &interval_timer_stop, NULL);
            interval_timer_start.it_interval.tv_nsec = time_new;
            interval_timer_start.it_value.tv_nsec = time_new;
            timer_settime (interval_timer_id, 0, &interval_timer_start, NULL);
        }

        //if KEY1 is pressed
        if ((*(read_key)) & 0x02) {
            j--;
            if (j < 0)
            { 
                j = 0;
            }

            time_new = (int)(((100.0 + 100.0*j)/320.0) * 1000000);  //Calculating new time to decrease sweep time
            
            timer_settime (interval_timer_id, 0, &interval_timer_stop, NULL);
            interval_timer_start.it_interval.tv_nsec = time_new;
            interval_timer_start.it_value.tv_nsec = time_new;
            timer_settime (interval_timer_id, 0, &interval_timer_start, NULL);

        }

        //For a rising edge
        if (edge && (ADC_value > (ADC_value_prev+50)) && init_trigger){
            timer_settime(interval_timer_id, 0, &interval_timer_start, NULL);   //To start timer
            init_trigger = 0;
        }
        
        //For a falling edge
        else if (!edge && ((ADC_value+50) < ADC_value_prev) && init_trigger){
            timer_settime (interval_timer_id, 0, &interval_timer_start, NULL);  //To start timer
            init_trigger = 0;
        }

        ADC_value_prev = ADC_value;
        
        y_output = (int)(screen_y - (((double)ADC_value / 4095.0) * (screen_y - 1)));

        if (x_output > (screen_x)){

            timer_settime (interval_timer_id, 0, &interval_timer_stop, NULL);

            clear_screen();

            for(i = 0; i < (screen_x); i++)
            {
                sprintf (commands_to_video, "line %d,%d %d,%d %x\n", i, value_ADC[i], i + 1, value_ADC[i+1], VIDEO_COLOUR);
                write (video_FD, commands_to_video, strlen(commands_to_video));
            }
            write (video_FD, "sync", COMMAND_STR_SIZE);
            
            //Resetting values
            init_trigger = 1;
            x_prev = 0;
            x_output = 0;
            y_prev = screen_y - 1;
            memset(value_ADC, 0, sizeof(value_ADC));
       }
    }

    timer_settime (interval_timer_id, 0, &interval_timer_stop, NULL);  //To stop the timer

    unmap_physical (LW_virtual, LW_BRIDGE_SPAN);
    close (video_FD);
	close_physical(fd);
}

// Handler function that is called when a timeout occurs
void timeout_handler (int signo){
    value_ADC[x_output] = y_output;
    x_prev = x_output;
    y_prev = y_output;
    x_output++;
    count++;
}

//Function to clear screen
void clear_screen(void){
  write (video_FD, "clear", COMMAND_STR_SIZE);
  write (video_FD, "sync", COMMAND_STR_SIZE);
  write (video_FD, "clear", COMMAND_STR_SIZE);
}

//Function to free resources when encountered with an error
void free_resources(void){
  if (LW_virtual != NULL)
    unmap_physical (LW_virtual, LW_BRIDGE_SPAN);

  if (fd != -1)
	  close_physical(fd);
  
  if (video_FD != -1)
    close (video_FD);

  if (key_fd != -1)
    close (key_fd);
}
