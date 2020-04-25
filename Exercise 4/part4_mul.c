#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <ctype.h>
#include "./address_map_arm.h"
#include <time.h>


#define BYTES 256
#define KEY_BYTES 2
#define SW_BYTES 4
#define stopwatch_BYTES 8


//Used to store stopwatch data when character device driver is read
char min[2], sec[2], ms[2];


volatile sig_atomic_t stop;
void catchSIGINT(int signum){
    stop = 1;
}

//Function to read timer, called whenever we want to read current time value from /dev/stopwatch
void read_timer(int *stopwatch_FD, char *stopwatch_buffer){

    int stopwatch_read;
    int stopwatch_val;

    stopwatch_read = 0;
	while ((stopwatch_val = read (*stopwatch_FD, stopwatch_buffer, 8)) !=0)
		    {stopwatch_read += stopwatch_val;}
    stopwatch_buffer[stopwatch_read] = '\0';
    sprintf(min,"%c%c", stopwatch_buffer[0],stopwatch_buffer[1]);
    sprintf(sec,"%c%c", stopwatch_buffer[3],stopwatch_buffer[4]);
    sprintf(ms,"%c%c", stopwatch_buffer[6],stopwatch_buffer[7]);

}

/*The function for the math quiz, difficulty is increased by increasing the range of random numbers
 after every 5 correct answers*/
