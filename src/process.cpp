//          Copyright Oliver Kowalke 2009.
//          Copyright Edvard Severin Pettersen 2017.
// Distributed under the Boost Software License, Version 1.0.
//    (See accompanying file LICENSE.md or copy at
//          http://www.boost.org/LICENSE_1_0.txt)

#include <proxc/config.hpp>

#include <proxc/process.hpp>
#include <proxc/runtime/scheduler.hpp>

PROXC_NAMESPACE_BEGIN

////////////////////////////////////////////////////////////////////////////////
// Process implementation
////////////////////////////////////////////////////////////////////////////////

Process::~Process() noexcept
{
    ctx_.reset();
}

Process::Process( Process && other ) noexcept
    : ctx_{}
{
    ctx_.swap( other.ctx_ );
}

Process & Process::operator = ( Process && other ) noexcept
{
    if ( this != & other ) {
        ctx_.swap( other.ctx_ );
    }
    return *this;
}

void Process::swap( Process & other ) noexcept
{
    ctx_.swap( other.ctx_ );
}

auto Process::get_id() const noexcept
    -> Id
{
    return ctx_->get_id();
}

void Process::launch() noexcept
{
    runtime::Scheduler::self()->commit( ctx_.get() );
}

void Process::join()
{
    runtime::Scheduler::self()->join( ctx_.get() );
    ctx_.reset();
}

void Process::detach()
{
    ctx_.reset();
}

PROXC_NAMESPACE_END

