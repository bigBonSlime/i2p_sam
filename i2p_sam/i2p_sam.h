#ifndef I2P_SAM_H
#define I2P_SAM_H
#include "boost/asio.hpp"
#include "sam_error.h"
#include <iostream>
#include <memory>
#include <string>

namespace i2p_sam {
const std::string min_version = "3.1";
const std::string max_version = "3.3";
enum sam_session_type { stream };
const uint64_t max_sam_answer_lenght = 10000;
const uint16_t sam_default_port = 7656;
std::string public_destination_from_priv_key(const std::string priv_key);

i2p_sam::errors::sam_error get_result(std::string);

std::string get_value(const std::string &, const std::string &);

class sam_socket {
public:
    boost::asio::any_io_executor get_io_executor();

    template <typename T>
    void async_write([[maybe_unused]] const void *data, [[maybe_unused]] uint64_t len, T handler) {
        boost::asio::async_write(
            this->socket, boost::asio::buffer(data, len),
            [handler](const boost::system::error_code error, uint64_t bytes_transferred) {
                if (!error) {
                    handler(errors::sam_error(), bytes_transferred);
                } else {
                    handler(errors::sam_error(errors::error_codes::network_error, error.what()),
                            bytes_transferred);
                }
            });
    }

    template <typename T> void async_write(const std::string &data, T handler) {
        std::shared_ptr<std::string> p_data(new std::string(data));
        async_write(
            p_data->data(), p_data->size(),
            [handler, p_data](errors::sam_error err, uint64_t bytes) { handler(err, bytes); });
    }

    template <typename T> void async_read(void *dest, uint64_t bytes, T handler) {
        boost::asio::async_read(
            this->socket, boost::asio::buffer(dest, bytes),
            [handler](const boost::system::error_code &error, uint64_t bytes_transferred) {
                if (!error) {
                    handler(errors::sam_error(), bytes_transferred);
                } else {
                    handler(errors::sam_error(errors::error_codes::network_error, error.what()),
                            bytes_transferred);
                }
            });
    }

    template <typename T> void async_read_line(uint64_t max_size, T handler) {
        std::shared_ptr<std::vector<char>> vec(new std::vector<char>);
        boost::asio::async_read_until(
            this->socket, boost::asio::dynamic_buffer(*vec, max_size), '\n',
            [vec, handler](const boost::system::error_code &error, std::size_t) {
                if (error) {
                    handler(std::move(std::string(vec->begin(), vec->end())),
                            i2p_sam::errors::sam_error(i2p_sam::errors::error_codes::network_error,
                                                       error.what()));
                } else {
                    handler(std::move(std::string(vec->begin(), vec->end())),
                            i2p_sam::errors::sam_error());
                }
            });
    }

    template <typename T>
    void async_connect(const std::string host, const uint16_t port, T handler) {
        std::shared_ptr<boost::asio::ip::tcp::resolver> resolver(
            new boost::asio::ip::tcp::resolver(this->socket.get_executor()));
        resolver->async_resolve(
            host, std::to_string(port),
            [resolver, handler,
             &sock = this->socket](const boost::system::error_code &ec,
                                   boost::asio::ip::tcp::resolver::results_type results) {
                if (!ec) {
                    boost::asio::async_connect(
                        sock, results,
                        [handler](const boost::system::error_code &ec,
                                  const boost::asio::ip ::tcp::endpoint &) {
                            if (!ec) {
                                handler(errors::sam_error());
                            } else {
                                handler(errors::sam_error(errors::error_codes::network_error,
                                                          ec.what()));
                            }
                        });
                } else {
                    handler(errors::sam_error(errors::error_codes::network_error, ec.what()));
                }
            });
    }

    void close() { socket.close(); }

    template <typename executor_type> sam_socket(executor_type &ex) : socket(ex) {}
    template <typename executor_type> sam_socket(const executor_type &ex) : socket(ex) {}

private:
    boost::asio::ip::tcp::socket socket;
};

class sam_session {
public:
    std::string get_id();
    std::string get_public_destination();
    std::string get_private_destination();

protected:
    sam_session(sam_socket &sock, const std::string &id_, const std::string &public_destination_,
                const std::string &private_destination_);
    sam_session() = delete;
    sam_session &operator=(const sam_session &) = delete;
    sam_session(sam_session &&) = default;
    sam_session(const sam_session &) = delete;
    sam_session &operator=(sam_session &&) = default;
    ~sam_session() = default;

    sam_socket socket;
    std::string id;
    std::string public_destination;
    std::string private_destination;
};

class stream_session : public sam_session {
private:
    stream_session(sam_socket &, const std::string &, const std::string &, const std::string &);

public:
    stream_session(const stream_session &) = delete;
    stream_session(stream_session &&) = default;
    stream_session &operator=(const stream_session &) = delete;
    stream_session &operator=(stream_session &&) = default;
    ~stream_session() = default;

