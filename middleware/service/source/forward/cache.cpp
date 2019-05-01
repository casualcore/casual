//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!



#include "service/forward/cache.h"
#include "service/common.h"

#include "common/message/dispatch.h"
#include "common/message/handle.h"
#include "common/exception/casual.h"

#include "common/server/handle/call.h"
#include "common/communication/instance.h"
#include "common/flag.h"

#include "domain/pending/message/send.h"

#include <iomanip>

namespace casual
{
   using namespace common;

   namespace service
   {
      namespace forward
      {

         namespace local
         {
            namespace
            {
               namespace ipc
               {
                  template< typename M> 
                  bool send( const process::Handle& process, M&& message)
                  {
                     try
                     {
                        if( ! communication::ipc::non::blocking::send( process.ipc, message))
                        {
                           log::line( log, "could not send message ", message.type(), " to process: ", process, " - action: eventually send");

                           casual::domain::pending::message::send( process, message);
                        }
                     }
                     catch( const exception::system::communication::Unavailable&)
                     {
                        // No-op, we just drop the message
                        log::line( log, "failed to sendmessage ", message.type(), " to process: ", process, " - action: discard");
                        return false;
                     }
                     return true;
                  }

               } // ipc
            } // <unnamed>
         } // local
         namespace handle
         {
            struct base
            {
               base( State& state) : m_state( state) {}

            protected:
               State& m_state;
            };

            namespace service
            {
               namespace send
               {
                  namespace error
                  {
                     void reply( State& state, common::message::service::call::callee::Request& message)
                     {
                        Trace trace{ "service::forward::handle::service::send::error::reply"};

                        if( ! message.flags.exist( common::message::service::call::request::Flag::no_reply))
                        {
                           common::message::service::call::Reply reply;
                           reply.correlation = message.correlation;
                           reply.execution = message.execution;
                           reply.transaction.trid = message.trid;
                           reply.code.result = common::code::xatmi::service_error;
                           reply.buffer = buffer::Payload{ nullptr};
                           
                           local::ipc::send( message.process, reply);
                        }
                     }
                  } // error
               } // send

               namespace name
               {
                  struct Lookup : handle::base
                  {
                     using base::base;

                     void operator () ( const message::service::lookup::Reply& message)
                     {
                        try
                        {
                           handle( message);
                        }
                        catch( ...)
                        {
                           exception::handle();
                        }
                     }

                     void handle( const message::service::lookup::Reply& message)
                     {
                        Trace trace{ "service::forward::handle::service::name::Lookup::handle"};

                        log::line( log, "service lookup reply received - message: ", message);

                        if( message.state == message::service::lookup::Reply::State::busy)
                        {
                           log::line( log, "service is busy - action: wait for idle");
                           return;
                        }

                        auto& pending_queue = m_state.requested[ message.service.name];

                        if( pending_queue.empty())
                        {
                           log::line( log::category::error, "service lookup reply for a service '", message.service.name, "' has no registered call - action: discard");
                           return;
                        }

                        // We consume the request regardless
                        auto request = std::move( pending_queue.front());
                        pending_queue.pop_front();
                        request.service = message.service;

                        // If something goes wrong, we try to send error reply to caller
                        auto error_reply = execute::scope( [&](){
                           send::error::reply( m_state, request);
                        });


                        if( message.state == message::service::lookup::Reply::State::absent)
                        {
                           log::line( log::category::error, "service '", message.service.name, "' has no entry - action: send error reply");
                           return;
                        }

                        log::line( log, "send request - to: ", message.process, " - request: ", request);

                        if( ! local::ipc::send( message.process, request))
                        {
                           log::line( log::category::error, "call to service ", std::quoted( message.service.name), "' failed - action: send error reply");
                           return;
                        }

                        error_reply.release();
                     }
                  };

               } // name

               struct Call : handle::base
               {
                  using base::base;

                  void operator () ( common::message::service::call::callee::Request& message)
                  {
                     Trace trace{ "service::forward::handle::service::Call::operator()"};

                     log::line( log, "call request received for service: ", message.service, " from: ", message.process);

                     // lookup service
                     {
                        message::service::lookup::Request request;
                        request.requested = message.service.name;
                        request.process = process::handle();

                        communication::ipc::blocking::send( communication::instance::outbound::service::manager::device(), request);
                     }

                     m_state.requested[ message.service.name].push_back( std::move( message));

                  }
               };

            } // service
         } // handle


         Cache::Cache()
         {
            Trace trace{ "service::forward::Cache ctor"};

            // Connect to domain
            communication::instance::connect( communication::instance::identity::forward::cache);

         }

         Cache::~Cache()  = default;

         void Cache::start()
         {

            try
            {
               auto handler = communication::ipc::inbound::device().handler(
                     handle::service::name::Lookup{ m_state},
                     handle::service::Call{ m_state},
                     message::handle::Shutdown{}
               );

               message::dispatch::blocking::pump( handler, communication::ipc::inbound::device());
            }
            catch( const exception::casual::Shutdown&)
            {
               exception::handle();
            }
         }
      } // forward
   } // service
} // casual



