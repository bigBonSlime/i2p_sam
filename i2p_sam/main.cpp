#include "boost/asio.hpp"
#include "i2p_sam.h"
#include <iostream>
#include <string>

void readdata(std::shared_ptr<i2p_sam::sam_socket> sock) {
    sock->async_read_line(1000000, [sock](std::string res, i2p_sam::errors::sam_error ec) {
        if (!ec) {
            std::cout << res << std::endl;
            readdata(sock);
        } else {
            std::cout << "Read error: " << ec.what() << std::endl;
        }
    });
}

int main() {
    boost::asio::io_context io;
    std::string dest = "";
    i2p_sam::datagram_session::async_create_datagram_session(
        io, "ID", "TRANSIENT", "", "",
        [=](std::shared_ptr<i2p_sam::datagram_session> s, i2p_sam::errors::sam_error ec) {
            std::string data = "DATA";
            for (int i = 0; i < 1000; i++) {
                s->async_send(dest, data.data(), static_cast<uint16_t>(data.size()), "",
                              [s](i2p_sam::errors::sam_error ec) { std::cout << ec.what() << '\n';
                                 
                              });
            }
        });

    io.run();
}