    template <typename T>
    friend void async_create_stream_session(boost::asio::io_context &, const std::string &,
                                            const std::string &, const std::string &,
                                            const std::string &, T, const std::string &, uint16_t);

    template <typename T>
    void async_accept(T handler, const std::string &host = "127.0.0.1",
                      uint16_t port = i2p_sam::sam_default_port,
                      const std::string &handshake_params = "") {
        std::shared_ptr<sam_socket> sam_sock(new sam_socket(this->socket.get_io_executor()));
        sam_sock->async_connect(
            host, port, [sam_sock, id = this->id, handshake_params, handler](errors::sam_error ec) {
                if (!ec) {
                    async_handshake(
                        *sam_sock, handshake_params, [id, sam_sock, handler](errors::sam_error ec) {
                            if (!ec) {
                                sam_sock->async_write(
                                    "STREAM ACCEPT ID=" + id + "\n",
                                    [sam_sock, handler](errors::sam_error ec, uint64_t) {
                                        if (!ec) {
                                            sam_sock->async_read_line(
                                                max_sam_answer_lenght,
                                                [sam_sock, handler](std::string res,
                                                                    errors::sam_error ec) {
                                                    if (!ec) {
                                                        errors::sam_error er =
                                                            i2p_sam::get_result(res);
                                                        if (!er) {
                                                            sam_sock->async_read_line(
                                                                max_sam_answer_lenght,
                                                                [sam_sock,
                                                                 handler](std::string res,
                                                                          errors::sam_error ec) {
                                                                    if (!ec) {
                                                                        handler(
                                                                            sam_sock,
                                                                            res.substr(
                                                                                0, res.find(" ")),
                                                                            static_cast<
                                                                                uint16_t>(std::stoi(
                                                                                i2p_sam::get_value(
                                                                                    res,
                                                                                    "FROM_PORT"))),
                                                                            static_cast<
                                                                                uint16_t>(std::stoi(
                                                                                i2p_sam::get_value(
                                                                                    res,
                                                                                    "TO_PORT"))),
                                                                            ec);
                                                                    } else {
                                                                        handler(
                                                                            sam_sock, "",
                                                                            static_cast<uint16_t>(
                                                                                0),
                                                                            static_cast<uint16_t>(
                                                                                0),
                                                                            ec);
                                                                    }
                                                                });

                                                        } else {
                                                            handler(sam_sock, "",
                                                                    static_cast<uint16_t>(0),
                                                                    static_cast<uint16_t>(0), ec);
                                                        }

                                                    } else {
                                                        handler(sam_sock, "",
                                                                static_cast<uint16_t>(0),
                                                                static_cast<uint16_t>(0), ec);
                                                    }
                                                });
                                        }
                                    });

                            } else {
                                handler(sam_sock, "", static_cast<uint16_t>(0),
                                        static_cast<uint16_t>(0), ec);
                            }
                        });
                } else {
                    handler(sam_sock, "", static_cast<uint16_t>(0), static_cast<uint16_t>(0), ec);
                }
            });
    }
    /*
                                 /|\
                                / | \
                                  |    weird shit
    */

    template <typename T>
    void async_connect(const std::string &id, [[maybe_unused]] const std::string &dest, T handler,
                       uint16_t from_port = 0, uint16_t to_port = 0,
                       const std::string &sam_host = "127.0.0.1",
                       uint16_t sam_port = i2p_sam::sam_default_port,
                       const std::string &handshake_params = "") {
        std::shared_ptr<sam_socket> sam_sock(new sam_socket(this->socket.get_io_executor()));
        sam_sock->async_connect(sam_host, sam_port, [=](errors::sam_error ec) {
            if (!ec) {
                async_handshake(*sam_sock, handshake_params, [=](errors::sam_error ec) {
                    if (!ec) {
                        sam_sock->async_write(
                            "STREAM CONNECT ID=" + id + " DESTINATION=" + dest +
                                " FROM_PORT=" + std::to_string(from_port) +
                                " TO_PORT=" + std::to_string(to_port) + "\n",
                            [=](errors::sam_error ec, uint64_t) {
                                if (!ec) {
                                    sam_sock->async_read_line(
                                        i2p_sam::max_sam_answer_lenght,
                                        [=](std::string res, errors::sam_error ec) {
                                            handler(sam_sock, (ec == i2p_sam::errors::sam_error())
                                                                  ? i2p_sam::get_result(res)
                                                                  : ec);
                                        });
                                } else {
                                    handler(sam_sock, ec);
                                }
                            });

                    } else {
                        handler(sam_sock, ec);
                    }
                });

            } else {
                handler(sam_sock, ec);
            }
        });
    }

