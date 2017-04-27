
#pragma once

#include <iostream>

#include <atomic>
#include <chrono>
#include <mutex>
#include <vector>

#include <proxc/config.hpp>

#include <proxc/context.hpp>
#include <proxc/scheduler.hpp>
#include <proxc/spinlock.hpp>
#include <proxc/alt/sync.hpp>
#include <proxc/alt/choice_base.hpp>
#include <proxc/channel/op_result.hpp>

#include <boost/assert.hpp>

PROXC_NAMESPACE_BEGIN

namespace alt {

// forward declaration
class ChoiceBase;

} // namespace alt

namespace channel {
namespace detail {

template<typename ItemT>
struct alignas(cache_alignment) ChanEnd
{
    Context *            ctx_;
    ItemT &              item_;
    alt::ChoiceBase *    alt_choice_;
    ChanEnd( Context * ctx,
             ItemT & item,
             alt::ChoiceBase * alt_choice = nullptr )
        : ctx_{ ctx }
        , item_{ item }
        , alt_choice_{ alt_choice }
    {}
};

////////////////////////////////////////////////////////////////////////////////
// ChannelImpl
////////////////////////////////////////////////////////////////////////////////

template<typename T>
class ChannelImpl
{
public:
    using ItemT   = T;
    using EndT    = ChanEnd< ItemT >;
    using ChoiceT = alt::ChoiceBase;

private:
    Spinlock    splk_{};

    alignas(cache_alignment) std::atomic< bool >    closed_{ false };

    alignas(cache_alignment) std::atomic< EndT * >    tx_end_{ nullptr };
    alignas(cache_alignment) std::atomic< bool >      tx_consumed_{ false };

    alignas(cache_alignment) std::atomic< EndT * >    rx_end_{ nullptr };
    alignas(cache_alignment) std::atomic< bool >      rx_consumed_{ false };

    alignas(cache_alignment) alt::Sync    alt_sync_{};


public:
    ChannelImpl() = default;
    ~ChannelImpl() noexcept;

    // make non-copyable
    ChannelImpl( ChannelImpl const & )               = delete;
    ChannelImpl & operator = ( ChannelImpl const & ) = delete;

    bool is_closed() const noexcept;
    void close() noexcept;

    bool has_tx_() const noexcept;
    bool has_rx_() const noexcept;

    // normal send and receieve methods
    OpResult send( EndT & ) noexcept;

    OpResult recv( EndT & ) noexcept;

    // send and receive methods with duration timeout
    template<typename Rep, typename Period>
    OpResult send_for( EndT &,
                       std::chrono::duration< Rep, Period > const & ) noexcept;

    template<typename Rep, typename Period>
    OpResult recv_for( EndT &,
                       std::chrono::duration< Rep, Period > const & ) noexcept;

    // send and receive methods with timepoint timeout
    template<typename Clock, typename Dur>
    OpResult send_until( EndT &,
                         std::chrono::time_point< Clock, Dur > const & ) noexcept;

    template<typename Clock, typename Dur>
    OpResult recv_until( EndT &,
                         std::chrono::time_point< Clock, Dur > const & ) noexcept;

    // send and receive methods for alting
    void alt_send_enter( EndT & ) noexcept;
    void alt_send_leave() noexcept;
    bool alt_send_ready() noexcept;
    AltResult alt_send() noexcept;
    AltResult alt_send_blocking( std::unique_lock< Spinlock > & ) noexcept;
    AltResult alt_send_nonblocking( std::unique_lock< Spinlock > & ) noexcept;

