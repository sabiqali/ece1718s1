#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>


#define GREEN 32
#define YELLOW 33
#define BLUE 34
#define MAGENTA 35
#define CYAN 36
#define WHITE 37


//Function to swap numbers
void swap (int * num1, int * num2){

    int temp;

    temp = *num1;
    *num1 = *num2;
    *num2 = temp;
}

//Fucntion to draw pixels
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

	printf ("\e[2J"); 					        // clear the screen
  	printf ("\e[?25l");					        // hide the cursor


    draw_line(1,80,1,1, WHITE, '$');            //Draw lines with different colors, orientation and character
    draw_line(1,1,1,24, YELLOW, '%');
    draw_line(1,20,24,1, CYAN, '|');
    draw_line(1,40,24,1, MAGENTA, '/');
    draw_line(1,80,24,1, GREEN, '*');
    draw_line(1,80,24,6, BLUE, '"');
    draw_line(1,80,24,12, WHITE, '*');
    draw_line(1,80,24,24, WHITE, '@');

	c = getchar ();						        // wait for user to press return
	printf ("\e[2J"); 					        // clear the screen
	printf ("\e[%2dm", WHITE);			        // reset foreground color
	printf ("\e[%d;%dH", 1, 1);		            // move cursor to upper left
	printf ("\e[?25h");					        // show the cursor
	fflush (stdout);

}

