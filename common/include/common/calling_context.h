//!
//! casual_calling_context.h
//!
//! Created on: Jun 16, 2012
//!     Author: Lazan
//!

#ifndef CASUAL_CALLING_CONTEXT_H_
#define CASUAL_CALLING_CONTEXT_H_


#include "common/queue.h"
#include "common/platform.h"

#include <set>
#include <unordered_map>
#include <unordered_set>

namespace casual
{
   namespace common
   {


      namespace server
      {
         class Context;
      }

      namespace calling
      {
         namespace internal
         {
            struct Pending
            {
               Pending() : callDescriptor( 0), called( 0), timeout( 0) {}
               typedef common::platform::seconds_type seconds_type;

               int callDescriptor;
               seconds_type called;
               seconds_type timeout;


            };
         }


         struct State
         {
            typedef std::unordered_set< int> pending_calls_type;
            typedef std::unordered_map< int, message::service::Reply> reply_cache_type;

            pending_calls_type pendingCalls;
            reply_cache_type replyCache;

            common::Uuid callId = common::Uuid::make();

            int currentCallingDescriptor;

            std::string currentService;
         };

         class Context
         {
         public:
            static Context& instance();




            int asyncCall( const std::string& service, char* idata, long ilen, long flags);

            int getReply( int* idPtr, char** odata, long& olen, long flags);

            void clean();

            void callId( const common::Uuid& uuid);
            const common::Uuid& callId() const;

            void currentService( const std::string& service);
            const std::string& currentService() const;

         private:

            int allocateCallingDescriptor();


            typedef State::pending_calls_type pending_calls_type;
            typedef State::reply_cache_type reply_cache_type;

            Context();

            reply_cache_type::iterator find( int callDescriptor);

            reply_cache_type::iterator fetch( int callDescriptor);

            reply_cache_type::iterator add( message::service::Reply&& reply);

            message::service::name::lookup::Reply serviceQueue( const std::string& service);


            void consume();


            //ipc::send::Queue& m_brokerQueue = ipc::getBrokerQueue();
            //ipc::receive::Queue& m_receiveQueue = ipc::getReceiveQueue();


            State m_state;

            int m_callingDescriptor = 10;

         };
      } // calling
	} // common
} // casual


#endif /* CASUAL_CALLING_CONTEXT_H_ */
