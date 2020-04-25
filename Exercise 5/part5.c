#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <signal.h>
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include "address_map_arm.h"
#include "interrupt_ID.h"

#define WHITE 37



void *LW_virtual;					//Lightweight bridge base address
volatile int *KEY_ptr;	            // virtual address for Key buttons
volatile int *SW_ptr;	            // virtual address for SW buttons


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

//Function to draw pixel
void draw_pixel(int x, int y, char color, char c)
{
  	printf ("\e[%2dm\e[%d;%dH%c", color, y, x, c);
  	fflush (stdout);
}

//Function to draw line
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

//Function that draws the lines moving in random direction
void draw(int x_Xs[], int y_Xs[], int dx_Xs[], int dy_Xs[], int color[], int N){
    int i;
    int nextpoint;
    printf("%d\n",N);
    printf ("\e[2J");
    for (i = 0; i < N; i++){
        nextpoint = (i+1)%N;

    if (!(*SW_ptr)){
        draw_line(x_Xs[i],x_Xs[nextpoint], y_Xs[i], y_Xs[nextpoint], color[i], '-');}

        draw_pixel(x_Xs[i], y_Xs[i], color[i], 'X');
        draw_pixel(x_Xs[nextpoint], y_Xs[nextpoint], color[i], 'X');
    }
    for (i = 0; i < N; i++){
        if (x_Xs[i] == 1 || x_Xs[i] == 80){ dx_Xs[i] = dx_Xs[i] * -1;}
        if (y_Xs[i] == 1 || y_Xs[i] == 24){ dy_Xs[i] = dy_Xs[i] * -1;}

        x_Xs[i] += dx_Xs[i];
        y_Xs[i] += dy_Xs[i];
    }
}


int main (void){


    int N = 5;                                              //Initial number of lines
    int x_Xs[10],y_Xs[10],dx_Xs[10],dy_Xs[10],color[10];    //Arays to store coordinates, deltas and colors
    //char c;                                                 //What character to print
    int fd = -1;
    srand(time(NULL));
    int i;
    //int step = 1;
    int sleep_factor = 2;

	if ((fd = open( "/dev/mem", (O_RDWR | O_SYNC))) == -1)
	{
		printf ("ERROR: could not open \"/dev/mem\"...\n");
		return (-1);
	}

    LW_virtual = mmap (NULL, LW_BRIDGE_SPAN, (PROT_READ | PROT_WRITE), MAP_SHARED, fd, LW_BRIDGE_BASE);
	if (LW_virtual == MAP_FAILED)
	{
		printf ("ERROR: mmap() failed...\n");
		close (fd);
		return (-1);
	}

    SW_ptr = (int *)(LW_virtual + SW_BASE);                 //Set virtual address of SW
    KEY_ptr = (int *)(LW_virtual+ KEY_BASE);			    //Set virtual address of Keys
	*(KEY_ptr+3) = 0xF;

    signal(SIGINT,catchSIGINT);

    //Time period of animation, initial condition 0,1sec
    struct timespec ts;
       	ts.tv_sec = 0;
       	ts.tv_nsec = 100000000L;


    //Initialize arrays
    for(i = 0;i < 10; i++){
        x_Xs[i] = rand()%80 +1;
        y_Xs[i] = rand() %24 +1;
        dx_Xs[i] = (rand()%2) *2 -1;
        dy_Xs[i] = (rand()%2) *2 -1;
        color[i] = rand()%7 +31;
    }

	printf ("\e[2J"); 					// clear the screen
  	printf ("\e[?25l");					// hide the cursor
    fflush (stdout);

    while(!stop){


    //If KEY0 is pressed the speed is increased
    if (*(KEY_ptr+3) & 0x1){

        ts.tv_nsec /= sleep_factor;
        if (ts.tv_nsec <= 12500000){ ts.tv_nsec = 12500000;}
        *(KEY_ptr + 3) = 0xF;

    }


    //If KEY1 is pressed the speed is decreased
    if (*(KEY_ptr+3) & 0x2){

        ts.tv_nsec *= sleep_factor;
        if (ts.tv_nsec >= 800000000){ ts.tv_nsec = 800000000;}
        *(KEY_ptr + 3) = 0xF;

    }

    //if KEY2 is pressed, the number of points is increased (max 10)
    if (*(KEY_ptr+3) & 0x4){

        N++;
        if (N > 10) { N = 10;}
        *(KEY_ptr + 3) = 0xF;

    }

    //if KEY3 is pressed, the number of points is decreased (min 20)
    if (*(KEY_ptr+3) & 0x8){

        N--;
        if (N < 2) { N = 2;}
        *(KEY_ptr + 3) = 0xF;

    }


    //draw points
    draw(x_Xs,y_Xs,dx_Xs,dy_Xs,color, N);
    nanosleep(&ts , NULL);
    printf ("\e[2J");

    }

    //unmap_physical (LW_virtual, LW_BRIDGE_SPAN);
	//close_physical(fd);
    if (munmap (LW_virtual, LW_BRIDGE_SPAN) != 0)
	{
		printf ("ERROR: munmap() failed...\n");
		return (-1);
	}
    close (fd);
	//c = getchar ();						                // wait for user to press return
	printf ("\e[2J"); 					                    // clear the screen,
	printf ("\e[%2dm\e[%d;%dH", WHITE, 1, 1);		        // reset foreground color, move cursor to upper left
	printf ("\e[?25h");					                    // show the cursor
	fflush (stdout);

    return 0;
}

