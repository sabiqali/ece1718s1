#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include "address_map_arm.h"


#define BYTES 256
#define KEY_BYTES 2
#define SW_BYTES 4
#define stopwatch_BYTES 8

volatile sig_atomic_t stop;
void catchSIGINT(int signum){
    stop = 1;
}

//global variables to store the min, sec and ms values
char min[2], sec[2], ms[2];

//Main function
int main(int argc, char *argv[]){

    //File descriptors for character device drivers
	int key_fd;
	int sw_fd;
	int ledr_fd;
	int stopwatch_fd;

    //variable to hold "stop" or "run" condition
    char * stopwatch_condition;
    //variable to hold mins, secs or ms
    char send_time[BYTES];

    int key0_press = 0;

    //Buffers for character data
  	char key_buffer[KEY_BYTES];
  	char sw_buffer[SW_BYTES];
  	char stopwatch_buffer[BYTES];
	int key_val, sw_val, key_read, sw_read;
    int stopwatch_val, stopwatch_read;

    //variable to hold sw_buffer value when converted to long from string
    long number = 0;


  	// catch SIGINT from ctrl+c, instead of having it abruptly close this program
  	signal(SIGINT, catchSIGINT);

	// Open the character device drivers for read/write
	if ((key_fd = open("/dev/key", O_RDONLY)) == -1)
	{
		printf("Error opening /dev/key: %s\n", strerror(errno));
		return -1;
	}

    if ((sw_fd = open("/dev/sw", O_RDONLY)) == -1)
	{
		printf("Error opening /dev/sw: %s\n", strerror(errno));
		return -1;
	}

	if ((ledr_fd = open("/dev/ledr", O_WRONLY)) == -1)
	{
		printf("Error opening /dev/ledr: %s\n", strerror(errno));
		return -1;
	}

	if ((stopwatch_fd = open("/dev/stopwatch", O_RDWR)) == -1)
	{
		printf("Error opening /dev/stopwatch: %s\n", strerror(errno));
		return -1;
	}


    //Initial conditions LEDs are off
    write (ledr_fd, sw_buffer, BYTES);


    //Initial condition, the stopwatch is stopped, initial time is 59:59:99 and disp is on
    sprintf(send_time, "59:59:99");
    write (stopwatch_fd, send_time, strlen(send_time));
    write (stopwatch_fd, "disp", 4);
    write (stopwatch_fd, "stop", 4);


   //Run continuously until stop command
  	while (!stop)
	{

        write(ledr_fd,sw_buffer,BYTES);
        //Read the driver until EOF
		key_read = 0;
		while ((key_val = read (key_fd, key_buffer, KEY_BYTES)) != 0)
			{key_read += key_val; }

        //Read the driver until EOF
        sw_read = 0;
		while ((sw_val = read (sw_fd, sw_buffer, SW_BYTES)) != 0)
			{sw_read += sw_val; }

        //Read the driver until EOF
        stopwatch_read = 0;
		while ((stopwatch_val = read (stopwatch_fd, stopwatch_buffer, 8))  != 0)
            {stopwatch_read += stopwatch_val;}
        stopwatch_buffer[stopwatch_read] = '\0';

        sprintf(min,"%c%c", stopwatch_buffer[0],stopwatch_buffer[1]);
        sprintf(sec,"%c%c", stopwatch_buffer[3],stopwatch_buffer[4]);
        sprintf(ms,"%c%c", stopwatch_buffer[6],stopwatch_buffer[7]);


        //Read KEY value, if KEY0 is pressed:
		if ((*(key_buffer)) & 0x01) {

            //When KEY0 is pressed start stopwatch
            key0_press = !key0_press;

		    if(key0_press){ stopwatch_condition = "stop";}
		    else {stopwatch_condition = "run";}

            write (stopwatch_fd, stopwatch_condition, strlen(stopwatch_condition));

        }


        //Read KEY value, if KEY1 is pressed change ms:
        if ((*(key_buffer)) & 0x02) {

            number = strtol(sw_buffer,NULL,16);
            if (number > 99){number = 99;}
            sprintf(ms,"%02ld",number);

        }

        //Read KEY value, if KEY2 is pressed change sec:
        if ((*(key_buffer)) & 0x04) {

            number = strtol(sw_buffer,NULL,16);
            if (number > 59){number = 59;}
            sprintf(sec,"%02ld",number);

        }

        //Read KEY value, if KEY3 is pressed change min:
        if ((*(key_buffer)) & 0x08) {

            number = strtol(sw_buffer,NULL,16);
            if (number > 59){number = 59;}
            sprintf(min,"%02ld",number);

        }

            //Send updated time value to /dev/stopwatch
            sprintf(send_time, "%s:%s:%s",min,sec,ms);
            write (stopwatch_fd, send_time, strlen(send_time));
            write (stopwatch_fd, "disp", 4);

    }




    //Close files
	close (key_fd);
	close (sw_fd);
    close (ledr_fd);
    close (stopwatch_fd);

    return 0;
}
