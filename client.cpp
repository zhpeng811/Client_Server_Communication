#include <iostream>
#include <fstream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/time.h> 
#include <string>

#include "tands.h"
#include "util.h"

using namespace std;

/**
 * some of the client protocol code are based on the code
 * from the following website provided by professor Schaeffer in the class slides:
 * binarytides.com/server-client-example-c-sockets-linux
 */ 
class Client {
public:
    Client(int port, char* ipAddress);
    void cleanup();
    void closeSocket();
    void initLogFile();
    bool createAndConnect();
    bool sendMessage(int n);
    bool getResponse();
    void parseInput();

private:
    int port; // the port number provided by user
    int numTransactions = 0; // the number of transactions client sent

    // the client socket of the current transaction
    // each transaction will have a new socket
    int clientSocketFd;

    char* ipAddress; // the ip address provided by the user
    string fileName; // filename of the output log 
    ofstream outputFile; // output file stream of the log file
};

/**
 * Constructor, setting the port and ip address
 * @param port the port to send requests
 * @param ipAddress the ip address to send requests
 */
Client::Client(int port, char* ipAddress) {
    this->port = port;
    this->ipAddress = ipAddress;
}

/**
 * cleanup function before exiting
 */
void Client::cleanup() {
    closeSocket();
    outputFile.close();
}

/**
 * close the current opened socket
 */
void Client::closeSocket() {
    close(clientSocketFd);
    clientSocketFd = -1;
}

/**
 * initialize the log file by getting the hostname and open the file
 */
void Client::initLogFile() {
    // assuming the host name is at most 64 bytes
    char buffer[64];
    if (gethostname(buffer, sizeof(buffer)) < 0) {
        perror("Could not retrieve hostname");
        cleanup();
        exit(EXIT_FAILURE);
    }

    string hostName(buffer); // convert char[] to string
    fileName = hostName + "." + to_string(getpid());

    outputFile.open(fileName);
    outputFile << "Using port " << port << "\n";
    outputFile << "Using server address " << ipAddress << "\n";
    outputFile << "Host " << fileName << "\n";
}

/**
 * create a client socket and connect it to the address of the server at the specified port
 */
bool Client::createAndConnect() {
    // Create a client socket
    clientSocketFd = socket(AF_INET, SOCK_STREAM, 0);
    if (clientSocketFd < 0) {
        perror("Could not create socket");
        return false;
	}

    struct sockaddr_in server;
    server.sin_addr.s_addr = inet_addr(ipAddress);
	server.sin_family = AF_INET;
	server.sin_port = htons(port);

    if (connect(clientSocketFd, (struct sockaddr *)&server, sizeof(server)) < 0) {
        perror("Could not connect to server");
        return false;
    }

    return true;
}

/**
 * send a T<n> message to the server
 */
bool Client::sendMessage(int n) {
    bool flag = createAndConnect();
    // failed to create socket or could not connect to server
    if (!flag || clientSocketFd < 0) {
        return false;
    }

    outputFile << getEpochTime() + ": Send (T " << n << ")\n";
    const char* message = (fileName + " " + to_string(n)).c_str();
    if (send(clientSocketFd, message, strlen(message), 0) < 0) {
        perror("client fail to send message");
        return false;
    }

    // transaction is increased once the message is sent
    numTransactions++;

    return this->getResponse();
}   

/**
 * get a D<n> response from the server
 */
bool Client::getResponse() {
    // the response is a number that represents the transaction number
    // 16 bytes should be enough to handle the response
    char response[16] = {};
    if (recv(clientSocketFd, response, sizeof(response), 0) < 0) {
        perror("error occured while client trying to recieve message");
        return false;
    }

    outputFile << getEpochTime() + ": Recv (" << response << ")\n";

    // communication finished, close the file descriptor
    closeSocket();
    return true;
}

/**
 * parse user inputs, the input can be from command line or redirected from a file
 * input must be either T<n> or S<n>, where n is an integer
 */
void Client::parseInput() {
    string inputLine = "";
	while (getline(cin, inputLine)) {
		char op = inputLine.at(0);
		try {
			int n = stoi(inputLine.substr(1));
			if (op == 'T') {
                if (!this->sendMessage(n)) {
                    cout << "error occured during transaction\n";
                    outputFile << "Sent " << numTransactions << " transactions\n";
                    return;
                }
			} else if (op == 'S') {
                outputFile << "Sleeping " << n << " Units\n";
                Sleep(n);
			} else {
				cout << "invalid input\n";
			}
		} catch (...) {
			cout << "invalid input\n";
		}
	}
    outputFile << "Sent " << numTransactions << " transactions\n";
}

int main(int argc, char** argv) {
    if (argc != 3) {
        cout << "Usage: client <port> <ip-address>\n";
        return -1;
    }

    int port = atoi(argv[1]);
    char* ipAddress = argv[2];
    if (port < 5000 || port > 64000) {
        cout << "Port must be in the range [5000, 64000]\n";
        return -1;
    }

    Client* client = new Client(port, ipAddress);
    client->initLogFile();
    client->parseInput();

    // after finished all inputs
    client->cleanup();
}