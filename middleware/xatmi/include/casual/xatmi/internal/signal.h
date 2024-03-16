//!
//! Copyright (c) 2018, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "common/signal.h"
#include "common/flag.h"

#include <optional>


namespace casual::xatmi::internal::signal
{
    template< typename Flag>
    inline std::optional< casual::common::signal::thread::scope::Block> maybe_block( Flag flags)
    {
        if( common::flag::exists( flags, Flag::signal_restart))
            return casual::common::signal::thread::scope::Block{};

        return {};
    }
} // casual::xatmi::internal::signal
