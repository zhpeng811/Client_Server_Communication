/**
 * server.cpp
 * Ze Hui Peng(zhpeng)
 * Fall 2020 CMPUT 379 Assignment 3
 */

#include <iostream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/time.h> 
#include <string>
#include <unistd.h>
#include <fcntl.h>
#include <unordered_map>
#include <iomanip> // setprecision()
#include <string.h> // strlen()

#include "tands.h"
#include "util.h"

using namespace std;

#define MAX_BACKLOG 50
#define TIMEOUT_SEC 30

/**
 * some of the server protocol code are based on the code
 * from the following website provided by professor Schaeffer in the class slides:
 * binarytides.com/server-client-example-c-sockets-linux
 */ 
class Server {
public:
    Server(int port);
    void cleanup();
    bool waitForRequest();
    int acceptRequest();
    void handleClientMessage(int clientSocketFd);
    void printSummary();

private:
    int transactionNum = 1;
    int serverSocketfd;
    fd_set fdset;
    double transactionStartTime = 0.0;
    double transactionEndTime = 0.0;
    unordered_map<string, int> transactions;
};

/**
 * initialize the server by creating the socket, bind it with the specific port
 * and listen for incoming requests
 * @param port the port number to listen to
 */
Server::Server(int port) {
    struct sockaddr_in serverAddr;
    
    // Create socket
    serverSocketfd = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocketfd == -1) {
		perror("create socket failed");
        exit(EXIT_FAILURE);
	}

    // Prepare the sockaddr_in structure
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serverAddr.sin_port = htons(port);

    // Bind, using bind from socket.h instead of from std namespace
	if(::bind(serverSocketfd, (struct sockaddr *)&serverAddr, sizeof(serverAddr))) {
		perror("bind failed");
        cleanup();
        exit(EXIT_FAILURE);
	}

    // listen with a maximum length of queue
	if(listen(serverSocketfd, MAX_BACKLOG) < 0) {
        perror("listen failed");
        cleanup();
		exit(EXIT_FAILURE);
    }

    // idea obtained from https://stackoverflow.com/a/12611162
    FD_ZERO(&fdset); // file descriptor is set to zero bits for all file descriptors
    FD_SET(serverSocketfd, &fdset); // set the bit for the server socket file descriptor
}

/**
 * cleanup by closing the socket
 */
void Server::cleanup() {
    close(serverSocketfd);
}

/**
 * wait for client requests
 * @return true if a client request arrives
 *         false if select() failed or timed out
 */
bool Server::waitForRequest() {
    // For Linux, the timeout have to be reinitialized for every select() call
    // From the select() man page:
    /*
     * On Linux, select() modifies 'timeout' to reflect the amount of time not slept...
     * Consider timeout to be undefined after select() returns
     */
    timeval timeout;
    timeout.tv_sec = TIMEOUT_SEC; // set the timeout to TIMEOUT_SEC(i.e. 30 seconds)
    timeout.tv_usec = 0;

    // select will block until a request arrived or times out
    int selectStatus = select(serverSocketfd + 1, &fdset, NULL, NULL, &timeout);
    if (selectStatus == 0) { // timeout
        return false;
    } else if (selectStatus == -1) { // error
        perror("select failed");
        return false;
    } else {
        return true;
    }
}

/**
 * accept the request from client
 * @return the client socket file descriptor
 *         or -1 if accept() failed
 */
int Server::acceptRequest() {
    struct sockaddr_in clientAddr;
    int AddrLen = sizeof(struct sockaddr_in);
    // accept connection from an incoming client
	int clientSocketFd = accept(serverSocketfd, (struct sockaddr *)&clientAddr, (socklen_t*)&AddrLen);
	if (clientSocketFd < 0) { // bad connection
        perror("accept failed");
	}
    return clientSocketFd;
}

/**
 * handle client message by receive the message and
 * send the current transaction number back to the client
 * @param clientSocketFd the file descriptor of the client socket
 */
void Server::handleClientMessage(int clientSocketFd) {
    // the client message contains the host name and the transaction number
    // 64 bytes should be enough to handle both information
    char clientMessage[64] = {};
    if (recv(clientSocketFd, clientMessage, sizeof(clientMessage), 0) < 0) {
        perror("error occured while server trying to recieve message");
        return;
    }

    string message = string(clientMessage);
    int spaceIdx = message.find(" ");
    string clientHostname = message.substr(0, spaceIdx);
    int n = stoi(message.substr(spaceIdx + 1));

    string startTime = getEpochTime();
    if (this->transactionStartTime == 0.0) {
        // first transaction, record the time
        transactionStartTime = stod(startTime);
    }

    cout << startTime << ": # " << transactionNum << " (T " << n << ") from " << clientHostname << "\n";

    // transaction simulation
    Trans(n);

    string endTime = getEpochTime();
    this->transactionEndTime = stod(endTime);
    cout << endTime << ": # " << transactionNum << " (Done) from " << clientHostname << "\n";
    
    if (transactions.find(clientHostname) == transactions.end()) {
        transactions[clientHostname] = 1;
    } else {
        transactions[clientHostname] += 1;
    }

    // reply client
    const char* reply = (string("D ") + to_string(transactionNum)).c_str();
	if (send(clientSocketFd, reply, strlen(reply), 0) < 0) {
        perror("server fail to send message");
        return;
    }

    // successful transaction
    this->transactionNum++;
}

/**
 * print the summary of the server, including the number of transactions received from each client
 * and the total transactions/second
 */
void Server::printSummary() {
    cout << "SUMMARY\n";
    for (auto iter = transactions.begin(); iter != transactions.end(); iter++) {
        cout << "  " << iter->second << " transactions from " << iter->first << "\n";
    }
    int totalTransactions = transactionNum - 1; // -1 since transaction num starts from 1
    double totalTranactionTime = transactionEndTime - transactionStartTime;
    cout << "  " << setprecision(2) << fixed << totalTransactions / totalTranactionTime << 
            " transactions/sec  (" << totalTransactions << "/" << totalTranactionTime << ")\n";
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

    Server* server = new Server(port);

    while (true) {
        bool flag = server->waitForRequest();
        if (!flag) { // server timeout or select() failed
            server->printSummary();
            break;
        }

        // accept connection from an incoming client
	    int clientSocketFd = server->acceptRequest();
	    if (clientSocketFd < 0) { // bad connection
            perror("accept failed, discarding this request");
            continue;
	    }
        
        server->handleClientMessage(clientSocketFd);
    }

    delete server;
}