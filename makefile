all: server client
server: server.o utils.o
	cc  server.o utils.o -pthread -o server
server.o: server.c
	cc -c server.c
client: client.o utils.o mq.o
	cc  client.o utils.o mq.o -pthread -o client
client.o: client.c
	cc -c client.c 
utils.o: utils.h
	cc -c utils.c
mq.o: mq.h
	cc -c mq.c

clean :
	rm server.o utils.o client.o mq.o
