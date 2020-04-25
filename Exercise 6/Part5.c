#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <sys/mman.h>
#include <time.h>
#include "address_map_arm.h"
#define video_BYTES 8 // number of characters to read from /dev/video


int screen_x, screen_y;

void *LW_virtual;					//Lightweight bridge base address
volatile int *KEY_ptr;	            // virtual address for Key buttons
volatile int *SW_ptr;	            // virtual address for SW buttons


volatile sig_atomic_t stop;
void catchSIGINT(int signum){
    stop = 1;
}

//Draw function, writes to back buffer, swaps/syncs it and clears screen
void draw(int x_Xs[], int y_Xs[], int dx_Xs[],int dy_Xs[], int video_fd, int N, struct timespec ts) {
    int i;
    int nextpoint;
    char command[64]; // buffer for commands written to /dev/video
    int color = 0x07E0;


    for (i = 0; i < N; i++) {
        nextpoint = (i+1)%N;

        sprintf (command, "box %d,%d %d,%d %X\n", x_Xs[i], y_Xs[i], x_Xs[nextpoint], y_Xs[nextpoint], color);
        write (video_fd, command, strlen(command));

        if (!(*SW_ptr)){

            sprintf (command, "line %d,%d %d,%d %X\n", x_Xs[i], y_Xs[i], x_Xs[nextpoint], y_Xs[nextpoint], color);
            write (video_fd, command, strlen(command));
        }
    }


    nanosleep(&ts , NULL);
    write (video_fd, "sync", 4);
    write (video_fd, "clear", 5);


    for (i = 0; i < N; i++){
        if (x_Xs[i] == 0 || x_Xs[i] == screen_x - 1){ dx_Xs[i] = dx_Xs[i] * -1;}
        if (y_Xs[i] == 0 || y_Xs[i] == screen_y - 1){ dy_Xs[i] = dy_Xs[i] * -1;}

        x_Xs[i] += dx_Xs[i];
        y_Xs[i] += dy_Xs[i];
    }
}

int main(int argc, char *argv[]){

    int video_fd; // file descriptor
    char buffer[video_BYTES]; // buffer for data read from /dev/video
    char command[64]; // buffer for commands written to /dev/video
    int N = 8 ;
    int i;
    int fd = -1;
    int x_Xs[16],y_Xs[16],dx_Xs[16],dy_Xs[16];
    int color = 0x90FF; //purple
    int sleep_factor = 2;
    int video_read;
    int video_val = 0;
    srand(time(NULL));
    signal(SIGINT,catchSIGINT);


    if ((fd = open( "/dev/mem", (O_RDWR | O_SYNC))) == -1){
        printf ("ERROR: could not open \"/dev/mem\"...\n");
        return (-1);
    }
    LW_virtual = mmap (NULL, LW_BRIDGE_SPAN, (PROT_READ | PROT_WRITE), MAP_SHARED, fd, LW_BRIDGE_BASE);
	if (LW_virtual == MAP_FAILED)
	{
		printf ("ERROR: mmap() failed...\n");
		close (fd);
		return (NULL);
	}

    SW_ptr = (int *)(LW_virtual + SW_BASE);                 //Set virtual address of SW
    KEY_ptr = (int *)(LW_virtual+ KEY_BASE);			    //Set virtual address of Keys
	*(KEY_ptr+3) = 0xF;

    signal(SIGINT,catchSIGINT);

    //Time period of animation, initial condition 0.01sec
    struct timespec ts;
       	ts.tv_sec = 0;
       	ts.tv_nsec = 10000000L;

    // Open the character device driver
    if ((video_fd = open("/dev/video", O_RDWR)) == -1)
    {
        printf("Error opening /dev/video: %s\n", strerror(errno));
        return -1;
    }

    // Set screen_x and screen_y by reading from the driver
    video_read = 0;
	while ((video_val = read (video_fd, buffer, video_BYTES)) != 0)
		{video_read += video_val; }

    buffer[video_read] = '\0';
    sscanf(buffer,"%d %d", &screen_x, &screen_y);

    for(i = 0;i < 16; i++){
        x_Xs[i] = rand()%screen_x;
        y_Xs[i] = rand() %screen_y;
        dx_Xs[i] = (rand()%2) *2 -1;
        dy_Xs[i] = (rand()%2) *2 -1;

    }

    //clear screen at start
    write (video_fd, "clear", 5);


    while(!stop){

        //If KEY0 is pressed the speed is increased
        if (*(KEY_ptr+3) & 0x1){

            ts.tv_nsec /= sleep_factor;
            if (ts.tv_nsec <= 1250000){ ts.tv_nsec = 1250000;}
            *(KEY_ptr + 3) = 0xF;

        }


        //If KEY1 is pressed the speed is decreased
        if (*(KEY_ptr+3) & 0x2){

            ts.tv_nsec *= sleep_factor;
            if (ts.tv_nsec >= 80000000){ ts.tv_nsec = 80000000;}
            *(KEY_ptr + 3) = 0xF;

        }

        //if KEY2 is pressed, the number of points is increased (max 16)
        if (*(KEY_ptr+3) & 0x4){

            N++;
            if (N > 16) { N = 16;}
            *(KEY_ptr + 3) = 0xF;

        }

        //if KEY3 is pressed, the number of points is decreased (min 2)
        if (*(KEY_ptr+3) & 0x8){

            N--;
            if (N < 2) { N = 2;}
            *(KEY_ptr + 3) = 0xF;

        }

        draw(x_Xs,y_Xs,dx_Xs,dy_Xs,video_fd, N, ts);


    }

    if (munmap (LW_virtual, LW_BRIDGE_SPAN) != 0)
	{
		printf ("ERROR: munmap() failed...\n");
		return (-1);
	}
    close (fd);
    close (video_fd);
    return 0;
}
