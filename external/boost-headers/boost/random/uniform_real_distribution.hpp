#ifndef BOOST_RANDOM_UNIFORM_REAL_DISTRIBUTION_HPP
#define BOOST_RANDOM_UNIFORM_REAL_DISTRIBUTION_HPP

#include <random>

namespace boost {
    namespace random {
        template <typename RealType = double>
        using uniform_real_distribution = std::uniform_real_distribution<RealType>;
    }
}

#endif
