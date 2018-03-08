#include "globals.h"
#include "stdint.h"
#include <time.h>

class TCP_Packet {

public:
  struct TCP_Header {
    // Total size is 9 Bytes
    uint16_t seqNumber = 0; // 2 bytes
    uint16_t ackNumber = 0; // 2 bytes

    // pos 0 = ack, pos 1 = syn, pos 2 = fin
    uint8_t flags[FLAGS] = {0}; // 3 bytes
    uint16_t dataLen = 0;       // 2 bytes
    // set the cwnd for extra credit portion. Default is 5120 bytes (5 packets)
  } header;

  // Member Variables
  uint8_t data[PACKET_SIZE] = {0}; // 1015 bytes for data
  bool sent = false;
  bool acked = false;
  long long trueFileSeqNum = 0;
  struct timespec start;

  // constructor
  TCP_Packet() {
    memset((char *)&data, 0, PACKET_SIZE);
    memset((char *)&header.flags, 0, FLAGS);
  }

  TCP_Packet &operator=(const TCP_Packet &other) {
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
      for (int i = 0; i < PACKET_SIZE; i++) {
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
  uint16_t getSeqNumber() { return header.seqNumber; }
  uint16_t getAckNumber() { return header.ackNumber; }

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
    if (len > PACKET_SIZE) {
      std::cout << "Data length larger than PACKET_SIZE. Not modifying data"
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
    for (int i = 0; i < PACKET_SIZE; i++) {
      data[i] = 0;
    }
  }

  void resetAcked() { acked = false; }

  void startTimer() {
    if (clock_gettime(CLOCK_MONOTONIC, &start) == -1) {
      perror("clock gettime");
    }
  }

  void setSeqNumber(uint16_t seq) { header.seqNumber = seq; }
  void setAckNumber(uint16_t ack) { header.ackNumber = ack; }

  void setSent() { sent = true; }
  void setAcked() { acked = true; }

  // Converters from Stream to Packet and Vice Versa
  void convertBufferToPacket(uint8_t *buff) {
    header.seqNumber = (buff[1] << 8) | buff[0];
    header.ackNumber = (buff[3] << 8) | buff[2];
    header.flags[0] = buff[4];
    header.flags[1] = buff[5];
    header.flags[2] = buff[6];
    header.dataLen = (buff[8] << 8) | buff[7];

    for (int i = 0; i < header.dataLen; i++) {
      data[i] = buff[9 + i];
    }
  }

  void convertPacketToBuffer(uint8_t *temp) {
    memset((char *)temp, 0, MSS);

    memcpy(temp, &header.seqNumber, sizeof(uint16_t));
    memcpy(temp + sizeof(uint16_t), &header.ackNumber, sizeof(uint16_t));
    memcpy(temp + 2 * sizeof(uint16_t), &header.flags[0], sizeof(uint8_t));
    memcpy(temp + 2 * sizeof(uint16_t) + sizeof(uint8_t), &header.flags[1],
           sizeof(uint8_t));
    memcpy(temp + 2 * sizeof(uint16_t) + 2 * sizeof(uint8_t), &header.flags[2],
           sizeof(uint8_t));
    memcpy(temp + 2 * sizeof(uint16_t) + 3 * sizeof(uint8_t), &header.dataLen,
           sizeof(uint16_t));
    if (header.dataLen > 0) {
      memcpy(temp + 3 * sizeof(uint16_t) + 3 * sizeof(uint8_t), &data,
             header.dataLen);
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
  bool sortBySeqNum(const TCP_Packet &lhs, const TCP_Packet &rhs) {
    return lhs.trueFileSeqNum < rhs.trueFileSeqNum;
  }
};