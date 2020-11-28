CC = g++
CFLAGS = -c -Wall -O2 -std=c++11
PROGRAM_CLIENT = client
PROGRAM_SERVER = server
OBJS_CLIENT = client.o tands.o util.o
OBJS_SERVER = server.o tands.o util.o
SERVER_MAN_PAGE = server.pdf
CLIENT_MAN_PAGE = client.pdf

all: $(PROGRAM_CLIENT) $(PROGRAM_SERVER) $(SERVER_MAN_PAGE) $(CLIENT_MAN_PAGE)

man: $(MAN_PAGES)

debug: CFLAGS = -c -Wall -g -std=c++11
debug: $(PROGRAM_CLIENT) $(PROGRAM_SERVER)

$(PROGRAM_CLIENT): $(OBJS_CLIENT)
	$(CC) $(OBJS_CLIENT) -o client

$(PROGRAM_SERVER): $(OBJS_SERVER)
	$(CC) $(OBJS_SERVER) -o server

$(SERVER_MAN_PAGE): 
	man -t ./server.man | ps2pdf - server.pdf

$(CLIENT_MAN_PAGE):
	man -t ./client.man | ps2pdf - client.pdf

server.o: server.cpp
	$(CC) $(CFLAGS) server.cpp -o server.o

client.o: client.cpp
	$(CC) $(CFLAGS) client.cpp -o client.o

util.o: util.cpp util.h
	$(CC) $(CFLAGS) util.cpp -o util.o

tands.o: tands.c tands.h
	$(CC) $(CFLAGS) tands.c -o tands.o

clean:
	@rm -f $(OBJS_CLIENT) $(OBJS_SERVER) $(PROGRAM_CLIENT) $(PROGRAM_SERVER) $(SERVER_MAN_PAGE) $(CLIENT_MAN_PAGE)
