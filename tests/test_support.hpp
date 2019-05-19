#pragma once

#include <stdexcept>

namespace test {

inline void require(bool condition, const char *message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

} // namespace test
