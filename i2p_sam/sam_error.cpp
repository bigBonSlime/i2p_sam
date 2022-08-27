#include "sam_error.h"

/*
bool i2p_sam::errors::sam_error::operator==(const sam_error &a) {
    return (this->error_code == a.error_code);
}

bool i2p_sam::errors::sam_error::operator!=(const sam_error &a) {
    return !(*this == a);
}
*/

bool i2p_sam::errors::sam_error::operator!() {
    sam_error success_er;
    return (this->error_code == success_er.error_code);
}

std::string i2p_sam::errors::sam_error::what() { return this->info; }

i2p_sam::errors::error_codes i2p_sam::errors::sam_error::category() { return this->error_code; }

i2p_sam::errors::sam_error::operator bool() {
    sam_error success_er;
    return (this->error_code != success_er.error_code);
}