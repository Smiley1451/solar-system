#ifndef BOOST_FILESYSTEM_PATH_HPP
#define BOOST_FILESYSTEM_PATH_HPP

#include <string>

namespace boost {
    namespace filesystem {
        class path {
            std::string path_str;
        public:
            path() = default;
            path(const std::string& p) : path_str(p) {}
            path(const char* p) : path_str(p ? p : "") {}
            const std::string& string() const { return path_str; }
            const char* c_str() const { return path_str.c_str(); }
        };
    }
}

#endif
