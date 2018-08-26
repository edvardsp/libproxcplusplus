//          Copyright Edvard Severin Pettersen 2017.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE.md or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <atomic>

#include <proxc/config.hpp>

PROXC_NAMESPACE_BEGIN
namespace alt {

struct Sync
{
    enum class State
    {
        None,
        Offered,
        Accepted,
    };
    std::atomic< State >    state_{ State::None };
    Sync() = default;
};

} // namespace alt
PROXC_NAMESPACE_END

