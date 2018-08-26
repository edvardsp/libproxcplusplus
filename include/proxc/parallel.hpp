//          Copyright Edvard Severin Pettersen 2017.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE.md or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#pragma once

#include <iterator>
#include <type_traits>
#include <utility>

#include <proxc/config.hpp>

#include <proxc/process.hpp>
#include <proxc/runtime/context.hpp>
#include <proxc/runtime/scheduler.hpp>

#include <boost/intrusive_ptr.hpp>
#include <boost/range/irange.hpp>

PROXC_NAMESPACE_BEGIN
namespace detail {

template<typename P>
struct is_proc
    : std::integral_constant<
        bool,
        std::is_same<
            typename std::decay< P >::type,
            Process
        >::value
    >
{};

template<typename ...>
struct are_procs
    : std::true_type
{};

template<typename Proc, typename ... Procs>
struct are_procs< Proc, Procs ... >
    : std::integral_constant<
        bool,
           is_proc< Proc >::value
        && are_procs< Procs ... >{}
    >
{};

template<typename Proc>
auto parallel_impl( Proc && proc )
    -> std::enable_if_t<
           is_proc< Proc >::value
        && std::is_move_assignable< Proc >::value
    >
{
    Proc p = std::forward< Proc >( proc );
    p.launch();
    p.join();
}

template<typename Proc, typename ... Procs>
auto parallel_impl(Proc && proc, Procs && ... procs )
    -> std::enable_if_t<
           is_proc< Proc >::value
        && std::is_move_assignable< Proc >::value
    >
{
    Proc p = std::forward< Proc >( proc );
    p.launch();
    parallel_impl( std::forward< Procs >( procs ) ... );
    p.join();
}

} // namespace detail

template<typename ... Procs>
void parallel(Procs && ... procs)
{
    static_assert( detail::are_procs< Procs ... >::value,
        "Supplied arguments must be of type proxc::Process" );
    static_assert( detail::traits::are_move_assigneable< Procs ... >::value,
        "Supplied processes must be move assigneable" );
    detail::parallel_impl( std::forward< Procs >( procs ) ... );
}

PROXC_NAMESPACE_END

