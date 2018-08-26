//          Copyright Oliver Kowalke 2009.
//          Copyright Edvard Severin Pettersen 2017.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE.md or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <iterator>
#include <tuple>
#include <type_traits>
#include <utility>

#include <proxc/config.hpp>

#include <proxc/runtime/context.hpp>
#include <proxc/runtime/scheduler.hpp>
#include <proxc/detail/apply.hpp>
#include <proxc/detail/delegate.hpp>
#include <proxc/detail/traits.hpp>

#include <boost/intrusive_ptr.hpp>
#include <boost/range/irange.hpp>

PROXC_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////
// Process definition
////////////////////////////////////////////////////////////////////////////////

class Process
{
private:
    using CtxPtr = boost::intrusive_ptr< runtime::Context >;

    CtxPtr    ctx_{};

public:
    using Id = runtime::Context::Id;

    Process() = default;

    template< typename Fn
            , typename ... Args
            , typename
    >
    Process( Fn && fn, Args && ... args );

    ~Process() noexcept;

    // make non-copyable
    Process( Process const & ) = delete;
    Process & operator = ( Process const & ) = delete;

    // make moveable
    Process( Process && ) noexcept;
    Process & operator = ( Process && ) noexcept;

    void swap( Process & other ) noexcept;
    Id get_id() const noexcept;

    void launch() noexcept;
    void join();
    void detach();
};

////////////////////////////////////////////////////////////////////////////////
// Process implementation
////////////////////////////////////////////////////////////////////////////////

template< typename Fn
        , typename ... Args
        , typename = std::enable_if_t<
            detail::traits::are_callable_with_arg< Fn( Args ... ) >::value
        >
>
Process::Process( Fn && fn, Args && ... args )
    : ctx_{ runtime::Scheduler::make_work(
        std::forward< Fn >( fn ),
        std::forward< Args >( args ) ...
    ) }
{
}

////////////////////////////////////////////////////////////////////////////////
// Process methods
////////////////////////////////////////////////////////////////////////////////

template<typename Fn, typename ... Args>
Process proc( Fn &&, Args && ... );

namespace detail {

template< typename InputIt,
          typename ... Fns
>
auto proc_for_impl( InputIt first, InputIt last, Fns && ... fns )
    -> std::enable_if_t<
        detail::traits::is_inputiterator< InputIt >{}
    >
{
    static_assert( detail::traits::are_callable_with_arg< typename InputIt::value_type, Fns ... >{},
        "Supplied functions does not have the correct function signature" );

    using FnT = detail::delegate< void( typename InputIt::value_type ) >;
    using ArrT = std::array< FnT, sizeof...(Fns) >;

    ArrT fn_arr{ { std::forward< Fns >( fns ) ... } };
    const std::size_t total_size = sizeof...(Fns) * std::distance( first, last );

    std::vector< Process > procs;
    procs.reserve( total_size );
    for ( auto data = first; data != last; ++data ) {
        for ( const auto& fn : fn_arr ) {
            procs.emplace_back( fn, *data );
            procs.back().launch();
        }
        runtime::Scheduler::self()->yield();
    }
    std::for_each( procs.begin(), procs.end(),
        []( auto& proc ){ proc.join(); } );
}

template< typename Input,
          typename ... Fns
>
auto proc_for_impl( Input first, Input last, Fns && ... fns )
    -> std::enable_if_t<
        std::is_integral< Input >{}
    >
{
    auto iota = boost::irange( first, last );
    proc_for_impl( iota.begin(), iota.end(), std::forward< Fns >( fns ) ... );
}

} // namespace detail

template< typename Fn,
          typename ... Args
>
Process proc( Fn && fn, Args && ... args )
{
    static_assert( detail::traits::are_callable_with_arg< Fn( Args ... ) >::value,
        "Supplied function is not callable with the given arguments");

    return Process{
        std::forward< Fn >( fn ),
        std::forward< Args >( args ) ...
    };
}

template< typename Input,
          typename ... Fns
>
Process proc_for( Input first, Input last, Fns && ... fns )
{
    return Process{
        & detail::proc_for_impl< Input, Fns ... >,
        first, last,
        std::forward< Fns >( fns ) ...
    };
}

template< typename InputIt
        , typename = std::enable_if_t<
            std::is_same<
                Process,
                typename std::decay<
                    typename std::iterator_traits< InputIt>::value_type
                >::type
            >::value
        > >
Process proc_for( InputIt first, InputIt last )
{
    return Process{
        [first,last]{
            std::for_each( first, last, std::mem_fn( & Process::launch ) );
            std::for_each( first, last, std::mem_fn( & Process::join ) );
        }
    };
}

PROXC_NAMESPACE_END

