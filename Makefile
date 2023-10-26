# author : Ravi Patidar 23M0796
all: server

server: queueSupport.o start.o gradingServer.o
	cc -o server queueSupport.o start.o gradingServer.o -lpthread

queueSupport.o: queueSupport.c support.h
	cc -c queueSupport.c

start.o: start.c support.h
	cc -c start.c

gradingServer.o: gradingServer.c support.h
	cc -c gradingServer.c

clean:
	rm -f *.o
