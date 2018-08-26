//          Copyright Edvard Severin Pettersen 2017.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE.md or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <proxc/config.hpp>

PROXC_NAMESPACE_BEGIN
namespace channel {

enum class OpResult {
    Ok,
    Empty,
    Full,
    Timeout,
    Closed,
    Error,
};

enum class AltResult {
    Ok,
    NoEnd,
    Closed,
    SyncFailed,
    SelectFailed,
    TryLater,
};

} // namespace channel
PROXC_NAMESPACE_END

