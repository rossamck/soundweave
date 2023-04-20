#include "holepunch.h"







UDPSocket::UDPSocket(const std::string& hostname, int port) {
    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    // std::cout << "Socket = " << sockfd << std::endl;
    if (sockfd < 0) {
      std::cerr << "Error opening socket" << std::endl;
      exit(1);
    }

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(hostname.c_str());

    if (bind(sockfd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
      std::cerr << "Error binding socket" << std::endl;
      exit(1);
    }
  }

UDPSocket::~UDPSocket() {
    if (sockfd >= 0) {
        std::cout << "The UDP instance has gone out of scope, but the socket is still open" << std::endl;
    } else {
        std::cout << "The UDP instance has gone out of scope, and the socket is closed" << std::endl;
    }
}


bool UDPSocket::send(const std::string& message, const std::string& hostname, int port) {
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(hostname.c_str());

    int num_bytes = sendto(sockfd, message.c_str(), message.size(), 0, (struct sockaddr*)&addr, sizeof(addr));
    if (num_bytes < 0) {
      std::cerr << "Error sending message" << std::endl;
      return false;
    }

    return true;
  }

bool UDPSocket::receive(std::string& message, std::string& sender_ip, int& sender_port, const std::string& listen_ip) {
    // Set up the listen address
    sockaddr_in listen_addr;
    std::memset(&listen_addr, 0, sizeof(listen_addr));
    listen_addr.sin_family = AF_INET;
    listen_addr.sin_addr.s_addr = inet_addr(listen_ip.c_str());
    listen_addr.sin_port = htons(port_);

    // Set up the sender address
    sockaddr_in sender_addr;
    socklen_t sender_addr_len = sizeof(sender_addr);
    std::memset(&sender_addr, 0, sizeof(sender_addr));

    // Receive a message from the sender
    char buffer[BUFFER_SIZE];
    int buffer_len = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (sockaddr*)&sender_addr, &sender_addr_len);
    if (buffer_len < 0) {
        std::cerr << "Failed to receive message" << std::endl;
        return false;
    }

    // Extract the IP address and port number of the sender
    sender_ip = inet_ntoa(sender_addr.sin_addr);
    sender_port = ntohs(sender_addr.sin_port);
    message = std::string(buffer, buffer_len);

    return true;
}

int UDPSocket::getBoundPort() {
    // Set up the socket address
    sockaddr_in socket_addr;
    socklen_t addrlen = sizeof(socket_addr);
    std::memset(&socket_addr, 0, sizeof(socket_addr));

    if (getsockname(sockfd, (sockaddr*)&socket_addr, &addrlen) < 0) {
        // Handle error
        return -1;
    }

    // Return the bound port number
    return ntohs(socket_addr.sin_port);
}

int UDPSocket::getSock()
  {
    return sockfd;
  }


void UDPSocket::close() {
      if (sockfd >= 0) {
        ::close(sockfd);
        sockfd = -1;
    }
}

bool checkSender(std::string received_ip, std::string target_ip) {

    if (received_ip != target_ip)
    {
        std::cout << "No match" << std::endl;
        return false;
    }
    std::cout << "Addresses match" << std::endl;
    return true;
}

ipInformation connectToClient()
{
    std::string rendezvousIP;
    int rendezvousPort;

    rendezvousIP = "64.226.97.53";
    rendezvousPort = 12345;

    UDPSocket local_socket("0.0.0.0", 0);
    local_socket.send("Requesting address of other client", rendezvousIP, rendezvousPort);



    std::cout << "Connecting to: " << rendezvousIP << std::endl;
    std::cout << "Socket = " << local_socket.getSock() << std::endl;

    
    // std::string message;

    std::string message;
    std::string sender_ip;
    int sender_port;
    std::string listen_ip = "0.0.0.0";  // Listen for messages from any IP address

    //add section to make sure only listening for messages from server


    std::string ip_message;
    local_socket.receive(ip_message, sender_ip, sender_port, listen_ip);
    std::cout << "Received message from " << sender_ip << ":" << sender_port << ": " << ip_message << std::endl;
  
    while (!checkSender(sender_ip, rendezvousIP))
    {
        std::cout << "Incorrect sender" << std::endl;
        local_socket.receive(ip_message, sender_ip, sender_port, listen_ip);
    }

    // Extract the IP address and port number of the other client from the message
    char ip[16];
    int port;
    std::sscanf(ip_message.c_str(), "%[^:]:%d", ip, &port);
    std::cout << "Client IP: " << ip << " Client port: " << port << std::endl;

    

    ipInformation otherClient; 
    strcpy(otherClient.ip, ip);
    otherClient.port = port;
    otherClient.own_port = local_socket.getBoundPort();
    otherClient.sock = local_socket.getSock();

    otherClient.print();

    
    //This next section should halt until the server instructs clients to connect to each other
    std::cout << "Debug test: ";
    // local_socket.receive(message);
    local_socket.receive(message, sender_ip, sender_port, listen_ip);
    // std::cout << "Received message from " << sender_ip << ":" << sender_port << ": " << message << std::endl;
    std::cout << message << std::endl;
    std::cout << std::endl;







    // Send a message to the other client and wait for a response
    std::string conn_message = "Connection test";
    local_socket.send(conn_message, ip, port);




    // sendMessage(sock, otherClientAddr, message, std::strlen(message));
    // Wait for a response from the other client
    char buffer[BUFFER_SIZE];
    int bufferLen;
    sockaddr_in senderAddr;
    // receiveMessage(3, senderAddr, buffer, bufferLen);
    local_socket.receive(message, sender_ip, sender_port, listen_ip);
    std::cout << "Received message from " << sender_ip << ":" << sender_port << ": " << message << std::endl;

    //Print original message for debugging
   
    while (message != conn_message)
    {
        std::cout << "Connection not successful, retrying" << std::endl;
        local_socket.receive(message, sender_ip, sender_port, listen_ip);

    }
    std::cout << "Connection success" << std::endl;

    std::cout << "Original message: " << conn_message << std::endl;
    std::cout << "Received message from: " << sender_ip << ":" << sender_port << ": " << message << std::endl;

    return otherClient;
}