#include <iostream>
#include <boost/asio.hpp>
#include <vector>
#include <sstream>
#include <map>

using boost::asio::ip::udp;

struct ClientInfo
{
    int id;
    std::string public_ip;
    std::string local_ip;
    std::string target_ip;
    std::string action;
    unsigned short port;

    ClientInfo() : id(0), public_ip(""), local_ip(""), target_ip(""), action(""), port(0) {}
    ClientInfo(int id, const std::string &public_ip, const std::string &local_ip, unsigned short port)
        : id(id), public_ip(public_ip), local_ip(local_ip), target_ip(public_ip), action(""), port(port) {}

    // Serialize the struct into a string
    std::string serialize() const
    {
        std::ostringstream oss;
        oss << id << " " << public_ip << " " << local_ip << " " << target_ip << " " << action << " " << port;
        return oss.str();
    }

    // Deserialize the string back into the struct
    static ClientInfo deserialize(const std::string &str)
    {
        std::istringstream iss(str);
        ClientInfo clientInfo;
        iss >> clientInfo.id >> clientInfo.public_ip >> clientInfo.local_ip >> clientInfo.target_ip >> clientInfo.action >> clientInfo.port;
        return clientInfo;
    }

    void print() const
    {
        std::cout << "ID: " << id << std::endl;
        std::cout << "Public IP: " << public_ip << std::endl;
        std::cout << "Local IP: " << local_ip << std::endl;
        std::cout << "Target IP: " << target_ip << std::endl;
        std::cout << "Action: " << action << std::endl;
        std::cout << "Port: " << port << std::endl;
    }
};

class SessionManager
{
public:
    SessionManager() : next_session_id_(1) {}

    int create_session()
    {
        int session_id = next_session_id_++;
        sessions_[session_id] = std::vector<ClientInfo>();
        return session_id;
    }

    void add_client_to_session(int session_id, const ClientInfo &client_info)
    {
        sessions_[session_id].push_back(client_info);
    }

    std::vector<ClientInfo> get_clients_in_session(int session_id) const
    {
        return sessions_.at(session_id);
    }

    bool client_ip_exists(int session_id, const std::string &public_ip)
    {
        for (const auto &client : sessions_[session_id])
        {
            if (client.public_ip == public_ip)
            {
                return true;
            }
        }
        return false;
    }

private:
    int next_session_id_;
    std::map<int, std::vector<ClientInfo>> sessions_;
};

void print_clients_in_session(int session_id, const SessionManager &session_manager)
{
    std::vector<ClientInfo> clients = session_manager.get_clients_in_session(session_id);
    std::cout << "\nClients in session " << session_id << ":" << std::endl;
    for (const auto &client : clients)
    {
        client.print();
    }
}


void send_client_info_to_new_client(udp::socket &socket, const ClientInfo &new_client, const std::vector<ClientInfo> &existing_clients)
{
    for (const auto &client : existing_clients)
    {
        if (client.id != new_client.id)
        {
            ClientInfo client_info = client;
            client_info.action = "CONNECT_PEER";
            if (client.public_ip == new_client.public_ip) {
                client_info.target_ip = client.local_ip;
            } else {
                client_info.target_ip = client.public_ip;
            }
            std::string serialized_data = client_info.serialize();
            udp::endpoint client_endpoint(boost::asio::ip::address::from_string(new_client.public_ip), new_client.port);
            socket.send_to(boost::asio::buffer(serialized_data), client_endpoint);
        }
    }
}


void send_new_client_info_to_existing_clients(udp::socket &socket, const ClientInfo &new_client, const std::vector<ClientInfo> &existing_clients)
{
    for (const auto &client : existing_clients)
    {
        if (client.id != new_client.id)
        {
            ClientInfo client_info = new_client;
            client_info.action = "CONNECT_PEER";
            if (client.public_ip == new_client.public_ip) {
                client_info.target_ip = new_client.local_ip;
            } else {
                client_info.target_ip = new_client.public_ip;
            }
            std::string serialized_data = client_info.serialize();
            udp::endpoint client_endpoint(boost::asio::ip::address::from_string(client.public_ip), client.port);
            socket.send_to(boost::asio::buffer(serialized_data), client_endpoint);
        }
    }
}




int main()
{
    SessionManager session_manager;
    int session_id = session_manager.create_session();

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
        

        if (received_data.substr(0, 7) == "connect")
        {
            std::cout << "Received connection request from client: " << received_data << std::endl;
            std::string local_ip = received_data.substr(8); // Extract the local IP from the received_data string
                                                            // Store the client information in a struct
            ClientInfo own_info;

            own_info.id = client_id;
            own_info.public_ip = sender_endpoint.address().to_string();
            own_info.local_ip = local_ip;
            own_info.target_ip = own_info.public_ip;
            own_info.action = "SELF";
            own_info.port = sender_endpoint.port();
            own_info.print();

            // Send connecting client its own clientinfo struct
            std::string ownSerialisedData = own_info.serialize();
            std::cout << "Serialised test = " << ownSerialisedData << std::endl;

            udp::endpoint client_endpoint(boost::asio::ip::address::from_string(own_info.public_ip), own_info.port);

            socket.send_to(boost::asio::buffer(ownSerialisedData), client_endpoint);

            if (session_manager.client_ip_exists(session_id, own_info.public_ip))
            {
                std::cout << "Warning: Client with public IP " << own_info.public_ip << " is already in the session." << std::endl;
            }
            
            
                std::cout << "Adding client to session ID: " << session_id << std::endl;
                session_manager.add_client_to_session(session_id, own_info);
                send_client_info_to_new_client(socket, own_info, session_manager.get_clients_in_session(session_id));
                send_new_client_info_to_existing_clients(socket, own_info, session_manager.get_clients_in_session(session_id));

                print_clients_in_session(session_id, session_manager);
            

            client_id++; // increment client ID
        }
        else {
            std::cout << "Received from client: " << received_data << std::endl;
        }
    }
}