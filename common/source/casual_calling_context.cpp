//!
//! casual_calling_context.cpp
//!
//! Created on: Jun 16, 2012
//!     Author: Lazan
//!

#include "casual_calling_context.h"

#include "casual_utility_environment.h"

#include "casual_utility_flag.h"

#include "casual_message.h"
#include "casual_queue.h"
#include "casual_error.h"

#include "xatmi.h"

namespace casual
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
                  unused = value.callDescriptor + 1;
                  return true;
               }
               return false;
            }

            int unused;
         };

      }

      Context& Context::instance()
      {
         static Context singleton;
         return singleton;
      }

      int Context::allocateCallingDescriptor()
      {
         if( m_callingDescriptor < 10)
         {
            //
            // all calling descriptors are used, vi have to reuse.
            // This is highly unlikely, since there have to be std::max< int> pending replies,
            // which is a extreme high number of pending replies. But, we have to handle it...
            //

            local::UnusedCallingDescriptor finder;

            pending_calls_type::iterator findIter = std::find_if( m_pendingReplies.begin(), m_pendingReplies.end(), local::UnusedCallingDescriptor());

            if( findIter == m_pendingReplies.end())
            {
               throw exception::NotReallySureWhatToNameThisException();
            }

            return finder.unused;
         }
         return m_callingDescriptor++;
      }

      int Context::asyncCall( const std::string& service, char* idata, long ilen, long flags)
      {
         try
         {

            //
            // get the buffer
            //
            buffer::Buffer& buffer = buffer::Context::instance().getBuffer( idata);

            //
            // Get a queue corresponding to the service
            //
            message::ServiceRequest serviceLookup;
            serviceLookup.requested = service;
            serviceLookup.server.queue_key = m_receiveQueue.getKey();

            queue::Writer broker( m_brokerQueue);
            broker( serviceLookup);

            message::ServiceResponse serviceResponse;
            queue::blocking::Reader reader( m_receiveQueue);
            reader( serviceResponse);

            if( serviceResponse.server.empty())
            {
               throw exception::NotReallySureWhatToNameThisException();
            }

            //
            // Call the service
            //
            message::ServiceCall messageCall( buffer);
            messageCall.callDescriptor = allocateCallingDescriptor();
            messageCall.reply.queue_key = m_receiveQueue.getKey();

            messageCall.service = serviceResponse.requested;

            //
            // Store the pending-call information
            //
            internal::Pending pendingReply;
            pendingReply.callDescriptor = messageCall.callDescriptor;
            pendingReply.called = utility::environment::getTime();
            m_pendingReplies.insert( pendingReply);

            ipc::send::Queue callQueue( serviceResponse.server.front().queue_key);
            queue::Writer callWriter( callQueue);

            callWriter( messageCall);

            return messageCall.callDescriptor;

         }
         catch( ...)
         {
            return error::handler();
         }
         return 0;
      }

      int Context::getReply( int* idPtr, char** odata, long& olen, long flags)
      {
         try
         {
            //
            // TODO: validate input...

            if( utility::flag< TPGETANY>( flags))
            {
               *idPtr = 0;
            }

            //
            // TODO: Should we care if odata is a valid buffer? As of now, we pretty much
            // have no use for the users buffer.
            //
            // get the buffer
            //
            buffer::Buffer& buffer = buffer::Context::instance().getBuffer( *odata);

            buffer::scoped::Deallocator deallocate( buffer);

            //
            // Vi fetch all on the queue.
            //
            consume();

            //
            // First we treat the call as a NOBLOCK call. Later we block if requested
            //

            //
            //
            //
            reply_cache_type::iterator replyIter = find( *idPtr);

            if( replyIter == m_replyCache.end())
            {
               if( !utility::flag< TPNOBLOCK>( flags))
               {
                  //
                  // We block
                  //
                  replyIter = fetch( *idPtr);
               }
               else
               {
                  //
                  // No message yet for the user...
                  // Don't deallocate buffer
                  //
                  deallocate.release();

               }
            }


            //
            // We deliver the message
            //
            *idPtr = replyIter->first;
            *odata = replyIter->second.getBuffer().raw();
            olen = replyIter->second.getBuffer().size();



         }
         catch( ...)
         {
            return error::handler();
         }

         return 0;
      }

      void Context::clean()
      {
         m_callingDescriptor = 10;

         //
         // TODO: Do some cleaning on buffers, pending replys and such...
         //

      }

      Context::Context()
            : m_brokerQueue( ipc::getBrokerQueue()), m_callingDescriptor( 10)
      {

      }


      ipc::send::Queue& Context::brokerQueue()
      {
         return m_brokerQueue;
      }

      ipc::receive::Queue& Context::receiveQueue()
      {
         return m_receiveQueue;
      }


      Context::reply_cache_type::iterator Context::find( int callDescriptor)
      {
         if( callDescriptor == 0)
         {
            return m_replyCache.begin();
         }
         return m_replyCache.find( callDescriptor);
      }

      namespace local
      {
         namespace scoped
         {
            struct ReplyGuard
            {
               ReplyGuard( message::ServiceReply& reply)
                     : m_reply( reply), m_deallocate( true)
               {
               }

               ~ReplyGuard()
               {
                  if( m_deallocate)
                  {
                     buffer::Context::instance().deallocate( m_reply.getBuffer().raw());
                  }
               }

               void release()
               {
                  m_deallocate = false;
               }

            private:
               message::ServiceReply& m_reply;
               bool m_deallocate;

            };
         }
      }

      Context::reply_cache_type::iterator Context::fetch( int callDescriptor)
      {
         reply_cache_type::iterator fetchIter = find( callDescriptor);

         if( fetchIter == m_replyCache.end())
         {
            queue::blocking::Reader reader( m_receiveQueue);

            do
            {

               message::ServiceReply reply( buffer::Context::instance().create());

               //
               // Use guard if we get a signal.
               //
               local::scoped::ReplyGuard guard( reply);

               reader( reply);

               fetchIter = add( reply);
               guard.release();

            } while( fetchIter->first != callDescriptor);
         }

         return fetchIter;
      }

      Context::reply_cache_type::iterator Context::add( message::ServiceReply& reply)
      {
         // TODO: Check if the received message is pending

         // TODO: Check if the calling descriptor is already received...

         return m_replyCache.insert( std::make_pair( reply.callDescriptor, reply)).first;

      }

      void Context::consume()
      {
         //
         // pop from queue until it's empty (at least empty for callReplies)
         //

         message::ServiceReply reply( buffer::Context::instance().create());
         local::scoped::ReplyGuard guard( reply);

         queue::non_blocking::Reader reader( m_receiveQueue);

         while( reader( reply))
         {

            add( reply);

            //
            // create new buffer
            //
            reply.setBuffer( buffer::Context::instance().create());
         }

      }

   }

}
