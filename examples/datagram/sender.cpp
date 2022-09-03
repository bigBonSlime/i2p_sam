#include "boost/asio.hpp"
#include "i2p_sam.h"
#include <iostream>
#include <string>

int main() {
    boost::asio::io_context io;
    std::string dest = "target_destination";
    i2p_sam::datagram_session::async_create_datagram_session(
        io, false, "ID", dest, "", "",
        [=](std::shared_ptr<i2p_sam::datagram_session> s, i2p_sam::errors::sam_error ec) {
            std::cout << "Session create " << ec.what() << '\n';
            std::string t_dest = "target destination";
            std::string data = "Hello world";
            for (int i = 0; i < 1000; i++) {
                s->async_send(t_dest, data.data(), (uint16_t)data.size(), "",
                              [](i2p_sam::errors::sam_error) {

                              });
            }
        });

    io.run();
}
