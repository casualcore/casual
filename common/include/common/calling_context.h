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
            typedef std::vector< int> pending_calls_type;
            typedef std::deque< message::service::Reply> reply_cache_type;
            using cache_range = decltype( range::make( reply_cache_type().begin(), reply_cache_type().end()));

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

            int canccel( int cd);

            void clean();

            void callId( const common::Uuid& uuid);
            const common::Uuid& callId() const;

            void currentService( const std::string& service);
            const std::string& currentService() const;

         private:

            State::pending_calls_type::iterator reserveDescriptor();

            typedef State::reply_cache_type reply_cache_type;


            using cache_range = State::cache_range;

            Context();

            cache_range find( int callDescriptor);

            cache_range fetch( int callDescriptor);

            cache_range add( message::service::Reply&& reply);

            void consume();

            State m_state;

         };
      } // calling
	} // common
} // casual


#endif /* CASUAL_CALLING_CONTEXT_H_ */