    template <typename T>
    void async_forward(const uint16_t forward_port, T handler,
                       const std::string &forward_host = "127.0.0.1", const bool silent = false,
                       const bool ssl = false, const std::string &sam_host = "127.0.0.1",
                       uint16_t sam_port = i2p_sam::sam_default_port,
                       const std::string &handshake_params = "") {
        std::shared_ptr<sam_socket> sam_sock(new sam_socket(this->socket.get_io_executor()));
        sam_sock->async_connect(sam_host, sam_port, [=, id = this->get_id()](errors::sam_error ec) {
            if (!ec) {
                async_handshake(*sam_sock, handshake_params, [=](errors::sam_error ec) {
                    if (!ec) {
                        sam_sock->async_write(
                            "STREAM FORWARD ID=" + id + " PORT=" + std::to_string(forward_port) +
                                " HOST=" + forward_host + " SILENT=" + (silent ? "true" : "false") +
                                " SSL=" + (ssl ? "true" : "false") + "\n",
                            [=](errors::sam_error ec, uint64_t) {
                                if (ec) {
                                    sam_sock->close();
                                }
                                sam_sock->async_read_line(
                                    i2p_sam::max_sam_answer_lenght,
                                    [=](std::string res, errors::sam_error ec) {
                                        handler(sam_sock, ec ? ec : i2p_sam::get_result(res));
                                    });
                            });
                    } else {
                        handler(sam_sock, ec);
                    }
                });
            } else {
                handler(sam_sock, ec);
            }
        });
    }
};

template <typename T>
void async_handshake(sam_socket &sam_sock, const std::string &args, T handler) {
    std::string sam_request = std::string("HELLO VERSION ") + args + std::string("\n");

    sam_sock.async_write(sam_request, [handler, &sam_sock](errors::sam_error err, uint64_t) {
        if (!err) {
            sam_sock.async_read_line(
                max_sam_answer_lenght, [handler](std::string s, i2p_sam::errors::sam_error ec) {
                    if (!ec) {
                        handler(get_result(s));
                    } else {
                        handler(errors::sam_error(errors::network_error, ec.what()));
                    }
                });

        } else {
            handler(err);
        }
    });
}

template <typename T>
void async_create_stream_session(boost::asio::io_context &io_context, const std::string &id,
                                 const std::string &destination,
                                 const std::string &handshake_params,
                                 const std::string &stream_params, T handler,
                                 const std::string &sam_host = "127.0.0.1",
                                 uint16_t sam_port = 7656) {
    std::shared_ptr<sam_socket> sam_sock(new sam_socket(io_context));
    sam_sock->async_connect(sam_host, sam_port, [=](errors::sam_error ec) {
        if (!ec) {
            async_handshake(*sam_sock, handshake_params, [=](errors::sam_error ec) {
                if (!ec) {
                    std::string sam_create_request = "SESSION CREATE STYLE=STREAM ID=" + id +
                                                     " DESTINATION=" + destination + " " +
                                                     stream_params + "\n";
                    sam_sock->async_write(sam_create_request, [=, sign_type =
                                                                      get_value(stream_params,
                                                                                "SIGNATURE_TYPE")](
                                                                  errors::sam_error ec, uint64_t) {
                        if (!ec) {
                            sam_sock->async_read_line(10000, [=](std::string s,
                                                                 errors::sam_error ec) {
                                if (!ec) {
                                    auto res = get_result(s);
                                    if (res == i2p_sam::errors::sam_error()) {
                                        std::string ret_destination =
                                            i2p_sam::get_value(s, "DESTINATION");
                                        handler(
                                            std::shared_ptr<stream_session>(new stream_session(
                                                *sam_sock, id,
                                                public_destination_from_priv_key(ret_destination),
                                                ret_destination)),
                                            i2p_sam::errors::sam_error());

                                    } else {
                                        handler(std::shared_ptr<stream_session>(
                                                    new stream_session(*sam_sock, "", "", "")),
                                                ec);
                                    }
                                } else {
                                    handler(std::shared_ptr<stream_session>(
                                                new stream_session(*sam_sock, "", "", "")),
                                            ec);
                                }
                            });
                        } else {
                            handler(std::shared_ptr<stream_session>(
                                        new stream_session(*sam_sock, "", "", "")),
                                    ec);
                        }
                    });
                } else {
                    handler(
                        std::shared_ptr<stream_session>(new stream_session(*sam_sock, "", "", "")),
                        ec);
                }
            });
        } else {
            handler(std::shared_ptr<stream_session>(new stream_session(*sam_sock, "", "", "")), ec);
        }
    });
}

} // namespace i2p_sam

#endif