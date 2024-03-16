//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "common/service/call/context.h"
#include "common/service/lookup.h"


#include "common/communication/ipc.h"
#include "common/communication/instance.h"

#include "common/log.h"

#include "common/buffer/pool.h"
#include "common/buffer/transport.h"

#include "common/environment.h"
#include "common/flag.h"
#include "common/signal.h"

#include "common/code/raise.h"
#include "common/code/xatmi.h"

#include "common/transaction/context.h"

#include "common/execute.h"

#include "xatmi.h"



#include <algorithm>
#include <cassert>

namespace casual
{
   namespace common::service::call
   {
      Context& Context::instance()
      {
         static Context singleton;
         return singleton;
      }

      namespace local
      {
         namespace
         {
            namespace prepare
            {
               struct Reply 
               {
                  platform::descriptor::type descriptor;
                  message::service::call::caller::Request message;
               };

               auto lookup( std::string service, async::Flag flags)
               {
                  Trace trace( "service::call::local::prepare::lookup");

                  auto transform_context = []( auto flags)
                  {
                     // if no-reply we treat it as a _forward-call_, and we'll not block until the service is idle.
                     // Hence, it's a fire-and-forget message.

                     message::service::lookup::request::Context context;
                     context.semantic = flag::exists( flags, call::async::Flag::no_reply) ? decltype( context.semantic)::no_reply : decltype( context.semantic)::regular;
                     return context;
                  };

                  if( auto& current = common::transaction::Context::instance().current())
                  {                     
                     if( ! flag::exists( flags, call::async::Flag::no_transaction))
                     {
                        if( flag::exists( flags, call::async::Flag::no_reply))
                           code::raise::error( code::xatmi::argument, "TPNOREPLY can only be used with TPNOTRAN");

                        return service::Lookup{ std::move( service), transform_context( flags), current.deadline};
                     }
                  }

                  return service::Lookup{ std::move( service), transform_context( flags)};
               }

               inline Reply message(
                     State& state,
                     const platform::time::point::type& start,
                     common::buffer::payload::Send&& buffer,
                     service::header::Fields header,
                     async::Flag flags,
                     const service::Lookup::Reply& lookup)
               {
                  Trace trace( "service::call::local::prepare::message");

                  message::service::call::caller::Request message( std::move( buffer));

                  message.correlation = lookup.correlation;
                  message.service = lookup.service;
                  message.service.timeout = lookup.service.timeout;

                  message.process = process::handle();
                  message.parent = common::execution::service::name();

                  message.flags = static_cast< message::service::call::request::Flag>( flags);

                  message.header = std::move( header);

                  auto& transaction = common::transaction::context().current();

                  // Check if we should associate descriptor with message-correlation and transaction
                  if( flag::exists( flags, async::Flag::no_reply))
                  {
                     if( transaction && ! flag::exists( flags, async::Flag::no_transaction))
                        code::raise::error( code::xatmi::argument, "flag ", async::Flag::no_reply, " used within a transaction context without ", async::Flag::no_transaction);

                     log::line( log::debug, "no_reply - no descriptor reservation");

                     // No reply, hence no descriptor and no transaction (we validated this before)
                     return Reply{ 0, std::move( message)};
                  }
                  else
                  {
                     log::line( log::debug, "descriptor reservation - flags: ", flags);

                     auto& descriptor = state.pending.reserve( message.correlation);

                     if( ! flag::exists( flags, async::Flag::no_transaction) && transaction)
                     {
                        message.trid = transaction.trid;
                        transaction.associate( message.correlation);
                     }

                    
                     return Reply{ descriptor.descriptor, std::move( message) };
                  }
               }
            } // prepare
         } // <unnamed>
      } // local
      
      descriptor_type Context::async( service::Lookup&& service, common::buffer::payload::Send buffer, service::header::Fields header, async::Flag flags)
      {
         Trace trace( "service::call::Context::async lookup");

         log::line( log::debug, "service: ", service, ", buffer: ", buffer, " flags: ", flags);

         auto start = platform::time::clock::type::now();

         // TODO: Invoke pre-transport buffer modifiers
         //buffer::transport::Context::instance().dispatch( idata, ilen, service, buffer::transport::Lifecycle::pre_call);

         // Get target corresponding to the service
         auto target = service();

         // The service exists. Take care of reserving descriptor and determine timeout
         auto prepared = local::prepare::message( m_state, start, std::move( buffer), std::move( header), flags, target);

         // If some thing goes wrong we unreserve the descriptor
         auto unreserve = common::execute::scope( [&](){ m_state.pending.unreserve( prepared.descriptor);});


         if( target.busy())
         {
            // We wait for an instance to become idle.
            target = service();
         }

         if( target.state != decltype( target.state)::idle)
            code::raise::error( code::casual::invalid_semantics, "unable to reserve instance of service '", target.service, "'");

         // Call the service
         {
            prepared.message.service = target.service;
            prepared.message.pending = target.pending;

            log::line( log::debug, "async - message: ", prepared.message);

            communication::device::blocking::send( target.process.ipc, prepared.message);
         }
         log::line( log::category::event::service, "send|", target.service.name, '|', prepared.descriptor);

         unreserve.release();
         return prepared.descriptor;
      }

