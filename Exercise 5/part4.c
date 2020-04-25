#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <signal.h>
#include <time.h>

#define BLUE 34
#define WHITE 37
#define N 5

int x_Xs[N],y_Xs[N],dx_Xs[N],dy_Xs[N],color[N];
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


//Fucntion to draw lines
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

//Fucntion that draws lines moving in random direction
void draw(){
    int i;
    int nextpoint;
    printf ("\e[2J");
    for (i = 0; i < N; i++){
        nextpoint = (i+1)%N;

        draw_line(x_Xs[i],x_Xs[nextpoint], y_Xs[i], y_Xs[nextpoint], color[i], '-');
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


void main (void){

    char c;
    srand(time(NULL));
    int i;
    int step = 1;


    signal(SIGINT,catchSIGINT);

    struct timespec ts;
       	ts.tv_sec = 0;
       	ts.tv_nsec = 100000000L;

    for(i = 0;i < N; i++){
        x_Xs[i] = rand()%80 +1;
        y_Xs[i] = rand() %24 +1;
        dx_Xs[i] = (rand()%2) *2 -1;
        dy_Xs[i] = (rand()%2) *2 -1;
        color[i] = rand()%7 +31;
    }

	printf ("\e[2J"); 					// clear the screen
  	printf ("\e[?25l");					// hide the cursor
    //fflush (stdout);

    //While not stop draw the lines and move them around
    while(!stop){


    draw();
    nanosleep(&ts , NULL);
    printf ("\e[2J");


    }

	//c = getchar ();						        // wait for user to press return
	printf ("\e[2J"); 					            // clear the screen
	printf ("\e[%2dm\e[%d;%dH", WHITE, 1, 1);		// reset foreground color, move cursor to upper left
	printf ("\e[?25h");					            // show the cursor
	fflush (stdout);

}


