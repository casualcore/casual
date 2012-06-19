//!
//! casual_calling_context.cpp
//!
//! Created on: Jun 16, 2012
//!     Author: Lazan
//!

#include "casual_calling_context.h"

#include "casual_utility_environment.h"

#include "casual_message.h"
#include "casual_queue.h"
#include "casual_error.h"




namespace casual
{

	namespace calling
	{

 	    Context& Context::instance()
		{
 	    	static Context singleton;
 	    	return singleton;
		}

 	    int Context::allocateCallingDescriptor()
 	    {
 	    	static int callingDescriptor = 10;
 	    	return callingDescriptor++;
 	    }

 	    int Context::asyncCall(const std::string& service, char* idata, long ilen, long flags)
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
 	    		serviceLookup.server.queue_key = m_localQueue.getKey();

 	    		queue::Writer broker( m_brokerQueue);
 	    		broker( serviceLookup);

 	    		message::ServiceResponse serviceResponse;
 	    		queue::blocking::Reader reader( m_localQueue);
 	    		reader( serviceResponse);

 	    		if( serviceResponse.server.empty())
 	    		{
 	    			throw exception::NotReallySureWhatToNameThisException();
 	    		}

 	    		//
 	    		// Call the service
 	    		//
 	    		message::ServiceCall messageCall( buffer);
 	    		messageCall.callCorrelation =  allocateCallingDescriptor();
 	    		messageCall.reply.queue_key = m_localQueue.getKey();

 	    		messageCall.service = serviceResponse.requested;

 	    		//
 	    		// Store the timeout-information
 	    		//
 	    		internal::Pending pendingReply;
 	    		pendingReply.callDescriptor = messageCall.callCorrelation;
 	    		pendingReply.called = utility::environment::getTime();
 	    		m_pendingReplies.push_back( pendingReply);


 	    		ipc::send::Queue callQueue( serviceResponse.server.front().queue_key);
 	    		queue::Writer callWriter( callQueue);

 	    		callWriter( messageCall);

 	    	}
 	    	catch( ...)
 	    	{
 	    		return error::handler();
 	    	}
 	    	return 0;
 	    }


 	   int Context::getReply( int& idPtr, char** odata, long& olen, long flags)
 	   {
 		    //
			// get the buffer
			//
			buffer::Buffer& buffer = buffer::Context::instance().getBuffer( *odata);

			// TODO do stuff :)

			return 0;
 	   }

 	   Context::Context()
 	   	   : m_brokerQueue( ipc::getBrokerQueue())
 	   {

 	   }
	}

}
