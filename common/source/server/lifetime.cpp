//!
//! lifetime.cpp
//!
//! Created on: Oct 27, 2014
//!     Author: Lazan
//!

#include "common/server/lifetime.h"
#include "common/queue.h"

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
               std::vector< platform::pid_type> shutdown( const std::vector< process::Handle>& servers, std::chrono::microseconds timeout)
               {
                  std::vector< platform::pid_type> result;

                  for( auto& exit : process::lifetime::ended())
                  {
                     result.push_back( exit.pid);
                  }


                  std::vector< platform::pid_type> requested;

                  for( auto& handle : servers)
                  {
                     message::shutdown::Request message;
                     queue::non_blocking::basic_send< queue::policy::Ignore> send;

                     if( send( handle.queue, message))
                     {
                        requested.push_back( handle.pid);
                     }
                  }

                  range::append( range::intersection( requested, process::lifetime::wait( result, timeout)), result);

                  return result;

               }

            } // soft

            namespace hard
            {
               std::vector< platform::pid_type> shutdown( const std::vector< process::Handle>& servers, std::chrono::microseconds timeout)
               {
                  std::vector< platform::pid_type> origin;

                  for( auto& handle: servers)
                  {
                     origin.push_back( handle.pid);
                  }

                  auto result = soft::shutdown( servers, timeout);

                  auto running = range::difference( origin, result);

                  range::append( process::lifetime::terminate( range::to_vector( running), timeout), result);

                  return result;

               }
            } // hard



         } // lifetime

      } // server

   } // common



} // casual
