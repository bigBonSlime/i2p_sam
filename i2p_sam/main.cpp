#include "boost/asio.hpp"
#include "i2p_sam.h"
#include <iostream>
#include <string>

int counter = 0;

void readdata(std::shared_ptr<i2p_sam::datagram_session> sock) {
    sock->async_read_datagram(
        [sock]([[maybe_unused]] std::string dest, [[maybe_unused]] std::size_t size,
               [[maybe_unused]] uint16_t from_port, [[maybe_unused]] uint16_t to_port,
               std::shared_ptr<std::byte[]> data, i2p_sam::errors::sam_error ec) {
            if (!ec) {
                std::string s((char *)data.get(), size);
                std::cout << s << " " << counter << '\n';
                counter += 1;
                readdata(sock);
            } else {

                std::cout << ec.what();
            }
        });
}

int main() {
    boost::asio::io_context io;
    std::string dest = "target_destination";
    i2p_sam::datagram_session::async_create_datagram_session(
        io, false, "ID", dest, "", "",
        [=](std::shared_ptr<i2p_sam::datagram_session> s, i2p_sam::errors::sam_error ec) {
            std::cout << "Session create " << ec.what() << '\n';
            readdata(s);
        });

    io.run();
}
