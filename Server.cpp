#include "Practical.h"
#include <iostream>
#include <stdio.h>
#include <string.h>    //strlen
#include <cstring>
#include <stdlib.h>    //strlen
#include <cstdlib>
#include <cstdio>
#include <sys/socket.h>
#include <arpa/inet.h> //inet_addr
#include <unistd.h>    //write
#include <iomanip>
#include <fstream>
#include <algorithm>
#include <pthread.h>
#include <list>
#include <dirent.h>
#include <openssl/md5.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

using namespace std;

void *ThreadMain(void *arg); // Main program of a thread
void handle_Prompt(int clntSocket);
void handle_List(int clntSocket);
void handle_Diff(int clntSocket);
void handle_Sync(int clntSocket);
// Structure of arguments to pass to client thread
struct ThreadArgs {
  int clntSock; // Socket descriptor for client
};

int main(int argc, char *argv[]) {

  if (argc != 2) // Test for correct number of 
    DieWithUserMessage("Parameter(s)", "<Server Port/Service>");

  char *servPort = argv[1]; // First arg:  local port
  int servSock = SetupTCPServerSocket(servPort);
  if (servSock < 0)
    DieWithUserMessage("SetupTCPServerSocket() failed", "unable to establish");

  for (;;) { // Run forever
    int clntSock = AcceptTCPConnection(servSock);

    // Create separate memory for client argument
    struct ThreadArgs *threadArgs = (struct ThreadArgs *) malloc(
        sizeof(struct ThreadArgs));
    if (threadArgs == NULL)
      DieWithSystemMessage("malloc() failed");
    threadArgs->clntSock = clntSock;

    // Create client thread
    pthread_t threadID;
    int returnValue = pthread_create(&threadID, NULL, ThreadMain, threadArgs);
    if (returnValue != 0)
      DieWithUserMessage("pthread_create() failed", strerror(returnValue));
    printf("with thread %ld\n", (long int) threadID);
  }
  // NOT REACHED
}

void *ThreadMain(void *threadArgs) {
  //cout << "threadMain" << endl;
  // Guarantees that thread resources are deallocated upon return
  pthread_detach(pthread_self());

  // Extract socket file descriptor from argument
  int clntSock = ((struct ThreadArgs *) threadArgs)->clntSock;
  free(threadArgs); // Deallocate memory for argument

  //HandleTCPClient(clntSock);//replace this with our own methods
  handle_Prompt(clntSock);
  return (NULL);
}

void handle_Prompt(int clntSocket){
  while(1)
    {
    char buffer[2]; // Buffer for prompt

    // Receive prompt (1-4) from client
    ssize_t numBytesRcvd = recv(clntSocket, buffer, 2, 0);
    if (numBytesRcvd < 0)
      DieWithSystemMessage("recv() failed in prompt");

    int prompt = atoi(buffer);
    if(prompt == 1){//for testing
      handle_List(clntSocket);
      }
    else if(prompt == 2)
    {
      cout << "Begin Diff" << endl;
      handle_Diff(clntSocket);
      cout << "Diff Complete" << endl;
    }
    else if(prompt == 3)
    {
      cout << "Begin Sync"<<endl;
      handle_Sync(clntSocket);
      cout << "Sync Complete" << endl;
    }
    else if(prompt == 4)
    {
      cout << "leave" << endl;
      close(clntSocket);
    } 
  }

}

bool checkIfNotLink(char* name)
{
  //cout << "name:" << name <<endl;
  if(strcmp(name,"."))
  {
    return true;
  }
  else if(strcmp(name,".."))
  {
    return true;
  }
  else if(strcmp(name,".DS_STORE"))
  {
    return true;
  }
  else
  {
    return false;
  }
}


