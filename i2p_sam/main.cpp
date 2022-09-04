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
    std::string my_dest =
        "Oki2q~CuZEaoASGPl~p9l6bUxXv44HiVAKmDLPQfqPyu3ksoJXzk6aiYNH~"
        "y0XyjksLllX5yEBOzNSgr7mhsoIgTXhvBEcbNY~"
        "di9oT28SnEgQyWDvgka2DDR52L8ev113MGx6rOoWYEjIWwtwqIbsoAzymcaVfqbM3CBnfMnmE3XASvBU58VV2dk7Yc"
        "~vTCD4w5MeglqmSYBK4oRNSdhZWrBweUw9Gh4uNL6Km5-Q~"
        "TP7EN595wLzrf6FegrYizRU26UGzG4qlMLfsNxOp5U3CtxegXvub5o0mv0IThOh2mjmW4JbxSha2rzcP-"
        "0AYe4OOje0262O3jXrfhW-ikSEjpnTi-P4CB3Ml395eBA~USYkAGU96lu7EquPtSMnn11YjYz-"
        "MoQQabkdzSWSMz0f5B9zI6vkx4UbxwkG51MzL0Oo54qDPRmkcflzg4XaWs9OzdtbHVPWkx5y9nHLKOF4Qua2Ee2lh7"
        "n5o~4NAIs~DG3-"
        "FqkFZV7En2k3hGgmkEAAAAIeJq736Ugq5vsjZiDUXpTFxNMNKNKfBoHFcLiBh51d54InUV6cEkUOczECDZPGqOFJOG"
        "fpLMB2hdvwvQYFZGbUP8h5H0NRYw1ZpvoDWYXOXI6MenDfC-jquXqR0xvnqK4NxytQgEezVIrthp-"
        "aQbr6z4hzQCAPtT26a6RWyK3P6R92u5b7dVU0sEVaC7Fcwk5-Xg1PnDPAJMKEPLUqFj4YbO9kr9RhTeS6-"
        "xFFP75GEsO0UeAtFYWZYayNJF5U2ScXD9k~qVpC8jO7ne4IaA-"
        "7CgJFMGA1QGjPGn6n16Ww6BmNeiphFGM2nPdCJWvXmJWeDCf4a1nDthkt1xPfU9QmI3W86ZRySUyEZhWDdBp7~"
        "Hk5I~"; // maybe TRANSIENT
    i2p_sam::datagram_session::async_create_datagram_session(
        io, false, "ACCEPTD", my_dest, "", "",
        [=](std::shared_ptr<i2p_sam::datagram_session> s, i2p_sam::errors::sam_error ec) {
            std::cout << "Session create " << ec.what() << '\n';
            readdata(s);
        });

    i2p_sam::datagram_session::async_create_datagram_session(
        io, false, "SENDERD", "TRANSIENT", "", "",
        [=](std::shared_ptr<i2p_sam::datagram_session> s, i2p_sam::errors::sam_error ec) {
            std::cout << "Session create " << ec.what() << '\n';
            std::string t_dest = i2p_sam::public_destination_from_priv_key(my_dest);
            std::string data = "Hello world";
            for (int i = 0; i < 1000; i++) {
                s->async_send(t_dest, data.data(), (uint16_t)data.size(), "",
                              [](i2p_sam::errors::sam_error) {

                              });
            }
        });

    {
        std::string id = "ID";
        std::string handshake_params = "";
        std::string stream_params = "SIGNATURE_TYPE=ECDSA_SHA512_P521";
        std::string destiantion = "TRANSIENT";
        i2p_sam::stream_session::async_create_stream_session(
            io, id, destiantion, handshake_params, stream_params,
            [](std::shared_ptr<i2p_sam::stream_session> s, i2p_sam::errors::sam_error) {
                s->async_accept([s]([[maybe_unused]] std::shared_ptr<i2p_sam::sam_socket> sam_sock,
                                    std::string, uint16_t, uint16_t,
                                    i2p_sam::errors::sam_error) {});
            });
    }

    io.run();
}
