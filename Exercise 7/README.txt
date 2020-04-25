To run the module:
1. run the command "make" to create the .ko file
2. execute the command "insmod accel.ko" to insert the driver into the kernel module

To remove the driver:
1. execute the command "rmmond accel.ko" to remove the driver from the kernel module

To compile the user-space program:
1. execute the command "gcc -Wall -o part4 part4.c"
2. this will create the executable which can be run with the command "./part4"