//almost exactly the same as list but also send the checks
void handle_Diff(int clntSocket){
 int sock = clntSocket;

  char out[100];
  //char *getcwd(char *buf, size_t size);
  string outString = getcwd(out , 100);
  outString = outString + "/serverSongs";
  cout << outString <<endl;
  DIR *dir;
 //dir = opendir()
  struct dirent *ent;
  //dir = opendir(outString.c_str());
  dir = opendir("./serverSongs");
  
  if(dir != NULL){
    while((ent = readdir(dir)) != NULL){

      //get filename
      char *message = ent->d_name;

      struct stat statbuf;
      int fail = lstat(message, &statbuf);
      /*if(!(S_ISREG(statbuf.st_mode)))
        {
          cout << fail << endl;
          cout << "continuing " << S_ISREG(statbuf.st_mode)<< endl;
          continue;
        }*/
      /*if(checkIfNotLink(message))
      {
        continue;
      }*/
        int len = (unsigned int)strlen(message);
        message[len] = '\0';
        char messageArray[12]; 
        sprintf(messageArray, "%d", len+1);
        char *mg = messageArray;

        char * fileBuffer;
        int fileLength;


        char messagePath [80];
        strcpy(messagePath, "./serverSongs/");
        strcat(messagePath, message);

        //read the file into filebuffer
        ifstream is (messagePath, ifstream::in);
        if (is) {
          // get length of file:
          is.seekg (0, is.end);
          fileLength = is.tellg();
          is.seekg (0, is.beg);

          //this is where the entire contents of the file goes
          fileBuffer = new char[fileLength];

          //cout << "Reading " << fileLength << " characters... ";
          // read data as a block:
          is.read (fileBuffer,fileLength);

          if (is)
            cout << "all characters read successfully." << endl;
          else
            cout << "error: only " << is.gcount() << " could be read from " <<  messagePath<< endl;;
        is.close();

        }
        else
        {
          cerr << "problem with the file " << messagePath << endl;
        }


        char hashBuffer[16];
        MD5((unsigned char *)fileBuffer, strlen(fileBuffer), (unsigned char *)hashBuffer);


        char answerBuf[33];    /* Buff contains formatted answer */
        for (int i = 0; i < 16; i++) 
        {
          sprintf(&answerBuf[i*2],"%02x ", hashBuffer[i]);
        }
        answerBuf[32] = '\0';
        printf("%s\n", answerBuf);

        send(sock, messageArray, 12, 0);
        send(sock , message , strlen(message)+1, 0);

        send(sock, answerBuf, 33, 0);
      
    }
    char messageArray[12] = "done";
    send(sock, messageArray, 12, 0); 
    closedir(dir);
  
  } else {
    cerr << "Could not open directory" << endl;
    char messageArray[12] = "done";
    send(sock, messageArray, 12, 0); 
    closedir(dir);
  }
  
}
  void handle_List(int clntSocket){
  int sock = clntSocket;

  char out[100];
  //char *getcwd(char *buf, size_t size);
  string outString = getcwd(out , 100);
  outString = outString + "/serverSongs";
  //cout << outString <<endl;
  DIR *dir;
 //dir = opendir()
  struct dirent *ent;
  dir = opendir(outString.c_str());
  
  if(dir != NULL){

    while((ent = readdir(dir)) != NULL){

      //get filename
      char *message = ent->d_name;
      //cout << string(message);
      //if()      
      /*struct stat statbuf;
      lstat(message, &statbuf);
      if(!S_ISREG(statbuf.st_mode)){continue;}*/

        int len = (unsigned int)strlen(message);
        message[len+1] = '\0';
        char messageArray[12]; 
        sprintf(messageArray, "%d", len+1);
        char *mg = messageArray;

        send(sock, messageArray, 12, 0);
        send(sock , message , strlen(message)+1, 0);
      
    }
    char messageArray[12] = "done";
    send(sock, messageArray, 12, 0); 
    closedir(dir);
  
  } else {
    cerr << "Could not open directory" << endl;
    char messageArray[12] = "done";
    send(sock, messageArray, 12, 0); 
    closedir(dir);
  }  
}

