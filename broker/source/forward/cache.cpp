/*
 * cache.cpp
 *
 *  Created on: 26 jun 2015
 *      Author: 40043280
 */

#include "broker/forward/cache.h"

#include "common/message/dispatch.h"
#include "common/message/handle.h"

#include "common/server/handle.h"

#include "common/flag.h"

#include "common/queue.h"

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
                     void reply( State& state, common::message::service::call::callee::Request&& message)
                     {
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
                              common::queue::non_blocking::Send send;
                              if( ! send( message.process.queue, reply))
                              {
                                 //
                                 // We failed to send reply for some reason (ipc-queue full?)
                                 // we'll try to send it later
                                 //
                                 state.pending.emplace_back( std::move( reply), message.process.queue);

                                 log::debug << "could not send error reply to process: " << message.process << " - will try later\n";
                              }
                           }
                           catch( const exception::queue::Unavailable&)
                           {
                              //
                              // No-op, we just drop the message
                              //
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
                        log::debug << "service lookup reply received - message: " << message << '\n';

                        if( message.state == message::service::lookup::Reply::State::busy)
                        {
                           log::debug << "service is busy - action: wait for idle\n";
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
                        scope::Execute error_reply{ [&](){
                           send::error::reply( m_state, std::move( request));
                        }};


                        if( message.state == message::service::lookup::Reply::State::absent)
                        {
                           log::error << "service '" << message.service.name << "' has no entry - action: send error reply\n";
                           return;
                        }

                        common::queue::non_blocking::Send send;
                        if( ! send( message.process.queue, request))
                        {
                           //
                           // We could not send the call. We put in pending and hope to send it
                           // later
                           //
                           m_state.pending.emplace_back( request, message.process.queue);

                           log::debug << "could not forward call to process: " << message.process << " - will try later\n";

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
                     log::debug << "call request received for service: " << message.service << " from: " << message.process << '\n';

                     //
                     // lookup service
                     //
                     {
                        message::service::lookup::Request request;
                        request.requested = message.service.name;
                        request.process = process::handle();

                        common::queue::blocking::Send send;
                        send( ipc::broker::id(), request);
                     }

                     m_state.reqested[ message.service.name].push_back( std::move( message));

                  }
               };

            } // service
         } // handle


         Cache::Cache()
         {
            //
            // Connect to broker
            //
            server::connect( ipc::receive::queue(), {});

            {
               message::forward::connect::Request connect;
               connect.process = process::handle();
               connect.identification = common::Uuid{ "f17d010925644f728d432fa4a6cf5257"};

               common::queue::blocking::Send send;
               auto correlation = send( ipc::broker::id(), connect);

               {
                  common::queue::blocking::Reader reader{ ipc::receive::queue()};
                  auto reply = message::reverse::type( connect);
                  reader( reply, correlation);

                  if( reply.directive != decltype( reply)::Directive::start)
                  {
                     throw exception::Shutdown{ "broker denied startup"};
                  }
               }
            }
         }

         Cache::~Cache()
         {

         }


         void Cache::start()
         {

            try
            {
               message::dispatch::Handler handler{
                     handle::service::name::Lookup{ m_state},
                     handle::service::Call{ m_state},
                     message::handle::Shutdown{},
               };


               while( true)
               {

                  if( m_state.pending.empty())
                  {
                     common::queue::blocking::Reader reader{ ipc::receive::queue()};
                     handler( reader.next());

                  }
                  else
                  {
                     common::queue::non_blocking::Send send;
                     auto sender = message::pending::sender( send);

                     auto remain = common::range::remove_if(
                        m_state.pending,
                        sender);

                     m_state.pending.erase( remain.last, std::end( m_state.pending));


                     common::queue::non_blocking::Reader reader{ ipc::receive::queue()};
                     while( handler( reader.next()) && m_state.pending.size() < platform::batch::transaction)
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



