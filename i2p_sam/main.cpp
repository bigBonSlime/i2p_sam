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

    i2p_sam::stream_session::async_create_stream_session(
        io, "LEFT", "TRANSIENT", "", "",
        [](std::shared_ptr<i2p_sam::stream_session> s, i2p_sam::errors::sam_error ec) {
            std::cout << ec.what() << std::endl;
            std::cout << s->get_public_destination() << std::endl;
            s->async_forward(6065, [s](std::shared_ptr<i2p_sam::sam_socket> sock,
                                       i2p_sam::errors::sam_error ec) {
                std::cout << ec.what() << std::endl;
                [[maybe_unused]] auto ss = new i2p_sam::sam_socket(std::move(*sock));
                [[maybe_unused]] auto sss = new i2p_sam::stream_session(std::move(*s));
            });
        });

    io.run();
}
