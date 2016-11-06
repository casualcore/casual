//!
//! casual
//!


#include "broker/forward/cache.h"
#include "broker/common.h"

#include "common/message/dispatch.h"
#include "common/message/handle.h"

#include "common/server/handle.h"

#include "common/flag.h"


namespace casual
{
   using namespace common;

   namespace broker
   {
      namespace forward
      {

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
                        Trace trace{ "broker::forward::handle::service::send::error::reply"};

                        if( ! common::flag< TPNOREPLY>( message.flags))
                        {
                           common::message::service::call::Reply reply;
                           reply.correlation = message.correlation;
                           reply.execution = message.execution;
                           reply.transaction.trid = message.trid;
                           reply.error = TPESVCERR;
                           reply.descriptor = message.descriptor;
                           reply.buffer = buffer::Payload{ nullptr};

                           try
                           {
                              if( ! communication::ipc::non::blocking::send( message.process.queue, reply))
                              {
                                 //
                                 // We failed to send reply for some reason (ipc-queue full?)
                                 // we'll try to send it later
                                 //
                                 state.pending.emplace_back( std::move( reply), message.process.queue);

                                 log << "could not send error reply to process: " << message.process << " - will try later\n";
                              }
                           }
                           catch( const exception::queue::Unavailable&)
                           {
                              //
                              // No-op, we just drop the message
                              //
                              log << "could not send error reply to process: " << message.process << " - queue unavailable - action: ignore\n";
                           }
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
                           error::handler();
                        }
                     }

                     void handle( const message::service::lookup::Reply& message)
                     {
                        Trace trace{ "broker::forward::handle::service::name::Lookup::handle"};

                        log << "service lookup reply received - message: " << message << '\n';

                        if( message.state == message::service::lookup::Reply::State::busy)
                        {
                           log << "service is busy - action: wait for idle\n";
                           return;
                        }

                        auto& pending_queue = m_state.reqested[ message.service.name];

                        if( pending_queue.empty())
                        {
                           log::error << "service lookup reply for a service '" << message.service.name << "' has no registered call - action: discard\n";
                           return;
                        }



                        //
                        // We consume the request regardless
                        //
                        auto request = std::move( pending_queue.front());
                        pending_queue.pop_front();
                        request.service = message.service;

                        //
                        // If something goes wrong, we try to send error reply to caller
                        //
                        auto error_reply = scope::execute( [&](){
                           send::error::reply( m_state, request);
                        });


                        if( message.state == message::service::lookup::Reply::State::absent)
                        {
                           log::error << "service '" << message.service.name << "' has no entry - action: send error reply\n";
                           return;
                        }

                        log << "send request - to: " << message.process.queue << " - request: " << request << std::endl;

                        if( ! communication::ipc::non::blocking::send( message.process.queue, request))
                        {
                           //
                           // We could not send the call. We put in pending and hope to send it
                           // later
                           //
                           m_state.pending.emplace_back( request, message.process.queue);

                           log << "could not forward call to process: " << message.process << " - will try later\n";

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
                     Trace trace{ "broker::forward::handle::service::Call::operator()"};

                     log << "call request received for service: " << message.service << " from: " << message.process << '\n';

                     //
                     // lookup service
                     //
                     {
                        message::service::lookup::Request request;
                        request.requested = message.service.name;
                        request.process = process::handle();

                        communication::ipc::blocking::send( communication::ipc::broker::device(), request);
                     }

                     m_state.reqested[ message.service.name].push_back( std::move( message));

                  }
               };

            } // service
         } // handle


         Cache::Cache()
         {
            Trace trace{ "broker::forward::Cache ctor"};

            //
            // Connect to domain
            //
            process::instance::connect( process::instance::identity::forward::cache());

         }

         Cache::~Cache()
         {

         }


         void Cache::start()
         {

            try
            {
               auto handler = communication::ipc::inbound::device().handler(
                     handle::service::name::Lookup{ m_state},
                     handle::service::Call{ m_state},
                     message::handle::Shutdown{}
               );


               while( true)
               {

                  if( m_state.pending.empty())
                  {
                     handler( communication::ipc::inbound::device().next( communication::ipc::policy::Blocking{}));
                  }
                  else
                  {
                     signal::handle();
                     signal::thread::scope::Block block;

                     auto sender = message::pending::sender( communication::ipc::policy::non::Blocking{});

                     auto remain = common::range::remove_if(
                        m_state.pending,
                        sender);

                     m_state.pending.erase( std::end( remain), std::end( m_state.pending));

                     while( handler( communication::ipc::inbound::device().next( communication::ipc::policy::non::Blocking{}))
                           && m_state.pending.size() < platform::batch::transaction())
                     {
                        ;
                     }
                  }
               }
            }
            catch( const exception::Shutdown&)
            {
               error::handler();
            }
         }
      } // forward
   } // broker
} // casual



