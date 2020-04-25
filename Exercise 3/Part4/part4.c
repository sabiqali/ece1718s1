#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>

#define BYTES 256				// max number of characters to read from /dev/chardev

volatile sig_atomic_t stop;
void catchSIGINT(int signum){
    stop = 1;
}

/* This code uses the character device driver /dev/chardev. The code reads the default
 * message from the driver and then prints it. After this the code changes the message in
 * a loop by writing to the driver, and prints each new message. The program exits if it
 * receives a kill signal (for example, ^C typed on stdin). */
int main(int argc, char *argv[]){

	int ledr_FD,key_FD,hex_FD,sw_FD;							// file descriptor
  	char ledr_buffer[BYTES],sw_buffer[BYTES],hex_buffer[BYTES],key_buffer[BYTES];			// buffer for chardev character data
	int key_val, key_read,sw_val,sw_read;				// number of characters read from chardev
	char new_msg[128];						// space for the new message that we generate
	int i_msg;
    
  	// catch SIGINT from ctrl+c, instead of having it abruptly close this program
  	signal(SIGINT, catchSIGINT);
    
	// Open the character device driver for read/write
	if ((key_FD = open("/dev/key", O_RDONLY)) == -1)
	{
		printf("Error opening /dev/chardev: %s\n", strerror(errno));
		return -1;
	}
    if ((sw_FD = open("/dev/sw", O_RDONLY)) == -1)
	{
		printf("Error opening /dev/chardev: %s\n", strerror(errno));
		return -1;
	}
    if ((ledr_FD = open("/dev/ledr", O_WRONLY)) == -1)
	{
		printf("Error opening /dev/chardev: %s\n", strerror(errno));
		return -1;
	}
    if ((hex_FD = open("/dev/hex", O_WRONLY)) == -1)
	{
		printf("Error opening /dev/chardev: %s\n", strerror(errno));
		return -1;
	}

    write (hex_FD, "0", strlen("0"));
    write (ledr_FD, "0", strlen("0"));

	i_msg = 0;
  	while (!stop)
	{
		sw_read = 0;
		while ((sw_val = read (sw_FD, sw_buffer, BYTES)) != 0) {
            sw_read += sw_val;
        }
		sw_buffer[sw_read] = '\0';	// NULL terminate
		printf ("%s", sw_buffer);

        key_read = 0;
        key_buffer[0] = '\0';
		while ((key_val = read (key_FD, key_buffer, BYTES)) != 0) {
            key_read += key_val;
        }
		key_buffer[key_read] = '\0';	// NULL terminate
		printf ("%s", key_buffer);
        
        //printf("%d", strtoi(sw_buffer));
        //long number = strtol(sw_buffer,NULL,16);
        long key_number = strtol(key_buffer,NULL,16);
        //i_msg += number;
        //number = 0;
        //sprintf(key_buffer, "%d\0", i_msg);
        //write (ledr_FD, sw_buffer, strlen(sw_buffer));
        //write (hex_FD, key_buffer, strlen(key_buffer)); 
        if(key_number != 0) {
            long number = strtol(sw_buffer,NULL,16);
            i_msg += number;
            sprintf(key_buffer, "%d\0", i_msg);
            write (ledr_FD, sw_buffer, strlen(sw_buffer));
            write (hex_FD, key_buffer, strlen(key_buffer));   
        }

		//sprintf (new_msg, "New message %d\n", i_msg);
		//i_msg++;
		//write (chardev_FD, new_msg, strlen(new_msg));

		sleep (1);
	}
	close (key_FD);
    close (sw_FD);
    close (ledr_FD);
    close (hex_FD);
   return 0;
}
