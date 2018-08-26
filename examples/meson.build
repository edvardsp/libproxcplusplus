#          Copyright Edvard Severin Pettersen 2017.
# Distributed under the Boost Software License, Version 1.0.
#    (See accompanying file LICENSE.md or copy at
#          http://www.boost.org/LICENSE_1_0.txt)

examples = [
    ['concurrent_sieve',      ['concurrent_sieve.cpp']],
    ['concurrent_fibonacci',  ['concurrent_fibonacci.cpp']],
    ['commstime',             ['commstime.cpp']],
    ['stressed_alt',          ['stressed_alt.cpp']],
    ['dining_philosophers',   ['dining_philosophers.cpp']],
    ['monte_carlo_pi',        ['monte_carlo_pi.cpp']],
    ['piping',                ['piping.cpp']],
    ['concurrent_mandelbrot', ['concurrent_mandelbrot.cpp']],
    ['concurrent_mandelbrot_nc', ['concurrent_mandelbrot_nc.cpp']]
]

foreach e : examples
    exe = executable(e[0], e[1],
        include_directories : [inc, src, exm],
        dependencies : [boost_dep, thread_dep],
        link_with : proxc)
endforeach

