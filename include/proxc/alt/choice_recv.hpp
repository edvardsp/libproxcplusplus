//          Copyright Edvard Severin Pettersen 2017.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE.md or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <proxc/config.hpp>

#include <proxc/channel.hpp>
#include <proxc/runtime/context.hpp>
#include <proxc/alt/choice_base.hpp>
#include <proxc/detail/delegate.hpp>

#include <boost/assert.hpp>

PROXC_NAMESPACE_BEGIN
namespace alt {

template<typename T>
class ChoiceRecv : public ChoiceBase
{
public:
    using ItemT = typename std::remove_cv< typename std::decay< T >::type >::type;
    using RxT   = channel::Rx< ItemT >;
    using EndT  = channel::detail::ChanEnd< ItemT >;
    using FnT   = proxc::detail::delegate< void( ItemT ) >;

private:
    RxT &    rx_;
    ItemT    item_;
    EndT     end_;
    FnT      fn_;

public:
    ChoiceRecv( Alt *              alt,
                runtime::Context * ctx,
                RxT &              rx,
                FnT                fn )
        : ChoiceBase{ alt }
        , rx_{ rx }
        , item_{}
        , end_{ ctx, item_, this }
        , fn_{ std::move( fn ) }
    {}

    ~ChoiceRecv() {}

    void enter() noexcept
    {
        rx_.alt_enter( end_ );
    }

    void leave() noexcept
    {
        rx_.alt_leave();
    }

    bool is_ready() const noexcept
    {
        return rx_.alt_ready();
    }

    Result try_complete() noexcept
    {
        auto res = rx_.alt_recv();
        switch ( res ) {
        case channel::AltResult::Ok:       return Result::Ok;
        case channel::AltResult::TryLater: return Result::TryLater;
        default:                           return Result::Failed;
        }
    }

    void run_func() noexcept
    {
        if ( fn_ ) {
            fn_( std::move( item_ ) );
        }
    }
};

} // namespace alt
PROXC_NAMESPACE_END


