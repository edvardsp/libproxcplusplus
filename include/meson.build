#          Copyright Edvard Severin Pettersen 2017.
# Distributed under the Boost Software License, Version 1.0.
#    (See accompanying file LICENSE.md or copy at
#          http://www.boost.org/LICENSE_1_0.txt)

install_headers('proxc.hpp')
install_headers(
    'proxc/alt.hpp',
    'proxc/channel.hpp',
    'proxc/config.hpp',
    'proxc/parallel.hpp',
    'proxc/process.hpp',
    'proxc/timer.hpp',
    'proxc/this_proc.hpp',
    'proxc/version.hpp',
    subdir : 'proxc')
install_headers(
    'proxc/runtime/context.hpp',
    'proxc/runtime/scheduler.hpp',
    subdir : 'runtime')
install_headers(
    'proxc/alt/state.hpp',
    'proxc/alt/sync.hpp',
    'proxc/alt/choice_base.hpp',
    'proxc/alt/choice_send.hpp',
    'proxc/alt/choice_recv.hpp',
    subdir : 'proxc/alt')
install_headers(
    'proxc/channel/op_result.hpp',
    'proxc/channel/sync.hpp',
    'proxc/channel/tx.hpp',
    'proxc/channel/rx.hpp',
    'proxc/channel/op.hpp',
    subdir : 'proxc/channel')
install_headers(
    'proxc/scheduling_policy/policy_base.hpp',
    'proxc/scheduling_policy/round_robin.hpp',
    'proxc/scheduling_policy/work_stealing.hpp',
    subdir : 'proxc/scheduling_policy')
install_headers(
    'proxc/detail/circular_array.hpp',
    'proxc/detail/cpu_relax.hpp',
    'proxc/detail/delegate.hpp',
    'proxc/detail/hook.hpp',
    'proxc/detail/mpsc_queue.hpp',
    'proxc/detail/num_cpus.hpp',
    'proxc/detail/queue.hpp',
    'proxc/detail/spinlock.hpp',
    'proxc/detail/traits.hpp',
    'proxc/detail/work_steal_deque.hpp',
    'proxc/detail/xorshift.hpp',
    subdir : 'proxc/detail')