    void alt_recv_enter( EndT & ) noexcept;
    void alt_recv_leave() noexcept;
    bool alt_recv_ready() noexcept;
    AltResult alt_recv() noexcept;
    AltResult alt_recv_blocking( std::unique_lock< Spinlock > & ) noexcept;
    AltResult alt_recv_nonblocking( std::unique_lock< Spinlock > & ) noexcept;
};

////////////////////////////////////////////////////////////////////////////////
// ChannelImpl public methods
////////////////////////////////////////////////////////////////////////////////

template<typename T>
ChannelImpl< T >::~ChannelImpl< T >() noexcept
{
    close();

    BOOST_ASSERT( ! has_tx_() );
    BOOST_ASSERT( ! has_rx_() );
}

template<typename T>
bool ChannelImpl< T >::is_closed() const noexcept
{
    return closed_.load( std::memory_order_acquire );
}

template<typename T>
void ChannelImpl< T >::close() noexcept
{
    std::unique_lock< Spinlock > lk{ splk_ };
    /* std::cout << "chan closing\n"; */
    closed_.store( true, std::memory_order_release );
    EndT * tx = tx_end_.exchange( nullptr, std::memory_order_acq_rel );
    if ( tx != nullptr ) {
        ChoiceT * alt_choice = tx->alt_choice_;
        if ( alt_choice != nullptr ) {
            alt_choice->maybe_wakeup();
        } else {
            /* std::cout << "closing: waking up tx\n"; */
            Scheduler::self()->schedule( tx->ctx_ );
        }
    }
    EndT * rx = rx_end_.exchange( nullptr, std::memory_order_acq_rel );
    if ( rx != nullptr ) {
        ChoiceT * alt_choice = rx->alt_choice_;
        if ( alt_choice != nullptr ) {
            alt_choice->maybe_wakeup();
        } else {
            /* std::cout << "closing: waking up rx\n"; */
            Scheduler::self()->schedule( rx->ctx_ );
        }
    }
}

template<typename T>
bool ChannelImpl< T >::has_tx_() const noexcept
{
    return tx_end_.load( std::memory_order_relaxed ) != nullptr;
}

template<typename T>
bool ChannelImpl< T >::has_rx_() const noexcept
{
    return rx_end_.load( std::memory_order_relaxed ) != nullptr;
}

template<typename T>
OpResult ChannelImpl< T >::send( EndT & tx ) noexcept
{
    BOOST_ASSERT( ! has_tx_() );
    BOOST_ASSERT( ! tx_consumed_.load( std::memory_order_relaxed ) );

    std::unique_lock< Spinlock > lk{ splk_ };

    if ( is_closed() ) {
        return OpResult::Closed;
    }

    EndT * rx = rx_end_.exchange( nullptr, std::memory_order_acq_rel );
    if ( rx != nullptr ) {
        ChoiceT * alt_choice = rx->alt_choice_;
        if ( alt_choice == nullptr || alt_choice->try_select() ) {
            rx->item_ = std::move( tx.item_ );
            rx_consumed_.store( true, std::memory_order_release );
            Scheduler::self()->schedule( rx->ctx_ );
            /* std::cout << "tx consumed, moving on\n"; */
            return OpResult::Ok;
        }
    }

    tx_end_.store( std::addressof( tx ), std::memory_order_release );
    /* std::cout << "tx waiting\n"; */
    Scheduler::self()->wait( lk );
    /* std::cout << "tx wokeup\n"; */
    return ( tx_consumed_.exchange( false, std::memory_order_acq_rel ) )
        ? OpResult::Ok
        : OpResult::Closed ;
}

template<typename T>
OpResult ChannelImpl< T >::recv( EndT & rx ) noexcept
{
    BOOST_ASSERT( ! has_rx_() );
    BOOST_ASSERT( ! rx_consumed_.load( std::memory_order_relaxed ) );

    std::unique_lock< Spinlock > lk{ splk_ };

    if ( is_closed() ) {
        return OpResult::Closed;
    }

    EndT * tx = tx_end_.exchange( nullptr, std::memory_order_acq_rel );
    if ( tx != nullptr ) {
        ChoiceT * alt_choice = tx->alt_choice_;
        if ( alt_choice == nullptr || alt_choice->try_select() ) {
            rx.item_ = std::move( tx->item_ );
            tx_consumed_.store( true, std::memory_order_release );
            Scheduler::self()->schedule( tx->ctx_ );
            /* std::cout << "rx consumed, moving on\n"; */
            return OpResult::Ok;
        }
    }

    rx_end_.store( std::addressof( rx ), std::memory_order_release );
    /* std::cout << "rx waiting\n"; */
    Scheduler::self()->wait( lk );
    /* std::cout << "rx wokeup\n"; */
    return ( rx_consumed_.exchange( false, std::memory_order_acq_rel ) )
        ? OpResult::Ok
        : OpResult::Closed ;
}

template<typename T>
template<typename Rep, typename Period>
OpResult ChannelImpl< T >::send_for(
    EndT & tx,
    std::chrono::duration< Rep, Period > const & duration
) noexcept
{
    auto timepoint = std::chrono::steady_clock::now() + duration;
    return send_until( tx, timepoint );
}

template<typename T>
template<typename Clock, typename Dur>
OpResult ChannelImpl< T >::send_until(
    EndT & tx,
    std::chrono::time_point< Clock, Dur > const & time_point
) noexcept
{
    BOOST_ASSERT( ! has_tx_() );
    BOOST_ASSERT( ! tx_consumed_.load( std::memory_order_relaxed ) );

    std::unique_lock< Spinlock > lk{ splk_ };

    if ( is_closed() ) {
        return OpResult::Closed;
    }

    EndT * rx = rx_end_.exchange( nullptr, std::memory_order_acq_rel );
    if ( rx != nullptr ) {
        ChoiceT * alt_choice = rx->alt_choice_;
        if ( alt_choice == nullptr || alt_choice->try_select() ) {
            rx->item_ = std::move( tx.item_ );
            rx_consumed_.store( true, std::memory_order_release );
            Scheduler::self()->schedule( rx->ctx_ );
            return OpResult::Ok;
        }
    }

    tx_end_.store( std::addressof( tx ), std::memory_order_release );
    bool timeout = Scheduler::self()->wait_until( time_point, lk, true );
    tx_end_.store( nullptr, std::memory_order_release );

    if ( tx_consumed_.exchange( false, std::memory_order_acq_rel ) ) {
        return OpResult::Ok;
    } else if ( timeout ) {
        return OpResult::Timeout;
    } else {
        return OpResult::Closed;
    }
}

template<typename T>
template<typename Rep, typename Period>
OpResult ChannelImpl< T >::recv_for(
    EndT & rx,
    std::chrono::duration< Rep, Period > const & duration
) noexcept
{
    auto timepoint = std::chrono::steady_clock::now() + duration;
    return recv_until( rx, timepoint );
}

template<typename T>
template<typename Clock, typename Dur>
OpResult ChannelImpl< T >::recv_until(
    EndT & rx,
    std::chrono::time_point< Clock, Dur > const & time_point
) noexcept
{
    BOOST_ASSERT( rx_end_.load( std::memory_order_relaxed ) == nullptr );

    std::unique_lock< Spinlock > lk{ splk_ };

    if ( is_closed() ) {
        return OpResult::Closed;
    }

    EndT * tx = tx_end_.exchange( nullptr, std::memory_order_acq_rel );
    if ( tx != nullptr ) {
        ChoiceT * alt_choice = tx->alt_choice_;
        if ( alt_choice == nullptr || alt_choice->try_select() ) {
            rx.item_ = std::move( tx->item_ );
            tx_consumed_.store( true, std::memory_order_release );
            Scheduler::self()->schedule( tx->ctx_ );
            return OpResult::Ok;
        }
    }

    rx_end_.store( std::addressof( rx ), std::memory_order_release );
    bool timeout = Scheduler::self()->wait_until( time_point, lk, true );
    rx_end_.store( nullptr, std::memory_order_release );

    if ( rx_consumed_.exchange( false, std::memory_order_acq_rel ) ) {
        return OpResult::Ok;
    } else if ( timeout ) {
        return OpResult::Timeout;
    } else {
        return OpResult::Closed;
    }
}

template<typename T>
void ChannelImpl< T >::alt_send_enter( EndT & tx ) noexcept
{
    BOOST_ASSERT( tx_end_.load( std::memory_order_relaxed ) == nullptr );
    BOOST_ASSERT( ! tx_consumed_.load( std::memory_order_relaxed ) );
    std::unique_lock< Spinlock > lk{ splk_ };
    tx_end_.store( std::addressof( tx ), std::memory_order_release );
}

template<typename T>
void ChannelImpl< T >::alt_send_leave() noexcept
{
    auto offered = alt::Sync::State::Offered;
    alt_sync_.state_.compare_exchange_strong(
        offered,
        alt::Sync::State::None,
        std::memory_order_acq_rel,
        std::memory_order_relaxed
    );
    std::unique_lock< Spinlock > lk{ splk_ };
    tx_end_.store( nullptr, std::memory_order_release );
    tx_consumed_.store( false, std::memory_order_release );
}

template<typename T>
bool ChannelImpl< T >::alt_send_ready() noexcept
{
    // no spinlock
    if ( is_closed() ) {
        return false;
    }
    if ( rx_end_.load( std::memory_order_acquire ) != nullptr ) {
        return true;
    } else {
        return false;
    }
}

template<typename T>
AltResult ChannelImpl< T >::alt_send() noexcept
{
    BOOST_ASSERT( has_tx_() );

    const auto offered  = alt::Sync::State::Offered;
    const auto accepted = alt::Sync::State::Accepted;
    if ( offered == alt_sync_.state_.load( std::memory_order_acquire ) ) {
        // Rx is alting and offering sync. Rx is currently spinning,
        // waiting for alt_sync state to change. Meanwhile, complete
        // the channel operation and signal the sync has been accepted.
        EndT * tx = tx_end_.exchange( nullptr, std::memory_order_release );
        EndT * rx = rx_end_.exchange( nullptr, std::memory_order_release );
        rx->item_ = std::move( tx->item_ );
        alt_sync_.state_.store( accepted, std::memory_order_release );
        return AltResult::Ok;
    }

    std::unique_lock< Spinlock > lk{ splk_ };
    if ( is_closed() ) {
        return AltResult::Closed;
    }

    EndT * rx = rx_end_.load( std::memory_order_acquire );
    if ( rx == nullptr ) {
        return AltResult::NoEnd;
    }

    EndT * tx = rx_end_.load( std::memory_order_acquire );
    ChoiceT * rx_choice = rx->alt_choice_;
    if ( rx_choice == nullptr ) {
        tx_end_.store( nullptr, std::memory_order_release );
        rx_end_.store( nullptr, std::memory_order_release );
        rx->item_ = std::move( tx->item_ );
        rx_consumed_.store( true, std::memory_order_release );
        Scheduler::self()->schedule( rx->ctx_ );
        return AltResult::Ok;
    }
    // else, Alting Rx, not offering sync
    ChoiceT * tx_choice = tx->alt_choice_;
    BOOST_ASSERT( tx_choice != nullptr );

    const auto none     = alt::Sync::State::None;
    const auto offered  = alt::Sync::State::Offered;
    const auto accepted = alt::Sync::State::Accepted;
    if ( *tx_choice < *rx_choice ) {
        // offer sync. if sync accepted, rx completed
        // channel operation. if rejected, channel
        // operation failed.
        alt_sync_.state_.store( offered, std::memory_order_release );
        /* lk.unlock(); */
        while ( offered == alt_sync_.state_.load( std::memory_order_acquire ) )
            { /* spin */ }
        return accepted == alt_sync_.state_.exchange( none, std::memory_order_release );

    } else {
        // check and accept sync.
        // if rx_choice is in checking state, and sync
        // not offered, try later. if sync offered,
        // complete channel operation and accept.
        // if in waiting state, try normal selecting.
        // if in done state, channel operation failed.
    }
}

template<typename T>
AltResult ChannelImpl< T >::alt_send_blocking( std::unique_lock< Spinlock > & lk ) noexcept
{
    lk.lock();

    if ( is_closed() ) {
        return AltResult::Closed;
    }

    EndT * rx = rx_end_.load( std::memory_order_acquire );
    if ( rx != nullptr ) {
        bool success = false;
        ChoiceT * alt_choice = rx->alt_choice_;
        success = ( alt_choice != nullptr )
            ? alt_choice->try_select()
            : true ;

        return ( success )
            ? AltResult::Ok
            : AltResult::SelectFailed ;

    } else {
        return AltResult::NoEnd;
    }
}

template<typename T>
AltResult ChannelImpl< T >::alt_send_nonblocking( std::unique_lock< Spinlock > & lk ) noexcept
{
    if ( ! lk.try_lock() ) {
        return AltResult::SyncFailed;
    }

    if ( is_closed() ) {
        return AltResult::Closed;
    }

    EndT * rx = rx_end_.load( std::memory_order_acquire );
    if ( rx != nullptr ) {
        bool success = false;
        ChoiceT * alt_choice = rx->alt_choice_;
        success = ( alt_choice != nullptr )
            ? alt_choice->try_alt_select()
            : true ;

        return ( success )
            ? AltResult::Ok
            : AltResult::SelectFailed ;

    } else {
        return AltResult::NoEnd;
    }
}

template<typename T>
void ChannelImpl< T >::alt_recv_enter( EndT & rx ) noexcept
{
    BOOST_ASSERT( rx_end_.load( std::memory_order_relaxed ) == nullptr );
    BOOST_ASSERT( ! rx_consumed_.load( std::memory_order_relaxed ) );
    std::unique_lock< Spinlock > lk{ splk_ };
    rx_end_.store( std::addressof( rx ), std::memory_order_release );
}

template<typename T>
void ChannelImpl< T >::alt_recv_leave() noexcept
{
    auto offered = alt::Sync::State::Offered;
    alt_sync_.state_.compare_exchange_strong(
        offered,
        alt::Sync::State::None,
        std::memory_order_acq_rel,
        std::memory_order_relaxed
    );
    std::unique_lock< Spinlock > lk{ splk_ };
    rx_end_.store( nullptr, std::memory_order_release );
    rx_consumed_.store( false, std::memory_order_release );
}

template<typename T>
bool ChannelImpl< T >::alt_recv_ready() noexcept
{
    // no spinlock
    if ( is_closed() ) {
        return false;
    }
    if ( tx_end_.load( std::memory_order_acquire ) != nullptr ) {
        return true;
    } else {
        return false;
    }
}

template<typename T>
AltResult ChannelImpl< T >::alt_recv() noexcept
{
    BOOST_ASSERT( has_rx_() );

    std::unique_lock< Spinlock > lk{ splk_, std::defer_lock };

    AltResult result = ( true )
        ? alt_recv_blocking( lk )
        : alt_recv_nonblocking( lk ) ;

    if ( result == AltResult::Ok ) {
        EndT * tx = tx_end_.exchange( nullptr, std::memory_order_acq_rel );
        EndT * rx = rx_end_.exchange( nullptr, std::memory_order_acq_rel );
        rx->item_ = std::move( tx->item_ );
        tx_consumed_.store( true, std::memory_order_release );
        Scheduler::self()->schedule( tx->ctx_ );
    }
    return result;
}

template<typename T>
AltResult ChannelImpl< T >::alt_recv_blocking( std::unique_lock< Spinlock > & lk ) noexcept
{
    lk.lock();

    if ( is_closed() ) {
        return AltResult::Closed;
    }

    EndT * tx = tx_end_.load( std::memory_order_acquire );
    if ( tx != nullptr ) {
        bool success = false;
        ChoiceT * alt_choice = tx->alt_choice_;
        success = ( alt_choice != nullptr )
            ? alt_choice->try_select()
            : true ;

        return ( success )
            ? AltResult::Ok
            : AltResult::SelectFailed ;

    } else {
        return AltResult::NoEnd;
    }
}

template<typename T>
AltResult ChannelImpl< T >::alt_recv_nonblocking( std::unique_lock< Spinlock > & lk ) noexcept
{
    if ( ! lk.try_lock() ) {
        return AltResult::SyncFailed;
    }

    if ( is_closed() ) {
        return AltResult::Closed;
    }

    EndT * tx = tx_end_.load( std::memory_order_acquire );
    if ( tx != nullptr ) {
        bool success = false;
        ChoiceT * alt_choice = tx->alt_choice_;
        success = ( alt_choice != nullptr )
            ? alt_choice->try_alt_select()
            : true ;

        return ( success )
            ? AltResult::Ok
            : AltResult::SelectFailed ;

    } else {
        return AltResult::NoEnd;
    }
}

////////////////////////////////////////////////////////////////////////////////
// ChannelId
////////////////////////////////////////////////////////////////////////////////

class ChannelId
{
private:
    void *    ptr_{ nullptr };
public:
    template<typename ItemT>
    ChannelId( ChannelImpl< ItemT > * chan )
        : ptr_{ static_cast< void *>( chan ) }
    {}
    bool operator == ( ChannelId const & other ) const noexcept { return ptr_ == other.ptr_; }
    bool operator != ( ChannelId const & other ) const noexcept { return ptr_ != other.ptr_; }
    bool operator <= ( ChannelId const & other ) const noexcept { return ptr_ <= other.ptr_; }
    bool operator >= ( ChannelId const & other ) const noexcept { return ptr_ >= other.ptr_; }
    bool operator <  ( ChannelId const & other ) const noexcept { return ptr_  < other.ptr_; }
    bool operator >  ( ChannelId const & other ) const noexcept { return ptr_  > other.ptr_; }
    explicit operator bool() const noexcept { return ptr_ != nullptr; }
    bool operator ! () const noexcept { return ptr_ == nullptr; }
};

} // namespace detail
} // namespace channel
PROXC_NAMESPACE_END

