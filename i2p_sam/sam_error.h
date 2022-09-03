#ifndef I2P_SAM_ERROR_H
#define I2P_SAM_ERROR_H
// Very bad error class
#include <cstdint>
#include <string>
namespace i2p_sam {

namespace errors {
enum error_codes {
    success,
    network_error,
    i2p_error_cant_reach_peer,
    i2p_error_duplicated_dest,
    i2p_error,
    i2p_error_invalid_key,
    i2p_error_key_not_found,
    i2p_error_peer_not_found,
    i2p_error_duplicated_id,
    i2p_error_noversion,
    i2p_error_timeout,
    unexpected_input,
    unknown_error
};

class sam_error {
public:
    sam_error(error_codes error_code_ = error_codes::success, std::string info_ = "Success")
        : error_code(error_code_), info(info_) {}

    // bool operator==(const sam_error &);
    // bool operator!=(const sam_error &);
    operator bool();
    bool operator!();
    error_codes category();
    std::string what();

private:
    error_codes error_code;
    const std::string info;
};

} // namespace errors
} // namespace i2p_sam
#endif