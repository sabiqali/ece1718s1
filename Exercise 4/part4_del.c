#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <ctype.h>
#include <time.h>
#include "./address_map_arm.h"

#define BYTES 256
#define KEY_BYTES 2
#define SW_BYTES 4
#define stopwatch_BYTES 8


//These will store the values of the time that is being passed around
char min[2], sec[2], ms[2];


volatile sig_atomic_t stop;
void catchSIGINT(int signum){
    stop = 1;
}

//Will get the stopwatch value. used for checking the game time.
void get_time(int *stopwatch_fd, char *stopwatch_buffer){

    int stopwatch_read;
    int stopwatch_val;

    stopwatch_read = 0;

	while ((stopwatch_val = read (*stopwatch_fd, stopwatch_buffer, 8)) !=0)
		    {stopwatch_read += stopwatch_val;}

    stopwatch_buffer[stopwatch_read] = '\0';

    sprintf(min,"%c%c", stopwatch_buffer[0],stopwatch_buffer[1]);
    sprintf(sec,"%c%c", stopwatch_buffer[3],stopwatch_buffer[4]);
    sprintf(ms,"%c%c", stopwatch_buffer[6],stopwatch_buffer[7]);

    return;
}

//function which describes the game. 
//Difficulty updates after 5 correct answers
//Exits when time has elapsed
void game(int *stopwatch_fd, char *stopwatch_buffer, char * init_time){

    //seeding the random number generator 
    srand(time(NULL));

    int num1, num2, correct;
    int duration;
    int time_passed = 0;
    int user_answer;
    char user_input[10];
    int count_correct = 0;
    int time;
    char time_start[10];
    double tot_time;
    double avg_time;

    get_time(stopwatch_fd, stopwatch_buffer);
    time = 10000*atoi(min) + 100*atoi(sec) + atoi(ms);

    sprintf (time_start, "%s:%s:%s", min, sec, ms);

    int difficulty = 10;

    while (time > 0){

        num1 = rand() % difficulty;
        num2 = rand() % difficulty;

        correct = num1 * num2;

        printf("%d * %d = ", num1, num2);


        scanf("%s",user_input);
        user_answer = atoi(user_input);

        //check if answer is not correct and keep counting down if so and keep trying
        while (user_answer != correct){
            get_time(stopwatch_fd
            ,stopwatch_buffer);
	        time_passed = 10000*atoi(min) + 100*atoi(sec) + atoi(ms);
  	        if (time_passed != 0){
	    	    printf("Try again: ");
            	scanf("%s",user_input);
            	user_answer = atoi(user_input);
	        }
            else 
                break;
        }

        //If user answer is correct, if it is then increment the correct timer and reset the time
        if (user_answer == correct){
            get_time(stopwatch_fd, stopwatch_buffer);
            time_passed = 10000*atoi(min) + 100*atoi(sec) + atoi(ms);
            duration += (time - time_passed);
	        if (time_passed != 0){
            	write (*stopwatch_fd, init_time, strlen(init_time));
		        count_correct++;
  	        }
	        else
                break;
            if((count_correct%5)==0){
                difficulty = difficulty * 5;
            }

        }

    //checking the time at the end of the brackets
    get_time(stopwatch_fd, stopwatch_buffer);
    time = 10000*atoi(min) + 100*atoi(sec) + atoi(ms);
    }

    //calculating evaluation results
    tot_time = (((duration %1000000)/10000)* 60) + ((duration %10000)/100) + ((double)(duration%100)/100);
    
    if (count_correct){
        avg_time = tot_time / (double) count_correct;
        printf( "Time expired! You answered %d questions, in an average of %.2f seconds.\n", count_correct, avg_time);
    }
    else{
        printf( "No questions answered correctly!\n");
    }
}

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

    //Buffers for character data
  	char key_buffer[KEY_BYTES];
  	char sw_buffer[SW_BYTES];
  	char stopwatch_buffer[BYTES];
	int key_val, sw_val, key_read, sw_read;
    int stopwatch_val, stopwatch_read;

    //variable to hold SW_buffer value when converted to long from string
    long number = 0;


  	// catch SIGINT from ctrl+c, instead of having it abruptly close this program
  	signal(SIGINT, catchSIGINT);

	// Open the character device drivers for read/write
	if ((key_fd = open("/dev/key", O_RDONLY)) == -1)
	{
		printf("Error opening /dev/KEY: %s\n", strerror(errno));
		return -1;
	}

    if ((sw_fd = open("/dev/sw", O_RDONLY)) == -1)
	{
		printf("Error opening /dev/SW: %s\n", strerror(errno));
		return -1;
	}

	if ((ledr_fd = open("/dev/ledr", O_WRONLY)) == -1)
	{
		printf("Error opening /dev/LEDR: %s\n", strerror(errno));
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
    sprintf(send_time, "00:15:00");
    write (stopwatch_fd, send_time, strlen(send_time));
    //write (LEDR_FD, SW_buffer, 3);
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


        //read_timer(&stopwatch_FD, stopwatch_buffer);

        //Read KEY value, if KEY0 is pressed:
		if ((*(key_buffer)) & 0x01) {

            //When KEY0 is pressed start stopwatch
		    stopwatch_condition = "run";
		    write (stopwatch_fd, stopwatch_condition, strlen(stopwatch_condition));

            //Call game function, initiate game
		    game(&stopwatch_fd, stopwatch_buffer, send_time);


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

