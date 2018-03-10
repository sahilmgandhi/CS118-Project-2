# CS 118 Project 2 - TCP over UDP

This is project code for implementing a TCP connection with the SR (selective repeat) protocol over a UDP connection. The spec for this project can be seen in spec.pdf.

## Compiling project

Compile: ` make `

Compile extra credit: ` make ec `

Create tarball:  `make dist`

Clean project: `make clean`

## Running code

Non Extra credit:
These fields are required!
``` bash
./p2_server {$portnumber}
./p2_client {$hostname} {$filename} {$portnumber}
```

Extra credit:
These fields are required!
``` bash
./ec_p2_server {$portnumber}
./ec_p2_client {$hostname} {$filename} {$portnumber}
```

## Division of work:

We divided the work by incrementally building up different helper methods and functions (like initiating handshake, chunking file, putting it together, fin seq, etc) and handling one of these ourselves and making good comments and communicating well so that they could all fit together properly and had well defined behavior. As such implementing the non extra credit portion was not too difficult and we were able to handle it pretty well.