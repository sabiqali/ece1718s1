#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#define video_BYTES 8 // number of characters to read from /dev/video

int screen_x, screen_y;

volatile sig_atomic_t stop;
void catchSIGINT(int signum){
    stop = 1;
}


int main(int argc, char *argv[]){

    int video_fd; // file descriptor
    char buffer[video_BYTES]; // buffer for data read from /dev/video
    char command[64]; // buffer for commands written to /dev/video
    int y;
    int step = 1;

    int video_read;
    int video_val = 0;

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

    write (video_fd, "clear", 5);
    y = (screen_y/2);


    //Draw moving line
    while(!stop){

        sprintf (command, "line %d,%d %d,%d %X\n", (screen_x/5), y, (screen_x*4/5), y, 0xF000); // yellow
        write (video_fd, command, strlen(command));
        write (video_fd, "sync", 4);
        sprintf (command, "line %d,%d %d,%d %X\n", (screen_x/5), y, (screen_x*4/5), y, 0); // black
        write (video_fd, command, strlen(command));
        if (y == 0){step = 1;}
        if (y == (screen_y - 1)){step = -1;}
        y += step;

    }


    close (video_fd);
    return 0;
}
