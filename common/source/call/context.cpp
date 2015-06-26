//!
//! casual_calling_context.cpp
//!
//! Created on: Jun 16, 2012
//!     Author: Lazan
//!

#include "common/call/context.h"
#include "common/call/timeout.h"

#include "common/queue.h"
#include "common/internal/log.h"
#include "common/internal/trace.h"

#include "common/buffer/pool.h"
#include "common/buffer/transport.h"

#include "common/environment.h"
#include "common/flag.h"
#include "common/error.h"
#include "common/exception.h"
#include "common/signal.h"

#include "common/transaction/context.h"

#include "xatmi.h"

//
// std
//
#include <algorithm>
#include <cassert>

namespace casual
{
   namespace common
   {
      namespace call
      {
         namespace local
         {
            namespace
            {

               namespace queue
               {
                  struct Policy
                  {
                     void apply()
                     {
                        try
                        {
                           throw;
                        }
                        catch( const exception::signal::Timeout&)
                        {
                           throw exception::xatmi::Timeout{};
                        }
                     }
                  };

                  namespace blocking
                  {
                     using Send = common::queue::blocking::basic_send< Policy>;
                     using Receive = common::queue::blocking::basic_reader< Policy>;

                  } // blocking

                  namespace non_blocking
                  {
                     using Receive = common::queue::non_blocking::basic_reader< Policy>;

                  } // non_blocking

               } // queue


               namespace service
               {
                  struct Lookup
                  {
                     Lookup( const std::string& service)
                     {
                        message::service::name::lookup::Request serviceLookup;
                        serviceLookup.requested = service;
                        serviceLookup.process = process::handle();

                        queue::blocking::Send send;
                        send( ipc::broker::id(), serviceLookup);
                     }

                     message::service::name::lookup::Reply operator () () const
                     {
                        message::service::name::lookup::Reply result;
                        queue::blocking::Receive receive( ipc::receive::queue());
                        receive( result);

                        return result;
                     }
                  };

               } // service


               namespace validate
               {

                  inline void input( const char* buffer, long size, long flags)
                  {
                     if( buffer == nullptr)
                     {
                        throw exception::xatmi::InvalidArguments{ "buffer is null"};
                     }
                     if( flag< TPNOREPLY>( flags) && ! flag< TPNOTRAN>( flags))
                     {
                        throw exception::xatmi::InvalidArguments{ "TPNOREPLY can only be used with TPNOTRAN"};
                     }
                  }

               } // validate


            } // <unnamed>

         } // local

         State::Pending::Pending()
          : m_descriptors{
            { 1, false },
            { 2, false },
            { 3, false },
            { 4, false },
            { 5, false },
            { 6, false },
            { 7, false },
            { 8, false }}
         {

         }


         descriptor_type State::Pending::reserve( const Uuid& correlation)
         {
            auto descriptor = reserve();

            m_correlations.emplace_back( descriptor, correlation);

            return descriptor;
         }

         descriptor_type State::Pending::reserve()
         {
            auto found = range::find_if( m_descriptors, negate( std::mem_fn( &Descriptor::active)));

            if( found)
            {
               found->active = true;
               return found->descriptor;
            }
            else
            {
               m_descriptors.emplace_back( m_descriptors.back().descriptor + 1, true);
               return m_descriptors.back().descriptor;
            }
         }

         void State::Pending::unreserve( descriptor_type descriptor)
         {

            {
               auto found = range::find( m_descriptors, descriptor);

               if( found)
               {
                  found->active = false;
               }
               else
               {
                  throw exception::xatmi::service::InvalidDescriptor{ "invalid call descriptor: " + std::to_string( descriptor)};
               }
            }

            //
            // Remove message correlation association
            //
            {
               auto found = range::find( m_correlations, descriptor);
               if( found)
               {
                  m_correlations.erase( found.first);
               }
            }
         }

         bool State::Pending::active( descriptor_type descriptor) const
         {
            auto found = range::find( m_descriptors, descriptor);

            if( found)
            {
               return found->active;
            }
            return false;
         }

         const Uuid& State::Pending::correlation( descriptor_type descriptor) const
         {
            auto found = range::find( m_correlations, descriptor);
            if( found)
            {
               return found->correlation;
            }
            throw exception::xatmi::service::InvalidDescriptor{ "invalid call descriptor: " + std::to_string( descriptor)};
         }

