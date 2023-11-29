# author : Ravi Patidar 23M0796
all: server

server: queue.o gradingServer.o handle_clients.o
	cc -o server queue.o gradingServer.o handle_clients.o -lpthread

gradingServer.o: gradingServer.c queue.h handle_clients.h
	cc -c gradingServer.c

handle_clients.o: handle_clients.c handle_clients.h queue.h
	cc -c handle_clients.c 

queue.o: queue.c queue.h
	cc -c queue.c

clean:
	rm -f *.o server
