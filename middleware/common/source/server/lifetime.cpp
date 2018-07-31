//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/server/lifetime.h"
#include "common/communication/ipc.h"

#include "common/log.h"


namespace casual
{
   namespace common
   {
      namespace server
      {
         namespace lifetime
         {

            namespace soft
            {
               std::vector< process::lifetime::Exit> shutdown( const std::vector< process::Handle>& servers, common::platform::time::unit timeout)
               {
                  Trace trace{ "common::server::lifetime::soft::shutdown"};

                  log::line( log::debug, "servers: ", servers);

                  auto result = process::lifetime::ended();


                  std::vector< strong::process::id> requested;

                  for( auto& handle : servers)
                  {
                     message::shutdown::Request message;

                     try
                     {
                        if( communication::ipc::non::blocking::send( handle.ipc, message))
                        {
                           requested.push_back( handle.pid);
                        }
                     }
                     catch( const exception::system::communication::Unavailable&)
                     {
                        //
                        // The server's queue is absent...
                        //
                     }

                  }

                  auto terminated = process::lifetime::wait( requested, timeout);

                  algorithm::append( std::get< 0>( algorithm::intersection( terminated, requested)), result);

                  log::line( log::debug, "soft off-line: ", result);

                  return result;

               }

            } // soft

            namespace hard
            {
               std::vector< process::lifetime::Exit> shutdown( const std::vector< process::Handle>& servers, common::platform::time::unit timeout)
               {
                  Trace trace{ "common::server::lifetime::hard::shutdown"};

                  std::vector< strong::process::id> origin;

                  for( auto& handle: servers)
                  {
                     origin.push_back( handle.pid);
                  }

                  auto result = soft::shutdown( servers, timeout);


                  auto running = algorithm::difference( origin, result);

                  log::line( log::debug, "still on-line: ", running);

                  algorithm::append( process::lifetime::terminate( range::to_vector( running), timeout), result);

                  log::line( log::debug, "hard off-line: ", std::get< 0>( algorithm::intersection( running, result)));

                  return result;

               }
            } // hard
         } // lifetime
      } // server
   } // common
} // casual