         void State::Pending::discard( descriptor_type descriptor)
         {
            //
            // Can't be associated with a transaction
            //
            if( transaction::Context::instance().associated( descriptor))
            {
               throw exception::xatmi::TransactionNotSupported{ "descriptor " + std::to_string( descriptor) + " is associated with a transaction"};
            }

            //
            // Discards the correlation (directly if in cache, or later if not)
            //
            ipc::receive::queue().discard( correlation( descriptor));

            unreserve( descriptor);
         }


         Context& Context::instance()
         {
            static Context singleton;
            return singleton;
         }

         namespace local
         {
            namespace
            {
               namespace descriptor
               {
                  struct Discard
                  {
                     Discard( State& state, platform::descriptor_type descriptor)
                      : m_state( state), m_descriptor( descriptor) {}

                     ~Discard()
                     {
                        if( m_descriptor != 0)
                        {
                           m_state.pending.unreserve( m_descriptor);
                        }
                     }

                     platform::descriptor_type release()
                     {
                        auto result = m_descriptor;
                        m_descriptor = 0;
                        return result;
                     }

                     State& m_state;
                     platform::descriptor_type m_descriptor;
                  };

               } // descriptor

            } // <unnamed>
         } // local


         descriptor_type Context::async( const std::string& service, char* idata, long ilen, long flags)
         {
            trace::internal::Scope trace( "calling::Context::async");

            log::internal::debug << "input - service: " << service << " data: @" << static_cast< void*>( idata) << " len: " << ilen << " flags: " << flags << std::endl;

            local::validate::input( idata, ilen, flags);

            local::service::Lookup lookup( service);

            //
            // We do as much as possible while we wait for the broker reply
            //

            auto start = platform::clock_type::now();

            //
            // Invoke pre-transport buffer modifiers
            //
            buffer::transport::Context::instance().dispatch( idata, ilen, service, buffer::transport::Lifecycle::pre_call);

            message::service::call::caller::Request message( buffer::pool::Holder::instance().get( idata, ilen));

            //
            // Prepare message
            //
            {

               message.correlation = uuid::make();

               auto& transaction = transaction::Context::instance().current();

               //
               // Check if we should associate descriptor with message-correlation and transaction
               //
               if( flag< TPNOREPLY>( flags))
               {
                  //
                  // No reply, hence no descriptor and no transaction (we validated this before)
                  //
                  message.descriptor = 0;
               }
               else if( ! flag< TPNOTRAN>( flags) && transaction)
               {
                  message.descriptor = m_state.pending.reserve( message.correlation);
                  transaction.descriptors.push_back( message.descriptor);
                  message.trid = transaction.trid;
               }
               else
               {
                  message.descriptor = m_state.pending.reserve( message.correlation);
               }

               message.process = process::handle();
               //message.execution = execution::id();
               message.parent = execution::service();
               message.flags = flags;

               log::internal::debug << "descriptor: " << message.descriptor << " service: " << service << " data: @" << static_cast< void*>( idata) << " len: " << ilen << " flags: " << flags << std::endl;
            }


            //
            // If some thing goes wrong we unreserve the descriptor
            //
            common::scope::Execute unreserve{ [&](){ m_state.pending.unreserve( message.descriptor);}};


            //
            // Get a queue corresponding to the service
            //
            auto target = lookup();

            if( ! target.process)
            {
               throw common::exception::xatmi::service::NoEntry( service);
            }



            //
            // Keep track of timeouts
            //
            // TODO:
            //
            if( message.descriptor != 0)
            {
               Timeout::instance().add(
                     message.descriptor,
                     flag< TPNOTIME>( flags) ? std::chrono::microseconds{ 0} : target.service.timeout,
                     start);
            }

            //
            // If something goes wrong (most likely a timeout), we need to send ack to broker in that case, cus the service(instance)
            // will not do it...
            //
            common::scope::Execute send_ack{ [&]()
               {
                  message::service::call::ACK ack;
                  ack.process = target.process;
                  ack.service = target.service.name;
                  local::queue::blocking::Send send;
                  send( common::ipc::broker::id(), ack);
               }};


            //
            // Call the service
            //
            message.service = target.service;

            local::queue::blocking::Send send;
            send( target.process.queue, message);

            unreserve.release();
            send_ack.release();
            return message.descriptor;
         }



