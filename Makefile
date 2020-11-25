CC = g++
CFLAGS = -c -Wall -O2 -std=c++11
PROGRAM_CLIENT = client
PROGRAM_SERVER = server
OBJS_CLIENT = client.o tands.o
OBJS_SERVER = server.o tands.o

all: $(PROGRAM_CLIENT) $(PROGRAM_SERVER)

debug: CFLAGS = -c -Wall -g -std=c++11
debug: $(PROGRAM)

$(PROGRAM_CLIENT): $(OBJS_CLIENT)
	$(CC) $(OBJS_CLIENT) -o client

$(PROGRAM_SERVER): $(OBJS_SERVER)
	$(CC) $(OBJS_SERVER) -o server

server.o: server.cpp
	$(CC) $(CFLAGS) server.cpp -o server.o

client.o: client.cpp
	$(CC) $(CFLAGS) client.cpp -o client.o

tands.o: tands.c tands.h
	$(CC) $(CFLAGS) tands.c -o tands.o

clean:
	@rm -f $(OBJS_CLIENT) $(OBJS_SERVER) $(PROGRAM_CLIENT) $(PROGRAM_SERVER)