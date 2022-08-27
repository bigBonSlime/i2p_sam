#include "boost/asio.hpp"
#include "i2p_sam.h"
#include <iostream>
#include <string>
int main() {
    [[maybe_unused]] boost::asio::io_context io;
    [[maybe_unused]] std::string host = "127.0.0.1";
    [[maybe_unused]] uint16_t port = 7656;
    [[maybe_unused]] std::string id = "KEK5";
    [[maybe_unused]] std::string handshake_params = "MIN=3.2 MAX=3.3";
    [[maybe_unused]] std::string params = "SIGNATURE_TYPE=ECDSA_SHA512_P521";
    [[maybe_unused]] std::string destiantion = "TRANSIENT";

    i2p_sam::async_create_stream_session(
        io, host, port, id, destiantion, handshake_params, params,
        [](std::shared_ptr<i2p_sam::stream_session> s, i2p_sam::errors::sam_error ec) {
            std::cout << ec.what() << std::endl;
            std::cout << s->get_public_destination() << std::endl;
            s->async_accept([s]([[maybe_unused]] std::shared_ptr<i2p_sam::sam_socket> sam_sock,
                                std::string, uint16_t, uint16_t, i2p_sam::errors::sam_error) {});
        });

    io.run();
}
