// Sahil Gandhi and Arpit Jasapara
// CS 118 Winter 2018
// Project 2
// This is the server code for the second project.

// 1) Send file (< 1015 bytes)
// 2) Send file (> 1015 bytes)
// 3) Put things in the windows array
// 4) Introduce timers
// 5) Fin Sequence
// 6) Handle packet drops
// 7) Extra credit??

#include <errno.h>
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <locale>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <strings.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>
#include <cstdlib>
#include "globals.h"
#include "tcp_packet.h"

using namespace std;

int port = 5000;
uint16_t serverSeqNum = 0;
uint16_t clientSeqNum = 0;
TCP_Packet packetWindow[WINDOW / MSS];

/**
 * This method throws the perror and exits the program
 * @param s           A string that is the error message
 **/
void throwError(string s) {
  perror(s.c_str());
  exit(1);
}

/**
 * Sends the SYN + ACK back to the client
 * @param sockfd      The socket for the connection
 * @param their_addr  The sockaddr_in struct
 * @return string     String representing the file name that was parsed
 **/
string initiateConnection(int sockfd, struct sockaddr_in &their_addr) {
  int recvlen;
  uint8_t buf[MSS + 1];
  socklen_t sin_size = sizeof(struct sockaddr_in);
  TCP_Packet p;
  TCP_Packet sendB;
  uint8_t sendBuf[MSS];
  string fileName = "";

  while (1) {
    recvlen = recvfrom(sockfd, buf, MSS, 0, (struct sockaddr *)&their_addr,
                       &sin_size);
    if (recvlen > 0) {
      buf[recvlen] = 0;
      p.convertBufferToPacket(buf);
      if (p.getSyn()) {
        cout << "Receiving packet " << p.getSeqNumber() << endl;
        clientSeqNum = p.getSeqNumber();
        sendB.setFlags(1, 1, 0);
        serverSeqNum = rand() % 10000;
        sendB.setSeqNumber(serverSeqNum);
        sendB.setAckNumber(clientSeqNum);
        sendB.convertPacketToBuffer(sendBuf);
        cout << "Sending packet " << serverSeqNum << " " << WINDOW << " SYN"
             << endl;
        if (sendto(sockfd, &sendBuf, MSS, 0, (struct sockaddr *)&their_addr,
                   sizeof(their_addr)) < 0) {
          throwError("Could not send to the server");
        }
      } else if (p.getAck()) {
        cout << "Receiving packet " << p.getSeqNumber() << endl;
        clientSeqNum = p.getSeqNumber();
        serverSeqNum += 1;
        sendB.setFlags(1, 0, 0);
        sendB.setSeqNumber(serverSeqNum);
        sendB.setAckNumber(clientSeqNum);
        for (int i = 0; i < p.getLen(); i++) {
          fileName += (char)p.data[i];
        }
        sendB.convertPacketToBuffer(sendBuf);
        cout << "Sending packet " << serverSeqNum << " " << WINDOW << " SYN"
             << endl;
        if (sendto(sockfd, &sendBuf, MSS, 0, (struct sockaddr *)&their_addr,
                   sizeof(their_addr)) < 0) {
          throwError("Could not send to the server");
        }
        serverSeqNum += 1;
        return fileName;
      }
      memset((char *)&buf, 0, MSS + 1);
    }
  }
}

/**
 * Sends the FIN to the client
 **/
void closeConnection() {}

/**
 * Sends the ack back to the client when it sends a FIN
 **/
void handleClose() {}

/**
 * Send chunked file to the client
 **/
void sendChunkedFile(int sockfd, struct sockaddr_in &their_addr,
                     long long fileSize, streampos fs, char *fileBuffer) {
  long numPackets = 0;
  uint8_t sendBuf[MSS];
  TCP_Packet p;
  // Split file into packets and send them
  numPackets = fs / PACKET_SIZE + 1;
  for (long i = 0; i < numPackets; i++) {
    p.setFlags(0, 0, 0);
    cout << serverSeqNum;
    p.setSeqNumber(serverSeqNum);
    p.setAckNumber(clientSeqNum);
    cout << "Sending packet " << serverSeqNum << " " << WINDOW << endl;
    if (i == numPackets - 1) {
      p.setData((uint8_t *)(fileBuffer + i * MSS), (int)(fileSize - MSS * i));
      serverSeqNum += (uint16_t)(int)(fileSize - MSS * i);
      p.setFlags(0, 0, 1);
    } else {
      p.setData((uint8_t *)(fileBuffer + i * MSS), MSS);
      serverSeqNum += MSS;
    }
    p.convertPacketToBuffer(sendBuf);
    if (sendto(sockfd, &sendBuf, MSS, 0, (struct sockaddr *)&their_addr,
               sizeof(their_addr)) < 0) {
      throwError("Could not send to the server");
    }
  }
}

/**
 * This method will reap zombie processes (signal handler for it)
 * @param sig   The signal for the signal handler
 **/
void handle_sigchild(int sig) {
  while (waitpid((pid_t)(-1), 0, WNOHANG) > 0)
    ;
  fprintf(stderr,
          "Child exited successfully with code %d. Reaped child process.\n",
          sig);
}

int main(int argc, char *argv[]) {
  // Just takes the port number as the argument.

  // Processing the arguments
  if (argc != 2) {
    throwError(
        "Please input the 1 argument: portnumber. Exiting the program ...");
  }
  port = atoi(argv[1]);
  if (port < 1024) {
    throwError("Could not process int or trying to use privileged port. "
               "Exiting the program ...");
  }

  int sockfd;
  struct sockaddr_in my_addr;
  struct sockaddr_in their_addr;
  socklen_t sin_size;

  if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    throwError("socket");

  memset((char *)&my_addr, 0, sizeof(my_addr));
  my_addr.sin_family = AF_INET;
  my_addr.sin_port = htons(port);
  my_addr.sin_addr.s_addr = htonl(INADDR_ANY);

  if (::bind(sockfd, (struct sockaddr *)&my_addr, sizeof(struct sockaddr)) < 0)
    throwError("bind");
  sin_size = sizeof(struct sockaddr_in);

  // Initiate connection and get the fileName
  string fileName = initiateConnection(sockfd, their_addr);
  cout << fileName << endl;

  char *fileBuffer;
  long long fileSize = 0;
  ifstream inFile;

  // Open and read in file
  inFile.open(fileName.c_str(), ios::in | ios::binary | ios::ate);
  streampos fs;
  if (inFile.is_open()) {
    fs = inFile.tellg();
    fileSize = (long long)(fs);
    fileBuffer = new char[(long long)(fs) + 1];
    inFile.seekg(0, ios::beg);
    inFile.read(fileBuffer, fs);
    inFile.close();
  }

  sendChunkedFile(sockfd, their_addr, fileSize, fs, fileBuffer);
  delete fileBuffer;
  close(sockfd);
}