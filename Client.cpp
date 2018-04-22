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
#include <vector>


void printList(int serverSocket);
void recieveNamesAndChecks(int serverSocket);
void crawlDirectory();
void compareLists();
void printDiffs();
void fileSync(int serverSocket);

using namespace std;

struct fileNameAndChecksum
{
    string name;
    string checksum;
};


static vector<fileNameAndChecksum*> clientNamesAndChecks;
static vector<fileNameAndChecksum*> recievedNamesAndChecks;

static vector<fileNameAndChecksum*> onServerNotClient;
static vector<fileNameAndChecksum*> onClientNotServer;

int main(int argc, char *argv[]) {

  if (argc != 3) // Test for correct number of arguments
    DieWithUserMessage("Parameter(s)",
        "<Server Address/Name> <Server Port>");

  char *server = argv[1];     // First arg: server address/name
  const char *service =  argv[2] ;


  // Create a connected TCP socket
  int sock = SetupTCPClientSocket(server, service);
  if (sock < 0)
    DieWithUserMessage("SetupTCPClientSocket() failed", "unable to connect");

  while(1)
  {
    char query[2];
    cout << endl;
    cout << "Enter one of the following options" << endl;
    cout << "1 - List" << endl;
    cout << "2 - Diff" << endl;
    cout << "3 - Sync" << endl;
    cout << "4 - Leave" << endl;
    cout << endl;

    cin >> query;
    int queryLength = strlen(query);

    // Send the string to the server
    ssize_t numBytes = send(sock, query, queryLength, 0);
    if (numBytes < 0)
      DieWithSystemMessage("send() failed ");
    else if (numBytes != queryLength)
      DieWithUserMessage("send()", "sent unexpected number of bytes");

    int queryInt = atoi(query);

    if(queryInt == 1)
    {
      printList(sock);
    }
    else if (queryInt == 2)
    {
      recieveNamesAndChecks(sock);
      crawlDirectory();
      compareLists();
      printDiffs();
    }
    else if (queryInt == 3)
    {
      cout << "begin sync" << endl;
      fileSync(sock);
      cout << "end sync" << endl;
    }
    else if(queryInt == 4)
    {
      close(sock);
    }

  }
  close(sock);
  exit(0);
  
}

void printList(int serverSocket){

cout << "The files on the server are: " << endl;
int sock = serverSocket;
bool isDone = false;
  while(!isDone){
  //cout << "in the while" << endl; 
        int bufSize = 12;
        char resp[bufSize];
        ssize_t bytesRcved = recv(sock, resp, bufSize, 0);
        if(bytesRcved < 0){
            printf("recv() in list size failed\n");
            exit(1);
            }
        if (string(resp)=="done")
        {
          isDone = true;
          cout << "done" <<endl;

        }
        else
        {
        //if(bytesRcved )
         // cout << "resp: ";
         // cout<< resp <<endl;
          int listSize = atoi(resp);

          char list[listSize];
          bytesRcved = recv(sock, list, listSize, 0);
          //ssize_t bytesRcved = recv(sock, list, 50, 0);
          if(bytesRcved < 0){
            printf("recv() in list failed\n");
            exit(1);}
          if(string(list) == "." || string(list) == ".."){}
          else{
              cout << "list: ";
              cout << list << endl;
              }
            }
          }
}

void printDiffs()
{
  cout << "On the server" << endl;
  for(int i = 0; i < recievedNamesAndChecks.size(); i++)
  {
    cout << recievedNamesAndChecks[i]->name << endl;
  }
  cout << endl;

  cout << "On the client" << endl;
  for(int i = 0; i < clientNamesAndChecks.size(); i++)
  {
    cout << clientNamesAndChecks[i]->name << endl;
  }
  cout << endl;

  cout << "On the server but not on the client" << endl;
  for(int i = 0; i < onServerNotClient.size(); i++)
  {
    cout << onServerNotClient[i]->name << endl;
  }
  cout << endl;

  cout << "On the client but not on the server" << endl;
  for(int i = 0; i < onClientNotServer.size(); i++)
  {
    cout << onClientNotServer[i]->name << endl;
  }
}


void recieveNamesAndChecks(int serverSocket)
{
int sock = serverSocket;
bool isDone = false;
  
  while(!isDone){
  //cout << "in the while" << endl; 
        int bufSize = 12;
        char resp[bufSize];
        ssize_t bytesRcved = recv(sock, resp, bufSize, 0);
        if(bytesRcved < 0){
            printf("recv() in name and check size failed\n");
            exit(1);
            }
        if (string(resp)=="done")
        {
          isDone = true;
        }
        else
        {
         // cout << "resp: ";
        //  cout << resp <<endl;
          int nameSize = atoi(resp);

          char name[nameSize];
          bytesRcved = recv(sock, name, nameSize, 0);
          //ssize_t bytesRcved = recv(sock, list, 50, 0);
          if(bytesRcved < 0){
            printf("recv() in list failed\n");
            exit(1);}
          
          fileNameAndChecksum *myNC = new fileNameAndChecksum;
          myNC->name = name;

          
          char check[33];
          
          bytesRcved = recv(sock, check, 33, 0);
          if(bytesRcved < 0){
            printf("recv() 2 in list failed\n");
            exit(1);}

          myNC->checksum = check;
 
          //cout << "myNC.name =  " << myNC->name <<  " myNC.checksum = "  << myNC->checksum << endl;

          recievedNamesAndChecks.push_back(myNC);
          }



  }

}

