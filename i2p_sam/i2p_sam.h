#ifndef I2P_SAM_H
#define I2P_SAM_H
#include "boost/asio.hpp"
#include "sam_error.h"
#include <deque>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

namespace i2p_sam {
const std::string sam_version = "3.3";
enum sam_session_type { stream };
const uint64_t max_sam_answer_lenght = 10000000;
// const uint64_t max_socket_buffer_lenght = 100000;
const uint16_t sam_default_port = 7656;
const uint16_t sam_udp_port = 7655;
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

    boost::asio::ip::tcp::endpoint remote_endpoint();

    boost::asio::ip::tcp::endpoint local_endpoint();

    template <typename T> void async_write(const std::string &data, T handler) {
        std::shared_ptr<std::string> p_data(new std::string(data));
        async_write(
            p_data->data(), p_data->size(),
            [handler, p_data](errors::sam_error err, uint64_t bytes) { handler(err, bytes); });
    }

    template <typename T>
    void async_read([[maybe_unused]] std::byte *dest, [[maybe_unused]] uint64_t bytes,
                    [[maybe_unused]] T handler) {

        if (bytes == 0) {
            handler(errors::sam_error(), 0);
            return;
        }
        if (bytes <= buffer.size()) {
            std::memcpy(dest, buffer.data(), bytes);
            buffer.erase(buffer.begin(), buffer.begin() + bytes);
            handler(errors::sam_error(), bytes);
        } else {
            std::size_t buffer_size = buffer.size();
            if (!buffer.empty()) {
                std::memcpy(dest, buffer.data(), buffer.size());
                buffer.clear();
            }
            boost::asio::async_read(
                this->socket, boost::asio::buffer(dest + buffer.size(), bytes - buffer.size()),
                [=](const boost::system::error_code error, std::size_t bytes_transferred) {
                    handler(error ? (errors::sam_error(errors::error_codes::network_error,
                                                       error.what()))
                                  : (errors::sam_error()),
                            buffer_size + bytes_transferred);
                });
            /*
            std::size_t read_from_buffer = buffer.size() - buffer_start_pos;
            boost::asio::async_read(
                this->socket,
                boost::asio::buffer(dest + read_from_buffer, bytes - read_from_buffer),
                [dest, read_from_buffer, buffer = &buffer, buffer_start_pos = &buffer_start_pos,
                 handler](const boost::system::error_code error, std::size_t bytes_transferred) {
                    if (read_from_buffer != 0) {
                        std::memcpy(dest, buffer->data() + *buffer_start_pos, read_from_buffer);
                    }
                    buffer->clear();
                    *buffer_start_pos = 0;
                    handler(error ? (errors::sam_error(errors::error_codes::network_error,
                                                       error.what()))
                                  : (errors::sam_error()),
                            read_from_buffer + bytes_transferred);
                });
                */
        }
    }

