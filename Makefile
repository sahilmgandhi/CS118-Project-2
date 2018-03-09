CC=g++ -std=c++11
UID1=504742401
UID2=704598105

default:
	$(CC) -Wall -Wextra -g -o p2_server -I. server.cpp
	$(CC) -Wall -Wextra -g -o p2_client -I. client.cpp

dist:
	tar -cvzf $project2_(UID1)_(UID2).tar server.cpp client.cpp README report.pdf Makefile

clean:
	rm -rf p2_server p2_client
	rm $project2_(UID1)_(UID2).tar

ec:
	$(CC) -Wall -Wextra -g -o ec_p2_server -I. ec_server.cpp
	$(CC) -Wall -Wextra -g -o ec_p2_client -I. ec_client.cpp
