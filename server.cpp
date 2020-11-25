#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/time.h> 
#include <string>
#include <unistd.h>
#include "tands.h"

using namespace std;

#define MAX_BACKLOG 50

string getEpochTime() {
    timeval time;
	gettimeofday(&time, NULL);
    return to_string(time.tv_sec) + "." + to_string(time.tv_usec).substr(0, 2);
}

int InitServer(int port) {
    int socketFd;
    struct sockaddr_in serverAddr;
    
    // Create socket
    socketFd = socket(AF_INET, SOCK_STREAM, 0);
    if (socketFd == -1) {
		cout << "Could not create socket" << endl;
        return -1;
	}

    // Prepare the sockaddr_in structure
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serverAddr.sin_port = htons(port);

    // Bind, using bind from socket.h instead of from std namespace
	if(::bind(socketFd, (struct sockaddr *)&serverAddr, sizeof(serverAddr))) {
		perror("bind failed");
		return -1;
	}

    // Listen
	if(listen(socketFd, MAX_BACKLOG) < 0) {
        perror("listen failed");
		return -1;
    }

    return socketFd;
}

int main(int argc, char** argv) {
    if (argc != 2) {
        cout << "Usage: server <port>\n";
        return -1;
    }

    int port = atoi(argv[1]);
    if (port < 5000 || port > 64000) {
        cout << "Port must be in the range [5000, 64000]\n";
        return -1;
    }
    cout << "Using port " << port << "\n";

    int transactionNum = 1;
    int serverSocketFd = InitServer(port);
    int AddrLen = sizeof(struct sockaddr_in);
    struct sockaddr_in clientAddr;
    // the client message contains the host name and the transaction number
    // 64 bytes should be enough to handle both information
    char clientMessage[64];
    // string timerStart, timerEnd;
    while (true) {
        // accept connection from an incoming client
        cout << "hello" << endl;
	    int clientSocketFd = accept(serverSocketFd, (struct sockaddr *)&clientAddr, (socklen_t*)&AddrLen);
	    if (clientSocketFd < 0) { // bad connection
            perror("bad client connection");
		    continue;
	    }
        // if (timerStart.empty()) {
        //     timerStart = getEpochTime();
        // }
        // the client message contains the host name and the transaction number
        // 64 bytes should be enough to handle both information
        memset(clientMessage, 0, strlen(clientMessage));
        if (recv(clientSocketFd, clientMessage, sizeof(clientMessage), 0) < 0) {
            perror("error occured while server trying to recieve message");
            continue;
        }

        string message = string(clientMessage);
        int spaceIdx = message.find(" ");
        string clientHostname = message.substr(0, spaceIdx);
        int n = stoi(message.substr(spaceIdx + 1));

        cout << getEpochTime() << ": # " << transactionNum << " (T " << n << ") from " << clientHostname << "\n";
        Trans(n);
        cout << getEpochTime() << ": # " << transactionNum << " (Done) from " << clientHostname << "\n";

        //Send the message back to client
        const char* reply = (string("D ") + to_string(transactionNum)).c_str();
		if (send(clientSocketFd, reply, strlen(reply), 0) < 0) {
            perror("server fail to send message");
            return -1;
        }
        transactionNum++;
    }
}