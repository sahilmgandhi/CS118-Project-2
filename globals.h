#ifndef GLOBALS_H
#define GLOBALS_H

#define PACKET_SIZE 1024
#define HEADER_SIZE 8
#define MSS 1032 // Packet + Header size
#define RTO 500

// Macros for TCP Header
#define SEQ 0
#define ACK 1
#define WIN 2
#define FLAGS 3
#define NUM_FIELDS 4

// Macros for congestion control
// **ONLY IF WE attempt this for extra credit**
#define SS 0
#define CA 1

#define INIT_CWND 1024
#define INIT_SSTHRESH 15360

#endif