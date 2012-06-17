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


namespace casual
{

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
			};
		}

		class Context
		{
		public:
			static Context& instance();

			int allocateCallingDescriptor();


			int asyncCall( const std::string& service, char* idata, long ilen, long flags);

			int getReply( int& idPtr, char** odata, long& olen, long flags);

		private:
			Context();

			ipc::send::Queue m_brokerQueue;
			queue::Writer m_brokerWriter;

			ipc::receive::Queue m_localQueue;
			queue::Reader m_localReader;


			std::vector< internal::Pending> m_pendingReplies;
			std::vector< message::ServiceReply> m_replyCache;


		};
	}
}


#endif /* CASUAL_CALLING_CONTEXT_H_ */
