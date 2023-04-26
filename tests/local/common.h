// common.h
#pragma once

#include <iostream>
#include <boost/asio.hpp>
// #include <boost/thread.hpp>
#include <vector>
#include <string>
#include <boost/bind/bind.hpp>
#include <functional>
#include <fstream>



using boost::asio::ip::udp;
using namespace boost::placeholders;

void audio_data_callback(const std::vector<short> &audio_data, const udp::endpoint &server_endpoint, udp::socket &socket);
void received_audio_data_callback(const std::vector<short> &audio_data);
