#include "i2p_sam.h"
#include "base64.h"
#include <iostream>

std::string i2p_sam::sam_session::get_id() { return id; }
std::string i2p_sam::sam_session::get_public_destination() { return public_destination; }
std::string i2p_sam::sam_session::get_private_destination() { return private_destination; }

i2p_sam::sam_session::sam_session(sam_socket &sock, const std::string &id_,
                                  const std::string &public_destination_,
                                  const std::string &private_destination_)
    : socket(std::move(sock)), id(id_), public_destination(public_destination_),
      private_destination(private_destination_) {}

i2p_sam::stream_session::stream_session(i2p_sam::sam_socket &sock, const std::string &id_,
                                        const std::string &public_destination_,
                                        const std::string &private_destination_)
    : sam_session(sock, id_, public_destination_, private_destination_) {}

std::string i2p_sam::get_value(const std::string &s, const std::string &value) {
    if (value.empty()) {
        return std::string();
    }
    size_t beg_pos = s.find(value + "=");
    if (beg_pos == std::string::npos) {
        return std::string();
    }
    beg_pos += value.size() + 1;
    size_t end_pos = s.find_first_of(" \n", beg_pos);
    if (end_pos == std::string::npos) {
        return s.substr(beg_pos);
    } else {
        return s.substr(beg_pos, end_pos - beg_pos);
    }
}

i2p_sam::errors::sam_error i2p_sam::get_result(std::string s) { // not all errors
    std::string err = get_value(s, "RESULT");
    if (err == "OK") {
        return {i2p_sam::errors::error_codes::success, "OK"};
    }
    if (err == "I2P_ERROR") {
        return {i2p_sam::errors::error_codes::i2p_error, get_value(s, "MESSAGE")};
    }
    if (err == "NOVERSION") {
        return {i2p_sam::errors::error_codes::i2p_error_noversion, "NOVERSION"};
    }
    if (err == "DUPLICATED_ID") {
        return {i2p_sam::errors::error_codes::i2p_error_duplicated_id, "DUPLICATED_ID"};
    }
    if (err == "DUPLICATED_DEST") {
        return {i2p_sam::errors::error_codes::i2p_error_duplicated_dest, "DUPLICATED_DEST"};
    }
    if (err == "INVALID_KEY") {
        return {i2p_sam::errors::error_codes::i2p_error_invalid_key, "INVALID_KEY"};
    }

    return {i2p_sam::errors::error_codes::unknown_error, "Unknown error"};
}

bool is_big_endian(void) {
    union {
        uint32_t i;
        char c[4];
    } bint = {0x01020304};

    return bint.c[0] == 1;
}

std::string i2p_sam::public_destination_from_priv_key(const std::string priv_key) {
    // to do: fix base64 to dont throw exceptions
    try {
        auto res = base64::base64_decode(priv_key);
        uint16_t len = 0;
        if (res.size() < 387) {
            return "";
        }
        std::memcpy(&len, res.data() + 385,
                    2); // https://geti2p.net/spec/common-structures#keysandcert
        if (!is_big_endian()) {
            uint8_t *bytes = reinterpret_cast<uint8_t *>(&len);
            std::swap(bytes[0], bytes[1]);
        }
        if (387 + len > res.size()) {
            return "";
        }
        res = res.substr(0, 387 + len);
        return base64::base64_encode(res, false);
    } catch (...) {
        return "";
    }
}

boost::asio::any_io_executor i2p_sam::sam_socket::get_io_executor() {
    return this->socket.get_executor();
}
