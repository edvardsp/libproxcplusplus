//          Copyright Edvard Severin Pettersen 2017.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE.md or copy at
//          https://www.boost.org/LICENSE_1_0.txt)

#include <algorithm>
#include <chrono>
#include <iostream>

#include <proxc.hpp>

using namespace proxc;

using ItemT = std::size_t;
using ChanT = Chan< ItemT >;

constexpr std::size_t REPEAT = 100;
constexpr std::size_t RUNS = 50;

void chainer( ChanT::Rx in, ChanT::Tx out )
{
    for ( auto i : in ) {
        out << i + 1;
    }
}

void prefix( ChanT::Rx in, ChanT::Tx out )
{
    out << std::size_t{ 0 };
    for ( auto i : in ) {
        out << i;
    }
}

void delta( ChanT::Rx in, ChanT::Tx out, ChanT::Tx out_consume )
{
    for ( auto i : in ) {
        if ( out_consume << i ) {
            out << i;
        } else {
            break;
        }
    }
}

void consumer( ChanT::Rx in )
{
    for ( std::size_t i = 0; i < REPEAT; ++i ) {
        in();
    }
}

void commstime( std::size_t chain )
{
    std::size_t sum = 0;
    for ( std::size_t run = 0; run < RUNS; ++run ) {
        ChanT pre2del_ch, consume_ch;
        ChanVec< ItemT > chain_chs{ chain + 1 };

        std::vector< Process > chains;
        chains.reserve( chain );
        for ( std::size_t i = 0; i < chain; ++i ) {
            chains.emplace_back( chainer,
                chain_chs[i+1].move_rx(),
                chain_chs[i].move_tx() );
        }

        auto start = std::chrono::system_clock::now();

        parallel(
            proc( prefix, chain_chs[0].move_rx(), pre2del_ch.move_tx() ),
            proc( delta, pre2del_ch.move_rx(), chain_chs[chain].move_tx(), consume_ch.move_tx() ),
            proc( consumer, consume_ch.move_rx() ),
            proc_for( chains.begin(), chains.end() )
        );

        auto end = std::chrono::system_clock::now();
        std::chrono::duration< std::size_t, std::nano > diff = end - start;
        sum += diff.count();
    }
    std::cout << chain << "," << sum / RUNS << std::endl;
}

int main()
{
    constexpr std::size_t chain = 100;
    commstime( chain );
    return 0;
}

