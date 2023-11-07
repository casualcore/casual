//!
//! Copyright (c) 2018, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "common/signal.h"

#include <optional>


namespace casual::xatmi::internal::signal
{
    template< typename Flags>
    inline std::optional< casual::common::signal::thread::scope::Block> maybe_block( Flags flags)
    {
        using enum_type = typename Flags::enum_type;

        if( flags.exist( enum_type::signal_restart))
            return casual::common::signal::thread::scope::Block{};

        return {};
    }
} // casual::xatmi::internal::signal
