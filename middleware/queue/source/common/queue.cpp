//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "queue/common/queue.h"

#include "common/communication/instance.h"


#include "common/message/dispatch/handle.h"


namespace casual
{
   using namespace common;
   namespace queue
   {
      namespace local
      {
         namespace
         {
            auto request( const std::string& queue, ipc::message::lookup::request::context::Semantic semantic)
            {
               ipc::message::lookup::Request request{ common::process::handle()};
               request.name = queue;
               request.context.semantic = semantic;
               return communication::device::blocking::send(
                  communication::instance::outbound::queue::manager::optional::device(),
                  request);
            }
         } // <unnamed>
      } // local

      Lookup::Lookup( common::string::Argument queue, Semantic semantic)
         : m_name( std::move( queue)), m_correlation{ local::request( m_name, semantic)}
      {}

      ipc::message::lookup::Reply Lookup::operator () () const
      {
         return common::communication::ipc::receive< ipc::message::lookup::Reply>( m_correlation);
      }

      const std::string& Lookup::name() const
      {
         return m_name;
      }


   } // queue
} // casual
