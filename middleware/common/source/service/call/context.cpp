//!
//! casual
//!

#include "common/service/call/context.h"
#include "common/service/lookup.h"


#include "common/communication/ipc.h"
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
      namespace service
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

                  } // queue


                  namespace validate
                  {

                     inline message::service::lookup::Request::Context input( const char* buffer, long size, long flags)
                     {
                        Trace trace( "call::validate::input", log::internal::transaction);

                        if( flag< TPNOREPLY>( flags) && ! flag< TPNOTRAN>( flags) && common::transaction::Context::instance().current())
                        {
                           throw exception::xatmi::invalid::Argument{ "TPNOREPLY can only be used with TPNOTRAN"};
                        }

                        return flag< TPNOREPLY>( flags) ? message::service::lookup::Request::Context::no_reply : message::service::lookup::Request::Context::regular;
                     }

                  } // validate


               } // <unnamed>

            } // local




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
                     inline message::service::call::caller::Request message(
                           State& state,
                           const platform::time_point& start,
                           char* idata,
                           long ilen,
                           long flags,
                           const message::service::call::Service& service)
                     {
                        message::service::call::caller::Request message( buffer::pool::Holder::instance().get( idata, ilen));

                        message.correlation = uuid::make();
                        message.service = service;

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
                        else
                        {
                           auto& descriptor = state.pending.reserve( message.correlation);

                           if( ! flag< TPNOTIME>( flags))
                           {
                              descriptor.timeout.set( start, service.timeout);
                           }

                           message.descriptor = descriptor.descriptor;

                           auto& transaction = common::transaction::Context::instance().current();

                           if( ! flag< TPNOTRAN>( flags) && transaction)
                           {
                              message.trid = transaction.trid;
                              transaction.associate( message.correlation);

                              //
                              // We use the transaction deadline if it's earlier
                              //
                              if( transaction.timout.deadline() < descriptor.timeout.deadline())
                              {
                                 descriptor.timeout.set( start, std::chrono::duration_cast< std::chrono::microseconds>( transaction.timout.deadline() - start));
                              }
                           }

                           message.service.timeout = descriptor.timeout.timeout;
                        }

                        message.process = process::handle();
                        message.parent = execution::service::name();
                        message.flags = flags;
                        message.header = service::header::fields();

                        return message;
                     }

                  } // prepare

               } // <unnamed>
            } // local


            descriptor_type Context::async( const std::string& service, char* idata, long ilen, long flags)
            {
               trace::internal::Scope trace( "calling::Context::async");

               log::internal::debug << "input - service: " << service << " data: @" << static_cast< void*>( idata) << " len: " << ilen << " flags: " << flags << std::endl;

               auto context = local::validate::input( idata, ilen, flags);

               service::Lookup lookup( service, context);

               //
               // We do as much as possible while we wait for the broker reply
               //

               auto start = platform::clock_type::now();

               //
               // Invoke pre-transport buffer modifiers
               //
               buffer::transport::Context::instance().dispatch( idata, ilen, service, buffer::transport::Lifecycle::pre_call);



               //
               // Get a queue corresponding to the service
               //
               auto target = lookup();

               if( target.state == message::service::lookup::Reply::State::absent)
               {
                  throw common::exception::xatmi::service::no::Entry( service);
               }

               //
               // The service exists. Take care of reserving descriptor and determine timeout
               //
               auto message = local::prepare::message( m_state, start, idata, ilen, flags, target.service);

               //
               // If some thing goes wrong we unreserve the descriptor
               //
               auto unreserve = common::scope::execute( [&](){ m_state.pending.unreserve( message.descriptor);});


               //
               // If something goes wrong (most likely a timeout), we need to send ack to broker in that case, cus the service(instance)
               // will not do it...
               //
               auto send_ack = common::scope::execute( [&]()
                  {
                     message::service::call::ACK ack;
                     ack.process = target.process;
                     ack.service = target.service.name;
                     communication::ipc::blocking::send( communication::ipc::broker::device(), ack);
                  });


               //
               // Make sure we timeout if we don't keep our deadline
               //
               auto deadline = m_state.pending.deadline( message.descriptor, start);


               if( target.state == message::service::lookup::Reply::State::busy)
               {
                  //
                  // We wait for an instance to become idle.
                  //
                  target = lookup();
               }

               //
               // Call the service
               //
               message.service = target.service;

               log::internal::debug << "async - message: " << message << std::endl;


               communication::ipc::blocking::send( target.process.queue, message);

               unreserve.release();
               send_ack.release();
               return message.descriptor;
            }



            void Context::reply( descriptor_type& descriptor, char** odata, long& olen, long flags)
            {
               trace::internal::Scope trace( "calling::Context::reply");

               log::internal::debug << "descriptor: " << descriptor << " data: @" << static_cast< void*>( *odata) << " len: " << olen << " flags: " << flags << std::endl;

               //
               // TODO: validate input...


               auto start = platform::clock_type::now();

               if( common::flag< TPGETANY>( flags))
               {
                  descriptor = 0;
               }

               //
               // Make sure we timeout if we don't keep our deadline
               //
               auto deadline = m_state.pending.deadline( descriptor, start);


               //
               // We fetch, and if TPNOBLOCK is not set, we block
               //
               message::service::call::Reply reply;

               if( ! receive( reply, descriptor, flags))
               {
                  throw common::exception::xatmi::no::Message();
               }

               log::internal::debug << "reply: " << reply << '\n';

               descriptor = reply.descriptor;

               user_code( reply.code);


               //
               // We unreserve pending (at end of scope, regardless of outcome)
               //
               auto discard = scope::execute( [&](){ m_state.pending.unreserve( descriptor);});

               //
               // Update transaction state
               //
               common::transaction::Context::instance().update( reply);


               //
               // Check any errors
               //
               if( reply.error != 0 && reply.error != TPESVCFAIL)
               {
                  exception::xatmi::propagate( reply.error);
               }


               //
               // Check buffer types
               //
               if( *odata != nullptr && common::flag< TPNOCHANGE>( flags))
               {
                  auto output = buffer::pool::Holder::instance().get( *odata);

                  if( output.payload().type != reply.buffer.type)
                  {
                     throw exception::xatmi::buffer::type::Output{};
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

               if( reply.error == TPESVCFAIL)
               {
                  throw exception::xatmi::service::Fail{};
               }

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

            bool Context::pending() const
            {
               return ! m_state.pending.empty();
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
                        return communication::ipc::non::blocking::receive( communication::ipc::inbound::device(), reply, args...);
                     }
                     else
                     {
                        communication::ipc::blocking::receive( communication::ipc::inbound::device(), reply, args...);
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
                  auto& correlation = m_state.pending.get( descriptor).correlation;

                  return local::receive( reply, flags, correlation);
               }
            }
         } // call
      } // service
   } // common
} // casual
