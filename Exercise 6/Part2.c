#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#define video_BYTES 8 // number of characters to read from /dev/video

int screen_x, screen_y;

int main(int argc, char *argv[]){

    int video_fd; // file descriptor
    char buffer[video_BYTES]; // buffer for data read from /dev/video
    char command[64]; // buffer for commands written to /dev/video

    int video_read;
    int video_val = 0;

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

    write (video_FD, "clear", 5);

    /* Draw a few lines */
    sprintf (command, "line %d,%d %d,%d %X\n", 0, screen_y - 1, screen_x - 1, 0, 0xFFE0); // yellow
    write (video_fd, command, strlen(command));
    sprintf (command, "line %d,%d %d,%d %X\n", 0, screen_y - 1, (screen_x >> 1) - 1, 0, 0x07FF); // cyan
    write (video_fd, command, strlen(command));
    sprintf (command, "line %d,%d %d,%d %X\n", 0, screen_y - 1, (screen_x >> 2) - 1, 0, 0x07E0); // green
    write (video_fd, command, strlen(command));
    sprintf (command, "line %d,%d %d,%d %X\n", 0, screen_y - 1, 0, 0, 0xF800); // red
    write (video_fd, command, strlen(command));
    sprintf (command, "line %d,%d %d,%d %X\n", 0, screen_y - 1, screen_x - 1, screen_y - 1, 0xF81F); //pink
    write (video_fd, command, strlen(command));


    close (video_fd);
    return 0;
}
