//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "queue/common/queue.h"

#include "common/communication/instance.h"


#include "common/message/handle.h"


namespace casual
{
   using namespace common;
   namespace queue
   {
      namespace local
      {
         namespace
         {
            common::message::queue::lookup::Request request( const std::string& queue)
            {
               common::message::queue::lookup::Request request{ common::process::handle()};
               request.name = queue;
               return request;
            }
         } // <unnamed>
      } // local

      Lookup::Lookup( std::string queue)
         : m_name( std::move( queue)), m_correlation{
            common::communication::device::blocking::send(
                  common::communication::instance::outbound::queue::manager::optional::device(),
                  local::request( m_name))}
      {
      }

      common::message::queue::lookup::Reply Lookup::operator () () const
      {
         message::queue::lookup::Reply reply;

         auto& device = communication::ipc::inbound::device();

         auto handler = common::message::dispatch::handler( device,
            common::message::handle::assign( reply),
            common::message::handle::Shutdown{});

         auto condition = message::dispatch::condition::compose( 
            message::dispatch::condition::done( [&]( ){ return ! reply.correlation.empty();}));

         message::dispatch::relaxed::pump( condition, handler, device);

         return reply;
      }

      const std::string& Lookup::name() const
      {
         return m_name;
      }


   } // queue
} // casual
