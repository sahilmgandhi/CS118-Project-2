#include "globals.h"
#include "stdint.h"

class TCP_Packet {

public:
  struct TCP_Header {
    // Total size is
    uint16_t seqNumber = 0; // 2 bytes
    uint16_t ackNumber = 0; // 2 bytes

    // pos 0 = ack, pos 1 = syn, pos 2 = fin
    uint8_t flags[3] = {0, 0, 0}; // 3 bytes
    uint16_t dataLen = 0;         // 2 bytes
    // set the cwnd for extra credit portion. Default is 5120 bytes (5 packets)
  } header;

  uint8_t data[1015] = {0}; // 1015 bytes for data

  // Store the data in a vector
  // Mark packet as sent and acked as necessary
  // Packets by default arent acked or sent

  // bool sent = false;
  // bool acked = false;
};