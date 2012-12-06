//!
//! casual_calling_context_implementation.h
//!
//! Created on: Jul 26, 2012
//!     Author: Lazan
//!

#ifndef CASUAL_CALLING_CONTEXT_IMPLEMENTATION_H_
#define CASUAL_CALLING_CONTEXT_IMPLEMENTATION_H_


//#include "common/calling_context.h"
//#include "common/message.h"

/*
namespace casual
{
   namespace callee
   {
      namespace state
      {

         namespace internal
         {

            template< typename Q, typename T>
            typename Q::result_type timeoutWrapper( Q& queue, T& value)
            {
               try
               {
                  return queue( value);
               }
               catch( const exception::signal::Timeout&)
               {
                  PendingTimeout::instance().timeout();
                  return timeoutWrapper( queue, value);
               }
            }

            template< typename Q, typename T>
            typename Q::result_type timeoutWrapper( Q& queue, T& value, int callDescriptor)
            {
               try
               {
                  return queue( value);
               }
               catch( const exception::signal::Timeout&)
               {
                  PendingTimeout::instance().timeout();
                  PendingTimeout::instance().check( callDescriptor);

                  return timeoutWrapper( queue, value, callDescriptor);
               }
            }


         }

         message::ServiceResponse Context::serviceQueue( const std::string& service)
         {
            //
            // Get a queue corresponding to the service
            //
            message::ServiceRequest serviceLookup;
            serviceLookup.requested = service;
            serviceLookup.server.queue_key = m_receiveQueue.getKey();

            queue::blocking::Writer broker( m_brokerQueue);
            internal::timeoutWrapper( broker, serviceLookup);


            message::ServiceResponse result;
            queue::blocking::Reader reader( m_receiveQueue);
            internal::timeoutWrapper( reader, result);

            return result;

         }


         int asyncCall( const std::string& service, buffer::Buffer& buffer, long flags)
         {

            // TODO validate



            const int callDescriptor = allocateCallingDescriptor();

            const utility::platform::seconds_type time = utility::environment::getTime();


            //
            // Get a queue corresponding to the service
            //
            message::ServiceResponse serviceResponse = serviceQueue( service);

            if( serviceResponse.server.empty())
            {
               throw exception::xatmi::service::NoEntry( service);
            }

            //
            // Keep track of (ev.) coming timeouts
            //
            local::PendingTimeout::instance().add(
                  local::Timeout( callDescriptor, serviceResponse.service.timeout, time));


            //
            // Call the service
            //
            message::ServiceCall messageCall( buffer);
            messageCall.callDescriptor = callDescriptor;
            messageCall.reply.queue_key = m_receiveQueue.getKey();

            messageCall.service = serviceResponse.service;



            ipc::send::Queue callQueue( serviceResponse.server.front().queue_key);
            queue::blocking::Writer callWriter( callQueue);

            local::timeoutWrapper( callWriter, messageCall, callDescriptor);


            //
            // Add the descriptor to pending
            //
            m_pendingCalls.insert( callDescriptor);

            return callDescriptor;
         }



      } // state

   } // calling

} // casual
*/



#endif /* CASUAL_CALLING_CONTEXT_IMPLEMENTATION_H_ */