void handle_Sync(int clntSocket){
  int sock = clntSocket;
  bool isDone = false;
  while(!isDone)
  {
    int bufSize = 100;
        char resp[bufSize];
        ssize_t bytesRcved = recv(sock, resp, bufSize, 0);
        if(bytesRcved < 0){
            printf("recv() of the name failed\n");
            exit(1);
            }
        if (string(resp)=="done")
        {
          isDone = true;
          cout << "done" <<endl;

        }
        else//send the file
        {
          //recieve filename
          char* message = resp;

          char messagePath [80];
          strcpy(messagePath, "./serverSongs/");
          strcat(messagePath, message);

            char * fileBuffer;
            int fileLength;

          ifstream is (messagePath, ifstream::in);
        if (is) {
          // get length of file:
          is.seekg (0, is.end);
          fileLength = is.tellg();
          is.seekg (0, is.beg);

          //this is where the entire contents of the file goes
          fileBuffer = new char[fileLength];

          cout << "Reading " << fileLength << " characters... ";
          // read data as a block:
          is.read (fileBuffer,fileLength);

          if (is)
            cout << "all characters read successfully." << endl;
          else
            cout << "error: only " << is.gcount() << " could be read from " <<  messagePath<< endl;;
          is.close();

          }
        else
          {
            cerr << "problem with the file " << messagePath << endl;
          }

          for(int i = 0; i<(int)(fileLength / 1024); i++)
          {
            char messageChunk[1024];
            for(int j = 0; j < 1024; j++){
              //get next 1024 segment
              messageChunk[j] = fileBuffer[i*1024 + j];
            }
            send(sock, messageChunk, 1024, 0);
          }
          int remainingBytes = fileLength % 1024;
          char messageChunk[remainingBytes];
          if(remainingBytes!=0){
            int j = 0;
            for(int i = fileLength-remainingBytes; i<fileLength; i++)
            {
              messageChunk[j]=fileBuffer[i];
              j++;
            }
            send(sock, messageChunk, remainingBytes, 0);
          }
      }
    }

      while(!isDone){
        char fileByte[30];
        //bytesRcved = recv(sock, fileByte, 30, 0);
        ssize_t bytesRcved = recv(sock, fileByte, 50, 0);
        if(bytesRcved < 0){
          printf("recv() in list failed\n");
          exit(1);}
        if (string(fileByte)=="done")
        {
          isDone = true;
          cout << "done" <<endl;

        }
        else//recieve the file
        {
            //set Name
            char* name = fileByte;
            
            //recieve size
            char fileByte[10];
            bytesRcved = recv(sock, fileByte, 10, 0);
            //ssize_t bytesRcved = recv(sock, list, 50, 0);
            if(bytesRcved < 0){
              printf("recv() in list failed\n");
              exit(1);}

              ofstream myfile;
            myfile.open(name);
        
            int fileSize = atoi(fileByte);
            for(int i = 0; i<(int)(fileSize / 1024); i++)
            {
          
              char fileByte[1024];
              bytesRcved = recv(sock, fileByte, 1024, 0);
              //ssize_t bytesRcved = recv(sock, list, 50, 0);
              if(bytesRcved < 0){
              printf("recv() in list failed\n");
              exit(1);}
              myfile << fileByte; //add to file
          
            }
            //do last part
            int remainingBytes = fileSize % 1024;
            if(remainingBytes!=0){
              char fileByte[remainingBytes];
              bytesRcved = recv(sock, fileByte, remainingBytes, 0);
              //ssize_t bytesRcved = recv(sock, list, 50, 0);
              if(bytesRcved < 0){
                printf("recv() in list failed\n");
                exit(1);}
              myfile << fileByte; //add to file
            }
            myfile.close();
        }
            //reciever
      }
      cout << "I think we are done" << endl;

    }