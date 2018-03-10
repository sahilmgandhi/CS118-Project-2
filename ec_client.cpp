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
#include <algorithm>
#include "globals.h"
#include "ec_tcp_packet.h"

using namespace std;

int port = 5000;
uint16_t clientSeqNum = 0;
long long trueFileSeqNum = 0;

EC_TCP_Packet initWindow[2];
vector<EC_TCP_Packet> packetWindow;

bool atLeastOnePacketWritten = false;
vector<uint8_t> fileVector;

uint32_t ackToSend = 0;

/**
 * This method throws the perror and exits the program
 * @param s           A string that is the error message
 **/
void throwError(string s) {
  perror(s.c_str());
  exit(1);
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
  EC_TCP_Packet p;
  socklen_t sin_size;
  clientSeqNum = rand() % 10000;
  p.setSeqNumber(clientSeqNum % EC_MAXSEQ);
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
      EC_TCP_Packet rec;
      rec.convertBufferToPacket(buf);
      cout << "Receiving packet " << rec.getSeqNumber() << endl;
      if (rec.getAck() && rec.getSyn() &&
          rec.getAckNumber() == initWindow[0].getSeqNumber()) {
        // Ack for the initial Syn packet

        initWindow[0].setAcked();
        // Send back a packet with the filename
        if (!initWindow[1].isSent()) {
          EC_TCP_Packet sendFilename;
          sendFilename.setData((uint8_t *)fileName.c_str(), fileName.length());
          sendFilename.setFlags(1, 0, 0);
          sendFilename.setAckNumber(rec.getSeqNumber() % EC_MAXSEQ);
          clientSeqNum += 1;
          sendFilename.setSeqNumber(clientSeqNum % EC_MAXSEQ);
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
        cout << "Sending packet " << initWindow[i].getSeqNumber()
             << " Retransmission " << endl;
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

void setSendAckNum() {
  sort(packetWindow.begin(), packetWindow.end());
  for (int i = 0; i < packetWindow.size() - 1; i++) {
    if (packetWindow[i].getSeqNumber() + MSS !=
        packetWindow[i + 1].getSeqNumber()) {
      ackToSend = packetWindow[i].getSeqNumber() + MSS;
      return;
    }
  }
  ackToSend = packetWindow[packetWindow.size() - 1].getSeqNumber() + MSS;
}

/**
 * Receives the file from the server
 * @param sockfd        int referring to sock file descriptor
 * @param addr          struct for sockaddr_in
 * @return              The final sequence number of the fin
 **/
int receiveFile(int sockfd, struct sockaddr_in addr) {
  socklen_t sin_size;
  uint8_t buf[MSS + 1];
  uint8_t data[MSS];
  uint8_t packet[MSS];
  int recvlen;
  EC_TCP_Packet ack;
  int dup = 0;
  while (1) {
    recvlen = recvfrom(sockfd, buf, MSS, 0 | MSG_DONTWAIT,
                       (struct sockaddr *)&addr, &sin_size);
    if (recvlen > 0) {
      dup = 0;
      buf[recvlen] = 0;
      EC_TCP_Packet rec;

      rec.convertBufferToPacket(buf);
      cout << "Receiving packet " << rec.getSeqNumber() << endl;
      if (rec.getFin()) {
        return rec.getSeqNumber();
      } else {
        if (packetWindow.size() > 0) {
          for (unsigned long i = 0; i < packetWindow.size(); i++) {
            if (packetWindow[i].getSeqNumber() == rec.getSeqNumber()) {
              cout << "Sending packet " << ackToSend << " Retransmission"
                   << endl;
              ack.setAckNumber(ackToSend % EC_MAXSEQ);
              dup = 1;
              break;
            }
          }
        }
        if (dup == 0) {
          atLeastOnePacketWritten = true;
          rec.getData(data);
          clientSeqNum++;
          packetWindow.push_back(rec);
          uint32_t oldAck = ackToSend;
          setSendAckNum();
          ack.setAckNumber(ackToSend % EC_MAXSEQ);
          if (oldAck == ackToSend) {
            cout << "Sending packet " << ackToSend << " Retransmission" << endl;
          } else {
            cout << "Sending packet " << ackToSend << endl;
          }
        }
        ack.setSeqNumber(clientSeqNum % EC_MAXSEQ);
        ack.setFlags(1, 0, 0);
        ack.convertPacketToBuffer(packet);
        if (sendto(sockfd, &packet, MSS, 0, (struct sockaddr *)&addr,
                   sizeof(addr)) < 0)
          throwError("Could not send to the server");
      }
    }
  }
}

/**
 * Assemble the given file from chunks into a coherent file
 * @param fileVector  The vector containing the raw file data
 **/
void assembleFileFromChunks() {
  uint8_t *fileBuffer;
  for (int i = 0; i < packetWindow.size(); i++) {
    for (int j = 0; j < packetWindow[i].getLen(); j++) {
      fileVector.push_back(packetWindow[i].data[j]);
    }
  }
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
  EC_TCP_Packet finPacket;
  EC_TCP_Packet ackPacket;
  socklen_t sin_size;
  uint8_t buf[MSS + 1];
  clientSeqNum++;

  ackPacket.setSeqNumber(clientSeqNum % EC_MAXSEQ);
  ackPacket.setAckNumber(finAckNum % EC_MAXSEQ);
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
  finPacket.setSeqNumber(clientSeqNum % EC_MAXSEQ);
  finPacket.setAckNumber((finAckNum + 1) % EC_MAXSEQ);
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
      EC_TCP_Packet recPacket;
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
      cout << "Sending packet " << finPacket.getAckNumber()
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
  finSeqNum = receiveFile(sockfd, addr);

  closeConnection(sockfd, addr, finSeqNum);
  if (!atLeastOnePacketWritten) {
    cout << "404 Error! The packet was not found on the server side! Closing "
            "the client program"
         << endl;
  } else {
    sort(packetWindow.begin(), packetWindow.end());
    assembleFileFromChunks();
  }
  close(sockfd);
  return 0;
}
