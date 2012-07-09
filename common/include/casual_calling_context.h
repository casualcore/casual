//!
//! casual_calling_context.h
//!
//! Created on: Jun 16, 2012
//!     Author: Lazan
//!

#ifndef CASUAL_CALLING_CONTEXT_H_
#define CASUAL_CALLING_CONTEXT_H_


#include "casual_queue.h"

#include "casual_utility_platform.h"

#include <set>
#include <map>


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
				typedef utility::platform::seconds_type seconds_type;

				int callDescriptor;
				seconds_type called;
				seconds_type timeout;

				bool operator < ( const Pending& rhs) const
				{
					return callDescriptor < rhs.callDescriptor;
				}
			};
		}

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



			typedef std::set< internal::Pending> pending_calls_type;
			typedef std::map< int, message::ServiceReply> reply_cache_type;

			Context();

			reply_cache_type::iterator find( int callDescriptor);

			reply_cache_type::iterator fetch( int callDescriptor);

			reply_cache_type::iterator add( message::ServiceReply& reply);


			void consume();


			ipc::send::Queue m_brokerQueue;
			ipc::receive::Queue m_receiveQueue;


			pending_calls_type m_pendingReplies;


			reply_cache_type m_replyCache;

			int m_callingDescriptor;

		};
	}
}


#endif /* CASUAL_CALLING_CONTEXT_H_ */
