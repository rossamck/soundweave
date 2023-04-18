#ifndef HEADER_H
#define HEADER_H

#include <iostream>

struct ipInformation {
  char ip[16];
  int port;
  int own_port;
  int sock;

  void print() const {
    std::cout << "ip: " << ip << ", port: " << port << ", own port: " << own_port << std::endl;
  }
};

ipInformation connectToClient();

#endif