#ifndef BOOST_ALGORITHM_STRING_HPP
#define BOOST_ALGORITHM_STRING_HPP

#include <string>
#include <algorithm>
#include <cctype>
#include "string/trim.hpp"

namespace boost {
    namespace algorithm {
        inline std::string trim_copy(const std::string &s) {
            std::string res = s;
            trim(res);
            return res;
        }

        inline void replace_all(std::string &str, const std::string &from, const std::string &to) {
            if (from.empty()) return;
            size_t start_pos = 0;
            while ((start_pos = str.find(from, start_pos)) != std::string::npos) {
                str.replace(start_pos, from.length(), to);
                start_pos += to.length();
            }
        }
    }
    using algorithm::trim_copy;
    using algorithm::replace_all;
}

#endif
