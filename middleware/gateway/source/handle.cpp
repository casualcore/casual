//!
//! casual 
//!

#include "gateway/handle.h"
#include "gateway/common.h"

#include "common/exception/casual.h"

namespace casual
{
   namespace gateway
   {
      namespace handle
      {

         Disconnect::Disconnect( std::thread& thread) : m_thread( thread) {}

         void Disconnect::operator() ( message_type& message)
         {
            Trace trace{ "gateway::handle::Disconnect::operator()"};

            if( message.reason == message_type::Reason::disconnect && message.remote.id)
            {
               common::log::category::information << "disconnect from domain: " << message.remote << '\n';
            }

            //
            // We got a disconnect from the worker thread, we wait for it
            // to finish
            //
            m_thread.join();

            // TODO: we may need to distinguish disconnect from shutdown...
            throw common::exception::casual::Shutdown{ "disconnected"};
         }



      } // handle

   } // gateway

} // casual
