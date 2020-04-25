#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#include "address_map_arm.h"
#define accel_BYTES 256 // number of characters to read from /dev/accel

#define RED 31
#define YELLOW 33
#define CYAN 36
#define WHITE 37

volatile sig_atomic_t stop;
void catchSIGINT(int signum){
    stop = 1;
}

void plot_pixel(int x, int y, char color, char c)
{
  	printf ("\e[%2dm\e[%d;%dH%c", color, y, x, c);
  	fflush (stdout);
}

int main(int argc, char *argv[]){

    int accel_FD; // file descriptor
    char buffer[accel_BYTES]; // buffer for data read from /dev/accel

    int accel_read;
    int accel_val = 0;
    int X, Y, Z, H, S;
    double alpha_x = 0.5;
    double alpha_y = 0.8;
    int av_x = 0;
    int av_y = 0;
    int mov_x, mov_y;
    char text_[40];
    int color = CYAN;
    char character = 'O';
    int toggle_char = 0;
    int toggle_color = 0;

    signal(SIGINT,catchSIGINT);

    struct timespec ts; //0.1s
       	ts.tv_sec = 0;
       	ts.tv_nsec = 100000000L;

    fflush (stdout);
    // Open the character device driver
    if ((accel_FD = open("/dev/accel", O_RDWR)) == -1)
    {
        printf("Error opening /dev/accel: %s\n", strerror(errno));
        return -1;
    }

    printf ("\e[2J"); 					// clear the screen
  	printf ("\e[?25l");					// hide the cursor
    plot_pixel (40, 12, CYAN, 'O');
    write (accel_FD, "rate 100", 8);
    write (accel_FD, "format 0 16", 11);

    while(!stop){


        // Read from /dev/accel
        accel_read = 0;
        while ((accel_val = read (accel_FD, buffer, accel_BYTES)) != 0)
            {accel_read += accel_val; }

        nanosleep(&ts , NULL);

        buffer[accel_read] = '\0';
        sscanf(buffer,"%X %d %d %d %d", &H, &X, &Y, &Z, &S);

        //If single tap change color
        if (H == 0xC2){
            //plot_pixel (mov_x, mov_y, color, 'X');
            printf ("\e[2J");
            toggle_color = !toggle_color;
            if (toggle_color){
                color = RED;
            }
            else{
                color = CYAN;
            }

            sprintf(text_, "X = %d, Y = %d, Z = %d", X*S, Y*S, Z*S);

            printf ("\e[%2dm\e[%d;%dH", YELLOW, 1, 1);
            printf("%s", text_);
            plot_pixel (mov_x, mov_y, color, character);
        }

         //If double tap change character
        else if (H == 0xE2){

            printf ("\e[2J");
            toggle_char = !toggle_char;
            if (toggle_char){
                character = 'X';
            }
            else{
                character = 'O';
            }

            sprintf(text_, "X = %d, Y = %d, Z = %d", X*S, Y*S, Z*S);

            printf ("\e[%2dm\e[%d;%dH", YELLOW, 1, 1);
            printf("%s", text_);
            plot_pixel (mov_x, mov_y, color, character);
        }

        //if activity move character
        else if (H == 0x82){

            printf ("\e[2J");
            av_x = (av_x * alpha_x) + (X * (1 - alpha_x));
            av_y = (av_y * alpha_y) + (Y * (1 - alpha_y));

            mov_x = 40 + av_x;
            if (mov_x < 1){ mov_x = 1;}
            if (mov_x > 80){ mov_x = 80;}

            mov_y = 12 + av_y;
            if (mov_y < 1){ mov_y = 1;}
            if (mov_y > 24){ mov_y = 24;}

            sprintf(text_, "X = %d, Y = %d, Z = %d", X*S, Y*S, Z*S);

            printf ("\e[%2dm\e[%d;%dH", YELLOW, 1, 1);
            printf("%s", text_);
            plot_pixel (mov_x, mov_y, color, character);

        }


    }

    printf ("\e[2J"); 					        // clear the screen
	printf ("\e[%2dm\e[%d;%dH", WHITE, 1, 1);	//reset foreground color, move cursor to upper left
	printf ("\e[?25h");					        // show the cursor

	fflush (stdout);
    close (accel_FD);
    return 0;
}
