//          Copyright Edvard Severin Pettersen 2017.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE.md or copy at
//          https://www.boost.org/LICENSE_1_0.txt)

#include <iostream>
#include <vector>

#include <proxc.hpp>

using namespace proxc;

using ItemT = std::size_t;

void generate( Chan< ItemT >::Tx out, Chan< ItemT >::Rx ex )
{
    ItemT i{ 2 };
    while ( ex ) {
        out << i++;
    }
}

void filter( Chan< ItemT >::Rx in, Chan< ItemT >::Tx out )
{
    ItemT prime = in();
    for ( auto i : in ) {
        if ( i % prime != 0 ) {
            out << i;
        }
    }
}

int main()
{
    constexpr std::size_t n = 100;

    Chan< ItemT >    ex_ch;
    ChanVec< ItemT > chs{ n };

    std::vector< Process > filters;
    filters.reserve( n - 1 );
    for ( std::size_t i = 0; i < n - 1; ++i ) {
        filters.emplace_back( filter, chs[i].move_rx(), chs[i+1].move_tx() );
    }

    this_proc::yield();

    auto start = std::chrono::steady_clock::now();

    parallel(
        proc( generate, chs[0].move_tx(), ex_ch.move_rx() ),
        proc_for( filters.begin(), filters.end() ),
        proc( [start,n]( Chan< ItemT >::Rx in, Chan< ItemT >::Tx ){
                in();
                auto end = std::chrono::steady_clock::now();
                std::chrono::duration< std::size_t, std::nano > diff = end - start;
                std::cout << n << "," << diff.count() / n << std::endl;
            },
            chs[n-1].move_rx(), ex_ch.move_tx() )
    );
    return 0;
}
