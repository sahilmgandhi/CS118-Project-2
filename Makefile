CC=g++ -std=c++11
UID2=504742401
UID1=704598105

default:
	$(CC) -Wall -Wextra -g -o p2_server -I. server.cpp
	$(CC) -Wall -Wextra -g -o p2_client -I. client.cpp
	$(CC) -Wall -Wextra -g -o ec_p2_server -I. ec_server.cpp
	$(CC) -Wall -Wextra -g -o ec_p2_client -I. ec_client.cpp

dist:
	tar -cvzf project2_$(UID1)_$(UID2).tar server.cpp client.cpp globals.h tcp_packet.h ec_tcp_packet.h ec_server.cpp ec_client.cpp README.md report.pdf Makefile p2_slides.pptx
	zip project2_$(UID1)_$(UID2) server.cpp client.cpp globals.h tcp_packet.h ec_tcp_packet.h ec_server.cpp ec_client.cpp README.md report.pdf Makefile p2_slides.pptx


clean:
	rm -rf p2_server p2_client ec_p2_client ec_p2_server
	rm project2_$(UID1)_$(UID2).tar

ec:
	$(CC) -Wall -Wextra -g -o ec_p2_server -I. ec_server.cpp
	$(CC) -Wall -Wextra -g -o ec_p2_client -I. ec_client.cpp
