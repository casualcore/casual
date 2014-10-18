//!
//! casual_calling_context.cpp
//!
//! Created on: Jun 16, 2012
//!     Author: Lazan
//!

#include "common/calling_context.h"

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
      namespace calling
      {
         namespace local
         {

            class Timeout
            {
            public:
               typedef common::platform::seconds_type seconds_type;

               Timeout( int a_callDescriptor, seconds_type a_timeout, seconds_type a_registred)
                  : callDescriptor( a_callDescriptor), timeout( a_timeout), registred( a_registred) {}



               common::platform::seconds_type endTime() const
               {
                  return registred + timeout;
               }

               bool operator < ( const Timeout& rhs) const
               {
                  return endTime() < rhs.endTime();
               }

               struct Transform
               {
                  int operator () ( const Timeout& value) const
                  {
                     return value.callDescriptor;
                  }
               };

               const int callDescriptor;
               const seconds_type timeout;
               const seconds_type registred;


            };


            // TOOD: This type should probably be in it's own TU
            class PendingTimeout
            {
            public:
               static PendingTimeout& instance()
               {
                  static PendingTimeout singleton;
                  return singleton;
               }

               void add( const Timeout& pending)
               {
                  //
                  // We only register if there is a timeout
                  //
                  if( pending.timeout != 0)
                  {

                     if( m_pending.insert( pending).first == m_pending.begin())
                     {
                        //
                        // The pending we inserted is the one that will timeout first, we have
                        // to (re)set the alarm
                        //
                        const common::platform::seconds_type timeout = pending.endTime() - common::environment::getTime();

                        common::signal::alarm::set( timeout > 0 ? timeout : 1);
                     }

                  }
               }



               void timeout()
               {
                  assert( !m_pending.empty());

                  const common::platform::seconds_type time = common::environment::getTime();

                  //
                  // Find all (at least one) pending that has a timeout.
                  //
                  std::set< Timeout>::iterator passedEnd = std::find_if(
                        m_pending.begin(),
                        m_pending.end(),
                        Passed( time));

                  //
                  // TODO: There might be a glitch if the timeout is set very close to a "full second", not sure
                  // if we are safe.
                  assert( passedEnd != m_pending.begin());

                  //
                  // Transfer the pending to timeouts...
                  //
                  std::transform(
                        m_pending.begin(),
                        passedEnd,
                        std::inserter( m_timeouts, m_timeouts.begin()),
                        Timeout::Transform());

                  m_pending.erase(
                        m_pending.begin(),
                        passedEnd);

                  if( !m_pending.empty())
                  {
                     //
                     // register a new alarm
                     //
                     common::signal::alarm::set( m_pending.begin()->endTime() - time);

                  }
                  else
                  {
                     //
                     // No more pending, reset the alarm
                     //
                     common::signal::alarm::set( 0);
                  }
               }

               void check( const int callDescriptor)
               {
                  std::set< int>::iterator findIter = m_timeouts.find( callDescriptor);

                  if( findIter != m_timeouts.end())
                  {
                     m_timeouts.erase( findIter);
                     throw common::exception::xatmi::Timeout();
                  }
               }

            private:

               struct Passed
               {
                  Passed( common::platform::seconds_type time) : m_time( time) {}

                  bool operator () ( const Timeout& value)
                  {
                     return m_time > value.endTime();
                  }
               private:
                  const common::platform::seconds_type m_time;
               };


               std::set< Timeout> m_pending;
               std::set< int> m_timeouts;
            };


            template< typename Q, typename T>
            auto timeoutWrapper( Q& queue, T& value) -> decltype( queue( value))
            {
               try
               {
                  return queue( value);
               }
               catch( const common::exception::signal::Timeout&)
               {
                  PendingTimeout::instance().timeout();
                  return timeoutWrapper( queue, value);
               }
            }

            template< typename Q, typename T>
            auto timeoutWrapper( Q& queue, T& value, int callDescriptor) -> decltype( queue( value))
            {
               try
               {
                  return queue( value);
               }
               catch( const common::exception::signal::Timeout&)
               {
                  PendingTimeout::instance().timeout();
                  PendingTimeout::instance().check( callDescriptor);

                  return timeoutWrapper( queue, value, callDescriptor);
               }
            }


            namespace service
            {
               namespace lookup
               {
                  void request( const std::string& service)
                  {
                     message::service::name::lookup::Request serviceLookup;
                     serviceLookup.requested = service;
                     serviceLookup.process = process::handle();

                     queue::blocking::Writer broker( ipc::broker::id());
                     local::timeoutWrapper( broker, serviceLookup);

                  }

                  message::service::name::lookup::Reply reply()
                  {
                     message::service::name::lookup::Reply result;
                     queue::blocking::Reader reader( ipc::receive::queue());
                     local::timeoutWrapper( reader, result);

                     return result;
                  }

               } // lookup
            } // service

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
               throw exception::NotReallySureWhatToNameThisException{ "wrong call descriptor"};
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

         void Context::callId( const common::Uuid& uuid)
         {
            if( uuid == common::Uuid::empty())
            {
               m_state.callId = common::Uuid::make();
            }
            else
            {
               m_state.callId = uuid;
            }
         }

         const common::Uuid& Context::callId() const
         {
            return m_state.callId;
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


            const common::platform::seconds_type time = common::environment::getTime();


            message::service::caller::Call message( buffer::pool::Holder::instance().get( idata));
            message.callDescriptor = descriptor;
            message.reply = process::handle();
            message.trid = transaction::Context::instance().currentTransaction().trid;
            message.callId = m_state.callId;
            message.callee = m_state.currentService;


            //
            // Get a queue corresponding to the service
            //
            auto lookup = local::service::lookup::reply();

            if( lookup.server.empty())
            {
               throw common::exception::xatmi::service::NoEntry( service);
            }

            //
            // Keep track of (ev.) coming timeouts
            //
            local::PendingTimeout::instance().add(
                  local::Timeout( descriptor, lookup.service.timeout, time));


            //
            // Call the service
            //

            message.service = lookup.service;

            queue::blocking::Writer callWriter( lookup.server.front().queue);

            local::timeoutWrapper( callWriter, message, descriptor);


            return descriptor;
         }



         int Context::getReply( int* idPtr, char** odata, long& olen, long flags)
         {
            trace::internal::Scope trace( "calling::Context::getReply");

            log::internal::debug << "cd: " << *idPtr << " data: @" << static_cast< void*>( *odata) << " len: " << olen << " flags: " << flags << std::endl;

            //
            // TODO: validate input...

            //decltype( range::make( m_state.pendingCalls)) pending;


            if( common::flag< TPGETANY>( flags))
            {
               *idPtr = 0;
            }
            else
            {
               if( ! m_state.pending.active( *idPtr))
               {
                  throw common::exception::xatmi::service::InvalidDescriptor();
               }
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


            //
            // Check if there has been an timeout
            //
            local::PendingTimeout::instance().check( *idPtr);


            return 0;
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

            while( ! found && ! common::flag< TPNOBLOCK>( flags))
            {
               queue::blocking::Reader reader( ipc::receive::queue());

               message::service::Reply reply;

               local::timeoutWrapper( reader, reply, descriptor);

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

            while( true)
            {
               message::service::Reply reply;

               queue::non_blocking::Reader reader( ipc::receive::queue());

               if( ! local::timeoutWrapper( reader, reply))
               {
                  break;
               }

               m_state.reply.cache.add( std::move( reply));
            }

         }
      } // calling
   } // common
} // casual
