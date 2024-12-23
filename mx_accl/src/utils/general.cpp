#include <memx/accl/utils/general.h>

void MX::Utils::mx_checkandthrow(MX::Utils::mx_retval ret){
    if(!ret.error_flag){
        throw std::runtime_error(ret.error_msg);
    }
}

void MX::Utils::mx_checkandprint(MX::Utils::mx_retval ret){
    if(!ret.error_flag){
        std::cerr<<ret.error_msg<<"\n";
    }
}
