// Sahil Gandhi and Arpit Jasapara
// CS 118 Winter 2018
// Project 2
// This is the server code for the second project.

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
#define BUFSIZE 2048


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
  int sockfd, recvlen;
  struct sockaddr_in my_addr;
  struct sockaddr_in their_addr;
  unsigned char buf[BUFSIZE]; 
  socklen_t sin_size;
  if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    throwError("socket");
  memset((char *)&my_addr, 0, sizeof(my_addr));
  my_addr.sin_family = AF_INET;
  my_addr.sin_port = htons(PORT);
  my_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  if (bind(sockfd, (struct sockaddr *)&my_addr, sizeof(struct sockaddr)) < 0)
      throwError("bind");
  sin_size = sizeof(struct sockaddr_in);
  // Start listening in on connections:
  while (1) {
    recvlen = recvfrom(sockfd, buf, BUFSIZE, 0, (struct sockaddr *)&their_addr, &sin_size);
    if (recvlen > 0){
      buf[recvlen] = 0;
      printf("received message: \"%s\"\n", buf);
    }
  }
  close(sockfd);
  return 0;
}