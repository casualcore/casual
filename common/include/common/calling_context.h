//!
//! casual_calling_context.h
//!
//! Created on: Jun 16, 2012
//!     Author: Lazan
//!

#ifndef CASUAL_CALLING_CONTEXT_H_
#define CASUAL_CALLING_CONTEXT_H_


#include "common/queue.h"

#include "utility/platform.h"

#include <set>
#include <unordered_map>
#include <unordered_set>

namespace casual
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
				typedef utility::platform::seconds_type seconds_type;

				int callDescriptor;
				seconds_type called;
				seconds_type timeout;


			};
		}


		struct State
      {
		   typedef std::unordered_set< int> pending_calls_type;
		   typedef std::unordered_map< int, message::ServiceReply> reply_cache_type;

         pending_calls_type pendingCalls;
         reply_cache_type replyCache;

         int currentCallingDescriptor;
      };

		class Context
		{
		public:
			static Context& instance();

			int allocateCallingDescriptor();


			int asyncCall( const std::string& service, char* idata, long ilen, long flags);

			int getReply( int* idPtr, char** odata, long& olen, long flags);

			void clean();

		private:

			friend class casual::server::Context;

			ipc::send::Queue& brokerQueue();
			ipc::receive::Queue& receiveQueue();



			typedef State::pending_calls_type pending_calls_type;
			typedef State::reply_cache_type reply_cache_type;

			Context();

			reply_cache_type::iterator find( int callDescriptor);

			reply_cache_type::iterator fetch( int callDescriptor);

			reply_cache_type::iterator add( message::ServiceReply& reply);

			message::ServiceResponse serviceQueue( const std::string& service);


			void consume();


			ipc::send::Queue m_brokerQueue;
			ipc::receive::Queue m_receiveQueue;


			State m_state;

			int m_callingDescriptor;

		};
	}
}


#endif /* CASUAL_CALLING_CONTEXT_H_ */
