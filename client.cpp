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
uint16_t clientSeqNum = 0;
long long trueFileSeqNum = 0;

TCP_Packet initWindow[2];
TCP_Packet movingPackWind[26];
int startWindSeq = 0;
int lastWindSeq = 0;
vector<TCP_Packet> packetWindow;
vector<uint8_t> fileVector;

/**
 * This method throws the perror and exits the program
 * @param s           A string that is the error message
 **/
void throwError(string s) {
  perror(s.c_str());
  exit(1);
}

/**
 * This method will reap zombie processes (signal handler for it)
 * @param sig         The signal for the signal handler
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
 * @param sockfd      Integer representing the socket number
 * @param addr        The socaddr_in structure
 * @param fileName    The name of the file passed in!
 **/
void initiateConnection(int sockfd, struct sockaddr_in addr, string fileName) {

  // send SYN
  TCP_Packet p;
  socklen_t sin_size;
  clientSeqNum = rand() % 10000;
  p.setSeqNumber(clientSeqNum % MAXSEQ);
  p.setFlags(0, 1, 0);
  uint8_t packet[MSS];
  p.convertPacketToBuffer(packet);
  cout << "Sending packet " << p.getSeqNumber() << " SYN " << endl;
  if (sendto(sockfd, &packet, MSS, 0, (struct sockaddr *)&addr, sizeof(addr)) <
      0) {
    throwError("Could not send to the server");
  }
  p.startTimer();
  p.setSent();
  initWindow[0] = p;

  uint8_t buf[MSS + 1];
  int recvlen;

  // Receive SYN ACK
  while (1) {
    recvlen = recvfrom(sockfd, buf, MSS, 0 | MSG_DONTWAIT,
                       (struct sockaddr *)&addr, &sin_size);
    if (recvlen > 0) {
      buf[recvlen] = 0;
      TCP_Packet rec;
      rec.convertBufferToPacket(buf);
      cout << "Receiving packet " << rec.getSeqNumber() << endl;
      if (rec.getAck() && rec.getSyn() &&
          rec.getAckNumber() == initWindow[0].getSeqNumber()) {
        // Ack for the initial Syn packet

        initWindow[0].setAcked();
        // Send back a packet with the filename
        if (!initWindow[1].isSent()) {
          TCP_Packet sendFilename;
          sendFilename.setData((uint8_t *)fileName.c_str(), fileName.length());
          sendFilename.setFlags(1, 0, 0);
          sendFilename.setAckNumber(rec.getSeqNumber() % MAXSEQ);
          clientSeqNum += 1;
          sendFilename.setSeqNumber(clientSeqNum % MAXSEQ);
          clientSeqNum += 1;
          sendFilename.convertPacketToBuffer(packet);
          cout << "Sending packet " << sendFilename.getSeqNumber() << endl;
          if (sendto(sockfd, &packet, MSS, 0, (struct sockaddr *)&addr,
                     sizeof(addr)) < 0) {
            throwError("Could not send to the server");
          }
          sendFilename.startTimer();
          sendFilename.setSent();
          initWindow[1] = sendFilename;
        } else {
          // we are receiving the syn again, so reset the timer and resend the
          // packet. It keeps its flags, ack number, data, and seq number, just
          // the timer will be restarted.

          initWindow[1].convertPacketToBuffer(packet);
          cout << "Sending packet " << initWindow[1].getSeqNumber() << endl;
          if (sendto(sockfd, &packet, MSS, 0, (struct sockaddr *)&addr,
                     sizeof(addr)) < 0) {
            throwError("Could not send to the server");
          }
          initWindow[1].startTimer();
        }
      } else if (rec.getAck() &&
                 rec.getAckNumber() == initWindow[1].getSeqNumber()) {
        // Ack for the filename packet
        initWindow[1].setAcked();
        startWindSeq = rec.getSeqNumber();
      }
      // else we just ignore it, since we are not dealing with that right now,
      // JUST the initial handshake
    }
    // Always poll the other packets in the init window and see if we have to
    // send anything to the server again!
    int counter = 0;
    for (int i = 0; i < 2; i++) {
      if (initWindow[i].isSent() && initWindow[i].isAcked()) {
        counter++;
      } else if (initWindow[i].isSent() && initWindow[i].hasTimedOut(1)) {
        initWindow[i].convertPacketToBuffer(packet);
        cout << "Sending packet " << initWindow[i].getSeqNumber() << endl;
        if (sendto(sockfd, &packet, MSS, 0, (struct sockaddr *)&addr,
                   sizeof(addr)) < 0) {
          throwError("Could not send to the server");
        }
        initWindow[i].startTimer();
      }
      // else if it hasnt timed out or has not been sent yet, then we don't need
      // to worry about it
    }
    if (counter == 2) {
      // Both the packets were acked, so we can return from this and continue
      // with receiving the files
      break;
    }
    memset((char *)&buf, 0, MSS + 1);
  }
}

