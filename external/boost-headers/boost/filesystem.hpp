#ifndef BOOST_FILESYSTEM_HPP
#define BOOST_FILESYSTEM_HPP

#include <string>

namespace boost {
    namespace filesystem {
        class path {
            std::string path_str;
        public:
            path() = default;
            path(const std::string& p) : path_str(p) {}
            path(const char* p) : path_str(p) {}
            std::string string() const { return path_str; }
        };
    }
}

#endif
