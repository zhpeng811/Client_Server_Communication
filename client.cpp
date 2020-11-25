#include <iostream>
#include <fstream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/time.h> 
#include <string>
#include "tands.h"

using namespace std;

string getEpochTime() {
    timeval time;
	gettimeofday(&time, NULL);
    return to_string(time.tv_sec) + "." + to_string(time.tv_usec).substr(0, 2);
}

int connectServer(int port, char* ipAddress) {
    // Create socket
    int socketFd = socket(AF_INET, SOCK_STREAM, 0);
    if (socketFd < 0) {
        perror("Could not create socket");
        return -1;
	}

    struct sockaddr_in server;

    server.sin_addr.s_addr = inet_addr(ipAddress);
	server.sin_family = AF_INET;
	server.sin_port = htons(port);

    if (connect(socketFd, (struct sockaddr *)&server, sizeof(server)) < 0) {
        perror("Could not connect to server");
        return -1;
    }

    return socketFd;
}

int parseInput(string fileName, ofstream& outputFile, int port, char* ipAddress) {
    string inputLine = "";
    int numTransactions = 0;
    // the response is a number that represents the transaction number
    // 16 bytes should be enough to handle the response
    char response[16];
	while (getline(cin, inputLine)) {
		char op = inputLine.at(0);
		try {
			int n = stoi(inputLine.substr(1));
			if (op == 'T') {
                int socketFd = connectServer(port, ipAddress);

                outputFile << getEpochTime() + ": Send (T " << n << ")\n";
                const char* message = (fileName + " " + to_string(n)).c_str();
                if (send(socketFd, message, strlen(message), 0) < 0) {
                    perror("client fail to send message");
                    return -1;
                }
                numTransactions++;

                memset(response, 0, strlen(response));
                if (recv(socketFd, response, sizeof(response), 0) < 0) {
                    perror("error occured while client trying to recieve message");
                    return -1;
                }

                outputFile << getEpochTime() + ": Recv (" << response << ")\n";
                close(socketFd);

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

    return 0;
}

string getClientLogName() {
    char buffer[30];
    if (gethostname(buffer, sizeof(buffer)) < 0) {
        perror("Could not retrieve hostname");
        return "";
    }
    string machineName(buffer); // convert char[] to string
    string fileName = machineName + "." + to_string(getpid());

    return fileName;
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

    ofstream outputFile;
    string fileName = getClientLogName();
    if (fileName == "") return -1;
    outputFile.open(fileName);
    outputFile << "Using port " << port << "\n";
    outputFile << "Using server address " << ipAddress << "\n";
    outputFile << "Host " << fileName << "\n";

    if (parseInput(fileName, outputFile, port, ipAddress) < 0) {
        cout << "error occured during transaction\n";
    }
    outputFile.close();
}