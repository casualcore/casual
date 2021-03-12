//!
//! Copyright (c) 2021, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!

#pragma once

#include "gateway/common.h"

#include "common/communication/tcp.h"
#include "common/algorithm.h"
#include "common/signal/timer.h"

#include <chrono>

namespace casual
{
   namespace gateway::group::connector
   {
      namespace external
      {
         //! Tries to connect the provided connections, if connect success, give
         //! the socket and the connection to `functor`.
         //! used by outbound and reverse inbound 
         template< typename C, typename F>
         void connect( C& connections, F&& functor)
         {
            Trace trace{ "gateway::group::connector::external::connect"};

            auto connected = [&functor]( auto& connection)
            {
               try
               {
                  ++connection.metric.attempts;
                  if( auto socket = common::communication::tcp::non::blocking::connect( connection.configuration.address))
                  {
                     // let the functor do what it needs to.
                     functor( std::move( socket), std::move( connection));
                     return true;
                  }
                  return false;
               }
               catch( ...)
               {
                  auto code = common::exception::code();
                  common::log::line( common::log::category::error, code, " connect severely failed for address: '", connection.configuration.address, "' - action: discard connection");
                  return true;
               }
            };

            common::algorithm::trim( connections, common::algorithm::remove_if( connections, connected));

            // check if we need to set a timeout to keep trying to connect

            auto min_attempts = []( auto& l, auto& r){ return l.metric.attempts < r.metric.attempts;};

            if( auto min = common::algorithm::min( connections, min_attempts))
            {
               if( min->metric.attempts < 100)
                  common::signal::timer::set( std::chrono::milliseconds{ 10});
               else
                  common::signal::timer::set( std::chrono::seconds{ 3});
            }

            common::log::line( verbose::log, "connections: ", connections);
         }
         
      } // external
      
   } // gateway::connector
} // casual