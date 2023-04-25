#include <iostream>
#include <boost/asio.hpp>
#include <vector>
#include <sstream>
#include <map>
#include <boost/asio/ip/tcp.hpp>
#include <thread>
#include <unordered_map>
#include <boost/bind/bind.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/shared_ptr.hpp>

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
    std::unordered_map<int, std::vector<ClientInfo>> sessions_;
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
            if (client.public_ip == new_client.public_ip)
            {
                client_info.target_ip = client.local_ip;
            }
            else
            {
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
            if (client.public_ip == new_client.public_ip)
            {
                client_info.target_ip = new_client.local_ip;
            }
            else
            {
                client_info.target_ip = new_client.public_ip;
            }
            std::string serialized_data = client_info.serialize();
            udp::endpoint client_endpoint(boost::asio::ip::address::from_string(client.public_ip), client.port);
            socket.send_to(boost::asio::buffer(serialized_data), client_endpoint);
        }
    }
}


class TcpConnection : public boost::enable_shared_from_this<TcpConnection>
{
public:
    typedef boost::shared_ptr<TcpConnection> pointer;

    static pointer create(boost::asio::io_context& io_context)
    {
        return pointer(new TcpConnection(io_context));
    }

    boost::asio::ip::tcp::socket& socket()
    {
        return socket_;
    }

    void start()
    {
        // Do something with the connection, such as reading and writing data asynchronously
         do_read();
    }

private:
    TcpConnection(boost::asio::io_context& io_context)
        : socket_(io_context)
    {
    }

    void do_read()
    {
        boost::asio::async_read_until(socket_, buffer_, '\n',
            boost::bind(&TcpConnection::handle_read, shared_from_this(), boost::asio::placeholders::error, boost::asio::placeholders::bytes_transferred));
    }

void handle_read(const boost::system::error_code& error, std::size_t bytes_transferred)
{
    if (!error)
    {
        std::istream is(&buffer_);
        std::string message;
        std::getline(is, message);
        // Process the received message
        std::cout << "Received Message: " << message << std::endl;
        if (message == "SYN") {
            std::cout << "Received SYN" << std::endl;
            do_write("ACK\n"); // Send ACK when receiving SYN
        } else if (message == "ACK") {
            std::cout << "Received ACK" << std::endl;
        } else {
            do_write(message);
        }
    }
}


void do_write(const std::string& message)
{
    std::cout << "Sending message to client IP: " << socket_.remote_endpoint().address().to_string() 
              << ", port: " << socket_.remote_endpoint().port() << " - " << message << std::endl;
    boost::asio::async_write(socket_, boost::asio::buffer(message),
        boost::bind(&TcpConnection::handle_write, shared_from_this(), boost::asio::placeholders::error));
}


    void handle_write(const boost::system::error_code& error)
    {
        if (!error)
        {
            do_read();
        }
    }

    boost::asio::ip::tcp::socket socket_;
    boost::asio::streambuf buffer_;
};



class TcpServer
{
public:
    TcpServer(boost::asio::io_context& io_context, unsigned short port)
        : io_context_(io_context), acceptor_(io_context, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port))
    {
        start_accept();
    }

private:
    void start_accept()
    {
        TcpConnection::pointer new_connection = TcpConnection::create(io_context_);

        acceptor_.async_accept(new_connection->socket(),
            boost::bind(&TcpServer::handle_accept, this, new_connection, boost::asio::placeholders::error));
    }

void handle_accept(TcpConnection::pointer new_connection, const boost::system::error_code& error)
{
    if (!error)
    {
        std::cout << "Accepted new connection from " << new_connection->socket().remote_endpoint().address().to_string() 
                  << ":" << new_connection->socket().remote_endpoint().port() << std::endl;
        new_connection->start();
    }

    start_accept();
}


    boost::asio::io_context& io_context_;
    boost::asio::ip::tcp::acceptor acceptor_;
};






class Server
{
public:
    constexpr static unsigned short kUdpPort = 13579;

    Server(boost::asio::io_context &io_context)
        : io_context_(io_context), socket_(io_context, udp::endpoint(udp::v4(), kUdpPort)), session_manager_(), client_id_(1)
    {
        session_manager_.create_session();
    }

    void run()
    {
        std::cout << "Server initialized, waiting for clients." << std::endl;
        handle_udp_connections();
    }

private:
    void handle_udp_connections()
    {
        while (true)
        {
            char data[1024];
            udp::endpoint sender_endpoint;
            size_t len = socket_.receive_from(boost::asio::buffer(data), sender_endpoint);

            std::string_view received_data(data, len);

            if (received_data.substr(0, 7) == "connect")
            {
                std::cout << "Received new connection request" << std::endl;
                handle_connect_request(sender_endpoint, received_data);
            }
            else
            {
                std::cout << "Received from client: " << received_data << std::endl;
            }
        }
    }

    void handle_connect_request(udp::endpoint &sender_endpoint, std::string_view &received_data)

    {
        int session_id = 1; // Assuming there is only one session

        std::cout << "Received connection request from client: " << received_data << std::endl;
        std::string local_ip = std::string(received_data.substr(8)); // Extract the local IP from the received_data string

        // Store the client information in a struct
        ClientInfo own_info;

        own_info.id = client_id_;
        own_info.public_ip = sender_endpoint.address().to_string();
        own_info.local_ip = local_ip;
        own_info.target_ip = own_info.public_ip;
        own_info.action = "SELF";
        own_info.port = sender_endpoint.port();
        own_info.print();

        // Send connecting client its own clientinfo struct
        std::string own_serialized_data = "OWN_INFO:" + own_info.serialize();
        std::cout << "Serialized test = " << own_serialized_data << std::endl;

        udp::endpoint client_endpoint(boost::asio::ip::address::from_string(own_info.public_ip), own_info.port);

        socket_.send_to(boost::asio::buffer(own_serialized_data), client_endpoint);

        if (session_manager_.client_ip_exists(session_id, own_info.public_ip))
        {
            std::cout << "Warning: Client with public IP " << own_info.public_ip << " is already in the session." << std::endl;
        }

        std::cout << "Adding client to session ID: " << session_id << std::endl;
        session_manager_.add_client_to_session(session_id, own_info);
        send_client_info_to_new_client(socket_, own_info, session_manager_.get_clients_in_session(session_id));
        send_new_client_info_to_existing_clients(socket_, own_info, session_manager_.get_clients_in_session(session_id));

        print_clients_in_session(session_id, session_manager_);

        client_id_++; // increment client ID
    }



    boost::asio::io_context &io_context_;
    udp::socket socket_;
    SessionManager session_manager_;
    int client_id_;
};

void run_tcp_server(boost::asio::io_context &io_context, unsigned short port)
{
    try
    {
        TcpServer tcp_server(io_context, port);
        io_context.run();
    }
    catch (std::exception &e)
    {
        std::cerr << "Error in TCP server: " << e.what() << std::endl;
    }
}


int main()
{
    constexpr unsigned short kTcpPort = 24680;

    boost::asio::io_context io_context;
    Server server(io_context);

    // Run the TCP server in its own thread
    std::thread tcp_server_thread(run_tcp_server, std::ref(io_context), kTcpPort);

    // Run the UDP server in the main thread
    server.run();

    // Wait for the TCP server thread to finish before exiting the main function
    tcp_server_thread.join();

    return 0;
}

