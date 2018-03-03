// Sahil Gandhi and Arpit Jasapara
// CS 118 Winter 2018
// Project 2
// This is the client code for the second project.

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
#include <vector>
#include "globals.h"
#include "tcp_packet.h"

using namespace std;

int port = 5000;
uint16_t serverSeqNum = 0;
uint16_t clientSeqNum = 0;
TCP_Packet packetWindow[WINDOW / MSS];

/**
 * This method throws the perror and exits the program
 * @param s         A string that is the error message
 **/
void throwError(string s) {
  perror(s.c_str());
  exit(1);
}

/**
 * This method will reap zombie processes (signal handler for it)
 * @param sig       The signal for the signal handler
 **/
void handle_sigchild(int sig) {
  while (waitpid((pid_t)(-1), 0, WNOHANG) > 0)
    ;
  fprintf(stderr,
          "Child exited successfully with code %d. Reaped child process.\n",
          sig);
}

/**
 * Send the SYN to start the connection send the ACK to finsih the connection
 * and send the datafile name.
 * @param sockfd    Integer represendting the socket number
 * @param addr      The socaddr_in structure
 * @param fileName  The name of the file passed in!
 **/
void initiateConnection(int sockfd, struct sockaddr_in addr, string fileName) {

  // send SYN
  TCP_Packet p;
  socklen_t sin_size;
  clientSeqNum = rand() % 10000;
  p.setSeqNumber(clientSeqNum);
  p.setFlags(0, 1, 0);
  uint8_t packet[MSS];
  p.convertPacketToBuffer(packet);
  cout << "Sending packet " << p.getSeqNumber() << " SYN " << endl;
  if (sendto(sockfd, &packet, MSS, 0, (struct sockaddr *)&addr, sizeof(addr)) <
      0) {
    throwError("Could not send to the server");
  }
  uint8_t buf[MSS + 1];
  int recvlen;

  // Receive SYN ACK
  while (1) {
    recvlen =
        recvfrom(sockfd, buf, MSS, 0, (struct sockaddr *)&addr, &sin_size);
    if (recvlen > 0) {
      buf[recvlen] = 0;
      TCP_Packet rec;
      rec.convertBufferToPacket(buf);
      if (rec.getAck()) {
        cout << "Receiving packet " << rec.getSeqNumber() << endl;

        // Send back a packet with the filename
        TCP_Packet sendFilename;
        sendFilename.setData((uint8_t *)fileName.c_str(), fileName.length());
        sendFilename.setFlags(1, 0, 0);
        sendFilename.setAckNumber(rec.getSeqNumber());
        clientSeqNum += 1;
        serverSeqNum = rec.getSeqNumber();
        sendFilename.setSeqNumber(clientSeqNum);
        sendFilename.convertPacketToBuffer(packet);
        cout << "Sending packet " << sendFilename.getSeqNumber() << endl;
        if (sendto(sockfd, &packet, MSS, 0, (struct sockaddr *)&addr,
                   sizeof(addr)) < 0) {
          throwError("Could not send to the server");
        }
        return;
      }
    }
  }
}

/**
 * Send the FIN to close the connection
 **/
void closeConnection() {}

/**
 * Send the ACK back for the fin from the server
 **/
void handleClose() {}

/**
 * Assemble the given file from chunks into a coherent file
 **/
void assembleFileFromChunks() {}

/**
 * Attach a timer to the different segments and poll them at a certain rate??
 **/
void createSegmentTimer() {}

int main(int argc, char *argv[]) {
  // <server hostname><server portnumber><filename> --> Inputs from the
  // console

  // Processing the arguments:
  if (argc != 4) {
    throwError("Please input 3 arguments in this order: hostname, portnumber, "
               "filename. Exiting the program ...");
  }

  string hostName(argv[1]);
  string fileName(argv[3]);

  port = atoi(argv[2]);
  if (port < 1024) {
    throwError("Could not process int or trying to use privileged port. "
               "Exiting the program ...");
  }

  int sockfd;
  struct hostent *server;
  struct sockaddr_in addr;
  socklen_t sin_size;
  uint8_t buf[MSS + 1];
  int recvlen;
  vector<char> fileBuffer;

  if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    throwError("socket");
  server = gethostbyname(hostName.c_str());

  if (server == NULL)
    throwError("Could not find server");

  memset((char *)&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  memcpy((char *)&addr.sin_addr.s_addr, (char *)server->h_addr,
         server->h_length);
  addr.sin_port = htons(port);

  initiateConnection(sockfd, addr, fileName);

  // Then do other things here!
  while(1){
    recvlen =recvfrom(sockfd, buf, MSS, 0, (struct sockaddr *)&addr, &sin_size);
    buf[recvlen] = 0;
    TCP_Packet rec;
    rec.convertBufferToPacket(buf);
  }

  close(sockfd);
  return 0;
}
