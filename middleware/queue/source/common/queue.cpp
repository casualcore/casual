//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "queue/common/queue.h"

#include "common/communication/ipc.h"


#include "common/message/handle.h"


namespace casual
{
   namespace queue
   {
      namespace local
      {
         namespace
         {
            common::message::queue::lookup::Request request( const std::string& queue)
            {
               common::message::queue::lookup::Request request;
               request.process = common::process::handle();
               request.name = queue;

               return request;
            }
         } // <unnamed>
      } // local

      Lookup::Lookup( std::string queue)
         : m_name( std::move( queue)), m_correlation{
            common::communication::ipc::blocking::send(
                  common::communication::ipc::queue::manager::optional::device(),
                  local::request( m_name))}
      {
      }

      common::message::queue::lookup::Reply Lookup::operator () () const
      {

         common::message::queue::lookup::Reply reply;

         auto& device = common::communication::ipc::inbound::device();

         auto handler = device.handler(
            common::message::handle::assign( reply),
            common::message::handle::Shutdown{});


         handler( device.select( device.policy_blocking(), [&]( auto& complete){
            return complete.correlation == m_correlation || complete.type == common::message::shutdown::Request::type();
         }));

         return reply;
      }

      const std::string& Lookup::name() const
      {
         return m_name;
      }


   } // queue
} // casual
