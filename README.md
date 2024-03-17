Instructions for Running the Program:-

1. **Compiling the Program**

First, you need to compile the program using the provided Makefile. In your terminal, navigate to the directory containing the Makefile and source files, then run the following command:

      -> make

This command will compile the program according to the rules specified in the Makefile and create an executable named prog.


2. **Running the program**

   a) *Running the server*:- In order to run the server you have to write 
   1. ./rfserver

   If you want to run the command directly by providing % rfs WRITE local-file-path remote-file-path, you can set the path
   by following the below step:-

   Temporarily (current session only): In the terminal, run:

   bash
   Copy code
   export PATH=$PATH:/path/to/directory
   Replace /path/to/directory with the actual path to the directory containing rfs.



   b) *Running the client*:- For running the client you can either run it by following any of the below instrcutions.
   1. ./rfs

  4. **Running the test scripts**
     There are two test scripts that you need to run. If they are not in executable format, you can run the below command.
     1) test_multithreading.sh :- tests the multithreading strategy implemented in our code
     2) test_rfs.sh :- test cases for all the other functionalities
        
     chmod +x rfs test_multithreading.sh
     chmod +x rfs test_rfs.sh

     After they are executables, you can run the below commands to execute them:-
     ./test_multithreading.sh
     ./test_rfs.sh

5. **Running the commands**:-
   WRITE command:- ./rfs WRITE Client/Client.txt Server/server.txt
   STOP command:- ./rfs STOP
   GET command:- ./rfs GET Server/file.txt Client/retrieved_file.txt
   GET with versioning :- ./rfs GET Server/file.txt Client/Version.txt 20231207135749
   WRITE with versioning:- ./rfs WRITE Client/Client.txt Server/server.txt
   LS command:- ./rfs LS  Server/filebackup20231207135749.txt > ls_output.txt
   RM command:- Create 3 files with versioning first.
   ./rfs WRITE Client/Client.txt Server/RM.txt
   ./rfs WRITE Client/Client.txt Server/RMbackup213456.txt
   ./rfs WRITE Client/Client.txt Server/RMbackup201345.txt
   Run the below command to delete all the versions.
   ./rfs RM Server/RM.txt