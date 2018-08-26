//          Copyright Edvard Severin Pettersen 2017.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE.md or copy at
//          https://www.boost.org/LICENSE_1_0.txt)

#include <array>
#include <chrono>
#include <iostream>
#include <vector>

#include <proxc.hpp>

using namespace proxc;

constexpr std::size_t NUM = 200;

void writer( Chan< int >::Tx tx )
{
    int n = 0;
    while ( ! tx.is_closed() ) {
        tx << n++;
    }
}

void reader( std::array< Chan< int >::Rx, NUM > rxs )
{
    constexpr std::size_t num_runs = 100;
    constexpr std::size_t num_iter = 1000;
    double total_us = 0.;

    timer::Egg egg{ std::chrono::milliseconds( 1 ) };
    for ( std::size_t run = 0; run < num_runs; ++run ) {
        auto start = std::chrono::steady_clock::now();
        for ( std::size_t iter = 0; iter < num_iter; ++iter ) {
            Alt()
                .recv_for( rxs.begin(), rxs.end(),
                    []( int ){} )
                .select();
        }
        auto end = std::chrono::steady_clock::now();
        std::chrono::duration< double, std::micro > diff = end - start;
        total_us += diff.count();
    }
    std::cout << "Avg. us per alt: " << total_us / num_runs / num_iter << "us" << std::endl;
}

int main()
{
    ChanArr< int, NUM > chs;

    std::vector< Process > writers;
    writers.reserve( NUM );
    for ( std::size_t i = 0; i < NUM; ++i ) {
        writers.emplace_back( writer, chs[i].move_tx() );
    }

    parallel(
        proc_for( writers.begin(), writers.end() ),
        proc( reader, chs.collect_rx() )
    );

    return 0;
}
