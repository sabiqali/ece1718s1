Instructions to add kernels and to compile codes:

1. From the Linux Library drivers, insmod the following drivers:
   - KEY.ko
   - LEDR.ko
   - HEX.ko
   - audio.ko

2. In the directory Lab_8, go to the directory "stopwatch" and "make" the stopwatch kernel file. Next insmod stopwatch.ko.

2. In the directory Lab_8, go to the directory "video" and "make" the stopwatch kernel file. Next insmod video.ko.

3. Go back to the directory deliverables and "make part5" to make the executable for part5 and similarly for the other parts.


Part 1 :
./part1

Part 2 :
./part2 1001000100000 
(Basically a 13 bit value for the notes)

Part 3, 4, 5 and 6 :

./part3 
(or part4/5/6)

Then, when asked for the keyboard path, paste the entire path to your input keyboard. 
(For example: /dev/input/by-id/usb-DELL_Dell_USB_Entry_Keyboard-event-kbd)

