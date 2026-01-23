#pragma once

#include <boost/asio.hpp>
#include <iostream>
#include <string>

using namespace std;

class UdpSender 
{
public:
    UdpSender(const string& host, unsigned short port)
        : io_context_(),
        socket_(io_context_),
        endpoint_(boost::asio::ip::make_address(host), port)
    {
        socket_.open(boost::asio::ip::udp::v4());
    }

    ~UdpSender() 
    {
        socket_.close();
    }

    void SendString(const string& message) 
    {
        boost::system::error_code ec;
        socket_.send_to(boost::asio::buffer(message), endpoint_, 0, ec);
        if (ec) 
            std::cerr << "UDP sending error: " << ec.message() << std::endl;
    }

private:
    boost::asio::io_context io_context_;
    boost::asio::ip::udp::socket socket_;
    boost::asio::ip::udp::endpoint endpoint_;
};
