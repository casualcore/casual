//!
//! Copyright (c) 2021, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "common/message/type.h"
#include "common/log/stream.h"

namespace casual
{
   namespace common::message::internal
   {
      namespace dump
      {
         using State = message::basic_message< Type::internal_dump_state>;

         namespace state
         {
            template< typename State>
            auto handle( const State& state)
            {
               return [&state]( const internal::dump::State&)
               {
                  std::ostringstream out;
                  log::write( out, "state: ", state);
                  log::stream::write( "casual.internal", std::move( out).str());
               };
            }
         } // state


         
      } // dump
   } // common::message::internal
} // casual