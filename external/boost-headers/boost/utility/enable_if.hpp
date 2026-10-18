#ifndef BOOST_UTILITY_ENABLE_IF_HPP
#define BOOST_UTILITY_ENABLE_IF_HPP

#include <type_traits>

namespace boost {
    template <bool B, typename T = void>
    using enable_if_c = std::enable_if<B, T>;

    template <typename Cond, typename T = void>
    using enable_if = std::enable_if<Cond::value, T>;

    template <bool B, typename T = void>
    using disable_if_c = std::enable_if<!B, T>;

    template <typename Cond, typename T = void>
    using disable_if = std::enable_if<!Cond::value, T>;
}

#endif // BOOST_UTILITY_ENABLE_IF_HPP
