//          Copyright Oliver Kowalke 2009.
//          Copyright Edvard Severin Pettersen 2017.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE.md or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <chrono>
#include <condition_variable>
#include <mutex>

#include <proxc/config.hpp>

#include <proxc/runtime/context.hpp>
#include <proxc/runtime/scheduler.hpp>

#include <proxc/scheduling_policy/policy_base.hpp>

PROXC_NAMESPACE_BEGIN

namespace scheduling_policy {
namespace detail {

template<typename T>
class RoundRobinPolicy : public PolicyBase<T>
{
private:
    runtime::Scheduler::ReadyQueue      ready_queue_{};
    std::mutex                          mtx_{};
    std::condition_variable             cnd_{};
    bool                                flag_{ false };

public:
    using TimePointT = typename PolicyBase<T>::TimePointT;

    RoundRobinPolicy() = default;

    RoundRobinPolicy(RoundRobinPolicy const &)             = delete;
    RoundRobinPolicy & operator=(RoundRobinPolicy const &) = delete;

    void enqueue(T *) noexcept;

    T * pick_next() noexcept;

    bool is_ready() const noexcept;

    void suspend_until(TimePointT const &) noexcept;

    void notify() noexcept;
};

} // namespace detail

using RoundRobin = detail::RoundRobinPolicy< runtime::Context >;

} // namespace scheduling_policy

PROXC_NAMESPACE_END

