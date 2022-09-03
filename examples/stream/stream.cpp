#include "boost/asio.hpp"
#include "i2p_sam.h"
#include <iostream>
#include <string>

int main() {
    boost::asio::io_context io;
    std::string id = "ID";
    std::string handshake_params = "";
    std::string stream_params = "SIGNATURE_TYPE=ECDSA_SHA512_P521";
    std::string destiantion = "TRANSIENT";
    i2p_sam::stream_session::async_create_stream_session(
        io, id, destiantion, handshake_params, stream_params,
        [](std::shared_ptr<i2p_sam::stream_session> s, i2p_sam::errors::sam_error e) {
            s->async_accept([s]([[maybe_unused]] std::shared_ptr<i2p_sam::sam_socket> sam_sock,
                                std::string, uint16_t, uint16_t, i2p_sam::errors::sam_error) {});
        });
    io.run();
}
