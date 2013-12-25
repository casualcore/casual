//!
//! casual_calling_context.cpp
//!
//! Created on: Jun 16, 2012
//!     Author: Lazan
//!

#include "common/calling_context.h"
#include "common/message.h"
#include "common/queue.h"
#include "common/log.h"


#include "common/environment.h"
#include "common/flag.h"
#include "common/error.h"
#include "common/exception.h"
#include "common/trace.h"
#include "common/signal.h"

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
            struct UnusedCallingDescriptor
            {
               UnusedCallingDescriptor()
                     : unused( 10)
               {
               }

               bool operator()( const internal::Pending& value)
               {
                  if( value.callDescriptor - unused > 1)
                  {
                     ++unused;
                     return true;
                  }

                  unused = value.callDescriptor + 1;
                  return false;
               }

               int unused;
            };


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

         } // local


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

         int Context::allocateCallingDescriptor()
         {
            if( m_callingDescriptor < 10)
            {
               //
               // all calling descriptors are used, vi have to reuse.
               // This is highly unlikely, since there have to be std::max< int> pending replies,
               // which is a extreme high number. But, we have to handle it...
               //
               // TODO: We don't try to handle it until we can unit test it.
               throw common::exception::xatmi::LimitReached( "No free call descriptors");

               /*
               // TODO: this is not standard-conformant...
               local::UnusedCallingDescriptor finder;

               pending_calls_type::iterator findIter = std::find_if(
                     m_pendingReplies.begin(),
                     m_pendingReplies.end(),
                     finder);

               if( findIter == m_pendingReplies.end())
               {
                  throw exception::xatmi::LimitReached( "No free call descriptors");
               }

               return finder.unused;
               */
            }
            return m_callingDescriptor++;
         }

         int Context::asyncCall( const std::string& service, char* idata, long ilen, long flags)
         {
            common::Trace trace( "calling::Context::asyncCall");
            common::log::debug << "service: " << service << " data: @" << static_cast< void*>( idata) << " len: " << ilen << " flags: " << flags << std::endl;



            // TODO validate



            const int callDescriptor = allocateCallingDescriptor();

            const common::platform::seconds_type time = common::environment::getTime();


            //
            // Get a queue corresponding to the service
            //
            message::service::name::lookup::Reply lookup = serviceQueue( service);

            if( lookup.server.empty())
            {
               throw common::exception::xatmi::service::NoEntry( service);
            }

            //
            // Keep track of (ev.) coming timeouts
            //
            local::PendingTimeout::instance().add(
                  local::Timeout( callDescriptor, lookup.service.timeout, time));


            //
            // Call the service
            //

            message::service::caller::Call messageCall( buffer::Context::instance().get( idata));
            messageCall.callDescriptor = callDescriptor;
            messageCall.reply.queue_id = ipc::getReceiveQueue().id();
            messageCall.service = lookup.service;
            messageCall.callId = m_state.callId;
            messageCall.callee = m_state.currentService;


            //ipc::send::Queue callQueue( lookup.server.front().queue_id);
            queue::blocking::Writer callWriter( lookup.server.front().queue_id);

            local::timeoutWrapper( callWriter, messageCall, callDescriptor);


            //
            // Add the descriptor to pending
            //
            m_state.pendingCalls.insert( callDescriptor);

            return callDescriptor;
         }



         int Context::getReply( int* idPtr, char** odata, long& olen, long flags)
         {
            common::Trace trace( "calling::Context::getReply");
            common::log::debug << "cd: " << *idPtr << " data: @" << static_cast< void*>( *odata) << " len: " << olen << " flags: " << flags << std::endl;



            //
            // TODO: validate input...

            if( common::flag< TPGETANY>( flags))
            {
               *idPtr = 0;
            }
            else
            {
               if( m_state.pendingCalls.find( *idPtr) == m_state.pendingCalls.end())
               {
                  throw common::exception::xatmi::service::InvalidDescriptor();
               }
            }

            //
            // TODO: Should we care if odata is a valid buffer? As of now, we pretty much
            // have no use for the users buffer.
            //


            //
            // Vi fetch all on the queue.
            //
            consume();

            //
            // First we treat the call as a NOBLOCK call. Later we block if requested
            //

            auto replyIter = find( *idPtr);

            if( replyIter == m_state.replyCache.end())
            {
               if( !common::flag< TPNOBLOCK>( flags))
               {
                  //
                  // We block
                  //
                  replyIter = fetch( *idPtr);
               }
               else
               {
                  throw common::exception::xatmi::NoMessage();
               }
            }

            message::service::Reply reply = std::move( replyIter->second);

            //
            // Get the user allocated buffer
            //
            buffer::Buffer userBuffer = buffer::Context::instance().extract( *odata);

            // TODO: Check if the user accept different buffer-types, and compare types
            // i.e. reply.buffer == userBuffer

            //
            // We deliver the message
            //
            *idPtr = reply.callDescriptor;
            *odata = platform::public_buffer( reply.buffer.raw());
            olen = reply.buffer.size();


            // TOOD: Temp
            common::log::debug << "cd: " << *idPtr << " buffer: " << static_cast< void*>( *odata) << " size: " << olen << std::endl;

            //
            // Add the buffer to the pool
            //
            buffer::Context::instance().add( std::move( reply.buffer));



            //
            // We remove our representation
            //
            m_state.pendingCalls.erase( *idPtr);
            m_state.replyCache.erase( replyIter);

            //
            // Check if there has been an timeout
            //
            local::PendingTimeout::instance().check( *idPtr);


            return 0;
         }

         void Context::clean()
         {
            m_callingDescriptor = 10;

            //
            // TODO: Do some cleaning on buffers, pending replies and such...
            //

         }

         Context::Context()
         {

         }


         Context::reply_cache_type::iterator Context::find( int callDescriptor)
         {
            if( callDescriptor == 0)
            {
               return m_state.replyCache.begin();
            }
            return m_state.replyCache.find( callDescriptor);
         }



         Context::reply_cache_type::iterator Context::fetch( int callDescriptor)
         {
            reply_cache_type::iterator fetchIter = find( callDescriptor);

            if( fetchIter == m_state.replyCache.end())
            {
               queue::blocking::Reader reader( ipc::getReceiveQueue());

               do
               {

                  message::service::Reply reply;

                  local::timeoutWrapper( reader, reply, callDescriptor);

                  fetchIter = add( std::move( reply));

               } while( fetchIter->first != callDescriptor);
            }

            return fetchIter;
         }

         Context::reply_cache_type::iterator Context::add( message::service::Reply&& reply)
         {
            // TODO: Check if the received message is pending

            // TODO: Check if the calling descriptor is already received...

            return m_state.replyCache.emplace( reply.callDescriptor, std::move( reply)).first;

         }

         message::service::name::lookup::Reply Context::serviceQueue( const std::string& service)
         {
            //
            // Get a queue corresponding to the service
            //
            auto& receiveQueue = ipc::getReceiveQueue();

            message::service::name::lookup::Request serviceLookup;
            serviceLookup.requested = service;
            serviceLookup.server.queue_id = receiveQueue.id();

            queue::blocking::Writer broker( ipc::getBrokerQueue().id());
            local::timeoutWrapper( broker, serviceLookup);


            message::service::name::lookup::Reply result;
            queue::blocking::Reader reader( receiveQueue);
            local::timeoutWrapper( reader, result);

            return result;

         }



         void Context::consume()
         {
            //
            // pop from queue until it's empty (at least empty for callReplies)
            //

            while( true)
            {
               message::service::Reply reply;

               queue::non_blocking::Reader reader( ipc::getReceiveQueue());

               if( ! local::timeoutWrapper( reader, reply))
               {
                  break;
               }

               add( std::move( reply));
            }

         }
      } // calling
   } // common
} // casual