         void Context::reply( descriptor_type& descriptor, char** odata, long& olen, long flags)
         {
            trace::internal::Scope trace( "calling::Context::getReply");

            log::internal::debug << "descriptor: " << descriptor << " data: @" << static_cast< void*>( *odata) << " len: " << olen << " flags: " << flags << std::endl;

            //
            // TODO: validate input...


            if( common::flag< TPGETANY>( flags))
            {
               descriptor = 0;
            }
            else if( ! m_state.pending.active( descriptor))
            {
               throw common::exception::xatmi::service::InvalidDescriptor();
            }

            //
            // Keep track of the current timeout for this descriptor.
            // and make sure we unset the timer regardless
            //
            call::Timeout::Unset unset( descriptor);


            //
            // We fetch, and if TPNOBLOCK is not set, we block
            //
            message::service::call::Reply reply;

            if( ! receive( reply, descriptor, flags))
            {
               throw common::exception::xatmi::NoMessage();
            }

            descriptor = reply.descriptor;

            //
            // We unreserve pending (at end of scope, regardless of outcome)
            //
            common::scope::Execute discard{ [&](){ m_state.pending.unreserve( descriptor);}};

            //
            // This call is consumed, so we remove the timeout.
            //
            Timeout::instance().remove( reply.descriptor);

            //
            // Update transaction state
            //
            transaction::Context::instance().update( reply);

            //
            // Check buffer types
            //
            if( *odata != nullptr && common::flag< TPNOCHANGE>( flags))
            {
               auto output = buffer::pool::Holder::instance().get( *odata);

               if( output.payload.type != reply.buffer.type)
               {
                  throw exception::xatmi::buffer::TypeNotExpected{};
               }
            }

            //
            // We always deallocate user output buffer
            //
            {
               buffer::pool::Holder::instance().deallocate( *odata);
               *odata = nullptr;
            }

            //
            // We prepare the output buffer
            //
            {
               *odata = reply.buffer.memory.data();
               olen = reply.buffer.memory.size();

               //
               // Apply post transport buffer manipulation
               // TODO: get service-name
               //
               buffer::transport::Context::instance().dispatch(
                     *odata, olen, "", buffer::transport::Lifecycle::post_call, reply.buffer.type);

               //
               // Add the buffer to the pool
               //
               buffer::pool::Holder::instance().insert( std::move( reply.buffer));
            }

            log::internal::debug << "descriptor: " << reply.descriptor << " data: @" << static_cast< void*>( *odata) << " len: " << olen << " flags: " << flags << std::endl;

         }


         void Context::sync( const std::string& service, char* idata, const long ilen, char*& odata, long& olen, const long flags)
         {
            //
            // casual always has 'block-semantics' in sync call. We remove possible no-block
            //
            auto supported_flags = ( ~TPNOBLOCK) & flags;

            auto descriptor = async( service, idata, ilen, supported_flags);
            reply( descriptor, &odata, olen, supported_flags);
         }


         void Context::cancel( descriptor_type descriptor)
         {
            m_state.pending.discard( descriptor);
         }


         void Context::clean()
         {

            //
            // TODO: Do some cleaning on buffers, pending replies and such...
            //

         }

         long Context::user_code() const
         {
            return m_state.user_code;
         }
         void Context::user_code( long code)
         {
            m_state.user_code = code;
         }

         Context::Context()
         {

         }

         namespace local
         {
            namespace
            {
               template< typename... Args>
               bool receive( message::service::call::Reply& reply, long flags, Args&... args)
               {
                  if( common::flag< TPNOBLOCK>( flags))
                  {
                     local::queue::non_blocking::Receive receive{ ipc::receive::queue()};
                     return receive( reply, args...);
                  }
                  else
                  {
                     local::queue::blocking::Receive receive{ ipc::receive::queue()};
                     receive( reply, args...);
                  }
                  return true;
               }
            } // <unnamed>
         } // local

         bool Context::receive( message::service::call::Reply& reply, descriptor_type descriptor, long flags)
         {
            if( descriptor == 0)
            {
               //
               // We fetch any
               //
               return local::receive( reply, flags);
            }
            else
            {
               auto& correlation = m_state.pending.correlation( descriptor);

               return local::receive( reply, flags, correlation);
            }
         }
      } // call
   } // common
} // casual