void initializeMovingWindow() {
  for (int i = 1; i < 27; i++) {
    movingPackWind[i - 1].setSeqNumber((startWindSeq + i * MSS) % MAXSEQ);
  }
  lastWindSeq = movingPackWind[25].getSeqNumber();
}

int receiveFile(int sockfd, struct sockaddr_in addr) {
  socklen_t sin_size;
  uint8_t buf[MSS + 1];
  uint8_t data[MSS];
  uint8_t packet[MSS];
  int recvlen;
  TCP_Packet ack;
  int dup = 0;
  while (1) {
    recvlen = recvfrom(sockfd, buf, MSS, 0 | MSG_DONTWAIT,
                       (struct sockaddr *)&addr, &sin_size);
    if (recvlen > 0) {
      dup = 0;
      buf[recvlen] = 0;
      TCP_Packet rec;

      rec.convertBufferToPacket(buf);
      cout << "Receiving packet " << rec.getSeqNumber() << endl;
      if (rec.getFin()) {
        // Write the rest to the buffer:
        for (int i = 0; i < 26; i++) {
          if (movingPackWind[i].isAcked()) {
            movingPackWind[i].getData(data);
            for (int i = 0; i < movingPackWind[i].getLen(); i++) {
              fileVector.push_back(data[i]);
            }
          }
        }
        return rec.getSeqNumber();
      }
      ack.setAckNumber(rec.getSeqNumber() % MAXSEQ);
      ack.setSeqNumber(clientSeqNum % MAXSEQ);
      ack.setFlags(1, 0, 0);
      ack.convertPacketToBuffer(packet);

      int currSeqNum = rec.getSeqNumber();
      int numPacketsToWriteToFile = 0;
      for (int i = 1; i < 5; i++) {
        if (currSeqNum == ((lastWindSeq + i * MSS) % MAXSEQ)) {
          numPacketsToWriteToFile = i;
          break;
        }
      }
      if (numPacketsToWriteToFile == 0) {
        for (int i = 0; i < 26; i++) {
          if (movingPackWind[i].getSeqNumber() == rec.getSeqNumber() &&
              movingPackWind[i].isAcked()) {
            dup = 1;
            break;
          } else if (movingPackWind[i].getSeqNumber() == rec.getSeqNumber() &&
                     !movingPackWind[i].isAcked()) {
            movingPackWind[i].setAcked();
            break;
          }
        }
      }

      if (dup == 0) {
        cout << "Sending packet " << ack.getAckNumber() << endl;
      } else if (dup == 1) {
        cout << "Sending packet " << ack.getAckNumber() << " Retransmission"
             << endl;
      }
      if (sendto(sockfd, &packet, MSS, 0, (struct sockaddr *)&addr,
                 sizeof(addr)) < 0) {
        throwError("Could not send to the server");
      }
      if (dup == 0) {
        // here we have to push out the window and update lastWindSeq, or do
        // nothing!
        if (numPacketsToWriteToFile > 0) {
          for (int i = 0; i < numPacketsToWriteToFile; i++) {
            if (movingPackWind[i].isAcked()) {
              movingPackWind[i].getData(data);
              for (int i = 0; i < movingPackWind[i].getLen(); i++) {
                fileVector.push_back(data[i]);
              }
            }
          }
          for (int i = 0; i < 26 - numPacketsToWriteToFile; i++) {
            movingPackWind[i] = movingPackWind[i + 1];
          }
          int counter = 1;
          for (int i = 26 - numPacketsToWriteToFile; i < 26; i++) {
            movingPackWind[i].setSeqNumber((lastWindSeq + counter * MSS) %
                                           MAXSEQ);
            counter++;
          }
          lastWindSeq = movingPackWind[25].getSeqNumber();
        }
      }

      if (dup == 0) {
        rec.getData(data);
        clientSeqNum++;
        packetWindow.push_back(rec);
        for (int i = 0; i < rec.getLen(); i++)
          fileVector.push_back(data[i]);
      }
      dup = 0; // reset the dup flag
    }
  }
}

