#include "globals.h"
#include "stdint.h"
#include <time.h>

class EC_TCP_Packet {

public:
  struct EC_TCP_Header {
    // Total size is 9 Bytes
    uint32_t seqNumber = 0; // 4 bytes
    uint32_t ackNumber = 0; // 4 bytes

    // pos 0 = ack, pos 1 = syn, pos 2 = fin
    uint8_t flags[FLAGS] = {0}; // 3 bytes
    uint16_t dataLen = 0;       // 2 bytes
  } header;                     // Total Header Size = 13 Bytes

  // Member Variables
  uint8_t data[EC_PACKET_SIZE] = {0}; // 1011 bytes for data
  bool sent = false;
  bool acked = false;
  struct timespec start;

  // constructor
  EC_TCP_Packet() {
    memset((char *)&data, 0, EC_PACKET_SIZE);
    memset((char *)&header.flags, 0, FLAGS);
  }

  EC_TCP_Packet &operator=(const EC_TCP_Packet &other) {
    if (this != &other) {
      sent = other.sent;
      acked = other.acked;
      header.seqNumber = other.header.seqNumber;
      header.ackNumber = other.header.ackNumber;
      header.dataLen = other.header.dataLen;
      header.flags[0] = other.header.flags[0];
      header.flags[1] = other.header.flags[1];
      header.flags[2] = other.header.flags[2];
      start = other.start;
      for (int i = 0; i < EC_PACKET_SIZE; i++) {
        data[i] = other.data[i];
      }
    }
    return *this;
  }

  // Getters:
  bool isSent() { return sent; }
  bool isAcked() { return acked; }
  uint16_t getLen() { return header.dataLen; }
  bool getAck() { return header.flags[0] == 1; }
  bool getSyn() { return header.flags[1] == 1; }
  bool getFin() { return header.flags[2] == 1; }
  uint32_t getSeqNumber() { return header.seqNumber; }
  uint32_t getAckNumber() { return header.ackNumber; }
  // uint32_t getCwnd() { return header.cwnd; }

  void getData(uint8_t *buff) {
    memset((char *)buff, 0, MSS);
    for (int i = 0; i < header.dataLen; i++)
      buff[i] = data[i];
  }

  // Setters:
  void setFlags(uint8_t a, uint8_t s, uint8_t f) {
    header.flags[0] = a;
    header.flags[1] = s;
    header.flags[2] = f;
  }

  void setData(uint8_t *buff, int len) {
    if (len > EC_PACKET_SIZE) {
      std::cout << "Data length larger than EC_PACKET_SIZE. Not modifying data"
                << std::endl;
      return;
    } else {
      for (int i = 0; i < len; i++) {
        data[i] = buff[i];
      }
      header.dataLen = len;
    }
  }

  void resetData() {
    for (int i = 0; i < EC_PACKET_SIZE; i++) {
      data[i] = 0;
    }
  }

  void resetAcked() { acked = false; }

  void startTimer() {
    if (clock_gettime(CLOCK_MONOTONIC, &start) == -1) {
      perror("clock gettime");
    }
  }

  void setSeqNumber(uint32_t seq) { header.seqNumber = seq; }
  void setAckNumber(uint32_t ack) { header.ackNumber = ack; }

  void setSent() { sent = true; }
  void setAcked() { acked = true; }

  // Converters from Stream to Packet and Vice Versa
  void convertBufferToPacket(uint8_t *buff) {
    header.seqNumber = (buff[3] << 24) | (buff[2] << 16);
    header.seqNumber |= (buff[1] << 8);
    header.seqNumber |= buff[0];
    header.ackNumber = (buff[7] << 24) | (buff[6] << 16);
    header.ackNumber |= (buff[5] << 8);
    header.ackNumber |= buff[4];
    header.flags[0] = buff[8];
    header.flags[1] = buff[9];
    header.flags[2] = buff[10];
    header.dataLen = (buff[12] << 8) | buff[11];

    for (int i = 0; i < header.dataLen; i++) {
      data[i] = buff[13 + i];
    }
  }

  void convertPacketToBuffer(uint8_t *temp) {
    memset((char *)temp, 0, MSS);

    memcpy(temp, &header.seqNumber, sizeof(uint32_t));
    memcpy(temp + sizeof(uint32_t), &header.ackNumber, sizeof(uint32_t));
    memcpy(temp + 2 * sizeof(uint32_t), &header.flags[0], sizeof(uint8_t));
    memcpy(temp + 2 * sizeof(uint32_t) + sizeof(uint8_t), &header.flags[1],
           sizeof(uint8_t));
    memcpy(temp + 2 * sizeof(uint32_t) + 2 * sizeof(uint8_t), &header.flags[2],
           sizeof(uint8_t));
    memcpy(temp + 2 * sizeof(uint32_t) + 3 * sizeof(uint8_t), &header.dataLen,
           sizeof(uint16_t));
    if (header.dataLen > 0) {
      memcpy(temp + sizeof(uint16_t) + 2 * sizeof(uint32_t) +
                 3 * sizeof(uint8_t),
             &data, header.dataLen);
    }
  }

  // Misc functions
  bool hasTimedOut(int numRTO) {
    struct timespec stop;
    if (clock_gettime(CLOCK_MONOTONIC, &stop) == -1) {
      perror("clock gettime");
    }

    // std::cout << (long double)((stop.tv_sec - start.tv_sec) +
    //                            (long double)(stop.tv_nsec - start.tv_nsec) /
    //                                BILLION)
    //           << std::endl;
    return ((long double)((stop.tv_sec - start.tv_sec) +
                          (long double)(stop.tv_nsec - start.tv_nsec) /
                              BILLION) > numRTO * RTO);
  }

  // Comparator for
  bool operator<(const EC_TCP_Packet &rhs) {
    return header.seqNumber < rhs.header.seqNumber;
  }
};