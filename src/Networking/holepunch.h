// holepunch.h
#ifndef HOLEPUNCH_H
#define HOLEPUNCH_H

#include <string>
#include <netinet/in.h>
#include <iostream>
#include <string>
#include <cstring>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

const int BUFFER_SIZE = 1024;

struct ClientInfo {
    sockaddr_in addr;
};

struct ipInformation {
    char ip[16];
    int port;
    int own_port;
    int sock;

    void print() {
        std::cout << "Client IP: " << ip << " Client Port: " << port << " Own Port: " << own_port << std::endl;
    }
};

class UDPSocket {
public:
    UDPSocket(const std::string& hostname, int port);

    ~UDPSocket();

    bool send(const std::string& message, const std::string& hostname, int port);

    bool receive(std::string& message, std::string& sender_ip, int& sender_port, const std::string& listen_ip);

    int getBoundPort();

    int getSock();

    void close();

private:
    int sockfd;
    int port_;
};

bool checkSender(std::string received_ip, std::string target_ip);

ipInformation connectToClient();

#endif // HOLEPUNCH_H
