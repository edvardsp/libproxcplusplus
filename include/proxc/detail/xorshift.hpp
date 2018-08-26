//          Copyright Oliver Kowalke 2009.
//          Copyright Edvard Severin Pettersen 2017.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE.md or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <limits>
#include <random>

#include <proxc/config.hpp>

PROXC_NAMESPACE_BEGIN
namespace detail {

template<typename Integer>
class XorShift
{
private:
    Integer x_{ 123456789 }
          , y_{ 362436069 }
          , z_{ 521288629 }
          , w_{ std::random_device{}() };

    Integer random_() noexcept
    {
        Integer t = x_ ^ (x_ << Integer{ 11 });
        x_ = y_;
        y_ = z_;
        z_ = w_;
        return w_ = w_ ^ (w_ >> Integer{ 19 }) ^ (t ^ (t >> Integer{ 8 }));
    }

public:
    using result_type = Integer;

    XorShift() = default;
    ~XorShift() = default;

    constexpr Integer min() const noexcept
    {
        return std::numeric_limits< Integer >::min();
    }

    constexpr Integer max() const noexcept
    {
        return std::numeric_limits< Integer >::max();
    }

    Integer operator () () noexcept
    {
        return random_();
    }

};

} // namespace detail
PROXC_NAMESPACE_END

