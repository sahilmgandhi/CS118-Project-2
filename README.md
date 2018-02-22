# CS 118 Project 2 - TCP over UDP

This is project code for implementing a TCP connection with the SR (selective repeat) protocol over a UDP connection. The spec for this project can be seen in spec.pdf.

## Compiling project

Compile: ` make `

Create tarball:  `make dist`

Clean project: `make clean`

## Running code

These fields are required!
``` bash
./p2_server {$portnumber}
./p2_client {$hostname} {$filename} {$portnumber}
```