# project4
GetMyMusic!
Goal
 build GetMyMusic!, a file/music synchronization application. 

Like so many other people, Prof S is frustrated with the fact that every machine he owns has a different collection of songs 
in his iTunes library. Moreover, Prof S does not want to use iCloud to solve this because he wants his music stored on his own devices, 
not on Apple's servers. Students will solve this problem and develop GetMyMusic!, a networked application that enables multiple machines
to ensure that they have the same files in their music directory.

Clients can send one of 4 messages to the server: 
1) List Files (ask the server to return a list of files it currently has); 


2) Diff (based on 1)), the client should show a "diff" of the files it has in comparison to the server - both files the server has that the client does not have, and files the client has and the server does not have); 


3) Sync (an operation that (using the information learned in 2)) pulls to the local device all files from the server that
the local device does not have and vice versa); 

4) Leave (the client should end its session with the server and take care of any open connections). 
The server must be multithreaded to handle multiple concurrent client requests (reads and writes) and store/retrieve historical information
about each client in a file.