void game(int *stopwatch_FD, char *stopwatch_buffer, char * init_time){


    srand(time(NULL));

    //The two numbers for addition and correct answer of addition
    int num1, num2, correct;

    int duration;
    int time_passed = 0;
    //Answer from user
    int user_answer;
    char user_input[10];

    //Count of correct answers
    int count_correct = 0;

    //variables to keep track of time
    int time;
    char time_start[10];
    double tot_time;
    double avg_time;

    read_timer(stopwatch_FD, stopwatch_buffer);
    time = 10000*atoi(min) + 100*atoi(sec) + atoi(ms);

    sprintf (time_start, "%s:%s:%s", min, sec, ms);

    //To increase difficulty of math questions
    int difficulty = 10;

    while (time > 0){

        num1 = rand() % difficulty;
        num2 = rand() % difficulty;

        correct = num1 * num2;

        printf("%d * %d = ", num1, num2);


        scanf("%s",user_input);
        user_answer = atoi(user_input);

        //If user answer is not correct
        while (user_answer!=correct){
            read_timer(stopwatch_FD,stopwatch_buffer);
	    time_passed = 10000*atoi(min) + 100*atoi(sec) + atoi(ms);
  	    if (time_passed != 0){
	    	printf("Try again: ");
            	scanf("%s",user_input);
            	user_answer = atoi(user_input);
	     }
	     else{break;}
       }

        //If user answer is correct, increment counter, read time value to get duration and reset timer
        if (user_answer == correct){
           // count_correct++;
            read_timer(stopwatch_FD, stopwatch_buffer);
            time_passed = 10000*atoi(min) + 100*atoi(sec) + atoi(ms);
            duration += (time - time_passed);
	    if (time_passed != 0){
            	write (*stopwatch_FD, init_time, strlen(init_time));
		count_correct++;
  	    }
	    else{break;}
            if((count_correct%5)==0){
                difficulty = difficulty * 5;
            }

        }

    //Check time at end of each question answered
    read_timer(stopwatch_FD, stopwatch_buffer);
    time = 10000*atoi(min) + 100*atoi(sec) + atoi(ms);
    }

    //Calculate duration of questions (convert minutes into seconds, we disregarded milliseconds)
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
	int KEY_FD;
	int SW_FD;
	int LEDR_FD;
	int stopwatch_FD;

    //variable to hold "stop" or "run" condition
    char * stopwatch_condition;
    //variable to hold mins, secs or ms
    char send_time[BYTES];

    //Buffers for character data
  	char KEY_buffer[KEY_BYTES];
  	char SW_buffer[SW_BYTES];
  	char stopwatch_buffer[BYTES];
	int KEY_val, SW_val, KEY_read, SW_read;
    int stopwatch_val, stopwatch_read;

    //variable to hold SW_buffer value when converted to long from string
    long number = 0;


  	// catch SIGINT from ctrl+c, instead of having it abruptly close this program
  	signal(SIGINT, catchSIGINT);

	// Open the character device drivers for read/write
	if ((KEY_FD = open("/dev/key", O_RDONLY)) == -1)
	{
		printf("Error opening /dev/KEY: %s\n", strerror(errno));
		return -1;
	}

    if ((SW_FD = open("/dev/sw", O_RDONLY)) == -1)
	{
		printf("Error opening /dev/SW: %s\n", strerror(errno));
		return -1;
	}

	if ((LEDR_FD = open("/dev/ledr", O_WRONLY)) == -1)
	{
		printf("Error opening /dev/LEDR: %s\n", strerror(errno));
		return -1;
	}

	if ((stopwatch_FD = open("/dev/stopwatch", O_RDWR)) == -1)
	{
		printf("Error opening /dev/stopwatch: %s\n", strerror(errno));
		return -1;
	}


    //Initial conditions LEDs are off
    write (LEDR_FD, SW_buffer, BYTES);


    //Initial condition, the stopwatch is stopped, initial time is 59:59:99 and disp is on
    sprintf(send_time, "00:15:00");
    write (stopwatch_FD, send_time, strlen(send_time));
    //write (LEDR_FD, SW_buffer, 3);
    write (stopwatch_FD, "disp", 4);
    write (stopwatch_FD, "stop", 4);


   //Run continuously until stop command
  	while (!stop)
	{

        write(LEDR_FD,SW_buffer,BYTES);
        //Read the driver until EOF
		KEY_read = 0;
		while ((KEY_val = read (KEY_FD, KEY_buffer, KEY_BYTES)) != 0)
			{KEY_read += KEY_val; }

        //Read the driver until EOF
        SW_read = 0;
		while ((SW_val = read (SW_FD, SW_buffer, SW_BYTES)) != 0)
			{SW_read += SW_val; }

        //Read the driver until EOF
        stopwatch_read = 0;
		while ((stopwatch_val = read (stopwatch_FD, stopwatch_buffer, 8))  != 0)
            {stopwatch_read += stopwatch_val;}
        stopwatch_buffer[stopwatch_read] = '\0';

        sprintf(min,"%c%c", stopwatch_buffer[0],stopwatch_buffer[1]);
        sprintf(sec,"%c%c", stopwatch_buffer[3],stopwatch_buffer[4]);
        sprintf(ms,"%c%c", stopwatch_buffer[6],stopwatch_buffer[7]);


        //read_timer(&stopwatch_FD, stopwatch_buffer);

        //Read KEY value, if KEY0 is pressed:
		if ((*(KEY_buffer)) & 0x01) {

            //When KEY0 is pressed start stopwatch
		    stopwatch_condition = "run";
		    write (stopwatch_FD, stopwatch_condition, strlen(stopwatch_condition));

            //Call game function, initiate game
		    game(&stopwatch_FD, stopwatch_buffer, send_time);


        }


        //Read KEY value, if KEY1 is pressed change ms:
        if ((*(KEY_buffer)) & 0x02) {

            number = strtol(SW_buffer,NULL,16);
            if (number > 99){number = 99;}
            sprintf(ms,"%02ld",number);

        }

        //Read KEY value, if KEY2 is pressed change sec:
        if ((*(KEY_buffer)) & 0x04) {

            number = strtol(SW_buffer,NULL,16);
            if (number > 59){number = 59;}
            sprintf(sec,"%02ld",number);

        }

        //Read KEY value, if KEY3 is pressed change min:
        if ((*(KEY_buffer)) & 0x08) {

            number = strtol(SW_buffer,NULL,16);
            if (number > 59){number = 59;}
            sprintf(min,"%02ld",number);

        }

            //Send updated time value to /dev/stopwatch
            sprintf(send_time, "%s:%s:%s",min,sec,ms);
            write (stopwatch_FD, send_time, strlen(send_time));
            write (stopwatch_FD, "disp", 4);

    }




    //Close files
	close (KEY_FD);
	close (SW_FD);
    close (LEDR_FD);
    close (stopwatch_FD);

    return 0;
}