/**
 * Assemble the given file from chunks into a coherent file
 * @param fileVector  The vector containing the raw file data
 **/
void assembleFileFromChunks() {
  uint8_t *fileBuffer;
  fileBuffer = new uint8_t[fileVector.size() + 1];
  for (unsigned long i = 0; i < fileVector.size(); i++) {
    fileBuffer[i] = fileVector[i];
    // cout << fileVector[i];
  }
  fileBuffer[fileVector.size()] = 0;
  ofstream outFile;
  outFile.open("received.data", ios::out | ios::binary);
  if (outFile.is_open()) {
    outFile.write((const char *)fileBuffer, (streamsize)(fileVector.size()));
    outFile.close();
  }

  delete fileBuffer;
}

/**
 * Received a FIN from the server, and starting its own FIN sequence
 * @param sockfd      Integer representing the socket number
 * @param addr        The socaddr_in structure
 * @param finAckNum   The ack number to send back for the fin received to
 *                    terminate the connection
 **/
void closeConnection(int sockfd, struct sockaddr_in addr, int finAckNum) {
  // send SYN
  TCP_Packet finPacket;
  TCP_Packet ackPacket;
  socklen_t sin_size;
  uint8_t buf[MSS + 1];
  clientSeqNum++;

  ackPacket.setSeqNumber(clientSeqNum % MAXSEQ);
  ackPacket.setAckNumber(finAckNum % MAXSEQ);
  ackPacket.setFlags(1, 0, 0);
  uint8_t packet[MSS];
  bool hasBeenReSent = false;
  ackPacket.convertPacketToBuffer(packet);
  cout << "Sending packet " << ackPacket.getAckNumber() << endl;
  if (sendto(sockfd, &packet, MSS, 0, (struct sockaddr *)&addr, sizeof(addr)) <
      0) {
    throwError("Could not send to the server");
  }
  ackPacket.setSent();

  clientSeqNum++;
  finPacket.setSeqNumber(clientSeqNum % MAXSEQ);
  finPacket.setAckNumber((finAckNum + 1) % MAXSEQ);
  finPacket.setFlags(0, 0, 1);
  finPacket.convertPacketToBuffer(packet);
  cout << "Sending packet " << finPacket.getAckNumber() << " FIN " << endl;
  if (sendto(sockfd, &packet, MSS, 0, (struct sockaddr *)&addr, sizeof(addr)) <
      0) {
    throwError("Could not send to the server");
  }
  finPacket.setSent();
  finPacket.startTimer();

  int recvlen;

  while (1) {
    recvlen = recvfrom(sockfd, buf, MSS, 0 | MSG_DONTWAIT,
                       (struct sockaddr *)&addr, &sin_size);
    if (recvlen > 0) {
      buf[recvlen] = 0;
      TCP_Packet recPacket;
      recPacket.convertBufferToPacket(buf);

      cout << "Receiving packet " << recPacket.getSeqNumber() << endl;
      if (recPacket.getFin() && recPacket.getSeqNumber() == finAckNum) {
        // Duplicate fin from the server. Resend the ack for it
        ackPacket.convertPacketToBuffer(packet);
        cout << "Sending packet " << ackPacket.getAckNumber()
             << " Retransmission " << endl;
        if (sendto(sockfd, &packet, MSS, 0, (struct sockaddr *)&addr,
                   sizeof(addr)) < 0) {
          throwError("Could not send to the server");
        }
      } else if (recPacket.getAck() &&
                 recPacket.getAckNumber() == finPacket.getSeqNumber()) {
        // We have gotten the ack for the fin and can now end
        break;
      }
    }
    if (finPacket.hasTimedOut(1) && !finPacket.hasTimedOut(2) &&
        !hasBeenReSent) {
      cout << "Sending packet " << finPacket.getSeqNumber()
           << " Retransmission "
           << " FIN " << endl;
      if (sendto(sockfd, &packet, MSS, 0, (struct sockaddr *)&addr,
                 sizeof(addr)) < 0) {
        throwError("Could not send to the server");
      }
      hasBeenReSent = true;
    } else if (finPacket.hasTimedOut(2) && hasBeenReSent) {
      break;
    }
  }
}

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
  int finSeqNum = 0;

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
  initializeMovingWindow();
  finSeqNum = receiveFile(sockfd, addr);
  closeConnection(sockfd, addr, finSeqNum);
  assembleFileFromChunks();
  close(sockfd);
  return 0;
}
