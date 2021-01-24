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
            auto request( const std::string& queue)
            {
               ipc::message::lookup::Request request{ common::process::handle()};
               request.name = queue;
               return communication::device::blocking::send(
                  communication::instance::outbound::queue::manager::optional::device(),
                  request);
            }
         } // <unnamed>
      } // local

      Lookup::Lookup( std::string queue)
         : m_name( std::move( queue)), m_correlation{ local::request( m_name)}
      {}

      ipc::message::lookup::Reply Lookup::operator () () const
      {
         ipc::message::lookup::Reply reply;
         common::communication::device::blocking::receive( communication::ipc::inbound::device(), reply, m_correlation);
         return reply;
      }

      const std::string& Lookup::name() const
      {
         return m_name;
      }


   } // queue
} // casual
