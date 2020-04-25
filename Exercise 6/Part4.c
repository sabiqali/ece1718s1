#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <time.h>
#define video_BYTES 8 // number of characters to read from /dev/video
#define N 8

int screen_x, screen_y;

volatile sig_atomic_t stop;
void catchSIGINT(int signum){
    stop = 1;
}


//Function to draw boxes and lines
void draw(int x_Xs[], int y_Xs[], int dx_Xs[],int dy_Xs[], int video_fd){
    int i;
    int nextpoint;
    char command[64]; // buffer for commands written to /dev/video
    int color = 0x07E0;

    for (i = 0; i < N; i++) {
        nextpoint = (i+1)%N;

        sprintf (command, "box %d,%d %d,%d %X\n", x_Xs[i], y_Xs[i], x_Xs[nextpoint], y_Xs[nextpoint], color);
        write (video_fd, command, strlen(command));

        sprintf (command, "line %d,%d %d,%d %X\n", x_Xs[i], y_Xs[i], x_Xs[nextpoint], y_Xs[nextpoint], color);
        write (video_fd, command, strlen(command));
    }

    write (video_fd, "sync", 4);

    write (video_fd, "clear", 5);


    for (i = 0; i < N; i++) {
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
    int y;
    int i;
    int x_Xs[N],y_Xs[N],dx_Xs[N],dy_Xs[N];
    int color = 0xF00F; //Pink

    int video_read;
    int video_val = 0;
    srand(time(NULL));
    signal(SIGINT,catchSIGINT);


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

    for(i = 0;i < N; i++) {
        x_Xs[i] = rand()%screen_x;
        y_Xs[i] = rand() %screen_y;
        dx_Xs[i] = (rand()%2) *2 -1;
        dy_Xs[i] = (rand()%2) *2 -1;
    }

    write (video_fd, "clear", 5);

    while(!stop){
        draw(x_Xs,y_Xs,dx_Xs,dy_Xs,video_fd);
    }

    close (video_fd);
    return 0;
}
