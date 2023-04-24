#ifndef CLIENT_INFO_H
#define CLIENT_INFO_H

#include <iostream>
#include <sstream>
#include <string>

struct ClientInfo {
  int id;
  std::string public_ip;
  std::string local_ip;
  std::string target_ip;
  std::string action;
  unsigned short port;
  unsigned short udp_port; // Added udp_port member

  ClientInfo()
      : id(0), public_ip(""), local_ip(""), target_ip(""), action(""), port(0), udp_port(0) {}

  ClientInfo(int id, const std::string &public_ip, const std::string &local_ip,
             unsigned short port, unsigned short udp_port)
      : id(id),
        public_ip(public_ip),
        local_ip(local_ip),
        target_ip(public_ip),
        action(""),
        port(port),
        udp_port(udp_port) {} // Added udp_port parameter

  std::string serialize() const {
    std::ostringstream oss;
    oss << id << " " << public_ip << " " << local_ip << " " << target_ip << " "
        << action << " " << port << " " << udp_port; // Added udp_port serialization
    return oss.str();
  }

  static ClientInfo deserialize(const std::string &str) {
    std::istringstream iss(str);
    ClientInfo clientInfo;
    iss >> clientInfo.id >> clientInfo.public_ip >> clientInfo.local_ip >>
        clientInfo.target_ip >> clientInfo.action >> clientInfo.port >> clientInfo.udp_port; // Added udp_port deserialization
    return clientInfo;
  }

  void print() const {
    std::cout << "ID: " << id << std::endl;
    std::cout << "Public IP: " << public_ip << std::endl;
    std::cout << "Local IP: " << local_ip << std::endl;
    std::cout << "Target IP: " << target_ip << std::endl;
    std::cout << "Action: " << action << std::endl;
    std::cout << "Port: " << port << std::endl;
    std::cout << "UDP Port: " << udp_port << std::endl; // Added udp_port print
  }
};

#endif  // CLIENT_INFO_H
