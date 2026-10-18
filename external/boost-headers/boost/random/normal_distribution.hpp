#ifndef BOOST_RANDOM_NORMAL_DISTRIBUTION_HPP
#define BOOST_RANDOM_NORMAL_DISTRIBUTION_HPP

#include <random>

namespace boost {
    namespace random {
        template <typename RealType = double>
        using normal_distribution = std::normal_distribution<RealType>;
    }
}

#endif
