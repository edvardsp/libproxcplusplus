//          Copyright Edvard Severin Pettersen 2017.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE.md or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <chrono>
#include <thread>

#include <proxc/config.hpp>
#include <proxc.hpp>

#include "setup.hpp"

using namespace proxc;

void test_egg_timer()
{
    auto dur = std::chrono::microseconds( 100 );
    timer::Egg egg{ dur };
    throw_assert( ! egg.expired(), "timer should not be expired" );

    for ( std::size_t i = 0; i < 100; ++i ) {
        std::this_thread::sleep_for( dur );
        throw_assert( egg.expired(), "timer should be expired" );

        std::this_thread::sleep_for( dur );
        throw_assert( egg.expired(), "timer should keep being expired" );

        egg.reset();

        throw_assert( ! egg.expired(), "timer should not be expired after reset" );
        throw_assert( ! egg.expired(), "timer should still not be expired" );
    }
}

void test_repeat_timer()
{
    auto dur = std::chrono::microseconds( 1000 );
    timer::Repeat rep{ dur };

    for ( std::size_t i = 0; i < 100; ++i ) {
        throw_assert( ! rep.expired(), "timer should not be expired" );
        throw_assert( ! rep.expired(), "timer should not be expired" );
        std::this_thread::sleep_for( dur );
        throw_assert( rep.expired(), "timer should be expired" );
        throw_assert( ! rep.expired(), "timer set new time_point after expired" );
        throw_assert( ! rep.expired(), "timer set new time_point after expired" );

        std::this_thread::sleep_for( dur );
        throw_assert( rep.expired(), "timer should be expired" );
    }
}

void test_date_timer()
{
    auto tp = std::chrono::steady_clock::now() + std::chrono::milliseconds( 100 );
    timer::Date date{ tp };

    throw_assert( ! date.expired(), "timer should not be expired" );
    std::this_thread::sleep_until( tp );
    throw_assert(   date.expired(), "timer should be expired" );
    date.reset();
    throw_assert(   date.expired(), "reset should not do anything" );
    std::this_thread::sleep_until( tp );
    throw_assert(   date.expired(), "timer should keep being expired" );
}

int main()
{
    test_egg_timer();
    /* test_repeat_timer(); */
    test_date_timer();

    return 0;
}
