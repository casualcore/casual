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
                     Policy( platform::descriptor_type descriptor) : m_descriptor( descriptor) {}
                     Policy() = default;

                     void apply()
                     {
                        try
                        {
                           throw;
                        }
                        catch( const exception::signal::Timeout&)
                        {
                           if( Timeout::instance().passed( m_descriptor))
                           {
                              Timeout::instance().remove( m_descriptor);
                              throw exception::xatmi::Timeout{};
                           }
                        }
                     }
                  private:
                     platform::descriptor_type m_descriptor = Timeout::Type::cTransaction;
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
                  namespace lookup
                  {
                     void request( const std::string& service)
                     {
                        message::service::name::lookup::Request serviceLookup;
                        serviceLookup.requested = service;
                        serviceLookup.process = process::handle();

                        queue::blocking::Send send;
                        send( ipc::broker::id(), serviceLookup);

                     }

                     message::service::name::lookup::Reply reply()
                     {
                        message::service::name::lookup::Reply result;
                        queue::blocking::Receive receive( ipc::receive::queue());
                        receive( result);

                        return result;
                     }

                  } // lookup
               } // service

            } // <unnamed>

         } // local


         int State::Pending::reserve()
         {
            int current = 0;

            auto found = range::find_if( m_descriptors, [&]( const Descriptor& d){
               current = d.descriptor;
               return ! d.active;
            });

            if( ! found)
            {
               m_descriptors.emplace_back( current + 1, true);
               found = range::back( m_descriptors);
            }

            found->active = true;
            return found->descriptor;
         }

         void State::Pending::unreserve( descriptor_type descriptor)
         {
            auto found = range::find( m_descriptors, descriptor);

            if( found)
            {
               found->active = false;
            }
            else
            {
               throw exception::invalid::Argument{ "wrong call descriptor: " + std::to_string( descriptor)};
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


         State::Reply::Cache::cache_range State::Reply::Cache::add( message::service::Reply&& value)
         {
            m_cache.push_back( std::move( value));
            return range::back( m_cache);
         }

         State::Reply::Cache::cache_range State::Reply::Cache::search( descriptor_type descriptor)
         {
            return range::find_if( m_cache, [=]( const cache_type::value_type& r){
               return r.callDescriptor == descriptor;
            });

         }

         void State::Reply::Cache::erase( cache_range range)
         {
            if( range)
            {
               m_cache.erase( range.first);
            }
         }




         Context& Context::instance()
         {
            static Context singleton;
            return singleton;
         }

         void Context::execution( const Uuid& uuid)
         {
            if( ! uuid)
            {
               m_state.execution = uuid::make();
            }
            else
            {
               m_state.execution = uuid;
            }
         }

         const common::Uuid& Context::execution() const
         {
            return m_state.execution;
         }



         void Context::currentService( const std::string& service)
         {
            m_state.currentService = service;
         }

         const std::string& Context::currentService() const
         {
            return m_state.currentService;
         }



         int Context::asyncCall( const std::string& service, char* idata, long ilen, long flags)
         {
            trace::internal::Scope trace( "calling::Context::asyncCall");

            // TODO validate

            local::service::lookup::request( service);

            //
            // We do as much as possible while we wait for the broker reply
            //

            const auto descriptor = m_state.pending.reserve();

            log::internal::debug << "cd: " << descriptor << " service: " << service << " data: @" << static_cast< void*>( idata) << " len: " << ilen << " flags: " << flags << std::endl;


            //const common::platform::seconds_type time = common::environment::getTime();


            message::service::caller::Call message( buffer::pool::Holder::instance().get( idata));
            message.descriptor = descriptor;
            message.reply = process::handle();
            message.trid = transaction::Context::instance().currentTransaction().trid;
            message.execution = m_state.execution;
            message.callee = m_state.currentService;



            //
            // Get a queue corresponding to the service
            //
            auto lookup = local::service::lookup::reply();

            if( ! lookup.supplier)
            {
               throw common::exception::xatmi::service::NoEntry( service);
            }

            //
            // Keep track of timeouts
            //
            Timeout::instance().add( descriptor, lookup.service.timeout);


            //
            // Call the service
            //
            message.service = lookup.service;

            local::queue::blocking::Send send{ descriptor};
            send( lookup.supplier.queue, message);

            return descriptor;
         }



         void Context::getReply( int* idPtr, char** odata, long& olen, long flags)
         {
            trace::internal::Scope trace( "calling::Context::getReply");

            log::internal::debug << "cd: " << *idPtr << " data: @" << static_cast< void*>( *odata) << " len: " << olen << " flags: " << flags << std::endl;

            //
            // TODO: validate input...


            if( common::flag< TPGETANY>( flags))
            {
               *idPtr = 0;
            }
            else if( ! m_state.pending.active( *idPtr))
            {
               throw common::exception::xatmi::service::InvalidDescriptor();
            }



            //
            // We fetch from cache, and if TPNOBLOCK is not set, we block
            //
            auto found = fetch( *idPtr, flags);

            if( ! found)
            {
               throw common::exception::xatmi::NoMessage();
            }

            message::service::Reply reply = std::move( *found);
            m_state.reply.cache.erase( found);


            //
            // Check buffer types
            //
            if( *odata != nullptr && common::flag< TPNOCHANGE>( flags))
            {
               auto& output = buffer::pool::Holder::instance().get( *odata);

               if( output.type != reply.buffer.type)
               {
                  throw exception::xatmi::buffer::TypeNotExpected{};
               }

               buffer::pool::Holder::instance().deallocate( *odata);
               *odata = nullptr;
            }

            //
            // We deliver the message
            //
            *idPtr = reply.callDescriptor;
            *odata = reply.buffer.memory.data();
            olen = reply.buffer.memory.size();


            // TOOD: Temp
            log::internal::debug << "cd: " << *idPtr << " buffer: " << static_cast< void*>( *odata) << " size: " << olen << std::endl;

            //
            // Add the buffer to the pool
            //
            buffer::pool::Holder::instance().insert( std::move( reply.buffer));

            //
            // We remove pending
            //
            m_state.pending.unreserve( *idPtr);

            if( Timeout::instance().remove( *idPtr))
            {
               throw exception::xatmi::Timeout{ "descriptor " + std::to_string( *idPtr) + " reached deadline", __FILE__, __LINE__};
            }

         }

         int Context::canccel( int cd)
         {
            // TODO:
            return 0;
         }

         void Context::clean()
         {

            //
            // TODO: Do some cleaning on buffers, pending replies and such...
            //

         }

         Context::Context()
         {

         }



         Context::cache_range Context::fetch( State::descriptor_type descriptor, long flags)
         {
            //
            // Vi fetch all on the queue.
            //
            consume();

            auto found = m_state.reply.cache.search( descriptor);

            local::queue::blocking::Receive receive{ ipc::receive::queue(), descriptor};

            while( ! found && ! common::flag< TPNOBLOCK>( flags))
            {
               message::service::Reply reply;
               receive( reply);

               auto cached = m_state.reply.cache.add( std::move( reply));

               if( cached->callDescriptor == descriptor)
               {
                  return cached;
               }
            }
            return found;
         }




         void Context::consume()
         {
            //
            // pop from queue until it's empty (at least empty for callReplies)
            //

            local::queue::non_blocking::Receive receive{ ipc::receive::queue()};

            while( true)
            {
               message::service::Reply reply;

               if( ! receive( reply))
               {
                  break;
               }

               m_state.reply.cache.add( std::move( reply));
            }

         }
      } // call
   } // common
} // casual
