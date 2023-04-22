#include <iostream>
#include <boost/asio.hpp>


using boost::asio::ip::udp;

constexpr int MIN_NUM_CLIENTS = 2;

struct ClientInfo {
    int id;
    std::string public_ip;
    std::string local_ip;
    std::string target_ip;
    std::string action;
    unsigned short port;

    ClientInfo() : id(0), public_ip(""), local_ip(""), target_ip(""), action(""), port(0) {}
    ClientInfo(int id, const std::string& public_ip, const std::string& local_ip, unsigned short port)
        : id(id), public_ip(public_ip), local_ip(local_ip), target_ip(public_ip), action(""), port(port) {}

    // Serialize the struct into a string
    std::string serialize() const {
        std::ostringstream oss;
        oss << id << " " << public_ip << " " << local_ip << " " << target_ip << " " << action << " " << port;
        return oss.str();
    }

    // Deserialize the string back into the struct
    static ClientInfo deserialize(const std::string& str) {
        std::istringstream iss(str);
        ClientInfo clientInfo;
        iss >> clientInfo.id >> clientInfo.public_ip >> clientInfo.local_ip >> clientInfo.target_ip >> clientInfo.action >> clientInfo.port;
        return clientInfo;
    }

    void print() const {
        std::cout << "ID: " << id << std::endl;
        std::cout << "Public IP: " << public_ip << std::endl;
        std::cout << "Local IP: " << local_ip << std::endl;
        std::cout << "Target IP: " << target_ip << std::endl;
        std::cout << "Action: " << action << std::endl;
        std::cout << "Port: " << port << std::endl;
    }
};

bool client_ip_exists(const std::vector<ClientInfo> &clients, const std::string &public_ip)
{
    for (const auto &client : clients)
    {
        if (client.public_ip == public_ip)
        {
            return true;
        }
    }
    return false;
}

int main()
{
    try
    {
        std::cout << "new server test" << std::endl;
        boost::asio::io_context io_context;

        udp::socket socket(io_context, udp::endpoint(udp::v4(), 13579));
        std::cout << "Server initialized, waiting for clients." << std::endl;

        std::vector<ClientInfo> clients;
        int client_id = 1;

        while (true)
        {
            char data[1024];
            udp::endpoint sender_endpoint;
            size_t len = socket.receive_from(boost::asio::buffer(data), sender_endpoint);

            std::string received_data(data, len);
            std::cout << "Received data from client: " << received_data << std::endl;

            if (received_data.substr(0, 7) == "connect")
            {

                std::string local_ip = received_data.substr(8); // Extract the local IP from the received_data string

                // Store the client information in a struct
                ClientInfo client_info;

                client_info.id = client_id;
                client_info.public_ip = sender_endpoint.address().to_string();
                client_info.local_ip = local_ip; 
                client_info.target_ip = client_info.public_ip;
                client_info.action = "testaction";
                client_info.port = sender_endpoint.port();
                std::cout << "TEST" << std::endl;
                client_info.print();

                if (client_ip_exists(clients, client_info.public_ip))
                {
                    std::cout << "Warning: Client with public IP " << client_info.public_ip << " is already in the session." << std::endl;
                
                }

                clients.push_back(client_info);
                std::cout << "Client connected: " << client_info.public_ip << ":" << client_info.port << std::endl;

                // Send client ID to the connected client
                std::string id_message = "ID:" + std::to_string(client_id);
                socket.send_to(boost::asio::buffer(id_message), sender_endpoint);
                std::cout << "Sent client ID to the connected client: " << client_id << std::endl;
                client_id++;

                if (clients.size() >= MIN_NUM_CLIENTS)
                {
                    std::string serializedData = client_info.serialize();
                    std::cout << "Serialised test = " << serializedData << std::endl;

                    // Send the new client's info to all existing clients
                    std::string new_client_info = client_info.public_ip + ":" + std::to_string(client_info.port) + ";";
                    for (const auto &client : clients)
                    {
                        
                        udp::endpoint client_endpoint(boost::asio::ip::address::from_string(client.public_ip), client.port);
                        if (client_endpoint != sender_endpoint)
                        {
                            socket.send_to(boost::asio::buffer(serializedData), client_endpoint);
                        }
                    }

                    // Send all existing clients' info to the new client
                    std::string existing_clients_info;
                    for (const auto &client : clients)
                    {
                        if (client.public_ip != client_info.public_ip || client.port != client_info.port)
                        {
                            existing_clients_info += client.public_ip + ":" + std::to_string(client.port) + ";";
                        }
                    }
                    socket.send_to(boost::asio::buffer(existing_clients_info), sender_endpoint);

                    // Send "NEW" prefix to all existing clients when a new client joins
                    if (clients.size() >= 3)
                    {
                        std::string new_client_message = "NEW:" + new_client_info;
                        for (const auto &client : clients)
                        {
                            udp::endpoint client_endpoint(boost::asio::ip::address::from_string(client.public_ip), client.port);
                            if (client_endpoint != sender_endpoint)
                            {
                                socket.send_to(boost::asio::buffer(new_client_message), client_endpoint);
                            }
                        }
                    }
                }
            }
        }
    }
    catch (std::exception &e)
    {
        std::cerr << e.what() << std::endl;
    }
}