    template <typename T>
    void async_read_line([[maybe_unused]] uint64_t max_size, [[maybe_unused]] T handler) {
        auto buffer_p = &buffer;
        boost::asio::async_read_until(
            this->socket, boost::asio::dynamic_buffer(*buffer_p, max_size), '\n',
            [handler, buffer_p]([[maybe_unused]] const boost::system::error_code &error,
                                std::size_t delimiter_pos) {
                if (!error) {
                    std::string res_str(reinterpret_cast<char *>(buffer_p->data()), delimiter_pos);
                    buffer_p->erase(buffer_p->begin(),
                                    buffer_p->begin() +
                                        delimiter_pos); // tooooooooo long. idk how to fix
                    handler(std::move(res_str), i2p_sam::errors::sam_error());
                } else {
                    std::string res_str(reinterpret_cast<char *>(buffer_p->data()),
                                        buffer_p->size());
                    buffer_p->clear();
                    handler(std::move(res_str), i2p_sam::errors::sam_error(
                                                    i2p_sam::errors::network_error, error.what()));
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
    std::vector<std::byte> buffer;
    // boost::asio::streambuf buffer;
    // std::size_t buffer_start_pos = 0;
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

template <typename T>
void async_create_session(boost::asio::io_context &io_context, const std::string &style,
                          const std::string &id, const std::string &destination,
                          const std::string &handshake_params, const std::string &stream_params,
                          T handler, const std::string &sam_host = "127.0.0.1",
                          uint16_t sam_port = 7656) {
    std::shared_ptr<sam_socket> sam_sock(new sam_socket(io_context));
    sam_sock->async_connect(sam_host, sam_port, [=](errors::sam_error ec) {
        if (!ec) {
            async_handshake(*sam_sock, handshake_params, [=](errors::sam_error ec) {
                if (!ec) {
                    std::string sam_create_request = "SESSION CREATE STYLE=" + style + " ID=" + id +
                                                     " DESTINATION=" + destination + " " +
                                                     stream_params + "\n";
                    sam_sock->async_write(sam_create_request, [=](errors::sam_error ec, uint64_t) {
                        if (!ec) {
                            sam_sock->async_read_line(10000, [=](std::string s,
                                                                 errors::sam_error ec) {
                                if (!ec) {
                                    auto res = get_result(s);
                                    if (res == i2p_sam::errors::sam_error()) {
                                        std::string ret_destination =
                                            i2p_sam::get_value(s, "DESTINATION");
                                        handler(sam_sock, id,
                                                public_destination_from_priv_key(ret_destination),
                                                ret_destination, i2p_sam::errors::sam_error());

                                    } else {
                                        handler(sam_sock, "", "", "", ec);
                                    }
                                } else {
                                    handler(sam_sock, "", "", "", ec);
                                }
                            });
                        } else {
                            handler(sam_sock, "", "", "", ec);
                        }
                    });
                } else {
                    handler(sam_sock, "", "", "", ec);
                }
            });
        } else {
            handler(sam_sock, "", "", "", ec);
        }
    });
}

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
    static void async_create_stream_session(boost::asio::io_context &io_context,
                                            const std::string &id, const std::string &destination,
                                            const std::string &handshake_params,
                                            const std::string &stream_params, T handler,
                                            const std::string &sam_host = "127.0.0.1",
                                            uint16_t sam_port = 7656) {
        i2p_sam::async_create_session(
            io_context, "STREAM", id, destination, handshake_params, stream_params,
            [handler](std::shared_ptr<i2p_sam::sam_socket> sam_sock, std::string id,
                      std::string pub_dest, std::string priv_dest, i2p_sam::errors::sam_error ec) {
                if (!ec) {
                    handler(std::shared_ptr<stream_session>(
                                new stream_session(*sam_sock, id, pub_dest, priv_dest)),
                            i2p_sam::errors::sam_error());
                } else {
                    handler(
                        std::shared_ptr<stream_session>(new stream_session(*sam_sock, "", "", "")),
                        ec);
                }
            },
            sam_host, sam_port);
    }

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

class datagram_session : public sam_session {
private:
    bool anonymous;
    datagram_session(sam_socket &, const std::string &, const std::string &, const std::string &,
                     bool anonymous_ = false);
    template <typename T>
    void async_send_udp(const std::string &host, uint16_t port,
                        std::shared_ptr<std::byte[]> data_ptr, std::size_t size, T handler) {
        std::shared_ptr<boost::asio::ip::udp::resolver> resolver(
            new boost::asio::ip::udp::resolver(this->socket.get_io_executor()));
        std::shared_ptr<boost::asio::ip::udp::socket> udpsocket(
            new boost::asio::ip::udp::socket(this->socket.get_io_executor()));
        udpsocket->open(boost::asio::ip::udp::v4());
        resolver->async_resolve(
            host, std::to_string(port),
            [data_ptr, udpsocket, handler, size,
             resolver](const boost::system::error_code &ec,
                       boost::asio::ip::udp::resolver::results_type results) {
                if (!ec) {
                    udpsocket->async_send_to(
                        boost::asio::buffer(data_ptr.get(), size), *(results.begin()),
                        [data_ptr, handler, udpsocket](const boost::system::error_code &ec,
                                                       std::size_t) {
                            if (!ec) {
                                handler(i2p_sam::errors::sam_error());
                            } else {
                                handler(i2p_sam::errors::sam_error(i2p_sam::errors::network_error,
                                                                   ec.what()));
                            }
                        });
                } else {
                    handler(i2p_sam::errors::sam_error(i2p_sam::errors::network_error, ec.what()));
                }
            });
    }

public:
    datagram_session(const datagram_session &) = delete;
    datagram_session(datagram_session &&) = default;
    datagram_session &operator=(const datagram_session &) = delete;
    datagram_session &operator=(datagram_session &&) = default;
    ~datagram_session() = default;

    template <typename T>
    static void async_create_datagram_session(boost::asio::io_context &io_context, bool anonymous,
                                              const std::string &id, const std::string &destination,
                                              const std::string &handshake_params,
                                              const std::string &datagram_params, T handler,
                                              const std::string &sam_host = "127.0.0.1",
                                              uint16_t sam_port = 7656) {
        std::string session_type = (anonymous ? "RAW" : "DATAGRAM");
        i2p_sam::async_create_session(
            io_context, session_type, id, destination, handshake_params, datagram_params,
            [handler, anonymous](std::shared_ptr<i2p_sam::sam_socket> sam_sock, std::string id,
                                 std::string pub_dest, std::string priv_dest,
                                 i2p_sam::errors::sam_error ec) {
                if (!ec) {
                    handler(std::shared_ptr<datagram_session>(
                                new datagram_session(*sam_sock, id, pub_dest, priv_dest)),
                            i2p_sam::errors::sam_error());
                } else {
                    handler(std::shared_ptr<datagram_session>(
                                new datagram_session(*sam_sock, "", "", "", anonymous)),
                            ec);
                }
            },
            sam_host, sam_port);
    }

    template <typename T> // uint16_t because datagram limits
    void async_send([[maybe_unused]] const std::string &destination, [[maybe_unused]] void *data,
                    [[maybe_unused]] uint16_t size,
                    [[maybe_unused]] const std::string &datagram_params,
                    [[maybe_unused]] T handler) {
        std::string datagram_info = i2p_sam::sam_version + " " + this->get_id() + " " +
                                    destination + " " + datagram_params + "\n";
        std::shared_ptr<std::byte[]> data_ptr(new std::byte[datagram_info.size() + size]);
        std::memcpy(data_ptr.get(), datagram_info.data(), datagram_info.size());
        std::memcpy(data_ptr.get() + datagram_info.size(), data, size);
        async_send_udp(this->socket.remote_endpoint().address().to_string(), i2p_sam::sam_udp_port,
                       data_ptr, datagram_info.size() + size, handler);
    }

    template <typename T> // forward
    void async_read_datagram(T handler, const std::string &host, uint16_t port,
                             bool silent = true) {
        this->async_read_datagram([=](std::string dest, std::size_t size, uint16_t from_port,
                                      uint16_t to_port, std::shared_ptr<std::byte[]> data,
                                      i2p_sam::errors::sam_error ec) {
            if (!ec) {
                std::string header;
                if (!silent) {
                    if (dest != "") {
                        header += "DESTINATION=" + dest + " ";
                    }
                    header += "SIZE=" + std::to_string(size) +
                              " FROM_PORT=" + std::to_string(from_port) +
                              " TO_PORT=" std::to_string(to_port) + "\n";
                }
                std::shared_ptr<std::byte[]> data_ptr(new std::byte[header.size() + size]);
                std::memcpy(data_ptr.get(), header.data(), header.size());
                std::memcpy(data_ptr.get() + header.size(), data.get(), size);
                async_send_udp(host, port, data_ptr, size + header.size(), handler);
            } else {
                handler(ec);
            }
        });
    }

    template <typename T> // read
    void async_read_datagram(T handler) {
        this->socket.async_read_line(
            i2p_sam::max_sam_answer_lenght,
            [this, handler, socket = &this->socket](std::string s, i2p_sam::errors::sam_error ec) {
                if (!ec) {
                    const char datagram_rec[] = "DATAGRAM RECEIVED";
                    const char raw_rec[] = "RAW RECEIVED";
                    [[maybe_unused]] auto i = sizeof(datagram_rec);
                    if (((s.size() >= sizeof(datagram_rec)) &&
                         (std::memcmp(s.data(), datagram_rec, sizeof(datagram_rec) - 1) == 0)) ||
                        ((s.size() >= sizeof(raw_rec)) &&
                         (std::memcmp(s.data(), raw_rec, sizeof(raw_rec) - 1) == 0))) {
                        std::string dest = i2p_sam::get_value(s, "DESTINATION");
                        std::size_t size =
                            static_cast<std::size_t>(std::stoull(i2p_sam::get_value(s, "SIZE")));
                        std::uint16_t from_port = static_cast<std::uint16_t>(
                            std::stoull(i2p_sam::get_value(s, "FROM_PORT")));
                        std::uint16_t to_port = static_cast<std::uint16_t>(
                            std::stoull(i2p_sam::get_value(s, "TO_PORT")));
                        std::shared_ptr<std::byte[]> data(new std::byte[size]);
                        socket->async_read(
                            data.get(), size, [=](i2p_sam::errors::sam_error ec, uint64_t bc) {
                                if (!ec) {
                                    handler(dest, size, from_port, to_port, std::move(data), ec);
                                } else {
                                    handler(dest, bc, from_port, to_port, std::move(data), ec);
                                }
                            });

                    } else { // PONG???
                        if ((s.size() >= 4) && (std::memcmp(s.data(), "PONG", 4) == 0)) {
                            std::string text = s.substr(s.find(' ') + 1);
                            socket->async_write("PONG " + text + "\n",
                                                [](i2p_sam::errors::sam_error, uint64_t) {

                                                });
                        } else {
                            handler(
                                "", 0, 0, 0, nullptr,
                                i2p_sam::errors::sam_error(i2p_sam::errors::unexpected_input, ""));
                        }
                    }
                } else {
                    handler("", 0, 0, 0, nullptr, ec);
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

} // namespace i2p_sam

#endif