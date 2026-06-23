//! 
//! Copyright (c) 2015, The casual project
//!
//! This software is licensed under the MIT license, https://opensource.org/licenses/MIT
//!


#include "service/call/context.h"
#include "service/lookup.h"

#include "transaction/context.h"

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

#include "common/execute.h"

#include "xatmi.h"



#include <algorithm>
#include <cassert>

namespace casual
{
   namespace service::call
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
                  common::strong::correlation::id correlation;
                  common::message::service::call::caller::Request message;
               };

               auto lookup( std::string service, async::Flag flags, const std::optional< common::chronology::time_point>& deadline)
               {
                  common::Trace trace( "service::call::local::prepare::lookup");

                  using Semantic = common::message::service::lookup::request::context::Semantic;

                  auto transform_context = []( auto flags)
                  {
                     // if no-reply we treat it as a _forward-call_, and we'll not block until the service is idle.
                     // Hence, it's a fire-and-forget message.
                     
                     common::message::service::lookup::request::Context context;
                     context.semantic = common::flag::contains( flags, call::async::Flag::no_reply) ? Semantic::no_reply : Semantic::regular;
                     return context;
                  };

                  const auto context = transform_context( flags);

                  if( auto& current = casual::transaction::context().current())
                  {                     
                     if( ! common::flag::contains( flags, call::async::Flag::no_transaction))
                     {
                        if( common::flag::contains( flags, call::async::Flag::no_reply))
                           common::code::raise::error( common::code::xatmi::argument, "TPNOREPLY can only be used with TPNOTRAN");

                        return service::Lookup{ std::move( service), current.trid, context, current.deadline};
                     }
                  }

                  // if noreply we don't supply our current deadline. There are use cases where servers
                  // will call it self with noreply, as poor mans polling mechanism. If we supply the deadline
                  // the server will eventually run out of time (if a timeout is set for the service).
                  if( context.semantic == Semantic::no_reply)
                     return service::Lookup{ std::move( service), {}, context, {}};
                  else
                     return service::Lookup{ std::move( service), {}, context, deadline};
               }

               inline Reply message(
                     State& state,
                     common::buffer::payload::Send&& buffer,
                     const header::Fields& header,
                     async::Flag flags,
                     const service::lookup::Reply& lookup)
               {
                  common::Trace trace( "service::call::local::prepare::message");

                  common::message::service::call::caller::Request message( std::move( buffer), common::process::handle());
                  // set stuff from lookup-reply (service, span, deadline, etc)
                  message.update( lookup); 
                  message.parent.service = common::execution::context::get().service;
                  message.parent.span = common::execution::context::get().span;

                  message.flags = static_cast< common::message::service::call::request::Flag>( flags);

                  message.header = header;

                  auto& transaction = casual::transaction::context().current();

                  // Check if we should associate descriptor with message-correlation and transaction
                  if( common::flag::contains( flags, async::Flag::no_reply))
                  {
                     if( transaction && ! common::flag::contains( flags, async::Flag::no_transaction))
                        common::code::raise::error( common::code::xatmi::argument, "flag ", async::Flag::no_reply, " used within a transaction context without ", async::Flag::no_transaction);

                     common::log::debug( "no_reply - no descriptor reservation");

                     // No reply, hence no descriptor and no transaction (we validated this before)
                     return Reply{ {}, std::move( message)};
                  }
                  else
                  {
                     common::log::debug( "descriptor reservation - flags: ", flags);

                     auto& correlation = state.pending.reserve( message.correlation);

                     if( ! common::flag::contains( flags, async::Flag::no_transaction) && transaction)
                     {
                        message.trid = transaction.trid;
                        transaction.associate( correlation);
                     }

                     return Reply{ correlation, std::move( message)};
                  }
               }
            } // prepare
         } // <unnamed>
      } // local
      
      common::strong::correlation::id Context::async( service::Lookup&& service, common::buffer::payload::Send buffer, async::Flag flags, const header::Fields& header)
      {
         common::Trace trace( "service::call::Context::async lookup");

         common::log::debug( "service: ", service, ", buffer: ", buffer, " flags: ", flags);

         // TODO: Invoke pre-transport buffer modifiers
         //buffer::transport::Context::instance().dispatch( idata, ilen, service, buffer::transport::Lifecycle::pre_call);

         // Get target corresponding to the service
         auto target = service::lookup::reply( std::move( service));

         // The service exists. Take care of reserving descriptor and determine timeout
         auto prepared = local::prepare::message( m_state, std::move( buffer), std::move( header), flags, target);

         // If some thing goes wrong we unreserve the descriptor
         auto unreserve = common::execute::scope( [&](){ m_state.pending.unreserve( prepared.correlation);});


         if( target.state != decltype( target.state)::idle)
            common::code::raise::error( common::code::casual::invalid_semantics, "unable to reserve instance of service '", target.service, "'");

         // Call the service
         {
            common::log::debug( "async - message: ", prepared.message);

            common::communication::device::blocking::send( target.process.ipc, prepared.message);
         }
         common::log::line( common::log::category::event::service, "send|", target.service.name, '|', prepared.correlation);

         unreserve.release();
         return prepared.correlation;
      }


      common::strong::correlation::id Context::async( const std::string& service, common::buffer::payload::Send buffer, async::Flag flags, const header::Fields& header)
      {
         return async( local::prepare::lookup( service, flags, m_state.deadline), std::move( buffer), flags, header); 
      }

      namespace local
      {
         namespace
         {
            template< typename... Args>
            bool receive( common::message::service::call::Reply& reply, reply::Flag flags, Args&&... args)
            {
               if( common::flag::contains( flags, reply::Flag::no_block))
               {
                  return common::communication::device::non::blocking::receive( 
                     common::communication::ipc::inbound::device(), 
                     reply, 
                     std::forward< Args>( args)...);
               }
               else
               {
                  return common::communication::device::blocking::receive( 
                     common::communication::ipc::inbound::device(), 
                     reply, 
                     std::forward< Args>( args)...);
               }
            }
         } // <unnamed>
      } // local

      reply::Result Context::reply( const common::strong::correlation::id& correlation, reply::Flag flags)
      {
         common::Trace trace( "calling::Context::reply");
         common::log::debug( "correlation: ", correlation, " flags: ", flags);

         //
         // TODO: validate input...

         auto get_reply = [&]()
         {
            common::Trace trace( "calling::Context::reply get_reply");
             
            common::message::service::call::Reply reply;

            if( common::flag::contains( flags, reply::Flag::any))
            {
               // We fetch any
               if( ! local::receive( reply, flags))
                  common::code::raise::error( common::code::xatmi::no_message);

               return std::make_pair(
                  std::move( reply),
                  m_state.pending.validate( reply.correlation));
            }
            else
            {
               if( ! local::receive( reply, flags, m_state.pending.validate( correlation)))
                  common::code::raise::error( common::code::xatmi::no_message);

               return std::make_pair(
                  std::move( reply),
                  correlation);
            }
         };


         reply::Result result;
         auto [ reply, xatmi_descriptor] = get_reply();

         common::log::line( common::log::category::event::service , "receive|", xatmi_descriptor, '|', reply.code.result);

         result.correlation = xatmi_descriptor;
         result.user = reply.code.user;
         result.buffer = std::move( reply.buffer);
         result.header = std::move( reply.header);


         // We unreserve pending (at end of scope, regardless of outcome)
         auto discard = common::execute::scope( [&](){ m_state.pending.unreserve( result.correlation);});

         // Update transaction state
         casual::transaction::context().update( reply);

         // Check any errors
         switch( reply.code.result)
         {
            case common::code::xatmi::ok:
               break;
            case common::code::xatmi::service_fail:
            {
               call::Fail exception;
               exception.result = std::move( result);
               throw exception;
            }
            default: 
            {
               common::code::raise::error( reply.code.result);
            }
         }
         return result;
      }

      reply::Result Context::reply( reply::Flag flags)
      {
         return reply( common::strong::correlation::id{}, flags | reply::Flag::any);
      }

      namespace local
      {
         namespace
         {
            namespace suspend
            {
               auto wrapper( sync::Flag flags)
               {
                  common::Trace trace{ "service::call::local::suspend::wrapper"};
                  
                  auto scoped = []( casual::transaction::Transaction* transaction)
                  {
                     return common::execute::scope( [ transaction]()
                     {
                        if( ! transaction)
                           return;

                        casual::transaction::context().resources_resume( *transaction);
                     });
                  };

                  auto& current = casual::transaction::context().current();
                  
                  if( current && ! common::flag::contains( flags, sync::Flag::no_transaction))
                  {
                     if( ! current.involved().empty())
                     {
                        // Let TM know about our involved resources, to enable the callee to synchronize
                        common::message::transaction::resource::involved::Request request{ common::process::handle()};
                        request.trid = current.trid;
                        request.involved = current.involved();
                        request.reply = false; // we don't need a reply
                        common::communication::device::blocking::send( common::communication::instance::outbound::transaction::manager::device(), request);
                     }

                     casual::transaction::context().resources_suspend( current);
                     return scoped( &current);   
                  }

                  return scoped( nullptr);
               }
            } // suspend

         } // <unnamed>
      } // local

      sync::Result Context::sync( const std::string& service, common::buffer::payload::Send buffer, sync::Flag flags, const header::Fields& header)
      {
         // We can't have no-block when getting the reply
         flags -= sync::Flag::no_block;

         // Suspend if ongoing transaction and no no_transaction flag.
         auto guard = local::suspend::wrapper( flags);

         auto descriptor = async( service, buffer, common::flag::convert( async::valid_flags, flags), header);
         auto result = reply( descriptor, common::flag::convert( reply::valid_flags, flags));

         return { 
            .buffer = std::move( result.buffer),
            .header = std::move( result.header), 
            .user = result.user};
      }


      void Context::cancel( const common::strong::correlation::id& correlation)
      {
         m_state.pending.discard( correlation);
      }

      void Context::clear()
      {
         m_state.deadline = {}; 
         m_state.pending.finalize();
         // TODO: Do some cleaning on buffers, pending replies and such...
      }

      Context::Context() = default;

      bool Context::pending() const
      {
         return ! m_state.pending.empty();
      }

      void Context::deadline( common::chronology::time_point now, std::optional< common::chronology::duration> timeout)
      {
         if( timeout)
            m_state.deadline = now + *timeout;
         else 
            m_state.deadline = std::nullopt;
      }

      std::optional< common::chronology::time_point> Context::deadline() const
      {
         return m_state.deadline;
      }

      void Context::finalize( std::span< const common::strong::correlation::id> transaction_associated)
      {
         common::Trace trace( "service::call::Context::finalize");

         auto correlations = m_state.pending.finalize();

         auto [ associated, discardable] = common::algorithm::intersection( correlations, transaction_associated);

         common::log::debug( "associated: ", associated, " discardable: ", discardable);

         for( auto& discard: discardable)
            common::communication::ipc::inbound::device().discard( discard);

         while( ! associated.empty())
         {
            auto reply = common::communication::ipc::receive< common::message::service::call::Reply>();
            common::log::debug( "reply: ", reply);

            // disassociate the the call from transaction
            casual::transaction::context().update( reply);

            associated = common::algorithm::remove( associated, reply.correlation);
         }
      }

      bool Context::empty() const
      {
         return m_state.pending.empty();
      }


      bool Context::receive( common::message::service::call::Reply& reply, const common::strong::correlation::id& correlation, reply::Flag flags)
      {
         if( common::flag::contains( flags, reply::Flag::any))
         {
            // We fetch any
            return local::receive( reply, flags);
         }
         else
         {
            return local::receive( reply, flags, correlation);
         }
      }

   } // service::call
} // casual
