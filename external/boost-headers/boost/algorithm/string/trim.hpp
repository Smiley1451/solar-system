#ifndef BOOST_ALGORITHM_STRING_TRIM_HPP
#define BOOST_ALGORITHM_STRING_TRIM_HPP

#include <string>
#include <algorithm>
#include <cctype>

namespace boost {
    namespace algorithm {
        inline void trim_left(std::string &s) {
            s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) { return !std::isspace(ch); }));
        }

        inline void trim_right(std::string &s) {
            s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) { return !std::isspace(ch); }).base(), s.end());
        }

        inline void trim(std::string &s) {
            trim_left(s);
            trim_right(s);
        }
    }
}

#endif
