//          Copyright Edvard Severin Pettersen 2017.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE.md or copy at
//          https://www.boost.org/LICENSE_1_0.txt)

#include <chrono>
#include <iostream>
#include <string>
#include <vector>

#include <proxc.hpp>

using namespace proxc;

constexpr int N = 6;

[[noreturn]]
void printer( std::array< Chan< std::string >::Rx, N > rxs )
{
    for ( ;; ) {
        Alt()
            .recv_for( rxs.begin(), rxs.end(),
                []( auto msg ){ std::cout << msg << std::endl; } )
            .select();
    }
}

[[noreturn]]
void a_fork( Chan< int >::Rx left, Chan< int >::Rx right )
{
    for ( ;; ) {
        Alt()
            .recv( left,  [&left] (auto) { left(); } )
            .recv( right, [&right](auto) { right(); } )
            .select();
    }
}

[[noreturn]]
void philosopher( Chan< std::string >::Tx report, int i,
                  Chan< int >::Tx left, Chan< int >::Tx right,
                  Chan< int >::Tx down, Chan< int >::Tx up )
{
    const std::chrono::seconds dur( i );
    const std::string id{ std::to_string( i ) };
    for ( ;; ) {
        report.send( id + " thinking" );
        this_proc::delay_for( dur );

        report.send( id + " hungry" );
        down << i;

        report.send( id + " sitting" );
        parallel(
            proc( [&left,i] { left << i; } ),
            proc( [&right,i]{ right << i; } )
        );

        report.send( id + " eating" );
        this_proc::delay_for( dur );

        report.send( id + " leaving" );
        parallel(
            proc( [&left,i] { left << i; } ),
            proc( [&right,i]{ right << i; } )
        );

        up << i;
    }
}

[[noreturn]]
void security( std::array< Chan< int >::Rx, N > down,
               std::array< Chan< int >::Rx, N > up )
{
    int sitting = 0;
    for ( ;; ) {
        Alt()
            .recv_for_if( sitting < N - 1,
                down.begin(), down.end(),
                [&sitting](auto){ ++sitting; } )
            .recv_for_if( sitting > 0,
                up.begin(), up.end(),
                [&sitting](auto){ --sitting; } )
            .select();
    }
}

int main()
{
    ChanArr< std::string, N > print_chs;
    ChanArr< int, N > left_chs;
    ChanArr< int, N > right_chs;
    ChanArr< int, N > up_chs;
    ChanArr< int, N > down_chs;

    std::vector< Process > philosophers;
    philosophers.reserve( N );
    for ( std::size_t i = 0; i < N; ++i ) {
        philosophers.emplace_back( philosopher,
            print_chs[i].move_tx(), i + 1,
            left_chs[i].move_tx(), right_chs[i].move_tx(),
            down_chs[i].move_tx(), up_chs[i].move_tx() );
    }

    std::vector< Process > forks;
    forks.reserve( N );
    for ( std::size_t i = 0; i < N; ++i ) {
        forks.emplace_back( a_fork,
            left_chs[i].move_rx(), right_chs[(i+1)%N].move_rx() );
    }

    parallel(
        proc_for( philosophers.begin(), philosophers.end() ),
        proc_for( forks.begin(), forks.end() ),
        proc( security, down_chs.collect_rx(), up_chs.collect_rx() ),
        proc( printer, print_chs.collect_rx() )
    );

    return  0;
}
