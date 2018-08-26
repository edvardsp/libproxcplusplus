//          Copyright Edvard Severin Pettersen 2017.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE.md or copy at
//          https://www.boost.org/LICENSE_1_0.txt)

#include <chrono>

#include <proxc/config.hpp>
#include <proxc.hpp>
#include <proxc/scheduling_policy/work_stealing.hpp>

#include "setup.hpp"

using namespace proxc;

using PolicyType = scheduling_policy::WorkStealing;
using WorkType = runtime::Context;
using ClockType = std::chrono::steady_clock;

void test_enqueuing_and_pick_next(PolicyType & policy)
{
    const std::size_t num_work = 1000;

    {
        auto work = policy.pick_next();
        throw_assert(work == nullptr, "work should be nullptr");
    }

    throw_assert( ! policy.is_ready(), "policy should not be ready" );

    policy.reserve(num_work);
    for (std::size_t i = 0; i < num_work; ++i) {
        policy.enqueue(new WorkType(runtime::context::work_type, [](void *){  }));
    }

    throw_assert(policy.is_ready(), "policy should be ready");

    for (std::size_t i = 0; i < num_work; ++i) {
        auto work = policy.pick_next();
        throw_assert(work != nullptr, "work should not be nullptr");
        delete work;
    }

    throw_assert( ! policy.is_ready(), "policy should not be ready");
}

void test_suspend_until(PolicyType & policy)
{
    using namespace std::chrono;
    using namespace std::chrono_literals;

    auto check_duration = [&policy](auto duration){
        auto start = ClockType::now();
        policy.suspend_until(ClockType::now() + duration);
        auto diff = ClockType::now() - start;
        throw_assert(diff > duration, "time suspended is not greater than duration");

        auto time_over = diff - duration;
        throw_assert(time_over < 300us, "time over suspension is greater than 200us => " << duration_cast<microseconds>(time_over).count() << "us");
    };

    check_duration(750us);
    check_duration(1000us);
    check_duration(1250us);
    check_duration(1500us);
    check_duration(1750us);
    check_duration(2000us);
}

int main()
{
    scheduling_policy::WorkStealing policy{};

    /* test_enqueuing_and_pick_next(policy); */
    /* test_suspend_until(policy); */

    return 0;
}
