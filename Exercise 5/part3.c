#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <signal.h>

#define CYAN 36
#define WHITE 37
 

volatile sig_atomic_t stop;
void catchSIGINT(int signum){
    stop = 1;
}

//Function to swap numbers
void swap (int * num1, int * num2){

    int temp;

    temp = *num1;
    *num1 = *num2;
    *num2 = temp;
}

//Function to draw pixels
void draw_pixel(int x, int y, char color, char c)
{
  	printf ("\e[%2dm\e[%d;%dH%c", color, y, x, c);
  	fflush (stdout);
}

//Function to draw lines
void draw_line(int x0, int x1, int y0, int y1, char color, char c){

    int deltaX, deltaY, error, x, y, y_step;

    bool is_steep;

    is_steep = (abs(y1 - y0)) > (abs(x1 - x0));

    if (is_steep){

        swap(&x0,&y0);
        swap(&x1,&y1);
    }

    if (x0 > x1){

        swap(&x0,&x1);
        swap(&y0,&y1);

    }

    deltaX = x1 -x0;
    deltaY = abs(y1 - y0);
    error = -(deltaX/2);

    y = y0;
    if (y0 < y1) { y_step = 1;}
    else {y_step = -1;}

    for (x = x0; x < (x1 + 1); x++){

        if (is_steep){ draw_pixel(y,x, color, c);}
        else {draw_pixel(x,y, color, c);}

        error += deltaY;

        if (error >= 0){
            y += y_step;
            error -= deltaX;

        }

    }


}




void main (void){

    char c;

int i = 12;
int step = 1;

signal(SIGINT,catchSIGINT);
    	struct timespec ts;
        	ts.tv_sec = 0;
        	ts.tv_nsec = 100000000L;

	printf ("\e[2J"); 					// clear the screen
  	printf ("\e[?25l");					// hide the cursor
    while(!stop){

    draw_line(20,60,i,i, CYAN, '*');
    nanosleep(&ts , NULL);
    printf ("\e[2J");
    if (i == 1){step = 1;}
    if (i == 24){step = -1;}
    i += step;

    }

	//c = getchar ();						    // wait for user to press return
	printf ("\e[2J"); 					        // clear the screen
	printf ("\e[%2dm\e[%d;%dH", WHITE, 1, 1);	//reset foreground color, move cursor to upper left
	printf ("\e[?25h");					        // show the cursor
	fflush (stdout);

}

