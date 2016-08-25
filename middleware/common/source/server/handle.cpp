//!
//! casual
//!

#include "common/server/handle.h"

#include "common/service/lookup.h"

namespace casual
{
   namespace common
   {
      namespace server
      {

         namespace local
         {
            namespace
            {
               void advertise( std::vector< message::service::advertise::Service> services)
               {
                  if( ! services.empty())
                  {
                     message::service::Advertise advertise;
                     advertise.process = process::handle();
                     advertise.services = std::move( services);

                     communication::ipc::blocking::send( communication::ipc::broker::device(), advertise);
                  }
               }
            } // <unnamed>
         } // local

         namespace handle
         {

            namespace policy
            {
               void Default::connect( std::vector< message::service::advertise::Service> services, const std::vector< transaction::Resource>& resources)
               {
                  //
                  // Connection to the domain has been done before...
                  //


                  //
                  // Let the broker know about our services...
                  //
                  local::advertise( std::move( services));

                  //
                  // configure resources, if any.
                  //
                  transaction::Context::instance().set( resources);

               }

               void Default::reply( platform::ipc::id::type id, message::service::call::Reply& message)
               {
                  communication::ipc::blocking::send( id, message);
               }

               void Default::ack( const message::service::call::callee::Request& message)
               {
                  message::service::call::ACK ack;
                  ack.process = process::handle();
                  ack.service = message.service.name;

                  communication::ipc::blocking::send( communication::ipc::broker::device(), ack);
               }


               void Default::statistics( platform::ipc::id::type id,  message::traffic::Event& event)
               {
                  log::internal::debug << "policy::Default::statistics - event:" << event << std::endl;

                  try
                  {
                     communication::ipc::blocking::send( id, event);
                  }
                  catch( ...)
                  {
                     error::handler();
                  }
               }

               void Default::transaction( const message::service::call::callee::Request& message, const server::Service& service, const platform::time_point& now)
               {
                  log::internal::debug << "service: " << service << std::endl;

                  //
                  // We keep track of callers transaction (can be null-trid).
                  //
                  transaction::Context::instance().caller = message.trid;

                  switch( service.transaction)
                  {
                     case service::transaction::Type::automatic:
                     {
                        if( message.trid)
                        {
                           transaction::Context::instance().join( message.trid);
                        }
                        else
                        {
                           transaction::Context::instance().start( now);
                        }
                        break;
                     }
                     case service::transaction::Type::join:
                     {
                        transaction::Context::instance().join( message.trid);
                        break;
                     }
                     case service::transaction::Type::atomic:
                     {
                        transaction::Context::instance().start( now);
                        break;
                     }
                     case service::transaction::Type::none:
                     default:
                     {
                        //
                        // We don't start or join any transactions
                        // (technically we join a null-trid)
                        //
                        transaction::Context::instance().join( transaction::ID{ process::handle()});
                        break;
                     }

                  }

                  //
                  // Set 'global deadline'
                  //
                  transaction::Context::instance().current().timout.set( now, message.service.timeout);

               }

               void Default::transaction( message::service::call::Reply& message, int return_state)
               {
                  transaction::Context::instance().finalize( message, return_state);
               }

               void Default::forward( const message::service::call::callee::Request& message, const State::jump_t& jump)
               {

                  if( transaction::Context::instance().pending())
                  {
                     throw common::exception::xatmi::service::Error( "service: " + message.service.name + " tried to forward with pending transactions");
                  }

                  common::service::Lookup lookup{ jump.forward.service, message::service::lookup::Request::Context::forward};


                  message::service::call::callee::Request request;
                  request.correlation = message.correlation;
                  request.parent = message.service.name;
                  request.descriptor = message.descriptor;
                  request.trid = message.trid;
                  request.process = message.process;
                  request.flags = message.flags;

                  if( jump.buffer.data == nullptr)
                  {
                     request.buffer = buffer::Payload{ nullptr};
                  }
                  else
                  {
                     request.buffer = buffer::pool::Holder::instance().release( jump.buffer.data, jump.buffer.len);
                  }

                  auto target = lookup();

                  if( target.state == message::service::lookup::Reply::State::absent)
                  {
                     throw common::exception::xatmi::service::no::Entry( target.service.name);
                  }
                  else if( target.state ==  message::service::lookup::Reply::State::busy)
                  {
                     //
                     // We wait for service to become idle
                     //
                     target = lookup();
                  }

                  request.service = target.service;


                  log::internal::debug << "policy::Default::forward - request:" << request << std::endl;

                  communication::ipc::blocking::send( target.process.queue, request);
               }

               Admin::Admin( communication::error::type handler)
                  : m_error_handler{ std::move( handler)}
               {}


               void Admin::connect( std::vector< message::service::advertise::Service> services, const std::vector< transaction::Resource>& resources)
               {
                  //
                  // Connection to the domain has been done before...
                  //


                  if( ! resources.empty())
                  {
                     throw common::exception::invalid::Semantic{ "can't build and link an administration server with resources"};
                  }

                  //
                  // Let the broker know about our services...
                  //
                  local::advertise( std::move( services));
               }

               void Admin::reply( platform::ipc::id::type id, message::service::call::Reply& message)
               {
                  communication::ipc::blocking::send( id, message, m_error_handler);
               }

               void Admin::ack( const message::service::call::callee::Request& message)
               {
                  message::service::call::ACK ack;
                  ack.process = common::process::handle();
                  ack.service = message.service.name;

                  communication::ipc::blocking::send( communication::ipc::broker::device(), ack, m_error_handler);
               }


               void Admin::statistics( platform::ipc::id::type id, message::traffic::Event&)
               {
                  // no-op
               }

               void Admin::transaction( const message::service::call::callee::Request&, const server::Service&, const common::platform::time_point&)
               {
                  // no-op
               }
               void Admin::transaction( message::service::call::Reply& message, int return_state)
               {
                  // no-op
               }

               void Admin::forward( const common::message::service::call::callee::Request& message, const common::server::State::jump_t& jump)
               {
                  throw common::exception::xatmi::System{ "can't forward within an administration server"};
               }

            } // policy

         } // handle
      } // server
   } // common



} // casual
