#include <arpa/inet.h>
#include <cstring>
#include <iostream>
#include <netinet/in.h>
#include <sys/socket.h>
#include <unistd.h>

const int BUFFER_SIZE = 1024;
const int PORT = 12345;

struct ClientInfo
{
    sockaddr_in addr;
};




// Function prototypes
void runServer();
void establishConnection(int sock, ClientInfo& client1, ClientInfo& client2);
void sendMessage(int sock, sockaddr_in addr, const char* message, int messageLen);
void receiveMessage(int sock, sockaddr_in& addr, char* buffer, int& bufferLen);

int main()
{
    while (true)
    {
        runServer();
    }
    return 0;
}




void receiveMessage(int sock, sockaddr_in& senderAddr, char* buffer, int& bufferLen)
{
    socklen_t senderAddrLen = sizeof(senderAddr);
    bufferLen = recvfrom(sock, buffer, BUFFER_SIZE, 0, (sockaddr*)&senderAddr, &senderAddrLen);
}

void sendMessage(int sock, sockaddr_in recipientAddr, const char* buffer, int bufferLen)
{
    sendto(sock, buffer, bufferLen, 0, (sockaddr*)&recipientAddr, sizeof(recipientAddr));
}

void establishConnection(int sock, ClientInfo& client1, ClientInfo& client2)
{
    // Send the other client's address and port number to each client
    char buffer[BUFFER_SIZE];
    int bufferLen;
    sockaddr_in* otherClientAddr;

    // Send client 2's address to client 1
    std::cout << "Sending client 2's address to client 1" << std::endl;
    otherClientAddr = &client2.addr;
    bufferLen = std::sprintf(buffer, "%s:%d", inet_ntoa(otherClientAddr->sin_addr), ntohs(otherClientAddr->sin_port));
    sendMessage(sock, client1.addr, buffer, bufferLen);

    // Send client 1's address to client 2
    std::cout << "Sending client 1's address to client 2" << std::endl;
    otherClientAddr = &client1.addr;
    bufferLen = std::sprintf(buffer, "%s:%d", inet_ntoa(otherClientAddr->sin_addr), ntohs(otherClientAddr->sin_port));
    sendMessage(sock, client2.addr, buffer, bufferLen);

    // Send a message to each client, asking them to send a message to the other client
    std::cout << "Instructing clients to connect..." << std::endl;
    const char* message = "INITIATE CONNECTION";
    sendMessage(sock, client1.addr, message, std::strlen(message));
    sendMessage(sock, client2.addr, message, std::strlen(message));
}




void runServer()
{
    // Create a socket
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0)
    {
        std::cerr << "Failed to create socket" << std::endl;
        return;
    }

    // Set up the server address
    sockaddr_in serverAddr;
    std::memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddr.sin_port = htons(PORT);

    // Bind the socket to the server address
    if (bind(sock, (sockaddr*)&serverAddr, sizeof(serverAddr)) < 0)
    {
        std::cerr << "Failed to bind socket to address" << std::endl;
        close(sock);
        return;
    }

    // Set up the client info structs
    ClientInfo client1;
    ClientInfo client2;

    // Wait for initial messages from both clients
    char buffer[BUFFER_SIZE];
    int bufferLen;
    std::memset(&client1.addr, 0, sizeof(client1.addr));
    std::memset(&client2.addr, 0, sizeof(client2.addr));
    std::cout << "Waiting for initial messages from clients..." << std::endl;
    while (true)
    {
        // Receive a message from a client
        sockaddr_in senderAddr;
        receiveMessage(sock, senderAddr, buffer, bufferLen);
        std::cout << "Received message from client: " << buffer << std::endl;

        // Store the client's address
        if (std::memcmp(&client1.addr, &senderAddr, sizeof(senderAddr)) == 0)
        {
            std::cout << "Received message from client 1" << std::endl;
            client1.addr = senderAddr;
        }
        else if (std::memcmp(&client2.addr, &senderAddr, sizeof(senderAddr)) == 0)
        {
            std::cout << "Received message from client 2" << std::endl;
            client2.addr = senderAddr;
        }
        else if (client1.addr.sin_port == 0)
        {
           std::cout << "Received initial message from client 1" << std::endl;
        client1.addr = senderAddr;
    }
    else
    {
        std::cout << "Received initial message from client 2" << std::endl;
        client2.addr = senderAddr;
    }

    // If both client addresses have been received, establish the connection
    if (client1.addr.sin_port != 0 && client2.addr.sin_port != 0)
    {
        establishConnection(sock, client1, client2);
        break;
    }
}

    // Send a confirmation message to both clients
    // std::cout << "Sending confirmation message to both clients" << std::endl;
    // const char* confirmationMessage = "Connection established";
    // sendMessage(sock, client1.addr, confirmationMessage, std::strlen(confirmationMessage));
    // sendMessage(sock, client2.addr, confirmationMessage, std::strlen(confirmationMessage));


    

    // Close the socket
    close(sock);
    }





