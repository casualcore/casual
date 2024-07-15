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
            auto request( const std::string& queue, queue::Lookup::Action action, queue::Lookup::Semantic semantic)
            {
               ipc::message::lookup::Request request{ common::process::handle()};
               request.name = queue;
               request.context.semantic = semantic;
               request.context.action = action;
               return communication::device::blocking::send(
                  communication::instance::outbound::queue::manager::optional::device(),
                  request);
            }
         } // <unnamed>
      } // local

      Lookup::Lookup( common::string::Argument queue, Action action, Semantic semantic)
         : m_name( std::move( queue)), m_correlation{ local::request( m_name, action, semantic)}
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
