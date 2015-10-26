//!
//! lifetime.cpp
//!
//! Created on: Oct 27, 2014
//!     Author: Lazan
//!

#include "common/server/lifetime.h"
#include "common/queue.h"

#include "common/trace.h"
#include "common/internal/log.h"
#include "common/internal/trace.h"

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
               std::vector< process::lifetime::Exit> shutdown( const std::vector< process::Handle>& servers, std::chrono::microseconds timeout)
               {
                  trace::internal::Scope trace{ "common::server::lifetime::soft::shutdown"};

                  log::internal::debug << "servers: " << range::make( servers) << std::endl;

                  auto result = process::lifetime::ended();


                  std::vector< platform::pid_type> requested;

                  for( auto& handle : servers)
                  {
                     message::shutdown::Request message;
                     queue::non_blocking::basic_send< queue::policy::Timeout> send;

                     try
                     {
                        if( send( handle.queue, message))
                        {
                           requested.push_back( handle.pid);
                        }
                     }
                     catch( exception::queue::Unavailable&)
                     {
                        //
                        // The server's queue is absent...
                        //
                     }

                  }

                  auto terminated = process::lifetime::wait( requested, timeout);

                  range::append( std::get< 0>( range::intersection( terminated, requested)), result);

                  log::internal::debug << "soft off-line: " << range::make( result) << std::endl;

                  return result;

               }

            } // soft

            namespace hard
            {
               std::vector< process::lifetime::Exit> shutdown( const std::vector< process::Handle>& servers, std::chrono::microseconds timeout)
               {
                  trace::internal::Scope trace{ "common::server::lifetime::hard::shutdown"};

                  std::vector< platform::pid_type> origin;

                  for( auto& handle: servers)
                  {
                     origin.push_back( handle.pid);
                  }

                  auto result = soft::shutdown( servers, timeout);


                  auto running = range::difference( origin, result);

                  log::internal::debug << "still on-line: " << range::make( running) << std::endl;

                  range::append( process::lifetime::terminate( range::to_vector( running), timeout), result);

                  log::internal::debug << "hard off-line: " << std::get< 0>( range::intersection( running, result)) << std::endl;

                  return result;

               }
            } // hard



         } // lifetime

      } // server

   } // common



} // casual
