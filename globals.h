#ifndef GLOBALS_H
#define GLOBALS_H

#define PACKET_SIZE 1015
#define HEADER_SIZE 9
#define MSS 1024 // Packet + Header size
#define RTO 0.5  // In seconds (500 ms)

#define EC_PACKET_SIZE 1011
#define EC_HEADER_SIZE 13
#define EC_MAXSEQ 2147483648

#define MAXSEQ 30720 // This is a value in bytes
#define WINDOW 5120  // This is a value in bytes

// Macros for TCP Header
// #define WIN 2
#define FLAGS 3
#define NUM_FIELDS 3

// Macros for congestion control
// **ONLY IF WE attempt this for extra credit**
#define SS 0
#define CA 1

#define INIT_CWND 1024
#define INIT_SSTHRESH 15360
#define EC_RWND 100

// Random other Macros
#define BILLION 1000000000L
#define RECEIVEWINDOW 25

#endif