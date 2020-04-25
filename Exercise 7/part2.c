#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include "address_map_arm.h"

#define accel_BYTES 20 // number of characters to read from /dev/accel


volatile sig_atomic_t stop;
void catchSIGINT(int signum){
    stop = 1;
}


int main(int argc, char *argv[]){

    int accel_FD; // file descriptor
    char buffer[accel_BYTES]; // buffer for data read from /dev/accel

    int accel_read;
    int accel_val = 0;
    int X, Y, Z, R, S;

    signal(SIGINT,catchSIGINT);


    // Open the character device driver
    if ((accel_FD = open("/dev/accel", O_RDONLY)) == -1)
    {
        printf("Error opening /dev/accel: %s\n", strerror(errno));
        return -1;
    }


    while(!stop){


        // Read from /dev/accel
        accel_read = 0;
        while ((accel_val = read (accel_FD, buffer, accel_BYTES)) != 0)
            {accel_read += accel_val; }

        buffer[accel_read] = '\0';
        sscanf(buffer,"%d %d %d %d %d", &R, &X, &Y, &Z, &S);

        //If value updated print new value
        if (R == 1){

            printf("X = %d, Y = %d, Z = %d\n", X*S, Y*S, Z*S);

        }


    }


    close (accel_FD);
    return 0;
}
