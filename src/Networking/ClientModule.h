// ClientModule.h
#ifndef CLIENT_MODULE_H
#define CLIENT_MODULE_H

#include <iostream>
#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <vector>
#include <string>

class ClientModule {
public:
    ClientModule();
    void start();
    void stop();
    void join();
    boost::asio::ip::udp::socket& get_socket(); // Add this line

private:
    boost::thread module_thread_;
    bool running_;
    boost::asio::ip::udp::socket* socket_ptr_; // Add this line

};

#endif // CLIENT_MODULE_H