void crawlDirectory()
{

  char out[100];
  //char *getcwd(char *buf, size_t size);
  string outString = getcwd(out , 100);
  outString = outString + "/clientSongs";
  DIR *dir;
 //dir = opendir()
  struct dirent *ent;
  dir = opendir(outString.c_str());

  
  if(dir != NULL){  
    while((ent = readdir(dir)) != NULL){
      //get filename

      char *message = ent->d_name;

        int len = (unsigned int)strlen(message);
        message[len+1] = '\0';
        char messageArray[12]; 
        sprintf(messageArray, "%d", len+1);
        char *mg = messageArray;
/*
        if(strcmp(message, "..") || strcmp(message, "."))
        {
          continue;
        }
        */

        char * fileBuffer;
        int fileLength;

        char messagePath [80];
        strcpy(messagePath, "./clientSongs/");
        strcat(messagePath, message);

        //read the file into filebuffer
        ifstream is (messagePath, ifstream::in);
        if (is) {
          // get length of file:
          is.seekg (0, is.end);
          fileLength = is.tellg();
          is.seekg (0, is.beg);

          //this is where the entire contents of the file goes
          //char fileBuffer [fileLength];

                    //this is where the entire contents of the file goes
          fileBuffer = new char[fileLength];

          // read data as a block:
          is.read (fileBuffer,fileLength);
/*
          if (is){}
            //cout << "all characters read successfully." << endl;
          else
            cout << "error: only " << is.gcount() << " could be read from " <<  messagePath<< endl;;
    */
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

        fileNameAndChecksum* myNC = new fileNameAndChecksum;
        myNC->name = message;
        myNC->checksum = answerBuf;


        clientNamesAndChecks.push_back(myNC);
  }
}

}
void compareLists()
{
  //get all the names in recievedNamesAndChecks that are not in clientNamesAndChecks
  //and adds them to onServerNotClient
  bool onTheClient = false;
  for(int i = 0; i < recievedNamesAndChecks.size(); i++)
  {
    onTheClient = false;
      for(int j = 0; j < clientNamesAndChecks.size(); j++)
      {
          if((clientNamesAndChecks[j]->checksum).compare(recievedNamesAndChecks[i]->checksum)==0)
          {
              onTheClient = true;
          }
      }
      if(onTheClient == false)
      {
        onServerNotClient.push_back(recievedNamesAndChecks[i]);
      }
          
      onTheClient = false;
  }

  bool onTheServer = false;
  for(int i = 0; i < clientNamesAndChecks.size(); i++)
  {
    onTheServer = false;
      for(int j = 0; j < recievedNamesAndChecks.size(); j++)
      {
          if((clientNamesAndChecks[i]->checksum).compare(recievedNamesAndChecks[j]->checksum)==0)
          {
              onTheServer = true;
          }
      }
      if(onTheServer == false)
      {
        onClientNotServer.push_back(clientNamesAndChecks[i]);
      }
          
      onTheServer = false;
  }

}

void fileSync(int serverSocket)
{
  int sock = serverSocket;
  vector<fileNameAndChecksum> nameAndCheckVector;
  bool isDone = false;
  cout << "going to do file transfer" << endl;
  
  //i need list of files to sync.
  
  //pull
  for(int i=0; i<onServerNotClient.size();i++)
  {
    cout << "in the first while" << endl; 
      int messageSize = 100;
      //char name[messageSize];
      char *name = new char[100];
      strcpy(name, onServerNotClient[i]->name.c_str());
      //add '\0'
      send(sock, name, 100, 0);//do i need to add null character?

      //recieve message with size of mp3

      int bufSize = 10;
      char resp[bufSize];
      ssize_t bytesRcved = recv(sock, resp, bufSize, 0);
      if(bytesRcved < 0){
          printf("recv() in list size failed\n");
          exit(1);
          }
      /*if (string(resp)=="done")
      {
        isDone = true;
        cout << "done" <<endl;

      }*/
      else
      {
        //write file
        ofstream myfile;
        char* pathAndName = "/serverSongs";
        strcat(pathAndName, name);
        myfile.open(pathAndName);
        
        int fileSize = atoi(resp);
        for(int i = 0; i<(int)(fileSize / 1024); i++)
        {
          
          char fileByte[1024];
          bytesRcved = recv(sock, fileByte, 1024, 0);
          //ssize_t bytesRcved = recv(sock, list, 50, 0);
          if(bytesRcved < 0){
            printf("recv() in list failed\n");
            exit(1);}
          else
          {
            cout << "recieved 1024 bytes of the file" << endl;
          }
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
    }
    
    //send done
    char messageArray[30] = "done";
    send(sock, messageArray, 30, 0); 
    
    //send files to server
    for(int i =0; i<onClientNotServer.size(); i++)
    {
      
      char* message = new char[30];
      strcpy(message, onClientNotServer[i]->name.c_str());
      char messagePath [80];
      strcpy(messagePath, "./clientSongs/");
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

        int messageSize = 30;
        //char name[messageSize];
        //name = onClientNotServer[i]->name + '\0'; 
        //add '\0'
        send(sock, message, 30, 0);//do i need to add null character?
        //filelength to char something
        char messageArray[10]; 
        sprintf(messageArray, "%d", fileLength);
        cout<< messageArray << ": " << fileLength << endl;
        send(sock, messageArray, 10, 0);//send size

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
    char messageArrayy[30] = "done";
    send(sock, messageArrayy, 30, 0);
  }



