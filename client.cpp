// Sahil Gandhi and Arpit Jasapara
// CS 118 Winter 2018
// Project 2
// This is the client code for the second project.

#include <iostream>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <netdb.h>
#include <strings.h>
#include <signal.h>
#include <string>
#include <time.h>
#include <fcntl.h>
#include <locale>
#include <fstream>

using namespace std;

#define PORT 5000


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

int main() {
  int sockfd;
  struct hostent* server;
  struct sockaddr_in addr;
  char* msg = "test";
  if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    throwError("socket");
  server = gethostbyname("localhost");
  if(server == NULL) 
    throwError("server");
  memset((char*) &addr,0, sizeof(addr));
  addr.sin_family = AF_INET;
  memcpy((char *) &addr.sin_addr.s_addr, (char*) server->h_addr, server->h_length);
  addr.sin_port = htons(PORT);
  
  if (sendto(sockfd, msg, strlen(msg), 0, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    throwError("sendto failed");
  close(sockfd);
  return 0;
}