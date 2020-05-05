#pragma once

#include <limits>

template <typename TO, typename FROM>
TO safe_narrow(const FROM &x) {
    if (x > std::numeric_limits<FROM>::max()) {
        throw std::bad_cast();
    }
    return static_cast<TO>(x);
}