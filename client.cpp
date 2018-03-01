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
#include "globals.h"
#include "tcp_packet.h"

using namespace std;

int port = 5000;
/**
     * This method throws the perror and exits the program
     * @param s A string that is the error message
     **/
void throwError(string s) {
  perror(s.c_str());
  exit(1);
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

uint8_t *convertPacket(TCP_Packet p) {
  uint8_t *temp = new uint8_t[MSS];

  memcpy(temp, &p.header.seqNumber, sizeof(uint16_t));
  memcpy(temp + sizeof(uint16_t), &p.header.ackNumber, sizeof(uint16_t));
  memcpy(temp + 2 * sizeof(uint16_t), &p.header.flags[0], sizeof(uint8_t));
  memcpy(temp + 2 * sizeof(uint16_t) + sizeof(uint8_t), &p.header.flags[1],
         sizeof(uint8_t));
  memcpy(temp + 2 * sizeof(uint16_t) + 2 * sizeof(uint8_t), &p.header.flags[2],
         sizeof(uint8_t));
  memcpy(temp + 2 * sizeof(uint16_t) + 3 * sizeof(uint8_t), &p.header.dataLen,
         sizeof(uint16_t));
  return temp;
}

/**
 * Send the SYN to start the connection, and then the ACK to finsh off the
 *connection
 **/
void initateConnection() {}

/**
 * Send the FIN to close the connection
 **/
void closeConnection() {}

/**
 * Send the ACK back for the fin from the server
 **/
void handleClose() {}

/**
 * Break up the file into chunks to keep
 **/
void breakFileIntoChunks() {}

/**
 * Attach a timer to the different segments and poll them at a certain rate??
 **/
void createSegmentTimer() {}

int main(int argc, char *argv[]) {
  // <server hostname><server portnumber><filename> --> Inputs from the console

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
  char msg[] = "test";

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

  TCP_Packet p;
  p.header.flags[0] = 1;
  cout << "made it here" << endl;
  uint8_t *packet = convertPacket(p);
  cout << sizeof(packet) << endl;
  if (sendto(sockfd, packet, sizeof(packet), 0, (struct sockaddr *)&addr,
             sizeof(addr)) < 0)
    throwError("Could not send to the server");
  delete[] packet;
  close(sockfd);
  return 0;
}
