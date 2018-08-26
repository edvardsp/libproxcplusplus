//          Copyright Edvard Severin Pettersen 2017.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE.md or copy at
//          https://www.boost.org/LICENSE_1_0.txt)

#include <array>
#include <chrono>
#include <iostream>
#include <random>
#include <vector>

#include <proxc.hpp>

using namespace proxc;

constexpr std::size_t NUM_WORKERS = 8;
constexpr std::size_t NUM_ITERS = ~std::size_t{ 0 } - std::size_t{ 1 };

void monte_carlo_pi( Chan< double >::Tx out, std::size_t iters ) noexcept
{
    std::mt19937 rng{ std::random_device{}() };
    std::uniform_real_distribution< double > distr{ 0., 1. };

    std::size_t in_circle = 0;
    for ( std::size_t i = 0; i < iters; ++i ) {
        auto x = distr( rng );
        auto y = distr( rng );
        auto length = x * x + y * y;
        if ( length <= 1. ) {
            ++in_circle;
        }
        this_proc::yield();
    }
    out << ( 4. * in_circle ) / static_cast< double >( iters );
}

void calculate( std::array< Chan< double >::Rx, NUM_WORKERS > in, std::size_t workers ) noexcept
{
    double sum = 0.;
    std::for_each( in.begin(), in.end(),
        [&sum]( auto& rx ){ sum += rx(); });
    sum /= static_cast< double >( workers );
    std::cout << "pi: " << sum << std::endl;
}

int main()
{
    constexpr std::size_t iter_work = NUM_ITERS / NUM_WORKERS;
    ChanArr< double, NUM_WORKERS > chs;

    std::vector< Process > workers;
    workers.reserve( NUM_WORKERS );
    for ( std::size_t i = 0; i < NUM_WORKERS; ++i ) {
        workers.emplace_back( monte_carlo_pi, chs[i].move_tx(), iter_work );
    }

    parallel(
        proc_for( workers.begin(), workers.end() ),
        proc( calculate, chs.collect_rx(), NUM_WORKERS )
    );

    return 0;
}

