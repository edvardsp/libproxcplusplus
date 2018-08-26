//          Copyright Oliver Kowalke 2009.
//          Copyright Edvard Severin Pettersen 2017.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE.md or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <proxc/config.hpp>

#include <proxc/runtime/context.hpp>
#include <proxc/runtime/scheduler.hpp>

#include <proxc/scheduling_policy/round_robin.hpp>

#include <proxc/detail/hook.hpp>

#include <boost/assert.hpp>

PROXC_NAMESPACE_BEGIN
namespace scheduling_policy {
namespace detail {

namespace hook = proxc::detail::hook;

template<>
void RoundRobin::enqueue( runtime::Context * ctx ) noexcept
{
    BOOST_ASSERT(ctx != nullptr);
    ctx->link(ready_queue_);
}

template<>
runtime::Context * RoundRobin::pick_next() noexcept
{
    runtime::Context * next = nullptr;
    if ( ! ready_queue_.empty() ) {
        next = &ready_queue_.front();
        ready_queue_.pop_front();
        BOOST_ASSERT(next != nullptr);
        BOOST_ASSERT( ! next->is_linked< hook::Ready >());
    }
    return next;
}

template<>
bool RoundRobin::is_ready() const noexcept
{
    return ! ready_queue_.empty();
}

template<>
void RoundRobin::suspend_until(TimePointT const & time_point) noexcept
{
    std::unique_lock<std::mutex> lk{ mtx_ };
    cnd_.wait_until(lk, time_point,
        [&](){ return flag_; });
    flag_ = false;
}

template<>
void RoundRobin::notify() noexcept
{
    std::unique_lock<std::mutex> lk{ mtx_ };
    flag_ = true;
    lk.unlock();
    cnd_.notify_all();
}

} // namespace detail
} // namespace scheduling_policy
PROXC_NAMESPACE_END

