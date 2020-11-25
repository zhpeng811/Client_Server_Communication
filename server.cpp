#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/time.h> 
#include <string>
#include <unistd.h>
#include <fcntl.h>
#include <unordered_map>
#include "tands.h"

using namespace std;

#define MAX_BACKLOG 50
#define TIMEOUT_SEC 30

string getEpochTime() {
    timeval time;
	gettimeofday(&time, NULL);
    return to_string(time.tv_sec) + "." + to_string(time.tv_usec).substr(0, 2);
}

/**
 * 
 * @param port the port number to listen to
 */
int initServer(int port) {
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

    // listen with a maximum length of queue
	if(listen(socketFd, MAX_BACKLOG) < 0) {
        perror("listen failed");
		return -1;
    }

    return socketFd;
}

int handleClientMessage(int clientSocketFd, int transactionNum, unordered_map<string, int>& transactions) {
    // the client message contains the host name and the transaction number
    // 64 bytes should be enough to handle both information
    char clientMessage[64] = {};
    if (recv(clientSocketFd, clientMessage, sizeof(clientMessage), 0) < 0) {
        perror("error occured while server trying to recieve message");
        return -1;
    }

    string message = string(clientMessage);
    int spaceIdx = message.find(" ");
    string clientHostname = message.substr(0, spaceIdx);
    int n = stoi(message.substr(spaceIdx + 1));

    cout << getEpochTime() << ": # " << transactionNum << " (T " << n << ") from " << clientHostname << "\n";
    Trans(n);
    cout << getEpochTime() << ": # " << transactionNum << " (Done) from " << clientHostname << "\n";
    
    if (transactions.find(clientHostname) == transactions.end()) {
        transactions[clientHostname] = 1;
    } else {
        transactions[clientHostname] += 1;
    }

    // reply client
    const char* reply = (string("D ") + to_string(transactionNum)).c_str();
	if (send(clientSocketFd, reply, strlen(reply), 0) < 0) {
        perror("server fail to send message");
        return -1;
    }
    return 0;
}

void printSummary(unordered_map<string, int>& transactions) {
    cout << "SUMMARY\n";
    for (auto iter = transactions.begin(); iter != transactions.end(); iter++) {
        cout << "  " << iter->second << " transactions from " << iter->first << "\n";
    }
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
    int serverSocketFd = initServer(port);
    int AddrLen = sizeof(struct sockaddr_in);
    struct sockaddr_in clientAddr;

    long timerStart = stol(getEpochTime());

    // idea obtained from https://stackoverflow.com/a/12611162
    fd_set fdset;
    FD_ZERO(&fdset); // file descriptor is set to zero bits for all file descriptors
    FD_SET(serverSocketFd, &fdset); // set the bit for the server socket file descriptor

    // int opt = 3;
    // setsockopt(serverSocketFd, SOL_SOCKET, SO_RCVLOWAT,&opt,sizeof(opt));

    timeval timeout;
    timeout.tv_sec = TIMEOUT_SEC;
    timeout.tv_usec = 0;

    unordered_map<string, int> transactions;

    while (true) {
        int selectStatus = select(serverSocketFd + 1, &fdset, NULL, NULL, &timeout);
        if (selectStatus == 0) { // timeout
            printSummary(transactions);
            break;
        } else if (selectStatus == -1) { // error
            perror("select failed");
            return -1;
        }

        // accept connection from an incoming client
	    int clientSocketFd = accept(serverSocketFd, (struct sockaddr *)&clientAddr, (socklen_t*)&AddrLen);
	    if (clientSocketFd < 0) { // bad connection
            perror("accept failed");
            return -1;
	    }
        
        if (handleClientMessage(clientSocketFd, transactionNum, transactions) < 0) {
            return -1;
        }
        transactionNum++;
    }
}