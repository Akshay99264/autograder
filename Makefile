# author : Ravi Patidar 23M0796
all: server

server: 
	gcc -w gradingServer.c -o server -lpthread -luuid -I/usr/include/postgresql -lpq -std=c99

clean:
	rm -f *.o server
