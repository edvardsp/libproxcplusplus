#          Copyright Edvard Severin Pettersen 2017.
# Distributed under the Boost Software License, Version 1.0.
#    (See accompanying file LICENSE.md or copy at
#          http://www.boost.org/LICENSE_1_0.txt)

proxc_sources = [
    'alt.cpp',
    'context.cpp',
    'process.cpp',
    'scheduler.cpp',
    'scheduling_policy/round_robin.cpp',
    'scheduling_policy/work_stealing.cpp'
]

proxc = shared_library('proxc',
    proxc_sources,
    include_directories : inc,
    dependencies : [thread_dep, boost_dep],
    install : true)

