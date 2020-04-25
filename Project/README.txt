PLEASE PERFORM THE FOLLOWING STEPS TO EXECUTE THE PROGRAM:


Inserting Dependencies:
1. Please go to Linux_Libraries/Drivers from your root folder.
2. Insert the following mods: KEY.ko,SW.ko,video.ko,HEX.ko,SW.ko.
3. Go to the project folder/Design_Files.
4. Insert the mod stopwatch.ko.
5. If the mod doesn't insert or malfunctions, we have provided a folder in Design_Files called Stopwatch. Go inside it and run "make". It will create a new mod.


Executing the Program:
1. Find out the name of your keyboard by going to "/dev/input/by-id/"
2. Note down the entire path of your keyboard, i.e. "/dev/input/by-id/<your_keyboard_name>"
3. Go to the Project_Folder/Design_Files.
4. Run "make"
5. The executable will be created "drum_machine_v2"
6. run the executable using the command "./drum_machine_v2 <your full keybaord path that was obtained in step 2>"
7. Go through the execution as described in video demo. It explains the detailed steps required to use the drum machine and the steps to follow for all frequencies. 
   If any errors or challenges are encountered, please refer to the challenges/drawbacks section of the report for their solution.