      descriptor_type Context::async( service::Lookup&& service, common::buffer::payload::Send buffer, async::Flag flags)
      {
         return async( std::move( service), std::move( buffer), service::header::fields(), flags);
      }

      descriptor_type Context::async( const std::string& service, common::buffer::payload::Send buffer, async::Flag flags)
      {
         return async( local::prepare::lookup( service, flags), std::move( buffer), flags); 
      }

      namespace local
      {
         namespace
         {
            template< typename... Args>
            bool receive( message::service::call::Reply& reply, reply::Flag flags, Args&&... args)
            {
               if( flag::exists( flags, reply::Flag::no_block))
               {
                  return communication::device::non::blocking::receive( 
                     communication::ipc::inbound::device(), 
                     reply, 
                     std::forward< Args>( args)...);
               }
               else
               {
                  return communication::device::blocking::receive( 
                     communication::ipc::inbound::device(), 
                     reply, 
                     std::forward< Args>( args)...);
               }
            }
         } // <unnamed>
      } // local

      reply::Result Context::reply( descriptor_type descriptor, reply::Flag flags)
      {
         Trace trace( "calling::Context::reply");
         log::line( log::debug, "descriptor: ", descriptor, " flags: ", flags);

         //
         // TODO: validate input...

         auto get_reply = [&]()
         {
            Trace trace( "calling::Context::reply get_reply");
             
            message::service::call::Reply reply;

            if( flag::exists( flags, reply::Flag::any))
            {
               // We fetch any
               if( ! local::receive( reply, flags))
                  code::raise::error( code::xatmi::no_message);

               return std::make_pair(
                  std::move( reply),
                  m_state.pending.get( reply.correlation).descriptor);
            }
            else
            {
               auto& pending = m_state.pending.get( descriptor);

               if( ! local::receive( reply, flags, pending.correlation))
                  code::raise::error( code::xatmi::no_message);

               return std::make_pair(
                  std::move( reply),
                  pending.descriptor);
            }
         };


         reply::Result result;
         auto [ reply, xatmi_descriptor] = get_reply();

         log::line( log::category::event::service , "receive|", xatmi_descriptor, '|', reply.code.result);

         result.descriptor = xatmi_descriptor;
         result.user = reply.code.user;
         result.buffer = std::move( reply.buffer);


         // We unreserve pending (at end of scope, regardless of outcome)
         auto discard = execute::scope( [&](){ m_state.pending.unreserve( result.descriptor);});

         // Update transaction state
         common::transaction::context().update( reply);

         // Check any errors
         switch( reply.code.result)
         {
            case code::xatmi::ok:
               break;
            case code::xatmi::service_fail:
            {
               call::Fail exception;
               exception.result = std::move( result);
               throw exception;
            }
            default: 
            {
               code::raise::error( reply.code.result);
            }
         }
         return result;
      }

      namespace local
      {
         namespace
         {
            namespace suspend
            {
               auto wrapper( sync::Flag flags)
               {
                  Trace trace{ "service::call::local::suspend::wrapper"};
                  
                  auto scoped = []( common::transaction::Transaction* transaction)
                  {
                     return execute::scope( [ transaction]()
                     {
                        if( ! transaction)
                           return;

                        common::transaction::context().resources_resume( *transaction);
                     });
                  };

                  auto& current = common::transaction::context().current();
                  
                  if( current && ! flag::exists( flags, sync::Flag::no_transaction))
                  {
                     if( ! current.involved().empty())
                     {
                        // Let TM know about our involved resources, to enable the callee to synchronize
                        message::transaction::resource::involved::Request request{ process::handle()};
                        request.trid = current.trid;
                        request.involved = current.involved();
                        request.reply = false; // we don't need a reply
                        communication::device::blocking::send( communication::instance::outbound::transaction::manager::device(), request);
                     }

                     common::transaction::context().resources_suspend( current);
                     return scoped( &current);   
                  }

                  return scoped( nullptr);
               }
            } // suspend

         } // <unnamed>
      } // local

      sync::Result Context::sync( const std::string& service, common::buffer::payload::Send buffer, sync::Flag flags)
      {
         // We can't have no-block when getting the reply
         flags -= sync::Flag::no_block;

         // Suspend if ongoing transaction and no no_transaction flag.
         auto guard = local::suspend::wrapper( flags);

         auto descriptor = async( service, buffer, flag::convert( async::valid_flags, flags));
         auto result = reply( descriptor, flag::convert( reply::valid_flags, flags));

         return { std::move( result.buffer), result.user};
      }


      void Context::cancel( descriptor_type descriptor)
      {
         m_state.pending.discard( descriptor);
      }


      void Context::clean()
      {
         // TODO: Do some cleaning on buffers, pending replies and such...
      }

      Context::Context()
      = default;

      bool Context::pending() const
      {
         return ! m_state.pending.empty();

      }



      bool Context::receive( message::service::call::Reply& reply, descriptor_type descriptor, reply::Flag flags)
      {
         if( flag::exists( flags, reply::Flag::any))
         {
            // We fetch any
            return local::receive( reply, flags);
         }
         else
         {
            auto& correlation = m_state.pending.get( descriptor).correlation;

            return local::receive( reply, flags, correlation);
         }
      }

   } // common::service::call
} // casual
