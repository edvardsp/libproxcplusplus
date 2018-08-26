//          Copyright Edvard Severin Pettersen 2017.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE.md or copy at
//          https://www.boost.org/LICENSE_1_0.txt)

#include <iostream>
#include <vector>

#include <proxc.hpp>

using namespace proxc;

constexpr std::size_t N = 100;

void elem( Chan< int >::Rx in, Chan< int >::Tx out )
{
    while ( in >> out );
}

int main()
{
    ChanArr< int, N > chs;

    std::vector< Process > elems;
    for ( std::size_t i = 0; i < N; ++i ) {
        elems.emplace_back( elem, chs[i].move_rx(), chs[i+1].move_tx() );
    }

    parallel(
        proc_for( elems.begin(), elems.end() ),
        proc( []( Chan< int >::Tx start, Chan< int >::Rx end ){
                int start_item = 1337;
                start << start_item;
                for ( std::size_t i = 0; i < 1000; ++i ) {
                    end >> start;
                }
                int end_item{};
                end >> end_item;

                std::cout << "Start item: " << start_item << std::endl;
                std::cout << "End item: " << end_item << std::endl;
            },
            chs[0].move_tx(), chs[N].move_rx() )
    );

    return 0;
}

