//          Copyright Oliver Kowalke 2009.
//          Copyright Edvard Severin Pettersen 2017.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE.md or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <chrono>

#include <proxc/config.hpp>

#include <proxc/process.hpp>
#include <proxc/runtime/scheduler.hpp>

PROXC_NAMESPACE_BEGIN
namespace this_proc {

Process::Id get_id() noexcept
{
    return runtime::Scheduler::running()->get_id();
}

void yield() noexcept
{
    runtime::Scheduler::self()->yield();
}

template<typename Rep, typename Period>
void delay_for( std::chrono::duration< Rep, Period > const & duration ) noexcept
{
    static_cast< void >(
        runtime::Scheduler::self()->sleep_until(
            std::chrono::steady_clock::now() + duration )
    );
}

template<typename Clock, typename Dur>
void delay_until( std::chrono::time_point< Clock, Dur > const & time_point ) noexcept
{
    static_cast< void >(
        runtime::Scheduler::self()->sleep_until( time_point )
    );
}

} // namespace this_proc
PROXC_NAMESPACE_END

